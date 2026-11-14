// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "precomp.h"

#if (CFG_SUPPORT_STATISTICS == 1)
static uint8_t aucStr[WAKE_STR_BUFFER_LEN];
#endif

#if (CFG_SUPPORT_TRACE_TC4 == 1)
struct COMMAND {
	uint8_t ucCID;
	u_int8_t fgSetQuery;
	u_int8_t fgNeedResp;
	uint8_t ucCmdSeqNum;
};

struct DATA_FRAME {
	uint16_t u2EthType;
	uint16_t u2Reserved;
};

struct MGMT_FRAME {
	uint16_t u2FrameCtl;
	uint16_t u2DurationID;
};

struct TC_RES_RELEASE_ENTRY {
	uint64_t u8RelaseTime;
	uint32_t u4RelCID;
	uint32_t u4Tc4RelCnt;
	uint32_t u4AvailableTc4;
};

struct CMD_TRACE_ENTRY {
	uint64_t u8TxTime;
	enum COMMAND_TYPE eCmdType;
	union {
		struct COMMAND rCmd;
		struct DATA_FRAME rDataFrame;
		struct MGMT_FRAME rMgmtFrame;
	} u;
};

#define TC_RELEASE_TRACE_BUF_MAX_NUM 100
#define TXED_CMD_TRACE_BUF_MAX_NUM 100

static struct TC_RES_RELEASE_ENTRY *gprTcReleaseTraceBuffer;
static struct CMD_TRACE_ENTRY *gprCmdTraceEntry;
void wlanDebugTC4Init(void)
{
	/* debug for command/tc4 resource begin */
	gprTcReleaseTraceBuffer =
		kalMemAlloc(TC_RELEASE_TRACE_BUF_MAX_NUM * sizeof(
				    struct TC_RES_RELEASE_ENTRY), PHY_MEM_TYPE);
	kalMemZero(gprTcReleaseTraceBuffer,
		   TC_RELEASE_TRACE_BUF_MAX_NUM * sizeof(struct
				   TC_RES_RELEASE_ENTRY));
	gprCmdTraceEntry = kalMemAlloc(TXED_CMD_TRACE_BUF_MAX_NUM *
				       sizeof(struct CMD_TRACE_ENTRY),
					   PHY_MEM_TYPE);
	kalMemZero(gprCmdTraceEntry,
		   TXED_CMD_TRACE_BUF_MAX_NUM * sizeof(struct
				   CMD_TRACE_ENTRY));
	/* debug for command/tc4 resource end */
}

void wlanDebugTC4Uninit(void)
{
	/* debug for command/tc4 resource begin */
	kalMemFree(gprTcReleaseTraceBuffer, PHY_MEM_TYPE,
		   TC_RELEASE_TRACE_BUF_MAX_NUM * sizeof(struct
				   TC_RES_RELEASE_ENTRY));
	kalMemFree(gprCmdTraceEntry, PHY_MEM_TYPE,
		   TXED_CMD_TRACE_BUF_MAX_NUM * sizeof(struct
				   CMD_TRACE_ENTRY));
	/* debug for command/tc4 resource end */
}

void wlanTraceTxCmd(struct CMD_INFO *prCmd)
{
	static uint16_t u2CurEntry;
	struct CMD_TRACE_ENTRY *prCurCmd =
			&gprCmdTraceEntry[u2CurEntry];

	prCurCmd->u8TxTime = kalGetTimeTickNs();
	prCurCmd->eCmdType = prCmd->eCmdType;
	if (prCmd->eCmdType == COMMAND_TYPE_MANAGEMENT_FRAME) {
		struct WLAN_MAC_MGMT_HEADER *prMgmt = (struct
			WLAN_MAC_MGMT_HEADER *)prCmd->prMsduInfo->prPacket;

		prCurCmd->u.rMgmtFrame.u2FrameCtl = prMgmt->u2FrameCtrl;
		prCurCmd->u.rMgmtFrame.u2DurationID = prMgmt->u2Duration;
	} else {
		prCurCmd->u.rCmd.ucCID = prCmd->ucCID;
		prCurCmd->u.rCmd.ucCmdSeqNum = prCmd->ucCmdSeqNum;
		prCurCmd->u.rCmd.fgNeedResp = prCmd->fgNeedResp;
		prCurCmd->u.rCmd.fgSetQuery = prCmd->fgSetQuery;
	}
	u2CurEntry++;
	if (u2CurEntry == TC_RELEASE_TRACE_BUF_MAX_NUM)
		u2CurEntry = 0;
}

void wlanTraceReleaseTcRes(struct ADAPTER *prAdapter,
			   uint32_t u4TxRlsCnt, uint32_t u4Available)
{
	static uint16_t u2CurEntry;
	struct TC_RES_RELEASE_ENTRY *prCurBuf =
			&gprTcReleaseTraceBuffer[u2CurEntry];

	prCurBuf->u8RelaseTime = kalGetTimeTickNs();
	prCurBuf->u4Tc4RelCnt =  u4TxRlsCnt;
	prCurBuf->u4AvailableTc4 = u4Available;
	u2CurEntry++;
	if (u2CurEntry == TXED_CMD_TRACE_BUF_MAX_NUM)
		u2CurEntry = 0;
}

