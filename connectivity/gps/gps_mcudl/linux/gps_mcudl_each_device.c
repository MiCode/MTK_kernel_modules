/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */
#include <asm/arch_timer.h>
#include "gps_each_device.h"
#include "gps_each_link.h"
#if GPS_DL_MOCK_HAL
#include "gps_mock_mvcd.h"
#endif
#include "gps_dl_hal.h"
#include "gps_dl_ctrld.h"
#include "gps_data_link_devices.h"
#include "gps_dl_subsys_reset.h"
#include "gps_dl_hist_rec.h"
#include "gps_dl_linux_plat_drv.h"
#include "gps_mcudl_each_device.h"
#include "gps_mcudl_each_link.h"
#include "gps_mcudl_link_state.h"
#include "gps_mcudl_log.h"
#if GPS_DL_GET_PLATFORM_CLOCK_FREQ
#include "gps_dl_linux_clock_mng.h"
#endif
#include "gps_mcudl_hal_conn.h"


static bool gps_mcudl_xdevice_should_be_less_log(enum gps_mcudl_xid x_id)
{
	return (
		x_id == GPS_MDLX_GDLOG ||
		x_id == GPS_MDLX_GDLOG2 ||
		x_id == GPS_MDLX_MPELOG ||
		x_id == GPS_MDLX_MPELOG2
	);
}

static unsigned int gps_mcudl_each_device_poll(struct file *filp, poll_table *p_table)
{
	unsigned int mask = 0;
	int pid;
	struct gps_mcudl_each_device *dev;
	enum gps_mcudl_xid link_id;
	wait_queue_head_t *p_wq;

	dev = (struct gps_mcudl_each_device *)filp->private_data;
	pid = current->pid;
	link_id = (enum gps_mcudl_xid)dev->index;

	/* Fix @210821:
	 *   Do not return before poll_wait to fix the issue epoll_wait cannot be waken up.
	 */
#if 0
	if (gps_each_link_poll_is_in_data_ready(link_id)) {
		mask = (POLLIN | POLLRDNORM);
		MDL_LOGXI_DRW(link_id, "no_wait: pid=%d, mask=0x%x", pid, mask);
		return mask;
	}
#endif

	p_wq = gps_mcudl_each_link_poll_get_wq_ptr(link_id);
	MDL_LOGXD_DRW(link_id, "poll_wait: pid=%d, start", pid);
	poll_wait(filp, p_wq, p_table);
	if (gps_mcudl_each_link_poll_is_in_data_ready(link_id))
		mask = (POLLIN | POLLRDNORM);

	gps_mcudl_each_link_rec_poll(link_id, pid, mask);
	if (gps_mcudl_xdevice_should_be_less_log(link_id)) {
		/* do not print for gps debug log poll to reduce kernel log */
		return mask;
	}

	MDL_LOGXI_DRW(link_id, "poll_wait: pid=%d, mask=0x%x", pid, mask);
	return mask;
}

static ssize_t gps_mcudl_each_device_read(struct file *filp,
	char __user *buf, size_t count, loff_t *f_pos)
{
	int retval;
	int retlen;
	int pid;
	struct gps_mcudl_each_device *dev;
	enum gps_mcudl_xid link_id;
	bool print_log = false;

	dev = (struct gps_mcudl_each_device *)filp->private_data;
	pid = current->pid;
	link_id = (enum gps_mcudl_xid)dev->index;

	/* show read log after ram code downloading to avoid print too much */
	if (!gps_mcudl_xdevice_should_be_less_log(link_id)) {
		MDL_LOGXD_DRW(link_id, "buf_len = %ld, pid = %d", count, pid);
		print_log = true;
	} else
		MDL_LOGXD_DRW(link_id, "buf_len = %ld, pid = %d", count, pid);
	gps_mcudl_each_link_rec_read_start(link_id, pid, count);

	retlen = gps_mcudl_each_link_read(link_id, &dev->i_buf[0], count);
	if (retlen > 0) {
		retval = copy_to_user(buf, &dev->i_buf[0], retlen);
		if (retval != 0) {
			MDL_LOGXW_DRW(link_id, "copy to user len = %d, retval = %d",
				retlen, retval);
			retlen = -EFAULT;
		}
	}
	if (retlen <= 0)
		MDL_LOGXW_DRW(link_id, "buf_len = %ld, pid = %d, ret_len = %d", count, pid, retlen);
	if (print_log)
		MDL_LOGXI_DRW(link_id, "buf_len = %ld, pid = %d, ret_len = %d", count, pid, retlen);
	else
		MDL_LOGXD_DRW(link_id, "buf_len = %ld, pid = %d, ret_len = %d", count, pid, retlen);
	gps_mcudl_each_link_rec_read_end(link_id, pid, retlen);
	return retlen;
}

