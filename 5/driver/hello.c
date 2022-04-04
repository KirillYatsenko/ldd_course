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
#include <linux/ktime.h>
#include <linux/slab.h>

MODULE_AUTHOR("Kirill Yatsenko <kirill.yatsenko@globallogic.com>");
MODULE_DESCRIPTION("HM #1");
MODULE_LICENSE("Dual BSD/GPL");

static uint count=1;
module_param(count,int,0660);

LIST_HEAD(time_history);

struct time_entry {
	ktime_t start;
	ktime_t end;
	struct list_head node;
};

static void release_time_history(void)
{
	struct time_entry *entry = NULL;
	struct time_entry *tmp = NULL;

	list_for_each_entry_safe(entry, tmp, &time_history, node) {
		kfree(entry);
		list_del(&entry->node);
	}
}

static int print_message(uint num)
{
	int rc = 0;
	struct time_entry *entry = NULL;

	if (!num || (num > 5 && num < 10)) {
		printk(KERN_WARNING "Count is: %d\n", num);
	}

	BUG_ON(num > 10);

	while (num--) {
		BUG_ON(num == 5);

		entry = kzalloc(sizeof(*entry), GFP_KERNEL);
		if (!entry) {
			rc = -ENOMEM;
			goto error;
		}

		entry->start = ktime_get();
		printk(KERN_INFO "Hello World!\n");
		entry->end = ktime_get();

		list_add_tail(&entry->node, &time_history);
	}

	return rc;

error:
	release_time_history();
	return rc;
}

static int __init hello_init(void)
{
	int rc = print_message(count);
	if (rc)
		printk(KERN_ERR "Unable to print message, rc: %d\n", rc);

	return rc;
}

static void __exit hello_exit(void)
{
	struct time_entry *entry = NULL;
	struct time_entry *tmp = NULL;

	pr_debug("printing time history\n");

	list_for_each_entry_safe(entry, tmp, &time_history, node) {
		pr_debug("time: %lld print duration: %lld\n", entry->start,
			 entry->end - entry->start);

		kfree(entry);
		list_del(&entry->node);
	}

	pr_debug("done printing time history\n");
}

module_init(hello_init);
module_exit(hello_exit);