void wlanDumpTcResAndTxedCmd(uint8_t *pucBuf,
			     uint32_t maxLen)
{
	uint16_t i = 0;
	struct TC_RES_RELEASE_ENTRY *prTcRel =
			gprTcReleaseTraceBuffer;
	struct CMD_TRACE_ENTRY *prCmd = gprCmdTraceEntry;

	if (pucBuf) {
		int bufLen = 0;

		for (; i < TXED_CMD_TRACE_BUF_MAX_NUM / 2; i++) {
			bufLen = snprintf(pucBuf, maxLen,
				  "%d: Time %llu, Type %d, Content %08x; %d: Time %llu, Type %d, Content %08x\n",
				  i * 2, prCmd[i * 2].u8TxTime,
				  prCmd[i * 2].eCmdType,
				  *(uint32_t *)
					(&prCmd[i * 2].u.rCmd.ucCID),
				  i * 2 + 1,
				  prCmd[i * 2 + 1].u8TxTime,
				  prCmd[i * 2 + 1].eCmdType,
				  *(uint32_t *)
					(&prCmd[i * 2 + 1].u.rCmd.ucCID));
			if (bufLen <= 0)
				break;
			pucBuf += bufLen;
			maxLen -= bufLen;
		}
		for (i = 0; i < TC_RELEASE_TRACE_BUF_MAX_NUM / 2; i++) {
			bufLen = snprintf(pucBuf, maxLen,
				  "%d: Time %llu, Tc4Cnt %d, Free %d, CID %08x; %d: Time %llu, Tc4Cnt %d, Free %d CID %08x\n",
				  i * 2, prTcRel[i * 2].u8RelaseTime,
				  prTcRel[i * 2].u4Tc4RelCnt,
				  prTcRel[i * 2].u4AvailableTc4,
				  prTcRel[i * 2].u4RelCID,
				  i * 2 + 1,
				  prTcRel[i * 2 + 1].u8RelaseTime,
				  prTcRel[i * 2 + 1].u4Tc4RelCnt,
				  prTcRel[i * 2 + 1].u4AvailableTc4,
				  prTcRel[i * 2 + 1].u4RelCID);
			if (bufLen <= 0)
				break;
			pucBuf += bufLen;
			maxLen -= bufLen;
		}
	} else {
		for (; i < TXED_CMD_TRACE_BUF_MAX_NUM / 4; i++) {
			LOG_FUNC(
				 "%d: Time %llu, Type %d, Content %08x; %d: Time %llu, Type %d, Content %08x; ",
				 i * 4, prCmd[i * 4].u8TxTime,
				 prCmd[i * 4].eCmdType,
				 *(uint32_t *)(&prCmd[i * 4].u.rCmd.ucCID),
				 i * 4 + 1, prCmd[i * 4 + 1].u8TxTime,
				 prCmd[i * 4 + 1].eCmdType,
				 *(uint32_t *)(&prCmd[i * 4 + 1].u.rCmd.ucCID));
			LOG_FUNC(
				 "%d: Time %llu, Type %d, Content %08x; %d: Time %llu, Type %d, Content %08x\n",
				 i * 4 + 2, prCmd[i * 4 + 2].u8TxTime,
				 prCmd[i * 4 + 2].eCmdType,
				 *(uint32_t *)(&prCmd[i * 4 + 2].u.rCmd.ucCID),
				 i * 4 + 3, prCmd[i * 4 + 3].u8TxTime,
				 prCmd[i * 4 + 3].eCmdType,
				 *(uint32_t *)(&prCmd[i * 4 + 3].u.rCmd.ucCID));
		}
		for (i = 0; i < TC_RELEASE_TRACE_BUF_MAX_NUM / 4; i++) {
			LOG_FUNC(
				"%d: Time %llu, Tc4Cnt %d, Free %d, CID %08x; %d: Time %llu, Tc4Cnt %d, Free %d, CID %08x;",
				i * 4, prTcRel[i * 4].u8RelaseTime,
				prTcRel[i * 4].u4Tc4RelCnt,
				prTcRel[i * 4].u4AvailableTc4,
				prTcRel[i * 4].u4RelCID,
				i * 4 + 1, prTcRel[i * 4 + 1].u8RelaseTime,
				prTcRel[i * 4 + 1].u4Tc4RelCnt,
				prTcRel[i * 4 + 1].u4AvailableTc4,
				prTcRel[i * 4 + 1].u4RelCID);
			LOG_FUNC(
				"%d: Time %llu, Tc4Cnt %d, Free %d, CID %08x; %d: Time %llu, Tc4Cnt %d, Free %d, CID %08x\n",
				i * 4 + 2, prTcRel[i * 4 + 2].u8RelaseTime,
				prTcRel[i * 4 + 2].u4Tc4RelCnt,
				prTcRel[i * 4 + 2].u4AvailableTc4,
				prTcRel[i * 4 + 2].u4RelCID,
				i * 4 + 3, prTcRel[i * 4 + 3].u8RelaseTime,
				prTcRel[i * 4 + 3].u4Tc4RelCnt,
				prTcRel[i * 4 + 3].u4AvailableTc4,
				prTcRel[i * 4 + 3].u4RelCID);
		}
	}
}
#endif


#if (CFG_SUPPORT_STATISTICS == 1)

void wlanWakeStaticsInit(struct GLUE_INFO *prGlueInfo)
{
	if (!prGlueInfo)
		return;

	prGlueInfo->prWakeInfoStatics =
		kalMemAlloc(WAKE_MAX_CMD_EVENT_NUM * sizeof(
				    struct WAKE_INFO_T), PHY_MEM_TYPE);
	if (prGlueInfo->prWakeInfoStatics != NULL)
		kalMemZero(prGlueInfo->prWakeInfoStatics,
		   WAKE_MAX_CMD_EVENT_NUM * sizeof(struct
				   WAKE_INFO_T));
}

