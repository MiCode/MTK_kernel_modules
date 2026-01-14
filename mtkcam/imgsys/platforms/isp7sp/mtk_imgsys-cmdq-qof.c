/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 *
 * Author: Yuan-Jung Kuo <yuan-jung.kuo@mediatek.com>
 *          Nick.wen <nick.wen@mediatek.com>
 *
 */

#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/soc/mediatek/mtk-cmdq-ext.h>
#include <linux/kernel.h>
#include <linux/delay.h>

#include "vcp_status.h"
#include "mtk_imgsys-engine.h"
#include "mtk_imgsys-cmdq.h"
#include "mtk_imgsys-cmdq-plat.h"
#include "mtk_imgsys-cmdq-qof-data.h"

#define IMG_MAIN_BASE (0x15000000)
#define HWCCF_BASE (0x1ec3a000)
#define HWCCF_GCE_OFST (0x1000)
#define MTK_POLL_DELAY_US		(10)
#define MTK_POLL_TIMEOUT		USEC_PER_SEC
#define MTK_POLL_100MS_TIMEOUT		(100 * USEC_PER_MSEC)
#define MTK_POLL_IRQ_DELAY_US		(3)
#define MTK_POLL_IRQ_TIMEOUT		USEC_PER_SEC
#define MTK_POLL_HWV_PREPARE_CNT	(100)
#define MTK_POLL_HWV_PREPARE_US		(2)
#define MTK_CG_RST_VAL				(0xF000)

#define IMG_MAIN_CG_TRAW0_CGPDN_OFS (1)
#define IMG_MAIN_CG_WPE0_CGPDN_OFS (4)
#define IMG_MAIN_CG_WPE1_CGPDN_OFS (6)
#define IMG_MAIN_CG_WPE2_CGPDN_OFS (7)
#define IMG_MAIN_CG_GALS_RX_DIP0_CGPDN_OFS (21)
#define IMG_MAIN_CG_GALS_RX_TRAW0_CGPDN_OFS (23)
#define IMG_MAIN_CG_GALS_RX_WPE0_CGPDN_OFS (24)
#define IMG_MAIN_CG_GALS_RX_WPE1_CGPDN_OFS (25)
#define IMG_MAIN_CG_GALS_RX_WPE2_CGPDN_OFS (26)

#define IMG_HWV_DELAY_CNT (0xFFFF)
#define IMG_CG_UNGATING_DELAY_CNT (0xFFFF)
#define IMG_MTCMOS_STABLE_CNT (0xFFFF)

#define IMG_NULL_MAGINC_NUM	(0xDEAD)

/* IMG sw token for Qof */
#define CMDQ_SYNC_TOKEN_DIP_POWER_CTRL			(862)
#define CMDQ_SYNC_TOKEN_DIP_TRIG_PWR_ON			(863)
#define CMDQ_SYNC_TOKEN_DIP_PWR_ON				(864)
#define CMDQ_SYNC_TOKEN_DIP_TRIG_PWR_OFf		(865)
#define CMDQ_SYNC_TOKEN_DIP_PWR_OFF				(866)
#define CMDQ_SYNC_TOKEN_DIP_PWR_HAND_SHAKE		(867)

#define CMDQ_SYNC_TOKEN_TRAW_POWER_CTRL			(868)
#define CMDQ_SYNC_TOKEN_TRAW_TRIG_PWR_ON		(869)
#define CMDQ_SYNC_TOKEN_TRAW_PWR_ON				(870)
#define CMDQ_SYNC_TOKEN_TRAW_TRIG_PWR_OFf		(871)
#define CMDQ_SYNC_TOKEN_TRAW_PWR_OFF			(872)
#define CMDQ_SYNC_TOKEN_TRAW_PWR_HAND_SHAKE		(873)


/* Footprint definition */
#define QOF_FP_CG_IMGSYS_MAIN	BIT(14)
#define QOF_FP_CG_DIP_TOP_DIP1	BIT(13)
#define QOF_FP_CG_DIP_NR1_DIP1	BIT(12)
#define QOF_FP_CG_DIP_NR2_DIP1	BIT(11)
#define QOF_FP_CG_TRAW_DIP1		BIT(10)
#define QOF_FP_CG_TRAW_CAP_DIP	BIT(9)
#define QOF_FP_CG_WPE1_DIP1		BIT(8)
#define QOF_FP_CG_WPE2_DIP1		BIT(7)
#define QOF_FP_CG_WPE3_DIP1		BIT(6)
#define QOF_FP_USER_DIP			BIT(5)
#define QOF_FP_USER_TRAW		BIT(4)
#define QOF_FP_PWR_ON			BIT(3)
#define QOF_FP_PWR_OFF			BIT(2)
#define QOF_FP_STATUS_ACK		BIT(1)
#define QOF_FP_HWCCF_ACK		BIT(0)

u8 g_qof_ver = 0;
u32 g_dbg_log_on = 0;
u32 g_disable_qof = 0;
u32 g_force_dump_vcp = 0;
static struct cmdq_pkt *g_gce_set_rg_pkt;
static struct qof_events qof_events_7sp[IMGSYS_CMDQ_SYNC_POOL_NUM];
u32 **g_dip_work_buf_va = NULL;
u32 **g_traw_work_buf_va = NULL;
unsigned int **g_qof_work_buf_va = NULL;

/** Struct list
 *
 * @brief List global struct
 */
enum IMG_GCE_PWR_THREAD_ID {
	IMG_GCE_THREAD_PWR_START,
	IMG_GCE_THREAD_DIP_PWR_ON = IMG_GCE_THREAD_PWR_START,
	IMG_GCE_THREAD_DIP_PWR_OFF,
	IMG_GCE_THREAD_TRAW_PWR_ON,
	IMG_GCE_THREAD_TRAW_PWR_OFF,
	IMG_GCE_THREAD_PWR_END = IMG_GCE_THREAD_TRAW_PWR_OFF,
};
#define QOF_TOTAL_THREAD (IMG_GCE_THREAD_PWR_END - IMG_GCE_THREAD_PWR_START + 1)
static struct cmdq_client *imgsys_pwr_clt[QOF_TOTAL_THREAD];

/* Debug dump reg */
enum DUMP_RG_ID_LIST {
	DUMP_RG_START,
	DUMP_RG_DIP_PWR_CON = DUMP_RG_START,
	DUMP_RG_TRAW_PWR_CON,
	DUMP_RG_CG_IMGSYS_MAIN,
	DUMP_RG_CG_TRAW_DIP1,
	DUMP_RG_CG_TRAW_CAP_DIP,
	DUMP_RG_CG_DIP_NR1_DIP1,
	DUMP_RG_CG_DIP_NR2_DIP1,
	DUMP_RG_CG_DIP_TOP_DIP1,
	DUMP_RG_CG_WPE1_DIP1,
	DUMP_RG_CG_WPE2_DIP1,
	DUMP_RG_CG_WPE3_DIP1,
	DUMP_RG_VOTER_ON,
	DUMP_RG_VOTER_OFF,
	DUMP_RG_HWV_DONE,
	DUMP_RG_CCF_MTCMOS_SET_STA,
	DUMP_RG_CCF_MTCMOS_CLR_STA,
	DUMP_RG_HWV_SW_RECORD,
	DUMP_RG_HW_CCF_MTCMOS_STATUS,
	DUMP_RG_HW_CCF_MTCMOS_STATUS_CLR,
	DUMP_RG_HW_CCF_MTCMOS_ENABLE,
	DUMP_RG_CCF_INT_STATUS,
	DUMP_RG_END = DUMP_RG_CCF_INT_STATUS,
};
#define DUMP_RG_TOTAL_NUM (DUMP_RG_END - DUMP_RG_START + 1)
#define READ_QOF_RG(rg_idx) 	g_qof_dump_reg[rg_idx] == NULL ? IMG_NULL_MAGINC_NUM : (unsigned int)ioread32((void *)g_qof_dump_reg[rg_idx])
static void __iomem *g_qof_dump_reg[DUMP_RG_TOTAL_NUM];

