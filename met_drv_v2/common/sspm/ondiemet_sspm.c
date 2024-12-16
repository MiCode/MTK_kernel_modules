// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/delay.h>
#include <linux/module.h> /* symbol_get */

#define MET_USER_EVENT_SUPPORT
#include "met_drv.h"

#if IS_ENABLED(CONFIG_MTK_TINYSYS_SSPM_SUPPORT) && defined(ONDIEMET_SUPPORT)
#include "ondiemet_sspm.h"
#include "met_kernel_symbol.h"

#if IS_ENABLED(CONFIG_MTK_GMO_RAM_OPTIMIZE) || IS_ENABLED(CONFIG_MTK_MET_MEM_ALLOC)
#ifdef CONFIG_MET_ARM_32BIT
#include <asm/dma-mapping.h> /* arm_coherent_dma_ops */
#else /* CONFIG_MET_ARM_32BIT */
#include <linux/dma-mapping.h>
#endif /* CONFIG_MET_ARM_32BIT */
#else /* CONFIG_MTK_GMO_RAM_OPTIMIZE */
#include "sspm_reservedmem.h"
/* #include "sspm_reservedmem_define.h" */
#endif /* CONFIG_MTK_GMO_RAM_OPTIMIZE */

dma_addr_t ondiemet_sspm_log_phy_addr;
void *ondiemet_sspm_log_virt_addr;

#ifdef DYNAMIC_ALLOC_ODM_BUF_SIZE
uint32_t ondiemet_sspm_log_size = DYNAMIC_ALLOC_ODM_BUF_SIZE;
#else
uint32_t ondiemet_sspm_log_size = 0x800000;
#endif


/* SSPM_LOG_FILE 0 */
/* SSPM_LOG_SRAM 1 */
/* SSPM_LOG_DRAM 2 */
int sspm_log_mode;
/* SSPM_RUN_NORMAL mode 0 */
/* SSPM_RUN_CONTINUOUS mode 1 */
int sspm_run_mode;
int met_sspm_log_discard = -1;
int sspm_log_size = 500;

int sspm_buffer_size;
int sspm_buf_available;
EXPORT_SYMBOL(sspm_buf_available);
int sspm_buf_mapped = -1; /* get buffer by MET itself */


static ssize_t sspm_ipi_supported_show(struct device *dev, struct device_attribute *attr, char *buf);
static DEVICE_ATTR(sspm_ipi_supported, 0444, sspm_ipi_supported_show, NULL);
static ssize_t sspm_buffer_size_show(struct device *dev, struct device_attribute *attr, char *buf);
static DEVICE_ATTR(sspm_buffer_size, 0444, sspm_buffer_size_show, NULL);

static ssize_t sspm_available_show(struct device *dev, struct device_attribute *attr, char *buf);
static DEVICE_ATTR(sspm_available, 0444, sspm_available_show, NULL);

static ssize_t sspm_log_discard_show(struct device *dev, struct device_attribute *attr, char *buf);
static DEVICE_ATTR(sspm_log_discard, 0444, sspm_log_discard_show, NULL);

static ssize_t sspm_log_mode_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t sspm_log_mode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static DEVICE_ATTR(sspm_log_mode, 0664, sspm_log_mode_show, sspm_log_mode_store);

static ssize_t sspm_log_size_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t sspm_log_size_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static DEVICE_ATTR(sspm_log_size, 0664, sspm_log_size_show, sspm_log_size_store);


static ssize_t sspm_run_mode_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t sspm_run_mode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static DEVICE_ATTR(sspm_run_mode, 0664, sspm_run_mode_show, sspm_run_mode_store);

static ssize_t sspm_modules_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t sspm_modules_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static DEVICE_ATTR(sspm_modules, 0664, sspm_modules_show, sspm_modules_store);

static ssize_t sspm_op_ctrl_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static DEVICE_ATTR(sspm_op_ctrl, 0220, NULL, sspm_op_ctrl_store);


static ssize_t sspm_ipi_supported_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ipi_supported;
	int i;

	ipi_supported = 0;
#if IS_ENABLED(CONFIG_MTK_TINYSYS_SSPM_SUPPORT) && defined(ONDIEMET_SUPPORT)
	#ifndef SSPM_VERSION_V2
	ipi_supported = 1;
	#else
	if(sspm_ipidev_symbol)
		ipi_supported = 1;
	#endif
#endif
	i = snprintf(buf, PAGE_SIZE, "%d\n", ipi_supported);
	if (i < 0) return -1;

	return i;
}


