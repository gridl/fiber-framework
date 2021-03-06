#include "private/ff_common.h"

#include "private/ff_blocking_stack.h"
#include "private/ff_stack.h"
#include "private/ff_semaphore.h"

struct ff_blocking_stack
{
	struct ff_stack *simple_stack;
	struct ff_semaphore *producer_semaphore;
	struct ff_semaphore *consumer_semaphore;
};

struct ff_blocking_stack *ff_blocking_stack_create(int max_size)
{
	struct ff_blocking_stack *stack;

	ff_assert(max_size > 0);

	stack = (struct ff_blocking_stack *) ff_malloc(sizeof(*stack));
	stack->simple_stack = ff_stack_create();
	stack->producer_semaphore = ff_semaphore_create(0);
	stack->consumer_semaphore = ff_semaphore_create(max_size);

	return stack;
}

void ff_blocking_stack_delete(struct ff_blocking_stack *stack)
{
	ff_semaphore_delete(stack->consumer_semaphore);
	ff_semaphore_delete(stack->producer_semaphore);
	ff_stack_delete(stack->simple_stack);
	ff_free(stack);
}

void ff_blocking_stack_pop(struct ff_blocking_stack *stack, const void **data)
{
	ff_semaphore_down(stack->producer_semaphore);
	ff_stack_top(stack->simple_stack, data);
	ff_stack_pop(stack->simple_stack);
	ff_semaphore_up(stack->consumer_semaphore);
}

enum ff_result ff_blocking_stack_pop_with_timeout(struct ff_blocking_stack *stack, const void **data, int timeout)
{
	enum ff_result result;

	ff_assert(timeout > 0);

	result = ff_semaphore_down_with_timeout(stack->producer_semaphore, timeout);
	if (result == FF_SUCCESS)
	{
		ff_stack_top(stack->simple_stack, data);
		ff_stack_pop(stack->simple_stack);
		ff_semaphore_up(stack->consumer_semaphore);
	}
	else
	{
		ff_log_debug(L"cannot get data from the blocking stack=%p during timeout=%d", stack, timeout);
	}
	return result;
}

void ff_blocking_stack_push(struct ff_blocking_stack *stack, const void *data)
{
	ff_semaphore_down(stack->consumer_semaphore);
	ff_stack_push(stack->simple_stack, data);
	ff_semaphore_up(stack->producer_semaphore);
}

enum ff_result ff_blocking_stack_push_with_timeout(struct ff_blocking_stack *stack, const void *data, int timeout)
{
	enum ff_result result;

	ff_assert(timeout > 0);

	result = ff_semaphore_down_with_timeout(stack->consumer_semaphore, timeout);
	if (result == FF_SUCCESS)
	{
		ff_stack_push(stack->simple_stack, data);
		ff_semaphore_up(stack->producer_semaphore);
	}
	else
	{
		ff_log_debug(L"cannot put data=%p to the blocking stack=%p during timeout=%d", data, stack, timeout);
	}
	return result;
}
