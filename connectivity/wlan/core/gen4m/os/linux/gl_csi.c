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
#include "gl_csi.h"

#if CFG_SUPPORT_CSI
/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
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

uint8_t glCsiGetBandIdx(struct GLUE_INFO *prGlueInfo)
{
	if (!prGlueInfo)
		return 0;

	return prGlueInfo->ucCSIBandIdx;
}

void glCsiSetBandIdx(struct GLUE_INFO *prGlueInfo, uint8_t ucBandIdx)
{
	if (!prGlueInfo)
		return;

	prGlueInfo->ucCSIBandIdx = ucBandIdx;
}

struct CSI_INFO_T *glCsiGetCSIInfo(struct GLUE_INFO *prGlueInfo)
{
	if (!prGlueInfo)
		return NULL;

	return &prGlueInfo->rCSIInfo;
}

uint8_t *glCsiGetCSIBuf(struct GLUE_INFO *prGlueInfo)
{
	if (!prGlueInfo)
		return NULL;

	return prGlueInfo->aucCSIBuf;
}

struct CSI_DATA_T *glCsiGetCSIData(struct GLUE_INFO *prGlueInfo)
{
	if (!prGlueInfo)
		return NULL;

	return prGlueInfo->rCSIInfo.prCSIData;
}

void glCsiSupportInit(struct GLUE_INFO *prGlueInfo)
{
	if (!prGlueInfo) {
		DBGLOG(REQ, ERROR, "prGlueInfo Empty.\n");
		return;
	}

	kalMemZero(&prGlueInfo->rCSIInfo, sizeof(prGlueInfo->rCSIInfo));

	/* init CSI wait queue	*/
	init_waitqueue_head(&(prGlueInfo->waitq_csi));
	LINK_INITIALIZE(&(prGlueInfo->rCSIInfo.rStaList));
	prGlueInfo->rCSIInfo.eCSIOutput = CSI_OUTPUT_PROC_FILE;
}

void glCsiSupportDeinit(struct GLUE_INFO *prGlueInfo)
{
	struct CSI_INFO_T *prCSIInfo = glCsiGetCSIInfo(prGlueInfo);

	glCsiFreeStaList(prGlueInfo);

	if (prCSIInfo && prCSIInfo->prCSIData) {
		kalMemFree(prCSIInfo->prCSIData,
			   VIR_MEM_TYPE,
			   sizeof(struct CSI_DATA_T));
		prCSIInfo->prCSIData = NULL;
	}
}

void glCsiSetEnable(struct GLUE_INFO *prGlueInfo,
	struct CSI_INFO_T *prCSIInfo, u_int8_t fgEnable)
{
	prCSIInfo->bIncomplete = FALSE;
	prCSIInfo->u4CopiedDataSize = 0;
	prCSIInfo->u4RemainingDataSize = 0;
	prCSIInfo->u4CSIBufferHead = 0;
	prCSIInfo->u4CSIBufferTail = 0;
	prCSIInfo->u4CSIBufferUsed = 0;

	if (fgEnable) {
		if (prCSIInfo->prCSIData) {
			DBGLOG(REQ, WARN,
				"CSI data NOT freed before alloc.\n");
			kalMemFree(prCSIInfo->prCSIData,
				   VIR_MEM_TYPE,
				   sizeof(struct CSI_DATA_T));
			prCSIInfo->prCSIData = NULL;
		}

		prCSIInfo->prCSIData = kalMemAlloc(sizeof(struct CSI_DATA_T),
			VIR_MEM_TYPE);
		if (!prCSIInfo->prCSIData)
			DBGLOG(REQ, ERROR, "Alloc CSI data failed.\n");
	} else {
		if (prCSIInfo->prCSIData) {
			kalMemFree(prCSIInfo->prCSIData,
				   VIR_MEM_TYPE,
				   sizeof(struct CSI_DATA_T));
			prCSIInfo->prCSIData = NULL;
		}
	}
}

int32_t glCsiAddSta(struct GLUE_INFO *prGlueInfo,
			struct CMD_CSI_CONTROL_T *prCSICtrl)
{
	struct CSI_INFO_T *prCSIInfo = glCsiGetCSIInfo(prGlueInfo);
	struct CSI_STA *prCSISta = NULL;

	if (!prCSIInfo) {
		DBGLOG(REQ, ERROR, "CSI data Empty.\n");
		return -1;
	}

	/* list full */
	if (prCSIInfo->ucStaCount >= CSI_MAX_STA_MAC_NUM) {
		DBGLOG(REQ, DEBUG,
			"[CSI] List is full! Current number=%d\n",
			prCSIInfo->ucStaCount);
		return -1;
	}

	/* search sta list */
	KAL_ACQUIRE_MUTEX(prGlueInfo->prAdapter, MUTEX_CSI_STA_LIST);
	LINK_FOR_EACH_ENTRY(prCSISta, &prCSIInfo->rStaList,
		rLinkEntry, struct CSI_STA) {
		if (EQUAL_MAC_ADDR(prCSISta->aucMacAddress,
				prCSICtrl->aucMacAddr)) {
			DBGLOG(REQ, DEBUG,
				"[CSI] Sta mac (" MACSTR
				") is already in csi sta list",
				MAC2STR(prCSISta->aucMacAddress));
			KAL_RELEASE_MUTEX(
				prGlueInfo->prAdapter, MUTEX_CSI_STA_LIST);
			return -1;
		}
	}
	KAL_RELEASE_MUTEX(prGlueInfo->prAdapter, MUTEX_CSI_STA_LIST);

	/* new sta */
	prCSISta = (struct CSI_STA *)
			kalMemAlloc(sizeof(struct CSI_STA), VIR_MEM_TYPE);
	if (!prCSISta) {
		DBGLOG(REQ, LOUD,
			"[CSI] Allocate memory for prCSISta failed!\n");
		return -1;
	}

	kalMemSet(prCSISta, 0, sizeof(struct CSI_STA));
	COPY_MAC_ADDR(prCSISta->aucMacAddress, prCSICtrl->aucMacAddr);
	KAL_ACQUIRE_MUTEX(prGlueInfo->prAdapter, MUTEX_CSI_STA_LIST);
	LINK_INSERT_HEAD(&prCSIInfo->rStaList, &prCSISta->rLinkEntry);
	KAL_RELEASE_MUTEX(prGlueInfo->prAdapter, MUTEX_CSI_STA_LIST);

	DBGLOG(REQ, DEBUG,
		"[CSI] Add sta mac (" MACSTR ") to CSI List.\n",
		MAC2STR(prCSISta->aucMacAddress));
	prCSIInfo->ucStaCount++;

	return 0;
}

int32_t glCsiDelSta(struct GLUE_INFO *prGlueInfo,
			struct CMD_CSI_CONTROL_T *prCSICtrl)
{
	struct CSI_INFO_T *prCSIInfo = glCsiGetCSIInfo(prGlueInfo);
	struct CSI_STA *prCSISta = NULL;
	uint8_t fgIsFound;

