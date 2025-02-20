/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/input.h>
#include <linux/pm_wakeup.h>
#include <linux/interrupt.h>

#include "btmtk_woble.h"
#include "btmtk_chip_reset.h"

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
#define ATTRIBUTE_NO_KCFI __nocfi
#else
#define ATTRIBUTE_NO_KCFI
#endif

static int is_support_unify_woble(struct btmtk_dev *bdev)
{
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();

	if (bdev->bt_cfg.support_unify_woble)
		return bmain_info->hif_hook_chip.support_woble;
	else
		return 0;
}

void btmtk_woble_wake_lock(struct btmtk_dev *bdev)
{
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();

	if (bdev->bt_cfg.support_woble_wakelock) {
		BTMTK_INFO("%s: enter", __func__);
		__pm_stay_awake(bmain_info->woble_ws);
		BTMTK_INFO("%s: exit", __func__);
	}
}

void btmtk_woble_wake_unlock(struct btmtk_dev *bdev)
{
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();

	if (bdev->bt_cfg.support_woble_wakelock) {
		BTMTK_INFO("%s: enter", __func__);
		__pm_relax(bmain_info->woble_ws);
		BTMTK_INFO("%s: exit", __func__);
	}
}

int btmtk_send_apcf_reserved(struct btmtk_dev *bdev)
{
	int ret = 0;

	if (bdev == NULL) {
		BTMTK_ERR("%s: Incorrect bdev", __func__);
		ret = -1;
		goto exit;
	}

	if (is_support_unify_woble(bdev)) {
		ret = btmtk_send_cmd_to_fw(bdev,
				RES_APCF_CMD, RES_APCF_EVT,
				0, 0, BTMTK_TX_PKT_FROM_HOST, CMD_NEED_FILTER);
		BTMTK_INFO("%s: ret %d", __func__, ret);
	} else
		BTMTK_WARN("%s: not support for 0x%x", __func__, bdev->chip_id);

exit:
	return ret;
}

static int btmtk_load_woble_setting(char *bin_name,
		struct device *dev, u32 *code_len, struct btmtk_woble *bt_woble)
{
	int err;
	struct btmtk_dev *bdev = bt_woble->bdev;

	*code_len = 0;

	err = btmtk_load_code_from_setting_files(bin_name, dev, code_len, bdev);
	if (err) {
		BTMTK_ERR("woble_setting btmtk_load_code_from_setting_files failed!!");
		goto LOAD_END;
	}

	err = btmtk_load_fw_cfg_setting("APCF",
			bt_woble->woble_setting_apcf, WOBLE_SETTING_COUNT, bdev->setting_file, FW_CFG_INX_LEN_2);
	if (err)
		goto LOAD_END;

	err = btmtk_load_fw_cfg_setting("APCF_ADD_MAC",
			bt_woble->woble_setting_apcf_fill_mac, WOBLE_SETTING_COUNT,
			bdev->setting_file, FW_CFG_INX_LEN_2);
	if (err)
		goto LOAD_END;

	err = btmtk_load_fw_cfg_setting("APCF_ADD_MAC_LOCATION",
			bt_woble->woble_setting_apcf_fill_mac_location, WOBLE_SETTING_COUNT,
			bdev->setting_file, FW_CFG_INX_LEN_2);
	if (err)
		goto LOAD_END;

	err = btmtk_load_fw_cfg_setting("RADIOOFF", &bt_woble->woble_setting_radio_off, 1,
			bdev->setting_file, FW_CFG_INX_LEN_2);
	if (err)
		goto LOAD_END;

	switch (bdev->bt_cfg.unify_woble_type) {
	case 0:
		err = btmtk_load_fw_cfg_setting("WAKEUP_TYPE_LEGACY", &bt_woble->woble_setting_wakeup_type, 1,
			bdev->setting_file, FW_CFG_INX_LEN_2);
		break;
	case 1:
		err = btmtk_load_fw_cfg_setting("WAKEUP_TYPE_WAVEFORM", &bt_woble->woble_setting_wakeup_type, 1,
			bdev->setting_file, FW_CFG_INX_LEN_2);
		break;
	case 2:
		err = btmtk_load_fw_cfg_setting("WAKEUP_TYPE_IR", &bt_woble->woble_setting_wakeup_type, 1,
			bdev->setting_file, FW_CFG_INX_LEN_2);
		break;
	default:
		BTMTK_WARN("%s: unify_woble_type unknown(%d)", __func__, bdev->bt_cfg.unify_woble_type);
	}
	if (err)
		BTMTK_WARN("%s: Parse unify_woble_type(%d) failed", __func__, bdev->bt_cfg.unify_woble_type);

	err = btmtk_load_fw_cfg_setting("RADIOOFF_STATUS_EVENT",
			&bt_woble->woble_setting_radio_off_status_event, 1, bdev->setting_file, FW_CFG_INX_LEN_2);
	if (err)
		goto LOAD_END;

	err = btmtk_load_fw_cfg_setting("RADIOOFF_COMPLETE_EVENT",
			&bt_woble->woble_setting_radio_off_comp_event, 1, bdev->setting_file, FW_CFG_INX_LEN_2);
	if (err)
		goto LOAD_END;

	err = btmtk_load_fw_cfg_setting("RADIOON",
			&bt_woble->woble_setting_radio_on, 1, bdev->setting_file, FW_CFG_INX_LEN_2);
	if (err)
		goto LOAD_END;

	err = btmtk_load_fw_cfg_setting("RADIOON_STATUS_EVENT",
			&bt_woble->woble_setting_radio_on_status_event, 1, bdev->setting_file, FW_CFG_INX_LEN_2);
	if (err)
		goto LOAD_END;

