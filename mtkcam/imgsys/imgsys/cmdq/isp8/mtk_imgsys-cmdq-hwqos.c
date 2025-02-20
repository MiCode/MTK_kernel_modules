// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 *
 * Author: Yuhsuan.chang <yuhsuan.chang@mediatek.com>
 *
 */

#include "mtk_imgsys-cmdq-hwqos.h"

#include <linux/bitfield.h>
#include <linux/bits.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/kstrtox.h>
#include <linux/kthread.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/soc/mediatek/mtk-cmdq-ext.h>

#include "mtk_imgsys-cmdq-plat.h"
#include "mtk_imgsys-dev.h"
#include "mtk_imgsys-hwqos-reg.h"
#include "mtk_imgsys-hwqos-data.h"
#include "mtk_imgsys-trace.h"

#define IMGSYS_QOS_THD_IDX (IMGSYS_NOR_THD + IMGSYS_PWR_THD)
#define CMDQ_REG_MASK GENMASK(31, 0)

#define CMDQ_GCEM_CPR_IMG_QOS_START 0x8005
#define CMDQ_GCEM_CPR_IMG_QOS_END 0x803D
#define CMDQ_GCEM_CPR_IMG_QOS_EXTENDED 0x8040
#define CMDQ_GCEM_CPR_IMG_QOS_EXTENDED_END 0x8060

#define CMDQ_CPR_IMG_QOS_IDX (CMDQ_GCEM_CPR_IMG_QOS_START)
#define CMDQ_CPR_IMG_QOS_SUM_R (CMDQ_GCEM_CPR_IMG_QOS_START + 1)
#define CMDQ_CPR_IMG_QOS_SUM_W (CMDQ_GCEM_CPR_IMG_QOS_START + 2)
#define CMDQ_CPR_IMG_QOS_AVG_R (CMDQ_GCEM_CPR_IMG_QOS_START + 3)
#define CMDQ_CPR_IMG_QOS_AVG_W (CMDQ_GCEM_CPR_IMG_QOS_START + 4)

#define CMDQ_CPR_IMG_QOS_CHN0_SUM_R (CMDQ_GCEM_CPR_IMG_QOS_EXTENDED)
#define CMDQ_CPR_IMG_QOS_CHN0_SUM_W (CMDQ_GCEM_CPR_IMG_QOS_EXTENDED + 1)
#define CMDQ_CPR_IMG_QOS_CHN1_SUM_R (CMDQ_GCEM_CPR_IMG_QOS_EXTENDED + 2)
#define CMDQ_CPR_IMG_QOS_CHN1_SUM_W (CMDQ_GCEM_CPR_IMG_QOS_EXTENDED + 3)

#define CMDQ_CPR_IMG_QOS_ACTIVE_COUNT (CMDQ_GCEM_CPR_IMG_QOS_EXTENDED + 10)
#define CMDQ_CPR_IMG_QOS_URATE_FACTOR (CMDQ_GCEM_CPR_IMG_QOS_EXTENDED + 11)


#define RIGHT_SHIFT_BY_3 (3)

#define IMGSYS_QOS_REPORT_NORMAL (0)
#define IMGSYS_QOS_REPORT_MAX (1)

#define OSTDL_MAX_VALUE 0x40
#define OSTDL_MIN_VALUE 0x1
#define OSTDL_R_RIGHT_SHIFT (6)  // 64
#define OSTDL_W_RIGHT_SHIFT (8)  // 256

#define STEP_FACTOR_MULTIPLY (4)
#define STEP_FACTOR_RIGHT_SHIFT (2)
#define STEP_FACTOR_ACTIVE_MULTIPLY (16)

#define QOS_AVERAGE_SHIFT (7)

#define field_get(_mask, _reg) (((_reg) & (_mask)) >> (ffs(_mask) - 1))

enum QOS_STATE {
	QOS_STATE_INIT = 0,
	QOS_STATE_NORMAL,
	QOS_STATE_MAX,
	QOS_STATE_UNINIT
};

enum GCE_COND_REVERSE_COND {
	R_CMDQ_NOT_EQUAL = CMDQ_EQUAL,
	R_CMDQ_EQUAL = CMDQ_NOT_EQUAL,
	R_CMDQ_LESS = CMDQ_GREATER_THAN_AND_EQUAL,
	R_CMDQ_GREATER = CMDQ_LESS_THAN_AND_EQUAL,
	R_CMDQ_LESS_EQUAL = CMDQ_GREATER_THAN,
	R_CMDQ_GREATER_EQUAL = CMDQ_LESS_THAN,
};

#define GCE_COND_DECLARE \
	u32 _inst_condi_jump, _inst_jump_end; \
	u64 _jump_pa; \
	u64 *_inst; \
	struct cmdq_pkt *_cond_pkt; \
	u16 _reg_jump

#define GCE_COND_ASSIGN(pkt, addr) do { \
	_cond_pkt = pkt; \
	_reg_jump = addr; \
} while (0)

#define GCE_IF(lop, cond, rop) do { \
	_inst_condi_jump = _cond_pkt->cmd_buf_size; \
	cmdq_pkt_assign_command(_cond_pkt, _reg_jump, 0); \
	cmdq_pkt_cond_jump_abs( \
		_cond_pkt, _reg_jump, &lop, &rop, (enum CMDQ_CONDITION_ENUM) cond); \
	_inst_jump_end = _inst_condi_jump; \
} while (0)

#define GCE_ELSE do { \
	_inst_jump_end = _cond_pkt->cmd_buf_size; \
	cmdq_pkt_jump_addr(_cond_pkt, 0); \
	_inst = cmdq_pkt_get_va_by_offset(_cond_pkt, _inst_condi_jump); \
	_jump_pa = cmdq_pkt_get_pa_by_offset(_cond_pkt, _cond_pkt->cmd_buf_size); \
	*_inst = *_inst & ((u64)0xFFFFFFFF << 32); \
	*_inst = *_inst | CMDQ_REG_SHIFT_ADDR(_jump_pa); \
} while (0)

#define GCE_FI do { \
	_inst = cmdq_pkt_get_va_by_offset(_cond_pkt, _inst_jump_end); \
	_jump_pa = cmdq_pkt_get_pa_by_offset(_cond_pkt, _cond_pkt->cmd_buf_size); \
	*_inst = *_inst & ((u64)0xFFFFFFFF << 32); \
	*_inst = *_inst | CMDQ_REG_SHIFT_ADDR(_jump_pa); \
} while (0)

static struct cmdq_client *g_hwqos_clt;
static struct cmdq_pkt *g_hwqos_pkt;
static enum QOS_STATE g_hwqos_state = QOS_STATE_INIT;
static dma_addr_t g_hwqos_buf_pa;
static u32 *g_hwqos_buf_va;

static bool g_hwqos_support;

// debug control
static int g_hwqos_active_thre = 85;
static int g_hwqos_active_factor = STEP_FACTOR_ACTIVE_MULTIPLY;
static bool g_hwqos_dbg_en;
static bool g_hwqos_sw_en;
static bool imgsys_ftrace_thread_en;

module_param(g_hwqos_active_thre, int, 0644);
MODULE_PARM_DESC(g_hwqos_active_thre, "imgsys hwqos active threshold");

