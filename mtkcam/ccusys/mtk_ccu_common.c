// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2021 MediaTek Inc.

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/version.h>

#include <linux/dma-mapping.h>
#include <linux/dma-buf.h>
#include <linux/irqflags.h>
#include <linux/timekeeping.h>

#include "mtk_ccu_common.h"

#define MTK_CCU_TAG "[ccu_rproc]"
#define LOG_ERR(format, args...) \
	pr_err(MTK_CCU_TAG "[%s] " format, __func__, ##args)
#define DEV_INFO(dev, format, args...)
/* #define DEV_INFO dev_info(dev, format, args...) */

uint32_t ccu_mailbox_max;

static inline unsigned int mtk_ccu_mstojiffies(unsigned int Ms)
{
	return ((Ms * HZ + 512) >> 10);
}

void mtk_ccu_memclr(void *dst, int len)
{
	int i = 0;
	uint32_t *dstPtr = (uint32_t *)dst;

	for (i = 0; i < len/4; i++)
		writel(0, dstPtr + i);
}

void mtk_ccu_readl(void *dst, const void *src, uint32_t len)
{
	int i, copy_len;
	char data[4];

	for (i = 0; i < (len >> 2); ++i)
		*((uint32_t *)dst + i) = readl((uint32_t *)src + i);

	if ((len & 3) != 0) {
		copy_len = len & ~(0x3);
		*((uint32_t *)data) = readl((char *)src + copy_len);
		for (i = 0; i < (len & 3); ++i)
			*((char *)dst + copy_len + i) = data[i];
	}
}

void mtk_ccu_memcpy(void *dst, const void *src, uint32_t len)
{
	int i, copy_len;
	uint32_t data = 0;
	uint32_t align_data = 0;

	for (i = 0; i < (len >> 2); ++i)
		writel(*((uint32_t *)src + i), (uint32_t *)dst + i);

	if ((len & 3) != 0) {
		copy_len = len & ~(0x3);
		for (i = 0; i < (len & 3); ++i) {
			data = *((char *)src + copy_len + i);
			align_data += data << (i << 3);
		}
		writel(align_data, (uint32_t *)dst + (len >> 2));
	}
}

struct mtk_ccu_mem_info *mtk_ccu_get_meminfo(struct mtk_ccu *ccu,
	enum mtk_ccu_buffer_type type)
{
	if (type >= MTK_CCU_BUF_MAX)
		return NULL;
	else
		return &ccu->buffer_handle[type].meminfo;
}

static int mtk_ccu_open(struct inode *inode, struct file *flip)
{
	struct mtk_ccu *ccu = container_of(inode->i_cdev,
					   struct mtk_ccu,
					   ccu_cdev);

	flip->private_data = ccu;
	return 0;
}

static int mtk_ccu_release(struct inode *inode, struct file *flip)
{
	return 0;
}


static int mtk_ccu_wakeup(struct mtk_ccu *ccu)
{
	ccu->bWaitCond = true;
	wake_up_interruptible(&ccu->WaitQueueHead);
	return 0;
}

static int mtk_ccu_waitirq(struct mtk_ccu *ccu)
{
	signed int ret = 0, timeout = 0;

	timeout = wait_event_interruptible_timeout(
		ccu->WaitQueueHead,
		ccu->bWaitCond,
		mtk_ccu_mstojiffies(100));
	ccu->bWaitCond = false;


	if (timeout > 0) {
		DEV_INFO(ccu->dev, "remain_t:%d,log_idx:%d\n", timeout, ccu->g_LogBufIdx);
		ret = ccu->g_LogBufIdx;
	}

	return ret;
}

static long mtk_ccu_ioctl(struct file *flip, unsigned int cmd,
	unsigned long arg)
{
	int ret = 0;
	int log_idx;
	uint32_t log_level[2] = {0};
	struct mtk_ccu_buffer log_info = {0};
	struct mtk_ccu *ccu = flip->private_data;

	switch (cmd) {
	case MTK_CCU_IOCTL_SET_LOG_LEVEL:
	{

		ret = copy_from_user(log_level, (void *)arg, sizeof(log_level));
		if (ret) {
			dev_err(ccu->dev, "set log level failed\n");
			return -EFAULT;
		}
		ccu->log_level = log_level[0];
		ccu->log_taglevel = log_level[1];

		break;
	}
	case MTK_CCU_IOCTL_WAIT_IRQ:
	{
		log_idx = mtk_ccu_waitirq(ccu);
		if (log_idx < 0) {
			log_info = ccu->log_info[3];
			log_info.buf_idx = log_idx;
		} else if (log_idx > 0) {
			log_info = ccu->log_info[log_idx-1];
			log_info.buf_idx = log_idx;
		}
		log_info.ktime = ccu->ktime;
		log_info.gtick = ccu->gtick;
		log_info.systick_freq = ccu->systick_freq;
		ret = copy_to_user((void *)arg, &log_info,
			sizeof(struct mtk_ccu_buffer));
		if (ret) {
			dev_err(ccu->dev, "copy_to_user failed\n");
			ret = -EFAULT;
		}
		break;
	}
	case MTK_CCU_IOCTL_FLUSH_LOG:
	{
		mtk_ccu_wakeup(ccu);
		break;
	}

	default:
		dev_warn(ccu->dev, "ioctl:No such command!\n");
		ret = -EINVAL;
		break;
	}

	if (ret != 0) {
		dev_err(ccu->dev, "fail, cmd(%d), cmd_nr(%d)",
			cmd, _IOC_NR(cmd));
		dev_err(ccu->dev, "fail, (process, pid, tgid)=(%s, %d, %d)\n",
			current->comm, current->pid, current->tgid);
	}
	return ret;
}

static int mtk_ccu_mmap(struct file *flip,
	struct vm_area_struct *vma)
{
	unsigned long length = (vma->vm_end - vma->vm_start);
	struct mtk_ccu *ccu = flip->private_data;
	int ret;

	if (length > (MTK_CCU_DRAM_LOG_BUF_SIZE * 4))
		return -EINVAL;

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	if (ccu->smmu_enabled)
		ret = dma_buf_mmap(ccu->ext_buf.dmabuf, vma, vma->vm_pgoff);
	else
		ret = dma_mmap_attrs(ccu->dev, vma, ccu->ext_buf.meminfo.va,
			ccu->ext_buf.meminfo.mva, length, DMA_ATTR_WRITE_COMBINE);

	if (ret)
		dev_err(ccu->dev, "Remapping memory failed, error: %d\n", ret);
	return ret;
}

static const struct file_operations mtk_ccu_fops = {
	.owner = THIS_MODULE,
	.open = mtk_ccu_open,
	.release = mtk_ccu_release,
	.unlocked_ioctl = mtk_ccu_ioctl,
	.mmap = mtk_ccu_mmap,
};

void mtk_ccu_unreg_chardev(struct mtk_ccu *ccu)
{
	cdev_del(&ccu->ccu_cdev);
	unregister_chrdev_region(ccu->dev_no, 1);
}

int mtk_ccu_reg_chardev(struct mtk_ccu *ccu)
{
	struct device *dev = ccu->dev;
	int cdev_ret = -1;
	int alloc_ret = -1;
	int ret = 0;

	alloc_ret = alloc_chrdev_region(&ccu->dev_no, 0, 1, MTK_CCU_DEV_NAME);
	if (alloc_ret) {
		dev_err(dev, "failed to alloc chr_dev region, %d\n", alloc_ret);
		goto ERR;
	}

	cdev_init(&ccu->ccu_cdev, &mtk_ccu_fops);
	ccu->ccu_cdev.owner = THIS_MODULE;
	cdev_ret = cdev_add(&ccu->ccu_cdev, ccu->dev_no, 1);
	if (cdev_ret) {
		dev_err(dev, "Attach file operation failed, %d\n", cdev_ret);
		goto ERR;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
	ccu->ccu_class = class_create("ccurprocdrv");
#else
	ccu->ccu_class = class_create(THIS_MODULE, "ccurprocdrv");
#endif
	if (IS_ERR(ccu->ccu_class)) {
		ret = PTR_ERR(ccu->ccu_class);
		dev_err(dev, "Unable to create class, err = %d\n", ret);
		goto ERR;
	}
	ccu->ccu_cdev_dev = device_create(
	ccu->ccu_class, dev, ccu->dev_no, NULL, MTK_CCU_DEV_NAME);
	if (IS_ERR(ccu->ccu_cdev_dev)) {
		ret = PTR_ERR(ccu->ccu_cdev_dev);
		dev_err(ccu->dev,
		"Failed to create device: /dev/%s, err = %d",
		MTK_CCU_DEV_NAME, ret);
		goto ERR;
	}


	return 0;
ERR:
	if (alloc_ret == 0)
		unregister_chrdev_region(ccu->dev_no, 1);
	if (cdev_ret == 0)
		cdev_del(&ccu->ccu_cdev);
	return -EFAULT;
}

void mtk_ccu_ipc_log_handle(uint32_t data, uint32_t len, void *priv)
{
	struct mtk_ccu *ccu = priv;
#if IS_ENABLED(CONFIG_MTK_CCU_DEBUG)
	unsigned long flags;
#endif

	if (ccu == NULL)
		return;

#if IS_ENABLED(CONFIG_MTK_CCU_DEBUG)
	if (data > LOG_BUF_IDX_MAX) {
		dev_info(ccu->dev, "BUG: log buf idx %d\n", data);
		return;
	}
#endif

	DEV_INFO(ccu->dev, "got APMCU_FLUSH_LOG:%d\n", data);
#if IS_ENABLED(CONFIG_MTK_CCU_DEBUG)
	ccu->bWaitCond = true;
	ccu->g_LogBufIdx = (uint32_t)data;
	local_irq_save(flags);
	ccu->ktime = ktime_get_ns();
	ccu->gtick = (uint32_t)arch_timer_read_counter();
	local_irq_restore(flags);
	wake_up_interruptible(&ccu->WaitQueueHead);
#endif
}

void mtk_ccu_ipc_assert_handle(uint32_t data, uint32_t len, void *priv)
{
	struct mtk_ccu *ccu = priv;

	if (ccu == NULL)
		return;

	dev_err(ccu->dev, "got AP_ISR_CCU_ASSERT:%d, para 0x%x\n", data, len);

#if IS_ENABLED(CONFIG_MTK_CCU_DEBUG)
	ccu->bWaitCond = true;
	ccu->g_LogBufIdx = 0xFFFFFFFF;
	wake_up_interruptible(&ccu->WaitQueueHead);
#else
	/*
	 * SRAM log @ccu->dmem_base + MTK_CCU_SRAM_LOG_OFFSET, size MTK_CCU_SRAM_LOG_BUF_SIZE
	 * CCU CSR @ccu->ccu_base, size MTK_CCU_REG_LOG_BUF_SIZE-MTK_CCU_EXTRA_REG_LOG_BUF_SIZE
	 * CCU EXTRA CSR @ccu->ccu_base + MTK_CCU_EXTRA_REG_OFFSET, size MTK_CCU_EXTRA_REG_LOG_BUF_SIZE
	 */
#endif
}

void mtk_ccu_ipc_warning_handle(uint32_t data, uint32_t len, void *priv)
{
	struct mtk_ccu *ccu = priv;

	if (ccu == NULL)
		return;

	dev_err(ccu->dev, "got AP_ISR_CCU_WARNING:%d\n", data);

#if IS_ENABLED(CONFIG_MTK_CCU_DEBUG)
	ccu->bWaitCond = true;
	ccu->g_LogBufIdx = -2;
	wake_up_interruptible(&ccu->WaitQueueHead);
#endif
}

int rproc_bootx(struct rproc *rproc, unsigned int uid)
{
	struct mtk_ccu *ccu;
	int ret;

	if (!rproc) {
		LOG_ERR("rproc is NULL");
		return -EINVAL;
	}

	ccu = (struct mtk_ccu *)rproc->priv;
	if (!ccu) {
		LOG_ERR("ccu is NULL");
		return -EINVAL;
	}

	if (uid < RPROC_UID_MAX)
		atomic_inc(&ccu->bootcnt[uid][0]);

	ret = rproc_boot(rproc);

	if ((ret) && (uid < RPROC_UID_MAX))
		atomic_inc(&ccu->bootcnt[uid][1]);

	return ret;
}
EXPORT_SYMBOL_GPL(rproc_bootx);

int rproc_shutdownx(struct rproc *rproc, unsigned int uid)
{
	struct mtk_ccu *ccu;
	int ret;

	if (!rproc) {
		LOG_ERR("rproc is NULL");
		return -EINVAL;
	}

	ccu = (struct mtk_ccu *)rproc->priv;
	if (!ccu) {
		LOG_ERR("ccu is NULL");
		return -EINVAL;
	}

	if (uid < RPROC_UID_MAX)
		atomic_dec(&ccu->bootcnt[uid][0]);

	ret = rproc_shutdown(rproc);
	if ((ret) && (uid < RPROC_UID_MAX))
		atomic_inc(&ccu->bootcnt[uid][2]);

	return ret;
}
EXPORT_SYMBOL_GPL(rproc_shutdownx);

MODULE_DESCRIPTION("MTK CCU Rproc Driver");
MODULE_LICENSE("GPL v2");
