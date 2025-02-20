// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2022 MediaTek Inc.


#include "mtk_cam-seninf-tsrec-regs-def.h"
#include "mtk_cam-seninf-tsrec-regs.h"
#include "mtk_cam-seninf-tsrec-def.h"
#include "mtk_cam-seninf-tsrec.h"

#ifdef FS_UT
#include "ut_fs_tsrec.h"
#endif


#define PFX "TSREC-REGS"
#define TSREC_LOG_DBG_DEF_CAT LOG_TSREC_REG


#ifndef FS_UT
/**
 * For common ctrl RG, e.g., TOP_TSREC_CFG, TSREC_INTR_EN, TSREC_INT_EN_2
 * that using one register to control several tsrec hw.
 */
DEFINE_SPINLOCK(tsrec_top_cfg_concurrency_lock);
DEFINE_SPINLOCK(tsrec_intr_en_concurrency_lock);
DEFINE_SPINLOCK(tsrec_intr_en_2_concurrency_lock);
#endif


/******************************************************************************
 * TSREC static variables
 *****************************************************************************/
static unsigned int tsrec_intr_write_clr;


/******************************************************************************
 * TSREC read/write register structure & variables
 *****************************************************************************/
#ifndef FS_UT
static void __iomem *tsrec_base_addr;
#endif // !FS_UT


struct tsrec_w_buffer {
	unsigned int shift;
	unsigned int mask;
	unsigned int op; // overwrite:2, set:1, clear:0;
	// unsigned int ts;
	unsigned int before;
	unsigned int after;
};

struct tsrec_r_buffer {
	unsigned int shift;
	// unsigned int ts;
	unsigned int val;
};


/******************************************************************************
 * TSREC registers basic/utilities functions
 *****************************************************************************/
static unsigned int tsrec_get_mask(
	const unsigned int shift, const unsigned int flag)
{
	return (flag) ? (1U << shift) : (~(1U << shift));
}


#ifndef FS_UT
static void tsrec_write_reg(struct tsrec_w_buffer *buf, const char *caller)
{
	void __iomem *__p = (tsrec_base_addr + buf->shift);
	u32 __v;

	/* error handling */
	/* --- SENINF NOT power up (exclude timer cfg */
	/* --- due to unsing this as a flag) --- */
	if (unlikely(get_tsrec_timer_en_status() == 0)) {
		TSREC_LOG_INF(
			"[%s] ERROR: tsrec_timer_en_status:%u (resume/suspend will change this status), maybe touch RG at a wrong timing, return   [%#x(+%#x)]:(%#x => %#x), mask:%#x (%u)(SET:1/CLR:0/OVW:2)]\n",
			caller,
			get_tsrec_timer_en_status(),
			TSREC_ADDR_BY_OFFSET(buf->shift),
			buf->shift,
			buf->before,
			buf->after,
			buf->mask,
			buf->op);
		return;
	}

	__v = readl(__p);
	buf->before = __v;

	/* overwrite:2 / set:1 / clear: 0 */
	if (buf->op == 2)
		__v = buf->mask;
	else if (buf->op)
		__v |= buf->mask;
	else
		__v &= buf->mask;

	writel(__v, __p);

	buf->after = readl(__p);
}
#else // => FS_UT
static void tsrec_write_reg(struct tsrec_w_buffer *buf, const char *caller)
{
	__uint32_t __v = ut_fs_tsrec_read_reg(buf->shift);

	buf->before = __v;

	/* overwrite:2 / set:1 / clear: 0 */
	if (buf->op == 2)
		__v = buf->mask;
	else if (buf->op)
		__v |= buf->mask;
	else
		__v &= buf->mask;

	ut_fs_tsrec_write_reg(buf->shift, __v);

	buf->after = ut_fs_tsrec_read_reg(buf->shift);
}
#endif // !FS_UT


#ifndef FS_UT
static void tsrec_read_reg(struct tsrec_r_buffer *buf, const char *caller)
{
	void __iomem *__p = (tsrec_base_addr + buf->shift);

	/* error handling */
	/* --- SENINF NOT power up (exclude timer cfg */
	/* --- due to unsing this as a flag) --- */
	if (unlikely(get_tsrec_timer_en_status() == 0)) {
		TSREC_LOG_INF(
			"[%s] ERROR: tsrec_timer_en_status:%u (resume/suspend will change this status), maybe touch RG at a wrong timing, return   [%#x(+%#x)]:(%#x)]\n",
			caller,
			get_tsrec_timer_en_status(),
			TSREC_ADDR_BY_OFFSET(buf->shift),
			buf->shift,
			buf->val);
		return;
	}

	buf->val = readl(__p);
}
#else // => FS_UT
static void tsrec_read_reg(struct tsrec_r_buffer *buf, const char *caller)
{
	buf->val = ut_fs_tsrec_read_reg(buf->shift);
}
#endif // !FS_UT


/******************************************************************************
 * TSREC registers CTRL/operation functions
 *****************************************************************************/
