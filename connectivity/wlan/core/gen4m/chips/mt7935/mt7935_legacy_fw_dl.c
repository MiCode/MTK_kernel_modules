// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2024 MediaTek Inc.
 */

/*! \file  mt7935_legacy_fw_dl.c
 */

/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * Copyright  (C) [2024]  MediaTek Inc. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#if MT7935
#include "precomp.h"
#include "mt7935_legacy_fw_dl.h"

#include "gl_coredump.h"

#if (CFG_MTK_WIFI_SUPPORT_LEGACY_CBMCU_FWDL == 1)
#include "fw_dl_legacy_cbmcu.h"
#endif

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/******************************************************************************
 *                                 M A C R O S
 ******************************************************************************
 */

/******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 ******************************************************************************
 */
#if (CFG_MTK_WIFI_SUPPORT_LEGACY_CBMCU_FWDL == 1)
static void mt7935ConstructCbmcuName(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucName, uint8_t *pucNameIdx);
#endif /* CFG_MTK_WIFI_SUPPORT_LEGACY_CBMCU_FWDL */


/*******************************************************************************
 *                            P U B L I C   D A T A
 ******************************************************************************
 */
uint8_t *apucmt7935FwName[] = {
	(uint8_t *) CFG_FW_FILENAME "_7935",
	NULL
};


/*******************************************************************************
 *                              F U N C T I O N S
 ******************************************************************************
 */
static uint32_t mt7935GetFlavorVer(struct GLUE_INFO *prGlueInfo,
				   uint8_t *flavor)
{
	int32_t ret;
	u_int8_t fgTestFW = FALSE;

#if CFG_WIFI_TESTMODE_FW_REDOWNLOAD
	if (prGlueInfo)
		fgTestFW = prGlueInfo->fgTestFwDl;
#endif

	ret = kalScnprintf(flavor, CFG_FW_FLAVOR_MAX_LEN,
		fgTestFW ? "1t" : "1");

	return (uint32_t) ret;
}

static void mt7935_ConstructFirmwarePrio(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucNameTable, uint8_t **apucName,
	uint8_t *pucNameIdx, uint8_t ucMaxNameIdx)
{
	int ret = 0;
	uint8_t ucIdx = 0;
	uint8_t aucFlavor[CFG_FW_FLAVOR_MAX_LEN];

	kalMemZero(aucFlavor, sizeof(aucFlavor));
	mt7935GetFlavorVer(prGlueInfo, &aucFlavor[0]);

#if CFG_SUPPORT_SINGLE_FW_BINARY
	/* Type 0. mt7935_wifi.bin */
	ret = kalSnprintf(*(apucName + (*pucNameIdx)),
			CFG_FW_NAME_MAX_LEN,
			"mt7935_wifi.bin");
	if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
		(*pucNameIdx) += 1;
	else
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);

	/* Type 1. mt7935_wifi_flavor.bin */
	ret = kalSnprintf(*(apucName + (*pucNameIdx)),
			CFG_FW_NAME_MAX_LEN,
			"mt7935_wifi_%s.bin",
			aucFlavor);
	if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
		(*pucNameIdx) += 1;
	else
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);
#endif

	/* Type 2. WIFI_RAM_CODE_MT7935_1_1.bin */
	ret = kalSnprintf(*(apucName + (*pucNameIdx)),
			CFG_FW_NAME_MAX_LEN,
			"WIFI_RAM_CODE_MT%x_%s_%u.bin",
			MT7935_CHIP_ID,
			aucFlavor,
			MT7935_ROM_VERSION);
	if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
		(*pucNameIdx) += 1;
	else
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);

	for (ucIdx = 0; apucmt7935FwName[ucIdx]; ucIdx++) {
		if ((*pucNameIdx + 3) >= ucMaxNameIdx) {
			/* the table is not large enough */
			DBGLOG(INIT, ERROR,
				"kalFirmwareImageMapping >> file name array is not enough.\n");
			ASSERT(0);
			continue;
		}

		/* Type 3. WIFI_RAM_CODE_7935.bin */
		ret = kalSnprintf(*(apucName + (*pucNameIdx)),
				CFG_FW_NAME_MAX_LEN, "%s.bin",
				apucmt7935FwName[ucIdx]);
		if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
			(*pucNameIdx) += 1;
		else
			DBGLOG(INIT, ERROR,
				"[%u] kalSnprintf failed, ret: %d\n",
				__LINE__, ret);
	}
}

