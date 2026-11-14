/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _BTMTK_PROJ_CE_H_
#define _BTMTK_PROJ_CE_H_

#include "btmtk_uart_tty.h"

#define INIT_STATE_PINCTRL_NAME	("bt_ce_gpio_init")                 /* UART TX/RX: gpio mode, pull down */
#define PRE_ON_PINCTRL_NAME		("bt_ce_gpio_pre_on")               /* UART TX/RX: gpio mode, pull up */
#define POWER_ON_TX_PINCTRL_NAME	("bt_ce_uart_tx_aux")           /* UART TX: uart mode */
#define POWER_ON_RX_PINCTRL_NAME	("bt_ce_uart_rx_aux")           /* UART RX: uart mode */
#define RST_ON_PINCTRL_NAME		("bt_rst_on")                       /* BT RST: gpio mode,  output high */
#define RST_OFF_PINCTRL_NAME		("bt_rst_off")                  /* BT RST: gpio mode,  output low */
#define FALCON_PMIC_DEFAULT_PINCTRL_NAME ("falcon_pmic_en_default") /* Optinal. PMIC: disable */
#define FALCON_PMIC_SET_PINCTRL_NAME ("falcon_pmic_en_set")         /* Optinal. PMIC: enable */

#define BT_PMIC_EN_REGULATOR "bt1v8"

#if (USE_DEVICE_NODE == 0)
int btmtk_set_gpio_default(void);
int btmtk_pre_power_on_handler(struct btmtk_uart_dev *cif_dev);
int btmtk_ce_gpio_init(struct btmtk_uart_dev *cif_dev);
int btmtk_ce_init(void);
int btmtk_ce_subsys_reset(struct btmtk_dev *bdev);
#endif // (USE_DEVICE_NODE == 0)
#endif
