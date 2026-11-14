/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2025 MediaTek Inc.
 */

#include "gl_plat.h"

#include "precomp.h"

#ifdef CONFIG_WLAN_MTK_EMI
#if KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
#include <soc/mediatek/emi.h>
#else
#include <memory/mediatek/emi.h>
#endif
#define DOMAIN_AP	0
#define DOMAIN_CONN	2
#endif

#define CPU_BIG_CORE (0xf0)
#define CPU_X_CORE (0x80)
#define CPU_HP_CORE (CPU_BIG_CORE - CPU_X_CORE)
#define CPU_LITTLE_CORE (CPU_ALL_CORE - CPU_BIG_CORE)

#define RPS_ALL_CORE (CPU_ALL_CORE - 0x11)
#define RPS_BIG_CORE (CPU_BIG_CORE - 0x10)
#define RPS_LITTLE_CORE (CPU_LITTLE_CORE - 0x01)

#define MIN_CPU_FREQ (-2)
#define AUTO_PRIORITY 0
#define HIGH_PRIORITY 100

#define BOOST_CPU_TABLE_NUM (PERF_MON_TP_MAX_THRESHOLD + 1)

/* Used to get address of saving fw version offset.               */
/* EMI_base + MCU_EMI_LOG offset(0xB00000) + fw ver offset(0x24). */
#define FW_VERSION_OFFSET_ADDRESS	0xB00024

#if (KERNEL_VERSION(5, 10, 0) <= CFG80211_VERSION_CODE)
#include <linux/regulator/consumer.h>
#endif
#include <linux/platform_device.h>
#include <linux/pinctrl/consumer.h>
#include "wlan_pinctrl.h"
#include <linux/nvmem-consumer.h>

static uint32_t u4EmiMetOffset = 0x45D400;

enum ENUM_CPU_BOOST_STATUS {
	ENUM_CPU_BOOST_STATUS_INIT = 0,
	ENUM_CPU_BOOST_STATUS_LV0,
	ENUM_CPU_BOOST_STATUS_LV1,
	ENUM_CPU_BOOST_STATUS_LV2,
	ENUM_CPU_BOOST_STATUS_NUM
};

static enum ENUM_CPU_BOOST_STATUS eCurrBoost;

enum ENUM_CPU_BOOST_STATUS eBoostCpuTable[BOOST_CPU_TABLE_NUM] = {
	ENUM_CPU_BOOST_STATUS_LV0, /* 0 */
	ENUM_CPU_BOOST_STATUS_LV1, /* 1: 20Mbps */
	ENUM_CPU_BOOST_STATUS_LV1, /* 2 */
	ENUM_CPU_BOOST_STATUS_LV1, /* 3: 100Mbps */
	ENUM_CPU_BOOST_STATUS_LV1, /* 4 */
	ENUM_CPU_BOOST_STATUS_LV2, /* 5: 250Mbps */
	ENUM_CPU_BOOST_STATUS_LV2, /* 6 */
	ENUM_CPU_BOOST_STATUS_LV2, /* 7 */
	ENUM_CPU_BOOST_STATUS_LV2, /* 8: 1200Mbps */
	ENUM_CPU_BOOST_STATUS_LV2, /* 9: 2000Mbps */
	ENUM_CPU_BOOST_STATUS_LV2, /* 10: 3000Mbps */
	ENUM_CPU_BOOST_STATUS_LV2, /* 11: 4000Mbps */
	ENUM_CPU_BOOST_STATUS_LV2  /* 12: 5000Mbps */
};

