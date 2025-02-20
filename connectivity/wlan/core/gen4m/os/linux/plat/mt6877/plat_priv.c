// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "gl_os.h"

#if KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
#include <uapi/linux/sched/types.h>
#include <linux/sched/task.h>
#include <linux/cpufreq.h>
#endif
#include <linux/pm_qos.h>
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

#include <linux/mfd/mt6359/registers.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/mfd/mt6397/core.h>
#include <linux/of.h>
#include <linux/of_platform.h>

#define DEFAULT_CPU_FREQ (0)
#define CPU_ALL_CORE (0xff)
#define CPU_BIG_CORE (0xf0)
#define CPU_X_CORE (0x80)
#define CPU_HP_CORE (CPU_BIG_CORE - CPU_X_CORE)
#define CPU_LITTLE_CORE (CPU_ALL_CORE - CPU_BIG_CORE)

#define RPS_ALL_CORE (CPU_ALL_CORE - 0x11)
#define RPS_BIG_CORE (CPU_BIG_CORE - 0x10)
#define RPS_LITTLE_CORE (CPU_LITTLE_CORE - 0x01)

/* Used to get address of saving fw version offset.               */
/* EMI_base + MCU_EMI_LOG offset(0xB00000) + fw ver offset(0x24). */
#define FW_VERSION_OFFSET_ADDRESS	0xB00024

#define MT6359_PMIC_REG_BASE		((unsigned int)(0x0))
#define MT6359_LDO_VFE28_OP_EN_SET	(MT6359_PMIC_REG_BASE+0x1b90)
#define MT6359_LDO_VFE28_OP_EN_CLR	(MT6359_PMIC_REG_BASE+0x1b92)
#define MT6359_LDO_VFE28_OP_CFG_CLR	(MT6359_PMIC_REG_BASE+0x1b98)

#if (KERNEL_VERSION(5, 10, 0) <= CFG80211_VERSION_CODE)
#include <linux/regulator/consumer.h>
#endif
#include <linux/platform_device.h>
#include <linux/pinctrl/consumer.h>
#include "wlan_pinctrl.h"

enum ENUM_CPU_BOOST_STATUS {
	ENUM_CPU_BOOST_STATUS_INIT = 0,
#if CFG_SUPPORT_LITTLE_CPU_BOOST
	ENUM_CPU_BOOST_STATUS_START_LITTLE,
#endif /* CFG_SUPPORT_LITTLE_CPU_BOOST */
	ENUM_CPU_BOOST_STATUS_START_ALL,
	ENUM_CPU_BOOST_STATUS_STOP,
	ENUM_CPU_BOOST_STATUS_NUM
};

static uint32_t u4EmiMetOffset = 0x45D400;
static struct regmap *g_regmap;

#if CFG_SUPPORT_LITTLE_CPU_BOOST
uint32_t kalGetLittleCpuBoostThreshold(void)
{
	DBGLOG(SW4, TRACE, "Get Little CPU Boost Threshold\n");
	/* 3, stands for 100Mbps */
	return 3;
}
#endif /* CFG_SUPPORT_LITTLE_CPU_BOOST */

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
	/* TODO */
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

int32_t kalBoostCpu(struct ADAPTER *prAdapter,
		    uint32_t u4TarPerfLevel,
		    uint32_t u4BoostCpuTh)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	static u_int8_t fgRequested = ENUM_CPU_BOOST_STATUS_INIT;
	uint32_t u4CpuFreq = prAdapter->rWifiVar.au4CpuBoostMinFreq * 1000;
#if CFG_SUPPORT_LITTLE_CPU_BOOST
	uint32_t u4BoostLittleCpuTh = prAdapter->rWifiVar.u4BoostLittleCpuTh;
#endif /* CFG_SUPPORT_LITTLE_CPU_BOOST */

	WIPHY_PRIV(wlanGetWiphy(), prGlueInfo);

	if (fgRequested == ENUM_CPU_BOOST_STATUS_INIT) {
		/* initially enable rps working at small cores */
		kalSetRpsMap(prGlueInfo, RPS_LITTLE_CORE);
		kalSetISRMask(prAdapter, CPU_LITTLE_CORE);
		fgRequested = ENUM_CPU_BOOST_STATUS_STOP;
	}

