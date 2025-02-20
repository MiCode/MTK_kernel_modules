// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "rate.c"
 *    \brief  This file contains the transmission rate handling routines.
 *
 *    This file contains the transmission rate handling routines for setting up
 *    ACK/CTS Rate, Highest Tx Rate, Lowest Tx Rate, Initial Tx Rate and do
 *    conversion between Rate Set and Data Rates.
 */

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "precomp.h"

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
/* The list of valid data rates. */
const uint8_t aucDataRate[] = {
	RATE_1M,		/* RATE_1M_INDEX = 0 */
	RATE_2M,		/* RATE_2M_INDEX */
	RATE_5_5M,		/* RATE_5_5M_INDEX */
	RATE_11M,		/* RATE_11M_INDEX */
	RATE_22M,		/* RATE_22M_INDEX */
	RATE_33M,		/* RATE_33M_INDEX */
	RATE_6M,		/* RATE_6M_INDEX */
	RATE_9M,		/* RATE_9M_INDEX */
	RATE_12M,		/* RATE_12M_INDEX */
	RATE_18M,		/* RATE_18M_INDEX */
	RATE_24M,		/* RATE_24M_INDEX */
	RATE_36M,		/* RATE_36M_INDEX */
	RATE_48M,		/* RATE_48M_INDEX */
	RATE_54M,		/* RATE_54M_INDEX */
	RATE_VHT_PHY,		/* RATE_VHT_PHY_INDEX */
	RATE_HT_PHY,		/* RATE_HT_PHY_INDEX */
	RATE_H2E_ONLY		/* RATE_H2E_ONLY_INDEX */
};

static const uint8_t aucDefaultAckCtsRateIndex[RATE_NUM_SW] = {
	RATE_1M_SW_INDEX,	/* RATE_1M_SW_INDEX = 0 */
	RATE_2M_SW_INDEX,	/* RATE_2M_SW_INDEX */
	RATE_5_5M_SW_INDEX,	/* RATE_5_5M_SW_INDEX */
	RATE_11M_SW_INDEX,	/* RATE_11M_SW_INDEX */
	RATE_1M_SW_INDEX,	/* RATE_22M_SW_INDEX - Not supported */
	RATE_1M_SW_INDEX,	/* RATE_33M_SW_INDEX - Not supported */
	RATE_6M_SW_INDEX,	/* RATE_6M_SW_INDEX */
	RATE_6M_SW_INDEX,	/* RATE_9M_SW_INDEX */
	RATE_12M_SW_INDEX,	/* RATE_12M_SW_INDEX */
	RATE_12M_SW_INDEX,	/* RATE_18M_SW_INDEX */
	RATE_24M_SW_INDEX,	/* RATE_24M_SW_INDEX */
	RATE_24M_SW_INDEX,	/* RATE_36M_SW_INDEX */
	RATE_24M_SW_INDEX,	/* RATE_48M_SW_INDEX */
	RATE_24M_SW_INDEX	/* RATE_54M_SW_INDEX */
};

const u_int8_t afgIsOFDMRate[RATE_NUM_SW] = {
	FALSE,			/* RATE_1M_INDEX = 0 */
	FALSE,			/* RATE_2M_INDEX */
	FALSE,			/* RATE_5_5M_INDEX */
	FALSE,			/* RATE_11M_INDEX */
	FALSE,			/* RATE_22M_INDEX - Not supported */
	FALSE,			/* RATE_33M_INDEX - Not supported */
	TRUE,			/* RATE_6M_INDEX */
	TRUE,			/* RATE_9M_INDEX */
	TRUE,			/* RATE_12M_INDEX */
	TRUE,			/* RATE_18M_INDEX */
	TRUE,			/* RATE_24M_INDEX */
	TRUE,			/* RATE_36M_INDEX */
	TRUE,			/* RATE_48M_INDEX */
	TRUE			/* RATE_54M_INDEX */
};

