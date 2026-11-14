// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/****************************************************************************
 *[File]             gl_qa_agent.c
 *[Version]          v1.0
 *[Revision Date]    2019/01/01
 *[Author]
 *[Description]
 *    DUT API implementation for test tool
 *[Copyright]
 *    Copyright (C) 2010 MediaTek Incorporation. All Rights Reserved.
 ****************************************************************************/


/*****************************************************************************
 *                         C O M P I L E R   F L A G S
 *****************************************************************************
 */

/*****************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *****************************************************************************
 */

#include "gl_os.h"

#include "precomp.h"

/*****************************************************************************
 *                              C O N S T A N T S
 *****************************************************************************
 */

/*****************************************************************************
 *                             D A T A   T Y P E S
 *****************************************************************************
 */

/*****************************************************************************
 *                            P U B L I C   D A T A
 *****************************************************************************
 */
/* export to other common part file */
#if CFG_SUPPORT_QA_TOOL
struct PARAM_RX_STAT g_HqaRxStat;
#if (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1)
struct TESTMODE_CAP g_HqaCap;
#endif
#if (CFG_SUPPORT_CONNAC3X == 0) && (CFG_SUPPORT_CONNAC5X == 0)
uint32_t u4RxStatSeqNum;
#else
uint16_t u2RxStatSeqNum;
#endif

#if (CONFIG_WLAN_SERVICE == 1)
struct list_mode_event g_HqaListModeStatus;
#endif
#endif

/*****************************************************************************
 *                           P R I V A T E   D A T A
 *****************************************************************************
 */


/*****************************************************************************
 *                                 M A C R O S
 *****************************************************************************
 */

/*****************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *****************************************************************************
 */


/*****************************************************************************
 *                              F U N C T I O N S
 *****************************************************************************
 */
int32_t connacSetICapStart(struct GLUE_INFO *prGlueInfo,
			   uint32_t u4Trigger, uint32_t u4RingCapEn,
			   uint32_t u4Event, uint32_t u4Node, uint32_t u4Len,
			   uint32_t u4StopCycle,
			   uint32_t u4BW, uint32_t u4MacTriggerEvent,
			   uint32_t u4SourceAddrLSB,
			   uint32_t u4SourceAddrMSB, uint32_t u4Band)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

int32_t connacGetICapStatus(struct GLUE_INFO *prGlueInfo)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

int32_t connacGetICapIQData(struct GLUE_INFO *prGlueInfo,
			    uint8_t *pData, uint32_t u4IQType, uint32_t u4WFNum)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

int32_t mt6632SetICapStart(struct GLUE_INFO *prGlueInfo,
	uint32_t u4Trigger, uint32_t u4RingCapEn,
	uint32_t u4Event, uint32_t u4Node, uint32_t u4Len,
	uint32_t u4StopCycle, uint32_t u4BW, uint32_t u4MacTriggerEvent,
	uint32_t u4SourceAddrLSB, uint32_t u4SourceAddrMSB, uint32_t u4Band)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

int32_t mt6632GetICapStatus(struct GLUE_INFO *prGlueInfo)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}
