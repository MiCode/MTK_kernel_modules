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
#include "gl_kal.h"
#include "fw_log_parser.h"

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

static uint8_t *apucFwDbgModule[] = {
	(uint8_t *) DISP_STRING("HEM"),			/* 0 */
	(uint8_t *) DISP_STRING("AIS"),
	(uint8_t *) DISP_STRING("RLM"),
	(uint8_t *) DISP_STRING("MEM"),
	(uint8_t *) DISP_STRING("CNM"),
	(uint8_t *) DISP_STRING("RSN"),			/* 5 */
	(uint8_t *) DISP_STRING("RXM"),
	(uint8_t *) DISP_STRING("TXM"),
	(uint8_t *) DISP_STRING("BSS"),
	(uint8_t *) DISP_STRING("EVENT"),
	(uint8_t *) DISP_STRING("SCN"),			/* 10 */
	(uint8_t *) DISP_STRING("SAA"),
	(uint8_t *) DISP_STRING("AAA"),
	(uint8_t *) DISP_STRING("MGT"),
	(uint8_t *) DISP_STRING("INIT"),
	(uint8_t *) DISP_STRING("REQ"),			/* 15 */
	(uint8_t *) DISP_STRING("WAPI"),
	(uint8_t *) DISP_STRING("NIC"),
	(uint8_t *) DISP_STRING("HAL"),
	(uint8_t *) DISP_STRING("COEX"),
	(uint8_t *) DISP_STRING("PM"),			/* 20 */
	(uint8_t *) DISP_STRING("LP"),
	(uint8_t *) DISP_STRING("RFTEST"),
	(uint8_t *) DISP_STRING("ICAP"),
	(uint8_t *) DISP_STRING("RRM"),
	(uint8_t *) DISP_STRING("MQM"),			/* 25 */
	(uint8_t *) DISP_STRING("RDM"),
	(uint8_t *) DISP_STRING("P2P"),
	(uint8_t *) DISP_STRING("RLM_AR"),
	(uint8_t *) DISP_STRING("AR"),
	(uint8_t *) DISP_STRING("ROAMING"),		/* 30 */
	(uint8_t *) DISP_STRING("RTT"),
	(uint8_t *) DISP_STRING("TDLS"),
	(uint8_t *) DISP_STRING("BF"),
	(uint8_t *) DISP_STRING("MU"),
	(uint8_t *) DISP_STRING("SLT"),			/* 35 */
	(uint8_t *) DISP_STRING("CMD"),
	(uint8_t *) DISP_STRING("RA"),
	(uint8_t *) DISP_STRING("PF"),
	(uint8_t *) DISP_STRING("WTBL"),
	(uint8_t *) DISP_STRING("SER"),			/* 40 */
	(uint8_t *) DISP_STRING("WH_HIF"),
	(uint8_t *) DISP_STRING("TP"),
	(uint8_t *) DISP_STRING("SUSPEND_MODE"),
	(uint8_t *) DISP_STRING("DBDC"),
	(uint8_t *) DISP_STRING("ISR"),			/* 45 */
	(uint8_t *) DISP_STRING("HWCFG"),
	(uint8_t *) DISP_STRING("VOW"),
	(uint8_t *) DISP_STRING("EMUL"),
	(uint8_t *) DISP_STRING("ARP_NS"),
	(uint8_t *) DISP_STRING("TIMER"),		/* 50 */
	(uint8_t *) DISP_STRING("STAREC"),
	(uint8_t *) DISP_STRING("THERMAL"),
	(uint8_t *) DISP_STRING("TXPOWER"),
	(uint8_t *) DISP_STRING("PKT_DROP"),
	(uint8_t *) DISP_STRING("STATS"),		/* 55 */
	(uint8_t *) DISP_STRING("RSSI_MONITOR"),
	(uint8_t *) DISP_STRING("WFC_KEEP_ALIVE"),
	(uint8_t *) DISP_STRING("TB"),
	(uint8_t *) DISP_STRING("TXCMD"),
	(uint8_t *) DISP_STRING("DYNSND"),		/* 60 */
	(uint8_t *) DISP_STRING("SPE"),
	(uint8_t *) DISP_STRING("EXT_TXCMD"),
	(uint8_t *) DISP_STRING("EXT_CMDRPT_TX"),
	(uint8_t *) DISP_STRING("EXT_CMDRPT_TRIG"),
	(uint8_t *) DISP_STRING("EXT_SPL"),		/* 65 */
	(uint8_t *) DISP_STRING("EXT_RXRPT"),
	(uint8_t *) DISP_STRING("EXT_TXV"),
	(uint8_t *) DISP_STRING("EXT_HERA"),
	(uint8_t *) DISP_STRING("SCS"),
	(uint8_t *) DISP_STRING("COANT"),		/* 70 */
	(uint8_t *) DISP_STRING("ECC"),
	(uint8_t *) DISP_STRING("MURU"),
	(uint8_t *) DISP_STRING("TAM"),
	(uint8_t *) DISP_STRING("SR"),
	(uint8_t *) DISP_STRING("RXV"),			/* 75 */
	(uint8_t *) DISP_STRING("CONNCOEX"),
	(uint8_t *) DISP_STRING("MDVT"),
	(uint8_t *) DISP_STRING("CAL"),
	(uint8_t *) DISP_STRING("EXT_RANGE"),
	(uint8_t *) DISP_STRING("AHDBG"),		/* 80 */
	(uint8_t *) DISP_STRING("DPP"),
	(uint8_t *) DISP_STRING("NAN"),
	(uint8_t *) DISP_STRING("X"),
	(uint8_t *) DISP_STRING("X"),
	(uint8_t *) DISP_STRING("X"),			/* 85 */
	(uint8_t *) DISP_STRING("X"),
	(uint8_t *) DISP_STRING("X"),
	(uint8_t *) DISP_STRING("X"),
	(uint8_t *) DISP_STRING("X"),
	(uint8_t *) DISP_STRING("X"),			/* 90 */
	(uint8_t *) DISP_STRING("X"),
	(uint8_t *) DISP_STRING("X"),
	(uint8_t *) DISP_STRING("X"),
	(uint8_t *) DISP_STRING("X"),
	(uint8_t *) DISP_STRING("X"),			/* 95 */
	(uint8_t *) DISP_STRING("X"),
	(uint8_t *) DISP_STRING("X"),
	(uint8_t *) DISP_STRING("X"),
	(uint8_t *) DISP_STRING("X"),
	(uint8_t *) DISP_STRING("X"),			/* 100 */
	(uint8_t *) DISP_STRING("X"),
	(uint8_t *) DISP_STRING("X"),
	(uint8_t *) DISP_STRING("X"),
	(uint8_t *) DISP_STRING("X"),
	(uint8_t *) DISP_STRING("X"),			/* 105 */
	(uint8_t *) DISP_STRING("X"),
	(uint8_t *) DISP_STRING("X"),
	(uint8_t *) DISP_STRING("X"),
	(uint8_t *) DISP_STRING("X"),
	(uint8_t *) DISP_STRING("X"),			/* 110 */
	(uint8_t *) DISP_STRING("X"),
	(uint8_t *) DISP_STRING("X"),
	(uint8_t *) DISP_STRING("ANTSWAP"),
};