	if (!prCSIInfo) {
		DBGLOG(REQ, ERROR, "CSI data Empty.\n");
		return -1;
	}

	/* list empty */
	if (prCSIInfo->ucStaCount == 0) {
		DBGLOG(REQ, ERROR,
			"[CSI] List is empty! Current number=%d\n",
			prCSIInfo->ucStaCount);
		return -1;
	}

	/* search sta list */
	fgIsFound = FALSE;
	KAL_ACQUIRE_MUTEX(prGlueInfo->prAdapter, MUTEX_CSI_STA_LIST);
	LINK_FOR_EACH_ENTRY(prCSISta, &prCSIInfo->rStaList,
		rLinkEntry, struct CSI_STA) {
		if (EQUAL_MAC_ADDR(prCSISta->aucMacAddress,
				prCSICtrl->aucMacAddr)) {
			DBGLOG(REQ, DEBUG,
				"[CSI] Del sta mac (" MACSTR ") to CSI List.\n",
				MAC2STR(prCSISta->aucMacAddress));
			fgIsFound = TRUE;

			LINK_REMOVE_HEAD(&prCSIInfo->rStaList,
				prCSISta, struct CSI_STA *);
			kalMemFree(prCSISta, VIR_MEM_TYPE,
				sizeof(struct CSI_STA));
			prCSIInfo->ucStaCount--;
			break;
		}
	}
	KAL_RELEASE_MUTEX(prGlueInfo->prAdapter, MUTEX_CSI_STA_LIST);

	/* not find */
	if (!fgIsFound) {
		DBGLOG(REQ, DEBUG,
			"[CSI] Sta mac (" MACSTR
			") is not found in CSI list.\n",
			MAC2STR(prCSISta->aucMacAddress));
		return -1;
	}

	return 0;
}

void glCsiFreeStaList(struct GLUE_INFO *prGlueInfo)
{
	struct CSI_STA *prCSISta = NULL;
	struct CSI_INFO_T *prCSIInfo = glCsiGetCSIInfo(prGlueInfo);

	KAL_ACQUIRE_MUTEX(prGlueInfo->prAdapter, MUTEX_CSI_STA_LIST);
	while (!LINK_IS_EMPTY(&prCSIInfo->rStaList)) {
		LINK_REMOVE_HEAD(&prCSIInfo->rStaList,
			prCSISta, struct CSI_STA *);
		DBGLOG(INIT, DEBUG, "[CSI] Remove sta mac (" MACSTR ")\n",
			MAC2STR(prCSISta->aucMacAddress));
		kalMemFree(prCSISta, VIR_MEM_TYPE,
			sizeof(struct CSI_STA));
	}
	KAL_RELEASE_MUTEX(prGlueInfo->prAdapter, MUTEX_CSI_STA_LIST);
}

void nicEventCSIData(struct ADAPTER *prAdapter,
			struct WIFI_EVENT *prEvent)
{
	struct CSI_TLV_ELEMENT *prCSITlvData;
	int32_t i4EventLen;
	int16_t i2Idx = 0;
	int8_t *prBuf = NULL;
	uint16_t *pru2Tmp = NULL;
	uint32_t *p32tmp = NULL;
	struct CSI_DATA_T *prCSIData = NULL;
	struct CSI_DATA_T *prCSIBuffer;
	struct CSI_INFO_T *prCSIInfo = NULL;
	/* u2Offset is 8 bytes currently, tag 4 bytes + length 4 bytes */
	uint16_t u2Offset = OFFSET_OF(struct CSI_TLV_ELEMENT, aucbody);
	uint32_t u4Tmp = 0;
	uint8_t ucLastTagFlg = false;
	uint16_t u2DataCount = 0;
	u_int8_t bStatus;

#define CSI_EVENT_MAX_SIZE 2500

	if (!prAdapter || !prAdapter->prGlueInfo)
		return;

	i4EventLen = prEvent->u2PacketLength -
			sizeof(struct WIFI_EVENT);
	if (i4EventLen > CSI_EVENT_MAX_SIZE
		|| i4EventLen < sizeof(struct CSI_TLV_ELEMENT)) {
		DBGLOG(NIC, WARN, "[CSI] Invalid CSI event size %u\n",
			i4EventLen);
		return;
	}
	prCSIInfo = glCsiGetCSIInfo(prAdapter->prGlueInfo);
	prCSIData = (struct CSI_DATA_T *)
			kalMemAlloc(sizeof(struct CSI_DATA_T), VIR_MEM_TYPE);

	if (!prCSIData) {
		DBGLOG(NIC, WARN, "[CSI] Alloc prCSIData failed!");
		return;
	}

	/* TimeStamp: msec */
	prCSIData->u8TimeStamp = div_u64(kalGetBootTime(), USEC_PER_MSEC);

	prBuf = (int8_t *) (prEvent->aucBuffer);

#if CFG_CSI_DEBUG
	DBGLOG_MEM8(NIC, TRACE, (uint8_t *) prBuf, i4EventLen);
#endif
	while ((i4EventLen >= u2Offset) && (ucLastTagFlg == false)) {
		prCSITlvData = (struct CSI_TLV_ELEMENT *) prBuf;

		if (prCSITlvData->body_len >
			(i4EventLen - (int32_t) u2Offset)) {
			DBGLOG(NIC, WARN,
				"[CSI] Invalid body_len: %u, tag_type=%u\n",
				prCSITlvData->body_len,
				prCSITlvData->tag_type);
			goto out;
		}


#if CFG_CSI_DEBUG
		DBGLOG(NIC, TRACE, "[CSI] tag_type=%d, body_len=%d\n",
			prCSITlvData->tag_type, prCSITlvData->body_len);
#endif
		if (prCSITlvData->tag_type == (CSI_EVENT_TLV_TAG_NUM - 1))
			ucLastTagFlg = true;

		switch (prCSITlvData->tag_type) {
		case CSI_EVENT_VERSION:
			if (prCSITlvData->body_len != sizeof(uint32_t)) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid FwVer len %u",
					prCSITlvData->body_len);
				goto out;
			}