void mtk_cam_seninf_s_tsrec_top_cfg_clk_en_bit(const unsigned int tsrec_n,
	const unsigned int clk_en)
{
	struct tsrec_w_buffer w_buf = {0};

	/* check case / error handling */
	if (unlikely(!chk_tsrec_no_valid(tsrec_n, __func__))) {
		TSREC_LOG_INF(
			"ERROR: get non-valid tsrec_n, force return   [tsrec_n:%u, clk_en:%u]\n",
			tsrec_n,
			clk_en);
		return;
	}

	/* prepare for write register */
	w_buf.shift = TSREC_TOP_CFG_OFFSET;
	w_buf.mask = tsrec_get_mask(tsrec_n, clk_en);
	w_buf.op = (clk_en) ? 1 : 0;

	TSREC_SPIN_LOCK(&tsrec_top_cfg_concurrency_lock);

	tsrec_write_reg(&w_buf, __func__);

	/* sync reg ctrl info to tsrec_status */
	notify_tsrec_update_tsrec_n_clk_en_status(tsrec_n, clk_en);
	notify_tsrec_update_top_cfg(w_buf.after);

	TSREC_SPIN_UNLOCK(&tsrec_top_cfg_concurrency_lock);


	TSREC_LOG_DBG(
		"[%#x(+%#x)]:(%#x => %#x), mask:%#x (%u)(SET:1/CLR:0/OVW:2)   [tsrec_n:%u, clk_en:%u]\n",
		TSREC_ADDR_BY_OFFSET(w_buf.shift),
		w_buf.shift,
		w_buf.before,
		w_buf.after,
		w_buf.mask,
		w_buf.op,
		tsrec_n,
		clk_en);
}


void mtk_cam_seninf_s_tsrec_timer_cfg(const unsigned int en)
{
	struct tsrec_w_buffer w_buf = {0};
	union REG_TSREC_TIMER_CFG reg = {0};
	const unsigned int tsrec_using_global_timer =
		(TSREC_WITH_GLOBAL_TIMER) ? TSREC_USE_GLOBAL_TIMER : 0;

	/* config reg value */
	reg.bits.TSREC_TIMER_FIX_CLK_EN = 1;
	reg.bits.TSREC_TIMER_CK_EN = 1;
	reg.bits.TSREC_TIMER_CLR = 1; // write clr
#if (TSREC_WITH_GLOBAL_TIMER)
	reg.bits.TSREC_TIMER_SEL = (tsrec_using_global_timer) ? 1 : 0;
	reg.bits.TSREC_TIMER_EN = (tsrec_using_global_timer) ? 0 : 1;
#endif // TSREC_WITH_GLOBAL_TIMER

	/* prepare for write register */
	w_buf.shift = TSREC_TIMER_CFG_OFFSET;
	w_buf.mask = (en) ? (reg.val) : ~(reg.val);
	w_buf.op = (en) ? 1 : 0;
	tsrec_write_reg(&w_buf, __func__);

	/* sync reg ctrl info to tsrec_status */
	notify_tsrec_update_timer_cfg(w_buf.after);

	TSREC_LOG_DBG(
		"[%#x(+%#x)]:(%#x => %#x), mask:%#x (%u)(SET:1/CLR:0/OVW:2), timer_en_status:%u, (w/g_timer:%u(using:%u))   [en:%u]\n",
		TSREC_ADDR_BY_OFFSET(w_buf.shift),
		w_buf.shift,
		w_buf.before,
		w_buf.after,
		w_buf.mask,
		w_buf.op,
		notify_tsrec_get_timer_en_status(),
		TSREC_WITH_GLOBAL_TIMER,
		tsrec_using_global_timer,
		en);
}


unsigned long long mtk_cam_seninf_tsrec_latch_time(void)
{
	struct tsrec_w_buffer w_buf = {0};
	struct tsrec_r_buffer r_buf = {0};
#if (TSREC_WITH_64_BITS_TIMER_RG)
	struct tsrec_r_buffer r_buf_M = {0};
#endif
	union REG_TSREC_TIMER_CFG reg = {0};
	unsigned long long val = 0;

	/* config reg value */
	reg.bits.TSREC_TIMER_LAT = 1; // write clr

	/* prepare for write register */
	w_buf.shift = TSREC_TIMER_CFG_OFFSET;
	w_buf.mask = (reg.val);
	w_buf.op = 1; // set
	tsrec_write_reg(&w_buf, __func__);

	/* prepare for read register */
	r_buf.shift = TSREC_TIMER_LAT_OFFSET;
	tsrec_read_reg(&r_buf, __func__);
	val = r_buf.val;

#if (TSREC_WITH_64_BITS_TIMER_RG)
	r_buf_M.shift = TSREC_TIMER_LAT_M_OFFSET;
	tsrec_read_reg(&r_buf_M, __func__);
	val = r_buf_M.val;
	val = ((val << 32) | (r_buf.val));
#endif // TSREC_WITH_64_BITS_TIMER_RG


#if !defined(REDUCE_TSREC_LOG_IN_ISR_FUNC)
#if !(TSREC_WITH_64_BITS_TIMER_RG)
	TSREC_LOG_DBG(
		"[%#x(+%#x)]:(%#x => %#x), mask:%#x (%u)(SET:1/CLR:0/OVW:2), latch time = [%#x(+%#x)]:%llu\n",
		TSREC_ADDR_BY_OFFSET(w_buf.shift),
		w_buf.shift,
		w_buf.before,
		w_buf.after,
		w_buf.mask,
		w_buf.op,
		TSREC_ADDR_BY_OFFSET(r_buf.shift),
		r_buf.shift,
		val);
#else // !TSREC_WITH_64_BITS_TIMER_RG
	TSREC_LOG_DBG(
		"[%#x(+%#x)]:(%#x => %#x), mask:%#x (%u)(SET:1/CLR:0/OVW:2), latch time = [%#x(+%#x)|%#x(+%#x)]:%llu\n",
		TSREC_ADDR_BY_OFFSET(w_buf.shift),
		w_buf.shift,
		w_buf.before,
		w_buf.after,
		w_buf.mask,
		w_buf.op,
		TSREC_ADDR_BY_OFFSET(r_buf_M.shift),
		r_buf_M.shift,
		TSREC_ADDR_BY_OFFSET(r_buf.shift),
		r_buf.shift,
		val);
#endif // !TSREC_WITH_64_BITS_TIMER_RG
#endif // !REDUCE_TSREC_LOG_IN_ISR_FUNC


	return val;
}


