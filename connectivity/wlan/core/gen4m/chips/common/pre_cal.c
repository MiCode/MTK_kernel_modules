/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
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
#if (CFG_SUPPORT_CONNFEM == 1)
#include "connfem_api.h"
#endif

/*******************************************************************************
 *                              C O N S T A N T S
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
#if CFG_MTK_ANDROID_EMI
u_int8_t *gEmiCalResult;
uint32_t gEmiCalSize;
uint32_t gEmiCalOffset;
u_int8_t gEmiCalNoUseEmiData;
#endif

static u_int8_t g_fgPreCal;
static u_int8_t g_fgCalDisabled;
#if CFG_MTK_ANDROID_WMT
static u_int8_t g_fgEverCal = FALSE;
#endif

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

uint32_t wlanAccessCalibrationEMI(struct ADAPTER *prAdapter,
	struct INIT_EVENT_PHY_ACTION_RSP *pCalEvent,
	uint8_t backupEMI)
{
#define TURN_ON_EMI_BACKUP 1

	uint32_t u4Status = WLAN_STATUS_FAILURE;

#if CFG_MTK_ANDROID_EMI
	do {
		if (backupEMI == TRUE) {
			if (!pCalEvent) {
				DBGLOG(INIT, ERROR, "pCalEvent null\n");
				break;
			}

			if (pCalEvent->u4EmiLength == 0) {
				DBGLOG(INIT, ERROR, "gEmiCalSize 0\n");
				break;
			}

			if (g_fgCalDisabled) {
				DBGLOG(INIT, ERROR, "Calibration disabled.\n");
				break;
			}

			gEmiCalOffset = emi_mem_offset_convert(
				pCalEvent->u4EmiAddress);
			gEmiCalSize = pCalEvent->u4EmiLength;

			/*** backup calibration result ******/
			if (gEmiCalNoUseEmiData == TRUE) {
				DBGLOG(INIT, INFO, "No EMI backup.\n");
				u4Status = WLAN_STATUS_SUCCESS;
				break;
			}

			if (gEmiCalResult != NULL) {
				kalMemFree(gEmiCalResult,
					VIR_MEM_TYPE,
					gEmiCalSize);
				gEmiCalResult = NULL;
			}

			gEmiCalResult = kalMemAlloc(gEmiCalSize, VIR_MEM_TYPE);

			if (gEmiCalResult == NULL) {
				DBGLOG(INIT, ERROR,
					"gEmiCalResult kalMemAlloc NULL\n");
				break;
			}

			DBGLOG(INIT, INFO,
				"Offset(0x%x), Size(0x%x), NoUse(%d), backup(%d)\n",
					gEmiCalOffset, gEmiCalSize,
					gEmiCalNoUseEmiData, backupEMI);

#if (TURN_ON_EMI_BACKUP == 1)
			emi_mem_read(prAdapter->chip_info,
				gEmiCalOffset, gEmiCalResult,
				gEmiCalSize);
#endif
			u4Status = WLAN_STATUS_SUCCESS;
		} else {
			if (gEmiCalNoUseEmiData == TRUE) {
				DBGLOG(INIT, INFO, "No EMI restore.\n");
				u4Status = WLAN_STATUS_SUCCESS;
			}
			else if (gEmiCalOffset == 0 || gEmiCalSize == 0)
				DBGLOG(INIT, INFO, "No EMI restore data.\n");
			else {
				if (gEmiCalResult == NULL) {
					DBGLOG(INIT, ERROR,
						"gEmiCalResult NULL\n");
					break;
				}

				DBGLOG(INIT, INFO,
					"Offset(0x%x), Size(0x%x), NoUse(%d), backup(%d)\n",
					gEmiCalOffset, gEmiCalSize,
					gEmiCalNoUseEmiData, backupEMI);

#if (TURN_ON_EMI_BACKUP == 1)
				/*** restore calibration result ******/
				emi_mem_write(prAdapter->chip_info,
					gEmiCalOffset, gEmiCalResult,
					gEmiCalSize);
#endif
				u4Status = WLAN_STATUS_SUCCESS;
			}
		}
	} while (FALSE);
#endif /* CFG_MTK_ANDROID_EMI */

	return u4Status;
}

