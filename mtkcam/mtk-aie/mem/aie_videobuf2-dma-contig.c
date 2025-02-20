// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/dma-buf.h>
#include <linux/module.h>
#include <linux/refcount.h>
#include <linux/scatterlist.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/version.h>

#include <media/videobuf2-v4l2.h>
#include "aie_videobuf2-dma-contig.h"
#include "aie_videobuf2-memops.h"

struct vb2_dc_buf {
	struct device			*dev;
	void				*vaddr;
	unsigned long			size;
	void				*cookie;
	dma_addr_t			dma_addr;
	unsigned long			attrs;
	enum dma_data_direction		dma_dir;
	struct sg_table			*dma_sgt;
	struct frame_vector		*vec;

	/* MMAP related */
	struct vb2_vmarea_handler	handler;
	refcount_t			refcount;
	struct sg_table			*sgt_base;

	/* DMABUF related */
	struct dma_buf_attachment	*db_attach;

	struct vb2_buffer		*vb;
};

/*********************************************/
/*        scatterlist table functions        */
/*********************************************/

unsigned long aie_vb2_dc_get_contiguous_size(struct sg_table *sgt)
{
	struct scatterlist *s;
	dma_addr_t expected = sg_dma_address(sgt->sgl);
	unsigned int i;
	unsigned long size = 0;

	for_each_sgtable_dma_sg(sgt, s, i) {
		if (sg_dma_address(s) != expected)
			break;
		expected += sg_dma_len(s);
		size += sg_dma_len(s);
	}
	return size;
}
EXPORT_SYMBOL_GPL(aie_vb2_dc_get_contiguous_size);
/*********************************************/
/*         callbacks for all buffers         */
/*********************************************/

static void *aie_vb2_dc_cookie(struct vb2_buffer *vb, void *buf_priv)
{
	struct vb2_dc_buf *buf = buf_priv;

	return &buf->dma_addr;
}

static void *aie_vb2_dc_vaddr(struct vb2_buffer *vb, void *buf_priv)
{
	struct vb2_dc_buf *buf = buf_priv;
	struct iosys_map map;
	int ret;

	if (!buf->vaddr && buf->db_attach) {
#ifdef AIE_DMA_BUF_UNLOCK_API
		ret = dma_buf_vmap_unlocked(buf->db_attach->dmabuf, &map);
#else
		ret = dma_buf_vmap(buf->db_attach->dmabuf, &map);
#endif
		buf->vaddr = ret ? NULL : map.vaddr;
	}

	return buf->vaddr;
}

static unsigned int aie_vb2_dc_num_users(void *buf_priv)
{
	struct vb2_dc_buf *buf = buf_priv;

	return refcount_read(&buf->refcount);
}

static void aie_vb2_dc_prepare(void *buf_priv)
{
	struct vb2_dc_buf *buf = buf_priv;
	struct sg_table *sgt = buf->dma_sgt;

	if (!sgt)
		return;

	dma_sync_sgtable_for_device(buf->dev, sgt, buf->dma_dir);
}

static void aie_vb2_dc_finish(void *buf_priv)
{
	struct vb2_dc_buf *buf = buf_priv;
	struct sg_table *sgt = buf->dma_sgt;

	if (!sgt)
		return;

	dma_sync_sgtable_for_cpu(buf->dev, sgt, buf->dma_dir);
}

/*********************************************/
/*        callbacks for MMAP buffers         */
/*********************************************/

static void aie_vb2_dc_put(void *buf_priv)
{
	struct vb2_dc_buf *buf = buf_priv;

	if (!refcount_dec_and_test(&buf->refcount))
		return;

	if (buf->sgt_base) {
		sg_free_table(buf->sgt_base);
		kfree(buf->sgt_base);
	}
	dma_free_attrs(buf->dev, buf->size, buf->cookie, buf->dma_addr,
		       buf->attrs);
	put_device(buf->dev);
	kfree(buf);
}

