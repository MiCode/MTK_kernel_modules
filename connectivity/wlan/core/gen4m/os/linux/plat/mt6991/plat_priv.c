// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#include "gl_os.h"

#if KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
#include <uapi/linux/sched/types.h>
#include <linux/sched/task.h>
#include <linux/cpufreq.h>
#endif
#include <linux/pm_qos.h>
#include <linux/gpio.h>
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

#define CONN_INFRA_ID	0x02050601

#define MAX_CPU_FREQ (2500 * 1000)
#define MID_BIG_CPU_FREQ (2000 * 1000)
#define MID_LITTLE_CPU_FREQ (1000 * 1000)
#define AUTO_CPU_FREQ (0)
#define BIG_CPU_FREQ_MAX (3250 * 1000)
#define BIG_CPU_FREQ_MIN (1250 * 1000)
#define LITTLE_CPU_FREQ_MAX (2000 * 1000)
#define LITTLE_CPU_FREQ_MIN (600 * 1000)
#define UNDEFINED_CPU_FREQ (-2)
#define CPU_ALL_CORE (0xff)
#define CPU_BIG_CORE (0xf0)
#define CPU_MID_CORE (0x70)
#define CPU_X_CORE (0x80)
#define CPU_HP_CORE (CPU_BIG_CORE - CPU_X_CORE)
#define CPU_LITTLE_CORE (CPU_ALL_CORE - CPU_BIG_CORE)
#define CPU_MID_LITTLE_CORE (CPU_ALL_CORE - CPU_X_CORE)
#define AUTO_PRIORITY 0
#define HIGH_PRIORITY 100

#define RPS_ALL_CORE (CPU_ALL_CORE - 0x11)
#define RPS_BIG_CORE (CPU_BIG_CORE - 0x10)
#define RPS_HP_CORE (CPU_HP_CORE - 0x10)
#define RPS_LITTLE_CORE (CPU_LITTLE_CORE - 0x01)

#define TX_CPU_BIG_CORE (CPU_BIG_CORE - 0x20)

#define BOOST_CPU_TABLE_NUM (PERF_MON_TP_MAX_THRESHOLD + 1)

#define OPP_BW_MAX_NUM 9

#if (KERNEL_VERSION(5, 10, 0) <= CFG80211_VERSION_CODE)
#include <linux/regulator/consumer.h>
#endif
#include <linux/platform_device.h>
#include <linux/pinctrl/consumer.h>
#include "wlan_pinctrl.h"

/* for dram boost */
#include "dvfsrc-exp.h"
#include <linux/interconnect.h>

static uint32_t u4EmiMetOffset = 0x18000;
static uint32_t u4ProjectId = 6991;

#if defined(CFG_MTK_WIFI_CONNV3_SUPPORT)
#define RST_PIN_MIN_WAIT_TIME		200 /* ms */

static struct pinctrl *pinctrl_ptr;
static int8_t last_wf_rst_pin_state;
static OS_SYSTIME last_toggle_time;

static int32_t mt6991_wlan_pinctrl_init(struct mt66xx_chip_info *chip_info);
static int32_t mt6991_wlan_pinctrl_action(struct mt66xx_chip_info *chip_info,
	enum WLAN_PINCTRL_MSG msg);

static struct WLAN_PINCTRL_OPS mt6991_pinctrl_ops = {
	.init = mt6991_wlan_pinctrl_init,
	.action = mt6991_wlan_pinctrl_action,
};
#endif

enum ENUM_CPU_BOOST_STATUS {
	ENUM_CPU_BOOST_STATUS_INIT = 0,
	ENUM_CPU_BOOST_STATUS_LV0,
	ENUM_CPU_BOOST_STATUS_LV1,
	ENUM_CPU_BOOST_STATUS_LV2,
	ENUM_CPU_BOOST_STATUS_LV3,
	ENUM_CPU_BOOST_STATUS_LV4,
	ENUM_CPU_BOOST_STATUS_NUM
};
static enum ENUM_CPU_BOOST_STATUS eCurrBoost;

