// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/slab.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/file.h>

#include <media/videobuf2-v4l2.h>
#include "aie_videobuf2-memops.h"

/*#define WORKAROUND*/

/**
 * vb2_create_framevec() - map virtual addresses to pfns
 * @start:	Virtual user address where we start mapping
 * @length:	Length of a range to map
 * @write:	Should we map for writing into the area
 *
 * This function allocates and fills in a vector with pfns corresponding to
 * virtual address range passed in arguments. If pfns have corresponding pages,
 * page references are also grabbed to pin pages in memory. The function
 * returns pointer to the vector on success and error pointer in case of
 * failure. Returned vector needs to be freed via vb2_destroy_pfnvec().
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 2, 0)
struct frame_vector *aie_vb2_create_framevec(unsigned long start,
					 unsigned long length,
					 bool write)
#else
struct frame_vector *aie_vb2_create_framevec(unsigned long start,
					 unsigned long length)
#endif
{
	int ret;
	unsigned long first, last;
	unsigned long nr;
	struct frame_vector *vec;

	first = start >> PAGE_SHIFT;
	last = (start + length - 1) >> PAGE_SHIFT;
	nr = last - first + 1;
	vec = frame_vector_create(nr);
	if (!vec)
		return ERR_PTR(-ENOMEM);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 2, 0)
	ret = get_vaddr_frames(start & PAGE_MASK, nr, write, vec);
#else
	ret = get_vaddr_frames(start & PAGE_MASK, nr, vec);
#endif
	if (ret < 0)
		goto out_destroy;
	/* We accept only complete set of PFNs */
	if (ret != nr) {
		ret = -EFAULT;
		goto out_release;
	}
	return vec;
out_release:
	put_vaddr_frames(vec);
out_destroy:
	frame_vector_destroy(vec);
	return ERR_PTR(ret);
}
EXPORT_SYMBOL(aie_vb2_create_framevec);

/**
 * vb2_destroy_framevec() - release vector of mapped pfns
 * @vec:	vector of pfns / pages to release
 *
 * This releases references to all pages in the vector @vec (if corresponding
 * pfns are backed by pages) and frees the passed vector.
 */
void aie_vb2_destroy_framevec(struct frame_vector *vec)
{
	put_vaddr_frames(vec);
	frame_vector_destroy(vec);
}
EXPORT_SYMBOL(aie_vb2_destroy_framevec);

/**
 * vb2_common_vm_open() - increase refcount of the vma
 * @vma:	virtual memory region for the mapping
 *
 * This function adds another user to the provided vma. It expects
 * struct vb2_vmarea_handler pointer in vma->vm_private_data.
 */
static void aie_vb2_common_vm_open(struct vm_area_struct *vma)
{
	struct vb2_vmarea_handler *h = vma->vm_private_data;

	pr_debug("%s: %p, refcount: %d, vma: %08lx-%08lx\n",
	       __func__, h, refcount_read(h->refcount), vma->vm_start,
	       vma->vm_end);

	refcount_inc(h->refcount);
}

/**
 * vb2_common_vm_close() - decrease refcount of the vma
 * @vma:	virtual memory region for the mapping
 *
 * This function releases the user from the provided vma. It expects
 * struct vb2_vmarea_handler pointer in vma->vm_private_data.
 */
static void aie_vb2_common_vm_close(struct vm_area_struct *vma)
{
	struct vb2_vmarea_handler *h = vma->vm_private_data;

	pr_debug("%s: %p, refcount: %d, vma: %08lx-%08lx\n",
	       __func__, h, refcount_read(h->refcount), vma->vm_start,
	       vma->vm_end);

	h->put(h->arg);
}

/*
 * vb2_common_vm_ops - common vm_ops used for tracking refcount of mmapped
 * video buffers
 */
const struct vm_operations_struct aie_vb2_common_vm_ops = {
	.open = aie_vb2_common_vm_open,
	.close = aie_vb2_common_vm_close,
};
EXPORT_SYMBOL_GPL(aie_vb2_common_vm_ops);

MODULE_DESCRIPTION("AIE specific memory handling routines for aie_videobuf2");
MODULE_LICENSE("GPL");
