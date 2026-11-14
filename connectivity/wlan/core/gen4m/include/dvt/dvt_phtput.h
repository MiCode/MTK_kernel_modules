/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "dvt_phtput.h"
 *    \brief  The declaration of nic ph-tput functions
 *
 *    Detail description.
 */


#ifndef _DVT_PHTPUT_H
#define _DVT_PHTPUT_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

struct PhTputSetting {
	uint16_t u2CmdId;
	uint8_t fgIsSec;
};

enum ENUM_PHPTUT_SETTING {
	/*STA*/
	ENUM_PHTPUT_LEGACY_OPEN_STA		= 1,
	ENUM_PHTPUT_LEGACY_SEC_GCMP256_STA	= 2,
	ENUM_PHTPUT_LEGACY_SEC_WEP128_STA	= 3,
	ENUM_PHTPUT_LEGACY_SEC_TKIP_STA		= 4,
	ENUM_PHTPUT_LEGACY_SEC_AES_STA		= 5,
	ENUM_PHTPUT_MLO_OPEN_BN0_BN2_STA	= 101,
	ENUM_PHTPUT_MLO_SEC_BN0_BN2_STA		= 102,
	ENUM_PHTPUT_MLO_OPEN_BN0_BN1_STA	= 103,
	ENUM_PHTPUT_MLO_SEC_BN0_BN1_STA		= 104,
	ENUM_PHTPUT_DBDC_OPEN_BN0_BN2_STA	= 201,
	ENUM_PHTPUT_DBDC_SEC_BN0_BN2_STA	= 202,
	ENUM_PHTPUT_DBDC_OPEN_BN0_BN1_STA	= 203,
	ENUM_PHTPUT_DBDC_SEC_BN0_BN1_STA	= 204,
	/*AP*/
	ENUM_PHTPUT_LEGACY_OPEN_AP		= 1001,
	ENUM_PHTPUT_LEGACY_SEC_GCMP256_AP	= 1002,
	ENUM_PHTPUT_LEGACY_SEC_WEP128_AP	= 1003,
	ENUM_PHTPUT_LEGACY_SEC_TKIP_AP		= 1004,
	ENUM_PHTPUT_LEGACY_SEC_AES_AP		= 1005,
	ENUM_PHTPUT_MLO_OPEN_BN0_BN2_AP		= 1101,
	ENUM_PHTPUT_MLO_SEC_BN0_BN2_AP		= 1102,
	ENUM_PHTPUT_MLO_OPEN_BN0_BN1_AP		= 1103,
	ENUM_PHTPUT_MLO_SEC_BN0_BN1_AP		= 1104,
	ENUM_PHTPUT_DBDC_OPEN_BN0_BN2_AP	= 1201,
	ENUM_PHTPUT_DBDC_SEC_BN0_BN2_AP		= 1202,
	ENUM_PHTPUT_DBDC_OPEN_BN0_BN1_AP	= 1203,
	ENUM_PHTPUT_DBDC_SEC_BN0_BN1_AP		= 1204
};

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
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

uint32_t dvtSetupPhTput(struct net_device *prNetDev,
			uint32_t u4CaseIndex);
uint32_t dvtActivateNetworkPhTput(struct net_device *prNetDev,
			uint8_t ucBssIndex,
			struct PhTputSetting *prPhtputSetting);
uint32_t dvtDeactivateNetworkPhTput(struct net_device *prNetDev,
			uint8_t ucBssIndex);

#endif /* _DVT_PHTPUT_H */

