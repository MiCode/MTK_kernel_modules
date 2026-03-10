/**
 *  Copyright (c) 2018 MediaTek Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */
#include <linux/gpio/consumer.h>
#include <linux/of_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_device.h>
#include "btmtk_main.h"
#include "conninfra.h"
#include "connfem.h"
#include "btmtk_proj_sp.h"
#include "btmtk_uart_tty.h"
#include "btmtk_fw_log.h"
#include <linux/platform_device.h>

#if IS_ENABLED(CONFIG_PMIC_LBAT_SERVICE) && LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)
#include "pmic_lbat_service.h"
#endif

#define READ_PMIC_STATE_CMD_LEN		16
#define READ_PMIC_STATE_EVENT_LEN	16

#define DEFAULT_STATE_PINCTRL_NAME	("bt_combo_gpio_init")
#define PRE_ON_PINCTRL_NAME		("bt_combo_gpio_pre_on")
#define POWER_ON_TX_PINCTRL_NAME	("bt_combo_uart_tx_aux")
#define POWER_ON_RX_PINCTRL_NAME	("bt_combo_uart_rx_aux")
#define RST_ON_PINCTRL_NAME		("bt_rst_on")
#define RST_OFF_PINCTRL_NAME		("bt_rst_off")
#define INIT_STATE_PINCTRL_NAME		("bt_combo_gpio_init")
#define BTMTK_UART_NAME			("btmtk_uart")
#define BT_FIND_MY_PHONE_HIGH	("bt-find-my-phone-high")
#define BT_FIND_MY_PHONE_LOW	("bt-find-my-phone-low")


#if (USE_DEVICE_NODE == 1)
static struct pinctrl *pinctrl_ptr;
extern struct btmtk_dev *g_sbdev;
int g_bt_state;

/* dump gpio */
static void __iomem *vir_0x1000_5000 = NULL; /* GPIO */
#define CONSYS_REG_READ(addr) (*((volatile unsigned int *)(addr)))

extern void btmtk_uart_trigger_assert_by_tx_thread(struct btmtk_dev *bdev);
static void btmtk_dump_gpio_state(void);

static inline int btmtk_pinctrl_exec(const char *name);

static int __maybe_unused btmtk_char_suspend(struct device *dev)
{
	//BTMTK_INFO("%s", __func__);
	g_sbdev->suspend_state = TRUE;
	return 0;
}

static int __maybe_unused btmtk_char_resume(struct device *dev)
{
	//BTMTK_INFO("%s", __func__);
	g_sbdev->suspend_state = FALSE;
	return 0;
}

static int __maybe_unused btmtk_char_runtime_suspend(struct device *dev)
{
	//BTMTK_INFO("%s", __func__);
	return 0;
}

static int __maybe_unused btmtk_char_runtime_resume(struct device *dev)
{
	//BTMTK_INFO("%s", __func__);
	return 0;
}


static const struct dev_pm_ops btmtk_char_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(btmtk_char_suspend, btmtk_char_resume)
	SET_RUNTIME_PM_OPS(btmtk_char_runtime_suspend, btmtk_char_runtime_resume, NULL)
};

static struct platform_device *btmtk_uart_device;

static int btmtk_uart_driver_probe(struct platform_device *pdev) {
	BTMTK_DBG("%s", __func__);
	btmtk_uart_device = pdev;
	return 0;
}

static int btmtk_uart_driver_remove(struct platform_device *pdev) {
	BTMTK_DBG("%s", __func__);
	return 0;
}

static struct platform_driver btmtk_uart_driver = {
	.driver = {
		.name	= BTMTK_UART_NAME,
		.owner	= THIS_MODULE,
		.pm		= &btmtk_char_pm_ops,
	},
	.probe = btmtk_uart_driver_probe,
	.remove = btmtk_uart_driver_remove,
};

static struct platform_device *btmtk_uart_device;


void btmtk_platform_driver_init(void) {
	int ret = 0;

	BTMTK_DBG("%s", __func__);
	ret = platform_driver_register(&btmtk_uart_driver);
	BTMTK_INFO("%s: platform_driver_register ret = %d", __func__, ret);
	btmtk_uart_device = platform_device_alloc(BTMTK_UART_NAME, 0);
	if (btmtk_uart_device == NULL) {
		platform_driver_unregister(&btmtk_uart_driver);
		BTMTK_ERR("%s: platform_device_alloc device fail", __func__);
		return;
	}
	ret = platform_device_add(btmtk_uart_device);
	if (ret) {
		platform_driver_unregister(&btmtk_uart_driver);
		BTMTK_ERR("%s: platform_device_add fail", __func__);
		return;
	}

}

void btmtk_platform_driver_deinit(void) {
	BTMTK_INFO("%s", __func__);
	platform_driver_unregister(&btmtk_uart_driver);
}

void btmtk_dev_link_uart(void)
{
	unsigned int dl_flags = 0;
	struct btmtk_uart_dev *cif_dev = NULL;
	struct device_link	*link;

	BTMTK_DBG("%s", __func__);
	if (!g_sbdev) {
		BTMTK_ERR("%s: g_sbdev is NULL", __func__);
		return;
	}

	cif_dev = (struct btmtk_uart_dev *)g_sbdev->cif_dev;

	if (!cif_dev) {
		BTMTK_ERR("%s: cif_dev is NULL", __func__);
		return;
	}

	if (!cif_dev->tty) {
		BTMTK_ERR("%s: tty is NULL", __func__);
		return;
	}
	BTMTK_INFO("%s: tty addr[%p]", __func__, cif_dev->tty);
	if (!cif_dev->tty->dev) {
		BTMTK_ERR("%s: tty->dev is NULL", __func__);
		return;
	}

	if (btmtk_uart_device == NULL) {
		BTMTK_ERR("%s: btmtk_uart_device is NULL", __func__);
		return;
	}

	/* set up dl_flags */
	dl_flags = DL_FLAG_PM_RUNTIME | DL_FLAG_STATELESS;

	/* create device link by device_link_add */
	link = device_link_add(&btmtk_uart_device->dev, cif_dev->tty->dev, dl_flags);

	if (!link) {
		BTMTK_WARN("%s: uart device link add fail. probe fail", __func__);
		return;
	}
	/* supplier is not probed */
	if (link->status == DL_STATE_DORMANT) {
		BTMTK_WARN("%s: uart is not probed", __func__);
		return;
	}
}

#if IS_ENABLED(CONFIG_MTK_UARTHUB)
void btmtk_uarthub_err_cb(unsigned int err_type)
{
	struct btmtk_uart_dev *cif_dev = NULL;
	int state;

	BTMTK_INFO("%s: err_type[%d]", __func__, err_type);

	if (g_sbdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return;
	}

	state = btmtk_get_chip_state(g_sbdev);

	if (state != BTMTK_STATE_WORKING) {
		BTMTK_ERR("%s: state[%d] is not working", __func__, state);
		return;
	}

	cif_dev = (struct btmtk_uart_dev *)g_sbdev->cif_dev;
	if (cif_dev == NULL) {
		BTMTK_ERR("%s: cif_dev is NULL", __func__);
		return;
	}

	if (cif_dev->tty  == NULL) {
		BTMTK_ERR("%s: tty is NULL", __func__);
		return;
	}

	if (((1 << dev0_tx_timeout_err) | (1 << dev0_tx_pkt_type_err) | (1 << dev0_rx_timeout_err)
			| (1 << intfhub_dev0_tx_err) | (1 << rx_pkt_type_err)) & err_type) {
		BTMTK_INFO("%s: dev0 err dump", __func__);
		if (cif_dev->hub_en && btmtk_get_chip_state(g_sbdev) != BTMTK_STATE_DISCONNECT)
			mtk8250_uart_dump(cif_dev->tty);
	}

	if (((1 << rx_pkt_type_err) | (1 << dev0_rx_timeout_err)) & err_type) {
		if (g_sbdev->assert_reason[0] == '\0') {
			strncpy(g_sbdev->assert_reason, "[BT_DRV assert] uarthub rx error",
					strlen("[BT_DRV assert] uarthub rx error") + 1);
			BTMTK_ERR("%s: [assert_reason] %s", __func__, g_sbdev->assert_reason);
		}

		btmtk_uart_trigger_assert_by_tx_thread(g_sbdev);
	}
	return;
}