	err = btmtk_load_fw_cfg_setting("RADIOON_COMPLETE_EVENT",
			&bt_woble->woble_setting_radio_on_comp_event, 1, bdev->setting_file, FW_CFG_INX_LEN_2);
	if (err)
		goto LOAD_END;

	err = btmtk_load_fw_cfg_setting("APCF_RESUME",
			bt_woble->woble_setting_apcf_resume, WOBLE_SETTING_COUNT, bdev->setting_file, FW_CFG_INX_LEN_2);

LOAD_END:
	/* release setting file memory */
	if (bdev) {
		kfree(bdev->setting_file);
		bdev->setting_file = NULL;
	}

	if (err)
		BTMTK_ERR("%s: error return %d", __func__, err);

	return err;
}

#if WAKEUP_BT_IRQ
void btmtk_sdio_irq_wake_lock_timeout(struct btmtk_dev *bdev)
{
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();

	BTMTK_INFO("%s: enter", __func__);
	__pm_wakeup_event(bmain_info->irq_ws, WAIT_POWERKEY_TIMEOUT);
	BTMTK_INFO("%s: exit", __func__);
}
#else
static int btmtk_send_woble_read_BDADDR_cmd(struct btmtk_dev *bdev)
{
	int i;
	int ret = -1;

	BTMTK_INFO("%s: begin", __func__);
	if (bdev == NULL || bdev->io_buf == NULL) {
		BTMTK_ERR("%s: Incorrect bdev", __func__);
		return ret;
	}

	for (i = 0; i < BD_ADDRESS_SIZE; i++) {
		if (bdev->bdaddr[i] != 0) {
			ret = 0;
			goto done;
		}
	}
	ret = btmtk_send_cmd_to_fw(bdev,
			READ_ADDRESS_CMD, READ_ADDRESS_EVT,
			0, 0, BTMTK_TX_PKT_FROM_HOST, CMD_NO_NEED_FILTER);

	/*BD address will get in btmtk_rx_work*/
	if (ret < 0)
		BTMTK_ERR("%s: failed(%d)", __func__, ret);

done:
	BTMTK_INFO("%s, end, ret = %d", __func__, ret);
	return ret;
}


static int btmtk_send_unify_woble_suspend_default_cmd(struct btmtk_dev *bdev)
{
	int ret = 0;	/* if successful, 0 */

	BTMTK_INFO("%s: begin", __func__);
	ret = btmtk_send_cmd_to_fw(bdev,
		WOBLE_ENABLE_DEFAULT_CMD, WOBLE_ENABLE_DEFAULT_EVT,
		0, 0, BTMTK_TX_PKT_FROM_HOST, CMD_NEED_FILTER);
	if (ret < 0)
		BTMTK_ERR("%s: failed(%d)", __func__, ret);

	BTMTK_INFO("%s: end. ret = %d", __func__, ret);
	return ret;
}


static int btmtk_send_unify_woble_resume_default_cmd(struct btmtk_dev *bdev)
{
	int ret = 0;	/* if successful, 0 */

	BTMTK_INFO("%s: begin", __func__);
	ret = btmtk_send_cmd_to_fw(bdev,
		WOBLE_DISABLE_DEFAULT_CMD, WOBLE_DISABLE_DEFAULT_EVT,
		0, 0, BTMTK_TX_PKT_FROM_HOST, CMD_NEED_FILTER);
	if (ret < 0)
		BTMTK_ERR("%s: failed(%d)", __func__, ret);

	BTMTK_INFO("%s: end. ret = %d", __func__, ret);
	return ret;
}


static int btmtk_send_woble_suspend_cmd(struct btmtk_dev *bdev)
{
	/* radio off cmd with wobx_mode_disable, used when unify woble off */
	int ret = 0;	/* if successful, 0 */

	BTMTK_INFO("%s: not support woble, send radio off cmd", __func__);
	ret = btmtk_send_cmd_to_fw(bdev,
		RADIO_OFF_CMD, RADIO_OFF_EVT,
		0, 0, BTMTK_TX_PKT_FROM_HOST, CMD_NO_NEED_FILTER);
	if (ret < 0)
		BTMTK_ERR("%s: failed(%d)", __func__, ret);

	return ret;
}

static int btmtk_send_woble_resume_cmd(struct btmtk_dev *bdev)
{
	/* radio on cmd with wobx_mode_disable, used when unify woble off */
	int ret = 0;	/* if successful, 0 */

	BTMTK_INFO("%s: begin", __func__);
	ret = btmtk_send_cmd_to_fw(bdev,
		RADIO_ON_CMD, RADIO_ON_EVT,
		0, 0, BTMTK_TX_PKT_FROM_HOST, CMD_NO_NEED_FILTER);
	if (ret < 0)
		BTMTK_ERR("%s: failed(%d)", __func__, ret);

	return ret;
}

static int btmtk_set_Woble_APCF_filter_parameter(struct btmtk_dev *bdev)
{
	int ret = 0;

	BTMTK_INFO("%s: begin", __func__);
	ret = btmtk_send_cmd_to_fw(bdev,
		APCF_FILTER_CMD, APCF_FILTER_EVT,
		0, 0, BTMTK_TX_PKT_FROM_HOST, CMD_NO_NEED_FILTER);
	if (ret < 0)
		BTMTK_ERR("%s: end ret %d", __func__, ret);
	else
		ret = 0;

	BTMTK_INFO("%s: end ret=%d", __func__, ret);
	return ret;
}

/**
 * Set APCF manufacturer data and filter parameter
 */
