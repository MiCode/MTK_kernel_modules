// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   uni_fw_dl.c
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
static const char * const apucGlobalSubsysStr[GLOBAL_SUBSYS_NUM] = {
	[GLOBAL_SUBSYS_BT] = "BT",
	[GLOBAL_SUBSYS_WF] = "WF",
	[GLOBAL_SUBSYS_GPS] = "GPS",
	[GLOBAL_SUBSYS_ZB] = "ZB",
};

static const char * const apucSectionTypeStr[SUBSYS_SEC_TYPE_NUM] = {
	[SUBSYS_SEC_TYPE_RELEASE_INFO] = "RELEASE_INFO",
	[SUBSYS_SEC_TYPE_BINARY_INFO] = "BINARY_INFO",
	[SUBSYS_SEC_TYPE_IDX_LOG] = "IDX_LOG",
	[SUBSYS_SEC_TYPE_TX_PWR] = "TX_PWR",
	[SUBSYS_SEC_TYPE_SIGNATURE] = "SIGNATURE",
};

static const char * const apucSectionSubsysStr[SUBSYS_SEC_TYPE_SUBSYS_NUM] = {
	[SUBSYS_SEC_TYPE_SUBSYS_RESERVED] = "RESERVED",
	[SUBSYS_SEC_TYPE_SUBSYS_CBMCU] = "CBMCU",
	[SUBSYS_SEC_TYPE_SUBSYS_FM] = "FM",
	[SUBSYS_SEC_TYPE_SUBSYS_BT] = "BT",
	[SUBSYS_SEC_TYPE_SUBSYS_WF_WM] = "WF_WM",
	[SUBSYS_SEC_TYPE_SUBSYS_GPS] = "GPS",
	[SUBSYS_SEC_TYPE_SUBSYS_ZB] = "ZB",
	[SUBSYS_SEC_TYPE_SUBSYS_WF_DSP] = "WF_DSP",
	[SUBSYS_SEC_TYPE_SUBSYS_WF_WA] = "WF_WA",
};

static const char * const apucSectionSegmentStr[SUBSYS_SEC_TYPE_SEGMENT_NUM] = {
	[SUBSYS_SEC_TYPE_SEGMENT_GENERAL] = "GENERAL",
	[SUBSYS_SEC_TYPE_SEGMENT_REBB] = "REBB",
	[SUBSYS_SEC_TYPE_SEGMENT_MOBILE] = "MOBILE",
	[SUBSYS_SEC_TYPE_SEGMENT_CE] = "CE",
};

#define GLOBAL_SUBSYS_STR(_subsys) \
	apucGlobalSubsysStr[_subsys]

#define SECTION_TYPE_STR(_type) \
	apucSectionTypeStr[_type]

#define SECTION_SUBSYS_STR(_subsys) \
	apucSectionSubsysStr[_subsys]

#define SECTION_SEGMENT_STR(_segment) \
	apucSectionSegmentStr[_segment]

#if (CFG_MTK_FPGA_PLATFORM != 0)
#define UNI_FWDL_DEFAULT_TIMEOUT			(1000 * 20) /* ms */
#else
#define UNI_FWDL_DEFAULT_TIMEOUT			(1000 * 4) /* ms */
#endif /* CFG_MTK_FPGA_PLATFORM */

#define UNI_FWDL_WAIT_DURATION				(10 * 1) /* ms */

#define UNI_FWDL_WAKEUP_CONDITION \
	(BIT(WAKEUP_SOURCE_HALT) | \
	 BIT(WAKEUP_SOURCE_DL_DONE) | \
	 BIT(WAKEUP_SOURCE_DL_FAIL) | \
	 BIT(WAKEUP_SOURCE_REQ_DL) | \
	 BIT(WAKEUP_SOURCE_BROM_PATCH_DONE))

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
static uint32_t uniFwdlOpenFwBinary(struct GLUE_INFO *prGlueInfo,
				    uint8_t **pprBinaryContent,
				    uint32_t *pu4BinarySize)
{
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct FWDL_OPS_T *prFwdlOps;
	uint8_t *prFwBuffer = NULL;
	uint8_t *apucName[CFG_FW_FILE_NAME_TOTAL + 1];
	uint8_t aucNameBody[CFG_FW_FILE_NAME_TOTAL][CFG_FW_NAME_MAX_LEN];
	uint32_t u4FwSize = 0, u4Status = WLAN_STATUS_FAILURE;
	uint8_t ucIdx, ucNum = 0;

	glGetChipInfo((void **)&prChipInfo);
	if (!prChipInfo) {
		DBGLOG(UNI_FWDL, ERROR, "NULL prChipInfo\n");
		goto exit;
	}

	prFwdlOps = prChipInfo->fw_dl_ops;
	if (!prFwdlOps->constructFirmwarePrio) {
		DBGLOG(UNI_FWDL, ERROR, "Construct FW binary failed\n");
		goto exit;
	}
	kalMemZero(apucName, sizeof(apucName));
	kalMemZero(aucNameBody, sizeof(aucNameBody));
	for (ucIdx = 0; ucIdx < CFG_FW_FILE_NAME_TOTAL; ucIdx++)
		apucName[ucIdx] = (uint8_t *)(aucNameBody + ucIdx);
	prFwdlOps->constructFirmwarePrio(prGlueInfo, NULL,
					 (uint8_t **)apucName,
					 &ucNum,
					 CFG_FW_FILE_NAME_TOTAL);

	if (kalFirmwareOpen(prGlueInfo, (uint8_t **)apucName) !=
	    WLAN_STATUS_SUCCESS)
		goto exit;

	kalFirmwareSize(prGlueInfo, &u4FwSize);
	if (u4FwSize == 0) {
		DBGLOG(UNI_FWDL, ERROR,
			"zero len binary\n");
		goto exit;
	}

	prFwBuffer = kalMemAlloc(u4FwSize, VIR_MEM_TYPE);
	if (!prFwBuffer) {
		DBGLOG(UNI_FWDL, ERROR,
			"malloc for fw binary %u failed\n",
			u4FwSize);
		goto exit;
	}

	if (kalFirmwareLoad(prGlueInfo, prFwBuffer, 0, &u4FwSize) !=
	    WLAN_STATUS_SUCCESS)
		goto free_mem;

	*pprBinaryContent = prFwBuffer;
	*pu4BinarySize = u4FwSize;

	u4Status = WLAN_STATUS_SUCCESS;

	goto exit;

free_mem:
	if (prFwBuffer && u4FwSize)
		kalMemFree(prFwBuffer, VIR_MEM_TYPE, u4FwSize);
	kalFirmwareClose(prGlueInfo);

exit:
	return u4Status;
}

