#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/ktime.h>
#include <linux/timer.h>

MODULE_LICENSE("GPL");

static struct timer_list my_timer;
static unsigned long start;

static int restart = 5;
static unsigned long delay_in_jiffies = (HZ * 200L) / MSEC_PER_SEC;

static void timer_callback(struct timer_list *timer)
{
	pr_info("timer_callback called (%lu).\n", jiffies);

	if (restart--) {
		start += delay_in_jiffies;
		mod_timer(timer, start);
	}
}

static __init int timer_init(void)
{
	unsigned long now;

	pr_info("Timer module installing\n");

	timer_setup(&my_timer, timer_callback, 0);

	now = jiffies;
	start = now + delay_in_jiffies;
	pr_info("Starting timer to fire in %lu ms (%lu)\n", start, now);

	mod_timer(&my_timer, start);


	return 0;
}

static __exit void timer_exit(void)
{
	if (del_timer(&my_timer))
		pr_info("Рendng timer deleted\n");

	pr_info("Timer module uninstalling\n");

	return;
}

module_init(timer_init);
module_exit(timer_exit);

