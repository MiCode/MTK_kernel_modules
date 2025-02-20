/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2018 MediaTek Inc.
 */
#ifndef __MTK_IMGSYS_SLC_H__
#define __MTK_IMGSYS_SLC_H__

enum SLC_SB_R {
	SLC_SB_RNC = 0,
	SLC_SB_RNA,
	SLC_SB_RD,
	SLC_SB_RI,
};

enum SLC_SB_W {
	SLC_SB_WNA = 0,
	SLC_SB_WA,
};

#ifdef __cplusplus
struct slc_info {
	uint8_t slc_sb = 0;
	uint8_t gid = 0;
	uint8_t bid = 0;
	uint8_t read_count = 0;
};
#else
struct slc_info {
	uint8_t slc_sb;
	uint8_t gid;
	uint8_t bid;
	uint8_t read_count;
};
#endif

#endif /* __MTK_IMGSYS_SLC_H__ */
