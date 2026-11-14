// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   fw_dl.c
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
 *                              F U N C T I O N S
 *******************************************************************************
 */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called to return the string of WFDL status code
 *
 *
 * @param ucStatus  Status code of FWDL event
 *
 * @return String of FWDL event status code
 */
/*----------------------------------------------------------------------------*/

#if (CFG_MTK_WIFI_SUPPORT_LEGACY_CBMCU_FWDL == 1)

#include "fw_dl_legacy_cbmcu.h"

uint32_t wlanCbmcuCheckCertSect(struct MULTI_HEADER_V2_RAW_FORMAT *prFwHead)
{
	struct MULTI_HEADER_V2_SEC_RAW_FORMAT *prSect;
	uint32_t u4Ret = WLAN_STATUS_FAILURE;

	/* The fw bin sections in order : Header + Cert + ILM + DLM */

	if (prFwHead->u4SecNum < 2) {
		DBGLOG(INIT, ERROR, "CBMCU Sect Num should >= 2 (%d)\n",
			prFwHead->u4SecNum);
		goto out;
	}

	prSect = &prFwHead->aucSections[0];
	if (((prSect->u4Type & SEC_TYPE_SUBSYS_MASK) != SEC_TYPE_SUBSYS_CBMCU)
	    || ((prSect->u4Type & SEC_TYPE_SUBSYS_SEC_MASK) !=
		SEC_TYPE_SUBSYS_SEC_IMG_SIGN)) {
		DBGLOG(INIT, ERROR, "CBMCU 1st sect should be cert (0x%x)\n",
			prSect->u4Type);
		goto out;
	}

	u4Ret = WLAN_STATUS_SUCCESS;
out:
	return u4Ret;
}

uint32_t wlanCbmcuPatchSendSemaControl(struct ADAPTER *prAdapter,
				    uint8_t *pucPatchStatus)
{
	struct INIT_CMD_PATCH_SEMA_CONTROL rCmd = {0};
	struct INIT_EVENT_BT_PATCH_SEMA_CTRL_T rEvent = {0};
	uint32_t u4Status = WLAN_STATUS_SUCCESS;

	rCmd.ucGetSemaphore = PATCH_GET_SEMA_CONTROL;

	u4Status = wlanSendInitSetQueryCmd(prAdapter,
		INIT_CMD_ID_CB_QUERY_PATCH, &rCmd, sizeof(rCmd),
		TRUE, TRUE,
		INIT_EVENT_ID_PATCH_SEMA_CTRL, &rEvent, sizeof(rEvent));

	if (u4Status != WLAN_STATUS_SUCCESS)
		goto exit;

	*pucPatchStatus = rEvent.ucStatus;

exit:
	return u4Status;
}

int32_t wlanCbmcuPatchIsDownloaded(struct ADAPTER *prAdapter)
{
	uint8_t ucPatchStatus = CB_PATCH_STATUS_NO_SEMA_NEED_PATCH;
	uint32_t u4Count = 0;
	uint32_t rStatus;

	while (ucPatchStatus == CB_PATCH_STATUS_NO_SEMA_NEED_PATCH) {
		if (u4Count)
			kalMdelay(100);

		rStatus = wlanCbmcuPatchSendSemaControl(prAdapter,
					&ucPatchStatus);
		if (rStatus != WLAN_STATUS_SUCCESS)
			return -1;

		u4Count++;

		if (u4Count > 50) {
			DBGLOG(INIT, WARN, "Patch status check timeout!!\n");
			return -2;
		}
	}

	DBGLOG(INIT, INFO, "CBMCU Patch status = %d\n", ucPatchStatus);

	if (ucPatchStatus == CB_PATCH_STATUS_NO_NEED_TO_PATCH)
		return 1;
	else
		return 0;
}

