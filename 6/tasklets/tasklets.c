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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/slab.h>

MODULE_AUTHOR("Kirill Yatsenko <kirill.yatsenko@globallogic.com>");
MODULE_DESCRIPTION("HM #6");
MODULE_LICENSE("Dual BSD/GPL");

#define MS_TO_NS(x)	((x) * NSEC_PER_MSEC)

struct hrtimer hr_timer;
struct tasklet_struct tlet;
struct tasklet_struct hi_tlet;

struct work_struct work;
struct delayed_work delayed_work;

static unsigned long delay_in_ms = 200L;

static void workqueue_cb(struct work_struct *work)
{
	unsigned int ms = jiffies_to_msecs(jiffies);

	pr_info("called (%dms)\n", ms);
}

static void tasklet_cb(unsigned long arg)
{
	unsigned long delay;
	char *message = (char *)arg;

	pr_info("%s: %lu\n", message, jiffies);

	delay = msecs_to_jiffies(delay_in_ms);

	schedule_work(&work);
	schedule_delayed_work(&delayed_work, delay);
}

static enum hrtimer_restart hrt_cb( struct hrtimer *timer)
{
	pr_info("hrt_cb called (%llu).\n",
		ktime_to_ms(timer->base->get_time()));

	pr_info("Scheduling tasklets...\n");

	tasklet_schedule(&tlet);
	tasklet_hi_schedule(&hi_tlet);

	return HRTIMER_NORESTART;
}

static void hrt_init(void)
{
	ktime_t ktime;

	pr_info("Setuping hr timer\n");

	ktime = ktime_set(0, MS_TO_NS(delay_in_ms));
	hrtimer_init(&hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hr_timer.function = &hrt_cb;

	pr_info("Starting timer to fire in %llu ms (%lu)\n",
		ktime_to_ms(hr_timer.base->get_time()) + delay_in_ms, jiffies);

	hrtimer_start(&hr_timer, ktime, HRTIMER_MODE_REL);
}

static int __init tasklets_init(void)
{
	char *regular = "regular";
	char *hi = "hi";

	INIT_WORK(&work, workqueue_cb);
	INIT_DELAYED_WORK(&delayed_work, workqueue_cb);

	tasklet_init(&tlet, tasklet_cb, (unsigned long)regular);
	tasklet_init(&hi_tlet, tasklet_cb, (unsigned long)hi);

	hrt_init();

	return 0;
}

static void __exit tasklets_exit(void)
{
	int ret;

	tasklet_kill(&tlet);
	tasklet_kill(&hi_tlet);

	ret = hrtimer_cancel(&hr_timer);
	if (ret)
		pr_info("The timer was still in use...\n");

	flush_scheduled_work();
}

module_init(tasklets_init);
module_exit(tasklets_exit);
