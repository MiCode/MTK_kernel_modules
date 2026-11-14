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

#include <linux/fs.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#include <linux/module.h>
#if IS_ENABLED(CONFIG_DEBUG_FS)
#include <linux/debugfs.h>
#include <linux/version_compat_defs.h>
#endif
#include <linux/mm.h>
#include <linux/memory_group_manager.h>
#if IS_ENABLED(CONFIG_MALI_MTK_MGMM)
#include <linux/shrinker.h>
#include <linux/ktime.h>
#include <soc/mediatek/emi.h>
#define MTK_EMI_DRAM_OFFSET 0x40000000
#define PREFILL_TARGET (0)
#define RANK_POOL_LIMIT (SZ_256M >> PAGE_SHIFT)
#define X_GUARD (SZ_16M >> PAGE_SHIFT)
#define REFILL_TARGET (X_GUARD << 1)
#define NORMAL_MODE (3)
#define RELAX_MODE (4)
#define BYPASS_MODE (5)
#endif /* CONFIG_MALI_MTK_MGMM */
#if IS_ENABLED(CONFIG_MALI_MTK_SLC_ALL_CACHE_MODE)
#include <mtk_heap.h>
#include <slbc_ops.h>
#endif
#include "mtk_platform_utils.h" /* MTK_INLINE */

#if (KERNEL_VERSION(4, 20, 0) > LINUX_VERSION_CODE)
static inline vm_fault_t vmf_insert_pfn_prot(struct vm_area_struct *vma,
			unsigned long addr, unsigned long pfn, pgprot_t pgprot)
{
	int err = vm_insert_pfn_prot(vma, addr, pfn, pgprot);

	if (unlikely(err == -ENOMEM))
		return VM_FAULT_OOM;
	if (unlikely(err < 0 && err != -EBUSY))
		return VM_FAULT_SIGBUS;

	return VM_FAULT_NOPAGE;
}
#endif

#define PTE_PBHA_SHIFT (59)
#define PTE_PBHA_MASK ((uint64_t)0xf << PTE_PBHA_SHIFT)
#define PTE_RES_BIT_MULTI_AS_SHIFT (63)

#define IMPORTED_MEMORY_ID (MEMORY_GROUP_MANAGER_NR_GROUPS - 1)

/**
 * struct mgm_group - Structure to keep track of the number of allocated
 *                    pages per group
 *
 * @size:  The number of allocated small(4KB) pages
 * @lp_size:  The number of allocated large(2MB) pages
 * @insert_pfn: The number of calls to map pages for CPU access.
 * @update_gpu_pte: The number of calls to update GPU page table entries.
 *
 * This structure allows page allocation information to be displayed via
 * debugfs. Display is organized per group with small and large sized pages.
 */
struct mgm_group {
	atomic_t size;
	atomic_t lp_size;
	atomic_t insert_pfn;
	atomic_t update_gpu_pte;
};

/**
 * struct mgm_groups - Structure for groups of memory group manager
 *
 * @groups: To keep track of the number of allocated pages of all groups
 * @dev: device attached
 * @mgm_debugfs_root: debugfs root directory of memory group manager
 *
 * This structure allows page allocation information to be displayed via
 * debugfs. Display is organized per group with small and large sized pages.
 */
struct mgm_groups {
	struct mgm_group groups[MEMORY_GROUP_MANAGER_NR_GROUPS];
	struct device *dev;
#if IS_ENABLED(CONFIG_DEBUG_FS)
	struct dentry *mgm_debugfs_root;
#endif

#if IS_ENABLED(CONFIG_MALI_MTK_MGMM)
	spinlock_t MGMFree_lst_lk;
	spinlock_t free_4K_lst_lk;
	struct list_head free_4K_lst;
	struct list_head free_list_r[2][2];
	size_t nr_rank[2][2];
	size_t max_pool[2];
	bool bRank0[2]; // true: rank0, false: rank1
	int rank_mode;
	gfp_t gfp_mask;
	size_t count;
	struct shrinker reclaim;
	uint64_t ui64RankBoundary;
	size_t szRefillTarget;
	size_t szSelectTarget;
#endif /* CONFIG_MALI_MTK_MGMM */
};

#if IS_ENABLED(CONFIG_DEBUG_FS)

static int mgm_size_get(void *data, u64 *val)
{
	struct mgm_group *group = data;

	*val = atomic_read(&group->size);

	return 0;
}

static int mgm_lp_size_get(void *data, u64 *val)
{
	struct mgm_group *group = data;
	*val = atomic_read(&group->lp_size);
	return 0;
}

static int mgm_insert_pfn_get(void *data, u64 *val)
{
	struct mgm_group *group = data;
	*val = atomic_read(&group->insert_pfn);
	return 0;
}

static int mgm_update_gpu_pte_get(void *data, u64 *val)
{
	struct mgm_group *group = data;
	*val = atomic_read(&group->update_gpu_pte);
	return 0;
}


#if IS_ENABLED(CONFIG_MALI_MTK_MGMM)
static int rank_mode_get(void *data, u64 *val)
{
	int rank_mode = *(int*)data;
	*val = rank_mode;

	return 0;
}

static int rank_mode_set(void *data, u64 val)
{
	int *rank_mode = (int *)data;
	*rank_mode = val;

	return 0;
}

void mtk_mgm_pool_fill(struct mgm_groups *data, int order, int i32Rank, size_t target);
void mtk_mgm_pool_flush(struct mgm_groups *data, int order, int rank, size_t target, size_t target_waste);
static int rank0_get(void *data, u64 *val)
{
	struct mgm_groups *mgm_data;

	mgm_data = (struct mgm_groups *)data;
	*val = mgm_data->nr_rank[0][0];

	return 0;
}

static int rank0_set(void *data, u64 val)
{
	struct mgm_groups *mgm_data;
	int64_t tmp;

	mgm_data = (struct mgm_groups *)data;
	tmp = val - mgm_data->nr_rank[0][0];

	if (tmp > 0)
		mtk_mgm_pool_fill(mgm_data, 0, 0, val);
	else
		mtk_mgm_pool_flush(mgm_data, 0, 0, val, 0);

	return 0;
}

