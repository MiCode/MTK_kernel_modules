// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   reset_hif.c
 *  \brief  reset hif
 *
 *  This file contains all implementations of reset module
 */


/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */


/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/sdio_func.h>	/* sdio_readl(), etc */
#include <linux/mmc/host.h>		/* mmc_add_host(), etc */
#include <linux/mmc/sdio_ids.h>

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>

#include "reset.h"

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */
#ifndef CHIP_RESET_DTS_COMPATIBLE_NAME
#define CHIP_RESET_DTS_COMPATIBLE_NAME		"mediatek,mtk-wifi-reset"
#endif
#ifndef CHIP_RESET_GPIO_PROPERTY_NAME
#define CHIP_RESET_GPIO_PROPERTY_NAME		"reset-gpio-num"
#endif
#ifndef CHIP_RESET_INVERT_PROPERTY_NAME
#define CHIP_RESET_INVERT_PROPERTY_NAME		"invert-ms"
#endif
#ifndef CHIP_RESET_DEFAULT_VAL_PROPERTY_NAME
#define CHIP_RESET_DEFAULT_VAL_PROPERTY_NAME	"default-gpio-val"
#endif
#ifndef CHIP_POWER_DTS_COMPATIBLE_NAME
#define CHIP_POWER_DTS_COMPATIBLE_NAME		"mediatek,mtk-wifi-power"
#endif
#ifndef CHIP_POWER_GPIO_PROPERTY_NAME
#define CHIP_POWER_GPIO_PROPERTY_NAME		"power-gpio-num"
#endif
#ifndef CHIP_POWER_DEFAULT_VAL_PROPERTY_NAME
#define CHIP_POWER_DEFAULT_VAL_PROPERTY_NAME	"default-gpio-val"
#endif
#ifndef RESET_PIN_SET_LOW_TIME
#define RESET_PIN_SET_LOW_TIME			50
#endif

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */


/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */
struct ResetGpioInfo {
	bool flag_inited;
	unsigned int gpio_num;
	unsigned int default_level;
	unsigned int action_level;
	unsigned int invert_time;
};

struct PowerGpioInfo {
	bool flag_inited;
	unsigned int gpio_num;
	unsigned int switch_on_level;
	unsigned int switch_off_level;
};

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */


/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */


/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */
static struct mmc_host *prSdioHost;
static bool isSdioAdded;

static struct ResetGpioInfo resetGpioInfo;
static bool isResetGpioReleased;

static struct PowerGpioInfo powerGpioInfo;
static bool isPowerSwitchOn;

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
static void dtsGetResetGpioInfo(void)
{
	int i4Status;
	struct device_node *node;
	unsigned int gpio_num, default_level, action_level, invert_time;

	node = of_find_compatible_node(NULL,
				       NULL,
				       CHIP_RESET_DTS_COMPATIBLE_NAME);
	if (!node) {
		MR_Err("%s: Failed to find dts node: %s\n",
		       __func__, CHIP_RESET_DTS_COMPATIBLE_NAME);
		return;
	}
	if (of_property_read_u32(node, CHIP_RESET_GPIO_PROPERTY_NAME,
				&gpio_num) != 0) {
		MR_Err("%s: Failed to get gpio_num: %s\n",
		       __func__, CHIP_RESET_GPIO_PROPERTY_NAME);
		return;
	}
	if (of_property_read_u32(node, CHIP_RESET_INVERT_PROPERTY_NAME,
				&invert_time) != 0) {
		MR_Err("%s: Failed to get invert_time: %s\n",
		       __func__, CHIP_RESET_INVERT_PROPERTY_NAME);
		invert_time = RESET_PIN_SET_LOW_TIME;
	}
	if (of_property_read_u32(node, CHIP_RESET_DEFAULT_VAL_PROPERTY_NAME,
				&default_level) != 0) {
		MR_Err("%s: Failed to get default_level: %s\n",
		       __func__, CHIP_RESET_DEFAULT_VAL_PROPERTY_NAME);
		default_level = 1;
	}
	default_level = (default_level == 0) ? 0 : 1;
	action_level = (default_level == 0) ? 1 : 0;

	MR_Info("%s: read wifi reset gpio %d pull %s %dms from dts\n", __func__,
		gpio_num, (action_level == 0) ? "down" : "up", invert_time);

	resetGpioInfo.gpio_num = gpio_num;
	resetGpioInfo.default_level = default_level;
	resetGpioInfo.action_level = action_level;
	resetGpioInfo.invert_time = invert_time;

	i4Status = gpio_request(resetGpioInfo.gpio_num, "wifi-reset");
	if (i4Status < 0) {
		MR_Err("%s: gpio %d request failed, ret = %d\n",
		       __func__, resetGpioInfo.gpio_num, i4Status);
		resetGpioInfo.flag_inited = false;
	} else {
		resetGpioInfo.flag_inited = true;
	}
}

