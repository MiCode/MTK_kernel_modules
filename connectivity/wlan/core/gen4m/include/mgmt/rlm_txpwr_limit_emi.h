/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "rlm_tx_pwr_limit_emi.h"
 *    \brief
 */

#ifndef _RLM_TX_PWR_LIMIT_EMI_H
#define _RLM_TX_PWR_LIMIT_EMI_H

#if (CFG_SUPPORT_PWR_LMT_EMI == 1)

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
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

enum ENUM_PWR_LIMIT_TYPE {
	PWR_LIMIT_TYPE_LEGACY = 0,
	PWR_LIMIT_TYPE_HE = 1,
	PWR_LIMIT_TYPE_EHT = 2,
	PWR_LIMIT_TYPE_NUM,
};

#if (CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1)
/* add new element at the end for backward compatibility*/
enum ENUM_PWR_LIMIT_MULTIBAND_TYPE {
	PWR_LIMIT_MULTIBAND_TYPE_0_1 = 0,
	PWR_LIMIT_MULTIBAND_TYPE_0_2 = 1,
	PWR_LIMIT_MULTIBAND_TYPE_1_2 = 2,
	PWR_LIMIT_MULTIBAND_TYPE_0_1_2 = 3,
	PWR_LIMIT_MULTIBAND_TYPE_NUM,
};
#endif /* CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1 */

enum ENUM_CMD_PWR_LIMIT_TYPE {
	PWR_LIMIT_CMD_TYPE_ANT_V2 = 7,
};

/*
 * Limit type = PWR_LIMIT_TYPE_COMP_LEGACY(0)
 * We use same structure with V1/V2, merge in CFG_SUPPORT_PWR_LMT_EMI == 1
 */

enum ENUM_POWER_LIMIT_LEGACY {
	PWR_LIMIT_LEGACY_CCK_L,
	PWR_LIMIT_LEGACY_CCK_H,
	PWR_LIMIT_LEGACY_OFDM_L,
	PWR_LIMIT_LEGACY_OFDM_H,
	PWR_LIMIT_LEGACY_20M_L,
	PWR_LIMIT_LEGACY_20M_H,
	PWR_LIMIT_LEGACY_40M_L,
	PWR_LIMIT_LEGACY_40M_H,
	PWR_LIMIT_LEGACY_80M_L,
	PWR_LIMIT_LEGACY_80M_H,
	PWR_LIMIT_LEGACY_160M_L,
	PWR_LIMIT_LEGACY_160M_H,
	PWR_LIMIT_LEGACY_NUM,
};

/* Limit type = PWR_LIMIT_TYPE_COMP_HE(1)
 * We use same structure with BW80/160 in CFG_SUPPORT_PWR_LMT_EMI == 1
 */

enum ENUM_POWER_LIMIT_HE {
	PWR_LIMIT_HE_RU26_L, /* MCS0~4 */
	PWR_LIMIT_HE_RU26_H, /* MCS5~9 */
	PWR_LIMIT_HE_RU26_U, /* MCS10~11 */

	PWR_LIMIT_HE_RU52_L, /* MCS0~4 */
	PWR_LIMIT_HE_RU52_H, /* MCS5~9 */
	PWR_LIMIT_HE_RU52_U, /* MCS10~11 */

	PWR_LIMIT_HE_RU106_L, /* MCS0~4 */
	PWR_LIMIT_HE_RU106_H, /* MCS5~9 */
	PWR_LIMIT_HE_RU106_U, /* MCS10~11 */

	PWR_LIMIT_HE_RU242_L, /* MCS0~4 */
	PWR_LIMIT_HE_RU242_H, /* MCS5~9 */
	PWR_LIMIT_HE_RU242_U, /* MCS10~11 */

	PWR_LIMIT_HE_RU484_L, /* MCS0~4 */
	PWR_LIMIT_HE_RU484_H, /* MCS5~9 */
	PWR_LIMIT_HE_RU484_U, /* MCS10~11 */

	PWR_LIMIT_HE_RU996_L, /* MCS0~4 */
	PWR_LIMIT_HE_RU996_H, /* MCS5~9 */
	PWR_LIMIT_HE_RU996_U, /* MCS10~11 */

	PWR_LIMIT_HE_RU1992_L, /* MCS0~4 */
	PWR_LIMIT_HE_RU1992_H, /* MCS5~9 */
	PWR_LIMIT_HE_RU1992_U, /* MCS10~11 */
	PWR_LIMIT_HE_NUM,
};

