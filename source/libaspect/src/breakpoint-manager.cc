#include "breakpoint-manager.h"
#include "user-context.h"

#include <sys/mman.h>
#include <limits.h> /* for PAGESIZE */
#include <errno.h>
#include <string.h>
#include <iostream>
#include <signal.h>
#include <stdlib.h>

#ifndef PAGESIZE
#define PAGESIZE 4096
#endif

namespace aspcpp {

struct BreakpointManagerImpl
{
  bool (*handler) (long pc, struct BreakpointManager::Breakpoint *bp, void *);
  void *context;
  struct sigaction oldAction;
  struct user_ctx *ctx;
  struct BreakpointManager::Breakpoint currentBreakpoint;
  bool isInitialized;
};

static int
WriteEnable (long address)
{
  int status;
  long addr_page_aligned;
  addr_page_aligned = address & ~(PAGESIZE-1);
  status = mprotect ((void *)addr_page_aligned, PAGESIZE, PROT_READ | PROT_EXEC | PROT_WRITE);
  if (status == -1) 
    {
      std::cerr << "mprotect failed: " << strerror (errno) << std::endl;
      goto error;
    }
  return 0;
 error:
  return -1;
}

static int
WriteDisable (long address)
{
  int status;
  long addr_page_aligned;
  addr_page_aligned = address & ~(PAGESIZE-1);
  status = mprotect ((void *)addr_page_aligned, PAGESIZE, PROT_READ | PROT_EXEC);
  if (status == -1)
    {
      std::cerr << "mprotect failed: " << strerror (errno) << std::endl;
      goto error;
    }
  return 0;
 error:
  return -1;
}

uint8_t
BreakpointManager::Write (long address, uint8_t data)
{
  volatile uint8_t *p = (uint8_t *)address;
  uint8_t old = p[0];
  if (WriteEnable (address) == -1)
    {
      goto error;
    }
  p[0] = data;
  if (WriteDisable (address) == -1)
    {
      goto error;
    }
  if (p[0] != data)
    {
      goto error;
    }
  return old;
 error:
  return 0;
}


struct BreakpointManager::Breakpoint 
BreakpointManager::Enable (long address)
{
  struct Breakpoint bp;
  bp.data = Write (address, 0xcc);
  bp.address = address;
  return bp;
}
void 
BreakpointManager::Disable (struct Breakpoint breakpoint)
{
  uint8_t old;
  old = Write (breakpoint.address, breakpoint.data);
  return;
}


/**
 * We are here thanks to the steping mode so 
 * We re-enable the breakpoint from this handler,
 * disable the stepping mode and re-enable the
 * original handler.
 */
void 
BreakpointManager::ReenableTrapHandler (int signo, siginfo_t *info, void *context)
{
  ucontext_t *user_ctx = (ucontext_t *)context;
  BreakpointManagerImpl *manager = Peek ();
  /* Disable step by step. */
  user_context_disable_step (user_ctx);
  /* re-enable old action handler. */
  if (sigaction (SIGTRAP, &manager->oldAction, NULL) == -1)
    {
      goto error;
    }
  /* re-enable breakpoint. */
  Enable (manager->currentBreakpoint.address);
 error:
  return;
}

/**
 * We have just hit a breakpoint so, notify the user and continue.
 */
void
BreakpointManager::NormalTrapHandler (int signo, siginfo_t *info, void *context)
{
  ucontext_t *user_ctx = (ucontext_t *)context;
  BreakpointManagerImpl *manager = Peek ();
  // get address at which the breakpoint occured
  long pc = user_context_get_pc (user_ctx) - 1;
  // invoke user handler and get back the breakpoint data structure
  struct Breakpoint bp;
  bool found = manager->handler (pc, &bp, manager->context);
  if (!found)
    {
      std::cerr << "Unable to find handler for trap. Probably a genuine program bug." << std::endl;
      abort ();
    }

  /* Restore the instruction previously there.
   */
  Disable (bp);
  /* Make sure to re-execute the instruction we just restored 
   * upon return to the user stack.
   */
  user_context_set_pc (user_ctx, user_context_get_pc (user_ctx)-1);

        
  /* install the re-enable action handler. */
  {
    struct sigaction action;
    action.sa_sigaction = ReenableTrapHandler;
    action.sa_flags = SA_SIGINFO;
    manager->currentBreakpoint = bp;
    if (sigaction (SIGTRAP, &action, &manager->oldAction) == -1)
      {
	std::cerr << "could not install trap signal handler" << std::endl;
	goto error;
      }
  }

  /* schedule a task breakpoint on the next instruction
   * to re-enable the breakpoint at that point.
   */
  user_context_enable_step (user_ctx);
 error:
  return;
}

void 
BreakpointManager::SetHandler (bool (*handler) (long, struct Breakpoint *, void *), void *context)
{
  Peek ()->handler = handler;
  Peek ()->context = context;
}

BreakpointManagerImpl *
BreakpointManager::Peek (void)
{
  static BreakpointManagerImpl manager;
  if (!manager.isInitialized)
    {
      struct sigaction action;
      action.sa_sigaction = &BreakpointManager::NormalTrapHandler;
      action.sa_flags = SA_SIGINFO;
      if (sigaction (SIGTRAP, &action, NULL) == -1)
	{
	  std::cerr << "could not install trap signal handler" << std::endl;
	}
      manager.isInitialized = true;
    }
  return &manager;
}

} // namespace aspcpp
