/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
#include <asm/io.h>

#include "gps_dl_config.h"
#include "gps_dl_log.h"

#include "gps_nv_each_device.h"
#include "gps_mcusys_nv_data_api.h"


static void gps_nv_each_device_memset(gpsmdl_u8 *p_dst, const gpsmdl_u8 dat_u8, gpsmdl_u32 len)
{
	(void)memset_io(p_dst, dat_u8, len);
}

static int gps_nv_each_device_open(struct inode *inode, struct file *filp)
{
	int pid;
	struct gps_nv_each_device *p_dev;
	enum gps_mcusys_nv_data_id nv_id;
	int retval;
	bool old_is_open;
	unsigned int file_flags;
	bool do_trim;
	int trim_result;


	p_dev = container_of(inode->i_cdev, struct gps_nv_each_device, cdev);
	filp->private_data = p_dev;
	pid = current->pid;
	nv_id = p_dev->nv_id;
	old_is_open = p_dev->is_open;

	file_flags = filp->f_flags;
	GDL_LOGD("pid=%d, nv_id=%d, major=%d, minor=%d, old_is_open=%d, flgas=0x%x",
		pid, nv_id, imajor(inode), iminor(inode), old_is_open, file_flags);

	retval = -EBUSY;
	do_trim = false;
	trim_result = 0;
	/* TODO: In future, may intro mutex and allow multiple user(open/read/write).
	 * Currently, retrun failure for multiple open operation.
	 */
	if (!old_is_open) {
		/* TODO: Check and handle more flags, such as NON_BLOCK */
		if ((file_flags & O_ACCMODE) == O_WRONLY) {
			/* TODO: retry for lock failure or block until seccess */
			trim_result = gps_mcusys_nv_common_shared_mem_invalidate2(nv_id,
				&gps_nv_each_device_memset);
			do_trim = true;
		}

		if (trim_result == 0)
			retval = 0;
	}

	if (retval == 0) {
		p_dev->is_open = true;
		gps_mcusys_nv_common_shared_mem_set_local_open(nv_id, true);
	}

	if (nv_id == GPS_MCUSYS_NV_DATA_ID_NVFILE ||
		nv_id == GPS_MCUSYS_NV_DATA_ID_AP_MPE ||
		old_is_open)
		GDL_LOGW("pid=%d, nv_id=%d, retval=%d, trim=%d,%d, old_is_open=%d, flgas=0x%x",
			pid, nv_id, retval, do_trim, trim_result,
			old_is_open, file_flags);
	else
		GDL_LOGD("pid=%d, nv_id=%d, retval=%d, trim=%d,%d, old_is_open=%d, flgas=0x%x",
			pid, nv_id, retval, do_trim, trim_result,
			old_is_open, file_flags);

	return retval;
}

static int gps_nv_each_device_release(struct inode *inode, struct file *filp)
{
	int pid;
	struct gps_nv_each_device *p_dev;
	enum gps_mcusys_nv_data_id nv_id;
	bool old_is_open;

	p_dev = (struct gps_nv_each_device *)filp->private_data;
	old_is_open = p_dev->is_open;
	p_dev->is_open = false;
	pid = current->pid;
	nv_id = p_dev->nv_id;
	gps_mcusys_nv_common_shared_mem_set_local_open(nv_id, false);

	if (nv_id == GPS_MCUSYS_NV_DATA_ID_NVFILE ||
		nv_id == GPS_MCUSYS_NV_DATA_ID_AP_MPE ||
		!old_is_open)
		GDL_LOGW("pid=%d, nv_id=%d, major=%d, minor=%d, old_is_open=%d",
			pid, nv_id, imajor(inode), iminor(inode), old_is_open);
	else
		GDL_LOGD("pid=%d, nv_id=%d, major=%d, minor=%d, old_is_open=%d",
			pid, nv_id, imajor(inode), iminor(inode), old_is_open);
	return 0;
}