void wlanGetEpaElnaFromNvram(
	uint8_t **pu1DataPointer,
	uint32_t *pu4DataLen)
{
#define MAX_NVRAM_READY_COUNT 10
#define MAX_NVRAM_FEM_MAX 512

	/* ePA /eLNA */
	uint8_t u1TypeID;
	uint8_t u1LenLSB;
	uint8_t u1LenMSB;
	uint8_t u1Total_Size_LSB, u1Total_Size_MSB;
	uint32_t u4Tag7_9_data_len = 0, u46GCOMM_len = 0;
	uint16_t u2NVRAM_Toal_Size = 0;
	uint32_t u4NvramStartOffset = 0, u4NvramOffset = 0;
	uint8_t *pu1Addr;
	struct WIFI_NVRAM_TAG_FORMAT *prTagDataCurr;
	int retryCount = 0;
	enum WIFI_NVRAM_TAG {
		NVRAM_TAG_NVRAM_CTRL = 0,
		NVRAM_TAG_2G4_TX_POWER = 1,
		NVRAM_TAG_5G_TX_POWER = 2,
		NVRAM_TAG_2G4_WF0_PATH = 3,
		NVRAM_TAG_2G4_WF1_PATH = 4,
		NVRAM_TAG_5G_WF0_PATH = 5,
		NVRAM_TAG_5G_WF1_PATH = 6,
		NVRAM_TAG_2G4_COMMON = 7,
		NVRAM_TAG_5G_COMMON = 8,
		NVRAM_TAG_SYSTEM = 9,
		NVRAM_TAG_CO_ANT = 10,
		NVRAM_TAG_11N_DELAY = 11,
		NVRAM_TAG_2G4_WF0_AUX_PATH = 12,
		NVRAM_TAG_5G_WF0_AUX_PATH = 13,
		NVRAM_TAG_2G4_WF1_AUX_PATH = 14,
		NVRAM_TAG_5G_WF1_AUX_PATH = 15,
		NVRAM_TAG_6G_TX_POWER = 16,
		NVRAM_TAG_6G_WF0_PATH = 17,
		NVRAM_TAG_6G_WF1_PATH = 18,
		NVRAM_TAG_6G_WF0_AUX_PATH = 19,
		NVRAM_TAG_6G_WF1_AUX_PATH = 20,
		NVRAM_TAG_6G_COMMON = 21,
		NVRAM_TAG_NUMBER
	};

	while (g_NvramFsm != NVRAM_STATE_READY) {
		kalMsleep(100);
		retryCount++;

		if (retryCount > MAX_NVRAM_READY_COUNT) {
			DBGLOG(INIT, WARN, "g_NvramFsm != NVRAM_STATE_READY\n");
			return;
		}
	}

	/* Get NVRAM Start Addr */
	pu1Addr = (uint8_t *)(struct WIFI_CFG_PARAM_STRUCT *)&g_aucNvram[0];

	/* Shift to NVRAM Tag */
	u4NvramOffset = OFFSET_OF(struct WIFI_CFG_PARAM_STRUCT, ucTypeID0);
	prTagDataCurr =
		(struct WIFI_NVRAM_TAG_FORMAT *)(pu1Addr
		+ u4NvramOffset);

    /* Shift to Tag ID 0 to get NVRAM total length */
	u1TypeID = prTagDataCurr->u1NvramTypeID;
	if (u1TypeID == NVRAM_TAG_NVRAM_CTRL) {
		u1Total_Size_LSB = g_aucNvram[u4NvramOffset + 4];
		u1Total_Size_MSB = g_aucNvram[u4NvramOffset + 5];
		u2NVRAM_Toal_Size = ((uint16_t)u1Total_Size_MSB << 8)
			 | ((uint16_t)u1Total_Size_LSB);
		u2NVRAM_Toal_Size += 256;  /* NVRAM BASIC data len */
	}

	/* Shift to NVRAM Tag 7 - 9, r2G4Cmm, r5GCmm , rSys*/
	while (u4NvramOffset < u2NVRAM_Toal_Size) {
		u1TypeID = prTagDataCurr->u1NvramTypeID;
		u1LenLSB = prTagDataCurr->u1NvramTypeLenLsb;
		u1LenMSB = prTagDataCurr->u1NvramTypeLenMsb;
		DBGLOG(INIT, TRACE, "CurOfs[%d]:Next(%d)Len:%d\n",
			u4NvramOffset, u1TypeID,
			(u1LenMSB << 8) | (u1LenLSB));

		/*sanity check*/
		if ((u1TypeID == 0) &&
			(u1LenLSB == 0) && (u1LenMSB == 0)) {
			DBGLOG(INIT, TRACE, "TLV is Null\n");
			break;
		}

		/*check Type ID is exist on NVRAM*/
		if (u1TypeID == NVRAM_TAG_2G4_COMMON) {
			u4NvramStartOffset = u4NvramOffset;
			DBGLOG(INIT, TRACE,
			"NVRAM tag(%d) exist! ofst %x\n",
			u1TypeID, u4NvramStartOffset);
		}

		if (u1TypeID == NVRAM_TAG_CO_ANT) {
			*pu1DataPointer = pu1Addr + u4NvramStartOffset;
			*pu4DataLen = u4NvramOffset - u4NvramStartOffset;
			DBGLOG(INIT, TRACE,
			"NVRAM datapointer %x tag7 ofst %x tag7-9 Len %x\n",
			*pu1DataPointer, u4NvramStartOffset, *pu4DataLen);
			kalMemCopy(&g_aucNvram_OnlyPreCal[0],
				&g_aucNvram[u4NvramStartOffset], *pu4DataLen);
			u4Tag7_9_data_len = *pu4DataLen;
		}

		if (u1TypeID == NVRAM_TAG_6G_COMMON) {
			/* Only to get 6G COMMON TLV */
			*pu1DataPointer = pu1Addr + u4NvramOffset;
			u46GCOMM_len = sizeof(struct WIFI_NVRAM_TAG_FORMAT);
			u46GCOMM_len += (u1LenMSB << 8) | (u1LenLSB);
			kalMemCopy(&g_aucNvram_OnlyPreCal[u4Tag7_9_data_len],
			 &g_aucNvram[u4NvramOffset], u46GCOMM_len);
			*pu4DataLen += u46GCOMM_len;
			DBGLOG(INIT, TRACE,
				"NVRAM tag(%d) u46GCOMM_len %d, u4NvramOffset %d, pu4DataLen %d\n",
				u1TypeID, u46GCOMM_len, pu4DataLen);
		}
		u4NvramOffset += sizeof(struct WIFI_NVRAM_TAG_FORMAT);
		u4NvramOffset += (u1LenMSB << 8) | (u1LenLSB);

		/*get the nex TLV format*/
		prTagDataCurr = (struct WIFI_NVRAM_TAG_FORMAT *)
			(pu1Addr + u4NvramOffset);
	}

	/* Get NVRAM Start Addr */
	pu1Addr = (uint8_t *)
		(struct WIFI_CFG_PARAM_STRUCT *)&g_aucNvram_OnlyPreCal[0];
	*pu1DataPointer = pu1Addr;

	if (((*pu4DataLen) & 0x3) != 0) {
		DBGLOG(INIT, WARN,
			"NVRAM datapointer Len adjust to 4 byte alignment(%x->%x)\n",
			*pu4DataLen,
			*pu4DataLen + 4 - ((*pu4DataLen) & 0x3));
		*pu4DataLen += 4 - ((*pu4DataLen) & 0x3);
	}

	if (*pu4DataLen > MAX_NVRAM_FEM_MAX) {
		*pu4DataLen = MAX_NVRAM_FEM_MAX;
		DBGLOG(INIT, WARN,
			"NVRAM datapointer Len(test) adjust (%x) for command max\n",
			*pu4DataLen);
	}

	DBGLOG_MEM8(INIT, TRACE, *pu1DataPointer, *pu4DataLen);
}