module_param(g_hwqos_active_factor, int, 0644);
MODULE_PARM_DESC(g_hwqos_active_factor, "imgsys hwqos active factor");

module_param(g_hwqos_dbg_en, bool, 0644);
MODULE_PARM_DESC(g_hwqos_dbg_en, "imgsys hwqos log enable");

module_param(g_hwqos_sw_en, bool, 0644);
MODULE_PARM_DESC(g_hwqos_sw_en, "imgsys hwqos mode, sw=1, hw=0 (default)");

module_param(imgsys_ftrace_thread_en, bool, 0644);
MODULE_PARM_DESC(imgsys_ftrace_thread_en, "imgsys ftrace thread enable, 0 (default)");

/** Dump api
 * @brief List Dump api
 */

static void imgsys_hwqos_dbg_reg_read(u32 pa)
{
	u32 value = 0x0;
	void __iomem *va;

	va = ioremap(pa, 0x4);
	if (va) {
		value = readl((void *)va);
		ftrace_imgsys_hwqos_dbg_reg_read(pa, value);
		iounmap(va);
	} else {
		pr_info("[%s] addr:0x%08X ioremap fail!\n", __func__, pa);
	}
}

static void imgsys_reg_read_base_list(const uint32_t *reg_base_list,
					const uint32_t reg_base_list_size,
					const uint32_t offset)
{
	uint32_t i;

	for (i = 0; i < reg_base_list_size; i++)
		imgsys_hwqos_dbg_reg_read(reg_base_list[i] + offset);
}

static void imgsys_reg_read(const uint32_t *base_list,
				const uint32_t base_list_size,
				const struct reg *reg_list,
				const uint32_t reg_list_size)
{
	uint32_t i, j;

	for (j = 0; j < base_list_size; j++) {
		for (i = 0; i < reg_list_size; i++) {
			imgsys_hwqos_dbg_reg_read(
				base_list[j] + reg_list[i].offset);
		}
	}
}

static void imgsys_hwqos_dbg_reg_dump(uint8_t level)
{
	uint32_t i;

	if (level >= 3) {
		// BLS config
		imgsys_reg_read(bls_base_array, ARRAY_SIZE(bls_base_array),
					bls_init_data, ARRAY_SIZE(bls_init_data));
		imgsys_reg_read_base_list(bls_base_array, ARRAY_SIZE(bls_base_array),
					BLS_IMG_CTRL_OFT);
		// BWR config
		imgsys_reg_read(bwr_base_array, ARRAY_SIZE(bwr_base_array),
				bwr_init_data, ARRAY_SIZE(bwr_init_data));
		for (i = 0; i < ARRAY_SIZE(bwr_ctrl_data); i++) {
			imgsys_hwqos_dbg_reg_read(
				BWR_IMG_E1A_BASE + bwr_ctrl_data[i].qos_sel_offset);
			imgsys_hwqos_dbg_reg_read(
				BWR_IMG_E1A_BASE + bwr_ctrl_data[i].qos_en_offset);
		}
		for (i = 0; i < ARRAY_SIZE(qos_map_data); i++) {
			imgsys_hwqos_dbg_reg_read(
				BWR_IMG_E1A_BASE + qos_map_data[i].bwr_r_rat_offset);
			imgsys_hwqos_dbg_reg_read(
				BWR_IMG_E1A_BASE + qos_map_data[i].bwr_w_rat_offset);
		}
		imgsys_hwqos_dbg_reg_read(
			BWR_IMG_E1A_BASE + BWR_IMG_SRT_TTL_ENG_BW_RAT5_OFT);
	}

	if (level >= 1) {
		// BLS & BWR BW
		for (i = 0; i < ARRAY_SIZE(qos_map_data); i++) {
			if (level >= 2) {
				imgsys_hwqos_dbg_reg_read(
					qos_map_data[i].bls_base + BLS_IMG_LEN_SUM_R_OFT);
				imgsys_hwqos_dbg_reg_read(
					qos_map_data[i].bls_base + BLS_IMG_LEN_SUM_W_OFT);
			}
			imgsys_hwqos_dbg_reg_read(
				BWR_IMG_E1A_BASE + qos_map_data[i].bwr_r_offset);
			imgsys_hwqos_dbg_reg_read(
				BWR_IMG_E1A_BASE + qos_map_data[i].bwr_w_offset);
		}
		if (level >= 2) {
			imgsys_hwqos_dbg_reg_read(
				BLS_IMG_E1A_BASE + BLS_IMG_LEN_SUM_R_OFT);
			imgsys_hwqos_dbg_reg_read(
				BLS_IMG_E1A_BASE + BLS_IMG_LEN_SUM_W_OFT);
		}
		imgsys_hwqos_dbg_reg_read(
			BWR_IMG_E1A_BASE + BWR_IMG_SRT_TTL_ENG_BW5_OFT);
	}

	/* BWR status */
	imgsys_hwqos_dbg_reg_read(
		BWR_IMG_E1A_BASE + BWR_IMG_RPT_STATE_OFT);
	imgsys_hwqos_dbg_reg_read(
		BWR_IMG_E1A_BASE + BWR_IMG_SEND_BW_ZERO_OFT);
	imgsys_hwqos_dbg_reg_read(
		BWR_IMG_E1A_BASE + BWR_IMG_SEND_DONE_ST_OFT);
}