void mtk_cam_seninf_s_tsrec_intr_wclr_en(const unsigned int wclr_en)
{
	struct tsrec_w_buffer w_buf = {0};
	union REG_TSREC_INT_EN reg = {0};

	/* setup intr write clr cfg */
	reg.bits.TSREC_INT_WCLR_EN = 1;
	tsrec_intr_write_clr = wclr_en;

	/* prepare for write register */
	w_buf.shift = TSREC_INT_EN_OFFSET;
	w_buf.mask = (wclr_en) ? (reg.val) : ~(reg.val);
	w_buf.op = (wclr_en) ? 1 : 0;
	tsrec_write_reg(&w_buf, __func__);

	/* sync reg ctrl info to tsrec_status */
	notify_tsrec_update_intr_en(w_buf.after);

	TSREC_LOG_DBG(
		"[%#x(+%#x)]:(%#x => %#x), mask:%#x (%u)(SET:1/CLR:0/OVW:2)   [wclr_en:%u]\n",
		TSREC_ADDR_BY_OFFSET(w_buf.shift),
		w_buf.shift,
		w_buf.before,
		w_buf.after,
		w_buf.mask,
		w_buf.op,
		wclr_en);
}


unsigned int mtk_cam_seninf_g_tsrec_intr_en(void)
{
	struct tsrec_r_buffer r_buf = {0};

	/* prepare for read register */
	r_buf.shift = TSREC_INT_EN_OFFSET;
	tsrec_read_reg(&r_buf, __func__);

	/* sync reg ctrl info to tsrec_status */
	notify_tsrec_update_intr_en(r_buf.val);

	TSREC_LOG_DBG(
		"[%#x(+%#x)]:(%#x)\n",
		TSREC_ADDR_BY_OFFSET(r_buf.shift),
		r_buf.shift,
		r_buf.val);

	return r_buf.val;
}


void mtk_cam_seninf_s_tsrec_intr_en(const unsigned int tsrec_n,
	const unsigned int exp0, const unsigned int exp1, const unsigned int exp2,
	const unsigned int trig_src, const unsigned int en)
{
	struct tsrec_w_buffer w_buf = {0};
	union REG_TSREC_INT_EN reg = {0};

	/* check case / error handling */
	if (unlikely(!chk_tsrec_no_valid(tsrec_n, __func__))) {
		TSREC_LOG_INF(
			"ERROR: get non-valid tsrec_n, force return   [tsrec_n:%u, exp_en(%u/%u/%u), trig_src:%u, en:%u]\n",
			tsrec_n,
			exp0, exp1, exp2,
			trig_src,
			en);
		return;
	}

	/* this RG control tsrec a~d */
	if (tsrec_n >= 4) {
		TSREC_LOG_INF(
			"ERROR: get wrong tsrec_n (this RG ctrl tsrec 0~3), force return   [tsrec_n:%u, exp_en(%u/%u/%u), trig_src:%u, en:%u]\n",
			tsrec_n,
			exp0, exp1, exp2,
			trig_src,
			en);
		return;
	}

	/* INTR ctrl for tsrec a~d (0~3) */
	switch (tsrec_n) {
	case 0:
		/* trig_src: 1 => 1st-Hsync / 0 => Vsync */
		if (trig_src) {
			reg.bits.TSREC_A_EXP0_HSYNC_INT_EN = (exp0) ? 1 : 0;
			reg.bits.TSREC_A_EXP1_HSYNC_INT_EN = (exp1) ? 1 : 0;
			reg.bits.TSREC_A_EXP2_HSYNC_INT_EN = (exp2) ? 1 : 0;
		} else {
			reg.bits.TSREC_A_EXP0_VSYNC_INT_EN = (exp0) ? 1 : 0;
			reg.bits.TSREC_A_EXP1_VSYNC_INT_EN = (exp1) ? 1 : 0;
			reg.bits.TSREC_A_EXP2_VSYNC_INT_EN = (exp2) ? 1 : 0;
		}
		break;
	case 1:
		/* trig_src: 1 => 1st-Hsync / 0 => Vsync */
		if (trig_src) {
			reg.bits.TSREC_B_EXP0_HSYNC_INT_EN = (exp0) ? 1 : 0;
			reg.bits.TSREC_B_EXP1_HSYNC_INT_EN = (exp1) ? 1 : 0;
			reg.bits.TSREC_B_EXP2_HSYNC_INT_EN = (exp2) ? 1 : 0;
		} else {
			reg.bits.TSREC_B_EXP0_VSYNC_INT_EN = (exp0) ? 1 : 0;
			reg.bits.TSREC_B_EXP1_VSYNC_INT_EN = (exp1) ? 1 : 0;
			reg.bits.TSREC_B_EXP2_VSYNC_INT_EN = (exp2) ? 1 : 0;
		}
		break;
	case 2:
		/* trig_src: 1 => 1st-Hsync / 0 => Vsync */
		if (trig_src) {
			reg.bits.TSREC_C_EXP0_HSYNC_INT_EN = (exp0) ? 1 : 0;
			reg.bits.TSREC_C_EXP1_HSYNC_INT_EN = (exp1) ? 1 : 0;
			reg.bits.TSREC_C_EXP2_HSYNC_INT_EN = (exp2) ? 1 : 0;
		} else {
			reg.bits.TSREC_C_EXP0_VSYNC_INT_EN = (exp0) ? 1 : 0;
			reg.bits.TSREC_C_EXP1_VSYNC_INT_EN = (exp1) ? 1 : 0;
			reg.bits.TSREC_C_EXP2_VSYNC_INT_EN = (exp2) ? 1 : 0;
		}
		break;
	case 3:
		/* trig_src: 1 => 1st-Hsync / 0 => Vsync */
		if (trig_src) {
			reg.bits.TSREC_D_EXP0_HSYNC_INT_EN = (exp0) ? 1 : 0;
			reg.bits.TSREC_D_EXP1_HSYNC_INT_EN = (exp1) ? 1 : 0;
			reg.bits.TSREC_D_EXP2_HSYNC_INT_EN = (exp2) ? 1 : 0;
		} else {
			reg.bits.TSREC_D_EXP0_VSYNC_INT_EN = (exp0) ? 1 : 0;
			reg.bits.TSREC_D_EXP1_VSYNC_INT_EN = (exp1) ? 1 : 0;
			reg.bits.TSREC_D_EXP2_VSYNC_INT_EN = (exp2) ? 1 : 0;
		}
		break;
	default:
		/* should be detected at before case handling */
		break;
	}

	/* prepare for write register */
	w_buf.shift = TSREC_INT_EN_OFFSET;
	w_buf.mask = (en) ? (reg.val) : ~(reg.val);
	w_buf.op = (en) ? 1 : 0;

	TSREC_SPIN_LOCK(&tsrec_intr_en_concurrency_lock);

	tsrec_write_reg(&w_buf, __func__);

	/* sync reg ctrl info to tsrec_status */
	notify_tsrec_update_intr_en(w_buf.after);

	TSREC_SPIN_UNLOCK(&tsrec_intr_en_concurrency_lock);

	TSREC_LOG_DBG(
		"[%#x(+%#x)]:(%#x => %#x), mask:%#x (%u)(SET:1/CLR:0/OVW:2)   [tsrec_n:%u, exp_en(%u/%u/%u), trig_src:%u, en:%u]\n",
		TSREC_ADDR_BY_OFFSET(w_buf.shift),
		w_buf.shift,
		w_buf.before,
		w_buf.after,
		w_buf.mask,
		w_buf.op,
		tsrec_n,
		exp0, exp1, exp2,
		trig_src,
		en);
}


