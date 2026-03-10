/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include "precomp.h"
#include <linux/platform_device.h>
#if (CFG_SUPPORT_CONNINFRA == 1)
#include "conninfra.h"
#endif

static u_int8_t emi_is_remap_type(enum EMI_ALLOC_TYPE type)
{
	if (type == EMI_ALLOC_TYPE_WMT ||
	    type == EMI_ALLOC_TYPE_CONNINFRA ||
	    type == EMI_ALLOC_TYPE_LK)
		return TRUE;
	else
		return FALSE;
}

static int32_t emi_init_type_lk(void *dev,
	phys_addr_t *pa,
	uint32_t *size)
{
#ifdef CONFIG_OF
	struct platform_device *pdev = dev;
	struct device_node *node = pdev->dev.of_node;
	u64 phy_addr = 0;
	unsigned int phy_size = 0;

	if (!node)
		return -1;

	if (of_property_read_u64(node, "conninfra-emi-addr", &phy_addr))
		return -1;

	if (of_property_read_u32(node, "conninfra-emi-size", &phy_size))
		return -1;

	of_node_put(node);

	if (phy_addr == 0 && phy_size <= 0) {
		DBGLOG(INIT, ERROR,
			"init failed, phy_addr: 0x%x phy_size: 0x%x.\n",
			phy_addr, phy_size);
		return -1;
	}

	*pa = phy_addr;
	*size = phy_size;
	return 0;
#else
	DBGLOG(INIT, ERROR, "kernel option CONFIG_OF not enabled.\n");
	return -1;
#endif
}

static void emi_uninit_type_lk(void *dev,
	phys_addr_t pa,
	void *va,
	uint32_t size)
{
#ifdef CONFIG_OF
	iounmap(va);
	release_mem_region(pa, size);
#endif
}

static int32_t emi_init_type_dma(void *dev,
	phys_addr_t *pa,
	void **va,
	uint32_t *size)
{
#define MCU_EMI_SIZE		(2 * 64 * 1024)

	dma_addr_t phy_addr;
	void *vir_addr;

	vir_addr = KAL_DMA_ALLOC_COHERENT(dev, MCU_EMI_SIZE,
		&phy_addr);
	if (vir_addr == NULL)
		return -ENOMEM;

	*va = vir_addr;
	*pa = (phys_addr_t)phy_addr;
	*size = MCU_EMI_SIZE;

	return 0;
}

static void emi_uninit_type_dma(void *dev,
	phys_addr_t pa,
	void *va,
	uint32_t size)
{
	if (va == NULL)
		return;

	KAL_DMA_FREE_COHERENT(dev, size, va, (dma_addr_t)pa);
}

int32_t emi_mem_init(struct mt66xx_chip_info *chip, void *dev)
{
	struct EMI_MEM_INFO *emi = &chip->rEmiInfo;
	int32_t ret = 0;

#if defined(UT_TEST_MODE) && defined(CFG_BUILD_X86_PLATFORM)
	/* Refer 6765 dts setting */
	emi->size = 0x400000;
	emi->va = kmalloc(emi->size, GFP_KERNEL);
	emi->pa = (phys_addr_t)emi->va;
	memset(emi->va, 0, emi->size);
#else
	switch (emi->type) {
	case EMI_ALLOC_TYPE_IN_DRIVER:
		ret = emi_init_type_dma(dev,
			&emi->pa, &emi->va, &emi->size);
		break;
	case EMI_ALLOC_TYPE_WMT:
#if CFG_MTK_ANDROID_WMT
#if IS_ENABLED(CFG_SUPPORT_CONNAC1X)
		emi->pa = gConEmiPhyBase;
		emi->size = gConEmiSize;
#endif
#endif
		break;
	case EMI_ALLOC_TYPE_CONNINFRA:
#if CFG_MTK_ANDROID_WMT
#if (CFG_SUPPORT_CONNINFRA == 1)
		conninfra_get_phy_addr(
			&emi->pa,
			&emi->size);
#endif
#endif
		break;
	case EMI_ALLOC_TYPE_LK:
		ret = emi_init_type_lk(dev, &emi->pa, &emi->size);
		break;
	default:
		DBGLOG(HAL, ERROR, "unexpected type: %d\n",
			emi->type);
		goto exit;
	}

	if (ret)
		goto exit;

	if (emi_is_remap_type(emi->type)) {
		struct resource *res;

		res = request_mem_region(emi->pa, emi->size, EMI_NAME);
		if (!res) {
			DBGLOG(HAL, ERROR,
				"request_mem_region failed, pa(0x%llx) size(0x%x) name(%s)\n",
				(uint64_t)emi->pa,
				emi->size,
				EMI_NAME);
			ret = -EBUSY;
			goto exit;
		}
		emi->va = ioremap(emi->pa, emi->size);
	}

	if (emi->va == NULL) {
		DBGLOG(INIT, ERROR, "Alloc emi mem failed\n");
		ret = -ENOMEM;
		goto exit;
	}
#endif

	DBGLOG(HAL, INFO, "type: %d, emi pa=0x%x va=0x%x size=0x%x\n",
		emi->type,
		emi->pa,
		emi->va,
		emi->size);

	emi->initialized = TRUE;

exit:
	return ret;
}

