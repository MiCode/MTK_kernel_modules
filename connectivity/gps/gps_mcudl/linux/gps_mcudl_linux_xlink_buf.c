/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <linux/version.h>
#include <linux/dma-mapping.h>
#include <linux/of_device.h>
#include <linux/device.h>
#include <linux/platform_device.h>

#include "gps_mcudl_config.h"
#include "gps_mcudl_log.h"
#include "gps_mcudl_context.h"
#include "gps_mcudl_each_link.h"
#include "gps_mcudl_xlink_buf.h"

void gps_mcudl_dma_buf_free(struct gps_dl_dma_buf *p_dma_buf, enum gps_mcudl_xid x_id)
{
	struct gps_mcudl_each_device *p_dev;

	p_dev = gps_mcudl_device_get(x_id);
	if (p_dev == NULL) {
		MDL_LOGXE_INI(x_id, "gps_mcudl_device_get return null");
		return;
	}

	if (p_dma_buf->vir_addr) {
#if 0
		dma_free_coherent(p_dev->dev,
			p_dma_buf->len, p_dma_buf->vir_addr, p_dma_buf->phy_addr);
#else
		kfree(p_dma_buf->vir_addr);
#endif
	}

	memset(p_dma_buf, 0, sizeof(*p_dma_buf));
}

int gps_mcudl_dma_buf_alloc(struct gps_dl_dma_buf *p_dma_buf, enum gps_mcudl_xid x_id,
	enum gps_dl_dma_dir dir, unsigned int len)
{
	struct gps_mcudl_each_device *p_dev = NULL;
	struct device *p_linux_plat_dev = NULL;

	p_dev = gps_mcudl_device_get(x_id);
	if (p_dev == NULL) {
		MDL_LOGXE_INI(x_id, "gps_mcudl_device_get return null");
		return -1;
	}

	p_linux_plat_dev = (struct device *)p_dev->private_data;

	memset(p_dma_buf, 0, sizeof(*p_dma_buf));
	p_dma_buf->dev_index = x_id;
	p_dma_buf->dir = dir;
	p_dma_buf->len = len;
	p_dma_buf->is_for_mcudl = true;
	p_dma_buf->entry_l = GPS_MCUDL_DMA_BUF_ENTRY_MAX;

#if 0
	MDL_LOGI_INI("p_linux_plat_dev = 0x%p", p_linux_plat_dev);
	if (p_linux_plat_dev == NULL) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		p_dma_buf->vir_addr = dma_alloc_coherent(
			p_dev->dev, len, &p_dma_buf->phy_addr, GFP_DMA | GFP_DMA32 | __GFP_ZERO);
#else
		p_dma_buf->vir_addr = dma_zalloc_coherent(
			p_dev->dev, len, &p_dma_buf->phy_addr, GFP_DMA | GFP_DMA32);
#endif

	} else {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		p_dma_buf->vir_addr = dma_alloc_coherent(
			p_linux_plat_dev, len, &p_dma_buf->phy_addr, GFP_DMA | __GFP_ZERO);/* | GFP_DMA32); */
#else
		p_dma_buf->vir_addr = dma_zalloc_coherent(
			p_linux_plat_dev, len, &p_dma_buf->phy_addr, GFP_DMA);/* | GFP_DMA32); */
#endif
	}
#else
	/* for mcudl, no need to use phy_addr, so we can use kzalloc */
	p_dma_buf->vir_addr = kzalloc(len, GFP_KERNEL);
	p_dma_buf->phy_addr = 0;
#endif
	MDL_LOGD_INI(
#if GPS_DL_ON_LINUX
		"alloc gps mcudl dma buf(%d,%d), addr: vir=0x%p, phy=0x%llx, len=%u",
#else
		"alloc gps mcudl dma buf(%d,%d), addr: vir=0x%p, phy=0x%08x, len=%u",
#endif
		p_dma_buf->dev_index, p_dma_buf->dir,
		p_dma_buf->vir_addr, p_dma_buf->phy_addr, p_dma_buf->len);

	if (NULL == p_dma_buf->vir_addr) {
		GDL_LOGXE_INI(x_id,
			"alloc gps mcudl dma buf(%d,%d)(len = %u) fail", x_id, dir, len);
		/* force move forward even fail */
		/* return -ENOMEM; */
	}

	return 0;
}

int gps_mcudl_dma_buf_alloc2(enum gps_mcudl_xid x_id)
{
	int retval;
	struct gps_mcudl_each_device *p_dev;
	struct gps_mcudl_each_link *p_link;

	p_dev = gps_mcudl_device_get(x_id);
	p_link = gps_mcudl_link_get(x_id);
	if (p_dev == NULL) {
		MDL_LOGXE_INI(x_id, "gps_mcudl_device_get return null");
		return -1;
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
	of_dma_configure(p_dev->dev, p_dev->dev->of_node, false);
#else
	of_dma_configure(p_dev->dev, p_dev->dev->of_node);
#endif

	if (!p_dev->dev->coherent_dma_mask)
		p_dev->dev->coherent_dma_mask = DMA_BIT_MASK(32);

	if (!p_dev->dev->dma_mask)
		p_dev->dev->dma_mask = &p_dev->dev->coherent_dma_mask;

	retval = gps_mcudl_dma_buf_alloc(
		&p_link->tx_dma_buf, x_id, GDL_DMA_A2D, p_link->cfg.tx_buf_size);
	if (retval)
		return retval;

	retval = gps_mcudl_dma_buf_alloc(
		&p_link->rx_dma_buf, x_id, GDL_DMA_D2A, p_link->cfg.rx_buf_size);
	if (retval)
		return retval;

	return 0;
}

