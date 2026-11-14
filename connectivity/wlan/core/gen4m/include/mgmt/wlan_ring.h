/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _WLAN_RING_H_
#define _WLAN_RING_H_

struct wlan_ring {
	/* addr where ring buffer starts */
	void *base;
	/* addr storing the next writable pos, guaranteed to be >= read except when write overflow, but it's ok. */
	unsigned int write;
	/* addr storing the next readable pos, except when read == write as buffer empty */
	unsigned int read;
	/* must be power of 2 */
	unsigned int max_size;
};

struct wlan_ring_segment {
	/* addr points into ring buffer for read/write */
	void *ring_pt;
	/* size to read/write */
	unsigned int sz;
	/* pos in external data buffer to read/write */
	unsigned int data_pos;
	/* the size to be read/write after this segment completed */
	unsigned int remain;
};

void wlan_ring_init(void *base, unsigned int max_size, unsigned int read,
	unsigned int write, struct wlan_ring *ring);
unsigned int wlan_ring_read_prepare(unsigned int sz, struct wlan_ring_segment *seg, struct wlan_ring *ring);
#define wlan_ring_read_all_prepare(seg, ring)  wlan_ring_read_prepare((ring)->max_size, seg, ring)
unsigned int wlan_ring_write_prepare(unsigned int sz, struct wlan_ring_segment *seg, struct wlan_ring *ring);
unsigned int wlan_ring_overwrite_prepare(unsigned int sz,
	struct wlan_ring_segment *seg, struct wlan_ring *ring);

#define WLAN_RING_INIT(...) wlan_ring_init(__VA_ARGS__)
#define WLAN_RING_READ_PREPARE(...) wlan_ring_read_prepare(__VA_ARGS__)
/* making sure max_size is power of 2 */
#define WLAN_RING_VALIDATE_SIZE(max_size) \
	KAL_WARN_ON(!max_size || (max_size & (max_size - 1)))
#define WLAN_RING_EMPTY(ring) ((ring)->read == (ring)->write)
/* equation works even when write overflow */
#define WLAN_RING_SIZE(ring) ((ring)->write - (ring)->read)
#define WLAN_RING_FULL(ring) (WLAN_RING_SIZE(ring) == (ring)->max_size)
#define WLAN_RING_WRITE_REMAIN_SIZE(ring) ((ring)->max_size - WLAN_RING_SIZE(ring))

#define WLAN_RING_READ_FOR_EACH(_sz, _seg, _ring) \
	for (wlan_ring_read_prepare(_sz, &(_seg), _ring), \
		_wlan_ring_segment_prepare((_ring)->read, &(_seg), (_ring)); \
		(_seg).sz > 0; \
		_wlan_ring_read_commit(&(_seg), (_ring)), \
		_wlan_ring_segment_prepare((_ring)->read, &(_seg), (_ring)))

#define WLAN_RING_READ_ALL_FOR_EACH(seg, ring) WLAN_RING_READ_FOR_EACH((ring)->max_size, seg, ring)

#define WLAN_RING_READ_FOR_EACH_ITEM(_sz, _seg, _ring) \
	for (wlan_ring_read_prepare(_sz, &(_seg), _ring), \
		_wlan_ring_segment_prepare_item((_ring)->read, &(_seg), (_ring)); \
		(_seg).sz > 0; \
		_wlan_ring_read_commit(&(_seg), (_ring)), \
		_wlan_ring_segment_prepare_item((_ring)->read, &(_seg), (_ring)))

#define WLAN_RING_WRITE_FOR_EACH(_sz, _seg, _ring) \
	for (wlan_ring_write_prepare(_sz, &(_seg), _ring), \
		_wlan_ring_segment_prepare((_ring)->write, &(_seg), (_ring)); \
		(_seg).sz > 0; \
		_wlan_ring_write_commit(&(_seg), (_ring)), \
		_wlan_ring_segment_prepare((_ring)->write, &(_seg), (_ring)))

#define WLAN_RING_OVERWRITE_FOR_EACH(_sz, _seg, _ring) \
	for (wlan_ring_overwrite_prepare(_sz, &(_seg), _ring), \
		_wlan_ring_segment_prepare((_ring)->write, &(_seg), (_ring)); \
		(_seg).sz > 0; \
		_wlan_ring_write_commit(&(_seg), (_ring)), \
		_wlan_ring_segment_prepare((_ring)->write, &(_seg), (_ring)))

void wlan_ring_dump(const char *title, struct wlan_ring *ring);
void wlan_ring_dump_segment(const char *title, struct wlan_ring_segment *seg);


/* ring Buffer Internal API */
void _wlan_ring_segment_prepare(unsigned int from, struct wlan_ring_segment *seg, struct wlan_ring *ring);
void _wlan_ring_segment_prepare_item(unsigned int from, struct wlan_ring_segment *seg, struct wlan_ring *ring);
void _wlan_ring_read_commit(struct wlan_ring_segment *seg, struct wlan_ring *ring);
void _wlan_ring_write_commit(struct wlan_ring_segment *seg, struct wlan_ring *ring);

#endif
