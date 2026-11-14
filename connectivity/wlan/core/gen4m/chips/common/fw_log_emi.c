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
#include "gl_fw_log.h"
#include "fw_log_emi.h"

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define MAX_FW_LOG_SIZE				0x20000
#define FW_LOG_STATS_UPDATE_PERIOD		1000
#define FW_LOG_KEY				"EMIFWLOG"

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */
struct FW_LOG_EMI_CTRL g_fw_log_emi_ctx;

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
static void fw_log_emi_refresh_sub_header(struct ADAPTER *ad,
	struct FW_LOG_EMI_CTRL *ctrl,
	struct FW_LOG_EMI_SUB_CTRL *sub_ctrl);
static void fw_log_emi_stats_dump(struct ADAPTER *ad,
	struct FW_LOG_EMI_CTRL *ctrl,
	u_int8_t force);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
static void fw_log_emi_update_rp(struct ADAPTER *ad,
	struct FW_LOG_EMI_CTRL *ctrl,
	struct FW_LOG_EMI_SUB_CTRL *sub_ctrl,
	uint32_t rp)
{
	DBGLOG(INIT, LOUD,
		"[%d %s] rp: 0x%x\n",
		sub_ctrl->type,
		fw_log_type_to_str(sub_ctrl->type),
		rp);
	ccif_set_fw_log_read_pointer(ad,
				     sub_ctrl->type,
				     rp);
}

static u_int8_t fw_log_emi_is_empty(struct FW_LOG_EMI_SUB_CTRL *sub_ctrl)
{
	if (sub_ctrl->irp == sub_ctrl->wp)
		return TRUE;
	else
		return FALSE;
}

static void __fw_log_emi_sub_handler(struct ADAPTER *ad,
	struct FW_LOG_EMI_CTRL *ctrl,
	struct FW_LOG_EMI_SUB_CTRL *sub_ctrl,
	u_int8_t force)
{
	struct FW_LOG_EMI_STATS *stats = &ctrl->stats;
	struct FW_LOG_EMI_SUB_STATS *sub_stats =
		&stats->sub_stats[sub_ctrl->type];
	uint32_t offset = 0, rp = 0, recv = 0, handled = 0;

	fw_log_emi_refresh_sub_header(ad, ctrl, sub_ctrl);

	if (force)
		sub_ctrl->wp = sub_ctrl->iwp;

	if (fw_log_emi_is_empty(sub_ctrl))
		return;

	if (sub_ctrl->wp > sub_ctrl->irp)
		recv = sub_ctrl->wp - sub_ctrl->irp;
	else
		recv = sub_ctrl->length - sub_ctrl->irp + sub_ctrl->wp;
	if (recv == 0 || recv > sub_ctrl->length) {
		DBGLOG(INIT, ERROR,
			"Invalid rcv length (%u %u)\n",
			recv, sub_ctrl->length);
		fw_log_emi_stats_dump(ad, ctrl, TRUE);
		return;
	}
	handled = recv;

	rp = sub_ctrl->irp;
	while (recv) {
		uint32_t size = 0;

		if (rp + recv > sub_ctrl->length)
			size = sub_ctrl->length - rp;
		else
			size = recv;

		DBGLOG(INIT, LOUD,
			"[%d %s] Read data from 0x%x, size: 0x%x, offset: 0x%x\n",
			sub_ctrl->type,
			fw_log_type_to_str(sub_ctrl->type),
			rp,
			size,
			offset);

		emi_mem_read(ad->chip_info,
			     sub_ctrl->buf_base_addr + rp,
			     sub_ctrl->buffer + offset,
			     size);

		offset += size;
		rp += size;
		rp %= sub_ctrl->length;
		recv -= size;
	}

	handled = fw_log_notify_rcv(sub_ctrl->type,
				    sub_ctrl->buffer,
				    handled);
	sub_stats->handle_size += handled;

	sub_ctrl->irp += handled;
	sub_ctrl->irp %= sub_ctrl->length;
	fw_log_emi_update_rp(ad, ctrl, sub_ctrl, sub_ctrl->irp);
}

static int32_t __fw_log_emi_handler(u_int8_t force)
{
	struct FW_LOG_EMI_CTRL *ctrl = &g_fw_log_emi_ctx;
	struct FW_LOG_EMI_STATS *stats = &ctrl->stats;
	struct ADAPTER *ad = NULL;
	uint8_t i = 0;

	ad = (struct ADAPTER *)ctrl->priv;

	if (!ad) {
		stats->skipped++;
		goto exit;
	}

#if CFG_ENABLE_WAKE_LOCK
	KAL_WAKE_LOCK(ad, ctrl->wakelock);
#endif
	KAL_ACQUIRE_MUTEX(ad, MUTEX_FW_LOG);
	for (i = 0; i < ENUM_FW_LOG_CTRL_TYPE_NUM; i++) {
		struct FW_LOG_EMI_SUB_CTRL *sub_ctrl = &ctrl->sub_ctrls[i];

		__fw_log_emi_sub_handler(ad,
					 ctrl,
					 sub_ctrl,
					 force);
	}
	KAL_RELEASE_MUTEX(ad, MUTEX_FW_LOG);
#if CFG_ENABLE_WAKE_LOCK
	KAL_WAKE_UNLOCK(ad, ctrl->wakelock);
#endif

	stats->handled++;

	fw_log_emi_stats_dump(ad, ctrl, FALSE);

exit:
	return 0;
}