enum ENUM_CPU_BOOST_STATUS eBoostCpuTable[BOOST_CPU_TABLE_NUM] = {
	ENUM_CPU_BOOST_STATUS_LV0, /* 0 */
	ENUM_CPU_BOOST_STATUS_LV0, /* 1 */
	ENUM_CPU_BOOST_STATUS_LV0, /* 2 */
	ENUM_CPU_BOOST_STATUS_LV0, /* 3: 100Mbps */
	ENUM_CPU_BOOST_STATUS_LV0, /* 4 */
	ENUM_CPU_BOOST_STATUS_LV0, /* 5: 250Mbps */
	ENUM_CPU_BOOST_STATUS_LV0, /* 6 */
	ENUM_CPU_BOOST_STATUS_LV0, /* 7 */
	ENUM_CPU_BOOST_STATUS_LV1, /* 8: 1200Mbps */
	ENUM_CPU_BOOST_STATUS_LV2, /* 9: 2000Mbps */
	ENUM_CPU_BOOST_STATUS_LV3, /* 10: 3000Mbps */
	ENUM_CPU_BOOST_STATUS_LV4, /* 11: 4000Mbps */
	ENUM_CPU_BOOST_STATUS_LV4  /* 12: 5000Mbps */
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
			.u4CpuMask = CPU_MID_LITTLE_CORE,
			.u4Priority = AUTO_PRIORITY
		},
		.rHifNapiThreadInfo = {
			.u4CpuMask = CPU_LITTLE_CORE,
			.u4Priority = AUTO_PRIORITY
		},
		.u4RpsMap = RPS_LITTLE_CORE,
		.u4ISRMask = CPU_LITTLE_CORE,
		.i4RxRfbRetWorkCpu = -1,
		.i4TxWorkCpu = -1,
		.i4RxWorkCpu = -1,
		.i4RxNapiWorkCpu = -1,
		.fgKeepPcieWakeup = FALSE,
		.u4WfdmaTh = 0,
		.i4TxFreeMsduWorkCpu = -1,
		.fgWifiNappingForceDis = FALSE,
		.i4DramBoostLv = -1,
		.eSkbAllocWorkCoreType = CPU_CORE_NONE,
		.eTxFreeSkbWorkCoreType = CPU_CORE_NONE,
	},
	{
		/* ENUM_CPU_BOOST_STATUS_LV1 */
		.rCpuInfo = {
			.i4LittleCpuFreq = MID_LITTLE_CPU_FREQ,
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
			.u4CpuMask = CPU_MID_LITTLE_CORE,
			.u4Priority = AUTO_PRIORITY
		},
		.rHifNapiThreadInfo = {
			.u4CpuMask = CPU_MID_LITTLE_CORE,
			.u4Priority = AUTO_PRIORITY
		},
		.u4RpsMap = RPS_LITTLE_CORE,
		.u4ISRMask = CPU_LITTLE_CORE,
		.i4TxFreeMsduWorkCpu = 2,
		.i4RxRfbRetWorkCpu = 2,
		.i4TxWorkCpu = 2,
		.i4RxWorkCpu = 3,
		.i4RxNapiWorkCpu = 1,
		.fgKeepPcieWakeup = FALSE,
		.u4WfdmaTh = 1,
		.fgWifiNappingForceDis = TRUE,
		.i4DramBoostLv = -1,
		.eSkbAllocWorkCoreType = CPU_CORE_LITTLE,
		.eTxFreeSkbWorkCoreType = CPU_CORE_LITTLE,
	},
	{
		/* ENUM_CPU_BOOST_STATUS_LV2 */
		.rCpuInfo = {
			.i4LittleCpuFreq = MID_LITTLE_CPU_FREQ,
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
			.u4CpuMask = CPU_MID_LITTLE_CORE,
			.u4Priority = AUTO_PRIORITY
		},
		.rHifNapiThreadInfo = {
			.u4CpuMask = CPU_MID_LITTLE_CORE,
			.u4Priority = AUTO_PRIORITY
		},
		.u4RpsMap = RPS_LITTLE_CORE,
		.u4ISRMask = CPU_LITTLE_CORE,
		.i4TxFreeMsduWorkCpu = 2,
		.i4RxRfbRetWorkCpu = 2,
		.i4TxWorkCpu = 2,
		.i4RxWorkCpu = 3,
		.i4RxNapiWorkCpu = 1,
		.fgKeepPcieWakeup = TRUE,
		.u4WfdmaTh = 1,
		.fgWifiNappingForceDis = TRUE,
		.i4DramBoostLv = -1,
		.eSkbAllocWorkCoreType = CPU_CORE_LITTLE,
		.eTxFreeSkbWorkCoreType = CPU_CORE_LITTLE,
	},
	{
		/* ENUM_CPU_BOOST_STATUS_LV3 */
		.rCpuInfo = {
			.i4LittleCpuFreq = MID_LITTLE_CPU_FREQ,
			.i4BigCpuFreq = AUTO_CPU_FREQ
		},
		.rHifThreadInfo = {
			.u4CpuMask = CPU_MID_CORE,
			.u4Priority = AUTO_PRIORITY
		},
		.rMainThreadInfo = {
			.u4CpuMask = CPU_MID_LITTLE_CORE,
			.u4Priority = AUTO_PRIORITY
		},
		.rRxThreadInfo = {
			.u4CpuMask = CPU_MID_CORE,
			.u4Priority = AUTO_PRIORITY
		},
		.rRxNapiThreadInfo = {
			.u4CpuMask = CPU_MID_CORE,
			.u4Priority = AUTO_PRIORITY
		},
		.rHifNapiThreadInfo = {
			.u4CpuMask = CPU_MID_LITTLE_CORE,
			.u4Priority = AUTO_PRIORITY
		},
		.u4RpsMap = RPS_BIG_CORE,
		.u4ISRMask = CPU_BIG_CORE,
		.i4TxFreeMsduWorkCpu = 4,
		.i4RxRfbRetWorkCpu = 2,
		.i4TxWorkCpu = 2,
		.i4RxWorkCpu = 3,
		.i4RxNapiWorkCpu = 1,
		.fgKeepPcieWakeup = TRUE,
		.u4WfdmaTh = 1,
		.fgWifiNappingForceDis = TRUE,
		.i4DramBoostLv = -1,
		.eSkbAllocWorkCoreType = CPU_CORE_LITTLE,
		.eTxFreeSkbWorkCoreType = CPU_CORE_BIG,
	},
	{
		/* ENUM_CPU_BOOST_STATUS_LV4 */
		.rCpuInfo = {
			.i4LittleCpuFreq = MAX_CPU_FREQ,
			.i4BigCpuFreq = MAX_CPU_FREQ
		},
		.rHifThreadInfo = {
			.u4CpuMask = CPU_MID_CORE,
			.u4Priority = HIGH_PRIORITY
		},
		.rMainThreadInfo = {
			.u4CpuMask = CPU_MID_LITTLE_CORE,
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
		.rHifNapiThreadInfo = {
			.u4CpuMask = CPU_BIG_CORE,
			.u4Priority = HIGH_PRIORITY
		},
		.u4RpsMap = RPS_BIG_CORE,
		.u4ISRMask = CPU_X_CORE,
		.i4TxFreeMsduWorkCpu = 4,
		.i4RxRfbRetWorkCpu = 2,
		.i4TxWorkCpu = 6,
		.i4RxWorkCpu = 3,
		.i4RxNapiWorkCpu = 5,
		.fgKeepPcieWakeup = TRUE,
		.u4WfdmaTh = 2,
		.fgWifiNappingForceDis = TRUE,
		.i4DramBoostLv = 3,
		.eSkbAllocWorkCoreType = CPU_CORE_LITTLE,
		.eTxFreeSkbWorkCoreType = CPU_CORE_BIG,
	}
};

