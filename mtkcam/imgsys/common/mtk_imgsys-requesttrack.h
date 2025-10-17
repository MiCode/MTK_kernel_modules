/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#ifndef IMGSYS_MTK_IMGSYS_REQUESTTRACK_H_
#define IMGSYS_MTK_IMGSYS_REQUESTTRACK_H_

#ifndef __KERNEL__
#include <stdint.h>
#endif

#include <linux/types.h>

/******************************************************************************
 *
 *        ---------------------------------------------------------------> Time
 * USER      (+)                                                      (-)
 * IMGSTREAM   \(-)....(+)      (-)..(+)                     (-)...(+)/
 * DAEMON                \      /      \(-)....(+)           /
 * KERNEL                (-)..(+)                \(-)....(+)/
 *
 * (+)--(-)  main flow
 * ........  sub flow
 *
 * MSB <---------------------------- 64 bits ------------------------------> LSB
 * |8 bits|   8 bits  |   8 bits  |    8 bits |   8 bits  |   8 bits  | 16 bits|
 * | valid| main flow | main flow |  sub flow | sub flow  | sub flow  |reserved|
 * |   1  |   (FROM)  |    (TO)   |(IMGSTREAM)|  (DAEMON) |  (KERNEL) |        |
 *
 * @param valid          valid if 1; invalid if 0.
 * @param mainflow_from  Updated at every (+)
 * @param mainflow_to    Updated at every (-)
 *
 ******************************************************************************/
union request_track {
	__u64     u64;
	struct {
		__u8    reserved[2];
		__u8    subflow_kernel;
		__u8    subflow_daemon;
		__u8    subflow_imgstream;
		__u8    mainflow_to;
		__u8    mainflow_from;
		__u8    valid;
	};
};

#ifdef __cplusplus
static_assert(sizeof(request_track) == 8);
#endif

enum {
	REQUEST_FROM_USER_TO_IMGSTREAM          = 0,
	REQUEST_FROM_IMGSTREAM_TO_KERNEL        = 1,
	REQUEST_FROM_KERNEL_TO_IMGSTREAM        = 2,
	REQUEST_FROM_IMGSTREAM_TO_DAEMON        = 3,
	REQUEST_FROM_DAEMON_TO_KERNEL           = 4,
	REQUEST_DONE_FROM_KERNEL_TO_IMGSTREAM   = 5,
	REQUEST_DONE_FROM_IMGSTREAM_TO_USER     = 6,
};

#endif  // IMGSYS_MTK_IMGSYS_REQUESTTRACK_H_
