// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include <stdbool.h>

#include "ut_fs_tsrec.h"
#include "ut_fs_test.h"

#include "mtk_cam-seninf-tsrec-regs-def.h"
#include "mtk_cam-seninf-tsrec-regs.h"
#include "mtk_cam-seninf-tsrec-def.h"
#include "mtk_cam-seninf-tsrec.h"


#define PFX "UT_FS_TSREC"


#define SENINF_MAX_CNT               (12)
#define TSREC_MAX_CNT                (6)

#if !(TSREC_WITH_64_BITS_TIMER_RG)
#define UT_TSREC_RG_LAST \
	(TSREC_ADDR_BY_OFFSET(TSREC_EXP_CNT_OFFSET( \
		(TSREC_MAX_CNT-1), (TSREC_EXP_MAX_CNT-1), (TSREC_TS_REC_MAX_CNT-1))))
#else
#define UT_TSREC_RG_LAST \
	(TSREC_ADDR_BY_OFFSET(TSREC_EXP_CNT_M_OFFSET( \
		(TSREC_MAX_CNT-1), (TSREC_EXP_MAX_CNT-1), (TSREC_TS_REC_MAX_CNT-1))))
#endif // !TSREC_WITH_64_BITS_TIMER_RG

#define UT_TSREC_REGS_SIZE (UT_TSREC_RG_LAST - TSREC_BASE + 1)
static unsigned int regs[UT_TSREC_REGS_SIZE];


/******************************************************************************
 * unit test static structures
 *****************************************************************************/
struct ut_fs_tsrec_irq_info_st {
	unsigned int intr_status;
	unsigned int intr_status_2;

	unsigned long long tsrec_ts_us;
};

struct ut_fs_tsrec_msgfifo {
	// unsigned int push_idx;
	// unsigned int pop_idx;

	// struct ut_fs_tsrec_irq_info_st irq_info[TSREC_MAX_CNT];

	unsigned int items;
	struct ut_fs_tsrec_irq_info_st irq_info;
};
static struct ut_fs_tsrec_msgfifo ut_msg_fifo;


/******************************************************************************
 * for unit test (fake function of linux APIs)
 *****************************************************************************/
unsigned long long ktime_get_boottime_ns(void)
{
	return 0ULL;
}


/******************************************************************************
 * for unit test - TSREC
 *****************************************************************************/
void ut_fs_tsrec_write_reg(const unsigned int addr, const unsigned int val)
{
	unsigned int shift = addr;

	if (shift < UT_TSREC_REGS_SIZE)
		regs[shift] = val;
	else
		UT_ERR(
			"ERROR: invalid addr:%#X(+%u) (UT_TSREC_REGS_SIZE:%u), skip write val:%u\n",
			addr, shift, UT_TSREC_REGS_SIZE, val);
}


unsigned int ut_fs_tsrec_read_reg(const unsigned int addr)
{
	unsigned int shift = addr;

	if (shift < UT_TSREC_REGS_SIZE)
		return regs[shift];

	UT_ERR(
		"ERROR: invalid addr:%#X(+%u) (UT_TSREC_REGS_SIZE:%u), skip read val\n",
		addr, shift, UT_TSREC_REGS_SIZE);
	return 0;
}


unsigned int ut_fs_tsrec_chk_fifo_not_empty(void)
{
	return ut_msg_fifo.items;
}


void ut_fs_tsrec_fifo_out(
	unsigned int *p_intr_status, unsigned int *p_intr_status_2,
	unsigned long long *p_tsrec_ts_us)
{
	if (!p_intr_status || !p_intr_status_2 || !p_tsrec_ts_us)
		return;
	if (ut_msg_fifo.items == 0)
		return;

	*p_intr_status = ut_msg_fifo.irq_info.intr_status;
	*p_intr_status_2 = ut_msg_fifo.irq_info.intr_status_2;
	*p_tsrec_ts_us = ut_msg_fifo.irq_info.tsrec_ts_us;

	--ut_msg_fifo.items;
}


unsigned int ut_fs_tsrec_fifo_in(
	const unsigned int intr_status, const unsigned int intr_status_2,
	const unsigned long long tsrec_ts_us)
{
	struct ut_fs_tsrec_irq_info_st irq_info = {0};

	/* sync info */
	irq_info.intr_status = intr_status;
	irq_info.intr_status_2 = intr_status_2;
	irq_info.tsrec_ts_us = tsrec_ts_us;

	ut_msg_fifo.irq_info = irq_info;

	/* hardcode => only support push one item */
	return ++ut_msg_fifo.items;
}


/******************************************************************************
 * UT TSREC static functions
 *****************************************************************************/