uint32_t kalGetCpuBoostThreshold(void)
{
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

u_int8_t kalCheckBoostCpuMargin(struct ADAPTER *prAdapter)
{
	struct PERF_MONITOR *prPerMonitor;
	enum ENUM_CPU_BOOST_STATUS eNewBoost;
	uint32_t u4Margin;
	uint64_t maxTput = 0;

	prPerMonitor = &prAdapter->rPerMonitor;

	if (!prPerMonitor->fgPolicyReady)
		return FALSE;

	maxTput = prPerMonitor->ulThroughput >> 20;

	eNewBoost = eBoostCpuTable[prPerMonitor->u4PrevPerfLevel];
	if (prPerMonitor->u4PrevPerfLevel
		&& (rBoostInfo[eNewBoost].rCpuInfo.i4BigCpuFreq
			!= AUTO_CPU_FREQ
			|| rBoostInfo[eNewBoost].rCpuInfo.i4LittleCpuFreq
				!= AUTO_CPU_FREQ)) {
		u4Margin = kal_div_u64(prPerMonitor->ulStableTput, 10);

		if (maxTput < (prPerMonitor->ulStableTput + u4Margin)
			&& maxTput > (prPerMonitor->ulStableTput - u4Margin)) {
			return TRUE;
		}
		DBGLOG(INIT, TRACE, "Out of Margin, [%u/%u/%u]",
			prPerMonitor->ulStableTput - u4Margin,
			maxTput,
			prPerMonitor->ulStableTput + u4Margin);
	}

	return FALSE;
}

#if KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
void kalSetTaskUtilMinPct(int pid, unsigned int min)
{
	int ret = 0;
	unsigned int blc_1024;
	struct task_struct *p;
	struct sched_attr attr = {};

	if (pid < 0)
		return;

	/* Fill in sched_attr */
	attr.sched_policy = -1;
	attr.sched_flags =
		SCHED_FLAG_KEEP_ALL |
		SCHED_FLAG_UTIL_CLAMP |
		SCHED_FLAG_RESET_ON_FORK;

	if (min == 0) {
		attr.sched_util_min = -1;
		attr.sched_util_max = -1;
	} else {
		blc_1024 = (min << 10) / 100U;
		blc_1024 = clamp(blc_1024, 1U, 1024U);
		attr.sched_util_min = (blc_1024 << 10) / 1280;
		attr.sched_util_max = (blc_1024 << 10) / 1280;
	}

	/* get task_struct */
	rcu_read_lock();
	p = find_task_by_vpid(pid);
	if (likely(p))
		get_task_struct(p);
	rcu_read_unlock();

	/* sched_setattr */
	if (likely(p)) {
		ret = sched_setattr(p, &attr);
		put_task_struct(p);
	}
}

static LIST_HEAD(wlan_policy_list);
struct wlan_policy {
	struct freq_qos_request	qos_req;
	struct list_head	list;
	int cpu;
};

void kalSetCpuFreq(int32_t freq, uint32_t set_mask)
{
	int cpu, ret;
	struct cpufreq_policy *policy;
	struct wlan_policy *wReq;

	if (list_empty(&wlan_policy_list)) {
		for_each_possible_cpu(cpu) {
			policy = cpufreq_cpu_get(cpu);
			if (!policy)
				continue;

			wReq = kzalloc(sizeof(struct wlan_policy), GFP_KERNEL);
			if (!wReq)
				break;
			wReq->cpu = cpu;

			ret = freq_qos_add_request(&policy->constraints,
				&wReq->qos_req, FREQ_QOS_MIN, AUTO_CPU_FREQ);
			if (ret < 0) {
				DBGLOG(INIT, INFO,
					"freq_qos_add_request fail cpu%d ret=%d\n",
					wReq->cpu, ret);
				kfree(wReq);
				break;
			}

			list_add_tail(&wReq->list, &wlan_policy_list);
			cpufreq_cpu_put(policy);
		}
	}

	list_for_each_entry(wReq, &wlan_policy_list, list) {
		if (!((0x1 << wReq->cpu) & set_mask))
			continue;

		ret = freq_qos_update_request(&wReq->qos_req, freq);
		if (ret < 0) {
			DBGLOG(INIT, INFO,
				"freq_qos_update_request fail cpu%d freq=%d ret=%d\n",
				wReq->cpu, freq, ret);
		}
	}
}

void kalSetDramBoost(struct ADAPTER *prAdapter, int32_t iLv)
{
	struct platform_device *pdev;
#ifdef CONFIG_OF
	struct device_node *node;
	static struct icc_path *bw_path;
#endif /* CONFIG_OF */
	static unsigned int peak_bw[OPP_BW_MAX_NUM], current_bw;
	unsigned int prev_bw = 0, i;

	kalGetPlatDev(&pdev);
	if (!pdev) {
		DBGLOG(INIT, ERROR, "pdev is NULL\n");
		return;
	}

	if (!bw_path) {
#ifdef CONFIG_OF
		/* Update the peak bw of dram */
		node = pdev->dev.of_node;
		bw_path = of_icc_get(&pdev->dev, "wifi-perf-bw");
		if (IS_ERR(bw_path)) {
			DBGLOG(INIT, ERROR,
				"WLAN-OF: unable to get bw path!\n");
			return;
		}

#if IS_ENABLED(CONFIG_MTK_DVFSRC)
		for (i = 0; i < OPP_BW_MAX_NUM; i++)
			peak_bw[i] = dvfsrc_get_required_opp_peak_bw(node, i);
#endif /* CONFIG_MTK_DVFSRC */
#endif /* CONFIG_OF */
	}

	if (!IS_ERR(bw_path)) {
		prev_bw = current_bw;

		if (iLv != -1 && iLv < OPP_BW_MAX_NUM)
			current_bw = peak_bw[iLv];
		else
			current_bw = 0;

		icc_set_bw(bw_path, 0, current_bw);
		DBGLOG(INIT, INFO, "[%d] bw %u => %u\n",
			iLv, prev_bw, current_bw);
	}
}

static int kalSetCpuMask(struct task_struct *task, uint32_t set_mask)
{
	int r = -1;
#if CFG_SUPPORT_TPUT_ON_BIG_CORE
	struct cpumask cpu_mask;
	int i;

	if (task == NULL)
		return r;

	if (set_mask == CPU_ALL_CORE)
		r = set_cpus_allowed_ptr(task, cpu_all_mask);
	else {
		cpumask_clear(&cpu_mask);
		for (i = 0; i < num_possible_cpus(); i++)
			if ((0x1 << i) & set_mask)
				cpumask_or(&cpu_mask, &cpu_mask, cpumask_of(i));
		r = set_cpus_allowed_ptr(task, &cpu_mask);
	}
	DBGLOG(INIT, INFO, "set_cpus_allowed_ptr()=%d", r);
#endif
	return r;
}

void kalSetRunOnNonXCore(struct task_struct *task)
{
	kalSetCpuMask(task, CPU_HP_CORE | CPU_LITTLE_CORE);
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

#if CFG_SUPPORT_HIF_RX_NAPI
	if (prGlueInfo->rHifInfo.rNapiDev.napi_thread) {
		kalSetCpuMask(prGlueInfo->rHifInfo.rNapiDev.napi_thread,
			prBoostInfo->rHifNapiThreadInfo.u4CpuMask);
		kalSetTaskUtilMinPct(prGlueInfo->rHifInfo.rNapiDev.u4ThreadPid,
			prBoostInfo->rHifNapiThreadInfo.u4Priority);
	}
#endif /* CFG_SUPPORT_HIF_RX_NAPI */

	kalSetRpsMap(prGlueInfo, prBoostInfo->u4RpsMap);
	kalSetISRMask(prAdapter, prBoostInfo->u4ISRMask);

#if CFG_SUPPORT_TX_FREE_MSDU_WORK
	kalTxFreeMsduWorkSetCpu(prGlueInfo,
			prBoostInfo->i4TxFreeMsduWorkCpu);
#endif /* CFG_SUPPORT_TX_FREE_MSDU_WORK */

#if CFG_SUPPORT_TX_FREE_SKB_WORK
	kalTxFreeSkbWorkSetCpu(prGlueInfo,
			prBoostInfo->eTxFreeSkbWorkCoreType);
#endif /* CFG_SUPPORT_TX_FREE_SKB_WORK */

#if CFG_SUPPORT_RETURN_WORK
	kalRxRfbReturnWorkSetCpu(prGlueInfo,
			prBoostInfo->i4RxRfbRetWorkCpu);
#endif /* CFG_SUPPORT_RETURN_WORK */

#if CFG_SUPPORT_SKB_ALLOC_WORK
	kalSkbAllocWorkSetCpu(prGlueInfo,
			prBoostInfo->eSkbAllocWorkCoreType);
#endif /* CFG_SUPPORT_SKB_ALLOC_WORK */

#if CFG_SUPPORT_TX_WORK
	kalTxWorkSetCpu(prGlueInfo, prBoostInfo->i4TxWorkCpu);
#endif /* CFG_SUPPORT_TX_WORK */

#if CFG_SUPPORT_RX_WORK
	kalRxWorkSetCpu(prGlueInfo, prBoostInfo->i4RxWorkCpu);
#endif /* CFG_SUPPORT_RX_WORK */

#if CFG_SUPPORT_RX_NAPI_WORK
	kalRxNapiWorkSetCpu(prGlueInfo, prBoostInfo->i4RxNapiWorkCpu);
#endif /* CFG_SUPPORT_RX_NAPI_WORK */

#if defined(_HIF_PCIE)
	kalSetPcieKeepWakeup(prGlueInfo, prBoostInfo->fgKeepPcieWakeup);
	kalConfigWfdmaTh(prGlueInfo, prBoostInfo->u4WfdmaTh);
#endif

	kalSetDramBoost(prAdapter, prBoostInfo->i4DramBoostLv);


#if CFG_SUPPORT_TX_FREE_MSDU_WORK
#define TX_FREE_MSDU_WORK_TEMPLATE " TxFreeMsduWork:[%d]"
#else /* CFG_SUPPORT_TX_FREE_MSDU_WORK */
#define TX_FREE_MSDU_WORK_TEMPLATE ""
#endif /* CFG_SUPPORT_TX_FREE_MSDU_WORK */

#if CFG_SUPPORT_RETURN_WORK
#define RETURN_WORK_TEMPLATE " RxRfbRetWork:[%d]"
#else /* CFG_SUPPORT_RETURN_WORK */
#define RETURN_WORK_TEMPLATE ""
#endif /* CFG_SUPPORT_RETURN_WORK */

#if CFG_SUPPORT_TX_WORK
#define TX_WORK_TEMPLATE " TxWork:[%d]"
#else /* CFG_SUPPORT_TX_WORK */
#define TX_WORK_TEMPLATE ""
#endif /* CFG_SUPPORT_TX_WORK */

#if CFG_SUPPORT_RX_WORK
#define RX_WORK_TEMPLATE " RxWork:[%d]"
#else /* CFG_SUPPORT_RX_WORK */
#define RX_WORK_TEMPLATE ""
#endif /* CFG_SUPPORT_RX_WORK */

#if CFG_SUPPORT_RX_NAPI_WORK
#define RX_NAPI_WORK_TEMPLATE " RxNapiWork:[%d]"
#else /* CFG_SUPPORT_RX_NAPI_WORK */
#define RX_NAPI_WORK_TEMPLATE ""
#endif /* CFG_SUPPORT_RX_NAPI_WORK */

#if CFG_SUPPORT_RX_NAPI_THREADED
#if CFG_SUPPORT_HIF_RX_NAPI
#define PLAT_THREAD_INFO "ThreadInfo:[%02x:%02x:%02x:%02x:%02x][%u:%u:%u:%u:%u] "
#else
#define PLAT_THREAD_INFO "ThreadInfo:[%02x:%02x:%02x:%02x][%u:%u:%u:%u] "
#endif /* CFG_SUPPORT_HIF_RX_NAPI */
#else /* CFG_SUPPORT_RX_NAPI_THREADED */
#if CFG_SUPPORT_HIF_RX_NAPI
#define PLAT_THREAD_INFO "ThreadInfo:[%02x:%02x:%02x:%02x][%u:%u:%u:%u] "
#else
#define PLAT_THREAD_INFO "ThreadInfo:[%02x:%02x:%02x][%u:%u:%u] "
#endif /* CFG_SUPPORT_HIF_RX_NAPI */
#endif /* CFG_SUPPORT_RX_NAPI_THREADED */

#if CFG_SUPPORT_SKB_ALLOC_WORK
#define SKB_ALLOC_WORK_TEMPLATE " SkbAllocWork:[%u]"
#else /* CFG_SUPPORT_SKB_ALLOC_WORK */
#define SKB_ALLOC_WORK_TEMPLATE ""
#endif /* CFG_SUPPORT_SKB_ALLOC_WORK */

#if CFG_SUPPORT_TX_FREE_SKB_WORK
#define TX_FREE_SKB_WORK_TEMPLATE " TxFreeSkbWork:[%u]"
#else /* CFG_SUPPORT_TX_FREE_SKB_WORK */
#define TX_FREE_SKB_WORK_TEMPLATE ""
#endif /* CFG_SUPPORT_TX_FREE_SKB_WORK */

#define TEMP_LOG_TEMPLATE \
	"CPUInfo[%d:%d] " \
	PLAT_THREAD_INFO \
	"Rps:[%02x] ISR:[%02x] D:[%d] Pcie:[%u]" \
	TX_FREE_MSDU_WORK_TEMPLATE \
	RETURN_WORK_TEMPLATE \
	TX_WORK_TEMPLATE \
	RX_WORK_TEMPLATE \
	RX_NAPI_WORK_TEMPLATE \
	SKB_ALLOC_WORK_TEMPLATE \
	TX_FREE_SKB_WORK_TEMPLATE \
	"%s\n"

	DBGLOG(INIT, INFO,
		TEMP_LOG_TEMPLATE,
		prBoostInfo->rCpuInfo.i4LittleCpuFreq,
		prBoostInfo->rCpuInfo.i4BigCpuFreq,
		prBoostInfo->rHifThreadInfo.u4CpuMask,
		prBoostInfo->rMainThreadInfo.u4CpuMask,
		prBoostInfo->rRxThreadInfo.u4CpuMask,
#if CFG_SUPPORT_RX_NAPI_THREADED
		prBoostInfo->rRxNapiThreadInfo.u4CpuMask,
#endif /* CFG_SUPPORT_RX_NAPI_THREADED */
#if CFG_SUPPORT_HIF_RX_NAPI
		prBoostInfo->rHifNapiThreadInfo.u4CpuMask,
#endif /* CFG_SUPPORT_HIF_RX_NAPI */
		prBoostInfo->rHifThreadInfo.u4Priority,
		prBoostInfo->rMainThreadInfo.u4Priority,
		prBoostInfo->rRxThreadInfo.u4Priority,
#if CFG_SUPPORT_RX_NAPI_THREADED
		prBoostInfo->rRxNapiThreadInfo.u4Priority,
#endif /* CFG_SUPPORT_RX_NAPI_THREADED */
#if CFG_SUPPORT_HIF_RX_NAPI
		prBoostInfo->rHifNapiThreadInfo.u4Priority,
#endif /* CFG_SUPPORT_HIF_RX_NAPI */
		prBoostInfo->u4RpsMap,
		prBoostInfo->u4ISRMask,
		prBoostInfo->i4DramBoostLv,
		prBoostInfo->fgKeepPcieWakeup,
#if CFG_SUPPORT_TX_FREE_MSDU_WORK
		prBoostInfo->i4TxFreeMsduWorkCpu,
#endif /* CFG_SUPPORT_TX_FREE_MSDU_WORK */
#if CFG_SUPPORT_RETURN_WORK
		prBoostInfo->i4RxRfbRetWorkCpu,
#endif /* CFG_SUPPORT_RETURN_WORK */
#if CFG_SUPPORT_TX_WORK
		prBoostInfo->i4TxWorkCpu,
#endif /* CFG_SUPPORT_TX_WORK */
#if CFG_SUPPORT_RX_WORK
		prBoostInfo->i4RxWorkCpu,
#endif /* CFG_SUPPORT_RX_WORK */
#if CFG_SUPPORT_RX_NAPI_WORK
		prBoostInfo->i4RxNapiWorkCpu,
#endif /* CFG_SUPPORT_RX_NAPI_WORK */
#if CFG_SUPPORT_SKB_ALLOC_WORK
		prBoostInfo->eSkbAllocWorkCoreType,
#endif /* CFG_SUPPORT_SKB_ALLOC_WORK */
#if CFG_SUPPORT_TX_FREE_SKB_WORK
		prBoostInfo->eTxFreeSkbWorkCoreType,
#endif /* CFG_SUPPORT_TX_FREE_SKB_WORK */
		"");
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
		if (prBoostInfo->rCpuInfo.i4LittleCpuFreq ==
			UNDEFINED_CPU_FREQ)
			prBoostInfo->rCpuInfo.i4LittleCpuFreq = u4CpuFreq;

		if (prBoostInfo->rCpuInfo.i4BigCpuFreq ==
			UNDEFINED_CPU_FREQ)
			prBoostInfo->rCpuInfo.i4BigCpuFreq = u4CpuFreq;
	}
}

