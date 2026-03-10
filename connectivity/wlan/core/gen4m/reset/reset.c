/******************************************************************************
 *
 * This file is provided under a dual license.  When you use or
 * distribute this software, you may choose to be licensed under
 * version 2 of the GNU General Public License ("GPLv2 License")
 * or BSD License.
 *
 * GPLv2 License
 *
 * Copyright(C) 2017 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * BSD LICENSE
 *
 * Copyright(C) 2017 MediaTek Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/
/*! \file   reset.c
*   \brief  reset module
*
*    This file contains all implementations of reset module
*/


/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/workqueue.h>
#include <linux/module.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/sdio_func.h>/* sdio_readl(), etc */
#include <linux/mmc/host.h>		/* mmc_add_host(), etc */
#include <linux/mmc/sdio_ids.h>
#include <linux/errno.h>
#include "precomp.h"

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/
#define RST_STATUS_BT_PROBE_DONE         BIT(0)
#define RST_STATUS_WIFI_PROBE_DONE       BIT(1)

/**
* For chip reset pin set low time
*/
#define RESET_PIN_SET_LOW_TIME		500  /* in unit of ms */

/**
 * For chip reset pin number configureation
 */
#define WIFI_DONGLE_RESET_GPIO_PIN	220

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

enum ENUM_RST_STATE_TYPE_T {
	RST_STATE_UNKNOWN = 0,
	RST_STATE_IDLE,
	RST_STATE_START,
	RST_STATE_GOING,
	RST_STATE_MAX
};

struct WHOLE_CHIP_RESET_STRUCT {
	struct mmc_host *prHost;
	struct mutex rResetMutex;
	enum ENUM_RST_STATE_TYPE_T eResetState;
};

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

MODULE_LICENSE("Dual BSD/GPL");

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/
static int32_t g_resetFlag;
u_int8_t fgBtProbed = FALSE;
u_int8_t fgWiFiProbed = FALSE;
u_int8_t fgWaitResetDone = FALSE;
static struct WHOLE_CHIP_RESET_STRUCT g_prResetInfo;
/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

/*----------------------------------------------------------------------------*/
/*!
 * @brief Set L0 reset state
 *
 * @param   state
 *
 * @retval  none
 */
/*----------------------------------------------------------------------------*/
void resetSetState(enum ENUM_RST_STATE_TYPE_T eNextstate)
{
	static const char *const apcState[RST_STATE_MAX] = {
		"RST_STATE_UNKNOWN",
		"RST_STATE_IDLE",
		"RST_STATE_START",
		"RST_STATE_GOING",
	};

	MR_Dbg("%s: current_state[%s], next_state[%s]\n", __func__,
		apcState[g_prResetInfo.eResetState],
		apcState[eNextstate]);

	if (eNextstate >= RST_STATE_MAX) {
		MR_Err("%s: [SER][L0] unsupported L0 reset state\n",
			__func__);
	} else {
		mutex_lock(&g_prResetInfo.rResetMutex);
		if (g_prResetInfo.eResetState != eNextstate)
			g_prResetInfo.eResetState = eNextstate;
		mutex_unlock(&g_prResetInfo.rResetMutex);
	}
}


int resetGetState(void)
{
	enum ENUM_RST_STATE_TYPE_T eCurrstate;

	mutex_lock(&g_prResetInfo.rResetMutex);
	eCurrstate = g_prResetInfo.eResetState;
	mutex_unlock(&g_prResetInfo.rResetMutex);

	return eCurrstate;
}


#if defined(_HIF_USB)
void resetUsbTogglePin(void)
{
	struct device_node *node;
	uint32_t gpio_num = 0;
	int32_t i4Status = 0;

	node = of_find_compatible_node(NULL, NULL, "mediatek,mtk-wifi-reset");
	gpio_num = of_get_named_gpio(node, "wifireset-gpios", 0);

	i4Status = gpio_request(gpio_num, "wifireset-gpios");
	MR_Err("[SER][L0]%s: Invoke gpio_request(%d,%d)\n", __func__,
			gpio_num, i4Status);
	i4Status = gpio_direction_output(gpio_num, 0);
	MR_Err("[SER][L0]%s: Invoke gpio_direction_output 0(%d,%d)\n", __func__,
			gpio_num, i4Status);
	mdelay(RESET_PIN_SET_LOW_TIME);
	i4Status = gpio_direction_output(gpio_num, 1);
	MR_Err("[SER][L0]%s: Invoke gpio_direction_output 1(%d,%d)\n", __func__,
			gpio_num, i4Status);
	gpio_free(gpio_num);
}
#endif

#if defined(_HIF_SDIO)
void resetSdioTogglePin(void)
{
	struct mmc_host *host;

	host = g_prResetInfo.prHost;
	host->rescan_entered = 0;

	MR_Err("[SER][L0] mmc_remove_host\n");
	mmc_remove_host(host);
	MR_Err("[SER][L0] mmc_add_host\n");
	mmc_add_host(host);
}
#endif

