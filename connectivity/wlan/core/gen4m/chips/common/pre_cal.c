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
#if (CFG_SUPPORT_CONNFEM == 1)
#include "connfem_api.h"
#endif

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

#define PALIGN_4(_value)             (((_value) + 3) & ~3u)

#if (CFG_SUPPORT_CONNFEM == 1) && !defined(CONNFEM_API_VERSION)
#define CONNFEM_API_VERSION 1
#endif

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

#if ((CFG_SUPPORT_CONNFEM == 1) && (CONNFEM_API_VERSION >= 2))

#define PHYACT_MAX_FEM_NUM 8
#define PHYACT_MAX_PIN_NUM 6

#define PHYACT_MAX_STATE_NUM 32
#define PHYACT_MAX_STATE_CAT_NUM 8
#define PHYACT_MAX_ST_CAT_NUM 10
#define PHYACT_MAX_USAGE_NUM 8
#define HAL_PHY_EPA_CUS_LAYOUT_NUM 32


enum PreCal_TagID {
	PC_TAG_ID_NVRAM = 0,
	PC_TAG_ID_CONNFEM = 1,
	PC_TAG_ID_CAL = 2
};

enum ENUM_INICMD_CONN_FEM_HEADER_T {
	ENUM_INICMD_CONN_FEM_HEADER_FEM     = 1,
	ENUM_INICMD_CONN_FEM_HEADER_SPDT    = 2,
	ENUM_INICMD_CONN_FEM_HEADER_WF      = 3,
	ENUM_INICMD_CONN_FEM_HEADER_BT      = 4
};



struct PHYACT_CUS_PIN_T {
	uint8_t u1AntSelNo;
	uint8_t u1FemPin;
	uint8_t u1Polarity;
};


/*******************************************************
 ***************** FEM info *****************************
 ********************************************************/

struct PHYACT_FEM_TT_T {
	uint8_t u1StateName;
	uint8_t u1TruthTable;
};

struct PHYACT_FEM_INFO_T {
	uint32_t u4FemId;
	uint32_t u4Flag;
	uint8_t u1PinNum;
	uint8_t u1PinName[PHYACT_MAX_PIN_NUM];
	uint8_t u1StateNum;
	struct PHYACT_FEM_TT_T rState[PHYACT_MAX_STATE_NUM];
};

struct PHYACT_FEM_INFOS_T {
	uint8_t u1FemCount;
	uint8_t u1Reserve[3];
	struct PHYACT_FEM_INFO_T rFem[PHYACT_MAX_FEM_NUM];
};

/*******************************************************
 ***************** FEM layout usage *********************
 ********************************************************/
struct PHYACT_LAYOUT_USAGE_PIN_T {
	uint8_t u1PinName;
	uint8_t u1AntSelNo;
	uint8_t u1Polarity;
	uint8_t u1Reserve[1];    //4-byte alignment reserve
};

struct PHYACT_LAYOUT_USAGE_T {
	uint8_t u1BandPathWf;
	uint8_t u1BandPathBt;
	uint8_t u1FemIdx;
	uint8_t u1PinNum;
	struct PHYACT_LAYOUT_USAGE_PIN_T rPin[PHYACT_MAX_PIN_NUM];
};


struct PHYACT_LAYOUT_USAGES_T {
	uint8_t u1LayoutUsageCount;
	uint8_t u1Reserve[3];
	uint32_t u4Flag;
	struct PHYACT_LAYOUT_USAGE_T rLayout[PHYACT_MAX_USAGE_NUM];
};

/*******************************************************
 ***************** FEM used state usage *********************
 ********************************************************/
struct PHYACT_STATE_USAGE_CAT_T {
	uint8_t u1Cat;   /* ENUM_INICMD_EFEM_STATE_CAT_T*/
	uint8_t u1StateUsedNum;
	uint8_t u1StateUsedName[PHYACT_MAX_ST_CAT_NUM];
};

struct PHYACT_STATE_USAGE_T {
	uint8_t u1FemIdx;
	uint8_t u1StateCatNum;
	uint8_t u1Reserve[2];
	struct PHYACT_STATE_USAGE_CAT_T u1StateCat[PHYACT_MAX_STATE_CAT_NUM];
};

struct PHYACT_STATE_USAGES_T {
	uint8_t u1FemCount;
	uint8_t u1Reserve[3];
	struct PHYACT_STATE_USAGE_T rFem[PHYACT_MAX_FEM_NUM];
};

/*******************************************************
 ***************** connfem  *********************
 ********************************************************/

struct PHYACT_CONN_FEM_HEADER_T {
	uint8_t  u1Tag;
	uint8_t  u1Version;
	uint16_t u2Len;
};