			prCSIData->ucFwVer = (uint8_t) le32_to_cpup(
					(uint32_t *) prCSITlvData->aucbody);
			break;
		case CSI_EVENT_CBW:
			if (prCSITlvData->body_len != sizeof(uint32_t)) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid CBW len %u",
					prCSITlvData->body_len);
				goto out;
			}

			prCSIData->ucBw = (uint8_t) le32_to_cpup(
					(uint32_t *) prCSITlvData->aucbody);
			break;
		case CSI_EVENT_RSSI:
			if (prCSITlvData->body_len != sizeof(uint32_t)) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid RSSI len %u",
					prCSITlvData->body_len);
				goto out;
			}

			prCSIData->cRssi = (int8_t) le32_to_cpup(
					(uint32_t *) prCSITlvData->aucbody);
			break;
		case CSI_EVENT_SNR:
			if (prCSITlvData->body_len != sizeof(uint32_t)) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid SNR len %u",
					prCSITlvData->body_len);
				goto out;
			}

			prCSIData->ucSNR = (uint8_t) le32_to_cpup(
					(uint32_t *) prCSITlvData->aucbody);
			break;
		case CSI_EVENT_BAND:
			if (prCSITlvData->body_len != sizeof(uint32_t)) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid BAND len %u",
					prCSITlvData->body_len);
				goto out;
			}

			prCSIData->ucDbdcIdx = (uint8_t) le32_to_cpup(
					(uint32_t *) prCSITlvData->aucbody);
				DBGLOG(NIC, DEBUG, "[CSI] Band=%u\n",
						prCSIData->ucDbdcIdx);
			break;
		case CSI_EVENT_CSI_NUM:
			if (prCSITlvData->body_len != sizeof(uint32_t)) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid CSI num len %u",
					prCSITlvData->body_len);
				goto out;
			}

			prCSIData->u2DataCount = (uint16_t) le32_to_cpup(
					(uint32_t *) prCSITlvData->aucbody);

			if (prCSIData->u2DataCount > CSI_MAX_DATA_COUNT) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid CSI count %u\n",
					prCSIData->u2DataCount);
				goto out;
			} else
				u2DataCount = prCSIData->u2DataCount;
			break;
		case CSI_EVENT_CSI_I_DATA:
			if (prCSITlvData->body_len !=
				sizeof(int16_t) * u2DataCount) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid CSI num len %u",
					prCSITlvData->body_len);
				goto out;
			}

			kalMemZero(prCSIData->ac2IData,
				sizeof(prCSIData->ac2IData));

			pru2Tmp = (int16_t *) prCSITlvData->aucbody;
			for (i2Idx = 0; i2Idx < u2DataCount; i2Idx++)
				prCSIData->ac2IData[i2Idx] =
					(int16_t) le16_to_cpup(pru2Tmp + i2Idx);
			break;
		case CSI_EVENT_CSI_Q_DATA:
			if (prCSITlvData->body_len !=
				sizeof(int16_t) * u2DataCount) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid CSI num len %u",
					prCSITlvData->body_len);
				goto out;
			}

			kalMemZero(prCSIData->ac2QData,
				sizeof(prCSIData->ac2QData));

			pru2Tmp = (int16_t *) prCSITlvData->aucbody;
			for (i2Idx = 0; i2Idx < u2DataCount; i2Idx++)
				prCSIData->ac2QData[i2Idx] =
					(int16_t) le16_to_cpup(pru2Tmp + i2Idx);
			break;
		case CSI_EVENT_DBW:
			if (prCSITlvData->body_len != sizeof(uint32_t)) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid DBW len %u",
					prCSITlvData->body_len);
				goto out;
			}

			prCSIData->ucDataBw = (uint8_t) le32_to_cpup(
					(int32_t *) prCSITlvData->aucbody);
			break;
		case CSI_EVENT_CH_IDX:
			if (prCSITlvData->body_len != sizeof(uint32_t)) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid CH IDX len %u",
					prCSITlvData->body_len);
				goto out;
			}

			prCSIData->ucPrimaryChIdx = (uint8_t) le32_to_cpup(
					(int32_t *) prCSITlvData->aucbody);
			break;
		case CSI_EVENT_TA:
			/*
			 * TA length is 8-byte long (MAC addr 6 bytes +
			 * 2 bytes padding), the 2-byte padding keeps
			 * the next Tag at a 4-byte aligned address.
			 */
			if (prCSITlvData->body_len !=
				ALIGN_4(sizeof(prCSIData->aucTA))) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid TA len %u",
					prCSITlvData->body_len);
				goto out;
			}
			kalMemCopy(prCSIData->aucTA, prCSITlvData->aucbody,
				sizeof(prCSIData->aucTA));
			break;
		case CSI_EVENT_EXTRA_INFO:
			if (prCSITlvData->body_len != sizeof(uint32_t)) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid Error len %u",
					prCSITlvData->body_len);
				goto out;
			}

			prCSIData->u4ExtraInfo = le32_to_cpup(
					(int32_t *) prCSITlvData->aucbody);
			break;
		case CSI_EVENT_RX_MODE:
			if (prCSITlvData->body_len != sizeof(uint32_t)) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid Rx Mode len %u",
					prCSITlvData->body_len);
				goto out;
			}

			u4Tmp = le32_to_cpup((int32_t *) prCSITlvData->aucbody);

			prCSIData->ucRxMode = (uint8_t) GET_CSI_RX_MODE(u4Tmp);
			if (prCSIData->ucRxMode == 0)
				prCSIData->bIsCck = TRUE;
			else
				prCSIData->bIsCck = FALSE;

			prCSIData->u2RxRate = GET_CSI_RATE(u4Tmp);
			break;
		case CSI_EVENT_RSVD1:
			if (prCSITlvData->body_len >
				sizeof(int32_t) * CSI_MAX_RSVD1_COUNT) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid RSVD1 len %u",
					prCSITlvData->body_len);
				goto out;
			}

			kalMemCopy(prCSIData->ai4Rsvd1,
				prCSITlvData->aucbody,
				prCSITlvData->body_len);

			prCSIData->ucRsvd1Cnt = (uint8_t)
				prCSITlvData->body_len / sizeof(int32_t);
			break;
		case CSI_EVENT_RSVD2:
			if (prCSITlvData->body_len >
				sizeof(int32_t) * CSI_MAX_RSVD2_COUNT) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid RSVD2 len %u",
					prCSITlvData->body_len);
				goto out;
			}

			prCSIData->ucRsvd2Cnt =
				prCSITlvData->body_len / sizeof(int32_t);
			p32tmp = (int32_t *) (prCSITlvData->aucbody);
			for (i2Idx = 0; i2Idx < prCSIData->ucRsvd2Cnt; i2Idx++)
				prCSIData->au4Rsvd2[i2Idx] =
					(int32_t) le2cpu32(*(p32tmp + i2Idx));
			break;
		case CSI_EVENT_RSVD3:
			if (prCSITlvData->body_len != sizeof(uint32_t)) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid RSVD3 len %u",
					prCSITlvData->body_len);
				goto out;
			}

			prCSIData->i4Rsvd3 = (int32_t) le32_to_cpup(
					(int32_t *) prCSITlvData->aucbody);
			break;
		case CSI_EVENT_RSVD4:
			if (prCSITlvData->body_len != sizeof(uint32_t)) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid RSVD4 len %u",
					prCSITlvData->body_len);
				goto out;
			}

			prCSIData->ucRsvd4 = (uint8_t) le32_to_cpup(
					(uint32_t *) prCSITlvData->aucbody);
			break;
		case CSI_EVENT_H_IDX:
			if (prCSITlvData->body_len != sizeof(uint32_t)) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid Antenna_pattern len %u",
					prCSITlvData->body_len);
				goto out;
			}

			prCSIData->Antenna_pattern = le32_to_cpup(
					(uint32_t *) prCSITlvData->aucbody);

			/*to drop pkt that usr do not need*/
			if ((prCSIInfo->Matrix_Get_Bit) &&
				!(prCSIInfo->Matrix_Get_Bit &
				(1 << (prCSIData->Antenna_pattern & 0xf)))) {
				DBGLOG(NIC, WARN,
					"[CSI] drop Antenna_pattern=%d\n",
					prCSIData->Antenna_pattern);
				goto out;
			}
			break;
		case CSI_EVENT_TX_RX_IDX:
			if (prCSITlvData->body_len != sizeof(uint32_t)) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid TRxIdx len %u\n",
					prCSITlvData->body_len);
				goto out;
			}

			prCSIData->u4TRxIdx = le32_to_cpup(
					(uint32_t *) prCSITlvData->aucbody);
			break;
		case CSI_EVENT_RSVD5:
			if (prCSITlvData->body_len != sizeof(uint32_t)) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid RSVD5 len %u",
					prCSITlvData->body_len);
				goto out;
			}

			prCSIData->u4Rsvd5 = le32_to_cpup(
					(uint32_t *) prCSITlvData->aucbody);
			break;
		case CSI_EVENT_BW_SEG:
			if (prCSITlvData->body_len != sizeof(uint32_t)) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid Segment Number len %u",
					prCSITlvData->body_len);
				goto out;
			}

			prCSIData->u4SegmentNum = le32_to_cpup(
					(uint32_t *) prCSITlvData->aucbody);
			break;
		case CSI_EVENT_REMAIN_LAST:
			if (prCSITlvData->body_len != sizeof(uint32_t)) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid Remain Last len %u",
					prCSITlvData->body_len);
				goto out;
			}

			prCSIData->ucRemainLast = (uint8_t) le32_to_cpup(
					(uint32_t *) prCSITlvData->aucbody);
			break;
		case CSI_EVENT_RSVD6:
			if (prCSITlvData->body_len != sizeof(uint32_t)) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid RSVD6 len %u",
					prCSITlvData->body_len);
				goto out;
			}

			prCSIData->u4Rsvd6 = le32_to_cpup(
					(uint32_t *) prCSITlvData->aucbody);
			break;
		case CSI_EVENT_TONE_VALID:
			if (prCSITlvData->body_len != sizeof(uint32_t)) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid TONE_VALID len %u",
					prCSITlvData->body_len);
				goto out;
			}

			prCSIData->u4ToneValid = le32_to_cpup(
					(uint32_t *) prCSITlvData->aucbody);
			break;
		default:
			DBGLOG(NIC, WARN, "[CSI] Unsupported CSI tag %d\n",
				prCSITlvData->tag_type);
			if (prCSITlvData->body_len
				> (i4EventLen - sizeof(struct CSI_TLV_ELEMENT))
				|| prCSITlvData->body_len
					< sizeof(struct CSI_TLV_ELEMENT)) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid body_len %u",
					prCSITlvData->body_len);
				goto out;
			}
		};

		i4EventLen -= (u2Offset + prCSITlvData->body_len);

		if (i4EventLen >= u2Offset)
			prBuf += (u2Offset + prCSITlvData->body_len);
	}

	DBGLOG(NIC, DEBUG, "[CSI] u2DataCount=%d\n",
				prCSIData->u2DataCount);

	bStatus = wlanPushCSISegmentData(prAdapter, prCSIData);

	if (bStatus && prCSIData->ucRemainLast == 0) {
		if (prCSIData->u4SegmentNum == 0)
			prCSIBuffer = prCSIData;
		else
			prCSIBuffer = &(prCSIInfo->rCSISegmentTemp);

		/* mask the null tone */
		wlanApplyCSIToneMask(prCSIBuffer->ucRxMode,
			prCSIBuffer->ucBw, prCSIBuffer->ucDataBw,
			prCSIBuffer->ucPrimaryChIdx,
			prCSIBuffer->ac2IData, prCSIBuffer->ac2QData);

		wlanPushCSIData(prAdapter, prCSIBuffer);

		switch (prCSIInfo->eCSIOutput) {
		case CSI_OUTPUT_PROC_FILE:
			wake_up_interruptible(
				&(prAdapter->prGlueInfo->waitq_csi));
			break;
		case CSI_OUTPUT_VENDOR_EVENT:
			mtk_cfg80211_vendor_event_csi_raw_data(prAdapter);
			break;
		default:
			DBGLOG(NIC, ERROR,
				"[CSI] Unknown CSI data output method = %d",
				prCSIInfo->eCSIOutput);
		}
	}

