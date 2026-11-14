/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <linux/device.h>
#include <linux/pm_wakeup.h>
#include "btmtk_define.h"
#include "btmtk_main.h"

#include "btmtk_chip_reset.h"

static void btmtk_reset_wake_lock(struct btmtk_dev *bdev)
{
	struct btmtk_dev *bt0 = btmtk_get_bt0(bdev);

	if (bt0 == NULL) {
		BTMTK_ERR("Error: bt0 is NULL\n");
		return;
	}

	BTMTK_INFO_DEV(bt0, "%s: enter", __func__);
	__pm_stay_awake(bt0->main_info.chip_reset_ws);
	BTMTK_INFO_DEV(bt0, "%s: exit", __func__);
}

static void btmtk_reset_wake_unlock(struct btmtk_dev *bdev)
{
	struct btmtk_dev *bt0 = btmtk_get_bt0(bdev);

	if (bt0 == NULL) {
		BTMTK_ERR("Error: bt0 is NULL\n");
		return;
	}

	BTMTK_INFO_DEV(bt0, "%s: enter", __func__);
	__pm_relax(bt0->main_info.chip_reset_ws);
	BTMTK_INFO_DEV(bt0, "%s: exit", __func__);
}

#if (KERNEL_VERSION(4, 15, 0) > LINUX_VERSION_CODE)
static void btmtk_reset_timer(unsigned long arg)
{
	struct btmtk_dev *bt0 = btmtk_get_bt0((struct btmtk_dev *)arg);

	BTMTK_INFO_DEV(bt0, "%s: chip_reset not trigger in %d seconds, trigger it directly",
		__func__, CHIP_RESET_TIMEOUT);
#ifdef CHIP_IF_USB
	bt0->chip_reset_signal = 0x00;
#endif
	schedule_work(&bt0->reset_waker);
}
#else
static void btmtk_reset_timer(struct timer_list *timer)
{
	struct btmtk_dev *bt0 = btmtk_get_bt0(from_timer(bt0, timer, chip_reset_timer));

	if (bt0 == NULL) {
		BTMTK_ERR("%s bt0 is NULL", __func__);
		return;
	}

	BTMTK_INFO_DEV(bt0, "%s: chip_reset not trigger in %d seconds, trigger it directly",
		__func__, CHIP_RESET_TIMEOUT);
#ifdef CHIP_IF_USB
	bt0->chip_reset_signal = 0x00;
#endif
	schedule_work(&bt0->reset_waker);
}
#endif

void btmtk_reset_timer_add(struct btmtk_dev *bdev)
{
	struct btmtk_dev *bt0 = btmtk_get_bt0(bdev);

	BTMTK_DBG_DEV(bt0, "%s: create chip_reset timer", __func__);
	if (bt0 == NULL) {
		BTMTK_ERR("%s bt0 is NULL", __func__);
		return;
	}
#if (KERNEL_VERSION(4, 15, 0) > LINUX_VERSION_CODE)
	init_timer(&bt0->chip_reset_timer);
	bt0->chip_reset_timer.function = btmtk_reset_timer;
	bt0->chip_reset_timer.data = (unsigned long)bt0;
#else
	timer_setup(&bt0->chip_reset_timer, btmtk_reset_timer, 0);
#endif
}

void btmtk_reset_timer_update(struct btmtk_dev *bdev)
{
	struct btmtk_dev *bt0 = btmtk_get_bt0(bdev);

	if (bt0 == NULL) {
		BTMTK_ERR("%s bt0 is NULL", __func__);
		return;
	}
	mod_timer(&bt0->chip_reset_timer, jiffies + CHIP_RESET_TIMEOUT * HZ);
}

void btmtk_reset_timer_del(struct btmtk_dev *bdev)
{
	struct btmtk_dev *bt0 = btmtk_get_bt0(bdev);

	if (bt0 == NULL) {
		BTMTK_ERR("%s bt0 is NULL", __func__);
		return;
	}

	if (timer_pending(&bt0->chip_reset_timer)) {
#if (KERNEL_VERSION(6, 15, 0) > LINUX_VERSION_CODE)
		del_timer_sync(&bt0->chip_reset_timer);
#else
		timer_delete_sync(&bt0->chip_reset_timer);
#endif
		BTMTK_INFO_DEV(bt0, "%s exit", __func__);
	}
}

bool btmtk_reset_timer_check(struct btmtk_dev *bdev)
{
	struct btmtk_dev *bt0 = btmtk_get_bt0(bdev);

	if (bt0 == NULL) {
		BTMTK_ERR("%s bt0 is NULL", __func__);
		return false;
	}

	if (timer_pending(&bt0->chip_reset_timer))
		return true;
	else
		return false;
}

