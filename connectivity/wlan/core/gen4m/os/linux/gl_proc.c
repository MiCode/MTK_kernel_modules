// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "gl_proc.c"
 *  \brief  This file defines the interface which can interact with users
 *          in /proc fs.
 *
 *    Detail description.
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
#include "gl_os.h"
#include "gl_kal.h"
#include "debug.h"
#include "wlan_lib.h"
#include "debug.h"
#include "wlan_oid.h"
#include <linux/rtc.h>

#if (CFG_TWT_SMART_STA == 1)
#include "twt.h"
#endif

#if CFG_SUPPORT_CSI
#include "gl_csi.h"
#endif

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define PROC_MAX_BUF_SIZE        3000

#ifdef CFG_COMBO_SLT_GOLDEN
#define PROC_ROOT_NAME							"ra"
#else
#define PROC_ROOT_NAME							"wlan"
#endif

/* Only used by CE or in userdebug/eng load */
#if (!CFG_MTK_ANDROID_WMT) || (BUILD_QA_DBG)
#define PROC_DRIVER_CMD                         "driver"
#if CFG_SUPPORT_CSI
#define PROC_CSI_DATA_NAME			"csi_data"
#endif
#endif /* (!CFG_MTK_ANDROID_WMT) || (BUILD_QA_DBG) */

/* Only used by CE */
#if (!CFG_MTK_ANDROID_WMT)
#define PROC_COUNTRY				"country"
#if (CFG_CE_ASSERT_DUMP == 1)
#define PROC_CORE_DUMP				"core_dump"
#endif
#endif /* (!CFG_MTK_ANDROID_WMT) */

/* Only used by CE debug */
#if (!CFG_MTK_ANDROID_WMT) && (BUILD_QA_DBG)
#define PROC_EFUSE_DUMP                         "efuse_dump"
#define PROC_PKT_DELAY_DBG			"pktDelay"
#if CFG_WIFI_TXPWR_TBL_DUMP
#define PROC_GET_TXPWR_TBL                      "get_txpwr_tbl"
#endif
#endif /* (!CFG_MTK_ANDROID_WMT) && (BUILD_QA_DBG) */

/* Only used in userdebug/eng load */
#if (BUILD_QA_DBG)
#define PROC_CFG                                "cfg"
#define PROC_MCR_ACCESS                         "mcr"
#if (CFG_TWT_SMART_STA == 1)
#define PROC_TWT_SMART				"twt_smart_sta"
#endif
#endif /* (BUILD_QA_DBG) */

#if (BUILD_QA_DBG) || (ENABLE_DBG_LEVEL)
#define PROC_DBG_LEVEL_NAME                     "dbgLevel"
#endif

#define PROC_MCR_ACCESS_MAX_USER_INPUT_LEN      20
#define PROC_RX_STATISTICS_MAX_USER_INPUT_LEN   10
#define PROC_TX_STATISTICS_MAX_USER_INPUT_LEN   10
#define PROC_DBG_LEVEL_MAX_USER_INPUT_LEN       20
#define PROC_DBG_LEVEL_MAX_DISPLAY_STR_LEN      30
#define PROC_UID_SHELL							2000
#define PROC_GID_WIFI							1010

#if CFG_WIFI_TXPWR_TBL_DUMP
#define TMP_SZ (1024)
#define CDD_PWR_OFFSET (6)
#define TXPWR_DUMP_SZ (16384)
#endif

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */
/* notice: str only can be an array */
#define SNPRINTF(buf, size, arg)   {buf += \
		snprintf((char *)(buf), size, PRINTF_ARG arg); }

#if CFG_WIFI_TXPWR_TBL_DUMP
#define TXPWR_TABLE_ENTRY(_siso_mcs, _cdd_mcs, _mimo_mcs, _idx)	\
{								\
	.mcs[STREAM_SISO] = _siso_mcs,				\
	.mcs[STREAM_CDD] = _cdd_mcs,				\
	.mcs[STREAM_MIMO] = _mimo_mcs,				\
	.idx = (_idx),						\
}
#endif

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */
#if (CFG_TWT_SMART_STA == 1)
struct _TWT_SMART_STA_T g_TwtSmartStaCtrl;
#endif

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */
static struct GLUE_INFO *g_prGlueInfo_proc;
static struct proc_dir_entry *gprProcRoot;

#if (BUILD_QA_DBG) || (ENABLE_DBG_LEVEL)
static uint8_t aucDbModuleName[][PROC_DBG_LEVEL_MAX_DISPLAY_STR_LEN] = {
	"INIT", "HAL", "INTR", "REQ", "TX", "RX", "RFTEST", "EMU",
	"SW1", "SW2", "SW3", "SW4", "HEM", "AIS", "RLM", "MEM",
	"CNM", "RSN", "BSS", "SCN", "SAA", "AAA", "P2P", "QM",
	"SEC", "BOW", "WAPI", "ROAMING", "TDLS", "PF", "OID", "NIC"
};
#endif /* (BUILD_QA_DBG) || (ENABLE_DBG_LEVEL) */

#if BUILD_QA_DBG
static uint32_t u4McrOffset;
#endif /* (BUILD_QA_DBG) */

/* This u32 is only for DriverCmdRead/Write,
 * should not be used by other function
 */
static int32_t g_i4NextDriverReadLen;

#if (!CFG_MTK_ANDROID_WMT) && (BUILD_QA_DBG)
#if CFG_WIFI_TXPWR_TBL_DUMP
static struct txpwr_table_entry dsss[] = {
	TXPWR_TABLE_ENTRY("DSSS1", "", "", MODULATION_SYSTEM_CCK_1M),
	TXPWR_TABLE_ENTRY("DSSS2", "", "", MODULATION_SYSTEM_CCK_2M),
	TXPWR_TABLE_ENTRY("CCK5", "", "", MODULATION_SYSTEM_CCK_5M),
	TXPWR_TABLE_ENTRY("CCK11", "", "", MODULATION_SYSTEM_CCK_11M),
};

static struct txpwr_table_entry ofdm[] = {
	TXPWR_TABLE_ENTRY("OFDM6", "OFDM6", "", MODULATION_SYSTEM_OFDM_6M),
	TXPWR_TABLE_ENTRY("OFDM9", "OFDM9", "", MODULATION_SYSTEM_OFDM_9M),
	TXPWR_TABLE_ENTRY("OFDM12", "OFDM12", "", MODULATION_SYSTEM_OFDM_12M),
	TXPWR_TABLE_ENTRY("OFDM18", "OFDM18", "", MODULATION_SYSTEM_OFDM_18M),
	TXPWR_TABLE_ENTRY("OFDM24", "OFDM24", "", MODULATION_SYSTEM_OFDM_24M),
	TXPWR_TABLE_ENTRY("OFDM36", "OFDM36", "", MODULATION_SYSTEM_OFDM_36M),
	TXPWR_TABLE_ENTRY("OFDM48", "OFDM48", "", MODULATION_SYSTEM_OFDM_48M),
	TXPWR_TABLE_ENTRY("OFDM54", "OFDM54", "", MODULATION_SYSTEM_OFDM_54M),
};

static struct txpwr_table_entry ht20[] = {
	TXPWR_TABLE_ENTRY("MCS0", "MCS0", "MCS8", MODULATION_SYSTEM_HT20_MCS0),
	TXPWR_TABLE_ENTRY("MCS1", "MCS1", "MCS9", MODULATION_SYSTEM_HT20_MCS1),
	TXPWR_TABLE_ENTRY("MCS2", "MCS2", "MCS10", MODULATION_SYSTEM_HT20_MCS2),
	TXPWR_TABLE_ENTRY("MCS3", "MCS3", "MCS11", MODULATION_SYSTEM_HT20_MCS3),
	TXPWR_TABLE_ENTRY("MCS4", "MCS4", "MCS12", MODULATION_SYSTEM_HT20_MCS4),
	TXPWR_TABLE_ENTRY("MCS5", "MCS5", "MCS13", MODULATION_SYSTEM_HT20_MCS5),
	TXPWR_TABLE_ENTRY("MCS6", "MCS6", "MCS14", MODULATION_SYSTEM_HT20_MCS6),
	TXPWR_TABLE_ENTRY("MCS7", "MCS7", "MCS15", MODULATION_SYSTEM_HT20_MCS7),
};
static struct txpwr_table_entry ht40[] = {
	TXPWR_TABLE_ENTRY("MCS0", "MCS0", "MCS8", MODULATION_SYSTEM_HT40_MCS0),
	TXPWR_TABLE_ENTRY("MCS1", "MCS1", "MCS9", MODULATION_SYSTEM_HT40_MCS1),
	TXPWR_TABLE_ENTRY("MCS2", "MCS2", "MCS10", MODULATION_SYSTEM_HT40_MCS2),
	TXPWR_TABLE_ENTRY("MCS3", "MCS3", "MCS11", MODULATION_SYSTEM_HT40_MCS3),
	TXPWR_TABLE_ENTRY("MCS4", "MCS4", "MCS12", MODULATION_SYSTEM_HT40_MCS4),
	TXPWR_TABLE_ENTRY("MCS5", "MCS5", "MCS13", MODULATION_SYSTEM_HT40_MCS5),
	TXPWR_TABLE_ENTRY("MCS6", "MCS6", "MCS14", MODULATION_SYSTEM_HT40_MCS6),
	TXPWR_TABLE_ENTRY("MCS7", "MCS7", "MCS15", MODULATION_SYSTEM_HT40_MCS7),
	TXPWR_TABLE_ENTRY("MCS32", "MCS32", "MCS32",
		MODULATION_SYSTEM_HT40_MCS32),
};
static struct txpwr_table_entry vht[] = {
	TXPWR_TABLE_ENTRY("MCS0", "MCS0", "MCS0", MODULATION_SYSTEM_VHT20_MCS0),
	TXPWR_TABLE_ENTRY("MCS1", "MCS1", "MCS1", MODULATION_SYSTEM_VHT20_MCS1),
	TXPWR_TABLE_ENTRY("MCS2", "MCS2", "MCS2", MODULATION_SYSTEM_VHT20_MCS2),
	TXPWR_TABLE_ENTRY("MCS3", "MCS3", "MCS3", MODULATION_SYSTEM_VHT20_MCS3),
	TXPWR_TABLE_ENTRY("MCS4", "MCS4", "MCS4", MODULATION_SYSTEM_VHT20_MCS4),
	TXPWR_TABLE_ENTRY("MCS5", "MCS5", "MCS5", MODULATION_SYSTEM_VHT20_MCS5),
	TXPWR_TABLE_ENTRY("MCS6", "MCS6", "MCS6", MODULATION_SYSTEM_VHT20_MCS6),
	TXPWR_TABLE_ENTRY("MCS7", "MCS7", "MCS7", MODULATION_SYSTEM_VHT20_MCS7),
	TXPWR_TABLE_ENTRY("MCS8", "MCS8", "MCS8", MODULATION_SYSTEM_VHT20_MCS8),
	TXPWR_TABLE_ENTRY("MCS9", "MCS9", "MCS9", MODULATION_SYSTEM_VHT20_MCS9),
};

#if (CFG_WIFI_TXPWR_TBL_DUMP_HE == 1)
static struct txpwr_table_entry he[] = {
	TXPWR_TABLE_ENTRY("MCS0", "MCS0", "MCS0", MODULATION_SYSTEM_HE26_MCS0),
	TXPWR_TABLE_ENTRY("MCS1", "MCS1", "MCS1", MODULATION_SYSTEM_HE26_MCS1),
	TXPWR_TABLE_ENTRY("MCS2", "MCS2", "MCS2", MODULATION_SYSTEM_HE26_MCS2),
	TXPWR_TABLE_ENTRY("MCS3", "MCS3", "MCS3", MODULATION_SYSTEM_HE26_MCS3),
	TXPWR_TABLE_ENTRY("MCS4", "MCS4", "MCS4", MODULATION_SYSTEM_HE26_MCS4),
	TXPWR_TABLE_ENTRY("MCS5", "MCS5", "MCS5", MODULATION_SYSTEM_HE26_MCS5),
	TXPWR_TABLE_ENTRY("MCS6", "MCS6", "MCS6", MODULATION_SYSTEM_HE26_MCS6),
	TXPWR_TABLE_ENTRY("MCS7", "MCS7", "MCS7", MODULATION_SYSTEM_HE26_MCS7),
	TXPWR_TABLE_ENTRY("MCS8", "MCS8", "MCS8", MODULATION_SYSTEM_HE26_MCS8),
	TXPWR_TABLE_ENTRY("MCS9", "MCS9", "MCS9", MODULATION_SYSTEM_HE26_MCS9),
	TXPWR_TABLE_ENTRY("MCS10", "MCS10", "MCS10",
		MODULATION_SYSTEM_HE26_MCS10),
	TXPWR_TABLE_ENTRY("MCS11", "MCS11", "MCS11",
		MODULATION_SYSTEM_HE26_MCS11),
};
#endif

