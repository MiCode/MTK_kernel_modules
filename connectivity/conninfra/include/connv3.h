/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef _CONNV3_H_
#define _CONNV3_H_

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
*                             D A T A   T Y P E S
********************************************************************************
*/
enum connv3_drv_type {
	CONNV3_DRV_TYPE_BT = 0,
	CONNV3_DRV_TYPE_WIFI = 1,
	CONNV3_DRV_TYPE_MODEM = 2,
	CONNV3_DRV_TYPE_CONNV3 = 3,
	CONNV3_DRV_TYPE_MAX
};

#define CONNV3_CB_RET_CAL_PASS 0x0
#define CONNV3_CB_RET_CAL_FAIL 0x1

/* bus hang error define */
#define CONNV3_BUS_CB_TOP_BUS_HANG			0x1
#define CONNV3_BUS_AP2CONN_RX_SLP_PROT_ERR		0x2
#define CONNV3_BUS_AP2CONN_TX_SLP_PROT_ERR		0x4
#define CONNV3_BUS_CONN_INFRA_OFF_CLK_ERR		0x8
#define CONNV3_BUS_CONN_INFRA_BUS_HANG_IRQ		0x10

#define CONNV3_ERR_RST_ONGOING			-0x7788

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


/* power on/off */
int connv3_pwr_on(enum connv3_drv_type drv_type);
int connv3_pwr_on_done(enum connv3_drv_type drv_type);
int connv3_pwr_off(enum connv3_drv_type drv_type);
int connv3_ext_32k_on(void);

/* chip reset
 * return:
 *    <0: error
 *    =0: triggered
 *    =1: ongoing
 */
int connv3_trigger_whole_chip_rst(enum connv3_drv_type drv, char *reason);

/* whole chip reset callback
 * return:
 *    =0: success
 *    !0: fail
 */
struct connv3_whole_chip_rst_cb {
	int (*pre_whole_chip_rst)(enum connv3_drv_type drv, char *reason);
	int (*post_whole_chip_rst)(void);
};


/* PMIC state */
void connv3_update_pmic_state(enum connv3_drv_type drv, char *buffer, int buf_sz);

/* subsys callback register */
struct connv3_pre_calibration_cb {
	int (*pre_on_cb)(void);
	int (*pwr_on_cb)(void);
	int (*do_cal_cb)(void);
	/* for security efuse download */
	int (*efuse_on_cb)(void);
	/* for pre-cal error handling
	 * callback to subsys only when there is error during pre-cal flow and
	 * hint subsys to break the process.
	 */
	int (*pre_cal_error)(void);
};

struct connv3_power_on_cb {
	int (*pre_power_on)(void);
	int (*power_on_notify)(void);
	int (*chip_power_down_notify)(unsigned int);
};

/* CR relative callback function
 * used by bus dump, hif dump, power dump
 */
struct connv3_cr_cb {
	void *priv_data;
	int (*read)(void *priv_data, unsigned int addr, unsigned int *value);
	int (*write)(void *priv_data, unsigned int addr, unsigned int value);
	int (*write_mask)(void *priv_data, unsigned int addr, unsigned int mask, unsigned int value);
};

/* Call from connv3 driver to subsys.
 * - power_dump_start
 * 	Parameter
 * 		- priv_data
 * 		- force_dump
 * 			- 0: normal dump
 * 			- 1: force dump
 * 	Return
 * 		- 0 if it is ok to dump (FW is wakeup).
 * 		    Driver has to make FW wakeup until power_dump_end is called.
 * 		- Else if FW is sleep.
 * - power_dump_end
 * 	Power dump flow is completed. Driver could make FW to sleep now.
 * - cr_cb: callback function to access CR through HIF
 */
struct connv3_power_dump_cb {
	int (*power_dump_start)(void *priv_data, unsigned int force_dump);
	int (*power_dump_end)(void *priv_data);
};

/* Return value:
 * - 0: no error
 * - CONNV3_BUS_CB_TOP_BUS_HANG ~ CONNV3_BUS_CONN_INFRA_BUS_HANG_IRQ
 */
int connv3_conninfra_bus_dump(enum connv3_drv_type drv_type);

/* HIF dump
 * addr is connsys view
 */
struct connv3_hif_dump_cb {
	int (*hif_dump_start)(enum connv3_drv_type from_drv, void *priv_data);
	int (*hif_dump_end)(enum connv3_drv_type from_drv, void *priv_data);
};
/* Return value:
 * - 0: no error, to_drv could support hif debug dump
 * - Others: to_drv not support hif debug dump
 */
int __must_check connv3_hif_dbg_start(enum connv3_drv_type from_drv, enum connv3_drv_type to_drv);
/* from_drv hint to_drv that hif debug dump is finished.
 * to_drv could get fully control.
 * 0: no error
 * other: something wrong. Need check log.
 */
int connv3_hif_dbg_end(enum connv3_drv_type from_drv, enum connv3_drv_type to_drv);
int connv3_hif_dbg_read(
	enum connv3_drv_type from_drv, enum connv3_drv_type to_drv,
	unsigned int addr, unsigned int *value);
int connv3_hif_dbg_write(
	enum connv3_drv_type from_drv, enum connv3_drv_type to_drv,
	unsigned int addr, unsigned int value);
int connv3_hif_dbg_write_mask(
	enum connv3_drv_type from_drv, enum connv3_drv_type to_drv,
	unsigned int addr, unsigned int mask, unsigned int value);

struct connv3_sub_drv_ops_cb {
	/* power on */
	struct connv3_power_on_cb pwr_on_cb;
	/* chip reset */
	struct connv3_whole_chip_rst_cb rst_cb;
	/* calibration */
	struct connv3_pre_calibration_cb pre_cal_cb;
	struct connv3_cr_cb cr_cb;
	/* power dump */
	struct connv3_power_dump_cb pwr_dump_cb;
	/* hif dump */
	struct connv3_hif_dump_cb hif_dump_cb;
};

int connv3_sub_drv_ops_register(enum connv3_drv_type drv_type, struct connv3_sub_drv_ops_cb *cb);
int connv3_sub_drv_ops_unregister(enum connv3_drv_type drv_type);

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

#endif /* _CONNINFRA_H_ */