static void __kalBoostCpuInit(struct ADAPTER *prAdapter)
{
	/* initially enable RPS working at small cores */
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
		DBGLOG(INIT, INFO, "TputLv:%u BoostLv[%u->%u]\n",
			u4TarPerfLevel, eCurrBoost, eNewBoost);
		kalTraceEvent("%s TputLv:%u BoostLv[%u->%u]\n", __func__,
			u4TarPerfLevel, eCurrBoost, eNewBoost);
		kalSetCpuBoost(prAdapter, &rBoostInfo[eNewBoost]);
		eCurrBoost = eNewBoost;
	}

	return 0;
}

int32_t kalBoostCpuPolicy(struct ADAPTER *prAdapter)
{
	struct PERF_MONITOR *prPerMonitor = &prAdapter->rPerMonitor;
	static struct BOOST_INFO rPolicyBoostInfo;
	static enum ENUM_CPU_BOOST_STATUS ePolicyBoost;
	static unsigned long ulMeanTput, ulPrevTxCnt, ulPrevTxFailCnt;
	static uint8_t ucTriggerCnt, ucOutOfMarginCnt;
	unsigned long ulCurrTxCnt, ulCurrTxFailCnt, ulTput;
	uint32_t u4NewBigCpuFreq = 0;
	uint32_t u4NewLitteCpuFreq = 0;
	uint32_t u4BigCpuFreq, u4LitteCpuFreq;
	uint32_t u4PER;

	if (prAdapter->rWifiVar.fgBoostCpuPolicyEn == FEATURE_DISABLED)
		return 0;

	if (rBoostInfo[eCurrBoost].rCpuInfo.i4BigCpuFreq
		== AUTO_CPU_FREQ
		&& rBoostInfo[eCurrBoost].rCpuInfo.i4LittleCpuFreq
			== AUTO_CPU_FREQ) {
		prPerMonitor->fgPolicyReady = FALSE;
		goto reset;
	}

	ulCurrTxCnt = GLUE_GET_REF_CNT(
		prAdapter->rHifStats.u4DataMsduRptCount);
	ulCurrTxFailCnt = prAdapter->rMsduReportStats.rCounting.u4TxFail;

	u4PER = (ulCurrTxCnt - ulPrevTxCnt) == 0 ? 0 :
			(1000 * (ulCurrTxFailCnt - ulPrevTxFailCnt)) /
				(ulCurrTxCnt - ulPrevTxCnt);

	ulPrevTxCnt = ulCurrTxCnt;
	ulPrevTxFailCnt = ulCurrTxFailCnt;
	DBGLOG(INIT, TRACE, "PerErrorRate=%u.%1u%%, Tx Total:Fail[%u:%u]",
			u4PER / 10, u4PER % 10,
			ulCurrTxCnt, ulCurrTxFailCnt);

	/*shift for int claculation, e.g u4PerErrorRate = 10 => PER = 1% */
	if (u4PER > 10) {
		DBGLOG(INIT, TRACE, "PerErrorRate=%u.%1u%% > 1%",
			u4PER / 10, u4PER % 10);
	}

	if (ePolicyBoost != eCurrBoost
		&& (eCurrBoost == ENUM_CPU_BOOST_STATUS_LV0
			|| !kalCheckBoostCpuMargin(prAdapter))) {
		DBGLOG(INIT, TRACE,
				"Boost new:Curr[%u:%u], PolicyReady:FALSE ",
				ePolicyBoost,
				eCurrBoost);
		ePolicyBoost = eCurrBoost;
		prPerMonitor->fgPolicyReady = FALSE;
		kalMemCopy(&rPolicyBoostInfo, &rBoostInfo[ePolicyBoost],
			sizeof(struct BOOST_INFO));
		goto reset;
	}

	ucTriggerCnt++;
	ulTput = prPerMonitor->ulThroughput >> 20;
	ulMeanTput = (ulMeanTput == 0)
		? ulTput : (ulMeanTput >> 1) + (ulTput >> 1);

	/* If Stable Tput isn't initialized, fgPolicyReady = FALSE
	 * and calculate avg 10 times Tput and padding offset
	 * as Stable Tput.
	 */
	if (!prPerMonitor->fgPolicyReady) {
		if (ucTriggerCnt >= 10) {
			prPerMonitor->ulStableTput =
				kal_div_u64(ulMeanTput * 95, 100);
			prPerMonitor->fgPolicyReady = TRUE;
			DBGLOG(INIT, TRACE, "PolicyReady: True, StableTput: %u",
				prPerMonitor->ulStableTput);
			goto reset;
		}
		return 0;
	}

	if (!kalCheckBoostCpuMargin(prAdapter)) {
		ucOutOfMarginCnt++;
		if (ucOutOfMarginCnt > 5)
			prPerMonitor->fgPolicyReady = FALSE;
	} else {
		ucOutOfMarginCnt = 0;
	}

	if (ucTriggerCnt < 5)
		return 0;

	if (ulMeanTput > prPerMonitor->ulStableTput) {
		if (rPolicyBoostInfo.rCpuInfo.i4BigCpuFreq) {
			u4BigCpuFreq =
				rPolicyBoostInfo.rCpuInfo.i4BigCpuFreq
					- 200000;
			u4NewBigCpuFreq =
				(u4BigCpuFreq < BIG_CPU_FREQ_MIN)
				? BIG_CPU_FREQ_MIN : u4BigCpuFreq;
		}
		if (rPolicyBoostInfo.rCpuInfo.i4LittleCpuFreq) {
			u4LitteCpuFreq =
				rPolicyBoostInfo.rCpuInfo.i4LittleCpuFreq
					- 200000;
			u4NewLitteCpuFreq =
				(u4LitteCpuFreq < LITTLE_CPU_FREQ_MIN)
				? LITTLE_CPU_FREQ_MIN : u4LitteCpuFreq;
		}
	} else {
		if (rPolicyBoostInfo.rCpuInfo.i4BigCpuFreq) {
			u4BigCpuFreq = rPolicyBoostInfo.rCpuInfo.i4BigCpuFreq
				+ 200000;
			u4NewBigCpuFreq = (u4BigCpuFreq > BIG_CPU_FREQ_MAX)
				? BIG_CPU_FREQ_MAX : u4BigCpuFreq;
		}
		if (rPolicyBoostInfo.rCpuInfo.i4LittleCpuFreq) {
			u4LitteCpuFreq =
				rPolicyBoostInfo.rCpuInfo.i4LittleCpuFreq
					+ 200000;
			u4NewLitteCpuFreq =
				(u4LitteCpuFreq > LITTLE_CPU_FREQ_MAX)
				? LITTLE_CPU_FREQ_MAX : u4LitteCpuFreq;
		}
	}
	DBGLOG(INIT, LOUD,
		"Tput[Mean:Curr:Th][%u:%u:%u] Freq[B][L][%u->%u][%u->%u]",
		(unsigned long) (ulMeanTput),
		(unsigned long) (ulTput),
		prPerMonitor->ulStableTput,
		rPolicyBoostInfo.rCpuInfo.i4BigCpuFreq,
		u4NewBigCpuFreq,
		rPolicyBoostInfo.rCpuInfo.i4LittleCpuFreq,
		u4NewLitteCpuFreq);

	rPolicyBoostInfo.rCpuInfo.i4BigCpuFreq = u4NewBigCpuFreq;
	rPolicyBoostInfo.rCpuInfo.i4LittleCpuFreq = u4NewLitteCpuFreq;
	kalSetCpuBoost(prAdapter, &rPolicyBoostInfo);

reset:
	ucTriggerCnt = 0;
	ulMeanTput = 0;

	return 0;
}

