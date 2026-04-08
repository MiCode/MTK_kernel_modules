// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2022 MediaTek Inc.


#define PFX "TSREC-REGS"
#define TSREC_LOG_DBG_DEF_CAT LOG_TSREC_REG


#include "mtk_cam-seninf-tsrec-regs-def.h"
#include "mtk_cam-seninf-tsrec-regs.h"
#include "mtk_cam-seninf-tsrec-def.h"
#include "mtk_cam-seninf-tsrec.h"
#include "mtk_cam-seninf-tsrec-utils-impl.h"


#define TSREC_MSG_LOG_STR_LEN (128)


/******************************************************************************
 * TSREC structure / define / static variables
 *****************************************************************************/
/* structure / define */
#ifndef FS_UT
/**
 * For common ctrl RG, e.g., TOP_TSREC_CFG
 * that using one register to control several tsrec hw.
 */
DEFINE_SPINLOCK(tsrec_top_cfg_concurrency_lock);
#endif


#define TSREC_HW_DBG_MAX_CNT            (32)
#define TSREC_TOP_HW_FOUND_BIT_SHIFT    (31)
struct tsrec_iomem_info_st {
	void __iomem *seninf_top_base_addr;

	unsigned int using_unify_iomem;

	unsigned int found_bits;
	unsigned int hw_max_cnt;

	unsigned int top_shift;
	unsigned int base_shift_arr[TSREC_HW_DBG_MAX_CNT];
};


/* static variables */
static unsigned int tsrec_intr_write_clr;


static unsigned int tsrec_dts_hw_cnt;
static void __iomem *tsrec_top_base_addr;
static void __iomem **tsrec_base_addr_arr;


/******************************************************************************
 * TSREC read/write register structure & variables
 *****************************************************************************/
struct tsrec_w_buffer {
	void __iomem *base_addr;
	int tsrec_no; // -1:tsrec_top, others:0 ~ N => tsrec 0 ~ N;
	unsigned int shift;
	unsigned int mask;
	unsigned int op; // overwrite:2, set:1, clear:0;
	// unsigned int ts;
	unsigned int before;
	unsigned int after;
};