struct cmdq_pwr_buf {
	struct cmdq_pkt *on_pkt;
	struct cmdq_pkt *off_pkt;
};

static struct cmdq_pwr_buf pwr_buf_handle[ISP7SP_PWR_NUM] = {
	{
		.on_pkt = NULL,
		.off_pkt = NULL,
	},
	{
		.on_pkt = NULL,
		.off_pkt = NULL,
	}
};

/** Local api
 * @brief List local api
 */
static void imgsys_cmdq_init_qof_events(void)
{
	struct qof_events *event;

	/* ISP_DIP */
	event = &qof_events_7sp[ISP7SP_ISP_DIP];
	event->power_ctrl = CMDQ_SYNC_TOKEN_DIP_POWER_CTRL;
	event->trig_pwr_on = CMDQ_SYNC_TOKEN_DIP_TRIG_PWR_ON;
	event->trig_pwr_off = CMDQ_SYNC_TOKEN_DIP_TRIG_PWR_OFf;
	event->pwr_off = CMDQ_SYNC_TOKEN_DIP_PWR_OFF;
	event->pwr_hand_shake = CMDQ_SYNC_TOKEN_DIP_PWR_HAND_SHAKE;

	/* TRAW */
	event = &qof_events_7sp[ISP7SP_ISP_TRAW];
	event->power_ctrl = CMDQ_SYNC_TOKEN_DIP_POWER_CTRL;	// Use the same as dip now, need to check.
	event->trig_pwr_on = CMDQ_SYNC_TOKEN_TRAW_TRIG_PWR_ON;
	event->trig_pwr_off = CMDQ_SYNC_TOKEN_TRAW_TRIG_PWR_OFf;
	event->pwr_off = CMDQ_SYNC_TOKEN_TRAW_PWR_OFF;
	event->pwr_hand_shake = CMDQ_SYNC_TOKEN_TRAW_PWR_HAND_SHAKE;

	/* common */
	//g_qof_lock_event = imgsys_event[IMGSYS_CMDQ_SYNC_TOKEN_TRAW_PWR_ON].event;
}

static void imgsys_cmdq_qof_alloc_buf(struct cmdq_client *imgsys_clt, u32 **buf_va, dma_addr_t *buf_pa)
{
	int *p;

	if (!imgsys_clt) {
		cmdq_err("param is NULL");
		dump_stack();
		return;
	}

	*buf_va = cmdq_mbox_buf_alloc(imgsys_clt, buf_pa);
	if (*buf_va) {
		p = (int*)*buf_va;
		*p = 0;
	} else {
		pr_err("%s: cmdq mbox buf alloc fail\n", __func__);
	}
}

static unsigned int imgsys_get_cg_fp(unsigned int addr)
{
	unsigned int val = 0;
	switch(addr) {
		case IMG_CG_IMGSYS_MAIN:
			val = QOF_FP_CG_IMGSYS_MAIN;
			break;
		case IMG_CG_DIP_TOP_DIP1:
			val = QOF_FP_CG_DIP_TOP_DIP1;
			break;
		case IMG_CG_DIP_NR1_DIP1:
			val = QOF_FP_CG_DIP_NR1_DIP1;
			break;
		case IMG_CG_DIP_NR2_DIP1:
			val = QOF_FP_CG_DIP_NR2_DIP1;
			break;
		case IMG_CG_TRAW_DIP1:
			val = QOF_FP_CG_TRAW_DIP1;
			break;
		case IMG_CG_TRAW_CAP_DIP:
			val = QOF_FP_CG_TRAW_CAP_DIP;
			break;
		case IMG_CG_WPE1_DIP1:
			val = QOF_FP_CG_WPE1_DIP1;
			break;
		case IMG_CG_WPE2_DIP1:
			val = QOF_FP_CG_WPE2_DIP1;
			break;
		case IMG_CG_WPE3_DIP1:
			val = QOF_FP_CG_WPE3_DIP1;
			break;
		default:
			val = 0;
			break;
	}
	return val;
}

static void imgsys_set_footprint(struct cmdq_pkt *pkt, dma_addr_t qof_buf_pa,
		unsigned int val, enum CMDQ_LOGIC_ENUM s_op)
{
	dma_addr_t user_count_pa;
	const u16 var1 = CMDQ_THR_SPR_IDX2;
	struct cmdq_operand lop;
	struct cmdq_operand rop;

	/* Load user_count from DRAM to register */
	user_count_pa = qof_buf_pa;
	cmdq_pkt_read(pkt, NULL, user_count_pa, var1);

	lop.reg = true;
	lop.idx = var1;
	rop.reg = false;
	rop.value = val;
	cmdq_pkt_logic_command(pkt, s_op, CMDQ_THR_SPR_IDX2, &lop, &rop);
	cmdq_pkt_write_indriect(pkt, NULL, user_count_pa, CMDQ_THR_SPR_IDX2, ~0);
}

static void mtk_imgsys_cmdq_hwv_is_done(struct cmdq_pkt* pkt, const struct imgsys_mtcmos_data *pwr)
{
	unsigned int hwv_done_ofs = pwr->hwv_done_ofs;
	unsigned int hwv_shift = pwr->hwv_shift;

	/* no need to add GCE ofst from owner chong-ming */
	cmdq_pkt_poll_timeout(pkt, BIT(hwv_shift)/*poll val*/,
			SUBSYS_NO_SUPPORT, (HWCCF_BASE + hwv_done_ofs)/*addr*/,
			BIT(hwv_shift) /*mask*/, IMG_HWV_DELAY_CNT /*delay cnt*/,
			CMDQ_GPR_R13 /*GPR*/);
}

static void mtk_imgsys_qof_mtcmos_ctrl(bool isMTCMOSOn, struct cmdq_pkt *pkt,
		const struct imgsys_mtcmos_data *pwr, dma_addr_t qof_work_buf_pa)
{
	unsigned int reg = isMTCMOSOn ? pwr->vote_on_ofs : pwr->vote_off_ofs;
	unsigned int hwv_shift = pwr->hwv_shift;
	unsigned int val;
	unsigned int sta_addr;

	/* wait previous hwvote activity done */
	mtk_imgsys_cmdq_hwv_is_done(pkt, pwr);

	/* write hwccf to vote choices */
	val = BIT(hwv_shift);

	//cmdq_pkt_acquire_event(pkt, g_qof_lock_event);
	cmdq_pkt_write(pkt, NULL, (HWCCF_BASE + HWCCF_GCE_OFST + reg) /*address*/,
			       val, 0xffffffff);
	//cmdq_pkt_clear_event(pkt, g_qof_lock_event);

	/* poll MTCMOS on/off */
	if (isMTCMOSOn == true)
		val = BIT(hwv_shift);
	else
		val = 0;

	cmdq_pkt_poll_timeout(pkt, val /*poll val*/, SUBSYS_NO_SUPPORT,
			(HWCCF_BASE + HWCCF_GCE_OFST + reg)/*addr*/,
			BIT(hwv_shift) /*mask*/, IMG_MTCMOS_STABLE_CNT /*delay cnt*/,
			CMDQ_GPR_R13 /*GPR*/);
	imgsys_set_footprint(pkt, qof_work_buf_pa, QOF_FP_HWCCF_ACK, CMDQ_LOGIC_OR);

	/* wait hwccf finish vote request */
	mtk_imgsys_cmdq_hwv_is_done(pkt, pwr);

	sta_addr = isMTCMOSOn ? IMG_CCF_MTCMOS_SET_STA : IMG_CCF_MTCMOS_CLR_STA;
	cmdq_pkt_poll_timeout(pkt, 0 /*poll val*/, SUBSYS_NO_SUPPORT,
			(sta_addr)/*addr*/,
			BIT(hwv_shift) /*mask*/, IMG_MTCMOS_STABLE_CNT /*delay cnt*/,
			CMDQ_GPR_R13 /*GPR*/);
	imgsys_set_footprint(pkt, qof_work_buf_pa, QOF_FP_STATUS_ACK, CMDQ_LOGIC_OR);
}

