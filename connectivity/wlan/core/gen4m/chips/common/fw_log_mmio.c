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
#include "fw_log_mmio.h"

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define DEFAULT_LOG_READ_POINTER_PATH		ENUM_LOG_READ_POINTER_PATH_CCIF
#define FW_LOG_STATS_UPDATE_PERIOD		1000

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */
struct FW_LOG_MMIO_CTRL g_ctx;

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
static void fwLogCtrlRefreshSubHeader(struct ADAPTER *prAdapter,
	struct FW_LOG_MMIO_CTRL *prCtrl,
	struct FW_LOG_MMIO_SUB_CTRL *prSubCtrl);
static void fwLogMmioStatsDump(struct ADAPTER *prAdapter,
	struct FW_LOG_MMIO_CTRL *prCtrl);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
static uint32_t fwLogCtrlInitQueryBaseAddr(struct ADAPTER *prAdapter,
	uint32_t *pu4Addr)
{
	struct INIT_CMD_LOG_BUF_CTRL rCmd = {0};
	struct INIT_WIFI_EVENT_LOG_BUF_CTRL rEvent = {0};
	uint32_t u4Status = WLAN_STATUS_SUCCESS;

	rCmd.ucType = BIT(FW_LOG_CTRL_CMD_GET_BASE_ADDR);

	u4Status = wlanSendInitSetQueryCmd(prAdapter,
		INIT_CMD_ID_LOG_BUF_CTRL, &rCmd, sizeof(rCmd),
		TRUE, FALSE,
		INIT_EVENT_ID_LOG_BUF_CTRL, &rEvent, sizeof(rEvent));
	if (u4Status != WLAN_STATUS_SUCCESS)
		goto exit;

	if (rEvent.ucStatus != 0) {
		u4Status = WLAN_STATUS_FAILURE;
		goto exit;
	}

	if (rEvent.ucType != BIT(FW_LOG_CTRL_CMD_GET_BASE_ADDR)) {
		u4Status = WLAN_STATUS_FAILURE;
		goto exit;
	}

	DBGLOG(INIT, TRACE, "Log base addr: 0x%x.\n",
		rEvent.u4Address);
	if (pu4Addr)
		*pu4Addr = rEvent.u4Address;

exit:
	return u4Status;
}

static uint32_t fwLogCtrlInitUpdateRead(struct ADAPTER *prAdapter,
	enum FW_LOG_CMD_CTRL_TYPE type,
	uint32_t u4Addr)
{
	struct INIT_CMD_LOG_BUF_CTRL rCmd = {0};
	uint32_t u4Status = WLAN_STATUS_SUCCESS;

	rCmd.ucType = BIT(type);
	switch (type) {
	case FW_LOG_CTRL_CMD_UPDATE_MCU_READ:
		rCmd.u4Address_MCU = u4Addr;
		break;
	case FW_LOG_CTRL_CMD_UPDATE_WIFI_READ:
		rCmd.u4Address_WIFI = u4Addr;
		break;
	case FW_LOG_CTRL_CMD_UPDATE_BT_READ:
		rCmd.u4Address_BT = u4Addr;
		break;
	case FW_LOG_CTRL_CMD_UPDATE_GPS_READ:
		rCmd.u4Address_GPS = u4Addr;
		break;
	default:
		DBGLOG(INIT, ERROR,
			"Invalid log ctrl type: %d\n", type);
		u4Status = WLAN_STATUS_FAILURE;
		goto exit;
	}

	u4Status = wlanSendInitSetQueryCmd(prAdapter,
		INIT_CMD_ID_LOG_BUF_CTRL, &rCmd, sizeof(rCmd),
		FALSE, FALSE,
		0, NULL, 0);

exit:
	return u4Status;
}