unsigned int mtk_cam_seninf_g_tsrec_intr_status(void)
{
	struct tsrec_r_buffer r_buf = {0};

	/* prepare for read register */
	r_buf.shift = TSREC_INT_STATUS_OFFSET;
	tsrec_read_reg(&r_buf, __func__);

	/* sync reg ctrl info to tsrec_status */
	notify_tsrec_update_intr_status(r_buf.val);

#if !defined(REDUCE_TSREC_LOG_IN_ISR_FUNC)
	TSREC_LOG_DBG(
		"[%#x(+%#x)]:(%#x)\n",
		TSREC_ADDR_BY_OFFSET(r_buf.shift),
		r_buf.shift,
		r_buf.val);
#endif

	return r_buf.val;
}


void mtk_cam_seninf_clr_tsrec_intr_status(const unsigned int mask)
{
	struct tsrec_w_buffer w_buf = {0};

	/* case handling for not using intr write clr */
	if (!tsrec_intr_write_clr) {
		/* sync reg ctrl info to tsrec_status */
		notify_tsrec_update_intr_status(0);

#if !defined(REDUCE_TSREC_LOG_IN_ISR_FUNC)
		TSREC_LOG_DBG(
			"NOTICE: tsrec_intr_write_clr:%u, call notify tsrec update intr status to 0 for dbg and sync reg value, skip   [mask:%#x]\n",
			tsrec_intr_write_clr,
			mask);
#endif

		return;
	}

	/* prepare for write register */
	w_buf.shift = TSREC_INT_STATUS_OFFSET;
	w_buf.mask = mask;
	w_buf.op = 2; // overwrite
	tsrec_write_reg(&w_buf, __func__);

	/* sync reg ctrl info to tsrec_status */
	notify_tsrec_update_intr_status(w_buf.after);

#if !defined(REDUCE_TSREC_LOG_IN_ISR_FUNC)
	TSREC_LOG_DBG(
		"[%#x(+%#x)]:(%#x => %#x), mask:%#x (%u)(SET:1/CLR:0/OVW:2)   [mask:%u]\n",
		TSREC_ADDR_BY_OFFSET(w_buf.shift),
		w_buf.shift,
		w_buf.before,
		w_buf.after,
		w_buf.mask,
		w_buf.op,
		mask);
#endif
}


unsigned int mtk_cam_seninf_g_tsrec_intr_en_2(void)
{
	struct tsrec_r_buffer r_buf = {0};

	/* prepare for read register */
	r_buf.shift = TSREC_INT_EN_2_OFFSET;
	tsrec_read_reg(&r_buf, __func__);

	/* sync reg ctrl info to tsrec_status */
	notify_tsrec_update_intr_en_2(r_buf.val);

	TSREC_LOG_DBG(
		"[%#x(+%#x)]:(%#x)\n",
		TSREC_ADDR_BY_OFFSET(r_buf.shift),
		r_buf.shift,
		r_buf.val);

	return r_buf.val;
}


