// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   gl_p2p_kal.c
 *    \brief
 *
 */

/******************************************************************************
 *                         C O M P I L E R   F L A G S
 ******************************************************************************
 */

/******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 ******************************************************************************
 */
#include "precomp.h"

#if CFG_ENABLE_WIFI_DIRECT
/******************************************************************************
 *                              C O N S T A N T S
 ******************************************************************************
 */

/******************************************************************************
 *                             D A T A   T Y P E S
 ******************************************************************************
 */

/******************************************************************************
 *                            P U B L I C   D A T A
 ******************************************************************************
 */

/******************************************************************************
 *                           P R I V A T E   D A T A
 ******************************************************************************
 */

/******************************************************************************
 *                                 M A C R O S
 ******************************************************************************
 */

/******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 ******************************************************************************
 */

/******************************************************************************
 *                              F U N C T I O N S
 ******************************************************************************
 */
void *kalGetP2pNetHdl(struct GLUE_INFO *prGlueInfo,
	uint32_t u4Idx, u_int8_t fgIsRole)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return NULL;
}

#if CFG_AP_80211KVR_INTERFACE
int32_t kalGetMulAPIfIdx(struct GLUE_INFO *prGlueInfo,
	uint32_t u4Idx, uint32_t *pu4IfIndex)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return 0;
}
#endif
void *kalGetP2pDevScanReq(struct GLUE_INFO *prGlueInfo)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return NULL;
}

u_int8_t kalGetP2pDevScanSpecificSSID(struct GLUE_INFO *prGlueInfo)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return 0;
}

void kalP2pIndicateListenOffloadEvent(
	struct GLUE_INFO *prGlueInfo,
	uint32_t event)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

void kalIdcRegisterRilNotifier(struct GLUE_INFO *prGlueInfo)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

void kalIdcUnregisterRilNotifier(void)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

void kalIdcGetRilInfo(void)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}
void kalP2pCsaNotifyWorkInit(struct BSS_INFO *prBssInfo)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}
#endif /* CFG_ENABLE_WIFI_DIRECT */