out:
	kalMemFree(prCSIData, VIR_MEM_TYPE, sizeof(struct CSI_DATA_T));
}

u_int8_t
wlanPushCSISegmentData(struct ADAPTER *prAdapter,
	struct CSI_DATA_T *prCSIData)
{
	struct CSI_INFO_T *prCSIInfo = glCsiGetCSIInfo(prAdapter->prGlueInfo);
	struct CSI_DATA_T *prCSISegmentTemp;

#if CFG_CSI_DEBUG
	DBGLOG(NIC, DEBUG,
		"[CSI] Segment number=%d, Remain last=%d\n",
		prCSIData->u4SegmentNum, prCSIData->ucRemainLast);
#endif

	/* Put the segment CSI data into CSI segment temp */
	if (prCSIData->u4SegmentNum == 0 &&
		prCSIData->ucRemainLast == 1) {
		kalMemCopy(&(prCSIInfo->rCSISegmentTemp),
				prCSIData, sizeof(struct CSI_DATA_T));

		return TRUE;
	} else if (prCSIData->u4SegmentNum != 0) {
		prCSISegmentTemp = &(prCSIInfo->rCSISegmentTemp);
		if (prCSIData->Antenna_pattern !=
			prCSISegmentTemp->Antenna_pattern ||
			prCSIData->u4SegmentNum !=
			(prCSISegmentTemp->u4SegmentNum + 1)) {
			DBGLOG(NIC, ERROR,
				"[CSI] H_IDX [%d/%d] is different or SEG_NUM [%d/%d] is not sequential.\n",
				prCSISegmentTemp->Antenna_pattern,
				prCSIData->Antenna_pattern,
				prCSISegmentTemp->u4SegmentNum,
				prCSIData->u4SegmentNum);

			return FALSE;
		}

		kalMemCopy(&prCSISegmentTemp->ac2IData[
				prCSISegmentTemp->u2DataCount],
			prCSIData->ac2IData,
			prCSIData->u2DataCount * sizeof(int16_t));

		kalMemCopy(&prCSISegmentTemp->ac2QData[
				prCSISegmentTemp->u2DataCount],
			prCSIData->ac2QData,
			prCSIData->u2DataCount * sizeof(int16_t));

		prCSISegmentTemp->u2DataCount += prCSIData->u2DataCount;
		prCSISegmentTemp->u4SegmentNum = prCSIData->u4SegmentNum;
		prCSISegmentTemp->ucRemainLast = prCSIData->ucRemainLast;
	}