int btmtk_wakeup_uarthub(void) {
	int ready_retry = 50, ret = 0;
	struct btmtk_uart_dev *cif_dev = NULL;

	if (!g_sbdev) {
		BTMTK_ERR("%s: cif_dev is NULL", __func__);
		return -1;
	}

	cif_dev = (struct btmtk_uart_dev *)g_sbdev->cif_dev;
	if (!cif_dev) {
		BTMTK_ERR("%s: cif_dev is NULL", __func__);
		return -1;
	}

	/* Set TX,RX request */
	ret = mtk8250_uart_hub_dev0_set_tx_request(cif_dev->tty);
	BTMTK_DBG("%s mtk8250_uart_hub_dev0_set_tx_request, ret[%d]", __func__, ret);
	if (ret) {
		BTMTK_ERR("%s mtk8250_uart_hub_dev0_set_tx_request fail ret[%d]", __func__, ret);
		return -1;
	}

	/* Polling UARTHUB is ready state */
	while (mtk8250_uart_hub_is_ready() <= 0 && --ready_retry) {
		BTMTK_WARN("%s ready_retry[%d]", __func__, ready_retry);
		usleep_range(1000, 1100);
	}

	if (ready_retry <= 0) {
		ret = mtk8250_uart_hub_dump_with_tag("BT driver own");
		BTMTK_ERR("%s mtk8250_uart_hub_dump_with_tag ready_retry[%d] ret[%d]", __func__, ready_retry, ret);
		return -1;
	}

	return 0;
}

void btmtk_release_uarthub(bool force)
{
	int ret = -1;
	struct btmtk_uart_dev *cif_dev = NULL;

	BTMTK_DBG("%s: start", __func__);

	if (!g_sbdev) {
		BTMTK_ERR("%s: cif_dev is NULL", __func__);
		return;
	}

	cif_dev = (struct btmtk_uart_dev *)g_sbdev->cif_dev;
	if (!cif_dev) {
		BTMTK_ERR("%s: cif_dev is NULL", __func__);
		return;
	}
	/* force is for only bt off flow */
	if (cif_dev->hub_en && force) {
		/* set tx/rx gpio PU */
		btmtk_pinctrl_exec(PRE_ON_PINCTRL_NAME);
		ret = mtk8250_uart_hub_dev0_clear_tx_request();
		BTMTK_DBG("%s mtk8250_uart_hub_dev0_clear_tx_request ret[%d]", __func__, ret);
	}

	/* Clr TX,RX request, let uarthub can sleep */
	if (cif_dev->hub_en && (cif_dev->sleep_en || force)) {
		ret =  mtk8250_uart_hub_dev0_clear_rx_request(cif_dev->tty);
		BTMTK_DBG("%s mtk8250_uart_hub_dev0_clear_rx_request ret[%d]", __func__, ret);
		if (ret)
			BTMTK_ERR("%s mtk8250_uart_hub_dev0_clear_rx_request fail ret[%d]", __func__, ret);
	}

	return;
}


#endif

void btmtk_sp_coredump_start(void)
{

	struct btmtk_uart_dev *cif_dev = NULL;

	if (!g_sbdev) {
		BTMTK_ERR("%s: g_sbdev is NULL", __func__);
		return;
	}

	cif_dev = (struct btmtk_uart_dev *)g_sbdev->cif_dev;

	if (!cif_dev) {
		BTMTK_ERR("%s: cif_dev is NULL", __func__);
		return;
	}
	BTMTK_INFO("%s: set bt assert_state[1]", __func__);
	atomic_set(&g_sbdev->assert_state, BTMTK_ASSERT_START);

#if IS_ENABLED(CONFIG_MTK_UARTHUB)
	/* uarthub assert bit to avoid MD/ADSP send data */
	if (cif_dev->hub_en && btmtk_get_chip_state(g_sbdev) != BTMTK_STATE_DISCONNECT) {
		mtk8250_uart_hub_assert_bit_ctrl(1);
		BTMTK_DBG("%s mtk8250_uart_hub_assert_bit_ctrl(1)", __func__);
		mtk8250_uart_dump(cif_dev->tty);
	}
#endif

}

void btmtk_sp_coredump_end(void)
{
	struct btmtk_uart_dev *cif_dev = NULL;
	int ret = 0;
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();

	BTMTK_INFO("%s: start", __func__);

	if (!g_sbdev) {
		BTMTK_ERR("%s: cif_dev is NULL", __func__);
		return;
	}

	cif_dev = (struct btmtk_uart_dev *)g_sbdev->cif_dev;
	if (!cif_dev) {
		BTMTK_ERR("%s: cif_dev is NULL", __func__);
		return;
	}
	if(g_sbdev->collect_fwdump)
		ret = connv3_coredump_end(bmain_info->hif_hook.coredump_handler, g_sbdev->assert_reason);

	if (ret)
		BTMTK_ERR("%s: coredump_end fail ret[%d]", __func__, ret);

	atomic_set(&g_sbdev->assert_state, BTMTK_ASSERT_END);

	btmtk_fwdump_wake_unlock();

#if IS_ENABLED(CONFIG_MTK_UARTHUB)
	/* uarthub reset */
	if (cif_dev->hub_en) {
		mtk8250_uart_hub_assert_bit_ctrl(0);
		BTMTK_DBG("%s mtk8250_uart_hub_assert_bit_ctrl(0)", __func__);
		mtk8250_uart_hub_reset();
		BTMTK_INFO("%s mtk8250_uart_hub_reset", __func__);
	}
#endif
}

static inline int btmtk_pinctrl_exec(const char *name)
{
	struct pinctrl_state *pinctrl;
	int ret = -1;

	BTMTK_INFO("%s start %s", __func__, name);
	if (IS_ERR(pinctrl_ptr)) {
		BTMTK_ERR("[ERR] %s: fail to get bt pinctrl", __func__);
		return -1;
	}

	pinctrl = pinctrl_lookup_state(pinctrl_ptr, name);
	if (!IS_ERR(pinctrl)) {
		ret = pinctrl_select_state(pinctrl_ptr, pinctrl);
		if (ret) {
			BTMTK_ERR("%s: pinctrl %s fail [%d]", __func__, name, ret);
			return -1;
		}
	} else {
		BTMTK_ERR("%s: pinctrl %s lookup fail", __func__, name);
		return -1;
	}

	return 0;
}

