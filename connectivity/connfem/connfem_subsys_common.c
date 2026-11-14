// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/kernel.h>
#include "connfem.h"

/*******************************************************************************
 *				M A C R O S
 ******************************************************************************/
#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "["KBUILD_MODNAME"][CM]" fmt

/*******************************************************************************
 *			    D A T A   T Y P E S
 ******************************************************************************/


/*******************************************************************************
 *		    F U N C T I O N   D E C L A R A T I O N S
 ******************************************************************************/
static void *cm_epaelna_flags_get(void);
static struct connfem_epaelna_flag_tbl_entry *cm_epaelna_flags_tbl_get(void);
static unsigned int cm_epaelna_flags_cnt(void);

/*******************************************************************************
 *			    P U B L I C   D A T A
 ******************************************************************************/
struct connfem_epaelna_subsys_cb cfm_cm_epaelna_cb = {
	.flags_get = cm_epaelna_flags_get,
	.flags_tbl_get = cm_epaelna_flags_tbl_get,
	.flags_cnt = cm_epaelna_flags_cnt
};

/*******************************************************************************
 *			   P R I V A T E   D A T A
 ******************************************************************************/
static struct connfem_epaelna_flags_common cm_epaelna_flags;

static struct connfem_epaelna_flag_tbl_entry cm_epaelna_flags_map[] = {
	{"rx-mode",	&cm_epaelna_flags.rxmode},
	{"fe-ant-cnt",	&cm_epaelna_flags.fe_ant_cnt},
	{"fe-main-bt-share-lp2g",	&cm_epaelna_flags.fe_main_bt_share_lp2g},
	{"fe-conn-spdt",	&cm_epaelna_flags.fe_conn_spdt},
	{"fe-reserved",	&cm_epaelna_flags.fe_reserved},
	{"bd-type",	&cm_epaelna_flags.bd_type},
	{"fe-conn-dpdt-sp3t",	&cm_epaelna_flags.fe_conn_dpdt_sp3t},
	{"fe-bt-wf-usage",	&cm_epaelna_flags.fe_bt_wf_usage},
	{"fe-conn-spdt-2",	&cm_epaelna_flags.fe_conn_spdt_2},
	{NULL, NULL}
};

/*******************************************************************************
 *			      F U N C T I O N S
 ******************************************************************************/
static void *cm_epaelna_flags_get(void)
{
	return &cm_epaelna_flags;
}

static struct connfem_epaelna_flag_tbl_entry *cm_epaelna_flags_tbl_get(void)
{
	return cm_epaelna_flags_map;
}

static unsigned int cm_epaelna_flags_cnt(void)
{
	return sizeof(cm_epaelna_flags_map) /
		sizeof(struct connfem_epaelna_flag_tbl_entry) - 1;
}