static uint8_t *apucFwDbgLvl[] = {
	(uint8_t *) DISP_STRING("ERROR"),
	(uint8_t *) DISP_STRING("WARN"),
	(uint8_t *) DISP_STRING("STATE"),
	(uint8_t *) DISP_STRING("INFO"),
	(uint8_t *) DISP_STRING("LOUD"),
	(uint8_t *) DISP_STRING("EVENT"),		/* 5 */
	(uint8_t *) DISP_STRING("TRACE"),
};

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

uint32_t wlanOpenIdxLogBin(
	struct ADAPTER *prAdapter
)
{
	void *prFwBuffer = NULL;
	struct FW_LOG_IDX_DATA *prIdxLogBin = NULL;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct mt66xx_chip_info *prChipInfo = NULL;
	uint8_t *apucName[2] = {0};
	uint8_t ucNameBody[CFG_FW_NAME_MAX_LEN] = {0};
	uint32_t u4FwSize = 0;
	uint32_t u4Ret = WLAN_STATUS_FAILURE;

	DBGLOG(INIT, STATE, "open wlanOpenIdxLogBin\n");

	if (!prAdapter) {
		DBGLOG(INIT, ERROR, "prAdapter NULL\n");
		goto out;
	}

	prGlueInfo = prAdapter->prGlueInfo;
	prChipInfo = prAdapter->chip_info;

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

	/* <1> Open firmware */
	apucName[0] = ucNameBody;
	prChipInfo->fw_dl_ops->constrcutIdxLogBin(prGlueInfo, apucName);

	if (kalFirmwareOpen(prGlueInfo, apucName) !=
		WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, WARN, "Can't open %s\n", apucName[0]);
		goto out;
	}

	/* <2> Query firmware size */
	kalFirmwareSize(prGlueInfo, &u4FwSize);

	DBGLOG(INIT, TRACE, "FW idx bin path=%s, size=%u\n",
		apucName[0], u4FwSize);

	/* <3> Use vmalloc for allocating large memory trunk */
	prFwBuffer = vmalloc(ALIGN_4(u4FwSize));

	if (prFwBuffer == NULL) {
		DBGLOG(INIT, ERROR, "Can't alloc buf %u\n", ALIGN_4(u4FwSize));
		goto close_firm;
	}

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
	prAdapter->fgFwLogBinLoad = TRUE;

	u4Ret = WLAN_STATUS_SUCCESS;

	DBGLOG(INIT, TRACE, "Open wlanOpenIdxLogBin success\n");

	goto close_firm;

err_free:
	vfree(prFwBuffer);
close_firm:
	kalFirmwareClose(prGlueInfo);
out:
	return u4Ret;
}

void wlanCloseIdxLogBin(
	struct ADAPTER *prAdapter
)
{
	struct FW_LOG_IDX_DATA *prIdxLogBin = NULL;

	DBGLOG(INIT, STATE, "Close wlanCloseIdxLogBin\n");

	if (!prAdapter || !prAdapter->prFwLogIdx) {
		DBGLOG(INIT, ERROR, "prAdapter NULL\n");
		return;
	}

	prIdxLogBin = prAdapter->prFwLogIdx;

	prAdapter->prFwLogIdx = NULL;
	prAdapter->fgFwLogBinLoad = FALSE;

	if (prIdxLogBin->prFwBuffer != NULL)
		vfree(prIdxLogBin->prFwBuffer);

	kalMemFree(prIdxLogBin, VIR_MEM_TYPE, sizeof(struct FW_LOG_IDX_DATA));

	DBGLOG(INIT, TRACE, "Close wlanCloseIdxLogBin success\n");
}

