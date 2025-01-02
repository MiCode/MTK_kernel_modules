/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 * Copyright (c) 2016 MediaTek Inc.
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

#if CFG_ENABLE_FW_DOWNLOAD
uint32_t wlanGetDataMode(IN struct ADAPTER *prAdapter,
	IN enum ENUM_IMG_DL_IDX_T eDlIdx, IN uint8_t ucFeatureSet)
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

uint32_t wlanGetPatchDataModeV2(IN struct ADAPTER *prAdapter,
	IN uint32_t u4SecInfo)
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

void wlanGetHarvardFwInfo(IN struct ADAPTER *prAdapter,
	IN uint8_t u4SecIdx, IN enum ENUM_IMG_DL_IDX_T eDlIdx,
	OUT uint32_t *pu4Addr, OUT uint32_t *pu4Len,
	OUT uint32_t *pu4DataMode, OUT u_int8_t *pfgIsEMIDownload,
	OUT u_int8_t *pfgIsNotDownload)
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

void wlanGetConnacFwInfo(IN struct ADAPTER *prAdapter,
	IN uint8_t u4SecIdx, IN enum ENUM_IMG_DL_IDX_T eDlIdx,
	OUT uint32_t *pu4Addr, OUT uint32_t *pu4Len,
	OUT uint32_t *pu4DataMode, OUT u_int8_t *pfgIsEMIDownload,
	OUT u_int8_t *pfgIsNotDownload)
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
void wlanImageSectionGetCompressFwInfo(IN struct ADAPTER
	*prAdapter, IN void *pvFwImageMapFile,
	IN uint32_t u4FwImageFileLength, IN uint8_t ucTotSecNum,
	IN uint8_t ucCurSecNum, IN enum ENUM_IMG_DL_IDX_T eDlIdx,
	OUT uint32_t *pu4Addr, OUT uint32_t *pu4Len,
	OUT uint32_t *pu4DataMode, OUT uint32_t *pu4BlockSize,
	OUT uint32_t *pu4CRC, OUT uint32_t *pu4UncompressedLength)
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
			   sizeof(prTailer->ram_version));
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

void wlanImageSectionGetPatchInfo(IN struct ADAPTER
	*prAdapter,
	IN void *pvFwImageMapFile, IN uint32_t u4FwImageFileLength,
	OUT uint32_t *pu4StartOffset, OUT uint32_t *pu4Addr,
	OUT uint32_t *pu4Len,
	OUT uint32_t *pu4DataMode)
{
	struct PATCH_FORMAT_T *prPatchFormat;
	uint8_t aucBuffer[32];
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;

	prPatchFormat = (struct PATCH_FORMAT_T *) pvFwImageMapFile;

	*pu4StartOffset = offsetof(struct PATCH_FORMAT_T,
				   ucPatchImage);
	*pu4Addr = prChipInfo->patch_addr;
	*pu4Len = u4FwImageFileLength - offsetof(struct
			PATCH_FORMAT_T, ucPatchImage);
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

uint32_t wlanGetPatchInfoAndDownloadV2(IN struct ADAPTER
	*prAdapter,
	IN void *pvFwImageMapFile, IN uint32_t u4FwImageFileLength,
	IN enum ENUM_IMG_DL_IDX_T eDlIdx,
	IN uint32_t u4DataMode)
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
			region->img_ptr = pvFwImageMapFile +
				be2cpu32(sec_map->section_offset);
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

	if (prFwDlOps->downloadSectionV2) {
		u4Status = prFwDlOps->downloadSectionV2(
			prAdapter, u4DataMode,
			eDlIdx, &target);
	} else {
		u4Status = wlanDownloadSectionV2(
			prAdapter, u4DataMode,
			eDlIdx, &target);
	}

	if (target.patch_region)
		kalMemFree(target.patch_region, PHY_MEM_TYPE,
			num_of_region * sizeof(struct patch_dl_buf));

	return u4Status;
}

uint32_t wlanDownloadSection(IN struct ADAPTER *prAdapter,
			     IN uint32_t u4Addr, IN uint32_t u4Len,
			     IN uint32_t u4DataMode, IN uint8_t *pucStartPtr,
			     IN enum ENUM_IMG_DL_IDX_T eDlIdx)
{
	uint32_t u4ImgSecSize, u4Offset;
	uint8_t *pucSecBuf;

	if (wlanImageSectionConfig(prAdapter, u4Addr, u4Len,
				   u4DataMode, eDlIdx) != WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, ERROR,
		       "Firmware download configuration failed!\n");
		return WLAN_STATUS_FAILURE;
	}

	for (u4Offset = 0; u4Offset < u4Len;
	     u4Offset += CMD_PKT_SIZE_FOR_IMAGE) {
		if (u4Offset + CMD_PKT_SIZE_FOR_IMAGE < u4Len)
			u4ImgSecSize = CMD_PKT_SIZE_FOR_IMAGE;
		else
			u4ImgSecSize = u4Len - u4Offset;

		pucSecBuf = (uint8_t *) pucStartPtr + u4Offset;
		if (wlanImageSectionDownload(prAdapter, u4ImgSecSize,
					     pucSecBuf) !=
					     WLAN_STATUS_SUCCESS) {
			DBGLOG(INIT, ERROR,
				"Firmware scatter download failed!\n");
			return WLAN_STATUS_FAILURE;
		}
	}

	return WLAN_STATUS_SUCCESS;
}

uint32_t wlanDownloadSectionV2(IN struct ADAPTER *prAdapter,
		IN uint32_t u4DataMode,
		IN enum ENUM_IMG_DL_IDX_T eDlIdx,
		struct patch_dl_target *target)
{
	uint32_t u4ImgSecSize, u4Offset;
	uint8_t *pucSecBuf;
	uint32_t num_of_region, i;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;

	num_of_region = target->num_of_region;
	for (i = 0; i < num_of_region; i++) {
		struct patch_dl_buf *region;

		region = &target->patch_region[i];
		if (region->img_ptr == NULL)
			continue;

		/* 2. config PDA */
		if (wlanImageSectionConfig(prAdapter, region->img_dest_addr,
			region->img_size, u4DataMode, eDlIdx) !=
			WLAN_STATUS_SUCCESS) {
			DBGLOG(INIT, ERROR,
				"Firmware download configuration failed!\n");
			u4Status = WLAN_STATUS_FAILURE;
			goto out;
		}

		/* 3. image scatter */
		for (u4Offset = 0; u4Offset < region->img_size;
			u4Offset += CMD_PKT_SIZE_FOR_IMAGE) {
			if (u4Offset + CMD_PKT_SIZE_FOR_IMAGE <
				region->img_size)
				u4ImgSecSize = CMD_PKT_SIZE_FOR_IMAGE;
			else
				u4ImgSecSize = region->img_size - u4Offset;

			pucSecBuf = (uint8_t *) region->img_ptr + u4Offset;
			if (wlanImageSectionDownload(prAdapter, u4ImgSecSize,
					pucSecBuf) !=
					WLAN_STATUS_SUCCESS) {
				DBGLOG(INIT, ERROR,
					"Firmware scatter download failed!\n");

				u4Status = WLAN_STATUS_FAILURE;
				goto out;
			}
		}
	}

out:
	return u4Status;
}

uint32_t wlanDownloadEMISection(IN struct ADAPTER
				*prAdapter, IN uint32_t u4DestAddr,
				IN uint32_t u4Len, IN uint8_t *pucStartPtr)
{
#if CFG_MTK_ANDROID_EMI
	uint8_t __iomem *pucEmiBaseAddr = NULL;
	uint32_t u4Offset = u4DestAddr & WIFI_EMI_ADDR_MASK;

	if (!gConEmiPhyBase) {
		DBGLOG(INIT, ERROR,
		       "Consys emi memory address gConEmiPhyBase invalid\n");
		return WLAN_STATUS_FAILURE;
	}

	request_mem_region(gConEmiPhyBase, gConEmiSize, "WIFI-EMI");
	kalSetEmiMpuProtection(gConEmiPhyBase, false);
	pucEmiBaseAddr = ioremap_nocache(gConEmiPhyBase, gConEmiSize);
	DBGLOG(INIT, INFO,
	       "EmiPhyBase:0x%llx offset:0x%x, ioremap region 0x%lX @ 0x%lX\n",
	       (uint64_t)gConEmiPhyBase, u4Offset, gConEmiSize, pucEmiBaseAddr);
	if (!pucEmiBaseAddr) {
		DBGLOG(INIT, ERROR, "ioremap failed\n");
		return WLAN_STATUS_FAILURE;
	}

	kalMemCopy((pucEmiBaseAddr + u4Offset), pucStartPtr, u4Len);

	kalSetEmiMpuProtection(gConEmiPhyBase, true);
	iounmap(pucEmiBaseAddr);
	release_mem_region(gConEmiPhyBase, gConEmiSize);
#endif /* CFG_MTK_ANDROID_EMI */
	return WLAN_STATUS_SUCCESS;
}

#if CFG_SUPPORT_COMPRESSION_FW_OPTION
u_int8_t wlanImageSectionCheckFwCompressInfo(
	IN struct ADAPTER *prAdapter,
	IN void *pvFwImageMapFile, IN uint32_t u4FwImageFileLength,
	IN enum ENUM_IMG_DL_IDX_T eDlIdx)
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
	IN struct ADAPTER *prAdapter, IN void *pvFwImageMapFile,
	IN uint32_t u4FwImageFileLength, IN uint8_t ucSectionNumber,
	IN enum ENUM_IMG_DL_IDX_T eDlIdx,
	OUT uint8_t *pucIsCompressed,
	OUT struct INIT_CMD_WIFI_DECOMPRESSION_START *prFwImageInFo)
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
	struct FWDL_OPS_T *prFwDlOps;

	prFwDlOps = prAdapter->chip_info->fw_dl_ops;
	ASSERT(prFwDlOps);

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

				if (prFwDlOps->downloadSection) {
					u4Status = prFwDlOps->downloadSection(
						prAdapter,
						u4Addr + u4offset,
						u4ChunkSize,
						u4DataMode,
						pvFwImageMapFile + u4FileOffset,
						eDlIdx);
				} else {

					u4Status = wlanDownloadSection(
						prAdapter,
						u4Addr + u4offset,
						u4ChunkSize,
						u4DataMode,
						pvFwImageMapFile + u4FileOffset,
						eDlIdx);
				}
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
				ucSectionNumber, eDlIdx);
		*pucIsCompressed = FALSE;
	}
	return u4Status;
}
#endif

