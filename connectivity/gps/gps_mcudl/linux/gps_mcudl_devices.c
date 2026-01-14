/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 - 2021 MediaTek Inc.
 */

#include <linux/dma-mapping.h>
#include <linux/of_device.h>

#include <linux/device.h>
#include <linux/platform_device.h>

#include "gps_dl_config.h"
#include "gps_dl_context.h"
#include "gps_dl_hw_api.h"
#include "gps_dl_isr.h"
#include "gps_data_link_devices.h"
/*#include "gps_mcudl_each_link.h"*/
#if GPS_DL_HAS_PLAT_DRV
#include "gps_dl_linux_plat_drv.h"
#include "gps_dl_linux_reserved_mem.h"
#endif
#if GPS_DL_MOCK_HAL
#include "gps_mock_hal.h"
#endif
#include "gps_dl_procfs.h"
#include "gps_dl_subsys_reset.h"
#include "gps_mcudl_context.h"
#include "gps_mcudl_each_link.h"
#include "gps_mcudl_xlink_buf.h"
#include "gps_mcusys_nv_data_api.h"
#include "gps_mcudl_hal_user_fw_own_ctrl.h"


#define GPS_MCUDL_DEV_NAME "gps_mcudl_cdev"
int gps_mcudl_devno_major;
int gps_mcudl_devno_minor;

#if 0
void gps_dl_dma_buf_free(struct gps_dl_dma_buf *p_dma_buf, enum gps_mcudl_xid xid)
{
	struct gps_mcudl_each_device *p_dev;

	p_dev = gps_dl_device_get(xid);
	if (p_dev == NULL) {
		GDL_LOGXE_INI(xid, "gps_dl_device_get return null");
		return;
	}

	if (p_dma_buf->vir_addr)
		dma_free_coherent(p_dev->dev,
			p_dma_buf->len, p_dma_buf->vir_addr, p_dma_buf->phy_addr);

	memset(p_dma_buf, 0, sizeof(*p_dma_buf));
}

int gps_dl_dma_buf_alloc(struct gps_dl_dma_buf *p_dma_buf, enum gps_mcudl_xid xid,
	enum gps_dl_dma_dir dir, unsigned int len)
{
	struct gps_mcudl_each_device *p_dev = NULL;
	struct device *p_dma_alloc_dev = NULL;
	gfp_t dma_alloc_flag;

	p_dev = gps_dl_device_get(xid);
	if (p_dev == NULL) {
		GDL_LOGXE_INI(xid, "gps_dl_device_get return null");
		return -1;
	}

	p_dma_alloc_dev = (struct device *)p_dev->private_data;
	if (p_dma_alloc_dev == NULL) {
		GDL_LOGI_INI("p_linux_plat_dev is null, set to p_dev->dev");
		p_dma_alloc_dev = p_dev->dev;
	}

	dma_alloc_flag = __GFP_ZERO;
	dma_alloc_flag |= GFP_KERNEL;

	memset(p_dma_buf, 0, sizeof(*p_dma_buf));
	p_dma_buf->dev_index = xid;
	p_dma_buf->dir = dir;
	p_dma_buf->len = len;

	GDL_LOGI_INI("dma_alloc_coherent: p_dev=0x%p, flag=0x%x, len=%u",
		p_dma_alloc_dev, dma_alloc_flag, len);
#if 0
	p_dma_buf->vir_addr = dma_alloc_coherent(
		p_dma_alloc_dev, len, &p_dma_buf->phy_addr, dma_alloc_flag);
#else
	p_dma_buf->vir_addr = kmalloc(len, GFP_KERNEL);
	p_dma_buf->phy_addr = 0ULL;
#endif
	GDL_LOGI_INI(
#if GPS_DL_ON_LINUX
		"alloc gps dl dma buf(%d,%d), addr: vir=0x%p, phy=0x%pad, len=%u",
#else
		"alloc gps dl dma buf(%d,%d), addr: vir=0x%p, phy=0x%08x, len=%u",
#endif
		p_dma_buf->dev_index, p_dma_buf->dir,
		p_dma_buf->vir_addr, p_dma_buf->phy_addr, p_dma_buf->len);

	if (NULL == p_dma_buf->vir_addr) {
		GDL_LOGXE_INI(xid,
			"alloc gps dl dma buf(%d,%d)(len = %u) fail", xid, dir, len);
		/* force move forward even fail */
		/* return -ENOMEM; */
	}

	return 0;
}

