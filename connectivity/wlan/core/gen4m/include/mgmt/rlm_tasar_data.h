/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "rlm_tasar_data.h"
 *    \brief
 */


#ifndef _RLM_TASAR_DATA_H
#define _RLM_TASAR_DATA_H

#if (CFG_SUPPORT_TAS_HOST_CONTROL == 1)
/******************************************************************************
 *                         C O M P I L E R   F L A G S
 ******************************************************************************
 */

/******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 ******************************************************************************
 */

/******************************************************************************
 *                              C O N S T A N T S
 ******************************************************************************
 */
#define TASAR_ECI_NUM 40
#define TASAR_WIFI_CHANNEL_GRP_NUM 10
#define TASAR_BT_CHANNEL_GRP_NUM 3
#define TASAR_CONFIG_MAIN_VERSION 0
/******************************************************************************
 *                             D A T A   T Y P E S
 ******************************************************************************
 */
struct tasar_algorithm_common {
	unsigned char ucConnStatusResendInterval[4];
	unsigned char ucConnStatusResendtimes[4];
	unsigned char ucScfUartErrMargin[2];
	unsigned char ucTimeWindow0Sec;
	unsigned char ucTimeWindow2Sec;
	unsigned char ucTimeWindow4Sec;
	unsigned char ucTimeWindow30Sec;
	unsigned char ucTimeWindow60Sec;
	unsigned char ucTimeWindow100Sec;
	unsigned char ucTimeWindow360Sec;
	unsigned char ucReserve[3]; /* 4 bytes alignment*/
}; /* 20 bytes */

struct tasar_algorithm_reg_specific {
	unsigned char ucScfChipsLatMargin[2];
	unsigned char ucScfDeLimitMargin[2];
	unsigned char ucScfMdDeLimit[2];
	unsigned char ucScConnDeLimit[2];
	unsigned char ucMdMaxRefPeriod[4];
	unsigned char ucMdMaxPower[2];
	unsigned char ucJointMode;
	unsigned char ucScfConnOffMargin[2];
	unsigned char ucScfMdOffMargin[2];
	unsigned char ucScfChgInstantEn;
	unsigned char ucIsFboEn;
	unsigned char ucBtTaVer;
	unsigned char ucWifiTaVer;
	unsigned char ucScfSub6ExpireThresh[2];
	unsigned char ucSmoothChgSpeedScale;
	unsigned char ucRegulatory;
	unsigned char ucMaxScfWfFlight;
	unsigned char ucMaxScfBTFlight;
	unsigned char ucMaxScfConnFlight;
	unsigned char ucReserve[2]; /* 4 bytes alignment*/
}; /* 32 bytes */

struct tasar_wifi_channel_group {
	unsigned char ucBand;
	unsigned char ucLowChannel;
	unsigned char ucUppChannel;
	unsigned char ucSisoMimoDelta;
	unsigned char ucReserve[4];
}; /* 8 bytes */

struct tasar_scf_factor {
	unsigned char ucSub6Cust;
	unsigned char ucMmw6Cust;
	unsigned char ucWifiCust;
	unsigned char ucBTCust;
	unsigned char ucReserve[4]; /* 4 bytes alignment*/
}; /* 8 bytes */

struct tasar_wifi_plimit {
	unsigned char ucWF0Ant0;
	unsigned char ucWF0Ant1;
	unsigned char ucWF1Ant0;
	unsigned char ucWF1Ant1;
	unsigned char ucWF2Ant0;
	unsigned char ucWF2Ant1;
	unsigned char ucReserve[6]; /* 4 bytes alignment*/
}; /* 12 bytes */

struct tasar_bt_plimit {
	unsigned char ucAnt0;
	unsigned char ucAnt1;
	unsigned char ucAnt2;
	unsigned char ucReserve; /* 4 bytes alignment*/
}; /* 4 bytes */

struct tasar_plimit {
	struct tasar_scf_factor scf_factor;
	struct tasar_wifi_plimit wifi_plimit[TASAR_WIFI_CHANNEL_GRP_NUM];
	struct tasar_bt_plimit bt_plimit[TASAR_BT_CHANNEL_GRP_NUM];
}; /* 8 + 12 * 10 + 4 * 3 =  140 bytes */

struct tasar_config {
	unsigned char version[4]; /* 4 byte*/
	struct tasar_algorithm_common common; /* 20 bytes */
	struct tasar_algorithm_reg_specific reg_specific; /* 32 bytes */
	struct tasar_wifi_channel_group chngrp[TASAR_WIFI_CHANNEL_GRP_NUM];
		/* 8 * 10 = 80 bytes*/
	struct tasar_plimit plimit[TASAR_ECI_NUM]; /* 140 * 40 = 5600 bytes */
}; /* 4 + 20 + 32 + 80 + 5600 = 5736 bytes*/

/******************************************************************************
 *                            P U B L I C   D A T A
 ******************************************************************************
 */

/******************************************************************************
 *                           P R I V A T E   D A T A
 ******************************************************************************s
 */

/******************************************************************************
 *                                 M A C R O S
 ******************************************************************************
 */

/******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 ******************************************************************************
 */

#endif /* CFG_SUPPORT_TAS_HOST_CONTROL == 1 */

#endif /* _RLM_TASAR_DATA_H */
