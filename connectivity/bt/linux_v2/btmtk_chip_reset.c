/**
 *  Copyright (c) 2018 MediaTek Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See http://www.gnu.org/licenses/gpl-2.0.html for more details.
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
	BTMTK_DBG("%s: create chip_reset timer", __func__);
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
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();
	int state = BTMTK_STATE_INIT;
	int err = 0;
	int cur = 0;
	unsigned char fstate = BTMTK_FOPS_STATE_INIT;
#if (USE_DEVICE_NODE == 0)
	int cif_event = 0
	struct btmtk_cif_state *cif_state = NULL;
#endif

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

	fstate = btmtk_fops_get_state(bdev);
	if (fstate == BTMTK_FOPS_STATE_CLOSED) {
		BTMTK_WARN("%s: fops is closed(%d), not trigger reset", __func__, fstate);
		return;
	}

	btmtk_reset_timer_del(bdev);
	if (atomic_read(&bmain_info->chip_reset) ||
		atomic_read(&bmain_info->subsys_reset)) {
		BTMTK_INFO("%s return, chip_reset = %d, subsys_reset = %d!", __func__,
			atomic_read(&bmain_info->chip_reset), atomic_read(&bmain_info->subsys_reset));
		return;
	}

#if (USE_DEVICE_NODE == 0)
	if (bmain_info->hif_hook.dump_debug_sop)
		bmain_info->hif_hook.dump_debug_sop(bdev);

	DUMP_TIME_STAMP("chip_reset_start");
	cif_event = HIF_EVENT_SUBSYS_RESET;
	if (BTMTK_CIF_IS_NULL(bdev, cif_event)) {
		/* Error */
		BTMTK_WARN("%s priv setting is NULL", __func__);
		return;
	}

	cif_state = &bdev->cif_state[cif_event];

	if (!bdev->bt_cfg.support_dongle_reset) {
		BTMTK_ERR("%s chip_reset is not support", __func__);
		return;
	}

	/* Set Entering state */
	btmtk_set_chip_state((void *)bdev, cif_state->ops_enter);
#endif

	BTMTK_INFO("%s: Receive a byte (0xFF) subsys_reset_state[%d] chip_reset_flag[%d]", __func__, 
					atomic_read(&bmain_info->subsys_reset), bmain_info->chip_reset_flag);
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
			/*
			 * Discard this part for SP platform since Consys power is off after BT off,
			 * Nothing is remain on memory after BT off, so leave do this at BT on
			 * for SP platform
			 */
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
#if (USE_DEVICE_NODE == 0)

			err = btmtk_load_rom_patch(bdev);
			if (err < 0) {
				BTMTK_INFO("btmtk load rom patch failed!");
				goto L0RESET;
			}
#endif
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
		BTMTK_INFO("%s: chip_reset_flag[%d], subsys_reset_count[%d], whole_reset_count[%d]",
			__func__,
			bmain_info->chip_reset_flag,
			atomic_read(&bmain_info->subsys_reset_conti_count),
			atomic_read(&bmain_info->whole_reset_count));
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
			cur = atomic_cmpxchg(&bmain_info->chip_reset, BTMTK_RESET_DONE, BTMTK_RESET_DOING);
			if (cur == BTMTK_RESET_DOING) {
				BTMTK_INFO("%s: have chip_reset in progress, return", __func__);
				return;
			}
			DUMP_TIME_STAMP("whole_chip_reset_start");
			err = bmain_info->hif_hook.whole_reset(bdev);
			atomic_inc(&bmain_info->whole_reset_count);
			/* trigger whole chip reset every three subsys resets*/
			atomic_set(&bmain_info->subsys_reset_conti_count, 0);
			DUMP_TIME_STAMP("whole_chip_reset_end");
		} else {
			BTMTK_INFO("%s: Not support whole chip reset, reset reset_conti_count to 0", __func__);
			atomic_set(&bmain_info->subsys_reset_conti_count, 0);
#if (USE_DEVICE_NODE == 1)
			btmtk_send_hw_err_to_host(bdev);
#endif
		}
	}

	DUMP_TIME_STAMP("chip_reset_end");

#if (USE_DEVICE_NODE == 0)
	/* Set End/Error state */
	if (err < 0)
		btmtk_set_chip_state((void *)bdev, cif_state->ops_error);
	else
		btmtk_set_chip_state((void *)bdev, cif_state->ops_end);
#endif

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

