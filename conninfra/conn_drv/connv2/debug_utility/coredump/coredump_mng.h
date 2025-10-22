// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _PLATFORM_COREDUMP_MNG_H_
#define _PLATFORM_COREDUMP_MNG_H_

#include <linux/platform_device.h>
#include "connsys_coredump_hw_config.h"

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
typedef struct coredump_hw_config*(*CONSYS_COREDUMP_GET_PLATFORM_CONFIG) (int conn_type);
typedef unsigned int(*CONSYS_COREDUMP_GET_PLATFORM_CHIPID) (void);
typedef char*(*CONSYS_COREDUMP_GET_TASK_STRING) (int conn_type, unsigned int task_id);
typedef char*(*CONSYS_COREDUMP_GET_SYS_NAME) (int conn_type);
typedef bool(*CONSYS_COREDUMP_IS_HOST_VIEW_CR) (unsigned int addr, unsigned int* host_view);
typedef bool(*CONSYS_COREDUMP_IS_HOST_CSR_READABLE) (void);
typedef enum cr_category(*CONSYS_COREDUMP_GET_CR_CATEGORY) (unsigned int addr);
typedef unsigned int(*CONSYS_COREDUMP_GET_EMI_OFFSET) (int conn_type);
typedef void (*CONSYS_COREDUMP_GET_EMI_PHY_ADDR)(phys_addr_t* base, unsigned int *size);
typedef void (*CONSYS_COREDUMP_GET_MCIF_EMI_PHY_ADDR)(phys_addr_t* base, unsigned int *size);
typedef int (*CONSYS_COREDUMP_SETUP_DUMP_REGION)(int conn_type);
typedef unsigned int (*CONSYS_COREDUMP_SETUP_DYNAMIC_REMAP)(int conn_type, unsigned int idx, unsigned int base, unsigned int length);
typedef void __iomem*(*CONSYS_COREDUMP_REMAP)(int conn_type, unsigned int base, unsigned int length);
typedef void (*CONSYS_COREDUMP_UNMAP)(void __iomem* vir_addr);
typedef char*(*CONSYS_COREDUMP_GET_TAG_NAME)(int conn_type);
typedef bool(*CONSYS_COREDUMP_IS_SUPPORTED)(unsigned int conn_type);
typedef void(*CONSYS_COREDUMP_GET_EMI_DUMP_OFFSET)(unsigned int *start, unsigned int *end);

struct consys_platform_coredump_ops {
	CONSYS_COREDUMP_GET_PLATFORM_CONFIG consys_coredump_get_platform_config;
	CONSYS_COREDUMP_GET_PLATFORM_CHIPID consys_coredump_get_platform_chipid;
	CONSYS_COREDUMP_GET_TASK_STRING consys_coredump_get_task_string;
	CONSYS_COREDUMP_GET_SYS_NAME consys_coredump_get_sys_name;
	CONSYS_COREDUMP_IS_HOST_VIEW_CR consys_coredump_is_host_view_cr;
	CONSYS_COREDUMP_IS_HOST_CSR_READABLE consys_coredump_is_host_csr_readable;
	CONSYS_COREDUMP_GET_CR_CATEGORY consys_coredump_get_cr_category;
	CONSYS_COREDUMP_GET_EMI_OFFSET consys_coredump_get_emi_offset;
	CONSYS_COREDUMP_GET_EMI_PHY_ADDR consys_coredump_get_emi_phy_addr;
	CONSYS_COREDUMP_GET_MCIF_EMI_PHY_ADDR consys_coredump_get_mcif_emi_phy_addr;
	CONSYS_COREDUMP_SETUP_DUMP_REGION consys_coredump_setup_dump_region;
	CONSYS_COREDUMP_SETUP_DYNAMIC_REMAP consys_coredump_setup_dynamic_remap;
	CONSYS_COREDUMP_REMAP consys_coredump_remap;
	CONSYS_COREDUMP_UNMAP consys_coredump_unmap;
	CONSYS_COREDUMP_GET_TAG_NAME consys_coredump_get_tag_name;
	CONSYS_COREDUMP_IS_SUPPORTED consys_coredump_is_supported;
	CONSYS_COREDUMP_GET_EMI_DUMP_OFFSET consys_coredump_get_emi_dump_offset;
};

/********************************************************************************
*/

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
int coredump_mng_init(void* plat_data);
int coredump_mng_deinit(void);
struct coredump_hw_config* coredump_mng_get_platform_config(int conn_type);
unsigned int coredump_mng_get_platform_chipid(void);
char* coredump_mng_get_task_string(int conn_type, unsigned int task_id);
char* coredump_mng_get_sys_name(int conn_type);
bool coredump_mng_is_host_view_cr(unsigned int addr, unsigned int* host_view);
bool coredump_mng_is_host_csr_readable(void);
enum cr_category coredump_mng_get_cr_category(unsigned int addr);
unsigned int coredump_mng_get_emi_offset(int conn_type);
void coredump_mng_get_emi_phy_addr(phys_addr_t* base, unsigned int *size);
void coredump_mng_get_mcif_emi_phy_addr(phys_addr_t* base, unsigned int *size);
int coredump_mng_setup_dump_region(int conn_type);
unsigned int coredump_mng_setup_dynamic_remap(int conn_type, unsigned int idx, unsigned int base, unsigned int length);
void __iomem* coredump_mng_remap(int conn_type, unsigned int base, unsigned int length);
void coredump_mng_unmap(void __iomem* vir_addr);
char* coredump_mng_get_tag_name(int conn_type);
bool coredump_mng_is_supported(unsigned int conn_type);
void coredump_mng_get_emi_dump_offset(unsigned int *start, unsigned int *end);

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

#endif              /* _PLATFORM_COREDUMP_MNG_H_ */
