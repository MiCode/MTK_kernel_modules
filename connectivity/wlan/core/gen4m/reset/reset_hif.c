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
#include <linux/module.h>
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

static struct ResetGpioInfo resetGpioInfo[MAX_DONGLE_NUM];
static bool isResetGpioReleased[MAX_DONGLE_NUM];

static struct PowerGpioInfo powerGpioInfo[MAX_DONGLE_NUM];
static bool isPowerSwitchOn[MAX_DONGLE_NUM];

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
static void dtsGetResetGpioInfo(uint32_t dongle_id, struct device_node *node)
{
#if CFG_RESETKO_SUPPORT_MULTI_CARD
	int i;
#endif
	int i4Status;
	unsigned int gpio_num, default_level, action_level, invert_time;

	if (!node)
		node = of_find_compatible_node(NULL, NULL,
					       CHIP_RESET_DTS_COMPATIBLE_NAME);
	if (!node) {
		MR_Err("[%d] %s: Failed to find dts node: %s\n",
		       dongle_id, __func__, CHIP_RESET_DTS_COMPATIBLE_NAME);
		return;
	}
	if (of_property_read_u32(node, CHIP_RESET_GPIO_PROPERTY_NAME,
				&gpio_num) != 0) {
		MR_Err("[%d] %s: Failed to get gpio_num: %s\n",
		       dongle_id, __func__, CHIP_RESET_GPIO_PROPERTY_NAME);
		return;
	}
	if (of_property_read_u32(node, CHIP_RESET_INVERT_PROPERTY_NAME,
				&invert_time) != 0) {
		MR_Err("[%d] %s: Failed to get invert_time: %s\n",
		       dongle_id, __func__, CHIP_RESET_INVERT_PROPERTY_NAME);
		invert_time = RESET_PIN_SET_LOW_TIME;
	}
	if (of_property_read_u32(node, CHIP_RESET_DEFAULT_VAL_PROPERTY_NAME,
				&default_level) != 0) {
		MR_Err("[%d] %s: Failed to get default_level: %s\n",
		       dongle_id, __func__,
		       CHIP_RESET_DEFAULT_VAL_PROPERTY_NAME);
		default_level = 1;
	}
	default_level = (default_level == 0) ? 0 : 1;
	action_level = (default_level == 0) ? 1 : 0;

	MR_Info("[%d] %s: read wifi reset gpio %d pull %s %dms from dts\n",
		dongle_id, __func__,
		gpio_num, (action_level == 0) ? "down" : "up", invert_time);

	if (dongle_id >= MAX_DONGLE_NUM)
		return;

	resetGpioInfo[dongle_id].gpio_num = gpio_num;
	resetGpioInfo[dongle_id].default_level = default_level;
	resetGpioInfo[dongle_id].action_level = action_level;
	resetGpioInfo[dongle_id].invert_time = invert_time;

#if CFG_RESETKO_SUPPORT_MULTI_CARD
	for (i = 0; i < MAX_DONGLE_NUM; i++) {
		if ((i != dongle_id) &&
		    (gpio_num == resetGpioInfo[i].gpio_num)) {
			MR_Info(
			  "[%d] %s: gpio %d already requested by dongle [%d]\n",
			  dongle_id, __func__, gpio_num, i);
			resetGpioInfo[dongle_id].flag_inited = true;
			return;
		}
	}
#endif

	i4Status = gpio_request(resetGpioInfo[dongle_id].gpio_num,
				"wifi-reset");
	if (i4Status < 0) {
		MR_Err("[%d] %s: gpio %d request failed, ret = %d\n",
		       dongle_id, __func__,
		       resetGpioInfo[dongle_id].gpio_num, i4Status);
		resetGpioInfo[dongle_id].flag_inited = false;
	} else {
		resetGpioInfo[dongle_id].flag_inited = true;
	}
}