static void *aie_vb2_dc_alloc(struct vb2_buffer *vb,
			  struct device *dev,
			  unsigned long size)
{
	struct vb2_dc_buf *buf;

	if (WARN_ON(!dev))
		return ERR_PTR(-EINVAL);

	buf = kzalloc(sizeof(*buf), GFP_KERNEL);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	buf->attrs = vb->vb2_queue->dma_attrs;
	buf->cookie = dma_alloc_attrs(dev, size, &buf->dma_addr,
				      GFP_KERNEL | vb->vb2_queue->gfp_flags,
				      buf->attrs);
	if (!buf->cookie) {
		dev_info(dev, "dma_alloc_coherent of size %ld failed\n", size);
		kfree(buf);
		return ERR_PTR(-ENOMEM);
	}

	if ((buf->attrs & DMA_ATTR_NO_KERNEL_MAPPING) == 0)
		buf->vaddr = buf->cookie;

	/* Prevent the device from being released while the buffer is used */
	buf->dev = get_device(dev);
	buf->size = size;
	buf->dma_dir = vb->vb2_queue->dma_dir;

	buf->handler.refcount = &buf->refcount;
	buf->handler.put = aie_vb2_dc_put;
	buf->handler.arg = buf;
	buf->vb = vb;

	refcount_set(&buf->refcount, 1);

	return buf;
}

static int aie_vb2_dc_mmap(void *buf_priv, struct vm_area_struct *vma)
{
	struct vb2_dc_buf *buf = buf_priv;
	int ret;

	if (!buf) {
		pr_info("No buffer to map\n");
		return -EINVAL;
	}

	ret = dma_mmap_attrs(buf->dev, vma, buf->cookie,
		buf->dma_addr, buf->size, buf->attrs);

	if (ret) {
		pr_info("Remapping memory failed, error: %d\n", ret);
		return ret;
	}

	vm_flags_set(vma, VM_DONTEXPAND | VM_DONTDUMP);

	vma->vm_private_data	= &buf->handler;
	vma->vm_ops		= &aie_vb2_common_vm_ops;
	vma->vm_ops->open(vma);

	pr_debug("%s: mapped dma addr 0x%08lx at 0x%08lx, size %ld\n",
		__func__, (unsigned long)buf->dma_addr, vma->vm_start,
		buf->size);

	return 0;
}

/*********************************************/
/*         DMABUF ops for exporters          */
/*********************************************/

struct vb2_dc_attachment {
	struct sg_table sgt;
	enum dma_data_direction dma_dir;
};

static int aie_vb2_dc_dmabuf_ops_attach(struct dma_buf *dbuf,
	struct dma_buf_attachment *dbuf_attach)
{
	struct vb2_dc_attachment *attach;
	unsigned int i;
	struct scatterlist *rd, *wr;
	struct sg_table *sgt;
	struct vb2_dc_buf *buf = dbuf->priv;
	int ret;

	attach = kzalloc(sizeof(*attach), GFP_KERNEL);
	if (!attach)
		return -ENOMEM;

	sgt = &attach->sgt;
	/* Copy the buf->base_sgt scatter list to the attachment, as we can't
	 * map the same scatter list to multiple attachments at the same time.
	 */
	ret = sg_alloc_table(sgt, buf->sgt_base->orig_nents, GFP_KERNEL);
	if (ret) {
		kfree(attach);
		return -ENOMEM;
	}

	rd = buf->sgt_base->sgl;
	wr = sgt->sgl;
	for (i = 0; i < sgt->orig_nents; ++i) {
		sg_set_page(wr, sg_page(rd), rd->length, rd->offset);
		rd = sg_next(rd);
		wr = sg_next(wr);
	}

	attach->dma_dir = DMA_NONE;
	dbuf_attach->priv = attach;

	return 0;
}

static void aie_vb2_dc_dmabuf_ops_detach(struct dma_buf *dbuf,
	struct dma_buf_attachment *db_attach)
{
	struct vb2_dc_attachment *attach = db_attach->priv;
	struct sg_table *sgt;

	if (!attach)
		return;

	sgt = &attach->sgt;

	/* release the scatterlist cache */
	if (attach->dma_dir != DMA_NONE)
		/*
		 * Cache sync can be skipped here, as the vb2_dc memory is
		 * allocated from device coherent memory, which means the
		 * memory locations do not require any explicit cache
		 * maintenance prior or after being used by the device.
		 */
		dma_unmap_sgtable(db_attach->dev, sgt, attach->dma_dir,
				  DMA_ATTR_SKIP_CPU_SYNC);
	sg_free_table(sgt);
	kfree(attach);
	db_attach->priv = NULL;
}

static struct sg_table *aie_vb2_dc_dmabuf_ops_map(
	struct dma_buf_attachment *db_attach, enum dma_data_direction dma_dir)
{
	struct vb2_dc_attachment *attach = db_attach->priv;
	struct sg_table *sgt;

