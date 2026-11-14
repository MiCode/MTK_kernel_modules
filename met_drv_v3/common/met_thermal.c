//SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/thermal.h>
#include <linux/of.h>
#include "thermal_core.h"

#include "met_drv.h"
#include "core_plf_init.h"
#include "core_plf_trace.h"
#include "mtk_typedefs.h"

#define TZ_NUM_MAX 128
static struct thermal_zone_device *dts_tz_list[TZ_NUM_MAX], *ext_tz_list[TZ_NUM_MAX];
static unsigned int dts_tz_num, ext_tz_num;
static bool dts_tz_updated;
struct delayed_work dwork;

static int output_header_dts_len = 0;
static int output_header_ext_len = 0;
static int dts_print_done = 0;
static int ext_print_done = 0;
static int read_idx = 0;
#define FILE_NODE_STR_LEN 256
char ext_tz_str[FILE_NODE_STR_LEN] = {'\0'};
struct kobject *kobj_thermal;

static ssize_t ext_tz_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return SNPRINTF(buf, PAGE_SIZE, "%s\n", ext_tz_str);
}

static ssize_t ext_tz_store(struct kobject *kobj,
                   struct kobj_attribute *attr, const char *buf, size_t n)
{
    int ret;

    ret = SNPRINTF(ext_tz_str, FILE_NODE_STR_LEN, "%s", buf);
    if (ret < 0) {
        return -EINVAL;
    }

    ext_tz_str[n-1]='\0';

    return n;
}

struct kobj_attribute ext_tz_attr = __ATTR(ext_tz, 0664, ext_tz_show, ext_tz_store);

static int met_thermal_create(struct kobject *parent)
{
    int ret = 0;

    kobj_thermal = parent;

    ret = sysfs_create_file(kobj_thermal, &ext_tz_attr.attr);
    if (ret != 0) {
        pr_debug("Failed to create thermal ext_tz_attr in sysfs\n");
    }

    return ret;
}

static void met_thermal_delete(void)
{
    sysfs_remove_file(kobj_thermal, &ext_tz_attr.attr);
}