static void fwLogCtrlUpdateRp(struct ADAPTER *prAdapter,
	struct FW_LOG_MMIO_CTRL *prCtrl,
	struct FW_LOG_MMIO_SUB_CTRL *prSubCtrl,
	uint32_t u4Rp)
{
	DBGLOG(INIT, LOUD, "[%d %s] u4Rp: 0x%x\n",
		prSubCtrl->type,
		fw_log_type_to_str(prSubCtrl->type),
		u4Rp);

	if (prCtrl->ePath == ENUM_LOG_READ_POINTER_PATH_CCIF)
		ccif_set_fw_log_read_pointer(prAdapter, prSubCtrl->type,
			u4Rp);
	else {
		enum FW_LOG_CMD_CTRL_TYPE type;

		switch (prSubCtrl->type) {
		case ENUM_FW_LOG_CTRL_TYPE_MCU:
			type = FW_LOG_CTRL_CMD_UPDATE_MCU_READ;
			break;
		case ENUM_FW_LOG_CTRL_TYPE_WIFI:
			type = FW_LOG_CTRL_CMD_UPDATE_WIFI_READ;
			break;
		default:
			return;
		}
		nicUniCmdFwLogUpdateRead(prAdapter, type, u4Rp);
	}
}

static u_int8_t fwLogCtrlIsBufEmpty(struct FW_LOG_MMIO_SUB_CTRL *prSubCtrl)
{
	if (prSubCtrl->irp == prSubCtrl->wp)
		return TRUE;
	else
		return FALSE;
}

static void fwLogCtrlSubHandler(struct ADAPTER *prAdapter,
	struct FW_LOG_MMIO_CTRL *prCtrl,
	struct FW_LOG_MMIO_SUB_CTRL *prSubCtrl,
	u_int8_t fgForceRead)
{
	struct FW_LOG_MMIO_STATS *prStats = &prCtrl->stats;
	struct FW_LOG_MMIO_SUB_STATS *prSubStats =
		&prStats->sub_stats[prSubCtrl->type];
	uint32_t u4Offset = 0, u4Rp = 0, u4Recv = 0, u4Handled = 0;

	fwLogCtrlRefreshSubHeader(prAdapter, prCtrl, prSubCtrl);

	if (fgForceRead)
		prSubCtrl->wp = prSubCtrl->iwp;
	prSubCtrl->wp &= ~BITS(0, 2);

	if (fwLogCtrlIsBufEmpty(prSubCtrl))
		return;

	if (prSubCtrl->wp > prSubCtrl->irp)
		u4Recv = prSubCtrl->wp - prSubCtrl->irp;
	else
		u4Recv = prSubCtrl->length - prSubCtrl->irp + prSubCtrl->wp;
	if (u4Recv > prSubCtrl->length) {
		struct CHIP_DBG_OPS *prDebugOps = NULL;

		DBGLOG(INIT, ERROR,
			"Invalid rcv length (%u %u)\n",
			u4Recv, prSubCtrl->length);

		prDebugOps = prAdapter->chip_info->prDebugOps;

		if (prDebugOps && prDebugOps->dumpBusStatus)
			prDebugOps->dumpBusStatus(prAdapter);

		WARN_ON_ONCE(TRUE);

		return;
	}
	u4Handled = u4Recv;

	u4Rp = prSubCtrl->irp;
	while (u4Recv) {
		uint32_t u4Size = 0;

		if (u4Rp + u4Recv > prSubCtrl->length)
			u4Size = prSubCtrl->length - u4Rp;
		else
			u4Size = u4Recv;

		DBGLOG(INIT, LOUD,
			"[%d %s] Read data from 0x%x, size: 0x%x\n",
			prSubCtrl->type,
			fw_log_type_to_str(prSubCtrl->type),
			u4Rp,
			u4Size);

		HAL_RMCR_RD_RANGE(prAdapter,
			prSubCtrl->buf_base_addr + u4Rp,
			prSubCtrl->buffer + u4Offset,
			u4Size);

		u4Offset += u4Size;
		u4Rp += u4Size;
		u4Rp %= prSubCtrl->length;
		u4Recv -= u4Size;
	}

	u4Handled = fw_log_notify_rcv(prSubCtrl->type,
		prSubCtrl->buffer, u4Handled);
	prSubStats->handle_size += u4Handled;

	prSubCtrl->irp += u4Handled;
	prSubCtrl->irp %= prSubCtrl->length;
	fwLogCtrlUpdateRp(prAdapter, prCtrl, prSubCtrl,
		prSubCtrl->irp);
}

