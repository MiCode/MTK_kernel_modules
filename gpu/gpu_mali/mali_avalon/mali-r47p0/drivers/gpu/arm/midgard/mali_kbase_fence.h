/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 *
 * (C) COPYRIGHT 2010-2023 ARM Limited. All rights reserved.
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

#ifndef _KBASE_FENCE_H_
#define _KBASE_FENCE_H_

/*
 * mali_kbase_fence.[hc] has fence code used only by
 * - CONFIG_SYNC_FILE      - explicit fences
 */

#if IS_ENABLED(CONFIG_SYNC_FILE)

#include "mali_kbase.h"

#include <linux/list.h>
#include <linux/version_compat_defs.h>

/* Maximum number of characters in DMA fence timeline name. */
#define MAX_TIMELINE_NAME (32)

/**
 * struct kbase_kcpu_dma_fence_meta - Metadata structure for dma fence objects containing
 *                                    information about KCPU queue. One instance per KCPU
 *                                    queue.
 *
 * @refcount:       Atomic value to keep track of number of references to an instance.
 *                  An instance can outlive the KCPU queue itself.
 * @kbdev:          Pointer to Kbase device.
 * @kctx_id:        Kbase context ID.
 * @timeline_name:  String of timeline name for associated fence object.
 */
struct kbase_kcpu_dma_fence_meta {
	kbase_refcount_t refcount;
	struct kbase_device *kbdev;
	u32 kctx_id;
	char timeline_name[MAX_TIMELINE_NAME];
};

/**
 * struct kbase_kcpu_dma_fence - Structure which extends a dma fence object to include a
 *                               reference to metadata containing more informaiton about it.
 *
 * @base:      Fence object itself.
 * @metadata:  Pointer to metadata structure.
 */
struct kbase_kcpu_dma_fence {
	struct dma_fence base;
	struct kbase_kcpu_dma_fence_meta *metadata;
};

extern const struct dma_fence_ops kbase_fence_ops;

/**
 * kbase_fence_out_new() - Creates a new output fence and puts it on the atom
 * @katom: Atom to create an output fence for
 *
 * Return: A new fence object on success, NULL on failure.
 */
struct dma_fence *kbase_fence_out_new(struct kbase_jd_atom *katom);

#if IS_ENABLED(CONFIG_SYNC_FILE)
/**
 * kbase_fence_fence_in_set() - Assign input fence to atom
 * @katom: Atom to assign input fence to
 * @fence: Input fence to assign to atom
 *
 * This function will take ownership of one fence reference!
 */
#define kbase_fence_fence_in_set(katom, fence)        \
	do {                                          \
		WARN_ON((katom)->dma_fence.fence_in); \
		(katom)->dma_fence.fence_in = fence;  \
	} while (0)
#endif


/**
 * kbase_fence_get() - Retrieve fence for a KCPUQ fence command.
 * @fence_info: KCPUQ fence command
 *
 * A ref will be taken for the fence, so use @kbase_fence_put() to release it
 *
 * Return: The fence, or NULL if there is no fence for KCPUQ fence command
 */
#define kbase_fence_get(fence_info) dma_fence_get((fence_info)->fence)

static inline struct kbase_kcpu_dma_fence *kbase_kcpu_dma_fence_get(struct dma_fence *fence)
{
	if (fence->ops == &kbase_fence_ops)
		return (struct kbase_kcpu_dma_fence *)fence;

	return NULL;
}

static inline void kbase_kcpu_dma_fence_meta_put(struct kbase_kcpu_dma_fence_meta *metadata)
{
	if (kbase_refcount_dec_and_test(&metadata->refcount)) {
		atomic_dec(&metadata->kbdev->live_fence_metadata);
		kfree(metadata);
	}
}

static inline void kbase_kcpu_dma_fence_put(struct dma_fence *fence)
{
	struct kbase_kcpu_dma_fence *kcpu_fence = kbase_kcpu_dma_fence_get(fence);

	if (kcpu_fence)
		kbase_kcpu_dma_fence_meta_put(kcpu_fence->metadata);
}

/**
 * kbase_fence_put() - Releases a reference to a fence
 * @fence: Fence to release reference for.
 */
static inline void kbase_fence_put(struct dma_fence *fence)
{
	dma_fence_put(fence);
}

#endif /* IS_ENABLED(CONFIG_SYNC_FILE) */

#endif /* _KBASE_FENCE_H_ */