struct tsrec_r_buffer {
	void __iomem *base_addr;
	int tsrec_no; // -1:tsrec_top, others:0 ~ N => tsrec 0 ~ N;
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


static inline void __iomem *g_tsrec_top_base_addr(void)
{
	return tsrec_top_base_addr;
}


static inline void __iomem *g_tsrec_no_base_addr(const unsigned int tsrec_no)
{
	if (unlikely((!tsrec_base_addr_arr) || (tsrec_no >= tsrec_dts_hw_cnt)))
		return NULL;
	return tsrec_base_addr_arr[tsrec_no];
}


static inline void tsrec_dump_w_buf(const struct tsrec_w_buffer *w_buf,
	const char *msg, const char *caller)
{
	TSREC_LOG_INF(
		"[%s] [%#x(%p(+%#x))]:(%#x => %#x), mask:%#x (%u)(SET:1/CLR:0/OVW:2)   %s\n",
		caller,
		TSREC_ADDR(w_buf->tsrec_no, w_buf->shift),
		w_buf->base_addr,
		w_buf->shift,
		w_buf->before,
		w_buf->after,
		w_buf->mask,
		w_buf->op,
		msg);
}


static inline void tsrec_dump_r_buf(const struct tsrec_r_buffer *r_buf,
	const char *msg, const char *caller)
{
	TSREC_LOG_INF(
		"[%s] [%#x(%p(+%#x))]:%#x   %s\n",
		caller,
		TSREC_ADDR(r_buf->tsrec_no, r_buf->shift),
		r_buf->base_addr,
		r_buf->shift,
		r_buf->val,
		msg);
}


static inline void tsrec_dump_r_buf_64(const struct tsrec_r_buffer *r_buf,
	const struct tsrec_r_buffer *r_buf_M,
	const unsigned long long val,
	const char *msg, const char *caller)
{
	TSREC_LOG_INF(
		"[%s] [%#x(%p(+%#x))|%#x(%p(+%#x))]:%llu(%#x|%#x)   %s\n",
		caller,
		TSREC_ADDR(r_buf_M->tsrec_no, r_buf_M->shift),
		r_buf_M->base_addr,
		r_buf_M->shift,
		TSREC_ADDR(r_buf->tsrec_no, r_buf->shift),
		r_buf->base_addr,
		r_buf->shift,
		val,
		r_buf_M->val,
		r_buf->val,
		msg);
}


static inline void tsrec_dump_timer_latch(const struct tsrec_w_buffer *w_buf,
	const struct tsrec_r_buffer *r_buf,
	const unsigned long long val,
	const char *caller)
{
	TSREC_LOG_INF(
		"[%s] [%#x(%p(+%#x))]:(%#x => %#x), mask:%#x (%u)(SET:1/CLR:0/OVW:2), latch timer = [%#x(%p(+%#x))]:%llu(%#x)\n",
		caller,
		TSREC_ADDR(w_buf->tsrec_no, w_buf->shift),
		w_buf->base_addr,
		w_buf->shift,
		w_buf->before,
		w_buf->after,
		w_buf->mask,
		w_buf->op,
		TSREC_ADDR(r_buf->tsrec_no, r_buf->shift),
		r_buf->base_addr,
		r_buf->shift,
		val,
		r_buf->val);
}


static inline void tsrec_dump_timer_latch_64(const struct tsrec_w_buffer *w_buf,
	const struct tsrec_r_buffer *r_buf, const struct tsrec_r_buffer *r_buf_M,
	const unsigned long long val,
	const char *caller)
{
	TSREC_LOG_INF(
		"[%s] [%#x(%p(+%#x))]:(%#x => %#x), mask:%#x (%u)(SET:1/CLR:0/OVW:2), latch timer = [%#x(%p(+%#x))|%#x(%p(+%#x))]:%llu(%#x|%#x)\n",
		caller,
		TSREC_ADDR(w_buf->tsrec_no, w_buf->shift),
		w_buf->base_addr,
		w_buf->shift,
		w_buf->before,
		w_buf->after,
		w_buf->mask,
		w_buf->op,
		TSREC_ADDR(r_buf_M->tsrec_no, r_buf_M->shift),
		r_buf_M->base_addr,
		r_buf_M->shift,
		TSREC_ADDR(r_buf->tsrec_no, r_buf->shift),
		r_buf->base_addr,
		r_buf->shift,
		val,
		r_buf_M->val,
		r_buf->val);
}


static inline int chk_tsrec_w_buffer_valid(const struct tsrec_w_buffer *w_buf,
	const char *caller)
{
#ifndef FS_UT
	if (unlikely(w_buf->base_addr == NULL)) {
		TSREC_LOG_INF(
			"[%s] ERROR: detect invalid params, dump buffer\n",
			caller);
		tsrec_dump_w_buf(w_buf, "", caller);
		return 0;
	}
#endif
	return 1;
}


static inline int chk_tsrec_r_buffer_valid(const struct tsrec_r_buffer *r_buf,
	const char *caller)
{
#ifndef FS_UT
	if (unlikely(r_buf->base_addr == NULL)) {
		TSREC_LOG_INF(
			"[%s] ERROR: detect invalid params, dump buffer\n",
			caller);
		tsrec_dump_r_buf(r_buf, "", caller);
		return 0;
	}
#endif
	return 1;
}


#ifndef FS_UT
static void tsrec_write_reg(struct tsrec_w_buffer *buf, const char *caller)
{
	void __iomem *__p;
	u32 __v;

	/* error handling */
	/* --- SENINF NOT power up (exclude timer cfg */
	/* --- due to unsing this as a flag) --- */
	if (unlikely(get_tsrec_timer_en_status() == 0)) {
		TSREC_LOG_INF(
			"[%s] ERROR: tsrec_timer_en_status:%u (resume/suspend will change this status), maybe touch RG at a wrong timing, return\n",
			caller, get_tsrec_timer_en_status());
		tsrec_dump_w_buf(buf, "", caller);
		return;
	}
	if (unlikely(buf->base_addr == NULL))
		return;

#if (ADDR_SHIFT_FROM_TSREC_TOP)
	__p = (tsrec_top_base_addr + buf->shift);
#else
	__p = (buf->base_addr + buf->shift);
#endif
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
	void __iomem *__p;

	/* error handling */
	/* --- SENINF NOT power up (exclude timer cfg */
	/* --- due to unsing this as a flag) --- */
	if (unlikely(get_tsrec_timer_en_status() == 0)) {
		TSREC_LOG_INF(
			"[%s] ERROR: tsrec_timer_en_status:%u (resume/suspend will change this status), maybe touch RG at a wrong timing, return\n",
			caller, get_tsrec_timer_en_status());
		tsrec_dump_r_buf(buf, "", caller);
		return;
	}
	if (unlikely(buf->base_addr == NULL))
		return;

#if (ADDR_SHIFT_FROM_TSREC_TOP)
	__p = (tsrec_top_base_addr + buf->shift);
#else
	__p = (buf->base_addr + buf->shift);
#endif
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
	w_buf.tsrec_no = -1; // tsrec_top
	w_buf.base_addr = g_tsrec_top_base_addr();
	w_buf.shift = TSREC_TOP_CFG_OFFSET;
	w_buf.mask = tsrec_get_mask(tsrec_n, clk_en);
	w_buf.op = (clk_en) ? 1 : 0;
	if (unlikely(!chk_tsrec_w_buffer_valid(&w_buf, __func__)))
		return;

	TSREC_SPIN_LOCK(&tsrec_top_cfg_concurrency_lock);

	tsrec_write_reg(&w_buf, __func__);

	/* sync reg ctrl info to tsrec_status */
	notify_tsrec_update_tsrec_n_clk_en_status(tsrec_n, clk_en);
	notify_tsrec_update_top_cfg(w_buf.after);

	TSREC_SPIN_UNLOCK(&tsrec_top_cfg_concurrency_lock);

	if (unlikely(_TSREC_LOG_ENABLED(LOG_TSREC_REG))) {
		char msg[TSREC_MSG_LOG_STR_LEN] = {0};
		int len = 0;

		TSREC_SNPRF(TSREC_MSG_LOG_STR_LEN, msg, len,
			"[tsrec_n:%u, clk_en:%u]",
			tsrec_n,
			clk_en);
		tsrec_dump_w_buf(&w_buf, msg, __func__);
	}
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
	w_buf.tsrec_no = -1; // tsrec_top
	w_buf.base_addr = g_tsrec_top_base_addr();
	w_buf.shift = TSREC_TIMER_CFG_OFFSET;
	w_buf.mask = (en) ? (reg.val) : ~(reg.val);
	w_buf.op = (en) ? 1 : 0;
	if (unlikely(!chk_tsrec_w_buffer_valid(&w_buf, __func__)))
		return;

	tsrec_write_reg(&w_buf, __func__);

	/* sync reg ctrl info to tsrec_status */
	notify_tsrec_update_timer_cfg(w_buf.after);

	if (unlikely(_TSREC_LOG_ENABLED(LOG_TSREC_REG))) {
		char msg[TSREC_MSG_LOG_STR_LEN] = {0};
		int len = 0;

		TSREC_SNPRF(TSREC_MSG_LOG_STR_LEN, msg, len,
			"[timer_en_status:%u, (w/g_timer:%u(using:%u)), en:%u]",
			notify_tsrec_get_timer_en_status(),
			TSREC_WITH_GLOBAL_TIMER,
			tsrec_using_global_timer,
			en);
		tsrec_dump_w_buf(&w_buf, msg, __func__);
	}
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
	w_buf.tsrec_no = -1; // tsrec_top
	w_buf.base_addr = g_tsrec_top_base_addr();
	w_buf.shift = TSREC_TIMER_CFG_OFFSET;
	w_buf.mask = (reg.val);
	w_buf.op = 1; // set
	if (unlikely(!chk_tsrec_w_buffer_valid(&w_buf, __func__)))
		return 0;

	tsrec_write_reg(&w_buf, __func__);

	/* prepare for read register */
	r_buf.tsrec_no = -1; // tsrec_top
	r_buf.base_addr = g_tsrec_top_base_addr();
	r_buf.shift = TSREC_TIMER_LAT_OFFSET;
	if (unlikely(!chk_tsrec_r_buffer_valid(&r_buf, __func__)))
		return 0;

	tsrec_read_reg(&r_buf, __func__);
	val = r_buf.val;

#if (TSREC_WITH_64_BITS_TIMER_RG)
	r_buf_M.tsrec_no = -1; // tsrec_top
	r_buf_M.base_addr = g_tsrec_top_base_addr();
	r_buf_M.shift = TSREC_TIMER_LAT_M_OFFSET;
	if (unlikely(!chk_tsrec_r_buffer_valid(&r_buf_M, __func__)))
		return 0;

	tsrec_read_reg(&r_buf_M, __func__);
	val = r_buf_M.val;
	val = ((val << 32) | (r_buf.val));
#endif


#if !defined(REDUCE_TSREC_LOG_IN_ISR_FUNC)
	if (unlikely(_TSREC_LOG_ENABLED(LOG_TSREC_REG))) {
#if !(TSREC_WITH_64_BITS_TIMER_RG)
		tsrec_dump_timer_latch(
			&w_buf, &r_buf, val, __func__);
#else
		tsrec_dump_timer_latch_64(
			&w_buf, &r_buf, &r_buf_M, val, __func__);
#endif
	}
#endif


	return val;
}


void mtk_cam_seninf_s_tsrec_intr_wclr_en(const unsigned int wclr_en)
{
	TSREC_LOG_DBG(
		"NOTICE: set INTR wclr_en:(%u => %u)\n",
		tsrec_intr_write_clr, wclr_en);

	tsrec_intr_write_clr = wclr_en;
}


void mtk_cam_seninf_tsrec_s_device_irq_sel(const unsigned int irq_id,
	const unsigned int val)
{
	struct tsrec_w_buffer w_buf = {0};

	/* prepare for write register */
	w_buf.tsrec_no = -1; // tsrec_top
	w_buf.base_addr = g_tsrec_top_base_addr();
	w_buf.shift = TSREC_DEVICE_IRQ_SEL_0_OFFSET + 4 * irq_id;
	w_buf.mask = val;
	w_buf.op = 2; // overwrite
	if (unlikely(!chk_tsrec_w_buffer_valid(&w_buf, __func__)))
		return;

	tsrec_write_reg(&w_buf, __func__);

	if (unlikely(_TSREC_LOG_ENABLED(LOG_TSREC_REG))) {
		char msg[TSREC_MSG_LOG_STR_LEN] = {0};
		int len = 0;

		TSREC_SNPRF(TSREC_MSG_LOG_STR_LEN, msg, len,
			"[irq_id:%u, val:%#x]",
			irq_id, val);
		tsrec_dump_w_buf(&w_buf, msg, __func__);
	}
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
	w_buf.tsrec_no = tsrec_n;
	w_buf.base_addr = g_tsrec_no_base_addr(tsrec_n);
	w_buf.shift = TSREC_CFG_OFFSET(tsrec_n);
	w_buf.mask = (clr_exp_cnt_n < 0)
		? TSREC_BIT_MASK(TSREC_EXP_MAX_CNT)
		: tsrec_get_mask((unsigned int)clr_exp_cnt_n, 1);
	w_buf.op = 1; // set
	if (unlikely(!chk_tsrec_w_buffer_valid(&w_buf, __func__)))
		return;

	tsrec_write_reg(&w_buf, __func__);

	if (unlikely(_TSREC_LOG_ENABLED(LOG_TSREC_REG))) {
		char msg[TSREC_MSG_LOG_STR_LEN] = {0};
		int len = 0;

		TSREC_SNPRF(TSREC_MSG_LOG_STR_LEN, msg, len,
			"[tsrec_n:%u, clr_exp_cnt_n:%d]",
			tsrec_n,
			clr_exp_cnt_n);
		tsrec_dump_w_buf(&w_buf, msg, __func__);
	}
}


unsigned int mtk_cam_seninf_g_tsrec_n_intr_en(const unsigned int tsrec_n)
{
	struct tsrec_r_buffer r_buf = {0};

	/* prepare for read register */
	r_buf.tsrec_no = tsrec_n;
	r_buf.base_addr = g_tsrec_no_base_addr(tsrec_n);
	r_buf.shift = TSREC_INT_EN_OFFSET(tsrec_n);
	if (unlikely(!chk_tsrec_r_buffer_valid(&r_buf, __func__)))
		return 0;

	tsrec_read_reg(&r_buf, __func__);

	/* sync reg ctrl info to tsrec_status */
	notify_tsrec_update_tsrec_n_intr_en(tsrec_n, r_buf.val);

	if (unlikely(_TSREC_LOG_ENABLED(LOG_TSREC_REG)))
		tsrec_dump_r_buf(&r_buf, "", __func__);

	return r_buf.val;
}


void mtk_cam_seninf_s_tsrec_n_intr_en(const unsigned int tsrec_n,
	const unsigned int exp0, const unsigned int exp1, const unsigned int exp2,
	const unsigned int trig_src, const unsigned int en)
{
	const unsigned int wclr_en = TSREC_INTR_W_CLR_EN;
	struct tsrec_w_buffer w_buf = {0};
	unsigned int val = 0;

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

	/* INTR ctrl */
	if (wclr_en)
		val |= (1UL << TSREC_INT_WCLR_EN_BIT);

	/* trig_src: (1 => 1st-Hsync / 0 => Vsync) */
	if (trig_src) {
		val |= (exp0) ? (1UL << (0 + TSREC_INT_EN_HSYNC_BASE_BIT)) : 0;
		val |= (exp1) ? (1UL << (1 + TSREC_INT_EN_HSYNC_BASE_BIT)) : 0;
		val |= (exp2) ? (1UL << (2 + TSREC_INT_EN_HSYNC_BASE_BIT)) : 0;
	} else {
		val |= (exp0) ? (1UL << (0 + TSREC_INT_EN_VSYNC_BASE_BIT)) : 0;
		val |= (exp1) ? (1UL << (1 + TSREC_INT_EN_VSYNC_BASE_BIT)) : 0;
		val |= (exp2) ? (1UL << (2 + TSREC_INT_EN_VSYNC_BASE_BIT)) : 0;
	}

	/* prepare for write register */
	w_buf.tsrec_no = tsrec_n;
	w_buf.base_addr = g_tsrec_no_base_addr(tsrec_n);
	w_buf.shift = TSREC_INT_EN_OFFSET(tsrec_n);
	w_buf.mask = (en) ? val : ~val;
	w_buf.op = (en) ? 1 : 0;
	if (unlikely(!chk_tsrec_w_buffer_valid(&w_buf, __func__)))
		return;

	tsrec_write_reg(&w_buf, __func__);

	/* sync reg ctrl info to tsrec_status */
	notify_tsrec_update_tsrec_n_intr_en(tsrec_n, w_buf.after);

	if (unlikely(_TSREC_LOG_ENABLED(LOG_TSREC_REG))) {
		char msg[TSREC_MSG_LOG_STR_LEN] = {0};
		int len = 0;

		TSREC_SNPRF(TSREC_MSG_LOG_STR_LEN, msg, len,
			"[tsrec_n:%u, exp_en(%u/%u/%u), trig_src:%u, en:%u]",
			tsrec_n,
			exp0, exp1, exp2,
			trig_src,
			en);
		tsrec_dump_w_buf(&w_buf, msg, __func__);
	}
}


unsigned int mtk_cam_seninf_g_tsrec_n_intr_status(const unsigned int tsrec_n)
{
	struct tsrec_r_buffer r_buf = {0};

	/* prepare for read register */
	r_buf.tsrec_no = tsrec_n;
	r_buf.base_addr = g_tsrec_no_base_addr(tsrec_n);
	r_buf.shift = TSREC_INT_STATUS_OFFSET(tsrec_n);
	if (unlikely(!chk_tsrec_r_buffer_valid(&r_buf, __func__)))
		return 0;

	tsrec_read_reg(&r_buf, __func__);

	/* sync reg ctrl info to tsrec_status */
	notify_tsrec_update_tsrec_n_intr_status(tsrec_n, r_buf.val);

#if !defined(REDUCE_TSREC_LOG_IN_ISR_FUNC)
	if (unlikely(_TSREC_LOG_ENABLED(LOG_TSREC_REG)))
		tsrec_dump_r_buf(&r_buf, "", __func__);
#endif

	return r_buf.val;
}


void mtk_cam_seninf_clr_tsrec_n_intr_status(const unsigned int tsrec_n,
	const unsigned int mask)
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
	w_buf.tsrec_no = tsrec_n;
	w_buf.base_addr = g_tsrec_no_base_addr(tsrec_n);
	w_buf.shift = TSREC_INT_STATUS_OFFSET(tsrec_n);
	w_buf.mask = mask;
	w_buf.op = 2; // overwrite
	if (unlikely(!chk_tsrec_w_buffer_valid(&w_buf, __func__)))
		return;