static void imgsys_cmdq_modules_cg_ungating(struct cmdq_pkt *pkt,
		unsigned int addr, const struct imgsys_cg_data *cg, unsigned int val, dma_addr_t qof_work_buf_pa)
{
	unsigned int reg;
	unsigned int clr_ofs = cg->clr_ofs;
	unsigned int sta_ofs = cg->sta_ofs;

	/* Clock un-gating */
	reg = addr + clr_ofs;
	cmdq_pkt_write(pkt, NULL, (reg) /* address*/ ,
			val /* val */, val);

	/* Wait clk un-gating */
	reg = addr + sta_ofs;

	cmdq_pkt_poll_timeout(pkt, 0/*poll val*/, SUBSYS_NO_SUPPORT, (reg)/*addr*/,
			val /*mask*/, IMG_CG_UNGATING_DELAY_CNT /*delay cnt*/,
			CMDQ_GPR_R13 /*GPR*/);

	imgsys_set_footprint(pkt, qof_work_buf_pa, imgsys_get_cg_fp(addr), CMDQ_LOGIC_OR);
}

static void imgsys_cmdq_dip_cg_unating(struct cmdq_pkt *pkt,
		const struct imgsys_cg_data *cg, dma_addr_t qof_work_buf_pa)
{
	unsigned int reg;
	unsigned int val;
	unsigned int clr_ofs = cg->clr_ofs;
	unsigned int main_cg =
		BIT(IMG_MAIN_CG_WPE0_CGPDN_OFS) |
		BIT(IMG_MAIN_CG_WPE1_CGPDN_OFS) |
		BIT(IMG_MAIN_CG_WPE2_CGPDN_OFS) |
		BIT(IMG_MAIN_CG_GALS_RX_DIP0_CGPDN_OFS) |
		BIT(IMG_MAIN_CG_GALS_RX_WPE0_CGPDN_OFS) |
		BIT(IMG_MAIN_CG_GALS_RX_WPE1_CGPDN_OFS) |
		BIT(IMG_MAIN_CG_GALS_RX_WPE2_CGPDN_OFS);

	/* Turn on main cg */
	reg = IMG_CG_IMGSYS_MAIN + clr_ofs;
	cmdq_pkt_write(pkt, NULL, reg /* address*/,
					main_cg, 0xffffffff);

	val = BIT(0)|BIT(1);
	imgsys_cmdq_modules_cg_ungating(pkt, IMG_CG_DIP_NR1_DIP1, cg, val, qof_work_buf_pa);

	/* DIP_NR2_DIP1 */
	val = BIT(0)|BIT(1)|BIT(2);
	imgsys_cmdq_modules_cg_ungating(pkt, IMG_CG_DIP_NR2_DIP1, cg, val, qof_work_buf_pa);

	/* DIP_TOP_DIP1 */
	val = BIT(0)|BIT(1)|BIT(2)|BIT(3)|BIT(4)|BIT(5)|BIT(6)|BIT(7)|BIT(8);
	imgsys_cmdq_modules_cg_ungating(pkt, IMG_CG_DIP_TOP_DIP1, cg, val, qof_work_buf_pa);

	/* WPE1_DIP1 */
	val = BIT(0)|BIT(1)|BIT(2);
	imgsys_cmdq_modules_cg_ungating(pkt, IMG_CG_WPE1_DIP1, cg, val, qof_work_buf_pa);

	/* WPE2_DIP1 */
	val = BIT(0)|BIT(1)|BIT(2);
	imgsys_cmdq_modules_cg_ungating(pkt, IMG_CG_WPE2_DIP1, cg, val, qof_work_buf_pa);

	/* WPE3_DIP1 */
	val = BIT(0)|BIT(1)|BIT(2);
	imgsys_cmdq_modules_cg_ungating(pkt, IMG_CG_WPE3_DIP1, cg, val, qof_work_buf_pa);
}

static void imgsys_cmdq_traw_cg_unating(struct cmdq_pkt *pkt,
		const struct imgsys_cg_data *cg, dma_addr_t qof_work_buf_pa)
{
	unsigned int reg;
	unsigned int val;
	unsigned int clr_ofs = cg->clr_ofs;
	unsigned int main_cg =
		BIT(IMG_MAIN_CG_TRAW0_CGPDN_OFS) |
		BIT(IMG_MAIN_CG_GALS_RX_TRAW0_CGPDN_OFS);

	/* Turn on main cg */
	reg = IMG_CG_IMGSYS_MAIN + clr_ofs;
	cmdq_pkt_write(pkt, NULL, reg /* address*/,
					main_cg, 0xffffffff);

	/* 1 &traw_dip1_clk	CLK_TRAW_DIP1_TRAW */
	val = BIT(0)|BIT(1)|BIT(2)|BIT(3);
	imgsys_cmdq_modules_cg_ungating(pkt, IMG_CG_TRAW_DIP1, cg, val, qof_work_buf_pa);

	/* 2 TRAW CAP */
	val = BIT(0);
	imgsys_cmdq_modules_cg_ungating(pkt, IMG_CG_TRAW_CAP_DIP, cg, val, qof_work_buf_pa);
}

static void imgsys_cmdq_set_larb_golden(struct qof_larb_info larb_info, struct cmdq_pkt *pkt)
{
	unsigned int reg_ba, ofset, bound, i;
	if (larb_info.larb_reg_list == NULL) {
		pr_err("[%s][%d]Param is null !\n", __func__, __LINE__);
		return;
	}

	reg_ba = larb_info.reg_ba;
	bound = larb_info.reg_list_size;

	for (i = 0; i < bound; i++) {
		ofset = reg_ba + larb_info.larb_reg_list[i].ofset;
		cmdq_pkt_write(pkt, NULL, ofset /*address*/,
				larb_info.larb_reg_list[i].val, 0xffffffff);
	}
}

static void mtk_imgsys_set_dip_larb_golden(struct cmdq_pkt *pkt)
{
	if (pkt == NULL) {
		pr_err("[%s][%d]Param is null !\n", __func__, __LINE__);
		return;
	}

	/* larb 10 */
	imgsys_cmdq_set_larb_golden(qof_larb10_info, pkt);

	/* larb 11 */
	imgsys_cmdq_set_larb_golden(qof_larb11_info, pkt);

	/* larb 15 */
	imgsys_cmdq_set_larb_golden(qof_larb15_info, pkt);

	/* larb 22 */
	imgsys_cmdq_set_larb_golden(qof_larb22_info, pkt);

	/* larb 23 */
	imgsys_cmdq_set_larb_golden(qof_larb23_info, pkt);

	/* larb 38 */
	imgsys_cmdq_set_larb_golden(qof_larb38_info, pkt);

	/* larb 39 */
	imgsys_cmdq_set_larb_golden(qof_larb39_info, pkt);
}

