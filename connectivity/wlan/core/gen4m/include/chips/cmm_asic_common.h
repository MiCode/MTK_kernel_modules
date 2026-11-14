/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file  cmm_connac_common.h
 *    \brief This file contains the info of CONNAC
 */

#ifndef _CMM_ASIC_COMMON_H
#define _CMM_ASIC_COMMON_H

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#if (CFG_TESTMODE_FWDL_SUPPORT == 1)
extern u_int8_t g_fgWlanOnOffHoldRtnlLock;
#endif

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
uint32_t asicGetFwDlInfo(struct ADAPTER *prAdapter,
	char *pcBuf, int i4TotalLen);
uint32_t asicGetChipID(struct ADAPTER *prAdapter);
void fillNicTxDescAppend(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo,
	uint8_t *prTxDescBuffer);
void fillTxDescAppendByHostV2(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo, uint16_t u4MsduId,
	dma_addr_t rDmaAddr, uint32_t u4Idx, u_int8_t fgIsLast,
	uint8_t *pucBuffer);
void halDumpTxdInfo(struct ADAPTER *prAdapter, uint8_t *tmac_info);
void asicWakeUpWiFi(struct ADAPTER *prAdapter);

int connsys_power_on(void);
int connsys_power_done(void);
void connsys_power_off(void);

#if CFG_MTK_ANDROID_WMT
#if !CFG_SUPPORT_CONNAC1X
void unregister_chrdev_cbs(void);
void register_chrdev_cbs(void);
#endif
void unregister_plat_connsys_cbs(void);
void register_plat_connsys_cbs(void);
#endif

int wlan_test_mode_on(bool uIsSwtichTestMode);

#endif /* _CMM_ASIC_COMMON_H */

