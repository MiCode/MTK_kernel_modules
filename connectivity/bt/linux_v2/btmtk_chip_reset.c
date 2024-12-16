// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018 MediaTek Inc.
 */

#include "btmtk_chip_reset.h"

#if (KERNEL_VERSION(4, 15, 0) > LINUX_VERSION_CODE)
static void btmtk_reset_timer(unsigned long arg)
{
	struct btmtk_dev *bdev = (struct btmtk_dev *)arg;

	BTMTK_INFO("%s: chip_reset not trigger in %d seconds, trigger it directly",
		__func__, CHIP_RESET_TIMEOUT);
	schedule_work(&bdev->reset_waker);
}
#else
static void btmtk_reset_timer(struct timer_list *timer)
{
	struct btmtk_dev *bdev = from_timer(bdev, timer, chip_reset_timer);

	BTMTK_INFO("%s: chip_reset not trigger in %d seconds, trigger it directly",
		__func__, CHIP_RESET_TIMEOUT);
	schedule_work(&bdev->reset_waker);
}
#endif

void btmtk_reset_timer_add(struct btmtk_dev *bdev)
{
	BTMTK_INFO("%s: create chip_reset timer", __func__);
#if (KERNEL_VERSION(4, 15, 0) > LINUX_VERSION_CODE)
	init_timer(&bdev->chip_reset_timer);
	bdev->chip_reset_timer.function = btmtk_reset_timer;
	bdev->chip_reset_timer.data = (unsigned long)bdev;
#else
	timer_setup(&bdev->chip_reset_timer, btmtk_reset_timer, 0);
#endif
}

void btmtk_reset_timer_update(struct btmtk_dev *bdev)
{
	mod_timer(&bdev->chip_reset_timer, jiffies + CHIP_RESET_TIMEOUT * HZ);
}

void btmtk_reset_timer_del(struct btmtk_dev *bdev)
{
	if (timer_pending(&bdev->chip_reset_timer)) {
		del_timer_sync(&bdev->chip_reset_timer);
		BTMTK_INFO("%s exit", __func__);
	}
}