struct BOOST_INFO rBoostInfo[] = {
	{
		/* ENUM_CPU_BOOST_STATUS_INIT */
	},
	{
		/* ENUM_CPU_BOOST_STATUS_LV0 */
		.rCpuInfo = {
			.i4LittleCpuFreq = AUTO_CPU_FREQ,
			.i4BigCpuFreq = AUTO_CPU_FREQ
		},
		.rHifThreadInfo = {
			.u4CpuMask = CPU_LITTLE_CORE,
			.u4Priority = AUTO_PRIORITY
		},
		.rMainThreadInfo = {
			.u4CpuMask = CPU_LITTLE_CORE,
			.u4Priority = AUTO_PRIORITY
		},
		.rRxThreadInfo = {
			.u4CpuMask = CPU_LITTLE_CORE,
			.u4Priority = AUTO_PRIORITY
		},
		.rRxNapiThreadInfo = {
			.u4CpuMask = CPU_LITTLE_CORE,
			.u4Priority = AUTO_PRIORITY
		},
		.u4RpsMap = RPS_LITTLE_CORE,
		.u4ISRMask = CPU_LITTLE_CORE,
		.i4DramBoostLv = -1,
	},
	{
		/* ENUM_CPU_BOOST_STATUS_LV1 */
		.rCpuInfo = {
			.i4LittleCpuFreq = AUTO_CPU_FREQ,
			.i4BigCpuFreq = AUTO_CPU_FREQ
		},
		.rHifThreadInfo = {
			.u4CpuMask = CPU_ALL_CORE,
			.u4Priority = AUTO_PRIORITY
		},
		.rMainThreadInfo = {
			.u4CpuMask = CPU_ALL_CORE,
			.u4Priority = AUTO_PRIORITY
		},
		.rRxThreadInfo = {
			.u4CpuMask = CPU_ALL_CORE,
			.u4Priority = AUTO_PRIORITY
		},
		.rRxNapiThreadInfo = {
			.u4CpuMask = CPU_ALL_CORE,
			.u4Priority = AUTO_PRIORITY
		},
		.u4RpsMap = RPS_LITTLE_CORE,
		.u4ISRMask = CPU_LITTLE_CORE,
		.i4DramBoostLv = -1,
	},
	{
		/* ENUM_CPU_BOOST_STATUS_LV2 */
		.rCpuInfo = {
			.i4LittleCpuFreq = AUTO_CPU_FREQ,
			.i4BigCpuFreq = MIN_CPU_FREQ,
		},
		.rHifThreadInfo = {
			.u4CpuMask = CPU_BIG_CORE,
			.u4Priority = HIGH_PRIORITY
		},
		.rMainThreadInfo = {
			.u4CpuMask = CPU_BIG_CORE,
			.u4Priority = HIGH_PRIORITY
		},
		.rRxThreadInfo = {
			.u4CpuMask = CPU_BIG_CORE,
			.u4Priority = HIGH_PRIORITY
		},
		.rRxNapiThreadInfo = {
			.u4CpuMask = CPU_BIG_CORE,
			.u4Priority = HIGH_PRIORITY
		},
		.u4RpsMap = RPS_BIG_CORE,
		.u4ISRMask = CPU_BIG_CORE,
		.i4DramBoostLv = -1,
	}
};

uint32_t kalGetCpuBoostThreshold(void)
{
	DBGLOG(SW4, TRACE, "Get CPU Boost Threshold\n");
	/* 5, stands for 250Mbps */
	return 5;
}

int32_t kalCheckTputLoad(struct ADAPTER *prAdapter,
			 uint32_t u4CurrPerfLevel,
			 uint32_t u4TarPerfLevel,
			 int32_t i4Pending,
			 uint32_t u4Used)
{
	uint32_t pendingTh =
		CFG_TX_STOP_NETIF_PER_QUEUE_THRESHOLD *
		prAdapter->rWifiVar.u4PerfMonPendingTh / 100;
	uint32_t usedTh = (HIF_TX_MSDU_TOKEN_NUM / 2) *
		prAdapter->rWifiVar.u4PerfMonUsedTh / 100;
	return u4TarPerfLevel >= 3 &&
	       u4TarPerfLevel < prAdapter->rWifiVar.u4BoostCpuTh &&
	       i4Pending >= pendingTh &&
	       u4Used >= usedTh ?
	       TRUE : FALSE;
}

/**
 * kalSetCpuBoost() - Set CPU boost parameters
 * @prAdapter: pointer to Adapter
 * @prBoostInfo: pointer to Boost Info to retrieve how to boost cpu
 *
 * This function provides parameters configuration for each boost status.
 *
 * About RPS, the system default value of the rps_cpus file is zero.
 * This disables RPS, so the CPU that handles the network interrupt also
 * processes the packet.
 * To enable RPS, configure the appropriate RPS CPU mask with the CPUs that
 * should process packets from the specified network device and receive queue.
 * If the network interrupt rate is extremely high, excluding the CPU that
 * handles network interrupts may also improve performance.
 */
