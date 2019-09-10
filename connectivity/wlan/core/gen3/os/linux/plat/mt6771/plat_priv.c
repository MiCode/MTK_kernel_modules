/*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See http://www.gnu.org/licenses/gpl-2.0.html for more details.
*/

#include "legacy_controller.h"
#include <linux/pm_qos.h>

#ifdef CONFIG_MTK_EMI
#include <mt_emi_api.h>
#endif

#ifdef CONFIG_MTK_QOS_SUPPORT
#include <helio-dvfsrc-opp.h>
#endif

#include "gl_typedef.h"

#define MAX_CPU_FREQ 2340000 /* in kHZ */
#define CLUSTER_NUM  2       /* 2 clusters, 4 big cores + 4 little cores */

int kalBoostCpu(unsigned int level)
{
	int i = 0;
	static unsigned int requested;
	struct ppm_limit_data freq_to_set[CLUSTER_NUM];

#ifdef CONFIG_MTK_QOS_SUPPORT
	static struct pm_qos_request wifi_qos_request;
#endif

	for (i = 0; i < CLUSTER_NUM; i++) {
		freq_to_set[i].max = -1; /* -1 means don't care */
		freq_to_set[i].min = level ? MAX_CPU_FREQ : -1;
	}
	update_userlimit_cpu_freq(PPM_KIR_WIFI, CLUSTER_NUM, freq_to_set);

#ifdef CONFIG_MTK_QOS_SUPPORT
	if (level) {
		if (!requested) {
			requested = 1;
			pm_qos_add_request(&wifi_qos_request, PM_QOS_EMI_OPP, DDR_OPP_0);
		}
		pm_qos_update_request(&wifi_qos_request, DDR_OPP_0);
	} else if (requested) {
		pm_qos_update_request(&wifi_qos_request, DDR_OPP_UNREQ);
		pm_qos_remove_request(&wifi_qos_request);
		requested = 0;
	}
#endif

	return 0;
}

#ifdef CONFIG_MTK_EMI
VOID kalSetEmiMpuProtection(phys_addr_t emiPhyBase, UINT_32 size, BOOLEAN enable)
{
	struct emi_region_info_t region_info;

	/*set MPU for EMI share Memory */
	region_info.start = emiPhyBase;
	region_info.end = emiPhyBase + size - 1;
	region_info.region = 25;
	SET_ACCESS_PERMISSION(region_info.apc, UNLOCK, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN,
			      FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN,
			      FORBIDDEN, FORBIDDEN, FORBIDDEN, NO_PROTECTION, FORBIDDEN,
			      enable ? FORBIDDEN:NO_PROTECTION);
	emi_mpu_set_protection(&region_info);
}
#endif