#if CFG_SUPPORT_LITTLE_CPU_BOOST
	if (u4TarPerfLevel >= u4BoostLittleCpuTh &&
		u4TarPerfLevel < u4BoostCpuTh &&
		fgRequested == ENUM_CPU_BOOST_STATUS_STOP) {
		pr_info("Boost little (%u>=%u) freq=%u\n",
			u4TarPerfLevel, u4BoostLittleCpuTh, u4CpuFreq/1000);
		fgRequested = ENUM_CPU_BOOST_STATUS_START_LITTLE;

		kalSetCpuFreq(u4CpuFreq, CPU_LITTLE_CORE);
		kalSetCpuMask(prGlueInfo->hif_thread, CPU_ALL_CORE);
		kalSetCpuMask(prGlueInfo->main_thread, CPU_ALL_CORE);
		kalSetCpuMask(prGlueInfo->rx_thread, CPU_ALL_CORE);

		kalSetRpsMap(prGlueInfo, RPS_LITTLE_CORE);
		kalSetISRMask(prAdapter, CPU_ALL_CORE);
		kalSetDramBoost(prAdapter, TRUE);
	} else if (u4TarPerfLevel >= u4BoostCpuTh &&
		(fgRequested == ENUM_CPU_BOOST_STATUS_STOP ||
		 fgRequested == ENUM_CPU_BOOST_STATUS_START_LITTLE)) {
#else /* CFG_SUPPORT_LITTLE_CPU_BOOST */
	if (u4TarPerfLevel >= u4BoostCpuTh &&
		fgRequested == ENUM_CPU_BOOST_STATUS_STOP) {
#endif /* CFG_SUPPORT_LITTLE_CPU_BOOST */
		pr_info("Boost and migrate big (%u>=%u) freq=%u\n",
			u4TarPerfLevel, u4BoostCpuTh, u4CpuFreq/1000);
		fgRequested = ENUM_CPU_BOOST_STATUS_START_ALL;

		kalSetCpuFreq(u4CpuFreq, CPU_BIG_CORE);
		kalSetCpuMask(prGlueInfo->hif_thread, CPU_BIG_CORE);
		kalSetCpuMask(prGlueInfo->main_thread, CPU_BIG_CORE);
		kalSetCpuMask(prGlueInfo->rx_thread, CPU_BIG_CORE);
		kalSetCpuFreq(DEFAULT_CPU_FREQ, CPU_LITTLE_CORE);

		kalSetTaskUtilMinPct(prGlueInfo->u4TxThreadPid, 100);
		kalSetTaskUtilMinPct(prGlueInfo->u4RxThreadPid, 100);
		kalSetTaskUtilMinPct(prGlueInfo->u4HifThreadPid, 100);
		kalSetRpsMap(prGlueInfo, RPS_BIG_CORE);
		kalSetISRMask(prAdapter, CPU_BIG_CORE);
		kalSetDramBoost(prAdapter, TRUE);
#if CFG_SUPPORT_LITTLE_CPU_BOOST
	} else if (u4TarPerfLevel < u4BoostCpuTh &&
		u4TarPerfLevel >= u4BoostLittleCpuTh &&
		fgRequested == ENUM_CPU_BOOST_STATUS_START_ALL) {
		pr_info("%s stop big core (%u<%u)\n",
			__func__, u4TarPerfLevel, u4BoostCpuTh);
		fgRequested = ENUM_CPU_BOOST_STATUS_START_LITTLE;

		kalSetCpuMask(prGlueInfo->hif_thread, CPU_LITTLE_CORE);
		kalSetCpuMask(prGlueInfo->main_thread, CPU_LITTLE_CORE);
		kalSetCpuMask(prGlueInfo->rx_thread, CPU_LITTLE_CORE);

		kalSetTaskUtilMinPct(prGlueInfo->u4TxThreadPid, 0);
		kalSetTaskUtilMinPct(prGlueInfo->u4RxThreadPid, 0);
		kalSetTaskUtilMinPct(prGlueInfo->u4HifThreadPid, 0);

		kalSetRpsMap(prGlueInfo, RPS_LITTLE_CORE);
		kalSetISRMask(prAdapter, CPU_LITTLE_CORE);
		kalSetCpuFreq(DEFAULT_CPU_FREQ, CPU_BIG_CORE);
	} else if (u4TarPerfLevel < u4BoostLittleCpuTh &&
		(fgRequested == ENUM_CPU_BOOST_STATUS_START_ALL ||
		 fgRequested == ENUM_CPU_BOOST_STATUS_START_LITTLE)) {
#else /* CFG_SUPPORT_LITTLE_CPU_BOOST */
	} else if (u4TarPerfLevel < u4BoostCpuTh &&
		fgRequested == ENUM_CPU_BOOST_STATUS_START_ALL) {
#endif /* CFG_SUPPORT_LITTLE_CPU_BOOST */
		pr_info("%s stop all (%u<%u)\n",
			__func__,
			u4TarPerfLevel,
#if CFG_SUPPORT_LITTLE_CPU_BOOST
			u4BoostLittleCpuTh
#else
			u4BoostCpuTh
#endif /* CFG_SUPPORT_LITTLE_CPU_BOOST */
		);
		fgRequested = ENUM_CPU_BOOST_STATUS_STOP;

		kalSetCpuMask(prGlueInfo->hif_thread, CPU_ALL_CORE);
		kalSetCpuMask(prGlueInfo->main_thread, CPU_ALL_CORE);
		kalSetCpuMask(prGlueInfo->rx_thread, CPU_ALL_CORE);

		kalSetTaskUtilMinPct(prGlueInfo->u4TxThreadPid, 0);
		kalSetTaskUtilMinPct(prGlueInfo->u4RxThreadPid, 0);
		kalSetTaskUtilMinPct(prGlueInfo->u4HifThreadPid, 0);
		kalSetRpsMap(prGlueInfo, RPS_LITTLE_CORE);
		kalSetISRMask(prAdapter, CPU_ALL_CORE);
		kalSetCpuFreq(DEFAULT_CPU_FREQ, CPU_ALL_CORE);
		kalSetDramBoost(prAdapter, FALSE);
	}

	kalTraceInt(fgRequested == ENUM_CPU_BOOST_STATUS_START_ALL,
		"kalBoostCpu");

	return 0;
}
#endif

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
#if IS_ENABLED(CONFIG_MTK_EMI_LEGACY)
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