	tsrec_write_reg(&w_buf, __func__);

	/* sync reg ctrl info to tsrec_status */
	notify_tsrec_update_tsrec_n_intr_status(tsrec_n, w_buf.after);

#if !defined(REDUCE_TSREC_LOG_IN_ISR_FUNC)
	if (unlikely(_TSREC_LOG_ENABLED(LOG_TSREC_REG))) {
		char msg[TSREC_MSG_LOG_STR_LEN] = {0};
		int len = 0;

		TSREC_SNPRF(TSREC_MSG_LOG_STR_LEN, msg, len,
			"[mask:%#x]",
			mask);
		tsrec_dump_w_buf(&w_buf, msg, __func__);
	}
#endif
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
	w_buf.tsrec_no = tsrec_n;
	w_buf.base_addr = g_tsrec_no_base_addr(tsrec_n);
	w_buf.shift = TSREC_SW_RST_OFFSET(tsrec_n);
	w_buf.mask = (en) ? 1 : 0;
	w_buf.op = (en) ? 1 : 0;
	if (unlikely(!chk_tsrec_w_buffer_valid(&w_buf, __func__)))
		return;

	tsrec_write_reg(&w_buf, __func__);

	if (unlikely(_TSREC_LOG_ENABLED(LOG_TSREC_REG))) {
		char msg[TSREC_MSG_LOG_STR_LEN] = {0};
		int len = 0;

		TSREC_SNPRF(TSREC_MSG_LOG_STR_LEN, msg, len,
			"[tsrec_n:%u, en:%u]",
			tsrec_n,
			en);
		tsrec_dump_w_buf(&w_buf, msg, __func__);
	}
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
	r_buf.tsrec_no = tsrec_n;
	r_buf.base_addr = g_tsrec_no_base_addr(tsrec_n);
	r_buf.shift = TSREC_TS_CNT_OFFSET(tsrec_n);
	if (unlikely(!chk_tsrec_r_buffer_valid(&r_buf, __func__)))
		return 0;

