// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#if (CFG_SUPPORT_POWER_THROTTLING == 1)

/*******************************************************************************
 *            C O M P I L E R	 F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *            E X T E R N A L	R E F E R E N C E S
 *******************************************************************************
 */
#include "precomp.h"
#include "thrm.h"

/*******************************************************************************
 *            C O N S T A N T S
 *******************************************************************************
 */

/*******************************************************************************
*			      D A T A	T Y P E S
********************************************************************************
*/
enum ENUM_THERMAL_PROTECT_LEVEL {
	THERMAL_PROTECT_LEVEL_0 = 0,
	THERMAL_PROTECT_LEVEL_1,
	THERMAL_PROTECT_LEVEL_2,
	THERMAL_PROTECT_LEVEL_3
};

/*******************************************************************************
 *            F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

#define THRM_PROT_CHECKTIME_PASSIVE	30
#define THRM_PROT_CHECKTIME_AGGRESIVE	10

#define THRM_PROT_DUTY_FORCE_50		50
#define THRM_PROT_DUTY_NORMAL		100
#define THRM_PROT_DUTY_MIN		20
#define THRM_PROT_DUTY_LEVEL_OFFSET	5


/*******************************************************************************
 *            P R I V A T E  F U N C T I O N S
 *******************************************************************************
 */

uint32_t thrmProtEnable(struct ADAPTER *prAdapter, uint8_t ucBand,
			uint8_t ucProtType, uint8_t ucTrigType,
			int32_t i4TrigTemp, int32_t i4RestoreTemp,
			uint32_t u4CheckTime)
{
	struct EXT_CMD_THERMAL_PROTECT_ENABLE *ext_cmd_buf;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	ext_cmd_buf =  cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
		sizeof(struct EXT_CMD_THERMAL_PROTECT_ENABLE));

	if (!ext_cmd_buf)
		return WLAN_STATUS_FAILURE;

	ext_cmd_buf->band_idx = ucBand;
	ext_cmd_buf->protection_type = ucProtType;
	ext_cmd_buf->trigger_type = ucTrigType;
	ext_cmd_buf->trigger_temp = i4TrigTemp;
	ext_cmd_buf->restore_temp = i4RestoreTemp;
	ext_cmd_buf->recheck_time = u4CheckTime;
	ext_cmd_buf->sub_cmd_id = THERMAL_PROTECT_ENABLE;

	rStatus = wlanSendSetQueryExtCmd(prAdapter,
				CMD_ID_LAYER_0_EXT_MAGIC_NUM,
				EXT_CMD_ID_THERMAL_PROTECT,
				TRUE,
				FALSE,
				FALSE,
				NULL,
				NULL,
				sizeof(struct EXT_CMD_THERMAL_PROTECT_ENABLE),
				(uint8_t *) ext_cmd_buf, NULL,
				0);

	cnmMemFree(prAdapter, ext_cmd_buf);

	return rStatus;
}

uint32_t thrmProtDisable(struct ADAPTER *prAdapter, uint8_t ucBand,
		uint8_t ucProtType, uint8_t ucTrigType)
{
	struct EXT_CMD_THERMAL_PROTECT_DISABLE *ext_cmd_buf;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	ext_cmd_buf = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
		sizeof(struct EXT_CMD_THERMAL_PROTECT_DISABLE));

	if (!ext_cmd_buf)
		return WLAN_STATUS_FAILURE;

	ext_cmd_buf->band_idx = ucBand;
	ext_cmd_buf->protection_type = ucProtType;
	ext_cmd_buf->trigger_type = ucTrigType;
	ext_cmd_buf->sub_cmd_id = THERMAL_PROTECT_DISABLE;

	rStatus = wlanSendSetQueryExtCmd(prAdapter,
				CMD_ID_LAYER_0_EXT_MAGIC_NUM,
				EXT_CMD_ID_THERMAL_PROTECT,
				TRUE,
				FALSE,
				FALSE,
				NULL,
				NULL,
				sizeof(struct EXT_CMD_THERMAL_PROTECT_DISABLE),
				(uint8_t *) ext_cmd_buf, NULL,
				0);
	cnmMemFree(prAdapter, ext_cmd_buf);

	return rStatus;
}

