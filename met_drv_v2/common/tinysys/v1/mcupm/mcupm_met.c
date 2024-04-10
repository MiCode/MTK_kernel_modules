// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
/*****************************************************************************
 * headers
 *****************************************************************************/
#include <linux/string.h>
#include <linux/module.h> /* for symbol_get */
#include <linux/of.h> /* for of_find_node_by_name */

#include "met_drv.h" /* for metdevice */
#include "interface.h" /* for PR_BOOTMSG */
#include "tinysys_mcupm.h"
#include "tinysys_mgr.h" /* for ondiemet_module */
#include "mcupm_met_log.h" /* for mcupm_buffer_size */


/*****************************************************************************
 * define declaration
 *****************************************************************************/
#define MCUPM_CPU_PMU_HEADER_LEN    1536


/*****************************************************************************
 * struct & enum declaration
 *****************************************************************************/


/*****************************************************************************
 * external function declaration
 *****************************************************************************/
void mcupm_process_argument_real(const char *arg);
int mcupm_print_header_real(char *buf, int len, int cnt);


/*****************************************************************************
 * internal function declaration
 *****************************************************************************/
static void _mcupm_start(void);
static void _mcupm_stop(void);
static int _mcupm_process_argument(const char *arg, int len);
static int _mcupm_print_help(char *buf, int len);
static int _mcupm_print_header(char *buf, int len);
static void generate_mcupm_cpu_pmu_header(void);


/*****************************************************************************
 * external variable declaration
 *****************************************************************************/
struct metdevice met_mcupm = {
	.name = "mcupm",
	.owner = THIS_MODULE,
	.type = MET_TYPE_MISC,
	.cpu_related = 0,
	.ondiemet_mode = 1,
	.ondiemet_start = _mcupm_start,
	.ondiemet_stop = _mcupm_stop,
	.ondiemet_process_argument = _mcupm_process_argument,
	.ondiemet_print_help = _mcupm_print_help,
	.ondiemet_print_header = _mcupm_print_header,
};
EXPORT_SYMBOL(met_mcupm);


/*****************************************************************************
 * internal variable declaration
 *****************************************************************************/
static const char help[] = "--mcupm=module_name\n";
static const char header[] = "met-info [000] 0.0: mcupm_header: ";
static char mcupm_cpu_pmu_header[MCUPM_CPU_PMU_HEADER_LEN] = "";


/*****************************************************************************
 * external function implement
 *****************************************************************************/
void notify_mcupm_cpu_pmu(int flag)
{
	if (flag == 1) {
		ondiemet_module[ONDIEMET_MCUPM] |= (0x1 << MID_MCUPM_CPU_PMU);
	} else {
		ondiemet_module[ONDIEMET_MCUPM] &= ~(0x1 << MID_MCUPM_CPU_PMU);
	}
}


/*****************************************************************************
 * internal function implement
 *****************************************************************************/
static void _mcupm_start(void)
{
	if (mcupm_buffer_size == 0) {
		ondiemet_module[ONDIEMET_MCUPM] = 0;
		met_mcupm.mode = 0;
		return;
	}

	return;
}


static void _mcupm_stop(void)
{
	return;
}


static int _mcupm_process_argument(const char *arg, int len)
{
	void (*mcupm_process_argument_real_sym)(const char *arg) = NULL;

	if (strncmp(arg, "cpu_pmu", strlen("cpu_pmu")) == 0) {
		ondiemet_module[ONDIEMET_MCUPM] |= (0x1 << MID_MCUPM_CPU_PMU);
	}

	mcupm_process_argument_real_sym = symbol_get(mcupm_process_argument_real);
	if (mcupm_process_argument_real_sym) {
		mcupm_process_argument_real_sym(arg);
	}
	met_mcupm.mode = 1;

	return 0;
}


static int _mcupm_print_help(char *buf, int len)
{
	return snprintf(buf, PAGE_SIZE, help);
}