	tsrec_read_reg(&r_buf, __func__);
	reg.val = r_buf.val;

#if !defined(REDUCE_TSREC_LOG_IN_ISR_FUNC)
	if (unlikely(_TSREC_LOG_ENABLED(LOG_TSREC_REG))) {
		char msg[TSREC_MSG_LOG_STR_LEN] = {0};
		int len = 0;

		TSREC_SNPRF(TSREC_MSG_LOG_STR_LEN, msg, len,
			"[EXP_TS_CNT(%u/%u/%u), tsrec_n:%u]",
			reg.bits.TSREC_N_EXP0_TS_CNT,
			reg.bits.TSREC_N_EXP1_TS_CNT,
			reg.bits.TSREC_N_EXP2_TS_CNT,
			tsrec_n);
		tsrec_dump_r_buf(&r_buf, msg, __func__);
	}
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
	w_buf.tsrec_no = tsrec_n;
	w_buf.base_addr = g_tsrec_no_base_addr(tsrec_n);
	w_buf.shift = TSREC_TRIG_SRC_OFFSET(tsrec_n);
	w_buf.mask = (exp_n < 0)
		? val
		: tsrec_get_mask(
			(unsigned int)(exp_n * TSREC_EXP_TRIG_SRC_SHIFT),
			src_type);
	w_buf.op = src_type;
	if (unlikely(!chk_tsrec_w_buffer_valid(&w_buf, __func__)))
		return;