static int btmtk_set_Woble_APCF(struct btmtk_woble *bt_woble)
{
	int ret = 0;
	u8 i = 0;
	struct btmtk_dev *bdev = bt_woble->bdev;
	struct data_struct cmd = {0}, event = {0};

	BTMTK_INFO("%s: woble_setting_apcf[0].length %d",
			__func__, bt_woble->woble_setting_apcf[0].length);

	BTMTK_GET_CMD_OR_EVENT_DATA(bt_woble->bdev, APCF_CMD, cmd);
	BTMTK_GET_CMD_OR_EVENT_DATA(bt_woble->bdev, APCF_EVT, event);

	/* start to send apcf cmd from woble setting file */
	if (bt_woble->woble_setting_apcf[0].length) {
		for (i = 0; i < WOBLE_SETTING_COUNT; i++) {
			if (!bt_woble->woble_setting_apcf[i].length)
				continue;

			BTMTK_INFO("%s: apcf_fill_mac[%d].content[0] = 0x%02x", __func__, i,
					bt_woble->woble_setting_apcf_fill_mac[i].content[0]);
			BTMTK_INFO("%s: apcf_fill_mac_location[%d].length = %d", __func__, i,
					bt_woble->woble_setting_apcf_fill_mac_location[i].length);

			if ((bt_woble->woble_setting_apcf_fill_mac[i].content[0] == 1) &&
				bt_woble->woble_setting_apcf_fill_mac_location[i].length) {
				/* need add BD addr to apcf cmd */
				memcpy(bt_woble->woble_setting_apcf[i].content +
					(*bt_woble->woble_setting_apcf_fill_mac_location[i].content + 1),
					bdev->bdaddr, BD_ADDRESS_SIZE);
				BTMTK_INFO("%s: apcf[%d], add local BDADDR to location %d", __func__, i,
						(*bt_woble->woble_setting_apcf_fill_mac_location[i].content));
			}
#if CFG_SHOW_FULL_MACADDR
			BTMTK_INFO_RAW(bt_woble->woble_setting_apcf[i].content, bt_woble->woble_setting_apcf[i].length,
				"Send woble_setting_apcf[%d] ", i);
#endif
			ret = btmtk_main_send_cmd(bdev, bt_woble->woble_setting_apcf[i].content,
				bt_woble->woble_setting_apcf[i].length, event.content, event.len, 0, 0,
				BTMTK_TX_PKT_FROM_HOST, CMD_NO_NEED_FILTER);
			if (ret < 0) {
				BTMTK_ERR("%s: manufactur_data error ret %d", __func__, ret);
				return ret;
			}
		}
	} else { /* use default */
		BTMTK_INFO("%s: use default manufactur data", __func__);
		memcpy(cmd.content + 10, bdev->bdaddr, BD_ADDRESS_SIZE);
		ret = btmtk_main_send_cmd(bdev, cmd.content, cmd.len,
			event.content, event.len, 0, 0, BTMTK_TX_PKT_FROM_HOST, CMD_NO_NEED_FILTER);
		if (ret < 0) {
			BTMTK_ERR("%s: manufactur_data error ret %d", __func__, ret);
			return ret;
		}

		ret = btmtk_set_Woble_APCF_filter_parameter(bdev);
	}

	BTMTK_INFO("%s: end ret=%d", __func__, ret);
	return 0;
}

static void btmtk_remove_APCF_Filter_Index(char *radio_off, int length,
	struct btmtk_woble *bt_woble)
{
	u8 i = 0;
	u8 j = 0;
	unsigned char filter_index = 0;

	if (bt_woble->woble_setting_apcf[0].length) {
		for (i = 0; i < WOBLE_SETTING_COUNT; i++) {
			if (bt_woble->woble_setting_apcf[i].content[4] == 0x06) {
				filter_index = bt_woble->woble_setting_apcf[i].content[6];
				break;
			}
		}
	}
	for (i = 6; i < length; i++) {
		if (radio_off[i+1] == 0x40 &&
			radio_off[i+2] == filter_index) {
			radio_off[i] = 0x02;
			for (j = i+3; j < length; j++)
				radio_off[j] = radio_off[j+1];
			break;
		} else if (radio_off[i+1] == 0x40 &&
			radio_off[i+3] == filter_index) {
			radio_off[i] = 0x02;
			for (j = i+2; j < length; j++)
				radio_off[j] = radio_off[j+1];
			break;
		}
		i += radio_off[i];
		continue;
	}
	radio_off[3] = radio_off[3] - 1;
}


static int btmtk_set_Woble_Radio_Off(struct btmtk_woble *bt_woble)
{
	int ret = 0;
	int length = 0;
	unsigned char fstate = 0;
	unsigned char state = 0;
	char *radio_off = NULL;
	struct btmtk_dev *bdev = bt_woble->bdev;

	BTMTK_INFO("%s: woble_setting_radio_off.length %d", __func__,
		bt_woble->woble_setting_radio_off.length);

	fstate = btmtk_fops_get_state(bdev);
	state = btmtk_get_chip_state(bdev);

	if (bt_woble->woble_setting_radio_off.length) {
		/* start to send radio off cmd from woble setting file */
		length = bt_woble->woble_setting_radio_off.length +
				bt_woble->woble_setting_wakeup_type.length;
		radio_off = kzalloc(length, GFP_KERNEL);
		if (!radio_off) {
			BTMTK_ERR("%s: alloc memory fail (radio_off)",
				__func__);
			ret = -ENOMEM;
			goto Finish;
		}

		memcpy(radio_off,
			bt_woble->woble_setting_radio_off.content,
			bt_woble->woble_setting_radio_off.length);
		if (bt_woble->woble_setting_wakeup_type.length) {
			memcpy(radio_off + bt_woble->woble_setting_radio_off.length,
				bt_woble->woble_setting_wakeup_type.content,
				bt_woble->woble_setting_wakeup_type.length);
			radio_off[3] += bt_woble->woble_setting_wakeup_type.length;
		}

		/* Don't send G20 apcf index when BT off with G20 to do suspend. Because when
		* the power key is pressed at the first time, the platform will be woken up by the
		* wake-up advertisment. G20 will send wake-up adv when rc disconnected, but
		* AB1613 will not send wake-up adv. AB1613 APCF manu data type:0x06, G20 service
		* data type:0x07
		*/
		if ((fstate == BTMTK_FOPS_STATE_CLOSED ||
			fstate == BTMTK_FOPS_STATE_INIT) && state == BTMTK_STATE_SUSPEND) {
			length = length - 1;
			btmtk_remove_APCF_Filter_Index(radio_off, length, bt_woble);
		}

		BTMTK_INFO_RAW(radio_off, length, "Send radio off");
		ret = btmtk_main_send_cmd(bdev, radio_off, length,
			bt_woble->woble_setting_radio_off_comp_event.content,
			bt_woble->woble_setting_radio_off_comp_event.length, 0, 0,
			BTMTK_TX_PKT_FROM_HOST, CMD_NEED_FILTER);

		kfree(radio_off);
		radio_off = NULL;
	} else { /* use default */
		BTMTK_INFO("%s: use default radio off cmd", __func__);
		ret = btmtk_send_unify_woble_suspend_default_cmd(bdev);
	}

Finish:
	BTMTK_INFO("%s, end ret=%d", __func__, ret);
	return ret;
}

