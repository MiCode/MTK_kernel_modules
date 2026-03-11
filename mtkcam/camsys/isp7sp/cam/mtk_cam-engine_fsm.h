/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef __MTK_CAM_ENGINE_FSM_H
#define __MTK_CAM_ENGINE_FSM_H

#include <linux/compiler.h>
#include <linux/dev_printk.h>

/*
 * legend:
 *     |: sof
 *     o: done
 *     x: won't happen
 *     ^: irq_handler execution
 *     !: cq done
 *
 *               (1)        (2)        (3)        (4)
 *         ....___|______o___|______o___|______o___|______o___|____....
 *
 *
 *     out             <2>        <3>       <4>        ...
 *     in              <1>        <2>       <3>        <4>
 *                  !          !          !
 * ideal case:     ^ ^    ^   ^ ^    ^   ^ ^    ^   ^      ^
 *
 *     out             <2>        <3>       ...
 *     in              <1>        <2>       <3>        ...
 *                  !          !
 * case a-1:       ^  ^       ^  ^
 *     irq                  (o+|)
 *
 *     out             <2>        <x>       <3>
 *     in              <1>        <2>       <2>        <3>
 *                  !                  !  (!)
 * case a-2:       ^  ^               ^  ^   ^
 *     irq                         (o*2+|)
 *
 *                  !                      !   x
 * case a-3:       ^  ^                   ^  ^
 *     irq                           (o*2+|*2)
 *
 *     handling:
 *        a-1:
 *             1) handle done for <1>
 *             2) handle sof, sw's inner == <2>
 *        a-2:
 *             1) => done for <1>
 *             2) => sw's inner == <2>
 *                (=> trigger cq may meet cq trigger delay)
 *             2) at sof (3),
 *                => inner == <2> && fbc_cnt == 0
 *                => <2>'s done (*)
 *        a-3:
 *             1) handle done for <1>
 *             2) sw's inner == <2> && fbc_cnt == 0
 *                => <2>'s done (*)
 *
 *
 *               (1)        (2)        (3)        (4)
 *         ....___|______o___|______o___|______o___|______o___|____....
 *
 *
 *     out             <2>        <3>       <4>        ...
 *     in              <1>        <2>       <3>        <4>
 *                  !          !          !
 * ideal case:     ^ ^    ^   ^ ^    ^   ^ ^    ^   ^      ^
 *
 *     out             <2>        <x>       <3>
 *     in              <1>        <2>       <2>        ...
 *                  !                  !
 * case b-1:       ^  ^    ^          ^   ^
 *     irq                           (|+o)
 *
 *     out             <2>        <x>       <3>
 *     in              <1>        <2>       <2>        ...
 *                  !                     !
 * case b-2:       ^  ^    ^             ^ ^
 *     irq                           (|*2+o)
 *
 *     out             <2>        <x>       <x>        <3>
 *     in              <1>        <2>       <2>        ...
 *                  !                             !
 * case b-3:       ^  ^    ^                     ^  ^
 *     irq                                   (|*2+o*2)
 *
 *     handling
 *        b-1:
 *             1) handle done for <1> (duplicated)
 *             2) sw's inner == 2, fbc_cnt == 0
 *                => <2>'s done (*)
 *        b-2:
 *             1) handle done for <1> (duplicated)
 *             2) sw's inner == 2, fbc_cnt == 0
 *                => <2>'s done (*)
 *        b-2:
 *             1) handle done for <1> (duplicated)
 *             2) sw's inner == 2, fbc_cnt == 0
 *                => <2>'s done (*)
 *
 * case c: hw incomplete
 *      - no db_load at sof
 *      - re-do for inner
 *
 * case d: cq trigger delay
 *
 * case e: camsv separated irq for sof & done
 *         cpu may receive sof/done in reverse order
 *
 *   hw:    ____|____o_|____o_|____
 *  isr:    ____|_______|o_____o_|____ (possible)
 *
 */

#define ENABLE_FSM 1
#define ENABLE_FSM_LOG 0
/* note:
 *   in current usage, no lock is required since all data is accessed in
 *   interrupt & threaded_irq, which are mutual-exclusive executing (both on
 *   cpu0 only)
 */
struct engine_fsm {
#if ENABLE_FSM_LOG
	struct device *dev;
#endif

	/* cookie infos */
	int latest_done;
	int inner;
};

static inline void fsm_log_ret(struct engine_fsm *fsm, const char *func, int ret)
{
#if ENABLE_FSM_LOG
	if (fsm->dev)
		dev_info(fsm->dev, "%s: (0x%x 0x%x) ret = %d\n", func,
			 fsm->latest_done, fsm->inner, ret);
#endif
}

static inline bool engine_in_processing(struct engine_fsm *fsm)
{
	return fsm->inner != fsm->latest_done;
}

static inline void engine_fsm_reset(struct engine_fsm *fsm, struct device *dev)
{
#if ENABLE_FSM_LOG
	fsm->dev = dev;
#endif

	/* initialized to -1 for first frame */
	fsm->latest_done = -1;
	fsm->inner = -1;
}

static inline int engine_update_for_done(struct engine_fsm *fsm)
{
	fsm->latest_done = fsm->inner;
	return fsm->inner;
}

/* return: 0: nothing, 1: have done */
static inline int engine_fsm_sof(struct engine_fsm *fsm,
				 int cookie_inner, int fbc_emtpy,
				 int *cookie_done)
{
	bool inner_updated;
	int ret = 0;

	inner_updated = cookie_inner != fsm->inner;

#if ENABLE_FSM
	/* inner changes while in processing: done is missed */
	if (cookie_done && engine_in_processing(fsm) && inner_updated) {

		*cookie_done = engine_update_for_done(fsm);
		ret = 1;
	}
#endif

	fsm->inner = cookie_inner;

#if ENABLE_FSM
	/* note: if ret is already 1, may still handle it in next sof */
	if (cookie_done && !ret && fbc_emtpy && engine_in_processing(fsm)) {

		*cookie_done = engine_update_for_done(fsm);
		ret = 1;
	}
#endif

#if ENABLE_FSM_LOG
	fsm_log_ret(fsm, __func__, ret);
#endif
	return ret;
}

static inline int engine_fsm_partial_done(struct engine_fsm *fsm,
					  int *cookie_done)
{
	*cookie_done = fsm->inner;

#if ENABLE_FSM_LOG
	fsm_log_ret(fsm, __func__, 0);
#endif
	return 0;
}

/* return: 0: nothing, 1: have done */
static inline int engine_fsm_hw_done(struct engine_fsm *fsm, int *cookie_done)
{
	int ret = 0;

#if ENABLE_FSM
	if (engine_in_processing(fsm)) {

		*cookie_done = engine_update_for_done(fsm);
		ret = 1;
	}
#else
	*cookie_done = fsm->inner;
	ret = 1;
#endif

#if ENABLE_FSM_LOG
	fsm_log_ret(fsm, __func__, ret);
#endif
	return ret;
}

#endif /* __MTK_CAM_ENGINE_FSM_H */