int btmtk_pre_power_on_handler(void)
{
	/*
	 * Set BT_RST PU/OUPUT
	 * Setup BT UART
	 */
	int ret = 0;
	struct btmtk_uart_dev *cif_dev = NULL;

	BTMTK_DBG("%s: start", __func__);

	if (!g_sbdev) {
		BTMTK_ERR("%s: cif_dev is NULL", __func__);
		return -1;
	}

	cif_dev = (struct btmtk_uart_dev *)g_sbdev->cif_dev;
	if (!cif_dev) {
		BTMTK_ERR("%s: cif_dev is NULL", __func__);
		return -1;
	}

	if (btmtk_get_chip_state(g_sbdev) == BTMTK_STATE_DISCONNECT) {
		BTMTK_WARN("%s: uart disconnected", __func__);
		return -1;
	}

	if (cif_dev->is_pre_on_done) {
		BTMTK_INFO("%s: already do pre_on_cb", __func__);
		return 0;
	}

	ret = btmtk_pinctrl_exec(PRE_ON_PINCTRL_NAME);

#if IS_ENABLED(CONFIG_MTK_UARTHUB)

	/* use uarthub multi-host mode (default) */
	ret = mtk8250_uart_hub_enable_bypass_mode(0);
	BTMTK_INFO("%s mtk8250_uart_hub_enable_bypass_mode(0) ret[%d]", __func__, ret);

	ret = mtk8250_uart_hub_dev0_set_rx_request();
	BTMTK_DBG("%s mtk8250_uart_hub_dev0_set_rx_request ret[%d]", __func__, ret);
	ret = btmtk_wakeup_uarthub();

	if(ret < 0)
		return ret;

	ret = mtk8250_uart_hub_reset_flow_ctrl();
	BTMTK_DBG("%s mtk8250_uart_hub_reset_flow_ctrl ret[%d]", __func__, ret);

	/* disable ADSP,MD when fw dl */
	ret = mtk8250_uart_hub_fifo_ctrl(1);
	BTMTK_DBG("%s: Set mtk8250_uart_hub_fifo_ctrl(1) ret[%d]", __func__, ret);

	/* use uarthub bypass mode*/
	ret = mtk8250_uart_hub_enable_bypass_mode(1);
	BTMTK_INFO("%s mtk8250_uart_hub_enable_bypass_mode(1) ret[%d]", __func__, ret);

#endif
	/* reopen tty */
	if(cif_dev != NULL && cif_dev->tty != NULL && cif_dev->tty->port != NULL
		&& g_sbdev->is_pre_cal_done) {
		BTMTK_INFO("%s tty_port[%p], port_count[%d]",
				__func__, cif_dev->tty->port, cif_dev->tty->port->count);
		if (cif_dev->tty->port->count == 0)
			cif_dev->tty->ops->open(cif_dev->tty, NULL);
	}

	btmtk_pinctrl_exec(POWER_ON_TX_PINCTRL_NAME);
	btmtk_pinctrl_exec(RST_ON_PINCTRL_NAME);
	btmtk_dump_gpio_state();

	cif_dev->is_pre_on_done = TRUE;
	BTMTK_DBG("%s: is_pre_on_done true", __func__);

	return 0;
}

int btmtk_set_uart_rx_aux(void)
{
	BTMTK_DBG("%s: start", __func__);
	return btmtk_pinctrl_exec(POWER_ON_RX_PINCTRL_NAME);
}

int btmtk_set_gpio_default(void)
{
	struct btmtk_uart_dev *cif_dev = NULL;
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();

	BTMTK_DBG("%s: start", __func__);

	if (g_sbdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return -1;
	}
	cif_dev = (struct btmtk_uart_dev *)g_sbdev->cif_dev;
	if (!cif_dev) {
		BTMTK_ERR("%s: cif_dev is NULL", __func__);
		return -1;
	}

	btmtk_pinctrl_exec(RST_OFF_PINCTRL_NAME);
	btmtk_dump_gpio_state();
	msleep(10);
	if(!bmain_info->find_my_phone_mode)
		btmtk_pinctrl_exec(DEFAULT_STATE_PINCTRL_NAME);
	else
		BTMTK_INFO("%s: into find my phone mode, skip set tx/rx gpio PD", __func__);

	return 0;
}

/* for bt close flow, add tty close */
int btmtk_set_gpio_default_for_close(void)
{
	struct btmtk_uart_dev *cif_dev = NULL;
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();

	BTMTK_DBG("%s: start", __func__);

	if (g_sbdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return -1;
	}
	cif_dev = (struct btmtk_uart_dev *)g_sbdev->cif_dev;
	if (!cif_dev) {
		BTMTK_ERR("%s: cif_dev is NULL", __func__);
		return -1;
	}
	cif_dev->is_pre_on_done = FALSE;
	BTMTK_DBG("%s: is_pre_on_done false", __func__);

	btmtk_pinctrl_exec(RST_OFF_PINCTRL_NAME);
	btmtk_dump_gpio_state();
	msleep(50);
	cif_dev->tty->ops->close(cif_dev->tty, NULL);
	if(!bmain_info->find_my_phone_mode)
		btmtk_pinctrl_exec(DEFAULT_STATE_PINCTRL_NAME);
	else
		BTMTK_INFO("%s: into find my phone mode, skip set tx/rx gpio PD", __func__);

	return 0;
}


static int btmtk_power_on_notify_handler(void)
{
	/* Execute BT power on then power off (if BT is off before this callback */
	BTMTK_INFO("%s", __func__);

	/* Don't block caller thread for connv3 API */
	schedule_work(&g_sbdev->pwr_on_uds_work);
	return 0;
}

int btmtk_sp_whole_chip_reset(struct btmtk_dev *bdev)
{
	BTMTK_DBG("%s: bt_state[%d]", __func__, g_bt_state);

	/* happen when whole chip reset is triggered by fw node */
	if (g_sbdev->assert_reason[0] == '\0') {
		memset(g_sbdev->assert_reason, 0, ASSERT_REASON_SIZE);
		strncpy(g_sbdev->assert_reason, "[BT_DRV assert] BT whole chip reset"
			, strlen("[BT_DRV assert] BT whole chip reset") + 1);
	}

	return connv3_trigger_whole_chip_rst(CONNV3_DRV_TYPE_BT , g_sbdev->assert_reason);
}

int btmtk_sp_close(void)
{
	struct btmtk_uart_dev *cif_dev = NULL;

	BTMTK_INFO("%s enter!", __func__);
	if (g_sbdev == NULL) {
		BTMTK_ERR("%s, bdev is NULL", __func__);
		return -EINVAL;
	}

	cif_dev = (struct btmtk_uart_dev *)g_sbdev->cif_dev;
	if (cif_dev == NULL) {
		BTMTK_ERR("%s, cif_dev is NULL", __func__);
		return -EINVAL;
	}

	/* wait coredump end */
	if (atomic_read(&g_sbdev->assert_state) == BTMTK_ASSERT_START) {
		BTMTK_WARN("%s: wait dump_comp , can't close yet", __func__);
		if (!wait_for_completion_timeout(&g_sbdev->dump_comp, msecs_to_jiffies(WAIT_FW_DUMP_TIMEOUT))) {
			BTMTK_ERR("%s: uanble to finish dump_comp in 15s", __func__);
			btmtk_sp_coredump_end();
		}
	}
	if (atomic_read(&g_sbdev->assert_state) == BTMTK_ASSERT_START) {
		BTMTK_WARN("%s: coredump not complete and without wait 15s", __func__);
		btmtk_sp_coredump_end();
	}

	cancel_work_sync(&g_sbdev->reset_waker);

#if IS_ENABLED(CONFIG_MTK_UARTHUB)
	btmtk_release_uarthub(true);
#endif
	btmtk_set_gpio_default_for_close();

	return 0;
}


static int btmtk_pre_chip_rst_handler(enum connv3_drv_type drv, char *reason)
{
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();
	unsigned char fstate = BTMTK_FOPS_STATE_INIT;

	if (g_sbdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return -1;
	}

	if(bmain_info->hif_hook.trigger_assert == NULL) {
		BTMTK_ERR("%s: hif_hook.trigger_assert is NULL", __func__);
		return -1;
	}

	g_bt_state = btmtk_get_chip_state(g_sbdev);
	fstate = btmtk_fops_get_state(g_sbdev);
	/* let bt close not send power off cmd */
	bmain_info->reset_stack_flag = HW_ERR_CODE_CHIP_RESET;

	/* Ask FW to do coredump */
	BTMTK_ERR("%s: state[%d], reason[%s]", __func__, g_bt_state, reason);
	if (fstate == BTMTK_FOPS_STATE_CLOSED) {
		BTMTK_WARN("%s: BT fops is closed, no need to trigger whole chip reset", __func__);
		return 0;
	} else if (drv == CONNV3_DRV_TYPE_CONNV3 && strncmp(reason, "PMIC Fault", strlen("PMIC Fault")) == 0) {
		BTMTK_WARN("%s: PMIC Fault, no need to wait coredump", __func__);
		goto exit;
	} else {
		if (g_sbdev->assert_reason[0] == '\0') {
			unsigned int len = strlen(reason) + 1;

			len = (len >= ASSERT_REASON_SIZE) ? ASSERT_REASON_SIZE : len;
			strncpy(g_sbdev->assert_reason, reason, len);
			reason[len] = '\0';
			BTMTK_ERR("%s: [assert_reason] %s", __func__, g_sbdev->assert_reason);
		}
		atomic_set(&bmain_info->chip_reset, BTMTK_RESET_DOING);
		bmain_info->hif_hook.trigger_assert(g_sbdev);
	}

exit:
	g_sbdev->is_whole_chip_reset = TRUE;
	bt_close(g_sbdev->hdev);

	return 0;
}