static struct txpwr_table txpwr_tables[] = {
	{"Legacy", dsss, ARRAY_SIZE(dsss)},
	{"11g", ofdm, ARRAY_SIZE(ofdm)},
	{"11a", ofdm, ARRAY_SIZE(ofdm)},
	{"HT20", ht20, ARRAY_SIZE(ht20)},
	{"HT40", ht40, ARRAY_SIZE(ht40)},
	{"VHT20", vht, ARRAY_SIZE(vht)},
	{"VHT40", vht, ARRAY_SIZE(vht)},
	{"VHT80", vht, ARRAY_SIZE(vht)},
	{"VHT160", vht, ARRAY_SIZE(vht)},
#if (CFG_WIFI_TXPWR_TBL_DUMP_HE == 1)
	{"HE26", he, ARRAY_SIZE(he)},
	{"HE52", he, ARRAY_SIZE(he)},
	{"HE106", he, ARRAY_SIZE(he)},
	{"HE242", he, ARRAY_SIZE(he)},
	{"HE484", he, ARRAY_SIZE(he)},
	{"HE996", he, ARRAY_SIZE(he)},
	{"HE996X2", he, ARRAY_SIZE(he)},
#endif
};
#endif
#endif /* (!CFG_MTK_ANDROID_WMT) && (BUILD_QA_DBG) */

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#if (!CFG_MTK_ANDROID_WMT) || (BUILD_QA_DBG)
#if WLAN_INCLUDE_PROC
#if CFG_SUPPORT_EASY_DEBUG
static ssize_t procDriverCmdWrite(struct file *file, const char __user *buffer,
	size_t count, loff_t *data)
{
	uint8_t *pucProcBuf = kalMemZAlloc(PROC_MAX_BUF_SIZE, VIR_MEM_TYPE);
	uint32_t u4CopySize = PROC_MAX_BUF_SIZE;
	struct GLUE_INFO *prGlueInfo = g_prGlueInfo_proc;
	int32_t i4Ret = 0;

	if (buffer == NULL || pucProcBuf == NULL || prGlueInfo == NULL) {
		i4Ret = 0;
		goto freeBuf;
	}

	u4CopySize = (count < u4CopySize) ? count : (u4CopySize - 1);

	if (copy_from_user(pucProcBuf, buffer, u4CopySize)) {
		DBGLOG(REQ, WARN, "error of copy from user\n");
		i4Ret = -EFAULT;
		goto freeBuf;
	}
	pucProcBuf[u4CopySize] = '\0';

	/* This proc driver command will call priv_driver_cmds, which is
	 * the same callback function of iwpriv driver command.
	 * Since "AP_START" command needs know rtnl is locked or not and
	 * iwpriv command has already hold rtnl_lock in kernel, the proc
	 * driver "AP_START" command needs to be modified to "PROC_AP_START"
	 * to distinguish whether the caller holds rtnl_lock.
	 */
	if (u4CopySize >= 8 && strnicmp(pucProcBuf, "AP_START", 8) == 0) {
		uint8_t *pucProcBufTmp = kalMemZAlloc(u4CopySize + 6,
			VIR_MEM_TYPE);

		if (pucProcBufTmp == NULL) {
			i4Ret = 0;
			goto freeBuf;
		}
		kalSnprintf(pucProcBufTmp, u4CopySize + 6, "PROC_%s",
			pucProcBuf);
		kalMemFree(pucProcBuf, VIR_MEM_TYPE, PROC_MAX_BUF_SIZE);
		pucProcBuf = pucProcBufTmp;
		DBGLOG(P2P, INFO, "Add prefix, command: %s\n", pucProcBuf);
	}

	if (kalStrLen(pucProcBuf) > 0)
		priv_driver_cmds(prGlueInfo->prDevHandler, pucProcBuf,
			kalStrLen(pucProcBuf));

	i4Ret = u4CopySize;
freeBuf:
	if (pucProcBuf)
		kalMemFree(pucProcBuf, VIR_MEM_TYPE, PROC_MAX_BUF_SIZE);
	return i4Ret;
}

static ssize_t procDriverCmdRead(struct file *filp, char __user *buf,
	size_t count, loff_t *f_pos)
{
	return 0;
}

#if KERNEL_VERSION(5, 6, 0) <= CFG80211_VERSION_CODE
static const struct proc_ops drivercmd_ops = {
	.proc_write = procDriverCmdWrite,
};
#else
static const struct file_operations drivercmd_ops = {
	.owner = THIS_MODULE,
	.write = procDriverCmdWrite,
};
#endif
#endif /* CFG_SUPPORT_EASY_DEBUG */
#endif /* WLAN_INCLUDE_PROC */

#if CFG_SUPPORT_CSI
static int procCSIDataOpen(struct inode *n, struct file *f)
{
	struct CSI_INFO_T *prCSIInfo = NULL;

	if (g_prGlueInfo_proc && g_prGlueInfo_proc->prAdapter) {
		prCSIInfo = glCsiGetCSIInfo();
		prCSIInfo->bIncomplete = FALSE;
	}

	return 0;
}

static int procCSIDataRelease(struct inode *n, struct file *f)
{
	struct CSI_INFO_T *prCSIInfo = NULL;

	if (g_prGlueInfo_proc && g_prGlueInfo_proc->prAdapter) {
		prCSIInfo = glCsiGetCSIInfo();
		prCSIInfo->bIncomplete = FALSE;
	}

	return 0;
}

static ssize_t procCSIDataRead(struct file *filp,
	char __user *buf, size_t count, loff_t *f_pos)
{
	uint8_t *temp;
	uint32_t u4CopySize = 0;
	uint32_t u4StartIdx = 0;
	int32_t i4Pos = 0;
	struct CSI_INFO_T *prCSIInfo = NULL;
	struct CSI_DATA_T *prTempCSIData = NULL;
	u_int8_t bStatus;

	if (g_prGlueInfo_proc && g_prGlueInfo_proc->u4ReadyFlag &&
			g_prGlueInfo_proc->prAdapter) {
		prCSIInfo = glCsiGetCSIInfo();
		temp = glCsiGetCSIBuf();
	} else {
		DBGLOG(REQ, WARN, "[CSI] driver is not ready.\n");
		return 0;
	}

	if (prCSIInfo->bIncomplete == FALSE) {

		wait_event_interruptible(g_prGlueInfo_proc->waitq_csi,
			prCSIInfo->u4CSIBufferUsed != 0);
		if (kalIsHalted() || kalIsResetting()) {
			DBGLOG(INIT, WARN,
				"[CSI] kalIsHalted=%u kalIsResetting=%u\n",
				kalIsHalted(), kalIsResetting());
			return -EFAULT;
		}

		prTempCSIData = glCsiGetCSIData();
		if (!prTempCSIData) {
			DBGLOG(REQ, ERROR, "[CSI] NULL CSI data.\n");
			return -EFAULT;
		}

		/*
		 * No older CSI data in buffer waiting for reading out,
		 * so prepare a new one for reading.
		 */
		bStatus = wlanPopCSIData(g_prGlueInfo_proc->prAdapter,
			prTempCSIData);
		if (bStatus)
			i4Pos = wlanCSIDataPrepare(temp,
				prCSIInfo, prTempCSIData);

		/* The frist run of reading the CSI data */
		u4StartIdx = 0;
		if (i4Pos > count) {
#ifdef CFG_SSVD_SVACE64
			u4CopySize = (uint32_t)count;
#else
			u4CopySize = count;
#endif
			prCSIInfo->u4RemainingDataSize = i4Pos - count;
			prCSIInfo->u4CopiedDataSize = count;
			prCSIInfo->bIncomplete = TRUE;
		} else {
			u4CopySize = i4Pos;
		}
	} else {
		/* Reading the remaining CSI data in the buffer */

		u4StartIdx = prCSIInfo->u4CopiedDataSize;
		if (prCSIInfo->u4RemainingDataSize > count) {
#ifdef CFG_SSVD_SVACE64
			u4CopySize = (uint32_t)count;
#else
			u4CopySize = count;
#endif
			prCSIInfo->u4RemainingDataSize -= count;
			prCSIInfo->u4CopiedDataSize += count;
		} else {
			u4CopySize = prCSIInfo->u4RemainingDataSize;
			prCSIInfo->bIncomplete = FALSE;
		}
	}

	if (copy_to_user(buf, temp + u4StartIdx, u4CopySize)) {
		DBGLOG(REQ, ERROR, "[CSI] copy to user failed\n");
		return -EFAULT;
	}

	*f_pos += u4CopySize;

#if CFG_CSI_DEBUG
	DBGLOG(REQ, INFO,
		"[CSI] copy size = %d, [used|head idx|tail idx] = [%d|%d|%d]\n",
		u4CopySize, prCSIInfo->u4CSIBufferUsed,
		prCSIInfo->u4CSIBufferHead, prCSIInfo->u4CSIBufferTail);
#endif

	return (ssize_t)u4CopySize;
}
#if KERNEL_VERSION(5, 6, 0) <= CFG80211_VERSION_CODE
static const struct proc_ops csidata_ops = {
	.proc_read = procCSIDataRead,
	.proc_open = procCSIDataOpen,
	.proc_release = procCSIDataRelease,
};
#else
static const struct file_operations csidata_ops = {
	.owner = THIS_MODULE,
	.read = procCSIDataRead,
	.open = procCSIDataOpen,
	.release = procCSIDataRelease,
};
#endif
#endif /* CFG_SUPPORT_CSI */
#endif /* (!CFG_MTK_ANDROID_WMT) || (BUILD_QA_DBG) */

#if (!CFG_MTK_ANDROID_WMT)
static ssize_t procCountryRead(struct file *filp, char __user *buf,
	size_t count, loff_t *f_pos)
{
	uint8_t *pucProcBuf = kalMemZAlloc(PROC_MAX_BUF_SIZE, VIR_MEM_TYPE);
	uint32_t u4CopySize;
	uint32_t country = 0;
	char acCountryStr[MAX_COUNTRY_CODE_LEN + 1] = {0};
	int32_t i4Ret = 0;

	/* if *f_pos > 0, it means has read successed last time */
	if (*f_pos > 0 || buf == NULL || pucProcBuf == NULL) {
		i4Ret = 0;
		goto freeBuf;
	}

	country = rlmDomainGetCountryCode();
	rlmDomainU32ToAlpha(country, acCountryStr);

	if (country)
		kalSnprintf(pucProcBuf, PROC_MAX_BUF_SIZE,
			"Current Country Code: %s\n", acCountryStr);
	else
		kalSnprintf(pucProcBuf, PROC_MAX_BUF_SIZE,
			"Current Country Code: NULL\n");

	u4CopySize = kalStrLen(pucProcBuf);
	if (u4CopySize > count)
		u4CopySize = count;
	if (copy_to_user(buf, pucProcBuf, u4CopySize)) {
		DBGLOG(REQ, WARN, "copy to user failed\n");
		i4Ret = -EFAULT;
		goto freeBuf;
	}
	*f_pos += u4CopySize;
	i4Ret = u4CopySize;
freeBuf:
	if (pucProcBuf)
		kalMemFree(pucProcBuf, VIR_MEM_TYPE, PROC_MAX_BUF_SIZE);
	return i4Ret;
}