/*
 * Limit type = PWR_LIMIT_TYPE_COMP_EHT(2)
 * We use same structure with EHT/EHT6G in CFG_SUPPORT_PWR_LMT_EMI == 1
 */
 #if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
enum ENUM_POWER_LIMIT_EHT {
	PWR_LIMIT_EHT_RU26_L, /* MCS0~4 */
	PWR_LIMIT_EHT_RU26_H, /* MCS5~9 */
	PWR_LIMIT_EHT_RU26_U, /* MCS10~15 */

	PWR_LIMIT_EHT_RU52_L, /* MCS0~4 */
	PWR_LIMIT_EHT_RU52_H, /* MCS5~9 */
	PWR_LIMIT_EHT_RU52_U, /* MCS10~15 */

	PWR_LIMIT_EHT_RU106_L, /* MCS0~4 */
	PWR_LIMIT_EHT_RU106_H, /* MCS5~9 */
	PWR_LIMIT_EHT_RU106_U, /* MCS10~15 */

	PWR_LIMIT_EHT_RU242_L, /* MCS0~4 */
	PWR_LIMIT_EHT_RU242_H, /* MCS5~9 */
	PWR_LIMIT_EHT_RU242_U, /* MCS10~15 */

	PWR_LIMIT_EHT_RU484_L, /* MCS0~4 */
	PWR_LIMIT_EHT_RU484_H, /* MCS5~9 */
	PWR_LIMIT_EHT_RU484_U, /* MCS10~15 */

	PWR_LIMIT_EHT_RU996_L, /* MCS0~4 */
	PWR_LIMIT_EHT_RU996_H, /* MCS5~9 */
	PWR_LIMIT_EHT_RU996_U, /* MCS10~15 */

	PWR_LIMIT_EHT_RU996X2_L, /* MCS0~4 */
	PWR_LIMIT_EHT_RU996X2_H, /* MCS5~9 */
	PWR_LIMIT_EHT_RU996X2_U, /* MCS10~15 */

	PWR_LIMIT_EHT_RU996X4_L, /* MCS0~4 */
	PWR_LIMIT_EHT_RU996X4_H, /* MCS5~9 */
	PWR_LIMIT_EHT_RU996X4_U, /* MCS10~15 */

	PWR_LIMIT_EHT_RU26_52_L, /* MCS0~4 */
	PWR_LIMIT_EHT_RU26_52_H, /* MCS5~9 */
	PWR_LIMIT_EHT_RU26_52_U, /* MCS10~15 */

	PWR_LIMIT_EHT_RU26_106_L, /* MCS0~4 */
	PWR_LIMIT_EHT_RU26_106_H, /* MCS5~9 */
	PWR_LIMIT_EHT_RU26_106_U, /* MCS10~15 */

	PWR_LIMIT_EHT_RU484_242_L, /* MCS0~4 */
	PWR_LIMIT_EHT_RU484_242_H, /* MCS5~9 */
	PWR_LIMIT_EHT_RU484_242_U, /* MCS10~15 */

	PWR_LIMIT_EHT_RU996_484_L, /* MCS0~4 */
	PWR_LIMIT_EHT_RU996_484_H, /* MCS5~9 */
	PWR_LIMIT_EHT_RU996_484_U, /* MCS10~15 */

	PWR_LIMIT_EHT_RU996_484_242_L, /* MCS0~4 */
	PWR_LIMIT_EHT_RU996_484_242_H, /* MCS5~9 */
	PWR_LIMIT_EHT_RU996_484_242_U, /* MCS10~15 */

	PWR_LIMIT_EHT_RU996X2_484_L, /* MCS0~4 */
	PWR_LIMIT_EHT_RU996X2_484_H, /* MCS5~9 */
	PWR_LIMIT_EHT_RU996X2_484_U, /* MCS10~15 */

	PWR_LIMIT_EHT_RU996X3_L, /* MCS0~4 */
	PWR_LIMIT_EHT_RU996X3_H, /* MCS5~9 */
	PWR_LIMIT_EHT_RU996X3_U, /* MCS10~15 */

	PWR_LIMIT_EHT_RU996X3_484_L, /* MCS0~4 */
	PWR_LIMIT_EHT_RU996X3_484_H, /* MCS5~9 */
	PWR_LIMIT_EHT_RU996X3_484_U, /* MCS10~15 */

	PWR_LIMIT_EHT_NUM,
};
#endif

