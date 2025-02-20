/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2024 MediaTek Inc.
 */

#ifndef MT6899_EMI_H
#define MT6899_EMI_H

int consys_emi_mpu_set_region_protection_mt6899(void);
void consys_emi_get_md_shared_emi_mt6899(phys_addr_t* base, unsigned int* size);

#endif /* MT6899_EMI_H */
