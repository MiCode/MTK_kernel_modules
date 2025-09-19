/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2018 MediaTek Inc.
 *
 * Author: Daniel Huang <daniel.huang@mediatek.com>
 *
 */

#ifndef _MTK_IMGSYS_CMDQ_TOKEN_8_H_
#define _MTK_IMGSYS_CMDQ_TOKEN_8_H_
#include <linux/platform_device.h>
#include <linux/soc/mediatek/mtk-cmdq-ext.h>

struct token_data {
	uint64_t frm_owner;
	uint32_t req_fd;
	uint32_t req_no;
	int sw_ridx;
};

unsigned int imgsys_cmdq_try_vsdof_wfe(struct token_data *data, struct cmdq_pkt *pkt,
										uint32_t event);
unsigned int imgsys_cmdq_try_vsdof_wfe_no_clear(struct token_data *data, struct cmdq_pkt *pkt,
										uint32_t event);
unsigned int imgsys_cmdq_try_vsdof_set_event(struct token_data *data, struct cmdq_pkt *pkt,
										uint32_t event);
unsigned int imgsys_cmdq_try_vsdof_clear_event(struct token_data *data, struct cmdq_pkt *pkt,
										uint32_t event);

unsigned int imgsys_cmdq_release_token_vsdof(int sw_ridx);

unsigned int imgsys_cmdq_frm_sync_init(void);

unsigned int imgsys_cmdq_frm_sync_uninit(void);

unsigned int imgsys_cmdq_frm_sync_dump_event_info(int event);

unsigned int imgsys_cmdq_is_vsdof_event(uint32_t event);

struct platform_device *imgsys_cmdq_set_frm_sync_pdev(struct device *dev);

#endif /* _MTK_IMGSYS_CMDQ_TOKEN_8_H_ */
