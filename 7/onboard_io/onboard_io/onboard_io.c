/* SPDX-License-Identifier: GPL-2.0
 *
 * BBB On-board IO demo.
 * Just on leds at init and of at exit
 *
 */

#include "asm-generic/gpio.h"
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/debugfs.h>
#include <linux/delay.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Oleksandr Redchuk (at GL training courses)");
MODULE_DESCRIPTION("BBB Onboard IO Demo");
MODULE_VERSION("0.1");

#define GPIO_NUMBER(port, bit) (32 * (port) + (bit))

/* On-board LEDs
 *  0: D2	GPIO1_21	heartbeat
 *  1: D3	GPIO1_22	uSD access
 *  2: D4	GPIO1_23	active
 *  3: D5	GPIO1_24	MMC access
 *
 * uSD and MMC access LEDs are not used in nfs boot mode, but they are already requested
 * So that, we don't use gpio_request()/gpio_free() here.
 */

#define LED_SD  GPIO_NUMBER(1, 22)
#define LED_MMC GPIO_NUMBER(1, 24)

/* On-board button.
 *
 * HDMI interface must be disabled.
 */
#define BUTTON  GPIO_NUMBER(2, 8)

struct my_irq_data {
	int led_state;
};

struct my_irq_data my_irq_data;
struct dentry droot;

struct dentry *root_dentry;
struct dentry *counter_dentry;

static u32 counter = 0;
static int led_gpio = -1;
static int button_gpio = -1;
static int button_irq = -1;

static bool simulate_busy=false;
module_param(simulate_busy,bool,0660);

static irqreturn_t hw_button_intr(int irq, void *dev_id) {
	struct my_irq_data *data = (struct my_irq_data *)dev_id;

	counter++;

	data->led_state = !data->led_state;
	gpio_set_value(led_gpio, data->led_state);

	if (simulate_busy) {
		msleep(2000);
		pr_info("irq handled successfully\n");
	}

	return IRQ_WAKE_THREAD;
}

static irqreturn_t thread_button_intr(int irq, void *dev_id) {
	pr_info("thread_button_intr called\n");
	pr_info("counter: %d\n", counter);

	return IRQ_HANDLED;
}

static int led_gpio_init(int gpio)
{
	int ret;

	ret = gpio_direction_output(gpio, 0);
	if (ret)
		return ret;

	led_gpio = gpio;
	return 0;
}

static int button_gpio_init(int gpio)
{
	int ret;

	ret = gpio_request(gpio, "Onboard user button");
	if (ret)
		goto err_register;

	ret = gpio_direction_input(gpio);
	if (ret)
		goto err_input;

	ret = gpio_set_debounce(gpio, 200);
	if (ret) {
		pr_err("unable to set debounce time\n");
		goto err_input;
	}

	button_irq = gpio_to_irq(gpio);
	if (button_irq < 0) {
		pr_err("Unable to convert gpio to irq\n");
		goto err_input;
	}

	my_irq_data.led_state = 0;

	ret = request_threaded_irq((unsigned int)button_irq, hw_button_intr,
				   thread_button_intr,
				   IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
				   "hm7_irq", &my_irq_data);
	if (ret) {
		pr_err("Unable to request threaded irq\n");
		goto err_input;
	}

	button_gpio = gpio;

	pr_info("Init GPIO%d OK\n", button_gpio);

	return 0;

err_input:
	gpio_free(gpio);
err_register:
	return ret;
}

static void button_gpio_deinit(void)
{
	if (button_gpio >= 0) {
		gpio_free(button_gpio);
		pr_info("Deinit GPIO%d\n", button_gpio);
	}
}

static void debugfs_deinit(void)
{
	debugfs_remove_recursive(root_dentry);
}

static void debugfs_init(void)
{
	root_dentry = debugfs_create_dir(KBUILD_MODNAME, NULL);
	if (IS_ERR_OR_NULL(root_dentry)) {
		pr_err("Unable to create debugfs dir\n");
		root_dentry = NULL;
	}

	counter_dentry = debugfs_create_u32("counter", 0444, root_dentry,
					    &counter);
	if (!counter_dentry) {
		pr_err("Unable to create counter debugfs entry\n");
		debugfs_deinit();
	}

	pr_info("Debugs fs entries created successfully\n");
}

/* Module entry/exit points */
static int __init onboard_io_init(void)
{
	int ret;
	int gpio;
	int button_state;

	ret = button_gpio_init(BUTTON);
	if (ret) {
		pr_err("Can't set GPIO%d for button\n", BUTTON);
		goto err_button;
	}

	button_state = gpio_get_value(button_gpio);

	gpio = button_state ? LED_MMC : LED_SD;
	if (ret) {
		pr_err("Can't set GPIO%d for output\n", gpio);
		goto err_button;
	}

	ret = led_gpio_init(gpio);
	if (ret) {
		pr_err("Can't set GPIO%d for output\n", gpio);
		goto err_led;
	}

	gpio_set_value(led_gpio, 1);
	pr_info("LED at GPIO%d ON\n", led_gpio);

	debugfs_init();

	return 0;

err_led:
	button_gpio_deinit();
err_button:
	return ret;
}

static void __exit onboard_io_exit(void)
{
	if (led_gpio >= 0) {
		gpio_set_value(led_gpio, 0);
		pr_info("LED at GPIO%d OFF\n", led_gpio);
	}

	button_gpio_deinit();

	if (button_irq >= 0)
		free_irq(button_irq, &my_irq_data);

	debugfs_deinit();
}

module_init(onboard_io_init);
module_exit(onboard_io_exit);