static void dtsGetPowerGpioInfo(uint32_t dongle_id, struct device_node *node)
{
#if CFG_RESETKO_SUPPORT_MULTI_CARD
	int i;
#endif
	int i4Status;
	unsigned int gpio_num, default_level, action_level;

	if (!node)
		node = of_find_compatible_node(NULL, NULL,
					       CHIP_POWER_DTS_COMPATIBLE_NAME);
	if (!node) {
		MR_Err("[%d] %s: Failed to find dts node: %s\n",
		       dongle_id, __func__, CHIP_POWER_DTS_COMPATIBLE_NAME);
		return;
	}
	if (of_property_read_u32(node, CHIP_POWER_GPIO_PROPERTY_NAME,
				&gpio_num) != 0) {
		MR_Err("[%d] %s: Failed to get gpio_num: %s\n",
		       dongle_id, __func__, CHIP_POWER_GPIO_PROPERTY_NAME);
		return;
	}
	if (of_property_read_u32(node, CHIP_POWER_DEFAULT_VAL_PROPERTY_NAME,
				&default_level) != 0) {
		MR_Err("[%d] %s: Failed to get default_level: %s\n",
		       dongle_id, __func__,
		       CHIP_POWER_DEFAULT_VAL_PROPERTY_NAME);
		return;
	}
	default_level = (default_level == 0) ? 0 : 1;
	action_level = (default_level == 0) ? 1 : 0;

	MR_Info("[%d] %s: read wifi power gpio %d pull %s from dts\n",
		dongle_id, __func__,
		gpio_num, (action_level == 0) ? "down" : "up");

	if (dongle_id >= MAX_DONGLE_NUM)
		return;

	powerGpioInfo[dongle_id].gpio_num = gpio_num;
	powerGpioInfo[dongle_id].switch_off_level = default_level;
	powerGpioInfo[dongle_id].switch_on_level = action_level;

#if CFG_RESETKO_SUPPORT_MULTI_CARD
	for (i = 0; i < MAX_DONGLE_NUM; i++) {
		if ((i != dongle_id) &&
		    (gpio_num == resetGpioInfo[i].gpio_num)) {
			MR_Info(
			  "[%d] %s: gpio %d already requested by dongle [%d]\n",
			  dongle_id, __func__, gpio_num, i);
			powerGpioInfo[dongle_id].flag_inited = true;
			return;
		}
	}
#endif

	i4Status = gpio_request(powerGpioInfo[dongle_id].gpio_num,
				"wifi-power");
	if (i4Status < 0) {
		MR_Err("[%d] %s: gpio %d request failed, ret = %d\n",
		       dongle_id, __func__,
		       powerGpioInfo[dongle_id].gpio_num, i4Status);
		powerGpioInfo[dongle_id].flag_inited = false;
	} else {
		powerGpioInfo[dongle_id].flag_inited = true;
	}
}

void resetHif_Init(uint32_t dongle_id, struct device_node *node)
{
	prSdioHost = NULL;
	isSdioAdded = false;

	if (dongle_id >= MAX_DONGLE_NUM)
		return;

	isResetGpioReleased[dongle_id] = true;
	memset(&resetGpioInfo[dongle_id], 0, sizeof(struct ResetGpioInfo));
	dtsGetResetGpioInfo(dongle_id, node);

	isPowerSwitchOn[dongle_id] = true;
	memset(&powerGpioInfo[dongle_id], 0, sizeof(struct PowerGpioInfo));
	dtsGetPowerGpioInfo(dongle_id, node);
}

void resetHif_Uninit(uint32_t dongle_id)
{
#if CFG_RESETKO_SUPPORT_MULTI_CARD
	int i;
#endif
	bool skip;

	if (dongle_id >= MAX_DONGLE_NUM)
		return;

	skip = false;
	if (resetGpioInfo[dongle_id].flag_inited) {
		resetGpioInfo[dongle_id].flag_inited = false;
#if CFG_RESETKO_SUPPORT_MULTI_CARD
		for (i = 0; i < MAX_DONGLE_NUM; i++) {
			if ((resetGpioInfo[i].flag_inited == true) &&
			    (resetGpioInfo[dongle_id].gpio_num ==
			     resetGpioInfo[i].gpio_num)) {
				skip = true;
				break;
			}
		}
#endif
		if (!skip)
			gpio_free(resetGpioInfo[dongle_id].gpio_num);
	}

	skip = false;
	if (powerGpioInfo[dongle_id].flag_inited) {
		powerGpioInfo[dongle_id].flag_inited = false;
#if CFG_RESETKO_SUPPORT_MULTI_CARD
		for (i = 0; i < MAX_DONGLE_NUM; i++) {
			if ((powerGpioInfo[i].flag_inited == true) &&
			    (powerGpioInfo[dongle_id].gpio_num ==
			     powerGpioInfo[i].gpio_num)) {
				skip = true;
				break;
			}
		}
#endif
		if (!skip)
			gpio_free(powerGpioInfo[dongle_id].gpio_num);
	}
}