static void resetTogglePin(void)
{
	MR_Dbg("%s: toggle reset pin\n", __func__);

	resetSetState(RST_STATE_GOING);

#if defined(_HIF_USB)
	resetUsbTogglePin();
#elif defined(_HIF_SDIO)
	resetSdioTogglePin();
#endif
}

/*----------------------------------------------------------------------------*/
/*!
* \brief export function for set reset status for WiFi/BT
*
* \param[in] module connectivity module.
* \param[in] status reset status
* \param[in] SDIO host card information
*
* \retval void
*/
/*----------------------------------------------------------------------------*/
void rstNotifyWholeChipRstStatus(enum ENUM_RST_MODULE_TYPE_T module,
				enum ENUM_RST_MODULE_STATE_TYPE_T status,
				void *data)
{
	struct sdio_func *func = (struct sdio_func *)data;

	static const char *const apcModule[RST_MODULE_MAX] = {
		"BT",
		"WIFI",
	};
	static const char *const apcStatus[RST_MODULE_STATE_MAX] = {
		"RST_MODULE_STATE_PRERESET",
		"RST_MODULE_STATE_KO_INSMOD",
		"RST_MODULE_STATE_KO_RMMOD",
		"RST_MODULE_STATE_PROBE_START",
		"RST_MODULE_STATE_PROBE_DONE",
	};


	MR_Dbg("%s: module[%s], status[%s]\n", __func__,
		apcModule[module], apcStatus[status]);

#if defined(_HIF_SDIO)
	if (func)
		g_prResetInfo.prHost = func->card->host;
#endif

	switch (status) {
	case RST_MODULE_STATE_PRERESET:
		if (resetGetState() == RST_STATE_IDLE) {
			goto TOGGLE_PIN;
		} else if (fgBtProbed == FALSE
			|| fgWiFiProbed == FALSE) {
			MR_Dbg("WiFi or BT not probe start\n");
		} else {
			MR_Dbg("Wait previous reset done\n");
			fgWaitResetDone = TRUE;
		}
		break;

	case RST_MODULE_STATE_KO_INSMOD:
		if (resetGetState() == RST_STATE_UNKNOWN)
			resetSetState(RST_STATE_IDLE);
		break;

	case RST_MODULE_STATE_KO_RMMOD:
		if (module == RST_MODULE_BT)
			fgBtProbed = FALSE;
		else if (module == RST_MODULE_WIFI)
			fgWiFiProbed = FALSE;

		if (fgBtProbed && fgWiFiProbed)
			resetSetState(RST_STATE_IDLE);

		break;

	case RST_MODULE_STATE_PROBE_START:
		if (module == RST_MODULE_BT)
			fgBtProbed = TRUE;
		else if (module == RST_MODULE_WIFI)
			fgWiFiProbed = TRUE;

		break;

	case RST_MODULE_STATE_PROBE_DONE:
		if (module == RST_MODULE_BT)
			g_resetFlag |= RST_STATUS_BT_PROBE_DONE;
		else if (module == RST_MODULE_WIFI)
			g_resetFlag |= RST_STATUS_WIFI_PROBE_DONE;

		if (fgBtProbed && fgWiFiProbed) {
			if (g_resetFlag & RST_STATUS_WIFI_PROBE_DONE
				&& g_resetFlag & RST_STATUS_BT_PROBE_DONE)
				resetSetState(RST_STATE_IDLE);
		} else {
			if (fgBtProbed) {
				if (g_resetFlag & RST_STATUS_BT_PROBE_DONE)
					resetSetState(RST_STATE_IDLE);
			}

			if (fgWiFiProbed) {
				if (g_resetFlag & RST_STATUS_WIFI_PROBE_DONE)
					resetSetState(RST_STATE_IDLE);
			}
		}

		if (resetGetState() == RST_STATE_IDLE
			&& fgWaitResetDone == TRUE) {
			fgWaitResetDone = FALSE;
			goto TOGGLE_PIN;
		}

		break;

	default:
		/* Make sure we have handle all STATEs */
		break;
	}

	return;

TOGGLE_PIN:
	g_resetFlag &= ~RST_STATUS_BT_PROBE_DONE;
	g_resetFlag &= ~RST_STATUS_WIFI_PROBE_DONE;

	resetSetState(RST_STATE_START);
	resetTogglePin();
}
EXPORT_SYMBOL(rstNotifyWholeChipRstStatus);

static int __init resetInit(void)
{
	MR_Err("%s\n", __func__);

	mutex_init(&g_prResetInfo.rResetMutex);
	g_prResetInfo.eResetState = RST_STATE_UNKNOWN;

	return 0;
}

static void __exit resetExit(void)
{
	MR_Dbg("%s\n", __func__);
}

module_init(resetInit);
module_exit(resetExit);