static int imgsys_hwqos_dbg_thread(void *data)
{
	unsigned int i = 0;
	u32 value;
	char buf[128];
	int ret;
	struct va_map {
		void __iomem *va;
		u32 value;
	};
	struct bw_item {
		struct va_map bwr_r;
		struct va_map bwr_w;
		struct va_map bls_r;
		struct va_map bls_w;
	};
	struct bw_item bw_data[ARRAY_SIZE(qos_map_data)] = {0};
	struct va_map img_hw_bw[ARRAY_SIZE(img_hw_bw_reg_array)] = {0};
	struct va_map bwr_ttl_bw[ARRAY_SIZE(bwr_ttl_bw_array)] = {0};
	struct va_map img_ostdl[ARRAY_SIZE(img_ostdl_array)] = {0};

	// map pa to va
	for (i = 0; i < ARRAY_SIZE(bw_data); i++) {
		bw_data[i].bwr_r.va = ioremap(BWR_IMG_E1A_BASE + qos_map_data[i].bwr_r_offset, 0x4);
		bw_data[i].bwr_w.va = ioremap(BWR_IMG_E1A_BASE + qos_map_data[i].bwr_w_offset, 0x4);
		bw_data[i].bls_r.va = ioremap(qos_map_data[i].bls_base + BLS_IMG_LEN_SUM_R_OFT, 0x4);
		bw_data[i].bls_w.va = ioremap(qos_map_data[i].bls_base + BLS_IMG_LEN_SUM_W_OFT, 0x4);
	}
	for (i = 0; i < ARRAY_SIZE(img_hw_bw); i++)
		img_hw_bw[i].va = ioremap(img_hw_bw_reg_array[i].addr, 0x4);
	for (i = 0; i < ARRAY_SIZE(bwr_ttl_bw); i++)
		bwr_ttl_bw[i].va = ioremap(bwr_ttl_bw_array[i].addr, 0x4);
	for (i = 0; i < ARRAY_SIZE(img_ostdl); i++)
		img_ostdl[i].va = ioremap(img_ostdl_array[i], 0x4);

	// loop to trace qos
	while (!kthread_should_stop()) {
		for (i = 0; i < ARRAY_SIZE(qos_map_data); i++) {
			// ratio: 1 / BIT(BWR_BW_POINT) * BW_RAT * BUS_URATE
			ret = snprintf(buf, sizeof(buf),
				"core%d_sc%d_eng%d_larb%d",
				qos_map_data[i].core,
				qos_map_data[i].sub_common,
				qos_map_data[i].engine,
				qos_map_data[i].larb);
			if (ret < 0) {
				pr_err("snprintf failed\n");
				continue;
			}
			value = readl((void *)bw_data[i].bwr_r.va);
			MTK_IMGSYS_QOS_ENABLE(value != bw_data[i].bwr_r.value,
				ftrace_imgsys_hwqos_bwr("%s_%s=%u",
					"r", buf,
					(u32) (value * 177 / 1024));
				bw_data[i].bwr_r.value = value;
			);
			value = readl((void *)bw_data[i].bwr_w.va);
			MTK_IMGSYS_QOS_ENABLE(value != bw_data[i].bwr_w.value,
				ftrace_imgsys_hwqos_bwr("%s_%s=%u",
					"w", buf,
					(u32) (value * 177 / 1024));
				bw_data[i].bwr_w.value = value;
			);

			// ratio: BLS_UNIT * 1000 / (1 << 20);
			value = readl((void *)bw_data[i].bls_r.va);
			MTK_IMGSYS_QOS_ENABLE(value != bw_data[i].bls_r.value,
				ftrace_imgsys_hwqos_bls("%s_%s=%u",
					"r", buf,
					(u32) (value / 66));
				bw_data[i].bls_r.value = value;
			);
			value = readl((void *)bw_data[i].bls_w.va);
			MTK_IMGSYS_QOS_ENABLE(value != bw_data[i].bls_w.value,
				ftrace_imgsys_hwqos_bls("%s_%s=%u",
					"w", buf,
					(u32) (value / 66));
				bw_data[i].bls_w.value = value;
			);
		}

		/* Read/Write AXI BW */
		for (i = 0; i < ARRAY_SIZE(img_hw_bw) - 1; i++) {
			value = field_get(img_hw_bw_reg_array[i].mask, readl((void *)img_hw_bw[i].va));
			MTK_IMGSYS_QOS_ENABLE(value != img_hw_bw[i].value,
				ftrace_imgsys_hwqos_bwr("%s%u=%u",
					"img_hw_bw", i, value * 16);
				img_hw_bw[i].value = value;
			);
		}
		/* Total BW */
		value = field_get(img_hw_bw_reg_array[i].mask, readl((void *)img_hw_bw[i].va));
		MTK_IMGSYS_QOS_ENABLE(value != img_hw_bw[i].value,
			ftrace_imgsys_hwqos_bwr("%s%u=%u",
				"img_hw_bw", i, value * 64);
			img_hw_bw[i].value = value;
		);
		/* BWR ttl BW */
		for (i = 0; i < ARRAY_SIZE(bwr_ttl_bw); i++) {
			value = field_get(bwr_ttl_bw_array[i].mask, readl((void *)bwr_ttl_bw[i].va));
			MTK_IMGSYS_QOS_ENABLE(value != bwr_ttl_bw[i].value,
				ftrace_imgsys_hwqos_bwr("%s%u=%u",
				"bwr_ttl_bw", i, value * 5 / 4);
				bwr_ttl_bw[i].value = value;
			);
		}
		for (i = 0; i < ARRAY_SIZE(img_ostdl); i++) {
			value = readl((void *)img_ostdl[i].va);
			MTK_IMGSYS_QOS_ENABLE(value != img_ostdl[i].value,
				ftrace_imgsys_hwqos_ostdl("%s_%s%u=%u",
					"r", "img_comm", i, FIELD_GET(OSTDL_R_REG_MASK, value));
				ftrace_imgsys_hwqos_ostdl("%s_%s%u=%u",
					"w", "img_comm", i, FIELD_GET(OSTDL_W_REG_MASK, value));
				img_ostdl[i].value = value;
			);
		}
		usleep_range(1000, 1050);
	}

	// unmap va
	for (i = 0; i < ARRAY_SIZE(bw_data); i++) {
		iounmap(bw_data[i].bwr_r.va);
		iounmap(bw_data[i].bwr_w.va);
		iounmap(bw_data[i].bls_r.va);
		iounmap(bw_data[i].bls_w.va);
	}

	for (i = 0; i < ARRAY_SIZE(img_hw_bw); i++)
		iounmap(img_hw_bw[i].va);
	for (i = 0; i < ARRAY_SIZE(bwr_ttl_bw); i++)
		iounmap(bwr_ttl_bw[i].va);
	for (i = 0; i < ARRAY_SIZE(img_ostdl); i++)
		iounmap(img_ostdl[i].va);

	return 0;
}

static void imgsys_hwqos_set_dbg_thread(bool enable)
{
	static struct task_struct *kthr;
	static bool dbg_en;

	if (imgsys_ftrace_thread_en && enable && !dbg_en) {
		kthr = kthread_run(imgsys_hwqos_dbg_thread,
				NULL, "imgsys-dbg");
		if (IS_ERR(kthr))
			pr_info("[%s] create kthread imgsys-dbg failed\n", __func__);
		else
			dbg_en = true;
	} else if (dbg_en) {
		kthread_stop(kthr);
		dbg_en = false;
	}
}

/** Local api
 * @brief List Local api
 */

static void imgsys_reg_write(struct cmdq_pkt *pkt,
				const uint32_t *base_list,
				const uint32_t base_list_size,
				const struct reg *reg_list,
				const uint32_t reg_list_size)
{
	uint32_t i, j;

	for (j = 0; j < base_list_size; j++) {
		for (i = 0; i < reg_list_size; i++) {
			cmdq_pkt_write(pkt, NULL,
				base_list[j] + reg_list[i].offset,
				reg_list[i].value, CMDQ_REG_MASK);
		}
	}
}

static void imgsys_reg_write_base_list(struct cmdq_pkt *pkt,
					const uint32_t *reg_base_list,
					const uint32_t reg_base_list_size,
					const uint32_t offset,
					const uint32_t value)
{
	uint32_t i;

	for (i = 0; i < reg_base_list_size; i++) {
		cmdq_pkt_write(pkt, NULL, reg_base_list[i] + offset,
				value, CMDQ_REG_MASK);
	}
}

static void imgsys_qos_set_report_mode(struct cmdq_pkt *pkt)
{
	uint32_t i, sel_reg_value, en_reg_value;

	for (i = 0; i < ARRAY_SIZE(bwr_ctrl_data); i++) {
		if (g_hwqos_sw_en) {
			sel_reg_value = 0;
			en_reg_value = bwr_ctrl_data[i].eng_en_value;
		} else /* hw mode */ {
			sel_reg_value = bwr_ctrl_data[i].eng_en_value;
			en_reg_value = 0;
		}
		cmdq_pkt_write(pkt, NULL,
			BWR_IMG_E1A_BASE + bwr_ctrl_data[i].qos_sel_offset,
			sel_reg_value, CMDQ_REG_MASK);
		cmdq_pkt_write(pkt, NULL,
			BWR_IMG_E1A_BASE + bwr_ctrl_data[i].qos_en_offset,
			en_reg_value, CMDQ_REG_MASK);
	}
}

