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

#ifndef _CONNINFRA_H_
#define _CONNINFRA_H_


/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*								  M A C R O S
********************************************************************************
*/

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/
enum consys_drv_type {
	CONNDRV_TYPE_BT = 0,
	CONNDRV_TYPE_FM = 1,
	CONNDRV_TYPE_GPS = 2,
	CONNDRV_TYPE_WIFI = 3,
	CONNDRV_TYPE_MAX
};

#define CONNINFRA_CB_RET_CAL_PASS_POWER_OFF 0x0
#define CONNINFRA_CB_RET_CAL_PASS_POWER_ON  0x2
#define CONNINFRA_CB_RET_CAL_FAIL_POWER_OFF 0x1
#define CONNINFRA_CB_RET_CAL_FAIL_POWER_ON  0x3

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

/* EMI */
void conninfra_get_phy_addr(unsigned int *addr, unsigned int *size);

/* power on/off */
int conninfra_pwr_on(enum consys_drv_type drv_type);
int conninfra_pwr_off(enum consys_drv_type drv_type);


/* chip reset
* return:
*    <0: error
*    =0: triggered
*    =1: ongoing
*/
int conninfra_trigger_whole_chip_rst(enum consys_drv_type drv, char *reason);

struct whole_chip_rst_cb {
	int (*pre_whole_chip_rst)(void);
	int (*post_whole_chip_rst)(void);
};

/* driver state query */

/* VCN control */

/* Thermal */

/* Config */

/* semaphore */

/* calibration */



/* subsys callback register */
struct pre_calibration_cb {
	int (*pwr_on_cb)(void);
	int (*do_cal_cb)(void);
};

struct sub_drv_ops_cb {
	/* chip reset */
	struct whole_chip_rst_cb rst_cb;

	/* calibration */
	struct pre_calibration_cb pre_cal_cb;

	/* thermal query */
	int (*thermal_qry)(void);

};

int conninfra_sub_drv_ops_register(enum consys_drv_type drv_type, struct sub_drv_ops_cb *cb);
int conninfra_sub_drv_ops_unregister(enum consys_drv_type drv_type);


/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

#endif /* _CONNINFRA_H_ */
