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
#define RX_PAGE_POOL_MAX_SIZE		(32768)

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */
struct wifi_page_pool_context;

struct wifi_page_pool_ops {
	bool (*alloc_mem)(void);
	void (*free_mem)(void);
	void (*release_mem)(struct device *dev);
};

struct wifi_page_pool_context {
	struct wifi_page_pool_ops ops;
	struct device *dev;
	struct page_pool *pool;
	struct kmem_cache *cache;
	uint32_t max_page_num;
	bool is_cma_mem;
	bool is_dynamic_alloc;
	uint32_t page_num;
	uint32_t size;
	spinlock_t pool_lock;
	spinlock_t page_lock;
};

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
static void update_page_pool_page_num(void);
static int wifi_page_pool_probe(struct platform_device *pdev);
static int wifi_page_pool_remove(struct platform_device *pdev);
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
void wifi_page_pool_set_page_num(uint32_t num)
{
	if (!pool_ctx.is_dynamic_alloc)
		return;

	if (num > pool_ctx.max_page_num)
		num = pool_ctx.max_page_num;
	pool_ctx.size = num;

	update_page_pool_page_num();
}
EXPORT_SYMBOL(wifi_page_pool_set_page_num);

uint32_t wifi_page_pool_get_page_num(void)
{
	return pool_ctx.page_num;
}
EXPORT_SYMBOL(wifi_page_pool_get_page_num);

uint32_t wifi_page_pool_get_max_page_num(void)
{
	return pool_ctx.max_page_num;
}
EXPORT_SYMBOL(wifi_page_pool_get_max_page_num);

static void set_page_num(uint32_t cnt)
{
	unsigned long flags = 0;

	spin_lock_irqsave(&pool_ctx.page_lock, flags);
	pool_ctx.page_num = cnt;
	spin_unlock_irqrestore(&pool_ctx.page_lock, flags);
}

struct page *wifi_page_pool_alloc_page(void)
{
	struct page *page = NULL;
	unsigned long flags = 0;

	if (!pool_ctx.pool)
		return NULL;

	if (is_page_pool_empty(pool_ctx.pool))
		return NULL;

	spin_lock_irqsave(&pool_ctx.pool_lock, flags);
	page = page_pool_alloc_pages(pool_ctx.pool, GFP_KERNEL);
	spin_unlock_irqrestore(&pool_ctx.pool_lock, flags);
	if (!page) {
		pr_info("%s: page alloc fail", __func__);
		goto exit;
	}

