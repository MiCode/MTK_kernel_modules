// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
/*****************************************************************************
 * headers
 *****************************************************************************/
#include <linux/device.h> /* DEVICE_ATTR */

#include "interface.h"

#include "tinysys_mgr.h"
#include "tinysys_log.h"

#if FEATURE_SSPM_NUM
#include "sspm/sspm_met_ipi_handle.h"
#include "sspm/sspm_met_log.h"
#endif

#if FEATURE_MCUPM_NUM
#include "mcupm/mcupm_met_ipi_handle.h"
#include "mcupm/mcupm_met_log.h"
#endif


/*****************************************************************************
 * define declaration
 *****************************************************************************/


/*****************************************************************************
 * struct & enum declaration
 *****************************************************************************/


/*****************************************************************************
 * external function declaration
 *****************************************************************************/


/*****************************************************************************
 * internal function declaration
 *****************************************************************************/
#if FEATURE_SSPM_NUM
static ssize_t sspm_ipi_supported_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t sspm_buffer_size_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t sspm_available_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t sspm_log_discard_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t sspm_log_mode_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t sspm_log_mode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t sspm_log_size_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t sspm_log_size_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t sspm_run_mode_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t sspm_run_mode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t sspm_modules_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t sspm_modules_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t sspm_op_ctrl_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);

static int _create_sspm_node(struct device *dev);
static void _remove_sspm_node(struct device *dev);
#endif

#if FEATURE_MCUPM_NUM
static ssize_t _mcupm_ipi_supported_show(
	struct kobject *kobj,
	struct kobj_attribute *attr,
	char *buf);

static ssize_t _mcupm_buffer_size_show(
	struct kobject *kobj,
	struct kobj_attribute *attr,
	char *buf);

static ssize_t _mcupm_available_show(
	struct kobject *kobj,
	struct kobj_attribute *attr,
	char *buf);

static ssize_t _mcupm_log_discard_show(
	struct kobject *kobj,
	struct kobj_attribute *attr,
	char *buf);

static ssize_t _mcupm_log_size_show(
	struct kobject *kobj,
	struct kobj_attribute *attr,
	char *buf);
static ssize_t _mcupm_log_size_store(
	struct kobject *kobj,
	struct kobj_attribute *attr,
	const char *buf,
	size_t count);

static ssize_t _mcupm_run_mode_show(
	struct kobject *kobj,
	struct kobj_attribute *attr,
	char *buf);
static ssize_t _mcupm_run_mode_store(
	struct kobject *kobj,
	struct kobj_attribute *attr,
	const char *buf,
	size_t count);

static ssize_t _mcupm_modules_show(
	struct kobject *kobj,
	struct kobj_attribute *attr,
	char *buf);
static ssize_t _mcupm_modules_store(
	struct kobject *kobj,
	struct kobj_attribute *attr,
	const char *buf,
	size_t count);

static ssize_t _mcupm_op_ctrl_store(
	struct kobject *kobj,
	struct kobj_attribute *attr,
	const char *buf,
	size_t count);

static int _create_mcupm_node(struct kobject *kobj);
static void _remove_mcupm_node(void);
#endif


/*****************************************************************************
 * external variable declaration
 *****************************************************************************/
unsigned int ondiemet_module[ONDIEMET_NUM];
EXPORT_SYMBOL(ondiemet_module);


/*****************************************************************************
 * internal variable declaration
 *****************************************************************************/
static struct kobject *_g_tinysys_kobj;

#if FEATURE_SSPM_NUM
static int _sspm_log_mode;
static int _sspm_run_mode;
static int _sspm_log_size;
static int _sspm_log_discard;

static DEVICE_ATTR(sspm_ipi_supported, 0444, sspm_ipi_supported_show, NULL);
static DEVICE_ATTR(sspm_buffer_size, 0444, sspm_buffer_size_show, NULL);
static DEVICE_ATTR(sspm_available, 0444, sspm_available_show, NULL);
static DEVICE_ATTR(sspm_log_discard, 0444, sspm_log_discard_show, NULL);
static DEVICE_ATTR(sspm_log_mode, 0664, sspm_log_mode_show, sspm_log_mode_store);
static DEVICE_ATTR(sspm_log_size, 0664, sspm_log_size_show, sspm_log_size_store);
static DEVICE_ATTR(sspm_run_mode, 0664, sspm_run_mode_show, sspm_run_mode_store);
static DEVICE_ATTR(sspm_modules, 0664, sspm_modules_show, sspm_modules_store);
static DEVICE_ATTR(sspm_op_ctrl, 0220, NULL, sspm_op_ctrl_store);
#endif

