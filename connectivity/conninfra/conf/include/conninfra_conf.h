/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */
/*! \file
*    \brief  Declaration of library functions
*
*    Any definitions in this file will be shared among GLUE Layer and internal Driver Stack.
*/



#ifndef _CONNINFRA_CONF_H_
#define _CONNINFRA_CONF_H_

#include "osal.h"

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/
#define CUST_CFG_INFRA "WMT.cfg"
#define CUST_CFG_INFRA_SOC "WMT_SOC.cfg"

#define CONNINFRA_CONF_EXP_PATTERN_NOT_FOUND 1

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/



/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/



/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                        P U B L I C   D A T A
********************************************************************************
*/
struct conf_byte_ary {
	unsigned int size;
	char *data;
};

struct conf_string_data {
	unsigned int size;
	char *data;
};

struct conninfra_conf {

	char conf_name[NAME_MAX + 1];
	//const osal_firmware *conf_inst;
	unsigned char cfg_exist;

	/* To avoid build error for short config function */
	short dummy_short;
	unsigned char co_clock_flag;

	/* POS. If set, means using ext TCXO */
	unsigned char tcxo_gpio;

	unsigned char pre_cal_mode;
	unsigned int vcn33_1_voltage;

	struct conf_byte_ary *dummy_byte_ary;
	struct conf_string_data *exp_filter;
};



/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/





/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/
const struct conninfra_conf *conninfra_conf_get_cfg(void);
int conninfra_conf_set_cfg_file(const char *name);
/* Give an input string to check if it is in the exception filter list.
 * Return value:
 *     CONNINFRA_CONF_EXP_PATTERN_NOT_FOUND: not exist.
 *      >=0 : index of the matched item.
 */
int conninfra_conf_exp_filter_check(const char *input);

int conninfra_conf_init(void);
int conninfra_conf_deinit(void);

#endif				/* _CONNINFRA_CONF_H_ */