static void mt7935_ConstructPatchName(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucName, uint8_t *pucNameIdx)
{
	int ret = 0;
	uint8_t aucFlavor[CFG_FW_FLAVOR_MAX_LEN];

	kalMemZero(aucFlavor, sizeof(aucFlavor));
	mt7935GetFlavorVer(prGlueInfo, &aucFlavor[0]);

#if CFG_SUPPORT_SINGLE_FW_BINARY
	/* Type 0. mt7935_wifi.bin */
	ret = kalSnprintf(*(apucName + (*pucNameIdx)),
			CFG_FW_NAME_MAX_LEN,
			"mt7935_wifi.bin");
	if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
		(*pucNameIdx) += 1;
	else
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);

	/* Type 1. mt7935_wifi_flavor.bin */
	ret = kalSnprintf(*(apucName + (*pucNameIdx)),
			CFG_FW_NAME_MAX_LEN,
			"mt7935_wifi_%s.bin",
			aucFlavor);
	if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
		(*pucNameIdx) += 1;
	else
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);
#endif

	/* Type 2. WIFI_MT7935_PATCH_MCU_1_1_hdr.bin */
	ret = kalSnprintf(apucName[(*pucNameIdx)],
			  CFG_FW_NAME_MAX_LEN,
			  "WIFI_MT%x_PATCH_MCU_%s_%u_hdr.bin",
			  MT7935_CHIP_ID,
			  aucFlavor,
			  MT7935_ROM_VERSION);
	if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
		(*pucNameIdx) += 1;
	else
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);

	/* Type 3. mt7935_patch_e1_hdr.bin */
	ret = kalSnprintf(apucName[(*pucNameIdx)],
			  CFG_FW_NAME_MAX_LEN,
			  "mt7935_patch_e1_hdr.bin");
	if (ret < 0 || ret >= CFG_FW_NAME_MAX_LEN)
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);
}

#if CFG_MTK_WIFI_SUPPORT_PHY_FWDL
static void mt7935_ConstructPhyName(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucName, uint8_t *pucNameIdx)
{
	int ret = 0;
	uint8_t aucFlavor[CFG_FW_FLAVOR_MAX_LEN];

	kalMemZero(aucFlavor, sizeof(aucFlavor));
	mt7935GetFlavorVer(prGlueInfo, &aucFlavor[0]);

	/* Type 1. WIFI_MT7935_PHY_RAM_CODE_1_1_hdr.bin */
	ret = kalSnprintf(apucName[(*pucNameIdx)],
			  CFG_FW_NAME_MAX_LEN,
			  "WIFI_MT%x_PHY_RAM_CODE_%s_%u.bin",
			  MT7935_CHIP_ID,
			  aucFlavor,
			  MT7935_ROM_VERSION);
	if (ret < 0 || ret >= CFG_FW_NAME_MAX_LEN)
		DBGLOG(INIT, ERROR,
			"kalSnprintf failed, ret: %d\n", ret);
	else
		(*pucNameIdx) += 1;
}
#endif

#if (CFG_SUPPORT_FW_IDX_LOG_TRANS == 1)
static void mt7935_ConstructIdxLogBinName(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucName)
{
	int ret = 0;
	uint8_t aucFlavor[CFG_FW_FLAVOR_MAX_LEN];

	mt7935GetFlavorVer(prGlueInfo, &aucFlavor[0]);

	/* ex: WIFI_RAM_CODE_MT7935_2_1_idxlog.bin */
	ret = kalSnprintf(apucName[0],
			  CFG_FW_NAME_MAX_LEN,
			  "WIFI_RAM_CODE_MT%x_%s_%u_idxlog.bin",
			  MT7935_CHIP_ID,
			  aucFlavor,
			  MT7935_ROM_VERSION);

	if (ret < 0 || ret >= CFG_FW_NAME_MAX_LEN)
		DBGLOG(INIT, ERROR,
			"kalSnprintf failed, ret: %d\n", ret);
}
#endif /* CFG_SUPPORT_FW_IDX_LOG_TRANS */