void emi_mem_uninit(struct mt66xx_chip_info *chip, void *dev)
{
	struct EMI_MEM_INFO *emi = NULL;

	emi = &chip->rEmiInfo;

	if (!emi->initialized)
		return;

	DBGLOG(HAL, INFO, "type: %d, emi pa=0x%x va=0x%x size=0x%x\n",
		emi->type,
		emi->pa,
		emi->va,
		emi->size);

	emi->initialized = FALSE;

#if defined(UT_TEST_MODE) && defined(CFG_BUILD_X86_PLATFORM)
	kfree(emi->va);
#else
	switch (emi->type) {
	case EMI_ALLOC_TYPE_IN_DRIVER:
		emi_uninit_type_dma(dev,
			emi->pa, emi->va, emi->size);
		break;
	default:
		if (emi_is_remap_type(emi->type)) {
			iounmap(emi->va);
			release_mem_region(emi->pa, emi->size);
		}
		break;
	}
#endif
}

static u_int8_t __emi_mem_sanity_chk(struct mt66xx_chip_info *chip,
	uint32_t offset, void *buf, uint32_t size)
{
	if (!buf)
		return FALSE;
	else if (size > emi_mem_get_size(chip))
		return FALSE;
	else if ((offset + size) > emi_mem_get_size(chip))
		return FALSE;

	return TRUE;
}

int32_t emi_mem_write(struct mt66xx_chip_info *chip,
	uint32_t offset, void *buf, uint32_t size)
{
	struct EMI_MEM_INFO *emi = NULL;

	emi = &chip->rEmiInfo;

	if (!emi->initialized)
		return -EINVAL;

	offset = emi_mem_offset_convert(offset);

	if (!__emi_mem_sanity_chk(chip, offset, buf, size))
		return -EINVAL;

	if (emi_is_remap_type(emi->type)) {
#if CFG_MTK_ANDROID_EMI
		kalSetEmiMpuProtection(emi->pa, false);
#endif

		kalMemCopyToIo(emi->va + offset, buf, size);

#if CFG_MTK_ANDROID_EMI
		kalSetEmiMpuProtection(emi->pa, true);
#endif
	} else {
		kalMemCopy(emi->va + offset, buf, size);
	}

	return 0;
}

int32_t emi_mem_read(struct mt66xx_chip_info *chip,
	uint32_t offset, void *buf, uint32_t size)
{
	struct EMI_MEM_INFO *emi = NULL;

	emi = &chip->rEmiInfo;

	if (!emi->initialized)
		return -EINVAL;

	offset = emi_mem_offset_convert(offset);

	if (!__emi_mem_sanity_chk(chip, offset, buf, size))
		return -EINVAL;

	if (emi_is_remap_type(emi->type)) {
#if CFG_MTK_ANDROID_EMI
		kalSetEmiMpuProtection(emi->pa, false);
#endif

		kalMemCopyFromIo(buf, emi->va + offset, size);

#if CFG_MTK_ANDROID_EMI
		kalSetEmiMpuProtection(emi->pa, true);
#endif
	} else {
		kalMemCopy(buf, emi->va + offset, size);
	}

	return 0;
}

void *emi_mem_get_vir_base(struct mt66xx_chip_info *chip)
{
	struct EMI_MEM_INFO *emi = NULL;

	emi = &chip->rEmiInfo;

	if (!emi->initialized)
		return NULL;

	return emi->va;
}

phys_addr_t emi_mem_get_phy_base(struct mt66xx_chip_info *chip)
{
	struct EMI_MEM_INFO *emi = NULL;

	emi = &chip->rEmiInfo;

	if (!emi->initialized)
		return 0;

	return emi->pa;
}

uint32_t emi_mem_get_size(struct mt66xx_chip_info *chip)
{
	struct EMI_MEM_INFO *emi = NULL;

	emi = &chip->rEmiInfo;

	if (!emi->initialized)
		return 0;

	return emi->size;
}

uint32_t emi_mem_offset_convert(uint32_t offset)
{
	return offset & WIFI_EMI_ADDR_MASK;
}