/*******************************************************************************
 *                             D A T A   T Y P E S
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
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
/*----------------------------------------------------------------------------*/
/*!
 * @brief Convert the given Supported Rate & Extended Supported Rate IE to the
 *        Operational Rate Set and Basic Rate Set, and also check if any Basic
 *        Rate Code is unknown by driver.
 *
 * @param[in] prIeSupportedRate          Pointer to the Supported Rate IE
 * @param[in] prIeExtSupportedRate       Pointer to the Ext Supported Rate IE
 * @param[out] pu2OperationalRateSet     Pointer to the Operational Rate Set
 * @param[out] pu2BSSBasicRateSet        Pointer to the Basic Rate Set
 * @param[out] pfgIsUnknownBSSBasicRate  Pointer to a Flag to indicate
 that
 *                                       Basic Rate Set has unknown Rate Code
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void
rateGetRateSetFromIEs(
		      struct IE_SUPPORTED_RATE_IOT *prIeSupportedRate,
		      struct IE_EXT_SUPPORTED_RATE *prIeExtSupportedRate,
		      uint16_t *pu2OperationalRateSet,
		      uint16_t *pu2BSSBasicRateSet,
		      u_int8_t *pfgIsUnknownBSSBasicRate)
{
	uint16_t u2OperationalRateSet = 0;
	uint16_t u2BSSBasicRateSet = 0;
	u_int8_t fgIsUnknownBSSBasicRate = FALSE;
	uint8_t ucRate;
	uint8_t i;
	uint32_t j;

	if (prIeSupportedRate) {
		/* NOTE(Kevin): Buffalo WHR-G54S's supported rate set
		 *   IE exceed 8.
		 * IE_LEN(pucIE) == 12, "1(B), 2(B), 5.5(B), 6(B), 9(B), 11(B),
		 * 12(B), 18(B), 24(B), 36(B), 48(B), 54(B)"
		 */
		/* ASSERT(prIeSupportedRate->ucLength
		 *  <= ELEM_MAX_LEN_SUP_RATES);
		 */
		ASSERT(prIeSupportedRate->ucLength <= RATE_NUM_SW);

		DBGLOG(RLM, LOUD, "Dump supported rate\n");
		DBGLOG_MEM8(RLM, LOUD, (uint8_t *) prIeSupportedRate,
			(uint32_t) prIeSupportedRate->ucLength);

		for (i = 0; i < prIeSupportedRate->ucLength &&
			    i < ELEM_MAX_LEN_SUP_RATES_IOT; i++) {
			ucRate =
			    prIeSupportedRate->aucSupportedRates[i] & RATE_MASK;

			/* Search all valid data rates */
			for (j = 0; j < ARRAY_SIZE(aucDataRate); j++) {
				if (ucRate == aucDataRate[j]) {
					u2OperationalRateSet |= BIT(j);

					if (prIeSupportedRate->aucSupportedRates
						[i] & RATE_BASIC_BIT)
						u2BSSBasicRateSet |= BIT(j);

					break;
				}
			}

			if (j == ARRAY_SIZE(aucDataRate) &&
			    prIeSupportedRate->aucSupportedRates[i] &
			     RATE_BASIC_BIT) {
				fgIsUnknownBSSBasicRate = TRUE;
				/* A data rate not list in the aucDataRate[] */
			}
		}
	}

	if (prIeExtSupportedRate) {
		/* ASSERT(prIeExtSupportedRate->ucLength
		 *  <= ELEM_MAX_LEN_EXTENDED_SUP_RATES);
		 */
		if (prIeExtSupportedRate->ucLength >
			ELEM_MAX_LEN_EXTENDED_SUP_RATES - 1)
			return;
		for (i = 0; i < prIeExtSupportedRate->ucLength &&
			    i < ELEM_MAX_LEN_EXTENDED_SUP_RATES; i++) {
			ucRate =
			    prIeExtSupportedRate->aucExtSupportedRates[i] &
			    RATE_MASK;

			/* Search all valid data rates */
			for (j = 0; j < ARRAY_SIZE(aucDataRate); j++) {
				if (ucRate == aucDataRate[j]) {
					u2OperationalRateSet |= BIT(j);

					if (prIeExtSupportedRate->
					    aucExtSupportedRates[i]
					    & RATE_BASIC_BIT)
						u2BSSBasicRateSet |= BIT(j);

					break;
				}
			}

			if (j == ARRAY_SIZE(aucDataRate) &&
			    prIeExtSupportedRate->aucExtSupportedRates[i] &
			     RATE_BASIC_BIT) {
				fgIsUnknownBSSBasicRate = TRUE;
				/* A data rate not list in the aucDataRate[] */
			}
		}
	}

	*pu2OperationalRateSet = u2OperationalRateSet;
	*pu2BSSBasicRateSet = u2BSSBasicRateSet;
	*pfgIsUnknownBSSBasicRate = fgIsUnknownBSSBasicRate;

	DBGLOG(RLM, TRACE, "OP rate:%d, Basic rate:%d, Unknown rate:%d\n",
		u2OperationalRateSet,
		u2BSSBasicRateSet,
		fgIsUnknownBSSBasicRate);

	return;

}				/* end of rateGetRateSetFromIEs() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief Convert the given Operational Rate Set & Basic Rate Set to the Rate
 *        Code Format for used in (Ext)Supportec Rate IE.
 *
 * @param[in] u2OperationalRateSet   Operational Rate Set
 * @param[in] u2BSSBasicRateSet      Basic Rate Set
 * @param[out] pucDataRates          Pointer to the Data Rate Buffer
 * @param[out] pucDataRatesLen       Pointer to the Data Rate Buffer Length
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void
rateGetDataRatesFromRateSet(uint16_t u2OperationalRateSet,
			    uint16_t u2BSSBasicRateSet,
			    uint8_t *pucDataRates,
			    uint8_t *pucDataRatesLen)
{
	uint32_t i, j;

	for (i = RATE_1M_SW_INDEX, j = 0; i < RATE_NUM_SW; i++) {
		if (u2OperationalRateSet & BIT(i)) {

			*(pucDataRates + j) = aucDataRate[i];

			if (u2BSSBasicRateSet & BIT(i))
				*(pucDataRates + j) |= RATE_BASIC_BIT;

			j++;
		}
	}

	*pucDataRatesLen = (uint8_t) j;

	return;

}				/* end of rateGetDataRatesFromRateSet() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief Get the highest rate from given Rate Set.
 *
 * \param[in] u2RateSet              Rate Set
 * \param[out] pucHighestRateIndex   Pointer to buffer of the Highest Rate Index
 *
 * \retval TRUE  Highest Rate Index was found
 * \retval FALSE Highest Rate Index was not found
 */