	tsrec_write_reg(&w_buf, __func__);

	/* sync reg ctrl info to tsrec_status */
	notify_tsrec_update_tsrec_n_trig_src(tsrec_n, w_buf.after);

	if (unlikely(_TSREC_LOG_ENABLED(LOG_TSREC_REG))) {
		char msg[TSREC_MSG_LOG_STR_LEN] = {0};
		int len = 0;

		TSREC_SNPRF(TSREC_MSG_LOG_STR_LEN, msg, len,
			"[tsrec_n:%u, exp_n:%d, src_type:%u]",
			tsrec_n,
			exp_n,
			src_type);
		tsrec_dump_w_buf(&w_buf, msg, __func__);
	}
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
	w_buf.tsrec_no = tsrec_n;
	w_buf.base_addr = g_tsrec_no_base_addr(tsrec_n);
	w_buf.shift = TSREC_EXP_VC_DT_OFFSET(tsrec_n, exp_n);
	w_buf.mask = reg.val;
	w_buf.op = 2; // overwrite
	if (unlikely(!chk_tsrec_w_buffer_valid(&w_buf, __func__)))
		return;

	tsrec_write_reg(&w_buf, __func__);

	/* sync reg ctrl info to tsrec_status */
	notify_tsrec_update_tsrec_n_exp_vc_dt(tsrec_n, exp_n, w_buf.after);

