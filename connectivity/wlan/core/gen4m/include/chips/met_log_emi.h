/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _MET_LOG_EMI_H
#define _MET_LOG_EMI_H

#if CFG_SUPPORT_MET_LOG
struct MET_LOG_EMI_CTRL {
	uint8_t initialized;
	uint32_t project_id;
	struct workqueue_struct *wq;
	struct work_struct work;
	void *priv;
	uint32_t start_addr;
	uint32_t end_addr;
	uint32_t offset;
	uint32_t emi_size;
	uint32_t rp; /* read pointer */
	uint32_t irp; /* internal read pointer, used by driver */
	uint32_t wp; /* write pointer */
	uint32_t checksum; /* 1: new MET data cover un-read MET data */
	uint8_t *buffer;
#if (CFG_SUPPORT_WIFI_MET_CONNAC3_V2 == 1)
	uint32_t start_addr_wifi_long;
	uint32_t end_addr_wifi_long;
	uint32_t offset_wifi_long;
	uint32_t emi_size_wifi_long;
	uint32_t rp_wifi_long; /* read pointer */
	uint32_t irp_wifi_long; /* internal read pointer, used by driver */
	uint32_t wp_wifi_long; /* write pointer */
	uint32_t checksum_wifi_long;
	uint8_t *buffer_wifi_long;
	uint32_t start_addr_common;
	uint32_t end_addr_common;
	uint32_t offset_common;
	uint32_t emi_size_common;
	uint32_t rp_common; /* read pointer */
	uint32_t irp_common; /* internal read pointer, used by driver */
	uint32_t wp_common; /* write pointer */
	uint32_t checksum_common;
	uint8_t *buffer_common;
#endif
};

struct MET_LOG_HEADER {
	uint32_t rp; /* read pointer */
	uint32_t wp; /* write pointer */
	uint32_t checksum; /* 1: new MET data cover un-read MET data */
};

uint32_t met_log_emi_init(struct ADAPTER *ad);
uint32_t met_log_emi_deinit(struct ADAPTER *ad);
#endif /* CFG_SUPPORT_MET_LOG */

#endif /* _MET_LOG_EMI_H */
