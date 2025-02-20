// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */


/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

#if (CFG_SUPPORT_FW_IDX_LOG_TRANS == 1)
/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

#include "precomp.h"

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define IDXLOGSTX		0xA8

#define CTRL_TYPE_TIME_SYNC	1
#define CTRL_TYPE_LOG_LOST	2

#define DBG_LOG_BUF_SIZE 256


/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                       P R I V A T E   D A T A
 *******************************************************************************
 */
struct IDX_LOG_ENTRY {
	uint8_t *prFile;	/* file name */
	uint8_t *prLine;	/* line info of the log */
	uint8_t *prStr;		/* log body */
	uint32_t u4StrLen;	/* log body length */
	uint32_t u4EntryLen;	/* total log message entry length */
};

static const char * const apucFwDbgModule[] = {
	"HEM",			/* 0 */
	"AIS",
	"RLM",
	"MEM",
	"CNM",
	"RSN",			/* 5 */
	"RXM",
	"TXM",
	"BSS",
	"EVENT",
	"SCN",			/* 10 */
	"SAA",
	"AAA",
	"MGT",
	"INIT",
	"REQ",			/* 15 */
	"WAPI",
	"NIC",
	"HAL",
	"COEX",
	"PM",			/* 20 */
	"LP",
	"RFTEST",
	"ICAP",
	"RRM",
	"MQM",			/* 25 */
	"RDM",
	"P2P",
	"RLM_AR",
	"AR",
	"ROAMING",		/* 30 */
	"RTT",
	"TDLS",
	"BF",
	"MU",
	"SLT",			/* 35 */
	"CMD",
	"RA",
	"PF",
	"WTBL",
	"SER",			/* 40 */
	"WH_HIF",
	"TP",
	"SUSPEND_MODE",
	"DBDC",
	"ISR",			/* 45 */
	"HWCFG",
	"VOW",
	"EMUL",
	"ARP_NS",
	"TIMER",		/* 50 */
	"STAREC",
	"THERMAL",
	"TXPOWER",
	"PKT_DROP",
	"STATS",		/* 55 */
	"RSSI_MONITOR",
	"WFC_KEEP_ALIVE",
	"TB",
	"TXCMD",
	"DYNSND",		/* 60 */
	"SPE",
	"EXT_TXCMD",
	"EXT_CMDRPT_TX",
	"EXT_CMDRPT_TRIG",
	"EXT_SPL",		/* 65 */
	"EXT_RXRPT",
	"EXT_TXV",
	"EXT_HERA",
	"SCS",
	"COANT",		/* 70 */
	"ECC",
	"MURU",
	"TAM",
	"SR",
	"RXV",			/* 75 */
	"CONNCOEX",
	"MDVT",
	"CAL",
	"EXT_RANGE",
	"AHDBG",		/* 80 */
	"DPP",
	"NAN",
	"DYNWMM",
	"EXT_SND",
	"EXT_BSRP",		/* 85 */
	"EXT_TPUTM",
	"MLO",
	"AMSDU",
	"MLR",
};

static const char * const apucFwDbgLvl[] = {
	"ERROR",
	"WARN",
	"STATE",
	"INFO",
	"LOUD",
	"EVENT",		/* 5 */
	"TRACE",
};

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
#if 0
uint32_t wlanStoreIdxLogBin(struct ADAPTER *prAdapter, uint8_t *prSrc,
			    uint32_t u4FwSize)
{
	void *prFwBuffer = NULL;
	struct FW_LOG_IDX_DATA *prIdxLogBin = NULL;

	if (IS_FEATURE_DISABLED(prAdapter->rWifiVar.fgFwIdxLogTrans)) {
		DBGLOG(INIT, STATE, "FW IDX LOG Trans hasn't be enabled\n");
		return -1;
	}

	if (!prSrc || !u4FwSize) {
		DBGLOG(INIT, WARN, "Src is invalid\n");
		return -1;
	}

	prFwBuffer = vmalloc(ALIGN_4(u4FwSize));
	if (!prFwBuffer)
		return -1;

	prIdxLogBin = kalMemAlloc(sizeof(struct FW_LOG_IDX_DATA), VIR_MEM_TYPE);
	if (!prIdxLogBin) {
		vfree(prFwBuffer);
		return -1;
	}

	kalMemCopy(prFwBuffer, prSrc, u4FwSize);

	kalMemSet(prIdxLogBin, 0, sizeof(struct FW_LOG_IDX_DATA));
	prIdxLogBin->u4FwSize = u4FwSize;
	prIdxLogBin->prFwBuffer = prFwBuffer;

	prAdapter->prFwLogIdx = prIdxLogBin;

	return 0;
}
#endif