static ssize_t gps_mcudl_each_device_write(struct file *filp,
	const char __user *buf, size_t count, loff_t *f_pos)
{
	int retval;
	int retlen = 0;
	int copy_size;
	int pid;
	struct gps_mcudl_each_device *dev;
	enum gps_mcudl_xid link_id;
	bool print_log = false;

	dev = (struct gps_mcudl_each_device *)filp->private_data;
	pid = current->pid;
	link_id = (enum gps_mcudl_xid)dev->index;

	MDL_LOGXD_DRW(link_id, "len = %ld, pid = %d", count, pid);
	gps_mcudl_each_link_rec_write_start(link_id, pid, count);
	if (count > 0) {
		if (count > GPS_DATA_PATH_BUF_MAX) {
			MDL_LOGXW_DRW(link_id, "len = %ld is too long", count);
			copy_size = GPS_DATA_PATH_BUF_MAX;
		} else
			copy_size = count;

		retval = copy_from_user(&dev->o_buf[0], &buf[0], copy_size);
		if (retval != 0) {
			MDL_LOGXW_DRW(link_id, "copy from user len = %d, retval = %d",
				copy_size, retval);
			retlen = -EFAULT;
		} else {
			retval = gps_mcudl_each_link_write(link_id, &dev->o_buf[0], copy_size);
			if (retval == 0)
				retlen = copy_size;
			else
				retlen = retval;
		}
	}

	if (print_log)
		MDL_LOGXI_DRW(link_id, "len = %ld, pid = %d, ret_len = %d", count, pid, retlen);
	else
		MDL_LOGXD_DRW(link_id, "len = %ld, pid = %d, ret_len = %d", count, pid, retlen);
	gps_mcudl_each_link_rec_write_end(link_id, pid, retlen);
	return retlen;
}

#if 0
void gps_each_device_data_submit(unsigned char *buf, unsigned int len, int index)
{
	struct gps_mcudl_each_device *dev;

	dev = gps_dl_device_get(index);

	MDL_LOGI("gps_each_device_data_submit len = %d, index = %d, dev = %p",
		len, index, dev);

	if (!dev)
		return;

	if (!dev->is_open)
		return;

#if GPS_DL_CTRLD_MOCK_LINK_LAYER
	/* TODO: using mutex, len check */
	memcpy(&dev->i_buf[0], buf, len);
	dev->i_len = len;
	wake_up(&dev->r_wq);
#else
	gps_dl_add_to_rx_queue(buf, len, index);
	/* wake_up(&dev->r_wq); */
#endif

	MDL_LOGI("gps_each_device_data_submit copy and wakeup done");
}
#endif

static int gps_mcudl_each_device_open(struct inode *inode, struct file *filp)
{
	struct gps_mcudl_each_device *dev; /* device information */
	int retval = -EBUSY;

	dev = container_of(inode->i_cdev, struct gps_mcudl_each_device, cdev);
	filp->private_data = dev; /* for other methods */

	MDL_LOGXD(dev->index, "major = %d, minor = %d, pid = %d",
		imajor(inode), iminor(inode), current->pid);

	if (!dev->is_open) {
		retval = gps_mcudl_each_link_open((enum gps_mcudl_xid)dev->index);
		if (0 == retval) {
			dev->is_open = true;
			/*gps_each_link_rec_reset(dev->index);*/
		}
	}

	MDL_LOGXW(dev->index, "major = %d, minor = %d, pid = %d, retval=%d",
		imajor(inode), iminor(inode), current->pid, retval);
	return retval;
}

