/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2018 MediaTek Inc.
 *
 * Author: Daniel Huang <daniel.huang@mediatek.com>
 *
 */

#ifndef _MTK_IMGSYS_CMDQ_DVFS_8_H_
#define _MTK_IMGSYS_CMDQ_DVFS_8_H_

#define IMGSYS_DVFS_ENABLE     (1)

#define IMGSYS_DVFS_MAX_VOLT	700000
#define IMGSYS_DVFS_MHz			1000000
#define IMGSYS_DVFS_RATIO_L		4
#define IMGSYS_DVFS_RATIO_H		6

#if DVFS_QOS_READY
void mtk_imgsys_mmdvfs_init_plat8(struct mtk_imgsys_dev *imgsys_dev);
void mtk_imgsys_mmdvfs_uninit_plat8(struct mtk_imgsys_dev *imgsys_dev);
void mtk_imgsys_mmdvfs_set_plat8(struct mtk_imgsys_dev *imgsys_dev,
				struct swfrm_info_t *frm_info,
				bool isSet);
void mtk_imgsys_mmdvfs_reset_plat8(struct mtk_imgsys_dev *imgsys_dev);
void mtk_imgsys_mmdvfs_mmqos_cal_plat8(struct mtk_imgsys_dev *imgsys_dev,
				struct swfrm_info_t *frm_info,
				bool isSet);
#endif


#endif /* _MTK_IMGSYS_CMDQ_DVFS_8_H_ */
