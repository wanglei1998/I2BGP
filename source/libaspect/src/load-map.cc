#include "load-map.h"

#include <iostream>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include <link.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>

namespace aspcpp {

LoadMap::Iterator LoadMap::Begin (void) const
{
  return m_items.begin ();
}
LoadMap::Iterator LoadMap::End (void) const
{
  return m_items.end ();
}

LoadMap::LoadMap ()
{}

LoadMap LoadMap::Parse (void)
{
  LoadMap map;
  static struct link_map *link_map = 0;
  if (link_map == 0)
    {
      link_map = GetLinkMap ();
      if (link_map == 0)
	{
	  return map;
	}
    }
  struct link_map *cur;
  for (cur = link_map; cur != 0; cur = cur->l_next)
    {
      struct Item item;
      item.filename = cur->l_name;
      if (item.filename == "" && cur->l_prev == 0)
	{
	  // this is the first entry so this is the main binary
	  item.filename = "/proc/self/exe";
	}
      else if (item.filename == "" && cur->l_prev != 0)
	{
	  // this is likely the vdso: nothing iteresting
	  continue;
	}
      item.load_base = cur->l_addr;
      map.m_items.push_back (item);
    }
  return map;
}

bool
LoadMap::Contains (struct Item item) const
{
  for (Iterator i = Begin (); i != End (); ++i)
    {
      if (item.filename == i->filename)
	{
	  return true;
	}
    }
  return false;
}

LoadMap 
LoadMap::GetEntriesAdded (LoadMap old) const
{
  LoadMap added;
  for (Iterator i = Begin (); i != End (); ++i)
    {
      if (!old.Contains (*i))
	{
	  added.m_items.push_back (*i);
	}
    }
  return added;
}
LoadMap 
LoadMap::GetEntriesDeleted (LoadMap old) const
{
  LoadMap deleted;
  for (Iterator i = old.Begin (); i != old.End (); ++i)
    {
      if (!Contains (*i))
	{
	  deleted.m_items.push_back (*i);
	}
    }
  return deleted;
}


struct link_map *
LoadMap::GetLinkMap (void)
{
  void * handle;
  struct link_map *map;
  dlerror ();
  handle = dlopen (0, RTLD_LAZY);
  if (handle == 0)
    {
      std::cout << "error getting handle for main binary: " << dlerror () << std::endl;
      goto error;
    }
  if (dlinfo (handle, RTLD_DI_LINKMAP, &map) == -1)
    {
      std::cout << "error getting link map: " << dlerror () << std::endl;
      dlclose (handle);
      goto error;
    }
  dlclose (handle);
  return map;
 error:
  return 0;
}

struct r_debug *
LoadMap::GetLdDebug(void)
{
  int fd = open ("/proc/self/auxv", O_RDONLY);
  ElfW(auxv_t) auxv;
  ElfW(Phdr) *phdr = 0;
  unsigned long phnum = 0;
  int status = read (fd, &auxv, sizeof(auxv));
  while (status == sizeof(auxv))
    {
      if (auxv.a_type == AT_PHDR)
        {
          phdr = (ElfW(Phdr) *)auxv.a_un.a_val;
        }
      else if (auxv.a_type == AT_PHNUM)
        {
	  phnum = auxv.a_un.a_val;
        }      
      status = read (fd, &auxv, sizeof(auxv));
    }
  close (fd);
  for (unsigned long i = 0; i < phnum; i++)
    {
      if (phdr[i].p_type == PT_DYNAMIC)
	{
	  ElfW(Dyn) *dyn = (ElfW(Dyn) *)phdr[i].p_vaddr;
	  for (ElfW(Dyn) *cur = dyn; cur->d_tag != DT_NULL; cur++)
	    {
	      if (cur->d_tag == DT_DEBUG)
		{
		  struct r_debug *r_debug;
		  r_debug = (struct r_debug *) cur->d_un.d_ptr;
		  return r_debug;
		}
	    }
	}
    }
  return 0;
}


long
LoadMap::GetChangeNotifierAddress (void)
{
  struct r_debug *debug = GetLdDebug ();
  return (long)debug->r_brk;
}

} // namespace aspcpp
