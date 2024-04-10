/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*
 ** Id: //Department/DaVinci/BRANCHES/
 *      MT6620_WIFI_DRIVER_V2_3/include/nic/nic_rate.h#1
 */

/*! \file  nic_rate.h
 *    \brief This file contains the rate utility function of
 *	   IEEE 802.11 family for MediaTek 802.11 Wireless LAN Adapters.
 */


#ifndef _NIC_RATE_H
#define _NIC_RATE_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/*******************************************************************************
 *                         D A T A   T Y P E S
 *******************************************************************************
 */

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

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
uint32_t
nicGetPhyRateByMcsRate(
	IN uint8_t ucIdx,
	IN uint8_t ucBw,
	IN uint8_t ucGI
);

uint32_t
nicGetHwRateByPhyRate(
	IN uint8_t ucIdx
);

uint32_t
nicSwIndex2RateIndex(
	IN uint8_t ucRateSwIndex,
	OUT uint8_t *pucRateIndex,
	OUT uint8_t *pucPreambleOption
);

uint32_t
nicRateIndex2RateCode(
	IN uint8_t ucPreambleOption,
	IN uint8_t ucRateIndex,
	OUT uint16_t *pu2RateCode
);

uint32_t
nicRateCode2PhyRate(
	IN uint16_t u2RateCode,
	IN uint8_t ucBandwidth,
	IN uint8_t ucGI,
	IN uint8_t ucRateNss
);

uint32_t
nicRateCode2DataRate(
	IN uint16_t u2RateCode,
	IN uint8_t ucBandwidth,
	IN uint8_t ucGI
);

u_int8_t
nicGetRateIndexFromRateSetWithLimit(
	IN uint16_t u2RateSet,
	IN uint32_t u4PhyRateLimit,
	IN u_int8_t fgGetLowest,
	OUT uint8_t *pucRateSwIndex
);

uint16_t
nicRateInfo2RateCode(IN uint32_t  u4TxMode,
	IN uint32_t  u4Rate);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#endif /* _NIC_RATE_H */