uint32_t thrmProtDutyCfg(struct ADAPTER *prAdapter, uint8_t ucBand,
	uint8_t ucLevel, uint8_t ucDuty)
{
	struct EXT_CMD_THERMAL_PROTECT_DUTY_CFG *ext_cmd_buf;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	ext_cmd_buf = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
		sizeof(struct EXT_CMD_THERMAL_PROTECT_DUTY_CFG));

	if (!ext_cmd_buf)
		return WLAN_STATUS_FAILURE;

	ext_cmd_buf->band_idx = ucBand;
	ext_cmd_buf->level_idx = ucLevel;
	ext_cmd_buf->duty = ucDuty;
	ext_cmd_buf->sub_cmd_id = THERMAL_PROTECT_DUTY_CONFIG;

	rStatus = wlanSendSetQueryExtCmd(prAdapter,
				CMD_ID_LAYER_0_EXT_MAGIC_NUM,
				EXT_CMD_ID_THERMAL_PROTECT,
				TRUE,
				FALSE,
				FALSE,
				NULL,
				NULL,
				sizeof(struct EXT_CMD_THERMAL_PROTECT_DUTY_CFG),
				(uint8_t *) ext_cmd_buf, NULL,
				0);

	cnmMemFree(prAdapter, ext_cmd_buf);

	return rStatus;
}

uint32_t thrmProtStateAct(struct ADAPTER *prAdapter, uint8_t ucBand,
	uint8_t ucProtType, uint8_t ucTrigType, uint8_t ucState)
{
	struct EXT_CMD_THERMAL_PROTECT_STATE_ACT *ext_cmd_buf;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	ext_cmd_buf = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
		sizeof(struct EXT_CMD_THERMAL_PROTECT_STATE_ACT));

	if (!ext_cmd_buf)
		return WLAN_STATUS_FAILURE;

	ext_cmd_buf->band_idx = ucBand;
	ext_cmd_buf->protect_type = ucProtType;
	ext_cmd_buf->trig_type = ucTrigType;
	ext_cmd_buf->state = ucState;
	ext_cmd_buf->sub_cmd_id = THERMAL_PROTECT_STATE_ACT;

	rStatus = wlanSendSetQueryExtCmd(prAdapter,
			CMD_ID_LAYER_0_EXT_MAGIC_NUM,
			EXT_CMD_ID_THERMAL_PROTECT,
			TRUE,
			FALSE,
			FALSE,
			NULL,
			NULL,
			sizeof(struct EXT_CMD_THERMAL_PROTECT_STATE_ACT),
			(uint8_t *) ext_cmd_buf, NULL,
			0);
	cnmMemFree(prAdapter, ext_cmd_buf);

	return rStatus;
}

void thrmProtEventHandler(struct ADAPTER *prAdapter, uint8_t *prBuf)
{
	uint8_t ucEventId = (uint8_t) *(prBuf);

	DBGLOG(NIC, TRACE, "Thermal protect event: %d\n", ucEventId);

	switch (ucEventId) {
	case THERMAL_PROTECT_EVENT_DUTY_NOTIFY:
	{
		struct EXT_EVENT_THERMAL_PROTECT_DUTY_NOTIFY *prEvent;

		prEvent =
			(struct EXT_EVENT_THERMAL_PROTECT_DUTY_NOTIFY *) prBuf;
		DBGLOG(NIC, DEBUG,
			"Duty Event, Band[%d] Level[%d], Duty[%d], Temp[%d]\n",
			prEvent->u1BandIdx, prEvent->u1LevelIdx,
			prEvent->u1DutyPercent, prEvent->i4Temp);

	}

	break;

	case THERMAL_PROTECT_EVENT_RADIO_NOTIFY:
	{
		struct EXT_EVENT_THERMAL_PROTECT_RADIO_NOTIFY *prEvent;

		prEvent =
			(struct EXT_EVENT_THERMAL_PROTECT_RADIO_NOTIFY *) prBuf;

		DBGLOG(NIC, TRACE,
			"Radio event, Band[%d] Level[%d] Temp[%d] Action[%d]\n",
			prEvent->u1BandIdx, prEvent->u1LevelIdx,
			prEvent->i4Temp, prEvent->eProtectActType);
	}
	break;
	case THERMAL_PROTECT_EVENT_MECH_INFO:
	{
		struct EXT_EVENT_THERMAL_PROTECT_MECH_INFO *prEvent;
		uint8_t i = 0;

		prEvent =
			(struct EXT_EVENT_THERMAL_PROTECT_MECH_INFO *) prBuf;
		for (i = THERMAL_PROTECT_TYPE_NTX_CTRL;
			i < THERMAL_PROTECT_TYPE_NUM; i++) {

			DBGLOG(NIC, DEBUG,
				"MechInfo[%d/%d/%d/%d/%d/%d/%d]",
				prEvent->u1ProtectionType[i],
				prEvent->u1TriggerType[i],
				prEvent->i4TriggerTemp[i],
				prEvent->i4RestoreTemp[i],
				prEvent->u2RecheckTime[i],
				prEvent->u1State[i],
				prEvent->fgEnable[i]);
		}
	}
		break;
	case THERMAL_PROTECT_EVENT_DUTY_INFO:
	{
		struct EXT_EVENT_THERMAL_PROTECT_DUTY_INFO *prEvent;

		prEvent =
			(struct EXT_EVENT_THERMAL_PROTECT_DUTY_INFO *) prBuf;

		DBGLOG(NIC, DEBUG,
			"Duty Info, Band[%d] Duty[%d/%d/%d/%d]\n",
			prEvent->u1BandIdx, prEvent->u1Duty0, prEvent->u1Duty1,
			prEvent->u1Duty2, prEvent->u1Duty3);
	}
		break;
	default:
		break;
	}
}

