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
#define pr_fmt(fmt) "["KBUILD_MODNAME"][BT]" fmt

/*******************************************************************************
 *			    D A T A   T Y P E S
 ******************************************************************************/


/*******************************************************************************
 *		    F U N C T I O N   D E C L A R A T I O N S
 ******************************************************************************/
static void *bt_epaelna_flags_get(void);
static struct connfem_epaelna_flag_tbl_entry *bt_epaelna_flags_tbl_get(void);
static unsigned int bt_epaelna_flags_cnt(void);

/*******************************************************************************
 *			    P U B L I C   D A T A
 ******************************************************************************/
struct connfem_epaelna_subsys_cb cfm_bt_epaelna_cb = {
	.flags_get = bt_epaelna_flags_get,
	.flags_tbl_get = bt_epaelna_flags_tbl_get,
	.flags_cnt = bt_epaelna_flags_cnt
};

/*******************************************************************************
 *			   P R I V A T E   D A T A
 ******************************************************************************/
static struct connfem_epaelna_flags_bt bt_epaelna_flags;

static struct connfem_epaelna_flag_tbl_entry bt_epaelna_flags_map[] = {
	{"bypass",	(unsigned char*)&bt_epaelna_flags.bypass},
	{"epa_elna",	(unsigned char*)&bt_epaelna_flags.epa_elna},
	{"epa-elna",	(unsigned char*)&bt_epaelna_flags.epa_elna},
	{"epa",		(unsigned char*)&bt_epaelna_flags.epa},
	{"elna",	(unsigned char*)&bt_epaelna_flags.elna},
	{"efem-mode",	(unsigned char*)&bt_epaelna_flags.efem_mode},
	{"rx-mode",	(unsigned char*)&bt_epaelna_flags.rx_mode},
	{NULL, NULL}
};

/*******************************************************************************
 *			      F U N C T I O N S
 ******************************************************************************/
static void *bt_epaelna_flags_get(void)
{
	return &bt_epaelna_flags;
}

static struct connfem_epaelna_flag_tbl_entry *bt_epaelna_flags_tbl_get(void)
{
	return bt_epaelna_flags_map;
}

static unsigned int bt_epaelna_flags_cnt(void)
{
	return sizeof(bt_epaelna_flags_map) /
		sizeof(struct connfem_epaelna_flag_tbl_entry) - 1;
}