static int gps_mcudl_each_device_hw_resume(enum gps_mcudl_xid link_id)
{
#if 0
	int pid;
	int retval;

	pid = current->pid;
	MDL_LOGXW(link_id, "pid = %d", pid);

	retval = gps_each_link_hw_resume(link_id);

	/* device read may arrive before resume, not reset the recording here
	 * gps_each_link_rec_reset(link_id);
	 */

	return retval;
#endif
	return -EINVAL;
}

static int gps_mcudl_each_device_release(struct inode *inode, struct file *filp)
{
	struct gps_mcudl_each_device *dev;

	dev = (struct gps_mcudl_each_device *)filp->private_data;
	dev->is_open = false;

	MDL_LOGXW(dev->index, "major = %d, minor = %d, pid = %d",
		imajor(inode), iminor(inode), current->pid);

	gps_mcudl_each_link_close((enum gps_mcudl_xid)dev->index);
	/*gps_each_link_rec_force_dump(dev->index);*/

	return 0;
}

static int gps_mcudl_each_device_hw_suspend(enum gps_mcudl_xid link_id, bool need_clk_ext)
{
#if 0
	int pid;
	int retval;

	pid = current->pid;
	MDL_LOGXW(link_id, "pid = %d, clk_ext = %d", pid, need_clk_ext);

	retval = gps_each_link_hw_suspend(link_id, need_clk_ext);
	gps_each_link_rec_force_dump(link_id);

	return retval;
#endif
	return -EINVAL;
}

#define GPSDL_IOC_GPS_HWVER            6
#define GPSDL_IOC_GPS_IC_HW_VERSION    7
#define GPSDL_IOC_GPS_IC_FW_VERSION    8
#define GPSDL_IOC_D1_EFUSE_GET         9
#define GPSDL_IOC_RTC_FLAG             10
#define GPSDL_IOC_CO_CLOCK_FLAG        11
#define GPSDL_IOC_TRIGGER_ASSERT       12
#define GPSDL_IOC_QUERY_STATUS         13
#define GPSDL_IOC_TAKE_GPS_WAKELOCK    14
#define GPSDL_IOC_GIVE_GPS_WAKELOCK    15
#define GPSDL_IOC_GET_GPS_LNA_PIN      16
#define GPSDL_IOC_GPS_FWCTL            17
#define GPSDL_IOC_GPS_HW_SUSPEND       18
#define GPSDL_IOC_GPS_HW_RESUME        19
#define GPSDL_IOC_GPS_LISTEN_RST_EVT   20
#define GPSDL_IOC_GPS_GET_MD_STATUS    21
#define GPSDL_IOC_GPS_CTRL_L5_LNA      27
#define GPSDL_IOC_GPS_GET_BOOT_TIME    28
#if 0
#define GPSDL_IOC_GPS_EAP_SAP_TIMESYNC 29
#endif
#define GPSDL_IOC_GET_PLATFORM_CLOCK_FREQ   30
#define GPSDL_IOC_MNLD_STATE_NOTIFY    31