	sgt = &attach->sgt;
	/* return previously mapped sg table */
	if (attach->dma_dir == dma_dir)
		return sgt;

	/* release any previous cache */
	if (attach->dma_dir != DMA_NONE) {
		dma_unmap_sgtable(db_attach->dev, sgt, attach->dma_dir,
				  DMA_ATTR_SKIP_CPU_SYNC);
		attach->dma_dir = DMA_NONE;
	}

	/*
	 * mapping to the client with new direction, no cache sync
	 * required see comment in vb2_dc_dmabuf_ops_detach()
	 */
	if (dma_map_sgtable(db_attach->dev, sgt, dma_dir,
			    DMA_ATTR_SKIP_CPU_SYNC)) {
		pr_info("failed to map scatterlist\n");
		return ERR_PTR(-EIO);
	}

	attach->dma_dir = dma_dir;

	return sgt;
}

static void aie_vb2_dc_dmabuf_ops_unmap(struct dma_buf_attachment *db_attach,
	struct sg_table *sgt, enum dma_data_direction dma_dir)
{
	/* nothing to be done here */
}

static void aie_vb2_dc_dmabuf_ops_release(struct dma_buf *dbuf)
{
	/* drop reference obtained in vb2_dc_get_dmabuf */
	aie_vb2_dc_put(dbuf->priv);
}

static int
aie_vb2_dc_dmabuf_ops_begin_cpu_access(struct dma_buf *dbuf,
				   enum dma_data_direction direction)
{
	return 0;
}

static int
aie_vb2_dc_dmabuf_ops_end_cpu_access(struct dma_buf *dbuf,
				 enum dma_data_direction direction)
{
	return 0;
}

static int aie_vb2_dc_dmabuf_ops_vmap(struct dma_buf *dbuf, struct iosys_map *map)
{
	struct vb2_dc_buf *buf = dbuf->priv;

	iosys_map_set_vaddr(map, buf->vaddr);

	return 0;
}

static int aie_vb2_dc_dmabuf_ops_mmap(struct dma_buf *dbuf,
	struct vm_area_struct *vma)
{
	return aie_vb2_dc_mmap(dbuf->priv, vma);
}

static const struct dma_buf_ops aie_vb2_dc_dmabuf_ops = {
	.attach = aie_vb2_dc_dmabuf_ops_attach,
	.detach = aie_vb2_dc_dmabuf_ops_detach,
	.map_dma_buf = aie_vb2_dc_dmabuf_ops_map,
	.unmap_dma_buf = aie_vb2_dc_dmabuf_ops_unmap,
	.begin_cpu_access = aie_vb2_dc_dmabuf_ops_begin_cpu_access,
	.end_cpu_access = aie_vb2_dc_dmabuf_ops_end_cpu_access,
	.vmap = aie_vb2_dc_dmabuf_ops_vmap,
	.mmap = aie_vb2_dc_dmabuf_ops_mmap,
	.release = aie_vb2_dc_dmabuf_ops_release,
};

static struct sg_table *aie_vb2_dc_get_base_sgt(struct vb2_dc_buf *buf)
{
	int ret;
	struct sg_table *sgt;

	sgt = kmalloc(sizeof(*sgt), GFP_KERNEL);
	if (!sgt)
		return NULL;

	ret = dma_get_sgtable_attrs(buf->dev, sgt, buf->cookie, buf->dma_addr,
		buf->size, buf->attrs);
	if (ret < 0) {
		dev_info(buf->dev, "failed to get scatterlist from DMA API\n");
		kfree(sgt);
		return NULL;
	}

	return sgt;
}

static struct dma_buf *aie_vb2_dc_get_dmabuf(struct vb2_buffer *vb,
					 void *buf_priv,
					 unsigned long flags)
{
	struct vb2_dc_buf *buf = buf_priv;
	struct dma_buf *dbuf;
	DEFINE_DMA_BUF_EXPORT_INFO(exp_info);

	exp_info.ops = &aie_vb2_dc_dmabuf_ops;
	exp_info.size = buf->size;
	exp_info.flags = flags;
	exp_info.priv = buf;

	if (!buf->sgt_base)
		buf->sgt_base = aie_vb2_dc_get_base_sgt(buf);

	if (WARN_ON(!buf->sgt_base))
		return NULL;

	dbuf = dma_buf_export(&exp_info);
	if (IS_ERR(dbuf))
		return NULL;

	/* dmabuf keeps reference to vb2 buffer */
	refcount_inc(&buf->refcount);

	return dbuf;
}

