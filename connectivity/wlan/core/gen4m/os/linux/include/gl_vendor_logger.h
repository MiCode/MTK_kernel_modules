/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2024 MediaTek Inc.
 */
#include "wlan_ring.h"

#ifndef _GL_VENDOR_LOGGER_H_
#define _GL_VENDOR_LOGGER_H_

#if CFG_SUPPORT_LOGGER

#define LOGGER_RING_NAME		"logger_ring"

#define LOGGER_BUF_SIZE			(2*1024*1024) /* 2MB */
#define LOGGER_RING_NAME_MAX		32

#define CFG80211_VENDOR_EVT_SKB_SZ	(1024*1024) /* 1MB */
#define RING_DATA_HDR_SIZE		16
#define RING_DATA_ENTRY_HDR_SIZE	12

struct logger_ring {
	/* ring related variable */
	struct wlan_ring ring_cache;
	size_t ring_size;
	void *ring_base;
};

struct logger_dev {
	uint8_t		name[LOGGER_RING_NAME_MAX];
	uint32_t	log_level;
	uint32_t	flags;
	uint32_t	interval;
	uint32_t	threshold;
	bool		sched_pull;
	struct logger_ring	iRing;
};

struct logger_ring_status {
	uint8_t name[LOGGER_RING_NAME_MAX];
	uint32_t flags;
	int ring_id;
	uint32_t ring_buffer_byte_size;
	uint32_t verbose_level;
	uint32_t written_bytes;
	uint32_t read_bytes;
	uint32_t written_records;
};

struct logger_ring_data {
	uint32_t num_entries;
	uint32_t entry_size;
	uint8_t flags;
	uint8_t type;
	uint64_t timestamp;
};

int mtk_cfg80211_vendor_start_logging(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				const void *data, int len);

int mtk_cfg80211_vendor_get_ring_status(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				const void *data, int len);

int mtk_cfg80211_vendor_get_ring_data(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				const void *data, int len);

int mtk_cfg80211_vendor_get_logging_feature(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				const void *data, int len);

int mtk_cfg80211_vendor_reset_logging(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				const void *data, int len);

static int logger_get_ring_status(struct logger_ring_status *ring_status);
static int logger_get_ring_data(struct GLUE_INFO *prGlueInfo);
static int logger_start_logging(
	struct GLUE_INFO *prGlueInfo,
	char *ring_name,
	uint32_t log_level,
	uint32_t flags,
	uint32_t time_interval,
	uint32_t threshold);
static int logger_reset_logging(struct GLUE_INFO *prGlueInfo);
static void logger_poll_worker(struct work_struct *work);
static ssize_t logger_read(char *buf, size_t count);
static ssize_t logger_get_buf_size(struct logger_ring *iRing);

int logger_init(struct GLUE_INFO *prGlueInfo);
int logger_deinit(struct GLUE_INFO *prGlueInfo);
int logger_work_init(struct GLUE_INFO *prGlueInfo);
int logger_work_uninit(struct GLUE_INFO *prGlueInfo);

ssize_t wifi_logger_write(char *buf, size_t count);

#endif /* CFG_SUPPORT_LOGGER */
#endif /* _GL_VENDOR_LOGGER_H_ */
