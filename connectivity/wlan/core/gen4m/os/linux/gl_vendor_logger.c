// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*
 ** gl_vendor_logger.c
 **
 **
 */

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "gl_os.h"
#include "debug.h"
#include "wlan_lib.h"
#include "gl_wext.h"
#include "precomp.h"
#include <linux/can/netlink.h>
#include <net/netlink.h>
#include <net/cfg80211.h>
#include <net/mac80211.h>
#include "gl_cfg80211.h"
#include "gl_vendor.h"
#include "wlan_oid.h"
#include "rlm_domain.h"
#include "gl_kal.h"

#if CFG_SUPPORT_LOGGER

char logger_buf[RING_DATA_HDR_SIZE + CFG80211_VENDOR_EVT_SKB_SZ];

struct logger_dev *gLoggerDev;
int32_t g_ring_cnt = 1;
static struct GLUE_INFO *g_prGlueInfo;

#define RING_DBGLOG(_Mod, _Clz, _Fmt, ...) \
	do { \
		if ((au2DebugModule[DBG_##_Mod##_IDX] & \
			 DBG_CLASS_##_Clz) == 0) \
			break; \
		pr_info("[%u]%s:(" #_Mod " " #_Clz ") " _Fmt, \
			 KAL_GET_CURRENT_THREAD_ID(), \
			 __func__, ##__VA_ARGS__); \
	} while (0)

#define RING_DBGLOG_LIMITED(_Mod, _Clz, _Fmt, ...) \
	do { \
		if ((au2DebugModule[DBG_##_Mod##_IDX] & \
			 DBG_CLASS_##_Clz) == 0) \
			break; \
		pr_info_ratelimited(\
			"[%u]%s:(" #_Mod " " #_Clz ") " _Fmt, \
			 KAL_GET_CURRENT_THREAD_ID(), \
			 __func__, ##__VA_ARGS__); \
	} while (0)

void dumpHex_ring(uint8_t *pucStartAddr, uint16_t u2Length)
{
#define BUFSIZE 160
	uint8_t output[BUFSIZE] = {0};
	uint32_t i = 0;
	uint32_t printed = 0;
	uint32_t offset = 0;

	ASSERT(pucStartAddr);
	pr_info("dump: 0x%p, Length: %d\n", pucStartAddr, u2Length);

	while (u2Length > 0) {
		for (i = 0, offset = 0; i < 128 && u2Length; i++) {
			offset += snprintf(output + offset, BUFSIZE - offset,
					"%c",
					pucStartAddr[printed + i]);
			u2Length--;
		}
		output[128] = '\0';
		pr_info("%04x: %s\n", printed, output);
		printed += 128;
	}
}

int mtk_cfg80211_vendor_get_logging_feature(struct wiphy *wiphy,
					struct wireless_dev *wdev,
					const void *data, int len)
{
	struct sk_buff *skb;
	uint32_t supported_features = 0;
	struct GLUE_INFO *prGlueInfo = NULL;

	RING_DBGLOG(SA, INFO, "[logger][enter]\n");

	if (!wiphy || !wdev || !data || len <= 0) {
		RING_DBGLOG(SA, INFO, "wrong input parameters\n");
		return -EINVAL;
	}

	WIPHY_PRIV(wiphy, prGlueInfo);
	if (!prGlueInfo) {
		RING_DBGLOG(SA, INFO, "wrong prGlueInfo parameters\n");
		return -EINVAL;
	}

	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy,
		sizeof(supported_features));
	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		return -ENOMEM;
	}

	if (unlikely(
		nla_put_nohdr(skb, sizeof(supported_features),
		&supported_features) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		goto nla_put_failure;
	}

	DBGLOG(REQ, TRACE, "supported feature set=0x%llx\n",
		supported_features);

	return cfg80211_vendor_cmd_reply(skb);

nla_put_failure:
	kfree_skb(skb);
	return -EFAULT;
}

int mtk_cfg80211_vendor_get_ring_status(struct wiphy *wiphy,
					struct wireless_dev *wdev,
					const void *data, int len)
{
	struct sk_buff *skb;
	struct logger_ring_status ring_status;
	size_t ring_status_size = sizeof(struct logger_ring_status);
	struct GLUE_INFO *prGlueInfo = NULL;

	RING_DBGLOG(SA, INFO, "[logger][enter]\n");

	ASSERT(wiphy);

	WIPHY_PRIV(wiphy, prGlueInfo);
	if (!prGlueInfo) {
		RING_DBGLOG(SA, INFO, "wrong prGlueInfo parameters\n");
		return -EINVAL;
	}

	g_prGlueInfo = prGlueInfo;
	logger_get_ring_status(&ring_status);

	RING_DBGLOG(SA, TEMP, "[logger] size=%d, nla_size=%d\n",
		ring_status_size, nla_total_size(ring_status_size));

	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy,
		nla_total_size(ring_status_size) *
		g_ring_cnt + nla_total_size(sizeof(g_ring_cnt)));
	if (!skb) {
		RING_DBGLOG(SA, INFO, "Allocate skb failed\n");
		return -ENOMEM;
	}

	(void)nla_put_u32(skb, LOGGER_ATTRIBUTE_RING_NUM, g_ring_cnt);
	(void)nla_put(skb, LOGGER_ATTRIBUTE_RING_STATUS, ring_status_size,
				&ring_status);

	return cfg80211_vendor_cmd_reply(skb);
}

int mtk_cfg80211_vendor_get_ring_data(
	struct wiphy *wiphy,
	struct wireless_dev *wdev,
	const void *data,
	int len)
{
	int ret = 0, rem, type;
	char ring_name[LOGGER_RING_NAME_MAX] = {0};
	const struct nlattr *iter;
	struct GLUE_INFO *prGlueInfo = NULL;

	RING_DBGLOG(SA, INFO, "[logger]enter [0x%x, 0x%x, 0x%x, %d]\n",
		wiphy, wdev, data, len);

	ASSERT(wiphy);

	WIPHY_PRIV(wiphy, prGlueInfo);
	if (!prGlueInfo) {
		RING_DBGLOG(SA, INFO, "wrong prGlueInfo parameters\n");
		return -EINVAL;
	}

	nla_for_each_attr(iter, data, len, rem) {
		type = nla_type(iter);
		switch (type) {
		case LOGGER_ATTRIBUTE_RING_NAME:
			kalMemCopy(ring_name, nla_data(iter),
				sizeof(ring_name));
			ring_name[LOGGER_RING_NAME_MAX - 1] = '\0';
			RING_DBGLOG(SA, INFO, "ring_name: %s\n", ring_name);
			break;
		default:
			RING_DBGLOG(SA, INFO, "Unknown type: %d\n", type);
			return ret;
		}
	}

	logger_get_ring_data(prGlueInfo);

	return ret;
}

int mtk_cfg80211_vendor_start_logging(struct wiphy *wiphy,
					struct wireless_dev *wdev,
					const void *data, int len)
{
	int ret = 0, rem, type;
	int log_level = 0, flags = 0, time_interval = 0, threshold = 0;
	const struct nlattr *iter = NULL;
	char ring_name[LOGGER_RING_NAME_MAX] = {0};
	struct GLUE_INFO *prGlueInfo = NULL;

	RING_DBGLOG(SA, INFO, "[logger]enter [0x%x, 0x%x, 0x%x, %d]\n",
		wiphy, wdev, data, len);

	ASSERT(wiphy);

	WIPHY_PRIV(wiphy, prGlueInfo);
	if (!prGlueInfo) {
		RING_DBGLOG(SA, INFO, "wrong prGlueInfo parameters\n");
		return -EINVAL;
	}

	nla_for_each_attr(iter, data, len, rem) {
		type = nla_type(iter);
		switch (type) {
		case LOGGER_ATTRIBUTE_RING_NAME:
			kalMemCopy(ring_name, nla_data(iter),
				sizeof(ring_name));
			ring_name[LOGGER_RING_NAME_MAX - 1] = '\0';
			break;
		case LOGGER_ATTRIBUTE_LOG_LEVEL:
			log_level = nla_get_u32(iter);
			break;
		case LOGGER_ATTRIBUTE_RING_FLAGS:
			flags = nla_get_u32(iter);
			break;
		case LOGGER_ATTRIBUTE_LOG_TIME_INTVAL:
			time_interval = nla_get_u32(iter);
			break;
		case LOGGER_ATTRIBUTE_LOG_MIN_DATA_SIZE:
			threshold = nla_get_u32(iter);
			break;
		default:
			RING_DBGLOG(SA, INFO, "Unknown type:%d\n", type);
			ret = -1;
			goto exit;
		}
	}

	RING_DBGLOG(SA, INFO,
		"[logger]%s:lv=%d, flag=%d, intv=%d, thrs=%d\n",
		ring_name, log_level, flags, time_interval, threshold);
	logger_start_logging(prGlueInfo, ring_name, log_level,
		flags, time_interval, threshold);

exit:
	return ret;
}

int mtk_cfg80211_vendor_reset_logging(struct wiphy *wiphy,
					struct wireless_dev *wdev,
					const void *data, int len)
{
	int ret = 0;
	struct GLUE_INFO *prGlueInfo = NULL;

	RING_DBGLOG(SA, INFO, "[logger][Enter]");

	if (!wiphy || !wdev || !data || len <= 0) {
		RING_DBGLOG(SA, INFO, "wrong input parameters\n");
		return -EINVAL;
	}

	WIPHY_PRIV(wiphy, prGlueInfo);
	if (!prGlueInfo) {
		RING_DBGLOG(SA, INFO, "prGlueInfo is NULL\n");
		return -EINVAL;
	}

	logger_reset_logging(prGlueInfo);

	return ret;
}

static int logger_get_ring_data(struct GLUE_INFO *prGlueInfo)
{
	RING_DBGLOG(SA, INFO, "[Enter]\n");

	cancel_delayed_work_sync(&prGlueInfo->rLoggerWork);
	schedule_delayed_work(&prGlueInfo->rLoggerWork, 0);

	return 0;
}

static int logger_start_logging(
	struct GLUE_INFO *prGlueInfo,
	char *ring_name,
	uint32_t log_level,
	uint32_t flags,
	uint32_t time_interval,
	uint32_t threshold)
{
	RING_DBGLOG(SA, INFO, "[Enter]\n");

	if (kalStrLen(ring_name) < LOGGER_RING_NAME_MAX)
		kalStrnCpy(gLoggerDev->name, ring_name,
			kalStrLen(ring_name) + 1);
	else
		kalStrnCpy(gLoggerDev->name, ring_name,
			LOGGER_RING_NAME_MAX - 1);

	gLoggerDev->log_level = log_level;
	gLoggerDev->flags = flags;
	gLoggerDev->threshold = threshold;

	if (time_interval == 0 || log_level == 0) {
		gLoggerDev->interval = 0;
		cancel_delayed_work_sync(&prGlueInfo->rLoggerWork);
	} else {
		gLoggerDev->interval = MSEC_TO_JIFFIES(time_interval);
		RING_DBGLOG(SA, INFO, "[time_interval=%d, interval=%d]\n",
			time_interval, gLoggerDev->interval);
		cancel_delayed_work_sync(&prGlueInfo->rLoggerWork);
		schedule_delayed_work(&prGlueInfo->rLoggerWork,
			gLoggerDev->interval);
	}

	return 0;
}

static int logger_reset_logging(struct GLUE_INFO *prGlueInfo)
{
	RING_DBGLOG(SA, INFO, "[Enter]\n");

	if (prGlueInfo) {
		gLoggerDev->log_level = 0;
		gLoggerDev->flags = 0;
		gLoggerDev->interval = 0;
		gLoggerDev->threshold = 0;

		cancel_delayed_work_sync(&prGlueInfo->rLoggerWork);
	} else {
		RING_DBGLOG(SA, INFO, "prGlueInfo is null\n");
	}
	return 0;
}

int logger_work_init(struct GLUE_INFO *prGlueInfo)
{
	RING_DBGLOG(SA, INFO, "[Enter]");
	if (prGlueInfo) {
		INIT_DELAYED_WORK(&(prGlueInfo->rLoggerWork),
			logger_poll_worker);
	} else {
		RING_DBGLOG(SA, INFO, "prGlueInfo is null\n");
	}
	return 0;
}

int logger_work_uninit(struct GLUE_INFO *prGlueInfo)
{
	RING_DBGLOG(SA, INFO, "[Enter]");
	if (!prGlueInfo)
		RING_DBGLOG(SA, INFO, "prGlueInfo is null\n");

	cancel_delayed_work_sync(&prGlueInfo->rLoggerWork);

	return 0;
}

static int logger_get_ring_status(struct logger_ring_status *ring_status)
{
	if (ring_status) {
		kalStrnCpy(ring_status->name, LOGGER_RING_NAME,
			LOGGER_RING_NAME_MAX);
		ring_status->ring_id = 1;
		ring_status->ring_buffer_byte_size
			= logger_get_buf_size(&gLoggerDev->iRing);
		ring_status->written_bytes = 0;
		ring_status->written_records = 0;
		ring_status->read_bytes = 0;
		ring_status->verbose_level = 1;
	} else {
		RING_DBGLOG(SA, INFO, "ring_status is null\n");
	}
	return 0;
}

static void logger_poll_worker(struct work_struct *work)
{
	struct GLUE_INFO *prGlueInfo;
	struct sk_buff *skb = NULL;
	struct wiphy *wiphy;
	struct wireless_dev *wdev;
	struct nlmsghdr *nlh;
	struct logger_ring_status ring_status;
	struct logger_ring_data *prRingData;
	size_t read = 0;
	uint32_t u4HeaderLen, u4DataLen, u4MaxDataLen;
	uint32_t u4RingBufSize;

	RING_DBGLOG(SA, INFO, "[logger]Enter");

	wiphy = wlanGetWiphy();
	WIPHY_PRIV(wiphy, prGlueInfo);
	if (!prGlueInfo) {
		RING_DBGLOG(SA, INFO, "prGlueInfo is NULL");
		return;
	}

	wdev = prGlueInfo->prDevHandler->ieee80211_ptr;
	skb = kalCfg80211VendorEventAlloc(wiphy, wdev,
			CFG80211_VENDOR_EVT_SKB_SZ, WIFI_EVENT_RING_EVENT,
			GFP_KERNEL);
	if (!skb) {
		RING_DBGLOG(SA, INFO, "%s allocate skb failed\n", __func__);
		return;
	}

	logger_get_ring_status(&ring_status);

	u4HeaderLen = 4 * sizeof(uint32_t) + (2 * NLA_HDRLEN) + NLMSG_HDRLEN;
	u4MaxDataLen = CFG80211_VENDOR_EVT_SKB_SZ - u4HeaderLen
		- sizeof(ring_status);
	RING_DBGLOG(SA, TEMP, "[logger]u4HeaderLen=%d, u4MaxDataLen=%d",
		u4HeaderLen, u4MaxDataLen);

	u4RingBufSize = logger_get_buf_size(&gLoggerDev->iRing);
	if (u4RingBufSize == 0) {
		RING_DBGLOG(SA, INFO, "[logger]u4RingBufSize is zero");
		return;
	}

	if (u4RingBufSize < u4MaxDataLen)
		u4DataLen = u4RingBufSize;
	else
		u4DataLen = u4MaxDataLen;

	/* Ring Data Header Config */
	prRingData = (struct logger_ring_data *)logger_buf;
	prRingData->num_entries = 1;
	prRingData->entry_size = u4DataLen;
	prRingData->flags = 0;
	prRingData->type = 0;
	prRingData->timestamp = 0;

	RING_DBGLOG(SA, INFO, "[logger]Buf=%d, DataLen=%d, MaxDataLen=%d",
		u4RingBufSize, u4DataLen, u4MaxDataLen);
	read = logger_read(logger_buf + RING_DATA_HDR_SIZE, u4DataLen);
	RING_DBGLOG(SA, INFO, "[logger]read=%d, entry_size=%d",
		read, prRingData->entry_size);

	/* Set halpid for sending unicast event to wifi hal */
	nlh = (struct nlmsghdr *)skb->data;
	nla_put(skb, LOGGER_ATTRIBUTE_RING_STATUS,
		sizeof(ring_status), &ring_status);
	nla_put(skb, LOGGER_ATTRIBUTE_RING_DATA,
		read + RING_DATA_HDR_SIZE, logger_buf);

	cfg80211_vendor_event(skb, GFP_KERNEL);

	if (!gLoggerDev->sched_pull) {
		RING_DBGLOG(SA, INFO, "[logger]sched_pull=%d",
			gLoggerDev->sched_pull);
		gLoggerDev->sched_pull = TRUE;
	}

	if (gLoggerDev->interval) {
		RING_DBGLOG(SA, INFO, "[logger]interval=%dms, jiffies=%d",
			gLoggerDev->interval,
			MSEC_TO_JIFFIES(gLoggerDev->interval));
		schedule_delayed_work(&prGlueInfo->rLoggerWork,
			MSEC_TO_JIFFIES(gLoggerDev->interval));
	}
}

void
logger_pullreq(struct GLUE_INFO *prGlueInfo)
{
	RING_DBGLOG(SA, INFO, "[Enter]\n");
	if (prGlueInfo) {
		cancel_delayed_work(&prGlueInfo->rLoggerWork);
		schedule_delayed_work(&prGlueInfo->rLoggerWork, 0);
	} else {
		RING_DBGLOG(SA, INFO, "prGlueInfo is null\n");
	}
}

static int logger_ring_init(struct logger_ring *iRing, size_t size)
{
	int ret = 0;
	void *pBuffer = NULL;

	RING_DBGLOG(SA, INFO, "[size=%d]\n", size);

	if (unlikely(iRing->ring_base)) {
		RING_DBGLOG(SA, ERROR, "logger_ring has init?\n");
		ret = -EPERM;
	} else {
		pBuffer = kmalloc(size, GFP_KERNEL);
		if (pBuffer == NULL && size > PAGE_SIZE)
			pBuffer = vmalloc(size);

		if (!pBuffer)
			ret = -ENOMEM;
		else {
			iRing->ring_base = pBuffer;
			iRing->ring_size = size;
			WLAN_RING_INIT(
				iRing->ring_base,
				iRing->ring_size,
				0,
				0,
				&iRing->ring_cache);
		}
	}

	return ret;
}

static void logger_ring_deinit(struct logger_ring *iRing)
{
	RING_DBGLOG(SA, INFO, "[Enter]\n");
	if (likely(iRing->ring_base)) {
		kvfree(iRing->ring_base);
		iRing->ring_base = NULL;
	}
}

static ssize_t logger_read(
	char *buf,
	size_t count)
{
	ssize_t read = 0;
	struct wlan_ring_segment ring_seg;
	struct wlan_ring *ring = &(gLoggerDev->iRing.ring_cache);
	ssize_t left_to_read = 0;

	if (likely(gLoggerDev->iRing.ring_base)) {
		left_to_read = count < WLAN_RING_SIZE(ring)
				? count : WLAN_RING_SIZE(ring);
		if (WLAN_RING_EMPTY(ring) ||
			!WLAN_RING_READ_PREPARE(left_to_read,
				&ring_seg, ring)) {
			RING_DBGLOG(SA, TEMP,
				"no data/taken by other reader?\n");
			goto return_fn;
		}

		WLAN_RING_READ_FOR_EACH(left_to_read, ring_seg, ring) {
			memcpy(buf + read, ring_seg.ring_pt,
				ring_seg.sz);
			left_to_read -= ring_seg.sz;
			read += ring_seg.sz;
		}
	} else {
		RING_DBGLOG(SA, ERROR, "logger_ring not init yet\n");
		read = -EPERM;
	}

return_fn:
	RING_DBGLOG(SA, INFO, "[Done] read:%ld left:%ld\n", read,
		left_to_read);
	return read;
}

static ssize_t logger_write(struct logger_ring *iRing, char *buf,
	size_t count)
{
	ssize_t written = 0;
	struct wlan_ring_segment ring_seg;
	struct wlan_ring *ring = &iRing->ring_cache;
	ssize_t left_to_write = count;

	if (likely(iRing->ring_base)) {
		/* no enough buffer to write */
		if (WLAN_RING_WRITE_REMAIN_SIZE(ring) < left_to_write)
			goto skip;

		WLAN_RING_WRITE_FOR_EACH(left_to_write, ring_seg, ring) {
			memcpy(ring_seg.ring_pt, buf + written, ring_seg.sz);
			left_to_write -= ring_seg.sz;
			written += ring_seg.sz;
		}

	} else {
		RING_DBGLOG(SA, ERROR, "logger_ring not init yet\n");
		written = -EPERM;
	}

skip:
	RING_DBGLOG(SA, TEMP, "[Done] written:%ld left:%ld\n", written,
		left_to_write);
	return written;
}

static ssize_t logger_get_buf_size(struct logger_ring *iRing)
{
	struct wlan_ring *ring = &iRing->ring_cache;

	if (unlikely(iRing->ring_base == NULL)) {
		RING_DBGLOG(SA, ERROR, "logger_ring not init yet\n");
		return -EPERM;
	}

	return WLAN_RING_SIZE(ring);
}

int logger_init(struct GLUE_INFO *prGlueInfo)
{
	int result = 0;
	int err = 0;

	RING_DBGLOG(SA, INFO, "[Enter]\n");

	gLoggerDev = kzalloc(sizeof(struct logger_dev), GFP_KERNEL);
	if (gLoggerDev == NULL) {
		RING_DBGLOG(SA, ERROR, "gLoggerDev is null\n");
		result = -ENOMEM;
		return result;
	}

	err = logger_ring_init(&gLoggerDev->iRing, LOGGER_BUF_SIZE);
	if (err) {
		result = -ENOMEM;
		RING_DBGLOG(SA, ERROR,
			"Error %d logger_ring_init\n", err);
		kfree(gLoggerDev);
		return result;
	}

	gLoggerDev->sched_pull = TRUE;
	return result;
}

int logger_deinit(struct GLUE_INFO *prGlueInfo)
{
	RING_DBGLOG(SA, INFO, "[Enter]\n");

	logger_ring_deinit(&gLoggerDev->iRing);
	kfree(gLoggerDev);
	return 0;
}

ssize_t wifi_logger_write(char *buf, size_t count)
{
	ssize_t ret = 0;
	uint32_t u4RingBufLen;

	ret = logger_write(&gLoggerDev->iRing, buf, count);

	u4RingBufLen = logger_get_buf_size(&gLoggerDev->iRing);
	if (gLoggerDev->threshold > 0 &&
		(u4RingBufLen >= gLoggerDev->threshold) &&
			gLoggerDev->sched_pull) {
		RING_DBGLOG(SA, INFO, "sched_pull=%d[threshold=%d:len=%d]\n",
			gLoggerDev->sched_pull,
			gLoggerDev->threshold,
			u4RingBufLen);
		gLoggerDev->sched_pull = FALSE;
		logger_pullreq(g_prGlueInfo);
	}

	return ret;
}
#endif
