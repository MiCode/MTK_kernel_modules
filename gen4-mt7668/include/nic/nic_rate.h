/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*
** Id: //Department/DaVinci/BRANCHES/MT6620_WIFI_DRIVER_V2_3/include/nic/nic_rate.h#1
*/

/*! \file  nic_rate.h
*    \brief This file contains the rate utility function of
*	   IEEE 802.11 family for MediaTek 802.11 Wireless LAN Adapters.
*/


#ifndef _NIC_RATE_H
#define _NIC_RATE_H

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*                         D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
UINT_32
nicGetPhyRateByMcsRate(
	IN UINT_8 ucIdx,
	IN UINT_8 ucBw,
	IN UINT_8 ucGI
	);

UINT_32
nicGetHwRateByPhyRate(
	IN UINT_8 ucIdx
	);

WLAN_STATUS
nicSwIndex2RateIndex(
	IN UINT_8 ucRateSwIndex,
	OUT PUINT_8 pucRateIndex,
	OUT PUINT_8 pucPreambleOption
	);

WLAN_STATUS
nicRateIndex2RateCode(
	IN UINT_8 ucPreambleOption,
	IN UINT_8 ucRateIndex,
	OUT PUINT_16 pu2RateCode
	);

UINT_32
nicRateCode2PhyRate(
	IN UINT_16 u2RateCode,
	IN UINT_8 ucBandwidth,
	IN UINT_8 ucGI,
	IN UINT_8 ucRateNss
	);

UINT_32
nicRateCode2DataRate(
	IN UINT_16 u2RateCode,
	IN UINT_8 ucBandwidth,
	IN UINT_8 ucGI
	);

BOOLEAN
nicGetRateIndexFromRateSetWithLimit(
	IN UINT_16 u2RateSet,
	IN UINT_32 u4PhyRateLimit,
	IN BOOLEAN fgGetLowest,
	OUT PUINT_8 pucRateSwIndex
	);

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

#endif /* _NIC_RATE_H */
