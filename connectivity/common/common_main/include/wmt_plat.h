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

#ifndef _WMT_PLAT_H_
#define _WMT_PLAT_H_
#include "osal_typedef.h"
#include "stp_exp.h"
#include <mtk_wcn_cmb_stub.h>
#include "mtk_wcn_cmb_hw.h"

/* #include "mtk_wcn_consys_hw.h" */

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

#if 1				/* moved from wmt_exp.h */
#ifndef DFT_TAG
#define DFT_TAG         "[WMT-DFT]"
#endif

#define WMT_PLAT_LOG_LOUD                 4
#define WMT_PLAT_LOG_DBG                  3
#define WMT_PLAT_LOG_INFO                 2
#define WMT_PLAT_LOG_WARN                 1
#define WMT_PLAT_LOG_ERR                  0

extern INT32 wmtPlatLogLvl;

#define WMT_PLAT_PR_LOUD(fmt, arg...) \
do { \
	if (wmtPlatLogLvl >= WMT_PLAT_LOG_LOUD) \
		pr_info(DFT_TAG "[L]%s:"  fmt, __func__, ##arg); \
} while (0)
#define WMT_PLAT_PR_INFO(fmt, arg...) \
do { \
	if (wmtPlatLogLvl >= WMT_PLAT_LOG_INFO) \
		pr_info(DFT_TAG "[I]%s:"  fmt, __func__, ##arg); \
} while (0)
#define WMT_PLAT_PR_WARN(fmt, arg...) \
do { \
	if (wmtPlatLogLvl >= WMT_PLAT_LOG_WARN) \
		pr_warn(DFT_TAG "[W]%s:"  fmt, __func__, ##arg); \
} while (0)
#define WMT_PLAT_PR_ERR(fmt, arg...) \
do { \
	if (wmtPlatLogLvl >= WMT_PLAT_LOG_ERR) \
		pr_err(DFT_TAG "[E]%s(%d):"  fmt, __func__, __LINE__, ##arg); \
} while (0)
#define WMT_PLAT_PR_DBG(fmt, arg...) \
do { \
	if (wmtPlatLogLvl >= WMT_PLAT_LOG_DBG) \
		pr_info(DFT_TAG "[D]%s:"  fmt, __func__, ##arg); \
} while (0)

#endif

#define CFG_WMT_PS_SUPPORT 1	/* moved from wmt_exp.h */

#define CFG_WMT_DUMP_INT_STATUS 0
#define CONSYS_ENALBE_SET_JTAG 1

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*								  M A C R O S
********************************************************************************
*/

#if (defined(MT6630) || defined(MT6632))
#define CONSYS_WMT_REG_SUSPEND_CB_ENABLE 1
#else
#define CONSYS_WMT_REG_SUSPEND_CB_ENABLE 0
#endif

#if defined(MERGE_INTERFACE_SUPPORT) && (defined(MT6628) || defined(MT6630) || defined(MT6632))
#define MTK_WCN_CMB_MERGE_INTERFACE_SUPPORT 1
#else
#define MTK_WCN_CMB_MERGE_INTERFACE_SUPPORT 0
#endif

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/
typedef enum _ENUM_PIN_ID_ {
	PIN_LDO = 0,
	PIN_PMU = 1,
	PIN_RTC = 2,
	PIN_RST = 3,
	PIN_BGF_EINT = 4,
	PIN_WIFI_EINT = 5,
	PIN_ALL_EINT = 6,
	PIN_UART_GRP = 7,
	PIN_PCM_GRP = 8,
	PIN_I2S_GRP = 9,
	PIN_SDIO_GRP = 10,
	PIN_GPS_SYNC = 11,
	PIN_GPS_LNA = 12,
	PIN_UART_RX = 13,
#if CFG_WMT_LTE_COEX_HANDLING
	PIN_TDM_REQ = 14,
#endif
	PIN_ID_MAX
} ENUM_PIN_ID, *P_ENUM_PIN_ID;
#if 0
typedef enum _ENUM_PIN_ID_ {
	PIN_BGF_EINT = 0,
	PIN_I2S_GRP = 1,
	PIN_GPS_SYNC = 2,
	PIN_GPS_LNA = 3,
#if CFG_WMT_LTE_COEX_HANDLING
	PIN_TDM_REQ = 4,
#endif
	PIN_ID_MAX
} ENUM_PIN_ID, *P_ENUM_PIN_ID;
#endif

typedef enum _ENUM_FUNC_STATE_ {
	FUNC_ON = 0,
	FUNC_OFF = 1,
	FUNC_RST = 2,
	FUNC_STAT = 3,
	FUNC_CTRL_MAX,
} ENUM_FUNC_STATE, *P_ENUM_FUNC_STATE;

typedef enum _ENUM_PIN_STATE_ {
	PIN_STA_INIT = 0,
	PIN_STA_OUT_L = 1,
	PIN_STA_OUT_H = 2,
	PIN_STA_IN_L = 3,
	PIN_STA_MUX = 4,
	PIN_STA_EINT_EN = 5,
	PIN_STA_EINT_DIS = 6,
	PIN_STA_DEINIT = 7,
	PIN_STA_SHOW = 8,
	PIN_STA_IN_PU = 9,
	PIN_STA_IN_NP = 10,
	PIN_STA_IN_H = 11,
	PIN_STA_MAX
} ENUM_PIN_STATE, *P_ENUM_PIN_STATE;

typedef enum _CMB_IF_TYPE_ {
	CMB_IF_UART = 0,
	CMB_IF_WIFI_SDIO = 1,
	CMB_IF_BGF_SDIO = 2,
	CMB_IF_BGWF_SDIO = 3,
	CMB_IF_TYPE_MAX
} CMB_IF_TYPE, *P_CMB_IF_TYPE;

typedef INT32(*fp_set_pin) (ENUM_PIN_STATE);

typedef enum _ENUM_WL_OP_ {
	WL_OP_GET = 0,
	WL_OP_PUT = 1,
	WL_OP_MAX
} ENUM_WL_OP, *P_ENUM_WL_OP;

typedef enum _ENUM_PALDO_TYPE_ {
	BT_PALDO = 0,
	WIFI_PALDO = 1,
	FM_PALDO = 2,
	GPS_PALDO = 3,
	PMIC_CHIPID_PALDO = 4,
	WIFI_5G_PALDO = 5,
	EFUSE_PALDO = 6,
	PALDO_TYPE_MAX
} ENUM_PALDO_TYPE, *P_ENUM_PALDO_TYPE;

typedef enum _ENUM_PALDO_OP_ {
	PALDO_OFF = 0,
	PALDO_ON = 1,
	PALDO_OP_MAX
} ENUM_PALDO_OP, *P_ENUM_PALDO_OP;

typedef enum _ENUM_HOST_DUMP_STATE_T {
	STP_HOST_DUMP_NOT_START = 0,
	STP_HOST_DUMP_GET = 1,
	STP_HOST_DUMP_GET_DONE = 2,
	STP_HOST_DUMP_END = 3,
	STP_HOST_DUMP_MAX
} ENUM_HOST_DUMP_STATE, *P_ENUM_HOST_DUMP_STATE_T;

typedef enum _ENUM_FORCE_TRG_ASSERT_T {
	STP_FORCE_TRG_ASSERT_EMI = 0,
	STP_FORCE_TRG_ASSERT_DEBUG_PIN = 1,
	STP_FORCE_TRG_ASSERT_MAX = 2
} ENUM_FORCE_TRG_ASSERT_T, *P_ENUM_FORCE_TRG_ASSERT_T;

typedef enum _ENUM_CHIP_DUMP_STATE_T {
	STP_CHIP_DUMP_NOT_START = 0,
	STP_CHIP_DUMP_PUT = 1,
	STP_CHIP_DUMP_PUT_DONE = 2,
	STP_CHIP_DUMP_END = 3,
	STP_CHIP_DUMP_MAX
} ENUM_CHIP_DUMP_STATE, *P_ENUM_CHIP_DUMP_STATE_T;

typedef enum {
	WMT_SLEEP_COUNT_TOP = 0,
	WMT_SLEEP_COUNT_MCU = 1,
	WMT_SLEEP_COUNT_BT = 2,
	WMT_SLEEP_COUNT_WF = 3,
	WMT_SLEEP_COUNT_GPS = 4,
	WMT_SLEEP_COUNT_MAX
} WMT_SLEEP_COUNT_TYPE;

typedef enum {
	WMT_ERRCODE_SUCCESS = 0,
	WMT_ERRCODE_ALREADY_ON,
	WMT_ERRCODE_ALREADY_OFF,
	WMT_ERRCODE_READYTO_OFF,
	WMT_ERRCODE_EMI_NOT_READY,
	WMT_ERRCODE_GPIO_CTRL_FAIL,
	WMT_ERRCODE_POLL_CHIPID_FAIL,
	WMT_ERRCODE_POLL_NOT_GOTO_IDLE,
	WMT_ERRCODE_NO_HIF_INFO,
	WMT_ERRCODE_SDIO_SLOT_SDIO2_FAIL,
	WMT_ERRCODE_SDIO_FUNC_STP_FAIL,
	WMT_ERRCODE_OPEN_STP_FAIL,
	WMT_ERRCODE_UART_BAUDRATE_FAIL,
	WMT_ERRCODE_STP_CONFIG_FAIL,
	WMT_ERRCODE_HW_CHECK_FAIL,
	WMT_ERRCODE_VER_CHECK_FAIL,
	WMT_ERRCODE_CHIPID_NOT_SUPPORT,
	WMT_ERRCODE_NULL_FUNC_POINTER,
	WMT_ERRCODE_PATCH_INFO_FAIL,
	WMT_ERRCODE_PATCH_DWN_FAIL,
	WMT_ERRCODE_ADIE_NOT_EXIST,
	WMT_ERRCODE_INIT_TABLE_FAIL,
	WMT_ERRCODE_INIT_DLM_TABLE_FAIL,
	WMT_ERRCODE_INIT_MCUCLK_TABLE_FAIL,
	WMT_ERRCODE_INIT_RESET_CMD_FAIL,
	WMT_ERRCODE_INIT_COCLOCK_TYPE_FAIL,
	WMT_ERRCODE_INIT_MERGE_PCM_TABLE_FAIL,
	WMT_ERRCODE_INIT_FM_MODE_FAIL,
	WMT_ERRCODE_INIT_SET_REGISTER_FAIL,
	WMT_ERRCODE_INIT_SET_COREDUMP_FAIL,
	WMT_ERRCODE_INIT_WIFI_CONFIG_FAIL,
	WMT_ERRCODE_INIT_WIFI_ANT_SWAP_FAIL,
	WMT_ERRCODE_INIT_EPA_FAIL,
	WMT_ERRCODE_INIT_EPA_ELNA_FAIL,
	WMT_ERRCODE_INIT_EPA_ELNA_INVERT_CR_FAIL,
	WMT_ERRCODE_INIT_COEX_FAIL,
	WMT_ERRCODE_CALIBRATION_FAIL,
	WMT_ERRCODE_READ_ADIE_TX_CMD_FAIL,
	WMT_ERRCODE_READ_ADIE_RX_EVT_FAIL,
	WMT_ERRCODE_READ_PMIC_CHIPID_FAIL,
	WMT_ERRCODE_SET_STP_DBG_INFO_FAIL,
	/* return error code for WMT */
	WMT_ERRCODE_MAX
} WMT_ERRCODE_TYPE;

typedef struct _EMI_CTRL_STATE_OFFSET_ {
	UINT32 emi_apmem_ctrl_state;
	UINT32 emi_apmem_ctrl_host_sync_state;
	UINT32 emi_apmem_ctrl_host_sync_num;
	UINT32 emi_apmem_ctrl_chip_sync_state;
	UINT32 emi_apmem_ctrl_chip_sync_num;
	UINT32 emi_apmem_ctrl_chip_sync_addr;
	UINT32 emi_apmem_ctrl_chip_sync_len;
	UINT32 emi_apmem_ctrl_chip_print_buff_start;
	UINT32 emi_apmem_ctrl_chip_print_buff_len;
	UINT32 emi_apmem_ctrl_chip_print_buff_idx;
	UINT32 emi_apmem_ctrl_chip_int_status;
	UINT32 emi_apmem_ctrl_chip_paded_dump_end;
	UINT32 emi_apmem_ctrl_host_outband_assert_w1;
	UINT32 emi_apmem_ctrl_chip_page_dump_num;
	UINT32 emi_apmem_ctrl_assert_flag;
	UINT32 emi_apmem_ctrl_chip_check_sleep;
} EMI_CTRL_STATE_OFFSET, *P_EMI_CTRL_STATE_OFFSET;

typedef struct _BGF_IRQ_BALANCE_ {
	UINT32 counter;
	unsigned long flags;
	spinlock_t lock;
} BGF_IRQ_BALANCE, *P_BGF_IRQ_BALANCE;

typedef struct _CONSYS_EMI_ADDR_INFO_ {
	UINT32 emi_phy_addr;
	UINT32 emi_ap_phy_addr;
	UINT32 paged_trace_off;
	UINT32 paged_dump_off;
	UINT32 full_dump_off;
	UINT32 emi_remap_offset;
	P_EMI_CTRL_STATE_OFFSET p_ecso;
	UINT32 emi_size;
	UINT32 pda_dl_patch_flag;
	UINT32 emi_met_size;
	UINT32 emi_met_data_offset;
	UINT32 emi_core_dump_offset;
	UINT32 emi_direct_path_ap_phy_addr;
	UINT32 emi_direct_path_size;
	UINT32 emi_ram_bt_buildtime_offset;
	UINT32 emi_ram_wifi_buildtime_offset;
	UINT32 emi_ram_mcu_buildtime_offset;
	UINT32 emi_patch_mcu_buildtime_offset;
} CONSYS_EMI_ADDR_INFO, *P_CONSYS_EMI_ADDR_INFO;

typedef struct _GPIO_TDM_REQ_INFO_ {
	UINT32 ant_sel_index;
	UINT32 gpio_number;
	UINT32 cr_address;
} GPIO_TDM_REQ_INFO, *P_GPIO_TDM_REQ_INFO;

struct consys_sw_state {
	UINT16 clock_hif_ctrl;
	UINT16 clock_umac_ctrl;
	UINT32 resource_disable_sleep;
	UINT32 clock_mcu;
	UINT32 info_time;
	UINT8 is_gating;
	UINT8 sub_system;
};

typedef struct consys_state {
	UINT32 lp[2];
	UINT32 gating[2];
	UINT64 sleep_counter[WMT_SLEEP_COUNT_MAX];
	UINT64 sleep_timer[WMT_SLEEP_COUNT_MAX];
	struct consys_sw_state sw_state;
} CONSYS_STATE, *P_CONSYS_STATE;


typedef VOID(*irq_cb) (VOID);
typedef INT32(*device_audio_if_cb) (enum CMB_STUB_AIF_X aif, MTK_WCN_BOOL share);
typedef VOID(*func_ctrl_cb) (UINT32 on, UINT32 type);
typedef long (*thermal_query_ctrl_cb) (VOID);
typedef INT32(*trigger_assert_cb) (UINT32 type, UINT32 reason);
typedef INT32(*deep_idle_ctrl_cb) (UINT32);

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/
extern INT32 gWmtDbgLvl;
extern struct device *wmt_dev;
#ifdef CFG_WMT_READ_EFUSE_VCN33
extern INT32 wmt_set_pmic_voltage(UINT32 level);
#endif
/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

INT32 wmt_plat_init(P_PWR_SEQ_TIME pPwrSeqTime, UINT32 co_clock_type);
INT32 wmt_plat_deinit(VOID);
INT32 wmt_plat_merge_if_flag_get(VOID);
INT32 wmt_plat_set_comm_if_type(ENUM_STP_TX_IF_TYPE type);
INT32 wmt_plat_merge_if_flag_ctrl(UINT32 enagle);
ENUM_STP_TX_IF_TYPE wmt_plat_get_comm_if_type(VOID);

INT32 wmt_plat_pwr_ctrl(ENUM_FUNC_STATE state);

INT32 wmt_plat_gpio_ctrl(ENUM_PIN_ID id, ENUM_PIN_STATE state);

INT32 wmt_plat_eirq_ctrl(ENUM_PIN_ID id, ENUM_PIN_STATE state);

INT32 wmt_plat_wake_lock_ctrl(ENUM_WL_OP opId);
INT32 wmt_plat_sdio_ctrl(UINT32 sdioPortNum, ENUM_FUNC_STATE on);

INT32 wmt_plat_audio_ctrl(enum CMB_STUB_AIF_X state, enum CMB_STUB_AIF_CTRL ctrl);
VOID wmt_lib_plat_irq_cb_reg(irq_cb bgf_irq_cb);
VOID wmt_lib_plat_aif_cb_reg(device_audio_if_cb aif_ctrl_cb);

VOID wmt_plat_irq_cb_reg(irq_cb bgf_irq_cb);
VOID wmt_plat_aif_cb_reg(device_audio_if_cb aif_ctrl_cb);
VOID wmt_plat_func_ctrl_cb_reg(func_ctrl_cb subsys_func_ctrl);
VOID wmt_plat_thermal_ctrl_cb_reg(thermal_query_ctrl_cb thermal_query_ctrl);
VOID wmt_plat_trigger_assert_cb_reg(trigger_assert_cb trigger_assert);
VOID wmt_plat_deep_idle_ctrl_cb_reg(deep_idle_ctrl_cb deep_idle_ctrl);

INT32 wmt_plat_soc_paldo_ctrl(ENUM_PALDO_TYPE ePt, ENUM_PALDO_OP ePo);
UINT8 *wmt_plat_get_emi_virt_add(UINT32 offset);
#if CONSYS_ENALBE_SET_JTAG
UINT32 wmt_plat_jtag_flag_ctrl(UINT32 en);
#endif
#if CFG_WMT_DUMP_INT_STATUS
VOID wmt_plat_BGF_irq_dump_status(VOID);
MTK_WCN_BOOL wmt_plat_dump_BGF_irq_status(VOID);
#endif
P_CONSYS_EMI_ADDR_INFO wmt_plat_get_emi_phy_add(VOID);
UINT32 wmt_plat_read_cpupcr(VOID);
UINT32 wmt_plat_read_dmaregs(UINT32);
INT32 wmt_plat_set_host_dump_state(ENUM_HOST_DUMP_STATE state);
UINT32 wmt_plat_force_trigger_assert(ENUM_FORCE_TRG_ASSERT_T type);
INT32 wmt_plat_update_host_sync_num(VOID);
INT32 wmt_plat_get_dump_info(UINT32 offset);
INT32 wmt_plat_write_emi_l(UINT32 offset, UINT32 value);
UINT32 wmt_plat_get_soc_chipid(VOID);
INT32 wmt_plat_get_adie_chipid(VOID);
UINT32 wmt_plat_soc_co_clock_flag_get(VOID);
INT32 wmt_plat_set_dbg_mode(UINT32 flag);
INT32 wmt_plat_set_dynamic_dumpmem(PUINT32 buf);
#if CFG_WMT_LTE_COEX_HANDLING
INT32 wmt_plat_get_tdm_antsel_index(VOID);
#endif
INT32 wmt_plat_consys_hw_init(VOID);
/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

#endif /* _WMT_PLAT_H_ */