static int32_t __fwLogMmioHandler(u_int8_t fgForceRead)
{
	struct FW_LOG_MMIO_CTRL *prCtrl = &g_ctx;
	struct FW_LOG_MMIO_STATS *prStats = &prCtrl->stats;
	struct ADAPTER *prAdapter = NULL;
	uint8_t i = 0;

	prAdapter = (struct ADAPTER *)prCtrl->priv;
	if (!prAdapter) {
		prStats->skipped++;
		return 0;
	}

#if CFG_ENABLE_WAKE_LOCK
	KAL_WAKE_LOCK(prAdapter, prCtrl->prWakeLock);
#endif
	KAL_ACQUIRE_MUTEX(prAdapter, MUTEX_FW_LOG);

	ACQUIRE_POWER_CONTROL_FROM_PM(prAdapter,
		DRV_OWN_SRC_FW_LOG_MMIO_HANDLER);

	if (prAdapter->fgIsFwOwn == TRUE) {
		DBGLOG(INIT, WARN,
			"Skip due to driver own failed.\n");
		prStats->skipped++;

		RECLAIM_POWER_CONTROL_TO_PM(prAdapter, FALSE,
			DRV_OWN_SRC_FW_LOG_MMIO_HANDLER);

		KAL_RELEASE_MUTEX(prAdapter, MUTEX_FW_LOG);
#if CFG_ENABLE_WAKE_LOCK
		KAL_WAKE_UNLOCK(prAdapter, prCtrl->prWakeLock);
#endif
		goto exit;
	}

	for (i = 0; i < ENUM_FW_LOG_CTRL_TYPE_NUM; i++) {
		struct FW_LOG_MMIO_SUB_CTRL *prSubCtrl = &prCtrl->sub_ctrls[i];

		fwLogCtrlSubHandler(prAdapter, prCtrl, prSubCtrl,
				    fgForceRead);
	}

	RECLAIM_POWER_CONTROL_TO_PM(prAdapter, FALSE,
		DRV_OWN_SRC_FW_LOG_MMIO_HANDLER);
	KAL_RELEASE_MUTEX(prAdapter, MUTEX_FW_LOG);
#if CFG_ENABLE_WAKE_LOCK
	KAL_WAKE_UNLOCK(prAdapter, prCtrl->prWakeLock);
#endif

	prStats->handled++;

exit:
	fwLogMmioStatsDump(prAdapter, prCtrl);

	return 0;
}

int32_t fwLogMmioHandler(void)
{
	struct FW_LOG_MMIO_CTRL *prCtrl = &g_ctx;
	struct FW_LOG_MMIO_STATS *prStats = &prCtrl->stats;

	prStats->request++;

	if (!prCtrl->initialized) {
		prStats->skipped++;
		goto exit;
	} else if (!prCtrl->started) {
		prCtrl->defered = TRUE;
		prStats->skipped++;
		goto exit;
	}

	queue_work(prCtrl->wq, &prCtrl->work);

exit:
	return 0;
}

static void fwLogCtrlRefreshCommonHeader(struct ADAPTER *prAdapter,
	struct FW_LOG_MMIO_CTRL *prCtrl)
{
	struct FW_LOG_COMMON_HEADER rCommonHeader = {0};
	uint8_t i = 0;

	HAL_RMCR_RD_RANGE(prAdapter, prCtrl->base_addr,
			   &rCommonHeader,
			   sizeof(rCommonHeader));

	for (i = 0; i < ENUM_FW_LOG_CTRL_TYPE_NUM; i++) {
		struct FW_LOG_MMIO_SUB_CTRL *prSubCtrl = &prCtrl->sub_ctrls[i];

		switch (i) {
		case ENUM_FW_LOG_CTRL_TYPE_MCU:
			prSubCtrl->base_addr = prCtrl->base_addr +
				rCommonHeader.mcu_log_base_addr;
			prSubCtrl->length = rCommonHeader.mcu_log_length;
			break;
		case ENUM_FW_LOG_CTRL_TYPE_WIFI:
			prSubCtrl->base_addr = prCtrl->base_addr +
				rCommonHeader.wifi_log_base_addr;
			prSubCtrl->length = rCommonHeader.wifi_log_length;
			break;
		default:
			DBGLOG(INIT, ERROR, "Unknown type: %d\n",
				prSubCtrl->type);
			continue;
		}

		DBGLOG(INIT, TRACE,
			"type: %d %s, base_addr: 0x%x, length: 0x%x\n",
			i,
			fw_log_type_to_str(i),
			prSubCtrl->base_addr,
			prSubCtrl->length);
	}
}

