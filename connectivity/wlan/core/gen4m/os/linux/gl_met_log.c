// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "precomp.h"

#ifdef CFG_MTK_CONNSYS_DEDICATED_LOG_PATH
#if (CFG_SUPPORT_CONNINFRA == 1)
#include "connsys_debug_utility.h"
#include "metlog.h"
#endif
#endif

#if CFG_MTK_WIFI_MET_LOG_EMI
#include "met_log_emi.h"
#endif

#if CFG_SUPPORT_MET_LOG
/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define MET_LOG_TAG		"MCU_MET_DATA"

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
void met_log_print_data(uint8_t *buffer, uint32_t size,
	uint32_t module_id, uint32_t project_id, uint32_t chip_id)
{
	uint32_t *pu4StartAddr = (uint32_t *) buffer;
	uint8_t *pucAddr;
	uint32_t u4Length = size;

	DBGLOG(MET, DEBUG,
		"DUMP ADDRESS: 0x%p, Length: %d\n",
		pu4StartAddr, u4Length);

	while (u4Length > 0) {
		if (u4Length >= 8) {
			DBGLOG(MET, DEBUG,
				"%s:0%d%d%04X0000,%08x%08x\n",
				MET_LOG_TAG, module_id,
				project_id, chip_id,
				pu4StartAddr[1], pu4StartAddr[0]);
			pu4StartAddr += 2;
			u4Length -= 8;
		} else {
			switch (u4Length) {
			case 1:
				pucAddr = (uint8_t *) &pu4StartAddr[0];
				DBGLOG(MET, DEBUG,
					"%s:0%d%d%04X0000,%02x\n",
					MET_LOG_TAG, module_id,
					project_id, chip_id,
					pucAddr[0]);
				break;
			case 2:
				pucAddr = (uint8_t *) &pu4StartAddr[0];
				DBGLOG(MET, DEBUG,
					"%s:0%d%d%04X0000,%02x%02x\n",
					MET_LOG_TAG, module_id,
					project_id, chip_id,
					pucAddr[1], pucAddr[0]);
				break;
			case 3:
				pucAddr = (uint8_t *) &pu4StartAddr[0];
				DBGLOG(MET, DEBUG,
					"%s:0%d%d%04X0000,%02x%02x%02x\n",
					MET_LOG_TAG, module_id,
					project_id, chip_id,
					pucAddr[2], pucAddr[1], pucAddr[0]);
				break;
			case 4:
				DBGLOG(MET, DEBUG,
					"%s:0%d%d%04X0000,%08x\n",
					MET_LOG_TAG, module_id,
					project_id, chip_id,
					pu4StartAddr[0]);
				break;
			case 5:
				pucAddr = (uint8_t *) &pu4StartAddr[1];
				DBGLOG(MET, DEBUG,
					"%s:0%d%d%04X0000,%02x%08x\n",
					MET_LOG_TAG, module_id,
					project_id, chip_id,
					pucAddr[0], pu4StartAddr[0]);
				break;
			case 6:
				pucAddr = (uint8_t *) &pu4StartAddr[1];
				DBGLOG(MET, DEBUG,
					"%s:0%d%d%04X0000,%02x%02x%08x\n",
					MET_LOG_TAG, module_id,
					project_id, chip_id,
					pucAddr[1], pucAddr[0],
					pu4StartAddr[0]);
				break;
			case 7:
				pucAddr = (uint8_t *) &pu4StartAddr[1];
				DBGLOG(MET, DEBUG,
					"%s:0%d%d%04X0000,%02x%02x%02x%08x\n",
					MET_LOG_TAG, module_id,
					project_id, chip_id,
					pucAddr[2], pucAddr[1], pucAddr[0],
					pu4StartAddr[0]);
				break;
			}
			u4Length = 0;
		}
	}
}

