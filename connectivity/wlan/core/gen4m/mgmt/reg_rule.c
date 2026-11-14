// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
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

struct mtk_cc_dfs_regd_mapping {
	uint16_t countryCode;
	enum ENUM_REG_DOMAIN ucDfsRegion;
};

const struct mtk_cc_dfs_regd_mapping my_dfs_ca = {
	.countryCode = COUNTRY_CODE_CA,
	.ucDfsRegion = ENUM_RDM_FCC
};

const struct mtk_cc_dfs_regd_mapping my_dfs_cf = {
	.countryCode = COUNTRY_CODE_CF,
	.ucDfsRegion = ENUM_RDM_FCC
};

const struct mtk_cc_dfs_regd_mapping my_dfs_td = {
	.countryCode = COUNTRY_CODE_TD,
	.ucDfsRegion = ENUM_RDM_FCC
};

const struct mtk_cc_dfs_regd_mapping my_dfs_us = {
	.countryCode = COUNTRY_CODE_US,
	.ucDfsRegion = ENUM_RDM_FCC
};

const struct mtk_cc_dfs_regd_mapping my_dfs_tw = {
	.countryCode = COUNTRY_CODE_TW,
	.ucDfsRegion = ENUM_RDM_FCC
};

const struct mtk_cc_dfs_regd_mapping my_dfs_na = {
	.countryCode = COUNTRY_CODE_NA,
	.ucDfsRegion = ENUM_RDM_FCC
};

const struct mtk_cc_dfs_regd_mapping my_dfs_cn = {
	.countryCode = COUNTRY_CODE_CN,
	.ucDfsRegion = ENUM_RDM_CHN
};

const struct mtk_cc_dfs_regd_mapping my_dfs_jp = {
	.countryCode = COUNTRY_CODE_JP,
	.ucDfsRegion = ENUM_RDM_JAP
};

const struct mtk_cc_dfs_regd_mapping my_dfs_kr = {
	.countryCode = COUNTRY_CODE_KR,
	.ucDfsRegion = ENUM_RDM_KR
};

const struct mtk_cc_dfs_regd_mapping *g_prDfsCountryTable[] = {
	&my_dfs_ca,
	&my_dfs_cf,
	&my_dfs_us,
	&my_dfs_td,
	&my_dfs_tw,
	&my_dfs_na,
	&my_dfs_cn,
	&my_dfs_jp,
	&my_dfs_td,
	&my_dfs_kr,
	NULL /* this NULL SHOULD be at the end of the array */
};

uint8_t regCountryDfsMapping(struct ADAPTER *prAdapter)
{
	u8 idx = 0;
	const struct mtk_cc_dfs_regd_mapping *prRegd = NULL;

	DBGLOG(P2P, TRACE,
		"Country Code = 0x%04x\n",
		prAdapter->rWifiVar.u2CountryCode);
	while (g_prDfsCountryTable[idx]) {
		prRegd = g_prDfsCountryTable[idx];

		if (prRegd->countryCode ==
			prAdapter->rWifiVar.u2CountryCode)
			return (uint8_t) prRegd->ucDfsRegion;

		idx++;
	}

	return (uint8_t) ENUM_RDM_CE;
}


