/*
 * Copyright (C) 2019 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#ifndef __MET_GPU_MONITOR_H__
#define __MET_GPU_MONITOR_H__

#define	MET_GPU_STALL_MONITOR
#define IO_ADDR_GPU_STALL		0x1021c000
#define IO_SIZE_GPU_STALL		0x1000
#define OFFSET_STALL_GPU_M0_CHECK	0x200
#define OFFSET_STALL_GPU_M1_CHECK	0x204
#define OFFSET_STALL_GPU_M0_EMI_CHECK	0x208
#define OFFSET_STALL_GPU_M1_EMI_CHECK	0x20c

#endif	/* __MET_GPU_MONITOR_H__ */
