// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
/*****************************************************************************
 * headers
 *****************************************************************************/
#include <linux/string.h>

#include "met_drv.h" /* for metdevice */
#include "interface.h" /* for PR_BOOTMSG */
#include "tinysys_cpu_eb.h"
#include "tinysys_mgr.h" /* for ondiemet_module */
#include "cpu_eb_met_log.h" /* for cpu_eb_buffer_size */
#include "cpu_eb_cpu_pmu_hw_def.h" /* for  cpu_eb_cpu_pmu_header */
#include "cpu_eb_ptpod_hw_def.h" /* for  cpu_eb_ptpod_header */


/*****************************************************************************
 * define declaration
 *****************************************************************************/


/*****************************************************************************
 * struct & enum declaration
 *****************************************************************************/


/*****************************************************************************
 * external function declaration
 *****************************************************************************/


/*****************************************************************************
 * internal function declaration
 *****************************************************************************/
static void _cpu_eb_start(void);
static void _cpu_eb_stop(void);
static int _cpu_eb_process_argument(const char *arg, int len);
static int _cpu_eb_print_help(char *buf, int len);
static int _cpu_eb_print_header(char *buf, int len);


/*****************************************************************************
 * external variable declaration
 *****************************************************************************/
struct metdevice met_cpu_eb = {
	.name = "cpu_eb",
	.owner = THIS_MODULE,
	.type = MET_TYPE_MISC,
	.cpu_related = 0,
	.ondiemet_mode = 1,
	.ondiemet_start = _cpu_eb_start,
	.ondiemet_stop = _cpu_eb_stop,
	.ondiemet_process_argument = _cpu_eb_process_argument,
	.ondiemet_print_help = _cpu_eb_print_help,
	.ondiemet_print_header = _cpu_eb_print_header,
};
EXPORT_SYMBOL(met_cpu_eb);


/*****************************************************************************
 * internal variable declaration
 *****************************************************************************/
static const char help[] = "--cpu_eb=module_name\n";
static const char header[] = "met-info [000] 0.0: cpu_eb_header: ";


/*****************************************************************************
 * external function implement
 *****************************************************************************/
void notify_cpu_eb_cpu_pmu(int flag)
{
	if (flag == 1) {
		ondiemet_module[ONDIEMET_CPU_EB] |= (0x1 << MID_EB_CPU_PMU);
	} else {
		ondiemet_module[ONDIEMET_CPU_EB] &= ~(0x1 << MID_EB_CPU_PMU);
	}
}


/*****************************************************************************
 * internal function implement
 *****************************************************************************/
static void _cpu_eb_start(void)
{
	if (cpu_eb_buffer_size == 0) {
		ondiemet_module[ONDIEMET_CPU_EB] = 0;
		met_cpu_eb.mode = 0;
		return;
	}

	return;
}


static void _cpu_eb_stop(void)
{
	return;
}


static int _cpu_eb_process_argument(const char *arg, int len)
{
	if (strncmp(arg, "cpu_pmu", strlen("cpu_pmu")) == 0) {
		ondiemet_module[ONDIEMET_CPU_EB] |= (0x1 << MID_EB_CPU_PMU);
	}

	if (strncmp(arg, "sensor_network", strlen("sensor_network")) == 0) {
		ondiemet_module[ONDIEMET_CPU_EB] |= (0x1 << MID_EB_SENSOR_NETWORK);
	}

	if (strncmp(arg, "system_pi", strlen("system_pi")) == 0) {
		ondiemet_module[ONDIEMET_CPU_EB] |= (0x1 << MID_EB_SYSTEM_PI);
	}

	if (strncmp(arg, "ptpod", strlen("ptpod")) == 0) {
		ondiemet_module[ONDIEMET_CPU_EB] |= (0x1 << MID_EB_PTPOD);
	}
	met_cpu_eb.mode = 1;

	return 0;
}


static int _cpu_eb_print_help(char *buf, int len)
{
	return snprintf(buf, PAGE_SIZE, help);
}


static int _cpu_eb_print_header(char *buf, int len)
{
	int cnt = 0;
	int cpu_pmu_flag = 0;
	int ptpod_flag = 0;

	if (met_cpu_eb.mode == 0) {
		PR_BOOTMSG("eb %s %d\n", __FUNCTION__, __LINE__);
		return 0;
	}

	len = snprintf(buf, PAGE_SIZE, "%s", header);
	if (ondiemet_module[ONDIEMET_CPU_EB] & (0x1 << MID_EB_CPU_PMU)) {
		len += snprintf(buf + len, PAGE_SIZE - len, "cpu_pmu");
		cnt++;
		cpu_pmu_flag = 1;
	}

	if (ondiemet_module[ONDIEMET_CPU_EB] & (0x1 << MID_EB_SENSOR_NETWORK)) {
		if (cnt > 0)
			len += snprintf(buf + len, PAGE_SIZE - len, ",sensor_network");
		else
			len += snprintf(buf + len, PAGE_SIZE - len, "sensor_network");
		cnt++;
	}

	if (ondiemet_module[ONDIEMET_CPU_EB] & (0x1 << MID_EB_SYSTEM_PI)) {
		if (cnt > 0)
			len += snprintf(buf + len, PAGE_SIZE - len, ",system_pi");
		else
			len += snprintf(buf + len, PAGE_SIZE - len, "system_pi");
		cnt++;
	}

	if (ondiemet_module[ONDIEMET_CPU_EB] & (0x1 << MID_EB_PTPOD)) {
		if (cnt > 0)
			len += snprintf(buf + len, PAGE_SIZE - len, ",ptpod");
		else
			len += snprintf(buf + len, PAGE_SIZE - len, "ptpod");
		cnt++;
		ptpod_flag = 1;
	}

	len += snprintf(buf + len, PAGE_SIZE - len, "\n");

	if (cpu_pmu_flag == 1) {
		len += snprintf(buf + len, PAGE_SIZE - len, cpu_eb_cpu_pmu_header);
	}

	if (ptpod_flag == 1) {
		len += snprintf(buf + len, PAGE_SIZE - len, cpu_eb_ptpod_header);
	}

	ondiemet_module[ONDIEMET_CPU_EB] = 0;
	met_cpu_eb.mode = 0;

	return len;
}