static int btmtk_set_Woble_Radio_On(struct btmtk_woble *bt_woble)
{
	int ret = 0;
	struct btmtk_dev *bdev = bt_woble->bdev;

	BTMTK_INFO("%s: woble_setting_radio_on.length %d", __func__,
		bt_woble->woble_setting_radio_on.length);
	if (bt_woble->woble_setting_radio_on.length) {
		/* start to send radio on cmd from woble setting file */
		BTMTK_INFO_RAW(bt_woble->woble_setting_radio_on.content,
			bt_woble->woble_setting_radio_on.length, "send radio on");

		ret = btmtk_main_send_cmd(bdev, bt_woble->woble_setting_radio_on.content,
			bt_woble->woble_setting_radio_on.length,
			bt_woble->woble_setting_radio_on_comp_event.content,
			bt_woble->woble_setting_radio_on_comp_event.length, 0, 0,
			BTMTK_TX_PKT_FROM_HOST, CMD_NEED_FILTER);
	} else { /* use default */
		BTMTK_WARN("%s: use default radio on cmd", __func__);
		ret = btmtk_send_unify_woble_resume_default_cmd(bdev);
	}

	BTMTK_INFO("%s, end ret=%d", __func__, ret);
	return ret;
}

static int btmtk_del_Woble_APCF_index(struct btmtk_dev *bdev)
{
	int ret = 0;

	BTMTK_INFO("%s, enter", __func__);
	ret = btmtk_send_cmd_to_fw(bdev,
		APCF_DELETE_CMD, APCF_DELETE_EVT,
		0, 0, BTMTK_TX_PKT_FROM_HOST, CMD_NO_NEED_FILTER);
	if (ret < 0)
		BTMTK_ERR("%s: got error %d", __func__, ret);

	BTMTK_INFO("%s, end", __func__);
	return ret;
}


static int btmtk_set_Woble_APCF_Resume(struct btmtk_woble *bt_woble)
{
	struct data_struct event = {0};
	u8 i = 0;
	int ret = -1;
	struct btmtk_dev *bdev = bt_woble->bdev;

	BTMTK_INFO("%s, enter, bt_woble->woble_setting_apcf_resume[0].length= %d",
			__func__, bt_woble->woble_setting_apcf_resume[0].length);
	BTMTK_GET_CMD_OR_EVENT_DATA(bdev, APCF_RESUME_EVT, event);

	if (bt_woble->woble_setting_apcf_resume[0].length) {
		BTMTK_INFO("%s: handle leave woble apcf from file", __func__);
		for (i = 0; i < WOBLE_SETTING_COUNT; i++) {
			if (!bt_woble->woble_setting_apcf_resume[i].length)
				continue;

			BTMTK_INFO_RAW(bt_woble->woble_setting_apcf_resume[i].content,
				bt_woble->woble_setting_apcf_resume[i].length,
				"%s: send apcf resume %d:", __func__, i);

			ret = btmtk_main_send_cmd(bdev,
				bt_woble->woble_setting_apcf_resume[i].content,
				bt_woble->woble_setting_apcf_resume[i].length,
				event.content, event.len,
				0, 0, BTMTK_TX_PKT_FROM_HOST, CMD_NO_NEED_FILTER);
			if (ret < 0) {
				BTMTK_ERR("%s: Send apcf resume fail %d", __func__, ret);
				return ret;
			}
		}
	} else { /* use default */
		BTMTK_WARN("%s: use default apcf resume cmd", __func__);
		ret = btmtk_del_Woble_APCF_index(bdev);
		if (ret < 0)
			BTMTK_ERR("%s: btmtk_del_Woble_APCF_index return fail %d", __func__, ret);
	}
	BTMTK_INFO("%s, end", __func__);
	return ret;
}

static void btmtk_check_wobx_debug_log(struct btmtk_dev *bdev)
{
	int ret = 0;

	BTMTK_INFO("%s: begin", __func__);

	ret = btmtk_send_cmd_to_fw(bdev,
		CHECK_WOBX_DEBUG_CMD, CHECK_WOBX_DEBUG_EVT,
		0, 0, BTMTK_TX_PKT_FROM_HOST, CMD_NEED_FILTER);
	if (ret < 0)
		BTMTK_ERR("%s: failed(%d)", __func__, ret);

	/* Driver just print event to kernel log in rx_work,
	 * Please reference wiki to know what it is.
	 */
}
#endif