static void uniFwdlCloseFwBinary(struct GLUE_INFO *prGlueInfo,
				 uint8_t *prFwBuffer,
				 uint32_t u4FwSize)
{
	if (prFwBuffer && u4FwSize)
		kalMemFree(prFwBuffer, VIR_MEM_TYPE, u4FwSize);

	kalFirmwareClose(prGlueInfo);
}

static uint32_t
uniFwdlParseGlobalSubsysType(uint32_t u4GlobalSubsys,
			     enum GLOBAL_SUBSYS *peGlobalSubsysType)
{
	switch ((u4GlobalSubsys & 0x000000FF)) {
	case 0x03:
		*peGlobalSubsysType = GLOBAL_SUBSYS_BT;
		return WLAN_STATUS_SUCCESS;
	case 0x04:
		*peGlobalSubsysType = GLOBAL_SUBSYS_WF;
		return WLAN_STATUS_SUCCESS;
	case 0x05:
		*peGlobalSubsysType = GLOBAL_SUBSYS_GPS;
		return WLAN_STATUS_SUCCESS;
	case 0x06:
		*peGlobalSubsysType = GLOBAL_SUBSYS_ZB;
		return WLAN_STATUS_SUCCESS;
	default:
		DBGLOG(UNI_FWDL, ERROR,
			"Unsupported global subsys=%u\n",
			u4GlobalSubsys);
		return WLAN_STATUS_FAILURE;
	}
}

static uint32_t uniFwdlParseSecType(uint32_t u4Type,
				    enum SUBSYS_SEC_TYPE *peType)
{
	switch ((u4Type & 0x0000FFFF)) {
	case 0x0001:
		*peType = SUBSYS_SEC_TYPE_RELEASE_INFO;
		return WLAN_STATUS_SUCCESS;
	case 0x0002:
		*peType = SUBSYS_SEC_TYPE_BINARY_INFO;
		return WLAN_STATUS_SUCCESS;
	case 0x0003:
		*peType = SUBSYS_SEC_TYPE_IDX_LOG;
		return WLAN_STATUS_SUCCESS;
	case 0x0004:
		*peType = SUBSYS_SEC_TYPE_TX_PWR;
		return WLAN_STATUS_SUCCESS;
	case 0x0005:
		*peType = SUBSYS_SEC_TYPE_SIGNATURE;
		return WLAN_STATUS_SUCCESS;
	default:
		DBGLOG(UNI_FWDL, ERROR,
			"Unsupported section type=%u\n",
			(u4Type & 0x0000FFFF));
		return WLAN_STATUS_FAILURE;
	}
}

static uint32_t uniFwdlParseSecSubsys(uint32_t u4Type,
				      enum SUBSYS_SEC_TYPE_SUBSYS *peSubsys)
{
	switch ((u4Type & 0x00FF0000) >> 16) {
	case 0x00:
		*peSubsys = SUBSYS_SEC_TYPE_SUBSYS_RESERVED;
		return WLAN_STATUS_SUCCESS;
	case 0x01:
		*peSubsys = SUBSYS_SEC_TYPE_SUBSYS_CBMCU;
		return WLAN_STATUS_SUCCESS;
	case 0x02:
		*peSubsys = SUBSYS_SEC_TYPE_SUBSYS_FM;
		return WLAN_STATUS_SUCCESS;
	case 0x03:
		*peSubsys = SUBSYS_SEC_TYPE_SUBSYS_BT;
		return WLAN_STATUS_SUCCESS;
	case 0x04:
		*peSubsys = SUBSYS_SEC_TYPE_SUBSYS_WF_WM;
		return WLAN_STATUS_SUCCESS;
	case 0x05:
		*peSubsys = SUBSYS_SEC_TYPE_SUBSYS_GPS;
		return WLAN_STATUS_SUCCESS;
	case 0x06:
		*peSubsys = SUBSYS_SEC_TYPE_SUBSYS_ZB;
		return WLAN_STATUS_SUCCESS;
	case 0x07:
		*peSubsys = SUBSYS_SEC_TYPE_SUBSYS_WF_DSP;
		return WLAN_STATUS_SUCCESS;
	case 0x08:
		*peSubsys = SUBSYS_SEC_TYPE_SUBSYS_WF_WA;
		return WLAN_STATUS_SUCCESS;
	default:
		DBGLOG(UNI_FWDL, ERROR,
			"Unsupported section type subsys=%u\n",
			(u4Type & 0x00FF0000) >> 16);
		return WLAN_STATUS_FAILURE;
	}
}