#if (CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1)
/* add new element at the end for backward compatibility*/
enum ENUM_POWER_LIMIT_MULTIBAND {
	PWR_LIMIT_MULTIBAND_RU26,
	PWR_LIMIT_MULTIBAND_RU52,
	PWR_LIMIT_MULTIBAND_RU106,
	PWR_LIMIT_MULTIBAND_RU242,
	PWR_LIMIT_MULTIBAND_RU484,
	PWR_LIMIT_MULTIBAND_RU996,
	PWR_LIMIT_MULTIBAND_RU996X2,
	PWR_LIMIT_MULTIBAND_RU996X4,
	PWR_LIMIT_MULTIBAND_RU26_52,
	PWR_LIMIT_MULTIBAND_RU26_106,
	PWR_LIMIT_MULTIBAND_RU484_242,
	PWR_LIMIT_MULTIBAND_RU996_484,
	PWR_LIMIT_MULTIBAND_RU996_484_242,
	PWR_LIMIT_MULTIBAND_RU996X2_484,
	PWR_LIMIT_MULTIBAND_RU996X3,
	PWR_LIMIT_MULTIBAND_RU996X3_484,
	PWR_LIMIT_MULTIBAND_NUM,
};
#endif /* CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1 */

struct CHANNEL_POWER_LIMIT_LEGACY {
	uint8_t ucCentralCh;
	int8_t cPwrLimitCCK_L; /* CCK_L, 1M,2M */
	int8_t cPwrLimitCCK_H; /* CCK_H, 5.5M,11M */
	int8_t cPwrLimitOFDM_L; /* OFDM_L,  6M ~ 18M */
	int8_t cPwrLimitOFDM_H; /* OFDM_H, 24M ~ 54M */
	int8_t cPwrLimit20L; /* MCS0~4 */
	int8_t cPwrLimit20H; /* MCS5~8 */
	int8_t cPwrLimit40L; /* MCS0~4 */
	int8_t cPwrLimit40H; /* MCS5~9 */
	int8_t cPwrLimit80L; /* MCS0~4 */
	int8_t cPwrLimit80H; /* MCS5~9 */
	int8_t cPwrLimit160L; /* MCS0~4 */
	int8_t cPwrLimit160H; /* MCS5~9 */

	uint8_t ucFlag; /*Not used in driver*/
	uint8_t ucValid;
};

struct CHANNEL_POWER_LIMIT_HE {
	uint8_t ucCentralCh;
	int8_t cPwrLimitRU26L; /* MCS0~4 */
	int8_t cPwrLimitRU26H; /* MCS5~9 */
	int8_t cPwrLimitRU26U; /* MCS10~11 */

	int8_t cPwrLimitRU52L; /* MCS0~4 */
	int8_t cPwrLimitRU52H; /* MCS5~9 */
	int8_t cPwrLimitRU52U; /* MCS10~11 */

	int8_t cPwrLimitRU106L; /* MCS0~4 */
	int8_t cPwrLimitRU106H; /* MCS5~9 */
	int8_t cPwrLimitRU106U; /* MCS10~11 */
	/*RU242/SU20*/
	int8_t cPwrLimitRU242L; /* MCS0~4 */
	int8_t cPwrLimitRU242H; /* MCS5~9 */
	int8_t cPwrLimitRU242U; /* MCS10~11 */
	/*RU484/SU40*/
	int8_t cPwrLimitRU484L; /* MCS0~4 */
	int8_t cPwrLimitRU484H; /* MCS5~9 */
	int8_t cPwrLimitRU484U; /* MCS10~11 */
	/*RU996/SU80*/
	int8_t cPwrLimitRU996L; /* MCS0~4 */
	int8_t cPwrLimitRU996H; /* MCS5~9 */
	int8_t cPwrLimitRU996U; /* MCS10~11 */
	/*RU1992/SU160*/
	int8_t cPwrLimitRU1992L; /* MCS0~4 */
	int8_t cPwrLimitRU1992H; /* MCS5~9 */
	int8_t cPwrLimitRU1992U; /* MCS10~11 */

	uint8_t ucFlag;
	uint8_t ucValid;

};

#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
struct CHANNEL_POWER_LIMIT_EHT {
	uint8_t ucCentralCh;
	int8_t cPwrLimitEHT26L; /* MCS0~4 */
	int8_t cPwrLimitEHT26H; /* MCS5~9 */
	int8_t cPwrLimitEHT26U; /* MCS10~15 */

	int8_t cPwrLimitEHT52L; /* MCS0~4 */
	int8_t cPwrLimitEHT52H; /* MCS5~9 */
	int8_t cPwrLimitEHT52U; /* MCS10~15 */

