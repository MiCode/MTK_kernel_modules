/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#ifndef __MTK_CAM_BIT_MAPPING_H
#define __MTK_CAM_BIT_MAPPING_H

#include <linux/bitops.h>

#include "mtk_cam-defs.h"

#define BIT_MAP(offset, cnt)	(cnt << 16 | offset)
#define BIT_MAP_OFFSET(m)	(m & ((1u << 16) - 1))
#define BIT_MAP_CNT(m)		((unsigned int)m >> 16)

static inline
unsigned long bit_map_subset_of(unsigned int map, unsigned long set)
{
	return (set >> BIT_MAP_OFFSET(map)) & ((1u << BIT_MAP_CNT(map)) - 1);
}

static inline
unsigned long bit_map_subset_mask(unsigned int map)
{
	return ((1u << BIT_MAP_CNT(map)) - 1);
}

static inline
unsigned long bit_map_bit(unsigned int map, long idx)
{
	return BIT(idx + BIT_MAP_OFFSET(map));
}

static inline bool is_mask_containing(unsigned long a, unsigned long b)
{
	return (a & b) == b;
}

static inline int find_first_bit_set(unsigned long set)
{
	return ffs(set) - 1;
}

/* subdev mappings */
#define MAP_SUBDEV_RAW		BIT_MAP(MTKCAM_SUBDEV_RAW_START, \
					MTKCAM_SUBDEV_RAW_NUM)
#define MAP_SUBDEV_CAMSV	BIT_MAP(MTKCAM_SUBDEV_CAMSV_START, \
					MTKCAM_SUBDEV_CAMSV_NUM)
#define MAP_SUBDEV_MRAW		BIT_MAP(MTKCAM_SUBDEV_MRAW_START, \
					MTKCAM_SUBDEV_MRAW_NUM)
static inline unsigned long ipi_pipe_id_to_bit(int ipi_pipe_id)
{
	return BIT(ipi_pipe_id);
}

/* stream */
#define MAP_STREAM		BIT_MAP(0, 8)

/* hw */
//#define MAP_HW_RAW		BIT_MAP(0, 3)
//#define MAP_HW_CAMSV		BIT_MAP(4, 16)
//#define MAP_HW_MRAW		BIT_MAP(20, 3)
#define MAP_HW_RAW		BIT_MAP(MTKCAM_SUBDEV_RAW_START, \
					MTKCAM_SUBDEV_RAW_NUM)
#define MAP_HW_CAMSV		BIT_MAP(MTKCAM_SUBDEV_CAMSV_START, \
					MTKCAM_SUBDEV_CAMSV_NUM)
#define MAP_HW_MRAW		BIT_MAP(MTKCAM_SUBDEV_MRAW_START, \
					MTKCAM_SUBDEV_MRAW_NUM)

#endif //__MTK_CAM_BIT_MAPPING_H