uint32_t wlanCbmcuDownloadConfig(
	struct ADAPTER *prAdapter,
	uint32_t u4DestAddr, uint32_t u4ImgSecSize)
{
	struct INIT_CMD_CB_DOWNLOAD_CONFIG rCmd = {0};
	struct INIT_EVENT_CMD_RESULT rEvent = {0};
	u_int8_t fgCheckStatus = FALSE;
	uint32_t u4Status;

	rCmd.u4Address = u4DestAddr; /* meaningless for cbmcu fwdl */
	rCmd.u4Length = u4ImgSecSize;

#if CFG_ENABLE_FW_DOWNLOAD_ACK
	fgCheckStatus = TRUE;
#endif

	u4Status = wlanSendInitSetQueryCmd(prAdapter,
		INIT_CMD_ID_CB_DOWNLOAD_CONFIG, &rCmd, sizeof(rCmd),
		fgCheckStatus, FALSE,
		INIT_EVENT_ID_CMD_RESULT, &rEvent, sizeof(rEvent));

	if (fgCheckStatus) {
		if (u4Status != WLAN_STATUS_SUCCESS) {
			DBGLOG(INIT, ERROR,
				"wlanSendInitSetQueryCmd failed(0x%x).\n",
				u4Status);
			u4Status = WLAN_STATUS_FAILURE;
		} else if (rEvent.ucStatus != 0) {
			DBGLOG(INIT, ERROR,
				"Event status: %d\n",
				rEvent.ucStatus);
			u4Status = WLAN_STATUS_FAILURE;
		}
	}

	return u4Status;
}

uint32_t wlanCbmcuPatchFinish(struct ADAPTER *prAdapter)
{
	struct INIT_CMD_CB_PATCH_FINISH rCmd = {0};
	struct INIT_EVENT_CMD_RESULT rEvent = {0};
	uint32_t u4Status;
#define CBMCU_BYPASS_CHECK_SEQ_NO TRUE

	u4Status = wlanSendInitSetQueryCmd(prAdapter,
		INIT_CMD_ID_CB_PATCH_FINISH, &rCmd, sizeof(rCmd),
		TRUE, CBMCU_BYPASS_CHECK_SEQ_NO,
		INIT_EVENT_ID_CMD_RESULT, &rEvent, sizeof(rEvent));
	if (u4Status != WLAN_STATUS_SUCCESS)
		goto exit;

	if (rEvent.ucStatus != 0) {
		DBGLOG(INIT, ERROR, "Event status: %d\n", rEvent.ucStatus);
		u4Status = WLAN_STATUS_FAILURE;
	}

exit:
	if (u4Status != WLAN_STATUS_SUCCESS)
		DBGLOG(INIT, ERROR, "PATCH FINISH EVT failed\n");
	else
		DBGLOG(INIT, DEBUG, "PATCH FINISH EVT success!!\n");

	return u4Status;
}

uint32_t wlanCbmcuDownloadFirstStage(struct ADAPTER *prAdapter,
		struct MULTI_HEADER_V2_RAW_FORMAT *prFwHead)
{
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
	uint32_t u4Len;
	uint8_t *prPtr;

	/* download global desc + section map + cert section */
	u4Len = 64 +
		sizeof(struct MULTI_HEADER_V2_SEC_RAW_FORMAT) *
		prFwHead->u4SecNum +
		prFwHead->aucSections[0].u4Size;
	prPtr = &prFwHead->aucReserved1[0];

	if (wlanCbmcuDownloadConfig(prAdapter, 0, u4Len) !=
		WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, ERROR,
			"CBMCU download config failed!\n");
		u4Status = WLAN_STATUS_FAILURE;
		goto out;
	}

	if (wlanImageSectionDownload(prAdapter, prPtr, u4Len) !=
		WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, ERROR,
			"CBMCU 1st stage download failed!\n");
		u4Status = WLAN_STATUS_FAILURE;
		goto out;
	}

	/* start cert verify */
	u4Status = wlanCbmcuPatchFinish(prAdapter);
	if (u4Status != WLAN_STATUS_SUCCESS)
		DBGLOG(INIT, ERROR, "CBMCU Cert check fail\n");

out:

	return u4Status;
}