void wlanWakeStaticsUninit(struct GLUE_INFO *prGlueInfo)
{
	if (!prGlueInfo)
		return;

	if (prGlueInfo->prWakeInfoStatics != NULL)
		kalMemFree(prGlueInfo->prWakeInfoStatics, PHY_MEM_TYPE,
		WAKE_MAX_CMD_EVENT_NUM * sizeof(struct WAKE_INFO_T));
	prGlueInfo->prWakeInfoStatics = NULL;
}

uint32_t wlanWakeLogCmd(struct GLUE_INFO *prGlueInfo, uint8_t ucCmdId)
{
	struct WAKE_INFO_T *prWakeInfoStatics = NULL;
	int i = 0;
	int j = 0;

	if (prGlueInfo == NULL)
		return 1;

	prWakeInfoStatics = prGlueInfo->prWakeInfoStatics;

	if ((prWakeInfoStatics == NULL) || (wlan_fb_power_down != TRUE))
		return 1;

	for (i = 0; i < WAKE_MAX_CMD_EVENT_NUM; i++) {
		if ((prWakeInfoStatics->arCmd[i].ucFlagIsUesd == TRUE)
			&& (prWakeInfoStatics->arCmd[i].ucCmdId == ucCmdId)) {
			/*old item ++*/
			prWakeInfoStatics->arCmd[i].u2Cnt++;
			prWakeInfoStatics->u4TotalCmd++;
			break;
		}
	}

	if (i >= WAKE_MAX_CMD_EVENT_NUM) {
		/*add new item*/
		for (j = 0; j < WAKE_MAX_CMD_EVENT_NUM; j++) {
			if (prWakeInfoStatics->arCmd[j].ucFlagIsUesd != TRUE) {
				prWakeInfoStatics->ucCmdCnt++;
				prWakeInfoStatics->arCmd[j].ucCmdId = ucCmdId;
				prWakeInfoStatics->arCmd[j].u2Cnt++;
				prWakeInfoStatics->u4TotalCmd++;
				prWakeInfoStatics->arCmd[j].ucFlagIsUesd
					= TRUE;
				break;
			}
		}

		if (j >= WAKE_MAX_CMD_EVENT_NUM) {
			DBGLOG_LIMITED(OID, WARN,
			"Wake cmd over flow %d-0x%02x\n",
			WAKE_MAX_CMD_EVENT_NUM, ucCmdId);
		}
	}
	return 0;
}

uint32_t wlanWakeLogEvent(struct GLUE_INFO *prGlueInfo, uint8_t ucEventId)
{
	struct WAKE_INFO_T *prWakeInfoStatics = NULL;
	int i = 0;
	int j = 0;

	if (prGlueInfo == NULL)
		return 1;

	prWakeInfoStatics = prGlueInfo->prWakeInfoStatics;

	if ((prWakeInfoStatics == NULL) || (wlan_fb_power_down != TRUE))
		return 1;

	for (i = 0; i < WAKE_MAX_CMD_EVENT_NUM; i++) {
		if ((prWakeInfoStatics->arEvent[i].ucFlagIsUesd == TRUE)
		&&
		(prWakeInfoStatics->arEvent[i].ucEventId == ucEventId)) {
			/*old item ++*/
			prWakeInfoStatics->arEvent[i].u2Cnt++;
			prWakeInfoStatics->u4TotalEvent++;
			break;
		}
	}

	if (i >= WAKE_MAX_CMD_EVENT_NUM) {
		/*add new item*/
		for (j = 0; j < WAKE_MAX_CMD_EVENT_NUM; j++) {
			if (prWakeInfoStatics->arEvent[j].ucFlagIsUesd
				!= TRUE) {
				prWakeInfoStatics->ucEventCnt++;
				prWakeInfoStatics->arEvent[j].ucEventId
					= ucEventId;
				prWakeInfoStatics->arEvent[j].u2Cnt++;
				prWakeInfoStatics->u4TotalEvent++;
				prWakeInfoStatics->arEvent[j].ucFlagIsUesd
					= TRUE;
				break;
			}
		}

		if (j >= WAKE_MAX_CMD_EVENT_NUM) {
			DBGLOG(OID, WARN,
			"Wake event over flow %d-0x%02x\n",
			WAKE_MAX_CMD_EVENT_NUM, ucEventId);
		}
	}
	return 0;
}

void wlanLogTxData(struct GLUE_INFO *prGlueInfo, enum WAKE_DATA_TYPE dataType)
{
	struct WAKE_INFO_T *prWakeInfoStatics = NULL;

	if (prGlueInfo == NULL)
		return;

	prWakeInfoStatics = prGlueInfo->prWakeInfoStatics;

	if ((prWakeInfoStatics != NULL) && (wlan_fb_power_down == TRUE)) {
		prWakeInfoStatics->au4TxDataCnt[dataType]++;
		prWakeInfoStatics->u4TxCnt++;
	}
}

void wlanLogRxData(struct GLUE_INFO *prGlueInfo, enum WAKE_DATA_TYPE dataType)
{
	struct WAKE_INFO_T *prWakeInfoStatics = NULL;

	if (prGlueInfo == NULL)
		return;

	prWakeInfoStatics = prGlueInfo->prWakeInfoStatics;

	if ((prWakeInfoStatics != NULL) && (wlan_fb_power_down == TRUE)) {
		prWakeInfoStatics->au4RxDataCnt[dataType]++;
		prWakeInfoStatics->u4RxCnt++;
	}
}