#if CFG_SUPPORT_MCC_BOOST_CPU
u_int8_t kalIsMccBoost(struct ADAPTER *prAdapter)
{
	return FALSE;
}
#endif /* CFG_SUPPORT_MCC_BOOST_CPU */
#endif

uint32_t kalGetEmiMetOffset(void)
{
	return u4EmiMetOffset;
}

uint32_t kalGetProjectId(void)
{
	return u4ProjectId;
}

void kalSetEmiMetOffset(uint32_t newEmiMetOffset)
{
	u4EmiMetOffset = newEmiMetOffset;
}

void kalDumpPlatGPIOStat(void)
{
	DBGLOG(INIT, INFO, "GPIO 244, val=%d\n",
		gpio_get_value(512 + 244));
	DBGLOG(INIT, INFO, "GPIO 248, val=%d\n",
		gpio_get_value(512 + 248));
	DBGLOG(INIT, INFO, "GPIO 249, val=%d\n",
		gpio_get_value(512 + 249));
}

#ifdef CONFIG_WLAN_MTK_EMI
void kalSetEmiMpuProtection(phys_addr_t emiPhyBase, bool enable)
{
}

void kalSetDrvEmiMpuProtection(phys_addr_t emiPhyBase, uint32_t offset,
			       uint32_t size)
{
#if KERNEL_VERSION(6, 0, 0) >= LINUX_VERSION_CODE
	struct emimpu_region_t region;
	unsigned long long start = emiPhyBase + offset;
	unsigned long long end = emiPhyBase + offset + size - 1;
	int ret;

	DBGLOG(INIT, INFO, "emiPhyBase: 0x%p, offset: %d, size: %d\n",
				emiPhyBase, offset, size);

	ret = mtk_emimpu_init_region(&region, 18);
	if (ret) {
		DBGLOG(INIT, ERROR, "mtk_emimpu_init_region failed, ret: %d\n",
				ret);
		return;
	}
	mtk_emimpu_set_addr(&region, start, end);
	mtk_emimpu_set_apc(&region, DOMAIN_AP, MTK_EMIMPU_NO_PROTECTION);
	mtk_emimpu_set_apc(&region, DOMAIN_CONN, MTK_EMIMPU_NO_PROTECTION);
	mtk_emimpu_lock_region(&region, MTK_EMIMPU_LOCK);
	ret = mtk_emimpu_set_protection(&region);
	if (ret)
		DBGLOG(INIT, ERROR,
			"mtk_emimpu_set_protection failed, ret: %d\n",
			ret);
	mtk_emimpu_free_region(&region);
#endif
}
#endif

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