static int btmtk_handle_leaving_WoBLE_state(struct btmtk_woble *bt_woble)
{
#if !WAKEUP_BT_IRQ
	int ret = 0;
	unsigned char fstate = 0;
	struct btmtk_dev *bdev = bt_woble->bdev;
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();
#endif

	BTMTK_INFO("%s: begin", __func__);

#if WAKEUP_BT_IRQ
	/* Can't enter woble mode */
	BTMTK_INFO("not support woble mode for wakeup bt irq");
	return 0;
#else
	fstate = btmtk_fops_get_state(bdev);
	if (!bdev->bt_cfg.support_woble_for_bt_disable) {
		if (fstate != BTMTK_FOPS_STATE_OPENED) {
			BTMTK_WARN("%s: fops is not opened, return", __func__);
			return 0;
		}
	}

	if (fstate != BTMTK_FOPS_STATE_OPENED) {
		BTMTK_WARN("%s: fops is not open yet(%d), need to start traffic before leaving woble",
				__func__, fstate);
		/* start traffic to recv event*/
		ret = bmain_info->hif_hook.open(bdev->hdev);
		if (ret < 0) {
			BTMTK_ERR("%s, cif_open failed", __func__);
			goto Finish;
		}
	}

	if (is_support_unify_woble(bdev)) {
		ret = btmtk_set_Woble_Radio_On(bt_woble);
		if (ret < 0)
			goto Finish;

		ret = btmtk_set_Woble_APCF_Resume(bt_woble);
		if (ret < 0)
			goto Finish;
	} else {
		/* radio on cmd with wobx_mode_disable, used when unify woble off */
		ret = btmtk_send_woble_resume_cmd(bdev);
	}

Finish:
	if (ret < 0) {
		BTMTK_INFO("%s: woble_resume_fail!!!", __func__);
	} else {
		/* It's wobx debug log method. */
		btmtk_check_wobx_debug_log(bdev);

		if (fstate != BTMTK_FOPS_STATE_OPENED) {
			ret = btmtk_send_deinit_cmds(bdev);
			if (ret < 0) {
				BTMTK_ERR("%s, btmtk_send_deinit_cmds failed", __func__);
				goto exit;
			}

			BTMTK_WARN("%s: fops is not open(%d), need to stop traffic after leaving woble",
					__func__, fstate);
			/* stop traffic to stop recv data from fw*/
			ret = bmain_info->hif_hook.close(bdev->hdev);
			if (ret < 0) {
				BTMTK_ERR("%s, cif_close failed", __func__);
				goto exit;
			}
		} else
			bdev->power_state = BTMTK_DONGLE_STATE_POWER_ON;
		BTMTK_INFO("%s: success", __func__);
	}

exit:
	BTMTK_INFO("%s: end", __func__);
	return ret;
#endif
}

