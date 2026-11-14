// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "rlm_tasar.c"
 *    \brief
 *
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
#include "rlm_tasar.h"
#include "rlm_tasar_data.h"

#if (CFG_SUPPORT_TAS_HOST_CONTROL == 1)
/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/*------------------------------------------------------------------------------
 * It will auto enable g_rTasarCountryTbl[0].tbl[0]/eci:0 setting
 *------------------------------------------------------------------------------
 */
#define CFG_SUPPORT_TASAR_DEFAULT_ON		1

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

struct tasar_country g_rTasarCountryTbl_reg0[] = {
	{{'U', 'S'}},
};
struct tasar_country g_rTasarCountryTbl_reg1[] = {
	{{'C', 'A'}}
};
struct tasar_country g_rTasarCountryTbl_reg2[] = {
	{{'A', 'E'}}
};
struct tasar_country g_rTasarCountryTbl_reg3[] = {
	{{'A', 'G'}}
};
struct tasar_country g_rTasarCountryTbl_reg4[] = {
	{{'A', 'I'}}
};
struct tasar_country g_rTasarCountryTbl_reg5[] = {
	{{'A', 'L'}}
};
struct tasar_country g_rTasarCountryTbl_reg6[] = {
	{{'A', 'M'}}
};
struct tasar_country g_rTasarCountryTbl_reg7[] = {
	{{'A', 'T'}}
};
struct tasar_country g_rTasarCountryTbl_reg8[] = {
	{{'B', 'B'}}
};
struct tasar_country g_rTasarCountryTbl_reg9[] = {
	{{'B', 'E'}}
};
struct tasar_country g_rTasarCountryTbl_reg10[] = {
	{{'A', 'F'}}
};
struct tasar_country g_rTasarCountryTbl_reg11[] = {
	{{'A', 'L'}}
};
struct tasar_country_tbl_all g_rTasarCountryTbl[] = {
	REG_TBL_REG(g_rTasarCountryTbl_reg0, "REG0_ConnsysTasar.cfg"),
	REG_TBL_REG(g_rTasarCountryTbl_reg1, "REG1_ConnsysTasar.cfg"),
	REG_TBL_REG(g_rTasarCountryTbl_reg2, "REG2_ConnsysTasar.cfg"),
	REG_TBL_REG(g_rTasarCountryTbl_reg3, "REG3_ConnsysTasar.cfg"),
	REG_TBL_REG(g_rTasarCountryTbl_reg4, "REG4_ConnsysTasar.cfg"),
	REG_TBL_REG(g_rTasarCountryTbl_reg5, "REG5_ConnsysTasar.cfg"),
	REG_TBL_REG(g_rTasarCountryTbl_reg6, "REG6_ConnsysTasar.cfg"),
	REG_TBL_REG(g_rTasarCountryTbl_reg7, "REG7_ConnsysTasar.cfg"),
	REG_TBL_REG(g_rTasarCountryTbl_reg8, "REG8_ConnsysTasar.cfg"),
	REG_TBL_REG(g_rTasarCountryTbl_reg9, "REG9_ConnsysTasar.cfg"),
	REG_TBL_REG(g_rTasarCountryTbl_reg10, "REG10_ConnsysTasar.cfg"),
	REG_TBL_REG(g_rTasarCountryTbl_reg11, "REG11_ConnsysTasar.cfg")
};

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */
#define TASAR_COUNTRY_TBL_SIZE \
	(sizeof(g_rTasarCountryTbl)/sizeof(struct tasar_country_tbl_all))
/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