void kalSetCpuBoost(struct ADAPTER *prAdapter,
		struct BOOST_INFO *prBoostInfo)
{
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;

	kalSetCpuFreq(prBoostInfo->rCpuInfo.i4LittleCpuFreq,
			CPU_LITTLE_CORE);
	kalSetCpuFreq(prBoostInfo->rCpuInfo.i4BigCpuFreq,
			CPU_BIG_CORE);

	kalSetCpuMask(prGlueInfo->hif_thread,
			prBoostInfo->rHifThreadInfo.u4CpuMask);
	kalSetCpuMask(prGlueInfo->main_thread,
			prBoostInfo->rMainThreadInfo.u4CpuMask);
	kalSetCpuMask(prGlueInfo->rx_thread,
			prBoostInfo->rRxThreadInfo.u4CpuMask);

	kalSetTaskUtilMinPct(prGlueInfo->u4HifThreadPid,
			prBoostInfo->rHifThreadInfo.u4Priority);
	kalSetTaskUtilMinPct(prGlueInfo->u4TxThreadPid,
			prBoostInfo->rMainThreadInfo.u4Priority);
	kalSetTaskUtilMinPct(prGlueInfo->u4RxThreadPid,
			prBoostInfo->rRxThreadInfo.u4Priority);

#if CFG_SUPPORT_RX_NAPI_THREADED
	if (prGlueInfo->napi_thread) {
		kalSetCpuMask(prGlueInfo->napi_thread,
			prBoostInfo->rRxNapiThreadInfo.u4CpuMask);
		kalSetTaskUtilMinPct(prGlueInfo->u4RxNapiThreadPid,
			prBoostInfo->rRxNapiThreadInfo.u4Priority);
	}
#endif /* CFG_SUPPORT_RX_NAPI_THREADED */

	kalSetRpsMap(prGlueInfo, prBoostInfo->u4RpsMap);
	kalSetISRMask(prAdapter, prBoostInfo->u4ISRMask);

	kalSetDramBoost(prAdapter, prBoostInfo->i4DramBoostLv);


#if CFG_SUPPORT_RX_NAPI_THREADED
#define PLAT_THREAD_INFO "ThreadInfo:[%02x:%02x:%02x:%02x][%u:%u:%u:%u] "
#else
#define PLAT_THREAD_INFO "ThreadInfo:[%02x:%02x:%02x][%u:%u:%u] "
#endif /* CFG_SUPPORT_RX_NAPI_THREADED */

#define TEMP_LOG_TEMPLATE \
	"CPUInfo[%d:%d] " \
	PLAT_THREAD_INFO \
	"Rps:[%02x] ISR:[%02x] D:[%d]\n"

	DBGLOG(INIT, DEBUG,
		TEMP_LOG_TEMPLATE,
		prBoostInfo->rCpuInfo.i4LittleCpuFreq,
		prBoostInfo->rCpuInfo.i4BigCpuFreq,
		prBoostInfo->rHifThreadInfo.u4CpuMask,
		prBoostInfo->rMainThreadInfo.u4CpuMask,
		prBoostInfo->rRxThreadInfo.u4CpuMask,
#if CFG_SUPPORT_RX_NAPI_THREADED
		prBoostInfo->rRxNapiThreadInfo.u4CpuMask,
#endif /* CFG_SUPPORT_RX_NAPI_THREADED */
		prBoostInfo->rHifThreadInfo.u4Priority,
		prBoostInfo->rMainThreadInfo.u4Priority,
		prBoostInfo->rRxThreadInfo.u4Priority,
#if CFG_SUPPORT_RX_NAPI_THREADED
		prBoostInfo->rRxNapiThreadInfo.u4Priority,
#endif /* CFG_SUPPORT_RX_NAPI_THREADED */
		prBoostInfo->u4RpsMap,
		prBoostInfo->u4ISRMask,
		prBoostInfo->i4DramBoostLv);
#undef TEMP_LOG_TEMPLATE
}

