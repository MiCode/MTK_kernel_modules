/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2025 MediaTek Inc.
 */

#ifndef MT6858_EMI_H
#define MT6858_EMI_H

int consys_emi_mpu_set_region_protection_mt6858(void);
void consys_emi_get_md_shared_emi_mt6858(phys_addr_t* base, unsigned int* size);

#endif /* MT6858_EMI_H */
