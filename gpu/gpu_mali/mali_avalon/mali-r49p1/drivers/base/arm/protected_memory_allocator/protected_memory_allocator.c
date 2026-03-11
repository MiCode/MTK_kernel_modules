// SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note
/*
 *
 * (C) COPYRIGHT 2019-2023 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 *
 */

#include <linux/version.h>
#include <linux/of.h>
#include <linux/of_reserved_mem.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/io.h>
#include <linux/protected_memory_allocator.h>

#if IS_ENABLED(CONFIG_MALI_MTK_GPU_IOMMU)
#pragma message "enable CONFIG_MALI_MTK_GPU_IOMMU"
#include <mtk_gpufreq.h>
#if IS_ENABLED(CONFIG_MALI_MTK_GHPM_STAGE1_ENABLE)
#pragma message "CONFIG_MALI_MTK_GHPM_STAGE1_ENABLE enabled (pma)"
#include <ghpm_wrapper.h>
#else
#pragma message "CONFIG_MALI_MTK_GHPM_STAGE1_ENABLE disabled (pma)"
#endif /* CONFIG_MALI_MTK_GHPM_STAGE1_ENABLE */
#include <linux/err.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#endif /* CONFIG_MALI_MTK_GPU_IOMMU */

#if IS_ENABLED(CONFIG_MALI_MTK_GPU_PMA_PAGE_HEAP)
#include <linux/dma-buf.h>
#include <linux/dma-heap.h>
#if IS_ENABLED(CONFIG_MTK_TRUSTED_MEMORY_SUBSYSTEM) && IS_ENABLED(CONFIG_MTK_GZ_KREE)
#include <trusted_mem_api.h>
#include <mtk_heap.h>
#endif /* CONFIG_MTK_TRUSTED_MEMORY_SUBSYSTEM && CONFIG_MTK_GZ_KREE */
#endif /* CONFIG_MALI_MTK_GPU_PMA_PAGE_HEAP */

/* Size of a bitfield element in bytes */
#define BITFIELD_ELEM_SIZE sizeof(u64)

/* We can track whether or not 64 pages are currently allocated in a u64 */
#define PAGES_PER_BITFIELD_ELEM (BITFIELD_ELEM_SIZE * BITS_PER_BYTE)

/* Order 6 (ie, 64) corresponds to the number of pages held in a bitfield */
#define ORDER_OF_PAGES_PER_BITFIELD_ELEM 6

/**
 * struct simple_pma_device - Simple implementation of a protected memory
 *                            allocator device
 * @pma_dev: Protected memory allocator device pointer
 * @dev:     Device pointer
 * @allocated_pages_bitfield_arr: Status of all the physical memory pages within the
 *                                protected memory region, one bit per page
 * @rmem_base:      Base physical address of the reserved memory region
 * @rmem_size:      Size of the reserved memory region, in pages
 * @num_free_pages: Number of free pages in the memory region
 * @rmem_lock:      Lock to serialize the allocation and freeing of
 *                  physical pages from the protected memory region
 */
struct simple_pma_device {
	struct protected_memory_allocator_device pma_dev;
	struct device *dev;
	u64 *allocated_pages_bitfield_arr;
	phys_addr_t rmem_base;
	size_t rmem_size;
	size_t num_free_pages;
	spinlock_t rmem_lock;
#if IS_ENABLED(CONFIG_MALI_MTK_GPU_PMA_PAGE_HEAP)
	struct dma_buf_info heap_dma_info;
#endif /* CONFIG_MALI_MTK_GPU_PMA_PAGE_HEAP */
};

/**
 * ALLOC_PAGES_BITFIELD_ARR_SIZE() - Number of elements in array
 *                                   'allocated_pages_bitfield_arr'
 * If the number of pages required does not divide exactly by
 * PAGES_PER_BITFIELD_ELEM, adds an extra page for the remainder.
 * @num_pages: number of pages
 */
#define ALLOC_PAGES_BITFIELD_ARR_SIZE(num_pages)                                                \
	((PAGES_PER_BITFIELD_ELEM * (0 != (num_pages % PAGES_PER_BITFIELD_ELEM)) + num_pages) / \
	 PAGES_PER_BITFIELD_ELEM)