#if defined(CFG_MTK_WIFI_CONNV3_SUPPORT)
int32_t kalPlatOpsInit(void)
{
#if defined(MT6653)
	struct mt66xx_hif_driver_data *driver_data =
		&mt66xx_driver_data_mt6653;
	struct mt66xx_chip_info *chip = driver_data->chip_info;

	chip->pinctrl_ops = &mt6991_pinctrl_ops;
#endif

	return 0;
}

static int32_t mt6991_wlan_pinctrl_init(struct mt66xx_chip_info *chip_info)
{
	struct platform_device *pdev;
	int32_t ret = 0;

	kalGetPlatDev(&pdev);
	if (!pdev) {
		DBGLOG(INIT, ERROR, "NULL platform_device\n");
		ret = -EINVAL;
		goto exit;
	}

	pinctrl_ptr = devm_pinctrl_get(&pdev->dev);

	if (KAL_IS_ERR(pinctrl_ptr)) {
		ret = PTR_ERR(pinctrl_ptr);
		DBGLOG(INIT, ERROR,
			"devm_pinctrl_get failed, ret=%d.\n",
			ret);
		goto exit;
	}

	last_toggle_time = 0;
	last_wf_rst_pin_state = -1;

exit:
	return ret;
}

static void ensure_rst_pin_min_wait_time(int8_t state)
{
	OS_SYSTIME current_time = 0;
	uint32_t retry = 0;

	if (last_wf_rst_pin_state == state)
		return;

	while (TRUE) {
		GET_CURRENT_SYSTIME(&current_time);

		if (last_toggle_time == 0)
			break;
		else if (CHECK_FOR_TIMEOUT(current_time,
					   last_toggle_time,
					   RST_PIN_MIN_WAIT_TIME))
			break;

		DBGLOG_LIMITED(INIT, LOUD, "retry: %d.\n", retry);
		retry++;
		kalMdelay(1);
	}
	GET_CURRENT_SYSTIME(&last_toggle_time);
	last_wf_rst_pin_state = state;
}