static int btmtk_post_chip_rst_handler(void)
{
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();

	BTMTK_INFO("%s: state[%d]", __func__, g_bt_state);
	if (g_sbdev->is_whole_chip_reset) {
		/* for let hw err evt can send event */
		bmain_info->reset_stack_flag = HW_ERR_CODE_CHIP_RESET;
		btmtk_send_hw_err_to_host(g_sbdev);
	} else
		BTMTK_WARN("%s: BT is not on state, no need to send hw err", __func__);

	/* move to bt open flow, ensure hw err event send to stack restart bt*/
	//atomic_set(&bmain_info->chip_reset, BTMTK_RESET_DONE);

	return 0;
}


int btmtk_pre_cal_pre_on_cb(void)
{
	BTMTK_DBG("%s start", __func__);
	return btmtk_pre_power_on_handler();
}

int btmtk_pre_cal_pwr_on_cb(void)
{
	int ret = 0;
	struct btmtk_uart_dev *cif_dev = NULL;

	BTMTK_INFO("%s start", __func__);

	/* Turn on BT */
	if (g_sbdev == NULL) {
		BTMTK_ERR("g_sbdev == NULL");
		return -1;
	}

	if (g_sbdev->hdev == NULL) {
		BTMTK_ERR("g_sbdev->hdev == NULL");
		return -1;
	}

	cif_dev = (struct btmtk_uart_dev *)g_sbdev->cif_dev;
	if (!cif_dev) {
		BTMTK_ERR("[ERR] cif_dev is NULL");
		return -1;
	}
	cif_dev->is_pre_cal = TRUE;

	ret = bt_open(g_sbdev->hdev);
	cif_dev->is_pre_cal = FALSE;
	if (ret) {
		BTMTK_ERR("%s: BT turn on fail!", __func__);
		return ret;
	}
	BTMTK_INFO("%s: BT turn on ok!", __func__);

	return ret;
}
int btmtk_pre_cal_do_cal_cb(void)
{
	int ret = -1;
	struct btmtk_uart_dev *cif_dev = NULL;

	BTMTK_INFO("%s start", __func__);

	/* Todo pre-cal cmd */
	/* Todo precal BK */

	/* err handle for uart disconnect */
	if (g_sbdev == NULL) {
		BTMTK_ERR("%s: g_sbdev == NULL", __func__);
		goto exit;
	}

	if (g_sbdev->hdev == NULL) {
		BTMTK_ERR("%s: g_sbdev->hdev == NULL", __func__);
		goto exit;
	}

	cif_dev = (struct btmtk_uart_dev *)g_sbdev->cif_dev;
	if (!cif_dev) {
		BTMTK_ERR("[ERR] cif_dev is NULL");
		goto exit;
	}

	cif_dev->is_pre_cal = TRUE;
	ret = bt_close(g_sbdev->hdev);
	cif_dev->is_pre_cal = FALSE;
	if (ret) {
		BTMTK_ERR("%s: BT turn off fail!", __func__);
		return ret;
	}
	ret = 0;
	BTMTK_INFO("%s: BT turn off ok!", __func__);

exit:
	if (g_sbdev)
		g_sbdev->is_pre_cal_done = TRUE;
	return ret;
}

/*
 *******************************************************************************
 *						 bt HIF dump feature
 *******************************************************************************
 */
int btmtk_dump_start(void *priv_data, unsigned int force_dump){

	struct btmtk_uart_dev *cif_dev = NULL;

	if (g_sbdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return -1;
	}

	if (btmtk_get_chip_state(g_sbdev) != BTMTK_STATE_WORKING
			|| btmtk_fops_get_state(g_sbdev) != BTMTK_FOPS_STATE_OPENED) {
		BTMTK_WARN("%s: not in working state(%d) fops(%d)"
			, __func__, btmtk_get_chip_state(g_sbdev), btmtk_fops_get_state(g_sbdev));
		return -1;
	}

	cif_dev = (struct btmtk_uart_dev *)g_sbdev->cif_dev;
	if (!cif_dev) {
		BTMTK_ERR("%s: cif_dev is NULL", __func__);
		return -1;
	}

	if (!force_dump && cif_dev->own_state != BTMTK_DRV_OWN) {
		BTMTK_WARN("%s: not in drv own, force_dump[%d], own_state[%d]", __func__, force_dump, cif_dev->own_state);
		return -1;
	}

	BTMTK_INFO("%s start", __func__);
	/* make sure keep drv own */
	atomic_set(&cif_dev->fw_own_timer_flag, FW_OWN_TIMER_UKNOWN);
	return 0;

}
int btmtk_dump_end(void *priv_data){
	struct btmtk_uart_dev *cif_dev = NULL;

	BTMTK_INFO("%s start", __func__);

	if (g_sbdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return -1;
	}

	cif_dev = (struct btmtk_uart_dev *)g_sbdev->cif_dev;
	if (!cif_dev) {
		BTMTK_ERR("%s: cif_dev is NULL", __func__);
		return -1;
	}

	/*reset fw own timer */
	atomic_set(&cif_dev->fw_own_timer_flag, FW_OWN_TIMER_INIT);
	mod_timer(&cif_dev->fw_own_timer, jiffies + msecs_to_jiffies(FW_OWN_TIMEOUT));

	return 0;
}

int btmtk_hif_dump_start(enum connv3_drv_type from_drv, void *priv_data){
	return btmtk_dump_start(priv_data, TRUE);
}

int btmtk_hif_dump_end(enum connv3_drv_type from_drv, void *priv_data){
	return btmtk_dump_end(priv_data);
}

struct connv3_whole_chip_rst_cb btmtk_whole_chip_rst_cb = {
	.pre_whole_chip_rst = btmtk_pre_chip_rst_handler,
	.post_whole_chip_rst = btmtk_post_chip_rst_handler,
};

struct connv3_power_on_cb btmtk_pwr_on_cb = {
	.pre_power_on = btmtk_pre_power_on_handler,
	.power_on_notify = btmtk_power_on_notify_handler,
};

struct connv3_pre_calibration_cb btmtk_pre_cal_cb = {
	.pre_on_cb = btmtk_pre_cal_pre_on_cb,
	.pwr_on_cb = btmtk_pre_cal_pwr_on_cb,
	.do_cal_cb = btmtk_pre_cal_do_cal_cb,
};

/*
 * POWER dump
 */

struct connv3_power_dump_cb btmtk_pwr_dump_cb = {
	.power_dump_start = btmtk_dump_start,
	.power_dump_end = btmtk_dump_end,
};

/*
 * connv3_drv_type
 *
 *    CONNV3_DRV_TYPE_BT = 0
 *    CONNV3_DRV_TYPE_WIFI = 1
 */

extern struct connv3_cr_cb btmtk_connv3_cr_cb;

/*
 * HIF dump
 *
 *    addr is connsys view
 */
struct connv3_hif_dump_cb btmtk_hif_dump_cb = {
	.hif_dump_start = btmtk_hif_dump_start,
	.hif_dump_end = btmtk_hif_dump_end,
};

/* connv3_sub_drv_ops_cb
 *
 *    All callbacks needs by conninfra driver, 3 types of callback functions
 *    1. power on
 *    2. chip reset
 *    3. pre-calibration
 */