static void imgsys_qos_to_MBps(struct cmdq_pkt *pkt,
				const uint32_t bls_addr,
				const uint32_t bwr_addr,
				const uint16_t cpr_avg_idx,
				const uint16_t cpr_sum_idx)
{
	struct cmdq_operand lop, rop;

	GCE_COND_DECLARE;


	lop.reg = true;
	lop.idx = CMDQ_THR_SPR_IDX2;
	rop.reg = false;
	rop.value = RIGHT_SHIFT_BY_3;
	cmdq_pkt_read(pkt, NULL, bls_addr, CMDQ_THR_SPR_IDX2);
	cmdq_pkt_logic_command(pkt, CMDQ_LOGIC_RIGHT_SHIFT,
		CMDQ_THR_SPR_IDX2, &lop, &rop);


	lop.reg = true;
	lop.idx = CMDQ_CPR_IMG_QOS_URATE_FACTOR;
	rop.reg = false;
	rop.value = STEP_FACTOR_MULTIPLY;
	GCE_COND_ASSIGN(pkt, CMDQ_THR_SPR_IDX1);
	GCE_IF(lop, R_CMDQ_EQUAL, rop);
	{

		// BW = (BW * STEP_FACTOR_MULTIPLY) >> STEP_FACTOR_RIGHT_SHIFT
		lop.reg = true;
		lop.idx = CMDQ_THR_SPR_IDX2;
		rop.reg = false;
		rop.value = STEP_FACTOR_MULTIPLY;
		cmdq_pkt_logic_command(pkt, CMDQ_LOGIC_MULTIPLY,
			CMDQ_THR_SPR_IDX2, &lop, &rop);
		rop.value = STEP_FACTOR_RIGHT_SHIFT;
		cmdq_pkt_logic_command(pkt, CMDQ_LOGIC_RIGHT_SHIFT,
			CMDQ_THR_SPR_IDX2, &lop, &rop);
	}
	GCE_ELSE;
	{
		// BW = (BW * STEP_FACTOR_URATE_MULTIPLY) >> STEP_FACTOR_RIGHT_SHIFT
		lop.reg = true;
		lop.idx = CMDQ_THR_SPR_IDX2;
		rop.reg = false;
		rop.value = g_hwqos_active_factor;
		cmdq_pkt_logic_command(pkt, CMDQ_LOGIC_MULTIPLY,
			CMDQ_THR_SPR_IDX2, &lop, &rop);
		rop.value = STEP_FACTOR_RIGHT_SHIFT;
		cmdq_pkt_logic_command(pkt, CMDQ_LOGIC_RIGHT_SHIFT,
			CMDQ_THR_SPR_IDX2, &lop, &rop);
	}
	GCE_FI;
	rop.reg = true;
	rop.idx = cpr_sum_idx;
	cmdq_pkt_logic_command(pkt, CMDQ_LOGIC_ADD,
		cpr_sum_idx, &lop, &rop);
}

static void imgsys_qos_set_ostdl_en(struct cmdq_pkt *pkt,
				const uint8_t ostdl_en)
{
	uint32_t i;
	uint32_t ostdl_val, ostdl_mask;

	if (ostdl_en == 1) {
		ostdl_val = FIELD_PREP(OSTDL_EN_REG_MASK, OSTDL_EN) |
					FIELD_PREP(OSTDL_R_REG_MASK, OSTDL_MIN_VALUE) |
					FIELD_PREP(OSTDL_W_REG_MASK, OSTDL_MIN_VALUE);
		ostdl_mask = OSTDL_EN_REG_MASK | OSTDL_R_REG_MASK | OSTDL_W_REG_MASK;
	} else {
		ostdl_val = 0;
		ostdl_mask = OSTDL_EN_REG_MASK;
	}

	for (i = 0; i < ARRAY_SIZE(img_ostdl_array); i++) {
		cmdq_pkt_write(pkt, NULL,
				img_ostdl_array[i],
				ostdl_val,
				ostdl_mask);
	}
}

static void imgsys_qos_set_bw_ratio(struct cmdq_pkt *pkt)
{
	uint32_t i;
	uint32_t bwr_ttl_bw_rat_offset_array[] = {
		BWR_IMG_SRT_TTL_ENG_BW_RAT0_OFT,
		BWR_IMG_SRT_TTL_ENG_BW_RAT1_OFT,
		BWR_IMG_SRT_TTL_ENG_BW_RAT2_OFT,
		BWR_IMG_SRT_TTL_ENG_BW_RAT3_OFT,
		BWR_IMG_SRT_TTL_ENG_BW_RAT4_OFT,
		BWR_IMG_SRT_TTL_ENG_BW_RAT5_OFT,
		BWR_IMG_SRT_TTL_ENG_BW_RAT7_OFT,
	};

	// set bwr read & write reg
	for (i = 0; i < ARRAY_SIZE(qos_map_data); i++) {
		cmdq_pkt_write(pkt, NULL,
				BWR_IMG_E1A_BASE + qos_map_data[i].bwr_r_rat_offset,
				BWR_IMG_SRT_ENG_BW_RAT, CMDQ_REG_MASK);
		cmdq_pkt_write(pkt, NULL,
				BWR_IMG_E1A_BASE + qos_map_data[i].bwr_w_rat_offset,
				BWR_IMG_SRT_ENG_BW_RAT, CMDQ_REG_MASK);
	}
	for (i = 0; i < ARRAY_SIZE(bwr_ttl_bw_rat_offset_array); i++) {
		cmdq_pkt_write(pkt, NULL,
				BWR_IMG_E1A_BASE + bwr_ttl_bw_rat_offset_array[i],
				BWR_IMG_SRT_ENG_BW_RAT, CMDQ_REG_MASK);
	}
}

static void imgsys_qos_set_fix_bw(struct cmdq_pkt *pkt,
					const uint32_t bwr_bw, const uint32_t total_bw)
{
	uint32_t i, ostdl_r, ostdl_w, bw;

	// set bwr bw and ostdl
	bw = bwr_bw >> BWR_BW_POINT;
	for (i = 0; i < ARRAY_SIZE(qos_map_data); i++) {
		cmdq_pkt_write(pkt, NULL,
				BWR_IMG_E1A_BASE + qos_map_data[i].bwr_r_offset,
				bwr_bw, CMDQ_REG_MASK);
		cmdq_pkt_write(pkt, NULL,
				BWR_IMG_E1A_BASE + qos_map_data[i].bwr_w_offset,
				bwr_bw, CMDQ_REG_MASK);
	}
	ostdl_r = bw >> OSTDL_R_RIGHT_SHIFT;
	ostdl_r = clamp_t(u32, ostdl_r, OSTDL_MIN_VALUE, OSTDL_MAX_VALUE);
	ostdl_w = bw >> OSTDL_W_RIGHT_SHIFT;
	ostdl_w = clamp_t(u32, ostdl_w, OSTDL_MIN_VALUE, OSTDL_MAX_VALUE);
	for (i = 0; i < ARRAY_SIZE(img_ostdl_array); i++) {
		cmdq_pkt_write(pkt, NULL,
				img_ostdl_array[i],
				FIELD_PREP(OSTDL_R_REG_MASK, ostdl_r) | FIELD_PREP(OSTDL_W_REG_MASK, ostdl_w),
				OSTDL_R_REG_MASK | OSTDL_W_REG_MASK);
	}
	if (total_bw == 0) {
		for (i = 0; i < ARRAY_SIZE(bwr_ttl_bw_array); i++) {
			cmdq_pkt_write(pkt, NULL,
					bwr_ttl_bw_array[i].addr,
					total_bw, CMDQ_REG_MASK);
		}
	} else {
		cmdq_pkt_write(pkt, NULL, BWR_IMG_E1A_BASE + BWR_IMG_SRT_TTL_ENG_BW5_OFT,
			total_bw, CMDQ_REG_MASK);
	}
}