static unsigned int gps_nv_each_device_poll(struct file *filp, poll_table *p_table)
{
	unsigned int mask = 0;
	int pid;
	struct gps_nv_each_device *p_dev;
	enum gps_mcusys_nv_data_id nv_id;
	struct gps_each_link_waitable *p_rd_waitable;

	p_dev = (struct gps_nv_each_device *)filp->private_data;
	pid = current->pid;
	nv_id = p_dev->nv_id;

	p_rd_waitable = gps_nv_each_link_get_read_waitable_ptr(nv_id);
	if (!p_rd_waitable) {
		GDL_LOGE("pid=%d, nv_id=%d, mask=0x%x, no waitable", pid, nv_id, mask);
		return mask;
	}

	poll_wait(filp, &p_rd_waitable->wq, p_table);
	if (GDL_OKAY == gps_dl_waitable_try_wait_on(p_rd_waitable)) {
		mask = (POLLIN | POLLRDNORM);
		p_dev->epoll_in_flag = true;
		GDL_LOGI("pid=%d, nv_id=%d, mask=0x%x", pid, nv_id, mask);
	} else
		GDL_LOGD("pid=%d, nv_id=%d, mask=0x%x", pid, nv_id, mask);
	return mask;
}

#define COPY_MAX 128

static gpsmdl_u32 gps_nv_each_device_copy_to_user(gpsmdl_u8 *p_dst, const gpsmdl_u8 *p_src, gpsmdl_u32 len)
{
	int retval;
	char __user *p_user;
	gpsmdl_u8 temp_buffer[COPY_MAX];
	const gpsmdl_u8 *p_src2 = p_src;
	unsigned int copy_len, cnt_len;

	p_user = (char __user *)p_dst;

	/* copy device mem into user buffer
	 * 1.copy device mem into a kernel buffer
	 * 2.copy kernel buffer into user buffer
	 */
	cnt_len = len;
	while (cnt_len > 0) {
		copy_len = (cnt_len >= COPY_MAX) ? COPY_MAX : cnt_len;
		memcpy_fromio(temp_buffer, p_src2, copy_len);
		retval = copy_to_user(p_user, temp_buffer, copy_len);
		if (retval != 0) {
			GDL_LOGI_DRW("copy_to_user: len=%d, retval=%d", copy_len, retval);
			return (len - cnt_len);
		}
		p_src2 = p_src2 + copy_len;
		p_user = p_user + copy_len;
		cnt_len = cnt_len - copy_len;
	}
	return len;
}

static ssize_t gps_nv_each_device_read(struct file *filp,
	char __user *buf, size_t count, loff_t *f_pos)
{
	int retlen = 0;
	int pid;
	struct gps_nv_each_device *p_dev;
	enum gps_mcusys_nv_data_id nv_id;
	gpsmdl_u32 copy_len = 0;
	bool epoll_in_flag = false;
	bool data_ready_flag = false;
	enum GDL_RET_STATUS try_wait_status;
	struct gps_each_link_waitable *p_rd_waitable;
	long l_offset = 0;
	gpsmdl_u32 u32_offset;

	p_dev = (struct gps_nv_each_device *)filp->private_data;
	pid = current->pid;
	nv_id = p_dev->nv_id;

	if (count > 0) {
		epoll_in_flag = p_dev->epoll_in_flag;
		if (!epoll_in_flag) {
			p_rd_waitable = gps_nv_each_link_get_read_waitable_ptr(nv_id);
			if (p_rd_waitable) {
				try_wait_status =
					gps_dl_waitable_try_wait_on(p_rd_waitable);
				data_ready_flag = (try_wait_status == GDL_OKAY);
			}
		}

		l_offset = (long)(*f_pos);
		if (l_offset >= 0 && l_offset <= (long)0xFFFFFFFF)
			u32_offset = (gpsmdl_u32)l_offset;
		else
			u32_offset = 0xFFFFFFFF;

		/* TODO: Change return type of gps_mcusys_nv_common_shared_mem_read2 to int,
		 * making it can bring fail value, such as copy_to_user failure.
		 */
		/* TODO: retry for lock failure or block until seccess */
		copy_len = gps_mcusys_nv_common_shared_mem_read2(nv_id,
			(gpsmdl_u8 *)buf, (gpsmdl_u32)count, u32_offset,
			&gps_nv_each_device_copy_to_user);
		*f_pos += copy_len;
	}
	GDL_LOGI("pid=%d, nv_id=%d, buf_len=%zu, copy_len=%d, offset=%ld, flag=%d,%d",
		pid, nv_id, count, copy_len, l_offset, epoll_in_flag, data_ready_flag);

	/* TODO: retlen might be negtive value in future if we need to handle signals in read */
	retlen = (int)copy_len;
	return retlen;
}