void dbgFwLogDebugPrint(
	uint8_t *pucIdxLog,
	uint16_t u2MsgSize,
	struct IDX_LOG_ENTRY *prLogEntry
)
{
	uint16_t u2Idx = 0;

	/* Log msg data */
	if (pucIdxLog) {

		DBGLOG(INIT, ERROR, "Log msg data(len=%u):", u2MsgSize);

		for (u2Idx = 0; u2Idx < u2MsgSize; u2Idx++) {
			if ((u2Idx % 16) == 0)
				DBGLOG(INIT, ERROR, "\n");

			DBGLOG(INIT, ERROR, "%x ",
				(uint8_t)pucIdxLog[u2Idx]);
		}
	}

	/* Log format body */
	if (prLogEntry) {

		DBGLOG(INIT, ERROR,
			"Log format string: %s\n", (char *)prLogEntry->prFile);

		DBGLOG(INIT, ERROR,
			"Log format data(len=%u):", prLogEntry->u4EntryLen);

		for (u2Idx = 0; u2Idx < prLogEntry->u4EntryLen; u2Idx++) {
			if ((u2Idx % 16) == 0)
				DBGLOG(INIT, ERROR, "\n");
			DBGLOG(INIT, ERROR, "%x ",
				(uint8_t)prLogEntry->prFile[u2Idx]);
		}
	}
}

bool dbgFwLogFindEntry(
	struct FW_LOG_IDX_DATA *prFwLogIdx,
	uint32_t u4Offset,
	struct IDX_LOG_ENTRY *prLogEntry
)
{
	uint8_t *prLogStr = NULL;
	uint32_t i = 0, u4EndLen = 0;

	if (!prLogEntry || !prFwLogIdx || !prFwLogIdx->prFwBuffer)
		return FALSE;

	DBGLOG(INIT, TRACE, "u4Offset=%u(0x%x), u4FwSize=%u\n",
		u4Offset, u4Offset, prFwLogIdx->u4FwSize);

	if (u4Offset >= prFwLogIdx->u4FwSize) {
		DBGLOG(INIT, ERROR, "wrong offset=0x%x\n", u4Offset);
		return FALSE;
	}

	/* FW Log BIN Entry :
	 * "$&""file name":"line info":"DBG Log String""0x00"
	 */

	prLogStr = &(((uint8_t *)prFwLogIdx->prFwBuffer)[u4Offset]);
	u4EndLen = prFwLogIdx->u4FwSize - u4Offset;

	/* parse the file name */
	prLogEntry->prFile = prLogStr;

	for (i = 0; i < u4EndLen; i++) {
		if (prLogStr[i] == ':')
			break;
	}

	if (i >= u4EndLen) {
		DBGLOG(INIT, ERROR, "File name parse fail\n");
		return FALSE;
	}

	/* parse the line info */
	i++;
	prLogEntry->prLine = &prLogStr[i];

	for (; i < u4EndLen; i++) {
		if (prLogStr[i] == ':')
			break;
	}

	if (i >= u4EndLen) {
		DBGLOG(INIT, ERROR, "Line info parse fail\n");
		return FALSE;
	}

	/* parse the log string */
	i++;
	prLogEntry->prStr = &prLogStr[i];
	for (; i < u4EndLen; i++) {

		/* find the log string end (expect 0x0a, 0x00) */
		if (prLogStr[i] == 0x0) {

			prLogEntry->u4EntryLen = i + 1;
			prLogEntry->u4StrLen = &prLogStr[i] - prLogEntry->prStr;

			return TRUE;
		}
	}

	return FALSE;
}

int32_t dbgCheckTransText(
	uint8_t ucText
)
{
	DBGLOG(INIT, TRACE,
		"dbgCheckTransText: '%c'(0x%x)\n", (char)ucText, ucText);

	/* IDX format won't have '%s' & '%c' case */
	if ((ucText == 'd') || (ucText == 'u') || (ucText == 'o') ||
		(ucText == 'p') || (ucText == 'x') || (ucText == 'X') ||
		(ucText == 'i'))
#if 0
		(ucText == 'f') || (ucText == 'e') || (ucText == 'E') ||
		(ucText == 'g') || (ucText == 'G'))
#endif
		return 0;

	/* for %lx/%llx case */
	else if (ucText == 'l')
		return 1;

	/* for %hx/%hhx case */
	else if (ucText == 'h')
		return 1;

	/* for %zu case */
	else if (ucText == 'z')
		return 1;

	/* numbers */
	else if ((ucText >= '0') && (ucText <= '9'))
		return 1;

	/* Error using %s %c */
	else if ((ucText == 's') || (ucText == 'c'))
		return -2;

	else
		return -1;
}

#define PARAM_SIZE 8