static void ut_fs_tsrec_chk_regs_addr(void)
{
	unsigned int i = 0, j = 0, k = 0;

	UT_FUNC_START();

	UT_INF("==> Print TSREC SW/HW spec. define:\n");
	UT_INF("%30s%3u\n", "TSREC_MAX_CNT:",
		TSREC_MAX_CNT);
	UT_INF("%30s%3u\n", "TSREC_EXP_MAX_CNT:",
		TSREC_EXP_MAX_CNT);
	UT_INF("%30s%3u\n", "TSREC_TS_REC_MAX_CNT:",
		TSREC_TS_REC_MAX_CNT);
	UT_INF("%30s%3u\n", "TSREC_WITH_GLOBAL_TIMER:",
		TSREC_WITH_GLOBAL_TIMER);
	UT_INF("%30s%3u\n", "TSREC_USE_GLOBAL_TIMER:",
		TSREC_USE_GLOBAL_TIMER);
	UT_INF("%30s%3u\n", "TSREC_WITH_64_BITS_TIMER_RG:",
		TSREC_WITH_64_BITS_TIMER_RG);

	UT_INF("\n");
	UT_INF("==> Print TSREC regs addr:\n");
	UT_INF("%30s %#10X\n", "TSREC_BASE:",
		TSREC_BASE);
	UT_INF("%30s %#10X\n", "SENINF_BASE:",
		SENINF_BASE);
	UT_INF("----------------------------------------------------------\n");
	UT_INF("%30s %#10X(+%#6X)\n", "TSREC_CFG:",
		TSREC_ADDR_BY_OFFSET(TSREC_TOP_CFG_OFFSET),
		TSREC_TOP_CFG_OFFSET);
	UT_INF("%30s %#10X(+%#6X)\n", "TSREC_TIMER_CFG:",
		TSREC_ADDR_BY_OFFSET(TSREC_TIMER_CFG_OFFSET),
		TSREC_TIMER_CFG_OFFSET);
	UT_INF("%30s %#10X(+%#6X)\n", "TSREC_TIMER_LAT:",
		TSREC_ADDR_BY_OFFSET(TSREC_TIMER_LAT_OFFSET),
		TSREC_TIMER_LAT_OFFSET);

	UT_INF("%30s %#10X(+%#6X)\n", "TSREC_INT_EN:",
		TSREC_ADDR_BY_OFFSET(TSREC_INT_EN_OFFSET),
		TSREC_INT_EN_OFFSET);
	UT_INF("%30s %#10X(+%#6X)\n", "TSREC_INT_STATUS:",
		TSREC_ADDR_BY_OFFSET(TSREC_INT_STATUS_OFFSET),
		TSREC_INT_STATUS_OFFSET);
	UT_INF("%30s %#10X(+%#6X)\n", "TSREC_INT_EN_2:",
		TSREC_ADDR_BY_OFFSET(TSREC_INT_EN_2_OFFSET),
		TSREC_INT_EN_2_OFFSET);
	UT_INF("%30s %#10X(+%#6X)\n", "TSREC_INT_STATUS_2:",
		TSREC_ADDR_BY_OFFSET(TSREC_INT_STATUS_2_OFFSET),
		TSREC_INT_STATUS_2_OFFSET);

#if (TSREC_WITH_64_BITS_TIMER_RG)
	UT_INF("%30s %#10X(+%#6X)\n", "TSREC_TIMER_LAT_M:",
		TSREC_ADDR_BY_OFFSET(TSREC_TIMER_LAT_M_OFFSET),
		TSREC_TIMER_LAT_M_OFFSET);
#endif // TSREC_WITH_64_BITS_TIMER_RG

	for (i = 0; i < TSREC_MAX_CNT; ++i) {
		UT_INF("\n");
		UT_INF("[%2u]%26s %#10X(+%#6X)\n", i, "TSREC_n_CFG:",
			TSREC_ADDR_BY_OFFSET(
				TSREC_CFG_OFFSET(i)),
			TSREC_CFG_OFFSET(i));
		UT_INF("[%2u]%26s %#10X(+%#6X)\n", i, "TSREC_n_SW_RST:",
			TSREC_ADDR_BY_OFFSET(
				TSREC_SW_RST_OFFSET(i)),
			TSREC_SW_RST_OFFSET(i));
		UT_INF("[%2u]%26s %#10X(+%#6X)\n", i, "TSREC_n_TS_CNT:",
			TSREC_ADDR_BY_OFFSET(
				TSREC_TS_CNT_OFFSET(i)),
			TSREC_TS_CNT_OFFSET(i));
		UT_INF("[%2u]%26s %#10X(+%#6X)\n", i, "TSREC_n_TRIG_SRC:",
			TSREC_ADDR_BY_OFFSET(
				TSREC_TRIG_SRC_OFFSET(i)),
			TSREC_TRIG_SRC_OFFSET(i));

		for (j = 0; j < TSREC_EXP_MAX_CNT; ++j) {
			UT_INF("[%2u]%22s(%u): %#10X(+%#6X)\n", i,
				"TSREC_n_EXP_VC_DT",
				j,
				TSREC_ADDR_BY_OFFSET(
					TSREC_EXP_VC_DT_OFFSET(i, j)),
				TSREC_EXP_VC_DT_OFFSET(i, j));
		}

		for (j = 0; j < TSREC_EXP_MAX_CNT; ++j) {
			for (k = 0; k < TSREC_TS_REC_MAX_CNT; ++k) {
				UT_INF("[%2u]%21s%u(%u): %#10X(+%#6X)\n", i,
					"TSREC_n_EXP_CNT",
					k, j,
					TSREC_ADDR_BY_OFFSET(
						TSREC_EXP_CNT_OFFSET(i, j, k)),
					TSREC_EXP_CNT_OFFSET(i, j, k));
			}
		}

#if (TSREC_WITH_64_BITS_TIMER_RG)
		for (j = 0; j < TSREC_EXP_MAX_CNT; ++j) {
			for (k = 0; k < TSREC_TS_REC_MAX_CNT; ++k) {
				UT_INF("[%2u]%21s%u(%u): %#10X(+%#6X)\n", i,
					"TSREC_n_EXP_CNT_M",
					k, j,
					TSREC_ADDR_BY_OFFSET(
						TSREC_EXP_CNT_M_OFFSET(i, j, k)),
					TSREC_EXP_CNT_M_OFFSET(i, j, k));
			}
		}
#endif // TSREC_WITH_64_BITS_TIMER_RG

		UT_INF("\n");
	}

	UT_FUNC_END();
}


/*--------------------------------------------------------------------------*/
// for tsrec regs APIs UT
/*--------------------------------------------------------------------------*/
static void ut_fs_tsrec_top_cfg_clk_en(void)
{
	unsigned int i = 0;

	UT_FUNC_START();

	UT_INF("UT Test => clk ON for each bit (tsrec n) ...\n");
	for (i = 0; i < TSREC_MAX_CNT; ++i) {
		// clk on
		mtk_cam_seninf_s_tsrec_top_cfg_clk_en_bit(i, 1);
	}

	UT_INF("\n");
	UT_INF("UT Reset: clk OFF for each bit (tsrec n) ...\n");
	for (i = 0; i < TSREC_MAX_CNT; ++i) {
		// clk off
		mtk_cam_seninf_s_tsrec_top_cfg_clk_en_bit(i, 0);
	}

	UT_FUNC_END();
}