static ssize_t procCountryWrite(struct file *file, const char __user *buffer,
	size_t count, loff_t *data)
{
	uint8_t *pucProcBuf = kalMemZAlloc(PROC_MAX_BUF_SIZE, VIR_MEM_TYPE);
	uint32_t u4BufLen = 0;
	uint32_t rStatus;
	uint32_t u4CopySize = PROC_MAX_BUF_SIZE;
	int32_t i4Ret = 0;

	if (buffer == NULL || pucProcBuf == NULL) {
		i4Ret = 0;
		goto freeBuf;
	}

	u4CopySize = (count < u4CopySize) ? count : (u4CopySize - 1);

	if (u4CopySize < 2) {
		DBGLOG(REQ, WARN, "Invaild country code len[%u]\n",
			u4CopySize);
		i4Ret = -EFAULT;
		goto freeBuf;
	}
	if (copy_from_user(pucProcBuf, buffer, u4CopySize)) {
		DBGLOG(REQ, WARN, "error of copy from user\n");
		i4Ret = -EFAULT;
		goto freeBuf;
	}
	pucProcBuf[u4CopySize] = '\0';

	if (regd_is_single_sku_en()) {
		struct COUNTRY_CODE_SETTING prCountrySetting = {0};

		prCountrySetting.aucCountryCode[0] = pucProcBuf[0];
		prCountrySetting.aucCountryCode[1] = pucProcBuf[1];
		prCountrySetting.ucCountryLength = 2;
		prCountrySetting.fgNeedHoldRtnlLock = 1;
		rStatus = kalIoctl(g_prGlueInfo_proc,
					wlanoidSetCountryCode,
					&prCountrySetting,
					sizeof(struct COUNTRY_CODE_SETTING),
					&u4BufLen);
	} else {
		rStatus = kalIoctl(g_prGlueInfo_proc, wlanoidSetCountryCode,
			   pucProcBuf, 2, &u4BufLen);
	}

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, INFO, "failed set country code: %s\n",
			pucProcBuf);
		i4Ret = -EINVAL;
		goto freeBuf;
	}

	i4Ret = u4CopySize;
freeBuf:
	if (pucProcBuf)
		kalMemFree(pucProcBuf, VIR_MEM_TYPE, PROC_MAX_BUF_SIZE);
	return i4Ret;
}

#if KERNEL_VERSION(5, 6, 0) <= CFG80211_VERSION_CODE
static const struct proc_ops country_ops = {
	.proc_read = procCountryRead,
	.proc_write = procCountryWrite,
};
#else
static const struct file_operations country_ops = {
	.owner = THIS_MODULE,
	.read = procCountryRead,
	.write = procCountryWrite,
};
#endif

#if (CFG_CE_ASSERT_DUMP == 1)
static ssize_t procCoreDumpRead(struct file *file, char __user *buf,
			size_t count, loff_t *f_pos)
{
	struct GLUE_INFO *prGlueInfo = g_prGlueInfo_proc;
	struct ADAPTER *prAdapter;
	struct sk_buff *skb = NULL;
	int copyLen = 0;
	unsigned long ret_len = 0;

	KAL_SPIN_LOCK_DECLARATION();

	if (!prGlueInfo) {
		DBGLOG(REQ, WARN, "procCfgRead prGlueInfo is  NULL\n");
		return -EFAULT;
	}

	prAdapter = prGlueInfo->prAdapter;

	if (!prAdapter) {
		DBGLOG(REQ, WARN, "procCfgRead prAdapter is  NULL\n");
		return -EFAULT;
	}

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_CORE_DUMP);
	skb = skb_dequeue(&prGlueInfo->rCoreDumpSkbQueue);
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_CORE_DUMP);

	if (skb == NULL)
		return 0;

	if (skb->len <= count) {
		ret_len = copy_to_user(buf, skb->data, skb->len);
		if (ret_len) {
			DBGLOG(INIT, ERROR,
		"%s: copy_to_user failed, skb->len = %d, ret_len = %ld, count = %zd",
					__func__, skb->len, ret_len, count);
			/* copy_to_user failed, add skb to fw log queue */
			KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_CORE_DUMP);
			skb_queue_head(&prGlueInfo->rCoreDumpSkbQueue, skb);
			KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_CORE_DUMP);
			copyLen = -EFAULT;
			goto out;
		}
		copyLen = skb->len;
	} else
		DBGLOG(INIT, ERROR,
			"%s: socket buffer length error(count: %d, skb.len: %d)",
			__func__, (int)count, skb->len);

	kfree_skb(skb);

out:
	return copyLen;
}

static unsigned int procCoreDumpPoll(struct file *file, poll_table *wait)
{
	struct GLUE_INFO *prGlueInfo = g_prGlueInfo_proc;
	unsigned int mask = 0;

	if (!prGlueInfo) {
		DBGLOG(REQ, WARN, "procCoreDumpPoll prGlueInfo is  NULL\n");
		return -EFAULT;
	}

	poll_wait(file, &prGlueInfo->waitq_coredump, wait);
	if (skb_queue_len(&prGlueInfo->rCoreDumpSkbQueue) > 0)
		mask |= POLLIN | POLLRDNORM;

	return mask;
}

#if KERNEL_VERSION(5, 6, 0) <= CFG80211_VERSION_CODE
static const struct proc_ops coredump_ops = {
	.proc_read = procCoreDumpRead,
	.proc_poll = procCoreDumpPoll
};
#else
static const struct file_operations coredump_ops = {
	.owner = THIS_MODULE,
	.read = procCoreDumpRead,
	.poll = procCoreDumpPoll
};
#endif
#endif /* CFG_CE_ASSERT_DUMP */
#endif /* (!CFG_MTK_ANDROID_WMT) */

#if (!CFG_MTK_ANDROID_WMT) && (BUILD_QA_DBG)
static ssize_t procPktDelayDbgCfgRead(struct file *filp, char __user *buf,
	size_t count, loff_t *f_pos)
{
	uint8_t *pucProcBuf = kalMemZAlloc(PROC_MAX_BUF_SIZE, VIR_MEM_TYPE);
	uint8_t *temp = NULL;
	uint8_t *str = NULL;
	uint32_t u4CopySize = 0;
	uint8_t ucTxRxFlag = 0;
	uint8_t ucTxIpProto = 0;
	uint16_t u2TxUdpPort = 0;
	uint32_t u4TxDelayThreshold = 0;
	uint8_t ucRxIpProto = 0;
	uint16_t u2RxUdpPort = 0;
	uint32_t u4RxDelayThreshold = 0;
	uint32_t u4StrLen = 0;
	int32_t i4Ret = 0;

	/* if *f_ops>0, we should return 0 to make cat command exit */
	if (*f_pos > 0 || buf == NULL || pucProcBuf == NULL) {
		i4Ret = 0;
		goto freeBuf;
	}

	str = "\n"
		"Usage: txLog/rxLog/reset 1(ICMP)/6(TCP)/11(UDP) Dst/SrcPortNum DelayThreshold(us)\n"
		"Print tx delay log,                                   such as: echo txLog 0 0 0 > pktDelay\n"
		"Print tx UDP delay log,                               such as: echo txLog 11 0 0 > pktDelay\n"
		"Print tx UDP dst port19305 delay log,                 such as: echo txLog 11 19305 0 > pktDelay\n"
		"Print rx UDP src port19305 delay more than 500us log, such as: echo rxLog 11 19305 500 > pktDelay\n"
		"Print tx TCP delay more than 500us log,               such as: echo txLog 6 0 500 > pktDelay\n"
		"Close log,                                            such as: echo reset 0 0 0 > pktDelay\n"
		"\n";
	u4StrLen = kalStrLen(str);
	temp = pucProcBuf;
	kalStrnCpy(temp, str, u4StrLen);
	temp += u4StrLen;

#if (CFG_SUPPORT_STATISTICS == 1)
	StatsEnvGetPktDelay(&ucTxRxFlag, &ucTxIpProto, &u2TxUdpPort,
			&u4TxDelayThreshold, &ucRxIpProto, &u2RxUdpPort,
			&u4RxDelayThreshold);
#endif

	if (ucTxRxFlag & BIT(0)) {
		SNPRINTF(temp, PROC_MAX_BUF_SIZE - kalStrLen(pucProcBuf),
			("txLog %x %d %d\n", ucTxIpProto, u2TxUdpPort,
			u4TxDelayThreshold));
	}
	if (ucTxRxFlag & BIT(1)) {
		SNPRINTF(temp, PROC_MAX_BUF_SIZE - kalStrLen(pucProcBuf),
			("rxLog %x %d %d\n", ucRxIpProto, u2RxUdpPort,
			u4RxDelayThreshold));
	}
	if (ucTxRxFlag == 0)
		SNPRINTF(temp, PROC_MAX_BUF_SIZE - kalStrLen(pucProcBuf),
			("reset 0 0 0, there is no tx/rx delay log\n"));

	u4CopySize = kalStrLen(pucProcBuf);
	if (u4CopySize > count)
		u4CopySize = count;
	if (copy_to_user(buf, pucProcBuf, u4CopySize)) {
		DBGLOG(REQ, WARN, "copy to user failed\n");
		i4Ret = -EFAULT;
		goto freeBuf;
	}

	*f_pos += u4CopySize;
	i4Ret = u4CopySize;
freeBuf:
	if (pucProcBuf)
		kalMemFree(pucProcBuf, VIR_MEM_TYPE, PROC_MAX_BUF_SIZE);
	return i4Ret;
}

static ssize_t procPktDelayDbgCfgWrite(struct file *file, const char *buffer,
	size_t count, loff_t *data)
{
#define MODULE_NAME_LENGTH 7
#define MODULE_RESET 0
#define MODULE_TX 1
#define MODULE_RX 2

	uint8_t *pucProcBuf = kalMemZAlloc(PROC_MAX_BUF_SIZE, VIR_MEM_TYPE);
	uint32_t u4CopySize = PROC_MAX_BUF_SIZE;
	uint8_t *temp = pucProcBuf;
	uint8_t aucModule[MODULE_NAME_LENGTH];
	uint32_t u4DelayThreshold = 0;
	uint32_t u4PortNum = 0;
	uint32_t u4IpProto = 0;
	uint8_t aucResetArray[MODULE_NAME_LENGTH] = "reset";
	uint8_t aucTxArray[MODULE_NAME_LENGTH] = "txLog";
	uint8_t aucRxArray[MODULE_NAME_LENGTH] = "rxLog";
	uint8_t ucTxOrRx = 0;
	int32_t i4Ret = 0;

	if (buffer == NULL || pucProcBuf == NULL) {
		i4Ret = 0;
		goto freeBuf;
	}

	u4CopySize = (count < u4CopySize) ? count : (u4CopySize - 1);

	if (copy_from_user(pucProcBuf, buffer, u4CopySize)) {
		DBGLOG(REQ, WARN, "error of copy from user\n");
		i4Ret = -EFAULT;
		goto freeBuf;
	}
	pucProcBuf[u4CopySize] = '\0';

	temp = pucProcBuf;
	while (temp) {
		kalMemSet(aucModule, 0, MODULE_NAME_LENGTH);

		/* pick up a string and teminated after meet : */
		if (sscanf
		    (temp, "%6s %x %d %d", aucModule, &u4IpProto, &u4PortNum,
		     &u4DelayThreshold) != 4) {
			pr_info("read param fail, aucModule=%s\n", aucModule);
			break;
		}

		if (kalStrnCmp
			(aucModule, aucResetArray, MODULE_NAME_LENGTH) == 0) {
			ucTxOrRx = MODULE_RESET;
		} else if (kalStrnCmp
			(aucModule, aucTxArray, MODULE_NAME_LENGTH) == 0) {
			ucTxOrRx = MODULE_TX;
		} else if (kalStrnCmp
			(aucModule, aucRxArray, MODULE_NAME_LENGTH) == 0) {
			ucTxOrRx = MODULE_RX;
		} else {
			pr_info("input module error!\n");
			break;
		}

		temp = kalStrChr(temp, ',');
		if (!temp)
			break;
		temp++;		/* skip ',' */
	}

#if (CFG_SUPPORT_STATISTICS == 1)
	StatsEnvSetPktDelay(ucTxOrRx, (uint8_t) u4IpProto, (uint16_t) u4PortNum,
		u4DelayThreshold);
#endif

	i4Ret = u4CopySize;
freeBuf:
	if (pucProcBuf)
		kalMemFree(pucProcBuf, VIR_MEM_TYPE, PROC_MAX_BUF_SIZE);
	return i4Ret;
}