static void mtk_imgsys_set_traw_larb_golden(struct cmdq_pkt *pkt)
{
	if (pkt == NULL) {
		pr_err("[%s][%d]Param is null !\n", __func__, __LINE__);
		return;
	}

	/* larb 28 */
	imgsys_cmdq_set_larb_golden(qof_larb28_info, pkt);

	/* larb 40 */
	imgsys_cmdq_set_larb_golden(qof_larb40_info, pkt);
}

static void mtk_imgsys_dip_direct_link_reset(struct cmdq_pkt *pkt)
{
	cmdq_pkt_write(pkt, NULL,
			(IMG_MAIN_BASE + 0x260) /*address*/, 0x17F,
			0xffffffff);
	cmdq_pkt_write(pkt, NULL,
			(IMG_MAIN_BASE + 0x260) /*address*/, 0x0,
			0xffffffff);
}

static void mtk_imgsys_traw_direct_link_reset(struct cmdq_pkt *pkt)
{
	cmdq_pkt_write(pkt, NULL,
			(IMG_MAIN_BASE + 0x260) /*address*/, 0xB2,
			0xffffffff);
	cmdq_pkt_write(pkt, NULL,
			(IMG_MAIN_BASE + 0x260) /*address*/, 0x0,
			0xffffffff);
}

/* mtcmos data */
static const struct imgsys_mtcmos_data isp7sp_mtcmos_data[] = {
	[ISP7SP_ISP_DIP] = {
		.module_list = ISP_DIP_MODULES,
		.vote_on_ofs = 0x0198,
		.vote_off_ofs = 0x019C,
		.hwv_done_ofs = 0x141C,
		.hwv_shift = 28,
		.hwv_gce_ofst = 0x1000,
		.cg_ungating = imgsys_cmdq_dip_cg_unating,
		.cg_data = &dip_cg_data,
		.set_larb_golden = mtk_imgsys_set_dip_larb_golden,
		.direct_link_reset = mtk_imgsys_dip_direct_link_reset,
	},
	[ISP7SP_ISP_TRAW] = {
		.module_list = ISP_TRAW_MODULES,
		.vote_on_ofs = 0x0198,
		.vote_off_ofs = 0x019C,
		.hwv_done_ofs = 0x141C,
		.hwv_shift = 27,
		.hwv_gce_ofst = 0x1000,
		.cg_ungating = imgsys_cmdq_traw_cg_unating,
		.cg_data = &traw_cg_data,
		.set_larb_golden = mtk_imgsys_set_traw_larb_golden,
		.direct_link_reset = mtk_imgsys_traw_direct_link_reset,
	},
};
#define isp7sp_mtcmos_data_SIZE (ARRAY_SIZE(isp7sp_mtcmos_data))

static void mtk_imgsys_cmdq_power_ctrl(struct mtk_imgsys_dev *imgsys_dev,
		bool isPowerOn, struct cmdq_pkt *pkt,
		const struct imgsys_mtcmos_data *pwr)
{
	int i;

	if (isPowerOn) {
		mtk_imgsys_qof_mtcmos_ctrl(isPowerOn, pkt, pwr, imgsys_dev->qof_work_buf_pa);

		pwr->cg_ungating(pkt, pwr->cg_data, imgsys_dev->qof_work_buf_pa);
		pwr->set_larb_golden(pkt);

		/* reset module after power on */
		for (i = 0; i < imgsys_dev->modules_num; i++) {
			if ((BIT(i) & pwr->module_list) && imgsys_dev->modules[i].cmdq_set) {
				imgsys_dev->modules[i].cmdq_set(imgsys_dev, (void*)pkt);
			}
		}

		/* We need to reset direct link path after macro reset */
		pwr->direct_link_reset(pkt);
	} else {
		mtk_imgsys_qof_mtcmos_ctrl(isPowerOn, pkt, pwr, imgsys_dev->qof_work_buf_pa);
	}
}

static void imgsys_cmdq_power_on_locked(struct mtk_imgsys_dev *imgsys_dev,
		struct cmdq_pkt *pkt, dma_addr_t work_buf_pa,
		const struct imgsys_mtcmos_data *pwr)
{
	struct cmdq_operand lop, rop;
	/* spr is gce per-thread internal 32bit ram, use as variable */
	const u16 reg_jump = CMDQ_THR_SPR_IDX1;
	const u16 var1 = CMDQ_THR_SPR_IDX2;
	dma_addr_t user_count_pa = work_buf_pa + offsetof(struct qof_state, user_count);
	u32 inst_condi_jump, inst_jump_end;
	u64 *inst, jump_pa;

	/* read data from some pa to spr for compare later */
	cmdq_pkt_read(pkt, NULL, user_count_pa, var1);
	lop.reg = true;
	lop.idx = var1;
	rop.reg = false;
	rop.value = 1;
	/* mark condition jump and change offset later */
	inst_condi_jump = pkt->cmd_buf_size;
	cmdq_pkt_assign_command(pkt, reg_jump, 0);

	/* case: counter > 1 */
	cmdq_pkt_cond_jump_abs(pkt, reg_jump, &lop, &rop, CMDQ_EQUAL);
	/* now fill in code that condition not match, here is nop jump */
	// cmdq_pkt_write(pkt, NULL, imgsys_dev->time_ans_pa, 0xadddead, ~0);

	inst_jump_end = pkt->cmd_buf_size;
	/* Finsih else statement, jump to the end of if-else braces.
	*  Assign jump address as zero initially and we will modify it later.
	*/
	cmdq_pkt_jump_addr(pkt, 0);

	/* following instructinos is condition TRUE,
	   thus conditional jump should jump current offset
	*/
	inst = cmdq_pkt_get_va_by_offset(pkt, inst_condi_jump);
	jump_pa = cmdq_pkt_get_pa_by_offset(pkt, pkt->cmd_buf_size);
	*inst = *inst & ((u64)0xFFFFFFFF << 32);
	*inst = *inst | CMDQ_REG_SHIFT_ADDR(jump_pa);

	/* now fill in code that condition match */
	/* case: counter == 1 */
	mtk_imgsys_cmdq_power_ctrl(imgsys_dev, true, pkt, pwr);

	/* this is the end of whole condition, thus condition FALSE part should jump here */
	jump_pa = cmdq_pkt_get_pa_by_offset(pkt, pkt->cmd_buf_size);
	inst = cmdq_pkt_get_va_by_offset(pkt, inst_jump_end);
	*inst = *inst & ((u64)0xFFFFFFFF << 32);
	*inst = *inst | CMDQ_REG_SHIFT_ADDR(jump_pa);
}

