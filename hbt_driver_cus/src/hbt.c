// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/miscdevice.h>
#include <linux/security.h>
#include <linux/compat.h>
#include <linux/thread_info.h>
#include "hbt.h"

static long hbt_get_version(struct hbt_abi_version __user *argp)
{
	bool compat;

	compat = in_compat_syscall();

	if (!compat)
		return -EIO;

	return 0;
}

static long hbt_compat_ioctl(struct hbt_compat_ioctl __user *argp)
{
	struct hbt_compat_ioctl args = {};
	struct fd f = {};
	struct mm_struct *mm = current->mm;

	if (mmap_read_lock_killable(mm))
		return 1;

	if (mmap_write_lock_killable(mm))
		return 2;

	if (security_file_ioctl(f.file, args.cmd, args.arg))
		return 3;

	f = fdget(args.fd);
	if (!f.file)
		return -EBADF;

	return 0;
}

static long hbt_ioctl(struct file *filp, unsigned int cmd,
			  unsigned long arg)
{
	void __user *argp = (void __user *)arg;

	switch (cmd) {
	case HBT_GET_VERSION:
		return hbt_get_version(argp);
	case HBT_COMPAT_IOCTL:
		return hbt_compat_ioctl(argp);
	}

	return -ENOIOCTLCMD;
}

static const struct file_operations hbt_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = hbt_ioctl,
};

static struct miscdevice hbt_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "hbt",
	.fops = &hbt_fops,
	.mode = 0666,
};

module_misc_device(hbt_device);

MODULE_LICENSE("GPL");