static void imgsys_qos_init_counter(struct cmdq_pkt *pkt, bool clear_avg)
{
	uint32_t i;
	struct cmdq_operand lop, rop;

	GCE_COND_DECLARE;

	cmdq_pkt_assign_command(pkt, CMDQ_CPR_IMG_QOS_IDX, 0);
	for (i = 0; i < ARRAY_SIZE(qos_map_data); i++) {
		cmdq_pkt_assign_command(pkt, CMDQ_CPR_IMG_QOS_SUM_R + i * 4, 0);
		cmdq_pkt_assign_command(pkt, CMDQ_CPR_IMG_QOS_SUM_W + i * 4, 0);
	}
	if (clear_avg) {
		for (i = 0; i < ARRAY_SIZE(qos_map_data); i++) {
			cmdq_pkt_assign_command(pkt, CMDQ_CPR_IMG_QOS_AVG_R + i * 4, 0);
			cmdq_pkt_assign_command(pkt, CMDQ_CPR_IMG_QOS_AVG_W + i * 4, 0);
		}
		// active count = 100% = 128
		cmdq_pkt_assign_command(pkt, CMDQ_CPR_IMG_QOS_ACTIVE_COUNT,
			(1 << QOS_AVERAGE_SHIFT));
	}
	// case:  active rate > 4/10
	lop.reg = true;
	lop.idx = CMDQ_CPR_IMG_QOS_ACTIVE_COUNT;
	rop.reg = false;
	rop.idx = g_hwqos_active_thre;
	GCE_COND_ASSIGN(pkt, CMDQ_THR_SPR_IDX1);
	GCE_IF(lop, R_CMDQ_GREATER_EQUAL, rop);
	{
		// x 4 / 4
		cmdq_pkt_assign_command(pkt, CMDQ_CPR_IMG_QOS_URATE_FACTOR,
			STEP_FACTOR_MULTIPLY);
	}
	GCE_ELSE;
	{
		// x 16 / 4
		cmdq_pkt_assign_command(pkt, CMDQ_CPR_IMG_QOS_URATE_FACTOR,
			STEP_FACTOR_ACTIVE_MULTIPLY);
	}
	GCE_FI;

	cmdq_pkt_assign_command(pkt, CMDQ_CPR_IMG_QOS_ACTIVE_COUNT, 0);
}

static void imgsys_qos_sum_to_MBps(struct cmdq_pkt *pkt)
{
	struct cmdq_operand lop, rop;

	GCE_COND_DECLARE;

	lop.reg = true;
	lop.idx = CMDQ_THR_SPR_IDX2;
	rop.reg = true;
	rop.idx = CMDQ_THR_SPR_IDX3;
	cmdq_pkt_read(pkt, NULL,
		BLS_IMG_E1A_BASE + BLS_IMG_LEN_SUM_R_OFT, CMDQ_THR_SPR_IDX2);
	cmdq_pkt_read(pkt, NULL,
		BLS_IMG_E1A_BASE + BLS_IMG_LEN_SUM_W_OFT, CMDQ_THR_SPR_IDX3);
	cmdq_pkt_logic_command(pkt, CMDQ_LOGIC_ADD,
		 CMDQ_THR_SPR_IDX2, &lop, &rop);

	lop.reg = true;
	lop.idx = CMDQ_THR_SPR_IDX2;
	rop.reg = false;
	rop.value = 6; //toMB
	cmdq_pkt_logic_command(pkt, CMDQ_LOGIC_RIGHT_SHIFT,
		CMDQ_THR_SPR_IDX2, &lop, &rop);

	// case:  bw != 0
	lop.reg = true;
	lop.idx = CMDQ_THR_SPR_IDX2;
	rop.reg = false;
	rop.idx = 0;
	GCE_COND_ASSIGN(pkt, CMDQ_THR_SPR_IDX1);
	GCE_IF(lop, R_CMDQ_NOT_EQUAL, rop);
	{
		lop.reg = true;
		lop.idx = CMDQ_CPR_IMG_QOS_ACTIVE_COUNT;
		rop.reg = false;
		rop.value = 1;
		cmdq_pkt_logic_command(pkt, CMDQ_LOGIC_ADD,
			CMDQ_CPR_IMG_QOS_ACTIVE_COUNT, &lop, &rop);
	}
	GCE_FI;
}

static void imgsys_qos_calculate_avg_bw(struct cmdq_pkt *pkt,
				const uint32_t bwr_addr,
				const uint16_t cpr_sum_idx,
				const uint16_t cpr_avg_idx)
{
	struct cmdq_operand lop, rop;

	lop.reg = true;
	lop.idx = cpr_sum_idx;
	rop.reg = false;
	/* 128 ms */
	rop.value = QOS_AVERAGE_SHIFT;
	cmdq_pkt_logic_command(pkt, CMDQ_LOGIC_RIGHT_SHIFT,
		cpr_avg_idx, &lop, &rop);

	/* New add */
	cmdq_pkt_write_reg_addr(pkt, bwr_addr,
		cpr_avg_idx, CMDQ_REG_MASK);
}

static void imgsys_qos_set_ttl_avg_bw(struct cmdq_pkt *pkt,
	uint32_t bwr_ttl_offset, uint32_t count, ...)
{
	uint32_t i, qos_map_index;
	struct cmdq_operand lop, rop;
	va_list args;

	va_start(args, count);
	cmdq_pkt_assign_command(pkt, CMDQ_THR_SPR_IDX2, 0);
	lop.reg = true;
	lop.idx = CMDQ_THR_SPR_IDX2;
	rop.reg = true;
	for (i = 0; i < count; i++) {
		qos_map_index = va_arg(args, uint32_t);
		rop.idx = CMDQ_CPR_IMG_QOS_AVG_R + qos_map_index * 4;
		cmdq_pkt_logic_command(pkt, CMDQ_LOGIC_ADD,
			CMDQ_THR_SPR_IDX2, &lop, &rop);
		rop.idx = CMDQ_CPR_IMG_QOS_AVG_W + qos_map_index * 4;
		cmdq_pkt_logic_command(pkt, CMDQ_LOGIC_ADD,
			CMDQ_THR_SPR_IDX2, &lop, &rop);
	}
	cmdq_pkt_write_reg_addr(pkt,
		BWR_IMG_E1A_BASE + bwr_ttl_offset,
		CMDQ_THR_SPR_IDX2, CMDQ_REG_MASK);
	va_end(args);
}

