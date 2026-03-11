// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <leds-mtk.h>

#define MET_USER_EVENT_SUPPORT
#include "met_drv.h"
#include "trace.h"
#include "core_plf_init.h"
#include "mtk_typedefs.h"

#include <mtk_printk_ctrl.h>

static int met_backlight_enable;
static DEFINE_SPINLOCK(met_backlight_lock);
static struct kobject *kobj_met_backlight;

static ssize_t bl_tag_enable_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static ssize_t bl_tag_enable_store(struct kobject *kobj,
				   struct kobj_attribute *attr, const char *buf, size_t n);
static struct kobj_attribute bl_tag_enable_attr =
__ATTR(backlight_tag_enable, 0664, bl_tag_enable_show, bl_tag_enable_store);

static int met_backlight_trigger_size;
static ssize_t bl_trigger_size_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static ssize_t bl_trigger_size_store(struct kobject *kobj,
				   struct kobj_attribute *attr, const char *buf, size_t n);

static struct kobj_attribute bl_trigger_size_attr =
__ATTR(trigger_size, 0664, bl_trigger_size_show, bl_trigger_size_store);


#define TRIGGER_SIZE_MAX 254
/*reserved 2 char for specail use and end char*/
char _trigger_DAQ_[TRIGGER_SIZE_MAX+2] = {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
					0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
					0x4D, 0x0}; 

void reset_trigger_DAQ(int size)
{
	int i = 0;
	if ((size >= 0) && (size < TRIGGER_SIZE_MAX))
	{
		for(i=0;i<size;i++) 
		{
			_trigger_DAQ_[i] = 0x01;
		}
		_trigger_DAQ_[i++] = 0x4E;
		_trigger_DAQ_[i++] = '\0';

		met_backlight_trigger_size = size;
	}
}

/* old method to trigger DAQ: use the backlight pwm to control the special pin to casue voltage 5V ->1.8V */
/* but the EVB HW sometimes not support , so we use UART to instead it */
/*
#if IS_ENABLED(CONFIG_LEDS_MTK)
#if IS_ENABLED(CONFIG_LEDS_MTK_DISP) || IS_ENABLED(CONFIG_LEDS_MTK_PWM) || IS_ENABLED(CONFIG_LEDS_MTK_I2C)

static int led_brightness_changed_event(struct notifier_block *nb,
					unsigned long event, void *v)
{
	struct led_conf_info *led_conf;

	led_conf = (struct led_conf_info *)v;

	switch (event) {
	case LED_BRIGHTNESS_CHANGED:
		output_met_backlight_tag_real(led_conf->cdev.brightness);
		break;
	default:
		break;
	}

	return NOTIFY_DONE;
}

static struct notifier_block leds_change_notifier = {
	.notifier_call = led_brightness_changed_event,
};
#endif
#endif
*/
int enable_met_backlight_tag_real(void)
{
	return met_backlight_enable;
}

int output_met_backlight_tag_real(int level)
{
	int ret;
	unsigned long flags;

	spin_lock_irqsave(&met_backlight_lock, flags);
#ifdef CONFIG_MET_MODULE
	ret = met_tag_oneshot_real(33880, "_MM_BL_", level);
#else
	ret = met_tag_oneshot(33880, "_MM_BL_", level);
#endif
	spin_unlock_irqrestore(&met_backlight_lock, flags);

	return ret;
}

static ssize_t bl_tag_enable_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	int ret;

	ret = SNPRINTF(buf, PAGE_SIZE, "%d\n", met_backlight_enable);

	return ret;
}

static ssize_t bl_tag_enable_store(struct kobject *kobj,
				   struct kobj_attribute *attr, const char *buf, size_t n)
{
	int value;
	int ret;

	if ((n == 0) || (buf == NULL))
		return -EINVAL;

	if (kstrtoint(buf, 0, &value) != 0)
		return -EINVAL;

	if (value < 0)
		return -EINVAL;

	/*use UART to trigger the DAQ start*/
#if IS_ENABLED(CONFIG_MTK_PRINTK)
	if (value == 0)
	{
		ret = met_tag_oneshot_real(33880, "_MM_BL_", 255);

		update_uartlog_status(true, 1);
		pr_info("%s\n", _trigger_DAQ_);
		pr_info("trigger patern size[%zu]\n", strlen(_trigger_DAQ_));

		ret = met_tag_oneshot_real(33880, "_MM_BL_", 0);

		update_uartlog_status(true, 0);
		PR_BOOTMSG("[backlight] UART tigger DAQ\n");
	}
#else
	PR_BOOTMSG("[backlight] UART tigger DAQ, but CONFIG_MTK_PRINTK is not set\n");
#endif
/* old method to trigger DAQ: use the backlight pwm to control the special pin to casue voltage 5V ->1.8V */
/* but the EVB HW sometimes not support , so we use UART to instead it */
/*
#if IS_ENABLED(CONFIG_LEDS_MTK)
#if IS_ENABLED(CONFIG_LEDS_MTK_DISP) || IS_ENABLED(CONFIG_LEDS_MTK_PWM) || IS_ENABLED(CONFIG_LEDS_MTK_I2C)
	if (value == 1) {
		if (mtk_leds_register_notifier_symbol)
			mtk_leds_register_notifier_symbol(&leds_change_notifier);
	} else if (value == 0) {
		if (mtk_leds_unregister_notifier_symbol)
			mtk_leds_unregister_notifier_symbol(&leds_change_notifier);
	}
#endif
#endif
*/
	met_backlight_enable = value;

	return n;
}


/*for DAQ trigger size adjust*/
static ssize_t bl_trigger_size_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	int ret;

	ret = SNPRINTF(buf, PAGE_SIZE, "%d\n", met_backlight_trigger_size);

	return ret;
}

static ssize_t bl_trigger_size_store(struct kobject *kobj,
				   struct kobj_attribute *attr, const char *buf, size_t n)
{
	int value;

	if ((n == 0) || (buf == NULL))
		return -EINVAL;

	if (kstrtoint(buf, 0, &value) != 0)
		return -EINVAL;

	if (value < 0)
		return -EINVAL;

	reset_trigger_DAQ(value);

	return n;
}


static int met_backlight_create(struct kobject *parent)
{
	int ret = 0;
	kobj_met_backlight = parent;

	ret = sysfs_create_file(kobj_met_backlight, &bl_tag_enable_attr.attr);
	if (ret != 0) {
		pr_debug("Failed to create montype0 in sysfs\n");
		return ret;
	}

	/* create for trigger pattern adjust*/
	ret = sysfs_create_file(kobj_met_backlight, &bl_trigger_size_attr.attr);
	if (ret != 0) {
		pr_debug("Failed to create trigger_size in sysfs\n");
		return ret;
	}
	reset_trigger_DAQ(16);

	return ret;
}

static void met_backlight_delete(void)
{
	sysfs_remove_file(kobj_met_backlight, &bl_trigger_size_attr.attr);
	sysfs_remove_file(kobj_met_backlight, &bl_tag_enable_attr.attr);
	kobj_met_backlight = NULL;
}

struct metdevice met_backlight = {
	.name = "backlight",
	.owner = THIS_MODULE,
	.type = MET_TYPE_BUS,
	.create_subfs = met_backlight_create,
	.delete_subfs = met_backlight_delete,
	.cpu_related = 0,
};
EXPORT_SYMBOL(met_backlight);