	return TRUE;

}

u_int8_t
wlanPushCSIData(struct ADAPTER *prAdapter, struct CSI_DATA_T *prCSIData)
{
	struct CSI_INFO_T *prCSIInfo = glCsiGetCSIInfo(prAdapter->prGlueInfo);

	KAL_ACQUIRE_MUTEX(prAdapter, MUTEX_CSI_BUFFER);

	/* Put the CSI data into CSI event queue */
	if (prCSIInfo->u4CSIBufferUsed != 0) {
		prCSIInfo->u4CSIBufferTail++;
		prCSIInfo->u4CSIBufferTail %= CSI_RING_SIZE;
	}

	kalMemCopy(&(prCSIInfo->arCSIBuffer[prCSIInfo->u4CSIBufferTail]),
		prCSIData, sizeof(struct CSI_DATA_T));

	if (prCSIInfo->u4CSIBufferUsed < CSI_RING_SIZE) {
		prCSIInfo->u4CSIBufferUsed++;
	} else {
		/*
		 * While new CSI event comes and the ring buffer is
		 * already full, the new coming CSI event will
		 * overwrite the oldest one in the ring buffer.
		 * Thus, the Head pointer which points to * the
		 * oldest CSI event in the buffer should be moved too.
		 */
		prCSIInfo->u4CSIBufferHead++;
		prCSIInfo->u4CSIBufferHead %= CSI_RING_SIZE;
	}

#if CFG_CSI_DEBUG
	DBGLOG(NIC, DEBUG,
		"[CSI] Push idx = %d, data count = %d, H_IDX = %d\n",
		prCSIInfo->u4CSIBufferTail,
		prCSIData->u2DataCount,
		prCSIData->Antenna_pattern);
#endif

	KAL_RELEASE_MUTEX(prAdapter, MUTEX_CSI_BUFFER);

	return TRUE;
}

u_int8_t
wlanPopCSIData(struct ADAPTER *prAdapter, struct CSI_DATA_T *prCSIData)
{
	struct CSI_INFO_T *prCSIInfo = glCsiGetCSIInfo(prAdapter->prGlueInfo);

	KAL_ACQUIRE_MUTEX(prAdapter, MUTEX_CSI_BUFFER);

	/* No CSI data in the ring buffer */
	if (prCSIInfo->u4CSIBufferUsed == 0) {
		KAL_RELEASE_MUTEX(prAdapter, MUTEX_CSI_BUFFER);
		return FALSE;
	}

	kalMemCopy(prCSIData,
		&(prCSIInfo->arCSIBuffer[prCSIInfo->u4CSIBufferHead]),
		sizeof(struct CSI_DATA_T));

#if CFG_CSI_DEBUG
	DBGLOG(NIC, DEBUG,
		"[CSI] Pop idx = %d, data count = %d, H_IDX = %d\n",
		prCSIInfo->u4CSIBufferHead,
		prCSIData->u2DataCount,
		prCSIData->Antenna_pattern);
#endif

	prCSIInfo->u4CSIBufferUsed--;
	if (prCSIInfo->u4CSIBufferUsed != 0) {
		prCSIInfo->u4CSIBufferHead++;
		prCSIInfo->u4CSIBufferHead %= CSI_RING_SIZE;
	}
	KAL_RELEASE_MUTEX(prAdapter, MUTEX_CSI_BUFFER);

	return TRUE;
}

ssize_t wlanCSIDataPrepare(
	uint8_t *buf,
	struct CSI_INFO_T *prCSIInfo,
	struct CSI_DATA_T *prCSIData)
{
	int32_t i4Pos = 0;
	uint8_t *tmpBuf = buf;
	uint16_t u2DataSize = prCSIData->u2DataCount * sizeof(int16_t);
	uint16_t u2Rsvd1Size = prCSIData->ucRsvd1Cnt * sizeof(int32_t);
	enum ENUM_CSI_MODULATION_BW_TYPE_T eModulationType = CSI_TYPE_CCK_BW20;

	if (prCSIData->ucBw == 0)
		eModulationType = prCSIData->bIsCck ?
			CSI_TYPE_CCK_BW20 : CSI_TYPE_OFDM_BW20;
	else if (prCSIData->ucBw == 1)
		eModulationType = CSI_TYPE_OFDM_BW40;
	else if (prCSIData->ucBw == 2)
		eModulationType = CSI_TYPE_OFDM_BW80;

	put_unaligned(0xAC, (tmpBuf + i4Pos));
	i4Pos++;

	/* Just bypass total length feild here and update it in the end */
	i4Pos += 2;

	put_unaligned(CSI_DATA_VER, (uint8_t *) (tmpBuf + i4Pos));
	i4Pos++;
	put_unaligned(sizeof(uint8_t), (uint16_t *) (tmpBuf + i4Pos));
	i4Pos += 2;
	put_unaligned(prCSIData->ucFwVer, (uint8_t *) (tmpBuf + i4Pos));
	i4Pos += sizeof(uint8_t);

	put_unaligned(CSI_DATA_TYPE, (uint8_t *) (tmpBuf + i4Pos));
	i4Pos++;
	put_unaligned(sizeof(uint8_t), (uint16_t *) (tmpBuf + i4Pos));
	i4Pos += 2;
	put_unaligned(eModulationType, (uint8_t *) (tmpBuf + i4Pos));
	i4Pos += sizeof(uint8_t);

	put_unaligned(CSI_DATA_TS, (uint8_t *) (tmpBuf + i4Pos));
	i4Pos++;
	put_unaligned(sizeof(uint64_t), (uint16_t *) (tmpBuf + i4Pos));
	i4Pos += 2;
	put_unaligned(prCSIData->u8TimeStamp, (uint64_t *) (tmpBuf + i4Pos));
	i4Pos += sizeof(uint64_t);

	put_unaligned(CSI_DATA_RSSI, (uint8_t *) (tmpBuf + i4Pos));
	i4Pos++;
	put_unaligned(sizeof(uint8_t), (uint16_t *) (tmpBuf + i4Pos));
	i4Pos += 2;
	put_unaligned(prCSIData->cRssi, (uint8_t *) (tmpBuf + i4Pos));
	i4Pos += sizeof(uint8_t);

	put_unaligned(CSI_DATA_SNR, (tmpBuf + i4Pos));
	i4Pos++;
	put_unaligned(sizeof(uint8_t), (uint16_t *) (tmpBuf + i4Pos));
	i4Pos += 2;
	put_unaligned(prCSIData->ucSNR, (uint8_t *) (tmpBuf + i4Pos));
	i4Pos += sizeof(uint8_t);