	if (unlikely(_TSREC_LOG_ENABLED(LOG_TSREC_REG))) {
		char msg[TSREC_MSG_LOG_STR_LEN] = {0};
		int len = 0;

		TSREC_SNPRF(TSREC_MSG_LOG_STR_LEN, msg, len,
			"[tsrec_n:%u, exp_n:%u, v_vc:%#x, h_vc:%#x, h_dt:%#x, HW bit width(v_vc:%u, h_vc:%u, h_dt:%u)]",
			tsrec_n,
			exp_n,
			vsync_vc,
			hsync_vc,
			hsync_dt,
			TSREC_N_EXP_VSYNC_VC_BIT_WIDTH,
			TSREC_N_EXP_HSYNC_VC_BIT_WIDTH,
			TSREC_N_EXP_HSYNC_DT_BIT_WIDTH);
		tsrec_dump_w_buf(&w_buf, msg, __func__);
	}
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
	w_buf.tsrec_no = tsrec_n;
	w_buf.base_addr = g_tsrec_no_base_addr(tsrec_n);
	w_buf.shift = TSREC_EXP_VC_DT_OFFSET(tsrec_n, exp_n);
	w_buf.mask = 0;
	w_buf.op = 0; // clear
	if (unlikely(!chk_tsrec_w_buffer_valid(&w_buf, __func__)))
		return;

	tsrec_write_reg(&w_buf, __func__);

	/* sync reg ctrl info to tsrec_status */
	notify_tsrec_update_tsrec_n_exp_vc_dt(tsrec_n, exp_n, w_buf.after);

	if (unlikely(_TSREC_LOG_ENABLED(LOG_TSREC_REG))) {
		char msg[TSREC_MSG_LOG_STR_LEN] = {0};
		int len = 0;

		TSREC_SNPRF(TSREC_MSG_LOG_STR_LEN, msg, len,
			"[tsrec_n:%u, exp_n:%u]",
			tsrec_n,
			exp_n);
		tsrec_dump_w_buf(&w_buf, msg, __func__);
	}
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
	r_buf.tsrec_no = tsrec_n;
	r_buf.base_addr = g_tsrec_no_base_addr(tsrec_n);
	r_buf.shift = TSREC_EXP_CNT_OFFSET(tsrec_n, exp_n, cnt);
	if (unlikely(!chk_tsrec_r_buffer_valid(&r_buf, __func__)))
		return 0;

	tsrec_read_reg(&r_buf, __func__);
	val = r_buf.val;

#if (TSREC_WITH_64_BITS_TIMER_RG)
	r_buf_M.shift = TSREC_EXP_CNT_M_OFFSET(tsrec_n, exp_n, cnt);
	tsrec_read_reg(&r_buf_M, __func__);
	val = r_buf_M.val;
	val = ((val << 32) | (r_buf.val));
#endif


#if !defined(REDUCE_TSREC_LOG_IN_ISR_FUNC)
	if (unlikely(_TSREC_LOG_ENABLED(LOG_TSREC_REG))) {
		char msg[TSREC_MSG_LOG_STR_LEN] = {0};
		int len = 0;

		TSREC_SNPRF(TSREC_MSG_LOG_STR_LEN, msg, len,
			"[tsrec_n:%u, exp_n:%u, cnt:%u]",
			tsrec_n,
			exp_n,
			cnt);

#if !(TSREC_WITH_64_BITS_TIMER_RG)
		tsrec_dump_r_buf(&r_buf, msg, __func__);
#else
		tsrec_dump_r_buf_64(&r_buf, &r_buf_M, val, msg, __func__);
#endif
	}
#endif

	return val;
}


/******************************************************************************
 * TSREC registers init function
 *****************************************************************************/