/*********************************************/
/*       callbacks for USERPTR buffers       */
/*********************************************/

static void aie_vb2_dc_put_userptr(void *buf_priv)
{
	struct vb2_dc_buf *buf = buf_priv;
	struct sg_table *sgt = buf->dma_sgt;
	int i;
	struct page **pages;

	if (sgt) {
		/*
		 * No need to sync to CPU, it's already synced to the CPU
		 * since the finish() memop will have been called before this.
		 */
		dma_unmap_sgtable(buf->dev, sgt, buf->dma_dir,
				  DMA_ATTR_SKIP_CPU_SYNC);
		pages = frame_vector_pages(buf->vec);
		/* sgt should exist only if vector contains pages... */
		WARN_ON(IS_ERR(pages));
		if (buf->dma_dir == DMA_FROM_DEVICE ||
		    buf->dma_dir == DMA_BIDIRECTIONAL)
			for (i = 0; i < frame_vector_count(buf->vec); i++)
				set_page_dirty_lock(pages[i]);
		sg_free_table(sgt);
		kfree(sgt);
	} else {
		dma_unmap_resource(buf->dev, buf->dma_addr, buf->size,
				   buf->dma_dir, 0);
	}
	aie_vb2_destroy_framevec(buf->vec);
	kfree(buf);
}

static void *aie_vb2_dc_get_userptr(struct vb2_buffer *vb, struct device *dev,
				unsigned long vaddr, unsigned long size)
{
	struct vb2_dc_buf *buf;
	struct frame_vector *vec;
	unsigned int offset;
	int n_pages, i;
	int ret = 0;
	struct sg_table *sgt;
	unsigned long contig_size;
	unsigned long dma_align = dma_get_cache_alignment();

	/* Only cache aligned DMA transfers are reliable */
	if (!IS_ALIGNED(vaddr | size, dma_align)) {
		pr_debug("user data must be aligned to %lu bytes\n", dma_align);
		return ERR_PTR(-EINVAL);
	}

	if (!size) {
		pr_debug("size is zero\n");
		return ERR_PTR(-EINVAL);
	}

	if (WARN_ON(!dev))
		return ERR_PTR(-EINVAL);

	buf = kzalloc(sizeof(*buf), GFP_KERNEL);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	buf->dev = dev;
	buf->dma_dir = vb->vb2_queue->dma_dir;
	buf->vb = vb;

	offset = lower_32_bits(offset_in_page(vaddr));
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 2, 0)
	vec = aie_vb2_create_framevec(vaddr, size, buf->dma_dir == DMA_FROM_DEVICE ||
	                                           buf->dma_dir == DMA_BIDIRECTIONAL);
#else
	vec = aie_vb2_create_framevec(vaddr, size);
#endif
	if (IS_ERR(vec)) {
		ret = PTR_ERR(vec);
		goto fail_buf;
	}
	buf->vec = vec;
	n_pages = frame_vector_count(vec);
	ret = frame_vector_to_pages(vec);
	if (ret < 0) {
		unsigned long *nums = frame_vector_pfns(vec);

		/*
		 * Failed to convert to pages... Check the memory is physically
		 * contiguous and use direct mapping
		 */
		for (i = 1; i < n_pages; i++)
			if (nums[i-1] + 1 != nums[i])
				goto fail_pfnvec;
		buf->dma_addr = dma_map_resource(buf->dev,
				__pfn_to_phys(nums[0]), size, buf->dma_dir, 0);
		if (dma_mapping_error(buf->dev, buf->dma_addr)) {
			ret = -ENOMEM;
			goto fail_pfnvec;
		}
		goto out;
	}

	sgt = kzalloc(sizeof(*sgt), GFP_KERNEL);
	if (!sgt) {
		ret = -ENOMEM;
		goto fail_pfnvec;
	}

	ret = sg_alloc_table_from_pages(sgt, frame_vector_pages(vec), n_pages,
		offset, size, GFP_KERNEL);
	if (ret) {
		pr_info("failed to initialize sg table\n");
		goto fail_sgt;
	}

	/*
	 * No need to sync to the device, this will happen later when the
	 * prepare() memop is called.
	 */
	if (dma_map_sgtable(buf->dev, sgt, buf->dma_dir,
			    DMA_ATTR_SKIP_CPU_SYNC)) {
		pr_info("failed to map scatterlist\n");
		ret = -EIO;
		goto fail_sgt_init;
	}

	contig_size = aie_vb2_dc_get_contiguous_size(sgt);
	if (contig_size < size) {
		pr_info("contiguous mapping is too small %lu/%lu\n",
			contig_size, size);
		ret = -EFAULT;
		goto fail_map_sg;
	}

	buf->dma_addr = sg_dma_address(sgt->sgl);
	buf->dma_sgt = sgt;