uint32_t wlanRcvPhyActionRsp(struct ADAPTER *prAdapter,
	struct HAL_PHY_ACTION_TLV_HEADER *prPhyTlvHeader)
{
	struct HAL_PHY_ACTION_TLV *prPhyTlv;
	struct INIT_EVENT_PHY_ACTION_RSP *prPhyEvent;
	uint32_t u4Status = WLAN_STATUS_FAILURE;

	if (prPhyTlvHeader->u4MagicNum != HAL_PHY_ACTION_MAGIC_NUM) {
		DBGLOG(INIT, ERROR, "HAL_PHY_ACTION_MAGIC_NUM failed\n");
		u4Status = WLAN_STATUS_FAILURE;
		goto exit;
	}

	prPhyTlv =
		(struct HAL_PHY_ACTION_TLV *)prPhyTlvHeader->aucBuffer;

	prPhyEvent = (struct INIT_EVENT_PHY_ACTION_RSP *)
		prPhyTlv->aucBuffer;

	if (prPhyTlv->u2Tag == HAL_PHY_ACTION_TAG_CAL) {
		DBGLOG(INIT, INFO,
			"HAL_PHY_ACTION_TAG_CAL ucEvent[0x%x]status[0x%x]emiAddr[0x%x]emiLen[0x%x]\n",
			prPhyEvent->ucEvent,
			prPhyEvent->ucStatus,
			prPhyEvent->u4EmiAddress,
			prPhyEvent->u4EmiLength);

		if ((prPhyEvent->ucEvent == HAL_PHY_ACTION_CAL_FORCE_CAL_RSP &&
		     prPhyEvent->ucStatus == HAL_PHY_ACTION_STATUS_SUCCESS) ||
		    (prPhyEvent->ucEvent == HAL_PHY_ACTION_CAL_USE_BACKUP_RSP &&
		     prPhyEvent->ucStatus == HAL_PHY_ACTION_STATUS_RECAL)) {
			/* read from EMI, backup in driver */
			wlanAccessCalibrationEMI(prAdapter,
				prPhyEvent,
				TRUE);
		}

		u4Status = WLAN_STATUS_SUCCESS;
	} else if (prPhyTlv->u2Tag == HAL_PHY_ACTION_TAG_NVRAM) {
		DBGLOG(INIT, INFO,
			"HAL_PHY_ACTION_TAG_NVRAM status[0x%x]\n",
			prPhyEvent->ucStatus);

		u4Status = WLAN_STATUS_SUCCESS;
	} else if (prPhyTlv->u2Tag == HAL_PHY_ACTION_TAG_COM_FEM) {
		DBGLOG(INIT, INFO,
			"HAL_PHY_ACTION_TAG_COM_FEM status[0x%x]\n",
			prPhyEvent->ucStatus);

		u4Status = WLAN_STATUS_SUCCESS;
	}

exit:
	return u4Status;
}

#if (CFG_SUPPORT_CONNFEM == 1)
struct COM_FEM_TAG_FORMAT {
	uint32_t tag_fem_info_id;
	struct connfem_epaelna_pin_info tag_pin_info;
#if (CFG_SUPPORT_CONNAC3X == 1)
	struct connfem_epaelna_flags_common tag_flags_common;
#endif
};