static void tasarDumpCurrentConfig(struct tasar_config *rCfg)
{
	uint32_t u4Grp = 0, u4Eci = 0;

	if (!rCfg)
		return;

	DBGLOG(RLM, INFO, "[TAS]tasar_config adr = %x, size:%d\n",
		rCfg, sizeof(struct tasar_config));
	DBGLOG(RLM, INFO, "[TAS]version adr = %x, size:%d\n",
		&rCfg->version[0], sizeof(rCfg->version));
	DBGLOG(RLM, INFO, "[TAS]common adr = %x, size:%d\n",
		&rCfg->common, sizeof(rCfg->common));
	DBGLOG(RLM, INFO, "[TAS]reg_specific adr = %x, size:%d\n",
		&rCfg->reg_specific, sizeof(rCfg->reg_specific));
	DBGLOG(RLM, INFO, "[TAS]chngrp[0] adr = %x, size:%d\n",
		&rCfg->chngrp[0], sizeof(rCfg->chngrp[0]));
	DBGLOG(RLM, INFO, "[TAS]plimit[0] adr = %x, size:%d\n",
		&rCfg->plimit[0], sizeof(rCfg->plimit[0]));

	DBGLOG(RLM, INFO, "[TAS]version [%d]\n", TAS_VERSION(rCfg));

	DBGLOG(RLM, INFO, "[TAS]common [%d/%d/%d/%d/%d/%d/%d/%d/%d/%d]\n",
		CONN_STATUS_RESEND_INTERVAL(rCfg),
		CONN_STATUS_RESEND_TIMES(rCfg),
		SCF_UART_ERR_MARGIN(rCfg),
		TIME_WD_0_SEC(rCfg),
		TIME_WD_2_SEC(rCfg),
		TIME_WD_4_SEC(rCfg),
		TIME_WD_30_SEC(rCfg),
		TIME_WD_60_SEC(rCfg),
		TIME_WD_100_SEC(rCfg),
		TIME_WD_360_SEC(rCfg));

	DBGLOG(RLM, INFO,
		"[TAS]Reg[%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d]\n",
		SCF_CHIPS_LAT_MARGIN(rCfg),
		SCF_DELIMIT_MARGIN(rCfg),
		SCF_MD_DELIMIT(rCfg),
		SCF_CONN_DELIMIT(rCfg),
		MD_MAX_REF_PERIOD(rCfg),
		MD_MAX_POWER(rCfg),
		JOINT_MODE(rCfg),
		SCF_CONN_OFF_MARGIN(rCfg),
		SCF_MD_OFF_MARGIN(rCfg),
		SCF_CHG_INSTANT_EN(rCfg),
		IS_FBO_EN(rCfg),
		BT_TA_VER(rCfg),
		WIFI_TA_VER(rCfg),
		SCF_SUB6_EXP_TH(rCfg),
		SMOOTH_CHG_SPEED_SCALE(rCfg),
		REGULATORY(rCfg));

	DBGLOG(RLM, INFO,
		"[TAS]Reg[%d/%d/%d]\n",
		MAX_SCF_WF_FLIGHT(rCfg),
		MAX_SCF_BT_FLIGHT(rCfg),
		MAX_SCF_CONN_FLIGHT(rCfg));

	for (u4Grp = 0; u4Grp < TASAR_WIFI_CHANNEL_GRP_NUM; u4Grp++)
		DBGLOG(RLM, TRACE, "[TAS]chngrp [grp:%d][%d/%d/%d]\n", u4Grp,
			WIFI_CHANNEL_GRP_BAND(rCfg, u4Grp),
			WIFI_CHANNEL_GRP_LOW_CH(rCfg, u4Grp),
			WIFI_CHANNEL_GRP_UP_CH(rCfg, u4Grp),
			WIFI_CHANNEL_GRP_SISO_MIMO_DELTA(rCfg, u4Grp));

	for (u4Eci = 0; u4Eci < TASAR_ECI_NUM; u4Eci++) {
		DBGLOG(RLM, TRACE, "[TAS]plimit scf [eci:%d][%d/%d/%d/%d]\n",
			u4Eci,
			WIFI_PLIMIT_SCF_SUB6(rCfg, u4Eci),
			WIFI_PLIMIT_SCF_MMW6(rCfg, u4Eci),
			WIFI_PLIMIT_SCF_WF(rCfg, u4Eci),
			WIFI_PLIMIT_SCF_BT(rCfg, u4Eci));

		for (u4Grp = 0; u4Grp < TASAR_WIFI_CHANNEL_GRP_NUM; u4Grp++)
			DBGLOG(RLM, TRACE,
				"[TAS]plmt[G:%d/E:%d]-%d/%d/%d/%d/%d/%d\n",
				u4Grp, u4Eci,
				WIFI_PLIMIT_WF0_ANT0(rCfg, u4Grp, u4Eci),
				WIFI_PLIMIT_WF0_ANT1(rCfg, u4Grp, u4Eci),
				WIFI_PLIMIT_WF1_ANT0(rCfg, u4Grp, u4Eci),
				WIFI_PLIMIT_WF1_ANT1(rCfg, u4Grp, u4Eci),
				WIFI_PLIMIT_WF2_ANT0(rCfg, u4Grp, u4Eci),
				WIFI_PLIMIT_WF2_ANT1(rCfg, u4Grp, u4Eci));
	}
}