static int gps_mcudl_each_device_ioctl_inner(struct file *filp, unsigned int cmd, unsigned long arg, bool is_compat)
{
	struct gps_mcudl_each_device *dev; /* device information */
	struct boot_time_info gps_boot_time;
	unsigned long flags;
	int retval;
	unsigned int md2gps_status = 0;
	unsigned int old_bitmask, new_bitmask;

#if 0
	dev = container_of(inode->i_cdev, struct gps_mcudl_each_device, cdev);
	filp->private_data = dev; /* for other methods */
#endif
	dev = (struct gps_mcudl_each_device *)(filp->private_data);

	MDL_LOGXD(dev->index, "cmd = %d, is_compat = %d", cmd, is_compat);
#if 0
	int retval = 0;
	ENUM_WMTHWVER_TYPE_T hw_ver_sym = WMTHWVER_INVALID;
	UINT32 hw_version = 0;
	UINT32 fw_version = 0;
	UINT32 gps_lna_pin = 0;
#endif
	switch (cmd) {
	case GPSDL_IOC_TRIGGER_ASSERT:
		/* Trigger FW assert for debug */
		MDL_LOGXW_DRW(dev->index, "GPSDL_IOC_TRIGGER_ASSERT, reason = %ld", arg);

		/* TODO: assert dev->is_open */
#if 0
		if (dev->index == GPS_DATA_LINK_ID0)
			retval = gps_dl_trigger_gps_subsys_reset(false);
		else
			retval = gps_mcudl_each_link_reset(dev->index);
#endif
		retval = gps_mcudl_each_link_send_reset_evt(dev->index);
		break;
	case GPSDL_IOC_QUERY_STATUS:
		retval = gps_mcudl_each_link_check(dev->index, arg);
		/*gps_each_link_rec_force_dump(dev->index);*/
		MDL_LOGXW_DRW(dev->index, "GPSDL_IOC_QUERY_STATUS, reason = %ld, ret = %d", arg, retval);
		break;
	case GPSDL_IOC_CO_CLOCK_FLAG:
		retval = gps_dl_link_get_clock_flag();
		MDL_LOGXD_ONF(dev->index, "gps clock flag = 0x%x", retval);
		break;
#if 0
	case GPSDL_IOC_GPS_HWVER:
		/*get combo hw version */
		/* hw_ver_sym = mtk_wcn_wmt_hwver_get(); */

		GPS_DBG_FUNC("GPS_ioctl(): get hw version = %d, sizeof(hw_ver_sym) = %zd\n",
				  hw_ver_sym, sizeof(hw_ver_sym));
		if (copy_to_user((int __user *)arg, &hw_ver_sym, sizeof(hw_ver_sym)))
			retval = -EFAULT;

		break;
	case GPSDL_IOC_GPS_IC_HW_VERSION:
		/*get combo hw version from ic,  without wmt mapping */
		hw_version = mtk_wcn_wmt_ic_info_get(WMTCHIN_HWVER);

		GPS_DBG_FUNC("GPS_ioctl(): get hw version = 0x%x\n", hw_version);
		if (copy_to_user((int __user *)arg, &hw_version, sizeof(hw_version)))
			retval = -EFAULT;

		break;

	case GPSDL_IOC_GPS_IC_FW_VERSION:
		/*get combo fw version from ic, without wmt mapping */
		fw_version = mtk_wcn_wmt_ic_info_get(WMTCHIN_FWVER);

		GPS_DBG_FUNC("GPS_ioctl(): get fw version = 0x%x\n", fw_version);
		if (copy_to_user((int __user *)arg, &fw_version, sizeof(fw_version)))
			retval = -EFAULT;

		break;
	case GPSDL_IOC_RTC_FLAG:

		retval = rtc_GPS_low_power_detected();

		GPS_DBG_FUNC("low power flag (%d)\n", retval);
		break;
	case GPSDL_IOC_CO_CLOCK_FLAG:
#if SOC_CO_CLOCK_FLAG
		retval = mtk_wcn_wmt_co_clock_flag_get();
#endif
		GPS_DBG_FUNC("GPS co_clock_flag (%d)\n", retval);
		break;
	case GPSDL_IOC_D1_EFUSE_GET:
#if defined(CONFIG_MACH_MT6735)
		do {
			char *addr = ioremap(0x10206198, 0x4);

			retval = *(unsigned int *)addr;
			GPS_DBG_FUNC("D1 efuse (0x%x)\n", retval);
			iounmap(addr);
		} while (0);
#elif defined(CONFIG_MACH_MT6763)
		do {
			char *addr = ioremap(0x11f10048, 0x4);

			retval = *(unsigned int *)addr;
			GPS_DBG_FUNC("MT6763 efuse (0x%x)\n", retval);
			iounmap(addr);
		} while (0);
#else
		GPS_ERR_FUNC("Read Efuse not supported in this platform\n");
#endif
		break;

	case GPSDL_IOC_TAKE_GPS_WAKELOCK:
		GPS_INFO_FUNC("Ioctl to take gps wakelock\n");
		gps_hold_wake_lock(1);
		if (wake_lock_acquired == 1)
			retval = 0;
		else
			retval = -EAGAIN;
		break;
	case GPSDL_IOC_GIVE_GPS_WAKELOCK:
		GPS_INFO_FUNC("Ioctl to give gps wakelock\n");
		gps_hold_wake_lock(0);
		if (wake_lock_acquired == 0)
			retval = 0;
		else
			retval = -EAGAIN;
		break;
#ifdef GPS_FWCTL_SUPPORT
	case GPSDL_IOC_GPS_FWCTL:
		GPS_INFO_FUNC("GPSDL_IOC_GPS_FWCTL\n");
		retval = GPS_fwctl((struct gps_fwctl_data *)arg);
		break;
#endif
	case GPSDL_IOC_GET_GPS_LNA_PIN:
		gps_lna_pin = mtk_wmt_get_gps_lna_pin_num();
		GPS_DBG_FUNC("GPS_ioctl(): get gps lna pin = %d\n", gps_lna_pin);
		if (copy_to_user((int __user *)arg, &gps_lna_pin, sizeof(gps_lna_pin)))
			retval = -EFAULT;
		break;
#endif
	case GPSDL_IOC_GPS_HW_SUSPEND:
		/* arg == 1 stand for need clk extension, otherwise is normal deep sotp mode */
		retval = gps_mcudl_each_device_hw_suspend(dev->index, (arg == 1));
		MDL_LOGXI_ONF(dev->index,
			"GPSDL_IOC_GPS_HW_SUSPEND: arg = %ld, ret = %d", arg, retval);
		break;
	case GPSDL_IOC_GPS_HW_RESUME:
		retval = gps_mcudl_each_device_hw_resume(dev->index);
		MDL_LOGXI_ONF(dev->index,
			"GPSDL_IOC_GPS_HW_RESUME: arg = %ld, ret = %d", arg, retval);
		break;
	case GPSDL_IOC_GPS_LISTEN_RST_EVT:
		retval = gps_mcudl_each_link_listen_state_ntf(dev->index);
		MDL_LOGXD_ONF(dev->index,
			"GPSDL_IOC_GPS_LISTEN_RST_EVT retval = %d", retval);
		break;
	case 21505:
	case 21506:
	case 21515:
		/* known unsupported cmd */
		retval = -EFAULT;
		MDL_LOGXD_DRW(dev->index, "cmd = %d, not support", cmd);
		break;
	case GPSDL_IOC_GPS_GET_MD_STATUS:
			retval = 0;
			if (b13_gps_status_addr != 0) {
				do {
					char *addr = ioremap((phys_addr_t)b13_gps_status_addr, 0x4);

					md2gps_status = *(unsigned int *)addr;
					GDL_LOGXI_ONF(dev->index, "MD2GPS_REG (0x%x), md2gps_status=0x%x\n",
						b13_gps_status_addr, md2gps_status);
					if (copy_to_user((int __user *)arg, &md2gps_status, sizeof(md2gps_status)))
						retval = -EFAULT;
					iounmap(addr);
				} while (0);
			} else {
				retval = -EFAULT;
				GDL_LOGXE_ONF(dev->index, "Can't get MD2GPS_REG in this platform\n");
			}
			break;
	case GPSDL_IOC_GPS_GET_BOOT_TIME:
		if (arg == 0)
			retval = -EINVAL;
		else
			retval = 0;
		local_irq_save(flags);
		#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
		gps_boot_time.now_time = ktime_get_boottime_ns();
		gps_boot_time.arch_counter = __arch_counter_get_cntvct();
		#else
		gps_boot_time.now_time = ktime_get_boot_ns();
		gps_boot_time.arch_counter = arch_counter_get_cntvct();
		#endif
		local_irq_restore(flags);
		if (copy_to_user((char __user *)arg, &gps_boot_time, sizeof(struct boot_time_info))) {
			GDL_LOGXI_ONF(dev->index,
			"GPSDL_IOC_GPS_GET_BOOT_TIME: copy_to_user error");
			retval = -EFAULT;
		}
		GDL_LOGXI_ONF(dev->index,
			"GPSDL_IOC_GPS_GET_BOOT_TIME now_time = %lld,arch_counter = %lld",
			gps_boot_time.now_time, gps_boot_time.arch_counter);
		break;
#if GPS_DL_GET_PLATFORM_CLOCK_FREQ
		case GPSDL_IOC_GET_PLATFORM_CLOCK_FREQ:
			retval = gps_dl_clock_mng_get_platform_clock();
			break;
#endif

	case GPSDL_IOC_MNLD_STATE_NOTIFY:
		old_bitmask = gps_mcudl_get_opp_vote_phase_bitmask();
		retval = 0;
		if (arg == 0) {
			/* MNLD: STOPPING to IDLE/SUSPEND_DONE */
			gps_mcudl_set_opp_vote_phase(GPS_MNLD_FSM_STOPPING, false);
		} else if (arg == 1) {
			/* MNLD: IDLE/SUSPEND_DONE to STARTING */
			gps_mcudl_set_opp_vote_phase(GPS_MNLD_FSM_STARTING, true);
		} else if (arg == 2) {
			/* MNLD: STARTING to STARTED */
			gps_mcudl_set_opp_vote_phase(GPS_MNLD_FSM_STARTING, false);
			gps_mcudl_set_opp_vote_phase(GPS_DSP_NOT_WORKING, false);
		} else if (arg == 3) {
			/* MNLD: STARTED to STOPPING */
			gps_mcudl_set_opp_vote_phase(GPS_DSP_NOT_WORKING, true);
			gps_mcudl_set_opp_vote_phase(GPS_MNLD_FSM_STOPPING, true);
		} else if (arg == 10) {
			/* MNLD: After opening LPPM dev node */
			gps_mcudl_set_opp_vote_phase(GPS_MCU_OPENING, false);
		} else if (arg == 11) {
			/* MNLD: Before closing LPPM dev node
			 * MNLD: SUSPEND_DONE to IDLE
			 */
			gps_mcudl_set_opp_vote_phase(GPS_MNLD_LPPM_CLOSING, true);
		} else {
			retval = -EFAULT;
		}
		new_bitmask = gps_mcudl_get_opp_vote_phase_bitmask();
		MDL_LOGXI(dev->index, "cmd=%d, arg=%lu, vote_bitmask=0x%04x,0x%04x",
			cmd, arg, old_bitmask, new_bitmask);
		break;

	default:
		retval = -EFAULT;
		MDL_LOGXI_DRW(dev->index, "cmd = %d, not support", cmd);
		break;
	}

	return retval;
}


