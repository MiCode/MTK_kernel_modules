/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef _CONNV3_HW_H_
#define _CONNV3_HW_H_

#include <linux/platform_device.h>
#include <linux/types.h>
#include "connv3.h"
#include "connv3_core.h"

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/
enum connv3_plt_state {
	CONNV3_PLT_STATE_READY = 0,
	CONNV3_PLT_STATE_CLK_ERROR,
	CONNV3_PLT_STATE_MAX,
};

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

struct connv3_dev_cb {
	int (*connv3_pmic_event_notifier) (unsigned int, unsigned int);
};

struct connv3_hw_ops_struct {
	u32 (*connsys_plt_clk_init) (struct platform_device *pdev, struct connv3_dev_cb *dev_cb);
	u32 (*connsys_plt_get_chipid) (void);
	u32 (*connsys_plt_get_adie_chipid) (void);
	u32 (*connsys_plt_pre_cal_blocking_enable) (void);
	/* Indicate which reset type is supported in this platform
	 * Return value:
	 * - 0: Power shutdown
	 * - 1: POR_RESET is supported
	 */
	u32 (*connsys_plt_reset_type_support) (void);
	u8* (*connsys_plt_get_custom_option)(u32* size);
	/* Check platform resource status, resource maybe clock or others.
	 */
	u32 (*connsys_plt_check_status) (void);
};

#define DRV_GEN_SUPPORT_FULL 0x7
struct connv3_plat_data {
	const u32 chip_id;
	const u32 consys_hw_version;
	const void* hw_ops;
	const void* platform_pmic_ops;
	const void* platform_pinctrl_ops;
	const void* platform_coredump_ops;
	const void* platform_dbg_ops;
};

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
int connv3_hw_init(struct platform_device *pdev, struct connv3_dev_cb *dev_cb);
int connv3_hw_deinit(void);

/* Parameter:
 * - curr_status: current power on status
 * - off_radio: the radio to turn off
 * Output:
 * - *pmic_state: pmic state change after this function call
 *     - 0: no change
 *     - 1: pmic shutdown
 */
int connv3_hw_pwr_off(unsigned int curr_status, unsigned int off_radio, unsigned int *pmic_state);
int connv3_hw_pwr_on(unsigned int curr_status, unsigned int on_radio);
int connv3_hw_pwr_on_done(unsigned int radio);
int connv3_hw_ext_32k_onoff(bool);
int connv3_hw_pwr_rst(void);

int connv3_hw_pmic_parse_state(char *buffer, int buf_sz);

int connv3_hw_clk_init(struct platform_device *pdev, struct connv3_dev_cb *dev_cb);
unsigned int connv3_hw_get_chipid(void);
unsigned int connv3_hw_get_adie_chipid(void);
/* Get platform supported reset type
 * If the platform function is not implemented, default value is 0 (shutdown PMIC).
 */
unsigned int connv3_hw_get_reset_type_support(void);

unsigned int connv3_hw_get_connsys_ic_info(uint8_t *buf, u32 buf_sz);
unsigned int connv3_hw_get_pmic_ic_info(uint8_t *buf, u32 buf_sz);
unsigned int connv3_hw_get_connsys_adie_ic_info(uint8_t *buf, u32 buf_sz);
unsigned int connv3_hw_pre_cal_blocking_enable(void);

int connv3_hw_bus_dump(enum connv3_drv_type drv_type, struct connv3_cr_cb *cb);
/* power dump */
int connv3_hw_power_info_dump(
	enum connv3_drv_type drv_type, struct connv3_cr_cb *cb,
	char *buf, unsigned int size);
int connv3_hw_power_info_reset(
	enum connv3_drv_type drv_type, struct connv3_cr_cb *cb);

/* DFD relative function */
int connv3_hw_dfd_trigger(bool);

u8* connv3_hw_get_custom_option(u32 *size);

int connv3_hw_check_status(void);

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

#endif	/* _CONNV3_HW_H_ */