/* Update the CPU floor */
void kalUpdateBoostInfo(struct ADAPTER *prAdapter)
{
	uint32_t u4CpuFreq = prAdapter->rWifiVar.au4CpuBoostMinFreq * 1000;
	struct BOOST_INFO *prBoostInfo;
	int i;

	for (i = 0; i < ENUM_CPU_BOOST_STATUS_NUM; i++) {
		prBoostInfo = &rBoostInfo[i];
		if (prBoostInfo->rCpuInfo.i4LittleCpuFreq == MIN_CPU_FREQ)
			prBoostInfo->rCpuInfo.i4LittleCpuFreq = u4CpuFreq;

		if (prBoostInfo->rCpuInfo.i4BigCpuFreq == MIN_CPU_FREQ)
			prBoostInfo->rCpuInfo.i4BigCpuFreq = u4CpuFreq;
	}
}

static void __kalBoostCpuInit(struct ADAPTER *prAdapter)
{
	if (eCurrBoost == ENUM_CPU_BOOST_STATUS_INIT) {
		eCurrBoost = ENUM_CPU_BOOST_STATUS_LV0;
		kalUpdateBoostInfo(prAdapter);
		kalSetCpuBoost(prAdapter, &rBoostInfo[eCurrBoost]);
	}
}

void kalBoostCpuInit(struct ADAPTER *prAdapter)
{
	eCurrBoost = ENUM_CPU_BOOST_STATUS_INIT;
	__kalBoostCpuInit(prAdapter);
}

int32_t kalBoostCpu(struct ADAPTER *prAdapter,
		    uint32_t u4TarPerfLevel,
		    uint32_t u4BoostCpuTh)
{
	enum ENUM_CPU_BOOST_STATUS eNewBoost;

	if (prAdapter->rWifiVar.fgBoostCpuEn == FEATURE_DISABLED)
		return 0;

	__kalBoostCpuInit(prAdapter);

	if (u4TarPerfLevel >= BOOST_CPU_TABLE_NUM)
		eNewBoost = eBoostCpuTable[BOOST_CPU_TABLE_NUM - 1];
	else
		eNewBoost = eBoostCpuTable[u4TarPerfLevel];

	if (eCurrBoost != eNewBoost) {
		DBGLOG(INIT, DEBUG, "TputLv:%u BoostLv[%u->%u]\n",
			u4TarPerfLevel, eCurrBoost, eNewBoost);
		kalTraceEvent("%s TputLv:%u BoostLv[%u->%u]\n", __func__,
			u4TarPerfLevel, eCurrBoost, eNewBoost);
		kalSetCpuBoost(prAdapter, &rBoostInfo[eNewBoost]);
		eCurrBoost = eNewBoost;
	}

	return 0;
}

uint32_t kalGetChipID(void)
{
	return 0x6858;
}

uint32_t kalGetConnsysVersion(void)
{
	return 0x02050405;
}

uint32_t kalGetWfIpVersion(void)
{
	return 0x02040600;
}

uint32_t kalGetFwVerOffset(void)
{
	struct mt66xx_chip_info *prChipInfo = NULL;
	uint32_t u4FwVerOffset = 0;
	uint32_t u4FwVerOffsetAddr = FW_VERSION_OFFSET_ADDRESS;

	glGetChipInfo((void **)&prChipInfo);
	if (emi_mem_read(prChipInfo, u4FwVerOffsetAddr, &u4FwVerOffset,
		sizeof(u4FwVerOffset))) {
		DBGLOG(INIT, WARN, "emi_mem_read %x failed.\n",
			u4FwVerOffsetAddr);
		return WLAN_STATUS_FAILURE;
	}
	return u4FwVerOffset;
}

uint32_t kalGetEmiMetOffset(void)
{
	return u4EmiMetOffset;
}

void kalSetEmiMetOffset(uint32_t newEmiMetOffset)
{
	u4EmiMetOffset = newEmiMetOffset;
}

#ifdef CONFIG_WLAN_MTK_EMI
void kalSetEmiMpuProtection(phys_addr_t emiPhyBase, bool enable)
{
}

void kalSetDrvEmiMpuProtection(phys_addr_t emiPhyBase, uint32_t offset,
			       uint32_t size)
{
}
#endif