void mtk_cam_seninf_s_tsrec_intr_en_2(const unsigned int tsrec_n,
	const unsigned int exp0, const unsigned int exp1, const unsigned int exp2,
	const unsigned int trig_src, const unsigned int en)
{
	struct tsrec_w_buffer w_buf = {0};
	union REG_TSREC_INT_EN_2 reg = {0};

	/* check case / error handling */
	if (unlikely(!chk_tsrec_no_valid(tsrec_n, __func__))) {
		TSREC_LOG_INF(
			"ERROR: get non-valid tsrec_n, force return   [tsrec_n:%u, exp_en(%u/%u/%u), trig_src:%u, en:%u]\n",
			tsrec_n,
			exp0, exp1, exp2,
			trig_src,
			en);
		return;
	}

	/* this RG control tsrec e~f */
	if (tsrec_n <= 3) {
		TSREC_LOG_INF(
			"ERROR: get wrong tsrec_n (this RG ctrl tsrec 4~5), force return   [tsrec_n:%u, exp_en(%u/%u/%u), trig_src:%u, en:%u]\n",
			tsrec_n,
			exp0, exp1, exp2,
			trig_src,
			en);
		return;
	}

	/* INTR ctrl for tsrec e~f (4~5) */
	switch (tsrec_n) {
	case 4:
		/* trig_src: 1 => 1st-Hsync / 0 => Vsync */
		if (trig_src) {
			reg.bits.TSREC_E_EXP0_HSYNC_INT_EN = (exp0) ? 1 : 0;
			reg.bits.TSREC_E_EXP1_HSYNC_INT_EN = (exp1) ? 1 : 0;
			reg.bits.TSREC_E_EXP2_HSYNC_INT_EN = (exp2) ? 1 : 0;
		} else {
			reg.bits.TSREC_E_EXP0_VSYNC_INT_EN = (exp0) ? 1 : 0;
			reg.bits.TSREC_E_EXP1_VSYNC_INT_EN = (exp1) ? 1 : 0;
			reg.bits.TSREC_E_EXP2_VSYNC_INT_EN = (exp2) ? 1 : 0;
		}
		break;
	case 5:
		/* trig_src: 1 => 1st-Hsync / 0 => Vsync */
		if (trig_src) {
			reg.bits.TSREC_F_EXP0_HSYNC_INT_EN = (exp0) ? 1 : 0;
			reg.bits.TSREC_F_EXP1_HSYNC_INT_EN = (exp1) ? 1 : 0;
			reg.bits.TSREC_F_EXP2_HSYNC_INT_EN = (exp2) ? 1 : 0;
		} else {
			reg.bits.TSREC_F_EXP0_VSYNC_INT_EN = (exp0) ? 1 : 0;
			reg.bits.TSREC_F_EXP1_VSYNC_INT_EN = (exp1) ? 1 : 0;
			reg.bits.TSREC_F_EXP2_VSYNC_INT_EN = (exp2) ? 1 : 0;
		}
		break;
	default:
		/* should be detected at before case handling */
		break;
	}

	/* prepare for write register */
	w_buf.shift = TSREC_INT_EN_2_OFFSET;
	w_buf.mask = (en) ? (reg.val) : ~(reg.val);
	w_buf.op = (en) ? 1 : 0;

	TSREC_SPIN_LOCK(&tsrec_intr_en_2_concurrency_lock);

	tsrec_write_reg(&w_buf, __func__);

	/* sync reg ctrl info to tsrec_status */
	notify_tsrec_update_intr_en_2(w_buf.after);

	TSREC_SPIN_UNLOCK(&tsrec_intr_en_2_concurrency_lock);

	TSREC_LOG_DBG(
		"[%#x(+%#x)]:(%#x => %#x), mask:%#x (%u)(SET:1/CLR:0/OVW:2)   [tsrec_n:%u, exp_en(%u/%u/%u), trig_src:%u, en:%u]\n",
		TSREC_ADDR_BY_OFFSET(w_buf.shift),
		w_buf.shift,
		w_buf.before,
		w_buf.after,
		w_buf.mask,
		w_buf.op,
		tsrec_n,
		exp0, exp1, exp2,
		trig_src,
		en);
}


unsigned int mtk_cam_seninf_g_tsrec_intr_status_2(void)
{
	struct tsrec_r_buffer r_buf = {0};

	/* prepare for read register */
	r_buf.shift = TSREC_INT_STATUS_2_OFFSET;
	tsrec_read_reg(&r_buf, __func__);

	/* sync reg ctrl info to tsrec_status */
	notify_tsrec_update_intr_status_2(r_buf.val);

#if !defined(REDUCE_TSREC_LOG_IN_ISR_FUNC)
	TSREC_LOG_DBG(
		"[%#x(+%#x)]:(%#x)\n",
		TSREC_ADDR_BY_OFFSET(r_buf.shift),
		r_buf.shift,
		r_buf.val);
#endif

	return r_buf.val;
}