static int rank1_get(void *data, u64 *val)
{
	struct mgm_groups *mgm_data;

	mgm_data = (struct mgm_groups *)data;
	*val = mgm_data->nr_rank[0][1];

	return 0;
}

static int rank1_set(void *data, u64 val)
{
	struct mgm_groups *mgm_data;
	int64_t tmp;

	mgm_data = (struct mgm_groups *)data;
	tmp = val - mgm_data->nr_rank[0][1];

	if (tmp > 0)
		mtk_mgm_pool_fill(mgm_data, 0, 1, val);
	else
		mtk_mgm_pool_flush(mgm_data, 0, 1, val, 0);

	return 0;
}

static int lp_rank0_get(void *data, u64 *val)
{
	struct mgm_groups *mgm_data;

	mgm_data = (struct mgm_groups *)data;
	*val = mgm_data->nr_rank[1][0];

	return 0;
}

static int lp_rank0_set(void *data, u64 val)
{
	struct mgm_groups *mgm_data;
	int64_t tmp;

	mgm_data = (struct mgm_groups *)data;
	tmp = val - mgm_data->nr_rank[1][0];

	if (tmp > 0)
		mtk_mgm_pool_fill(mgm_data, 9, 0, val);
	else
		mtk_mgm_pool_flush(mgm_data, 9, 0, val, 0);

	return 0;
}

static int lp_rank1_get(void *data, u64 *val)
{
	struct mgm_groups *mgm_data;

	mgm_data = (struct mgm_groups *)data;
	*val = mgm_data->nr_rank[1][1];

	return 0;
}

static int lp_rank1_set(void *data, u64 val)
{
	struct mgm_groups *mgm_data;
	int64_t tmp;

	mgm_data = (struct mgm_groups *)data;
	tmp = val - mgm_data->nr_rank[1][1];

	if (tmp > 0)
		mtk_mgm_pool_fill(mgm_data, 9, 1, val);
	else
		mtk_mgm_pool_flush(mgm_data, 9, 1, val, 0);

	return 0;
}

static int refill_get(void *data, u64 *val)
{
	struct mgm_groups *mgm_data;

	mgm_data = (struct mgm_groups *)data;
	*val = mgm_data->szRefillTarget;

	return 0;
}

static int refill_set(void *data, u64 val)
{
	struct mgm_groups *mgm_data;

	mgm_data = (struct mgm_groups *)data;
	mgm_data->szRefillTarget = val;

	return 0;
}

static int sel_target_get(void *data, u64 *val)
{
	struct mgm_groups *mgm_data;

	mgm_data = (struct mgm_groups *)data;
	*val = mgm_data->szSelectTarget;

	return 0;
}

static int sel_target_set(void *data, u64 val)
{
	struct mgm_groups *mgm_data;

	mgm_data = (struct mgm_groups *)data;
	mgm_data->szSelectTarget = val;

	return 0;
}

static int sel_count_get(void *data, u64 *val)
{
	struct mgm_groups *mgm_data;

	mgm_data = (struct mgm_groups *)data;
	*val = mgm_data->count;

	return 0;
}

static int sel_get(void *data, u64 *val)
{
	struct mgm_groups *mgm_data;

	mgm_data = (struct mgm_groups *)data;
	*val = 0x0;
	if (!mgm_data->bRank0[0])
		*val = 0x1;
	if (!mgm_data->bRank0[1])
		*val |= 0x10;

	return 0;
}

static int max_pool_mb_set(void *data, u64 val)
{
	struct mgm_groups *mgm_data;

	mgm_data = (struct mgm_groups *)data;

	mgm_data->max_pool[0] = (val & 0xffffffff) << 8;
	mgm_data->max_pool[1] = ((val >> 32) << 8);

	return 0;
}