#if FEATURE_MCUPM_NUM
static int _mcupm_run_mode;
static int _mcupm_log_size;
static int _mcupm_log_discard;
static struct kobject *_mcupm_kobj;
static struct kobj_attribute _attr_mcupm_ipi_supported = \
	__ATTR(ipi_supported, 0444, _mcupm_ipi_supported_show, NULL);
static struct kobj_attribute _attr_mcupm_buffer_size = \
	__ATTR(buffer_size, 0444, _mcupm_buffer_size_show, NULL);
static struct kobj_attribute _attr_mcupm_available = \
	__ATTR(available, 0444, _mcupm_available_show, NULL);
static struct kobj_attribute _attr_mcupm_log_discard = \
	__ATTR(log_discard, 0444, _mcupm_log_discard_show, NULL);
static struct kobj_attribute _attr_mcupm_log_size = \
	__ATTR(log_size, 0664, _mcupm_log_size_show, _mcupm_log_size_store);
static struct kobj_attribute _attr_mcupm_run_mode = \
	__ATTR(run_mode, 0664, _mcupm_run_mode_show, _mcupm_run_mode_store);
static struct kobj_attribute _attr_mcupm_modules = \
	__ATTR(modules, 0664, _mcupm_modules_show, _mcupm_modules_store);
static struct kobj_attribute _attr_mcupm_op_ctrl = \
	__ATTR(op_ctrl, 0220, NULL, _mcupm_op_ctrl_store);
#endif


/*****************************************************************************
 * external function ipmlement
 *****************************************************************************/
int ondiemet_attr_init(struct device *dev)
{
	int ret = 0;

	PR_BOOTMSG("%s\n", __FUNCTION__);
	_g_tinysys_kobj = kobject_create_and_add("tinysys", &dev->kobj);
	if (_g_tinysys_kobj == NULL) {
		pr_debug("can not create kobject: tinysys\n");
		return -1;
	}

#if FEATURE_SSPM_NUM
	ret = _create_sspm_node(dev);
	if (ret != 0) {
		pr_debug("can not create sspm node\n");
		return ret;
	}
#endif

#if FEATURE_MCUPM_NUM
	ret = _create_mcupm_node(_g_tinysys_kobj);
	if (ret != 0) {
		pr_debug("can not create mcupm node\n");
		return ret;
	}
#endif

	return ret;
}


int ondiemet_attr_uninit(struct device *dev)
{
#if FEATURE_SSPM_NUM
	_remove_sspm_node(dev);
#endif

#if FEATURE_MCUPM_NUM
	_remove_mcupm_node();
#endif

	if (_g_tinysys_kobj != NULL) {
		kobject_del(_g_tinysys_kobj);
		kobject_put(_g_tinysys_kobj);
		_g_tinysys_kobj = NULL;
	}

	return 0;
}


int ondiemet_log_manager_init(struct device *dev)
{
	return tinysys_log_manager_init(dev);
}


int ondiemet_log_manager_uninit(struct device *dev)
{
	return tinysys_log_manager_uninit(dev);
}


void ondiemet_log_manager_start()
{
	tinysys_log_manager_start();
}


void ondiemet_log_manager_stop()
{
	tinysys_log_manager_stop();
}


void ondiemet_start()
{
#if FEATURE_SSPM_NUM
	sspm_start();
#endif

#if FEATURE_MCUPM_NUM
	mcupm_start();
#endif

}


void ondiemet_stop()
{
#if FEATURE_SSPM_NUM
	sspm_stop();
#endif

#if FEATURE_MCUPM_NUM
	mcupm_stop();
#endif

}


void ondiemet_extract()
{
#if FEATURE_SSPM_NUM
	sspm_extract();
#endif

#if FEATURE_MCUPM_NUM
	mcupm_extract();
#endif

}


