/******************************************************************************
 *
 * This file is provided under a dual license.  When you use or
 * distribute this software, you may choose to be licensed under
 * version 2 of the GNU General Public License ("GPLv2 License")
 * or BSD License.
 *
 * GPLv2 License
 *
 * Copyright(C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * BSD LICENSE
 *
 * Copyright(C) 2016 MediaTek Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/
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
#ifdef MT6639
#include "coda/mt6639/wf_wfdma_mcu_dma0.h"
#endif
#include "conn_dbg.h"

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
static char *wlanInitEventStatusCodeToStr(uint8_t ucStatus)
{
	switch (ucStatus) {
	case WIFI_FW_DOWNLOAD_SUCCESS:
		return "success";
	case WIFI_FW_DOWNLOAD_INVALID_PARAM:
		return "invalid param";
	case WIFI_FW_DOWNLOAD_INVALID_CRC:
		return "invalid crc";
	case WIFI_FW_DOWNLOAD_DECRYPTION_FAIL:
		return "decrypt fail";
	case WIFI_FW_DOWNLOAD_UNKNOWN_CMD:
		return "unknown";
	case WIFI_FW_DOWNLOAD_TIMEOUT:
		return "timeout";
	case WIFI_FW_DOWNLOAD_SEC_BOOT_CHK_FAIL:
		return "sec boot fail";
	default:
		return "unknown";
	}
}

#if CFG_ENABLE_FW_DOWNLOAD
uint32_t wlanGetDataMode(struct ADAPTER *prAdapter,
	enum ENUM_IMG_DL_IDX_T eDlIdx, uint8_t ucFeatureSet)
{
	uint32_t u4DataMode = 0;

	if (ucFeatureSet & FW_FEATURE_SET_ENCRY) {
		u4DataMode |= DOWNLOAD_CONFIG_RESET_OPTION;
		u4DataMode |= (ucFeatureSet &
			       FW_FEATURE_SET_KEY_MASK);
		u4DataMode |= DOWNLOAD_CONFIG_ENCRYPTION_MODE;
		if (ucFeatureSet & FW_FEATURE_ENCRY_MODE)
			u4DataMode |= DOWNLOAD_CONFIG_ENCRY_MODE_SEL;
	}

	if (eDlIdx == IMG_DL_IDX_CR4_FW)
		u4DataMode |= DOWNLOAD_CONFIG_WORKING_PDA_OPTION;

#if CFG_ENABLE_FW_DOWNLOAD_ACK
	u4DataMode |= DOWNLOAD_CONFIG_ACK_OPTION;	/* ACK needed */
#endif
	return u4DataMode;
}

uint32_t wlanGetPatchDataModeV2(struct ADAPTER *prAdapter,
	uint32_t u4SecInfo)
{
	uint32_t u4DataMode = 0;

#if CFG_ENABLE_FW_DOWNLOAD_ACK
	u4DataMode = DOWNLOAD_CONFIG_ACK_OPTION;	/* ACK needed */
#endif

	if (u4SecInfo == PATCH_SECINFO_NOT_SUPPORT)
		return u4DataMode;

	switch ((u4SecInfo & PATCH_SECINFO_ENC_TYPE_MASK) >>
		PATCH_SECINFO_ENC_TYPE_SHFT) {
	case PATCH_SECINFO_ENC_TYPE_PLAIN:
		break;
	case PATCH_SECINFO_ENC_TYPE_AES:
		u4DataMode |= DOWNLOAD_CONFIG_ENCRYPTION_MODE;
		u4DataMode |= ((u4SecInfo & PATCH_SECINFO_ENC_AES_KEY_MASK)
				 << DOWNLOAD_CONFIG_KEY_INDEX_SHFT) &
				DOWNLOAD_CONFIG_KEY_INDEX_MASK;
		u4DataMode |= DOWNLOAD_CONFIG_RESET_OPTION;
		break;
	case PATCH_SECINFO_ENC_TYPE_SCRAMBLE:
		u4DataMode |= DOWNLOAD_CONFIG_ENCRYPTION_MODE;
		u4DataMode |= DOWNLOAD_CONFIG_ENCRY_MODE_SEL;
		u4DataMode |= DOWNLOAD_CONFIG_RESET_OPTION;
		break;
	default:
		DBGLOG(INIT, ERROR, "Encryption type not support!\n");
	}

	return u4DataMode;
}

void wlanGetHarvardFwInfo(struct ADAPTER *prAdapter,
	uint8_t u4SecIdx, enum ENUM_IMG_DL_IDX_T eDlIdx,
	uint32_t *pu4Addr, uint32_t *pu4Len,
	uint32_t *pu4DataMode, u_int8_t *pfgIsEMIDownload,
	u_int8_t *pfgIsNotDownload)
{
	struct TAILER_FORMAT_T *prTailer;

	if (eDlIdx == IMG_DL_IDX_N9_FW)
		prTailer = &prAdapter->rVerInfo.rN9tailer[u4SecIdx];
	else
		prTailer = &prAdapter->rVerInfo.rCR4tailer[u4SecIdx];

	*pu4Addr = prTailer->addr;
	*pu4Len = (prTailer->len + LEN_4_BYTE_CRC);
	*pu4DataMode = wlanGetDataMode(prAdapter, eDlIdx,
				       prTailer->feature_set);
	*pfgIsEMIDownload = FALSE;
	*pfgIsNotDownload = FALSE;
}

void wlanGetConnacFwInfo(struct ADAPTER *prAdapter,
	uint8_t u4SecIdx, enum ENUM_IMG_DL_IDX_T eDlIdx,
	uint32_t *pu4Addr, uint32_t *pu4Len,
	uint32_t *pu4DataMode, u_int8_t *pfgIsEMIDownload,
	u_int8_t *pfgIsNotDownload)
{
	struct TAILER_REGION_FORMAT_T *prTailer =
			&prAdapter->rVerInfo.rRegionTailers[u4SecIdx];

	*pu4Addr = prTailer->u4Addr;
	*pu4Len = prTailer->u4Len;
	*pu4DataMode = wlanGetDataMode(prAdapter, eDlIdx,
				       prTailer->ucFeatureSet);
	*pfgIsEMIDownload = prTailer->ucFeatureSet &
			    DOWNLOAD_CONFIG_EMI;
	*pfgIsNotDownload = prTailer->ucFeatureSet &
			    FW_FEATURE_NOT_DOWNLOAD;
}

#if CFG_SUPPORT_COMPRESSION_FW_OPTION
void wlanImageSectionGetCompressFwInfo(struct ADAPTER
	*prAdapter, void *pvFwImageMapFile,
	uint32_t u4FwImageFileLength, uint8_t ucTotSecNum,
	uint8_t ucCurSecNum, enum ENUM_IMG_DL_IDX_T eDlIdx,
	uint32_t *pu4Addr, uint32_t *pu4Len,
	uint32_t *pu4DataMode, uint32_t *pu4BlockSize,
	uint32_t *pu4CRC, uint32_t *pu4UncompressedLength)
{
	struct FW_IMAGE_TAILER_T_2 *prFwHead;
	struct TAILER_FORMAT_T_2 *prTailer;
	uint8_t aucBuf[32];

	prFwHead = (struct FW_IMAGE_TAILER_T_2 *)
		   (pvFwImageMapFile + u4FwImageFileLength - sizeof(
			    struct FW_IMAGE_TAILER_T_2));
	if (ucTotSecNum == 1)
		prTailer = &prFwHead->dlm_info;
	else
		prTailer = &prFwHead->ilm_info;

	prTailer = &prTailer[ucCurSecNum];

	*pu4Addr = prTailer->addr;
	*pu4Len = (prTailer->len);
	*pu4BlockSize = (prTailer->block_size);
	*pu4CRC = (prTailer->crc);
	*pu4UncompressedLength = (prTailer->real_size);
	*pu4DataMode = wlanGetDataMode(prAdapter, eDlIdx,
				       prTailer->feature_set);

	/* Dump image information */
	if (ucCurSecNum == 0) {
		DBGLOG(INIT, INFO,
		       "%s INFO: chip_info[%u:E%u] feature[0x%02X]\n",
		       (eDlIdx == IMG_DL_IDX_N9_FW) ? "N9" : "CR4",
		       prTailer->chip_info,
		       prTailer->eco_code, prTailer->feature_set);
		kalMemZero(aucBuf, 32);
		kalStrnCpy(aucBuf, prTailer->ram_version,
			   sizeof(aucBuf) - 1);
		DBGLOG(INIT, INFO, "date[%s] version[%s]\n",
		       prTailer->ram_built_date, aucBuf);
	}
	/* Backup to FW version info */
	if (eDlIdx == IMG_DL_IDX_N9_FW) {
		kalMemCopy(&prAdapter->rVerInfo.rN9Compressedtailer,
			   prTailer, sizeof(struct TAILER_FORMAT_T_2));
		prAdapter->rVerInfo.fgIsN9CompressedFW = TRUE;
	} else {
		kalMemCopy(&prAdapter->rVerInfo.rCR4Compressedtailer,
			   prTailer, sizeof(struct TAILER_FORMAT_T_2));
		prAdapter->rVerInfo.fgIsCR4CompressedFW = TRUE;
	}
}
#endif

void wlanImageSectionGetPatchInfo(struct ADAPTER
	*prAdapter,
	void *pvFwImageMapFile, uint32_t u4FwImageFileLength,
	uint32_t *pu4StartOffset, uint32_t *pu4Addr,
	uint32_t *pu4Len,
	uint32_t *pu4DataMode)
{
	struct PATCH_FORMAT_T *prPatchFormat;
	uint8_t aucBuffer[32];
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;

	prPatchFormat = (struct PATCH_FORMAT_T *) pvFwImageMapFile;

	*pu4StartOffset = sizeof(*prPatchFormat);
	*pu4Addr = prChipInfo->patch_addr;
	*pu4Len = u4FwImageFileLength - sizeof(*prPatchFormat);
	*pu4DataMode = wlanGetDataMode(prAdapter, IMG_DL_IDX_PATCH,
				       0);

	/* Dump image information */
	kalMemZero(aucBuffer, 32);
	kalStrnCpy(aucBuffer, prPatchFormat->aucPlatform, 4);
	DBGLOG(INIT, INFO,
	       "PATCH INFO: platform[%s] HW/SW ver[0x%04X] ver[0x%04X]\n",
	       aucBuffer, prPatchFormat->u4SwHwVersion,
	       prPatchFormat->u4PatchVersion);

	kalStrnCpy(aucBuffer, prPatchFormat->aucBuildDate, 16);
	DBGLOG(INIT, INFO, "date[%s]\n", aucBuffer);

	/* Backup to FW version info */
	kalMemCopy(&prAdapter->rVerInfo.rPatchHeader, prPatchFormat,
		   sizeof(struct PATCH_FORMAT_T));
}

uint32_t wlanGetPatchInfoAndDownloadV2(struct ADAPTER
	*prAdapter,
	void *pvFwImageMapFile, uint32_t u4FwImageFileLength,
	enum ENUM_IMG_DL_IDX_T eDlIdx,
	uint32_t u4DataMode)
{
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;
	struct FWDL_OPS_T *prFwDlOps;
	struct PATCH_FORMAT_V2_T *prPatchFormat;
	uint8_t aucBuffer[32];
	struct PATCH_GLO_DESC *glo_desc;
	struct PATCH_SEC_MAP *sec_map;
	uint8_t *img_ptr;
	uint32_t num_of_region = 0, i;
	uint32_t sec_info = 0;
	struct patch_dl_target target = {0};

	prFwDlOps = prChipInfo->fw_dl_ops;
	if (!prFwDlOps) {
		DBGLOG(INIT, ERROR, "prFwDlOps is NULL!!\n");

		return WLAN_STATUS_FAILURE;
	}

	if (pvFwImageMapFile == NULL) {
		DBGLOG(INIT, ERROR, "pvFwImageMapFile is NULL!!\n");

		return WLAN_STATUS_FAILURE;
	}
	/* patch header */
	img_ptr = pvFwImageMapFile;
	prPatchFormat = (struct PATCH_FORMAT_V2_T *)img_ptr;

	/* Dump image information */
	kalMemZero(aucBuffer, 32);
	kalStrnCpy(aucBuffer, prPatchFormat->aucPlatform, 4);
	DBGLOG(INIT, INFO,
	       "PATCH INFO: platform[%s] HW/SW ver[0x%04X] ver[0x%04X]\n",
	       aucBuffer, prPatchFormat->u4SwHwVersion,
	       prPatchFormat->u4PatchVersion);

	kalStrnCpy(aucBuffer, prPatchFormat->aucBuildDate, 16);
	DBGLOG(INIT, INFO, "date[%s]\n", aucBuffer);

	/* Backup to FW version info */
	kalMemCopy(&prAdapter->rVerInfo.rPatchHeader, prPatchFormat,
		   sizeof(struct PATCH_FORMAT_T));

	/* global descriptor */
	img_ptr += sizeof(struct PATCH_FORMAT_V2_T);
	glo_desc = (struct PATCH_GLO_DESC *)img_ptr;
	num_of_region = be2cpu32(glo_desc->section_num);
	DBGLOG(INIT, INFO,
			"\tPatch ver: 0x%x, Section num: 0x%x, subsys: 0x%x\n",
			glo_desc->patch_ver,
			num_of_region,
			be2cpu32(glo_desc->subsys));

	if (num_of_region > FW_MAX_SECTION_NUM) {
		DBGLOG(INIT, ERROR,
			"num_of_region is bigger than max section number!\n");

		return WLAN_STATUS_FAILURE;
	}

	/* section map */
	img_ptr += sizeof(struct PATCH_GLO_DESC);

	target.patch_region = (struct patch_dl_buf *)kalMemAlloc(
		num_of_region * sizeof(struct patch_dl_buf), PHY_MEM_TYPE);

	if (!target.patch_region) {
		DBGLOG(INIT, WARN,
			"parse patch failed!No memory to allocate.\n");
		return WLAN_STATUS_FAILURE;
	}
	target.num_of_region = num_of_region;

	for (i = 0; i < num_of_region; i++) {
		struct patch_dl_buf *region;
		uint32_t section_type;

		region = &target.patch_region[i];
		sec_map = (struct PATCH_SEC_MAP *)img_ptr;
		img_ptr += sizeof(struct PATCH_SEC_MAP);

		section_type = be2cpu32(sec_map->section_type);
		DBGLOG(INIT, INFO,
			"\tSection %d: type = 0x%x, offset = 0x%x, size = 0x%x\n",
			i, section_type, be2cpu32(sec_map->section_offset),
			be2cpu32(sec_map->section_size));

		if ((section_type & PATCH_SEC_TYPE_MASK) ==
			PATCH_SEC_TYPE_BIN_INFO) {
			region->img_dest_addr =
				be2cpu32(sec_map->bin_info_spec.dl_addr);
			region->img_size =
				be2cpu32(sec_map->bin_info_spec.dl_size);
			region->img_ptr = (uint8_t *)(
				(uintptr_t)pvFwImageMapFile +
				be2cpu32(sec_map->section_offset));
			sec_info = be2cpu32(sec_map->bin_info_spec.sec_info);

			DBGLOG(INIT, INFO,
				"\tTarget address: 0x%x, length: 0x%x\n",
				region->img_dest_addr, region->img_size);
		} else {
			region->img_ptr = NULL;
			DBGLOG(INIT, INFO, "\tNot binary\n");
		}
	}

	u4DataMode = wlanGetPatchDataModeV2(prAdapter, sec_info);

	DBGLOG(INIT, INFO,
		"FormatV2 num_of_regoin[%d] datamode[0x%08x]\n",
		target.num_of_region, u4DataMode);

	u4Status = wlanDownloadSectionV2(prAdapter,
		u4DataMode, eDlIdx, &target);

	if (target.patch_region)
		kalMemFree(target.patch_region, PHY_MEM_TYPE,
			num_of_region * sizeof(struct patch_dl_buf));

	return u4Status;
}