static void ut_fs_tsrec_timer_cfg(void)
{
	UT_FUNC_START();

	UT_INF("UT Test => timer cfg enable ...\n");
	mtk_cam_seninf_s_tsrec_timer_cfg(1);

	UT_INF("\n");
	UT_INF("UT Reset: timer cfg disable ...\n");
	mtk_cam_seninf_s_tsrec_timer_cfg(0);

	UT_FUNC_END();
}


static void ut_fs_tsrec_latch_time(void)
{
	unsigned long long tick = 0xffffffff, time_us = 0xffffffff;
	unsigned long long val = 0, ret = 0;
#if !(TSREC_WITH_64_BITS_TIMER_RG)
	unsigned int test_val = 1122334455;
#else
	unsigned int test_val = 1133552244;
	unsigned int test_val_M = 1122334455;
#endif

	UT_FUNC_START();

#if !(TSREC_WITH_64_BITS_TIMER_RG)

	regs[TSREC_TIMER_LAT_OFFSET] = test_val;
	val = regs[TSREC_TIMER_LAT_OFFSET];

	UT_INF("UT Test => set (LAT:%u(%#x)) : %llu(%#llx)\n",
		regs[TSREC_TIMER_LAT_OFFSET],
		regs[TSREC_TIMER_LAT_OFFSET],
		val,
		val);

	ret = mtk_cam_seninf_tsrec_latch_time();
	/* auto check result */
	if (ret != val) {
		UT_ERR("ERROR: func return val:%llu does NOT match to UT set val:%llu\n",
			ret, val);
	} else {
		UT_INF("PASS: func return val:%llu matches to UT set val:%llu\n",
			ret, val);
	}

#else // !TSREC_WITH_64_BITS_TIMER_RG

	regs[TSREC_TIMER_LAT_OFFSET] = test_val;
	regs[TSREC_TIMER_LAT_M_OFFSET] = test_val_M;
	val = regs[TSREC_TIMER_LAT_M_OFFSET];
	val = ((val << 32) | (regs[TSREC_TIMER_LAT_OFFSET]));

	UT_INF("UT Test => set (LAT_M:%u(%#x) | LAT_L:%u(%#x)) : %llu(%#llx)\n",
		regs[TSREC_TIMER_LAT_M_OFFSET],
		regs[TSREC_TIMER_LAT_M_OFFSET],
		regs[TSREC_TIMER_LAT_OFFSET],
		regs[TSREC_TIMER_LAT_OFFSET],
		val,
		val);

	ret = mtk_cam_seninf_tsrec_latch_time();
	/* auto check result */
	if (ret != val) {
		UT_ERR("ERROR: func return val:%llu not match to UT set val:%llu\n",
			ret, val);
	} else {
		UT_INF("PASS: func return val:%llu matches to UT set val:%llu\n",
			ret, val);
	}

#endif // !TSREC_WITH_64_BITS_TIMER_RG

	UT_INF("\n");

	UT_INF("UT Test => get tsrec current time utility function when timer DISABLE...\n");
	mtk_cam_seninf_s_tsrec_timer_cfg(0);
	UT_INF("UT Test => + tick:%llu, time_us:%llu\n", tick, time_us);
	ret = mtk_cam_seninf_g_tsrec_current_time(&tick, &time_us);
	UT_INF("UT Test => - tick:%llu, time_us:%llu, ret:%llu\n", tick, time_us, ret);
	if (ret == 0 && tick == 0 && time_us == 0) {
		UT_INF("PASS: after func, tick:%llu, time_us:%llu, ret:%llu are all to zero\n",
			tick, time_us, ret);
	} else {
		UT_ERR("ERROR: after func, tick:%llu, time_us:%llu, ret:%llu are NOT all to zero\n",
			tick, time_us, ret);
	}

	UT_INF("\n");
	UT_INF("UT Test => get tsrec current time utility function when timer ENABLE...\n");
	mtk_cam_seninf_s_tsrec_timer_cfg(1);
	UT_INF("UT Test => + tick:%llu, time_us:%llu\n", tick, time_us);
	ret = mtk_cam_seninf_g_tsrec_current_time(&tick, &time_us);
	UT_INF("UT Test => - tick:%llu, time_us:%llu, ret:%llu\n", tick, time_us, ret);
	if (ret == 1 && tick == val && time_us == TSREC_TICK_TO_US(val)) {
		UT_INF(
			"PASS: after func, ret:%llu, tick:%llu & time_us:%llu match to regs settings before\n",
			ret, tick, time_us);
	} else {
		UT_ERR(
			"ERROR: after func, ret:%llu, tick:%llu & time_us:%llu does NOT match to regs settings before\n",
			ret, tick, time_us);
	}

	/* reset operation */
	UT_INF("\n");
	UT_INF("UT Reset: tsrec timer disable ...\n");
	mtk_cam_seninf_s_tsrec_timer_cfg(0);

	UT_FUNC_END();
}