int32_t fw_log_emi_handler(void)
{
	struct FW_LOG_EMI_CTRL *ctrl = &g_fw_log_emi_ctx;
	struct FW_LOG_EMI_STATS *stats = &ctrl->stats;
	struct ADAPTER *ad = NULL;

	stats->request++;

	ad = (struct ADAPTER *)ctrl->priv;

	if (!ctrl->initialized || !ctrl->started) {
		stats->skipped++;
		goto exit;
	} else if (!ad) {
		stats->skipped++;
		goto exit;
	}

	queue_work(ctrl->wq, &ctrl->work);

exit:
	return 0;
}

static void fw_log_emi_work(struct work_struct *work)
{
	__fw_log_emi_handler(FALSE);
}

static u_int8_t __fw_log_emi_should_flush_buffer(struct FW_LOG_EMI_CTRL *ctrl,
	struct FW_LOG_EMI_SUB_CTRL *sub_ctrl)
{
	uint32_t rest = 0;

	if (sub_ctrl->irp == sub_ctrl->wp)
		return FALSE;

	if (sub_ctrl->irp > sub_ctrl->wp)
		rest = sub_ctrl->length - sub_ctrl->irp + sub_ctrl->wp;
	else
		rest = sub_ctrl->wp - sub_ctrl->irp;

	if (rest >= sub_ctrl->length / 2)
		return TRUE;
	else
		return FALSE;
}

static void __fw_log_emi_force_reset_buffer(struct ADAPTER *ad,
	struct FW_LOG_EMI_CTRL *ctrl)
{
	uint8_t i = 0;

	KAL_ACQUIRE_MUTEX(ad, MUTEX_FW_LOG);
	for (i = 0; i < ENUM_FW_LOG_CTRL_TYPE_NUM; i++) {
		struct FW_LOG_EMI_SUB_CTRL *sub_ctrl = &ctrl->sub_ctrls[i];

		fw_log_emi_refresh_sub_header(ad, ctrl, sub_ctrl);

		if (!__fw_log_emi_should_flush_buffer(ctrl, sub_ctrl))
			continue;

		DBGLOG(INIT, WARN,
			"[%d %s] Force reset rp from 0x%x to 0x%x\n",
			sub_ctrl->type,
			fw_log_type_to_str(sub_ctrl->type),
			sub_ctrl->irp,
			sub_ctrl->wp);
		sub_ctrl->irp = sub_ctrl->wp;
		fw_log_emi_update_rp(ad, ctrl, sub_ctrl, sub_ctrl->irp);
	}
	KAL_RELEASE_MUTEX(ad, MUTEX_FW_LOG);

	fw_log_emi_stats_dump(ad, ctrl, TRUE);
}

static u_int8_t __fw_log_emi_check_alignment(uint32_t value)
{
	if (value & BITS(0, 2))
		return FALSE;
	else
		return TRUE;
}

static int32_t fw_log_emi_refresh_common_header(struct ADAPTER *ad,
	struct FW_LOG_EMI_CTRL *ctrl)
{
	struct mt66xx_chip_info *chip_info = ad->chip_info;
	struct FW_LOG_COMMON_HEADER common = {0};
	uint8_t i = 0;
	int32_t ret = 0;

	ctrl->base_addr = chip_info->fw_log_info.base;

	ret = emi_mem_read(chip_info,
			   ctrl->base_addr,
			   &common,
			   sizeof(common));
	if (ret)
		goto exit;

	if (kalMemCmp(common.key, FW_LOG_KEY, sizeof(common.key)) != 0) {
		DBGLOG(INIT, ERROR, "INVALID FW LOG KEY.\n");
		ret = -EINVAL;
		goto exit;
	}