static uint32_t uniFwdlParseSecSegment(uint32_t u4Type,
				       enum SUBSYS_SEC_TYPE_SEGMENT *peSegment)
{
	switch ((u4Type & 0xFF000000) >> 24) {
	case 0x00:
		*peSegment = SUBSYS_SEC_TYPE_SEGMENT_GENERAL;
		return WLAN_STATUS_SUCCESS;
	case 0x01:
		*peSegment = SUBSYS_SEC_TYPE_SEGMENT_REBB;
		return WLAN_STATUS_SUCCESS;
	case 0x02:
		*peSegment = SUBSYS_SEC_TYPE_SEGMENT_MOBILE;
		return WLAN_STATUS_SUCCESS;
	case 0x03:
		*peSegment = SUBSYS_SEC_TYPE_SEGMENT_CE;
		return WLAN_STATUS_SUCCESS;
	default:
		DBGLOG(UNI_FWDL, ERROR,
			"Unsupported section type segment=%u\n",
			(u4Type & 0xFF000000) >> 24);
		return WLAN_STATUS_FAILURE;
	}
}

static uint32_t uniFwdlParseFwBinaryInfo(uint8_t *prFwBuffer,
					 uint32_t u4FwSize,
					 struct UNI_FWDL_BINARY_INFO *prInfo)
{
	struct MULTI_HEADER_V2_RAW_FORMAT *prRaw;
	struct UNI_FWDL_IMAGE_HEADER *prHeader = &prInfo->rHeader;
	struct UNI_FWDL_GLOBAL_DESCRIPTOR *prDescriptor = &prInfo->rDescriptor;
	uint8_t *prPos, *prEnd;
	uint32_t u4Status = WLAN_STATUS_FAILURE;
	uint8_t ucIdx = 0;

	if (!prFwBuffer || u4FwSize == 0 || !prInfo) {
		DBGLOG(UNI_FWDL, ERROR,
			"prFwBuffer=0x%p u4FwSize=%u prInfo=0x%p\n",
			prFwBuffer, u4FwSize, prInfo);
		goto exit;
	}

	prPos = prFwBuffer;
	prEnd = prFwBuffer + u4FwSize;

	prRaw = (struct MULTI_HEADER_V2_RAW_FORMAT *)prFwBuffer;
	prPos += sizeof(*prRaw);

	if (prPos > prEnd) {
		DBGLOG(UNI_FWDL, ERROR,
			"size not enough for MULTI_HEADER_V2_RAW_FORMAT\n");
		goto dump;
	}

	kalMemZero(prInfo, sizeof(*prInfo));

	prInfo->prFwBuffer = prFwBuffer;
	prInfo->u4FwSize = u4FwSize;
	prInfo->prHeader = prFwBuffer;
	prInfo->u4HeaderSz =
		OFFSET_OF(struct MULTI_HEADER_V2_RAW_FORMAT,
			  aucSections[0]) +
		sizeof(struct MULTI_HEADER_V2_SEC_RAW_FORMAT) *
			prRaw->u4SecNum;

	/* <1> Image header */
	kalMemCopy(prHeader->aucPackTime,
		   prRaw->aucPackTime,
		   sizeof(prRaw->aucPackTime));
	kalMemCopy(prHeader->aucChipIdNEcoVer,
		   prRaw->aucChipIdNEcoVer,
		   sizeof(prRaw->aucChipIdNEcoVer));
	prHeader->u4PatchVer = prRaw->u4PatchVer;

	/* <2> Global descriptor */
	if (uniFwdlParseGlobalSubsysType(prRaw->u4GlobalSubsys,
					 &prDescriptor->eGlobalSubsysType) !=
	    WLAN_STATUS_SUCCESS)
		goto dump;

	if (prRaw->u4SecNum >= UNI_FWDL_MAX_SECTION_NUM) {
		DBGLOG(UNI_FWDL, ERROR, "Exceed max section num=%u\n",
			prRaw->u4SecNum);
		goto dump;
	}
	prDescriptor->u4SectionNum = prRaw->u4SecNum;

	DBGLOG(UNI_FWDL, INFO,
		"pack_time=[%s] chip_id_eco=[%s] patch_ver=[%u] global_subsys=[%u %s] blk_num=[%u]\n",
		prHeader->aucPackTime,
		prHeader->aucChipIdNEcoVer,
		prHeader->u4PatchVer,
		prDescriptor->eGlobalSubsysType,
		GLOBAL_SUBSYS_STR(prDescriptor->eGlobalSubsysType),
		prDescriptor->u4SectionNum);