#if IS_ENABLED(CONFIG_MALI_MTK_GPU_PMA_PAGE_HEAP)
static phys_addr_t alloc_sec_dma_heap(struct simple_pma_device *const epma_dev, size_t pma_size, struct dma_buf_info *dma_info)
{
	struct dma_heap *heap = NULL;
	struct dma_buf *buf = NULL;
	struct dma_buf_attachment *buf_attachment = NULL;
	struct sg_table *sgt = NULL;
	struct scatterlist *s = NULL;
	int i = 0;
	phys_addr_t pma_base = 0;
	uint64_t phy_addr = 0;
	u64 sec_handle = 0;
	const char heap_name[] = "mtk_prot_page-uncached";
	//TODO try svp_page or prot_page
	// mtk_prot_page-uncached mtk_svp_page-uncached mtk_svp_region-aligned mtk_prot_region-aligned

	(void)sgt;
	heap = dma_heap_find(heap_name);
	if (!heap) {
		dev_err(epma_dev->dev, "failed to find heap %s\n", heap_name);
		return 0;
	}
	buf = dma_heap_buffer_alloc(heap, pma_size, 0, 0);
	if (!buf) {
		dev_err(epma_dev->dev, "failed to allocate buffer\n");
		return 0;
	}
	buf_attachment = dma_buf_attach(buf, epma_dev->dev);
	if (!buf_attachment) {
		dev_err(epma_dev->dev, "failed to attach buffer\n");
		return 0;
	}

#if IS_ENABLED(CONFIG_MTK_TRUSTED_MEMORY_SUBSYSTEM) && IS_ENABLED(CONFIG_MTK_GZ_KREE)
	sec_handle = dmabuf_to_secure_handle(buf);

	if (sec_handle && is_support_secure_handle(buf)) {
		// For region base sec mem
		// use trusted_mem api
#if IS_ENABLED(CONFIG_ARM_FFA_TRANSPORT)
		trusted_mem_ffa_query_pa(&sec_handle, &phy_addr);
#else
		trusted_mem_api_query_pa(0, 0, 0, NULL, &sec_handle, NULL, 0, 0, &phy_addr);
#endif
	} else {
		// For page base sec mem
		// use sg_dma_address to get PA
#if (KERNEL_VERSION(6, 1, 55) <= LINUX_VERSION_CODE)
		sgt = dma_buf_map_attachment_unlocked(buf_attachment, DMA_BIDIRECTIONAL);
#else
		sgt = dma_buf_map_attachment(buf_attachment, DMA_BIDIRECTIONAL);
#endif
		if (IS_ERR_OR_NULL(sgt)) {
			dev_err(epma_dev->dev, "failed to get sg table\n");
			return 0;
		}
		if(sgt->sgl != NULL){
			phy_addr = sg_dma_address(sgt->sgl);
			/*for_each_sg(sgt->sgl, s, sgt->nents, i) {
				phy_addr = sg_phys(s);
			}*/
		} else {
			dev_err(epma_dev->dev, "failed to get pa from sgl\n");
			return 0;
		}
	}

	pma_base = phy_addr;
	dev_vdbg(epma_dev->dev, "pma:base=%llx,size=%zu,heap=%s\n",
			(unsigned long long) pma_base, pma_size, heap_name);
#else
#if (KERNEL_VERSION(6, 1, 55) <= LINUX_VERSION_CODE)
	sgt = dma_buf_map_attachment_unlocked(buf_attachment, DMA_BIDIRECTIONAL);
#else
	sgt = dma_buf_map_attachment(buf_attachment, DMA_BIDIRECTIONAL);
#endif
	if (sgt == NULL) {
		dev_err(epma_dev->dev, "failed to get sg table\n");
		return 0;
	}
	if(sgt->sgl != NULL){
		pma_base = sg_dma_address(sgt->sgl);
	} else {
		dev_err(epma_dev->dev, "failed to get pa from sgl\n");
		return 0;
	}
	dev_vdbg(epma_dev->dev, "pma:base=%llx,size=%zu,heap_name=%s\n",
		(unsigned long long) pma_base, pma_size, heap_name);
#endif
	if(dma_info != NULL)
	{
		dma_info->buf = buf;
		dma_info->buf_attachment = buf_attachment;
		dma_info->sgt = sgt;
	}

	return pma_base;
}

static struct protected_memory_allocation *simple_pma_alloc_dma_page(
	struct protected_memory_allocator_device *pma_dev, unsigned int order)
{
	size_t num_pages_to_alloc;
	struct simple_pma_device *const epma_dev =
		container_of(pma_dev, struct simple_pma_device, pma_dev);
	struct protected_memory_allocation *pma;

	num_pages_to_alloc = (size_t)1 << order;

	dev_vdbg(epma_dev->dev, "%s(pma_dev=%px, order=%u\n",
		__func__, (void *)pma_dev, order);

	pma = devm_kzalloc(epma_dev->dev, sizeof(*pma), GFP_KERNEL);
	if (!pma) {
		dev_err(epma_dev->dev, "Failed to alloc pma struct");
		return NULL;
	}

	pma->pa = alloc_sec_dma_heap(epma_dev, num_pages_to_alloc<<PAGE_SHIFT, &(pma->dma_info));
	pma->order = order;

	if (pma->pa == 0) {
		devm_kfree(epma_dev->dev, pma);
		dev_err(epma_dev->dev, "Failed to get pma pa");
		return NULL;
	}

	if (order > 0) {
		dev_err(epma_dev->dev, "pma:base=%llx, order=%u\n",
			(unsigned long long) pma->pa, order);
	}

	return pma;
}

static phys_addr_t simple_pma_get_dma_phys_addr(
	struct protected_memory_allocator_device *pma_dev,
	struct protected_memory_allocation *pma)
{
	struct simple_pma_device *const epma_dev =
		container_of(pma_dev, struct simple_pma_device, pma_dev);

	if (!pma) {
		dev_err(epma_dev->dev, "Failed to get pma struct");
		return 0;
	}

	dev_vdbg(epma_dev->dev, "%s(pma_dev=%px, pma=%px, pa=%llx\n",
		__func__, (void *)pma_dev, (void *)pma,
		(unsigned long long)pma->pa);
	return pma->pa;
}

static void simple_pma_free_dma_page(
	struct protected_memory_allocator_device *pma_dev,
	struct protected_memory_allocation *pma)
{
	struct dma_buf *buf;
	struct dma_buf_attachment *buf_attachment;
	struct sg_table *sgt;
	struct simple_pma_device *const epma_dev =
		container_of(pma_dev, struct simple_pma_device, pma_dev);

	if (!pma) {
		dev_err(epma_dev->dev, "Failed to get pma struct");
		return;
	}

	buf = pma->dma_info.buf;
	buf_attachment = pma->dma_info.buf_attachment;
	sgt = pma->dma_info.sgt;

	dev_vdbg(epma_dev->dev, "%s(pma_dev=%px, pma=%px, pa=%llx\n",
		__func__, (void *)pma_dev, (void *)pma,
		(unsigned long long)pma->pa);

	if (sgt) {
#if (KERNEL_VERSION(6, 1, 55) <= LINUX_VERSION_CODE)
		dma_buf_unmap_attachment_unlocked(buf_attachment, sgt, DMA_BIDIRECTIONAL);
#else
		dma_buf_unmap_attachment(buf_attachment, sgt, DMA_BIDIRECTIONAL);
#endif /* KERNEL_VERSION */
	}
	if (buf_attachment) {
		dma_buf_detach(buf, buf_attachment);
	}
	if (buf) {
		dma_buf_put(buf);
	}
	devm_kfree(epma_dev->dev, pma);
}