	for (i = 0; i < ENUM_FW_LOG_CTRL_TYPE_NUM; i++) {
		struct FW_LOG_EMI_SUB_CTRL *sub_ctrl = &ctrl->sub_ctrls[i];

		switch (i) {
		case ENUM_FW_LOG_CTRL_TYPE_MCU:
			sub_ctrl->base_addr = ctrl->base_addr +
				common.mcu_log_base_addr;
			sub_ctrl->length = common.mcu_log_length;
			break;
		case ENUM_FW_LOG_CTRL_TYPE_WIFI:
			sub_ctrl->base_addr = ctrl->base_addr +
				common.wifi_log_base_addr;
			sub_ctrl->length = common.wifi_log_length;
			break;
		default:
			DBGLOG(INIT, ERROR, "Unknown type: %d\n",
				sub_ctrl->type);
			continue;
		}

		DBGLOG(INIT, DEBUG,
			"[%d %s] base_addr: 0x%x, length: 0x%x\n",
			i,
			fw_log_type_to_str(i),
			sub_ctrl->base_addr,
			sub_ctrl->length);
		if (!__fw_log_emi_check_alignment(sub_ctrl->base_addr) ||
		    !__fw_log_emi_check_alignment(sub_ctrl->length)) {
			ret = -EINVAL;
			goto exit;
		} else if (sub_ctrl->length > MAX_FW_LOG_SIZE) {
			ret = -EINVAL;
			goto exit;
		} else if (emi_mem_offset_convert(sub_ctrl->base_addr) >
			   emi_mem_get_size(chip_info)) {
			ret = -EINVAL;
			goto exit;
		}
	}

exit:
	if (ret)
		DBGLOG_MEM8(INIT, ERROR, &common, sizeof(common));

	return ret;
}

static void fw_log_emi_refresh_sub_header(struct ADAPTER *ad,
	struct FW_LOG_EMI_CTRL *ctrl,
	struct FW_LOG_EMI_SUB_CTRL *sub_ctrl)
{
	struct FW_LOG_SUB_HEADER sub_header = {0};

	emi_mem_read(ad->chip_info,
		     sub_ctrl->base_addr,
		     &sub_header,
		     sizeof(sub_header));

	sub_ctrl->rp = sub_header.rp;
	sub_ctrl->wp = sub_header.wp;
	sub_ctrl->iwp = sub_header.internal_wp;

	DBGLOG(INIT, LOUD,
		"[%d %s] rp: 0x%x, irp: 0x%x, wp: 0x%x, iwp: 0x%x\n",
		sub_ctrl->type,
		fw_log_type_to_str(sub_ctrl->type),
		sub_ctrl->rp,
		sub_ctrl->irp,
		sub_ctrl->wp,
		sub_ctrl->iwp);
}

static uint32_t fw_log_emi_sub_ctrl_init(struct ADAPTER *ad,
	struct FW_LOG_EMI_CTRL *ctrl,
	enum ENUM_FW_LOG_CTRL_TYPE type)
{
	struct FW_LOG_EMI_SUB_CTRL *sub_ctrl = &ctrl->sub_ctrls[type];
	uint32_t status = WLAN_STATUS_SUCCESS;

	sub_ctrl->type = type;
	sub_ctrl->buf_base_addr = sub_ctrl->base_addr +
		sizeof(struct FW_LOG_SUB_HEADER);
	if (sub_ctrl->length) {
		sub_ctrl->buffer = kalMemAlloc(sub_ctrl->length,
			VIR_MEM_TYPE);
		if (!sub_ctrl->buffer) {
			DBGLOG(INIT, ERROR, "Alloc buffer failed\n");
			status = WLAN_STATUS_RESOURCES;
		} else {
			kalMemZero(sub_ctrl->buffer, sub_ctrl->length);
		}
	} else {
		status = WLAN_STATUS_INVALID_LENGTH;
	}
	DBGLOG(INIT, DEBUG, "[%d %s] buf_base_addr: 0x%x, status: 0x%x\n",
		sub_ctrl->type,
		fw_log_type_to_str(sub_ctrl->type),
		sub_ctrl->buf_base_addr,
		status);

	return status;
}

static void fw_log_emi_sub_ctrl_deinit(struct ADAPTER *ad,
	struct FW_LOG_EMI_CTRL *ctrl,
	enum ENUM_FW_LOG_CTRL_TYPE type)
{
	struct FW_LOG_EMI_SUB_CTRL *sub_ctrl = &ctrl->sub_ctrls[type];

	if (sub_ctrl->length == 0 || !sub_ctrl->buffer)
		return;

	kalMemFree(sub_ctrl->buffer, VIR_MEM_TYPE, sub_ctrl->length);
}