int btmtk_read_pmic_state(struct btmtk_dev *bdev)
{
	int ret = 0;
	u8 read_pmic_state_cmd[] = { 0x01, 0x6F, 0xFC, 0x05, 0x01, 0x16, 0x01, 0x00, 0x01 };
	u8 read_pmic_state_event[] = { 0x04, 0xE4, 0x35, 0x02, 0x16, 0x31, 0x00, 0x00 };


	BTMTK_INFO("%s enter", __func__);
	ret = btmtk_main_send_cmd(bdev, read_pmic_state_cmd, sizeof(read_pmic_state_cmd),
			read_pmic_state_event, sizeof(read_pmic_state_event), 0, RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV);
	if (ret < 0)
		BTMTK_ERR("%s: failed(%d)", __func__, ret);
	else
		connv3_update_pmic_state(CONNV3_DRV_TYPE_BT, bdev->io_buf + sizeof(read_pmic_state_event),
									bdev->io_buf[5] - 1);

	return ret;
}

int btmtk_send_connfem_cmd(struct btmtk_dev *bdev)
{
	struct connfem_epaelna_fem_info fem_info;
	struct connfem_epaelna_fem_info bt_fem_info;
	struct connfem_epaelna_flags_common common_flag;
	struct connfem_epaelna_pin_info pin_info;
	struct connfem_epaelna_flags_bt bt_flag;
	int32_t ret = 0;
	uint8_t *cmd = NULL;
	uint8_t cmd_header[] = {0x01, 0x6F, 0xFC, 0x42, 0x01, 0x55, 0x3E, 0x00,
			0x01, 0x04, 0x03, 0x33, 0x00, 0x10};
	uint8_t event[] = {0x04, 0xE4, 0x06, 0x02, 0x55, 0x02, 0x00, 0x00, 0x01};
	uint32_t cmd_len = 0, i = 0, offset = 0;
	const uint32_t pin_struct_size = sizeof(struct connfem_epaelna_pin);

	BTMTK_INFO("%s", __func__);

	/* Get data from connfem_api */
	connfem_epaelna_get_fem_info(&fem_info);
	connfem_epaelna_get_pin_info(&pin_info);
	connfem_epaelna_get_bt_fem_info(&bt_fem_info);
	connfem_epaelna_get_flags(CONNFEM_SUBSYS_NONE, &common_flag);
	connfem_epaelna_get_flags(CONNFEM_SUBSYS_BT, &bt_flag);

	if (fem_info.part[CONNFEM_PORT_BT].vid == 0 &&
	    fem_info.part[CONNFEM_PORT_BT].pid == 0) {
		BTMTK_INFO("CONNFEM BTvid/pid == 0, ignore");
		return 0;
	}

	/*
	 * command and event example
	 *  0  1  2      3  4  5  6  7  8  9  A  B  C  D
	 * 01 6F FC length 01 55 LL LL 01 XX XX XX XX NN YYYYYY ..  YYYYYY AA BB BB CC DD DD DD DD
	 * lengthL : LL + 4
	 * LLLL : length = 1 + 4 + 1 + 3*num + 3 (only 1 byte length valid,
	 *					  value 251 should be maxium)
	 * XXXXXXXX : 4 byte,  efem ID
	 * NN : 1 byte, total efem number
	 * YYYYYY: 3 byte * number, u1AntSelNo,    u1FemPin,     u1Polarity;
	 * AA : bt flag
	 * BBBB : 2.4G part = VID + PID
	 * CC : 1 byte Rx Mode info
	 * DDDDDDDD: 4 bytes SPDT info
	 * ZZZZZZZZ: 4 bytes BT dedicate efem ID
	 *
	 * RX: 04 E4 06 02 55 02 00 01 SS (SS : status)
	 */
	cmd_len = sizeof(cmd_header) + pin_info.count * pin_struct_size + 12;
	cmd = vmalloc(cmd_len);
	if (!cmd) {
		BTMTK_ERR("unable to allocate confem command");
		return -1;
	}

	memcpy(cmd, cmd_header, sizeof(cmd_header));

	/* assign WMT over HCI command length */
	cmd[3] = cmd_len - 4;

	/* assign payload length */
	cmd[6] = cmd_len - 8;

	/* assign femid */
	memcpy(&cmd[9], &fem_info.id, sizeof(fem_info.id));
	offset = sizeof(cmd_header);

	/* assign pin count */
	cmd[offset-1] = pin_info.count;

	/* assign pin mapping info */
	for (i = 0; i < pin_info.count; i++) {
		memcpy(&cmd[offset], &pin_info.pin[i], pin_struct_size);
		offset += pin_struct_size;
	}

	/* config priority: epa_elna > elna > epa > bypass */
	cmd[offset++] = (bt_flag.epa_elna) ? 3 :
			(bt_flag.epa) ? 2 :
			(bt_flag.elna) ? 1 : 0;

	cmd[offset++] = fem_info.part[CONNFEM_PORT_BT].vid;
	cmd[offset++] = fem_info.part[CONNFEM_PORT_BT].pid;

	cmd[offset++] = common_flag.rxmode;
	cmd[offset++] = common_flag.fe_ant_cnt;
	cmd[offset++] = common_flag.fe_main_bt_share_lp2g;
	cmd[offset++] = common_flag.fe_conn_spdt;
	cmd[offset++] = common_flag.fe_reserved;

	memcpy(&cmd[offset], &bt_fem_info.id, sizeof(bt_fem_info.id));
	offset += sizeof(bt_fem_info.id);

	BTMTK_DBG_RAW(cmd, offset, "%s: Send: ", __func__);

	ret = btmtk_main_send_cmd(bdev, cmd, cmd_len,
			event, sizeof(event), 0, RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV);

	if (ret < 0)
		BTMTK_ERR("%s: failed(%d)", __func__, ret);

	vfree(cmd);
	return 0;
}

int btmtk_set_pcm_pin_mux(void)
{
	return 0;
}

/*
 *******************************************************************************
 *			 bt notify fw not in uds feature
 *******************************************************************************
 */
void btmtk_intcmd_set_fw_uds_mode(unsigned char enable)
{
	int ret;
	/*
	 * cmd[9]: enable/disable pull UDS_EN to PMIC
	 *   0=disable, 1=enable
	 */
	u8 cmd[] = { 0x01, 0x6F, 0xFC, 0x06, 0x01, 0x16, 0x02, 0x00, 0x02, 0x00};
	/*
	 * evt[7]: Status
	 *   0=control OK, other=check eror code
	 * evt[9]: Current setting of pull of UDS_EN. Shall be same as cmd[9]
	 */
	u8 evt[] = { 0x04, 0xE4, 0x07, 0x02, 0x16, 0x03, 0x00};

	BTMTK_INFO("%s(%u)", __func__, enable);

	cmd[9] = enable;

	ret = btmtk_main_send_cmd(g_sbdev,
			cmd, sizeof(cmd), evt, sizeof(evt),
			DELAY_TIMES, RETRY_TIMES, BTMTK_TX_PKT_SEND_NO_ASSERT);

	if (ret < 0)
		BTMTK_ERR("%s faill to set uds_mode[%u] to fw", __func__, enable);

}

#if IS_ENABLED(CONFIG_PMIC_LBAT_SERVICE) && LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)
static void btmtk_volt_notifier_cb(unsigned int volt)
{
	if (!g_sbdev) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return;
	}

	BTMTK_INFO("%s volt[%u]->[%u]", __func__, g_sbdev->volt, volt);

	g_sbdev->volt = volt;

	if (btmtk_fops_get_state(g_sbdev) != BTMTK_FOPS_STATE_OPENED) {
		BTMTK_INFO("%s: BT not opened, fops[%d]", __func__, btmtk_fops_get_state(g_sbdev));
		return;
	}
	/* disable UDS when volt below 3.5V
	 * enable  UDS when volt return 3.6V
	 */
	if (volt == BACKOFF_VOLT)
		btmtk_intcmd_set_fw_uds_mode(WMT_PARA_UDS_OFF);
	else if (volt == RESTORE_VOLT)
		btmtk_intcmd_set_fw_uds_mode(WMT_PARA_UDS_ON);
}
#endif