uint32_t wlanOpenIdxLogBin(struct ADAPTER *prAdapter)
{
	void *prFwBuffer = NULL;
	struct FW_LOG_IDX_DATA *prIdxLogBin;
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;
	uint8_t *apucName[2] = {0};
	uint8_t ucNameBody[CFG_FW_NAME_MAX_LEN] = {0};
	uint32_t u4FwSize = 0;
	uint32_t u4Ret = WLAN_STATUS_FAILURE;

	DBGLOG(INIT, STATE, "open wlanOpenIdxLogBin\n");

	if (IS_FEATURE_DISABLED(prAdapter->rWifiVar.fgFwIdxLogTrans)) {
		DBGLOG(INIT, STATE, "FW IDX LOG Trans hasn't be enabled\n");
		goto out;
	}

	if (prAdapter->prFwLogIdx) {
		DBGLOG(INIT, WARN, "FW IDX LOG has loaded\n");
		goto out;
	}

	if (!prChipInfo->fw_dl_ops->constrcutIdxLogBin) {
		DBGLOG(INIT, WARN, "constrcutIdxLogBin is NULL\n");
		goto out;
	}

	apucName[0] = ucNameBody;
	prChipInfo->fw_dl_ops->constrcutIdxLogBin(prGlueInfo, apucName);

	DBGLOG(INIT, TRACE, "idx bin file is %s\n", apucName[0]);

	if (kalFirmwareOpen(prGlueInfo, apucName) !=
		WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, WARN, "Can't open %s\n", apucName[0]);
		goto out;
	}

	/* <2> Query firmare size */
	kalFirmwareSize(prGlueInfo, &u4FwSize);

	/* <3> Use vmalloc for allocating large memory trunk */
	prFwBuffer = vmalloc(ALIGN_4(u4FwSize));

	if (prFwBuffer == NULL)
		goto out;

	/* <4> Load image binary into buffer */
	if (kalFirmwareLoad(prGlueInfo, prFwBuffer, 0,
			    &u4FwSize) != WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, ERROR, "Can't load %s\n", apucName[0]);
		goto err_free;
	}

	/* <5> write back info */
	prIdxLogBin = kalMemAlloc(sizeof(struct FW_LOG_IDX_DATA), VIR_MEM_TYPE);
	if (prIdxLogBin == NULL) {
		DBGLOG(INIT, ERROR, "Can't alloc prIdxLogBin\n");
		goto err_free;
	}
	kalMemSet(prIdxLogBin, 0, sizeof(struct FW_LOG_IDX_DATA));
	prIdxLogBin->u4FwSize = u4FwSize;
	prIdxLogBin->prFwBuffer = prFwBuffer;

	prAdapter->prFwLogIdx = prIdxLogBin;

	u4Ret = WLAN_STATUS_SUCCESS;

	goto close_firm;

err_free:
	vfree(prFwBuffer);
close_firm:
	kalFirmwareClose(prGlueInfo);
out:
	return u4Ret;
}

void wlanCloseIdxLogBin(struct ADAPTER *prAdapter)
{
	struct FW_LOG_IDX_DATA *prIdxLogBin = prAdapter->prFwLogIdx;

	DBGLOG(INIT, STATE, "close wlanCloseIdxLogBin\n");

	if (!prIdxLogBin)
		return;

	prAdapter->prFwLogIdx = NULL;

	if (prIdxLogBin->prFwBuffer != NULL)
		vfree(prIdxLogBin->prFwBuffer);

	kalMemFree(prIdxLogBin, VIR_MEM_TYPE, sizeof(struct FW_LOG_IDX_DATA));
}