void mtk_cam_seninf_clr_tsrec_intr_status_2(const unsigned int mask)
{
	struct tsrec_w_buffer w_buf = {0};

	/* case handling for not using intr write clr */
	if (!tsrec_intr_write_clr) {
		/* sync reg ctrl info to tsrec_status */
		notify_tsrec_update_intr_status_2(0);

#if !defined(REDUCE_TSREC_LOG_IN_ISR_FUNC)
		TSREC_LOG_DBG(
			"NOTICE: tsrec_intr_write_clr:%u, call notify tsrec update intr status 2 to 0 for dbg and sync reg value, skip   [mask:%#x]\n",
			tsrec_intr_write_clr,
			mask);
#endif

		return;
	}

	/* prepare for write register */
	w_buf.shift = TSREC_INT_STATUS_2_OFFSET;
	w_buf.mask = mask;
	w_buf.op = 2; // overwrite
	tsrec_write_reg(&w_buf, __func__);

	/* sync reg ctrl info to tsrec_status */
	notify_tsrec_update_intr_status_2(w_buf.after);

#if !defined(REDUCE_TSREC_LOG_IN_ISR_FUNC)
	TSREC_LOG_DBG(
		"[%#x(+%#x)]:(%#x => %#x), mask:%#x (%u)(SET:1/CLR:0/OVW:2)   [mask:%u]\n",
		TSREC_ADDR_BY_OFFSET(w_buf.shift),
		w_buf.shift,
		w_buf.before,
		w_buf.after,
		w_buf.mask,
		w_buf.op,
		mask);
#endif
}


/*---------------------------------------------------------------------------*/
void mtk_cam_seninf_s_tsrec_n_cfg(const unsigned int tsrec_n,
	const int clr_exp_cnt_n)
{
	struct tsrec_w_buffer w_buf = {0};

	/* check case / error handling */
	if (unlikely(!chk_tsrec_no_valid(tsrec_n, __func__)
		|| clr_exp_cnt_n >= TSREC_EXP_MAX_CNT)) {

		TSREC_LOG_INF(
			"ERROR: non-valid input, force return   [tsrec_n:%u, clr_exp_cnt_n:%d(<%u)]\n",
			tsrec_n,
			clr_exp_cnt_n,
			TSREC_EXP_MAX_CNT);
		return;
	}

	/* prepare for write register */
	w_buf.shift = TSREC_CFG_OFFSET(tsrec_n);
	w_buf.mask = (clr_exp_cnt_n < 0)
		? TSREC_BIT_MASK(TSREC_EXP_MAX_CNT)
		: tsrec_get_mask((unsigned int)clr_exp_cnt_n, 1);
	w_buf.op = 1; // set
	tsrec_write_reg(&w_buf, __func__);

	TSREC_LOG_DBG(
		"[%#x(+%#x)]:(%#x => %#x), mask:%#x (%u)(SET:1/CLR:0/OVW:2)   [tsrec_n:%u, clr_exp_cnt_n:%d]\n",
		TSREC_ADDR_BY_OFFSET(w_buf.shift),
		w_buf.shift,
		w_buf.before,
		w_buf.after,
		w_buf.mask,
		w_buf.op,
		tsrec_n,
		clr_exp_cnt_n);
}


void mtk_cam_seninf_s_tsrec_sw_rst(const unsigned int tsrec_n,
	const unsigned int en)
{
	struct tsrec_w_buffer w_buf = {0};

	/* check case / error handling */
	if (unlikely(!chk_tsrec_no_valid(tsrec_n, __func__))) {
		TSREC_LOG_INF(
			"ERROR: get non-valid tsrec_n, force return   [tsrec_n:%u, en:%u]\n",
			tsrec_n,
			en);
		return;
	}

	/* prepare for write register */
	w_buf.shift = TSREC_SW_RST_OFFSET(tsrec_n);
	w_buf.mask = (en) ? 1 : 0;
	w_buf.op = (en) ? 1 : 0;
	tsrec_write_reg(&w_buf, __func__);

	TSREC_LOG_DBG(
		"[%#x(+%#x)]:(%#x => %#x), mask:%#x (%u)(SET:1/CLR:0/OVW:2)   [tsrec_n:%u, en:%u]\n",
		TSREC_ADDR_BY_OFFSET(w_buf.shift),
		w_buf.shift,
		w_buf.before,
		w_buf.after,
		w_buf.mask,
		w_buf.op,
		tsrec_n,
		en);
}


unsigned int mtk_cam_seninf_g_tsrec_ts_cnt(const unsigned int tsrec_n)
{
	struct tsrec_r_buffer r_buf = {0};
	union REG_TSREC_N_TS_CNT reg = {0};

	/* check case / error handling */
	if (unlikely(!chk_tsrec_no_valid(tsrec_n, __func__))) {
		TSREC_LOG_INF(
			"ERROR: get non-valid tsrec_n, force return   [tsrec_n:%u]\n",
			tsrec_n);
		return 0;
	}

	/* prepare for read register */
	r_buf.shift = TSREC_TS_CNT_OFFSET(tsrec_n);
	tsrec_read_reg(&r_buf, __func__);

	reg.val = r_buf.val;

#if !defined(REDUCE_TSREC_LOG_IN_ISR_FUNC)
	TSREC_LOG_DBG(
		"[%#x(+%#x)]:(%#x) => EXP_TS_CNT(%u/%u/%u)   [tsrec_n:%u]\n",
		TSREC_ADDR_BY_OFFSET(r_buf.shift),
		r_buf.shift,
		r_buf.val,
		reg.bits.TSREC_N_EXP0_TS_CNT,
		reg.bits.TSREC_N_EXP1_TS_CNT,
		reg.bits.TSREC_N_EXP2_TS_CNT,
		tsrec_n);
#endif

	return r_buf.val;
}


