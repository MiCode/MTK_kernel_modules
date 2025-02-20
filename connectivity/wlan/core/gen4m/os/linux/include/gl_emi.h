/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _GL_EMI_H
#define _GL_EMI_H

#if CFG_MTK_ANDROID_WMT
#if CFG_SUPPORT_CONNAC1X
extern phys_addr_t gConEmiPhyBase;
extern unsigned long long gConEmiSize;
#endif
#endif

#define EMI_NAME		"WIFI-EMI"
/* connsys hw maximum capability: 64MB */
#define WIFI_EMI_ADDR_MASK	0x3FFFFFF

enum EMI_ALLOC_TYPE {
	EMI_ALLOC_TYPE_NONE,
	EMI_ALLOC_TYPE_WMT,
	EMI_ALLOC_TYPE_CONNINFRA,
	EMI_ALLOC_TYPE_IN_DRIVER,
	EMI_ALLOC_TYPE_LK,
	EMI_ALLOC_TYPE_DTS,
	EMI_ALLOC_TYPE_NUM
};

struct EMI_MEM_INFO {
	const enum EMI_ALLOC_TYPE type;
	const uint32_t coredump_size;
	const uint32_t coredump2_size;
	phys_addr_t pa;
	void *va;
	uint32_t size;
	uint8_t initialized;
	uint8_t isReqMemRegSuccess;
};

struct mt66xx_chip_info;

int32_t emi_mem_init(struct mt66xx_chip_info *chip, void *dev);
void emi_mem_uninit(struct mt66xx_chip_info *chip, void *dev);
int32_t emi_mem_write(struct mt66xx_chip_info *chip,
	uint32_t offset, void *buf, uint32_t size);
int32_t emi_mem_read(struct mt66xx_chip_info *chip,
	uint32_t offset, void *buf, uint32_t size);
void *emi_mem_get_vir_base(struct mt66xx_chip_info *chip);
phys_addr_t emi_mem_get_phy_base(struct mt66xx_chip_info *chip);
uint32_t emi_mem_get_size(struct mt66xx_chip_info *chip);
uint32_t emi_mem_offset_convert(uint32_t offset);

#endif /* _GL_EMI_H */
