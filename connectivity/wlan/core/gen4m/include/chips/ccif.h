/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _CCIF_H
#define _CCIF_H

enum CCIF_MSG {
	SW_INT_FW_LOG = 0,
	SW_INT_SUBSYS_RESET,
	SW_INT_WHOLE_RESET,
	/* CH3 for 6877: WFDMA, 6653:PMIC */
	SW_INT_SW_WFDMA,
	SW_INT_PMIC_RESET = 3,
	SW_INT_TIME_SYNC,
};

struct CCIF_OPS {
	uint32_t (*get_interrupt_status)(struct ADAPTER *ad);
	void (*notify_utc_time_to_fw)(struct ADAPTER *ad,
		uint32_t sec,
		uint32_t usec);
	void (*set_fw_log_read_pointer)(struct ADAPTER *ad,
		enum ENUM_FW_LOG_CTRL_TYPE type,
		uint32_t read_pointer);
	uint32_t (*get_fw_log_read_pointer)(struct ADAPTER *ad,
		enum ENUM_FW_LOG_CTRL_TYPE type);
	int32_t (*trigger_fw_assert)(struct ADAPTER *ad);
};

static inline uint32_t ccif_get_interrupt_status(struct ADAPTER *ad)
{
	if (!ad || !ad->chip_info ||
	    !ad->chip_info->ccif_ops ||
	    !ad->chip_info->ccif_ops->get_interrupt_status)
		return 0;

	return ad->chip_info->ccif_ops->get_interrupt_status(ad);
}

static inline void ccif_notify_utc_time_to_fw(struct ADAPTER *ad,
	uint32_t sec,
	uint32_t usec)
{
	if (!ad || !ad->chip_info ||
	    !ad->chip_info->ccif_ops ||
	    !ad->chip_info->ccif_ops->notify_utc_time_to_fw)
		return;

	ad->chip_info->ccif_ops->notify_utc_time_to_fw(ad, sec, usec);
}

static inline void ccif_set_fw_log_read_pointer(struct ADAPTER *ad,
	enum ENUM_FW_LOG_CTRL_TYPE type,
	uint32_t read_pointer)
{
	struct mt66xx_chip_info *prChipInfo = NULL;

	if (!ad)
		return;

	prChipInfo = ad->chip_info;

	if (prChipInfo && prChipInfo->ccif_ops &&
	    prChipInfo->ccif_ops->set_fw_log_read_pointer)
		prChipInfo->ccif_ops->set_fw_log_read_pointer(ad,
			type,
			read_pointer);
}

static inline uint32_t ccif_get_fw_log_read_pointer(struct ADAPTER *ad,
	enum ENUM_FW_LOG_CTRL_TYPE type)
{
	struct mt66xx_chip_info *prChipInfo = NULL;
	uint32_t u4Rp = 0;

	if (!ad)
		goto exit;

	prChipInfo = ad->chip_info;

	if (prChipInfo && prChipInfo->ccif_ops &&
	    prChipInfo->ccif_ops->get_fw_log_read_pointer)
		u4Rp = prChipInfo->ccif_ops->get_fw_log_read_pointer(ad,
			type);

exit:
	return u4Rp;
}

static inline int32_t ccif_trigger_fw_assert(struct ADAPTER *ad)
{
	struct mt66xx_chip_info *prChipInfo = NULL;
	int32_t ret = 0;

	if (!ad)
		goto exit;

	prChipInfo = ad->chip_info;

	if (prChipInfo && prChipInfo->ccif_ops &&
	    prChipInfo->ccif_ops->trigger_fw_assert)
		ret = prChipInfo->ccif_ops->trigger_fw_assert(ad);

exit:
	return ret;
}

#endif