static void btmtk_volt_notify_register(void)
{
#if IS_ENABLED(CONFIG_PMIC_LBAT_SERVICE) && LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)

	static struct lbat_user *lbat_pt;
	int32_t ret = 0;

	BTMTK_INFO("%s", __func__);

	lbat_pt = lbat_user_register("BT Get Battery Voltage", RESTORE_VOLT,
				BACKOFF_VOLT, 0, btmtk_volt_notifier_cb);

	if (IS_ERR(lbat_pt))
		ret = PTR_ERR(lbat_pt);

	if (ret)
		BTMTK_ERR("%s failed", __func__);
	else
		BTMTK_INFO("%s success", __func__);

#endif

}

int btmtk_connv3_sub_drv_init(struct btmtk_dev *bdev)
{
	struct btmtk_uart_dev *cif_dev = NULL;
	struct tty_struct *tty = NULL;
	struct connv3_sub_drv_ops_cb btmtk_drv_cbs;
	int ret;

	btmtk_drv_cbs.pwr_on_cb = btmtk_pwr_on_cb;
	btmtk_drv_cbs.rst_cb = btmtk_whole_chip_rst_cb;
	btmtk_drv_cbs.pre_cal_cb = btmtk_pre_cal_cb;
	btmtk_drv_cbs.cr_cb = btmtk_connv3_cr_cb;
	btmtk_drv_cbs.pwr_dump_cb = btmtk_pwr_dump_cb;
	btmtk_drv_cbs.hif_dump_cb = btmtk_hif_dump_cb;

	BTMTK_DBG("%s start", __func__);
	if (!bdev) {
		BTMTK_ERR("[ERR] bdev is NULL");
		return -1;
	}

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;
	if (!cif_dev) {
		BTMTK_ERR("[ERR] cif_dev is NULL");
		return -1;
	}

	tty = cif_dev->tty;
	if (!tty) {
		BTMTK_ERR("[ERR] tty is NULL");
		return -1;
	}

	tty->dev->of_node = of_find_compatible_node(NULL, NULL, "mediatek,bt");
	if (!tty->dev->of_node)
		BTMTK_ERR("[ERR] %s: mediatek,bt of_node not found", __func__);

	ret = of_property_read_u32(tty->dev->of_node, "baudrate", &cif_dev->baudrate);
	if (ret < 0)
		BTMTK_ERR("[ERR] %s: mediatek,bt baudrate ret[%d]", __func__, ret);

	ret = of_property_read_u32(tty->dev->of_node, "hub-en", &cif_dev->hub_en);
	if (ret < 0)
		BTMTK_ERR("[ERR] %s: mediatek,bt hub-en ret[%d]", __func__, ret);

	ret = of_property_read_u32(tty->dev->of_node, "sleep-en", &cif_dev->sleep_en);
	if(ret < 0)
		BTMTK_ERR("[ERR] %s: mediatek,bt sleep-en ret[%d]", __func__, ret);

	ret = of_property_read_string(tty->dev->of_node, "flavor-bin", &bdev->flavor_bin);
	if(ret < 0){
		const static char default_flavor[] = "1";
		bdev->flavor_bin = default_flavor;
		BTMTK_ERR("[ERR] %s: mediatek,bt flavor-bin ret[%d], using default flavor[1]", __func__, ret);
	}

	pinctrl_ptr = devm_pinctrl_get(tty->dev);
	if (IS_ERR(pinctrl_ptr)) {
		BTMTK_ERR("[ERR] %s: fail to get bt pinctrl", __func__);
		return -1;
	}

	btmtk_pinctrl_exec(BT_FIND_MY_PHONE_HIGH);
	btmtk_pinctrl_exec(BT_FIND_MY_PHONE_LOW);

	/* set gpio to default */
	btmtk_set_gpio_default();

	btmtk_dev_link_uart();

	btmtk_volt_notify_register();

	connv3_sub_drv_ops_register(CONNV3_DRV_TYPE_BT, &btmtk_drv_cbs);
#if IS_ENABLED(CONFIG_MTK_UARTHUB)
	if (cif_dev->hub_en) {
		ret = mtk8250_uart_hub_register_cb(btmtk_uarthub_err_cb);
		BTMTK_DBG("%s mtk8250_uart_hub_register_cb ret[%d]", __func__, ret);
	}
#endif
	BTMTK_INFO("%s end, baudrate[%d] hub_en[%d] sleep_en[%d]", __func__, cif_dev->baudrate, cif_dev->hub_en, cif_dev->sleep_en);
	return 0;
}


int btmtk_connv3_sub_drv_deinit(void)
{
	return connv3_sub_drv_ops_unregister(CONNV3_DRV_TYPE_BT);
}

/*
 *******************************************************************************
 *                       bt power throttling feature
 *******************************************************************************
 */
static inline bool btmtk_pwrctrl_support(void)
{
	return TRUE;
}

static void btmtk_send_set_tx_power_cmd(struct btmtk_dev *bdev)
{
	struct btmtk_uart_dev *cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;
	struct btmtk_dypwr_st *dy_pwr = &cif_dev->dy_pwr;
	uint8_t cmd_set[6] = { 0x01, 0x2D, 0xFC, 0x02, 0x02, 0x00 };
	uint8_t evt_set[] = { 0x04, 0x0E, 0x06, 0x01, 0x2D, 0xFC };
	int ret = 0;

	cmd_set[5] = dy_pwr->set_val;
	ret = btmtk_main_send_cmd(bdev, cmd_set, sizeof(cmd_set),
				evt_set, sizeof(evt_set), 0, RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV);

	if (ret < 0)
		BTMTK_ERR("%s: failed(%d)", __func__, ret);

	if (bdev->io_buf[6] != HCI_EVT_CC_STATUS_SUCCESS)
		BTMTK_ERR("%s: status error[0x%02x]!", __func__, bdev->io_buf[6]);
	else {
		dy_pwr->fw_sel_dbm = bdev->io_buf[8];
		BTMTK_INFO("%s: fw_sel_dbm[%d]", __func__, dy_pwr->fw_sel_dbm);
	}

	if (dy_pwr->cb != NULL)
		dy_pwr->cb(dy_pwr->buf, dy_pwr->len);
}

void btmtk_async_trx_work(struct work_struct *work)
{
	struct btmtk_dev *bdev = container_of(work, struct btmtk_dev, async_trx_work);

	btmtk_send_set_tx_power_cmd(bdev);
}

void btmtk_pwr_on_uds_work(struct work_struct *work)
{
	//struct btmtk_dev *bdev = container_of(work, struct btmtk_dev, pwr_on_uds_work);
	int ret = 0;

	BTMTK_INFO("%s start", __func__);

	/* err handle for uart disconnect */
	if (g_sbdev == NULL) {
		BTMTK_ERR("%s: g_sbdev == NULL", __func__);
		return;
	}

	if (g_sbdev->hdev == NULL) {
		BTMTK_ERR("%s: g_sbdev->hdev == NULL", __func__);
		return;
	}

	if (btmtk_get_chip_state(g_sbdev) == BTMTK_STATE_DISCONNECT) {
		BTMTK_WARN("%s: uart disconnected", __func__);
		return;
	}

	if (!g_sbdev->is_pre_cal_done) {
		BTMTK_WARN("%s: pre-cal not done", __func__);
		return;
	}

	ret = bt_open(g_sbdev->hdev);

	if (ret) {
		BTMTK_ERR("%s: BT turn on fail!", __func__);
		return;
	}
	BTMTK_INFO("%s: BT turn on ok!", __func__);

	ret = bt_close(g_sbdev->hdev);
	if (ret) {
		BTMTK_ERR("%s: BT turn off fail!", __func__);
		return;
	}
	BTMTK_INFO("%s: BT turn off ok!", __func__);

}