static void wlanWakeStaticsClear(struct GLUE_INFO *prGlueInfo)
{
	struct WAKE_INFO_T *prWakeInfoStatics = NULL;

	if (prGlueInfo == NULL)
		return;

	prWakeInfoStatics = prGlueInfo->prWakeInfoStatics;

	if (prWakeInfoStatics != NULL) {
		kalMemZero(prWakeInfoStatics,
			WAKE_MAX_CMD_EVENT_NUM * sizeof(struct
				   WAKE_INFO_T));
	}
}

uint32_t wlanWakeDumpRes(struct GLUE_INFO *prGlueInfo)
{
	struct WAKE_INFO_T *prWakeInfoStatics = NULL;
	uint8_t i = 0;
	uint8_t flag = 0;
	char *pos = NULL;
	char *end = NULL;
	int ret = 0;

	if (prGlueInfo == NULL)
		return 1;

	prWakeInfoStatics = prGlueInfo->prWakeInfoStatics;

	if ((prWakeInfoStatics == NULL)
	|| (wlan_fb_power_down != TRUE)) {
		wlanWakeStaticsClear(prGlueInfo);
		return 1;
	}

	/*Log Style: one line log or human friendly log.*/
#if 1
	kalMemZero(&aucStr[0], sizeof(uint8_t)*WAKE_STR_BUFFER_LEN);
	pos = &aucStr[0];
	end = &aucStr[0] + WAKE_STR_BUFFER_LEN - 1;

	if (prWakeInfoStatics->ucCmdCnt > 0) {
		flag = 1;
		ret = snprintf(pos, (end - pos + 1), "CMD(%u:%u)= ",
			prWakeInfoStatics->ucCmdCnt,
			prWakeInfoStatics->u4TotalCmd);
		if (ret < 0 || ret >= (end - pos + 1))
			return 1;
		pos += ret;

		for (i = 0; i < prWakeInfoStatics->ucCmdCnt; i++) {
			ret = snprintf(pos, (end - pos + 1), "0x%02x-%d ",
				prWakeInfoStatics->arCmd[i].ucCmdId,
				prWakeInfoStatics->arCmd[i].u2Cnt);
			if (ret < 0 || ret >= (end - pos + 1))
				return 1;
			pos += ret;
		}
	}

	if (prWakeInfoStatics->ucEventCnt > 0) {
		flag = 1;
		ret = snprintf(pos, (end - pos + 1), "EVENT(%u:%u)= ",
			prWakeInfoStatics->ucEventCnt,
			prWakeInfoStatics->u4TotalEvent);
		if (ret < 0 || ret >= (end - pos + 1))
			return 1;
		pos += ret;

		for (i = 0; i < prWakeInfoStatics->ucEventCnt; i++) {
			ret = snprintf(pos, (end - pos + 1), "0x%02x-%d ",
				prWakeInfoStatics->arEvent[i].ucEventId,
				prWakeInfoStatics->arEvent[i].u2Cnt);
			if (ret < 0 || ret >= (end - pos + 1))
				return 1;
			pos += ret;
		}
	}

	if (prWakeInfoStatics->u4TxCnt > 0) {
		flag = 1;
		ret = snprintf(pos, (end - pos + 1),
			"TX(%u)=%u-%u-%u-%u-%u-%u ",
			prWakeInfoStatics->u4TxCnt,
			prWakeInfoStatics->au4TxDataCnt[WLAN_WAKE_ARP],
			prWakeInfoStatics->au4TxDataCnt[WLAN_WAKE_IPV4],
			prWakeInfoStatics->au4TxDataCnt[WLAN_WAKE_IPV6],
			prWakeInfoStatics->au4TxDataCnt[WLAN_WAKE_1X],
			prWakeInfoStatics->au4TxDataCnt[WLAN_WAKE_TDLS],
			prWakeInfoStatics->au4TxDataCnt[WLAN_WAKE_OTHER]);

		if (ret < 0 || ret >= (end - pos + 1))
			return 1;
		pos += ret;
	}

	if (prWakeInfoStatics->u4RxCnt > 0) {
		flag = 1;
		ret = snprintf(pos, (end - pos + 1),
			"RX(%u)=%u-%u-%u-%u-%u-%u ",
			prWakeInfoStatics->u4RxCnt,
			prWakeInfoStatics->au4RxDataCnt[WLAN_WAKE_ARP],
			prWakeInfoStatics->au4RxDataCnt[WLAN_WAKE_IPV4],
			prWakeInfoStatics->au4RxDataCnt[WLAN_WAKE_IPV6],
			prWakeInfoStatics->au4RxDataCnt[WLAN_WAKE_1X],
			prWakeInfoStatics->au4RxDataCnt[WLAN_WAKE_TDLS],
			prWakeInfoStatics->au4RxDataCnt[WLAN_WAKE_OTHER]);
		if (ret < 0 || ret >= (end - pos + 1))
			return 1;
		pos += ret;
	}

	if (flag != 0)
		DBGLOG(OID, DEBUG, "[WLAN-LP] %s\n", (char *)&aucStr[0]);
#else
	/*1.dump cmd*/
	if (prWakeInfoStatics->ucCmdCnt > 0) {
		kalMemZero(&aucStr[0], sizeof(uint8_t)*WAKE_STR_BUFFER_LEN);
		pos = &aucStr[0];
		end = &aucStr[0] + WAKE_STR_BUFFER_LEN - 1;
		for (i = 0; i < prWakeInfoStatics->ucCmdCnt; i++) {

			ret = snprintf(pos, end - pos, " 0x%02x ",
			prWakeInfoStatics->arCmd[i].ucCmdId);
			if (ret < 0 || ret >= end - pos)
				return 1;
			pos += ret;
		}
		DBGLOG(OID, DEBUG, "[LP-CMD-ID-%u][%s]\n",
			prWakeInfoStatics->ucCmdCnt, (char *)&aucStr[0]);

		kalMemZero(&aucStr[0], sizeof(uint8_t)*WAKE_STR_BUFFER_LEN);
		pos = &aucStr[0];
		end = &aucStr[0] + WAKE_STR_BUFFER_LEN - 1;
		for (i = 0; i < prWakeInfoStatics->ucCmdCnt; i++) {

			ret = snprintf(pos, end - pos, " %u ",
				prWakeInfoStatics->arCmd[i].u2Cnt);
			if (ret < 0 || ret >= end - pos)
				return 1;
			pos += ret;
		}
		DBGLOG(OID, DEBUG, "[LP-CMD-CNT-%u][%s]\n",
			prWakeInfoStatics->u4TotalCmd, (char *)&aucStr[0]);
	}

	/*2.dump event*/
	if (prWakeInfoStatics->ucCmdCnt > 0) {

		kalMemZero(&aucStr[0], sizeof(uint8_t)*WAKE_STR_BUFFER_LEN);
		pos = &aucStr[0];
		end = &aucStr[0] + WAKE_STR_BUFFER_LEN - 1;
		for (i = 0; i < prWakeInfoStatics->ucEventCnt; i++) {

			ret = snprintf(pos, end - pos, " 0x%02x ",
				prWakeInfoStatics->arEvent[i].ucEventId);
			if (ret < 0 || ret >= end - pos)
				return 1;
			pos += ret;
		}
		DBGLOG(OID, DEBUG, "[LP-EVENT-ID-%u][%s]\n",
			prWakeInfoStatics->ucEventCnt, (char *)&aucStr[0]);

		kalMemZero(&aucStr[0], sizeof(uint8_t)*WAKE_STR_BUFFER_LEN);
		pos = &aucStr[0];
		end = &aucStr[0] + WAKE_STR_BUFFER_LEN - 1;
		for (i = 0; i < prWakeInfoStatics->ucEventCnt; i++) {

			ret = snprintf(pos, end - pos, " %u ",
				prWakeInfoStatics->arEvent[i].u2Cnt);
			if (ret < 0 || ret >= end - pos) {
				end[-1] = '\0';
				return 1;
			}
			pos += ret;
		}
		DBGLOG(OID, DEBUG, "[LP-EVENT-CNT-%u][%s]\n",
			prWakeInfoStatics->u4TotalEvent, (char *)&aucStr[0]);
	}

	/*3.dump tx/rx data*/
	if (prWakeInfoStatics->u4TxCnt > 0) {
		DBGLOG(OID, DEBUG, "[LP-EVENT-TX-%u][%u-%u-%u-%u-%u-%u]\n",
			prWakeInfoStatics->u4TxCnt,
			prWakeInfoStatics->au4TxDataCnt[WLAN_WAKE_ARP],
			prWakeInfoStatics->au4TxDataCnt[WLAN_WAKE_IPV4],
			prWakeInfoStatics->au4TxDataCnt[WLAN_WAKE_IPV6],
			prWakeInfoStatics->au4TxDataCnt[WLAN_WAKE_1X],
			prWakeInfoStatics->au4TxDataCnt[WLAN_WAKE_TDLS],
			prWakeInfoStatics->au4TxDataCnt[WLAN_WAKE_OTHER]);
	}

	if (prWakeInfoStatics->u4RxCnt > 0) {
		DBGLOG(OID, DEBUG, "[LP-EVENT-RX-%u][%u-%u-%u-%u-%u-%u]\n",
			prWakeInfoStatics->u4RxCnt,
			prWakeInfoStatics->au4RxDataCnt[WLAN_WAKE_ARP],
			prWakeInfoStatics->au4RxDataCnt[WLAN_WAKE_IPV4],
			prWakeInfoStatics->au4RxDataCnt[WLAN_WAKE_IPV6],
			prWakeInfoStatics->au4RxDataCnt[WLAN_WAKE_1X],
			prWakeInfoStatics->au4RxDataCnt[WLAN_WAKE_TDLS],
			prWakeInfoStatics->au4RxDataCnt[WLAN_WAKE_OTHER]);
	}
#endif
	wlanWakeStaticsClear(prGlueInfo);
	return 0;
}