static bool tasarUpdateConfig(struct ADAPTER *prAdapter, int8_t *pcFileName)
{
	uint8_t *pucConfigBuf = NULL;
	uint32_t u4ConfigReadLen = 0;
	bool ret;

	if (kalRequestFirmware(pcFileName, &pucConfigBuf,
		&u4ConfigReadLen, FALSE,
		kalGetGlueDevHdl(prAdapter->prGlueInfo)) == 0) {
		/* ToDo:: Nothing */
	}

	if (!pucConfigBuf) {
		DBGLOG(RLM, TRACE, "[TAS] File invalid\n");
		ret = FALSE;
		goto free;
	}

	DBGLOG(RLM, TRACE, "[TAS] structure size/file size[%d/%d]\n",
		sizeof(prAdapter->rTasarCfg), u4ConfigReadLen);

	if (sizeof(prAdapter->rTasarCfg) != u4ConfigReadLen) {
		DBGLOG(RLM, INFO, "[TAS] invalid file size\n");
		ret = FALSE;
		goto free;
	}

	memcpy(&prAdapter->rTasarCfg, pucConfigBuf, u4ConfigReadLen);
	tasarDumpCurrentConfig(&prAdapter->rTasarCfg);
	ret = TRUE;

free:
	if (pucConfigBuf)
		kalMemFree(pucConfigBuf, VIR_MEM_TYPE, u4ConfigReadLen);
	return ret;

}

static uint32_t tasarSearchReg(struct tasar_scenrio_ctrl rScenrio)
{
	uint32_t i, j;
	uint16_t u2CountryCode = 0;
	struct tasar_country *prTasCC;

	for (i = 0; i < TASAR_COUNTRY_TBL_SIZE; i++) {
		for (j = 0; j < g_rTasarCountryTbl[i].cnt; j++) {
			prTasCC = &g_rTasarCountryTbl[i].tbl[j];
			DBGLOG(RLM, TRACE,
				"[TAS] Tbl[%d][%d]CC[%c][%c]FileName:[%s]\n",
				i,
				j,
				prTasCC->aucCountryCode[0],
				prTasCC->aucCountryCode[1],
				g_rTasarCountryTbl[i].ucFileName);

			u2CountryCode =
				(((uint16_t)prTasCC->aucCountryCode[0] << 8) |
				(uint16_t) prTasCC->aucCountryCode[1]);

			if (rScenrio.u2CountryCode == u2CountryCode)
				return i;
		}
	}

	/* return TASAR_COUNTRY_TBL_SIZE mean no match found*/
	DBGLOG(RLM, INFO, "[TAS] no match reg found in this cc %x\n",
		rScenrio.u2CountryCode);

	return TASAR_COUNTRY_TBL_SIZE;
}


