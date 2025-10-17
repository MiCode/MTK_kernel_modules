/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2018 MediaTek Inc.
 */

#ifndef __MTK_CAM_POOL_H
#define __MTK_CAM_POOL_H

#include <linux/dma-buf.h>

struct dma_buf;
struct dma_buf *mtk_cam_cached_buffer_alloc(size_t size);
struct dma_buf *mtk_cam_noncached_buffer_alloc(size_t size);

struct mtk_cam_device_buf {
	struct dma_buf *dbuf;
	size_t size;
	struct dma_buf_attachment *db_attach;
	struct sg_table *dma_sgt;

	dma_addr_t daddr;
	void *vaddr;
};

/* attach + map_attachment + get_dma_buf */
int mtk_cam_device_buf_init(struct mtk_cam_device_buf *buf,
			    struct dma_buf *dbuf,
			    struct device *dev,
			    size_t expected_size);

/* detach + unmap_attachment + dma_buf_put */
void mtk_cam_device_buf_uninit(struct mtk_cam_device_buf *buf);
int mtk_cam_device_buf_vmap(struct mtk_cam_device_buf *buf);
static inline int mtk_cam_device_buf_fd(struct mtk_cam_device_buf *buf)
{
	get_dma_buf(buf->dbuf);
	return dma_buf_fd(buf->dbuf, O_RDWR | O_CLOEXEC);
}

struct mtk_cam_pool_priv {
	struct mtk_cam_pool *pool;
	int index : 6;
	int available : 2;
};

struct mtk_cam_pool {
	size_t element_size;
	size_t n_element;
	void *elements;

	spinlock_t lock;
	int fetch_idx;
	int available_cnt;
};

/**
 * Must be created and managed through
 * mtk_cam_pool_wrapper_[create/get/put] functions
 */
struct mtk_cam_pool_wrapper {
	struct kref refcount;
	struct mtk_cam_pool pool;
	struct mtk_cam_device_buf mem;
};

struct mtk_cam_device_refcnt_buf {
	struct kref refcount;
	struct mtk_cam_device_buf buf;
};


/* alloc/destroy */
int mtk_cam_pool_alloc(struct mtk_cam_pool *pool,
		       size_t element_size, int n_element);
void mtk_cam_pool_destroy(struct mtk_cam_pool *pool);

/* config each element */
typedef void (*fn_config_element)(void *data, int index, void *element);
int mtk_cam_pool_config(struct mtk_cam_pool *pool,
			fn_config_element fn, void *data);

static inline void mtk_cam_pool_priv_init(struct mtk_cam_pool_priv *priv)
{
	priv->pool = NULL;
}

static inline bool mtk_cam_pool_priv_is_valid(struct mtk_cam_pool_priv *priv)
{
	return priv->pool != NULL;
}

int mtk_cam_pool_fetch(struct mtk_cam_pool *pool,
		       void *buf, size_t size);
void mtk_cam_pool_return(void *buf, size_t size);

int mtk_cam_pool_available_cnt(struct mtk_cam_pool *pool);

/* a wrapper to divide buffer into chunks as buffer pool */
struct mtk_cam_pool_buffer {
	struct mtk_cam_pool_priv priv;

	dma_addr_t daddr;
	void *vaddr;
	int size;
};

/* with built-in config func */
int mtk_cam_buffer_pool_alloc(struct mtk_cam_pool *pool,
			      struct mtk_cam_device_buf *buf, int n_buffers);

int mtk_cam_buffer_pool_realloc(struct mtk_cam_pool *pool,
			      struct mtk_cam_device_buf *buf, int total_size, int n_buffers);

static inline void mtk_cam_pool_buffer_init(struct mtk_cam_pool_buffer *buf)
{
	mtk_cam_pool_priv_init(&buf->priv);
}

static inline bool mtk_cam_pool_buffer_is_valid(struct mtk_cam_pool_buffer *buf)
{
	return mtk_cam_pool_priv_is_valid(&buf->priv);
}

static inline int mtk_cam_buffer_pool_fetch(struct mtk_cam_pool *pool,
					    struct mtk_cam_pool_buffer *buf)
{
	return mtk_cam_pool_fetch(pool, buf, sizeof(*buf));
}

static inline void mtk_cam_buffer_pool_return(struct mtk_cam_pool_buffer *buf)
{
	mtk_cam_pool_return(buf, sizeof(*buf));
}

#endif //__MTK_CAM_POOL_H
