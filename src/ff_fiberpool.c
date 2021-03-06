#include "private/ff_common.h"

#include "private/ff_fiberpool.h"
#include "private/ff_blocking_stack.h"
#include "private/ff_fiber.h"

struct ff_fiberpool
{
	struct ff_blocking_stack *pending_tasks;
	struct ff_fiber **fibers;
	int max_fibers_cnt;
	int running_fibers_cnt;
	int busy_fibers_cnt;
};

struct fiberpool_task
{
	ff_fiberpool_func func;
	void *ctx;
};

static void generic_fiberpool_func(void *ctx)
{
	struct ff_fiberpool *fiberpool;
	struct ff_blocking_stack *pending_tasks;

	fiberpool = (struct ff_fiberpool *) ctx;
	pending_tasks = fiberpool->pending_tasks;
	for (;;)
	{
		struct fiberpool_task *task;

		ff_assert(fiberpool->busy_fibers_cnt > 0);
		ff_assert(fiberpool->busy_fibers_cnt <= fiberpool->running_fibers_cnt);
		ff_assert(fiberpool->running_fibers_cnt <= fiberpool->max_fibers_cnt);

		fiberpool->busy_fibers_cnt--;
		ff_blocking_stack_pop(pending_tasks, (const void **) &task);
		if (task == NULL)
		{
			break;
		}
		fiberpool->busy_fibers_cnt++;

		task->func(task->ctx);
		ff_free(task);
	}
	fiberpool->running_fibers_cnt--;
}

static void add_worker_fiber(struct ff_fiberpool *fiberpool)
{
	struct ff_fiber *worker_fiber;

	worker_fiber = ff_fiber_create(generic_fiberpool_func, 0);
	fiberpool->fibers[fiberpool->running_fibers_cnt] = worker_fiber;
	fiberpool->running_fibers_cnt++;
	fiberpool->busy_fibers_cnt++;
	ff_fiber_start(worker_fiber, fiberpool);
}

struct ff_fiberpool *ff_fiberpool_create(int max_fibers_cnt)
{
	struct ff_fiberpool *fiberpool;

	ff_assert(max_fibers_cnt > 0);

	fiberpool = (struct ff_fiberpool *) ff_malloc(sizeof(*fiberpool));
	fiberpool->pending_tasks = ff_blocking_stack_create(max_fibers_cnt);
	fiberpool->fibers = (struct ff_fiber **) ff_calloc(max_fibers_cnt, sizeof(fiberpool->fibers[0]));
	fiberpool->max_fibers_cnt = max_fibers_cnt;
	fiberpool->running_fibers_cnt = 0;
	fiberpool->busy_fibers_cnt = 0;

	return fiberpool;
}

void ff_fiberpool_delete(struct ff_fiberpool *fiberpool)
{
	struct ff_blocking_stack *pending_tasks;
	struct ff_fiber **fibers;
	int i;
	int running_fibers_cnt;

	pending_tasks = fiberpool->pending_tasks;
	running_fibers_cnt = fiberpool->running_fibers_cnt;
	for (i = 0; i < running_fibers_cnt; i++)
	{
		ff_blocking_stack_push(pending_tasks, NULL);
	}
	fibers = fiberpool->fibers;
	for (i = 0; i < running_fibers_cnt; i++)
	{
		struct ff_fiber *fiber;

		fiber = fibers[i];
		ff_fiber_join(fiber);
		ff_fiber_delete(fiber);
	}
	ff_assert(fiberpool->busy_fibers_cnt == 0);
	ff_assert(fiberpool->running_fibers_cnt == 0);

	ff_free(fibers);
	ff_blocking_stack_delete(pending_tasks);
	ff_free(fiberpool);
}

void ff_fiberpool_execute_async(struct ff_fiberpool *fiberpool, ff_fiberpool_func func, void *ctx)
{
	struct fiberpool_task *task;

	ff_assert(fiberpool->busy_fibers_cnt >= 0);
	ff_assert(fiberpool->busy_fibers_cnt <= fiberpool->running_fibers_cnt);
	ff_assert(fiberpool->running_fibers_cnt <= fiberpool->max_fibers_cnt);

	task = (struct fiberpool_task *) ff_malloc(sizeof(*task));
	task->func = func;
	task->ctx = ctx;
	ff_blocking_stack_push(fiberpool->pending_tasks, task);

	if (fiberpool->running_fibers_cnt < fiberpool->max_fibers_cnt)
	{
		if (fiberpool->busy_fibers_cnt == fiberpool->running_fibers_cnt)
		{
			add_worker_fiber(fiberpool);
		}
	}
	else
	{
		ff_log_debug(L"fiberpool=%p already has maximum size %d, so it cannot contain new fibers", fiberpool, fiberpool->max_fibers_cnt);
	}
}