unsigned int mtk_cam_seninf_g_tsrec_ts_cnt_by_exp_n(const unsigned int reg_val,
	const unsigned int exp_n)
{
	union REG_TSREC_N_TS_CNT reg = {0};

	reg.val = reg_val;

	if (exp_n == 0)
		return reg.bits.TSREC_N_EXP0_TS_CNT;
	else if (exp_n == 1)
		return reg.bits.TSREC_N_EXP1_TS_CNT;
	else if (exp_n == 2)
		return reg.bits.TSREC_N_EXP2_TS_CNT;

	TSREC_LOG_INF(
		"ERROR: get non-valid exp_n:%u (max:%u), chk regs define, return exp0:%u\n",
		exp_n,
		TSREC_EXP_MAX_CNT,
		reg.bits.TSREC_N_EXP0_TS_CNT);

	return reg.bits.TSREC_N_EXP0_TS_CNT;
}


void mtk_cam_seninf_s_tsrec_trig_src(const unsigned int tsrec_n,
	const int exp_n, const unsigned int src_type)
{
	struct tsrec_w_buffer w_buf = {0};
	unsigned int i = 0;
	unsigned int val = 0;

	/* check case / error handling */
	if (unlikely(!chk_tsrec_no_valid(tsrec_n, __func__)
		|| exp_n >= TSREC_EXP_MAX_CNT)) {

		TSREC_LOG_INF(
			"ERROR: non-valid input, force return   [tsrec_n:%u, exp_n:%d(<%u), src_type:%u]\n",
			tsrec_n,
			exp_n,
			TSREC_EXP_MAX_CNT,
			src_type);
		return;
	}

	/* exp_n < 0 => all exp => hsync: e.g. 0x111; vsync: e.g. 0x000 = 0 */
	if (exp_n < 0 && src_type > 0) {
		for (i = 0; i < TSREC_EXP_MAX_CNT; ++i) {
			val |= tsrec_get_mask(
				(i * TSREC_EXP_TRIG_SRC_SHIFT),
				src_type);
		}
	}

	/* prepare for write register */
	w_buf.shift = TSREC_TRIG_SRC_OFFSET(tsrec_n);
	w_buf.mask = (exp_n < 0)
		? val
		: tsrec_get_mask(
			(unsigned int)(exp_n * TSREC_EXP_TRIG_SRC_SHIFT),
			src_type);
	w_buf.op = src_type;
	tsrec_write_reg(&w_buf, __func__);

	/* sync reg ctrl info to tsrec_status */
	notify_tsrec_update_tsrec_n_trig_src(tsrec_n, w_buf.after);

	TSREC_LOG_DBG(
		"[%#x(+%#x)]:(%#x => %#x), mask:%#x (%u)(SET:1/CLR:0/OVW:2)   [tsrec_n:%u, exp_n:%d, src_type:%u]\n",
		TSREC_ADDR_BY_OFFSET(w_buf.shift),
		w_buf.shift,
		w_buf.before,
		w_buf.after,
		w_buf.mask,
		w_buf.op,
		tsrec_n,
		exp_n,
		src_type);
}


void mtk_cam_seninf_s_tsrec_exp_vc_dt(const unsigned int tsrec_n,
	const unsigned int exp_n,
	const unsigned int vsync_vc,
	const unsigned int hsync_vc, const unsigned int hsync_dt)
{
	struct tsrec_w_buffer w_buf = {0};
	union REG_TSREC_N_EXP_VC_DT reg = {0};

	/* check case / error handling */
	if (unlikely(!chk_tsrec_no_valid(tsrec_n, __func__)
		|| exp_n >= TSREC_EXP_MAX_CNT)) {

		TSREC_LOG_INF(
			"ERROR: non-valid input, force return   [tsrec_n:%u, exp_n:%u(<%u), v_vc:%u, h_vc:%u, h_dt:%u]\n",
			tsrec_n,
			exp_n,
			TSREC_EXP_MAX_CNT,
			vsync_vc,
			hsync_vc,
			hsync_dt);
		return;
	}

	/* config reg value */
	reg.bits.TSREC_N_EXP_VSYNC_VC =
		vsync_vc & TSREC_BIT_MASK(TSREC_N_EXP_VSYNC_VC_BIT_WIDTH);
	reg.bits.TSREC_N_EXP_HSYNC_VC =
		hsync_vc & TSREC_BIT_MASK(TSREC_N_EXP_HSYNC_VC_BIT_WIDTH);
	reg.bits.TSREC_N_EXP_HSYNC_DT =
		hsync_dt & TSREC_BIT_MASK(TSREC_N_EXP_HSYNC_DT_BIT_WIDTH);

	/* prepare for write register */
	w_buf.shift = TSREC_EXP_VC_DT_OFFSET(tsrec_n, exp_n);
	w_buf.mask = reg.val;
	w_buf.op = 2; // overwrite
	tsrec_write_reg(&w_buf, __func__);

	/* sync reg ctrl info to tsrec_status */
	notify_tsrec_update_tsrec_n_exp_vc_dt(tsrec_n, exp_n, w_buf.after);

	TSREC_LOG_DBG(
		"[%#x(+%#x)]:(%#x => %#x), mask:%#x (%u)(SET:1/CLR:0/OVW:2)   [tsrec_n:%u, exp_n:%u, v_vc:%#x, h_vc:%#x, h_dt:%#x, HW bit width(v_vc:%u, h_vc:%u, h_dt:%u)]\n",
		TSREC_ADDR_BY_OFFSET(w_buf.shift),
		w_buf.shift,
		w_buf.before,
		w_buf.after,
		w_buf.mask,
		w_buf.op,
		tsrec_n,
		exp_n,
		vsync_vc,
		hsync_vc,
		hsync_dt,
		TSREC_N_EXP_VSYNC_VC_BIT_WIDTH,
		TSREC_N_EXP_HSYNC_VC_BIT_WIDTH,
		TSREC_N_EXP_HSYNC_DT_BIT_WIDTH);
}