static void imgsys_cmdq_power_off_locked(struct mtk_imgsys_dev *imgsys_dev,
		struct cmdq_pkt *pkt, dma_addr_t work_buf_pa,
		const struct imgsys_mtcmos_data *pwr)
{
	struct cmdq_operand lop, rop;
	/* spr is gce per-thread internal 32bit ram, use as variable */
	const u16 reg_jump = CMDQ_THR_SPR_IDX1;
	const u16 var1 = CMDQ_THR_SPR_IDX2;
	dma_addr_t user_count_pa = work_buf_pa + offsetof(struct qof_state, user_count);
	u32 inst_condi_jump, inst_jump_end;
	u64 *inst, jump_pa;

	/* read data from some pa to spr for compare later */
	cmdq_pkt_read(pkt, NULL, user_count_pa, var1);
	lop.reg = true;
	lop.idx = var1;
	rop.reg = false;
	rop.value = 0;
	/* mark condition jump and change offset later */
	inst_condi_jump = pkt->cmd_buf_size;
	cmdq_pkt_assign_command(pkt, reg_jump, 0);

	cmdq_pkt_cond_jump_abs(pkt, reg_jump, &lop, &rop, CMDQ_EQUAL);
	/* now fill in code that condition not match */
	/* case: counter >= 0 */
	// cmdq_pkt_write(pkt, NULL, imgsys_dev->time_ans_sub_pa, 0xddadead, ~0);

	inst_jump_end = pkt->cmd_buf_size;
	/* Finsih else statement, jump to the end of if-else braces.
	*  Assign jump address as zero initially and we will modify it later.
	*/
	cmdq_pkt_jump_addr(pkt, 0);

	/* following instructinos is condition TRUE,
	* thus conditional jump should jump current offset
	*/
	inst = cmdq_pkt_get_va_by_offset(pkt, inst_condi_jump);
	jump_pa = cmdq_pkt_get_pa_by_offset(pkt, pkt->cmd_buf_size);
	*inst = *inst & ((u64)0xFFFFFFFF << 32);
	*inst = *inst | CMDQ_REG_SHIFT_ADDR(jump_pa);
	/* now fill in code that condition match, here is nop jump */
	/* case: counter == 0 */
	mtk_imgsys_cmdq_power_ctrl(imgsys_dev, false, pkt, pwr);
	/* Reset footprint */
	imgsys_set_footprint(pkt, imgsys_dev->qof_work_buf_pa, 0, CMDQ_LOGIC_AND);

	/* this is end of whole condition, thus condition FALSE part should jump here */
	jump_pa = cmdq_pkt_get_pa_by_offset(pkt, pkt->cmd_buf_size);
	inst = cmdq_pkt_get_va_by_offset(pkt, inst_jump_end);
	*inst = *inst & ((u64)0xFFFFFFFF << 32);
	*inst = *inst | CMDQ_REG_SHIFT_ADDR(jump_pa);
}

static void imgsys_user_count_operation(struct cmdq_pkt *pkt, dma_addr_t work_buf_pa,
		enum CMDQ_LOGIC_ENUM s_op)
{
	dma_addr_t user_count_pa;
	const u16 var1 = CMDQ_THR_SPR_IDX2;
	struct cmdq_operand lop;
	struct cmdq_operand rop;

	/* Load user_count from DRAM to register */
	user_count_pa = work_buf_pa + offsetof(struct qof_state, user_count);
	cmdq_pkt_read(pkt, NULL, user_count_pa, var1);

	lop.reg = true;
	lop.idx = var1;
	rop.reg = false;
	rop.value = 1;
	cmdq_pkt_logic_command(pkt,
			s_op, CMDQ_THR_SPR_IDX2, &lop, &rop);
	cmdq_pkt_write_indriect(pkt, NULL, user_count_pa, CMDQ_THR_SPR_IDX2, ~0);
}

static void imgsys_cmdq_start_power_on_task(struct mtk_imgsys_dev *imgsys_dev,
		struct cmdq_client *client, u32 pwr_id,
		struct qof_events *event)
{
	struct cmdq_pkt *pwr_on_pkt;
	dma_addr_t work_buf_pa;

	if (pwr_id >= isp7sp_mtcmos_data_SIZE) {
		pr_info("%s:mtcmos id is invalid\n", __func__);
		return;
	}

	pwr_on_pkt = pwr_buf_handle[pwr_id].on_pkt;

	/* Power on kernel thread */
	cmdq_mbox_enable(client->chan);

	if (!pwr_on_pkt) {
		pwr_on_pkt = pwr_buf_handle[pwr_id].on_pkt = cmdq_pkt_create(client);
		if (!pwr_on_pkt) {
			pr_info("%s:create cmdq package fail\n", __func__);
			return;
		}
	}

	/* Program start */
	/* Wait for power on event */
	cmdq_pkt_wfe(pwr_on_pkt, event->trig_pwr_on);

	/* Increase user counter */
	//work_buf_pa = imgsys_dev->work_buf_pa + pwr_id * sizeof(struct qof_state);
	if (pwr_id == ISP7SP_ISP_DIP) {
		work_buf_pa = imgsys_dev->work_buf_pa;
	} else {
		work_buf_pa = imgsys_dev->traw_work_buf_pa;
	}
	imgsys_user_count_operation(pwr_on_pkt, work_buf_pa, CMDQ_LOGIC_ADD);

	imgsys_cmdq_power_on_locked(imgsys_dev, pwr_on_pkt,
			work_buf_pa, &isp7sp_mtcmos_data[pwr_id]);
	if (pwr_id == ISP7SP_ISP_DIP) {
		imgsys_set_footprint(pwr_on_pkt, imgsys_dev->qof_work_buf_pa, (QOF_FP_USER_DIP|QOF_FP_PWR_ON), CMDQ_LOGIC_OR);
	} else {
		imgsys_set_footprint(pwr_on_pkt, imgsys_dev->qof_work_buf_pa, (QOF_FP_USER_TRAW|QOF_FP_PWR_ON), CMDQ_LOGIC_OR);
	}

	/* MTCMOS is power on, notify caller thread */
	cmdq_pkt_clear_event(pwr_on_pkt, event->trig_pwr_on);
	cmdq_pkt_set_event(pwr_on_pkt, event->pwr_hand_shake);

	/* Program end */
	pwr_on_pkt->priority = IMGSYS_PRI_HIGH;
	cmdq_pkt_finalize_loop(pwr_on_pkt);
	//cmdq_dump_pkt(pwr_on_pkt, 0, true);
	cmdq_pkt_flush_async(pwr_on_pkt, NULL, (void *)pwr_on_pkt);
}

static void imgsys_cmdq_start_power_off_task(struct mtk_imgsys_dev *imgsys_dev,
		struct cmdq_client *client, u32 pwr_id,
		struct qof_events *event)
{
	struct cmdq_pkt *pwr_off_pkt;
	dma_addr_t work_buf_pa;

	if (pwr_id >= isp7sp_mtcmos_data_SIZE) {
		pr_info("%s:mtcmos id is invalid\n", __func__);
		return;
	}

	pwr_off_pkt = pwr_buf_handle[pwr_id].off_pkt;

	/* Power off GCE thread */
	cmdq_mbox_enable(client->chan);

	if (!pwr_off_pkt) {
		pwr_off_pkt = pwr_buf_handle[pwr_id].off_pkt = cmdq_pkt_create(client);
		if (!pwr_off_pkt) {
			pr_info("%s:create cmdq package fail\n", __func__);
			return;
		}
	}

	/* Program start */
	cmdq_pkt_wfe(pwr_off_pkt, event->trig_pwr_off);

	/* Decrease user counter */
	//work_buf_pa = imgsys_dev->work_buf_pa + pwr_id * sizeof(struct qof_state);
	if (pwr_id == ISP7SP_ISP_DIP) {
		work_buf_pa = imgsys_dev->work_buf_pa;
		imgsys_set_footprint(pwr_off_pkt, imgsys_dev->qof_work_buf_pa, (QOF_FP_USER_DIP|QOF_FP_PWR_OFF), CMDQ_LOGIC_OR);
	} else {
		work_buf_pa = imgsys_dev->traw_work_buf_pa;
		imgsys_set_footprint(pwr_off_pkt, imgsys_dev->qof_work_buf_pa, (QOF_FP_USER_TRAW|QOF_FP_PWR_OFF), CMDQ_LOGIC_OR);
	}
	imgsys_user_count_operation(pwr_off_pkt, work_buf_pa, CMDQ_LOGIC_SUBTRACT);