static uint32_t tasarSendSetChipCmd(struct ADAPTER *prAdapter, char *pcCommand)
{
	struct PARAM_CUSTOM_CHIP_CONFIG_STRUCT rChipConfigInfo = {0};
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	uint32_t u4CmdLen = 0, u4BufLen = 0;
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;

	if (!pcCommand) {
		DBGLOG(RLM, INFO, "[TAS] cmd null return\n");
		return WLAN_STATUS_FAILURE;
	}

	u4CmdLen = kalStrnLen(pcCommand, CHIP_CONFIG_RESP_SIZE - 1);

	rChipConfigInfo.ucType = CHIP_CONFIG_TYPE_WO_RESPONSE;
	rChipConfigInfo.u2MsgSize = u4CmdLen;

	kalStrnCpy(rChipConfigInfo.aucCmd, pcCommand,
		CHIP_CONFIG_RESP_SIZE - 1);
	rChipConfigInfo.aucCmd[CHIP_CONFIG_RESP_SIZE - 1] = '\0';
	DBGLOG(RLM, INFO, "[TAS] cmd :%s\n", rChipConfigInfo.aucCmd);

	rWlanStatus = kalIoctl(prGlueInfo, wlanoidSetChipConfig,
		&rChipConfigInfo, sizeof(rChipConfigInfo),
		&u4BufLen);
	return rWlanStatus;
}

static uint32_t tasarSendPowerLimit(
	struct ADAPTER *prAdapter,
	uint32_t u4ChGrp,
	uint32_t u4Eci)
{
	char pcTasCommand[CHIP_CONFIG_RESP_SIZE];
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	uint32_t pos = 0;
	struct tasar_config *prTasarCfg;

	if (!prAdapter) {
		DBGLOG(RLM, INFO, "[TAS] prAdapter\n");
		return WLAN_STATUS_FAILURE;
	}
	if (u4Eci >= TASAR_ECI_NUM) {
		DBGLOG(RLM, INFO, "[TAS] u4Eci invalid\n");
		return WLAN_STATUS_FAILURE;
	}
	if (u4ChGrp >= TASAR_WIFI_CHANNEL_GRP_NUM) {
		DBGLOG(RLM, INFO, "[TAS] u4ChGrp invalid\n");
		return WLAN_STATUS_FAILURE;
	}

	prTasarCfg = &prAdapter->rTasarCfg;

	if ((WIFI_CHANNEL_GRP_BAND(prTasarCfg, u4ChGrp) == 0xFF) ||
		(WIFI_CHANNEL_GRP_LOW_CH(prTasarCfg, u4ChGrp) == 0xFF) ||
		(WIFI_CHANNEL_GRP_UP_CH(prTasarCfg, u4ChGrp) == 0xFF) ||
		(WIFI_CHANNEL_GRP_SISO_MIMO_DELTA(prTasarCfg, u4ChGrp)
			== 0xFF)) {
		DBGLOG(RLM, INFO, "[TAS] Channel Group %d defined invalid\n",
			u4ChGrp);
		return rWlanStatus;
	}

	pos += kalScnprintf(pcTasCommand + pos, CHIP_CONFIG_RESP_SIZE - pos,
		"coex tasar_set 20 %d %d %d %d %d %d %d %d %d %d %d\n",
		u4ChGrp,
		WIFI_CHANNEL_GRP_BAND(prTasarCfg, u4ChGrp),
		WIFI_CHANNEL_GRP_LOW_CH(prTasarCfg, u4ChGrp),
		WIFI_CHANNEL_GRP_UP_CH(prTasarCfg, u4ChGrp),
		WIFI_CHANNEL_GRP_SISO_MIMO_DELTA(prTasarCfg, u4ChGrp),
		WIFI_PLIMIT_WF0_ANT0(prTasarCfg, u4ChGrp, u4Eci),
		WIFI_PLIMIT_WF0_ANT1(prTasarCfg, u4ChGrp, u4Eci),
		WIFI_PLIMIT_WF1_ANT0(prTasarCfg, u4ChGrp, u4Eci),
		WIFI_PLIMIT_WF1_ANT1(prTasarCfg, u4ChGrp, u4Eci),
		WIFI_PLIMIT_WF2_ANT0(prTasarCfg, u4ChGrp, u4Eci),
		WIFI_PLIMIT_WF2_ANT1(prTasarCfg, u4ChGrp, u4Eci)
	);
	rWlanStatus = tasarSendSetChipCmd(prAdapter, pcTasCommand);

	return rWlanStatus;
}

static uint32_t tasarSendCommonField(
	struct ADAPTER *prAdapter)
{
	char pcTasCommand[CHIP_CONFIG_RESP_SIZE];
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	uint32_t pos = 0;
	struct tasar_config *prTasarCfg;