static int ATTRIBUTE_NO_KCFI btmtk_handle_entering_WoBLE_state(struct btmtk_woble *bt_woble)
{
#if !WAKEUP_BT_IRQ
	int ret = 0;
	unsigned char fstate = 0;
	int state = 0;
	int send_fail = 0;
	int retry = 3;
	struct btmtk_dev *bdev = bt_woble->bdev;
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();
#endif

	BTMTK_INFO("%s: begin", __func__);
#if WAKEUP_BT_IRQ
	/* Can't enter woble mode */
	BTMTK_INFO("not support woble mode for wakeup bt irq");
	return 0;
#else
	fstate = btmtk_fops_get_state(bdev);
	if (!bdev->bt_cfg.support_woble_for_bt_disable) {
		if (fstate != BTMTK_FOPS_STATE_OPENED) {
			BTMTK_WARN("%s: fops is not open yet(%d)!, return", __func__, fstate);
			return 0;
		}
	}

	state = btmtk_get_chip_state(bdev);
	if (state == BTMTK_STATE_FW_DUMP) {
		BTMTK_WARN("%s: FW dumping ongoing, don't send any cmd to FW!!!", __func__);
		goto Finish;
	}

	if (atomic_read(&bmain_info->chip_reset) || atomic_read(&bmain_info->subsys_reset)) {
		BTMTK_ERR("%s chip_reset is %d, subsys_reset is %d", __func__,
			atomic_read(&bmain_info->chip_reset), atomic_read(&bmain_info->subsys_reset));
		goto Finish;
	}

SEND_CMD_RETRY:
	/* Power on first if state is power off */
	ret = btmtk_reset_power_on(bdev);
	if (ret < 0) {
		BTMTK_ERR("%s: reset power_on fail return", __func__);
		goto Finish;
	}

	if (fstate != BTMTK_FOPS_STATE_OPENED) {
		BTMTK_WARN("%s: fops is not open yet(%d), need to start traffic before enter woble",
				__func__, fstate);
		/* start traffic to recv event*/
		ret = bmain_info->hif_hook.open(bdev->hdev);
		if (ret < 0 && ret != -EBUSY) {
			BTMTK_ERR("%s, cif_open failed", __func__);
			goto Finish;
		}
	}
	if (is_support_unify_woble(bdev)) {
#if (CFG_GKI_SUPPORT == 0)
		do {
			typedef ssize_t (*func) (u16 u16Key, const char *buf, size_t size);
			char *func_name = "MDrv_PM_Write_Key";
			func pFunc = NULL;
			ssize_t sret = 0;
			u8 buf = 0;

			pFunc = (func) btmtk_kallsyms_lookup_name(func_name);
			if (pFunc && bdev->bt_cfg.unify_woble_type == 1) {
				buf = 1;
				sret = pFunc(PM_KEY_BTW, &buf, sizeof(u8));
				BTMTK_INFO("%s: Invoke %s, buf = %d, sret = %zd", __func__,
					func_name, buf, sret);

			} else {
				BTMTK_WARN("%s: No Exported Func Found [%s]", __func__, func_name);
			}
		} while (0);
#endif
		ret = btmtk_send_woble_read_BDADDR_cmd(bdev);
		if (ret < 0) {
			send_fail = 1;
			goto STOP_TRAFFIC;
		}

		ret = btmtk_set_Woble_APCF(bt_woble);
		if (ret < 0) {
			send_fail = 1;
			goto STOP_TRAFFIC;
		}

		ret = btmtk_set_Woble_Radio_Off(bt_woble);
		if (ret < 0) {
			send_fail = 1;
			goto STOP_TRAFFIC;
		}
	} else {
		/* radio off cmd with wobx_mode_disable, used when unify woble off */
		ret = btmtk_send_woble_suspend_cmd(bdev);
	}

STOP_TRAFFIC:
	if (send_fail == 1 && retry > 0) {
		BTMTK_INFO("%s: woble , reset done flag = %d, send_fail flag = %d", __func__, test_bit(BT_WOBLE_RESET_DONE, &bdev->flags), send_fail);
		btmtk_handle_mutex_unlock(bdev);

		set_bit(BT_WOBLE_FAIL_FOR_SUBSYS_RESET, &bdev->flags);
		bmain_info->chip_reset_flag = 0;
		send_fail = 0;
		retry --;
		btmtk_reset_trigger(bdev);
		wait_event_timeout(bdev->p_woble_fail_q, test_bit(BT_WOBLE_RESET_DONE, &bdev->flags),
				msecs_to_jiffies(10000));

		btmtk_handle_mutex_lock(bdev);

		clear_bit(BT_WOBLE_FAIL_FOR_SUBSYS_RESET, &bdev->flags);
		if (!test_bit(BT_WOBLE_RESET_DONE, &bdev->flags)) {
			BTMTK_ERR("%s: wait event timeout", __func__);
			goto Finish;
		}
		clear_bit(BT_WOBLE_RESET_DONE, &bdev->flags);
		if (test_bit(BT_WOBLE_SKIP_WHOLE_CHIP_RESET, &bdev->flags)) {
			clear_bit(BT_WOBLE_SKIP_WHOLE_CHIP_RESET, &bdev->flags);
			BTMTK_ERR("%s: L0.5 fail, return", __func__);
			goto Finish;
		}
		BTMTK_INFO("%s: try send cmd, retry = %d", __func__, retry);
		goto SEND_CMD_RETRY;

	}
	if (fstate != BTMTK_FOPS_STATE_OPENED) {
		BTMTK_WARN("%s: fops is not open(%d), need to stop traffic after enter woble",
				__func__, fstate);
		/* stop traffic to stop recv data from fw*/
		ret = bmain_info->hif_hook.close(bdev->hdev);
		if (ret < 0) {
			BTMTK_ERR("%s, cif_close failed", __func__);
			goto Finish;
		}
	}

Finish:
	if (ret) {
		bdev->power_state = BTMTK_DONGLE_STATE_ERROR;
		btmtk_woble_wake_lock(bdev);
	}

	BTMTK_INFO("%s: end ret = %d, power_state =%d", __func__, ret, bdev->power_state);
	return ret;
#endif
}

int btmtk_woble_suspend(struct btmtk_woble *bt_woble)
{
	int ret = 0;
	unsigned char fstate = 0;
	struct btmtk_dev *bdev = NULL;
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();
	struct irq_desc *desc = NULL;

	UNUSED(bmain_info);
	BTMTK_INFO("%s: enter", __func__);

	if (!bt_woble) {
		BTMTK_WARN("%s: invalid parameters!", __func__);
		goto exit;
	}

	bdev = bt_woble->bdev;
	if (!bdev) {
		BTMTK_INFO("%s: bdev is NULL", __func__);
		return 0;
	}
	fstate = btmtk_fops_get_state(bdev);

#if 0 // (USE_DEVICE_NODE == 0)
	if (bdev->bt_cfg.support_auto_picus == true &&
		(bdev->bt_cfg.support_picus_to_host == true || atomic_read(&bmain_info->fwlog_ref_cnt) != 0)) {
		btmtk_picus_enable(bdev, 1);
	}
#endif

	if (!is_support_unify_woble(bdev) && (fstate != BTMTK_FOPS_STATE_OPENED)) {
		BTMTK_WARN("%s: when not support woble, in bt off state, do nothing!", __func__);
		goto exit;
	}

	ret = btmtk_handle_entering_WoBLE_state(bt_woble);
	if (ret)
		BTMTK_ERR("%s: btmtk_handle_entering_WoBLE_state return fail %d", __func__, ret);

	if (bdev->bt_cfg.support_woble_by_eint) {
		if (bt_woble->wobt_irq != 0 && atomic_read(&(bt_woble->irq_enable_count)) == 0) {
			/* clear irq data before enable woble irq to avoid DUT wake up
			 * automatically by edge trigger, sync from Jira CONN-50629
			 */
#if (KERNEL_VERSION(5, 11, 0) > LINUX_VERSION_CODE)
			desc = irq_to_desc(bt_woble->wobt_irq);
#else
			if (irq_get_irq_data(bt_woble->wobt_irq))
				desc = irq_data_to_desc(irq_get_irq_data(bt_woble->wobt_irq));
			else
				BTMTK_INFO("%s:irq_get_irq_data return is NULL", __func__);
#endif
			if (desc)
				desc->irq_data.chip->irq_ack(&desc->irq_data);
			else
				BTMTK_INFO("%s:can't get desc", __func__);

			BTMTK_INFO("%s, enable BT IRQ:%d", __func__, bt_woble->wobt_irq);
			irq_set_irq_wake(bt_woble->wobt_irq, 1);
			enable_irq(bt_woble->wobt_irq);
			atomic_inc(&(bt_woble->irq_enable_count));
		} else
			BTMTK_INFO("%s, irq_enable count:%d", __func__, atomic_read(&(bt_woble->irq_enable_count)));
	}

exit:
	BTMTK_INFO("%s: end", __func__);
	return ret;
}

