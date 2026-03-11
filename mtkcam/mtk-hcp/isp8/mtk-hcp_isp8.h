/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef MTK_HCP_ISP8_H
#define MTK_HCP_ISP8_H

#include "mtk-hcp.h"

//#define IMG_MODULE_SET 7

#if SMVR_DECOUPLE
int isp8_release_working_buffer(struct mtk_hcp *hcp_dev, unsigned int mode);
int isp8_release_gce_working_buffer(struct mtk_hcp *hcp_dev);
int isp8_allocate_working_buffer(struct mtk_hcp *hcp_dev, unsigned int mode, unsigned int gmb_en);
//int isp7s_allocate_working_buffer2(struct mtk_hcp *hcp_dev, unsigned int mode);
int isp8_get_init_info(struct img_init_info *info);
void *isp8_get_gce_virt(unsigned int mode);
void *isp8_get_gce_token_virt(unsigned int mode);
void *isp8_get_hwid_virt(unsigned int mode);
int isp8_partial_flush(struct mtk_hcp *hcp_dev, struct flush_buf_info *b_info);
#else
int isp8_release_working_buffer(struct mtk_hcp *hcp_dev);
int isp8_allocate_working_buffer(struct mtk_hcp *hcp_dev, unsigned int mode);
int isp8_get_init_info(struct img_init_info *info);
void *isp8_get_gce_virt(void);
void *isp8_get_hwid_virt(void);
int isp8_partial_flush(struct mtk_hcp *hcp_dev, struct flush_buf_info *b_info);
#endif

extern struct mtk_hcp_data isp8_hcp_data;

#endif /* _MTK_HCP_ISP7sp_H */
