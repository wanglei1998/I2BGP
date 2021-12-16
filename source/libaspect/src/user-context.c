#define _GNU_SOURCE 1 /* for REG_EIP */

#include <sys/ucontext.h>

#include "user-context.h"

#if __WORDSIZE == 64
#define IP_REGISTER REG_RIP
#else
#define IP_REGISTER REG_EIP
#endif

#define TF_N 8

unsigned long 
user_context_get_pc (ucontext_t *user_ctx)
{
  return user_ctx->uc_mcontext.gregs[IP_REGISTER];
}

void 
user_context_set_pc (ucontext_t *user_ctx, unsigned long pc)
{
  user_ctx->uc_mcontext.gregs[IP_REGISTER] = pc;
}

void user_context_enable_step (ucontext_t *user_ctx)
{
  user_ctx->uc_mcontext.gregs[REG_EFL] |= (1<<TF_N);
}

void user_context_disable_step (ucontext_t *user_ctx)
{
  user_ctx->uc_mcontext.gregs[REG_EFL] &= ~(1<<TF_N);
}