void btmtk_reset_waker(struct work_struct *work)
{
	struct btmtk_dev *bt0 = container_of(work, struct btmtk_dev, reset_waker);
	struct btmtk_dev *bdev = bt0;
	int state = 0;
	int err = 0;
	int cur = 0;
	int subsys_reset_flag = 0;
#if (USE_DEVICE_NODE == 0)
	int set_chip_state_flag = 0;
	int cif_event = 0;
	struct btmtk_cif_state *cif_state = NULL;
	struct btmtk_dev *bt1 = NULL;
#else
	unsigned char fstate = 0;
#endif

	if (bt0 == NULL) {
		BTMTK_ERR_DEV(bt0, "%s bt0 is NULL", __func__);
		return;
	}

	btmtk_handle_mutex_lock(bt0);

	/*Noted, we must lock wakelock location properly to protect SDIO RW operation
	and chip reset procedure from breaking by suspend event.*/
	btmtk_reset_wake_lock(bt0);

	/* Check chip state is ok to do reset or not */
	state = btmtk_get_chip_state(bt0);
	if ((state == BTMTK_STATE_SUSPEND && !test_bit(BT_WOBLE_FAIL_FOR_SUBSYS_RESET, &bt0->flags)) || state == BTMTK_STATE_DISCONNECT) {
		BTMTK_INFO_DEV(bt0, "%s suspend or disconnect state don't do chip reset!", __func__);
		goto Finish;
	}

	if (state == BTMTK_STATE_PROBE) {
		if (!test_bit(BT_PROBE_FAIL_FOR_SUBSYS_RESET, &bt0->flags)) {
			bt0->main_info.chip_reset_flag = 1;
			BTMTK_INFO_DEV(bt0, "%s just do whole chip reset in probe stage!", __func__);
		}
	}

#if (USE_DEVICE_NODE == 1)
	fstate = btmtk_fops_get_state(bt0);
	if (fstate == BTMTK_FOPS_STATE_CLOSED) {
		BTMTK_WARN_DEV(bt0, "%s: fops is closed(%d), not trigger reset", __func__, fstate);
		goto Finish;
	}
#endif

	if (is_mt7663(bt0->chip_id)) {
		bt0->main_info.chip_reset_flag = 1;
		BTMTK_WARN_DEV(bt0, "%s: 7663 not support L0.5 reset, do whole chip reset", __func__);
	}

	btmtk_reset_timer_del(bt0);

	if (atomic_read(&bt0->main_info.chip_reset) ||
		atomic_read(&bt0->main_info.subsys_reset)) {
		BTMTK_INFO_DEV(bt0, "%s return, chip_reset = %d, subsys_reset = %d!", __func__,
			atomic_read(&bt0->main_info.chip_reset), atomic_read(&bt0->main_info.subsys_reset));
		goto Finish;
	}

#if (USE_DEVICE_NODE == 0)
	if (bt0->main_info.hif_hook.dump_debug_sop)
		bt0->main_info.hif_hook.dump_debug_sop(bt0);

#ifdef CFG_SUPPORT_CHIP_RESET_KO
	send_reset_event(RESET_MODULE_TYPE_BT, RFSM_EVENT_L0_RESET_READY);
#endif
	if (!bt0->bt_cfg.support_dongle_subsys_reset) {
		BTMTK_WARN_DEV(bt0, "%s subsys reset is not support", __func__);
		bt0->main_info.chip_reset_flag = 1;
	}

	DUMP_TIME_STAMP("chip_reset_start");
	cif_event = HIF_EVENT_SUBSYS_RESET;
	if (BTMTK_CIF_IS_NULL(bt0, cif_event)) {
		/* Error */
		BTMTK_WARN_DEV(bt0, "%s priv setting is NULL", __func__);
		goto Finish;
	}

	cif_state = &bt0->cif_state[cif_event];

	if (!bt0->bt_cfg.support_dongle_subsys_reset && !bt0->bt_cfg.support_dongle_whole_reset) {
		BTMTK_ERR_DEV(bt0, "%s chip_reset is not support", __func__);
		goto Finish;
	}

	/* Set Entering state */
	btmtk_set_chip_state(bt0, cif_state->ops_enter);
	set_chip_state_flag = 1;
#endif

	BTMTK_INFO_DEV(bt0, "%s: Receive WDT interrupt subsys_reset_state[%d] chip_reset_flag[%d]", __func__,
					atomic_read(&bt0->main_info.subsys_reset), bt0->main_info.chip_reset_flag);
	/* read interrupt EP15 CR */

	bt0->sco_num = 0;

	if (bt0->main_info.chip_reset_flag == 0 &&
			atomic_read(&bt0->main_info.subsys_reset_conti_count) < BTMTK_MAX_SUBSYS_RESET_COUNT) {
		if (bt0->main_info.hif_hook.subsys_reset) {
			cur = atomic_cmpxchg(&bt0->main_info.subsys_reset, BTMTK_RESET_DONE, BTMTK_RESET_DOING);
			if (cur == BTMTK_RESET_DOING) {
				BTMTK_INFO_DEV(bt0, "%s: subsys reset in progress, return", __func__);
				err = 0;
				goto Finish;
			}
			DUMP_TIME_STAMP("subsys_chip_reset_start");
			/*
			 * Discard this part for SP platform since Consys power is off after BT off,
			 * Nothing is remain on memory after BT off, so leave do this at BT on
			 * for SP platform
			 */
			err = bt0->main_info.hif_hook.subsys_reset(bt0);
			atomic_set(&bt0->main_info.subsys_reset, BTMTK_RESET_DONE);
			if (err < 0) {
				if (err == -ENODEV) {
					BTMTK_INFO_DEV(bt0, "%s, usb disconnect, no more chip reset", __func__);
					goto Finish;
				}
				BTMTK_INFO_DEV(bt0, "subsys reset failed, do whole chip reset!");
				goto L0RESET;
			}
			atomic_inc(&bt0->main_info.subsys_reset_count);
			atomic_inc(&bt0->main_info.subsys_reset_conti_count);
			DUMP_TIME_STAMP("subsys_chip_reset_end");

			if (!test_bit(BT_PROBE_FAIL_FOR_SUBSYS_RESET, &bt0->flags)) {
				bt0->main_info.reset_stack_flag = HW_ERR_CODE_CHIP_RESET;
			}
#if (USE_DEVICE_NODE == 1) || !defined(CHIP_IF_UART_TTY)
			err = btmtk_cap_init(bt0);
			if (err < 0) {
				BTMTK_ERR_DEV(bt0, "btmtk init failed!");
				goto L0RESET;
			}
#if (USE_DEVICE_NODE == 0)
			err = btmtk_load_rom_patch(bt0);
			if (err < 0) {
				BTMTK_INFO_DEV(bt0, "btmtk load rom patch failed!");
				goto L0RESET;
			}
#endif
#endif
			btmtk_woble_wake_unlock(bt0);
			btmtk_assert_wake_unlock(bt0);
			if (bt0->main_info.hif_hook.chip_reset_notify)
				bt0->main_info.hif_hook.chip_reset_notify(bt0);
			subsys_reset_flag = 1;
		} else {
			err = -1;
			BTMTK_INFO_DEV(bt0, "%s: Not support subsys chip reset", __func__);
			goto L0RESET;
		}
	} else {
		err = -1;
		BTMTK_INFO_DEV(bt0, "%s: chip_reset_flag[%d], subsys_reset_count[%d], whole_reset_count[%d]",
			__func__,
			bt0->main_info.chip_reset_flag,
			atomic_read(&bt0->main_info.subsys_reset_conti_count),
			atomic_read(&bt0->main_info.whole_reset_count));
	}

	if (err < 0 && test_bit(BT_WOBLE_FAIL_FOR_SUBSYS_RESET, &bt0->flags)) {
		BTMTK_WARN_DEV(bt0, "%s: L0.5 fail when Woble fail, skip L0, return", __func__);
		goto Finish;
	}

L0RESET:
	if (err < 0) {
		/* L0.5 reset failed or not support, do whole chip reset */
		/* TODO: need to confirm with usb host when suspend fail, to do chip reset,
		 * because usb3.0 need to toggle reset pin after hub_event unfreeze,
		 * otherwise, it will not occur disconnect on Capy Platform. When Mstar
		 * chip has usb3.0 port, we will use Mstar platform to do comparison
		 * test, then found the final solution.
		 */
		/* msleep(2000); */
#if (USE_DEVICE_NODE == 0)
		if (bt0->bt_cfg.support_dongle_whole_reset
				&& DONGLE_FILE_IS_VALID(bt0, bt0->bt_cfg.bt_cfg_file_idx)) {
#endif
			if (bt0->main_info.hif_hook.whole_reset) {
				cur = atomic_cmpxchg(&bt0->main_info.chip_reset, BTMTK_RESET_DONE, BTMTK_RESET_DOING);
				if (cur == BTMTK_RESET_DOING) {
					err = 0;
					BTMTK_INFO_DEV(bt0, "%s: have chip_reset in progress, return", __func__);
					goto Finish;
				}
#ifdef CFG_SUPPORT_CHIP_RESET_KO
				DUMP_TIME_STAMP("Send reset message to reset ko");
				send_reset_event(RESET_MODULE_TYPE_BT, RFSM_EVENT_TRIGGER_RESET);
				atomic_inc(&bt0->main_info.whole_reset_count);
#else
				DUMP_TIME_STAMP("whole_chip_reset_start");
				err = bt0->main_info.hif_hook.whole_reset(bt0);
				atomic_inc(&bt0->main_info.whole_reset_count);
				/* trigger whole chip reset every three subsys resets*/
				atomic_set(&bt0->main_info.subsys_reset_conti_count, 0);
				DUMP_TIME_STAMP("whole_chip_reset_end");
#endif
			} else {
				BTMTK_INFO_DEV(bt0, "%s: Not support whole chip reset, reset_conti_count to 0",
						__func__);
				atomic_set(&bt0->main_info.subsys_reset_conti_count, 0);
#if (USE_DEVICE_NODE == 1)
				btmtk_send_hw_err_to_host(bt0);
#endif
			}
#if (USE_DEVICE_NODE == 0)
		} else {
			BTMTK_INFO_DEV(bt0, "%s: Not support whole chip reset", __func__);
		}
#endif
	}

Finish:
#if (USE_DEVICE_NODE == 0)
	bt0->main_info.chip_reset_flag = 0;
	/* Set End/Error state */
	if (set_chip_state_flag) {
		if (err < 0)
			btmtk_set_chip_state(bt0, cif_state->ops_error);
		else
			btmtk_set_chip_state(bt0, cif_state->ops_end);
	}

	bt1 = btmtk_get_bt1(bt0);
	if (bt1) {
		if (set_chip_state_flag) {
			btmtk_send_hw_err_to_host(bt1);
			if (err < 0)
				btmtk_set_chip_state(bt1, cif_state->ops_error);
			else
				btmtk_set_chip_state(bt1, cif_state->ops_end);
		}
	}
#endif
	/* HW ERR event must be sent to host for L0.5 after set chip state.
	 * Otherwise bt close maybe return directly because the chip state is not working
	 */
	if (subsys_reset_flag == 1)
		btmtk_send_hw_err_to_host(bt0);

	if (test_bit(BT_WOBLE_FAIL_FOR_SUBSYS_RESET, &bt0->flags)) {
		set_bit(BT_WOBLE_RESET_DONE, &bt0->flags);

		if (err < 0)
			set_bit(BT_WOBLE_SKIP_WHOLE_CHIP_RESET, &bt0->flags);

		wake_up(&bt0->p_woble_fail_q);
	}

	if (test_bit(BT_PROBE_FAIL_FOR_SUBSYS_RESET, &bt0->flags) ||
			test_bit(BT_PROBE_FAIL_FOR_WHOLE_RESET, &bt0->flags)) {
		set_bit(BT_PROBE_RESET_DONE, &bt0->flags);
		if (err < 0)
			set_bit(BT_PROBE_DO_WHOLE_CHIP_RESET, &bt0->flags);

		if (test_bit(BT_PROBE_FAIL_FOR_SUBSYS_RESET, &bt0->flags))
			wake_up_interruptible(&bt0->probe_fail_wq);

		if (test_bit(BT_PROBE_FAIL_FOR_WHOLE_RESET, &bt0->flags))
			clear_bit(BT_PROBE_FAIL_FOR_WHOLE_RESET, &bt0->flags);
	}

	btmtk_reset_wake_unlock(bt0);
	btmtk_handle_mutex_unlock(bt0);

	DUMP_TIME_STAMP("chip_reset_end");
}

void btmtk_reset_trigger(struct btmtk_dev *bdev)
{
	struct btmtk_dev *bt0 = btmtk_get_bt0(bdev);

	if (bt0 == NULL) {
		BTMTK_ERR_DEV(bt0, "%s bt0 is NULL", __func__);
		return;
	}

	if (atomic_read(&bt0->main_info.chip_reset) ||
		atomic_read(&bt0->main_info.subsys_reset)) {
		BTMTK_INFO_DEV(bt0, "%s return, chip_reset = %d, subsys_reset = %d!", __func__,
			atomic_read(&bt0->main_info.chip_reset), atomic_read(&bt0->main_info.subsys_reset));
		return;
	}

	schedule_work(&bt0->reset_waker);
}
