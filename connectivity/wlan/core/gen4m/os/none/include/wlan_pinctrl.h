/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _WLAN_PINCTRL_H
#define _WLAN_PINCTRL_H

enum WLAN_PINCTRL_MSG {
	WLAN_PINCTRL_MSG_FUNC_ON,
	WLAN_PINCTRL_MSG_FUNC_OFF,
	WLAN_PINCTRL_MSG_FUNC_PTA_UART_INIT,
	WLAN_PINCTRL_MSG_FUNC_PTA_UART_ON,
	WLAN_PINCTRL_MSG_FUNC_PTA_UART_OFF,
	WLAN_PINCTRL_MSG_FUNC_NUM,
};

struct WLAN_PINCTRL_OPS {
	int32_t (*init)(void);
	int32_t (*action)(enum WLAN_PINCTRL_MSG msg);
};

static inline int32_t wlan_pinctrl_init(struct mt66xx_chip_info *chip_info)
{
	if (!chip_info ||
	    !chip_info->pinctrl_ops ||
	    !chip_info->pinctrl_ops->init)
		return 0;

	return chip_info->pinctrl_ops->init();
}

static inline int32_t wlan_pinctrl_action(struct mt66xx_chip_info *chip_info,
	enum WLAN_PINCTRL_MSG msg)
{
	if (!chip_info ||
	    !chip_info->pinctrl_ops ||
	    !chip_info->pinctrl_ops->action)
		return 0;

	return chip_info->pinctrl_ops->action(msg);
}

#endif