int gps_dl_dma_buf_alloc2(enum gps_mcudl_xid xid)
{
	int retval;
	struct gps_mcudl_each_device *p_dev;
	struct gps_mcudl_each_link *p_link;
	int ret1 = 0, ret2 = 0;

	p_dev = gps_mcudl_device_get(xid);
	p_link = gps_mcudl_link_get(xid);
	if (p_dev == NULL) {
		GDL_LOGXE_INI(xid, "gps_dl_device_get return null");
		return -1;
	}

	p_dev->dev->of_node = of_find_node_by_path("/consys@18002000");

	#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
		ret1 = of_dma_configure(p_dev->dev, p_dev->dev->of_node, false);
	#else
		ret1 = of_dma_configure(p_dev->dev, p_dev->dev->of_node);
	#endif

#if 0
#if 1
	if (!p_dev->dev->coherent_dma_mask)
		p_dev->dev->coherent_dma_mask = DMA_BIT_MASK(32);

	if (!p_dev->dev->dma_mask)
		p_dev->dev->dma_mask = &p_dev->dev->coherent_dma_mask;
#else
	ret2 = dma_set_mask(p_dev->dev, DMA_BIT_MASK(32));
#endif
#endif

	GDL_LOGXE_INI(xid, "of_node=%p, dma: ret1=%d, ret2=%d", p_dev->dev->of_node, ret1, ret2);

	retval = gps_dl_dma_buf_alloc(
		&p_link->tx_dma_buf, xid, GDL_DMA_A2D, p_link->cfg.tx_buf_size);

	if (retval)
		return retval;

	retval = gps_dl_dma_buf_alloc(
		&p_link->rx_dma_buf, xid, GDL_DMA_D2A, p_link->cfg.rx_buf_size);

	if (retval)
		return retval;

	return 0;
}
#endif

void gps_mcudl_ctx_links_deinit(void)
{
	enum gps_mcudl_xid xid;

	struct gps_mcudl_each_device *p_dev = NULL;
	struct gps_mcudl_each_link *p_link = NULL;

	for (xid = 0; xid < GPS_MDLX_CH_NUM; xid++) {
		p_dev = gps_mcudl_device_get(xid);
		p_link = gps_mcudl_link_get(xid);

		gps_mcudl_dma_buf_free(&p_link->tx_dma_buf, xid);
		gps_mcudl_dma_buf_free(&p_link->rx_dma_buf, xid);

		/* un-binding each device and link */
		p_link->p_device = NULL;
		p_dev->p_link = NULL;
		gps_mcudl_each_link_deinit(xid);
	}
}

int gps_mcudl_ctx_links_init(void)
{
	int retval;
	enum gps_mcudl_xid xid;
	struct gps_mcudl_each_device *p_dev = NULL;
	struct gps_mcudl_each_link *p_link = NULL;
	enum gps_each_link_waitable_type j;

	for (xid = 0; xid < GPS_MDLX_CH_NUM; xid++) {
		p_dev = gps_mcudl_device_get(xid);
		p_link = gps_mcudl_link_get(xid);

		retval = gps_mcudl_dma_buf_alloc2(xid);
		if (retval)
			return retval;

		for (j = 0; j < GPS_DL_WAIT_NUM; j++)
			gps_dl_link_waitable_init(&p_link->waitables[j], j);

		/* binding each device and link */
		p_link->p_device = p_dev;
		p_dev->p_link = p_link;

#if 0
		/* Todo: MNL read buf is 512, here is work-around */
		/* the solution should be make on gdl_dma_buf_get */
		gps_dl_set_rx_transfer_max(xid, GPS_LIBMNL_READ_MAX);
#endif
		gps_mcudl_each_link_init(xid);
		gps_mcusys_gpsbin_state_set(GPS_MCUSYS_GPSBIN_POST_OFF);
	}
	gps_mcusys_scif_set_lppm_open_ack_done(false);
	return 0;
}

void gps_mcudl_devices_exit(void)
{
	enum gps_mcudl_xid xid;
	enum gps_mcusys_nv_data_id nv_id;
	dev_t devno = MKDEV(gps_mcudl_devno_major, gps_mcudl_devno_minor);
	struct gps_mcudl_each_device *p_dev = NULL;
	struct gps_nv_each_device *p_nv_dev = NULL;

#if GPS_DL_HAS_PLAT_DRV
	/*gps_dl_linux_plat_drv_unregister();*/
#endif
	for (xid = 0; xid < GPS_MDLX_CH_NUM; xid++) {
		p_dev = gps_mcudl_device_get(xid);
		gps_mcudl_cdev_cleanup(p_dev, xid);
	}

	for (nv_id = 0; nv_id < GPS_MCUSYS_NV_DATA_NUM; nv_id++) {
		p_nv_dev = gps_nv_device_get(nv_id);
		gps_nv_cdev_cleanup(p_nv_dev, nv_id);
	}

	unregister_chrdev_region(devno, GPS_MDLX_CH_NUM + GPS_MCUSYS_NV_DATA_NUM);
}