static gpsmdl_u32 gps_nv_each_device_copy_from_user(gpsmdl_u8 *p_dst, const gpsmdl_u8 *p_src, gpsmdl_u32 len)
{
	int retval;
	const char __user *p_user;
	gpsmdl_u8 temp_buffer[COPY_MAX];
	gpsmdl_u8 *p_dst2 = p_dst;
	unsigned int copy_len, cnt_len;

	p_user = (const char __user *)p_src;

	/* copy user buffer into device mem
	 * 1.copy user buffer into a kernel buffer
	 * 2.copy kernel buffer into device memory
	 */
	cnt_len = len;
	while (cnt_len > 0) {
		copy_len = (cnt_len >= COPY_MAX) ? COPY_MAX : cnt_len;
		retval = copy_from_user(temp_buffer, p_user, copy_len);
		if (retval != 0) {
			GDL_LOGI_DRW("copy_from_user: len=%d, retval=%d", copy_len, retval);
			return (len - cnt_len);
		}
		memcpy_toio(p_dst2, temp_buffer, copy_len);
		p_user = p_user + copy_len;
		p_dst2 = p_dst2 + copy_len;

		cnt_len = cnt_len - copy_len;
	}
	return len;
}

static ssize_t gps_nv_each_device_write(struct file *filp,
	const char __user *buf, size_t count, loff_t *f_pos)
{
	int retlen = 0;
	int copy_len;
	int pid;
	struct gps_nv_each_device *p_dev;
	enum gps_mcusys_nv_data_id nv_id;
	long l_offset;
	gpsmdl_u32 u32_offset;

	p_dev = (struct gps_nv_each_device *)filp->private_data;
	pid = current->pid;
	nv_id = p_dev->nv_id;

	l_offset = (long)(*f_pos);
	if (l_offset >= 0 && l_offset <= (long)0xFFFFFFFF)
		u32_offset = (gpsmdl_u32)l_offset;
	else
		u32_offset = 0xFFFFFFFF;

	/* TODO: Change return type of gps_mcusys_nv_common_shared_mem_write2 to int,
	 * making it can bring fail value, such as copy_from_user failure.
	 */
	/* TODO: retry for lock failure or block until seccess */
	copy_len = gps_mcusys_nv_common_shared_mem_write2(nv_id,
		(const gpsmdl_u8 *)buf, (gpsmdl_u32)count, u32_offset,
		&gps_nv_each_device_copy_from_user);
	*f_pos += copy_len;

	/* TODO: retlen might be negtive value in future if we need to handle signals in write */
	retlen = (int)copy_len;
	GDL_LOGI("pid=%d, nv_id=%d, wr_len=%zu, offset=%ld, ret_len=%d",
		pid, nv_id, count, l_offset, retlen);
	return retlen;
}

static loff_t gps_nv_each_device_seek(struct file *filp, loff_t offset, int whence)
{
	int retval;
	int pid;
	struct gps_nv_each_device *p_dev;
	enum gps_mcusys_nv_data_id nv_id;
	gpsmdl_u32 data_size = 0, block_size = 0;
	int get_info_ret;
	bool get_info_okay;
	loff_t old_pos, new_pos;

	p_dev = (struct gps_nv_each_device *)filp->private_data;
	pid = current->pid;
	nv_id = p_dev->nv_id;
	old_pos = filp->f_pos;
	new_pos = old_pos;
	get_info_ret = gps_mcusys_nv_common_shared_mem_get_info(nv_id, &data_size, &block_size);
	get_info_okay = (get_info_ret == 0);

	retval = -EINVAL;
	if (get_info_okay) {
		retval = 0;
		switch (whence) {
		case 0: /* SEEK_SET */
			new_pos = offset;
			break;
		case 1: /* SEEK_CUR */
			new_pos = old_pos + offset;
			break;
		case 2: /* SEEK_END */
			new_pos = data_size + offset;
			break;
		default:
			retval = -EINVAL;
			break;
		}

		if (new_pos >= 0 && new_pos <= data_size && retval == 0)
			retval = 0;
	}

	if (retval == 0)
		filp->f_pos = new_pos;

	GDL_LOGI("pid=%d, nv_id=%d, case=%d, off=%ld, pos=%ld->%ld, info=%d,%u,%u, ret_val=%d",
		pid, nv_id, whence,
		(long)offset, (long)old_pos, (long)new_pos,
		get_info_ret, data_size, block_size,
		retval);
	return retval;
}


#define GPS_NV_IOC_QUERY_BLOCK_SIZE     1
#define GPS_NV_IOC_QUERY_DATA_SIZE      2
#define GPS_NV_IOC_CLEAR_DATA           3