uint32_t fw_log_emi_start(struct ADAPTER *ad)
{
	struct FW_LOG_EMI_CTRL *ctrl = &g_fw_log_emi_ctx;
	uint32_t status = WLAN_STATUS_SUCCESS;
	uint8_t i = 0;
	int32_t ret = 0;

	DBGLOG(INIT, TRACE, "\n");

	if (!ctrl->initialized)
		return 0;

	ret = fw_log_emi_refresh_common_header(ad, ctrl);
	if (ret) {
		status = WLAN_STATUS_FAILURE;
		goto exit;
	}

	for (i = 0; i < ENUM_FW_LOG_CTRL_TYPE_NUM; i++) {
		status = fw_log_emi_sub_ctrl_init(ad, ctrl, i);
		if (status != WLAN_STATUS_SUCCESS)
			goto deinit;
	}

	ctrl->started = TRUE;

	__fw_log_emi_handler(FALSE);
	goto exit;

deinit:
	for (i = 0; i < ENUM_FW_LOG_CTRL_TYPE_NUM; i++)
		fw_log_emi_sub_ctrl_deinit(ad, ctrl, i);

exit:
	return status;
}

void fw_log_emi_stop(struct ADAPTER *ad)
{
	struct FW_LOG_EMI_CTRL *ctrl = &g_fw_log_emi_ctx;
	uint8_t i = 0;

	DBGLOG(INIT, TRACE, "\n");

	if (!ctrl->initialized || !ctrl->started)
		return;

	__fw_log_emi_handler(TRUE);

	ctrl->started = FALSE;

	cancel_work_sync(&ctrl->work);

	for (i = 0; i < ENUM_FW_LOG_CTRL_TYPE_NUM; i++)
		fw_log_emi_sub_ctrl_deinit(ctrl->priv, ctrl, i);
}

void fw_log_emi_set_enabled(struct ADAPTER *ad, u_int8_t enabled)
{
	struct FW_LOG_EMI_CTRL *ctrl = &g_fw_log_emi_ctx;

	DBGLOG(INIT, DEBUG, "enabled: %d\n", enabled);

	if (enabled)
		__fw_log_emi_force_reset_buffer(ad, ctrl);
}

uint32_t fw_log_emi_init(struct ADAPTER *ad)
{
	struct FW_LOG_EMI_CTRL *ctrl = &g_fw_log_emi_ctx;
	uint32_t status = WLAN_STATUS_SUCCESS;

	DBGLOG(INIT, TRACE, "\n");

	kalMemZero(ctrl, sizeof(*ctrl));

	ctrl->priv = ad;
#if CFG_ENABLE_WAKE_LOCK
	KAL_WAKE_LOCK_INIT(ad, ctrl->wakelock, "wlan_fw_log");
#endif
	ctrl->wq = create_singlethread_workqueue("fw_log_emi");
	if (!ctrl->wq) {
		DBGLOG(INIT, ERROR,
			"create_singlethread_workqueue failed.\n");
		status = WLAN_STATUS_RESOURCES;
		goto exit;
	}
	INIT_WORK(&ctrl->work, fw_log_emi_work);
	ctrl->initialized = TRUE;

exit:
	return status;
}

void fw_log_emi_deinit(struct ADAPTER *ad)
{
	struct FW_LOG_EMI_CTRL *ctrl = &g_fw_log_emi_ctx;

	DBGLOG(INIT, TRACE, "\n");

	ctrl->initialized = FALSE;
	if (ctrl->wq)
		destroy_workqueue(ctrl->wq);
#if CFG_ENABLE_WAKE_LOCK
	if (KAL_WAKE_LOCK_ACTIVE(ad, ctrl->wakelock))
		KAL_WAKE_UNLOCK(ad, ctrl->wakelock);
	KAL_WAKE_LOCK_DESTROY(ad, ctrl->wakelock);
#endif
	ctrl->priv = NULL;
}

static void fw_log_emi_stats_dump(struct ADAPTER *ad,
	struct FW_LOG_EMI_CTRL *ctrl,
	u_int8_t force)
{
	struct FW_LOG_EMI_STATS *stats = &ctrl->stats;
	uint8_t buf[512];
	uint32_t written = 0;
	uint8_t i = 0;

	if (!force && time_before(jiffies, stats->update_period))
		return;

	stats->update_period = jiffies +
		FW_LOG_STATS_UPDATE_PERIOD * HZ / 1000;

	kalMemZero(&buf, sizeof(buf));
	written += kalSnprintf(buf + written,
			       sizeof(buf) - written,
			       "fw log stats[0x%x 0x%x 0x%x]",
			       stats->request,
			       stats->skipped,
			       stats->handled);
	for (i = 0; i < ENUM_FW_LOG_CTRL_TYPE_NUM; i++) {
		struct FW_LOG_EMI_SUB_CTRL *sub_ctrl = &ctrl->sub_ctrls[i];

		written += kalSnprintf(buf + written,
				       sizeof(buf) - written,
				       " [%s][0x%x][0x%x 0x%x 0x%x 0x%x]",
				       fw_log_type_to_str(i),
				       stats->sub_stats[i].handle_size,
				       sub_ctrl->rp,
				       sub_ctrl->irp,
				       sub_ctrl->wp,
				       sub_ctrl->iwp);
	}
	DBGLOG(INIT, DEBUG, "%s\n", buf);
}