static void fwLogCtrlRefreshSubHeader(struct ADAPTER *prAdapter,
	struct FW_LOG_MMIO_CTRL *prCtrl,
	struct FW_LOG_MMIO_SUB_CTRL *prSubCtrl)
{
	struct FW_LOG_SUB_HEADER rSubHeader = {0};

	HAL_RMCR_RD_RANGE(prAdapter, prSubCtrl->base_addr,
			   &rSubHeader,
			   sizeof(rSubHeader));

	prSubCtrl->buf_base_addr = prSubCtrl->base_addr + sizeof(rSubHeader);
	prSubCtrl->rp = rSubHeader.rp;
	prSubCtrl->wp = rSubHeader.wp;
	prSubCtrl->iwp = rSubHeader.internal_wp;

	DBGLOG(INIT, LOUD,
		"[%d %s] buf_base_addr: 0x%x, rp: 0x%x, irp: 0x%x, wp: 0x%x, iwp: 0x%x\n",
		prSubCtrl->type,
		fw_log_type_to_str(prSubCtrl->type),
		prSubCtrl->buf_base_addr,
		prSubCtrl->rp,
		prSubCtrl->irp,
		prSubCtrl->wp,
		prSubCtrl->iwp);
}

static uint32_t fwLogCtrlInitSubCtrl(struct ADAPTER *prAdapter,
	struct FW_LOG_MMIO_CTRL *prCtrl,
	enum ENUM_FW_LOG_CTRL_TYPE eType)
{
	struct FW_LOG_MMIO_SUB_CTRL *prSubCtrl = &prCtrl->sub_ctrls[eType];
	struct FW_LOG_SUB_HEADER rSubHeader = {0};
	uint32_t u4Status = WLAN_STATUS_SUCCESS;

	prSubCtrl->type = eType;

	HAL_RMCR_RD_RANGE(prAdapter, prSubCtrl->base_addr,
			   &rSubHeader,
			   sizeof(rSubHeader));

	prSubCtrl->buf_base_addr = prSubCtrl->base_addr + sizeof(rSubHeader);
	if (prSubCtrl->length) {
		prSubCtrl->buffer = kalMemAlloc(prSubCtrl->length,
			VIR_MEM_TYPE);
		if (!prSubCtrl->buffer) {
			DBGLOG(INIT, ERROR, "Alloc buffer failed\n");
			u4Status = WLAN_STATUS_RESOURCES;
		} else {
			kalMemZero(prSubCtrl->buffer, prSubCtrl->length);
		}
	} else {
		u4Status = WLAN_STATUS_INVALID_LENGTH;
	}

	return u4Status;
}

static void fwLogCtrlDeInitSubCtrl(struct ADAPTER *prAdapter,
	struct FW_LOG_MMIO_CTRL *prCtrl,
	enum ENUM_FW_LOG_CTRL_TYPE eType)
{
	struct FW_LOG_MMIO_SUB_CTRL *prSubCtrl = &prCtrl->sub_ctrls[eType];

	if (prSubCtrl->length == 0 || !prSubCtrl->buffer)
		return;

	kalMemFree(prSubCtrl->buffer, VIR_MEM_TYPE, prSubCtrl->length);
}

uint32_t fwLogMmioStart(struct ADAPTER *prAdapter)
{
	struct FW_LOG_MMIO_CTRL *prCtrl = &g_ctx;

	DBGLOG(INIT, TRACE, "\n");

	if (!prCtrl->initialized)
		return 0;

	prCtrl->started = TRUE;

	if (prCtrl->defered)
		__fwLogMmioHandler(FALSE);

	return 0;
}

void fwLogMmioStop(struct ADAPTER *prAdapter)
{
	struct FW_LOG_MMIO_CTRL *prCtrl = &g_ctx;

	DBGLOG(INIT, TRACE, "\n");

	if (!prCtrl->initialized)
		return;

	cancel_work_sync(&prCtrl->work);
	__fwLogMmioHandler(TRUE);

	prCtrl->started = FALSE;
}

static void fwLogMmioWork(struct work_struct *work)
{
	__fwLogMmioHandler(FALSE);
}