void kalPmicCtrl(u_int8_t fgIsEnabled)
{
#ifdef CONFIG_OF
	struct device_node *pmic_node;
	struct platform_device *pmic_pdev;
	struct mt6397_chip *chip;
	struct GLUE_INFO *prGlueInfo;

	WIPHY_PRIV(wlanGetWiphy(), prGlueInfo);
	if (prGlueInfo == NULL)
		return;

	if (g_regmap != NULL)
		goto skip_parse_phandle;

	pmic_node = of_parse_phandle(prGlueInfo->rHifInfo.pdev->dev.of_node,
					"pmic", 0);
	if (!pmic_node) {
		DBGLOG(INIT, ERROR, "Get pmic_node fail\n");
		return;
	}

	pmic_pdev = of_find_device_by_node(pmic_node);
	if (!pmic_pdev) {
		DBGLOG(INIT, ERROR, "Get pmic_pdev fail\n");
		return;
	}

	chip = dev_get_drvdata(&(pmic_pdev->dev));
	if (!chip) {
		DBGLOG(INIT, ERROR, "Get chip fail\n");
		return;
	}

	g_regmap = chip->regmap;
	if (IS_ERR_VALUE(g_regmap)) {
		g_regmap = NULL;
		DBGLOG(INIT, ERROR, "Get regmap fail\n");
	}
skip_parse_phandle:
#else
	DBGLOG(INIT, WARN, "CONFIG_OF not enabled.\n");
	return;
#endif
	if (fgIsEnabled) {
		regmap_write(g_regmap,
			MT6359_LDO_VFE28_OP_EN_SET, 0x1 << 8);
		regmap_write(g_regmap,
			MT6359_LDO_VFE28_OP_CFG_CLR, 0x1 << 8);
	} else {
		regmap_write(g_regmap,
			MT6359_LDO_VFE28_OP_EN_CLR, 0x1 << 8);
		regmap_write(g_regmap,
			MT6359_LDO_VFE28_OP_CFG_CLR, 0x1 << 8);
		regmap_write(g_regmap,
			MT6359_LDO_VFE28_OP_CFG_CLR, 0x1 << 8);
	}
}

