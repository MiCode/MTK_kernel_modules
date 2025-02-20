/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 *
 * Author: Daniel Huang <daniel.huang@mediatek.com>
 *
 */

#ifndef _MTK_IMGSYS_PQDIP_H_
#define _MTK_IMGSYS_PQDIP_H_

// Standard C header file

// kernel header file

// mtk imgsys local header file
#include <mtk_imgsys-dev.h>
#include <mtk_imgsys-cmdq.h>

// Local header file
#include "./../mtk_imgsys-engine.h"
#include "./../mtk_imgsys-debug.h"

/********************************************************************
 * Global Define
 ********************************************************************/
#define PQDIP_CQ_DESC_NUM	36 // align with userspace
#define PQDIP_REG_SIZE  (0x6000) // align with userspace
#define PQDIP_TDR_BUF_MAXSZ 40960 // align with userspace

/********************************************************************
 * Enum Define
 ********************************************************************/


/********************************************************************
 * Structure Define
 ********************************************************************/

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Public Functions
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void imgsys_pqdip_set_initial_value(struct mtk_imgsys_dev *imgsys_dev);
void imgsys_pqdip_set_hw_initial_value(struct mtk_imgsys_dev *imgsys_dev);
void imgsys_pqdip_debug_dump(struct mtk_imgsys_dev *imgsys_dev,
							unsigned int engine);
void imgsys_pqdip_uninit(struct mtk_imgsys_dev *imgsys_dev);
void imgsys_pqdip_updatecq(struct mtk_imgsys_dev *imgsys_dev,
			struct img_swfrm_info *user_info, int req_fd, u64 tuning_iova,
			unsigned int mode);

#endif /* _MTK_IMGSYS_PQDIP_H_ */
