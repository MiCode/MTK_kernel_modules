
/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 - 2023 Xiaomi Inc.
 */
#include <conn_power_throttling.h>
#include <linux/cdev.h>
#include <linux/dcache.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "[" KBUILD_MODNAME "]" fmt
/* Definition
******************************************************************************/
/* GPS driver num */
#define DRV_TYPE_GPS 2
/* GPS lowbattery level */
enum conn_pwr_low_battery_level battery_level;
#define PERMISSION_MSK (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH) /*664*/
/******************************************************************************/

static int major;
static struct class *cls;
struct device *mydev;
static char sysfs_buff[10] = "0";

// cat命令时,将会调用该函数
static ssize_t show_sys_device(struct device *dev,
                               struct device_attribute *attr,
                               char *buf) {
  pr_info("mi_gps_pwr show\n");
  return sprintf(buf, "%s\n", sysfs_buff);
}

// echo命令时,将会调用该函数
static ssize_t store_sys_device(struct device *dev,
                                struct device_attribute *attr, const char *buf,
                                size_t count) {
  pr_info("mi_gps_pwr store\n");
  if (1 == simple_strtol(buf, NULL, 10)) {
    battery_level = CONN_PWR_THR_LV_4;
  } else {
    battery_level = CONN_PWR_THR_LV_0;
  }
  // battery_level = *(enum conn_pwr_low_battery_level *)buf;
  pr_info("battery_level:%d", battery_level);
  if (conn_pwr_set_customer_level(CONN_PWR_DRV_GPS, battery_level) < 0) {
    pr_info("mi_gps_pwr set failed\n");
  }
  sprintf(sysfs_buff, "%s", buf);
  return count;
}

//定义sys_device_file的设备属性文件
static DEVICE_ATTR(sys_device_file, PERMISSION_MSK, show_sys_device,
                   store_sys_device);
static struct attribute *sys_device_attributes[] = {
    &dev_attr_sys_device_file.attr,
    NULL ////属性结构体数组最后一项必须以NULL结尾。
};

static const struct attribute_group sys_device_attr_group = {
    .attrs = sys_device_attributes,
};

struct file_operations file_ops = {
    .owner = THIS_MODULE,
};

static int gps_pwr_init(void)
{
  major = register_chrdev(0, "mi_gps_pwr", &file_ops);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6,4,0))
  cls = class_create("mi_gps_pwr");
#else
  cls = class_create(THIS_MODULE, "mi_gps_pwr");
#endif
  mydev = device_create(cls, 0, MKDEV(major, 0), NULL,
                        "mi_gps_pwr"); //创建mi_gps_pwr设备
  if (sysfs_create_group(&(mydev->kobj), &sys_device_attr_group)) {
    //在mi_gps_pwr设备目录下创建sys_device_file属性文件
    pr_info("mi_gps_pwr failed\n");
    return -1;
  }
  pr_info("mi_gps_pwr init done!\n");
  return 0;
}

static void gps_pwr_exit(void)
{
  sysfs_remove_group(&(mydev->kobj), &sys_device_attr_group);
  device_destroy(cls, MKDEV(major, 0));
  class_destroy(cls);
  unregister_chrdev(major, "mi_gps_pwr");
}

module_init(gps_pwr_init);
module_exit(gps_pwr_exit);
/*****************************************************************************/
MODULE_AUTHOR("chenyucheng@xiaomi.com");
MODULE_DESCRIPTION("MI_GPS_PWR dev");
MODULE_LICENSE("GPL");