struct LAA_TAG_FORMAT {
	struct connfem_epaelna_laa_pin_info tag_laa_pin_info;
};
#endif

uint32_t wlanSendPhyAction(struct ADAPTER *prAdapter,
	uint16_t u2Tag,
	uint8_t ucCalCmd)
{
	struct HAL_PHY_ACTION_TLV_HEADER *prCmd = NULL;
	struct HAL_PHY_ACTION_TLV_HEADER *prEvent = NULL;
	struct HAL_PHY_ACTION_TLV *prPhyTlv;
	struct INIT_CMD_PHY_ACTION_CAL *prPhyCal;
#if (CFG_SUPPORT_CONNFEM == 1)
	struct COM_FEM_TAG_FORMAT *prTagDataComFEM;
	struct LAA_TAG_FORMAT *prTagDataLAA;
	struct connfem_epaelna_fem_info fem_info;
	struct connfem_epaelna_pin_info pin_info;
	struct connfem_epaelna_laa_pin_info laa_pin_info;
#if (CFG_SUPPORT_CONNAC3X == 1)
	struct connfem_epaelna_flags_common flags_common;
#endif
#endif
	uint8_t *u1EpaELnaDataPointer = NULL;
	uint32_t u4EpaELnaDataSize = 0, u4CmdSize = 0, u4EvtSize = 0;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;

	DBGLOG(INIT, INFO, "SendPhyAction begin, tag: %d, cmd: %d, skip: %d\n",
		u2Tag, ucCalCmd, g_fgCalDisabled);

	ASSERT(prAdapter);

	wlanGetEpaElnaFromNvram(&u1EpaELnaDataPointer,
		&u4EpaELnaDataSize);

	if (u1EpaELnaDataPointer == NULL) {
		DBGLOG(INIT, ERROR, "Get u1EpaELnaDataPointer failed\n");
		return WLAN_STATUS_FAILURE;
	}

#if (CFG_SUPPORT_CONNFEM == 1)
	/* Get data from connfem_api */
	connfem_epaelna_get_fem_info(&fem_info);
	connfem_epaelna_get_pin_info(&pin_info);
	connfem_epaelna_laa_get_pin_info(&laa_pin_info);
#if (CFG_SUPPORT_CONNAC3X == 1)
	connfem_epaelna_get_flags(CONNFEM_SUBSYS_NONE, &flags_common);
#endif
#endif

	/* 1. Allocate CMD Info Packet and its Buffer. */
	if (u2Tag == HAL_PHY_ACTION_TAG_NVRAM) {
		u4CmdSize = sizeof(struct HAL_PHY_ACTION_TLV_HEADER) +
			sizeof(struct HAL_PHY_ACTION_TLV) +
			u4EpaELnaDataSize;
#if (CFG_SUPPORT_CONNFEM == 1)
	} else if (u2Tag == HAL_PHY_ACTION_TAG_COM_FEM) {
		u4CmdSize = sizeof(struct HAL_PHY_ACTION_TLV_HEADER) +
			sizeof(struct HAL_PHY_ACTION_TLV) +
			u4EpaELnaDataSize +
			sizeof(struct HAL_PHY_ACTION_TLV) +
			sizeof(struct COM_FEM_TAG_FORMAT) +
			sizeof(struct HAL_PHY_ACTION_TLV) +
			sizeof(struct LAA_TAG_FORMAT);
	} else if (u2Tag == HAL_PHY_ACTION_TAG_LAA) {
		u4CmdSize = sizeof(struct HAL_PHY_ACTION_TLV_HEADER) +
			sizeof(struct HAL_PHY_ACTION_TLV) +
			u4EpaELnaDataSize +
			sizeof(struct HAL_PHY_ACTION_TLV) +
			sizeof(struct COM_FEM_TAG_FORMAT) +
			sizeof(struct HAL_PHY_ACTION_TLV) +
			sizeof(struct LAA_TAG_FORMAT);
#endif
	} else {
		u4CmdSize = sizeof(struct HAL_PHY_ACTION_TLV_HEADER) +
			sizeof(struct HAL_PHY_ACTION_TLV) +
			sizeof(struct INIT_CMD_PHY_ACTION_CAL);
#if (CFG_SUPPORT_CONNFEM == 1)
		u4CmdSize += sizeof(struct HAL_PHY_ACTION_TLV) +
			sizeof(struct COM_FEM_TAG_FORMAT);
#endif
	}

	if ((ucCalCmd == HAL_PHY_ACTION_CAL_FORCE_CAL_REQ) ||
		(ucCalCmd == HAL_PHY_ACTION_CAL_USE_BACKUP_REQ)) {
		u4CmdSize = sizeof(struct HAL_PHY_ACTION_TLV_HEADER) +
			sizeof(struct HAL_PHY_ACTION_TLV) +
			sizeof(struct INIT_CMD_PHY_ACTION_CAL) +
			sizeof(struct HAL_PHY_ACTION_TLV) +
			u4EpaELnaDataSize;
#if (CFG_SUPPORT_CONNFEM == 1)
		u4CmdSize += sizeof(struct HAL_PHY_ACTION_TLV) +
			sizeof(struct COM_FEM_TAG_FORMAT) +
			sizeof(struct HAL_PHY_ACTION_TLV) +
			sizeof(struct LAA_TAG_FORMAT);
#endif
	}

	prCmd = kalMemAlloc(u4CmdSize, VIR_MEM_TYPE);
	if (!prCmd) {
		DBGLOG(INIT, ERROR, "Alloc cmd packet failed.\n");
		u4Status = WLAN_STATUS_FAILURE;
		goto exit;
	}

	u4EvtSize = sizeof(struct HAL_PHY_ACTION_TLV_HEADER) +
		sizeof(struct HAL_PHY_ACTION_TLV) +
		sizeof(struct INIT_EVENT_PHY_ACTION_RSP);
	prEvent = kalMemAlloc(u4EvtSize, VIR_MEM_TYPE);
	if (!prEvent) {
		DBGLOG(INIT, ERROR, "Alloc event packet failed.\n");
		u4Status = WLAN_STATUS_FAILURE;
		goto exit;
	}

	/* Process TLV Header Part1 */
	prCmd->u4MagicNum = HAL_PHY_ACTION_MAGIC_NUM;
	prCmd->ucVersion = HAL_PHY_ACTION_VERSION;

	if (u2Tag == HAL_PHY_ACTION_TAG_NVRAM) {
		/* Process TLV Header Part2 */
		/* Process HAL_PHY_ACTION_TAG_NVRAM only */
		prCmd->ucTagNums = 1;
		prCmd->u2BufLength =
			sizeof(struct HAL_PHY_ACTION_TLV) +
			u4EpaELnaDataSize;

		/* Process TLV Content */
		/* TAG HAL_PHY_ACTION_TAG_NVRAM */
		prPhyTlv =
			(struct HAL_PHY_ACTION_TLV *)prCmd->aucBuffer;
		prPhyTlv->u2Tag = HAL_PHY_ACTION_TAG_NVRAM;
		prPhyTlv->u2BufLength = u4EpaELnaDataSize;
		kalMemCopy(prPhyTlv->aucBuffer,
			u1EpaELnaDataPointer, u4EpaELnaDataSize);
#if (CFG_SUPPORT_CONNFEM == 1)
	} else if (u2Tag == HAL_PHY_ACTION_TAG_COM_FEM ||
		u2Tag == HAL_PHY_ACTION_TAG_LAA) {
		/* Process TLV Header Part2 */
		/* Add HAL_PHY_ACTION_TAG_COM_FEM and HAL_PHY_ACTION_TAG_LAA */
		prCmd->ucTagNums = 3;
		prCmd->u2BufLength =
			sizeof(struct HAL_PHY_ACTION_TLV) +
			u4EpaELnaDataSize +
			sizeof(struct HAL_PHY_ACTION_TLV) +
			sizeof(struct COM_FEM_TAG_FORMAT) +
			sizeof(struct HAL_PHY_ACTION_TLV) +
			sizeof(struct LAA_TAG_FORMAT);

		/* Process TLV Content */
		/* TAG HAL_PHY_ACTION_TAG_NVRAM */
		prPhyTlv =
			(struct HAL_PHY_ACTION_TLV *)prCmd->aucBuffer;
		prPhyTlv->u2Tag = HAL_PHY_ACTION_TAG_NVRAM;
		prPhyTlv->u2BufLength = u4EpaELnaDataSize;
		kalMemCopy(prPhyTlv->aucBuffer,
			u1EpaELnaDataPointer, u4EpaELnaDataSize);

		/* TAG HAL_PHY_ACTION_TAG_COM_FEM */
		prPhyTlv =
			(struct HAL_PHY_ACTION_TLV *)
			(prCmd->aucBuffer +
			sizeof(struct HAL_PHY_ACTION_TLV) +
			u4EpaELnaDataSize);
		prPhyTlv->u2Tag = HAL_PHY_ACTION_TAG_COM_FEM;
		prPhyTlv->u2BufLength = sizeof(struct COM_FEM_TAG_FORMAT);
		prTagDataComFEM =
			(struct COM_FEM_TAG_FORMAT *)prPhyTlv->aucBuffer;
		prTagDataComFEM->tag_fem_info_id = fem_info.id;
		kalMemCopy(&prTagDataComFEM->tag_pin_info,
			&pin_info, sizeof(struct connfem_epaelna_pin_info));
#if (CFG_SUPPORT_CONNAC3X == 1)
		kalMemCopy(&prTagDataComFEM->tag_flags_common,
			&flags_common,
			sizeof(struct connfem_epaelna_flags_common));
#endif

		/* TAG HAL_PHY_ACTION_TAG_LAA */
		prPhyTlv =
			(struct HAL_PHY_ACTION_TLV *)
			(prCmd->aucBuffer +
			sizeof(struct HAL_PHY_ACTION_TLV) +
			u4EpaELnaDataSize +
			sizeof(struct HAL_PHY_ACTION_TLV) +
			sizeof(struct COM_FEM_TAG_FORMAT));
		prPhyTlv->u2Tag = HAL_PHY_ACTION_TAG_LAA;
		prPhyTlv->u2BufLength = sizeof(struct LAA_TAG_FORMAT);
		prTagDataLAA =
			(struct LAA_TAG_FORMAT *)prPhyTlv->aucBuffer;
		kalMemCopy(&prTagDataLAA->tag_laa_pin_info,
			&laa_pin_info,
			sizeof(struct connfem_epaelna_laa_pin_info));
#endif
	} else if (
		(ucCalCmd == HAL_PHY_ACTION_CAL_FORCE_CAL_REQ) ||
		(ucCalCmd == HAL_PHY_ACTION_CAL_USE_BACKUP_REQ)) {
		/* Process TLV Header Part2 */
		/* Add HAL_PHY_ACTION_TAG_NVRAM */
		prCmd->ucTagNums = 2;
		prCmd->u2BufLength =
			sizeof(struct HAL_PHY_ACTION_TLV) +
			sizeof(struct INIT_CMD_PHY_ACTION_CAL) +
			sizeof(struct HAL_PHY_ACTION_TLV) +
			u4EpaELnaDataSize;
#if (CFG_SUPPORT_CONNFEM == 1)
		/* Add HAL_PHY_ACTION_TAG_NVRAM, HAL_PHY_ACTION_TAG_COM_FEM */
		/* and HAL_PHY_ACTION_TAG_LAA */
		prCmd->ucTagNums = 4;
		prCmd->u2BufLength =
			sizeof(struct HAL_PHY_ACTION_TLV) +
			sizeof(struct INIT_CMD_PHY_ACTION_CAL) +
			sizeof(struct HAL_PHY_ACTION_TLV) +
			u4EpaELnaDataSize +
			sizeof(struct HAL_PHY_ACTION_TLV) +
			sizeof(struct COM_FEM_TAG_FORMAT) +
			sizeof(struct HAL_PHY_ACTION_TLV) +
			sizeof(struct LAA_TAG_FORMAT);
#endif

		/* Process TLV Content */
		/* TAG HAL_PHY_ACTION_TAG_CAL */
		prPhyTlv =
			(struct HAL_PHY_ACTION_TLV *)prCmd->aucBuffer;
		prPhyTlv->u2Tag = HAL_PHY_ACTION_TAG_CAL;
		prPhyTlv->u2BufLength = sizeof(struct INIT_CMD_PHY_ACTION_CAL);
		prPhyCal =
			(struct INIT_CMD_PHY_ACTION_CAL *)prPhyTlv->aucBuffer;
		prPhyCal->ucCmd = ucCalCmd;
#if CFG_MTK_ANDROID_WMT
		prPhyCal->ucCalSaveResult = 1;
#else
		prPhyCal->ucCalSaveResult = 0;
#endif
		prPhyCal->ucSkipCal = g_fgCalDisabled;

		/* TAG HAL_PHY_ACTION_TAG_NVRAM */
		prPhyTlv =
			(struct HAL_PHY_ACTION_TLV *)
			(prCmd->aucBuffer +
			sizeof(struct HAL_PHY_ACTION_TLV) +
			sizeof(struct INIT_CMD_PHY_ACTION_CAL));
		prPhyTlv->u2Tag = HAL_PHY_ACTION_TAG_NVRAM;
		prPhyTlv->u2BufLength = u4EpaELnaDataSize;
		kalMemCopy(prPhyTlv->aucBuffer,
		u1EpaELnaDataPointer, u4EpaELnaDataSize);

#if (CFG_SUPPORT_CONNFEM == 1)
		/* TAG HAL_PHY_ACTION_TAG_COM_FEM */
		prPhyTlv =
			(struct HAL_PHY_ACTION_TLV *)
			(prCmd->aucBuffer +
			sizeof(struct HAL_PHY_ACTION_TLV) +
			sizeof(struct INIT_CMD_PHY_ACTION_CAL) +
			sizeof(struct HAL_PHY_ACTION_TLV) +
			u4EpaELnaDataSize);
		prPhyTlv->u2Tag = HAL_PHY_ACTION_TAG_COM_FEM;
		prPhyTlv->u2BufLength = sizeof(struct COM_FEM_TAG_FORMAT);
		prTagDataComFEM =
			(struct COM_FEM_TAG_FORMAT *)prPhyTlv->aucBuffer;
		prTagDataComFEM->tag_fem_info_id = fem_info.id;
		kalMemCopy(&prTagDataComFEM->tag_pin_info,
			&pin_info, sizeof(struct connfem_epaelna_pin_info));
#if (CFG_SUPPORT_CONNAC3X == 1)
		kalMemCopy(&prTagDataComFEM->tag_flags_common,
			&flags_common,
			sizeof(struct connfem_epaelna_flags_common));
#endif

		/* TAG HAL_PHY_ACTION_TAG_LAA */
		prPhyTlv =
			(struct HAL_PHY_ACTION_TLV *)
			(prCmd->aucBuffer +
			sizeof(struct HAL_PHY_ACTION_TLV) +
			sizeof(struct INIT_CMD_PHY_ACTION_CAL) +
			sizeof(struct HAL_PHY_ACTION_TLV) +
			u4EpaELnaDataSize +
			sizeof(struct HAL_PHY_ACTION_TLV) +
			sizeof(struct COM_FEM_TAG_FORMAT));
		prPhyTlv->u2Tag = HAL_PHY_ACTION_TAG_LAA;
		prPhyTlv->u2BufLength = sizeof(struct LAA_TAG_FORMAT);
		prTagDataLAA =
			(struct LAA_TAG_FORMAT *)prPhyTlv->aucBuffer;
		kalMemCopy(&prTagDataLAA->tag_laa_pin_info,
			&laa_pin_info,
			sizeof(struct connfem_epaelna_laa_pin_info));
#endif
	} else {
		/* Process TLV Header Part2 */
		prCmd->ucTagNums = 1;
		prCmd->u2BufLength =
			sizeof(struct HAL_PHY_ACTION_TLV) +
			sizeof(struct INIT_CMD_PHY_ACTION_CAL);
#if (CFG_SUPPORT_CONNFEM == 1)
		prCmd->ucTagNums = 2;
		prCmd->u2BufLength =
			sizeof(struct HAL_PHY_ACTION_TLV) +
			sizeof(struct INIT_CMD_PHY_ACTION_CAL) +
			sizeof(struct HAL_PHY_ACTION_TLV) +
			sizeof(struct COM_FEM_TAG_FORMAT);
#endif

		/* Process TLV Content */
		/* TAG HAL_PHY_ACTION_TAG_CAL */
		prPhyTlv =
			(struct HAL_PHY_ACTION_TLV *)prCmd->aucBuffer;
		prPhyTlv->u2Tag = HAL_PHY_ACTION_TAG_CAL;
		prPhyTlv->u2BufLength = sizeof(struct INIT_CMD_PHY_ACTION_CAL);
		prPhyCal =
			(struct INIT_CMD_PHY_ACTION_CAL *)prPhyTlv->aucBuffer;
		prPhyCal->ucCmd = ucCalCmd;
#if CFG_MTK_ANDROID_WMT
		prPhyCal->ucCalSaveResult = 1;
#else
		prPhyCal->ucCalSaveResult = 0;
#endif
		prPhyCal->ucSkipCal = g_fgCalDisabled;

#if (CFG_SUPPORT_CONNFEM == 1)
		/* TAG HAL_PHY_ACTION_TAG_COM_FEM */
		prPhyTlv =
			(struct HAL_PHY_ACTION_TLV *)
			(prCmd->aucBuffer +
			sizeof(struct HAL_PHY_ACTION_TLV) +
			sizeof(struct INIT_CMD_PHY_ACTION_CAL));
		prPhyTlv->u2Tag = HAL_PHY_ACTION_TAG_COM_FEM;
		prPhyTlv->u2BufLength = sizeof(struct COM_FEM_TAG_FORMAT);
		prTagDataComFEM =
			(struct COM_FEM_TAG_FORMAT *)prPhyTlv->aucBuffer;
		prTagDataComFEM->tag_fem_info_id = fem_info.id;
		kalMemCopy(&prTagDataComFEM->tag_pin_info,
			&pin_info, sizeof(struct connfem_epaelna_pin_info));
#if (CFG_SUPPORT_CONNAC3X == 1)
		kalMemCopy(&prTagDataComFEM->tag_flags_common,
			&flags_common,
			sizeof(struct connfem_epaelna_flags_common));
#endif
#endif
	}

	u4Status = wlanSendInitSetQueryCmdImpl(prAdapter,
		INIT_CMD_ID_PHY_ACTION, prCmd, u4CmdSize,
		TRUE, FALSE,
		INIT_EVENT_ID_PHY_ACTION, prEvent, u4EvtSize,
		CFG_PRE_CAL_SLEEP_WAITING_INTERVAL,
		CFG_PRE_CAL_RX_RESPONSE_TIMEOUT);
	if (u4Status != WLAN_STATUS_SUCCESS)
		goto exit;

	u4Status = wlanRcvPhyActionRsp(prAdapter, prEvent);

exit:
	if (prCmd)
		kalMemFree(prCmd, VIR_MEM_TYPE, u4CmdSize);

	if (prEvent)
		kalMemFree(prEvent, VIR_MEM_TYPE, u4EvtSize);

	return u4Status;
}

