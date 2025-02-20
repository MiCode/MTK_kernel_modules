/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef __BTMTK_CONFIG_H__
#define __BTMTK_CONFIG_H__

#include <linux/version.h>

/* It's for reset procedure */
#include <linux/of_gpio.h>
#include <linux/mmc/host.h>

/**
 * Kernel configuration check
 */
#ifndef CONFIG_PM
	#error "ERROR : CONFIG_PM should be turn on."
#endif

/**
 * Support IC configuration
 */
#define SUPPORT_MT7662 1
#define SUPPORT_MT7668 1
#define SUPPORT_MT7663 1

/**
 * BTMTK LOG location, last char must be '/'
 */
/* #define BTMTK_LOG_PATH	"/data/misc/bluedroid/" */

#ifndef LOWER_POWER_SINK
#define LOWER_POWER_SINK 1
#endif

/**
 * Fixed STPBT Major Device Id
 */
#define FIXED_STPBT_MAJOR_DEV_ID 111

/**
 * WoBLE by BLE RC
 */
 /*Linux build fail due to wake_lock, please set SUPPORT_ANDROID 0 for Linux*/
/*#define SUPPORT_ANDROID 0 */
#define BT_RC_VENDOR_DEFAULT 1
#define BT_RC_VENDOR_S0 0

#define WAIT_POWERKEY_TIMEOUT 5000

/**
 * Support toggle GPIO
 */
#define MT76x8_PMU_EN_PIN_NAME		"mt76x8_pmu_en_gpio"
#define MT76x8_PMU_EN_DELAY_NAME	"mt76x8_pmu_en_delay"
#define MT76x8_PMU_EN_DEFAULT_DELAY	(5) /* Default delay 5ms */

/**
 * L0 reset
 */
#define L0_RESET_TAG				"[SER][L0] "

#endif /* __BTMTK_CONFIG_H__ */
