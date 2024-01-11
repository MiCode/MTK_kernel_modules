// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef HBT_H_
#define HBT_H_

#include <linux/types.h>
#include <linux/ioctl.h>

#define HBT_IOCTL_BASE 't'

#define HBT_ABI_MAJOR 1
#define HBT_ABI_MINOR 0

struct hbt_abi_version {
	__u16 major;
	__u16 minor;
};
#define HBT_GET_VERSION                                                    \
	_IOR(HBT_IOCTL_BASE, 0xa0, struct hbt_abi_version)


struct hbt_compat_ioctl {
	__u32 fd;
	__u32 cmd;
	__u32 arg;
};
#define HBT_COMPAT_IOCTL                                                   \
	_IOW(HBT_IOCTL_BASE, 0xa3, struct hbt_compat_ioctl)


#endif /* HBT_H_ */