static void ut_fs_tsrec_int_en_proc(
	const unsigned int exp0, const unsigned int exp1, const unsigned int exp2,
	const unsigned int trig_src, const unsigned int flag)
{
	unsigned int i = 0;

	for (i = 0; i < TSREC_MAX_CNT; ++i) {
		unsigned int intr_en = 0, intr_en_2 = 0;

		/* set */
		/* vsync or 1st-Hsync / Enable */
		mtk_cam_seninf_s_tsrec_intr_en(i,
			exp0, exp1, exp2,
			trig_src, flag);
		mtk_cam_seninf_s_tsrec_intr_en_2(i,
			exp0, exp1, exp2,
			trig_src, flag);
		mtk_cam_seninf_s_tsrec_intr_wclr_en(1);

		/* read back and check */
		intr_en = mtk_cam_seninf_g_tsrec_intr_en();
		intr_en_2 = mtk_cam_seninf_g_tsrec_intr_en_2();
		if (intr_en == regs[TSREC_INT_EN_OFFSET]
				&& intr_en_2 == regs[TSREC_INT_EN_2_OFFSET]) {
			UT_INF(
				"PASS: get (intr_en/intr_en_2) : (%#x/%#x) matches to UT read %#x/%#x\n",
				intr_en, intr_en_2,
				regs[TSREC_INT_EN_OFFSET],
				regs[TSREC_INT_EN_2_OFFSET]);
		} else {
			UT_ERR(
				"ERROR: get (intr_en/intr_en_2) : (%#x/%#x) does NOT matches to UT read %#x/%#x\n",
				intr_en, intr_en_2,
				regs[TSREC_INT_EN_OFFSET],
				regs[TSREC_INT_EN_2_OFFSET]);
		}
	}
}


static void ut_fs_tsrec_int_en(void)
{
	/* custom config */
	const unsigned int exp0_en = 1;
	const unsigned int exp1_en = 0;
	const unsigned int exp2_en = 0;
	unsigned int i = 0;

	UT_FUNC_START();

	/* Vsync:0 / 1st-Hsync:1 */
	for (i = 0; i < 2; ++i) {
		UT_INF(
			"UT Test => (Vsync:0/1st-Hsync:1):(%u) / enable INTR (for each tsrec) ...\n",
			i);
		UT_INF(
			"UT Cofig: exp_en(%u/%u/%u), trig_src:%u(Vsync:0/1st-Hsync:1), enable:%u ...\n",
			exp0_en, exp1_en, exp2_en, i, 1);

		/* main test procedure */
		for (i = 0; i < TSREC_MAX_CNT; ++i)
			ut_fs_tsrec_int_en_proc(exp0_en, exp1_en, exp2_en, i, 1);


		UT_INF("\n");
		UT_INF(
			"UT Reset: (Vsync:0/1st-Hsync:1):(%u) / disable INTR (for each tsrec) ...\n",
			i);
		UT_INF(
			"UT Config => exp_en(%u/%u/%u), trig_src:%u(Vsync:0/1st-Hsync:1), enable:%u ...\n",
			exp0_en, exp1_en, exp2_en, i, 0);

		/* main test procedure */
		for (i = 0; i < TSREC_MAX_CNT; ++i)
			ut_fs_tsrec_int_en_proc(exp0_en, exp1_en, exp2_en, i, 0);


		/* for reset */
		mtk_cam_seninf_s_tsrec_intr_wclr_en(0);


		UT_INF("\n");
		UT_INF("\n");
		UT_INF("\n");
	}

	UT_FUNC_END();
}


static void ut_fs_tsrec_int_status_proc(const unsigned int n,
	const unsigned int val)
{
	const unsigned int addr_offset[2] = {
		TSREC_INT_STATUS_OFFSET,
		TSREC_INT_STATUS_2_OFFSET,
	};
	unsigned int ret = 0;

	/* error handling, check n range (how many int_en/int_status RGs) */
	if (n > 2) {
		UT_ERR(
			"ERROR: for error handling, plz check n:%u range (how many int_en/int_status RGs)\n",
			n);
		return;
	}


	/* set UT RG val */
	regs[addr_offset[n]] = val;
	UT_INF("UT Test => get (n:%u, val:%#x), set int_status(+%#x):%#x ...\n",
		n, val,
		addr_offset[n], regs[addr_offset[n]]);

	/* get int status */
	switch (n) {
	case 0:
		ret = mtk_cam_seninf_g_tsrec_intr_status();
		break;

	case 1:
		ret = mtk_cam_seninf_g_tsrec_intr_status_2();
		break;

	default:
		break;
	}

	/* check result */
	if (ret != val) {
		UT_ERR(
			"ERROR: func return ret:%#x (int_status) does not match to UT val:%#x\n",
			ret, val);
	} else {
		UT_INF(
			"PASS: func return ret:%#x (int_status) matches to UT val:%#x\n",
			ret, val);
	}
	UT_INF("\n");

	/* test clear status */
	ret = val;
	// val = 0;
	UT_INF("UT Test => for testing write clear, enable write clear ...\n");
	mtk_cam_seninf_s_tsrec_intr_wclr_en(1);

	UT_INF("\n");
	UT_INF("UT Test => write clear, write val:%#x ...\n", val);
	/* call for write clear */
	switch (n) {
	case 0:
		mtk_cam_seninf_clr_tsrec_intr_status(val);
		break;

	case 1:
		mtk_cam_seninf_clr_tsrec_intr_status_2(val);
		break;

	default:
		break;
	}

	/* check result */
	ret = regs[addr_offset[n]];
	regs[addr_offset[n]] ^= val;
	if (regs[addr_offset[n]]) {
		UT_ERR("ERROR: int_status(+%#x):%#x => %#x, is not cleared ...\n",
			addr_offset[n], ret, regs[addr_offset[n]]);
	} else {
		UT_INF("PASS: int_status(+%#x):%#x => %#x, is cleared ...\n",
			addr_offset[n], ret, regs[addr_offset[n]]);
	}

	UT_INF("\n");
	UT_INF("\n");
	UT_INF("\n");
}


