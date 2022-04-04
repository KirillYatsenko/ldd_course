/*
 * Copyright (c) 2017, GlobalLogic Ukraine LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by the GlobalLogic.
 * 4. Neither the name of the GlobalLogic nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GLOBALLOGIC UKRAINE LLC ``AS IS`` AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL GLOBALLOGIC UKRAINE LLC BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "linux/jiffies.h"
#include "linux/wait.h"
#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/delay.h>

#ifdef pr_fmt
#undef pr_fmt
#endif

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#define THREAD_NAME_FMT	"thread/%d"

MODULE_AUTHOR("Kirill Yatsenko <kirill.yatsenko@globallogic.com>");
MODULE_DESCRIPTION("HM #13");
MODULE_LICENSE("Dual BSD/GPL");

static unsigned long counter;
struct task_struct *tasks[5];

DEFINE_SPINLOCK(lock);
DECLARE_WAIT_QUEUE_HEAD(deinit_queue);

static int inc_thread(void *data)
{
	int ret;
	int thread_i;
	unsigned long delay = msecs_to_jiffies(5000);

	while (true) {
		spin_lock(&lock);
		pr_info("Global counter: %ld\n", ++counter);
		spin_unlock(&lock);

		ret = sscanf(current->comm, THREAD_NAME_FMT, &thread_i);
		if (ret != 1) {
			pr_err("Unable to get task number\n");
		} else {
			if (!(thread_i % 5))
				pr_info("=========================\n");

			pr_info("Thread number: %d\n", thread_i);
		}

		ret = wait_event_timeout(deinit_queue, kthread_should_stop(),
					 delay);
		if (ret) {
			pr_info("Stoping thread: '%s'\n", current->comm);
			return 0;
		}
	}

	return 0;
}

static void threads_array_deinit(void)
{
	int i;
	size_t array_size = ARRAY_SIZE(tasks);

	for (i = 0; i < array_size; i++)
		if (tasks[i])
			kthread_stop(tasks[i]);

	pr_info("Threads were deinited\n");
}

static int threads_array_init(void)
{
	int i;
	int ret;
	struct task_struct *thread;
	size_t array_size = ARRAY_SIZE(tasks);

	for (i = 0; i < array_size; i++) {
		thread = kthread_create(inc_thread, NULL, THREAD_NAME_FMT, i);
		if (IS_ERR(thread)) {
			ret = PTR_ERR(thread);
			pr_err("Error while creating thread: %d ret: %d\n", i,
			       ret);
			goto err;
		}

		tasks[i] = thread;
	}

	for (i = 0; i < array_size; i++) {
		ret = wake_up_process(tasks[i]);
		if (!ret) {
			pr_err("Unable to wake up thread: %d ret: %d\n", i,
			       ret);
			goto err;
		}
	}

	pr_info("Threads were created and waked up\n");

	return 0;

err:
	threads_array_deinit();
	return ret;
}

static int __init threads_module_init(void)
{
	int ret;

	ret = threads_array_init();
	if (ret) {
		pr_err("Unable to init threads list\n");
		return ret;
	}

	pr_info("Threads list inited\n");

	return 0;
}

static void __exit threads_module_deinit(void)
{
	threads_array_deinit();
	pr_info("Threads list deinited\n");
}

module_init(threads_module_init);
module_exit(threads_module_deinit);
