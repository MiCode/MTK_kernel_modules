/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef _METLOG_H_
#define _METLOG_H_

#include <linux/types.h>
#include <linux/compiler.h>

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/
/* Close debug log */
//#define DEBUG_LOG_ON 1

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/
struct conn_metlog_info {
	int type;
	phys_addr_t read_cr;
	phys_addr_t write_cr;
	phys_addr_t met_base_ap;
	unsigned int met_base_fw;
	unsigned int met_size;
	unsigned int output_len;
};

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

int conn_metlog_start(struct conn_metlog_info *info);
int conn_metlog_stop(int type);

#endif /*_METLOG_H_*/