static ssize_t sspm_buffer_size_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i;

	mutex_lock(&dev->mutex);
	i = snprintf(buf, PAGE_SIZE, "%d\n", sspm_buffer_size);
	mutex_unlock(&dev->mutex);
	return i;
}

static ssize_t sspm_available_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i;

	mutex_lock(&dev->mutex);
	i = snprintf(buf, PAGE_SIZE, "%d\n", 1);
	mutex_unlock(&dev->mutex);
	return i;
}

static ssize_t sspm_log_discard_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i;

	mutex_lock(&dev->mutex);
	i = snprintf(buf, PAGE_SIZE, "%d\n", met_sspm_log_discard);
	mutex_unlock(&dev->mutex);
	return i;
}

static ssize_t sspm_log_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i;

	mutex_lock(&dev->mutex);
	i = snprintf(buf, PAGE_SIZE, "%d\n", sspm_log_mode);
	mutex_unlock(&dev->mutex);
	return i;
}

static ssize_t sspm_log_mode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int value;

	if (kstrtoint(buf, 0, &value) != 0)
		return -EINVAL;
	mutex_lock(&dev->mutex);
	sspm_log_mode = value;
	mutex_unlock(&dev->mutex);
	return count;
}


static ssize_t sspm_log_size_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i;

	mutex_lock(&dev->mutex);
	i = snprintf(buf, PAGE_SIZE, "%d\n", sspm_log_size);
	mutex_unlock(&dev->mutex);
	return i;
}

static ssize_t sspm_log_size_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int value;

	if (kstrtoint(buf, 0, &value) != 0)
		return -EINVAL;
	mutex_lock(&dev->mutex);
	sspm_log_size = value;
	mutex_unlock(&dev->mutex);
	return count;
}


static ssize_t sspm_run_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i;

	mutex_lock(&dev->mutex);
	i = snprintf(buf, PAGE_SIZE, "%d\n", sspm_run_mode);
	mutex_unlock(&dev->mutex);
	return i;
}

static ssize_t sspm_run_mode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int value;

	if (kstrtoint(buf, 0, &value) != 0)
		return -EINVAL;
	mutex_lock(&dev->mutex);
	sspm_run_mode = value;
	mutex_unlock(&dev->mutex);
	return count;
}

static ssize_t sspm_op_ctrl_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int value;

	if (kstrtoint(buf, 0, &value) != 0)
		return -EINVAL;
	mutex_lock(&dev->mutex);
	if (value == 1)
		sspm_start();
	else if (value == 2)
		sspm_stop();
	else if (value == 3)
		sspm_extract();
	else if (value == 4)
		sspm_flush();
	mutex_unlock(&dev->mutex);
	return count;
}

static ssize_t sspm_modules_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i;

	mutex_lock(&dev->mutex);
	i = snprintf(buf, PAGE_SIZE, "%x\n", ondiemet_module[ONDIEMET_SSPM]);
	mutex_unlock(&dev->mutex);
	return i;
}

static ssize_t sspm_modules_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	uint32_t value;

	if (kstrtouint(buf, 0, &value) != 0)
		return -EINVAL;
	mutex_lock(&dev->mutex);
	ondiemet_module[ONDIEMET_SSPM] = value;
	mutex_unlock(&dev->mutex);
	return count;
}