uint8_t *dbgFwLogFindEntry(struct FW_LOG_IDX_DATA *prFwLogIdx,
			uint32_t u4Offset, struct IDX_LOG_ENTRY *prLogEntry)
{
	uint8_t *prLogStr = NULL;
	uint32_t i, ucEndLen;

	if ((u4Offset >= prFwLogIdx->u4FwSize) || !prLogEntry)
		return NULL;

	/* FW Log BIN Entry :
	 * "$&""file name":"line info":"DBG Log String""0x00"
	 */

	prLogStr = &(((uint8_t *)prFwLogIdx->prFwBuffer)[u4Offset]);
	ucEndLen = prFwLogIdx->u4FwSize - u4Offset;

	/* parse the file name */
	prLogEntry->prFile = prLogStr;

	for (i = 0; i < ucEndLen; i++) {
		if (prLogStr[i] == ':')
			break;
	}

	if (i >= ucEndLen)
		return NULL;

	/* parse the line info */
	i++;
	prLogEntry->prLine = &prLogStr[i];

	for (; i < ucEndLen; i++) {
		if (prLogStr[i] == ':')
			break;
	}

	if (i >= ucEndLen)
		return NULL;

	/* parse the log string */
	i++;
	prLogEntry->prStr = &prLogStr[i];
	for (; i < ucEndLen; i++) {
		/* find the log string end (expect 0x0a, 0x00) */
		if (prLogStr[i] == 0x0) {
			prLogEntry->u4EntryLen = i + 1;
			prLogEntry->u4StrLen = &prLogStr[i] - prLogEntry->prStr;
			return prLogStr;
		}
	}

	return NULL;
}

