/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#ifndef __MTK_CAM_ENGINE_H
#define __MTK_CAM_ENGINE_H

#include <linux/atomic.h>

#include "mtk_cam-engine_fsm.h"

enum MTK_CAMSYS_IRQ_EVENT {
	/* with normal_data */
	CAMSYS_IRQ_SETTING_DONE = 0,
	CAMSYS_IRQ_FRAME_START,
	CAMSYS_IRQ_AFO_DONE,
	CAMSYS_IRQ_FRAME_DONE,
	CAMSYS_IRQ_TRY_SENSOR_SET,
	CAMSYS_IRQ_FRAME_DROP,
	CAMSYS_IRQ_FRAME_START_DCIF_MAIN,
	CAMSYS_IRQ_FRAME_SKIPPED,
	/* with error_data */
	CAMSYS_IRQ_ERROR,
};

enum MTK_CAMSYS_ENGINE_TYPE {
	CAMSYS_ENGINE_RAW,
	CAMSYS_ENGINE_MRAW,
	CAMSYS_ENGINE_CAMSV,
	CAMSYS_ENGINE_SENINF,
};

struct mtk_camsys_irq_normal_data {
};

struct mtk_camsys_irq_error_data {
	int err_status;
};

struct mtk_camsys_irq_info {
	int irq_type;
	u64 ts_ns;
	int frame_idx;
	int frame_idx_inner;
	int cookie_done;
	int fbc_empty;
	unsigned int sof_tags;
	unsigned int done_tags;
	unsigned int err_tags;
	union {
		struct mtk_camsys_irq_normal_data	n;
		struct mtk_camsys_irq_error_data	e;
	};
};

struct mtk_cam_device;
struct engine_callback {
	int (*isr_event)(struct mtk_cam_device *cam,
			 int engine_type, unsigned int engine_id,
			 struct mtk_camsys_irq_info *irq_info);

	int (*reset_sensor)(struct mtk_cam_device *cam,
			    int engine_type, unsigned int engine_id,
			    int inner_cookie);
	int (*dump_request)(struct mtk_cam_device *cam,
			    int engine_type, unsigned int engine_id,
			    int inner_cookie);
};

#define do_engine_callback(cb, func, ...) \
({\
	typeof(cb) _cb = (cb);\
	_cb && _cb->func ? _cb->func(__VA_ARGS__) : -1;\
})

struct apply_cq_ref {
	int cookie;
	atomic_t cq_done_cnt;
	atomic_t inner_cnt;
};

static inline void apply_cq_ref_reset(struct apply_cq_ref *ref)
{
	ref->cookie = -1;
	atomic_set(&ref->cq_done_cnt, 0);
	atomic_set(&ref->inner_cnt, 0);
}

static inline void apply_cq_ref_init(struct apply_cq_ref *ref,
				     int cookie, int cnt)
{
	/* it's abnormal if cnt is not zero */
	WARN_ON(atomic_read(&ref->cq_done_cnt));

	ref->cookie = cookie;
	atomic_set(&ref->cq_done_cnt, cnt);
	atomic_set(&ref->inner_cnt, cnt);
}

static inline bool apply_cq_ref_handle_cq_done(struct apply_cq_ref *ref)
{
	return atomic_dec_and_test(&ref->cq_done_cnt);
}

static inline bool apply_cq_ref_handle_sof(struct apply_cq_ref *ref,
					   int inner_cookie)
{
	if (ref && ref->cookie == inner_cookie) {
		atomic_dec(&ref->inner_cnt);
		return 1;
	}
	return 0;
}

static inline bool apply_cq_ref_is_to_inner(struct apply_cq_ref *ref)
{
	return  atomic_read(&ref->inner_cnt) == 0;
}


static inline int assign_apply_cq_ref(struct apply_cq_ref **ref,
				      struct apply_cq_ref *target)
{
	if (*ref)
		return -1;
	*ref = target;
	return 0;
}

static inline bool engine_handle_cq_done(struct apply_cq_ref **ref)
{
	return apply_cq_ref_handle_cq_done(*ref);
}

static inline void engine_handle_sof(struct apply_cq_ref **ref, int inner_cookie)
{
	bool reset_ref = apply_cq_ref_handle_sof(*ref, inner_cookie);

	if (reset_ref)
		*ref = NULL;
}

#endif //__MTK_CAM_ENGINE_H