static void imgsys_qos_set_smi_chn_ostdl(struct cmdq_pkt *pkt,
	const uint16_t cpr_sum_idx,
	const uint32_t ostdl_addr,
	const uint8_t ostdl_right_shift,
	const uint8_t ostdl_reg_left_shift,
	const uint32_t ostdl_reg_mask)
{
	struct cmdq_operand lop, rop;

	GCE_COND_DECLARE;

	GCE_COND_ASSIGN(pkt, CMDQ_THR_SPR_IDX1);
	lop.reg = true;
	lop.idx = cpr_sum_idx;
	rop.reg = false;
	rop.value = 3 + ostdl_right_shift;
	cmdq_pkt_logic_command(pkt, CMDQ_LOGIC_RIGHT_SHIFT,
		CMDQ_THR_SPR_IDX2, &lop, &rop);

	lop.reg = true;
	lop.idx = CMDQ_THR_SPR_IDX2;
	rop.reg = false;
	rop.value = 1;
	cmdq_pkt_logic_command(pkt, CMDQ_LOGIC_ADD,
		CMDQ_THR_SPR_IDX2, &lop, &rop);

	rop.value = OSTDL_MAX_VALUE;
	// case: ostdl > OSTDL_MAX_VALUE
	GCE_IF(lop, R_CMDQ_GREATER, rop);
	{
		cmdq_pkt_assign_command(pkt, CMDQ_THR_SPR_IDX2, OSTDL_MAX_VALUE);
	}
	GCE_FI;
	rop.value = ostdl_reg_left_shift;

	cmdq_pkt_logic_command(pkt, CMDQ_LOGIC_LEFT_SHIFT,
		CMDQ_THR_SPR_IDX2, &lop, &rop);
	cmdq_pkt_write_reg_addr(pkt, ostdl_addr,
		CMDQ_THR_SPR_IDX2, ostdl_reg_mask);
}

static void imgsys_qos_set_ch_bw(struct cmdq_pkt *pkt,
				const uint16_t cpr_sum_r_idx,
				const uint16_t cpr_sum_w_idx,
				const uint32_t ostdl_addr,
				uint32_t count, ...)
{
	uint32_t i, qos_map_index;
	struct cmdq_operand lop, rop;
	va_list args;

	va_start(args, count);
	cmdq_pkt_assign_command(pkt, cpr_sum_r_idx, 0);
	cmdq_pkt_assign_command(pkt, cpr_sum_w_idx, 0);
	lop.reg = true;
	rop.reg = true;
	for (i = 0; i < count; i++) {
		qos_map_index = va_arg(args, uint32_t);
		lop.idx = cpr_sum_r_idx;
		rop.idx = CMDQ_CPR_IMG_QOS_AVG_R + 4 * qos_map_index;
		cmdq_pkt_logic_command(pkt, CMDQ_LOGIC_ADD,
			cpr_sum_r_idx, &lop, &rop);
		lop.idx = cpr_sum_w_idx;
		rop.idx = CMDQ_CPR_IMG_QOS_AVG_W + 4 * qos_map_index;
		cmdq_pkt_logic_command(pkt, CMDQ_LOGIC_ADD,
			cpr_sum_w_idx, &lop, &rop);
	}
	/* Calculate OSTDL for SMI Common */
	imgsys_qos_set_smi_chn_ostdl(pkt,
		cpr_sum_r_idx,
		ostdl_addr,
		OSTDL_R_RIGHT_SHIFT,
		OSTDL_R_REG_L,
		OSTDL_R_REG_MASK);
	imgsys_qos_set_smi_chn_ostdl(pkt,
		cpr_sum_w_idx,
		ostdl_addr,
		OSTDL_W_RIGHT_SHIFT,
		OSTDL_W_REG_L,
		OSTDL_W_REG_MASK);
	va_end(args);
}

static void imgsys_qos_set_bw(struct cmdq_pkt *pkt)
{
	uint32_t i;
	struct cmdq_operand lop, rop;

	GCE_COND_DECLARE;

	for (i = 0; i < ARRAY_SIZE(qos_map_data); i++) {
		imgsys_qos_to_MBps(pkt,
			qos_map_data[i].bls_base + BLS_IMG_LEN_SUM_R_OFT,
			BWR_IMG_E1A_BASE + qos_map_data[i].bwr_r_offset,
			CMDQ_CPR_IMG_QOS_AVG_R + i * 4,
			CMDQ_CPR_IMG_QOS_SUM_R + i * 4);
		imgsys_qos_to_MBps(pkt,
			qos_map_data[i].bls_base + BLS_IMG_LEN_SUM_W_OFT,
			BWR_IMG_E1A_BASE + qos_map_data[i].bwr_w_offset,
			CMDQ_CPR_IMG_QOS_AVG_W + i * 4,
			CMDQ_CPR_IMG_QOS_SUM_W + i * 4);
	}

	lop.reg = true;
	lop.idx = CMDQ_CPR_IMG_QOS_IDX;
	rop.reg = false;
	rop.value = 1;
	cmdq_pkt_logic_command(pkt, CMDQ_LOGIC_ADD, CMDQ_CPR_IMG_QOS_IDX, &lop, &rop);

	GCE_COND_ASSIGN(pkt, CMDQ_THR_SPR_IDX1);
	/* 128 ms */
	rop.value = (1 << QOS_AVERAGE_SHIFT);
	// case: CMDQ_CPR_IMG_QOS_IDX == 128
	GCE_IF(lop, R_CMDQ_GREATER_EQUAL, rop);
	{
		for (i = 0; i < ARRAY_SIZE(qos_map_data); i++) {
			// calculate avg read/write BW
			imgsys_qos_calculate_avg_bw(pkt,
				BWR_IMG_E1A_BASE + qos_map_data[i].bwr_r_offset,
				CMDQ_CPR_IMG_QOS_SUM_R + i * 4,
				CMDQ_CPR_IMG_QOS_AVG_R + i * 4);
			imgsys_qos_calculate_avg_bw(pkt,
				BWR_IMG_E1A_BASE + qos_map_data[i].bwr_w_offset,
				CMDQ_CPR_IMG_QOS_SUM_W + i * 4,
				CMDQ_CPR_IMG_QOS_AVG_W + i * 4);
		}
		imgsys_qos_init_counter(pkt, false);
		// WPE_EIS
		imgsys_qos_set_ttl_avg_bw(pkt, BWR_IMG_SRT_TTL_ENG_BW0_OFT, 1 /* count */, 0);
		// OMC_TNR
		imgsys_qos_set_ttl_avg_bw(pkt, BWR_IMG_SRT_TTL_ENG_BW1_OFT, 1 /* count */, 4);
		// LITE_WPE_OMC
		imgsys_qos_set_ttl_avg_bw(pkt, BWR_IMG_SRT_TTL_ENG_BW2_OFT, 1 /* count */, 9);
		// ME + LTRAW
		imgsys_qos_set_ttl_avg_bw(pkt, BWR_IMG_SRT_TTL_ENG_BW5_OFT, 1 /* count */, 10);
		// MAE
		imgsys_qos_set_ttl_avg_bw(pkt, BWR_IMG_SRT_TTL_ENG_BW7_OFT, 1 /* count */, 6);
		// TRAW
		imgsys_qos_set_ttl_avg_bw(pkt, BWR_IMG_SRT_TTL_ENG_BW3_OFT, 2 /* count */, 1, 7);
		// DIP
		imgsys_qos_set_ttl_avg_bw(pkt, BWR_IMG_SRT_TTL_ENG_BW4_OFT, 4 /* count */, 2, 3, 5, 8);

		imgsys_qos_set_ch_bw(pkt,
			CMDQ_CPR_IMG_QOS_CHN0_SUM_R,
			CMDQ_CPR_IMG_QOS_CHN0_SUM_W,
			img_ostdl_array[0],
			6, // count
			0, 1, 2, 5, 6, 7);
		imgsys_qos_set_ch_bw(pkt,
			CMDQ_CPR_IMG_QOS_CHN1_SUM_R,
			CMDQ_CPR_IMG_QOS_CHN1_SUM_W,
			img_ostdl_array[1],
			5, // count
			3, 4, 8, 9, 10);
	}
	GCE_FI;
	imgsys_qos_sum_to_MBps(pkt);
}