	/* <3> Section map */
	for (ucIdx = 0; ucIdx < prDescriptor->u4SectionNum; ucIdx++) {
		struct MULTI_HEADER_V2_SEC_RAW_FORMAT *prSecRaw;
		struct UNI_FWDL_SEC_MAP_INFO *prSecMapInfo;

		prSecRaw = (struct MULTI_HEADER_V2_SEC_RAW_FORMAT *)
			&prRaw->aucSections[ucIdx];
		prSecMapInfo = &prInfo->aucSecMaps[ucIdx];
		prPos += sizeof(*prSecRaw);

		if (prPos > prEnd) {
			DBGLOG(UNI_FWDL, ERROR,
				"Sec[%u] size not enough for MULTI_HEADER_V2_SEC_RAW_FORMAT\n",
				ucIdx);
			goto dump;
		}

		if (uniFwdlParseSecType(prSecRaw->u4Type,
					&prSecMapInfo->eType) !=
		    WLAN_STATUS_SUCCESS)
			goto dump;
		else if (uniFwdlParseSecSubsys(prSecRaw->u4Type,
					       &prSecMapInfo->eSubsys) !=
		    WLAN_STATUS_SUCCESS)
			goto dump;
		else if (uniFwdlParseSecSegment(prSecRaw->u4Type,
						&prSecMapInfo->eSegment) !=
		    WLAN_STATUS_SUCCESS)
			goto dump;

		/*
		 * Used for slot-DL mode, block 0 contains:
		 *     1. image_header
		 *     2. global_header
		 *     3. section_map
		 *     4. cert_section
		 */
		if (ucIdx == 0 &&
		    prSecMapInfo->eType == SUBSYS_SEC_TYPE_SIGNATURE)
			prInfo->u4HeaderSz += prSecRaw->u4Size;

		prSecMapInfo->u4SectionOffset = prSecRaw->u4Offset;
		prSecMapInfo->u4SectionSize = prSecRaw->u4Size;

		kalMemCopy(prSecMapInfo->aucSectionSpecific,
			   prSecRaw->aucSpecific,
			   sizeof(prSecRaw->aucSpecific));

		if (prSecMapInfo->u4SectionSize)
			prSecMapInfo->prContent =
				prFwBuffer + prSecMapInfo->u4SectionOffset;

		DBGLOG(UNI_FWDL, INFO,
			"blk[%u] type=[%u %s] subsys=[%u %s] segment=[%u %s] offset=[0x%x] size=[0x%x]\n",
			ucIdx,
			prSecMapInfo->eType,
			SECTION_TYPE_STR(prSecMapInfo->eType),
			prSecMapInfo->eSubsys,
			SECTION_SUBSYS_STR(prSecMapInfo->eSubsys),
			prSecMapInfo->eSegment,
			SECTION_SEGMENT_STR(prSecMapInfo->eSegment),
			prSecMapInfo->u4SectionOffset,
			prSecMapInfo->u4SectionSize);
		DBGLOG_MEM8(UNI_FWDL, LOUD,
			prSecMapInfo->aucSectionSpecific,
			sizeof(prSecMapInfo->aucSectionSpecific));
		DBGLOG_MEM8(UNI_FWDL, LOUD,
			prSecMapInfo->prContent,
			prSecMapInfo->u4SectionSize);
	}

	for (ucIdx = 0; ucIdx < prDescriptor->u4SectionNum; ucIdx++) {
		struct MULTI_HEADER_V2_SEC_RAW_FORMAT *prSecRaw;

		prSecRaw = (struct MULTI_HEADER_V2_SEC_RAW_FORMAT *)
			&prRaw->aucSections[ucIdx];
		prPos += prSecRaw->u4Size;

		if (prPos > prEnd) {
			DBGLOG(UNI_FWDL, ERROR,
				"Sec[%u] size not enough for content\n",
				ucIdx);
			goto dump;
		}
	}

	if (prPos != prEnd) {
		DBGLOG(UNI_FWDL, ERROR, "size mismatch\n");
		goto dump;
	}

	u4Status = WLAN_STATUS_SUCCESS;

dump:
	if (u4Status != WLAN_STATUS_SUCCESS) {
		DBGLOG(UNI_FWDL, ERROR,
			"Dump FW binary raw data, total size=0x%x\n",
			u4FwSize);
		DBGLOG_MEM8(UNI_FWDL, ERROR, prFwBuffer,
			u4FwSize < sizeof(struct MULTI_HEADER_V2_RAW_FORMAT) ?
			u4FwSize : sizeof(struct MULTI_HEADER_V2_RAW_FORMAT));
	}

exit:
	return u4Status;
}