int sspm_attr_init(struct device *dev)
{
	int ret;

#if IS_ENABLED(CONFIG_MTK_GMO_RAM_OPTIMIZE) || IS_ENABLED(CONFIG_MTK_MET_MEM_ALLOC)
#ifdef CONFIG_MET_ARM_32BIT
	struct dma_map_ops *ops = (struct dma_map_ops *)symbol_get(arm_coherent_dma_ops);

	if (ops && ops->alloc) {
		dev->coherent_dma_mask = DMA_BIT_MASK(32);
		ondiemet_sspm_log_virt_addr = ops->alloc(dev,
						ondiemet_sspm_log_size,
						&ondiemet_sspm_log_phy_addr,
						GFP_KERNEL,
						0);
	}
#else /* CONFIG_MET_ARM_32BIT */
	/* dma_alloc */
	ondiemet_sspm_log_virt_addr = dma_alloc_coherent(dev,
			ondiemet_sspm_log_size,
			&ondiemet_sspm_log_phy_addr,
			GFP_KERNEL);
#endif /* CONFIG_MET_ARM_32BIT */
#else /* CONFIG_MTK_GMO_RAM_OPTIMIZE */

	phys_addr_t (*sspm_reserve_mem_get_phys_sym)(unsigned int id) = NULL;
	phys_addr_t (*sspm_reserve_mem_get_virt_sym)(unsigned int id) = NULL;
	phys_addr_t (*sspm_reserve_mem_get_size_sym)(unsigned int id) = NULL;

	sspm_reserve_mem_get_phys_sym = (phys_addr_t (*)(unsigned int id))symbol_get(sspm_reserve_mem_get_virt);
	sspm_reserve_mem_get_virt_sym = (phys_addr_t (*)(unsigned int id))symbol_get(sspm_reserve_mem_get_phys);
	sspm_reserve_mem_get_size_sym = (phys_addr_t (*)(unsigned int id))symbol_get(sspm_reserve_mem_get_size);
	if (sspm_reserve_mem_get_phys_sym)
		ondiemet_sspm_log_virt_addr = (void*)sspm_reserve_mem_get_virt(MET_MEM_ID);
	if (sspm_reserve_mem_get_virt_sym)
		ondiemet_sspm_log_phy_addr = sspm_reserve_mem_get_phys(MET_MEM_ID);
	if (sspm_reserve_mem_get_size_sym)
		ondiemet_sspm_log_size = sspm_reserve_mem_get_size(MET_MEM_ID);
#endif /* CONFIG_MTK_GMO_RAM_OPTIMIZE */

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
	ret = device_create_file(dev, &dev_attr_sspm_op_ctrl);
	if (ret != 0) {
		pr_debug("can not create device file: sspm_op_ctrl\n");
		return ret;
	}
	ret = device_create_file(dev, &dev_attr_sspm_modules);
	if (ret != 0) {
		pr_debug("can not create device file: sspm_modules\n");
		return ret;
	}
	ret = device_create_file(dev, &dev_attr_sspm_ipi_supported);
	if (ret != 0) {
		pr_debug("can not create device file: sspm_ipi_supported\n");
		return ret;
	}

	if (ondiemet_sspm_log_virt_addr != NULL) {
		start_sspm_ipi_recv_thread();
		sspm_buf_available = 1;
		sspm_buffer_size = ondiemet_sspm_log_size;
	} else {
		sspm_buf_available = 0;
		sspm_buffer_size = -1;
	}

	return 0;
}

int sspm_attr_uninit(struct device *dev)
{
	/* dma_free */
	if (ondiemet_sspm_log_virt_addr != NULL) {
#if IS_ENABLED(CONFIG_MTK_GMO_RAM_OPTIMIZE) || IS_ENABLED(CONFIG_MTK_MET_MEM_ALLOC)
#ifdef CONFIG_MET_ARM_32BIT
		struct dma_map_ops *ops = (struct dma_map_ops *)symbol_get(arm_coherent_dma_ops);

		if (ops && ops->free) {
			ops->free(dev,
				ondiemet_sspm_log_size,
				ondiemet_sspm_log_virt_addr,
				ondiemet_sspm_log_phy_addr,
				0);
		}
#else /* CONFIG_MET_ARM_32BIT */
		dma_free_coherent(dev,
			ondiemet_sspm_log_size,
			ondiemet_sspm_log_virt_addr,
			ondiemet_sspm_log_phy_addr);
#endif /* CONFIG_MET_ARM_32BIT */
#endif /* CONFIG_MTK_GMO_RAM_OPTIMIZE */
		ondiemet_sspm_log_virt_addr = NULL;
		stop_sspm_ipi_recv_thread();
	}

	device_remove_file(dev, &dev_attr_sspm_buffer_size);
	device_remove_file(dev, &dev_attr_sspm_available);
	device_remove_file(dev, &dev_attr_sspm_log_discard);
	device_remove_file(dev, &dev_attr_sspm_log_mode);
	device_remove_file(dev, &dev_attr_sspm_log_size);
	device_remove_file(dev, &dev_attr_sspm_run_mode);
	device_remove_file(dev, &dev_attr_sspm_op_ctrl);
	device_remove_file(dev, &dev_attr_sspm_modules);
	device_remove_file(dev, &dev_attr_sspm_ipi_supported);

	return 0;
}

#if 0 /* move to sspm_attr_init() */
void sspm_get_buffer_info(void)
{
	if (ondiemet_sspm_log_virt_addr != NULL) {
		sspm_buf_available = 1;
		sspm_buffer_size = ondiemet_sspm_log_size;
	} else {
		sspm_buf_available = 0;
		sspm_buffer_size = -1;
	}
}
#endif

