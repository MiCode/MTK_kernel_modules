// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "precomp.h"

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
static struct FW_LOG_INFO *fw_log_info;

uint8_t *fw_log_type_to_str(enum ENUM_FW_LOG_CTRL_TYPE type)
{
	switch (type) {
	case ENUM_FW_LOG_CTRL_TYPE_MCU:
		return "WF_MCU";
	case ENUM_FW_LOG_CTRL_TYPE_WIFI:
		return "WF_WM";
	default:
		return "UNKNOWN";
	}
}

uint32_t fw_log_init(struct ADAPTER *ad)
{
	struct FW_LOG_INFO *fw_log = NULL;
	uint32_t status = WLAN_STATUS_SUCCESS;

	if (!ad ||
	    !ad->chip_info)
		return 0;

	fw_log = &ad->chip_info->fw_log_info;
	fw_log_info = fw_log;

	if (fw_log->ops &&
	    fw_log->ops->init)
		status = fw_log->ops->init(ad);

	return status;
}

void fw_log_deinit(struct ADAPTER *ad)
{
	struct FW_LOG_INFO *fw_log = NULL;

	if (!ad ||
	    !ad->chip_info)
		return;

	fw_log = &ad->chip_info->fw_log_info;
	if (fw_log->ops &&
	    fw_log->ops->deinit)
		fw_log->ops->deinit(ad);

	fw_log_info = NULL;
}

uint32_t fw_log_start(struct ADAPTER *ad)
{
	struct FW_LOG_INFO *fw_log = NULL;

	if (!ad ||
	    !ad->chip_info)
		return 0;

	fw_log = &ad->chip_info->fw_log_info;
	if (!fw_log->ops ||
	    !fw_log->ops->start)
		return 0;

	return fw_log->ops->start(ad);
}

void fw_log_stop(struct ADAPTER *ad)
{
	struct FW_LOG_INFO *fw_log = NULL;

	if (!ad ||
	    !ad->chip_info)
		return;

	fw_log = &ad->chip_info->fw_log_info;
	if (!fw_log->ops ||
	    !fw_log->ops->stop)
		return;

	fw_log->ops->stop(ad);
}

void fw_log_set_enabled(struct ADAPTER *ad, u_int8_t enabled)
{
	struct FW_LOG_INFO *fw_log = NULL;

	if (!ad ||
	    !ad->chip_info)
		return;

	fw_log = &ad->chip_info->fw_log_info;
	if (!fw_log->ops ||
	    !fw_log->ops->set_enabled)
		return;

	fw_log->ops->set_enabled(ad, enabled);
}

int32_t fw_log_handler(void)
{
	if (!fw_log_info ||
	    !fw_log_info->ops ||
	    !fw_log_info->ops->handler)
		return 0;

	return fw_log_info->ops->handler();
}