static void pma_alloc_test(struct protected_memory_allocator_device *pma_dev)
{
	struct simple_pma_device *const epma_dev =
		container_of(pma_dev, struct simple_pma_device, pma_dev);
	struct protected_memory_allocation *pma = simple_pma_alloc_dma_page(pma_dev, 3);
	phys_addr_t pa = simple_pma_get_dma_phys_addr(pma_dev, pma);
	dev_err(epma_dev->dev, "pma_alloc_test: phy_addr=%llx\n", (unsigned long long) pa);
	simple_pma_free_dma_page(pma_dev, pma);
}
#endif /* CONFIG_MALI_MTK_GPU_PMA_PAGE_HEAP */

/**
 * small_granularity_alloc() - Allocate 1-32 power-of-two pages.
 * @epma_dev: protected memory allocator device structure.
 * @alloc_bitfield_idx: index of the relevant bitfield.
 * @start_bit: starting bitfield index.
 * @order: bitshift for number of pages. Order of 0 to 5 equals 1 to 32 pages.
 * @pma: protected_memory_allocation struct.
 *
 * Allocate a power-of-two number of pages, N, where
 * 0 <= N <= ORDER_OF_PAGES_PER_BITFIELD_ELEM - 1.  ie, Up to 32 pages. The routine
 * fills-in a pma structure and sets the appropriate bits in the allocated-pages
 * bitfield array but assumes the caller has already determined that these are
 * already clear.
 *
 * This routine always works within only a single allocated-pages bitfield element.
 * It can be thought of as the 'small-granularity' allocator.
 */
static void small_granularity_alloc(struct simple_pma_device *const epma_dev,
				    size_t alloc_bitfield_idx, size_t start_bit, size_t order,
				    struct protected_memory_allocation *pma)
{
	size_t i;
	size_t page_idx;
	u64 *bitfield;
	size_t alloc_pages_bitfield_size;

	if (WARN_ON(!epma_dev) || WARN_ON(!pma))
		return;

	WARN(epma_dev->rmem_size == 0, "%s: rmem_size is 0", __func__);
	alloc_pages_bitfield_size = ALLOC_PAGES_BITFIELD_ARR_SIZE(epma_dev->rmem_size);

	WARN(alloc_bitfield_idx >= alloc_pages_bitfield_size, "%s: idx>bf_size: %zu %zu", __func__,
	     alloc_bitfield_idx, alloc_pages_bitfield_size);

	WARN((start_bit + (1ULL << order)) > PAGES_PER_BITFIELD_ELEM,
	     "%s: start=%zu order=%zu ppbe=%zu", __func__, start_bit, order,
	     PAGES_PER_BITFIELD_ELEM);

	bitfield = &epma_dev->allocated_pages_bitfield_arr[alloc_bitfield_idx];

	for (i = 0; i < (1ULL << order); i++) {
		/* Check the pages represented by this bit are actually free */
		WARN(*bitfield & (1ULL << (start_bit + i)),
		     "in %s: page not free: %zu %zu %.16llx %zu\n", __func__, i, order, *bitfield,
		     alloc_pages_bitfield_size);

		/* Mark the pages as now allocated */
		*bitfield |= (1ULL << (start_bit + i));
	}

	/* Compute the page index */
	page_idx = (alloc_bitfield_idx * PAGES_PER_BITFIELD_ELEM) + start_bit;

	/* Fill-in the allocation struct for the caller */
	pma->pa = epma_dev->rmem_base + (page_idx << PAGE_SHIFT);
	pma->order = order;
}

/**
 * large_granularity_alloc() - Allocate pages at multiples of 64 pages.
 * @epma_dev: protected memory allocator device structure.
 * @start_alloc_bitfield_idx: index of the starting bitfield.
 * @order: bitshift for number of pages. Order of 6+ equals 64+ pages.
 * @pma: protected_memory_allocation struct.
 *
 * Allocate a power-of-two number of pages, N, where
 * N >= ORDER_OF_PAGES_PER_BITFIELD_ELEM. ie, 64 pages or more. The routine fills-in
 * a pma structure and sets the appropriate bits in the allocated-pages bitfield array
 * but assumes the caller has already determined that these are already clear.
 *
 * Unlike small_granularity_alloc, this routine can work with multiple 64-page groups,
 * ie multiple elements from the allocated-pages bitfield array. However, it always
 * works with complete sets of these 64-page groups. It can therefore be thought of
 * as the 'large-granularity' allocator.
 */
static void large_granularity_alloc(struct simple_pma_device *const epma_dev,
				    size_t start_alloc_bitfield_idx, size_t order,
				    struct protected_memory_allocation *pma)
{
	size_t i;
	size_t num_pages_to_alloc = (size_t)1 << order;
	size_t num_bitfield_elements_needed = num_pages_to_alloc / PAGES_PER_BITFIELD_ELEM;
	size_t start_page_idx = start_alloc_bitfield_idx * PAGES_PER_BITFIELD_ELEM;

	if (WARN_ON(!epma_dev) || WARN_ON(!pma))
		return;

	/*
	 * Are there anough bitfield array elements (groups of 64 pages)
	 * between the start element and the end of the bitfield array
	 * to fulfill the request?
	 */
	WARN((start_alloc_bitfield_idx + order) >=
		     ALLOC_PAGES_BITFIELD_ARR_SIZE(epma_dev->rmem_size),
	     "%s: start=%zu order=%zu ms=%zu", __func__, start_alloc_bitfield_idx, order,
	     epma_dev->rmem_size);

	for (i = 0; i < num_bitfield_elements_needed; i++) {
		u64 *bitfield =
			&epma_dev->allocated_pages_bitfield_arr[start_alloc_bitfield_idx + i];

		/* We expect all pages that relate to this bitfield element to be free */
		WARN((*bitfield != 0), "in %s: pages not free: i=%zu o=%zu bf=%.16llx\n", __func__,
		     i, order, *bitfield);

		/* Mark all the pages for this element as not free */
		*bitfield = ~0ULL;
	}

