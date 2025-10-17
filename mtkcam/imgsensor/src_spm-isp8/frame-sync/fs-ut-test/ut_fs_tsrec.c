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


#if (TSREC_HW_VER_ISP_8)
#define SENINF_BASE                  (0x3A300000)
#define UT_TSREC_RG_LAST             (0x3A3d0084)   // ISP8

#define TSREC_MAX_CNT                (12)
#define TSREC_IRQ_MAX_CNT            (4)    /* HW support up to 4 VM devices */
#else	// => ISP7sp/ISP7
#define SENINF_BASE                  (0x1A00E000)
#if !(TSREC_WITH_64_BITS_TIMER_RG)
#define UT_TSREC_RG_LAST             (0x1A02481C)   // ISP7s
#else
#define UT_TSREC_RG_LAST             (0x1A02484C)   // ISP7sp --- 64 bits ts
#endif

#define TSREC_MAX_CNT                (6)
#define TSREC_IRQ_MAX_CNT            (1)
#endif


#define UT_TSREC_REGS_SIZE           (UT_TSREC_RG_LAST - TSREC_BASE + 1)
static unsigned int regs[UT_TSREC_REGS_SIZE];
static unsigned int regs_base[TSREC_MAX_CNT];


/******************************************************************************
 * unit test static structures
 *****************************************************************************/
struct ut_fs_tsrec_irq_info_st {
	unsigned int tsrec_no;
	unsigned int status;

	unsigned int intr_status;
	unsigned int intr_status_2;

	unsigned int pre_latch_exp_no;

	unsigned long long tsrec_ts_us;
	unsigned long long sys_ts_ns;
	unsigned long long duration_ns;
	unsigned long long duration_2_ns;
	unsigned long long worker_handle_ts_ns;
};

#define UT_TSREC_MSG_FIFO_SIZE       (TSREC_MAX_CNT * TSREC_EXP_MAX_CNT)
struct ut_fs_tsrec_msgfifo {
	// unsigned int push_idx;
	// unsigned int pop_idx;

	struct ut_fs_tsrec_irq_info_st irq_info[UT_TSREC_MSG_FIFO_SIZE];

	unsigned int items;
	// struct ut_fs_tsrec_irq_info_st irq_info;
};
static struct ut_fs_tsrec_msgfifo ut_msg_fifo;


static struct device_node seninf_top_node = {
	.name = "ut_tsrec_seninf_top",
	.full_name = "ut_tsrec_seninf_top@",
};
static struct device seninf_dev = {
	.of_node = &seninf_top_node,
};


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
unsigned int ut_fs_tsrec_g_tsrec_max_cnt(void)
{
	return TSREC_MAX_CNT;
}

unsigned int ut_fs_tsrec_g_seninf_max_cnt(void)
{
	return SENINF_MAX_CNT;
}

unsigned int ut_fs_tsrec_g_tsrec_irq_max_cnt(void)
{
	return TSREC_IRQ_MAX_CNT;
}


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


void ut_fs_tsrec_fifo_out(void *p_data)
{
	if (!p_data)
		return;
	if (ut_msg_fifo.items == 0)
		return;

	memcpy(p_data, &ut_msg_fifo.irq_info[ut_msg_fifo.items],
		sizeof(struct ut_fs_tsrec_irq_info_st));

	--ut_msg_fifo.items;
}


unsigned int ut_fs_tsrec_fifo_in(const void *p_data)
{
	if (ut_msg_fifo.items + 1 >= UT_TSREC_MSG_FIFO_SIZE)
		return -1;

	++ut_msg_fifo.items;
	memcpy(&ut_msg_fifo.irq_info[ut_msg_fifo.items], p_data,
		sizeof(struct ut_fs_tsrec_irq_info_st));

	return ut_msg_fifo.items;
}


/******************************************************************************
 * UT TSREC static functions
 *****************************************************************************/
static inline void ut_fs_tsrec_init_regs_base_info(void)
{
	unsigned int i;

	for (i = 0; i < TSREC_MAX_CNT; ++i) {
		regs_base[i] =
			TSREC_N_1ST_BASE_OFFSET + (i * TSREC_N_OFFSET);
	}
}

