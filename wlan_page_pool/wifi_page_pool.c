/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */
/*! \file  wifi_page_pool.c
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/cma.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/of_reserved_mem.h>
#include <linux/of_address.h>
#include <linux/of.h>
#include <net/page_pool.h>

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define CFG_CMA_ALLOC 1
#define RX_PAGE_POOL_SIZE (32768)

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */
struct wifi_page_pool_context {
	struct page_pool *pool;
#if CFG_CMA_ALLOC
	struct page *cma_page;
#else
	struct kmem_cache *cache;
#endif
};

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
static int wifi_page_pool_probe(struct platform_device *pdev);
static int wifi_page_pool_remove(struct platform_device *pdev);
static int page_pool_setup(struct platform_device *pdev);
static bool is_page_pool_empty(struct page_pool *pool);

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */
#ifdef CONFIG_OF
const struct of_device_id wifi_page_pool_of_ids[] = {
	{.compatible = "mediatek,wifi_page_pool",},
	{}
};
#endif

static struct platform_driver wifi_page_pool_drv = {
	.probe = wifi_page_pool_probe,
	.remove = wifi_page_pool_remove,
	.driver = {
		.name = "wifi_page_pool",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = wifi_page_pool_of_ids,
#endif
	},
};

static struct wifi_page_pool_context pool_ctx;

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
struct page *wifi_page_pool_alloc_page(void)
{
	struct page *page;

	if (!pool_ctx.pool)
		return NULL;

	if (is_page_pool_empty(pool_ctx.pool))
		return NULL;

	page = page_pool_alloc_pages(pool_ctx.pool, GFP_KERNEL);
	if (!page) {
		pr_info("%s: page alloc fail", __func__);
		return NULL;
	}

	init_page_count(page);
	page->pp = pool_ctx.pool;
	page->pp_magic = PP_SIGNATURE;

	return page;
}
EXPORT_SYMBOL(wifi_page_pool_alloc_page);

static void put_page_pool(struct page_pool *pool, struct page *page)
{
	init_page_count(page);
	page->pp = pool;
	page->pp_magic = PP_SIGNATURE;
	page_pool_recycle_direct(pool, page);
}

#if CFG_CMA_ALLOC
static bool alloc_page_pool_mem(struct device *dev, struct page_pool *pool)
{
	uint32_t i;
	struct page *page;
	void *kaddr;
	bool ret = true;

	if (!dev->cma_area) {
		pr_info("%s: no cma_area", __func__);
		return false;
	}
	pool_ctx.cma_page = cma_alloc(dev->cma_area, RX_PAGE_POOL_SIZE,
				      get_order(SZ_4K), GFP_KERNEL);
	if (!pool_ctx.cma_page) {
		pr_info("%s: cma_page alloc fail", __func__);
		return false;
	}

	kaddr = page_address(pool_ctx.cma_page);
	for (i = 0; i < RX_PAGE_POOL_SIZE; i++) {
		page = virt_to_page(kaddr);
		if (!page) {
			pr_info("%s: page is NULL", __func__);
			break;
		}
		put_page_pool(pool, page);
		kaddr += SZ_4K;
	}
	pr_info("%s: alloc page num[%d]", __func__, i);

	return ret;
}
#else
static bool alloc_page_pool_mem(struct device *dev, struct page_pool *pool)
{
	uint32_t i;
	void *addr;
	struct page *page;

	pool_ctx.cache = kmem_cache_create_usercopy(
		"wlan_rx_skb_cache",
		PAGE_SIZE,
		0,
		SLAB_CACHE_DMA32,
		0,
		PAGE_SIZE,
		NULL);
	if (!pool_ctx.cache) {
		pr_info("%s: skb cache create fail", __func__);
		return false;
	}

	for (i = 0; i < RX_PAGE_POOL_SIZE; i++) {
		addr = kmem_cache_alloc(pool_ctx.cache, GFP_KERNEL);
		if (!addr) {
			pr_info("%s: vir_addr is NULL", __func__);
			continue;
		}
		page = virt_to_page(addr);
		if (!page) {
			pr_info("%s: page is NULL", __func__);
			continue;
		}
		put_page_pool(pool, page);
	}

	return true;
}
#endif