int btmtk_woble_resume(struct btmtk_woble *bt_woble)
{
	int ret = 0;
	unsigned char fstate = 0;
	struct btmtk_dev *bdev = NULL;
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();

	BTMTK_INFO("%s: enter", __func__);

	if (!bt_woble) {
		BTMTK_ERR("%s: invalid parameters!", __func__);
		goto exit;
	}

	bdev = bt_woble->bdev;
	if (!bdev) {
		BTMTK_INFO("%s: bdev is NULL", __func__);
		return 0;
	}

	fstate = btmtk_fops_get_state(bdev);

	if (!is_support_unify_woble(bdev) && (fstate != BTMTK_FOPS_STATE_OPENED)) {
		BTMTK_WARN("%s: when not support woble, in bt off state, do nothing!", __func__);
		goto exit;
	}

	if (bdev->power_state == BTMTK_DONGLE_STATE_ERROR) {
		BTMTK_INFO("%s: In BTMTK_DONGLE_STATE_ERROR(Could suspend caused), do assert", __func__);
		btmtk_send_assert_cmd(bdev);
		ret = -EBADFD;
		goto exit;
	}

	ret = btmtk_handle_leaving_WoBLE_state(bt_woble);
	if (ret < 0) {
		BTMTK_ERR("%s: btmtk_handle_leaving_WoBLE_state return fail %d", __func__, ret);
		/* avoid rtc to to suspend again, do FW dump first */
		btmtk_woble_wake_lock(bdev);
		goto exit;
	}

#if 0 //(USE_DEVICE_NODE == 0)
	if (bdev->bt_cfg.support_auto_picus == true &&
		(bdev->bt_cfg.support_picus_to_host == true || atomic_read(&bmain_info->fwlog_ref_cnt) != 0)) {
		btmtk_picus_enable(bdev, 0);
	}
#endif

	if (bdev->bt_cfg.reset_stack_after_woble
		&& bmain_info->reset_stack_flag == HW_ERR_NONE
		&& fstate == BTMTK_FOPS_STATE_OPENED)
		bmain_info->reset_stack_flag = HW_ERR_CODE_RESET_STACK_AFTER_WOBLE;

	(void)btmtk_send_hw_err_to_host(bdev);
	BTMTK_INFO("%s: end(%d), reset_stack_flag = %d, fstate = %d", __func__, ret,
			bmain_info->reset_stack_flag, fstate);

exit:
	BTMTK_INFO("%s: end", __func__);
	return ret;
}

static irqreturn_t btmtk_woble_isr(int irq, struct btmtk_woble *bt_woble)
{
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();

	BTMTK_DBG("%s begin", __func__);
	disable_irq_nosync(bt_woble->wobt_irq);
	atomic_dec(&(bt_woble->irq_enable_count));
	BTMTK_INFO("disable BT IRQ, call wake lock");
	__pm_wakeup_event(bmain_info->eint_ws, WAIT_POWERKEY_TIMEOUT);

	input_report_key(bt_woble->WoBLEInputDev, KEY_WAKEUP, 1);
	input_sync(bt_woble->WoBLEInputDev);
	input_report_key(bt_woble->WoBLEInputDev, KEY_WAKEUP, 0);
	input_sync(bt_woble->WoBLEInputDev);
	BTMTK_DBG("%s end", __func__);
	return IRQ_HANDLED;
}

static int btmtk_RegisterBTIrq(struct btmtk_woble *bt_woble)
{
	struct device_node *eint_node = NULL;
	int interrupts[2];

	BTMTK_DBG("%s begin", __func__);
	eint_node = of_find_compatible_node(NULL, NULL, "mediatek,woble_eint");
	if (eint_node) {
		BTMTK_INFO("Get woble_eint compatible node");
		bt_woble->wobt_irq = irq_of_parse_and_map(eint_node, 0);
		BTMTK_INFO("woble_irq number:%d", bt_woble->wobt_irq);
		if (bt_woble->wobt_irq) {
			(void)of_property_read_u32_array(eint_node, "interrupts",
						   interrupts, ARRAY_SIZE(interrupts));
			bt_woble->wobt_irqlevel = interrupts[1];
			if (request_irq(bt_woble->wobt_irq, (void *)btmtk_woble_isr,
					bt_woble->wobt_irqlevel, "woble-eint", bt_woble))
				BTMTK_INFO("WOBTIRQ LINE NOT AVAILABLE!!");
			else {
				BTMTK_INFO("disable BT IRQ");
				disable_irq_nosync(bt_woble->wobt_irq);
			}

		} else
			BTMTK_INFO("can't find woble_eint irq");

	} else {
		bt_woble->wobt_irq = 0;
		BTMTK_INFO("can't find woble_eint compatible node");
	}

	BTMTK_DBG("%s end", __func__);
	return 0;
}