static void dtsGetPowerGpioInfo(void)
{
	int i4Status;
	struct device_node *node;
	unsigned int gpio_num, default_level, action_level;

	node = of_find_compatible_node(NULL,
				       NULL,
				       CHIP_POWER_DTS_COMPATIBLE_NAME);
	if (!node) {
		MR_Err("%s: Failed to find dts node: %s\n",
		       __func__, CHIP_POWER_DTS_COMPATIBLE_NAME);
		return;
	}
	if (of_property_read_u32(node, CHIP_POWER_GPIO_PROPERTY_NAME,
				&gpio_num) != 0) {
		MR_Err("%s: Failed to get gpio_num: %s\n",
		       __func__, CHIP_POWER_GPIO_PROPERTY_NAME);
		return;
	}
	if (of_property_read_u32(node, CHIP_POWER_DEFAULT_VAL_PROPERTY_NAME,
				&default_level) != 0) {
		MR_Err("%s: Failed to get default_level: %s\n",
		       __func__, CHIP_POWER_DEFAULT_VAL_PROPERTY_NAME);
		return;
	}
	default_level = (default_level == 0) ? 0 : 1;
	action_level = (default_level == 0) ? 1 : 0;

	MR_Info("%s: read wifi power gpio %d pull %s from dts\n", __func__,
		gpio_num, (action_level == 0) ? "down" : "up");

	powerGpioInfo.gpio_num = gpio_num;
	powerGpioInfo.switch_off_level = default_level;
	powerGpioInfo.switch_on_level = action_level;

	i4Status = gpio_request(powerGpioInfo.gpio_num, "wifi-power");
	if (i4Status < 0) {
		MR_Err("%s: gpio %d request failed, ret = %d\n",
		       __func__, powerGpioInfo.gpio_num, i4Status);
		powerGpioInfo.flag_inited = false;
	} else {
		powerGpioInfo.flag_inited = true;
	}
}

void resetHif_Init(void)
{
	prSdioHost = NULL;
	isSdioAdded = true;

	isResetGpioReleased = true;
	memset(&resetGpioInfo, 0, sizeof(struct ResetGpioInfo));
	dtsGetResetGpioInfo();

	isPowerSwitchOn = true;
	memset(&powerGpioInfo, 0, sizeof(struct PowerGpioInfo));
	dtsGetPowerGpioInfo();
}

void resetHif_Uninit(void)
{
	if (resetGpioInfo.flag_inited) {
		gpio_free(resetGpioInfo.gpio_num);
		resetGpioInfo.flag_inited = false;
	}
	if (powerGpioInfo.flag_inited) {
		gpio_free(powerGpioInfo.gpio_num);
		powerGpioInfo.flag_inited = false;
	}
}

enum ReturnStatus resetHif_UpdateSdioHost(void *data)
{
	struct mmc_host *host;
	struct sdio_func *func = (struct sdio_func *)data;

	if (!func || !func->card)
		return RESET_RETURN_STATUS_FAIL;

	host = func->card->host;
	if (prSdioHost != host) {
		MR_Warn("%s: sdio host is updated as %p\n", __func__, host);
		prSdioHost = host;
		dump_stack();
	}
	MR_Info("%s: update sdio host as %p\n", __func__, prSdioHost);

	return RESET_RETURN_STATUS_SUCCESS;
}

void resetHif_SdioRemoveHost(void)
{
	if (!prSdioHost) {
		MR_Err("%s: sdio host is NULL\n", __func__);
		return;
	}
	if (!isSdioAdded) {
		MR_Err("%s: sdio is already removed\n", __func__);
		return;
	}
	prSdioHost->rescan_entered = 0;
	MR_Warn("%s: mmc_remove_host\n", __func__);
	mmc_remove_host(prSdioHost);
	isSdioAdded = false;
}