int btmtk_query_tx_power(struct btmtk_dev *bdev, BT_RX_EVT_HANDLER_CB cb)
{
	struct btmtk_uart_dev *cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;
	uint8_t cmd_query[] = { 0x01, 0x2D, 0xFC, 0x01, 0x01 };
	uint8_t evt_query[] = { 0x04, 0x0E, 0x08, 0x01, 0x2D, 0xFC };
	int ret = 0;

	if (!btmtk_pwrctrl_support())
		return 0;

	BTMTK_INFO("%s: lp_cur_lv[%d], dy_max_dbm[%d], dy_min_dbm[%d], lp_bdy_dbm[%d], fw_sel_dbm[%d]",
		__func__,
		cif_dev->dy_pwr.lp_cur_lv,
		cif_dev->dy_pwr.dy_max_dbm,
		cif_dev->dy_pwr.dy_min_dbm,
		cif_dev->dy_pwr.lp_bdy_dbm,
		cif_dev->dy_pwr.fw_sel_dbm);

	/*
	 * Query
	 * Cmd: 01 2D FC 01 01
	 * Evt: 04 0E 08 01 2D FC SS 01 XX YY ZZ
	 * SS: Status
	 * XX: Dynamic range Max dBm
	 * YY: Dynamic range Min dBm
	 * ZZ: Low power region boundary dBm
	 */
	ret = btmtk_main_send_cmd(bdev, cmd_query, sizeof(cmd_query),
				evt_query, sizeof(evt_query), 0, RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV);

	if (ret < 0)
		BTMTK_ERR("%s: failed(%d)", __func__, ret);

	if (bdev->io_buf) {
		if (bdev->io_buf[6] != HCI_EVT_CC_STATUS_SUCCESS)
			BTMTK_ERR("%s: status error[0x%02x]!", __func__, bdev->io_buf[6]);
		else {
			cif_dev->dy_pwr.dy_max_dbm = bdev->io_buf[8];
			cif_dev->dy_pwr.dy_min_dbm = bdev->io_buf[9];
			cif_dev->dy_pwr.lp_bdy_dbm = bdev->io_buf[10];
			BTMTK_INFO("%s: dy_max_dbm[%d], dy_min_dbm[%d], lp_bdy_dbm[%d]",
				    __func__,
				    cif_dev->dy_pwr.dy_max_dbm,
				    cif_dev->dy_pwr.dy_min_dbm,
				    cif_dev->dy_pwr.lp_bdy_dbm);
		}
		if (cb)
			cb(cif_dev->dy_pwr.buf, cif_dev->dy_pwr.len);
	}

	return 0;
}

int btmtk_set_tx_power(struct btmtk_dev *bdev, int8_t req_val, BT_RX_EVT_HANDLER_CB cb)
{
	struct btmtk_uart_dev *cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;
	struct btmtk_dypwr_st *dy_pwr = &cif_dev->dy_pwr;
	bool isblocking = (cb) ? TRUE : FALSE;

	/*
	 * Set
	 * Cmd: 01 2D FC 02 02 RR
	 * Evt: 04 0E 06 01 2D FC SS 02 XX
	 * RR: Requested TX power upper limitation dBm
	 * SS: Status
	 * XX: Selected TX power upper limitation dBm
	 */

	/* do not send if not support */
	if (dy_pwr->dy_max_dbm <= dy_pwr->dy_min_dbm) {
		BTMTK_INFO("%s: invalid dbm range, skip set cmd", __func__);
		return 1;
	}

	/* value select flow */
	if (req_val >= dy_pwr->dy_min_dbm) {
		dy_pwr->set_val = req_val;
		/* check max limitation */
		if (dy_pwr->set_val > dy_pwr->dy_max_dbm)
			dy_pwr->set_val = dy_pwr->dy_max_dbm;
		/* power throttling limitation */
		if (dy_pwr->lp_cur_lv >= CONN_PWR_THR_LV_4) { //TODO_PWRCTRL
			dy_pwr->set_val = dy_pwr->lp_bdy_dbm;
			/* lp_bdy_dbm may be larger than dy_max_dbm, check again */
			if (dy_pwr->set_val > dy_pwr->dy_max_dbm)
				dy_pwr->set_val = dy_pwr->dy_max_dbm;
		}
	} else {
		BTMTK_INFO("%s: invalid dbm value, skip set cmd", __func__);
		return 1;
	}

	BTMTK_INFO("%s: set_val[%d]", __func__, dy_pwr->set_val);

	dy_pwr->cb = cb;
	if (isblocking)
		btmtk_send_set_tx_power_cmd(bdev);
	else
		/* Don't block caller thread for set operation */
		schedule_work(&bdev->async_trx_work);
	return 0;
}


int btmtk_pwrctrl_level_change_cb(enum conn_pwr_event_type type, void *data)
{
	struct btmtk_uart_dev *cif_dev = (struct btmtk_uart_dev *)g_sbdev->cif_dev;
	int8_t set_val = cif_dev->dy_pwr.dy_max_dbm;

	BTMTK_INFO("%s", __func__);
	switch (type) {
	case CONN_PWR_EVENT_LEVEL:
		cif_dev->dy_pwr.lp_cur_lv = *((int *) data);
		BTMTK_INFO("%s: lp_cur_bat_lv = %d", __func__, cif_dev->dy_pwr.lp_cur_lv);
		btmtk_set_tx_power(g_sbdev, set_val, NULL);
		break;
	case CONN_PWR_EVENT_MAX_TEMP:
		BTMTK_ERR("Unsupport now");
		break;
	default:
		BTMTK_ERR("Uknown type for power throttling callback");
		break;
	}

	return 0;
}

int btmtk_pwrctrl_pre_on(struct btmtk_dev *bdev)
{
	struct btmtk_uart_dev *cif_dev = NULL;

	if (!btmtk_pwrctrl_support())
		return 0;

	if (!bdev) {
		BTMTK_ERR("[ERR] bdev is NULL");
		return -1;
	}

	cif_dev = bdev->cif_dev;
	memset(&cif_dev->dy_pwr, 0x00, sizeof(cif_dev->dy_pwr));
	cif_dev->dy_pwr.lp_cur_lv = CONN_PWR_THR_LV_0;
	conn_pwr_drv_pre_on(CONN_PWR_DRV_BT, &cif_dev->dy_pwr.lp_cur_lv);
	BTMTK_INFO("%s: lp_cur_bat_lv = %d", __func__, cif_dev->dy_pwr.lp_cur_lv);
	return 0;
}

void btmtk_pwrctrl_post_off(void)
{
	if (!btmtk_pwrctrl_support())
		return;

	conn_pwr_drv_post_off(CONN_PWR_DRV_BT);
}

void btmtk_pwrctrl_register_evt(void)
{
	if (!btmtk_pwrctrl_support())
		return;

	BTMTK_DBG("%s", __func__);
	/* Register callbacks for power throttling feature */
	conn_pwr_register_event_cb(CONN_PWR_DRV_BT, (CONN_PWR_EVENT_CB)btmtk_pwrctrl_level_change_cb);
}

/* btmtk_intcmd_wmt_utc_sync
 *
 *    Send time sync command to FW to synchronize time
 *
 * Arguments:
 *    [IN] hdev     - hci_device as control structure during BT life cycle
 *
 * Return Value:
 *    N/A
 *
 */