#if CFG_SUPPORT_SINGLE_FW_BINARY
uint32_t wlanParseSingleBinaryFile(void *pvFileBuf,
	uint32_t u4FileLength, void **ppvIdxFileBuf,
	uint32_t *pu4IdxFileLength, enum ENUM_IMG_DL_IDX_T eDlIdx)
{
	struct HEADER_SINGLE_BINARY *prHeader;
	uint32_t u4FixedHeaderSize, u4DynamicHeaderSize;
	void *u4FilePtr;
	void *u4FileOffsetPtr;
	void *u4FileLengthPtr;
	uint32_t u4IdxFileOffset;
	uint32_t u4IdxFileLength;
	uint32_t u4NextIdxFileOffset;
	uint32_t ret = WLAN_STATUS_SUCCESS;
	uint32_t u4Idx = 0;
	uint32_t i;
	void *pvFwBuffer = NULL;

	*ppvIdxFileBuf = NULL;
	*pu4IdxFileLength = 0;

	prHeader = (struct HEADER_SINGLE_BINARY *) pvFileBuf;

	for (i = 0; i < sizeof(prHeader->aucFileType); i++) {
		if (prHeader->aucFileType[i] == eDlIdx) {
			u4Idx = i;
			break;
		}
	}

	if (i >= sizeof(prHeader->aucFileType)) {
		DBGLOG(INIT, WARN,
			"No match file type\n");
		ret = WLAN_STATUS_NOT_SUPPORTED;
		goto exit;
	}

	if (u4Idx >= prHeader->u4FileNumber) {
		DBGLOG(INIT, WARN,
			"File number not match\n");
		ret = WLAN_STATUS_NOT_SUPPORTED;
		goto exit;
	}

	u4FixedHeaderSize = sizeof(struct HEADER_SINGLE_BINARY);
	/* Per file has 2 uint32_t in Dynamic Header,
	 * 1 for file offset, 1 for file length
	 * and extra 1 uint32_t for end padding, 0xEDED
	 */
	u4DynamicHeaderSize =
		((prHeader->u4FileNumber * 2) + 1) * sizeof(uint32_t);

	u4FileOffsetPtr = pvFileBuf +
			u4FixedHeaderSize +
			u4Idx * 2 * sizeof(uint32_t);
	u4FileLengthPtr = u4FileOffsetPtr + sizeof(uint32_t);
	u4IdxFileOffset = *((uint32_t *)u4FileOffsetPtr);
	u4IdxFileLength = *((uint32_t *)u4FileLengthPtr);
	u4FilePtr = pvFileBuf + u4FixedHeaderSize +
			u4DynamicHeaderSize +
			u4IdxFileOffset;

	if (u4Idx == prHeader->u4FileNumber - 1)
		u4NextIdxFileOffset = u4FileLength -
				u4FixedHeaderSize -
				u4DynamicHeaderSize;
	else
		u4NextIdxFileOffset =
			*((uint32_t *)(u4FileOffsetPtr +
			2 * sizeof(uint32_t)));

	if (u4IdxFileLength != u4NextIdxFileOffset - u4IdxFileOffset) {
		DBGLOG(INIT, WARN,
			"File length not match\n");
		ret = WLAN_STATUS_NOT_SUPPORTED;
		goto exit;
	}

	pvFwBuffer = kalMemAlloc(u4IdxFileLength, VIR_MEM_TYPE);
	if (!pvFwBuffer) {
		DBGLOG(INIT, ERROR, "vmalloc(%u) failed\n",
			u4IdxFileLength);
		ret = WLAN_STATUS_RESOURCES;
		goto exit;
	}

	kalMemCopy(pvFwBuffer, u4FilePtr, u4IdxFileLength);
	*ppvIdxFileBuf = pvFwBuffer;
	*pu4IdxFileLength = u4IdxFileLength;

exit:
	return ret;
}
#endif

uint32_t wlanDownloadSection(struct ADAPTER *prAdapter,
			     uint32_t u4Addr, uint32_t u4Len,
			     uint32_t u4DataMode, uint8_t *pucStartPtr,
			     enum ENUM_IMG_DL_IDX_T eDlIdx)
{
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	struct BUS_INFO *prBusInfo = NULL;
#endif

	if (wlanImageSectionConfig(prAdapter, u4Addr, u4Len,
				   u4DataMode, eDlIdx) != WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, ERROR,
		       "Firmware download configuration failed!\n");
		return WLAN_STATUS_FAILURE;
	}

#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	prBusInfo = prAdapter->chip_info->bus_info;
	if (prBusInfo->enableFwDlMode)
		prBusInfo->enableFwDlMode(prAdapter);
#endif

	if (wlanImageSectionDownload(prAdapter, pucStartPtr,
				     u4Len) !=
				     WLAN_STATUS_SUCCESS)
		return WLAN_STATUS_FAILURE;

	return WLAN_STATUS_SUCCESS;
}

uint32_t wlanDownloadSectionV2(struct ADAPTER *prAdapter,
		uint32_t u4DataMode,
		enum ENUM_IMG_DL_IDX_T eDlIdx,
		struct patch_dl_target *target)
{
	uint32_t num_of_region, i;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;

	num_of_region = target->num_of_region;
	for (i = 0; i < num_of_region; i++) {
		struct patch_dl_buf *region;

		region = &target->patch_region[i];
		if (region->img_ptr == NULL)
			continue;

		/* 2. config PDA */
#if (CFG_SUPPORT_CONNAC3X == 1)
#if CFG_SUPPORT_WIFI_DL_BT_PATCH
		if (eDlIdx == IMG_DL_IDX_BT_PATCH) {
			struct FWDL_OPS_T *prFwDlOps;

			prFwDlOps = prAdapter->chip_info->fw_dl_ops;
			if (prFwDlOps->configBtImageSection)
				if (prFwDlOps->configBtImageSection(
					prAdapter, region) !=
					WLAN_STATUS_SUCCESS) {
				DBGLOG(INIT, ERROR,
					"Firmware download configuration failed!\n");
				u4Status = WLAN_STATUS_FAILURE;
				goto out;
			}
		} else
#endif
#endif
		if (wlanImageSectionConfig(prAdapter, region->img_dest_addr,
			region->img_size, u4DataMode, eDlIdx) !=
			WLAN_STATUS_SUCCESS) {
			DBGLOG(INIT, ERROR,
				"Firmware download configuration failed!\n");
			u4Status = WLAN_STATUS_FAILURE;
			goto out;
		}

		/* 3. image scatter */
		u4Status = wlanImageSectionDownload(prAdapter,
						    region->img_ptr,
						    region->img_size);
		if (u4Status != WLAN_STATUS_SUCCESS) {
			u4Status = WLAN_STATUS_FAILURE;
			break;
		}
	}

out:

	return u4Status;
}

uint32_t wlanDownloadEMISection(struct ADAPTER *prAdapter,
	uint32_t u4DestAddr,
	uint32_t u4DataMode,
	uint8_t *pucStartPtr,
	uint32_t u4Len)
{
	emi_mem_write(prAdapter->chip_info, u4DestAddr, pucStartPtr, u4Len);
	return WLAN_STATUS_SUCCESS;
}

static uint32_t wlanEmiSectionGetBufSize(struct ADAPTER *prAdapter,
	uint32_t *pu4Size)
{
	struct INIT_CMD_QUERY_INFO rCmd = {0};
	struct INIT_EVENT_QUERY_INFO *prEvent = NULL;
	struct INIT_EVENT_TLV_GENERAL *prTlv = NULL;
	struct INIT_EVENT_QUERY_INFO_FWDL_EMI_SIZE *prEmiEvent = NULL;
	uint32_t u4EventSize;
	uint32_t u4Status = WLAN_STATUS_FAILURE;

	rCmd.u4QueryBitmap = BIT(INIT_CMD_QUERY_TYPE_FWDL_EMI_SIZE);

	u4EventSize = sizeof(struct INIT_EVENT_QUERY_INFO) +
		sizeof(struct INIT_EVENT_TLV_GENERAL) +
		sizeof(struct INIT_EVENT_QUERY_INFO_FWDL_EMI_SIZE);
	prEvent = kalMemAlloc(u4EventSize, VIR_MEM_TYPE);
	if (!prEvent) {
		DBGLOG(INIT, ERROR, "Allocate event packet FAILED.\n");
		goto exit;
	}

	u4Status = wlanSendInitSetQueryCmd(prAdapter,
		INIT_CMD_ID_QUERY_INFO, &rCmd, sizeof(rCmd),
		TRUE, FALSE,
		INIT_EVENT_ID_QUERY_INFO_RESULT, prEvent, u4EventSize);
	if (u4Status != WLAN_STATUS_SUCCESS)
		goto exit;

	if (prEvent->u2TotalElementNum != 1) {
		DBGLOG(INIT, ERROR, "Unexpected element num: %d.\n",
			prEvent->u2TotalElementNum);
		goto exit;
	}

	prTlv = (struct INIT_EVENT_TLV_GENERAL *)&prEvent->aucTlvBuffer[0];
	if (prTlv->u2Tag != INIT_CMD_QUERY_TYPE_FWDL_EMI_SIZE) {
		DBGLOG(INIT, ERROR, "Unexpected tag id: %d.\n",
			prTlv->u2Tag);
		goto exit;
	}

	prEmiEvent = (struct INIT_EVENT_QUERY_INFO_FWDL_EMI_SIZE *)
		&prTlv->aucBuffer[0];
	/* sanity check */
	if (prEmiEvent->u4Length <= CMD_PKT_SIZE_FOR_IMAGE) {
		DBGLOG(INIT, ERROR, "Invalid emi buffer size(%d).\n",
			prEmiEvent->u4Length);
		goto exit;
	}

	DBGLOG(INIT, INFO, "u4Length: 0x%x\n", prEmiEvent->u4Length);
	*pu4Size = prEmiEvent->u4Length;

	u4Status = WLAN_STATUS_SUCCESS;

exit:
	if (prEvent)
		kalMemFree(prEvent, VIR_MEM_TYPE, u4EventSize);

	return u4Status;
}

static uint32_t wlanEmiSectionDlConfig(struct ADAPTER *prAdapter,
	uint32_t u4DestAddr,
	uint32_t u4DataMode,
	uint32_t u4Len)
{
	struct INIT_CMD_EMI_FW_DOWNLOAD_CONFIG rCmd = {
		.u4Address = u4DestAddr,
		.u4Length = u4Len,
		.u4DataMode = u4DataMode,
	};
	struct INIT_EVENT_CMD_RESULT rEvt = {0};
	uint32_t u4Status = WLAN_STATUS_SUCCESS;

	u4Status = wlanSendInitSetQueryCmd(prAdapter,
		INIT_CMD_ID_EMI_FW_DOWNLOAD_CONFIG, &rCmd, sizeof(rCmd),
		TRUE, FALSE,
		INIT_EVENT_ID_CMD_RESULT, &rEvt, sizeof(rEvt));

	if (u4Status != WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, ERROR, "Send query cmd failed: %u\n", u4Status);
	} else if (rEvt.ucStatus != 0) {
		DBGLOG(INIT, ERROR, "Error status: %d\n", rEvt.ucStatus);
		u4Status = WLAN_STATUS_FAILURE;
	}

	return u4Status;
}

static uint32_t wlanEmiSectionStartCmd(struct ADAPTER *prAdapter,
	uint32_t u4DestAddr,
	uint32_t u4Len,
	u_int8_t fgLast)
{
	struct INIT_CMD_EMI_FW_TRIGGER_AXI_DMA rCmd = {
		.u4DownloadSize = u4Len,
		.ucDoneBit = fgLast,
	};
	struct INIT_EVENT_CMD_RESULT rEvt = {0};
	uint32_t u4Status = WLAN_STATUS_SUCCESS;

	u4Status = wlanSendInitSetQueryCmd(prAdapter,
		INIT_CMD_ID_EMI_FW_TRIGGER_AXI_DMA, &rCmd, sizeof(rCmd),
		TRUE, FALSE,
		INIT_EVENT_ID_CMD_RESULT, &rEvt, sizeof(rEvt));

	if (u4Status != WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, ERROR, "Send query cmd failed: %u\n", u4Status);
	} else if (rEvt.ucStatus != 0) {
		DBGLOG(INIT, ERROR, "Error status: %d\n", rEvt.ucStatus);
		u4Status = WLAN_STATUS_FAILURE;
	}

	return u4Status;
}