	int8_t cPwrLimitEHT106L; /* MCS0~4 */
	int8_t cPwrLimitEHT106H; /* MCS5~9 */
	int8_t cPwrLimitEHT106U; /* MCS10~15 */
	/*RU242/SU20*/
	int8_t cPwrLimitEHT242L; /* MCS0~4 */
	int8_t cPwrLimitEHT242H; /* MCS5~9 */
	int8_t cPwrLimitEHT242U; /* MCS10~15 */
	/*RU484/SU40*/
	int8_t cPwrLimitEHT484L; /* MCS0~4 */
	int8_t cPwrLimitEHT484H; /* MCS5~9 */
	int8_t cPwrLimitEHT484U; /* MCS10~15 */
	/*RU996/SU80*/
	int8_t cPwrLimitEHT996L; /* MCS0~4 */
	int8_t cPwrLimitEHT996H; /* MCS5~9 */
	int8_t cPwrLimitEHT996U; /* MCS10~15 */
	/*RU1992/SU160*/
	int8_t cPwrLimitEHT996X2L; /* MCS0~4 */
	int8_t cPwrLimitEHT996X2H; /* MCS5~9 */
	int8_t cPwrLimitEHT996X2U; /* MCS10~15 */
	/*RU1992/SU320*/
	int8_t cPwrLimitEHT996X4L; /* MCS0~4 */
	int8_t cPwrLimitEHT996X4H; /* MCS5~9 */
	int8_t cPwrLimitEHT996X4U; /* MCS10~15 */

	int8_t cPwrLimitEHT26_52L; /* MCS0~4 */
	int8_t cPwrLimitEHT26_52H; /* MCS5~9 */
	int8_t cPwrLimitEHT26_52U; /* MCS10~15 */

	int8_t cPwrLimitEHT26_106L; /* MCS0~4 */
	int8_t cPwrLimitEHT26_106H; /* MCS5~9 */
	int8_t cPwrLimitEHT26_106U; /* MCS10~15 */

	int8_t cPwrLimitEHT484_242L; /* MCS0~4 */
	int8_t cPwrLimitEHT484_242H; /* MCS5~9 */
	int8_t cPwrLimitEHT484_242U; /* MCS10~15 */

	int8_t cPwrLimitEHT996_484L; /* MCS0~4 */
	int8_t cPwrLimitEHT996_484H; /* MCS5~9 */
	int8_t cPwrLimitEHT996_484U; /* MCS10~15 */

	int8_t cPwrLimitEHT996_484_242L; /* MCS0~4 */
	int8_t cPwrLimitEHT996_484_242H; /* MCS5~9 */
	int8_t cPwrLimitEHT996_484_242U; /* MCS10~15 */

	int8_t cPwrLimitEHT996X2_484L; /* MCS0~4 */
	int8_t cPwrLimitEHT996X2_484H; /* MCS5~9 */
	int8_t cPwrLimitEHT996X2_484U; /* MCS10~15 */

	int8_t cPwrLimitEHT996X3L; /* MCS0~4 */
	int8_t cPwrLimitEHT996X3H; /* MCS5~9 */
	int8_t cPwrLimitEHT996X3U; /* MCS10~15 */

	int8_t cPwrLimitEHT996X3_484L; /* MCS0~4 */
	int8_t cPwrLimitEHT996X3_484H; /* MCS5~9 */
	int8_t cPwrLimitEHT996X3_484U; /* MCS10~15 */

	uint8_t ucFlag;
	uint8_t ucValid;

};
#endif

/*Must Keep the max channel number in each rf band*/
#define MAX_SUPPORT_CHANNEL_NUMBER 300

struct SET_COUNTRY_CHANNEL_POWER_LIMIT {
	uint16_t u2CountryCode;
	uint8_t ucNum; /*Numbers of channel to set power limit*/
	uint8_t ucLimitType;
	uint8_t ucVersion;

	union {
		struct CHANNEL_POWER_LIMIT_LEGACY
			rLegacy[MAX_SUPPORT_CHANNEL_NUMBER];
		struct CHANNEL_POWER_LIMIT_HE
			rHE[MAX_SUPPORT_CHANNEL_NUMBER];
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
		struct CHANNEL_POWER_LIMIT_EHT
			rEHT[MAX_SUPPORT_CHANNEL_NUMBER];
#endif
	} u;
};

#pragma pack(push, 1)
struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_LEGACY {
	uint8_t aucCountryCode[2];
	int16_t i2CentralCh;
	int8_t aucPwrLimit[PWR_LIMIT_LEGACY_NUM];
};
#pragma pack(pop)
#define CFG_TBL_SIZE_LEGACY \
	(sizeof(struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_LEGACY))