uint32_t wlanPhyAction(struct ADAPTER *prAdapter)
{
	uint32_t u4Status = WLAN_STATUS_SUCCESS;

	DBGLOG(INIT, INFO, "fgPreCal = %d\n", g_fgPreCal);

	if (g_fgPreCal == FALSE) {
		/* Setup calibration data from backup file */
#if CFG_MTK_ANDROID_WMT
		if (wlanAccessCalibrationEMI(prAdapter, NULL, FALSE) ==
			WLAN_STATUS_SUCCESS)
			u4Status = wlanSendPhyAction(prAdapter,
				HAL_PHY_ACTION_TAG_CAL,
				HAL_PHY_ACTION_CAL_USE_BACKUP_REQ);
		else
#endif
			u4Status = wlanSendPhyAction(prAdapter,
				HAL_PHY_ACTION_TAG_CAL,
				HAL_PHY_ACTION_CAL_FORCE_CAL_REQ);
	} else {
#if (CFG_SUPPORT_CONNFEM == 1)
		wlanSendPhyAction(prAdapter,
			HAL_PHY_ACTION_TAG_COM_FEM,
			0);
#else
		wlanSendPhyAction(prAdapter,
			HAL_PHY_ACTION_TAG_NVRAM,
			0);
#endif
		wlanSendPhyAction(prAdapter,
			HAL_PHY_ACTION_TAG_CAL,
			HAL_PHY_ACTION_CAL_FORCE_CAL_REQ);
	}

	return u4Status;
}

