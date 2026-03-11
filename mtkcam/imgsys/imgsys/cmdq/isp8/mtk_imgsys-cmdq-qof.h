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
 *		such as number of users who are
 *		using this mtcmos power.
 */
struct qof_reg_data {
	unsigned int ofset;
	unsigned int val;
};

struct qof_larb_info {
	unsigned int reg_ba;
	unsigned int reg_list_size;
	struct qof_reg_data *larb_reg_list;
};

struct reg_table_unit {
	unsigned int addr;
	unsigned int val;
	unsigned int mask;
};

struct qof_events {
	u16 sw_event_lock;
	u16 hw_event_restore;
};

/**
 * @brief Record every mtcmos status
 *		such as number of users who are
 *		using this mtcmos power.
 */
struct imgsys_cg_data {
	u32 clr_ofs;
	u32 sta_ofs;
};

struct imgsys_mtcmos_data {
	u32 pwr_id;
	void (*cg_ungating)(struct cmdq_pkt *pkt, const struct imgsys_cg_data *cg, dma_addr_t qof_work_buf_pa);
	const struct imgsys_cg_data *cg_data;
	void (*set_larb_golden)(struct cmdq_pkt *pkt);
	void (*direct_link_reset)(struct cmdq_pkt *pkt);
	void (*qof_restore_done)(struct cmdq_pkt *pkt, u32 pwr_id);
};

void mtk_imgsys_cmdq_qof_init(struct mtk_imgsys_dev *imgsys_dev, struct cmdq_client *imgsys_clt);
void mtk_imgsys_cmdq_qof_release(struct mtk_imgsys_dev *imgsys_dev, struct cmdq_client *imgsys_clt);
void mtk_imgsys_cmdq_get_non_qof_module(u32 *non_qof_modules);
void mtk_imgsys_cmdq_qof_stream_on(struct mtk_imgsys_dev *imgsys_dev);
void mtk_imgsys_cmdq_qof_stream_off(struct mtk_imgsys_dev *imgsys_dev);
void mtk_imgsys_cmdq_qof_add(struct cmdq_pkt *pkt, bool *qof_need_sub, u32 hw_comb);
void mtk_imgsys_cmdq_qof_sub(struct cmdq_pkt *pkt, bool *qof_need_sub);
void mtk_imgsys_cmdq_qof_dump(uint32_t hwcomb, bool need_dump_cg);
int smi_isp_dip_get_if_in_use(void *data);
int smi_isp_dip_put(void *data);
int smi_isp_traw_get_if_in_use(void *data);
int smi_isp_traw_put(void *data);
int smi_isp_wpe1_eis_get_if_in_use(void *data);
int smi_isp_wpe1_eis_put(void *data);
int smi_isp_wpe2_tnr_get_if_in_use(void *data);
int smi_isp_wpe2_tnr_put(void *data);
int smi_isp_wpe3_lite_get_if_in_use(void *data);
int smi_isp_wpe3_lite_put(void *data);