	if (!prAdapter) {
		DBGLOG(RLM, INFO, "[TAS] prAdapter\n");
		return WLAN_STATUS_FAILURE;
	}

	prTasarCfg = &prAdapter->rTasarCfg;

	pos += kalScnprintf(pcTasCommand + pos, CHIP_CONFIG_RESP_SIZE - pos,
		"coex tasar_set 21 0 0x%x %d %d\n",
		CONN_STATUS_RESEND_INTERVAL(prTasarCfg),
		CONN_STATUS_RESEND_TIMES(prTasarCfg),
		SCF_UART_ERR_MARGIN(prTasarCfg)
	);

	rWlanStatus = tasarSendSetChipCmd(prAdapter, pcTasCommand);

	return rWlanStatus;
}

static uint32_t tasarSendRegSpecificField(
	struct ADAPTER *prAdapter)
{
	char pcTasCommand[CHIP_CONFIG_RESP_SIZE];
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	uint32_t pos = 0;
	struct tasar_config *prTasarCfg;

	if (!prAdapter) {
		DBGLOG(RLM, INFO, "[TAS] prAdapter\n");
		return WLAN_STATUS_FAILURE;
	}

	prTasarCfg = &prAdapter->rTasarCfg;

	kalMemZero(pcTasCommand, sizeof(pcTasCommand));
	pos = 0;
	pos += kalScnprintf(pcTasCommand + pos, CHIP_CONFIG_RESP_SIZE - pos,
		"coex tasar_set 21 1 %d %d %d %d\n",
		SCF_CHIPS_LAT_MARGIN(prTasarCfg),
		SCF_DELIMIT_MARGIN(prTasarCfg),
		SCF_MD_DELIMIT(prTasarCfg),
		SCF_CONN_DELIMIT(prTasarCfg)
	);
	rWlanStatus = tasarSendSetChipCmd(prAdapter, pcTasCommand);

	kalMemZero(pcTasCommand, sizeof(pcTasCommand));
	pos = 0;
	pos += kalScnprintf(pcTasCommand + pos, CHIP_CONFIG_RESP_SIZE - pos,
		"coex tasar_set 21 2 %d %d %d %d %d\n",
		MD_MAX_REF_PERIOD(prTasarCfg),
		MD_MAX_POWER(prTasarCfg),
		JOINT_MODE(prTasarCfg),
		SCF_CONN_OFF_MARGIN(prTasarCfg),
		SCF_MD_OFF_MARGIN(prTasarCfg)
	);
	rWlanStatus = tasarSendSetChipCmd(prAdapter, pcTasCommand);

	kalMemZero(pcTasCommand, sizeof(pcTasCommand));
	pos = 0;
	pos += kalScnprintf(pcTasCommand + pos, CHIP_CONFIG_RESP_SIZE - pos,
		"coex tasar_set 21 3 %d %d %d %d %d %d %d\n",
		SCF_CHG_INSTANT_EN(prTasarCfg),
		IS_FBO_EN(prTasarCfg),
		BT_TA_VER(prTasarCfg),
		WIFI_TA_VER(prTasarCfg),
		SCF_SUB6_EXP_TH(prTasarCfg),
		SMOOTH_CHG_SPEED_SCALE(prTasarCfg),
		REGULATORY(prTasarCfg)
	);
	rWlanStatus = tasarSendSetChipCmd(prAdapter, pcTasCommand);

	kalMemZero(pcTasCommand, sizeof(pcTasCommand));
	pos = 0;
	pos += kalScnprintf(pcTasCommand + pos, CHIP_CONFIG_RESP_SIZE - pos,
		"coex tasar_set 21 4 %d %d %d\n",
		MAX_SCF_WF_FLIGHT(prTasarCfg),
		MAX_SCF_BT_FLIGHT(prTasarCfg),
		MAX_SCF_CONN_FLIGHT(prTasarCfg)
	);
	rWlanStatus = tasarSendSetChipCmd(prAdapter, pcTasCommand);

	return rWlanStatus;
}