uint32_t uniFwdlGetReleaseManifest(struct GLUE_INFO *prGlueInfo,
				   uint8_t *pucManifestBuffer,
				   uint32_t *pu4ManifestSize,
				   uint32_t u4BufferMaxSize)
{
	struct UNI_FWDL_BINARY_INFO *prInfo = NULL;
	uint8_t *prFwBuffer = NULL;
	uint32_t u4FwSize = 0, u4Status = WLAN_STATUS_FAILURE;
	uint8_t ucIdx;

	if (!pucManifestBuffer || !pu4ManifestSize || u4BufferMaxSize == 0) {
		DBGLOG(UNI_FWDL, ERROR,
			"pucManifestBuffer=0x%p pu4ManifestSize=%u u4BufferMaxSize=%u\n",
			pucManifestBuffer, pu4ManifestSize, u4BufferMaxSize);
		goto exit;
	}

	kalMemZero(pucManifestBuffer, u4BufferMaxSize);
	*pu4ManifestSize = 0;

	u4Status = uniFwdlOpenFwBinary(prGlueInfo, &prFwBuffer, &u4FwSize);
	if (u4Status != WLAN_STATUS_SUCCESS)
		goto exit;

	prInfo = (struct UNI_FWDL_BINARY_INFO *)
		kalMemZAlloc(sizeof(*prInfo), VIR_MEM_TYPE);
	if (!prInfo) {
		DBGLOG(UNI_FWDL, ERROR, "vmalloc(%u) failed\n",
			sizeof(*prInfo));
		u4Status = WLAN_STATUS_FAILURE;
		goto free_buf;
	}

	u4Status = uniFwdlParseFwBinaryInfo(prFwBuffer, u4FwSize, prInfo);
	if (u4Status != WLAN_STATUS_SUCCESS)
		goto free_buf;

	for (ucIdx = 0; ucIdx < prInfo->rDescriptor.u4SectionNum; ucIdx++) {
		struct UNI_FWDL_SEC_MAP_INFO *prSecMapInfo;

		prSecMapInfo = &prInfo->aucSecMaps[ucIdx];

		if (prSecMapInfo->eSubsys != SUBSYS_SEC_TYPE_SUBSYS_WF_WM ||
		    prSecMapInfo->eType != SUBSYS_SEC_TYPE_RELEASE_INFO)
			continue;

		*pu4ManifestSize =
			kalSnprintf(pucManifestBuffer,
				    kal_min_t(uint32_t, u4BufferMaxSize,
					      prSecMapInfo->u4SectionSize),
				    "%s",
				    prSecMapInfo->prContent);
		break;
	}

	if (ucIdx >= prInfo->rDescriptor.u4SectionNum)
		u4Status = WLAN_STATUS_FAILURE;
	else
		u4Status = WLAN_STATUS_SUCCESS;

free_buf:
	if (prInfo)
		kalMemFree(prInfo, VIR_MEM_TYPE, sizeof(*prInfo));

	uniFwdlCloseFwBinary(prGlueInfo, prFwBuffer, u4FwSize);
exit:
	return u4Status;
}

static void uniFwdlWakeupDlThread(struct ADAPTER *prAdapter,
				  enum UNI_FWLD_WAKEUP_SOURCE eSource)
{
	struct UNI_FWDL_CTX *prCtx = &prAdapter->rUniFwdlCtx;

	KAL_SET_BIT(eSource, prCtx->ulFlags);
	wake_up_interruptible(&prCtx->rWaitQ);
}

static uint32_t uniFwdlGetDlBlockById(struct UNI_FWDL_BINARY_INFO *prInfo,
				      uint16_t u2Radio, uint16_t u2BlockId,
				      uint8_t **pprContent, uint32_t *pu4Size)
{
	struct UNI_FWDL_SEC_MAP_INFO *prSecMapInfo;
	uint16_t u2SectionId;

	if (u2BlockId == 0) {
		*pprContent = prInfo->prHeader;
		*pu4Size = prInfo->u4HeaderSz;

		goto exit;
	}

	u2SectionId = u2BlockId - 1;

	if (u2SectionId >= prInfo->rDescriptor.u4SectionNum) {
		DBGLOG(UNI_FWDL, ERROR, "Invalid block ID %u\n", u2SectionId);
		return WLAN_STATUS_FAILURE;
	}

	prSecMapInfo = &prInfo->aucSecMaps[u2SectionId];

	if (prSecMapInfo->eType != SUBSYS_SEC_TYPE_BINARY_INFO) {
		DBGLOG(UNI_FWDL, ERROR,
			"Section ID(%u) Type(%u) not supported DL\n",
			u2SectionId, prSecMapInfo->eType);
		return WLAN_STATUS_FAILURE;
	}

	*pprContent = prSecMapInfo->prContent;
	*pu4Size = prSecMapInfo->u4SectionSize;

exit:
	return WLAN_STATUS_SUCCESS;
}

static void uniFwdlHandleDlResp(struct ADAPTER *prAdapter,
				struct UNI_FWDL_NOTIF_FWDL_RESP *prResp)
{
	struct UNI_FWDL_CTX *prCtx = &prAdapter->rUniFwdlCtx;

	DBGLOG(UNI_FWDL, INFO,
		"radio=%u resp=%u block_id=%u error_code=%u state=%u\n",
		prResp->eDlRadio,
		prResp->eResp,
		prResp->u2DlBlockId,
		prResp->u2ErrorCode,
		prResp->eCommState);

	switch (prResp->eResp) {
	case UNI_FWDL_RESP_STATUS_DEFAULT:
		break;

	case UNI_FWDL_RESP_STATUS_SUCCESS:
		break;

	case UNI_FWDL_RESP_STATUS_REQ_DL:
		prCtx->eCurrentDlRadio = prResp->eDlRadio;
		prCtx->u2CurrentDlBlockId = prResp->u2DlBlockId;
		uniFwdlWakeupDlThread(prAdapter,
				      WAKEUP_SOURCE_REQ_DL);
		break;

	case UNI_FWDL_RESP_STATUS_FAIL:
		prCtx->u2ErrorCode = prResp->u2ErrorCode;
		uniFwdlWakeupDlThread(prAdapter,
				      WAKEUP_SOURCE_DL_FAIL);
		break;

	default:
		DBGLOG(UNI_FWDL, WARN, "Unsupported fwdl resp(%u)\n",
			prResp->eResp);
		break;
	}
}