void btmtk_reset_waker(struct work_struct *work)
{
	struct btmtk_dev *bdev = container_of(work, struct btmtk_dev, reset_waker);
	struct btmtk_cif_state *cif_state = NULL;
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();
	int state = BTMTK_STATE_INIT;
	int cif_event = 0, err = 0;
	int cur = 0;

	/* Check chip state is ok to do reset or not */
	state = btmtk_get_chip_state(bdev);
	if (state == BTMTK_STATE_SUSPEND) {
		BTMTK_INFO("%s suspend state don't do chip reset!", __func__);
		return;
	}
	if (state == BTMTK_STATE_PROBE) {
		bmain_info->chip_reset_flag = 1;
		BTMTK_INFO("%s just do whole chip reset in probe stage!", __func__);
	}

	btmtk_reset_timer_del(bdev);

	if (atomic_read(&bmain_info->chip_reset) ||
		atomic_read(&bmain_info->subsys_reset)) {
		BTMTK_INFO("%s return, chip_reset = %d, subsys_reset = %d!", __func__,
			atomic_read(&bmain_info->chip_reset), atomic_read(&bmain_info->subsys_reset));
		return;
	}

#ifdef CFG_CHIP_RESET_KO_SUPPORT
	if (rstNotifyWholeChipRstStatus(RST_MODULE_BT, RST_MODULE_STATE_DUMP_START, NULL) == RST_MODULE_RET_FAIL)
		return;
#endif

	if (bmain_info->hif_hook.dump_debug_sop)
		bmain_info->hif_hook.dump_debug_sop(bdev);

#ifdef CFG_CHIP_RESET_KO_SUPPORT
	rstNotifyWholeChipRstStatus(RST_MODULE_BT, RST_MODULE_STATE_DUMP_END, NULL);
#endif

	DUMP_TIME_STAMP("chip_reset_start");
	cif_event = HIF_EVENT_SUBSYS_RESET;
	if (BTMTK_CIF_IS_NULL(bdev, cif_event)) {
		/* Error */
		BTMTK_WARN("%s priv setting is NULL", __func__);
		return;
	}

	if (!bdev->bt_cfg.support_dongle_reset) {
		BTMTK_ERR("%s chip_reset is not support", __func__);
		return;
	}

	cif_state = &bdev->cif_state[cif_event];

	/* Set Entering state */
	btmtk_set_chip_state((void *)bdev, cif_state->ops_enter);

	BTMTK_INFO("%s: Receive a byte (0xFF)", __func__);
	/* read interrupt EP15 CR */

	bdev->sco_num = 0;

	if (bmain_info->chip_reset_flag == 0 &&
			atomic_read(&bmain_info->subsys_reset_conti_count) < BTMTK_MAX_SUBSYS_RESET_COUNT) {
		if (bmain_info->hif_hook.subsys_reset) {
			cur = atomic_cmpxchg(&bmain_info->subsys_reset, BTMTK_RESET_DONE, BTMTK_RESET_DOING);
			if (cur == BTMTK_RESET_DOING) {
				BTMTK_INFO("%s: subsys reset in progress, return", __func__);
				return;
			}
			DUMP_TIME_STAMP("subsys_chip_reset_start");
			err = bmain_info->hif_hook.subsys_reset(bdev);
			atomic_set(&bmain_info->subsys_reset, BTMTK_RESET_DONE);
			if (err < 0) {
				BTMTK_INFO("subsys reset failed, do whole chip reset!");
				goto L0RESET;
			}
			atomic_inc(&bmain_info->subsys_reset_count);
			atomic_inc(&bmain_info->subsys_reset_conti_count);
			DUMP_TIME_STAMP("subsys_chip_reset_end");

			bmain_info->reset_stack_flag = HW_ERR_CODE_CHIP_RESET;

			err = btmtk_cap_init(bdev);
			if (err < 0) {
				BTMTK_ERR("btmtk init failed!");
				goto L0RESET;
			}
			err = btmtk_load_rom_patch(bdev);
			if (err < 0) {
				BTMTK_INFO("btmtk load rom patch failed!");
				goto L0RESET;
			}
			btmtk_send_hw_err_to_host(bdev);
			btmtk_woble_wake_unlock(bdev);
			if (bmain_info->hif_hook.chip_reset_notify)
				bmain_info->hif_hook.chip_reset_notify(bdev);
		} else {
			err = -1;
			BTMTK_INFO("%s: Not support subsys chip reset", __func__);
			goto L0RESET;
		}
	} else {
		err = -1;
		BTMTK_INFO("%s: chip_reset_flag is %d, subsys_reset_count %d",
			__func__,
			bmain_info->chip_reset_flag,
			atomic_read(&bmain_info->subsys_reset_conti_count));
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
		if (bmain_info->hif_hook.whole_reset) {
			DUMP_TIME_STAMP("whole_chip_reset_start");
			bmain_info->hif_hook.whole_reset(bdev);
			atomic_inc(&bmain_info->whole_reset_count);
			DUMP_TIME_STAMP("whole_chip_reset_end");
		} else {
			BTMTK_INFO("%s: Not support whole chip reset", __func__);
		}
	}

	DUMP_TIME_STAMP("chip_reset_end");
	/* Set End/Error state */
	if (err < 0)
		btmtk_set_chip_state((void *)bdev, cif_state->ops_error);
	else
		btmtk_set_chip_state((void *)bdev, cif_state->ops_end);
}

void btmtk_reset_trigger(struct btmtk_dev *bdev)
{
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();

	if (atomic_read(&bmain_info->chip_reset) ||
		atomic_read(&bmain_info->subsys_reset)) {
		BTMTK_INFO("%s return, chip_reset = %d, subsys_reset = %d!", __func__,
			atomic_read(&bmain_info->chip_reset), atomic_read(&bmain_info->subsys_reset));
		return;
	}

	schedule_work(&bdev->reset_waker);
}