int32_t dbgCheckTransText(uint8_t ucText)
{
	/* IDX format won't have '%s' & '%c' case */
	if ((ucText == 'd') || (ucText == 'u') || (ucText == 'o') ||
	    (ucText == 'p') || (ucText == 'x') || (ucText == 'X') ||
	    (ucText == 'f') || (ucText == 'e') || (ucText == 'E') ||
	    (ucText == 'g') || (ucText == 'G'))
		return 0;
	else if (ucText >= '0' && ucText <= '9')
		return 1;
	else if (ucText == 'l') /* for %lx case */
		return 1;
	else
		return -1;
}
uint8_t *dbgFwLogIdxToStr(struct IDX_LOG_ENTRY *prLogEntry,
				 struct IDX_LOG_V2_FORMAT *prIdxV2Header,
				 uint8_t *pucIdxLog,
				 uint8_t *aucLogBuf)
{
	uint8_t *prLogPtr;
#define PARAM_SIZE 8
	uint8_t aucFormat[PARAM_SIZE]; /* expect the "%0.X" < 7 chars */
	uint32_t i, j, l = 0, u4LogLen = 0;
	int32_t k;
	uint32_t *prArgu;
	uint32_t i4Ret;
	uint32_t u4HeadPos;
	uint8_t aucModule[PARAM_SIZE] = {0};
	uint8_t aucLevel[PARAM_SIZE] = {0};
	const char *prModule;
	const char *prLevel;

	prIdxV2Header = (struct IDX_LOG_V2_FORMAT *)pucIdxLog;
	prArgu = (uint32_t *)(pucIdxLog + sizeof(struct IDX_LOG_V2_FORMAT));
	prLogPtr = prLogEntry->prStr;

	if (prIdxV2Header->ucModId >= ARRAY_SIZE(apucFwDbgModule)) {
		kalSnprintf(aucModule, PARAM_SIZE, "%d",
			prIdxV2Header->ucModId);
		prModule = aucModule;
	} else {
		prModule = apucFwDbgModule[prIdxV2Header->ucModId];
	}

	if (prIdxV2Header->ucLevelId >= ARRAY_SIZE(apucFwDbgLvl)) {
		kalSnprintf(aucLevel, PARAM_SIZE, "%d",
			prIdxV2Header->ucLevelId);
		prLevel = aucLevel;
	} else {
		prLevel = apucFwDbgLvl[prIdxV2Header->ucLevelId];
	}

	u4LogLen = kalSnprintf(aucLogBuf, DBG_LOG_BUF_SIZE, "<FW> %s(%s):",
				prModule, prLevel);
	u4HeadPos = u4LogLen;

	for (i = 0; i < prLogEntry->u4StrLen; i++) {
		if (prLogPtr[i] == 0x0)
			break;

		if (u4LogLen >= DBG_LOG_BUF_SIZE)
			break;

		if (prLogPtr[i] != '%') {
			/* not special word */
			aucLogBuf[u4LogLen++] = prLogPtr[i];
		} else if ((i+1) >= prLogEntry->u4StrLen) {
			/* only 1 '%' in the end of log, not handle it */
			break;
		} else if (prLogPtr[i+1] == '%') {
			/* '%%' case */
			aucLogBuf[u4LogLen++] = '%';
			i += 1; /* for the 2nd '%' */
		} else { /* ex: %0...x */
			if (l >= prIdxV2Header->ucNumArgs) /* args mismatch */
				goto out;

			aucFormat[0] = '%';
			for (j = 1; (j < (prLogEntry->u4StrLen - i)) &&
					(j < (PARAM_SIZE - 1)); j++) {
				i4Ret = dbgCheckTransText(prLogPtr[i+j]);
				if (i4Ret == 0) {
					aucFormat[j] = prLogPtr[i+j];
					aucFormat[j+1] = 0x0;
					k = kalSnprintf(&aucLogBuf[u4LogLen],
						(DBG_LOG_BUF_SIZE - u4LogLen),
						aucFormat, prArgu[l++]);
					if (k < 0) /* buf is full */
						goto out;

					u4LogLen += k;
					i += j;
					break;
				} else if (i4Ret == 1) {
					aucFormat[j] = prLogPtr[i+j];
				} else {
					/* -1 : format error */
					u4LogLen = kalSnprintf(
					    &aucLogBuf[u4HeadPos],
					    (DBG_LOG_BUF_SIZE - u4HeadPos),
					    "%s",
					    prLogEntry->prStr);
					u4LogLen += u4HeadPos;
					goto out;
				}
			}
		}
	}

out:
	if (u4LogLen >= DBG_LOG_BUF_SIZE)
		u4LogLen = DBG_LOG_BUF_SIZE - 1;
	aucLogBuf[u4LogLen] = 0x0;

	return aucLogBuf;
}