uint32_t fwLogMmioInitMcu(struct ADAPTER *prAdapter)
{
	struct FW_LOG_MMIO_CTRL *prCtrl = &g_ctx;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
	uint8_t i = 0;

	DBGLOG(INIT, TRACE, "\n");

	kalMemZero(prCtrl, sizeof(*prCtrl));

	prCtrl->priv = prAdapter;
	prCtrl->ePath = DEFAULT_LOG_READ_POINTER_PATH;

	u4Status = fwLogCtrlInitQueryBaseAddr(prAdapter, &prCtrl->base_addr);
	if (u4Status != WLAN_STATUS_SUCCESS)
		goto exit;

	fwLogCtrlRefreshCommonHeader(prAdapter, prCtrl);

	for (i = 0; i < ENUM_FW_LOG_CTRL_TYPE_NUM; i++) {
		u4Status = fwLogCtrlInitSubCtrl(prAdapter, prCtrl, i);
		if (u4Status != WLAN_STATUS_SUCCESS)
			goto exit;
	}

	prCtrl->wq = create_singlethread_workqueue("fw_log_mmio");
	if (!prCtrl->wq) {
		DBGLOG(INIT, ERROR, "create_singlethread_workqueue failed.\n");
		u4Status = WLAN_STATUS_RESOURCES;
		goto exit;
	}
	INIT_WORK(&prCtrl->work, fwLogMmioWork);
#if CFG_ENABLE_WAKE_LOCK
	KAL_WAKE_LOCK_INIT(prAdapter, prCtrl->prWakeLock, "wlan_fw_log");
#endif

	prCtrl->initialized = TRUE;

exit:
	if (u4Status != WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, ERROR, "u4Status: 0x%x\n", u4Status);
		fwLogMmioDeInitMcu(prAdapter);
	}

	return u4Status;
}

void fwLogMmioDeInitMcu(struct ADAPTER *prAdapter)
{
	struct FW_LOG_MMIO_CTRL *prCtrl = &g_ctx;
	uint8_t i = 0;

	DBGLOG(INIT, TRACE, "\n");

	prCtrl->initialized = FALSE;

#if CFG_ENABLE_WAKE_LOCK
	if (KAL_WAKE_LOCK_ACTIVE(prAdapter, prCtrl->prWakeLock))
		KAL_WAKE_UNLOCK(prAdapter, prCtrl->prWakeLock);
	KAL_WAKE_LOCK_DESTROY(prAdapter, prCtrl->prWakeLock);
#endif

	if (prCtrl->wq)
		destroy_workqueue(prCtrl->wq);

	for (i = 0; i < ENUM_FW_LOG_CTRL_TYPE_NUM; i++)
		fwLogCtrlDeInitSubCtrl(prCtrl->priv, prCtrl, i);

	prCtrl->priv = NULL;
}

static void fwLogMmioStatsDump(struct ADAPTER *prAdapter,
	struct FW_LOG_MMIO_CTRL *prCtrl)
{
	struct FW_LOG_MMIO_STATS *prStats = &prCtrl->stats;
	uint8_t buf[512];
	uint32_t written = 0;
	uint8_t i = 0;

	if (time_before(jiffies, prStats->update_period))
		return;

	prStats->update_period = jiffies +
		FW_LOG_STATS_UPDATE_PERIOD * HZ / 1000;

	kalMemZero(&buf, sizeof(buf));
	written += kalSnprintf(buf + written,
			       sizeof(buf) - written,
			       "fw log stats[0x%x 0x%x 0x%x]",
			       prStats->request,
			       prStats->skipped,
			       prStats->handled);
	for (i = 0; i < ENUM_FW_LOG_CTRL_TYPE_NUM; i++) {
		struct FW_LOG_MMIO_SUB_CTRL *prSubCtrl = &prCtrl->sub_ctrls[i];

		written += kalSnprintf(buf + written,
				       sizeof(buf) - written,
				       " [%s][0x%x][0x%x 0x%x 0x%x 0x%x]",
				       fw_log_type_to_str(i),
				       prStats->sub_stats[i].handle_size,
				       prSubCtrl->rp,
				       prSubCtrl->irp,
				       prSubCtrl->wp,
				       prSubCtrl->iwp);
	}
	DBGLOG(INIT, DEBUG, "%s\n", buf);
}

