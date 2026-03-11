/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 *
 * Author: Yuan-Jung Kuo <yuan-jung.kuo@mediatek.com>
 *          Nick.wen <nick.wen@mediatek.com>
 *
 */

#include "mtk_imgsys-dev.h"
#include "mtk_imgsys-cmdq.h"


/**
 * @brief Record every mtcmos status
 *        such as number of users who are
 *        using this mtcmos power.
 */
struct imgsys_cg_data {
	u32 clr_ofs;
	u32 sta_ofs;
};

struct qof_state {
	u32 user_count;
};

struct qof_events {
	u16 power_ctrl;
	u16 trig_pwr_on;
	u16 trig_pwr_off;
	u16 pwr_off;
	u16 pwr_hand_shake;
};

struct qof_reg_data {
	unsigned int ofset;
	unsigned int val;
};

struct qof_larb_info {
	unsigned int reg_ba;
	unsigned int reg_list_size;
	struct qof_reg_data *larb_reg_list;
};

struct imgsys_mtcmos_data {
	u32 module_list;
	u32 vote_on_ofs;
	u32 vote_off_ofs;
	u32 hwv_done_ofs;
	u32 hwv_shift;
	u32 hwv_gce_ofst;
	/* Related clocks in power domain */
	void (*cg_ungating)(struct cmdq_pkt *pkt, const struct imgsys_cg_data *cg, dma_addr_t qof_work_buf_pa);
	const struct imgsys_cg_data *cg_data;
	void (*set_larb_golden)(struct cmdq_pkt *pkt);
	void (*direct_link_reset)(struct cmdq_pkt *pkt);
};

void mtk_imgsys_cmdq_qof_init(struct mtk_imgsys_dev *imgsys_dev, struct cmdq_client *imgsys_clt);
void mtk_imgsys_cmdq_qof_release(struct mtk_imgsys_dev *imgsys_dev, struct cmdq_client *imgsys_clt);
void mtk_imgsys_cmdq_qof_streamon(struct mtk_imgsys_dev *imgsys_dev);
void mtk_imgsys_cmdq_qof_streamoff(struct mtk_imgsys_dev *imgsys_dev);
void mtk_imgsys_cmdq_qof_add(struct cmdq_pkt *pkt, u32 hwcomb, bool *qof_need_sub);
void mtk_imgsys_cmdq_qof_sub(struct cmdq_pkt *pkt, bool *qof_need_sub);
bool mtk_imgsys_cmdq_qof_get_pwr_status(u32 pwr);
void mtk_imgsys_cmdq_qof_dump(uint32_t hwcomb, bool need_dump_vcp);