	init_page_count(page);
	page->pp = pool_ctx.pool;
	page->pp_magic = PP_SIGNATURE;
exit:

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

static void update_page_pool_page_num(void)
{
	if (pool_ctx.size > pool_ctx.page_num)
		pool_ctx.ops.alloc_mem();
	else if (pool_ctx.size < pool_ctx.page_num)
		pool_ctx.ops.free_mem();
}

static bool alloc_page_pool_cma_mem(void)
{
	struct page *page, *pages;
	void *kaddr;
	uint32_t i, level, alloc_cnt = 0, req_cnt = 0;

	if (!pool_ctx.dev || !pool_ctx.dev->cma_area) {
		pr_info("%s: no cma_area", __func__);
		return false;
	}

	if (pool_ctx.page_num >= pool_ctx.size)
		return true;

	req_cnt = pool_ctx.size - pool_ctx.page_num;
	level = req_cnt;
	while (req_cnt > alloc_cnt) {
		while (level > (req_cnt - alloc_cnt))
			level = level >> 1;

		if (level == 0)
			break;

		pages = cma_alloc(pool_ctx.dev->cma_area, level,
				  get_order(SZ_4K), GFP_KERNEL);
		if (!pages) {
			level = level >> 1;
			continue;
		}

		kaddr = page_address(pages);
		for (i = 0; i < level; i++) {
			page = virt_to_page(kaddr);
			if (!page) {
				pr_info("%s: page is NULL", __func__);
				break;
			}
			put_page_pool(pool_ctx.pool, page);
			kaddr += SZ_4K;
			alloc_cnt++;
		}
	}

	set_page_num(pool_ctx.page_num + alloc_cnt);

	pr_info("%s: page pool alloc[cur:%u req:%u alloc:%u]",
		__func__, pool_ctx.page_num, req_cnt, alloc_cnt);

	return true;
}

static bool alloc_page_pool_kernel_mem(void)
{
	struct page *page;
	uint32_t i, alloc_cnt = 0, req_cnt = 0;
	void *addr;

	if (pool_ctx.cache)
		goto alloc_page;

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

alloc_page:
	if (pool_ctx.page_num >= pool_ctx.size)
		goto exit;


	req_cnt = pool_ctx.size - pool_ctx.page_num;
	for (i = 0; i < req_cnt; i++) {
		addr = kmem_cache_alloc(pool_ctx.cache, GFP_KERNEL);
		if (!addr)
			continue;

		page = virt_to_page(addr);
		if (!page) {
			pr_info("%s: page is NULL", __func__);
			kmem_cache_free(pool_ctx.cache, addr);
			continue;
		}

		put_page_pool(pool_ctx.pool, page);
		alloc_cnt++;
	}

	set_page_num(pool_ctx.page_num + alloc_cnt);

	pr_info("%s: page pool alloc[cur:%u req:%u alloc:%u]",
		__func__, pool_ctx.page_num, req_cnt, alloc_cnt);

exit:
	return true;
}

static void free_page_pool_cma_mem(void)
{
	struct page *page;
	uint32_t i, release_cnt = 0, req_cnt = 0;
	unsigned long flags = 0;

	if (!pool_ctx.dev)
		return;

	if (pool_ctx.size >= pool_ctx.page_num)
		return;

	req_cnt = pool_ctx.page_num - pool_ctx.size;
	for (i = 0; i < req_cnt; i++) {
		if (!is_page_pool_empty(pool_ctx.pool)) {
			spin_lock_irqsave(&pool_ctx.pool_lock, flags);
			page = page_pool_alloc_pages(pool_ctx.pool, GFP_KERNEL);
			spin_unlock_irqrestore(&pool_ctx.pool_lock, flags);
			if (!page) {
				pr_info("%s: page alloc fail", __func__);
				continue;
			}
			cma_release(pool_ctx.dev->cma_area, page, 1);
			release_cnt++;
		}
	}

	set_page_num(pool_ctx.page_num - release_cnt);

	pr_info("%s: page pool release[cur:%u req:%u release:%u]",
		__func__, pool_ctx.page_num, req_cnt, release_cnt);
}

static void free_page_pool_kernel_mem(void)
{
	struct page *page;
	uint32_t i, release_cnt = 0, req_cnt = 0;
	unsigned long flags = 0;

	if (!pool_ctx.cache)
		return;

	if (pool_ctx.size >= pool_ctx.page_num)
		return;

	req_cnt = pool_ctx.page_num - pool_ctx.size;
	for (i = 0; i < req_cnt; i++) {
		if (!is_page_pool_empty(pool_ctx.pool)) {
			spin_lock_irqsave(&pool_ctx.pool_lock, flags);
			page = page_pool_alloc_pages(pool_ctx.pool, GFP_KERNEL);
			spin_unlock_irqrestore(&pool_ctx.pool_lock, flags);
			if (!page) {
				pr_info("%s: page alloc fail", __func__);
				continue;
			}
			kmem_cache_free(pool_ctx.cache, page_to_virt(page));
			release_cnt++;
		}
	}

	set_page_num(pool_ctx.page_num - release_cnt);

	pr_info("%s: page pool release[cur:%u req:%u release:%u]",
		__func__, pool_ctx.page_num, req_cnt, release_cnt);
}

static void release_page_pool_cma_mem(struct device *dev)
{
	struct page *page;
	unsigned long flags = 0;

	if (!dev->cma_area)
		return;

	while (!is_page_pool_empty(pool_ctx.pool)) {
		spin_lock_irqsave(&pool_ctx.pool_lock, flags);
		page = page_pool_alloc_pages(pool_ctx.pool, GFP_KERNEL);
		spin_unlock_irqrestore(&pool_ctx.pool_lock, flags);
		cma_release(dev->cma_area, page, 1);
	}
}

static void release_page_pool_kernel_mem(struct device *dev)
{
	struct page *page;
	unsigned long flags = 0;

	if (!pool_ctx.cache)
		return;

	while (!is_page_pool_empty(pool_ctx.pool)) {
		spin_lock_irqsave(&pool_ctx.pool_lock, flags);
		page = page_pool_alloc_pages(pool_ctx.pool, GFP_KERNEL);
		spin_unlock_irqrestore(&pool_ctx.pool_lock, flags);
		kmem_cache_free(pool_ctx.cache, page_to_virt(page));
	}

	kmem_cache_destroy(pool_ctx.cache);
	pool_ctx.cache = NULL;
}

static bool is_page_pool_empty(struct page_pool *pool)
{
	return !pool->alloc.count && __ptr_ring_empty(&pool->ring);
}

static int create_page_pool(struct device *dev)
{
	struct page_pool_params pp = { 0 };

	pp.max_len = PAGE_SIZE;
	pp.flags = 0;
	pp.pool_size = pool_ctx.max_page_num;
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

	update_page_pool_page_num();

	return 0;
}

static void release_page_pool(struct device *dev)
{
	if (!pool_ctx.pool)
		return;

	pool_ctx.ops.release_mem(dev);
	page_pool_destroy(pool_ctx.pool);
	pool_ctx.pool = NULL;
}

static void wifi_page_pool_ops_init(struct device *dev)
{
	if (pool_ctx.is_cma_mem) {
		pool_ctx.ops.alloc_mem = alloc_page_pool_cma_mem;
		pool_ctx.ops.free_mem = free_page_pool_cma_mem;
		pool_ctx.ops.release_mem = release_page_pool_cma_mem;
	} else {
		pool_ctx.ops.alloc_mem = alloc_page_pool_kernel_mem;
		pool_ctx.ops.free_mem = free_page_pool_kernel_mem;
		pool_ctx.ops.release_mem = release_page_pool_kernel_mem;
	}
}

static int page_pool_setup(struct platform_device *pdev)
{
	int ret = 0;
#ifdef CONFIG_OF
	struct device_node *node = NULL;
	uint32_t rsv_mem_size = 0, is_dynamic = 0;

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
	}
	pool_ctx.is_cma_mem = pdev->dev.cma_area ? true : false;