static void generate_mcupm_cpu_pmu_header(void)
{
	int len = 0;
	int ret = 0;
	struct device_node *node = NULL;
	struct device_node *core_node = NULL;
	int cluster_num = 0;
	int cord_id = 0;
	char cluster_name[16];
	char core_name[16];
	int i, j;
	char *ptr = mcupm_cpu_pmu_header;

	len = snprintf(ptr, MCUPM_CPU_PMU_HEADER_LEN,
		"met-info [000] 0.0: met_mcupm_dsu_pmu_header: DSU,0x11,0x2A,0x2B\n");
	if (len < 0)
		return ;

	len += snprintf(ptr + len, MCUPM_CPU_PMU_HEADER_LEN - len,
		"met-info [000] 0.0: pmu_sampler: mcupm\n");
	if (len < 0)
		return ;

	node = of_find_node_by_name(NULL, "cpu-map");
	if (!node)
		node = of_find_node_by_name(NULL, "virtual-cpu-map");

	if (node) {
		cluster_num = of_get_child_count(node);
		of_node_put(node);
		for (i = 0; i < cluster_num; i++) {
			ret = snprintf(cluster_name, sizeof(cluster_name), "cluster%d", i);
			if (ret < 0)
				return ;
			node = of_find_node_by_name(NULL, cluster_name);
			if (node) {
				j = 0;
				do {
					ret = snprintf(core_name, sizeof(core_name), "core%d", j);
					if (ret < 0)
						return ;
					core_node = of_get_child_by_name(node, core_name);
					if (core_node) {
						if (i == 0) {
							len += snprintf(ptr + len, MCUPM_CPU_PMU_HEADER_LEN - len,
								"met-info [000] 0.0: met_mcupm_cpu_header_v2: %d," \
								"0x3,0x4,0x8,0x11,0x16,0x17,0x19,0x1B,0x2A,0x2B,0x66," \
								"0x67,0x70,0x71,0x73,0x74,0x75,0xE7,0xE8\n", cord_id);
						} else {
							len += snprintf(ptr + len, MCUPM_CPU_PMU_HEADER_LEN - len,
								"met-info [000] 0.0: met_mcupm_cpu_header_v2: %d," \
								"0x3,0x4,0x8,0x11,0x16,0x17,0x19,0x1B," \
								"0x2A,0x2B,0x70,0x71,0x73,0x74,0x75\n", cord_id);
						}
						cord_id++;
						of_node_put(core_node);
					}
					j++;
				} while (core_node);
				of_node_put(node);
			}
		}
	}
}


static int _mcupm_print_header(char *buf, int len)
{
	int cnt = 0;
	int cpu_pmu_flag = 0;
	int (*mcupm_print_header_real_sym)(char *buf, int len, int cnt) = NULL;

	if (met_mcupm.mode == 0) {
		PR_BOOTMSG("mcupm %s %d\n", __FUNCTION__, __LINE__);
		return 0;
	}

	len = snprintf(buf, PAGE_SIZE, "%s", header);
	if (len < 0)
		return 0;

	if (ondiemet_module[ONDIEMET_MCUPM] & (0x1 << MID_MCUPM_CPU_PMU)) {
		len += snprintf(buf + len, PAGE_SIZE - len, "cpu_pmu");
		cnt++;
		cpu_pmu_flag = 1;
	}

	mcupm_print_header_real_sym = symbol_get(mcupm_print_header_real);
	if (mcupm_print_header_real_sym) {
		len = mcupm_print_header_real_sym(buf, len, cnt);
	}

	if (cpu_pmu_flag == 1) {
		if (strlen(mcupm_cpu_pmu_header) == 0) {
			generate_mcupm_cpu_pmu_header();
		}
		len += snprintf(buf + len, PAGE_SIZE - len, mcupm_cpu_pmu_header);
	}

	ondiemet_module[ONDIEMET_MCUPM] = 0;
	met_mcupm.mode = 0;

	if (len < 0)
		return 0;

	return len;
}

