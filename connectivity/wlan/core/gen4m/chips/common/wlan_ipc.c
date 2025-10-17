// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2023 MediaTek Inc.
 */
/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#if (CFG_MTK_WIFI_SUPPORT_IPC == 1)
#include "precomp.h"
#include "wlan_ipc.h"
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

uint8_t wlanIPCAccessConnVonSysRam(struct GLUE_INFO *prGlueInfo,
	enum IPC_RW_TYPE eType,
	uint32_t u4Offset,
	uint32_t *pu4Addr,
	uint32_t u4Size)
{
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct WLAN_IPC_INFO *prIPCInfo = NULL;
	uint8_t u1Status = 0, fgResult = FALSE;

	glGetChipInfo((void **)&prChipInfo);
	if (prChipInfo == NULL) {
		DBGLOG(INIT, ERROR, "NULL prChipInfo.\n");
		return FALSE;
	}

	prIPCInfo = prChipInfo->ipc_info;
	if (prIPCInfo == NULL) {
		DBGLOG(INIT, ERROR, "NULL prIPCInfo.\n");
		return FALSE;
	};
	/* WLAN_IPC_WRITE = 0, WLAN_IPC_READ = 1 */
	u1Status = (eType << 1) + (u4Size > sizeof(uint32_t));
	switch (u1Status) {
	/* Write 4 bytes */
	case 0: {
		fgResult = kalDevRegWrite(prGlueInfo,
			prIPCInfo->conn_von_sysram_base_addr + u4Offset,
			*pu4Addr);
		break;
	}
	/* Write > 4 bytes */
	case 1: {
		fgResult = kalDevRegWriteRange(prGlueInfo,
			prIPCInfo->conn_von_sysram_base_addr + u4Offset,
			pu4Addr,
			u4Size);
		break;
	}
	/* Read 4 bytes */
	case 2: {
		fgResult = kalDevRegRead(HIF_DEV_REG_ONOFF_READ,
			prGlueInfo,
			prIPCInfo->conn_von_sysram_base_addr + u4Offset,
			pu4Addr);
		break;
	}
	/* Read > 4 bytes */
	case 3: {
		fgResult = kalDevRegReadRange(HIF_DEV_REG_ONOFF_READ,
			prGlueInfo,
			prIPCInfo->conn_von_sysram_base_addr + u4Offset,
			pu4Addr,
			u4Size);
		break;
	}
	default: {
		DBGLOG(HAL, ERROR,
			"The input type: %d is not supported.\n",
			eType);
		return FALSE;
	}
	}

	DBGLOG(HAL, TRACE,
		"Status: %d, Offset: 0x%08x, value: 0x%08x\n",
		u1Status, u4Offset, *pu4Addr);

	return fgResult;
}

uint32_t wlanIPCAccessPciCfgSpace(
	enum IPC_RW_TYPE eType,
	int32_t i4Offset,
	uint32_t u4BitOffset,
	uint32_t *pu4Value)
{
	struct mt66xx_chip_info *prChipInfo = NULL;
	int ret = 0;

	glGetChipInfo((void **)&prChipInfo);
	if (prChipInfo == NULL) {
		DBGLOG(INIT, ERROR, "NULL prChipInfo.\n");
		return WLAN_STATUS_FAILURE;
	}

	/* Handle the case if the bit offset > 31 */
	while (u4BitOffset >= 32) {
		u4BitOffset -= 32;
		i4Offset += 4;
	}

	DBGLOG(INIT, TRACE,
		"offset:[0x%08x], bit offset:[%u]\n",
		i4Offset, u4BitOffset);
	switch (eType) {
	case WLAN_IPC_READ: {
		ret = glReadPcieCfgSpace(i4Offset, pu4Value);
		break;
	}
	case WLAN_IPC_WRITE: {
		ret = glWritePcieCfgSpace(i4Offset, *pu4Value);
		break;
	}
	case WLAN_IPC_SET: {
		ret = glReadPcieCfgSpace(i4Offset, pu4Value);
		*pu4Value |= (1 << u4BitOffset);
		ret = glWritePcieCfgSpace(i4Offset, *pu4Value);
		break;
	}
	case WLAN_IPC_CLR: {
		ret = glReadPcieCfgSpace(i4Offset, pu4Value);
		*pu4Value &= ~(1 << u4BitOffset);
		ret = glWritePcieCfgSpace(i4Offset, *pu4Value);
		break;
	}
	default: {
		DBGLOG(INIT, ERROR, "The input type is not supported.\n");
		return WLAN_STATUS_NOT_SUPPORTED;
	}
	}

	return ret;
}