	ret = of_property_read_u32(node, "emi-size", &rsv_mem_size);
	if (!ret)
		pool_ctx.max_page_num = rsv_mem_size / PAGE_SIZE;
	else
		pool_ctx.max_page_num = RX_PAGE_POOL_MAX_SIZE;

	ret = of_property_read_u32(node, "dynamic-alloc", &is_dynamic);
	if (!ret && is_dynamic)
		pool_ctx.is_dynamic_alloc = true;

	if (!pool_ctx.is_dynamic_alloc)
		pool_ctx.size = pool_ctx.max_page_num;

	spin_lock_init(&pool_ctx.pool_lock);
	spin_lock_init(&pool_ctx.page_lock);

	pr_info("%s: reserved memory size[0x%08x] num[%u] cma[%u] dynamic[%u]",
		__func__, rsv_mem_size, pool_ctx.max_page_num,
		pool_ctx.is_cma_mem, pool_ctx.is_dynamic_alloc);
	ret = 0;
#else
	pr_info("%s: kernel option CONFIG_OF not enabled.");
#endif
	pool_ctx.dev = &pdev->dev;

exit:
	return ret;
}

static int wifi_page_pool_probe(struct platform_device *pdev)
{
	int ret = 0;

	ret = page_pool_setup(pdev);
	if (ret)
		goto exit;

	wifi_page_pool_ops_init(&pdev->dev);
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