#if KERNEL_VERSION(5, 6, 0) <= CFG80211_VERSION_CODE
static const struct proc_ops proc_pkt_delay_dbg_ops = {
	.proc_read = procPktDelayDbgCfgRead,
	.proc_write = procPktDelayDbgCfgWrite,
};
#else
static const struct file_operations proc_pkt_delay_dbg_ops = {
	.owner = THIS_MODULE,
	.read = procPktDelayDbgCfgRead,
	.write = procPktDelayDbgCfgWrite,
};
#endif

#if WLAN_INCLUDE_PROC
#if CFG_SUPPORT_EASY_DEBUG
static void *procEfuseDump_start(struct seq_file *s, loff_t *pos)
{
	static unsigned long counter;

	if (*pos == 0)
		counter = *pos;	/* read file init */

	if (counter >= EFUSE_ADDR_MAX)
		return NULL;
	return &counter;
}

static void *procEfuseDump_next(struct seq_file *s, void *v, loff_t *pos)
{
	unsigned long *tmp_v = (unsigned long *)v;

	(*tmp_v) += EFUSE_BLOCK_SIZE;

	if (*tmp_v >= EFUSE_ADDR_MAX)
		return NULL;
	return tmp_v;
}

static void procEfuseDump_stop(struct seq_file *s, void *v)
{
	/* nothing to do, we use a static value in start() */
}

static int procEfuseDump_show(struct seq_file *s, void *v)
{
#if  (CFG_EEPROM_PAGE_ACCESS == 1)
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	struct GLUE_INFO *prGlueInfo;
	uint32_t idx_addr, idx_value;
	struct PARAM_CUSTOM_ACCESS_EFUSE rAccessEfuseInfo = { };

	prGlueInfo = g_prGlueInfo_proc;

	if (prGlueInfo == NULL) {
		seq_puts(s, "prGlueInfo is null\n");
		return -EPERM;
	}

	if (prGlueInfo->prAdapter == NULL) {
		seq_puts(s, "prAdapter is null\n");
		return -EPERM;
	}

	if (prGlueInfo &&
	    prGlueInfo->prAdapter &&
	    prGlueInfo->prAdapter->chip_info &&
	    !prGlueInfo->prAdapter->chip_info->is_support_efuse) {
		seq_puts(s, "efuse ops is invalid\n");
		return -EPERM; /* return negative value to stop read process */
	}

	idx_addr = *(loff_t *) v;
	rAccessEfuseInfo.u4Address =
		(idx_addr / EFUSE_BLOCK_SIZE) * EFUSE_BLOCK_SIZE;

	rStatus = kalIoctl(prGlueInfo, wlanoidQueryProcessAccessEfuseRead,
			&rAccessEfuseInfo,
			sizeof(struct PARAM_CUSTOM_ACCESS_EFUSE), &u4BufLen);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		seq_printf(s, "efuse read fail (0x%03X)\n",
			rAccessEfuseInfo.u4Address);
		return 0;
	}

	for (idx_value = 0; idx_value < EFUSE_BLOCK_SIZE; idx_value++)
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
		seq_printf(s, "0x%03X=0x%02X\n",
			rAccessEfuseInfo.u4Address + idx_value,
			rAccessEfuseInfo.aucData[idx_value]);
#else
		seq_printf(s, "0x%03X=0x%02X\n",
			rAccessEfuseInfo.u4Address + idx_value,
			prGlueInfo->prAdapter->aucEepromVaule[idx_value]);
#endif

	return 0;
#else
	seq_puts(s, "efuse ops is invalid\n");
	return -EPERM; /* return negative value to stop read process */
#endif
}

static int procEfuseDumpOpen(struct inode *inode, struct file *file)
{
	static const struct seq_operations procEfuseDump_ops = {
		.start = procEfuseDump_start,
		.next = procEfuseDump_next,
		.stop = procEfuseDump_stop,
		.show = procEfuseDump_show
	};

	return seq_open(file, &procEfuseDump_ops);
}

#if KERNEL_VERSION(5, 6, 0) <= CFG80211_VERSION_CODE
static const struct proc_ops efusedump_ops = {
	.proc_open = procEfuseDumpOpen,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = seq_release,
};
#else
static const struct file_operations efusedump_ops = {
	.owner = THIS_MODULE,
	.open = procEfuseDumpOpen,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};
#endif
#endif /* CFG_SUPPORT_EASY_DEBUG */
#endif /* WLAN_INCLUDE_PROC */

#if CFG_WIFI_TXPWR_TBL_DUMP
void print_txpwr_tbl(struct txpwr_table *txpwr_tbl, unsigned char ch,
				unsigned char *tx_pwr[], char pwr_offset[],
				char *stream_buf[], unsigned int stream_pos[])
{
	struct txpwr_table_entry *tmp_tbl = txpwr_tbl->tables;
	unsigned int idx, pwr_idx, stream_idx;
	char pwr[TXPWR_TBL_NUM] = {0}, tmp_pwr = 0;
	char prefix[5], tmp[4];
	char *buf = NULL;
	unsigned int *pos = NULL;
	int i;

	DBGLOG(REQ, INFO, "Enter print_txpwr_tbl\n");

	/* n_tables: MCS number of each modulation */
	for (i = 0; i < txpwr_tbl->n_tables; i++) {
		idx = tmp_tbl[i].idx;

		for (pwr_idx = 0; pwr_idx < TXPWR_TBL_NUM; pwr_idx++) {
			if (!tx_pwr[pwr_idx]) {
				DBGLOG(REQ, WARN,
				       "Power table[%d] is NULL\n", pwr_idx);
				return;
			}
			pwr[pwr_idx] = tx_pwr[pwr_idx][idx] +
				       pwr_offset[pwr_idx];
			pwr[pwr_idx] = (pwr[pwr_idx] > MAX_TX_POWER) ?
				       MAX_TX_POWER : pwr[pwr_idx];
		}

		for (stream_idx = 0; stream_idx < STREAM_NUM; stream_idx++) {
			buf = stream_buf[stream_idx];
			pos = &stream_pos[stream_idx];

			if (tmp_tbl[i].mcs[stream_idx][0] == '\0')
				continue;

			switch (stream_idx) {
			case STREAM_SISO:
				kalStrnCpy(prefix, "siso", sizeof(prefix));
				break;
			/*
			 * CDD offset did not include
			 * The CDD values are the same as the values of SISO
			 */
			case STREAM_CDD:
				kalStrnCpy(prefix, "cdd", sizeof(prefix));
				break;
			case STREAM_MIMO:
				kalStrnCpy(prefix, "mimo", sizeof(prefix));
				break;
			}

			*pos += kalScnprintf(buf + *pos, TMP_SZ - *pos,
				"%s, %d, %s, %s, ",
				prefix, ch,
				txpwr_tbl->phy_mode,
				tmp_tbl[i].mcs[stream_idx]);

			for (pwr_idx = 0; pwr_idx < TXPWR_TBL_NUM; pwr_idx++) {
				tmp_pwr = pwr[pwr_idx];

				tmp_pwr = (tmp_pwr > 0) ? tmp_pwr : 0;

				if (pwr_idx + 1 == TXPWR_TBL_NUM)
					kalStrnCpy(tmp, "\n", sizeof(tmp));
				else
					kalStrnCpy(tmp, ", ", sizeof(tmp));
				*pos += kalScnprintf(buf + *pos, TMP_SZ - *pos,
					"%d.%d%s",
					tmp_pwr / 2,
					tmp_pwr % 2 * 5,
					tmp);
			}
		}
	}
}

char *g_txpwr_tbl_read_buffer;
char *g_txpwr_tbl_read_buffer_head;
unsigned int g_txpwr_tbl_read_residual;