static int gps_nv_each_device_ioctl_inner(struct file *filp, unsigned int cmd, unsigned long arg, bool is_compat)
{
	int retval = 0;
	int pid;
	struct gps_nv_each_device *p_dev;
	enum gps_mcusys_nv_data_id nv_id;
	gpsmdl_u32 data_size = 0, block_size = 0;
	int get_info_ret;
	int trim_result;

	p_dev = (struct gps_nv_each_device *)filp->private_data;
	pid = current->pid;
	nv_id = p_dev->nv_id;

	switch (cmd) {
	case GPS_NV_IOC_QUERY_BLOCK_SIZE:
		get_info_ret = gps_mcusys_nv_common_shared_mem_get_info(nv_id, &data_size, &block_size);
		if (get_info_ret < 0)
			retval = -EFAULT;
		else
			retval = block_size;
		break;
	case GPS_NV_IOC_QUERY_DATA_SIZE:
		get_info_ret = gps_mcusys_nv_common_shared_mem_get_info(nv_id, &data_size, &block_size);
		if (get_info_ret < 0)
			retval = -EFAULT;
		else
			retval = data_size;
		break;
	case GPS_NV_IOC_CLEAR_DATA:
		trim_result = gps_mcusys_nv_common_shared_mem_invalidate2(nv_id,
			&gps_nv_each_device_memset);
		if (trim_result < 0)
			retval = -EFAULT;
		else
			retval = 0;
		break;
	default:
		retval = -EINVAL;
		break;
	}

	if (cmd == GPS_NV_IOC_QUERY_BLOCK_SIZE ||
		cmd == GPS_NV_IOC_QUERY_DATA_SIZE ||
		(cmd == GPS_NV_IOC_CLEAR_DATA && trim_result < 0))
		GDL_LOGI("pid=%d, nv_id=%d, cmd=%d, arg=%ld, is_compat=%d, retval=%d",
			pid, nv_id, cmd, arg, is_compat, retval);
	return retval;
}

static long gps_nv_each_device_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	return gps_nv_each_device_ioctl_inner(filp, cmd, arg, false);
}

static long gps_nv_each_device_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	return gps_nv_each_device_ioctl_inner(filp, cmd, arg, true);
}

static const struct file_operations gps_nv_each_device_fops = {
	.owner = THIS_MODULE,
	.open = gps_nv_each_device_open,
	.poll = gps_nv_each_device_poll,
	.read = gps_nv_each_device_read,
	.write = gps_nv_each_device_write,
	.release = gps_nv_each_device_release,
	.llseek = gps_nv_each_device_seek,

	/* Add ioctl to support get data/block size and trim data size */
	.unlocked_ioctl = gps_nv_each_device_unlocked_ioctl,
	.compat_ioctl = gps_nv_each_device_compat_ioctl,
};

int  gps_nv_cdev_setup(struct gps_nv_each_device *dev, enum gps_mcusys_nv_data_id nv_id)
{
	int result;

	gps_dl_link_waitable_init(&dev->wait_read, GPS_DL_WAIT_READ);
	dev->nv_id = nv_id;
	dev->is_open = false;
	dev->epoll_in_flag = false;

	cdev_init(&dev->cdev, &gps_nv_each_device_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &gps_nv_each_device_fops;

	result = cdev_add(&dev->cdev, dev->devno, 1);
	if (result) {
		GDL_LOGE("nv_id=%d, cdev_add: error=%d", nv_id, result);
		return result;
	}

	GDL_LOGD("nv_id=%d, class_create: %s", nv_id, dev->cfg.dev_name);
	dev->cls = class_create(THIS_MODULE, dev->cfg.dev_name);
	if (IS_ERR(dev->cls)) {
		GDL_LOGE("nv_id=%d, class_create: %s, failed", nv_id, dev->cfg.dev_name);
		return -1;
	}

	dev->dev = device_create(dev->cls, NULL, dev->devno, NULL, dev->cfg.dev_name);
	if (IS_ERR(dev->dev)) {
		GDL_LOGE("nv_id=%d, device_create: %s, failed", nv_id, dev->cfg.dev_name);
		return -1;
	}

	return 0;
}

void gps_nv_cdev_cleanup(struct gps_nv_each_device *p_dev, enum gps_mcusys_nv_data_id nv_id)
{
	if (p_dev->dev) {
		device_destroy(p_dev->cls, p_dev->devno);
		p_dev->dev = NULL;
	}

	if (p_dev->cls) {
		class_destroy(p_dev->cls);
		p_dev->cls = NULL;
	}

	cdev_del(&p_dev->cdev);
}

void gps_nv_device_context_init(void)
{
}

void gps_nv_device_context_deinit(void)
{
}