void uniFwdlRcvNotifCb(struct ADAPTER *prAdapter,
		       enum UNI_FWDL_NOTIF_ID eNotifId, void *prBuf,
		       uint32_t u4Size)
{
	DBGLOG(UNI_FWDL, INFO, "id=%u\n", eNotifId);

	switch (eNotifId) {
	case UNI_FWDL_NOTIF_ID_QUERY_HW_INFO:
	{
		uniFwdlWakeupDlThread(prAdapter,
				      WAKEUP_SOURCE_QUERY_HW_INFO);
	}
		break;

	case UNI_FWDL_NOTIF_ID_BROM_PATCH_DONE:
	{
		struct UNI_FWDL_NOTIF_NROM_PATCH_DONE *prDone;

		prDone = (struct UNI_FWDL_NOTIF_NROM_PATCH_DONE *)prBuf;
		DBGLOG(UNI_FWDL, INFO, "raido=%u\n", prDone->eRadioType);
		uniFwdlWakeupDlThread(prAdapter,
				      WAKEUP_SOURCE_BROM_PATCH_DONE);
	}
		break;

	case UNI_FWDL_NOTIF_ID_DL_DONE:
	{
		struct UNI_FWDL_NOTIF_DL_DONE *prDone;

		prDone = (struct UNI_FWDL_NOTIF_DL_DONE *)prBuf;
		DBGLOG(UNI_FWDL, INFO, "raido=%u\n", prDone->eRadioType);
		uniFwdlWakeupDlThread(prAdapter,
				      WAKEUP_SOURCE_DL_DONE);
	}
		break;

	case UNI_FWDL_NOTIF_ID_FWDL_RESP:
	{
		struct UNI_FWDL_NOTIF_FWDL_RESP *prResp;

		prResp = (struct UNI_FWDL_NOTIF_FWDL_RESP *)prBuf;
		uniFwdlHandleDlResp(prAdapter, prResp);
	}
		break;

	case UNI_FWDL_NOTIF_ID_BOOT_STAGE:
	{
		struct UNI_FWDL_NOTIF_BOOT_STAGE *prStage;

		prStage = (struct UNI_FWDL_NOTIF_BOOT_STAGE *)prBuf;
		DBGLOG(UNI_FWDL, INFO, "boot_stage=%u\n",
			prStage->eBootStage);
	}
		break;

	default:
		DBGLOG(UNI_FWDL, WARN,
			"Unsupported notification id(%u)\n",
			eNotifId);
		break;
	}
}

static uint32_t uniFwdlQueryHwInfo(struct ADAPTER *prAdapter,
				   struct UNI_FWDL_CTX *prCtx)
{
#define QUERY_HW_INFO_BUFFER_SIZE		256

	uint8_t *prBuffer = NULL;
	struct UNI_FWDL_NOTIF_HW_INFO *prInfo;
	uint32_t u4Status = WLAN_STATUS_FAILURE;
	int32_t i4Ret;
	uint8_t ucIdx;
	uint8_t *prPos;

	prBuffer = kalMemZAlloc(QUERY_HW_INFO_BUFFER_SIZE, VIR_MEM_TYPE);
	if (!prBuffer)
		goto exit;

	u4Status = uniFwdlHifQueryHwInfo(prAdapter, prBuffer,
					 QUERY_HW_INFO_BUFFER_SIZE);
	if (u4Status != WLAN_STATUS_SUCCESS)
		goto exit;

	i4Ret = wait_event_interruptible_timeout(prCtx->rWaitQ,
			(prCtx->ulFlags & BIT(WAKEUP_SOURCE_QUERY_HW_INFO)),
			MSEC_TO_JIFFIES(UNI_FWDL_DEFAULT_TIMEOUT));
	if (i4Ret <= 0) {
		DBGLOG(UNI_FWDL, WARN,
			"Wait for QUERY_HW_INFO failed, ret=%d\n",
			i4Ret);
		u4Status = WLAN_STATUS_FAILURE;
		goto exit;
	}

	KAL_CLR_BIT(BIT(WAKEUP_SOURCE_QUERY_HW_INFO), prCtx->ulFlags);

