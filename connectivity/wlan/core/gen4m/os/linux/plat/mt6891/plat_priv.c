// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "gl_os.h"

#if KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
#include <uapi/linux/sched/types.h>
#include <linux/sched/task.h>
#include <linux/cpufreq.h>
#elif KERNEL_VERSION(4, 19, 0) <= CFG80211_VERSION_CODE
#include <cpu_ctrl.h>
#include <topo_ctrl.h>
#include <linux/soc/mediatek/mtk-pm-qos.h>
#include <helio-dvfsrc-opp.h>
#define pm_qos_add_request(_req, _class, _value) \
		mtk_pm_qos_add_request(_req, _class, _value)
#define pm_qos_update_request(_req, _value) \
		mtk_pm_qos_update_request(_req, _value)
#define pm_qos_remove_request(_req) \
		mtk_pm_qos_remove_request(_req)
#define pm_qos_request mtk_pm_qos_request
#define PM_QOS_DDR_OPP MTK_PM_QOS_DDR_OPP
#define ppm_limit_data cpu_ctrl_data
#else
#include <cpu_ctrl.h>
#include <topo_ctrl.h>
#include <linux/pm_qos.h>
#include <helio-dvfsrc-opp-mt6885.h>
#endif

#include "precomp.h"

#if IS_ENABLED(CONFIG_MTK_EMI_LEGACY)
#include <soc/mediatek/emi.h>
#define	REGION_WIFI	26
#define WIFI_EMI_MEM_SIZE      0x140000
#define WIFI_EMI_MEM_OFFSET    0x2B0000
#define	DOMAIN_AP	0
#define	DOMAIN_CONN	2
#endif

#define DEFAULT_CPU_FREQ (0)
#define MAX_CPU_FREQ (3 * 1024 * 1024) /* in kHZ */
#define MAX_CLUSTER_NUM  3
#define CPU_ALL_CORE (0xff)
#define CPU_BIG_CORE (0xf0)
#define CPU_LITTLE_CORE (CPU_ALL_CORE - CPU_BIG_CORE)

#define CONNSYS_VERSION_ID  0x20010101

enum ENUM_CPU_BOOST_STATUS {
	ENUM_CPU_BOOST_STATUS_INIT = 0,
	ENUM_CPU_BOOST_STATUS_START,
	ENUM_CPU_BOOST_STATUS_STOP,
	ENUM_CPU_BOOST_STATUS_NUM
};

static uint32_t u4EmiMetOffset = 0x247000;

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

void kalSetTaskUtilMinPct(int pid, unsigned int min)
{
#if KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
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
#elif KERNEL_VERSION(4, 19, 0) <= CFG80211_VERSION_CODE
	/* TODO */
#else
	set_task_util_min_pct(pid, min);
#endif
}

#if KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
static LIST_HEAD(wlan_policy_list);
struct wlan_policy {
	struct freq_qos_request	qos_req;
	struct list_head	list;
	int cpu;
};
#endif

void kalSetCpuFreq(int32_t freq)
{
#if KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
	int cpu, ret;
	struct cpufreq_policy *policy;
	struct wlan_policy *wReq;

	if (freq < 0)
		freq = DEFAULT_CPU_FREQ;

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
				&wReq->qos_req, FREQ_QOS_MIN, DEFAULT_CPU_FREQ);
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
		ret = freq_qos_update_request(&wReq->qos_req, freq);
		if (ret < 0) {
			DBGLOG(INIT, INFO,
				"freq_qos_update_request fail cpu%d freq=%d ret=%d\n",
				wReq->cpu, freq, ret);
		}
	}
#else
	int32_t i = 0;
	struct ppm_limit_data *freq_to_set;
	uint32_t u4ClusterNum = topo_ctrl_get_nr_clusters();

	freq_to_set = kmalloc_array(u4ClusterNum, sizeof(struct ppm_limit_data),
			GFP_KERNEL);
	if (!freq_to_set)
		return;

	for (i = 0; i < u4ClusterNum; i++) {
		freq_to_set[i].min = freq;
		freq_to_set[i].max = freq;
	}

	update_userlimit_cpu_freq(CPU_KIR_WIFI,
		u4ClusterNum, freq_to_set);

	kfree(freq_to_set);
#endif
}