static void ut_fs_tsrec_int_status(void)
{
	const unsigned int test_val[2] = {0xAAB, 0x2A};
	unsigned int i = 0;

	UT_FUNC_START();

	/* main test procedure */
	for (i = 0; i < 2; ++i)
		ut_fs_tsrec_int_status_proc(i, test_val[i]);

	/* reset status */
	regs[TSREC_INT_STATUS_OFFSET] = 0;
	regs[TSREC_INT_STATUS_2_OFFSET] = 0;
	UT_INF("\n");
	UT_INF(
		"UT Reset => ALL int_status RGs (TSREC & UT) reset to 0, UT RGs:(%#x/%#x)\n",
		regs[TSREC_INT_STATUS_OFFSET],
		regs[TSREC_INT_STATUS_2_OFFSET]);
	mtk_cam_seninf_clr_tsrec_intr_status(0);
	mtk_cam_seninf_clr_tsrec_intr_status_2(0);

	UT_INF("\n");
	UT_INF("UT Reset => w_clr set to disable\n");
	mtk_cam_seninf_s_tsrec_intr_wclr_en(0);

	UT_FUNC_END();
}


static void ut_fs_tsrec_n_cfg(void)
{
	unsigned int i = 0, j = 0;

	UT_FUNC_START();

	for (i = 0; i < TSREC_MAX_CNT; ++i) {
		mtk_cam_seninf_s_tsrec_n_cfg(i, -1);

		// due to RG is write clr.
		regs[TSREC_CFG_OFFSET(i)] = 0;

		for (j = 0; j < TSREC_EXP_MAX_CNT; ++j) {
			mtk_cam_seninf_s_tsrec_n_cfg(i, (int)j);

			// due to RG is write clr.
			regs[TSREC_CFG_OFFSET(i)] = 0;
		}
	}

	UT_FUNC_END();
}


static void ut_fs_tsrec_sw_rst(void)
{
	unsigned int i = 0;

	UT_FUNC_START();

	for (i = 0; i < TSREC_MAX_CNT; ++i) {
		mtk_cam_seninf_s_tsrec_sw_rst(i, 1);
		mtk_cam_seninf_s_tsrec_sw_rst(i, 0);
	}

	UT_FUNC_END();
}


static void ut_fs_tsrec_ts_cnt(void)
{
	union REG_TSREC_N_TS_CNT reg = {0};
	unsigned int i = 0;
	unsigned int ret = 0;

	UT_FUNC_START();

	for (i = 0; i < TSREC_MAX_CNT; ++i) {
		reg.bits.TSREC_N_EXP0_TS_CNT = ((i+1) % TSREC_TS_REC_MAX_CNT);
		reg.bits.TSREC_N_EXP1_TS_CNT = ((i+2) % TSREC_TS_REC_MAX_CNT);
		reg.bits.TSREC_N_EXP2_TS_CNT = ((i+3) % TSREC_TS_REC_MAX_CNT);
		regs[TSREC_TS_CNT_OFFSET(i)] = reg.val;

		ret = mtk_cam_seninf_g_tsrec_ts_cnt(i);

		if (ret != reg.val) {
			UT_ERR("ERROR: func return val:%u does NOT match to UT set val:%u\n",
				ret, reg.val);
		} else {
			UT_INF("PASS: func return val:%u matches to UT set val:%u\n",
				ret, reg.val);
		}
	}

	UT_FUNC_END();
}


static void ut_fs_tsrec_trig_src(void)
{
	unsigned int i = 0, j = 0;

	UT_FUNC_START();

	for (i = 0; i < TSREC_MAX_CNT; ++i) {
		mtk_cam_seninf_s_tsrec_trig_src(i, -1, 1);
		mtk_cam_seninf_s_tsrec_trig_src(i, -1, 0);

		for (j = 0; j < TSREC_EXP_MAX_CNT; ++j)
			mtk_cam_seninf_s_tsrec_trig_src(i, (int)j, 1);

		for (j = 0; j < TSREC_EXP_MAX_CNT; ++j)
			mtk_cam_seninf_s_tsrec_trig_src(i, (int)j, 0);
	}

	UT_FUNC_END();
}


static void ut_fs_tsrec_exp_vc_dt(void)
{
	unsigned int i = 0, j = 0;

	UT_FUNC_START();

	for (i = 0; i < TSREC_MAX_CNT; ++i) {
		for (j = 0; j < TSREC_EXP_MAX_CNT; ++j) {
			/* custom config */
			const unsigned int v_vc = i + 10;
			const unsigned int h_vc = i + 10;
			const unsigned int h_dt = j + 20;
			union REG_TSREC_N_EXP_VC_DT reg = {0};

			/* set */
			mtk_cam_seninf_s_tsrec_exp_vc_dt(i, j,
					v_vc, h_vc, h_dt);

			/* read back and check */
			reg.val = regs[TSREC_EXP_VC_DT_OFFSET(i, j)];
			if (reg.bits.TSREC_N_EXP_VSYNC_VC == v_vc
				&& reg.bits.TSREC_N_EXP_HSYNC_VC == h_vc
				&& reg.bits.TSREC_N_EXP_HSYNC_DT == h_dt) {
				/* pass */
				UT_INF(
					"PASS: UT get (vc/dt):(%u/%u/%u) matches to UT set (vc/dt):(%u/%u/%u)\n",
					reg.bits.TSREC_N_EXP_VSYNC_VC,
					reg.bits.TSREC_N_EXP_HSYNC_VC,
					reg.bits.TSREC_N_EXP_HSYNC_DT,
					v_vc, h_vc, h_dt);
			} else {
				UT_ERR(
					"ERROR: UT get (vc/dt):(%u/%u/%u) does NOT match to UT set (vc/dt):(%u/%u/%u)\n",
					reg.bits.TSREC_N_EXP_VSYNC_VC,
					reg.bits.TSREC_N_EXP_HSYNC_VC,
					reg.bits.TSREC_N_EXP_HSYNC_DT,
					v_vc, h_vc, h_dt);
			}

			/* reset settings */
			UT_INF("\n");
			UT_INF("UT Reset => exp vc/dt cfg ...\n");
			mtk_cam_seninf_tsrec_rst_tsrec_exp_vc_dt(i, j);
		}
	}

	UT_FUNC_END();
}