#pragma pack(push, 1)
struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_HE {
	uint8_t aucCountryCode[2];
	int16_t i2CentralCh;
	int8_t aucPwrLimit[PWR_LIMIT_HE_NUM];
};
#pragma pack(pop)
#define CFG_TBL_SIZE_HE \
	(sizeof(struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_HE))

#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
#pragma pack(push, 1)
struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_EHT {
	uint8_t aucCountryCode[2];
	int16_t i2CentralCh;
	int8_t aucPwrLimit[PWR_LIMIT_EHT_NUM];
};
#pragma pack(pop)
#define CFG_TBL_SIZE_EHT \
	(sizeof(struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_EHT))
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */

struct EMI_POWER_LIMIT_INFO {
	uint32_t u4EmiAddrOffset;
	uint8_t u1LimitType;
	uint8_t u1Size;
	uint16_t u2ChannelNum;
	uint8_t u1reserve;
};

struct TXP_REQUEST_CHANNEL_ELEMENT {
	struct LINK_ENTRY node;
	uint8_t ucBssIndex;
	uint8_t ucTokenID;
};

struct TXP_LIMIT_EMI_CTRL {
	int32_t i4PwrLmtLockCounter;
	struct LINK rReqChLUT;
};

enum ENUM_CONNECTION_NOTIFIED_REASON {
	CNM_REQUEST_CHANNEL,
	NAN_INIT,
	NAN_TIMELINE_UPDATE,
	CNM_RLM_SYNC_OP_PARAMS
};

enum ENUM_TX_PWR_EMI_STATUS_ACTION {
	TX_PWR_EMI_STATUS_ACTION_REQUEST_CHANNEL_START,
	TX_PWR_EMI_STATUS_ACTION_REQUEST_CHANNEL_END,
	TX_PWR_EMI_STATUS_ACTION_UPDATE_CMD,
	TX_PWR_EMI_STATUS_ACTION_UPDATE_EVENT,
	TX_PWR_EMI_STATUS_ACTION_CHECK,
	TX_PWR_EMI_STATUS_ACTION_CLEAR,
	TX_PWR_EMI_STATUS_ACTION_NAN_INIT_START,
	TX_PWR_EMI_STATUS_ACTION_NAN_INIT_END,
	TX_PWR_EMI_STATUS_ACTION_NAN_TIMELINE_UPDATE_START,
	TX_PWR_EMI_STATUS_ACTION_NAN_TIMELINE_UPDATE_END,
	TX_PWR_EMI_STATUS_ACTION_CNM_RLM_SYNC_OP_PARAMS_START,
	TX_PWR_EMI_STATUS_ACTION_CNM_RLM_SYNC_OP_PARAMS_END,
};

enum ENUM_TX_PWR_EMI_SCENARIO_TYPE {
	TX_PWR_EMI_SCENARIO_TYPE_CONNECTION,
	TX_PWR_EMI_SCENARIO_TYPE_UPDATE,
	TX_PWR_EMI_SCENARIO_TYPE_NAN_INIT,
	TX_PWR_EMI_SCENARIO_TYPE_NAN_TIMELINE_UPDATE,
	TX_PWR_EMI_SCENARIO_TYPE_CNM_RLM_SYNC_OP_PARAMS,
	TX_PWR_EMI_SCENARIO_TYPE_NUM,
};

enum ENUM_PWR_LIMIT_RF_BAND {
	PWR_LIMIT_RF_BAND_2G4,
	PWR_LIMIT_RF_BAND_5G,
#if (CFG_SUPPORT_WIFI_6G == 1)
	PWR_LIMIT_RF_BAND_6G,
#endif
	PWR_LIMIT_RF_BAND_NUM
};

enum ENUM_PWR_LIMIT_PROTOCOL {
	PWR_LIMIT_PROTOCOL_LEGACY,
	PWR_LIMIT_PROTOCOL_HE,
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	PWR_LIMIT_PROTOCOL_EHT,
#endif
	PWR_LIMIT_PROTOCOL_NUM,
};

enum ENUM_PWR_LIMIT_CONFIG_BASE {
	PWR_LIMIT_CONFIG_BASE_2G4_5G,
#if (CFG_SUPPORT_WIFI_6G == 1)
	PWR_LIMIT_CONFIG_BASE_6G,
	PWR_LIMIT_CONFIG_BASE_6G_VLP,
	PWR_LIMIT_CONFIG_BASE_6G_SP,
#endif /* CFG_SUPPORT_WIFI_6G == 1 */
	PWR_LIMIT_CONFIG_BASE_NUM,
};

