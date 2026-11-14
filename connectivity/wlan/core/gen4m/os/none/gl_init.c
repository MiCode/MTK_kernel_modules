// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/****************************************************************************
 *[File]             gl_init.c
 *[Version]          v1.0
 *[Revision Date]    2019/01/01
 *[Author]
 *[Description]
 *    DUT initialization API implementation for os related logic.
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

#include "precomp.h"

#include "gl_os.h"

/*****************************************************************************
 *                              C O N S T A N T S
 *****************************************************************************
 */

enum ENUM_NVRAM_STATE g_NvramFsm = NVRAM_STATE_INIT;
uint8_t g_aucNvram[MAX_CFG_FILE_WIFI_REC_SIZE];
uint8_t g_aucNvram_OnlyPreCal[MAX_CFG_FILE_WIFI_RECAL_SIZE];

/*****************************************************************************
 *                             D A T A   T Y P E S
 *****************************************************************************
 */

/*****************************************************************************
 *                            P U B L I C   D A T A
 *****************************************************************************
 */

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
uint32_t wlanDownloadBufferBin(struct ADAPTER *prAdapter)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

uint32_t wlanConnac2XDownloadBufferBin(struct ADAPTER *prAdapter)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

uint32_t wlanConnac3XDownloadBufferBin(struct ADAPTER *prAdapter)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

uint32_t wlanConnacDownloadBufferBin(struct ADAPTER *prAdapter)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

void *wlanGetAisNetDev(struct GLUE_INFO *prGlueInfo,
	uint8_t ucAisIndex)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return NULL;
}

void *wlanGetP2pNetDev(struct GLUE_INFO *prGlueInfo,
	uint8_t ucP2pIndex)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return NULL;
}

void *wlanGetNetDev(struct GLUE_INFO *prGlueInfo,
	uint8_t ucBssIndex)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return NULL;
}