static uint32_t tasarSendAlgoDataToFW(struct ADAPTER *prAdapter)
{
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;

	DBGLOG(RLM, INFO, "[TAS] update algorithm data\n");
	rWlanStatus = tasarSendCommonField(prAdapter);
	rWlanStatus = tasarSendRegSpecificField(prAdapter);

	return rWlanStatus;
}

static uint32_t tasarSendScf(struct ADAPTER *prAdapter, uint32_t u4Eci)
{
	char pcTasCommand[CHIP_CONFIG_RESP_SIZE];
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	uint32_t pos = 0;
	struct tasar_config *prTasarCfg;

	if (!prAdapter) {
		DBGLOG(RLM, INFO, "[TAS] prAdapter invalid\n");
		return WLAN_STATUS_FAILURE;
	}
	if (u4Eci >= TASAR_ECI_NUM) {
		DBGLOG(RLM, INFO, "[TAS] u4Eci invalid\n");
		return WLAN_STATUS_FAILURE;
	}

	prTasarCfg = &prAdapter->rTasarCfg;

	pos += kalScnprintf(pcTasCommand + pos, CHIP_CONFIG_RESP_SIZE - pos,
		"coex tasar_set 21 5 %d %d %d %d\n",
		WIFI_PLIMIT_SCF_SUB6(prTasarCfg, u4Eci),
		WIFI_PLIMIT_SCF_MMW6(prTasarCfg, u4Eci),
		WIFI_PLIMIT_SCF_WF(prTasarCfg, u4Eci),
		WIFI_PLIMIT_SCF_BT(prTasarCfg, u4Eci)
	);
	rWlanStatus = tasarSendSetChipCmd(prAdapter, pcTasCommand);

	return rWlanStatus;
}

static uint32_t tasarSendConfigEnable(struct ADAPTER *prAdapter)
{
	char pcTasCommand[CHIP_CONFIG_RESP_SIZE];
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	uint32_t pos = 0;

	if (!prAdapter) {
		DBGLOG(RLM, INFO, "[TAS] prAdapter invalid\n");
		return WLAN_STATUS_FAILURE;
	}

	pos += kalScnprintf(pcTasCommand + pos, CHIP_CONFIG_RESP_SIZE - pos,
		"coex tasar_set 21 20\n"
	);
	rWlanStatus = tasarSendSetChipCmd(prAdapter, pcTasCommand);

	return rWlanStatus;
}

static uint32_t tasarSendPwrLimitToFW(struct ADAPTER *prAdapter, uint32_t u4Eci)
{
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4ChGrp;

	DBGLOG(RLM, INFO, "[TAS] update to u4Eci :%d\n", u4Eci);

	if (u4Eci >= TASAR_ECI_NUM) {
		DBGLOG(RLM, INFO, "[TAS] u4Eci invalid\n");
		return WLAN_STATUS_FAILURE;
	}

	/* TAS Scaling Factor*/
	rWlanStatus = tasarSendScf(prAdapter, u4Eci);

	/* WF Channel Group Power Limit*/
	for (u4ChGrp = 0; u4ChGrp < TASAR_WIFI_CHANNEL_GRP_NUM; u4ChGrp++)
		rWlanStatus = tasarSendPowerLimit(prAdapter, u4ChGrp, u4Eci);

	DBGLOG(RLM, INFO, "[TAS] status :%x\n", rWlanStatus);

	return rWlanStatus;
}

uint32_t tasarDataExtract(uint8_t *ctx, uint32_t cnt)
{
	uint32_t i, j = 0;

	for (i = 0; i < cnt; i++)
		j += ctx[i] << (i * 8);
	return j;
}