#if CFG_MTK_ANDROID_WMT
int wlan_precal_get_res(uint32_t *pEmiCalOffset, uint32_t *pEmiCalSize)
{
#if CFG_MTK_ANDROID_EMI
	/* Shift 4 for bypass Cal result CRC */
	*pEmiCalOffset = gEmiCalOffset + 0x4;

	/* 2k size for RFCR backup */
	*pEmiCalSize = 2048;
#endif

	DBGLOG(INIT, INFO, "EMI_GET_CAL emiAddr[0x%x]emiLen[%d]\n",
		*pEmiCalOffset,
		*pEmiCalSize);

	return WLAN_STATUS_SUCCESS;
}

int wlan_precal_pwron_v1(void)
{
	DBGLOG(INIT, INFO, "ever = %d\n", g_fgEverCal);

	if (g_fgEverCal == TRUE)
		return 1;

#if CFG_MTK_ANDROID_EMI
	/* CONNAC 2 use backup restore EMI */
	gEmiCalNoUseEmiData = FALSE;
#endif

	wfsys_lock();

	return 0;
}

int wlan_precal_docal_v1(void)
{
	int32_t ret = 0;

	DBGLOG(INIT, INFO, "ever = %d\n", g_fgEverCal);

	if (!g_fgEverCal) {
		update_pre_cal_status(1);
		g_fgPreCal = TRUE;

		ret = wlanFuncOnImpl();
		if (ret) {
			DBGLOG(INIT, ERROR, "failed, ret=%d\n", ret);
			goto exit;
		}

		wlanFuncOffImpl();

exit:
		g_fgPreCal = FALSE;
		update_pre_cal_status(0);
		g_fgEverCal = TRUE;

		wfsys_unlock();
	}
	return ret;
}