#endif

uint32_t wlanSetDriverDbgLevel(uint32_t u4DbgIdx, uint32_t u4DbgMask)
{
	uint32_t u4Idx;
	uint32_t fgStatus = WLAN_STATUS_SUCCESS;

	if (u4DbgIdx == DBG_ALL_MODULE_IDX) {
		for (u4Idx = 0; u4Idx < DBG_MODULE_NUM; u4Idx++)
			au2DebugModule[u4Idx] = (uint16_t) u4DbgMask;
		LOG_FUNC("Set ALL DBG module log level to [0x%03x]\n",
				u4DbgMask);
	} else if (u4DbgIdx < DBG_MODULE_NUM) {
		au2DebugModule[u4DbgIdx] = (uint16_t) u4DbgMask;
		LOG_FUNC("Set DBG module[%u] log level to [0x%03x]\n",
				u4DbgIdx, u4DbgMask);
	} else {
		fgStatus = WLAN_STATUS_FAILURE;
	}

	if (fgStatus == WLAN_STATUS_SUCCESS)
		wlanDriverDbgLevelSync();

	return fgStatus;
}

uint32_t wlanGetDriverDbgLevel(uint32_t u4DbgIdx, uint32_t *pu4DbgMask)
{
	if (u4DbgIdx < DBG_MODULE_NUM) {
		*pu4DbgMask = au2DebugModule[u4DbgIdx];
		return WLAN_STATUS_SUCCESS;
	}

	return WLAN_STATUS_FAILURE;
}