#if (CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1)
struct TX_PWR_CTRL_MULTIBAND_PWR {
	uint8_t u1RfBand;
	uint8_t u1Startch;
	uint8_t u1Endch;
	int8_t Op[PWR_LIMIT_MULTIBAND_NUM];
	int8_t i1MBPwrLmt[PWR_LIMIT_MULTIBAND_NUM];
};

struct TX_PWR_CTRL_MULTIBAND_SETTING {
	uint8_t u1ChGrpCnt;
	struct TX_PWR_CTRL_MULTIBAND_PWR
		rMultiBandPwr[MAX_SUPPORT_CHANNEL_NUMBER];
};

struct TX_PWR_CTRL_MULTIBAND_EMI_DATA {
	uint8_t u1Channel[MAX_SUPPORT_CHANNEL_NUMBER];
	int8_t i1MBPwrLmt[MAX_SUPPORT_CHANNEL_NUMBER][PWR_LIMIT_MULTIBAND_NUM];
};

struct TX_PWR_BAND_MASK_TBL {
	enum ENUM_PWR_LIMIT_MULTIBAND_TYPE idx;
	uint8_t u1BandMask;
};
#endif /* CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1 */
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
#define PWR_LIMIT_COUNTRY_CFG_TBL_SET_LEGACY(base, pData, num) \
{ \
	g_rlmPowerLimitConfigTable[base].Legacy.table = \
		(struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_LEGACY *) \
		pData; \
	g_rlmPowerLimitConfigTable[base].Legacy.table_num = num; \
	DBGLOG(RLM, TRACE, \
		"PWR_LIMIT_COUNTRY_CFG_TBL_SET_LEGACY [base:%d]NUM:%d\n", \
		base, g_rlmPowerLimitConfigTable[base].Legacy.table_num); \
}

#define PWR_LIMIT_COUNTRY_CFG_TBL_SET_HE(base, pData, num) \
{ \
	g_rlmPowerLimitConfigTable[base].HE.table = \
		(struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_HE *)pData; \
	g_rlmPowerLimitConfigTable[base].HE.table_num = num; \
	DBGLOG(RLM, TRACE, \
		"PWR_LIMIT_COUNTRY_CFG_TBL_SET_HE [base:%d]NUM:%d\n", \
		base, g_rlmPowerLimitConfigTable[base].Legacy.table_num); \
}

#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
#define PWR_LIMIT_COUNTRY_CFG_TBL_SET_EHT(base, pData, num) \
{ \
	g_rlmPowerLimitConfigTable[base].EHT.table = \
		(struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_EHT *)pData; \
	g_rlmPowerLimitConfigTable[base].EHT.table_num = num; \
	DBGLOG(RLM, TRACE, \
		"PWR_LIMIT_COUNTRY_CFG_TBL_SET_EHT [base:%d]NUM:%d\n", \
		base, g_rlmPowerLimitConfigTable[base].Legacy.table_num); \
}
#endif

#define PWR_LIMIT_FOR_EACH_RF_BAND(idx) \
	for (idx = 0; idx < PWR_LIMIT_RF_BAND_NUM; idx++)

#define PWR_LIMIT_FOR_EACH_PROTOCOL(idx) \
	for (idx = 0; idx < PWR_LIMIT_PROTOCOL_NUM; idx++)

#if (CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1)
#define PWR_LIMIT_FOR_EACH_MULTI_BAND(idx) \
	for (idx = 0; idx < PWR_LIMIT_MULTIBAND_TYPE_NUM; idx++)
#endif /* CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1 */

#define PWR_LIMIT_FOR_EACH_SUBBAND(idx, rPerPwrLimitInfo) \
	for (idx = rPerPwrLimitInfo.eStartSubBand; \
		idx <= rPerPwrLimitInfo.eEndSubBand; \
		idx++)

#define PWR_LIMIT_FOR_EACH_SUB_BAND_CHANNEL(channel, sub_band_idx) \
	for (channel = g_rRlmSubBand[sub_band_idx].ucStartCh; \
		channel <= g_rRlmSubBand[sub_band_idx].ucEndCh; \
		channel += rlmDomainGetChannelInterval(sub_band_idx, channel))

#define PWR_LIMIT_ACCUMULATE(pwr_limit, value) \
{ \
	pwr_limit = pwr_limit + value; \
	if (pwr_limit > MAX_TX_POWER) \
		pwr_limit = MAX_TX_POWER; \
}