static int32_t mt6991_wlan_pinctrl_action(struct mt66xx_chip_info *chip_info,
	enum WLAN_PINCTRL_MSG msg)
{
	struct pinctrl_state *pinctrl;
	uint8_t *name;
	int32_t ret = 0;

	if (!pinctrl_ptr || KAL_IS_ERR(pinctrl_ptr)) {
		ret = -EINVAL;
		goto exit;
	}

	switch (msg) {
	case WLAN_PINCTRL_MSG_FUNC_ON:
		name = "wf_rst_on";
		ensure_rst_pin_min_wait_time(1);
		break;
	case WLAN_PINCTRL_MSG_FUNC_OFF:
		name = "wf_rst_off";
		ensure_rst_pin_min_wait_time(0);
		break;
	case WLAN_PINCTRL_MSG_FUNC_PTA_UART_INIT:
		name = "wf_rst_pta_uart_init";
		break;
	case WLAN_PINCTRL_MSG_FUNC_PTA_UART_ON:
		name = "wf_rst_pta_uart_on";
		break;
	case WLAN_PINCTRL_MSG_FUNC_PTA_UART_OFF:
		name = "wf_rst_pta_uart_off";
		break;
	default:
		DBGLOG(INIT, ERROR,
			"Unknown msg: %d.\n",
			msg);
		ret = -EINVAL;
		goto exit;
	}

