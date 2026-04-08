/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (c) 2018 MediaTek Inc.
 *
 */
#ifndef MTK_IMGSYS_FRM_SYNC_ISP8_H
#define MTK_IMGSYS_FRM_SYNC_ISP8_H
#include "mtk_imgsys_frm_sync.h"
//#include "mt6989-gce.h"

#define MAX_GCE_NORRING_DPE   16
#define MAX_GCE_NORRING_IMGSYS   153

#define event_group(event)	(((event) >> 16) & 0xFFFF)

extern struct mtk_img_frm_sync_data mtk_img_frm_sync_data_isp8;

int dpe_event_init_isp8(struct mtk_img_frm_sync *mtk_img_frm_sync_dev);
int mtk_img_frm_sync_init_isp8(struct mtk_img_frm_sync *mtk_img_frm_sync_dev);
int mtk_img_frm_sync_uninit_isp8(struct mtk_img_frm_sync *mtk_img_frm_sync_dev);
int init_isp8(struct mtk_img_frm_sync *mtk_img_frm_sync_dev, struct group k_group);
int uninit_isp8(struct mtk_img_frm_sync *mtk_img_frm_sync_dev, struct group k_group);
int Handler_frame_token_sync_imgsys_isp8(struct mtk_img_frm_sync *mtk_img_frm_sync_dev,
		struct imgsys_in_data *in_data, struct imgsys_out_data *out_data);
int Handler_frame_token_sync_DPE_isp8(struct mtk_img_frm_sync *mtk_img_frm_sync_dev,
		struct dpe_in_data *in_data, struct dpe_out_data *out_data);
int Handler_frame_token_sync_MAE_isp8(struct mtk_img_frm_sync *mtk_img_frm_sync_dev,
		struct mae_in_data *in_data, struct mae_out_data *out_data);
int release_frame_token_imgsys_isp8(struct mtk_img_frm_sync *mtk_img_frm_sync_dev,
		struct imgsys_deque_done_in_data *in_data);
int release_frame_token_DPE_isp8(struct mtk_img_frm_sync *mtk_img_frm_sync_dev,
		struct dpe_deque_done_in_data *in_data);
int release_frame_token_MAE_isp8(struct mtk_img_frm_sync *mtk_img_frm_sync_dev,
		struct mae_deque_done_in_data *in_data);
int clear_token_user_isp8(struct mtk_img_frm_sync *mtk_img_frm_sync_dev,
		unsigned long frm_owner, unsigned long imgstm_inst);
int vsdof_frm_sync_timeout_isp8(struct mtk_img_frm_sync *mtk_img_frm_sync_dev,
		int gce_event_id);

#endif /* MTK_IMGSYS_FRM_SYNC_ISP8_H */