	imgsys_cmdq_power_off_locked(imgsys_dev, pwr_off_pkt,
			work_buf_pa, &isp7sp_mtcmos_data[pwr_id]);
	/* clr trig pwr off event */
	cmdq_pkt_clear_event(pwr_off_pkt, event->trig_pwr_off);

	cmdq_pkt_set_event(pwr_off_pkt, event->pwr_off);

	/* Program end */
	pwr_off_pkt->priority = IMGSYS_PRI_HIGH;
	cmdq_pkt_finalize_loop(pwr_off_pkt);
	//cmdq_dump_pkt(pwr_off_pkt, 0, true);
	cmdq_pkt_flush_async(pwr_off_pkt, NULL, (void *)pwr_off_pkt);
}

static void imgsys_cmdq_stop_power_tasks(struct mtk_imgsys_dev *imgsys_dev,
		struct cmdq_client *clt_on, struct cmdq_client *clt_off, u32 pwr_id)
{
	cmdq_pkt_destroy(pwr_buf_handle[pwr_id].on_pkt);
	cmdq_pkt_destroy(pwr_buf_handle[pwr_id].off_pkt);
	pwr_buf_handle[pwr_id].on_pkt = pwr_buf_handle[pwr_id].off_pkt = NULL;

	cmdq_mbox_disable(clt_on->chan);
	cmdq_mbox_disable(clt_off->chan);
}

static void imgsys_cmdq_init_pwr_thread(struct mtk_imgsys_dev *imgsys_dev)
{
	struct device *dev = imgsys_dev->dev;
	u32 idx = 0, thd_idx = 0;

	for (thd_idx = IMGSYS_NOR_THD; thd_idx < IMGSYS_NOR_THD + IMGSYS_PWR_THD; thd_idx++) {
		idx = thd_idx - IMGSYS_NOR_THD;
		if (idx < QOF_TOTAL_THREAD) {
			imgsys_pwr_clt[thd_idx - IMGSYS_NOR_THD] = cmdq_mbox_create(dev, thd_idx);
			pr_info(
				"%s: cmdq_mbox_create pwr_thd(%d, 0x%lx)\n",
				__func__, thd_idx, (unsigned long)imgsys_pwr_clt[thd_idx - IMGSYS_NOR_THD]);
		} else {
			pr_err("[%s] thread indexs are not match ! [thd_id: %d][index: %d][total pwr thd num: %d/%d]\n",
				__func__, thd_idx, idx, QOF_TOTAL_THREAD, IMGSYS_PWR_THD);
		}
	}
}

/** Global api
 * @brief List Global api
 */
void mtk_imgsys_cmdq_qof_init(struct mtk_imgsys_dev *imgsys_dev, struct cmdq_client *imgsys_clt)
{
	int ver = 0;
	int dump_rg_idx = 0;

	if (of_property_read_u32_index(imgsys_dev->dev->of_node,
		"mediatek,imgsys-qof-ver", 0, &ver) == 0)
		pr_info("[%s] qof version = %u\n", __func__, ver);
	imgsys_dev->qof_ver = ver;
	g_qof_ver = ver;

	if (imgsys_dev->qof_ver == MTK_IMGSYS_QOF_FUNCTION_OFF)
		return;

	/* create pwr thread */
	imgsys_cmdq_init_pwr_thread(imgsys_dev);

	/* configure qof events ISP_DIP */
	imgsys_cmdq_init_qof_events();

	/* allocate work buf */
	imgsys_cmdq_qof_alloc_buf(imgsys_clt, &(imgsys_dev->work_buf_va), &(imgsys_dev->work_buf_pa));
	imgsys_cmdq_qof_alloc_buf(imgsys_clt, &(imgsys_dev->traw_work_buf_va), &(imgsys_dev->traw_work_buf_pa));
	imgsys_cmdq_qof_alloc_buf(imgsys_clt, &(imgsys_dev->qof_work_buf_va), &(imgsys_dev->qof_work_buf_pa));
	g_dip_work_buf_va = &(imgsys_dev->work_buf_va);
	g_traw_work_buf_va = &(imgsys_dev->traw_work_buf_va);
	g_qof_work_buf_va = &(imgsys_dev->qof_work_buf_va);

	/* Set global param */
	g_qof_dump_reg[DUMP_RG_DIP_PWR_CON] 			= ioremap(IMG_ISP_DIP_PWR_CON, 4);
	g_qof_dump_reg[DUMP_RG_TRAW_PWR_CON] 			= ioremap(IMG_TRAW_PWR_CON, 4);
	g_qof_dump_reg[DUMP_RG_CG_IMGSYS_MAIN] 			= ioremap(IMG_CG_IMGSYS_MAIN, 4);
	g_qof_dump_reg[DUMP_RG_CG_TRAW_DIP1] 			= ioremap(IMG_CG_TRAW_DIP1, 4);
	g_qof_dump_reg[DUMP_RG_CG_TRAW_CAP_DIP] 		= ioremap(IMG_CG_TRAW_CAP_DIP, 4);
	g_qof_dump_reg[DUMP_RG_CG_DIP_NR1_DIP1] 		= ioremap(IMG_CG_DIP_NR1_DIP1, 4);
	g_qof_dump_reg[DUMP_RG_CG_DIP_NR2_DIP1] 		= ioremap(IMG_CG_DIP_NR2_DIP1, 4);
	g_qof_dump_reg[DUMP_RG_CG_DIP_TOP_DIP1] 		= ioremap(IMG_CG_DIP_TOP_DIP1, 4);
	g_qof_dump_reg[DUMP_RG_CG_WPE1_DIP1] 			= ioremap(IMG_CG_WPE1_DIP1, 4);
	g_qof_dump_reg[DUMP_RG_CG_WPE2_DIP1] 			= ioremap(IMG_CG_WPE2_DIP1, 4);
	g_qof_dump_reg[DUMP_RG_CG_WPE3_DIP1] 			= ioremap(IMG_CG_WPE3_DIP1, 4);
	g_qof_dump_reg[DUMP_RG_VOTER_ON] 				= ioremap(HWCCF_BASE + HWCCF_GCE_OFST + isp7sp_mtcmos_data[ISP7SP_ISP_DIP].vote_on_ofs, 4);
	g_qof_dump_reg[DUMP_RG_VOTER_OFF] 				= ioremap(HWCCF_BASE + HWCCF_GCE_OFST + isp7sp_mtcmos_data[ISP7SP_ISP_DIP].vote_off_ofs, 4);
	g_qof_dump_reg[DUMP_RG_HWV_DONE] 				= ioremap(HWCCF_BASE + isp7sp_mtcmos_data[ISP7SP_ISP_DIP].hwv_done_ofs, 4);
	g_qof_dump_reg[DUMP_RG_CCF_MTCMOS_SET_STA] 		= ioremap(IMG_CCF_MTCMOS_SET_STA, 4);
	g_qof_dump_reg[DUMP_RG_CCF_MTCMOS_CLR_STA] 		= ioremap(IMG_CCF_MTCMOS_CLR_STA, 4);
	g_qof_dump_reg[DUMP_RG_HWV_SW_RECORD]			= ioremap(IMG_HWV_SW_RECORD, 4);
	g_qof_dump_reg[DUMP_RG_HW_CCF_MTCMOS_STATUS]	= ioremap(IMG_HW_CCF_MTCMOS_STATUS, 4);
	g_qof_dump_reg[DUMP_RG_HW_CCF_MTCMOS_STATUS_CLR]= ioremap(IMG_HW_CCF_MTCMOS_STATUS_CLR, 4);
	g_qof_dump_reg[DUMP_RG_HW_CCF_MTCMOS_ENABLE]	= ioremap(IMG_HW_CCF_MTCMOS_ENABLE, 4);
	g_qof_dump_reg[DUMP_RG_CCF_INT_STATUS]			= ioremap(IMG_HW_CCF_INT_STATUS, 4);

	for (dump_rg_idx = DUMP_RG_START; dump_rg_idx < DUMP_RG_TOTAL_NUM; dump_rg_idx++) {
		if (!g_qof_dump_reg[dump_rg_idx]) {
			pr_err("%s Unable to ioremap %d registers !\n",
				__func__, dump_rg_idx);
		}
	}
}