#if (CFG_MTK_WIFI_SUPPORT_LEGACY_CBMCU_FWDL == 1)
static void mt7935ConstructCbmcuName(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucName, uint8_t *pucNameIdx)
{
	int ret = 0;
	uint8_t aucFlavor[CFG_FW_FLAVOR_MAX_LEN];

	kalMemZero(aucFlavor, sizeof(aucFlavor));
	mt7935GetFlavorVer(prGlueInfo, &aucFlavor[0]);

	/* Type 1. WIFI_MT7935_PHY_RAM_CODE_1_1_hdr.bin */
	ret = kalSnprintf(apucName[(*pucNameIdx)],
			  CFG_FW_NAME_MAX_LEN,
			  "CBMCU_CODE_MT%x_%s_%u.bin",
			  MT7935_CHIP_ID,
			  aucFlavor,
			  MT7935_ROM_VERSION);
	if (ret < 0 || ret >= CFG_FW_NAME_MAX_LEN)
		DBGLOG(INIT, ERROR,
			"construct cbmcu fw name fail, ret: %d\n", ret);
	else
		(*pucNameIdx) += 1;
}
#endif /* CFG_MTK_WIFI_SUPPORT_LEGACY_CBMCU_FWDL */

static uint32_t mt7935_wlanDownloadPatch(struct ADAPTER *prAdapter)
{
	uint32_t status  = wlanDownloadPatch(prAdapter);

	if (status == WLAN_STATUS_SUCCESS)
		wifi_coredump_set_enable(TRUE);

	return status;
}

static uint32_t mt7935_legacy_mcu_init(struct ADAPTER *prAdapter)
{
	uint32_t u4Status = mt7935_mcu_init(prAdapter);
	return u4Status;
}

static void mt7935_legacy_mcu_deinit(struct ADAPTER *prAdapter)
{
	mt7935_mcu_deinit(prAdapter);
}

#if CFG_ENABLE_FW_DOWNLOAD
struct FWDL_OPS_T mt7935_fw_dl_ops = {
	.constructFirmwarePrio = mt7935_ConstructFirmwarePrio,
	.constructPatchName = mt7935_ConstructPatchName,
#if CFG_SUPPORT_SINGLE_FW_BINARY
	.parseSingleBinaryFile = wlanParseSingleBinaryFile,
#endif
#if (CFG_SUPPORT_FW_IDX_LOG_TRANS == 1)
	.constrcutIdxLogBin = mt7935_ConstructIdxLogBinName,
#endif /* CFG_SUPPORT_FW_IDX_LOG_TRANS */
	.downloadPatch = mt7935_wlanDownloadPatch,
	.downloadFirmware = wlanConnacFormatDownload,
	.downloadByDynMemMap = NULL,
	.getFwInfo = wlanGetConnacFwInfo,
	.getFwDlInfo = asicGetFwDlInfo,
	.downloadEMI = wlanDownloadEMISectionViaDma,
#if (CFG_SUPPORT_PRE_ON_PHY_ACTION == 1)
	.phyAction = wlanPhyAction,
#else
	.phyAction = NULL,
#endif
#if defined(_HIF_PCIE)
	.mcu_init = mt7935_legacy_mcu_init,
	.mcu_deinit = mt7935_legacy_mcu_deinit,
#endif
#if CFG_SUPPORT_WIFI_DL_BT_PATCH
	.constructBtPatchName = asicConnac3xConstructBtPatchName,
	.downloadBtPatch = asicConnac3xDownloadBtPatch,
#if (CFG_SUPPORT_CONNAC3X == 1)
	.configBtImageSection = asicConnac3xConfigBtImageSection,
#endif
#endif
	.getFwVerInfo = wlanParseRamCodeReleaseManifest,
	.getFlavorVer = mt7935GetFlavorVer,
#if CFG_MTK_WIFI_SUPPORT_PHY_FWDL
	.constructPhyName = mt7935_ConstructPhyName,
	.downloadPhyFw  = wlanDownloadPhyFw,
#endif

#if (CFG_MTK_WIFI_SUPPORT_LEGACY_CBMCU_FWDL == 1)
	.constructCbmcuFwName = mt7935ConstructCbmcuName,
	.downloadCbmcuFw = wlanDownloadCbmcuFw,
#endif /* CFG_MTK_WIFI_SUPPORT_LEGACY_CBMCU_FWDL */

};
#endif /* CFG_ENABLE_FW_DOWNLOAD */

#endif  /* MT7935 */