uint32_t wlanDownloadEMISectionViaDma(struct ADAPTER *prAdapter,
	uint32_t u4DestAddr,
	uint32_t u4DataMode,
	uint8_t *pucStartPtr,
	uint32_t u4Len)
{
	uint8_t *prImgPos = pucStartPtr, *prImgEnd = pucStartPtr + u4Len;
	uint32_t u4UmacSz = 0;
	uint32_t u4Status = WLAN_STATUS_FAILURE;

	if (u4Len == 0)
		goto exit;

	if (wlanEmiSectionGetBufSize(prAdapter, &u4UmacSz) !=
			WLAN_STATUS_SUCCESS)
		goto exit;

	while (prImgPos != prImgEnd) {
		uint8_t *prSecStart, *prSecEnd;
		uint32_t u4SecLen;

		prSecStart = prImgPos;
		prSecEnd = prSecStart + u4UmacSz > prImgEnd ?
			prImgEnd : prSecStart + u4UmacSz;
		u4SecLen = prSecEnd - prSecStart;

		if (wlanEmiSectionDlConfig(prAdapter, u4DestAddr, u4DataMode,
					   u4SecLen) != WLAN_STATUS_SUCCESS)
			goto exit;

		if (wlanImageSectionDownload(prAdapter, prSecStart,
					     u4SecLen) !=
				WLAN_STATUS_SUCCESS)
			goto exit;

		if (wlanEmiSectionStartCmd(prAdapter, u4DestAddr, u4SecLen,
					   (prSecEnd == prImgEnd) ?
						TRUE : FALSE) !=
				WLAN_STATUS_SUCCESS)
			goto exit;

		prImgPos += u4SecLen;
	}
	u4Status = WLAN_STATUS_SUCCESS;

exit:
	return u4Status;
}

#if CFG_SUPPORT_COMPRESSION_FW_OPTION
u_int8_t wlanImageSectionCheckFwCompressInfo(
	struct ADAPTER *prAdapter,
	void *pvFwImageMapFile, uint32_t u4FwImageFileLength,
	enum ENUM_IMG_DL_IDX_T eDlIdx)
{
	uint8_t ucCompression;
	struct FW_IMAGE_TAILER_CHECK *prCheckInfo;

	if (eDlIdx == IMG_DL_IDX_PATCH)
		return FALSE;

	prCheckInfo = (struct FW_IMAGE_TAILER_CHECK *)
		      (pvFwImageMapFile + u4FwImageFileLength - sizeof(
			       struct FW_IMAGE_TAILER_CHECK));
	DBGLOG(INIT, INFO, "feature_set %d\n",
	       prCheckInfo->feature_set);
	ucCompression = (uint8_t)((prCheckInfo->feature_set &
				   COMPRESSION_OPTION_MASK)
				  >> COMPRESSION_OPTION_OFFSET);
	DBGLOG(INIT, INFO, "Compressed Check INFORMATION %d\n",
	       ucCompression);
	if (ucCompression == 1) {
		DBGLOG(INIT, INFO, "Compressed FW\n");
		return TRUE;
	}
	return FALSE;
}

uint32_t wlanCompressedImageSectionDownloadStage(
	struct ADAPTER *prAdapter, void *pvFwImageMapFile,
	uint32_t u4FwImageFileLength, uint8_t ucSectionNumber,
	enum ENUM_IMG_DL_IDX_T eDlIdx,
	uint8_t *pucIsCompressed,
	struct INIT_CMD_WIFI_DECOMPRESSION_START *prFwImageInFo)
{
	uint32_t i;
	int32_t  i4TotalLen;
	uint32_t u4FileOffset = 0;
	uint32_t u4StartOffset = 0;
	uint32_t u4DataMode = 0;
	uint32_t u4Addr, u4Len, u4BlockSize, u4CRC;
	uint32_t u4UnCompressedLength;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
	uint8_t *pucStartPtr;
	uint32_t u4offset = 0, u4ChunkSize;
	u_int8_t fgIsDynamicMemMap = FALSE;

	/* 3a. parse file header for decision of
	 * divided firmware download or not
	 */
	if (wlanImageSectionCheckFwCompressInfo(prAdapter,
						pvFwImageMapFile,
						u4FwImageFileLength,
						eDlIdx) == TRUE) {
		for (i = 0; i < ucSectionNumber; ++i) {
			wlanImageSectionGetCompressFwInfo(prAdapter,
				pvFwImageMapFile,
				u4FwImageFileLength, ucSectionNumber, i, eDlIdx,
				&u4Addr, &u4Len, &u4DataMode,
				&u4BlockSize, &u4CRC, &u4UnCompressedLength);
			u4offset = 0;
			if (i == 0) {
				prFwImageInFo->u4BlockSize = u4BlockSize;
				prFwImageInFo->u4Region1Address = u4Addr;
				prFwImageInFo->u4Region1CRC = u4CRC;
				prFwImageInFo->u4Region1length =
					u4UnCompressedLength;
			} else {
				prFwImageInFo->u4Region2Address = u4Addr;
				prFwImageInFo->u4Region2CRC = u4CRC;
				prFwImageInFo->u4Region2length =
					u4UnCompressedLength;
			}
			i4TotalLen = u4Len;
			DBGLOG(INIT, INFO,
			       "DL Offset[%u] addr[0x%08x] len[%u] datamode[0x%08x]\n",
			       u4FileOffset, u4Addr, u4Len, u4DataMode);
			DBGLOG(INIT, INFO, "DL BLOCK[%u]  COMlen[%u] CRC[%u]\n",
			       u4BlockSize, u4UnCompressedLength, u4CRC);
			pucStartPtr =
				(uint8_t *) pvFwImageMapFile + u4StartOffset;
			while (i4TotalLen) {
				u4ChunkSize =  *((unsigned int *)(pucStartPtr +
					u4FileOffset));
				u4FileOffset += 4;
				DBGLOG(INIT, INFO,
					"Downloaded Length %d! Addr %x\n",
				  i4TotalLen, u4Addr + u4offset);
				DBGLOG(INIT, INFO,
					"u4ChunkSize Length %d!\n",
					u4ChunkSize);

				u4Status = wlanDownloadSection(prAdapter,
					u4Addr + u4offset,
					u4ChunkSize,
					u4DataMode,
					pvFwImageMapFile + u4FileOffset,
					eDlIdx);
				/* escape from loop if any
				 * pending error occurs
				 */
				if (u4Status == WLAN_STATUS_FAILURE)
					break;

				i4TotalLen -= u4ChunkSize;
				u4offset += u4BlockSize;
				u4FileOffset += u4ChunkSize;
				if (i4TotalLen < 0) {
					DBGLOG(INIT, ERROR,
						"Firmware scatter download failed!\n");
					u4Status = WLAN_STATUS_FAILURE;
					break;
				}
			}
		}
		*pucIsCompressed = TRUE;
	} else {
		u4Status = wlanImageSectionDownloadStage(prAdapter,
				pvFwImageMapFile, u4FwImageFileLength,
				ucSectionNumber, eDlIdx, &fgIsDynamicMemMap);
		*pucIsCompressed = FALSE;
	}
	return u4Status;
}
#endif

uint32_t wlanImageSectionDownloadStage(
	struct ADAPTER *prAdapter, void *pvFwImageMapFile,
	uint32_t u4FwImageFileLength, uint8_t ucSectionNumber,
	enum ENUM_IMG_DL_IDX_T eDlIdx, u_int8_t *pfgIsDynamicMemMap)
{
	uint32_t u4SecIdx, u4Offset = 0;
	uint32_t u4Addr, u4Len, u4DataMode = 0;
	u_int8_t fgIsEMIDownload = FALSE;
	u_int8_t fgIsNotDownload = FALSE;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;
	struct PATCH_FORMAT_T *prPatchHeader;
	struct FWDL_OPS_T *prFwDlOps;

	*pfgIsDynamicMemMap = FALSE;
	prFwDlOps = prChipInfo->fw_dl_ops;