void mtk_cam_seninf_tsrec_rst_tsrec_exp_vc_dt(const unsigned int tsrec_n,
	const unsigned int exp_n)
{
	struct tsrec_w_buffer w_buf = {0};

	/* check case / error handling */
	if (unlikely(!chk_tsrec_no_valid(tsrec_n, __func__)
		|| exp_n >= TSREC_EXP_MAX_CNT)) {

		TSREC_LOG_INF(
			"ERROR: non-valid input, force return   [tsrec_n:%u, exp_n:%u(<%u)]\n",
			tsrec_n,
			exp_n,
			TSREC_EXP_MAX_CNT);
		return;
	}

	/* prepare for write register */
	w_buf.shift = TSREC_EXP_VC_DT_OFFSET(tsrec_n, exp_n);
	w_buf.mask = 0;
	w_buf.op = 0; // clear
	tsrec_write_reg(&w_buf, __func__);

	/* sync reg ctrl info to tsrec_status */
	notify_tsrec_update_tsrec_n_exp_vc_dt(tsrec_n, exp_n, w_buf.after);

	TSREC_LOG_DBG(
		"[%#x(+%#x)]:(%#x => %#x), mask:%#x (%u)(SET:1/CLR:0/OVW:2)   [tsrec_n:%u, exp_n:%u]\n",
		TSREC_ADDR_BY_OFFSET(w_buf.shift),
		w_buf.shift,
		w_buf.before,
		w_buf.after,
		w_buf.mask,
		w_buf.op,
		tsrec_n,
		exp_n);
}


unsigned long long mtk_cam_seninf_g_tsrec_exp_cnt(const unsigned int tsrec_n,
	const unsigned int exp_n, const unsigned int cnt)
{
	struct tsrec_r_buffer r_buf = {0};
#if (TSREC_WITH_64_BITS_TIMER_RG)
	struct tsrec_r_buffer r_buf_M = {0};
#endif
	unsigned long long val = 0;

	/* check case / error handling */
	if (unlikely(!chk_tsrec_no_valid(tsrec_n, __func__)
		|| exp_n >= TSREC_EXP_MAX_CNT
		|| cnt >= TSREC_TS_REC_MAX_CNT)) {

		TSREC_LOG_INF(
			"ERROR: non-valid input, force return   [tsrec_n:%u, exp_n:%u(<%u), cnt:%u(<%u)]\n",
			tsrec_n,
			exp_n,
			TSREC_EXP_MAX_CNT,
			cnt,
			TSREC_TS_REC_MAX_CNT);
		return 0;
	}

	/* prepare for read register */
	r_buf.shift = TSREC_EXP_CNT_OFFSET(tsrec_n, exp_n, cnt);
	tsrec_read_reg(&r_buf, __func__);
	val = r_buf.val;

#if (TSREC_WITH_64_BITS_TIMER_RG)
	r_buf_M.shift = TSREC_EXP_CNT_M_OFFSET(tsrec_n, exp_n, cnt);
	tsrec_read_reg(&r_buf_M, __func__);
	val = r_buf_M.val;
	val = ((val << 32) | (r_buf.val));
#endif


#if !defined(REDUCE_TSREC_LOG_IN_ISR_FUNC)
#if !(TSREC_WITH_64_BITS_TIMER_RG)
	TSREC_LOG_DBG(
		"[%#x(+%#x)]:(%llu)   [tsrec_n:%u, exp_n:%u, cnt:%u]\n",
		TSREC_ADDR_BY_OFFSET(r_buf.shift),
		r_buf.shift,
		val,
		tsrec_n,
		exp_n,
		cnt);
#else // !TSREC_WITH_64_BITS_TIMER_RG
	TSREC_LOG_DBG(
		"[%#x(+%#x)|%#x(+%#x)]:(%llu)   [tsrec_n:%u, exp_n:%u, cnt:%u]\n",
		TSREC_ADDR_BY_OFFSET(r_buf_M.shift),
		r_buf_M.shift,
		TSREC_ADDR_BY_OFFSET(r_buf.shift),
		r_buf.shift,
		val,
		tsrec_n,
		exp_n,
		cnt);
#endif // !TSREC_WITH_64_BITS_TIMER_RG
#endif // !REDUCE_TSREC_LOG_IN_ISR_FUNC

	return val;
}


/******************************************************************************
 * TSREC registers init function
 *****************************************************************************/
#ifndef FS_UT
void mtk_cam_seninf_tsrec_regs_iomem_init(void __iomem *p_seninf_base)
{
	tsrec_base_addr = p_seninf_base + (TSREC_BASE - SENINF_BASE);

	TSREC_LOG_INF(
		"NOTICE: tsrec_base_addr:%p (p_seninf_base:%p, TSREC_BASE:%#x/SENINF_BASE:%#x)\n",
		tsrec_base_addr,
		p_seninf_base,
		TSREC_BASE,
		SENINF_BASE);
}
#endif // !FS_UT