/*****************************************************************************
 * internal function ipmlement
 *****************************************************************************/
#if FEATURE_SSPM_NUM
static ssize_t sspm_ipi_supported_show(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	int ipi_supported = 1;
	int i = 0;

	i = snprintf(buf, PAGE_SIZE, "%d\n", ipi_supported);
	if (i < 0)
		return 0;

	return i;
}


static ssize_t sspm_buffer_size_show(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	int i = 0;

	i = snprintf(buf, PAGE_SIZE, "%d\n", sspm_buffer_size);
	if (i < 0)
		return 0;

	return i;
}


static ssize_t sspm_available_show(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	int i = 0;

	i = snprintf(buf, PAGE_SIZE, "%d\n", 1);
	if (i < 0)
		return 0;

	return i;
}


static ssize_t sspm_log_discard_show(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	int i = 0;

	i = snprintf(buf, PAGE_SIZE, "%d\n", _sspm_log_discard);
	if (i < 0)
		return 0;

	return i;
}

static ssize_t sspm_log_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i;

	mutex_lock(&dev->mutex);
	i = snprintf(buf, PAGE_SIZE, "%d\n", _sspm_log_mode);
	mutex_unlock(&dev->mutex);

	if (i < 0)
		return 0;

	return i;
}


static ssize_t sspm_log_mode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int value;

	if (kstrtoint(buf, 0, &value) != 0)
		return -EINVAL;
	mutex_lock(&dev->mutex);
	_sspm_log_mode = value;
	mutex_unlock(&dev->mutex);

	return count;
}


static ssize_t sspm_log_size_show(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	int i = 0;

	i = snprintf(buf, PAGE_SIZE, "%d\n", _sspm_log_size);
	if (i < 0)
		return 0;

	return i;
}


static ssize_t sspm_log_size_store(
	struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count)
{
	int value = 0;

	if (kstrtoint(buf, 0, &value) != 0) {
		return -EINVAL;
	}

	_sspm_log_size = value;

	return count;
}


static ssize_t sspm_run_mode_show(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	int i = 0;

	i = snprintf(buf, PAGE_SIZE, "%d\n", _sspm_run_mode);
	if (i < 0)
		return 0;

	return i;
}


static ssize_t sspm_run_mode_store(
	struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count)
{
	int value = 0;

	if (kstrtoint(buf, 0, &value) != 0) {
		return -EINVAL;
	}

	_sspm_run_mode = value;

	return count;
}


static ssize_t sspm_op_ctrl_store(
	struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count)
{
	int value = 0;

	if (kstrtoint(buf, 0, &value) != 0) {
		return -EINVAL;
	}

	if (value == 1) {
		sspm_start();
	} else if (value == 2) {
		sspm_stop();
	} else if (value == 3) {
		sspm_extract();
	} else if (value == 4) {
		sspm_flush();
	}

	return count;
}


static ssize_t sspm_modules_show(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	int i = 0;

	i = snprintf(buf, PAGE_SIZE, "0x%X\n", ondiemet_module[ONDIEMET_SSPM]);
	if (i < 0)
		return 0;

	return i;
}

static ssize_t sspm_modules_store(
	struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count)
{
	unsigned int value;

	if (kstrtoint(buf, 0, &value) != 0) {
		return -EINVAL;
	}

	ondiemet_module[ONDIEMET_SSPM] = value;

	return count;
}