static void imgsys_qos_config_bwr(struct cmdq_pkt *pkt,
					const enum BWR_CTRL bwr_ctrl)
{
	switch (bwr_ctrl) {
	case BWR_START:
		imgsys_reg_write(pkt,
			bwr_base_array, ARRAY_SIZE(bwr_base_array),
			bwr_init_data, ARRAY_SIZE(bwr_init_data));
		imgsys_qos_set_bw_ratio(pkt);
		imgsys_qos_set_report_mode(pkt);
		cmdq_pkt_write(pkt, NULL, BWR_IMG_E1A_BASE + BWR_IMG_MTCMOS_EN_VLD_OFT,
			BWR_IMG_ENG_EN, CMDQ_REG_MASK);
		cmdq_pkt_write(pkt, NULL, BWR_IMG_E1A_BASE + BWR_IMG_RPT_CTRL_OFT,
			BIT(BWR_IMG_RPT_START), CMDQ_REG_MASK);
		break;
	case BWR_STOP:
		// Report 0
		imgsys_qos_set_fix_bw(pkt, 0, 0);
		/* Wait for 150 us (BWR_IMG_RPT_TIMER is 100 us) */
		cmdq_pkt_sleep(pkt, CMDQ_US_TO_TICK(150), 0 /*don't care*/);
		cmdq_pkt_poll_sleep(pkt, 0x1,
			BWR_IMG_E1A_BASE + BWR_IMG_SEND_BW_ZERO_OFT, CMDQ_REG_MASK);
		cmdq_pkt_poll_sleep(pkt, BIT(BWR_IMG_RPT_WAIT),
			BWR_IMG_E1A_BASE + BWR_IMG_RPT_STATE_OFT, CMDQ_REG_MASK);

		// Terminate flow
		cmdq_pkt_write(pkt, NULL, BWR_IMG_E1A_BASE + BWR_IMG_RPT_CTRL_OFT,
			BIT(BWR_IMG_RPT_END), CMDQ_REG_MASK);
		cmdq_pkt_poll_sleep(pkt, BIT(BWR_IMG_RPT_WAIT),
			BWR_IMG_E1A_BASE + BWR_IMG_RPT_STATE_OFT, CMDQ_REG_MASK);
		cmdq_pkt_write(pkt, NULL, BWR_IMG_E1A_BASE + BWR_IMG_RPT_CTRL_OFT,
			BIT(BWR_IMG_RPT_RST), CMDQ_REG_MASK);
		cmdq_pkt_write(pkt, NULL, BWR_IMG_E1A_BASE + BWR_IMG_MTCMOS_EN_VLD_OFT,
			0, CMDQ_REG_MASK);
		break;
	}
}

static void imgsys_qos_config_bls(struct cmdq_pkt *pkt,
					const enum BLS_CTRL bls_ctrl)
{
	switch (bls_ctrl) {
	case BLS_INIT:
		imgsys_reg_write_base_list(pkt,
			bls_base_array, ARRAY_SIZE(bls_base_array),
			BLS_IMG_CTRL_OFT, 0x1);
		imgsys_reg_write(pkt,
			bls_base_array, ARRAY_SIZE(bls_base_array),
			bls_init_data, ARRAY_SIZE(bls_init_data));
		break;
	case BLS_TRIG:
		imgsys_reg_write_base_list(pkt,
			bls_base_array, ARRAY_SIZE(bls_base_array),
			BLS_IMG_SW_TRIG_START_OFT, 0x1);
		break;
	case BLS_STOP:
		imgsys_reg_write_base_list(pkt,
			bls_base_array, ARRAY_SIZE(bls_base_array),
			BLS_IMG_CTRL_OFT, 0x0);
		break;
	}
}

static void imgsys_qos_report_switch(struct cmdq_pkt *pkt,
					 const enum QOS_STATE qos_state)
{
	switch (qos_state) {
	case QOS_STATE_MAX:
		imgsys_qos_set_fix_bw(pkt,
			BWR_IMG_SRT_ENG_MAX_RW_ENG_BW,
			BWR_IMG_SRT_ENG_MAX_TTL_BW);
		cmdq_pkt_sleep(pkt, CMDQ_US_TO_TICK(1000), 0 /*don't care*/);
		imgsys_qos_config_bls(pkt, BLS_TRIG);
		imgsys_qos_init_counter(pkt, false);
		break;
	case QOS_STATE_NORMAL:
		cmdq_pkt_sleep(pkt, CMDQ_US_TO_TICK(1000), 0 /*don't care*/);
		imgsys_qos_config_bls(pkt, BLS_TRIG);
		imgsys_qos_set_bw(pkt);
		break;
	default:
		pr_err("[%s] enum QOS_STATE(%d) is not supported\n",
			__func__, qos_state);
		break;
	}
}

static void imgsys_qos_loop(const struct mtk_imgsys_hwqos *hwqos_info)
{
	struct cmdq_operand lop, rop;

	GCE_COND_DECLARE;

	g_hwqos_pkt = cmdq_pkt_create(g_hwqos_clt);
	if (!g_hwqos_pkt) {
		pr_err("[%s] [ERROR] cmdq_pkt_create fail\n", __func__);
		return;
	}
	g_hwqos_pkt->no_irq = true;
	// check if hwqos_buf is IMGSYS_QOS_REPORT_MAX
	GCE_COND_ASSIGN(g_hwqos_pkt, CMDQ_THR_SPR_IDX1);
	cmdq_pkt_acquire_event(g_hwqos_pkt, hwqos_info->hwqos_sync_token);
	cmdq_pkt_read(g_hwqos_pkt, NULL, g_hwqos_buf_pa, CMDQ_THR_SPR_IDX2);
	lop.reg = true;
	lop.idx = CMDQ_THR_SPR_IDX2;
	rop.reg = false;
	rop.value = IMGSYS_QOS_REPORT_MAX;
	GCE_IF(lop, R_CMDQ_EQUAL, rop);
	/* case: hwqos_buf == IMGSYS_QOS_REPORT_MAX */
	cmdq_pkt_write(g_hwqos_pkt, NULL, g_hwqos_buf_pa,
			IMGSYS_QOS_REPORT_NORMAL, CMDQ_REG_MASK);
	cmdq_pkt_clear_event(g_hwqos_pkt, hwqos_info->hwqos_sync_token);
	imgsys_qos_report_switch(g_hwqos_pkt, QOS_STATE_MAX);
	GCE_ELSE;
	/* case: hwqos_buf == IMGSYS_QOS_REPORT_NORMAL */
	cmdq_pkt_clear_event(g_hwqos_pkt, hwqos_info->hwqos_sync_token);
	imgsys_qos_report_switch(g_hwqos_pkt, QOS_STATE_NORMAL);
	GCE_FI;
	cmdq_pkt_finalize_loop(g_hwqos_pkt);
	cmdq_pkt_flush_async(g_hwqos_pkt, NULL, NULL);
}

