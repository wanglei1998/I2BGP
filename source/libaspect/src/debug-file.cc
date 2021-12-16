#include "debug-file.h"
#include "reader.h"
#include "dwarf2-info.h"
#include "dwarf2-constants.h"
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <string.h>


#define ERROR_REPORT(x)

namespace aspcpp {

DebugFile::DebugFile (std::string filename)
{
  int fd;
  struct stat stat_buf;

  fd = open (filename.c_str (), O_RDONLY);
  if (fd == -1)
    {
      std::cerr << "error opening \"" << filename << "\"" << std::endl;
      return;
    }
  if (fstat (fd, &stat_buf) == -1)
    {
      std::cerr << "unable to stat \"" << filename << "\"" << std::endl;
      close (fd);
      return;
    }
  m_dataSize = stat_buf.st_size;
  m_data = (uint8_t *)mmap (0, stat_buf.st_size, PROT_READ, MAP_SHARED, fd, 0);
  close (fd);

  m_reader1 = CreateReader (m_data, m_dataSize);
  m_reader2 = CreateReader (m_data, m_dataSize);

  InitializeSection (m_data, ".debug_info", &m_infoStart, &m_infoEnd);
  InitializeSection (m_data, ".debug_str", &m_strStart, &m_strEnd);
  InitializeSection (m_data, ".debug_abbrev", &m_abbrevStart, &m_abbrevEnd);
}
DebugFile::~DebugFile ()
{
  delete m_reader1;
  delete m_reader2;
}
struct reader *
DebugFile::CreateReader (uint8_t *data, uint64_t size)
{
  struct reader *reader = new struct reader ();
  reader_initialize (reader, data, size);
  ElfW(Ehdr) *header = (ElfW(Ehdr) *)data;
  if (header->e_ident[EI_DATA] == ELFDATA2LSB)
    {
      reader_set_lsb (reader);
    }
  else
    {
      reader_set_msb (reader);
    }
  return reader;
}
void
DebugFile::InitializeSection (uint8_t *data, const char *name, 
			      uint64_t *start, uint64_t *end)
{
  ElfW(Shdr) *section = FindSectionByName (data, name);
  if (section != 0)
    {
      *start = section->sh_offset;
      *end = section->sh_offset + section->sh_size;
    }
  else
    {
      *start = 0;
      *end = 0;
    }
}
ElfW(Shdr) *
DebugFile::FindSectionByName (uint8_t *data, const char *name)
{
  ElfW(Ehdr) *header = (ElfW(Ehdr) *)data;
  uint32_t name_section_index;
  ElfW(Shdr) *section = (ElfW(Shdr) *)(data + header->e_shoff);
  if (header->e_shstrndx < SHN_LORESERVE)
    {
      name_section_index = header->e_shstrndx; 
    } 
  else if (header->e_shstrndx == SHN_XINDEX)
    {
      /* the index number of the section which contains 
       * the strings for the section names is stored in 
       * the sh_link field of the section header number 
       * zero.
       */
      name_section_index = section->sh_link;
    } 
  else
    {
      ERROR_REPORT ("invalid string table index");
      return 0;
    }
  char *strBase = (char *)(data + section[name_section_index].sh_offset);
  for (uint32_t i = 0; i < header->e_shnum; i++)
    {
      if (strcmp ((char*)(strBase + section[i].sh_name), name) == 0)
	{
	  return &section[i];
	}
    }
  return 0;
}

void 
DebugFile::Iterate (Iterator *iterator)
{
  if (m_infoStart == m_infoEnd
      || m_abbrevStart == m_abbrevEnd
      || m_strStart == m_strEnd)
    {
      return;
    }
  struct MyDwIter
  {
    struct dwarf2_info_iterator dw_i;
    static void StartCuh (struct dwarf2_info_iterator *iter, const struct dwarf2_info_cuh *cuh, uint64_t pos)
    {}
    static void EndCuh (struct dwarf2_info_iterator *i, const struct dwarf2_info_cuh *cuh)
    {
      struct MyDwIter *self = (struct MyDwIter *)i;
      self->m_iterator->EndFile ();
    }
    static void EndChildren (struct dwarf2_info_iterator *i, const struct dwarf2_info_cuh *cuh)
    {
      //std::cout << "end children" << std::endl;
      struct MyDwIter *self = (struct MyDwIter *)i;
      enum DwType state = self->m_stack.back ();
      switch (state)
	{
	case CU:
	case FUNCTION:
	case UNKNOWN:
	  break;
	case CLASS:
	  self->m_iterator->EndClass ();
	  break;
	case NAMESPACE:
	  self->m_iterator->EndNamespace ();
	  break;
	}
      self->m_stack.pop_back ();
    }
    static void Entry (struct dwarf2_info_iterator *i, const struct dwarf2_info_cuh *cuh,
		       const struct dwarf2_info_entry *entry, uint64_t pos, bool hasChildren)
    {
      struct MyDwIter *self = (struct MyDwIter *)i;
      enum DwType state;
      //dwarf2_info_entry_print (entry, &self->m_reader1->reader);
      switch (entry->tag)
	{
	case DW_TAG_COMPILE_UNIT:
	  self->m_iterator->StartFile (self->ReadEntryName (cuh, entry));
	  state = CU;
	  break;
	case DW_TAG_CLASS_TYPE:
	  self->m_iterator->StartClass (self->ReadEntryName (cuh, entry));
	  state = CLASS;
	  if (!hasChildren)
	    {
	      self->m_iterator->EndClass ();
	    }
	  break;
	case DW_TAG_NAMESPACE:
	  self->m_iterator->StartNamespace (self->ReadEntryName (cuh, entry));
	  state = NAMESPACE;
	  if (!hasChildren)
	    {
	      //std::cout << "no children ?" << std::endl;
	      self->m_iterator->EndNamespace ();
	    }
	  break;
	case DW_TAG_SUBPROGRAM:
	  state = FUNCTION;
	  if (entry->used & DW2_INFO_ATTR_LOW_PC)
	    {
	      //std::cout << "fn=" << self->ReadEntryName (cuh, entry) << std::endl;
	      self->m_iterator->Function (self->ReadEntryName (cuh, entry), entry->low_pc);
	    }
	  break;
	default:
	  state = UNKNOWN;
	  break;
	}
      if (hasChildren)
	{
	  self->m_stack.push_back (state);
	}
    }
    std::string ReadName (uint64_t offset)
    {
      reader_seek (m_reader1, offset);
      int len = reader_read_str_len (m_reader1);
      uint8_t *str = (uint8_t *)malloc (len + 1);
      reader_seek (m_reader1, offset);
      reader_read_buffer (m_reader1, str, len + 1);
      std::string retval = (char*)str;
      free (str);
      return retval;
    }

    std::string  ReadEntryName (const struct dwarf2_info_cuh *org_cuh, 
				const struct dwarf2_info_entry *org_entry)
    {
      int64_t retval = dwarf2_info_entry_get_name (org_cuh, org_entry,
						   m_reader1, m_reader2);
      if (retval == -1)
	{
	  return "";
	}
      else
	{
	  return ReadName (retval);
	}
    }

    Iterator *m_iterator;
    std::list<enum DwType> m_stack;
    struct reader *m_reader1;
    struct reader *m_reader2;
  } i;
  i.dw_i.start_cuh = MyDwIter::StartCuh;
  i.dw_i.end_cuh = MyDwIter::EndCuh;
  i.dw_i.end_children = MyDwIter::EndChildren;
  i.dw_i.entry = MyDwIter::Entry;
  i.m_iterator = iterator;
  i.m_reader1 = m_reader1;
  i.m_reader2 = m_reader2;

  struct dwarf2_info info;
  dwarf2_info_initialize (&info, m_infoStart, m_infoEnd, m_strStart, m_abbrevStart, m_abbrevEnd);
  dwarf2_info_iterate (&info, &i.dw_i, m_reader1, m_reader2);
}

std::list<long> 
DebugFile::Lookup (std::string filename, std::string function)
{
  class MyIterator : public Iterator
  {
  public:
    MyIterator (std::string filename, std::string function)
      : m_search (false),
	m_targetFilename (filename),
	m_targetFunction (function),
	m_current ("")
    {
    }
    std::list<long> GetMatches (void) const
    {
      return m_matches;
    }
  private:
    virtual void StartFile (std::string filename)
    {
      if (m_targetFilename == "*"
	  || filename.find (m_targetFilename) != std::string::npos)
	{
	  m_search = true;
	}
    }
    virtual void EndFile (void)
    {
      m_search = false;
    }
    virtual void StartNamespace (std::string name)
    {
      if (!m_search)
	{
	  return;
	}
      m_stack.push_back (m_current);
      m_current += "::" + name;
      //std::cout << "start " << m_current << std::endl;
    }
    virtual void EndNamespace (void)
    {
      if (!m_search)
	{
	  return;
	}
      //std::cout << "end " << m_current << std::endl;
      m_current = m_stack.back ();
      m_stack.pop_back ();
    }
    virtual void StartClass (std::string name)
    {
      if (!m_search)
	{
	  return;
	}
      m_stack.push_back (m_current);
      m_current += "::" + name;
    }
    virtual void EndClass (void)
    {
      if (!m_search)
	{
	  return;
	}
      m_current = m_stack.back ();
      m_stack.pop_back ();
    }
    virtual void Function (std::string name, unsigned long start)
    {
      if (!m_search)
	{
	  return;
	}
      std::string current = m_current + "::" + name;
      //std::cout << current << " ?? " << m_targetFunction << std::endl;
      if (current == m_targetFunction)
	{
	  m_matches.push_back (start);
	}
    }
    bool m_search;
    std::string m_targetFilename;
    std::string m_targetFunction;
    std::string m_current;
    std::list<std::string> m_stack;
    std::list<long> m_matches;
  } iterator = MyIterator (filename, function);
  Iterate (&iterator);
  return iterator.GetMatches ();
}


} // namespace aspcpp
