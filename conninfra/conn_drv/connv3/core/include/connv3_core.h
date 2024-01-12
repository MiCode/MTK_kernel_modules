/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef _CONNV3_CORE_H_
#define _CONNV3_CORE_H_

#include <linux/semaphore.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/time.h>

#include "osal.h"
#include "msg_thread.h"
#include "connv3.h"

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/
/* Disable pre-cal check on FPGA because FPGA may not have BT and WIFI together */
#if defined(CONFIG_FPGA_EARLY_PORTING)
#define ENABLE_PRE_CAL_BLOCKING_CHECK	0
#else
#define ENABLE_PRE_CAL_BLOCKING_CHECK	1
#endif
#define CHIP_RST_REASON_MAX_LEN			128

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
enum connv3_drv_status {
	DRV_STS_POWER_OFF = 0,	/* initial state */
	DRV_STS_PRE_POWER_ON = 1,
	DRV_STS_POWER_ON = 2,	/* powered on */
	DRV_STS_RESET = 3,
	DRV_STS_MAX
};

enum pre_cal_status {
	PRE_CAL_NOT_INIT = 0,
	PRE_CAL_NEED_RESCHEDULE = 1,
	PRE_CAL_SCHEDULED = 2,
	PRE_CAL_EXECUTING = 3,
	PRE_CAL_DONE = 4
};

enum pre_cal_caller {
	PRE_CAL_BY_NONE = 0,
	PRE_CAL_BY_SUBDRV_REGISTER = 1,
	PRE_CAL_BY_SUBDRV_PWR_ON = 2,
	PRE_CAL_BY_SCREEN_ON = 3
};

enum chip_rst_status {
	CHIP_RST_NONE = 0,
	CHIP_RST_START = 1,
	CHIP_RST_PRE_CB = 2,
	CHIP_RST_RESET = 3,
	CHIP_RST_POST_CB = 4,
	CHIP_RST_DONE = 5
};

enum connv3_radio_off_mode {
	CONNV3_RADIO_OFF_MODE_PMIC_OFF = 0,
	CONNV3_RADIO_OFF_MODE_UDS = 1,
	CONNV3_RADIO_OFF_MODE_MAX,
};

struct subsys_drv_inst {
	enum connv3_drv_status drv_status;	/* Controlled driver status */
	unsigned int rst_state;
	struct connv3_sub_drv_ops_cb ops_cb;
	struct msg_thread_ctx msg_ctx;
};

struct pre_cal_info {
	enum pre_cal_status status;
	enum pre_cal_caller caller;
	struct work_struct pre_cal_work;
	OSAL_SLEEPABLE_LOCK pre_cal_lock;
};


/*
 * state of conninfra
 *
 */
struct connv3_ctx {
	enum connv3_drv_status core_status;

	struct subsys_drv_inst drv_inst[CONNV3_DRV_TYPE_MAX];
	/*struct spinlock infra_lock;*/
	spinlock_t infra_lock;
	/* pre power on state */
	atomic_t pre_pwr_state;
	struct semaphore pre_pwr_sema;

	/* For power dump function
	 * 1. Enable power status dump when conninfra power on
	 * 	- Power status dump includes: sleep count and power state
	 * 2. Reset and clear power status when platform suspend
	 * 3. Dump power status when platform resume
	 */
	//spinlock_t power_dump_lock;
	atomic_t power_dump_enable;

	OSAL_SLEEPABLE_LOCK core_lock;

	/* chip reset */
	enum chip_rst_status rst_status;
	spinlock_t rst_lock;

	struct semaphore rst_sema;
	atomic_t rst_state;
	enum connv3_drv_type trg_drv;
	char trg_reason[CHIP_RST_REASON_MAX_LEN];

	/* pre_cal */
	struct semaphore pre_cal_sema;
	atomic_t pre_cal_state;

	struct msg_thread_ctx msg_ctx;
	struct msg_thread_ctx cb_ctx;

