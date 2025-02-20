/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 *
 * Author: Floria Huang <floria.huang@mediatek.com>
 *
 */

#ifndef _MTK_IMGSYS_WPE_H_
#define _MTK_IMGSYS_WPE_H_

#include "mtk_imgsys-dev.h"
#include "mtk_imgsys-cmdq.h"
#include "./../mtk_imgsys-engine.h"
#include "./../mtk_imgsys-debug.h"

/********************************************************************
 * Global Define
 ********************************************************************/

#define WPE_UFOD_P2_DESC_OFST 26 // align with userspace
#define WPE_CQ_DESC_NUM	30 // align with userspace
#define WPE_REG_SIZE 4096  // align with userspace
#define WPE_TDR_BUF_MAXSZ 16384 // align with userspace
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Public Functions
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void imgsys_wpe_set_initial_value(struct mtk_imgsys_dev *imgsys_dev);
void imgsys_wpe_set_hw_initial_value(struct mtk_imgsys_dev *imgsys_dev);
void imgsys_wpe_debug_dump(struct mtk_imgsys_dev *imgsys_dev,
							unsigned int engine);
void imgsys_wpe_uninit(struct mtk_imgsys_dev *imgsys_dev);
void imgsys_wpe_updatecq(struct mtk_imgsys_dev *imgsys_dev,
			struct img_swfrm_info *user_info, int req_fd, u64 tuning_iova,
			unsigned int mode);

#endif /* _MTK_IMGSYS_WPE_H_ */