static void get_thermal_zone_from_dts(void) {
    struct device_node *np = NULL, *child = NULL;
    unsigned int tz_idx = 0;

    if(dts_tz_updated) {
        return;
    }

    np = of_find_node_by_name(NULL, "thermal-zones");
    if (!np) {
        pr_debug("unable to find thermal-zones\n");
        return;
    }

    for_each_available_child_of_node(np, child) {
        struct thermal_zone_device *tz;

        tz = thermal_zone_get_zone_by_name(child->name);
        if (!tz || IS_ERR(tz)) {
            continue;
        }

        // Do not interfere original kernel polling
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
        if (tz->polling_delay_jiffies) {
#else
        if (tz->polling_delay) {
#endif
            continue;
        }

        dts_tz_list[tz_idx++] = tz;
    }

    dts_tz_num = tz_idx;
    dts_tz_updated = true;
}

static void get_thermal_zone_from_ext(void) {
    char *cur = ext_tz_str, *tz_name;
    struct thermal_zone_device *tz;
    unsigned int tz_idx = 0;
    const char *delim_comma = ",";

    while (cur != NULL) {
        tz_name = strsep(&cur, delim_comma);

        tz = thermal_zone_get_zone_by_name(tz_name);
        if (!tz || IS_ERR(tz)) {
            continue;
        }

        // Do not interfere original kernel polling
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
        if (tz->polling_delay_jiffies) {
#else
        if (tz->polling_delay) {
#endif
            continue;
        }

        ext_tz_list[tz_idx++] = tz;
    }

    ext_tz_num = tz_idx;
}

static void wq_get_thermal(struct work_struct *work)
{
    int i = 0;

    if (!dts_tz_num && !ext_tz_num) {
        return;
    }

    for(i=0; i<dts_tz_num; i++) {
        if (dts_tz_list[i]) {
            thermal_zone_device_update(dts_tz_list[i], THERMAL_EVENT_UNSPECIFIED);
        }
    }

    for(i=0; i<ext_tz_num; i++) {
        if (ext_tz_list[i]) {
            thermal_zone_device_update(ext_tz_list[i], THERMAL_EVENT_UNSPECIFIED);
        }
    }
}

static void thermal_start(void)
{
    get_thermal_zone_from_dts();
    get_thermal_zone_from_ext();

    INIT_DELAYED_WORK(&dwork, wq_get_thermal);
    output_header_dts_len = 0;
    output_header_ext_len = 0;
    dts_print_done = 0;
    ext_print_done = 0;

}

static void thermal_stop(void)
{
    cancel_delayed_work_sync(&dwork);
}

static void thermal_polling(unsigned long long stamp, int cpu)
{
    schedule_delayed_work(&dwork, 0);
}

static const char help[] = "  --thermal                             monitor thermal\n";
static int thermal_print_help(char *buf, int len)
{
    return SNPRINTF(buf, PAGE_SIZE, help);
}

static void clear_ext_tz(void) {
    int i = 0;

    for(i=0; i<ext_tz_num; i++) {
        ext_tz_list[i] = NULL;
    }
    ext_tz_num = 0;
    ext_tz_str[0] = '\0';
}

static const char dts_tz_header[] = "met-info [000] 0.0: dts_tz: ";
static const char ext_tz_header[] = "met-info [000] 0.0: ext_tz: ";
static int thermal_print_header(char *buf, int len)
{
    int i;
    int write_len;
    len = 0;
    met_thermal.header_read_again = 0;
    //print dts_tz header from dts file
    if(dts_tz_num) {
        if(!dts_print_done) {
            if (output_header_dts_len == 0) {
                len = SNPRINTF(buf, PAGE_SIZE, "%s", dts_tz_header);
                output_header_dts_len = 1; 
            }
            for (i = read_idx; i <= dts_tz_num; i++) {
                if (i == dts_tz_num) {
                    output_header_dts_len = 0;
                    read_idx = 0;
                    buf[len - 1] = '\n';
                    met_thermal.header_read_again = 0;
                    dts_print_done = 1;
                }else {
                    write_len = strlen(dts_tz_list[i]->type);
                    if ((len + write_len) < PAGE_SIZE) {
                        len += SNPRINTF(buf+len, PAGE_SIZE-len, "%s,", dts_tz_list[i]->type);
                    } else {
                        met_thermal.header_read_again = 1;
                        read_idx = i;
                        return len;
                    }
                }
            }
        }
    }
    //print ext_tz header
    if(ext_tz_num) {
        if(!ext_print_done) {
            if (output_header_ext_len == 0) {
                len += SNPRINTF(buf+len, PAGE_SIZE-len, "%s", ext_tz_header);
                output_header_ext_len = 1;
            }
            for (i = read_idx; i <= ext_tz_num; i++) {
                if (i == ext_tz_num) {
                    output_header_ext_len = 0;
                    read_idx = 0;
                    buf[len - 1] = '\n';
                    met_thermal.header_read_again = 0;
                    ext_print_done = 1;
                } else {
                    write_len = strlen(ext_tz_list[i]->type);
                    if ((len + write_len) < PAGE_SIZE) {
                        len += SNPRINTF(buf+len, PAGE_SIZE-len, "%s,", ext_tz_list[i]->type);
                    } else {
                        met_thermal.header_read_again = 1;
                        read_idx = i;
                        return len;
                    }
                }
            }
        }
    }
    clear_ext_tz();
    return len;
}

struct metdevice met_thermal = {
    .name = "thermal",
    .owner = THIS_MODULE,
    .type = MET_TYPE_BUS,
    .create_subfs = met_thermal_create,
    .delete_subfs = met_thermal_delete,
    .start = thermal_start,
    .stop = thermal_stop,
    .polling_interval = 1,    /* ms */
    .timed_polling = thermal_polling,
    .print_help = thermal_print_help,
    .print_header = thermal_print_header,
};
