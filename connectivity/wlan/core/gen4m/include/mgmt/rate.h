/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file  rate.h
 *  \brief This file contains the rate utility function of
 *         IEEE 802.11 family for MediaTek 802.11 Wireless LAN Adapters.
 */

#ifndef _RATE_H
#define _RATE_H

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
extern const uint8_t aucDataRate[];

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
/*----------------------------------------------------------------------------*/
/* Routines in rate.c                                                         */
/*----------------------------------------------------------------------------*/
void
rateGetRateSetFromIEs(struct IE_SUPPORTED_RATE_IOT
		      *prIeSupportedRate,
		      struct IE_EXT_SUPPORTED_RATE *prIeExtSupportedRate,
		      uint16_t *pu2OperationalRateSet,
		      uint16_t *pu2BSSBasicRateSet,
		      u_int8_t *pfgIsUnknownBSSBasicRate);

void
rateGetDataRatesFromRateSet(uint16_t
			    u2OperationalRateSet, uint16_t u2BSSBasicRateSet,
			    uint8_t *pucDataRates,
			    uint8_t *pucDataRatesLen);

u_int8_t rateGetHighestRateIndexFromRateSet(
	uint16_t u2RateSet, uint8_t *pucHighestRateIndex);

u_int8_t rateGetLowestRateIndexFromRateSet(
	uint16_t u2RateSet, uint8_t *pucLowestRateIndex);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#endif /* _RATE_H */