	/* Fill-in the allocation struct for the caller */
	pma->pa = epma_dev->rmem_base + (start_page_idx << PAGE_SHIFT);
	pma->order = order;
}

static struct protected_memory_allocation *
simple_pma_alloc_page(struct protected_memory_allocator_device *pma_dev, unsigned int order)
{
	struct simple_pma_device *const epma_dev =
		container_of(pma_dev, struct simple_pma_device, pma_dev);
	struct protected_memory_allocation *pma;
	size_t num_pages_to_alloc;

	u64 *bitfields = epma_dev->allocated_pages_bitfield_arr;
	size_t i;
	size_t bit;
	size_t count;

	dev_dbg(epma_dev->dev, "%s(pma_dev=%px, order=%u\n", __func__, (void *)pma_dev, order);

	/* This is an example function that follows an extremely simple logic
	 * and is very likely to fail to allocate memory if put under stress.
	 *
	 * The simple_pma_device maintains an array of u64s, with one bit used
	 * to track the status of each page.
	 *
	 * In order to create a memory allocation, the allocator looks for an
	 * adjacent group of cleared bits. This does leave the algorithm open
	 * to fragmentation issues, but is deemed sufficient for now.
	 * If successful, the allocator shall mark all the pages as allocated
	 * and increment the offset accordingly.
	 *
	 * Allocations of 64 pages or more (order 6) can be allocated only with
	 * 64-page alignment, in order to keep the algorithm as simple as
	 * possible. ie, starting from bit 0 of any 64-bit page-allocation
	 * bitfield. For this, the large-granularity allocator is utilised.
	 *
	 * Allocations of lower-order can only be allocated entirely within the
	 * same group of 64 pages, with the small-ganularity allocator  (ie
	 * always from the same 64-bit page-allocation bitfield) - again, to
	 * keep things as simple as possible, but flexible to meet
	 * current needs.
	 */

	num_pages_to_alloc = (size_t)1 << order;

	pma = devm_kzalloc(epma_dev->dev, sizeof(*pma), GFP_KERNEL);
	if (!pma) {
		dev_err(epma_dev->dev, "Failed to alloc pma struct");
		return NULL;
	}

	spin_lock(&epma_dev->rmem_lock);

	if (epma_dev->num_free_pages < num_pages_to_alloc) {
		dev_err(epma_dev->dev, "not enough free pages\n");
		devm_kfree(epma_dev->dev, pma);
		spin_unlock(&epma_dev->rmem_lock);
		return NULL;
	}

	/*
	 * For order 0-5 (ie, 1 to 32 pages) we always allocate within the same set of 64 pages
	 * Currently, most allocations will be very small (1 page), so the more likely path
	 * here is order < ORDER_OF_PAGES_PER_BITFIELD_ELEM.
	 */
	if (likely(order < ORDER_OF_PAGES_PER_BITFIELD_ELEM)) {
		size_t alloc_pages_bitmap_size = ALLOC_PAGES_BITFIELD_ARR_SIZE(epma_dev->rmem_size);

		for (i = 0; i < alloc_pages_bitmap_size; i++) {
			count = 0;

			for (bit = 0; bit < PAGES_PER_BITFIELD_ELEM; bit++) {
				if (0 == (bitfields[i] & (1ULL << bit))) {
					if ((count + 1) >= num_pages_to_alloc) {
						/*
						 * We've found enough free, consecutive pages with which to
						 * make an allocation
						 */
						small_granularity_alloc(epma_dev, i, bit - count,
									order, pma);

						epma_dev->num_free_pages -= num_pages_to_alloc;

						spin_unlock(&epma_dev->rmem_lock);
						return pma;
					}

					/* So far so good, but we need more set bits yet */
					count++;
				} else {
					/*
					 * We found an allocated page, so nothing we've seen so far can be used.
					 * Keep looking.
					 */
					count = 0;
				}
			}
		}
	} else {
		/**
		 * For allocations of order ORDER_OF_PAGES_PER_BITFIELD_ELEM and above (>= 64 pages), we know
		 * we'll only get allocations for whole groups of 64 pages, which hugely simplifies the task.
		 */
		size_t alloc_pages_bitmap_size = ALLOC_PAGES_BITFIELD_ARR_SIZE(epma_dev->rmem_size);

		/* How many 64-bit bitfield elements will be needed for the allocation? */
		size_t num_bitfield_elements_needed = num_pages_to_alloc / PAGES_PER_BITFIELD_ELEM;

		count = 0;

		for (i = 0; i < alloc_pages_bitmap_size; i++) {
			/* Are all the pages free for the i'th u64 bitfield element? */
			if (bitfields[i] == 0) {
				count += PAGES_PER_BITFIELD_ELEM;

				if (count >= (1ULL << order)) {
					size_t start_idx = (i + 1) - num_bitfield_elements_needed;

					large_granularity_alloc(epma_dev, start_idx, order, pma);

					epma_dev->num_free_pages -= 1ULL << order;
					spin_unlock(&epma_dev->rmem_lock);
					return pma;
				}
			} else {
				count = 0;
			}
		}
	}

	spin_unlock(&epma_dev->rmem_lock);
	devm_kfree(epma_dev->dev, pma);

	dev_err(epma_dev->dev,
		"not enough contiguous pages (need %zu), total free pages left %zu\n",
		num_pages_to_alloc, epma_dev->num_free_pages);
	return NULL;
}

static phys_addr_t simple_pma_get_phys_addr(struct protected_memory_allocator_device *pma_dev,
					    struct protected_memory_allocation *pma)
{
	struct simple_pma_device *const epma_dev =
		container_of(pma_dev, struct simple_pma_device, pma_dev);