int32_t dbgFormatToString(
	uint8_t *pucFormat,
	uint8_t ucFormatSize,
	uint32_t value,
	uint8_t *pucLogBuf,
	uint8_t ucLogBufSize
)
{
	/*
	 *	k = kalSnprintf(&aucLogBuf[u4LogLen],
	 *		(DBG_LOG_BUF_SIZE - u4LogLen),
	 *		aucFormat, prArgu[l++]);
	 */

	uint8_t ucRightJst = 0;
	uint8_t ucFormatIdx = 0;
	bool fgZeroAlign = FALSE;

	DBGLOG(INIT, TRACE, "pucFormat=%s\n", (char *)pucFormat);
	DBGLOG(INIT, TRACE, "value=0x%x\n", value);

	if (pucFormat[0] != '%') {
		DBGLOG(INIT, ERROR, "Format not start with '%%'\n");
		return -1;
	}

	if (ucFormatSize > PARAM_SIZE || ucFormatSize < 2) {
		DBGLOG(INIT, ERROR,
			"Wrong format size=%u\n", (uint8_t)ucFormatSize);
		return -1;
	}

	/* Parse format of sprintf */
	for (ucFormatIdx = 1; ucFormatIdx < ucFormatSize; ucFormatIdx++) {

		DBGLOG(INIT, TRACE, "pucFormat[%u]='%c'(0x%x)\n", ucFormatIdx,
			(char)pucFormat[ucFormatIdx], pucFormat[ucFormatIdx]);

		if ((ucFormatIdx == 1) && (pucFormat[ucFormatIdx] == '0')) {
			DBGLOG(INIT, TRACE, "zero Align\n");
			fgZeroAlign = TRUE;
			continue;

		} else if (pucFormat[ucFormatIdx] >= '0' &&
			pucFormat[ucFormatIdx] <= '9') {

			/* Justiced value */
			ucRightJst *= 10;
			ucRightJst += (uint8_t)(pucFormat[ucFormatIdx] - '0');

			DBGLOG(INIT, TRACE, "ucRightJst=%u\n", ucRightJst);

		} else if (pucFormat[ucFormatIdx] == 'z') {

			if ((ucFormatIdx+1) > (ucFormatSize-1)) {
				DBGLOG(INIT, ERROR,
					"Wrong format size=%u:%u\n",
					(uint8_t)(ucFormatIdx+1),
					(uint8_t)ucFormatSize);
				return -1;
			}

			if (pucFormat[ucFormatIdx+1] == 'u') {

				/* %zu */
				DBGLOG(INIT, TRACE,
					"pucFormat[%u]='%c'(0x%x), [%u]='%c'(0x%x)\n",
					(uint8_t)(ucFormatIdx),
					(char)pucFormat[ucFormatIdx],
					pucFormat[ucFormatIdx],
					(uint8_t)(ucFormatIdx+1),
					(char)pucFormat[ucFormatIdx+1],
					pucFormat[ucFormatIdx+1]);

				return kalSnprintf(pucLogBuf, ucLogBufSize,
						(fgZeroAlign)?"%0*zu":"%*zu",
						ucRightJst,
						(size_t)value);

			} else {
				DBGLOG(INIT, ERROR, "Format specifier error\n");
				return -1;
			}

		} else if (pucFormat[ucFormatIdx] == 'l') {

			if ((ucFormatIdx+1) > (ucFormatSize-1)) {
				DBGLOG(INIT, ERROR,
					"Wrong format size=%u:%u\n",
					(uint8_t)(ucFormatIdx+1),
					(uint8_t)ucFormatSize);
				return -1;
			}

			/* %li, %ld, %lo, %lu, %lx, %lX */
			DBGLOG(INIT, TRACE,
				"pucFormat[%u]='%c'(0x%x)\n",
				(uint8_t)(ucFormatIdx+1),
				(char)pucFormat[ucFormatIdx+1],
				pucFormat[ucFormatIdx+1]);


			/* %lli, %lld, %llo, %llu, %llx, %llX */
			if (pucFormat[ucFormatIdx+1] == 'l') {

				if ((ucFormatIdx+2) > (ucFormatSize-1)) {
					DBGLOG(INIT, ERROR,
						"Wrong format size=%u:%u\n",
						(uint8_t)(ucFormatIdx+2),
						(uint8_t)ucFormatSize);
					return -1;
				}

				DBGLOG(INIT, TRACE,
					"pucFormat[%u]='%c'(0x%x)\n",
					(uint8_t)(ucFormatIdx+2),
					(char)pucFormat[ucFormatIdx+2],
					pucFormat[ucFormatIdx+2]);

				if (pucFormat[ucFormatIdx+2] == 'i') {
					return kalSnprintf(pucLogBuf,
						ucLogBufSize,
						(fgZeroAlign)?"%0*lli":"%*lli",
						ucRightJst,
						(long long)value);

				} else if (pucFormat[ucFormatIdx+2] == 'd') {
					return kalSnprintf(pucLogBuf,
						ucLogBufSize,
						(fgZeroAlign)?"%0*lld":"%*lld",
						ucRightJst,
						(long long)value);

				} else if (pucFormat[ucFormatIdx+2] == 'o') {
					return kalSnprintf(pucLogBuf,
						ucLogBufSize,
						(fgZeroAlign)?"%0*llo":"%*llo",
						ucRightJst,
						(unsigned long long)value);

				} else if (pucFormat[ucFormatIdx+2] == 'u') {
					return kalSnprintf(pucLogBuf,
						ucLogBufSize,
						(fgZeroAlign)?"%0*llu":"%*llu",
						ucRightJst,
						(unsigned long long)value);

				} else if (pucFormat[ucFormatIdx+2] == 'x') {
					return kalSnprintf(pucLogBuf,
						ucLogBufSize,
						(fgZeroAlign)?"%0*llx":"%*llx",
						ucRightJst,
						(unsigned long long)value);

				} else if (pucFormat[ucFormatIdx+2] == 'X') {
					return kalSnprintf(pucLogBuf,
						ucLogBufSize,
						(fgZeroAlign)?"%0*llX":"%*llX",
						ucRightJst,
						(unsigned long long)value);
				} else {
					DBGLOG(INIT, ERROR,
						"Format specifier error\n");
					return -1;
				}

			/* %li, %ld, %lo, %lu, %lx, %lX */
			} else if (pucFormat[ucFormatIdx+1] == 'i') {
				long tmp = 0;

				/* Casting to i32 to decide sign of number */
				if (sizeof(long) > sizeof(uint32_t))
					tmp = (long)((int)value);
				else
					tmp = (long)value;

				return kalSnprintf(pucLogBuf, ucLogBufSize,
						(fgZeroAlign)?"%0*li":"%*li",
						ucRightJst, tmp);

			} else if (pucFormat[ucFormatIdx+1] == 'd') {
				long tmp = 0;

				/* Casting to i32 to decide sign of number */
				if (sizeof(long) > sizeof(uint32_t))
					tmp = (long)((int)value);
				else
					tmp = (long)value;

				return kalSnprintf(pucLogBuf, ucLogBufSize,
						(fgZeroAlign)?"%0*ld":"%*ld",
						ucRightJst, tmp);

			} else if (pucFormat[ucFormatIdx+1] == 'o') {
				return kalSnprintf(pucLogBuf, ucLogBufSize,
						(fgZeroAlign)?"%0*lo":"%*lo",
						ucRightJst,
						(unsigned long)value);

			} else if (pucFormat[ucFormatIdx+1] == 'u') {
				return kalSnprintf(pucLogBuf, ucLogBufSize,
						(fgZeroAlign)?"%0*lu":"%*lu",
						ucRightJst,
						(unsigned long)value);

			} else if (pucFormat[ucFormatIdx+1] == 'x') {
				return kalSnprintf(pucLogBuf, ucLogBufSize,
						(fgZeroAlign)?"%0*lx":"%*lx",
						ucRightJst,
						(unsigned long)value);

			} else if (pucFormat[ucFormatIdx+1] == 'X') {
				return kalSnprintf(pucLogBuf, ucLogBufSize,
						(fgZeroAlign)?"%0*lX":"%*lX",
						ucRightJst,
						(unsigned long)value);

			} else {
				DBGLOG(INIT, ERROR, "Format specifier error\n");
				return -1;
			}

		} else if (pucFormat[ucFormatIdx] == 'h') {

			if ((ucFormatIdx+1) > (ucFormatSize-1)) {
				DBGLOG(INIT, ERROR,
					"Wrong format size=%u:%u\n",
					(uint8_t)(ucFormatIdx+1),
					(uint8_t)ucFormatSize);
				return -1;
			}

			DBGLOG(INIT, TRACE,
				"pucFormat[%u]='%c'(0x%x)\n",
				(uint8_t)(ucFormatIdx+1),
				(char)pucFormat[ucFormatIdx+1],
				pucFormat[ucFormatIdx+1]);

			/* %hhi, %hhd, %hho, %hhu, %hhx, %hhX */
			if (pucFormat[ucFormatIdx+1] == 'h') {

				if ((ucFormatIdx+2) > (ucFormatSize-1)) {
					DBGLOG(INIT, ERROR,
						"Wrong format size=%u:%u\n",
						(uint8_t)(ucFormatIdx+2),
						(uint8_t)ucFormatSize);
					return -1;
				}

				DBGLOG(INIT, TRACE,
					"pucFormat[%u]='%c'(0x%x)\n",
					(uint8_t)(ucFormatIdx+2),
					(char)pucFormat[ucFormatIdx+2],
					pucFormat[ucFormatIdx+2]);

				if (pucFormat[ucFormatIdx+2] == 'i') {
					return kalSnprintf(pucLogBuf,
						ucLogBufSize,
						(fgZeroAlign)?"%0*hhi":"%*hhi",
						ucRightJst, (signed char)value);

				} else if (pucFormat[ucFormatIdx+2] == 'd') {
					return kalSnprintf(pucLogBuf,
						ucLogBufSize,
						(fgZeroAlign)?"%0*hhd":"%*hhd",
						ucRightJst, (signed char)value);

				} else if (pucFormat[ucFormatIdx+2] == 'o') {
					return kalSnprintf(pucLogBuf,
						ucLogBufSize,
						(fgZeroAlign)?"%0*hho":"%*hho",
						ucRightJst,
						(unsigned char)value);

				} else if (pucFormat[ucFormatIdx+2] == 'u') {
					return kalSnprintf(pucLogBuf,
						ucLogBufSize,
						(fgZeroAlign)?"%0*hhu":"%*hhu",
						ucRightJst,
						(unsigned char)value);

				} else if (pucFormat[ucFormatIdx+2] == 'x') {
					return kalSnprintf(pucLogBuf,
						ucLogBufSize,
						(fgZeroAlign)?"%0*hhx":"%*hhx",
						ucRightJst,
						(unsigned char)value);

				} else if (pucFormat[ucFormatIdx+2] == 'X') {
					return kalSnprintf(pucLogBuf,
						ucLogBufSize,
						(fgZeroAlign)?"%0*hhX":"%*hhX",
						ucRightJst,
						(unsigned char)value);
				} else {
					DBGLOG(INIT, ERROR,
						"Format specifier error\n");
					return -1;
				}

			/* %hi, %hd, %ho, %hu, %hx, %hX */
			} else if (pucFormat[ucFormatIdx+1] == 'i') {
				return kalSnprintf(pucLogBuf, ucLogBufSize,
						(fgZeroAlign)?"%0*li":"%*li",
						ucRightJst,
						(long)value);

			} else if (pucFormat[ucFormatIdx+1] == 'd') {
				return kalSnprintf(pucLogBuf, ucLogBufSize,
						(fgZeroAlign)?"%0*ld":"%*ld",
						ucRightJst,
						(long)value);

			} else if (pucFormat[ucFormatIdx+1] == 'o') {
				return kalSnprintf(pucLogBuf, ucLogBufSize,
						(fgZeroAlign)?"%0*lo":"%*lo",
						ucRightJst,
						(unsigned long)value);

			} else if (pucFormat[ucFormatIdx+1] == 'u') {
				return kalSnprintf(pucLogBuf, ucLogBufSize,
						(fgZeroAlign)?"%0*lu":"%*lu",
						ucRightJst,
						(unsigned long)value);

			} else if (pucFormat[ucFormatIdx+1] == 'x') {
				return kalSnprintf(pucLogBuf, ucLogBufSize,
						(fgZeroAlign)?"%0*lx":"%*lx",
						ucRightJst,
						(unsigned long)value);

			} else if (pucFormat[ucFormatIdx+1] == 'X') {
				return kalSnprintf(pucLogBuf, ucLogBufSize,
						(fgZeroAlign)?"%0*lX":"%*lX",
						ucRightJst,
						(unsigned long)value);

			} else {
				DBGLOG(INIT, ERROR, "Format specifier error\n");
				return -1;
			}


		/* %i, %d, %o, %u, %p, %x, %X */
		} else if (pucFormat[ucFormatIdx] == 'i') {
			return kalSnprintf(pucLogBuf, ucLogBufSize,
						(fgZeroAlign)?"%0*i":"%*i",
						ucRightJst, (int)value);

		} else if (pucFormat[ucFormatIdx] == 'd') {
			return kalSnprintf(pucLogBuf, ucLogBufSize,
						(fgZeroAlign)?"%0*d":"%*d",
						ucRightJst, (int)value);

		} else if (pucFormat[ucFormatIdx] == 'u') {
			return kalSnprintf(pucLogBuf, ucLogBufSize,
						(fgZeroAlign)?"%0*u":"%*u",
						ucRightJst, value);

		} else if (pucFormat[ucFormatIdx] == 'o') {
			return kalSnprintf(pucLogBuf, ucLogBufSize,
						(fgZeroAlign)?"%0*o":"%*o",
						ucRightJst, value);

		} else if (pucFormat[ucFormatIdx] == 'p') {
			return kalSnprintf(pucLogBuf, ucLogBufSize,
						"0x%*x", ucRightJst, value);

		} else if (pucFormat[ucFormatIdx] == 'x') {
			return kalSnprintf(pucLogBuf, ucLogBufSize,
						(fgZeroAlign)?"%0*x":"%*x",
						ucRightJst, value);

		} else if (pucFormat[ucFormatIdx] == 'X') {
			return kalSnprintf(pucLogBuf, ucLogBufSize,
						(fgZeroAlign)?"%0*X":"%*X",
						ucRightJst, value);

		} else {
			DBGLOG(INIT, ERROR, "Format error\n");
			return -1;
		}

	}

	DBGLOG(INIT, ERROR, "Format parse fail\n");
	return -1;
}