static long gps_mcudl_each_device_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	return gps_mcudl_each_device_ioctl_inner(filp, cmd, arg, false);
}

static long gps_mcudl_each_device_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	return gps_mcudl_each_device_ioctl_inner(filp, cmd, arg, true);
}

static const struct file_operations gps_mcudl_each_device_fops = {
	.owner = THIS_MODULE,
	.open = gps_mcudl_each_device_open,
	.poll = gps_mcudl_each_device_poll,
	.read = gps_mcudl_each_device_read,
	.write = gps_mcudl_each_device_write,
	.release = gps_mcudl_each_device_release,
	.unlocked_ioctl = gps_mcudl_each_device_unlocked_ioctl,
	.compat_ioctl = gps_mcudl_each_device_compat_ioctl,
};

int gps_mcudl_cdev_setup(struct gps_mcudl_each_device *dev, enum gps_mcudl_xid xid)
{
	int result;

	init_waitqueue_head(&dev->r_wq);
	dev->i_len = 0;

	dev->index = xid;
	/* assert dev->xid == dev->cfg.xid */

	cdev_init(&dev->cdev, &gps_mcudl_each_device_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &gps_mcudl_each_device_fops;

	result = cdev_add(&dev->cdev, dev->devno, 1);
	if (result) {
		MDL_LOGE("cdev_add error %d on xid %d", result, xid);
		return result;
	}

	MDL_LOGD("class_create: %s, xid = %d", dev->cfg.dev_name, dev->cfg.xid);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
	dev->cls = class_create(dev->cfg.dev_name);
#else
	dev->cls = class_create(THIS_MODULE, dev->cfg.dev_name);
#endif
	if (IS_ERR(dev->cls)) {
		MDL_LOGE("class_create fail on %s", dev->cfg.dev_name);
		return -1;
	}

	dev->dev = device_create(dev->cls, NULL, dev->devno, NULL, dev->cfg.dev_name);
	if (IS_ERR(dev->dev)) {
		MDL_LOGE("device_create fail on %s", dev->cfg.dev_name);
		return -1;
	}

	return 0;
}

void gps_mcudl_cdev_cleanup(struct gps_mcudl_each_device *dev, enum gps_mcudl_xid xid)
{
	if (dev->dev) {
		device_destroy(dev->cls, dev->devno);
		dev->dev = NULL;
	}

	if (dev->cls) {
		class_destroy(dev->cls);
		dev->cls = NULL;
	}

	cdev_del(&dev->cdev);
}