uint32_t wlanDbgLevelUiSupport(struct ADAPTER *prAdapter, uint32_t u4Version,
		uint32_t ucModule)
{
	uint32_t u4Enable = ENUM_WIFI_LOG_LEVEL_SUPPORT_DISABLE;

	switch (u4Version) {
	case ENUM_WIFI_LOG_LEVEL_VERSION_V1:
		switch (ucModule) {
		case ENUM_WIFI_LOG_MODULE_DRIVER:
			u4Enable = ENUM_WIFI_LOG_LEVEL_SUPPORT_ENABLE;
			break;
		case ENUM_WIFI_LOG_MODULE_FW:
			u4Enable = ENUM_WIFI_LOG_LEVEL_SUPPORT_ENABLE;
			break;
		}
		break;
	default:
		break;
	}

	return u4Enable;
}

uint32_t wlanDbgGetLogLevelImpl(struct ADAPTER *prAdapter,
		uint32_t u4Version, uint32_t ucModule)
{
	uint32_t u4Level = ENUM_WIFI_LOG_LEVEL_DEFAULT;

	switch (u4Version) {
	case ENUM_WIFI_LOG_LEVEL_VERSION_V1:
		wlanDbgGetGlobalLogLevel(ucModule, &u4Level);
		break;
	default:
		break;
	}

	return u4Level;
}

void wlanDbgSetLogLevelImpl(struct ADAPTER *prAdapter,
		uint32_t u4Version, uint32_t u4Module, uint32_t u4level)
{
	wlanDbgSetLogLevel(prAdapter, u4Version, u4Module, u4level, FALSE);
}