enum ReturnStatus resetHif_UpdateSdioHost(void *data)
{
	struct mmc_host *host;
	struct sdio_func *func = (struct sdio_func *)data;

	if (!func || !func->card)
		return RESET_RETURN_STATUS_FAIL;

	isSdioAdded = true;
	host = func->card->host;
	if (prSdioHost != host) {
		MR_Warn("[0] %s: sdio host is updated as %p\n", __func__, host);
		prSdioHost = host;
		dump_stack();
	}
	MR_Info("[0] %s: update sdio host as %p\n", __func__, prSdioHost);

	return RESET_RETURN_STATUS_SUCCESS;
}

void resetHif_SdioRemoveHost(void)
{
	if (!prSdioHost) {
		MR_Err("[0] %s: sdio host is NULL\n", __func__);
		return;
	}
	if (!isSdioAdded) {
		MR_Err("[0] %s: sdio is already removed\n", __func__);
		return;
	}
	prSdioHost->rescan_entered = 0;
	MR_Warn("[0] %s: mmc_remove_host\n", __func__);
	mmc_remove_host(prSdioHost);
	isSdioAdded = false;
}

void resetHif_SdioAddHost(void)
{
	if (!prSdioHost) {
		MR_Err("[0] %s: sdio host is NULL\n", __func__);
		return;
	}
	if (isSdioAdded) {
		MR_Err("[0] %s: sdio is already added\n", __func__);
		return;
	}
	prSdioHost->rescan_entered = 0;
	MR_Warn("[0] %s: mmc_add_host\n", __func__);
	mmc_add_host(prSdioHost);
	isSdioAdded = true;
}

bool resetHif_isSdioAdded(void)
{
	return isSdioAdded;
}

static int resetHif_GpioOutput(unsigned int gpo, unsigned int val)
{
#if ((CFG_CHIP_RESET_USE_MSTAR_GPIO_API == 1) && (CFG_ENABLE_GKI_SUPPORT != 1))
	typedef void (*gpioMstarFunc)(uint32_t);
	gpioMstarFunc pFuncSetLow = NULL;
	gpioMstarFunc pFuncSetHigh = NULL;
	char *func_name_L = "MDrv_GPIO_Set_Low";
	char *func_name_H = "MDrv_GPIO_Set_High";
	int ret = -EIO;

	if (val) {
		pFuncSetHigh = (gpioMstarFunc)__symbol_get(func_name_H);
		if (pFuncSetHigh) {
			pFuncSetHigh(gpo);
			__symbol_put(func_name_H);
			ret = 0;
		}
	} else {
		pFuncSetLow = (gpioMstarFunc)__symbol_get(func_name_L);
		if (pFuncSetLow) {
			pFuncSetLow(gpo);
			__symbol_put(func_name_L);
			ret = 0;
		}
	}
	return ret;
#else
	return gpio_direction_output(gpo, val);
#endif

}

void resetHif_ResetGpioPull(uint32_t dongle_id)
{
	int i4Status;

	if (dongle_id >= MAX_DONGLE_NUM)
		return;

	if (!isResetGpioReleased[dongle_id]) {
		MR_Err("[%d] %s: reset gpio is already pulled\n",
			dongle_id, __func__);
		return;
	}
	if (!resetGpioInfo[dongle_id].flag_inited) {
		MR_Err("[%d] %s: reset gpio is unknown\n",
			dongle_id, __func__);
		return;
	}

	i4Status = resetHif_GpioOutput(resetGpioInfo[dongle_id].gpio_num,
					 resetGpioInfo[dongle_id].action_level);
	if (i4Status < 0) {
		MR_Err("[%d] %s: gpio %d set output %d failed, ret = %d\n",
		       dongle_id, __func__, resetGpioInfo[dongle_id].gpio_num,
		       resetGpioInfo[dongle_id].action_level, i4Status);
		return;
	}

	MR_Warn("[%d] %s: pull reset gpio (%d, %d)\n", dongle_id, __func__,
		resetGpioInfo[dongle_id].gpio_num,
		resetGpioInfo[dongle_id].action_level);
	isResetGpioReleased[dongle_id] = false;
}

