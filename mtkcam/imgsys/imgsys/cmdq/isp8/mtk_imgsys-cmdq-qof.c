/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 *
 * Author: Yuan-Jung Kuo <yuan-jung.kuo@mediatek.com>
 *          Nick.wen <nick.wen@mediatek.com>
 *
 */
#include <linux/pm_runtime.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/iopoll.h>
#include <linux/of_device.h>
#include <linux/soc/mediatek/mtk-cmdq-ext.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include "vcp_status.h"
#include "mtk_imgsys-engine-isp8.h"
#include "mtk_imgsys-cmdq.h"
#include "mtk_imgsys-cmdq-plat.h"
#include "mtk_imgsys-cmdq-qof-data.h"
#include "mtk-smi-dbg.h"
#include "mtk_imgsys-trace.h"

/* HW event: restore event for qof */
#define CMDQ_EVENT_IMG_QOF_RESTORE_EVENT_0_DIP			(312)
#define CMDQ_EVENT_IMG_QOF_RESTORE_EVENT_1_TRAW			(313)
#define CMDQ_EVENT_IMG_QOF_RESTORE_EVENT_2_WPE_EIS		(314)
#define CMDQ_EVENT_IMG_QOF_RESTORE_EVENT_3_WPE_TNR		(315)
#define CMDQ_EVENT_IMG_QOF_RESTORE_EVENT_4_LITE			(316)
/* SW event: sw event for qof */
#define CMDQ_SW_EVENT_QOF_DIP							(862)
#define CMDQ_SW_EVENT_QOF_TRAW							(863)
#define CMDQ_SW_EVENT_QOF_WPE_EIS						(864)
#define CMDQ_SW_EVENT_QOF_WPE_TNR						(865)
#define CMDQ_SW_EVENT_QOF_LITE							(866)
#define CMDQ_SW_EVENT_QOF_SMI_CB_HSK					(867)
#define CMDQ_SW_EVENT_QOF_SMI_SW_EVENT					(868)

/* delay cnt */
#define IMG_HWV_DELAY_CNT								(0xFFFF)
#define IMG_CG_UNGATING_DELAY_CNT						(0xFFFF)
#define IMG_MTCMOS_STABLE_CNT							(0xFFFF)
#define INT_STATUS_POLL_TIMES							(20)
#define INT_STATUS_POLL_US								(50000)
#define POLL_DELAY_US									(1)
#define TIMEOUT_500US									(500)
#define TIMEOUT_1000US									(1000)	// = 1ms
#define TIMEOUT_2000US									(2000)	// = 2ms
#define TIMEOUT_100000US								(100000)
/* others */
#define MOD_BIT_OFST									(3)
#define QOF_SUPPORT_SMI_GCE_CALLBACK					(1)
#define QOF_ERROR_CODE									(0xdead)
/* func */
#define IS_MOD_SUPPORT_QOF(m)		(((m < QOF_TOTAL_MODULE)) && ((g_qof_ver & BIT(m)) == BIT(m)))
#define IS_CON_PWR_ON(val)			((val & (BIT(30)|BIT(31))) == (BIT(30)|BIT(31)) ? TRUE:FALSE)
#define QOF_GET_REMAP_ADDR(addr)	(g_maped_rg[MAPED_RG_QOF_REG_BASE] + (addr - QOF_REG_BASE))
#define QOF_LOGI(fmt, args...)		pr_info("[QOF_LOGI]%s:%d " fmt "\n", __func__, __LINE__, ##args)
#define QOF_LOGE(fmt, args...)		pr_err("[QOF_ERROR]%s:%d " fmt "\n", __func__, __LINE__, ##args)
#define write_mask(addr, val, mask)\
	do {\
		unsigned int tmp = readl(addr) & ~(mask);\
		tmp = tmp | ((val) & mask);\
		writel(tmp, addr);\
	} while(0)

/* common params */
static u32 imgsys_voter_cnt_locked;
static u32 g_qof_ver;
static u32 g_ftrace_time;
struct mtk_imgsys_dev *g_imgsys_dev;
static void __iomem *g_maped_rg[MAPED_RG_LIST_NUM] = {0};
static struct qof_events qof_events_isp8[ISP8_PWR_NUM];
static struct cmdq_client *imgsys_pwr_clt[QOF_TOTAL_THREAD] = {NULL};
static struct cmdq_client *smi_cb_pwr_ctl;
static struct cmdq_pkt *g_smi_cb_pkt;
static unsigned int **g_qof_work_buf_va;
spinlock_t qof_lock;
static bool is_smi_use_qof_locked[ISP8_PWR_NUM] = {false};
static bool is_poll_event_mode;

/* dbg params*/
static bool imgsys_ftrace_qof_thread_en;
static int g_qof_debug_level;

module_param(imgsys_ftrace_qof_thread_en, bool, 0644);
MODULE_PARM_DESC(imgsys_ftrace_qof_thread_en, "imgsys ftrace thread enable, 0 (default)");

enum QOF_CNT_CTRL {
	QOF_CNT_CTRL_ADD,
	QOF_CNT_CTRL_SUB,
	QOF_CNT_CTRL_MAX,
};

enum QOF_DEBUG_MODE {
	QOF_DEBUG_MODE_PERFRAME_DUMP = 1,
	QOF_DEBUG_MODE_IMMEDIATE_DUMP = 2,
	QOF_DEBUG_MODE_IMMEDIATE_CG_DUMP = 3,
	QOF_DEBUG_MODE_FORCE_SMI_EVENT_POLL = 4,
};

enum QOF_GCE_THREAD_LIST {
	QOF_GCE_THREAD_DIP = IMGSYS_NOR_THD,
	QOF_GCE_THREAD_TRAW,
	QOF_GCE_THREAD_WPE12,
	QOF_GCE_THREAD_WPE3,
	QOF_GCE_THREAD_SMI_CB,
};

u32 sw_event_lock_list[ISP8_PWR_NUM] = {
	CMDQ_SW_EVENT_QOF_DIP,
	CMDQ_SW_EVENT_QOF_TRAW,
	CMDQ_SW_EVENT_QOF_WPE_EIS,
	CMDQ_SW_EVENT_QOF_WPE_TNR,
	CMDQ_SW_EVENT_QOF_LITE,
};

u32 hw_event_restore_list[ISP8_PWR_NUM] = {
	CMDQ_EVENT_IMG_QOF_RESTORE_EVENT_0_DIP,
	CMDQ_EVENT_IMG_QOF_RESTORE_EVENT_1_TRAW,
	CMDQ_EVENT_IMG_QOF_RESTORE_EVENT_2_WPE_EIS,
	CMDQ_EVENT_IMG_QOF_RESTORE_EVENT_3_WPE_TNR,
	CMDQ_EVENT_IMG_QOF_RESTORE_EVENT_4_LITE,
};

struct cmdq_pwr_buf {
	struct cmdq_pkt *restore_pkt;
};

static struct cmdq_pwr_buf pwr_buf_handle[QOF_TOTAL_THREAD] = {
	{
		.restore_pkt = NULL,
	},
	{
		.restore_pkt = NULL,
	},
	{
		.restore_pkt = NULL,
	},
	{
		.restore_pkt = NULL,
	},
	{
		.restore_pkt = NULL,
	},
};

/* mtcmos data */
static void qof_locked_stream_off_sync(void);
static void qof_locked_set_engine_off(const u32 mod);
static bool qof_poll_pwr_status(u32 pwr, bool enable);
static void imgsys_cmdq_qof_set_restore_done(struct cmdq_pkt *pkt, u32 pwr_id);
static void mtk_imgsys_set_dip_larb_golden(struct cmdq_pkt *pkt);
static void mtk_imgsys_set_wpe_eis_larb_golden(struct cmdq_pkt *pkt);
static void mtk_imgsys_set_wpe_tnr_larb_golden(struct cmdq_pkt *pkt);
static void mtk_imgsys_set_wpe_lite_larb_golden(struct cmdq_pkt *pkt);
static void mtk_imgsys_set_traw_larb_golden(struct cmdq_pkt *pkt);
static void imgsys_qof_traw_direct_link_reset(struct cmdq_pkt *pkt);
static void imgsys_cmdq_dip_cg_unating(struct cmdq_pkt *pkt,
		const struct imgsys_cg_data *cg, dma_addr_t qof_work_buf_pa);
static void imgsys_cmdq_traw_cg_unating(struct cmdq_pkt *pkt,
		const struct imgsys_cg_data *cg, dma_addr_t qof_work_buf_pa);
static void imgsys_cmdq_wpe1_cg_unating(struct cmdq_pkt *pkt,
		const struct imgsys_cg_data *cg, dma_addr_t qof_work_buf_pa);
static void imgsys_cmdq_wpe2_cg_unating(struct cmdq_pkt *pkt,
		const struct imgsys_cg_data *cg, dma_addr_t qof_work_buf_pa);
static void imgsys_cmdq_wpe3_cg_unating(struct cmdq_pkt *pkt,
		const struct imgsys_cg_data *cg, dma_addr_t qof_work_buf_pa);
static void qof_module_vote_add(struct cmdq_pkt *pkt, u32 pwr, u32 user);
static void qof_module_vote_sub(struct cmdq_pkt *pkt, u32 pwr, u32 user);
static void qof_stop_all_gce_loop(void);
static void imgsys_qof_set_dbg_thread(bool enable);
static struct imgsys_mtcmos_data isp8_module_data[] = {
	[ISP8_PWR_DIP] = {
		.pwr_id = ISP8_PWR_DIP,
		.cg_ungating = imgsys_cmdq_dip_cg_unating,
		.cg_data = &common_cg_data,
		.set_larb_golden = mtk_imgsys_set_dip_larb_golden,
		//.direct_link_reset = mtk_imgsys_dip_direct_link_reset,
		.qof_restore_done = imgsys_cmdq_qof_set_restore_done,
	},
	[ISP8_PWR_TRAW] = {
		.pwr_id = ISP8_PWR_TRAW,
		.cg_ungating = imgsys_cmdq_traw_cg_unating,
		.cg_data = &common_cg_data,
		.set_larb_golden = mtk_imgsys_set_traw_larb_golden,
		.direct_link_reset = imgsys_qof_traw_direct_link_reset,
		.qof_restore_done = imgsys_cmdq_qof_set_restore_done,
	},
	[ISP8_PWR_WPE_1_EIS] = {
		.pwr_id = ISP8_PWR_WPE_1_EIS,
		.cg_ungating = imgsys_cmdq_wpe1_cg_unating,
		.cg_data = &common_cg_data,
		.set_larb_golden = mtk_imgsys_set_wpe_eis_larb_golden,
		//.direct_link_reset = mtk_imgsys_wpe_1_eis_direct_link_reset,
		.qof_restore_done = imgsys_cmdq_qof_set_restore_done,
	},
	[ISP8_PWR_WPE_2_TNR] = {
		.pwr_id = ISP8_PWR_WPE_2_TNR,
		.cg_ungating = imgsys_cmdq_wpe2_cg_unating,
		.cg_data = &common_cg_data,
		.set_larb_golden = mtk_imgsys_set_wpe_tnr_larb_golden,
		//.direct_link_reset = mtk_imgsys_wpe_2_tnr_direct_link_reset,
		.qof_restore_done = imgsys_cmdq_qof_set_restore_done,
	},
	[ISP8_PWR_WPE_3_LITE] = {
		.pwr_id = ISP8_PWR_WPE_3_LITE,
		.cg_ungating = imgsys_cmdq_wpe3_cg_unating,
		.cg_data = &common_cg_data,
		.set_larb_golden = mtk_imgsys_set_wpe_lite_larb_golden,
		//.direct_link_reset = mtk_imgsys_wpe_3_lite_direct_link_reset,
		.qof_restore_done = imgsys_cmdq_qof_set_restore_done,
	},
};

static unsigned int get_thread_id_by_pwr_id(u32 pwr)
{
	u32 th_id = IMG_GCE_THREAD_PWR_START;

	switch(pwr) {
	case ISP8_PWR_DIP:
		th_id = IMG_GCE_THREAD_DIP;
		break;
	case ISP8_PWR_TRAW:
		th_id = IMG_GCE_THREAD_TRAW;
		break;
	case ISP8_PWR_WPE_1_EIS:
		th_id = IMG_GCE_THREAD_WPE_12;
		break;
	case ISP8_PWR_WPE_2_TNR:
		th_id = IMG_GCE_THREAD_WPE_12;
		break;
	case ISP8_PWR_WPE_3_LITE:
		th_id = IMG_GCE_THREAD_WPE_3_LITE;
		break;
	default:
		QOF_LOGI("qof pwr[%u] is not mapping\n", pwr);
		break;
	}
	return th_id;
}

static void qof_dump_gce_loop_thread(u32 pwr)
{
	struct cmdq_instruction *inst = NULL;
	dma_addr_t pc = 0;
	u32 th_id = 0;

	cmdq_thread_dump(smi_cb_pwr_ctl->chan, g_smi_cb_pkt, (u64 **)&inst, &pc);

	th_id = get_thread_id_by_pwr_id(pwr);
	cmdq_thread_dump(imgsys_pwr_clt[th_id]->chan, pwr_buf_handle[th_id].restore_pkt, (u64 **)&inst, &pc);
}

bool is_qof_engine_enabled(int mod)
{
	int bit_mod = mod + MOD_BIT_OFST;
	int addr = qof_reg_table[ISP8_PWR_DIP][QOF_IMG_QOF_ENG_EN].addr;

	return (readl(QOF_GET_REMAP_ADDR(addr)) & BIT(bit_mod)) == BIT(bit_mod);
}

bool check_cg_status(u32 cg_reg_idx, u32 val)
{
	if((readl(g_maped_rg[cg_reg_idx]) & val) != 0) {
		QOF_LOGE("g_maped_rg[%d] CG not ungate(0x%x)!!!\n", cg_reg_idx, readl(g_maped_rg[cg_reg_idx]));
		mtk_imgsys_cmdq_qof_dump(0, false);
		return false;
	}
	return true;
}