	put_unaligned(CSI_DATA_DBW, (uint8_t *) (tmpBuf + i4Pos));
	i4Pos++;
	put_unaligned(sizeof(uint8_t), (uint16_t *) (tmpBuf + i4Pos));
	i4Pos += 2;
	put_unaligned(prCSIData->ucDataBw, (uint8_t *) (tmpBuf + i4Pos));
	i4Pos += sizeof(uint8_t);

	put_unaligned(CSI_DATA_CH_IDX, (uint8_t *) (tmpBuf + i4Pos));
	i4Pos++;
	put_unaligned(sizeof(uint8_t), (uint16_t *) (tmpBuf + i4Pos));
	i4Pos += 2;
	put_unaligned(prCSIData->ucPrimaryChIdx, (uint8_t *) (tmpBuf + i4Pos));
	i4Pos += sizeof(uint8_t);

	put_unaligned(CSI_DATA_TA, (uint8_t *) (tmpBuf + i4Pos));
	i4Pos++;
	put_unaligned(MAC_ADDR_LEN, (uint16_t *) (tmpBuf + i4Pos));
	i4Pos += 2;
	kalMemCopy((tmpBuf + i4Pos), prCSIData->aucTA, MAC_ADDR_LEN);
	i4Pos += MAC_ADDR_LEN;

	put_unaligned(CSI_DATA_EXTRA_INFO, (uint8_t *) (tmpBuf + i4Pos));
	i4Pos++;
	put_unaligned(sizeof(uint32_t), (uint16_t *) (tmpBuf + i4Pos));
	i4Pos += 2;
	put_unaligned(prCSIData->u4ExtraInfo, (uint32_t *) (tmpBuf + i4Pos));
	i4Pos += sizeof(uint32_t);

	put_unaligned(CSI_DATA_I, (uint8_t *) (tmpBuf + i4Pos));
	i4Pos++;
	put_unaligned(u2DataSize, (uint16_t *) (tmpBuf + i4Pos));
	i4Pos += 2;
	kalMemCopy((tmpBuf + i4Pos), prCSIData->ac2IData, u2DataSize);
	i4Pos += u2DataSize;

	put_unaligned(CSI_DATA_Q, (uint8_t *) (tmpBuf + i4Pos));
	i4Pos++;
	put_unaligned(u2DataSize, (uint16_t *) (tmpBuf + i4Pos));
	i4Pos += 2;
	kalMemCopy((tmpBuf + i4Pos), prCSIData->ac2QData, u2DataSize);
	i4Pos += u2DataSize;

	if (prCSIInfo->ucValue1[CSI_CONFIG_INFO] & CSI_INFO_RSVD1) {
		put_unaligned(CSI_DATA_RSVD1, (uint8_t *) (tmpBuf + i4Pos));
		i4Pos++;
		put_unaligned(u2Rsvd1Size, (uint16_t *) (tmpBuf + i4Pos));
		i4Pos += 2;
		kalMemCopy((tmpBuf + i4Pos),
				prCSIData->ai4Rsvd1, u2Rsvd1Size);
		i4Pos += u2Rsvd1Size;

		put_unaligned(CSI_DATA_RSVD2, (uint8_t *) (tmpBuf + i4Pos));
		i4Pos++;
		put_unaligned(u2Rsvd1Size, (uint16_t *) (tmpBuf + i4Pos));
		i4Pos += 2;
		kalMemCopy((tmpBuf + i4Pos),
				prCSIData->au4Rsvd2, u2Rsvd1Size);
		i4Pos += u2Rsvd1Size;

		put_unaligned(CSI_DATA_RSVD3, (uint8_t *) (tmpBuf + i4Pos));
		i4Pos++;
		put_unaligned(sizeof(int32_t), (int16_t *) (tmpBuf + i4Pos));
		i4Pos += 2;
		put_unaligned(prCSIData->i4Rsvd3, (int32_t *) (tmpBuf + i4Pos));
		i4Pos += sizeof(int32_t);
	}

	if (prCSIInfo->ucValue1[CSI_CONFIG_INFO] & CSI_INFO_RSVD2) {
		put_unaligned(CSI_DATA_RSVD4, (uint8_t *) (tmpBuf + i4Pos));
		i4Pos++;
		put_unaligned(sizeof(uint8_t), (int16_t *) (tmpBuf + i4Pos));
		i4Pos += 2;
		put_unaligned(prCSIData->ucRsvd4, (uint8_t *) (tmpBuf + i4Pos));
		i4Pos += sizeof(uint8_t);
	}

	put_unaligned(CSI_DATA_TX_IDX, (uint8_t *) (tmpBuf + i4Pos));
	i4Pos++;
	put_unaligned(sizeof(uint8_t), (int16_t *) (tmpBuf + i4Pos));
	i4Pos += 2;
	put_unaligned(((uint8_t) GET_CSI_TX_IDX(prCSIData->u4TRxIdx)),
					(uint8_t *) (tmpBuf + i4Pos));
	i4Pos += sizeof(uint8_t);

	put_unaligned(CSI_DATA_RX_IDX, (uint8_t *) (tmpBuf + i4Pos));
	i4Pos++;
	put_unaligned(sizeof(uint8_t), (int16_t *) (tmpBuf + i4Pos));
	i4Pos += 2;
	put_unaligned(((uint8_t) GET_CSI_RX_IDX(prCSIData->u4TRxIdx)),
					(uint8_t *) (tmpBuf + i4Pos));
	i4Pos += sizeof(uint8_t);

	put_unaligned(CSI_DATA_FRAME_MODE, (uint8_t *) (tmpBuf + i4Pos));
	i4Pos++;
	put_unaligned(sizeof(uint8_t), (int16_t *) (tmpBuf + i4Pos));
	i4Pos += 2;
	put_unaligned(prCSIData->ucRxMode, (uint8_t *) (tmpBuf + i4Pos));
	i4Pos += sizeof(uint8_t);

	/* add antenna pattern*/
	put_unaligned(CSI_DATA_H_IDX, (uint8_t *) (tmpBuf + i4Pos));
	i4Pos++;
	put_unaligned(sizeof(uint32_t), (int16_t *) (tmpBuf + i4Pos));
	i4Pos += 2;
	put_unaligned(prCSIData->Antenna_pattern,
				(uint32_t *) (tmpBuf + i4Pos));
	i4Pos += sizeof(uint32_t);

	put_unaligned(CSI_DATA_RX_RATE, (uint8_t *) (tmpBuf + i4Pos));
	i4Pos++;
	put_unaligned(sizeof(uint8_t), (int16_t *) (tmpBuf + i4Pos));
	i4Pos += 2;
	put_unaligned(prCSIData->u2RxRate, (uint8_t *) (tmpBuf + i4Pos));
	i4Pos += sizeof(uint8_t);

