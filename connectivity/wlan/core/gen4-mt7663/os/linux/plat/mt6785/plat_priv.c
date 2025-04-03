/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <cpu_ctrl.h>
#include <topo_ctrl.h>
#ifdef WLAN_FORCE_DDR_OPP
#include <linux/pm_qos.h>
#endif

#include "precomp.h"

#ifdef CONFIG_MTK_EMI
#include <mt_emi_api.h>
#define WIFI_EMI_MEM_OFFSET    0x140000
#define WIFI_EMI_MEM_SIZE      0x130000
#endif


#define MAX_CPU_FREQ (3 * 1024 * 1024) /* in kHZ */
#define MAX_CLUSTER_NUM  3


uint32_t kalGetCpuBoostThreshold(void)
{
	DBGLOG(SW4, TRACE, "enter kalGetCpuBoostThreshold\n");
	/*  3, stands for 100Mbps */
	return 3;
}

int32_t kalBoostCpu(IN struct ADAPTER *prAdapter,
		    IN uint32_t u4TarPerfLevel,
		    IN uint32_t u4BoostCpuTh)
{
	struct ppm_limit_data freq_to_set[MAX_CLUSTER_NUM];
	int32_t i = 0, i4Freq = -1;
#ifdef WLAN_FORCE_DDR_OPP
	static struct pm_qos_request wifi_qos_request;
	static u_int8_t fgRequested;
#endif
	uint32_t u4ClusterNum = topo_ctrl_get_nr_clusters();

	ASSERT(u4ClusterNum <= MAX_CLUSTER_NUM);
	/* ACAO, we dont have to set core number */
	i4Freq = (u4TarPerfLevel >= u4BoostCpuTh) ? MAX_CPU_FREQ : -1;
	for (i = 0; i < u4ClusterNum; i++) {
		freq_to_set[i].min = i4Freq;
		freq_to_set[i].max = i4Freq;
	}

	update_userlimit_cpu_freq(CPU_KIR_WIFI, u4ClusterNum, freq_to_set);

#ifdef WLAN_FORCE_DDR_OPP
	if (u4TarPerfLevel >= u4BoostCpuTh) {
		if (!fgRequested) {
			fgRequested = 1;
			pm_qos_add_request(&wifi_qos_request,
					   PM_QOS_DDR_OPP,
					   DDR_OPP_0);
		}
		pm_qos_update_request(&wifi_qos_request, DDR_OPP_0);
	} else if (fgRequested) {
		pm_qos_update_request(&wifi_qos_request, DDR_OPP_UNREQ);
		pm_qos_remove_request(&wifi_qos_request);
		fgRequested = 0;
	}
#endif
	return 0;
}

#ifdef CONFIG_MTK_EMI
void kalSetEmiMpuProtection(phys_addr_t emiPhyBase, bool enable)
{
}

void kalSetDrvEmiMpuProtection(phys_addr_t emiPhyBase, uint32_t offset,
			       uint32_t size)
{
	struct emi_region_info_t region_info;

	DBGLOG(INIT, INFO, "emiPhyBase: 0x%x, offset: %u, size: %u\n",
			emiPhyBase, offset, size);

	/*set MPU for EMI share Memory */
	region_info.start = emiPhyBase + offset;
	region_info.end = emiPhyBase + offset + size - 1;
	region_info.region = 18;
	SET_ACCESS_PERMISSION(region_info.apc, LOCK, FORBIDDEN, FORBIDDEN,
			      FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN,
			      FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN,
			      FORBIDDEN, FORBIDDEN, FORBIDDEN, NO_PROTECTION,
			      FORBIDDEN, NO_PROTECTION);
	emi_mpu_set_protection(&region_info);
}
#endif

int32_t kalGetFwFlavor(uint8_t *flavor)
{
	*flavor = 'a';
	return 1;
}