	pinctrl = pinctrl_lookup_state(pinctrl_ptr, name);

	if (KAL_IS_ERR(pinctrl)) {
		ret = PTR_ERR(pinctrl);
		DBGLOG(INIT, ERROR,
			"pinctrl_lookup_state %s, ret=%d.\n",
			name,
			ret);
		goto exit;
	}

	ret = pinctrl_select_state(pinctrl_ptr, pinctrl);
	DBGLOG(INIT, INFO,
		"pinctrl_select_state msg: %d, ret: %d.\n",
		msg, ret);

exit:
	return ret;
}
#endif

int32_t kalGetScpDumpInfo(u64 *addr, unsigned int *size)
{
	struct device_node *scp_node = NULL;

	scp_node = of_find_compatible_node(NULL, NULL,
		"mediatek,mt6991-conn_scp");
	if (!scp_node) {
		DBGLOG(INIT, ERROR, "kernel option CONFIG_OF not enabled.\n");
		return -EINVAL;
	}

	if (of_property_read_u64(scp_node, "dfd-value-addr", addr))
		return -EINVAL;

	if (of_property_read_u32(scp_node, "dfd-value-size", size))
		return -EINVAL;

	return 0;
}

#if CFG_MTK_WIFI_PCIE_SR
u_int8_t kalIsSupportPcieL2(void)
{
	return TRUE;
}
#endif

#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
u_int8_t kalIsSupportMawd(void)
{
	return FALSE;
}

u_int8_t kalIsSupportSdo(void)
{
	return FALSE;
}

u_int8_t kalIsSupportRro(void)
{
	return FALSE;
}

uint32_t kalGetMawdVer(void)
{
	return MAWD_VER_1_1;
}

uint32_t kalGetConnInfraId(void)
{
	return CONN_INFRA_ID;
}
#endif

uint32_t kalGetTxBigCpuMask(void)
{
	return TX_CPU_BIG_CORE;
}

#if (CFG_VOLT_INFO == 1)
uint8_t kalVnfGetEnInitStatus(void)
{
	return FEATURE_ENABLED;
}

uint32_t kalVnfGetVoltLowBnd(void)
{
	return VOLT_INFO_LOW_BOUND;
}
#endif /* #if (CFG_VOLT_INFO == 1) */
