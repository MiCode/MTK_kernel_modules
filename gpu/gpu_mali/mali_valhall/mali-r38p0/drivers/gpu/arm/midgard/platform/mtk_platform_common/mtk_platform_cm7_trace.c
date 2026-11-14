// SPDX-License-Identifier: GPL-2.0
/*
* Copyright (c) 2022 MediaTek Inc.
*/

#include <linux/of.h>
#include <linux/seq_file.h>
#include <linux/sysfs.h>
#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <platform/mtk_platform_common.h>
#include <bus_tracer_v1.h>
#include "mtk_platform_cm7_trace.h"

static int g_etm_enable_diagnosis_node = 0;
static u32 g_etm = 0;

#if IS_ENABLED(CONFIG_MALI_MTK_CM7_TRACE)
static ssize_t etm_show(struct device * const dev,
                struct device_attribute * const attr, char * const buf)
{
	struct kbase_device *const kbdev = dev_get_drvdata(dev);
	int err;
	if (IS_ERR_OR_NULL(kbdev))
		return -ENODEV;

	err = scnprintf(buf, PAGE_SIZE, "%llu\n", g_etm);

	return err;
}

static ssize_t etm_store(struct device * const dev,
                struct device_attribute * const attr, const char * const buf,
                size_t const count)
{
	struct kbase_device *const kbdev = dev_get_drvdata(dev);
	if (IS_ERR_OR_NULL(kbdev))
		return -ENODEV;

	if (kstrtouint(buf, 10, &g_etm) == 0) {
		dev_info(kbdev->dev, "[ETB] etm set to %d\n\r", g_etm);
	}
#if IS_ENABLED(CONFIG_MTK_GPU_DIAGNOSIS_DEBUG)
	if (g_etm)
		enable_etb_for_gpu_mcu();
	else
		disable_etb_capture();
#endif /*CONFIG_MTK_GPU_DIAGNOSIS_DEBUG*/
	return count;
}

static DEVICE_ATTR_RW(etm);

int mtk_cm7_trace_sysfs_init(struct kbase_device *kbdev) {
	struct device_node *node;
	const char *diagnosis_enable;
	int err;
	if (IS_ERR_OR_NULL(kbdev))
		return -1;

#if IS_ENABLED(CONFIG_MTK_GPU_DIAGNOSIS_DEBUG)
	g_etm_enable_diagnosis_node = 1;
#else
	node = of_find_node_by_path("/soc/mali@13000000");
	if (node) {
		if(of_property_read_string(node, "gpu_diagnosis", &diagnosis_enable) == 0) {
			if (strnstr(diagnosis_enable, "on",2)) {
				g_etm_enable_diagnosis_node = 1;
			}
		}
		of_node_put(node);
	}
#endif /*CONFIG_MTK_GPU_DIAGNOSIS_DEBUG*/

	if (g_etm_enable_diagnosis_node == 1){
		err = sysfs_create_file(&kbdev->dev->kobj,&dev_attr_etm.attr);
		if (err) {
			dev_info(kbdev->dev, "SysFS file etm creation failed\n");
			return -1;
		}
	}

	return 0;
}
int mtk_cm7_trace_sysfs_term(struct kbase_device *kbdev) {
	if (IS_ERR_OR_NULL(kbdev))
		return -1;

	if (g_etm_enable_diagnosis_node == 1){
		sysfs_remove_file(&kbdev->dev->kobj, &dev_attr_etm.attr);
	}

	return 0;
}
#else
int mtk_cm7_trace_sysfs_init(struct kbase_device *kbdev) {
	return 0;
}
int mtk_cm7_trace_sysfs_term(struct kbase_device *kbdev) {
	return 0;
}

#endif /* CONFIG_MALI_MTK_CM7_TRACE */