void resetHif_ResetGpioRelease(uint32_t dongle_id)
{
	int i4Status;

	if (dongle_id >= MAX_DONGLE_NUM)
		return;

	if (isResetGpioReleased[dongle_id]) {
		MR_Err("[%d] %s: reset gpio is already released\n",
			dongle_id, __func__);
		return;
	}
	if (!resetGpioInfo[dongle_id].flag_inited) {
		MR_Err("[%d] %s: reset gpio is unknown\n", dongle_id, __func__);
		return;
	}

	i4Status = resetHif_GpioOutput(resetGpioInfo[dongle_id].gpio_num,
					resetGpioInfo[dongle_id].default_level);
	if (i4Status < 0) {
		MR_Err("[%d] %s: gpio %d set output %d failed, ret = %d\n",
		       dongle_id, __func__, resetGpioInfo[dongle_id].gpio_num,
		       resetGpioInfo[dongle_id].default_level, i4Status);
		return;
	}

	MR_Warn("[%d] %s: release reset gpio (%d, %d)\n", dongle_id, __func__,
		resetGpioInfo[dongle_id].gpio_num,
		resetGpioInfo[dongle_id].default_level);
	isResetGpioReleased[dongle_id] = true;
}

bool resetHif_isResetGpioReleased(uint32_t dongle_id)
{
	if (dongle_id >= MAX_DONGLE_NUM)
		return true;
	return isResetGpioReleased[dongle_id];
}

void resetHif_PowerGpioSwitchOn(uint32_t dongle_id)
{
	int i4Status;

	if (dongle_id >= MAX_DONGLE_NUM)
		return;

	if (isPowerSwitchOn[dongle_id]) {
		MR_Err("[%d] %s: power is already switched on\n",
			dongle_id, __func__);
		return;
	}
	if (!powerGpioInfo[dongle_id].flag_inited) {
		MR_Err("[%d] %s: power gpio is unknown\n", dongle_id, __func__);
		return;
	}

	i4Status = resetHif_GpioOutput(powerGpioInfo[dongle_id].gpio_num,
				      powerGpioInfo[dongle_id].switch_on_level);
	if (i4Status < 0) {
		MR_Err("[%d] %s: gpio %d set output %d failed, ret = %d\n",
		       dongle_id, __func__, powerGpioInfo[dongle_id].gpio_num,
		       powerGpioInfo[dongle_id].switch_on_level, i4Status);
		return;
	}

	MR_Warn("[%d] %s: power switch on (%d, %d)\n", dongle_id, __func__,
		powerGpioInfo[dongle_id].gpio_num,
		powerGpioInfo[dongle_id].switch_on_level);
	isPowerSwitchOn[dongle_id] = true;
}

void resetHif_PowerGpioSwitchOff(uint32_t dongle_id)
{
	int i4Status;

	if (dongle_id >= MAX_DONGLE_NUM)
		return;

	if (!isPowerSwitchOn[dongle_id]) {
		MR_Err("[%d] %s: power is already switched off\n",
			dongle_id, __func__);
		return;
	}
	if (!powerGpioInfo[dongle_id].flag_inited) {
		MR_Err("[%d] %s: power gpio is unknown\n", dongle_id, __func__);
		return;
	}

	i4Status = resetHif_GpioOutput(powerGpioInfo[dongle_id].gpio_num,
				     powerGpioInfo[dongle_id].switch_off_level);
	if (i4Status < 0) {
		MR_Err("[%d] %s: gpio %d set output %d failed, ret = %d\n",
		       dongle_id, __func__, powerGpioInfo[dongle_id].gpio_num,
		       powerGpioInfo[dongle_id].switch_off_level, i4Status);
		return;
	}

	MR_Warn("[%d] %s: power switch off (%d, %d)\n", dongle_id, __func__,
		powerGpioInfo[dongle_id].gpio_num,
		powerGpioInfo[dongle_id].switch_off_level);
	isPowerSwitchOn[dongle_id] = false;
}

bool resetHif_isPowerSwitchOn(uint32_t dongle_id)
{
	if (dongle_id >= MAX_DONGLE_NUM)
		return true;
	return isPowerSwitchOn[dongle_id];
}