int wlan_precal_pwron_v2(void)
{
	int32_t ret = 0;

	DBGLOG(INIT, INFO, "\n");

#if CFG_MTK_ANDROID_EMI
	/* CONNAC 3 , no use backup /restore EMI */
	/* (FW no use CRC , can't change region) */
	gEmiCalNoUseEmiData = TRUE;
#endif

	if (!wfsys_is_locked())
		wfsys_lock();

	update_pre_cal_status(1);
	g_fgPreCal = TRUE;

	ret = wlanFuncOnImpl();
	if (ret)
		goto exit;

	wlanFuncOffImpl();

exit:
	g_fgPreCal = FALSE;
	update_pre_cal_status(0);

	if (ret) {
		DBGLOG(INIT, ERROR, "failed, ret=%d\n", ret);
		wfsys_unlock();
	}

	return ret;
}

int wlan_precal_docal_v2(void)
{
	DBGLOG(INIT, INFO, "\n");

	if (!g_fgEverCal)
		g_fgEverCal = TRUE;

	if (wfsys_is_locked())
		wfsys_unlock();

	return 0;
}

int wlan_precal_err(void)
{
	DBGLOG(INIT, INFO, "\n");

	if (!g_fgEverCal)
		g_fgEverCal = TRUE;

	if (wfsys_is_locked())
		wfsys_unlock();

	return 0;
}

void set_cal_enabled(u_int8_t enabled)
{
	if (enabled)
		g_fgCalDisabled = FALSE;
	else
		g_fgCalDisabled = TRUE;
}

u_int8_t is_cal_flow_finished(void)
{
	return g_fgEverCal;
}
#endif

void wlanCalDebugCmd(uint32_t cmd, uint32_t para)
{
#if CFG_MTK_ANDROID_EMI
	switch (cmd) {
	case 0:
		gEmiCalOffset = 0;
		gEmiCalSize = 0;
		break;

	case 1:
		if (para == 1)
			gEmiCalNoUseEmiData = TRUE;
		else
			gEmiCalNoUseEmiData = FALSE;
		break;
	}

	DBGLOG(RFTEST, INFO,
		"gEmiCalOffset(0x%x), gEmiCalSize(0x%x), gEmiCalNoUseEmiData(%d)\n",
		gEmiCalOffset, gEmiCalSize, gEmiCalNoUseEmiData);
#endif
}