	prInfo = (struct UNI_FWDL_NOTIF_HW_INFO *)prBuffer;
	prPos = (uint8_t *)&prInfo->aucEntries[0];
	for (ucIdx = 0; ucIdx < prInfo->u4EntriesNum; ucIdx++) {
		struct UNI_FWDL_NOTIF_HW_INFO_ENTRY *prEntry =
			(struct UNI_FWDL_NOTIF_HW_INFO_ENTRY *)prPos;

		prPos += sizeof(*prEntry) + prEntry->u2Length;

		if (prEntry->u2Length == 0)
			continue;

		switch (prEntry->u2Tag) {
		case UNI_FWDL_NOTIF_HW_INFO_TAG_CHIP_ID:
			DBGLOG(UNI_FWDL, INFO,
				"chip id=0x%x\n",
				*((uint32_t *)prEntry->aucData));
			break;
		case UNI_FWDL_NOTIF_HW_INFO_TAG_HW_VER:
			DBGLOG(UNI_FWDL, INFO,
				"hw ver=0x%x\n",
				*((uint32_t *)prEntry->aucData));
			break;
		case UNI_FWDL_NOTIF_HW_INFO_TAG_FW_VER:
			DBGLOG(UNI_FWDL, INFO,
				"fw ver=0x%x\n",
				*((uint32_t *)prEntry->aucData));
			break;
		default:
			DBGLOG(UNI_FWDL, WARN,
				"unsupported tag(%u)\n",
				prEntry->u2Tag);
			continue;
		}
	}

	u4Status = WLAN_STATUS_SUCCESS;

exit:
	if (prBuffer)
		kalMemFree(prBuffer, VIR_MEM_TYPE, QUERY_HW_INFO_BUFFER_SIZE);

	return u4Status;
}

static uint32_t uniFwdlInit(struct ADAPTER *prAdapter)
{
	struct UNI_FWDL_CTX *prCtx = &prAdapter->rUniFwdlCtx;

	kalMemZero(prCtx, sizeof(*prCtx));

	init_waitqueue_head(&prCtx->rWaitQ);

	prCtx->eCurrentDlRadio = UNI_FWDL_HIF_DL_RADIO_TYPE_RESERVED;
	prCtx->u2CurrentDlBlockId = UNI_FWDL_MAX_SECTION_NUM;

	return WLAN_STATUS_SUCCESS;
}

static void uniFwdlDeInit(struct ADAPTER *prAdapter)
{
}