void met_log_print_long_data(uint8_t *buffer, uint32_t size,
	uint32_t module_id, uint32_t project_id, uint32_t chip_id)
{
	uint32_t *pu4StartAddr = (uint32_t *) buffer;
	uint8_t *pucAddr;
	uint32_t u4Length = size;

	DBGLOG(MET, DEBUG,
		"DUMP ADDRESS: 0x%p, Length: %d\n",
		pu4StartAddr, u4Length);

	while (u4Length > 0) {
		if (u4Length >= 16) {
			DBGLOG(MET, DEBUG,
				"%s:0%d%d%04X0000,%08x%08x%08x%08x\n",
				MET_LOG_TAG, module_id,
				project_id, chip_id,
				pu4StartAddr[3], pu4StartAddr[2],
				pu4StartAddr[1], pu4StartAddr[0]);
			pu4StartAddr += 4;
			u4Length -= 16;
		} else {
			switch (u4Length) {
			case 1:
				pucAddr = (uint8_t *) &pu4StartAddr[0];
				DBGLOG(MET, DEBUG,
					"%s:0%d%d%04X0000,%02x\n",
					MET_LOG_TAG, module_id,
					project_id, chip_id,
					pucAddr[0]);
				break;
			case 2:
				pucAddr = (uint8_t *) &pu4StartAddr[0];
				DBGLOG(MET, DEBUG,
					"%s:0%d%d%04X0000,%02x%02x\n",
					MET_LOG_TAG, module_id,
					project_id, chip_id,
					pucAddr[1], pucAddr[0]);
				break;
			case 3:
				pucAddr = (uint8_t *) &pu4StartAddr[0];
				DBGLOG(MET, DEBUG,
					"%s:0%d%d%04X0000,%02x%02x%02x\n",
					MET_LOG_TAG, module_id,
					project_id, chip_id,
					pucAddr[2], pucAddr[1], pucAddr[0]);
				break;
			case 4:
				DBGLOG(MET, DEBUG,
					"%s:0%d%d%04X0000,%08x\n",
					MET_LOG_TAG, module_id,
					project_id, chip_id,
					pu4StartAddr[0]);
				break;
			case 5:
				pucAddr = (uint8_t *) &pu4StartAddr[1];
				DBGLOG(MET, DEBUG,
					"%s:0%d%d%04X0000,%02x%08x\n",
					MET_LOG_TAG, module_id,
					project_id, chip_id,
					pucAddr[0], pu4StartAddr[0]);
				break;
			case 6:
				pucAddr = (uint8_t *) &pu4StartAddr[1];
				DBGLOG(MET, DEBUG,
					"%s:0%d%d%04X0000,%02x%02x%08x\n",
					MET_LOG_TAG, module_id,
					project_id, chip_id,
					pucAddr[1], pucAddr[0],
					pu4StartAddr[0]);
				break;
			case 7:
				pucAddr = (uint8_t *) &pu4StartAddr[1];
				DBGLOG(MET, DEBUG,
					"%s:0%d%d%04X0000,%02x%02x%02x%08x\n",
					MET_LOG_TAG, module_id,
					project_id, chip_id,
					pucAddr[2], pucAddr[1], pucAddr[0],
					pu4StartAddr[0]);
				break;
			case 8:
				DBGLOG(MET, DEBUG,
					"%s:0%d%d%04X0000,%08x%08x\n",
					MET_LOG_TAG, module_id,
					project_id, chip_id,
					pu4StartAddr[1], pu4StartAddr[0]);
				break;
			case 9:
				pucAddr = (uint8_t *) &pu4StartAddr[2];
				DBGLOG(MET, DEBUG,
					"%s:0%d%d%04X0000,%02x%08x%08x\n",
					MET_LOG_TAG, module_id,
					project_id, chip_id,
					pucAddr[0],
					pu4StartAddr[1], pu4StartAddr[0]);
				break;
			case 10:
				pucAddr = (uint8_t *) &pu4StartAddr[2];
				DBGLOG(MET, DEBUG,
					"%s:0%d%d%04X0000,%02x%02x%08x%08x\n",
					MET_LOG_TAG, module_id,
					project_id, chip_id,
					pucAddr[1], pucAddr[0],
					pu4StartAddr[1], pu4StartAddr[0]);
				break;
			case 11:
				pucAddr = (uint8_t *) &pu4StartAddr[2];
				DBGLOG(MET, DEBUG,
					"%s:0%d%d%04X0000,%02x%02x%02x%08x%08x\n",
					MET_LOG_TAG, module_id,
					project_id, chip_id,
					pucAddr[2], pucAddr[1], pucAddr[0],
					pu4StartAddr[1], pu4StartAddr[0]);
				break;
			case 12:
				DBGLOG(MET, DEBUG,
					"%s:0%d%d%04X0000,%08x%08x%08x\n",
					MET_LOG_TAG, module_id,
					project_id, chip_id,
					pu4StartAddr[2],
					pu4StartAddr[1], pu4StartAddr[0]);
				break;
			case 13:
				pucAddr = (uint8_t *) &pu4StartAddr[3];
				DBGLOG(MET, DEBUG,
					"%s:0%d%d%04X0000,%02x%08x%08x%08x\n",
					MET_LOG_TAG, module_id,
					project_id, chip_id,
					pucAddr[0], pu4StartAddr[2],
					pu4StartAddr[1], pu4StartAddr[0]);
				break;
			case 14:
				pucAddr = (uint8_t *) &pu4StartAddr[3];
				DBGLOG(MET, DEBUG,
					"%s:0%d%d%04X0000,%02x%02x%08x%08x%08x\n",
					MET_LOG_TAG, module_id,
					project_id, chip_id,
					pucAddr[1], pucAddr[0], pu4StartAddr[2],
					pu4StartAddr[1], pu4StartAddr[0]);
				break;
			case 15:
				pucAddr = (uint8_t *) &pu4StartAddr[3];
				DBGLOG(MET, DEBUG,
					"%s:0%d%d%04X0000,%02x%02x%02x%08x%08x%08x\n",
					MET_LOG_TAG, module_id,
					project_id, chip_id,
					pucAddr[2], pucAddr[1],
					pucAddr[0], pu4StartAddr[2],
					pu4StartAddr[1], pu4StartAddr[0]);
				break;
			}
			u4Length = 0;
		}
	}
}