	dev_dbg(epma_dev->dev, "%s(pma_dev=%px, pma=%px, pa=%pK\n", __func__, (void *)pma_dev,
		(void *)pma, (void *)pma->pa);

	return pma->pa;
}

static void simple_pma_free_page(struct protected_memory_allocator_device *pma_dev,
				 struct protected_memory_allocation *pma)
{
	struct simple_pma_device *const epma_dev =
		container_of(pma_dev, struct simple_pma_device, pma_dev);
	size_t num_pages_in_allocation;
	size_t offset;
	size_t i;
	size_t bitfield_idx;
	size_t bitfield_start_bit;
	size_t page_num;
	u64 *bitfield;
	size_t alloc_pages_bitmap_size;
	size_t num_bitfield_elems_used_by_alloc;

	WARN_ON(pma == NULL);

	dev_dbg(epma_dev->dev, "%s(pma_dev=%px, pma=%px, pa=%pK\n", __func__, (void *)pma_dev,
		(void *)pma, (void *)pma->pa);

	WARN_ON(pma->pa < epma_dev->rmem_base);

	/* This is an example function that follows an extremely simple logic
	 * and is vulnerable to abuse.
	 */
	offset = (pma->pa - epma_dev->rmem_base);
	num_pages_in_allocation = (size_t)1 << pma->order;

	/* The number of bitfield elements used by the allocation */
	num_bitfield_elems_used_by_alloc = num_pages_in_allocation / PAGES_PER_BITFIELD_ELEM;

	/* The page number of the first page of the allocation, relative to rmem_base */
	page_num = offset >> PAGE_SHIFT;

	/* Which u64 bitfield refers to this page? */
	bitfield_idx = page_num / PAGES_PER_BITFIELD_ELEM;

	alloc_pages_bitmap_size = ALLOC_PAGES_BITFIELD_ARR_SIZE(epma_dev->rmem_size);

	/* Is the allocation within expected bounds? */
	WARN_ON((bitfield_idx + num_bitfield_elems_used_by_alloc) >= alloc_pages_bitmap_size);

	spin_lock(&epma_dev->rmem_lock);

	if (pma->order < ORDER_OF_PAGES_PER_BITFIELD_ELEM) {
		bitfield = &epma_dev->allocated_pages_bitfield_arr[bitfield_idx];

		/* Which bit within that u64 bitfield is the lsb covering this allocation?  */
		bitfield_start_bit = page_num % PAGES_PER_BITFIELD_ELEM;

		/* Clear the bits for the pages we're now freeing */
		*bitfield &= ~(((1ULL << num_pages_in_allocation) - 1) << bitfield_start_bit);
	} else {
		WARN(page_num % PAGES_PER_BITFIELD_ELEM,
		     "%s: Expecting allocs of order >= %d to be %zu-page aligned\n", __func__,
		     ORDER_OF_PAGES_PER_BITFIELD_ELEM, PAGES_PER_BITFIELD_ELEM);

		for (i = 0; i < num_bitfield_elems_used_by_alloc; i++) {
			bitfield = &epma_dev->allocated_pages_bitfield_arr[bitfield_idx + i];

			/* We expect all bits to be set (all pages allocated) */
			WARN((*bitfield != ~0ULL),
			     "%s: alloc being freed is not fully allocated: of=%zu np=%zu bf=%.16llx\n",
			     __func__, offset, num_pages_in_allocation, *bitfield);

			/*
			 * Now clear all the bits in the bitfield element to mark all the pages
			 * it refers to as free.
			 */
			*bitfield = 0ULL;
		}
	}

	epma_dev->num_free_pages += num_pages_in_allocation;
	spin_unlock(&epma_dev->rmem_lock);
	devm_kfree(epma_dev->dev, pma);
}

#if IS_ENABLED(CONFIG_MALI_MTK_GPU_IOMMU)
static int mtk_gpu_iommu_init(struct platform_device *pdev)
{
	int ret = 1;
	struct device *dev = &pdev->dev;

#if defined(CONFIG_MTK_GPUFREQ_V2)

#if IS_ENABLED(CONFIG_MALI_MTK_GHPM_STAGE1_ENABLE)
	/* On mfg0 and gpueb */
	ret = gpueb_ctrl(GHPM_ON, MFG1_OFF, SUSPEND_POWER_ON);
	if (ret) {
		dev_err(dev, "gpueb on fail, return value=%d \n", ret);
		return ret;
	}
#endif /* CONFIG_MALI_MTK_GHPM_STAGE1_ENABLE */

	/* on,off/ SWCG(BG3D)/ MTCMOS/ BUCK */
	if (gpufreq_power_control(GPU_PWR_ON) < 0) {
		dev_err(dev, "Power On Failed");
		return ret;
	}

	/* Control runtime active-sleep state of GPU */
	if (gpufreq_active_sleep_control(GPU_PWR_ON) < 0) {
		dev_err(dev, "Active Failed (on)");
		return ret;
	}
#endif /* CONFIG_MTK_GPUFREQ_V2 */

	/* Create platform device for the sub node. */
	ret = of_platform_populate(dev->of_node, NULL, NULL, dev);
	if (ret) {
		dev_err(dev, "[gpu_iommu] Create sub node fail %d", ret);
		return ret;
	}

#if defined(CONFIG_MTK_GPUFREQ_V2)
	/* Control runtime active-sleep state of GPU */
	if (gpufreq_active_sleep_control(GPU_PWR_OFF) < 0) {
		dev_err(dev, "Sleep Failed (off)");
		return ret;
	}

	/* on,off/ SWCG(BG3D)/ MTCMOS/ BUCK */
	if (gpufreq_power_control(GPU_PWR_OFF) < 0) {
		dev_err(dev, "Power Off Failed");
		return 1;
	}


#if IS_ENABLED(CONFIG_MALI_MTK_GHPM_STAGE1_ENABLE)
	/* Off mfg0 and gpueb */
	ret = gpueb_ctrl(GHPM_OFF, MFG1_OFF, SUSPEND_POWER_OFF);
	if (ret) {
		dev_err(dev, "gpueb off fail, return value=%d \n", ret);
		return ret;
	}
#endif /* CONFIG_MALI_MTK_GHPM_STAGE1_ENABLE */
#endif /* CONFIG_MTK_GPUFREQ_V2 */

	dev_info(dev, "[gpu_iommu] init done %d", ret);
	return ret;
}
#endif /* CONFIG_MALI_MTK_GPU_IOMMU */