	unsigned int hw_ver;
	unsigned int fw_ver;
	unsigned int ip_ver;

	struct osal_op_history cored_op_history;

	struct pre_cal_info cal_info;
};

//typedef enum _ENUM_CONNINFRA_CORE_OPID_T {
typedef enum {
	CONNV3_OPID_PWR_ON 			= 0,
	CONNV3_OPID_PWR_OFF			= 1,
	CONNV3_OPID_PWR_ON_DONE			= 3,
	CONNV3_OPID_PRE_CAL_PREPARE		= 4,
	CONNV3_OPID_PRE_CAL_CHECK		= 5,
	CONNV3_OPID_RESET_POWER_STATE		= 6,
	CONNV3_OPID_DUMP_POWER_STATE		= 7,
	CONNV3_OPID_EXT_32K_ON			= 8,
	CONNV3_OPID_RESET_AND_DUMP_POWER_STATE	= 9,
	CONNV3_OPID_MAX
} connv3_core_opid;

/* For the operation which may callback subsys driver */
typedef enum {
	CONNV3_CB_OPID_CHIP_RST         = 0,
	CONNV3_CB_OPID_PRE_CAL          = 1,
	CONNV3_CB_OPID_MAX
} connv3_core_cb_opid;

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

int connv3_core_init(void);
int connv3_core_deinit(void);

int connv3_core_power_on(enum connv3_drv_type type);
int connv3_core_power_on_done(enum connv3_drv_type type);
int connv3_core_power_off(enum connv3_drv_type type);
int connv3_core_ext_32k_on(void);

int connv3_core_lock_rst(void);
int connv3_core_unlock_rst(void);
int connv3_core_trg_chip_rst(enum connv3_drv_type drv, char *reason);

int connv3_core_pmic_event_cb(unsigned int id, unsigned int event);
void connv3_core_update_pmic_status(enum connv3_drv_type drv, char *buffer, int buf_sz);

int connv3_core_subsys_ops_reg(enum connv3_drv_type type,
						struct connv3_sub_drv_ops_cb *cb);
int connv3_core_subsys_ops_unreg(enum connv3_drv_type type);

int connv3_core_screen_on(void);
int connv3_core_screen_off(void);

/* pre_cal */
int connv3_core_pre_cal_start(void);

#if ENABLE_PRE_CAL_BLOCKING_CHECK
void connv3_core_pre_cal_blocking(void);
#endif

/* Check if L0 reset is ongoing.
 */
int connv3_core_is_rst_locking(void);
/* Check if L0 reset is ongoing and in a stage that
 * it may do power off in anytime.
 * It is dangerous to access connsys chip.
 */
int connv3_core_is_rst_power_off_stage(void);

int connv3_core_bus_dump(enum connv3_drv_type drv_type);

int connv3_core_reset_power_state(void);
int connv3_core_dump_power_state(char *buf, unsigned int size);
/* Call following two function in single op
 * 1. Dump power state if has started.
 * 2. Reset power state
 */
int connv3_core_reset_and_dump_power_state(char *buf, unsigned int size, bool force_dump);

/* HIF dump
 */
int connv3_core_hif_dbg_start(enum connv3_drv_type from_drv, enum connv3_drv_type to_drv);
int connv3_core_hif_dbg_end(enum connv3_drv_type from_drv, enum connv3_drv_type to_drv);
int connv3_core_hif_dbg_read(
	enum connv3_drv_type from_drv, enum connv3_drv_type to_drv,
	unsigned int addr, unsigned int *value);
int connv3_core_hif_dbg_write(
	enum connv3_drv_type from_drv, enum connv3_drv_type to_drv,
	unsigned int addr, unsigned int value);
int connv3_core_hif_dbg_write_mask(
	enum connv3_drv_type from_drv, enum connv3_drv_type to_drv,
	unsigned int addr, unsigned int mask, unsigned int value);

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

#endif				/* _CONNINFRA_CORE_H_ */
