#include "hook-manager.h"
#include "load-map.h"
#include "breakpoint-manager.h"
#include "debug-file.h"
#include <iostream>

namespace aspcpp {
namespace {

typedef std::list<struct BreakpointManager::Breakpoint> BreakpointList;

struct File
{
  struct LoadMap::Item file;
  BreakpointList breakpoints;
};

typedef std::list<struct File> FileList;

}

struct HookManager::Hook
{
  FileList fileList;
  HookManager::HookFunction function;
  std::string source;
  std::string functionName;
};


HookManager::HookManager ()
 : m_currentLoadMap (LoadMap::Parse ())
{
  long notifier = LoadMap::GetChangeNotifierAddress ();
  BreakpointManager::SetHandler (&HookManager::BreakpointHandler, (void*)this);
  m_loadMapNotifier = BreakpointManager::Enable (notifier);
}

HookManager::~HookManager ()
{
  while (!m_hooks.empty ())
    {
      Hook *hook = m_hooks.front ();
      DeleteHook (hook);
    }
  BreakpointManager::Disable (m_loadMapNotifier);
}


bool
HookManager::BreakpointHandler (long address, struct BreakpointManager::Breakpoint *bp,
				void *context)
{
  HookManager *self = (HookManager *)context;
  if (self->m_loadMapNotifier.address == address)
    {
      self->Update ();
      *bp = self->m_loadMapNotifier;
      return true;
    }
  else
    {
      for (std::list<Hook *>::iterator i = self->m_hooks.begin (); i != self->m_hooks.end (); ++i)
	{
	  Hook *hook = *i;
	  for (FileList::iterator j = hook->fileList.begin (); j != hook->fileList.end (); ++j)
	    {
	      struct File file = *j;
	      for (BreakpointList::iterator k = file.breakpoints.begin (); 
		   k != file.breakpoints.end (); ++k)
		{
		  if (k->address == address)
		    {
		      *bp = *k;
		      hook->function ();
		      return true;
		    }
		}
	    }
	}
    }
  return false;
}

void
HookManager::Update (void)
{
  LoadMap newLoadMap = LoadMap::Parse ();
  LoadMap deleted = newLoadMap.GetEntriesDeleted (m_currentLoadMap);
  for (LoadMap::Iterator i = deleted.Begin (); i != deleted.End (); ++i)
    {
      for (std::list<Hook *>::iterator j = m_hooks.begin (); j != m_hooks.end (); ++j)
	{
	  NotifyDeleted (*i, *j);
	}
    }
  LoadMap added = newLoadMap.GetEntriesAdded (m_currentLoadMap);
  for (LoadMap::Iterator i = added.Begin (); i != added.End (); ++i)
    {
      DebugFile file = DebugFile (i->filename);
      for (std::list<Hook *>::iterator j = m_hooks.begin (); j != m_hooks.end (); ++j)
	{
	  NotifyAdded (&file, *i, *j);
	}
    }
  m_currentLoadMap = newLoadMap;
}

void HookManager::NotifyDeleted (struct LoadMap::Item item, struct Hook *hook)
{
  // if there are breakpoints in a load map item which got deleted,
  // just behave as if they had disappeared and delete them from the list.
  FileList::iterator j = hook->fileList.begin ();
  while (j != hook->fileList.end ())
    {
      LoadMap::Item other = j->file;
      if (other.filename == item.filename &&
	  other.load_base == item.load_base)
	{
	  j = hook->fileList.erase (j);
	}
      else
	{
	  j++;
	}
    }
}

void 
HookManager::NotifyAdded (DebugFile *file, struct LoadMap::Item item, struct Hook *hook)
{
  std::list<long> matches = file->Lookup (hook->source, hook->functionName);
  if (!matches.empty ())
    {
      for (std::list<long>::iterator j = matches.begin (); j != matches.end (); ++j)
	{
	  AddBreakpoint (hook, item, *j);
	}
    }  
}

void
HookManager::AddBreakpoint (struct Hook *hook, struct LoadMap::Item item, long address)
{
  FileList::iterator i = hook->fileList.begin ();
  while (i != hook->fileList.end ())
    {
      if (i->file.load_base == item.load_base &&
	  i->file.filename == item.filename)
	{
	  break;
	}
      ++i;
    }
  if (i == hook->fileList.end ())
    {
      struct File file;
      file.file = item;
      hook->fileList.push_back (file);
      AddBreakpoint (hook, item, address);
      return;
    }
  struct BreakpointManager::Breakpoint bp = BreakpointManager::Enable (i->file.load_base + address);
  i->breakpoints.push_back (bp);	      
}

HookManager::Hook *
HookManager::AddHookBySourceAndFunction (std::string source, std::string functionName, 
					 HookFunction hookFunction)
{
  Hook *hook = new Hook ();
  hook->source = source;
  hook->functionName = functionName;
  hook->function = hookFunction;
  m_hooks.push_back (hook);
  Update ();
  for (LoadMap::Iterator i = m_currentLoadMap.Begin (); i != m_currentLoadMap.End (); ++i)
    {
      DebugFile file = DebugFile (i->filename);
      NotifyAdded (&file, *i, hook);
    }
  return hook;
}

HookManager::Hook *
HookManager::AddHookByFunction (std::string function, HookFunction hookFunction)
{
  return AddHookBySourceAndFunction ("*", function, hookFunction);
}

void 
HookManager::DeleteHook (Hook *hook)
{
  Update ();
  for (std::list<Hook *>::iterator i = m_hooks.begin (); i != m_hooks.end (); i++)
    {
      if ((*i) == hook)
	{
	  m_hooks.erase (i);
	  delete hook;
	  return;
	}
    }
}


} // namespace aspcpp