#if IS_ENABLED(CONFIG_MALI_MTK_GPU_PROTECTED_MEMORY_SUPPORT)
static int protected_memory_allocator_probe(struct platform_device *pdev) __attribute__((unused));
#endif /* CONFIG_MALI_MTK_GPU_PROTECTED_MEMORY_SUPPORT */
static int protected_memory_allocator_probe(struct platform_device *pdev)
{
	struct simple_pma_device *epma_dev;
	struct device_node *np;
	phys_addr_t rmem_base;
	size_t rmem_size;
	size_t alloc_bitmap_pages_arr_size;
#if (KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE)
	struct reserved_mem *rmem;
#endif

	np = pdev->dev.of_node;

	if (!np) {
		dev_err(&pdev->dev, "device node pointer not set\n");
		return -ENODEV;
	}

	np = of_parse_phandle(np, "memory-region", 0);
	if (!np) {
		dev_err(&pdev->dev, "memory-region node not set\n");
		return -ENODEV;
	}

#if (KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE)
	rmem = of_reserved_mem_lookup(np);
	if (rmem) {
		rmem_base = rmem->base;
		rmem_size = rmem->size >> PAGE_SHIFT;
	} else
#endif
	{
		of_node_put(np);
		dev_err(&pdev->dev, "could not read reserved memory-region\n");
		return -ENODEV;
	}

	of_node_put(np);
	epma_dev = devm_kzalloc(&pdev->dev, sizeof(*epma_dev), GFP_KERNEL);
	if (!epma_dev)
		return -ENOMEM;

	epma_dev->pma_dev.ops.pma_alloc_page = simple_pma_alloc_page;
	epma_dev->pma_dev.ops.pma_get_phys_addr = simple_pma_get_phys_addr;
	epma_dev->pma_dev.ops.pma_free_page = simple_pma_free_page;
	epma_dev->pma_dev.owner = THIS_MODULE;
	epma_dev->dev = &pdev->dev;
	epma_dev->rmem_base = rmem_base;
	epma_dev->rmem_size = rmem_size;
	epma_dev->num_free_pages = rmem_size;
	spin_lock_init(&epma_dev->rmem_lock);

	alloc_bitmap_pages_arr_size = ALLOC_PAGES_BITFIELD_ARR_SIZE(epma_dev->rmem_size);

	epma_dev->allocated_pages_bitfield_arr = devm_kzalloc(
		&pdev->dev, alloc_bitmap_pages_arr_size * BITFIELD_ELEM_SIZE, GFP_KERNEL);

	if (!epma_dev->allocated_pages_bitfield_arr) {
		dev_err(&pdev->dev, "failed to allocate resources\n");
		devm_kfree(&pdev->dev, epma_dev);
		return -ENOMEM;
	}

	if (epma_dev->rmem_size % PAGES_PER_BITFIELD_ELEM) {
		size_t extra_pages =
			alloc_bitmap_pages_arr_size * PAGES_PER_BITFIELD_ELEM - epma_dev->rmem_size;
		size_t last_bitfield_index = alloc_bitmap_pages_arr_size - 1;

		/* Mark the extra pages (that lie outside the reserved range) as
		 * always in use.
		 */
		epma_dev->allocated_pages_bitfield_arr[last_bitfield_index] =
			((1ULL << extra_pages) - 1) << (PAGES_PER_BITFIELD_ELEM - extra_pages);
	}

	platform_set_drvdata(pdev, &epma_dev->pma_dev);
	dev_info(&pdev->dev, "Protected memory allocator probed successfully\n");
	dev_info(&pdev->dev, "Protected memory region: base=%pK num pages=%zu\n", (void *)rmem_base,
		 rmem_size);

	return 0;
}

#if IS_ENABLED(CONFIG_MALI_MTK_GPU_PROTECTED_MEMORY_SUPPORT)
/* Below macro is hard coded*/
#define GPR(X, Y) (X + (Y << 2))
static int get_gpueb_gpr_val_v1(struct platform_device *pdev, uint32_t gpr_id, uint64_t *p_GPR_target_64)
{
	struct device_node *np;
	struct resource *res = NULL;
	void __iomem *gpueb_base;
	void __iomem *GPR_target;
	uint32_t gpr_offset;

	np = pdev->dev.of_node;

	if (!np) {
		dev_err(&pdev->dev, "device node pointer not set\n");
		return -ENODEV;
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "gpueb_base");

	if (!res) {
		dev_err(&pdev->dev, "can't have GPR access\n");
		return -ENODEV;
	}

	of_property_read_u32(np, "gpr-offset", &gpr_offset);

	if(!gpr_offset) {
		dev_err(&pdev->dev, "can't have GPR offset access\n");
		return -ENODEV;
	}

	dev_info(&pdev->dev,
		"Using on addr(base + %x, %d)\n",
                 gpr_offset, gpr_id);

	gpueb_base = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (unlikely(!gpueb_base)) {
		dev_err(&pdev->dev, "fail to ioremap gpueb_base: 0x%llx", (unsigned long long)res->start);
		return -ENODEV;
	}
	GPR_target = GPR(gpueb_base + gpr_offset, gpr_id);

	/* Note GPR is 32 bits */
	*p_GPR_target_64 = *(uint32_t*)GPR_target;
	devm_iounmap(&pdev->dev, gpueb_base);

	return 0;
}