static void tsrec_regs_dbg_dump_iomem_info(
	const struct tsrec_iomem_info_st *p_iomem_info)
{
	const unsigned int tsrec_hw_cnt = p_iomem_info->hw_max_cnt;
	const unsigned int log_str_len = TSREC_LOG_BUF_STR_LEN;
	unsigned int i;
	int len = 0, ret;
	char *log_buf = NULL;

	ret = alloc_log_buf(log_str_len, &log_buf);
	if (unlikely(ret != 0)) {
		TSREC_LOG_INF("ERROR: log_buf allocate memory failed\n");
		return;
	}

	/* show log info format msg */
	TSREC_SNPRF(log_str_len, log_buf, len,
		"iomem(unify_mapping:%u):(tsrec_top:%p(seninf_top:%p)",
		p_iomem_info->using_unify_iomem,
		tsrec_top_base_addr,
		p_iomem_info->seninf_top_base_addr);

	TSREC_SNPRF(log_str_len, log_buf, len,
		"(+%#x)",
		p_iomem_info->top_shift);

	TSREC_SNPRF(log_str_len, log_buf, len,
		", hw_cnt(%u/%#x):(",
		tsrec_hw_cnt,
		p_iomem_info->found_bits);

	if (likely(tsrec_base_addr_arr != NULL)) {
		for (i = 0; i < tsrec_hw_cnt; ++i) {
			TSREC_SNPRF(log_str_len, log_buf, len,
				"[%u]:", i);
#ifdef TSREC_DBG_PRINT_IOMEM_ADDR
			TSREC_SNPRF(log_str_len, log_buf, len,
				"%p", tsrec_base_addr_arr[i]);
#endif
			TSREC_SNPRF(log_str_len, log_buf, len,
				"(+%#x)/",
				p_iomem_info->base_shift_arr[i]);
		}
	}
	TSREC_SNPRF(log_str_len, log_buf, len, ")");

	TSREC_LOG_INF("NOTICE: %s\n", log_buf);
	TSREC_KFREE(log_buf);
}


#ifndef FS_UT
static inline int tsrec_setup_iomem_addr_by_base_shift(void __iomem *base_addr,
	const unsigned int base_shift, void __iomem **p_tsrec_addr)
{
	if (unlikely(base_addr == NULL))
		return 1;
	*p_tsrec_addr = base_addr + base_shift;
	return 0;
}


static inline int tsrec_setup_iomem_addr_by_ioremap(
	struct device_node *p_dev_node, void __iomem **p_tsrec_addr)
{
	int index;

	index = of_property_match_string(p_dev_node, "reg-names", "base");
	if (unlikely(index < 0)) {
		TSREC_LOG_INF(
			"ERROR: call of_property_match_string error:(%d), return\n",
			index);
		return 1;
	}
	*p_tsrec_addr = devm_of_iomap(seninf_dev, p_dev_node, index, NULL);
	return 0;
}
#endif


static inline void tsrec_top_regs_iomem_uninit(void)
{
	tsrec_top_base_addr = NULL;
}


static void tsrec_top_regs_iomem_init(struct tsrec_iomem_info_st *p_iomem_info)
{
#ifndef FS_UT
	struct device_node *p_dev_node;
	unsigned int base_shift = 0;
	int ret;

	/* 1. find tsrec_top device node */
	p_dev_node = tsrec_utils_of_find_comp_node(
		seninf_dev->of_node, TSREC_TOP_COMP_NAME, __func__, 0);
	if (unlikely(p_dev_node == NULL))
		return;

	/* 2. get the "base-shift" from the "seninf-tsrec-top" node */
	ret = tsrec_utils_of_prop_r_u32(p_dev_node, "base-shift", &base_shift, __func__, 1);
	if (unlikely(ret != 0)) {
		tsrec_top_base_addr = NULL;
		return;
	}
	p_iomem_info->top_shift = base_shift;

	/* 3. check and setup iomemm info */
	if (p_iomem_info->using_unify_iomem) {
		ret = tsrec_setup_iomem_addr_by_base_shift(
			p_iomem_info->seninf_top_base_addr, base_shift, &tsrec_top_base_addr);
		if (unlikely(ret != 0)) {
			TSREC_LOG_INF(
				"ERROR: seninf_top_base_addr:%p is invalid, not add base-shift (%#x), return\n",
				p_iomem_info->seninf_top_base_addr, base_shift);
			return;
		}
	} else {
		ret = tsrec_setup_iomem_addr_by_ioremap(p_dev_node, &tsrec_top_base_addr);
		if (unlikely(ret != 0)) {
			TSREC_LOG_INF(
				"ERROR: tsrec_top ioremap failed, return\n");
			return;
		}
	}
#endif

	/* X. end */
	p_iomem_info->found_bits |= (1UL << TSREC_TOP_HW_FOUND_BIT_SHIFT);
	TSREC_LOG_DBG(
		"NOTICE: tsrec_top_base_addr:%p(+%#x), found_bits:%#x (seninf_top_base_addr:%p)\n",
		tsrec_top_base_addr,
		p_iomem_info->top_shift,
		p_iomem_info->found_bits,
		p_iomem_info->seninf_top_base_addr);
}


static void tsrec_no_regs_iomem_init(const unsigned int target, int *p_result,
	struct tsrec_iomem_info_st *p_iomem_info)
{
#ifndef FS_UT
	struct device_node *p_dev_node = seninf_dev->of_node;
	unsigned int base_shift = 0, tsrec_no;
	int ret;

