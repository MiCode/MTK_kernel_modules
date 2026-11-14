// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2025 MediaTek Inc.
 */

#if CFG_MTK_ANDROID_WMT

#include "gl_plat.h"

#if KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
#include <uapi/linux/sched/types.h>
#include <linux/sched/task.h>
#include <linux/cpufreq.h>
/* for dram boost */
#include "dvfsrc-exp.h"
#include <linux/interconnect.h>
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
#include <helio-dvfsrc-opp.h>
#include <helio-dvfsrc-opp-mt6885.h>
#endif

#define OPP_BW_MAX_NUM 9

static LIST_HEAD(wlan_policy_list);
struct wlan_policy {
	struct freq_qos_request	qos_req;
	struct list_head	list;
	int cpu;
};

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

	/* sched_setattr_nocheck */
	if (likely(p)) {
		ret = sched_setattr_nocheck(p, &attr);
		if (ret < 0) {
			DBGLOG(INIT, ERROR,
				"sched_setattr_nocheck pid[%u] min[%u] fail\n",
				pid, min);
		}
		put_task_struct(p);
	}
#elif KERNEL_VERSION(4, 19, 0) <= CFG80211_VERSION_CODE
	/* TODO */
#else
	set_task_util_min_pct(pid, min);
#endif
}

void kalSetCpuFreq(int32_t freq, uint32_t set_mask)
{
#if KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
	int cpu, ret;
	struct cpufreq_policy *policy;
	struct wlan_policy *wReq, *prNext;
	u_int8_t fgFail = FALSE;

	if (freq < 0)
		freq = AUTO_CPU_FREQ;

	if (list_empty(&wlan_policy_list)) {
		for_each_possible_cpu(cpu) {
			policy = cpufreq_cpu_get(cpu);
			if (!policy)
				continue;

			wReq = kzalloc(sizeof(struct wlan_policy), GFP_KERNEL);
			if (!wReq) {
				fgFail = TRUE;
				goto end;
			}
			wReq->cpu = cpu;

			ret = freq_qos_add_request(&policy->constraints,
				&wReq->qos_req, FREQ_QOS_MIN, AUTO_CPU_FREQ);
			if (ret < 0) {
				DBGLOG(INIT, DEBUG,
					"freq_qos_add_request fail cpu%d ret=%d\n",
					wReq->cpu, ret);
				kfree(wReq);
				fgFail = TRUE;
				goto end;
			}

			list_add_tail(&wReq->list, &wlan_policy_list);
end:
			cpufreq_cpu_put(policy);
			if (fgFail == TRUE)
				break;
		}

		if (fgFail == TRUE) {
			list_for_each_entry_safe(wReq, prNext,
				&wlan_policy_list, list) {
				freq_qos_remove_request(&wReq->qos_req);
				list_del(&wReq->list);
				kfree(wReq);
			}
		}
	}

	list_for_each_entry(wReq, &wlan_policy_list, list) {
		if (!((0x1 << wReq->cpu) & set_mask))
			continue;

		ret = freq_qos_update_request(&wReq->qos_req, freq);
		if (ret < 0) {
			DBGLOG(INIT, DEBUG,
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

uint32_t __weak kalGetDramBwMaxIdx(void)
{
	return 0;
}

void kalSetDramBoost(struct ADAPTER *prAdapter, int32_t iLv)
{
#if KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
	struct platform_device *pdev;
#ifdef CONFIG_OF
	struct device_node *node;
	static struct icc_path *bw_path;
#endif /* CONFIG_OF */
	static unsigned int peak_bw[OPP_BW_MAX_NUM] = {0}, current_bw;
	unsigned int prev_bw = 0;
	uint32_t u4MaxIdx, i;

	u4MaxIdx = kalGetDramBwMaxIdx();
	if (u4MaxIdx == 0 || u4MaxIdx > OPP_BW_MAX_NUM)
		return;

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
		for (i = 0; i < u4MaxIdx; i++) {
			peak_bw[i] = dvfsrc_get_required_opp_peak_bw(node, i);
			if (peak_bw[i] == 0) {
				DBGLOG(INIT, INFO, "i:%u bw:%u\n", i,
					peak_bw[i]);
				break;
			}
		}
#endif /* CONFIG_MTK_DVFSRC */
#endif /* CONFIG_OF */
	}

	if (!IS_ERR(bw_path)) {
		prev_bw = current_bw;

		if (iLv != -1 && iLv < u4MaxIdx)
			current_bw = peak_bw[iLv];
		else
			current_bw = 0;

		icc_set_bw(bw_path, 0, current_bw);
		DBGLOG(INIT, DEBUG, "[%d] bw %u => %u\n",
			iLv, prev_bw, current_bw);
	}
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

int kalSetCpuMask(struct task_struct *task, uint32_t set_mask)
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
	DBGLOG(INIT, TRACE, "set_cpus_allowed_ptr()=%d", r);
#endif
	return r;
}
#endif /* CFG_MTK_ANDROID_WMT */