void tasarInit(struct ADAPTER *prAdapter)
{
	if (!prAdapter) {
		DBGLOG(RLM, INFO, "[TAS] prAdapter null\n");
		return;
	}
	/* update default status*/
	prAdapter->rTasarScenrio.u4RegTblIndex = TASAR_COUNTRY_TBL_SIZE;
	prAdapter->rTasarScenrio.u2CountryCode = 0;
	prAdapter->rTasarScenrio.u4Eci = 0;

	DBGLOG(RLM, TRACE, "[TAS] Init reg:%d/CC:%d/Eci:%d\n",
		prAdapter->rTasarScenrio.u4RegTblIndex,
		prAdapter->rTasarScenrio.u2CountryCode,
		prAdapter->rTasarScenrio.u4Eci);

#if (CFG_SUPPORT_TASAR_DEFAULT_ON == 1)
	struct tasar_scenrio_ctrl rScenrio = {0};

	rScenrio.u2CountryCode =
		(uint16_t) g_rTasarCountryTbl[0].tbl[0].aucCountryCode[0] << 8;
	rScenrio.u2CountryCode +=
		(uint16_t) g_rTasarCountryTbl[0].tbl[0].aucCountryCode[1];
	rScenrio.u4Eci = 0;
	tasarUpdateScenrio(prAdapter, rScenrio);
#endif

}

void tasarUpdateScenrio(
	struct ADAPTER *prAdapter,
	struct tasar_scenrio_ctrl rScenrio)
{
	bool fgRegChange = FALSE;
	uint32_t u4Idx = TASAR_COUNTRY_TBL_SIZE;
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;

	if (!prAdapter) {
		DBGLOG(RLM, INFO, "[TAS] prAdapter null\n");
		return;
	}

	DBGLOG(RLM, TRACE,
		"[TAS] change reg:%d/CC:%d/Eci:%d to [CC:%x/Eci:%d]\n",
		prAdapter->rTasarScenrio.u4RegTblIndex,
		prAdapter->rTasarScenrio.u2CountryCode,
		prAdapter->rTasarScenrio.u4Eci,
		rScenrio.u2CountryCode,
		rScenrio.u4Eci);

	if (rScenrio.u4Eci >= TASAR_ECI_NUM) {
		DBGLOG(RLM, INFO, "[TAS] ECI invalid\n");
		return;
	}

	u4Idx = tasarSearchReg(rScenrio);
	if (u4Idx < TASAR_COUNTRY_TBL_SIZE) {
		if (u4Idx != prAdapter->rTasarScenrio.u4RegTblIndex) {
			if (tasarUpdateConfig(prAdapter,
				g_rTasarCountryTbl[u4Idx].ucFileName)) {
				DBGLOG(RLM, INFO,
					"[TAS] reg change to %d\n",
					u4Idx);
				fgRegChange = TRUE;
			} else {
				DBGLOG(RLM, INFO,
					"[TAS] Host Config parser fail\n");
					return;
			}
		} else {
			DBGLOG(RLM, INFO, "[TAS] reg no change\n");
		}
	} else {
		DBGLOG(RLM, INFO,
			"[TAS] no match reg found, return\n");
			return;
	}

	/* Reg Change need to send common & reg specific field data */
	if (fgRegChange) {
		rWlanStatus = tasarSendAlgoDataToFW(prAdapter);
		if (rWlanStatus != WLAN_STATUS_SUCCESS)
			return;
	}

	/* ECI change need to send different power limit */
	if (fgRegChange || prAdapter->rTasarScenrio.u4Eci != rScenrio.u4Eci) {
		rWlanStatus = tasarSendPwrLimitToFW(prAdapter, rScenrio.u4Eci);
		if (rWlanStatus != WLAN_STATUS_SUCCESS)
			return;
		rWlanStatus = tasarSendConfigEnable(prAdapter);
		if (rWlanStatus != WLAN_STATUS_SUCCESS)
			return;
	}

	/* update latest status*/
	prAdapter->rTasarScenrio.u4RegTblIndex = u4Idx;
	prAdapter->rTasarScenrio.u2CountryCode = rScenrio.u2CountryCode;
	prAdapter->rTasarScenrio.u4Eci = rScenrio.u4Eci;

	DBGLOG(RLM, TRACE, "[TAS] update reg:%d/CC:%d/Eci:%d\n",
		prAdapter->rTasarScenrio.u4RegTblIndex,
		prAdapter->rTasarScenrio.u2CountryCode,
		prAdapter->rTasarScenrio.u4Eci);
}
#endif /* CFG_SUPPORT_TAS_HOST_CONTROL == 1 */
