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
#include "mtk_platform_diagnosis_mode.h"


static u64 g_diagnosis_mode = 0;
static u64 g_diagnosis_dump_mask = 0x0000000000000000;
static int g_enable_diagnosis_node = 0;

u64 mtk_diagnosis_mode_get_mode() {
	return g_diagnosis_mode;
}

u64 mtk_diagnosis_mode_get_dump_mask() {
	return g_diagnosis_dump_mask;
}

#if IS_ENABLED(CONFIG_MALI_MTK_DIAGNOSIS_MODE)
static ssize_t diagnosis_mode_show(struct device * const dev,
                struct device_attribute * const attr, char * const buf)
{
	struct kbase_device *const kbdev = dev_get_drvdata(dev);
	int err;
	if (!kbdev)
		return -ENODEV;

	err = scnprintf(buf, PAGE_SIZE, "%llu\n", g_diagnosis_mode);

	return err;
}

static ssize_t diagnosis_mode_store(struct device * const dev,
                struct device_attribute * const attr, const char * const buf,
                size_t const count)
{
	struct kbase_device *const kbdev = dev_get_drvdata(dev);
	if (!kbdev)
		return -ENODEV;

	if (count == 2) {
		if (kstrtoull(buf, 10, &g_diagnosis_mode) == 0) {
			dev_info(kbdev->dev, "diagnosis_mode set to %llu\n\r", g_diagnosis_mode);
		}
	}

	return count;
}

static DEVICE_ATTR_RW(diagnosis_mode);

static ssize_t diagnosis_dump_mask_show(struct device * const dev,
                struct device_attribute * const attr, char * const buf)
{
	struct kbase_device *const kbdev = dev_get_drvdata(dev);
	int err;
	if (!kbdev)
		return -ENODEV;

	err = scnprintf(buf, PAGE_SIZE, "0x%016llx\n", g_diagnosis_dump_mask);

	return err;
}

static ssize_t diagnosis_dump_mask_store(struct device * const dev,
                struct device_attribute * const attr, const char * const buf,
                size_t const count)
{
	struct kbase_device *const kbdev = dev_get_drvdata(dev);
	if (!kbdev)
		return -ENODEV;

	if (count == 19) {
		if (kstrtoull(buf, 0, &g_diagnosis_dump_mask) == 0) {
			dev_info(kbdev->dev, "diagnosis_dump_mask set to 0x%016llx\n\r", g_diagnosis_dump_mask);
		}
	}

	return count;
}

static DEVICE_ATTR_RW(diagnosis_dump_mask);

int mtk_diagnosis_mode_sysfs_init(struct kbase_device *kbdev) {
	struct device_node *node;
	const char *diagnosis_enable;
	int err;
	if (IS_ERR_OR_NULL(kbdev))
		return -1;

#if IS_ENABLED(CONFIG_MTK_GPU_DIAGNOSIS_DEBUG)
	g_enable_diagnosis_node = 1;
#else
	node = of_find_node_by_path("/soc/mali@13000000");
	if (node) {
		if(of_property_read_string(node, "gpu_diagnosis", &diagnosis_enable) == 0) {
			if (strnstr(diagnosis_enable, "on",2)) {
				g_enable_diagnosis_node = 1;
			}
		}
		of_node_put(node);
	}
#endif /*CONFIG_MTK_GPU_DIAGNOSIS_DEBUG*/

	if (g_enable_diagnosis_node == 1){
		err = sysfs_create_file(&kbdev->dev->kobj,&dev_attr_diagnosis_mode.attr);
		if (err) {
			dev_info(kbdev->dev, "SysFS file diagnosis_mode creation failed\n");
			return -1;
		}

		err = sysfs_create_file(&kbdev->dev->kobj,&dev_attr_diagnosis_dump_mask.attr);
		if (err) {
			dev_info(kbdev->dev, "SysFS file diagnosis_dump_mask creation failed\n");
			return -1;
	        }

	}

	return 0;
}
int mtk_diagnosis_mode_sysfs_term(struct kbase_device *kbdev) {
	if (IS_ERR_OR_NULL(kbdev))
		return -1;

	if (g_enable_diagnosis_node == 1){
		sysfs_remove_file(&kbdev->dev->kobj, &dev_attr_diagnosis_dump_mask.attr);
		sysfs_remove_file(&kbdev->dev->kobj, &dev_attr_diagnosis_mode.attr);
	}

	return 0;
}
#else
int mtk_diagnosis_mode_sysfs_init(struct kbase_device *kbdev) {
	return 0;
}
int mtk_diagnosis_mode_sysfs_term(struct kbase_device *kbdev) {
	return 0;
}

#endif /* CONFIG_MALI_MTK_DIAGNOSIS_MODE */