void wlanDbgSetLogLevel(struct ADAPTER *prAdapter,
		uint32_t u4Version, uint32_t u4Module,
		uint32_t u4level, u_int8_t fgEarlySet)
{
	uint32_t u4DriverLevel = ENUM_WIFI_LOG_LEVEL_DEFAULT;
	uint32_t u4FwLevel = ENUM_WIFI_LOG_LEVEL_DEFAULT;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	if (u4level >= ENUM_WIFI_LOG_LEVEL_NUM)
		return;

	switch (u4Version) {
	case ENUM_WIFI_LOG_LEVEL_VERSION_V1:
		wlanDbgSetGlobalLogLevel(u4Module, u4level);
		switch (u4Module) {
		case ENUM_WIFI_LOG_MODULE_DRIVER:
		{
			uint32_t u4DriverLogMask;

			switch (u4level) {
			case ENUM_WIFI_LOG_LEVEL_DEFAULT:
				u4DriverLogMask = DBG_LOG_LEVEL_DEFAULT;
				break;
			case ENUM_WIFI_LOG_LEVEL_MORE:
				u4DriverLogMask = DBG_LOG_LEVEL_MORE;
				break;
			case ENUM_WIFI_LOG_LEVEL_EXTREME:
				u4DriverLogMask = DBG_LOG_LEVEL_EXTREME;
				break;
			case ENUM_WIFI_LOG_LEVEL_UV:
				u4DriverLogMask = DBG_LOG_LEVEL_UV;
				break;
			default:
				u4DriverLogMask = DBG_LOG_LEVEL_DEFAULT;
				break;
			}

			wlanSetDriverDbgLevel(DBG_ALL_MODULE_IDX,
					(u4DriverLogMask & DBG_CLASS_MASK));
		}
			break;
		case ENUM_WIFI_LOG_MODULE_FW:
		{
			struct CMD_EVENT_LOG_UI_INFO cmd;
			prAdapter->fgSetLogLevel = false;

			kalMemZero(&cmd,
					sizeof(struct CMD_EVENT_LOG_UI_INFO));
			cmd.ucVersion = u4Version;
			cmd.ucLogLevel = u4level;

			if (fgEarlySet) {
				/* Set during wifi on flow */
				rStatus = wlanSendFwLogControlCmd(prAdapter,
					CMD_ID_LOG_UI_INFO,
					nicCmdEventSetCommon,
					nicOidCmdTimeoutCommon,
					sizeof(struct CMD_EVENT_LOG_UI_INFO),
					(uint8_t *)&cmd);
			} else {
				rStatus = wlanSendSetQueryCmd(prAdapter,
					CMD_ID_LOG_UI_INFO,
					TRUE,
					FALSE,
					FALSE,
					nicCmdEventSetCommon,
					nicOidCmdTimeoutCommon,
					sizeof(struct CMD_EVENT_LOG_UI_INFO),
					(uint8_t *)&cmd,
					NULL,
					0);
			}

			if (rStatus != WLAN_STATUS_FAILURE)
				prAdapter->fgSetLogLevel = true;
			else
				DBGLOG(INIT, DEBUG,
				       "Log level setting fail!\n");
		}
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	wlanDbgGetGlobalLogLevel(ENUM_WIFI_LOG_MODULE_DRIVER, &u4DriverLevel);
	wlanDbgGetGlobalLogLevel(ENUM_WIFI_LOG_MODULE_FW, &u4FwLevel);
	kalSetLogTooMuch(u4DriverLevel, u4FwLevel);

	DBGLOG(INIT, DEBUG,
		"version=%d module=%d u4level=%d result[D:%d|F:%d]\n",
		u4Version, u4Module, u4level, u4DriverLevel, u4FwLevel);

}

u_int8_t wlanDbgGetGlobalLogLevel(uint32_t u4Module, uint32_t *pu4Level)
{
	if (u4Module != ENUM_WIFI_LOG_MODULE_DRIVER &&
			u4Module != ENUM_WIFI_LOG_MODULE_FW)
		return FALSE;

	*pu4Level = au4LogLevel[u4Module];
	return TRUE;
}

u_int8_t wlanDbgSetGlobalLogLevel(uint32_t u4Module, uint32_t u4Level)
{
	if (u4Module != ENUM_WIFI_LOG_MODULE_DRIVER &&
			u4Module != ENUM_WIFI_LOG_MODULE_FW)
		return FALSE;

	au4LogLevel[u4Module] = u4Level;
	return TRUE;
}

void wlanDriverDbgLevelSync(void)
{
	uint8_t i = 0;
	uint32_t u4Mask = DBG_CLASS_MASK;
	uint32_t u4DriverLogLevel = ENUM_WIFI_LOG_LEVEL_DEFAULT;

	/* get the lowest level as module's level */
	for (i = 0; i < DBG_MODULE_NUM; i++)
		u4Mask &= au2DebugModule[i];

	if ((u4Mask & DBG_LOG_LEVEL_EXTREME) == DBG_LOG_LEVEL_EXTREME)
		u4DriverLogLevel = ENUM_WIFI_LOG_LEVEL_EXTREME;
	else if ((u4Mask & DBG_LOG_LEVEL_MORE) == DBG_LOG_LEVEL_MORE)
		u4DriverLogLevel = ENUM_WIFI_LOG_LEVEL_MORE;
	else if ((u4Mask & DBG_LOG_LEVEL_DEFAULT) == DBG_LOG_LEVEL_DEFAULT)
		u4DriverLogLevel = ENUM_WIFI_LOG_LEVEL_DEFAULT;
	else
		u4DriverLogLevel = ENUM_WIFI_LOG_LEVEL_UV;

	DBGLOG(INIT, DEBUG, "u4DriverLogLevel=%d\n", u4DriverLogLevel);
	wlanDbgSetGlobalLogLevel(ENUM_WIFI_LOG_MODULE_DRIVER, u4DriverLogLevel);
}

static void
firmwareHexDump(const uint8_t *pucPreFix,
		int32_t i4PreFixType,
		int32_t i4RowSize, int32_t i4GroupSize,
		const void *pvBuf, size_t len, u_int8_t fgAscii)
{
#define OLD_KBUILD_MODNAME KBUILD_MODNAME
#undef KBUILD_MODNAME
#define KBUILD_MODNAME "wlan_mt6632_fw"

	const uint8_t *pucPtr = pvBuf;
	int32_t i, i4LineLen, i4Remaining = len;
	uint8_t ucLineBuf[32 * 3 + 2 + 32 + 1];

	if (i4RowSize != 16 && i4RowSize != 32)
		i4RowSize = 16;

	for (i = 0; i < len; i += i4RowSize) {
		i4LineLen = kal_min_t(int32_t, i4Remaining, i4RowSize);
		i4Remaining -= i4RowSize;

		KAL_HEX_DUMP_TO_BUFFER(pucPtr + i, i4LineLen, i4RowSize,
			i4GroupSize, ucLineBuf, sizeof(ucLineBuf), fgAscii);

		switch (i4PreFixType) {
		case DUMP_PREFIX_ADDRESS:
			pr_info("%s%p: %s\n",
				pucPreFix, pucPtr + i, ucLineBuf);
			break;
		case DUMP_PREFIX_OFFSET:
			pr_info("%s%.8x: %s\n", pucPreFix, i, ucLineBuf);
			break;
		default:
			pr_info("%s%s\n", pucPreFix, ucLineBuf);
			break;
		}
	}
#undef KBUILD_MODNAME
#define KBUILD_MODNAME OLD_KBUILD_MODNAME
}
#if (CFG_SUPPORT_CONNAC3X == 1 && CFG_SUPPORT_UPSTREAM_TOOL == 1)
static void PrintSuportUpstreamTool(uint8_t *pucLogContent, uint16_t u2MsgSize)
{
#define OLD_KBUILD_MODNAME KBUILD_MODNAME
#define OLD_LOG_FUNC LOG_FUNC
#undef KBUILD_MODNAME
#undef LOG_FUNC
#define KBUILD_MODNAME "wlan_mt6632_fw"
#define LOG_FUNC pr_info
	struct IDX_LOG_V2_FORMAT *prIdxHeader;
	struct TEXT_LOG_FORMAT *prTextLog;
	uint8_t *prLogStr;
	uint32_t *prArg;
	int32_t i;
	uint8_t buf[512] = {0};

	prIdxHeader = (struct IDX_LOG_V2_FORMAT *)pucLogContent;
	prArg = (uint32_t *)(pucLogContent + sizeof(struct IDX_LOG_V2_FORMAT));
	if (prIdxHeader->ucVerType == VER_TYPE_IDX_LOG_V2) {
		for (i = 0;
		    i < (u2MsgSize - sizeof(struct IDX_LOG_V2_FORMAT)) / 4;
		    i++) {
			if (i)
				kalSnprintf(buf, sizeof(buf), "%s,0x%X", buf,
					LE32_TO_CPU(prArg[i]));
			else
				kalSnprintf(buf, sizeof(buf), "0x%X",
					LE32_TO_CPU(prArg[i]));
		}
		kalWiphy_info(gprWdev[0]->wiphy, "idx: 0x%08X,%ld,%s\n",
			LE32_TO_CPU(prIdxHeader->u4IdxId),
			(u2MsgSize - sizeof(struct IDX_LOG_V2_FORMAT)) / 4,
			buf);
	} else if (prIdxHeader->ucVerType == VER_TYPE_TXT_LOG) {
		prTextLog = (struct TEXT_LOG_FORMAT *)pucLogContent;

		prLogStr = (uint8_t *)pucLogContent +
				      sizeof(struct TEXT_LOG_FORMAT);

		/* Expect the rx text log last byte is '0x0a' */
		prLogStr[prTextLog->ucPayloadSize_wo_padding-1] = '\0';
		kalWiphy_info(gprWdev[0]->wiphy, "%.*s",
			prTextLog->ucPayloadSize_wo_padding,
			prLogStr);
	} else {
		LOG_FUNC("Not supported type!! type=0x%x\n",
			prIdxHeader->ucVerType);
	}
#undef KBUILD_MODNAME
#undef LOG_FUNC
#define KBUILD_MODNAME OLD_KBUILD_MODNAME
#define LOG_FUNC OLD_LOG_FUNC
#undef OLD_KBUILD_MODNAME
#undef OLD_LOG_FUNC
}
#endif
void wlanPrintFwLog(struct ADAPTER *prAdapter,
			uint8_t *pucLogContent,
		    uint16_t u2MsgSize, uint8_t ucMsgType,
		    const uint8_t *pucFmt, ...)
{
#define OLD_KBUILD_MODNAME KBUILD_MODNAME
#define OLD_LOG_FUNC LOG_FUNC
#undef KBUILD_MODNAME
#undef LOG_FUNC
#define KBUILD_MODNAME "wlan_mt6632_fw"
#define LOG_FUNC pr_info
#define DBG_LOG_BUF_SIZE 128

	int8_t aucLogBuffer[DBG_LOG_BUF_SIZE];
	int32_t err;
	va_list args;
#if CFG_SUPPORT_MULTI_CARD
	struct wiphy *wiphy = NULL;
	struct net_device *prDev = NULL;
#endif

	if (u2MsgSize > DEBUG_MSG_SIZE_MAX - 1) {
		LOG_FUNC("Firmware Log Size(%d) is too large, type %d\n",
			u2MsgSize, ucMsgType);
		return;
	}
#if (CFG_SUPPORT_CONNAC3X == 1 && CFG_SUPPORT_UPSTREAM_TOOL == 1)
	PrintSuportUpstreamTool(pucLogContent, u2MsgSize);
#endif
	switch (ucMsgType) {
	case DEBUG_MSG_TYPE_ASCII: {
		uint8_t *pucChr;

		pucLogContent[u2MsgSize] = '\0';

		/* skip newline */
		pucChr = kalStrChr(pucLogContent, '\0');
		if (*(pucChr - 1) == '\n')
			*(pucChr - 1) = '\0';

#if CFG_SUPPORT_MULTI_CARD
		if (prAdapter && prAdapter->prGlueInfo) {
			/*
			 * Print FW log by wiphy name
			 */
			wiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
			prDev = prAdapter->prGlueInfo->prDevHandler;
			LOG_FUNC("<FW><%s %s>%s\n",
					prDev->name, wiphy_name(wiphy),
					pucLogContent);
		} else
#endif /* CFG_SUPPORT_MULTI_CARD */
			LOG_FUNC("<FW>%s\n", pucLogContent);
	}
	break;
	case DEBUG_MSG_TYPE_DRIVER:
		if (!pucFmt)
			break;

		/* Only 128 Bytes is available to print in driver */
		va_start(args, pucFmt);
		err = kalVsnprintf(aucLogBuffer,
			sizeof(aucLogBuffer) - 1, pucFmt, args);
		va_end(args);
		aucLogBuffer[DBG_LOG_BUF_SIZE - 1] = '\0';
		if (err >= 0)
			LOG_FUNC("%s\n", aucLogBuffer);
		break;
	case DEBUG_MSG_TYPE_MEM8:
		firmwareHexDump("fw data:", DUMP_PREFIX_ADDRESS,
				16, 1, pucLogContent, u2MsgSize, true);
		break;
	default:
		firmwareHexDump("fw data:", DUMP_PREFIX_ADDRESS,
				16, 4, pucLogContent, u2MsgSize, true);
		break;
	}

#undef KBUILD_MODNAME
#undef LOG_FUNC
#define KBUILD_MODNAME OLD_KBUILD_MODNAME
#define LOG_FUNC OLD_LOG_FUNC
#undef OLD_KBUILD_MODNAME
#undef OLD_LOG_FUNC
}

#if (CFG_SUPPORT_WF_DUMP_BT_COREDUMP == 1)
u_int8_t wlanBtCoreDumpInfo(u_int8_t fgIsSet, u_int8_t fgval)
{
	static u_int8_t fgIsBtDump = FALSE;

	if (fgIsSet == TRUE)
		fgIsBtDump = fgval;

	return fgIsBtDump;
}
#endif /* CFG_SUPPORT_WF_DUMP_BT_COREDUMP */