int32_t btmtk_intcmd_wmt_utc_sync(void)
{
	struct bt_utc_struct utc;
	uint8_t cmd[] =  {0x01, 0x5D, 0xFC, 0x0A, 0x00, 0x02,
			  0x00, 0x00, 0x00, 0x00,	/* UTC time second unit */
			  0x00, 0x00, 0x00, 0x00};	/* UTC time microsecond unit*/
	uint8_t evt[] = {0x04, 0x0E, 0x0E, 0x01, 0x5D, 0xFC, 0x00, 0x00, 0x02};
	int ret = 0;

	BTMTK_INFO("[InternalCmd] %s", __func__);
	btmtk_getUTCtime(&utc);

	//connsys_log_get_utc_time(&sec, &usec);
	memcpy(cmd + 6, &utc.sec, sizeof(uint32_t));
	memcpy(cmd + 6 + sizeof(uint32_t), &utc.usec, sizeof(uint32_t));

	ret = btmtk_main_send_cmd(g_sbdev, cmd, sizeof(cmd),
		evt, sizeof(evt), DELAY_TIMES, RETRY_TIMES, BTMTK_TX_PKT_SEND_NO_ASSERT);

	if (ret < 0)
		BTMTK_ERR("%s: failed(%d)", __func__, ret);

	return 0;
}


static void btmtk_dump_gpio_state(void)
{
#define GET_BIT(V, INDEX) ((V & (0x1U << INDEX)) >> INDEX)

	unsigned int aux, dir, out;

	if (vir_0x1000_5000 == NULL)
		vir_0x1000_5000 = ioremap(0x10005000, 0x500);

	if (vir_0x1000_5000 == NULL) {
		BTMTK_ERR("%s: vir_0x1000_5000[%lx]", __func__, vir_0x1000_5000);
		return;
	}

	/* 0x1000_5000
	 * 	0x0070	GPIO_DIR7
	 * 		0: GPIO Dir. as Input; 1: GPIO Dir. as Output
	 * 		GPIO_DIR register for GPIO224~GPIO241; GPIO242~GPIO255:
	 * 		2: 226
	 * 		3: 227
	 * 		4: 228
	 * 		5: 229
	 * 		16: 240
	 * 	0x04E0	GPIO_MODE30
	 * 		[2:0]    Aux mode of PAD_COEX_UTXD
	 * 		000:B:GPIO240;
	 * 		001:I1:URXD3;
	 * 		010:O:PCM1_DO2;
	 * 		011:I1:URXD2;
	 * 		100:O:I2S8_LRCK;
	 * 		101:;110:;111:;
	 * 	0x0170	GPIO_DOUT7
	 * 		0: GPIO output low; 1: GPIO output high;
	 * 		16: 240
	 */
	aux = CONSYS_REG_READ(vir_0x1000_5000 + 0x04E0);
	dir = CONSYS_REG_READ(vir_0x1000_5000 + 0x0070);
	out = CONSYS_REG_READ(vir_0x1000_5000 + 0x0170);

	BTMTK_DBG("%s: aux[0x%08x] dir[0x%08x] out[0x%08x]", __func__,aux, dir, out);
	BTMTK_INFO("%s: GPIO_240 aux=[%d] dir=[%s] out[%d]", __func__,
		((aux & 0x07)), (GET_BIT(dir, 16) ? "OUT" : "IN"), GET_BIT(out, 16));

}


/*******************************************************************************
*                           bt host debug information for low power
********************************************************************************
*/
#define BTHOST_INFO_MAX	16
#define BTHOST_DESC_LEN 16

struct bthost_info{
	uint32_t		id; //0 for not used
	char 		desc[BTHOST_DESC_LEN];
	uint32_t		value;
};
struct bthost_info bthost_info_table[BTHOST_INFO_MAX];

void bthost_debug_init(void)
{
	uint32_t i = 0;
	for (i = 0; i < BTHOST_INFO_MAX; i++){
		bthost_info_table[i].id = 0;
		bthost_info_table[i].desc[0] = '\0';
		bthost_info_table[i].value = 0;
	}
}

void bthost_debug_print(void)
{
	uint32_t i = 0;
	int32_t ret = 0;
	uint8_t *pos = NULL, *end = NULL;
	uint8_t dump_buffer[700]={0};

	pos = &dump_buffer[0];
	end = pos + 700 - 1;

	ret = snprintf(pos, (end - pos + 1), "[bt host info] ");
	if (ret < 0 || ret >= (end - pos + 1)) {
		BTMTK_ERR("snprintf [bt host info] fail");
	} else {
		pos += ret;
	}

	for (i = 0; i < BTHOST_INFO_MAX; i++){
		if (bthost_info_table[i].id == 0){
			ret = snprintf(pos, (end - pos + 1),"[%d-%d] not set", i, BTHOST_INFO_MAX);
			if (ret < 0 || ret >= (end - pos + 1)){
				BTMTK_ERR("%s: snprintf fail i[%d] ret[%d]", __func__, i, ret);
				break;
			}
			pos += ret;
			break;
		}
		else {
			ret = snprintf(pos, (end - pos + 1),"[%d][%s : 0x%08x] ", i,
			bthost_info_table[i].desc,
			bthost_info_table[i].value);
			if (ret < 0 || ret >= (end - pos + 1)){
				BTMTK_ERR("%s: snprintf fail i[%d] ret[%d]", __func__, i, ret);
				break;
			}
			pos += ret;
		}
	}
	BTMTK_INFO("%s", dump_buffer);
}

void bthost_debug_save(uint32_t id, uint32_t value, char* desc)
{
	uint32_t i = 0;
	if (id == 0) {
		BTMTK_WARN("%s: id (%d) must > 0\n", __func__, id);
		return;
	}
	for (i = 0; i < BTHOST_INFO_MAX; i++){
		// if the id is existed, save to the same column
		if (bthost_info_table[i].id == id){
			bthost_info_table[i].value = value;
			return;
		}
		// save to the new column
		if (bthost_info_table[i].id == 0){
			bthost_info_table[i].id = id;
			strncpy(bthost_info_table[i].desc, desc, BTHOST_DESC_LEN - 1);
			bthost_info_table[i].value = value;
			return;
		}
	}
	BTMTK_WARN("%s: no space for %d\n", __func__, id);
}

/*******************************************************************************
*                           bt find my phone mode
********************************************************************************
*/

int btmtk_find_my_phone_cmd(void)
{
	u8 cmd[] = { 0x01, 0x6F, 0xFC, 0x09, 0x01, 0x17, 0x05, 0x00, 0x01, 0x1F, 0x00, 0x00, 0x00 };
	u8 evt[] = { 0x04, 0xE4, 0x0A, 0x02, 0x17, 0x06, 0x00, 0x00, 0x01, 0x1F, 0x00, 0x00, 0x00 };
	int ret = 0;

	BTMTK_INFO("%s", __func__);
	ret = btmtk_main_send_cmd(g_sbdev, cmd, sizeof(cmd), evt, sizeof(evt),
			DELAY_TIMES, RETRY_TIMES, BTMTK_TX_PKT_SEND_NO_ASSERT);
	if (ret < 0)
		BTMTK_ERR("%s: failed, ret[%d]", __func__, ret);
	return ret;
}

int btmtk_uart_launcher_deinit(void)
{
	int cnt = 30;
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();

	while (g_sbdev->fops_state != BTMTK_FOPS_STATE_CLOSED && --cnt) {
		BTMTK_INFO("%s: wait bt close, fops[%d] count[%d]", __func__, g_sbdev->fops_state, cnt);
		msleep(100); /* wait BT close */
	}

	if (bmain_info->hif_hook.cif_mutex_lock)
		bmain_info->hif_hook.cif_mutex_lock(g_sbdev);

	btmtk_set_chip_state(g_sbdev, BTMTK_STATE_DISCONNECT);

	if (bmain_info->hif_hook.cif_mutex_unlock)
		bmain_info->hif_hook.cif_mutex_unlock(g_sbdev);

	// if not wait to BT close
	if (!cnt) {
		BTMTK_INFO("%s: BT not closed fops(%d)", __func__, g_sbdev->fops_state);
		msleep(50);
	}

	return 0;
}

#endif // (USE_DEVICE_NODE == 1)