int met_log_start(struct GLUE_INFO *prGlueInfo)
{
#if defined(CFG_MTK_CONNSYS_DEDICATED_LOG_PATH) && (CFG_SUPPORT_CONNINFRA == 1)
	struct conn_metlog_info rMetInfo;
	phys_addr_t u4ConEmiPhyBase = 0;
	uint32_t u4EmiMetOffset = 0;
#endif

	DBGLOG(MET, DEBUG, "Start MET log.\n");

#if CFG_MTK_WIFI_MET_LOG_EMI
	return met_log_emi_init(prGlueInfo->prAdapter);
#else
#if defined(CFG_MTK_CONNSYS_DEDICATED_LOG_PATH) && (CFG_SUPPORT_CONNINFRA == 1)
	u4ConEmiPhyBase = emi_mem_get_phy_base(
				prGlueInfo->prAdapter->chip_info);
	u4EmiMetOffset = emi_mem_offset_convert(
				kalGetEmiMetOffset());

	DBGLOG(MET, DEBUG, "u4ConEmiPhyBase:%pa", &u4ConEmiPhyBase);
	if (!u4ConEmiPhyBase) {
		DBGLOG(MET, ERROR, "conninfra_get_phy_addr error.\n");
		return -1;
	}

	rMetInfo.type = CONNDRV_TYPE_WIFI;
	rMetInfo.read_cr = u4ConEmiPhyBase + u4EmiMetOffset;
	rMetInfo.write_cr = u4ConEmiPhyBase + u4EmiMetOffset + 0x4;
	rMetInfo.met_base_ap = u4ConEmiPhyBase + u4EmiMetOffset + 0x8;
	rMetInfo.met_base_fw = 0xF0000000 + u4EmiMetOffset + 0x8;
	rMetInfo.met_size = 0x8000 - 0x8;
	rMetInfo.output_len = 64;

	return conn_metlog_start(&rMetInfo);
#else
	return 0;
#endif
#endif
}

int met_log_stop(struct GLUE_INFO *prGlueInfo)
{
	DBGLOG(MET, TRACE, "Stop MET log.\n");

#if CFG_MTK_WIFI_MET_LOG_EMI
	return met_log_emi_deinit(prGlueInfo->prAdapter);
#else
#if defined(CFG_MTK_CONNSYS_DEDICATED_LOG_PATH) && (CFG_SUPPORT_CONNINFRA == 1)
	return conn_metlog_stop(CONNDRV_TYPE_WIFI);
#else
	return 0;
#endif
#endif
}
#endif /* CFG_SUPPORT_MET_LOG */