bool qof_check_module_cg_status(u32 pwr)
{
	unsigned int val;
	void __iomem *addr;

	if ((readl(g_maped_rg[MAPED_RG_ISP_MAIN_PWR_CON]) & (BIT(30)|BIT(31)))
		!= (BIT(30)|BIT(31))) {
		QOF_LOGI("mod[%u] isp_main is off. sta=0x%x\n",
			pwr, readl(g_maped_rg[MAPED_RG_ISP_MAIN_PWR_CON]));
		return false;
	}
	addr = QOF_GET_REMAP_ADDR(qof_reg_table[pwr][QOF_IMG_QOF_STATE_DBG].addr);
	if ((readl(addr) & BIT(1)) != BIT(1)) {
		QOF_LOGI("qof[%u] mtcmos is off. sta=0x%x\n", pwr, readl(addr));
		return false;
	}

	switch(pwr) {
	case ISP8_PWR_DIP:
		val = BIT(0)|BIT(1);
		if (!check_cg_status(MAPED_RG_IMG_CG_DIP_NR1_DIP1, val))
			return false;
		val = BIT(0)|BIT(1)|BIT(2);
		if (!check_cg_status(MAPED_RG_IMG_CG_DIP_NR2_DIP1, val))
			return false;
		val = BIT(0)|BIT(1)|BIT(2)|BIT(3)|BIT(4)|BIT(5)|BIT(6)|BIT(7)|BIT(8);
		if (!check_cg_status(MAPED_RG_IMG_CG_DIP_TOP_DIP1, val))
			return false;
		break;
	case ISP8_PWR_TRAW:
		val = BIT(0);
		if (!check_cg_status(MAPED_RG_IMG_CG_TRAW_CAP_DIP1, val))
			return false;
		val = BIT(0)|BIT(1)|BIT(2)|BIT(3);
		if (!check_cg_status(MAPED_RG_IMG_CG_TRAW_DIP1, val))
			return false;
		break;
	case ISP8_PWR_WPE_1_EIS:
		val = BIT(0)|BIT(1)|BIT(2);
		if (!check_cg_status(MAPED_RG_IMG_CG_WPE1_DIP1, val))
			return false;
		break;
	case ISP8_PWR_WPE_2_TNR:
		val = BIT(0)|BIT(1)|BIT(2);
		if (!check_cg_status(MAPED_RG_IMG_CG_WPE2_DIP1, val))
			return false;
		break;
	case ISP8_PWR_WPE_3_LITE:
		val = BIT(0)|BIT(1)|BIT(2);
		if (!check_cg_status(MAPED_RG_IMG_CG_WPE3_DIP1, val))
			return false;
		break;
	default:
		break;
	}
	return true;
}

void imgsys_cmdq_smi_cb_pwr_ctrl_locked(struct mtk_imgsys_dev *imgsys_dev,
	struct cmdq_pkt *pkt, dma_addr_t work_buf_pa)
{
	struct cmdq_operand lop, rop;
	/* spr is gce per-thread internal 32bit ram, use as variable */
	const u16 reg_jump = CMDQ_THR_SPR_IDX1;
	const u16 var1 = CMDQ_THR_SPR_IDX2;
	dma_addr_t user_count_pa = work_buf_pa;
	u32 inst_condi_jump, inst_jump_end;
	bool qof_need_sub[ISP8_PWR_NUM] = {0};
	u64 *inst, jump_pa;
	u32 i = 0;

	/* read data from some pa to spr for compare later */
	cmdq_pkt_read(pkt, NULL, user_count_pa, var1);
	lop.reg = true;
	lop.idx = var1;
	rop.reg = false;
	rop.value = 1;
	/* mark condition jump and change offset later */
	inst_condi_jump = pkt->cmd_buf_size;
	cmdq_pkt_assign_command(pkt, reg_jump, 0);

	/* case: counter != 1 */
	cmdq_pkt_cond_jump_abs(pkt, reg_jump, &lop, &rop, CMDQ_EQUAL);
	/* now fill in code that condition not match, here is nop jump */
	for(i=0;i<ISP8_PWR_NUM;i++)
		qof_need_sub[i] = true;

	cmdq_pkt_write(pkt, NULL, QOF_SPARE_REG_FOOTPRINT,
		BIT(QOF_FOOTPRINT_BIT_BEFORE_SUB), BIT(QOF_FOOTPRINT_BIT_BEFORE_SUB));
	mtk_imgsys_cmdq_qof_sub(pkt, qof_need_sub);
	cmdq_pkt_write(pkt, NULL, QOF_SPARE_REG_FOOTPRINT,
		BIT(QOF_FOOTPRINT_BIT_AFTER_SUB), BIT(QOF_FOOTPRINT_BIT_AFTER_SUB));

	inst_jump_end = pkt->cmd_buf_size;
	/* Finish else statement, jump to the end of if-else braces. */
	/* Assign jump address as zero initially and we will modify it later. */
	cmdq_pkt_jump_addr(pkt, 0);

	/* following instructinos is condition TRUE, */
	/* thus conditional jump should jump current offset */
	inst = cmdq_pkt_get_va_by_offset(pkt, inst_condi_jump);
	jump_pa = cmdq_pkt_get_pa_by_offset(pkt, pkt->cmd_buf_size);
	if (inst == NULL) {
		QOF_LOGE("param inst is null\n");
		return;
	}
	*inst = *inst & ((u64)0xFFFFFFFF << 32);
	*inst = *inst | CMDQ_REG_SHIFT_ADDR(jump_pa);

	/* now fill in code that condition match */
	/* case: counter == 1 */
	for(i=0;i<ISP8_PWR_NUM;i++)
		qof_need_sub[i] = false;
	cmdq_pkt_write(pkt, NULL, QOF_SPARE_REG_FOOTPRINT,
		BIT(QOF_FOOTPRINT_BIT_BEFORE_ADD), BIT(QOF_FOOTPRINT_BIT_BEFORE_ADD));
	mtk_imgsys_cmdq_qof_add(pkt, qof_need_sub, 0xffffffff);
	cmdq_pkt_write(pkt, NULL, QOF_SPARE_REG_FOOTPRINT,
		BIT(QOF_FOOTPRINT_BIT_AFTER_ADD), BIT(QOF_FOOTPRINT_BIT_AFTER_ADD));

	/* this is the end of whole condition, thus condition FALSE part should jump here */
	jump_pa = cmdq_pkt_get_pa_by_offset(pkt, pkt->cmd_buf_size);
	inst = cmdq_pkt_get_va_by_offset(pkt, inst_jump_end);
	if (inst == NULL) {
		QOF_LOGE("param inst is null\n");
		return;
	}
	*inst = *inst & ((u64)0xFFFFFFFF << 32);
	*inst = *inst | CMDQ_REG_SHIFT_ADDR(jump_pa);
}

static void qof_start_smi_cb_loop(struct mtk_imgsys_dev *imgsys_dev,
		struct cmdq_client *client)
{
	struct cmdq_pkt *smi_cb_pkt;
	dma_addr_t work_buf_pa;

	/* Power on kernel thread */
	cmdq_mbox_enable(client->chan);

	smi_cb_pkt = g_smi_cb_pkt = cmdq_pkt_create(client);
	if (!smi_cb_pkt) {
		pr_info("%s:create cmdq package fail\n", __func__);
			return;
	}

	/* Program start */
	/* Wait for power on event */
	cmdq_pkt_wfe(smi_cb_pkt, CMDQ_SW_EVENT_QOF_SMI_SW_EVENT);
	cmdq_pkt_write(smi_cb_pkt, NULL, QOF_SPARE_REG_FOOTPRINT,
		BIT(QOF_FOOTPRINT_BIT_GET_SMI_EVENT), BIT(QOF_FOOTPRINT_BIT_GET_SMI_EVENT));

	/* do pwr ctrl */
	work_buf_pa = imgsys_dev->work_buf_pa;
	imgsys_cmdq_smi_cb_pwr_ctrl_locked(imgsys_dev, smi_cb_pkt,
			work_buf_pa);
	cmdq_pkt_write(smi_cb_pkt, NULL, QOF_SPARE_REG_FOOTPRINT,
		BIT(QOF_FOOTPRINT_BIT_FINISH_LOGIC), BIT(QOF_FOOTPRINT_BIT_FINISH_LOGIC));

	/* MTCMOS is power on, notify caller thread */
	cmdq_pkt_set_event(smi_cb_pkt, CMDQ_SW_EVENT_QOF_SMI_CB_HSK);
	cmdq_pkt_write(smi_cb_pkt, NULL, QOF_SPARE_REG_FOOTPRINT,
		BIT(QOF_FOOTPRINT_BIT_HSK_DONE), BIT(QOF_FOOTPRINT_BIT_HSK_DONE));

	/* Program end */
	smi_cb_pkt->priority = IMGSYS_PRI_HIGH;
	smi_cb_pkt->no_irq = true;
	cmdq_pkt_finalize_loop(smi_cb_pkt);
	//cmdq_dump_pkt(smi_cb_pkt, 0, true);
	cmdq_pkt_flush_async(smi_cb_pkt, NULL, (void *)smi_cb_pkt);
}

bool qof_poll_smi_hsk_timeout(u64 timeout_time)
{
	u64 time_start = 0, time_cur = 0; /* unit is us */
	struct cmdq_client *pwr_clt = smi_cb_pwr_ctl;
	unsigned int last_reg = 0, cur_reg = 0;

	if (pwr_clt == NULL) {
		QOF_LOGE("pwr_clt is null\n");
		return false;
	}
	time_start = ktime_get_boottime_ns()/1000;

	if (is_poll_event_mode) {
		while (cmdq_get_event(pwr_clt->chan, CMDQ_SW_EVENT_QOF_SMI_CB_HSK) == 0) {
			time_cur = ktime_get_boottime_ns()/1000;
			if (time_cur < time_start) {
				time_start = time_cur;
				QOF_LOGE("timer start change time_start= %llu", time_start);
			} else if ((time_cur - time_start) > timeout_time) {
				QOF_LOGE("timeout(%llu us)! smi waiting event  HSK=%u", time_cur - time_start,
					cmdq_get_event(pwr_clt->chan, CMDQ_SW_EVENT_QOF_SMI_CB_HSK));
				mtk_imgsys_cmdq_qof_dump(0, false);
				return false;
			}
		}
	} else {
		cur_reg = readl(QOF_GET_REMAP_ADDR(QOF_SPARE_REG_FOOTPRINT));
		QOF_LOGI("poll smi_gce start, cur_reg=0x%x", cur_reg);
		while ((cur_reg & BIT(QOF_FOOTPRINT_BIT_HSK_DONE)) == 0) {
			time_cur = ktime_get_boottime_ns()/1000;
			if((g_qof_debug_level == QOF_DEBUG_MODE_PERFRAME_DUMP) &&
				(cur_reg != last_reg)){
				QOF_LOGI("footpring polling mode, last=0x%x, cur=0x%x", last_reg, cur_reg);
				last_reg = cur_reg;
			}
			if (time_cur < time_start) {
				time_start = time_cur;
				QOF_LOGE("timer start change time_start= %llu", time_start);
			} else if ((time_cur - time_start) > timeout_time) {
				QOF_LOGE("timeout(%llu us)! smi waiting event  HSK=%u, cur_reg=0x%x",
					time_cur - time_start,
					cmdq_get_event(pwr_clt->chan, CMDQ_SW_EVENT_QOF_SMI_CB_HSK), cur_reg);
				mtk_imgsys_cmdq_qof_dump(0, false);
				return false;
			}
			cur_reg = readl(QOF_GET_REMAP_ADDR(QOF_SPARE_REG_FOOTPRINT));
		}
		QOF_LOGI("poll smi_gce done, total_time=%llu(us),timeout_time=%llu(us) cur_reg=0x%x, smi_sw_event=%d",
			time_cur - time_start, timeout_time, cur_reg,
			cmdq_get_event(pwr_clt->chan, CMDQ_SW_EVENT_QOF_SMI_SW_EVENT));
	}
	return true;
}

static bool qof_smi_counter_ctrl(enum QOF_CNT_CTRL ctrl)
{
	bool ret = true;

	if ((g_qof_work_buf_va == NULL) || (*g_qof_work_buf_va == NULL) || ctrl >= QOF_CNT_CTRL_MAX) {
		QOF_LOGE("smi get_if_in_use hang!!!! param wrong[%u]\n",
			(g_qof_work_buf_va == NULL));
		return false;
	}

	switch(ctrl) {
	case QOF_CNT_CTRL_ADD:
		(**((unsigned int **)g_qof_work_buf_va))++;
		break;
	case QOF_CNT_CTRL_SUB:
		if (**((unsigned int **)g_qof_work_buf_va) == 0) {
			QOF_LOGI("work buf cnt is 0, no need to sub\n");
			ret = false;
		} else
			(**((unsigned int **)g_qof_work_buf_va))--;
		break;
	default:
		QOF_LOGI("ctrl id is wrong [%u]\n", ctrl);
		ret = false;
		break;
	}

	return ret;
}