	put_unaligned(CSI_DATA_RSVD5, (uint8_t *) (tmpBuf + i4Pos));
	i4Pos++;
	put_unaligned(sizeof(uint32_t), (int16_t *) (tmpBuf + i4Pos));
	i4Pos += 2;
	put_unaligned(prCSIData->u4Rsvd5, (uint32_t *) (tmpBuf + i4Pos));
	i4Pos += sizeof(uint32_t);

	put_unaligned(CSI_DATA_RSVD6, (uint8_t *) (tmpBuf + i4Pos));
	i4Pos++;
	put_unaligned(sizeof(uint32_t), (int16_t *) (tmpBuf + i4Pos));
	i4Pos += 2;
	put_unaligned(prCSIData->u4Rsvd6, (uint32_t *) (tmpBuf + i4Pos));
	i4Pos += sizeof(uint32_t);

	put_unaligned(CSI_DATA_BAND, (uint8_t *) (tmpBuf + i4Pos));
	i4Pos += sizeof(uint8_t);
	put_unaligned(sizeof(uint8_t), (int16_t *) (tmpBuf + i4Pos));
	i4Pos += sizeof(int16_t);
	put_unaligned(prCSIData->ucDbdcIdx, (uint8_t *) (tmpBuf + i4Pos));
	i4Pos += sizeof(uint8_t);

	put_unaligned(CSI_DATA_TONE_VALID, (uint8_t *) (tmpBuf + i4Pos));
	i4Pos += sizeof(uint8_t);
	put_unaligned(sizeof(uint32_t), (int16_t *) (tmpBuf + i4Pos));
	i4Pos += sizeof(int16_t);
	put_unaligned(prCSIData->u4ToneValid, (uint32_t *) (tmpBuf + i4Pos));
	i4Pos += sizeof(uint32_t);

	/*
	 * The lengths of magic number (1 byte) and total length (2 bytes)
	 * fields should not be counted in the total length value
	 */
	put_unaligned(i4Pos - 3, (uint16_t *) (tmpBuf + 1));

#if CFG_CSI_DEBUG
	DBGLOG(REQ, DEBUG, "[CSI] debug: i4Pos = %d", i4Pos);
#endif

	return i4Pos;
}

/*
*CSI TONE MASK
*this function mask(clear) the null tone && pilot tones
*for example, bw20 has  64 tones in total.however there
*are some null tone && pilot tones we do not need for the
*feature of channel state information(CSI).
*so we need to clear these tone.
*/
void wlanApplyCSIToneMask(
	uint8_t ucRxMode,
	uint8_t ucCBW,
	uint8_t ucDBW,
	uint8_t ucPrimaryChIdx,
	int16_t *ai2IData,
	int16_t *ai2QData)
{
#define ZERO(index) \
{ ai2IData[index] = 0; ai2QData[index] = 0; }

#define ZERO_RANGE(start, end) \
{\
	kalMemZero(&ai2IData[start], sizeof(int16_t) * (end - start + 1));\
	kalMemZero(&ai2QData[start], sizeof(int16_t) * (end - start + 1));\
}

	/*Mask the NULL TONE*/
	if (ucRxMode == RX_VT_LEGACY_OFDM) {
		if (ucDBW == RX_VT_FR_MODE_20) {
			ZERO(0);
		} else if (ucDBW == RX_VT_FR_MODE_40) {
			ZERO(32); ZERO(96);
		} else if (ucDBW == RX_VT_FR_MODE_80) {
			ZERO(32); ZERO(96);
			ZERO(160); ZERO(224);
		} else if (ucDBW == RX_VT_FR_MODE_160) {
			ZERO(32); ZERO(96);
			ZERO(160); ZERO(224);
			ZERO(288); ZERO(352);
			ZERO(416); ZERO(480);
		} else if (ucDBW == RX_VT_FR_MODE_320) {
			ZERO(32); ZERO(96);
			ZERO(160); ZERO(224);
			ZERO(288); ZERO(352);
			ZERO(416); ZERO(480);
			ZERO(672); ZERO(736);
			ZERO(800); ZERO(864);
			ZERO(928); ZERO(992);
		}
	} else if (ucRxMode == RX_VT_MIXED_MODE ||
		ucRxMode == RX_VT_GREEN_MODE ||
		ucRxMode == RX_VT_VHT_MODE) {

		if (ucDBW == RX_VT_FR_MODE_20) {
			ZERO(0);
		} else if (ucDBW == RX_VT_FR_MODE_40) {
			ZERO(0); ZERO(1);
			ZERO(127);
		} else if (ucDBW == RX_VT_FR_MODE_80) {
			ZERO(0); ZERO(1);
			ZERO(255);
		} else if (ucDBW == RX_VT_FR_MODE_160) {
			ZERO(5); ZERO(127);
			ZERO(128); ZERO(129);
			ZERO(261); ZERO(383);
			ZERO(384); ZERO(385);
		}
	} else if (ucRxMode == RX_VT_HE_MODE)
		ZERO(0);

	/*Mask the VHT Pilots*/
	if (ucRxMode == RX_VT_VHT_MODE) {
		if (ucDBW == RX_VT_FR_MODE_20) {
			ZERO(7); ZERO(21); ZERO(43); ZERO(57);
		} else if (ucDBW == RX_VT_FR_MODE_40) {
			ZERO(11); ZERO(25); ZERO(53);
			ZERO(75); ZERO(103); ZERO(117);
		} else if (ucDBW == RX_VT_FR_MODE_80) {
			ZERO(11); ZERO(39); ZERO(75); ZERO(103);
			ZERO(153); ZERO(181); ZERO(217); ZERO(245);
		} else if (ucDBW == RX_VT_FR_MODE_160) {
			ZERO(25); ZERO(53); ZERO(89); ZERO(117);
			ZERO(139); ZERO(167); ZERO(203); ZERO(231);
			ZERO(281); ZERO(309); ZERO(345); ZERO(373);
			ZERO(395); ZERO(423); ZERO(459); ZERO(487);
		}
	}
}