static void ut_fs_tsrec_exp_cnt(void)
{
	unsigned int i = 0, j = 0, k = 0;

	UT_FUNC_START();

	for (i = 0; i < TSREC_MAX_CNT; ++i) {
		for (j = 0; j < TSREC_EXP_MAX_CNT; ++j) {
			for (k = 0; k < TSREC_TS_REC_MAX_CNT; ++k) {
				unsigned long long ret = 0, val = 0;

				val = i * 100 + 100 +
					j * 10000 + 10000 +
					k * 1000000 + 1000000;

#if !(TSREC_WITH_64_BITS_TIMER_RG)
				regs[TSREC_EXP_CNT_OFFSET(i, j, k)] =
					(unsigned int)val;

				UT_INF("UT Test => set (TS:%u(%#x)) : %llu(%#llx)\n",
					regs[TSREC_EXP_CNT_OFFSET(i, j, k)],
					regs[TSREC_EXP_CNT_OFFSET(i, j, k)],
					val,
					val);
#else // !TSREC_WITH_64_BITS_TIMER_RG
				regs[TSREC_EXP_CNT_OFFSET(i, j, k)] = val;
				regs[TSREC_EXP_CNT_M_OFFSET(i, j, k)] = val + 32;

				val = regs[TSREC_EXP_CNT_M_OFFSET(i, j, k)];
				val = ((val << 32)
					| (regs[TSREC_EXP_CNT_OFFSET(i, j, k)]));

				UT_INF("UT Test => set (TS_M:%u(%#x)|TS_L:%u(%#x)) : %llu(%#llx)\n",
					regs[TSREC_EXP_CNT_M_OFFSET(i, j, k)],
					regs[TSREC_EXP_CNT_M_OFFSET(i, j, k)],
					regs[TSREC_EXP_CNT_OFFSET(i, j, k)],
					regs[TSREC_EXP_CNT_OFFSET(i, j, k)],
					val,
					val);
#endif // !TSREC_WITH_64_BITS_TIMER_RG

				ret = mtk_cam_seninf_g_tsrec_exp_cnt(i, j, k);

				if (ret != val) {
					UT_ERR(
						"ERROR: func return val:%llu does NOT match to UT set val:%llu\n",
						ret, val);
				} else {
					UT_INF(
						"PASS: func return val:%llu matches to UT set val:%llu\n",
						ret, val);
				}
			}
		}
	}

	UT_FUNC_END();
}


static void ut_fs_tsrec_reg_api(void)
{
	ut_fs_tsrec_top_cfg_clk_en();

	ut_fs_tsrec_timer_cfg();

	ut_fs_tsrec_latch_time();

	ut_fs_tsrec_int_en();

	ut_fs_tsrec_int_status();

	ut_fs_tsrec_n_cfg();

	ut_fs_tsrec_sw_rst();

	ut_fs_tsrec_ts_cnt();

	ut_fs_tsrec_trig_src();

	ut_fs_tsrec_exp_vc_dt();

	ut_fs_tsrec_exp_cnt();
}


/*--------------------------------------------------------------------------*/
// for tsrec flow APIs UT
/*--------------------------------------------------------------------------*/
static void ut_fs_tsrec_init(void)
{
	unsigned int i = 0;

	UT_FUNC_START();

	UT_INF("before UT call init ... dump data\n");
	for (i = 0; i < SENINF_MAX_CNT; ++i)
		mtk_cam_seninf_tsrec_dbg_dump_vc_dt_info(i, __func__);
	mtk_cam_seninf_tsrec_dbg_dump_tsrec_n_regs_info(__func__);


	// UT_INF("UT first init\n");
	// mtk_cam_seninf_tsrec_uninit();
	mtk_cam_seninf_tsrec_init();


	// UT_INF("UT second init w/o uninit\n");
	// mtk_cam_seninf_tsrec_init();

	// UT_INF("UT third init w/ uninit\n");
	// mtk_cam_seninf_tsrec_uninit();
	// mtk_cam_seninf_tsrec_init();


	UT_INF("after UT call init ... dump data again\n");
	for (i = 0; i < SENINF_MAX_CNT; ++i)
		mtk_cam_seninf_tsrec_dbg_dump_vc_dt_info(i, __func__);
	mtk_cam_seninf_tsrec_dbg_dump_tsrec_n_regs_info(__func__);

	UT_FUNC_END();
}


static void ut_fs_tsrec_timer_enable(void)
{
	UT_FUNC_START();

	UT_INF("UT Test => check if enable at first and disable at least (1) ...\n");
	mtk_cam_seninf_tsrec_timer_enable(1);
	mtk_cam_seninf_tsrec_timer_enable(1);
	mtk_cam_seninf_tsrec_timer_enable(1);
	mtk_cam_seninf_tsrec_timer_enable(0);
	mtk_cam_seninf_tsrec_timer_enable(0);
	mtk_cam_seninf_tsrec_timer_enable(0);

	UT_INF("\n");
	UT_INF("UT Test => check if enable at first and disable at least (2) ...\n");
	mtk_cam_seninf_tsrec_timer_enable(1);
	mtk_cam_seninf_tsrec_timer_enable(1);
	mtk_cam_seninf_tsrec_timer_enable(0);
	mtk_cam_seninf_tsrec_timer_enable(0);
	mtk_cam_seninf_tsrec_timer_enable(1);
	mtk_cam_seninf_tsrec_timer_enable(0);

	UT_INF("\n");
	UT_INF("UT Test => check repeatly disable then enable disable once ...\n");
	mtk_cam_seninf_tsrec_timer_enable(0);
	mtk_cam_seninf_tsrec_timer_enable(0);
	mtk_cam_seninf_tsrec_timer_enable(1);
	mtk_cam_seninf_tsrec_timer_enable(0);

	UT_FUNC_END();
}