static int get_gpueb_gpr_val_v2(uint32_t gpr_id, uint64_t *p_GPR_target_64)
{
	void __iomem *gpueb_gpr_base;
	void __iomem *GPR_target;
	struct platform_device *pdev = NULL;
	struct device *gpueb_dev = NULL;
	struct device_node *of_gpueb = NULL;
	struct resource *res = NULL;

	pr_info("Using on gpr_id(%d)\n", gpr_id);

	of_gpueb = of_find_compatible_node(NULL, NULL, "mediatek,gpueb");
	if (!of_gpueb) {
		pr_err("fail to find gpueb of_node");
		return -ENODEV;
	}
	/* find our device by node */
	pdev = of_find_device_by_node(of_gpueb);
	gpueb_dev = &pdev->dev;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "gpueb_gpr_base");
	if (unlikely(!res)) {
		pr_err("fail to get resource gpueb_gpr_base");
		return -ENODEV;
	}
	gpueb_gpr_base = devm_ioremap(gpueb_dev, res->start, resource_size(res));
	if (unlikely(!gpueb_gpr_base)) {
		pr_err("fail to ioremap gpueb_gpr_base: 0x%llx", (unsigned long long)res->start);
		return -ENODEV;
	}
	GPR_target = GPR(gpueb_gpr_base, gpr_id);

	/* Note GPR is 32 bits */
	*p_GPR_target_64 = *(uint32_t*)GPR_target;
	devm_iounmap(&pdev->dev, gpueb_gpr_base);

	return 0;
}

static int mtk_protected_memory_allocator_probe(struct platform_device *pdev)
{
	struct simple_pma_device *epma_dev;
	struct device_node *np;
	phys_addr_t rmem_base = 0;
	size_t rmem_size;
	size_t alloc_bitmap_pages_arr_size;
	uint32_t gpr_id, gmpu_table_size, psize, pma_version;
	uint64_t GPR_target_64;
#if IS_ENABLED(CONFIG_MALI_MTK_GPU_PMA_PAGE_HEAP_2MB)
	struct protected_memory_allocation *pma;
#endif /* CONFIG_MALI_MTK_GPU_PMA_PAGE_HEAP_2MB */
#if IS_ENABLED(CONFIG_MALI_MTK_GPU_IOMMU)
	uint32_t dis_init_gpu_iommu = 0;
#endif /* CONFIG_MALI_MTK_GPU_IOMMU */

	np = pdev->dev.of_node;

	if (!np) {
		dev_err(&pdev->dev, "device node pointer not set\n");
		return -ENODEV;
	}

#if IS_ENABLED(CONFIG_MALI_MTK_GPU_IOMMU)
	of_property_read_u32(np, "disable-init-gpu-iommu", &dis_init_gpu_iommu);
	if(dis_init_gpu_iommu == 0) {
		if(mtk_gpu_iommu_init(pdev)) {
			dev_err(&pdev->dev, "can't init gpu iommu\n");
			return -ENODEV;
		}
	}else{
		dev_info(&pdev->dev, "Skip init gpu iommu\n");
	}
#endif /* CONFIG_MALI_MTK_GPU_IOMMU */
	of_property_read_u32(np, "gmpu-table-size", &gmpu_table_size);
	of_property_read_u32(np, "gpr-id", &gpr_id);
	of_property_read_u32(np, "protected-reserve-size", &psize);

	if(!psize) {
		dev_err(&pdev->dev, "can't find reserved-memory\n");
		return -ENODEV;
	}

	of_property_read_u32(np, "pma-version", &pma_version);
#if IS_ENABLED(CONFIG_MALI_MTK_GPU_PMA_PAGE_HEAP) && !IS_ENABLED(CONFIG_MALI_MTK_GPU_PMA_PAGE_HEAP_2MB)
	of_node_put(np);
	epma_dev = devm_kzalloc(&pdev->dev, sizeof(*epma_dev), GFP_KERNEL);
	if (!epma_dev)
		return -ENOMEM;

	epma_dev->pma_dev.ops.pma_alloc_page = simple_pma_alloc_dma_page;
	epma_dev->pma_dev.ops.pma_get_phys_addr = simple_pma_get_dma_phys_addr;
	epma_dev->pma_dev.ops.pma_free_page = simple_pma_free_dma_page;
	epma_dev->pma_dev.owner = THIS_MODULE;
	epma_dev->dev = &pdev->dev;

	platform_set_drvdata(pdev, &epma_dev->pma_dev);
	dev_info(&pdev->dev,
		"Protected memory allocator probed successfully\n");

	//pma_alloc_test(&epma_dev->pma_dev);
	if (dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(44))) //TODO DMA_BIT_MASK(kbdev->gpu_props.mmu.pa_bits), dma_set_mask_and_coherent or dma_set_mask
		return -ENOMEM;
#else
#if IS_ENABLED(CONFIG_MALI_MTK_GPU_PMA_PAGE_HEAP_2MB)
	of_node_put(np);
	epma_dev = devm_kzalloc(&pdev->dev, sizeof(*epma_dev), GFP_KERNEL);
	if (!epma_dev)
		return -ENOMEM;

	epma_dev->pma_dev.ops.pma_alloc_page = simple_pma_alloc_page;
	epma_dev->pma_dev.ops.pma_get_phys_addr = simple_pma_get_phys_addr;
	epma_dev->pma_dev.ops.pma_free_page = simple_pma_free_page;
	epma_dev->pma_dev.owner = THIS_MODULE;
	epma_dev->dev = &pdev->dev;

	pma = simple_pma_alloc_dma_page(&epma_dev->pma_dev, psize>>PAGE_SHIFT); // 200 pages*4k
	if (!pma)
		return -ENOMEM;

	rmem_base = simple_pma_get_dma_phys_addr(&epma_dev->pma_dev, pma);
	rmem_size = psize;
	rmem_size = rmem_size >> PAGE_SHIFT;
	dev_info(&pdev->dev, "addr=0x%llx, pg=%zu, psize=%u\n", rmem_base, rmem_size, psize);
	epma_dev->rmem_base = rmem_base;
	epma_dev->rmem_size = rmem_size;
	epma_dev->num_free_pages = rmem_size;
	spin_lock_init(&epma_dev->rmem_lock);
