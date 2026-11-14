/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _GL_CONCURRENCY_MATRIX_H
#define _GL_CONCURRENCY_MATRIX_H

/******************************************************************************
 *                                 M A C R O S
 ******************************************************************************
 */

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

#define MAX_IFACE_COMBINATIONS 16
#define MAX_IFACE_LIMITS 8

struct wifi_iface_limit {
	/* Max number of interfaces of same type */
	uint32_t max_limit;

	/* BIT mask of interfaces from wifi_interface_type */
	uint32_t iface_mask;
};

struct wifi_iface_combination {
	/* Maximum number of concurrent interfaces allowed in this
	 * combination
	 */
	uint32_t max_ifaces;

	/* Total number of interface limits in a combination */
	uint32_t num_iface_limits;

	/* Interface limits */
	struct wifi_iface_limit iface_limits[MAX_IFACE_LIMITS];
};

struct wifi_iface_concurrency_matrix {
	/* Total count of possible iface combinations */
	uint32_t num_iface_combinations;

	/* Interface combinations */
	struct wifi_iface_combination iface_combinations[
		MAX_IFACE_COMBINATIONS];
};

struct mtk_wifi_iface_combination {
	/* Maximum number of concurrent interfaces allowed in this
	 * combination
	 */
	uint32_t max_ifaces;

	/* Total number of interface limits in a combination */
	uint32_t num_iface_limits;

	/* Interface limits */
	struct wifi_iface_limit *iface_limits;
};

struct mtk_wifi_iface_concurrency_matrix {
	/* Total count of possible iface combinations */
	uint32_t num_iface_combinations;

	/* Interface combinations */
	struct mtk_wifi_iface_combination *iface_combinations;
};

enum wifi_interface_type {
	WIFI_INTERFACE_TYPE_STA        = 0,
	WIFI_INTERFACE_TYPE_AP         = 1,
	WIFI_INTERFACE_TYPE_P2P        = 2,
	WIFI_INTERFACE_TYPE_NAN        = 3,
	WIFI_INTERFACE_TYPE_AP_BRIDGED = 4,
};

/******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 ******************************************************************************
 */

int mtk_cfg80211_vendor_get_chip_concurrency_matrix(struct wiphy *wiphy,
		struct wireless_dev *wdev, const void *data, int data_len);

#endif /* _GL_CONCURRENCY_MATRIX_H */