#if (CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1)
#define PWR_LMT_MULTIBAND_2G_INFO_REGISTER(ver) \
{ \
	PWR_LMT_MULTIBAND_INFO_REGISTER( \
		ver, \
		PWR_LMT_SUBBAND_2G4, \
		PWR_LMT_SUBBAND_2G4, \
		PWR_LIMIT_RF_BAND_2G4); \
}

#define PWR_LMT_MULTIBAND_5G_INFO_REGISTER(ver) \
{ \
	PWR_LMT_MULTIBAND_INFO_REGISTER( \
		ver, \
		PWR_LMT_SUBBAND_UNII1, \
		PWR_LMT_SUBBAND_UNII3, \
		PWR_LIMIT_RF_BAND_5G); \
}

#define PWR_LMT_MULTIBAND_6G_INFO_REGISTER(ver) \
{ \
	PWR_LMT_MULTIBAND_INFO_REGISTER( \
		ver, \
		PWR_LMT_SUBBAND_UNII5A, \
		PWR_LMT_SUBBAND_UNII8, \
		PWR_LIMIT_RF_BAND_6G); \
}

#define PWR_LMT_MULTIBAND_INFO_REGISTER(ver, start_band, end_band, \
	rf_idx) \
{ \
	g_RlmMulitPwrLimitInfo[rf_idx].ucVersion = ver; \
	g_RlmMulitPwrLimitInfo[rf_idx].eStartSubBand = start_band; \
	g_RlmMulitPwrLimitInfo[rf_idx].eEndSubBand = end_band; \
	DBGLOG(RLM, TRACE, \
		"[MulBnPwr] Patch MulitBand R[%d]V[%d]Start[%d]End[%d]\n", \
		rf_idx, \
		g_RlmMulitPwrLimitInfo[rf_idx].ucVersion, \
		g_RlmMulitPwrLimitInfo[rf_idx].eStartSubBand, \
		g_RlmMulitPwrLimitInfo[rf_idx].eEndSubBand); \
}
#endif /* CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1 */

#define PWR_LMT_2G_INFO_REGISTER(limitType, ver, protocol_idx) \
{ \
	PWR_LMT_INFO_REGISTER( \
		limitType, \
		ver, \
		PWR_LMT_SUBBAND_2G4, \
		PWR_LMT_SUBBAND_2G4, \
		PWR_LIMIT_RF_BAND_2G4, \
		protocol_idx); \
}

#define PWR_LMT_5G_INFO_REGISTER(limitType, ver, protocol_idx) \
{ \
	PWR_LMT_INFO_REGISTER( \
		limitType, \
		ver, \
		PWR_LMT_SUBBAND_UNII1, \
		PWR_LMT_SUBBAND_UNII3, \
		PWR_LIMIT_RF_BAND_5G, \
		protocol_idx); \
}

#define PWR_LMT_6G_INFO_REGISTER(limitType, ver, protocol_idx) \
{ \
	PWR_LMT_INFO_REGISTER( \
		limitType, \
		ver, \
		PWR_LMT_SUBBAND_UNII5A, \
		PWR_LMT_SUBBAND_UNII8, \
		PWR_LIMIT_RF_BAND_6G, \
		protocol_idx); \
}

#define PWR_LMT_INFO_REGISTER(limitType, ver, start_band, end_band, \
	rf_idx, protocol_idx) \
{ \
	g_RlmPwrLimitInfo[rf_idx][protocol_idx].eLimitType = limitType; \
	g_RlmPwrLimitInfo[rf_idx][protocol_idx].ucVersion = ver; \
	g_RlmPwrLimitInfo[rf_idx][protocol_idx].eStartSubBand = start_band; \
	g_RlmPwrLimitInfo[rf_idx][protocol_idx].eEndSubBand = end_band; \
	DBGLOG(RLM, TRACE, \
		"Patch R[%d]P[%d]T[%d]V[%d]Start[%d]End[%d]\n", \
		rf_idx, \
		protocol_idx, \
		g_RlmPwrLimitInfo[rf_idx][protocol_idx].eLimitType, \
		g_RlmPwrLimitInfo[rf_idx][protocol_idx].ucVersion, \
		g_RlmPwrLimitInfo[rf_idx][protocol_idx].eStartSubBand, \
		g_RlmPwrLimitInfo[rf_idx][protocol_idx].eEndSubBand); \
}