static void ut_fs_tsrec_update_vc_dt_info_proc(const unsigned int idx,
	const struct mtk_cam_seninf_tsrec_vc_dt_info info_arr[],
	const unsigned int arr_len)
{
	struct seninf_ctx *inf_ctx = NULL;
	unsigned int i = 0;

	UT_INF("UT Test => reset seninf_idx:%u vc/dt info ...\n", idx);
	mtk_cam_seninf_tsrec_reset_vc_dt_info(inf_ctx, idx);

	UT_INF("\n");
	for (i = 0; i < arr_len; ++i) {
		UT_INF(
			"UT Test => set (vc/dt) : (%#x/%#x), PAD:%u, cust_assign_to_tsrec_exp_id:%u, is_sensor_hw_pre_latch_exp:%u\n",
			info_arr[i].vc, info_arr[i].dt, info_arr[i].out_pad,
			info_arr[i].cust_assign_to_tsrec_exp_id,
			info_arr[i].is_sensor_hw_pre_latch_exp);

		mtk_cam_seninf_tsrec_update_vc_dt_info(inf_ctx, idx, &info_arr[i]);
	}
}


static void ut_fs_tsrec_update_vc_dt_info(void)
{
	struct seninf_ctx *inf_ctx = NULL;
	/* custom config */
	const struct mtk_cam_seninf_tsrec_vc_dt_info info_arr_0[] = {
		{.vc = 0x0, .dt = 0x2b, .out_pad = PAD_SRC_RAW0},
		{.vc = 0x1, .dt = 0x2b, .out_pad = PAD_SRC_RAW1,
			.is_sensor_hw_pre_latch_exp = 1},
		{.vc = 0x3, .dt = 0x2b, .out_pad = PAD_SRC_PDAF1},
		{.vc = 0x4, .dt = 0x2b, .out_pad = PAD_SRC_PDAF3},
	};
	const struct mtk_cam_seninf_tsrec_vc_dt_info info_arr_1[] = {
		{.vc = 0x0, .dt = 0x2c, .out_pad = PAD_SRC_RAW_EXT0,
			.cust_assign_to_tsrec_exp_id = 1},
		{.vc = 0x1, .dt = 0x12, .out_pad = PAD_SRC_GENERAL0,
			.cust_assign_to_tsrec_exp_id = 0},
		{.vc = 0x2, .dt = 0x2e, .out_pad = PAD_SRC_PDAF0,
			.cust_assign_to_tsrec_exp_id = 0},
		{.vc = 0x0, .dt = 0x2b, .out_pad = PAD_SRC_RAW0,
			.cust_assign_to_tsrec_exp_id = 3,
			.is_sensor_hw_pre_latch_exp = 1},
	};
	const unsigned int idx = 6;

	UT_FUNC_START();

	UT_INF("UT Test => using seninf_idx:%u\n", idx);

	/* test case 1 */
	/* stagger sensor vc info from log */
	mtk_cam_seninf_tsrec_dbg_dump_vc_dt_info(idx, __func__);
	mtk_cam_seninf_tsrec_reset_vc_dt_info(inf_ctx, idx);
	mtk_cam_seninf_tsrec_dbg_dump_vc_dt_info(idx, __func__);
	ut_fs_tsrec_update_vc_dt_info_proc(idx, info_arr_1, 4);

	/* test case 2 */
	/* pre-ISP sensor vc info from log */
	mtk_cam_seninf_tsrec_dbg_dump_vc_dt_info(idx, __func__);
	mtk_cam_seninf_tsrec_reset_vc_dt_info(inf_ctx, idx);
	mtk_cam_seninf_tsrec_dbg_dump_vc_dt_info(idx, __func__);
	ut_fs_tsrec_update_vc_dt_info_proc(idx, info_arr_0, 4);


	// UT_INF("UT Reset => call reset vc dt info\n");
	// mtk_cam_seninf_tsrec_reset_vc_dt_info(inf_ctx, idx);
	mtk_cam_seninf_tsrec_dbg_dump_vc_dt_info(idx, __func__);
	mtk_cam_seninf_tsrec_dbg_dump_tsrec_n_regs_info(__func__);

	UT_FUNC_END();
}


static void ut_fs_tsrec_settings_start(void)
{
	unsigned int idx = 6, tsrec_no = 2;

	UT_FUNC_START();

	regs[TSREC_TOP_CFG_OFFSET] = (1U << tsrec_no);
	UT_INF("UT Test => set TSREC_TOP_CFG to %#x for checking it be reset ...\n",
		regs[TSREC_TOP_CFG_OFFSET]);

	UT_INF("UT Test => using seninf_idx:%u, tsrec_no:%u ...\n",
		idx, tsrec_no);
	UT_INF("\n");

	mtk_cam_seninf_tsrec_n_start(idx, tsrec_no);

	UT_INF("\n");
	UT_INF("UT Reset => using seninf_idx:%u, tsrec_no:%u\n",
		idx, tsrec_no);
	mtk_cam_seninf_tsrec_n_reset(idx);
	regs[TSREC_TOP_CFG_OFFSET] = 0;

	UT_FUNC_END();
}


