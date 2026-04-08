// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */
#include "isp_pspm_ioctl.h"

#define TAG "ISP_PSPM_IOCTL"

static u32 log_level;
module_param(log_level, uint, 0644);
MODULE_PARM_DESC(log_level, "enable isp pspm debug log when level >=1");


struct proc_dir_entry *isp_pspm_ioctl_root;
EXPORT_SYMBOL(isp_pspm_ioctl_root);

static int device_show(struct seq_file *m, void *v)
{
	return 0;
}

static int device_open(struct inode *inode, struct file *file)
{
	return single_open(file, device_show, inode->i_private);
}

static long device_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	ssize_t ret = 0;
	// void __user *argp = (void __user *)arg;
#ifdef ISP_CSI_PSPM_SUPPORT
	struct ISP_P2 idx;

	switch (cmd) {
#if IS_ENABLED(CONFIG_MTK_ISP_PSPM)
	case ISP_PSPM_SETPARAM:
	if (copy_from_user(&idx, (__user struct ISP_PSPM_P2 *)(arg),
			sizeof(idx))) {
		ret = -EFAULT;
		goto ret_ioctl;
	}
	isp_log_detail(log_level, "data(%d) fps(%d), ds_type(%d), eis_type(%d), vr_data(%d)",
		idx.data, idx.fps, idx.ds_type, idx.eis_type, idx.vr_data);
	isp_log_detail(log_level, "disp_fps(%d), disp_ds_type(%d), disp_eis_type(%d), disp_data(%d)",
		idx.disp_fps, idx.disp_ds_type, idx.disp_eis_type, idx.disp_data);

	set_p2_idx(idx);
	goto ret_ioctl;
#else
	case ISP_PSPM_SETPARAM:
		[[fallthrough]];
	break;
#endif

	default:
		isp_log_basic(" %s %d: unknown cmd %x\n",
			__FILE__, __LINE__, cmd);
		ret = -EINVAL;
		goto ret_ioctl;
	}

ret_ioctl:
	return ret;
#else
	return ret;
#endif
}

static const struct proc_ops Fops = {
	.proc_compat_ioctl = device_ioctl,
	.proc_ioctl = device_ioctl,
	.proc_open = device_open,
	.proc_release = single_release,
};

static void __exit exit_isp_pspm_ioctl(void) {}
static int __init init_isp_pspm_ioctl(void)
{
	struct proc_dir_entry *pe, *parent;

	isp_log_basic("Start to init isp_pspm_ioctl driver\n");

	parent = proc_mkdir("isp_pspm", NULL);
	isp_pspm_ioctl_root = parent;

	pe = proc_create("isp_pspm_ioctl", 0660, parent, &Fops);
	if (!pe) {
		isp_log_basic("failed to create isp_pspm_ioctl");
		return -ENOMEM;
	}

	isp_log_basic("init isp_pspm_ioctl driver done\n");

	return 0;
}

module_init(init_isp_pspm_ioctl);
module_exit(exit_isp_pspm_ioctl);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MediaTek isp_pspm_ioctl");
MODULE_AUTHOR("MediaTek Inc.");
