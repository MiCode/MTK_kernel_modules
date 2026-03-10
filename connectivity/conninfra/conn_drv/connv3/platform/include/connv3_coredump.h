// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _PLATFORM_COREDUMP_MNG_H_
#define _PLATFORM_COREDUMP_MNG_H_

#include <linux/platform_device.h>
#include "consys_hw.h"
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

struct consys_platform_coredump_ops {
    CONSYS_COREDUMP_GET_PLATFORM_CONFIG consys_coredump_get_platform_config;
    CONSYS_COREDUMP_GET_PLATFORM_CHIPID consys_coredump_get_platform_chipid;
    CONSYS_COREDUMP_GET_TASK_STRING consys_coredump_get_task_string;
    CONSYS_COREDUMP_GET_SYS_NAME consys_coredump_get_sys_name;
    CONSYS_COREDUMP_IS_HOST_VIEW_CR consys_coredump_is_host_view_cr;
    CONSYS_COREDUMP_IS_HOST_CSR_READABLE consys_coredump_is_host_csr_readable;
    CONSYS_COREDUMP_GET_CR_CATEGORY consys_coredump_get_cr_category;
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
int coredump_mng_init(const struct conninfra_plat_data* plat_data);
int coredump_mng_deinit(void);
struct coredump_hw_config* coredump_mng_get_platform_config(int conn_type);
unsigned int coredump_mng_get_platform_chipid(void);
char* coredump_mng_get_task_string(int conn_type, unsigned int task_id);
char* coredump_mng_get_sys_name(int conn_type);
bool coredump_mng_is_host_view_cr(unsigned int addr, unsigned int* host_view);
bool coredump_mng_is_host_csr_readable(void);
enum cr_category coredump_mng_get_cr_category(unsigned int addr);

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

#endif              /* _PLATFORM_COREDUMP_MNG_H_ */