/*extern const char *met_get_platform_name(void);*/
extern char *met_get_platform(void);
void sspm_start(void)
{
	int32_t ret = 0;
	uint32_t rdata;
	uint32_t ipi_buf[4];
	const char* platform_name = NULL;
	unsigned int platform_id = 0;
	met_sspm_log_discard = -1;

	/* clear DRAM buffer */
	if (ondiemet_sspm_log_virt_addr != NULL)
		memset_io((void *)ondiemet_sspm_log_virt_addr, 0, ondiemet_sspm_log_size);
	else
		return;

	/*platform_name = met_get_platform_name();*/
	platform_name = met_get_platform();
	if (platform_name) {
		char buf[5];

		memset(buf, 0x0, 5);
		memcpy(buf, &platform_name[2], 4);

		ret = kstrtouint(buf, 10, &platform_id);
	}

	/* send DRAM physical address */
	ipi_buf[0] = MET_MAIN_ID | MET_BUFFER_INFO;
	ipi_buf[1] = (unsigned int)ondiemet_sspm_log_phy_addr; /* address */
	if (ret == 0)
		ipi_buf[2] = platform_id;
	else
		ipi_buf[2] = 0;

	ipi_buf[3] = 0;
	ret = met_ipi_to_sspm_command((void *)ipi_buf, 0, &rdata, 1);

	/* start ondiemet now */
	ipi_buf[0] = MET_MAIN_ID | MET_OP | MET_OP_START;
	ipi_buf[1] = ondiemet_module[ONDIEMET_SSPM];
	ipi_buf[2] = sspm_log_mode;
	ipi_buf[3] = sspm_run_mode;
	ret = met_ipi_to_sspm_command((void *)ipi_buf, 0, &rdata, 1);
}

extern unsigned int met_get_chip_id(void);
void sspm_stop(void)
{
	int32_t ret;
	uint32_t rdata;
	uint32_t ipi_buf[4];

	unsigned int chip_id = 0;
	chip_id = met_get_chip_id();

	if (sspm_buf_available == 1) {
		ipi_buf[0] = MET_MAIN_ID|MET_OP|MET_OP_STOP;
		ipi_buf[1] = chip_id;
		ipi_buf[2] = 0;
		ipi_buf[3] = 0;
		ret = met_ipi_to_sspm_command((void *)ipi_buf, 0, &rdata, 1);
	}
}

void sspm_extract(void)
{
	int32_t ret;
	uint32_t rdata;
	uint32_t ipi_buf[4];
	int32_t count;

	count = 20;
	if (sspm_buf_available == 1) {
		while ((sspm_buffer_dumping == 1) && (count != 0)) {
			msleep(50);
			count--;
		}
		ipi_buf[0] = MET_MAIN_ID|MET_OP|MET_OP_EXTRACT;
		ipi_buf[1] = 0;
		ipi_buf[2] = 0;
		ipi_buf[3] = 0;
		ret = met_ipi_to_sspm_command((void *)ipi_buf, 0, &rdata, 1);
	}

	if (sspm_run_mode == SSPM_RUN_NORMAL)
		ondiemet_module[ONDIEMET_SSPM] = 0;
}

void sspm_flush(void)
{
	int32_t ret;
	uint32_t rdata;
	uint32_t ipi_buf[4];

	if (sspm_buf_available == 1) {
		ipi_buf[0] = MET_MAIN_ID|MET_OP|MET_OP_FLUSH;
		ipi_buf[1] = 0;
		ipi_buf[2] = 0;
		ipi_buf[3] = 0;
		ret = met_ipi_to_sspm_command((void *)ipi_buf, 0, &rdata, 1);
	}

	if (sspm_run_mode == SSPM_RUN_NORMAL)
		ondiemet_module[ONDIEMET_SSPM] = 0;
}
#else /* CONFIG_MTK_TINYSYS_SSPM_SUPPORT && ONDIEMET_SUPPORT */
int sspm_buffer_size = -1;

int sspm_attr_init(struct device *dev)
{
	return 0;
}

int sspm_attr_uninit(struct device *dev)
{
	return 0;
}

void sspm_start(void)
{
}

void sspm_stop(void)
{
}

void sspm_extract(void)
{
}

void sspm_flush(void)
{
}

#endif /* CONFIG_MTK_TINYSYS_SSPM_SUPPORT && ONDIEMET_SUPPORT */
