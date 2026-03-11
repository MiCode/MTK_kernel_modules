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
int coredump_mng_init(void* plat_data)
{
    if (consys_platform_coredump_ops == NULL) {
        consys_platform_coredump_ops =
            (const struct consys_platform_coredump_ops*)plat_data;
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

unsigned int coredump_mng_get_emi_offset(int conn_type)
{
    if (consys_platform_coredump_ops &&
        consys_platform_coredump_ops->consys_coredump_get_emi_offset)
        return consys_platform_coredump_ops->consys_coredump_get_emi_offset(conn_type);

    return -1;
}

void coredump_mng_get_emi_phy_addr(phys_addr_t* base, unsigned int *size)
{
    if (consys_platform_coredump_ops &&
        consys_platform_coredump_ops->consys_coredump_get_emi_phy_addr)
        consys_platform_coredump_ops->consys_coredump_get_emi_phy_addr(base, size);
}

void coredump_mng_get_mcif_emi_phy_addr(phys_addr_t* base, unsigned int *size)
{
    if (consys_platform_coredump_ops &&
        consys_platform_coredump_ops->consys_coredump_get_mcif_emi_phy_addr)
        consys_platform_coredump_ops->consys_coredump_get_mcif_emi_phy_addr(base, size);
}

int coredump_mng_setup_dump_region(int conn_type)
{
    if (consys_platform_coredump_ops &&
        consys_platform_coredump_ops->consys_coredump_setup_dump_region)
        return consys_platform_coredump_ops->consys_coredump_setup_dump_region(conn_type);

    return -1;
}

unsigned int coredump_mng_setup_dynamic_remap(int conn_type, unsigned int idx, unsigned int base, unsigned int length)
{
    if (consys_platform_coredump_ops &&
        consys_platform_coredump_ops->consys_coredump_setup_dynamic_remap)
        return consys_platform_coredump_ops->consys_coredump_setup_dynamic_remap(conn_type, idx, base, length);

    return -1;
}

void __iomem* coredump_mng_remap(int conn_type, unsigned int base, unsigned int length)
{
	void __iomem* vir_addr = 0;

    if (consys_platform_coredump_ops &&
        consys_platform_coredump_ops->consys_coredump_remap)
        return consys_platform_coredump_ops->consys_coredump_remap(conn_type, base, length);

    return vir_addr;
}

void coredump_mng_unmap(void __iomem* vir_addr)
{
    if (consys_platform_coredump_ops &&
        consys_platform_coredump_ops->consys_coredump_unmap)
        consys_platform_coredump_ops->consys_coredump_unmap(vir_addr);
}

char* coredump_mng_get_tag_name(int conn_type)
{
    if (consys_platform_coredump_ops &&
        consys_platform_coredump_ops->consys_coredump_get_tag_name)
        return consys_platform_coredump_ops->consys_coredump_get_tag_name(conn_type);

    return 0;
}

bool coredump_mng_is_supported(unsigned int drv)
{
	if (consys_platform_coredump_ops &&
	    consys_platform_coredump_ops->consys_coredump_is_supported)
		return consys_platform_coredump_ops->consys_coredump_is_supported(drv);

	return true;
}

void coredump_mng_get_emi_dump_offset(unsigned int *start, unsigned int *end)
{
	phys_addr_t base;
	unsigned int size = 0;

	if (consys_platform_coredump_ops &&
	    consys_platform_coredump_ops->consys_coredump_get_emi_dump_offset)
		consys_platform_coredump_ops->consys_coredump_get_emi_dump_offset(start, end);
	else {
		if (start)
			*start = 0;
		if (end) {
			coredump_mng_get_emi_phy_addr(&base, &size);
			*end = size;
		}
	}
}