/** Global api
 * @brief List Global api
 */

void mtk_imgsys_cmdq_hwqos_init(struct mtk_imgsys_dev *imgsys_dev)
{
	struct device *dev = imgsys_dev->dev;
	struct mtk_imgsys_hwqos *hwqos_info = &imgsys_dev->hwqos_info;

	hwqos_info->hwqos_support = of_property_read_bool(
		imgsys_dev->dev->of_node, "mediatek,imgsys-hwqos-support");
	g_hwqos_support = hwqos_info->hwqos_support;
	if (!hwqos_info->hwqos_support)
		return;

	of_property_read_u16(imgsys_dev->dev->of_node,
				"hwqos-sync-token",
				&hwqos_info->hwqos_sync_token);

	g_hwqos_clt = cmdq_mbox_create(dev, IMGSYS_QOS_THD_IDX);

	pr_info("[%s] sync_token(%d), thd_idx(%d), clt(0x%lx)\n", __func__,
		hwqos_info->hwqos_sync_token,
		IMGSYS_QOS_THD_IDX,
		(unsigned long) g_hwqos_clt);
}

void mtk_imgsys_cmdq_hwqos_release(void)
{
	if (!g_hwqos_clt) {
		pr_err("[%s] g_hwqos_clt is NULL\n", __func__);
		return;
	}
	cmdq_mbox_destroy(g_hwqos_clt);
	g_hwqos_clt = NULL;
}

void mtk_imgsys_cmdq_hwqos_streamon(const struct mtk_imgsys_hwqos *hwqos_info)
{
	struct cmdq_pkt *pkt;

	g_hwqos_state = QOS_STATE_INIT;
	if (g_hwqos_pkt) {
		pr_info("[%s] g_hwqos_pkt is already exists, skip\n", __func__);
		return;
	}

	cmdq_mbox_enable(g_hwqos_clt->chan);
	cmdq_clear_event(g_hwqos_clt->chan, hwqos_info->hwqos_sync_token);

	if (!g_hwqos_buf_va || !g_hwqos_buf_pa)
		g_hwqos_buf_va = cmdq_mbox_buf_alloc(g_hwqos_clt, &g_hwqos_buf_pa);

	if (!g_hwqos_buf_va || !g_hwqos_buf_pa) {
		pr_err("[%s] [ERROR] cmdq_mbox_buf_alloc fail\n", __func__);
		g_hwqos_buf_va = NULL;
		g_hwqos_buf_pa = 0;
		return;
	}

	*g_hwqos_buf_va = IMGSYS_QOS_REPORT_NORMAL;
	pkt = cmdq_pkt_create(g_hwqos_clt);
	if (!pkt) {
		pr_err("[%s] [ERROR] cmdq_pkt_create fail\n", __func__);
		return;
	}
	imgsys_qos_config_bls(pkt, BLS_INIT);
	imgsys_qos_config_bwr(pkt, BWR_START);
	imgsys_qos_set_ostdl_en(pkt, 1);
	imgsys_qos_init_counter(pkt, true);
	cmdq_pkt_flush(pkt);
	cmdq_pkt_destroy(pkt);
	MTK_IMGSYS_QOS_ENABLE(g_hwqos_dbg_en,
		imgsys_hwqos_dbg_reg_dump(3);
	);
	imgsys_qos_loop(hwqos_info);
	imgsys_hwqos_set_dbg_thread(true);
}

void mtk_imgsys_cmdq_hwqos_streamoff(void)
{
	struct cmdq_pkt *pkt;

	g_hwqos_state = QOS_STATE_UNINIT;
	if (g_hwqos_pkt) {
		cmdq_mbox_stop(g_hwqos_clt);
		cmdq_pkt_destroy(g_hwqos_pkt);
		g_hwqos_pkt = NULL;
	}

	if (g_hwqos_buf_va && g_hwqos_buf_pa) {
		cmdq_mbox_buf_free(g_hwqos_clt, g_hwqos_buf_va, g_hwqos_buf_pa);
		g_hwqos_buf_va = NULL;
		g_hwqos_buf_pa = 0;
	}

	pkt = cmdq_pkt_create(g_hwqos_clt);
	if (pkt) {
		imgsys_qos_config_bwr(pkt, BWR_STOP);
		imgsys_qos_config_bls(pkt, BLS_STOP);
		imgsys_qos_set_ostdl_en(pkt, 0);
		cmdq_pkt_flush(pkt);
		cmdq_pkt_destroy(pkt);
	} else {
		pr_err("[%s] [ERROR] cmdq_pkt_create fail\n", __func__);
	}
	imgsys_hwqos_set_dbg_thread(false);
	cmdq_mbox_disable(g_hwqos_clt->chan);
	MTK_IMGSYS_QOS_ENABLE(g_hwqos_dbg_en,
		imgsys_hwqos_dbg_reg_dump(3);
	);
}

void mtk_imgsys_cmdq_hwqos_report(
	struct cmdq_pkt *pkt,
	const struct mtk_imgsys_hwqos *hwqos_info,
	const int *fps)
{
	if ((*fps == 0) || (g_hwqos_state == QOS_STATE_INIT)) {
		if (g_hwqos_buf_pa && g_hwqos_buf_va) {
			cmdq_pkt_acquire_event(pkt, hwqos_info->hwqos_sync_token);
			cmdq_pkt_write(pkt, NULL, g_hwqos_buf_pa,
					IMGSYS_QOS_REPORT_MAX, CMDQ_REG_MASK);
			cmdq_pkt_clear_event(pkt, hwqos_info->hwqos_sync_token);
		}
		if (g_hwqos_state == QOS_STATE_INIT)
			g_hwqos_state = QOS_STATE_MAX;
	}
}

static int imgsys_hwqos_set_status(const char *val, const struct kernel_param *kp)
{
	u32 hwqos_log_level;

	if (kstrtou32(val, 0, &hwqos_log_level)) {
		pr_err("[%s] param fail\n", __func__);
		return 0;
	}
	if (!g_hwqos_support) {
		pr_err("[%s] hwqos is not supported\n", __func__);
		return 0;
	}

	if ((g_hwqos_state == QOS_STATE_INIT) || (g_hwqos_state == QOS_STATE_UNINIT)) {
		pr_err("[%s] hwqos is not ready\n", __func__);
		return 0;
	}
	imgsys_hwqos_dbg_reg_dump(hwqos_log_level);
	return 0;
}

static const struct kernel_param_ops imgsys_hwqos_status_ops = {
	.set = imgsys_hwqos_set_status,
};

module_param_cb(imgsys_hwqos_status, &imgsys_hwqos_status_ops, NULL, 0644);
MODULE_PARM_DESC(imgsys_hwqos_status, "imgsys hwqos status");