static void ut_fs_tsrec_chk_regs_addr(void)
{
	unsigned int i = 0, j = 0, k = 0;

	UT_FUNC_START();

	UT_INF("==> Print TSREC SW/HW spec. define:\n");
	UT_INF("%30s%3u\n", "TSREC_HW_VER_ISP_8:",
		TSREC_HW_VER_ISP_8);
	UT_INF("\n");
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
	UT_INF("%30s%3u\n", "TSREC_IRQ_MAX_CNT:",
		TSREC_IRQ_MAX_CNT);

	UT_INF("\n");
	UT_INF("==> Config for iomem type. define:\n");
	UT_INF("%30s%3u\n", "SENINF_UNIFY_IOMEM_MAPPING:",
		SENINF_UNIFY_IOMEM_MAPPING);
	UT_INF("%30s%3u\n", "ADDR_SHIFT_FROM_TSREC_TOP:",
		ADDR_SHIFT_FROM_TSREC_TOP);

	UT_INF("\n");
	UT_INF("==> Print TSREC regs addr:\n");
	UT_INF("%30s %#10X\n", "TSREC_BASE:",
		TSREC_BASE);
	UT_INF("%30s %#10X\n", "SENINF_BASE:",
		SENINF_BASE);

	UT_INF("\n");
	UT_INF("%30s %#10X\n", "TSREC_N_1ST_BASE_OFFSET:",
		TSREC_N_1ST_BASE_OFFSET);
	UT_INF("%30s %#10X\n", "TSREC_N_OFFSET:",
		TSREC_N_OFFSET);
	ut_fs_tsrec_init_regs_base_info();

	UT_INF("----------------------------------------------------------\n");
	UT_INF("%30s %#10X(+%#6X)\n", "TSREC_CFG:",
		TSREC_ADDR(-1, TSREC_TOP_CFG_OFFSET),
		TSREC_TOP_CFG_OFFSET);
	UT_INF("%30s %#10X(+%#6X)\n", "TSREC_TIMER_CFG:",
		TSREC_ADDR(-1, TSREC_TIMER_CFG_OFFSET),
		TSREC_TIMER_CFG_OFFSET);
	UT_INF("%30s %#10X(+%#6X)\n", "TSREC_TIMER_LAT:",
		TSREC_ADDR(-1, TSREC_TIMER_LAT_OFFSET),
		TSREC_TIMER_LAT_OFFSET);

#if (!TSREC_HW_VER_ISP_8)
	UT_INF("%30s %#10X(+%#6X)\n", "TSREC_INT_EN:",
		TSREC_ADDR(-1, TSREC_INT_EN_OFFSET),
		TSREC_INT_EN_OFFSET);
	UT_INF("%30s %#10X(+%#6X)\n", "TSREC_INT_STATUS:",
		TSREC_ADDR(-1, TSREC_INT_STATUS_OFFSET),
		TSREC_INT_STATUS_OFFSET);
	UT_INF("%30s %#10X(+%#6X)\n", "TSREC_INT_EN_2:",
		TSREC_ADDR(-1, TSREC_INT_EN_2_OFFSET),
		TSREC_INT_EN_2_OFFSET);
	UT_INF("%30s %#10X(+%#6X)\n", "TSREC_INT_STATUS_2:",
		TSREC_ADDR(-1, TSREC_INT_STATUS_2_OFFSET),
		TSREC_INT_STATUS_2_OFFSET);
#endif

#if (TSREC_WITH_64_BITS_TIMER_RG)
	UT_INF("%30s %#10X(+%#6X)\n", "TSREC_TIMER_LAT_M:",
		TSREC_ADDR(-1, TSREC_TIMER_LAT_M_OFFSET),
		TSREC_TIMER_LAT_M_OFFSET);
#endif

	for (i = 0; i < TSREC_MAX_CNT; ++i) {
		UT_INF("\n");
		UT_INF("%30s [%2u]:%#x\n", "TSREC_N_REG_BASE:", i, regs_base[i]);
		UT_INF("[%2u]%26s %#10X(+%#6X)\n", i, "TSREC_n_CFG:",
			TSREC_ADDR(i,
				TSREC_CFG_OFFSET(i)),
			TSREC_CFG_OFFSET(i));
#if (TSREC_HW_VER_ISP_8)
		UT_INF("[%2u]%26s %#10X(+%#6X)\n", i, "TSREC_n_INT_EN_OFFSET:",
			TSREC_ADDR(i,
				TSREC_N_INT_EN_OFFSET),
			TSREC_N_INT_EN_OFFSET);
		UT_INF("[%2u]%26s %#10X(+%#6X)\n", i, "TSREC_n_INT_STATUS_OFFSET:",
			TSREC_ADDR(i,
				TSREC_N_INT_STATUS_OFFSET),
			TSREC_N_INT_STATUS_OFFSET);
#endif
		UT_INF("[%2u]%26s %#10X(+%#6X)\n", i, "TSREC_n_SW_RST:",
			TSREC_ADDR(i,
				TSREC_SW_RST_OFFSET(i)),
			TSREC_SW_RST_OFFSET(i));
		UT_INF("[%2u]%26s %#10X(+%#6X)\n", i, "TSREC_n_TS_CNT:",
			TSREC_ADDR(i,
				TSREC_TS_CNT_OFFSET(i)),
			TSREC_TS_CNT_OFFSET(i));
		UT_INF("[%2u]%26s %#10X(+%#6X)\n", i, "TSREC_n_TRIG_SRC:",
			TSREC_ADDR(i,
				TSREC_TRIG_SRC_OFFSET(i)),
			TSREC_TRIG_SRC_OFFSET(i));

		for (j = 0; j < TSREC_EXP_MAX_CNT; ++j) {
			UT_INF("[%2u]%22s(%u): %#10X(+%#6X)\n", i,
				"TSREC_n_EXP_VC_DT",
				j,
				TSREC_ADDR(i,
					TSREC_EXP_VC_DT_OFFSET(i, j)),
				TSREC_EXP_VC_DT_OFFSET(i, j));
		}

		for (j = 0; j < TSREC_EXP_MAX_CNT; ++j) {
			for (k = 0; k < TSREC_TS_REC_MAX_CNT; ++k) {
				UT_INF("[%2u]%21s%u(%u): %#10X(+%#6X)\n", i,
					"TSREC_n_EXP_CNT",
					k, j,
					TSREC_ADDR(i,
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
					TSREC_ADDR(i,
						TSREC_EXP_CNT_M_OFFSET(i, j, k)),
					TSREC_EXP_CNT_M_OFFSET(i, j, k));
			}
		}
#endif

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


#if !(TSREC_HW_VER_ISP_8)
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
#else
static void ut_fs_tsrec_device_irq_sel(void)
{
	const unsigned int irq_sel_val[TSREC_IRQ_MAX_CNT] = {0xf, 0xf0, 0xf00, 0xf000};
	unsigned int i;

	UT_FUNC_START();

	UT_INF("UT => irq sel val:(%#x/%#x/%#x/%#x)\n",
		irq_sel_val[0], irq_sel_val[1], irq_sel_val[2], irq_sel_val[3]);

	for (i = 0; i < TSREC_IRQ_MAX_CNT; ++i)
		mtk_cam_seninf_tsrec_s_device_irq_sel(i, irq_sel_val[i]);

	UT_FUNC_END();
}
#endif // !TSREC_HW_VER_ISP_8


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


#if (TSREC_HW_VER_ISP_8)
static void ut_fs_tsrec_n_int_en_proc(const unsigned int n,
	const unsigned int exp_en[],
	const unsigned int intr_en_true, const unsigned int flag,
	const unsigned int en)
{
	/* flag => vsync:0 / 1st-Hsync:1 / vsync & hsync:2 */
	const unsigned int vsync_en = (flag == 0 || flag == 2) ? 1 : 0;
	const unsigned int hsync_en = (flag == 1 || flag == 2) ? 1 : 0;
	const unsigned int addr_offset = TSREC_INT_EN_OFFSET(n);
	unsigned int intr_en;

	UT_INF(
		"UT Test => set INTR_EN:(%u), for (n:%u, exp_en:(%u/%u/%u), intr_en_true:%#x, flag:%u(Vsync:0/Hsync:1/Vsync+Hsync:2)) ...\n",
		en, n, exp_en[0], exp_en[1], exp_en[2], intr_en_true, flag);

	/* INTR enable */
	if (vsync_en) {
		mtk_cam_seninf_s_tsrec_n_intr_en(n,
			exp_en[0], exp_en[1], exp_en[2],
			0, en);
	}
	if (hsync_en) {
		mtk_cam_seninf_s_tsrec_n_intr_en(n,
			exp_en[0], exp_en[1], exp_en[2],
			1, en);
	}

	/* read back and check */
	intr_en = mtk_cam_seninf_g_tsrec_n_intr_en(n);
	if (intr_en == regs[addr_offset]) {
		UT_INF(
			"PASS: read back intr_en:%#x matches to intr_en set:%#x\n",
			intr_en, regs[addr_offset]);
	} else {
		UT_ERR(
			"ERROR: read back intr_en:%#x does NOT match to intr_en set:%#x\n",
			intr_en, regs[addr_offset]);
	}
	if (intr_en == intr_en_true) {
		UT_INF(
			"PASS: read back intr_en:%#x matches to UT intr_en_true:%#x\n",
			intr_en, intr_en_true);
	} else {
		UT_ERR(
			"ERROR: read back intr_en:%#x does NOT match to UT intr_en_true:%#x\n",
			intr_en, intr_en_true);
	}
}


static void ut_fs_tsrec_n_int_en(void)
{
	/* custom config */
	const unsigned int exp_en[TSREC_EXP_MAX_CNT] = {1, 0, 1};
	unsigned int vsync_en = 0, hsync_en = 0, intr_en;
	unsigned int i;

	/* setup UT test variables for checking */
	for (i = 0; i < TSREC_EXP_MAX_CNT; ++i) {
		if (exp_en[i] == 0)
			continue;

		vsync_en |= (1UL << (i + TSREC_INT_EN_VSYNC_BASE_BIT));
		hsync_en |= (1UL << (i + TSREC_INT_EN_HSYNC_BASE_BIT));
	}
	intr_en = (vsync_en | hsync_en);


	UT_FUNC_START();

	for (i = 0; i < TSREC_MAX_CNT; ++i) {
		/* flag => vsync:0 / 1st-Hsync:1 / vsync & hsync:2 */

		/* only vsync */
		ut_fs_tsrec_n_int_en_proc(i, exp_en, vsync_en, 0, 1);
		ut_fs_tsrec_n_int_en_proc(i, exp_en, 0, 0, 0);

		/* only hsync */
		ut_fs_tsrec_n_int_en_proc(i, exp_en, hsync_en, 1, 1);
		ut_fs_tsrec_n_int_en_proc(i, exp_en, 0, 1, 0);

		/* vsync & hsync */
		ut_fs_tsrec_n_int_en_proc(i, exp_en, intr_en, 2, 1);
		ut_fs_tsrec_n_int_en_proc(i, exp_en, 0, 2, 0);
	}

	UT_FUNC_END();
}


static void ut_fs_tsrec_n_int_status_proc(const unsigned int n,
	const unsigned int val)
{
	const unsigned int addr_offset = TSREC_INT_STATUS_OFFSET(n);
	unsigned int ret;

	/* set UT RG val */
	regs[addr_offset] = val;
	UT_INF("UT Test => get (n:%u, val:%#x), set int_status(+%#x):%#x ...\n",
		n, val,
		addr_offset, regs[addr_offset]);

	/* get int status to check if result matches */
	ret = mtk_cam_seninf_g_tsrec_n_intr_status(n);
	if (ret != val) {
		UT_ERR(
			"ERROR: func return ret:%#x (int_status) does not match to UT val:%#x\n",
			ret, val);
	} else {
		UT_INF(
			"PASS: func return ret:%#x (int_status) matches to UT val:%#x\n",
			ret, val);
	}

	/* test clear status */
	UT_INF("UT Test => for testing write clear, enable write clear ...\n");
	mtk_cam_seninf_s_tsrec_intr_wclr_en(1);
	UT_INF("UT Test => write clear, write val:%#x ...\n", val);
	/* call for write clear */
	mtk_cam_seninf_clr_tsrec_n_intr_status(n, val);
	/* check result */
	ret = regs[addr_offset];
	regs[addr_offset] ^= val;
	if (regs[addr_offset]) {
		UT_ERR("ERROR: int_status(+%#x):%#x => %#x, is not cleared ...\n",
			addr_offset, ret, regs[addr_offset]);
	} else {
		UT_INF("PASS: int_status(+%#x):%#x => %#x, is cleared ...\n",
			addr_offset, ret, regs[addr_offset]);
	}

	/* reset all operations & variables */
	UT_INF("UT Test => test END, reset all ...\n");
	mtk_cam_seninf_clr_tsrec_n_intr_status(n, 0);
	mtk_cam_seninf_s_tsrec_intr_wclr_en(0);
	regs[addr_offset] = 0;


	UT_INF("\n");
	UT_INF("\n");
	UT_INF("\n");
}


static void ut_fs_tsrec_n_int_status(void)
{
	const unsigned int test_val = 0x1b;	// exp0, exp1 Vsync & 1st-Hsync
	unsigned int i;

	UT_FUNC_START();

	for (i = 0; i < TSREC_MAX_CNT; ++i)
		ut_fs_tsrec_n_int_status_proc(i, test_val);

	UT_FUNC_END();
}
#endif // TSREC_HW_VER_ISP_8


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

#if !(TSREC_HW_VER_ISP_8)
	ut_fs_tsrec_int_en();

	ut_fs_tsrec_int_status();
#else
	ut_fs_tsrec_device_irq_sel();
#endif

	ut_fs_tsrec_n_cfg();

#if (TSREC_HW_VER_ISP_8)
	ut_fs_tsrec_n_int_en();

	ut_fs_tsrec_n_int_status();
#endif

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
	mtk_cam_seninf_tsrec_init(&seninf_dev, NULL);


	// UT_INF("UT second init w/o uninit\n");
	// mtk_cam_seninf_tsrec_init(&seninf_dev, NULL);

	// UT_INF("UT third init w/ uninit\n");
	// mtk_cam_seninf_tsrec_uninit();
	// mtk_cam_seninf_tsrec_init(&seninf_dev, NULL);


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


#if (TSREC_HW_VER_ISP_8)
static void ut_fs_tsrec_irq_handler_isp8(void)
{
	/* coustom config */
	const unsigned int irq_tsrec_no = 2;
	const unsigned int irq_status = 0x2d; // 101101b
	const unsigned int base_shift = TSREC_INT_STATUS_OFFSET(irq_tsrec_no);
	const unsigned int irq_tsrec_no_2 = 6;
	const unsigned int irq_status_2 = 0x5; // 101b
	const unsigned int base_shift_2 = TSREC_INT_STATUS_OFFSET(irq_tsrec_no_2);

	UT_FUNC_START();

	/* setup status for testing */
	regs[base_shift] = irq_status;
	UT_INF("UT Test => set tsrec_no:%u (base_shift:%#x), INT_STATUS:%#x\n",
		irq_tsrec_no, base_shift, regs[base_shift]);
	mtk_cam_seninf_tsrec_ut_dbg_update_tsrec_intr_en_bits(irq_tsrec_no, 1);
	regs[base_shift_2] = irq_status_2;
	UT_INF("UT Test => set tsrec_no:%u (base_shift:%#x), INT_STATUS:%#x\n",
		irq_tsrec_no_2, base_shift_2, regs[base_shift_2]);
	mtk_cam_seninf_tsrec_ut_dbg_update_tsrec_intr_en_bits(irq_tsrec_no_2, 1);

	/* use dbg api to test */
	mtk_cam_seninf_tsrec_ut_dbg_irq_seninf_tsrec();

	/* reset/clear status */
	regs[base_shift] = 0;
	mtk_cam_seninf_tsrec_ut_dbg_update_tsrec_intr_en_bits(irq_tsrec_no, 0);
	regs[base_shift_2] = 0;
	mtk_cam_seninf_tsrec_ut_dbg_update_tsrec_intr_en_bits(irq_tsrec_no_2, 0);

	UT_FUNC_END();
}
#else
static void ut_fs_tsrec_irq_handler_isp7sp(void)
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
#endif // TSREC_HW_VER_ISP_8


static void ut_fs_tsrec_irq_handler(void)
{
#if (TSREC_HW_VER_ISP_8)
	ut_fs_tsrec_irq_handler_isp8();
#else
	ut_fs_tsrec_irq_handler_isp7sp();
#endif
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
// simulation for procedure of probe & remove parse dts hw info
/*--------------------------------------------------------------------------*/
static inline void ut_fs_tsrec_probe(void)
{
	struct seninf_core core;

	mtk_cam_seninf_tsrec_init(&seninf_dev, NULL);
	mtk_cam_seninf_tsrec_irq_init(&core);
}


static inline void ut_fs_tsrec_remove(void)
{
	mtk_cam_seninf_tsrec_uninit();
}


/*--------------------------------------------------------------------------*/
// tsrec UT main entry
/*--------------------------------------------------------------------------*/
void ut_fs_tsrec(void)
{
	bool terminated = false;
	unsigned int select_ut_case = 0;

	while (!terminated) {
		ut_fs_tsrec_probe();

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

		ut_fs_tsrec_remove();

		printf("\n\n\n");
	}
}