static int _create_sspm_node(struct device *dev)
{
	int ret = 0;

	ret = device_create_file(dev, &dev_attr_sspm_ipi_supported);
	if (ret != 0) {
		pr_debug("can not create device file: sspm_ipi_supported\n");
		return ret;
	}

	ret = device_create_file(dev, &dev_attr_sspm_buffer_size);
	if (ret != 0) {
		pr_debug("can not create device file: sspm_buffer_size\n");
		return ret;
	}

	ret = device_create_file(dev, &dev_attr_sspm_available);
	if (ret != 0) {
		pr_debug("can not create device file: sspm_available\n");
		return ret;
	}

	ret = device_create_file(dev, &dev_attr_sspm_log_discard);
	if (ret != 0) {
		pr_debug("can not create device file: sspm_log_discard\n");
		return ret;
	}

	ret = device_create_file(dev, &dev_attr_sspm_log_mode);
	if (ret != 0) {
		pr_debug("can not create device file: sspm_log_mode\n");
		return ret;
	}

	ret = device_create_file(dev, &dev_attr_sspm_log_size);
	if (ret != 0) {
		pr_debug("can not create device file: sspm_log_size\n");
		return ret;
	}

	ret = device_create_file(dev, &dev_attr_sspm_run_mode);
	if (ret != 0) {
		pr_debug("can not create device file: sspm_run_mode\n");
		return ret;
	}

	ret = device_create_file(dev, &dev_attr_sspm_modules);
	if (ret != 0) {
		pr_debug("can not create device file: sspm_modules\n");
		return ret;
	}

	ret = device_create_file(dev, &dev_attr_sspm_op_ctrl);
	if (ret != 0) {
		pr_debug("can not create device file: sspm_op_ctrl\n");
		return ret;
	}
	return ret;
}


static void _remove_sspm_node(struct device *dev)
{
	device_remove_file(dev, &dev_attr_sspm_ipi_supported);
	device_remove_file(dev, &dev_attr_sspm_buffer_size);
	device_remove_file(dev, &dev_attr_sspm_available);
	device_remove_file(dev, &dev_attr_sspm_log_discard);
	device_remove_file(dev, &dev_attr_sspm_log_mode);
	device_remove_file(dev, &dev_attr_sspm_log_size);
	device_remove_file(dev, &dev_attr_sspm_run_mode);
	device_remove_file(dev, &dev_attr_sspm_modules);
	device_remove_file(dev, &dev_attr_sspm_op_ctrl);
}
#endif


#if FEATURE_MCUPM_NUM
static ssize_t _mcupm_ipi_supported_show(
	struct kobject *kobj,
	struct kobj_attribute *attr,
	char *buf)
{
	int ipi_supported = 1;
	int i = 0;

	i = snprintf(buf, PAGE_SIZE, "%d\n", ipi_supported);
	if (i < 0)
		return 0;

	return i;
}


static ssize_t _mcupm_buffer_size_show(
	struct kobject *kobj,
	struct kobj_attribute *attr,
	char *buf)
{
	int i = 0;

	i = snprintf(buf, PAGE_SIZE, "%d\n", mcupm_buffer_size);
	if (i < 0)
		return 0;

	return i;
}


static ssize_t _mcupm_available_show(
	struct kobject *kobj,
	struct kobj_attribute *attr,
	char *buf)
{
	int i = 0;

	i = snprintf(buf, PAGE_SIZE, "%d\n", 1);
	if (i < 0)
		return 0;

	return i;
}


static ssize_t _mcupm_log_discard_show(
	struct kobject *kobj,
	struct kobj_attribute *attr,
	char *buf)
{
	int i = 0;

	i = snprintf(buf, PAGE_SIZE, "%d\n", _mcupm_log_discard);
	if (i < 0)
		return 0;

	return i;
}


static ssize_t _mcupm_log_size_show(
	struct kobject *kobj,
	struct kobj_attribute *attr,
	char *buf)
{
	int i = 0;

	i = snprintf(buf, PAGE_SIZE, "%d\n", _mcupm_log_size);
	if (i < 0)
		return 0;

	return i;
}


static ssize_t _mcupm_log_size_store(
	struct kobject *kobj,
	struct kobj_attribute *attr,
	const char *buf,
	size_t count)
{
	int value = 0;

	if (kstrtoint(buf, 0, &value) != 0) {
		return -EINVAL;
	}

	_mcupm_log_size = value;

	return count;
}


static ssize_t _mcupm_run_mode_show(
	struct kobject *kobj,
	struct kobj_attribute *attr,
	char *buf)
{
	int i = 0;

	i = snprintf(buf, PAGE_SIZE, "%d\n", _mcupm_run_mode);
	if (i < 0)
		return 0;

	return i;
}


static ssize_t _mcupm_run_mode_store(
	struct kobject *kobj,
	struct kobj_attribute *attr,
	const char *buf,
	size_t count)
{
	int value = 0;

	if (kstrtoint(buf, 0, &value) != 0) {
		return -EINVAL;
	}

	_mcupm_run_mode = value;

	return count;
}


