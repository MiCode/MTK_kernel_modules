// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "coredump_mng.h"

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

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/
const struct consys_platform_coredump_ops* consys_platform_coredump_ops = NULL;

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/
int coredump_mng_init(const struct conninfra_plat_data* plat_data)
{
    if (consys_platform_coredump_ops == NULL) {
        consys_platform_coredump_ops =
            (const struct consys_platform_coredump_ops*)plat_data->platform_coredump_ops;
    }

    return 0;
}

int coredump_mng_deinit(void)
{
    consys_platform_coredump_ops = NULL;
    return 0;
}

struct coredump_hw_config* coredump_mng_get_platform_config(int conn_type)
{
    if (consys_platform_coredump_ops &&
        consys_platform_coredump_ops->consys_coredump_get_platform_config)
        return consys_platform_coredump_ops->consys_coredump_get_platform_config(conn_type);

    return NULL;
}

unsigned int coredump_mng_get_platform_chipid(void)
{
    if (consys_platform_coredump_ops &&
        consys_platform_coredump_ops->consys_coredump_get_platform_chipid)
        return consys_platform_coredump_ops->consys_coredump_get_platform_chipid();

    return 0;
}

char* coredump_mng_get_task_string(int conn_type, unsigned int task_id)
{
    if (consys_platform_coredump_ops &&
        consys_platform_coredump_ops->consys_coredump_get_task_string)
        return consys_platform_coredump_ops->consys_coredump_get_task_string(conn_type, task_id);

    return NULL;
}

char* coredump_mng_get_sys_name(int conn_type)
{
    if (consys_platform_coredump_ops &&
        consys_platform_coredump_ops->consys_coredump_get_sys_name)
        return consys_platform_coredump_ops->consys_coredump_get_sys_name(conn_type);

    return NULL;
}

bool coredump_mng_is_host_view_cr(unsigned int addr, unsigned int* host_view)
{
    if (consys_platform_coredump_ops &&
        consys_platform_coredump_ops->consys_coredump_is_host_view_cr)
        return consys_platform_coredump_ops->consys_coredump_is_host_view_cr(addr, host_view);

    return false;
}

bool coredump_mng_is_host_csr_readable(void)
{
    if (consys_platform_coredump_ops &&
        consys_platform_coredump_ops->consys_coredump_is_host_csr_readable)
        return consys_platform_coredump_ops->consys_coredump_is_host_csr_readable();

    return false;
}

enum cr_category coredump_mng_get_cr_category(unsigned int addr)
{
    if (consys_platform_coredump_ops &&
        consys_platform_coredump_ops->consys_coredump_get_cr_category)
        return consys_platform_coredump_ops->consys_coredump_get_cr_category(addr);

    return SUBSYS_CR;
}