struct PHYACT_CONN_FEM_FEM_V2_T {
	struct PHYACT_CONN_FEM_HEADER_T header;

	uint32_t u4LayoutId; /* still no used*/
	struct PHYACT_FEM_INFOS_T rFemInfo;
	struct PHYACT_LAYOUT_USAGES_T rLayoutUsage;
};

struct PHYACT_CONN_FEM_WF_V2_T {
	struct PHYACT_CONN_FEM_HEADER_T header;
	struct PHYACT_STATE_USAGES_T rStateUsage;

};

struct PHYACT_CONN_FEM_SPDT_V2_T {
	struct PHYACT_CONN_FEM_HEADER_T header;

    //V2 start
	uint8_t u1SpdtInfo;
	uint8_t u1SpdtInfo2;
	uint8_t u1SpdtInfo3;
	uint8_t u1SpdtInfo4;
	uint8_t u1SpdtInfo5;
	uint8_t u1PinNum;
	struct PHYACT_CUS_PIN_T rPin[HAL_PHY_EPA_CUS_LAYOUT_NUM];

};

#endif /* #if(CONNFEM_API_VERSION >= 2) */

void wlanDebugDumpCalibrationEMI(
	uint8_t *pucEmiStartAddr,
	uint32_t u4EmiSize)
{
#if CFG_MTK_ANDROID_EMI

	uint32_t i, index = 0, u4ArrSize = 0;
	uint32_t *p4ucSum = NULL;

	u4ArrSize = (u4EmiSize/1000 + 1) * sizeof(uint32_t);

	p4ucSum = kalMemAlloc(u4ArrSize, VIR_MEM_TYPE);

	if (p4ucSum == NULL) {
		DBGLOG(INIT, ERROR,
			"puSum kalMemAlloc NULL\n");
		return;
	}

	kalMemSet(p4ucSum, 0, u4ArrSize);

	for (i = 0; i < u4EmiSize; i++) {
		index = i/1000;
		*(p4ucSum + index) += *(pucEmiStartAddr + i);
	}

	for (i = 0; i < (u4EmiSize/1000 + 1); i++)
		DBGLOG(INIT, DEBUG, "Sum[%d]=0x%08x\n", i, *(p4ucSum + i));

	kalMemFree(p4ucSum, VIR_MEM_TYPE, u4ArrSize);
#endif
}

