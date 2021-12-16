#ifndef HOOK_MANAGER_H
#define HOOK_MANAGER_H
#include <list>
#include <stdint.h>
#include <string>

#include "load-map.h"
#include "breakpoint-manager.h"

namespace aspcpp {

class Context;
class DebugFile;

class HookManager
{
 public:
  struct Hook;
  typedef void (*HookFunction) (void);
  HookManager ();
  ~HookManager ();
  Hook *AddHookBySourceAndFunction (std::string source, std::string function, HookFunction hookFunction);
  Hook *AddHookByFunction (std::string function, HookFunction hookFunction);
  void DeleteHook (Hook *hook);

 private:
  void Update (void);
  void NotifyDeleted (struct LoadMap::Item item, struct Hook *hook);
  void NotifyAdded (DebugFile *file, struct LoadMap::Item item, struct Hook *hook);
  void AddBreakpoint (struct Hook *hook, LoadMap::Item item, long address);
  static bool BreakpointHandler (long address, struct BreakpointManager::Breakpoint *bp, 
				 void *context);

  std::list<Hook *> m_hooks;
  LoadMap m_currentLoadMap;
  struct BreakpointManager::Breakpoint m_loadMapNotifier;
};

} // namespace aspcpp

#endif /* HOOK_MANAGER_H */
