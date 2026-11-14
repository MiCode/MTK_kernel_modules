/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _NAN_LOG_H_
#define _NAN_LOG_H_

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */
#if CFG_SUPPORT_NAN
/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define NAN_LOG_TO_KERNEL_LOG 0

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */
struct _NAN_SCHED_EVENT_SLOT_STATISTICS_T {
	uint32_t u4TotalSlotTxByte;
	uint32_t u4SlotTxByte[NAN_SLOTS_PER_DW_INTERVAL];
	uint32_t u4TotalSlotRxByte;
	uint32_t u4SlotRxByte[NAN_SLOTS_PER_DW_INTERVAL];
};

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */
#define NAN_DBGLOG(_Mod, _Clz, _Fmt, ...) \
	do { \
		if ((au2DebugModule[DBG_##_Mod##_IDX] & \
			 DBG_CLASS_##_Clz) == 0) \
			break; \
		NAN_LOG_FUNC("[%u]%s:(" #_Mod " " #_Clz ") " _Fmt, \
			 KAL_GET_CURRENT_THREAD_ID(), \
			 __func__, ##__VA_ARGS__); \
	} while (0)
#define NAN_LOG_FUNC                kalNanPrint
#define kalNanPrint(_Fmt...) \
	((NAN_LOG_TO_KERNEL_LOG == 0) \
	? kalPrintSALog(_Fmt) \
	: pr_info(WLAN_TAG _Fmt))

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
static inline uint32_t nanSchedDbgDumpCommittedSlotAndChannel(
	struct ADAPTER *prAdapter, const char *pucEvent)
{
	return WLAN_STATUS_SUCCESS;
}
static inline uint32_t nanSchedDbgDumpPeerCommittedSlotAndChannel(
	struct ADAPTER *prAdapter, uint8_t *pucNmiAddr, const char *pucEvent)
{
	return WLAN_STATUS_SUCCESS;
}

static inline void nicNanSlotStatisticsEvt(
	struct ADAPTER *prAdapter, uint8_t *pucEventBuf)
{
}

/* NDI Info */
static inline void nanLogNdiInfo(struct _NAN_NDP_INSTANCE_T *prNDP)
{
}

/* Rx Req Fail */
static inline void nanLogFailRxReqStr(char *pucReasonStr)
{
}
static inline void nanLogFailRxReq(uint8_t ucReasonCode)
{
}

/* Rx Resp Fail */
static inline void nanLogFailRxRespStr(char *pucReasonStr)
{
}
static inline void nanLogFailRxResp(uint8_t ucReasonCode)
{
}

/* Rx Confirm Fail */
static inline void nanLogFailRxConfmStr(char *pucReasonStr)
{
}
static inline void nanLogFailRxConfm(uint8_t ucReasonCode)
{
}

/* Rx Key Install Fail */
static inline void nanLogFailRxKeyInstlStr(char *pucReasonStr)
{
}
static inline void nanLogFailRxKeyInstl(uint8_t ucReasonCode)
{
}

/* NDP Final Confirm */
static inline void nanLogNdpFinlConfm(char *pucStatusStr,
				      struct _NAN_NDL_INSTANCE_T *prNDL,
				      struct _NAN_NDP_INSTANCE_T *prNDP)
{
}

/* Tx */
static inline void nanLogTx(struct _NAN_ACTION_FRAME_T *prNAF)
{
}

/* Tx Done */
static inline void nanLogTxDone(uint8_t ucNafOui, uint8_t ucTxDoneStatus)
{
}

/* Tx and Tx Done Together for Follow Up Frame */
static inline void nanLogTxAndTxDoneFollowup(const char *pucTypeStr,
			struct NAN_FOLLOW_UP_EVENT *prFollowupEvt)
{
}

/* Rx */
static inline void nanLogRx(const char *pcFrameType, uint8_t *pucSrcMacAddr)
{
}

/* Create Publish Service */
static inline void nanLogPublish(uint16_t u2PublishId)
{
}

/* Create Subscribe Service */
static inline void nanLogSubscribe(uint16_t u2SubscribeId)
{
}

/* Start Cluster with New MAC address (NMI) */
static inline void nanLogClusterMac(uint8_t *pucOwnMacAddr)
{
}

/* Start Cluster with Cluster ID */
static inline void nanLogClusterId(uint8_t *pucClusterId)
{
}

/* Join Cluster */
static inline void nanLogJoinCluster(uint8_t *pucClusterId)
{
}

/* Match Publish/Subscribe Service */
static inline void nanLogMatch(uint8_t *pucPeerMacAddr)
{
}

/* NDP Termination Reason */
static inline void nanLogEndReason(char *pucTerminationReason)
{
}

/* Change Tx Done Status Code Number to String */
static inline const char *nanTxDoneStatusString(uint8_t status_code)
{
	return "Undefined";
}
#endif /* CFG_SUPPORT_NAN */
#endif /* _NAN_LOG_H_ */