static ssize_t _mcupm_op_ctrl_store(
	struct kobject *kobj,
	struct kobj_attribute *attr,
	const char *buf,
	size_t count)
{
	int value = 0;

	if (kstrtoint(buf, 0, &value) != 0) {
		return -EINVAL;
	}

	if (value == 1) {
		mcupm_start();
	} else if (value == 2) {
		mcupm_stop();
	} else if (value == 3) {
		mcupm_extract();
	} else if (value == 4) {
		mcupm_flush();
	}

	return count;
}


static ssize_t _mcupm_modules_show(
	struct kobject *kobj,
	struct kobj_attribute *attr,
	char *buf)
{
	int i = 0;

	i = snprintf(buf, PAGE_SIZE, "0x%X\n", ondiemet_module[ONDIEMET_MCUPM]);
	if (i < 0)
		return 0;

	return i;
}

static ssize_t _mcupm_modules_store(
	struct kobject *kobj,
	struct kobj_attribute *attr,
	const char *buf,
	size_t count)
{
	unsigned int value;

	if (kstrtoint(buf, 0, &value) != 0) {
		return -EINVAL;
	}

	ondiemet_module[ONDIEMET_MCUPM] = value;

	return count;
}


static int _create_mcupm_node(struct kobject *parent)
{
	int ret = 0;

	_mcupm_kobj = kobject_create_and_add("mcupm", parent);
	if (_mcupm_kobj == NULL) {
		return -1;
	}

	ret = sysfs_create_file(_mcupm_kobj, &_attr_mcupm_available.attr);
	if (ret != 0) {
		pr_debug("can not create device file: available\n");
		return ret;
	}

	ret = sysfs_create_file(_mcupm_kobj, &_attr_mcupm_buffer_size.attr);
	if (ret != 0) {
		pr_debug("can not create device file: buffer_size\n");
		return ret;
	}

	ret = sysfs_create_file(_mcupm_kobj, &_attr_mcupm_log_discard.attr);
	if (ret != 0) {
		pr_debug("can not create device file: log_discard\n");
		return ret;
	}

	ret = sysfs_create_file(_mcupm_kobj, &_attr_mcupm_log_size.attr);
	if (ret != 0) {
		pr_debug("can not create device file: log_size\n");
		return ret;
	}

	ret = sysfs_create_file(_mcupm_kobj, &_attr_mcupm_run_mode.attr);
	if (ret != 0) {
		pr_debug("can not create device file: run_mode\n");
		return ret;
	}

	ret = sysfs_create_file(_mcupm_kobj, &_attr_mcupm_op_ctrl.attr);
	if (ret != 0) {
		pr_debug("can not create device file: op_ctrl\n");
		return ret;
	}

	ret = sysfs_create_file(_mcupm_kobj, &_attr_mcupm_modules.attr);
	if (ret != 0) {
		pr_debug("can not create device file: modules\n");
		return ret;
	}

	ret = sysfs_create_file(_mcupm_kobj, &_attr_mcupm_ipi_supported.attr);
	if (ret != 0) {
		pr_debug("can not create device file: ipi_supported\n");
		return ret;
	}
	return ret;
}


static void _remove_mcupm_node()
{
	if (_mcupm_kobj != NULL) {
		sysfs_remove_file(_mcupm_kobj, &_attr_mcupm_buffer_size.attr);
		sysfs_remove_file(_mcupm_kobj, &_attr_mcupm_available.attr);
		sysfs_remove_file(_mcupm_kobj, &_attr_mcupm_log_discard.attr);
		sysfs_remove_file(_mcupm_kobj, &_attr_mcupm_log_size.attr);
		sysfs_remove_file(_mcupm_kobj, &_attr_mcupm_run_mode.attr);
		sysfs_remove_file(_mcupm_kobj, &_attr_mcupm_op_ctrl.attr);
		sysfs_remove_file(_mcupm_kobj, &_attr_mcupm_modules.attr);
		sysfs_remove_file(_mcupm_kobj, &_attr_mcupm_ipi_supported.attr);

		kobject_del(_mcupm_kobj);
		kobject_put(_mcupm_kobj);
		_mcupm_kobj = NULL;
	}
}
#endif