uint32_t uniFwdlDownloadFW(struct ADAPTER *prAdapter)
{
	struct UNI_FWDL_CTX *prCtx = &prAdapter->rUniFwdlCtx;
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct UNI_FWDL_INFO *prUniFwdlInfo =
		prAdapter->chip_info->uni_fwdl_info;
	struct UNI_FWDL_BINARY_INFO *prInfo = NULL;
	struct START_DL_PARAMS rParams;
	uint64_t u8StartTime, u8Timeout;
	uint8_t *prFwBuffer = NULL;
	uint32_t u4FwSize = 0, u4Status = WLAN_STATUS_FAILURE;
	int32_t i4Ret;

	u8StartTime = kalGetBootTime();
	u8Timeout = u8StartTime + MSEC_TO_USEC(UNI_FWDL_DEFAULT_TIMEOUT);

	if (!prUniFwdlInfo) {
		DBGLOG(UNI_FWDL, ERROR, "No pre-defined uni_fwdl_info.\n");
		goto exit;
	}

	DBGLOG(UNI_FWDL, INFO,
		"sync=0x%x timeout=%u calibration=%u\n",
		prUniFwdlInfo->u4SyncInfo,
		prUniFwdlInfo->u4FwTimeoutDuration,
		prUniFwdlInfo->ucCalibrationAction);

	u4Status = uniFwdlInit(prAdapter);
	if (u4Status != WLAN_STATUS_SUCCESS)
		goto exit;

	u4Status = uniFwdlOpenFwBinary(prGlueInfo, &prFwBuffer, &u4FwSize);
	if (u4Status != WLAN_STATUS_SUCCESS)
		goto deinit_fwdl;

	prInfo = (struct UNI_FWDL_BINARY_INFO *)
		kalMemZAlloc(sizeof(*prInfo), VIR_MEM_TYPE);
	if (!prInfo) {
		DBGLOG(UNI_FWDL, ERROR, "vmalloc(%u) failed\n",
			sizeof(*prInfo));
		goto close_binary;
	}

	u4Status = uniFwdlParseFwBinaryInfo(prFwBuffer, u4FwSize, prInfo);
	if (u4Status != WLAN_STATUS_SUCCESS)
		goto free_info;

	u4Status = uniFwdlHifInit(prAdapter, uniFwdlRcvNotifCb, u4FwSize);
	if (u4Status != WLAN_STATUS_SUCCESS)
		goto free_info;

	if (uniFwdlQueryHwInfo(prAdapter, prCtx) != WLAN_STATUS_SUCCESS)
		DBGLOG(UNI_FWDL, ERROR, "Query HW info failed.\n");

	kalMemZero(&rParams, sizeof(rParams));
	rParams.eRadio = UNI_FWDL_HIF_DL_RADIO_TYPE_WF;
	rParams.prFwBuffer = prInfo->prFwBuffer;
	rParams.u4FwSize = prInfo->u4FwSize;
	rParams.u4HeaderSz = prInfo->u4HeaderSz;
	rParams.u4SyncInfo = prUniFwdlInfo->u4SyncInfo;
	rParams.eCoDlRadioBitmap = 0;
	rParams.u4CoDlWfHeaderSz = 0;
	rParams.u4CoDlBtHeaderSz = 0;
	rParams.u4CoDlZbHeaderSz = 0;
	rParams.eAction = prUniFwdlInfo->ucCalibrationAction;
	rParams.u4FwTimeoutDuration = prUniFwdlInfo->u4FwTimeoutDuration;
	u4Status = uniFwdlHifStartDl(prAdapter, &rParams);
	if (u4Status != WLAN_STATUS_SUCCESS)
		goto deinit_hif;

	while (TRUE) {
		do {
			i4Ret = wait_event_interruptible_timeout(prCtx->rWaitQ,
				(prCtx->ulFlags & UNI_FWDL_WAKEUP_CONDITION),
				MSEC_TO_JIFFIES(UNI_FWDL_WAIT_DURATION));

			/* timeout */
			if (i4Ret == 0 && (kalGetBootTime() >= u8Timeout)) {
				DBGLOG(UNI_FWDL, WARN,
					"Wait timeout\n");
				KAL_SET_BIT(WAKEUP_SOURCE_HALT,
					    prCtx->ulFlags);
				uniFwdlHifDump(prAdapter);
				break;
			}
		} while (i4Ret == 0);

		if (KAL_TEST_AND_CLEAR_BIT(WAKEUP_SOURCE_HALT,
					   prCtx->ulFlags)) {
			DBGLOG(UNI_FWDL, WARN, "HALT\n");
			u4Status = WLAN_STATUS_FAILURE;
			break;
		}

		if (KAL_TEST_AND_CLEAR_BIT(WAKEUP_SOURCE_DL_DONE,
					   prCtx->ulFlags)) {
			DBGLOG(UNI_FWDL, INFO, "DL DONE\n");
			u4Status = WLAN_STATUS_SUCCESS;
			break;
		}

		if (KAL_TEST_AND_CLEAR_BIT(WAKEUP_SOURCE_DL_FAIL,
					   prCtx->ulFlags)) {
			DBGLOG(UNI_FWDL, WARN, "DL FAIL, reason=%u\n",
				prCtx->u2ErrorCode);
			u4Status = WLAN_STATUS_FAILURE;
			break;
		}

		if (KAL_TEST_AND_CLEAR_BIT(WAKEUP_SOURCE_BROM_PATCH_DONE,
					   prCtx->ulFlags)) {
			if (prUniFwdlInfo->u4SyncInfo &
			    UNI_FWDL_SYNC_INFO_DFD_DUMP) {
				/* TODO */
			}

			if (prUniFwdlInfo->u4SyncInfo &
			    UNI_FWDL_SYNC_INFO_DL_XTAL_PKT) {
				/* TODO */
			}

#if (CFG_SUPPORT_UNI_FWDL_SET_SKU_CONFIG == 1)
			if (uniFwdlHifSendMsg(prAdapter,
					      UNI_FWDL_HIF_DL_RADIO_TYPE_WF,
					      UNI_FWDL_MSG_ID_SET_SKU_CONFIG,
					      NULL, 0) !=
			    WLAN_STATUS_SUCCESS) {
				DBGLOG(UNI_FWDL, ERROR,
					"Set SKU CFG failed\n");
				u4Status = WLAN_STATUS_FAILURE;
				break;
			}
#endif /* CFG_SUPPORT_UNI_FWDL_SET_SKU_CONFIG */

			if (uniFwdlHifSendMsg(prAdapter,
					      UNI_FWDL_HIF_DL_RADIO_TYPE_WF,
					      UNI_FWDL_MSG_ID_RESUME_DL,
					      NULL, 0) !=
			    WLAN_STATUS_SUCCESS) {
				DBGLOG(UNI_FWDL, ERROR,
					"Resume DL failed\n");
				u4Status = WLAN_STATUS_FAILURE;
				break;
			}
		}

		if (KAL_TEST_AND_CLEAR_BIT(WAKEUP_SOURCE_REQ_DL,
					   prCtx->ulFlags)) {
			uint8_t *prContent;
			uint32_t u4ContentSize;

			if (uniFwdlGetDlBlockById(prInfo,
						  prCtx->eCurrentDlRadio,
						  prCtx->u2CurrentDlBlockId,
						  &prContent,
						  &u4ContentSize) !=
			    WLAN_STATUS_SUCCESS) {
				DBGLOG(UNI_FWDL, WARN,
					"Get radio(%u) block(%u) failed\n",
					prCtx->eCurrentDlRadio,
					prCtx->u2CurrentDlBlockId);
				u4Status = WLAN_STATUS_FAILURE;
				break;
			}

			if (uniFwdlHifDlBlock(prAdapter,
					      prContent,
					      u4ContentSize) !=
			    WLAN_STATUS_SUCCESS) {
				DBGLOG(UNI_FWDL, WARN,
					"DL block failed\n");
				u4Status = WLAN_STATUS_FAILURE;
				break;
			}
		}
	}

deinit_hif:
	uniFwdlHifDeInit(prAdapter);

free_info:
	if (prInfo)
		kalMemFree(prInfo, VIR_MEM_TYPE, sizeof(*prInfo));

close_binary:
	uniFwdlCloseFwBinary(prGlueInfo, prFwBuffer, u4FwSize);

deinit_fwdl:
	uniFwdlDeInit(prAdapter);

exit:
	DBGLOG(UNI_FWDL, INFO,
		"Status=0x%x Time=%llums\n",
		u4Status,
		USEC_TO_MSEC(kalGetBootTime() - u8StartTime));
	if (u4Status == WLAN_STATUS_SUCCESS)
		prAdapter->fgIsFwDownloaded = TRUE;
	return u4Status;
}