static ssize_t procGetTxpwrTblRead(struct file *filp, char __user *buf,
				   size_t count, loff_t *f_pos)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER  *prAdapter = NULL;
	struct WIFI_VAR *prWifiVar;
	struct BSS_INFO *prBssInfo = NULL;
	unsigned char ucBssIndex;
	uint32_t status;
	struct PARAM_CMD_GET_TXPWR_TBL pwr_tbl;
	struct POWER_LIMIT *tx_pwr_tbl = pwr_tbl.tx_pwr_tbl;
	char *buffer;
	unsigned int pos = 0, buf_len = TXPWR_DUMP_SZ, oid_len;
	unsigned char i, j;
	char *stream_buf[STREAM_NUM] = {NULL};
	unsigned int stream_pos[STREAM_NUM] = {0};
	unsigned char *tx_pwr[TXPWR_TBL_NUM] =  {NULL};
	char pwr_offset[TXPWR_TBL_NUM] = {0};
	int ret;

	DBGLOG(REQ, INFO, "Enter procGetTxpwrTblRead\n");

	/* Re-entry to the func to print the remaining table */
	if (*f_pos > 0) { /* re-entry */
		pos = g_txpwr_tbl_read_residual;
		buffer = g_txpwr_tbl_read_buffer;
		goto next_entry;
	}

	if (buf == NULL) {
		DBGLOG(REQ, WARN, "empty buf: exit cat");
		return 0;
	}

	prGlueInfo = g_prGlueInfo_proc;
	if (!prGlueInfo) {
		DBGLOG(REQ, WARN, "can't get glue info");
		return -EFAULT;
	}

	prAdapter = prGlueInfo->prAdapter;
	prWifiVar = &prAdapter->rWifiVar;
	ucBssIndex = prWifiVar->ucBssIdStartValue;
	prBssInfo = prAdapter->aprBssInfo[ucBssIndex];
	if (!prBssInfo) {
		DBGLOG(REQ, WARN, "can't get the BssInfo from adapter");
		return -EFAULT;
	}

	kalMemZero(&pwr_tbl, sizeof(pwr_tbl));


	/* Complete the cmd/event process */
	status = kalIoctl(prGlueInfo, wlanoidGetTxPwrTbl,
			&pwr_tbl, sizeof(pwr_tbl), &oid_len);

	if (status != WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, WARN, "Query Tx Power Table fail\n");
		return -EINVAL;
	}
	DBGLOG(REQ, INFO, "Query Tx Power Table success\n");


	buffer = (char *) kalMemAlloc(buf_len, VIR_MEM_TYPE);
	if (!buffer) {
		return -ENOMEM;
		DBGLOG(REQ, WARN, "buffer is empty\n");
	}

	g_txpwr_tbl_read_buffer = buffer;
	g_txpwr_tbl_read_buffer_head = buffer;

	for (i = 0; i < STREAM_NUM; i++) {
		stream_buf[i] = (char *) kalMemAlloc(TMP_SZ, VIR_MEM_TYPE);
		if (!stream_buf[i]) {
			ret = -ENOMEM;
			goto out;
		}
	}
	DBGLOG(REQ, INFO, "stream init\n");


	pos = kalScnprintf(buffer, buf_len,
				"\n%s",
				"spatial stream, Channel, bw, modulation, ");
	pos += kalScnprintf(buffer + pos, buf_len - pos,
				"%s\n",
				"regulatory limit, board limit, target power");

	for (i = 0; i < ARRAY_SIZE(txpwr_tables); i++) {
		for (j = 0; j < STREAM_NUM; j++) {
			kalMemZero(stream_buf[j], TMP_SZ);
			stream_pos[j] = 0;
		}

		for (j = 0; j < TXPWR_TBL_NUM; j++) {
			tx_pwr[j] = NULL;
			pwr_offset[j] = 0;
		}

		/*
		 * In the rate is not supported on this channel,
		 * its limit table value will be the default 127
		*/

		switch (i) {
		case DSSS:
			if (pwr_tbl.ucCenterCh > 14)
				continue;
			DBGLOG(REQ, INFO, "Print DSSS table\n");
			for (j = 0; j < TXPWR_TBL_NUM; j++)
				tx_pwr[j] = tx_pwr_tbl[j].tx_pwr_dsss;
			break;
		case OFDM_24G:
			if (pwr_tbl.ucCenterCh > 14)
				continue;
			DBGLOG(REQ, INFO, "Print OFDM_24G table\n");
			for (j = 0; j < TXPWR_TBL_NUM; j++)
				tx_pwr[j] = tx_pwr_tbl[j].tx_pwr_ofdm;
			break;
		case OFDM_5G:
			if (pwr_tbl.ucCenterCh <= 14)
				continue;
			DBGLOG(REQ, INFO, "Print OFDM_5G table\n");
			for (j = 0; j < TXPWR_TBL_NUM; j++)
				tx_pwr[j] = tx_pwr_tbl[j].tx_pwr_ofdm;
			break;
		case HT20:
			DBGLOG(REQ, INFO, "Print HT20 table\n");
			for (j = 0; j < TXPWR_TBL_NUM; j++)
				tx_pwr[j] = tx_pwr_tbl[j].tx_pwr_ht20;
			break;
		case HT40:
			if (pwr_tbl.ucCenterCh <= 14 ||
					tx_pwr_tbl[0].tx_pwr_ht40[0] >= 127)
				continue;
			DBGLOG(REQ, INFO, "Print HT40 table\n");
			for (j = 0; j < TXPWR_TBL_NUM; j++)
				tx_pwr[j] = tx_pwr_tbl[j].tx_pwr_ht40;
			break;
		case VHT20:
			DBGLOG(REQ, INFO, "Print VHT20 table\n");
			for (j = 0; j < TXPWR_TBL_NUM; j++)
				tx_pwr[j] = tx_pwr_tbl[j].tx_pwr_vht20;
			break;
		case VHT40:
			if (pwr_tbl.ucCenterCh <= 14 ||
					tx_pwr_tbl[0].tx_pwr_vht40[0] >= 127)
				continue;
			DBGLOG(REQ, INFO, "Print VHT40 table\n");
			for (j = 0; j < TXPWR_TBL_NUM; j++)
				tx_pwr[j] = tx_pwr_tbl[j].tx_pwr_vht40;
			break;
		case VHT80:
			if (pwr_tbl.ucCenterCh <= 14 ||
					tx_pwr_tbl[0].tx_pwr_vht80[0] >= 127)
				continue;
			DBGLOG(REQ, INFO, "Print VHT80 table\n");
			for (j = 0; j < TXPWR_TBL_NUM; j++)
				tx_pwr[j] = tx_pwr_tbl[j].tx_pwr_vht80;
			break;
		case VHT160:
			if (pwr_tbl.ucCenterCh <= 14 ||
					tx_pwr_tbl[0].tx_pwr_vht160[0] >= 127)
				continue;
			DBGLOG(REQ, INFO, "Print VHT160 table\n");
			for (j = 0; j < TXPWR_TBL_NUM; j++)
				tx_pwr[j] = tx_pwr_tbl[j].tx_pwr_vht160;
			break;
#if (CFG_WIFI_TXPWR_TBL_DUMP_HE == 1)
		case HE26:
			DBGLOG(REQ, INFO, "Print HE26 table\n");
			for (j = 0; j < TXPWR_TBL_NUM; j++)
				tx_pwr[j] = tx_pwr_tbl[j].tx_pwr_he26;
			break;
		case HE52:
			DBGLOG(REQ, INFO, "Print HE52 table\n");
			for (j = 0; j < TXPWR_TBL_NUM; j++)
				tx_pwr[j] = tx_pwr_tbl[j].tx_pwr_he52;
			break;
		case HE106:
			DBGLOG(REQ, INFO, "Print HE106 table\n");
			for (j = 0; j < TXPWR_TBL_NUM; j++)
				tx_pwr[j] = tx_pwr_tbl[j].tx_pwr_he106;
			break;
		case HE242:
			DBGLOG(REQ, INFO, "Print HE242 table\n");
			for (j = 0; j < TXPWR_TBL_NUM; j++)
				tx_pwr[j] = tx_pwr_tbl[j].tx_pwr_he242;
			break;
		case HE484:
			if (pwr_tbl.ucCenterCh <= 14 ||
					tx_pwr_tbl[0].tx_pwr_he484[0] >= 127)
				continue;
			DBGLOG(REQ, INFO, "Print HE484 table\n");
			for (j = 0; j < TXPWR_TBL_NUM; j++)
				tx_pwr[j] = tx_pwr_tbl[j].tx_pwr_he484;
			break;
		case HE996:
			if (pwr_tbl.ucCenterCh <= 14 ||
					tx_pwr_tbl[0].tx_pwr_he996[0] >= 127)
				continue;
			DBGLOG(REQ, INFO, "Print HE996 table\n");
			for (j = 0; j < TXPWR_TBL_NUM; j++)
				tx_pwr[j] = tx_pwr_tbl[j].tx_pwr_he996;
			break;
		case HE996X2:
			if (pwr_tbl.ucCenterCh <= 14 ||
					tx_pwr_tbl[0].tx_pwr_he996x2[0] >= 127)
				continue;
			DBGLOG(REQ, INFO, "Print HE996X2 table\n");
			for (j = 0; j < TXPWR_TBL_NUM; j++)
				tx_pwr[j] = tx_pwr_tbl[j].tx_pwr_he996x2;
			break;
#endif
		default:
			break;
		}

		print_txpwr_tbl(&txpwr_tables[i], pwr_tbl.ucCenterCh,
				tx_pwr, pwr_offset,
				stream_buf, stream_pos);

		for (j = 0; j < STREAM_NUM; j++) {
			pos += kalScnprintf(buffer + pos, buf_len - pos,
				"%s",
				stream_buf[j]);
		}
	}

	g_txpwr_tbl_read_residual = pos;

next_entry:
	if (pos > count)
		pos = count;

	if (copy_to_user(buf, buffer, pos)) {
		DBGLOG(INIT, WARN, "copy to user failed\n");
		ret = -EFAULT;
		goto out;
	}

	g_txpwr_tbl_read_buffer += pos;
	g_txpwr_tbl_read_residual -= pos;

	*f_pos += pos;
	ret = pos;
out:
	if (ret == 0 || ret == -ENOMEM) {
		for (i = 0; i < STREAM_NUM; i++) {
			if (stream_buf[i])
				kalMemFree(stream_buf[i], VIR_MEM_TYPE, TMP_SZ);
		}
		if (g_txpwr_tbl_read_buffer_head)
			kalMemFree(g_txpwr_tbl_read_buffer_head,
				VIR_MEM_TYPE, buf_len);

		g_txpwr_tbl_read_buffer = NULL;
		g_txpwr_tbl_read_buffer_head = NULL;
		g_txpwr_tbl_read_residual = 0;
	}

	return ret;
}

static const struct file_operations get_txpwr_tbl_ops = {
	.owner = THIS_MODULE,
	.read = procGetTxpwrTblRead,
};
#endif /* CFG_WIFI_TXPWR_TBL_DUMP */
#endif /* (!CFG_MTK_ANDROID_WMT) && (BUILD_QA_DBG) */

#if (BUILD_QA_DBG) || (ENABLE_DBG_LEVEL)
static ssize_t procDbgLevelRead(struct file *filp, char __user *buf,
	size_t count, loff_t *f_pos)
{
	uint8_t *pucProcBuf = kalMemZAlloc(PROC_MAX_BUF_SIZE, VIR_MEM_TYPE);
	uint8_t *temp = pucProcBuf;
	uint8_t *str = NULL;
	uint32_t u4CopySize = 0;
	uint16_t i;
	uint16_t u2ModuleNum = 0;
	uint32_t u4StrLen = 0;
	uint32_t u4Level1, u4Level2;
	int32_t i4Ret = 0;

	/* if *f_ops>0, we should return 0 to make cat command exit */
	if (*f_pos > 0 || buf == NULL || pucProcBuf == NULL) {
		i4Ret = 0;
		goto freeBuf;
	}

	str = "\nTEMP|LOUD|INFO|TRACE | EVENT|STATE|WARN|ERROR\n"
	    "bit7|bit6|bit5|bit4 | bit3|bit2|bit1|bit0\n\n"
	    "Usage: Module Index:Module Level, such as 0x00:0xff\n\n"
	    "Debug Module\tIndex\tLevel\tDebug Module\tIndex\tLevel\n\n";
	u4StrLen = kalStrLen(str);
	kalStrnCpy(temp, str, u4StrLen);
	temp += u4StrLen;

	u2ModuleNum =
	    (sizeof(aucDbModuleName) /
	     PROC_DBG_LEVEL_MAX_DISPLAY_STR_LEN) & 0xfe;

	for (i = 0; i < u2ModuleNum; i += 2) {
		wlanGetDriverDbgLevel(i, &u4Level1);
		wlanGetDriverDbgLevel(i + 1, &u4Level2);
		SNPRINTF(temp, PROC_MAX_BUF_SIZE - kalStrLen(pucProcBuf),
			("DBG_%s_IDX\t(0x%02x):\t0x%02x\t"
			 "DBG_%s_IDX\t(0x%02x):\t0x%02x\n",
			 &aucDbModuleName[i][0], i, (uint8_t) u4Level1,
			 &aucDbModuleName[i + 1][0], i + 1,
			 (uint8_t) u4Level2));
	}

	if ((sizeof(aucDbModuleName) /
	     PROC_DBG_LEVEL_MAX_DISPLAY_STR_LEN) & 0x1) {
		wlanGetDriverDbgLevel(u2ModuleNum, &u4Level1);
		SNPRINTF(temp, PROC_MAX_BUF_SIZE - kalStrLen(pucProcBuf),
			 ("DBG_%s_IDX\t(0x%02x):\t0x%02x\n",
			  &aucDbModuleName[u2ModuleNum][0], u2ModuleNum,
			  (uint8_t) u4Level1));
	}

	u4CopySize = kalStrLen(pucProcBuf);
	if (u4CopySize > count)
		u4CopySize = count;
	if (copy_to_user(buf, pucProcBuf, u4CopySize)) {
		DBGLOG(REQ, WARN, "copy to user failed\n");
		i4Ret = -EFAULT;
		goto freeBuf;
	}

	*f_pos += u4CopySize;
	i4Ret = u4CopySize;
freeBuf:
	if (pucProcBuf)
		kalMemFree(pucProcBuf, VIR_MEM_TYPE, PROC_MAX_BUF_SIZE);
	return i4Ret;
}

static ssize_t procDbgLevelWrite(struct file *file, const char __user *buffer,
	size_t count, loff_t *data)
{
	uint32_t u4NewDbgModule, u4NewDbgLevel;
	uint8_t *pucProcBuf = kalMemZAlloc(PROC_MAX_BUF_SIZE, VIR_MEM_TYPE);
	uint8_t *temp = NULL;
	uint32_t u4CopySize = PROC_MAX_BUF_SIZE;
	int32_t i4Ret = 0;

	if (buffer == NULL || pucProcBuf == NULL) {
		i4Ret = 0;
		goto freeBuf;
	}

	temp = pucProcBuf;
	u4CopySize = (count < u4CopySize) ? count : (u4CopySize - 1);

	if (copy_from_user(pucProcBuf, buffer, u4CopySize)) {
		DBGLOG(REQ, WARN, "error of copy from user\n");
		i4Ret = -EFAULT;
		goto freeBuf;
	}
	pucProcBuf[u4CopySize] = '\0';

	/*add chip reset cmd for manual test*/
#if CFG_CHIP_RESET_SUPPORT
	if (temp[0] == 'R') {
		DBGLOG(INIT, INFO, "WIFI trigger reset!!\n");
		GL_USER_DEFINE_RESET_TRIGGER(g_prGlueInfo_proc->prAdapter,
			RST_CMD_TRIGGER, RST_FLAG_DO_WHOLE_RESET);
		temp[0] = 'X';
	}
#endif

	while (temp) {
		if (sscanf(temp,
			"0x%x:0x%x", &u4NewDbgModule, &u4NewDbgLevel) != 2) {
			pr_info("debug module and debug level should be one byte in length\n");
			break;
		}
		if (u4NewDbgModule == 0xFF) {
			wlanSetDriverDbgLevel(DBG_ALL_MODULE_IDX,
					(u4NewDbgLevel & DBG_CLASS_MASK));
			break;
		}
		if (u4NewDbgModule >= DBG_MODULE_NUM) {
			pr_info("debug module index should less than %d\n",
				DBG_MODULE_NUM);
			break;
		}
		wlanSetDriverDbgLevel(u4NewDbgModule,
				(u4NewDbgLevel & DBG_CLASS_MASK));
		temp = kalStrChr(temp, ',');
		if (!temp)
			break;
		temp++;		/* skip ',' */
	}

	i4Ret = u4CopySize;
freeBuf:
	if (pucProcBuf)
		kalMemFree(pucProcBuf, VIR_MEM_TYPE, PROC_MAX_BUF_SIZE);
	return i4Ret;
}