uint32_t wlanAccessCalibrationEMI(struct ADAPTER *prAdapter,
	struct INIT_EVENT_PHY_ACTION_RSP *pCalEvent,
	uint8_t backupEMI)
{
	uint32_t u4Status = WLAN_STATUS_FAILURE;

#if CFG_MTK_ANDROID_EMI
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	struct HIF_MEM_OPS *prMemOps = &prHifInfo->rMemOps;
	struct HIF_MEM *prMem = NULL;
	uint8_t *prEmi2Address = NULL;

	#define TURN_ON_EMI_BACKUP 1
	#define DUMP_PRE_CAL_RESULT 0

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
				DBGLOG(INIT, DEBUG, "No EMI backup.\n");
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

			DBGLOG(INIT, DEBUG,
				"Offset(0x%x), Size(0x%x), NoUse(%d), backup(%d)\n",
					gEmiCalOffset, gEmiCalSize,
					gEmiCalNoUseEmiData, backupEMI);

#if (TURN_ON_EMI_BACKUP == 1)
			if (prMemOps->getWifiMiscRsvEmi) {
				prMem = prMemOps->getWifiMiscRsvEmi(prChipInfo,
					WIFI_MISC_MEM_BLOCK_PRECAL);
				if (prMem != 0) {
					prEmi2Address =
						(uint8_t *)prMem->va;
					if (prEmi2Address == NULL) {
						DBGLOG(INIT, ERROR,
							"PreCal EMI2 Address is NULL(1)\n");
						break;
					}

					/* backup force cal result data of emi2
					 * to driver buffer
					 */
					kalMemCopyFromIo(gEmiCalResult,
						prEmi2Address, gEmiCalSize);

					/* dump cal result for debug */
#if (DUMP_PRE_CAL_RESULT == 1)
					wlanDebugDumpCalibrationEMI(
						gEmiCalResult,
						gEmiCalSize);
#endif
				} else {
					DBGLOG(INIT, DEBUG,
						"precal prMem(1) = 0x%x\n",
						prMem);

					emi_mem_read(prAdapter->chip_info,
					gEmiCalOffset, gEmiCalResult,
					gEmiCalSize);
				}
			} else {
				DBGLOG(INIT, ERROR,
					"getWifiMiscRsvEmi is null(1)\n");

				emi_mem_read(prAdapter->chip_info,
				gEmiCalOffset, gEmiCalResult,
				gEmiCalSize);
			}
#endif

			u4Status = WLAN_STATUS_SUCCESS;
		} else {
			if (gEmiCalNoUseEmiData == TRUE) {
				DBGLOG(INIT, DEBUG, "No EMI restore.\n");
				u4Status = WLAN_STATUS_SUCCESS;
				break;
			}

			if (gEmiCalOffset == 0 || gEmiCalSize == 0) {
				DBGLOG(INIT, DEBUG, "No EMI restore data.\n");
				break;
			}

			if (gEmiCalResult == NULL) {
				DBGLOG(INIT, ERROR,
					"gEmiCalResult NULL\n");
				break;
			}

			DBGLOG(INIT, DEBUG,
				"Offset(0x%x), Size(0x%x), NoUse(%d), backup(%d)\n",
				gEmiCalOffset, gEmiCalSize,
				gEmiCalNoUseEmiData, backupEMI);

#if (TURN_ON_EMI_BACKUP == 1)
			/*** restore calibration result ******/
			if (prMemOps->getWifiMiscRsvEmi) {
				prMem = prMemOps->getWifiMiscRsvEmi(
					prChipInfo,
					WIFI_MISC_MEM_BLOCK_PRECAL);
				if (prMem != 0) {
					prEmi2Address =
						(uint8_t *)prMem->va;
					if (prEmi2Address == NULL) {
						DBGLOG(INIT, ERROR,
							"PreCal EMI2 Address is NULL(2)\n");
						break;
					}

					/* Restore force cal result data of
					 * driver buffer overwrite to emi2
					 */
					kalMemCopyToIo(prEmi2Address,
						gEmiCalResult,
						gEmiCalSize);

					/* dump cal result for debug */
#if (DUMP_PRE_CAL_RESULT == 1)
					wlanDebugDumpCalibrationEMI(
						prEmi2Address,
						gEmiCalSize);
#endif
				} else {
					DBGLOG(INIT, DEBUG,
					"precal prMem(2) = 0x%x\n",
					prMem);

					emi_mem_write(
						prAdapter->chip_info,
						gEmiCalOffset,
						gEmiCalResult,
						gEmiCalSize);
				}
			} else {
				DBGLOG(INIT, ERROR,
					"getWifiMiscRsvEmi is null(2)\n");

				emi_mem_write(
					prAdapter->chip_info,
					gEmiCalOffset,
					gEmiCalResult,
					gEmiCalSize);
			}
#endif

			u4Status = WLAN_STATUS_SUCCESS;
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
#define MAX_NVRAM_FEM_MAX 1024

	/* ePA /eLNA */
	uint8_t u1TypeID, u1LenLSB, u1LenMSB;
	uint8_t cnt1, u1NeedAdd;
	uint8_t u1Total_Size_LSB, u1Total_Size_MSB;
	uint32_t u4Tag_len = 0;
	uint16_t u2NVRAM_Toal_Size = 0;
	uint32_t u4NvramOffset = 0;
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

	uint8_t au1NeedTags[] = {NVRAM_TAG_2G4_COMMON, NVRAM_TAG_5G_COMMON,
		NVRAM_TAG_6G_COMMON, NVRAM_TAG_SYSTEM};

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

		//check if the tag needs to be added in phy action
		u1NeedAdd = 0;
		//rule1 : predefined need tags
		for (cnt1 = 0; cnt1 < ARRAY_SIZE(au1NeedTags); ++cnt1)
			if (u1TypeID == au1NeedTags[cnt1]) {
				u1NeedAdd = 1;
				break;
			}


		// rule2 : if tag id with valid bit , add in phy action
		if (u1TypeID & 0x80)
			u1NeedAdd = 1;

		if (u1NeedAdd) {
			u4Tag_len = sizeof(struct WIFI_NVRAM_TAG_FORMAT);
			u4Tag_len += (u1LenMSB << 8) | (u1LenLSB);
			kalMemCopy(&g_aucNvram_OnlyPreCal[*pu4DataLen],
			 &g_aucNvram[u4NvramOffset], u4Tag_len);
			*pu4DataLen += u4Tag_len;
			DBGLOG(INIT, TRACE,
			"Add tag(%d),len(%d),Offset(%d),PhyActLen(%d)\n",
			u1TypeID, u4Tag_len, u4NvramOffset, *pu4DataLen);
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
		DBGLOG(INIT, TRACE,
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
		DBGLOG(INIT, TRACE,
			"HAL_PHY_ACTION_TAG_NVRAM status[0x%x]\n",
			prPhyEvent->ucStatus);

		u4Status = WLAN_STATUS_SUCCESS;
	} else if (prPhyTlv->u2Tag == HAL_PHY_ACTION_TAG_COM_FEM) {
		DBGLOG(INIT, TRACE,
			"HAL_PHY_ACTION_TAG_COM_FEM status[0x%x]\n",
			prPhyEvent->ucStatus);

		u4Status = WLAN_STATUS_SUCCESS;
	}

exit:
	return u4Status;
}

#if ((CFG_SUPPORT_CONNFEM == 1) && (CONNFEM_API_VERSION >= 2))

uint8_t _AddConnfemSkuTag(struct ADAPTER *prAdapter,
	uint8_t *au1TagBuf,
	uint32_t *pu4TagLen)
{
	uint32_t u4TagLen = 0, u4TagLenBk;
	const struct connfem_sku *pSku = NULL;
	uint8_t cnt1, cnt2, cnt3;
	uint8_t u1SpdtInfo = 0;

	struct PHYACT_CONN_FEM_HEADER_T *pPlvHeader;
	struct PHYACT_FEM_INFOS_T *pFemInfos;
	struct PHYACT_LAYOUT_USAGES_T *pLayoutUsage;
	struct PHYACT_STATE_USAGES_T *pStateUsage;
	struct PHYACT_CONN_FEM_SPDT_V2_T *pSpdt;
	struct PHYACT_STATE_USAGE_CAT_T *pUsageCat;

	if ((pu4TagLen == NULL) || (au1TagBuf == NULL)) {
		DBGLOG(INIT, ERROR, "_AddConnfemTag fail");
		return 1;
	}

	/* init */
	*pu4TagLen = 0;

	/*get connfem sku data*/
	connfem_sku_data(&pSku);

	if (pSku == NULL) {
		DBGLOG(INIT, ERROR, "sku null");
		return 1;
	}

	/* header */
	pPlvHeader = (struct PHYACT_CONN_FEM_HEADER_T *) &au1TagBuf[u4TagLen];
	pPlvHeader->u1Tag = ENUM_INICMD_CONN_FEM_HEADER_FEM;
	pPlvHeader->u1Version = 2;
	//pPlvHeader->u2Len = ;
	u4TagLen += sizeof(struct PHYACT_CONN_FEM_HEADER_T);
	u4TagLenBk = u4TagLen;

	/****** FEM INFO **********/
	/* layout id */
	au1TagBuf[u4TagLen] = 0;
	au1TagBuf[u4TagLen] = 0;
	au1TagBuf[u4TagLen] = 0;
	au1TagBuf[u4TagLen] = 0;
	u4TagLen += 4;

	/* fem count */
	pFemInfos = (struct PHYACT_FEM_INFOS_T *) &au1TagBuf[u4TagLen];
	pFemInfos->u1FemCount = pSku->fem_count;
	u4TagLen += sizeof(uint32_t);

	for (cnt1 = 0 ; cnt1 < pFemInfos->u1FemCount ; cnt1++) {
		/* vid/pid */
		pFemInfos->rFem[cnt1].u4FemId =
			(pSku->fem[cnt1].info.vid << 16) |
			pSku->fem[cnt1].info.pid;
		pFemInfos->rFem[cnt1].u4Flag = pSku->fem[cnt1].info.flag;

		/* pin */
		pFemInfos->rFem[cnt1].u1PinNum = pSku->fem[cnt1].ctrl_pin.count;
		for (cnt2 = 0; cnt2 < pFemInfos->rFem[cnt1].u1PinNum; cnt2++) {
			pFemInfos->rFem[cnt1].u1PinName[cnt2] =
				pSku->fem[cnt1].ctrl_pin.id[cnt2];
		}

		/* state */
		pFemInfos->rFem[cnt1].u1StateNum =
			pSku->fem[cnt1].tt.logic_count;
		for (cnt2 = 0; cnt2 < pFemInfos->rFem[cnt1].u1StateNum;
			cnt2++) {
			pFemInfos->rFem[cnt1].rState[cnt2].u1StateName =
				pSku->fem[cnt1].tt.logic[cnt2].op;
			pFemInfos->rFem[cnt1].rState[cnt2].u1TruthTable =
				pSku->fem[cnt1].tt.logic[cnt2].binary;
		}

	}
	u4TagLen += sizeof(struct PHYACT_FEM_INFO_T) * pFemInfos->u1FemCount;

	/****** LAYOUT USAGE **********/
	pLayoutUsage = (struct PHYACT_LAYOUT_USAGES_T *) &au1TagBuf[u4TagLen];
	pLayoutUsage->u1LayoutUsageCount = pSku->layout_count;
	pLayoutUsage->u4Flag = pSku->layout_flag;
	u4TagLen += sizeof(uint32_t) * 2;

	for (cnt1 = 0; cnt1 < pLayoutUsage->u1LayoutUsageCount; cnt1++) {
		pLayoutUsage->rLayout[cnt1].u1BandPathWf =
			pSku->layout[cnt1].bandpath[1];
		pLayoutUsage->rLayout[cnt1].u1BandPathBt =
			pSku->layout[cnt1].bandpath[2];
		pLayoutUsage->rLayout[cnt1].u1FemIdx =
			pSku->layout[cnt1].fem_idx;
		pLayoutUsage->rLayout[cnt1].u1PinNum =
			pSku->layout[cnt1].pin_count;

		for (cnt2 = 0; cnt2 < pLayoutUsage->rLayout[cnt1].u1PinNum;
			cnt2++) {
			pLayoutUsage->rLayout[cnt1].rPin[cnt2].u1PinName =
				pSku->layout[cnt1].pinmap[cnt2].pin2;
			pLayoutUsage->rLayout[cnt1].rPin[cnt2].u1AntSelNo =
				pSku->layout[cnt1].pinmap[cnt2].pin1;
			pLayoutUsage->rLayout[cnt1].rPin[cnt2].u1Polarity =
				pSku->layout[cnt1].pinmap[cnt2].flag;
		}
	}
	u4TagLen += (sizeof(struct PHYACT_LAYOUT_USAGE_T) *
				pLayoutUsage->u1LayoutUsageCount);

	pPlvHeader->u2Len = u4TagLen - u4TagLenBk;

	/* header */
	pPlvHeader = (struct PHYACT_CONN_FEM_HEADER_T *) &au1TagBuf[u4TagLen];
	pPlvHeader->u1Tag = ENUM_INICMD_CONN_FEM_HEADER_WF;
	pPlvHeader->u1Version = 2;
	//pPlvHeader->u2Len = ;
	u4TagLen += sizeof(struct PHYACT_CONN_FEM_HEADER_T);
	u4TagLenBk = u4TagLen;

	/****** STATE USAGE **********/
	pStateUsage = (struct PHYACT_STATE_USAGES_T *) &au1TagBuf[u4TagLen];
	pStateUsage->u1FemCount = pSku->fem_count;
	u4TagLen += sizeof(uint32_t);

	for (cnt1 = 0 ; cnt1 < pStateUsage->u1FemCount ; cnt1++) {
		/* vid/pid */
		pStateUsage->rFem[cnt1].u1FemIdx = cnt1;
		pStateUsage->rFem[cnt1].u1StateCatNum =
			pSku->fem[cnt1].tt_usage_wf.cat_count;
		for (cnt2 = 0; cnt2 < pStateUsage->rFem[cnt1].u1StateCatNum;
			cnt2++) {

			pUsageCat =
				&pStateUsage->rFem[cnt1].u1StateCat[cnt2];
			pUsageCat->u1Cat =
				pSku->fem[cnt1].tt_usage_wf.cat[cnt2].id;
			pUsageCat->u1StateUsedNum =
				pSku->fem[cnt1].tt_usage_wf.cat[cnt2].op_count;

			for (cnt3 = 0;
			    cnt3 < pUsageCat->u1StateUsedNum;
				cnt3++) {
				pUsageCat->u1StateUsedName[cnt3] =
				pSku->fem[cnt1].tt_usage_wf.cat[cnt2].op[cnt3];
			}
		}
	}

	u4TagLen += sizeof(struct PHYACT_STATE_USAGE_T) *
		pStateUsage->u1FemCount;

	pPlvHeader->u2Len = u4TagLen - u4TagLenBk;

	/* header */
	pPlvHeader = (struct PHYACT_CONN_FEM_HEADER_T *) &au1TagBuf[u4TagLen];
	pPlvHeader->u1Tag = ENUM_INICMD_CONN_FEM_HEADER_SPDT;
	pPlvHeader->u1Version = 2;
	//pPlvHeader->u2Len = ;
	u4TagLenBk = u4TagLen + sizeof(struct PHYACT_CONN_FEM_HEADER_T);
	pSpdt = (struct PHYACT_CONN_FEM_SPDT_V2_T *) &au1TagBuf[u4TagLen];

	/****** SPDT **********/
	connfem_sku_flag_u8(CONNFEM_SUBSYS_NONE, "fe-ant-cnt",
		&u1SpdtInfo);
	pSpdt->u1SpdtInfo = u1SpdtInfo;

	connfem_sku_flag_u8(CONNFEM_SUBSYS_NONE, "fe-conn-dpdt-sp3t",
		&u1SpdtInfo);
	pSpdt->u1SpdtInfo2 = u1SpdtInfo;

	connfem_sku_flag_u8(CONNFEM_SUBSYS_NONE, "fe-conn-spdt",
		&u1SpdtInfo);
	pSpdt->u1SpdtInfo3 = u1SpdtInfo;

	connfem_sku_flag_u8(CONNFEM_SUBSYS_NONE, "fe-bt-wf-usage",
		&u1SpdtInfo);
	pSpdt->u1SpdtInfo4 = u1SpdtInfo;

	connfem_sku_flag_u8(CONNFEM_SUBSYS_NONE, "fe-conn-spdt-2",
		&u1SpdtInfo);
	pSpdt->u1SpdtInfo5 = u1SpdtInfo;

	pSpdt->u1PinNum = pSku->spdt.pin_count;
	for (cnt1 = 0 ; cnt1 < pSku->spdt.pin_count; cnt1++) {
		pSpdt->rPin[cnt1].u1AntSelNo = pSku->spdt.pinmap[cnt1].pin1;
		pSpdt->rPin[cnt1].u1FemPin = pSku->spdt.pinmap[cnt1].pin2;
		pSpdt->rPin[cnt1].u1Polarity = pSku->spdt.pinmap[cnt1].flag;
	}

	u4TagLen += sizeof(struct PHYACT_CONN_FEM_SPDT_V2_T);
	pPlvHeader->u2Len = u4TagLen - u4TagLenBk;

	DBGLOG(INIT, TRACE, "_AddConnfemTag , Len=%d", u4TagLen);
	DBGLOG_MEM8(INIT, TRACE, au1TagBuf, u4TagLen);

	*pu4TagLen = u4TagLen;
	return 0;
}


uint32_t wlanSendPhyActionV2(struct ADAPTER *prAdapter,
	uint16_t u2Tag,
	uint8_t ucCalCmd)
{

#define CMD_MAX_BUF_SIZE 2248

	struct HAL_PHY_ACTION_TLV_HEADER *prCmd = NULL;
	struct HAL_PHY_ACTION_TLV_HEADER *prEvent = NULL;
	struct HAL_PHY_ACTION_TLV *prPhyTlv;
	struct INIT_CMD_PHY_ACTION_CAL *prPhyCal;
	uint8_t *u1EpaELnaDataPointer = NULL;
	uint8_t u1EpaElnaDummyArray[10] = {0};
	uint32_t u4EpaELnaDataSize = 0;

	uint32_t u4CmdSize = 0, u4TagSize = 0;
	uint32_t u4EvtSize = 0, u4TmpSize;
	uint8_t  u1TagNum, cnt1;
	uint8_t *pu1TagBuf = NULL;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
	uint8_t  au1TagList[10];

	/* 1. Allocate CMD Info Packet and its Buffer. */
	prCmd = kalMemAlloc(CMD_MAX_BUF_SIZE, VIR_MEM_TYPE);
	if (!prCmd) {
		DBGLOG(INIT, ERROR, "Alloc cmd packet failed");
		u4Status = WLAN_STATUS_FAILURE;
		goto exit;
	}

	/* Allocate Tag Buffer  */
	pu1TagBuf = kalMemAlloc(CMD_MAX_BUF_SIZE, VIR_MEM_TYPE);
	if (!pu1TagBuf) {
		DBGLOG(INIT, ERROR, "Alloc pu1TagBuf failed");
		u4Status = WLAN_STATUS_FAILURE;
		goto exit;
	}

	u4EvtSize = sizeof(struct HAL_PHY_ACTION_TLV_HEADER) +
			sizeof(struct HAL_PHY_ACTION_TLV) +
			sizeof(struct INIT_EVENT_PHY_ACTION_RSP);
	prEvent = kalMemAlloc(u4EvtSize, VIR_MEM_TYPE);
	if (!prEvent) {
		DBGLOG(INIT, ERROR, "Alloc event packet failed");
		u4Status = WLAN_STATUS_FAILURE;
		goto exit;
	}

	/* 2. TLV handle */
	if (u2Tag == HAL_PHY_ACTION_TAG_COM_FEM) {
		u1TagNum = 1;
		au1TagList[0] = PC_TAG_ID_CONNFEM;
	} else if (u2Tag == HAL_PHY_ACTION_TAG_CAL) {
		u1TagNum = 2;
		au1TagList[0] = PC_TAG_ID_NVRAM;
		au1TagList[1] = PC_TAG_ID_CAL;
	} else {
		DBGLOG(INIT, DEBUG, "unknown tag");
		goto exit;
	}

	/* Process TLV Header Part1 */
	prCmd->u4MagicNum = HAL_PHY_ACTION_MAGIC_NUM;
	prCmd->ucVersion = HAL_PHY_ACTION_VERSION;
	prCmd->ucTagNums = u1TagNum;

	u4CmdSize = sizeof(struct HAL_PHY_ACTION_TLV_HEADER);

	for (cnt1 = 0; cnt1 < u1TagNum; cnt1++) {

		u4TmpSize = u4CmdSize -
			sizeof(struct HAL_PHY_ACTION_TLV_HEADER);
		prPhyTlv = (struct HAL_PHY_ACTION_TLV *)
			&prCmd->aucBuffer[u4TmpSize];

		switch (au1TagList[cnt1]) {
		case PC_TAG_ID_CONNFEM:
			_AddConnfemSkuTag(prAdapter, pu1TagBuf, &u4TagSize);

			//u4TagSize = u4EpaELnaDataSize;

			prPhyTlv->u2Tag = HAL_PHY_ACTION_TAG_COM_FEM;
			prPhyTlv->u2BufLength = PALIGN_4(u4TagSize);
			kalMemCopy(prPhyTlv->aucBuffer,
				pu1TagBuf, u4TagSize);

			break;
		case PC_TAG_ID_NVRAM:
			wlanGetEpaElnaFromNvram(&u1EpaELnaDataPointer,
				&u4EpaELnaDataSize);
			if (u1EpaELnaDataPointer == NULL) {
				DBGLOG(INIT, WARN, "Get pointer failed");

				u1EpaELnaDataPointer = u1EpaElnaDummyArray;
				u4EpaELnaDataSize = 0;
			}
			u4TagSize = u4EpaELnaDataSize;

			prPhyTlv->u2Tag = HAL_PHY_ACTION_TAG_NVRAM;
			prPhyTlv->u2BufLength = PALIGN_4(u4EpaELnaDataSize);
			kalMemCopy(prPhyTlv->aucBuffer,
				u1EpaELnaDataPointer, u4EpaELnaDataSize);

			break;
		case PC_TAG_ID_CAL:

			prPhyTlv->u2Tag = HAL_PHY_ACTION_TAG_CAL;
			prPhyTlv->u2BufLength =
				sizeof(struct INIT_CMD_PHY_ACTION_CAL);
			prPhyCal =
			(struct INIT_CMD_PHY_ACTION_CAL *)prPhyTlv->aucBuffer;
			prPhyCal->ucCmd = ucCalCmd;
#if CFG_MTK_ANDROID_WMT
			prPhyCal->ucCalSaveResult = 1;
#else
			prPhyCal->ucCalSaveResult = 0;
#endif
			prPhyCal->ucSkipCal = g_fgCalDisabled;

			break;
		default:
			break;
		}

		u4CmdSize += sizeof(struct HAL_PHY_ACTION_TLV) + u4TagSize;

		DBGLOG(INIT, TRACE, "Tag=%d, Len=%d",
			au1TagList[cnt1], u4TagSize);
		DBGLOG_MEM8(INIT, TRACE, (uint8_t *)prPhyTlv, u4TagSize);
	}

	/* 1. Allocate CMD Info Packet and its Buffer. */
	/* u2Tag  */
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

	if (pu1TagBuf)
		kalMemFree(pu1TagBuf, VIR_MEM_TYPE, u4EvtSize);

	return u4Status;
}

#endif /* #if(CONNFEM_API_VERSION >= 2) */

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
	struct connfem_epaelna_flags_common flags_common = {0};
#endif
#endif
	uint8_t *u1EpaELnaDataPointer = NULL;
	uint8_t u1EpaElnaDummyArray[10] = {0};
	uint32_t u4EpaELnaDataSize = 0, u4CmdSize = 0, u4EvtSize = 0;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;

	DBGLOG(INIT, TRACE, "SendPhyAction begin, tag: %d, cmd: %d, skip: %d\n",
		u2Tag, ucCalCmd, g_fgCalDisabled);

	ASSERT(prAdapter);

	wlanGetEpaElnaFromNvram(&u1EpaELnaDataPointer,
		&u4EpaELnaDataSize);

	if (u1EpaELnaDataPointer == NULL) {
		DBGLOG(INIT, ERROR, "Get u1EpaELnaDataPointer failed\n");
#if (CFG_MTK_ANDROID_WMT == 1)
		KAL_WARN_ON(TRUE);
#endif
		u1EpaELnaDataPointer = u1EpaElnaDummyArray;
		u4EpaELnaDataSize = 0;
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

	DBGLOG(INIT, TRACE, "fgPreCal = %d\n", g_fgPreCal);

	if (g_fgPreCal == FALSE) {
		/* Setup calibration data from backup file */
#if (CFG_SUPPORT_CONNFEM == 1)
#if (CONNFEM_API_VERSION >= 2)
		if (connfem_is_available(CONNFEM_TYPE_SKU)) {
			DBGLOG(INIT, TRACE, "connfem sku support");

			wlanSendPhyActionV2(prAdapter,
				HAL_PHY_ACTION_TAG_COM_FEM,
				0);
		} else
			wlanSendPhyAction(prAdapter,
			HAL_PHY_ACTION_TAG_COM_FEM,
			0);
#endif /* #if(CONNFEM_API_VERSION >= 2) */
#endif /*#if (CFG_SUPPORT_CONNFEM == 1) */

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

#if (CONNFEM_API_VERSION >= 2)
		if (connfem_is_available(CONNFEM_TYPE_SKU)) {
			DBGLOG(INIT, TRACE, "connfem sku support");

			wlanSendPhyActionV2(prAdapter,
				HAL_PHY_ACTION_TAG_COM_FEM,
				0);
		} else
			wlanSendPhyAction(prAdapter,
			HAL_PHY_ACTION_TAG_COM_FEM,
			0);
#else /* #if(CONNFEM_API_VERSION >= 2) */
		wlanSendPhyAction(prAdapter,
			HAL_PHY_ACTION_TAG_COM_FEM,
			0);
#endif /* #if(CONNFEM_API_VERSION >= 2) */

#else /* #if (CFG_SUPPORT_CONNFEM == 1) */
		wlanSendPhyAction(prAdapter,
			HAL_PHY_ACTION_TAG_NVRAM,
			0);
#endif /* #if (CFG_SUPPORT_CONNFEM == 1) */
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

	DBGLOG(INIT, DEBUG, "EMI_GET_CAL emiAddr[0x%x]emiLen[%d]\n",
		*pEmiCalOffset,
		*pEmiCalSize);

	return WLAN_STATUS_SUCCESS;
}

int wlan_precal_pwron_v1(void)
{
	DBGLOG(INIT, DEBUG, "ever = %d\n", g_fgEverCal);

	if (g_fgEverCal == TRUE)
		return 1;

#if CFG_MTK_ANDROID_EMI
	/* CONNAC 2 use backup /restore EMI */
	gEmiCalNoUseEmiData = FALSE;
#endif

	wfsys_lock();

	return 0;
}

int wlan_precal_docal_v1(void)
{
	int32_t ret = 0;

	DBGLOG(INIT, DEBUG, "ever = %d\n", g_fgEverCal);

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
		wlan_precal_done_notify();

		wfsys_unlock();
	}
	return ret;
}

int wlan_precal_pwron_v2(void)
{

#ifdef MT6639

	int32_t ret = 0;

	DBGLOG(INIT, DEBUG, "\n");

#if CFG_MTK_ANDROID_EMI
	// CONNAC 3 , no use backup /restore EMI
	// (FW no use CRC , can't change region)
	gEmiCalNoUseEmiData = TRUE;
#endif

	if (!wfsys_is_locked())
		wfsys_lock();
	else
		wfsys_lock_extend();

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

#else /* #ifdef MT6639 */

	DBGLOG(INIT, DEBUG, "ever = %d\n", g_fgEverCal);

	if (g_fgEverCal == TRUE)
		return 1;

#if CFG_MTK_ANDROID_EMI
	/* CONNAC 2 use backup /restore EMI */
	gEmiCalNoUseEmiData = FALSE;
#endif

	if (!wfsys_is_locked())
		wfsys_lock();
	else
		wfsys_lock_extend();

	return 0;

#endif /* #ifdef MT6639 */

}

int wlan_precal_docal_v2(void)
{

#ifdef MT6639

	DBGLOG(INIT, DEBUG, "\n");

	if (!g_fgEverCal) {
		g_fgEverCal = TRUE;
	}

	if (wfsys_is_locked())
		wfsys_unlock();

	return 0;

#else /* #ifdef MT6639 */

	int32_t ret = 0;

	DBGLOG(INIT, DEBUG, "ever = %d\n", g_fgEverCal);

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

		if (wfsys_is_locked())
			wfsys_unlock();
	}
	return ret;

#endif /* #ifdef MT6639 */
}

int wlan_precal_err(void)
{
	DBGLOG(INIT, DEBUG, "\n");

	if (!g_fgEverCal) {
		g_fgEverCal = TRUE;
		wlan_precal_done_notify();
	}

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
#if (CFG_WLAN_CONNAC3_DEV == 1)
	return TRUE;
#else
	return g_fgEverCal;
#endif
}

void wlan_precal_done_notify(void)
{
	DBGLOG(RFTEST, DEBUG, "wlan precal done\n");

#if CFG_TESTMODE_WMT_WIFI_ON_SUPPORT
	/* prevent turn on wifi by wmt driver before precal finished */
	/* so we register cb function after precal done */
	register_set_wifi_test_mode_fwdl_handler(set_wifi_test_mode_fwdl);
#endif

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

	DBGLOG(RFTEST, DEBUG,
		"gEmiCalOffset(0x%x), gEmiCalSize(0x%x), gEmiCalNoUseEmiData(%d)\n",
		gEmiCalOffset, gEmiCalSize, gEmiCalNoUseEmiData);
#endif
}