uint32_t wlanCbmcuDownloadIDLM(struct ADAPTER *prAdapter,
		struct MULTI_HEADER_V2_RAW_FORMAT *prFwHead)
{
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
	uint32_t u4Len;
	uint32_t i;
	uint8_t *prPtr;

	for (i = 1; i < prFwHead->u4SecNum; i++) {
		prPtr = (uint8_t *)prFwHead + prFwHead->aucSections[i].u4Offset;
		u4Len = prFwHead->aucSections[i].u4Size;

		if (wlanCbmcuDownloadConfig(prAdapter, 0, u4Len) !=
			WLAN_STATUS_SUCCESS) {
			DBGLOG(INIT, ERROR,
				"CBMCU Sect[%d] download config failed!\n", i);
			u4Status = WLAN_STATUS_FAILURE;
			goto out;
		}

		if (wlanImageSectionDownload(prAdapter, prPtr, u4Len) !=
			WLAN_STATUS_SUCCESS) {
			DBGLOG(INIT, ERROR,
				"CBMCU Sect[%d] download failed!\n", i);
			u4Status = WLAN_STATUS_FAILURE;
			goto out;
		}
	}

	/* patch finish */
	u4Status = wlanCbmcuPatchFinish(prAdapter);
	if (u4Status != WLAN_STATUS_SUCCESS)
		DBGLOG(INIT, ERROR, "CBMCU Patch Start failed\n");

out:

	return u4Status;
}

uint32_t wlanDownloadCbmcuFw(struct ADAPTER *prAdapter)
{
	uint32_t u4FwSize = 0;
	uint32_t u4Status = 0;
	int32_t i4PatchCheck;
	struct MULTI_HEADER_V2_RAW_FORMAT *prFwHead;
	void *prFwBuffer = NULL;

	if (!prAdapter)
		return WLAN_STATUS_FAILURE;

	DBGLOG(INIT, STATE, "CBMCU FW download start\n");

	/* step.1 open the CBMCU fw bin file */
	kalFirmwareImageMapping(prAdapter->prGlueInfo, &prFwBuffer,
				&u4FwSize, IMG_DL_IDX_CBMCU_FW);
	if (prFwBuffer == NULL) {
		DBGLOG(INIT, WARN,
			"Can't load CBMCU Bin! Bypass CBMCU Download\n");
		return WLAN_STATUS_SUCCESS;
	}

	prFwHead = (struct MULTI_HEADER_V2_RAW_FORMAT *) prFwBuffer;

	/* step 2. check CBMCU has downloaded PATCH, or not */
	i4PatchCheck = wlanCbmcuPatchIsDownloaded(prAdapter);
	if (i4PatchCheck < 0) {
		DBGLOG(INIT, STATE, "Get CBMCU Semaphore Fail (%d)\n",
			i4PatchCheck);
		u4Status =  WLAN_STATUS_FAILURE;
		goto out;
	} else if (i4PatchCheck == 1) {
		DBGLOG(INIT, STATE, "No need to download CBMCU patch\n");
		u4Status =  WLAN_STATUS_SUCCESS;
		goto out;
	}

	/* step 3. check the fw bin basic info */
	u4Status = wlanCbmcuCheckCertSect(prFwHead);
	if (u4Status != WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, ERROR, "CBMCU Bin error\n");
		goto out;
	}

	/* step 4. download global desc + sect map + cert section */
	u4Status = wlanCbmcuDownloadFirstStage(prAdapter, prFwHead);
	if (u4Status != WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, ERROR, "CBMCU DL 1st stage fail\n");
		goto out;
	}

	/* step 5. download cbmcu idlm */
	u4Status = wlanCbmcuDownloadIDLM(prAdapter, prFwHead);
	if (u4Status != WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, ERROR, "CBMCU DL IDLM fail\n");
		goto out;
	}

out:

	kalFirmwareImageUnmapping(prAdapter->prGlueInfo, NULL, prFwBuffer);

	return u4Status;
}

#endif /* CFG_MTK_WIFI_SUPPORT_LEGACY_CBMCU_FWDL */
