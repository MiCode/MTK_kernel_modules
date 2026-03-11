/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 *
 * Author: Floria Huang <floria.huang@mediatek.com>
 *
 */

#ifndef _MTK_IMGSYS_WPE_H_
#define _MTK_IMGSYS_WPE_H_


#include <mtk_imgsys-cmdq.h>
#include "./../mtk_imgsys-engine.h"
#include "./../mtk_imgsys-debug.h"
#include "./../mtk_imgsys-dev.h"

/********************************************************************
 * Global Define
 ********************************************************************/

#define WPE_UFOD_P2_DESC_OFST 25 // align with userspace
#define WPE_CQ_DESC_NUM	31 // align with userspace
#define WPE_REG_SIZE 4096  // align with userspace
#define WPE_TDR_BUF_MAXSZ 9216 // align with userspace
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Public Functions
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void imgsys_wpe_set_initial_value(struct mtk_imgsys_dev *imgsys_dev);
void imgsys_wpe_set_hw_initial_value(struct mtk_imgsys_dev *imgsys_dev);
void imgsys_wpe_cmdq_set_hw_initial_value(struct mtk_imgsys_dev *imgsys_dev,
		void *pkt, int hw_idx);
void imgsys_wpe_debug_dump(struct mtk_imgsys_dev *imgsys_dev,
							unsigned int engine);
void imgsys_wpe_uninit(struct mtk_imgsys_dev *imgsys_dev);
void imgsys_wpe_updatecq(struct mtk_imgsys_dev *imgsys_dev,
			struct img_swfrm_info *user_info, int req_fd, u64 tuning_iova,
			unsigned int mode);
int imgsys_wpe_tfault_callback(int port,
			dma_addr_t mva, void *data);
bool imgsys_wpe_done_chk(struct mtk_imgsys_dev *imgsys_dev, uint32_t engine);

#endif /* _MTK_IMGSYS_WPE_H_ */