void gps_mcudl_device_context_deinit(void)
{
#if 0
	gps_dl_procfs_remove();
#if GPS_DL_HAS_CONNINFRA_DRV
	gps_dl_unregister_conninfra_reset_cb();
#endif
	gps_dl_irq_deinit();

#if GPS_DL_HAS_CTRLD
	gps_dl_ctrld_deinit();
#endif

#if GPS_DL_MOCK_HAL
	gps_dl_mock_deinit();
#endif
#endif
	gps_mcudl_connsys_coredump_deinit();
	gps_mcudl_ctx_links_deinit();
	gps_mcudl_hal_user_fw_own_timer_destroy();
#if 0
#if GPS_DL_HAS_PLAT_DRV
	gps_dl_reserved_mem_deinit();
#endif
#endif
}

#if 0
int gps_dl_irq_init(void)
{
#if 0
	enum gps_dl_irq_index_enum irq_idx;

	for (irq_idx = 0; irq_idx < GPS_DL_IRQ_NUM; irq_idx++)
		;
#endif

	gps_dl_linux_irqs_register(gps_dl_irq_get(0), GPS_DL_IRQ_NUM);

	return 0;
}

int gps_dl_irq_deinit(void)
{
	gps_dl_linux_irqs_unregister(gps_dl_irq_get(0), GPS_DL_IRQ_NUM);
	return 0;
}
#endif

int gps_mcudl_devices_init(void)
{
	int result;
	enum gps_mcudl_xid xid;
	enum gps_mcusys_nv_data_id nv_id;
	dev_t devno = 0;
	struct gps_mcudl_each_device *p_dev = NULL;
	struct gps_nv_each_device *p_nv_dev = NULL;

	result = alloc_chrdev_region(&devno, gps_mcudl_devno_minor,
		GPS_MDLX_CH_NUM + GPS_MCUSYS_NV_DATA_NUM, GPS_MCUDL_DEV_NAME);

	gps_mcudl_devno_major = MAJOR(devno);

	if (result < 0) {
		GDL_LOGE_INI("fail to get major %d\n", gps_mcudl_devno_major);
		return result;
	}

	GDL_LOGD_INI("success to get major %d\n", gps_mcudl_devno_major);

	for (xid = 0; xid < GPS_MDLX_CH_NUM; xid++) {
		devno = MKDEV(gps_mcudl_devno_major, gps_mcudl_devno_minor + xid);
		p_dev = gps_mcudl_device_get(xid);
		p_dev->devno = devno;
		result = gps_mcudl_cdev_setup(p_dev, xid);
		if (result) {
			/* error happened */
			gps_mcudl_devices_exit();
			return result;
		}
	}

	for (nv_id = 0; nv_id < GPS_MCUSYS_NV_DATA_NUM; nv_id++) {
		devno = MKDEV(gps_mcudl_devno_major, gps_mcudl_devno_minor + GPS_MDLX_CH_NUM + nv_id);
		p_nv_dev = gps_nv_device_get(nv_id);
		p_nv_dev->devno = devno;
		result = gps_nv_cdev_setup(p_nv_dev, nv_id);
		if (result) {
			/* error happened */
			gps_mcudl_devices_exit();
			return result;
		}
	}

#if 0
#if GPS_DL_HAS_PLAT_DRV
	gps_dl_linux_plat_drv_register();
#else
	gps_dl_device_context_init();
#endif
#endif
	return 0;
}

void gps_mcudl_device_context_init(void)
{
#if 0
#if GPS_DL_HAS_PLAT_DRV
	gps_dl_reserved_mem_init();
#endif
#endif
#if 0
	gps_mcusys_nv_data_host_init();
#endif
	gps_mcudl_hal_user_fw_own_timer_setup();
	gps_mcudl_ctx_links_init();
	gps_mcudl_connsys_coredump_init();
#if 0
#if GPS_DL_MOCK_HAL
	gps_dl_mock_init();
#endif

#if GPS_DL_HAS_CTRLD
	gps_dl_ctrld_init();
#endif

#if (!(GPS_DL_NO_USE_IRQ || GPS_DL_HW_IS_MOCK))
	/* must after gps_dl_ctx_links_init */
	gps_dl_irq_init();
#endif
#if (GPS_DL_HAS_CONNINFRA_DRV)
	gps_dl_register_conninfra_reset_cb();
#endif
	gps_dl_procfs_setup();
#endif
}