out:
	buf->size = size;

	return buf;

fail_map_sg:
	dma_unmap_sgtable(buf->dev, sgt, buf->dma_dir, DMA_ATTR_SKIP_CPU_SYNC);

fail_sgt_init:
	sg_free_table(sgt);

fail_sgt:
	kfree(sgt);

fail_pfnvec:
	aie_vb2_destroy_framevec(vec);

fail_buf:
	kfree(buf);

	return ERR_PTR(ret);
}

/*********************************************/
/*       callbacks for DMABUF buffers        */
/*********************************************/
static int aie_vb2_dc_map_dmabuf(void *mem_priv)
{
	struct vb2_dc_buf *buf = mem_priv;
	struct sg_table *sgt;
	unsigned long contig_size;

	if (WARN_ON(!buf->db_attach)) {
		pr_info("trying to pin a non attached buffer\n");
		return -EINVAL;
	}

	if (WARN_ON(buf->dma_sgt)) {
		pr_info("dmabuf buffer is already pinned\n");
		return 0;
	}
	buf->db_attach->dma_map_attrs |= DMA_ATTR_SKIP_CPU_SYNC;
	/* get the associated scatterlist for this buffer */
#ifdef AIE_DMA_BUF_UNLOCK_API
	sgt = dma_buf_map_attachment_unlocked(buf->db_attach, buf->dma_dir);
#else
	sgt = dma_buf_map_attachment(buf->db_attach, buf->dma_dir);
#endif
	if (IS_ERR(sgt)) {
		pr_info("Error getting dmabuf scatterlist\n");
		return -EINVAL;
	}

	/* checking if dmabuf is big enough to store contiguous chunk */
	contig_size = aie_vb2_dc_get_contiguous_size(sgt);
	if (contig_size < buf->size) {
		pr_info("contiguous chunk is too small %lu/%lu\n",
		       contig_size, buf->size);
#ifdef AIE_DMA_BUF_UNLOCK_API
		dma_buf_unmap_attachment_unlocked(buf->db_attach, sgt, buf->dma_dir);
#else
		dma_buf_unmap_attachment(buf->db_attach, sgt, buf->dma_dir);
#endif
		return -EFAULT;
	}

	buf->dma_addr = sg_dma_address(sgt->sgl);
	buf->dma_sgt = sgt;
	buf->vaddr = NULL;

	return 0;
}

static void aie_vb2_dc_unmap_dmabuf(void *mem_priv)
{
	struct vb2_dc_buf *buf = mem_priv;
	struct sg_table *sgt = buf->dma_sgt;
	struct iosys_map map = IOSYS_MAP_INIT_VADDR(buf->vaddr);

	if (WARN_ON(!buf->db_attach)) {
		pr_info("trying to unpin a not attached buffer\n");
		return;
	}

	if (WARN_ON(!sgt)) {
		pr_info("dmabuf buffer is already unpinned\n");
		return;
	}

	if (buf->vaddr) {
#ifdef AIE_DMA_BUF_UNLOCK_API
		dma_buf_vunmap_unlocked(buf->db_attach->dmabuf, &map);
#else
		dma_buf_vunmap(buf->db_attach->dmabuf, &map);
#endif
		buf->vaddr = NULL;
	}
#ifdef AIE_DMA_BUF_UNLOCK_API
	dma_buf_unmap_attachment_unlocked(buf->db_attach, sgt, buf->dma_dir);
#else
	dma_buf_unmap_attachment(buf->db_attach, sgt, buf->dma_dir);
#endif
	buf->dma_addr = 0;
	buf->dma_sgt = NULL;
}

