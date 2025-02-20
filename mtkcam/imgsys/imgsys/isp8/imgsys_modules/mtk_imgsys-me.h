/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 *
 * Author: Marvin Lin <Marvin.Lin@mediatek.com>
 *
 */

#ifndef _MTK_IMGSYS_ME_H_
#define _MTK_IMGSYS_ME_H_

#include "./../mtk_imgsys-dev.h"
#include "./../mtk_imgsys-debug.h"

#define ME_BASE 0x34070000

#define ME_CTL_OFFSET      0x0000
#define ME_CTL_RANGE       0xE50
#define ME_CTL_RANGE_TF    0x140

#define MMG_CTL_OFFSET      0x0000
#define MMG_CTL_RANGE       0xA50
#define MMG_CTL_RANGE_TF    0x40

// align userspace
#define ME_CQ_DESC_NUM 117
#define ME_REG_SIZE 0x2000


void imgsys_me_set_initial_value(struct mtk_imgsys_dev *imgsys_dev);
void imgsys_me_debug_dump(struct mtk_imgsys_dev *imgsys_dev,
							unsigned int engine);
void imgsys_me_uninit(struct mtk_imgsys_dev *imgsys_dev);
void imgsys_me_set_hw_initial_value(struct mtk_imgsys_dev *imgsys_dev);
int ME_TranslationFault_callback(int port, dma_addr_t mva, void *data);
int MMG_TranslationFault_callback(int port, dma_addr_t mva, void *data);
bool imgsys_me_done_chk(struct mtk_imgsys_dev *imgsys_dev, uint32_t engine);
void imgsys_me_updatecq(struct mtk_imgsys_dev *imgsys_dev,
			struct img_swfrm_info *user_info, int req_fd, u64 tuning_iova,
			unsigned int mode);

#endif /* _MTK_IMGSYS_ME_H_ */