uint32_t wlanImageSectionDownloadStage(
	IN struct ADAPTER *prAdapter, IN void *pvFwImageMapFile,
	IN uint32_t u4FwImageFileLength, IN uint8_t ucSectionNumber,
	IN enum ENUM_IMG_DL_IDX_T eDlIdx)
{
	uint32_t u4SecIdx, u4Offset = 0;
	uint32_t u4Addr, u4Len, u4DataMode = 0;
	u_int8_t fgIsEMIDownload = FALSE;
	u_int8_t fgIsNotDownload = FALSE;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;
	struct PATCH_FORMAT_T *prPatchHeader;
	struct FWDL_OPS_T *prFwDlOps;

	prFwDlOps = prChipInfo->fw_dl_ops;
	ASSERT(prFwDlOps);

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
			DBGLOG(INIT, INFO,
		"FormatV1 DL Offset[%u] addr[0x%08x] len[%u] datamode[0x%08x]\n",
		       u4Offset, u4Addr, u4Len, u4DataMode);
/* For dynamic memory map::Begin */
#if (CFG_DOWNLOAD_DYN_MEMORY_MAP == 1)
			u4Status = prFwDlOps->downloadByDynMemMap(
						prAdapter, u4Addr, u4Len,
						pvFwImageMapFile
							+ u4Offset,
							eDlIdx);
#else
			if (prFwDlOps->downloadSection) {
				u4Status = prFwDlOps->downloadSection(
							prAdapter,
							u4Addr,
							u4Len,
							u4DataMode,
							pvFwImageMapFile
							+ u4Offset,
							eDlIdx);

			} else {
				u4Status = wlanDownloadSection(
							prAdapter,
							u4Addr,
							u4Len,
							u4DataMode,
							pvFwImageMapFile
							+ u4Offset,
							eDlIdx);
			}
#endif
		}