void resetHif_SdioAddHost(void)
{
	if (!prSdioHost) {
		MR_Err("%s: sdio host is NULL\n", __func__);
		return;
	}
	if (isSdioAdded) {
		MR_Err("%s: sdio is already added\n", __func__);
		return;
	}
	prSdioHost->rescan_entered = 0;
	MR_Warn("%s: mmc_add_host\n", __func__);
	mmc_add_host(prSdioHost);
}

bool resetHif_isSdioAdded(void)
{
	return isSdioAdded;
}

void resetHif_ResetGpioPull(void)
{
	int i4Status;

	if (!isResetGpioReleased) {
		MR_Err("%s: reset gpio is already pulled\n", __func__);
		return;
	}
	if (!resetGpioInfo.flag_inited) {
		MR_Err("%s: reset gpio is unknown\n", __func__);
		return;
	}

	i4Status = gpio_direction_output(resetGpioInfo.gpio_num,
					 resetGpioInfo.action_level);
	if (i4Status < 0) {
		MR_Err("%s: gpio %d set output %d failed, ret = %d\n",
		       __func__, resetGpioInfo.gpio_num,
		       resetGpioInfo.action_level, i4Status);
		return;
	}

	MR_Warn("%s: pull reset gpio (%d, %d)\n", __func__,
		resetGpioInfo.gpio_num, resetGpioInfo.action_level);
	isResetGpioReleased = false;
}

void resetHif_ResetGpioRelease(void)
{
	int i4Status;

	if (isResetGpioReleased) {
		MR_Err("%s: reset gpio is already released\n", __func__);
		return;
	}
	if (!resetGpioInfo.flag_inited) {
		MR_Err("%s: reset gpio is unknown\n", __func__);
		return;
	}

	i4Status = gpio_direction_output(resetGpioInfo.gpio_num,
					 resetGpioInfo.default_level);
	if (i4Status < 0) {
		MR_Err("%s: gpio %d set output %d failed, ret = %d\n",
		       __func__, resetGpioInfo.gpio_num,
		       resetGpioInfo.default_level, i4Status);
		return;
	}

	MR_Warn("%s: release reset gpio (%d, %d)\n", __func__,
		resetGpioInfo.gpio_num, resetGpioInfo.default_level);
	isResetGpioReleased = true;
}

bool resetHif_isResetGpioReleased(void)
{
	return isResetGpioReleased;
}

void resetHif_PowerGpioSwitchOn(void)
{
	int i4Status;

	if (isPowerSwitchOn) {
		MR_Err("%s: power is already switched on\n", __func__);
		return;
	}
	if (!powerGpioInfo.flag_inited) {
		MR_Err("%s: power gpio is unknown\n", __func__);
		return;
	}

	i4Status = gpio_direction_output(powerGpioInfo.gpio_num,
					 powerGpioInfo.switch_on_level);
	if (i4Status < 0) {
		MR_Err("%s: gpio %d set output %d failed, ret = %d\n",
		       __func__, powerGpioInfo.gpio_num,
		       powerGpioInfo.switch_on_level, i4Status);
		return;
	}

	MR_Warn("%s: power switch on (%d, %d)\n", __func__,
		powerGpioInfo.gpio_num, powerGpioInfo.switch_on_level);
	isPowerSwitchOn = true;
}

void resetHif_PowerGpioSwitchOff(void)
{
	int i4Status;

	if (!isPowerSwitchOn) {
		MR_Err("%s: power is already switched off\n", __func__);
		return;
	}
	if (!powerGpioInfo.flag_inited) {
		MR_Err("%s: power gpio is unknown\n", __func__);
		return;
	}

	i4Status = gpio_direction_output(powerGpioInfo.gpio_num,
					 powerGpioInfo.switch_off_level);
	if (i4Status < 0) {
		MR_Err("%s: gpio %d set output %d failed, ret = %d\n",
		       __func__, powerGpioInfo.gpio_num,
		       powerGpioInfo.switch_off_level, i4Status);
		return;
	}

	MR_Warn("%s: power switch off (%d, %d)\n", __func__,
		powerGpioInfo.gpio_num, powerGpioInfo.switch_off_level);
	isPowerSwitchOn = false;
}

bool resetHif_isPowerSwitchOn(void)
{
	return isPowerSwitchOn;
}


