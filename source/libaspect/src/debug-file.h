#ifndef DEBUG_FILE_H
#define DEBUG_FILE_H

#include <elf.h>
#include <link.h>
#include <string>
#include <list>

extern "C" {
struct reader;
struct dwarf2_info_cuh;
struct dwarf2_info_entry;
}

namespace aspcpp {

class DebugFile
{
public:
  DebugFile (std::string filename);
  ~DebugFile ();
  std::list<long> Lookup (std::string filename, std::string function);
private:
  class Iterator 
  {
  public:
    virtual void StartFile (std::string filename) = 0;
    virtual void EndFile (void) = 0;
    virtual void StartNamespace (std::string name) = 0;
    virtual void EndNamespace (void) = 0;
    virtual void StartClass (std::string name) = 0;
    virtual void EndClass (void) = 0;
    virtual void Function (std::string name, unsigned long start) = 0;
  };
  enum DwType
  {
    CLASS,
    NAMESPACE,
    FUNCTION,
    CU,
    UNKNOWN
  };
  ElfW(Shdr) *FindSectionByName (uint8_t *data, const char *name);
  void InitializeSection (uint8_t *data, const char *name, 
			  uint64_t *start, uint64_t *end);
  struct reader *CreateReader (uint8_t *data, uint64_t size);
  void Iterate (Iterator *iterator);

  struct reader *m_reader1;
  struct reader *m_reader2;
  uint8_t *m_data;
  uint64_t m_dataSize;
  uint64_t m_infoStart;
  uint64_t m_infoEnd;
  uint64_t m_strStart;
  uint64_t m_strEnd;
  uint64_t m_abbrevStart;
  uint64_t m_abbrevEnd;
};

} // namespace aspcpp

#endif /* DEBUG_FILE_H */
