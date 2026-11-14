/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "btmtk_proj_ce.h"

static struct pinctrl *pinctrl_ptr = NULL;
static struct regulator *supply_pmic_en = NULL;

static inline int btmtk_pinctrl_exec(const char *name) {
	struct pinctrl_state *pinctrl;
	int ret = 0;
	BTMTK_INFO("%s start %s", __func__, name);
	if (IS_ERR(pinctrl_ptr)) {
		BTMTK_INFO("%s: fail to get bt pinctrl", __func__);
		return 0;
	}
	pinctrl = pinctrl_lookup_state(pinctrl_ptr, name);
	if (!IS_ERR(pinctrl)) {
		ret = pinctrl_select_state(pinctrl_ptr, pinctrl);
		if (ret) {
			BTMTK_ERR("%s: pinctrl %s fail [%d]", __func__, name, ret);
			return 0;
		}
	} else {
		BTMTK_INFO("%s: pinctrl %s is not exist", __func__, name);
		return 0;
	}
	BTMTK_INFO("%s end %s", __func__, name);
	return 0;
}

int btmtk_pre_power_on_handler(struct btmtk_uart_dev *cif_dev) {
	int ret = 0;

	BTMTK_INFO("%s start", __func__);

	ret = btmtk_pinctrl_exec(PRE_ON_PINCTRL_NAME);
	if (ret < 0) {
		BTMTK_INFO("%s: pinctrl %s fail", __func__, PRE_ON_PINCTRL_NAME);
	}

	/* reopen tty */
	if(cif_dev != NULL && cif_dev->tty != NULL && cif_dev->tty->port != NULL) {
		BTMTK_INFO("%s tty_port[%p], port_count[%d]",
			__func__, cif_dev->tty->port, cif_dev->tty->port->count);
		if (cif_dev->tty->port->count == 0)
			cif_dev->tty->ops->open(cif_dev->tty, NULL);
	}
	msleep(100);

	ret = btmtk_pinctrl_exec(POWER_ON_TX_PINCTRL_NAME);
	if (ret < 0) {
		BTMTK_INFO("%s: pinctrl %s fail", __func__, POWER_ON_TX_PINCTRL_NAME);
	}
	msleep(100);

	ret = btmtk_pinctrl_exec(RST_ON_PINCTRL_NAME);
	if (ret < 0) {
		BTMTK_INFO("%s: pinctrl %s fail", __func__, RST_ON_PINCTRL_NAME);
	}
	msleep(100);

	ret = btmtk_pinctrl_exec(RST_OFF_PINCTRL_NAME);
	if (ret < 0) {
		BTMTK_INFO("%s: pinctrl %s fail", __func__, RST_ON_PINCTRL_NAME);
	}
	msleep(100);

	ret = btmtk_pinctrl_exec(RST_ON_PINCTRL_NAME);
	if (ret < 0) {
		BTMTK_INFO("%s: pinctrl %s fail", __func__, RST_ON_PINCTRL_NAME);
	}
	msleep(100);

	if (supply_pmic_en) {
		ret = regulator_enable(supply_pmic_en);
		if (ret < 0) {
			BTMTK_ERR("%s: regulator enable %s fail", __func__, BT_PMIC_EN_REGULATOR);
			return ret;
		}
		msleep(500);
	} else {
		BTMTK_INFO("%s: pmic_en is NULL", __func__);
	}

	ret = btmtk_pinctrl_exec(POWER_ON_RX_PINCTRL_NAME);
	if (ret < 0) {
		BTMTK_INFO("%s: pinctrl %s fail", __func__, POWER_ON_RX_PINCTRL_NAME);
	}

	return 0;
}

int btmtk_set_gpio_default(void) {
	int ret = 0;

	BTMTK_INFO("%s start", __func__);

	if (supply_pmic_en) {
		ret = regulator_disable(supply_pmic_en);
		if (ret < 0) {
			BTMTK_ERR("%s: regulator disable %s fail", __func__, BT_PMIC_EN_REGULATOR);
			return ret;
		}
	} else {
		BTMTK_INFO("%s: pmic_en is NULL", __func__);
	}

	ret = btmtk_pinctrl_exec(RST_OFF_PINCTRL_NAME);
	if (ret < 0) {
		BTMTK_INFO("%s: pinctrl %s fail", __func__, RST_OFF_PINCTRL_NAME);
	}
	msleep(10);

	ret = btmtk_pinctrl_exec(INIT_STATE_PINCTRL_NAME);
	if (ret < 0) {
		BTMTK_INFO("%s: pinctrl %s fail", __func__, INIT_STATE_PINCTRL_NAME);
	}
	return 0;
}

int btmtk_set_uart_rx_aux(void) {
	BTMTK_INFO("%s start", __func__);
	return btmtk_pinctrl_exec(POWER_ON_RX_PINCTRL_NAME);
}

int btmtk_ce_gpio_init(struct btmtk_uart_dev *cif_dev) {
	BTMTK_INFO("%s start", __func__);

	if (cif_dev == NULL) {
		BTMTK_ERR("%s: cif_dev is NULL", __func__);
		return -1;
	}

	if (pinctrl_ptr == NULL) {
		BTMTK_INFO("%s init pinctrl", __func__);
		cif_dev->tty->dev->of_node = of_find_compatible_node(NULL, NULL, "mediatek,bt");
		if (!cif_dev->tty->dev->of_node) {
			BTMTK_INFO("%s: mediatek,bt of_node not found", __func__);
		} else {
			pinctrl_ptr = devm_pinctrl_get(cif_dev->tty->dev);
			if (IS_ERR(pinctrl_ptr)) {
				BTMTK_INFO("%s: fail to get bt pinctrl", __func__);
			}
		}
	}

	if (supply_pmic_en == NULL) {
		BTMTK_INFO("%s init regulator", __func__);
		supply_pmic_en = regulator_get(cif_dev->tty->dev, BT_PMIC_EN_REGULATOR);
		if (!supply_pmic_en) {
			BTMTK_INFO("%s: fail to get bt pmic_en regulator", __func__);
		}
	}
	return 0;
}

int btmtk_ce_init(void) {
	BTMTK_INFO("%s start", __func__);
	btmtk_init_node();
	return 0;
}


int btmtk_ce_subsys_reset(struct btmtk_dev *bdev) {
	int ret = 0;

	BTMTK_INFO("%s start", __func__);

	ret = btmtk_pinctrl_exec(RST_OFF_PINCTRL_NAME);
	if (ret < 0) {
		BTMTK_INFO("%s: pinctrl %s fail", __func__, RST_OFF_PINCTRL_NAME);
	}
	msleep(100);

	ret = btmtk_pinctrl_exec(RST_ON_PINCTRL_NAME);
	if (ret < 0) {
		BTMTK_INFO("%s: pinctrl %s fail", __func__, RST_ON_PINCTRL_NAME);
	}
	msleep(100);

	return ret;
}