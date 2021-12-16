#ifndef BREAKPOINT_MANAGER_H
#define BREAKPOINT_MANAGER_H

#include <stdint.h>
#include <signal.h>

namespace aspcpp {

class BreakpointManagerImpl;

class BreakpointManager
{
public:
  struct Breakpoint
  {
    long address;
    uint8_t data;
  };
  static struct Breakpoint Enable (long address);
  static void Disable (struct Breakpoint breakpoint);
  static void SetHandler (bool (*handler) (long, struct Breakpoint *, void *), void *context);
private:
  static uint8_t Write (long address, uint8_t data);
  static void ReenableTrapHandler (int signo, siginfo_t *info, void *user_ctx);
  static void NormalTrapHandler (int signo, siginfo_t *info, void *user_ctx);
  static BreakpointManagerImpl *Peek (void);
};

} // namespace aspcpp

#endif /* BREAKPOINT_MANAGER_H */