	do {
		/* 1. find tsrec device node */
		p_dev_node = tsrec_utils_of_find_comp_node(p_dev_node, TSREC_COMP_NAME, __func__, 1);
		if (unlikely(p_dev_node == NULL)) {
			*p_result = 1;
			return;
		}

		/* 2. get the "tsrec-no" from the "seninf-tsrec" node */
		ret = tsrec_utils_of_prop_r_u32(p_dev_node, "tsrec-no", &tsrec_no, __func__, 1);
		if ((ret != 0) || (tsrec_no != target))
			continue;

		/* 3. get the "base-shift" from the "seninf-tsrec" node */
		ret = tsrec_utils_of_prop_r_u32(p_dev_node, "base-shift", &base_shift, __func__, 1);
		if (unlikely(ret != 0)) {
			*p_result = 2;
			return;
		}
		p_iomem_info->base_shift_arr[target] = base_shift;

		/* 4. check and setup iomem info */
		if (p_iomem_info->using_unify_iomem) {
			ret = tsrec_setup_iomem_addr_by_base_shift(
				tsrec_top_base_addr, base_shift, &tsrec_base_addr_arr[target]);
			if (unlikely(ret != 0)) {
				*p_result = 3;
				TSREC_LOG_INF(
					"ERROR: seninf-tsrec-top:%p is invalid, not add base-shift (%#x), tsrec-no:%u, return\n",
					tsrec_top_base_addr, base_shift, tsrec_no);
				return;
			}
		} else {
			ret = tsrec_setup_iomem_addr_by_ioremap(
				p_dev_node, &tsrec_base_addr_arr[target]);
			if (unlikely(ret != 0)) {
				*p_result = 3;
				TSREC_LOG_INF(
					"ERROR: tsrec-no:%u ioremap failed, return\n",
					tsrec_no);
				return;
			}
		}

		/* X. end */
		p_iomem_info->found_bits |= (1UL << target);
		*p_result = 0;
		TSREC_LOG_DBG(
			"NOTICE: dts(%s) tsrec_base_addr_arr[%u]:%p(+%#x/+%#x), found_bits:%#x, (%p), return\n",
			p_dev_node->full_name,
			target,
			tsrec_base_addr_arr[target],
			p_iomem_info->top_shift,
			p_iomem_info->base_shift_arr[target],
			p_iomem_info->found_bits,
			tsrec_top_base_addr);
		return;

	} while (p_dev_node);
#endif // !FS_UT
}


static inline void tsrec_regs_iomem_uninit(void)
{
	if (likely(tsrec_base_addr_arr != NULL)) {
		TSREC_DEVM_KFREE(tsrec_base_addr_arr);
		tsrec_base_addr_arr = NULL;
	}
}


static void tsrec_regs_iomem_init(struct tsrec_iomem_info_st *p_iomem_info)
{
	const unsigned int tsrec_hw_cnt = p_iomem_info->hw_max_cnt;
	unsigned int i;
	int ret = 0;

	/* DON't init */
	if (unlikely(tsrec_hw_cnt == 0))
		return;

	/* init, malloc */
	if (likely(tsrec_base_addr_arr == NULL)) {
		tsrec_base_addr_arr =
			TSREC_KCALLOC(tsrec_hw_cnt, sizeof(void __iomem *));
		if (unlikely(tsrec_base_addr_arr == NULL)) {
			TSREC_LOG_INF(
				"ERROR: tsrec_base_addr_arr array of pointer allocate memory failed, tsrec_hw_cnt:%u, return\n",
				tsrec_hw_cnt);
			return;
		}
	}
	memset(tsrec_base_addr_arr, 0, (tsrec_hw_cnt * sizeof(void __iomem *)));
	for (i = 0; i < tsrec_hw_cnt; ++i) {
		tsrec_no_regs_iomem_init(i, &ret, p_iomem_info);
		if (unlikely(ret != 0)) {
			TSREC_LOG_INF(
				"NOTICE: can't find tsrec_no:%u dts info, tsrec_hw_cnt:%u, ret:%d\n",
				i, tsrec_hw_cnt, ret);
		}
	}
}


void mtk_cam_seninf_tsrec_regs_iomem_uninit(void)
{
	tsrec_regs_iomem_uninit();
	tsrec_top_regs_iomem_uninit();

	tsrec_dts_hw_cnt = 0;

	TSREC_LOG_DBG(
		"tsrec_top_base_addr:%p, tsrec_base_addr_arr:%p, tsrec_dts_hw_cnt:%u\n",
		tsrec_top_base_addr,
		tsrec_base_addr_arr,
		tsrec_dts_hw_cnt);
}


void mtk_cam_seninf_tsrec_regs_iomem_init(void __iomem *p_seninf_base,
	const unsigned int tsrec_hw_cnt)
{
	struct tsrec_iomem_info_st iomem_info = {0};

	tsrec_dts_hw_cnt = tsrec_hw_cnt;

	/* set some iomem info for using in latter flow */
	iomem_info.using_unify_iomem = SENINF_UNIFY_IOMEM_MAPPING;
	iomem_info.seninf_top_base_addr = p_seninf_base;
	iomem_info.hw_max_cnt = tsrec_hw_cnt;

	/* init */
	tsrec_top_regs_iomem_init(&iomem_info);
	tsrec_regs_iomem_init(&iomem_info);

	/* dump info for dbg checking */
	tsrec_regs_dbg_dump_iomem_info(&iomem_info);
}