static void ut_fs_tsrec_query_ts_records(void)
{
	/* custom config */
	const unsigned int ut_curr_tick = 11223344;
	const unsigned int ut_ts_cnt = 0; // 0x312;
	const unsigned int tsrec_no = 3;
	unsigned int addr_offset = 0;
	unsigned int i = 0, j = 0;
#if (TSREC_WITH_64_BITS_TIMER_RG)
	unsigned int addr_offset_M = 0;
	unsigned long long ts_u64 = 0;
#endif // TSREC_WITH_64_BITS_TIMER_RG

	UT_FUNC_START();

	UT_INF("UT Test => using tsrec_no:%u ...\n", tsrec_no);

	regs[TSREC_TIMER_LAT_OFFSET] = ut_curr_tick;
	UT_INF("UT Test => set curr tick to %u for latching ...\n",
		regs[TSREC_TIMER_LAT_OFFSET]);

	regs[TSREC_TS_CNT_OFFSET(tsrec_no)] = ut_ts_cnt;
	UT_INF("UT Test => set TS CNT to %#x ...\n",
		regs[TSREC_TS_CNT_OFFSET(tsrec_no)]);

	for (i = 0; i < TSREC_EXP_MAX_CNT; ++i) {
		for (j = 0; j < TSREC_TS_REC_MAX_CNT; ++j) {

			addr_offset =
				TSREC_EXP_CNT_OFFSET(tsrec_no, i, j);

			regs[addr_offset] = 33333*TSREC_TICK_FACTOR
				+ i*10000*TSREC_TICK_FACTOR
				+ ((j%TSREC_TS_REC_MAX_CNT)*33333*TSREC_TICK_FACTOR);

#if (TSREC_WITH_64_BITS_TIMER_RG)

			addr_offset_M =
				TSREC_EXP_CNT_M_OFFSET(tsrec_no, i, j);

			regs[addr_offset_M] = 113;

			ts_u64 = regs[addr_offset_M];
			ts_u64 = (ts_u64 << 32) | regs[addr_offset];

			UT_INF("UT Test => set timestamp to exp%u[%u]:%llu (%u | %u)\n",
				i, j, ts_u64,
				regs[addr_offset_M], regs[addr_offset]);

#else // TSREC_WITH_64_BITS_TIMER_RG

			UT_INF("UT Test => set timestamp to exp%u[%u]:%u\n",
				i, j, regs[addr_offset]);

#endif // TSREC_WITH_64_BITS_TIMER_RG
		}
	}

	mtk_cam_seninf_tsrec_query_ts_records(tsrec_no);

	UT_INF("\n");
	mtk_cam_seninf_tsrec_dbg_dump_ts_records(tsrec_no);

	UT_FUNC_END();
}


static void ut_fs_tsrec_irq_handler(void)
{
	/* coustom config */
	// const unsigned int int_status = 0x400008;
	const unsigned int int_status = 0x400048;
	const unsigned int int_status_2 = 0x40000;

	UT_FUNC_START();

	/* setup status for testing */
	regs[TSREC_INT_STATUS_OFFSET] = int_status;
	UT_INF("UT Test => set INT_STATUS:%#x\n",
		regs[TSREC_INT_STATUS_OFFSET]);
	regs[TSREC_INT_STATUS_2_OFFSET] = int_status_2;
	UT_INF("UT Test => set INT_STATUS_2:%#x\n",
		regs[TSREC_INT_STATUS_2_OFFSET]);

	/* TODO: complete this UT API */
	mtk_cam_seninf_tsrec_ut_dbg_irq_seninf_tsrec();

	/* reset/clear status */
	regs[TSREC_INT_STATUS_OFFSET] = 0;
	regs[TSREC_INT_STATUS_2_OFFSET] = 0;

	UT_FUNC_END();
}


static void ut_fs_tsrec_flow_api(void)
{
	ut_fs_tsrec_init();

	ut_fs_tsrec_timer_enable();

	ut_fs_tsrec_update_vc_dt_info();

	ut_fs_tsrec_settings_start();

	ut_fs_tsrec_query_ts_records();

	ut_fs_tsrec_irq_handler();
}


/*--------------------------------------------------------------------------*/
// for tsrec sysfs API UT
/*--------------------------------------------------------------------------*/
static void ut_fs_tsrec_ut_sysfs_ctrl(void)
{
	unsigned int val = 0;

	printf(GREEN
		"\n\n\n>>> Please key in TSREC console ctrl cmd val ! <<<\n"
		NONE);

	printf(LIGHT_PURPLE
		">>> (Input 1 integer) \"set command val\" : "
		NONE);
	scanf("%u", &val);

	mtk_cam_seninf_tsrec_ut_dbg_sysfs_ctrl(val);
}


/*--------------------------------------------------------------------------*/
// tsrec UT main entry
/*--------------------------------------------------------------------------*/
void ut_fs_tsrec(void)
{
	bool terminated = false;
	unsigned int select_ut_case = 0;

	while (!terminated) {
		mtk_cam_seninf_tsrec_init_for_ut_test(1,
			TSREC_MAX_CNT, SENINF_MAX_CNT);

		printf(GREEN
			"\n\n\n>>> Please choose TSREC UT Test case bellow! <<<\n"
			NONE);
		printf(GREEN
			"(UT TSREC: reg from %#x to %#x)\n",
			TSREC_BASE, UT_TSREC_RG_LAST);
		printf(GREEN
			">>> Run : [1] TSREC register APIs test\n"
			NONE);
		printf(GREEN
			">>> Run : [2] TSREC flow APIs test\n"
			NONE);
		printf(GREEN
			">>> Run : [9] TSREC UT sysfs ctrl\n"
			NONE);
		printf(GREEN
			">>> Run : [X] End UT test\n"
			NONE);

		printf(LIGHT_PURPLE
			">>> (Input 1 integer) \"select a case id in []\" : "
			NONE);
		scanf("%u", &select_ut_case);


		switch (select_ut_case) {
		case 1:
			ut_fs_tsrec_chk_regs_addr();
			ut_fs_tsrec_reg_api();
			break;

		case 2:
			ut_fs_tsrec_chk_regs_addr();
			ut_fs_tsrec_flow_api();
			break;

		case 9:
			ut_fs_tsrec_ut_sysfs_ctrl();
			break;

		default:
			terminated = true;
			break;
		}

		mtk_cam_seninf_tsrec_init_for_ut_test(0,
			TSREC_MAX_CNT, SENINF_MAX_CNT);

		printf("\n\n\n");
	}
}
