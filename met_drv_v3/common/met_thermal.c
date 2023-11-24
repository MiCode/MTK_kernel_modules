//SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/thermal.h>
#include <linux/of.h>

#include "met_drv.h"
#include "core_plf_init.h"
#include "core_plf_trace.h"
#include "mtk_typedefs.h"

#define TZ_NUM_MAX 50
static struct thermal_zone_device *dts_tz_list[TZ_NUM_MAX], *ext_tz_list[TZ_NUM_MAX];
static unsigned int dts_tz_num, ext_tz_num;
static bool dts_tz_updated;
struct delayed_work dwork;

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
        if (tz->polling_delay) {
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
        if (tz->polling_delay) {
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

static const char dts_tz_header[] = "met-info [000] 0.0: dts_tz:";
static const char ext_tz_header[] = "met-info [000] 0.0: ext_tz:";
static int thermal_print_header(char *buf, int len)
{
    int i = 0;
    int ret = 0;
    unsigned int buf_len = PAGE_SIZE;

    if (dts_tz_num) {
        ret += SNPRINTF(buf + ret, buf_len - ret, "%s ", dts_tz_header);
        for(i=0; i<(dts_tz_num-1); i++) {
            if (dts_tz_list[i]) {
                ret += SNPRINTF(buf + ret, buf_len - ret, "%s, ", dts_tz_list[i]->type);
            }
        }
        if (dts_tz_list[dts_tz_num-1]) {
            ret += SNPRINTF(buf + ret, buf_len - ret, "%s\n", dts_tz_list[dts_tz_num-1]->type);
        }
    }

    if (ext_tz_num) {
        ret += SNPRINTF(buf + ret, buf_len - ret, "%s ", ext_tz_header);
        for(i=0; i<(ext_tz_num-1); i++) {
            if (ext_tz_list[i]) {
                ret += SNPRINTF(buf + ret, buf_len - ret, "%s, ", ext_tz_list[i]->type);
            }
        }
        if (ext_tz_list[ext_tz_num-1]) {
            ret += SNPRINTF(buf + ret, buf_len - ret, "%s\n", ext_tz_list[ext_tz_num-1]->type);
        }

        clear_ext_tz();
    }

    return ret;
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