void mtk_imgsys_cmdq_qof_release(struct mtk_imgsys_dev *imgsys_dev, struct cmdq_client *imgsys_clt)
{
	int idx = 0;
	int dump_rg_idx = 0;

	if (!imgsys_clt) {
		cmdq_err("cl is NULL");
		dump_stack();
		return;
	}

	for (idx = IMG_GCE_THREAD_PWR_START; idx < QOF_TOTAL_THREAD; idx++) {
		cmdq_mbox_destroy(imgsys_pwr_clt[idx]);
		imgsys_pwr_clt[idx] = NULL;
	}

	for (dump_rg_idx = DUMP_RG_START; dump_rg_idx < DUMP_RG_TOTAL_NUM; dump_rg_idx++) {
		if (g_qof_dump_reg[dump_rg_idx])
			iounmap(g_qof_dump_reg[dump_rg_idx]);
	}

	cmdq_mbox_buf_free(imgsys_clt, imgsys_dev->work_buf_va, imgsys_dev->work_buf_pa);
	cmdq_mbox_buf_free(imgsys_clt, imgsys_dev->traw_work_buf_va, imgsys_dev->traw_work_buf_pa);
	cmdq_mbox_buf_free(imgsys_clt, imgsys_dev->qof_work_buf_va, imgsys_dev->qof_work_buf_pa);
}

void mtk_imgsys_cmdq_qof_streamon(struct mtk_imgsys_dev *imgsys_dev)
{
	struct qof_state *qof;
	struct qof_events *qof_event;
	u32 pwr = ISP7SP_ISP_DIP, th_id = IMG_GCE_THREAD_PWR_START;

	if (!imgsys_dev) {
		cmdq_err("imgsys_dev is NULL");
		dump_stack();
		return;
	}

	qof = (struct qof_state*)imgsys_dev->work_buf_va;
	if (qof)
		qof->user_count = 0;
	qof = (struct qof_state*)imgsys_dev->traw_work_buf_va;
	if (qof)
		qof->user_count = 0;
	qof = (struct qof_state*)imgsys_dev->qof_work_buf_va;
	if (qof)
		qof->user_count = 0;

	for (; pwr < ISP7SP_PWR_NUM && ((th_id + 1) <= IMG_GCE_THREAD_PWR_END); pwr++, th_id += 2) {
		qof_event = &qof_events_7sp[pwr];

		/* Initial power on thread */
		imgsys_cmdq_start_power_on_task(imgsys_dev, imgsys_pwr_clt[th_id], pwr, qof_event);

		/* Initial power off thread */
		imgsys_cmdq_start_power_off_task(imgsys_dev, imgsys_pwr_clt[th_id + 1], pwr, qof_event);
	}
}

void mtk_imgsys_cmdq_qof_streamoff(struct mtk_imgsys_dev *imgsys_dev)
{
	struct qof_events *qof_event;
	u32 pwr = ISP7SP_ISP_DIP, th_id = IMG_GCE_THREAD_PWR_START;
	int idx = 0;

	for (idx = IMG_GCE_THREAD_PWR_START; idx < QOF_TOTAL_THREAD; idx++) {
		cmdq_mbox_stop(imgsys_pwr_clt[idx]);
	}

	for (; pwr < ISP7SP_PWR_NUM && ((th_id + 1) <= IMG_GCE_THREAD_PWR_END); pwr++, th_id += 2) {
		qof_event = &qof_events_7sp[pwr];
		imgsys_cmdq_stop_power_tasks(imgsys_dev,
			imgsys_pwr_clt[th_id],
			imgsys_pwr_clt[th_id + 1],
			pwr);
	}
}

void mtk_imgsys_cmdq_qof_add(struct cmdq_pkt *pkt, u32 hwcomb, bool *qof_need_sub)
{
	struct qof_events *qof_event;
	u32 pwr = ISP7SP_ISP_DIP;

	if (!pkt || !qof_need_sub) {
		cmdq_err("param is NULL");
		dump_stack();
		return;
	}

	if (g_dbg_log_on == 1)
		mtk_imgsys_cmdq_qof_dump(hwcomb, false);
	for (pwr = ISP7SP_ISP_DIP; pwr < ISP7SP_PWR_NUM; pwr++) {
		if (qof_need_sub[pwr] == false) {
			if (hwcomb & pwr_group[pwr]) {
				qof_event = &qof_events_7sp[pwr];

				/* Enter critical section */
				cmdq_pkt_acquire_event(pkt, qof_event->power_ctrl);

				cmdq_pkt_set_event(pkt, qof_event->trig_pwr_on);

				/* Wait power on handshake done */
				cmdq_pkt_wfe(pkt, qof_event->pwr_hand_shake);
				cmdq_pkt_clear_event(pkt, qof_event->pwr_hand_shake);

				/* End of critical section */
				cmdq_pkt_clear_event(pkt, qof_event->power_ctrl);

				qof_need_sub[pwr] = true;
			}
		}
	}
}

void mtk_imgsys_cmdq_qof_sub(struct cmdq_pkt *pkt, bool *qof_need_sub)
{
	struct qof_events *qof_event;
	u32 pwr = ISP7SP_ISP_DIP;

	if (!pkt || !qof_need_sub) {
		cmdq_err("param is NULL");
		dump_stack();
		return;
	}

	if (g_disable_qof != 1) {
		for (pwr = ISP7SP_ISP_DIP; pwr < ISP7SP_PWR_NUM; pwr++) {
			if (qof_need_sub[pwr] == true) {
				qof_event = &qof_events_7sp[pwr];
				/* Enter critical section protected by pwr_ctrl */
				cmdq_pkt_acquire_event(pkt, qof_event->power_ctrl);

				cmdq_pkt_set_event(pkt, qof_event->trig_pwr_off);

				cmdq_pkt_wfe(pkt, qof_event->pwr_off);
				cmdq_pkt_clear_event(pkt, qof_event->pwr_off);

				/* End of critical section */
				cmdq_pkt_clear_event(pkt, qof_event->power_ctrl);
				qof_need_sub[pwr] = false;
			}
		}
	}
}

bool mtk_imgsys_cmdq_qof_get_pwr_status(u32 pwr)
{
	int cnt = 0;

	if (g_qof_ver == MTK_IMGSYS_QOF_FUNCTION_OFF){
		pr_info("[%s] qof ver = %d\n", __func__, g_qof_ver);
		return false;
	}

	if (pwr == ISP7SP_ISP_DIP)
		cnt = (g_dip_work_buf_va == NULL || *g_dip_work_buf_va == NULL) ? 0 : **((int**)g_dip_work_buf_va);
	else if (pwr == ISP7SP_ISP_TRAW)
		cnt = (g_traw_work_buf_va == NULL || *g_traw_work_buf_va == NULL) ? 0 : **((int**)g_traw_work_buf_va);
	else
		pr_info("[%s] qof pwr = %d not support\n", __func__, pwr);
	return cnt > 0;
}

