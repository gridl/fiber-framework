#ifndef FF_BLOCKING_STACK_PUBLIC_H
#define FF_BLOCKING_STACK_PUBLIC_H

#include "ff/ff_common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ff_blocking_stack;

FF_API struct ff_blocking_stack *ff_blocking_stack_create(int max_size);

FF_API void ff_blocking_stack_delete(struct ff_blocking_stack *stack);

FF_API void ff_blocking_stack_pop(struct ff_blocking_stack *stack, const void **data);

FF_API enum ff_result ff_blocking_stack_pop_with_timeout(struct ff_blocking_stack *stack, const void **data, int timeout);

FF_API void ff_blocking_stack_push(struct ff_blocking_stack *stack, const void *data);

FF_API enum ff_result ff_blocking_stack_push_with_timeout(struct ff_blocking_stack *stack, const void *data, int timeout);

#ifdef __cplusplus
}
#endif

#endif