static bool is_page_pool_empty(struct page_pool *pool)
{
	return !pool->alloc.count && __ptr_ring_empty(&pool->ring);
}

static int create_page_pool(struct device *dev)
{
	struct page_pool_params pp = { 0 };

	pp.max_len = PAGE_SIZE;
	pp.flags = 0;
	pp.pool_size = RX_PAGE_POOL_SIZE;
	pp.nid = dev_to_node(dev);
	pp.dev = dev;
	pp.dma_dir = DMA_FROM_DEVICE;

	pool_ctx.pool = page_pool_create(&pp);

	if (IS_ERR(pool_ctx.pool)) {
		int err = PTR_ERR(pool_ctx.pool);

		pr_info("%s: create page pool fail", __func__);
		pool_ctx.pool = NULL;
		return err;
	}
	alloc_page_pool_mem(dev, pool_ctx.pool);
	return 0;
}

static void release_page_pool(struct device *dev)
{
	if (!pool_ctx.pool)
		return;

#if CFG_CMA_ALLOC
	if (!pool_ctx.cma_page || !dev->cma_area)
		return;

	while (!is_page_pool_empty(pool_ctx.pool))
		page_pool_alloc_pages(pool_ctx.pool, GFP_KERNEL);

	cma_release(dev->cma_area, pool_ctx.cma_page, RX_PAGE_POOL_SIZE);
	pool_ctx.cma_page = NULL;
#else
	if (!pool_ctx.cache)
		return;

	while (!is_page_pool_empty(pool_ctx.pool)) {
		struct page *page;

		page = page_pool_alloc_pages(pool_ctx.pool, GFP_KERNEL);
		kmem_cache_free(pool_ctx.cache, page_to_virt(page));
	}

	kmem_cache_destroy(pool_ctx.cache);
	pool_ctx.cache = NULL;
#endif

	page_pool_destroy(pool_ctx.pool);
	pool_ctx.pool = NULL;
}

static int wifi_page_pool_probe(struct platform_device *pdev)
{
	int ret = 0;

	ret = page_pool_setup(pdev);
	if (ret)
		goto exit;

	create_page_pool(&pdev->dev);

exit:
	pr_info("%s() done, ret: %d", __func__, ret);

	return 0;
}

static int wifi_page_pool_remove(struct platform_device *pdev)
{
	release_page_pool(&pdev->dev);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static int page_pool_setup(struct platform_device *pdev)
{
	int ret = 0;
#ifdef CONFIG_OF
	struct device_node *node = NULL;

	node = pdev->dev.of_node;
	if (!node) {
		pr_info("%s: get wifi device node fail.", __func__);
		of_node_put(node);
		ret = -1;
		goto exit;
	}
	of_node_put(node);

	ret = of_reserved_mem_device_init(&pdev->dev);
	if (ret) {
		pr_info("%s: of_reserved_mem_device_init failed(%d).",
			__func__, ret);
		goto exit;
	}

	return ret;
#else
	pr_info("%s: kernel option CONFIG_OF not enabled.");
#endif

exit:
	return ret;
}

static int __init wifi_page_pool_init(void)
{
	int ret = 0;

	pr_info("%s: init wifi page pool mod", __func__);

	memset(&pool_ctx, 0, sizeof(struct wifi_page_pool_context));

	ret = platform_driver_register(&wifi_page_pool_drv);
	if (ret) {
		pr_info("%s: PagePool driver registration failed: %d",
			__func__, ret);
		goto exit;
	}
	pr_info("%s: init done[%d]", __func__, ret);
exit:
	return ret;
}

static void __exit wifi_page_pool_exit(void)
{
	pr_info("%s: exit wifi page pool mod", __func__);

	platform_driver_unregister(&wifi_page_pool_drv);
}

module_init(wifi_page_pool_init);
module_exit(wifi_page_pool_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("WIFI Page Pool Driver");
MODULE_AUTHOR("Holiday Hao <holiday.hao@mediatek.com>");