#if KERNEL_VERSION(5, 6, 0) <= CFG80211_VERSION_CODE
static const struct proc_ops dbglevel_ops = {
	.proc_read = procDbgLevelRead,
	.proc_write = procDbgLevelWrite,
};
#else
static const struct file_operations dbglevel_ops = {
	.owner = THIS_MODULE,
	.read = procDbgLevelRead,
	.write = procDbgLevelWrite,
};
#endif
#endif /* (BUILD_QA_DBG) || (ENABLE_DBG_LEVEL) */

#if (BUILD_QA_DBG)
/*----------------------------------------------------------------------------*/
/*!
 * \brief The PROC function for reading MCR register to User Space, the offset
 *        of the MCR is specified in u4McrOffset.
 *
 * \param[in] page       Buffer provided by kernel.
 * \param[in out] start  Start Address to read(3 methods).
 * \param[in] off        Offset.
 * \param[in] count      Allowable number to read.
 * \param[out] eof       End of File indication.
 * \param[in] data       Pointer to the private data structure.
 *
 * \return number of characters print to the buffer from User Space.
 */
/*----------------------------------------------------------------------------*/
static ssize_t procMCRRead(struct file *filp, char __user *buf,
	 size_t count, loff_t *f_pos)
{
	uint8_t *pucProcBuf = kalMemZAlloc(PROC_MAX_BUF_SIZE, VIR_MEM_TYPE);
	struct GLUE_INFO *prGlueInfo;
	struct PARAM_CUSTOM_MCR_RW_STRUCT rMcrInfo = {0};
	uint32_t u4BufLen;
	uint32_t u4CopySize = 0;
	uint8_t *temp = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	int32_t i4Ret = 0;

	/* if *f_ops>0, we should return 0 to make cat command exit */
	if (*f_pos > 0 || buf == NULL || pucProcBuf == NULL) {
		i4Ret = 0;
		goto freeBuf;
	}

	temp = pucProcBuf;
	prGlueInfo = g_prGlueInfo_proc;
	rMcrInfo.u4McrData = 0;
	rMcrInfo.u4McrOffset = u4McrOffset;

	rStatus = kalIoctl(prGlueInfo,
		wlanoidQueryMcrRead, (void *)&rMcrInfo,
		sizeof(rMcrInfo), &u4BufLen);
	SNPRINTF(temp, PROC_MAX_BUF_SIZE - kalStrLen(pucProcBuf),
		("MCR (0x%08xh): 0x%08x\n", rMcrInfo.u4McrOffset,
		rMcrInfo.u4McrData));

	u4CopySize = kalStrLen(pucProcBuf);
	if (u4CopySize > count)
		u4CopySize = count;
	if (copy_to_user(buf, pucProcBuf, u4CopySize)) {
		DBGLOG(REQ, WARN, "copy to user failed\n");
		i4Ret = -EFAULT;
		goto freeBuf;
	}

	*f_pos += u4CopySize;
	i4Ret = u4CopySize;
freeBuf:
	if (pucProcBuf)
		kalMemFree(pucProcBuf, VIR_MEM_TYPE, PROC_MAX_BUF_SIZE);
	return i4Ret;
} /* end of procMCRRead() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief The PROC function for writing MCR register to HW or update u4McrOffset
 *        for reading MCR later.
 *
 * \param[in] file   pointer to file.
 * \param[in] buffer Buffer from user space.
 * \param[in] count  Number of characters to write
 * \param[in] data   Pointer to the private data structure.
 *
 * \return number of characters write from User Space.
 */
/*----------------------------------------------------------------------------*/
static ssize_t procMCRWrite(struct file *file, const char __user *buffer,
	size_t count, loff_t *data)
{
	struct GLUE_INFO *prGlueInfo;
	/* + 1 for "\0" */
	char acBuf[PROC_MCR_ACCESS_MAX_USER_INPUT_LEN + 1];
	uint32_t u4CopySize = 0;
	struct PARAM_CUSTOM_MCR_RW_STRUCT rMcrInfo;
	uint32_t u4BufLen;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	int num = 0;

	ASSERT(data);

	u4CopySize = (count < sizeof(acBuf)) ? count : (sizeof(acBuf) - 1);
	if (copy_from_user(acBuf, buffer, u4CopySize))
		return 0;
	acBuf[u4CopySize] = '\0';

	num =
	    sscanf(acBuf, "0x%x 0x%x", &rMcrInfo.u4McrOffset,
		   &rMcrInfo.u4McrData);
	if (num < 0) {
		DBGLOG(INIT, ERROR, "can not get correct data\n");
		return 0;
	}

	switch (num) {
	case 2:
		/* NOTE: Sometimes we want to test if bus will still be ok,
		 * after accessing the MCR which is not align to DW boundary.
		 */
		/* if (IS_ALIGN_4(rMcrInfo.u4McrOffset)) */
		{
			prGlueInfo = g_prGlueInfo_proc;

			u4McrOffset = rMcrInfo.u4McrOffset;

			/* rMcrInfo.u4McrOffset, rMcrInfo.u4McrData); */

			rStatus = kalIoctl(prGlueInfo, wlanoidSetMcrWrite,
					   (void *)&rMcrInfo, sizeof(rMcrInfo),
					   &u4BufLen);

		}
		break;
	case 1:
		/* if (IS_ALIGN_4(rMcrInfo.u4McrOffset)) */
		{
			u4McrOffset = rMcrInfo.u4McrOffset;
		}
		break;

	default:
		break;
	}

	return u4CopySize;
}				/* end of procMCRWrite() */

#if KERNEL_VERSION(5, 6, 0) <= CFG80211_VERSION_CODE
static const struct proc_ops mcr_ops = {
	.proc_read = procMCRRead,
	.proc_write = procMCRWrite,
};
#else
static const struct file_operations mcr_ops = {
	.owner = THIS_MODULE,
	.read = procMCRRead,
	.write = procMCRWrite,
};
#endif

#if WLAN_INCLUDE_PROC
#if CFG_SUPPORT_EASY_DEBUG
static ssize_t procCfgRead(struct file *filp, char __user *buf, size_t count,
	loff_t *f_pos)
{
	uint8_t *pucProcBuf = kalMemZAlloc(PROC_MAX_BUF_SIZE, VIR_MEM_TYPE);
	uint8_t *temp = pucProcBuf;
	uint8_t *str = NULL;
	uint8_t *str2 = "\nERROR DUMP CONFIGURATION:\n";
	uint32_t u4CopySize = 0;
	uint32_t i;
	uint32_t u4StrLen = 0;
	int32_t i4Ret = 0;

#define BUFFER_RESERVE_BYTE 50

	struct GLUE_INFO *prGlueInfo;

	struct WLAN_CFG_ENTRY *prWlanCfgEntry;
	struct ADAPTER *prAdapter;

	/* if *f_ops>0, we should return 0 to make cat command exit */
	if (*f_pos > 0 || buf == NULL || pucProcBuf == NULL) {
		i4Ret = 0;
		goto freeBuf;
	}

	prGlueInfo = g_prGlueInfo_proc;

	if (!prGlueInfo) {
		DBGLOG(REQ, WARN, "procCfgRead prGlueInfo is NULL\n");
		i4Ret = 0;
		goto freeBuf;
	}

	prAdapter = prGlueInfo->prAdapter;

	if (!prAdapter) {
		DBGLOG(REQ, WARN, "procCfgRead prAdapter is NULL\n");
		i4Ret = 0;
		goto freeBuf;
	}

	str = "\nDUMP CONFIGURATION :\n"
	    "<KEY|VALUE> OR <D:KEY|VALUE>\n"
	    "'D': driver part current setting\n"
	    "===================================\n";
	u4StrLen = kalStrLen(str);
	kalStrnCpy(temp, str, u4StrLen);
	temp += u4StrLen;

	for (i = 0; i < WLAN_CFG_ENTRY_NUM_MAX; i++) {
		prWlanCfgEntry =
			wlanCfgGetEntryByIndex(prAdapter, i, WLAN_CFG_DEFAULT);

		if ((!prWlanCfgEntry) || (prWlanCfgEntry->aucKey[0] == '\0'))
			break;

		SNPRINTF(temp, PROC_MAX_BUF_SIZE - kalStrLen(pucProcBuf),
			("%s|%s\n", prWlanCfgEntry->aucKey,
			prWlanCfgEntry->aucValue));

		if ((temp - pucProcBuf) != kalStrLen(pucProcBuf)) {
			DBGLOG(INIT, ERROR,
			       "Dump configuration error: temp offset=%d, buf length=%u, key[%d]=[%u], val[%d]=[%u]\n",
			       (int)(temp - pucProcBuf),
			       (uint32_t)kalStrLen(pucProcBuf),
			       WLAN_CFG_KEY_LEN_MAX,
			       (uint32_t)prWlanCfgEntry->aucKey[
				WLAN_CFG_KEY_LEN_MAX - 1],
			       WLAN_CFG_VALUE_LEN_MAX,
			       (uint32_t)prWlanCfgEntry->aucValue[
				WLAN_CFG_VALUE_LEN_MAX - 1]);
			kalMemSet(pucProcBuf, 0, u4StrLen);
			kalStrnCpy(pucProcBuf, str2, kalStrLen(str2) + 1);
			pucProcBuf[kalStrLen(str2)] = '\0';
			goto procCfgReadLabel;
		}

		if (kalStrLen(pucProcBuf) >
			(PROC_MAX_BUF_SIZE - BUFFER_RESERVE_BYTE))
			break;
	}

	for (i = 0; i < WLAN_CFG_REC_ENTRY_NUM_MAX; i++) {
		prWlanCfgEntry =
			wlanCfgGetEntryByIndex(prAdapter, i, WLAN_CFG_REC);

		if ((!prWlanCfgEntry) || (prWlanCfgEntry->aucKey[0] == '\0'))
			break;

		SNPRINTF(temp, PROC_MAX_BUF_SIZE - kalStrLen(pucProcBuf),
			("D:%s|%s\n", prWlanCfgEntry->aucKey,
			prWlanCfgEntry->aucValue));

		if ((temp - pucProcBuf) != kalStrLen(pucProcBuf)) {
			DBGLOG(INIT, ERROR,
			       "D:Dump configuration error: temp offset=%d, buf length=%u, key[%d]=[%u], val[%d]=[%u]\n",
			       (int)(temp - pucProcBuf),
			       (uint32_t)kalStrLen(pucProcBuf),
			       WLAN_CFG_KEY_LEN_MAX,
			       (uint32_t)prWlanCfgEntry->aucKey[
				WLAN_CFG_KEY_LEN_MAX - 1],
			       WLAN_CFG_VALUE_LEN_MAX,
			       (uint32_t)prWlanCfgEntry->aucValue[
				WLAN_CFG_VALUE_LEN_MAX - 1]);
			kalMemSet(pucProcBuf, 0, u4StrLen);
			kalStrnCpy(pucProcBuf, str2, kalStrLen(str2) + 1);
			pucProcBuf[kalStrLen(str2)] = '\0';
			goto procCfgReadLabel;
		}

		if (kalStrLen(pucProcBuf) >
			(PROC_MAX_BUF_SIZE - BUFFER_RESERVE_BYTE))
			break;
	}

procCfgReadLabel:
	u4CopySize = kalStrLen(pucProcBuf);
	if (u4CopySize > count)
		u4CopySize = count;
	if (copy_to_user(buf, pucProcBuf, u4CopySize)) {
		DBGLOG(REQ, WARN, "copy to user failed\n");
		i4Ret = -EFAULT;
		goto freeBuf;
	}

	*f_pos += u4CopySize;
	i4Ret = u4CopySize;
freeBuf:
	if (pucProcBuf)
		kalMemFree(pucProcBuf, VIR_MEM_TYPE, PROC_MAX_BUF_SIZE);
	return i4Ret;
}