uint32_t wlanFwLogIdxToStr(struct ADAPTER *prAdapter, uint8_t *pucIdxLog,
			   uint16_t u2MsgSize)
{
	struct IDX_LOG_V2_FORMAT *prIdxV2Header;
	struct TEXT_LOG_FORMAT *prTextLog;
#if 0
	struct CTRL_LOG_FORMAT *prCtrlLogHeader;
#endif
	struct IDX_LOG_ENTRY rLogEntry;
	uint8_t *prLogStr;
	uint8_t aucLogBuf[DBG_LOG_BUF_SIZE];

	if (!prAdapter->prFwLogIdx) {
		DBGLOG(INIT, STATE, "prFwLogIdx null\n");
		return WLAN_STATUS_FAILURE;
	}

	if (!pucIdxLog) {
		DBGLOG(INIT, STATE, "pointer null\n");
		return WLAN_STATUS_FAILURE;
	}

	/* min size should > sizeof(CTRL_LOG_LOST_FORMAT) 4 bytes */
	if (u2MsgSize < 4) {
		DBGLOG(INIT, STATE, "length error : %d\n", u2MsgSize);
		return WLAN_STATUS_FAILURE;
	}

#if 0
	DBGLOG(INIT, STATE, "EVT Content:\n");
	DBGLOG_MEM8(RSN, STATE, pucIdxLog, u2MsgSize);
#endif

	prIdxV2Header = (struct IDX_LOG_V2_FORMAT *)pucIdxLog;

	DBGLOG(INIT, TRACE, "STX=0x%X, VerType=%d, NumArgs=%d, ModId=%d, ",
		prIdxV2Header->ucStx, prIdxV2Header->ucVerType,
		prIdxV2Header->ucNumArgs, prIdxV2Header->ucModId);
	DBGLOG(INIT, TRACE, "SeqNo=0x%x, LvId=%d, TimeStamp=0x%x, IdxId=0x%x\n",
		prIdxV2Header->ucSeqNo, prIdxV2Header->ucLevelId,
		prIdxV2Header->u4TimeStamp, prIdxV2Header->u4IdxId);

	if (prIdxV2Header->ucStx != IDXLOGSTX) {
		uint8_t *pucChr;

		/* XXX: expect there is no this case  */
		DBGLOG(INIT, TRACE, "Not IDX Log!! ucStx=0x%x, type=0x%x\n",
			prIdxV2Header->ucStx, prIdxV2Header->ucVerType);

		pucIdxLog[u2MsgSize] = '\0';
		pucChr = kalStrChr(pucIdxLog, '\0');
		if (*(pucChr - 1) == '\n')
			*(pucChr - 1) = '\0';
		LOG_FUNC("<FW>%s\n", pucIdxLog);
		return WLAN_STATUS_SUCCESS;
	}

	if (prIdxV2Header->ucVerType == VER_TYPE_IDX_LOG_V2) {
		if ((prIdxV2Header->ucNumArgs * 4 +
		     sizeof(struct IDX_LOG_V2_FORMAT)) > u2MsgSize) {
			DBGLOG(INIT, WARN, "offset=0x%x, length err: %d:%d\n",
				prIdxV2Header->u4IdxId, u2MsgSize,
				(prIdxV2Header->ucNumArgs * 4 +
				 sizeof(struct IDX_LOG_V2_FORMAT)));
			return WLAN_STATUS_FAILURE;
		}

		/* find the related msg format entry in the bin */
		prLogStr = dbgFwLogFindEntry(prAdapter->prFwLogIdx,
					prIdxV2Header->u4IdxId, &rLogEntry);


		if (!prLogStr) {
			DBGLOG(INIT, STATE, "can't find log format\n");
			return WLAN_STATUS_FAILURE;
		}

		/* translate the fw log to string */
		dbgFwLogIdxToStr(&rLogEntry, prIdxV2Header, pucIdxLog,
				aucLogBuf);

		/* print the fw log  */
		LOG_FUNC("%s", aucLogBuf);
	} else if (prIdxV2Header->ucVerType == VER_TYPE_TXT_LOG) {
		prTextLog = (struct TEXT_LOG_FORMAT *)pucIdxLog;

		prLogStr = (uint8_t *)prTextLog +
				sizeof(struct TEXT_LOG_FORMAT);

		if ((prTextLog->ucPayloadSize_wo_padding +
			sizeof(struct TEXT_LOG_FORMAT)) > u2MsgSize) {
			DBGLOG(INIT, WARN, "error payload size (%d:%d:%d)\n",
				prTextLog->ucPayloadSize_wo_padding,
				prTextLog->ucPayloadSize_w_padding,
				u2MsgSize);
			return WLAN_STATUS_FAILURE;
		}

		/* Expect the rx text log last byte is '0x0a' */
		prLogStr[prTextLog->ucPayloadSize_wo_padding-1] = '\0';
		LOG_FUNC("<FW>%s", prLogStr);
	} else {
		/* TODO: process Time Sync Message here */

		DBGLOG(INIT, STATE, "Not supported type!! type=0x%x\n",
			prIdxV2Header->ucVerType);
	}

	return WLAN_STATUS_SUCCESS;
}

#endif /* CFG_SUPPORT_FW_IDX_LOG_TRANS */