void
wlanShiftCSI(
	uint8_t ucRxMode,
	uint8_t ucCBW,
	uint8_t ucDBW,
	uint8_t ucPrimaryChIdx,
	int16_t *ai2IData,
	int16_t *ai2QData,
	int16_t *ai2ShiftIData,
	int16_t *ai2ShiftQData)
{
	uint8_t ucSize = sizeof(int16_t);

	if (ai2IData == NULL) {
		DBGLOG(RSN, ERROR, "[CSI] ai2IData = NULL\n");
		return;
	}
	if (ai2QData == NULL) {
		DBGLOG(RSN, ERROR, "[CSI] ai2QData = NULL\n");
		return;
	}
	if (ai2ShiftIData == NULL) {
		DBGLOG(RSN, ERROR, "[CSI] ai2ShiftIData = NULL\n");
		return;
	}
	if (ai2ShiftQData == NULL) {
		DBGLOG(RSN, ERROR, "[CSI] ai2ShiftQData = NULL\n");
		return;
	}

#define COPY_RANGE(dest, start, end) \
{\
	kalMemCopy(&ai2ShiftIData[dest], \
		&ai2IData[start], ucSize * (end - start + 1)); \
	kalMemCopy(&ai2ShiftQData[dest], \
		&ai2QData[start], ucSize * (end - start + 1)); \
}

#define COPY(dest, src) \
{ ai2ShiftIData[dest] = ai2IData[src]; ai2ShiftQData[dest] = ai2QData[src]; }

	if (ucRxMode == RX_VT_LEGACY_OFDM) {
		if (ucCBW == RX_VT_FR_MODE_20) {
			COPY_RANGE(0, 0, 63);
		} else if (ucCBW == RX_VT_FR_MODE_40) {
			if (ucDBW == RX_VT_FR_MODE_40) {
				COPY_RANGE(0, 0, 127);
			} else if (ucDBW == RX_VT_FR_MODE_20) {
				if (ucPrimaryChIdx == 0) {
					COPY(0, 96);
					COPY_RANGE(38, 70, 95);
					COPY_RANGE(1, 97, 122);
				} else {
					COPY(0, 32);
					COPY_RANGE(38, 6, 31);
					COPY_RANGE(1, 33, 58);
				}
			}
		} else if (ucCBW == RX_VT_FR_MODE_80) {
			if (ucDBW == RX_VT_FR_MODE_80) {
				COPY_RANGE(0, 0, 255);
			} else if (ucDBW == RX_VT_FR_MODE_40) {
				if (ucPrimaryChIdx <= 1) {
					COPY(0, 192);
					COPY_RANGE(2, 198, 250);
					COPY_RANGE(74, 134, 186);
				} else {
					COPY(0, 64);
					COPY_RANGE(2, 70, 122);
					COPY_RANGE(74, 6, 58);
				}
			} else if (ucDBW == RX_VT_FR_MODE_20) {
				if (ucPrimaryChIdx == 0) {
					COPY(0, 160);
					COPY_RANGE(1, 161, 186);
					COPY_RANGE(38, 134, 159);
				} else if (ucPrimaryChIdx == 1) {
					COPY(0, 224);
					COPY_RANGE(1, 225, 250);
					COPY_RANGE(38, 198, 223);
				} else if (ucPrimaryChIdx == 2) {
					COPY(0, 32);
					COPY_RANGE(1, 33, 58);
					COPY_RANGE(38, 6, 31);
				} else {
					COPY(0, 96);
					COPY_RANGE(1, 97, 122);
					COPY_RANGE(38, 70, 95);
				}
			}
		}
	} else if (ucRxMode == RX_VT_MIXED_MODE ||
		ucRxMode == RX_VT_GREEN_MODE ||
		ucRxMode == RX_VT_VHT_MODE) {
		if (ucCBW == RX_VT_FR_MODE_20) {
			COPY_RANGE(0, 0, 63);
		} else if (ucCBW == RX_VT_FR_MODE_40) {
			if (ucDBW == RX_VT_FR_MODE_40) {
				COPY_RANGE(0, 0, 127);
			} else if (ucDBW == RX_VT_FR_MODE_20) {
				if (ucPrimaryChIdx == 0) {
					COPY(0, 96);
					COPY_RANGE(36, 68, 95);
					COPY_RANGE(1, 97, 124);
				} else {
					COPY(0, 32);
					COPY_RANGE(36, 4, 31);
					COPY_RANGE(1, 33, 60);
				}
			}
		} else if (ucCBW == RX_VT_FR_MODE_80) {
			if (ucDBW == RX_VT_FR_MODE_80) {
				COPY_RANGE(0, 0, 255);
			} else if (ucDBW == RX_VT_FR_MODE_40) {
				if (ucPrimaryChIdx <= 1) {
					COPY(0, 192);
					COPY_RANGE(2, 194, 250);
					COPY_RANGE(70, 134, 190);
				} else {
					COPY(0, 64);
					COPY_RANGE(2, 66, 122);
					COPY_RANGE(70, 6, 62);
				}
			} else if (ucDBW == RX_VT_FR_MODE_20) {
				if (ucPrimaryChIdx == 0) {
					COPY(0, 160);
					COPY_RANGE(1, 161, 188);
					COPY_RANGE(36, 132, 159);
				} else if (ucPrimaryChIdx == 1) {
					COPY(0, 224);
					COPY_RANGE(1, 225, 252);
					COPY_RANGE(36, 196, 223);
				} else if (ucPrimaryChIdx == 2) {
					COPY(0, 32);
					COPY_RANGE(1, 33, 60);
					COPY_RANGE(36, 4, 31);
				} else {
					COPY(0, 96);
					COPY_RANGE(1, 97, 124);
					COPY_RANGE(36, 68, 95);
				}
			}
		}
	} else if (ucRxMode == RX_VT_HE_MODE) {
		if (ucCBW == RX_VT_FR_MODE_20) {
			COPY_RANGE(0, 0, 63);
		} else if (ucCBW == RX_VT_FR_MODE_40) {
			if (ucDBW == RX_VT_FR_MODE_40) {
				COPY_RANGE(0, 0, 127);
			} else if (ucDBW == RX_VT_FR_MODE_20) {
				if (ucPrimaryChIdx == 0) {
					COPY(0, 96);
					COPY_RANGE(34, 66, 95);
					COPY_RANGE(1, 97, 126);
				} else {
					COPY(0, 32);
					COPY_RANGE(34, 2, 31);
					COPY_RANGE(1, 31, 62);
				}
			}
		} else if (ucCBW == RX_VT_FR_MODE_80) {
			if (ucDBW == RX_VT_FR_MODE_80) {
				COPY_RANGE(0, 0, 255);
			} else if (ucDBW == RX_VT_FR_MODE_40) {
				if (ucPrimaryChIdx <= 1) {
					COPY(0, 192);
					COPY_RANGE(1, 193, 253);
					COPY_RANGE(67, 131, 191);
				} else {
					COPY(0, 64);
					COPY_RANGE(1, 65, 125);
					COPY_RANGE(67, 3, 63);
				}
			} else if (ucDBW == RX_VT_FR_MODE_20) {
				if (ucPrimaryChIdx == 0) {
					COPY(0, 160);
					COPY_RANGE(1, 161, 190);
					COPY_RANGE(34, 130, 159);
				} else if (ucPrimaryChIdx == 1) {
					COPY(0, 224);
					COPY_RANGE(1, 225, 254);
					COPY_RANGE(34, 194, 223);
				} else if (ucPrimaryChIdx == 2) {
					COPY(0, 32);
					COPY_RANGE(1, 33, 62);
					COPY_RANGE(34, 2, 31);
				} else {
					COPY(0, 96);
					COPY_RANGE(1, 97, 126);
					COPY_RANGE(34, 66, 95);
				}
			}
		}
	}
}
#endif /* CFG_SUPPORT_CSI */