void thrmProtUpdateDutyCfg(struct ADAPTER *prAdapter, uint8_t ucDuty)
{
	uint8_t i = 0, j = 0;

	for (i = ENUM_BAND_0; i < ENUM_BAND_NUM; i++) {

		thrmProtDisable(prAdapter, i,
				THERMAL_PROTECT_TYPE_DUTY_CTRL,
				THERMAL_PROTECT_HIGH_TRIG);

		/* Init duty control */
		for (j = 0; j <= THERMAL_PROTECT_LEVEL_3; j++) {
			thrmProtDutyCfg(prAdapter, i, j,
				ucDuty -
				(j * THRM_PROT_DUTY_LEVEL_OFFSET));
		}

		thrmProtEnable(prAdapter, i,
			THERMAL_PROTECT_TYPE_DUTY_CTRL,
			THERMAL_PROTECT_HIGH_TRIG,
			prAdapter->rWifiVar.i4ThrmCtrlTemp,
			(prAdapter->rWifiVar.i4ThrmCtrlTemp -
				THRM_PROT_RESTORE_TEMP_OFFSET),
			THRM_PROT_CHECKTIME_PASSIVE);
	}
	prAdapter->rThrmProtCfg.ucCurrDutyCfg = ucDuty;
}

int thrmProtLvHandler(struct ADAPTER *prAdapter, uint8_t ucLevel)
{
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	uint8_t ucOrigLevel;
	uint8_t i = 0;

	ucOrigLevel = prAdapter->rThrmProtCfg.ucLevel;

	DBGLOG(NIC, TRACE, "Lv[%d/%d] Duty[%d/%d]\n", ucLevel,
		    ucOrigLevel, prAdapter->rThrmProtCfg.ucCurrDutyCfg,
			prAdapter->rWifiVar.aucThrmLvTxDuty[ucLevel]);

	prAdapter->rThrmProtCfg.ucLevel = ucLevel;

	if (ucOrigLevel == CONN_PWR_THR_LV_MAX)
		return 0;

	if (prAdapter->rThrmProtCfg.ucCurrDutyCfg == 0) {
		DBGLOG(NIC, DEBUG, "Enable radio off control.\n");
		for (i = ENUM_BAND_0; i < ENUM_BAND_NUM; i++) {
			/* Init radio off control */
			thrmProtEnable(prAdapter, i,
				THERMAL_PROTECT_TYPE_RADIO_CTRL,
				THERMAL_PROTECT_HIGH_TRIG,
				prWifiVar->i4ThrmRadioOffTemp,
				(prWifiVar->i4ThrmRadioOffTemp -
				THRM_PROT_RESTORE_TEMP_OFFSET),
				THRM_PROT_CHECKTIME_PASSIVE);
		}
	}


	if (prAdapter->rWifiVar.aucThrmLvTxDuty[ucLevel] !=
		prAdapter->rThrmProtCfg.ucCurrDutyCfg) {

		if (prAdapter->rWifiVar.aucThrmLvTxDuty[ucLevel] >
			THRM_PROT_DUTY_MIN)
			DBGLOG(NIC, WARN, "Invalid level duty[%d]\n",
				prAdapter->rWifiVar.aucThrmLvTxDuty[ucLevel]);

		thrmProtUpdateDutyCfg(prAdapter,
			prAdapter->rWifiVar.aucThrmLvTxDuty[ucLevel]);
	}

	return 0;
}

void thrmInit(struct ADAPTER *prAdapter)
{
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;

	DBGLOG(NIC, TRACE, "Radio off temp[%d] Duty control temp[%d]\n",
		prWifiVar->i4ThrmRadioOffTemp, prWifiVar->i4ThrmCtrlTemp);

	prAdapter->rThrmProtCfg.ucLevel = CONN_PWR_THR_LV_MAX;

	/* Register power level handler */
	kalPwrLevelHdlrRegister(prAdapter, thrmProtLvHandler);

	DBGLOG(NIC, TRACE, "Duty[%d]\n",
			prAdapter->rThrmProtCfg.ucCurrDutyCfg);
}

#endif
/* End of thrm.c */
