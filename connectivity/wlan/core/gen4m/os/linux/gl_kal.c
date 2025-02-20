// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   gl_kal.c
 *    \brief  GLUE Layer will export the required procedures here for internal
 *            driver stack.
 *
 *    This file contains all routines which are exported from GLUE Layer to
 *    internal driver stack.
 */


/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "gl_os.h"
#include "gl_kal.h"
#include "gl_wext.h"
#include "precomp.h"
#if KERNEL_VERSION(5, 16, 0) > LINUX_VERSION_CODE
#include <stdarg.h>
#endif

#if CFG_SUPPORT_AGPS_ASSIST
#include <net/netlink.h>
#endif

#if CFG_TC1_FEATURE
#include <tc1_partition.h>
#endif

/* for rps */
#include <linux/netdevice.h>
#if KERNEL_VERSION(6, 8, 0) <= CFG80211_VERSION_CODE
#include <net/rps.h>
#endif
#if KERNEL_VERSION(6, 6, 0) <= LINUX_VERSION_CODE
#include <net/netdev_rx_queue.h>
#endif
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/ipv6.h>
#include <net/ipv6.h>
#include <net/sch_generic.h>
#include <linux/skbuff.h>
#include <linux/module.h>
#include <linux/debugfs.h>
#if CFG_SUPPORT_THERMAL_QUERY
#include <linux/thermal.h>
#include "thermal_core.h"
#endif

#include <linux/platform_device.h>

/* for uevent */
#include <linux/miscdevice.h>   /* for misc_register, and SYNTH_MINOR */
#include <linux/kobject.h>

/* for wifi standalone log */
#if CFG_SUPPORT_SA_LOG
#include "gl_sa_log.h"
#include <linux/jiffies.h>
#include <linux/ratelimit.h>
#include <linux/rtc.h>
#include <linux/sched/clock.h>
#endif

/* for ATF smc call */
#if (CFG_WLAN_ATF_SUPPORT == 1)
#include <linux/arm-smccc.h>
#include <linux/soc/mediatek/mtk_sip_svc.h>
#endif

#if (CFG_SUPPORT_TX_PWR_ENV == 1)
#include "rlm.h"
#endif
#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
#include "rlm_domain.h"
#endif

#if CFG_MTK_MDDP_SUPPORT
#include "mddp.h"
#endif

#if CFG_SUPPORT_TPUT_FACTOR
#if KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
#include <linux/cpufreq.h>
#endif
#endif

extern void set_logtoomuch_enable(int value) __attribute__((weak));
extern int get_logtoomuch_enable(void) __attribute__((weak));
extern uint32_t get_wifi_standalone_log_mode(void) __attribute__((weak));

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
/* the maximum length of a file name */
#define FILE_NAME_MAX CFG_FW_NAME_MAX_LEN

/* the maximum number of all possible file name */
#define FILE_NAME_TOTAL 8

#if CFG_SUPPORT_NAN
/* Protocol family, consistent in both kernel prog and user prog. */
#define MTKPROTO 25
/* Multicast group, consistent in both kernel prog and user prog. */
#define MTKGRP 22
#endif

#if CFG_MODIFY_TX_POWER_BY_BAT_VOLT
#define BACKOFF_VOLT 3550
#define RESTORE_VOLT 3750
#endif

#if CFG_SUPPORT_TPUT_FACTOR
#define CPU_CNT 8
/* current max CPU count */
#define CPU_LOG_LEN 8
/* the max string length of CPU frequency log ex: "3050000 " */
#define PWR_MODE_LOG_HEAD_LEN 8
/* the max string length of pwr mode header log ex: "PwrMode:" */
#define PWR_MODE_LOG_LEN 8
/* the max string length of pwr mode log ex: "2," */
#endif

static uint8_t aucBandTranslate[BAND_NUM] = {
	KAL_BAND_2GHZ,
	KAL_BAND_2GHZ,
	KAL_BAND_5GHZ
#if (CFG_SUPPORT_WIFI_6G == 1)
	,
	KAL_BAND_6GHZ
#endif
};

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */
#if DBG
int allocatedMemSize;
#endif

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */
 #define RECORDKEY_BASE  0xFF
static void *pvIoBuffer;
static uint32_t pvIoBufferSize;
static uint32_t pvIoBufferUsage;

static struct KAL_HALT_CTRL_T rHaltCtrl = {
	.lock = __SEMAPHORE_INITIALIZER(rHaltCtrl.lock, 1),
	.owner = NULL,
	.fgHalt = TRUE,
	.fgHeldByKalIoctl = FALSE,
	.u4HoldStart = 0,
};
/* framebuffer callback related variable and status flag */
u_int8_t wlan_fb_power_down = FALSE;
#if CFG_MODIFY_TX_POWER_BY_BAT_VOLT
void *wlan_bat_volt_notifier_priv_data;
unsigned int wlan_bat_volt;
bool fgIsTxPowerDecreased = FALSE;
#endif

#if CFG_FORCE_ENABLE_PERF_MONITOR
u_int8_t wlan_perf_monitor_force_enable = TRUE;
#else
u_int8_t wlan_perf_monitor_force_enable = FALSE;
#endif

static int wlan_fb_notifier_callback(struct notifier_block
				*self, unsigned long event, void *data);

void *wlan_fb_notifier_priv_data;
#if IS_ENABLED(CONFIG_DRM_MEDIATEK_V2)
static struct notifier_block wlan_fb_notifier = {
	.notifier_call = wlan_fb_notifier_callback
};
#endif

static struct miscdevice wlan_object;

#if CFG_SUPPORT_SA_LOG
static unsigned long rtc_update;
#endif

#if (CFG_VOLT_INFO == 1)
static struct VOLT_INFO_T _rVnfInfo = {
	.u4CurrVolt = 0,
	.rDebParam.u4Total = 0,
	.rDebParam.u4Cnt = 0,
	.rBatNotify.lbat_pt = NULL,
	.rBatNotify.fgReg = FALSE,
	.prAdapter = NULL,
	.eState = VOLT_INFO_STATE_INIT
};
#endif /* CFG_VOLT_INFO */

#if CFG_SUPPORT_PCIE_GEN_SWITCH
uint32_t pcie_monitor_count;
#endif

/* kalSendUevent */
struct MSG_UEVENT_REQ {
	struct MSG_HDR rMsgHdr; /* Must be the first member */
	char event_string[300];
};

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

static uint32_t kalPerMonUpdate(struct ADAPTER *prAdapter);
static int32_t kalWorkGetCpu(struct GLUE_INFO *pr,
	enum ENUM_WORK eWork);

#if CFG_SUPPORT_SKIP_RX_GRO_FOR_TC
static u_int8_t kalGetSkipRxGro(struct net_device *prNetDev);
static void kalRxGroTcCheck(struct GLUE_INFO *glue);
#endif /* CFG_SUPPORT_SKIP_RX_GRO_FOR_TC */

#if CFG_SUPPORT_RX_NAPI
static void kalNapiScheduleCheck(struct GLUE_INFO *pr);
#if CFG_SUPPORT_RX_WORK
static void kalNapiWakeup(void);
#endif /* CFG_SUPPORT_RX_WORK */
#if CFG_NAPI_DELAY
static void kalNapiDelayTimerInit(struct GLUE_INFO *pr);
static void kalNapiDelayTimerUninit(struct GLUE_INFO *pr);
static void kalNapiDelayCheck(struct GLUE_INFO *pr);
#endif /* CFG_NAPI_DELAY */
#endif /* CFG_SUPPORT_RX_NAPI */

#if CFG_SUPPORT_TPUT_FACTOR
void kalTputFactorUpdate(struct ADAPTER *prAdapter);
#endif


/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
#if CFG_ENABLE_FW_DOWNLOAD

#if (defined(CONFIG_UIDGID_STRICT_TYPE_CHECKS) || \
	(KERNEL_VERSION(3, 14, 0) <= LINUX_VERSION_CODE))
#define  KUIDT_VALUE(v) (v.val)
#define  KGIDT_VALUE(v) (v.val)
#else
#define  KUIDT_VALUE(v) v
#define  KGIDT_VALUE(v) v
#endif

const struct firmware *wlan_fw_entry;

/* Default */
static uint8_t *apucFwName[] = {
	(uint8_t *) CFG_FW_FILENAME "_MT",
	(uint8_t *) CFG_FW_FILENAME "_",
	NULL
};

static uint8_t *apucCr4FwName[] = {
	(uint8_t *) CFG_CR4_FW_FILENAME "_MT",
	NULL
};

inline uint32_t kalRoundUpPowerOf2(uint32_t v)
{
	/* v is already a power of 2, or v is 0 */
	if (v && !(v & (v - 1)))
		return v;

	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;

	return v;
}

#if (CONFIG_WLAN_DRV_BUILD_IN == 0) && (BUILD_QA_DBG == 1)
/*----------------------------------------------------------------------------*/
/*!
 * \brief  To leverage systrace, use the same name, i.e. tracing_mark_write,
 *         which ftrace would write when systrace writes msg to
 *         /sys/kernel/debug/tracing/trace_marker with trace_options enabling
 *         print-parent
 *
 */
/*----------------------------------------------------------------------------*/
void tracing_mark_write(const char *fmt, ...)
{
#define __BUFFER_SIZE 1024
	va_list ap;
	char buf[__BUFFER_SIZE];
	int retval;

	if ((aucDebugModule[DBG_TRACE_IDX] & DBG_CLASS_TEMP) == 0)
		return;

	va_start(ap, fmt);
	retval = vsnprintf(buf, __BUFFER_SIZE, fmt, ap);
	if (retval < 0)
		LOG_FUNC("[%u] vsnprintf failed, ret: %d",
		__LINE__, retval);
	buf[__BUFFER_SIZE - 1] = '\0';
	va_end(ap);

#undef __BUFFER_SIZE

	trace_printk("%s", buf);
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is provided by GLUE Layer for internal driver stack to
 *        open firmware image in kernel space
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval WLAN_STATUS_SUCCESS.
 * \retval WLAN_STATUS_FAILURE.
 *
 */
/*----------------------------------------------------------------------------*/
uint32_t kalFirmwareOpen(struct GLUE_INFO *prGlueInfo,
			 uint8_t **apucNameTable)
{
	uint8_t ucNameIdx;
	/* PPUINT_8 apucNameTable; */
	uint8_t ucCurEcoVer = wlanGetEcoVersion(
				      prGlueInfo->prAdapter);
	u_int8_t fgResult = FALSE;
	u_int8_t fgNoMem = FALSE;
	int ret;

	/* Try to open FW binary */
	for (ucNameIdx = 0; apucNameTable[ucNameIdx]; ucNameIdx++) {
		/*
		 * Driver support request_firmware() to get files
		 * Android path: "/etc/firmware", "/vendor/firmware",
		 *               "/firmware/image"
		 * Linux path: "/lib/firmware", "/lib/firmware/update"
		 */
		ret = _kalRequestFirmware(&wlan_fw_entry,
				       apucNameTable[ucNameIdx],
				       prGlueInfo->prDev);

		if (ret) {
			DBGLOG(INIT, TRACE,
			       "Request FW image: %s failed, errno[%d][%d]\n",
			       apucNameTable[ucNameIdx], fgResult, ret);
			if (ret == -ENOMEM)
				fgNoMem = TRUE;
			continue;
		} else {
			DBGLOG(INIT, INFO, "Request FW image: %s done\n",
			       apucNameTable[ucNameIdx]);
			fgResult = TRUE;
			break;
		}
	}

	/* Check result */
	if (!fgResult)
		goto error_open;

	return WLAN_STATUS_SUCCESS;

error_open:
	DBGLOG(INIT, ERROR,
		"Request FW image failed! Cur ECO Ver[E%u]\n",
		ucCurEcoVer);
	if (fgNoMem)
		kalSendAeeWarning("WLAN", "request firmware failed\n");

	return WLAN_STATUS_FAILURE;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is provided by GLUE Layer for internal driver stack to
 *        release firmware image in kernel space
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval WLAN_STATUS_SUCCESS.
 * \retval WLAN_STATUS_FAILURE.
 *
 */
/*----------------------------------------------------------------------------*/
uint32_t kalFirmwareClose(struct GLUE_INFO *prGlueInfo)
{
	release_firmware(wlan_fw_entry);

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is provided by GLUE Layer for internal driver stack to
 *        load firmware image in kernel space
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval WLAN_STATUS_SUCCESS.
 * \retval WLAN_STATUS_FAILURE.
 *
 */
/*----------------------------------------------------------------------------*/
uint32_t kalFirmwareLoad(struct GLUE_INFO *prGlueInfo,
			 void *prBuf, uint32_t u4Offset,
			 uint32_t *pu4Size)
{
	ASSERT(prGlueInfo);
	ASSERT(pu4Size);
	ASSERT(prBuf);

	if ((wlan_fw_entry == NULL) || (wlan_fw_entry->size == 0)
	    || (wlan_fw_entry->data == NULL)) {
		goto error_read;
	} else {
		memcpy(prBuf, wlan_fw_entry->data, wlan_fw_entry->size);
		*pu4Size = wlan_fw_entry->size;
	}

	return WLAN_STATUS_SUCCESS;

error_read:
	return WLAN_STATUS_FAILURE;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is provided by GLUE Layer for internal driver stack to
 *        query firmware image size in kernel space
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval WLAN_STATUS_SUCCESS.
 * \retval WLAN_STATUS_FAILURE.
 *
 */
/*----------------------------------------------------------------------------*/

uint32_t kalFirmwareSize(struct GLUE_INFO *prGlueInfo,
			 uint32_t *pu4Size)
{
	ASSERT(prGlueInfo);
	ASSERT(pu4Size);

	*pu4Size = wlan_fw_entry->size;

	return WLAN_STATUS_SUCCESS;
}

void
kalConstructDefaultFirmwarePrio(struct GLUE_INFO
				*prGlueInfo, uint8_t **apucNameTable,
				uint8_t **apucName, uint8_t *pucNameIdx,
				uint8_t ucMaxNameIdx)
{
	struct mt66xx_chip_info *prChipInfo =
			prGlueInfo->prAdapter->chip_info;
	uint32_t chip_id = prChipInfo->chip_id;
	uint8_t sub_idx = 0;
	int ret = 0;

	for (sub_idx = 0; apucNameTable[sub_idx]; sub_idx++) {
		if ((*pucNameIdx + 3) >= ucMaxNameIdx) {
			/* the table is not large enough */
			DBGLOG(INIT, ERROR,
			       "kalFirmwareImageMapping >> file name array is not enough.\n");
			ASSERT(0);
			continue;
		}

		/* Type 1. WIFI_RAM_CODE_MTxxxx */
		ret = kalSnprintf(*(apucName + (*pucNameIdx)), FILE_NAME_MAX,
			"%s%x", apucNameTable[sub_idx], chip_id);
		if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
			(*pucNameIdx) += 1;
		else
			DBGLOG(INIT, ERROR,
					"[%u] kalSnprintf failed, ret: %d\n",
					__LINE__, ret);

		/* Type 2. WIFI_RAM_CODE_MTxxxx.bin */
		ret = kalSnprintf(*(apucName + (*pucNameIdx)), FILE_NAME_MAX,
			 "%s%x.bin",
			 apucNameTable[sub_idx], chip_id);
		if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
			(*pucNameIdx) += 1;
		else
			DBGLOG(INIT, ERROR,
					"[%u] kalSnprintf failed, ret: %d\n",
					__LINE__, ret);

		/* Type 3. WIFI_RAM_CODE_MTxxxx_Ex */
		ret = kalSnprintf(*(apucName + (*pucNameIdx)), FILE_NAME_MAX,
			 "%s%x_E%u",
			 apucNameTable[sub_idx], chip_id,
			 wlanGetEcoVersion(prGlueInfo->prAdapter));
		if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
			(*pucNameIdx) += 1;
		else
			DBGLOG(INIT, ERROR,
					"[%u] kalSnprintf failed, ret: %d\n",
					__LINE__, ret);

		/* Type 4. WIFI_RAM_CODE_MTxxxx_Ex.bin */
		ret = kalSnprintf(*(apucName + (*pucNameIdx)), FILE_NAME_MAX,
			 "%s%x_E%u.bin",
			 apucNameTable[sub_idx], chip_id,
				 wlanGetEcoVersion(prGlueInfo->prAdapter));
		if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
			(*pucNameIdx) += 1;
		else
			DBGLOG(INIT, ERROR,
					"[%u] kalSnprintf failed, ret: %d\n",
					__LINE__, ret);
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to load firmware image
 *
 * \param pvGlueInfo     Pointer of GLUE Data Structure
 * \param ppvMapFileBuf  Pointer of pointer to memory-mapped firmware image
 * \param pu4FileLength  File length and memory mapped length as well
 *
 * \retval Map File Handle, used for unammping
 */
/*----------------------------------------------------------------------------*/

void *
kalFirmwareImageMapping(struct GLUE_INFO *prGlueInfo,
			void **ppvMapFileBuf, uint32_t *pu4FileLength,
			enum ENUM_IMG_DL_IDX_T eDlIdx)
{
	uint8_t **apucNameTable = NULL;
	uint8_t *apucName[FILE_NAME_TOTAL +
					  1]; /* extra +1, for the purpose of
					       * detecting the end of the array
					       */
	uint8_t idx = 0, max_idx, ucRomVer = 0,
		aucNameBody[FILE_NAME_TOTAL][FILE_NAME_MAX], sub_idx = 0;
	struct mt66xx_chip_info *prChipInfo =
			prGlueInfo->prAdapter->chip_info;
	uint32_t chip_id = prChipInfo->chip_id;
	void *prFwBuffer = NULL;
	uint32_t u4FwSize = 0;

	ASSERT(prGlueInfo);
	ASSERT(ppvMapFileBuf);
	ASSERT(pu4FileLength);

	*ppvMapFileBuf = NULL;
	*pu4FileLength = 0;

	do {
		/* <0.0> Get FW name prefix table */
		switch (eDlIdx) {
		case IMG_DL_IDX_N9_FW:
			apucNameTable = apucFwName;
			break;

		case IMG_DL_IDX_CR4_FW:
			apucNameTable = apucCr4FwName;
			break;

		case IMG_DL_IDX_PATCH:
			break;

		case IMG_DL_IDX_MCU_ROM_EMI:
			break;

		case IMG_DL_IDX_WIFI_ROM_EMI:
			break;

#if CFG_SUPPORT_WIFI_DL_BT_PATCH
		case IMG_DL_IDX_BT_PATCH:
			break;
#endif
#if CFG_SUPPORT_WIFI_DL_ZB_PATCH
		case IMG_DL_IDX_ZB_PATCH:
			break;
#endif
		case IMG_DL_IDX_DSP_FW:
			break;
		default:
			ASSERT(0);
			break;
		}

		/* <0.2> Construct FW name */
		memset(apucName, 0, sizeof(apucName));

		/* magic number 1: reservation for detection
		 * of the end of the array
		 */
		max_idx = ARRAY_SIZE(apucName) - 1;

		idx = 0;
		apucName[idx] = (uint8_t *)(aucNameBody + idx);

		if (eDlIdx == IMG_DL_IDX_PATCH) {
			/* construct the file name for patch */
			for (sub_idx = 0; sub_idx < max_idx; sub_idx++)
				apucName[sub_idx] =
					(uint8_t *)(aucNameBody + sub_idx);

			/* mtxxxx_patch_ex_hdr.bin*/
			if (prChipInfo->fw_dl_ops->constructPatchName)
				prChipInfo->fw_dl_ops->constructPatchName(
					prGlueInfo, apucName, &idx);
			else {
				ucRomVer = wlanGetRomVersion(
						prGlueInfo->prAdapter) + 1;
				kalSnprintf(apucName[idx], FILE_NAME_MAX,
					"mt%x_patch_e%x_hdr.bin", chip_id,
					ucRomVer);
			}
			idx += 1;
#if CFG_SUPPORT_WIFI_DL_BT_PATCH
		} else if (eDlIdx == IMG_DL_IDX_BT_PATCH) {
			if (prChipInfo->fw_dl_ops->constructBtPatchName) {
				prChipInfo->fw_dl_ops->constructBtPatchName(
					prGlueInfo, apucName, &idx);
			} else {
				DBGLOG(INIT, ERROR, "No BT PATCH Name!\n");
				return NULL;
			}
			idx += 1;
#endif /* CFG_SUPPORT_WIFI_DL_BT_PATCH */
#if CFG_SUPPORT_WIFI_DL_ZB_PATCH
		} else if (eDlIdx == IMG_DL_IDX_ZB_PATCH) {
			if (prChipInfo->fw_dl_ops->constructZbPatchName) {
				prChipInfo->fw_dl_ops->constructZbPatchName(
					prGlueInfo, apucName, &idx);
			} else {
				DBGLOG(INIT, ERROR, "No Zb PATCH Name!\n");
				return NULL;
			}
			idx += 1;
#endif /* CFG_SUPPORT_WIFI_DL_ZB_PATCH */
		} else if (eDlIdx == IMG_DL_IDX_MCU_ROM_EMI ||
			   eDlIdx == IMG_DL_IDX_WIFI_ROM_EMI) {
			if (prChipInfo->fw_dl_ops->constructRomName)
				prChipInfo->fw_dl_ops->constructRomName(
					prGlueInfo, eDlIdx, apucName, &idx);
		} else if (eDlIdx == IMG_DL_IDX_DSP_FW) {
			if (prChipInfo->fw_dl_ops->constructDspName) {
				prChipInfo->fw_dl_ops->constructDspName(
					prGlueInfo, apucName, &idx);
			}
		} else {
			for (sub_idx = 0; sub_idx < max_idx; sub_idx++)
				apucName[sub_idx] =
					(uint8_t *)(aucNameBody + sub_idx);

			if (prChipInfo->fw_dl_ops->constructFirmwarePrio)
				prChipInfo->fw_dl_ops->constructFirmwarePrio(
					prGlueInfo, apucNameTable, apucName,
					&idx, max_idx);
			else
				kalConstructDefaultFirmwarePrio(
					prGlueInfo, apucNameTable, apucName,
					&idx, max_idx);
		}

		/* let the last pointer point to NULL
		 * so that we can detect the end of the array in
		 * kalFirmwareOpen().
		 */
		apucName[idx] = NULL;

		apucNameTable = apucName;

		/* <1> Open firmware */
		if (kalFirmwareOpen(prGlueInfo,
			apucNameTable) == WLAN_STATUS_SUCCESS)
		{
			/* <2> Query firmare size */
			kalFirmwareSize(prGlueInfo, &u4FwSize);
			/* <3> Use vmalloc for allocating large memory trunk */
			prFwBuffer = vmalloc(ALIGN_4(u4FwSize));
			if (!prFwBuffer) {
				DBGLOG(INIT, ERROR, "vmalloc(%u) failed\n",
					ALIGN_4(u4FwSize));
				kalFirmwareClose(prGlueInfo);
				break;
			}
			/* <4> Load image binary into buffer */
			if (kalFirmwareLoad(prGlueInfo, prFwBuffer, 0,
					    &u4FwSize) != WLAN_STATUS_SUCCESS) {
				vfree(prFwBuffer);
				kalFirmwareClose(prGlueInfo);
				break;
			}
			/* <5> write back info */
#if CFG_SUPPORT_SINGLE_FW_BINARY
			if (prChipInfo->fw_dl_ops->parseSingleBinaryFile &&
				prChipInfo->fw_dl_ops->parseSingleBinaryFile(
					prFwBuffer,
					u4FwSize,
					ppvMapFileBuf,
					pu4FileLength,
					eDlIdx) == WLAN_STATUS_SUCCESS) {
				vfree(prFwBuffer);
			} else {
				*ppvMapFileBuf = prFwBuffer;
				*pu4FileLength = u4FwSize;
			}
#else
			*ppvMapFileBuf = prFwBuffer;
			*pu4FileLength = u4FwSize;
#endif
			return prFwBuffer;
		}
	} while (FALSE);

	return NULL;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to unload firmware image mapped memory
 *
 * \param pvGlueInfo     Pointer of GLUE Data Structure
 * \param pvFwHandle     Pointer to mapping handle
 * \param pvMapFileBuf   Pointer to memory-mapped firmware image
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/

void kalFirmwareImageUnmapping(struct GLUE_INFO
		       *prGlueInfo, void *prFwHandle, void *pvMapFileBuf)
{
	ASSERT(prGlueInfo);

	/* pvMapFileBuf might be NULL when file doesn't exist */
	if (pvMapFileBuf)
		vfree(pvMapFileBuf);

	kalFirmwareClose(prGlueInfo);
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is provided by GLUE Layer for internal driver stack to
 *        acquire OS SPIN_LOCK.
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 * \param[in] rLockCategory  Specify which SPIN_LOCK
 * \param[out] pu4Flags      Pointer of a variable for saving IRQ flags
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void kalAcquireSpinLock(struct GLUE_INFO *prGlueInfo,
			enum ENUM_SPIN_LOCK_CATEGORY_E rLockCategory,
			unsigned long *plFlags)
{
	unsigned long ulFlags = 0;

	ASSERT(prGlueInfo);
	ASSERT(plFlags);

	if (rLockCategory < SPIN_LOCK_NUM) {
		/*DBGLOG(INIT, LOUD, "SPIN_LOCK[%u] Acq\n", rLockCategory);*/
#if CFG_USE_SPIN_LOCK_BOTTOM_HALF
		spin_lock_bh(&prGlueInfo->rSpinLock[rLockCategory]);
#else /* !CFG_USE_SPIN_LOCK_BOTTOM_HALF */
		spin_lock_irqsave(&prGlueInfo->rSpinLock[rLockCategory],
				  ulFlags);
#endif /* !CFG_USE_SPIN_LOCK_BOTTOM_HALF */

		*plFlags = ulFlags;

		/*DBGLOG(INIT, LOUD, "SPIN_LOCK[%u] Acqed\n", rLockCategory);*/
	}

}				/* end of kalAcquireSpinLock() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is provided by GLUE Layer for internal driver stack to
 *        acquire OS SPIN_LOCK_BH.
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 * \param[in] rLockCategory  Specify which SPIN_LOCK
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void kalAcquireSpinLockBh(struct GLUE_INFO *prGlueInfo,
			 enum ENUM_SPIN_LOCK_CATEGORY_E rLockCategory)
{

	if (prGlueInfo == NULL)
		DBGLOG(INIT, ERROR, "prGlueInfo NULL\n");
	else {
		if (rLockCategory < SPIN_LOCK_NUM)
			spin_lock_bh(&prGlueInfo->rSpinLock[rLockCategory]);
	}
}

void kalAcquireSpinLockIrq(struct GLUE_INFO *prGlueInfo,
			enum ENUM_SPIN_LOCK_CATEGORY_E rLockCategory,
			unsigned long *plFlags)
{
	unsigned long ulFlags = 0;

	if (prGlueInfo == NULL || plFlags == NULL)
		DBGLOG(INIT, ERROR, "prGlueInfo/plFlags NULL\n");
	else {
		if (rLockCategory < SPIN_LOCK_NUM) {
			spin_lock_irqsave(&prGlueInfo->rSpinLock[rLockCategory],
					  ulFlags);
			*plFlags = ulFlags;
		}
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is provided by GLUE Layer for internal driver stack to
 *        release OS SPIN_LOCK.
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 * \param[in] rLockCategory  Specify which SPIN_LOCK
 * \param[in] u4Flags        Saved IRQ flags
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void kalReleaseSpinLock(struct GLUE_INFO *prGlueInfo,
			enum ENUM_SPIN_LOCK_CATEGORY_E rLockCategory,
			unsigned long ulFlags)
{
	ASSERT(prGlueInfo);

	if (rLockCategory < SPIN_LOCK_NUM) {

#if CFG_USE_SPIN_LOCK_BOTTOM_HALF
		spin_unlock_bh(&prGlueInfo->rSpinLock[rLockCategory]);
#else /* !CFG_USE_SPIN_LOCK_BOTTOM_HALF */
		spin_unlock_irqrestore(
			&prGlueInfo->rSpinLock[rLockCategory], ulFlags);
#endif /* !CFG_USE_SPIN_LOCK_BOTTOM_HALF */
		/* DBGLOG(INIT, LOUD, "SPIN_UNLOCK[%u]\n", rLockCategory); */
	}

}				/* end of kalReleaseSpinLock() */


/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is provided by GLUE Layer for internal driver stack to
 *        release OS SPIN_LOCK_BH.
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 * \param[in] rLockCategory  Specify which SPIN_LOCK
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/

void kalReleaseSpinLockBh(struct GLUE_INFO *prGlueInfo,
			 enum ENUM_SPIN_LOCK_CATEGORY_E rLockCategory)
{
	if (prGlueInfo == NULL)
		DBGLOG(INIT, ERROR, "prGlueInfo NULL\n");
	else {
		if (rLockCategory < SPIN_LOCK_NUM)
			spin_unlock_bh(&prGlueInfo->rSpinLock[rLockCategory]);
	}
}

void kalReleaseSpinLockIrq(struct GLUE_INFO *prGlueInfo,
			enum ENUM_SPIN_LOCK_CATEGORY_E rLockCategory,
			unsigned long ulFlags)
{
	if (prGlueInfo == NULL)
		DBGLOG(INIT, ERROR, "prGlueInfo NULL\n");
	else {
		if (rLockCategory < SPIN_LOCK_NUM)
			spin_unlock_irqrestore(
				&prGlueInfo->rSpinLock[rLockCategory], ulFlags);
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is provided by GLUE Layer for internal driver stack to
 *        acquire OS MUTEX.
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 * \param[in] rMutexCategory  Specify which MUTEX
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void kalAcquireMutex(struct GLUE_INFO *prGlueInfo,
		     enum ENUM_MUTEX_CATEGORY_E rMutexCategory)
{
	ASSERT(prGlueInfo);

	if (rMutexCategory < MUTEX_NUM) {
		DBGLOG(INIT, TEMP,
			"MUTEX_LOCK[%u] Try to acquire\n", rMutexCategory);
		mutex_lock(&prGlueInfo->arMutex[rMutexCategory]);
		DBGLOG(INIT, TEMP, "MUTEX_LOCK[%u] Acquired\n", rMutexCategory);
	}

}				/* end of kalAcquireMutex() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is provided by GLUE Layer for internal driver stack to
 *        release OS MUTEX.
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 * \param[in] rMutexCategory  Specify which MUTEX
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void kalReleaseMutex(struct GLUE_INFO *prGlueInfo,
		     enum ENUM_MUTEX_CATEGORY_E rMutexCategory)
{
	ASSERT(prGlueInfo);

	if (rMutexCategory < MUTEX_NUM) {
		mutex_unlock(&prGlueInfo->arMutex[rMutexCategory]);
		DBGLOG(INIT, TEMP, "MUTEX_UNLOCK[%u]\n", rMutexCategory);
	}

}				/* end of kalReleaseMutex() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is provided by GLUE Layer for internal driver stack to
 *        update current MAC address.
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 * \param[in] pucMacAddr     Pointer of current MAC address
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void kalUpdateMACAddress(struct GLUE_INFO *prGlueInfo,
			 uint8_t *pucMacAddr)
{
	ASSERT(prGlueInfo);
	ASSERT(pucMacAddr);

	DBGLOG(INIT, INFO,
			MACSTR ", " MACSTR ".\n",
			MAC2STR(prGlueInfo->prDevHandler->dev_addr),
			MAC2STR(pucMacAddr));

	if (UNEQUAL_MAC_ADDR(prGlueInfo->prDevHandler->dev_addr,
			     pucMacAddr))
#if (KERNEL_VERSION(5, 16, 0) <= LINUX_VERSION_CODE)
		eth_hw_addr_set(prGlueInfo->prDevHandler, pucMacAddr);
#else
		memcpy(prGlueInfo->prDevHandler->dev_addr, pucMacAddr,
		       PARAM_MAC_ADDR_LEN);
#endif
}

#if CFG_TCP_IP_CHKSUM_OFFLOAD
/*----------------------------------------------------------------------------*/
/*!
 * \brief To query the packet information for offload related parameters.
 *
 * \param[in] pvPacket Pointer to the packet descriptor.
 * \param[in] pucFlag  Points to the offload related parameter.
 *
 * \return (none)
 *
 */
/*----------------------------------------------------------------------------*/
void kalQueryTxChksumOffloadParam(void *pvPacket,
				  uint8_t *pucFlag)
{
#if (CFG_FORCE_HW_CHECKSUM == 0)
	struct sk_buff *skb = (struct sk_buff *)pvPacket;
#endif
	uint8_t ucFlag = 0;

	ASSERT(pvPacket);
	ASSERT(pucFlag);

#if (CFG_FORCE_HW_CHECKSUM == 1)
	ucFlag |= TX_CS_IP_GEN | TX_CS_TCP_UDP_GEN;
#else
	if (skb->ip_summed == CHECKSUM_PARTIAL) {
#if DBG
		/* Kevin: do double check, we can remove this part in Normal
		 * Driver.
		 * Because we register NIC feature with NETIF_F_IP_CSUM for
		 * MT5912B MAC, so we'll process IP packet only.
		 */
		if (skb->protocol != htons(ETH_P_IP)) {

		} else
#endif
			ucFlag |= (TX_CS_IP_GEN | TX_CS_TCP_UDP_GEN);
	}
#endif

	*pucFlag = ucFlag;
}

/* 4 2007/10/8, mikewu, this is rewritten by Mike */
/*----------------------------------------------------------------------------*/
/*!
 * \brief To update the checksum offload status to the packet to be indicated to
 *        OS.
 *
 * \param[in] pvPacket Pointer to the packet descriptor.
 * \param[in] pucFlag  Points to the offload related parameter.
 *
 * \return (none)
 *
 */
/*----------------------------------------------------------------------------*/
void kalUpdateRxCSUMOffloadParam(void *pvPacket,
				 enum ENUM_CSUM_RESULT aeCSUM[])
{
	struct sk_buff *skb = (struct sk_buff *)pvPacket;

	ASSERT(pvPacket);

	if ((aeCSUM[CSUM_TYPE_IPV4] == CSUM_RES_SUCCESS ||
	     aeCSUM[CSUM_TYPE_IPV6] == CSUM_RES_SUCCESS) &&
	    (aeCSUM[CSUM_TYPE_TCP] == CSUM_RES_SUCCESS ||
	     aeCSUM[CSUM_TYPE_UDP] == CSUM_RES_SUCCESS)) {
		skb->ip_summed = CHECKSUM_UNNECESSARY;
	} else {
		skb->ip_summed = CHECKSUM_NONE;
#if DBG
		if (aeCSUM[CSUM_TYPE_IPV4] == CSUM_RES_NONE
		    && aeCSUM[CSUM_TYPE_IPV6] == CSUM_RES_NONE)
			DBGLOG(RX, TRACE, "RX: \"non-IPv4/IPv6\" Packet\n");
		else if (aeCSUM[CSUM_TYPE_IPV4] == CSUM_RES_FAILED)
			DBGLOG(RX, TRACE, "RX: \"bad IP Checksum\" Packet\n");
		else if (aeCSUM[CSUM_TYPE_TCP] == CSUM_RES_FAILED)
			DBGLOG(RX, TRACE, "RX: \"bad TCP Checksum\" Packet\n");
		else if (aeCSUM[CSUM_TYPE_UDP] == CSUM_RES_FAILED)
			DBGLOG(RX, TRACE, "RX: \"bad UDP Checksum\" Packet\n");
#endif
	}
}
#endif /* CFG_TCP_IP_CHKSUM_OFFLOAD */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is called to free packet allocated from kalPacketAlloc.
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 * \param[in] pvPacket       Pointer of the packet descriptor
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void kalPacketFree(struct GLUE_INFO *prGlueInfo,
		   void *pvPacket)
{
	if (prGlueInfo)
		RX_INC_CNT(&prGlueInfo->prAdapter->rRxCtrl,
			   RX_PACKET_FREE_COUNT);
	dev_kfree_skb((struct sk_buff *)pvPacket);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Only handles driver own creating packet (coalescing buffer).
 *
 * \param prGlueInfo   Pointer of GLUE Data Structure
 * \param u4Size       Pointer of Packet Handle
 * \param ppucData     Status Code for OS upper layer
 *
 * \return NULL: Failed to allocate skb, Not NULL get skb
 */
/*----------------------------------------------------------------------------*/
void *kalPacketAlloc(struct GLUE_INFO *prGlueInfo,
		     uint32_t u4Size,
		     u_int8_t fgIsTx,
		     uint8_t **ppucData)
{
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct sk_buff *prSkb = NULL;
	uint32_t u4TxHeadRoomSize = 0;

	glGetChipInfo((void **)&prChipInfo);

	if (fgIsTx) {
		if (prChipInfo) {
			u4TxHeadRoomSize = NIC_TX_DESC_AND_PADDING_LENGTH +
				prChipInfo->txd_append_size;
		}
	} else {
#ifdef CFG_SUPPORT_SNIFFER_RADIOTAP
		u4TxHeadRoomSize = CFG_RADIOTAP_HEADROOM;
#endif
	}

	if (in_interrupt())
		prSkb = __dev_alloc_skb(u4Size + u4TxHeadRoomSize,
					GFP_ATOMIC | __GFP_NOWARN);
	else
		prSkb = __dev_alloc_skb(u4Size + u4TxHeadRoomSize,
					GFP_KERNEL);

	if (prSkb) {
		skb_reserve(prSkb, u4TxHeadRoomSize);

		*ppucData = (uint8_t *) (prSkb->data);

		kalResetPacket(prGlueInfo, (void *) prSkb);

		if (prGlueInfo) {
			RX_INC_CNT(&prGlueInfo->prAdapter->rRxCtrl,
				RX_PACKET_ALLOC_COUNT);
		}
	}
#if DBG
	{
		uint32_t *pu4Head = (uint32_t *) &prSkb->cb[0];
		*pu4Head = (uint32_t) prSkb->head;
		DBGLOG(RX, TRACE, "prSkb->head = %#lx, prSkb->cb = %#lx\n",
		       (uint32_t) prSkb->head, *pu4Head);
	}
#endif
	return (void *) prSkb;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Only handles driver own creating packet (coalescing buffer).
 *
 * \param prGlueInfo   Pointer of GLUE Data Structure
 * \param u4Size       Pointer of Packet Handle
 * \param ppucData     Status Code for OS upper layer
 *
 * \return NULL: Failed to allocate skb, Not NULL get skb
 */
/*----------------------------------------------------------------------------*/
void *kalPacketAllocWithHeadroom(struct GLUE_INFO
		 *prGlueInfo, uint32_t u4Size, uint8_t **ppucData)
{
	struct sk_buff *prSkb = dev_alloc_skb(u4Size);

	if (!prSkb) {
		DBGLOG(TX, WARN, "alloc skb failed\n");
		return NULL;
	}

	/*
	 * Reserve NIC_TX_HEAD_ROOM as this skb
	 * is allocated by driver instead of kernel.
	 */
	skb_reserve(prSkb, NIC_TX_HEAD_ROOM);

	*ppucData = (uint8_t *) (prSkb->data);

	kalResetPacket(prGlueInfo, (void *) prSkb);
#if DBG
	{
		uint32_t *pu4Head = (uint32_t *) &prSkb->cb[0];
		*pu4Head = (uint32_t) prSkb->head;
		DBGLOG(RX, TRACE, "prSkb->head = %#lx, prSkb->cb = %#lx\n",
		       (uint32_t) prSkb->head, *pu4Head);
	}
#endif
	return (void *) prSkb;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Only handles driver own creating packet (coalescing buffer).
 *
 * \param pvPacket	socket buffer (skb)
 * \return			length of socket buffer (skb)
 */
/*----------------------------------------------------------------------------*/
uint32_t
kalQueryPacketLength(void *pvPacket)
{
	struct sk_buff *skb;

	if (!pvPacket)
		return 0;

	skb = (struct sk_buff *)pvPacket;
	return skb->len;
}

void
kalSetPacketLength(void *pvPacket, uint32_t u4len)
{
	struct sk_buff *skb;

	if (!pvPacket)
		return;

	skb = (struct sk_buff *)pvPacket;
	skb->len = u4len;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Only handles driver own creating packet (coalescing buffer).
 *
 * \param pvPacket	socket buffer (skb)
 * \return		16-bits of EtherType extracted from skb payload
 */
/*----------------------------------------------------------------------------*/
uint16_t
kalQueryPacketEtherType(void *pvPacket)
{
	uint8_t *pucEth;
	struct sk_buff *skb;

	if (!pvPacket)
		return 0xFFFF;

	skb = (struct sk_buff *)pvPacket;

	pucEth = skb->data;

	return (uint16_t) ((pucEth[ETH_TYPE_LEN_OFFSET] << 8)
					| (pucEth[ETH_TYPE_LEN_OFFSET + 1]));
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Only handles driver own creating packet (coalescing buffer).
 *
 * \param pvPacket	socket buffer (skb)
 * \return		8-bits value extended from 4-bits of IP
 * *			Version field from skb payload
 */
/*----------------------------------------------------------------------------*/
uint8_t
kalQueryPacketIPVersion(void *pvPacket)
{
	uint8_t *pucEth;
	struct sk_buff *skb;

	if (!pvPacket)
		return 0xFF;

	skb = (struct sk_buff *)pvPacket;

	pucEth = skb->data;

	return ((pucEth[ETH_HLEN] & IPVH_VERSION_MASK) >> IPVH_VERSION_OFFSET);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Only handles driver own creating packet (coalescing buffer).
 *
 * \param pvPacket	socket buffer (skb)
 * \return		8-bits value extended from 3-bits of precedence
 *			field from skb payload
 */
/*----------------------------------------------------------------------------*/
uint8_t
kalQueryPacketIPV4Precedence(void *pvPacket)
{
	uint8_t *pucEth;
	struct sk_buff *skb;

	if (!pvPacket)
		return 0xFF;

	skb = (struct sk_buff *)pvPacket;

	pucEth = skb->data;

	return ((pucEth[ETH_HLEN+1] & IPTOS_PREC_MASK) >> IPTOS_PREC_OFFSET);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Only handles driver own creating packet (coalescing buffer).
 *
 * \param pvPacket	socket buffer (skb)
 * \return		8-bits of IPv4 Protocol field from skb payload
 */
/*----------------------------------------------------------------------------*/
uint8_t
kalQueryPacketIPv4Protocol(void *pvPacket)
{
	uint8_t *pucEth;
	struct sk_buff *skb;

	if (!pvPacket)
		return 0xFF;

	skb = (struct sk_buff *)pvPacket;

	pucEth = skb->data;

	return pucEth[ETH_HLEN+9];
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Only handles driver own creating packet (coalescing buffer).
 *
 * \param pvPacket	socket buffer (skb)
 * \return		16-bits of IPv4 Identiication field from skb payload
 */
/*----------------------------------------------------------------------------*/
uint16_t
kalQueryPacketIPv4Identification(void *pvPacket)
{
	uint8_t *pucEth;
	struct sk_buff *skb;

	if (!pvPacket)
		return 0xFFFF;

	skb = (struct sk_buff *)pvPacket;

	pucEth = skb->data;

	return (uint16_t) ((pucEth[ETH_HLEN+4] << 8)
			| (pucEth[ETH_HLEN+5]));
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Only handles driver own creating packet (coalescing buffer).
 *
 * \param pvPacket	socket buffer (skb)
 * \return		16-bits of source port field from skb payload
 */
/*----------------------------------------------------------------------------*/
uint16_t
kalQueryPacketIPv4TCPUDPSrcPort(void *pvPacket)
{
	uint8_t *pucEth;
	struct sk_buff *skb;

	if (!pvPacket)
		return 0xFFFF;

	skb = (struct sk_buff *)pvPacket;

	pucEth = skb->data;

	return (uint16_t) ((pucEth[ETH_HLEN+20] << 8)
			| (pucEth[ETH_HLEN+21]));
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Only handles driver own creating packet (coalescing buffer).
 *
 * \param pvPacket	socket buffer (skb)
 * \retur		16-bits of destination port field from skb payload
 */
/*----------------------------------------------------------------------------*/
uint16_t
kalQueryPacketIPv4TCPUDPDstPort(void *pvPacket)
{
	uint8_t *pucEth;
	struct sk_buff *skb;

	if (!pvPacket)
		return 0xFFFF;

	skb = (struct sk_buff *)pvPacket;

	pucEth = skb->data;

	return (uint16_t) ((pucEth[ETH_HLEN+22] << 8)
			| (pucEth[ETH_HLEN+23]));
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Only handles driver own creating packet (coalescing buffer).
 *
 * \param pvPacket	socket buffer (skb)
 *		  pattern	pattern to be compared
 *		  length	length of pattern
 * \return			same as strncmp
 */
/*----------------------------------------------------------------------------*/
int
kalComparePacketIPv4UDPPayload(void *pvPacket, int8_t *pattern, size_t length)
{
	uint8_t *pucEth;
	uint8_t *pucUdp;
	struct sk_buff *skb;

	if (!pvPacket)
		return 0xFFFFFFFF;

	skb = (struct sk_buff *)pvPacket;

	pucEth = skb->data;

	pucUdp = &pucEth[ETH_HLEN+28];

	return strncmp(pucUdp, pattern, length);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Only handles driver own creating packet (coalescing buffer).
 *
 * \param pvPacket	socket buffer (skb)
 *		  offset	offset to be filled
 *		  pattern	pattern to be filled
 *		  length	length of pattern
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void
kalUpdatePacketIPv4UDPPayload(void *pvPacket,
		uint16_t offset,
		void *pattern,
		size_t length)
{
	uint8_t *pucEth;
	uint8_t *pucUdp;
	struct sk_buff *skb;

	if (!pvPacket)
		return;

	skb = (struct sk_buff *)pvPacket;

	pucEth = skb->data;

	pucUdp = &pucEth[ETH_HLEN+28];

	memcpy(&(pucUdp[offset]), pattern, length);
}


/*----------------------------------------------------------------------------*/
/*!
 * \brief Only handles driver own creating packet (coalescing buffer).
 *
 * \param pvPacket	socket buffer (skb)
 *	      ppucData  pointer to packet buffer
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void kalGetPacketBuf(void *pvPacket,
				uint8_t **ppucData)
{
	struct sk_buff *skb;

	if (!pvPacket) {
		*ppucData = NULL;
		return;
	}

	skb = (struct sk_buff *)pvPacket;
	*ppucData = (uint8_t *) (skb->data);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Only handles driver own creating packet (coalescing buffer).
 *
 * \param pvPacket	socket buffer (skb)
 *	      ppucData  pointer to packet buffer
 *	      length	postive value for skb_pull, negative for skb_push
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void kalGetPacketBufHeadManipulate(void *pvPacket,
		uint8_t **ppucData,
		int16_t length)
{
	struct sk_buff *skb;
	int len;

	if (!pvPacket)
		return;

	skb = (struct sk_buff *)pvPacket;

	if (length > 0) {
		len = length;
		skb_pull(skb, len);
	} else if (length < 0) {
		len = -length;
		skb_push(skb, len);
	}

	*ppucData = (uint8_t *) (skb->data);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Only handles driver own creating packet (coalescing buffer).
 *
 * \param pvPacket	socket buffer (skb)
 *	      ppucData  pointer to packet buffer
 *	      length	postive value for skb_put, negative for skb_trim
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void kalGetPacketBufTailManipulate(void *pvPacket,
		uint8_t **ppucData,
		int16_t length)
{
	struct sk_buff *skb;
	int len;

	if (!pvPacket)
		return;

	skb = (struct sk_buff *)pvPacket;

	if (length > 0) {
		len = length;
		skb_put(skb, len);
	} else if (length < 0) {
		len = -length;
		skb_trim(skb, len);
	}

	*ppucData = (uint8_t *) (skb->data);
}


uint32_t kalGetPacketMark(void *pvPacket)
{
	struct sk_buff *skb;
	uint32_t u4Mark;


	if (!pvPacket) {
		u4Mark = 0;
	} else {
		skb = (struct sk_buff *)pvPacket;
		u4Mark = skb->mark;
	}

	return u4Mark;
}

u_int8_t kalProcessRadiotap(void *pvPacket,
	uint8_t **ppucData,
	uint16_t radiotap_len,
	uint16_t u2RxByteCount)
{
	struct sk_buff *prSkb;

	prSkb = (struct sk_buff *)pvPacket;
	/* exceed skb headroom the kernel will panic */
	if (skb_headroom(prSkb) < radiotap_len) {
		DBGLOG(INIT, ERROR,
			"radiotap[%u] exceed skb headroom[%u]!\n",
			radiotap_len,
			skb_headroom(prSkb));
		return FALSE;
	}

	skb_push(prSkb, radiotap_len);
	*ppucData = (uint8_t *)(prSkb->data);
	kalMemZero(*ppucData, radiotap_len);

	skb_reset_tail_pointer(prSkb);
	skb_trim(prSkb, 0);
	skb_put(prSkb, (radiotap_len + u2RxByteCount));

	return TRUE;
}

void kalSetPacketDev(struct GLUE_INFO *prGlueInfo,
	uint8_t ucBssIndex,
	void *pvPacket)
{
	struct sk_buff *skb;

	if (pvPacket) {
		skb = (struct sk_buff *)pvPacket;
		skb->dev = wlanGetNetDev(prGlueInfo,
		ucBssIndex);
	}
}

void *kalGetPacketDev(void *pvPacket)
{
	struct sk_buff *skb;

	if (pvPacket) {
		skb = (struct sk_buff *)pvPacket;
		return (void *)skb->dev;
	}

	return (void *)NULL;
}

int kal_skb_checksum_help(void *pvPacket)
{
	struct sk_buff *prSkb;

	if (pvPacket) {
		prSkb = (struct sk_buff *)pvPacket;

		if (prSkb->ip_summed == CHECKSUM_PARTIAL)
			return skb_checksum_help(prSkb);
	}
	return 1;
}

void kalSkbCopyCbData(void *pvDstPacket, void *pvSrcPacket)
{
	struct sk_buff *prSkbDst;
	struct sk_buff *prSkbSrc;

	if (pvDstPacket && pvSrcPacket) {
		prSkbDst = (struct sk_buff *)pvDstPacket;
		prSkbSrc = (struct sk_buff *)pvSrcPacket;

		kalMemCopy(&prSkbDst->cb[0], &prSkbSrc->cb[0],
			sizeof(prSkbSrc->cb));
	}
}

void *kal_skb_copy(void *pvPacket)
{
	struct sk_buff *prSkb;
	void *prSkbCpy = NULL;

	if (pvPacket) {
		prSkb = (struct sk_buff *)pvPacket;
		prSkbCpy = (void *)skb_copy(prSkb, GFP_ATOMIC);
	}
	return prSkbCpy;
}

void kal_skb_reserve(void *pvPacket, uint8_t ucLength)
{
	struct sk_buff *prSkb;

	if (pvPacket) {
		prSkb = (struct sk_buff *)pvPacket;
		skb_reserve(prSkb, ucLength);
	}
}

void kal_skb_split(void *pvPacket, void *pvPacket1, const uint32_t u4Length)
{
	struct sk_buff *prSkb;
	struct sk_buff *prSkb1;

	if (pvPacket && pvPacket1) {
		prSkb = (struct sk_buff *)pvPacket;
		prSkb1 = (struct sk_buff *)pvPacket1;

		skb_split(prSkb, prSkb1, u4Length);
	}
}

uint8_t *kal_skb_push(void *pvPacket, uint32_t u4Length)
{
	struct sk_buff *prSkb;
	uint8_t *prSkbBuff = NULL;

	if (pvPacket) {
		prSkb = (struct sk_buff *)pvPacket;
		prSkbBuff = (uint8_t *)skb_push(prSkb, u4Length);
	}
	return prSkbBuff;
}

uint8_t *kal_skb_pull(void *pvPacket, uint32_t u4Length)
{
	struct sk_buff *prSkb;
	uint8_t *prSkbBuff = NULL;

	if (pvPacket) {
		prSkb = (struct sk_buff *)pvPacket;
		prSkbBuff = (uint8_t *)skb_pull(prSkb, u4Length);
	}
	return prSkbBuff;
}

/**
 * Detect and attempt to fix the wrong pointer problem if
 * pucRecvBuff and prRxStatus were changed and different from skb->head.
 *
 * If error detected and recoverable,
 * prSwRfb->pucRecvBuff = prSwRfb->prRxStatus = skb->data;
 * In case when CFG_SUPPORT_SNIFFER_RADIOTAP was defined, there will be
 * a 128-byte headroom (skb->data - skb->head).
 *
 * @return
 *	WLAN_STATUS_SUCCESS: sanity passed or wrong pointer fixed
 *	WLAN_STATUS_INVALID_PACKET: unrecoverable error
 */
uint32_t kalDuplicateSwRfbSanity(struct SW_RFB *prSwRfb)
{
#if CFG_FIX_INCONSISTENT_RFB_POINTER
	struct sk_buff *skb = prSwRfb->pvPacket;
	uint32_t offset = 0;

#ifdef CFG_SUPPORT_SNIFFER_RADIOTAP
	offset = CFG_RADIOTAP_HEADROOM;
#endif

	if (likely(skb->data == prSwRfb->pucRecvBuff &&
		   skb->data == prSwRfb->prRxStatus))
		return WLAN_STATUS_SUCCESS;

	if (likely(((uintptr_t)(const void *)skb->head & 0xFFF) == 0 &&
		   skb->data - skb->head == offset)) {
		prSwRfb->pucRecvBuff = skb->data;
		prSwRfb->prRxStatus = skb->data;
		/* RX buffer located at 4K aligned address, recoverable */
		return WLAN_STATUS_SUCCESS;
	}

	return WLAN_STATUS_INVALID_PACKET;
#else
	return WLAN_STATUS_SUCCESS;
#endif
}

#if CFG_SUPPORT_RX_PAGE_POOL
void kalSkbReuseCheck(struct SW_RFB *prSwRfb)
{
	struct sk_buff *prSkb;

	if (!prSwRfb)
		return;

	if (!prSwRfb->pvPacket)
		return;

	prSkb = (struct sk_buff *)prSwRfb->pvPacket;

	/* sanity check */
	if (prSwRfb->pucRecvBuff != prSkb->data) {
		DBGLOG(NIC, ERROR, "RX buffer not match, %04X != %04X\n",
			(uintptr_t)prSwRfb->pucRecvBuff & 0xFFFF,
			(uintptr_t)prSkb->data & 0xFFFF);
	}

	/*
	 * if skb headroom is not zero, then it may not 4 byte alignment,
	 * so we should not reuse it.
	 */
	if (prSkb->pp_recycle &&
#ifdef CFG_SUPPORT_SNIFFER_RADIOTAP
		(skb_headroom(prSkb) != CFG_RADIOTAP_HEADROOM)
#else
		skb_headroom(prSkb)
#endif
		) {
		DBGLOG_LIMITED(NIC, INFO,
			"Unexpected SKB with headroom[%d].\n",
			skb_headroom(prSkb));
		kalKfreeSkb(prSwRfb->pvPacket, TRUE);
		prSwRfb->pvPacket = NULL;
	}
}

void kalSkbMarkForRecycle(struct sk_buff *pkt)
{
	skb_mark_for_recycle(pkt);
}

#if (CFG_SUPPORT_PAGE_POOL_USE_CMA == 0)
static struct page *kalAllocPagePoolPageByIdx(
	struct GLUE_INFO *prGlueInfo, int i4Idx)
{
	struct page_pool *pool;
	struct page *prPage;

	if (i4Idx >= PAGE_POOL_NUM || i4Idx < 0) {
		DBGLOG(RX, ERROR, "index[%d] >= max num[%d]\n",
		       i4Idx, PAGE_POOL_NUM);
		return NULL;
	}

	pool = prGlueInfo->aprPagePool[i4Idx];
	if (!pool) {
		DBGLOG(RX, ERROR, "pool is null\n");
		return NULL;
	}
	mutex_lock(&prGlueInfo->arMutexPagePool[i4Idx]);
	prPage = page_pool_alloc_pages(pool, GFP_KERNEL);
	mutex_unlock(&prGlueInfo->arMutexPagePool[i4Idx]);

	return prPage;
}
#endif /* CFG_SUPPORT_PAGE_POOL_USE_CMA */

struct sk_buff *kalAllocRxSkbFromPp(
	struct GLUE_INFO *prGlueInfo, uint8_t **ppucData, int i4Idx)
{
#if (CFG_SUPPORT_PAGE_POOL_USE_CMA == 0)
	struct page *page;
	struct sk_buff *pkt = NULL;

	if (!prGlueInfo)
		goto fail;

	if (i4Idx >= 0) {
		page = kalAllocPagePoolPageByIdx(prGlueInfo, i4Idx);
		goto alloc;
	}

	if (prGlueInfo->u4LastAllocIdx >= PAGE_POOL_NUM)
		prGlueInfo->u4LastAllocIdx = 0;

	page = kalAllocPagePoolPageByIdx(
		prGlueInfo, prGlueInfo->u4LastAllocIdx);

	prGlueInfo->u4LastAllocIdx =
		(prGlueInfo->u4LastAllocIdx + 1) % PAGE_POOL_NUM;

alloc:
	if (!page)
		goto fail;

	pkt = build_skb(page_to_virt(page), PAGE_SIZE); /* ptr to sk_buff */
	if (!pkt) {
		page_pool_recycle_direct(page->pp, page);
		DBGLOG(RX, ERROR, "allocate skb fail\n");
		goto fail;
	}
	kalSkbMarkForRecycle(pkt);

#ifdef CFG_SUPPORT_SNIFFER_RADIOTAP
	skb_reserve(pkt, CFG_RADIOTAP_HEADROOM);
#endif

	*ppucData = (uint8_t *) (pkt->data);

fail:
	if (!pkt) {
		pkt = kalPacketAlloc(
			prGlueInfo, CFG_RX_MAX_MPDU_SIZE,
			FALSE, ppucData);
	}
	return pkt;
#else
	return kalAllocRxSkbFromCmaPp(prGlueInfo, ppucData);
#endif /* CFG_SUPPORT_PAGE_POOL_USE_CMA */
}

int kalPtrRingCnt(struct ptr_ring *ring)
{
	int count;

	/* Check if the ring is full */
	if (__ptr_ring_full(ring)) {
		count = ring->size;
	} else if (__ptr_ring_empty(ring)) {
		count = 0;
	} else {
		/* Calculate the number of items in the ring */
		count = ring->producer - ring->consumer_head;
		if (count < 0)
			count += ring->size;
	}

	return count;
}

void kalCreatePagePool(struct GLUE_INFO *prGlueInfo)
{
	struct page_pool *pool;
	struct page_pool_params pp = {0};
	int i;

	pp.max_len = PAGE_SIZE;
	pp.flags = 0;
	pp.pool_size = PAGE_POOL_MAX_SIZE;
	pp.nid = dev_to_node(prGlueInfo->prDev);
	pp.dev = prGlueInfo->prDev;
	pp.dma_dir = DMA_FROM_DEVICE;

	for (i = 0; i < PAGE_POOL_NUM; i++) {
		pool = page_pool_create(&pp);
		if (IS_ERR(pool)) {
			int err = PTR_ERR(pool);

			pr_info("%s: create page pool fail[%d]", __func__, err);
			continue;
		}
		prGlueInfo->aprPagePool[i] = pool;
	}
}

void kalReleasePagePool(struct GLUE_INFO *prGlueInfo)
{
	struct page_pool *pool;
	int i;

	for (i = 0; i < PAGE_POOL_NUM; i++) {
		pool = prGlueInfo->aprPagePool[i];
		if (!pool)
			continue;
		page_pool_destroy(pool);
		prGlueInfo->aprPagePool[i] = NULL;
	}
}
#endif /* CFG_SUPPORT_RX_PAGE_POOL */

/*----------------------------------------------------------------------------*/
/*!
 * \brief Process the received packet for indicating to OS.
 *
 * \param[in] prGlueInfo     Pointer to the Adapter structure.
 * \param[in] pvPacket       Pointer of the packet descriptor
 * \param[in] pucPacketStart The starting address of the buffer of Rx packet.
 * \param[in] u4PacketLen    The packet length.
 * \param[in] aerCSUM        The result of TCP/ IP checksum offload.
 *
 * \retval WLAN_STATUS_SUCCESS.
 * \retval WLAN_STATUS_FAILURE.
 *
 */
/*----------------------------------------------------------------------------*/
uint32_t
kalProcessRxPacket(struct GLUE_INFO *prGlueInfo,
		   void *pvPacket, uint8_t *pucPacketStart,
		   uint32_t u4PacketLen,
		   enum ENUM_CSUM_RESULT aerCSUM[])
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	struct sk_buff *skb = (struct sk_buff *)pvPacket;

	if (!skb || !pucPacketStart) {
		RX_INC_CNT(&prGlueInfo->prAdapter->rRxCtrl,
			RX_NULL_PACKET_COUNT);
		return WLAN_STATUS_FAILURE;
	}

	skb->data = (unsigned char *)pucPacketStart;

	/* Reset skb */
	skb_reset_tail_pointer(skb);
	skb_trim(skb, 0);

	if (skb_tailroom(skb) < 0 || u4PacketLen > skb_tailroom(skb)) {
		DBGLOG(RX, ERROR,
#ifdef NET_SKBUFF_DATA_USES_OFFSET
			"[skb:0x%p][skb->len:%d][skb->protocol:0x%02X] tail:%u, end:%u, data:%p\n",
#else
			"[skb:0x%p][skb->len:%d][skb->protocol:0x%02X] tail:%p, end:%p, data:%p\n",
#endif
			(uint8_t *) skb,
			skb->len,
			skb->protocol,
			skb->tail,
			skb->end,
			skb->data);
		DBGLOG_MEM32(RX, ERROR, (uint32_t *) skb->data, skb->len);
		return WLAN_STATUS_FAILURE;
	}

	/* Put data */
	skb_put(skb, u4PacketLen);

#if CFG_TCP_IP_CHKSUM_OFFLOAD
	if (prGlueInfo->prAdapter->fgIsSupportCsumOffload)
		kalUpdateRxCSUMOffloadParam(skb, aerCSUM);
#endif

	return rStatus;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief To indicate an array of received packets is available for higher
 *        level protocol uses.
 *
 * \param[in] prGlueInfo Pointer to the Adapter structure.
 * \param[in] apvPkts The packet array to be indicated
 * \param[in] ucPktNum The number of packets to be indicated
 *
 * \retval TRUE Success.
 *
 */
/*----------------------------------------------------------------------------*/
uint32_t kalRxIndicatePkts(struct GLUE_INFO *prGlueInfo,
			   void *apvPkts[], uint8_t ucPktNum)
{
	uint8_t ucIdx = 0;

	ASSERT(prGlueInfo);
	ASSERT(apvPkts);

	for (ucIdx = 0; ucIdx < ucPktNum; ucIdx++)
		kalRxIndicateOnePkt(prGlueInfo, apvPkts[ucIdx]);

	KAL_WAKE_LOCK_TIMEOUT(prGlueInfo->prAdapter,
		prGlueInfo->rTimeoutWakeLock, MSEC_TO_JIFFIES(
		prGlueInfo->prAdapter->rWifiVar.u4WakeLockRxTimeout));

	return WLAN_STATUS_SUCCESS;
}

#if CFG_SUPPORT_RX_GRO
/*----------------------------------------------------------------------------*/
/*!
 * \brief To indicate Tput is higher than ucGROEnableTput or not.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 *
 * \retval TRUE Success.
 *
 */
/*----------------------------------------------------------------------------*/
uint32_t kal_is_skb_gro(struct ADAPTER *prAdapter, uint8_t ucBssIdx)
{
	struct PERF_MONITOR *prPerMonitor;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;

	if (ucBssIdx >= MAX_BSSID_NUM)
		return 0;

	prPerMonitor = &prAdapter->rPerMonitor;
	if (prPerMonitor->ulRxTp[ucBssIdx] > prWifiVar->ucGROEnableTput)
		return 1;

	return 0;
}

static inline void napi_gro_flush_list(struct napi_struct *napi)
{
	napi_gro_flush(napi, false);
#if KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
	if (napi->rx_count) {
		netif_receive_skb_list(&napi->rx_list);
		INIT_LIST_HEAD(&napi->rx_list);
		napi->rx_count = 0;
	}
#endif
}

static inline void kal_gro_flush_queue(struct GLUE_INFO *prGlueInfo)
{
	if (prGlueInfo->u4PendingFlushNum) {
		preempt_disable();
		spin_lock_bh(&prGlueInfo->napi_spinlock);
		napi_gro_flush_list(&prGlueInfo->napi);
		GET_CURRENT_SYSTIME(
			&prGlueInfo->tmGROFlushTimeout);
		prGlueInfo->u4PendingFlushNum = 0;
		spin_unlock_bh(&prGlueInfo->napi_spinlock);
		preempt_enable();
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief flush Rx packet to kernel if kernel buffer is full or timeout(1ms)
 *
 * @param[in] prAdapter Pointer to the Adapter structure.
 *
 * @retval VOID
 *
 */
/*----------------------------------------------------------------------------*/
void kal_gro_flush(struct ADAPTER *prAdapter)
{
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;

	if (CHECK_FOR_TIMEOUT(kalGetTimeTick(),
		prGlueInfo->tmGROFlushTimeout,
		prWifiVar->ucGROFlushTimeout)) {
		napi_gro_flush(&prGlueInfo->napi, false);
		DBGLOG_LIMITED(INIT, TRACE, "napi_gro_flush.\n");
#if KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
		prGlueInfo->u4PendingFlushNum =
			prGlueInfo->napi.rx_count;
#else
		prGlueInfo->u4PendingFlushNum = 0;
#endif
	} else
		prGlueInfo->u4PendingFlushNum++;

	GET_CURRENT_SYSTIME(&prGlueInfo->tmGROFlushTimeout);
}
#endif
/*----------------------------------------------------------------------------*/
/*!
 * \brief To indicate one received packets is available for higher
 *        level protocol uses.
 *
 * \param[in] prGlueInfo Pointer to the Adapter structure.
 * \param[in] pvPkt The packet to be indicated
 *
 * \retval TRUE Success.
 *
 */
/*----------------------------------------------------------------------------*/
uint32_t kalRxIndicateOnePkt(struct GLUE_INFO
			     *prGlueInfo, void *pvPkt)
{
	struct net_device *prNetDev = prGlueInfo->prDevHandler;
	struct sk_buff *prSkb = NULL;
	struct mt66xx_chip_info *prChipInfo;
	uint8_t ucBssIdx;

	ASSERT(prGlueInfo);
	ASSERT(pvPkt);

	prSkb = pvPkt;
	prChipInfo = prGlueInfo->prAdapter->chip_info;
	ucBssIdx = GLUE_GET_PKT_BSS_IDX(prSkb);
	RX_INC_CNT(&prGlueInfo->prAdapter->rRxCtrl, RX_DATA_INDICATION_COUNT);
#if DBG && 0
	do {
		uint8_t *pu4Head = (uint8_t *) &prSkb->cb[0];
		uint32_t u4HeadValue = 0;

		kalMemCopy(&u4HeadValue, pu4Head, sizeof(u4HeadValue));
		DBGLOG(RX, TRACE, "prSkb->head = 0x%p, prSkb->cb = 0x%lx\n",
		       pu4Head, u4HeadValue);
	} while (0);
#endif

	if (ucBssIdx < MAX_BSSID_NUM) {
		prNetDev = (struct net_device *)wlanGetNetInterfaceByBssIdx(
			   prGlueInfo, ucBssIdx);
	} else {
		DBGLOG(RX, WARN, "Error ucBssIdx =%u\n", ucBssIdx);
		DBGLOG(RX, WARN, "Error pkt info =%u:%u\n",
			GLUE_IS_PKT_FLAG_SET(prSkb),
			GLUE_GET_PKT_IP_ID(prSkb));
	}
	if (!prNetDev)
		prNetDev = prGlueInfo->prDevHandler;

	DBGLOG(RX, LOUD, "ucBssIdx:%u, netdev:0x%p, name:%s\n",
		ucBssIdx, prNetDev, prNetDev->name);

	if (prNetDev->dev_addr == NULL) {
		DBGLOG(RX, WARN, "dev_addr == NULL\n");
		return WLAN_STATUS_FAILURE;
	}

	prNetDev->stats.rx_bytes += prSkb->len;
	prNetDev->stats.rx_packets++;
#if CFG_SUPPORT_PERF_IND
	if (GLUE_GET_PKT_BSS_IDX(prSkb) < MAX_BSSID_NUM) {
	/* update Performance Indicator statistics*/
		prGlueInfo->PerfIndCache.u4CurRxBytes
			[GLUE_GET_PKT_BSS_IDX(prSkb)] += prSkb->len;
	}
#endif

#if (CFG_SUPPORT_STATISTICS == 1)
	StatsEnvRxTime2Host(prGlueInfo->prAdapter, prSkb);
#endif

#if KERNEL_VERSION(4, 11, 0) <= CFG80211_VERSION_CODE
	/* ToDo jiffies assignment */
#else
	prNetDev->last_rx = jiffies;
#endif

#ifdef CFG_SUPPORT_SNIFFER_RADIOTAP
	if (prGlueInfo->fgIsEnableMon) {
		skb_reset_mac_header(prSkb);
		prSkb->ip_summed = CHECKSUM_UNNECESSARY;
		prSkb->pkt_type = PACKET_OTHERHOST;
		prSkb->protocol = htons(ETH_P_802_2);
	} else {
		prSkb->protocol = eth_type_trans(prSkb, prNetDev);
	}
#else
	prSkb->protocol = eth_type_trans(prSkb, prNetDev);
#endif
	prSkb->dev = prNetDev;

	if (prSkb->tail > prSkb->end) {
		DBGLOG(RX, ERROR,
#ifdef NET_SKBUFF_DATA_USES_OFFSET
		       "kalRxIndicateOnePkt [prSkb = 0x%p][prSkb->len = %d][prSkb->protocol = 0x%02x] %u,%u\n",
#else
		       "kalRxIndicateOnePkt [prSkb = 0x%p][prSkb->len = %d][prSkb->protocol = 0x%02x] %p,%p\n",
#endif
		       prSkb, prSkb->len, prSkb->protocol, prSkb->tail,
		       prSkb->end);
		DBGLOG_MEM32(RX, ERROR, (uint32_t *) prSkb->data,
			     prSkb->len);
	}

#if CFG_SUPPORT_WED_PROXY
	/* HW Rx request after skb->dev to be assign */
	wedHwRxRequest(prSkb);
#endif

	if (prSkb->protocol == NTOHS(ETH_P_8021Q)
	    && !FEAT_SUP_LLC_VLAN_RX(prChipInfo)) {
		/*
		 * DA-MAC + SA-MAC + 0x8100 was removed in eth_type_trans()
		 * pkt format here is
		 * TCI(2-bytes) + Len(2-btyes) + payload-type(2-bytes) + payload
		 * Remove "Len" field inserted by RX VLAN header translation
		 * Note: TCI+payload-type is a standard 8021Q header
		 *
		 * This format update is based on RX VLAN HW header translation.
		 * If the setting was changed, you may need to change rules here
		 * as well.
		 */
		const uint8_t vlan_skb_mem_move = 2;

		/* Remove "Len" and shift data pointer 2 bytes */
		kalMemCopy(prSkb->data + vlan_skb_mem_move, prSkb->data,
			   vlan_skb_mem_move);
		skb_pull_rcsum(prSkb, vlan_skb_mem_move);

		/* Have to update MAC header properly. Otherwise, wrong MACs
		 * woud be passed up
		 */
		kalMemMove(prSkb->data - ETH_HLEN,
			   prSkb->data - ETH_HLEN - vlan_skb_mem_move,
			   ETH_HLEN);
		prSkb->mac_header += vlan_skb_mem_move;

		skb_reset_network_header(prSkb);
		skb_reset_transport_header(prSkb);
		kal_skb_reset_mac_len(prSkb);
	}

	kalTraceEvent("Rx ipid=0x%04x", GLUE_GET_PKT_IP_ID(prSkb));

#if CFG_SUPPORT_RX_GRO
#if CFG_SUPPORT_SKIP_RX_GRO_FOR_TC
	if (kalGetSkipRxGro(prNetDev))
		goto skip_gro;
#endif /* CFG_SUPPORT_SKIP_RX_GRO_FOR_TC */

	if (kal_is_skb_gro(prGlueInfo->prAdapter, ucBssIdx)) {
#if CFG_SUPPORT_RX_PAGE_POOL
		/* avoid recycle & non-recycle skb merged to non-recycle skb */
		kalSkbMarkForRecycle(prSkb);
#endif
#if CFG_SUPPORT_RX_NAPI
		if (HAL_IS_RX_DIRECT(prGlueInfo->prAdapter)) {
			/* We should stay in NAPI context now */
			/*
			 * There is two context may reach here:
			 * 1. napi context
			 * 2. rx order timeout handler
			 */
			preempt_disable();
			spin_lock_bh(&prGlueInfo->napi_spinlock);
			napi_gro_receive(&prGlueInfo->napi, prSkb);
			RX_INC_CNT(&prGlueInfo->prAdapter->rRxCtrl,
				RX_DATA_NAPI_GRO_RECEIVE_COUNT);
			spin_unlock_bh(&prGlueInfo->napi_spinlock);
			preempt_enable();
		} else {
			skb_queue_tail(&prGlueInfo->rRxNapiSkbQ, prSkb);
			RX_INC_CNT(&prGlueInfo->prAdapter->rRxCtrl,
				RX_NAPI_SCHEDULE_COUNT);
			GLUE_SET_REF_CNT(1, prGlueInfo->fgNapiScheduled);
			kal_napi_schedule(&prGlueInfo->napi);
		}
#else /* CFG_SUPPORT_RX_NAPI */
		/* GRO receive function can't be interrupt so it need to
		 * disable preempt and protect by spin lock
		 */
		preempt_disable();
		spin_lock_bh(&prGlueInfo->napi_spinlock);
		napi_gro_receive(&prGlueInfo->napi, prSkb);
		RX_INC_CNT(&prGlueInfo->prAdapter->rRxCtrl,
			RX_DATA_NAPI_GRO_RECEIVE_COUNT);
		kal_gro_flush(prGlueInfo->prAdapter);
		spin_unlock_bh(&prGlueInfo->napi_spinlock);
		preempt_enable();
		DBGLOG_LIMITED(INIT, TRACE, "napi_gro_receive\n");
#endif /* CFG_SUPPORT_RX_NAPI */
		return WLAN_STATUS_SUCCESS;
	}
#if CFG_SUPPORT_SKIP_RX_GRO_FOR_TC
skip_gro:
#endif /* CFG_SUPPORT_SKIP_RX_GRO_FOR_TC */
#endif /* CFG_SUPPORT_RX_GRO */

#if KERNEL_VERSION(5, 18, 0) <= LINUX_VERSION_CODE
	netif_rx(prSkb);
#else
	if (!in_interrupt())
		netif_rx_ni(prSkb);
	else
		netif_rx(prSkb);
#endif
	RX_INC_CNT(&prGlueInfo->prAdapter->rRxCtrl,
		RX_DATA_INDICATION_END_COUNT);
	return WLAN_STATUS_SUCCESS;
}

#if CFG_RFB_RECOVERY
/*----------------------------------------------------------------------------*/
/*!
 * \brief to check whether SWRFB has severe memory leaks and trigger wifi
 *        reset .
 *
 * \param[in] prGlueInfo Pointer to the GLUE_INFO structure.
 *
 * \return (none)
 *
 */
/*----------------------------------------------------------------------------*/
void kalRxRFBFailRecoveryCheck(struct GLUE_INFO *prGlueInfo)
{
	struct RX_CTRL *prRxCtrl;

	if (!prGlueInfo)
		return;

	prRxCtrl = &prGlueInfo->prAdapter->rRxCtrl;

	if (RX_GET_TOTAL_RFB_CNT(prGlueInfo) >= CFG_RX_RFB_MEM_LEAK_THRESHOLD) {
		prRxCtrl->u4CheckRFBFailTime = 0;
		return;
	}

	/* If CheckRFBFailTime is 0 that indicates the first detection of a
	  * small amount of SWRFB, and set the next detection time to double
	  * confirm SWRFB leaks is not false alarm.
	  */
	if (prRxCtrl->u4CheckRFBFailTime == 0) {
		prRxCtrl->u4CheckRFBFailTime = kalGetTimeTick()
			+ CFG_RX_RFB_MEM_LEAK_INTERVAL;
		DBGLOG_LIMITED(RX, INFO,
			"Monitor RFB memory leak, check RFB fail time : %u\n",
			prRxCtrl->u4CheckRFBFailTime);
		return;
	}

	if (TIME_AFTER(kalGetTimeTick(), prRxCtrl->u4CheckRFBFailTime)) {
		DBGLOG(RX, ERROR,
			"Trigger chip reset due to RFB memory leak\n");
		GL_DEFAULT_RESET_TRIGGER(prGlueInfo->prAdapter,
			RST_RFB_FAIL);
	}
}
#endif

#if CFG_SUPPORT_NAN
/*----------------------------------------------------------------------------*/
/*!
 * \brief Called by driver to indicate event to upper layer, for example, the
 *        wpa supplicant or wireless tools.
 *
 * \param[in] pvAdapter Pointer to the adapter descriptor.
 * \param[in] eStatus Indicated status.
 * \param[in] NAN_BSS_ROLE_INDEX eIndex
 *
 * \return (none)
 *
 */
/*----------------------------------------------------------------------------*/
void kalNanIndicateStatusAndComplete(struct GLUE_INFO *prGlueInfo,
				uint32_t eStatus, uint8_t ucRoleIdx)
{

	DBGLOG(NAN, INFO, "NanIndicateStatus %x\n", eStatus);
	switch (eStatus) {
	case WLAN_STATUS_MEDIA_CONNECT:
#if !CFG_SUPPORT_NAN_CARRIER_ON_INIT
		netif_carrier_on(
			prGlueInfo->aprNANDevInfo[ucRoleIdx]->prDevHandler);
#endif
		break;

	case WLAN_STATUS_MEDIA_DISCONNECT:
	case WLAN_STATUS_MEDIA_DISCONNECT_LOCALLY:
#if !CFG_SUPPORT_NAN_CARRIER_ON_INIT
		netif_carrier_off(
			prGlueInfo->aprNANDevInfo[ucRoleIdx]->prDevHandler);
#endif
		break;

	default:
		break;
	}
}

void kalCreateUserSock(struct GLUE_INFO *prGlueInfo)
{
	prGlueInfo->NetLinkSK =
		netlink_kernel_create(&init_net, MTKPROTO, NULL);
	DBGLOG(INIT, INFO, "Create netlink Socket\n");
	if (!prGlueInfo->NetLinkSK)
		DBGLOG(INIT, INFO, "Create Socket Fail\n");
}
void kalReleaseUserSock(struct GLUE_INFO *prGlueInfo)
{
	if (prGlueInfo->NetLinkSK) {
		DBGLOG(INIT, INFO, "Release netlink Socket\n");
		netlink_kernel_release(prGlueInfo->NetLinkSK);
	}
}

int kalIndicateNetlink2User(struct GLUE_INFO *prGlueInfo, void *pvBuf,
			uint32_t u4BufLen)
{
	struct sk_buff *skb;
	struct nlmsghdr *nlh;
	int res;

	if (!prGlueInfo->NetLinkSK) {
		DBGLOG(NAN, INFO, "Socket not create\n");
		return -10;
	}
	DBGLOG(NAN, LOUD, "Creating skb.\n");

	skb = nlmsg_new(NLMSG_ALIGN(u4BufLen + 1), GFP_KERNEL);
	if (!skb) {
		DBGLOG(NAN, ERROR, "Allocation failure.\n");
		return -10;
	}

	nlh = nlmsg_put(skb, 0, 1, NLMSG_DONE, u4BufLen + 1, 0);
	if (nlh == NULL) {
		DBGLOG(NAN, ERROR, "netlink msg put failed!\n");
		kfree_skb(skb);
		return -10;
	}

	kalMemCopy(nlmsg_data(nlh), pvBuf, u4BufLen);
	res = nlmsg_multicast(prGlueInfo->NetLinkSK, skb, 0, MTKGRP,
			      GFP_KERNEL);
	if (res < 0)
		DBGLOG(NAN, ERROR, "nlmsg_multicast() error: %d\n", res);
	else
		DBGLOG(NAN, LOUD, "Success\n");
	return 0;
}
#endif

struct cfg80211_bss * kalInformConnectionBss(struct ADAPTER *prAdapter,
	struct ieee80211_channel *prChannel, uint8_t *arBssid,
	uint8_t ucBssIndex)
{
	uint8_t *pos = NULL;
	uint16_t len = 0;
	struct BSS_DESC *prBssDesc = NULL;
	struct STA_RECORD *prStaRec;
	struct cfg80211_bss *bss;

	/* create BSS on-the-fly */
	prBssDesc = aisGetTargetBssDesc(prAdapter, ucBssIndex);
	prStaRec = aisGetStaRecOfAP(prAdapter, ucBssIndex);

	if (!prBssDesc || !prChannel || !prStaRec)
		return NULL;

	pos = prBssDesc->pucIeBuf;
	len = prBssDesc->u2IELength;

#if KERNEL_VERSION(3, 18, 0) <= CFG80211_VERSION_CODE
	bss = cfg80211_inform_bss(
		wlanGetWiphy(),
		prChannel,
		CFG80211_BSS_FTYPE_PRESP,
		arBssid,
		0, /* TSF */
		prBssDesc->u2CapInfo,
		prBssDesc->u2BeaconInterval, /* beacon interval */
		pos, /* IE */
		len, /* IE Length */
		RCPI_TO_dBm(prBssDesc->ucRCPI) * 100, /* MBM */
		GFP_KERNEL);
#else
	bss = cfg80211_inform_bss(
		wlanGetWiphy(),
		prChannel,
		arBssid,
		0, /* TSF */
		prBssDesc->u2CapInfo,
		prBssDesc->u2BeaconInterval, /* beacon interval */
		pos, /* IE */
		len, /* IE Length */
		RCPI_TO_dBm(prBssDesc->ucRCPI) * 100, /* MBM */
		GFP_KERNEL);
#endif

	return bss;
}

struct LINK_INFO {
	uint8_t used;
	uint8_t *addr;
	uint8_t *bssid;
	uint8_t link_id;
	struct ieee80211_channel *channel;
	struct cfg80211_bss *bss;
};

uint32_t kalCollectLinkInfo(struct ADAPTER *prAdapter,
	struct LINK_INFO *link, uint8_t ucBssIndex)
{
	struct BSS_INFO *prBssInfo = NULL;
	struct ieee80211_channel *prChannel = NULL;
	struct cfg80211_bss *bss = NULL;
	struct cfg80211_bss *bss_others = NULL;
	uint8_t chnlNum, band;
	enum ENUM_BAND eBand;
	uint8_t ucLoopCnt = 15; /* only loop 15 times to avoid dead loop */

	prBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(REQ, ERROR, "Invalid prBssInfo:%d!\n", ucBssIndex);
		return WLAN_STATUS_FAILURE;
	}

	/* retrieve channel */
	chnlNum = wlanGetChannelNumberByNetwork(prAdapter, ucBssIndex);
	eBand = wlanGetBandIndexByNetwork(prAdapter, ucBssIndex);
	if (eBand > BAND_NULL && eBand < BAND_NUM)
		band = aucBandTranslate[eBand];
	else {
		DBGLOG(REQ, ERROR, "Invalid band:%d!\n", eBand);
		return WLAN_STATUS_FAILURE;
	}

	prChannel = ieee80211_get_channel(wlanGetWiphy(),
		    ieee80211_channel_to_frequency(chnlNum, band));

	if (!prChannel)
		DBGLOG(SCN, ERROR,
		       "prChannel is NULL and ucChannelNum is %d\n",
		       chnlNum);

	/* ensure BSS exists */
#if KERNEL_VERSION(4, 1, 0) <= CFG80211_VERSION_CODE
	bss = cfg80211_get_bss(
		wlanGetWiphy(),
		prChannel, prBssInfo->aucBSSID,
		prBssInfo->aucSSID, prBssInfo->ucSSIDLen,
		IEEE80211_BSS_TYPE_ESS,
		IEEE80211_PRIVACY_ANY);
#else
	bss = cfg80211_get_bss(
		wlanGetWiphy(),
		prChannel, prBssInfo->aucBSSID,
		prBssInfo->aucSSID, prBssInfo->ucSSIDLen,
		WLAN_CAPABILITY_ESS,
		WLAN_CAPABILITY_ESS);
#endif

	if (bss == NULL) {
		DBGLOG(SCN, WARN,
			"cannot find bss by cfg80211_get_bss");
		bss = kalInformConnectionBss(prAdapter,
			prChannel, prBssInfo->aucBSSID, ucBssIndex);
	}

#if (CFG_SUPPORT_STATISTICS == 1)
	StatsResetTxRx();
#endif
	/* remove all bsses that before and only channel
	 * different with the current connected one
	 * if without this patch, UI will show channel A is
	 * connected even if AP has change channel from A to B
	 */
	while (ucLoopCnt--) {
#if KERNEL_VERSION(4, 1, 0) <= CFG80211_VERSION_CODE
		bss_others = cfg80211_get_bss(
				wlanGetWiphy(),
				NULL, prBssInfo->aucBSSID,
				prBssInfo->aucSSID, prBssInfo->ucSSIDLen,
				IEEE80211_BSS_TYPE_ESS,
				IEEE80211_PRIVACY_ANY);
#else
		bss_others = cfg80211_get_bss(
				wlanGetWiphy(),
				NULL, prBssInfo->aucBSSID,
				prBssInfo->aucSSID, prBssInfo->ucSSIDLen,
				WLAN_CAPABILITY_ESS,
				WLAN_CAPABILITY_ESS);
#endif
		if (bss && bss_others && bss_others != bss) {
			DBGLOG(SCN, INFO,
			       "remove BSSes that only channel different\n");
			cfg80211_unlink_bss(
				wlanGetWiphy(),
				bss_others);
			cfg80211_put_bss(
				wlanGetWiphy(),
				bss_others);
		} else {
			if (bss_others) {
				DBGLOG(SCN, TRACE,
					"call cfg80211_put_bss for bss_others");
				cfg80211_put_bss(
					wlanGetWiphy(),
					bss_others);
			}
			break;
		}
	}

	link->used = TRUE;
	link->addr = prBssInfo->aucOwnMacAddr;
	link->bssid = prBssInfo->aucBSSID;
	link->channel = prChannel;
	link->bss = bss;

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	link->link_id = prBssInfo->prStaRecOfAP ?
		prBssInfo->prStaRecOfAP->ucLinkIndex : 0;
#endif

	return WLAN_STATUS_SUCCESS;
}

#if (CFG_SUPPORT_FILS_SK_OFFLOAD == 1)
void kalReportFilsInfoAndAddRemoveKey(struct ADAPTER *prAdapter,
	struct cfg80211_fils_resp_params *param, uint8_t add,
	uint8_t ucBssIndex)
{
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;
	struct FILS_INFO *prFils = NULL;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo)
		return;

	if (add) {
		prStaRec = prBssInfo->prStaRecOfAP;

		if (!prStaRec || !rsnIsFilsAuthAlg(prStaRec->ucAuthAlgNum))
			return;

		prFils = prStaRec->prFilsInfo;
		if (!prFils)
			return;

		if (param) {
			param->kek = prFils->kek;
			param->kek_len = prFils->kek_len;
			param->update_erp_next_seq_num = 1;
			param->erp_next_seq_num = prFils->prErpKey->nextSeq;
			param->pmk = prFils->aucFilsPmk;
			param->pmk_len = prFils->u2FilsPmkLen;
			param->pmkid = prFils->aucFilsErpPmkid;
		}

		filsInstallKey(prAdapter, prStaRec);
	} else {
		filsRemoveAllKeys(prAdapter, ucBssIndex);
	}
}
#endif /* CFG_SUPPORT_FILS_SK_OFFLOAD */

uint32_t kalReportAllLinkInfo(struct ADAPTER *prAdapter,
	struct net_device *netdev,
	uint32_t eStatus, void *pvBuf,
	uint32_t u4BufLen, uint8_t ucBssIndex)
{
	struct BSS_INFO *prBssInfo;
	struct CONNECTION_SETTINGS *prConnSettings = NULL;
	struct LINK_INFO links[MLD_LINK_MAX] = {0};
#if ((CFG_ADVANCED_80211_MLO == 1) || \
	(KERNEL_VERSION(6, 0, 0) <= CFG80211_VERSION_CODE)) && \
	(CFG_SUPPORT_802_11BE_MLO == 1)
	uint16_t valid_links = 0;
	struct MLD_STA_RECORD *prMldStaRec = NULL;
	uint8_t i;
#endif
	uint32_t status;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(INIT, INFO, "BSS Info not exist !!\n");
		return WLAN_STATUS_FAILURE;
	}

#if ((CFG_ADVANCED_80211_MLO == 1) || \
	(KERNEL_VERSION(6, 0, 0) <= CFG80211_VERSION_CODE)) && \
	(CFG_SUPPORT_802_11BE_MLO == 1)
	prMldStaRec = mldStarecGetByStarec(prAdapter, prBssInfo->prStaRecOfAP);
	if (prMldStaRec) {
		struct STA_RECORD *sta;
		uint8_t count = 0;

		LINK_FOR_EACH_ENTRY(sta, &prMldStaRec->rStarecList,
					rLinkEntryMld, struct STA_RECORD) {
			if (count >= MLD_LINK_MAX) {
				DBGLOG(INIT, INFO, "too many links!!!\n");
				return WLAN_STATUS_NOT_ACCEPTED;
			}

			status = kalCollectLinkInfo(prAdapter,
				&links[count], sta->ucBssIndex);
			if (status != WLAN_STATUS_SUCCESS)
				return status;

			valid_links |= BIT(links[count].link_id);
			count++;
		}
	} else
#endif
	{
		status = kalCollectLinkInfo(prAdapter, &links[0], ucBssIndex);
		if (status != WLAN_STATUS_SUCCESS)
			return status;
		/* no need to update valid_links for non-mlo */
	}

	/* CFG80211 Indication */
	prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);
	if (eStatus == WLAN_STATUS_ROAM_OUT_FIND_BEST) {
#if KERNEL_VERSION(4, 12, 0) <= CFG80211_VERSION_CODE
		struct cfg80211_roam_info rRoamInfo = {0};
		uint8_t ucAuthorized = pvBuf ? *(uint8_t *) pvBuf : FALSE;

#if (CFG_ADVANCED_80211_MLO == 1) || \
	(KERNEL_VERSION(6, 0, 0) <= CFG80211_VERSION_CODE)
#if (CFG_SUPPORT_802_11BE_MLO == 1)
		if (prMldStaRec) {
			rRoamInfo.ap_mld_addr = prMldStaRec->aucPeerMldAddr;
			rRoamInfo.valid_links = valid_links;
			for (i = 0; i < MLD_LINK_MAX; i++) {
				uint8_t id = links[i].link_id;

				if (!links[i].used)
					continue;

				rRoamInfo.links[id].addr = links[i].addr;
				rRoamInfo.links[id].bssid = links[i].bssid;
				rRoamInfo.links[id].bss = links[i].bss;
				rRoamInfo.links[id].channel = links[i].channel;
			}
		} else
#endif /*  (CFG_SUPPORT_802_11BE_MLO == 1) */
		{
			rRoamInfo.ap_mld_addr = NULL;
			rRoamInfo.valid_links = 0;
			rRoamInfo.links[0].addr = links[0].addr;
			rRoamInfo.links[0].bssid = links[0].bssid;
			rRoamInfo.links[0].bss = links[0].bss;
			rRoamInfo.links[0].channel = links[0].channel;
		}
#else /* (CFG_ADVANCED_80211_MLO == 1) || 6.0.0 <= CFG80211_VERSION_CODE */
		rRoamInfo.bss = links[0].bss;
#endif /* (CFG_ADVANCED_80211_MLO == 1) || 6.0.0 <= CFG80211_VERSION_CODE */

		rRoamInfo.req_ie = prConnSettings->aucReqIe;
		rRoamInfo.req_ie_len = prConnSettings->u4ReqIeLength;
		rRoamInfo.resp_ie = prConnSettings->aucRspIe;
		rRoamInfo.resp_ie_len =	prConnSettings->u4RspIeLength;
#if KERNEL_VERSION(4, 15, 0) > CFG80211_VERSION_CODE
		rRoamInfo.authorized = ucAuthorized;
#endif /* KERNEL_VERSION(4, 15, 0) > CFG80211_VERSION_CODE */

#if (CFG_SUPPORT_FILS_SK_OFFLOAD == 1)
		kalReportFilsInfoAndAddRemoveKey(prAdapter,
			&rRoamInfo.fils, TRUE, ucBssIndex);
#endif /* (CFG_SUPPORT_FILS_SK_OFFLOAD == 1) */

		cfg80211_roamed(netdev, &rRoamInfo, GFP_KERNEL);
#if KERNEL_VERSION(4, 15, 0) <= CFG80211_VERSION_CODE
		if (ucAuthorized) {
#if (KERNEL_VERSION(6, 2, 0) <= CFG80211_VERSION_CODE) || \
	(CFG_ADVANCED_80211_MLO == 1)
			cfg80211_port_authorized(netdev,
				links[0].bssid, NULL, 0, GFP_KERNEL);
#else
			cfg80211_port_authorized(netdev,
				links[0].bssid, GFP_KERNEL);
#endif
		}
#endif /* KERNEL_VERSION(4, 15, 0) <= CFG80211_VERSION_CODE */
#else /* KERNEL_VERSION(4, 12, 0) <= CFG80211_VERSION_CODE */
		cfg80211_roamed_bss(
			netdev,
			links[0].bss, /* always use main link */
			prConnSettings->aucReqIe,
			prConnSettings->u4ReqIeLength,
			prConnSettings->aucRspIe,
			prConnSettings->u4RspIeLength,
			GFP_KERNEL);
#endif /* KERNEL_VERSION(4, 12, 0) <= CFG80211_VERSION_CODE */
	} else {
		uint16_t u2JoinStatus;
#if KERNEL_VERSION(4, 18, 0) < CFG80211_VERSION_CODE
		struct cfg80211_connect_resp_params params;
#endif

		if (eStatus == WLAN_STATUS_MEDIA_CONNECT) {
			u2JoinStatus = WLAN_STATUS_SUCCESS;
		} else {
			if (prConnSettings->u2JoinStatus !=
					STATUS_CODE_AUTH_TIMEOUT &&
			    prConnSettings->u2JoinStatus !=
					STATUS_CODE_ASSOC_TIMEOUT &&
			    prConnSettings->u2JoinStatus !=
					STATUS_CODE_SUCCESSFUL)
				u2JoinStatus = prConnSettings->u2JoinStatus;
			else
#if CFG_SUPPORT_WPA3_LOG
				u2JoinStatus = wpa3LogJoinFailStatus(prAdapter,
					prBssInfo);
#else
				u2JoinStatus = WLAN_STATUS_AUTH_TIMEOUT;
#endif
		}

#if KERNEL_VERSION(4, 18, 0) < CFG80211_VERSION_CODE
		kalMemSet(&params, 0, sizeof(params));
		params.status = u2JoinStatus;
		params.req_ie = prConnSettings->aucReqIe;
		params.req_ie_len = prConnSettings->u4ReqIeLength;
		params.resp_ie = prConnSettings->aucRspIe;
		params.resp_ie_len = prConnSettings->u4RspIeLength;
		params.timeout_reason = NL80211_TIMEOUT_UNSPECIFIED;

#if ((CFG_ADVANCED_80211_MLO == 1) || \
	(KERNEL_VERSION(6, 0, 0) <= CFG80211_VERSION_CODE))

#if (CFG_SUPPORT_802_11BE_MLO == 1)
		if (prMldStaRec) {
			params.ap_mld_addr = prMldStaRec->aucPeerMldAddr;
			params.valid_links = valid_links;
			for (i = 0; i < MLD_LINK_MAX; i++) {
				uint8_t id = links[i].link_id;

				if (!links[i].used)
					continue;

				params.links[id].addr = links[i].addr;
				params.links[id].bssid = links[i].bssid;
				params.links[id].bss = links[i].bss;
			}

			DBGLOG(INIT, INFO, "JOIN %s: MLD "MACSTR" Status=%d",
				u2JoinStatus == WLAN_STATUS_SUCCESS ?
				"Success" : "Failure",
				MAC2STR(params.ap_mld_addr), u2JoinStatus);
		} else
#endif /*  (CFG_SUPPORT_802_11BE_MLO == 1) */
		{
			params.links[0].bssid = links[0].bssid;
			params.links[0].bss = links[0].bss;

			DBGLOG(INIT, INFO, "JOIN %s: AP "MACSTR" Status=%d",
				u2JoinStatus == WLAN_STATUS_SUCCESS ?
				"Success" : "Failure",
				MAC2STR(links[0].bssid), u2JoinStatus);
		}
#else /* (CFG_ADVANCED_80211_MLO == 1) || 6.0.0 <= CFG80211_VERSION_CODE */
		params.bssid =  links[0].bssid;
		params.bss = links[0].bss;
#endif /* (CFG_ADVANCED_80211_MLO == 1) || 6.0.0 <= CFG80211_VERSION_CODE */

#if (CFG_SUPPORT_FILS_SK_OFFLOAD == 1)
		kalReportFilsInfoAndAddRemoveKey(prAdapter,
			&params.fils, u2JoinStatus == WLAN_STATUS_SUCCESS,
			ucBssIndex);
#endif /* (CFG_SUPPORT_FILS_SK_OFFLOAD == 1) */

		cfg80211_connect_done(netdev, &params, GFP_KERNEL);

#else /* KERNEL_VERSION(4, 18, 0) < CFG80211_VERSION_CODE */
		DBGLOG(INIT, INFO, "JOIN %s: AP "MACSTR" Status=%d",
			u2JoinStatus == WLAN_STATUS_SUCCESS ?
			"Success" : "Failure",
			MAC2STR(links[0].bssid), u2JoinStatus);

		cfg80211_connect_result(
			netdev,
			links[0].bssid,
			prConnSettings->aucReqIe,
			prConnSettings->u4ReqIeLength,
			prConnSettings->aucRspIe,
			prConnSettings->u4RspIeLength,
			u2JoinStatus,
			GFP_KERNEL);

		if (links[0].bss)
			cfg80211_put_bss(wlanGetWiphy(), links[0].bss);
#endif /* KERNEL_VERSION(4, 18, 0) < CFG80211_VERSION_CODE */
	}

	return WLAN_STATUS_SUCCESS;
}

void kalInformFtEvent(struct GLUE_INFO *prGlueInfo, uint8_t ucBssIndex)
{
	struct ADAPTER *prAdapter;
	struct net_device *prDevHandler;
	struct FT_EVENT_PARAMS *ft_param;
	struct cfg80211_ft_event_params ft_event = {0};
	uint8_t *buf, *pos;
	uint16_t len;

	prAdapter = prGlueInfo->prAdapter;
	prDevHandler = wlanGetNetDev(prGlueInfo, ucBssIndex);
	ft_param = aisGetFtEventParam(prAdapter, ucBssIndex);

	if (!prDevHandler) {
		DBGLOG(INIT, ERROR,
			"ucBssIndex=%u, prDevHandler=%p\n",
			ucBssIndex, prDevHandler);
		return;
	}

	if (!ft_param->pcIe || !ft_param->u2IeLen || !ft_param->prTargetAp) {
		DBGLOG(INIT, INFO, "invalid ft param\n");
		return;
	}

	len = ft_param->u2IeLen + 255; /* 255 for mtk mlo link info */
	buf = kalMemZAlloc(len, VIR_MEM_TYPE);
	if (buf == NULL) {
		DBGLOG(INIT, INFO, "Can't allocate memory\n");
		return;
	}
	kalMemCopy(buf, ft_param->pcIe, ft_param->u2IeLen);
	pos = buf + ft_param->u2IeLen;

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	if (mldIsMultiLinkFormed(prAdapter, ft_param->prTargetAp)) {
		struct MLD_BSS_INFO *mld_bssinfo;
		struct BSS_INFO *bssinfo =
			aisGetAisBssInfo(prAdapter, ucBssIndex);
		struct MLD_STA_RECORD *mld_starec;
		struct STA_RECORD *starec;
		uint8_t *vendor_ie = pos;
		uint8_t aucMtkOui[] = VENDOR_OUI_MTK;
		struct IE_MTK_PRE_WIFI7 *pre;
		struct IE_MULTI_LINK_CONTROL *common;
		uint16_t present = 0;

		mld_bssinfo = mldBssGetByBss(prAdapter, bssinfo);
		mld_starec = mldStarecGetByStarec(prAdapter,
			ft_param->prTargetAp);

		if (!mld_starec || !mld_bssinfo) {
			DBGLOG(INIT, WARN, "mld_starec=%p, mld_bssinfo=%p\n",
					mld_starec, mld_bssinfo);
			goto mld_formed_end;
		}

		/* Vendor[PRE_WIFI7[ML[STA Profile * N]]] */

		MTK_OUI_IE(vendor_ie)->ucId = ELEM_ID_VENDOR;
		MTK_OUI_IE(vendor_ie)->ucLength = ELEM_MIN_LEN_MTK_OUI;
		MTK_OUI_IE(vendor_ie)->aucOui[0] = aucMtkOui[0];
		MTK_OUI_IE(vendor_ie)->aucOui[1] = aucMtkOui[1];
		MTK_OUI_IE(vendor_ie)->aucOui[2] = aucMtkOui[2];
		MTK_OUI_IE(vendor_ie)->aucCapability[0] |=
			MTK_SYNERGY_CAP_SUPPORT_TLV;
		pos += IE_SIZE(vendor_ie);

		pre = (struct IE_MTK_PRE_WIFI7 *) pos;
		pre->ucId = MTK_OUI_ID_PRE_WIFI7;
		pre->ucLength = 2;
		pre->ucVersion0 = 0;
		pre->ucVersion1 = 2;
		pos += IE_SIZE(pre);

		common = (struct IE_MULTI_LINK_CONTROL *) pos;
		common->ucId = ELEM_ID_RESERVED;
		common->ucExtId = ELEM_EXT_ID_MLD;

		present |= ML_CTRL_LINK_ID_INFO_PRESENT;

		BE_SET_ML_CTRL_TYPE(common->u2Ctrl, ML_CTRL_TYPE_BASIC);
		BE_SET_ML_CTRL_PRESENCE(common->u2Ctrl, present);

		/* filling common info field*/
		pos = common->aucCommonInfo;
		pos++; /* reserve for common info length */

		COPY_MAC_ADDR(pos, mld_bssinfo->aucOwnMldAddr);
		pos += MAC_ADDR_LEN;
		*pos++ = bssinfo->ucLinkIndex;

		/* update common info length, ie length, frame length */
		*common->aucCommonInfo = pos - common->aucCommonInfo;
		common->ucLength = pos - (uint8_t *) common - ELEM_HDR_LEN;

		/* use starec lists to loop all new roaming target APs */
		LINK_FOR_EACH_ENTRY(starec, &mld_starec->rStarecList,
			rLinkEntryMld, struct STA_RECORD) {
			struct IE_ML_STA_CONTROL *sta_ctrl =
				(struct IE_ML_STA_CONTROL *) pos;
			uint16_t control = 0;

			bssinfo = GET_BSS_INFO_BY_INDEX(prAdapter,
				starec->ucBssIndex);

			if (!bssinfo || bssinfo->ucBssIndex == ucBssIndex)
				continue;

			sta_ctrl->ucSubID = SUB_IE_MLD_PER_STA_PROFILE;

			/* filling STA control field (fixed length) */
			BE_SET_ML_STA_CTRL_LINK_ID(control,
				bssinfo->ucLinkIndex);

			/*
			 * meaningful if NSTR Link Pair Present is 1
			 * Bitmap subfield: 0 = 1 octet, 1 = 2 octets
			 */
			control |= (ML_STA_CTRL_MAC_ADDR_PRESENT);

			BE_SET_ML_STA_CTRL_FIELD(sta_ctrl->u2StaCtrl, control);

			/* filling STA info field (varied length) */
			pos = sta_ctrl->aucStaInfo;
			pos++; /* reserved for sta info length */

			COPY_MAC_ADDR(pos, bssinfo->aucOwnMacAddr);
			pos += MAC_ADDR_LEN;

			*sta_ctrl->aucStaInfo = pos - sta_ctrl->aucStaInfo;
			sta_ctrl->ucLength = pos - (uint8_t *)sta_ctrl - 2;
			common->ucLength += IE_SIZE(sta_ctrl);
		}

		pre->ucLength += IE_SIZE(common);
		MTK_OUI_IE(vendor_ie)->ucLength += IE_SIZE(pre);

		DBGLOG(INIT, INFO, "FT: MTK_PRE_WIFI7");
		DBGLOG_MEM8(INIT, INFO, vendor_ie, IE_SIZE(vendor_ie));
	}

mld_formed_end:
#endif

	ft_event.ies = buf;
	ft_event.ies_len = pos - buf;
#if (KERNEL_VERSION(6, 1, 0) <= CFG80211_VERSION_CODE)
	ft_event.target_ap =
		cnmStaRecAuthAddr(prAdapter,
			ft_param->prTargetAp);
#else
	ft_event.target_ap =
		ft_param->prTargetAp->aucMacAddr;
#endif
	ft_event.ric_ies = ft_param->pcRicIes;
	ft_event.ric_ies_len = ft_param->u2RicIesLen;

	cfg80211_ft_event(prDevHandler, &ft_event);

	kalMemFree(buf, VIR_MEM_TYPE, len);
}

#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
void kalUpdate6GPwrMode(struct ADAPTER *prAdapter, uint8_t ucBssIndex)
{
	struct AIS_FSM_INFO *prAisFsmInfo = NULL;
	struct BSS_INFO *prBssInfo = NULL;
	struct BSS_DESC *prBssDesc = NULL;
	uint8_t i = 0;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	for (i = 0; i < MLD_LINK_MAX; i++) {
		prBssInfo = aisGetLinkBssInfo(prAisFsmInfo, i);
		prBssDesc = aisGetLinkBssDesc(prAisFsmInfo, i);
		if (!prBssInfo || !prBssDesc)
			continue;

		rlmDomain6GPwrModeUpdate(prAdapter,
			prBssInfo->ucBssIndex,
			prBssDesc->e6GPwrMode);
	}
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief Called by driver to indicate event to upper layer, for example, the
 *        wpa supplicant or wireless tools.
 *
 * \param[in] pvAdapter Pointer to the adapter descriptor.
 * \param[in] eStatus Indicated status.
 * \param[in] pvBuf Indicated message buffer.
 * \param[in] u4BufLen Indicated message buffer size.
 *
 * \return (none)
 *
 */
/*----------------------------------------------------------------------------*/
void kalIndicateStatusAndComplete(struct GLUE_INFO *prGlueInfo,
				uint32_t eStatus, void *pvBuf,
				uint32_t u4BufLen, uint8_t ucBssIndex)
{
	uint32_t bufLen = 0;
	struct PARAM_STATUS_INDICATION *pStatus;
	struct PARAM_AUTH_EVENT *pAuth;
	struct PARAM_PMKID_CANDIDATE_LIST *pPmkid;
	struct BSS_INFO *prBssInfo;
	struct PARAM_BSSID_EX *prCurrBssid;
	struct ADAPTER *prAdapter = NULL;
	uint8_t fgScanAborted = FALSE;
	struct net_device *prDevHandler;
	struct BSS_DESC *prBssDesc = NULL;
#if (CFG_SUPPORT_TX_PWR_ENV == 1)
	int8_t aicTxPwrEnvMaxTxPwr[TX_PWR_ENV_MAX_TXPWR_BW_NUM];
#endif
#if CFG_ENABLE_WIFI_DIRECT
	struct AIS_FSM_INFO *prAisFsmInfo;
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	struct MLD_BSS_INFO *prMldBss = NULL;
#endif
	struct BSS_INFO *bss;
#endif
	GLUE_SPIN_LOCK_DECLARATION();

	ASSERT(prGlueInfo);
	prAdapter = prGlueInfo->prAdapter;
	ASSERT(prAdapter);

	pStatus = (struct PARAM_STATUS_INDICATION *)pvBuf;
	pAuth = (struct PARAM_AUTH_EVENT *)pStatus;
	pPmkid = (struct PARAM_PMKID_CANDIDATE_LIST *)(pStatus + 1);
	prDevHandler = wlanGetNetDev(prGlueInfo, ucBssIndex);
	prBssDesc = aisGetTargetBssDesc(prAdapter, ucBssIndex);
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
#if CFG_ENABLE_WIFI_DIRECT
	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	bss = prBssInfo;
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	prMldBss = mldBssGetByBss(prAdapter, prBssInfo);
#endif
#endif
	if (!prDevHandler || !prBssInfo) {
		DBGLOG(INIT, ERROR,
			"ucBssIndex=%u, prDevHandler=%p, prBssInfo=%p\n",
			ucBssIndex, prDevHandler, prBssInfo);
		return;
	}

	switch (eStatus) {
	case WLAN_STATUS_ROAM_OUT_FIND_BEST:
	case WLAN_STATUS_MEDIA_CONNECT:
#if CFG_SUPPORT_LINK_QUALITY_MONITOR
		/* clear the count */
		prGlueInfo->prAdapter->rLinkQualityInfo.u8TxTotalCount = 0;
		prGlueInfo->prAdapter->rLinkQualityInfo.u8RxTotalCount = 0;
		prGlueInfo->prAdapter->rLinkQualityInfo.u8RxErrCount = 0;
#endif /* CFG_SUPPORT_LINK_QUALITY_MONITOR */
		kalSetMediaStateIndicated(prGlueInfo,
			MEDIA_STATE_CONNECTED,
			ucBssIndex);

		prCurrBssid = aisGetCurrBssId(prGlueInfo->prAdapter,
			ucBssIndex);

		wext_indicate_wext_event(prGlueInfo, SIOCGIWAP,
					prCurrBssid->arMacAddress,
					bufLen, ucBssIndex);

		/* switch netif on */
		netif_carrier_on(prDevHandler);

		wlanDfsChannelsNotifyStaConnected(prAdapter,
			AIS_INDEX(prAdapter, prBssInfo->ucBssIndex));

		do {
			uint8_t aucSsid[PARAM_MAX_LEN_SSID + 1] = {0};

			kalStrnCpy(aucSsid, prCurrBssid->rSsid.aucSsid,
				sizeof(aucSsid) - 1);
			aucSsid[sizeof(aucSsid) - 1] = '\0';

			DBGLOG(INIT, INFO,
				"[wifi] %s netif_carrier_on [ssid:%s " MACSTR
				"], Mac:" MACSTR "\n",
				prDevHandler->name, aucSsid,
				MAC2STR(prCurrBssid->arMacAddress),
				MAC2STR(prBssInfo->aucOwnMacAddr));

		} while (0);

		if (prGlueInfo->fgIsRegistered == TRUE) {
			kalReportAllLinkInfo(prGlueInfo->prAdapter,
				prDevHandler, eStatus, pvBuf,
				u4BufLen, ucBssIndex);

#if CFG_ENABLE_WIFI_DIRECT
			if (prAisFsmInfo &&
				timerPendingTimer(
				&prAisFsmInfo->rJoinTimeoutTimer))
				prAisFsmInfo->ucIsSapCsaPending = TRUE;
			else
				ccmChannelSwitchProducer(prAdapter, prBssInfo,
							 __func__);
#endif /* CFG_ENABLE_WIFI_DIRECT */
		}
#if (CFG_SUPPORT_802_11AX == 1)
		if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.fgEnableSR))
			rlmSetSrControl(prAdapter, TRUE);
#endif
		if (prBssDesc) {
#if (CFG_SUPPORT_TX_PWR_ENV == 1)
			/* Set Transmit Power Envelope TxPower limit */
			if (prBssDesc->fgIsTxPwrEnvPresent) {
				rlmTxPwrEnvMaxPwrSend(
					prAdapter,
					prBssDesc->eBand,
					prBssDesc->ucChannelNum,
					prBssDesc->ucTxPwrEnvPwrLmtNum,
					prBssDesc->aicTxPwrEnvMaxTxPwr,
					TRUE);
			}
#endif

#if CFG_SUPPORT_802_11K
			/* Set Country IE + Power Constraint TxPower limit */
			if (prBssDesc->cPowerLimit != RLM_INVALID_POWER_LIMIT) {
				rlmSetMaxTxPwrLimit(prAdapter,
					prBssDesc->cPowerLimit, 1);
			}
#endif
		}

#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
		kalUpdate6GPwrMode(prAdapter, ucBssIndex);
#endif

		break;

	case WLAN_STATUS_MEDIA_DISCONNECT:
	case WLAN_STATUS_MEDIA_DISCONNECT_LOCALLY:
#if CFG_SUPPORT_LINK_QUALITY_MONITOR
		/* clear the count */
		prGlueInfo->prAdapter->rLinkQualityInfo.u8TxTotalCount = 0;
		prGlueInfo->prAdapter->rLinkQualityInfo.u8RxTotalCount = 0;
		prGlueInfo->prAdapter->rLinkQualityInfo.u8RxErrCount = 0;
#endif
		if (ucBssIndex < MAX_BSSID_NUM) {
			prGlueInfo->u4TxLinkSpeedCache[ucBssIndex] = 0;
			prGlueInfo->u4RxLinkSpeedCache[ucBssIndex] = 0;
		}

		/* indicate disassoc event */
		wext_indicate_wext_event(prGlueInfo, SIOCGIWAP, NULL, 0
			, ucBssIndex);
		/* For CR 90 and CR99, While supplicant do reassociate, driver
		 * will do netif_carrier_off first,
		 * after associated success, at joinComplete(),
		 * do netif_carier_on,
		 * but for unknown reason, the supplicant 1x pkt will not
		 * called the driver hardStartXmit, for template workaround
		 * these bugs, add this compiling flag
		 */
		/* switch netif off */

#if 1				/* CONSOLE_MESSAGE */
		DBGLOG(INIT, INFO, "[wifi] %s netif_carrier_off\n",
		       prDevHandler->name);
#endif

		netif_carrier_off(prDevHandler);

		wlanDfsChannelsNotifyStaDisconnected(prAdapter,
			AIS_INDEX(prAdapter, prBssInfo->ucBssIndex));
		ccmChannelSwitchProducerDfs(prAdapter, prBssInfo);

		/* Full2Partial: reset */
		if (prGlueInfo->prAdapter) {
			struct SCAN_INFO *prScanInfo =
				&(prGlueInfo->prAdapter->rWifiVar.rScanInfo);
			prScanInfo->fgIsScanForFull2Partial = FALSE;
			prScanInfo->u4LastFullScanTime = 0;
		}

		if (prGlueInfo->fgIsRegistered == TRUE) {
			uint16_t u2DeauthReason = 0;
#if CFG_WPS_DISCONNECT || (KERNEL_VERSION(4, 4, 0) <= CFG80211_VERSION_CODE)
			uint8_t *pDeauthIe = NULL;
			uint32_t u2DeauthLen = 0;

			u2DeauthReason = prBssInfo->u2DeauthReason;

#if CFG_SUPPORT_BIGDATA_PIP
			{
			struct _REPORT_DISCONNECT {
				uint16_t	u2Id;
				uint16_t	u2Len;
				uint8_t  ucReasonType;
				uint16_t  u2ReasonCode;
				int8_t  cRssi;
				uint16_t  u2DataRate;
				uint8_t  ucChannelNo;
			} __KAL_ATTRIB_PACKED__;

			struct _REPORT_DISCONNECT rPayload = {0};
			int8_t ret = 0;

			/* u2Id: Define category of report data.
			 * 0 - Antswp
			 * 1 - GPS Blank On
			 * 2 - Disconnect Reason
			 * others - Reserved
			 *
			 * u2Len: length of data.
			 * Report data format:
			 * | u2Id | u2Len | data |
			 */
			rPayload.u2Id = 2;
			rPayload.u2Len = 7;

			if (eStatus ==
			WLAN_STATUS_MEDIA_DISCONNECT_LOCALLY) {
				rPayload.ucReasonType = 2;
				rPayload.u2ReasonCode = 0;
			} else if (u2DeauthReason >=
				REASON_CODE_BEACON_TIMEOUT*100) {
				rPayload.ucReasonType = 0;
				rPayload.u2ReasonCode =
				u2DeauthReason - REASON_CODE_BEACON_TIMEOUT*100;
			} else {
				rPayload.ucReasonType = 1;
				rPayload.u2ReasonCode = u2DeauthReason;
			}

			if (prGlueInfo->prAdapter &&
			    ucBssIndex < MAX_BSSID_NUM) {
				rPayload.cRssi =
					prGlueInfo->prAdapter->rLinkQuality
					.rLq[ucBssIndex].cRssi;
				rPayload.u2DataRate =
					prGlueInfo->prAdapter->rLinkQuality
					.rLq[ucBssIndex].u2TxLinkSpeed;

				if (prBssDesc)
					rPayload.ucChannelNo =
						prBssDesc->ucChannelNum;

				DBGLOG(INIT, TRACE,
		    "[D2F]type(%u)-reason(%u)-rssi(%d)-speed(%u)-channel(%u)\n",
					rPayload.ucReasonType,
					rPayload.u2ReasonCode,
					rPayload.cRssi,
					rPayload.u2DataRate,
					rPayload.ucChannelNo);

				/* dumpMemory8((uint8_t *)&rPayload,
				 * sizeof(rPayload));
				 */

				ret = kalBigDataPip(prGlueInfo->prAdapter,
					(uint8_t *)&rPayload,
					sizeof(rPayload));

				if ((ret != 1) && (ret != -ETIME))
					DBGLOG(INIT, ERROR,
					"[D2F]data pip report fail(%d).\n",
					ret);

			}
			}
#endif

#if (CFG_SUPPORT_FILS_SK_OFFLOAD == 1)
			kalReportFilsInfoAndAddRemoveKey(prAdapter, NULL,
				FALSE, ucBssIndex);
#endif /* CFG_SUPPORT_FILS_SK_OFFLOAD */

#if CFG_SUPPORT_ASSURANCE
			if (eStatus != WLAN_STATUS_MEDIA_DISCONNECT_LOCALLY &&
				prBssInfo->u4DeauthIeLength != 0) {
				DBGLOG(INIT, INFO, "Dump: Deauth IE to upper");
				DBGLOG_MEM8(INIT, INFO, prBssInfo->aucDeauthIe,
					prBssInfo->u4DeauthIeLength);

				pDeauthIe = prBssInfo->aucDeauthIe;
				u2DeauthLen = prBssInfo->u4DeauthIeLength;
			}
#endif

			/* CFG80211 Indication */
			DBGLOG(INIT, INFO,
			    "[wifi]Indicate disconnection: Reason=%d Locally[%d]\n",
			    u2DeauthReason,
			    (eStatus ==
				WLAN_STATUS_MEDIA_DISCONNECT_LOCALLY));
			cfg80211_disconnected(prDevHandler,
			    u2DeauthReason, pDeauthIe, u2DeauthLen,
			    eStatus == WLAN_STATUS_MEDIA_DISCONNECT_LOCALLY,
			    GFP_KERNEL);

#else

#if CFG_MTK_ANDROID_WMT
#if KERNEL_VERSION(3, 10, 0) == LINUX_VERSION_CODE
			/* Don't indicate disconnection to upper layer for
			 * ANDROID kernel 3.10
			 */
			/* since cfg80211 will indicate disconnection to
			 * wpa_supplicant for this kernel
			 */
			if (eStatus == WLAN_STATUS_MEDIA_DISCONNECT)
#endif
#endif
			{
				u2DeauthReason = prBssInfo->u2DeauthReason;
				/* CFG80211 Indication */
				cfg80211_disconnected(prDevHandler,
						      u2DeauthReason, NULL, 0,
						      GFP_KERNEL);
			}


#endif
		}

		kalSetMediaStateIndicated(prGlueInfo,
			MEDIA_STATE_DISCONNECTED,
			ucBssIndex);

#if CFG_ENABLE_WIFI_DIRECT
		/* Check SAP channel */
		p2pFuncSwitchSapChannel(prAdapter,
			P2P_DEFAULT_SCENARIO);
#endif

		if (prBssDesc) {
#if (CFG_SUPPORT_TX_PWR_ENV == 1)
			/* Disable Transmit Envelope Power limit */
			if (prBssDesc->fgIsTxPwrEnvPresent) {
				rlmTxPwrEnvMaxPwrInit(aicTxPwrEnvMaxTxPwr);
				rlmTxPwrEnvMaxPwrSend(
					prAdapter,
					BAND_NULL,
					0,
					0,
					aicTxPwrEnvMaxTxPwr,
					FALSE);
			}
#endif

#if CFG_SUPPORT_802_11K
			/* Disable Country IE + Power Constraint TxPwr limit */
			if (prBssDesc->cPowerLimit != RLM_INVALID_POWER_LIMIT)
				rlmSetMaxTxPwrLimit(prAdapter, 0, 0);
#endif
		}
		break;

	case WLAN_STATUS_SCAN_COMPLETE:
		if (pvBuf && u4BufLen == sizeof(uint8_t))
			fgScanAborted = *(uint8_t *)pvBuf;

		/* indicate scan complete event */
		wext_indicate_wext_event(prGlueInfo, SIOCGIWSCAN, NULL, 0,
			ucBssIndex);

		if (fgScanAborted == FALSE) {
			kalScanLogCacheFlushBSS(prGlueInfo->prAdapter,
				SCAN_LOG_MSG_MAX_LEN);
			scanlog_dbg(LOG_SCAN_DONE_D2K, INFO, "Call cfg80211_scan_done (aborted=%u)\n",
				fgScanAborted);
		} else {
			scanlog_dbg(LOG_SCAN_ABORT_DONE_D2K, INFO, "Call cfg80211_scan_done (aborted=%u)\n",
				fgScanAborted);
		}

		/* 1. reset first for newly incoming request */
		GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
		if (prGlueInfo->prScanRequest != NULL) {
			kalCfg80211ScanDone(prGlueInfo->prScanRequest,
				fgScanAborted);
			prGlueInfo->prScanRequest = NULL;
		}
		GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

		break;

#if 0
	case WLAN_STATUS_MSDU_OK:
		if (netif_running(prDevHandler))
			netif_wake_queue(prDevHandler);
		break;
#endif

	case WLAN_STATUS_MEDIA_SPECIFIC_INDICATION:
		if (pStatus) {
			switch (pStatus->eStatusType) {
			case ENUM_STATUS_TYPE_AUTHENTICATION:
			{
				struct PARAM_INDICATION_EVENT *prEvent =
				(struct PARAM_INDICATION_EVENT *) pvBuf;

				/* indicate (UC/GC) MIC ERROR event only */
				if ((prEvent->rAuthReq.u4Flags ==
				     PARAM_AUTH_REQUEST_PAIRWISE_ERROR) ||
				    (prEvent->rAuthReq.u4Flags ==
				     PARAM_AUTH_REQUEST_GROUP_ERROR)) {
					cfg80211_michael_mic_failure(
					    prDevHandler, NULL,
					    (prEvent->rAuthReq.u4Flags ==
					    PARAM_AUTH_REQUEST_PAIRWISE_ERROR)
						? NL80211_KEYTYPE_PAIRWISE :
						NL80211_KEYTYPE_GROUP,
					    0, NULL, GFP_KERNEL);
					wext_indicate_wext_event(prGlueInfo,
					    IWEVMICHAELMICFAILURE,
					    (unsigned char *)
						&prEvent->rAuthReq,
					    prEvent->rAuthReq.u4Length,
					    ucBssIndex);
				}
				break;
			}
			case ENUM_STATUS_TYPE_CANDIDATE_LIST:
			{
				struct PARAM_INDICATION_EVENT *prEvent =
				(struct PARAM_INDICATION_EVENT *) pvBuf;

				cfg80211_pmksa_candidate_notify(
					prDevHandler,
					1000,
					prEvent->rCandi.arBSSID,
					prEvent->rCandi.u4Flags,
					GFP_KERNEL);

				wext_indicate_wext_event(
					prGlueInfo,
					IWEVPMKIDCAND,
					(unsigned char *) &prEvent->rCandi,
					sizeof(struct PARAM_PMKID_CANDIDATE),
					ucBssIndex);

				break;
			}
			case ENUM_STATUS_TYPE_FT_AUTH_STATUS: {
				kalInformFtEvent(prGlueInfo, ucBssIndex);
			}
				break;

			default:
				/* case ENUM_STATUS_TYPE_MEDIA_STREAM_MODE */
				break;
			}
		} else {

		}
		break;

#if CFG_SUPPORT_BCM && CFG_SUPPORT_BCM_BWCS
	case WLAN_STATUS_BWCS_UPDATE: {
		wext_indicate_wext_event(prGlueInfo, IWEVCUSTOM, pvBuf,
					 sizeof(struct PTA_IPC), ucBssIndex);
	}

	break;

#endif
	case WLAN_STATUS_JOIN_FAILURE: {
		struct CONNECTION_SETTINGS *prConnSettings =
			aisGetConnSettings(prAdapter, ucBssIndex);
		struct GL_WPA_INFO *prWpaInfo =
			aisGetWpaInfo(prAdapter, ucBssIndex);

		COPY_SSID(prBssInfo->aucSSID,
			prBssInfo->ucSSIDLen,
			prConnSettings->aucSSID,
			prConnSettings->ucSSIDLen);
		COPY_MAC_ADDR(prBssInfo->aucBSSID,
			prConnSettings->aucJoinBSSID);

		/* Make sure we remove all WEP key */
		if (prWpaInfo && prWpaInfo->u4WpaVersion ==
			IW_AUTH_WPA_VERSION_DISABLED
			&& prBssInfo->wepkeyWlanIdx < WTBL_SIZE) {
			uint32_t keyId;
			uint32_t u4SetLen;
			struct PARAM_REMOVE_KEY rRemoveKey;

			for (keyId = 0; keyId <= 3; keyId++) {
				if (!prBssInfo->wepkeyUsed[keyId])
					continue;

				rRemoveKey.u4Length =
					sizeof(struct PARAM_REMOVE_KEY);
				rRemoveKey.u4KeyIndex = keyId;
				rRemoveKey.ucBssIdx = ucBssIndex;
				COPY_MAC_ADDR(rRemoveKey.arBSSID,
					      prBssInfo->aucBSSID);
				DBGLOG(INIT, INFO,
					"JOIN Failure: remove WEP wlanidx: %d, keyid: %d",
					prBssInfo->wepkeyWlanIdx,
					rRemoveKey.u4KeyIndex);
				wlanSetRemoveKey(prAdapter,
					(void *)&rRemoveKey,
					sizeof(struct PARAM_REMOVE_KEY),
					&u4SetLen, FALSE);
			}
		}

		kalReportAllLinkInfo(prGlueInfo->prAdapter,
				prDevHandler, eStatus, pvBuf,
				u4BufLen, ucBssIndex);

		kalSetMediaStateIndicated(prGlueInfo,
			MEDIA_STATE_DISCONNECTED,
			ucBssIndex);

#if CFG_ENABLE_WIFI_DIRECT
		/* Check SAP channel */
		p2pFuncSwitchSapChannel(prAdapter,
			P2P_DEFAULT_SCENARIO);
#endif

		if (prBssDesc) {
#if (CFG_SUPPORT_TX_PWR_ENV == 1)
			/* Disable Transmit Envelope Power limit */
			if (prBssDesc->fgIsTxPwrEnvPresent) {
				rlmTxPwrEnvMaxPwrInit(aicTxPwrEnvMaxTxPwr);
				rlmTxPwrEnvMaxPwrSend(
					prAdapter,
					BAND_NULL,
					0,
					0,
					aicTxPwrEnvMaxTxPwr,
					FALSE);
			}
#endif

#if CFG_SUPPORT_802_11K
			/* Disable Country IE + Power Constraint TxPwr limit */
			if (prBssDesc->cPowerLimit != RLM_INVALID_POWER_LIMIT)
				rlmSetMaxTxPwrLimit(prAdapter, 0, 0);
#endif
		}
		break;
	}
	default:
		break;
	}
}				/* kalIndicateStatusAndComplete */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to update the (re)association request
 *        information to the structure used to query and set
 *        OID_802_11_ASSOCIATION_INFORMATION.
 *
 * \param[in] prGlueInfo Pointer to the Glue structure.
 * \param[in] pucFrameBody Pointer to the frame body of the last
 *                         (Re)Association Request frame from the AP.
 * \param[in] u4FrameBodyLen The length of the frame body of the last
 *                           (Re)Association Request frame.
 * \param[in] fgReassocRequest TRUE, if it is a Reassociation Request frame.
 *
 * \return (none)
 *
 */
/*----------------------------------------------------------------------------*/
void
kalUpdateReAssocReqInfo(struct GLUE_INFO *prGlueInfo,
			uint8_t *pucFrameBody, uint32_t u4FrameBodyLen,
			u_int8_t fgReassocRequest,
			uint8_t ucBssIndex)
{
	uint8_t *cp;
	struct CONNECTION_SETTINGS *prConnSettings = NULL;

	ASSERT(prGlueInfo);

	prConnSettings = aisGetConnSettings(
		prGlueInfo->prAdapter,
		ucBssIndex);

	if (fgReassocRequest) {
		if (u4FrameBodyLen < 15) {
			return;
		}
	} else {
		if (u4FrameBodyLen < 9) {
			return;
		}
	}

	cp = pucFrameBody;

	if (fgReassocRequest) {
		/* Capability information field 2 */
		/* Listen interval field 2 */
		/* Current AP address 6 */
		cp += 10;
		u4FrameBodyLen -= 10;
	} else {
		/* Capability information field 2 */
		/* Listen interval field 2 */
		cp += 4;
		u4FrameBodyLen -= 4;
	}

	wext_indicate_wext_event(prGlueInfo, IWEVASSOCREQIE, cp,
				 u4FrameBodyLen, ucBssIndex);

	if (u4FrameBodyLen > CONTROL_BUFFER_SIZE) {
		DBGLOG(INIT, WARN, "Assoc Req IE truncated %d to %d",
			u4FrameBodyLen, CONTROL_BUFFER_SIZE);
		u4FrameBodyLen = CONTROL_BUFFER_SIZE;
	}

	if (prConnSettings->u4ReqIeLength > 0) {
		kalMemFree(prConnSettings->aucReqIe, VIR_MEM_TYPE,
			prConnSettings->u4ReqIeLength);
		prConnSettings->u4ReqIeLength = 0;
		prConnSettings->aucReqIe = NULL;
	}

	if (u4FrameBodyLen > 0) {
		prConnSettings->aucReqIe =
			kalMemAlloc(u4FrameBodyLen, VIR_MEM_TYPE);

		if (prConnSettings->aucReqIe) {
			kalMemCopy(prConnSettings->aucReqIe, cp,
				u4FrameBodyLen);
			prConnSettings->u4ReqIeLength = u4FrameBodyLen;
		} else {
			DBGLOG(INIT, ERROR,
				"allocate memory for prConnSettings->aucReqIe failed!\n");
		}
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is called to update the (re)association
 *        response information to the structure used to reply with
 *        cfg80211_connect_result
 *
 * @param prGlueInfo      Pointer to adapter descriptor
 * @param pucFrameBody    Pointer to the frame body of the last (Re)Association
 *                         Response frame from the AP
 * @param u4FrameBodyLen  The length of the frame body of the last
 *                          (Re)Association Response frame
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void kalUpdateReAssocRspInfo(struct GLUE_INFO
			     *prGlueInfo, uint8_t *pucFrameBody,
			     uint32_t u4FrameBodyLen,
			     uint8_t ucBssIndex)
{
	uint32_t u4IEOffset =
		6;	/* cap_info, status_code & assoc_id */
	uint32_t u4IELength = u4FrameBodyLen - u4IEOffset;
	struct CONNECTION_SETTINGS *prConnSettings = NULL;
	struct BSS_INFO *bss =
		GET_BSS_INFO_BY_INDEX(prGlueInfo->prAdapter, ucBssIndex);

	ASSERT(prGlueInfo);

	if (!bss) {
		DBGLOG(ML, INFO, "bss is null\n");
		return;
	}

	if (u4IELength > CONTROL_BUFFER_SIZE) {
		DBGLOG(INIT, WARN, "Assoc Resp IE truncated %d to %d",
			u4IELength, CONTROL_BUFFER_SIZE);
		u4IELength = CONTROL_BUFFER_SIZE;
	}

	DBGLOG(INIT, LOUD, "[%d] Copy assoc resp info\n", ucBssIndex);

	if (IS_BSS_AIS(bss)) {
		prConnSettings = aisGetConnSettings(
			prGlueInfo->prAdapter,
			ucBssIndex);

		if (prConnSettings->u4RspIeLength > 0) {
			kalMemFree(prConnSettings->aucRspIe,
				VIR_MEM_TYPE,
				prConnSettings->u4RspIeLength);
			prConnSettings->u4RspIeLength = 0;
			prConnSettings->aucRspIe = NULL;
		}

		if (u4IELength > 0) {
			prConnSettings->aucRspIe =
				kalMemAlloc(u4IELength, VIR_MEM_TYPE);

			if (prConnSettings->aucRspIe) {
				kalMemCopy(prConnSettings->aucRspIe,
					pucFrameBody + u4IEOffset, u4IELength);
				prConnSettings->u4RspIeLength = u4IELength;
			} else {
				DBGLOG(INIT, ERROR,
					"allocate memory for prConnSettings->aucRspIe failed!\n");
			}
		}
#if CFG_ENABLE_WIFI_DIRECT
	} else if (!IS_BSS_APGO(bss)) {
		struct P2P_ROLE_FSM_INFO *fsm =
			P2P_ROLE_INDEX_2_ROLE_FSM_INFO(
				prGlueInfo->prAdapter,
				bss->u4PrivateData);
		struct P2P_JOIN_INFO *prJoinInfo =
			(struct P2P_JOIN_INFO *) NULL;

		if (!fsm)
			return;

		prJoinInfo = &(fsm->rJoinInfo);

		/* nic_cmd_event defined MAX_IE_LENGTH */
		if (u4IELength > MAX_IE_LENGTH) {
			DBGLOG(INIT, ERROR,
				"Assoc Resp IE truncated %d to %d",
				u4IELength, MAX_IE_LENGTH);
			u4IELength = MAX_IE_LENGTH;
		}

		prJoinInfo->u4BufLength = u4IELength;
		kalMemCopy(prJoinInfo->aucIEBuf,
			pucFrameBody + u4IEOffset,
			u4IELength);
	}
#endif
}				/* kalUpdateReAssocRspInfo */

void kalResetPacket(struct GLUE_INFO *prGlueInfo,
		    void *prPacket)
{
	struct sk_buff *prSkb = (struct sk_buff *)prPacket;

	/* Reset cb */
	kalMemZero(prSkb->cb, sizeof(prSkb->cb));
}

/*----------------------------------------------------------------------------*/
/*
 * \brief This function is to check the pairwise eapol and wapi 1x.
 *
 * \param[in] prPacket  Pointer to struct net_device
 *
 * \retval WLAN_STATUS
 */
/*----------------------------------------------------------------------------*/
u_int8_t kalIsPairwiseEapolPacket(void *prPacket)
{
	struct sk_buff *prSkb = (struct sk_buff *)prPacket;
	uint8_t *pucPacket = (uint8_t *)prSkb->data;
	uint16_t u2EthType = 0;
	uint16_t u2KeyInfo = 0;

	WLAN_GET_FIELD_BE16(&pucPacket[ETHER_HEADER_LEN - ETHER_TYPE_LEN],
			    &u2EthType);
#if CFG_SUPPORT_WAPI
	/* prBssInfo && prBssInfo->eNetworkType == NETWORK_TYPE_AIS &&
	 * wlanQueryWapiMode(prAdapter)
	 */
	if (u2EthType == ETH_WPI_1X)
		return TRUE;
#endif
	if (u2EthType != ETH_P_1X)
		return FALSE;
	u2KeyInfo = pucPacket[5 + ETHER_HEADER_LEN] << 8 |
		    pucPacket[6 + ETHER_HEADER_LEN];
#if 1
	/* BIT3 is pairwise key bit, and check SM is 0.  it means this is 4-way
	 * handshake frame
	 */
	DBGLOG(RSN, INFO, "u2KeyInfo=%d\n", u2KeyInfo);
	if ((u2KeyInfo & BIT(3)) && !(u2KeyInfo & BIT(13)))
		return TRUE;
#else
	/* BIT3 is pairwise key bit, bit 8 is key mic bit.
	 * only the two bits are set, it means this is 4-way handshake 4/4 or
	 * 2/4 frame
	 */
	DBGLOG(RSN, INFO, "u2KeyInfo=%d\n", u2KeyInfo);
	if ((u2KeyInfo & (BIT(3) | BIT(8))) == (BIT(3) | BIT(8)))
		return TRUE;
#endif
	return FALSE;
}

/*----------------------------------------------------------------------------*/
/*
 * \brief This function is TX entry point of NET DEVICE.
 *
 * \param[in] prSkb  Pointer of the sk_buff to be sent
 * \param[in] prDev  Pointer to struct net_device
 * \param[in] prGlueInfo  Pointer of prGlueInfo
 * \param[in] ucBssIndex  BSS index of this net device
 *
 * \retval WLAN_STATUS
 */
/*----------------------------------------------------------------------------*/
uint32_t
kalHardStartXmit(struct sk_buff *prOrgSkb,
		 struct net_device *prDev, struct GLUE_INFO *prGlueInfo,
		 uint8_t ucBssIndex)
{
	struct QUE_ENTRY *prQueueEntry = NULL;
	struct QUE *prTxQueue = NULL;
	uint16_t u2QueueIdx = 0;
	struct sk_buff *prSkbNew = NULL;
	struct sk_buff *prSkb = NULL;
	uint32_t u4SkbLen = 0;
	struct mt66xx_chip_info *prChipInfo;
	uint32_t u4TxHeadRoomSize = 0;
	struct ADAPTER *prAdapter = NULL;
	struct BSS_INFO *prBssInfo = NULL;
	uint32_t u4StopTh;

	ASSERT(prOrgSkb);
	ASSERT(prGlueInfo);

	prAdapter = prGlueInfo->prAdapter;
	prChipInfo = prGlueInfo->prAdapter->chip_info;
	u4TxHeadRoomSize = NIC_TX_DESC_AND_PADDING_LENGTH +
		prChipInfo->txd_append_size;

	if (test_bit(GLUE_FLAG_HALT_BIT, &prGlueInfo->ulFlag)) {
		DBGLOG(INIT, INFO, "GLUE_FLAG_HALT skip tx\n");
		dev_kfree_skb(prOrgSkb);
		return WLAN_STATUS_ADAPTER_NOT_READY;
	}

	if (unlikely(ucBssIndex >= MAX_BSSID_NUM)) {
		DBGLOG(INIT, INFO, "Invalid ucBssIndex:%u\n", ucBssIndex);
		dev_kfree_skb(prOrgSkb);
		return WLAN_STATUS_NOT_ACCEPTED;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
#if (CFG_SINGLE_BAND_MLSR_56 == 1)
	if (prBssInfo) {
		struct MLD_STA_RECORD *prMldStaRec;

		prMldStaRec = mldStarecGetByStarec(prAdapter,
			prBssInfo->prStaRecOfAP);
		if (prMldStaRec && prMldStaRec->fgIsSbMlsr) {
			struct STA_RECORD *prStaRec = cnmGetStaRecByWlanIndex(
				prAdapter, prMldStaRec->u2SecondMldId);

			/* only second link is active, change bssinfo */
			if (prStaRec &&
			    prMldStaRec->u4ActiveStaBitmap ==
				BIT(prStaRec->ucIndex)) {
				ucBssIndex = prStaRec->ucBssIndex;
				prBssInfo = GET_BSS_INFO_BY_INDEX(
					prAdapter, ucBssIndex);
			}
		}
	}
#endif /* CFG_SINGLE_BAND_MLSR_56 */

	if (!prBssInfo) {
		DBGLOG(INIT, INFO, "prBssInfo NULL for ucBssIndex:%u\n",
			ucBssIndex);
		dev_kfree_skb(prOrgSkb);
		return WLAN_STATUS_NOT_ACCEPTED;
	}

	if (prBssInfo->eNetworkType == NETWORK_TYPE_AIS &&
		prBssInfo->eConnectionState != MEDIA_STATE_CONNECTED) {
		DBGLOG(INIT, INFO,
			"ais status is not connected, skip this frame\n");
		dev_kfree_skb(prOrgSkb);
		return WLAN_STATUS_NOT_ACCEPTED;
	}

	if (prGlueInfo->prAdapter->fgIsEnableLpdvt) {
		DBGLOG(INIT, INFO, "LPDVT enable, skip this frame\n");
		dev_kfree_skb(prOrgSkb);
		return WLAN_STATUS_NOT_ACCEPTED;
	}

#if CFG_SUPPORT_WIFI_SYSDVT
	if (prAdapter->u2TxTest != TX_TEST_UNLIMITIED) {
		if (prAdapter->u2TxTestCount < prAdapter->u2TxTest) {
			DBGLOG(INIT, STATE,
				"DVT TX Test enable, skip this frame\n");
			dev_kfree_skb(prOrgSkb);
			return WLAN_STATUS_SUCCESS;
		}
		prAdapter->u2TxTestCount++;
	}
#endif /* CFG_SUPPORT_WIFI_SYSDVT */

	if (skb_headroom(prOrgSkb) < u4TxHeadRoomSize) {
		/*
		 * Should not happen
		 * kernel crash may happen as skb shared info
		 * channged.
		 * offer an change for lucky anyway
		 */
		prSkbNew = skb_realloc_headroom(prOrgSkb, u4TxHeadRoomSize);
		if (!prSkbNew) {
			dev_kfree_skb(prOrgSkb);
			DBGLOG(INIT, ERROR,
				"prChipInfo = %pM, u4TxHeadRoomSize: %u\n",
				prChipInfo, u4TxHeadRoomSize);
			return WLAN_STATUS_NOT_ACCEPTED;
		}
		dev_kfree_skb(prOrgSkb);
		prSkb = prSkbNew;
	} else
		prSkb = prOrgSkb;

#if CFG_SUPPORT_SKB_CLONED_COPY
	/*
	 * When the same pkt is tx to different bssid, the skb is cloned,
	 * and two skb share the same skb->data.
	 * In gen4m, the txd is in the headroom (a pointer to the offset
	 * before skb->data), which means that this two pkts will use the
	 * same location to store txd, so if we find that pkt is cloned,
	 * we need to do a skb_copy to avoid this condition.
	 * Otherwise, the txd of the first pkt will be overwritten by
	 * the second pkt, causing unexpected conditions.
	 */
	if (unlikely(skb_cloned(prSkb))) {
		prOrgSkb = prSkb;
		prSkbNew = skb_copy_expand(prOrgSkb,
				u4TxHeadRoomSize, 0, GFP_ATOMIC);
		if (!prSkbNew) {
			dev_kfree_skb(prOrgSkb);
			DBGLOG(INIT, ERROR, "cloned_skb copy fail\n");
			return WLAN_STATUS_NOT_ACCEPTED;
		}
		kmemleak_not_leak(prSkbNew); /* Omit memleak check */
		dev_kfree_skb(prOrgSkb);
		prSkb = prSkbNew;
	}
#endif

	prQueueEntry = (struct QUE_ENTRY *) GLUE_GET_PKT_QUEUE_ENTRY(prSkb);
	prTxQueue = &prGlueInfo->rTxQueue;

	GLUE_SET_PKT_BSS_IDX(prSkb, ucBssIndex);

	/* Parsing frame info */
	if (!wlanProcessTxFrame(prGlueInfo->prAdapter,
				(void *) prSkb)) {
		/* Cannot extract packet */
		DBGLOG(INIT, INFO,
		       "Cannot extract content, skip this frame\n");
		dev_kfree_skb(prSkb);
		return WLAN_STATUS_INVALID_PACKET;
	}

#if (CFG_SUPPORT_STATISTICS == 1)
	STATS_TX_TIME_ARRIVE(prSkb);
#endif

	/* Tx profiling */
	wlanTxProfilingTagPacket(prGlueInfo->prAdapter,
				 (void *) prSkb, TX_PROF_TAG_OS_TO_DRV);

	/* Handle normal data frame */
	u2QueueIdx = skb_get_queue_mapping(prSkb);
	u4SkbLen = prSkb->len;

	if (u2QueueIdx >= CFG_MAX_TXQ_NUM) {
		DBGLOG(INIT, INFO,
		       "Incorrect queue index, skip this frame\n");
		dev_kfree_skb(prSkb);
		return WLAN_STATUS_INVALID_PACKET;
	}

#if CFG_SUPPORT_TPENHANCE_MODE
	if (!kalTpeProcess(prGlueInfo,
			prSkb,
			prDev)) {
		return WLAN_STATUS_SUCCESS;
	}
#endif /* CFG_SUPPORT_TPENHANCE_MODE */

	TX_INC_CNT(&prAdapter->rTxCtrl, TX_IN_COUNT);

	if (!HAL_IS_TX_DIRECT(prGlueInfo->prAdapter)) {
		GLUE_SPIN_LOCK_DECLARATION();

		GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_QUE);
		QUEUE_INSERT_TAIL(prTxQueue, prQueueEntry);
		kalTraceInt(prTxQueue->u4NumElem, "TxQueue");
		GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_QUE);
	}

	GLUE_INC_REF_CNT(prGlueInfo->i4TxPendingFrameNum);
	GLUE_INC_REF_CNT(
		prGlueInfo->ai4TxPendingFrameNumPerQueue[ucBssIndex]
		[u2QueueIdx]);

	u4StopTh = prGlueInfo->u4TxStopTh[ucBssIndex];
	if (GLUE_GET_REF_CNT(prGlueInfo->ai4TxPendingFrameNumPerQueue
	    [ucBssIndex][u2QueueIdx]) >= u4StopTh) {
		netif_stop_subqueue(prDev, u2QueueIdx);

		DBGLOG_LIMITED(TX, INFO,
		       "Stop subqueue for BSS[%u] QIDX[%u] PKT_LEN[%u] TOT_CNT[%d] PER-Q_CNT[%d]\n",
		       ucBssIndex, u2QueueIdx, u4SkbLen,
		       GLUE_GET_REF_CNT(prGlueInfo->i4TxPendingFrameNum),
		       GLUE_GET_REF_CNT(
			       prGlueInfo->ai4TxPendingFrameNumPerQueue
			       [ucBssIndex][u2QueueIdx]));
	}

	/* Update NetDev statisitcs */
	prDev->stats.tx_bytes += u4SkbLen;
	prDev->stats.tx_packets++;
#if CFG_SUPPORT_PERF_IND
	/* update Performance Indicator statistics*/
	prGlueInfo->PerfIndCache.u4CurTxBytes[ucBssIndex] += u4SkbLen;
#endif
#if CFG_UPDATE_PACING_SHIFT_SUPPORT
	DBGLOG(TX, LOUD, "prAdapter->rWifiVar.u4PacingShift = %d\n",
		prAdapter->rWifiVar.u4PacingShift);

	if (prAdapter->rWifiVar.u4PacingShift != 0)
		sk_pacing_shift_update(prSkb->sk,
				       prAdapter->rWifiVar.u4PacingShift);
#endif

	DBGLOG(TX, LOUD,
	       "Enqueue frame for BSS[%u] QIDX[%u] PKT_LEN[%u] TOT_CNT[%d] PER-Q_CNT[%d]\n",
	       ucBssIndex, u2QueueIdx, u4SkbLen,
	       GLUE_GET_REF_CNT(prGlueInfo->i4TxPendingFrameNum),
	       GLUE_GET_REF_CNT(
		       prGlueInfo->ai4TxPendingFrameNumPerQueue[ucBssIndex]
		       [u2QueueIdx]));

#if CFG_SUPPORT_CPU_STAT
	CPU_STAT_INC_CNT(prGlueInfo, CPU_TX_IN);
#endif /* CFG_SUPPORT_CPU_STAT */

	if (HAL_IS_TX_DIRECT(prGlueInfo->prAdapter)) {
#if defined(_HIF_PCIE) && (HIF_TX_PREALLOC_DATA_BUFFER == 0)
		/* To reduce L3 buffer usage, release original owner ASAP */
		skb_orphan(prSkb);
#elif CFG_SUPPORT_TX_SKB_ORPHAN
		if (kalIsTxHighTput(prGlueInfo->prAdapter))
			skb_orphan(prSkb);
#endif
#if CFG_SUPPORT_PER_CPU_TX
		return kalPerCpuTxXmit(prSkb, prGlueInfo);
#elif CFG_SUPPORT_TX_WORK
		return kalTxWorkSchedule(prSkb, prGlueInfo);
#else /* CFG_SUPPORT_TX_WORK */
		return kalTxDirectStartXmit(prSkb, prGlueInfo);
#endif /* CFG_SUPPORT_TX_WORK */
	}

	kalSetEvent(prGlueInfo);

	return WLAN_STATUS_SUCCESS;
}				/* end of kalHardStartXmit() */

uint32_t kalResetStats(struct net_device *prDev)
{
	DBGLOG(QM, LOUD, "Reset NetDev[0x%p] statistics\n", prDev);

	if (prDev)
		kalMemZero(kalGetStats(prDev),
			   sizeof(struct net_device_stats));

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief A method of struct net_device, to get the network interface
 *        statistical information.
 *
 * Whenever an application needs to get statistics for the interface, this
 * method is called. This happens, for example, when ifconfig or netstat -i is
 * run.
 *
 * \param[in] prDev      Pointer to struct net_device.
 *
 * \return net_device_stats buffer pointer.
 */
/*----------------------------------------------------------------------------*/
void *kalGetStats(struct net_device *prDev)
{
	return (void *) &prDev->stats;
}				/* end of wlanGetStats() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief Notify OS with SendComplete event of the specific packet. Linux should
 *        free packets here.
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 * \param[in] pvPacket       Pointer of Packet Handle
 * \param[in] status         Status Code for OS upper layer
 *
 * \return -
 */
/*----------------------------------------------------------------------------*/
void kalSendComplete(struct GLUE_INFO *prGlueInfo, void *pvPacket,
	uint32_t u4Status)
{
	struct net_device *prDev = NULL;
	struct sk_buff *prSkb = NULL;
	uint16_t u2QueueIdx = 0;
	uint8_t ucBssIndex = 0;
	u_int8_t fgIsValidDevice = TRUE;
	struct ADAPTER *prAdapter = NULL;
#if CFG_ENABLE_WIFI_DIRECT
	struct BSS_INFO *prBssInfo = NULL;

	GLUE_SPIN_LOCK_DECLARATION();
#endif

	prAdapter = prGlueInfo->prAdapter;
	if (!pvPacket)
		return;

	prSkb = (struct sk_buff *)pvPacket;
	u2QueueIdx = skb_get_queue_mapping(prSkb);
	if (unlikely(u2QueueIdx >= CFG_MAX_TXQ_NUM)) {
		DBGLOG(TX, ERROR, "Invalid QIDX[%u]\n", u2QueueIdx);
		goto end;
	}

	ucBssIndex = GLUE_GET_PKT_BSS_IDX(pvPacket);

	if (unlikely(ucBssIndex >= MAX_BSSID_NUM)) {
		DBGLOG(TX, ERROR, "Invalid BSS[%u]\n", ucBssIndex);
		goto end;
	}

	GLUE_DEC_REF_CNT(prGlueInfo->i4TxPendingFrameNum);
	GLUE_DEC_REF_CNT(
		prGlueInfo->ai4TxPendingFrameNumPerQueue[ucBssIndex]
		[u2QueueIdx]);

	DBGLOG(TX, LOUD,
	       "Release frame for BSS[%u] QIDX[%u] PKT_LEN[%u] TOT_CNT[%d] PER-Q_CNT[%d]\n",
	       ucBssIndex, u2QueueIdx, prSkb->len,
	       GLUE_GET_REF_CNT(prGlueInfo->i4TxPendingFrameNum),
	       GLUE_GET_REF_CNT(
		       prGlueInfo->ai4TxPendingFrameNumPerQueue[ucBssIndex]
		       [u2QueueIdx]));

	prDev = wlanGetNetInterfaceByBssIdx(prGlueInfo, ucBssIndex);
	if (!prDev)
		goto end;

	if (IS_BSS_INDEX_AIS(prAdapter, ucBssIndex)) {
		struct AIS_FSM_INFO *prAisFsmInfo =
			aisGetAisFsmInfo(prAdapter, ucBssIndex);

		if (prAisFsmInfo &&
			!wlanGetAisNetDev(prGlueInfo, prAisFsmInfo->ucAisIndex))
			fgIsValidDevice = FALSE;
	}

#if CFG_ENABLE_WIFI_DIRECT
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

	if (prBssInfo && prBssInfo->eNetworkType == NETWORK_TYPE_P2P) {
		/* Make sure p2p netdevice is in registered state */
		GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
		if (prAdapter->rP2PNetRegState != ENUM_NET_REG_STATE_REGISTERED)
			fgIsValidDevice = FALSE;
		GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
	}
#endif

	if (fgIsValidDevice) {
		uint32_t u4StartTh = prGlueInfo->u4TxStartTh[ucBssIndex];

		if (netif_subqueue_stopped(prDev, prSkb) &&
		    prGlueInfo->ai4TxPendingFrameNumPerQueue[ucBssIndex]
		    [u2QueueIdx] <= u4StartTh) {
			netif_wake_subqueue(prDev, u2QueueIdx);
			DBGLOG_LIMITED(TX, INFO,
				"WakeUp Queue BSS[%u] QIDX[%u] PKT_LEN[%u] TOT_CNT[%d] PER-Q_CNT[%d]\n",
				ucBssIndex, u2QueueIdx, prSkb->len,
				GLUE_GET_REF_CNT(
					prGlueInfo->i4TxPendingFrameNum),
				GLUE_GET_REF_CNT(
					prGlueInfo->
					ai4TxPendingFrameNumPerQueue
					[ucBssIndex][u2QueueIdx]));
		}
	}

	DBGLOG(TX, LOUD, "----- pending frame %d -----\n",
	       prGlueInfo->i4TxPendingFrameNum);

end:
	if (u4Status == WLAN_STATUS_SUCCESS)
		dev_consume_skb_any((struct sk_buff *)pvPacket);
	else
		dev_kfree_skb_any((struct sk_buff *)pvPacket);
}

#if CFG_SUPPORT_EXT_CONFIG
/*----------------------------------------------------------------------------*/
/*!
 * \brief Read external configuration, ex. NVRAM or file
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
uint32_t kalReadExtCfg(struct GLUE_INFO *prGlueInfo)
{
	ASSERT(prGlueInfo);

	/* External data is given from user space by ioctl or /proc, not read by
	 *  driver.
	 */
	if (prGlueInfo->u4ExtCfgLength != 0)
		DBGLOG(INIT, TRACE,
		       "Read external configuration data -- OK\n");
	else
		DBGLOG(INIT, TRACE,
		       "Read external configuration data -- fail\n");

	return prGlueInfo->u4ExtCfgLength;
}
#endif

u_int8_t
kalIPv4FrameClassifier(struct GLUE_INFO *prGlueInfo,
		       void *prPacket, uint8_t *pucIpHdr,
		       struct TX_PACKET_INFO *prTxPktInfo)
{
	uint8_t ucIpVersion, ucIcmpType;
	uint8_t ucIpProto;
	uint8_t ucSeqNo;
	uint8_t *pucUdpHdr, *pucIcmp;
	uint16_t u2SrcPort;
	uint16_t u2DstPort;
	struct DHCP_PROTOCOL *prDhcp;
	uint32_t u4DhcpMagicCode;
	uint16_t u2IpId;
#if CFG_SUPPORT_WIFI_SYSDVT
#if (CFG_TCP_IP_CHKSUM_OFFLOAD)
	struct ADAPTER *prAdapter = NULL;
#endif
#endif /* Automation */

	/* IPv4 version check */
	ucIpVersion = (pucIpHdr[0] & IP_VERSION_MASK) >> IP_VERSION_OFFSET;
	if (ucIpVersion != IP_VERSION_4) {
		DBGLOG(TX, WARN, "Invalid IPv4 packet version: %u\n",
		       ucIpVersion);
		return FALSE;
	}

	ucIpProto = pucIpHdr[IPV4_HDR_IP_PROTOCOL_OFFSET];
	u2IpId = (pucIpHdr[IPV4_ADDR_LEN] << 8) | pucIpHdr[IPV4_ADDR_LEN + 1];
	GLUE_SET_PKT_IP_ID(prPacket, u2IpId);

#if CFG_SUPPORT_WIFI_SYSDVT
#if (CFG_TCP_IP_CHKSUM_OFFLOAD)
	prAdapter = prGlueInfo->prAdapter;

	/* set IP CHECKSUM to 0xff for verify CSO function */
	if (prAdapter->u4CSUMFlags & CSUM_OFFLOAD_EN_TX_IP
		&& CSO_TX_IPV4_ENABLED(prAdapter)) {
		pucIpHdr[IPV4_HDR_IP_CSUM_OFFSET] = 0xff;
		pucIpHdr[IPV4_HDR_IP_CSUM_OFFSET+1] = 0xff;
	}
#endif /* CFG_TCP_IP_CHKSUM_OFFLOAD */
#endif /* Automation */

#if (CFG_IP_FRAG_DISABLE_HW_CHECKSUM == 1)
	if ((pucIpHdr[IPV4_HDR_IP_FRAG_OFFSET] & IPV4_HDR_IP_FLAGS_MF_MASK) ||
	    (pucIpHdr[IPV4_HDR_IP_FRAG_OFFSET] & IPV4_HDR_IP_FRAG_PART1_MASK) ||
	    (pucIpHdr[IPV4_HDR_IP_FRAG_OFFSET+1])) {
		prTxPktInfo->u2Flag |= BIT(ENUM_PKT_IP_FRAG);
	}
#endif

	if (ucIpProto == IP_PRO_UDP) {
		pucUdpHdr = &pucIpHdr[IPV4_HDR_LEN];

#if CFG_SUPPORT_WIFI_SYSDVT
#if (CFG_TCP_IP_CHKSUM_OFFLOAD)
		/* set UDP CHECKSUM to 0xff for verify CSO function */
		if (prAdapter->u4CSUMFlags & CSUM_OFFLOAD_EN_TX_UDP
			&& CSO_TX_UDP_ENABLED(prAdapter)) {
			pucUdpHdr[UDP_HDR_UDP_CSUM_OFFSET] = 0xff;
			pucUdpHdr[UDP_HDR_UDP_CSUM_OFFSET+1] = 0xff;
		}
#endif /* CFG_TCP_IP_CHKSUM_OFFLOAD */
#endif /* Automation */

		/* Get UDP SRC port */
		WLAN_GET_FIELD_BE16(&pucUdpHdr[UDP_HDR_SRC_PORT_OFFSET],
					&u2SrcPort);

		/* Get UDP DST port */
		WLAN_GET_FIELD_BE16(&pucUdpHdr[UDP_HDR_DST_PORT_OFFSET],
				    &u2DstPort);

		/* DHCP protocol */
		if (u2DstPort == IP_PORT_DHCP_SERVER ||
		    u2DstPort == IP_PORT_DHCP_CLIENT) {
			prDhcp = (struct DHCP_PROTOCOL *)
							&pucUdpHdr[UDP_HDR_LEN];
			u4DhcpMagicCode = NTOHL(prDhcp->u4MagicCookie);
			if (u4DhcpMagicCode == DHCP_MAGIC_NUMBER) {
				ucSeqNo = nicIncreaseTxSeqNum(
							prGlueInfo->prAdapter);
				GLUE_SET_PKT_SEQ_NO(prPacket, ucSeqNo);
				prTxPktInfo->u2Flag |= BIT(ENUM_PKT_DHCP);
			}
		} else if (u2DstPort == UDP_PORT_DNS ||
			   u2SrcPort == UDP_PORT_DNS) {
			ucSeqNo = nicIncreaseTxSeqNum(prGlueInfo->prAdapter);
			GLUE_SET_PKT_SEQ_NO(prPacket, ucSeqNo);
			prTxPktInfo->u2Flag |= BIT(ENUM_PKT_DNS);
		}
#if CFG_WOW_SUPPORT
#if (CFG_SUPPORT_MDNS_OFFLOAD && CFG_SUPPORT_MDNS_OFFLOAD_GVA)
		else if (u2DstPort == UDP_PORT_MDNS) {
			uint8_t *pucMdnsHdr = NULL;

			if (!prGlueInfo->prAdapter->mdns_offload_enable)
				return TRUE;

			pucMdnsHdr = &pucIpHdr[IPV4_HDR_LEN + UDP_HDR_LEN];
			kalProcessMdnsRespPkt(prGlueInfo, pucMdnsHdr);
		}
#endif
#endif
	} else if (ucIpProto == IP_PRO_ICMP) {
		pucIcmp = &pucIpHdr[20];
		ucIcmpType = pucIcmp[0];
		if (ucIcmpType == 3) /* don't log network unreachable packet */
			return FALSE;
		ucSeqNo = nicIncreaseTxSeqNum(prGlueInfo->prAdapter);
		GLUE_SET_PKT_SEQ_NO(prPacket, ucSeqNo);
		prTxPktInfo->u2Flag |= BIT(ENUM_PKT_ICMP);
	}
	else if (ucIpProto == IP_PRO_TCP) {
		uint8_t *pucTcpHdr;

		pucTcpHdr = &pucIpHdr[IPV4_HDR_LEN];
#if CFG_SUPPORT_WIFI_SYSDVT
#if (CFG_TCP_IP_CHKSUM_OFFLOAD)
		/* set TCP CHECKSUM to 0xff for verify CSO function */
		if (prAdapter->u4CSUMFlags & CSUM_OFFLOAD_EN_TX_TCP
			&& CSO_TX_TCP_ENABLED(prAdapter)) {
			pucTcpHdr[TCP_HDR_TCP_CSUM_OFFSET] = 0xff;
			pucTcpHdr[TCP_HDR_TCP_CSUM_OFFSET+1] = 0xff;
		}
#endif /* CFG_TCP_IP_CHKSUM_OFFLOAD */
#endif /* Automation */

#if CFG_SUPPORT_TPENHANCE_MODE
		if (pucTcpHdr[TCP_HDR_FLAG_OFFSET] & TCP_HDR_FLAG_ACK_BIT)
			GLUE_SET_PKT_FLAG(prPacket, ENUM_PKT_TCP_ACK);
#endif /* CFG_SUPPORT_TPENHANCE_MODE */
	}

	return TRUE;
}

u_int8_t
kalIPv6FrameClassifier(struct GLUE_INFO *prGlueInfo,
		       void *prPacket, uint8_t *pucIpv6Hdr,
		       struct TX_PACKET_INFO *prTxPktInfo)
{
	uint8_t ucIpv6Proto;
	uint8_t *pucL3Hdr;
	struct ADAPTER *prAdapter;
	uint8_t ucSeqNo;

	prAdapter = prGlueInfo->prAdapter;
	ucIpv6Proto = pucIpv6Hdr[IPV6_HDR_IP_PROTOCOL_OFFSET];

	if (ucIpv6Proto == IP_PRO_UDP) {
		pucL3Hdr = &pucIpv6Hdr[IPV6_HDR_LEN];
#if CFG_SUPPORT_WIFI_SYSDVT
#if (CFG_TCP_IP_CHKSUM_OFFLOAD)
		/* set UDP CHECKSUM to 0xff for verify CSO function */
		if ((prAdapter->u4CSUMFlags & CSUM_OFFLOAD_EN_TX_UDP)
			&& CSO_TX_UDP_ENABLED(prAdapter)) {
			pucL3Hdr[UDP_HDR_UDP_CSUM_OFFSET] = 0xff;
			pucL3Hdr[UDP_HDR_UDP_CSUM_OFFSET+1] = 0xff;
		}
#endif /* CFG_TCP_IP_CHKSUM_OFFLOAD */
#endif /* Automation */
	} else if (ucIpv6Proto == IP_PRO_TCP) {
		pucL3Hdr = &pucIpv6Hdr[IPV6_HDR_LEN];
#if CFG_SUPPORT_WIFI_SYSDVT
#if (CFG_TCP_IP_CHKSUM_OFFLOAD)
		/* set TCP CHECKSUM to 0xff for verify CSO function */
		if ((prAdapter->u4CSUMFlags & CSUM_OFFLOAD_EN_TX_TCP)
			&& CSO_TX_TCP_ENABLED(prAdapter)) {
			pucL3Hdr[TCP_HDR_TCP_CSUM_OFFSET] = 0xff;
			pucL3Hdr[TCP_HDR_TCP_CSUM_OFFSET+1] = 0xff;
		}
#endif /* CFG_TCP_IP_CHKSUM_OFFLOAD */
#endif /* Automation */
	} else if (ucIpv6Proto == IPV6_PROTOCOL_ICMPV6) { /* ICMPV6 */
		ucSeqNo = nicIncreaseTxSeqNum(prAdapter);
		GLUE_SET_PKT_SEQ_NO(prPacket, ucSeqNo);
		prTxPktInfo->u2Flag |= BIT(ENUM_PKT_ICMPV6);
	}

	return TRUE;
}

u_int8_t
kalArpFrameClassifier(struct GLUE_INFO *prGlueInfo,
		      void *prPacket, uint8_t *pucIpHdr,
		      struct TX_PACKET_INFO *prTxPktInfo)
{
	uint8_t ucSeqNo;
	ucSeqNo = nicIncreaseTxSeqNum(prGlueInfo->prAdapter);
	GLUE_SET_PKT_SEQ_NO(prPacket, ucSeqNo);
	prTxPktInfo->u2Flag |= BIT(ENUM_PKT_ARP);
	return TRUE;
}

u_int8_t
kalTdlsFrameClassifier(struct GLUE_INFO *prGlueInfo,
		       void *prPacket, uint8_t *pucIpHdr,
		       struct TX_PACKET_INFO *prTxPktInfo)
{
	uint8_t ucSeqNo;

	ucSeqNo = nicIncreaseTxSeqNum(prGlueInfo->prAdapter);
	GLUE_SET_PKT_SEQ_NO(prPacket, ucSeqNo);
	prTxPktInfo->u2Flag |= BIT(ENUM_PKT_TDLS);

	return TRUE;
}

u_int8_t
kalSecurityFrameClassifier(struct GLUE_INFO *prGlueInfo,
			   void *prPacket, uint8_t *pucIpHdr,
			   uint16_t u2EthType, uint8_t *aucLookAheadBuf,
			   struct TX_PACKET_INFO *prTxPktInfo)
{
	uint8_t *pucEapol;
	uint8_t ucEapolType;
	uint8_t ucSeqNo;
	uint8_t	ucEAPoLKey = 0;
	uint8_t	ucEapOffset = ETHER_HEADER_LEN;
	uint16_t u2KeyInfo = 0;

	pucEapol = pucIpHdr;

	if (u2EthType == ETH_P_1X) {
		struct STA_RECORD *prStaRec;
		ucEapolType = pucEapol[1];

		prStaRec = cnmGetStaRecByAddress(prGlueInfo->prAdapter,
				GLUE_GET_PKT_BSS_IDX(prPacket),
				aucLookAheadBuf);

		if (prStaRec && prStaRec->fgTransmitKeyExist &&
				prStaRec->fgIsEapEncrypt) {
			/* Encrypt EAP frames if AIS connected and with a key */
			DBGLOG(TX, INFO, "Encrypt EAP packets\n");
		} else {
			/* Leave EAP to check */
			ucEAPoLKey = aucLookAheadBuf[1 + ucEapOffset];
			if (ucEAPoLKey != ETH_EAPOL_KEY)
				prTxPktInfo->u2Flag |=
					BIT(ENUM_PKT_NON_PROTECTED_1X);
			else {
				WLAN_GET_FIELD_BE16(
					&aucLookAheadBuf[5 + ucEapOffset],
					&u2KeyInfo);
				/* BIT3 is pairwise key bit */
				DBGLOG(TX, INFO, "u2KeyInfo=%d\n", u2KeyInfo);
				if (u2KeyInfo & BIT(3))
					prTxPktInfo->u2Flag |=
						BIT(ENUM_PKT_NON_PROTECTED_1X);
			}
		}

		ucSeqNo = nicIncreaseTxSeqNum(prGlueInfo->prAdapter);
		GLUE_SET_PKT_SEQ_NO(prPacket, ucSeqNo);
#if CFG_SUPPORT_WAPI
	} else if (u2EthType == ETH_WPI_1X) {
		ucSeqNo = nicIncreaseTxSeqNum(prGlueInfo->prAdapter);
		GLUE_SET_PKT_SEQ_NO(prPacket, ucSeqNo);
		prTxPktInfo->u2Flag |= BIT(ENUM_PKT_NON_PROTECTED_1X);
#endif
	}
	prTxPktInfo->u2Flag |= BIT(ENUM_PKT_1X);
	return TRUE;
}

/**
 * kalIPv6ChecksumHelp() - Calculate checksum in driver for special cases
 * @prSkb: pointer to skbuff
 * @pucIpv6Hdr: pointer to head of IPv6 packet
 * @u4Ipv6TotalLen: length of IPv6 packet, including header
 */
static
void kalIPv6ChecksumHelp(struct sk_buff *prSkb, const uint8_t *pucIpv6Hdr,
				const uint32_t u4Ipv6TotalLen)
{
	uint8_t ucIpv6Proto;

	ucIpv6Proto = pucIpv6Hdr[IPV6_HDR_IP_PROTOCOL_OFFSET];
	if (ucIpv6Proto == IP_PRO_UDP && u4Ipv6TotalLen <= IPV6_HDR_LEN + 16) {
		if (prSkb->ip_summed == CHECKSUM_PARTIAL)
			skb_checksum_help(prSkb);
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This inline function is to extract some packet information, including
 *        user priority, packet length, destination address, 802.1x and BT over
 *        Wi-Fi or not.
 *
 * @param prGlueInfo         Pointer to the glue structure
 * @param prPacket           Packet descriptor
 * @param prTxPktInfo        Extracted packet info
 *
 * @retval TRUE      Success to extract information
 * @retval FALSE     Fail to extract correct information
 */
/*----------------------------------------------------------------------------*/
u_int8_t
kalQoSFrameClassifierAndPacketInfo(struct GLUE_INFO *prGlueInfo,
		void *prPacket, struct TX_PACKET_INFO *prTxPktInfo)
{
	uint32_t u4PacketLen;
	uint16_t u2EtherTypeLen;
	struct sk_buff *prSkb = prPacket;
	uint8_t *aucLookAheadBuf = NULL;
	uint8_t ucEthTypeLenOffset = ETHER_HEADER_LEN - ETHER_TYPE_LEN;
	void *pucNextProtocol = NULL;
	uint32_t u4MinTxLen;

	u4PacketLen = prSkb->len;

	u4MinTxLen = prGlueInfo->prAdapter->chip_info->u4MinTxLen;
	if (u4PacketLen <= ETHER_HEADER_LEN + u4MinTxLen) {
		DBGLOG(INIT, WARN, "Invalid Ether packet length: %u\n",
		       u4PacketLen);
		return FALSE;
	}

	aucLookAheadBuf = prSkb->data;

	/* Reset Packet Info */
	kalMemZero(prTxPktInfo, sizeof(struct TX_PACKET_INFO));

	/* 4 <0> Obtain Ether Type/Len */
	WLAN_GET_FIELD_BE16(&aucLookAheadBuf[ucEthTypeLenOffset],
			    &u2EtherTypeLen);

	/*
	 * if EtherTypeLen >= ETHER_TYPE_MIN, it is a Ethernet II pkt
	 * and this field is EtherType.
	 * Otherwise, it is a 802.3 pkt and this field is Len instead.
	 */
#if CFG_WIFI_TX_ETH_CHK_EMPTY_PAYLOAD
	if (unlikely(u4PacketLen == ETHER_HEADER_LEN &&
		     u2EtherTypeLen >= ETHER_TYPE_MIN)) {
		DBGLOG(TX, WARN,
		       "Drop 802.3 header only but no payload packet\n");
		return FALSE;
	}
#endif

	/* 4 <1> Skip 802.1Q header (VLAN Tagging) */
	if (u2EtherTypeLen == ETH_P_VLAN) {
		prTxPktInfo->u2Flag |= BIT(ENUM_PKT_VLAN_EXIST);
		ucEthTypeLenOffset += ETH_802_1Q_HEADER_LEN;
		WLAN_GET_FIELD_BE16(&aucLookAheadBuf[ucEthTypeLenOffset],
				    &u2EtherTypeLen);
	}
	/* 4 <2> Obtain next protocol pointer */
	pucNextProtocol = &aucLookAheadBuf[ucEthTypeLenOffset + ETHER_TYPE_LEN];

	/* 4 <3> Handle ethernet format */
	switch (u2EtherTypeLen) {
	case ETH_P_IPV4:
		/* IPv4 header length check */
		if (u4PacketLen < (ucEthTypeLenOffset + ETHER_TYPE_LEN +
				   IPV4_HDR_LEN)) {
			DBGLOG(INIT, WARN, "Invalid IPv4 packet length: %u\n",
			       u4PacketLen);
			break;
		}
		kalIPv4FrameClassifier(prGlueInfo, prPacket,
				       pucNextProtocol, prTxPktInfo);
		break;

	case ETH_P_ARP:
		kalArpFrameClassifier(prGlueInfo, prPacket, pucNextProtocol,
				      prTxPktInfo);
		break;

	case ETH_P_1X:
	case ETH_P_PRE_1X:
#if CFG_SUPPORT_WAPI
	case ETH_WPI_1X:
#endif
		kalSecurityFrameClassifier(prGlueInfo, prPacket,
			pucNextProtocol, u2EtherTypeLen, aucLookAheadBuf,
			prTxPktInfo);
		break;

	case ETH_PRO_TDLS:
		kalTdlsFrameClassifier(prGlueInfo, prPacket,
				       pucNextProtocol, prTxPktInfo);
		break;

	case ETH_P_IPV6:
		kalIPv6FrameClassifier(prGlueInfo, prPacket,
				       pucNextProtocol, prTxPktInfo);
		kalIPv6ChecksumHelp(prSkb, pucNextProtocol,
			u4PacketLen - (ucEthTypeLenOffset + ETHER_TYPE_LEN));
		break;

	default:
		/* 4 <4> Handle 802.3 format if LEN <= 1500 */
		if (u2EtherTypeLen <= ETH_802_3_MAX_LEN)
			prTxPktInfo->u2Flag |= BIT(ENUM_PKT_802_3);
		break;
	}

	StatsTxPktInfoDisplay(prSkb);

	/* 4 <4.1> Check for PAL (BT over Wi-Fi) */
	/* Move to kalBowFrameClassifier */

	/* 4 <5> Return the value of Priority Parameter. */
	/* prSkb->priority is assigned by Linux wireless utility
	 * function(cfg80211_classify8021d)
	 */
	/* at net_dev selection callback (ndo_select_queue) */
	prTxPktInfo->ucPriorityParam = prSkb->priority;

#if CFG_CHANGE_PRIORITY_BY_SKB_MARK_FIELD
	/* Raise priority for special packet if skb mark filed
	 * marked with pre-defined value.
	 */
	if (prSkb->mark & BIT(NIC_TX_SKB_PRIORITY_MARK_BIT)) {
		prTxPktInfo->ucPriorityParam = NIC_TX_PRIORITY_DATA_TID;
		DBGLOG_LIMITED(INIT, TRACE,
				"skb mark field=[%x]", prSkb->mark);
	}
#endif

	/* 4 <6> Retrieve Packet Information - DA */
	/* Packet Length/ Destination Address */
	prTxPktInfo->u4PacketLen = u4PacketLen;

	kalMemCopy(prTxPktInfo->aucEthDestAddr, aucLookAheadBuf,
		   PARAM_MAC_ADDR_LEN);

	return TRUE;
}				/* end of kalQoSFrameClassifier() */

u_int8_t kalGetEthDestAddr(struct GLUE_INFO *prGlueInfo,
			   void *prPacket, uint8_t *pucEthDestAddr)
{
	struct sk_buff *prSkb = (struct sk_buff *)prPacket;
	uint8_t *aucLookAheadBuf = NULL;

	/* Sanity Check */
	if (!prPacket || !prGlueInfo)
		return FALSE;

	aucLookAheadBuf = prSkb->data;

	kalMemCopy(pucEthDestAddr, aucLookAheadBuf,
		   PARAM_MAC_ADDR_LEN);

	return TRUE;
}

/**
 * isOidWaitingComplete() - Check whether an OID waiting for completion
 *
 * !completion_done() represents for two cases:
 *   1. all wait() are complete()ed, i.e., no pending waiters
 *   2. there is a pending waiter waiting for complete() before timeout
 * WTF?
 *
 * completion_done() returns true returns true if x->done != 0.
 * It indicates there is one or more complete() were not consumed by wait().
 * Otherwise, if it returns false, x->done == 0, stands for there are no
 * posted completions that were not yet consumed by waiters.
 * The Linux documents add a comment said it implying that there are waiters,
 * however, it only points to case 2.
 *
 * The driver uses fgOidWaiting, which were set ONE before calling ioctl,
 * and to be set ZERO after calling complete(), to distinguish the two cases.
 * The initial state of the flag with value 0 is equivalent to no waiters.
 *
 * We check completion_done() again to avoid setting x->done more than 1.
 *
 * Return: TRUE: there is an OID waiter
 *	   FALSE: there is no OID waiter
 */
static u_int8_t isOidWaitingComplete(struct GLUE_INFO *prGlueInfo)
{
	return prGlueInfo->fgOidWaiting &&
		!completion_done(&prGlueInfo->rPendComp);
}

void kalOidComplete(struct GLUE_INFO *prGlueInfo,
	       struct CMD_INFO *prCmdInfo, uint32_t u4SetQueryInfoLen,
	       uint32_t rOidStatus)
{
	struct GL_IO_REQ *prIoReq = NULL;
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;

	ASSERT(prGlueInfo);

	prIoReq = &prGlueInfo->OidEntry;
	DBGLOG(NIC, TRACE,
		"Cmd=%p pfnOidHandler=%ps InformationBuffer=%p QryInfoLen=%p",
		prCmdInfo, prIoReq->pfnOidHandler,
		prCmdInfo ? prCmdInfo->pvInformationBuffer : NULL,
		prIoReq->pu4QryInfoLen);

	/* remove timeout check timer */
	wlanoidClearTimeoutCheck(prGlueInfo->prAdapter);

	/* complete ONLY if there are waiters */
	if (isOidWaitingComplete(prGlueInfo)) {
		/* only update when there are waiters */
		prGlueInfo->rPendStatus = rOidStatus;
		*prIoReq->pu4QryInfoLen = u4SetQueryInfoLen;

		kalUpdateCompHdlrRec(prGlueInfo->prAdapter, NULL, prCmdInfo);

		if (prCmdInfo)
			DBGLOG(TX, TRACE, "rPendComp=%p, cmd=0x%02X, seq=%u",
				&prGlueInfo->rPendComp,
				prCmdInfo->ucCID, prCmdInfo->ucCmdSeqNum);

		prGlueInfo->fgOidWaiting = FALSE;
		complete(&prGlueInfo->rPendComp);
	} else {
		uint32_t wIdx, cIdx;
		int i;

		DBGLOG(INIT, WARN, "SKIP multiple OID complete!\n");
		/* WARN_ON(TRUE); */

		if (prAdapter != NULL) {
			for (i = OID_HDLR_REC_NUM - 1,
				wIdx = prAdapter->u4WaitRecIdx,
				cIdx = prAdapter->u4CompRecIdx;
				i >= 0; i--) {
				DBGLOG(OID, WARN,
					"%dth last wait OID hdlr: %s\n",
					OID_HDLR_REC_NUM - i,
					prAdapter->arPrevWaitHdlrRec[
					(wIdx + i) % OID_HDLR_REC_NUM].aucName);
				DBGLOG(OID, WARN,
					"%dth last comp OID hdlr: %s\n",
					OID_HDLR_REC_NUM - i,
					prAdapter->arPrevCompHdlrRec[
					(cIdx + i) % OID_HDLR_REC_NUM].aucName);
			}
		}
	}

	if (rOidStatus == WLAN_STATUS_SUCCESS)
		DBGLOG(INIT, TRACE, "Complete OID, status:success\n");
	else
		DBGLOG(INIT, WARN, "Complete OID, status:0x%08x\n", rOidStatus);

	/* else let it timeout on kalIoctl entry */
}

void kalOidClearance(struct GLUE_INFO *prGlueInfo)
{

}

void kalGetLocalTime(unsigned long long *sec, unsigned long *nsec)
{
	if (sec != NULL && nsec != NULL) {
		*sec = local_clock();
		*nsec = do_div(*sec, 1000000000)/1000;
	} else
		DBGLOG(INIT, ERROR,
			"The input parameters error when get local time\n");
}

/*
 * kalThreadSchedRetrieve
 * Retrieve thread's current scheduling statistics and
 * stored in output "sched".
 * Return value:
 *	 0 : Schedstats successfully retrieved
 *	-1 : Kernel's schedstats feature not enabled
 *	-2 : pThread not yet initialized or sched is a NULL pointer
 */
static int32_t kalThreadSchedRetrieve(struct task_struct *pThread,
					struct KAL_THREAD_SCHEDSTATS *pSched)
{
#ifdef CONFIG_SCHEDSTATS
	struct sched_entity se;
	struct sched_statistics *stats;
	unsigned long long sec;
	unsigned long usec;

	if (!pSched)
		return -2;

	/* always clear sched to simplify error handling at caller side */
	memset(pSched, 0, sizeof(struct KAL_THREAD_SCHEDSTATS));

	if (!pThread || kalIsResetting())
		return -2;

	memcpy(&se, &pThread->se, sizeof(struct sched_entity));
	kalGetLocalTime(&sec, &usec);

#if (KERNEL_VERSION(5, 16, 0) <= LINUX_VERSION_CODE) || \
	((CFG_KERNEL_AN14_515 == 1) && \
	KERNEL_VERSION(5, 15, 110) <= LINUX_VERSION_CODE)
	stats = &pThread->stats;
#else
	stats = &pThread->se.statistics;
#endif

	pSched->time = sec*1000 + usec/1000;
	pSched->exec = se.sum_exec_runtime;
	pSched->runnable = stats->wait_sum;
	pSched->iowait = stats->iowait_sum;

	return 0;
#else
	/* always clear sched to simplify error handling at caller side */
	if (pSched)
		memset(pSched, 0, sizeof(struct KAL_THREAD_SCHEDSTATS));
	return -1;
#endif
}

/*
 * kalThreadSchedMark
 * Record the thread's current schedstats and stored in
 * output "schedstats" parameter for profiling at later time.
 * Return value:
 *	 0 : Schedstats successfully recorded
 *	-1 : Kernel's schedstats feature not enabled
 *	-2 : pThread not yet initialized or invalid parameters
 */
int32_t kalThreadSchedMark(struct task_struct *pThread,
				struct KAL_THREAD_SCHEDSTATS *pSchedstats)
{
	return kalThreadSchedRetrieve(pThread, pSchedstats);
}

/*
 * kalThreadSchedUnmark
 * Calculate scheduling statistics against the previously marked point.
 * The result will be filled back into the schedstats output parameter.
 * Return value:
 *	 0 : Schedstats successfully calculated
 *	-1 : Kernel's schedstats feature not enabled
 *	-2 : pThread not yet initialized or invalid parameters
 */
int32_t kalThreadSchedUnmark(struct task_struct *pThread,
				struct KAL_THREAD_SCHEDSTATS *pSchedstats)
{
	int32_t ret;
	struct KAL_THREAD_SCHEDSTATS sched_now;

	if (unlikely(!pSchedstats)) {
		ret = -2;
	} else {
		ret = kalThreadSchedRetrieve(pThread, &sched_now);
		if (ret == 0) {
			pSchedstats->time =
				sched_now.time - pSchedstats->time;
			pSchedstats->exec =
				sched_now.exec - pSchedstats->exec;
			pSchedstats->runnable =
				sched_now.runnable - pSchedstats->runnable;
			pSchedstats->iowait =
				sched_now.iowait - pSchedstats->iowait;
		}
	}
	return ret;
}


/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used to transfer linux ioctl to OID, and  we
 * need to specify the behavior of the OID by ourself
 *
 * @param prGlueInfo         Pointer to the glue structure
 * @param pvInfoBuf          Data buffer
 * @param u4InfoBufLen       Data buffer length
 * @param fgRead             Is this a read OID
 * @param fgWaitResp         does this OID need to wait for values
 * @param fgCmd              does this OID compose command packet
 * @param pu4QryInfoLen      The data length of the return values
 *
 * @retval TRUE      Success to extract information
 * @retval FALSE     Fail to extract correct information
 */
/*----------------------------------------------------------------------------*/

/* todo: enqueue the i/o requests for multiple processes access */
/*  */
/* currently, return -1 */
/*  */

/* static GL_IO_REQ_T OidEntry; */

uint8_t GET_IOCTL_BSSIDX(
	struct ADAPTER *prAdapter)
{
	uint8_t ucBssIndex = 0;
	struct GL_IO_REQ *prIoReq = NULL;

	if (prAdapter) {
		ucBssIndex = aisGetDefaultLinkBssIndex(prAdapter);

		if (prAdapter->prGlueInfo) {
			prIoReq =
				&(prAdapter->prGlueInfo
				->OidEntry);

			ucBssIndex = prIoReq->ucBssIndex;
		}
	}

	return ucBssIndex;
}

void SET_IOCTL_BSSIDX(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	if (prAdapter) {
		struct GL_IO_REQ *prIoReq = NULL;

		prIoReq =
			&(prAdapter->prGlueInfo
			->OidEntry);

		prIoReq->ucBssIndex = ucBssIndex;
	}
}

uint32_t
kalIoctl(struct GLUE_INFO *prGlueInfo,
	 PFN_OID_HANDLER_FUNC pfnOidHandler,
	 void *pvInfoBuf, uint32_t u4InfoBufLen,
	 uint32_t *pu4QryInfoLen)
{
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;

	return kalIoctlByBssIdx(
		prGlueInfo,
		pfnOidHandler,
		pvInfoBuf,
		u4InfoBufLen,
		pu4QryInfoLen,
		prWifiVar->ucBssIdStartValue);
}

/**
 * kalIoctlByBssIdx() - perform request to run in driver thread context
 *
 * @prGlueInfo: Pointer to Glue Info
 * @pfnOidHandler: Pointer to oid handler function executed in driver context
 * @pvInfoBuf: Input buffer
 * @u4InfoBufLen: Size of Input buffer
 * @pu4QryInfoLen: Returning stuffed bytes in query buffer
 * @ucBssIndex: Bss Index
 *
 * Return: return code by kalOidComplete()
 */
uint32_t
kalIoctlByBssIdx(struct GLUE_INFO *prGlueInfo,
	 PFN_OID_HANDLER_FUNC pfnOidHandler,
	 void *pvInfoBuf, uint32_t u4InfoBufLen,
	 uint32_t *pu4QryInfoLen, uint8_t ucBssIndex)
{
	struct GL_IO_REQ *prIoReq = NULL;
	struct KAL_THREAD_SCHEDSTATS schedstats;
	struct ADAPTER *prAdapter;
	uint32_t ret = WLAN_STATUS_SUCCESS;
	uint32_t waitRet = 0;
	int r;
	u_int8_t fgCmdDbgEn;

	KAL_TIME_INTERVAL_DECLARATION();

	if ((prGlueInfo == NULL) || (prGlueInfo->prAdapter == NULL))
		return WLAN_STATUS_FAILURE;

	prAdapter = prGlueInfo->prAdapter;
	fgCmdDbgEn = wlanIfCmdDbgEn(prAdapter);

	KAL_REC_TIME_START();

	/* GLUE_SPIN_LOCK_DECLARATION(); */

	/* <1> Check if driver is halt */
	/* if (prGlueInfo->u4Flag & GLUE_FLAG_HALT) { */
	/* return WLAN_STATUS_ADAPTER_NOT_READY; */
	/* } */

	r = down_killable(&g_halt_sem);
	if (r) {
		DBGLOG(OID, WARN, "down_killable(g_halt_sem) = %d\n", r);
		return WLAN_STATUS_FAILURE;
	}

	if (g_u4HaltFlag) {
		up(&g_halt_sem);
		return WLAN_STATUS_ADAPTER_NOT_READY;
	}

	ASSERT(prGlueInfo);

	r = down_killable(&prGlueInfo->ioctl_sem);
	if (r) {
		DBGLOG(OID, WARN, "down_killable(ioctl_sem) = %d\n", r);
		up(&g_halt_sem);
		return WLAN_STATUS_FAILURE;
	}

	if (kalIsResetting()) {
		up(&prGlueInfo->ioctl_sem);
		up(&g_halt_sem);
		return WLAN_STATUS_SUCCESS;
	}

	if (wlanIsChipAssert(prGlueInfo->prAdapter)) {
		up(&prGlueInfo->ioctl_sem);
		up(&g_halt_sem);
		return WLAN_STATUS_SUCCESS;
	}

	if (prGlueInfo->main_thread == NULL) {
		dump_stack();
		DBGLOG(OID, WARN, "skip executing request.\n");
		up(&prGlueInfo->ioctl_sem);
		up(&g_halt_sem);
		return WLAN_STATUS_FAILURE;
	}

	/* <2> TODO: thread-safe */

	/* <3> point to the OidEntry of Glue layer */

	prIoReq = &(prGlueInfo->OidEntry);

	ASSERT(prIoReq);

	/* <4> Compose the I/O request */
	prIoReq->prAdapter = prGlueInfo->prAdapter;
	prIoReq->pfnOidHandler = pfnOidHandler;
	prIoReq->pvInfoBuf = pvInfoBuf;
	prIoReq->u4InfoBufLen = u4InfoBufLen;
	prIoReq->pu4QryInfoLen = pu4QryInfoLen;
	prIoReq->rStatus = WLAN_STATUS_FAILURE;
	SET_IOCTL_BSSIDX(prGlueInfo->prAdapter, ucBssIndex);

	/* <5> Reset the status of pending OID */
	prGlueInfo->rPendStatus = WLAN_STATUS_FAILURE;
	/* prGlueInfo->u4TimeoutFlag = 0; */
	prGlueInfo->fgOidWaiting = TRUE;

	/* <7> schedule the OID bit
	 * Use memory barrier to ensure OidEntry is written done and then set
	 * bit.
	 */
	if (prGlueInfo->rPendComp.done != 0) {
		uint32_t wIdx, cIdx;
		int i;

		for (i = OID_HDLR_REC_NUM - 1,
				wIdx = prAdapter->u4WaitRecIdx,
				cIdx = prAdapter->u4CompRecIdx;
				i >= 0; i--) {
			DBGLOG(OID, WARN, "%dth last wait OID hdlr: %s\n",
				OID_HDLR_REC_NUM - i,
				prAdapter->arPrevWaitHdlrRec[
					(wIdx + i) % OID_HDLR_REC_NUM].aucName);
			DBGLOG(OID, WARN, "%dth last comp OID hdlr: %s\n",
				OID_HDLR_REC_NUM - i,
				prAdapter->arPrevCompHdlrRec[
					(cIdx + i) % OID_HDLR_REC_NUM].aucName);
		}
		DBGLOG(OID, WARN, "Current wait OID hdlr: %ps\n",
			pfnOidHandler);
	}
	kalSnprintf(prAdapter->arPrevWaitHdlrRec[
			prAdapter->u4WaitRecIdx].aucName,
			sizeof(prAdapter->arPrevWaitHdlrRec[
			prAdapter->u4WaitRecIdx].aucName),
			fgCmdDbgEn ? "%ps" : "%p",
			pfnOidHandler);
	prAdapter->u4WaitRecIdx = (prAdapter->u4WaitRecIdx + 1)
					% OID_HDLR_REC_NUM;

	smp_mb();
	set_bit(GLUE_FLAG_OID_BIT, &prGlueInfo->ulFlag);

	/* <7.1> Hold wakelock to ensure OS won't be suspended */
	KAL_WAKE_LOCK_TIMEOUT(prGlueInfo->prAdapter,
		prGlueInfo->rTimeoutWakeLock, MSEC_TO_JIFFIES(
		prGlueInfo->prAdapter->rWifiVar.u4WakeLockThreadWakeup));

	/* <8> Wake up main thread to handle kick start the I/O request.
	 * Use memory barrier to ensure set bit is done and then wake up main
	 * thread.
	 */
	smp_mb();
	wake_up_interruptible(&prGlueInfo->waitq);

	/* <9> Block and wait for event or timeout,
	 * current the timeout is 30 secs
	 */
	kalThreadSchedMark(prGlueInfo->main_thread, &schedstats);

	DBGLOG(OID, TRACE, "waiting, pfnOidHandler=%ps, BufLen=%u, QryLen=%p",
			prIoReq->pfnOidHandler, prIoReq->u4InfoBufLen,
			prIoReq->pu4QryInfoLen);
	waitRet = wait_for_completion_timeout(&prGlueInfo->rPendComp,
			MSEC_TO_JIFFIES(KAL_OID_WAIT_TIME));
	DBGLOG(OID, TRACE, "wait=%u, pfnOidHandler=%ps, BufLen=%u, QryLen=%p",
			waitRet, prIoReq->pfnOidHandler,
			prIoReq->u4InfoBufLen, prIoReq->pu4QryInfoLen);
	kalThreadSchedUnmark(prGlueInfo->main_thread, &schedstats);
	if (waitRet > 0) {
		/* Case 1: No timeout. */
		/* if return WLAN_STATUS_PENDING, the status of cmd is stored
		 * in prGlueInfo
		 */
		if (prIoReq->rStatus == WLAN_STATUS_PENDING)
			ret = prGlueInfo->rPendStatus;
		else
			ret = prIoReq->rStatus;
	} else {
#if 0
		/* Case 2: timeout */
		/* clear pending OID's cmd in CMD queue */
		if (fgCmd) {
			prGlueInfo->u4TimeoutFlag = 1;
			wlanReleasePendingOid(prGlueInfo->prAdapter, 0);
		}
#endif
		prGlueInfo->fgOidWaiting = FALSE;
		/* note: do not dump main_thread's call stack here, */
		/*       because it may be running on other cpu.    */
		DBGLOG(OID, WARN,
			"wait main_thread timeout, duration:%llums, sched(x%llu/r%llu/i%llu)\n",
			schedstats.time, schedstats.exec,
			schedstats.runnable, schedstats.iowait);
		KAL_WARN_ON(TRUE);

		ret = WLAN_STATUS_FAILURE;
	}

	/* <10> Clear bit for error handling */
	clear_bit(GLUE_FLAG_OID_BIT, &prGlueInfo->ulFlag);

	up(&prGlueInfo->ioctl_sem);
	up(&g_halt_sem);

	KAL_REC_TIME_END();
	if (ret != WLAN_STATUS_SUCCESS)
		DBGLOG(OID, WARN, "ret(%x) time: %u us\n",
			ret,
			KAL_GET_TIME_INTERVAL());

	return ret;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to clear all pending management frames
 *
 * \param prGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
void kalClearMgmtFrames(struct GLUE_INFO *prGlueInfo)
{
	struct QUE *prCmdQue;
	struct QUE rTempCmdQue;
	struct QUE *prTempCmdQue = &rTempCmdQue;
	struct QUE rReturnCmdQue;
	struct QUE *prReturnCmdQue = &rReturnCmdQue;
	struct QUE_ENTRY *prQueueEntry = (struct QUE_ENTRY *) NULL;
	struct CMD_INFO *prCmdInfo = (struct CMD_INFO *) NULL;

	GLUE_SPIN_LOCK_DECLARATION();

	ASSERT(prGlueInfo);

	QUEUE_INITIALIZE(prReturnCmdQue);
	/* Clear pending management frames in prGlueInfo->rCmdQueue */
	prCmdQue = &prGlueInfo->rCmdQueue;

	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
	QUEUE_MOVE_ALL(prTempCmdQue, prCmdQue);
	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);

	QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry,
			  struct QUE_ENTRY *);
	while (prQueueEntry) {
		prCmdInfo = (struct CMD_INFO *) prQueueEntry;

		if (prCmdInfo->eCmdType == COMMAND_TYPE_MANAGEMENT_FRAME) {
			wlanReleaseCommand(prGlueInfo->prAdapter, prCmdInfo,
					   TX_RESULT_QUEUE_CLEARANCE);
			cmdBufFreeCmdInfo(prGlueInfo->prAdapter, prCmdInfo);
			GLUE_DEC_REF_CNT(prGlueInfo->i4TxPendingCmdNum);
		} else {
			QUEUE_INSERT_TAIL(prReturnCmdQue, prQueueEntry);
		}

		QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry,
				  struct QUE_ENTRY *);
	}

	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
	QUEUE_CONCATENATE_QUEUES_HEAD(prCmdQue, prReturnCmdQue);
	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to clear all pending management frames
 *           belongs to dedicated network type
 * \param prGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
void kalClearMgmtFramesByBssIdx(struct GLUE_INFO
				*prGlueInfo, uint8_t ucBssIndex)
{
	struct QUE *prCmdQue;
	struct QUE rTempCmdQue;
	struct QUE *prTempCmdQue = &rTempCmdQue;
	struct QUE rReturnCmdQue;
	struct QUE *prReturnCmdQue = &rReturnCmdQue;
	struct QUE_ENTRY *prQueueEntry = (struct QUE_ENTRY *) NULL;
	struct CMD_INFO *prCmdInfo = (struct CMD_INFO *) NULL;
	struct MSDU_INFO *prMsduInfo;
	u_int8_t fgFree;

	GLUE_SPIN_LOCK_DECLARATION();

	ASSERT(prGlueInfo);

	QUEUE_INITIALIZE(prReturnCmdQue);
	/* Clear pending management frames in prGlueInfo->rCmdQueue */
	prCmdQue = &prGlueInfo->rCmdQueue;

	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
	QUEUE_MOVE_ALL(prTempCmdQue, prCmdQue);
	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);

	QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry,
			  struct QUE_ENTRY *);
	while (prQueueEntry) {
		prCmdInfo = (struct CMD_INFO *) prQueueEntry;
		prMsduInfo = prCmdInfo->prMsduInfo;
		fgFree = FALSE;

		if (prCmdInfo->eCmdType == COMMAND_TYPE_MANAGEMENT_FRAME
		    && prMsduInfo) {
			if (prMsduInfo->ucBssIndex == ucBssIndex)
				fgFree = TRUE;
		}

		if (fgFree) {
			wlanReleaseCommand(prGlueInfo->prAdapter, prCmdInfo,
					   TX_RESULT_QUEUE_CLEARANCE);
			cmdBufFreeCmdInfo(prGlueInfo->prAdapter, prCmdInfo);
			GLUE_DEC_REF_CNT(prGlueInfo->i4TxPendingCmdNum);
		} else {
			QUEUE_INSERT_TAIL(prReturnCmdQue, prQueueEntry);
		}

		QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry,
				  struct QUE_ENTRY *);
	}

	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
	QUEUE_CONCATENATE_QUEUES_HEAD(prCmdQue, prReturnCmdQue);
	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
}				/* kalClearMgmtFramesByBssIdx */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to clear all commands in command queue
 * \param prGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
void kalClearCommandQueue(struct GLUE_INFO *prGlueInfo,
	u_int8_t fgIsNeedHandler)
{
	struct QUE *prCmdQue;
	struct QUE rTempCmdQue;
	struct QUE *prTempCmdQue = &rTempCmdQue;
	struct QUE_ENTRY *prQueueEntry = (struct QUE_ENTRY *) NULL;
	struct CMD_INFO *prCmdInfo = (struct CMD_INFO *) NULL;

	GLUE_SPIN_LOCK_DECLARATION();

	ASSERT(prGlueInfo);

	QUEUE_INITIALIZE(prTempCmdQue);

	/* Clear ALL in prGlueInfo->rCmdQueue */
	prCmdQue = &prGlueInfo->rCmdQueue;

	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
	QUEUE_MOVE_ALL(prTempCmdQue, prCmdQue);
	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);

	QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry,
			  struct QUE_ENTRY *);
	while (prQueueEntry) {
		prCmdInfo = (struct CMD_INFO *) prQueueEntry;

		if (prCmdInfo->pfCmdTimeoutHandler)
			prCmdInfo->pfCmdTimeoutHandler(prGlueInfo->prAdapter,
						       prCmdInfo);
		else
			wlanReleaseCommandEx(prGlueInfo->prAdapter, prCmdInfo,
					   TX_RESULT_QUEUE_CLEARANCE,
					   fgIsNeedHandler);

		cmdBufFreeCmdInfo(prGlueInfo->prAdapter, prCmdInfo);
		GLUE_DEC_REF_CNT(prGlueInfo->i4TxPendingCmdNum);

		QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry,
				  struct QUE_ENTRY *);
	}

	if (prTempCmdQue->u4NumElem != 0)
		DBGLOG(INIT, INFO, "After clear, cmd queue(%d)\n",
			prTempCmdQue->u4NumElem);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to check use packet data into command Queue flow
 *
 * \param prAdapter     Pointer of prAdapter Structure
 * \param prSkb         Pointer to TX packet
 * \retval is TRUE (run CmdData path flow) or FALSE (not use)
 */
/*----------------------------------------------------------------------------*/
uint32_t kalProcessTxPacket(struct GLUE_INFO *prGlueInfo,
			    struct sk_buff *prSkb)
{
	uint32_t u4Status = WLAN_STATUS_SUCCESS;

	if (prSkb == NULL) {
		DBGLOG(INIT, WARN, "prSkb == NULL in tx\n");
		return u4Status;
	} else
		u4Status = wlanEnqueueTxPacket(prGlueInfo->prAdapter,
					       (void *) prSkb);

	return u4Status;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to process Tx request to main_thread
 *
 * \param prGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
void kalProcessTxReq(struct GLUE_INFO *prGlueInfo,
		     u_int8_t *pfgNeedHwAccess)
{
	struct QUE *prCmdQue = NULL;
	struct QUE *prTxQueue = NULL;
	struct QUE rTempQue;
	struct QUE *prTempQue = &rTempQue;
	struct QUE rTempReturnQue;
	struct QUE *prTempReturnQue = &rTempReturnQue;
	struct QUE_ENTRY *prQueueEntry = NULL;
	/* struct sk_buff      *prSkb = NULL; */
	uint32_t u4Status;
#if CFG_SUPPORT_MULTITHREAD
	uint32_t u4CmdCount = 0;
#endif
	uint32_t u4TxLoopCount;

	/* for spin lock acquire and release */
	GLUE_SPIN_LOCK_DECLARATION();

	prTxQueue = &prGlueInfo->rTxQueue;
	prCmdQue = &prGlueInfo->rCmdQueue;

	QUEUE_INITIALIZE(prTempQue);
	QUEUE_INITIALIZE(prTempReturnQue);

	u4TxLoopCount =
		prGlueInfo->prAdapter->rWifiVar.u4TxFromOsLoopCount;

	/* Process Mailbox Messages */
	wlanProcessMboxMessage(prGlueInfo->prAdapter);

	/* Process CMD request */
#if CFG_SUPPORT_MULTITHREAD
	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
	u4CmdCount = prCmdQue->u4NumElem;
	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
	if (u4CmdCount > 0)
		wlanProcessCommandQueue(prGlueInfo->prAdapter, prCmdQue);

#else
	if (prCmdQue->u4NumElem > 0) {
		if (*pfgNeedHwAccess == FALSE) {
			*pfgNeedHwAccess = TRUE;

			wlanAcquirePowerControl(prGlueInfo->prAdapter);
		}
		wlanProcessCommandQueue(prGlueInfo->prAdapter, prCmdQue);
	}
#endif

	while (u4TxLoopCount--) {
		while (QUEUE_IS_NOT_EMPTY(prTxQueue)) {
			GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_QUE);
			QUEUE_MOVE_ALL(prTempQue, prTxQueue);
			GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_QUE);

			kalTraceBegin("Move TxQueue %d", prTempQue->u4NumElem);
			/* Handle Packet Tx */
			while (QUEUE_IS_NOT_EMPTY(prTempQue)) {
				QUEUE_REMOVE_HEAD(prTempQue, prQueueEntry,
						  struct QUE_ENTRY *);

				if (prQueueEntry == NULL)
					break;

				u4Status = kalProcessTxPacket(prGlueInfo,
					      (struct sk_buff *)
					      GLUE_GET_PKT_DESCRIPTOR(
					      prQueueEntry));
				/* Enqueue packet back into TxQueue if resource
				 * is not enough
				 */
				if (u4Status == WLAN_STATUS_RESOURCES) {
					QUEUE_INSERT_TAIL(prTempReturnQue,
							  prQueueEntry);
					break;
				}

				if (wlanGetTxPendingFrameCount(
					    prGlueInfo->prAdapter) > 0)
					wlanTxPendingPackets(
						prGlueInfo->prAdapter,
						pfgNeedHwAccess);
			}
			kalTraceEnd();

			/* Enqueue packet back into TxQueue if resource is not
			 * enough
			 */
			if (QUEUE_IS_NOT_EMPTY(prTempReturnQue)) {
				QUEUE_CONCATENATE_QUEUES(prTempReturnQue,
							 prTempQue);

				GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo,
						       SPIN_LOCK_TX_QUE);
				QUEUE_CONCATENATE_QUEUES_HEAD(prTxQueue,
							      prTempReturnQue);
				GLUE_RELEASE_SPIN_LOCK(prGlueInfo,
						       SPIN_LOCK_TX_QUE);

				break;
			}
		}
		if (wlanGetTxPendingFrameCount(prGlueInfo->prAdapter) > 0)
			wlanTxPendingPackets(prGlueInfo->prAdapter,
					     pfgNeedHwAccess);
	}

}

#if CFG_SUPPORT_MULTITHREAD

/*----------------------------------------------------------------------------*/
/*!
 * @brief
 *
 * @param data       data pointer to private data of hif_thread
 *
 * @retval           If the function succeeds, the return value is 0.
 * Otherwise, an error code is returned.
 *
 */
/*----------------------------------------------------------------------------*/

int hif_thread(void *data)
{
	struct net_device *dev = data;
	struct GLUE_INFO *prGlueInfo = *((struct GLUE_INFO **)
					 netdev_priv(dev));
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
	int ret = 0;
	bool fgEnInt;
	struct RX_CTRL *prRxCtrl;
#if CFG_ENABLE_WAKE_LOCK
	KAL_WAKE_LOCK_T *prHifThreadWakeLock;

	KAL_WAKE_LOCK_INIT(prGlueInfo->prAdapter,
			   prHifThreadWakeLock, "WLAN hif_thread");
	KAL_WAKE_LOCK(prGlueInfo->prAdapter, prHifThreadWakeLock);
#endif

	DBGLOG(INIT, INFO, "%s:%u starts running...\n",
	       KAL_GET_CURRENT_THREAD_NAME(), KAL_GET_CURRENT_THREAD_ID());

	prGlueInfo->u4HifThreadPid = KAL_GET_CURRENT_THREAD_ID();

	kalSetThreadSchPolicyPriority(prGlueInfo);
	prRxCtrl = &prAdapter->rRxCtrl;
#if CFG_SUPPORT_TPUT_FACTOR
#if KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
	prGlueInfo->hif_cpu_mask = current->cpus_mask;
#endif
#endif /* CFG_SUPPORT_TPUT_FACTOR */

	while (TRUE) {

		if (test_bit(GLUE_FLAG_HALT_BIT, &prGlueInfo->ulFlag) ||
		    kalIsResetting()) {
			DBGLOG(INIT, INFO, "hif_thread should stop now...\n");
			break;
		}

		/* Unlock wakelock if hif_thread going to idle */
		if (!(prGlueInfo->ulFlag & GLUE_FLAG_HIF_PROCESS) &&
			!(prGlueInfo->ulHifFlag & HIF_FLAG))
			KAL_WAKE_UNLOCK(prGlueInfo->prAdapter,
					prHifThreadWakeLock);

		/*
		 * sleep on waitqueue if no events occurred. Event contain
		 * (1) GLUE_FLAG_INT (2) GLUE_FLAG_OID (3) GLUE_FLAG_TXREQ
		 * (4) GLUE_FLAG_HALT
		 *
		 */
		do {
			ret = wait_event_interruptible(prGlueInfo->waitq_hif,
				((prGlueInfo->ulFlag & GLUE_FLAG_HIF_PROCESS)
				!= 0) ||
				((prGlueInfo->ulHifFlag & HIF_FLAG) != 0));
		} while (ret != 0);

		kalTraceBegin("hif_thread");
#if CFG_SUPPORT_TPUT_FACTOR
#if KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
		prGlueInfo->hif_cpu_mask = current->cpus_mask;
#endif
#endif /* CFG_SUPPORT_TPUT_FACTOR */

		if (test_bit(GLUE_FLAG_HIF_TX_BIT,
					&prGlueInfo->ulFlag))
			RX_INC_HIF_CNT(prRxCtrl, HIF_FLAG_HIF_TX);
		if (test_bit(GLUE_FLAG_HIF_TX_CMD_BIT,
					&prGlueInfo->ulFlag))
			RX_INC_HIF_CNT(prRxCtrl, HIF_FLAG_HIF_TX_CMD);

#if CFG_ENABLE_WAKE_LOCK
		if (!KAL_WAKE_LOCK_ACTIVE(prGlueInfo->prAdapter,
					  prHifThreadWakeLock))
			KAL_WAKE_LOCK(prGlueInfo->prAdapter,
				      prHifThreadWakeLock);
#endif
		if (prAdapter->fgIsFwOwn
		    && (prGlueInfo->ulFlag == GLUE_FLAG_HIF_FW_OWN)) {
			DBGLOG(INIT, INFO,
			       "Only FW OWN request, but now already done FW OWN\n");
			clear_bit(GLUE_FLAG_HIF_FW_OWN_BIT,
				  &prGlueInfo->ulFlag);
			continue;
		}
		wlanAcquirePowerControl(prAdapter);

		/* Handle Interrupt */
		fgEnInt = test_and_clear_bit(
			GLUE_FLAG_INT_BIT, &prGlueInfo->ulFlag);
		if (fgEnInt ||
		    test_and_clear_bit(GLUE_FLAG_DRV_INT_BIT,
				       &prGlueInfo->ulFlag)) {
			kalTraceBegin("INT");
			if (fgEnInt)
				RX_INC_HIF_CNT(prRxCtrl, HIF_FLAG_INT);
			else
				RX_INC_HIF_CNT(prRxCtrl, HIF_FLAG_DRV_INT);

			if (test_bit(GLUE_FLAG_HALT_BIT, &prGlueInfo->ulFlag)
				|| kalIsResetting()) {
				if (!kalIsResetting())
					RX_INC_HIF_CNT(prRxCtrl, HIF_FLAG_HALT);
				/* Should stop now... skip pending interrupt */
				DBGLOG(INIT, INFO,
				       "ignore pending interrupt\n");
			} else {
				/* DBGLOG(INIT, INFO, ("HIF Interrupt!\n")); */
				prGlueInfo->TaskIsrCnt++;
				wlanIST(prAdapter, fgEnInt);
			}
			kalTraceEnd();
		}

		if (test_and_clear_bit(GLUE_FLAG_SER_INT_BIT,
				       &prGlueInfo->ulFlag)) {
			RX_INC_HIF_CNT(prRxCtrl, HIF_FLAG_SER_INT);
			TRACE(nicProcessSoftwareInterrupt(prAdapter),
				"SER-INT");
		}

		/* Skip Tx request if SER is operating */
		if ((prAdapter->fgIsFwOwn == FALSE) &&
		    !nicSerIsTxStop(prAdapter)) {
			/* TX Commands */
			if (test_and_clear_bit(GLUE_FLAG_HIF_TX_CMD_BIT,
					       &prGlueInfo->ulFlag))
				TRACE(wlanTxCmdMthread(prAdapter), "TX_CMD");
#if (CFG_TX_MGMT_BY_DATA_Q == 1)
			if (test_and_clear_bit(GLUE_FLAG_MGMT_DIRECT_HIF_TX_BIT,
				&prGlueInfo->ulFlag)) {
				RX_INC_HIF_CNT(prRxCtrl,
					HIF_FLAG_MGMT_DIRECT_HIF_TX);
				TRACE(nicTxMgmtDirectTxMsduMthread(prAdapter),
					"Mgmt-DirectTx");
			}
#endif /* CFG_TX_MGMT_BY_DATA_Q == 1 */

			/* Process TX data packet to HIF */
			if (test_and_clear_bit(GLUE_FLAG_HIF_TX_BIT,
					       &prGlueInfo->ulFlag))
				TRACE(nicTxMsduQueueMthread(prAdapter),	"TX");
		}

		/* Read chip status when chip no response */
		if (test_and_clear_bit(GLUE_FLAG_HIF_PRT_HIF_DBG_INFO_BIT,
				       &prGlueInfo->ulFlag)) {
			RX_INC_HIF_CNT(prRxCtrl,
					HIF_FLAG_HIF_PRT_HIF_DBG_INFO);
			TRACE(halPrintHifDbgInfo(prAdapter), "DBG_INFO");
		}

		/* Update Tx Quota */
		if (test_and_clear_bit(GLUE_FLAG_UPDATE_WMM_QUOTA_BIT,
					&prGlueInfo->ulFlag)) {
			RX_INC_HIF_CNT(prRxCtrl, HIF_FLAG_UPDATE_WMM_QUOTA);
			TRACE(halUpdateTxMaxQuota(prAdapter), "UPDATE_WMM");
		}
#if CFG_MTK_MDDP_SUPPORT
		/* Notify MD crash to FW */
		if (test_and_clear_bit(GLUE_FLAG_HIF_MDDP_BIT,
				       &prGlueInfo->ulFlag)) {
			RX_INC_HIF_CNT(prRxCtrl, HIF_FLAG_HIF_MDDP);
			mddpInHifThread(prAdapter);
		}
#endif

#if defined(_HIF_PCIE)
		/* HIF thread trigger whole chip reset */
		if (test_and_clear_bit(HIF_FLAG_AER_RESET_BIT,
				       &prGlueInfo->ulHifFlag))
			mtk_trigger_aer_slot_reset();

		/* Recovery MSI status */
		if (test_and_clear_bit(HIF_FLAG_MSI_RECOVERY_BIT,
				       &prGlueInfo->ulHifFlag)) {
			struct BUS_INFO *prBusInfo =
				prAdapter->chip_info->bus_info;

			if (prBusInfo->recoveryMsiStatus)
				prBusInfo->recoveryMsiStatus(prAdapter, TRUE);
		}
#endif
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
		if (test_and_clear_bit(HIF_FLAG_ALL_TOKENS_UNUSED_BIT,
				       &prGlueInfo->ulHifFlag))
			halHandleAllTokensUnused(prAdapter, FALSE);
#endif

		/* Set FW own */
		if (test_and_clear_bit(GLUE_FLAG_HIF_FW_OWN_BIT,
				       &prGlueInfo->ulFlag)) {
			RX_INC_HIF_CNT(prRxCtrl, HIF_FLAG_HIF_FW_OWN);
			prAdapter->fgWiFiInSleepyState = TRUE;
		}
		halUpdateHifConfig(prAdapter);
		halDumpHifStats(prAdapter);

		/* Release to FW own */
		wlanReleasePowerControl(prAdapter);
		kalTraceEnd(); /* hif_thread */
	}

	complete(&prGlueInfo->rHifHaltComp);
#if CFG_ENABLE_WAKE_LOCK
	if (KAL_WAKE_LOCK_ACTIVE(prGlueInfo->prAdapter,
				 prHifThreadWakeLock))
		KAL_WAKE_UNLOCK(prGlueInfo->prAdapter, prHifThreadWakeLock);
	KAL_WAKE_LOCK_DESTROY(prGlueInfo->prAdapter,
			      prHifThreadWakeLock);
#endif

	DBGLOG(INIT, INFO, "%s:%u stopped!\n",
	       KAL_GET_CURRENT_THREAD_NAME(), KAL_GET_CURRENT_THREAD_ID());

#if CFG_CHIP_RESET_HANG
	while (fgIsResetHangState == SER_L0_HANG_RST_HANG) {
		kalMsleep(SER_L0_HANG_LOG_TIME_INTERVAL);
		DBGLOG(INIT, STATE, "[SER][L0] SQC hang!\n");
	}
#endif

	prGlueInfo->hif_thread = NULL;
	prGlueInfo->u4HifThreadPid = 0xffffffff;

	return 0;
}

int rx_thread(void *data)
{
	struct net_device *dev = data;
	struct GLUE_INFO *prGlueInfo = *((struct GLUE_INFO **)
					 netdev_priv(dev));

	struct QUE rTempRxQue;
	struct QUE *prTempRxQue = NULL;
	struct QUE_ENTRY *prQueueEntry = NULL;

	int ret = 0;
#if CFG_ENABLE_WAKE_LOCK
	KAL_WAKE_LOCK_T *prRxThreadWakeLock;
#endif
	uint32_t u4LoopCount;

	/* for spin lock acquire and release */
	KAL_SPIN_LOCK_DECLARATION();

#if CFG_ENABLE_WAKE_LOCK
	KAL_WAKE_LOCK_INIT(prGlueInfo->prAdapter,
			   prRxThreadWakeLock, "WLAN rx_thread");
	KAL_WAKE_LOCK(prGlueInfo->prAdapter, prRxThreadWakeLock);
#endif

	DBGLOG(INIT, INFO, "%s:%u starts running...\n",
	       KAL_GET_CURRENT_THREAD_NAME(), KAL_GET_CURRENT_THREAD_ID());

	prGlueInfo->u4RxThreadPid = KAL_GET_CURRENT_THREAD_ID();

	kalSetThreadSchPolicyPriority(prGlueInfo);

	prTempRxQue = &rTempRxQue;

	while (TRUE) {

		if (test_bit(GLUE_FLAG_HALT_BIT, &prGlueInfo->ulFlag)
			|| kalIsResetting()
			) {
			DBGLOG(INIT, INFO, "rx_thread should stop now...\n");
			break;
		}

		/* Unlock wakelock if rx_thread going to idle */
		if (!(prGlueInfo->ulFlag & GLUE_FLAG_RX_PROCESS))
			KAL_WAKE_UNLOCK(prGlueInfo->prAdapter,
					prRxThreadWakeLock);

		/*
		 * sleep on waitqueue if no events occurred.
		 */
		do {
			ret = wait_event_interruptible(prGlueInfo->waitq_rx,
			    ((prGlueInfo->ulFlag & GLUE_FLAG_RX_PROCESS) != 0));
		} while (ret != 0);

		kalTraceBegin("rx_thread");

#if CFG_ENABLE_WAKE_LOCK
		if (!KAL_WAKE_LOCK_ACTIVE(prGlueInfo->prAdapter,
					  prRxThreadWakeLock))
			KAL_WAKE_LOCK(prGlueInfo->prAdapter,
				      prRxThreadWakeLock);
#endif
		if (test_and_clear_bit(GLUE_FLAG_RX_TO_OS_BIT,
				       &prGlueInfo->ulFlag)) {
			kalTraceBegin("RX_TO_OS");

#if CFG_SUPPORT_RX_NAPI && CFG_SUPPORT_RX_NAPI_IN_RX_THREAD
			if (HAL_IS_RX_DIRECT(prGlueInfo->prAdapter) &&
				!KAL_FIFO_IS_EMPTY(&prGlueInfo->rRxKfifoQ)) {
				__kalNapiSchedule(prGlueInfo->prAdapter);
			}
#endif

			u4LoopCount =
			    prGlueInfo->prAdapter->rWifiVar.u4Rx2OsLoopCount;

			while (u4LoopCount--) {
				while (QUEUE_IS_NOT_EMPTY(
				       &prGlueInfo->prAdapter->rRxQueue)) {
					QUEUE_INITIALIZE(prTempRxQue);

					GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo,
					    SPIN_LOCK_RX_TO_OS_QUE);
					QUEUE_MOVE_ALL(prTempRxQue,
					    &prGlueInfo->prAdapter->rRxQueue);
					GLUE_RELEASE_SPIN_LOCK(prGlueInfo,
					    SPIN_LOCK_RX_TO_OS_QUE);

					while (QUEUE_IS_NOT_EMPTY(
						prTempRxQue)) {
						QUEUE_REMOVE_HEAD(prTempRxQue,
						    prQueueEntry,
						    struct QUE_ENTRY *);
						kalRxIndicateOnePkt(prGlueInfo,
						    (void *)
						    GLUE_GET_PKT_DESCRIPTOR(
						    prQueueEntry));
					}

				    KAL_WAKE_LOCK_TIMEOUT(prGlueInfo->prAdapter,
					prGlueInfo->rTimeoutWakeLock,
					MSEC_TO_JIFFIES(prGlueInfo->prAdapter
					->rWifiVar.u4WakeLockRxTimeout));
				}
			}
#if CFG_SUPPORT_RX_GRO
#if (CFG_SUPPORT_RX_NAPI == 0)
			kal_gro_flush_queue(prGlueInfo);
#endif /* CFG_SUPPORT_RX_NAPI == 0 */
#endif /* CFG_SUPPORT_RX_GRO */
			kalTraceEnd(); /* RX_TO_OS */
		}

		kalTraceEnd(); /* rx_thread*/
	}

	complete(&prGlueInfo->rRxHaltComp);
#if CFG_ENABLE_WAKE_LOCK
	if (KAL_WAKE_LOCK_ACTIVE(prGlueInfo->prAdapter,
				 prRxThreadWakeLock))
		KAL_WAKE_UNLOCK(prGlueInfo->prAdapter, prRxThreadWakeLock);
	KAL_WAKE_LOCK_DESTROY(prGlueInfo->prAdapter,
			      prRxThreadWakeLock);
#endif

	DBGLOG(INIT, INFO, "%s:%u stopped!\n",
	       KAL_GET_CURRENT_THREAD_NAME(), KAL_GET_CURRENT_THREAD_ID());

#if CFG_CHIP_RESET_HANG
	while (fgIsResetHangState == SER_L0_HANG_RST_HANG) {
		kalMsleep(SER_L0_HANG_LOG_TIME_INTERVAL);
		DBGLOG(INIT, STATE, "[SER][L0] SQC hang!\n");
	}
#endif

	prGlueInfo->rx_thread = NULL;
	prGlueInfo->u4RxThreadPid = 0xffffffff;

	return 0;
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is a kernel thread function for handling command packets
 * Tx requests and interrupt events
 *
 * @param data       data pointer to private data of main_thread
 *
 * @retval           If the function succeeds, the return value is 0.
 * Otherwise, an error code is returned.
 *
 */
/*----------------------------------------------------------------------------*/

int main_thread(void *data)
{
	struct net_device *dev = data;
	struct GLUE_INFO *prGlueInfo = *((struct GLUE_INFO **)
					 netdev_priv(dev));
	struct GL_IO_REQ *prIoReq = NULL;
	int ret = 0;
	u_int8_t fgNeedHwAccess = FALSE;
#if CFG_ENABLE_WAKE_LOCK
	KAL_WAKE_LOCK_T *prTxThreadWakeLock;
#endif
#ifdef CFG_MTK_CONNSYS_DEDICATED_LOG_PATH
	struct CMD_CONNSYS_FW_LOG rFwLogCmd;
	uint32_t u4BufLen;
	uint32_t u4FwLevel = ENUM_WIFI_LOG_LEVEL_DEFAULT;
#endif

#if CFG_SUPPORT_MULTITHREAD
	prGlueInfo->u4TxThreadPid = KAL_GET_CURRENT_THREAD_ID();
#endif

	current->flags |= PF_NOFREEZE;
	ASSERT(prGlueInfo);
	ASSERT(prGlueInfo->prAdapter);
	kalSetThreadSchPolicyPriority(prGlueInfo);

#if CFG_ENABLE_WAKE_LOCK
	KAL_WAKE_LOCK_INIT(prGlueInfo->prAdapter,
			   prTxThreadWakeLock, "WLAN main_thread");
	KAL_WAKE_LOCK(prGlueInfo->prAdapter, prTxThreadWakeLock);
#endif

	DBGLOG(INIT, INFO, "%s:%u starts running...\n",
	       KAL_GET_CURRENT_THREAD_NAME(), KAL_GET_CURRENT_THREAD_ID());

#if CFG_SUPPORT_SA_LOG
	rtc_update = jiffies;	/* update rtc_update time base */
#endif

	while (TRUE) {
#ifdef UT_TEST_MODE
		testThreadBegin(prGlueInfo->prAdapter);
#endif

		if (test_bit(GLUE_FLAG_HALT_BIT, &prGlueInfo->ulFlag)
			|| kalIsResetting()
			) {
			DBGLOG(INIT, INFO, "%s should stop now...\n",
			       KAL_GET_CURRENT_THREAD_NAME());
			break;
		}

		/* Unlock wakelock if main_thread going to idle */
		if (!(prGlueInfo->ulFlag & GLUE_FLAG_MAIN_PROCESS))
			KAL_WAKE_UNLOCK(prGlueInfo->prAdapter,
					prTxThreadWakeLock);

		/*
		 * sleep on waitqueue if no events occurred. Event contain
		 * (1) GLUE_FLAG_INT (2) GLUE_FLAG_OID (3) GLUE_FLAG_TXREQ
		 * (4) GLUE_FLAG_HALT
		 */
		do {
			ret = wait_event_interruptible(prGlueInfo->waitq,
				((prGlueInfo->ulFlag & GLUE_FLAG_MAIN_PROCESS)
				!= 0));
		} while (ret != 0);
#if CFG_ENABLE_WAKE_LOCK
		if (!KAL_WAKE_LOCK_ACTIVE(prGlueInfo->prAdapter,
					  prTxThreadWakeLock))
			KAL_WAKE_LOCK(prGlueInfo->prAdapter,
				      prTxThreadWakeLock);
#endif
		kalTraceBegin("main_thread");

		if (test_and_clear_bit(GLUE_FLAG_DISABLE_PERF_BIT,
			&prGlueInfo->ulFlag))
			kalPerMonDisable(prGlueInfo);

#if CFG_ENABLE_WIFI_DIRECT
		/*run p2p multicast list work. */
		if (test_and_clear_bit(GLUE_FLAG_SUB_MOD_MULTICAST_BIT,
				       &prGlueInfo->ulFlag))
			p2pSetMulticastListWorkQueueWrapper(prGlueInfo);

		if (test_and_clear_bit(GLUE_FLAG_FRAME_FILTER_BIT,
				       &prGlueInfo->ulFlag)
			&& prGlueInfo->prP2PDevInfo) {
#if CFG_ENABLE_WIFI_DIRECT_CFG_80211
			/* P2p info will be null after p2p removed. */
			p2pFuncUpdateMgmtFrameRegister(prGlueInfo->prAdapter,
			       prGlueInfo->prP2PDevInfo->u4OsMgmtFrameFilter);
#endif
		}
#endif

#if CFG_SUPPORT_NAN
		if (test_and_clear_bit(GLUE_FLAG_NAN_MULTICAST_BIT,
				       &prGlueInfo->ulFlag))
			nanSetMulticastListWorkQueueWrapper(prGlueInfo);
#endif

		if (test_bit(GLUE_FLAG_HALT_BIT, &prGlueInfo->ulFlag)
			|| kalIsResetting()) {
			DBGLOG(INIT, INFO, "%s should stop now...\n",
			       KAL_GET_CURRENT_THREAD_NAME());
			break;
		}

		fgNeedHwAccess = FALSE;

#if CFG_SUPPORT_SDIO_READ_WRITE_PATTERN
		if (prGlueInfo->fgEnSdioTestPattern == TRUE) {
			if (fgNeedHwAccess == FALSE) {
				fgNeedHwAccess = TRUE;

				wlanAcquirePowerControl(prGlueInfo->prAdapter);
			}

			if (prGlueInfo->fgIsSdioTestInitialized == FALSE) {
				/* enable PRBS mode */
				kalDevRegWrite(prGlueInfo, MCR_WTMCR,
					       0x00080002);
				prGlueInfo->fgIsSdioTestInitialized = TRUE;
			}

			if (prGlueInfo->fgSdioReadWriteMode == TRUE) {
				/* read test */
				kalDevPortRead(prGlueInfo, MCR_WTMDR, 256,
					prGlueInfo->aucSdioTestBuffer,
					sizeof(prGlueInfo->aucSdioTestBuffer),
					FALSE);
			} else {
				/* write test */
				kalDevPortWrite(prGlueInfo, MCR_WTMDR, 172,
					prGlueInfo->aucSdioTestBuffer,
					sizeof(prGlueInfo->aucSdioTestBuffer));
			}
		}
#endif
#if CFG_SUPPORT_MULTITHREAD
#else

		/* Handle Interrupt */
		if (test_and_clear_bit(GLUE_FLAG_INT_BIT,
				       &prGlueInfo->ulFlag)) {
			kalTraceBegin("INT");
			if (fgNeedHwAccess == FALSE) {
				fgNeedHwAccess = TRUE;

				wlanAcquirePowerControl(prGlueInfo->prAdapter);
			}

			if (test_bit(GLUE_FLAG_HALT_BIT, &prGlueInfo->ulFlag) ||
					kalIsResetting()) {
				/* Should stop now... skip pending interrupt */
				DBGLOG(INIT, INFO,
					"ignore pending interrupt\n");
			} else {
				prGlueInfo->TaskIsrCnt++;
				wlanIST(prGlueInfo->prAdapter, true);
			}
			kalTraceEnd();
		}

		if (test_and_clear_bit(GLUE_FLAG_SER_INT_BIT,
				       &prGlueInfo->ulFlag)) {
			TRACE(nicProcessSoftwareInterrupt(prAdapter),
				"SER-INT");
		}

		/* Read chip status when chip no response */
		if (test_and_clear_bit(GLUE_FLAG_HIF_PRT_HIF_DBG_INFO_BIT,
				       &prGlueInfo->ulFlag))
			halPrintHifDbgInfo(prGlueInfo->prAdapter);

		if (test_and_clear_bit(GLUE_FLAG_UPDATE_WMM_QUOTA_BIT,
					&prGlueInfo->ulFlag))
			TRACE(halUpdateTxMaxQuota(prGlueInfo->prAdapter),
					"UPDATE_WMM");
#endif
		/* transfer ioctl to OID request */
#ifdef UT_TEST_MODE
		testProcessOid(prGlueInfo->prAdapter);
#endif
		if (test_and_clear_bit(GLUE_FLAG_OID_BIT,
				       &prGlueInfo->ulFlag)) {
			kalTraceBegin("OID");
			/* get current prIoReq */
			prIoReq = &(prGlueInfo->OidEntry);
			DBGLOG(NIC, TRACE, "pfnOidHandler=%ps",
					prIoReq->pfnOidHandler);
			prIoReq->rStatus = wlanSetInformation(
					prIoReq->prAdapter,
					prIoReq->pfnOidHandler,
					prIoReq->pvInfoBuf,
					prIoReq->u4InfoBufLen,
					prIoReq->pu4QryInfoLen);

			if (prIoReq->rStatus != WLAN_STATUS_PENDING) {
				/* complete ONLY if there are waiters */
				if (isOidWaitingComplete(prGlueInfo)) {
					kalUpdateCompHdlrRec(
						prGlueInfo->prAdapter,
						prIoReq->pfnOidHandler,
						NULL);

					DBGLOG(NIC, TRACE,
						"rPendComp=%p pfnOidHandler=%ps",
						&prGlueInfo->rPendComp,
						prIoReq->pfnOidHandler);
					prGlueInfo->fgOidWaiting = FALSE;
					complete(&prGlueInfo->rPendComp);
				} else
					DBGLOG(INIT, WARN,
						"SKIP multiple OID complete!\n"
					       );
			} else {
				wlanoidTimeoutCheck(
						prGlueInfo->prAdapter,
						prIoReq->pfnOidHandler);
			}
			kalTraceEnd();
		}

		/*
		 *
		 * if TX request, clear the TXREQ flag. TXREQ set by
		 * kalSetEvent/GlueSetEvent
		 * indicates the following requests occur
		 *
		 */

#ifdef UT_TEST_MODE
		testProcessTxReq(prGlueInfo->prAdapter);
#endif

		if (test_and_clear_bit(GLUE_FLAG_TXREQ_BIT,
				       &prGlueInfo->ulFlag))
			TRACE(kalProcessTxReq(prGlueInfo, &fgNeedHwAccess),
				"TXREQ");

#if CFG_SUPPORT_MULTITHREAD
		/* Process RX */
#ifdef UT_TEST_MODE
		testProcessRFBs(prGlueInfo->prAdapter);
#endif
		if (test_and_clear_bit(GLUE_FLAG_TX_CMD_DONE_BIT,
				       &prGlueInfo->ulFlag))
			TRACE(wlanTxCmdDoneMthread(prGlueInfo->prAdapter),
				"TX_CMD");
#endif
		if (test_and_clear_bit(GLUE_FLAG_RX_BIT,
					   &prGlueInfo->ulFlag))
			TRACE(nicRxProcessRFBs(prGlueInfo->prAdapter), "RX");

		/* Process RX, In linux, we don't need to free sk_buff by
		 * ourself
		 */

		/* In linux, we don't need to free sk_buff by ourself */

		/* In linux, we don't do reset */
#if CFG_SUPPORT_MULTITHREAD
#else
		if (fgNeedHwAccess == TRUE)
			wlanReleasePowerControl(prGlueInfo->prAdapter);
#endif
		/* handle cnmTimer time out */
#ifdef UT_TEST_MODE
		testTimeoutCheck(prGlueInfo->prAdapter);
#endif

		/* update current throughput */
		kalPerMonUpdate(prGlueInfo->prAdapter);

		wlanDumpAllBssStatistics(prGlueInfo->prAdapter);
		/* check tput factor */
#if CFG_SUPPORT_TPUT_FACTOR
		kalTputFactorUpdate(prGlueInfo->prAdapter);
#endif

#if CFG_RFB_TRACK
		nicRxRfbTrackCheck(prGlueInfo->prAdapter);
#endif /* CFG_RFB_TRACK */

#if CFG_SUPPORT_SKIP_RX_GRO_FOR_TC
		kalRxGroTcCheck(prGlueInfo);
#endif /* CFG_SUPPORT_SKIP_RX_GRO_FOR_TC */

		if (test_and_clear_bit(GLUE_FLAG_TIMEOUT_BIT,
				       &prGlueInfo->ulFlag))
			TRACE(wlanTimerTimeoutCheck(prGlueInfo->prAdapter),
				"TIMEOUT");

#if CFG_SUPPORT_SDIO_READ_WRITE_PATTERN
		if (prGlueInfo->fgEnSdioTestPattern == TRUE)
			kalSetEvent(prGlueInfo);
#endif
#ifdef UT_TEST_MODE
		testThreadEnd(prGlueInfo->prAdapter);
#endif

		if (test_and_clear_bit(GLUE_FLAG_SER_TIMEOUT_BIT,
				       &prGlueInfo->ulFlag))
			GL_DEFAULT_RESET_TRIGGER(prGlueInfo->prAdapter,
						 RST_SER_TIMEOUT);

		if (test_and_clear_bit(GLUE_FLAG_TX_TIMEOUT_DUMP_BIT,
				&prGlueInfo->ulFlag))
			kalTxTimeoutDump(prGlueInfo->prAdapter);

		kalSyncTimeToFW(prGlueInfo->prAdapter, FALSE);

#ifdef CFG_MTK_CONNSYS_DEDICATED_LOG_PATH
		if (!prGlueInfo->prAdapter->fgSetLogOnOff) {
			kalMemZero(&rFwLogCmd, sizeof(rFwLogCmd));
			rFwLogCmd.fgCmd = FW_LOG_CMD_ON_OFF;
			rFwLogCmd.fgValue = getFWLogOnOff();

			connsysFwLogControl(prGlueInfo->prAdapter,
				(void *)&rFwLogCmd,
				sizeof(struct CMD_CONNSYS_FW_LOG),
				&u4BufLen);
		}
		if (!prGlueInfo->prAdapter->fgSetLogLevel) {
			wlanDbgGetGlobalLogLevel(
					ENUM_WIFI_LOG_MODULE_FW, &u4FwLevel);

			wlanDbgSetLogLevelImpl(prGlueInfo->prAdapter,
						ENUM_WIFI_LOG_LEVEL_VERSION_V1,
						ENUM_WIFI_LOG_MODULE_FW,
						u4FwLevel);
		}
#endif

#if (CFG_SUPPORT_POWER_THROTTLING == 1)
		if (test_and_clear_bit(GLUE_FLAG_CNS_PWR_LEVEL_BIT,
			&prGlueInfo->ulFlag)) {
			connsysPowerLevelNotify(prGlueInfo->prAdapter);
		}

		if (test_and_clear_bit(GLUE_FLAG_CNS_PWR_TEMP_BIT,
			&prGlueInfo->ulFlag)) {
			connsysPowerTempNotify(prGlueInfo->prAdapter);
		}

		kalDumpPwrLevel(prGlueInfo->prAdapter);
#endif

		kalTraceEnd(); /* main_thread */
	}

#if 0
	if (fgNeedHwAccess == TRUE)
		wlanReleasePowerControl(prGlueInfo->prAdapter);
#endif

	/* flush the pending TX packets */
	if (GLUE_GET_REF_CNT(prGlueInfo->i4TxPendingFrameNum) > 0)
		kalFlushPendingTxPackets(prGlueInfo);

	/* remove pending oid */
	wlanReleasePendingOid(prGlueInfo->prAdapter, 1);

	if (kalIsResetting() && isOidWaitingComplete(prGlueInfo)) {
		struct GL_IO_REQ *prIoReq;

		DBGLOG(INIT, INFO,
			"main_thread stop, complete pending oid\n");

		prIoReq = &(prGlueInfo->OidEntry);
		prIoReq->rStatus = WLAN_STATUS_FAILURE;

		prGlueInfo->fgOidWaiting = FALSE;
		complete(&prGlueInfo->rPendComp);
	}

	complete(&prGlueInfo->rHaltComp);
#if CFG_ENABLE_WAKE_LOCK
	if (KAL_WAKE_LOCK_ACTIVE(prGlueInfo->prAdapter,
				 prTxThreadWakeLock))
		KAL_WAKE_UNLOCK(prGlueInfo->prAdapter, prTxThreadWakeLock);
	KAL_WAKE_LOCK_DESTROY(prGlueInfo->prAdapter,
			      prTxThreadWakeLock);
#endif

	DBGLOG(INIT, TRACE, "%s:%u stopped!\n",
	       KAL_GET_CURRENT_THREAD_NAME(), KAL_GET_CURRENT_THREAD_ID());

#if CFG_CHIP_RESET_HANG
	while (fgIsResetHangState == SER_L0_HANG_RST_HANG) {
		kalMsleep(SER_L0_HANG_LOG_TIME_INTERVAL);
		DBGLOG(INIT, STATE, "[SER][L0] SQC hang!\n");
	}
#endif

	prGlueInfo->main_thread = NULL;
	prGlueInfo->u4TxThreadPid = 0xffffffff;

	return 0;

}

void kalTxTimeoutDump(struct ADAPTER *prAdapter)
{
	struct STA_RECORD *prStaRec;

	secPrivacyDumpWTBL(prAdapter);

	cnmDumpStaRec(prAdapter, prAdapter->ucTxTimeoutStaIdx);

	prStaRec = cnmGetStaRecByIndex(prAdapter, prAdapter->ucTxTimeoutStaIdx);

	if (prStaRec != NULL)
		bssDumpBssInfo(prAdapter, prStaRec->ucBssIndex);
}

void kalSetTxTimeoutDump(struct GLUE_INFO *pr)
{
	set_bit(GLUE_FLAG_TX_TIMEOUT_DUMP_BIT, &pr->ulFlag);
	wake_up_interruptible(&pr->waitq);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to check if card is removed
 *
 * \param pvGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval TRUE:     card is removed
 *         FALSE:    card is still attached
 */
/*----------------------------------------------------------------------------*/
u_int8_t kalIsCardRemoved(struct GLUE_INFO *prGlueInfo)
{
	ASSERT(prGlueInfo);

	return FALSE;
	/* Linux MMC doesn't have removal notification yet */
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to send command to firmware for overriding
 *        netweork address
 *
 * \param pvGlueInfo Pointer of GLUE Data Structure
 *
 * \retval TRUE
 *         FALSE
 */
/*----------------------------------------------------------------------------*/
u_int8_t kalRetrieveNetworkAddress(struct GLUE_INFO *prGlueInfo,
			uint8_t *prMacAddr)
{
	ASSERT(prGlueInfo);

	/* Get MAC address override from wlan feature option */
	prGlueInfo->fgIsMacAddrOverride =
		prGlueInfo->prAdapter->rWifiVar.ucMacAddrOverride;

	wlanHwAddrToBin(
		prGlueInfo->prAdapter->rWifiVar.aucMacAddrStr,
		prGlueInfo->rMacAddrOverride);

	if (prGlueInfo->fgIsMacAddrOverride == FALSE) {

#ifdef CFG_ENABLE_EFUSE_MAC_ADDR
		if (prGlueInfo->prAdapter->fgIsEmbbededMacAddrValid) {
			COPY_MAC_ADDR(prMacAddr,
			      prGlueInfo->prAdapter->rWifiVar.aucMacAddress);
			return TRUE;
		} else {
			return FALSE;
		}
#else
#if CFG_TC1_FEATURE
		/*LGE_FacReadWifiMacAddr(prMacAddr);*/
		TC1_FAC_NAME(FacReadWifiMacAddr)(prMacAddr);
		DBGLOG(INIT, INFO,
			"MAC address: " MACSTR, MAC2STR(prMacAddr));
#else
		if (prGlueInfo->fgNvramAvailable == FALSE) {
			DBGLOG(INIT, INFO, "glLoadNvram fail\n");
			return FALSE;
		}
		kalMemCopy(prMacAddr, prGlueInfo->rRegInfo.aucMacAddr,
			   PARAM_MAC_ADDR_LEN * sizeof(uint8_t));
#endif
		return TRUE;
#endif
	} else {
		COPY_MAC_ADDR(prMacAddr, prGlueInfo->rMacAddrOverride);

		return TRUE;
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to flush pending TX packets in glue layer
 *
 * \param pvGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
void kalFlushPendingTxPackets(struct GLUE_INFO *prGlueInfo)
{
	struct QUE *prTxQue;
	struct QUE_ENTRY *prQueueEntry;
	void *prPacket;

	ASSERT(prGlueInfo);

	prTxQue = &(prGlueInfo->rTxQueue);

	if (GLUE_GET_REF_CNT(prGlueInfo->i4TxPendingFrameNum) == 0)
		return;

	if (!HAL_IS_TX_DIRECT()) {
		GLUE_SPIN_LOCK_DECLARATION();

		while (TRUE) {
			GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_QUE);
			QUEUE_REMOVE_HEAD(prTxQue, prQueueEntry,
					  struct QUE_ENTRY *);
			GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_QUE);

			if (prQueueEntry == NULL)
				break;

			prPacket = GLUE_GET_PKT_DESCRIPTOR(prQueueEntry);

			kalSendComplete(prGlueInfo, prPacket,
					WLAN_STATUS_NOT_ACCEPTED);
		}
	}
}

/**
 * kalScheduleNapiTask() - schedule NAPI to do flush/dequeue
 *
 * Main thread do something in prAdapter and then calls this function
 * to schedule NAPI for flushing/dequeueing data.
 * If the configuration supports NAPI to schedule the polling, this function
 * returns WLAN_STATUS_SUCCESS; otherwise, it returns WLAN_STATUS_NOT_ACCEPTED
 * suggesting the caller to flush the data in main thread.
 *
 * Return: WLAN_STATUS_SUCCESS The task was scheduled.
 *         WLAN_STATUS_NOT_ACCEPTED The configuration does not support NAPI.
 */
uint32_t kalScheduleNapiTask(struct ADAPTER *prAdapter)
{
	uint32_t rc = WLAN_STATUS_NOT_ACCEPTED;

#if CFG_SUPPORT_RX_NAPI
	if (HAL_IS_RX_DIRECT(prAdapter)) {
		kalNapiSchedule(prAdapter);
#if CFG_SUPPORT_RX_WORK
		kalNapiWakeup();
#endif /* CFG_SUPPORT_RX_WORK */
		rc = WLAN_STATUS_SUCCESS;
	}
#endif /* CFG_SUPPORT_RX_NAPI */

	return rc;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is get indicated media state
 *
 * \param pvGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval
 */
/*----------------------------------------------------------------------------*/
enum ENUM_PARAM_MEDIA_STATE kalGetMediaStateIndicated(
					struct GLUE_INFO *prGlueInfo,
					uint8_t ucBssIndex)
{
	struct AIS_FSM_INFO *prAisFsmInfo;

	prAisFsmInfo = aisGetAisFsmInfo(prGlueInfo->prAdapter, ucBssIndex);
	if (!prAisFsmInfo) {
		DBGLOG(INIT, ERROR, "aisGetAisFsmInfo(%u)=NULL\n", ucBssIndex);
		return MEDIA_STATE_DISCONNECTED;
	}
	return prAisFsmInfo->eParamMediaStateIndicated;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to set indicated media state
 *
 * \param pvGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
void kalSetMediaStateIndicated(struct GLUE_INFO *prGlueInfo,
				enum ENUM_PARAM_MEDIA_STATE
				eParamMediaStateIndicate,
				uint8_t ucBssIndex)
{
	struct AIS_FSM_INFO *prAisFsmInfo;

	prAisFsmInfo = aisGetAisFsmInfo(prGlueInfo->prAdapter, ucBssIndex);
	if (!prAisFsmInfo) {
		DBGLOG(INIT, ERROR, "aisGetAisFsmInfo(%u)=NULL\n", ucBssIndex);
		return;
	}
	prAisFsmInfo->eParamMediaStateIndicated = eParamMediaStateIndicate;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to clear pending OID staying in command queue
 *
 * \param prGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
void kalOidCmdClearance(struct GLUE_INFO *prGlueInfo)
{
	struct QUE *prCmdQue;
	struct QUE rTempCmdQue;
	struct QUE *prTempCmdQue = &rTempCmdQue;
	struct QUE rReturnCmdQue;
	struct QUE *prReturnCmdQue = &rReturnCmdQue;
	struct QUE_ENTRY *prQueueEntry = (struct QUE_ENTRY *) NULL;
	struct CMD_INFO *prCmdInfo = (struct CMD_INFO *) NULL;

	GLUE_SPIN_LOCK_DECLARATION();

	ASSERT(prGlueInfo);

	QUEUE_INITIALIZE(prReturnCmdQue);

	prCmdQue = &prGlueInfo->rCmdQueue;

	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
	QUEUE_MOVE_ALL(prTempCmdQue, prCmdQue);
	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);

	QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry,
			  struct QUE_ENTRY *);
	while (prQueueEntry) {

		if (((struct CMD_INFO *) prQueueEntry)->fgIsOid) {
			prCmdInfo = (struct CMD_INFO *) prQueueEntry;
			break;
		}
		QUEUE_INSERT_TAIL(prReturnCmdQue, prQueueEntry);
		QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry,
				struct QUE_ENTRY *);
	}

	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
	QUEUE_CONCATENATE_QUEUES_HEAD(prCmdQue, prReturnCmdQue);
	QUEUE_CONCATENATE_QUEUES_HEAD(prCmdQue, prTempCmdQue);
	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);

	if (prCmdInfo) {
		DBGLOG(OID, INFO, "Clear pending OID CMD ID[0x%02X] SEQ[%u]\n",
				prCmdInfo->ucCID, prCmdInfo->ucCmdSeqNum);
		if (prCmdInfo->pfCmdTimeoutHandler)
			prCmdInfo->pfCmdTimeoutHandler(prGlueInfo->prAdapter,
						       prCmdInfo);
		else
			kalOidComplete(prGlueInfo, prCmdInfo, 0,
				       WLAN_STATUS_NOT_ACCEPTED);

		prGlueInfo->fgOidWaiting = FALSE;
		cmdBufFreeCmdInfo(prGlueInfo->prAdapter, prCmdInfo);
		GLUE_DEC_REF_CNT(prGlueInfo->i4TxPendingCmdNum);
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to insert command into prCmdQueue
 *
 * \param prGlueInfo     Pointer of GLUE Data Structure
 *        prQueueEntry   Pointer of queue entry to be inserted
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
void kalEnqueueCommand(struct GLUE_INFO *prGlueInfo,
		       struct QUE_ENTRY *prQueueEntry)
{
	struct QUE *prCmdQue;
	struct CMD_INFO *prCmdInfo;
#if CFG_DBG_MGT_BUF
	struct MEM_TRACK *prMemTrack = NULL;
#endif

	GLUE_SPIN_LOCK_DECLARATION();

	ASSERT(prGlueInfo);
	ASSERT(prQueueEntry);

	prCmdQue = &prGlueInfo->rCmdQueue;

	prCmdInfo = (struct CMD_INFO *) prQueueEntry;

#if CFG_DBG_MGT_BUF
	if (prCmdInfo->pucInfoBuffer &&
	    !IS_FROM_BUF(prGlueInfo->prAdapter, prCmdInfo->pucInfoBuffer)) {
		prMemTrack =
			CONTAINER_OF((uint8_t (*)[])prCmdInfo->pucInfoBuffer,
				     struct MEM_TRACK, aucData);

		prMemTrack->ucCmdId = prCmdInfo->ucCID;
		/* 0x10 means the CmdId enqueue to rCmdQueue and is waiting for
		 * main_thread handling
		 */
		prMemTrack->ucWhere = 0x10;

	}
#endif

	DBGLOG_LIMITED(INIT, TRACE,
	       "EN-Q CMD TYPE[%u] ID[0x%02X] SEQ[%u] to CMD Q\n",
	       prCmdInfo->eCmdType, prCmdInfo->ucCID,
	       prCmdInfo->ucCmdSeqNum);

	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
	QUEUE_INSERT_TAIL(prCmdQue, prQueueEntry);
	GLUE_INC_REF_CNT(prGlueInfo->i4TxPendingCmdNum);
	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Handle EVENT_ID_ASSOC_INFO event packet by indicating to OS with
 *        proper information
 *
 * @param pvGlueInfo     Pointer of GLUE Data Structure
 * @param prAssocInfo    Pointer of EVENT_ID_ASSOC_INFO Packet
 *
 * @return none
 */
/*----------------------------------------------------------------------------*/
void kalHandleAssocInfo(struct GLUE_INFO *prGlueInfo,
			struct EVENT_ASSOC_INFO *prAssocInfo)
{
	/* to do */
}

uint32_t kalGetTxPendingFrameCount(struct GLUE_INFO
				   *prGlueInfo)
{
	ASSERT(prGlueInfo);

	return (uint32_t) (GLUE_GET_REF_CNT(
				   prGlueInfo->i4TxPendingFrameNum));
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to retrieve the number of pending commands
 *        (including MMPDU, 802.1X and command packets)
 *
 * \param prGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval
 */
/*----------------------------------------------------------------------------*/
uint32_t kalGetTxPendingCmdCount(struct GLUE_INFO
				 *prGlueInfo)
{
	ASSERT(prGlueInfo);

	return (uint32_t)GLUE_GET_REF_CNT(
		       prGlueInfo->i4TxPendingCmdNum);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Timer Initialization Procedure
 *
 * \param[in] prGlueInfo     Pointer to GLUE Data Structure
 * \param[in] prTimerHandler Pointer to timer handling function, whose only
 *                           argument is "prAdapter"
 *
 * \retval none
 *
 */
/*----------------------------------------------------------------------------*/

/* static struct timer_list tickfn; */

void kalOsTimerInitialize(struct GLUE_INFO *prGlueInfo,
			  void *prTimerHandler)
{

	ASSERT(prGlueInfo);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))
	timer_setup(&(prGlueInfo->tickfn), prTimerHandler, 0);
#else
	init_timer(&(prGlueInfo->tickfn));
	prGlueInfo->tickfn.function = prTimerHandler;
	prGlueInfo->tickfn.data = (unsigned long)prGlueInfo;
#endif
}

/* Todo */
/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set the time to do the time out check.
 *
 * \param[in] prGlueInfo Pointer to GLUE Data Structure
 * \param[in] rInterval  Time out interval from current time.
 *
 * \retval TRUE Success.
 */
/*----------------------------------------------------------------------------*/
u_int8_t kalSetTimer(struct GLUE_INFO *prGlueInfo,
		     uint32_t u4Interval)
{
	ASSERT(prGlueInfo);

	if (HAL_IS_RX_DIRECT(prGlueInfo->prAdapter)) {
		mod_timer(&prGlueInfo->tickfn,
			  jiffies + u4Interval * HZ / MSEC_PER_SEC);
	} else {
		del_timer_sync(&(prGlueInfo->tickfn));

		prGlueInfo->tickfn.expires = jiffies + u4Interval * HZ /
					     MSEC_PER_SEC;
		add_timer(&(prGlueInfo->tickfn));
	}

	return TRUE;		/* success */
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to cancel
 *
 * \param[in] prGlueInfo Pointer to GLUE Data Structure
 *
 * \retval TRUE  :   Timer has been canceled
 *         FALAE :   Timer doens't exist
 */
/*----------------------------------------------------------------------------*/
u_int8_t kalCancelTimer(struct GLUE_INFO *prGlueInfo)
{
	ASSERT(prGlueInfo);

	clear_bit(GLUE_FLAG_TIMEOUT_BIT, &prGlueInfo->ulFlag);

	if (del_timer_sync(&(prGlueInfo->tickfn)) >= 0)
		return TRUE;
	else
		return FALSE;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is a callback function for scanning done
 *
 * \param[in] prGlueInfo Pointer to GLUE Data Structure
 *
 * \retval none
 *
 */
/*----------------------------------------------------------------------------*/
void kalScanDone(struct GLUE_INFO *prGlueInfo,
		 uint8_t ucBssIndex,
		 uint32_t status)
{
	uint8_t fgAborted = (status != WLAN_STATUS_SUCCESS) ? TRUE : FALSE;
	ASSERT(prGlueInfo);

	if (IS_BSS_INDEX_AIS(prGlueInfo->prAdapter, ucBssIndex))
		scanLogEssResult(prGlueInfo->prAdapter);

	scanReportBss2Cfg80211(prGlueInfo->prAdapter,
			       BSS_TYPE_INFRASTRUCTURE, NULL);

	/* check for system configuration for generating error message on scan
	 * list
	 */
	wlanCheckSystemConfiguration(prGlueInfo->prAdapter);

	kalIndicateStatusAndComplete(prGlueInfo, WLAN_STATUS_SCAN_COMPLETE,
		&fgAborted, sizeof(fgAborted), ucBssIndex);
}

#if CFG_SUPPORT_SCAN_CACHE_RESULT
/*----------------------------------------------------------------------------*/
/*!
 * @brief update timestamp information of bss cache in kernel
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 * @return   status 0 if success, error code otherwise
 */
/*----------------------------------------------------------------------------*/
uint8_t kalUpdateBssTimestamp(struct GLUE_INFO *prGlueInfo)
{
	struct wiphy *wiphy;
	struct cfg80211_registered_device *rdev;
	struct cfg80211_internal_bss *bss = NULL;
	struct cfg80211_bss_ies *ies;
	uint64_t new_timestamp = kalGetBootTime();

	ASSERT(prGlueInfo);
	wiphy = wlanGetWiphy();
	if (!wiphy) {
		log_dbg(REQ, ERROR, "wiphy is null\n");
		return 1;
	}
	rdev = CONTAINER_OF(wiphy, struct cfg80211_registered_device, wiphy);

	log_dbg(REQ, INFO, "Update scan timestamp: %llu (%llu)\n",
		new_timestamp, le64_to_cpu(new_timestamp));

	/* add 1 ms to prevent scan time too short */
	new_timestamp += 1000;

	spin_lock_bh(&rdev->bss_lock);
	list_for_each_entry(bss, &rdev->bss_list, list) {
		ies = *(struct cfg80211_bss_ies **)
		(((size_t)&(bss->pub)) + offsetof(struct cfg80211_bss, ies));
		if (rcu_access_pointer(bss->pub.ies) == ies) {
			ies->tsf = le64_to_cpu(new_timestamp);
		} else {
			log_limited_dbg(REQ, WARN, "Update tsf fail. bss->pub.ies=%p ies=%p\n",
				bss->pub.ies, ies);
		}
	}
	spin_unlock_bh(&rdev->bss_lock);

	return 0;
}
#endif /* CFG_SUPPORT_SCAN_CACHE_RESULT */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to generate a random number
 *
 * \param none
 *
 * \retval UINT_32
 */
/*----------------------------------------------------------------------------*/
uint32_t kalRandomNumber(void)
{
	uint32_t number = 0;

	get_random_bytes(&number, 4);

	return number;
}

void kalRandomGetBytes(void *buf, uint32_t len)
{
	get_random_bytes(buf, len);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief command timeout call-back function
 *
 * \param[in] prGlueInfo Pointer to the GLUE data structure.
 *
 * \retval (none)
 */
/*----------------------------------------------------------------------------*/
#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
void kalTimeoutHandler(struct timer_list *timer)
#else
void kalTimeoutHandler(unsigned long arg)
#endif
{
#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
	struct GLUE_INFO *prGlueInfo =
		from_timer(prGlueInfo, timer, tickfn);
#else
	struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *)arg;
#endif

	ASSERT(prGlueInfo);

	/* Notify tx thread  for timeout event */
	set_bit(GLUE_FLAG_TIMEOUT_BIT, &prGlueInfo->ulFlag);
	wake_up_interruptible(&prGlueInfo->waitq);

}

void kalSetEvent(struct GLUE_INFO *pr)
{
	set_bit(GLUE_FLAG_TXREQ_BIT, &pr->ulFlag);
	wake_up_interruptible(&pr->waitq);
}

void kalSetSerTimeoutEvent(struct GLUE_INFO *pr)
{
	set_bit(GLUE_FLAG_SER_TIMEOUT_BIT, &pr->ulFlag);
	wake_up_interruptible(&pr->waitq);
}

void kalRxTaskletSchedule(struct GLUE_INFO *pr)
{
	tasklet_hi_schedule(&pr->rRxTask);
}

void kalRxTaskSchedule(struct GLUE_INFO *pr)
{
	uint32_t u4Cnt;

	if (!HAL_IS_RX_DIRECT(pr->prAdapter)) {
		DBGLOG(INIT, ERROR,
		       "Valid in RX-direct mode only\n");
		return;
	}

	/* do nothing if wifi is not ready */
	if (pr->fgRxTaskReady == FALSE)
		return;

#if CFG_SUPPORT_CPU_STAT
	CPU_STAT_INC_CNT(pr, CPU_RX_IN);
#endif /* CFG_SUPPORT_CPU_STAT */

	/* prevent multiple tasklet schedule in ISR */
	u4Cnt = GLUE_INC_REF_CNT(pr->u4RxTaskScheduleCnt);
	if (u4Cnt > 2) {
		/* more than 2 times schedule, no need to add */
		GLUE_DEC_REF_CNT(pr->u4RxTaskScheduleCnt);
		return;
	} else if (u4Cnt > 1) {
		/* just skip it, rx tasklet will reschedule itself */
		return;
	}

#if CFG_SUPPORT_RX_WORK
	kalRxWorkSchedule(pr);
#else /* CFG_SUPPORT_RX_WORK */
	kalRxTaskletSchedule(pr);
#endif /* CFG_SUPPORT_RX_WORK */
}

uint32_t kalRxTaskWorkDone(struct GLUE_INFO *pr, u_int8_t fgIsInt)
{
	if (!HAL_IS_RX_DIRECT(pr->prAdapter)) {
		DBGLOG(INIT, ERROR,
		       "Valid in RX-direct mode only\n");
		return WLAN_STATUS_NOT_SUPPORTED;
	}

	/* fix abnormal logic if u4RxTaskScheduleCnt is zero */
	if (GLUE_GET_REF_CNT(pr->u4RxTaskScheduleCnt) > 1) {
		/* more than 1 time schedule, do one more schedule */
		GLUE_DEC_REF_CNT(pr->u4RxTaskScheduleCnt);
		/* reschedule RxTasklet due to pending INT */
#if CFG_SUPPORT_RX_WORK
		kalRxWorkSchedule(pr);
#else /* CFG_SUPPORT_RX_WORK */
		kalRxTaskletSchedule(pr);
#endif /* CFG_SUPPORT_RX_WORK */
	} else {
		if (GLUE_GET_REF_CNT(pr->u4RxTaskScheduleCnt) == 1)
			GLUE_DEC_REF_CNT(pr->u4RxTaskScheduleCnt);

		/* no more schedule, so enable interrupt */
		if (fgIsInt) {
#if CFG_SUPPORT_RX_NAPI
#if CFG_SUPPORT_RX_WORK
			kalNapiWakeup();
#endif /* CFG_SUPPORT_RX_WORK */
#endif /* CFG_SUPPORT_RX_NAPI */
			nicEnableInterrupt(pr->prAdapter);
			return WLAN_STATUS_SUCCESS;
		}
	}

	return WLAN_STATUS_PENDING;
}

void kalSetIntEvent(struct GLUE_INFO *pr)
{
	KAL_WAKE_LOCK(pr->prAdapter, pr->rIntrWakeLock);

	/* Do not wakeup hif_thread in direct mode */
	if (HAL_IS_RX_DIRECT(pr->prAdapter))
		set_bit(GLUE_FLAG_RX_DIRECT_INT_BIT, &pr->ulFlag);
	else
		set_bit(GLUE_FLAG_INT_BIT, &pr->ulFlag);

	RX_INC_CNT(&pr->prAdapter->rRxCtrl, RX_INTR_COUNT);

#if CFG_SUPPORT_HIF_RX_NAPI
	set_bit(HIF_NAPI_SET_DRV_OWN_BIT, &pr->rHifInfo.rNapiDev.ulFlag);
#endif

	/* when we got interrupt, we wake up service thread */
#if CFG_SUPPORT_MULTITHREAD
	if (HAL_IS_RX_DIRECT(pr->prAdapter))
		kalRxTaskSchedule(pr);
	else
		wake_up_interruptible(&pr->waitq_hif);
#else
	wake_up_interruptible(&pr->waitq);
#endif
}

void kalSetDrvIntEvent(struct GLUE_INFO *pr)
{
	KAL_WAKE_LOCK(pr->prAdapter, pr->rIntrWakeLock);

	if (!HAL_IS_RX_DIRECT(pr->prAdapter))
		set_bit(GLUE_FLAG_DRV_INT_BIT, &pr->ulFlag);

#if CFG_SUPPORT_HIF_RX_NAPI
	/* set int bit to enable interrupt */
	set_bit(GLUE_FLAG_RX_DIRECT_INT_BIT, &pr->ulFlag);
	set_bit(HIF_NAPI_SCHE_NAPI_BIT, &pr->rHifInfo.rNapiDev.ulFlag);
#endif

	/* when we got interrupt, we wake up servie thread */
#if CFG_SUPPORT_MULTITHREAD
	if (HAL_IS_RX_DIRECT(pr->prAdapter))
		kalRxTaskSchedule(pr);
	else
		wake_up_interruptible(&pr->waitq_hif);
#else
	wake_up_interruptible(&pr->waitq);
#endif
}

void kalSetWmmUpdateEvent(struct GLUE_INFO *pr)
{
	set_bit(GLUE_FLAG_UPDATE_WMM_QUOTA_BIT, &pr->ulFlag);
#if CFG_SUPPORT_MULTITHREAD
	wake_up_interruptible(&pr->waitq_hif);
#endif
}

void kalSetMddpEvent(struct GLUE_INFO *pr)
{
	set_bit(GLUE_FLAG_HIF_MDDP_BIT, &pr->ulFlag);
#if CFG_SUPPORT_MULTITHREAD
	wake_up_interruptible(&pr->waitq_hif);
#endif
}

void kalSetHifAerResetEvent(struct GLUE_INFO *pr)
{
	set_bit(HIF_FLAG_AER_RESET_BIT, &pr->ulHifFlag);
#if CFG_SUPPORT_MULTITHREAD
	wake_up_interruptible(&pr->waitq_hif);
#endif
}

void kalSetHifMsiRecoveryEvent(struct GLUE_INFO *pr)
{
	set_bit(HIF_FLAG_MSI_RECOVERY_BIT, &pr->ulHifFlag);
#if CFG_SUPPORT_MULTITHREAD
	wake_up_interruptible(&pr->waitq_hif);
#endif
}

void kalSetHifHandleAllTokensUnusedEvent(struct GLUE_INFO *pr)
{
	set_bit(HIF_FLAG_ALL_TOKENS_UNUSED_BIT, &pr->ulHifFlag);
#if CFG_SUPPORT_MULTITHREAD
	wake_up_interruptible(&pr->waitq_hif);
#endif
}

void kalSetHifDbgEvent(struct GLUE_INFO *pr)
{
	set_bit(GLUE_FLAG_HIF_PRT_HIF_DBG_INFO_BIT, &(pr->ulFlag));
#if CFG_SUPPORT_MULTITHREAD
	wake_up_interruptible(&pr->waitq_hif);
#endif
}

#if CFG_SUPPORT_MULTITHREAD
void kalSetSerIntEvent(struct GLUE_INFO *pr)
{
	KAL_WAKE_LOCK(pr->prAdapter, pr->rIntrWakeLock);

	set_bit(GLUE_FLAG_SER_INT_BIT, &pr->ulFlag);

	wake_up_interruptible(&pr->waitq_hif);
}

#if (CFG_TX_MGMT_BY_DATA_Q == 1)
void kalSetMgmtDirectTxEvent2Hif(struct GLUE_INFO *pr)
{
	uint32_t u4ThreadWakeUp = 0;

	if (!pr->hif_thread)
		return;

	u4ThreadWakeUp = wlanGetThreadWakeUp(pr->prAdapter);

	KAL_WAKE_LOCK_TIMEOUT(pr->prAdapter, pr->rTimeoutWakeLock,
			      MSEC_TO_JIFFIES(u4ThreadWakeUp));

	set_bit(GLUE_FLAG_MGMT_DIRECT_HIF_TX_BIT, &pr->ulFlag);

	wake_up_interruptible(&pr->waitq_hif);
}
#endif /* CFG_TX_MGMT_BY_DATA_Q == 1 */

void kalSetTxEvent2Hif(struct GLUE_INFO *pr)
{
	if (!pr->hif_thread)
		return;

	KAL_WAKE_LOCK_TIMEOUT(pr->prAdapter, pr->rTimeoutWakeLock,
			      MSEC_TO_JIFFIES(
			      pr->prAdapter->rWifiVar.u4WakeLockThreadWakeup));

	set_bit(GLUE_FLAG_HIF_TX_BIT, &pr->ulFlag);

	wake_up_interruptible(&pr->waitq_hif);
}

void kalSetFwOwnEvent2Hif(struct GLUE_INFO *pr)
{
	if (!pr->hif_thread)
		return;

	KAL_WAKE_LOCK_TIMEOUT(pr->prAdapter, pr->rTimeoutWakeLock,
			      MSEC_TO_JIFFIES(
			      pr->prAdapter->rWifiVar.u4WakeLockThreadWakeup));

	set_bit(GLUE_FLAG_HIF_FW_OWN_BIT, &pr->ulFlag);
	wake_up_interruptible(&pr->waitq_hif);
}

void kalSetTxEvent2Rx(struct GLUE_INFO *pr)
{
	if (!pr->rx_thread)
		return;

	KAL_WAKE_LOCK_TIMEOUT(pr->prAdapter, pr->rTimeoutWakeLock,
			      MSEC_TO_JIFFIES(
			      pr->prAdapter->rWifiVar.u4WakeLockThreadWakeup));

	set_bit(GLUE_FLAG_RX_TO_OS_BIT, &pr->ulFlag);
	wake_up_interruptible(&pr->waitq_rx);
}

void kalSetTxCmdEvent2Hif(struct GLUE_INFO *pr)
{
	if (!pr->hif_thread)
		return;

	KAL_WAKE_LOCK_TIMEOUT(pr->prAdapter, pr->rTimeoutWakeLock,
			      MSEC_TO_JIFFIES(
			      pr->prAdapter->rWifiVar.u4WakeLockThreadWakeup));

	set_bit(GLUE_FLAG_HIF_TX_CMD_BIT, &pr->ulFlag);
	wake_up_interruptible(&pr->waitq_hif);
}

void kalSetTxCmdDoneEvent(struct GLUE_INFO *pr)
{
	/* do we need wake lock here */
	set_bit(GLUE_FLAG_TX_CMD_DONE_BIT, &pr->ulFlag);
	wake_up_interruptible(&pr->waitq);
}

void kalSetRxProcessEvent(struct GLUE_INFO *pr)
{
	/* Do not wakeup target if there is nothing waiting */
	if (QUEUE_IS_EMPTY(&pr->prAdapter->rRxCtrl.rReceivedRfbList))
		return;

	/* do we need wake lock here ? */
	set_bit(GLUE_FLAG_RX_BIT, &pr->ulFlag);
	wake_up_interruptible(&pr->waitq);
}
#endif
/*----------------------------------------------------------------------------*/
/*!
 * \brief to check if configuration file (NVRAM/Registry) exists
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *           TRUE
 *           FALSE
 */
/*----------------------------------------------------------------------------*/
u_int8_t kalIsConfigurationExist(struct GLUE_INFO
				 *prGlueInfo)
{
#if !defined(CONFIG_X86)
	ASSERT(prGlueInfo);

	return prGlueInfo->fgNvramAvailable;
#else
	/* there is no configuration data for x86-linux */
	/*return FALSE;*/


	/*Modify for Linux PC support NVRAM Setting*/
	return prGlueInfo->fgNvramAvailable;
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief to retrieve Registry information
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *           Pointer of REG_INFO_T
 */
/*----------------------------------------------------------------------------*/
struct REG_INFO *kalGetConfiguration(struct GLUE_INFO
				     *prGlueInfo)
{
	ASSERT(prGlueInfo);

	return &(prGlueInfo->rRegInfo);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief update RSSI and LinkQuality to GLUE layer
 *
 * \param[in]
 *           prGlueInfo
 *           eNetTypeIdx
 *           cRssi
 *           cLinkQuality
 *
 * \return
 *           None
 */
/*----------------------------------------------------------------------------*/
void
kalUpdateRSSI(struct GLUE_INFO *prGlueInfo,
	      uint8_t ucBssIndex,
	      int8_t cRssi, int8_t cLinkQuality)
{
	struct iw_statistics *pStats = (struct iw_statistics *)NULL;
	struct ADAPTER *prAdapter = NULL;
	struct BSS_INFO *prBssInfo = (struct BSS_INFO *) NULL;

	ASSERT(prGlueInfo);
	prAdapter = prGlueInfo->prAdapter;
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo || unlikely(ucBssIndex >= MAX_BSSID_NUM)) {
		DBGLOG(AIS, WARN, "bss is In-valid\n");
		return;
	}

	if (IS_BSS_AIS(prBssInfo))
		pStats = (struct iw_statistics *)
			(&(prGlueInfo->rIwStats[ucBssIndex]));
#if CFG_ENABLE_WIFI_DIRECT
#if CFG_SUPPORT_P2P_RSSI_QUERY
	else if (IS_BSS_P2P(prBssInfo))
		pStats = (struct iw_statistics *)
			(&(prGlueInfo->rP2pIwStats));
#endif
#endif
	if (pStats) {
		pStats->qual.qual = cLinkQuality;
		pStats->qual.noise = 0;
		pStats->qual.updated = IW_QUAL_QUAL_UPDATED |
				       IW_QUAL_NOISE_UPDATED;
		pStats->qual.level = 0x100 + cRssi;
		pStats->qual.updated |= IW_QUAL_LEVEL_UPDATED;
	}

}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Pre-allocate I/O buffer
 *
 * \param[in]
 *           none
 *
 * \return
 *           TRUE
 *           FALSE
 */
/*----------------------------------------------------------------------------*/
u_int8_t kalInitIOBuffer(u_int8_t is_pre_alloc)
{
	uint32_t u4Size;

	/* not pre-allocation for all memory usage */
	if (!is_pre_alloc) {
		pvIoBuffer = NULL;
		return FALSE;
	}

	/* pre-allocation for all memory usage */
	if (HIF_TX_COALESCING_BUFFER_SIZE >
	    HIF_RX_COALESCING_BUFFER_SIZE)
		u4Size = HIF_TX_COALESCING_BUFFER_SIZE;
	else
		u4Size = HIF_RX_COALESCING_BUFFER_SIZE;

	u4Size += HIF_EXTRA_IO_BUFFER_SIZE;

	pvIoBuffer = kmalloc(u4Size, GFP_KERNEL);
	if (pvIoBuffer) {
		pvIoBufferSize = u4Size;
		pvIoBufferUsage = 0;

		return TRUE;
	}

	return FALSE;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Free pre-allocated I/O buffer
 *
 * \param[in]
 *           none
 *
 * \return
 *           none
 */
/*----------------------------------------------------------------------------*/
void kalUninitIOBuffer(void)
{
	kfree(pvIoBuffer);

	pvIoBuffer = (void *) NULL;
	pvIoBufferSize = 0;
	pvIoBufferUsage = 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Dispatch pre-allocated I/O buffer
 *
 * \param[in]
 *           u4AllocSize
 *
 * \return
 *           PVOID for pointer of pre-allocated I/O buffer
 */
/*----------------------------------------------------------------------------*/
void *kalAllocateIOBuffer(uint32_t u4AllocSize)
{
	void *ret = (void *) NULL;

	if (pvIoBuffer) {
		if (u4AllocSize <= (pvIoBufferSize - pvIoBufferUsage)) {
			ret = (void *)
				&(((uint8_t *) (pvIoBuffer))[pvIoBufferUsage]);
			pvIoBufferUsage += u4AllocSize;
		}
	} else {
		/* fault tolerance */
		ret = (void *) kalMemAlloc(u4AllocSize, PHY_MEM_TYPE);
	}

	return ret;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Release all dispatched I/O buffer
 *
 * \param[in]
 *           none
 *
 * \return
 *           none
 */
/*----------------------------------------------------------------------------*/
void kalReleaseIOBuffer(void *pvAddr, uint32_t u4Size)
{
	if (pvIoBuffer) {
		pvIoBufferUsage -= u4Size;
	} else {
		/* fault tolerance */
		kalMemFree(pvAddr, PHY_MEM_TYPE, u4Size);
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in] prAdapter  Pointer of ADAPTER_T
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void
kalGetChannelList(struct GLUE_INFO *prGlueInfo,
		  enum ENUM_BAND eSpecificBand,
		  uint8_t ucMaxChannelNum, uint8_t *pucNumOfChannel,
		  struct RF_CHANNEL_INFO *paucChannelList)
{
	rlmDomainGetChnlList(prGlueInfo->prAdapter, eSpecificBand,
			     FALSE,
			     ucMaxChannelNum, pucNumOfChannel, paucChannelList);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in] prAdapter  Pointer of ADAPTER_T
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
u_int8_t kalIsAPmode(struct GLUE_INFO *prGlueInfo)
{
#if 0				/* Marked for MT6630 (New ucBssIndex) */
#if CFG_ENABLE_WIFI_DIRECT
	if (IS_NET_ACTIVE(prGlueInfo->prAdapter,
			  NETWORK_TYPE_P2P_INDEX) &&
	    p2pFuncIsAPMode(
		    prGlueInfo->prAdapter->rWifiVar.prP2pFsmInfo))
		return TRUE;
#endif
#endif

	return FALSE;
}

#if CFG_SUPPORT_802_11W
/*----------------------------------------------------------------------------*/
/*!
 * \brief to check if the MFP is DISABLD/OPTIONAL/REQUIRED
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *	 RSN_AUTH_MFP_DISABLED
 *	 RSN_AUTH_MFP_OPTIONAL
 *	 RSN_AUTH_MFP_DISABLED
 */
/*----------------------------------------------------------------------------*/
uint32_t kalGetMfpSetting(struct GLUE_INFO *prGlueInfo,
	uint8_t ucBssIndex)
{
	struct GL_WPA_INFO *prWpaInfo;

	ASSERT(prGlueInfo);

	prWpaInfo = aisGetWpaInfo(prGlueInfo->prAdapter,
		ucBssIndex);

	return prWpaInfo->u4Mfp;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief to check if the RSN IE CAP setting from supplicant
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *           TRUE
 *           FALSE
 */
/*----------------------------------------------------------------------------*/
uint8_t kalGetRsnIeMfpCap(struct GLUE_INFO *prGlueInfo,
	uint8_t ucBssIndex)
{
	struct GL_WPA_INFO *prWpaInfo;

	ASSERT(prGlueInfo);

	prWpaInfo = aisGetWpaInfo(prGlueInfo->prAdapter,
		ucBssIndex);

	return prWpaInfo->ucRSNMfpCap;
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief read request firmware file binary to pucData
 *
 * \param[in] pucPath  file name
 * \param[out] pucData  Request file output buffer
 * \param[in] u4Size  read size
 * \param[out] pu4ReadSize  real read size
 * \param[in] dev
 *
 * \return
 *           0: success
 *           not 0: fail
 */
/*----------------------------------------------------------------------------*/
int32_t kalRequestFirmware(const uint8_t *pucPath,
		uint8_t **ppucData, uint32_t *pu4ReadSize,
		uint8_t ucIsZeroPadding, struct device *dev)
{
	const struct firmware *fw;
	uint8_t *pucData = NULL;
	uint32_t u4Size;
	int ret = 0;

	/*
	 * Driver support request_firmware() to get files
	 * Android path: "/etc/firmware", "/vendor/firmware", "/firmware/image"
	 * Linux path: "/lib/firmware", "/lib/firmware/update"
	 */
	ret = _kalRequestFirmware(&fw, pucPath, dev);

	if (ret != 0) {
		DBGLOG(INIT, TRACE, "kalRequestFirmware %s Fail, errno[%d]!!\n",
		       pucPath, ret);
		*ppucData = NULL;
		*pu4ReadSize = 0;
		return ret;
	}

	DBGLOG(INIT, INFO, "kalRequestFirmware(): %s OK\n",
	       pucPath);

	if (ucIsZeroPadding)
		u4Size = (uint32_t) fw->size + 1;
	else
		u4Size = (uint32_t) fw->size;

	pucData = kalMemAlloc(u4Size, VIR_MEM_TYPE);
	if (pucData == NULL) {
		*ppucData = NULL;
		*pu4ReadSize = 0;
		release_firmware(fw);
		return -1;
	}
	kalMemCopy(pucData, fw->data, fw->size);
	if (ucIsZeroPadding)
		pucData[fw->size] = 0;
	*ppucData = pucData;
	*pu4ReadSize = u4Size;

	release_firmware(fw);

	return ret;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief    To indicate BSS-INFO to NL80211 as scanning result
 *
 * \param[in]
 *           prGlueInfo
 *           pucBeaconProbeResp
 *           u4FrameLen
 *
 *
 *
 * \return
 *           none
 */
/*----------------------------------------------------------------------------*/
void
kalIndicateBssInfo(struct GLUE_INFO *prGlueInfo,
		   uint8_t *pucBeaconProbeResp,
		   uint32_t u4FrameLen, uint8_t ucChannelNum,
		   enum ENUM_BAND eBand,
		   int32_t i4SignalStrength)
{
	struct wiphy *wiphy;
	struct ieee80211_channel *prChannel = NULL;

	ASSERT(prGlueInfo);
	wiphy = wlanGetWiphy();

	/* search through channel entries */
#if (CFG_SUPPORT_WIFI_6G == 1)
	if (eBand == BAND_6G) {
		prChannel =
			ieee80211_get_channel(
				wlanGetWiphy(),
				ieee80211_channel_to_frequency
				(ucChannelNum, KAL_BAND_6GHZ));
	} else
#endif
	if (ucChannelNum <= 14) {
		prChannel = ieee80211_get_channel(wiphy,
				ieee80211_channel_to_frequency(ucChannelNum,
								KAL_BAND_2GHZ));
	} else {
		prChannel = ieee80211_get_channel(wiphy,
				ieee80211_channel_to_frequency(ucChannelNum,
								KAL_BAND_5GHZ));
	}

	if (prChannel != NULL
	    && prGlueInfo->fgIsRegistered == TRUE) {
		struct cfg80211_bss *bss;
		struct ieee80211_mgmt *prMgmtFrame = (struct ieee80211_mgmt
						      *)pucBeaconProbeResp;
		char *pucBssSubType =
			ieee80211_is_beacon(prMgmtFrame->frame_control) ?
			"beacon" : "probe_resp";

#if CFG_SUPPORT_TSF_USING_BOOTTIME
		prMgmtFrame->u.beacon.timestamp = kalGetBootTime();
#endif

		kalScanResultLog(prGlueInfo->prAdapter,
			(struct ieee80211_mgmt *)pucBeaconProbeResp);

		log_dbg(SCN, TRACE, "cfg80211_inform_bss_frame %s bss=" MACSTR
			" sn=%u ch=%u rssi=%d len=%u tsf=%llu\n", pucBssSubType,
			MAC2STR(prMgmtFrame->bssid), prMgmtFrame->seq_ctrl,
			ucChannelNum, i4SignalStrength, u4FrameLen,
			prMgmtFrame->u.beacon.timestamp);

		/* indicate to NL80211 subsystem */
		bss = cfg80211_inform_bss_frame(wiphy, prChannel,
				(struct ieee80211_mgmt *)pucBeaconProbeResp,
				u4FrameLen, i4SignalStrength * 100, GFP_KERNEL);

		if (!bss) {
			/* ToDo:: DBGLOG */
			DBGLOG(REQ, WARN,
			       "cfg80211_inform_bss_frame() returned with NULL\n");
		} else
			cfg80211_put_bss(wiphy, bss);
	}

}

/*----------------------------------------------------------------------------*/
/*!
 * \brief    To indicate channel ready
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *           none
 */
/*----------------------------------------------------------------------------*/
void
kalReadyOnChannel(struct GLUE_INFO *prGlueInfo,
		  uint64_t u8Cookie,
		  enum ENUM_BAND eBand, enum ENUM_CHNL_EXT eSco,
		  uint8_t ucChannelNum, uint32_t u4DurationMs,
		  uint8_t ucBssIndex)
{
	struct ieee80211_channel *prChannel = NULL;
	enum nl80211_channel_type rChannelType;

	/* ucChannelNum = wlanGetChannelNumberByNetwork(prGlueInfo->prAdapter,
	 *                NETWORK_TYPE_AIS_INDEX);
	 */

	if (prGlueInfo->fgIsRegistered == TRUE) {
		struct net_device *prDevHandler =
			wlanGetNetDev(prGlueInfo, ucBssIndex);

		if (prDevHandler == NULL) {
			DBGLOG(REQ, WARN, "prDevHandler is null\n");
			return;
		}

#if (CFG_SUPPORT_WIFI_6G == 1)
		if (eBand == BAND_6G) {
			prChannel =
				ieee80211_get_channel(
					wlanGetWiphy(),
					ieee80211_channel_to_frequency
					(ucChannelNum, KAL_BAND_6GHZ));
		} else
#endif
		if (ucChannelNum <= 14) {
			prChannel =
				ieee80211_get_channel(wlanGetWiphy(),
				ieee80211_channel_to_frequency(ucChannelNum,
				KAL_BAND_2GHZ));
		} else {
			prChannel =
				ieee80211_get_channel(wlanGetWiphy(),
				ieee80211_channel_to_frequency(ucChannelNum,
				KAL_BAND_5GHZ));
		}

		switch (eSco) {
		case CHNL_EXT_SCN:
			rChannelType = NL80211_CHAN_NO_HT;
			break;

		case CHNL_EXT_SCA:
			rChannelType = NL80211_CHAN_HT40MINUS;
			break;

		case CHNL_EXT_SCB:
			rChannelType = NL80211_CHAN_HT40PLUS;
			break;

		case CHNL_EXT_RES:
		default:
			rChannelType = NL80211_CHAN_HT20;
			break;
		}

		if (!prChannel) {
			DBGLOG(AIS, ERROR, "prChannel is NULL, return!");
			return;
		}

		cfg80211_ready_on_channel(
			prDevHandler->ieee80211_ptr,
			u8Cookie, prChannel, u4DurationMs, GFP_KERNEL);
	}

}

/*----------------------------------------------------------------------------*/
/*!
 * \brief    To indicate channel expiration
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *           none
 */
/*----------------------------------------------------------------------------*/
void kalRemainOnChannelExpired(struct GLUE_INFO *prGlueInfo,
			  uint64_t u8Cookie, enum ENUM_BAND eBand,
			  enum ENUM_CHNL_EXT eSco, uint8_t ucChannelNum,
			  uint8_t ucBssIndex)
{
	struct ieee80211_channel *prChannel = NULL;
	enum nl80211_channel_type rChannelType;
	struct net_device *prDevHandler;

	ucChannelNum = wlanGetChannelNumberByNetwork(prGlueInfo->prAdapter,
			ucBssIndex);

	if (!prGlueInfo->fgIsRegistered)
		return;

	prDevHandler = wlanGetNetDev(prGlueInfo, ucBssIndex);
	if (!prDevHandler)
		return;

#if (CFG_SUPPORT_WIFI_6G == 1)
	if (eBand == BAND_6G) {
		prChannel = ieee80211_get_channel(wlanGetWiphy(),
				ieee80211_channel_to_frequency
				(ucChannelNum, KAL_BAND_6GHZ));
		} else
#endif
	if (ucChannelNum <= 14) {
		prChannel = ieee80211_get_channel(wlanGetWiphy(),
			ieee80211_channel_to_frequency(ucChannelNum,
			KAL_BAND_2GHZ));
	} else {
		prChannel = ieee80211_get_channel(wlanGetWiphy(),
			ieee80211_channel_to_frequency(ucChannelNum,
			KAL_BAND_5GHZ));
	}

	switch (eSco) {
	case CHNL_EXT_SCN:
		rChannelType = NL80211_CHAN_NO_HT;
		break;

	case CHNL_EXT_SCA:
		rChannelType = NL80211_CHAN_HT40MINUS;
		break;

	case CHNL_EXT_SCB:
		rChannelType = NL80211_CHAN_HT40PLUS;
		break;

	case CHNL_EXT_RES:
	default:
		rChannelType = NL80211_CHAN_HT20;
		break;
	}

	if (!prChannel) {
		DBGLOG(AIS, ERROR, "prChannel is NULL, return!");
		return;
	}

	cfg80211_remain_on_channel_expired(prDevHandler->ieee80211_ptr,
			u8Cookie, prChannel, GFP_KERNEL);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief    To indicate Mgmt tx status
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *           none
 */
/*----------------------------------------------------------------------------*/
void kalIndicateMgmtTxStatus(struct GLUE_INFO *prGlueInfo,
			uint64_t u8Cookie, u_int8_t fgIsAck,
			uint8_t *pucFrameBuf, uint32_t u4FrameLen,
			uint8_t ucBssIndex)
{

	do {
		struct net_device *prDevHandler;

		if ((prGlueInfo == NULL)
		    || (pucFrameBuf == NULL)
		    || (u4FrameLen == 0)) {
			DBGLOG(AIS, TRACE,
			       "Unexpected pointer PARAM. 0x%p, 0x%p, %d.",
			       prGlueInfo, pucFrameBuf, u4FrameLen);
			ASSERT(FALSE);
			break;
		}

		prDevHandler = wlanGetNetDev(prGlueInfo, ucBssIndex);
		if (!prDevHandler)
			return;

		cfg80211_mgmt_tx_status(prDevHandler->ieee80211_ptr,
			u8Cookie, pucFrameBuf, u4FrameLen, fgIsAck, GFP_KERNEL);

	} while (FALSE);

}				/* kalIndicateMgmtTxStatus */

void kalIndicateRxMgmtFrame(struct ADAPTER *prAdapter,
			    struct GLUE_INFO *prGlueInfo,
			    struct SW_RFB *prSwRfb,
			    uint8_t ucBssIndex,
			    uint32_t u4LinkId)
{
#if (KERNEL_VERSION(6, 0, 0) <= CFG80211_VERSION_CODE)
	struct cfg80211_rx_info rRxInfo;
#endif
	int32_t i4Freq = 0;
	uint8_t ucChnlNum = 0;
	struct RX_DESC_OPS_T *prRxDescOps;
	enum ENUM_BAND eBand;
	u_int8_t fgIsP2pNetDevice = FALSE;

	do {
		struct net_device *prDevHandler;
		struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPrivate = NULL;

		if ((prGlueInfo == NULL) || (prSwRfb == NULL)) {
			ASSERT(FALSE);
			break;
		}

		ucChnlNum = prSwRfb->ucChnlNum;

		prRxDescOps = prAdapter->chip_info->prRxDescOps;

		eBand = prSwRfb->eRfBand;

		i4Freq = nicChannelNum2Freq(ucChnlNum, eBand) / 1000;

		if (!prGlueInfo->fgIsRegistered) {
			DBGLOG(AIS, WARN,
				"Net dev is not ready!\n");
			break;
		}

		prDevHandler = wlanGetNetDev(prGlueInfo, ucBssIndex);
		if (!prDevHandler)
			return;

#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
		fgIsP2pNetDevice = mtk_IsP2PNetDevice(prGlueInfo, prDevHandler);
#endif
		prNetDevPrivate = (struct NETDEV_PRIVATE_GLUE_INFO *)
			netdev_priv(prDevHandler);

		if (!fgIsP2pNetDevice &&
		    prNetDevPrivate->u4OsMgmtFrameFilter == 0) {
			DBGLOG(AIS, WARN,
				"The cfg80211 hasn't do mgmt register!\n");
			break;
		}

#if (KERNEL_VERSION(6, 0, 0) <= CFG80211_VERSION_CODE)
		kalMemZero(&rRxInfo, sizeof(rRxInfo));
		rRxInfo.freq = MHZ_TO_KHZ(i4Freq);
		rRxInfo.sig_dbm = RCPI_TO_dBm(
			nicRxGetRcpiValueFromRxv(prAdapter,
				RCPI_MODE_WF0, prSwRfb));
		rRxInfo.buf = prSwRfb->pvHeader;
		rRxInfo.len = prSwRfb->u2PacketLen;
		rRxInfo.flags = GFP_ATOMIC;
#if (CFG_SUPPORT_802_11BE_MLO == 1)
		if (u4LinkId != MLD_LINK_ID_NONE) {
			nicMgmtMAT_L2M(prAdapter, prSwRfb);
			rRxInfo.have_link_id = true;
			rRxInfo.link_id = u4LinkId;
		}
#endif

		cfg80211_rx_mgmt_ext(prDevHandler->ieee80211_ptr, &rRxInfo);
#elif (KERNEL_VERSION(3, 18, 0) <= CFG80211_VERSION_CODE)
		cfg80211_rx_mgmt(prDevHandler->ieee80211_ptr,
			i4Freq,	/* in MHz */
			RCPI_TO_dBm((uint8_t) nicRxGetRcpiValueFromRxv(
			prGlueInfo->prAdapter,
			RCPI_MODE_MAX, prSwRfb)),
			prSwRfb->pvHeader, prSwRfb->u2PacketLen,
			NL80211_RXMGMT_FLAG_ANSWERED);

#elif (KERNEL_VERSION(3, 12, 0) <= CFG80211_VERSION_CODE)
		cfg80211_rx_mgmt(prDevHandler->ieee80211_ptr,
			i4Freq,	/* in MHz */
			RCPI_TO_dBm((uint8_t)
			nicRxGetRcpiValueFromRxv(
				prGlueInfo->prAdapter, RCPI_MODE_WF0, prSwRfb)),
			prSwRfb->pvHeader, prSwRfb->u2PacketLen,
			NL80211_RXMGMT_FLAG_ANSWERED,
			GFP_ATOMIC);
#else
		cfg80211_rx_mgmt(prDevHandler->ieee80211_ptr,
			i4Freq,	/* in MHz */
			RCPI_TO_dBm((uint8_t)
			nicRxGetRcpiValueFromRxv(
				prGlueInfo->prAdapter, RCPI_MODE_WF0, prSwRfb)),
			prSwRfb->pvHeader, prSwRfb->u2PacketLen,
			GFP_ATOMIC);
#endif

	} while (FALSE);

}				/* kalIndicateRxMgmtFrame */

#if CFG_SUPPORT_SDIO_READ_WRITE_PATTERN
/*----------------------------------------------------------------------------*/
/*!
 * \brief    To configure SDIO test pattern mode
 *
 * \param[in]
 *           prGlueInfo
 *           fgEn
 *           fgRead
 *
 * \return
 *           TRUE
 *           FALSE
 */
/*----------------------------------------------------------------------------*/
u_int8_t kalSetSdioTestPattern(struct GLUE_INFO *prGlueInfo,
				u_int8_t fgEn, u_int8_t fgRead)
{
	const uint8_t aucPattern[] = {
		0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55,
		0xaa, 0x55, 0x80, 0x80, 0x80, 0x7f, 0x80, 0x80,
		0x80, 0x7f, 0x7f, 0x7f, 0x80, 0x7f, 0x7f, 0x7f,
		0x40, 0x40, 0x40, 0xbf, 0x40, 0x40, 0x40, 0xbf,
		0xbf, 0xbf, 0x40, 0xbf, 0xbf, 0xbf, 0x20, 0x20,
		0x20, 0xdf, 0x20, 0x20, 0x20, 0xdf, 0xdf, 0xdf,
		0x20, 0xdf, 0xdf, 0xdf, 0x10, 0x10, 0x10, 0xef,
		0x10, 0x10, 0x10, 0xef, 0xef, 0xef, 0x10, 0xef,
		0xef, 0xef, 0x08, 0x08, 0x08, 0xf7, 0x08, 0x08,
		0x08, 0xf7, 0xf7, 0xf7, 0x08, 0xf7, 0xf7, 0xf7,
		0x04, 0x04, 0x04, 0xfb, 0x04, 0x04, 0x04, 0xfb,
		0xfb, 0xfb, 0x04, 0xfb, 0xfb, 0xfb, 0x02, 0x02,
		0x02, 0xfd, 0x02, 0x02, 0x02, 0xfd, 0xfd, 0xfd,
		0x02, 0xfd, 0xfd, 0xfd, 0x01, 0x01, 0x01, 0xfe,
		0x01, 0x01, 0x01, 0xfe, 0xfe, 0xfe, 0x01, 0xfe,
		0xfe, 0xfe, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00,
		0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff,
		0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff,
		0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00,
		0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
		0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0xff,
		0x00, 0x00, 0x00, 0xff
	};
	uint32_t i;

	ASSERT(prGlueInfo);

	/* access to MCR_WTMCR to engage PRBS mode */
	prGlueInfo->fgEnSdioTestPattern = fgEn;
	prGlueInfo->fgSdioReadWriteMode = fgRead;

	if (fgRead == FALSE) {
		/* fill buffer for data to be written */
		for (i = 0; i < sizeof(aucPattern); i++)
			prGlueInfo->aucSdioTestBuffer[i] = aucPattern[i];
	}

	return TRUE;
}
#endif

#if (CFG_MET_PACKET_TRACE_SUPPORT == 1)

struct proc_dir_entry *pMetProcDir;
void *pMetGlobalData;

#endif
/*----------------------------------------------------------------------------*/
/*!
 * \brief    To indicate scheduled scan results are avilable
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *           None
 */
/*----------------------------------------------------------------------------*/
void kalSchedScanResults(struct GLUE_INFO *prGlueInfo)
{
	ASSERT(prGlueInfo);
	scanReportBss2Cfg80211(prGlueInfo->prAdapter,
			       BSS_TYPE_INFRASTRUCTURE, NULL);

	scanlog_dbg(LOG_SCHED_SCAN_DONE_D2K, INFO, "Call cfg80211_sched_scan_results\n");
#if KERNEL_VERSION(4, 12, 0) <= CFG80211_VERSION_CODE
	cfg80211_sched_scan_results(wlanGetWiphy(), 0);
#else
	cfg80211_sched_scan_results(wlanGetWiphy());
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief    To indicate scheduled scan has been stopped
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *           None
 */
/*----------------------------------------------------------------------------*/
void kalSchedScanStopped(struct GLUE_INFO *prGlueInfo,
			 u_int8_t fgDriverTriggerd)
{
	/* DBGLOG(SCN, INFO, ("-->kalSchedScanStopped\n" )); */

	GLUE_SPIN_LOCK_DECLARATION();

	ASSERT(prGlueInfo);

#if 1
	/* 1. reset first for newly incoming request */
	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
	if (prGlueInfo->prSchedScanRequest != NULL)
		prGlueInfo->prSchedScanRequest = NULL;
	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
#endif
	DBGLOG(SCN, INFO, "Driver triggerd %d\n", fgDriverTriggerd);

	/* 2. indication to cfg80211 */
	/* 20150205 change cfg80211_sched_scan_stopped to work queue to use K
	 * thread to send event instead of Tx thread
	 * due to sched_scan_mtx dead lock issue by Tx thread serves oid cmds
	 * and send event in the same time
	 */
	if (fgDriverTriggerd) {
		DBGLOG(SCN, INFO, "start work queue to send event\n");
		schedule_delayed_work(&sched_workq, 0);
		DBGLOG(SCN, INFO, "main_thread return from %s\n", __func__);
	}
}

#if CFG_SUPPORT_WAKEUP_REASON_DEBUG
/*----------------------------------------------------------------------------*/
/*!
 * \brief    To check if device if wake up by wlan
 *
 * \param[in]
 *           prAdapter
 *
 * \return
 *           TRUE: wake up by wlan; otherwise, FALSE
 */
/*----------------------------------------------------------------------------*/
u_int8_t kalIsWakeupByWlan(struct ADAPTER *prAdapter)
{
	/*
	 * SUSPEND_FLAG_FOR_WAKEUP_REASON is set means system has suspended,
	 * but may be failed duo to some driver suspend failed. so we need
	 * help of function slp_get_wake_reason
	 */
	if (test_and_clear_bit(SUSPEND_FLAG_FOR_WAKEUP_REASON,
			       &prAdapter->ulSuspendFlag) == 0)
		return FALSE;

	return TRUE;
}
#endif



u_int8_t
kalGetIPv4Address(struct net_device *prDev,
		  uint32_t u4MaxNumOfAddr, uint8_t *pucIpv4Addrs,
		  uint32_t *pu4NumOfIpv4Addr)
{
	uint32_t u4NumIPv4 = 0;
	uint32_t u4AddrLen = IPV4_ADDR_LEN;
	struct in_ifaddr *prIfa;

	/* 4 <1> Sanity check of netDevice */
	if (!prDev || !(prDev->ip_ptr)
	    || !((struct in_device *)(prDev->ip_ptr))->ifa_list) {
		DBGLOG(INIT, INFO,
		       "IPv4 address is not available for dev(0x%p)\n", prDev);

		*pu4NumOfIpv4Addr = 0;
		return FALSE;
	}

	prIfa = ((struct in_device *)(prDev->ip_ptr))->ifa_list;

	/* 4 <2> copy the IPv4 address */
	while ((u4NumIPv4 < u4MaxNumOfAddr) && prIfa) {
		kalMemCopy(&pucIpv4Addrs[u4NumIPv4 * u4AddrLen],
			&prIfa->ifa_local, u4AddrLen);
		kalMemCopy(&pucIpv4Addrs[(u4NumIPv4+1) * u4AddrLen],
			&prIfa->ifa_mask, u4AddrLen);
		prIfa = prIfa->ifa_next;

		DBGLOG(INIT, INFO,
			"IPv4 addr [%u][" IPV4STR "] mask [" IPV4STR "]\n",
			u4NumIPv4,
			IPV4TOSTR(&pucIpv4Addrs[u4NumIPv4*u4AddrLen]),
			IPV4TOSTR(&pucIpv4Addrs[(u4NumIPv4+1)*u4AddrLen]));

		u4NumIPv4++;
	}

	*pu4NumOfIpv4Addr = u4NumIPv4;

	return TRUE;
}

#if IS_ENABLED(CONFIG_IPV6)
u_int8_t
kalGetIPv6Address(struct net_device *prDev,
		  uint32_t u4MaxNumOfAddr, uint8_t *pucIpv6Addrs,
		  uint32_t *pu4NumOfIpv6Addr)
{
	uint32_t u4NumIPv6 = 0;
	uint32_t u4AddrLen = IPV6_ADDR_LEN;
	struct inet6_ifaddr *prIfa;

	/* 4 <1> Sanity check of netDevice */
	if (!prDev || !(prDev->ip6_ptr)) {
		DBGLOG(INIT, INFO,
		       "IPv6 address is not available for dev(0x%p)\n", prDev);

		*pu4NumOfIpv6Addr = 0;
		return FALSE;
	}


	/* 4 <2> copy the IPv6 address */
	LIST_FOR_EACH_IPV6_ADDR(prIfa, prDev->ip6_ptr) {
		if (ipv6_addr_src_scope(&prIfa->addr) ==
					IPV6_ADDR_SCOPE_LINKLOCAL
				&& u4NumIPv6 > 0) {
			int32_t i = 0;

			for (i = u4NumIPv6 - 1; i >= 0; i--) {
				kalMemCopy(&pucIpv6Addrs[(i + 1) * u4AddrLen],
					&pucIpv6Addrs[i * u4AddrLen],
					u4AddrLen);
			}
			kalMemCopy(&pucIpv6Addrs[0], &prIfa->addr, u4AddrLen);
		} else {
			kalMemCopy(&pucIpv6Addrs[u4NumIPv6 * u4AddrLen],
				   &prIfa->addr, u4AddrLen);
		}
		DBGLOG(INIT, INFO,
			"IPv6 addr [%u] scope [%u][" IPV6STR "]\n", u4NumIPv6,
			ipv6_addr_src_scope(&prIfa->addr),
			IPV6TOSTR(&prIfa->addr));

		if ((u4NumIPv6 + 1) >= u4MaxNumOfAddr)
			break;
		u4NumIPv6++;
	}

	*pu4NumOfIpv6Addr = u4NumIPv6;

	return TRUE;
}
#endif /* IS_ENABLED(CONFIG_IPV6) */

/**
 * @prGlueInfo: pointer to Glue Info
 * @ucBssIdx: BSS index to set IP address
 * @pucIPv4Addr: buffer holding IPv4 addresses, containing IP address and
 *		 net mask (8 bytes in total) for each entry.
 * @u4NumIPv4Addr: number of IPv4 address in pucIPv4Addr,
 *		   IP address and net mask (8 bytes in total) for each entry.
 * @pucIPv6Addr: buffer holding IPv6 addresses
 * @u4NumIPv6Addr: number of IPv6 address in pucIPv6Addr
 */
void kalSetNetAddress(struct GLUE_INFO *prGlueInfo,
		 uint8_t ucBssIdx,
		 uint8_t *pucIPv4Addr, uint32_t u4NumIPv4Addr,
		 uint8_t *pucIPv6Addr, uint32_t u4NumIPv6Addr)
{
	uint32_t rStatus;
	uint32_t u4SetInfoLen = 0;
	uint32_t u4Len = 0;
	struct PARAM_NETWORK_ADDRESS_LIST *prParamNetAddrList;
	struct PARAM_NETWORK_ADDRESS *prParamNetAddr;
	uint32_t i;

	/* 4 <1> Calculate buffer size */
	u4Len += sizeof(struct PARAM_NETWORK_ADDRESS_LIST);
	/* IPv4 */
	u4Len += (OFFSET_OF(struct PARAM_NETWORK_ADDRESS, aucAddress) +
		  IPV4_ADDR_LEN * 2) * u4NumIPv4Addr;
	/* IPv6 */
	u4Len += (OFFSET_OF(struct PARAM_NETWORK_ADDRESS, aucAddress) +
		  IPV6_ADDR_LEN) * u4NumIPv6Addr;

	/* 4 <2> Allocate buffer */
	prParamNetAddrList = kalMemAlloc(u4Len, VIR_MEM_TYPE);

	if (!prParamNetAddrList) {
		DBGLOG(INIT, WARN,
		       "Fail to alloc buffer for setting BSS[%u] network address!\n",
		       ucBssIdx);
		return;
	}
	/* 4 <3> Fill up network address */
	prParamNetAddrList->u2AddressType = PARAM_PROTOCOL_ID_TCP_IP;
	prParamNetAddrList->u4AddressCount = 0;
	prParamNetAddrList->ucBssIdx = ucBssIdx;

	/* 4 <3.1> Fill up IPv4 address */
	prParamNetAddr =
		(struct PARAM_NETWORK_ADDRESS *)(prParamNetAddrList + 1);
	for (i = 0; i < u4NumIPv4Addr; i++) {
		prParamNetAddr->u2AddressType = PARAM_PROTOCOL_ID_TCP_IP;
		prParamNetAddr->u2AddressLength = IPV4_ADDR_LEN;
		kalMemCopy(prParamNetAddr->aucAddress,
			&pucIPv4Addr[IPV4_ADDR_LEN * 2 * i], IPV4_ADDR_LEN * 2);

		prParamNetAddr = (struct PARAM_NETWORK_ADDRESS *)
			((unsigned long) prParamNetAddr +
			(unsigned long) (IPV4_ADDR_LEN * 2 +
			OFFSET_OF(struct PARAM_NETWORK_ADDRESS, aucAddress)));
	}
	prParamNetAddrList->u4AddressCount += u4NumIPv4Addr;

	/* 4 <3.2> Fill up IPv6 address */
	for (i = 0; i < u4NumIPv6Addr; i++) {
		prParamNetAddr->u2AddressType = PARAM_PROTOCOL_ID_TCP_IP;
		prParamNetAddr->u2AddressLength = IPV6_ADDR_LEN;
		kalMemCopy(prParamNetAddr->aucAddress,
			   &pucIPv6Addr[IPV6_ADDR_LEN * i], IPV6_ADDR_LEN);

		prParamNetAddr = (struct PARAM_NETWORK_ADDRESS *) ((
			unsigned long) prParamNetAddr + (unsigned long) (
			IPV6_ADDR_LEN + OFFSET_OF(
			struct PARAM_NETWORK_ADDRESS, aucAddress)));
	}
	prParamNetAddrList->u4AddressCount += u4NumIPv6Addr;

	/* 4 <4> IOCTL to main_thread */
	rStatus = kalIoctl(prGlueInfo, wlanoidSetNetworkAddress,
			   prParamNetAddrList, u4Len, &u4SetInfoLen);

	if (rStatus != WLAN_STATUS_SUCCESS)
		DBGLOG(REQ, WARN, "%s: Fail to set network address\n",
		       __func__);
/* fos_change begin */
#if CFG_SUPPORT_SET_IPV6_NETWORK
		rStatus = kalIoctl(prGlueInfo, wlanoidSetIPv6NetworkAddress,
				   (void *) prParamNetAddrList, u4Len,
				   &u4SetInfoLen);

		if (rStatus != WLAN_STATUS_SUCCESS)
			DBGLOG(REQ, WARN,
				"%s: Fail to set IPv6 network address\n",
				   __func__);
#endif /* fos_change end */

	kalMemFree(prParamNetAddrList, VIR_MEM_TYPE, u4Len);

}

void kalSetNetAddressFromInterface(struct GLUE_INFO
		   *prGlueInfo, struct net_device *prDev, u_int8_t fgSet)
{
	uint32_t u4NumIPv4, u4NumIPv6;
	uint8_t pucIPv4Addr[IPV4_ADDR_LEN * CFG_PF_ARP_NS_MAX_NUM*2] = { 0 };
	uint8_t pucIPv6Addr[IPV6_ADDR_LEN * CFG_PF_ARP_NS_MAX_NUM] = { 0 };
	struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPrivate =
		(struct NETDEV_PRIVATE_GLUE_INFO *) NULL;
	struct BSS_INFO *prBssInfo = (struct BSS_INFO *) NULL;

#if CFG_ENABLE_WIFI_DIRECT
	GLUE_SPIN_LOCK_DECLARATION();
#endif

	prNetDevPrivate = (struct NETDEV_PRIVATE_GLUE_INFO *)
			  netdev_priv(prDev);

	if (prDev == NULL || prNetDevPrivate == NULL) {
		DBGLOG(REQ, WARN,
			"invalid arguments, prDev=0x%p, prNetDev=0x%p\n",
			prDev, prNetDevPrivate);
		return;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prGlueInfo->prAdapter,
		prNetDevPrivate->ucBssIdx);

	if (!prBssInfo) {
		DBGLOG(REQ, WARN,
			"invalid BssInfo, prDev=0x%p, BssIdx=%d\n",
			prDev, prNetDevPrivate->ucBssIdx);
		return;
	}

#if CFG_ENABLE_WIFI_DIRECT
	if (IS_BSS_P2P(prBssInfo)) { /* P2P */
		GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
		if (prGlueInfo->prAdapter->rP2PNetRegState !=
			ENUM_NET_REG_STATE_REGISTERED) {
			DBGLOG(REQ, WARN,
				"invalid p2p net register state=%d\n",
				prGlueInfo->prAdapter->rP2PNetRegState);
			GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
			return;
		}
		GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
	}
#endif
	if (prNetDevPrivate->prGlueInfo != prGlueInfo)
		DBGLOG(REQ, WARN, "%s: unexpected prGlueInfo(0x%p)!\n",
		       __func__, prNetDevPrivate->prGlueInfo);

	u4NumIPv4 = 0;
	u4NumIPv6 = 0;

	if (fgSet) {
		kalGetIPv4Address(prDev, CFG_PF_ARP_NS_MAX_NUM, pucIPv4Addr,
				  &u4NumIPv4);
		kalGetIPv6Address(prDev, CFG_PF_ARP_NS_MAX_NUM, pucIPv6Addr,
				  &u4NumIPv6);
	}

	if (u4NumIPv4 + u4NumIPv6 > CFG_PF_ARP_NS_MAX_NUM) {
		if (u4NumIPv4 >= CFG_PF_ARP_NS_MAX_NUM) {
			u4NumIPv4 = CFG_PF_ARP_NS_MAX_NUM;
			u4NumIPv6 = 0;
		} else {
			u4NumIPv6 = CFG_PF_ARP_NS_MAX_NUM - u4NumIPv4;
		}
	}

	DBGLOG(REQ, LOUD,
		"prDev=0x%p, BssIdx=%d, NumIPv4=%d, NumIPv6=%d\n",
		prDev, prNetDevPrivate->ucBssIdx, u4NumIPv4, u4NumIPv6);

	kalSetNetAddress(prGlueInfo, prNetDevPrivate->ucBssIdx,
			 pucIPv4Addr, u4NumIPv4, pucIPv6Addr, u4NumIPv6);
}

#if CFG_MET_PACKET_TRACE_SUPPORT

u_int8_t kalMetCheckProfilingPacket(struct GLUE_INFO
				    *prGlueInfo, void *prPacket)
{
	uint32_t u4PacketLen;
	uint16_t u2EtherTypeLen;
	struct sk_buff *prSkb = (struct sk_buff *)prPacket;
	uint8_t *aucLookAheadBuf = NULL;
	uint8_t ucEthTypeLenOffset = ETHER_HEADER_LEN -
				     ETHER_TYPE_LEN;
	uint8_t *pucNextProtocol = NULL;

	u4PacketLen = prSkb->len;

	if (u4PacketLen < ETHER_HEADER_LEN) {
		DBGLOG(INIT, WARN, "Invalid Ether packet length: %u\n",
		       u4PacketLen);
		return FALSE;
	}

	aucLookAheadBuf = prSkb->data;

	/* 4 <0> Obtain Ether Type/Len */
	WLAN_GET_FIELD_BE16(&aucLookAheadBuf[ucEthTypeLenOffset],
			    &u2EtherTypeLen);

	/* 4 <1> Skip 802.1Q header (VLAN Tagging) */
	if (u2EtherTypeLen == ETH_P_VLAN) {
		ucEthTypeLenOffset += ETH_802_1Q_HEADER_LEN;
		WLAN_GET_FIELD_BE16(&aucLookAheadBuf[ucEthTypeLenOffset],
				    &u2EtherTypeLen);
	}
	/* 4 <2> Obtain next protocol pointer */
	pucNextProtocol = &aucLookAheadBuf[ucEthTypeLenOffset +
							      ETHER_TYPE_LEN];

	/* 4 <3> Handle ethernet format */
	switch (u2EtherTypeLen) {

	/* IPv4 */
	case ETH_P_IPV4: {
		uint8_t *pucIpHdr = pucNextProtocol;
		uint8_t ucIpVersion;

		/* IPv4 header length check */
		if (u4PacketLen < (ucEthTypeLenOffset + ETHER_TYPE_LEN +
				   IPV4_HDR_LEN)) {
			DBGLOG(INIT, WARN, "Invalid IPv4 packet length: %u\n",
			       u4PacketLen);
			return FALSE;
		}

		/* IPv4 version check */
		ucIpVersion = (pucIpHdr[0] & IP_VERSION_MASK) >>
			      IP_VERSION_OFFSET;
		if (ucIpVersion != IP_VERSION_4) {
			DBGLOG(INIT, WARN, "Invalid IPv4 packet version: %u\n",
			       ucIpVersion);
			return FALSE;
		}

		if (pucIpHdr[IPV4_HDR_IP_PROTOCOL_OFFSET] == IP_PRO_UDP) {
			uint8_t *pucUdpHdr = &pucIpHdr[IPV4_HDR_LEN];
			uint16_t u2UdpDstPort;
			uint16_t u2UdpSrcPort;

			/* Get UDP DST port */
			WLAN_GET_FIELD_BE16(&pucUdpHdr[UDP_HDR_DST_PORT_OFFSET],
					    &u2UdpDstPort);

			/* Get UDP SRC port */
			WLAN_GET_FIELD_BE16(&pucUdpHdr[UDP_HDR_SRC_PORT_OFFSET],
					    &u2UdpSrcPort);

			if (u2UdpSrcPort == prGlueInfo->u2MetUdpPort) {
				uint16_t u2IpId;

				/* Store IP ID for Tag */
				WLAN_GET_FIELD_BE16(
				&pucIpHdr[IPV4_HDR_IP_IDENTIFICATION_OFFSET],
				&u2IpId);
#if 0
				DBGLOG(INIT, INFO,
				       "TX PKT PROTOCOL[0x%x] UDP DST port[%u] IP_ID[%u]\n",
				       pucIpHdr[IPV4_HDR_IP_PROTOCOL_OFFSET],
				       u2UdpDstPort,
				       u2IpId);
#endif
				GLUE_SET_PKT_IP_ID(prPacket, u2IpId);

				return TRUE;
			}
		}
	}
	break;

	default:
		break;
	}

	return FALSE;
}

#if 0

static unsigned long __read_mostly tracing_mark_write_addr;

static int __mt_find_tracing_mark_write_symbol_fn(
	void *prData, const char *pcNameBuf,
	struct module *prModule, unsigned long ulAddress)
{
	if (strcmp(pcNameBuf, "tracing_mark_write") == 0) {
		tracing_mark_write_addr = ulAddress;
		return 1;
	}
	return 0;
}
#endif

static inline void __mt_update_tracing_mark_write_addr(void)
{
#if 0
	if (unlikely(tracing_mark_write_addr == 0))
		kallsyms_on_each_symbol(
			__mt_find_tracing_mark_write_symbol_fn, NULL);
#endif
}

void kalMetTagPacket(struct GLUE_INFO *prGlueInfo,
		     void *prPacket, enum ENUM_TX_PROFILING_TAG eTag)
{
	if (!prGlueInfo->fgMetProfilingEn)
		return;

	switch (eTag) {
	case TX_PROF_TAG_OS_TO_DRV:
		if (kalMetCheckProfilingPacket(prGlueInfo, prPacket)) {
			/* trace_printk("S|%d|%s|%d\n", current->pid,
			 * "WIFI-CHIP", GLUE_GET_PKT_IP_ID(prPacket));
			 */
			__mt_update_tracing_mark_write_addr();
#if 0 /* #ifdef CONFIG_TRACING */ /* #if CFG_MET_PACKET_TRACE_SUPPORT */
			event_trace_printk(tracing_mark_write_addr,
					   "S|%d|%s|%d\n",
					   current->tgid, "WIFI-CHIP",
					   GLUE_GET_PKT_IP_ID(prPacket));
#endif
			GLUE_SET_PKT_FLAG_PROF_MET(prPacket);
		}
		break;

	case TX_PROF_TAG_DRV_TX_DONE:
		if (GLUE_GET_PKT_IS_PROF_MET(prPacket)) {
			/* trace_printk("F|%d|%s|%d\n", current->pid,
			 * "WIFI-CHIP", GLUE_GET_PKT_IP_ID(prPacket));
			 */
			__mt_update_tracing_mark_write_addr();
#if 0 /* #ifdef CONFIG_TRACING */ /* #if CFG_MET_PACKET_TRACE_SUPPORT */
			event_trace_printk(tracing_mark_write_addr,
					   "F|%d|%s|%d\n",
					   current->tgid, "WIFI-CHIP",
					   GLUE_GET_PKT_IP_ID(prPacket));
#endif
		}
		break;

	default:
		break;
	}
}

void kalMetInit(struct GLUE_INFO *prGlueInfo)
{
	prGlueInfo->fgMetProfilingEn = FALSE;
	prGlueInfo->u2MetUdpPort = 0;
}
#endif

u_int8_t kalSendUevent(struct ADAPTER *prAdapter, const char *src)
{
	struct MSG_UEVENT_REQ *prUeventReq;

	prUeventReq = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
				  sizeof(struct MSG_UEVENT_REQ));
	if (!prUeventReq)
		return FALSE;

	DBGLOG(INIT, TRACE, "Send UEvent: %s", src);
	prUeventReq->rMsgHdr.eMsgId = MID_UEVENT_REQ;
	strscpy(prUeventReq->event_string, src,
		sizeof(prUeventReq->event_string));

	mboxSendMsg(prAdapter, MBOX_ID_0, (struct MSG_HDR *)prUeventReq,
		    MSG_SEND_METHOD_BUF);

	return TRUE;
}

void kalSendUeventHandler(struct ADAPTER *prAdapter, struct MSG_HDR *prMsgHdr)
{
	struct MSG_UEVENT_REQ *prUevnetReq;
	const char *src;
	int ret;
	char *envp[2];
	char event_string[300];

	prUevnetReq = (struct MSG_UEVENT_REQ *)prMsgHdr;
	src = prUevnetReq->event_string;
	envp[0] = event_string;
	envp[1] = NULL;

	DBGLOG(INIT, INFO, "Send UEvent: %s", src);

	/* send uevent */
	strscpy(event_string, src, sizeof(event_string));
	if (event_string[0] == '\0') /* string is null */
		goto end;

	ret = kobject_uevent_env(&wlan_object.this_device->kobj,
				 KOBJ_CHANGE, envp);

	if (ret != 0)
		DBGLOG(INIT, WARN, "uevent failed\n");

end:
	cnmMemFree(prAdapter, prMsgHdr);
}

void kalWlanUeventInit(struct GLUE_INFO *prGlueInfo)
{
	int ret = 0;

	if (!prGlueInfo || prGlueInfo->fgWlanUevent)
		return;

	/* dev init */
#ifdef CFG_COMBO_SLT_GOLDEN
	wlan_object.name = "ra";
#else
	wlan_object.name = "wlan";
#endif
	wlan_object.minor = MISC_DYNAMIC_MINOR;
	ret = misc_register(&wlan_object);
	if (ret) {
		DBGLOG(INIT, WARN, "misc_register error:%d\n", ret);
		return;
	}

	ret = kobject_uevent(
			&wlan_object.this_device->kobj, KOBJ_ADD);

	if (ret) {
		misc_deregister(&wlan_object);
		DBGLOG(INIT, WARN, "uevent creat fail:%d\n", ret);
		return;
	}

	prGlueInfo->fgWlanUevent = TRUE;

	DBGLOG(INIT, TRACE, "Uevent init success, flag:%u\n",
		prGlueInfo->fgWlanUevent);
}

void kalWlanUeventDeinit(struct GLUE_INFO *prGlueInfo)
{
	if (!prGlueInfo || !prGlueInfo->fgWlanUevent)
		return;

	DBGLOG(INIT, TRACE, "Uevent deinit, flag:%u\n",
		prGlueInfo->fgWlanUevent);

	misc_deregister(&wlan_object);
	prGlueInfo->fgWlanUevent = FALSE;
}

#if CFG_SUPPORT_DATA_STALL
u_int8_t kalIndicateDriverEvent(struct ADAPTER *prAdapter,
				uint32_t event,
				uint16_t dataLen,
				uint8_t ucBssIdx,
				u_int8_t fgForceReport)
{
	struct sk_buff *skb = NULL;
	struct net_device *netdev;
	struct wiphy *wiphy;
	struct wireless_dev *wdev;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	char uevent[30];

	wiphy = wlanGetWiphy();
	netdev = wlanGetNetDev(prAdapter->prGlueInfo, ucBssIdx);
	if (!netdev)
		return -EINVAL;
	wdev = netdev->ieee80211_ptr;

	if (!wiphy || !wdev || !prWifiVar)
		return -EINVAL;

	if (!fgForceReport) {
		if (prAdapter->tmReportinterval > 0 &&
			!CHECK_FOR_TIMEOUT(kalGetTimeTick(),
			prAdapter->tmReportinterval,
			prWifiVar->u4ReportEventInterval*1000)) {
			return -ETIME;
		}
		GET_CURRENT_SYSTIME(&prAdapter->tmReportinterval);
	}

	kalSnprintf(uevent, sizeof(uevent), "code=%d", event);
	kalSendUevent(prAdapter, uevent);

	skb = kalCfg80211VendorEventAlloc(wiphy, wdev,
		dataLen,
		WIFI_EVENT_DRIVER_ERROR, GFP_KERNEL);
	if (!skb) {
		DBGLOG(REQ, ERROR, "%s allocate skb failed\n", __func__);
		return -ENOMEM;
	}

	if (dataLen > 0 &&
		unlikely(nla_put(skb, WIFI_ATTRIBUTE_ERROR_REASON
		, dataLen, &event) < 0))
		goto nla_put_failure;

	DBGLOG(INIT, ERROR, "event to cfg80211[id:%d][len:%d][F:%d]:%d\n",
		WIFI_EVENT_DRIVER_ERROR,
		dataLen, fgForceReport, event);

	cfg80211_vendor_event(skb, GFP_KERNEL);
	return TRUE;
nla_put_failure:
	kfree_skb(skb);
	return FALSE;
}
#endif

#if CFG_SUPPORT_BIGDATA_PIP
int8_t kalBigDataPip(struct ADAPTER *prAdapter,
					uint8_t *payload,
					uint16_t dataLen)
{
	struct sk_buff *skb = NULL;
	struct wiphy *wiphy;
	struct wireless_dev *wdev;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;

	wiphy = wlanGetWiphy();
	wdev = ((prAdapter->prGlueInfo)->prDevHandler)->ieee80211_ptr;

	if (!wiphy || !wdev || !prWifiVar || !payload)
		return -EINVAL;

	/* Max length of report data is discussed to 500.*/
	if (dataLen > 500)
		return -EINVAL;

	if (prAdapter->tmDataPipReportinterval > 0 &&
		!CHECK_FOR_TIMEOUT(kalGetTimeTick(),
		prAdapter->tmDataPipReportinterval, 20)) {
		return -ETIME;
	}
	GET_CURRENT_SYSTIME(&prAdapter->tmDataPipReportinterval);

	skb = kalCfg80211VendorEventAlloc(wiphy, wdev, dataLen,
		WIFI_EVENT_BIGDATA_PIP, GFP_KERNEL);
	if (!skb) {
		DBGLOG(REQ, ERROR, "%s allocate skb failed\n", __func__);
		return -ENOMEM;
	}

	if (dataLen > 0 &&
		unlikely(nla_put(skb, WIFI_ATTRIBUTE_PIP_PAYLOAD
		, dataLen, payload) < 0))
		goto nla_put_failure;

	cfg80211_vendor_event(skb, GFP_KERNEL);
	return TRUE;
nla_put_failure:
	kfree_skb(skb);
	return FALSE;
}
#endif

#if CFG_SUPPORT_DBDC
int8_t kalIndicateOpModeChange(struct ADAPTER *prAdapter,
					uint8_t ucBssIdx,
					uint8_t ucChannelBw,
					uint8_t ucTxNss,
					uint8_t ucRxNss)
{
	struct sk_buff *skb = NULL;
	struct wiphy *wiphy;
	struct wireless_dev *wdev;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	uint16_t dataLen = sizeof(uint32_t);
	uint32_t u4OpModeChange = WIFI_VENDOR_DATA_OP_MODE_CHANGE(
		ucBssIdx, ucChannelBw, ucTxNss, ucRxNss);

	wiphy = wlanGetWiphy();
	wdev = ((prAdapter->prGlueInfo)->prDevHandler)->ieee80211_ptr;

	if (!wiphy || !wdev || !prWifiVar)
		return -EINVAL;

	skb = kalCfg80211VendorEventAlloc(wiphy,
		wdev,
		dataLen, WIFI_EVENT_OP_MODE_CHANGE, GFP_KERNEL);
	if (!skb) {
		DBGLOG(REQ, ERROR, "%s allocate skb failed\n", __func__);
		return -ENOMEM;
	}

	if (dataLen > 0 &&
		unlikely(nla_put(skb, WIFI_ATTRIBUTE_OP_MODE_CHANGE
		, dataLen, &u4OpModeChange) < 0))
		goto nla_put_failure;

	cfg80211_vendor_event(skb, GFP_KERNEL);
	return TRUE;
nla_put_failure:
	kfree_skb(skb);
	return FALSE;
}
#endif

#if CFG_SUPPORT_AGPS_ASSIST
u_int8_t kalIndicateAgpsNotify(struct ADAPTER *prAdapter,
			       uint8_t cmd, uint8_t *data, uint16_t dataLen)
{
#ifdef CONFIG_NL80211_TESTMODE
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct sk_buff *skb = NULL;

	skb = cfg80211_testmode_alloc_event_skb(wlanGetWiphy(),
						dataLen, GFP_KERNEL);

	/* DBGLOG(CCX, INFO, ("WLAN_STATUS_AGPS_NOTIFY, cmd=%d\n", cmd)); */
	if (unlikely(nla_put(skb, MTK_ATTR_AGPS_CMD, sizeof(cmd),
			     &cmd) < 0))
		goto nla_put_failure;
	if (dataLen > 0 && data
	    && unlikely(nla_put(skb, MTK_ATTR_AGPS_DATA, dataLen,
				data) < 0))
		goto nla_put_failure;
	if (unlikely(nla_put(skb, MTK_ATTR_AGPS_IFINDEX,
	    sizeof(uint32_t), &prGlueInfo->prDevHandler->ifindex) < 0))
		goto nla_put_failure;
	/* currently, the ifname maybe wlan0, p2p0, so the maximum name length
	 * will be 5 bytes
	 */
	if (unlikely(nla_put(skb, MTK_ATTR_AGPS_IFNAME, 5,
			     prGlueInfo->prDevHandler->name) < 0))
		goto nla_put_failure;

	cfg80211_testmode_event(skb, GFP_KERNEL);
	return TRUE;
nla_put_failure:
	kfree_skb(skb);
#else
	DBGLOG(INIT, WARN, "CONFIG_NL80211_TESTMODE not enabled\n");
#endif
	return FALSE;
}
#endif

uint64_t kalGetBootTime(void)
{
	struct timespec64 ts;
	uint64_t bootTime = 0;

	ktime_get_boottime_ts64(&ts);

	bootTime = ts.tv_sec;
	bootTime *= USEC_PER_SEC;
	bootTime += ts.tv_nsec / NSEC_PER_USEC;
	return bootTime;
}

#if (CFG_CE_ASSERT_DUMP == 1)
uint32_t kalEnqCoreDumpLog(struct ADAPTER *prAdapter, uint8_t *pucBuffer,
			     uint16_t u2Size)
{
	struct sk_buff *skb_tmp = NULL;
	struct sk_buff_head *queue = NULL;

	KAL_SPIN_LOCK_DECLARATION();

	skb_tmp = alloc_skb(u2Size, GFP_ATOMIC);
	if (!skb_tmp)
		return WLAN_STATUS_RESOURCES;

	memcpy(skb_tmp->data, pucBuffer, u2Size);
	skb_tmp->len = u2Size;
	queue = &prAdapter->prGlueInfo->rCoreDumpSkbQueue;
	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_CORE_DUMP);
	skb_queue_tail(queue, skb_tmp);
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_CORE_DUMP);

	wake_up_interruptible(&prAdapter->prGlueInfo->waitq_coredump);

	return WLAN_STATUS_SUCCESS;
}

#endif

#if CFG_WOW_SUPPORT
void kalWowInit(struct GLUE_INFO *prGlueInfo)
{
	struct ADAPTER *prAdapter;

	if (prGlueInfo == NULL || prGlueInfo->prAdapter == NULL) {
		DBGLOG(INIT, ERROR, "NULL point unexpected\n");
		return;
	}

	prAdapter = prGlueInfo->prAdapter;

	kalMemZero(&prAdapter->rWowCtrl.stWowPort,
		   sizeof(struct WOW_PORT));
	prAdapter->rWowCtrl.ucReason = INVALID_WOW_WAKE_UP_REASON;
	prAdapter->fgWowLinkDownPendFlag = FALSE;
#if CFG_SUPPORT_MDNS_OFFLOAD
	prAdapter->mdns_offload_enable = FALSE;
	/* default wake up host when mdns packet not match */
#if CFG_SUPPORT_MDNS_OFFLOAD_TV
	/* for TV, wake up devices when there is no match record */
	prAdapter->mdns_wake_flag =
				MDNS_WAKEUP_BY_NO_MATCH_RECORD;
#else
	/* for GVA, wake up devices when get a sub mdns query packet*/
	prAdapter->mdns_wake_flag = MDNS_WAKEUP_BY_SUB_REQ;
#endif
	kalMdnsOffloadInit(prAdapter);
#endif
}

void kalWowCmdEventSetCb(struct ADAPTER *prAdapter,
			struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	if (prAdapter == NULL || prCmdInfo == NULL) {
		DBGLOG(INIT, ERROR, "NULL point unexpected\n");
		return;
	}

	if (prCmdInfo->ucCID == CMD_ID_SET_PF_CAPABILITY) {
		DBGLOG(INIT, STATE, "CMD_ID_SET_PF_CAPABILITY cmd done\n");
		prAdapter->fgSetPfCapabilityDone = TRUE;
	}

	if (prCmdInfo->ucCID == CMD_ID_SET_WOWLAN) {
		DBGLOG(INIT, STATE, "CMD_ID_SET_WOWLAN cmd done\n");
		prAdapter->fgSetWowDone = TRUE;
	}

}

void kalWowProcess(struct GLUE_INFO *prGlueInfo,
		   uint8_t enable)
{
	struct CMD_WOWLAN_PARAM rCmdWowlanParam;
	struct WOW_CTRL *pWOW_CTRL =
			&prGlueInfo->prAdapter->rWowCtrl;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t ii, wait = 0;
	struct BSS_INFO *prAisBssInfo = NULL;
	uint8_t fgWake = TRUE;

	kalMemZero(&rCmdWowlanParam,
		   sizeof(struct CMD_WOWLAN_PARAM));

	prGlueInfo->prAdapter->fgSetPfCapabilityDone = FALSE;
	prGlueInfo->prAdapter->fgSetWowDone = FALSE;
	if (prGlueInfo->prAdapter->rWifiVar.ucAdvPws == 1
		&& prGlueInfo->prAdapter->rWowCtrl.fgWowEnable == 0)
		fgWake = FALSE;

	prAisBssInfo = aisGetConnectedBssInfo(prGlueInfo->prAdapter);

	if (prAisBssInfo) {
		DBGLOG(PF, INFO,
		       "PF, pAd ucBssIndex=%d, ucOwnMacIndex=%d\n",
		       prAisBssInfo->ucBssIndex,
		       prAisBssInfo->ucOwnMacIndex);
	}
	DBGLOG(PF, INFO, "profile wow=%d, GpioInterval=%d\n",
	       prGlueInfo->prAdapter->rWifiVar.ucWow,
	       prGlueInfo->prAdapter->rWowCtrl.astWakeHif[0].u4GpioInterval);
#if CFG_SUPPORT_MDNS_OFFLOAD
	if (enable && prGlueInfo->prAdapter->mdns_offload_enable) {
		kalSendMdnsFlagsToFw(prGlueInfo);
	}
#endif
	/* 1. ARPNS offload */
	/* 2. SET SUSPEND MODE */
	wlanSetSuspendMode(prGlueInfo, enable);

	/* p2pSetSuspendMode(prGlueInfo, TRUE); */

	/* Let WOW enable/disable as last command, so we can back/restore DMA
	 * classify filter in FW
	 */

	kalMemCopy(&rCmdWowlanParam.astWakeHif[0],
		   &pWOW_CTRL->astWakeHif[0], sizeof(struct WOW_WAKE_HIF));

	if (fgWake) {
		/* copy UDP/TCP port setting */
		kalMemCopy(&rCmdWowlanParam.stWowPort,
				&prGlueInfo->prAdapter->rWowCtrl.stWowPort,
				sizeof(struct WOW_PORT));
	} else
		kalMemZero(&rCmdWowlanParam.stWowPort,
			sizeof(struct WOW_PORT));

	DBGLOG(PF, INFO,
	       "Cmd: IPV4/UDP=%d, IPV4/TCP=%d, IPV6/UDP=%d, IPV6/TCP=%d\n",
	       rCmdWowlanParam.stWowPort.ucIPv4UdpPortCnt,
	       rCmdWowlanParam.stWowPort.ucIPv4TcpPortCnt,
	       rCmdWowlanParam.stWowPort.ucIPv6UdpPortCnt,
	       rCmdWowlanParam.stWowPort.ucIPv6TcpPortCnt);

	for (ii = 0;
	     ii < rCmdWowlanParam.stWowPort.ucIPv4UdpPortCnt; ii++)
		DBGLOG(PF, INFO, "IPV4/UDP port[%d]=%d\n", ii,
		       rCmdWowlanParam.stWowPort.ausIPv4UdpPort[ii]);

	for (ii = 0;
	     ii < rCmdWowlanParam.stWowPort.ucIPv4TcpPortCnt; ii++)
		DBGLOG(PF, INFO, "IPV4/TCP port[%d]=%d\n", ii,
		       rCmdWowlanParam.stWowPort.ausIPv4TcpPort[ii]);

	for (ii = 0;
	     ii < rCmdWowlanParam.stWowPort.ucIPv6UdpPortCnt; ii++)
		DBGLOG(PF, INFO, "IPV6/UDP port[%d]=%d\n", ii,
		       rCmdWowlanParam.stWowPort.ausIPv6UdpPort[ii]);

	for (ii = 0;
	     ii < rCmdWowlanParam.stWowPort.ucIPv6TcpPortCnt; ii++)
		DBGLOG(PF, INFO, "IPV6/TCP port[%d]=%d\n", ii,
		       rCmdWowlanParam.stWowPort.ausIPv6TcpPort[ii]);


	/* GPIO parameter is necessary in suspend/resume */
	if (enable == 1) {

		rCmdWowlanParam.ucCmd = PM_WOWLAN_REQ_START;

		if (fgWake) {

			rCmdWowlanParam.ucDetectType =
				prGlueInfo->prAdapter->
				rWifiVar.ucWowDetectType;
			/* default: WOWLAN_DETECT_TYPE_MAGIC (0x1) */

			rCmdWowlanParam.u2DetectTypeExt =
				prGlueInfo->prAdapter->
				rWifiVar.u2WowDetectTypeExt;
			/* default: WOWLAN_DETECT_TYPE_EXT_PORT (0x1) */

		} else {

			rCmdWowlanParam.ucDetectType =
				WOWLAN_DETECT_TYPE_NONE;

			rCmdWowlanParam.u2DetectTypeExt =
				WOWLAN_DETECT_TYPE_EXT_NONE;
		}

		DBGLOG(PF, STATE,
				"Wow DetectType[0x%x] DetectTypeExt[0x%x]\n",
				rCmdWowlanParam.ucDetectType,
				rCmdWowlanParam.u2DetectTypeExt);

	} else {
		rCmdWowlanParam.ucCmd = PM_WOWLAN_REQ_STOP;
	}

	rStatus = wlanSendSetQueryCmd(prGlueInfo->prAdapter,
				      CMD_ID_SET_WOWLAN,
				      TRUE,
				      FALSE,
				      FALSE,
				      kalWowCmdEventSetCb,
				      nicOidCmdTimeoutCommon,
				      sizeof(struct CMD_WOWLAN_PARAM),
				      (uint8_t *)&rCmdWowlanParam,
				      NULL,
				      0);


	while (1) {
		kalMsleep(5);

		if (wait > 100) {
			DBGLOG(INIT, ERROR, "WoW timeout. WoW:%d\n",
				prGlueInfo->prAdapter->fgSetWowDone);
			break;
		}
		if (prGlueInfo->prAdapter->fgSetWowDone == TRUE) {
			DBGLOG(INIT, STATE, "WoW process done\n");
			break;
		}
		wait++;
	}

}

#if CFG_SUPPORT_MDNS_OFFLOAD
void kalMdnsOffloadInit(struct ADAPTER *prAdapter)
{
	struct MDNS_INFO_T *prMdnsInfo;
	struct MDNS_PARAM_ENTRY_T *prMdnsParamEntry;
	struct MDNS_PASSTHROUGH_ENTRY_T *prMdnsPassthroughEntry;
	struct MDNS_SETTING_FLAGS_T *prMdnsSaveFlags;
	int i = 0;
	int j = 0;

	prMdnsInfo = &prAdapter->rMdnsInfo;
	prMdnsSaveFlags = &prAdapter->rMdnsInfo.rMdnsSaveFlags;
	prMdnsInfo->rMdnsRecordCout = 0;
	prMdnsInfo->rMdnsPassthroughCout = 0;
	LINK_INITIALIZE(&prMdnsInfo->rMdnsRecordFreeList);
	LINK_INITIALIZE(&prMdnsInfo->rMdnsRecordList);
	LINK_INITIALIZE(&prMdnsInfo->rMdnsPassthroughFreeList);
	LINK_INITIALIZE(&prMdnsInfo->rMdnsPassthroughList);

	/* init rMdnsRecordFreeList */
	for (i = 0; i < MAX_MDNS_CACHE_NUM; i++) {
		prMdnsParamEntry = (struct MDNS_PARAM_ENTRY_T *)
				(&prMdnsInfo->rMdnsEntry[i]);
		prMdnsParamEntry->recordKey = (RECORDKEY_BASE - i)
						& RECORDKEY_BASE;
		LINK_INSERT_TAIL(&prMdnsInfo->rMdnsRecordFreeList,
			&prMdnsParamEntry->rLinkEntry);
	}

	/* init rMdnsPassthroughFreeList */
	for (i = 0; i < MAX_MDNS_PASSTHTOUGH_NUM; i++) {
		prMdnsPassthroughEntry = (struct MDNS_PASSTHROUGH_ENTRY_T *)
				(&prMdnsInfo->rMdnsPassthroughEntry[i]);
		LINK_INSERT_TAIL(&prMdnsInfo->rMdnsPassthroughFreeList,
			&prMdnsPassthroughEntry->rLinkEntry);
	}

	/* init mdns saved flags for update before suspend*/
	prMdnsSaveFlags->ucPassthroughBehavior = MDNS_PASSTHROUGH_FORWARD_ALL;
	prMdnsSaveFlags->ucIPV6WakeupFlag = 0;

	/* init the new fields */
	prMdnsInfo->dataBlock.index = 0;
	prMdnsInfo->passrthrough.count = 0;
	prMdnsInfo->currentIndex = 0;

	/* Initialize indices */
	for (i = 0; i < MAX_MDNS_CACHE_NUM; i++) {
		prMdnsInfo->rMdnsRecordIndices[i].ucquerynumber = 0;
		prMdnsInfo->rMdnsRecordIndices[i].u2responseIndex = 0;
		for (j = 0; j < 4; j++) {
			prMdnsInfo->rMdnsRecordIndices[i].u2nameIndex[j] = 0;
			prMdnsInfo->rMdnsRecordIndices[i].u2querytype[j] = 0;
		}
	}

	/* Initialize passrthrough */
	for (i = 0; i < MAX_MDNS_PASSTHTOUGH_NUM; i++)
		prMdnsInfo->passrthrough.nameIndices[i] = 0;

	/* Initialize dataBlock */
	kalMemZero(prMdnsInfo->dataBlock.data,
		sizeof(uint8_t)*MAX_MDNS_USE_SIZE);
}

struct MDNS_PARAM_ENTRY_T *mdnsAllocateParamEntry(struct ADAPTER *prAdapter)
{
	struct MDNS_INFO_T *prMdnsInfo;
	struct MDNS_PARAM_ENTRY_T *prMdnsParamEntry;
	struct LINK *prMdnsRecordFreeList;
	struct LINK *prMdnsRecordList;

	if (prAdapter == NULL) {
		DBGLOG(INIT, ERROR, "NULL point unexpected\n");
		return NULL;
	}

	prMdnsInfo = &prAdapter->rMdnsInfo;

	prMdnsRecordFreeList = &prMdnsInfo->rMdnsRecordFreeList;

	LINK_REMOVE_HEAD(prMdnsRecordFreeList, prMdnsParamEntry,
			struct MDNS_PARAM_ENTRY_T *);

	if (prMdnsParamEntry) {
		kalMemZero(&prMdnsParamEntry->mdns_param,
				sizeof(struct MDNS_PARAM_T));
		kalMemZero(&prMdnsParamEntry->rLinkEntry,
				sizeof(struct LINK_ENTRY));

		prMdnsRecordList = &prMdnsInfo->rMdnsRecordList;

		LINK_INSERT_TAIL(prMdnsRecordList,
				&prMdnsParamEntry->rLinkEntry);
	}

	return prMdnsParamEntry;
}

struct MDNS_PASSTHROUGH_ENTRY_T *mdnsAllocatePassthroughEntry(
	 struct ADAPTER *prAdapter)
{
	struct MDNS_INFO_T *prMdnsInfo;
	struct MDNS_PASSTHROUGH_ENTRY_T *prMdnsPassthroughEntry;
	struct LINK *rMdnsPassthroughFreeList;
	struct LINK *rMdnsPassthroughList;

	if (!prAdapter) {
		DBGLOG(NIC, ERROR, "NULL prAdapter!\n");
		return NULL;
	}

	prMdnsInfo = &prAdapter->rMdnsInfo;
	rMdnsPassthroughFreeList = &prMdnsInfo->rMdnsPassthroughFreeList;

	LINK_REMOVE_HEAD(rMdnsPassthroughFreeList, prMdnsPassthroughEntry,
			struct MDNS_PASSTHROUGH_ENTRY_T *);

	if (prMdnsPassthroughEntry) {
		kalMemZero(&prMdnsPassthroughEntry->mdns_passthrough,
				sizeof(struct MDNS_PASSTHROUGHLIST_T));
		kalMemZero(&prMdnsPassthroughEntry->rLinkEntry,
				sizeof(struct LINK_ENTRY));

		rMdnsPassthroughList = &prMdnsInfo->rMdnsPassthroughList;

		LINK_INSERT_TAIL(rMdnsPassthroughList,
				&prMdnsPassthroughEntry->rLinkEntry);
	}

	return prMdnsPassthroughEntry;
}

void kalSendMdnsEnableToFw(struct GLUE_INFO *prGlueInfo)
{
	struct CMD_MDNS_PARAM_T *cmdMdnsParam;
	struct WLAN_MAC_HEADER_QoS_T *prMacHeader;
	struct iphdr *prIphdr;
	struct udphdr *prUdphdr;
	uint32_t u4BufLen = 0;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	cmdMdnsParam =
		kalMemAlloc(sizeof(struct CMD_MDNS_PARAM_T), PHY_MEM_TYPE);
	if (!cmdMdnsParam) {
		DBGLOG(REQ, WARN, "%s, alloc mem failed\n", __func__);
		return;
	}

	kalMemZero(cmdMdnsParam, sizeof(struct CMD_MDNS_PARAM_T));

	cmdMdnsParam->ucCmd = MDNS_CMD_ENABLE;

	cmdMdnsParam->ucWakeFlag = prGlueInfo->prAdapter->mdns_wake_flag;

	DBGLOG(SW4, STATE, "mDNS Enable wake flag %u.\n",
			cmdMdnsParam->ucWakeFlag);
	/* Store 802.11 MAC header.*/

	prMacHeader = &cmdMdnsParam->aucMdnsMacHdr;
	prMacHeader->u2FrameCtrl = MAC_FRAME_QOS_DATA;
	prMacHeader->u2FrameCtrl |= MASK_FC_TO_DS;
	prMacHeader->u2FrameCtrl |= MASK_FC_PWR_MGT;

	/* SA, DA.*/
	COPY_MAC_ADDR(prMacHeader->aucAddr2,
					prGlueInfo->prDevHandler->dev_addr);
	prMacHeader->aucAddr3[0] = 1;
	prMacHeader->aucAddr3[1] = 0;
	prMacHeader->aucAddr3[2] = 0x5E;
	prMacHeader->aucAddr3[3] = 0;
	prMacHeader->aucAddr3[4] = 0;
	prMacHeader->aucAddr3[5] = 0xFB;

	prMacHeader->u2SeqCtrl = 0;

	prMacHeader->u2QosCtrl = 0;
	prMacHeader->u2QosCtrl |=
					(ACK_POLICY_NORMAL_ACK_IMPLICIT_BA_REQ
					<< MASK_QC_ACK_POLICY_OFFSET);

	/* Fill ip header */
	prIphdr = (struct iphdr *)&cmdMdnsParam->aucMdnsIPHdr[0];
	/* Length:(5*4) */
	prIphdr->ihl = 5;
	prIphdr->version = IP_VERSION_4;
	/* Diff.Services */
	prIphdr->tos = 0;
	/* IP length: will fill in fw
	*(IPV4_HEADER_LENGTH + UDP_HEADER_LENGTH + u4MdnsLen)
	*/
	prIphdr->tot_len = 0;
	prIphdr->id = 0;
	prIphdr->frag_off = 1 << 6;
	prIphdr->ttl = 255;
	/* Protocol, UDP.*/
	prIphdr->protocol = 17;
	prIphdr->check = 0;
	/* Source IP: FW could check and update SIP again.*/
	prIphdr->saddr = 0;
	/* DIP:MC IP(224.0.0.251) */
	prIphdr->daddr = HTONL(224 << 24 | 251);

	/* fill UDP Header */
	prUdphdr = (struct udphdr *)&cmdMdnsParam->aucMdnsUdpHdr[0];
	/* Source port: 5353 */
	prUdphdr->source = HTONS(5353);
	/* Destination port: 5353 */
	prUdphdr->dest = HTONS(5353);
	/* UDP length: Calculate by FW*/
	prUdphdr->len = 0;
	/* UDP CheckSum: Calculate by FW*/
	prUdphdr->check = 0;

	rStatus = kalIoctl(prGlueInfo, wlanoidSetMdnsCmdToFw, cmdMdnsParam,
			   sizeof(struct CMD_MDNS_PARAM_T), &u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS)
		DBGLOG(REQ, ERROR, "set mdns cmd error.\n");

	kalMemFree(cmdMdnsParam, PHY_MEM_TYPE,
		sizeof(struct CMD_MDNS_PARAM_T));
}

uint32_t kalAddMdnsRecord(struct GLUE_INFO *prGlueInfo,
		struct MDNS_INFO_UPLAYER_T *prMdnsUplayerInfo)
{
	struct MDNS_INFO_T *prMdnsInfo;
	struct MDNS_PARAM_ENTRY_T *prMdnsParamEntry;
	struct LINK *prMdnsRecordList;
	uint16_t u2MdnsUsedSize = 0;
	uint16_t u2MaxAvailMdnsSize = 0;
	uint16_t u2UplRecordSize = 0;

	if (prGlueInfo == NULL || prMdnsUplayerInfo == NULL) {
		DBGLOG(REQ, ERROR,
			"prGlueInfo or prMdnsUplayerInfo is null.\n");
		return WLAN_STATUS_FAILURE;
	}

	u2UplRecordSize = kalGetMdnsUplRecSz(prMdnsUplayerInfo);
	u2MdnsUsedSize = kalGetMdnsUsedSize(prGlueInfo);
	u2MaxAvailMdnsSize = kalGetMaxAvailMdnsSize();

	if (u2UplRecordSize + u2MdnsUsedSize > u2MaxAvailMdnsSize) {
		DBGLOG(REQ, ERROR, "mdns add record fail, no enough space\n");
		return WLAN_STATUS_FAILURE;
	}

	prMdnsInfo = &prGlueInfo->prAdapter->rMdnsInfo;
	prMdnsRecordList = &prMdnsInfo->rMdnsRecordList;

	prMdnsParamEntry = mdnsAllocateParamEntry(prGlueInfo->prAdapter);
	if (prMdnsParamEntry == NULL) {
		DBGLOG(REQ, INFO,
			"mdns record buffer is full, add record failed.\n");
		return WLAN_STATUS_FAILURE;
	} else {
		DBGLOG(REQ, INFO, "add mdns record buffer number %u.\n",
				prMdnsRecordList->u4NumElem);
		kalMemCopy(&prMdnsParamEntry->mdns_param,
				&prMdnsUplayerInfo->mdns_param,
				sizeof(struct MDNS_PARAM_T));
		prMdnsInfo->rMdnsRecordCout += 1;
		return prMdnsParamEntry->recordKey;
	}
}

uint32_t kalAddMdnsPassthrough(struct GLUE_INFO *prGlueInfo,
		struct MDNS_INFO_UPLAYER_T *prMdnsUplayerPassthroughInfo)
{
	struct MDNS_INFO_T *prMdnsInfo;
	struct MDNS_PASSTHROUGH_ENTRY_T *prMdnsPassthroughEntry;
	struct LINK *rMdnsPassthroughList;
	int length = 0;
	uint16_t u2MdnsUsedSize = 0;
	uint16_t u2MaxAvailMdnsSize = 0;
	uint16_t u2UplPasstSize = 0;

	if (prGlueInfo == NULL || prMdnsUplayerPassthroughInfo == NULL) {
		DBGLOG(REQ, ERROR,
			"prGlueInfo or prMdnsUplayerPassthroughInfo is null.\n");
		return WLAN_STATUS_FAILURE;
	}

	u2UplPasstSize = kalGetMdnsUplPTSz(prMdnsUplayerPassthroughInfo);
	u2MdnsUsedSize = kalGetMdnsUsedSize(prGlueInfo);
	u2MaxAvailMdnsSize = kalGetMaxAvailMdnsSize();

	if (u2UplPasstSize + u2MdnsUsedSize > u2MaxAvailMdnsSize) {
		DBGLOG(REQ, ERROR, "mdns add passthrough fail, no space\n");
		return WLAN_STATUS_FAILURE;
	}

	prMdnsInfo = &prGlueInfo->prAdapter->rMdnsInfo;
	rMdnsPassthroughList = &prMdnsInfo->rMdnsPassthroughList;

	LINK_FOR_EACH_ENTRY(prMdnsPassthroughEntry, rMdnsPassthroughList,
				rLinkEntry, struct MDNS_PASSTHROUGH_ENTRY_T) {
		if (kalMemCmp(&prMdnsPassthroughEntry->mdns_passthrough.name,
				&prMdnsUplayerPassthroughInfo->name,
				strlen((const char *)
				prMdnsPassthroughEntry->mdns_passthrough.name)
				+ 1) == 0) {
			DBGLOG(REQ, ERROR,
				"mdns passthrough is in the buffer.\n");
			return WLAN_STATUS_FAILURE;
		}
	}

	prMdnsPassthroughEntry =
		mdnsAllocatePassthroughEntry(prGlueInfo->prAdapter);
	if (prMdnsPassthroughEntry == NULL) {
		DBGLOG(REQ, INFO,
			"mdns passthrough buffer is full, add failed.\n");
		return WLAN_STATUS_FAILURE;
	}

	DBGLOG(REQ, INFO,
		"add mdns passthrough buffer number %u.\n",
			rMdnsPassthroughList->u4NumElem);
	length = strlen((const char *)
			prMdnsUplayerPassthroughInfo->name) + 1;

	kalMemCopy(&prMdnsPassthroughEntry->mdns_passthrough.name,
			&prMdnsUplayerPassthroughInfo->name,
			length);
	prMdnsPassthroughEntry->mdns_passthrough.u2PassthroghLength
		= length;
	prMdnsInfo->rMdnsPassthroughCout += 1;
	return WLAN_STATUS_SUCCESS;

}

uint16_t kalGetMdnsUsedSize(struct GLUE_INFO *prGlueInfo)
{
	struct MDNS_INFO_T *prMdnsInfo;
	uint16_t length = 0;
	uint16_t count = 0;
	uint8_t i = 0;

	/* passthrough */
	struct MDNS_PASSTHROUGH_ENTRY_T *prMdnsPassthroughEntry;
	struct LINK *prMdnsPassthroughList;

	/* record */
	struct MDNS_PARAM_ENTRY_T *prMdnsParamEntry;
	struct LINK *prMdnsRecordList;

	struct MDNS_PARAM_T *pMdnsParam = NULL;

	prMdnsInfo = &prGlueInfo->prAdapter->rMdnsInfo;

	prMdnsPassthroughList = &prMdnsInfo->rMdnsPassthroughList;
	prMdnsRecordList = &prMdnsInfo->rMdnsRecordList;

	/*count mdns record used size*/
	LINK_FOR_EACH_ENTRY(prMdnsParamEntry, prMdnsRecordList,
			rLinkEntry, struct MDNS_PARAM_ENTRY_T) {

		pMdnsParam = &prMdnsParamEntry->mdns_param;

		/* query[4] */
		for (i = 0 ; i < MDNS_QURTRY_NUMBER; i++) {
			if (pMdnsParam->query[i].name_length > 0) {
				length =
					pMdnsParam->query[i].name_length + 2;
				count += length;
				if (count > MAX_MDNS_USE_SIZE)
					goto exit;
			}
		}

		/* response */
		if (pMdnsParam->response_len > 0) {
			length =
				pMdnsParam->response_len + 2;
			count += length;
			if (count > MAX_MDNS_USE_SIZE)
				goto exit;
		}

	} /* end of  LINK_FOR_EACH_ENTRY record */

		/*add mdns passthrough used size*/
	LINK_FOR_EACH_ENTRY(prMdnsPassthroughEntry, prMdnsPassthroughList,
			rLinkEntry, struct MDNS_PASSTHROUGH_ENTRY_T) {

		length =
		    prMdnsPassthroughEntry->mdns_passthrough.u2PassthroghLength;
		length += 2;
		count += length;
		if (count > MAX_MDNS_USE_SIZE)
			goto exit;

	} /* end of  LINK_FOR_EACH_ENTRY passthrough  */

exit:
	DBGLOG(REQ, ERROR, "mdns used size %d .\n", count);
	if (count > MAX_MDNS_USE_SIZE)
		DBGLOG(REQ, ERROR,
			"mdns used OVER size %d > %d: [MAX_MDNS_USE_SIZE].\n",
			count, MAX_MDNS_USE_SIZE);
	return count;

}

uint16_t kalGetMaxAvailMdnsSize(void)
{
	return MAX_MDNS_USE_SIZE
		- MAX_MDNS_CACHE_NUM * sizeof(struct MDNS_RECORD_T)
		- sizeof(struct MDNS_PASSTHROUGH_T)
		- sizeof(uint16_t) * 3;
}

uint16_t kalGetMdnsUplRecSz(struct MDNS_INFO_UPLAYER_T *prMdnsUplayerInfo)
{
	struct MDNS_PARAM_T *pMdnsParam = NULL;
	uint16_t count = 0;
	uint16_t length = 0;
	uint8_t i = 0;

	if (prMdnsUplayerInfo == NULL) {
		DBGLOG(REQ, ERROR,
			"prGlueInfo or prMdnsUplayerInfo is null.\n");
		return MAX_MDNS_USE_SIZE;
	}

	pMdnsParam = &prMdnsUplayerInfo->mdns_param;

	/* query[4] */
	for (i = 0 ; i < MDNS_QURTRY_NUMBER; i++) {
		if (pMdnsParam->query[i].name_length > 0) {
			length =
				pMdnsParam->query[i].name_length + 2;
			count += length;
		}
	}

	/* response */
	if (pMdnsParam->response_len > 0) {
		length =
			pMdnsParam->response_len + 2;
		count += length;
	}
	return count;
}
uint16_t kalGetMdnsUplPTSz(struct MDNS_INFO_UPLAYER_T *prMdnsUplayerInfo)
{
	uint16_t count = 0;

	if (prMdnsUplayerInfo == NULL) {
		DBGLOG(REQ, ERROR,
			"prGlueInfo or prMdnsUplayerInfo is null.\n");
		return MAX_MDNS_USE_SIZE;
	}

	count = strlen((const char *)
			prMdnsUplayerInfo->name) + 1 + 2;
	return count;
}

void kalDelMdnsRecord(struct GLUE_INFO *prGlueInfo,
		struct MDNS_INFO_UPLAYER_T *prMdnsUplayerInfo) {
	struct MDNS_INFO_T *prMdnsInfo;
	struct MDNS_PARAM_ENTRY_T *prMdnsParamEntry;
	struct MDNS_PARAM_ENTRY_T *prMdnsParamEntryNext;
	struct LINK *prMdnsRecordList;
	struct LINK *prMdnsRecordFreeList;

	prMdnsInfo = &prGlueInfo->prAdapter->rMdnsInfo;
	prMdnsRecordList = &prMdnsInfo->rMdnsRecordList;
	prMdnsRecordFreeList = &prMdnsInfo->rMdnsRecordFreeList;

	LINK_FOR_EACH_ENTRY_SAFE(prMdnsParamEntry, prMdnsParamEntryNext,
		prMdnsRecordList, rLinkEntry, struct MDNS_PARAM_ENTRY_T) {
		if (kalMemCmp(&prMdnsParamEntry->mdns_param,
			&prMdnsUplayerInfo->mdns_param,
			sizeof(struct MDNS_PARAM_T)) == 0) {
			DBGLOG(REQ, ERROR, "del mdns record.\n");
			LINK_REMOVE_KNOWN_ENTRY(prMdnsRecordList,
				prMdnsParamEntry);
			LINK_INSERT_HEAD(prMdnsRecordFreeList,
				&prMdnsParamEntry->rLinkEntry);
			prMdnsInfo->rMdnsRecordCout -= 1;
		}
	}
}

void kalDelMdnsRecordWithRecordKey(struct GLUE_INFO *prGlueInfo,
		struct MDNS_INFO_UPLAYER_T *prMdnsUplayerInfo)
{
	struct MDNS_INFO_T *prMdnsInfo;
	struct MDNS_PARAM_ENTRY_T *prMdnsParamEntry;
	struct MDNS_PARAM_ENTRY_T *prMdnsParamEntryNext;
	struct LINK *prMdnsRecordList;
	struct LINK *prMdnsRecordFreeList;

	prMdnsInfo = &prGlueInfo->prAdapter->rMdnsInfo;
	prMdnsRecordList = &prMdnsInfo->rMdnsRecordList;
	prMdnsRecordFreeList = &prMdnsInfo->rMdnsRecordFreeList;

	LINK_FOR_EACH_ENTRY_SAFE(prMdnsParamEntry, prMdnsParamEntryNext,
		prMdnsRecordList, rLinkEntry, struct MDNS_PARAM_ENTRY_T) {
		if (prMdnsParamEntry->recordKey ==
			prMdnsUplayerInfo->recordKey) {
			DBGLOG(REQ, ERROR, "del mdns record.\n");
			LINK_REMOVE_KNOWN_ENTRY(prMdnsRecordList,
				prMdnsParamEntry);
			LINK_INSERT_HEAD(prMdnsRecordFreeList,
				&prMdnsParamEntry->rLinkEntry);
			prMdnsInfo->rMdnsRecordCout -= 1;
		}
	}
}

void kalClearMdnsRecord(struct GLUE_INFO *prGlueInfo)
{
	struct MDNS_INFO_T *prMdnsInfo;
	struct MDNS_PARAM_ENTRY_T *prMdnsParamEntry;
	struct MDNS_PARAM_ENTRY_T *prMdnsParamEntryNext;
	struct LINK *prMdnsRecordList;
	struct LINK *prMdnsRecordFreeList;

	prMdnsInfo = &prGlueInfo->prAdapter->rMdnsInfo;
	prMdnsRecordList = &prMdnsInfo->rMdnsRecordList;
	prMdnsRecordFreeList = &prMdnsInfo->rMdnsRecordFreeList;

	LINK_FOR_EACH_ENTRY_SAFE(prMdnsParamEntry, prMdnsParamEntryNext,
		prMdnsRecordList, rLinkEntry, struct MDNS_PARAM_ENTRY_T) {
			DBGLOG(REQ, ERROR, "clear mdns record.\n");
			LINK_REMOVE_KNOWN_ENTRY(prMdnsRecordList,
				prMdnsParamEntry);
			LINK_INSERT_HEAD(prMdnsRecordFreeList,
				&prMdnsParamEntry->rLinkEntry);
	}
	prMdnsInfo->rMdnsRecordCout = 0;
}

void kalShowMdnsRecord(struct GLUE_INFO *prGlueInfo)
{
	struct MDNS_INFO_T *prMdnsInfo;
	struct MDNS_PARAM_ENTRY_T *prMdnsParamEntry;
	struct LINK *prMdnsRecordList;
	int cnt = 0;
	uint8_t i = 0;

	prMdnsInfo = &prGlueInfo->prAdapter->rMdnsInfo;
	prMdnsRecordList = &prMdnsInfo->rMdnsRecordList;

	if (prMdnsInfo->rMdnsRecordCout <= 0) {
		DBGLOG(REQ, ERROR, " no record cnt %d\n",
			prMdnsInfo->rMdnsRecordCout);
		return;
	}

	LINK_FOR_EACH_ENTRY(prMdnsParamEntry, prMdnsRecordList,
				rLinkEntry, struct MDNS_PARAM_ENTRY_T) {
		for (i = 0 ; i < MDNS_QURTRY_NUMBER; i++)
			DBGLOG(REQ, ERROR, "ptr name: len[%u] %s\n",
		prMdnsParamEntry->mdns_param.query[i].name_length,
		prMdnsParamEntry->mdns_param.query[i].name);
		DBGLOG_MEM8(REQ, ERROR, prMdnsParamEntry->mdns_param.response,
			prMdnsParamEntry->mdns_param.response_len);
		cnt++;
	}
	DBGLOG(REQ, ERROR, "record cnt %d\n", cnt);
}

void kalDelMdnsPassthrough(struct GLUE_INFO *prGlueInfo,
		struct MDNS_INFO_UPLAYER_T *prMdnsUplayerPassthroughInfo)
{
	struct MDNS_INFO_T *prMdnsInfo;
	struct MDNS_PASSTHROUGH_ENTRY_T *prMdnsPassthroughEntry;
	struct MDNS_PASSTHROUGH_ENTRY_T *prMdnsPassthroughEntryNext;
	struct LINK *prMdnsPassthroughList;
	struct LINK *prMdnsPassthroughFreeList;

	prMdnsInfo = &prGlueInfo->prAdapter->rMdnsInfo;
	prMdnsPassthroughList = &prMdnsInfo->rMdnsPassthroughList;
	prMdnsPassthroughFreeList = &prMdnsInfo->rMdnsPassthroughFreeList;

	LINK_FOR_EACH_ENTRY_SAFE(prMdnsPassthroughEntry,
		prMdnsPassthroughEntryNext,
		prMdnsPassthroughList, rLinkEntry,
		struct MDNS_PASSTHROUGH_ENTRY_T) {
		if (kalMemCmp(&prMdnsPassthroughEntry->mdns_passthrough.name,
				&prMdnsUplayerPassthroughInfo->name,
				strlen((const char *)
				prMdnsPassthroughEntry->mdns_passthrough.name)
				+ 1) == 0) {
			DBGLOG(REQ, ERROR,
				"del mdns passthrough.\n");
			LINK_REMOVE_KNOWN_ENTRY(prMdnsPassthroughList,
				prMdnsPassthroughEntry);
			LINK_INSERT_HEAD(prMdnsPassthroughFreeList,
				&prMdnsPassthroughEntry->rLinkEntry);
			prMdnsInfo->rMdnsPassthroughCout -= 1;
		}
	}
}

void kalClearMdnsPassthrough(struct GLUE_INFO *prGlueInfo)
{
	struct MDNS_INFO_T *prMdnsInfo;
	struct MDNS_PASSTHROUGH_ENTRY_T *prMdnsPassthroughEntry;
	struct MDNS_PASSTHROUGH_ENTRY_T *prMdnsPassthroughEntryNext;
	struct LINK *prMdnsPassthroughList;
	struct LINK *prMdnsPassthroughFreeList;

	prMdnsInfo = &prGlueInfo->prAdapter->rMdnsInfo;
	prMdnsPassthroughList = &prMdnsInfo->rMdnsPassthroughList;
	prMdnsPassthroughFreeList = &prMdnsInfo->rMdnsPassthroughFreeList;

	LINK_FOR_EACH_ENTRY_SAFE(prMdnsPassthroughEntry,
		prMdnsPassthroughEntryNext,
		prMdnsPassthroughList, rLinkEntry,
		struct MDNS_PASSTHROUGH_ENTRY_T) {
			DBGLOG(REQ, ERROR, "clear mdns passthrough.\n");
			LINK_REMOVE_KNOWN_ENTRY(prMdnsPassthroughList,
				prMdnsPassthroughEntry);
			LINK_INSERT_HEAD(prMdnsPassthroughFreeList,
				&prMdnsPassthroughEntry->rLinkEntry);
	}
	prMdnsInfo->rMdnsPassthroughCout = 0;
}

uint16_t kalMdnsConvettoDataBlock(struct GLUE_INFO *prGlueInfo)
{
	struct MDNS_INFO_T *prMdnsInfo = &prGlueInfo->prAdapter->rMdnsInfo;

	struct LINK *prMdnsRecordList = &prMdnsInfo->rMdnsRecordList;
	struct LINK *prMdnsPassthroughList = &prMdnsInfo->rMdnsPassthroughList;

	struct MDNS_PARAM_ENTRY_T *prMdnsParamEntry;
	struct MDNS_PASSTHROUGH_ENTRY_T *prMdnsPassthroughEntry;

	struct MDNS_RECORD_T *prMdnsRecord = NULL;
	struct MDNS_PARAM_T *pMdnsParam = NULL;

	int recordtype = 0;
	uint16_t length = 0;
	uint8_t *value = NULL;
	uint16_t u2DataIndex = 0;
	uint16_t u2RecordCout = 0;
	uint16_t u2passthrough_count = 0;
	uint8_t i = 0;

	/* Reset currentIndex, passrthrough count, and dataBlock index */
	prMdnsInfo->currentIndex = 0;
	prMdnsInfo->passrthrough.count = 0;
	prMdnsInfo->dataBlock.index = 0;

	/* Loop over the record list */
	LINK_FOR_EACH_ENTRY(prMdnsParamEntry, prMdnsRecordList,
		rLinkEntry, struct MDNS_PARAM_ENTRY_T) {

		prMdnsRecord =
			&prMdnsInfo->rMdnsRecordIndices[u2RecordCout];
		pMdnsParam = &prMdnsParamEntry->mdns_param;

		/* query[4] */
		for (i = 0 ; i < MDNS_QURTRY_NUMBER; i++) {
			if (pMdnsParam->query[i].name_length > 0) {
				recordtype = MDNS_ELEM_TYPE_PTR;
				length =
					pMdnsParam->query[i].name_length;
				value =
					pMdnsParam->query[i].name;

				u2DataIndex = kalMdnsAddToDataBlock(
					&prMdnsInfo->dataBlock, value, length);
				if (u2DataIndex == FAIL_MDNS_OVERSIZE) {
					DBGLOG(REQ, ERROR, "DataBlock over\n");
					return FAIL_MDNS_OVERSIZE;
				}

				/* ucquerynumber count++*/
				prMdnsRecord->ucquerynumber++;

				/* Save the query type  */
				prMdnsRecord->u2querytype[i] =
					pMdnsParam->query[i].type;

				/* Save u2DataIndex in section 0 of indices */
				prMdnsRecord->u2nameIndex[i] = u2DataIndex;
			}
		}

		/* response */
		if (pMdnsParam->response_len > 0) {
			length = pMdnsParam->response_len;
			value = pMdnsParam->response;

			u2DataIndex = kalMdnsAddToDataBlock(
					&prMdnsInfo->dataBlock,
					value, length);
			if (u2DataIndex == FAIL_MDNS_OVERSIZE) {
				DBGLOG(REQ, ERROR, "AddToDataBlock over\n");
				return FAIL_MDNS_OVERSIZE;
			}
			prMdnsRecord->u2responseIndex =
				u2DataIndex;
		}
		/* Ensure u2RecordCout is within 0 to 15 */
		if (u2RecordCout > MAX_MDNS_CACHE_NUM) {
			DBGLOG(REQ, ERROR, "u2RecordCout ou.\n");
			return FAIL_MDNS_OVERSIZE;
		}

		u2RecordCout++;
		prMdnsInfo->currentIndex = u2RecordCout;

	} /* end of LINK_FOR_EACH_ENTRY*/

	/* Loop over the pass through list */
	LINK_FOR_EACH_ENTRY(prMdnsPassthroughEntry, prMdnsPassthroughList,
		rLinkEntry, struct MDNS_PASSTHROUGH_ENTRY_T) {
		length =
		    prMdnsPassthroughEntry->mdns_passthrough.u2PassthroghLength;
		value =
		    prMdnsPassthroughEntry->mdns_passthrough.name;

		u2DataIndex = kalMdnsAddToDataBlock(&prMdnsInfo->dataBlock,
				value, length);
			if (u2DataIndex == FAIL_MDNS_OVERSIZE) {
				DBGLOG(REQ, ERROR, "AddToDataBlock over\n");
				return FAIL_MDNS_OVERSIZE;
			}
		/* Save u2DataIndex and increment passrthrough count */
		prMdnsInfo->passrthrough.nameIndices[u2passthrough_count++] =
			u2DataIndex;
		prMdnsInfo->passrthrough.count = u2passthrough_count;

		/* Ensure count is within bounds of nameIndices array */
		if (prMdnsInfo->passrthrough.count >=
				MAX_MDNS_PASSTHTOUGH_NUM) {
		/* Handle error case where passrthrough over size */
			DBGLOG(REQ, ERROR,
			"Passrthrough nameIndices array beyond MAX_MDNS_PASSTHTOUGH_NUM.\n");
			break;
		}
	}
	DBGLOG(REQ, ERROR, "currentIndex:%d\n",
		prMdnsInfo->currentIndex);
	DBGLOG(REQ, ERROR, "prMdnsInfo->passrthrough.count:%d\n",
		prMdnsInfo->passrthrough.count);
	DBGLOG(REQ, ERROR, "prMdnsInfo->dataBlock.index:%d\n",
		prMdnsInfo->dataBlock.index);

	return u2DataIndex;
}

void kalShowMdnsPassthrough(struct GLUE_INFO *prGlueInfo)
{
	struct MDNS_INFO_T *prMdnsInfo;
	struct MDNS_PASSTHROUGH_ENTRY_T *prMdnsPassthroughEntry;
	struct LINK *prMdnsPassthroughList;
	int cnt = 0;

	prMdnsInfo = &prGlueInfo->prAdapter->rMdnsInfo;
	prMdnsPassthroughList = &prMdnsInfo->rMdnsPassthroughList;

	if (prMdnsInfo->rMdnsPassthroughCout <= 0) {
		DBGLOG(REQ, ERROR, " no passthrough count %d\n",
			prMdnsInfo->rMdnsPassthroughCout);
		return;
	}

	LINK_FOR_EACH_ENTRY(prMdnsPassthroughEntry, prMdnsPassthroughList,
				rLinkEntry, struct MDNS_PASSTHROUGH_ENTRY_T) {
		DBGLOG(REQ, ERROR, "passthrough name: %s\n",
			prMdnsPassthroughEntry->mdns_passthrough.name);
		cnt++;
	}
	DBGLOG(REQ, ERROR, "passthrough cnt %d\n", cnt);
}

void kalSendMdnsDisableToFw(struct GLUE_INFO *prGlueInfo)
{
	struct CMD_MDNS_PARAM_T *cmdMdnsParam;
	uint32_t u4BufLen = 0;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	cmdMdnsParam =
		kalMemAlloc(sizeof(struct CMD_MDNS_PARAM_T), PHY_MEM_TYPE);
	if (!cmdMdnsParam) {
		DBGLOG(REQ, WARN, "%s, alloc mem failed\n", __func__);
		return;
	}

	kalMemZero(cmdMdnsParam, sizeof(struct CMD_MDNS_PARAM_T));

	cmdMdnsParam->ucCmd = MDNS_CMD_DISABLE;

	DBGLOG(SW4, STATE, "mDNS disable.\n");

	rStatus = kalIoctl(prGlueInfo, wlanoidSetMdnsCmdToFw, cmdMdnsParam,
			   sizeof(struct CMD_MDNS_PARAM_T), &u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS)
		DBGLOG(REQ, ERROR, "set mdns cmd error.\n");

	kalMemFree(cmdMdnsParam, PHY_MEM_TYPE,
		sizeof(struct CMD_MDNS_PARAM_T));
}

uint32_t kalGetAndResetHitCounterToFw(struct GLUE_INFO *prGlueInfo,
		int recordKey)
{
	struct CMD_MDNS_PARAM_T *cmdMdnsParam;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen;
	uint32_t u4Hit = -1;

	cmdMdnsParam =
		kalMemAlloc(sizeof(struct CMD_MDNS_PARAM_T), PHY_MEM_TYPE);
	if (!cmdMdnsParam) {
		DBGLOG(REQ, WARN, "%s, alloc mem failed\n", __func__);
		return u4Hit;
	}

	kalMemZero(cmdMdnsParam, sizeof(struct CMD_MDNS_PARAM_T));

	cmdMdnsParam->ucCmd = MDNS_CMD_GET_HITCOUNTER;
	cmdMdnsParam->ucRecordId = RECORDKEY_BASE-recordKey + 1;

	DBGLOG(SW4, STATE, "mDNS gethit.\n");

	rStatus = kalIoctl(prGlueInfo,
						wlanoidGetMdnsHitMiss,
						cmdMdnsParam,
						sizeof(struct CMD_MDNS_PARAM_T),
						&u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, ERROR, "wlanoidGetMdnsHitMiss error.\n");
		return u4Hit;
	}

	kalMemFree(cmdMdnsParam, PHY_MEM_TYPE,
		sizeof(struct CMD_MDNS_PARAM_T));

	u4Hit = prGlueInfo->prAdapter->rMdnsInfo.rMdnsRecordEvent.u4MdnsHitMiss;
	return u4Hit;

}

uint32_t kalGetAndResetMissCounterToFw(struct GLUE_INFO *prGlueInfo)
{
	struct CMD_MDNS_PARAM_T *cmdMdnsParam;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen;
	uint32_t u4Miss = -1;

	cmdMdnsParam =
		kalMemAlloc(sizeof(struct CMD_MDNS_PARAM_T), PHY_MEM_TYPE);
	if (!cmdMdnsParam) {
		DBGLOG(REQ, WARN, "%s, alloc mem failed\n", __func__);
		return u4Miss;
	}

	kalMemZero(cmdMdnsParam, sizeof(struct CMD_MDNS_PARAM_T));

	cmdMdnsParam->ucCmd = MDNS_CMD_GET_MISSCOUNTER;
	DBGLOG(SW4, STATE, "mDNS getmiss.\n");


	rStatus = kalIoctl(prGlueInfo,
						wlanoidGetMdnsHitMiss,
						cmdMdnsParam,
						sizeof(struct CMD_MDNS_PARAM_T),
						&u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, ERROR, "wlanoidGetMdnsHitMiss error.\n");
		return u4Miss;
	}

	kalMemFree(cmdMdnsParam, PHY_MEM_TYPE,
		sizeof(struct CMD_MDNS_PARAM_T));

	u4Miss =
		prGlueInfo->prAdapter->rMdnsInfo.rMdnsRecordEvent.u4MdnsHitMiss;
	return u4Miss;

}

void kalSendMdnsFlagsToFw(struct GLUE_INFO *prGlueInfo)
{
	struct CMD_MDNS_PARAM_T *cmdMdnsParam;
	struct MDNS_INFO_T *prMdnsInfo;
	struct MDNS_SETTING_FLAGS_T *prMdnsSaveFlags;
	uint32_t u4BufLen = 0;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint16_t u2DataCurrentIndex = 0;
	uint8_t *ucPayloadAssemble = NULL;
	uint8_t i = 0;
	uint8_t j = 0;

	uint16_t u2totalSize = 0;
	/* Amount of data left to copy into cmdMdnsParam->ucpayload */
	uint16_t u2RemainSize = 0;
	/* Maximum size per chunk is 1024 bytes */
	uint16_t u2MaxPayloadSize = 0;
	/* Amount of data already copied into cmdMdnsParam->ucpayload */
	uint16_t u2CopiedSize = 0;
    /* Calculate the size of the current chunk */
	uint16_t u2ChunkSize = 0;

	prMdnsInfo = &prGlueInfo->prAdapter->rMdnsInfo;
	prMdnsSaveFlags = &prMdnsInfo->rMdnsSaveFlags;

	cmdMdnsParam = kalMemAlloc(sizeof(struct CMD_MDNS_PARAM_T),
		PHY_MEM_TYPE);
	if (!cmdMdnsParam) {
		DBGLOG(REQ, WARN, "%s, alloc CMD_MDNS_PARAM_T mem failed.\n",
			__func__);
		return;
	}

	ucPayloadAssemble = kalMemAlloc(sizeof(uint8_t)*MAX_MDNS_USE_SIZE,
		PHY_MEM_TYPE);
	if (!ucPayloadAssemble) {
		DBGLOG(REQ, WARN, "%s, alloc ucPayloadAssemble mem failed.\n",
			__func__);
		 kalMemFree(cmdMdnsParam, PHY_MEM_TYPE,
			sizeof(struct CMD_MDNS_PARAM_T));
		return;
	}

	kalMemZero(cmdMdnsParam, sizeof(struct CMD_MDNS_PARAM_T));
	cmdMdnsParam->ucCmd = MDNS_CMD_SET_IPV6_WAKEUP_FLAG;

	cmdMdnsParam->ucWakeFlag = prGlueInfo->prAdapter->mdns_wake_flag;
	cmdMdnsParam->ucPassthroughBehavior =
		prMdnsSaveFlags->ucPassthroughBehavior;
	cmdMdnsParam->ucIPV6WakeupFlag = prMdnsSaveFlags->ucIPV6WakeupFlag;


	/*Clear rMdnsRecordIndices */
	/* Initialize indices */
	for (i = 0; i < MAX_MDNS_CACHE_NUM; i++) {
		prMdnsInfo->rMdnsRecordIndices[i].ucquerynumber = 0;
		prMdnsInfo->rMdnsRecordIndices[i].u2responseIndex = 0;
		for (j = 0; j < 4; j++) {
			prMdnsInfo->rMdnsRecordIndices[i].u2nameIndex[j] = 0;
			prMdnsInfo->rMdnsRecordIndices[i].u2querytype[j] = 0;
		}
	}

	/* Add dataBlock, indices, and passrthrough*/
	/* into payload of cmdMdnsParam */
	kalMdnsConvettoDataBlock(prGlueInfo);

	/* Prepare the payload of cmdMdnsParam: first is dataBlock */
	u2DataCurrentIndex =
		kalMdnsCopyDataToPayload(&prMdnsInfo->dataBlock,
			ucPayloadAssemble, u2DataCurrentIndex);
	/* Then copy indices array */
	u2DataCurrentIndex =
		kalMdnsCopyRecordToPayload(prMdnsInfo->rMdnsRecordIndices,
			prMdnsInfo->currentIndex, ucPayloadAssemble,
			u2DataCurrentIndex);
	/* Finally, copy the passrthrough */
	u2DataCurrentIndex =
		kalMdnsCopyPassToPayload(&prMdnsInfo->passrthrough,
			ucPayloadAssemble, u2DataCurrentIndex);

	DBGLOG(REQ, WARN, "u2DataCurrentIndex:%d.\n", u2DataCurrentIndex);

	/* Amount of data already copied  */
	/*is indicated by u2DataCurrentIndex */
	u2totalSize = u2DataCurrentIndex;
	/* Amount of data left to copy  */
	u2RemainSize = u2totalSize;
	/* Maximum size per chunk is 1024 bytes */
	u2MaxPayloadSize = MAX_MDNS_TRANSFER_SIZE;
	/* Amount of data already copied  */
	u2CopiedSize = 0;

	cmdMdnsParam->u2PayloadTotallength = u2totalSize;

	while (u2RemainSize > 0) {
		/* Calculate the size of the current chunk */
		u2ChunkSize = u2RemainSize > u2MaxPayloadSize ?
			u2MaxPayloadSize : u2RemainSize;

		DBGLOG(REQ, INFO, "u2ChunkSize:%d.\n", u2ChunkSize);
		/* Clear cmdMdnsParam->ucpayload */
		memset(cmdMdnsParam->ucPayload, 0, u2MaxPayloadSize);

		/* Copy a chunk of data to cmdMdnsParam->ucpayload */
		memcpy(cmdMdnsParam->ucPayload,
			ucPayloadAssemble + u2CopiedSize, u2ChunkSize);

		/* Set the chunk order */
		cmdMdnsParam->ucPayloadOrder = u2CopiedSize / u2MaxPayloadSize;

		DBGLOG(REQ, INFO, "cmdMdnsParam->ucPayloadOrder:%d.\n",
			cmdMdnsParam->ucPayloadOrder);

		/* Then send it to the firmware */
		rStatus = kalIoctl(prGlueInfo, wlanoidSetMdnsCmdToFw,
			cmdMdnsParam,
			sizeof(struct CMD_MDNS_PARAM_T),
			&u4BufLen);

		if (rStatus != WLAN_STATUS_SUCCESS)
			break;
		/* Update the copied size and remaining size */
		u2CopiedSize += u2ChunkSize;
		u2RemainSize -= u2ChunkSize;
	}
	DBGLOG(REQ, INFO, "u2CopiedSize:%d.\n", u2CopiedSize);
	DBGLOG(REQ, INFO, "u2RemainSize:%d.\n", u2RemainSize);

	kalMemFree(cmdMdnsParam, PHY_MEM_TYPE,
		sizeof(struct CMD_MDNS_PARAM_T));
	kalMemFree(ucPayloadAssemble, PHY_MEM_TYPE,
		sizeof(uint8_t) * MAX_MDNS_USE_SIZE);
}

uint16_t kalMdnsCopyDataToPayload(struct MDNS_DATABLOCK_T  *dataBlock,
	uint8_t *payload, uint16_t start)
{

	uint16_t dataBlockLength = dataBlock->index;
	uint16_t i = 0;

	/* Include the length of dataBlock */
	payload[start] = (dataBlockLength >> 8) & 0xFF;
	payload[start + 1] = dataBlockLength & 0xFF;

	/* Copy the data of dataBlock into payload */
	for (i = 0; i < dataBlockLength; i++)
		payload[start + 2 + i] = dataBlock->data[i];

	return (start + 2 + dataBlockLength);
}

uint16_t kalMdnsCopyRecordToPayload(struct MDNS_RECORD_T *prMdnsRecordIndices,
	uint16_t indexCount, uint8_t *payload, uint16_t start)
{
	/* Calculate the length of indices array */
	uint16_t u2indicesLength = indexCount * sizeof(struct MDNS_RECORD_T);
	uint16_t i = 0;
	struct MDNS_RECORD_T *mdns_current = NULL;

	/* Include the length of indices array */
	payload[start] = (u2indicesLength >> 8) & 0xFF;
	payload[start+1] = u2indicesLength & 0xFF;

	/* Copy each index into payload */
	mdns_current = prMdnsRecordIndices;
	for (i = 0; i < indexCount; i++) {
		memcpy((payload + start + 2 + i * sizeof(struct MDNS_RECORD_T)),
			mdns_current, sizeof(struct MDNS_RECORD_T));
		mdns_current++;
	}

	return(start + 2 + u2indicesLength);
}

uint16_t kalMdnsCopyPassToPayload(struct MDNS_PASSTHROUGH_T *passrthrough,
	uint8_t *payload, uint16_t start)
{
	/* Calculate the length of passrthrough */
	uint16_t passrthroughLength =
		passrthrough->count * sizeof(uint16_t);
	uint16_t return_value = 0;

	/* Include the length of passrthrough */
	/*  High byte of length */
	payload[start] = (passrthroughLength >> 8) & 0xFF;
	/*  Low byte of length*/
	payload[start+1] = passrthroughLength & 0xFF;

	memcpy((payload + start + 2),
			passrthrough->nameIndices, passrthroughLength);


	return_value = start + 2 + passrthroughLength;
	return return_value;
}

uint16_t kalMdnsAddToDataBlock(struct MDNS_DATABLOCK_T *dataBlock,
	uint8_t *data, uint16_t dataLength)
{
	uint16_t currentIndex;
	uint16_t existingIndex = 0;
	uint16_t tempLength;
	uint16_t i = 0;

	if (dataBlock == NULL) {
		DBGLOG(REQ, ERROR, "dataBlock is NULL.\n");
		return FAIL_MDNS_OVERSIZE;
	}
	currentIndex = dataBlock->index;
	/* Check whether the data already exists in the data block */
	while (existingIndex < currentIndex) {
		/* Check if existingIndex is valid */
		if (existingIndex >= MAX_MDNS_USE_SIZE - 2)
			return FAIL_MDNS_OVERSIZE;

		tempLength =
			(dataBlock->data[existingIndex] << 8) +
			dataBlock->data[existingIndex + 1];

		/* Add the overflow checking */
		if (existingIndex + 2 + tempLength >= MAX_MDNS_USE_SIZE)
			return FAIL_MDNS_OVERSIZE;

		if (tempLength == dataLength &&
			memcmp(data, &dataBlock->data[existingIndex + 2],
				   dataLength) == 0) {
			return existingIndex;
		}
		existingIndex += tempLength + 2;
	}

	/* Check if the addition of new data will overflow the buffer */
	if (currentIndex + dataLength + 2 >= MAX_MDNS_USE_SIZE)
		return FAIL_MDNS_OVERSIZE;

	dataBlock->data[currentIndex++] = (dataLength >> 8) & 0xFF;
	dataBlock->data[currentIndex++] = dataLength & 0xFF;

	if (currentIndex + dataLength >= MAX_MDNS_USE_SIZE)
		return FAIL_MDNS_OVERSIZE;

	for (i = 0; i < dataLength; i++) {
		if (currentIndex >= MAX_MDNS_USE_SIZE)
			return FAIL_MDNS_OVERSIZE;
		dataBlock->data[currentIndex++] = data[i];
	}

	dataBlock->index = currentIndex;

	/* return the starting index of the new data */
	return currentIndex - dataLength - 2;
}

uint32_t kalMdnsProcess(struct GLUE_INFO *prGlueInfo,
		 struct MDNS_INFO_UPLAYER_T *prMdnsUplayerInfo)
{
	struct MDNS_INFO_T *prMdnsInfo;
	struct MDNS_SETTING_FLAGS_T *prMdnsSaveFlags;
	uint32_t u4Ret = WLAN_STATUS_SUCCESS;

	prMdnsInfo = &prGlueInfo->prAdapter->rMdnsInfo;
	prMdnsSaveFlags = &prMdnsInfo->rMdnsSaveFlags;

	if (prMdnsUplayerInfo->ucCmd == MDNS_CMD_ENABLE) {
		if (prGlueInfo->prAdapter->mdns_offload_enable == FALSE) {
			DBGLOG(REQ, INFO, "mDNS Enable.\n");
			prGlueInfo->prAdapter->mdns_offload_enable = TRUE;
			kalSendMdnsEnableToFw(prGlueInfo);
		} else
			DBGLOG(REQ, STATE, "mDNS already Enable.\n");
	}

	else if (prMdnsUplayerInfo->ucCmd == MDNS_CMD_ADD_RECORD) {
		DBGLOG(REQ, INFO, "Add Record.\n");
		u4Ret = kalAddMdnsRecord(prGlueInfo, prMdnsUplayerInfo);
	}

	else if (prMdnsUplayerInfo->ucCmd == MDNS_CMD_DEL_RECORD) {
		DBGLOG(REQ, INFO, "Delete Record.\n");
		kalDelMdnsRecordWithRecordKey(prGlueInfo,
			prMdnsUplayerInfo);
	}

	else if (prMdnsUplayerInfo->ucCmd == MDNS_CMD_DISABLE) {
		DBGLOG(REQ, INFO, "mdns disable.\n");
		prGlueInfo->prAdapter->mdns_offload_enable = FALSE;
		kalSendMdnsDisableToFw(prGlueInfo);
	}

	else if (prMdnsUplayerInfo->ucCmd == MDNS_CMD_SET_PASSTHTOUGH) {
		DBGLOG(REQ, INFO, "mdns set passthrough behavior.\n");
		prMdnsSaveFlags->ucPassthroughBehavior =
			prMdnsUplayerInfo->passthroughBehavior;
	}

	else if (prMdnsUplayerInfo->ucCmd == MDNS_CMD_ADD_PASSTHTOUGH) {
		DBGLOG(REQ, INFO, "add  passthrough.\n");
		kalAddMdnsPassthrough(prGlueInfo,
			prMdnsUplayerInfo);
	}

	else if (prMdnsUplayerInfo->ucCmd == MDNS_CMD_DEL_PASSTHTOUGH) {
		DBGLOG(REQ, INFO, "del  passthrough.\n");
		kalDelMdnsPassthrough(prGlueInfo,
			prMdnsUplayerInfo);
	}

	else if (prMdnsUplayerInfo->ucCmd == MDNS_CMD_GET_HITCOUNTER) {
		DBGLOG(REQ, INFO, "get hit count and reset to 0\n");
		return kalGetAndResetHitCounterToFw(prGlueInfo,
			prMdnsUplayerInfo->recordKey);
	}

	else if (prMdnsUplayerInfo->ucCmd == MDNS_CMD_GET_MISSCOUNTER) {
		DBGLOG(REQ, INFO, "get miss count and reset to 0\n");
		return kalGetAndResetMissCounterToFw(prGlueInfo);
	}

	else if (prMdnsUplayerInfo->ucCmd == MDNS_CMD_RESETALL) {
		DBGLOG(REQ, INFO, "reset all .\n");
		kalClearMdnsRecord(prGlueInfo);
		kalClearMdnsPassthrough(prGlueInfo);
	}

	else if (prMdnsUplayerInfo->ucCmd == MDNS_CMD_SET_IPV6_WAKEUP_FLAG) {
		DBGLOG(REQ, INFO, "mdns set IPV6WakeupFlag = %d.\n",
			prMdnsUplayerInfo->ucIPV6WakeupFlag);
		prMdnsSaveFlags->ucIPV6WakeupFlag =
			prMdnsUplayerInfo->ucIPV6WakeupFlag;
	}


#if CFG_SUPPORT_MDNS_OFFLOAD_GVA
	/* only for gva, need to remove record when ttl is 0 */
	else if (prMdnsUplayerInfo->ucCmd == MDNS_CMD_DEL_RECORD) {
		DBGLOG(REQ, INFO, "DEL Record.\n");
		kalDelMdnsRecord(prGlueInfo, prMdnsUplayerInfo);
	}
#endif

	return u4Ret;
}

#if CFG_SUPPORT_MDNS_OFFLOAD_GVA
uint16_t kalMdsnParseName(uint8_t *pucMdnsHdr, uint16_t offset,
			uint8_t *name, uint16_t *name_len)
{
	uint16_t pos = 0;
	uint16_t i = 0;
	bool compress = FALSE;
	uint8_t len = 0;
	uint8_t limit = MDNS_QUESTION_NAME_MAX_LEN - 1;

	/* parse compress name */
	/* there may be compress name contain compress name */
	pos = offset;
	while (pucMdnsHdr[offset] != 0) {
		len = pucMdnsHdr[offset];
		switch (len & 0xc0) {
		case 0x00:
			/* uncompress code */
			if ((*name_len + len) > limit) {
				*name_len = 0;
				return 0;
			}

			for (i = 0; i <= len; i++) {
				name[*name_len] = pucMdnsHdr[offset + i];
				*name_len += 1;
			}
			offset += len + 1;
			if (compress == FALSE)
				pos = offset;
			break;

		case 0x40:
		case 0x80:
			*name_len = 0;
			return 0;

		case 0xc0:
			if (compress == FALSE) {
				pos = offset;
				compress = TRUE;
			}
			DBGLOG(PF, LOUD, "0x%02x 0x%02x.\n",
				pucMdnsHdr[offset], pucMdnsHdr[offset + 1]);
			offset = ((pucMdnsHdr[offset] & 0x3f) << 8)
						| pucMdnsHdr[offset + 1];
			break;
		}
	}

	/* for compress, c0xx 2byes, for uncompress 0 as the end of the name */
	if (compress == TRUE)
		pos += 2;
	else
		pos += 1;

	*name_len += 1;

	return pos;
}

void kalProcessMdnsRespPkt(struct GLUE_INFO *prGlueInfo, uint8_t *pucMdnsHdr)
{
	uint8_t  i;
	uint16_t pos;
	uint16_t usMdnsId, usMdnsFlags;
	uint16_t usQuestionCnt, usAnswerCnt, usAuthCnt, usAddtionCnt;
	uint32_t ttl;
	uint16_t type, cl, dataLen;
	uint8_t  domName[MDNS_QUESTION_NAME_MAX_LEN];
	uint16_t domNameLen;
	struct WLAN_MDNS_HDR_T *prMdnsHdr;
	struct MDNS_INFO_UPLAYER_T *prMdnsUplayerInfo = NULL;
	struct MDNS_PARAM_T *prMdnsParam;
	struct MDNS_TEMPLATE_T *prMdnsTemplate;

	prMdnsUplayerInfo =
		kalMemAlloc(sizeof(struct MDNS_INFO_UPLAYER_T), PHY_MEM_TYPE);
	if (!prMdnsUplayerInfo) {
		DBGLOG(REQ, WARN, "%s, alloc mem failed\n", __func__);
		return;
	}

	kalMemZero(prMdnsUplayerInfo, sizeof(struct MDNS_INFO_UPLAYER_T));

	pos = 0;

	prMdnsHdr = (struct WLAN_MDNS_HDR_T *)pucMdnsHdr;
	usMdnsId = NTOHS(prMdnsHdr->usMdnsId);
	usMdnsFlags = NTOHS(prMdnsHdr->usMdnsFlags);
	usQuestionCnt = NTOHS(prMdnsHdr->usQuestionCnt);
	usAnswerCnt = NTOHS(prMdnsHdr->usAnswerCnt);
	usAuthCnt = NTOHS(prMdnsHdr->usAuthCnt);
	usAddtionCnt = NTOHS(prMdnsHdr->usAddtionCnt);
	pos +=  sizeof(struct WLAN_MDNS_HDR_T);

	DBGLOG(SW4, LOUD, "MDNS PKT[ID %u] flags[0x%x]\n",
			usMdnsId, usMdnsFlags);
	DBGLOG(SW4, LOUD, "questionCnt[%u] answerCnt[%u]\n",
			usQuestionCnt, usAnswerCnt);
	DBGLOG(SW4, LOUD, "authcnt[%u] addCnt[%u]\n",
			usAuthCnt, usAddtionCnt);

	if ((usMdnsFlags & 0x8000) == 0) {
		DBGLOG(SW4, LOUD, "MDNS is query packet\n");
		goto exit;
	}

	if ((usAnswerCnt == 1 && usAddtionCnt == 3)
		|| (usAnswerCnt == 4 && usAddtionCnt == 0)) {
		DBGLOG(SW4, LOUD, "MDNS ans %u add %u\n",
			usAnswerCnt, usAddtionCnt);
	} else {
		DBGLOG(SW4, LOUD, "MDNS ans %u add %u\n",
			usAnswerCnt, usAddtionCnt);
		goto exit;
	}

	prMdnsParam = &prMdnsUplayerInfo->mdns_param;

	for (i = 0; i < MDNS_QURTRY_NUMBER; i++) {
		kalMemZero(domName, MDNS_QUESTION_NAME_MAX_LEN);
		domNameLen = 0;

		pos = kalMdsnParseName(pucMdnsHdr, pos,
							domName,
							&domNameLen);

		type = NTOHS(*(uint16_t *)(pucMdnsHdr + pos));
		pos += MDNS_PAYLOAD_TYPE_LEN;

		cl = NTOHS(*(uint16_t *)(pucMdnsHdr + pos));
		pos += MDNS_PAYLOAD_CLASS_LEN;

		ttl = NTOHL(*(uint32_t *)(pucMdnsHdr + pos));
		pos += MDNS_PAYLOAD_TTL_LEN;

		dataLen = NTOHS(*(uint16_t *)(pucMdnsHdr + pos));
		pos += MDNS_PAYLOAD_DATALEN_LEN;

		if (type == &prMdnsParam->query[i].type)
			prMdnsTemplate = &prMdnsParam->query[i];

		switch (type) {
		case MDNS_ELEM_TYPE_PTR:
			if (ttl != 0)
				prMdnsUplayerInfo->ucCmd = MDNS_CMD_ADD_RECORD;
			else
				prMdnsUplayerInfo->ucCmd = MDNS_CMD_DEL_RECORD;
			break;
		default:
			DBGLOG(SW4, LOUD, "not needed type %u\n", type);
			goto exit;
		}

		kalMemCopy(prMdnsTemplate->name, domName, domNameLen);
		prMdnsTemplate->name_length = domNameLen;
		prMdnsTemplate->type = type;
		prMdnsTemplate->class = cl;
		pos += dataLen;
	}

	kalMemCopy(prMdnsParam->response, pucMdnsHdr, pos);
	prMdnsParam->response_len = pos;

	kalMdnsProcess(prGlueInfo, prMdnsUplayerInfo);

exit:
	kalMemFree(prMdnsUplayerInfo, PHY_MEM_TYPE,
		sizeof(struct MDNS_INFO_UPLAYER_T));
}
#endif /* CFG_SUPPORT_MDNS_OFFLOAD_GVA */
#endif /* CFG_SUPPORT_MDNS_OFFLOAD */
#endif /* CFG_WOW_SUPPORT */

#if CFG_SUPPORT_MULTITHREAD
void kalFreeTxMsduWorker(struct work_struct *work)
{
	struct GLUE_INFO *prGlueInfo;
	struct ADAPTER *prAdapter;
	struct QUE rTmpQue;
	struct QUE *prTmpQue = &rTmpQue;
	struct MSDU_INFO *prMsduInfo;

	if (g_u4HaltFlag)
		return;

	prGlueInfo = CONTAINER_OF(work, struct GLUE_INFO,
				  rTxMsduFreeWork);
	prAdapter = prGlueInfo->prAdapter;

	if (test_bit(GLUE_FLAG_HALT_BIT, &prGlueInfo->ulFlag))
		return;

	KAL_ACQUIRE_MUTEX(prAdapter, MUTEX_TX_DATA_DONE_QUE);
	QUEUE_MOVE_ALL(prTmpQue, &prAdapter->rTxDataDoneQueue);
	KAL_RELEASE_MUTEX(prAdapter, MUTEX_TX_DATA_DONE_QUE);

	while (QUEUE_IS_NOT_EMPTY(prTmpQue)) {
		QUEUE_REMOVE_HEAD(prTmpQue, prMsduInfo, struct MSDU_INFO *);
		if (prMsduInfo) {
			nicTxFreePacket(prAdapter, prMsduInfo, FALSE);
			nicTxReturnMsduInfo(prAdapter, prMsduInfo);
		}
	}
}

void kalFreeTxMsdu(struct ADAPTER *prAdapter,
		   struct MSDU_INFO *prMsduInfo)
{

	KAL_ACQUIRE_MUTEX(prAdapter, MUTEX_TX_DATA_DONE_QUE);
	QUEUE_INSERT_TAIL(&prAdapter->rTxDataDoneQueue, prMsduInfo);
	KAL_RELEASE_MUTEX(prAdapter, MUTEX_TX_DATA_DONE_QUE);

	schedule_work(&prAdapter->prGlueInfo->rTxMsduFreeWork);
}
#endif
int32_t kalHaltLock(uint32_t waitMs)
{
	int32_t i4Ret = 0;
	struct GLUE_INFO *prGlueInfo = NULL;

	if (waitMs) {
		i4Ret = down_timeout(&rHaltCtrl.lock,
				     MSEC_TO_JIFFIES(waitMs));
		if (!i4Ret)
			goto success;
		if (i4Ret != -ETIME)
			return i4Ret;

		prGlueInfo = wlanGetGlueInfo();
		if (rHaltCtrl.fgHeldByKalIoctl) {
			DBGLOG(INIT, ERROR,
			       "kalIoctl was executed longer than %u ms, show backtrace of tx_thread!\n",
			       kalGetTimeTick() - rHaltCtrl.u4HoldStart);
			if (prGlueInfo)
				kal_show_stack(prGlueInfo->prAdapter,
					prGlueInfo->main_thread, NULL);
		} else {
			DBGLOG(INIT, ERROR,
			       "halt lock held by %s pid %d longer than %u ms!\n",
			       rHaltCtrl.owner->comm, rHaltCtrl.owner->pid,
			       kalGetTimeTick() - rHaltCtrl.u4HoldStart);
			if (prGlueInfo)
				kal_show_stack(prGlueInfo->prAdapter,
					rHaltCtrl.owner, NULL);
		}
		return i4Ret;
	}
	down(&rHaltCtrl.lock);
success:
	rHaltCtrl.owner = current;
	rHaltCtrl.u4HoldStart = kalGetTimeTick();
	return 0;
}

int32_t kalHaltTryLock(void)
{
	int32_t i4Ret = 0;

	i4Ret = down_trylock(&rHaltCtrl.lock);
	if (i4Ret)
		return i4Ret;
	rHaltCtrl.owner = current;
	rHaltCtrl.u4HoldStart = kalGetTimeTick();
	return 0;
}

void kalHaltUnlock(void)
{
	if (kalGetTimeTick() - rHaltCtrl.u4HoldStart >
	    WLAN_OID_TIMEOUT_THRESHOLD * 2 &&
	    rHaltCtrl.owner)
		DBGLOG(INIT, ERROR,
		       "process %s pid %d hold halt lock longer than 4s!\n",
		       rHaltCtrl.owner->comm, rHaltCtrl.owner->pid);
	rHaltCtrl.owner = NULL;
	up(&rHaltCtrl.lock);
}

void kalSetHalted(u_int8_t fgHalt)
{
	rHaltCtrl.fgHalt = fgHalt;
}

u_int8_t kalIsHalted(void)
{
	return rHaltCtrl.fgHalt;
}


#if 0
void kalPerMonDump(struct GLUE_INFO *prGlueInfo)
{
	struct PERF_MONITOR *prPerMonitor;

	prPerMonitor = &prGlueInfo->prAdapter->rPerMonitor;
	DBGLOG(SW4, WARN, "ulPerfMonFlag:0x%lx\n",
	       prPerMonitor->ulPerfMonFlag);
	DBGLOG(SW4, WARN, "ulLastTxBytes:%d\n",
	       prPerMonitor->ulLastTxBytes);
	DBGLOG(SW4, WARN, "ulLastRxBytes:%d\n",
	       prPerMonitor->ulLastRxBytes);
	DBGLOG(SW4, WARN, "ulP2PLastTxBytes:%d\n",
	       prPerMonitor->ulP2PLastTxBytes);
	DBGLOG(SW4, WARN, "ulP2PLastRxBytes:%d\n",
	       prPerMonitor->ulP2PLastRxBytes);
	DBGLOG(SW4, WARN, "ulThroughput:%d\n",
	       prPerMonitor->ulThroughput);
	DBGLOG(SW4, WARN, "u4UpdatePeriod:%d\n",
	       prPerMonitor->u4UpdatePeriod);
	DBGLOG(SW4, WARN, "u4TarPerfLevel:%d\n",
	       prPerMonitor->u4TarPerfLevel);
	DBGLOG(SW4, WARN, "u4CurrPerfLevel:%d\n",
	       prPerMonitor->u4CurrPerfLevel);
	DBGLOG(SW4, WARN, "netStats tx_bytes:%d\n",
	       prGlueInfo->prDevHandler->stats.tx_bytes);
	DBGLOG(SW4, WARN, "netStats tx_bytes:%d\n",
	       prGlueInfo->prDevHandler->stats.rx_bytes);
	DBGLOG(SW4, WARN, "p2p netStats tx_bytes:%d\n",
	       prGlueInfo->prP2PInfo->prDevHandler->stats.tx_bytes);
	DBGLOG(SW4, WARN, "p2p netStats tx_bytes:%d\n",
	       prGlueInfo->prP2PInfo->prDevHandler->stats.rx_bytes);
}
#endif

#define PERF_UPDATE_PERIOD      1000 /* ms */
#define BYTE_PER_MBIT (2^17)
#if (CFG_SUPPORT_PERF_IND == 1)
void kalPerfIndReset(struct ADAPTER *prAdapter)
{
	kalMemZero(&prAdapter->prGlueInfo->PerfIndCache,
		sizeof(prAdapter->prGlueInfo->PerfIndCache));
} /* kalPerfIndReset */

void kalSetPerfReport(struct ADAPTER *prAdapter)
{
	struct CMD_PERF_IND *prCmdPerfReport;
	uint8_t i;
	uint32_t u4CurrentTp = 0;
#if CFG_SUPPORT_TPUT_FACTOR
	struct WLAN_TABLE *prWtbl = prAdapter->rWifiVar.arWtbl;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	uint32_t u4WtblBitMap = 0;
#endif
	prCmdPerfReport = (struct CMD_PERF_IND *)
		cnmMemAlloc(prAdapter, RAM_TYPE_BUF,
		sizeof(struct CMD_PERF_IND));

	if (!prCmdPerfReport) {
		DBGLOG(SW4, ERROR,
			"cnmMemAlloc for kalSetPerfReport failed!\n");
		return;
	}
	kalMemZero(prCmdPerfReport, sizeof(struct CMD_PERF_IND));

	prCmdPerfReport->ucCmdVer = 0;
	prCmdPerfReport->u2CmdLen = sizeof(struct CMD_PERF_IND);

	prCmdPerfReport->u4VaildPeriod = PERF_UPDATE_PERIOD;
	prCmdPerfReport->ucBssNum = prAdapter->ucSwBssIdNum;

	for (i = 0; i < prCmdPerfReport->ucBssNum; i++) {
		prCmdPerfReport->rUniCmdParm[i].u4CurTxBytes =
			prAdapter->prGlueInfo->PerfIndCache.u4CurTxBytes[i];
		prCmdPerfReport->rUniCmdParm[i].u4CurRxBytes =
			prAdapter->prGlueInfo->PerfIndCache.u4CurRxBytes[i];
		prCmdPerfReport->rUniCmdParm[i].u2CurRxRate =
			prAdapter->prGlueInfo->PerfIndCache.u2CurRxRate[i];
		prCmdPerfReport->rUniCmdParm[i].ucCurRxRCPI0 =
			prAdapter->prGlueInfo->PerfIndCache.ucCurRxRCPI0[i];
		prCmdPerfReport->rUniCmdParm[i].ucCurRxRCPI1 =
			prAdapter->prGlueInfo->PerfIndCache.ucCurRxRCPI1[i];
		prCmdPerfReport->rUniCmdParm[i].ucCurRxNss =
			prAdapter->prGlueInfo->PerfIndCache.ucCurRxNss[i];
		prCmdPerfReport->rUniCmdParm[i].ucCurRxNss2 =
			prAdapter->prGlueInfo->PerfIndCache.ucCurRxNss2[i];
		u4CurrentTp +=
		    prCmdPerfReport->rUniCmdParm[i].u4CurTxBytes +
		    prCmdPerfReport->rUniCmdParm[i].u4CurRxBytes;
	}
	if (u4CurrentTp != 0) {
		for (i = 0; i < prCmdPerfReport->ucBssNum; i++) {
			DBGLOG(SW4, TRACE,
			"Total TP[%d] BSS[%d] TX-Byte[%d],RX-Byte[%d],Rate[%d],RCPI0[%d],RCPI1[%d]\n",
			u4CurrentTp,
			i,
			prCmdPerfReport->rUniCmdParm[i].u4CurTxBytes,
			prCmdPerfReport->rUniCmdParm[i].u4CurRxBytes,
			prCmdPerfReport->rUniCmdParm[i].u2CurRxRate,
			prCmdPerfReport->rUniCmdParm[i].ucCurRxRCPI0,
			prCmdPerfReport->rUniCmdParm[i].ucCurRxRCPI1);
		}
#if CFG_SUPPORT_TPUT_FACTOR
		if (u4CurrentTp >
			(prWifiVar->u4TputFactorDumpThresh * BYTE_PER_MBIT)) {
			for (i = 0; i < WTBL_SIZE; i++) {
				if ((!prWtbl[i].ucUsed) ||
					(!prWtbl[i].ucPairwise))
					continue;
				u4WtblBitMap |= BIT(i);
			}
		}
		prCmdPerfReport->u4WtblBitMap = u4WtblBitMap;
#endif
		wlanSendSetQueryCmd(prAdapter,
			CMD_ID_PERF_IND,
			TRUE,
			FALSE,
			FALSE,
			NULL,
			NULL,
			sizeof(*prCmdPerfReport),
			(uint8_t *) prCmdPerfReport, NULL, 0);
	}
	cnmMemFree(prAdapter, prCmdPerfReport);
}				/* kalSetPerfReport */
#endif

inline int32_t kalPerMonInit(struct GLUE_INFO
			     *prGlueInfo)
{
	struct PERF_MONITOR *prPerMonitor;
	struct net_device *prDevHandler = NULL;
	uint8_t i = 0;

	prPerMonitor = &prGlueInfo->prAdapter->rPerMonitor;
	DBGLOG(SW4, TRACE, "enter %s\n", __func__);
	if (KAL_TEST_BIT(PERF_MON_RUNNING_BIT,
			 prPerMonitor->ulPerfMonFlag))
		DBGLOG(SW4, WARN,
		       "abnormal, perf monitor already running\n");
	KAL_CLR_BIT(PERF_MON_RUNNING_BIT,
		    prPerMonitor->ulPerfMonFlag);
	KAL_CLR_BIT(PERF_MON_DISABLE_BIT,
		    prPerMonitor->ulPerfMonFlag);
	KAL_SET_BIT(PERF_MON_STOP_BIT, prPerMonitor->ulPerfMonFlag);
	prPerMonitor->u4UpdatePeriod =
		prGlueInfo->prAdapter->rWifiVar.u4PerfMonUpdatePeriod;
	if (prPerMonitor->u4UpdatePeriod != 0 &&
		prPerMonitor->u4UpdatePeriod < SEC_TO_MSEC(1)) {
		prPerMonitor->u4TriggerCnt =
			SEC_TO_MSEC(1) / prPerMonitor->u4UpdatePeriod;
	} else
		prPerMonitor->u4TriggerCnt = 1;
	cnmTimerInitTimerOption(prGlueInfo->prAdapter,
				&prPerMonitor->rPerfMonTimer,
				(PFN_MGMT_TIMEOUT_FUNC) kalPerMonHandler,
				(unsigned long) NULL,
				TIMER_WAKELOCK_NONE);

	/* sync data with netdev */
	GET_BOOT_SYSTIME(&prPerMonitor->rLastUpdateTime);
	for (i = 0; i < MAX_BSSID_NUM; i++) {
		prDevHandler = wlanGetNetDev(prGlueInfo, i);
		if (prDevHandler) {
			prPerMonitor->ulLastTxBytes[i] =
				prDevHandler->stats.tx_bytes;
			prPerMonitor->ulLastRxBytes[i] =
				prDevHandler->stats.rx_bytes;
			prPerMonitor->ulLastTxPackets[i] =
				prDevHandler->stats.tx_packets;
			prPerMonitor->ulLastRxPackets[i] =
				prDevHandler->stats.rx_packets;
			prPerMonitor->ulTxPacketsDiffLastSec[i] = 0;
			prPerMonitor->ulRxPacketsDiffLastSec[i] = 0;
		}
	}

#if CFG_SUPPORT_PERF_IND
	kalPerfIndReset(prGlueInfo->prAdapter);
#endif
	/* enable rps on all cpu cores */
	kalSetRpsMap(prGlueInfo, 0xff);

	kalBoostCpuInit(prGlueInfo->prAdapter);

	KAL_SET_BIT(PERF_MON_INIT_BIT, prPerMonitor->ulPerfMonFlag);
	DBGLOG(SW4, TRACE, "exit %s\n", __func__);
	return 0;
}

inline int32_t kalPerMonDisable(struct GLUE_INFO
				*prGlueInfo)
{
	struct PERF_MONITOR *prPerMonitor;

	if (!prGlueInfo->prAdapter)
		return 0;

	prPerMonitor = &prGlueInfo->prAdapter->rPerMonitor;

	DBGLOG(SW4, TRACE, "enter %s\n", __func__);
	if (KAL_TEST_BIT(PERF_MON_RUNNING_BIT,
			 prPerMonitor->ulPerfMonFlag)) {
		DBGLOG(SW4, TRACE, "need to stop before disable\n");
		kalPerMonStop(prGlueInfo);
	}
	KAL_SET_BIT(PERF_MON_DISABLE_BIT,
		    prPerMonitor->ulPerfMonFlag);
	DBGLOG(SW4, TRACE, "exit %s\n", __func__);
	return 0;
}

inline int32_t kalPerMonEnable(struct GLUE_INFO
			       *prGlueInfo)
{
	struct PERF_MONITOR *prPerMonitor;

	if (!prGlueInfo->prAdapter)
		return 0;

	prPerMonitor = &prGlueInfo->prAdapter->rPerMonitor;

	DBGLOG(SW4, TRACE, "enter %s\n", __func__);
	KAL_CLR_BIT(PERF_MON_DISABLE_BIT,
		    prPerMonitor->ulPerfMonFlag);
	DBGLOG(SW4, TRACE, "exit %s\n", __func__);
	return 0;
}

inline int32_t kalSetPerMonEnable(struct GLUE_INFO *prGlueInfo)
{
	DBGLOG(SW4, TRACE, "enter %s\n", __func__);
	clear_bit(GLUE_FLAG_DISABLE_PERF_BIT, &prGlueInfo->ulFlag);
	kalPerMonEnable(prGlueInfo);
	DBGLOG(SW4, LOUD, "exit %s\n", __func__);
	return 0;
}

inline int32_t kalSetPerMonDisable(struct GLUE_INFO *prGlueInfo)
{
	DBGLOG(SW4, TRACE, "enter %s\n", __func__);
	set_bit(GLUE_FLAG_DISABLE_PERF_BIT, &prGlueInfo->ulFlag);
	wake_up_interruptible(&prGlueInfo->waitq);
	DBGLOG(SW4, LOUD, "exit %s\n", __func__);
	return 0;
}

inline int32_t kalPerMonStart(struct GLUE_INFO
			      *prGlueInfo)
{
	struct PERF_MONITOR *prPerMonitor;
#ifdef CFG_SUPPORT_SNIFFER_RADIOTAP
	if (prGlueInfo->fgIsEnableMon)
		return 0;
#endif
	prPerMonitor = &prGlueInfo->prAdapter->rPerMonitor;
	DBGLOG(SW4, TEMP, "enter %s\n", __func__);

	if (!wlan_perf_monitor_force_enable &&
		(wlan_fb_power_down
		|| prGlueInfo->fgIsInSuspendMode
		))

		return 0;

	if (KAL_TEST_BIT(PERF_MON_DISABLE_BIT,
			 prPerMonitor->ulPerfMonFlag) ||
	    KAL_TEST_BIT(PERF_MON_RUNNING_BIT,
			 prPerMonitor->ulPerfMonFlag))
		return 0;

	prPerMonitor->u4PrevPerfLevel = 0;
	prPerMonitor->u4CurrPerfLevel = 0;
	prPerMonitor->u4TarPerfLevel = 0;
	prPerMonitor->u4BoostPerfLevel = 0;
	prPerMonitor->u4UpdatePeriod =
		prGlueInfo->prAdapter->rWifiVar.u4PerfMonUpdatePeriod;
	prPerMonitor->fgPolicyReady = FALSE;
	cnmTimerStartTimer(prGlueInfo->prAdapter,
		&prPerMonitor->rPerfMonTimer, prPerMonitor->u4UpdatePeriod);
	KAL_SET_BIT(PERF_MON_RUNNING_BIT,
		    prPerMonitor->ulPerfMonFlag);
	KAL_CLR_BIT(PERF_MON_STOP_BIT, prPerMonitor->ulPerfMonFlag);
	DBGLOG(SW4, TRACE, "perf monitor started\n");
	return 0;
}

inline int32_t kalPerMonStop(struct GLUE_INFO
			     *prGlueInfo)
{
	struct PERF_MONITOR *prPerMonitor;

	prPerMonitor = &prGlueInfo->prAdapter->rPerMonitor;
	DBGLOG(SW4, TRACE, "enter %s\n", __func__);

	if (KAL_TEST_BIT(PERF_MON_DISABLE_BIT,
			 prPerMonitor->ulPerfMonFlag)) {
		DBGLOG(SW4, TRACE, "perf monitor disabled\n");
		return 0;
	}

	if (KAL_TEST_BIT(PERF_MON_STOP_BIT,
			 prPerMonitor->ulPerfMonFlag)) {
		DBGLOG(SW4, TRACE, "perf monitor already stopped\n");
		return 0;
	}

	KAL_SET_BIT(PERF_MON_STOP_BIT, prPerMonitor->ulPerfMonFlag);
	if (KAL_TEST_BIT(PERF_MON_RUNNING_BIT,
			 prPerMonitor->ulPerfMonFlag)) {
		cnmTimerStopTimer(prGlueInfo->prAdapter,
				  &prPerMonitor->rPerfMonTimer);
		KAL_CLR_BIT(PERF_MON_RUNNING_BIT,
			    prPerMonitor->ulPerfMonFlag);

		prPerMonitor->u4PrevPerfLevel = 0;
		prPerMonitor->u4CurrPerfLevel = 0;
		prPerMonitor->u4TarPerfLevel = 0;
		prPerMonitor->u4BoostPerfLevel = 0;
		/*Cancel CPU performance mode request*/
		kalBoostCpu(prGlueInfo->prAdapter,
			    prPerMonitor->u4TarPerfLevel,
			    prGlueInfo->prAdapter->rWifiVar.u4BoostCpuTh);
#if (CFG_COALESCING_INTERRUPT == 1)
		kalCoalescingInt(prGlueInfo->prAdapter,
		prPerMonitor->u4TarPerfLevel,
		prGlueInfo->prAdapter->rWifiVar.u4PerfMonTpCoalescingIntTh);
#endif
	}
	DBGLOG(SW4, TRACE, "perf monitor stopped\n");
	return 0;
}

inline int32_t kalPerMonDestroy(struct GLUE_INFO
				*prGlueInfo)
{
	struct ADAPTER *prAdapter;
	struct PERF_MONITOR *prPerMonitor;

	if (!prGlueInfo)
		goto end;

	prAdapter = prGlueInfo->prAdapter;
	if (!prAdapter)
		goto end;

	prPerMonitor = &prAdapter->rPerMonitor;
	kalPerMonDisable(prGlueInfo);
	KAL_CLR_BIT(PERF_MON_INIT_BIT, prPerMonitor->ulPerfMonFlag);
	DBGLOG(SW4, TRACE, "exit %s\n", __func__);

end:
	return 0;
}

static u_int8_t wlanIsFirstNetInterfaceByNetdev(struct GLUE_INFO *prGlueInfo,
				struct net_device *ndev, uint8_t ucBssIndex)
{
	uint8_t i;
	struct NET_INTERFACE_INFO *prNetInterfaceInfo =
			prGlueInfo->arNetInterfaceInfo;

	for (i = 0; i < MAX_BSSID_NUM; i++) {
		if (ndev == prNetInterfaceInfo[i].pvNetInterface)
			break;
	}
	return i == ucBssIndex;
}

static uint32_t kalPerMonUpdate(struct ADAPTER *prAdapter)
{
	struct PERF_MONITOR *perf = &prAdapter->rPerMonitor;
	struct GLUE_INFO *glue = prAdapter->prGlueInfo;
	struct BSS_INFO *bss;
	struct net_device *ndev = NULL;
#if CFG_SUPPORT_LINK_QUALITY_MONITOR
	struct WIFI_LINK_QUALITY_INFO *lq = &prAdapter->rLinkQualityInfo;
#endif
	OS_SYSTIME now, last;
	int32_t period;
	uint8_t i, j;
	signed long txDiffBytes[MAX_BSSID_NUM] = {0};
	signed long rxDiffBytes[MAX_BSSID_NUM] = {0};
	signed long rxDiffPkts[MAX_BSSID_NUM] = {0};
	signed long txDiffPkts[MAX_BSSID_NUM] = {0};
	unsigned long lastTxBytes;
	unsigned long lastRxBytes;
	unsigned long lastTxPkts;
	unsigned long lastRxPkts;
	unsigned long currentTxBytes;
	unsigned long currentRxBytes;
	unsigned long currentTxPkts;
	unsigned long currentRxPkts;
	uint64_t throughput = 0;
	uint64_t throughputInPPS = 0;
	char *buf = NULL;
	char *head1;
	char *head2;
	char *head3;
	char *head4;
	char *head5;
#if CFG_QUEUE_RX_IF_CONN_NOT_READY
	char *head6;
#endif /* CFG_QUEUE_RX_IF_CONN_NOT_READY */
#if CFG_SUPPORT_TX_FREE_SKB_WORK
	char *head7;
	struct TX_FREE_INFO *prTxFreeInfo;
#endif /* CFG_SUPPORT_TX_FREE_SKB_WORK */
	char *pos;
	char *end;
	uint32_t slen;
	uint8_t fgIsValidNetDevice = FALSE;
#if KERNEL_VERSION(5, 18, 0) <= LINUX_VERSION_CODE
	struct rtnl_link_stats64 rtnls;
#endif

	uint32_t ret = WLAN_STATUS_SUCCESS;
#if CFG_SUPPORT_SKB_ALLOC_WORK
	struct SKB_ALLOC_INFO *prSkbAllocInfo = &glue->rSkbAllocInfo;
#endif /* CFG_SUPPORT_SKB_ALLOC_WORK */

	GLUE_SPIN_LOCK_DECLARATION();

	GET_BOOT_SYSTIME(&now);
	last = perf->rLastUpdateTime;

	if (!KAL_TEST_BIT(PERF_MON_INIT_BIT, perf->ulPerfMonFlag) ||
	    !CHECK_FOR_TIMEOUT(now, last,
			MSEC_TO_SYSTIME(perf->u4UpdatePeriod))) {
		ret = WLAN_STATUS_PENDING;
		goto done;
	}

	perf->rLastUpdateTime = now;

	period = ((int32_t) now - (int32_t) last) * MSEC_PER_SEC / KAL_HZ;
	if (period < 0) {
		/* overflow should not happen */
		DBGLOG(SW4, WARN, "wrong period: now=%u, last=%u, period=%d\n",
			now, last, period);
		ret = WLAN_STATUS_FAILURE;
		goto done;
	}

	for (i = 0; i < MAX_BSSID_NUM; i++) {
		ndev = wlanGetNetInterfaceByBssIdx(glue, i);
		if (ndev && !wlanIsFirstNetInterfaceByNetdev(glue, ndev, i))
			continue;
		bss = GET_BSS_INFO_BY_INDEX(prAdapter, i);

		GLUE_ACQUIRE_SPIN_LOCK(glue, SPIN_LOCK_NET_DEV);
		fgIsValidNetDevice = FALSE;

#ifdef CFG_SUPPORT_SNIFFER_RADIOTAP
		if (ndev && glue->fgIsEnableMon)
			fgIsValidNetDevice = TRUE;
#endif /* CFG_SUPPORT_SNIFFER_RADIOTAP */

		if (IS_BSS_ALIVE(prAdapter, bss) && ndev) {
			if (!IS_BSS_P2P(bss)) /* non-p2p */
				fgIsValidNetDevice = TRUE;
#if CFG_ENABLE_WIFI_DIRECT
			else if (prAdapter->rP2PNetRegState ==
					ENUM_NET_REG_STATE_REGISTERED) /* p2p */
				fgIsValidNetDevice = TRUE;
#endif
		}

		if (fgIsValidNetDevice) {
			currentTxBytes = ndev->stats.tx_bytes;
			currentRxBytes = ndev->stats.rx_bytes;
			currentTxPkts = ndev->stats.tx_packets;
			currentRxPkts = ndev->stats.rx_packets;
		} else {
			currentTxBytes = perf->ulLastTxBytes[i];
			currentRxBytes = perf->ulLastRxBytes[i];
			currentTxPkts = perf->ulLastTxPackets[i];
			currentRxPkts = perf->ulLastRxPackets[i];
		}
		GLUE_RELEASE_SPIN_LOCK(glue, SPIN_LOCK_NET_DEV);

		lastTxBytes = perf->ulLastTxBytes[i];
		lastRxBytes = perf->ulLastRxBytes[i];
		perf->ulLastTxBytes[i] = currentTxBytes;
		perf->ulLastRxBytes[i] = currentRxBytes;
		txDiffBytes[i] = (signed long) (currentTxBytes - lastTxBytes);
		rxDiffBytes[i] = (signed long) (currentRxBytes - lastRxBytes);

		lastTxPkts = perf->ulLastTxPackets[i];
		lastRxPkts = perf->ulLastRxPackets[i];
		perf->ulLastTxPackets[i] = currentTxPkts;
		perf->ulLastRxPackets[i] = currentRxPkts;
		txDiffPkts[i] = (signed long) (currentTxPkts - lastTxPkts);
		rxDiffPkts[i] = (signed long) (currentRxPkts - lastRxPkts);
		perf->ulTxPacketsDiffLastSec[i] = txDiffPkts[i];
		perf->ulRxPacketsDiffLastSec[i] = rxDiffPkts[i];

		if (txDiffBytes[i] < 0 || rxDiffBytes[i] < 0) {
			/* overflow should not happen */
			DBGLOG(SW4, WARN,
				"[%d]wrong bytes: tx[%lu][%lu][%ld], rx[%lu][%lu][%ld],\n",
				i, currentTxBytes, lastTxBytes, txDiffBytes[i],
				currentRxBytes, lastRxBytes, rxDiffBytes[i]);
			ret = WLAN_STATUS_FAILURE;
			goto done;
		}

		/* Divsion first to avoid overflow */
		perf->ulTxTp[i] = (txDiffBytes[i] / period) * MSEC_PER_SEC;
		perf->ulRxTp[i] = (rxDiffBytes[i] / period) * MSEC_PER_SEC;

		throughput += txDiffBytes[i] + rxDiffBytes[i];
		throughputInPPS += txDiffPkts[i] + rxDiffPkts[i];
	}

#if (CFG_SUPPORT_RX_NAPI == 1)
	kalNapiScheduleCheck(glue);
#endif
#if CFG_NAPI_DELAY
	kalNapiDelayCheck(glue);
#endif /* CFG_NAPI_DELAY */

#if CFG_SUPPORT_RETURN_WORK
#if CFG_DYNAMIC_RFB_ADJUSTMENT
	if (throughput == 0 && prAdapter->u4RfbUnUseCntLv != 0)
		kalRxRfbReturnWorkSchedule(glue);
#endif /* CFG_DYNAMIC_RFB_ADJUSTMENT */
#endif /* CFG_SUPPORT_RETURN_WORK */

#if CFG_SAP_RPS_SUPPORT
	if (prAdapter->rWifiVar.fgSapRpsEnable == 1) {
		p2pFuncRpsKalCheck(prAdapter,
					period,
					rxDiffPkts);
	}
#endif

	perf->fgIdle = (throughput == 0 && glue->i4TxPendingFrameNum == 0);
	perf->ulThroughput = throughput * MSEC_PER_SEC;
	do_div(perf->ulThroughput, period);
	perf->ulThroughput <<= 3;

	perf->ulThroughputInPPS = throughputInPPS * ETHER_MAX_PKT_SZ
		* MSEC_PER_SEC;
	do_div(perf->ulThroughputInPPS, period);
	perf->ulThroughputInPPS <<= 3;

	/* The length should include
	 * 1. "[%ld:%ld:%ld:%ld]" for each bss, %ld range is
	 *    [-9223372036854775807, +9223372036854775807]
	 * 2. "[%d:...:%d]" for pending frame num, %d range is [-32767, 32767]
	 * 3. ["%lu:%lu:%lu:%lu] dropped packets by each ndev, "%lu" range is
	 *    [0, 18446744073709551615]
	 * 4. [%lu:%lu:%lu:%lu] rx reordering que cnt
	 * 5. [%u:...:%u] tx mgmt packets categorized by 16 typesubtype
	 * 6. [%lu:%lu:%lu:%lu] rx pending que cnt
	 * 7. [%u/%u:...:%u/%u] TxFreeSkb current que cnt and total cnt
	 */
	slen = (20 * 4 + 5) * MAX_BSSID_NUM + 1 +
	       (6 * CFG_MAX_TXQ_NUM + 2 - 1) * MAX_BSSID_NUM + 1 +
	       (20 * 4 + 5) * MAX_BSSID_NUM + 1 +
	       (20 + 1) * MAX_BSSID_NUM + 1 +
	       (20 * MAX_NUM_OF_FC_SUBTYPES + MAX_NUM_OF_FC_SUBTYPES - 1) + 1;
#if CFG_QUEUE_RX_IF_CONN_NOT_READY
	slen += (20 + 1) * MAX_BSSID_NUM + 1;
#endif /* CFG_QUEUE_RX_IF_CONN_NOT_READY */
#if CFG_SUPPORT_TX_FREE_SKB_WORK
	slen += ((20*1) * 2 * CON_WORK_MAX) + 1;
#endif /* CFG_SUPPORT_TX_FREE_SKB_WORK */
	pos = buf = kalMemZAlloc(slen, VIR_MEM_TYPE);
	if (pos == NULL) {
		DBGLOG(SW4, INFO, "Can't allocate memory\n");
		ret = WLAN_STATUS_RESOURCES;
		goto done;
	}
	end = buf + slen;
	head1 = pos;
	for (i = 0; i < MAX_BSSID_NUM; i++) {
		pos += kalSnprintf(pos, end - pos, "[%lld:%lld:%lld:%lld]",
			(long long) txDiffBytes[i],
			(long long) txDiffPkts[i],
			(long long) rxDiffBytes[i],
			(long long) rxDiffPkts[i]);
	}
	pos++;
	head2 = pos;
	for (i = 0; i < MAX_BSSID_NUM; i++) {
		pos += kalSnprintf(pos, end - pos, "[");
		for (j = 0; j < CFG_MAX_TXQ_NUM - 1; ++j) {
			pos += kalSnprintf(pos, end - pos, "%d:",
				glue->ai4TxPendingFrameNumPerQueue[i][j]);
		}
		pos += kalSnprintf(pos, end - pos, "%d]",
			glue->ai4TxPendingFrameNumPerQueue[i][j]);
	}
	pos++;
	head3 = pos;
	for (i = 0; i < MAX_BSSID_NUM; i++) {
		ndev = wlanGetNetDev(glue, i);
		bss = GET_BSS_INFO_BY_INDEX(prAdapter, i);

		GLUE_ACQUIRE_SPIN_LOCK(glue, SPIN_LOCK_NET_DEV);
		fgIsValidNetDevice = FALSE;

		if (ndev) {
			if (!IS_BSS_P2P(bss)) /* non-p2p */
				fgIsValidNetDevice = TRUE;
#if CFG_ENABLE_WIFI_DIRECT
			else if (prAdapter->rP2PNetRegState ==
					ENUM_NET_REG_STATE_REGISTERED) /* p2p */
				fgIsValidNetDevice = TRUE;
#endif
		}

		if (fgIsValidNetDevice) {
#if KERNEL_VERSION(5, 18, 0) <= LINUX_VERSION_CODE
			dev_get_stats(ndev, &rtnls);
			pos += kalSnprintf(pos, end - pos,
				"[%llu:%llu:%llu:%llu]",
				(unsigned long long) ndev->stats.tx_dropped,
				(unsigned long long) rtnls.tx_dropped,
				(unsigned long long) ndev->stats.rx_dropped,
				(unsigned long long) rtnls.rx_dropped);
#else
			pos += kalSnprintf(pos, end - pos,
				"[%llu:%llu:%llu:%llu]",
				(unsigned long long) ndev->stats.tx_dropped,
				(unsigned long long)
					atomic_long_read(&ndev->tx_dropped),
				(unsigned long long) ndev->stats.rx_dropped,
				(unsigned long long)
					atomic_long_read(&ndev->rx_dropped));
#endif
		}
		GLUE_RELEASE_SPIN_LOCK(glue, SPIN_LOCK_NET_DEV);
	}
	pos++;
	head4 = pos;
	for (i = 0; i < MAX_BSSID_NUM; i++) {
		if (i == MAX_BSSID_NUM - 1) {
			pos += kalSnprintf(pos, end - pos, "%u",
				REORDERING_GET_BSS_CNT(&prAdapter->rRxCtrl, i));
		} else {
			pos += kalSnprintf(pos, end - pos, "%u:",
				REORDERING_GET_BSS_CNT(&prAdapter->rRxCtrl, i));
		}
	}
	pos++;
	head5 = pos;
	for (i = 0; i < ARRAY_SIZE(prAdapter->au4MgmtSubtypeTxCnt); i++)
		pos += kalSnprintf(pos, end - pos,
				i == MAX_NUM_OF_FC_SUBTYPES-1 ? "%u":"%u:",
				prAdapter->au4MgmtSubtypeTxCnt[i]);
#if CFG_QUEUE_RX_IF_CONN_NOT_READY
	pos++;
	head6 = pos;
	for (i = 0; i < MAX_BSSID_NUM; i++) {
		pos += kalSnprintf(pos, end - pos,
			(i == MAX_BSSID_NUM - 1) ? "%u" : "%u:",
			RX_PENDING_GET_BSS_CNT(&prAdapter->rRxCtrl, i));
	}
#endif /* CFG_QUEUE_RX_IF_CONN_NOT_READY */

#if CFG_SUPPORT_TX_FREE_SKB_WORK
	pos++;
	head7 = pos;
	prTxFreeInfo = &glue->rTxFreeInfo;
	for (i = 0; i < CON_WORK_MAX; i++) {
		pos += kalSnprintf(pos, end - pos,
			(i == CON_WORK_MAX - 1) ? "%u/%u" : "%u/%u ",
			QUEUE_LENGTH(&prTxFreeInfo->rQueInfo[i].rQue),
			prTxFreeInfo->rQueInfo[i].u4TotalCnt);
	}
#endif /* CFG_SUPPORT_TX_FREE_SKB_WORK */

#if CFG_SUPPORT_CPU_STAT
#define FORMAT_INT_8 \
	"%d,%d,%d,%d,%d,%d,%d,%d"

#if CFG_SUPPORT_PER_CPU_TX
#define PER_CPU_TX_CNT_TEMPLATE " PerCpuTx["FORMAT_INT_8"]"
#else /* CFG_SUPPORT_PER_CPU_TX */
#define PER_CPU_TX_CNT_TEMPLATE ""
#endif /* CFG_SUPPORT_PER_CPU_TX */

#if CFG_SUPPORT_TX_WORK
#define TX_WORK_CNT_TEMPLATE \
	" TxWork[%d]["FORMAT_INT_8"]"
#else /* CFG_SUPPORT_TX_WORK */
#define TX_WORK_CNT_TEMPLATE ""
#endif /* CFG_SUPPORT_TX_WORK */

#if CFG_SUPPORT_RX_WORK
#define RX_WORK_CNT_TEMPLATE \
	" RxWork[%d]["FORMAT_INT_8"]"
#else /* CFG_SUPPORT_RX_WORK */
#define RX_WORK_CNT_TEMPLATE ""
#endif /* CFG_SUPPORT_RX_WORK */

#if CFG_SUPPORT_SKB_ALLOC_WORK
#define SKB_ALLOC_WORK_CNT_TEMPLATE \
	" SkbAllocWork[%d]["FORMAT_INT_8"][%u][0x%x][%u]"
#else /* CFG_SUPPORT_SKB_ALLOC_WORK */
#define SKB_ALLOC_WORK_CNT_TEMPLATE ""
#endif /* CFG_SUPPORT_SKB_ALLOC_WORK */

#if CFG_SUPPORT_TX_FREE_SKB_WORK
#define TX_FREE_SKB_WORK_CNT_TEMPLATE \
	" TxFreeSkbWork[%d]["FORMAT_INT_8"]"
#else /* CFG_SUPPORT_TX_FREE_SKB_WORK */
#define TX_FREE_SKB_WORK_CNT_TEMPLATE ""
#endif /* CFG_SUPPORT_TX_FREE_SKB_WORK */

#define CPU_STAT_CNT_TEMPLATE \
	" TxCpu["FORMAT_INT_8"]" PER_CPU_TX_CNT_TEMPLATE \
	TX_WORK_CNT_TEMPLATE \
	TX_FREE_SKB_WORK_CNT_TEMPLATE \
	" RxCpu["FORMAT_INT_8"]" RX_WORK_CNT_TEMPLATE \
	SKB_ALLOC_WORK_CNT_TEMPLATE
#else /* CFG_SUPPORT_CPU_STAT */
#define CPU_STAT_CNT_TEMPLATE ""
#endif /* CFG_SUPPORT_CPU_STAT */

#if CFG_SUPPORT_LINK_QUALITY_MONITOR
#define LINK_QUALITY_MONITOR_TEMPLATE \
	"LQ[%llu:%llu:%llu]"
#else /* CFG_SUPPORT_LINK_QUALITY_MONITOR */
#define LINK_QUALITY_MONITOR_TEMPLATE ""
#endif /* CFG_SUPPORT_LINK_QUALITY_MONITOR */

#ifdef CFG_SUPPORT_SNIFFER_RADIOTAP
#define RADIOTAP_LOG_TEMPLATE " Mo:[%u:%lu:%lu:%lu]"
#else /* CFG_SUPPORT_SNIFFER_RADIOTAP */
#define RADIOTAP_LOG_TEMPLATE ""
#endif /* CFG_SUPPORT_SNIFFER_RADIOTAP */

#if CFG_DYNAMIC_RFB_ADJUSTMENT
#define DYNAMIC_RFB_TEMPLATE " RfbLv:%u"
#else /* CFG_DYNAMIC_RFB_ADJUSTMENT */
#define DYNAMIC_RFB_TEMPLATE ""
#endif /* CFG_DYNAMIC_RFB_ADJUSTMENT */

#if CFG_SUPPORT_TX_FREE_SKB_WORK
#define TX_FREE_SKBQ_TEMPLATE " TxFreeSkbQ[%s]"
#else /* CFG_SUPPORT_TX_FREE_SKB_WORK */
#define TX_FREE_SKBQ_TEMPLATE ""
#endif /* CFG_SUPPORT_TX_FREE_SKB_WORK */

#define TEMP_LOG_TEMPLATE \
	"<%dms> Tput: %llu(%llu.%03llumbps) %s Pending:%d/%d %s " \
	RADIOTAP_LOG_TEMPLATE \
	LINK_QUALITY_MONITOR_TEMPLATE \
	" idle:%u lv:%u th:%u fg:0x%lx" \
	DYNAMIC_RFB_TEMPLATE \
	CPU_STAT_CNT_TEMPLATE \
	" TxDp[ST:BS:FO:QM:DP]:%u:%u:%u:%u:%u" \
	" Tx[SQ:TI:TM:TDD:TDM]:%u:%u:%u:%u:%u" \
	" MgmtSub[%s]" \
	TX_FREE_SKBQ_TEMPLATE \
	"\n"

	DBGLOG(SW4, INFO, TEMP_LOG_TEMPLATE,
		period,	(unsigned long long) perf->ulThroughput,
		(unsigned long long) (perf->ulThroughput >> 20),
		(unsigned long long) ((perf->ulThroughput >> 10) & BITS(0, 9)),
		head1, GLUE_GET_REF_CNT(glue->i4TxPendingFrameNum),
		prAdapter->rWifiVar.u4NetifStopTh, head2,
#ifdef CFG_SUPPORT_SNIFFER_RADIOTAP
		glue->fgIsEnableMon,
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_SNIFFER_LOG_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_PDMA_SCATTER_DATA_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl,
			RX_PDMA_SCATTER_INDICATION_COUNT),
#endif /* CFG_SUPPORT_SNIFFER_RADIOTAP */
#if CFG_SUPPORT_LINK_QUALITY_MONITOR
		(unsigned long long) lq->u8TxTotalCount,
		(unsigned long long) lq->u8RxTotalCount,
		(unsigned long long) lq->u8DiffIdleSlotCount,
#endif
		perf->fgIdle,
		perf->u4CurrPerfLevel,
		prAdapter->rWifiVar.u4BoostCpuTh,
		perf->ulPerfMonFlag,
#if CFG_DYNAMIC_RFB_ADJUSTMENT
		prAdapter->u4RfbUnUseCntLv,
#endif /* CFG_DYNAMIC_RFB_ADJUSTMENT */
#if CFG_SUPPORT_CPU_STAT
		CPU_STAT_GET_CNT(glue, CPU_TX_IN, 0),
		CPU_STAT_GET_CNT(glue, CPU_TX_IN, 1),
		CPU_STAT_GET_CNT(glue, CPU_TX_IN, 2),
		CPU_STAT_GET_CNT(glue, CPU_TX_IN, 3),
		CPU_STAT_GET_CNT(glue, CPU_TX_IN, 4),
		CPU_STAT_GET_CNT(glue, CPU_TX_IN, 5),
		CPU_STAT_GET_CNT(glue, CPU_TX_IN, 6),
		CPU_STAT_GET_CNT(glue, CPU_TX_IN, 7),
#if CFG_SUPPORT_PER_CPU_TX
		CPU_STAT_GET_CNT(glue, CPU_TX_PER_CPU, 0),
		CPU_STAT_GET_CNT(glue, CPU_TX_PER_CPU, 1),
		CPU_STAT_GET_CNT(glue, CPU_TX_PER_CPU, 2),
		CPU_STAT_GET_CNT(glue, CPU_TX_PER_CPU, 3),
		CPU_STAT_GET_CNT(glue, CPU_TX_PER_CPU, 4),
		CPU_STAT_GET_CNT(glue, CPU_TX_PER_CPU, 5),
		CPU_STAT_GET_CNT(glue, CPU_TX_PER_CPU, 6),
		CPU_STAT_GET_CNT(glue, CPU_TX_PER_CPU, 7),
#endif /* CFG_SUPPORT_PER_CPU_TX */
#if CFG_SUPPORT_TX_WORK
		kalWorkGetCpu(glue, TX_WORK),
		CPU_STAT_GET_CNT(glue, CPU_TX_WORK_DONE, 0),
		CPU_STAT_GET_CNT(glue, CPU_TX_WORK_DONE, 1),
		CPU_STAT_GET_CNT(glue, CPU_TX_WORK_DONE, 2),
		CPU_STAT_GET_CNT(glue, CPU_TX_WORK_DONE, 3),
		CPU_STAT_GET_CNT(glue, CPU_TX_WORK_DONE, 4),
		CPU_STAT_GET_CNT(glue, CPU_TX_WORK_DONE, 5),
		CPU_STAT_GET_CNT(glue, CPU_TX_WORK_DONE, 6),
		CPU_STAT_GET_CNT(glue, CPU_TX_WORK_DONE, 7),
#endif /* CFG_SUPPORT_TX_WORK */
#if CFG_SUPPORT_TX_FREE_SKB_WORK
		prTxFreeInfo->eCoreType,
		CPU_STAT_GET_CNT(glue, CPU_TX_FREE_SKB_DONE, 0),
		CPU_STAT_GET_CNT(glue, CPU_TX_FREE_SKB_DONE, 1),
		CPU_STAT_GET_CNT(glue, CPU_TX_FREE_SKB_DONE, 2),
		CPU_STAT_GET_CNT(glue, CPU_TX_FREE_SKB_DONE, 3),
		CPU_STAT_GET_CNT(glue, CPU_TX_FREE_SKB_DONE, 4),
		CPU_STAT_GET_CNT(glue, CPU_TX_FREE_SKB_DONE, 5),
		CPU_STAT_GET_CNT(glue, CPU_TX_FREE_SKB_DONE, 6),
		CPU_STAT_GET_CNT(glue, CPU_TX_FREE_SKB_DONE, 7),
#endif /* CFG_SUPPORT_TX_FREE_SKB_WORK */
		CPU_STAT_GET_CNT(glue, CPU_RX_IN, 0),
		CPU_STAT_GET_CNT(glue, CPU_RX_IN, 1),
		CPU_STAT_GET_CNT(glue, CPU_RX_IN, 2),
		CPU_STAT_GET_CNT(glue, CPU_RX_IN, 3),
		CPU_STAT_GET_CNT(glue, CPU_RX_IN, 4),
		CPU_STAT_GET_CNT(glue, CPU_RX_IN, 5),
		CPU_STAT_GET_CNT(glue, CPU_RX_IN, 6),
		CPU_STAT_GET_CNT(glue, CPU_RX_IN, 7),
#if CFG_SUPPORT_RX_WORK
		kalWorkGetCpu(glue, RX_WORK),
		CPU_STAT_GET_CNT(glue, CPU_RX_WORK_DONE, 0),
		CPU_STAT_GET_CNT(glue, CPU_RX_WORK_DONE, 1),
		CPU_STAT_GET_CNT(glue, CPU_RX_WORK_DONE, 2),
		CPU_STAT_GET_CNT(glue, CPU_RX_WORK_DONE, 3),
		CPU_STAT_GET_CNT(glue, CPU_RX_WORK_DONE, 4),
		CPU_STAT_GET_CNT(glue, CPU_RX_WORK_DONE, 5),
		CPU_STAT_GET_CNT(glue, CPU_RX_WORK_DONE, 6),
		CPU_STAT_GET_CNT(glue, CPU_RX_WORK_DONE, 7),
#endif /* CFG_SUPPORT_RX_WORK */
#if CFG_SUPPORT_SKB_ALLOC_WORK
		prSkbAllocInfo->eCoreType,
		CPU_STAT_GET_CNT(glue, CPU_SKB_ALLOC_DONE, 0),
		CPU_STAT_GET_CNT(glue, CPU_SKB_ALLOC_DONE, 1),
		CPU_STAT_GET_CNT(glue, CPU_SKB_ALLOC_DONE, 2),
		CPU_STAT_GET_CNT(glue, CPU_SKB_ALLOC_DONE, 3),
		CPU_STAT_GET_CNT(glue, CPU_SKB_ALLOC_DONE, 4),
		CPU_STAT_GET_CNT(glue, CPU_SKB_ALLOC_DONE, 5),
		CPU_STAT_GET_CNT(glue, CPU_SKB_ALLOC_DONE, 6),
		CPU_STAT_GET_CNT(glue, CPU_SKB_ALLOC_DONE, 7),
		prSkbAllocInfo->u4ScheCnt,
		prSkbAllocInfo->ulScheMask,
		skb_queue_len(&prSkbAllocInfo->rFreeSkbQ),
#endif /* CFG_SUPPORT_SKB_ALLOC_WORK */
#endif /* CFG_SUPPORT_CPU_STAT */
		TX_GET_CNT(&prAdapter->rTxCtrl, TX_INACTIVE_STA_DROP),
		TX_GET_CNT(&prAdapter->rTxCtrl, TX_INACTIVE_BSS_DROP),
		TX_GET_CNT(&prAdapter->rTxCtrl, TX_FORWARD_OVERFLOW_DROP),
		TX_GET_CNT(&prAdapter->rTxCtrl, TX_INVALID_MSDUINFO_COUNT),
		TX_GET_CNT(&prAdapter->rTxCtrl, TX_DROP_PID_COUNT),
		skb_queue_len(&glue->rTxDirectSkbQueue),
		TX_GET_CNT(&prAdapter->rTxCtrl, TX_IN_COUNT),
		TX_GET_CNT(&prAdapter->rTxCtrl, TX_MSDUINFO_COUNT),
		TX_GET_CNT(&prAdapter->rTxCtrl, TX_DIRECT_DEQUEUE_COUNT),
		TX_GET_CNT(&prAdapter->rTxCtrl, TX_DIRECT_MSDUINFO_COUNT),
		head5
#if CFG_SUPPORT_TX_FREE_SKB_WORK
		, head7
#endif /* CFG_SUPPORT_TX_FREE_SKB_WORK */
		);
#undef TEMP_LOG_TEMPLATE
#undef LINK_QUALITY_MONITOR_TEMPLATE
#if CFG_SUPPORT_CPU_STAT
#undef PER_CPU_TX_CNT_TEMPLATE
#undef TX_WORK_CNT_TEMPLATE
#undef RX_WORK_CNT_TEMPLATE
#undef SKB_ALLOC_WORK_CNT_TEMPLATE
#undef TX_FREE_SKB_WORK_CNT_TEMPLATE
#undef TX_FREE_SKBQ_TEMPLATE
#endif /* CFG_SUPPORT_CPU_STAT */
#undef CPU_STAT_CNT_TEMPLATE
#undef DYNAMIC_RFB_TEMPLATE

#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
#define RRO_LOG_TEMPLATE \
	"RRO[%d,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu," \
	"%lu,%lu,%lu] "
#else /* CFG_SUPPORT_HOST_OFFLOAD == 1 */
#define RRO_LOG_TEMPLATE ""
#endif /* CFG_SUPPORT_HOST_OFFLOAD == 1 */

#if CFG_RFB_TRACK
#define RRB_TRACK_TEMPLATE \
	"RfbTrack[%u:%u:%u:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d] "
#else /* CFG_RFB_TRACK */
#define RRB_TRACK_TEMPLATE ""
#endif /* CFG_RFB_TRACK */

#if CFG_QUEUE_RX_IF_CONN_NOT_READY
#define RX_PENDING_TEMPLATE "RxPending[%s] "
#else /* CFG_QUEUE_RX_IF_CONN_NOT_READY */
#define RX_PENDING_TEMPLATE ""
#endif /* CFG_QUEUE_RX_IF_CONN_NOT_READY */

#if CFG_SUPPORT_RX_GRO
#define NAPI_TEMPLATE "NAPI[%lu,%lu,%u,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%u] "
#else
#define NAPI_TEMPLATE "NAPI[%lu,%lu,%u,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu] "
#endif

#if CFG_NAPI_DELAY
#define NAPI_DELAY_TEMPLATE "NapiDelay[%u,%u,%u,0x%x,%u] "
#else /* CFG_NAPI_DELAY */
#define NAPI_DELAY_TEMPLATE ""
#endif /* CFG_NAPI_DELAY */

#define TEMP_LOG_TEMPLATE \
	"ndevdrp:%s " \
	NAPI_TEMPLATE \
	NAPI_DELAY_TEMPLATE \
	RRO_LOG_TEMPLATE \
	"RxReorder[%s] " \
	RX_PENDING_TEMPLATE \
	RRB_TRACK_TEMPLATE \
	"drv[RM,IL,RI,RIE,NGR|RF,PA,PF,DU,DA#" \
	"RT,RM,RW,RA,RB|DT,NS,IB,HS,LS_" \
	"DD,ME,BD,NI,DR|TE,PE,CE,DN,FE^" \
	"DE,IE,TME,CM,FB|ID,FD,NL]:" \
	"%lu,%lu,%lu,%lu,%lu|%lu,%lu,%lu,%lu,%lu#" \
	"%lu,%lu,%lu,%lu,%lu|%lu,%lu,%lu,%lu,%lu_" \
	"%lu,%lu,%lu,%lu,%lu|%lu,%lu,%lu,%lu,%lu^" \
	"%lu,%lu,%lu,%lu,%lu|%lu,%lu,%lu\n" \

	DBGLOG(SW4, INFO, TEMP_LOG_TEMPLATE,
		head3,
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_INTR_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_TASKLET_COUNT),
		glue->fgNapiScheduled,
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_NAPI_WORK_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_NAPI_SCHEDULE_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_NAPI_LEGACY_SCHED_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_NAPI_POLL_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_NAPI_FIFO_IN_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_NAPI_FIFO_OUT_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_NAPI_FIFO_FULL_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_NAPI_FIFO_ABNORMAL_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_NAPI_FIFO_ABN_FULL_COUNT),
#if CFG_SUPPORT_RX_GRO
		skb_queue_len(&glue->rRxNapiSkbQ),
#endif
#if CFG_NAPI_DELAY
		prAdapter->rWifiVar.u4NapiDelayTputTh,
		prAdapter->rWifiVar.u4NapiDelayCntTh,
		prAdapter->rWifiVar.u4NapiDelayTimeout,
		glue->ulNapiDelayFlag,
		KAL_GET_FIFO_CNT(glue),
#endif /* CFG_NAPI_DELAY */
#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
		prAdapter->rWifiVar.fgEnableRro,
		RX_RRO_GET_CNT(&prAdapter->rRxCtrl, RRO_STEP_ONE),
		RX_RRO_GET_CNT(&prAdapter->rRxCtrl, RRO_REPEAT),
		RX_RRO_GET_CNT(&prAdapter->rRxCtrl, RRO_OLDPKT),
		RX_RRO_GET_CNT(&prAdapter->rRxCtrl, RRO_WITHIN),
		RX_RRO_GET_CNT(&prAdapter->rRxCtrl, RRO_SURPASS),
		RX_RRO_GET_CNT(&prAdapter->rRxCtrl, RRO_SURPASS_BY_BAR),
		RX_RRO_GET_CNT(&prAdapter->rRxCtrl, RRO_SURPASS_BIG_SN),
		RX_RRO_GET_CNT(&prAdapter->rRxCtrl, RRO_DISCONNECT),
		RX_RRO_GET_CNT(&prAdapter->rRxCtrl, RRO_NOT_RRO_PKT),
		RX_RRO_GET_CNT(&prAdapter->rRxCtrl, RRO_TIMEOUT_STEP_ONE),
		RX_RRO_GET_CNT(&prAdapter->rRxCtrl, RRO_TIMEOUT_FLUSH_ALL),
		RX_RRO_GET_CNT(&prAdapter->rRxCtrl, RRO_BUF_RUN_OUT),
		/* used when recv abnormal reason */
		RX_RRO_GET_CNT(&prAdapter->rRxCtrl, RRO_COUNTER_NUM),
#endif /* CFG_SUPPORT_HOST_OFFLOAD == 1 */
		head4,
#if CFG_QUEUE_RX_IF_CONN_NOT_READY
		head6,
#endif /* CFG_QUEUE_RX_IF_CONN_NOT_READY */
#if CFG_RFB_TRACK
		prAdapter->rWifiVar.fgRfbTrackEn,
		prAdapter->rWifiVar.u4RfbTrackInterval,
		prAdapter->rWifiVar.u4RfbTrackTimeout,
		RFB_TRACK_GET_CNT(&prAdapter->rRxCtrl, RFB_TRACK_INIT),
		RFB_TRACK_GET_CNT(&prAdapter->rRxCtrl, RFB_TRACK_UNUSE),
		RFB_TRACK_GET_CNT(&prAdapter->rRxCtrl, RFB_TRACK_FREE),
		RFB_TRACK_GET_CNT(&prAdapter->rRxCtrl, RFB_TRACK_HIF),
		RFB_TRACK_GET_CNT(&prAdapter->rRxCtrl, RFB_TRACK_RX),
		RFB_TRACK_GET_CNT(&prAdapter->rRxCtrl, RFB_TRACK_MAIN),
		RFB_TRACK_GET_CNT(&prAdapter->rRxCtrl, RFB_TRACK_FIFO),
		RFB_TRACK_GET_CNT(&prAdapter->rRxCtrl, RFB_TRACK_NAPI),
		RFB_TRACK_GET_CNT(&prAdapter->rRxCtrl, RFB_TRACK_DATA),
		RFB_TRACK_GET_CNT(&prAdapter->rRxCtrl, RFB_TRACK_REORDERING_IN),
		RFB_TRACK_GET_CNT(&prAdapter->rRxCtrl,
			RFB_TRACK_REORDERING_OUT),
		RFB_TRACK_GET_CNT(&prAdapter->rRxCtrl, RFB_TRACK_INDICATED),
		RFB_TRACK_GET_CNT(&prAdapter->rRxCtrl, RFB_TRACK_PACKET_SETUP),
		RFB_TRACK_GET_CNT(&prAdapter->rRxCtrl, RFB_TRACK_ADJUST_UNUSE),
		RFB_TRACK_GET_CNT(&prAdapter->rRxCtrl, RFB_TRACK_MLO),
		RFB_TRACK_GET_CNT(&prAdapter->rRxCtrl, RFB_TRACK_FAIL),
#endif /* CFG_RFB_TRACK */
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_MPDU_TOTAL_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_ICS_LOG_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_DATA_INDICATION_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_DATA_INDICATION_END_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_DATA_NAPI_GRO_RECEIVE_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_DATA_FORWARD_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_PACKET_ALLOC_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_PACKET_FREE_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_DATA_RETURNED_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_DATA_RETAINED_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_DATA_REORDER_TOTAL_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_DATA_REORDER_MISS_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_DATA_REORDER_WITHIN_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_DATA_REORDER_AHEAD_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_DATA_REORDER_BEHIND_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_DROP_TOTAL_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_NO_STA_DROP_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_INACTIVE_BSS_DROP_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_HS20_DROP_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_LESS_SW_RFB_DROP_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_DUPICATE_DROP_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_MIC_ERROR_DROP_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_BAR_DROP_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_NO_INTEREST_DROP_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_REORDER_BEHIND_DROP_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_TYPE_ERR_DROP_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_POINTER_ERR_DROP_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_CLASS_ERR_DROP_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_DST_NULL_DROP_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_FCS_ERR_DROP_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_DAF_ERR_DROP_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_ICV_ERR_DROP_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_TKIP_MIC_ERROR_DROP_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_CIPHER_MISMATCH_DROP_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_FRAGMENT_BMC_DROP_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_ICS_DROP_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_FW_DROP_SSN_COUNT),
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_NULL_PACKET_COUNT)
		);
#undef TEMP_LOG_TEMPLATE
#undef RRO_LOG_TEMPLATE
#undef RADIOTAP_LOG_TEMPLATE
#undef RX_PENDING_TEMPLATE
#undef NAPI_DELAY_TEMPLATE

	kalTraceEvent("Tput: %llu.%03llumbps",
		(unsigned long long) (perf->ulThroughput >> 20),
		(unsigned long long) ((perf->ulThroughput >> 10) & BITS(0, 9)));

done:
	if (buf)
		kalMemFree(buf, VIR_MEM_TYPE, slen);
	return ret;
}

#if CFG_SUPPORT_TPUT_FACTOR
void kalTputFactorUpdate(struct ADAPTER *prAdapter)
{
#define TPUT_LOG_TEMPLATE \
		"wlanIdx=%d staIdx=%d BSSIdx=%d CH=%d " \
		"BN=%d Wmmset=%d NetType=%u OwnMAC="MACSTR" " \
		"PeerAddr="MACSTR "\n"

#if CFG_SUPPORT_PERMON
	struct PERF_MONITOR *perf = &prAdapter->rPerMonitor;
#endif
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	struct WLAN_TABLE *prWtbl = prAdapter->rWifiVar.arWtbl;
	struct BSS_INFO *prBssInfo;
	uint8_t i = 0;

#if KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
	struct cpufreq_policy *prCpuPolicy;
	int i4cpu;
#endif

	char *buf = NULL;
	char *pos = NULL, *end = NULL;
	uint32_t u4slen = 0;

	static OS_SYSTIME lv1_last, lv2_last;
	OS_SYSTIME now;

	if (test_bit(GLUE_FLAG_HALT_BIT, &prAdapter->prGlueInfo->ulFlag))
		return;

	if (IS_FEATURE_DISABLED(prWifiVar->fgTputFactorDump))
		return;

	if (kalGetTpMbps(prAdapter, PKT_PATH_ALL)
			< prWifiVar->u4TputFactorDumpThresh)
		return;

#if CFG_SUPPORT_PERMON
	now = perf->rLastUpdateTime;
	/* dump Tput factor with PerfMon */
	if (!CHECK_FOR_TIMEOUT(now, lv1_last,
		MSEC_TO_SYSTIME(perf->u4UpdatePeriod)))
		return;
#else
	GET_BOOT_SYSTIME(&now);
	/* dump Tput factor every 500m */
	if (!CHECK_FOR_TIMEOUT(now, lv1_last,
		MSEC_TO_SYSTIME(prWifiVar->u4TputFactorDumpPeriodL1)))
		return;
#endif

	lv1_last = now;

	/* The length should include
	 * 1. "%d " for each core frequency, %d range is
	 *    [0, 3050000]
	 * 2. "PCIe:%x,%x" for PCIe link speed
	 * 2.1 "usb:%d"  for USB speed
	 */
	u4slen = CPU_CNT*CPU_LOG_LEN +
	PWR_MODE_LOG_HEAD_LEN +
	MAX_BSSID_NUM*PWR_MODE_LOG_LEN;

	pos = buf = kalMemZAlloc(u4slen, VIR_MEM_TYPE);
	if (pos == NULL) {
		DBGLOG(SW4, INFO, "Can't allocate memory\n");
		return;
	}
	end = buf + u4slen;

#if KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
	for_each_possible_cpu(i4cpu) {
		prCpuPolicy = cpufreq_cpu_get(i4cpu);

		if (!prCpuPolicy)
			continue;

		pos += kalSnprintf(pos, end-pos, "%d,", prCpuPolicy->cur);
		cpufreq_cpu_put(prCpuPolicy);
	}
#endif
	pos += kalSnprintf(pos, end-pos, "PwrMode:");
	for (i = 0; i < MAX_BSSID_NUM; ++i) {
		prBssInfo = prAdapter->aprBssInfo[i];
		if (prBssInfo == NULL)
			continue;
		pos += kalSnprintf(pos, end-pos, "%d,", prBssInfo->ePwrMode);
	}
	DBGLOG(SW4, INFO,
		"freq %s mask:hif %x, rx %x, main %x, ApFS=%d",
		buf,
#if KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
		prAdapter->prGlueInfo->hif_cpu_mask,
		prAdapter->prGlueInfo->rx_thread->cpus_mask,
		prAdapter->prGlueInfo->main_thread->cpus_mask,
#else
		0x0,
		0x0,
		0x0,
#endif
		prWifiVar->ucApForceSleep
		);

	kalMemFree(buf, VIR_MEM_TYPE, u4slen);

	if (!CHECK_FOR_TIMEOUT(now, lv2_last,
		MSEC_TO_SYSTIME(prWifiVar->u4TputFactorDumpPeriodL2)))
		return;
	lv2_last = now;
	/*Iterate WTBL list to find inused tables
	 *and dump Tput factor (see TPUT_LOG_TEMPLATE)
	 */
	for (i = 0; i < WTBL_SIZE; i++) {
		if ((!prWtbl[i].ucUsed) || (!prWtbl[i].ucPairwise))
			continue;

		if (prWtbl[i].ucBssIndex >= MAX_BSSID_NUM)
			continue;

		prBssInfo = prAdapter->aprBssInfo[prWtbl[i].ucBssIndex];

		if (prBssInfo == NULL)
			continue;

		DBGLOG(SW4, INFO, TPUT_LOG_TEMPLATE,
			i,
			prWtbl[i].ucStaIndex,
			prWtbl[i].ucBssIndex,
			prBssInfo->ucPrimaryChannel,
			prBssInfo->eBand,
			prBssInfo->ucWmmQueSet,
			cnmGetBssNetworkType(prBssInfo),
			MAC2STR(prBssInfo->aucOwnMacAddr),
			MAC2STR(prWtbl[i].aucMacAddr));
	}
}
#endif

#if CFG_SUPPORT_MCC_BOOST_CPU
void kalMccBoostCheck(struct ADAPTER *prAdapter, uint32_t u4TputLv)
{
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	struct BSS_INFO *prBssInfo;
	u_int8_t fgMccBoost = FALSE;
	uint8_t i;

	if (!cnmIsMccMode(prAdapter))
		goto end;

	if (u4TputLv < prWifiVar->u4MccBoostTputLvTh)
		goto end;

	for (i = 0; i < MAX_BSSID_NUM; i++) {
		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, i);
		if (!IS_BSS_ALIVE(prAdapter, prBssInfo))
			continue;

		/* no ready yet */
		if (prBssInfo->u4PresentTime == 0)
			continue;

		if (prBssInfo->u4PresentTime <
				prWifiVar->u4MccBoostPresentTime) {
			fgMccBoost = TRUE;
			DBGLOG(INIT, INFO,
				"B:%u P:%u TputLv:%d State:%u->%u\n",
				i, prBssInfo->u4PresentTime, u4TputLv,
				prAdapter->fgMccBoost, fgMccBoost);
		}
	}

	/* Boost CPU when Tput > 100Mbps */
	if (u4TputLv >= prWifiVar->u4MccBoostForAllTputLvTh) {
		fgMccBoost = TRUE;
		DBGLOG(INIT, INFO, "TputLv:%u Th:%u State:%u->%u\n",
			u4TputLv,
			prWifiVar->u4MccBoostForAllTputLvTh,
			prAdapter->fgMccBoost, fgMccBoost);
	}
end:
	prAdapter->fgMccStateChange =
		(prAdapter->fgMccBoost != fgMccBoost) ? TRUE : FALSE;
	prAdapter->fgMccBoost = fgMccBoost;
}

u_int8_t kalIsMccStateChange(struct ADAPTER *prAdapter)
{
	return prAdapter->fgMccStateChange;
}

u_int8_t __weak kalIsMccBoost(struct ADAPTER *prAdapter)
{
	return prAdapter->fgMccBoost;
}
#endif /* CFG_SUPPORT_MCC_BOOST_CPU */

void kalPerMonHandler(struct ADAPTER *prAdapter,
		      unsigned long ulParam)
{
	/*Calculate current throughput*/
	struct PERF_MONITOR *prPerMonitor;
	uint32_t u4Idx = 0;
	uint8_t i = 0;
	uint64_t maxTput = 0;
	bool keep_alive = FALSE;
	struct net_device *prDevHandler = NULL;
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
#if CFG_SUPPORT_PERF_IND || CFG_SUPPORT_DATA_STALL
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
#endif
	uint32_t u4BoostCpuTh = prAdapter->rWifiVar.u4BoostCpuTh;
#if (CFG_COALESCING_INTERRUPT == 1)
	uint32_t u4CoalescingIntTh;
#endif
	bool fgIsStopPerfMon = FALSE;

	if (test_bit(GLUE_FLAG_HALT_BIT, &prGlueInfo->ulFlag))
		return;

	prPerMonitor = &prAdapter->rPerMonitor;
	DBGLOG(SW4, TRACE, "enter kalPerMonHandler\n");

	for (i = 0; i < MAX_BSSID_NUM; i++) {
		struct BSS_INFO *prBssInfo;

		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, i);
		prDevHandler = wlanGetNetInterfaceByBssIdx(prGlueInfo, i);
		if (IS_BSS_ALIVE(prAdapter, prBssInfo) && prDevHandler) {
			keep_alive |= netif_carrier_ok(prDevHandler);

			/* Adjust CPU threshold when:
				* CPU threshould has not been set in wifi.cfg
				* Current CPU boost threshold > 100M
				* Coex TDD & BSS in 2.4G
				* The specific BSS Tput > 100M
				* the PerfMonLv3 still stands for 100M
			*/
			DBGLOG(SW4, TRACE,
			"Coex_i[%d]ad[%d]k[%d]m[%d]eb[%d]tp[%u]tpi[%u]tpth[%d]th[%d]\n",
			i, prWifiVar->fgIsBoostCpuThAdjustable, keep_alive,
			prBssInfo->eCoexMode, prBssInfo->eBand,
			prWifiVar->u4PerfMonTpTh[u4BoostCpuTh],
			kalGetTpMbpsByBssId(prAdapter, PKT_PATH_ALL, i),
			prWifiVar->u4PerfMonTpTh[2],
			PERF_MON_COEX_TP_THRESHOLD);

			if ((prWifiVar->fgIsBoostCpuThAdjustable == TRUE) &&
				keep_alive &&
				(prBssInfo->eBand == BAND_2G4) &&
				(prBssInfo->eCoexMode == COEX_TDD_MODE) &&
				(prWifiVar->u4PerfMonTpTh[u4BoostCpuTh] >
					PERF_MON_COEX_TP_THRESHOLD) &&
				(kalGetTpMbpsByBssId(prAdapter, PKT_PATH_ALL, i)
					> PERF_MON_COEX_TP_THRESHOLD) &&
				(prWifiVar->u4PerfMonTpTh[2] ==
					PERF_MON_COEX_TP_THRESHOLD)) {
				/*  3, stands for 100Mbps */
				DBGLOG(SW4, INFO, "[Coex]CPUTh[3]\n");
				u4BoostCpuTh = 3;
			}
		}
	}

	prPerMonitor->u4TarPerfLevel = PERF_MON_TP_MAX_THRESHOLD;
	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.fgBoostCpuByPPSEn)) {
		maxTput = max(prPerMonitor->ulThroughput,
			prPerMonitor->ulThroughputInPPS);
	} else
		maxTput = prPerMonitor->ulThroughput;

	for (u4Idx = 0; u4Idx < PERF_MON_TP_MAX_THRESHOLD; u4Idx++) {
		if ((maxTput >> 20) <
			prAdapter->rWifiVar.u4PerfMonTpTh[u4Idx]) {
			prPerMonitor->u4TarPerfLevel = u4Idx;
			break;
		}
	}

	if (!wlan_perf_monitor_force_enable &&
			(wlan_fb_power_down ||
			prGlueInfo->fgIsInSuspendMode ||
			!keep_alive)) {
		DBGLOG(SW4, TRACE, "kalPerMonStop[%u][%u][%u][%u]\n",
			wlan_perf_monitor_force_enable, wlan_fb_power_down,
			prGlueInfo->fgIsInSuspendMode, keep_alive);
		kalPerMonStop(prGlueInfo);
		fgIsStopPerfMon = TRUE;
	} else {
		uint32_t u4CurrTputLv;

		if (prPerMonitor->u4UpdatePeriod < SEC_TO_MSEC(1)) {
			u4CurrTputLv = max(
				prPerMonitor->u4TarPerfLevel,
				prPerMonitor->u4CurrPerfLevel);
		} else {
			u4CurrTputLv =
				prPerMonitor->u4TarPerfLevel;
		}

		prPerMonitor->u4PrevPerfLevel =
			prPerMonitor->u4BoostPerfLevel;
		prPerMonitor->u4BoostPerfLevel = u4CurrTputLv;

#if CFG_SUPPORT_MCC_BOOST_CPU
		kalMccBoostCheck(prAdapter, u4CurrTputLv);
#endif /* CFG_SUPPORT_MCC_BOOST_CPU */

		if (kalCheckTputLoad(prAdapter,
			prPerMonitor->u4PrevPerfLevel,
			u4CurrTputLv,
			GLUE_GET_REF_CNT(prGlueInfo->i4TxPendingFrameNum),
			GLUE_GET_REF_CNT(prPerMonitor->u4UsedCnt))) {

			DBGLOG(SW4, INFO,
			"PerfMon overloading total:%3lu.%03lu mbps lv:%u->%u th:%u fg:0x%lx Pending[%d], Used[%d]\n",
			(unsigned long) (maxTput >> 20),
			(unsigned long) ((maxTput >> 10)
					& BITS(0, 9)),
			prPerMonitor->u4PrevPerfLevel,
			u4CurrTputLv,
			u4BoostCpuTh,
			prPerMonitor->ulPerfMonFlag,
			GLUE_GET_REF_CNT(prGlueInfo->i4TxPendingFrameNum),
			GLUE_GET_REF_CNT(prPerMonitor->u4UsedCnt));

			/* boost current level due to overloading */
			kalBoostCpu(prAdapter, u4CurrTputLv,
				u4CurrTputLv);
		} else if ((((u4CurrTputLv != prPerMonitor->u4PrevPerfLevel)
			&& !kalCheckBoostCpuMargin(prAdapter))
#if CFG_SUPPORT_MCC_BOOST_CPU
			|| kalIsMccStateChange(prAdapter)
#endif /* CFG_SUPPORT_MCC_BOOST_CPU */
			) && (u4BoostCpuTh < PERF_MON_TP_MAX_THRESHOLD)) {
			DBGLOG(SW4, INFO,
			"PerfMon total:%3lu.%03lu mbps lv:%u->%u th:%u fg:0x%lx\n",
			(unsigned long) (maxTput >> 20),
			(unsigned long) ((maxTput >> 10)
					& BITS(0, 9)),
			prPerMonitor->u4PrevPerfLevel,
			u4CurrTputLv,
			u4BoostCpuTh,
			prPerMonitor->ulPerfMonFlag);

#if CFG_SUPPORT_MCC_BOOST_CPU
			if (kalIsMccBoost(prAdapter)) {
				kalBoostCpu(prAdapter,
					PERF_MON_TP_MAX_THRESHOLD - 1,
					PERF_MON_TP_MAX_THRESHOLD - 1);
			} else
#endif /* CFG_SUPPORT_MCC_BOOST_CPU */
			kalBoostCpu(prAdapter, u4CurrTputLv,
				u4BoostCpuTh);
		} else {
			kalBoostCpuPolicy(prAdapter);
		}

/* switch pcie gen */
#if CFG_SUPPORT_PCIE_GEN_SWITCH
		kalSetPcieGen(prAdapter);
#endif

#if (CFG_COALESCING_INTERRUPT == 1)
		u4CoalescingIntTh =
			prAdapter->rWifiVar.u4PerfMonTpCoalescingIntTh;

		if ((u4CurrTputLv != prPerMonitor->u4PrevPerfLevel) &&
			(u4CoalescingIntTh <
			 PERF_MON_TP_MAX_THRESHOLD)) {

			DBGLOG(SW4, INFO,
			"PerfMon %3lu.%03lu mbps lv:%u CoalesTh:%u fg:0x%lx\n",
			(unsigned long) (maxTput >> 20),
			(unsigned long) ((maxTput >> 10)
					& BITS(0, 9)),
			u4CurrTputLv,
			u4CoalescingIntTh,
			prPerMonitor->ulPerfMonFlag);

			kalCoalescingInt(prAdapter,
				u4CurrTputLv,
				u4CoalescingIntTh);
		}
#endif

		prPerMonitor->u4UpdatePeriod =
			prAdapter->rWifiVar.u4PerfMonUpdatePeriod;

		cnmTimerStartTimer(prGlueInfo->prAdapter,
				&prPerMonitor->rPerfMonTimer,
				prPerMonitor->u4UpdatePeriod);

	}
	prPerMonitor->u4CurrPerfLevel =
		prPerMonitor->u4TarPerfLevel;

	/* do not add anything before boostcpu */
	prPerMonitor->u4RunCnt++;

	/* handle the case that u4UpdatePeriod is lower than 1s */
	if ((prPerMonitor->u4UpdatePeriod < SEC_TO_MSEC(1)) &&
		((prPerMonitor->u4RunCnt % prPerMonitor->u4TriggerCnt) != 0))
		return;

	if (prPerMonitor->fgIdle)
		goto end;

#if (CFG_SUPPORT_PERF_IND == 1)
	if (prWifiVar->fgPerfIndicatorEn &&
		!prGlueInfo->fgIsInSuspendMode)
		kalSetPerfReport(prAdapter);

	kalPerfIndReset(prAdapter);
#endif

#if CFG_SUPPORT_DATA_STALL
	/* test mode event */
	if (prWifiVar->u4ReportEventInterval == 0)
		KAL_REPORT_ERROR_EVENT(prAdapter,
			EVENT_TEST_MODE, 0, 0,  FALSE);
#endif

#if CFG_SUPPORT_LINK_QUALITY_MONITOR
	prAdapter->u4LinkQualityCounter++;
	if ((prAdapter->u4LinkQualityCounter %
	     CFG_LQ_MONITOR_FREQUENCY) == 0) {
		prAdapter->u4LinkQualityCounter = 0;
		if (prGlueInfo->fgIsInSuspendMode)
			DBGLOG(SW4, TRACE,
				"Skip wlanLinkQualityMonitor due to in suspend mode\n");
		else
			wlanLinkQualityMonitor(prGlueInfo, FALSE);
	}
#endif /* CFG_SUPPORT_LINK_QUALITY_MONITOR */

	/* check tx hang */
	if (!fgIsStopPerfMon) {
		prAdapter->u4HifChkFlag |= HIF_CHK_TX_HANG;
		kalSetHifDbgEvent(prAdapter->prGlueInfo);
	}

end:
	DBGLOG(SW4, TRACE, "exit kalPerMonHandler\n");
}

uint32_t kalPerMonGetInfo(struct ADAPTER *prAdapter,
			  uint8_t *pucBuf, uint32_t u4Max)
{
	struct PERF_MONITOR *prPerMonitor;
	uint32_t u4Len = 0;
	unsigned long ulWlanTxTpInBits, ulWlanRxTpInBits,
		 ulP2PTxTpInBits, ulP2PRxTpInBits;

	prPerMonitor = &prAdapter->rPerMonitor;

	ulWlanTxTpInBits = prPerMonitor->ulTxTp[0] << 3;
	ulWlanRxTpInBits = prPerMonitor->ulRxTp[0] << 3;
	ulP2PTxTpInBits = prPerMonitor->ulTxTp[1] << 3;
	ulP2PRxTpInBits = prPerMonitor->ulRxTp[1] << 3;

	LOGBUF(pucBuf, u4Max, u4Len,
	       "\nWi-Fi Throughput (update period %ums):\n",
	       prPerMonitor->u4UpdatePeriod);

	LOGBUF(pucBuf, u4Max, u4Len,
	       "wlan Tx: %3lu.%03lu mbps, Rx %3lu.%03lu mbps\n",
	       (ulWlanTxTpInBits >> 20),
	       ((ulWlanTxTpInBits >> 10) & BITS(0, 9)),
	       (ulWlanRxTpInBits >> 20),
	       ((ulWlanRxTpInBits >> 10) & BITS(0, 9)));

	LOGBUF(pucBuf, u4Max, u4Len,
	       "p2p  Tx: %3lu.%03lu mbps, Rx %3lu.%03lu mbps\n",
	       (ulP2PTxTpInBits >> 20), ((ulP2PTxTpInBits >> 10) & BITS(0,
					 9)),
	       (ulP2PRxTpInBits >> 20), ((ulP2PRxTpInBits >> 10) & BITS(0,
					 9)));

	LOGBUF(pucBuf, u4Max, u4Len, "Total: %3lu.%03lu mbps\n",
	       (prPerMonitor->ulThroughput >> 20),
	       ((prPerMonitor->ulThroughput >> 10) & BITS(0, 9)));

	LOGBUF(pucBuf, u4Max, u4Len,
	       "Performance level: %u threshold: %u flag: 0x%lx\n",
	       prPerMonitor->u4CurrPerfLevel,
	       prAdapter->rWifiVar.u4BoostCpuTh,
	       prPerMonitor->ulPerfMonFlag);

	return u4Len;
}

uint32_t kalGetTpMbpsByBssId(struct ADAPTER *prAdapter,
	enum ENUM_PKT_PATH ePath,
	uint8_t ucBssIdx)
{
	uint32_t u4TpMbps = 0;
	struct PERF_MONITOR *perf;

	if (!prAdapter
		|| ucBssIdx >= MAX_BSSID_NUM
		|| ePath > PKT_PATH_ALL)
		return 0;

	perf = &prAdapter->rPerMonitor;

	/* Get Tp per BSS */
	if (ePath == PKT_PATH_TX)
		u4TpMbps = (perf->ulTxTp[ucBssIdx] >> 17);
	else if (ePath == PKT_PATH_RX)
		u4TpMbps = (perf->ulRxTp[ucBssIdx] >> 17);
	else /* PKT_PATH_ALL */
		u4TpMbps = (perf->ulTxTp[ucBssIdx] >> 17)
			+ (perf->ulRxTp[ucBssIdx] >> 17);

	return u4TpMbps;
}

uint32_t kalGetTpMbps(struct ADAPTER *prAdapter,
	enum ENUM_PKT_PATH ePath)
{
	uint32_t u4TpMbps = 0;
	uint8_t i = 0;

	for (i = 0; i < MAX_BSSID_NUM; i++)
		u4TpMbps += kalGetTpMbpsByBssId(prAdapter, ePath, i);

	return u4TpMbps;
}

u_int8_t kalIsTxHighTput(struct ADAPTER *prAdapter)
{
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;

	if (kalGetTpMbps(prAdapter, PKT_PATH_TX) < prWifiVar->u4TxHighTputTh)
		return FALSE;

	return TRUE;
}

u_int8_t kalIsRxHighTput(struct ADAPTER *prAdapter)
{
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;

	if (kalGetTpMbps(prAdapter, PKT_PATH_RX) < prWifiVar->u4RxHighTputTh)
		return FALSE;

	return TRUE;
}

#if CFG_SUPPORT_DISABLE_DATA_DDONE_INTR
u_int8_t kalIsTputMode(struct ADAPTER *prAdapter,
	enum ENUM_PKT_PATH ePath,
	uint8_t ucBssIdx)
{
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	uint32_t u4TpMbps = 0;
	uint8_t ucRet = FALSE;

	if (ucBssIdx == MAX_BSSID_NUM)
		u4TpMbps = kalGetTpMbps(prAdapter, ePath);
	else
		u4TpMbps = kalGetTpMbpsByBssId(prAdapter, ePath, ucBssIdx);

	if (u4TpMbps > prWifiVar->u4TputThresholdMbps)
		ucRet = TRUE;

	return ucRet;
}
#endif /* CFG_SUPPORT_DISABLE_DATA_DDONE_INTR */

void __weak kalSetRunOnNonXCore(struct task_struct *task)
{
}

void __weak kalSetCpuBoost(struct ADAPTER *prAdapter,
		struct BOOST_INFO *prBoostInfo)
{
	DBGLOG(SW4, INFO, "enter kalSetCpuBoost\n");
}

void __weak kalBoostCpuInit(struct ADAPTER *prAdapter)
{
}

int32_t __weak kalBoostCpu(struct ADAPTER *prAdapter,
			   uint32_t u4TarPerfLevel, uint32_t u4BoostCpuTh)
{
	DBGLOG(SW4, INFO, "enter kalBoostCpu\n");
	return 0;
}

int32_t __weak kalBoostCpuPolicy(struct ADAPTER *prAdapter)
{
	return 0;
}

u_int8_t __weak kalCheckBoostCpuMargin(struct ADAPTER *prAdapter)
{
	return FALSE;
}

uint32_t __weak kalGetCpuBoostThreshold(void)
{
	DBGLOG(SW4, WARN, "enter kalGetCpuBoostThreshold\n");
	/*  1, stands for 20Mbps */
	return 1;
}

#if CFG_SUPPORT_LITTLE_CPU_BOOST
uint32_t __weak kalGetLittleCpuBoostThreshold(void)
{
	return kalGetCpuBoostThreshold();
}
#endif /* CFG_SUPPORT_LITTLE_CPU_BOOST */

uint32_t __weak kalGetChipID(void)
{
	/* SOC7_0_CHIP_ID = 0x70707070 */
	return 0x70707070;
}

uint32_t __weak kalGetConnsysVersion(void)
{
	/* SOC7_CONNSYS_VERSION_ID = 0x02050100 */
	return 0x02050100;
}

uint32_t __weak kalGetWfIpVersion(void)
{
	/* SOC7_WFSYS_VERSION_ID = 0x02020100 */
	return 0x02040100;
}

uint32_t __weak kalGetFwVerOffset(void)
{
	DBGLOG(SW4, WARN, "NO firmware version build.\n");
	return 0;
}

uint32_t __weak kalGetEmiMetOffset(void)
{
	DBGLOG(SW4, WARN, "enter kalGetEmiMetOffset\n");
	return 0;
}

uint32_t __weak kalGetProjectId(void)
{
	DBGLOG(SW4, WARN, "enter kalGetProjectId\n");
	return 0;
}

void __weak kalDumpPlatGPIOStat(void)
{
	DBGLOG(SW4, WARN, "enter kalDumpPlatGPIOStat\n");
}

void __weak kalSetEmiMetOffset(uint32_t newEmiMetOffset)
{
	DBGLOG(SW4, WARN, "enter kalSetEmiMetOffset\n");
}

int32_t __weak kalSetCpuNumFreq(uint32_t u4CoreNum,
				uint32_t u4Freq)
{
	DBGLOG(SW4, INFO,
	       "enter weak kalSetCpuNumFreq, u4CoreNum:%d, urFreq:%d\n",
	       u4CoreNum, u4Freq);
	return 0;
}

int32_t __weak kalGetFwFlavorByPlat(uint8_t *flavor)
{
	DBGLOG(SW4, TRACE, "NO firmware flavor build.\n");
	return 0;
}

int32_t kalGetFwFlavor(uint8_t *flavor)
{
	struct mt66xx_hif_driver_data *prDriverData;
	uint32_t u4StrLen = 0;

	prDriverData = get_platform_driver_data();

	if (prDriverData->fw_flavor) {
		u4StrLen = kalStrLen(prDriverData->fw_flavor);
		if (u4StrLen >= CFG_FW_FLAVOR_MAX_LEN) {
			DBGLOG(SW4, WARN,
				"get flavor length=%u over %u\n",
				u4StrLen, CFG_FW_FLAVOR_MAX_LEN);
			return WLAN_STATUS_FAILURE;
		}

		kalMemCopy(flavor, prDriverData->fw_flavor, u4StrLen);
		DBGLOG(SW4, TRACE, "kalGetFwFlavor:%s (%u)\n",
			flavor, u4StrLen);
		return 1;
	}

	return kalGetFwFlavorByPlat(flavor);
}

int32_t __weak kalGetConnsysVerId(void)
{
	DBGLOG(SW4, WARN, "NO CONNSYS version ID.\n");
	return 0;
}

void __weak kalSetEmiMpuProtection(phys_addr_t emiPhyBase, bool enable)
{
	DBGLOG(SW4, WARN, "EMI MPU function is not defined\n");
}

void __weak kalSetDrvEmiMpuProtection(phys_addr_t emiPhyBase, uint32_t offset,
				      uint32_t size)
{
	DBGLOG(SW4, WARN, "DRV EMI MPU function is not defined\n");
}

int32_t __weak kalCheckTputLoad(struct ADAPTER *prAdapter,
			 uint32_t u4CurrPerfLevel,
			 uint32_t u4TarPerfLevel,
			 int32_t i4Pending,
			 uint32_t u4Used)
{
	DBGLOG(SW4, TRACE, "enter kalCheckTputLoad\n");
	return FALSE;
}

int32_t __weak kalCheckVcoreBoost(struct ADAPTER *prAdapter,
				  uint8_t uBssIndex)
{
	return FALSE;
}

int32_t __weak kalPlatOpsInit(void)
{
	return 0;
}

int32_t __weak kalGetScpDumpInfo(u64 *addr, unsigned int *size)
{
	return 0;
}

#if CFG_MTK_WIFI_PCIE_SR
u_int8_t __weak kalIsSupportPcieL2(void)
{
	return FALSE;
}
#endif

#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
u_int8_t __weak kalIsSupportMawd(void)
{
	return FALSE;
}

u_int8_t __weak kalIsSupportSdo(void)
{
#if CFG_SUPPORT_WED_PROXY
	return TRUE;
#else
	return FALSE;
#endif
}

u_int8_t __weak kalIsSupportRro(void)
{
	return FALSE;
}

uint32_t __weak kalGetMawdVer(void)
{
	return 0;
}

uint32_t __weak kalGetConnInfraId(void)
{
	return 0;
}
#endif /* CFG_SUPPORT_HOST_OFFLOAD */

uint32_t __weak kalGetTxBigCpuMask(void)
{
	return 0xFF;
}

/* mimic store_rps_map as net-sysfs.c does */
int wlan_set_rps_map(struct netdev_rx_queue *queue, unsigned long rps_value)
{
#if KERNEL_VERSION(4, 14, 0) <= CFG80211_VERSION_CODE
	struct rps_map *old_map, *map;
	cpumask_var_t mask;
	int cpu, i;
	static DEFINE_MUTEX(rps_map_mutex);

	if (!alloc_cpumask_var(&mask, GFP_KERNEL))
		return -ENOMEM;

	*cpumask_bits(mask) = rps_value;
	map = kzalloc(max_t(unsigned int,
			RPS_MAP_SIZE(cpumask_weight(mask)), L1_CACHE_BYTES),
			GFP_KERNEL);
	if (!map) {
		free_cpumask_var(mask);
		return -ENOMEM;
	}

	i = 0;
	for_each_cpu_and(cpu, mask, cpu_online_mask)
		map->cpus[i++] = cpu;

	if (i) {
		map->len = i;
	} else {
		kfree(map);
		map = NULL;
		DBGLOG(RX, WARN, "Empty RPS bits, rps=0x%lx; cpu=0x%lx\n",
			rps_value, cpu_online_mask->bits[0]);
		return -EINVAL;
	}

	mutex_lock(&rps_map_mutex);
	old_map = rcu_dereference_protected(queue->rps_map,
				mutex_is_locked(&rps_map_mutex));
	rcu_assign_pointer(queue->rps_map, map);
	if (map)
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		static_branch_inc(&rps_needed);
#else
		static_key_slow_inc(&rps_needed);
#endif
	if (old_map)
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		static_branch_dec(&rps_needed);
#else
		static_key_slow_dec(&rps_needed);
#endif
	mutex_unlock(&rps_map_mutex);

	if (old_map)
		kfree_rcu(old_map, rcu);
	free_cpumask_var(mask);

	return 0;
#else
	return 0;
#endif
}

void kalSetRpsMap(struct GLUE_INFO *glue, unsigned long value)
{
	int32_t i = 0, j = 0;
	struct net_device *dev = NULL;

	for (i = 0; i < MAX_BSSID_NUM; i++) {
		dev = wlanGetNetDev(glue, i);
		if (dev) {
			DBGLOG(INIT, INFO, "Set bss %u(%s) RPS 0x%02x\n",
			       i, dev->name, value);
			for (j = 0; j < dev->real_num_rx_queues; ++j)
				wlan_set_rps_map(&dev->_rx[j], value);
		}
	}
}

#if CFG_SUPPORT_SKIP_RX_GRO_FOR_TC
static u_int8_t kalCheckIfTcApplied(struct net_device *prNetDev)
{
	struct netdev_queue *ingress_q;
	struct Qdisc *ingress_qdisc;
	const struct Qdisc_ops *qdisc_ops;
	const struct Qdisc_class_ops *qdisc_cops;
	struct tcf_block *ingress_block;
	u_int8_t fgRet = FALSE;

	if (!prNetDev) {
		DBGLOG(INIT, INFO, "prNetDev is NULL\n");
		return fgRet;
	}

	rcu_read_lock();

	ingress_q = rcu_dereference(prNetDev->ingress_queue);
	if (unlikely(!ingress_q))
		goto unlock;

	ingress_qdisc = rcu_dereference(ingress_q->qdisc);
	if (unlikely(!ingress_qdisc))
		goto unlock;

	qdisc_ops = ingress_qdisc->ops;
	if (!kalStrCmp(qdisc_ops->id, "clsact")) {
		qdisc_cops = qdisc_ops->cl_ops;
		if (unlikely(!qdisc_cops))
			goto unlock;
		if (unlikely(!(qdisc_cops->tcf_block)))
			goto unlock;

#if (KERNEL_VERSION(4, 19, 0) <= CFG80211_VERSION_CODE)
		ingress_block = qdisc_cops->tcf_block(ingress_qdisc,
					TC_H_MIN_INGRESS, NULL);
#else
		ingress_block = qdisc_cops->tcf_block(ingress_qdisc,
					TC_H_MIN_INGRESS);
#endif
		if (unlikely(!ingress_block))
			goto unlock;

		/* chain list will be empty when there is no filter */
		if (!list_empty(&(ingress_block->chain_list)))
			fgRet = TRUE;
	}
unlock:
	rcu_read_unlock();
	return fgRet;
}

static u_int8_t kalGetSkipRxGro(struct net_device *prNetDev)
{
	struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPrivate;

	prNetDevPrivate = (struct NETDEV_PRIVATE_GLUE_INFO *)
		netdev_priv(prNetDev);

	return prNetDevPrivate->fgSkipRxGro;
}

static void kalSetSkipRxGro(struct net_device *prNetDev, u_int8_t fgSkipRxGro)
{
	struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPrivate;

	prNetDevPrivate = (struct NETDEV_PRIVATE_GLUE_INFO *)
		netdev_priv(prNetDev);

	prNetDevPrivate->fgSkipRxGro = fgSkipRxGro;
}

static void kalRxGroTcCheck(struct GLUE_INFO *glue)
{
	uint8_t i;
	struct net_device *dev;
	u_int8_t fgSkipGro, fgTcApplied;

	for (i = 0; i < MAX_BSSID_NUM; i++) {
		dev = wlanGetNetDev(glue, i);
		if (!dev)
			continue;

		fgSkipGro = kalGetSkipRxGro(dev);
		fgTcApplied = kalCheckIfTcApplied(dev);

		/* skip gro when tc is applied */
		if (fgTcApplied != fgSkipGro) {
			kalSetSkipRxGro(dev, fgTcApplied);
			DBGLOG(INIT, INFO,
				"B:[%u] SkipRxGro:[%u->%u]",
				i,
				fgSkipGro, fgTcApplied);
		}
	}
}
#endif /* CFG_SUPPORT_SKIP_RX_GRO_FOR_TC */

int32_t kalPerMonSetForceEnableFlag(uint8_t uFlag)
{
	struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *)
				       wlan_fb_notifier_priv_data;

	wlan_perf_monitor_force_enable = uFlag == 0 ? FALSE : TRUE;
	DBGLOG(SW4, INFO,
	       "uFlag:%d, wlan_perf_monitor_ctrl_flag:%d\n", uFlag,
	       wlan_perf_monitor_force_enable);

	if (wlan_perf_monitor_force_enable && prGlueInfo
	    && !kalIsHalted())
		kalPerMonEnable(prGlueInfo);

	return 0;
}

#if CFG_MTK_ANDROID_WMT && \
	KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
enum ENUM_WLAN_FB_EVENT kalGetMtkDispEvent(const uint8_t *fn,
	unsigned long event, void *data)
{
	int32_t blank = 0;
	int32_t *pData = (int32_t *)data;
	enum ENUM_WLAN_FB_EVENT eEvent = WLAN_FB_EVENT_IGNORE;

	if (event != MTK_DISP_EARLY_EVENT_BLANK)
		goto end;

	blank = *pData;

	if (blank == MTK_DISP_BLANK_UNBLANK)
		eEvent = WLAN_FB_EVENT_UNBLANK;
	else if (blank == MTK_DISP_BLANK_POWERDOWN)
		eEvent = WLAN_FB_EVENT_POWERDOWN;

	/* use KAL_TRACE to print caller (request by mtk disp owner) */
	DBGLOG(SW4, INFO, "%s: event[%lu], blank[%d] eEvent[%u]\n",
		fn, event, blank, eEvent);
end:
	return eEvent;
}
#else /* CFG_MTK_ANDROID_WMT */
enum ENUM_WLAN_FB_EVENT kalGetFbEvent(const uint8_t *fn,
	unsigned long event, void *data)
{
	int32_t blank = 0;
	struct fb_event *evdata = data;
	enum ENUM_WLAN_FB_EVENT eEvent = WLAN_FB_EVENT_IGNORE;

	if (event != FB_EVENT_BLANK)
		goto end;

	blank = *(int32_t *)evdata->data;

	if (blank == FB_BLANK_UNBLANK)
		eEvent = WLAN_FB_EVENT_UNBLANK;
	else if (blank == FB_BLANK_POWERDOWN)
		eEvent = WLAN_FB_EVENT_POWERDOWN;

	/* use KAL_TRACE to print caller (request by mtk disp owner) */
	DBGLOG(SW4, INFO, "%s: event[%lu] blank[%d] eEvent[%u]\n",
		fn, event, blank, eEvent);
end:
	return eEvent;
}
#endif /* CFG_MTK_ANDROID_WMT */

static int wlan_fb_notifier_callback(struct notifier_block
				     *self, unsigned long event, void *data)
{
	struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *)
				       wlan_fb_notifier_priv_data;
	enum ENUM_WLAN_FB_EVENT eEvent = WLAN_FB_EVENT_IGNORE;

	if (!prGlueInfo)
		goto end;

#if CFG_MTK_ANDROID_WMT && \
	KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
	eEvent = kalGetMtkDispEvent(__func__, event, data);
#else /* CFG_MTK_ANDROID_WMT */
	eEvent = kalGetFbEvent(__func__, event, data);
#endif /* CFG_MTK_ANDROID_WMT */

	if (eEvent == WLAN_FB_EVENT_IGNORE)
		goto end;

	if (kalHaltTryLock())
		goto end;

	if (kalIsHalted()) {
		kalHaltUnlock();
		goto end;
	}

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
			WLAN_DRV_READY_CHECK_RESET)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return 0;
	}

	switch (eEvent) {
	case WLAN_FB_EVENT_UNBLANK:
		kalSetPerMonEnable(prGlueInfo);
		wlan_fb_power_down = FALSE;
		break;
	case WLAN_FB_EVENT_POWERDOWN:
		wlan_fb_power_down = TRUE;
		if (!wlan_perf_monitor_force_enable)
			kalSetPerMonDisable(prGlueInfo);
		break;
	default:
		break;
	}

	kalHaltUnlock();
	DBGLOG(SW4, INFO, "%s: end\n", __func__);
end:
	return 0;
}

int32_t kalFbNotifierReg(struct GLUE_INFO *prGlueInfo)
{
	int32_t i4Ret = -1;

	wlan_fb_notifier_priv_data = prGlueInfo;

#if IS_ENABLED(CONFIG_DRM_MEDIATEK_V2)
#if CFG_MTK_ANDROID_WMT && \
	KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
	i4Ret = mtk_disp_notifier_register("wlan_fb_notifier",
			&wlan_fb_notifier);
#else
	i4Ret = fb_register_client(&wlan_fb_notifier);
#endif
#endif
	if (i4Ret)
		DBGLOG(SW4, WARN, "Register wlan_fb_notifier failed:%d\n",
		       i4Ret);
	else
		DBGLOG(SW4, TRACE, "Register wlan_fb_notifier succeed\n");
	return i4Ret;
}

void kalFbNotifierUnReg(void)
{
#if IS_ENABLED(CONFIG_DRM_MEDIATEK_V2)
#if CFG_MTK_ANDROID_WMT && \
	KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
	mtk_disp_notifier_unregister(&wlan_fb_notifier);
#else
	fb_unregister_client(&wlan_fb_notifier);
#endif
#endif
	wlan_fb_notifier_priv_data = NULL;
}

#if CFG_SUPPORT_DFS
void kalIndicateAllQueueTxAllowed(struct GLUE_INFO *prGlueInfo,
	uint8_t ucBssIndex, uint8_t fgIsTxAllowed)
{
	struct net_device *prDevHandler;
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;

	prDevHandler = wlanGetNetDev(prAdapter->prGlueInfo, ucBssIndex);
	if (!prDevHandler) {
		DBGLOG(REQ, ERROR,
			"NetDev is null BssIndex[%d]!\n", ucBssIndex);
		return;
	}

	DBGLOG(REQ, INFO, "[%s] Kernel queue TxAllowed = %d\n",
		prDevHandler->name, fgIsTxAllowed);

	if (fgIsTxAllowed)
		netif_tx_wake_all_queues(prDevHandler);
	else
		netif_tx_stop_all_queues(prDevHandler);
}

void __kalIndicateChannelSwitch(struct GLUE_INFO *prGlueInfo,
				enum ENUM_CHNL_EXT eSco,
				uint8_t ucChannelNum,
				enum ENUM_BAND eBand,
				uint8_t ucBssIndex)
{
	struct cfg80211_chan_def chandef;
	struct ieee80211_channel *prChannel = NULL;
	struct net_device *prDevHandler;
	enum nl80211_channel_type rChannelType;
	uint8_t band = 0;
	struct BSS_INFO *prBssInfo;
	uint8_t linkIdx = 0;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prGlueInfo->prAdapter,
		ucBssIndex);
	if (prBssInfo)
		linkIdx = prBssInfo->ucLinkIndex;

	if (eBand > BAND_NULL && eBand < BAND_NUM)
		band = aucBandTranslate[eBand];
	else {
		DBGLOG(REQ, ERROR, "Invalid band:%d!\n", eBand);
		return;
	}
	prChannel = ieee80211_get_channel(
			wlanGetWiphy(),
			ieee80211_channel_to_frequency(ucChannelNum, band));

	if (!prChannel) {
		DBGLOG(REQ, ERROR, "ieee80211_get_channel fail!\n");
		return;
	}

	prDevHandler = wlanGetNetDev(prGlueInfo, ucBssIndex);
	if (!prDevHandler) {
		DBGLOG(REQ, ERROR,
			"NetDev is null BssIndex[%d]!\n", ucBssIndex);
		return;
	}

	switch (eSco) {
	case CHNL_EXT_SCN:
		rChannelType = NL80211_CHAN_NO_HT;
		break;

	case CHNL_EXT_SCA:
		rChannelType = NL80211_CHAN_HT40MINUS;
		break;

	case CHNL_EXT_SCB:
		rChannelType = NL80211_CHAN_HT40PLUS;
		break;

	case CHNL_EXT_RES:
	default:
		rChannelType = NL80211_CHAN_HT20;
		break;
	}

	DBGLOG(REQ, STATE, "DFS channel switch to %d\n", ucChannelNum);

	cfg80211_chandef_create(&chandef, prChannel, rChannelType);

#if (KERNEL_VERSION(6, 7, 0) <= CFG80211_VERSION_CODE)
	wiphy_lock(prDevHandler->ieee80211_ptr->wiphy);
#else
	mutex_lock(&prDevHandler->ieee80211_ptr->mtx);
#endif

#if (KERNEL_VERSION(6, 8, 0) <= CFG80211_VERSION_CODE)
	cfg80211_ch_switch_notify(prDevHandler, &chandef,
		linkIdx);
#elif (KERNEL_VERSION(6, 3, 0) <= CFG80211_VERSION_CODE)
	cfg80211_ch_switch_notify(prDevHandler, &chandef,
		linkIdx, 0);
#elif (CFG_ADVANCED_80211_MLO == 1)
	cfg80211_ch_switch_notify(prDevHandler, &chandef,
		linkIdx, 0);
#elif (KERNEL_VERSION(5, 19, 2) <= CFG80211_VERSION_CODE)
	cfg80211_ch_switch_notify(prDevHandler, &chandef,
		linkIdx);
#else
	cfg80211_ch_switch_notify(prDevHandler, &chandef);
#endif

#if (KERNEL_VERSION(6, 7, 0) <= CFG80211_VERSION_CODE)
	wiphy_unlock(prDevHandler->ieee80211_ptr->wiphy);
#else
	mutex_unlock(&prDevHandler->ieee80211_ptr->mtx);
#endif

	/* Check SAP channel */
	p2pFuncSwitchSapChannel(prGlueInfo->prAdapter,
		P2P_DEFAULT_SCENARIO);
}
#if (KERNEL_VERSION(6, 6, 0) <= CFG80211_VERSION_CODE)
void kalAisChnlSwitchNotifyWork(struct work_struct *work)
{
	struct GL_CH_SWITCH_WORK *prWorkContainer =
		CONTAINER_OF(work, struct GL_CH_SWITCH_WORK,
			rChSwitchNotifyWork);
	struct GLUE_INFO *prGlueInfo = wlanGetGlueInfo();
	struct ADAPTER *prAdapter;
	struct BSS_INFO *prBssInfo;

	if (!prGlueInfo ||
		prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return;
	}
	prAdapter = prGlueInfo->prAdapter;
	prBssInfo =
		CONTAINER_OF(prWorkContainer, struct BSS_INFO, rGlChSwitchWork);

	__kalIndicateChannelSwitch(prAdapter->prGlueInfo,
				prBssInfo->eBssSCO,
				prBssInfo->ucPrimaryChannel,
				prBssInfo->eBand,
				prBssInfo->ucBssIndex);
}
#endif

void kalAisCsaNotifyWorkInit(struct ADAPTER *prAdapter,
			uint8_t ucBssIndex)
{
#if (KERNEL_VERSION(6, 6, 0) <= CFG80211_VERSION_CODE)
	struct BSS_INFO *prBssInfo;

	prBssInfo =
		GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo)
		return;
	INIT_WORK(&(prBssInfo->rGlChSwitchWork.rChSwitchNotifyWork),
		kalAisChnlSwitchNotifyWork);
	prBssInfo->rGlChSwitchWork.fgWorkInit = TRUE;
#endif
}

void kalCsaNotifyWorkDeinit(struct ADAPTER *prAdapter,
			uint8_t ucBssIndex)
{
#if (KERNEL_VERSION(6, 6, 0) <= CFG80211_VERSION_CODE)
	struct BSS_INFO *prBssInfo;

	prBssInfo =
		GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo ||
		prBssInfo->rGlChSwitchWork.fgWorkInit == FALSE)
		return;
	cancel_work_sync(
		&prBssInfo->rGlChSwitchWork.rChSwitchNotifyWork);
	prBssInfo->rGlChSwitchWork.fgWorkInit = FALSE;
#endif
}



void kalIndicateChannelSwitch(struct GLUE_INFO *prGlueInfo,
			enum ENUM_CHNL_EXT eSco,
			uint8_t ucChannelNum, enum ENUM_BAND eBand,
			uint8_t ucBssIndex)
{

#if (KERNEL_VERSION(6, 6, 0) <= CFG80211_VERSION_CODE)
	struct ADAPTER *prAdapter;
	struct BSS_INFO *prBssInfo;

	prAdapter = prGlueInfo->prAdapter;
	prBssInfo =
		prAdapter->aprBssInfo[ucBssIndex];
	schedule_work(&prBssInfo->rGlChSwitchWork.rChSwitchNotifyWork);
#else
	__kalIndicateChannelSwitch(prGlueInfo,
				eSco,
				ucChannelNum,
				eBand,
				ucBssIndex);
#endif

}

#endif

void kalInitDevWakeup(struct ADAPTER *prAdapter, struct device *prDev)
{
	/*
	 * The remote wakeup function will be disabled after
	 * first time resume, we need to call device_init_wakeup()
	 * to notify usbcore that we support wakeup function,
	 * so usbcore will re-enable our remote wakeup function
	 * before entering suspend.
	 */
#if CFG_WOW_SUPPORT
	if (prAdapter->rWifiVar.ucWow &&
		(prAdapter->rWowCtrl.astWakeHif[0].ucWakeupHif
		!= ENUM_HIF_TYPE_GPIO))
		device_init_wakeup(prDev, TRUE);
#endif
}

u_int8_t kalIsOuiMask(const uint8_t pucMacAddrMask[MAC_ADDR_LEN])
{
	return (pucMacAddrMask[0] == 0xFF &&
		pucMacAddrMask[1] == 0xFF &&
		pucMacAddrMask[2] == 0xFF);
}

u_int8_t kalIsValidMacAddr(const uint8_t *addr)
{
	return (addr != NULL) && is_valid_ether_addr(addr);
}

#if (KERNEL_VERSION(3, 19, 0) <= CFG80211_VERSION_CODE)
u_int8_t kalParseRandomMac(const struct net_device *ndev,
		uint8_t *pucMacAddr, uint8_t *pucMacAddrMask,
		uint8_t *pucRandomMac)
{
	struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPrivate = NULL;
	struct ADAPTER *prAdapter = NULL;
	uint8_t ucBssIndex;
	struct BSS_INFO *prBssInfo;
	uint8_t ucMacAddr[MAC_ADDR_LEN];

	if (!ndev) {
		log_dbg(SCN, ERROR, "Invalid net device\n");
		return FALSE;
	}

	prNetDevPrivate =
		(struct NETDEV_PRIVATE_GLUE_INFO *) netdev_priv(ndev);

	if (!prNetDevPrivate || !(prNetDevPrivate->prGlueInfo)
		|| !(prNetDevPrivate->prGlueInfo->prAdapter)) {
		log_dbg(SCN, ERROR, "Invalid private param\n");
		return FALSE;
	}

	prAdapter = prNetDevPrivate->prGlueInfo->prAdapter;
	ucBssIndex = prNetDevPrivate->ucBssIdx;
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

	if (!prBssInfo) {
		log_dbg(SCN, WARN, "Invalid bss info (ind=%u)\n", ucBssIndex);
		return FALSE;
	}

	if (pucMacAddr)
		COPY_MAC_ADDR(ucMacAddr, pucMacAddr);
	else
		eth_zero_addr(ucMacAddr);

	/* Randomize all 6-bytes MAC.
	 * Not to keep first 3-bytes MAC OUI to be constant.
	 */
	get_random_mask_addr(pucRandomMac, ucMacAddr, pucMacAddrMask);

	return TRUE;
}

u_int8_t kalScanParseRandomMac(const struct net_device *ndev,
	const struct cfg80211_scan_request *request, uint8_t *pucRandomMac)
{
	uint8_t ucMacAddr[MAC_ADDR_LEN];
	uint8_t ucMacAddrMask[MAC_ADDR_LEN];

	ASSERT(request);
	ASSERT(pucRandomMac);

	if (!(request->flags & NL80211_SCAN_FLAG_RANDOM_ADDR)) {
		log_dbg(SCN, TRACE, "Scan random mac is not set\n");
		return FALSE;
	}
#if KERNEL_VERSION(4, 10, 0) <= CFG80211_VERSION_CODE
	{
		if (kalIsValidMacAddr(request->bssid)) {
			COPY_MAC_ADDR(pucRandomMac, request->bssid);
			log_dbg(SCN, INFO, "random mac=" MACSTR "\n",
				MAC2STR(pucRandomMac));
			return TRUE;
		}
	}
#endif
	COPY_MAC_ADDR(ucMacAddr, request->mac_addr);
	COPY_MAC_ADDR(ucMacAddrMask, request->mac_addr_mask);

	return kalParseRandomMac(ndev, ucMacAddr, ucMacAddrMask, pucRandomMac);
}

u_int8_t kalSchedScanParseRandomMac(const struct net_device *ndev,
	const struct cfg80211_sched_scan_request *request,
	uint8_t *pucRandomMac, uint8_t *pucRandomMacMask)
{
	uint8_t ucMacAddr[MAC_ADDR_LEN];

	ASSERT(request);
	ASSERT(pucRandomMac);
	ASSERT(pucRandomMacMask);

	if (!(request->flags & NL80211_SCAN_FLAG_RANDOM_ADDR)) {
		log_dbg(SCN, TRACE, "Scan random mac is not set\n");
		return FALSE;
	}
	COPY_MAC_ADDR(ucMacAddr, request->mac_addr);
	COPY_MAC_ADDR(pucRandomMacMask, request->mac_addr_mask);

	return kalParseRandomMac(ndev, ucMacAddr,
		pucRandomMacMask, pucRandomMac);
}
#else /* if (KERNEL_VERSION(3, 19, 0) <= CFG80211_VERSION_CODE) */
u_int8_t kalScanParseRandomMac(const struct net_device *ndev,
	const struct cfg80211_scan_request *request, uint8_t *pucRandomMac)
{
	return FALSE;
}

u_int8_t kalSchedScanParseRandomMac(const struct net_device *ndev,
	const struct cfg80211_sched_scan_request *request,
	uint8_t *pucRandomMac, uint8_t *pucRandomMacMask)
{
	return FALSE;
}
#endif

void kalScanReqLog(struct cfg80211_scan_request *request)
{
	char *strbuf = NULL, *pos = NULL, *end = NULL;
	uint32_t slen = 0;
	int i, snum, cnum;

	snum = min_t(int, request->n_ssids, CFG_SCAN_SSID_MAX_NUM + 1);
	cnum = min_t(u32, request->n_channels, MAXIMUM_OPERATION_CHANNEL_LIST);

	for (i = 0; i < snum; ++i) {
		if (request->ssids[i].ssid_len > 0)
			slen += request->ssids[i].ssid_len + 1;
	}

	/* The length should be added 11 + 15 + 8 + 1 for the format
	 * "n_ssids=%d:" and " n_channels=%u:" and 2 " ..." and null byte.
	 */
	slen += 35 + 4 * cnum;
	pos = strbuf = kalMemAlloc(slen, VIR_MEM_TYPE);
	if (strbuf == NULL) {
		scanlog_dbg(LOG_SCAN_REQ_K2D, INFO, "Can't allocate memory\n");
		return;
	}
	end = strbuf + slen;

	pos += kalSnprintf(pos, end - pos,
		"n_ssids=%d:", request->n_ssids % 100);
	for (i = 0; i < snum; ++i) {
		uint8_t len = request->ssids[i].ssid_len;
		char ssid[PARAM_MAX_LEN_SSID + 1] = {0};

		if (len == 0)
			continue;
		kalStrnCpy(ssid, request->ssids[i].ssid, sizeof(ssid) - 1);
		ssid[sizeof(ssid) - 1] = '\0';
		pos += kalSnprintf(pos, end - pos, " %s", ssid);
	}
	if (snum < request->n_ssids)
		pos += kalSnprintf(pos, end - pos, "%s", " ...");

	pos += kalSnprintf(pos, end - pos, " n_channels=%u:",
		request->n_channels % 100);
	for (i = 0; i < cnum; ++i) {
		pos += kalSnprintf(pos, end - pos, " %u",
			request->channels[i]->hw_value % 1000);
	}
	if (cnum < request->n_channels)
		pos += kalSnprintf(pos, end - pos, "%s", " ...");

#if (KERNEL_VERSION(3, 19, 0) <= CFG80211_VERSION_CODE)
	scanlog_dbg(LOG_SCAN_REQ_K2D, INFO, "Scan flags=0x%x [mac]addr="
		MACSTR " mask=" MACSTR " %s\n",
		request->flags,
		MAC2STR(request->mac_addr),
		MAC2STR(request->mac_addr_mask), strbuf);
#else
	scanlog_dbg(LOG_SCAN_REQ_K2D, INFO, "Scan flags=0x%x %s\n",
		request->flags, strbuf);
#endif

	kalMemFree(strbuf, VIR_MEM_TYPE, slen);
}

void kalScanResultLog(struct ADAPTER *prAdapter, struct ieee80211_mgmt *mgmt)
{
	KAL_SPIN_LOCK_DECLARATION();

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_BSSLIST_CFG);
	scanLogCacheAddBSS(
		&(prAdapter->rWifiVar.rScanInfo.rScanLogCache.rBSSListCFG),
		prAdapter->rWifiVar.rScanInfo.rScanLogCache.arBSSListBufCFG,
		LOG_SCAN_RESULT_D2K,
		mgmt->bssid,
		mgmt->seq_ctrl);
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_BSSLIST_CFG);
}

void kalScanLogCacheFlushBSS(struct ADAPTER *prAdapter,
	const uint16_t logBufLen)
{
	KAL_SPIN_LOCK_DECLARATION();

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_BSSLIST_CFG);
	scanLogCacheFlushBSS(
		&(prAdapter->rWifiVar.rScanInfo.rScanLogCache.rBSSListCFG),
		LOG_SCAN_DONE_D2K);
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_BSSLIST_CFG);
}


u_int8_t
kalChannelScoSwitch(enum nl80211_channel_type channel_type,
		enum ENUM_CHNL_EXT *prChnlSco)
{
	u_int8_t fgIsValid = FALSE;

	do {
		if (prChnlSco) {
			switch (channel_type) {
			case NL80211_CHAN_NO_HT:
				*prChnlSco = CHNL_EXT_SCN;
				break;
			case NL80211_CHAN_HT20:
				*prChnlSco = CHNL_EXT_SCN;
				break;
			case NL80211_CHAN_HT40MINUS:
				*prChnlSco = CHNL_EXT_SCA;
				break;
			case NL80211_CHAN_HT40PLUS:
				*prChnlSco = CHNL_EXT_SCB;
				break;
			default:
				ASSERT(FALSE);
				*prChnlSco = CHNL_EXT_SCN;
				break;
			}
		}
		fgIsValid = TRUE;
	} while (FALSE);

	return fgIsValid;
}

u_int8_t
kalChannelFormatSwitch(struct cfg80211_chan_def *channel_def,
		struct ieee80211_channel *channel,
		struct RF_CHANNEL_INFO *prRfChnlInfo)
{
	u_int8_t fgIsValid = FALSE;

	do {
		if (channel == NULL)
			break;

		DBGLOG(P2P, INFO, "switch channel band: %d, freq: %d\n",
				channel->band, channel->center_freq);

		if (prRfChnlInfo) {
			prRfChnlInfo->ucChannelNum =
				nicFreq2ChannelNum(channel->center_freq * 1000);

			switch (channel->band) {
			case KAL_BAND_2GHZ:
				prRfChnlInfo->eBand = BAND_2G4;
				break;
			case KAL_BAND_5GHZ:
				prRfChnlInfo->eBand = BAND_5G;
				break;
#if (CFG_SUPPORT_WIFI_6G == 1)
			case KAL_BAND_6GHZ:
				prRfChnlInfo->eBand = BAND_6G;
				break;
#endif
			default:
				prRfChnlInfo->eBand = BAND_2G4;
				break;
			}
		}

		if (channel_def && prRfChnlInfo) {
			switch (channel_def->width) {
			case NL80211_CHAN_WIDTH_20_NOHT:
			case NL80211_CHAN_WIDTH_20:
				prRfChnlInfo->ucChnlBw = MAX_BW_20MHZ;
				break;
			case NL80211_CHAN_WIDTH_40:
				prRfChnlInfo->ucChnlBw = MAX_BW_40MHZ;
				break;
			case NL80211_CHAN_WIDTH_80:
				prRfChnlInfo->ucChnlBw = MAX_BW_80MHZ;
				break;
			case NL80211_CHAN_WIDTH_80P80:
				prRfChnlInfo->ucChnlBw = MAX_BW_80_80_MHZ;
				break;
			case NL80211_CHAN_WIDTH_160:
				prRfChnlInfo->ucChnlBw = MAX_BW_160MHZ;
				break;
			default:
				prRfChnlInfo->ucChnlBw = MAX_BW_20MHZ;
				break;
			}
			prRfChnlInfo->u2PriChnlFreq = channel->center_freq;
			prRfChnlInfo->u4CenterFreq1 = channel_def->center_freq1;
			prRfChnlInfo->u4CenterFreq2 = channel_def->center_freq2;
		}

		fgIsValid = TRUE;
	} while (FALSE);

	return fgIsValid;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Notify kernel to remove/unlink bss.
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 * \param[in] prBssDesc      Pointer of BSS_DESC we want to remove
 *
 */
/*----------------------------------------------------------------------------*/
void kalRemoveBss(struct GLUE_INFO *prGlueInfo,
	uint8_t aucBSSID[],
	uint8_t ucChannelNum,
	enum ENUM_BAND eBand)
{
	struct cfg80211_bss *bss = NULL;
	struct ieee80211_channel *prChannel = NULL;

#if (CFG_SUPPORT_WIFI_6G == 1)
	if (eBand == BAND_6G) {
		prChannel =
			ieee80211_get_channel(
				wlanGetWiphy(),
				ieee80211_channel_to_frequency
				(ucChannelNum, KAL_BAND_6GHZ));
	} else
#endif
	if (ucChannelNum <= 14) {
		prChannel = ieee80211_get_channel(
			wlanGetWiphy(),
			ieee80211_channel_to_frequency(
				ucChannelNum,
				KAL_BAND_2GHZ)
		);
	} else {
		prChannel = ieee80211_get_channel(
			wlanGetWiphy(),
			ieee80211_channel_to_frequency(
				ucChannelNum,
				KAL_BAND_5GHZ)
		);
	}

#if (KERNEL_VERSION(4, 1, 0) <= CFG80211_VERSION_CODE)
	bss = cfg80211_get_bss(wlanGetWiphy(),
			prChannel, /* channel */
			aucBSSID,
			NULL, /* ssid */
			0, /* ssid length */
			IEEE80211_BSS_TYPE_ESS,
			IEEE80211_PRIVACY_ANY);
#else
	bss = cfg80211_get_bss(wlanGetWiphy(),
			prChannel, /* channel */
			aucBSSID,
			NULL, /* ssid */
			0, /* ssid length */
			WLAN_CAPABILITY_ESS,
			WLAN_CAPABILITY_ESS);
#endif

	if (bss != NULL) {
		cfg80211_unlink_bss(wlanGetWiphy(), bss);
		cfg80211_put_bss(wlanGetWiphy(), bss);
	}
}

int kalMaskMemCmp(const void *cs, const void *ct,
	const void *mask, size_t count)
{
	const uint8_t *su1, *su2, *su3;
	int res = 0;

	if (!mask)
		return kalMemCmp(cs, ct, count);

	for (su1 = cs, su2 = ct, su3 = mask;
		count > 0; ++su1, ++su2, ++su3, count--) {
		res = ((*su1)&(*su3)) - ((*su2)&(*su3));
		if (res != 0)
			break;
	}

	return res;
}

/*
 * This func is mainly from bionic's strtok.c
 */
char *strtok_r(char *s, const char *delim, char **last)
{
	char *spanp;
	int c, sc;
	char *tok;


	if (s == NULL) {
		s = *last;
		if (s == 0)
			return NULL;
	}
cont:
	c = *s++;
	for (spanp = (char *)delim; (sc = *spanp++) != 0;) {
		if (c == sc)
			goto cont;
	}

	if (c == 0) {		/* no non-delimiter characters */
		*last = NULL;
		return NULL;
	}
	tok = s - 1;

	for (;;) {
		c = *s++;
		spanp = (char *)delim;
		do {
			sc = *spanp++;
			if (sc == c) {
				if (c == 0)
					s = NULL;
				else
					s[-1] = 0;
				*last = s;
				return tok;
			}
		} while (sc != 0);
	}
}

#if CFG_SUPPORT_WPA3
int kalExternalAuthRequest(struct GLUE_INFO *prGlueInfo,
			   struct STA_RECORD *prStaRec)
{
	struct BSS_DESC *prBssDesc = NULL;
	struct net_device *ndev = NULL;
	struct BSS_INFO *prBssInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	uint8_t ucBssIndex = prStaRec->ucBssIndex;
	uint8_t *mld_addr = NULL;
	uint8_t fgConnReqMloSupport = FALSE;

	prAdapter = prGlueInfo->prAdapter;
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (prBssInfo == NULL) {
		DBGLOG(SAA, WARN,
			"SAE auth failed with NULL prBssInfo\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	if (IS_BSS_AIS(prBssInfo)) {
		struct CONNECTION_SETTINGS *prConnSettings = NULL;

		prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);
		if (!prConnSettings) {
			DBGLOG(SAA, WARN,
			       "SAE auth failed with NULL prAisFsmInfo\n");
			return WLAN_STATUS_INVALID_DATA;
		}

		fgConnReqMloSupport = !!(prConnSettings->u4ConnFlags &
					 CONNECT_REQ_MLO_SUPPORT);
		prBssDesc = aisGetTargetBssDesc(prAdapter, ucBssIndex);
		if (!prBssDesc) {
			DBGLOG(SAA, WARN,
			       "SAE auth failed without prTargetBssDesc\n");
			return WLAN_STATUS_INVALID_DATA;
		}
	}
#if CFG_ENABLE_WIFI_DIRECT
	else if (IS_BSS_P2P(prBssInfo)) {
		struct P2P_ROLE_FSM_INFO *prP2pRole = NULL;

		prP2pRole = p2pFuncGetRoleByBssIdx(prAdapter, ucBssIndex);
		if (!prP2pRole) {
			DBGLOG(SAA, WARN,
			       "SAE auth failed with NULL prP2pRoleFsmInfo\n");
			return WLAN_STATUS_INVALID_DATA;
		}

		fgConnReqMloSupport = !!(prP2pRole->rJoinInfo.u4ConnFlags &
					 CONNECT_REQ_MLO_SUPPORT);
		prBssDesc = p2pGetTargetBssDesc(prAdapter, ucBssIndex);
		if (!prBssDesc) {
			DBGLOG(SAA, WARN,
			       "SAE auth failed without prTargetBssDesc\n");
			return WLAN_STATUS_INVALID_DATA;
		}
		prBssDesc->u4RsnSelectedAKMSuite = RSN_AKM_SUITE_SAE;
	}
#endif

	if (prBssDesc == NULL) {
		DBGLOG(SAA, WARN,
			"SAE auth failed with NULL prBssDesc\n");
		return WLAN_STATUS_INVALID_DATA;
	}
	ndev = wlanGetNetDev(prGlueInfo, ucBssIndex);
	if (!ndev)
		return WLAN_STATUS_INVALID_DATA;

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	if (mldIsMultiLinkFormed(prAdapter, prStaRec))
		mld_addr = prStaRec->aucMldAddr;
#endif /* (CFG_SUPPORT_802_11BE_MLO == 1) */

	if (mld_addr == NULL || fgConnReqMloSupport) {
		struct cfg80211_external_auth_params params = {0};
		uint8_t mld_addr_str[50];

		params.action = NL80211_EXTERNAL_AUTH_START;
		COPY_MAC_ADDR(params.bssid, prBssDesc->aucBSSID);
		COPY_SSID(params.ssid.ssid, params.ssid.ssid_len,
			  prBssDesc->aucSSID, prBssDesc->ucSSIDLen);
		if (prBssDesc->u4RsnSelectedAKMSuite == RSN_AKM_SUITE_SAE)
			params.key_mgmt_suite = RSN_AKM_SUITE_SAE;
		else
			WLAN_GET_FIELD_BE32(&prBssDesc->u4RsnSelectedAKMSuite,
				&params.key_mgmt_suite);

		mld_addr_str[0] = '\0';
#if (CFG_ADVANCED_80211_MLO == 1) || \
	KERNEL_VERSION(6, 3, 0) <= CFG80211_VERSION_CODE
		if (mld_addr) {
			COPY_MAC_ADDR(params.mld_addr, mld_addr);
			kalSnprintf(mld_addr_str, sizeof(mld_addr_str),
				" MLD[" MACSTR "]", MAC2STR(mld_addr));
		}
#endif

		DBGLOG(SAA, INFO, "[WPA3] "MACSTR
		       "%s %s %d %d %02x-%02x-%02x-%02x",
		       MAC2STR(params.bssid), mld_addr_str,
		       params.ssid.ssid, params.ssid.ssid_len, params.action,
		       (uint8_t) (params.key_mgmt_suite & 0x000000FF),
		       (uint8_t) ((params.key_mgmt_suite >> 8) & 0x000000FF),
		       (uint8_t) ((params.key_mgmt_suite >> 16) & 0x000000FF),
		       (uint8_t) ((params.key_mgmt_suite >> 24) & 0x000000FF));

		return cfg80211_external_auth_request(
				ndev, &params, GFP_KERNEL);
	}
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	else { /* vendor external auth to support proprierary mlo */
		struct wiphy *wiphy;
		struct wireless_dev *wdev;
		struct PARAM_EXTERNAL_AUTH_INFO *info;
		struct MLD_BSS_INFO *prMldBssInfo = NULL;
		uint16_t size = 0;

		wiphy = prGlueInfo->prDevHandler->ieee80211_ptr->wiphy;
		wdev = wlanGetNetDev(prGlueInfo, ucBssIndex)->ieee80211_ptr;
		prMldBssInfo = mldBssGetByBss(prAdapter, prBssInfo);

		size = sizeof(struct PARAM_EXTERNAL_AUTH_INFO);
		info = kalMemZAlloc(size, VIR_MEM_TYPE);
		if (!info) {
			DBGLOG(SAA, ERROR,
				"alloc vendor external auth event fail\n");
			return -1;
		}

		info->id = GRID_EXTERNAL_AUTH;
		info->len = sizeof(struct PARAM_EXTERNAL_AUTH_INFO) - 2;
		info->action = NL80211_EXTERNAL_AUTH_START;
		COPY_MAC_ADDR(info->bssid, prBssDesc->aucBSSID);
		COPY_SSID(info->ssid, info->ssid_len,
			prBssDesc->aucSSID, prBssDesc->ucSSIDLen);
		info->ssid[info->ssid_len] = '\0';
		if (prBssDesc->u4RsnSelectedAKMSuite == RSN_AKM_SUITE_SAE)
			info->key_mgmt_suite = RSN_AKM_SUITE_SAE;
		else
			WLAN_GET_FIELD_BE32(&prBssDesc->u4RsnSelectedAKMSuite,
				&info->key_mgmt_suite);
		info->dot11MultiLinkActivated = TRUE;
		COPY_MAC_ADDR(info->own_ml_addr, prMldBssInfo ?
			prMldBssInfo->aucOwnMldAddr : prBssInfo->aucOwnMacAddr);
		COPY_MAC_ADDR(info->peer_ml_addr, mld_addr);

		DBGLOG(SAA, INFO,
			"[WPA3] "MACSTR" %s %d %d mlo=%d["
			MACSTR"] %02x-%02x-%02x-%02x",
			MAC2STR(info->bssid), info->ssid, info->ssid_len,
			info->action, info->dot11MultiLinkActivated,
			MAC2STR(info->peer_ml_addr),
			(uint8_t) (info->key_mgmt_suite & 0xFF),
			(uint8_t) ((info->key_mgmt_suite >> 8) & 0xFF),
			(uint8_t) ((info->key_mgmt_suite >> 16) & 0xFF),
			(uint8_t) ((info->key_mgmt_suite >> 24) & 0xFF));

		mtk_cfg80211_vendor_event_generic_response(
			wiphy, wdev, IE_SIZE(info), (uint8_t *)info);

		kalMemFree(info, VIR_MEM_TYPE, size);

		return WLAN_STATUS_SUCCESS;
	}
#endif /* CFG_SUPPORT_802_11BE_MLO */

	return WLAN_STATUS_NOT_ACCEPTED;
}
#endif /* CFG_SUPPORT_WPA3 */

const uint8_t *kalFindIeMatchMask(uint8_t eid,
				const uint8_t *ies, int len,
				const uint8_t *match,
				int match_len, int match_offset,
				const uint8_t *match_mask)
{
	/* match_offset can't be smaller than 2, unless match_len is
	 * zero, in which case match_offset must be zero as well.
	 */
	if (WARN_ON((match_len && match_offset < 2) ||
		(!match_len && match_offset)))
		return NULL;
	while (len >= 2 && len >= IE_SIZE(ies)) {
		if ((ies[0] == eid) &&
			(IE_SIZE(ies) >= match_offset + match_len) &&
			!kalMaskMemCmp(ies + match_offset,
			match, match_mask, match_len))
			return ies;
		len -= IE_SIZE(ies);
		ies += IE_SIZE(ies);
	}
	return NULL;
}

const uint8_t *kalFindIeExtIE(uint8_t eid,
				uint8_t exteid,
				const uint8_t *ies, int len)
{
	if (eid != ELEM_ID_RESERVED)
		return kalFindIeMatchMask(eid, ies, len, NULL, 0, 0, NULL);
	else
		return kalFindIeMatchMask(eid, ies, len, &exteid, 1, 2, NULL);
}

const uint8_t *kalFindVendorIe(uint32_t oui, int type,
				const uint8_t *ies, int len)
{
	const uint8_t *ie;
	uint8_t match[] = {oui >> 16, oui >> 8, oui, type};
	int match_len = type < 0 ? 3 : sizeof(match);

	if (WARN_ON(type > 0xff))
		return NULL;

	ie = kalFindIeMatchMask(ELEM_ID_VENDOR, ies, len, match,
		match_len, 2, NULL);

	if (ie && (ie[1] < 4))
		return NULL;

	return ie;
}

int _kalSnprintf(char *buf, size_t size, const char *fmt, ...)
{
	int retval;
	va_list ap;

	va_start(ap, fmt);
	retval = vsnprintf(buf, size, fmt, ap);
	va_end(ap);
	return (retval < 0)?(0):(retval);
}

int _kalSprintf(char *buf, const char *fmt, ...)
{
	int retval;
	va_list ap;

	va_start(ap, fmt);
	retval = vsprintf(buf, fmt, ap);
	va_end(ap);
	return (retval < 0)?(0):(retval);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Compare two struct timeval
 *
 * @param prTs1          a pointer to timeval
 * @param prTs2          a pointer to timeval
 *
 *
 * @retval 0             two time value is equal
 * @retval 1             prTs1 value > prTs2 value
 * @retval -1            prTs1 value < prTs2 value
 */
/*----------------------------------------------------------------------------*/
int kalTimeCompare(struct timespec64 *prTs1, struct timespec64 *prTs2)
{
	if (prTs1->tv_sec > prTs2->tv_sec)
		return 1;
	else if (prTs1->tv_sec < prTs2->tv_sec)
		return -1;
	/* sec part is equal */
	else if (prTs1->tv_nsec > prTs2->tv_nsec)
		return 1;
	else if (prTs1->tv_nsec < prTs2->tv_nsec)
		return -1;
	return 0;
}

u_int8_t kalGetDeltaTime(struct timespec64 *prTs1, struct timespec64 *prTs2,
			 struct timespec64 *prTsRst)
{
	/* Ignore now time < token time */
	if (kalTimeCompare(prTs1, prTs2) < 0)
		return FALSE;

	prTsRst->tv_sec = prTs1->tv_sec - prTs2->tv_sec;
	prTsRst->tv_nsec = prTs1->tv_nsec;
	if (prTs2->tv_nsec > prTs1->tv_nsec) {
		prTsRst->tv_sec -= 1;
		prTsRst->tv_nsec += SEC_TO_NSEC(1);
	}
	prTsRst->tv_nsec -= prTs2->tv_nsec;
	return TRUE;
}

void setTimeParameter(
		struct PARAM_CUSTOM_CHIP_CONFIG_STRUCT *prChipConfigInfo,
		int chipConfigInfoSize, unsigned int second,
		unsigned int usecond)
{
	int8_t aucKey[WLAN_CFG_VALUE_LEN_MAX];
	int8_t aucValue[WLAN_CFG_VALUE_LEN_MAX];

	kalMemZero(aucValue, sizeof(aucValue));
	kalMemZero(aucKey, sizeof(aucKey));
	kalSnprintf(aucKey, sizeof(aucKey), "SetChip");
	kalSnprintf(aucValue, sizeof(aucValue),
		"SyncTime %u %u", second, usecond);

	kalMemZero(prChipConfigInfo, chipConfigInfoSize);

	prChipConfigInfo->ucType = CHIP_CONFIG_TYPE_WO_RESPONSE;
	prChipConfigInfo->u2MsgSize = kalStrnLen(aucValue,
						   WLAN_CFG_VALUE_LEN_MAX);
	kalStrnCpy(prChipConfigInfo->aucCmd, aucValue,
		   CHIP_CONFIG_RESP_SIZE);
}

static uint32_t kalSyncTimeToFwViaCmd(struct ADAPTER *prAdapter,
	u_int8_t fgInitCmd,
	uint32_t u4Sec,
	uint32_t u4Usec)
{
	struct PARAM_CUSTOM_CHIP_CONFIG_STRUCT rChipConfigInfo = {0};
	struct CMD_CHIP_CONFIG rCmdChipConfig = {0};
	uint32_t u4SetBufferLen = sizeof(rChipConfigInfo);
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	setTimeParameter(&rChipConfigInfo, sizeof(rChipConfigInfo),
			u4Sec, u4Usec);

	kalMemZero(&rCmdChipConfig, sizeof(rCmdChipConfig));

	rCmdChipConfig.u2Id = rChipConfigInfo.u2Id;
	rCmdChipConfig.ucType = rChipConfigInfo.ucType;
	rCmdChipConfig.ucRespType = rChipConfigInfo.ucRespType;
	rCmdChipConfig.u2MsgSize = rChipConfigInfo.u2MsgSize;
	kalMemCopy(rCmdChipConfig.aucCmd, rChipConfigInfo.aucCmd,
		   rCmdChipConfig.u2MsgSize);

	if (!fgInitCmd) {
		rStatus = wlanSendSetQueryCmd(prAdapter,
			CMD_ID_CHIP_CONFIG,
			TRUE,
			FALSE,
			FALSE,
			nicCmdEventSetCommon,
			nicCmdTimeoutCommon,
			sizeof(struct CMD_CHIP_CONFIG),
			(uint8_t *) &rCmdChipConfig,
			&rChipConfigInfo, u4SetBufferLen);
	} else {
		rStatus = wlanSendInitSetQueryCmd(prAdapter,
			INIT_CMD_ID_LOG_TIME_SYNC,
			&rCmdChipConfig,
			sizeof(rCmdChipConfig),
			TRUE, FALSE,
			INIT_EVENT_ID_CMD_RESULT, NULL, 0);
	}

	return rStatus;
}

uint32_t
kalSyncTimeToFW(struct ADAPTER *prAdapter, u_int8_t fgInitCmd)
{
	struct mt66xx_chip_info *prChipInfo;
	struct timespec64 rTime;
	uint32_t u4Sec, u4Usec;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	prChipInfo = prAdapter->chip_info;
	ktime_get_real_ts64(&rTime);

	u4Sec = rTime.tv_sec;
	u4Usec = NSEC_TO_USEC(rTime.tv_nsec);

	if ((prChipInfo->chip_capability &
	    BIT(CHIP_CAPA_FW_LOG_TIME_SYNC)) == 0)
		return WLAN_STATUS_SUCCESS;

	if (TIME_AFTER((prAdapter->u4FWLastUpdateTime + 7200), u4Sec))
		return WLAN_STATUS_SUCCESS;

	DBGLOG(INIT, INFO, "Sync kernel time %u %u", u4Sec, u4Usec);

	if (prChipInfo->chip_capability &
	    BIT(CHIP_CAPA_FW_LOG_TIME_SYNC_BY_CCIF))
		ccif_notify_utc_time_to_fw(prAdapter, u4Sec, u4Usec);
	else
		kalSyncTimeToFwViaCmd(prAdapter, fgInitCmd, u4Sec, u4Usec);

	if (rStatus == WLAN_STATUS_SUCCESS) {
		/* Must sync time in the 1st trigger time in the main thread.
		 * So that don't record the updated time in the fw download
		 * stage, and the 7200 sec time sync period won't start before
		 * the 1st main thread process.
		 */
		if (fgInitCmd != TRUE)
			prAdapter->u4FWLastUpdateTime = u4Sec;
	} else
		DBGLOG(INIT, WARN,
			"Failed to sync kernel time to FW.");

	return rStatus;
}

static uint32_t
__kalSyncTimeToFWByIoctl(struct ADAPTER *prAdapter,
	void *pvSetBuffer,
	uint32_t u4SetBufferLen,
	uint32_t *pu4SetInfoLen)
{
	return kalSyncTimeToFW(prAdapter, FALSE);
}

void
kalSyncTimeToFWByIoctl(void)
{
#ifdef CFG_MTK_CONNSYS_DEDICATED_LOG_PATH
	struct GLUE_INFO *prGlueInfo;

	WIPHY_PRIV(wlanGetWiphy(), prGlueInfo);

	if (prGlueInfo && prGlueInfo->u4ReadyFlag == 1) {
		uint32_t u4BufLen = 0;
		uint32_t rStatus = WLAN_STATUS_SUCCESS;

		rStatus = kalIoctl(prGlueInfo, __kalSyncTimeToFWByIoctl,
				   NULL, 0, &u4BufLen);
		if (rStatus == WLAN_STATUS_FAILURE)
			DBGLOG(INIT, WARN,
				"Failed to sync kernel time to FW.");
	}
#endif
}

void kalUpdateCompHdlrRec(struct ADAPTER *prAdapter,
				PFN_OID_HANDLER_FUNC pfnOidHandler,
				struct CMD_INFO *prCmdInfo)
{
	u_int8_t fgCmdDbgEn = wlanIfCmdDbgEn(prAdapter);

	if (pfnOidHandler)
		kalSnprintf(prAdapter->arPrevCompHdlrRec[
					prAdapter->u4CompRecIdx].aucName,
				sizeof(prAdapter->arPrevCompHdlrRec[
					prAdapter->u4CompRecIdx].aucName),
				fgCmdDbgEn ? "%ps" : "%p",
				pfnOidHandler);
	else {
		if (prCmdInfo)
			kalSnprintf(prAdapter->arPrevCompHdlrRec[
					prAdapter->u4CompRecIdx].aucName,
					sizeof(prAdapter->arPrevCompHdlrRec[
					prAdapter->u4CompRecIdx].aucName),
					fgCmdDbgEn ? "[CID 0x%x] %ps" :
						"[CID 0x%x] %p",
					prCmdInfo->ucCID,
					__builtin_return_address(0));
		else
			kalSnprintf(prAdapter->arPrevCompHdlrRec[
					prAdapter->u4CompRecIdx].aucName,
					sizeof(prAdapter->arPrevCompHdlrRec[
					prAdapter->u4CompRecIdx].aucName),
					 fgCmdDbgEn ? "%ps" : "%p",
					__builtin_return_address(0));
	}

	prAdapter->u4CompRecIdx = (prAdapter->u4CompRecIdx + 1)
					% OID_HDLR_REC_NUM;
}

#if CFG_SUPPORT_SA_LOG
#define SA_LOG_TIMEBUF_LEN (128)

void kalPrintUTC(char *msg_buf, int msg_buf_size)
{
	u64 ts;
	unsigned long rem_nsec;
	int ret = 0;
	struct rtc_time tm;
	struct timespec64 tv = { 0 };
	char s[SA_LOG_TIMEBUF_LEN] = {0};

	ktime_get_real_ts64(&tv);
	rtc_time64_to_tm(tv.tv_sec, &tm);

	ts = local_clock();
	rem_nsec = do_div(ts, 1000000000);

	ret = snprintf(s,
		SA_LOG_TIMEBUF_LEN,
		"[%5lu.%06lu] %d-%02d-%02d %02d:%02d:%02d.%06u %s",
		(unsigned long)ts,
		rem_nsec / 1000,
		tm.tm_year + 1900,
		tm.tm_mon + 1,
		tm.tm_mday,
		tm.tm_hour,
		tm.tm_min,
		tm.tm_sec,
		(unsigned int)KAL_GET_USEC(tv),
		KAL_GET_CURRENT_THREAD_NAME());
	if (ret < 0) {
		LOG_FUNC("snprintf failed, ret: %d",
			ret);
	} else {
		wifi_salog_write(s,
			strlen(s));
		wifi_salog_write(msg_buf,
			msg_buf_size);
	}
}

void kalPrintSALog(const char *fmt, ...)
{
	char buffer[WIFI_LOG_MSG_BUFFER] = {0};
	int ret = 0;
	va_list args;

	va_start(args, fmt);
	ret = vsnprintf(buffer, WIFI_LOG_MSG_BUFFER, fmt, args);
	if (ret < 0) {
		LOG_FUNC("vsnprintf failed, ret: %d",
			ret);
	}
	va_end(args);

	if (strlen(buffer) < WIFI_LOG_MSG_MAX) {
		kalPrintUTC(buffer,
			strlen(buffer));
	} else {
		char sub_buffer[WIFI_LOG_MSG_MAX];

		strncpy(sub_buffer, buffer,
			WIFI_LOG_MSG_MAX - 1);
		sub_buffer[WIFI_LOG_MSG_MAX - 1] = '\0';
		kalPrintUTC(sub_buffer,
			WIFI_LOG_MSG_MAX);

		strncpy(sub_buffer, buffer + WIFI_LOG_MSG_MAX - 1,
			WIFI_LOG_MSG_MAX - 1);
		sub_buffer[WIFI_LOG_MSG_MAX - 1] = '\0';
		kalPrintUTC(sub_buffer,
			WIFI_LOG_MSG_MAX);
	}
}
#endif /* CFG_SUPPORT_SA_LOG */

#if (CFG_SUPPORT_POWER_THROTTLING == 1)
void kalPwrLevelHdlrRegister(struct ADAPTER *prAdapter,
					PFN_PWR_LEVEL_HANDLER hdlr)
{
	struct PWR_LEVEL_HANDLER_ELEMENT *prRegisterHdlr;

	prRegisterHdlr = (struct PWR_LEVEL_HANDLER_ELEMENT *)
			kalMemAlloc(sizeof(struct PWR_LEVEL_HANDLER_ELEMENT),
			VIR_MEM_TYPE);
	if (!prRegisterHdlr) {
		DBGLOG(INIT, WARN, "prRegisterHdlr memory alloc fail!\n");
		return;
	}

	prRegisterHdlr->prPwrLevelHandler = hdlr;

	LINK_INSERT_HEAD(&prAdapter->rPwrLevelHandlerList,
		&prRegisterHdlr->rLinkEntry);
	DBGLOG(INIT, TRACE, "%ps is registered.\n", hdlr);

	prRegisterHdlr->prPwrLevelHandler(prAdapter, prAdapter->u4PwrLevel);
}

void kalPwrLevelHdlrUnregisterAll(struct ADAPTER *prAdapter)
{
	struct PWR_LEVEL_HANDLER_ELEMENT *prRegisterHdlr = NULL;

	while (!LINK_IS_EMPTY(&prAdapter->rPwrLevelHandlerList)) {
		LINK_REMOVE_HEAD(&prAdapter->rPwrLevelHandlerList,
			prRegisterHdlr, struct PWR_LEVEL_HANDLER_ELEMENT *);
		DBGLOG(INIT, TRACE, "%ps is unregistered.\n",
			prRegisterHdlr->prPwrLevelHandler);

		kalMemFree(prRegisterHdlr, VIR_MEM_TYPE,
			sizeof(struct PWR_LEVEL_HANDLER_ELEMENT));
	}
}

void connsysPowerLevelNotify(struct ADAPTER *prAdapter)
{
	struct CMD_HEADER rCmdV1Header;
	struct CMD_FORMAT_V1 rCmd_v1;
	struct LINK *prPwrLevelHandlerList;
	struct PWR_LEVEL_HANDLER_ELEMENT  *prPwrLevelHdlr = NULL;
	uint8_t aucCmdValue[MAX_CMD_VALUE_MAX_LENGTH] = { 0 };
	uint32_t u4PwrLevel;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	int ret = -1;

	prPwrLevelHandlerList = &(prAdapter->rPwrLevelHandlerList);
	u4PwrLevel = prAdapter->u4PwrLevel;

	DBGLOG(INIT, INFO, "Notify each handler new power level: %d\n",
								u4PwrLevel);

	/* Notify registered handler. */
	LINK_FOR_EACH_ENTRY(prPwrLevelHdlr, prPwrLevelHandlerList, rLinkEntry,
		    struct PWR_LEVEL_HANDLER_ELEMENT) {
		ret = prPwrLevelHdlr->prPwrLevelHandler(prAdapter, u4PwrLevel);
		if (ret != 0)
			DBGLOG(INIT, WARN, "Fail!\n");
	}

	/* Notify firmware. */
	rCmdV1Header.cmdType = CMD_TYPE_SET;
	rCmdV1Header.cmdVersion = CMD_VER_1;
	rCmdV1Header.cmdBufferLen = 0;
	rCmdV1Header.itemNum = 0;

	kalMemSet(rCmdV1Header.buffer, 0, MAX_CMD_BUFFER_LENGTH);
	kalMemSet(&rCmd_v1, 0, sizeof(struct CMD_FORMAT_V1));

	rCmd_v1.itemType = ITEM_TYPE_STR;
	rCmd_v1.itemStringLength = kalStrLen("PowerLevelNotify");
	kalMemZero(rCmd_v1.itemString, MAX_CMD_NAME_MAX_LENGTH);
	kalMemCopy(rCmd_v1.itemString, "PowerLevelNotify",
		rCmd_v1.itemStringLength);

	kalSnprintf(aucCmdValue, sizeof(aucCmdValue), "%d", u4PwrLevel);
	rCmd_v1.itemValueLength = sizeof(aucCmdValue);
	kalMemZero(rCmd_v1.itemValue, MAX_CMD_VALUE_MAX_LENGTH);
	kalMemCopy(rCmd_v1.itemValue, aucCmdValue, sizeof(aucCmdValue));

	kalMemCopy(((struct CMD_FORMAT_V1 *)rCmdV1Header.buffer),
			&rCmd_v1,  sizeof(struct CMD_FORMAT_V1));

	rCmdV1Header.cmdBufferLen += sizeof(struct CMD_FORMAT_V1);
	rCmdV1Header.itemNum = 1;

	DBGLOG(INIT, INFO, "Notify FW new power level: %d\n", u4PwrLevel);

	rStatus = wlanSendSetQueryCmd(
			prAdapter, /* prAdapter */
			CMD_ID_GET_SET_CUSTOMER_CFG, /* 0x70 */
			TRUE,  /* fgSetQuery */
			FALSE, /* fgNeedResp */
			FALSE, /* fgIsOid */
			NULL,  /* pfCmdDoneHandler*/
			NULL,  /* pfCmdTimeoutHandler */
			sizeof(struct CMD_HEADER),
			(uint8_t *)&rCmdV1Header, /* pucInfoBuffer */
			NULL,  /* pvSetQueryBuffer */
			0      /* u4SetQueryBufferLen */
		);
}

void connsysPowerTempNotify(struct ADAPTER *prAdapter)
{
	uint32_t u4MaxTemp, u4RecoveryTemp;

	u4MaxTemp = (prAdapter->rTempInfo).max_temp;
	u4RecoveryTemp = (prAdapter->rTempInfo).recovery_temp;

	DBGLOG(INIT, INFO, "Ignore notify FW new temp info: %d, %d\n",
						u4MaxTemp, u4RecoveryTemp);
}

void connsysPowerTempUpdate(enum conn_pwr_msg_type status,
					int currentTemp)
{
	DBGLOG(INIT, INFO, "Update power message type %d, current temp: %d\n",
				status, currentTemp);
	conn_pwr_send_msg(CONN_PWR_DRV_WIFI, status, &currentTemp);
}

uint32_t kalDumpPwrLevel(struct ADAPTER *prAdapter)
{
	OS_SYSTIME now, last;

	GET_BOOT_SYSTIME(&now);
	last = prAdapter->rPwrLevelStatUpdateTime;

	if (!CHECK_FOR_TIMEOUT(now, last,
		SEC_TO_SYSTIME(PWR_LEVEL_STAT_UPDATE_INTERVAL))) {
		return WLAN_STATUS_PENDING;
	}

	prAdapter->rPwrLevelStatUpdateTime = now;

	DBGLOG(SW4, INFO, "Current power level: %d\n",	prAdapter->u4PwrLevel);

	return WLAN_STATUS_SUCCESS;
}

#endif

#if CFG_SUPPORT_NAN
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
static const char *nan_unisubevent_str(uint32_t u4SubEvent)
{
	static const char * const subevent_string[UNI_EVENT_NAN_TAG_NUM] = {
		[UNI_EVENT_NAN_TAG_DISCOVERY_RESULT] = "Discovery Result",
		[UNI_EVENT_NAN_TAG_FOLLOW_EVENT] = "Follow",
		[UNI_EVENT_NAN_TAG_MASTER_IND_ATTR] = "Master Ind",
		[UNI_EVENT_NAN_TAG_CLUSTER_ID_UPDATE] = "Cluster ID Update",
		[UNI_EVENT_NAN_TAG_REPLIED_EVENT] = "Replied",
		[UNI_EVENT_NAN_TAG_PUBLISH_TERMINATE_EVENT] =
			"Publish Terminate",
		[UNI_EVENT_NAN_TAG_SUBSCRIBE_TERMINATE_EVENT] =
			"Subscribe Terminate",
		[UNI_EVENT_NAN_TAG_ID_SCHEDULE_CONFIG] = "Schedule Config",
		[UNI_EVENT_NAN_TAG_ID_PEER_AVAILABILITY] = "Peer Availability",
		[UNI_EVENT_NAN_TAG_ID_PEER_CAPABILITY] = "Peer Capability",
		[UNI_EVENT_NAN_TAG_ID_CRB_HANDSHAKE_TOKEN] =
			"CRB Handshake Token",
		[UNI_EVENT_NAN_TAG_ID_DATA_NOTIFY] = "Data Notify",
		[UNI_EVENT_NAN_TAG_FTM_DONE] = "FTM Done",
		[UNI_EVENT_NAN_TAG_RANGING_BY_DISC] = "Ranging by Disc",
		[UNI_EVENT_NAN_TAG_NDL_FLOW_CTRL] = "NDL Flow Ctrl",
		[UNI_EVENT_NAN_TAG_DW_INTERVAL] = "DW Interval",
		[UNI_EVENT_NAN_TAG_NDL_DISCONNECT] = "NDL Disconnect",
		[UNI_EVENT_NAN_TAG_ID_PEER_CIPHER_SUITE_INFO] =
			"Peer Cipher Suite Info (CSIA)",
		[UNI_EVENT_NAN_TAG_ID_PEER_SEC_CONTEXT_INFO] =
			"Peer Security Context Info (SCIA)",
		[UNI_EVENT_NAN_TAG_ID_DE_EVENT_IND] = "DE Event",
		[UNI_EVENT_NAN_TAG_SELF_FOLLOW_EVENT] = "Self Follow",
		[UNI_EVENT_NAN_TAG_DISABLE_IND] = "Disable",
		[UNI_EVENT_NAN_TAG_NDL_FLOW_CTRL_V2] = "NDL Flow Ctrl v2",
		[UNI_EVENT_NAN_TAG_ID_DEVICE_CAPABILITY] = "Device Capability",
		[UNI_EVENT_NAN_ID_MATCH_EXPIRE] = "Match Expire",
		[UNI_EVENT_NAN_TAG_REPORT_BEACON] = "Report Beacon",
	};

	if (u4SubEvent < UNI_EVENT_NAN_TAG_NUM)
		return subevent_string[u4SubEvent];
	else
		return "";
}

void kalNanHandleVendorEvent(struct ADAPTER *prAdapter, uint8_t *prBuffer)
{
	struct UNI_CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;
	uint32_t u4SubEvent;
	int status = 0;

	ASSERT(prAdapter);

	prTlvElement =
	(struct UNI_CMD_EVENT_TLV_ELEMENT_T *)prBuffer;

	u4SubEvent = prTlvElement->u2Tag;

	DBGLOG(NAN, INFO, "subEvent:%d (%s)\n", u4SubEvent,
			nan_unisubevent_str(u4SubEvent));

	if (prAdapter->fgIsNANRegistered == FALSE) {
		DBGLOG(NAN, ERROR,
			"kalNanHandleVendorEvent, NAN is unregistered\n");
		return;
	}

	switch (u4SubEvent) {
	case UNI_EVENT_NAN_TAG_ID_DE_EVENT_IND:
		status = mtk_cfg80211_vendor_event_nan_event_indication(
			prAdapter, prTlvElement->aucbody);
		break;
	case UNI_EVENT_NAN_TAG_DISCOVERY_RESULT:
		status = mtk_cfg80211_vendor_event_nan_match_indication(
			prAdapter, prTlvElement->aucbody);
		break;
	case UNI_EVENT_NAN_TAG_FOLLOW_EVENT:
		status = mtk_cfg80211_vendor_event_nan_followup_indication(
			prAdapter, prTlvElement->aucbody);
		break;
	case UNI_EVENT_NAN_TAG_REPLIED_EVENT:
		status = mtk_cfg80211_vendor_event_nan_replied_indication(
			prAdapter, prTlvElement->aucbody);
		break;
	case UNI_EVENT_NAN_TAG_PUBLISH_TERMINATE_EVENT:
		status = mtk_cfg80211_vendor_event_nan_publish_terminate(
			prAdapter, prTlvElement->aucbody);
		break;
	case UNI_EVENT_NAN_TAG_SUBSCRIBE_TERMINATE_EVENT:
		status = mtk_cfg80211_vendor_event_nan_subscribe_terminate(
			prAdapter, prTlvElement->aucbody);
		break;
	case UNI_EVENT_NAN_TAG_SELF_FOLLOW_EVENT:
		status = mtk_cfg80211_vendor_event_nan_seldflwup_indication(
			prAdapter, prTlvElement->aucbody);
		break;
	case UNI_EVENT_NAN_TAG_MASTER_IND_ATTR:
		nanDevMasterIndEvtHandler(prAdapter, prTlvElement->aucbody);
		break;
	case UNI_EVENT_NAN_TAG_CLUSTER_ID_UPDATE:
		nanDevClusterIdEvtHandler(prAdapter, prTlvElement->aucbody);
		break;
	case UNI_EVENT_NAN_TAG_ID_SCHEDULE_CONFIG:
		status = mtk_cfg80211_vendor_event_nan_schedule_config(
			prAdapter, prTlvElement->aucbody);
		kal_fallthrough;
	case UNI_EVENT_NAN_TAG_ID_PEER_AVAILABILITY:
	case UNI_EVENT_NAN_TAG_ID_PEER_CAPABILITY:
	case UNI_EVENT_NAN_TAG_ID_CRB_HANDSHAKE_TOKEN:
	case UNI_EVENT_NAN_TAG_ID_DEVICE_CAPABILITY:
		nanSchedulerUniEventDispatch(prAdapter, u4SubEvent,
					  prTlvElement->aucbody);
		break;
	case UNI_EVENT_NAN_TAG_ID_PEER_SEC_CONTEXT_INFO:
		nanDiscUpdateSecContextInfoAttr(prAdapter,
						prTlvElement->aucbody);
		break;
	case UNI_EVENT_NAN_TAG_ID_PEER_CIPHER_SUITE_INFO:
		nanDiscUpdateCipherSuiteInfoAttr(prAdapter,
						 prTlvElement->aucbody);
		break;
	case UNI_EVENT_NAN_TAG_ID_DATA_NOTIFY:
		nicNanEventSTATxCTL(prAdapter, prTlvElement->aucbody);
		break;
	case UNI_EVENT_NAN_TAG_FTM_DONE:
		nanRangingFtmDoneEvt(prAdapter, prTlvElement->aucbody);
		break;
	case UNI_EVENT_NAN_TAG_RANGING_BY_DISC:
		nanRangingInvokedByDiscEvt(prAdapter, prTlvElement->aucbody);
		break;
#if CFG_SUPPORT_NAN_ADVANCE_DATA_CONTROL
	case UNI_EVENT_NAN_TAG_NDL_FLOW_CTRL:
		nicNanNdlFlowCtrlEvt(prAdapter, prTlvElement->aucbody);
		break;
	case UNI_EVENT_NAN_TAG_NDL_FLOW_CTRL_V2:
		nicNanNdlFlowCtrlEvtV2(prAdapter, prTlvElement->aucbody);
		break;
#endif
	case UNI_EVENT_NAN_TAG_NDL_DISCONNECT:
		nanDataEngingDisconnectEvt(prAdapter, prTlvElement->aucbody);
		break;
	case UNI_EVENT_NAN_TAG_DISABLE_IND:
		mtk_cfg80211_vendor_event_nan_disable_indication(
			prAdapter, prTlvElement->aucbody);
		break;
	case UNI_EVENT_NAN_ID_MATCH_EXPIRE:
		status = mtk_cfg80211_vendor_event_nan_match_expire(
			prAdapter, prTlvElement->aucbody);
		break;
	case UNI_EVENT_NAN_TAG_REPORT_BEACON:
		mtk_cfg80211_vendor_event_nan_report_beacon(
			prAdapter, prTlvElement->aucbody);
		break;
	default:
		DBGLOG(NAN, LOUD, "No match event!!\n");
		break;
	}
}

#else
static const char *nan_subevent_str(uint32_t u4SubEvent)
{
	static const char * const subevent_string[NAN_EVENT_NUM] = {
	[NAN_EVENT_TEST] = "Test", /* 0 */
	[NAN_EVENT_DISCOVERY_RESULT] = "Discovery Result",
	[NAN_EVENT_FOLLOW_EVENT] = "Follow",
	[NAN_EVENT_MASTER_IND_ATTR] = "Master Ind",
	[NAN_EVENT_CLUSTER_ID_UPDATE] = "Cluster ID Update",
	[NAN_EVENT_REPLIED_EVENT] = "Replied",
	[NAN_EVENT_PUBLISH_TERMINATE_EVENT] = "Publish Terminate",
	[NAN_EVENT_SUBSCRIBE_TERMINATE_EVENT] = "Subscribe Terminate",
	[NAN_EVENT_ID_SCHEDULE_CONFIG] = "Schedule Config",
	[NAN_EVENT_ID_PEER_AVAILABILITY] = "Peer Availability",
	[NAN_EVENT_ID_PEER_CAPABILITY] = "Peer Capability",
	[NAN_EVENT_ID_CRB_HANDSHAKE_TOKEN] = "CRB Handshake Token",
	[NAN_EVENT_ID_DATA_NOTIFY] = "Data Notify",
	[NAN_EVENT_FTM_DONE] = "FTM Done",
	[NAN_EVENT_RANGING_BY_DISC] = "Ranging by Disc",
	[NAN_EVENT_NDL_FLOW_CTRL] = "NDL Flow Ctrl",
	[NAN_EVENT_DW_INTERVAL] = "DW Interval",
	[NAN_EVENT_NDL_DISCONNECT] = "NDL Disconnect",
	[NAN_EVENT_ID_PEER_CIPHER_SUITE_INFO] = "Peer Cipher Suite Info (CSIA)",
	[NAN_EVENT_ID_PEER_SEC_CONTEXT_INFO] =
		"Peer Security Context Info (SCIA)",
	[NAN_EVENT_ID_DE_EVENT_IND] = "DE Event",
	[NAN_EVENT_SELF_FOLLOW_EVENT] = "Self Follow",
	[NAN_EVENT_DISABLE_IND] = "Disable",
	[NAN_EVENT_NDL_FLOW_CTRL_V2] = "NDL Flow Ctrl v2",
	[NAN_EVENT_ID_DEVICE_CAPABILITY] = "Device Capability",
	[NAN_EVENT_DISC_BCN_PERIOD] = "Discovery Beacon",
	[NAN_EVENT_SERVICE_DISC_CAPABILITY] =  "Service Discovery Capability",
	[NAN_EVENT_DEVICE_INFO] = "Device Info",
	[NAN_EVENT_REPORT_BEACON] = "Report Beacon",
	[NAN_EVENT_MATCH_EXPIRE] = "Match Expire",

	[NAN_EVENT_VENDOR_DISCOVERY_RESULT] = "Vendor Discovery Result",
	[NAN_EVENT_VENDOR_PUBLISH_REPLIED_EVENT] = "Vendor Publish Replied",
	[NAN_EVENT_VENDOR_FOLLOW_UP_RX_EVENT] = "Vendor Follow up RX",
	[NAN_EVENT_VENDOR_FOLLOW_UP_TX_EVENT] =  "Vendor Follow up TX",
	};

	if (u4SubEvent <= NAN_EVENT_MATCH_EXPIRE ||
	    u4SubEvent >= NAN_EVENT_VENDOR_DISCOVERY_RESULT &&
	    u4SubEvent < NAN_EVENT_NUM)
		return subevent_string[u4SubEvent];
	else
		return "";
}

void kalNanHandleVendorEvent(struct ADAPTER *prAdapter, uint8_t *prBuffer)
{
	struct _CMD_EVENT_TLV_COMMOM_T *prTlvCommon = NULL;
	struct _CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;
	uint32_t u4SubEvent;
	int status = 0;

	ASSERT(prAdapter);

	prTlvCommon = (struct _CMD_EVENT_TLV_COMMOM_T *)prBuffer;
	prTlvElement =
		(struct _CMD_EVENT_TLV_ELEMENT_T *)prTlvCommon->aucBuffer;

	u4SubEvent = prTlvElement->tag_type;

	DBGLOG(NAN, INFO, "subEvent:%d (%s)\n", u4SubEvent,
				nan_subevent_str(u4SubEvent));

	if (prAdapter->fgIsNANRegistered == FALSE) {
		DBGLOG(NAN, ERROR,
			"kalNanHandleVendorEvent, NAN is unregistered\n");
		return;
	}

	switch (u4SubEvent) {
	case NAN_EVENT_ID_DE_EVENT_IND:
		status = mtk_cfg80211_vendor_event_nan_event_indication(
			prAdapter, prTlvElement->aucbody);
		break;
	case NAN_EVENT_DISCOVERY_RESULT:
		status = mtk_cfg80211_vendor_event_nan_match_indication(
			prAdapter, prTlvElement->aucbody);
		break;
	case NAN_EVENT_FOLLOW_EVENT:
		status = mtk_cfg80211_vendor_event_nan_followup_indication(
			prAdapter, prTlvElement->aucbody);
		break;
	case NAN_EVENT_REPLIED_EVENT:
		status = mtk_cfg80211_vendor_event_nan_replied_indication(
			prAdapter, prTlvElement->aucbody);
		break;
	case NAN_EVENT_PUBLISH_TERMINATE_EVENT:
		status = mtk_cfg80211_vendor_event_nan_publish_terminate(
			prAdapter, prTlvElement->aucbody);
		break;
	case NAN_EVENT_SUBSCRIBE_TERMINATE_EVENT:
		status = mtk_cfg80211_vendor_event_nan_subscribe_terminate(
			prAdapter, prTlvElement->aucbody);
		break;
	case NAN_EVENT_SELF_FOLLOW_EVENT:
		status = mtk_cfg80211_vendor_event_nan_seldflwup_indication(
			prAdapter, prTlvElement->aucbody);
		break;
	case NAN_EVENT_MASTER_IND_ATTR:
		nanDevMasterIndEvtHandler(prAdapter, prTlvElement->aucbody);
		break;
	case NAN_EVENT_CLUSTER_ID_UPDATE:
		nanDevClusterIdEvtHandler(prAdapter, prTlvElement->aucbody);
		break;
	case NAN_EVENT_ID_SCHEDULE_CONFIG:
		status = mtk_cfg80211_vendor_event_nan_schedule_config(
			prAdapter, prTlvElement->aucbody);
		kal_fallthrough;
	case NAN_EVENT_ID_PEER_AVAILABILITY:
	case NAN_EVENT_ID_PEER_CAPABILITY:
	case NAN_EVENT_ID_CRB_HANDSHAKE_TOKEN:
	case NAN_EVENT_ID_DEVICE_CAPABILITY:
		nanSchedulerEventDispatch(prAdapter, u4SubEvent,
					  prTlvElement->aucbody);
		break;
	case NAN_EVENT_ID_PEER_SEC_CONTEXT_INFO:
		nanDiscUpdateSecContextInfoAttr(prAdapter,
						prTlvElement->aucbody);
		break;
	case NAN_EVENT_ID_PEER_CIPHER_SUITE_INFO:
		nanDiscUpdateCipherSuiteInfoAttr(prAdapter,
						 prTlvElement->aucbody);
		break;
	case NAN_EVENT_ID_DATA_NOTIFY:
		nicNanEventSTATxCTL(prAdapter, prTlvElement->aucbody);
		break;
	case NAN_EVENT_FTM_DONE:
		nanRangingFtmDoneEvt(prAdapter, prTlvElement->aucbody);
		break;
	case NAN_EVENT_RANGING_BY_DISC:
		nanRangingInvokedByDiscEvt(prAdapter, prTlvElement->aucbody);
		break;
#if CFG_SUPPORT_NAN_ADVANCE_DATA_CONTROL
	case NAN_EVENT_NDL_FLOW_CTRL:
		nicNanNdlFlowCtrlEvt(prAdapter, prTlvElement->aucbody);
		break;
	case NAN_EVENT_NDL_FLOW_CTRL_V2:
		nicNanNdlFlowCtrlEvtV2(prAdapter, prTlvElement->aucbody);
		break;
#endif
	case NAN_EVENT_NDL_DISCONNECT:
		nanDataEngingDisconnectEvt(prAdapter, prTlvElement->aucbody);
		break;
	case NAN_EVENT_DISABLE_IND:
		mtk_cfg80211_vendor_event_nan_disable_indication(
			prAdapter, prTlvElement->aucbody);
		break;
	case NAN_EVENT_MATCH_EXPIRE:
		mtk_cfg80211_vendor_event_nan_match_expire(
			prAdapter, prTlvElement->aucbody);
		break;
	case NAN_EVENT_REPORT_BEACON:
		mtk_cfg80211_vendor_event_nan_report_beacon(
			prAdapter, prTlvElement->aucbody);
		break;
	default:
		DBGLOG(NAN, LOUD, "No match event!!\n");
		break;
	}
}
#endif
#endif

#if (CFG_SUPPORT_SINGLE_SKU_LOCAL_DB == 1)
void kalApplyCustomRegulatory(const void *pRegdom,
	uint8_t fgNeedHoldRtnlLock)
{
	struct wiphy *pWiphy;
	u32 band_idx, ch_idx;
	struct ieee80211_supported_band *sband;
	struct ieee80211_channel *chan;
	pWiphy = wlanGetWiphy();

	DBGLOG(RLM, INFO, "%s()\n", __func__);

	if (!pWiphy) {
		DBGLOG(RLM, ERROR, "pWiphy = NULL\n");
		return;
	}

	/* to reset chan->flags */
	for (band_idx = 0; band_idx < KAL_NUM_BANDS; band_idx++) {
		sband = pWiphy->bands[band_idx];
		if (!sband)
			continue;

		for (ch_idx = 0; ch_idx < sband->n_channels; ch_idx++) {
			chan = &sband->channels[ch_idx];

			/*reset chan->flags*/
			chan->flags = 0;
		}

	}

	kalUpdateCustomRegulatoryByWiphy(pWiphy,
		pRegdom, fgNeedHoldRtnlLock);
}

void kalUpdateCustomRegulatoryByWiphy(struct wiphy *pWiphy,
	const void *pRegdom,
	uint8_t fgNeedHoldRtnlLock)
{
#if (KERNEL_VERSION(5, 5, 0) > CFG80211_VERSION_CODE) || \
	(CFG_SUPPORT_SINGLE_SKU_FORCE_CUSTOM_REG == 1)
	/* update to kernel */
	wiphy_apply_custom_regulatory(pWiphy,
		(const struct ieee80211_regdomain *)pRegdom);
#elif KERNEL_VERSION(5, 12, 0) > CFG80211_VERSION_CODE
	/* update regulatory domain to kernel */
	DBGLOG(RLM, INFO,
		"regulatory_flags=0x%x, lock=[%d], fgHoldLock=[%d]\n",
		pWiphy->regulatory_flags, rtnl_is_locked(),
		fgNeedHoldRtnlLock);

	if (fgNeedHoldRtnlLock)
		rtnl_lock();

	if (!rtnl_is_locked())
		DBGLOG(RLM, ERROR, "rtnl lock is not held\n");

	regulatory_set_wiphy_regd_sync_rtnl(pWiphy,
		(struct ieee80211_regdomain *)pRegdom);

	if (fgNeedHoldRtnlLock)
		rtnl_unlock();
#else /* KERNEL_VERSION(5, 12, 0) <= CFG80211_VERSION_CODE */
	/* update regulatory domain to kernel */
	DBGLOG(RLM, INFO,
		"regulatory_flags=0x%x, lock=[%d %d], fgHoldLock=[%d]\n",
		pWiphy->regulatory_flags, rtnl_is_locked(),
		mutex_is_locked(&pWiphy->mtx), fgNeedHoldRtnlLock);

	if (fgNeedHoldRtnlLock)
		rtnl_lock();
	wiphy_lock(pWiphy);

	if (!rtnl_is_locked() || !mutex_is_locked(&pWiphy->mtx))
		DBGLOG(RLM, ERROR, "rtnl or wiphy lock is not held\n");

	regulatory_set_wiphy_regd_sync(pWiphy,
		(struct ieee80211_regdomain *)pRegdom);

	wiphy_unlock(pWiphy);
	if (fgNeedHoldRtnlLock)
		rtnl_unlock();
#endif /* CFG80211_VERSION_CODE */
}
#endif

#if CFG_MODIFY_TX_POWER_BY_BAT_VOLT
void kalEnableTxPwrBackoffByBattVolt(struct ADAPTER *prAdapter, bool ucEnable)
{
	struct CMD_TX_POWER_PERCENTAGE_CTRL_T  rTxPwrPercentage = {0};

	if (!prAdapter)
		return;

	rTxPwrPercentage.ucPowerCtrlFormatId = PERCENTAGE_CTRL;
	rTxPwrPercentage.fgPercentageEnable = ucEnable;
	rTxPwrPercentage.ucBandIdx = 0;	/* TODO: how to get bandIdx */

	DBGLOG(NIC, INFO, "kalEnableTxPwrBackoffByBattVolt, ucEnable = %d",
				rTxPwrPercentage.fgPercentageEnable);

	wlanSendSetQueryExtCmd(prAdapter,
				CMD_ID_LAYER_0_EXT_MAGIC_NUM,
			    EXT_CMD_ID_TX_POWER_FEATURE_CTRL,
			    TRUE,
			    FALSE, FALSE, NULL, NULL,
			    sizeof(struct CMD_TX_POWER_PERCENTAGE_CTRL_T),
			    (uint8_t *)&rTxPwrPercentage, NULL, 0);
}

void kalSetTxPwrBackoffByBattVolt(struct ADAPTER *prAdapter, bool ucEnable)
{
	struct CMD_TX_POWER_PERCENTAGE_DROP_CTRL_T  rTxPwrDrop = {0};


	if (!prAdapter)
		return;

	rTxPwrDrop.ucPowerCtrlFormatId = PERCENTAGE_DROP_CTRL;
	if (ucEnable)
		rTxPwrDrop.i1PowerDropLevel =
			prAdapter->rWifiVar.u4BackoffLevel;
	else
		rTxPwrDrop.i1PowerDropLevel = 0;
	rTxPwrDrop.ucBandIdx = 0;   /* TODO: how to get bandIdx */

	DBGLOG(NIC, INFO, "kalSetTxPwrBackoffByBattVolt, i1PowerDropLevel = %d",
			rTxPwrDrop.i1PowerDropLevel);

	wlanSendSetQueryExtCmd(prAdapter,
				CMD_ID_LAYER_0_EXT_MAGIC_NUM,
			    EXT_CMD_ID_TX_POWER_FEATURE_CTRL,
			    TRUE,
			    FALSE, FALSE, NULL, NULL,
			    sizeof(struct CMD_TX_POWER_PERCENTAGE_DROP_CTRL_T),
			    (uint8_t *)&rTxPwrDrop, NULL, 0);

	if (prAdapter->rWifiVar.eDbdcMode == ENUM_DBDC_MODE_DYNAMIC) {
		DBGLOG(NIC, INFO,
			  "kalSetTxPwrBackoffByBattVolt, ENUM_DBDC_MODE_DYNAMIC");
		rTxPwrDrop.ucBandIdx = 1;

		wlanSendSetQueryExtCmd(prAdapter,
				CMD_ID_LAYER_0_EXT_MAGIC_NUM,
			    EXT_CMD_ID_TX_POWER_FEATURE_CTRL,
			    TRUE,
			    FALSE, FALSE, NULL, NULL,
			    sizeof(struct CMD_TX_POWER_PERCENTAGE_DROP_CTRL_T),
			    (uint8_t *)&rTxPwrDrop, NULL, 0);
	}
}

static void kal_bat_volt_notifier_callback(unsigned int volt)
{
	struct GLUE_INFO *prGlueInfo =
			(struct GLUE_INFO *)wlan_bat_volt_notifier_priv_data;
	struct ADAPTER *prAdapter = NULL;
	struct REG_INFO *prRegInfo = NULL;

	wlan_bat_volt = volt;
	if (prGlueInfo == NULL) {
		DBGLOG(NIC, ERROR, "volt = %d, prGlueInfo is NULL", volt);
		return;
	}
	prAdapter = prGlueInfo->prAdapter;
	prRegInfo = &prGlueInfo->rRegInfo;

	if (prRegInfo == NULL ||
		prGlueInfo->prAdapter == NULL ||
		prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(NIC, ERROR,
			"volt = %d, wlan not start", volt);
		return;
	}
	if (test_bit(GLUE_FLAG_HALT_BIT, &prGlueInfo->ulFlag)) {
		DBGLOG(NIC, ERROR, "volt = %d, Wi-Fi is stopped", volt);
		fgIsTxPowerDecreased = FALSE;
		return;
	}

	kalEnableTxPwrBackoffByBattVolt(prAdapter, TRUE);

	if (volt == RESTORE_VOLT && fgIsTxPowerDecreased == TRUE) {
		kalSetTxPwrBackoffByBattVolt(prAdapter, FALSE);
		fgIsTxPowerDecreased = FALSE;
	} else if (volt == BACKOFF_VOLT && fgIsTxPowerDecreased == FALSE) {
		kalSetTxPwrBackoffByBattVolt(prAdapter, TRUE);
		fgIsTxPowerDecreased = TRUE;
	}
}

int32_t kalBatNotifierReg(struct GLUE_INFO *prGlueInfo)
{
	int32_t i4Ret = 0;
#if (KERNEL_VERSION(4, 19, 0) <= LINUX_VERSION_CODE)
	static struct lbat_user *lbat_pt;
#else
	static struct lbat_user rWifiBatVolt;
#endif
	wlan_bat_volt_notifier_priv_data = prGlueInfo;
	wlan_bat_volt = 0;
#if (KERNEL_VERSION(4, 19, 0) <= LINUX_VERSION_CODE)
	lbat_pt = lbat_user_register("WiFi Get Battery Voltage", RESTORE_VOLT,
				BACKOFF_VOLT, 2000,
				kal_bat_volt_notifier_callback);
	if (IS_ERR(lbat_pt))
		i4Ret = PTR_ERR(lbat_pt);
#else
	i4Ret = lbat_user_register(&rWifiBatVolt, "WiFi Get Battery Voltage",
				RESTORE_VOLT, BACKOFF_VOLT, 2000,
			kal_bat_volt_notifier_callback);
#endif

	if (i4Ret)
		DBGLOG(SW4, ERROR, "Register rWifiBatVolt failed:%d\n", i4Ret);
	else
		DBGLOG(SW4, INFO, "Register rWifiBatVolt succeed\n");

	return i4Ret;
}

void kalBatNotifierUnReg(void)
{
	wlan_bat_volt_notifier_priv_data = NULL;
}
#endif

#if (CFG_COALESCING_INTERRUPT == 1)
int32_t kalCoalescingInt(struct ADAPTER *prAdapter,
			uint32_t u4TarPerfLevel,
			uint32_t u4CoalescingIntTh)
{
	struct BUS_INFO *prBusInfo;

	prBusInfo = prAdapter->chip_info->bus_info;
	if (prBusInfo->setWfdmaCoalescingInt &&
		prAdapter->rWifiVar.fgCoalescingIntEn) {
		if (u4TarPerfLevel >= u4CoalescingIntTh)
			nicSetCoalescingInt(prAdapter,
			TRUE,
			TRUE);
		else
			nicSetCoalescingInt(prAdapter,
			FALSE,
			FALSE);
	}

	return 0;
}
#endif

void kal_sched_set(struct task_struct *p, int policy,
		const struct sched_param *param,
		int nice)
{
#if KERNEL_VERSION(5, 9, 0) <= LINUX_VERSION_CODE
	/* apply auto-detection based on function description
	* TODO:
	* kernel prefer modify "current" only, add sanity here?
	*/
	if (policy == SCHED_NORMAL)
		sched_set_normal(p, nice);
	else if (policy == SCHED_FIFO)
		sched_set_fifo(p);
	else
		sched_set_fifo_low(p);
#else
	sched_setscheduler(p, policy, param);
#endif
}

void kalSetThreadSchPolicyPriority(struct GLUE_INFO *prGlueInfo)
{
	struct sched_param param = { .sched_priority = 1};

	if (!prGlueInfo) {
		DBGLOG(INIT, ERROR, "prGlueInfo is NULL, Just return!\n");
		return;
	}
	/* Set thread's Schedule policy & priority */
	if (prGlueInfo->prAdapter->rWifiVar.ucThreadPriority > 0) {
		param.sched_priority =
			prGlueInfo->prAdapter->rWifiVar.ucThreadPriority;

		kal_sched_set(current,
			prGlueInfo->prAdapter->rWifiVar.ucThreadScheduling,
			&param,
			prGlueInfo->prAdapter->rWifiVar.cThreadNice);

		DBGLOG(INIT, STATE,
			"[%s]Set pri = %d, sched = %d\n",
			KAL_GET_CURRENT_THREAD_NAME(),
			prGlueInfo->prAdapter->rWifiVar.ucThreadPriority,
			prGlueInfo->prAdapter->rWifiVar
			.ucThreadScheduling);
	}
	set_user_nice(current, prGlueInfo->prAdapter->rWifiVar.cThreadNice);
}

#if KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
MODULE_IMPORT_NS(VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver);
#endif

/* For Linux kernel version wrapper */
void kal_napi_complete_done(struct napi_struct *n, int work_done)
{
	if (!n)
		return;
#if KERNEL_VERSION(3, 19, 0) <= LINUX_VERSION_CODE
	napi_complete_done(n, work_done);
#else
	napi_complete(n);
#endif /* KERNEL_VERSION(3, 19, 0) */
}

void kal_napi_schedule(struct napi_struct *n)
{
	if (!n)
		return;
#if KERNEL_VERSION(4, 0, 0) <= CFG80211_VERSION_CODE
	if (in_interrupt())
		napi_schedule_irqoff(n);
	else
#endif /* KERNEL_VERSION(4, 0, 0) */
		napi_schedule(n);
}

#if CFG_SUPPORT_RX_GRO
uint8_t kalRxGroInit(struct net_device *prDev)
{
	/* Register GRO function to kernel */
	prDev->features |= NETIF_F_GRO;
	prDev->hw_features |= NETIF_F_GRO;
/*
 * Know Issue: may introduce KE when NAT + GRO + Raw socket
 * Disable it temporary
 */
#if 0
#if KERNEL_VERSION(5, 10, 0) <= CFG80211_VERSION_CODE
	prDev->features |= NETIF_F_GRO_FRAGLIST_BIT;
#endif
#endif
	DBGLOG(INIT, TRACE, "GRO Init Done\n");
	return 0;
}

#if CFG_SUPPORT_RX_NAPI_THREADED
void kalNapiThreadedInit(struct GLUE_INFO *prGlueInfo)
{
#if KERNEL_VERSION(5, 15, 0) <= CFG80211_VERSION_CODE
	if (dev_set_threaded(&prGlueInfo->dummy_dev, TRUE) != 0) {
		prGlueInfo->napi_thread = NULL;
		DBGLOG(INIT, ERROR, "Napi Threaded Init Fail\n");
	} else {
		prGlueInfo->napi_thread = prGlueInfo->napi.thread;
		prGlueInfo->u4RxNapiThreadPid =
			task_pid_nr(prGlueInfo->napi_thread);
		DBGLOG(INIT, TRACE, "Napi Threaded Init Done\n");
	}
#endif
}

void kalNapiThreadedUninit(struct GLUE_INFO *prGlueInfo)
{
	prGlueInfo->napi_thread = NULL;
	DBGLOG(INIT, TRACE, "Napi Threaded Uninit Done\n");
}
#endif /* CFG_SUPPORT_RX_NAPI_THREADED */

uint8_t kalNapiInit(struct GLUE_INFO *prGlueInfo)
{
	spin_lock_init(&prGlueInfo->napi_spinlock);
	skb_queue_head_init(&prGlueInfo->rRxNapiSkbQ);
	/* use dummy device to register napi */
	init_dummy_netdev(&prGlueInfo->dummy_dev);
#if (KERNEL_VERSION(6, 1, 0) <= CFG80211_VERSION_CODE)
	netif_napi_add(&prGlueInfo->dummy_dev, &prGlueInfo->napi,
			kalNapiPoll);
#else
	netif_napi_add(&prGlueInfo->dummy_dev, &prGlueInfo->napi,
			kalNapiPoll, NAPI_POLL_WEIGHT);
#endif
#if CFG_SUPPORT_RX_NAPI_THREADED
	kalNapiThreadedInit(prGlueInfo);
#endif /* CFG_SUPPORT_RX_NAPI_THREADED */
#if CFG_NAPI_DELAY
	kalNapiDelayTimerInit(prGlueInfo);
#endif /* CFG_NAPI_DELAY */
	DBGLOG(INIT, TRACE, "Napi Init Done\n");
	return 0;
}

uint8_t kalNapiUninit(struct GLUE_INFO *prGlueInfo)
{
	netif_napi_del(&prGlueInfo->napi);
#if CFG_SUPPORT_RX_NAPI_THREADED
	kalNapiThreadedUninit(prGlueInfo);
#endif /* CFG_SUPPORT_RX_NAPI_THREADED */
#if CFG_NAPI_DELAY
	kalNapiDelayTimerUninit(prGlueInfo);
#endif /* CFG_NAPI_DELAY */
	DBGLOG(INIT, TRACE, "Napi Uninit Done\n");
	return 0;
}

#if (CFG_SUPPORT_RX_NAPI == 1)
#if CFG_SUPPORT_RX_WORK
static void kalNapiWakeup(void)
{
	/*
	 * napi_schedule use __raise_softirq_irqoff instead of
	 * raise_softirq_irqoff since it expect this api only use in irq
	 * context.
	 *
	 * since __raise_softirq_irqoff only or_softirq_pending and does not
	 * wakeup ksoftirqd, it might introduce latency when there is no more
	 * irq or local_bh_enable on this cpu.
	 *
	 * In order to fix this issue, we need to call local_bh_enable to
	 * run __do_softirq to prevent this pending softirq left in the cpu
	 * and introduce latency for NET_RX_SOFTIRQ to call kalNapiPoll.
	 */
#if !CFG_SUPPORT_RX_NAPI_THREADED
	kalTraceEvent("kalNapiWakeup");
	local_bh_disable();
	local_bh_enable();
#endif /* !CFG_SUPPORT_RX_NAPI_THREADED */
}
#endif /* CFG_SUPPORT_RX_WORK */

static inline void __kalNapiSchedule(struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo;
	struct RX_CTRL *prRxCtrl;

	if (!prAdapter || !prAdapter->prGlueInfo)
		return;

	prRxCtrl = &prAdapter->rRxCtrl;
	prGlueInfo = prAdapter->prGlueInfo;

	RX_INC_CNT(prRxCtrl, RX_NAPI_SCHEDULE_COUNT);

	GLUE_SET_REF_CNT(1, prGlueInfo->fgNapiScheduled);
	kal_napi_schedule(prGlueInfo->prRxDirectNapi);
}

static inline void _kalNapiSchedule(struct ADAPTER *prAdapter)
{
#if CFG_SUPPORT_RX_NAPI_IN_RX_THREAD
	set_bit(GLUE_FLAG_RX_TO_OS_BIT, &prAdapter->prGlueInfo->ulFlag);
	wake_up_interruptible(&prAdapter->prGlueInfo->waitq_rx);
#elif CFG_SUPPORT_RX_NAPI_WORK
	kalRxNapiWorkSchedule(prAdapter->prGlueInfo);
#else /* CFG_SUPPORT_RX_NAPI_WORK */
	__kalNapiSchedule(prAdapter);
#endif /* CFG_SUPPORT_RX_NAPI_WORK */
}

static void kalNapiScheduleCheck(struct GLUE_INFO *pr)
{
	static OS_SYSTIME now, last;
	uint32_t u4ScheduleTimeout;
	uint32_t u4ScheduleCnt, u4NapiPollCnt;

	GET_BOOT_SYSTIME(&now);

	u4ScheduleCnt =
		RX_GET_CNT(&pr->prAdapter->rRxCtrl, RX_NAPI_SCHEDULE_COUNT);
	u4NapiPollCnt =
		RX_GET_CNT(&pr->prAdapter->rRxCtrl, RX_NAPI_POLL_COUNT);

	if (!pr->fgNapiScheduled) {
		pr->u4LastScheduleCnt = u4ScheduleCnt;
		pr->u4LastNapiPollCnt = u4NapiPollCnt;
		last = now;
		return;
	}

	if (pr->u4LastNapiPollCnt != 0 &&
	    u4ScheduleCnt > pr->u4LastScheduleCnt &&
	    u4NapiPollCnt == pr->u4LastNapiPollCnt) {
		u4ScheduleTimeout =
			pr->prAdapter->rWifiVar.u4NapiScheduleTimeout
				* MSEC_PER_SEC;
		if (CHECK_FOR_TIMEOUT(now, last,
			MSEC_TO_SYSTIME(u4ScheduleTimeout)))
			kalSendAeeWarning("Napi Schedule Timeout",
				"Napi Schedule Timeout\n");
	} else {
		pr->u4LastScheduleCnt = u4ScheduleCnt;
		pr->u4LastNapiPollCnt = u4NapiPollCnt;
		last = now;
	}
}

#if CFG_NAPI_DELAY
enum hrtimer_restart kalNapiDelayTimeout(struct hrtimer *timer)
{
	struct GLUE_INFO *pr = container_of(timer, struct GLUE_INFO,
						rNapiDelayTimer);

	DBGLOG(INIT, TEMP, "NapiDelayTimeout\n");

	_kalNapiSchedule(pr->prAdapter);

	clear_bit(NAPI_DELAY_START_BIT, &pr->ulNapiDelayFlag);
	return HRTIMER_NORESTART;
}

static inline void kalNapiDelayTimerStop(struct GLUE_INFO *pr)
{
	/* skip if timer is not started */
	if (test_and_clear_bit(NAPI_DELAY_START_BIT, &pr->ulNapiDelayFlag) == 0)
		return;

	hrtimer_cancel(&pr->rNapiDelayTimer);
	DBGLOG(INIT, TEMP, "NapiDelayTimerStop\n");
}

static inline void kalNapiDelayTimerStart(struct GLUE_INFO *pr,
	uint32_t u4Timeout)
{
	unsigned long nsecs;
	ktime_t delay;

	/* skip if timer is started */
	if (test_and_set_bit(NAPI_DELAY_START_BIT, &pr->ulNapiDelayFlag) == 1)
		return;

	DBGLOG(INIT, TEMP, "NapiDelayTimerStart u4Timeout:%u\n", u4Timeout);
	nsecs = u4Timeout * 1E6L;
	delay = ktime_set(0, nsecs);
	hrtimer_start(&pr->rNapiDelayTimer, delay, HRTIMER_MODE_REL);
}

static void kalNapiDelayTimerInit(struct GLUE_INFO *pr)
{
	hrtimer_init(&pr->rNapiDelayTimer, CLOCK_MONOTONIC,
			HRTIMER_MODE_REL);
	pr->rNapiDelayTimer.function = kalNapiDelayTimeout;
	pr->ulNapiDelayFlag = 0;
}

static void kalNapiDelayTimerUninit(struct GLUE_INFO *pr)
{
	kalNapiDelayTimerStop(pr);
}

static void kalNapiDelayCheck(struct GLUE_INFO *pr)
{
	struct ADAPTER *ad = pr->prAdapter;
	struct WIFI_VAR *prWifiVar = &ad->rWifiVar;

	if (kalGetTpMbps(ad, PKT_PATH_RX) < prWifiVar->u4NapiDelayTputTh) {
		if (test_and_clear_bit(NAPI_DELAY_ENABLE_BIT,
			&pr->ulNapiDelayFlag) == 1)
			DBGLOG(INIT, INFO, "Disable NAPI Delay\n");
	} else {
		if (test_and_set_bit(NAPI_DELAY_ENABLE_BIT,
			&pr->ulNapiDelayFlag) == 0)
			DBGLOG(INIT, INFO, "Enable NAPI Delay\n");
	}
}

static u_int8_t kalIsNapiDelay(struct GLUE_INFO *pr)
{
	struct ADAPTER *ad = pr->prAdapter;
	struct WIFI_VAR *prWifiVar = &ad->rWifiVar;

	if (test_bit(NAPI_DELAY_ENABLE_BIT, &pr->ulNapiDelayFlag) == 0)
		goto end;

	/* already call napi_schedule, just skip schedule */
	if (test_bit(NAPI_DELAY_SCHEDULE_BIT, &pr->ulNapiDelayFlag) == 1)
		return TRUE;

	/* start timer when delay napi, skip schedule */
	if (KAL_GET_FIFO_CNT(pr) < prWifiVar->u4NapiDelayCntTh) {
		kalNapiDelayTimerStart(pr, prWifiVar->u4NapiDelayTimeout);
		return TRUE;
	}

	set_bit(NAPI_DELAY_SCHEDULE_BIT, &pr->ulNapiDelayFlag);
end:
	kalNapiDelayTimerStop(pr);
	return FALSE;
}
#endif /* CFG_NAPI_DELAY */

void kalNapiSchedule(struct ADAPTER *ad)
{
#if CFG_NAPI_DELAY
	if (kalIsNapiDelay(ad->prGlueInfo))
		return;
#endif /* CFG_NAPI_DELAY */

	_kalNapiSchedule(ad);
}

uint8_t kalNapiRxDirectInit(struct GLUE_INFO *prGlueInfo)
{
	if (!prGlueInfo
		|| !HAL_IS_RX_DIRECT(prGlueInfo->prAdapter))
		return FALSE;

	/* Note: we use FIFO to transfer addresses of SwRfbs
	 * The max size of FIFO queue should be
	 *     MaxPktCnt * "size of data obj pointer"
	 */
	prGlueInfo->u4RxKfifoBufLen =
		kalRoundUpPowerOf2(CFG_RX_MAX_PKT_NUM) * sizeof(void *);
	prGlueInfo->prRxKfifoBuf = kalMemAlloc(
		prGlueInfo->u4RxKfifoBufLen,
		VIR_MEM_TYPE);

	if (!prGlueInfo->prRxKfifoBuf) {
		DBGLOG(INIT, ERROR,
			"Cannot alloc buf(%d) for NapiDirect\n",
			prGlueInfo->u4RxKfifoBufLen);
		return FALSE;
	}

	KAL_FIFO_INIT(&prGlueInfo->rRxKfifoQ,
		prGlueInfo->prRxKfifoBuf,
		prGlueInfo->u4RxKfifoBufLen);

	prGlueInfo->prRxDirectNapi = &prGlueInfo->napi;

	DBGLOG(INIT, TRACE,
		"Init NapiDirect done Buf[%p:%u] Fifo[%p:%u]\n",
		prGlueInfo->prRxKfifoBuf,
		prGlueInfo->u4RxKfifoBufLen,
		&prGlueInfo->rRxKfifoQ,
		KAL_FIFO_LEN(&prGlueInfo->rRxKfifoQ)
		);

	return TRUE;
}

uint8_t kalNapiRxDirectUninit(struct GLUE_INFO *prGlueInfo)
{
	struct SW_RFB *prSwRfb;

	if (!prGlueInfo
		|| !HAL_IS_RX_DIRECT(prGlueInfo->prAdapter))
		return FALSE;

	prGlueInfo->prRxDirectNapi = NULL;

	/* Return pending SwRFBs */
	while (KAL_FIFO_OUT(&prGlueInfo->rRxKfifoQ, prSwRfb)) {
		if (!prSwRfb) {
			DBGLOG(RX, ERROR, "prSwRfb null\n");
			break;
		}
		RX_INC_CNT(&prGlueInfo->prAdapter->rRxCtrl,
			RX_NAPI_FIFO_OUT_COUNT);
		nicRxReturnRFB(prGlueInfo->prAdapter, prSwRfb);
	}

	if (prGlueInfo->prRxKfifoBuf) {
		kalMemFree(prGlueInfo->prRxKfifoBuf,
			VIR_MEM_TYPE,
			prGlueInfo->u4RxKfifoBufLen);
		prGlueInfo->prRxKfifoBuf = NULL;
	}

	DBGLOG(INIT, TRACE, "Uninit NapiDirect done\n");

	return TRUE;
}

static int kalNapiPollSwRfb(struct napi_struct *napi, int budget)
{
	uint32_t work_done = 1;
	struct GLUE_INFO *prGlueInfo =
		CONTAINER_OF(napi, struct GLUE_INFO, napi);
	struct ADAPTER *prAdapter;
	static int32_t i4UserCnt;
	struct SW_RFB *prSwRfb;
	uint32_t u4Cnt;

	/* Allow one user only */
	if (GLUE_INC_REF_CNT(i4UserCnt) > 1)
		goto end;

	prAdapter = prGlueInfo->prAdapter;

	/*
	 * Indicate Rfb dequeue by main_thread:
	 * 1. nicEventHandleDelayBar
	 * 2. nicEventHandleFwDropSSN
	 */
	nicRxIndicateRfbMainToNapi(prAdapter);

#if CFG_NAPI_DELAY
	DBGLOG(RX, TEMP, "FIFO_CNT:%u\n", KAL_GET_FIFO_CNT(prGlueInfo));
#endif /* CFG_NAPI_DELAY */

	u4Cnt = KAL_GET_FIFO_CNT(prGlueInfo);
	while ((work_done <= u4Cnt) &&
	       KAL_FIFO_OUT(&prGlueInfo->rRxKfifoQ, prSwRfb)) {
		if (!prSwRfb) {
			DBGLOG(RX, ERROR, "prSwRfb null\n");
			break;
		}
#if CFG_RFB_TRACK
		RX_RFB_TRACK_UPDATE(prAdapter,
			prSwRfb, RFB_TRACK_NAPI);
#endif /* CFG_RFB_TRACK */
		RX_INC_CNT(&prAdapter->rRxCtrl,
			RX_NAPI_FIFO_OUT_COUNT);
		nicRxProcessPacketType(prAdapter, prSwRfb);
		work_done++;
#if CFG_SUPPORT_SKB_ALLOC_WORK
		kalSkbAllocWorkSchedule(prGlueInfo, FALSE);
#endif /* CFG_SUPPORT_SKB_ALLOC_WORK */
	}

#if CFG_SUPPORT_SKB_ALLOC_WORK
	kalSkbAllocWorkSchedule(prGlueInfo, TRUE);
#endif /* CFG_SUPPORT_SKB_ALLOC_WORK */
#if CFG_NAPI_DELAY
	clear_bit(NAPI_DELAY_SCHEDULE_BIT, &prGlueInfo->ulNapiDelayFlag);
#endif /* CFG_NAPI_DELAY */

#if CFG_SUPPORT_RX_GRO_PEAK
	work_done = budget / 2;
#else /* CFG_SUPPORT_RX_GRO_PEAK */
	/* Set max work_done budget */
	if (work_done > budget)
		work_done = budget;
#endif /* CFG_SUPPORT_RX_GRO_PEAK */

end:
	GLUE_DEC_REF_CNT(i4UserCnt);

#if !CFG_SUPPORT_RX_GRO_PEAK
	if (work_done < budget)
#endif
		kal_napi_complete_done(napi, work_done);

	return work_done;
}
#else
/* dummy header only */
uint8_t kalNapiRxDirectInit(struct GLUE_INFO *prGlueInfo)
{
	return FALSE;
}
uint8_t kalNapiRxDirectUninit(struct GLUE_INFO *prGlueInfo)
{
	return FALSE;
}
static int kalNapiPollSwRfb(struct napi_struct *napi, int budget)
{
	return 0;
}
#endif /* (CFG_SUPPORT_RX_NAPI == 1)*/

/*----------------------------------------------------------------------------*/
/*!
 * \brief A method of callback function for napi struct
 *
 * It just return false because driver indicate Rx packet directly.
 *
 * \param[in] napi      Pointer to struct napi_struct.
 * \param[in] budget    Polling time interval.
 *
 * \return false
 */
/*----------------------------------------------------------------------------*/
int kalNapiPoll(struct napi_struct *napi, int budget)
{
#if CFG_SUPPORT_RX_NAPI
	int work_done = 0;
	struct sk_buff *prSkb = NULL;
	struct GLUE_INFO *prGlueInfo =
		CONTAINER_OF(napi, struct GLUE_INFO, napi);
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
	struct RX_BA_ENTRY *prReorderQueParm;

	struct sk_buff_head rFlushSkbQ;
	struct sk_buff_head *prRxNapiSkbQ, *prFlushSkbQ;
	unsigned long u4Flags;
#if CFG_SUPPORT_RX_GRO_PEAK
	/* follow timeout rule in net_rx_action() */
	const unsigned long ulTimeLimit = jiffies + 2;
#endif
	GLUE_SET_REF_CNT(0, prGlueInfo->fgNapiScheduled);
	RX_INC_CNT(&prGlueInfo->prAdapter->rRxCtrl, RX_NAPI_POLL_COUNT);

#if CFG_QUEUE_RX_IF_CONN_NOT_READY
	if (HAL_IS_RX_DIRECT(prAdapter))
		nicRxDequeuePendingQueue(prAdapter);
#endif /* CFG_QUEUE_RX_IF_CONN_NOT_READY */

	/* Added in qmHandleReorderBubbleTimeout */
	while (prReorderQueParm =
			getReorderQueParm(&prAdapter->rTimeoutRxBaEntry,
				prAdapter, SPIN_LOCK_RX_FLUSH_TIMEOUT))
		qmFlushTimeoutReorderBubble(prAdapter, prReorderQueParm);

	/* Added in qmDelRxBaEntry */
	while (prReorderQueParm =
			getReorderQueParm(&prAdapter->rFlushRxBaEntry,
				prAdapter, SPIN_LOCK_RX_FLUSH_BA))
		qmFlushDeletedBaReorder(prAdapter, prReorderQueParm);

	if (HAL_IS_RX_DIRECT(prGlueInfo->prAdapter)) {
		/* Handle SwRFBs under RX-direct mode */
		return TRACE(kalNapiPollSwRfb(napi, budget),
			"kalNapiPollSwRfb");
	}

	prRxNapiSkbQ = &prGlueInfo->rRxNapiSkbQ;
	prFlushSkbQ = &rFlushSkbQ;
#if KERNEL_VERSION(3, 19, 0) <= LINUX_VERSION_CODE
	hrtimer_cancel(&napi->timer);
#endif /* KERNEL_VERSION(3, 19, 0) */

#if CFG_SUPPORT_RX_GRO_PEAK
next_try:
#endif
	/* Flush all RX SBKs to local SkbQ */
	__skb_queue_head_init(prFlushSkbQ);
	if (skb_queue_len(prRxNapiSkbQ)) {
		spin_lock_irqsave(&prRxNapiSkbQ->lock, u4Flags);
		skb_queue_splice_init(prRxNapiSkbQ, prFlushSkbQ);
		spin_unlock_irqrestore(&prRxNapiSkbQ->lock, u4Flags);
	}

	while ((work_done < budget) &&
	       skb_queue_len(prFlushSkbQ)) {

		prSkb = __skb_dequeue(prFlushSkbQ);
		if (!prSkb) {
			DBGLOG(RX, ERROR, "skb NULL %d %d\n",
				work_done, skb_queue_len(prFlushSkbQ));
			kal_napi_complete_done(napi, work_done);
			return work_done;
		}

		/*
		* Take this line instead to skip GRO in NAPI
		* if (netif_receive_skb(prSkb) != NET_RX_SUCCESS)
		*/
		RX_INC_CNT(&prGlueInfo->prAdapter->rRxCtrl,
			RX_DATA_NAPI_GRO_RECEIVE_COUNT);
#if KERNEL_VERSION(5, 12, 0) <= CFG80211_VERSION_CODE
		if (napi_gro_receive(napi, prSkb) == GRO_MERGED_FREE)
#else
		if (napi_gro_receive(napi, prSkb) == GRO_DROP)
#endif
			continue;

		work_done++;
	}

	if (skb_queue_len(prFlushSkbQ)) {
		spin_lock_irqsave(&prRxNapiSkbQ->lock, u4Flags);
		skb_queue_splice_init(prRxNapiSkbQ, prFlushSkbQ);
		skb_queue_splice_init(prFlushSkbQ, prRxNapiSkbQ);
		spin_unlock_irqrestore(&prRxNapiSkbQ->lock, u4Flags);
	}

#if CFG_SUPPORT_RX_GRO_PEAK
	if (skb_queue_len(prRxNapiSkbQ)
		&& time_before_eq(jiffies, ulTimeLimit)) {
		work_done = 0;
		goto next_try;
	}

	/* Debug check only */
	if (!time_before_eq(jiffies, ulTimeLimit))
		DBGLOG(RX, WARN, "timeout hit %lu\n", jiffies-ulTimeLimit);
#endif /* CFG_SUPPORT_RX_GRO_PEAK */
	work_done = kal_min_t(int, work_done, budget-1);
	kal_napi_complete_done(napi, work_done);
	if (skb_queue_len(prRxNapiSkbQ)) {
		RX_INC_CNT(&prAdapter->rRxCtrl, RX_NAPI_LEGACY_SCHED_COUNT);
		napi_schedule(napi);
	}

	return work_done;
#else /* CFG_SUPPORT_RX_NAPI */
	return 0;
#endif /* CFG_SUPPORT_RX_NAPI */
}

uint8_t kalNapiEnable(struct GLUE_INFO *prGlueInfo)
{
	napi_enable(&prGlueInfo->napi);
	DBGLOG(RX, TRACE, "RX NAPI enabled\n");
	return 0;
}

uint8_t kalNapiDisable(struct GLUE_INFO *prGlueInfo)
{
	DBGLOG(RX, INFO, "RX NAPI disable ongoing\n");
	GLUE_SET_REF_CNT(0, prGlueInfo->fgNapiScheduled);
	napi_synchronize(&prGlueInfo->napi);
	napi_disable(&prGlueInfo->napi);
	if (skb_queue_len(&prGlueInfo->rRxNapiSkbQ)) {
		struct sk_buff *skb;

		DBGLOG(INIT, WARN, "NAPI Remain pkts %d\n",
			skb_queue_len(&prGlueInfo->rRxNapiSkbQ));

		while ((skb = skb_dequeue(&prGlueInfo->rRxNapiSkbQ))
				!= NULL)
			kfree_skb(skb);
	}
	DBGLOG(RX, TRACE, "RX NAPI disabled\n");
	return 0;
}
#endif

uint8_t kalRxNapiValidSkb(struct GLUE_INFO *prGlueInfo,
	struct sk_buff *prSkb)
{
#if (CFG_SUPPORT_RX_GRO == 0) || (CFG_SUPPORT_RX_NAPI == 0)
	return FALSE;
#else
	uint8_t ucBssIdx;

	ucBssIdx = GLUE_GET_PKT_BSS_IDX(prSkb);

	return kal_is_skb_gro(prGlueInfo->prAdapter, ucBssIdx);
#endif
}

#if (CFG_WLAN_ATF_SUPPORT == 1)
uint32_t kalSendAtfSmcCmd(uint32_t u4Opid, uint32_t u4Arg2,
	uint32_t u4Arg3, uint32_t u4Arg4)
{
	struct GLUE_INFO *prGlueInfo;
	struct arm_smccc_res res;
	int32_t i4Ret;

	arm_smccc_smc(MTK_SIP_KERNEL_WLAN_CONTROL, u4Opid,
			u4Arg2, u4Arg3, u4Arg4, 0, 0, 0, &res);

	i4Ret = (int32_t)res.a0;
	switch (i4Ret) {
	case SMC_WLAN_SUCCESS:
		break;
	case -SMC_WLAN_UNKNOWN_OPID:
		DBGLOG(SMC, WARN, "Invaild SMC opid[%u]\n", u4Opid);
		break;
	case -SMC_WLAN_INVALID_REGISTER:
		DBGLOG(SMC, WARN, "Invaild address access[0x%08x]\n",
			u4Arg2);
		break;
	default:
		if (i4Ret < 0)
			DBGLOG(SMC, WARN, "Unknown status code[%d]\n", i4Ret);
		break;
	}

	if (i4Ret < 0) {
		WIPHY_PRIV(wlanGetWiphy(), prGlueInfo);
		if (prGlueInfo && prGlueInfo->u4ReadyFlag) {
			GL_DEFAULT_RESET_TRIGGER(prGlueInfo->prAdapter,
				RST_SMC_CMD_FAIL);
		} else {
#define SMC_FAIL_LOG_TEMPLATE \
	"SMC CMD failed. Opid[%u] Arg2[0x%08x] Arg3[0x%08x] Arg4[0x%08x]\n" \
	"CRDISPATCH_KEY: WLAN SMC CMD failed\n"
			dump_stack();
			kalSendAeeWarning("wlan", SMC_FAIL_LOG_TEMPLATE,
				u4Opid, u4Arg2, u4Arg3, u4Arg4);
		}
	}

	return res.a0;
}
#endif

void kalSetLogTooMuch(uint32_t u4DriverLevel,
	uint32_t u4FwLevel)
{
#if KERNEL_VERSION(4, 14, 0) >= LINUX_VERSION_CODE
#if (CFG_BUILT_IN_DRIVER == 0) && (CFG_MTK_ANDROID_WMT == 1)
	/*
	 * The function definition of get_logtoomuch_enable() and
	 * set_logtoomuch_enable of Android O0 or lower version are different
	 * from that of Android O1 or higher version. Wlan driver supports .ko
	 * module from Android O1. Use CFG_BUILT_IN_DRIVER to distinguish
	 * Android version higher than O1 instead.
	 */
	if ((u4DriverLevel > ENUM_WIFI_LOG_LEVEL_DEFAULT ||
			u4FwLevel > ENUM_WIFI_LOG_LEVEL_DEFAULT) &&
			get_logtoomuch_enable()) {
		DBGLOG(OID, TRACE,
			"Disable print log too much. driver: %d, fw: %d\n",
			u4DriverLevel,
			u4FwLevel);
		set_logtoomuch_enable(0);
	}
#endif
#endif /* KERNEL_VERSION(4, 14, 0) >= LINUX_VERSION_CODE */
}

uint64_t kalGetUIntRealTime(void)
{
	return ktime_get_real_ns();
}

void kalGetRealTime(struct REAL_TIME *prRealTime)
{
	struct rtc_time tm;
	struct timespec64 tv = { 0 };

	ktime_get_real_ts64(&tv);
	rtc_time64_to_tm(tv.tv_sec, &tm);
	prRealTime->i4TmMon = tm.tm_mon;
	prRealTime->i4TmDay = tm.tm_mday;
	prRealTime->i4TmHour = tm.tm_hour;
	prRealTime->i4TmMin = tm.tm_min;
	prRealTime->i4TmSec = tm.tm_sec;
	prRealTime->u4TvValSec = (uint32_t)tv.tv_sec;
	prRealTime->u4TvValUsec = (uint32_t)KAL_GET_USEC(tv);
}

void kalVendorEventRssiBeyondRange(
	struct GLUE_INFO *prGlueInfo,
	uint8_t ucBssIdx, int rssi)
{
#if KERNEL_VERSION(3, 16, 0) <= LINUX_VERSION_CODE
	mtk_cfg80211_vendor_event_rssi_beyond_range(prGlueInfo,
		aisGetDefaultLinkBssIndex(prGlueInfo->prAdapter), rssi);
#endif
}


#if CFG_SUPPORT_TPENHANCE_MODE
inline uint64_t kalTpeTimeUs(void)
{
	struct timespec64 _now;

	ktime_get_ts64(&_now);

	return (uint64_t)((int)_now.tv_sec * 1000000 +
			(int)KAL_GET_USEC(_now));
}

void kalTpeUpdate(struct GLUE_INFO *prGlueInfo, struct QUE *prSrcQue,
		uint8_t ucPktJump)
{
	int addPktCnt = 0;
	uint8_t *pucIpHeader = NULL;
	uint8_t *pucPktHeadBuf = NULL;
	uint8_t ucHitPkt;
	uint16_t *pu2DPort, *pu2SPort;
	uint32_t *pu4Ip;
	uint32_t u4Cnt, u4Cnt2, u4PktMax;
	struct QUE rTempQue;
	struct QUE *prTempQue = &rTempQue;
	struct QUE_ENTRY *prQueueEntry = NULL;
	struct sk_buff *prSkb;
	struct TPENHANCE_PKT_MAP auPktMap[TPENHANCE_SESSION_MAP_LEN];
	struct TPENHANCE_PKT_MAP *prPktMap;

	if (prGlueInfo == NULL) {
		DBGLOG(INIT, ERROR, "prGlueInfo is NULL\n");
		return;
	}

	if (!prSrcQue || QUEUE_IS_EMPTY(prSrcQue)) {
		DBGLOG(QM, TRACE, "Nothing to handle\n");
		return;
	}

	/* Resource init */
	QUEUE_INITIALIZE(prTempQue);
	kalMemZero(auPktMap, sizeof(auPktMap));
	u4PktMax = prSrcQue->u4NumElem;

	/* Queue revert */
	while (QUEUE_IS_NOT_EMPTY(prSrcQue)) {
		QUEUE_REMOVE_HEAD(prSrcQue, prQueueEntry,
			  struct QUE_ENTRY *);
		QUEUE_INSERT_HEAD(prTempQue, prQueueEntry);
	}
	QUEUE_MOVE_ALL(prSrcQue, prTempQue);

	/* Loop all pkts in waiting-Q */
	for (u4Cnt = 0; u4Cnt < u4PktMax; u4Cnt++) {
		QUEUE_REMOVE_HEAD(prSrcQue, prQueueEntry,
			  struct QUE_ENTRY *);
		if (!prQueueEntry)
			break;
		prSkb = (struct sk_buff *)GLUE_GET_PKT_DESCRIPTOR(
					  prQueueEntry);
		pucPktHeadBuf = prSkb->data;
		pucIpHeader = &pucPktHeadBuf[ETHER_HEADER_LEN];
		pu4Ip = (uint32_t *)(pucIpHeader + IPV4_HDR_IP_SRC_ADDR_OFFSET);
		pu2SPort = (uint16_t *)(pucIpHeader + IPV4_HDR_LEN);
		pu2DPort = (uint16_t *)(pucIpHeader +
					IPV4_HDR_LEN + 2);
		/* Check if having this session ack already */
		ucHitPkt = FALSE;
		for (u4Cnt2 = 0; u4Cnt2 < ARRAY_SIZE(auPktMap);
			u4Cnt2++) {
			if (u4PktMax < TPENHANCE_PKT_LATCH_MIN)
				break;

			prPktMap = &auPktMap[u4Cnt2];

			if (prPktMap->au2SPort == 0) {
				/*
				 * This is a new session.
				 * Create a new record and keep this ack.
				 */
				prPktMap->au2SPort = *pu2SPort;
				prPktMap->au2DPort = *pu2DPort;
				prPktMap->au4Ip = *pu4Ip;
				prPktMap->au2Hit++;
				break;
			}

			if (prPktMap->au2SPort == *pu2SPort
				&& prPktMap->au2DPort == *pu2DPort
				&& prPktMap->au4Ip == *pu4Ip) {
				/* A duplicated session found */
				if (ucPktJump == prPktMap->au2Hit) {
					/*
					 * reset hit counter &&
					 * keep this ack
					 */
					prPktMap->au2Hit = 0;
				} else {
					prPktMap->au2Hit++;
					ucHitPkt = TRUE;
				}
				break;
			}
		}

		if (ucHitPkt)
			dev_kfree_skb(prSkb);
		else {
			uint8_t ucBssIndex =
				GLUE_GET_PKT_BSS_IDX(prSkb);
			uint16_t u2QueueIdx =
				skb_get_queue_mapping(prSkb);

			GLUE_INC_REF_CNT(
			prGlueInfo->ai4TxPendingFrameNumPerQueue
			[ucBssIndex][u2QueueIdx]);

			GLUE_INC_REF_CNT(
			prGlueInfo->i4TxPendingFrameNum);
			QUEUE_INSERT_TAIL(prSrcQue, prQueueEntry);
			addPktCnt++;
		}
	}

	/* Queue revert */
	while (QUEUE_IS_NOT_EMPTY(prSrcQue)) {
		QUEUE_REMOVE_HEAD(prSrcQue, prQueueEntry,
				struct QUE_ENTRY *);
		QUEUE_INSERT_HEAD(prTempQue, prQueueEntry);
	}
	QUEUE_MOVE_ALL(prSrcQue, prTempQue);


	if (addPktCnt != prSrcQue->u4NumElem)
		DBGLOG(QM, ERROR, "mismatch : %d != %d\n",
			addPktCnt, prSrcQue->u4NumElem);
}

void kalTpeFlush(struct GLUE_INFO *prGlueInfo)
{
	struct QUE rTempQue;
	struct QUE *prTempQue = &rTempQue;
	struct QUE *prTpeAckQueue;
	struct QUE *prTxQueue;

	GLUE_SPIN_LOCK_DECLARATION();

	if (prGlueInfo == NULL) {
		DBGLOG(INIT, ERROR, "prGlueInfo is NULL\n");
		return;
	}

	prTpeAckQueue = &prGlueInfo->rTpeAckQueue;
	prTxQueue = &prGlueInfo->rTxQueue;

	if (QUEUE_IS_EMPTY(prTpeAckQueue))
		return;

	QUEUE_INITIALIZE(prTempQue);

	/* Ack-Q clean first */
	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TXACK_QUE);
	QUEUE_CONCATENATE_QUEUES(prTempQue, prTpeAckQueue);
	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TXACK_QUE);

	kalTpeUpdate(prGlueInfo, prTempQue,
		prGlueInfo->prAdapter->rWifiVar.ucTpEnhancePktNum);

	prGlueInfo->u8TpeTimestamp = kalTpeTimeUs();

	/* Append to Tx-Q */
	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_QUE);
	QUEUE_CONCATENATE_QUEUES(prTxQueue, prTempQue);
	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_QUE);
}

#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
void kalTpeTimeoutHandler(struct timer_list *timer)
#else
void kalTpeTimeoutHandler(unsigned long ulData)
#endif
{
#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
	struct GLUE_INFO *prGlueInfo = from_timer(prGlueInfo, timer, rTpeTimer);
#else
	struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *)ulData;
#endif

	if (prGlueInfo == NULL) {
		DBGLOG(INIT, ERROR, "prGlueInfo is NULL\n");
		return;
	}

	if (QUEUE_IS_NOT_EMPTY(&prGlueInfo->rTpeAckQueue)) {
		/* There are some pkts in ack-Q */
		/* de-Q all pkts right now */
		kalTpeFlush(prGlueInfo);
		kalSetEvent(prGlueInfo); /* Wakeup TX */
	}
}

void kalTpeInit(struct GLUE_INFO *prGlueInfo)
{
	struct QUE *prTpeAckQueue;
	struct ADAPTER *prAdapter;

	if (prGlueInfo == NULL) {
		DBGLOG(INIT, ERROR, "prGlueInfo is NULL\n");
		return;
	}

	prAdapter = prGlueInfo->prAdapter;

	if (!prAdapter->rWifiVar.ucTpEnhanceEnable)
		return;

	prTpeAckQueue = &prGlueInfo->rTpeAckQueue;
	QUEUE_INITIALIZE(prTpeAckQueue);
	prGlueInfo->u4TpeMaxPktNum = TPENHANCE_PKT_KEEP_MAX;
	prGlueInfo->u4TpeTimeout =
		prAdapter->rWifiVar.u4TpEnhanceInterval; /* us */

	DBGLOG(HAL, STATE,
		"InitTpEnhance. PktNum:%d. Interval = %d. RSSI = %d\n",
		prAdapter->rWifiVar.ucTpEnhancePktNum,
		prAdapter->rWifiVar.u4TpEnhanceInterval,
		prAdapter->rWifiVar.cTpEnhanceRSSI);

#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
	timer_setup(&prGlueInfo->rTpeTimer, kalTpeTimeoutHandler, 0);
#else
	init_timer(&prGlueInfo->rTpeTimer);
	prGlueInfo->rTpeTimer.function = kalTpeTimeoutHandler;
	prGlueInfo->rTpeTimer.data = ((unsigned long) prGlueInfo);
#endif
	prGlueInfo->rTpeTimer.expires = jiffies - 10;
	add_timer(&prGlueInfo->rTpeTimer);
}


void kalTpeUninit(struct GLUE_INFO *prGlueInfo)
{
	struct ADAPTER *prAdapter = NULL;

	if (prGlueInfo == NULL) {
		DBGLOG(INIT, ERROR, "prGlueInfo is NULL\n");
		return;
	}

	prAdapter = prGlueInfo->prAdapter;
	if (!prAdapter->rWifiVar.ucTpEnhanceEnable)
		return;

	del_timer_sync(&(prGlueInfo->rTpeTimer));
}

int kalTpeProcess(struct GLUE_INFO *prGlueInfo,
			struct sk_buff *prSkb,
			struct net_device *prDev)
{
	struct QUE_ENTRY *prQueueEntry = NULL;
	struct ADAPTER *prAdapter;
	struct QUE *prTpeAckQueue;
	struct WIFI_VAR *prWifiVar;
	uint64_t u8Nowus;
	uint8_t ucBssIndex;
	int8_t cRssi;
	struct PERF_MONITOR *prPerMonitor =
		&prGlueInfo->prAdapter->rPerMonitor;

	if (prGlueInfo == NULL) {
		DBGLOG(INIT, ERROR, "prGlueInfo is NULL\n");
		return;
	}

	prAdapter = prGlueInfo->prAdapter;
	if (prAdapter == NULL) {
		DBGLOG(INIT, ERROR, "prAdapter is NULL\n");
		return;
	}

	prWifiVar = &prAdapter->rWifiVar;

	if (!prWifiVar->ucTpEnhanceEnable)
		return WLAN_STATUS_PENDING;

	prTpeAckQueue = &prGlueInfo->rTpeAckQueue;

	/* ranged from (-128 ~ 30) in unit of dBm */
	ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);
	cRssi = prAdapter->rLinkQuality.rLq[ucBssIndex].cRssi;

	u8Nowus = kalTpeTimeUs();

	if (cRssi < prWifiVar->cTpEnhanceRSSI)
		goto TpeEndFlush;
	else if (!GLUE_TEST_PKT_FLAG(prSkb, ENUM_PKT_TCP_ACK))
		goto TpeEndFlush;
	else if (KAL_TEST_BIT(PERF_MON_RUNNING_BIT,
		prPerMonitor->ulPerfMonFlag)) {
		uint16_t u2TputMbps = 0;

		struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPrivate =
			(struct NETDEV_PRIVATE_GLUE_INFO *) NULL;
		uint8_t ucBssIndex;

		if (prGlueInfo == NULL || prDev == NULL ||
			prSkb == NULL) {
			DBGLOG(INIT, ERROR, "GlueInfo/Dev/Skb NULL\n");
			return;
		}

		prNetDevPrivate = (struct NETDEV_PRIVATE_GLUE_INFO *)
				  netdev_priv(prDev);
		ucBssIndex = prNetDevPrivate->ucBssIdx;

		u2TputMbps = prPerMonitor->ulRxTp[ucBssIndex] >> 17;

		if (u2TputMbps < prWifiVar->u4TpEnhanceThreshold)
			goto TpeEndFlush;
	}

	/* more space to Q-in? */
	if (prTpeAckQueue->u4NumElem < prGlueInfo->u4TpeMaxPktNum) {
		/* Q-ing status */
		GLUE_SPIN_LOCK_DECLARATION();
		GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TXACK_QUE);

		/* Save timestamp for first pkt in Q */
		if (QUEUE_IS_EMPTY(prTpeAckQueue))
			prGlueInfo->u8TpeTimestamp = u8Nowus;

		prQueueEntry = (struct QUE_ENTRY *)
				GLUE_GET_PKT_QUEUE_ENTRY(prSkb);

		QUEUE_INSERT_TAIL(prTpeAckQueue, prQueueEntry);
		GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TXACK_QUE);

		/* Update NetDev statisitcs */
		prDev->stats.tx_bytes += prSkb->len;
		prDev->stats.tx_packets++;

		if (u8Nowus >= prGlueInfo->u8TpeTimestamp +
			prGlueInfo->u4TpeTimeout) {
			/* Timeout already, flush out directly. */
			if (timer_pending(&prGlueInfo->rTpeTimer))
				del_timer(&prGlueInfo->rTpeTimer);
			kalTpeFlush(prGlueInfo);
			kalSetEvent(prGlueInfo);
		} else {
			/* Setup a timer for next flushing */
			if (!timer_pending(&prGlueInfo->rTpeTimer))
				mod_timer(&prGlueInfo->rTpeTimer,
					jiffies + usecs_to_jiffies(
					prGlueInfo->u4TpeTimeout * 2));
		}
		return WLAN_STATUS_SUCCESS;
	} else if (QUEUE_IS_NOT_EMPTY(prTpeAckQueue)) {
		/* cannot Q, try to flush out */
		/* Flush ack-Q */
		kalTpeFlush(prGlueInfo);
	}

TpeEndFlush:
	/* Addtional check if Tpe-Q should be flushed! */
	if (QUEUE_IS_NOT_EMPTY(prTpeAckQueue)
		&&
		(prTpeAckQueue->u4NumElem >= prGlueInfo->u4TpeMaxPktNum
		|| u8Nowus >= prGlueInfo->u8TpeTimestamp +
			prGlueInfo->u4TpeTimeout)) {
		kalTpeFlush(prGlueInfo);
	}
	return WLAN_STATUS_PENDING;
}
#endif /* CFG_SUPPORT_TPENHANCE_MODE */


void kalTxDirectInitSkbQ(struct GLUE_INFO *prGlueInfo)
{
	skb_queue_head_init(&prGlueInfo->rTxDirectSkbQueue);
}

void kalTxDirectClearSkbQ(struct GLUE_INFO *prGlueInfo)
{
	struct sk_buff *prSkb;

	if (prGlueInfo == NULL) {
		DBGLOG(TX, ERROR, "prGlueInfo NULL\n");
		return;
	}

	while (TRUE) {
		spin_lock_bh(&prGlueInfo->rSpinLock[SPIN_LOCK_TX_DIRECT]);
		prSkb = skb_dequeue(&prGlueInfo->rTxDirectSkbQueue);
		spin_unlock_bh(&prGlueInfo->rSpinLock[SPIN_LOCK_TX_DIRECT]);
		if (prSkb == NULL)
			break;

		kalSendComplete(prGlueInfo, prSkb,
				WLAN_STATUS_NOT_ACCEPTED);
	}
}

void kalTxDirectStartCheckQTimer(struct GLUE_INFO *prGlueInfo,
				uint8_t offset)
{
	if (prGlueInfo == NULL) {
		DBGLOG(TX, ERROR, "prGlueInfo NULL\n");
		return;
	}

	mod_timer(&prGlueInfo->rTxDirectHifTimer, jiffies + offset);
}

void kalTxDirectStartCheckSkbQTimer(struct GLUE_INFO *prGlueInfo,
				uint8_t offset)
{
	if (prGlueInfo == NULL) {
		DBGLOG(TX, ERROR, "prGlueInfo NULL\n");
		return;
	}

	mod_timer(&prGlueInfo->rTxDirectSkbTimer, jiffies + offset);
}

/*----------------------------------------------------------------------------*/
/*
 * \brief This function is the timeout function of timer rTxDirectSkbTimer.
 *        The purpose is to check if rTxDirectSkbQueue has any skb to be sent.
 *
 * \param[in] data  Pointer of GlueInfo
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
void kalTxDirectTimerCheckSkbQ(struct timer_list *timer)
#else
void kalTxDirectTimerCheckSkbQ(unsigned long data)
#endif
{
#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
	struct GLUE_INFO *prGlueInfo =
		from_timer(prGlueInfo, timer, rTxDirectSkbTimer);
#else
	struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *)data;
#endif

	if (skb_queue_len(&prGlueInfo->rTxDirectSkbQueue))
		kalTxDirectStartXmit(NULL, prGlueInfo);
}


#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
void kalTxDirectTimerCheckHifQ(struct timer_list *timer)
#else
void kalTxDirectTimerCheckHifQ(unsigned long data)
#endif
{

#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
	struct GLUE_INFO *prGlueInfo =
		from_timer(prGlueInfo, timer, rTxDirectHifTimer);
#else
	struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *)data;
#endif

	spin_lock_bh(&prGlueInfo->rSpinLock[SPIN_LOCK_TX_DIRECT]);

	nicTxDirectTimerCheckHifQ(prGlueInfo->prAdapter);

	spin_unlock_bh(&prGlueInfo->rSpinLock[SPIN_LOCK_TX_DIRECT]);

#if CFG_TX_DIRECT_VIA_HIF_THREAD
	kalSetTxEvent2Hif(prGlueInfo);
#endif /* CFG_TX_DIRECT_VIA_HIF_THREAD */
}

/*----------------------------------------------------------------------------*/
/*
 * \brief This function is have to called by kalHardStartXmit().
 *        The purpose is to let as many as possible TX processing in softirq
 *        instead of in kernel thread to reduce TX CPU usage.
 *        NOTE: Currently only USB interface can use this function.
 *
 * \param[in] prSkb  Pointer of the sk_buff to be sent
 * \param[in] prGlueInfo  Pointer of prGlueInfo
 *
 * \retval WLAN_STATUS
 */
/*----------------------------------------------------------------------------*/
uint32_t kalTxDirectStartXmit(struct sk_buff *prSkb,
			      struct GLUE_INFO *prGlueInfo)
{
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
	struct MSDU_INFO *prMsduInfo;
	uint32_t ret = WLAN_STATUS_SUCCESS;
	struct sk_buff_head rLocalSkbQ;
	struct sk_buff_head *prLocalSkbQ, *prTxDirectSkbQ;
	unsigned long u4Flags;

	prLocalSkbQ = &rLocalSkbQ;
	prTxDirectSkbQ = &prGlueInfo->rTxDirectSkbQueue;
	__skb_queue_head_init(prLocalSkbQ);

	if (!spin_trylock_bh(&prGlueInfo->rSpinLock[SPIN_LOCK_TX_DIRECT])) {
		/* fail to get lock */
		if (prSkb)
			skb_queue_tail(prTxDirectSkbQ, prSkb);

		kalTxDirectStartCheckSkbQTimer(prGlueInfo,
				TX_DIRECT_CHECK_INTERVAL);
		return ret;
	}

	if (skb_queue_len(prTxDirectSkbQ)) {
		spin_lock_irqsave(&prTxDirectSkbQ->lock, u4Flags);
		skb_queue_splice_init(prTxDirectSkbQ, prLocalSkbQ);
		/* do enqueue before unlock to prevent ooo */
		if (prSkb)
			__skb_queue_tail(prLocalSkbQ, prSkb);
		spin_unlock_irqrestore(&prTxDirectSkbQ->lock, u4Flags);
	} else {
		if (prSkb)
			__skb_queue_tail(prLocalSkbQ, prSkb);
	}

	while (skb_queue_len(prLocalSkbQ)) {
		prMsduInfo = cnmPktAlloc(prAdapter, 0);
		if (unlikely(prMsduInfo == NULL)) {
			DBGLOG(TX, LOUD, "cnmPktAlloc NULL\n");
			break;
		}

		prSkb = __skb_dequeue(prLocalSkbQ);
		if (unlikely(prSkb == NULL)) {
			/* should not enter here, just sanity check */
			nicTxReturnMsduInfo(prAdapter, prMsduInfo);
			break;
		}

		nicTxDirectStartXmitMain((void *)prSkb, prMsduInfo,
			prAdapter, 0xff, 0xff, 0xff);
	}

	TX_INC_CNT(&prGlueInfo->prAdapter->rTxCtrl,
		TX_DIRECT_DEQUEUE_COUNT);

	if (unlikely(skb_queue_len(prLocalSkbQ))) {
		spin_lock_irqsave(&prTxDirectSkbQ->lock, u4Flags);
		skb_queue_splice_init(prTxDirectSkbQ, prLocalSkbQ);
		skb_queue_splice_init(prLocalSkbQ, prTxDirectSkbQ);
		spin_unlock_irqrestore(&prTxDirectSkbQ->lock, u4Flags);
	}

	spin_unlock_bh(&prGlueInfo->rSpinLock[SPIN_LOCK_TX_DIRECT]);

	if (skb_queue_len(prTxDirectSkbQ))
		kalTxDirectStartCheckSkbQTimer(prGlueInfo,
				TX_DIRECT_CHECK_INTERVAL);

	return ret;
}

uint32_t kalGetTxDirectQueueLength(
				struct GLUE_INFO *prGlueInfo)
{
	return skb_queue_len(&prGlueInfo->rTxDirectSkbQueue);
}

void kalKfreeSkb(void *pvPacket, u_int8_t fgIsFreeData)
{
	struct sk_buff *pkt = (struct sk_buff *)pvPacket;

	if (pkt) {
		if (!fgIsFreeData)
			pkt->head = NULL;

		kfree_skb(pkt);
	}
}

void *kalBuildSkb(void *pvPacket, uint32_t u4MgmtLength,
	uint32_t u4TotLen, u_int8_t fgIsSetLen)
{
	struct sk_buff *pkt;

	/* Add kalGetSKBSharedInfoSize() is due to build_skb
	 * API will assume user have contain this in length parameter,
	 * but we don't do this, so if not add this will result kernel
	 * overwrite the content which is not expected.
	 */
	pkt = build_skb(pvPacket, u4MgmtLength
		+ kalGetSKBSharedInfoSize());

	/* Not need send skb shared info to peers, so not add
	 * kalGetSKBSharedInfoSize() here.
	 */
	if (pkt && fgIsSetLen)
		pkt->len = u4TotLen;

	return (void *)pkt;
}

uint32_t kalGetSKBSharedInfoSize(void)
{
	return SKB_RESERVED_SIZE +
		SKB_DATA_ALIGN(sizeof(struct skb_shared_info));
}

uint32_t kalGetChannelFrequency(
				uint8_t ucChannel,
				uint8_t ucBand)
{
	struct ieee80211_channel *prChannel;

	prChannel = (struct ieee80211_channel *)
			kal_ieee80211_get_channel(
				wlanGetWiphy(),
				kal_ieee80211_channel_to_frequency
				(ucChannel, ucBand)
			);

	if (!prChannel) {
		log_dbg(SCN, ERROR, "Ch=NULL!\n");
		return 0;
	}

	return prChannel->center_freq;

}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Check if Channel supported by HW
 *
 * \param[in/out] eBand:          BAND_2G4, BAND_5G or BAND_NULL
 *                                (both 2.4G and 5G)
 *                ucNumOfChannel: channel number
 *
 * \return TRUE/FALSE
 */
/*----------------------------------------------------------------------------*/
u_int8_t kalIsValidChnl(struct GLUE_INFO *prGlueInfo,
			uint8_t ucNumOfChannel,
			enum ENUM_BAND eBand)
{
	struct ieee80211_supported_band *channelList;
	int i, chSize;
	struct wiphy *pWiphy;

	pWiphy = wlanGetWiphy();

#if (CFG_SUPPORT_WIFI_6G == 1)
	if (eBand == BAND_6G && pWiphy->bands[KAL_BAND_6GHZ]) {
		channelList = pWiphy->bands[KAL_BAND_6GHZ];
		chSize = channelList->n_channels;
	} else
#endif
	if (eBand == BAND_5G && pWiphy->bands[KAL_BAND_5GHZ]) {
		channelList = pWiphy->bands[KAL_BAND_5GHZ];
		chSize = channelList->n_channels;
	} else if (eBand == BAND_2G4 && pWiphy->bands[KAL_BAND_2GHZ]) {
		channelList = pWiphy->bands[KAL_BAND_2GHZ];
		chSize = channelList->n_channels;
	} else
		return FALSE;

	for (i = 0; i < chSize; i++) {
		if ((channelList->channels[i]).hw_value == ucNumOfChannel)
			return TRUE;
	}
	return FALSE;
}

/**
 * rlmDomainChannelFlagString - Transform channel flags to readable string
 *
 * @ flags: the ieee80211_channel->flags for a channel
 * @ buf: string buffer to put the transformed string
 * @ buf_size: size of the buf
 **/
void kalChannelFlagString(u32 flags, char *buf, size_t buf_size)
{
	int32_t buf_written = 0;

	if (!flags || !buf || !buf_size)
		return;

	if (flags & IEEE80211_CHAN_DISABLED) {
		LOGBUF(buf, ((int32_t)buf_size), buf_written, "DISABLED ");
		/* If DISABLED, don't need to check other flags */
		return;
	}
	if (flags & IEEE80211_CHAN_PASSIVE_FLAG)
		LOGBUF(buf, ((int32_t)buf_size), buf_written,
		       IEEE80211_CHAN_PASSIVE_STR " ");
	if (flags & IEEE80211_CHAN_RADAR)
		LOGBUF(buf, ((int32_t)buf_size), buf_written, "RADAR ");
	if (flags & IEEE80211_CHAN_NO_HT40PLUS)
		LOGBUF(buf, ((int32_t)buf_size), buf_written, "NO_HT40PLUS ");
	if (flags & IEEE80211_CHAN_NO_HT40MINUS)
		LOGBUF(buf, ((int32_t)buf_size), buf_written, "NO_HT40MINUS ");
	if (flags & IEEE80211_CHAN_NO_80MHZ)
		LOGBUF(buf, ((int32_t)buf_size), buf_written, "NO_80MHZ ");
	if (flags & IEEE80211_CHAN_NO_160MHZ)
		LOGBUF(buf, ((int32_t)buf_size), buf_written, "NO_160MHZ ");
}

uint8_t kalGetChannelCount(struct GLUE_INFO *prGlueInfo)
{
	uint8_t channel_count = 0;
	struct wiphy *pWiphy;

	pWiphy = wlanGetWiphy();
	if (pWiphy->bands[KAL_BAND_2GHZ] != NULL) {
		channel_count +=
			pWiphy->bands[KAL_BAND_2GHZ]->n_channels;
	}

	if (pWiphy->bands[KAL_BAND_5GHZ] != NULL) {
		channel_count +=
			pWiphy->bands[KAL_BAND_5GHZ]->n_channels;
	}

#if (CFG_SUPPORT_WIFI_6G == 1)
	if (pWiphy->bands[KAL_BAND_6GHZ] != NULL) {
		channel_count +=
			pWiphy->bands[KAL_BAND_6GHZ]->n_channels;
	}
#endif
	if (channel_count == 0)
		DBGLOG(RLM, ERROR, "invalid channel count.\n");

	return channel_count;
}

#if (CFG_SUPPORT_SINGLE_SKU == 1)
u_int8_t kalFillChannels(
	struct GLUE_INFO *prGlueInfo,
	struct CMD_DOMAIN_CHANNEL *pChBase,
	uint8_t ucChSize,
	uint8_t ucOpChannelNum,
	u_int8_t fgDisconnectUponInvalidOpChannel
)
{
	uint32_t band_idx, ch_idx;
	struct ieee80211_supported_band *sband;
	struct ieee80211_channel *chan;
	uint32_t ch_count;
	struct wiphy *pWiphy;
	struct CMD_DOMAIN_CHANNEL *pCh;
	bool fgRet = false;
	char chan_flag_string[64] = {0};

	if (!prGlueInfo) {
		DBGLOG(RLM, ERROR, "prGlueInfo = NULL.\n");
		return false;
	}

	pWiphy = wlanGetWiphy();
	if (!pWiphy) {
		DBGLOG(RLM, ERROR, "ERROR. pWiphy = NULL.\n");
		return false;
	}

	/*
	 * Ready to parse the channel for bands
	 */
	ch_count = 0;
	for (band_idx = 0; band_idx < KAL_NUM_BANDS; band_idx++) {
		sband = pWiphy->bands[band_idx];
		if (!sband)
			continue;

		for (ch_idx = 0; ch_idx < sband->n_channels; ch_idx++) {
			chan = &sband->channels[ch_idx];
			pCh = pChBase + ch_count;
			/* Parse flags and get readable string */
			kalMemZero(chan_flag_string, sizeof(chan_flag_string));
			kalChannelFlagString(chan->flags,
						   chan_flag_string,
						   sizeof(chan_flag_string));

			if (chan->flags & IEEE80211_CHAN_DISABLED) {
				DBGLOG(RLM, INFO,
				"channels[%d][%d]: ch%d (freq = %d) flags=0x%x [ %s]\n",
				band_idx, ch_idx, chan->hw_value,
				chan->center_freq, chan->flags,
				chan_flag_string);

				/* Disconnect AP in the end of this function*/
				if (fgDisconnectUponInvalidOpChannel
						== true) {
					if (chan->hw_value
							== ucOpChannelNum)
						fgRet = true;
				}

				continue;
			}

			/* Allowable channel */
			if (ch_count >= ucChSize) {
				DBGLOG(RLM, ERROR,
				       "%s(): no buffer to store channel information.\n",
				       __func__);
				break;
			}

			rlmDomainAddActiveChannel(band_idx);

			DBGLOG(RLM, INFO,
			       "channels[%d][%d]: ch%d (freq = %d) flgs=0x%x [%s]\n",
				band_idx, ch_idx, chan->hw_value,
				chan->center_freq, chan->flags,
				chan_flag_string);

			pCh->u2ChNum = chan->hw_value;
			pCh->eFlags = chan->flags;

			ch_count += 1;
		}
	}

	return fgRet;
}
#endif

void *kalGetNetDevPriv(void *prNet)
{
	void *prPriv = NULL;
	struct net_device *prNetDev;

	prNetDev = (struct net_device *)prNet;

	if (prNetDev) {
		prPriv =
			netdev_priv(prNetDev);
	}

	return prPriv;
}

uint32_t kalGetNetDevRxPacket(void *prNet)
{
	struct net_device *prNetDev = NULL;
	uint32_t u4Num = 0;

	prNetDev = (struct net_device *)prNet;
	if (prNetDev)
		u4Num = prNetDev->stats.rx_packets;

	return u4Num;
}

#if CFG_SUPPORT_TDLS

void kalTdlsOpReq(
	struct GLUE_INFO *prGlueInfo,
	uint8_t ucBssIndex,
	uint8_t *aucMacAddr,
	uint16_t eOpMode,
	uint16_t u2ReasonCode
	)
{
	enum nl80211_tdls_operation oper;
	struct net_device *prDev = NULL;
	u_int8_t fgIsOpVaild = FALSE;

	if (!prGlueInfo) {
		DBGLOG(TDLS, ERROR, "GlueInfo NULL.\n");
		return;
	}
	prDev = wlanGetNetDev(prGlueInfo, ucBssIndex);

	if (!prDev)
		return;

	oper = (enum nl80211_tdls_operation)eOpMode;
	switch (oper) {
	case NL80211_TDLS_DISCOVERY_REQ:
	case NL80211_TDLS_SETUP:
	case NL80211_TDLS_TEARDOWN:
	case NL80211_TDLS_ENABLE_LINK:
	case NL80211_TDLS_DISABLE_LINK:
		fgIsOpVaild = TRUE;
		break;
	default:
		fgIsOpVaild = FALSE;
		break;
	}

	if (fgIsOpVaild)
		cfg80211_tdls_oper_request(prDev,
					aucMacAddr,
					oper,
					u2ReasonCode,
					GFP_ATOMIC);

}
#endif

#if defined(_HIF_PCIE)
void kalSetPcieKeepWakeup(struct GLUE_INFO *prGlueInfo,
	u_int8_t fgKeepPcieWakeup)
{
#if CFG_SUPPORT_PCIE_ASPM
	struct ADAPTER *prAdapter;
	struct BUS_INFO *prBusInfo = NULL;

	prAdapter = prGlueInfo->prAdapter;
	prBusInfo = prAdapter->chip_info->bus_info;
	if (prBusInfo->keepPcieWakeup != NULL)
		prBusInfo->keepPcieWakeup(prGlueInfo, fgKeepPcieWakeup);
#endif /* CFG_SUPPORT_PCIE_ASPM */
}

void kalConfigWfdmaTh(struct GLUE_INFO *prGlueInfo, uint32_t u4Th)
{
	struct ADAPTER *prAdapter;
	struct BUS_INFO *prBusInfo = NULL;

	prAdapter = prGlueInfo->prAdapter;
	prBusInfo = prAdapter->chip_info->bus_info;

	if (prBusInfo->u4WfdmaTh != u4Th) {
		prBusInfo->fgUpdateWfdmaTh = TRUE;
		prBusInfo->u4WfdmaTh = u4Th;
	}
}
#endif /* defined(_HIF_PCIE) */

#if defined(_HIF_PCIE) || defined(_HIF_AXI)
void kalSetISRMask(struct ADAPTER *prAdapter, uint32_t set_mask)
{
#define CPU_ALL_CORE (0xff)
	struct cpumask cpu_mask;
	int i;
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct GL_HIF_INFO *prHifInfo = NULL;

	if (!HAL_IS_RX_DIRECT(prAdapter))
		return;

	if (!prGlueInfo)
		return;

	prHifInfo = &prGlueInfo->rHifInfo;

	if (set_mask == CPU_ALL_CORE) {
		synchronize_irq(prHifInfo->u4IrqId);
		irq_set_affinity_hint(prHifInfo->u4IrqId,
			cpu_all_mask);
	} else {
		cpumask_clear(&cpu_mask);
		for (i = 0; i < num_possible_cpus(); i++)
			if ((0x1 << i) & set_mask)
				cpumask_or(&cpu_mask, &cpu_mask,
					cpumask_of(i));
		synchronize_irq(prHifInfo->u4IrqId);
		irq_set_affinity_hint(prHifInfo->u4IrqId,
			&cpu_mask);
	}

	DBGLOG(INIT, INFO, "irq_set_affinity_hint(%u, %u)",
		prHifInfo->u4IrqId, set_mask);
#undef CPU_ALL_CORE
}
#endif /* defined(_HIF_PCIE) || defined(_HIF_AXI) */

#if CFG_TCP_IP_CHKSUM_OFFLOAD
void kalConfigChksumOffload(
	struct GLUE_INFO *prGlueInfo, u_int8_t fgEnable)
{
	if (fgEnable) {
		prGlueInfo->prDevHandler->features |=
				(NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM);
	} else {
		prGlueInfo->prDevHandler->features &=
			~(NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM);
	}

	DBGLOG(OID, INFO, "Checksum offload: [%llu]\n",
			prGlueInfo->prDevHandler->features);
}
#endif

void kalConfigWiFiSnappingForceDisable(
	struct GLUE_INFO *prGlueInfo, u_int8_t fgForceDis)
{
	struct ADAPTER *prAdapter = NULL;
	struct mt66xx_chip_info *prChipInfo = NULL;

	prAdapter = prGlueInfo->prAdapter;
	if (prAdapter == NULL) {
		DBGLOG(INIT, ERROR, "prAdapter is NULL.\n");
		return;
	}
	prChipInfo = prAdapter->chip_info;

	if (prChipInfo->fgWifiNappingForceDisable != fgForceDis)
		prChipInfo->fgWifiNappingForceDisable = fgForceDis;
}

#if CFG_SUPPORT_THERMAL_QUERY
#if (KERNEL_VERSION(6, 1, 0) <= LINUX_VERSION_CODE)
static int get_connsys_thermal_temp(struct thermal_zone_device *tz, int *temp)
#else
static int get_connsys_thermal_temp(void *data, int *temp)
#endif
{
#if (KERNEL_VERSION(6, 1, 0) <= LINUX_VERSION_CODE)
	struct thermal_sensor_info *sensor = tz->devdata;
#else
	struct thermal_sensor_info *sensor = data;
#endif
	struct GLUE_INFO *glue = wlanGetGlueInfo();
	struct ADAPTER *ad = NULL;
	struct WIFI_VAR *wifi_var = NULL;
	struct THERMAL_TEMP_DATA temp_data;
	uint32_t status = WLAN_STATUS_SUCCESS;
	u_int8_t fgCache = FALSE;

	/* Acquire Wi-Fi lock first to prevent from the case
	 * that this function is called during Wi-Fi off.
	 */
	wfsys_lock();
	if (!wlanIsDriverReady(glue,
			       WLAN_DRV_READY_CHECK_WLAN_ON)) {
		status = WLAN_STATUS_FAILURE;
		*temp = THERMAL_TEMP_INVALID;
		goto exit;
	}

	ad = glue->prAdapter;
	wifi_var = &ad->rWifiVar;

	if (sensor->last_query_temp <= wifi_var->i4MaxTempLimit &&
	    time_before(jiffies, sensor->updated_period)) {
		status = WLAN_STATUS_SUCCESS;
		*temp = sensor->last_query_temp;
		fgCache = TRUE;
		goto exit;
	}

	kalMemZero(&temp_data, sizeof(temp_data));
	temp_data.eType = sensor->type;
	temp_data.ucIdx = sensor->sendor_idx;
	status = wlanQueryThermalTemp(ad, &temp_data);
	if (status != WLAN_STATUS_SUCCESS) {
		status = WLAN_STATUS_FAILURE;
		*temp = THERMAL_TEMP_INVALID;
		goto exit;
	}

	*temp = temp_data.u4Temperature;
	sensor->updated_period = jiffies +
		wifi_var->u4MinTempQueryTime * HZ / 1000;
	sensor->last_query_temp = *temp;

exit:
	DBGLOG(REQ, TRACE, "idx[%d] type[%d] name[%s], temp[%d][%d]\n",
		sensor->sendor_idx,
		sensor->type,
		sensor->name,
		fgCache,
		*temp);
	wfsys_unlock();
	return 0;
}

#if (KERNEL_VERSION(6, 1, 0) <= LINUX_VERSION_CODE)
static const struct thermal_zone_device_ops wf_thermal_ops = {
	.get_temp = get_connsys_thermal_temp,
};
#else
static const struct thermal_zone_of_device_ops wf_thermal_ops = {
	.get_temp = get_connsys_thermal_temp,
};
#endif

int thermal_cbs_register(struct platform_device *pdev)
{
	struct mt66xx_hif_driver_data *data = NULL;
	struct mt66xx_chip_info *chip_info = NULL;
	struct thermal_info *thermal_info = NULL;
	struct thermal_zone_device *tzdev = NULL;
	uint8_t idx = 0;
	int err = 0;

	if (!pdev) {
		err = -EINVAL;
		goto exit;
	}

	data = get_platform_driver_data();
	chip_info = data->chip_info;
	thermal_info = &chip_info->thermal_info;

	for (idx = 0; idx < thermal_info->sensor_num; idx++) {
		struct thermal_sensor_info *sensor =
			&thermal_info->sensor_info[idx];

#if (KERNEL_VERSION(6, 1, 0) <= LINUX_VERSION_CODE)
		tzdev = devm_thermal_of_zone_register(&pdev->dev,
			idx, sensor, &wf_thermal_ops);
#else
		tzdev = devm_thermal_zone_of_sensor_register(&pdev->dev,
			idx, sensor, &wf_thermal_ops);
#endif
		if (IS_ERR(tzdev)) {
			sensor->tzd = NULL;
			err = PTR_ERR(tzdev);
			DBGLOG(INIT, ERROR,
				"[%d] Failed to register the thermal cbs: %d\n",
				idx, err);
			break;
		}
		sensor->tzd = tzdev;
	}

exit:
	if (err)
		thermal_cbs_unregister(pdev);

	return err;
}

void thermal_cbs_unregister(struct platform_device *pdev)
{
	struct mt66xx_hif_driver_data *data = NULL;
	struct mt66xx_chip_info *chip_info = NULL;
	struct thermal_info *thermal_info = NULL;
	uint8_t idx = 0;

	if (!pdev)
		return;

	data = get_platform_driver_data();
	chip_info = data->chip_info;
	thermal_info = &chip_info->thermal_info;

	for (idx = 0; idx < thermal_info->sensor_num; idx++) {
		struct thermal_sensor_info *sensor =
			&thermal_info->sensor_info[idx];

		if (!sensor->tzd)
			continue;

#if (KERNEL_VERSION(6, 1, 0) <= LINUX_VERSION_CODE)
		devm_thermal_of_zone_unregister(&pdev->dev,
			sensor->tzd);
#else
		devm_thermal_zone_of_sensor_unregister(&pdev->dev,
			sensor->tzd);
#endif
		sensor->tzd = NULL;
	}
}

void thermal_state_reset(struct ADAPTER *ad)
{
	struct mt66xx_chip_info *chip_info = ad->chip_info;
	struct thermal_info *thermal_info = &chip_info->thermal_info;
	uint8_t idx = 0;

	for (idx = 0; idx < thermal_info->sensor_num; idx++) {
		struct thermal_sensor_info *sensor =
			&thermal_info->sensor_info[idx];

		if (!sensor->tzd)
			continue;

		sensor->updated_period = 0;
		sensor->last_query_temp = 0;
	}
}
#endif

#if defined(_HIF_USB)
void kalAcquiretHifStateLock(struct GLUE_INFO *prGlueInfo,
		unsigned long *plFlags)

{
	unsigned long ulFlags = 0;
	struct GL_HIF_INFO *prHifInfo;

	*plFlags = 0;

	if (prGlueInfo == NULL) {
		DBGLOG(INIT, ERROR, "prGlueInfo is NULL\n");
		return;
	}

	prHifInfo = &prGlueInfo->rHifInfo;

	if (prHifInfo == NULL) {
		DBGLOG(INIT, ERROR, "prHifInfo is NULL\n");
		return;
	}

	spin_lock_irqsave(&prHifInfo->rStateLock, ulFlags);
	*plFlags = ulFlags;
}

void kalReleaseHifStateLock(struct GLUE_INFO *prGlueInfo,
		unsigned long ulFlags)
{
	struct GL_HIF_INFO *prHifInfo;

	if (prGlueInfo == NULL) {
		DBGLOG(INIT, ERROR, "prGlueInfo is NULL\n");
		return;
	}

	prHifInfo = &prGlueInfo->rHifInfo;

	if (prHifInfo == NULL) {
		DBGLOG(INIT, ERROR, "prHifInfo is NULL\n");
		return;
	}

	spin_unlock_irqrestore(
		&prHifInfo->rStateLock,
		ulFlags);
}
#endif

void kalWlanHardStartXmit(void *pvPacket, void *pvDev)
{
	struct sk_buff *skb;
	struct net_device *dev;

	skb = (struct sk_buff *)pvPacket;
	dev = (struct net_device *)pvDev;

	wlanHardStartXmit(skb, dev);
}
int32_t kalNlaPut(void *pvPacket, uint32_t attrType,
		uint32_t attrLen, const void *data)
{
	struct sk_buff *skb = (struct sk_buff *)pvPacket;

	return nla_put(skb, attrType, attrLen, data);
}

void *
kalProcessRttReportDone(struct GLUE_INFO *prGlueInfo,
		uint32_t u4DataLen, uint32_t u4Count)
{
	struct wiphy *wiphy;
	struct wireless_dev *wdev = prGlueInfo->prDevHandler->ieee80211_ptr;
	struct sk_buff *skb;

	wiphy = wlanGetWiphy();
	if (!wiphy) {
		log_dbg(REQ, ERROR, "wiphy is null\n");
		return NULL;
	}
	skb = kalCfg80211VendorEventAlloc(wiphy, wdev,
				  sizeof(u32) + u4DataLen,
				  RTT_EVENT_COMPLETE, GFP_KERNEL);
	if (!skb) {
		DBGLOG(RTT, ERROR, "%s allocate skb failed\n", __func__);
		return NULL;
	}

	if (unlikely(nla_put_u32(skb, RTT_ATTRIBUTE_RESULT_CNT, u4Count) < 0)) {
		DBGLOG(RTT, ERROR, "put cnt fail");
		return NULL;
	}

	return (void *) skb;
}

void *kalGetGlueNetDevHdl(struct GLUE_INFO *prGlueInfo)
{
	return (void *)(prGlueInfo->prDevHandler);
}

struct device *kalGetGlueDevHdl(struct GLUE_INFO *prGlueInfo)
{
	return prGlueInfo->prDev;
}

void kalGetPlatDev(struct platform_device **pdev)
{
	struct mt66xx_chip_info *prChipInfo = NULL;

	glGetChipInfo((void **)&prChipInfo);
	if (prChipInfo)
		*pdev = (struct platform_device *)prChipInfo->platform_device;
	else
		*pdev = NULL;
}

void kalGetDev(void **dev)
{
	struct platform_device *prPlatDev;

	kalGetPlatDev(&prPlatDev);
	if (prPlatDev)
		*dev = &prPlatDev->dev;
	else
		*dev = NULL;
}

void kalClearGlueScanReq(struct GLUE_INFO *prGlueInfo)
{
	prGlueInfo->prScanRequest = NULL;
}

void *kalGetGlueScanReq(struct GLUE_INFO *prGlueInfo)
{
	return (void *)(prGlueInfo->prScanRequest);
}

void *kalGetGlueSchedScanReq(struct GLUE_INFO *prGlueInfo)
{
	return (void *)(prGlueInfo->prSchedScanRequest);
}

void kalClearGlueSchedScanReq(struct GLUE_INFO *prGlueInfo)
{
	kalMemFree(prGlueInfo->prSchedScanRequest->pucIE,
		   VIR_MEM_TYPE,
		   prGlueInfo->prSchedScanRequest->u4IELength);
	kalMemFree(prGlueInfo->prSchedScanRequest,
		   VIR_MEM_TYPE,
		   sizeof(struct PARAM_SCHED_SCAN_REQUEST));
	prGlueInfo->prSchedScanRequest = NULL;
}

void kalGetFtIeParam(void *pvftie,
	uint16_t *pu2MDID, uint32_t *pu4IeLength,
	const uint8_t **pucIe)
{
	struct cfg80211_update_ft_ies_params *ftie = NULL;

	ftie = (struct cfg80211_update_ft_ies_params *)pvftie;
	*pu2MDID = ftie->md;
	*pu4IeLength = ftie->ie_len;
	*pucIe = ftie->ie;
}

int kalRegulatoryHint(char *country)
{
	struct wiphy *pWiphy;

	pWiphy = wlanGetWiphy();
	return regulatory_hint(pWiphy, country);
}

#if (CFG_VOLT_INFO == 1)
/*----------------------------------------------------------------------------*/
/*!
 * \brief Get Volt info enable status, default value may different by platform,
 *        So the function is declare as __weak attibute and will implement in
 *        per platform's plat_priv.c
 *
 * \param[in] : void
 *
 * \return value : Volt info enable or not
 */
/*----------------------------------------------------------------------------*/
uint8_t __weak kalVnfGetEnInitStatus(void)
{
	return FEATURE_DISABLED;
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief Get Volt info low bound, default value may different by platform,
 *        So the function is declare as __weak attibute and will implement in
 *        per platform's plat_priv.c
 *
 * \param[in] : void
 *
 * \return value : voltage low bound
 */
/*----------------------------------------------------------------------------*/
uint32_t __weak kalVnfGetVoltLowBnd(void)
{
	return VOLT_INFO_LOW_BOUND_UNLMT;
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief Reset volt info debounce parameter
 *
 * \param[in] prVnfInfo : Pointer of _rVnfInfo
 *
 * \return value : void
 */
/*----------------------------------------------------------------------------*/
static void kalVnfRstDebParam(struct VOLT_INFO_T *prVnfInfo)
{
	prVnfInfo->rDebParam.u4Total = 0;
	prVnfInfo->rDebParam.u4Cnt = 0;
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief Start schedule volt info work
 *
 * \param[in] prVnfInfo : Pointer of _rVnfInfo
 *
 * \return value : void
 */
/*----------------------------------------------------------------------------*/
static void kalVnfSchedule(struct VOLT_INFO_T *prVnfInfo)
{
	if (prVnfInfo == NULL) {
		DBGLOG(SW4, ERROR, "prVnfInfo is NULL\n");
		return;
	}

	kalVnfRstDebParam(prVnfInfo);
	prVnfInfo->eState = VOLT_INFO_STATE_IN_PROGRESS;

	/* volt info entry */
	schedule_delayed_work(&(prVnfInfo->dwork), 0);
	DBGLOG(SW4, INFO, "VOLT_INFO Schedule, state[%d]\n", prVnfInfo->eState);
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief When host receive FW event, if volt info work is in progress, it will
 *        cancel the work, and re-trigger a new volt info work
 *
 * \param[in] prAdapter
 *
 * \return value : void
 */
/*----------------------------------------------------------------------------*/
void kalVnfEventHandler(struct ADAPTER *prAdapter)
{
	if (prAdapter == NULL) {
		DBGLOG(SW4, ERROR, "prAdapter is NULL\n");
		return;
	}

	/* To avoid concurrent access for _rVnfInfo
	 * between battery notify callback thread
	 * and FW get voltage info event handler on
	 * Wifi driver main thread.
	 */
	mutex_lock(&_rVnfInfo.rMutex);
	if (prAdapter->rWifiVar.fgVnfEn) {
		if (_rVnfInfo.eState == VOLT_INFO_STATE_IN_PROGRESS)
			cancel_delayed_work(&_rVnfInfo.dwork);

		kalVnfSchedule(&_rVnfInfo);
	}
	mutex_unlock(&_rVnfInfo.rMutex);
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief Calculate battery notify upper & lower threshold
 *
 *
 * \param[in] u4Base : voltage base
 * \param[in] u4Delta : voltage delta
 * \param[out] pu4UprThrd : voltage upper threshold
 * \param[out] pu4LwrThrd : voltage lower threshold
 *
 * \return value : void
 */
/*----------------------------------------------------------------------------*/
static void kalVnfBatNotifyGetThresh(uint32_t u4Base, uint32_t u4Delta,
	uint32_t *pu4HvThrd, uint32_t *pu4LvThrd)
{
	if (pu4HvThrd == NULL || pu4LvThrd == NULL)
		return;

	*pu4HvThrd = u4Base + u4Delta;
	*pu4LvThrd = u4Base - u4Delta;

	/* sanity check voltage boundary*/
	if (*pu4HvThrd >= VOLT_INFO_MAX_VOLT_THRESH)
		*pu4HvThrd = VOLT_INFO_MAX_VOLT_THRESH - 1;
	else if (*pu4LvThrd <= VOLT_INFO_MIN_VOLT_THRESH)
		*pu4LvThrd = VOLT_INFO_MIN_VOLT_THRESH + 1;
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief Send volt info to FW by CMD : CMD_ID_SEND_VOLT_INFO
 *
 * \param[in] prVnfInfo : Pointer of _rVnfInfo
 * \param[in] volt : The voltage info which will send to FW
 *
 * \return value : void
 */
/*----------------------------------------------------------------------------*/
static void kalVnfSendCmd(struct VOLT_INFO_T *prVnfInfo, unsigned int u4volt)
{
	struct CMD_SEND_VOLT_INFO_T rVnf;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	if (prVnfInfo == NULL || prVnfInfo->prAdapter == NULL) {
		DBGLOG(SW4, ERROR,
			"prVnfInfo or prVnfInfo->prAdapter is NULL\n");
		return;
	}

	kalMemZero(&rVnf, sizeof(rVnf));

	/* fill in CMD buffer */
	rVnf.u2Volt = (uint16_t)u4volt;

	rStatus = wlanSendSetQueryCmd(prVnfInfo->prAdapter, /* prAdapter */
		CMD_ID_SEND_VOLT_INFO, /* ucCID */
		TRUE, /* fgSetQuery */
		FALSE, /* fgNeedResp */
		FALSE, /* fgIsOid */
		NULL, /* pfCmdDoneHandler */
		NULL, /* pfCmdTimeoutHandler */
		sizeof(struct CMD_SEND_VOLT_INFO_T), /* u4SetQueryInfoLen */
		(uint8_t *) &rVnf, /* pucInfoBuffer */
		NULL, /* pvSetQueryBuffer */
		0	/* u4SetQueryBufferLen */);

	DBGLOG(SW4, INFO, "Send volt info[%d], status[%s]",
		rVnf.u2Volt,
		rStatus == WLAN_STATUS_PENDING ? "Success" : "Fail");
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief This func is use to check the wlan status
 *        e.g. Wi-Fi already start or stop
 *
 * \param[in] prVnfInfo : Pointer of _rVnfInfo
 *
 * \return value : WLAN_STATUS_SUCCESS
 *                 WLAN_STATUS_FAILURE
 */
/*----------------------------------------------------------------------------*/
static uint32_t kalVnfWlanCheck(struct VOLT_INFO_T *prVnfInfo)
{
	struct ADAPTER *prAdapter =  NULL;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct REG_INFO *prRegInfo = NULL;

	if (prVnfInfo == NULL || prVnfInfo->prAdapter == NULL) {
		DBGLOG(SW4, ERROR,
			"prVnfInfo or prVnfInfo->prAdapter is NULL\n");
		return WLAN_STATUS_FAILURE;
	}
	prAdapter = prVnfInfo->prAdapter;

	prGlueInfo = prAdapter->prGlueInfo;
	if (prGlueInfo == NULL) {
		DBGLOG(NIC, ERROR, "prGlueInfo is NULL");
		return WLAN_STATUS_FAILURE;
	}

	prRegInfo = &prGlueInfo->rRegInfo;
	if (prRegInfo == NULL ||
		prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(NIC, ERROR,
			"wlan not start");
		return WLAN_STATUS_FAILURE;
	}

	if (test_bit(GLUE_FLAG_HALT_BIT, &prGlueInfo->ulFlag)) {
		DBGLOG(NIC, ERROR, "Wi-Fi is stopped");
		return WLAN_STATUS_FAILURE;
	}

	return WLAN_STATUS_SUCCESS;
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief This func is use to for trigger volt info work schedule by battery
 *        notify
 *
 * \param[in] volt : parameter from schedule_delayed_work
 *
 * \return value : void
 */
/*----------------------------------------------------------------------------*/
static void kalVnfBatSchedule(struct work_struct *work)
{
	struct ADAPTER *prAdapter =  NULL;

	if (_rVnfInfo.prAdapter == NULL) {
		DBGLOG(SW4, ERROR,
			"_rVnfInfo->prAdapter is NULL\n");
		return;
	}
	prAdapter = _rVnfInfo.prAdapter;

	/* To avoid concurrent access for _rVnfInfo
	 * between battery notify callback thread
	 * and FW get voltage info event handler on
	 * Wifi driver main thread.
	 */
	mutex_lock(&_rVnfInfo.rMutex);

	/* If there is volt info handler in process,
	 * skip to trigger volt info work this time,
	 * Althought the volt will be inaccurate,
	 * but expected volt info work will re-trigger again soon
	 */
	if (prAdapter->rWifiVar.fgVnfEn &&
			_rVnfInfo.eState != VOLT_INFO_STATE_IN_PROGRESS) {
		DBGLOG(SW4, INFO, "VOLT_INFO Battery Schedule\n");
		kalVnfSchedule(&_rVnfInfo);
	} else {
		DBGLOG(SW4, INFO,
			"Skip volt info work, En[%d]State[%d]\n",
			prAdapter->rWifiVar.fgVnfEn,
			_rVnfInfo.eState);
	}
	mutex_unlock(&_rVnfInfo.rMutex);
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief This func is use to send volt info CMD & calculate new upper & lower
 *        battery notify threshold
 *
 * \param[in] volt : battery notify trigger callback volt
 *
 * \return value : void
 */
/*----------------------------------------------------------------------------*/
static void kalVnfBatNotifyCb(unsigned int volt)
{
	struct ADAPTER *prAdapter =  NULL;

	if (_rVnfInfo.prAdapter == NULL) {
		DBGLOG(SW4, ERROR,
			"volt = %d, _rVnfInfo.prAdapter is NULL", volt);
		return;
	}
	prAdapter = _rVnfInfo.prAdapter;

	if (kalVnfWlanCheck(&_rVnfInfo) != WLAN_STATUS_SUCCESS) {
		DBGLOG(SW4, ERROR,
			"volt = %d, wifi is not in normal operate state", volt);
		return;
	}

	/* To decouple Vnf & lbat lock dependency,
	 * We schedule a delay work for to determine whether need to trigger
	 * volt info work schedule
	 */
	DBGLOG(SW4, INFO,
		"volt[%d], Battery notifier try schedule Volt Info\n",
		volt);
	schedule_delayed_work(&_rVnfInfo.dBatWork, 0);
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief Get volt info debounce interval
 *
 * \param[in] prVnfInfo : Pointer of _rVnfInfo
 *
 * \return value : prVnfInfo->prAdapter->rWifiVar.u4VnfDebInterval
 */
/*----------------------------------------------------------------------------*/
static uint32_t kalVnfGetDebInterval(struct VOLT_INFO_T *prVnfInfo)
{
	return prVnfInfo->prAdapter->rWifiVar.u4VnfDebInterval;
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief Get volt info debounce times
 *        we also set a upper bounce for debounce tims, to make sure the when
 *        doing volt debounce work, variable use to store total volt will not
 *        be overflow
 *
 * \param[in] prVnfInfo : Pointer of _rVnfInfo
 *
 * \return value : u4DebTimes
 */
/*----------------------------------------------------------------------------*/
static uint32_t kalVnfGetDebTimes(struct VOLT_INFO_T *prVnfInfo)
{
	struct ADAPTER *prAdapter = prVnfInfo->prAdapter;
	uint32_t u4DebTimes = 0;

	if (prAdapter->rWifiVar.u4VnfDebTimes > VOLT_INFO_DEBOUNCE_TIMES_MAX)
		u4DebTimes = VOLT_INFO_DEBOUNCE_TIMES_MAX;
	else
		u4DebTimes = prAdapter->rWifiVar.u4VnfDebTimes;

	return u4DebTimes;
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief Set volt info notifier debounce prameter
 *
 * \param[in] prVnfInfo : Pointer of _rVnfInfo
 * \param[in] lbat_pt   : Pointer of battery notify info
 *
 * \return value : void
 */
/*----------------------------------------------------------------------------*/
static void kalVnfBatNotifySetDebParam(
	struct VOLT_INFO_T *prVnfInfo, struct lbat_user *lbat_pt)
{
	uint32_t u4HvDebTimes = 0;
	uint32_t u4LvDebTimes = 0;
	uint32_t u4HvDebInterval = 0;
	uint32_t u4LvDebInterval = 0;

	u4HvDebTimes = kalVnfGetDebTimes(prVnfInfo);
	u4LvDebTimes = u4HvDebTimes;
	u4HvDebInterval = kalVnfGetDebInterval(prVnfInfo);
	u4LvDebInterval = u4HvDebInterval;

	if (!IS_ERR(lbat_pt)) {
		lbat_user_set_debounce(lbat_pt,
			u4HvDebInterval,
			u4HvDebTimes,
			u4LvDebInterval,
			u4LvDebTimes);

		DBGLOG(SW4, INFO,
			"Volt_Info_Notify set_debounce,[Hv:%d,%d][Lv:%d,%d]\n",
			u4HvDebInterval,
			u4HvDebTimes,
			u4LvDebInterval,
			u4LvDebTimes);
	}
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is use to register volt info notifier.
 *        Only register when the first Wi-Fi on after DUT boot up.
 *
 * \param[in] prVnfInfo : Pointer of _rVnfInfo
 *
 * \return value : void
 */
/*----------------------------------------------------------------------------*/
static void kalVnfBatNotifyReg(struct VOLT_INFO_T *prVnfInfo)
{
	struct ADAPTER *prAdapter = prVnfInfo->prAdapter;
	struct lbat_user *lbat_pt;
	int32_t i4Ret = 0;
	uint32_t u4HvThrd = 0;
	uint32_t u4LvThrd = 0;

	/* Get register voltage Hv & Lv Threshold */
	kalVnfBatNotifyGetThresh(
		prVnfInfo->u4CurrVolt,
		prAdapter->rWifiVar.u4VnfDelta,
		&u4HvThrd, &u4LvThrd);

	/* Register battery notifier */
	lbat_pt = lbat_user_register("Volt_Info_Notify",
				u4HvThrd, u4LvThrd, 2000,
				kalVnfBatNotifyCb);

	if (!IS_ERR(lbat_pt)) {
		kalVnfBatNotifySetDebParam(prVnfInfo, lbat_pt);
		prVnfInfo->rBatNotify.lbat_pt = lbat_pt;

		/* 1. Battery notify will register only when the first Wi-Fi
		 *    on after DUT boot up.
		 * 2. If Battery notify register every time when Wi-Fi on,
		 *    it will result in multiple sets of thesholds at the
		 *    same time in the lbat list, since we don't unregister
		 *    the current threshold in lbat list when Wi-Fi turn off.
		 * 3. The lbat_pt & fgReg will be free when DUT shutdown.
		 */
		prVnfInfo->rBatNotify.fgReg = TRUE; /* Register success */

		DBGLOG(SW4, INFO,
			"Volt_Info_Notify Register SUCCESS, %d,%d,%d\n",
			u4HvThrd, u4LvThrd, 2000);
	} else {
		i4Ret = PTR_ERR(lbat_pt);
		prVnfInfo->rBatNotify.fgReg = FALSE; /* Register fail */

		DBGLOG(SW4, ERROR,
			"Register Volt_Info_Notifier FAIL: %d\n", i4Ret);
	}
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief Modify volt info register threshold
 *
 * \param[in] prVnfInfo : Pointer of _rVnfInfo
 *
 * \return value : void
 */
/*----------------------------------------------------------------------------*/
static void kalVnfBatNotifyModThresh(struct VOLT_INFO_T *prVnfInfo)
{
	struct ADAPTER *prAdapter = prVnfInfo->prAdapter;
	struct lbat_user *lbat_pt = prVnfInfo->rBatNotify.lbat_pt;
	uint32_t u4HvThrd = 0;
	uint32_t u4LvThrd = 0;

	/* Get register volt upper & lower bound */
	kalVnfBatNotifyGetThresh(
		prVnfInfo->u4CurrVolt,
		prAdapter->rWifiVar.u4VnfDelta,
		&u4HvThrd, &u4LvThrd);

	if (!IS_ERR(lbat_pt)) {
		lbat_user_modify_thd_locked(
			lbat_pt,
			u4HvThrd,
			u4LvThrd,
			2000);

		DBGLOG(SW4, INFO,
			"Volt_Info_Notify modify threshold %d,%d,%d\n",
			u4HvThrd,
			u4LvThrd,
			2000);
	}
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief This func is use to read current volt level
 *        The voltage will keep as low bound, if the low bound value have been
 *        set.
 *
 * * \param[in] prVnfInfo : Pointer of _rVnfInfo
 *
 * \return value : volt
 */
/*----------------------------------------------------------------------------*/
static uint32_t kalVnfReadVolt(struct VOLT_INFO_T *prVnfInfo)
{
	uint32_t u4Volt;
	uint32_t u4VoltLowBnd;

	u4VoltLowBnd = prVnfInfo->prAdapter->rWifiVar.u4VnfLowBnd;

	u4Volt = lbat_read_volt();

	/* Keep voltage level as low bound */
	if (u4VoltLowBnd != VOLT_INFO_LOW_BOUND_UNLMT &&
	    u4Volt < u4VoltLowBnd)
		u4Volt = u4VoltLowBnd;

	return u4Volt;
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief This func is use to :
 *        1. Calculate and send stable voltage info to FW which is from Vbat
 *        2. Register volt info nofify or modify volt info notify threshold
 *
 * \param[in] work : parameter from schedule_delayed_work
 *
 * \return value : void
 */
/*----------------------------------------------------------------------------*/
static void kalVnfHandler(struct work_struct *work)
{
	uint32_t u4DebTimes;
	uint32_t u4DebInterval;
	uint32_t u4AvgVolt;

	if (_rVnfInfo.prAdapter == NULL) {
		DBGLOG(SW4, ERROR,
			"_rVnfInfo->prAdapter is NULL\n");
		return;
	}

	/* To avoid concurrent access for _rVnfInfo
	 * between battery notify callback thread
	 * and FW get voltage info event handler on
	 * Wifi driver main thread.
	 */
	mutex_lock(&_rVnfInfo.rMutex);

	u4DebTimes = kalVnfGetDebTimes(&_rVnfInfo);
	u4DebInterval = kalVnfGetDebInterval(&_rVnfInfo);

	/* Calculate Avg volt */
	_rVnfInfo.rDebParam.u4Total += kalVnfReadVolt(&_rVnfInfo);
	_rVnfInfo.rDebParam.u4Cnt++;

	if (_rVnfInfo.rDebParam.u4Cnt < u4DebTimes) {
		schedule_delayed_work(&_rVnfInfo.dwork,
			msecs_to_jiffies(u4DebInterval));
		mutex_unlock(&_rVnfInfo.rMutex);
		return;
	}
	u4AvgVolt = _rVnfInfo.rDebParam.u4Total / _rVnfInfo.rDebParam.u4Cnt;

	DBGLOG(SW4, INFO,
		"Avg Volt[%d]DebTimes[%d]DebInterval[%d]",
		u4AvgVolt,
		u4DebTimes,
		u4DebInterval);

	kalVnfSendCmd(&_rVnfInfo, u4AvgVolt);
	_rVnfInfo.u4CurrVolt = u4AvgVolt;

	/* Bat notify register or modify threshold */
	if (_rVnfInfo.rBatNotify.fgReg == FALSE)
		kalVnfBatNotifyReg(&_rVnfInfo);
	else
		kalVnfBatNotifyModThresh(&_rVnfInfo);

	_rVnfInfo.eState = VOLT_INFO_STATE_COMPLETE;
	DBGLOG(SW4, INFO, "Volt_Info work complete, state[%d]",
		_rVnfInfo.eState);
	mutex_unlock(&_rVnfInfo.rMutex);
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief Active volt info work
 *
 * \param[in] prAdapter
 *
 * \return value : void
 */
/*----------------------------------------------------------------------------*/
void kalVnfActive(struct ADAPTER *prAdapter)
{
	if (prAdapter == NULL) {
		DBGLOG(SW4, ERROR, "prAdapter is NULL");
		return;
	}

	/* To avoid concurrent access for _rVnfInfo
	 * between battery notify callback thread
	 * and FW get voltage info event handler on
	 * Wifi driver main thread.
	 */
	mutex_lock(&_rVnfInfo.rMutex);

	/* If there is volt info handler in process,
	 * skip to trigger volt info work this time,
	 * Althought the volt will be inaccurate,
	 * but expected volt info work will re-trigger again soon
	 */
	if (prAdapter->rWifiVar.fgVnfEn &&
			_rVnfInfo.eState != VOLT_INFO_STATE_IN_PROGRESS) {
		DBGLOG(SW4, INFO, "VOLT_INFO Active\n");
		kalVnfSchedule(&_rVnfInfo);
	} else {
		DBGLOG(SW4, INFO,
			"Skip volt info work, En[%d]State[%d]\n",
			prAdapter->rWifiVar.fgVnfEn,
			_rVnfInfo.eState);
	}
	mutex_unlock(&_rVnfInfo.rMutex);
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief Reset all volt info parameter, except BatNotify parameter.
 *        Battery notify will register only when the first Wi-Fi on
 *        after DUT boot up.
 *        If Battery notify register every time when Wi-Fi on, it will
 *        result in multiple sets of thesholds at the same time in the
 *        lbat list, since we don't unregister the current threshold in
 *        lbat list when Wi-Fi turn off.
 *
 * \param[in] prVnfInfo : Pointer of _rVnfInfo
 *
 * \return value : void
 */
/*----------------------------------------------------------------------------*/
static void kalVnfRstParam(struct VOLT_INFO_T *prVnfInfo)
{
	if (prVnfInfo == NULL) {
		DBGLOG(SW4, ERROR, "prVnfInfo is NULL");
		return;
	}

	prVnfInfo->u4CurrVolt = 0;
	kalVnfRstDebParam(prVnfInfo);
	prVnfInfo->prAdapter = NULL;
	prVnfInfo->eState = VOLT_INFO_STATE_INIT;
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief Uninit Volt info work
 *
 * \param[in] void
 *
 * \return value : void
 */
/*----------------------------------------------------------------------------*/
void kalVnfUninit(void)
{
	uint8_t ret_BatWork;
	uint8_t ret_dwork;

	ret_BatWork = cancel_delayed_work_sync(&_rVnfInfo.dBatWork);
	ret_dwork = cancel_delayed_work_sync(&_rVnfInfo.dwork);
	kalVnfRstParam(&_rVnfInfo);
	DBGLOG(SW4, TRACE, "VOLT_INFO Uninit,ret_BatWork[%d],ret_dwork[%d]\n",
		ret_BatWork,
		ret_dwork);
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief Init volt info work
 *
 * \param[in] prAdapter
 *
 * \return value : void
 */
/*----------------------------------------------------------------------------*/
void kalVnfInit(struct ADAPTER *prAdapter)
{
	if (prAdapter == NULL) {
		DBGLOG(SW4, ERROR, "prAdapter is NULL\n");
		return;
	}

	kalVnfRstParam(&_rVnfInfo);
	_rVnfInfo.prAdapter = prAdapter;
	INIT_DELAYED_WORK(&_rVnfInfo.dwork, kalVnfHandler);
	INIT_DELAYED_WORK(&_rVnfInfo.dBatWork, kalVnfBatSchedule);
	mutex_init(&_rVnfInfo.rMutex);
	DBGLOG(SW4, INFO, "VOLT_INFO init\n");
}
#endif /* CFG_VOLT_INFO */

void kalTxDirectInit(struct GLUE_INFO *prGlueInfo)
{
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;

	if (!prAdapter) {
		DBGLOG(INIT, INFO, "prAdapter is NULL\n");
		return;
	}

	if (HAL_IS_TX_DIRECT(prAdapter)) {
		if (!prAdapter->fgTxDirectInited) {
			kalTxDirectInitSkbQ(prGlueInfo);
#if KERNEL_VERSION(4, 15, 0) <= CFG80211_VERSION_CODE
			timer_setup(&prGlueInfo->rTxDirectSkbTimer,
					kalTxDirectTimerCheckSkbQ, 0);
			timer_setup(&prGlueInfo->rTxDirectHifTimer,
					kalTxDirectTimerCheckHifQ, 0);
#else
			init_timer(&prGlueInfo->rTxDirectSkbTimer);
			prGlueInfo->rTxDirectSkbTimer.data =
					(unsigned long)prGlueInfo;
			prGlueInfo->rTxDirectSkbTimer.function =
					kalTxDirectTimerCheckSkbQ;

			init_timer(&prGlueInfo->rTxDirectHifTimer);
			prGlueInfo->rTxDirectHifTimer.data =
					(unsigned long)prGlueInfo;
			prGlueInfo->rTxDirectHifTimer.function =
				kalTxDirectTimerCheckHifQ;
#endif
			prAdapter->fgTxDirectInited = TRUE;
		}
	}
}

void kalTxDirectUninit(struct GLUE_INFO *prGlueInfo)
{
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;

	if (!prAdapter) {
		DBGLOG(INIT, INFO, "prAdapter is NULL\n");
		return;
	}

	if (HAL_IS_TX_DIRECT(prAdapter)) {
		if (prAdapter->fgTxDirectInited) {
			del_timer_sync(&prGlueInfo->rTxDirectSkbTimer);
			del_timer_sync(&prGlueInfo->rTxDirectHifTimer);
			kalTxDirectClearSkbQ(prGlueInfo);
		}
	}
}

#if (CFG_SUPPORT_SINGLE_SKU == 1)
#if (CFG_SUPPORT_SINGLE_SKU_LOCAL_DB == 1)

#if CFG_SUPPORT_BW320
#define BW_6G 320
#define BW_5G 160
#elif CFG_SUPPORT_BW160
#define BW_5G 160
#define BW_6G 160
#else
#define BW_5G 80
#define BW_6G 80
#endif

#if (CFG_SUPPORT_WIFI_6G == 1)
const struct ieee80211_regdomain default_regdom_ww = {
	.n_reg_rules = 5,
	.alpha2 = "99",
	.reg_rules = {
	/* channels 1..13 */
	REG_RULE_LIGHT(2412-10, 2472+10, 40, 0),
	/* channels 14 */
	REG_RULE_LIGHT(2484-10, 2484+10, 20, 0),
	/* channel 36..64 */
	REG_RULE_LIGHT(5150-10, 5350+10, BW_5G, 0),
	/* channel 100..165 */
	REG_RULE_LIGHT(5470-10, 5850+10, BW_5G, 0),
	/* 6G channel 1..17 */
	REG_RULE_LIGHT(5935-10, 7135+10, BW_6G, 0),
	}
};
#else
const struct ieee80211_regdomain default_regdom_ww = {
	.n_reg_rules = 4,
	.alpha2 = "99",
	.reg_rules = {
	/* channels 1..13 */
	REG_RULE_LIGHT(2412-10, 2472+10, 40, 0),
	/* channels 14 */
	REG_RULE_LIGHT(2484-10, 2484+10, 20, 0),
	/* channel 36..64 */
	REG_RULE_LIGHT(5150-10, 5350+10, BW_5G, 0),
	/* channel 100..165 */
	REG_RULE_LIGHT(5470-10, 5850+10, BW_5G, 0),
	}
};
#endif

const void *kalGetDefaultRegWW(void)
{
	return &default_regdom_ww;
}
#endif

uint8_t kalGetRdmVal(uint8_t dfs_region)
{
	uint8_t retVal = 0;

	switch (dfs_region) {
	case NL80211_DFS_FCC:
		retVal = ENUM_RDM_FCC;
		break;
	case NL80211_DFS_ETSI:
		retVal = ENUM_RDM_CE;
		break;
	case NL80211_DFS_JP:
		retVal = ENUM_RDM_JAP;
		break;
	case NL80211_DFS_UNSET:
		DBGLOG(P2P, ERROR,
			"rlmDomainGetDfsRegion is NL80211_DFS_UNSET!\n");
		break;
	default:
		retVal = rlmDomainGetDfsRegion();
		DBGLOG(P2P, INFO,
			"ucSetVal: %d\n", retVal);
		break;
	}

	if (rlmDomainIsSameCountryCode("KR", 2))
		retVal = ENUM_RDM_KR;

	return retVal;
}

u_int8_t kalIsETSIDfsRegin(void)
{
	return rlmDomainGetDfsRegion() == NL80211_DFS_ETSI ? TRUE : FALSE;
}

#endif

u_int8_t kalIsChFlagMatch(uint32_t uFlags,
	enum CHAN_FLAGS matchFlag)
{
#define IEEE80211_CHAN_NO_HT40 \
		(IEEE80211_CHAN_NO_HT40PLUS | IEEE80211_CHAN_NO_HT40MINUS)

	switch (matchFlag) {
	case CHAN_RADAR:
		return ((uFlags & IEEE80211_CHAN_RADAR)
				== IEEE80211_CHAN_RADAR) ? TRUE : FALSE;
	case CHAN_NO_HT40PLUS:
		return ((uFlags & IEEE80211_CHAN_NO_HT40PLUS)
				== IEEE80211_CHAN_NO_HT40PLUS) ? TRUE : FALSE;
	case CHAN_NO_HT40MINUS:
		return ((uFlags & IEEE80211_CHAN_NO_HT40MINUS)
				== IEEE80211_CHAN_NO_HT40MINUS) ? TRUE : FALSE;
	case CHAN_NO_HT40:
		return ((uFlags & IEEE80211_CHAN_NO_HT40)
				== IEEE80211_CHAN_NO_HT40) ? TRUE : FALSE;
	case CHAN_NO_80MHZ:
		return ((uFlags & IEEE80211_CHAN_NO_80MHZ)
				== IEEE80211_CHAN_NO_80MHZ) ? TRUE : FALSE;
	case CHAN_NO_160MHZ:
		return ((uFlags & IEEE80211_CHAN_NO_160MHZ)
				== IEEE80211_CHAN_NO_160MHZ) ? TRUE : FALSE;
	default:
		return FALSE;

	}
}

#if CFG_SUPPORT_TASKLET_FREE_MSDU
void kalTxFreeMsduTaskSchedule(struct GLUE_INFO *prGlueInfo)
{
	tasklet_schedule(&prGlueInfo->rTxMsduRetTask);
}
#endif /* CFG_SUPPORT_TASKLET_FREE_MSDU */

inline struct GL_WORK *kalGetWork(struct GLUE_INFO *pr,
	enum ENUM_WORK eWork)
{
	return &pr->arGlWork[eWork];
}

inline struct workqueue_struct *__kalWorkQueueInit(uint8_t *WqName)
{
	struct workqueue_struct *prWorkQueue;

	prWorkQueue = create_workqueue(WqName);
	if (!prWorkQueue)
		DBGLOG(INIT, ERROR, "wq[%s] init fail\n", WqName);
	else
		DBGLOG(INIT, TRACE, "wq[%s] init done\n", WqName);

	return prWorkQueue;
}

inline void __kalWorkQueueUninit(struct workqueue_struct *prWq)
{
	if (!prWq)
		return;

	flush_workqueue(prWq);
	destroy_workqueue(prWq);
}

/*
 * If the worker is running, it can only queue again on the same CPU,
 * it may become a issue when we would like to switch to another CPU when
 * boost CPU.
 *
 * So, we introduce multiple worker and you need to protect single entrance
 * in your pfWorkFunc.
 *
 * If ENUM_WORK_FLAG_MULTIWORK is set, multiple worker feature will be enabled.
 *
 */
static void kalWorkInit(struct GLUE_INFO *pr,
	enum ENUM_WORK eWork, uint8_t *WqName, GL_WORK_FUNC pfWorkFunc)
{
	enum ENUM_WORK_INDEX eIdx;
	struct GL_WORK *prWork;
	struct WORK_CONTAINER *prWorkContainer;

	if (!pr)
		return;

	if (!WqName || !pfWorkFunc)
		return;

	prWork = kalGetWork(pr, eWork);

	/* init cpu idx as free run */
	prWork->i4WorkCpu = -1;
	prWork->sWorkQueueName = WqName;
	WORK_SET_FLAG(prWork, ENUM_WORK_FLAG_NONE);
	prWork->eWorkIdx = WORKER_0;

	for (eIdx = WORKER_0; eIdx < WORKER_MAX; eIdx++) {
		prWorkContainer = &prWork->rWorkContainer[eIdx];
		prWorkContainer->pr = pr;
		prWorkContainer->eWork = eWork;
		prWorkContainer->eIdx = eIdx;
		INIT_WORK(&prWorkContainer->rWork, pfWorkFunc);
	}

	prWork->prWorkQueue = __kalWorkQueueInit(WqName);
}

static void kalWorkSetFlag(struct GLUE_INFO *pr,
	enum ENUM_WORK eWork, enum ENUM_WORK_FLAG eWorkFlag)
{
	struct GL_WORK *prWork;

	if (!pr)
		return;

	prWork = kalGetWork(pr, eWork);
	WORK_SET_FLAG(prWork, eWorkFlag);
}

static void kalWorkUninit(struct GLUE_INFO *pr,
	enum ENUM_WORK eWork)
{
	struct GL_WORK *prWork;
	struct workqueue_struct *prWq;

	if (!pr)
		return;

	prWork = kalGetWork(pr, eWork);

	prWq = prWork->prWorkQueue;
	prWork->prWorkQueue = NULL;
	__kalWorkQueueUninit(prWq);

	DBGLOG(INIT, TRACE, "%s uninit done\n", prWork->sWorkQueueName);
}

static void kalWorkSetCpu(struct GLUE_INFO *pr,
	enum ENUM_WORK eWork, int32_t i4CpuIdx)
{
	struct GL_WORK *prWork;

	if (!pr)
		return;

	prWork = kalGetWork(pr, eWork);
	if ((i4CpuIdx != -1) &&
		i4CpuIdx > num_possible_cpus()) {
		DBGLOG(INIT, ERROR,
			"%s Invalid CpuIdx:%d\n",
			prWork->sWorkQueueName, i4CpuIdx);
		return;
	}

	if (prWork->i4WorkCpu == i4CpuIdx)
		goto end;

	prWork->i4WorkCpu = i4CpuIdx;
	if (WORK_IS_FLAG(prWork, ENUM_WORK_FLAG_MULTIWORK)) {
		if (prWork->eWorkIdx == WORKER_0)
			prWork->eWorkIdx = WORKER_1;
		else
			prWork->eWorkIdx = WORKER_0;
	}

end:
	DBGLOG(INIT, INFO, "%s => %d",
		prWork->sWorkQueueName, prWork->i4WorkCpu);
}

static int32_t kalWorkGetCpu(struct GLUE_INFO *pr,
	enum ENUM_WORK eWork)
{
	struct GL_WORK *prWork;

	if (!pr)
		return -1;

	prWork = kalGetWork(pr, eWork);
	return prWork->i4WorkCpu;
}

static uint32_t kalWorkSchedule(struct GLUE_INFO *pr,
	enum ENUM_WORK eWork)
{
	struct GL_WORK *prWork;
	struct WORK_CONTAINER *prWorkContainer;
	int32_t i4Cpu;

	if (!pr)
		goto end;

	prWork = kalGetWork(pr, eWork);
	if (WORK_IS_FLAG(prWork, ENUM_WORK_FLAG_MULTIWORK)
		&& prWork->eWorkIdx != WORKER_0)
		prWorkContainer = &prWork->rWorkContainer[WORKER_1];
	else
		prWorkContainer = &prWork->rWorkContainer[WORKER_0];

	if (!prWork->prWorkQueue) {
		DBGLOG_LIMITED(INIT, ERROR, "Workqueue %s is NULL\n",
			prWork->sWorkQueueName);
		return WLAN_STATUS_NOT_ACCEPTED;
	}

	i4Cpu = prWork->i4WorkCpu;
	if (i4Cpu == -1) {
		queue_work(prWork->prWorkQueue, &prWorkContainer->rWork);
		goto end;
	}

	queue_work_on(i4Cpu, prWork->prWorkQueue, &prWorkContainer->rWork);

end:
	return WLAN_STATUS_SUCCESS;
}

inline struct GLUE_INFO *kalWorkGetGlueInfo(
	struct work_struct *work)
{
	struct GLUE_INFO *pr;
	struct WORK_CONTAINER *prWorkContainer;

	prWorkContainer = CONTAINER_OF(work, struct WORK_CONTAINER, rWork);
	pr = prWorkContainer->pr;
	return pr;
}

inline enum ENUM_WORK_INDEX kalWorkGetIdx(
	struct work_struct *work)
{
	struct WORK_CONTAINER *prWorkContainer;

	prWorkContainer = CONTAINER_OF(work, struct WORK_CONTAINER, rWork);
	return prWorkContainer->eIdx;
}

inline uint32_t kalWorkCheckState(struct work_struct *work)
{
	struct WORK_CONTAINER *prWorkContainer;
	struct GLUE_INFO *pr;
	struct GL_WORK *prWork;

	prWorkContainer = CONTAINER_OF(work, struct WORK_CONTAINER, rWork);
	pr = prWorkContainer->pr;
	prWork = kalGetWork(pr, prWorkContainer->eWork);
	if (prWorkContainer->eIdx != prWork->eWorkIdx) {
		DBGLOG(INIT, INFO, "[%s] eWorkIdx:%u->%u\n",
			prWork->sWorkQueueName, prWorkContainer->eIdx,
			prWork->eWorkIdx);
		return WLAN_STATUS_NOT_ACCEPTED;
	}

	return WLAN_STATUS_SUCCESS;
}

static void kalConWorkRun(struct work_struct *work)
{
	struct CON_WORK *prConWork =
		CONTAINER_OF(work, struct CON_WORK, rWork);
	struct GLUE_INFO *pr = prConWork->pr;
	uint8_t ucIdx;

	if (!pr)
		return;

	ucIdx = prConWork->ucIdx;
	if (unlikely(ucIdx >= CON_WORK_MAX)) {
		DBGLOG(INIT, INFO, "Invalid Idx %u\n", ucIdx);
		return;
	}

#if CFG_ENABLE_WAKE_LOCK
	KAL_WAKE_LOCK(pr->prAdapter, prConWork->wakelock);
#endif /* CFG_ENABLE_WAKE_LOCK */

	if (prConWork->func)
		prConWork->func(pr, ucIdx);

#if CFG_ENABLE_WAKE_LOCK
	KAL_WAKE_UNLOCK(pr->prAdapter, prConWork->wakelock);
#endif /* CFG_ENABLE_WAKE_LOCK */
}

inline void kalConWorkInit(struct GLUE_INFO *pr, struct CON_WORK *prConWork,
	const char *name, uint8_t ucIdx, PFN_CON_WORK_FUNC func)
{
	uint8_t aucBuf[WORKER_NAME_STR_MAX] = {0};

	prConWork->pr = pr;
	prConWork->ucIdx = ucIdx;
	kalSnprintf(aucBuf, sizeof(aucBuf), "%s/%u", name, ucIdx);
	prConWork->func = func;
	INIT_WORK(&prConWork->rWork, kalConWorkRun);
	prConWork->prWorkQueue = __kalWorkQueueInit(aucBuf);

#if CFG_ENABLE_WAKE_LOCK
	KAL_WAKE_LOCK_INIT(pr->prAdapter, prConWork->wakelock, aucBuf);
#endif /* CFG_ENABLE_WAKE_LOCK */
}

inline void kalConWorkUninit(struct GLUE_INFO *pr, struct CON_WORK *prConWork)
{
	struct workqueue_struct *prWq;

	/* set prWorkQueue to NULL to prevent more schedule */
	prWq = prConWork->prWorkQueue;
	prConWork->prWorkQueue = NULL;

	/* cancel all wq */
	__kalWorkQueueUninit(prWq);

#if CFG_ENABLE_WAKE_LOCK
	KAL_WAKE_LOCK_DESTROY(pr->prAdapter, prConWork->wakelock);
#endif /* CFG_ENABLE_WAKE_LOCK */
}

inline void kalConWorkSchedule(struct GLUE_INFO *pr, struct CON_WORK *prConWork,
	enum CPU_CORE_TYPE eCoreType)
{
	int32_t cpu;

	/* skip if workqueue is not inited */
	if (!prConWork->prWorkQueue)
		return;

	if (eCoreType == CPU_CORE_BIG) {
		/* use Big Core */
		cpu = CPU_BIG_CORE_START_IDX + prConWork->ucIdx;
	} else if (eCoreType == CPU_CORE_LITTLE) {
		/* use Little Core */
		cpu = prConWork->ucIdx;
	} else {
		/* use All Core */
		cpu = -1;
	}

	if (cpu != -1) {
		queue_work_on(cpu,
			prConWork->prWorkQueue, &prConWork->rWork);
	} else
		queue_work(prConWork->prWorkQueue, &prConWork->rWork);
}

inline unsigned long kalConWorkDispatchJobs(uint32_t arJobs[],
	uint32_t u4ReqNum)
{
	unsigned long ulMask = 0;
	uint32_t q, r, u4Num;
	uint8_t ucIdx;

	q = u4ReqNum >> CON_WORK_SHIFT;
	r = u4ReqNum & CON_WORK_MASK;

	for (ucIdx = 0; ucIdx < CON_WORK_MAX; ucIdx++) {
		u4Num = q;
		if ((ucIdx + 1) <= r)
			u4Num++;

		/* early break if no need to schedule */
		if (u4Num == 0)
			break;

		arJobs[ucIdx] = u4Num;
		set_bit(ucIdx, &ulMask);
	}

	return ulMask;
}

#if CFG_SUPPORT_TX_FREE_MSDU_WORK
void kalTxFreeMsduWork(struct work_struct *work)
{
	struct GLUE_INFO *pr = kalWorkGetGlueInfo(work);

	if (!pr)
		return;

	TRACE(halWpdmaFreeMsduWork(pr), "TxFreeMsduWork");
}

inline void kalTxFreeMsduWorkInit(struct GLUE_INFO *pr)
{
	kalWorkInit(pr, TX_FREE_MSDU_WORK, "TxFreeMsduWork", kalTxFreeMsduWork);
}

inline void kalTxFreeMsduWorkSetCpu(struct GLUE_INFO *pr,
	int32_t cpu)
{
	kalWorkSetCpu(pr, TX_FREE_MSDU_WORK, cpu);
}

inline void kalTxFreeMsduWorkUninit(struct GLUE_INFO *pr)
{
	kalWorkUninit(pr, TX_FREE_MSDU_WORK);
}

inline void kalTxFreeMsduWorkSchedule(struct GLUE_INFO *pr)
{
	kalWorkSchedule(pr, TX_FREE_MSDU_WORK);
}
#endif /* CFG_SUPPORT_TX_FREE_MSDU_WORK */

#if CFG_SUPPORT_TX_FREE_SKB_WORK
static void kalDoTxFreeSkb(struct GLUE_INFO *pr, uint8_t ucIdx)
{
	struct TX_FREE_INFO *prTxFreeInfo = &pr->rTxFreeInfo;
	struct TX_FREE_QUEUE_INFO *prQueInfo = &prTxFreeInfo->rQueInfo[ucIdx];
	struct QUE rQue;
	struct QUE *prQue = &rQue;
	struct QUE_ENTRY *prQueueEntry;
	void *pvPacket;

	spin_lock_bh(&prQueInfo->lock);
	QUEUE_MOVE_ALL(prQue, &prQueInfo->rQue);
	spin_unlock_bh(&prQueInfo->lock);

	while (QUEUE_IS_NOT_EMPTY(prQue)) {
		QUEUE_REMOVE_HEAD(prQue, prQueueEntry, struct QUE_ENTRY *);
		if (!prQueueEntry)
			break;

		pvPacket = (void *)GLUE_GET_PKT_DESCRIPTOR(prQueueEntry);
		kalSendComplete(pr, pvPacket, WLAN_STATUS_SUCCESS);
	}

#if CFG_SUPPORT_CPU_STAT
	CPU_STAT_INC_CNT(pr, CPU_TX_FREE_SKB_DONE);
#endif /* CFG_SUPPORT_CPU_STAT */
}

void kalTxFreeSkbWorkSetCpu(struct GLUE_INFO *pr, enum CPU_CORE_TYPE eCoreType)
{
	struct TX_FREE_INFO *prTxFreeInfo = &pr->rTxFreeInfo;

	prTxFreeInfo->eCoreType = eCoreType;
}

void kalTxFreeSkbWorkInit(struct GLUE_INFO *pr)
{
	struct TX_FREE_INFO *prTxFreeInfo = &pr->rTxFreeInfo;
	struct CON_WORK *prConWork;
	struct TX_FREE_QUEUE_INFO *prQueInfo;
	uint8_t ucIdx;

	prTxFreeInfo->eCoreType = CPU_CORE_NONE;
	GLUE_SET_REF_CNT(0, prTxFreeInfo->i4QueIdxCnt);

	for (ucIdx = 0; ucIdx < CON_WORK_MAX; ucIdx++) {
		prQueInfo = &prTxFreeInfo->rQueInfo[ucIdx];
		spin_lock_init(&prQueInfo->lock);
		spin_lock_bh(&prQueInfo->lock);
		prQueInfo->u4TotalCnt = 0;
		spin_unlock_bh(&prQueInfo->lock);
		QUEUE_INITIALIZE(&prQueInfo->rQue);

		prConWork = &prTxFreeInfo->rConWork[ucIdx];
		kalConWorkInit(pr, prConWork, "wifi_tx_free_skb", ucIdx,
			kalDoTxFreeSkb);
	}
}

void kalTxFreeSkbWorkUninit(struct GLUE_INFO *pr)
{
	struct TX_FREE_INFO *prTxFreeInfo = &pr->rTxFreeInfo;
	struct CON_WORK *prConWork;
	uint8_t ucIdx;

	for (ucIdx = 0; ucIdx < CON_WORK_MAX; ucIdx++) {
		prConWork = &prTxFreeInfo->rConWork[ucIdx];
		kalConWorkUninit(pr, prConWork);

		/* do last cleaning */
		kalDoTxFreeSkb(pr, ucIdx);
	}
}

inline void kalTxFreeSkbWorkSchedule(struct GLUE_INFO *pr,
	struct TX_FREE_INFO *prTxFreeInfo, uint8_t ucIdx)
{
	struct CON_WORK *prConWork;

	prConWork = &prTxFreeInfo->rConWork[ucIdx];
	kalConWorkSchedule(pr, prConWork, prTxFreeInfo->eCoreType);
}

uint32_t kalTxFreeSkbQueuePrepare(struct GLUE_INFO *pr,
	struct MSDU_INFO *prMsduInfo, struct QUE *prQue, uint8_t *pucIdx)
{
	struct TX_FREE_INFO *prTxFreeInfo = &pr->rTxFreeInfo;
	struct ADAPTER *ad = pr->prAdapter;
	struct WIFI_VAR *prWifiVar = &ad->rWifiVar;

	if (unlikely(IS_FEATURE_DISABLED(prWifiVar->fgTxFreeSkbWorkEn)))
		return WLAN_STATUS_NOT_ACCEPTED;

	if (unlikely(prTxFreeInfo->eCoreType == CPU_CORE_NONE))
		return WLAN_STATUS_NOT_ACCEPTED;

	if (unlikely(prMsduInfo->prPacket == NULL))
		return WLAN_STATUS_NOT_ACCEPTED;

	if (unlikely(prMsduInfo->eSrc != TX_PACKET_OS))
		return WLAN_STATUS_NOT_ACCEPTED;

#if CFG_SUPPORT_MLR
	if (unlikely(prMsduInfo->eFragPos > MSDU_FRAG_POS_FIRST))
		return WLAN_STATUS_NOT_ACCEPTED;
#endif /* CFG_SUPPORT_MLR */

	QUEUE_INSERT_TAIL(prQue,
		GLUE_GET_PKT_QUEUE_ENTRY(prMsduInfo->prPacket));

	prMsduInfo->prPacket = NULL;
	(*pucIdx)++;
	*pucIdx &= (CON_WORK_MAX - 1);

	return WLAN_STATUS_SUCCESS;
}

void kalTxFreeSkbQueueConcat(struct GLUE_INFO *pr, struct QUE *prQue)
{
	struct TX_FREE_INFO *prTxFreeInfo = &pr->rTxFreeInfo;
	int32_t i4QueIdxCnt;
	uint8_t ucIdx;
	struct TX_FREE_QUEUE_INFO *prQueInfo;

	if (QUEUE_LENGTH(prQue) == 0)
		return;

	i4QueIdxCnt = GLUE_INC_REF_CNT(prTxFreeInfo->i4QueIdxCnt);
	/* use last n bit as the queue idx */
	ucIdx = (uint8_t) i4QueIdxCnt & (CON_WORK_MAX - 1);

	prQueInfo = &(prTxFreeInfo->rQueInfo[ucIdx]);

	spin_lock_bh(&prQueInfo->lock);
	prQueInfo->u4TotalCnt += QUEUE_LENGTH(prQue);
	QUEUE_CONCATENATE_QUEUES(&prQueInfo->rQue, prQue);
	spin_unlock_bh(&prQueInfo->lock);

	kalTxFreeSkbWorkSchedule(pr, prTxFreeInfo, ucIdx);
}
#endif /* CFG_SUPPORT_TX_FREE_SKB_WORK */

#if CFG_SUPPORT_RX_NAPI_WORK
void kalRxNapiWork(struct work_struct *work)
{
	struct GLUE_INFO *pr = kalWorkGetGlueInfo(work);
	struct ADAPTER *prAdapter;
	struct RX_CTRL *prRxCtrl;

	if (!pr || !pr->prAdapter)
		return;

	prAdapter = pr->prAdapter;
	prRxCtrl = &prAdapter->rRxCtrl;

	RX_INC_CNT(prRxCtrl, RX_NAPI_WORK_COUNT);
	__kalNapiSchedule(prAdapter);
#if CFG_SUPPORT_RX_WORK
	kalNapiWakeup();
#endif /* CFG_SUPPORT_RX_WORK */
}

inline void kalRxNapiWorkSetCpu(struct GLUE_INFO *pr,
	int32_t i4CpuIdx)
{
	kalWorkSetCpu(pr, RX_NAPI_WORK, i4CpuIdx);
}

inline void kalRxNapiWorkInit(struct GLUE_INFO *pr)
{
	kalWorkInit(pr, RX_NAPI_WORK, "RxNapiWork", kalRxNapiWork);
}

inline void kalRxNapiWorkUninit(struct GLUE_INFO *pr)
{
	kalWorkUninit(pr, RX_NAPI_WORK);
}

inline void kalRxNapiWorkSchedule(struct GLUE_INFO *pr)
{
	int32_t i4RxNapiWorkCpu;

	i4RxNapiWorkCpu = kalWorkGetCpu(pr, RX_NAPI_WORK);
	if (i4RxNapiWorkCpu == -1) {
		/* no BoostCpu, favour latency over tput */
		__kalNapiSchedule(pr->prAdapter);
		return;
	}

	kalWorkSchedule(pr, RX_NAPI_WORK);
}
#endif /* CFG_SUPPORT_RX_NAPI_WORK */

#if CFG_SUPPORT_RX_WORK
void kalRxWork(struct work_struct *work)
{
	struct GLUE_INFO *pr = kalWorkGetGlueInfo(work);
	static int32_t i4UserCnt;

	if (!pr)
		return;

	if (GLUE_INC_REF_CNT(i4UserCnt) > 1)
		goto end;

	if (kalWorkCheckState(work) != WLAN_STATUS_SUCCESS)
		goto end;

	TRACE(halRxWork(pr), "halRxWork");

#if CFG_SUPPORT_CPU_STAT
	CPU_STAT_INC_CNT(pr, CPU_RX_WORK_DONE);
#endif /* CFG_SUPPORT_CPU_STAT */
end:
	/* reschedule to run one more time */
	if (GLUE_GET_REF_CNT(pr->u4RxTaskScheduleCnt))
		kalRxWorkSchedule(pr);

	GLUE_DEC_REF_CNT(i4UserCnt);
}

inline void kalRxWorkSetCpu(struct GLUE_INFO *pr,
	int32_t i4CpuIdx)
{
	kalWorkSetCpu(pr, RX_WORK, i4CpuIdx);
}

inline void kalRxWorkInit(struct GLUE_INFO *pr)
{
	kalWorkInit(pr, RX_WORK, "RxWork", kalRxWork);
	kalWorkSetFlag(pr, RX_WORK, ENUM_WORK_FLAG_MULTIWORK);
}

inline void kalRxWorkUninit(struct GLUE_INFO *pr)
{
	kalWorkUninit(pr, RX_WORK);
}

inline void kalRxWorkSchedule(struct GLUE_INFO *pr)
{
	kalWorkSchedule(pr, RX_WORK);
}
#endif /* CFG_SUPPORT_RX_WORK */

#if CFG_SUPPORT_TX_WORK
void kalTxWork(struct work_struct *work)
{
	struct GLUE_INFO *pr = kalWorkGetGlueInfo(work);

	if (!pr)
		return;

	if (skb_queue_len(&pr->rTxDirectSkbQueue))
		kalTxDirectStartXmit(NULL, pr);

#if CFG_SUPPORT_CPU_STAT
	CPU_STAT_INC_CNT(pr, CPU_TX_WORK_DONE);
#endif /* CFG_SUPPORT_CPU_STAT */
}

inline void kalTxWorkSetCpu(struct GLUE_INFO *pr,
	int32_t i4CpuIdx)
{
	kalWorkSetCpu(pr, TX_WORK, i4CpuIdx);
}

inline void kalTxWorkInit(struct GLUE_INFO *pr)
{
	kalWorkInit(pr, TX_WORK, "TxWork", kalTxWork);
}

inline void kalTxWorkUninit(struct GLUE_INFO *pr)
{
	kalWorkUninit(pr, TX_WORK);
}

uint32_t kalTxWorkSchedule(struct sk_buff *prSkb,
	struct GLUE_INFO *pr)
{
	int32_t i4TxWorkCpu, i4Cpu;

	i4TxWorkCpu = kalWorkGetCpu(pr, TX_WORK);
	if (i4TxWorkCpu == -1) {
		/* no BoostCpu, just go through tx direct path */
		return kalTxDirectStartXmit(prSkb, pr);
	}

	i4Cpu = get_cpu();
	put_cpu();

	if ((0x1 << i4Cpu) & kalGetTxBigCpuMask()) {
		/* The running cpu is BigCpu */
		return kalTxDirectStartXmit(prSkb, pr);
	}

	/* The running cpu is not BigCpu */
	if (prSkb)
		skb_queue_tail(&pr->rTxDirectSkbQueue, prSkb);

	if (kalWorkSchedule(pr, TX_WORK) ==
		WLAN_STATUS_NOT_ACCEPTED)
		return kalTxDirectStartXmit(NULL, pr);

	return WLAN_STATUS_SUCCESS;
}
#endif /* CFG_SUPPORT_TX_WORK */

#if CFG_SUPPORT_PER_CPU_TX
static uint32_t __kalPerCpuTxXmit(struct sk_buff *prSkb, struct GLUE_INFO *pr)
{
	static uint32_t counter;
	struct net_device *prDev;
	struct PER_CPU_TX_INFO *prPerCpuTxInfo = &pr->rPerCpuTxInfo;
	struct _PER_CPU_TX_INFO *prInfo;
	uint32_t xmit_more;
	uint32_t stopped;
	uint8_t ucBssIndex;
	uint16_t u2QueueIdx;
	int32_t *prPendingNum;

	if (!prPerCpuTxInfo->fgReady)
		return WLAN_STATUS_NOT_ACCEPTED;

	prInfo = get_cpu_ptr(prPerCpuTxInfo->prInfo);

	__skb_queue_tail(&prInfo->rSkbQ, prSkb);
	counter++;

#if (KERNEL_VERSION(5, 2, 0) <= CFG80211_VERSION_CODE)
	xmit_more = netdev_xmit_more();
#else
	xmit_more = prSkb->xmit_more;
#endif
	if (!xmit_more) {
		uint32_t u4DebugLevel;

		wlanGetDriverDbgLevel(DBG_TX_IDX, &u4DebugLevel);
		if (u4DebugLevel & DBG_CLASS_TEMP) {
			ucBssIndex = GLUE_GET_PKT_BSS_IDX(prSkb);
			prPendingNum =
				pr->ai4TxPendingFrameNumPerQueue[ucBssIndex];
			DBGLOG(TX, TEMP,
			       "xmit_more=%u, count=%u, bss=%u, [%d,%d,%d,%d]",
			       xmit_more, counter, ucBssIndex,
			       prPendingNum[0], prPendingNum[1],
			       prPendingNum[2], prPendingNum[3]);
		}
		tasklet_schedule(&prInfo->rTask);
	} else {
		ucBssIndex = GLUE_GET_PKT_BSS_IDX(prSkb);
		prDev = wlanGetNetDev(pr, ucBssIndex);
		stopped = netif_subqueue_stopped(prDev, prSkb);
		if (unlikely(stopped)) {
			u2QueueIdx = skb_get_queue_mapping(prSkb);
			prPendingNum =
				pr->ai4TxPendingFrameNumPerQueue[ucBssIndex];
			DBGLOG(TX, TRACE,
			       "xmit_more=%u, stopped=%u, count=%u, bss=%u, Pending=%d [%d,%d,%d,%d]",
			       xmit_more, stopped, counter, ucBssIndex,
			       prPendingNum[u2QueueIdx],
			       prPendingNum[0], prPendingNum[1],
			       prPendingNum[2], prPendingNum[3]);

			tasklet_schedule(&prInfo->rTask);
		}
	}

	put_cpu_ptr(prPerCpuTxInfo->prInfo);

	return WLAN_STATUS_SUCCESS;
}

uint32_t kalPerCpuTxXmit(struct sk_buff *prSkb, struct GLUE_INFO *pr)
{
	if (__kalPerCpuTxXmit(prSkb, pr) != WLAN_STATUS_SUCCESS)
		return kalTxDirectStartXmit(prSkb, pr);

	return WLAN_STATUS_SUCCESS;
}

static void __kalPerCpuTxRun(struct GLUE_INFO *pr)
{
	struct PER_CPU_TX_INFO *prPerCpuTxInfo = &pr->rPerCpuTxInfo;
	struct sk_buff_head *prTxDirectSkbQ = &pr->rTxDirectSkbQueue;
	struct _PER_CPU_TX_INFO *prInfo;
	unsigned long flag;
	int cpu;

	if (!prPerCpuTxInfo->fgReady)
		return;

	prInfo = get_cpu_ptr(prPerCpuTxInfo->prInfo);
	if (skb_queue_empty(&prInfo->rSkbQ))
		return;

	cpu = get_cpu();
	put_cpu();

	PER_CPU_TX_SET_RUN(prPerCpuTxInfo, cpu, TRUE);

	spin_lock_irqsave(&prTxDirectSkbQ->lock, flag);
	/* join rSkbQ into prTxDirectSkbQ and reinit rSkbQ */
	skb_queue_splice_tail_init(&prInfo->rSkbQ, prTxDirectSkbQ);
	spin_unlock_irqrestore(&prTxDirectSkbQ->lock, flag);
	put_cpu_ptr(prPerCpuTxInfo->prInfo);

	PER_CPU_TX_SET_RUN(prPerCpuTxInfo, cpu, FALSE);

	/* trigger tx */
#if CFG_SUPPORT_TX_WORK
	kalTxWorkSchedule(NULL, pr);
#else /* CFG_SUPPORT_TX_WORK */
	kalTxDirectStartXmit(NULL, pr);
#endif /* CFG_SUPPORT_TX_WORK */

#if CFG_SUPPORT_CPU_STAT
	CPU_STAT_INC_CNT(pr, CPU_TX_PER_CPU);
#endif /* CFG_SUPPORT_CPU_STAT */
}

static void kalPerCpuTxRun(unsigned long data)
{
	struct GLUE_INFO *pr = (struct GLUE_INFO *)data;

	__kalPerCpuTxRun(pr);
}

void kalPerCpuTxInit(struct GLUE_INFO *pr)
{
	struct PER_CPU_TX_INFO *prPerCpuTxInfo = &pr->rPerCpuTxInfo;
	int cpu;
	struct _PER_CPU_TX_INFO *prInfo;

	prPerCpuTxInfo->prInfo = alloc_percpu_gfp(struct _PER_CPU_TX_INFO,
					GFP_KERNEL | __GFP_ZERO);
	if (!prPerCpuTxInfo->prInfo) {
		DBGLOG(INIT, ERROR, "prPerCpuTxInfo->prInfo alloc fail.\n");
		return;
	}

	for_each_possible_cpu(cpu) {
		prInfo = per_cpu_ptr(prPerCpuTxInfo->prInfo, cpu);
		skb_queue_head_init(&prInfo->rSkbQ);
		tasklet_init(&prInfo->rTask, kalPerCpuTxRun, (unsigned long)pr);
	}

	prPerCpuTxInfo->fgReady = TRUE;
}

void kalPerCpuTxUninit(struct GLUE_INFO *pr)
{
	struct PER_CPU_TX_INFO *prPerCpuTxInfo = &pr->rPerCpuTxInfo;
	int cpu;
	struct _PER_CPU_TX_INFO *prInfo;
	struct sk_buff *prSkb;
	uint32_t u4Cnt = 0;

	prPerCpuTxInfo->fgReady = FALSE;

	/* just use to ensure all tasklet is not running */
	while (PER_CPU_TX_IS_RUNNING(prPerCpuTxInfo)) {
		DBGLOG_LIMITED(INIT, INFO,
			"Waiting for running tasklet u4Cnt:%u\n", u4Cnt);
		kalMsleep(1);
		if (++u4Cnt == PER_CPU_TX_WAITING_TIMEOUT) {
			DBGLOG(INIT, ERROR, "Waiting timeout\n");
			break;
		}
	}

	if (!prPerCpuTxInfo->prInfo)
		return;

	for_each_possible_cpu(cpu) {
		prInfo = per_cpu_ptr(prPerCpuTxInfo->prInfo, cpu);
		tasklet_kill(&prInfo->rTask);

		while (TRUE) {
			prSkb = __skb_dequeue(&prInfo->rSkbQ);
			if (!prSkb)
				break;

			kalSendComplete(pr, prSkb, WLAN_STATUS_NOT_ACCEPTED);
		}
	}

	free_percpu(prPerCpuTxInfo->prInfo);
}
#endif /* CFG_SUPPORT_PER_CPU_TX */

#if CFG_SUPPORT_RETURN_WORK
void kalRxRfbReturnWork(struct work_struct *work)
{
	struct GLUE_INFO *pr = kalWorkGetGlueInfo(work);
	struct ADAPTER *prAdapter;
#if CFG_SUPPORT_DYNAMIC_PAGE_POOL
	struct BUS_INFO *prBusInfo;
#if CFG_DYNAMIC_RFB_ADJUSTMENT
	uint32_t u4TputMbps, u4Idx;
	uint32_t u4RfbIdx = 0;
	u_int8_t fgRet;
#endif /* CFG_DYNAMIC_RFB_ADJUSTMENT */
#endif /* CFG_SUPPORT_DYNAMIC_PAGE_POOL */

	if (!pr)
		return;

	prAdapter = pr->prAdapter;
#if CFG_SUPPORT_DYNAMIC_PAGE_POOL
	prBusInfo = prAdapter->chip_info->bus_info;
#if CFG_DYNAMIC_RFB_ADJUSTMENT

	u4TputMbps = kalGetTpMbps(prAdapter, PKT_PATH_ALL);
	for (u4Idx = 0; u4Idx < PERF_MON_RFB_MAX_THRESHOLD; u4Idx++) {
		if (u4TputMbps <
		    prAdapter->rWifiVar.u4RfbBoostTpTh[u4Idx])
			break;
	}

	if (u4Idx > 0)
		u4RfbIdx = u4Idx - 1;

	if (u4RfbIdx == prAdapter->u4RfbUnUseCntLv)
		goto skip;

	/* set to target level or max level */
	if (u4RfbIdx > prAdapter->u4RfbUnUseCntLv)
		fgRet = nicRxSetRfbCntByLevel(prAdapter, u4RfbIdx);
	else
		fgRet = nicRxDecRfbCnt(prAdapter);

	if (fgRet) {
		DBGLOG(INIT, INFO, "Tput:%uMbps u4Idx:%u u4RfbIdx:%u\n",
			u4TputMbps, u4Idx, u4RfbIdx);
	}
skip:
#endif /* CFG_DYNAMIC_RFB_ADJUSTMENT */
	if (prBusInfo->u4WfdmaTh)
		kalIncPagePoolPageNum();
	else
		kalDecPagePoolPageNum();
#endif /* CFG_SUPPORT_DYNAMIC_PAGE_POOL */

	TRACE(wlanReturnPacketDelaySetup(prAdapter), "RxRfbReturnWork");
}

inline void kalRxRfbReturnWorkSetCpu(struct GLUE_INFO *pr,
	int32_t cpu)
{
	kalWorkSetCpu(pr, RX_RETURN_RFB_WORK, cpu);
}

inline void kalRxRfbReturnWorkInit(struct GLUE_INFO *pr)
{
	kalWorkInit(pr, RX_RETURN_RFB_WORK,
		"RxReturnRfbWork", kalRxRfbReturnWork);
}

inline void kalRxRfbReturnWorkUninit(struct GLUE_INFO *pr)
{
	kalWorkUninit(pr, RX_RETURN_RFB_WORK);
}

inline void kalRxRfbReturnWorkSchedule(struct GLUE_INFO *pr)
{
	kalWorkSchedule(pr, RX_RETURN_RFB_WORK);
}
#endif /* CFG_SUPPORT_RETURN_WORK */

#if CFG_SUPPORT_SKB_ALLOC_WORK
static void kalSkbAllocWorkDone(struct GLUE_INFO *pr, uint8_t ucIdx)
{
	struct SKB_ALLOC_INFO *prSkbAllocInfo = &pr->rSkbAllocInfo;
	uint32_t u4SkbNum = skb_queue_len(&prSkbAllocInfo->rFreeSkbQ);

	if (ucIdx < CON_WORK_MAX)
		clear_bit(ucIdx, &prSkbAllocInfo->ulScheMask);

	if (prSkbAllocInfo->ulScheMask == 0 ||
		u4SkbNum >= prSkbAllocInfo->u4TotalReqNum)
		kalRxRfbReturnWorkSchedule(pr);
}

static void kalDoSkbAlloc(struct GLUE_INFO *pr, uint8_t ucIdx)
{
	struct SKB_ALLOC_INFO *prSkbAllocInfo = &pr->rSkbAllocInfo;
	uint32_t u4ReqNum;
	struct sk_buff_head *prFreeSkbQ;
	struct sk_buff_head rSkbQ;
	struct sk_buff *prSkb;
	uint8_t *pucData;
	u_int8_t fgNoMem = FALSE;
	unsigned long u4Flags;

	u4ReqNum = prSkbAllocInfo->u4ReqNum[ucIdx];
	if (!u4ReqNum)
		return;

	__skb_queue_head_init(&rSkbQ);

	while (u4ReqNum--) {
		prSkb = (struct sk_buff *)__nicRxPacketAlloc(pr, &pucData,
				(int32_t)ucIdx);
		if (!prSkb) {
			fgNoMem = TRUE;
			break;
		}

		__skb_queue_tail(&rSkbQ, prSkb);
	}

	if (fgNoMem)
		set_bit(ucIdx, &prSkbAllocInfo->ulNoMemMask);
	else
		clear_bit(ucIdx, &prSkbAllocInfo->ulNoMemMask);

	if (skb_queue_empty(&rSkbQ))
		return;

	prFreeSkbQ = &prSkbAllocInfo->rFreeSkbQ;
	spin_lock_irqsave(&prFreeSkbQ->lock, u4Flags);
	skb_queue_splice_init(&rSkbQ, prFreeSkbQ);
	spin_unlock_irqrestore(&prFreeSkbQ->lock, u4Flags);

	kalSkbAllocWorkDone(pr, ucIdx);

#if CFG_SUPPORT_CPU_STAT
	CPU_STAT_INC_CNT(pr, CPU_SKB_ALLOC_DONE);
#endif /* CFG_SUPPORT_CPU_STAT */
}

uint32_t kalSkbAllocDeqSkb(struct GLUE_INFO *pr, void **pvPacket,
	uint8_t **ppucData)
{
	struct SKB_ALLOC_INFO *prSkbAllocInfo = &pr->rSkbAllocInfo;
	struct sk_buff *prSkb = NULL;

	if (!skb_queue_empty(&prSkbAllocInfo->rFreeSkbQ)) {
		prSkb = skb_dequeue(&prSkbAllocInfo->rFreeSkbQ);
		if (prSkb != NULL)
			*ppucData = prSkb->data;
	}
	*pvPacket = prSkb;

	/*
	 * return WLAN_STATUS_NOT_ACCEPTED
	 * to let it go through legacy alloc mem alloc path
	 */
	if (prSkbAllocInfo->eCoreType == CPU_CORE_NONE)
		return WLAN_STATUS_NOT_ACCEPTED;
	else
		return WLAN_STATUS_SUCCESS;
}

void kalSkbAllocWorkSetCpu(struct GLUE_INFO *pr, enum CPU_CORE_TYPE eCoreType)
{
	struct ADAPTER *ad;
	struct WIFI_VAR *prWifiVar;
	struct SKB_ALLOC_INFO *prSkbAllocInfo = &pr->rSkbAllocInfo;

	ad = pr->prAdapter;
	if (!ad)
		return;

	prWifiVar = &ad->rWifiVar;
	if (IS_FEATURE_ENABLED(prWifiVar->fgSkbAllocWorkEn)) {
		prSkbAllocInfo->eCoreType = eCoreType;
		DBGLOG(INIT, INFO, "eCoreType:%u\n", eCoreType);
	}
}

void kalSkbAllocWorkInit(struct GLUE_INFO *pr)
{
	struct SKB_ALLOC_INFO *prSkbAllocInfo = &pr->rSkbAllocInfo;
	struct CON_WORK *prConWork;
	uint8_t ucIdx;

	prSkbAllocInfo->eCoreType = CPU_CORE_NONE;
	prSkbAllocInfo->ulScheMask = 0;
	prSkbAllocInfo->ulNoMemMask = 0;
	skb_queue_head_init(&prSkbAllocInfo->rFreeSkbQ);
	for (ucIdx = 0; ucIdx < CON_WORK_MAX; ucIdx++) {
		prSkbAllocInfo->u4ReqNum[ucIdx] = 0;
		prConWork = &prSkbAllocInfo->rConWork[ucIdx];
		kalConWorkInit(pr, prConWork, "wifi_skb_alloc", ucIdx,
			kalDoSkbAlloc);
	}
}

void kalSkbAllocWorkUninit(struct GLUE_INFO *pr)
{
	struct SKB_ALLOC_INFO *prSkbAllocInfo = &pr->rSkbAllocInfo;
	struct CON_WORK *prConWork;
	uint8_t ucIdx;
	struct sk_buff *prSkb;

	for (ucIdx = 0; ucIdx < CON_WORK_MAX; ucIdx++) {
		prConWork = &prSkbAllocInfo->rConWork[ucIdx];
		kalConWorkUninit(pr, prConWork);
	}

	/* Free all skb */
	if (skb_queue_empty(&prSkbAllocInfo->rFreeSkbQ))
		return;

	while ((prSkb = skb_dequeue(&prSkbAllocInfo->rFreeSkbQ)) != NULL)
		kfree_skb(prSkb);
}

static void __kalSkbAllocWorkSchedule(struct GLUE_INFO *pr, uint32_t u4ReqNum)
{
	struct SKB_ALLOC_INFO *prSkbAllocInfo = &pr->rSkbAllocInfo;
	struct CON_WORK *prConWork;
	uint8_t ucIdx;
	uint32_t arJobs[CON_WORK_MAX] = { 0 };

	prSkbAllocInfo->u4TotalReqNum = u4ReqNum;
	prSkbAllocInfo->ulScheMask = kalConWorkDispatchJobs(arJobs, u4ReqNum);
	DBGLOG(INIT, TEMP, "u4ReqNum:%u ulScheMask:0x%x\n",
		prSkbAllocInfo->u4TotalReqNum, prSkbAllocInfo->ulScheMask);

	/* wakeup all worker to alloc skb */
	for (ucIdx = 0; ucIdx < CON_WORK_MAX; ucIdx++) {
		prSkbAllocInfo->u4ReqNum[ucIdx] = arJobs[ucIdx];
		if (prSkbAllocInfo->u4ReqNum[ucIdx] == 0)
			continue;
		prConWork = &prSkbAllocInfo->rConWork[ucIdx];
		kalConWorkSchedule(pr, prConWork, prSkbAllocInfo->eCoreType);
	}
}

void kalSkbAllocWorkSchedule(struct GLUE_INFO *pr, u_int8_t fgForce)
{
	struct ADAPTER *ad;
	struct WIFI_VAR *prWifiVar;
	struct SKB_ALLOC_INFO *prSkbAllocInfo = &pr->rSkbAllocInfo;
	uint32_t u4ReqNum, u4SkbNum;

	ad = pr->prAdapter;
	if (!ad)
		return;

	prWifiVar = &ad->rWifiVar;
	/* check if feature disabled */
	if (unlikely(prSkbAllocInfo->eCoreType == CPU_CORE_NONE)) {
		kalRxRfbReturnWorkSchedule(pr);
		return;
	}

	if (!fgForce && ++prSkbAllocInfo->u4ScheCnt <
		prWifiVar->u4SkbAllocScheduleTh)
		return;

	u4ReqNum = RX_GET_INDICATED_RFB_CNT(&ad->rRxCtrl);
	u4SkbNum = skb_queue_len(&prSkbAllocInfo->rFreeSkbQ);
	/* We need to prevent alloc more memory than we actual need */
	if (u4SkbNum >= u4ReqNum) {
		kalRxRfbReturnWorkSchedule(pr);
		goto end;
	}

	u4ReqNum -= u4SkbNum;
	__kalSkbAllocWorkSchedule(pr, u4ReqNum);
end:
	prSkbAllocInfo->u4ScheCnt = 0;
}

u_int8_t kalSkbAllocIsNoOOM(struct GLUE_INFO *pr)
{
	struct SKB_ALLOC_INFO *prSkbAllocInfo = &pr->rSkbAllocInfo;

	return (prSkbAllocInfo->ulNoMemMask == 0);
}
#endif /* CFG_SUPPORT_SKB_ALLOC_WORK */

#if CFG_SUPPORT_PCIE_GEN_SWITCH
void kalSetPcieGen(struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct BUS_INFO *prBusInfo = NULL;
	uint32_t u4Thr = 0, u4Time = 0;

	if (!prAdapter) {
		DBGLOG(INIT, INFO, "prAdapter is NULL\n");
		return;
	}

	prGlueInfo = prAdapter->prGlueInfo;
	prChipInfo = prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;
	u4Thr = prAdapter->rWifiVar.u4PcieGenSwitchTputThr;
	u4Time = prAdapter->rWifiVar.u4PcieGenSwitchJudgeTime;

	DBGLOG(SW4, TRACE,
		"Thr=[%u], Time=[%u], Tput=[%u], cnt=[%u], gen=[%u]\n",
		u4Thr, u4Time, kalGetTpMbps(prAdapter, PKT_PATH_ALL),
		pcie_monitor_count, prBusInfo->pcie_current_speed);
	if (kalGetTpMbps(prAdapter, PKT_PATH_ALL) > u4Thr) {
		/* change to gen3 */
		pcie_monitor_count = 0;
		if (prBusInfo->pcie_current_speed != PCIE_GEN3 &&
			prBusInfo->setPcieSpeed) {
			prBusInfo->setPcieSpeed(prGlueInfo, PCIE_GEN3);
		}
	} else {
		/* change to gen1 */
		pcie_monitor_count++;
		if (pcie_monitor_count > u4Time &&
			prBusInfo->pcie_current_speed != PCIE_GEN1 &&
			prBusInfo->setPcieSpeed) {
			prBusInfo->setPcieSpeed(prGlueInfo, PCIE_GEN1);
		}
		if (pcie_monitor_count > PCIE_GEN_SWITCH_MONITOR_TIMES_MAX)
			pcie_monitor_count = 0;
	}
}
#endif /* CFG_SUPPORT_PCIE_GEN_SWITCH */

void kalIndicateControlPortTxStatus(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo,
	enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
#if (KERNEL_VERSION(6, 0, 0) <= CFG80211_VERSION_CODE) && \
	(CFG_SUPPORT_CONTROL_PORT_OVER_NL80211 == 1)
	struct mt66xx_chip_info *prChipInfo;
	struct net_device *prNetDev;
	struct wireless_dev *prWdev;
	uint8_t *pucData = NULL;
	uint64_t u8Cookie;
	uint32_t u4TxHeadRoomSize = 0;
	size_t u4Len;

	if (!prAdapter || !prMsduInfo)
		return;

	prChipInfo = prAdapter->chip_info;
	u4TxHeadRoomSize = NIC_TX_DESC_AND_PADDING_LENGTH +
		prChipInfo->txd_append_size;
	prNetDev = wlanGetNetDev(prAdapter->prGlueInfo,
				 prMsduInfo->ucBssIndex);
	if (!prNetDev) {
		DBGLOG(TX, ERROR, "invalid bss idx(%u)\n",
			prMsduInfo->ucBssIndex);
		return;
	}

	prWdev = prNetDev->ieee80211_ptr;
	if (!(wiphy_ext_feature_isset(prWdev->wiphy,
		NL80211_EXT_FEATURE_CONTROL_PORT_OVER_NL80211_TX_STATUS)))
		return;

	kalGetPacketBuf(prMsduInfo->prPacket, &pucData);
	if (!pucData) {
		DBGLOG(TX, ERROR, "invalid skb data\n");
		return;
	}

	u8Cookie = (uint64_t)GLUE_GET_PKT_TX_COOKIE(prMsduInfo->prPacket);
	pucData += u4TxHeadRoomSize;
	u4Len = kalQueryPacketLength(prMsduInfo->prPacket) - u4TxHeadRoomSize;

	DBGLOG(TX, TRACE, "%s: cookie=0x%llx len=%zu status=%d\n",
		prNetDev->name, u8Cookie, u4Len, rTxDoneStatus);

	cfg80211_control_port_tx_status(prWdev, u8Cookie, pucData, u4Len,
		rTxDoneStatus == TX_RESULT_SUCCESS, GFP_ATOMIC);
#endif
}

#if CFG_SUPPORT_HIF_REG_WORK
void kalHifRegWork(struct work_struct *work)
{
	halHandleHifRegReq(kalWorkGetGlueInfo(work));
}

inline void kalHifRegWorkInit(struct GLUE_INFO *pr)
{
	GLUE_SET_REF_CNT(0, pr->u4HifRegStartCnt);
	GLUE_SET_REF_CNT(0, pr->u4HifRegReqCnt);
	pr->u4HifRegFifoLen =
		kalRoundUpPowerOf2(CFG_HIF_REG_MAX_REQ_NUM) * sizeof(void *);
	pr->prHifRegFifoBuf = kalMemAlloc(pr->u4HifRegFifoLen, VIR_MEM_TYPE);
	KAL_FIFO_INIT(&pr->rHifRegFifo,
		      pr->prHifRegFifoBuf,
		      pr->u4HifRegFifoLen);
	kalWorkInit(pr, HIF_REG_WORK, "HifRegWork", kalHifRegWork);
	spin_lock_init(&pr->rHifRegFifoLock);
#if CFG_MTK_WIFI_MBU
	pr->u4MbuTimeoutCnt = 0;
#endif
}

inline void kalHifRegWorkUninit(struct GLUE_INFO *pr)
{
	uint32_t u4Idx;

	for (u4Idx = 0; u4Idx < CFG_HIF_REG_WORK_TIMEOUT_CNT; u4Idx++) {
		if (GLUE_GET_REF_CNT(pr->u4HifRegStartCnt) == 0)
			break;

		kalUsleep(CFG_HIF_REG_WORK_TIMEOUT_TIME);
	}
	if (u4Idx == CFG_HIF_REG_WORK_TIMEOUT_CNT) {
		DBGLOG(HAL, ERROR, "work don't finish, StartCnt[%u]\n",
		       GLUE_GET_REF_CNT(pr->u4HifRegStartCnt));
	}

	for (u4Idx = 0; u4Idx < CFG_HIF_REG_REQ_TIMEOUT_CNT; u4Idx++) {
		if (GLUE_GET_REF_CNT(pr->u4HifRegReqCnt) == 0)
			break;

		kalUsleep(CFG_HIF_REG_REQ_TIMEOUT_TIME);
	}
	if (u4Idx == CFG_HIF_REG_REQ_TIMEOUT_CNT) {
		DBGLOG(HAL, ERROR, "reg request don't finish, ReqCnt[%u]\n",
		       GLUE_GET_REF_CNT(pr->u4HifRegReqCnt));
	}

	kalWorkUninit(pr, HIF_REG_WORK);
	if (pr->prHifRegFifoBuf) {
		kalMemFree(pr->prHifRegFifoBuf,
			   VIR_MEM_TYPE,
			   pr->u4HifRegFifoLen);
		pr->prHifRegFifoBuf = NULL;
		pr->u4HifRegFifoLen = 0;
	}
}

inline void kalHifRegWorkSchedule(struct GLUE_INFO *pr)
{
	kalWorkSchedule(pr, HIF_REG_WORK);
}
#endif /* CFG_SUPPORT_HIF_REG_WORK */

void __weak kalPmicCtrl(u_int8_t fgIsEnabled)
{
	DBGLOG(SW4, WARN, "PMIC control is not defined\n");
}