uint32_t wlanIPCCheckStatus(struct GLUE_INFO *prGlueInfo,
	enum ENUM_IPC_POLLING_TYPE eType,
	uint8_t fgIsBitCheck,
	uint8_t u1BitShift,
	uint32_t *pu4Val,
	uint32_t u4ExpVal,
	uint32_t u4Offset,
	uint32_t u4Range,
	uint32_t u4PollingTimes,
	uint32_t u4Delay)
{
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct WLAN_IPC_INFO *prIPCInfo = NULL;
	uint32_t u4Idx = u4PollingTimes;
	/* No need to compare the current value and expected value
	 * if the polling time or delay is set as 0.
	 * Just dump the given address for the purpose of debug.
	 */
	uint8_t fgIsNeedCheck = !(u4PollingTimes == 0 || u4Delay == 0);
	uint8_t fgIsChecked = !fgIsNeedCheck;

	if (prGlueInfo == NULL)
		return WLAN_STATUS_FAILURE;

	glGetChipInfo((void **)&prChipInfo);
	if (prChipInfo == NULL)
		return WLAN_STATUS_FAILURE;

	prIPCInfo = prChipInfo->ipc_info;
	if (prIPCInfo == NULL)
		return WLAN_STATUS_FAILURE;

	/* If the function is called for bit check,
	 * the expected value should be set as 1 or 0.
	 */
	if (fgIsBitCheck &&
	   (u4ExpVal & 0xfffffffe) != 0) {
		DBGLOG(INIT, WARN,
			"The bit check flag is set, but exp val:[0x%08x]\n",
			u4ExpVal);
		return WLAN_STATUS_NOT_SUPPORTED;
	}

	switch (eType) {
	case PCI_CFG_SPACE: {
		if (prIPCInfo->ipcAccessPciCfgSpace == NULL)
			break;

		for (; u4Idx > 0 && !fgIsChecked; --u4Idx) {
			prIPCInfo->ipcAccessPciCfgSpace(WLAN_IPC_READ,
				(int32_t)u4Offset,
				u1BitShift,
				pu4Val);
			DBGLOG(INIT, TRACE,
				"Offset:[0x%08x] curr val:[0x%x] exp val:[0x%x].\n",
				u4Offset, *pu4Val, u4ExpVal << u1BitShift);
			fgIsChecked = u4ExpVal ?
				(*pu4Val & (u4ExpVal << u1BitShift) != 0) :
				(*pu4Val & (u4ExpVal << u1BitShift) == 0);
			kalMsleep(u4Delay);
		}
		break;
	}
	case CONN_VON_SYSRAM: {
		if (prIPCInfo->ipcAccessConnVonSysRam == NULL)
			break;

		prIPCInfo->ipcAccessConnVonSysRam(prGlueInfo,
			WLAN_IPC_READ,
			u4Offset,
			pu4Val,
			u4Range);
		if (!fgIsNeedCheck) {
			DBGLOG_MEM32(HAL, TRACE, pu4Val, u4Range);
			break;
		}
		for (; u4Idx > 0 && !fgIsChecked; --u4Idx) {
			prIPCInfo->ipcAccessConnVonSysRam(prGlueInfo,
				WLAN_IPC_READ,
				u4Offset,
				pu4Val,
				u4Range);
			DBGLOG(INIT, TRACE,
				"Offset:[0x%08x] curr val:[%u] exp val:[%u].\n",
				u4Offset, *pu4Val, u4ExpVal);
			fgIsChecked = (*pu4Val == u4ExpVal);
			kalMsleep(u4Delay);
		}
		break;
	}
	default:
		return WLAN_STATUS_NOT_SUPPORTED;
	}

	return fgIsChecked ? WLAN_STATUS_SUCCESS : WLAN_STATUS_FAILURE;
}

uint32_t wlanGetUniFwHeaderInfo(const void *pvFwBuffer)
{
	struct UNI_FW_HDR_FORMAT_T *prUniFwHdr =
		(struct UNI_FW_HDR_FORMAT_T *)pvFwBuffer;
	uint8_t aucFwBuildDate[UNI_FW_HDR_BUILD_DATE_LENGTH + 1] = {0};

	if (pvFwBuffer == NULL)
		return WLAN_STATUS_INVALID_DATA;

	kalMemCopy(aucFwBuildDate, prUniFwHdr->aucBuildDate,
		UNI_FW_HDR_BUILD_DATE_LENGTH);
	DBGLOG(INIT, INFO, "FW build date:[%s]\n", aucFwBuildDate);

	return WLAN_STATUS_SUCCESS;
}
#endif /* CFG_MTK_WIFI_SUPPORT_IPC */