/*----------------------------------------------------------------------------*/
u_int8_t rateGetHighestRateIndexFromRateSet(uint16_t u2RateSet,
					    uint8_t *pucHighestRateIndex)
{
	int32_t i;

	for (i = RATE_54M_SW_INDEX; i >= RATE_1M_SW_INDEX; i--) {
		if (u2RateSet & BIT(i)) {
			*pucHighestRateIndex = (uint8_t) i;
			return TRUE;
		}
	}

	return FALSE;

}			/* end of rateGetHighestRateIndexFromRateSet() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief Get the lowest rate from given Rate Set.
 *
 * \param[in] u2RateSet              Rate Set
 * \param[out] pucLowestRateIndex    Pointer to buffer of the Lowest Rate Index
 *
 * \retval TRUE  Lowest Rate Index was found
 * \retval FALSE Lowest Rate Index was not found
 */
/*----------------------------------------------------------------------------*/
u_int8_t rateGetLowestRateIndexFromRateSet(uint16_t u2RateSet,
					   uint8_t *pucLowestRateIndex)
{
	uint32_t i;

	for (i = RATE_1M_SW_INDEX; i <= RATE_54M_SW_INDEX; i++) {
		if (u2RateSet & BIT(i)) {
			*pucLowestRateIndex = (uint8_t) i;
			return TRUE;
		}
	}

	return FALSE;

}				/* end of rateGetLowestRateIndexFromRateSet() */
