#ifndef USER_CONTEXT_H
#define USER_CONTEXT_H

#include <sys/ucontext.h>

#ifdef __cplusplus
extern "C" {
#endif

unsigned long user_context_get_pc (ucontext_t *user_ctx);
void user_context_set_pc (ucontext_t *user_ctx, unsigned long pc);

void user_context_enable_step (ucontext_t *user_ctx);
void user_context_disable_step (ucontext_t *user_ctx);

#ifdef __cplusplus
}
#endif

#endif /* USER_CONTEXT_H */