static ssize_t procCfgWrite(struct file *file, const char __user *buffer,
	size_t count, loff_t *data)
{
	uint8_t *pucProcBuf = kalMemZAlloc(PROC_MAX_BUF_SIZE, VIR_MEM_TYPE);
	uint32_t u4CopySize = PROC_MAX_BUF_SIZE-8;
	struct GLUE_INFO *prGlueInfo;
	uint8_t *pucTmp;
	uint32_t i = 0;
	int32_t i4Ret = 0;

	if (count <= 0) {
		DBGLOG(INIT, ERROR, "Wrong buffer size.\n");
		i4Ret = -EFAULT;
		goto freeBuf;
	}

	if (buffer == NULL || pucProcBuf == NULL) {
		i4Ret = 0;
		goto freeBuf;
	}

	kalMemSet(pucProcBuf, 0, u4CopySize);
	u4CopySize = (count < u4CopySize) ? count : (u4CopySize - 1);

	pucTmp = pucProcBuf;
	SNPRINTF(pucTmp, PROC_MAX_BUF_SIZE - kalStrLen(pucProcBuf),
		("%s ", "set_cfg"));

	if (copy_from_user(pucTmp, buffer, u4CopySize)) {
		DBGLOG(REQ, WARN, "error of copy from user\n");
		i4Ret = -EFAULT;
		goto freeBuf;
	}
	pucProcBuf[u4CopySize + 8] = '\0';

	for (i = 8 ; i < u4CopySize + 8; i++) {
		if (!isalnum(pucProcBuf[i]) && /* alphanumeric */
			pucProcBuf[i] != 0x20 && /* space */
			pucProcBuf[i] != 0x0a && /* control char */
			pucProcBuf[i] != 0x0d) {
			DBGLOG(INIT, ERROR, "wrong char[%d] 0x%x\n",
				i, pucProcBuf[i]);
			i4Ret = -EFAULT;
			goto freeBuf;
		}
	}

	prGlueInfo = g_prGlueInfo_proc;
	/* if g_i4NextDriverReadLen >0,
	 * the content for next DriverCmdRead will be
	 * in : pucProcBuf with length : g_i4NextDriverReadLen
	 */
	g_i4NextDriverReadLen =
		priv_driver_set_cfg(prGlueInfo->prDevHandler, pucProcBuf,
			kalStrLen(pucProcBuf));

	i4Ret = u4CopySize;
freeBuf:
	if (pucProcBuf)
		kalMemFree(pucProcBuf, VIR_MEM_TYPE, PROC_MAX_BUF_SIZE);
	return i4Ret;
}

#if KERNEL_VERSION(5, 6, 0) <= CFG80211_VERSION_CODE
static const struct proc_ops cfg_ops = {
	.proc_read = procCfgRead,
	.proc_write = procCfgWrite,
};
#else
static const struct file_operations cfg_ops = {
	.owner = THIS_MODULE,
	.read = procCfgRead,
	.write = procCfgWrite,
};
#endif
#endif /* CFG_SUPPORT_EASY_DEBUG */
#endif /* WLAN_INCLUDE_PROC */

#if (CFG_TWT_SMART_STA == 1)
static ssize_t procTwtSmartRead(struct file *filp, char __user *buf,
	size_t count, loff_t *f_pos)
{
	uint8_t *pucProcBuf = kalMemZAlloc(PROC_MAX_BUF_SIZE, VIR_MEM_TYPE);
	uint32_t u4CopySize = PROC_MAX_BUF_SIZE;
	int32_t i4Ret = 0;

	/* if *f_pos > 0, it means has read successed last time */
	if (*f_pos > 0 || buf == NULL || pucProcBuf == NULL) {
		i4Ret = 0;
		goto freeBuf;
	}

	kalSnprintf(pucProcBuf, PROC_MAX_BUF_SIZE,
		"Twt Smart Req:%d, TDReq:%d, Act:%d, State:%d\n",
		g_TwtSmartStaCtrl.fgTwtSmartStaReq,
		g_TwtSmartStaCtrl.fgTwtSmartStaTeardownReq,
		g_TwtSmartStaCtrl.fgTwtSmartStaActivated,
		g_TwtSmartStaCtrl.eState);

	u4CopySize = kalStrLen(pucProcBuf);
	u4CopySize = (count < u4CopySize) ? count : u4CopySize;
	u4CopySize = (u4CopySize < PROC_MAX_BUF_SIZE) ?
		u4CopySize : (PROC_MAX_BUF_SIZE - 1);
	if (copy_to_user(buf, pucProcBuf, u4CopySize)) {
		DBGLOG(INIT, WARN, "copy to user failed\n");
		i4Ret = -EFAULT;
		goto freeBuf;
	}
	*f_pos += u4CopySize;
	i4Ret = u4CopySize;
freeBuf:
	if (pucProcBuf)
		kalMemFree(pucProcBuf, VIR_MEM_TYPE, PROC_MAX_BUF_SIZE);
	return i4Ret;
}

static ssize_t procTwtSmartWrite(struct file *file, const char *buffer,
	size_t count, loff_t *data)
{
	size_t len = count;
	char buf[256];
	char *pBuf;
	int32_t x = 0;
	char *pToken = NULL;
	char *pDelimiter = " \t";
	int32_t res;

	if (len >= sizeof(buf))
		len = sizeof(buf) - 1;

	if (copy_from_user(buf, buffer, len)) {
		DBGLOG(INIT, WARN, "copy_from_user error\n");
		return -EFAULT;
	}

	buf[len] = '\0';
	DBGLOG(INIT, INFO, "%s: write parameter data = %s", __func__, buf);
	pBuf = buf;
	pToken = strsep(&pBuf, pDelimiter);

	if (pToken != NULL) {
		kalkStrtos32(pToken, 16, &res);
		x = (int)res;
	} else {
		DBGLOG(INIT, ERROR,
			"%s:%s input parameter fail![0]", __func__, buf);
		return -1;
	}

	switch (x) {
	case 0:
		break;

	case 1:
		g_TwtSmartStaCtrl.fgTwtSmartStaReq = TRUE;
		DBGLOG(INIT, INFO,
			"twt landing stareq %d",
			g_TwtSmartStaCtrl.fgTwtSmartStaReq);
		break;

	case 2:
		g_TwtSmartStaCtrl.u4TwtSwitch = 0;
		if (g_TwtSmartStaCtrl.fgTwtSmartStaActivated == TRUE)
			g_TwtSmartStaCtrl.fgTwtSmartStaTeardownReq = TRUE;

		g_TwtSmartStaCtrl.fgTwtSmartStaReq = FALSE;
		g_TwtSmartStaCtrl.eState = TWT_SMART_STA_STATE_IDLE;
		DBGLOG(INIT, INFO,
			"twt landing tdreq %d",
			g_TwtSmartStaCtrl.fgTwtSmartStaTeardownReq);
		break;
	}

	return len;

}

static const struct file_operations auto_twt_smart_ops = {
	.owner = THIS_MODULE,
	.read = procTwtSmartRead,
	.write = procTwtSmartWrite,
};
#endif
#endif /* (BUILD_QA_DBG) */

int32_t procInitFs(void)
{
	struct proc_dir_entry *prEntry;

	g_i4NextDriverReadLen = 0;
	prEntry = NULL;

	if (init_net.proc_net == (struct proc_dir_entry *)NULL) {
		DBGLOG(INIT, ERROR,
			"init proc fs fail: proc_net == NULL\n");
		return -ENOENT;
	}

	/*
	 * Directory: Root (/proc/net/wlan0)
	 */

	gprProcRoot = proc_mkdir(PROC_ROOT_NAME, init_net.proc_net);
	if (!gprProcRoot) {
		DBGLOG(INIT, ERROR,
			"gprProcRoot == NULL\n");
		return -ENOENT;
	}
	proc_set_user(gprProcRoot, KUIDT_INIT(PROC_UID_SHELL),
		      KGIDT_INIT(PROC_GID_WIFI));

#if (BUILD_QA_DBG) || (ENABLE_DBG_LEVEL)
	prEntry =
	     proc_create(PROC_DBG_LEVEL_NAME, 0664, gprProcRoot, &dbglevel_ops);
	if (prEntry == NULL) {
		DBGLOG(INIT, ERROR,
			"Unable to create /proc entry dbgLevel\n\r");
		return -1;
	}
	proc_set_user(prEntry, KUIDT_INIT(PROC_UID_SHELL),
		      KGIDT_INIT(PROC_GID_WIFI));
#endif

#if (BUILD_QA_DBG)
#if (CFG_TWT_SMART_STA == 1)
	prEntry =
	    proc_create(PROC_TWT_SMART, 0664, gprProcRoot, &auto_twt_smart_ops);
	if (prEntry == NULL) {
		DBGLOG(INIT, ERROR,
			"Unable to create /twt smart entry %s/n",
		       PROC_TWT_SMART);
		return -1;
	}
	proc_set_user(prEntry, KUIDT_INIT(PROC_UID_SHELL),
		      KGIDT_INIT(PROC_GID_WIFI));

	g_TwtSmartStaCtrl.fgTwtSmartStaActivated = FALSE;
	g_TwtSmartStaCtrl.fgTwtSmartStaReq = FALSE;
	g_TwtSmartStaCtrl.fgTwtSmartStaTeardownReq = FALSE;
	g_TwtSmartStaCtrl.ucBssIndex = 0;
	g_TwtSmartStaCtrl.ucFlowId = 0;
	g_TwtSmartStaCtrl.u4CurTp = 0;
	g_TwtSmartStaCtrl.u4LastTp = 0;
	g_TwtSmartStaCtrl.u4TwtSwitch == 0;
	g_TwtSmartStaCtrl.eState = TWT_SMART_STA_STATE_IDLE;
#endif
#endif /* (BUILD_QA_DBG) */

	DBGLOG(INIT, INFO, "init proc fs done\n");
	return 0;
}				/* end of procInitProcfs() */

