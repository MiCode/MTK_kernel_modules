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
#include "tinysys_gpueb.h"
#include "tinysys_mgr.h" /* for ondiemet_module */
#include "gpueb_met_log.h" /* for gpueb_buffer_size */


/*****************************************************************************
 * define declaration
 *****************************************************************************/
#define MAX_HEADER_LEN    1024


/*****************************************************************************
 * struct & enum declaration
 *****************************************************************************/


/*****************************************************************************
 * external function declaration
 *****************************************************************************/
void gpueb_process_argument_real(const char *arg);
int gpueb_print_header_real(char *buf, int len, int cnt);


/*****************************************************************************
 * internal function declaration
 *****************************************************************************/
static void _gpueb_start(void);
static void _gpueb_stop(void);
static int _gpueb_process_argument(const char *arg, int len);
static int _gpueb_print_help(char *buf, int len);
static int _gpueb_print_header(char *buf, int len);


/*****************************************************************************
 * external variable declaration
 *****************************************************************************/
struct metdevice met_gpueb = {
	.name = "gpueb",
	.owner = THIS_MODULE,
	.type = MET_TYPE_MISC,
	.cpu_related = 0,
	.ondiemet_mode = 1,
	.ondiemet_start = _gpueb_start,
	.ondiemet_stop = _gpueb_stop,
	.ondiemet_process_argument = _gpueb_process_argument,
	.ondiemet_print_help = _gpueb_print_help,
	.ondiemet_print_header = _gpueb_print_header,
};
EXPORT_SYMBOL(met_gpueb);


/*****************************************************************************
 * internal variable declaration
 *****************************************************************************/
static const char help[] = "--gpueb=module_name\n";
static const char header[] = "met-info [000] 0.0: gpueb_header: ";


/*****************************************************************************
 * external function implement
 *****************************************************************************/


/*****************************************************************************
 * internal function implement
 *****************************************************************************/
static void _gpueb_start(void)
{
	if (gpueb_buffer_size == 0) {
		ondiemet_module[ONDIEMET_GPUEB] = 0;
		met_gpueb.mode = 0;
		return;
	}

	return;
}


static void _gpueb_stop(void)
{
	return;
}


static int _gpueb_process_argument(const char *arg, int len)
{
	met_gpueb.mode = 1;

	return 0;
}


static int _gpueb_print_help(char *buf, int len)
{
	return snprintf(buf, PAGE_SIZE, help);
}


static int _gpueb_print_header(char *buf, int len)
{
	if (met_gpueb.mode == 0) {
		PR_BOOTMSG("gpueb %s %d\n", __FUNCTION__, __LINE__);
		return 0;
	}

	len = snprintf(buf, PAGE_SIZE, "%s", header);
	if (len < 0)
		return 0;

	ondiemet_module[ONDIEMET_GPUEB] = 0;
	met_gpueb.mode = 0;

	return len;
}