static int btmtk_woble_input_init(struct btmtk_woble *bt_woble)
{
	int ret = 0;

	bt_woble->WoBLEInputDev = input_allocate_device();
	if (!bt_woble->WoBLEInputDev || IS_ERR(bt_woble->WoBLEInputDev)) {
		BTMTK_ERR("input_allocate_device error");
		return -ENOMEM;
	}

	bt_woble->WoBLEInputDev->name = "WOBLE_INPUT_DEVICE";
	bt_woble->WoBLEInputDev->id.bustype = BUS_HOST;
	bt_woble->WoBLEInputDev->id.vendor = 0x0002;
	bt_woble->WoBLEInputDev->id.product = 0x0002;
	bt_woble->WoBLEInputDev->id.version = 0x0002;

	__set_bit(EV_KEY, bt_woble->WoBLEInputDev->evbit);
	__set_bit(KEY_WAKEUP, bt_woble->WoBLEInputDev->keybit);

	ret = input_register_device(bt_woble->WoBLEInputDev);
	if (ret < 0) {
		input_free_device(bt_woble->WoBLEInputDev);
		BTMTK_ERR("input_register_device %d", ret);
		return ret;
	}

	return ret;
}

static void btmtk_woble_input_deinit(struct btmtk_woble *bt_woble)
{
	if (bt_woble->WoBLEInputDev) {
		input_unregister_device(bt_woble->WoBLEInputDev);
		/* Do not need to free WOBLE_INPUT_DEVICE, because after unregister it,
		 * kernel will free it by itself.
		 */
		/* input_free_device(bt_woble->WoBLEInputDev); */
		bt_woble->WoBLEInputDev = NULL;
	}
}

static void btmtk_free_woble_setting_file(struct btmtk_woble *bt_woble)
{
	btmtk_free_fw_cfg_struct(bt_woble->woble_setting_apcf, WOBLE_SETTING_COUNT);
	btmtk_free_fw_cfg_struct(bt_woble->woble_setting_apcf_fill_mac, WOBLE_SETTING_COUNT);
	btmtk_free_fw_cfg_struct(bt_woble->woble_setting_apcf_fill_mac_location, WOBLE_SETTING_COUNT);
	btmtk_free_fw_cfg_struct(bt_woble->woble_setting_apcf_resume, WOBLE_SETTING_COUNT);
	btmtk_free_fw_cfg_struct(&bt_woble->woble_setting_radio_off, 1);
	btmtk_free_fw_cfg_struct(&bt_woble->woble_setting_radio_off_status_event, 1);
	btmtk_free_fw_cfg_struct(&bt_woble->woble_setting_radio_off_comp_event, 1);
	btmtk_free_fw_cfg_struct(&bt_woble->woble_setting_radio_on, 1);
	btmtk_free_fw_cfg_struct(&bt_woble->woble_setting_radio_on_status_event, 1);
	btmtk_free_fw_cfg_struct(&bt_woble->woble_setting_radio_on_comp_event, 1);
	btmtk_free_fw_cfg_struct(&bt_woble->woble_setting_wakeup_type, 1);

	bt_woble->woble_setting_len = 0;

	kfree(bt_woble->woble_setting_file_name);
	bt_woble->woble_setting_file_name = NULL;
}

int btmtk_woble_initialize(struct btmtk_dev *bdev, struct btmtk_woble *bt_woble)
{
	int err = 0;
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();

	bt_woble->bdev = bdev;
	/* Need to add Woble flow */
	if (is_support_unify_woble(bdev)) {
		if (bt_woble->woble_setting_file_name == NULL) {
			bt_woble->woble_setting_file_name = kzalloc(MAX_BIN_FILE_NAME_LEN, GFP_KERNEL);
			if (!bt_woble->woble_setting_file_name) {
				BTMTK_ERR("%s: alloc memory fail (bt_woble->woble_setting_file_name)", __func__);
				err = -1;
				goto end;
			}
		}

		(void)snprintf(bt_woble->woble_setting_file_name, MAX_BIN_FILE_NAME_LEN,
				"%s_%x.%s", WOBLE_CFG_NAME_PREFIX, bdev->chip_id & 0xffff,
				WOBLE_CFG_NAME_SUFFIX);

		BTMTK_INFO("%s: woble setting file name is %s", __func__, bt_woble->woble_setting_file_name);

		btmtk_load_woble_setting(bt_woble->woble_setting_file_name,
			bdev->intf_dev,
			&bt_woble->woble_setting_len,
			bt_woble);
		/* if reset_stack is true, when chip reset is done, we need to power on chip to do
		 * reset stack
		 */
		if (bmain_info->reset_stack_flag) {
			err = btmtk_reset_power_on(bdev);
			if (err < 0) {
				BTMTK_ERR("reset power on failed!");
				goto err;
			}
		}
	}

	if (bdev->bt_cfg.support_woble_by_eint) {
		btmtk_woble_input_init(bt_woble);
		btmtk_RegisterBTIrq(bt_woble);
	}

	return 0;

err:
	btmtk_free_woble_setting_file(bt_woble);
end:
	return err;
}

void btmtk_woble_uninitialize(struct btmtk_woble *bt_woble)
{
	struct btmtk_dev *bdev = bt_woble->bdev;

	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev == NULL", __func__);
		return;
	}

	BTMTK_INFO("%s begin", __func__);
	if (bdev->bt_cfg.support_woble_by_eint) {
		if (bt_woble->wobt_irq != 0 && atomic_read(&(bt_woble->irq_enable_count)) == 1) {
			BTMTK_INFO("disable BT IRQ:%d", bt_woble->wobt_irq);
			atomic_dec(&(bt_woble->irq_enable_count));
			disable_irq_nosync(bt_woble->wobt_irq);
		} else
			BTMTK_INFO("irq_enable count:%d", atomic_read(&(bt_woble->irq_enable_count)));
		if (bt_woble->wobt_irq) {
			free_irq(bt_woble->wobt_irq, bt_woble);
			bt_woble->wobt_irq = 0;
		}

		btmtk_woble_input_deinit(bt_woble);
	}

	btmtk_free_woble_setting_file(bt_woble);
	bt_woble->bdev = NULL;
}