bool qof_smi_cb_power_on(void)
{
	struct cmdq_client *pwr_clt = smi_cb_pwr_ctl;
	bool ret = true;

	if ((g_qof_work_buf_va == NULL) || (*g_qof_work_buf_va == NULL) || (pwr_clt == NULL)) {
		QOF_LOGE("smi get_if_in_use hang!!!! param wrong[%u/%u]\n",
			(g_qof_work_buf_va == NULL),
			(pwr_clt == NULL));
		return false;
	}
	if (**((unsigned int **)g_qof_work_buf_va) == 1) {
		cmdq_set_event(pwr_clt->chan, CMDQ_SW_EVENT_QOF_SMI_SW_EVENT);
		ret = qof_poll_smi_hsk_timeout(TIMEOUT_2000US);
		cmdq_clear_event(pwr_clt->chan, CMDQ_SW_EVENT_QOF_SMI_CB_HSK);
		write_mask(QOF_GET_REMAP_ADDR(QOF_SPARE_REG_FOOTPRINT), 0, 0xffffffff);
	}
	QOF_LOGI("mem_cnt=%u, HANDSHAKE=%u, ret=%u",
		**((unsigned int **)g_qof_work_buf_va),
		cmdq_get_event(pwr_clt->chan, CMDQ_SW_EVENT_QOF_SMI_CB_HSK), ret);
	return ret;
}

bool qof_smi_cb_power_off(u32 user)
{
	struct cmdq_client *pwr_clt = smi_cb_pwr_ctl;
	bool ret = true;
	u32 pwr = 0;

	if ((g_qof_work_buf_va == NULL) || (*g_qof_work_buf_va == NULL) || (pwr_clt == NULL)) {
		QOF_LOGE("smi get_if_in_use hang!!!! param wrong[%u/%u]\n",
			(g_qof_work_buf_va == NULL),
			(pwr_clt == NULL));
		return false;
	}

	if (**((unsigned int **)g_qof_work_buf_va) == 0) {
		QOF_LOGI("work buf cnt is 0, no need to sub\n");
		return true;
	}

	qof_smi_counter_ctrl(QOF_CNT_CTRL_SUB);
	if (**((unsigned int **)g_qof_work_buf_va) == 0) {
		switch(user) {
		case QOF_USER_AP:
			for (pwr = ISP8_PWR_START; pwr < ISP8_PWR_NUM; pwr++)
				qof_module_vote_sub(NULL, pwr, QOF_USER_AP);
			break;
		case QOF_USER_GCE:
			cmdq_set_event(pwr_clt->chan, CMDQ_SW_EVENT_QOF_SMI_SW_EVENT);
			ret = qof_poll_smi_hsk_timeout(TIMEOUT_2000US);
			cmdq_clear_event(pwr_clt->chan, CMDQ_SW_EVENT_QOF_SMI_CB_HSK);
			write_mask(QOF_GET_REMAP_ADDR(QOF_SPARE_REG_FOOTPRINT), 0, 0xffffffff);
			break;
		default:
			break;
		}
	}
	return ret;
}

static int qof_smi_isp_module_get_if_in_use(void *data, int module)
{
	int get_result = -1;
	int pm_res = -1;
	unsigned long flag;
	bool smi_use_qof = false;

	if (data == NULL) {
		QOF_LOGE("qof get if in use data is null\n");
		return -1;
	}

	if (g_qof_ver == 0)
		return 1;
	spin_lock_irqsave(&qof_lock, flag);

	if (imgsys_voter_cnt_locked == 0) {
		// pm get power fail
		smi_use_qof = false;
		get_result = -1;
		goto RETURN_FLOW;
	} else {
		imgsys_voter_cnt_locked++;
		pm_res = pm_runtime_get_if_in_use(g_imgsys_dev->dev);
		if (pm_res <= 0) {
			// pm get power fail
			smi_use_qof = false;
			get_result = -1;
			goto RETURN_FLOW;
		} else {
			if (is_qof_engine_enabled(module)) {
				if ((qof_smi_counter_ctrl(QOF_CNT_CTRL_ADD) == false) ||
				(qof_smi_cb_power_on() == false) ||
				(qof_poll_pwr_status(module, true) == false) ||
				(qof_check_module_cg_status(module) == false)) {
					// qof get power fail
					QOF_LOGE("qof get if in use fail\n");
					qof_dump_gce_loop_thread(module);
					mtk_imgsys_cmdq_qof_dump(0, false);
					qof_smi_counter_ctrl(QOF_CNT_CTRL_SUB);
					smi_use_qof = false;
					get_result = -1;
					pm_runtime_put(g_imgsys_dev->dev);
					goto RETURN_FLOW;
				} else {
					// qof get power success
					smi_use_qof = true;
					get_result = 1;
					goto RETURN_FLOW;
				}
			} else {
				// pm get power success
				smi_use_qof = false;
				get_result = 1;
				goto RETURN_FLOW;
			}
		}
	}

RETURN_FLOW:
	*(bool *)data = smi_use_qof;
	QOF_LOGI("get_result=%d, pm_res=%d, smi_use_qof=%u, mem_cnt=%u, MAIN[0x%x], VCORE[0x%x]\n",
		get_result,
		pm_res,
		*(bool *)data,
		((g_qof_work_buf_va == NULL) || (*g_qof_work_buf_va == NULL)) ?
			QOF_ERROR_CODE : **((unsigned int **)g_qof_work_buf_va),
		(readl(g_maped_rg[MAPED_RG_ISP_MAIN_PWR_CON])),
		(readl(g_maped_rg[MAPED_RG_ISP_VCORE_PWR_CON])));
	spin_unlock_irqrestore(&qof_lock, flag);
	return get_result;
}

static int qof_smi_isp_module_get(void *data, int module)
{
	unsigned long flag;

	if (g_qof_ver == 0)
		return 1;
	/* get is for smi force_all_on dbg mode use */
	if (imgsys_voter_cnt_locked == 0) {
		QOF_LOGI("Staus occur before stream on, turn off qof\n");
		pm_runtime_get_noresume(g_imgsys_dev->dev);
		*(bool *)data = false;
		g_qof_ver = 0;
	} else {
		spin_lock_irqsave(&qof_lock, flag);
		imgsys_voter_cnt_locked++;
		spin_unlock_irqrestore(&qof_lock, flag);

		if (qof_smi_isp_module_get_if_in_use(data, module) == -1)
			QOF_LOGE("qof get fail\n");

		spin_lock_irqsave(&qof_lock, flag);
		imgsys_voter_cnt_locked--;
		spin_unlock_irqrestore(&qof_lock, flag);
	}

	return 0;
}

static int qof_smi_isp_module_put(void *data, int module)
{
	unsigned long flag;
	bool smi_use_qof;

	if (g_qof_ver == 0)
		return 1;
	spin_lock_irqsave(&qof_lock, flag);
	if (data == NULL) {
		QOF_LOGE("data is null,set to default\n");
		smi_use_qof = false;
	} else
		smi_use_qof = *((bool *)data);

	if (imgsys_voter_cnt_locked == 0)
		QOF_LOGE("imgsys_voter_cnt_locked is 0\n");
	else {
		imgsys_voter_cnt_locked--;
		if (is_qof_engine_enabled(module)) {
			// put before stream_off(qof_engine_off), use qof gce sub(now is qof mode)
			if (smi_use_qof)
				qof_smi_cb_power_off(QOF_USER_GCE);
		} else {
			// put after stream_off(qof_engine_off), gce thread stop, use qof ap sub(now is pm mode)
			if (smi_use_qof)
				qof_smi_cb_power_off(QOF_USER_AP);
		}

		pm_runtime_put(g_imgsys_dev->dev);
	}
	QOF_LOGI("smi_use_qof=%u, mem_cnt=%u, MAIN[0x%x], VCORE[0x%x]\n",
		smi_use_qof,
		((g_qof_work_buf_va == NULL) || (*g_qof_work_buf_va == NULL)) ?
			QOF_ERROR_CODE : **((unsigned int **)g_qof_work_buf_va),
		(readl(g_maped_rg[MAPED_RG_ISP_MAIN_PWR_CON])),
		(readl(g_maped_rg[MAPED_RG_ISP_VCORE_PWR_CON])));
	spin_unlock_irqrestore(&qof_lock, flag);

	return 0;
}

/* SMI DIP CB */
static int smi_isp_dip_get(void *data)
{
	return qof_smi_isp_module_get(data, ISP8_PWR_DIP);
}

int smi_isp_dip_get_if_in_use(void *data)
{
	return qof_smi_isp_module_get_if_in_use(data, ISP8_PWR_DIP);
}
EXPORT_SYMBOL(smi_isp_dip_get_if_in_use);

int smi_isp_dip_put(void *data)
{
	return qof_smi_isp_module_put(data, ISP8_PWR_DIP);
}
EXPORT_SYMBOL(smi_isp_dip_put);

/* SMI TRAW CB */
static int smi_isp_traw_get(void *data)
{
	return qof_smi_isp_module_get(data, ISP8_PWR_TRAW);
}

int smi_isp_traw_get_if_in_use(void *data)
{
	return qof_smi_isp_module_get_if_in_use(data, ISP8_PWR_TRAW);
}
EXPORT_SYMBOL(smi_isp_traw_get_if_in_use);

int smi_isp_traw_put(void *data)
{
	return qof_smi_isp_module_put(data, ISP8_PWR_TRAW);
}
EXPORT_SYMBOL(smi_isp_traw_put);

/* SMI WPE1 CB */
static int smi_isp_wpe1_eis_get(void *data)
{
	return qof_smi_isp_module_get(data, ISP8_PWR_WPE_1_EIS);
}

int smi_isp_wpe1_eis_get_if_in_use(void *data)
{
	return qof_smi_isp_module_get_if_in_use(data, ISP8_PWR_WPE_1_EIS);
}
EXPORT_SYMBOL(smi_isp_wpe1_eis_get_if_in_use);

int smi_isp_wpe1_eis_put(void *data)
{
	return qof_smi_isp_module_put(data, ISP8_PWR_WPE_1_EIS);
}
EXPORT_SYMBOL(smi_isp_wpe1_eis_put);

/* SMI WPE2 CB */
static int smi_isp_wpe2_tnr_get(void *data)
{
	return qof_smi_isp_module_get(data, ISP8_PWR_WPE_2_TNR);
}

int smi_isp_wpe2_tnr_get_if_in_use(void *data)
{
	return qof_smi_isp_module_get_if_in_use(data, ISP8_PWR_WPE_2_TNR);
}
EXPORT_SYMBOL(smi_isp_wpe2_tnr_get_if_in_use);

int smi_isp_wpe2_tnr_put(void *data)
{
	return qof_smi_isp_module_put(data, ISP8_PWR_WPE_2_TNR);
}
EXPORT_SYMBOL(smi_isp_wpe2_tnr_put);

/* SMI WPE3 CB */
static int smi_isp_wpe3_lite_get(void *data)
{
	return qof_smi_isp_module_get(data, ISP8_PWR_WPE_3_LITE);
}

int smi_isp_wpe3_lite_get_if_in_use(void *data)
{
	return qof_smi_isp_module_get_if_in_use(data, ISP8_PWR_WPE_3_LITE);
}
EXPORT_SYMBOL(smi_isp_wpe3_lite_get_if_in_use);

int smi_isp_wpe3_lite_put(void *data)
{
	return qof_smi_isp_module_put(data, ISP8_PWR_WPE_3_LITE);
}
EXPORT_SYMBOL(smi_isp_wpe3_lite_put);

static struct smi_user_pwr_ctrl smi_isp_dip_pwr_cb = {
	 .name = "qof_isp_dip",
	 .data = &is_smi_use_qof_locked[ISP8_PWR_DIP],
	 .smi_user_id =  MTK_SMI_IMG_DIP,
	 .smi_user_get = smi_isp_dip_get,
	 .smi_user_get_if_in_use = smi_isp_dip_get_if_in_use,
	 .smi_user_put = smi_isp_dip_put,
};

static struct smi_user_pwr_ctrl smi_isp_traw_pwr_cb = {
	 .name = "qof_isp_traw",
	 .data = &is_smi_use_qof_locked[ISP8_PWR_TRAW],
	 .smi_user_id =  MTK_SMI_IMG_TRAW,
	 .smi_user_get = smi_isp_traw_get,
	 .smi_user_get_if_in_use = smi_isp_traw_get_if_in_use,
	 .smi_user_put = smi_isp_traw_put,
};

static struct smi_user_pwr_ctrl smi_isp_wpe1_eis_pwr_cb = {
	 .name = "qof_isp_wpe1_eis",
	 .data = &is_smi_use_qof_locked[ISP8_PWR_WPE_1_EIS],
	 .smi_user_id =  MTK_SMI_IMG_WPE_EIS,
	 .smi_user_get = smi_isp_wpe1_eis_get,
	 .smi_user_get_if_in_use = smi_isp_wpe1_eis_get_if_in_use,
	 .smi_user_put = smi_isp_wpe1_eis_put,
};

static struct smi_user_pwr_ctrl smi_isp_wpe2_tnr_pwr_cb = {
	 .name = "qof_isp_wpe2_tnr",
	 .data = &is_smi_use_qof_locked[ISP8_PWR_WPE_2_TNR],
	 .smi_user_id =  MTK_SMI_IMG_WPE_TNR,
	 .smi_user_get = smi_isp_wpe2_tnr_get,
	 .smi_user_get_if_in_use = smi_isp_wpe2_tnr_get_if_in_use,
	 .smi_user_put = smi_isp_wpe2_tnr_put,
};

static struct smi_user_pwr_ctrl smi_isp_wpe3_lite_pwr_cb = {
	 .name = "qof_isp_wpe3_lite",
	 .data = &is_smi_use_qof_locked[ISP8_PWR_WPE_3_LITE],
	 .smi_user_id =  MTK_SMI_IMG_WPE_LITE,
	 .smi_user_get = smi_isp_wpe3_lite_get,
	 .smi_user_get_if_in_use = smi_isp_wpe3_lite_get_if_in_use,
	 .smi_user_put = smi_isp_wpe3_lite_put,
};