/* For dynamic memory map::End */
	} else {
		for (u4SecIdx = 0; u4SecIdx < ucSectionNumber;
		     u4SecIdx++, u4Offset += u4Len) {
			prChipInfo->fw_dl_ops->getFwInfo(prAdapter, u4SecIdx,
				eDlIdx, &u4Addr,
				&u4Len, &u4DataMode, &fgIsEMIDownload,
				&fgIsNotDownload);

			DBGLOG(INIT, INFO,
			       "DL Offset[%u] addr[0x%08x] len[%u] datamode[0x%08x]\n",
			       u4Offset, u4Addr, u4Len, u4DataMode);

			if (fgIsNotDownload)
				continue;
			else if (fgIsEMIDownload)
				u4Status = wlanDownloadEMISection(prAdapter,
					u4Addr, u4Len,
					pvFwImageMapFile + u4Offset);
/* For dynamic memory map:: Begin */
#if (CFG_DOWNLOAD_DYN_MEMORY_MAP == 1)
			else if ((u4DataMode &
				DOWNLOAD_CONFIG_ENCRYPTION_MODE) == 0) {
				/* Non-encrypted F/W region,
				 * use dynamic memory mapping for download
				 */
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
				if (prFwDlOps->downloadSection) {
					u4Status = prFwDlOps->downloadSection(
						prAdapter,
						u4Addr, u4Len,
						u4DataMode,
						pvFwImageMapFile + u4Offset,
						eDlIdx);
				} else {
					u4Status = wlanDownloadSection(
						prAdapter,
						u4Addr, u4Len,
						u4DataMode,
						pvFwImageMapFile + u4Offset,
						eDlIdx);
				}

			/* escape from loop if any pending error occurs */
			if (u4Status == WLAN_STATUS_FAILURE)
				break;
		}
	}

	return u4Status;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called to confirm the status of
 *        previously patch semaphore control
 *
 * @param prAdapter      Pointer to the Adapter structure.
 *        ucCmdSeqNum    Sequence number of previous firmware scatter
 *
 * @return WLAN_STATUS_SUCCESS
 *         WLAN_STATUS_FAILURE
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanPatchRecvSemaResp(IN struct ADAPTER *prAdapter,
	IN uint8_t ucCmdSeqNum, OUT uint8_t *pucPatchStatus)
{
	struct mt66xx_chip_info *prChipInfo;
	uint8_t *aucBuffer;
	uint32_t u4EventSize;
	struct INIT_WIFI_EVENT *prInitEvent;
	struct INIT_EVENT_CMD_RESULT *prEventCmdResult;
	uint32_t u4RxPktLength;

	ASSERT(prAdapter);
	prChipInfo = prAdapter->chip_info;

	if (kalIsCardRemoved(prAdapter->prGlueInfo) == TRUE
	    || fgIsBusAccessFailed == TRUE)
		return WLAN_STATUS_FAILURE;

	u4EventSize = prChipInfo->rxd_size + prChipInfo->init_event_size +
		sizeof(struct INIT_EVENT_CMD_RESULT);
	aucBuffer = kalMemAlloc(u4EventSize, PHY_MEM_TYPE);
	if (aucBuffer == NULL) {
		DBGLOG(INIT, ERROR, "Alloc CMD buffer failed\n");
		return WLAN_STATUS_FAILURE;
	}

	if (nicRxWaitResponse(prAdapter, 0, aucBuffer, u4EventSize,
			      &u4RxPktLength) != WLAN_STATUS_SUCCESS) {

		DBGLOG(INIT, WARN, "Wait patch semaphore response fail\n");
		kalMemFree(aucBuffer, PHY_MEM_TYPE, u4EventSize);
		return WLAN_STATUS_FAILURE;
	}

	prInitEvent = (struct INIT_WIFI_EVENT *)
		(aucBuffer + prChipInfo->rxd_size);
	if (prInitEvent == NULL) {
		DBGLOG(INIT, ERROR, "prInitEvent is NULL\n");
		kalMemFree(aucBuffer, PHY_MEM_TYPE, u4EventSize);
		return WLAN_STATUS_FAILURE;
	}
	if (prInitEvent->ucEID != INIT_EVENT_ID_PATCH_SEMA_CTRL) {
		DBGLOG(INIT, WARN, "Unexpected EVENT ID, get 0x%0x\n",
		       prInitEvent->ucEID);
		kalMemFree(aucBuffer, PHY_MEM_TYPE, u4EventSize);
		return WLAN_STATUS_FAILURE;
	}

	if (prInitEvent->ucSeqNum != ucCmdSeqNum) {
		DBGLOG(INIT, WARN, "Unexpected SeqNum %d, %d\n",
		       ucCmdSeqNum, prInitEvent->ucSeqNum);
		kalMemFree(aucBuffer, PHY_MEM_TYPE, u4EventSize);
		return WLAN_STATUS_FAILURE;
	}

	prEventCmdResult = (struct INIT_EVENT_CMD_RESULT *)
		prInitEvent->aucBuffer;

	*pucPatchStatus = prEventCmdResult->ucStatus;

	kalMemFree(aucBuffer, PHY_MEM_TYPE, u4EventSize);

	return WLAN_STATUS_SUCCESS;
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
uint32_t wlanPatchSendSemaControl(IN struct ADAPTER *prAdapter,
				  OUT uint8_t *pucSeqNum)
{
	struct mt66xx_chip_info *prChipInfo;
	struct CMD_INFO *prCmdInfo;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
	struct INIT_CMD_PATCH_SEMA_CONTROL *prPatchSemaControl;

	ASSERT(prAdapter);
	prChipInfo = prAdapter->chip_info;

	DEBUGFUNC("wlanImagePatchSemaphoreCheck");

	/* 1. Allocate CMD Info Packet and its Buffer. */
	prCmdInfo = cmdBufAllocateCmdInfo(prAdapter,
		sizeof(struct INIT_HIF_TX_HEADER) +
		sizeof(struct INIT_HIF_TX_HEADER_PENDING_FOR_HW_32BYTES) +
		sizeof(struct INIT_CMD_PATCH_SEMA_CONTROL));

	if (!prCmdInfo) {
		DBGLOG(INIT, ERROR, "Allocate CMD_INFO_T ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	prCmdInfo->u2InfoBufLen = sizeof(struct INIT_HIF_TX_HEADER) +
		sizeof(struct INIT_HIF_TX_HEADER_PENDING_FOR_HW_32BYTES) +
		sizeof(struct INIT_CMD_PATCH_SEMA_CONTROL);

	prCmdInfo->ucCID = INIT_CMD_ID_PATCH_SEMAPHORE_CONTROL;
	NIC_FILL_CMD_TX_HDR(prAdapter,
		prCmdInfo->pucInfoBuffer,
		prCmdInfo->u2InfoBufLen,
		prCmdInfo->ucCID,
		INIT_CMD_PDA_PACKET_TYPE_ID,
		pucSeqNum, FALSE,
		(void **)&prPatchSemaControl, TRUE, 0, S2D_INDEX_CMD_H2N,
		FALSE);

	kalMemZero(prPatchSemaControl,
		   sizeof(struct INIT_CMD_PATCH_SEMA_CONTROL));

	/* Setup DOWNLOAD_BUF */
	kalMemZero(prPatchSemaControl,
		   sizeof(struct INIT_CMD_PATCH_SEMA_CONTROL));
	prPatchSemaControl->ucGetSemaphore = PATCH_GET_SEMA_CONTROL;

	/* 4. Send FW_Download command */
	if (nicTxInitCmd(prAdapter, prCmdInfo,
			 prChipInfo->u2TxInitCmdPort) != WLAN_STATUS_SUCCESS) {
		u4Status = WLAN_STATUS_FAILURE;
		DBGLOG(INIT, ERROR,
		       "Fail to transmit image download command\n");
	}
	/* 5. Free CMD Info Packet. */
	cmdBufFreeCmdInfo(prAdapter, prCmdInfo);

	return u4Status;
}

u_int8_t wlanPatchIsDownloaded(IN struct ADAPTER *prAdapter)
{
	uint8_t ucSeqNum, ucPatchStatus;
	uint32_t rStatus;
	uint32_t u4Count;

	ucPatchStatus = PATCH_STATUS_NO_SEMA_NEED_PATCH;
	u4Count = 0;

	while (ucPatchStatus == PATCH_STATUS_NO_SEMA_NEED_PATCH) {
		if (u4Count)
			kalMdelay(100);

		rStatus = wlanPatchSendSemaControl(prAdapter, &ucSeqNum);
		if (rStatus != WLAN_STATUS_SUCCESS) {
			DBGLOG(INIT, WARN,
			       "Send patch SEMA control CMD failed!!\n");
			break;
		}

		rStatus = wlanPatchRecvSemaResp(prAdapter, ucSeqNum,
						&ucPatchStatus);
		if (rStatus != WLAN_STATUS_SUCCESS) {
			DBGLOG(INIT, WARN,
			       "Recv patch SEMA control EVT failed!!\n");
			break;
		}

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

uint32_t wlanPatchSendComplete(IN struct ADAPTER *prAdapter
#if CFG_SUPPORT_WIFI_DL_BT_PATCH || CFG_SUPPORT_WIFI_DL_ZB_PATCH
			       , IN uint8_t ucPatchType
#endif
				)
{
	struct CMD_INFO *prCmdInfo;
	uint8_t ucTC, ucCmdSeqNum;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
	struct mt66xx_chip_info *prChipInfo;
	struct INIT_CMD_PATCH_FINISH *prPatchFinish;

	ASSERT(prAdapter);
	prChipInfo = prAdapter->chip_info;

	/* 1. Allocate CMD Info Packet and its Buffer. */
	prCmdInfo = cmdBufAllocateCmdInfo(prAdapter,
		sizeof(struct INIT_HIF_TX_HEADER) +
		sizeof(struct INIT_HIF_TX_HEADER_PENDING_FOR_HW_32BYTES) +
		sizeof(struct INIT_CMD_PATCH_FINISH));

	if (!prCmdInfo) {
		DBGLOG(INIT, ERROR, "Allocate CMD_INFO_T ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	kalMemZero(prCmdInfo->pucInfoBuffer,
		sizeof(struct INIT_HIF_TX_HEADER) +
		sizeof(struct INIT_HIF_TX_HEADER_PENDING_FOR_HW_32BYTES) +
		sizeof(struct INIT_CMD_PATCH_FINISH));
	prCmdInfo->u2InfoBufLen = sizeof(struct INIT_HIF_TX_HEADER) +
		sizeof(struct INIT_HIF_TX_HEADER_PENDING_FOR_HW_32BYTES) +
		sizeof(struct INIT_CMD_PATCH_FINISH);

#if (CFG_USE_TC4_RESOURCE_FOR_INIT_CMD == 1)
	/* 2. Always use TC4 (TC4 as CPU) */
	ucTC = TC4_INDEX;
#else
	/* 2. Use TC0's resource to send patch finish command.
	 * Only TC0 is allowed because SDIO HW always reports
	 * MCU's TXQ_CNT at TXQ0_CNT in CR4 architecutre)
	 */
	ucTC = TC0_INDEX;
#endif

	prCmdInfo->ucCID = INIT_CMD_ID_PATCH_FINISH;
	NIC_FILL_CMD_TX_HDR(prAdapter,
		prCmdInfo->pucInfoBuffer,
		prCmdInfo->u2InfoBufLen,
		prCmdInfo->ucCID,
		INIT_CMD_PACKET_TYPE_ID,
		&ucCmdSeqNum,
		FALSE,
		(void **)&prPatchFinish, TRUE, 0, S2D_INDEX_CMD_H2N, FALSE);

	prPatchFinish->ucCheckCrc = 0;
#if CFG_SUPPORT_WIFI_DL_BT_PATCH || CFG_SUPPORT_WIFI_DL_ZB_PATCH
	prPatchFinish->ucType = ucPatchType;
#endif

	/* 5. Seend WIFI start command */
	while (1) {
		/* 5.1 Acquire TX Resource */
		if (nicTxAcquireResource(prAdapter, ucTC,
				nicTxGetCmdPageCount(prAdapter,prCmdInfo),
					 TRUE) == WLAN_STATUS_RESOURCES) {
			if (nicTxPollingResource(prAdapter,
						 ucTC) != WLAN_STATUS_SUCCESS) {
				u4Status = WLAN_STATUS_FAILURE;
				DBGLOG(INIT, ERROR,
				       "Fail to get TX resource return within timeout\n");
				goto exit;
			}
			continue;
		}
		/* 5.2 Send CMD Info Packet */
		if (nicTxInitCmd(prAdapter, prCmdInfo,
				 prChipInfo->u2TxInitCmdPort) !=
				 WLAN_STATUS_SUCCESS) {
			u4Status = WLAN_STATUS_FAILURE;
			DBGLOG(INIT, ERROR,
			       "Fail to transmit WIFI start command\n");
			goto exit;
		}

		break;
	};

	DBGLOG(INIT, INFO,
	       "PATCH FINISH CMD send, waiting for RSP\n");

	/* kalMdelay(10000); */
#if CFG_SUPPORT_WIFI_DL_BT_PATCH || CFG_SUPPORT_WIFI_DL_ZB_PATCH
	/* BT always response with ucCmdSeqNum=0 */
	if ((ucPatchType != PATCH_FNSH_TYPE_WF) &&
	    (ucPatchType != PATCH_FNSH_TYPE_WF_MD))
		ucCmdSeqNum = 0;
#endif

	u4Status = wlanConfigWifiFuncStatus(prAdapter, ucCmdSeqNum);

	if (u4Status != WLAN_STATUS_SUCCESS)
		DBGLOG(INIT, INFO, "PATCH FINISH EVT failed\n");
	else
		DBGLOG(INIT, INFO, "PATCH FINISH EVT success!!\n");

exit:
	/* 6. Free CMD Info Packet. */
	cmdBufFreeCmdInfo(prAdapter, prCmdInfo);

	return u4Status;
}

#if (CFG_DOWNLOAD_DYN_MEMORY_MAP == 1)
uint32_t wlanPatchDynMemMapSendComplete(IN struct ADAPTER *prAdapter)
{
	struct CMD_INFO *prCmdInfo;
	uint8_t ucTC, ucCmdSeqNum;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
	struct mt66xx_chip_info *prChipInfo;
	struct INIT_CMD_PATCH_FINISH *prPatchFinish;

	ASSERT(prAdapter);
	prChipInfo = prAdapter->chip_info;

	/* 1. Allocate CMD Info Packet and its Buffer. */
	prCmdInfo = cmdBufAllocateCmdInfo(prAdapter,
		sizeof(struct INIT_HIF_TX_HEADER) +
		sizeof(struct INIT_HIF_TX_HEADER_PENDING_FOR_HW_32BYTES) +
		sizeof(struct INIT_CMD_PATCH_FINISH));

	if (!prCmdInfo) {
		DBGLOG(INIT, ERROR, "Allocate CMD_INFO_T ==> FAILED.\n");

		return WLAN_STATUS_FAILURE;
	}

	kalMemZero(prCmdInfo->pucInfoBuffer,
		sizeof(struct INIT_HIF_TX_HEADER) +
		sizeof(struct INIT_HIF_TX_HEADER_PENDING_FOR_HW_32BYTES) +
		sizeof(struct INIT_CMD_PATCH_FINISH));

	prCmdInfo->u2InfoBufLen = sizeof(struct INIT_HIF_TX_HEADER) +
		sizeof(struct INIT_HIF_TX_HEADER_PENDING_FOR_HW_32BYTES) +
		sizeof(struct INIT_CMD_PATCH_FINISH);

#if (CFG_USE_TC4_RESOURCE_FOR_INIT_CMD == 1)
	/* 2. Always use TC4 (TC4 as CPU) */
	ucTC = TC4_INDEX;
#else
	/* 2. Use TC0's resource to send patch finish command.
	 * Only TC0 is allowed because SDIO HW always reports
	 * MCU's TXQ_CNT at TXQ0_CNT in CR4 architecutre)
	 */
	ucTC = TC0_INDEX;
#endif

	prCmdInfo->ucCID = INIT_CMD_ID_DYN_MEM_MAP_PATCH_FINISH;
	NIC_FILL_CMD_TX_HDR(prAdapter,
		prCmdInfo->pucInfoBuffer,
		prCmdInfo->u2InfoBufLen,
		prCmdInfo->ucCID,
		INIT_CMD_PACKET_TYPE_ID,
		&ucCmdSeqNum,
		FALSE,
		(void **)&prPatchFinish, TRUE, 0, S2D_INDEX_CMD_H2N, FALSE);

	prPatchFinish->ucCheckCrc = 0;

	/* 5. Seend WIFI start command */
	while (1) {
		/* 5.1 Acquire TX Resource */
		if (nicTxAcquireResource(prAdapter, ucTC,
			nicTxGetCmdPageCount(prAdapter,prCmdInfo),
			TRUE) == WLAN_STATUS_RESOURCES) {
			if (nicTxPollingResource(prAdapter,
				ucTC) != WLAN_STATUS_SUCCESS) {
				u4Status = WLAN_STATUS_FAILURE;
				DBGLOG(INIT, ERROR,
				"Fail to get TX resource return within timeout\n");
				goto exit;
			}
			continue;
		}
		/* 5.2 Send CMD Info Packet */
		if (nicTxInitCmd(prAdapter, prCmdInfo,
				prChipInfo->u2TxInitCmdPort) !=
				WLAN_STATUS_SUCCESS) {
			u4Status = WLAN_STATUS_FAILURE;
			DBGLOG(INIT, ERROR,
				"Fail to transmit WIFI start command\n");
			goto exit;
		}

		break;
	};

	DBGLOG(INIT, INFO,
	"PATCH FINISH CMD send, waiting for RSP\n");

	/* kalMdelay(10000); */

	u4Status = wlanConfigWifiFuncStatus(prAdapter, ucCmdSeqNum);

	if (u4Status != WLAN_STATUS_SUCCESS)
		DBGLOG(INIT, INFO, "PATCH FINISH EVT failed\n");
	else
		DBGLOG(INIT, INFO, "PATCH FINISH EVT success!!\n");

exit:
	/* 6. Free CMD Info Packet. */
	cmdBufFreeCmdInfo(prAdapter, prCmdInfo);

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
	IN struct ADAPTER *prAdapter,
	IN uint32_t u4DestAddr, IN uint32_t u4ImgSecSize,
	IN uint32_t u4DataMode,
	IN enum ENUM_IMG_DL_IDX_T eDlIdx)
{
	struct CMD_INFO *prCmdInfo;
	struct INIT_CMD_DOWNLOAD_CONFIG *prInitCmdDownloadConfig;
	uint8_t ucTC, ucCmdSeqNum;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
	struct mt66xx_chip_info *prChipInfo;

	ASSERT(prAdapter);
	prChipInfo = prAdapter->chip_info;

	DEBUGFUNC("wlanImageSectionConfig");

	if (u4ImgSecSize == 0)
		return WLAN_STATUS_SUCCESS;
	/* 1. Allocate CMD Info Packet and its Buffer. */
	prCmdInfo = cmdBufAllocateCmdInfo(prAdapter,
		sizeof(struct INIT_HIF_TX_HEADER) +
		sizeof(struct INIT_HIF_TX_HEADER_PENDING_FOR_HW_32BYTES) +
		sizeof(struct INIT_CMD_DOWNLOAD_CONFIG));

	if (!prCmdInfo) {
		DBGLOG(INIT, ERROR, "Allocate CMD_INFO_T ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	prCmdInfo->u2InfoBufLen = sizeof(struct INIT_HIF_TX_HEADER) +
		sizeof(struct INIT_HIF_TX_HEADER_PENDING_FOR_HW_32BYTES) +
		sizeof(struct INIT_CMD_DOWNLOAD_CONFIG);

#if (CFG_USE_TC4_RESOURCE_FOR_INIT_CMD == 1)
	/* 2. Use TC4's resource to download image. (TC4 as CPU) */
	ucTC = TC4_INDEX;
#else
	/* 2. Use TC0's resource to send init_cmd.
	 * Only TC0 is allowed because SDIO HW always reports
	 * MCU's TXQ_CNT at TXQ0_CNT in CR4 architecutre)
	 */
	ucTC = TC0_INDEX;
#endif

	switch (eDlIdx) {
#if CFG_SUPPORT_WIFI_DL_BT_PATCH
	case IMG_DL_IDX_BT_PATCH:
		kal_fallthrough;
#endif
#if CFG_SUPPORT_WIFI_DL_ZB_PATCH
	case IMG_DL_IDX_ZB_PATCH:
		kal_fallthrough;
#endif
	case IMG_DL_IDX_PATCH:
		prCmdInfo->ucCID = INIT_CMD_ID_PATCH_START;
		break;
	default:
		prCmdInfo->ucCID = INIT_CMD_ID_DOWNLOAD_CONFIG;
		break;
	}
	NIC_FILL_CMD_TX_HDR(prAdapter,
		prCmdInfo->pucInfoBuffer,
		prCmdInfo->u2InfoBufLen,
		prCmdInfo->ucCID,
		INIT_CMD_PACKET_TYPE_ID,
		&ucCmdSeqNum,
		FALSE,
		(void **)&prInitCmdDownloadConfig,
		TRUE, 0, S2D_INDEX_CMD_H2N, FALSE);

	prInitCmdDownloadConfig->u4Address = u4DestAddr;
	prInitCmdDownloadConfig->u4Length = u4ImgSecSize;
	prInitCmdDownloadConfig->u4DataMode = u4DataMode;

	/* 6. Send FW_Download command */
	while (1) {
		/* 6.1 Acquire TX Resource */
		if (nicTxAcquireResource(prAdapter, ucTC,
				nicTxGetCmdPageCount(prAdapter, prCmdInfo),
					 TRUE) == WLAN_STATUS_RESOURCES) {
			if (nicTxPollingResource(prAdapter,
						 ucTC) != WLAN_STATUS_SUCCESS) {
				u4Status = WLAN_STATUS_FAILURE;
				DBGLOG(INIT, ERROR,
				       "Fail to get TX resource return within timeout\n");
				goto exit;
			}
			continue;
		}
		/* 6.2 Send CMD Info Packet */
		if (nicTxInitCmd(prAdapter, prCmdInfo,
				 prChipInfo->u2TxInitCmdPort) !=
				 WLAN_STATUS_SUCCESS) {
			u4Status = WLAN_STATUS_FAILURE;
			DBGLOG(INIT, ERROR,
			       "Fail to transmit image download command\n");
			goto exit;
		}

		break;
	};

#if CFG_ENABLE_FW_DOWNLOAD_ACK
	/* 7. Wait for INIT_EVENT_ID_CMD_RESULT */
	u4Status = wlanConfigWifiFuncStatus(prAdapter, ucCmdSeqNum);
#endif

exit:
	/* 8. Free CMD Info Packet. */
	cmdBufFreeCmdInfo(prAdapter, prCmdInfo);

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
uint32_t wlanImageSectionDownload(IN struct ADAPTER
				  *prAdapter, IN uint32_t u4ImgSecSize,
				  IN uint8_t *pucImgSecBuf)
{
	struct CMD_INFO *prCmdInfo;
	uint8_t *prBuf;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
	struct mt66xx_chip_info *prChipInfo;

	ASSERT(prAdapter);
	ASSERT(pucImgSecBuf);
	ASSERT(u4ImgSecSize <= CMD_PKT_SIZE_FOR_IMAGE);

	DEBUGFUNC("wlanImageSectionDownload");

	prChipInfo = prAdapter->chip_info;

	if (u4ImgSecSize == 0)
		return WLAN_STATUS_SUCCESS;
	/* 1. Allocate CMD Info Packet and its Buffer. */
	prCmdInfo = cmdBufAllocateCmdInfo(prAdapter,
		sizeof(struct INIT_HIF_TX_HEADER) +
		sizeof(struct INIT_HIF_TX_HEADER_PENDING_FOR_HW_32BYTES) +
		u4ImgSecSize);

	if (!prCmdInfo) {
		DBGLOG(INIT, ERROR, "Allocate CMD_INFO_T ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	/*
	 * For falcon WFDMA, driver doesn't need to append TXD.
	 * For old project/talos,
	 *     TXD length will be included in asicFillInitCmdTxd.
	 */
	prCmdInfo->u2InfoBufLen = (uint16_t) u4ImgSecSize;

	/* 2. Setup common CMD Info Packet */
	prCmdInfo->ucCID = 0;
	NIC_FILL_CMD_TX_HDR(prAdapter,
		prCmdInfo->pucInfoBuffer,
		prCmdInfo->u2InfoBufLen,
		prCmdInfo->ucCID,
		INIT_CMD_PDA_PACKET_TYPE_ID,
		NULL, FALSE, (void **)&prBuf, TRUE, 0, S2D_INDEX_CMD_H2N,
		FALSE);

	/* 3. Setup DOWNLOAD_BUF */
	kalMemCopy(prBuf, pucImgSecBuf, u4ImgSecSize);

	/* 4. Send FW_Download command */
	if (nicTxInitCmd(prAdapter, prCmdInfo,
			 prChipInfo->u2TxFwDlPort) != WLAN_STATUS_SUCCESS) {
		u4Status = WLAN_STATUS_FAILURE;
		DBGLOG(INIT, ERROR,
		       "Fail to transmit image download command\n");
	}
	/* 5. Free CMD Info Packet. */
	cmdBufFreeCmdInfo(prAdapter, prCmdInfo);

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
uint32_t wlanImageQueryStatus(IN struct ADAPTER *prAdapter)
{
	struct mt66xx_chip_info *prChipInfo;
	struct CMD_INFO *prCmdInfo;
	uint8_t *aucBuffer;
	uint32_t u4EventSize;
	uint32_t u4RxPktLength = 0;
	struct INIT_WIFI_EVENT *prInitEvent;
	struct INIT_EVENT_CMD_RESULT *prEventPendingError;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
	uint8_t ucTC, ucCmdSeqNum;

	ASSERT(prAdapter);
	prChipInfo = prAdapter->chip_info;

	DEBUGFUNC("wlanImageQueryStatus");

	/* 1. Allocate CMD Info Packet and it Buffer. */
	prCmdInfo = cmdBufAllocateCmdInfo(prAdapter,
		sizeof(struct INIT_HIF_TX_HEADER) +
		sizeof(struct INIT_HIF_TX_HEADER_PENDING_FOR_HW_32BYTES));

	if (!prCmdInfo) {
		DBGLOG(INIT, ERROR, "Allocate CMD_INFO_T ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	u4EventSize = prChipInfo->rxd_size + prChipInfo->init_event_size +
		sizeof(struct INIT_EVENT_CMD_RESULT);
	aucBuffer = kalMemAlloc(u4EventSize, PHY_MEM_TYPE);
	if (aucBuffer == NULL) {
		DBGLOG(INIT, ERROR, "Alloc CMD buffer failed\n");
		return WLAN_STATUS_FAILURE;
	}

	kalMemZero(prCmdInfo->pucInfoBuffer,
		sizeof(struct INIT_HIF_TX_HEADER) +
		sizeof(struct INIT_HIF_TX_HEADER_PENDING_FOR_HW_32BYTES));
	prCmdInfo->u2InfoBufLen = sizeof(struct INIT_HIF_TX_HEADER) +
		sizeof(struct INIT_HIF_TX_HEADER_PENDING_FOR_HW_32BYTES);

#if (CFG_USE_TC4_RESOURCE_FOR_INIT_CMD == 1)
	/* 2. Always use TC4 */
	ucTC = TC4_INDEX;
#else
	/* 2. Use TC0's resource to send init_cmd
	 * Only TC0 is allowed because SDIO HW always reports
	 * CPU's TXQ_CNT at TXQ0_CNT in CR4 architecutre)
	 */
	ucTC = TC0_INDEX;
#endif
	prCmdInfo->ucCID = INIT_CMD_ID_QUERY_PENDING_ERROR;
	NIC_FILL_CMD_TX_HDR(prAdapter,
		prCmdInfo->pucInfoBuffer,
		prCmdInfo->u2InfoBufLen,
		prCmdInfo->ucCID,
		INIT_CMD_PACKET_TYPE_ID,
		&ucCmdSeqNum, FALSE, NULL, TRUE, 0, S2D_INDEX_CMD_H2N, FALSE);

	/* 5. Send command */
	while (1) {
		/* 5.1 Acquire TX Resource */
		if (nicTxAcquireResource(prAdapter, ucTC,
				nicTxGetCmdPageCount(prAdapter, prCmdInfo),
					 TRUE) == WLAN_STATUS_RESOURCES) {
			if (nicTxPollingResource(prAdapter,
						 ucTC) != WLAN_STATUS_SUCCESS) {
				u4Status = WLAN_STATUS_FAILURE;
				DBGLOG(INIT, ERROR,
				       "Fail to get TX resource return within timeout\n");
				break;
			}
			continue;

		}
		/* 5.2 Send CMD Info Packet */
		if (nicTxInitCmd(prAdapter, prCmdInfo,
				 prChipInfo->u2TxInitCmdPort) !=
				 WLAN_STATUS_SUCCESS) {
			u4Status = WLAN_STATUS_FAILURE;
			DBGLOG(INIT, ERROR,
			       "Fail to transmit image download command\n");
		}

		break;
	};

	/* 6. Wait for INIT_EVENT_ID_PENDING_ERROR */
	do {
		if (kalIsCardRemoved(prAdapter->prGlueInfo) == TRUE
		    || fgIsBusAccessFailed == TRUE) {
			u4Status = WLAN_STATUS_FAILURE;
		} else if (nicRxWaitResponse(prAdapter, 0,
					     aucBuffer, u4EventSize,
					     &u4RxPktLength) !=
			   WLAN_STATUS_SUCCESS) {
			u4Status = WLAN_STATUS_FAILURE;
		} else {
			prInitEvent = (struct INIT_WIFI_EVENT *)
				(aucBuffer + prChipInfo->rxd_size);
			if (prInitEvent == NULL) {
				DBGLOG(INIT, ERROR, "prInitEvent is NULL\n");
				u4Status = WLAN_STATUS_FAILURE;
				break;
			}

			/* EID / SeqNum check */
			if (prInitEvent->ucEID != INIT_EVENT_ID_PENDING_ERROR)
				u4Status = WLAN_STATUS_FAILURE;
			else if (prInitEvent->ucSeqNum != ucCmdSeqNum)
				u4Status = WLAN_STATUS_FAILURE;
			else {
				prEventPendingError =
					(struct INIT_EVENT_CMD_RESULT *)
					prInitEvent->aucBuffer;
				/* 0 for download success */
				if (prEventPendingError->ucStatus != 0)
					u4Status = WLAN_STATUS_FAILURE;
				else
					u4Status = WLAN_STATUS_SUCCESS;
			}
		}
	} while (FALSE);

	/* 7. Free CMD Info Packet. */
	cmdBufFreeCmdInfo(prAdapter, prCmdInfo);

	kalMemFree(aucBuffer, PHY_MEM_TYPE, u4EventSize);

	return u4Status;
}

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

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called to confirm the status of
 *        previously downloaded firmware scatter
 *
 * @param prAdapter      Pointer to the Adapter structure.
 *        ucCmdSeqNum    Sequence number of previous firmware scatter
 *
 * @return WLAN_STATUS_SUCCESS
 *         WLAN_STATUS_FAILURE
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanConfigWifiFuncStatus(IN struct ADAPTER
				  *prAdapter, IN uint8_t ucCmdSeqNum)
{
	struct mt66xx_chip_info *prChipInfo;
	uint8_t *aucBuffer;
	uint32_t u4EventSize;
	struct INIT_WIFI_EVENT *prInitEvent;
	struct INIT_EVENT_CMD_RESULT *prEventCmdResult;
	uint32_t u4RxPktLength;
	uint32_t u4Status;
	uint8_t ucPortIdx = IMG_DL_STATUS_PORT_IDX;

	ASSERT(prAdapter);
	prChipInfo = prAdapter->chip_info;

	u4EventSize = prChipInfo->rxd_size + prChipInfo->init_event_size +
		sizeof(struct INIT_EVENT_CMD_RESULT);
	aucBuffer = kalMemAlloc(u4EventSize, PHY_MEM_TYPE);
	if (aucBuffer == NULL) {
		DBGLOG(INIT, ERROR, "Alloc CMD buffer failed\n");
		return WLAN_STATUS_FAILURE;
	}

	do {
		if (kalIsCardRemoved(prAdapter->prGlueInfo) == TRUE
		    || fgIsBusAccessFailed == TRUE) {
			u4Status = WLAN_STATUS_FAILURE;
		} else if (nicRxWaitResponse(prAdapter, ucPortIdx,
					     aucBuffer, u4EventSize,
					     &u4RxPktLength) !=
			   WLAN_STATUS_SUCCESS) {
			u4Status = WLAN_STATUS_FAILURE;
		} else {
			prInitEvent = (struct INIT_WIFI_EVENT *)
				(aucBuffer + prChipInfo->rxd_size);

			/* EID / SeqNum check */
			if (prInitEvent->ucEID != INIT_EVENT_ID_CMD_RESULT)
				u4Status = WLAN_STATUS_FAILURE;
			else if (prInitEvent->ucSeqNum != ucCmdSeqNum)
				u4Status = WLAN_STATUS_FAILURE;
			else {
				prEventCmdResult =
					(struct INIT_EVENT_CMD_RESULT *)
					prInitEvent->aucBuffer;

				/* 0 for download success */
				if (prEventCmdResult->ucStatus != 0) {
					DBGLOG(INIT, ERROR,
					"Start CMD failed, status[%u]:%s\n",
					prEventCmdResult->ucStatus,
					wlanInitEventStatusCodeToStr(
						prEventCmdResult->ucStatus)
					);
#if CFG_SUPPORT_COMPRESSION_FW_OPTION
					if (prEventCmdResult->ucStatus ==
					    WIFI_FW_DECOMPRESSION_FAILED)
						DBGLOG(INIT, ERROR,
							"Start Decompression CMD failed, status[%u]\n",
						  prEventCmdResult->ucStatus);
#endif
					u4Status = WLAN_STATUS_FAILURE;
				} else {
					u4Status = WLAN_STATUS_SUCCESS;
				}
			}
		}
	} while (FALSE);

	kalMemFree(aucBuffer, PHY_MEM_TYPE, u4EventSize);

	return u4Status;
}

uint32_t wlanConfigWifiFunc(IN struct ADAPTER *prAdapter,
			    IN u_int8_t fgEnable, IN uint32_t u4StartAddress,
			    IN uint8_t ucPDA)
{
	struct CMD_INFO *prCmdInfo;
	struct INIT_CMD_WIFI_START *prInitCmdWifiStart;
	uint8_t ucTC, ucCmdSeqNum;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
	struct mt66xx_chip_info *prChipInfo;

	ASSERT(prAdapter);
	prChipInfo = prAdapter->chip_info;

	DEBUGFUNC("wlanConfigWifiFunc");

	/* 1. Allocate CMD Info Packet and its Buffer. */
	prCmdInfo = cmdBufAllocateCmdInfo(prAdapter,
		sizeof(struct INIT_HIF_TX_HEADER) +
		sizeof(struct INIT_HIF_TX_HEADER_PENDING_FOR_HW_32BYTES) +
		sizeof(struct INIT_CMD_WIFI_START));

	if (!prCmdInfo) {
		DBGLOG(INIT, ERROR, "Allocate CMD_INFO_T ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	kalMemZero(prCmdInfo->pucInfoBuffer,
		sizeof(struct INIT_HIF_TX_HEADER) +
		sizeof(struct INIT_HIF_TX_HEADER_PENDING_FOR_HW_32BYTES) +
		sizeof(struct INIT_CMD_WIFI_START));
	prCmdInfo->u2InfoBufLen = sizeof(struct INIT_HIF_TX_HEADER) +
		sizeof(struct INIT_HIF_TX_HEADER_PENDING_FOR_HW_32BYTES) +
		sizeof(struct INIT_CMD_WIFI_START);

#if (CFG_USE_TC4_RESOURCE_FOR_INIT_CMD == 1)
	/* 2. Always use TC4 (TC4 as CPU) */
	ucTC = TC4_INDEX;
#else
	/* 2. Use TC0's resource to send init_cmd.
	 * Only TC0 is allowed because SDIO HW always reports
	 * CPU's TXQ_CNT at TXQ0_CNT in CR4 architecutre)
	 */
	ucTC = TC0_INDEX;
#endif
	prCmdInfo->ucCID = INIT_CMD_ID_WIFI_START;
	NIC_FILL_CMD_TX_HDR(prAdapter,
		prCmdInfo->pucInfoBuffer,
		prCmdInfo->u2InfoBufLen,
		prCmdInfo->ucCID,
		INIT_CMD_PACKET_TYPE_ID,
		&ucCmdSeqNum,
		FALSE,
		(void **)&prInitCmdWifiStart,
		TRUE, 0, S2D_INDEX_CMD_H2N, FALSE);

	prInitCmdWifiStart->u4Override = 0;
	if (fgEnable)
		prInitCmdWifiStart->u4Override |=
			START_OVERRIDE_START_ADDRESS;

	/* 5G cal until send efuse buffer mode CMD */
#if (CFG_EFUSE_BUFFER_MODE_DELAY_CAL == 1)
	if (prAdapter->fgIsSupportDelayCal == TRUE)
		prInitCmdWifiStart->u4Override |= START_DELAY_CALIBRATION;
#endif

	if (ucPDA == PDA_CR4)
		prInitCmdWifiStart->u4Override |= START_WORKING_PDA_OPTION;

	prInitCmdWifiStart->u4Address = u4StartAddress;

	/* 5. Seend WIFI start command */
	while (1) {
		/* 5.1 Acquire TX Resource */
		if (nicTxAcquireResource(prAdapter, ucTC,
				nicTxGetCmdPageCount(prAdapter, prCmdInfo),
					 TRUE) == WLAN_STATUS_RESOURCES) {
			if (nicTxPollingResource(prAdapter,
						 ucTC) != WLAN_STATUS_SUCCESS) {
				u4Status = WLAN_STATUS_FAILURE;
				DBGLOG(INIT, ERROR,
				       "Fail to get TX resource return within timeout\n");
				goto exit;
			}
			continue;
		}
		/* 5.2 Send CMD Info Packet */
		if (nicTxInitCmd(prAdapter, prCmdInfo,
				 prChipInfo->u2TxInitCmdPort)
					!= WLAN_STATUS_SUCCESS) {
			u4Status = WLAN_STATUS_FAILURE;
			DBGLOG(INIT, ERROR,
			       "Fail to transmit WIFI start command\n");
			goto exit;
		}

		break;
	};

	DBGLOG(INIT, INFO, "FW_START CMD send, waiting for RSP\n");

	u4Status = wlanConfigWifiFuncStatus(prAdapter, ucCmdSeqNum);

	if (u4Status != WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, INFO, "FW_START EVT failed\n");
		GL_DEFAULT_RESET_TRIGGER(prAdapter, RST_CMD_EVT_FAIL);
	} else {
		DBGLOG(INIT, INFO, "FW_START EVT success!!\n");

#if (CFG_SUPPORT_CONNAC3X == 0)
#if defined(_HIF_PCIE)
		if (ucPDA == PDA_CR4 && prChipInfo->is_support_wacpu)
			prChipInfo->rx_event_port = WFDMA1_RX_RING_IDX_1;
#endif /* _HIF_PCIE */
#endif

	}

exit:
	/* 6. Free CMD Info Packet. */
	cmdBufFreeCmdInfo(prAdapter, prCmdInfo);

	return u4Status;
}

#if (CFG_DOWNLOAD_DYN_MEMORY_MAP == 1)
uint32_t wlanRamCodeDynMemMapSendComplete(IN struct ADAPTER *prAdapter,
			IN u_int8_t fgEnable, IN uint32_t u4StartAddress,
			IN uint8_t ucPDA)
{
	struct CMD_INFO *prCmdInfo;
	struct INIT_CMD_WIFI_START *prInitCmdWifiStart;
	uint8_t ucTC, ucCmdSeqNum;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
	struct mt66xx_chip_info *prChipInfo;

	ASSERT(prAdapter);
	prChipInfo = prAdapter->chip_info;

	DEBUGFUNC("wlanConfigWifiFunc");

	/* 1. Allocate CMD Info Packet and its Buffer. */
	prCmdInfo = cmdBufAllocateCmdInfo(prAdapter,
		sizeof(struct INIT_HIF_TX_HEADER) +
		sizeof(struct INIT_HIF_TX_HEADER_PENDING_FOR_HW_32BYTES) +
		sizeof(struct INIT_CMD_WIFI_START));

	if (!prCmdInfo) {
		DBGLOG(INIT, ERROR, "Allocate CMD_INFO_T ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	kalMemZero(prCmdInfo->pucInfoBuffer,
		sizeof(struct INIT_HIF_TX_HEADER) +
		sizeof(struct INIT_HIF_TX_HEADER_PENDING_FOR_HW_32BYTES) +
		sizeof(struct INIT_CMD_WIFI_START));

	prCmdInfo->u2InfoBufLen = sizeof(struct INIT_HIF_TX_HEADER) +
		sizeof(struct INIT_HIF_TX_HEADER_PENDING_FOR_HW_32BYTES) +
		sizeof(struct INIT_CMD_WIFI_START);

#if (CFG_USE_TC4_RESOURCE_FOR_INIT_CMD == 1)
	/* 2. Always use TC4 (TC4 as CPU) */
	ucTC = TC4_INDEX;
#else
	/* 2. Use TC0's resource to send init_cmd.
	 * Only TC0 is allowed because SDIO HW always reports
	 * CPU's TXQ_CNT at TXQ0_CNT in CR4 architecutre)
	 */
	ucTC = TC0_INDEX;
#endif
	prCmdInfo->ucCID = INIT_CMD_ID_DYN_MEM_MAP_FW_FINISH;
	NIC_FILL_CMD_TX_HDR(prAdapter,
		prCmdInfo->pucInfoBuffer,
		prCmdInfo->u2InfoBufLen,
		prCmdInfo->ucCID,
		INIT_CMD_PACKET_TYPE_ID,
		&ucCmdSeqNum,
		FALSE,
		(void **)&prInitCmdWifiStart,
		TRUE, 0, S2D_INDEX_CMD_H2N, FALSE);

	prInitCmdWifiStart->u4Override = 0;
	if (fgEnable)
		prInitCmdWifiStart->u4Override |=
			START_OVERRIDE_START_ADDRESS;

	/* 5G cal until send efuse buffer mode CMD */
#if (CFG_EFUSE_BUFFER_MODE_DELAY_CAL == 1)
	if (prAdapter->fgIsSupportDelayCal == TRUE)
		prInitCmdWifiStart->u4Override |= START_DELAY_CALIBRATION;
#endif

	if (ucPDA == PDA_CR4)
		prInitCmdWifiStart->u4Override |= START_WORKING_PDA_OPTION;

	prInitCmdWifiStart->u4Address = u4StartAddress;

	/* 5. Seend WIFI start command */
	while (1) {
		/* 5.1 Acquire TX Resource */
		if (nicTxAcquireResource(prAdapter, ucTC,
				nicTxGetCmdPageCount(prAdapter, prCmdInfo),
					 TRUE) == WLAN_STATUS_RESOURCES) {
			if (nicTxPollingResource(prAdapter,
						 ucTC) != WLAN_STATUS_SUCCESS) {
				u4Status = WLAN_STATUS_FAILURE;
				DBGLOG(INIT, ERROR,
				       "Fail to get TX resource return within timeout\n");
				goto exit;
			}
			continue;
		}
		/* 5.2 Send CMD Info Packet */
		if (nicTxInitCmd(prAdapter, prCmdInfo,
				 prChipInfo->u2TxInitCmdPort)
					!= WLAN_STATUS_SUCCESS) {
			u4Status = WLAN_STATUS_FAILURE;
			DBGLOG(INIT, ERROR,
			       "Fail to transmit WIFI start command\n");
			goto exit;
		}

		break;
	};

	DBGLOG(INIT, INFO, "FW_START CMD send, waiting for RSP\n");

	if (ucPDA == PDA_CR4 && prChipInfo->is_support_wacpu) {
		prChipInfo->rx_event_port = WFDMA1_RX_RING_IDX_1;
		/* workaround for harrier powerOnCal too long issue
		* skip FW start event, fw ready bit check can cover this.
		*/
		return WLAN_STATUS_SUCCESS;
	}

	u4Status = wlanConfigWifiFuncStatus(prAdapter, ucCmdSeqNum);

	if (u4Status != WLAN_STATUS_SUCCESS)
		DBGLOG(INIT, INFO, "FW_START EVT failed\n");
	else
		DBGLOG(INIT, INFO, "FW_START EVT success!!\n");

exit:
	/* 6. Free CMD Info Packet. */
	cmdBufFreeCmdInfo(prAdapter, prCmdInfo);

	return u4Status;
}
#endif

#if CFG_SUPPORT_COMPRESSION_FW_OPTION
uint32_t
wlanCompressedFWConfigWifiFunc(IN struct ADAPTER *prAdapter,
	IN u_int8_t fgEnable,
	IN uint32_t u4StartAddress, IN uint8_t ucPDA,
	IN struct INIT_CMD_WIFI_DECOMPRESSION_START *prFwImageInFo)
{
	struct CMD_INFO *prCmdInfo;
	struct INIT_CMD_WIFI_DECOMPRESSION_START *prInitCmdWifiStart;
	uint8_t ucTC, ucCmdSeqNum;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
	struct mt66xx_chip_info *prChipInfo;

	ASSERT(prAdapter);
	prChipInfo = prAdapter->chip_info;

	DEBUGFUNC("wlanConfigWifiFunc");
	/* 1. Allocate CMD Info Packet and its Buffer. */
	prCmdInfo = cmdBufAllocateCmdInfo(prAdapter,
		sizeof(struct INIT_HIF_TX_HEADER) +
		sizeof(struct INIT_HIF_TX_HEADER_PENDING_FOR_HW_32BYTES) +
		sizeof(struct INIT_CMD_WIFI_DECOMPRESSION_START));

	if (!prCmdInfo) {
		DBGLOG(INIT, ERROR, "Allocate CMD_INFO_T ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	kalMemZero(prCmdInfo->pucInfoBuffer,
		sizeof(struct INIT_HIF_TX_HEADER) +
		sizeof(struct INIT_HIF_TX_HEADER_PENDING_FOR_HW_32BYTES) +
		sizeof(struct INIT_CMD_WIFI_DECOMPRESSION_START));
	prCmdInfo->u2InfoBufLen =
		sizeof(struct INIT_HIF_TX_HEADER) +
		sizeof(struct INIT_HIF_TX_HEADER_PENDING_FOR_HW_32BYTES) +
		sizeof(struct INIT_CMD_WIFI_DECOMPRESSION_START);

	/* 2. Always use TC0 */
	ucTC = TC0_INDEX;

	prCmdInfo->ucCID = INIT_CMD_ID_DECOMPRESSED_WIFI_START;
	NIC_FILL_CMD_TX_HDR(prAdapter,
		prCmdInfo->pucInfoBuffer,
		prCmdInfo->u2InfoBufLen,
		prCmdInfo->ucCID,
		INIT_CMD_PACKET_TYPE_ID,
		&ucCmdSeqNum,
		FALSE,
		&prInitCmdWifiStart, TRUE, 0, S2D_INDEX_CMD_H2N, FALSE);

	prInitCmdWifiStart->u4Override = 0;
	if (fgEnable)
		prInitCmdWifiStart->u4Override |=
			START_OVERRIDE_START_ADDRESS;

	/* 5G cal until send efuse buffer mode CMD */
#if (CFG_EFUSE_BUFFER_MODE_DELAY_CAL == 1)
	if (prAdapter->fgIsSupportDelayCal == TRUE)
		prInitCmdWifiStart->u4Override |= START_DELAY_CALIBRATION;
#endif
	if (ucPDA == PDA_CR4)
		prInitCmdWifiStart->u4Override |= START_WORKING_PDA_OPTION;

#if CFG_COMPRESSION_DEBUG
	prInitCmdWifiStart->u4Override |= START_CRC_CHECK;
#endif
#if CFG_DECOMPRESSION_TMP_ADDRESS
	prInitCmdWifiStart->u4Override |=
		CHANGE_DECOMPRESSION_TMP_ADDRESS;
	prInitCmdWifiStart->u4DecompressTmpAddress = 0xE6000;
#endif
	prInitCmdWifiStart->u4Address = u4StartAddress;
	prInitCmdWifiStart->u4Region1Address =
		prFwImageInFo->u4Region1Address;
	prInitCmdWifiStart->u4Region1CRC =
		prFwImageInFo->u4Region1CRC;
	prInitCmdWifiStart->u4BlockSize =
		prFwImageInFo->u4BlockSize;
	prInitCmdWifiStart->u4Region1length =
		prFwImageInFo->u4Region1length;
	prInitCmdWifiStart->u4Region2Address =
		prFwImageInFo->u4Region2Address;
	prInitCmdWifiStart->u4Region2CRC =
		prFwImageInFo->u4Region2CRC;
	prInitCmdWifiStart->u4Region2length =
		prFwImageInFo->u4Region2length;

	while (1) {
		/* 5.1 Acquire TX Resource */
		if (nicTxAcquireResource(prAdapter, ucTC,
				nicTxGetCmdPageCount(prAdapter, prCmdInfo),
					 TRUE) == WLAN_STATUS_RESOURCES) {
			if (nicTxPollingResource(prAdapter,
						 ucTC) != WLAN_STATUS_SUCCESS) {
				u4Status = WLAN_STATUS_FAILURE;
				DBGLOG(INIT, ERROR,
				       "Fail to get TX resource return within timeout\n");
				break;
			}
			continue;

		}
		/* 5.2 Send CMD Info Packet */
		if (nicTxInitCmd(prAdapter, prCmdInfo,
				 prChipInfo->u2TxInitCmdPort)
				 != WLAN_STATUS_SUCCESS) {
			u4Status = WLAN_STATUS_FAILURE;
			DBGLOG(INIT, ERROR,
			       "Fail to transmit WIFI start command\n");
		}

		break;
	};

	DBGLOG(INIT, INFO, "FW_START CMD send, waiting for RSP\n");

	u4Status = wlanConfigWifiFuncStatus(prAdapter, ucCmdSeqNum);

	if (u4Status != WLAN_STATUS_SUCCESS)
		DBGLOG(INIT, INFO, "FW_START EVT failed\n");
	else
		DBGLOG(INIT, INFO, "FW_START EVT success!!\n");


	/* 6. Free CMD Info Packet. */
	cmdBufFreeCmdInfo(prAdapter, prCmdInfo);

	return u4Status;
}
#endif
#if 0
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
#endif

uint32_t wlanGetHarvardTailerInfo(IN struct ADAPTER
	*prAdapter, IN void *prFwBuffer, IN uint32_t u4FwSize,
	IN uint32_t ucTotSecNum, IN enum ENUM_IMG_DL_IDX_T eDlIdx)
{
	struct TAILER_FORMAT_T *prTailers;
	uint8_t *pucStartPtr;
	uint32_t u4SecIdx;

	pucStartPtr = prFwBuffer + u4FwSize - sizeof(
			      struct TAILER_FORMAT_T) * ucTotSecNum;
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

uint32_t wlanGetConnacTailerInfo(IN struct ADAPTER
	*prAdapter, IN void *prFwBuffer,
	IN uint32_t u4FwSize, IN enum ENUM_IMG_DL_IDX_T eDlIdx)
{
	struct WIFI_VER_INFO *prVerInfo = &prAdapter->rVerInfo;
	struct TAILER_COMMON_FORMAT_T *prComTailer;
	struct TAILER_REGION_FORMAT_T *prRegTailer;
	uint8_t *pucImgPtr;
	uint8_t *pucTailertPtr;
	uint8_t *pucStartPtr;
	uint32_t u4SecIdx;

	pucImgPtr = prFwBuffer;
	pucStartPtr = prFwBuffer + u4FwSize -
		sizeof(struct TAILER_COMMON_FORMAT_T);
	prComTailer = (struct TAILER_COMMON_FORMAT_T *) pucStartPtr;
	kalMemCopy(&prVerInfo->rCommonTailer, prComTailer,
		   sizeof(struct TAILER_COMMON_FORMAT_T));

	/* Dump image information */
	DBGLOG(INIT, INFO,
	       "%s INFO: chip_info[%u:E%u] region_num[%d]\n",
	       (eDlIdx == IMG_DL_IDX_N9_FW) ? "N9" : "CR4",
	       prComTailer->ucChipInfo,
	       prComTailer->ucEcoCode + 1, prComTailer->ucRegionNum);

	DBGLOG(INIT, INFO, "date[%s] version[%s]\n",
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
		fwDlGetReleaseInfoSection(prAdapter, pucImgPtr);

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

uint32_t wlanDetectRamEntry(IN struct WIFI_VER_INFO
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

uint32_t wlanHarvardFormatDownload(IN struct ADAPTER
				   *prAdapter, IN enum ENUM_IMG_DL_IDX_T eDlIdx)
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
			prFwBuffer, u4FwSize, ucTotSecNum, eDlIdx);
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

uint32_t wlanConnacFormatDownload(IN struct ADAPTER
				  *prAdapter, IN enum ENUM_IMG_DL_IDX_T eDlIdx)
{
	void *prFwBuffer = NULL;
	uint32_t u4FwSize = 0;
	uint32_t ram_entry = 0;
	uint32_t rDlStatus = 0;
	uint32_t rCfgStatus = 0;
	uint8_t ucRegionNum;
	uint8_t ucPDA;

	kalFirmwareImageMapping(prAdapter->prGlueInfo, &prFwBuffer,
				&u4FwSize, eDlIdx);
	if (prFwBuffer == NULL) {
		DBGLOG(INIT, WARN, "FW[%u] load error!\n", eDlIdx);
		return WLAN_STATUS_FAILURE;
	}

	if (wlanGetConnacTailerInfo(prAdapter, prFwBuffer, u4FwSize,
				    eDlIdx) != WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, WARN, "Get tailer info error!\n");
		return WLAN_STATUS_FAILURE;
	}

	ucRegionNum = prAdapter->rVerInfo.rCommonTailer.ucRegionNum;
	ucPDA = (eDlIdx == IMG_DL_IDX_N9_FW) ? PDA_N9 : PDA_CR4;

	rDlStatus = wlanImageSectionDownloadStage(prAdapter,
			prFwBuffer, u4FwSize, ucRegionNum, eDlIdx);

	ram_entry = wlanDetectRamEntry(&prAdapter->rVerInfo);

/* To support dynamic memory map for WiFi RAM code download::Begin */
#if (CFG_DOWNLOAD_DYN_MEMORY_MAP == 1)
	rCfgStatus = wlanRamCodeDynMemMapSendComplete(prAdapter,
					(ram_entry == 0) ? FALSE : TRUE,
					ram_entry, ucPDA);
#else
	rCfgStatus = wlanConfigWifiFunc(prAdapter,
					(ram_entry == 0) ? FALSE : TRUE,
					ram_entry, ucPDA);
#endif
/* To support dynamic memory map for WiFi RAM code download::End */

	kalFirmwareImageUnmapping(prAdapter->prGlueInfo, NULL,
				  prFwBuffer);

	if ((rDlStatus != WLAN_STATUS_SUCCESS)
	    || (rCfgStatus != WLAN_STATUS_SUCCESS))
		return WLAN_STATUS_FAILURE;

	return WLAN_STATUS_SUCCESS;
}

uint32_t wlanDownloadFW(IN struct ADAPTER *prAdapter)
{
	uint32_t rStatus = 0;
	struct mt66xx_chip_info *prChipInfo;
	struct FWDL_OPS_T *prFwDlOps;

	if (!prAdapter)
		return WLAN_STATUS_FAILURE;

	prChipInfo = prAdapter->chip_info;
	prFwDlOps = prChipInfo->fw_dl_ops;

	HAL_ENABLE_FWDL(prAdapter, TRUE);

	if (prFwDlOps->downloadPatch)
		prFwDlOps->downloadPatch(prAdapter);

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

	DBGLOG(INIT, INFO, "FW download Start\n");

	if (prFwDlOps->downloadFirmware) {
		rStatus = prFwDlOps->downloadFirmware(prAdapter,
						      IMG_DL_IDX_N9_FW);
		if ((prChipInfo->is_support_cr4 || prChipInfo->is_support_wacpu)
		    && rStatus == WLAN_STATUS_SUCCESS)
			rStatus = prFwDlOps->downloadFirmware(prAdapter,
						IMG_DL_IDX_CR4_FW);
	} else
		DBGLOG(INIT, WARN, "Without downlaod firmware Ops\n");

	DBGLOG(INIT, INFO, "FW download End\n");

	HAL_ENABLE_FWDL(prAdapter, FALSE);

	return rStatus;
}

uint32_t wlanDownloadPatch(IN struct ADAPTER *prAdapter)
{
	uint32_t u4FwSize = 0;
	void *prFwBuffer = NULL;
	uint32_t u4Status;
#if CFG_SUPPORT_COMPRESSION_FW_OPTION
	uint8_t ucIsCompressed;
#endif
	if (!prAdapter)
		return WLAN_STATUS_FAILURE;


	DBGLOG(INIT, INFO, "Patch download start\n");

	prAdapter->rVerInfo.fgPatchIsDlByDrv = FALSE;

	kalFirmwareImageMapping(prAdapter->prGlueInfo, &prFwBuffer,
				&u4FwSize, IMG_DL_IDX_PATCH);
	if (prFwBuffer == NULL) {
		DBGLOG(INIT, WARN, "FW[%u] load error!\n",
		       IMG_DL_IDX_PATCH);
		return WLAN_STATUS_FAILURE;
	}

#if (CFG_ROM_PATCH_NO_SEM_CTRL == 0)
#pragma message("ROM code supports SEM-CTRL for ROM patch download")
	if (wlanPatchIsDownloaded(prAdapter)) {
		kalFirmwareImageUnmapping(prAdapter->prGlueInfo, NULL,
					  prFwBuffer);
		DBGLOG(INIT, INFO, "No need to download patch\n");
		return WLAN_STATUS_SUCCESS;
	}
#else
#pragma message("ROM code supports no SEM-CTRL for ROM patch download")
#endif

	/* Patch DL */
	do {
#if CFG_SUPPORT_COMPRESSION_FW_OPTION
		u4Status = wlanCompressedImageSectionDownloadStage(
			prAdapter, prFwBuffer, u4FwSize, 1,
			IMG_DL_IDX_PATCH, &ucIsCompressed, NULL);
#else
		u4Status = wlanImageSectionDownloadStage(
			prAdapter, prFwBuffer, u4FwSize, 1, IMG_DL_IDX_PATCH);
#endif

/* Dynamic memory map::Begin */
#if (CFG_DOWNLOAD_DYN_MEMORY_MAP == 1)
		if (u4Status == WLAN_STATUS_SUCCESS)
			wlanPatchDynMemMapSendComplete(prAdapter);
		else if (u4Status == WLAN_STATUS_NOT_ACCEPTED)
			u4Status = WLAN_STATUS_SUCCESS; /* already download*/
#else
#if CFG_SUPPORT_WIFI_DL_BT_PATCH || CFG_SUPPORT_WIFI_DL_ZB_PATCH
		/*
		 * PATCH_FNSH_TYPE_WF_MD:
		 * Download flow expects that the BT patch download is coming.
		 * The cal won't start after WF patch download finish.
		 */
		wlanPatchSendComplete(prAdapter, PATCH_FNSH_TYPE_WF_MD);
#else
		wlanPatchSendComplete(prAdapter);
#endif
#endif
/* Dynamic memory map::End */

		kalFirmwareImageUnmapping(prAdapter->prGlueInfo, NULL,
					  prFwBuffer);

		prAdapter->rVerInfo.fgPatchIsDlByDrv = TRUE;
	} while (0);

	DBGLOG(INIT, INFO, "Patch download end[%d].\n", u4Status);

	return u4Status;
}

uint32_t wlanGetPatchInfo(IN struct ADAPTER *prAdapter)
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
	uint8_t aucBuf[32] = {0}, aucDate[32] = {0};

	prFwDlOps = prAdapter->chip_info->fw_dl_ops;

	kalMemZero(aucBuf, sizeof(aucBuf));
	kalStrnCpy(aucBuf, prVerInfo->aucFwBranchInfo,
			sizeof(aucBuf));
	aucBuf[sizeof(prVerInfo->aucFwBranchInfo)] = '\0';

	kalMemZero(aucDate, sizeof(aucDate));
	kalStrnCpy(aucDate, prVerInfo->aucFwDateCode,
			sizeof(aucDate));
	aucDate[sizeof(prVerInfo->aucFwDateCode)] = '\0';

	u4Offset += snprintf(pcBuf + u4Offset,
			i4TotalLen - u4Offset,
			"\nN9 FW version %s-%u.%u.%u[DEC] (%s)\n",
			aucBuf,
			(uint32_t)(prVerInfo->u2FwOwnVersion >> 8),
			(uint32_t)(prVerInfo->u2FwOwnVersion & BITS(0, 7)),
			prVerInfo->ucFwBuildNumber, aucDate);

	if (prFwDlOps->getFwDlInfo)
		u4Offset += prFwDlOps->getFwDlInfo(prAdapter,
						   pcBuf + u4Offset,
						   i4TotalLen - u4Offset);

	if (!prVerInfo->fgPatchIsDlByDrv) {
		u4Offset += snprintf(pcBuf + u4Offset,
				     i4TotalLen - u4Offset,
				     "MCU patch is not downloaded by wlan driver, read patch info\n");
		wlanGetPatchInfo(prAdapter);
	}

	kalMemZero(aucBuf, sizeof(aucBuf));
	kalMemZero(aucDate, sizeof(aucDate));
	kalStrnCpy(aucBuf, prVerInfo->rPatchHeader.aucPlatform,
			sizeof(aucBuf));
	aucBuf[sizeof(prVerInfo->rPatchHeader.aucPlatform)] = '\0';
	kalStrnCpy(aucDate, prVerInfo->rPatchHeader.aucBuildDate,
			sizeof(aucDate));
	aucDate[sizeof(prVerInfo->rPatchHeader.aucBuildDate)] = '\0';
	u4Offset += snprintf(pcBuf + u4Offset,
			     i4TotalLen - u4Offset,
			     "Patch platform %s version 0x%04X %s\n",
			     aucBuf, prVerInfo->rPatchHeader.u4PatchVersion,
			     aucDate);

	u4Offset += snprintf(pcBuf + u4Offset, i4TotalLen - u4Offset,
			"Drv version %u.%u[DEC]\n",
			(uint32_t)(prVerInfo->u2FwPeerVersion >> 8),
			(uint32_t)(prVerInfo->u2FwPeerVersion & BITS(0, 7)));
	return u4Offset;
}

void fwDlGetReleaseInfoSection(struct ADAPTER *prAdapter, uint8_t *pucStartPtr)
{
	struct HEADER_RELEASE_INFO *prFirstInfo;
	struct HEADER_RELEASE_INFO *prRelInfo;
	uint8_t *pucCurPtr = pucStartPtr + RELEASE_INFO_SEPARATOR_LEN;
	uint16_t u2Len = 0, u2Offset = 0;
	uint8_t ucManifestExist = 0;

	prFirstInfo = (struct HEADER_RELEASE_INFO *)pucCurPtr;
	DBGLOG(INIT, INFO, "Release info tag[%u] len[%u]\n",
	       prFirstInfo->ucTag, prFirstInfo->u2Len);

	pucCurPtr += sizeof(struct HEADER_RELEASE_INFO);
	while (u2Offset < prFirstInfo->u2Len) {
		prRelInfo = (struct HEADER_RELEASE_INFO *)pucCurPtr;
		DBGLOG(INIT, INFO, "Release info tag[%u] len[%u] padding[%u]\n",
		       prRelInfo->ucTag, prRelInfo->u2Len,
		       prRelInfo->ucPaddingLen);

		pucCurPtr += sizeof(struct HEADER_RELEASE_INFO);
		switch (prRelInfo->ucTag) {
		case 0x01:
			fwDlGetReleaseManifest(prAdapter, prRelInfo, pucCurPtr);
			ucManifestExist = 1;
			break;
		case 0x02:
			if (!ucManifestExist)
				fwDlGetReleaseManifest(prAdapter,
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

void fwDlGetReleaseManifest(struct ADAPTER *prAdapter,
			    struct HEADER_RELEASE_INFO *prRelInfo,
			    uint8_t *pucStartPtr)
{
	kalMemZero(&prAdapter->rVerInfo.aucReleaseManifest,
		   sizeof(prAdapter->rVerInfo.aucReleaseManifest));
	kalMemCopy(&prAdapter->rVerInfo.aucReleaseManifest,
		   pucStartPtr, prRelInfo->u2Len);
	DBGLOG(INIT, INFO, "Release manifest: %s\n",
	       prAdapter->rVerInfo.aucReleaseManifest);
}

#endif  /* CFG_ENABLE_FW_DOWNLOAD */