#else
	if(pma_version == 2)
	{
		if( 0 != get_gpueb_gpr_val_v2(gpr_id, &GPR_target_64))
		{
			dev_err(&pdev->dev, "can't get gpueb gpr (%d)\n", gpr_id);
			return -ENODEV;
		}
	}else{
		if( 0 != get_gpueb_gpr_val_v1(pdev, gpr_id, &GPR_target_64))
		{
			dev_err(&pdev->dev, "can't get gpueb gpr (%d)\n", gpr_id);
			return -ENODEV;
		}
	}

	rmem_base = (GPR_target_64 << PAGE_SHIFT) + gmpu_table_size;
	rmem_size = psize; // at least 256KB
	rmem_size = rmem_size >> PAGE_SHIFT;

	dev_info(&pdev->dev,
		"addr=0x%llx, size=%zu pages, gmpu_table_size=+%u\n",
		(unsigned long long)rmem_base, rmem_size, gmpu_table_size);

	of_node_put(np);
	epma_dev = devm_kzalloc(&pdev->dev, sizeof(*epma_dev), GFP_KERNEL);
	if (!epma_dev)
		return -ENOMEM;

	epma_dev->pma_dev.ops.pma_alloc_page = simple_pma_alloc_page;
	epma_dev->pma_dev.ops.pma_get_phys_addr = simple_pma_get_phys_addr;
	epma_dev->pma_dev.ops.pma_free_page = simple_pma_free_page;
	epma_dev->pma_dev.owner = THIS_MODULE;
	epma_dev->dev = &pdev->dev;
	epma_dev->rmem_base = rmem_base;
	epma_dev->rmem_size = rmem_size;
	epma_dev->num_free_pages = rmem_size;
	spin_lock_init(&epma_dev->rmem_lock);
#endif /* CONFIG_MALI_MTK_GPU_PMA_PAGE_HEAP_2MB */
	alloc_bitmap_pages_arr_size = ALLOC_PAGES_BITFIELD_ARR_SIZE(epma_dev->rmem_size);

	epma_dev->allocated_pages_bitfield_arr = devm_kzalloc(&pdev->dev,
		alloc_bitmap_pages_arr_size * BITFIELD_ELEM_SIZE, GFP_KERNEL);

	if (!epma_dev->allocated_pages_bitfield_arr) {
		dev_err(&pdev->dev, "failed to allocate resources\n");
		devm_kfree(&pdev->dev, epma_dev);
		return -ENOMEM;
	}

	if (epma_dev->rmem_size % PAGES_PER_BITFIELD_ELEM) {
		size_t extra_pages =
			alloc_bitmap_pages_arr_size * PAGES_PER_BITFIELD_ELEM -
			epma_dev->rmem_size;
		size_t last_bitfield_index = alloc_bitmap_pages_arr_size - 1;

		/* Mark the extra pages (that lie outside the reserved range) as
		 * always in use.
		 */
		epma_dev->allocated_pages_bitfield_arr[last_bitfield_index] =
			((1ULL << extra_pages) - 1) <<
			(PAGES_PER_BITFIELD_ELEM - extra_pages);
	}
#endif /* CONFIG_MALI_MTK_GPU_PMA_PAGE_HEAP */

	platform_set_drvdata(pdev, &epma_dev->pma_dev);
	dev_info(&pdev->dev,
		"Protected memory allocator probed successfully\n");
	dev_info(&pdev->dev, "Protected memory region: base=%llx num pages=%zu\n",
		(unsigned long long)epma_dev->rmem_base, epma_dev->rmem_size);

	return 0;
}
#endif /* CONFIG_MALI_MTK_GPU_PROTECTED_MEMORY_SUPPORT */

static int protected_memory_allocator_remove(struct platform_device *pdev)
{
	struct protected_memory_allocator_device *pma_dev = platform_get_drvdata(pdev);
	struct simple_pma_device *epma_dev;
	struct device *dev;

	if (!pma_dev)
		return -EINVAL;

	epma_dev = container_of(pma_dev, struct simple_pma_device, pma_dev);
	dev = epma_dev->dev;

	if (epma_dev->num_free_pages < epma_dev->rmem_size) {
		dev_warn(&pdev->dev, "Leaking %zu pages of protected memory\n",
			 epma_dev->rmem_size - epma_dev->num_free_pages);
	}

	platform_set_drvdata(pdev, NULL);
	devm_kfree(dev, epma_dev->allocated_pages_bitfield_arr);
	devm_kfree(dev, epma_dev);

	dev_info(&pdev->dev, "Protected memory allocator removed successfully\n");

	return 0;
}

static const struct of_device_id protected_memory_allocator_dt_ids[] = {
	{ .compatible = "arm,protected-memory-allocator" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, protected_memory_allocator_dt_ids);

static struct platform_driver
#if IS_ENABLED(CONFIG_MALI_MTK_GPU_PROTECTED_MEMORY_SUPPORT)
	protected_memory_allocator_driver = { .probe = mtk_protected_memory_allocator_probe,
#else
	protected_memory_allocator_driver = { .probe = protected_memory_allocator_probe,
#endif /* CONFIG_MALI_MTK_GPU_PROTECTED_MEMORY_SUPPORT */
					      .remove = protected_memory_allocator_remove,
					      .driver = {
						      .name = "simple_protected_memory_allocator",
						      .of_match_table = of_match_ptr(
							      protected_memory_allocator_dt_ids),
					      } };

module_platform_driver(protected_memory_allocator_driver);

MODULE_IMPORT_NS(DMA_BUF);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("ARM Ltd.");
MODULE_VERSION("1.0");
