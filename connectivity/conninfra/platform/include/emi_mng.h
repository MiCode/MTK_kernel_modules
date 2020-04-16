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

#ifndef _PLATFORM_EMI_MNG_H_
#define _PLATFORM_EMI_MNG_H_

#include <linux/types.h>
#include "osal.h"
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

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/
typedef enum _ENUM_EMI_CTRL_STATE_OFFSET_ {
	EXP_APMEM_CTRL_STATE = 0x0,
	EXP_APMEM_CTRL_HOST_SYNC_STATE = 0x4,
	EXP_APMEM_CTRL_HOST_SYNC_NUM = 0x8,
	EXP_APMEM_CTRL_CHIP_SYNC_STATE = 0xc,
	EXP_APMEM_CTRL_CHIP_SYNC_NUM = 0x10,
	EXP_APMEM_CTRL_CHIP_SYNC_ADDR = 0x14,
	EXP_APMEM_CTRL_CHIP_SYNC_LEN = 0x18,
	EXP_APMEM_CTRL_CHIP_PRINT_BUFF_START = 0x1c,
	EXP_APMEM_CTRL_CHIP_PRINT_BUFF_LEN = 0x20,
	EXP_APMEM_CTRL_CHIP_PRINT_BUFF_IDX = 0x24,
	EXP_APMEM_CTRL_CHIP_INT_STATUS = 0x28,
	EXP_APMEM_CTRL_CHIP_PAGED_DUMP_END = 0x2c,
	EXP_APMEM_CTRL_HOST_OUTBAND_ASSERT_W1 = 0x30,
	EXP_APMEM_CTRL_CHIP_PAGE_DUMP_NUM = 0x44,
	EXP_APMEM_CTRL_CHIP_FW_DBGLOG_MODE = 0x40,
	EXP_APMEM_CTRL_CHIP_DYNAMIC_DUMP = 0x48,
	EXP_APMEM_CTRL_ASSERT_FLAG = 0x100,
	EXP_APMEM_CTRL_MAX
} ENUM_EMI_CTRL_STATE_OFFSET, *P_ENUM_EMI_CTRL_STATE_OFFSET;

typedef struct _EMI_CTRL_STATE_OFFSET_ {
	unsigned int emi_apmem_ctrl_state;
	unsigned int emi_apmem_ctrl_host_sync_state;
	unsigned int emi_apmem_ctrl_host_sync_num;
	//unsigned int emi_apmem_ctrl_chip_sync_state;
	//unsigned int emi_apmem_ctrl_chip_sync_num;
	//unsigned int emi_apmem_ctrl_chip_sync_addr;
	//unsigned int emi_apmem_ctrl_chip_sync_len;
	//unsigned int emi_apmem_ctrl_chip_print_buff_start;
	//unsigned int emi_apmem_ctrl_chip_print_buff_len;
	//unsigned int emi_apmem_ctrl_chip_print_buff_idx;
	//unsigned int emi_apmem_ctrl_chip_int_status;
	//unsigned int emi_apmem_ctrl_chip_paded_dump_end;
	//unsigned int emi_apmem_ctrl_host_outband_assert_w1;
	//unsigned int emi_apmem_ctrl_chip_page_dump_num;
	//unsigned int emi_apmem_ctrl_assert_flag;
} EMI_CTRL_STATE_OFFSET, *P_EMI_CTRL_STATE_OFFSET;


struct consys_emi_addr_info {
	unsigned int emi_ap_phy_addr;
	unsigned int emi_size;
#if 0
	unsigned int emi_phy_addr;
	unsigned int emi_ap_phy_addr;
	unsigned int paged_trace_off;
	unsigned int paged_dump_off;
	unsigned int full_dump_off;
	unsigned int emi_remap_offset;
	P_EMI_CTRL_STATE_OFFSET p_ecso;
	unsigned int emi_size;
	unsigned int pda_dl_patch_flag;
	unsigned int emi_met_size;
	unsigned int emi_met_data_offset;
	unsigned int emi_core_dump_offset;
#endif
};

typedef int(*CONSYS_IC_EMI_MPU_SET_REGION_PROTECTION) (void);
typedef unsigned int(*CONSYS_IC_EMI_SET_REMAPPING_REG) (unsigned int);
typedef int(*CONSYS_IC_EMI_COREDUMP_REMAPPING) (unsigned char __iomem **addr, unsigned int enable);
typedef int(*CONSYS_IC_EMI_COREDUMP_RESET) (unsigned char __iomem *addr);
//typedef P_CONSYS_EMI_ADDR_INFO(*CONSYS_IC_EMI_GET_PHY_ADDR) (void);

struct consys_platform_emi_ops {
	CONSYS_IC_EMI_MPU_SET_REGION_PROTECTION consys_ic_emi_mpu_set_region_protection;
	CONSYS_IC_EMI_SET_REMAPPING_REG consys_ic_emi_set_remapping_reg;
	//CONSYS_IC_EMI_GET_PHY_ADDR consys_ic_emi_get_phy_addr;
	CONSYS_IC_EMI_COREDUMP_REMAPPING consys_ic_emi_coredump_remapping;
	CONSYS_IC_EMI_COREDUMP_RESET consys_ic_reset_emi_coredump;
};

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

//extern unsigned long long gConEmiSize;
//extern phys_addr_t gConEmiPhyBase;

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

int emi_mng_init(void);
int emi_mng_deinit(void);

int emi_mng_set_region_protection(void);
int emi_mng_set_remapping_reg(void);
struct consys_emi_addr_info* emi_mng_get_phy_addr(void);


/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

#endif				/* _PLATFORM_EMI_MNG_H_ */