void kalReviseCfgsByPlatform(struct ADAPTER *prAdapter)
{
	struct platform_device *pdev;
	struct device *dev;
	struct nvmem_cell *cell;
	struct WIFI_VAR *prWifiVar;
	uint8_t *pucEfusebuf;

	if (!prAdapter ||
	    !prAdapter->chip_info ||
	    !prAdapter->chip_info->platform_device)
		return;

	prWifiVar = &prAdapter->rWifiVar;
	pdev = prAdapter->chip_info->platform_device;
	dev = &pdev->dev;
	cell = nvmem_cell_get(dev, "efuse_segment_cell");
	if (IS_ERR(cell)) {
		if (PTR_ERR(cell) == -EPROBE_DEFER) {
			DBGLOG(INIT, INFO, "EPROBE_DEFER\n");
			return;
		}
		DBGLOG(INIT, INFO, "IS_ERR cell\n");
		return;
	}

	pucEfusebuf = (uint8_t *)nvmem_cell_read(cell, NULL);
	nvmem_cell_put(cell);
	if (IS_ERR(pucEfusebuf)) {
		DBGLOG(INIT, INFO, "IS_ERR buff\n");
		return;
	}

	DBGLOG(INIT, INFO, "buf %d\n", *pucEfusebuf);
#if (CFG_SUPPORT_802_11AX == 1)
	/*
	 * efuse_segment_cell value meanings:
	 *     1: MT6858
	 *     2: MT6858T
	 */
	if (*pucEfusebuf == 1) { /* 1 means MT6858 project, disable wifi 6 */
		prWifiVar->ucStaHe = FEATURE_DISABLED;
		prWifiVar->ucApHe = FEATURE_DISABLED;
		prWifiVar->ucP2pGoHe = FEATURE_DISABLED;
		prWifiVar->ucP2pGcHe = FEATURE_DISABLED;
	}
#endif
	kfree(pucEfusebuf);
}

int32_t kalCheckVcoreBoost(struct ADAPTER *prAdapter,
		uint8_t uBssIndex)
{
#if (KERNEL_VERSION(5, 10, 0) <= CFG80211_VERSION_CODE) && \
	(CFG_SUPPORT_802_11AX == 1)
	struct BSS_INFO *prBssInfo;
	uint8_t ucPhyType;
	struct GL_HIF_INFO *prHifInfo = NULL;
#if defined(_HIF_PCIE)
	struct pci_dev *pdev = NULL;
#else
	struct platform_device *pdev = NULL;
#endif /* _HIF_PCIE */
	struct regulator *dvfsrc_vcore_power;

	prBssInfo = prAdapter->aprBssInfo[uBssIndex];
	ucPhyType = prBssInfo->ucPhyTypeSet;
	prHifInfo = &prAdapter->prGlueInfo->rHifInfo;
	pdev = prHifInfo->pdev;
	DBGLOG(BSS, INFO, "Vcore boost checking: AX + BW160\n");
	if (prBssInfo->eConnectionState == MEDIA_STATE_CONNECTED
		&& ucPhyType & PHY_TYPE_SET_802_11AX
		&& (prBssInfo->ucVhtChannelWidth ==
				VHT_OP_CHANNEL_WIDTH_160)) {
		if (prAdapter->ucVcoreBoost == FALSE) {
			DBGLOG(BSS, INFO, "Vcore boost to 0.65v\n");
			prAdapter->ucVcoreBoost = TRUE;
			dvfsrc_vcore_power = regulator_get(&pdev->dev,
					"dvfsrc-vcore");
			/* Raise VCORE to 0.65v */
			regulator_set_voltage(dvfsrc_vcore_power,
					650000, INT_MAX);
			return TRUE;
		}
	} else {
		if (prAdapter->ucVcoreBoost == TRUE) {
			DBGLOG(BSS, INFO, "Vcore back to 0.575v\n");
			prAdapter->ucVcoreBoost = FALSE;
			dvfsrc_vcore_power = regulator_get(&pdev->dev,
					"dvfsrc-vcore");
			/* Adjust VCORE back to normal*/
			regulator_set_voltage(dvfsrc_vcore_power,
					575000, INT_MAX);
			return FALSE;
		}
	}
	return FALSE;
#else
	return FALSE;
#endif
}