void kalSetDramBoost(struct ADAPTER *prAdapter, int32_t iLv)
{
#if KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
	/* TODO */
#else
	static struct pm_qos_request wifi_qos_request;

	KAL_ACQUIRE_MUTEX(prAdapter, MUTEX_BOOST_CPU);
	if (iLv != -1) {
		pr_info("Max Dram Freq start\n");
		pm_qos_add_request(&wifi_qos_request,
				   PM_QOS_DDR_OPP,
				   DDR_OPP_2);
		pm_qos_update_request(&wifi_qos_request, DDR_OPP_2);
	} else {
		pr_info("Max Dram Freq end\n");
		pm_qos_update_request(&wifi_qos_request, DDR_OPP_UNREQ);
		pm_qos_remove_request(&wifi_qos_request);
	}
	KAL_RELEASE_MUTEX(prAdapter, MUTEX_BOOST_CPU);
#endif
}

int32_t kalBoostCpu(struct ADAPTER *prAdapter,
		    uint32_t u4TarPerfLevel,
		    uint32_t u4BoostCpuTh)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4Freq = -1;
	static u_int8_t fgRequested = ENUM_CPU_BOOST_STATUS_INIT;

	WIPHY_PRIV(wlanGetWiphy(), prGlueInfo);
	i4Freq = (u4TarPerfLevel >= u4BoostCpuTh) ? MAX_CPU_FREQ : -1;

	if (fgRequested == ENUM_CPU_BOOST_STATUS_INIT) {
		/* initially enable rps working at small cores */
		kalSetRpsMap(prGlueInfo, CPU_LITTLE_CORE);
		fgRequested = ENUM_CPU_BOOST_STATUS_STOP;
	}

	if (u4TarPerfLevel >= u4BoostCpuTh) {
		if (fgRequested == ENUM_CPU_BOOST_STATUS_STOP) {
			pr_info("%s start (%d>=%d)\n",
				__func__, u4TarPerfLevel, u4BoostCpuTh);
			fgRequested = ENUM_CPU_BOOST_STATUS_START;

			kalSetTaskUtilMinPct(prGlueInfo->u4TxThreadPid, 100);
			kalSetTaskUtilMinPct(prGlueInfo->u4RxThreadPid, 100);
			kalSetTaskUtilMinPct(prGlueInfo->u4HifThreadPid, 100);
			kalSetRpsMap(prGlueInfo, CPU_BIG_CORE);
			kalSetCpuFreq(i4Freq);
			kalSetDramBoost(prAdapter, TRUE);
		}
	} else {
		if (fgRequested == ENUM_CPU_BOOST_STATUS_START) {
			pr_info("%s stop (%d<%d)\n",
				__func__, u4TarPerfLevel, u4BoostCpuTh);
			fgRequested = ENUM_CPU_BOOST_STATUS_STOP;

			kalSetTaskUtilMinPct(prGlueInfo->u4TxThreadPid, 0);
			kalSetTaskUtilMinPct(prGlueInfo->u4RxThreadPid, 0);
			kalSetTaskUtilMinPct(prGlueInfo->u4HifThreadPid, 0);
			kalSetRpsMap(prGlueInfo, CPU_LITTLE_CORE);
			kalSetCpuFreq(i4Freq);
			kalSetDramBoost(prAdapter, FALSE);
		}
	}
	kalTraceInt(fgRequested == ENUM_CPU_BOOST_STATUS_START, "kalBoostCpu");

	return 0;
}

uint32_t kalGetEmiMetOffset(void)
{
	return u4EmiMetOffset;
}

void kalSetEmiMetOffset(uint32_t newEmiMetOffset)
{
	u4EmiMetOffset = newEmiMetOffset;
}

#if IS_ENABLED(CONFIG_MTK_EMI_LEGACY)
void kalSetEmiMpuProtection(phys_addr_t emiPhyBase, bool enable)
{
}

void kalSetDrvEmiMpuProtection(phys_addr_t emiPhyBase, uint32_t offset,
			       uint32_t size)
{
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
}
#endif

int32_t kalGetFwFlavorByPlat(uint8_t *flavor)
{
#if (CFG_WLAN_CONNAC3_DEV == 1)
	*flavor = 'v';
#else
	*flavor = 'a';
#endif
	return 1;
}

int32_t kalGetConnsysVerId(void)
{
	return CONNSYS_VERSION_ID;
}