uint32_t dbgFwLogIdxToStr(
	struct IDX_LOG_ENTRY *prLogEntry,
	struct IDX_LOG_V2_FORMAT *prIdxV2Header,
	uint8_t *pucIdxLog,
	uint8_t *aucLogBuf
)
{
	uint8_t *prLogPtr = NULL;
	uint8_t aucFormat[PARAM_SIZE]; /* expect the "%0.X" < 7 chars */
	uint32_t i = 0, j = 0, l = 0, u4LogLen = 0;
	int32_t k = 0;
	uint32_t *prArgu = NULL;
	int32_t i4Ret = 0;
	uint32_t u4HeadPos = 0;
	uint8_t aucModule[PARAM_SIZE] = {0};
	uint8_t aucLevel[PARAM_SIZE] = {0};
	uint8_t *prModule = NULL;
	uint8_t *prLevel = NULL;

	if (!pucIdxLog) {
		DBGLOG(INIT, ERROR, "pucIdxLog NULL\n");
		goto out;
	}

	if (!prLogEntry) {
		DBGLOG(INIT, ERROR, "prLogEntry NULL\n");
		goto out;
	}

	prIdxV2Header = (struct IDX_LOG_V2_FORMAT *)pucIdxLog;
	prArgu = (uint32_t *)(pucIdxLog + sizeof(struct IDX_LOG_V2_FORMAT));
	prLogPtr = prLogEntry->prStr;


	/* Get mod and level from header */
	if (prIdxV2Header->ucModId >=
		sizeof(apucFwDbgModule)/sizeof(uint8_t *)) {
		kalSnprintf(aucModule, PARAM_SIZE, "%u",
			(uint8_t)prIdxV2Header->ucModId);
		prModule = aucModule;
	} else {
		prModule = apucFwDbgModule[prIdxV2Header->ucModId];
	}

	if (prIdxV2Header->ucLevelId >= ARRAY_SIZE(apucFwDbgLvl)) {
		kalSnprintf(aucLevel, PARAM_SIZE, "%u",
			(uint8_t)prIdxV2Header->ucLevelId);
		prLevel = aucLevel;
	} else {
		prLevel = apucFwDbgLvl[prIdxV2Header->ucLevelId];
	}

	DBGLOG(INIT, TRACE, "prModule=%s, prLevel=%s\n",
		(char *)prModule, (char *)prLevel);

	u4HeadPos = kalSnprintf(aucLogBuf, DBG_LOG_BUF_SIZE, "<FW> %s(%s):",
				prModule, prLevel);
	u4LogLen = u4HeadPos;

	DBGLOG(INIT, TRACE, "u4HeadPos=%u\n", u4HeadPos);


	/* Parse log body */
	for (i = 0; i < prLogEntry->u4StrLen; i++) {

		DBGLOG(INIT, TRACE, "prLogPtr[%u]='%c'(0x%x)\n",
			i, (char)prLogPtr[i], prLogPtr[i]);

		if (prLogPtr[i] == 0x0) {
			DBGLOG(INIT, TRACE, "End of log\n");
			break;
		}

		if (u4LogLen >= DBG_LOG_BUF_SIZE) {
			DBGLOG(INIT, TRACE, "Log too long\n");
			break;
		}

		/* not special word */
		if (prLogPtr[i] != '%') {
			aucLogBuf[u4LogLen++] = prLogPtr[i];

		} else if ((i+1) >= prLogEntry->u4StrLen) {

			/* only 1 '%' in the end of log, not handle it */
			DBGLOG(INIT, TRACE, "'%%' in the end of log\n");
			break;

		} else if (prLogPtr[i+1] == '%') {

			/* '%%' case */
			aucLogBuf[u4LogLen++] = '%';
			i += 1;
			DBGLOG(INIT, TRACE, "2 '%%'\n");

		} else {

			/* ex: %0...x */
			if (l > prIdxV2Header->ucNumArgs) {
				DBGLOG(INIT, ERROR,
					"Argument number mismatch: %u\n",
					(uint8_t)prIdxV2Header->ucNumArgs);
				goto out;
			}

			kalMemZero(aucFormat, PARAM_SIZE * sizeof(uint8_t));

			aucFormat[0] = '%';
			for (j = 1; (j < (prLogEntry->u4StrLen - i)) &&
					(j < (PARAM_SIZE - 1)); j++) {

				i4Ret = dbgCheckTransText(prLogPtr[i+j]);

				DBGLOG(INIT, TRACE, "i4Ret: %d\n", i4Ret);

				if (i4Ret == 0) {
					aucFormat[j] = prLogPtr[i+j];
					aucFormat[j+1] = 0x0;

					DBGLOG(INIT, TRACE,
						"Argu[%u]=0x%x %x %x %x\n",
						l,
						aucLogBuf[u4LogLen],
						aucLogBuf[u4LogLen+1],
						aucLogBuf[u4LogLen+2],
						aucLogBuf[u4LogLen+3]);

					/* Re-write for format-nonliteral */
					k = dbgFormatToString(aucFormat, j+1,
						prArgu[l++],
						&aucLogBuf[u4LogLen],
						(DBG_LOG_BUF_SIZE - u4LogLen));

					if (k < 0) {
						DBGLOG(INIT, ERROR,
						    "Format to string fail\n");
						goto out;
					}

					u4LogLen += k;
					i += j;
					break;

				} else if (i4Ret == 1) {

					aucFormat[j] = prLogPtr[i+j];
					continue;

				} else if (i4Ret == -2) {

					/* Error using %s, %c for log idx v2 */
					DBGLOG(INIT, ERROR,
					    "Error using %%s, %%c in log idx\n");

					/* Output all log body */
					k = (uint32_t)kalSnprintf(
					    &aucLogBuf[u4HeadPos],
					    (DBG_LOG_BUF_SIZE - u4HeadPos),
					    "%s",
					    prLogEntry->prStr);

					if (k < 0) {
						DBGLOG(INIT, ERROR,
							"Snprintf fail\n");
						goto out;
					}

					u4LogLen += k;
					break;
				}

				/* else case */
				/* Other invalid char after '%' */
				DBGLOG(INIT, ERROR,
					"other invalid char after '%%'\n");

				/* Output all log body */
				k = (uint32_t)kalSnprintf(
					&aucLogBuf[u4HeadPos],
					(DBG_LOG_BUF_SIZE - u4HeadPos),
					"%s",
					prLogEntry->prStr);

				if (k < 0) {
					DBGLOG(INIT, ERROR, "Snprintf fail\n");
					goto out;
				}

				u4LogLen += k;
				break;
			}
		}
	}

out:
	if (u4LogLen >= DBG_LOG_BUF_SIZE)
		u4LogLen = DBG_LOG_BUF_SIZE - 1;
	aucLogBuf[u4LogLen] = 0x0;
	u4LogLen++;

	return u4LogLen;
}