static void aie_vb2_dc_detach_dmabuf(void *mem_priv)
{
	struct vb2_dc_buf *buf = mem_priv;

	/* if vb2 works correctly you should never detach mapped buffer */
	if (WARN_ON(buf->dma_addr))
		aie_vb2_dc_unmap_dmabuf(buf);

	/* detach this attachment */
	dma_buf_detach(buf->db_attach->dmabuf, buf->db_attach);
	kfree(buf);
}

static void *aie_vb2_dc_attach_dmabuf(struct vb2_buffer *vb, struct device *dev,
				  struct dma_buf *dbuf, unsigned long size)
{
	struct vb2_dc_buf *buf;
	struct dma_buf_attachment *dba;

	if (dbuf->size < size)
		return ERR_PTR(-EFAULT);

	if (WARN_ON(!dev))
		return ERR_PTR(-EINVAL);

	buf = kzalloc(sizeof(*buf), GFP_KERNEL);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	buf->dev = dev;
	buf->vb = vb;

	/* create attachment for the dmabuf with the user device */
	dba = dma_buf_attach(dbuf, buf->dev);
	if (IS_ERR(dba)) {
		pr_info("failed to attach dmabuf\n");
		kfree(buf);
		return dba;
	}

	buf->dma_dir = vb->vb2_queue->dma_dir;
	buf->size = size;
	buf->db_attach = dba;

	return buf;
}

/*********************************************/
/*       DMA CONTIG exported functions       */
/*********************************************/

const struct vb2_mem_ops aie_vb2_dma_contig_memops = {
	.alloc		= aie_vb2_dc_alloc,
	.put		= aie_vb2_dc_put,
	.get_dmabuf	= aie_vb2_dc_get_dmabuf,
	.cookie		= aie_vb2_dc_cookie,
	.vaddr		= aie_vb2_dc_vaddr,
	.mmap		= aie_vb2_dc_mmap,
	.get_userptr	= aie_vb2_dc_get_userptr,
	.put_userptr	= aie_vb2_dc_put_userptr,
	.prepare	= aie_vb2_dc_prepare,
	.finish		= aie_vb2_dc_finish,
	.map_dmabuf	= aie_vb2_dc_map_dmabuf,
	.unmap_dmabuf	= aie_vb2_dc_unmap_dmabuf,
	.attach_dmabuf	= aie_vb2_dc_attach_dmabuf,
	.detach_dmabuf	= aie_vb2_dc_detach_dmabuf,
	.num_users	= aie_vb2_dc_num_users,
};
EXPORT_SYMBOL_GPL(aie_vb2_dma_contig_memops);

/**
 * vb2_dma_contig_set_max_seg_size() - configure DMA max segment size
 * @dev:	device for configuring DMA parameters
 * @size:	size of DMA max segment size to set
 *
 * To allow mapping the scatter-list into a single chunk in the DMA
 * address space, the device is required to have the DMA max segment
 * size parameter set to a value larger than the buffer size. Otherwise,
 * the DMA-mapping subsystem will split the mapping into max segment
 * size chunks. This function sets the DMA max segment size
 * parameter to let DMA-mapping map a buffer as a single chunk in DMA
 * address space.
 * This code assumes that the DMA-mapping subsystem will merge all
 * scatterlist segments if this is really possible (for example when
 * an IOMMU is available and enabled).
 * Ideally, this parameter should be set by the generic bus code, but it
 * is left with the default 64KiB value due to historical litmiations in
 * other subsystems (like limited USB host drivers) and there no good
 * place to set it to the proper value.
 * This function should be called from the drivers, which are known to
 * operate on platforms with IOMMU and provide access to shared buffers
 * (either USERPTR or DMABUF). This should be done before initializing
 * videobuf2 queue.
 */
int aie_vb2_dma_contig_set_max_seg_size(struct device *dev, unsigned int size)
{
	if (!dev->dma_parms) {
		dev_info(dev, "Failed to set max_seg_size: dma_parms is NULL\n");
		return -ENODEV;
	}
	if (dma_get_max_seg_size(dev) < size)
		return dma_set_max_seg_size(dev, size);

	return 0;
}
EXPORT_SYMBOL_GPL(aie_vb2_dma_contig_set_max_seg_size);

MODULE_DESCRIPTION("AIE DMA-contig memory handling routines for videobuf2");
MODULE_IMPORT_NS(DMA_BUF);
MODULE_LICENSE("GPL");