int32_t procUninitProcFs(void)
{
#if (BUILD_QA_DBG) || (ENABLE_DBG_LEVEL)
#if KERNEL_VERSION(3, 9, 0) <= LINUX_VERSION_CODE
	remove_proc_subtree(PROC_DBG_LEVEL_NAME, gprProcRoot);
#else
	remove_proc_entry(PROC_DBG_LEVEL_NAME, gprProcRoot);
#endif
#endif

#if (BUILD_QA_DBG)
#if (CFG_TWT_SMART_STA == 1)
#if KERNEL_VERSION(3, 9, 0) <= LINUX_VERSION_CODE
	remove_proc_subtree(PROC_TWT_SMART, gprProcRoot);
#else
	remove_proc_entry(PROC_TWT_SMART, gprProcRoot);
#endif
	g_TwtSmartStaCtrl.fgTwtSmartStaActivated = FALSE;
	g_TwtSmartStaCtrl.fgTwtSmartStaReq = FALSE;
	g_TwtSmartStaCtrl.fgTwtSmartStaTeardownReq = FALSE;
	g_TwtSmartStaCtrl.ucBssIndex = 0;
	g_TwtSmartStaCtrl.ucFlowId = 0;
	g_TwtSmartStaCtrl.u4CurTp = 0;
	g_TwtSmartStaCtrl.u4LastTp = 0;
	g_TwtSmartStaCtrl.u4TwtSwitch == 0;
	g_TwtSmartStaCtrl.eState = TWT_SMART_STA_STATE_IDLE;
#endif
#endif /* (BUILD_QA_DBG) */

	/*
	 * move PROC_ROOT_NAME to last since it's root directory of the others
	 * incorrect sequence would cause use-after-free error
	 */
#if KERNEL_VERSION(3, 9, 0) <= LINUX_VERSION_CODE
	remove_proc_subtree(PROC_ROOT_NAME, init_net.proc_net);
#else
	remove_proc_entry(PROC_ROOT_NAME, init_net.proc_net);
#endif

	DBGLOG(INIT, INFO, "uninit proc fs done\n");
	return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function clean up a PROC fs created by procInitProcfs().
 *
 * \param[in] prDev      Pointer to the struct net_device.
 * \param[in] pucDevName Pointer to the name of net_device.
 *
 * \return N/A
 */
/*----------------------------------------------------------------------------*/
int32_t procRemoveProcfs(void)
{
#if (!CFG_MTK_ANDROID_WMT) || (BUILD_QA_DBG)
#if CFG_SUPPORT_CSI
	struct CSI_INFO_T *prCSIInfo = NULL;

	prCSIInfo = glCsiGetCSIInfo();
	prCSIInfo->u4CSIBufferUsed = 1;
	wake_up_interruptible(&(g_prGlueInfo_proc->waitq_csi));
	remove_proc_entry(PROC_CSI_DATA_NAME, gprProcRoot);
#endif
	remove_proc_entry(PROC_DRIVER_CMD, gprProcRoot);
#endif /* (!CFG_MTK_ANDROID_WMT) || (BUILD_QA_DBG) */

#if (!CFG_MTK_ANDROID_WMT)
	remove_proc_entry(PROC_COUNTRY, gprProcRoot);
#if (CFG_CE_ASSERT_DUMP == 1)
	remove_proc_entry(PROC_CORE_DUMP, gprProcRoot);
#endif
#endif /* (!CFG_MTK_ANDROID_WMT) */

#if (!CFG_MTK_ANDROID_WMT) && (BUILD_QA_DBG)
	remove_proc_entry(PROC_EFUSE_DUMP, gprProcRoot);
	remove_proc_entry(PROC_PKT_DELAY_DBG, gprProcRoot);
#if CFG_WIFI_TXPWR_TBL_DUMP
	remove_proc_entry(PROC_GET_TXPWR_TBL, gprProcRoot);
#endif
#endif /* (!CFG_MTK_ANDROID_WMT) && (BUILD_QA_DBG) */

#if (BUILD_QA_DBG)
	remove_proc_entry(PROC_MCR_ACCESS, gprProcRoot);
	remove_proc_entry(PROC_CFG, gprProcRoot);
#endif /* (BUILD_QA_DBG) */

	DBGLOG(INIT, INFO, "remove proc fs done\n");
	return 0;
} /* end of procRemoveProcfs() */

int32_t procCreateFsEntry(struct GLUE_INFO *prGlueInfo)
{
	struct proc_dir_entry *prEntry;

	DBGLOG(INIT, TRACE, "[%s]\n", __func__);
	g_prGlueInfo_proc = prGlueInfo;
	prEntry = NULL;

#if (!CFG_MTK_ANDROID_WMT) || (BUILD_QA_DBG)
#if CFG_SUPPORT_EASY_DEBUG
	prEntry =
		proc_create(PROC_DRIVER_CMD, 0664, gprProcRoot, &drivercmd_ops);
	if (prEntry == NULL) {
		DBGLOG(INIT, ERROR,
			"Unable to create /proc entry for driver command\n\r");
		return -1;
	}
#endif

#if CFG_SUPPORT_CSI
	prEntry =
	       proc_create(PROC_CSI_DATA_NAME, 0600, gprProcRoot, &csidata_ops);
	if (prEntry == NULL) {
		DBGLOG(INIT, ERROR,
			"[CSI] Unable to create /proc entry csidata\n\r");
		return -1;
	}
#endif
#endif /* (!CFG_MTK_ANDROID_WMT) || (BUILD_QA_DBG) */

#if (!CFG_MTK_ANDROID_WMT)
	prEntry = proc_create(PROC_COUNTRY, 0664, gprProcRoot, &country_ops);
	if (prEntry == NULL) {
		DBGLOG(INIT, ERROR,
			"Unable to create /proc entry country\n\r");
		return -1;
	}

#if (CFG_CE_ASSERT_DUMP == 1)
	prEntry = proc_create(PROC_CORE_DUMP, 0664, gprProcRoot, &coredump_ops);
	if (prEntry == NULL) {
		DBGLOG(INIT, ERROR,
			"Unable to create /proc entry core_dump\n\r");
		return -1;
	}
#endif
#endif /* (!CFG_MTK_ANDROID_WMT) */

#if (!CFG_MTK_ANDROID_WMT) && (BUILD_QA_DBG)
	prEntry = proc_create(
		PROC_PKT_DELAY_DBG, 0664, gprProcRoot, &proc_pkt_delay_dbg_ops);
	if (prEntry == NULL) {
		DBGLOG(INIT, ERROR,
			"Unable to create /proc entry pktDelay\n\r");
		return -1;
	}
	proc_set_user(prEntry, KUIDT_INIT(PROC_UID_SHELL),
		      KGIDT_INIT(PROC_GID_WIFI));

#if CFG_WIFI_TXPWR_TBL_DUMP
	prEntry =
	 proc_create(PROC_GET_TXPWR_TBL, 0664, gprProcRoot, &get_txpwr_tbl_ops);
	if (prEntry == NULL) {
		DBGLOG(INIT, ERROR,
			"Unable to create /proc entry TXPWR Table\n\r");
		return -1;
	}
#endif

#if CFG_SUPPORT_EASY_DEBUG
	prEntry =
		proc_create(PROC_EFUSE_DUMP, 0664, gprProcRoot, &efusedump_ops);
	if (prEntry == NULL) {
		DBGLOG(INIT, ERROR,
			"Unable to create /proc entry efuse\n\r");
		return -1;
	}
#endif
#endif /* (!CFG_MTK_ANDROID_WMT) && (BUILD_QA_DBG) */

#if (BUILD_QA_DBG)
	prEntry = proc_create(PROC_MCR_ACCESS, 0664, gprProcRoot, &mcr_ops);
	if (prEntry == NULL) {
		DBGLOG(INIT, ERROR,
			"Unable to create /proc entry mcr\n\r");
		return -1;
	}

#if CFG_SUPPORT_EASY_DEBUG
	prEntry = proc_create(PROC_CFG, 0664, gprProcRoot, &cfg_ops);
	if (prEntry == NULL) {
		DBGLOG(INIT, ERROR,
			"Unable to create /proc entry for driver cfg\n\r");
		return -1;
	}
#endif
#endif /* ((BUILD_QA_DBG) */

	DBGLOG(INIT, INFO, "create proc fs done\n");
	return 0;
}

#ifdef FW_CFG_SUPPORT
#define MAX_CFG_OUTPUT_BUF_LENGTH 1024
static uint8_t aucCfgBuf[CMD_FORMAT_V1_LENGTH];
static uint8_t aucCfgQueryKey[MAX_CMD_NAME_MAX_LENGTH];
static uint8_t aucCfgOutputBuf[MAX_CFG_OUTPUT_BUF_LENGTH];

static ssize_t cfgRead(struct file *filp, char __user *buf, size_t count,
	loff_t *f_pos)
{
	uint32_t rStatus = WLAN_STATUS_FAILURE;
	uint8_t *temp = &aucCfgOutputBuf[0];
	uint32_t u4CopySize = 0;

	struct CMD_HEADER cmdV1Header;
	struct CMD_FORMAT_V1 *pr_cmd_v1 =
		(struct CMD_FORMAT_V1 *)cmdV1Header.buffer;

	/* if *f_pos >  0, we should return 0 to make cat command exit */
	if (*f_pos > 0 || gprGlueInfo == NULL)
		return 0;
	if (!kalStrLen(aucCfgQueryKey))
		return 0;

	kalMemSet(aucCfgOutputBuf, 0, MAX_CFG_OUTPUT_BUF_LENGTH);

	SNPRINTF(temp, sizeof(aucCfgOutputBuf) - kalStrLen(aucCfgOutputBuf),
		("\nprocCfgRead() %s:\n", aucCfgQueryKey));

	/* send to FW */
	cmdV1Header.cmdVersion = CMD_VER_1;
	cmdV1Header.cmdType = CMD_TYPE_QUERY;
	cmdV1Header.itemNum = 1;
	cmdV1Header.cmdBufferLen = sizeof(struct CMD_FORMAT_V1);
	kalMemSet(cmdV1Header.buffer, 0, MAX_CMD_BUFFER_LENGTH);

	pr_cmd_v1->itemStringLength = kalStrLen(aucCfgQueryKey);

	kalMemCopy(pr_cmd_v1->itemString, aucCfgQueryKey,
		kalStrLen(aucCfgQueryKey));

	rStatus = kalIoctl(gprGlueInfo, wlanoidQueryCfgRead,
		(void *)&cmdV1Header, sizeof(cmdV1Header), &u4CopySize);
	if (rStatus == WLAN_STATUS_FAILURE)
		DBGLOG(INIT, ERROR,
			"kalIoctl wlanoidQueryCfgRead fail 0x%x\n",
			rStatus);

	SNPRINTF(temp, sizeof(aucCfgOutputBuf) - kalStrLen(aucCfgOutputBuf),
		("%s\n", cmdV1Header.buffer));

	u4CopySize = kalStrLen(aucCfgOutputBuf);
	if (u4CopySize > count)
		u4CopySize = count;

	if (copy_to_user(buf, aucCfgOutputBuf, u4CopySize))
		DBGLOG(INIT, ERROR, "copy to user failed\n");

	*f_pos += u4CopySize;
	return (ssize_t) u4CopySize;
}

static ssize_t cfgWrite(struct file *filp, const char __user *buf,
	size_t count, loff_t *f_pos)
{
	/* echo xxx xxx > /proc/net/wlan/cfg */
	uint8_t i = 0;
	uint32_t u4CopySize = sizeof(aucCfgBuf);
	uint8_t token_num = 1;

	if (count <= 0) {
		DBGLOG(INIT, ERROR, "wrong copy size\n");
		return -EFAULT;
	}

	kalMemSet(aucCfgBuf, 0, u4CopySize);
	u4CopySize = (count < u4CopySize) ? count : (u4CopySize - 1);

	if (copy_from_user(aucCfgBuf, buf, u4CopySize)) {
		DBGLOG(INIT, ERROR, "copy from user failed\n");
		return -EFAULT;
	}
	aucCfgBuf[u4CopySize] = '\0';
	for (i = 0; i < u4CopySize; i++) {
		if (aucCfgBuf[i] == ' ') {
			token_num++;
			break;
		}
	}

	if (token_num == 1) {
		kalMemSet(aucCfgQueryKey, 0, sizeof(aucCfgQueryKey));
		u4CopySize = (u4CopySize < sizeof(aucCfgQueryKey)) ?
			u4CopySize : sizeof(aucCfgQueryKey);

		/* remove the 0x0a */
		memcpy(aucCfgQueryKey, aucCfgBuf, u4CopySize);
		if (aucCfgQueryKey[u4CopySize - 1] == 0x0a)
			aucCfgQueryKey[u4CopySize - 1] = '\0';
	} else {
		wlanFwCfgParse(gprGlueInfo->prAdapter, aucCfgBuf);
	}

	return count;
}
#if KERNEL_VERSION(5, 6, 0) <= CFG80211_VERSION_CODE
static const struct proc_ops fwcfg_ops = {
	.proc_read = cfgRead,
	.proc_write = cfgWrite,
};
#else
static const struct file_operations fwcfg_ops = {
	.owner = THIS_MODULE,
	.read = cfgRead,
	.write = cfgWrite,
};
#endif
int32_t cfgRemoveProcEntry(void)
{
	remove_proc_entry(PROC_CFG_NAME, gprProcRoot);
	DBGLOG(INIT, INFO, "cfg remove done\n", __func__);
	return 0;
}

int32_t cfgCreateProcEntry(struct GLUE_INFO *prGlueInfo)
{
	struct proc_dir_entry *prEntry;

	prGlueInfo->pProcRoot = gprProcRoot;
	gprGlueInfo = prGlueInfo;

	prEntry = proc_create(PROC_CFG_NAME, 0664, gprProcRoot, &fwcfg_ops);
	if (prEntry == NULL) {
		DBGLOG(INIT, ERROR, "Unable to create /proc entry cfg\n\r");
		return -1;
	}
	proc_set_user(prEntry, KUIDT_INIT(PROC_UID_SHELL),
		KGIDT_INIT(PROC_GID_WIFI));

	DBGLOG(INIT, INFO, "cfg create done\n", __func__);
	return 0;
}
#endif