	/* 3a. parse file header for decision of
	 * divided firmware download or not
	 */
	if (eDlIdx == IMG_DL_IDX_PATCH) {
		prPatchHeader = pvFwImageMapFile;
		if (prPatchHeader->u4PatchVersion == PATCH_VERSION_MAGIC_NUM) {
			u4Status = wlanGetPatchInfoAndDownloadV2(prAdapter,
				pvFwImageMapFile,
				u4FwImageFileLength,
				eDlIdx,
				u4DataMode);
		} else {
			wlanImageSectionGetPatchInfo(prAdapter,
				pvFwImageMapFile,
					     u4FwImageFileLength,
					     &u4Offset, &u4Addr,
					     &u4Len, &u4DataMode);
			DBGLOG_LIMITED(INIT, INFO,
		"FormatV1 DL Offset[%u] addr[0x%08x] len[%u] datamode[0x%08x]\n",
		       u4Offset, u4Addr, u4Len, u4DataMode);
/* For dynamic memory map::Begin */
#if (CFG_DOWNLOAD_DYN_MEMORY_MAP == 1)
			u4Status = prFwDlOps->downloadByDynMemMap(
						prAdapter, u4Addr, u4Len,
						(uint8_t *)(
						(uintptr_t)pvFwImageMapFile
						+ u4Offset),
						eDlIdx);
			*pfgIsDynamicMemMap = TRUE;
#else
			u4Status = wlanDownloadSection(
						prAdapter,
						u4Addr,
						u4Len,
						u4DataMode,
						(uint8_t *)(
						(uintptr_t)pvFwImageMapFile
						+ u4Offset),
						eDlIdx);
#endif
			if (prFwDlOps->setup_date_info)
				prFwDlOps->setup_date_info(prAdapter,
					eDlIdx,
					prPatchHeader->aucBuildDate);
		}
/* For dynamic memory map::End */
	} else if ((eDlIdx == IMG_DL_IDX_MCU_ROM_EMI) ||
		   (eDlIdx == IMG_DL_IDX_WIFI_ROM_EMI)) {
		struct ROM_EMI_HEADER *prRomEmiHeader =
			(struct ROM_EMI_HEADER *)pvFwImageMapFile;

		DBGLOG(INIT, INFO,
			"DL ROM EMI idx=%d, date=%s\n",
			eDlIdx,
			prRomEmiHeader->aucBuildDate);

		u4Addr = prRomEmiHeader->u4PatchAddr;
		u4Len = u4FwImageFileLength - sizeof(struct ROM_EMI_HEADER);
		u4Offset = sizeof(struct ROM_EMI_HEADER);
		u4Status = prFwDlOps->downloadEMI(prAdapter,
				u4Addr,
				0,
				(uint8_t *)(
				(uintptr_t)pvFwImageMapFile + u4Offset),
				u4Len);
		if (prFwDlOps->setup_date_info)
			prFwDlOps->setup_date_info(prAdapter,
				eDlIdx,
				prRomEmiHeader->aucBuildDate);
	} else {
		for (u4SecIdx = 0; u4SecIdx < ucSectionNumber;
		     u4SecIdx++, u4Offset += u4Len) {
			prChipInfo->fw_dl_ops->getFwInfo(prAdapter, u4SecIdx,
				eDlIdx, &u4Addr,
				&u4Len, &u4DataMode, &fgIsEMIDownload,
				&fgIsNotDownload);

			DBGLOG(INIT, TRACE,
			       "DL Offset[%u] addr[0x%08x] len[%u] datamode[0x%08x]\n",
			       u4Offset, u4Addr, u4Len, u4DataMode);

			if (fgIsNotDownload)
				continue;
			else if (fgIsEMIDownload)
				u4Status = prFwDlOps->downloadEMI(prAdapter,
					u4Addr,
					u4DataMode,
					(uint8_t *)(
					(uintptr_t)pvFwImageMapFile + u4Offset),
					u4Len);
/* For dynamic memory map:: Begin */
#if (CFG_DOWNLOAD_DYN_MEMORY_MAP == 1)
			else if ((u4DataMode &
				DOWNLOAD_CONFIG_ENCRYPTION_MODE) == 0) {
				/* Non-encrypted F/W region,
				 * use dynamic memory mapping for download
				 */
				*pfgIsDynamicMemMap = TRUE;
				u4Status = prFwDlOps->downloadByDynMemMap(
					prAdapter,
					u4Addr,
					u4Len,
					pvFwImageMapFile + u4Offset,
					eDlIdx);
			}
#endif
/* For dynamic memory map:: End */
			else
				u4Status = wlanDownloadSection(prAdapter,
					u4Addr, u4Len,
					u4DataMode,
					(uint8_t *)(
					(uintptr_t)pvFwImageMapFile
					+ u4Offset),
					eDlIdx);

			/* escape from loop if any pending error occurs */
			if (u4Status == WLAN_STATUS_FAILURE)
				break;
		}
	}
	return u4Status;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called to check the patch semaphore control.
 *
 * @param prAdapter      Pointer to the Adapter structure.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanPatchSendSemaControl(struct ADAPTER *prAdapter,
	uint8_t *pucPatchStatus)
{
	struct INIT_CMD_PATCH_SEMA_CONTROL rCmd = {0};
	struct INIT_EVENT_CMD_RESULT rEvent = {0};
	uint32_t u4Status = WLAN_STATUS_SUCCESS;

	DEBUGFUNC("wlanImagePatchSemaphoreCheck");

	rCmd.ucGetSemaphore = PATCH_GET_SEMA_CONTROL;

	u4Status = wlanSendInitSetQueryCmd(prAdapter,
		INIT_CMD_ID_PATCH_SEMAPHORE_CONTROL, &rCmd, sizeof(rCmd),
		TRUE, FALSE,
		INIT_EVENT_ID_PATCH_SEMA_CTRL, &rEvent, sizeof(rEvent));
	if (u4Status != WLAN_STATUS_SUCCESS)
		goto exit;

	*pucPatchStatus = rEvent.ucStatus;

exit:
	return u4Status;
}

u_int8_t wlanPatchIsDownloaded(struct ADAPTER *prAdapter)
{
	uint8_t ucPatchStatus = PATCH_STATUS_NO_SEMA_NEED_PATCH;
	uint32_t u4Count = 0;
	uint32_t rStatus;

	while (ucPatchStatus == PATCH_STATUS_NO_SEMA_NEED_PATCH) {
		if (u4Count)
			kalMdelay(100);

		rStatus = wlanPatchSendSemaControl(prAdapter, &ucPatchStatus);
		if (rStatus != WLAN_STATUS_SUCCESS)
			break;

		u4Count++;

		if (u4Count > 50) {
			DBGLOG(INIT, WARN, "Patch status check timeout!!\n");
			break;
		}
	}

	if (ucPatchStatus == PATCH_STATUS_NO_NEED_TO_PATCH) {
#if CFG_SUPPORT_WIFI_DL_BT_PATCH || CFG_SUPPORT_WIFI_DL_ZB_PATCH
		prAdapter->fgIsNeedDlPatch = FALSE;
#endif
		return TRUE;
	} else {
#if CFG_SUPPORT_WIFI_DL_BT_PATCH || CFG_SUPPORT_WIFI_DL_ZB_PATCH
		prAdapter->fgIsNeedDlPatch = TRUE;
#endif
		return FALSE;
	}
}

uint32_t wlanPatchSendComplete(struct ADAPTER *prAdapter
#if CFG_SUPPORT_WIFI_DL_BT_PATCH || CFG_SUPPORT_WIFI_DL_ZB_PATCH
			       , uint8_t ucPatchType
#endif
				)
{
	struct INIT_CMD_PATCH_FINISH rCmd = {0};
	struct INIT_EVENT_CMD_RESULT rEvent = {0};
	u_int8_t fgSkipCheckSeq = FALSE;
	uint32_t u4Status;

	rCmd.ucCheckCrc = 0;
#if CFG_SUPPORT_WIFI_DL_BT_PATCH || CFG_SUPPORT_WIFI_DL_ZB_PATCH
	rCmd.ucType = ucPatchType;
#endif

#if CFG_SUPPORT_WIFI_DL_BT_PATCH || CFG_SUPPORT_WIFI_DL_ZB_PATCH
	/* BT always response with ucCmdSeqNum=0 */
	if ((ucPatchType != PATCH_FNSH_TYPE_WF) &&
	    (ucPatchType != PATCH_FNSH_TYPE_WF_MD))
		fgSkipCheckSeq = TRUE;
#endif

	u4Status = wlanSendInitSetQueryCmd(prAdapter,
		INIT_CMD_ID_PATCH_FINISH, &rCmd, sizeof(rCmd),
		TRUE, fgSkipCheckSeq,
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
		DBGLOG(INIT, INFO, "PATCH FINISH EVT success!!\n");

	return u4Status;
}

#if (CFG_DOWNLOAD_DYN_MEMORY_MAP == 1)
uint32_t wlanPatchDynMemMapSendComplete(struct ADAPTER *prAdapter)
{
	struct INIT_CMD_PATCH_FINISH rCmd = {0};
	struct INIT_EVENT_CMD_RESULT rEvent = {0};
	uint32_t u4Status;

	rCmd.ucCheckCrc = 0;

	u4Status = wlanSendInitSetQueryCmd(prAdapter,
		INIT_CMD_ID_DYN_MEM_MAP_PATCH_FINISH, &rCmd, sizeof(rCmd),
		TRUE, FALSE,
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
		DBGLOG(INIT, INFO, "PATCH FINISH EVT success!!\n");

	return u4Status;
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called to configure FWDL parameters
 *
 * @param prAdapter      Pointer to the Adapter structure.
 *        u4DestAddr     Address of destination address
 *        u4ImgSecSize   Length of the firmware block
 *        fgReset        should be set to TRUE if this is the 1st configuration
 *
 * @return WLAN_STATUS_SUCCESS
 *         WLAN_STATUS_FAILURE
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanImageSectionConfig(
	struct ADAPTER *prAdapter,
	uint32_t u4DestAddr, uint32_t u4ImgSecSize,
	uint32_t u4DataMode,
	enum ENUM_IMG_DL_IDX_T eDlIdx)
{
	struct INIT_CMD_DOWNLOAD_CONFIG rCmd = {0};
	struct INIT_EVENT_CMD_RESULT rEvent = {0};
	uint8_t ucCmdId;
	u_int8_t fgCheckStatus = FALSE;
	uint32_t u4Status;

	rCmd.u4Address = u4DestAddr;
	rCmd.u4Length = u4ImgSecSize;
	rCmd.u4DataMode = u4DataMode;

	switch (eDlIdx) {
#if CFG_SUPPORT_WIFI_DL_BT_PATCH
	case IMG_DL_IDX_BT_PATCH:
		/* fallthrough */
#endif
#if CFG_SUPPORT_WIFI_DL_ZB_PATCH
	case IMG_DL_IDX_ZB_PATCH:
		/* fallthrough */
#endif
	case IMG_DL_IDX_PATCH:
		ucCmdId = INIT_CMD_ID_PATCH_START;
		break;
	default:
		ucCmdId = INIT_CMD_ID_DOWNLOAD_CONFIG;
		break;
	}

#if CFG_ENABLE_FW_DOWNLOAD_ACK
	fgCheckStatus = TRUE;
#else
	fgCheckStatus = FALSE;
#endif

	u4Status = wlanSendInitSetQueryCmd(prAdapter,
		ucCmdId, &rCmd, sizeof(rCmd),
		fgCheckStatus, FALSE,
		INIT_EVENT_ID_CMD_RESULT, &rEvent, sizeof(rEvent));

	if (fgCheckStatus) {
		if (u4Status != WLAN_STATUS_SUCCESS) {
			DBGLOG(INIT, ERROR,
				"wlanSendInitSetQueryCmd failed(0x%x).\n",
				u4Status);
			u4Status = WLAN_STATUS_FAILURE;
		} else if (rEvent.ucStatus != 0) {
			uint32_t u4Value = 0;

#ifdef MT6639
			HAL_MCR_RD(prAdapter,
				   WF_WFDMA_MCU_DMA0_PDA_DWLD_STATE_ADDR,
				   &u4Value);
#endif
			DBGLOG(INIT, ERROR,
				"Event status: %d, u4Value: 0x%x\n",
				rEvent.ucStatus,
				u4Value);
			u4Status = WLAN_STATUS_FAILURE;
		}
	}

	return u4Status;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called to download FW image.
 *
 * @param prAdapter      Pointer to the Adapter structure.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanImageSectionDownload(struct ADAPTER *prAdapter,
	uint8_t *pucImgBuf,
	uint32_t u4ImgSize)
{
	uint32_t u4Offset = 0;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;

	ASSERT(pucImgBuf);

	if (u4ImgSize == 0)
		return WLAN_STATUS_SUCCESS;

	do {
		uint8_t *pucSecBuf;
		uint32_t u4SecSize;

		if (u4Offset >= u4ImgSize)
			break;

		if (u4Offset + CMD_PKT_SIZE_FOR_IMAGE < u4ImgSize)
			u4SecSize = CMD_PKT_SIZE_FOR_IMAGE;
		else
			u4SecSize = u4ImgSize - u4Offset;

		pucSecBuf = (uint8_t *)pucImgBuf + u4Offset;

		u4Status = wlanSendInitSetQueryCmd(prAdapter,
			0, pucSecBuf, u4SecSize,
			FALSE, FALSE,
			0, NULL, 0);
		if (u4Status != WLAN_STATUS_SUCCESS) {
			DBGLOG(INIT, ERROR,
				"Firmware scatter download failed!\n");
			break;
		}
		u4Offset += u4SecSize;
	} while (TRUE);

	return u4Status;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called to confirm previously firmware
 *        download is done without error
 *
 * @param prAdapter      Pointer to the Adapter structure.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanImageQueryStatus(struct ADAPTER *prAdapter)
{
	struct INIT_EVENT_CMD_RESULT rEvent = {0};
	uint32_t u4Status = WLAN_STATUS_SUCCESS;

	u4Status = wlanSendInitSetQueryCmd(prAdapter,
		INIT_CMD_ID_QUERY_PENDING_ERROR, NULL, 0,
		TRUE, FALSE,
		INIT_EVENT_ID_PENDING_ERROR, &rEvent, sizeof(rEvent));
	if (u4Status != WLAN_STATUS_SUCCESS)
		goto exit;

	if (rEvent.ucStatus != 0) {
		DBGLOG(INIT, ERROR, "Event status: %d %s\n",
			rEvent.ucStatus,
			wlanInitEventStatusCodeToStr(rEvent.ucStatus));
		u4Status = WLAN_STATUS_FAILURE;
	}

exit:
	return u4Status;
}

uint32_t wlanConfigWifiFunc(struct ADAPTER *prAdapter,
			    u_int8_t fgEnable, uint32_t u4StartAddress,
			    uint8_t ucPDA)
{
	struct INIT_CMD_WIFI_START rCmd = {0};
	struct INIT_EVENT_CMD_RESULT rEvent = {0};
	uint32_t u4Status;

	rCmd.u4Override = 0;
	if (fgEnable)
		rCmd.u4Override |= START_OVERRIDE_START_ADDRESS;

	/* 5G cal until send efuse buffer mode CMD */
#if (CFG_EFUSE_BUFFER_MODE_DELAY_CAL == 1)
	if (prAdapter->fgIsSupportDelayCal == TRUE)
		rCmd.u4Override |= START_DELAY_CALIBRATION;
#endif

	if (ucPDA == PDA_CR4)
		rCmd.u4Override |= START_WORKING_PDA_OPTION;

	rCmd.u4Address = u4StartAddress;

	u4Status = wlanSendInitSetQueryCmd(prAdapter,
		INIT_CMD_ID_WIFI_START, &rCmd, sizeof(rCmd),
		TRUE, FALSE,
		INIT_EVENT_ID_CMD_RESULT, &rEvent, sizeof(rEvent));
	if (u4Status != WLAN_STATUS_SUCCESS)
		goto exit;

	if (rEvent.ucStatus != 0) {
		DBGLOG(INIT, ERROR, "Event status: %d\n", rEvent.ucStatus);
		u4Status = WLAN_STATUS_FAILURE;
	}

exit:
	if (u4Status != WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, INFO, "FW_START EVT failed\n");
		GL_DEFAULT_RESET_TRIGGER(prAdapter, RST_FW_DL_FAIL);
	} else
		DBGLOG(INIT, INFO, "FW_START EVT success!!\n");

	return u4Status;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used if FWDL by lk.
 *        Driver does not load and download firmware,
 *        only send the FW start command.
 *
 * @param prAdapter        Pointer to the Adapter structure.
 *        u4StartAddress   Align FWDL function only. Not used.
 *
 * @return u4Status        FW start CMD result
 */
/*----------------------------------------------------------------------------*/
#if CFG_WLAN_LK_FWDL_SUPPORT
uint32_t wlanFwImageSendStart(struct ADAPTER *prAdapter,
			uint32_t u4StartAddress)
{
	struct INIT_CMD_WIFI_START rCmd = {0};
	struct INIT_EVENT_CMD_RESULT rEvent = {0};
	u_int8_t fgWaitResp = TRUE;
	uint32_t u4Status;

	rCmd.u4Override = 0;

	/* 5G cal until send efuse buffer mode CMD */
#if (CFG_EFUSE_BUFFER_MODE_DELAY_CAL == 1)
	if (prAdapter->fgIsSupportDelayCal == TRUE)
		rCmd.u4Override |= START_DELAY_CALIBRATION;
#endif

	rCmd.u4Address = u4StartAddress;

	u4Status = wlanSendInitSetQueryCmd(prAdapter,
		INIT_CMD_ID_FW_IMAGE_START, &rCmd, sizeof(rCmd),
		fgWaitResp, FALSE,
		INIT_EVENT_ID_CMD_RESULT, &rEvent, sizeof(rEvent));
	if (u4Status != WLAN_STATUS_SUCCESS)
		goto exit;

	if (prAdapter->chip_info->checkbushang)
		prAdapter->chip_info->checkbushang((void *) prAdapter, FALSE);

	if (rEvent.ucStatus != 0) {
		DBGLOG(INIT, ERROR, "Event status: %d\n", rEvent.ucStatus);
		u4Status = WLAN_STATUS_FAILURE;
	}

exit:
	if (u4Status != WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, INFO, "FW_START EVT failed\n");
		GL_DEFAULT_RESET_TRIGGER(prAdapter, RST_FW_DL_FAIL);
	} else {
		DBGLOG(INIT, INFO, "FW_START EVT success!!\n");
	}

	return u4Status;
}
#endif

#if (CFG_DOWNLOAD_DYN_MEMORY_MAP == 1)
uint32_t wlanRamCodeDynMemMapSendComplete(struct ADAPTER *prAdapter,
			u_int8_t fgEnable, uint32_t u4StartAddress,
			uint8_t ucPDA)
{
	struct INIT_CMD_WIFI_START rCmd = {0};
	struct INIT_EVENT_CMD_RESULT rEvent = {0};
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;
	u_int8_t fgWaitResp = TRUE;
	uint32_t u4Status;

	rCmd.u4Override = 0;
	if (fgEnable)
		rCmd.u4Override |= START_OVERRIDE_START_ADDRESS;

	/* 5G cal until send efuse buffer mode CMD */
#if (CFG_EFUSE_BUFFER_MODE_DELAY_CAL == 1)
	if (prAdapter->fgIsSupportDelayCal == TRUE)
		rCmd.u4Override |= START_DELAY_CALIBRATION;
#endif

	if (ucPDA == PDA_CR4) {
		rCmd.u4Override |= START_WORKING_PDA_OPTION;
		if (prChipInfo->is_support_wacpu) {
			/* workaround for harrier powerOnCal too long issue
			* skip FW start event, fw ready bit check can cover
			* this.
			*/
			fgWaitResp = FALSE;
		}
	}

	rCmd.u4Address = u4StartAddress;

	u4Status = wlanSendInitSetQueryCmd(prAdapter,
		INIT_CMD_ID_DYN_MEM_MAP_FW_FINISH, &rCmd, sizeof(rCmd),
		fgWaitResp, FALSE,
		INIT_EVENT_ID_CMD_RESULT, &rEvent, sizeof(rEvent));
	if (u4Status != WLAN_STATUS_SUCCESS)
		goto exit;

	if (prAdapter->chip_info->checkbushang)
		prAdapter->chip_info->checkbushang((void *) prAdapter, FALSE);

	if (rEvent.ucStatus != 0) {
		DBGLOG(INIT, ERROR, "Event status: %d\n", rEvent.ucStatus);
		u4Status = WLAN_STATUS_FAILURE;
	}

exit:
	if (u4Status != WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, INFO, "FW_START EVT failed\n");
		GL_DEFAULT_RESET_TRIGGER(prAdapter, RST_FW_DL_FAIL);
	} else {
		DBGLOG(INIT, INFO, "FW_START EVT success!!\n");
	}

	return u4Status;
}
#endif

#if CFG_SUPPORT_COMPRESSION_FW_OPTION
uint32_t
wlanCompressedFWConfigWifiFunc(struct ADAPTER *prAdapter,
	u_int8_t fgEnable,
	uint32_t u4StartAddress, uint8_t ucPDA,
	struct INIT_CMD_WIFI_DECOMPRESSION_START *prFwImageInFo)
{
	struct INIT_CMD_WIFI_DECOMPRESSION_START rCmd = {0};
	struct INIT_EVENT_CMD_RESULT rEvent = {0};
	uint32_t u4Status;

	rCmd.u4Override = 0;
	if (fgEnable)
		rCmd.u4Override |= START_OVERRIDE_START_ADDRESS;

	/* 5G cal until send efuse buffer mode CMD */
#if (CFG_EFUSE_BUFFER_MODE_DELAY_CAL == 1)
	if (prAdapter->fgIsSupportDelayCal == TRUE)
		rCmd.u4Override |= START_DELAY_CALIBRATION;
#endif
	if (ucPDA == PDA_CR4)
		rCmd.u4Override |= START_WORKING_PDA_OPTION;

#if CFG_COMPRESSION_DEBUG
	rCmd.u4Override |= START_CRC_CHECK;
#endif
#if CFG_DECOMPRESSION_TMP_ADDRESS
	rCmd.u4Override |= CHANGE_DECOMPRESSION_TMP_ADDRESS;
	rCmd.u4DecompressTmpAddress = 0xE6000;
#endif
	rCmd.u4Address = u4StartAddress;
	rCmd.u4Region1Address = prFwImageInFo->u4Region1Address;
	rCmd.u4Region1CRC = prFwImageInFo->u4Region1CRC;
	rCmd.u4BlockSize = prFwImageInFo->u4BlockSize;
	rCmd.u4Region1length = prFwImageInFo->u4Region1length;
	rCmd.u4Region2Address = prFwImageInFo->u4Region2Address;
	rCmd.u4Region2CRC = prFwImageInFo->u4Region2CRC;
	rCmd.u4Region2length = prFwImageInFo->u4Region2length;

	u4Status = wlanSendInitSetQueryCmd(prAdapter,
		INIT_CMD_ID_DECOMPRESSED_WIFI_START, &rCmd, sizeof(rCmd),
		TRUE, FALSE,
		INIT_EVENT_ID_CMD_RESULT, &rEvent, sizeof(rEvent));
	if (u4Status != WLAN_STATUS_SUCCESS)
		goto exit;

	if (rEvent.ucStatus != 0) {
		DBGLOG(INIT, ERROR, "Event status: %d\n", rEvent.ucStatus);
		u4Status = WLAN_STATUS_FAILURE;
	}

exit:
	if (u4Status != WLAN_STATUS_SUCCESS)
		DBGLOG(INIT, INFO, "FW_START EVT failed\n");
	else
		DBGLOG(INIT, INFO, "FW_START EVT success!!\n");

	return u4Status;
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used to generate CRC32 checksum
 *
 * @param buf Pointer to the data.
 * @param len data length
 *
 * @return crc32 value
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanCRC32(uint8_t *buf, uint32_t len)
{
	uint32_t i, crc32 = 0xFFFFFFFF;
	const uint32_t crc32_ccitt_table[256] = {
		0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419,
		0x706af48f, 0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4,
		0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07,
		0x90bf1d91, 0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
		0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7, 0x136c9856,
		0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
		0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4,
		0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
		0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3,
		0x45df5c75, 0xdcd60dcf, 0xabd13d59, 0x26d930ac, 0x51de003a,
		0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599,
		0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
		0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190,
		0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f,
		0x9fbfe4a5, 0xe8b8d433, 0x7807c9a2, 0x0f00f934, 0x9609a88e,
		0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
		0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed,
		0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
		0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3,
		0xfbd44c65, 0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
		0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a,
		0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5,
		0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa, 0xbe0b1010,
		0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
		0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17,
		0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6,
		0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615,
		0x73dc1683, 0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
		0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1, 0xf00f9344,
		0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
		0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a,
		0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
		0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1,
		0xa6bc5767, 0x3fb506dd, 0x48b2364b, 0xd80d2bda, 0xaf0a1b4c,
		0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef,
		0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
		0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe,
		0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31,
		0x2cd99e8b, 0x5bdeae1d, 0x9b64c2b0, 0xec63f226, 0x756aa39c,
		0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
		0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b,
		0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
		0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1,
		0x18b74777, 0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
		0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45, 0xa00ae278,
		0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7,
		0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc, 0x40df0b66,
		0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
		0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605,
		0xcdd70693, 0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8,
		0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b,
		0x2d02ef8d
	};

	for (i = 0; i < len; i++)
		crc32 = crc32_ccitt_table[(crc32 ^ buf[i]) & 0xff] ^
			(crc32 >> 8);

	return ~crc32;
}

uint32_t wlanGetHarvardTailerInfo(struct ADAPTER
	*prAdapter, void *prFwBuffer, uint32_t u4FwSize,
	uint32_t ucTotSecNum, enum ENUM_IMG_DL_IDX_T eDlIdx)
{
	struct TAILER_FORMAT_T *prTailers;
	uint8_t *pucStartPtr;
	uint32_t u4SecIdx;

	pucStartPtr = (uint8_t *)((uintptr_t)prFwBuffer + u4FwSize -
		sizeof(struct TAILER_FORMAT_T) * ucTotSecNum);
	if (eDlIdx == IMG_DL_IDX_N9_FW) {
		kalMemCopy(&prAdapter->rVerInfo.rN9tailer, pucStartPtr,
			   sizeof(struct TAILER_FORMAT_T) * ucTotSecNum);
		prTailers = prAdapter->rVerInfo.rN9tailer;
	} else {
		kalMemCopy(&prAdapter->rVerInfo.rCR4tailer, pucStartPtr,
			   sizeof(struct TAILER_FORMAT_T) * ucTotSecNum);
		prTailers = prAdapter->rVerInfo.rCR4tailer;
	}

	for (u4SecIdx = 0; u4SecIdx < ucTotSecNum; u4SecIdx++) {
		/* Dump image information */
		DBGLOG(INIT, INFO,
		       "%s Section[%d]: chip_info[%u:E%u] feature[0x%02X]\n",
		       (eDlIdx == IMG_DL_IDX_N9_FW) ? "N9" : "CR4", u4SecIdx,
		       prTailers[u4SecIdx].chip_info,
		       prTailers[u4SecIdx].eco_code + 1,
		       prTailers[u4SecIdx].feature_set);


		DBGLOG(INIT, INFO, "date[%s] version[%s]\n",
		       prTailers[u4SecIdx].ram_built_date,
		       prTailers[u4SecIdx].ram_version);
	}

	return WLAN_STATUS_SUCCESS;
}

uint32_t wlanGetConnacTailerInfo(struct WIFI_VER_INFO *prVerInfo,
	void *prFwBuffer, uint32_t u4FwSize,
	enum ENUM_IMG_DL_IDX_T eDlIdx)
{
	struct TAILER_COMMON_FORMAT_T *prComTailer;
	struct TAILER_REGION_FORMAT_T *prRegTailer;
	uint8_t *pucImgPtr;
	uint8_t *pucTailertPtr;
	uint8_t *pucStartPtr;
	uint32_t u4SecIdx;

	pucImgPtr = prFwBuffer;
	pucStartPtr = (uint8_t *)((uintptr_t)prFwBuffer + u4FwSize -
		sizeof(struct TAILER_COMMON_FORMAT_T));
	prComTailer = (struct TAILER_COMMON_FORMAT_T *) pucStartPtr;
	kalMemCopy(&prVerInfo->rCommonTailer, prComTailer,
		   sizeof(struct TAILER_COMMON_FORMAT_T));

	/* Dump image information */
	DBGLOG(INIT, INFO,
		"%s: chip_info[%u:E%u] region_num[%d] date[%s] version[%s]\n",
			(eDlIdx == IMG_DL_IDX_N9_FW) ? "N9" : "CR4",
			prComTailer->ucChipInfo,
			prComTailer->ucEcoCode + 1,
			prComTailer->ucRegionNum,
			prComTailer->aucRamBuiltDate,
			prComTailer->aucRamVersion);

	if (prComTailer->ucRegionNum > MAX_FWDL_SECTION_NUM) {
		DBGLOG(INIT, INFO,
		       "Regions number[%d] > max section number[%d]\n",
		       prComTailer->ucRegionNum, MAX_FWDL_SECTION_NUM);
		return WLAN_STATUS_FAILURE;
	}

	pucStartPtr -= (prComTailer->ucRegionNum *
			sizeof(struct TAILER_REGION_FORMAT_T));
	pucTailertPtr = pucStartPtr;
	for (u4SecIdx = 0; u4SecIdx < prComTailer->ucRegionNum; u4SecIdx++) {
		prRegTailer = (struct TAILER_REGION_FORMAT_T *) pucStartPtr;
		kalMemCopy(&prVerInfo->rRegionTailers[u4SecIdx],
			   prRegTailer, sizeof(struct TAILER_REGION_FORMAT_T));

		/* Dump image information */
		DBGLOG(INIT, TRACE,
		       "Region[%d]: addr[0x%08X] feature[0x%02X] size[%u]\n",
		       u4SecIdx, prRegTailer->u4Addr,
		       prRegTailer->ucFeatureSet, prRegTailer->u4Len);
		DBGLOG(INIT, TRACE,
		       "uncompress_crc[0x%08X] uncompress_size[0x%08X] block_size[0x%08X]\n",
		       prRegTailer->u4CRC, prRegTailer->u4RealSize,
		       prRegTailer->u4BlockSize);
		pucImgPtr += prRegTailer->u4Len;
		pucStartPtr += sizeof(struct TAILER_REGION_FORMAT_T);
	}

	if (prComTailer->ucFormatFlag && pucImgPtr < pucTailertPtr)
		fwDlGetReleaseInfoSection(prVerInfo, pucImgPtr);

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used to extract the wifi ram code start address
 *
 * @param prVerInfo      Pointer to the P_WIFI_VER_INFO_T structure.
 *
 * @return addr          The ram code entry address.
 *                       0: use firmware defaut setting
 *                       others: use it as the start address
 */
/*----------------------------------------------------------------------------*/

uint32_t wlanDetectRamEntry(struct WIFI_VER_INFO
			    *prVerInfo)
{
	uint32_t addr = 0;
	uint32_t u4SecIdx;
	struct TAILER_COMMON_FORMAT_T *prComTailer =
			&prVerInfo->rCommonTailer;
	struct TAILER_REGION_FORMAT_T *prRegTailer;

	for (u4SecIdx = 0; u4SecIdx < prComTailer->ucRegionNum;
	     u4SecIdx++) {
		prRegTailer = &(prVerInfo->rRegionTailers[u4SecIdx]);

		if (prRegTailer->ucFeatureSet &
		    DOWNLOAD_CONFIG_VALID_RAM_ENTRY) {
			addr = prRegTailer->u4Addr;
			break;
		}
	}

	return addr;
}

uint32_t wlanHarvardFormatDownload(struct ADAPTER
				   *prAdapter, enum ENUM_IMG_DL_IDX_T eDlIdx)
{
	uint32_t u4FwSize = 0;
	void *prFwBuffer = NULL;
	uint32_t rDlStatus = 0;
	uint32_t rCfgStatus = 0;
	uint32_t ucTotSecNum;
	uint8_t ucPDA;
#if CFG_SUPPORT_COMPRESSION_FW_OPTION
	u_int8_t fgIsCompressed = FALSE;
	struct INIT_CMD_WIFI_DECOMPRESSION_START rFwImageInFo;
#else
	u_int8_t fgIsDynamicMemMap = FALSE;
#endif

	if (eDlIdx == IMG_DL_IDX_N9_FW) {
		ucTotSecNum = N9_FWDL_SECTION_NUM;
		ucPDA = PDA_N9;
	} else {
		ucTotSecNum = CR4_FWDL_SECTION_NUM;
		ucPDA = PDA_CR4;
	}

	kalFirmwareImageMapping(prAdapter->prGlueInfo, &prFwBuffer,
				&u4FwSize, eDlIdx);
	if (prFwBuffer == NULL) {
		DBGLOG(INIT, WARN, "FW[%u] load error!\n", eDlIdx);
		return WLAN_STATUS_FAILURE;
	}

	wlanGetHarvardTailerInfo(prAdapter, prFwBuffer, u4FwSize,
				 ucTotSecNum, eDlIdx);
#if CFG_SUPPORT_COMPRESSION_FW_OPTION
	rDlStatus = wlanCompressedImageSectionDownloadStage(
			    prAdapter, prFwBuffer, u4FwSize, ucTotSecNum,
			    eDlIdx, &fgIsCompressed, &rFwImageInFo);
	if (eDlIdx == IMG_DL_IDX_CR4_FW)
		prAdapter->fgIsCr4FwDownloaded = TRUE;
	if (fgIsCompressed == TRUE)
		rCfgStatus = wlanCompressedFWConfigWifiFunc(prAdapter,
				FALSE, 0, ucPDA, &rFwImageInFo);
	else
		rCfgStatus = wlanConfigWifiFunc(prAdapter, FALSE, 0, ucPDA);
#else
	rDlStatus = wlanImageSectionDownloadStage(prAdapter,
			prFwBuffer, u4FwSize, ucTotSecNum, eDlIdx,
			&fgIsDynamicMemMap);
	if (eDlIdx == IMG_DL_IDX_CR4_FW)
		prAdapter->fgIsCr4FwDownloaded = TRUE;
	rCfgStatus = wlanConfigWifiFunc(prAdapter, FALSE, 0, ucPDA);
#endif
	kalFirmwareImageUnmapping(prAdapter->prGlueInfo, NULL,
				  prFwBuffer);

	if ((rDlStatus != WLAN_STATUS_SUCCESS)
	    || (rCfgStatus != WLAN_STATUS_SUCCESS))
		return WLAN_STATUS_FAILURE;

	return WLAN_STATUS_SUCCESS;
}

#if CFG_WLAN_LK_FWDL_SUPPORT
uint32_t wlanFwImageDownload(struct ADAPTER
				  *prAdapter, enum ENUM_IMG_DL_IDX_T eDlIdx)
{
	uint32_t rCfgStatus = WLAN_STATUS_SUCCESS;
	uint8_t *pucManifestBuffer = NULL;
	uint32_t u4ManifestSize = 0;

	if (prAdapter->chip_info->checkbushang) {
		if (prAdapter->chip_info->checkbushang((void *) prAdapter,
				TRUE) != 0) {
			DBGLOG(INIT, WARN, "Check bus hang failed.\n");
			rCfgStatus = WLAN_STATUS_FAILURE;
			goto exit;
		}
	}

	pucManifestBuffer = (uint8_t *)kalMemAlloc(
		FW_VERSION_MAX_LEN, VIR_MEM_TYPE);
	if (!pucManifestBuffer) {
		DBGLOG(INIT, ERROR, "vmalloc(%u) failed\n", FW_VERSION_MAX_LEN);
	} else {
		wlanReadRamCodeReleaseManifest(pucManifestBuffer,
			&u4ManifestSize, FW_VERSION_MAX_LEN);

		kalMemZero(&prAdapter->rVerInfo.aucReleaseManifest,
			sizeof(prAdapter->rVerInfo.aucReleaseManifest));
		kalMemCopy(&prAdapter->rVerInfo.aucReleaseManifest,
			pucManifestBuffer, u4ManifestSize);
		DBGLOG(INIT, INFO, "aucReleaseManifest fw_ver=%s\n",
			&prAdapter->rVerInfo.aucReleaseManifest);

		kalMemFree(pucManifestBuffer, VIR_MEM_TYPE, FW_VERSION_MAX_LEN);
	}
	rCfgStatus = wlanFwImageSendStart(prAdapter, 0);

exit:
	if (rCfgStatus != WLAN_STATUS_SUCCESS)
		return WLAN_STATUS_FAILURE;

	return WLAN_STATUS_SUCCESS;
}
#else
uint32_t wlanConnacFormatDownload(struct ADAPTER
				  *prAdapter, enum ENUM_IMG_DL_IDX_T eDlIdx)
{
	void *prFwBuffer = NULL;
	uint32_t u4FwSize = 0;
	uint32_t ram_entry = 0;
	uint32_t rDlStatus = 0;
	uint32_t rCfgStatus = 0;
	uint8_t ucRegionNum;
	uint8_t ucPDA;
	u_int8_t fgIsDynamicMemMap = FALSE;

	kalFirmwareImageMapping(prAdapter->prGlueInfo, &prFwBuffer,
				&u4FwSize, eDlIdx);
	if (prFwBuffer == NULL) {
		DBGLOG(INIT, WARN, "FW[%u] load error!\n", eDlIdx);
		return WLAN_STATUS_FAILURE;
	}

	if (wlanGetConnacTailerInfo(&prAdapter->rVerInfo,
					prFwBuffer, u4FwSize,
					eDlIdx) != WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, WARN, "Get tailer info error!\n");
		rDlStatus = WLAN_STATUS_FAILURE;
		goto exit;
	}

#if (CFG_SUPPORT_CONNAC2X == 1)
	if (prAdapter->chip_info->checkbushang) {
		if (prAdapter->chip_info->checkbushang((void *) prAdapter,
				TRUE) != 0) {
			DBGLOG(INIT, WARN, "Check bus hang failed.\n");
			rDlStatus = WLAN_STATUS_FAILURE;
			goto exit;
		}
	}
#endif

	ucRegionNum = prAdapter->rVerInfo.rCommonTailer.ucRegionNum;
	ucPDA = (eDlIdx == IMG_DL_IDX_N9_FW) ? PDA_N9 : PDA_CR4;

	rDlStatus = wlanImageSectionDownloadStage(prAdapter,
			prFwBuffer, u4FwSize, ucRegionNum, eDlIdx,
			&fgIsDynamicMemMap);

	if (rDlStatus != WLAN_STATUS_SUCCESS)
		goto exit;

	ram_entry = wlanDetectRamEntry(&prAdapter->rVerInfo);

/* To support dynamic memory map for WiFi RAM code download::Begin */
#if (CFG_DOWNLOAD_DYN_MEMORY_MAP == 1)
	if (fgIsDynamicMemMap)
		rCfgStatus = wlanRamCodeDynMemMapSendComplete(prAdapter,
					(ram_entry == 0) ? FALSE : TRUE,
					ram_entry, ucPDA);
	else
		rCfgStatus = wlanConfigWifiFunc(prAdapter,
					(ram_entry == 0) ? FALSE : TRUE,
					ram_entry, ucPDA);
#else
		rCfgStatus = wlanConfigWifiFunc(prAdapter,
					(ram_entry == 0) ? FALSE : TRUE,
					ram_entry, ucPDA);
#if defined(BELLWETHER) || defined(MT7990)
		rCfgStatus = wlanConfigWifiFunc(prAdapter,
					FALSE,
					0,
					PDA_CR4);
#endif
#endif
/* To support dynamic memory map for WiFi RAM code download::End */

exit:
	kalFirmwareImageUnmapping(prAdapter->prGlueInfo, NULL,
				  prFwBuffer);

	if ((rDlStatus != WLAN_STATUS_SUCCESS)
	    || (rCfgStatus != WLAN_STATUS_SUCCESS))
		return WLAN_STATUS_FAILURE;

	return WLAN_STATUS_SUCCESS;
}
#endif

uint32_t wlanDownloadFW(struct ADAPTER *prAdapter)
{
	uint32_t rStatus = 0;
	struct mt66xx_chip_info *prChipInfo;
	struct FWDL_OPS_T *prFwDlOps;

	if (!prAdapter)
		return WLAN_STATUS_FAILURE;

	prChipInfo = prAdapter->chip_info;
	prFwDlOps = prChipInfo->fw_dl_ops;

	HAL_ENABLE_FWDL(prAdapter, TRUE);

	if (prFwDlOps->downloadPatch) {
		rStatus = prFwDlOps->downloadPatch(prAdapter);
		if (rStatus != WLAN_STATUS_SUCCESS) {
			DBGLOG(INIT, ERROR, "Patch Download fail\n");
			conn_dbg_add_log(CONN_DBG_LOG_TYPE_HW_ERR,
				"Patch Download fail\n");
			goto exit;
		}
	}

#if CFG_SUPPORT_WIFI_DL_ZB_PATCH
	if (prFwDlOps->downloadZbPatch && prAdapter->fgIsNeedDlPatch == TRUE) {
		if (prFwDlOps->downloadZbPatch(prAdapter)
			!= WLAN_STATUS_SUCCESS)
			DBGLOG(INIT, ERROR, "ZB Patch Download fail\n");
	}
#endif

#if CFG_SUPPORT_WIFI_DL_BT_PATCH
	/*  Add a condition for check if need to help bt dl patch,
	 *   because if it don't need to dl patch, it only have two case.
	 *   1. Ins BT driver -> Ins WIFI driver(BT help wifi dl patch).
	 *   2. Re-insert driver without power off(patch still exist,
	 *   so not need to dl again).
	 *   Add this is prevent if wifi use ccif to query BT status,
	 *   but BT have some error, so wifi not get any response
	 *   and timeout, but wifi didn't have any error,
	 *   so it shouldn't probe fail with this case.
	 */
	if (prFwDlOps->downloadBtPatch && prAdapter->fgIsNeedDlPatch == TRUE) {
		if (prFwDlOps->downloadBtPatch(prAdapter)
			!= WLAN_STATUS_SUCCESS)
			DBGLOG(INIT, ERROR, "BT Patch Download fail\n");
	}
#endif

	rStatus = kalSyncTimeToFW(prAdapter, TRUE);

	if (prChipInfo->queryPmicInfo)
		prChipInfo->queryPmicInfo(prAdapter);

	if (prFwDlOps->phyAction) {
		rStatus = prFwDlOps->phyAction(prAdapter);
		if (rStatus != WLAN_STATUS_SUCCESS) {
			DBGLOG(INIT, ERROR, "phyAction fail\n");
			conn_dbg_add_log(CONN_DBG_LOG_TYPE_HW_ERR,
				"phyAction fail\n");
			goto exit;
		}
	}

	DBGLOG(INIT, INFO, "FW download Start\n");

	if (prFwDlOps->downloadFirmware) {
		rStatus = prFwDlOps->downloadFirmware(prAdapter,
						      IMG_DL_IDX_N9_FW);
		if ((prChipInfo->is_support_cr4 || prChipInfo->is_support_wacpu)
		    && rStatus == WLAN_STATUS_SUCCESS)
			rStatus = prFwDlOps->downloadFirmware(prAdapter,
						IMG_DL_IDX_CR4_FW);
	}

exit:
	DBGLOG(INIT, TRACE, "FW download End\n");

	HAL_ENABLE_FWDL(prAdapter, FALSE);

	return rStatus;
}

uint32_t wlanDownloadPatch(struct ADAPTER *prAdapter)
{
	uint32_t u4FwSize = 0;
	void *prFwBuffer = NULL;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
#if CFG_SUPPORT_COMPRESSION_FW_OPTION
	uint8_t ucIsCompressed;
#else
	u_int8_t fgIsDynamicMemMap = FALSE;
#endif

	if (!prAdapter)
		return WLAN_STATUS_FAILURE;


	DBGLOG(INIT, TRACE, "Patch download start\n");

	prAdapter->rVerInfo.fgPatchIsDlByDrv = FALSE;

	kalFirmwareImageMapping(prAdapter->prGlueInfo, &prFwBuffer,
				&u4FwSize, IMG_DL_IDX_PATCH);
	if (prFwBuffer == NULL) {
		DBGLOG(INIT, WARN, "FW[%u] load error!\n",
		       IMG_DL_IDX_PATCH);
		return WLAN_STATUS_FAILURE;
	}

#if (CFG_ROM_PATCH_NO_SEM_CTRL == 0)
	if (wlanPatchIsDownloaded(prAdapter)) {
		DBGLOG(INIT, INFO, "No need to download patch\n");
		goto exit;
	}
#endif

	/* Patch DL */
	do {
#if CFG_SUPPORT_COMPRESSION_FW_OPTION
		u4Status = wlanCompressedImageSectionDownloadStage(
			prAdapter, prFwBuffer, u4FwSize, 1,
			IMG_DL_IDX_PATCH, &ucIsCompressed, NULL);
#else
		u4Status = wlanImageSectionDownloadStage(
			prAdapter, prFwBuffer, u4FwSize, 1, IMG_DL_IDX_PATCH,
			&fgIsDynamicMemMap);
#endif

		if (u4Status != WLAN_STATUS_SUCCESS)
			goto exit;

/* Dynamic memory map::Begin */
#if (CFG_DOWNLOAD_DYN_MEMORY_MAP == 1)
		u4Status = wlanPatchDynMemMapSendComplete(prAdapter);
#else
#if CFG_SUPPORT_WIFI_DL_BT_PATCH || CFG_SUPPORT_WIFI_DL_ZB_PATCH
		/*
		 * PATCH_FNSH_TYPE_WF_MD:
		 * Download flow expects that the BT patch download is coming.
		 * The cal won't start after WF patch download finish.
		 */
		u4Status = wlanPatchSendComplete(prAdapter,
			PATCH_FNSH_TYPE_WF_MD);
#else
		u4Status = wlanPatchSendComplete(prAdapter);
#endif
#endif
/* Dynamic memory map::End */

		prAdapter->rVerInfo.fgPatchIsDlByDrv = TRUE;
	} while (0);

exit:
	DBGLOG(INIT, TRACE, "Patch download end[%d].\n", u4Status);

	kalFirmwareImageUnmapping(prAdapter->prGlueInfo, NULL,
				  prFwBuffer);

	return u4Status;
}

uint32_t wlanGetPatchInfo(struct ADAPTER *prAdapter)
{
	uint32_t u4FwSize = 0;
	void *prFwBuffer = NULL;
	uint32_t u4StartOffset, u4Addr, u4Len, u4DataMode;

	if (!prAdapter)
		return WLAN_STATUS_FAILURE;

	kalFirmwareImageMapping(prAdapter->prGlueInfo, &prFwBuffer,
				&u4FwSize, IMG_DL_IDX_PATCH);
	if (prFwBuffer == NULL) {
		DBGLOG(INIT, WARN, "FW[%u] load error!\n",
		       IMG_DL_IDX_PATCH);
		return WLAN_STATUS_FAILURE;
	}

	wlanImageSectionGetPatchInfo(prAdapter, prFwBuffer,
				     u4FwSize, &u4StartOffset,
				     &u4Addr, &u4Len, &u4DataMode);

	kalFirmwareImageUnmapping(prAdapter->prGlueInfo, NULL,
				  prFwBuffer);

	return WLAN_STATUS_SUCCESS;
}

uint32_t fwDlGetFwdlInfo(struct ADAPTER *prAdapter,
			 char *pcBuf, int i4TotalLen)
{
	struct WIFI_VER_INFO *prVerInfo = &prAdapter->rVerInfo;
	struct FWDL_OPS_T *prFwDlOps;
	uint32_t u4Offset = 0;
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;
#if !CFG_WLAN_LK_FWDL_SUPPORT
	uint8_t aucBuf[32] = {0}, aucDate[32] = {0};
#endif

	prFwDlOps = prAdapter->chip_info->fw_dl_ops;

#if !CFG_WLAN_LK_FWDL_SUPPORT
	kalSnprintf(aucBuf, sizeof(aucBuf), "%4s", prVerInfo->aucFwBranchInfo);
	kalSnprintf(aucDate, sizeof(aucDate), "%16s", prVerInfo->aucFwDateCode);

	u4Offset += snprintf(pcBuf + u4Offset,
			i4TotalLen - u4Offset,
			"\nN9 FW version %s-%u.%u.%u[DEC] (%s)\n",
			aucBuf,
			(uint32_t)(prVerInfo->u2FwOwnVersion >> 8),
			(uint32_t)(prVerInfo->u2FwOwnVersion & BITS(0, 7)),
			prVerInfo->ucFwBuildNumber, aucDate);
#endif
	if (prFwDlOps->getFwDlInfo)
		u4Offset += prFwDlOps->getFwDlInfo(prAdapter,
						   pcBuf + u4Offset,
						   i4TotalLen - u4Offset);

	if (prChipInfo->patch_addr && !prVerInfo->fgPatchIsDlByDrv) {
		u4Offset += snprintf(pcBuf + u4Offset,
				     i4TotalLen - u4Offset,
				     "MCU patch is not downloaded by wlan driver, read patch info\n");
#if (CFG_MTK_ANDROID_WMT == 0)
		wlanGetPatchInfo(prAdapter);
#endif
	}

#if !CFG_WLAN_LK_FWDL_SUPPORT
	kalSnprintf(aucBuf, sizeof(aucBuf), "%4s",
			prVerInfo->rPatchHeader.aucPlatform);
	kalSnprintf(aucDate, sizeof(aucDate), "%16s",
			prVerInfo->rPatchHeader.aucBuildDate);

	u4Offset += snprintf(pcBuf + u4Offset,
			     i4TotalLen - u4Offset,
			     "Patch platform %s version 0x%04X %s\n",
			     aucBuf, prVerInfo->rPatchHeader.u4PatchVersion,
			     aucDate);

	u4Offset += snprintf(pcBuf + u4Offset, i4TotalLen - u4Offset,
			"Drv version %u.%u[DEC]\n",
			(uint32_t)(prVerInfo->u2FwPeerVersion >> 8),
			(uint32_t)(prVerInfo->u2FwPeerVersion & BITS(0, 7)));
#endif
	return u4Offset;
}

void fwDlGetReleaseInfoSection(struct WIFI_VER_INFO *prVerInfo,
	uint8_t *pucStartPtr)
{
	struct HEADER_RELEASE_INFO *prFirstInfo;
	struct HEADER_RELEASE_INFO *prRelInfo;
	uint8_t *pucCurPtr = pucStartPtr + RELEASE_INFO_SEPARATOR_LEN;
	uint16_t u2Len = 0, u2Offset = 0;
	uint8_t ucManifestExist = 0;

	prFirstInfo = (struct HEADER_RELEASE_INFO *)pucCurPtr;
	DBGLOG(INIT, TRACE, "Release info tag[%u] len[%u]\n",
	       prFirstInfo->ucTag, prFirstInfo->u2Len);

	pucCurPtr += sizeof(struct HEADER_RELEASE_INFO);
	while (u2Offset < prFirstInfo->u2Len) {
		prRelInfo = (struct HEADER_RELEASE_INFO *)pucCurPtr;
		DBGLOG(INIT, TRACE,
		       "Release info tag[%u] len[%u] padding[%u]\n",
		       prRelInfo->ucTag, prRelInfo->u2Len,
		       prRelInfo->ucPaddingLen);

		pucCurPtr += sizeof(struct HEADER_RELEASE_INFO);
		switch (prRelInfo->ucTag) {
		case 0x01:
			fwDlGetReleaseManifest(prVerInfo, prRelInfo, pucCurPtr);
			ucManifestExist = 1;
			break;
		case 0x02:
			if (!ucManifestExist)
				fwDlGetReleaseManifest(prVerInfo,
					prRelInfo, pucCurPtr);
			break;
		default:
			DBGLOG(INIT, WARN, "Not support release info tag[%u]\n",
			       prRelInfo->ucTag);
		}

		u2Len = prRelInfo->u2Len + prRelInfo->ucPaddingLen;
		pucCurPtr += u2Len;
		u2Offset += u2Len + sizeof(struct HEADER_RELEASE_INFO);
	}
}

void fwDlGetReleaseManifest(struct WIFI_VER_INFO *prVerInfo,
			    struct HEADER_RELEASE_INFO *prRelInfo,
			    uint8_t *pucStartPtr)
{
	kalMemZero(&prVerInfo->aucReleaseManifest,
		   sizeof(prVerInfo->aucReleaseManifest));
	kalMemCopy(&prVerInfo->aucReleaseManifest,
		   pucStartPtr, prRelInfo->u2Len);
	DBGLOG(INIT, INFO, "Release manifest: %s\n",
	       prVerInfo->aucReleaseManifest);
}


uint32_t wlanReadRamCodeReleaseManifest(uint8_t *pucManifestBuffer,
		uint32_t *pu4ManifestSize, uint32_t u4BufferMaxSize)
{
	struct mt66xx_chip_info *prChipInfo = NULL;
	uint32_t u4FwVerOffset = 0;
	uint32_t u4CopySize = 0;

	*pu4ManifestSize = 0;
	kalMemZero(pucManifestBuffer, u4BufferMaxSize);

	u4FwVerOffset = kalGetFwVerOffset();
	glGetChipInfo((void **)&prChipInfo);

	if (u4FwVerOffset) {
		u4CopySize = (u4BufferMaxSize < FW_VERSION_MAX_LEN) ?
			u4BufferMaxSize : FW_VERSION_MAX_LEN;
		emi_mem_read(prChipInfo, u4FwVerOffset,
			pucManifestBuffer, u4CopySize);
		*pu4ManifestSize = kalStrnLen(pucManifestBuffer,
			u4BufferMaxSize);
		DBGLOG(INIT, INFO, "ver[%d]:%s\n", *pu4ManifestSize,
			pucManifestBuffer);
	}
	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called to get RAM CODE release manifest when
 *        wifi is not on.
 *
 * @param ppucManifestBuffer Pointer to store Manifest string.
 *        pu4ManifestSize    Pointer of Manifest string length,
 *                           size is zero if manifest not copy into buffer.
 *        u4BufferMaxSize    The max length of Manifest Buffer.
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanParseRamCodeReleaseManifest(uint8_t *pucManifestBuffer,
		uint32_t *pu4ManifestSize, uint32_t u4BufferMaxSize)
{
#define FW_FILE_NAME_TOTAL 8
#define FW_FILE_NAME_MAX_LEN 64
	struct WIFI_VER_INFO *prVerInfo = NULL;
	struct mt66xx_chip_info *prChipInfo = NULL;
	void *pvDev = NULL;
	void *pvMapFileBuf = NULL;
	uint32_t u4FileLength = 0;
	uint32_t u4ReadLen = 0;
	uint8_t *prFwBuffer = NULL;
	uint8_t *aucFwName[FW_FILE_NAME_TOTAL + 1];
	uint8_t aucFwNameBody[FW_FILE_NAME_TOTAL][FW_FILE_NAME_MAX_LEN];
	uint8_t idx;
	uint8_t fgResult = FALSE;
	uint32_t u4Ret;

	kalMemZero(aucFwName, sizeof(aucFwName));
	kalMemZero(pucManifestBuffer, u4BufferMaxSize);
	*pu4ManifestSize = 0;

	glGetChipInfo((void **)&prChipInfo);
	if (prChipInfo == NULL) {
		DBGLOG(INIT, WARN, "glGetChipInfo failed\n");
		goto exit;
	}

	kalGetPlatDev(&pvDev);
	if (pvDev == NULL) {
		DBGLOG(INIT, WARN, "glGetPlatDev failed\n");
		goto exit;
	}

	for (idx = 0; idx < FW_FILE_NAME_TOTAL; idx++)
		aucFwName[idx] = (uint8_t *)(aucFwNameBody + idx);

	idx = 0;
	if (prChipInfo->fw_dl_ops->constructFirmwarePrio) {
		prChipInfo->fw_dl_ops->constructFirmwarePrio(
			NULL, NULL, aucFwName, &idx, FW_FILE_NAME_TOTAL);
	} else {
		DBGLOG(INIT, WARN, "Construct FW binary failed\n");
		goto exit;
	}

	for (idx = 0; aucFwName[idx]; idx++) {
		u4Ret = kalRequestFirmware(aucFwName[idx], &prFwBuffer,
				&u4ReadLen, FALSE, pvDev);

		if (u4Ret) {
			DBGLOG(INIT, TRACE,
			       "Request FW image: %s failed, errno[%d]\n",
			       aucFwName[idx], u4Ret);
			continue;
		} else {
			DBGLOG(INIT, INFO, "Request FW image: %s done\n",
			       aucFwName[idx]);
			fgResult = TRUE;
			break;
		}
	}

	if (!fgResult)
		goto exit;

#if CFG_SUPPORT_SINGLE_FW_BINARY
	if (prChipInfo->fw_dl_ops->parseSingleBinaryFile &&
		prChipInfo->fw_dl_ops->parseSingleBinaryFile(
			prFwBuffer,
			u4ReadLen,
			&pvMapFileBuf,
			&u4FileLength,
			0) == WLAN_STATUS_SUCCESS) {
		kalMemFree(prFwBuffer, VIR_MEM_TYPE, u4ReadLen);
	} else {
		pvMapFileBuf = prFwBuffer;
		u4FileLength = u4ReadLen;
	}
#else
	pvMapFileBuf = prFwBuffer;
	u4FileLength = u4ReadLen;
#endif

	prVerInfo = (struct WIFI_VER_INFO *)
		kalMemAlloc(sizeof(struct WIFI_VER_INFO), VIR_MEM_TYPE);
	if (!prVerInfo) {
		DBGLOG(INIT, WARN, "vmalloc(%u) failed\n",
			(uint32_t) sizeof(struct WIFI_VER_INFO));
		goto free_buf;
	}

	if (wlanGetConnacTailerInfo(prVerInfo, pvMapFileBuf, u4FileLength,
			IMG_DL_IDX_N9_FW) != WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, WARN, "Get tailer info error!\n");
		goto free_buf;
	}

	*pu4ManifestSize =
		kalStrnLen(prVerInfo->aucReleaseManifest, u4BufferMaxSize);

	kalMemCopy(pucManifestBuffer,
		&prVerInfo->aucReleaseManifest,
		*pu4ManifestSize);

free_buf:
	if (prVerInfo)
		kalMemFree(prVerInfo, VIR_MEM_TYPE,
			sizeof(struct WIFI_VER_INFO));
	if (pvMapFileBuf)
		kalMemFree(pvMapFileBuf, VIR_MEM_TYPE, u4FileLength);
exit:
	return WLAN_STATUS_SUCCESS;
}

#if IS_ENABLED(CFG_MTK_WIFI_SUPPORT_UDS_FWDL)
static void fwDlSetupRedlDmad(struct ADAPTER *prAdapter,
	phys_addr_t pa,
	uint32_t u4Size)
{
	struct GL_HIF_INFO *prHifInfo = &prAdapter->prGlueInfo->rHifInfo;
	struct RTMP_TX_RING *prTxRing = &prHifInfo->TxRing[TX_RING_FWDL];
	struct RTMP_DMACB *pTxCell = &prTxRing->Cell[prTxRing->TxCpuIdx];
	struct TXD_STRUCT *pTxD = (struct TXD_STRUCT *)pTxCell->AllocVa;

	pTxD->LastSec0 = 1;
	pTxD->LastSec1 = 0;
	pTxD->SDLen0 = u4Size;
	pTxD->SDLen1 = 0;
	pTxD->SDPtr0 = (uint64_t)pa & DMA_LOWER_32BITS_MASK;
#ifdef CONFIG_PHYS_ADDR_T_64BIT
	pTxD->SDPtr0Ext = ((uint64_t)pa >> DMA_BITS_OFFSET) &
		DMA_HIGHER_4BITS_MASK;
#else
	pTxD->SDPtr0Ext = 0;
#endif
	pTxD->SDPtr1 = 0;
	pTxD->Burst = 0;
	pTxD->DMADONE = 0;

	INC_RING_INDEX(prTxRing->TxCpuIdx, prTxRing->u4RingSize);

	prTxRing->u4UsedCnt++;
}

static void fwDlSetupRedlImg(struct ADAPTER *prAdapter,
	phys_addr_t rEmiPhyAddr,
	uint32_t u4EmiOffset,
	uint32_t u4Size)
{
	uint32_t u4Offset = 0;

	do {
		uint32_t u4SecSize = 0;

		if (u4Offset >= u4Size)
			break;

		if (u4Offset + FWDL_REDL_MAX_PKT_SIZE < u4Size)
			u4SecSize = FWDL_REDL_MAX_PKT_SIZE;
		else
			u4SecSize = u4Size - u4Offset;

		fwDlSetupRedlDmad(prAdapter,
			rEmiPhyAddr + u4EmiOffset + u4Offset,
			u4SecSize);

		u4Offset += u4SecSize;
	} while (TRUE);
}

uint32_t fwDlSetupReDl(struct ADAPTER *prAdapter,
	uint32_t u4EmiOffset, uint32_t u4Size)
{
	phys_addr_t rEmiPhyAddr;
	uint32_t u4EmiLength;

	DBGLOG(INIT, INFO, "u4EmiOffset: 0x%x, u4Size: 0x%x\n",
		u4EmiOffset, u4Size);

	rEmiPhyAddr = emi_mem_get_phy_base(prAdapter->chip_info);
	u4EmiLength = emi_mem_get_size(prAdapter->chip_info);
	u4EmiOffset = emi_mem_offset_convert(u4EmiOffset);

	if (u4Size == 0 || rEmiPhyAddr == 0 ||
	    (u4Size + u4EmiOffset) > u4EmiLength)
		return WLAN_STATUS_INVALID_DATA;

	/* 1. Recycle used dmad first */
	halWpdmaProcessCmdDmaDone(prAdapter->prGlueInfo,
		TX_RING_FWDL);

	/* 2. Stop recycle TX dmad for FWDL ring */
	halWpdmaStopRecycleDmad(prAdapter->prGlueInfo,
		TX_RING_FWDL);

	/* 3. Setup DMAD for redl packets */
	fwDlSetupRedlImg(prAdapter,
		rEmiPhyAddr,
		u4EmiOffset,
		u4Size);

	return WLAN_STATUS_SUCCESS;
}
#endif

#if (CFG_SUPPORT_CONNAC3X == 1)
#if CFG_SUPPORT_WIFI_DL_BT_PATCH
void asicConnac3xConstructBtPatchName(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucName, uint8_t *pucNameIdx)
{
	struct ADAPTER *prAdapter = NULL;
	struct mt66xx_chip_info *prChipInfo = NULL;
	uint32_t u4FlavorVer;

	if (prGlueInfo == NULL) {
		DBGLOG(INIT, ERROR, "prGlueInfo is NULL.\n");
		return;
	}

	prAdapter = prGlueInfo->prAdapter;
	if (prAdapter == NULL) {
		DBGLOG(INIT, ERROR, "prAdapter is NULL.\n");
		return;
	}

	prChipInfo = prAdapter->chip_info;
	if (prChipInfo == NULL) {
		DBGLOG(INIT, ERROR, "prChipInfo is NULL.\n");
		return;
	}

	if (IS_MOBILE_SEGMENT)
		u4FlavorVer = 0x1;
	else
		u4FlavorVer = 0x2;

	kalSnprintf(apucName[(*pucNameIdx)],
		CFG_FW_NAME_MAX_LEN, "BT_RAM_CODE_MT%x_%x_%x_hdr.bin",
		prChipInfo->chip_id, u4FlavorVer,
		asicConnac3xGetFwVer(prAdapter));
}

uint32_t wlanBtPatchSendSemaControl(struct ADAPTER *prAdapter,
				    uint8_t *pucPatchStatus)
{
	struct INIT_CMD_BT_PATCH_SEMA_CTRL rCmd = {0};
	struct INIT_EVENT_BT_PATCH_SEMA_CTRL_T rEvent = {0};
	uint32_t u4Status = WLAN_STATUS_SUCCESS;

	rCmd.ucGetSemaphore = PATCH_GET_SEMA_CONTROL;
	rCmd.u4Addr = 0;

	u4Status = wlanSendInitSetQueryCmd(prAdapter,
		INIT_CMD_ID_BT_PATCH_SEMAPHORE_CONTROL, &rCmd, sizeof(rCmd),
		TRUE, TRUE,
		INIT_EVENT_ID_BT_PATCH_SEMA_CTRL, &rEvent, sizeof(rEvent));
	if (u4Status != WLAN_STATUS_SUCCESS)
		goto exit;

	*pucPatchStatus = rEvent.ucStatus;

exit:
	return u4Status;
}

uint32_t wlanImageSectionGetBtPatchInfo(struct ADAPTER *prAdapter,
	void *pvFwImageMapFile, uint32_t u4FwImageFileLength,
	struct patch_dl_target *target)
{
	struct PATCH_FORMAT_V2_T *prPatchFormat;
	struct PATCH_GLO_DESC *glo_desc;
	struct PATCH_SEC_MAP *sec_map;
	struct patch_dl_buf *region;
	uint32_t section_type;
	uint32_t num_of_region, i, region_index;
	uint32_t u4Status = WLAN_STATUS_FAILURE;
	uint8_t *img_ptr;
	uint8_t aucBuffer[32];

	/* patch header */
	img_ptr = pvFwImageMapFile;
	prPatchFormat = (struct PATCH_FORMAT_V2_T *)img_ptr;

	/* Dump image information */
	kalMemZero(aucBuffer, 32);
	kalStrnCpy(aucBuffer, prPatchFormat->aucPlatform, 4);
	DBGLOG(INIT, INFO,
	       "PATCH INFO: platform[%s] HW/SW ver[0x%04X] ver[0x%04X]\n",
	       aucBuffer, prPatchFormat->u4SwHwVersion,
	       prPatchFormat->u4PatchVersion);

	kalStrnCpy(aucBuffer, prPatchFormat->aucBuildDate, 16);
	DBGLOG(INIT, INFO, "date[%s]\n", aucBuffer);

	if (prPatchFormat->u4PatchVersion != PATCH_VERSION_MAGIC_NUM) {
		DBGLOG(INIT, ERROR, "BT Patch format isn't V2\n");
		return WLAN_STATUS_FAILURE;
	}

	/* global descriptor */
	img_ptr += sizeof(struct PATCH_FORMAT_V2_T);
	glo_desc = (struct PATCH_GLO_DESC *)img_ptr;
	num_of_region = le2cpu32(glo_desc->section_num);
	DBGLOG(INIT, INFO,
			"\tPatch ver: 0x%x, Section num: 0x%x, subsys: 0x%x\n",
			glo_desc->patch_ver,
			num_of_region,
			le2cpu32(glo_desc->subsys));

	/* section map */
	img_ptr += sizeof(struct PATCH_GLO_DESC);

	target->num_of_region = num_of_region;
	target->patch_region = (struct patch_dl_buf *)kalMemAlloc(
				num_of_region * sizeof(struct patch_dl_buf),
				VIR_MEM_TYPE);

	if (!target->patch_region) {
		DBGLOG(INIT, WARN, "No memory to allocate.\n");
		return WLAN_STATUS_FAILURE;
	}

	kalMemZero(target->patch_region,
		   num_of_region * sizeof(struct patch_dl_buf));

	for (i = 0, region_index = 0; i < num_of_region; i++) {
		region = &target->patch_region[region_index];
		region->img_ptr = NULL;

		sec_map = (struct PATCH_SEC_MAP *)img_ptr;
		img_ptr += sizeof(struct PATCH_SEC_MAP);

		section_type = le2cpu32(sec_map->section_type);

		if ((section_type & PATCH_SEC_TYPE_MASK) !=
		     PATCH_SEC_TYPE_BIN_INFO)
			continue;

		region->bin_type = le2cpu32(sec_map->bin_info_spec.bin_type);
		/* only handle BT Patch */
		if (region->bin_type != FW_SECT_BINARY_TYPE_BT_PATCH &&
		  region->bin_type != FW_SECT_BINARY_TYPE_BT_CACHEABLE_PATCH &&
		  region->bin_type != FW_SECT_BINARY_TYPE_BT_RAM_POS)
			continue;

		region->img_dest_addr =
			le2cpu32(sec_map->bin_info_spec.dl_addr);
		/* PDA needs 16-byte aligned length */
		region->img_size =
			le2cpu32(sec_map->bin_info_spec.dl_size) +
			le2cpu32(sec_map->bin_info_spec.align_len);
		if (!(region->img_size % 16))
			DBGLOG(INIT, WARN,
			       "BT Patch is not 16-byte aligned\n");
		region->img_ptr = pvFwImageMapFile +
			le2cpu32(sec_map->section_offset);
		region->sec_info = le2cpu32(sec_map->bin_info_spec.sec_info);

		region->data_mode = wlanGetPatchDataModeV2(prAdapter,
							   region->sec_info);

		DBGLOG(INIT, INFO,
		       "BT Patch addr=0x%x: size=%d, ptr=0x%p, mode=0x%x, sec=0x%x, type=0x%x\n",
			region->img_dest_addr, region->img_size,
			region->img_ptr, region->data_mode,
			region->sec_info, region->bin_type);

		region_index++;
	}

	DBGLOG(INIT, INFO, "BT image region dl_count=0x%x, total=0x%x\n",
	       region_index, num_of_region);

	if (region_index == 0) {
		DBGLOG(INIT, ERROR, "Can't find the BT Patch\n");
		kalMemFree(target->patch_region, PHY_MEM_TYPE,
			num_of_region * sizeof(struct patch_dl_buf));
		target->patch_region = NULL;

	} else {
		u4Status = WLAN_STATUS_SUCCESS;
	}

	return u4Status;
}


int32_t wlanBtPatchIsDownloaded(struct ADAPTER *prAdapter)
{
	uint8_t ucPatchStatus = PATCH_STATUS_NO_SEMA_NEED_PATCH;
	uint32_t u4Count = 0;
	uint32_t rStatus;

	while (ucPatchStatus == PATCH_STATUS_NO_SEMA_NEED_PATCH) {
		if (u4Count)
			kalMdelay(100);

		rStatus = wlanBtPatchSendSemaControl(prAdapter, &ucPatchStatus);
		if (rStatus != WLAN_STATUS_SUCCESS)
			return -1;

		u4Count++;

		if (u4Count > 50) {
			DBGLOG(INIT, WARN, "Patch status check timeout!!\n");
			return -2;
		}
	}

	if (ucPatchStatus == PATCH_STATUS_NO_NEED_TO_PATCH)
		return 1;
	else
		return 0;
}

uint32_t asicConnac3xConfigBtImageSection(
	struct ADAPTER *prAdapter,
	struct patch_dl_buf *region)
{
	struct INIT_CMD_CO_DOWNLOAD_CONFIG rCmd = {0};
	struct INIT_EVENT_CMD_RESULT rEvent = {0};
	uint8_t ucCmdId;
	u_int8_t fgCheckStatus = FALSE;
	uint32_t u4Status;

	rCmd.u4Address = region->img_dest_addr;
	rCmd.u4Length = region->img_size;
	rCmd.u4DataMode = region->data_mode;
	rCmd.u4SecInfo = region->sec_info;
	rCmd.u4BinType = region->bin_type;

	ucCmdId = INIT_CMD_ID_CO_PATCH_DOWNLOAD_CONFIG;

#if CFG_ENABLE_FW_DOWNLOAD_ACK
	fgCheckStatus = TRUE;
#else
	fgCheckStatus = FALSE;
#endif

	u4Status = wlanSendInitSetQueryCmd(prAdapter,
		ucCmdId, &rCmd, sizeof(rCmd),
		fgCheckStatus, TRUE,
		INIT_EVENT_ID_CMD_RESULT, &rEvent, sizeof(rEvent));

	if (fgCheckStatus && rEvent.ucStatus != 0) {
		DBGLOG(INIT, ERROR, "Event status: %d\n", rEvent.ucStatus);
		u4Status = WLAN_STATUS_FAILURE;
	}

	return u4Status;
}

uint32_t asicConnac3xDownloadBtPatch(struct ADAPTER *prAdapter)
{
	uint32_t u4FwSize = 0;
	uint32_t u4Status = WLAN_STATUS_FAILURE;
	uint32_t u4DataMode = 0;
	int32_t i4BtPatchCheck;
	struct patch_dl_target target = {0};
	void *prFwBuffer = NULL;

#if CFG_SUPPORT_COMPRESSION_FW_OPTION
	#pragma message("WARN: Download BT Patch doesn't support COMPRESSION")
#endif
#if CFG_DOWNLOAD_DYN_MEMORY_MAP
	#pragma message("WARN: Download BT Patch doesn't support DYN_MEM_MAP")
#endif
#if CFG_ROM_PATCH_NO_SEM_CTRL
	#pragma message("WARN: Download BT Patch doesn't support NO_SEM_CTRL")
#endif

	if (!prAdapter)
		return WLAN_STATUS_FAILURE;

	DBGLOG(INIT, INFO, "BT Patch download start\n");

	/* Always check BT Patch Download for L0.5 reset case */

	/* refer from wlanImageSectionDownloadStage */

	/* step.1 open the PATCH file */
	kalFirmwareImageMapping(prAdapter->prGlueInfo, &prFwBuffer,
				&u4FwSize, IMG_DL_IDX_BT_PATCH);
	if (prFwBuffer == NULL) {
		DBGLOG(INIT, WARN, "FW[%u] load error!\n",
		       IMG_DL_IDX_BT_PATCH);
		return WLAN_STATUS_FAILURE;
	}

	/* step 2. get Addr info. Refer from : wlanImageSectionDownloadStage */
	u4Status = wlanImageSectionGetBtPatchInfo(prAdapter,
			prFwBuffer, u4FwSize, &target);

	if (u4Status != WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, ERROR, "Can't find the BT Patch Section\n");
		goto out;
	}

	/* step 3. check BT doesn't download PATCH */
	i4BtPatchCheck = wlanBtPatchIsDownloaded(prAdapter);
	if (i4BtPatchCheck < 0) {
		DBGLOG(INIT, INFO, "Get BT Semaphore Fail\n");
		u4Status =  WLAN_STATUS_FAILURE;
		goto out;
	} else if (i4BtPatchCheck == 1) {
		DBGLOG(INIT, INFO, "No need to download patch\n");
		u4Status =  WLAN_STATUS_SUCCESS;
		goto out;
	}

	/* step 4. download BT patch */
	u4Status = wlanDownloadSectionV2(prAdapter, u4DataMode,
					IMG_DL_IDX_BT_PATCH, &target);
	if (u4Status != WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, ERROR, "BT Patch download Fail\n");
		goto out;
	}

	/* step 5. send INIT_CMD_PATCH_FINISH */
	u4Status = wlanPatchSendComplete(prAdapter, PATCH_FNSH_TYPE_BT);

	if (u4Status != WLAN_STATUS_SUCCESS)
		DBGLOG(INIT, ERROR, "Send INIT_CMD_PATCH_FINISH Fail\n");
	else
		DBGLOG(INIT, INFO, "BT Patch download success\n");

out:
	if (target.patch_region != NULL) {
		/* This case is that the BT patch isn't downloaded this time.
		 * The original free action is in wlanDownloadSectionV2().
		 */
		kalMemFree(target.patch_region, PHY_MEM_TYPE,
			sizeof(struct patch_dl_buf) * target.num_of_region);
		target.patch_region = NULL;
		target.num_of_region = 0;
	}

	kalFirmwareImageUnmapping(prAdapter->prGlueInfo, NULL, prFwBuffer);

	return u4Status;
}
#endif /* CFG_SUPPORT_WIFI_DL_BT_PATCH */
#endif /* CFG_SUPPORT_CONNAC3X == 1 */


#endif  /* CFG_ENABLE_FW_DOWNLOAD */