static void imgsys_cmdq_qof_set_restore_done(struct cmdq_pkt *pkt, u32 pwr_id)
{
	unsigned int addr;
	unsigned int val;
	unsigned int mask;
	struct qof_events *qof_event;

	if (pkt == NULL || pwr_id >= ISP8_PWR_NUM) {
		QOF_LOGI("qof param wrong!\n");
		return;
	}

	addr = qof_reg_table[pwr_id][QOF_IMG_GCE_RESTORE_DONE].addr;
	val = qof_reg_table[pwr_id][QOF_IMG_GCE_RESTORE_DONE].val;
	mask = qof_reg_table[pwr_id][QOF_IMG_GCE_RESTORE_DONE].mask;
	qof_event = &qof_events_isp8[pwr_id];

	// set restore done
	cmdq_pkt_write(pkt, NULL, addr /* address*/ , val /* val */ , mask /* mask */ );

	// put down hw event
	cmdq_pkt_clear_event(pkt, qof_event->hw_event_restore);
}

static void imgsys_cmdq_qof_set_larb_golden(struct qof_larb_info larb_info, struct cmdq_pkt *pkt)
{
	unsigned int reg_ba, ofset, bound, i;
	if (larb_info.larb_reg_list == NULL) {
		QOF_LOGE("Param is null !\n");
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

static void imgsys_qof_traw_direct_link_reset(struct cmdq_pkt *pkt)
{

	cmdq_pkt_write(pkt, NULL,
			(IMG_CG_IMGSYS_MAIN + 0x260) /*address*/, BIT(1)|BIT(7),
			0xffffffff);
	cmdq_pkt_write(pkt, NULL,
			(IMG_CG_IMGSYS_MAIN + 0x260) /*address*/, 0x0,
			0xffffffff);
}

static void mtk_imgsys_set_dip_larb_golden(struct cmdq_pkt *pkt)
{
	if (pkt == NULL) {
		QOF_LOGE("Param is null !\n");
		return;
	}

	/* larb 10 */
	imgsys_cmdq_qof_set_larb_golden(qof_larb10_info, pkt);

	/* larb 15 */
	imgsys_cmdq_qof_set_larb_golden(qof_larb15_info, pkt);

	/* larb 38 */
	imgsys_cmdq_qof_set_larb_golden(qof_larb38_info, pkt);

	/* larb 39 */
	imgsys_cmdq_qof_set_larb_golden(qof_larb39_info, pkt);
}

static void mtk_imgsys_set_wpe_eis_larb_golden(struct cmdq_pkt *pkt)
{
	if (pkt == NULL) {
		QOF_LOGE("Param is null !\n");
		return;
	}

	/* larb 11 */
	imgsys_cmdq_qof_set_larb_golden(qof_larb11_info, pkt);
}

static void mtk_imgsys_set_wpe_tnr_larb_golden(struct cmdq_pkt *pkt)
{
	if (pkt == NULL) {
		QOF_LOGE("Param is null !\n");
		return;
	}

	/* larb 22 */
	imgsys_cmdq_qof_set_larb_golden(qof_larb22_info, pkt);
}

static void mtk_imgsys_set_wpe_lite_larb_golden(struct cmdq_pkt *pkt)
{
	if (pkt == NULL) {
		QOF_LOGE("Param is null !\n");
		return;
	}

	/* larb 23 */
	imgsys_cmdq_qof_set_larb_golden(qof_larb23_info, pkt);
}

static void mtk_imgsys_set_traw_larb_golden(struct cmdq_pkt *pkt)
{
	if (pkt == NULL) {
		QOF_LOGE("Param is null !\n");
		return;
	}

	/* larb 28 */
	imgsys_cmdq_qof_set_larb_golden(qof_larb28_info, pkt);

	/* larb 40 */
	imgsys_cmdq_qof_set_larb_golden(qof_larb40_info, pkt);
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

	cmdq_pkt_poll_sleep(pkt, 0/*poll val*/,
			(reg)/*addr*/, val /*mask*/);
}

static void imgsys_cmdq_dip_cg_unating(struct cmdq_pkt *pkt,
		const struct imgsys_cg_data *cg, dma_addr_t qof_work_buf_pa)
{
	unsigned int val;

	/* DIP_NR1_DIP1*/
	val = BIT(0)|BIT(1);
	imgsys_cmdq_modules_cg_ungating(pkt, IMG_CG_DIP_NR1_DIP1, cg, val, qof_work_buf_pa);

	/* DIP_NR2_DIP1 */
	val = BIT(0)|BIT(1)|BIT(2);
	imgsys_cmdq_modules_cg_ungating(pkt, IMG_CG_DIP_NR2_DIP1, cg, val, qof_work_buf_pa);

	/* DIP_TOP_DIP1 */
	val = BIT(0)|BIT(1)|BIT(2)|BIT(3)|BIT(4)|BIT(5)|BIT(6)|BIT(7)|BIT(8);
	imgsys_cmdq_modules_cg_ungating(pkt, IMG_CG_DIP_TOP_DIP1, cg, val, qof_work_buf_pa);
}

static void imgsys_cmdq_wpe1_cg_unating(struct cmdq_pkt *pkt,
		const struct imgsys_cg_data *cg, dma_addr_t qof_work_buf_pa)
{
	unsigned int val;

	/* WPE1_DIP1 */
	val = BIT(0)|BIT(1)|BIT(2);
	imgsys_cmdq_modules_cg_ungating(pkt, IMG_CG_WPE1_DIP1, cg, val, qof_work_buf_pa);
}

static void imgsys_cmdq_wpe2_cg_unating(struct cmdq_pkt *pkt,
		const struct imgsys_cg_data *cg, dma_addr_t qof_work_buf_pa)
{
	unsigned int val;

	/* WPE2_DIP1 */
	val = BIT(0)|BIT(1)|BIT(2);
	imgsys_cmdq_modules_cg_ungating(pkt, IMG_CG_WPE2_DIP1, cg, val, qof_work_buf_pa);
}

static void imgsys_cmdq_wpe3_cg_unating(struct cmdq_pkt *pkt,
		const struct imgsys_cg_data *cg, dma_addr_t qof_work_buf_pa)
{
	unsigned int val;

	/* WPE3_DIP1 */
	val = BIT(0)|BIT(1)|BIT(2);
	imgsys_cmdq_modules_cg_ungating(pkt, IMG_CG_WPE3_DIP1, cg, val, qof_work_buf_pa);
}

static void imgsys_cmdq_traw_cg_unating(struct cmdq_pkt *pkt,
		const struct imgsys_cg_data *cg, dma_addr_t qof_work_buf_pa)
{
	unsigned int val;

	/* 1 &traw_dip1_clk	CLK_TRAW_DIP1_TRAW */
	val = BIT(0)|BIT(1)|BIT(2)|BIT(3);
	imgsys_cmdq_modules_cg_ungating(pkt, IMG_CG_TRAW_DIP1, cg, val, qof_work_buf_pa);

	/* 2 TRAW CAP */
	val = BIT(0);
	imgsys_cmdq_modules_cg_ungating(pkt, IMG_CG_TRAW_CAP_DIP1, cg, val, qof_work_buf_pa);
}

void mtk_imgsys_qof_print_hw_info(u32 mod)
{
	struct qof_events *event;

	if (IS_MOD_SUPPORT_QOF(mod) == false)
		return;

	event = &qof_events_isp8[mod];

	QOF_LOGI("mod[%d]rg(0x%x):0x%x;rg(0x%x):0x%x;rg(0x%x):0x%x;rg(0x%x):0x%x;rg(0x%x):0x%x;rg(0x%x):0x%x",
		mod,
		qof_reg_table[mod][QOF_IMG_EVENT_CNT_ADD].addr,
		readl(QOF_GET_REMAP_ADDR(qof_reg_table[mod][QOF_IMG_EVENT_CNT_ADD].addr)),
		qof_reg_table[mod][QOF_IMG_ITC_SRC_SEL].addr,
		readl(QOF_GET_REMAP_ADDR(qof_reg_table[mod][QOF_IMG_ITC_SRC_SEL].addr)),
		qof_reg_table[mod][QOF_IMG_PWR_ACK_2ND_WAIT_TH].addr,
		readl(QOF_GET_REMAP_ADDR(qof_reg_table[mod][QOF_IMG_PWR_ACK_2ND_WAIT_TH].addr)),
		qof_reg_table[mod][QOF_IMG_POWER_STATE].addr,
		readl(QOF_GET_REMAP_ADDR(qof_reg_table[mod][QOF_IMG_POWER_STATE].addr)),
		qof_reg_table[mod][QOF_IMG_GCE_SAVE_DONE].addr,
		readl(QOF_GET_REMAP_ADDR(qof_reg_table[mod][QOF_IMG_GCE_SAVE_DONE].addr)),
		qof_reg_table[mod][QOF_IMG_QOF_EVENT_CNT].addr,
		readl(QOF_GET_REMAP_ADDR(qof_reg_table[mod][QOF_IMG_QOF_EVENT_CNT].addr)));

	QOF_LOGI("qof_hw_info:rg(0x%x):0x%x;rg(0x%x):0x%x;rg(0x%x):0x%x;rg(0x%x):0x%x;rg(0x%x):0x%x",
		qof_reg_table[mod][QOF_IMG_QOF_VOTER_DBG].addr,
		readl(QOF_GET_REMAP_ADDR(qof_reg_table[mod][QOF_IMG_QOF_VOTER_DBG].addr)),
		qof_reg_table[mod][QOF_IMG_QOF_DONE_STATUS].addr,
		readl(QOF_GET_REMAP_ADDR(qof_reg_table[mod][QOF_IMG_QOF_DONE_STATUS].addr)),
		qof_reg_table[mod][QOF_IMG_ITC_STATUS].addr,
		readl(QOF_GET_REMAP_ADDR(qof_reg_table[mod][QOF_IMG_ITC_STATUS].addr)),
		qof_reg_table[mod][QOF_IMG_QOF_MTC_ST_LSB].addr,
		readl(QOF_GET_REMAP_ADDR(qof_reg_table[mod][QOF_IMG_QOF_MTC_ST_LSB].addr)),
		qof_reg_table[mod][QOF_IMG_QOF_MTC_ST_MSB2].addr,
		readl(QOF_GET_REMAP_ADDR(qof_reg_table[mod][QOF_IMG_QOF_MTC_ST_MSB2].addr)));
}

static void mtk_qof_print_mtcmos_status(void)
{
	struct cmdq_client *pwr_clt = smi_cb_pwr_ctl;

	QOF_LOGI("FP[0x%x] MTCMOS:cnt[%u]cb_evt[%u]MAIN[0x%x]VCORE[0x%x]DIP[0x%x]TRAW[0x%x]W1[0x%x]W2[0x%x]W3[0x%x]",
		(readl(QOF_GET_REMAP_ADDR(QOF_SPARE_REG_FOOTPRINT))),
		imgsys_voter_cnt_locked,
		(pwr_clt == NULL ? QOF_ERROR_CODE : cmdq_get_event(pwr_clt->chan, CMDQ_SW_EVENT_QOF_SMI_SW_EVENT)),
		(readl(g_maped_rg[MAPED_RG_ISP_MAIN_PWR_CON])),
		(readl(g_maped_rg[MAPED_RG_ISP_VCORE_PWR_CON])),
		(readl(g_maped_rg[MAPED_RG_ISP_DIP_PWR_CON])),
		(readl(g_maped_rg[MAPED_RG_ISP_TRAW_PWR_CON])),
		(readl(g_maped_rg[MAPED_RG_ISP_WPE_EIS_PWR_CON])),
		(readl(g_maped_rg[MAPED_RG_ISP_WPE_TNR_PWR_CON])),
		(readl(g_maped_rg[MAPED_RG_ISP_WPE_LITE_PWR_CON])));
}

static void mtk_qof_print_cg_status(void)
{
	void __iomem *addr;
	unsigned int addr_val;

	addr = QOF_GET_REMAP_ADDR(qof_reg_table[ISP8_PWR_DIP][QOF_IMG_QOF_STATE_DBG].addr);
	addr_val = readl(addr);
	if (((addr_val & BIT(1)) == BIT(1)) ||
		((addr_val & BIT(6)) == BIT(6))) {
		QOF_LOGI(" dip CG Status: IMG_MAIN[0x%x], NR1[0x%x], NR2[0x%x], DIP_TOP[0x%x]",
		(readl(g_maped_rg[MAPED_RG_IMG_CG_IMGSYS_MAIN])),
		(readl(g_maped_rg[MAPED_RG_IMG_CG_DIP_NR1_DIP1])),
		(readl(g_maped_rg[MAPED_RG_IMG_CG_DIP_NR2_DIP1])),
		(readl(g_maped_rg[MAPED_RG_IMG_CG_DIP_TOP_DIP1])));
	} else
		QOF_LOGI("qof mtcmos dip is off. sta=0x%x", readl(addr));
	addr = QOF_GET_REMAP_ADDR(qof_reg_table[ISP8_PWR_TRAW][QOF_IMG_QOF_STATE_DBG].addr);
	addr_val = readl(addr);
	if (((addr_val & BIT(1)) == BIT(1)) ||
		((addr_val & BIT(6)) == BIT(6))) {
		QOF_LOGI(" traw CG Status: TRAW_CAP[0x%x], TRAW_DIP1[0x%x]",
		(readl(g_maped_rg[MAPED_RG_IMG_CG_TRAW_CAP_DIP1])),
		(readl(g_maped_rg[MAPED_RG_IMG_CG_TRAW_DIP1])));
	} else
		QOF_LOGI("qof mtcmos traw is off. sta=0x%x",  readl(addr));
	addr = QOF_GET_REMAP_ADDR(qof_reg_table[ISP8_PWR_WPE_1_EIS][QOF_IMG_QOF_STATE_DBG].addr);
	addr_val = readl(addr);
	if (((addr_val & BIT(1)) == BIT(1)) ||
		((addr_val & BIT(6)) == BIT(6))) {
		QOF_LOGI(" wpe1 CG Status: WPE1[0x%x]",
		(readl(g_maped_rg[MAPED_RG_IMG_CG_WPE1_DIP1])));
	} else
		QOF_LOGI("qof mtcmos wpe1 is off. sta=0x%x", readl(addr));
	addr = QOF_GET_REMAP_ADDR(qof_reg_table[ISP8_PWR_WPE_2_TNR][QOF_IMG_QOF_STATE_DBG].addr);
	addr_val = readl(addr);
	if (((addr_val & BIT(1)) == BIT(1)) ||
		((addr_val & BIT(6)) == BIT(6))) {
		QOF_LOGI(" wpe2 CG Status: WPE2[0x%x]",
		(readl(g_maped_rg[MAPED_RG_IMG_CG_WPE2_DIP1])));
	} else
		QOF_LOGI("qof mtcmos wpe2 is off. sta=0x%x", readl(addr));
	addr = QOF_GET_REMAP_ADDR(qof_reg_table[ISP8_PWR_WPE_3_LITE][QOF_IMG_QOF_STATE_DBG].addr);
	addr_val = readl(addr);
	if (((addr_val & BIT(1)) == BIT(1)) ||
		((addr_val & BIT(6)) == BIT(6))) {
		QOF_LOGI(" wpe3 CG Status: WPE3[0x%x]",
		(readl(g_maped_rg[MAPED_RG_IMG_CG_WPE3_DIP1])));
	} else
		QOF_LOGI("qof mtcmos wpe3 is off. sta=0x%x", readl(addr));
}

static void imgsys_cmdq_init_qof_events(void)
{
	struct qof_events *event;
	u32 usr_id = 0;

	for(usr_id = ISP8_PWR_START; usr_id < ISP8_PWR_NUM; usr_id++) {
		event = &qof_events_isp8[usr_id];
		event->sw_event_lock = sw_event_lock_list[usr_id];
		event->hw_event_restore = hw_event_restore_list[usr_id];
	}
}

static void imgsys_cmdq_qof_init_pwr_thread(struct mtk_imgsys_dev *imgsys_dev)
{
	struct device *dev = imgsys_dev->dev;
	u32 thd_idx = 0;

	for (thd_idx = IMGSYS_NOR_THD; thd_idx < IMGSYS_NOR_THD + IMGSYS_PWR_THD; thd_idx++) {
#ifdef QOF_SUPPORT_SMI_GCE_CALLBACK
		if (thd_idx != QOF_GCE_THREAD_SMI_CB) {
			imgsys_pwr_clt[thd_idx - IMGSYS_NOR_THD] = cmdq_mbox_create(dev, thd_idx);
			QOF_LOGI(
				"%s: cmdq_mbox_create pwr_thd(%d, 0x%lx)",
				__func__, thd_idx, (unsigned long)imgsys_pwr_clt[thd_idx - IMGSYS_NOR_THD]);
		} else {
			smi_cb_pwr_ctl = cmdq_mbox_create(dev, thd_idx);
			QOF_LOGI(
				"%s: cmdq_mbox_create smi_cb_pwr_thd(%d, 0x%lx)",
				__func__, thd_idx, (unsigned long)smi_cb_pwr_ctl);
		}
#else
		idx = thd_idx - IMGSYS_NOR_THD;
		if (idx < QOF_TOTAL_THREAD) {
			imgsys_pwr_clt[thd_idx - IMGSYS_NOR_THD] = cmdq_mbox_create(dev, thd_idx);
			QOF_LOGI(
				"%s: cmdq_mbox_create pwr_thd(%d, 0x%lx)",
				__func__, thd_idx, (unsigned long)imgsys_pwr_clt[thd_idx - IMGSYS_NOR_THD]);
		} else {
			imgsys_pwr_clt[thd_idx - IMGSYS_NOR_THD] = NULL;
			QOF_LOGI("qof [%s] thread indexs are not match !
				[thd_id: %d][index: %d][total pwr thd num: %d/%d]",
				__func__, thd_idx, idx, QOF_TOTAL_THREAD, IMGSYS_PWR_THD);
		}
#endif
	}
}

static void qof_cmdq_set_module_rst(struct mtk_imgsys_dev *imgsys_dev,
		struct cmdq_pkt *pkt,
		const struct imgsys_mtcmos_data *pwr)
{

	if (imgsys_dev == NULL || pkt == NULL || pwr == NULL) {
		QOF_LOGE("param is null (%d/%d/%d)",
			(imgsys_dev == NULL), (pkt == NULL), (pwr == NULL));
		return;
	}

	QOF_LOGI("imgsys_cmdq_restore_locked pwr->pwr_id = %d",pwr->pwr_id);
	switch (pwr->pwr_id) {
	case ISP8_PWR_DIP:
		if (imgsys_dev->modules[IMGSYS_MOD_DIP].cmdq_set)
			imgsys_dev->modules[IMGSYS_MOD_DIP].cmdq_set(imgsys_dev, pkt, REG_MAP_E_DIP);
		else
			QOF_LOGE("cmdq_set function pointer is null");
		break;
	case ISP8_PWR_TRAW:
		if (imgsys_dev->modules[IMGSYS_MOD_TRAW].cmdq_set)
			imgsys_dev->modules[IMGSYS_MOD_TRAW].cmdq_set(imgsys_dev, pkt, REG_MAP_E_TRAW);
		else
			QOF_LOGE("cmdq_set function pointer is null");
		break;
	case ISP8_PWR_WPE_1_EIS:
		if (imgsys_dev->modules[IMGSYS_MOD_WPE].cmdq_set)
			imgsys_dev->modules[IMGSYS_MOD_WPE].cmdq_set(imgsys_dev, pkt, REG_MAP_E_WPE_EIS);
		else
			QOF_LOGE("cmdq_set function pointer is null");
		if (imgsys_dev->modules[IMGSYS_MOD_PQDIP].cmdq_set)
			imgsys_dev->modules[IMGSYS_MOD_PQDIP].cmdq_set(imgsys_dev, pkt, REG_MAP_E_PQDIP_A);
		else
			QOF_LOGE("cmdq_set function pointer is null");
		break;
	case ISP8_PWR_WPE_2_TNR:
		if (imgsys_dev->modules[IMGSYS_MOD_OMC].cmdq_set)
			imgsys_dev->modules[IMGSYS_MOD_OMC].cmdq_set(imgsys_dev, pkt, REG_MAP_E_OMC_TNR);
		else
			QOF_LOGE("cmdq_set function pointer is null");
		if (imgsys_dev->modules[IMGSYS_MOD_PQDIP].cmdq_set)
			imgsys_dev->modules[IMGSYS_MOD_PQDIP].cmdq_set(imgsys_dev, pkt, REG_MAP_E_PQDIP_B);
		else
			QOF_LOGE("cmdq_set function pointer is null");
		break;
	case ISP8_PWR_WPE_3_LITE:
		if (imgsys_dev->modules[IMGSYS_MOD_WPE].cmdq_set)
			imgsys_dev->modules[IMGSYS_MOD_WPE].cmdq_set(imgsys_dev, pkt, REG_MAP_E_WPE_LITE);
		else
			QOF_LOGE("cmdq_set function pointer is null");
		if (imgsys_dev->modules[IMGSYS_MOD_OMC].cmdq_set)
			imgsys_dev->modules[IMGSYS_MOD_OMC].cmdq_set(imgsys_dev, pkt, REG_MAP_E_OMC_LITE);
		else
			QOF_LOGE("cmdq_set function pointer is null");
		break;
	default:
		QOF_LOGE("case invalid[%u]\n", pwr->pwr_id);
		break;
	}
}

static void imgsys_cmdq_restore_locked(struct mtk_imgsys_dev *imgsys_dev,
		struct cmdq_pkt *pkt,
		const struct imgsys_mtcmos_data *pwr)
{
	if (imgsys_dev == NULL || pkt == NULL || pwr == NULL) {
		QOF_LOGE("param is null (%d/%d/%d)\n",
			(imgsys_dev == NULL), (pkt == NULL), (pwr == NULL));
		return;
	}

	/* ungate CG */
	if (pwr->cg_ungating)
		pwr->cg_ungating(pkt, pwr->cg_data, imgsys_dev->qof_work_buf_pa);

	/* reset larb golden */
	if (pwr->set_larb_golden)
		pwr->set_larb_golden(pkt);

	/* set module rst */
	qof_cmdq_set_module_rst(imgsys_dev, pkt, pwr);

	/* reset module after power on */
	if (pwr->direct_link_reset)
		pwr->direct_link_reset(pkt);

	/* register qof */
	if (pwr->qof_restore_done)
		pwr->qof_restore_done(pkt, pwr->pwr_id);
}

static void imgsys_cmdq_wpe12_restore_locked(struct mtk_imgsys_dev *imgsys_dev,
		struct cmdq_pkt *pkt)
{
	struct qof_events *qof_event;
	int pwr_id = 0;

	if (imgsys_dev == NULL || pkt == NULL) {
		QOF_LOGE("param is null (%d/%d)\n",
			(imgsys_dev == NULL), (pkt == NULL));
		return;
	}

	/* Wait for restore hw event */
	pwr_id = ISP8_PWR_WPE_2_TNR;
	qof_event = &qof_events_isp8[pwr_id];
	cmdq_pkt_wfe(pkt, qof_event->hw_event_restore);

	imgsys_cmdq_restore_locked(imgsys_dev, pkt, &isp8_module_data[ISP8_PWR_WPE_2_TNR]);
	imgsys_cmdq_restore_locked(imgsys_dev, pkt, &isp8_module_data[ISP8_PWR_WPE_1_EIS]);
}

static void qof_start_pwr_restore_task(struct mtk_imgsys_dev *imgsys_dev,
		struct cmdq_client *client, u32 pwr_id, u32 th_id,
		struct qof_events *event)
{
	struct cmdq_pkt *restore_pkt;

	if (pwr_id >= ISP8_PWR_NUM || th_id >= IMG_GCE_THREAD_PWR_END) {
		QOF_LOGI("%s:qof id is invalid %u/%u\n", __func__, pwr_id, th_id);
		return;
	}

	restore_pkt = pwr_buf_handle[th_id].restore_pkt;

	/* Power on kernel thread */
	cmdq_mbox_enable(client->chan);

	if (!restore_pkt) {
		restore_pkt = pwr_buf_handle[th_id].restore_pkt = cmdq_pkt_create(client);
		if (!restore_pkt) {
			QOF_LOGI("%s:create cmdq package fail\n", __func__);
			return;
		}
	}

	/* Program start */
	/* Wait for restore hw event */
	cmdq_pkt_wfe(restore_pkt, event->hw_event_restore);

	// wa for gce thd lacked
	if (pwr_id == ISP8_PWR_WPE_1_EIS)
		imgsys_cmdq_wpe12_restore_locked(imgsys_dev, restore_pkt);
	else
		imgsys_cmdq_restore_locked(imgsys_dev, restore_pkt,
				&isp8_module_data[pwr_id]);

	/* Program end */
	restore_pkt->priority = IMGSYS_PRI_HIGH;
	restore_pkt->no_irq = true;
	cmdq_pkt_finalize_loop(restore_pkt);
	cmdq_pkt_flush_async(restore_pkt, NULL, (void *)restore_pkt);
}

static void qof_mapping_pwr_loop(u32 pwr, struct mtk_imgsys_dev *imgsys_dev)
{
	u32 th_id = IMG_GCE_THREAD_PWR_START;

	if (!imgsys_dev) {
		cmdq_err("qof imgsys_dev is NULL");
		return;
	}

	th_id = get_thread_id_by_pwr_id(pwr);
	if (th_id >= QOF_TOTAL_THREAD) {
		cmdq_err("qof th_id mapping wrong[%u]\n", th_id);
		return;
	}

	if (imgsys_pwr_clt[th_id])
		qof_start_pwr_restore_task(imgsys_dev, imgsys_pwr_clt[th_id], pwr, th_id, &qof_events_isp8[pwr]);
	else
		QOF_LOGI("qof imgsys_pwr_clt[%u] = null thid=%d\n", pwr, th_id);
}

static void qof_start_pwr_loop(struct mtk_imgsys_dev *imgsys_dev)
{
	u32 pwr = ISP8_PWR_START;

	if (!imgsys_dev) {
		cmdq_err("qof imgsys_dev is NULL");
		return;
	}

	for (; (pwr < ISP8_PWR_NUM); pwr++) {
		/* Initial power restore thread */
		if (pwr == ISP8_PWR_WPE_2_TNR) // wa for gce thd lacked
			continue;
		qof_mapping_pwr_loop(pwr, imgsys_dev);
	}
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
		p = (int *)*buf_va;
		*p = 0;
	} else {
		pr_err("%s: cmdq mbox buf alloc fail\n", __func__);
	}
}

void qof_start_all_gce_loop(struct mtk_imgsys_dev *imgsys_dev)
{
	qof_start_pwr_loop(imgsys_dev);
	qof_start_smi_cb_loop(imgsys_dev, smi_cb_pwr_ctl);
}

static void qof_stop_all_gce_loop(void)
{
	int idx = 0;
	u32 thd_idx = 0;

	/* pwr thread */
	for (thd_idx = IMGSYS_NOR_THD; thd_idx < IMGSYS_NOR_THD + IMGSYS_PWR_THD; thd_idx++) {
		idx = thd_idx - IMGSYS_NOR_THD;
		if (idx >= QOF_TOTAL_THREAD || idx < 0) {
			QOF_LOGE("idx is wrong %d\n", idx);
			continue;
		}
		if (thd_idx == QOF_GCE_THREAD_SMI_CB)
			continue;
		if (imgsys_pwr_clt[idx] != NULL) {
			cmdq_mbox_stop(imgsys_pwr_clt[idx]);
			cmdq_mbox_disable(imgsys_pwr_clt[idx]->chan);
		}
		if (pwr_buf_handle[idx].restore_pkt != NULL) {
			cmdq_pkt_destroy(pwr_buf_handle[idx].restore_pkt);
			pwr_buf_handle[idx].restore_pkt = NULL;
		}
	}

	/* smi cb thread */
	if (smi_cb_pwr_ctl != NULL) {
		cmdq_mbox_stop(smi_cb_pwr_ctl);
		cmdq_mbox_disable(smi_cb_pwr_ctl->chan);
	}
	if (g_smi_cb_pkt != NULL) {
		cmdq_pkt_destroy(g_smi_cb_pkt);
		g_smi_cb_pkt = NULL;
	}
}

void mtk_imgsys_cmdq_qof_init(struct mtk_imgsys_dev *imgsys_dev, struct cmdq_client *imgsys_clt)
{
	int ver = 0;
	int rg_idx = 0;
	unsigned long flag;
	u32 pwr = 0;

	QOF_LOGI("+\n");
	if (of_property_read_u32_index(imgsys_dev->dev->of_node,
		"mediatek,imgsys-qof-ver", 0, &ver) == 0)
		QOF_LOGI("[%s] qof version = %u\n", __func__, ver);

	/* init global val */
	imgsys_dev->qof_ver = ver;
	g_qof_ver = ver;
	g_qof_debug_level = 0;
	smi_cb_pwr_ctl = NULL;
	g_imgsys_dev = imgsys_dev;
	is_poll_event_mode = false;
	spin_lock_init(&qof_lock);
	spin_lock_irqsave(&qof_lock, flag);
	imgsys_voter_cnt_locked = 0;
	for(pwr = ISP8_PWR_START; pwr < ISP8_PWR_NUM; pwr++)
		is_smi_use_qof_locked[pwr] = false;
	spin_unlock_irqrestore(&qof_lock, flag);

	if (imgsys_dev->qof_ver == MTK_IMGSYS_QOF_FUNCTION_OFF)
		return;

	QOF_LOGI("qof version = %u, poll_event_mode = %d", ver, is_poll_event_mode);

	/* allocate work buf */
	imgsys_cmdq_qof_alloc_buf(imgsys_clt, &(imgsys_dev->work_buf_va), &(imgsys_dev->work_buf_pa));
	g_qof_work_buf_va = &(imgsys_dev->work_buf_va);

	imgsys_cmdq_qof_init_pwr_thread(imgsys_dev);

	imgsys_cmdq_init_qof_events();

	/* ioremap rg */
	g_maped_rg[MAPED_RG_ISP_TRAW_PWR_CON]		= ioremap(ISP_TRAW_PWR_CON, 4);
	g_maped_rg[MAPED_RG_ISP_DIP_PWR_CON]		= ioremap(ISP_DIP_PWR_CON, 4);
	g_maped_rg[MAPED_RG_ISP_MAIN_PWR_CON]		= ioremap(ISP_MAIN_PWR_CON, 4);
	g_maped_rg[MAPED_RG_ISP_VCORE_PWR_CON]		= ioremap(ISP_VCORE_PWR_CON, 4);
	g_maped_rg[MAPED_RG_ISP_WPE_EIS_PWR_CON]	= ioremap(ISP_WPE_EIS_PWR_CON, 4);
	g_maped_rg[MAPED_RG_ISP_WPE_TNR_PWR_CON]	= ioremap(ISP_WPE_TNR_PWR_CON, 4);
	g_maped_rg[MAPED_RG_ISP_WPE_LITE_PWR_CON]	= ioremap(ISP_WPE_LITE_PWR_CON, 4);
	g_maped_rg[MAPED_RG_IMG_CG_IMGSYS_MAIN]		= ioremap(IMG_CG_IMGSYS_MAIN, 4);
	g_maped_rg[MAPED_RG_IMG_CG_DIP_NR1_DIP1]	= ioremap(IMG_CG_DIP_NR1_DIP1, 4);
	g_maped_rg[MAPED_RG_IMG_CG_DIP_NR2_DIP1]	= ioremap(IMG_CG_DIP_NR2_DIP1, 4);
	g_maped_rg[MAPED_RG_IMG_CG_DIP_TOP_DIP1]	= ioremap(IMG_CG_DIP_TOP_DIP1, 4);
	g_maped_rg[MAPED_RG_IMG_CG_TRAW_CAP_DIP1]	= ioremap(IMG_CG_TRAW_CAP_DIP1, 4);
	g_maped_rg[MAPED_RG_IMG_CG_TRAW_DIP1]		= ioremap(IMG_CG_TRAW_DIP1, 4);
	g_maped_rg[MAPED_RG_IMG_CG_WPE1_DIP1]		= ioremap(IMG_CG_WPE1_DIP1, 4);
	g_maped_rg[MAPED_RG_IMG_CG_WPE2_DIP1]		= ioremap(IMG_CG_WPE2_DIP1, 4);
	g_maped_rg[MAPED_RG_IMG_CG_WPE3_DIP1]		= ioremap(IMG_CG_WPE3_DIP1, 4);
	g_maped_rg[MAPED_RG_IMG_LARB10_BASE]		= ioremap(IMG_LARB10_BASE, 4);
	g_maped_rg[MAPED_RG_IMG_LARB11_BASE]		= ioremap(IMG_LARB11_BASE, 4);
	g_maped_rg[MAPED_RG_IMG_LARB15_BASE]		= ioremap(IMG_LARB15_BASE, 4);
	g_maped_rg[MAPED_RG_IMG_LARB22_BASE]		= ioremap(IMG_LARB22_BASE, 4);
	g_maped_rg[MAPED_RG_IMG_LARB23_BASE]		= ioremap(IMG_LARB23_BASE, 4);
	g_maped_rg[MAPED_RG_IMG_LARB38_BASE]		= ioremap(IMG_LARB38_BASE, 4);
	g_maped_rg[MAPED_RG_IMG_LARB39_BASE]		= ioremap(IMG_LARB39_BASE, 4);
	g_maped_rg[MAPED_RG_IMG_LARB28_BASE]		= ioremap(IMG_LARB28_BASE, 4);
	g_maped_rg[MAPED_RG_IMG_LARB40_BASE]		= ioremap(IMG_LARB40_BASE, 4);
	g_maped_rg[MAPED_RG_QOF_REG_BASE]			= ioremap(QOF_REG_BASE, 4);
	g_maped_rg[MAPED_RG_MMPC_REG_BASE]			= ioremap(MMPC_REG_BASE, 4);

	for (rg_idx = MAPED_RG_LIST_START; rg_idx < MAPED_RG_LIST_NUM; rg_idx++) {
		if (!g_maped_rg[rg_idx]) {
			QOF_LOGI("qof %s Unable to ioremap %d registers !\n",
				__func__, rg_idx);
		}
	}
	/* smi cb register */
	if (IS_MOD_SUPPORT_QOF(ISP8_PWR_DIP))
		mtk_smi_dbg_register_pwr_ctrl_cb(&smi_isp_dip_pwr_cb);
	if (IS_MOD_SUPPORT_QOF(ISP8_PWR_TRAW))
		mtk_smi_dbg_register_pwr_ctrl_cb(&smi_isp_traw_pwr_cb);
	if (IS_MOD_SUPPORT_QOF(ISP8_PWR_WPE_1_EIS))
		mtk_smi_dbg_register_pwr_ctrl_cb(&smi_isp_wpe1_eis_pwr_cb);
	if (IS_MOD_SUPPORT_QOF(ISP8_PWR_WPE_2_TNR))
		mtk_smi_dbg_register_pwr_ctrl_cb(&smi_isp_wpe2_tnr_pwr_cb);
	if (IS_MOD_SUPPORT_QOF(ISP8_PWR_WPE_3_LITE))
		mtk_smi_dbg_register_pwr_ctrl_cb(&smi_isp_wpe3_lite_pwr_cb);
	QOF_LOGI("-\n");
}

void mtk_imgsys_cmdq_qof_release(struct mtk_imgsys_dev *imgsys_dev, struct cmdq_client *imgsys_clt)
{
	/* release resource */
	int idx = 0;
	int rg_idx = 0;
	u32 thd_idx = 0;

	if (!imgsys_clt) {
		cmdq_err("cl is NULL");
		dump_stack();
		return;
	}
	QOF_LOGI("release resource +\n");

	/* pwr thread */
	for (thd_idx = IMGSYS_NOR_THD; thd_idx < IMGSYS_NOR_THD + IMGSYS_PWR_THD; thd_idx++) {
		idx = thd_idx - IMGSYS_NOR_THD;
		if (idx >= QOF_TOTAL_THREAD || idx < 0) {
			QOF_LOGE("idx is wrong %d\n", idx);
			continue;
		}
		if ((thd_idx != QOF_GCE_THREAD_SMI_CB) &&
			imgsys_pwr_clt[idx] != NULL) {
			cmdq_mbox_destroy(imgsys_pwr_clt[idx]);
			imgsys_pwr_clt[idx] = NULL;
		}
	}

	if (smi_cb_pwr_ctl) {
		cmdq_mbox_destroy(smi_cb_pwr_ctl);
		smi_cb_pwr_ctl = NULL;
	}

	/* iounmap */
	for (rg_idx = MAPED_RG_LIST_START; rg_idx < MAPED_RG_LIST_NUM; rg_idx++) {
		if (g_maped_rg[rg_idx])
			iounmap(g_maped_rg[rg_idx]);
	}

	cmdq_mbox_buf_free(imgsys_clt, imgsys_dev->work_buf_va, imgsys_dev->work_buf_pa);
	QOF_LOGI("release resource -\n");
}

static void qof_engine_on_setting(const u32 mod)
{
	void __iomem *io_addr;
	/* Add voter */
	io_addr = QOF_GET_REMAP_ADDR(qof_reg_table[mod][QOF_IMG_APMCU_SET].addr);
	write_mask(io_addr,
		qof_reg_table[mod][QOF_IMG_APMCU_SET].val,
		qof_reg_table[mod][QOF_IMG_APMCU_SET].mask);
	/* Init setting */
	io_addr = QOF_GET_REMAP_ADDR(qof_reg_table[mod][QOF_IMG_HW_CLR_EN].addr);
	write_mask(io_addr,
		qof_reg_table[mod][QOF_IMG_HW_CLR_EN].val,
		qof_reg_table[mod][QOF_IMG_HW_CLR_EN].mask);
	io_addr = QOF_GET_REMAP_ADDR(qof_reg_table[mod][QOF_IMG_HW_SET_EN].addr);
	write_mask(io_addr,
		qof_reg_table[mod][QOF_IMG_HW_SET_EN].val,
		qof_reg_table[mod][QOF_IMG_HW_SET_EN].mask);
	io_addr = QOF_GET_REMAP_ADDR(qof_reg_table[mod][QOF_IMG_HW_SEQ_EN].addr);
	write_mask(io_addr,
		qof_reg_table[mod][QOF_IMG_HW_SEQ_EN].val,
		qof_reg_table[mod][QOF_IMG_HW_SEQ_EN].mask);
	io_addr = QOF_GET_REMAP_ADDR(qof_reg_table[mod][QOF_IMG_GCE_RESTORE_EN].addr);
	write_mask(io_addr,
		qof_reg_table[mod][QOF_IMG_GCE_RESTORE_EN].val,
		qof_reg_table[mod][QOF_IMG_GCE_RESTORE_EN].mask);
	io_addr = QOF_GET_REMAP_ADDR(qof_reg_table[mod][QOF_IMG_PWR_ACK_2ND_WAIT_TH].addr);
	write_mask(io_addr,
		qof_reg_table[mod][QOF_IMG_PWR_ACK_2ND_WAIT_TH].val,
		qof_reg_table[mod][QOF_IMG_PWR_ACK_2ND_WAIT_TH].mask);
	io_addr = QOF_GET_REMAP_ADDR(qof_reg_table[mod][QOF_IMG_PWR_ACK_WAIT_TH].addr);
	write_mask(io_addr,
		qof_reg_table[mod][QOF_IMG_PWR_ACK_WAIT_TH].val,
		qof_reg_table[mod][QOF_IMG_PWR_ACK_WAIT_TH].mask);
	/* qof engine enable */
	io_addr = QOF_GET_REMAP_ADDR(qof_reg_table[mod][QOF_IMG_QOF_ENG_EN].addr);
	write_mask(io_addr,
		qof_reg_table[mod][QOF_IMG_QOF_ENG_EN].val,
		qof_reg_table[mod][QOF_IMG_QOF_ENG_EN].mask);
}

static void qof_set_engine_on(const u32 mod)
{
	void __iomem *io_addr;
	int tmp;

	if(IS_MOD_SUPPORT_QOF(mod) == false)
		return;

	if (mod == ISP8_PWR_WPE_1_EIS) { // wa for gce thd lacked
		qof_engine_on_setting(ISP8_PWR_WPE_1_EIS);
		qof_engine_on_setting(ISP8_PWR_WPE_2_TNR);

		io_addr =  QOF_GET_REMAP_ADDR(qof_reg_table[ISP8_PWR_WPE_1_EIS][QOF_IMG_QOF_STATE_DBG].addr);
		if (readl_poll_timeout_atomic
			(io_addr, tmp, (tmp & BIT(1)) == BIT(1), POLL_DELAY_US, TIMEOUT_1000US) < 0) {
			QOF_LOGE("mod[%d] waiting for qof state pwr on timeout, disable support\n",ISP8_PWR_WPE_1_EIS);
			mtk_imgsys_cmdq_qof_dump(0, false);
			g_qof_ver &= (~BIT(ISP8_PWR_WPE_1_EIS));
			return;
		}
		io_addr =  QOF_GET_REMAP_ADDR(qof_reg_table[ISP8_PWR_WPE_2_TNR][QOF_IMG_QOF_STATE_DBG].addr);
		if (readl_poll_timeout_atomic
			(io_addr, tmp, (tmp & BIT(1)) == BIT(1), POLL_DELAY_US, TIMEOUT_1000US) < 0) {
			QOF_LOGE("mod[%d] waiting for qof state pwr on timeout, disable support\n",ISP8_PWR_WPE_2_TNR);
			mtk_imgsys_cmdq_qof_dump(0, false);
			g_qof_ver &= (~BIT(ISP8_PWR_WPE_2_TNR));
			return;
		}

		/* qof APMCU Vote sub */
		io_addr = QOF_GET_REMAP_ADDR(qof_reg_table[ISP8_PWR_WPE_1_EIS][QOF_IMG_APMCU_CLR].addr);
		write_mask(io_addr,
			qof_reg_table[ISP8_PWR_WPE_1_EIS][QOF_IMG_APMCU_CLR].val,
			qof_reg_table[ISP8_PWR_WPE_1_EIS][QOF_IMG_APMCU_CLR].mask);
		io_addr = QOF_GET_REMAP_ADDR(qof_reg_table[ISP8_PWR_WPE_2_TNR][QOF_IMG_APMCU_CLR].addr);
		write_mask(io_addr,
			qof_reg_table[ISP8_PWR_WPE_2_TNR][QOF_IMG_APMCU_CLR].val,
			qof_reg_table[ISP8_PWR_WPE_2_TNR][QOF_IMG_APMCU_CLR].mask);

		io_addr = QOF_GET_REMAP_ADDR(qof_reg_table[ISP8_PWR_WPE_1_EIS][QOF_IMG_QOF_STATE_DBG].addr);
		if (readl_poll_timeout_atomic
			(io_addr, tmp, (tmp & BIT(4)) == BIT(4), POLL_DELAY_US, TIMEOUT_1000US) < 0) {
			QOF_LOGI("Warning: mod[%d] waiting qof state pwr off timeout, qof may be voted by smi\n",
				ISP8_PWR_WPE_1_EIS);
			mtk_imgsys_cmdq_qof_dump(0, false);
			// engine off
		}
		io_addr = QOF_GET_REMAP_ADDR(qof_reg_table[ISP8_PWR_WPE_2_TNR][QOF_IMG_QOF_STATE_DBG].addr);
		if (readl_poll_timeout_atomic
			(io_addr, tmp, (tmp & BIT(4)) == BIT(4), POLL_DELAY_US, TIMEOUT_1000US) < 0) {
			QOF_LOGI("Warning: mod[%d] waiting qof state pwr off timeout, qof may be voted by smi\n",
				ISP8_PWR_WPE_2_TNR);
			mtk_imgsys_cmdq_qof_dump(0, false);
			// engine off
		}
	} else {
		qof_engine_on_setting(mod);

		io_addr =  QOF_GET_REMAP_ADDR(qof_reg_table[mod][QOF_IMG_QOF_STATE_DBG].addr);
		if (readl_poll_timeout_atomic
			(io_addr, tmp, (tmp & BIT(1)) == BIT(1), POLL_DELAY_US, TIMEOUT_1000US) < 0) {
			QOF_LOGE("mod[%d] waiting for qof state pwr on timeout, disable support\n",mod);
			mtk_imgsys_cmdq_qof_dump(0, false);
			g_qof_ver &= (~BIT(mod));
			return;
		}

		/* qof APMCU Vote sub */
		io_addr = QOF_GET_REMAP_ADDR(qof_reg_table[mod][QOF_IMG_APMCU_CLR].addr);
		write_mask(io_addr,
			qof_reg_table[mod][QOF_IMG_APMCU_CLR].val,
			qof_reg_table[mod][QOF_IMG_APMCU_CLR].mask);

		io_addr = QOF_GET_REMAP_ADDR(qof_reg_table[mod][QOF_IMG_QOF_STATE_DBG].addr);
		if (readl_poll_timeout_atomic
			(io_addr, tmp, (tmp & BIT(4)) == BIT(4), POLL_DELAY_US, TIMEOUT_1000US) < 0) {
			QOF_LOGI("Warning: mod[%d] waiting qof state pwr off timeout, qof may be voted by smi\n",mod);
			mtk_imgsys_cmdq_qof_dump(0, false);
			// engine off
		}
	}
}

static void qof_engine_off_setting(const u32 mod)
{
	void __iomem *io_addr;
	// disable hw_seq
	io_addr =  QOF_GET_REMAP_ADDR(qof_reg_table[mod][QOF_IMG_HW_SEQ_EN].addr);
	write_mask(io_addr,
		0,
		qof_reg_table[mod][QOF_IMG_HW_SEQ_EN].mask);
	// Set engine enable = 0
	io_addr =  QOF_GET_REMAP_ADDR(qof_reg_table[mod][QOF_IMG_QOF_ENG_EN].addr);
	write_mask(io_addr,
		0,
		qof_reg_table[mod][QOF_IMG_QOF_ENG_EN].mask);
	io_addr =  QOF_GET_REMAP_ADDR(qof_reg_table[mod][QOF_IMG_APMCU_CLR].addr);
	write_mask(io_addr,
		qof_reg_table[mod][QOF_IMG_APMCU_CLR].val,
		qof_reg_table[mod][QOF_IMG_APMCU_CLR].mask);
}

static void qof_locked_set_engine_off(const u32 mod)
{
	void __iomem *io_addr;
	int tmp;

	if (IS_MOD_SUPPORT_QOF(mod) == false)
		return;

	if ((g_qof_work_buf_va == NULL) || (*g_qof_work_buf_va == NULL)) {
		QOF_LOGE("param is null %d\n",
			(g_qof_work_buf_va == NULL));
		return;
	}

	if (mod == ISP8_PWR_WPE_1_EIS) { // wa for gce thd lacked
		io_addr =  QOF_GET_REMAP_ADDR(qof_reg_table[ISP8_PWR_WPE_1_EIS][QOF_IMG_APMCU_SET].addr);
		write_mask(io_addr,
			qof_reg_table[ISP8_PWR_WPE_1_EIS][QOF_IMG_APMCU_SET].val,
			qof_reg_table[ISP8_PWR_WPE_1_EIS][QOF_IMG_APMCU_SET].mask);
		io_addr =  QOF_GET_REMAP_ADDR(qof_reg_table[ISP8_PWR_WPE_2_TNR][QOF_IMG_APMCU_SET].addr);
		write_mask(io_addr,
			qof_reg_table[ISP8_PWR_WPE_2_TNR][QOF_IMG_APMCU_SET].val,
			qof_reg_table[ISP8_PWR_WPE_2_TNR][QOF_IMG_APMCU_SET].mask);

		io_addr =  QOF_GET_REMAP_ADDR(qof_reg_table[ISP8_PWR_WPE_1_EIS][QOF_IMG_QOF_STATE_DBG].addr);
		if (readl_poll_timeout_atomic
			(io_addr, tmp, (tmp & BIT(1)) == BIT(1), POLL_DELAY_US, TIMEOUT_100000US) < 0) {
			QOF_LOGE("fatal error: mod[%d] waiting for qof state pwr on timeout, disable support\n",
				ISP8_PWR_WPE_1_EIS);
			mtk_imgsys_cmdq_qof_dump(0, false);
			g_qof_ver &= (~BIT(ISP8_PWR_WPE_1_EIS));
		}
		io_addr =  QOF_GET_REMAP_ADDR(qof_reg_table[ISP8_PWR_WPE_2_TNR][QOF_IMG_QOF_STATE_DBG].addr);
		if (readl_poll_timeout_atomic
			(io_addr, tmp, (tmp & BIT(1)) == BIT(1), POLL_DELAY_US, TIMEOUT_100000US) < 0) {
			QOF_LOGE("fatal error: mod[%d] waiting for qof state pwr on timeout, disable support\n",
				ISP8_PWR_WPE_2_TNR);
			mtk_imgsys_cmdq_qof_dump(0, false);
			g_qof_ver &= (~BIT(ISP8_PWR_WPE_2_TNR));
		}
		qof_engine_off_setting(ISP8_PWR_WPE_1_EIS);
		qof_engine_off_setting(ISP8_PWR_WPE_2_TNR);
	} else {
		io_addr =  QOF_GET_REMAP_ADDR(qof_reg_table[mod][QOF_IMG_APMCU_SET].addr);
		write_mask(io_addr,
			qof_reg_table[mod][QOF_IMG_APMCU_SET].val,
			qof_reg_table[mod][QOF_IMG_APMCU_SET].mask);

		io_addr =  QOF_GET_REMAP_ADDR(qof_reg_table[mod][QOF_IMG_QOF_STATE_DBG].addr);
		if (readl_poll_timeout_atomic
			(io_addr, tmp, (tmp & BIT(1)) == BIT(1), POLL_DELAY_US, TIMEOUT_100000US) < 0) {
			QOF_LOGE("fatal error: mod[%d] waiting for qof state pwr on timeout, disable support\n", mod);
			mtk_imgsys_cmdq_qof_dump(0, false);
			g_qof_ver &= (~BIT(mod));
		}
		qof_engine_off_setting(mod);
	}
}

static void qof_locked_stream_off_sync(void)
{
	u32 qof_module = QOF_SUPPORT_START;

	QOF_LOGI("qof stream off+\n");
	for (; qof_module < QOF_TOTAL_MODULE; qof_module++) {
		/* engine off */
		if (qof_module == QOF_SUPPORT_WPE_TNR) // wa for gce thd lackd
			continue;
		qof_locked_set_engine_off(qof_module);
	}
	QOF_LOGI("qof stream off-\n");
}

void mtk_imgsys_cmdq_qof_stream_on(struct mtk_imgsys_dev *imgsys_dev)
{
	u32 mod = 0;
	unsigned long flag;

	QOF_LOGI("qof stream on+\n");

	qof_start_all_gce_loop(imgsys_dev);

	spin_lock_irqsave(&qof_lock, flag);

	pm_runtime_get_noresume(g_imgsys_dev->dev);

	for (mod = ISP8_PWR_START; mod < ISP8_PWR_NUM; mod++) {
		if (mod == QOF_SUPPORT_WPE_TNR) // wa for gce thd lacked
			continue;
		if (IS_MOD_SUPPORT_QOF(mod))
			qof_set_engine_on(mod);
		else
			QOF_LOGI("module[%u] not support QOF, ver[%u]\n", mod, g_qof_ver);
	}

	mtk_imgsys_cmdq_qof_dump(0, false);

	imgsys_voter_cnt_locked++;

	spin_unlock_irqrestore(&qof_lock, flag);

	imgsys_qof_set_dbg_thread(true);

	QOF_LOGI("stream on-. success. imgsys_voter_cnt_locked=%u\n", imgsys_voter_cnt_locked);
}

void mtk_imgsys_cmdq_get_non_qof_module(u32 *non_qof_modules)
{
	if (non_qof_modules == NULL) {
		QOF_LOGE("ver[%u], param is null!\n", g_qof_ver);
		return;
	}

	*non_qof_modules = 0;
	if ((g_qof_ver & BIT(QOF_SUPPORT_DIP)) == BIT(QOF_SUPPORT_DIP))
		*non_qof_modules |= BIT(IMGSYS_MOD_DIP);
	if ((g_qof_ver & BIT(QOF_SUPPORT_TRAW)) == BIT(QOF_SUPPORT_TRAW))
		*non_qof_modules |= BIT(IMGSYS_MOD_TRAW);
	if (((g_qof_ver & BIT(QOF_SUPPORT_WPE_EIS)) == BIT(QOF_SUPPORT_WPE_EIS)) &&
		((g_qof_ver & BIT(QOF_SUPPORT_WPE_TNR)) == BIT(QOF_SUPPORT_WPE_TNR)) &&
		((g_qof_ver & BIT(QOF_SUPPORT_WPE_LITE)) == BIT(QOF_SUPPORT_WPE_LITE))) {
		*non_qof_modules |=
			(
				BIT(IMGSYS_MOD_PQDIP) |
				BIT(IMGSYS_MOD_WPE) |
				BIT(IMGSYS_MOD_OMC)
			);
	}
	*non_qof_modules = ~*non_qof_modules;
}

void mtk_imgsys_cmdq_qof_stream_off(struct mtk_imgsys_dev *imgsys_dev)
{
	unsigned long flag;

	imgsys_qof_set_dbg_thread(false);

	spin_lock_irqsave(&qof_lock, flag);

	imgsys_voter_cnt_locked--;

	qof_locked_stream_off_sync();

	pm_runtime_put_noidle(g_imgsys_dev->dev);

	spin_unlock_irqrestore(&qof_lock, flag);

	qof_stop_all_gce_loop();

	QOF_LOGI("stream off success. imgsys_voter_cnt_locked=%u\n", imgsys_voter_cnt_locked);
}

static bool qof_poll_pwr_status(u32 pwr, bool enable)
{
	void __iomem *io_addr;
	u32 tmp;
	u32 val = (enable?BIT(1):BIT(4));

	io_addr = QOF_GET_REMAP_ADDR(qof_reg_table[pwr][QOF_IMG_POWER_STATE].addr);
	if (readl_poll_timeout_atomic
		(io_addr, tmp, (tmp & val) == val, POLL_DELAY_US, TIMEOUT_1000US) < 0) {
		QOF_LOGE("mod[%d] waiting for qof state ap add done timeout\n",pwr);
		mtk_imgsys_cmdq_qof_dump(0, false);
		return false;
	}
	return true;
}

static void qof_module_vote_add(struct cmdq_pkt *pkt, u32 pwr, u32 user)
{
	struct qof_events *qof_event;
	void __iomem *io_addr;
	u32 tmp;

	qof_event = &qof_events_isp8[pwr];
	if (user == QOF_USER_AP) {
		//add voter
		io_addr = QOF_GET_REMAP_ADDR(qof_reg_table[pwr][QOF_IMG_EVENT_CNT_ADD].addr);
		write_mask(io_addr,
			qof_reg_table[pwr][QOF_IMG_EVENT_CNT_ADD].val,
			qof_reg_table[pwr][QOF_IMG_EVENT_CNT_ADD].mask);

		io_addr = QOF_GET_REMAP_ADDR(qof_reg_table[pwr][QOF_IMG_POWER_STATE].addr);
		if (readl_poll_timeout_atomic
			(io_addr, tmp, (tmp & BIT(1)) == BIT(1), POLL_DELAY_US, TIMEOUT_1000US) < 0) {
			QOF_LOGE("mod[%d] waiting for qof state ap add done timeout\n",pwr);
			mtk_imgsys_cmdq_qof_dump(0, false);
			return;
		}
	} else if (user == QOF_USER_GCE_SWWA) { // wa for gce thd lacked
		/* Enter critical section */
		cmdq_pkt_acquire_event(pkt, qof_event->sw_event_lock);
		cmdq_pkt_write(pkt, NULL, (qof_reg_table[ISP8_PWR_WPE_1_EIS][QOF_IMG_EVENT_CNT_ADD].addr) /*address*/,
			qof_reg_table[ISP8_PWR_WPE_1_EIS][QOF_IMG_EVENT_CNT_ADD].val,
			qof_reg_table[ISP8_PWR_WPE_1_EIS][QOF_IMG_EVENT_CNT_ADD].mask);
		cmdq_pkt_write(pkt, NULL, (qof_reg_table[ISP8_PWR_WPE_2_TNR][QOF_IMG_EVENT_CNT_ADD].addr) /*address*/,
			qof_reg_table[ISP8_PWR_WPE_2_TNR][QOF_IMG_EVENT_CNT_ADD].val,
			qof_reg_table[ISP8_PWR_WPE_2_TNR][QOF_IMG_EVENT_CNT_ADD].mask);
		cmdq_pkt_poll_sleep(pkt, BIT(1)/*poll val*/,
			(qof_reg_table[ISP8_PWR_WPE_1_EIS][QOF_IMG_QOF_STATE_DBG].addr), BIT(1) /*mask*/);
		cmdq_pkt_poll_sleep(pkt, BIT(1)/*poll val*/,
			(qof_reg_table[ISP8_PWR_WPE_2_TNR][QOF_IMG_QOF_STATE_DBG].addr), BIT(1) /*mask*/);
		/* End of critical section */
		cmdq_pkt_clear_event(pkt, qof_event->sw_event_lock);
	} else {
		if (pkt == NULL) {
			QOF_LOGE("parameter wrong !\n");
			return;
		}
		/* Enter critical section */
		cmdq_pkt_acquire_event(pkt, qof_event->sw_event_lock);
		cmdq_pkt_write(pkt, NULL, (qof_reg_table[pwr][QOF_IMG_EVENT_CNT_ADD].addr) /*address*/,
			qof_reg_table[pwr][QOF_IMG_EVENT_CNT_ADD].val,
			qof_reg_table[pwr][QOF_IMG_EVENT_CNT_ADD].mask);
		cmdq_pkt_poll_sleep(pkt, BIT(1)/*poll val*/,
			(qof_reg_table[pwr][QOF_IMG_QOF_STATE_DBG].addr)/*addr*/, BIT(1) /*mask*/);
		/* End of critical section */
		cmdq_pkt_clear_event(pkt, qof_event->sw_event_lock);
	}
}

static void qof_module_vote_sub(struct cmdq_pkt *pkt, u32 pwr, u32 user)
{
	struct qof_events *qof_event;
	void __iomem *io_addr;

	if(IS_MOD_SUPPORT_QOF(pwr) == false)
		return;

	if (user == QOF_USER_AP) {
		// sub voter
		io_addr = QOF_GET_REMAP_ADDR(qof_reg_table[pwr][QOF_IMG_EVENT_CNT_SUB].addr);
		write_mask(io_addr,
			qof_reg_table[pwr][QOF_IMG_EVENT_CNT_SUB].val,
			qof_reg_table[pwr][QOF_IMG_EVENT_CNT_SUB].mask);
	} else if (user == QOF_USER_GCE_SWWA) { // wa for gce thd lacked
		if (pkt == NULL) {
			QOF_LOGE("parameter wrong !\n");
			return;
		}
		qof_event = &qof_events_isp8[pwr];

		/* Enter critical section */
		cmdq_pkt_acquire_event(pkt, qof_event->sw_event_lock);

		cmdq_pkt_write(pkt, NULL, (qof_reg_table[ISP8_PWR_WPE_1_EIS][QOF_IMG_EVENT_CNT_SUB].addr) /*address*/,
			qof_reg_table[ISP8_PWR_WPE_1_EIS][QOF_IMG_EVENT_CNT_SUB].val,
			qof_reg_table[ISP8_PWR_WPE_1_EIS][QOF_IMG_EVENT_CNT_SUB].mask);
		cmdq_pkt_write(pkt, NULL, (qof_reg_table[ISP8_PWR_WPE_2_TNR][QOF_IMG_EVENT_CNT_SUB].addr) /*address*/,
			qof_reg_table[ISP8_PWR_WPE_2_TNR][QOF_IMG_EVENT_CNT_SUB].val,
			qof_reg_table[ISP8_PWR_WPE_2_TNR][QOF_IMG_EVENT_CNT_SUB].mask);

		cmdq_pkt_poll_sleep(pkt, 0/*poll val*/,
			(qof_reg_table[ISP8_PWR_WPE_1_EIS][QOF_IMG_QOF_STATE_DBG].addr)/*addr*/, BIT(3) /*mask*/);
		cmdq_pkt_poll_sleep(pkt, 0/*poll val*/,
			(qof_reg_table[ISP8_PWR_WPE_2_TNR][QOF_IMG_QOF_STATE_DBG].addr)/*addr*/, BIT(3) /*mask*/);

		/* End of critical section */
		cmdq_pkt_clear_event(pkt, qof_event->sw_event_lock);
	} else {
		if (pkt == NULL) {
			QOF_LOGE("parameter wrong !\n");
			return;
		}
		qof_event = &qof_events_isp8[pwr];

		/* Enter critical section */
		cmdq_pkt_acquire_event(pkt, qof_event->sw_event_lock);

		cmdq_pkt_write(pkt, NULL, (qof_reg_table[pwr][QOF_IMG_EVENT_CNT_SUB].addr) /*address*/,
			qof_reg_table[pwr][QOF_IMG_EVENT_CNT_SUB].val,
			qof_reg_table[pwr][QOF_IMG_EVENT_CNT_SUB].mask);

		cmdq_pkt_poll_sleep(pkt, 0/*poll val*/,
			(qof_reg_table[pwr][QOF_IMG_QOF_STATE_DBG].addr)/*addr*/, BIT(3) /*mask*/);

		/* End of critical section */
		cmdq_pkt_clear_event(pkt, qof_event->sw_event_lock);
	}
}

void mtk_imgsys_cmdq_qof_add(struct cmdq_pkt *pkt, bool *qof_need_sub, u32 hw_comb)
{
	u32 pwr = 0;

	if(g_qof_debug_level == QOF_DEBUG_MODE_PERFRAME_DUMP)
		mtk_imgsys_cmdq_qof_dump(0, false);

	for (pwr = ISP8_PWR_START; pwr < ISP8_PWR_NUM; pwr++) {
		if ((IS_MOD_SUPPORT_QOF(pwr)) &&
			(qof_need_sub[pwr] == false) &&
			(hw_comb & pwr_group[pwr])) {
			if (pwr == ISP8_PWR_WPE_2_TNR)
				continue;
			if (pwr == ISP8_PWR_WPE_1_EIS) // wa for gce thd lacked
				qof_module_vote_add(pkt, pwr, QOF_USER_GCE_SWWA);
			else
				qof_module_vote_add(pkt, pwr, QOF_USER_GCE);
			qof_need_sub[pwr] = true;
		}
	}
}

void mtk_imgsys_cmdq_qof_sub(struct cmdq_pkt *pkt, bool *qof_need_sub)
{
	u32 pwr = 0;

	for (pwr = ISP8_PWR_START; pwr < ISP8_PWR_NUM; pwr++) {
		if (qof_need_sub[pwr] == true) {
			if (pwr == ISP8_PWR_WPE_2_TNR)
				continue;
			if (pwr == ISP8_PWR_WPE_1_EIS) // wa for gce thd lacked
				qof_module_vote_sub(pkt, pwr, QOF_USER_GCE_SWWA);
			else
				qof_module_vote_sub(pkt, pwr, QOF_USER_GCE);
			qof_need_sub[pwr] = false;
		}
	}
}

void mtk_imgsys_cmdq_qof_dump(uint32_t hwcomb, bool need_dump_cg)
{
	int mod = 0;

	if (g_qof_ver == MTK_IMGSYS_QOF_FUNCTION_OFF)
		return;

	QOF_LOGI("Common info ver[%u], hwcomb[0x%x], need_dump_cg[%u]", g_qof_ver, hwcomb, need_dump_cg);
	if ((readl(g_maped_rg[MAPED_RG_ISP_MAIN_PWR_CON]) & (BIT(30)|BIT(31)))
		!= (BIT(30)|BIT(31))) {
		QOF_LOGI("isp_main is off. sta=0x%x",
			readl(g_maped_rg[MAPED_RG_ISP_MAIN_PWR_CON]));
		return;
	}
	for (mod = ISP8_PWR_START; mod < ISP8_PWR_NUM; mod++)
		mtk_imgsys_qof_print_hw_info(mod);

	mtk_qof_print_mtcmos_status();

	if (need_dump_cg)
		mtk_qof_print_cg_status();
}

static void imgsys_qof_dbg_print_trace(int mod)
{
	u32 value;
	char buf[128];
	int ret;

	ret = snprintf(buf, sizeof(buf),
		"qof_mod_%d",
		mod);
	if (ret < 0 || mod < 0 || mod >= QOF_TOTAL_MODULE) {
		pr_err("snprintf failed\n");
		return;
	}
	value = readl(QOF_GET_REMAP_ADDR(qof_reg_table[mod][QOF_IMG_POWER_STATE].addr));
	if ((value & BIT(1)) == BIT(1))
		value = 1;
	else
		value = 0;
	switch(mod) {
	case ISP8_PWR_DIP:
		ftrace_imgsys_qof_mod0("%s=%u",
			buf,
			(u32) (value & (0xff)));
		break;
	case ISP8_PWR_TRAW:
		ftrace_imgsys_qof_mod1("%s=%u",
			buf,
			(u32) (value & (0xff)));
		break;
	case ISP8_PWR_WPE_1_EIS:
		ftrace_imgsys_qof_mod2("%s=%u",
			buf,
			(u32) (value & (0xff)));
		break;
	case ISP8_PWR_WPE_2_TNR:
		ftrace_imgsys_qof_mod3("%s=%u",
			buf,
			(u32) (value & (0xff)));
		break;
	case ISP8_PWR_WPE_3_LITE:
		ftrace_imgsys_qof_mod4("%s=%u",
			buf,
			(u32) (value & (0xff)));
		break;
	}
}

static int imgsys_qof_dbg_thread(void *data)
{
	// loop to trace qof
	while (!kthread_should_stop()) {
		imgsys_qof_dbg_print_trace(ISP8_PWR_DIP);
		imgsys_qof_dbg_print_trace(ISP8_PWR_TRAW);
		imgsys_qof_dbg_print_trace(ISP8_PWR_WPE_1_EIS);
		imgsys_qof_dbg_print_trace(ISP8_PWR_WPE_2_TNR);
		imgsys_qof_dbg_print_trace(ISP8_PWR_WPE_3_LITE);
		usleep_range(g_ftrace_time, g_ftrace_time + 5);
	}
	return 0;
}

static void imgsys_qof_set_dbg_thread(bool enable)
{
	static struct task_struct *kthr;
	static bool dbg_en;

	if (imgsys_ftrace_qof_thread_en && enable && !dbg_en) {
		QOF_LOGI("thread START\n");
		kthr = kthread_run(imgsys_qof_dbg_thread,
				NULL, "imgsys-qof-dbg");
		if (IS_ERR(kthr))
			pr_info("[%s] create kthread imgsys-qof-dbg failed\n", __func__);
		else
			dbg_en = true;
	} else if (dbg_en) {
		kthread_stop(kthr);
		dbg_en = false;
	}
}

int mtk_imgsys_qof_ctrl(const char *val, const struct kernel_param *kp)
{
	int ret;
	ret = sscanf(val, "%u %u %u", &g_qof_debug_level, &g_qof_ver, &g_ftrace_time);
	QOF_LOGI("g_qof_debug_level[%u], force ver:g_qof_ver[%u], g_ftrace_time[%u]\n",
		g_qof_debug_level,
		g_qof_ver,
		g_ftrace_time);
	if (ret <= 0) {
		QOF_LOGE("sscanf ret is wrong %d\n", ret);
		return 0;
	}
	if (g_qof_debug_level == QOF_DEBUG_MODE_IMMEDIATE_DUMP)
		mtk_imgsys_cmdq_qof_dump(0, false);
	else if (g_qof_debug_level == QOF_DEBUG_MODE_IMMEDIATE_CG_DUMP)
		mtk_imgsys_cmdq_qof_dump(0, true);
	else if (g_qof_debug_level == QOF_DEBUG_MODE_FORCE_SMI_EVENT_POLL) {
		is_poll_event_mode = true;
		QOF_LOGI("set is_poll_event_mode=%d", is_poll_event_mode);
	}

	return 0;
}

static const struct kernel_param_ops qof_ctrl_ops = {
	.set = mtk_imgsys_qof_ctrl,
};

module_param_cb(imgsys_qof_ctrl, &qof_ctrl_ops, NULL, 0644);
MODULE_PARM_DESC(imgsys_qof_ctrl, "imgsys_qof_ctrl");
