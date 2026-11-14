/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _CCM_H
#define _CCM_H

#if CFG_SUPPORT_CCM
enum ENUM_P2P_CCM_MODE {
	P2P_CCM_MODE_DISABLE,
	P2P_CCM_MODE_SCC
};
#endif /* CFG_SUPPORT_CCM */

enum ENUM_CCM_NET_TYPE {
	ENUM_CCM_SAP,
	ENUM_CCM_GO,
	ENUM_CCM_GC,
	ENUM_CCM_NET_NUM
};

#define CCM_SAP_BIT		BIT(ENUM_CCM_SAP)
#define CCM_GO_BIT		BIT(ENUM_CCM_GO)
#define CCM_GC_BIT		BIT(ENUM_CCM_GC)
#define CCM_ALL_NET_BITS	BITS(ENUM_CCM_SAP, ENUM_CCM_NET_NUM - 1)

typedef void(*CCM_CALLBACK_FUNC) (struct ADAPTER *);

/******************************************************************************
 *                                 M A C R O S
 ******************************************************************************
 */
struct CCM_AA_FOBIDEN_REGION_UNIT {
	uint32_t u4BoundForward1;
	uint32_t u4BoundForward2;
	uint32_t u4BoundInverse1;
	uint32_t u4BoundInverse2;
	uint32_t u4BoundIsolate;
};

/******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 ******************************************************************************
 */

#if CFG_SUPPORT_CCM
void ccmInit(struct ADAPTER *prAdapter);

void ccmUninit(struct ADAPTER *prAdapter);

void ccmRegisterStableCb(struct ADAPTER *prAdapter,
			 CCM_CALLBACK_FUNC func);

void ccmUnregisterStableCb(struct ADAPTER *prAdapter,
			   CCM_CALLBACK_FUNC func);

void ccmChannelSwitchProducer(struct ADAPTER *prAdapter,
			      struct BSS_INFO *prTargetBss,
			      const char *pucSrcFunc);

void ccmChannelSwitchProducerByNetType(struct ADAPTER *prAdapter,
			      struct BSS_INFO *prTargetBss,
			      const char *pucSrcFunc,
			      uint8_t ucNetTypeBits);

void ccmChannelSwitchProducerDfs(struct ADAPTER *prAdapter,
				 struct BSS_INFO *prTargetBss);

void ccmChannelSwitchConsumer(struct ADAPTER *prAdapter);

void ccmRemoveBssPendingEntry(struct ADAPTER *prAdapter,
			      struct BSS_INFO *prBssInfo);

void ccmPendingCheck(struct ADAPTER *prAdapter,
		     struct BSS_INFO *prTargetBss,
		     uint32_t u4GrantInterval);


void ccmAAForbiddenRegionCal(struct ADAPTER *prAdapter,
			     struct RF_CHANNEL_INFO *rChnlInfo,
			     uint8_t *prForbiddenListLen,
			     uint16_t *prTargetBw,
			     struct CCM_AA_FOBIDEN_REGION_UNIT *arRegionOutput);

u_int8_t ccmIsPreferAA(struct ADAPTER *prAdapter);

bool ccmAAAvailableCheck(struct ADAPTER *prAdapter,
			 struct RF_CHANNEL_INFO *prRfChnlInfo1,
			 struct RF_CHANNEL_INFO *prRfChnlInfo2);

u_int8_t ccmIsGcCsaReqChanAcceptable(struct ADAPTER *prAdapter,
				     struct BSS_INFO *prBssInfo,
				     struct RF_CHANNEL_INFO *prChnlInfo);
#else
static inline void ccmInit(struct ADAPTER *prAdapter) {}

static inline void
ccmUninit(struct ADAPTER *prAdapter) {}

static inline void
ccmRegisterStableCb(struct ADAPTER *prAdapter, CCM_CALLBACK_FUNC func) {}

static inline void
ccmUnregisterStableCb(struct ADAPTER *prAdapter, CCM_CALLBACK_FUNC func) {}

static inline void ccmChannelSwitchProducer(struct ADAPTER *prAdapter,
			      struct BSS_INFO *prTargetBss,
			      const char *pucSrcFunc)
{
#if CFG_ENABLE_WIFI_DIRECT
	p2pFuncSwitchSapChannel(prAdapter, P2P_DEFAULT_SCENARIO);
#endif
}

static inline void ccmChannelSwitchProducerByNetType(struct ADAPTER *prAdapter,
			      struct BSS_INFO *prTargetBss,
			      const char *pucSrcFunc,
			      uint8_t ucNetTypeBits)
{
#if CFG_ENABLE_WIFI_DIRECT
	p2pFuncSwitchSapChannel(prAdapter, P2P_DEFAULT_SCENARIO);
#endif
}

static inline void ccmChannelSwitchProducerDfs(struct ADAPTER *prAdapter,
					       struct BSS_INFO *prTargetBss)
{
}

static inline void ccmChannelSwitchConsumer(struct ADAPTER *prAdapter) {}

static inline
u_int8_t ccmIsGcCsaReqChanAcceptable(struct ADAPTER *prAdapter,
				     struct BSS_INFO *prBssInfo,
				     struct RF_CHANNEL_INFO *prChnlInfo)
{
	return FALSE;
}
#endif /* CFG_SUPPORT_CCM */

#endif /* _CCM_H */