uint32_t wlanFwLogIdxToStr(
	struct ADAPTER *prAdapter,
	uint8_t *pucIdxLog,
	uint16_t u2MsgSize
)
{
	struct IDX_LOG_V2_FORMAT *prIdxV2Header = NULL;
	struct TEXT_LOG_FORMAT *prTextLog = NULL;
#if 0
	struct CTRL_LOG_FORMAT *prCtrlLogHeader;
#endif
	struct IDX_LOG_ENTRY rLogEntry = {0};
	uint8_t aucLogBuf[DBG_LOG_BUF_SIZE] = {0};
	uint32_t rStatus = WLAN_STATUS_FAILURE;

	DBGLOG(INIT, TRACE, "wlanFwLogIdxToStr\n");

	if (!prAdapter || !prAdapter->prFwLogIdx) {
		DBGLOG(INIT, ERROR, "prFwLogIdx null\n");
		return WLAN_STATUS_FAILURE;
	}

	if (!pucIdxLog) {
		DBGLOG(INIT, ERROR, "pointer null\n");
		return WLAN_STATUS_FAILURE;
	}

	/* min size should >= sizeof(CTRL_LOG_LOST_FORMAT) 8 bytes */
	if (u2MsgSize < sizeof(struct CTRL_LOG_LOST_FORMAT)) {
		DBGLOG(INIT, ERROR, "length error : %u\n", u2MsgSize);
		return WLAN_STATUS_FAILURE;
	}

#if 0
	DBGLOG(INIT, STATE, "EVT Content:\n");
	DBGLOG_MEM8(RSN, STATE, pucIdxLog, u2MsgSize);
#endif

	prIdxV2Header = (struct IDX_LOG_V2_FORMAT *)pucIdxLog;

	DBGLOG(INIT, TRACE, "VerType=%u\n",
		(uint8_t)prIdxV2Header->ucVerType);

	if (prIdxV2Header->ucStx != IDXLOGSTX) {
		uint8_t *pucChr = NULL;

		/* XXX: expect there is no this case  */
		DBGLOG(INIT, ERROR,
			"Not IDX Log!! ucStx=0x%x, type=0x%x\n",
			(uint8_t)prIdxV2Header->ucStx,
			(uint8_t)prIdxV2Header->ucVerType);

		pucIdxLog[u2MsgSize] = '\0';
		pucChr = kalStrChr(pucIdxLog, '\0');
		if (*(pucChr - 1) == '\n')
			*(pucChr - 1) = '\0';

		LOG_FUNC("<FW>%s\n", pucIdxLog);

		dbgFwLogDebugPrint(pucIdxLog, u2MsgSize, NULL);

		return WLAN_STATUS_FAILURE;
	}

	if (prIdxV2Header->ucVerType == VER_TYPE_IDX_LOG_V2) {

		uint32_t u4LogSize = 0;

		DBGLOG(INIT, TRACE,
			"STX=0x%x, VerType=%u, NumArgs=%u, ModId=%u, ",
			(uint8_t)prIdxV2Header->ucStx,
			(uint8_t)prIdxV2Header->ucVerType,
			(uint8_t)prIdxV2Header->ucNumArgs,
			(uint8_t)prIdxV2Header->ucModId);
		DBGLOG(INIT, TRACE,
			"SeqNo=0x%x, LvId=%u, TimeStamp=0x%x, IdxId=0x%x\n",
			(uint8_t)prIdxV2Header->ucSeqNo,
			(uint8_t)prIdxV2Header->ucLevelId,
			prIdxV2Header->u4TimeStamp,
			prIdxV2Header->u4IdxId);

		u4LogSize = (uint32_t)prIdxV2Header->ucNumArgs * 4 +
				 sizeof(struct IDX_LOG_V2_FORMAT);

		DBGLOG(INIT, TRACE,
			"u4LogSize=%u, u2MsgSize=%u\n", u4LogSize, u2MsgSize);

		if (u4LogSize > u2MsgSize) {
			DBGLOG(INIT, ERROR, "offset=0x%x, length err: %u:%u\n",
				prIdxV2Header->u4IdxId, u2MsgSize,
				u4LogSize);

			dbgFwLogDebugPrint(pucIdxLog, u2MsgSize, NULL);

			return WLAN_STATUS_FAILURE;
		}

		/* find the related msg format entry in the bin */
		if (dbgFwLogFindEntry(prAdapter->prFwLogIdx,
				prIdxV2Header->u4IdxId, &rLogEntry) == FALSE) {
			DBGLOG(INIT, STATE, "can't find log format\n");

			dbgFwLogDebugPrint(pucIdxLog, u2MsgSize, &rLogEntry);

			return WLAN_STATUS_FAILURE;
		}

		/* translate the fw log to string */
		dbgFwLogIdxToStr(&rLogEntry, prIdxV2Header, pucIdxLog,
				aucLogBuf);

		/* print the fw log  */
		LOG_FUNC("%s", aucLogBuf);

	} else if (prIdxV2Header->ucVerType == VER_TYPE_TXT_LOG) {

		uint8_t *prLogStr = NULL;

		prTextLog = (struct TEXT_LOG_FORMAT *)pucIdxLog;

		prLogStr = (uint8_t *)prTextLog +
				sizeof(struct TEXT_LOG_FORMAT);

		if ((prTextLog->ucPayloadSize_wo_padding +
			sizeof(struct TEXT_LOG_FORMAT)) > u2MsgSize) {
			DBGLOG(INIT, ERROR, "error payload size (%u:%u:%u)\n",
				prTextLog->ucPayloadSize_wo_padding,
				prTextLog->ucPayloadSize_w_padding,
				u2MsgSize);

			dbgFwLogDebugPrint(pucIdxLog, u2MsgSize, &rLogEntry);

			return WLAN_STATUS_FAILURE;
		}

		/* Expect the rx text log last byte is '0x0a' */
		prLogStr[prTextLog->ucPayloadSize_wo_padding-1] = '\0';

		LOG_FUNC("<FW>%s", prLogStr);

	} else {

		/* TODO: process Time Sync Message here */

		DBGLOG(INIT, ERROR, "Not supported type!! type=0x%x\n",
			(uint8_t)(prIdxV2Header->ucVerType));

		dbgFwLogDebugPrint(pucIdxLog, u2MsgSize, &rLogEntry);

		return WLAN_STATUS_FAILURE;
	}

	rStatus = WLAN_STATUS_SUCCESS;
	return rStatus;
}

#endif /* CFG_SUPPORT_FW_IDX_LOG_TRANS */