/* backward compatible for txPwrCtrlStringToStruct parser use. */
#define PWR_LIMIT_PARSER_LEGACY_NUM (PWR_LIMIT_LEGACY_NUM - 3)
#define PWR_LIMIT_PARSER_LEGACY_NUM_V2 (PWR_LIMIT_LEGACY_NUM)
#define PWR_LIMIT_PARSER_HE_NUM (PWR_LIMIT_HE_NUM - 3)
#define PWR_LIMIT_PARSER_HE160_NUM (PWR_LIMIT_HE_NUM)
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
#define PWR_LIMIT_PARSER_EHT_NUM (PWR_LIMIT_EHT_NUM - 12)
#define PWR_LIMIT_PARSER_EHT6G_NUM (PWR_LIMIT_EHT_NUM)
#endif

/* backward compatible for struct TX_PWR_CTRL_CHANNEL_SETTING use*/
#define PWR_LIMIT_DYN_LEGACY_NUM (PWR_LIMIT_LEGACY_NUM)
#define PWR_LIMIT_DYN_LEGACY_6G_NUM (PWR_LIMIT_LEGACY_NUM)
#define PWR_LIMIT_DYN_HE_NUM (PWR_LIMIT_HE_NUM)
#define PWR_LIMIT_DYN_HE_6G_NUM (PWR_LIMIT_HE_NUM)
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
#define PWR_LIMIT_DYN_EHT_NUM (PWR_LIMIT_EHT_NUM)
#define PWR_LIMIT_DYN_EHT_6G_NUM (PWR_LIMIT_EHT_NUM)
#endif
/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
void rlmDomainPatchPwrLimitType(
	void);

void rlmDomainSendAntPowerSetting(
	struct ADAPTER *prAdapter);

void rlmDomainSetPwrLimitCountryCode(
	struct ADAPTER *prAdapter,
	uint8_t *pu1PwrLmtCountryCode);

uint8_t *rlmDomainPwrLmtGetMatchCountryCode(
	struct ADAPTER *prAdapter,
	uint16_t u2CountryCode);

void rlmDomainSetPwrLimitHeader(
	struct ADAPTER *prAdapter);

void rlmDomainSetPwrLimitPayload(
	struct ADAPTER *prAdapter);

void rlmDomainDumpAllPwrLmtData(
	char *message,
	struct ADAPTER *prAdapter);

void rlmDomainApplyDynPwrSetting(
	struct ADAPTER *prAdapter);

void rlmDomainWritePwrLimitToEmi(
	struct ADAPTER *prAdapter);

int32_t rlmDomainReadPwrLimitEmiData(
	struct ADAPTER *prAdapter,
	char *pcCommand,
	int32_t i4TotalLen,
	uint32_t u4RFBand,
	uint32_t u4Channel);

void rlmDomainPowerLimitEmiEvent(
	struct ADAPTER *prAdapter,
	uint8_t *pucEventBuf);

void rlmDoaminSetPwrLmtNewDataFlag(
	struct ADAPTER *prAdapter,
	bool en);

bool rlmDoaminGetPwrLmtNewDataFlag(
	struct ADAPTER *prAdapter);

void rlmDomainConnectionNotifiey(
	struct ADAPTER *prAdapter,
	enum ENUM_CONNECTION_NOTIFIED_REASON reason);

bool rlmDomainIsFWInReadEmiDataProcess(
	struct ADAPTER *prAdapter);

bool rlmDomainPwrLmtEmiStatusCtrl(
	struct ADAPTER *prAdapter,
	enum ENUM_TX_PWR_EMI_STATUS_ACTION action);

void rlmDomainSendCachePwrLmtData(
	struct ADAPTER *prAdapter);

void rlmDomainPwrLmtConnectionCMD(
	struct ADAPTER *prAdapter,
	enum ENUM_TX_PWR_EMI_SCENARIO_TYPE type);

void txPwrConnectionAddLUTElement(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	uint8_t ucTokenID);

void txPwrConnectionDelLUTElement(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	uint8_t ucTokenID);

struct TXP_REQUEST_CHANNEL_ELEMENT *txPwrConnectionFindLUTElement(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	uint8_t ucTokenID);

void txPwrConnectionShowLUTElement(
	struct ADAPTER *prAdapter);

uint8_t txPwrConnectionLUTCtrl(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	uint8_t ucTokenID,
	uint8_t fgIsReg);

void txPwrConnectionDelAllLUTElement(
	struct ADAPTER *prAdapter
);

#endif /*CFG_SUPPORT_PWR_LMT_EMI == 1 && CFG_SUPPORT_PWR_LIMIT_COUNTRY == 1*/
#endif /*_RLM_TX_PWR_LIMIT_EMI_H*/