static int max_pool_mb_get(void *data, u64 *val)
{
	struct mgm_groups *mgm_data;

	mgm_data = (struct mgm_groups *)data;
	*val = (mgm_data->max_pool[1] >> 8) << 32 | (mgm_data->max_pool[0] >> 8);

	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(fops_rank_mode, rank_mode_get, rank_mode_set, "%llu\n");
DEFINE_DEBUGFS_ATTRIBUTE(fops_rank0, rank0_get, rank0_set, "%llu\n");
DEFINE_DEBUGFS_ATTRIBUTE(fops_rank1, rank1_get, rank1_set, "%llu\n");
DEFINE_DEBUGFS_ATTRIBUTE(fops_LPrank0, lp_rank0_get, lp_rank0_set, "%llu\n");
DEFINE_DEBUGFS_ATTRIBUTE(fops_LPrank1, lp_rank1_get, lp_rank1_set, "%llu\n");
DEFINE_DEBUGFS_ATTRIBUTE(fops_refill, refill_get, refill_set, "%llu\n");
DEFINE_DEBUGFS_ATTRIBUTE(fops_sel_target, sel_target_get, sel_target_set, "%llu\n");
DEFINE_DEBUGFS_ATTRIBUTE(fops_sel_count, sel_count_get, NULL, "%#2llx\n");
DEFINE_DEBUGFS_ATTRIBUTE(fops_sel, sel_get, NULL, "%llx\n");
DEFINE_DEBUGFS_ATTRIBUTE(fops_max_pool_mb, max_pool_mb_get, max_pool_mb_set, "%#2llx\n");
#endif /* CONFIG_MALI_MTK_MGMM */
DEFINE_DEBUGFS_ATTRIBUTE(fops_mgm_size, mgm_size_get, NULL, "%llu\n");
DEFINE_DEBUGFS_ATTRIBUTE(fops_mgm_lp_size, mgm_lp_size_get, NULL, "%llu\n");
DEFINE_DEBUGFS_ATTRIBUTE(fops_mgm_insert_pfn, mgm_insert_pfn_get, NULL, "%llu\n");
DEFINE_DEBUGFS_ATTRIBUTE(fops_mgm_update_gpu_pte, mgm_update_gpu_pte_get, NULL, "%llu\n");

static void mgm_term_debugfs(struct mgm_groups *data)
{
	debugfs_remove_recursive(data->mgm_debugfs_root);
}

#define MGM_DEBUGFS_GROUP_NAME_MAX 10
static int mgm_initialize_debugfs(struct mgm_groups *mgm_data)
{
	int i;
	struct dentry *e, *g;
	char debugfs_group_name[MGM_DEBUGFS_GROUP_NAME_MAX];

	/*
	 * Create root directory of memory-group-manager
	 */
	mgm_data->mgm_debugfs_root =
		debugfs_create_dir("physical-memory-group-manager", NULL);
	if (IS_ERR_OR_NULL(mgm_data->mgm_debugfs_root)) {
		dev_err(mgm_data->dev, "fail to create debugfs root directory\n");
		return -ENODEV;
	}

	/*
	 * Create debugfs files per group
	 */
	for (i = 0; i < MEMORY_GROUP_MANAGER_NR_GROUPS; i++) {
		scnprintf(debugfs_group_name, MGM_DEBUGFS_GROUP_NAME_MAX,
				"group_%d", i);
		g = debugfs_create_dir(debugfs_group_name,
				mgm_data->mgm_debugfs_root);
		if (IS_ERR_OR_NULL(g)) {
			dev_err(mgm_data->dev, "fail to create group[%d]\n", i);
			goto remove_debugfs;
		}

		e = debugfs_create_file("size", 0444, g, &mgm_data->groups[i],
				&fops_mgm_size);
		if (IS_ERR_OR_NULL(e)) {
			dev_err(mgm_data->dev, "fail to create size[%d]\n", i);
			goto remove_debugfs;
		}

		e = debugfs_create_file("lp_size", 0444, g,
				&mgm_data->groups[i], &fops_mgm_lp_size);
		if (IS_ERR_OR_NULL(e)) {
			dev_err(mgm_data->dev,
				"fail to create lp_size[%d]\n", i);
			goto remove_debugfs;
		}

		e = debugfs_create_file("insert_pfn", 0444, g,
				&mgm_data->groups[i], &fops_mgm_insert_pfn);
		if (IS_ERR_OR_NULL(e)) {
			dev_err(mgm_data->dev,
				"fail to create insert_pfn[%d]\n", i);
			goto remove_debugfs;
		}

		e = debugfs_create_file("update_gpu_pte", 0444, g,
				&mgm_data->groups[i], &fops_mgm_update_gpu_pte);
		if (IS_ERR_OR_NULL(e)) {
			dev_err(mgm_data->dev,
				"fail to create update_gpu_pte[%d]\n", i);
			goto remove_debugfs;
		}
	}


#if IS_ENABLED(CONFIG_MALI_MTK_MGMM)
	e = debugfs_create_file("rank_mode", 0444, mgm_data->mgm_debugfs_root, &mgm_data->rank_mode,
			&fops_rank_mode);
	if (IS_ERR_OR_NULL(e)) {
		dev_vdbg(mgm_data->dev, "fail to create rank_mode\n");
		goto remove_debugfs;
	}

	e = debugfs_create_file("rank0", 0444, mgm_data->mgm_debugfs_root, mgm_data,
			&fops_rank0);
	if (IS_ERR_OR_NULL(e)) {
		dev_vdbg(mgm_data->dev, "fail to create rank0\n");
		goto remove_debugfs;
	}

	e = debugfs_create_file("rank1", 0444, mgm_data->mgm_debugfs_root, mgm_data,
			&fops_rank1);
	if (IS_ERR_OR_NULL(e)) {
		dev_vdbg(mgm_data->dev, "fail to create rank1\n");
		goto remove_debugfs;
	}

	e = debugfs_create_file("lp_rank0", 0444, mgm_data->mgm_debugfs_root, mgm_data,
			&fops_LPrank0);
	if (IS_ERR_OR_NULL(e)) {
		dev_vdbg(mgm_data->dev, "fail to create lp_rank0\n");
		goto remove_debugfs;
	}

	e = debugfs_create_file("lp_rank1", 0444, mgm_data->mgm_debugfs_root, mgm_data,
			&fops_LPrank1);
	if (IS_ERR_OR_NULL(e)) {
		dev_vdbg(mgm_data->dev, "fail to create lp_rank1\n");
		goto remove_debugfs;
	}

	e = debugfs_create_file("refill_target", 0444, mgm_data->mgm_debugfs_root, mgm_data,
			&fops_refill);
	if (IS_ERR_OR_NULL(e)) {
		dev_vdbg(mgm_data->dev, "fail to create refill_target\n");
		goto remove_debugfs;
	}

	e = debugfs_create_file("sel_target", 0444, mgm_data->mgm_debugfs_root, mgm_data,
			&fops_sel_target);
	if (IS_ERR_OR_NULL(e)) {
		dev_vdbg(mgm_data->dev, "fail to create sel_target\n");
		goto remove_debugfs;
	}

	e = debugfs_create_file("sel_count", 0444, mgm_data->mgm_debugfs_root, mgm_data,
			&fops_sel_count);
	if (IS_ERR_OR_NULL(e)) {
		dev_vdbg(mgm_data->dev, "fail to create sel_count\n");
		goto remove_debugfs;
	}

	e = debugfs_create_file("sel", 0444, mgm_data->mgm_debugfs_root, mgm_data,
			&fops_sel);
	if (IS_ERR_OR_NULL(e)) {
		dev_vdbg(mgm_data->dev, "fail to create sel\n");
		goto remove_debugfs;
	}

	e = debugfs_create_file("max_pool_mb", 0444, mgm_data->mgm_debugfs_root, mgm_data,
			&fops_max_pool_mb);
	if (IS_ERR_OR_NULL(e)) {
		dev_vdbg(mgm_data->dev, "fail to create max_pool_mb\n");
		goto remove_debugfs;
	}
#endif /* CONFIG_MALI_MTK_MGMM */
	return 0;

remove_debugfs:
	mgm_term_debugfs(mgm_data);
	return -ENODEV;
}

#else

static void mgm_term_debugfs(struct mgm_groups *data)
{
}

static int mgm_initialize_debugfs(struct mgm_groups *mgm_data)
{
	return 0;
}

#endif /* CONFIG_DEBUG_FS */

#define ORDER_SMALL_PAGE 0
#define ORDER_LARGE_PAGE 9
static void update_size(struct memory_group_manager_device *mgm_dev, unsigned int group_id,
			int order, bool alloc)
{
	struct mgm_groups *data = mgm_dev->data;

	switch (order) {
	case ORDER_SMALL_PAGE:
		if (alloc)
			atomic_inc(&data->groups[group_id].size);
		else {
			WARN_ON(atomic_read(&data->groups[group_id].size) == 0);
			atomic_dec(&data->groups[group_id].size);
		}
	break;

	case ORDER_LARGE_PAGE:
		if (alloc)
			atomic_inc(&data->groups[group_id].lp_size);
		else {
			WARN_ON(atomic_read(&data->groups[group_id].lp_size) == 0);
			atomic_dec(&data->groups[group_id].lp_size);
		}
	break;

	default:
		dev_err(data->dev, "Unknown order(%d)\n", order);
	break;
	}
}

#if IS_ENABLED(CONFIG_MALI_MTK_MGMM)
struct page* mtk_fetch_page(struct mgm_groups *data, int order, int i32Rank)
{
	struct page* p = NULL, *pp;
	int i, o = 0;
	int count;
	spin_lock(&data->MGMFree_lst_lk);
	i = i32Rank;

	if (order == 9)
		o = 1;

	if (data->nr_rank[o][i]) {
		p = list_first_entry(&data->free_list_r[o][i], struct page, lru);
		list_del_init(&p->lru);
		data->nr_rank[o][i]--;
		/* remove records from pools */
		mod_node_page_state(page_pgdat(p), NR_KERNEL_MISC_RECLAIMABLE, -(1 << order));
	}
	spin_unlock(&data->MGMFree_lst_lk);

	if (!p) {
		if (order == 0) { /* tried borrow from higher order */
			p = mtk_fetch_page(data, 9, i32Rank);

			if (p) {
				split_page(p, 9);
				count = (1 << 9) - 1;
				spin_lock(&data->MGMFree_lst_lk);
				pp = p + 1;
				while (count--) {
					list_add(&pp->lru, data->free_list_r[o] + i);
					pp++;
				}
				data->nr_rank[o][i] += 511;
				spin_unlock(&data->MGMFree_lst_lk);
				/* Add remaining page records back to cache pool */
				mod_node_page_state(page_pgdat(p), NR_KERNEL_MISC_RECLAIMABLE, 511);
			}
		}
	}

	return p;
}


static struct page *__MTKAllocPage(struct mgm_groups *data,
									gfp_t gfp_mask, unsigned int order)
{
	static size_t nr_4Kfree_lst = 0;
	static unsigned int try_order = 10;
	unsigned int order_scan_walk;
	unsigned int count;
	gfp_t horder_gfp_mask;
	struct page* p = NULL;
	struct page* pp = NULL;

	/*As now we only support pre-alloc for order 0 */
	if (order!=0)
		goto FALLBACK;


	/* if pre-alloc list pool got available 4K page */
	spin_lock(&data->free_4K_lst_lk);
	order_scan_walk = try_order;
	if (nr_4Kfree_lst) {
		p = list_first_entry(&data->free_4K_lst, struct page, lru);
		if (p) {
			list_del_init(&p->lru);
			nr_4Kfree_lst--;
			spin_unlock(&data->free_4K_lst_lk);
			return p;
		}
		else
			dev_err(data->dev, "Impossible! This is a bug\n");
	}
	spin_unlock(&data->free_4K_lst_lk);

	/* Try to alloc big page start from 10 */
	if (data->rank_mode == RELAX_MODE)
		horder_gfp_mask = ((gfp_mask & ~__GFP_RECLAIM) | __GFP_NORETRY | __GFP_NOWARN);
	else
		horder_gfp_mask = ((gfp_mask & ~__GFP_DIRECT_RECLAIM) | __GFP_NOWARN);

	while (order_scan_walk > order) {

		p = alloc_pages(horder_gfp_mask, order_scan_walk);
		if (p) {
			/* add batch records from system to cache memory */
			mod_node_page_state(page_pgdat(p), NR_KERNEL_MISC_RECLAIMABLE, (1 << order_scan_walk));
			split_page(p, order_scan_walk);
			count = (1 << order_scan_walk ) - 1;
			spin_lock(&data->free_4K_lst_lk);
			nr_4Kfree_lst += count;

			pp = p + 1;
			while (count--) {
				list_add(&pp->lru, &data->free_4K_lst);
				pp++;
			}
			if (order_scan_walk != 10)
				try_order = order_scan_walk + 1;
			spin_unlock(&data->free_4K_lst_lk);
			return p;
		}
		order_scan_walk--;
		dev_dbg(data->dev, "Order: empty; Try next order %u \n", order_scan_walk);
	}

FALLBACK:
	p = alloc_pages(gfp_mask, order);
	/* This page would insert into high order rank pool */
	if (p)
	mod_node_page_state(page_pgdat(p), NR_KERNEL_MISC_RECLAIMABLE, (1 << order));
	return p;
}

/*
 * MTKAllocPage would get one struct page per-call, however, if force_rank1 has been set,
 * total_count could be greater than 1 because target range PA may not meet easily
*/
static unsigned int MTKAllocPage(struct mgm_groups *data, gfp_t gfp_mask, int order, unsigned int force_rank1)
{
	struct page* p = NULL;
	int i, o = 0;
	unsigned int total_count = 0;

	if (order == 9)
		o = 1;

	do {
		p = __MTKAllocPage(data, gfp_mask, order);

		if (p) {
			spin_lock(&data->MGMFree_lst_lk);
			i = (page_to_phys(p) < data->ui64RankBoundary) ? 0 : 1;  // true: rank0, false: rank1
			list_add(&p->lru, data->free_list_r[o] + i);
			data->nr_rank[o][i]++;
			total_count++;
			spin_unlock(&data->MGMFree_lst_lk);
		} else {
			dev_dbg(data->dev, "%s encounter OOM, total_count=%u\n", __func__, total_count);
			break;
		}
	} while (force_rank1 == 1 && page_to_phys(p) < data->ui64RankBoundary &&
			data->nr_rank[o][0] < ((data->max_pool[o] >> order) << 1));

	return total_count;
}

void mtk_mgm_pool_flush(struct mgm_groups *data, int order, int rank, size_t target, size_t target_waste)
{
	struct page* pp = NULL;
	int o = 0;
	size_t count = 0;
	bool bFlip = false;
	size_t nr_pages_in;
	size_t w_count = 0;

	if (order == 9)
		o = 1;

	nr_pages_in = data->nr_rank[o][rank];

	if (target_waste)
		dev_dbg(data->dev, "Tried leak %zu pages\n", target_waste);

	while (data->nr_rank[o][rank] > target && (nr_pages_in - count)){
		pp = mtk_fetch_page(data, order, rank);

		if (bFlip || target_waste <= (nr_pages_in - count)) {
			__free_pages(pp, order);
			count++;
		} else
			w_count++;

		if (ktime_get()&0x1)
			bFlip = true;
		else
			bFlip = false;
	}

	while (target_waste > w_count) {
		pp = alloc_pages(GFP_HIGHUSER|__GFP_ZERO, order);
		if(pp)
			w_count++;
	}
	dev_dbg(data->dev, "Flush %d-pool[%d]: %zu freed, (%zu / %zu) pages wasted\n", order, rank, count, w_count, target_waste);
}


void mtk_mgm_pool_trim(struct mgm_groups *data, int order, int rank, size_t nr_pages)
{
	int o = 0;

	if (order == 9)
		o = 1;
	while (data->nr_rank[o][rank] > nr_pages)
			__free_pages(mtk_fetch_page(data, order, rank), order);
}

static unsigned long mtk_mgm_pool_reclaim_count_objects_local(size_t nr_rank, size_t target)
{
	unsigned long ret;
	if (nr_rank > target) {
		ret = ((nr_rank - target) >> 1);
	} else
		ret = nr_rank >> 3;

	if (ret >= (SZ_64M >> PAGE_SHIFT))
		 ret = (SZ_64M >> PAGE_SHIFT);

	 return ret;
}

static unsigned long mtk_mgm_pool_reclaim_count_objects(struct shrinker *s,
		struct shrink_control *sc)
{
	struct mgm_groups *data;
	size_t ret;

	data = container_of(s, struct mgm_groups, reclaim);
	ret = 0;

	ret += mtk_mgm_pool_reclaim_count_objects_local(data->nr_rank[0][0], data->szRefillTarget);
	ret += mtk_mgm_pool_reclaim_count_objects_local(data->nr_rank[0][1], data->szRefillTarget);

	ret += ((mtk_mgm_pool_reclaim_count_objects_local(data->nr_rank[1][0], data->szRefillTarget >> 9)) << 9);
	ret += ((mtk_mgm_pool_reclaim_count_objects_local(data->nr_rank[1][1], data->szRefillTarget >> 9)) << 9);

	return ret;
}


static unsigned long mtk_mgm_pool_reclaim_scan_objects(struct shrinker *s,
		struct shrink_control *sc)
{
	struct mgm_groups *data;
	size_t target = 0;
	size_t i = 0;
	size_t j = 0;
	size_t ret = 0;
	struct page *p;

	data = container_of(s, struct mgm_groups, reclaim);

	target = mtk_mgm_pool_reclaim_count_objects_local(data->nr_rank[0][0], data->szRefillTarget);
	for (i = 0; i < target; i++){
		p = mtk_fetch_page(data, 0, 0);
		if (p) {
			ret++;
			__free_pages(p, 0);
		} else
			break;
	}

	j = i;
	target = mtk_mgm_pool_reclaim_count_objects_local(data->nr_rank[0][1], data->szRefillTarget);

	for (i = 0; i < target; i++){
		p = mtk_fetch_page(data, 0, 1);
		if (p) {
			ret++;
			__free_pages(p, 0);
		} else
			break;
	}

	dev_dbg(data->dev, "mGMM pool[0]: reclaimed %zu (rank0:%zu, rank1:%zu)\n", j+i, j ,i);

	target = mtk_mgm_pool_reclaim_count_objects_local(data->nr_rank[1][0], data->szRefillTarget >> 9);
	for (i = 0; i < target; i++){
		p = mtk_fetch_page(data, 9, 0);
		if (p) {
			ret+=512;
			__free_pages(p, 9);
		} else
			break;
	}

	j = i;
	target = mtk_mgm_pool_reclaim_count_objects_local(data->nr_rank[1][1], data->szRefillTarget >> 9);

	for (i = 0; i < target; i++){
		p = mtk_fetch_page(data, 9, 1);
		if (p) {
			ret+=512;
			__free_pages(p, 9);
		} else
			break;
	}

	dev_dbg(data->dev, "mGMM pool[1]: reclaimed %zu (rank0:%zu, rank1:%zu)\n", j+i, j ,i);

	return ret;
}

void mtk_mgm_pool_fill(struct mgm_groups *data, int order, int i32Rank, size_t target)
{
	unsigned int nr_pages;
	size_t total_nr_pages = 0, in_nr_pages;
	int o = 0;

	if (order == 9)
		o = 1;

	in_nr_pages = data->nr_rank[o][i32Rank];
	while (data->nr_rank[o][i32Rank] < target) {
		nr_pages = MTKAllocPage(data, data->gfp_mask, order, i32Rank);
		total_nr_pages += nr_pages;
		if (nr_pages == 0 || total_nr_pages >= ((data->max_pool[o] >> order) << 1)) {
			dev_dbg(data->dev, "%s %d-pool[%d]: incompleted (%zu) / (%zu) | %u\n",
					__func__, order, i32Rank, data->nr_rank[o][i32Rank], total_nr_pages, nr_pages);
			return;
		}
	}
	dev_dbg(data->dev, "%s %d-pool[%d]: %zu -> %zu\n", __func__, order, i32Rank, in_nr_pages, target);
}
#endif /* CONFIG_MALI_MTK_MGMM */

/*
mode 0: rank0 first
mode 1: rank1 first
mode 2: rank0, rank1 cross per-page
...
*/
static struct page *example_mgm_alloc_page(
	struct memory_group_manager_device *mgm_dev, int group_id,
	gfp_t gfp_mask, unsigned int order)
{
	struct mgm_groups *const data = mgm_dev->data;
	struct page *p;

#if IS_ENABLED(CONFIG_MALI_MTK_MGMM)
	bool* pbRank0;
	int rank, o = 0;
	static int count = 0;
	int refill = 0;
	size_t tmp;
#endif /* CONFIG_MALI_MTK_MGMM */
	dev_vdbg(data->dev, "%s(mgm_dev=%pK, group_id=%d gfp_mask=0x%x order=%u\n", __func__,
		(void *)mgm_dev, group_id, gfp_mask, order);

	if (WARN_ON(group_id < 0) ||
		WARN_ON(group_id >= MEMORY_GROUP_MANAGER_NR_GROUPS))
		return NULL;
#if IS_ENABLED(CONFIG_MALI_MTK_MGMM)
	p = NULL;
	if (order == 0 || order == 9) { /* not support LP mode */

		if (data->gfp_mask != gfp_mask && ((data->gfp_mask | __GFP_NOWARN) != gfp_mask)) {
			dev_info(data->dev, "Change gfp_mask, drop all cached pool\n");
			mtk_mgm_pool_flush(data, 9, 0, 0, 0);
			mtk_mgm_pool_flush(data, 9, 1, 0, 0);
			mtk_mgm_pool_flush(data, 0, 0, 0, 0);
			mtk_mgm_pool_flush(data, 0, 1, 0, 0);
			data->gfp_mask = gfp_mask;
		}

		if (order == 9)
			o = 1;

		pbRank0 = data->bRank0 + o;
		rank = (*pbRank0) ? 0 : 1;

		if (data->rank_mode == 0) { /* rank-0 only */
			p = mtk_fetch_page(data, order, 0);
			if (!p)
				p = mtk_fetch_page(data, order, 1);
		} else if(data->rank_mode == 1) { /* rank-1 only */
			p = mtk_fetch_page(data, order, 1);
			if (!p)
				p = mtk_fetch_page(data, order, 0);
		} else if (data->rank_mode == 2) { /* per-4K flip */
			p = mtk_fetch_page(data, order, rank);
			*pbRank0 = !(*pbRank0);
		} else if (data->rank_mode >= 512) { /* per-rank_mode size flip */
			p = mtk_fetch_page(data, order, rank);
			count++;
			if (count == data->rank_mode) {
				count = 0;
				*pbRank0 = !(*pbRank0);
			}
		} else if(data->rank_mode < BYPASS_MODE) { /* production mode */
			p = mtk_fetch_page(data, order, rank);
			if (!p) {
				refill = 0;
				while (refill < data->szRefillTarget) {
					tmp = MTKAllocPage(data, gfp_mask, order, 0);
					if (tmp == 0) {
						dev_dbg(data->dev, "pool refill encounter OOM\n");
						break;
					}
					refill += (tmp << order);
				}
				dev_dbg(data->dev, "Refill %d-pool[%d]: (%d) / (%zu)\n", order, rank, refill, data->szRefillTarget);

				spin_lock(&data->MGMFree_lst_lk);
				if (*pbRank0) {// rank 0
					if (data->nr_rank[o][0] < (data->szSelectTarget >> order)) {
						*pbRank0 = !(*pbRank0);
						data->count++;
						dev_dbg(data->dev, "Select rank0->1 (%zu)\n", data->count);
					}
				} else {
					if (data->nr_rank[o][1] < (data->szSelectTarget >> order)) {
						*pbRank0 = !(*pbRank0);
						data->count++;
						dev_dbg(data->dev, "Select rank1->0 (%zu)\n", data->count);
					}
				}
				spin_unlock(&data->MGMFree_lst_lk);

				rank = (*pbRank0) ? 0 : 1;
				p = mtk_fetch_page(data, order, rank);
			}
		}
	}

	if (!p)
		p = alloc_pages(gfp_mask, order);
#else /* CONFIG_MALI_MTK_MGMM */
	p = alloc_pages(gfp_mask, order);
#endif /* CONFIG_MALI_MTK_MGMM */

	if (p) {
		update_size(mgm_dev, group_id, order, true);
	} else {
		struct mgm_groups *data = mgm_dev->data;

		dev_err(data->dev, "alloc_pages failed\n");
	}

	return p;
}
#if IS_ENABLED(CONFIG_MALI_MTK_MGMM)
static size_t mgmm_cache_pool_size(struct memory_group_manager_device *mgm_dev)
{
	struct mgm_groups *const data = mgm_dev->data;
	size_t ret;

	ret = data->nr_rank[0][0] + data->nr_rank[0][1];
	ret += ((data->nr_rank[1][0] + data->nr_rank[1][1]) << 9);

	return ret;
}
#endif

static void example_mgm_free_page(
	struct memory_group_manager_device *mgm_dev, int group_id,
	struct page *page, unsigned int order)
{
	struct mgm_groups *const data = mgm_dev->data;
#if IS_ENABLED(CONFIG_MALI_MTK_MGMM)
	int i;
#endif /* CONFIG_MALI_MTK_MGMM */
	dev_vdbg(data->dev, "%s(mgm_dev=%pK, group_id=%d page=%pK order=%u\n", __func__,
		(void *)mgm_dev, group_id, (void *)page, order);

	if (WARN_ON(group_id < 0) ||
		WARN_ON(group_id >= MEMORY_GROUP_MANAGER_NR_GROUPS))
		return;
#if IS_ENABLED(CONFIG_MALI_MTK_MGMM)
	if (data->rank_mode >= 0) {
		i = (page_to_phys(page) < data->ui64RankBoundary) ? 0 : 1; // true: rank0, false: rank1
		spin_lock(&data->MGMFree_lst_lk);
		if (order == 0) {
			if (data->nr_rank[0][i] < data->max_pool[0]) {
				clear_highpage(page);
				list_add(&page->lru, &data->free_list_r[0][i]);
				data->nr_rank[0][i]++;
			} else  {
				spin_unlock(&data->MGMFree_lst_lk);
				goto BUD_SYS;
			}
		} else if (order == 9) {
			if (data->nr_rank[1][i] < (data->max_pool[1] >> 9)) {
				clear_highpage(page);
				list_add(&page->lru, &data->free_list_r[1][i]);
				data->nr_rank[1][i]++;
			} else {
				spin_unlock(&data->MGMFree_lst_lk);
				goto BUD_SYS;
			}
		}
		/* If pool is not full, add records to cache memory */
		mod_node_page_state(page_pgdat(page), NR_KERNEL_MISC_RECLAIMABLE, 1 << order);
		spin_unlock(&data->MGMFree_lst_lk);
	} else {
BUD_SYS:
		__free_pages(page, order);
	}
#else /* CONFIG_MALI_MTK_MGMM */
	__free_pages(page, order);
#endif /* CONFIG_MALI_MTK_MGMM */

	update_size(mgm_dev, group_id, order, false);
}

static int example_mgm_get_import_memory_id(
	struct memory_group_manager_device *mgm_dev,
	struct memory_group_manager_import_data *import_data)
{
	struct mgm_groups *const data = mgm_dev->data;
#if IS_ENABLED(CONFIG_MALI_MTK_SLC_ALL_CACHE_MODE)
	int group_id = IMPORTED_MEMORY_ID;
	struct dma_buf *buf;
	int gid = 0;
#endif

	dev_vdbg(data->dev, "%s(mgm_dev=%pK, import_data=%pK (type=%d)\n", __func__, (void *)mgm_dev,
		(void *)import_data, (int)import_data->type);

	if (!WARN_ON(!import_data)) {
		WARN_ON(!import_data->u.dma_buf);

		WARN_ON(import_data->type !=
				MEMORY_GROUP_MANAGER_IMPORT_TYPE_DMA_BUF);
	}
#if IS_ENABLED(CONFIG_MALI_MTK_SLC_ALL_CACHE_MODE)
	buf = import_data->u.dma_buf;
	gid = dma_buf_get_gid(buf);
	if(gid == slbc_gid_val(ID_GPU)){
		group_id = GPU_ONLY_PBHA;
	}
	else if(gid == slbc_gid_val(ID_GPU_W)){
		group_id = GPU_TO_OVL_PBHA;
	}
	return group_id;
#endif
	return IMPORTED_MEMORY_ID;
}

static u64 example_mgm_update_gpu_pte(
	struct memory_group_manager_device *const mgm_dev, int const group_id,
	int const mmu_level, u64 pte)
{
	struct mgm_groups *const data = mgm_dev->data;

	dev_vdbg(data->dev, "%s(mgm_dev=%pK, group_id=%d, mmu_level=%d, pte=0x%llx)\n", __func__,
		(void *)mgm_dev, group_id, mmu_level, pte);

	if (WARN_ON(group_id < 0) ||
		WARN_ON(group_id >= MEMORY_GROUP_MANAGER_NR_GROUPS))
		return pte;

	pte |= ((u64)group_id << PTE_PBHA_SHIFT) & PTE_PBHA_MASK;

	/* Address could be translated into a different bus address here */
	pte |= ((u64)1 << PTE_RES_BIT_MULTI_AS_SHIFT);

	atomic_inc(&data->groups[group_id].update_gpu_pte);

	return pte;
}

static u64 example_mgm_pte_to_original_pte(struct memory_group_manager_device *const mgm_dev,
					   int const group_id, int const mmu_level, u64 pte)
{
	/* Undo the group ID modification */
	pte &= ~PTE_PBHA_MASK;
	/* Undo the bit set */
	pte &= ~((u64)1 << PTE_RES_BIT_MULTI_AS_SHIFT);

	return pte;
}

static vm_fault_t example_mgm_vmf_insert_pfn_prot(
	struct memory_group_manager_device *const mgm_dev, int const group_id,
	struct vm_area_struct *const vma, unsigned long const addr,
	unsigned long const pfn, pgprot_t const prot)
{
	struct mgm_groups *const data = mgm_dev->data;
	vm_fault_t fault;

	dev_vdbg(data->dev,
		"%s(mgm_dev=%pK, group_id=%d, vma=%pK, addr=0x%lx, pfn=0x%lx, prot=0x%llx)\n",
		__func__, (void *)mgm_dev, group_id, (void *)vma, addr, pfn,
		(unsigned long long)pgprot_val(prot));

	if (WARN_ON(group_id < 0) ||
		WARN_ON(group_id >= MEMORY_GROUP_MANAGER_NR_GROUPS))
		return VM_FAULT_SIGBUS;

	fault = vmf_insert_pfn_prot(vma, addr, pfn, prot);

	if (fault == VM_FAULT_NOPAGE)
		atomic_inc(&data->groups[group_id].insert_pfn);
	else
		dev_err(data->dev, "vmf_insert_pfn_prot failed\n");

	return fault;
}

static int mgm_initialize_data(struct mgm_groups *mgm_data)
{
	int i;

	for (i = 0; i < MEMORY_GROUP_MANAGER_NR_GROUPS; i++) {
		atomic_set(&mgm_data->groups[i].size, 0);
		atomic_set(&mgm_data->groups[i].lp_size, 0);
		atomic_set(&mgm_data->groups[i].insert_pfn, 0);
		atomic_set(&mgm_data->groups[i].update_gpu_pte, 0);
	}

	return mgm_initialize_debugfs(mgm_data);
}

static void mgm_term_data(struct mgm_groups *data)
{
	int i;

	for (i = 0; i < MEMORY_GROUP_MANAGER_NR_GROUPS; i++) {
		if (atomic_read(&data->groups[i].size) != 0)
			dev_warn(data->dev, "%d 0-order pages in group(%d) leaked\n",
				 atomic_read(&data->groups[i].size), i);
		if (atomic_read(&data->groups[i].lp_size) != 0)
			dev_warn(data->dev, "%d 9 order pages in group(%d) leaked\n",
				 atomic_read(&data->groups[i].lp_size), i);
	}

	mgm_term_debugfs(data);
}


static int memory_group_manager_probe(struct platform_device *pdev)
{
	struct memory_group_manager_device *mgm_dev;
	struct mgm_groups *mgm_data;
#if IS_ENABLED(CONFIG_MALI_MTK_MGMM)
	struct sysinfo info;
	size_t *nr_rank;
	size_t *nr_LP_rank;
#endif /* CONFIG_MALI_MTK_MGMM */
	mgm_dev = kzalloc(sizeof(*mgm_dev), GFP_KERNEL);
	if (!mgm_dev)
		return -ENOMEM;

	mgm_dev->owner = THIS_MODULE;
	mgm_dev->ops.mgm_alloc_page = example_mgm_alloc_page;
	mgm_dev->ops.mgm_free_page = example_mgm_free_page;
	mgm_dev->ops.mgm_get_import_memory_id =
			example_mgm_get_import_memory_id;
	mgm_dev->ops.mgm_vmf_insert_pfn_prot = example_mgm_vmf_insert_pfn_prot;
	mgm_dev->ops.mgm_update_gpu_pte = example_mgm_update_gpu_pte;
	mgm_dev->ops.mgm_pte_to_original_pte = example_mgm_pte_to_original_pte;
#if IS_ENABLED(CONFIG_MALI_MTK_MGMM)
	mgm_dev->ops.mgm_mtk_cache_pool_size = mgmm_cache_pool_size;
#endif

	mgm_data = kzalloc(sizeof(*mgm_data), GFP_KERNEL);
	if (!mgm_data) {
		kfree(mgm_dev);
		return -ENOMEM;
	}

	mgm_dev->data = mgm_data;
	mgm_data->dev = &pdev->dev;

	if (mgm_initialize_data(mgm_data)) {
		kfree(mgm_data);
		kfree(mgm_dev);
		return -ENOENT;
	}

	platform_set_drvdata(pdev, mgm_dev);
	dev_info(&pdev->dev, "Memory group manager probed successfully\n");

#if IS_ENABLED(CONFIG_MALI_MTK_MGMM)
	si_meminfo(&info);
	dev_info(&pdev->dev,"Total kmem: %zu (pages) [%d] %llx \n", info.totalram,  mtk_emicen_get_rk_cnt(), mtk_emicen_get_rk_size(0));
	spin_lock_init(&mgm_data->MGMFree_lst_lk);
	spin_lock_init(&mgm_data->free_4K_lst_lk);
	mgm_data->free_4K_lst.next = mgm_data->free_4K_lst.prev = &mgm_data->free_4K_lst;

	mgm_data->free_list_r[0][0].next = mgm_data->free_list_r[0][0].prev = &mgm_data->free_list_r[0][0];
	mgm_data->free_list_r[0][1].next = mgm_data->free_list_r[0][1].prev = &mgm_data->free_list_r[0][1];
	mgm_data->free_list_r[1][0].next = mgm_data->free_list_r[1][0].prev = &mgm_data->free_list_r[1][0];
	mgm_data->free_list_r[1][1].next = mgm_data->free_list_r[1][1].prev = &mgm_data->free_list_r[1][1];

	nr_rank = mgm_data->nr_rank[0];
	nr_LP_rank = mgm_data->nr_rank[1];
	nr_rank[0] = nr_rank[1] = 0;
	nr_LP_rank[0] = nr_LP_rank[1] = 0;

	mgm_data->bRank0[0] = mgm_data->bRank0[1] = true;
	mgm_data->max_pool[0] = mgm_data->max_pool[1] = RANK_POOL_LIMIT;
	mgm_data->count = 0;
	mgm_data->gfp_mask = GFP_HIGHUSER|__GFP_ZERO;
	mgm_data->reclaim.count_objects = mtk_mgm_pool_reclaim_count_objects;
	mgm_data->reclaim.scan_objects = mtk_mgm_pool_reclaim_scan_objects;
	mgm_data->reclaim.seeks = DEFAULT_SEEKS;
	mgm_data->reclaim.batch = 0;
#if (KERNEL_VERSION(6, 0, 0) > LINUX_VERSION_CODE)
	register_shrinker(&mgm_data->reclaim);
#else
	register_shrinker(&mgm_data->reclaim, "mali-mGMM");
#endif
	mgm_data->szSelectTarget = X_GUARD;
	/*
	 * Enable only in multiple rank env
	 */
	if (mtk_emicen_get_rk_cnt() == 2 &&
		(info.totalram << PAGE_SHIFT) > mtk_emicen_get_rk_size(0)) {
		mgm_data->ui64RankBoundary = MTK_EMI_DRAM_OFFSET + mtk_emicen_get_rk_size(0);
		mgm_data->rank_mode = RELAX_MODE;
		mgm_data->szRefillTarget = REFILL_TARGET;


		mtk_mgm_pool_fill(mgm_data, 9, 1, PREFILL_TARGET >> 9);
		mtk_mgm_pool_fill(mgm_data, 9, 0, PREFILL_TARGET >> 9);

		mtk_mgm_pool_trim(mgm_data, 9, 0, PREFILL_TARGET >> 9);
		mtk_mgm_pool_trim(mgm_data, 9, 1, PREFILL_TARGET >> 9);
	} else {
		mgm_data->ui64RankBoundary = MTK_EMI_DRAM_OFFSET;
		mgm_data->rank_mode = BYPASS_MODE;
	}

	dev_info(&pdev->dev,
		"mGMM init completed(%d): [0] {%zu, %zu}, [1] {%zu, %zu}\n",
		mgm_data->rank_mode, nr_rank[0], nr_rank[1], nr_LP_rank[0], nr_LP_rank[1]);

#endif	/* CONFIG_MALI_MTK_MGMM */
	return 0;
}

static int memory_group_manager_remove(struct platform_device *pdev)
{
	struct memory_group_manager_device *mgm_dev =
		platform_get_drvdata(pdev);
	struct mgm_groups *mgm_data = mgm_dev->data;

	mgm_term_data(mgm_data);
	kfree(mgm_data);

	kfree(mgm_dev);

	dev_info(&pdev->dev, "Memory group manager removed successfully\n");

	return 0;
}

static const struct of_device_id memory_group_manager_dt_ids[] = {
	{ .compatible = "arm,physical-memory-group-manager" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, memory_group_manager_dt_ids);

static struct platform_driver memory_group_manager_driver = {
	.probe = memory_group_manager_probe,
	.remove = memory_group_manager_remove,
	.driver = {
		.name = "physical-memory-group-manager",
		.of_match_table = of_match_ptr(memory_group_manager_dt_ids),
		/*
		 * Prevent the mgm_dev from being unbound and freed, as other's
		 * may have pointers to it and would get confused, or crash, if
		 * it suddenly disappear.
		 */
		.suppress_bind_attrs = true,
	}
};

module_platform_driver(memory_group_manager_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ARM Ltd.");
MODULE_VERSION("1.0");
