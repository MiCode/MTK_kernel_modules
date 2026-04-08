/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */
#ifndef _ISP_PSPM_IOCTL_H_
#define _ISP_PSPM_IOCTL_H_

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/uaccess.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/ioctl.h>
#ifdef ISP_CSI_PSPM_SUPPORT
#include "swpm_isp_wrapper.h"
#endif

#define MyTag "[ISP_PSPM_IOCTL]"
#define isp_log_basic(fmt, args...) pr_info(MyTag "[%s] " fmt "\n", __func__, ##args)
#define isp_log_detail(level, fmt, args...)                         \
	do {                                                            \
		if (level >= 1)                                             \
			pr_info(MyTag "[%s] " fmt "\n", __func__, ##args);  \
	} while (0)

struct ISP_PSPM_P2 {
	unsigned int data; /* current frame data size */
	unsigned int fps;
	unsigned int ds_type;
	unsigned int eis_type;
	unsigned int vr_data; /* vr data size for pqdip */
	unsigned int disp_fps;
	unsigned int disp_ds_type;
	unsigned int disp_eis_type;
	unsigned int disp_data; /* disp data size for pqdip */
} ISP_PSPM_P2;

#define ISP_PSPM_IOCTL_MGAIC 'I'
#define ISP_PSPM_SETPARAM _IOW(ISP_PSPM_IOCTL_MGAIC, 5, struct ISP_PSPM_P2)

#endif  // _ISP_PSPM_IOCTL_H_