void mtk_imgsys_cmdq_qof_dump(uint32_t hwcomb, bool need_dump_vcp)
{
	if (g_qof_ver == MTK_IMGSYS_QOF_FUNCTION_OFF){
		pr_info("[%s] qof ver = %d\n", __func__, g_qof_ver);
		return;
	}

	pr_info("[%s] ver:%d,qof_hwcomb=0x%x,[cnt_dip:%d/cnt_traw:%d/fp:0x%x]dip=0x%x,traw=0x%x, hwccf[vt(0x%x/0x%x),HW(0x%x,0x%x,0x%x), SWR(0x%x),MTS(0x%x/0x%x/0x%x/0x%x)]\n",
		__func__,
		g_qof_ver,
		hwcomb,
		(g_dip_work_buf_va == NULL || *g_dip_work_buf_va == NULL) ? IMG_NULL_MAGINC_NUM : **((int**)g_dip_work_buf_va),
		(g_traw_work_buf_va == NULL || *g_traw_work_buf_va == NULL) ? IMG_NULL_MAGINC_NUM : **((int**)g_traw_work_buf_va),
		(g_qof_work_buf_va == NULL || *g_qof_work_buf_va == NULL) ? IMG_NULL_MAGINC_NUM : **((unsigned int**)g_qof_work_buf_va),
		READ_QOF_RG(DUMP_RG_DIP_PWR_CON),
		READ_QOF_RG(DUMP_RG_TRAW_PWR_CON),
		/* HWCCF Info */
		READ_QOF_RG(DUMP_RG_VOTER_ON),
		READ_QOF_RG(DUMP_RG_VOTER_OFF),
		READ_QOF_RG(DUMP_RG_HWV_DONE),
		READ_QOF_RG(DUMP_RG_CCF_MTCMOS_SET_STA),
		READ_QOF_RG(DUMP_RG_CCF_MTCMOS_CLR_STA),
		READ_QOF_RG(DUMP_RG_HWV_SW_RECORD),
		READ_QOF_RG(DUMP_RG_HW_CCF_MTCMOS_STATUS),
		READ_QOF_RG(DUMP_RG_HW_CCF_MTCMOS_STATUS_CLR),
		READ_QOF_RG(DUMP_RG_HW_CCF_MTCMOS_ENABLE),
		READ_QOF_RG(DUMP_RG_CCF_INT_STATUS));

	pr_info("[%s] cg[main:0x%x,traw(0x%x,0x%x),nr(0x%x,0x%x),td0x%x,w(0x%x/0x%x/0x%x)] \n",
		/* CG Info */
		__func__,
		READ_QOF_RG(DUMP_RG_CG_IMGSYS_MAIN),
		READ_QOF_RG(DUMP_RG_CG_TRAW_DIP1),
		READ_QOF_RG(DUMP_RG_CG_TRAW_CAP_DIP),
		READ_QOF_RG(DUMP_RG_CG_DIP_NR1_DIP1),
		READ_QOF_RG(DUMP_RG_CG_DIP_NR2_DIP1),
		READ_QOF_RG(DUMP_RG_CG_DIP_TOP_DIP1),
		READ_QOF_RG(DUMP_RG_CG_WPE1_DIP1),
		READ_QOF_RG(DUMP_RG_CG_WPE2_DIP1),
		READ_QOF_RG(DUMP_RG_CG_WPE3_DIP1));

	if (need_dump_vcp == true || g_force_dump_vcp == 1)
		vcp_cmd_ex(VCP_DUMP, "imgsys_qof");
}

int mtk_query_qof_status(const char *val, const struct kernel_param *kp)
{
	int ret;
	int reserved = 0;

	ret = sscanf(val, "%d", &reserved);
	if (ret <= 0) {
		pr_err("[%s] param fail, plz check ! \n", __func__);
		return 0;
	}

 	if (g_qof_ver == MTK_IMGSYS_QOF_FUNCTION_OFF){
		pr_err("[%s] qof ver = %d\n", __func__, g_qof_ver);
		return 0;
	}

	mtk_imgsys_cmdq_qof_dump(IMG_NULL_MAGINC_NUM, false);

	return 0;
}

static struct kernel_param_ops query_qof_status_ops = {
	.set = mtk_query_qof_status,
};

module_param_cb(query_qof_status, &query_qof_status_ops, NULL, 0644);
MODULE_PARM_DESC(query_qof_status,
	"query_qof_status");

int mtk_qof_gce_set_reg(const char *val, const struct kernel_param *kp)
{
	int ret;
	struct cmdq_client *client;
	struct cmdq_pkt *gce_pkt;
	unsigned int addr = 0;
	unsigned int value = 0;
	unsigned mode = 0;

	ret = sscanf(val, "%u %u %u", &mode, &addr, &value);
	if (ret <= 0) {
		pr_err("[%s] param fail, plz check ! \n", __func__);
		return 0;
	}
	pr_info("[%s] mode =%u addr=0x%x,value=%u\n", __func__, mode, addr, value);
	client = imgsys_pwr_clt[0];
	cmdq_mbox_enable(client->chan);
	pr_info("[%s] cmdq_mbox_enable \n", __func__);
	gce_pkt = g_gce_set_rg_pkt = cmdq_pkt_create(client);
	if (!gce_pkt) {
		pr_info("%s:create cmdq package fail\n", __func__);
		return 0;
	}

	cmdq_pkt_write(gce_pkt, NULL, addr, value, 0xffffffff);
	pr_info("[%s] cmdq_pkt_write \n", __func__);
	gce_pkt->priority = IMGSYS_PRI_HIGH;
	cmdq_pkt_flush_async(gce_pkt, NULL, (void *)g_gce_set_rg_pkt);
	pr_info("[%s] cmdq_pkt_flush_async \n", __func__);
	cmdq_pkt_wait_complete(gce_pkt);
	cmdq_pkt_destroy(gce_pkt);

	return 0;
}

static struct kernel_param_ops qof_gce_set_reg_ops = {
	.set = mtk_qof_gce_set_reg,
};

module_param_cb(qof_gce_set_reg, &qof_gce_set_reg_ops, NULL, 0644);
MODULE_PARM_DESC(qof_gce_set_reg,
	"test for gce_set_reg_ops");

int mtk_qof_dbg_ctrl(const char *val, const struct kernel_param *kp)
{
	int ret;

	ret = sscanf(val, "%u %u %u", &g_dbg_log_on, &g_disable_qof, &g_force_dump_vcp);
	if (ret <= 0) {
		pr_err("[%s] param fail, plz check ! \n", __func__);
		return 0;
	}
	pr_info("[%s] g_dbg_log_on=%u g_disable_qof=%u g_force_dump_vcp=%u\n", __func__, g_dbg_log_on, g_disable_qof, g_force_dump_vcp);

	return 0;
}

static struct kernel_param_ops qof_dbg_ctrl_ops = {
	.set = mtk_qof_dbg_ctrl,
};

module_param_cb(qof_dbg_ctrl, &qof_dbg_ctrl_ops, NULL, 0644);
MODULE_PARM_DESC(qof_dbg_ctrl,
	"qof_dbg_ctrl");
