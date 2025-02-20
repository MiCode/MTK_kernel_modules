/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <linux/gpio/consumer.h>
#include <linux/of_device.h>
#include <linux/of.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_device.h>
#include "btmtk_main.h"
#include "conninfra.h"
#include "connfem.h"
#include "connfem_api.h"
#include "connfem_sku.h"
#include "btmtk_proj_sp.h"
#include "btmtk_proj_sp_platform.h"
#include "btmtk_uart_tty.h"
#include "btmtk_fw_log.h"
#include <linux/platform_device.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
#include "mtk_disp_notify.h"
#endif


#define READ_PMIC_STATE_CMD_LEN		16
#define READ_PMIC_STATE_EVENT_LEN	16


#define DFD_CB_INFRA_INFO_LEN		768
#define DFD_CB_INFRA_SRAM_LEN		1024
#define DFD_CB_INFRA_BT_SRAM_LEN	(1024 * 4)
#define DFD_BT_DEBUG_INFO_LEN		(768 + 16)
#define DFD_CB_INFRA_DEBUG_INFO_HW_LOG_LEL_ADDR    (128)

#define DEFAULT_STATE_PINCTRL_NAME	("bt_combo_gpio_init")
#define PRE_ON_PINCTRL_NAME		("bt_combo_gpio_pre_on")
#define POWER_ON_TX_PINCTRL_NAME	("bt_combo_uart_tx_aux")
#define POWER_ON_RX_PINCTRL_NAME	("bt_combo_uart_rx_aux")
#define RST_ON_PINCTRL_NAME		("bt_rst_on")
#define RST_OFF_PINCTRL_NAME		("bt_rst_off")
#define BTMTK_UART_NAME			("btmtk_uart")
#define BT_FIND_MY_PHONE_HIGH	("bt-find-my-phone-high")
#define BT_FIND_MY_PHONE_LOW	("bt-find-my-phone-low")


#if (USE_DEVICE_NODE == 1)
static struct pinctrl *pinctrl_ptr;
extern struct btmtk_dev *g_sbdev;
int g_bt_state;
struct platform_prop *g_platform_prop = NULL;

#define CONSYS_REG_READ(addr) (*((volatile unsigned int *)(addr)))

extern void btmtk_uart_trigger_assert_by_tx_thread(struct btmtk_dev *bdev);

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
#if IS_ENABLED(CONFIG_SUPPORT_UARTDBG)
		if (btmtk_get_chip_state(g_sbdev) != BTMTK_STATE_DISCONNECT)
			mtk8250_uart_dump(cif_dev->tty);
#endif
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

	/* Clear DEV0_HOST_AWAKE */
	if (cif_dev->sleep_flow_hw_mech_en && cif_dev->sleep_en) {
		ret = mtk8250_uart_hub_clear_host_awake_sta(0);
		BTMTK_INFO("%s: Clear DEV0_HOST_AWAKE ret[%d]", __func__, ret);
	}

	/* force is for only bt off flow */
	if (force) {
		ret = mtk8250_uart_hub_dev0_clear_tx_request();
		BTMTK_DBG("%s mtk8250_uart_hub_dev0_clear_tx_request ret[%d]", __func__, ret);
	}

	/* Clr TX,RX request, let uarthub can sleep */
	if ((cif_dev->sleep_en || force) && (btmtk_get_chip_state(g_sbdev) != BTMTK_STATE_DISCONNECT)) {
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
	if (cif_dev->hub_en) {
		/* uarthub assert bit to avoid MD/ADSP send data */
		mtk8250_uart_hub_assert_bit_ctrl(1);
		BTMTK_INFO("%s mtk8250_uart_hub_assert_bit_ctrl(1)", __func__);
	}
#endif

#if IS_ENABLED(CONFIG_SUPPORT_UARTDBG)
	if (btmtk_get_chip_state(g_sbdev) != BTMTK_STATE_DISCONNECT)
		mtk8250_uart_dump(cif_dev->tty);
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

	btmtk_assert_wake_unlock();

#if IS_ENABLED(CONFIG_MTK_UARTHUB)
	/* uarthub reset */
	if (cif_dev->hub_en) {
		//mtk8250_uart_hub_assert_bit_ctrl(0);
		//BTMTK_DBG("%s mtk8250_uart_hub_assert_bit_ctrl(0)", __func__);
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
	if (cif_dev->hub_en) {
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

		ret = mtk8250_uart_hub_bt_on_count_inc();
		BTMTK_INFO("%s mtk8250_uart_hub_bt_on_count_inc() ret[%d]", __func__, ret);
	}
#endif
	/* reopen tty */
	if(cif_dev != NULL && cif_dev->tty != NULL && cif_dev->tty->port != NULL
		&& g_sbdev->is_pre_cal_done) {
		BTMTK_INFO("%s tty_port[%p], port_count[%d]",
				__func__, cif_dev->tty->port, cif_dev->tty->port->count);
		if (cif_dev->tty->port->count == 0)
			cif_dev->tty->ops->open(cif_dev->tty, NULL);
	}
	btmtk_dump_gpio_state();
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

	/* wait PRE_ON_PINCTRL_NAME */
	msleep(20);

	btmtk_pinctrl_exec(RST_OFF_PINCTRL_NAME);
	msleep(50);

	if (!bmain_info->find_my_phone_mode) {
		/* If BT off, Wifi on keep Aux func */
		//btmtk_pinctrl_exec(DEFAULT_STATE_PINCTRL_NAME);
	} else {
		BTMTK_INFO("%s: into find my phone mode, skip set tx/rx gpio PD", __func__);
	}

	btmtk_dump_gpio_state();

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

static int btmtk_chip_power_down_notify_handler(unsigned int notify)
{
	struct btmtk_uart_dev *cif_dev = NULL;

        /* Execute when wifi and bt both power down */
        BTMTK_INFO("%s", __func__);

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

	btmtk_dump_gpio_state();

	/* If BT off and Wifi off set GPIO mode input low */
	BTMTK_INFO("%s: BT off and Wifi off, Set GPIO mode input low", __func__);
	btmtk_pinctrl_exec(DEFAULT_STATE_PINCTRL_NAME);

	btmtk_dump_gpio_state();

        /* If BT off, Wifi on keep Aux func*/
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

	/* If PMIC fault, need to call connv3_trigger_pmic_irq */
	if (strncmp(g_sbdev->assert_reason, "[BT_FW assert] trigger PMIC fault", strlen("[BT_FW assert] trigger PMIC fault")) == 0) {
		BTMTK_INFO("%s call connv3_trigger_pmic_irq", __func__);
		return connv3_trigger_pmic_irq(CONNV3_DRV_TYPE_BT, g_sbdev->assert_reason);
	}

	return connv3_trigger_whole_chip_rst(CONNV3_DRV_TYPE_BT , g_sbdev->assert_reason);
}

int btmtk_sp_close(void)
{
	struct btmtk_uart_dev *cif_dev = NULL;
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();

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
			if (g_sbdev->reset_type != CONNV3_CHIP_RST_TYPE_DFD_DUMP)
				btmtk_sp_coredump_end();
			/* trigger hif dump */
			if (bmain_info->hif_hook.dump_hif_debug_sop) {
				bmain_info->hif_hook.dump_hif_debug_sop(g_sbdev);
				flush_work(&g_sbdev->hif_dump_work);
			}
		}
	}

	cancel_work_sync(&g_sbdev->reset_waker);

	/* set tx/rx gpio PU */
	btmtk_pinctrl_exec(PRE_ON_PINCTRL_NAME);

#if IS_ENABLED(CONFIG_MTK_UARTHUB)
	if (cif_dev->hub_en) {
		mtk8250_uart_hub_assert_bit_ctrl(1);
		BTMTK_INFO("%s mtk8250_uart_hub_assert_bit_ctrl(1)", __func__);
		btmtk_release_uarthub(true);
	}
#endif

	btmtk_set_gpio_default();

	/* flush tty before tty close */
	down(&cif_dev->tty_flush_sem);
	BTMTK_INFO("%s: tty flush, tty_chars_in_buffer[%d]", __func__, tty_chars_in_buffer(cif_dev->tty));
	/* stop uart auto send next pkt to avoid flush conflict with send pkt */
	cif_dev->tty->flow.stopped = true;
	tty_driver_flush_buffer(cif_dev->tty);
	cif_dev->tty->flow.stopped = false;
	up(&cif_dev->tty_flush_sem);


	BTMTK_INFO("%s: tty close, tty_chars_in_buffer[%d]", __func__, tty_chars_in_buffer(cif_dev->tty));
	cif_dev->tty->ops->close(cif_dev->tty, NULL);
	cif_dev->is_pre_on_done = FALSE;

	return 0;
}

/*
	reset_type:
	0: As is Falcon, PMIC will power down.
	1: PMIC fault_b, combo chip is power down, coredump is invalid, can do force coredump.
	2: CONN_RST or PMIC_IRQB, combo chip is still on, need to do DFD dump.
*/
static int btmtk_pre_chip_rst_handler(enum connv3_drv_type drv, char *reason, unsigned int reset_type)
{
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();
	unsigned char fstate = BTMTK_FOPS_STATE_INIT;
	int ret = 0;
	char *temp_buf = NULL;

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
	BTMTK_ERR("%s: state[%d], reason[%s], reset_type[%d]", __func__, g_bt_state, reason, reset_type);
	if (fstate == BTMTK_FOPS_STATE_CLOSED) {
		BTMTK_WARN("%s: BT fops is closed, no need to trigger whole chip reset", __func__);
		return 0;
	}

	if (g_sbdev->assert_reason[0] == '\0') {
		unsigned int len = strlen(reason) + 1;

		len = (len >= ASSERT_REASON_SIZE) ? ASSERT_REASON_SIZE : len;
		strncpy(g_sbdev->assert_reason, reason, len);
		reason[len] = '\0';
		BTMTK_ERR("%s: [assert_reason] %s", __func__, g_sbdev->assert_reason);
	}

	temp_buf = vmalloc(DFD_EMI_DUMP_SIZE);
	if (temp_buf == NULL) {
		BTMTK_ERR("%s: vmalloc failed!!", __func__);
		return -1;
	}

	if (drv == CONNV3_DRV_TYPE_CONNV3 && (strncmp(reason, "PMIC Fault", strlen("PMIC Fault")) == 0 || reset_type == CONNV3_CHIP_RST_TYPE_PMIC_FAULT_B)) {
		BTMTK_WARN("%s: PMIC Fault, no need to wait coredump", __func__);
		goto exit;
	} else if (reset_type == CONNV3_CHIP_RST_TYPE_DFD_DUMP) {
		/* DFD
		1. Debug SOP
		2. Coredump, don't call coredump end
		*/
		g_sbdev->reset_type = reset_type;
		g_sbdev->is_dfd_done = FALSE;
		BTMTK_INFO("%s: reset type = %d", __func__, g_sbdev->reset_type);
	}

	atomic_set(&bmain_info->chip_reset, BTMTK_RESET_DOING);
	bmain_info->hif_hook.trigger_assert(g_sbdev);

	if (reset_type == CONNV3_CHIP_RST_TYPE_DFD_DUMP && (btmtk_get_chip_state(g_sbdev) != BTMTK_STATE_DISCONNECT)) {
		if (!wait_for_completion_timeout(&g_sbdev->dump_dfd_comp, msecs_to_jiffies(WAIT_FW_DUMP_TIMEOUT)))
			BTMTK_ERR("%s: uanble to finish dump_dfd_comp in 15s", __func__);

		g_sbdev->dfd_value_addr_remap_base = ioremap(g_sbdev->dfd_value_addr, g_sbdev->dfd_value_size);
		if (g_sbdev->dfd_value_addr_remap_base == NULL) {
			BTMTK_ERR("%s: dfd_value_addr_remap_base failed", __func__);
			goto exit;
		} else {
			memcpy_fromio(temp_buf, (char *)g_sbdev->dfd_value_addr_remap_base, g_sbdev->dfd_value_size);
			BTMTK_INFO("%s: dfd-value-size =  0x%04x, temp_buf[0] =  0x%02x, temp_buf[1] =  0x%02x", __func__, g_sbdev->dfd_value_size, temp_buf[0], temp_buf[1]);

			if ((btmtk_get_chip_state(g_sbdev) != BTMTK_STATE_DISCONNECT)) {
				ret = connv3_coredump_send(bmain_info->hif_hook.coredump_handler, "PRED", temp_buf, g_sbdev->dfd_value_size);
				if (ret)
					BTMTK_ERR("%s: connv3_coredump_send failed, ret = %d", __func__, ret);
			} else {
				BTMTK_ERR("%s: bt state is disconnect", __func__);
			}
			iounmap(g_sbdev->dfd_value_addr_remap_base);
		}
		complete_all(&g_sbdev->dump_comp);
	}

exit:
	g_sbdev->is_whole_chip_reset = TRUE;
	vfree(temp_buf);
	ret = bt_close(g_sbdev->hdev);
	if (ret) {
		BTMTK_ERR("%s: BT turn off fail!", __func__);
		return ret;
	}

	return 0;
}

static int btmtk_post_chip_rst_handler(void)
{
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();

	BTMTK_INFO("%s: state[%d]", __func__, g_bt_state);
	if (g_sbdev->is_whole_chip_reset || g_sbdev->reset_type == CONNV3_CHIP_RST_TYPE_DFD_DUMP) {
		/* for let hw err evt can send event */
		bmain_info->reset_stack_flag = HW_ERR_CODE_CHIP_RESET;
		btmtk_send_hw_err_to_host(g_sbdev);
	} else
		BTMTK_WARN("%s: BT is not on state, no need to send hw err", __func__);

	/* move to bt open flow, ensure hw err event send to stack restart bt*/
	//atomic_set(&bmain_info->chip_reset, BTMTK_RESET_DONE);

	return 0;
}

#if CFG_MTK_BT_DFD_DUMP_SUPPORT
/*
	1. bt_rst = 1 (TBD)
	2. partial patch download option, if rom code support dump
	3. dump DFD post data through coredump API
              I. Get data from FW by command/event
              II. Connv3_coredump_send(handler, tag=“DFDP”, content, length)
	4. end coredump
        *** type: how connv3 control PMIC ***
              * 0: do nothing, keep present state
              * 1: PMIC off
*/
static int btmtk_post_chip_rst_on_handler(unsigned int type)
{
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();
	int ret = 0;
	u32 length = 0;
	int total_length = 0;
	u32 offset = 0;
	/* dump DFD post data, last 2 bytes are length, cmd[8] is Flag */
	u8 cmd[] = { 0x01, 0x6F, 0xFC, 0x09, 0x01, 0x18, 0x05, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00 };
	/* evt[7] is status */
	u8 evt[] = { 0x04, 0xE4, 0xF5, 0x02, 0x18, 0xF1, 0x00, 0x00};
	BTMTK_INFO("%s: type[%d]", __func__, type);

	if (g_sbdev->reset_type == CONNV3_CHIP_RST_TYPE_DFD_DUMP) {
		btmtk_pre_power_on_handler();

		BTMTK_INFO("%s: g_sbdev->reset_type = %d !", __func__, g_sbdev->reset_type);
		/* config pre_pwr_on related (GPIO, baudrate, uarthub, tx_thread) */
		ret = bmain_info->hif_hook.pre_open(g_sbdev);
		if (ret != 0) {
			BTMTK_ERR("%s: btmtk_sp_pre_open failed, ret = %d", __func__, ret);
			goto end;
		}

		for (int region = 1; region <= 5; region++) {
			BTMTK_INFO("%s: region[%d]", __func__, region);
			switch (region) {
			case 1:
				BTMTK_WARN("%s: dump DFD_CB_INFRA_INFO", __func__);
				cmd[8] = 0x01;
				total_length = DFD_CB_INFRA_INFO_LEN;
				/* DFD_CB_INFRA_INFO_ADDR 0x74000000 */
				/* reset offset to 0 */
				offset = 0;
				break;

			case 2:
				BTMTK_WARN("%s: dump DFD_CB_INFRA_SRAM", __func__);
				cmd[8] = 0x02;
				total_length = DFD_CB_INFRA_SRAM_LEN;
				/* DFD_CB_INFRA_SRAM_ADDR 0x74004000 */
				/* reset offset to 0 */
				offset = 0;
				/* reset length to 0 */
				length = 0;
				break;

			case 3:
				BTMTK_WARN("%s: dump DFD_CB_INFRA_BT_SRAM", __func__);
				cmd[8] = 0x04;
				total_length = DFD_CB_INFRA_BT_SRAM_LEN;
				/* DFD_CB_INFRA_BT_SRAM_ADDR 0x74005400 */
				/* reset offset to 0 */
				offset = 0;
				/* reset length to 0 */
				length = 0;
				break;

			case 4:
				BTMTK_WARN("%s: dump DFD_BT_DEBUG_INFO", __func__);
				cmd[8] = 0x07;
				total_length = DFD_BT_DEBUG_INFO_LEN;
				/* DFD_BT_DEBUG_INFO_ADDR 0x7405B580 */
				/* reset offset to 0 */
				offset = 0;
				/* reset length to 0 */
				length = 0;
				break;

			case 5:
				BTMTK_WARN("%s: dump DFD_CB_INFRA_DEBUG_INFO_HW_LOG_LEL", __func__);
				cmd[8] = 0x05;
				total_length = DFD_CB_INFRA_DEBUG_INFO_HW_LOG_LEL_ADDR;
				/* DFD_BT_DEBUG_INFO_ADDR 0x7C05B260 */
				/* reset offset to 0 */
				offset = 0;
				/* reset length to 0 */
				length = 0;
				break;

			default:
				BTMTK_ERR("%s: default, do not happened", __func__);
				goto end;
				break;
			}

			while (total_length >= 0) {
				if (total_length <= 0) {
					BTMTK_WARN("%s, total_length: %d", __func__, total_length);
					break;
				}

				/* offset: cmd[10] | cmd[9] */
				cmd[10] = (offset >> 8) & 0xFF;
				cmd[9] = offset & 0xFF;

				/* length: cmd[12] | cmd[11], 0xF0 is max length FW can support */
				length = (total_length >= 0xF0) ? 0xF0 : total_length;
				cmd[12] = (length >> 8) & 0xFF;
				cmd[11] = length & 0xFF;
				length = cmd[12] | cmd[11];

				BTMTK_INFO("%s, length = %d, offset = %04x", __func__, length, offset);

				/* dump DFD post data */
				ret = btmtk_main_send_cmd(g_sbdev, cmd, sizeof(cmd), evt, sizeof(evt), DELAY_TIMES, RETRY_TIMES, BTMTK_TX_PKT_SEND_NO_ASSERT, CMD_NO_NEED_FILTER);
				BTMTK_INFO("%s, dump DFD post data, ret = %d", __func__, ret);
				if (ret < 0) {
					BTMTK_ERR("%s: failed, ret[%d]", __func__, ret);
					goto end;
				}
				/* delay 40ms for dfd data rx */
				mdelay(40);

				/* update total length */
				total_length -= length;

				/* update offset */
				offset += length;
			} /* end while */
		} /* end for */
end:
		/* end coredump */
		btmtk_sp_coredump_end();

		// reset UART pin?
	}

	return ret;
}
#endif

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

int btmtk_pre_fmd(void) {
	BTMTK_INFO("%s start", __func__);
	return btmtk_set_powered_off_finder_mode(1);
}

int btmtk_post_fmd(void) {
	BTMTK_INFO("%s: set poweroff_finder_mode to 0", __func__);
	atomic_set(&g_sbdev->poweroff_finder_mode, 0);
	return 0;
}

struct connv3_whole_chip_rst_cb btmtk_whole_chip_rst_cb = {
	.pre_whole_chip_rst = btmtk_pre_chip_rst_handler,
#if CFG_MTK_BT_DFD_DUMP_SUPPORT
	.post_reset_on = btmtk_post_chip_rst_on_handler,
#endif
	.post_whole_chip_rst = btmtk_post_chip_rst_handler,
};

struct connv3_power_on_cb btmtk_pwr_on_cb = {
	.pre_power_on = btmtk_pre_power_on_handler,
	.power_on_notify = btmtk_power_on_notify_handler,
	.chip_power_down_notify = btmtk_chip_power_down_notify_handler,
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
 * FMD mode
 */

struct connv3_fmd_cb btmtk_fmd_cb = {
	.pre_fmd_cb = btmtk_pre_fmd,
	.post_fmd_cb = btmtk_post_fmd,
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

	if (bdev->chip_id == 0x6653) {
		BTMTK_INFO("%s bdev->chip_id = 0x%04x", __func__, bdev->chip_id);
		read_pmic_state_event[2] = 0xF5;
		read_pmic_state_event[5] = 0xF1;
	}

	BTMTK_INFO("%s enter", __func__);
	ret = btmtk_main_send_cmd(bdev, read_pmic_state_cmd, sizeof(read_pmic_state_cmd),
			read_pmic_state_event, sizeof(read_pmic_state_event), 0, RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);
	if (ret < 0)
		BTMTK_ERR("%s: failed(%d)", __func__, ret);
	else
		connv3_update_pmic_state(CONNV3_DRV_TYPE_BT, bdev->io_buf + sizeof(read_pmic_state_event),
									bdev->io_buf[5] - 1);

	return ret;
}

static int btmtk_send_connfem_new(struct btmtk_dev *bdev)
{
	const struct connfem_sku *fem_info = NULL;
	uint32_t i = 0, j = 0, fem_id = 0;
	uint16_t offset = 0, cmd_len = 0;
	uint8_t value = 0;
	uint8_t *cmd = NULL;
	uint8_t cmd_header[] = {0x02, 0x6F, 0xFC, 0x00, 0x00, 0x01, 0x55, 0x00,
			0x00, 0x01, 0x00};
	uint8_t event[] = {0x04, 0xE4, 0x06, 0x02, 0x55, 0x02, 0x00, 0x00, 0x01};
	int32_t ret = 0;
	const uint32_t pin_struct_size = sizeof(struct connfem_sku_pinmap);

	/* Get data from connfem_api */
	connfem_sku_data(&fem_info);
	if (!fem_info)
	{
		BTMTK_ERR("%s: get sku failed\n", __func__);
		return -EINVAL;
	}

	cmd = vmalloc(HCI_MAX_ACL_SIZE);
	if (!cmd) {
		BTMTK_ERR("unable to allocate confem_new command");
		return -1;
	}

	memcpy(cmd, cmd_header, sizeof(cmd_header));
	offset = sizeof(cmd_header);
	connfem_sku_flag_u8(CONNFEM_SUBSYS_BT, "efem-mode", &value);
	cmd[offset++] = value;
	connfem_sku_flag_u8(CONNFEM_SUBSYS_BT, "rx-mode", &value);
	cmd[offset++] = value;
	connfem_sku_flag_u8(CONNFEM_SUBSYS_NONE, "fe-ant-cnt", &value);
	cmd[offset++] = value;
	connfem_sku_flag_u8(CONNFEM_SUBSYS_NONE, "fe-conn-dpdt-sp3t", &value);
	cmd[offset++] = value;
	connfem_sku_flag_u8(CONNFEM_SUBSYS_NONE, "fe-conn-spdt", &value);
	cmd[offset++] = value;
	connfem_sku_flag_u8(CONNFEM_SUBSYS_NONE, "fe-bt-wf-usage", &value);
	cmd[offset++] = value;
	connfem_sku_flag_u8(CONNFEM_SUBSYS_NONE, "fe-conn-spdt-2", &value);
	cmd[offset++] = value;

	/* spdt info */
	cmd[offset++] = fem_info->spdt.pin_count;
	for (i = 0; i < fem_info->spdt.pin_count; i++) {
		memcpy(&cmd[offset], &fem_info->spdt.pinmap[i], pin_struct_size);
		offset += pin_struct_size;
	}

	/* eFem array info */
	cmd[offset++] = fem_info->fem_count;
	for (i = 0; i < fem_info->fem_count; i++) {
		fem_id = ((fem_info->fem[i].info.vid << 16) | fem_info->fem[i].info.pid);
		memcpy(&cmd[offset], &fem_id, 4);
		offset += 4;
		memcpy(&cmd[offset], &fem_info->fem[i].info.flag, 4);
		offset += 4;
		cmd[offset++] = fem_info->fem[i].ctrl_pin.count;
		cmd[offset++] = fem_info->fem[i].tt.logic_count;
	}

	for (i = 0; i < fem_info->fem_count; i++)
		for (j = 0; j < fem_info->fem[i].ctrl_pin.count; j++)
			cmd[offset++] = fem_info->fem[i].ctrl_pin.id[j];

	for (i = 0; i < fem_info->fem_count; i++)
		for (j = 0; j < fem_info->fem[i].tt.logic_count; j++) {
			cmd[offset++] = fem_info->fem[i].tt.logic[j].op;
			cmd[offset++] = fem_info->fem[i].tt.logic[j].binary;
		}

	/* layout usage array info */
	memcpy(&cmd[offset], &fem_info->layout_flag, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	cmd[offset++] = fem_info->layout_count;

	for (i = 0; i < fem_info->layout_count; i++) {
		cmd[offset++] = fem_info->layout[i].bandpath[CONNFEM_SUBSYS_WIFI];
		cmd[offset++] = fem_info->layout[i].bandpath[CONNFEM_SUBSYS_BT];
		cmd[offset++] = fem_info->layout[i].fem_idx;
		cmd[offset++] = fem_info->layout[i].pin_count;
	}

	for (i = 0; i < fem_info->layout_count; i++)
		for (j = 0; j < fem_info->layout[i].pin_count; j++) {
			memcpy(&cmd[offset], &fem_info->layout[i].pinmap[j], pin_struct_size);
			offset += pin_struct_size;
		}

	/* assign WMT over ACL command length */
	cmd_len = offset - 5;
	memcpy(&cmd[3], &cmd_len, 2);

	/* assign payload length */
	cmd_len = offset - 9;
	memcpy(&cmd[7], &cmd_len, 2);

	BTMTK_DBG_RAW(cmd, offset, "%s: Send: ", __func__);

	ret = btmtk_main_send_cmd(bdev, cmd, offset,
			event, sizeof(event), 0, RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);

	vfree(cmd);
	return 0;

}

int btmtk_send_connfem_legacy(struct btmtk_dev *bdev)
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
			event, sizeof(event), 0, RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);

	if (ret < 0)
		BTMTK_ERR("%s: failed(%d)", __func__, ret);

	vfree(cmd);
	return 0;
}

int btmtk_send_connfem_cmd(struct btmtk_dev *bdev)
{
	if (connfem_is_available(CONNFEM_TYPE_EPAELNA))
		return btmtk_send_connfem_legacy(bdev);
	else if (connfem_is_available(CONNFEM_TYPE_SKU))
		return btmtk_send_connfem_new(bdev);
	else
		BTMTK_ERR("Not valid FEM data");

	return -1;
}


int btmtk_set_pcm_pin_mux(void)
{
	return 0;
}

/*
 *******************************************************************************
 *			 fw wakeup host through irq feature
 *******************************************************************************
 */

static irqreturn_t btmtk_host_wake_isr(int irq, void *dev)
{
	struct btmtk_dev *bdev = (struct btmtk_dev *)dev;
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();
	int host_wake;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
	host_wake = gpiod_get_value(bdev->gpio_cs);
#else
	host_wake = gpio_get_value(bdev->wakeup_irq);
#endif
	irq_set_irq_type(irq, host_wake ? IRQF_TRIGGER_FALLING: IRQF_TRIGGER_RISING);
	BTMTK_DBG("%s: %d", __func__, host_wake);

	/* 0 value for wake up*/
	if (!host_wake)
		bmain_info->hif_hook.wakeup_host(bdev);

	return IRQ_HANDLED;
}

int btmtk_register_wakeup_irq(struct btmtk_dev *bdev, struct tty_struct *tty) {
	int ret;
	int irq;

	if (!tty->dev->of_node) {
		BTMTK_ERR("[ERR] %s: mediatek,bt of_node not found", __func__);
		return -1;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
	bdev->gpio_cs = gpiod_get_index(tty->dev, NULL, 0, GPIOD_IN);
	ret = PTR_ERR_OR_ZERO(bdev->gpio_cs);
	if (ret) {
		BTMTK_ERR("[ERR] %s: uanble to get bt host wake gpio", __func__);
		return -1;
	}

	irq = gpiod_to_irq(bdev->gpio_cs);
#else
	bdev->wakeup_irq = of_get_gpio(tty->dev->of_node, 0);
	if (!gpio_is_valid(bdev->wakeup_irq)) {
		BTMTK_ERR("[ERR] %s: uanble to get bt host wake gpio", __func__);
		return -1;
	}

	ret = gpio_request(bdev->wakeup_irq, "hostwake_gpio");
	if (unlikely(ret)) {
		BTMTK_ERR("[ERR] %s: request host wake gpio fail", __func__);
		return -1;
	}

	ret = gpio_direction_input(bdev->wakeup_irq);
	if (ret) {
		BTMTK_ERR("[ERR] %s: gpio_direction_input fail: %d", __func__, ret);
		goto exit;
	}

	irq = gpio_to_irq(bdev->wakeup_irq);
#endif

	ret = request_irq(irq, btmtk_host_wake_isr, IRQF_TRIGGER_RISING,
					"bt_hostwake", (void *)bdev);

	if (ret) {
		BTMTK_ERR("[ERR] %s: request irq for bt hostwake fail: %d", __func__, ret);
		goto exit;
	}

	ret = enable_irq_wake(irq);
	if (ret) {
		BTMTK_ERR("%s: enable irq_wake failed ret = %d", __func__, ret);
		goto exit;
	}
	BTMTK_INFO("%s: request irq(%d) for bt hostwake done", __func__, irq);

	return 0;

exit:
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
	gpiod_put(bdev->gpio_cs);
#else
	gpio_free(bdev->wakeup_irq);
#endif
	return -1;

}


/*
 *******************************************************************************
 *			 bt notify fw blank state feature
 *******************************************************************************
 */

enum wmt_blank_state {
	// wmt parameter: 0(screen off) / 1(screen on)
	WMT_PARA_SCREEN_OFF = 0,
	WMT_PARA_SCREEN_ON = 1
};

int btmtk_intcmd_wmt_blank_status(unsigned char blank_state) {
	u8 cmd[] = { 0x01, 0x5D, 0xFC, 0x03, 0x00, 0x04, 0x00};
	u8 evt[] = {0x04, 0x0E, 0x07, 0x01, 0x5D, 0xFC};
	int ret;

	if (g_sbdev->dynamic_fwdl_start) {
		BTMTK_INFO("%s: fw doing re-download, skip", __func__);
		return 0;
	}

	BTMTK_INFO("%s: blank_state[%d]", __func__, blank_state);

	cmd[6] = blank_state;
	ret = btmtk_main_send_cmd(g_sbdev,
			cmd, sizeof(cmd), evt, sizeof(evt),
			DELAY_TIMES, RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);
	if (ret < 0)
		BTMTK_ERR("%s faill to set blank_state[%d] to fw", __func__, blank_state);

	return ret;
}


#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
int btmtk_disp_notify_cb(struct notifier_block *nb, unsigned long value, void *v)
{
	int *data = (int *)v;
	int32_t new_state = 0;
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();
	/*
		MTK_DISP_EARLY_EVENT_BLANK (0x00): This event will happen before lcm suspend/resume
		MTK_DISP_EVENT_BLANK (0x01): This event will happen after lcm suspend/resume
		MTK_DISP_BLANK_UNBLANK (0x00): which means display resume (power on)
		MTK_DISP_BLANK_POWERDOWN (0x01): which mean display suspend (power off)
	*/

	BTMTK_INFO("%s: before_after_blank[%ld], blank_power_down[%d], fops_state[%d], fw_log_on[%d]",
				__func__, value, *data, btmtk_fops_get_state(g_sbdev), bmain_info->fw_log_on);

	if (value == MTK_DISP_EARLY_EVENT_BLANK) {
		switch (*data) {
			case MTK_DISP_BLANK_UNBLANK:
				new_state = WMT_PARA_SCREEN_ON;
				break;
			case MTK_DISP_BLANK_POWERDOWN:
				new_state = WMT_PARA_SCREEN_OFF;
				break;
			default:
				BTMTK_DBG("%s: goto end", __func__);
				goto end;
		}

		if (btmtk_fops_get_state(g_sbdev) == BTMTK_FOPS_STATE_OPENED && bmain_info->fw_log_on) {
			BTMTK_DBG("%s: blank state [%d]->[%d], and send cmd", __func__, g_sbdev->blank_state, new_state);
			g_sbdev->blank_state = new_state;
			btmtk_intcmd_wmt_blank_status(g_sbdev->blank_state);
		} else {
			BTMTK_DBG("%s: blank state [%d]->[%d]", __func__, g_sbdev->blank_state, new_state);
			g_sbdev->blank_state = new_state;
		}
	}
end:
	BTMTK_INFO("%s: end", __func__);
	return 0;
}

struct notifier_block btmtk_disp_notifier = {
	.notifier_call = btmtk_disp_notify_cb,
};
#endif

static void btmtk_fb_notify_register(void)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
	BTMTK_INFO("%s", __func__);
#if IS_ENABLED(CONFIG_DRM_MEDIATEK_V2)
	if (mtk_disp_notifier_register("btmtk_disp_notifier", &btmtk_disp_notifier)) {
		BTMTK_ERR("%s: Register mtk_disp_notifier failed", __func__);
	}
#endif
#endif

}

static void btmtk_fb_notify_unregister(void)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
#if IS_ENABLED(CONFIG_DRM_MEDIATEK_V2)
	BTMTK_INFO("%s", __func__);
	mtk_disp_notifier_unregister(&btmtk_disp_notifier);
#endif
#endif
}

#define PLATFORM_LEN 4
void btmtk_platform_prop_register(struct btmtk_dev *bdev) {
	struct btmtk_uart_dev *cif_dev = NULL;
	char platform_str[PLATFORM_LEN + 1] = {0};
	struct device_node *root_node;
	const char *model, *mt_ptr;
	int ret;
	u32 platform;

	BTMTK_DBG("%s start", __func__);

	if (!bdev) {
		BTMTK_ERR("[ERR] bdev is NULL");
		return;
	}

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;
	if (!cif_dev) {
		BTMTK_ERR("[ERR] cif_dev is NULL");
		return;
	}

	/* get root node of dts */
	root_node = of_find_node_by_path("/");
	if (!root_node) {
		BTMTK_ERR("[ERR] %s: root node not found", __func__);
		return;
	}

	ret = of_property_read_string(root_node, "model", &model);
	if (ret < 0) {
		BTMTK_ERR("[ERR] %s: root_node model ret[%d]", __func__, ret);
		return;
	}

	BTMTK_INFO("%s: root_node model[%s]", __func__, model);

	/* dts module property is "....MTXXXX"
	 * so parsing the platform id XXXX after MT
	 */
	mt_ptr = strstr(model, "MT");
	if (mt_ptr != NULL) {
		if (strlen(mt_ptr) >= PLATFORM_LEN + 2) {
			strncpy(platform_str, mt_ptr + 2, PLATFORM_LEN);
			platform_str[PLATFORM_LEN] = '\0';
			BTMTK_INFO("%s: platform[%s]", __func__, platform_str);
		} else {
			BTMTK_WARN("%s: can not find platform id in dts root node", __func__);
			return;
		}
	} else {
		BTMTK_WARN("%s: can not find MTXXXX in dts root node", __func__);
		return;
	}

	/* str to u32 */
	ret = kstrtou32(platform_str, 0, &platform);
	if (ret) {
		BTMTK_WARN("%s: convert platform string failed ret[%d]", __func__, ret);
		return;
	}

	/* register platform data */
	switch (platform) {
		case 6985:
			g_platform_prop = &MT6985_prop;
			break;
		case 6989:
			BTMTK_INFO("%s: platform[%u] not create yet", __func__, platform);
			//g_platform_prop = &MT6989_prop;
			break;
		case 6991:
			BTMTK_INFO("%s: platform[%u]", __func__, platform);
			g_platform_prop = &MT6991_prop;
			break;
		default:
			BTMTK_WARN("%s: not recognize platform[%u]", __func__, platform);
			break;
	}
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
	btmtk_drv_cbs.fmd_cb = btmtk_fmd_cb;

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

	atomic_set(&bdev->poweroff_finder_mode, 0);
	bdev->dfd_value_addr_remap_base = NULL;
	tty->dev->of_node = of_find_compatible_node(NULL, NULL, "mediatek,mt6991-conn_scp");
	if (!tty->dev->of_node)
		BTMTK_ERR("[ERR] %s: mediatek,mt6991-conn_scp of_node not found", __func__);

	ret = of_property_read_u64(tty->dev->of_node, "dfd-value-addr", &bdev->dfd_value_addr);
	if (ret < 0)
		BTMTK_ERR("[ERR] %s: mediatek,mt6991-conn_scp dfd-value-addr ret[%d]", __func__, ret);
	BTMTK_INFO("%s: dfd-value-addr =  %llu", __func__, bdev->dfd_value_addr);

	ret = of_property_read_u32(tty->dev->of_node, "dfd-value-size", &bdev->dfd_value_size);
	if (ret < 0)
		BTMTK_ERR("[ERR] %s: mediatek,mt6991-conn_scp dfd-value-size ret[%d]", __func__, ret);
	BTMTK_INFO("%s: dfd-value-size =  0x%04x", __func__, bdev->dfd_value_size);

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

	if (strncmp(bdev->flavor_bin, "eap", strlen("eap")) == 0) {
		BTMTK_WARN("EAP project");
		bdev->is_eap = true;
	}

	pinctrl_ptr = devm_pinctrl_get(tty->dev);
	if (IS_ERR(pinctrl_ptr)) {
		BTMTK_ERR("[ERR] %s: fail to get bt pinctrl", __func__);
		//return -1;
	}

	/* if support PowerOffBLE, this operation would notify fw leave PowerOffBLE mode */
	btmtk_pinctrl_exec(BT_FIND_MY_PHONE_HIGH);
	usleep_range(5000, 5100);
	btmtk_pinctrl_exec(BT_FIND_MY_PHONE_LOW);

	if (bdev->is_eap && cif_dev->sleep_en) {
		ret = btmtk_register_wakeup_irq(bdev, tty);
		if (ret < 0)
			BTMTK_WARN("%s: btmtk_register_wakeup_irq fail", __func__);
	} else
		BTMTK_INFO("%s: not support fw wakeup irq", __func__);

	btmtk_platform_prop_register(bdev);

	/* set gpio to default */
	btmtk_set_gpio_default();

	btmtk_dev_link_uart();

	btmtk_fb_notify_register();

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
	btmtk_fb_notify_unregister();
	return connv3_sub_drv_ops_unregister(CONNV3_DRV_TYPE_BT);
}

/*
 *******************************************************************************
 *                       bt power throttling feature
 *******************************************************************************
 */
static inline bool btmtk_pwrctrl_support(void)
{
#if defined(BTMTK_PLAT_ALPS) && BTMTK_PLAT_ALPS
	return TRUE;
#else
	return FALSE;
#endif
}

static void btmtk_send_set_tx_power_cmd(struct btmtk_dev *bdev)
{
	struct btmtk_uart_dev *cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;
	struct btmtk_dypwr_st *dy_pwr = &cif_dev->dy_pwr;
	uint8_t cmd_set[6] = { 0x01, 0x2D, 0xFC, 0x02, 0x02, 0x00 };
	uint8_t evt_set[] = { 0x04, 0x0E, 0x06, 0x01, 0x2D, 0xFC };
	int ret = 0;

	if (btmtk_fops_get_state(bdev) != BTMTK_FOPS_STATE_OPENED) {
		BTMTK_WARN("%s: not in working state(%d) fops(%d)"
			, __func__, btmtk_get_chip_state(bdev), btmtk_fops_get_state(bdev));
		return;
	}

	if (g_sbdev->dynamic_fwdl_start) {
		BTMTK_INFO("%s: fw doing re-download, skip", __func__);
		return;
	}

	cmd_set[5] = dy_pwr->set_val;
	ret = btmtk_main_send_cmd(bdev, cmd_set, sizeof(cmd_set),
				evt_set, sizeof(evt_set), 0, RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);

	if (ret < 0) {
		BTMTK_ERR("%s: failed(%d)", __func__, ret);
		return;
	}

	if (bdev->io_buf == NULL) {
		BTMTK_ERR("%s: io_buf is NULL", __func__);
		return;
	}

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

	if (cif_dev == NULL) {
		BTMTK_ERR("%s: cif_dev is NULL", __func__);
		return -1;
	}

	if (g_sbdev->dynamic_fwdl_start) {
		BTMTK_INFO("%s: fw doing re-download, skip", __func__);
		return 0;
	}

	BTMTK_DBG("%s: lp_cur_lv[%d], dy_max_dbm[%d], dy_min_dbm[%d], lp_bdy_dbm[%d], fw_sel_dbm[%d]",
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
				evt_query, sizeof(evt_query), 0, RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);

	if (ret < 0)
		BTMTK_ERR("%s: failed(%d)", __func__, ret);

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
#if defined(BTMTK_PLAT_ALPS) && BTMTK_PLAT_ALPS
		/* power throttling limitation */
		if (dy_pwr->lp_cur_lv >= CONN_PWR_THR_LV_4) { //TODO_PWRCTRL
			dy_pwr->set_val = dy_pwr->lp_bdy_dbm;
			/* lp_bdy_dbm may be larger than dy_max_dbm, check again */
			if (dy_pwr->set_val > dy_pwr->dy_max_dbm)
				dy_pwr->set_val = dy_pwr->dy_max_dbm;
		}
#endif
	} else {
		BTMTK_INFO("%s: invalid dbm value, skip set cmd", __func__);
		return 1;
	}

	BTMTK_INFO("%s: set_val[%d], isblocking[%d]", __func__, dy_pwr->set_val, isblocking);

	dy_pwr->cb = cb;
	if (isblocking)
		btmtk_send_set_tx_power_cmd(bdev);
	else
		/* Don't block caller thread for set operation */
		schedule_work(&bdev->async_trx_work);
	return 0;
}

#if defined(BTMTK_PLAT_ALPS) && BTMTK_PLAT_ALPS
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
#endif
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
#if defined(BTMTK_PLAT_ALPS) && BTMTK_PLAT_ALPS
	cif_dev->dy_pwr.lp_cur_lv = CONN_PWR_THR_LV_0;
	conn_pwr_drv_pre_on(CONN_PWR_DRV_BT, &cif_dev->dy_pwr.lp_cur_lv);
#endif
	BTMTK_INFO("%s: lp_cur_bat_lv = %d", __func__, cif_dev->dy_pwr.lp_cur_lv);
	return 0;
}

void btmtk_pwrctrl_post_off(void)
{
	if (!btmtk_pwrctrl_support())
		return;
#if defined(BTMTK_PLAT_ALPS) && BTMTK_PLAT_ALPS
	conn_pwr_drv_post_off(CONN_PWR_DRV_BT);
#endif
}

void btmtk_pwrctrl_register_evt(void)
{
	if (!btmtk_pwrctrl_support())
		return;

	BTMTK_DBG("%s", __func__);
#if defined(BTMTK_PLAT_ALPS) && BTMTK_PLAT_ALPS
	/* Register callbacks for power throttling feature */
	conn_pwr_register_event_cb(CONN_PWR_DRV_BT, (CONN_PWR_EVENT_CB)btmtk_pwrctrl_level_change_cb);
#endif
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

	if (g_sbdev->dynamic_fwdl_start) {
		BTMTK_INFO("%s: fw doing re-download, skip", __func__);
		return 0;
	}

	ret = btmtk_main_send_cmd(g_sbdev, cmd, sizeof(cmd),
		evt, sizeof(evt), DELAY_TIMES, RETRY_TIMES, BTMTK_TX_PKT_SEND_NO_ASSERT, CMD_NO_NEED_FILTER);

	if (ret < 0)
		BTMTK_ERR("%s: failed(%d)", __func__, ret);

	return 0;
}

int32_t btmtk_send_finder_eids(uint8_t *buf, uint32_t len)
{
	int ret = 0;
	uint8_t evt[] = {0x04, 0x0E, 0x04, 0x01, 0x64, 0xFD};

	BTMTK_INFO("%s", __func__);
	if (g_sbdev->dynamic_fwdl_start) {
		BTMTK_INFO("%s: fw doing re-download, skip", __func__);
                return 0;
        }

	ret = btmtk_main_send_cmd(g_sbdev, buf, len,
		evt, sizeof(evt), DELAY_TIMES, RETRY_TIMES, BTMTK_TX_PKT_SEND_NO_ASSERT, CMD_NO_NEED_FILTER);

	if (ret < 0)
		BTMTK_ERR("%s: failed(%d)", __func__, ret);

	return ret;
}

/*******************************************************************************
*                           bt gpio dump
********************************************************************************
*/

#define GET_BIT(V, INDEX) ((V & (0x1U << INDEX)) >> INDEX)

void btmtk_dump_gpio_state_(struct bt_gpio *gpio, char *tag)
{
	unsigned int aux, dir, out, pu, pd;

	/* MT6985 example
	 * 0x1000_5000 (GPIO_BASE)
	 * 	0x0070	GPIO_DIR7
	 * 		0: Input; 1: Output
	 * 		GPIO_DIR register for GPIO224~GPIO241; GPIO242~GPIO255:
	 *		0: 224
	 *		1: 225
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
	 *		0: 224
	 *		1: 225
	 * 		16: 240
	 * 	0x04C0	GPIO_MODE30
	 * 		[2:0]    Aux mode of PAD_BT_UTXD
	 * 		000:B:GPIO224;
	 * 		001:O:UTXD3;
	 * 		010:O:UARTHUB_UART_TX;
	 * 		011:I1:URXD3;
	 * 		100:O:MD_MCIF_UTXD0;
	 * 		101:;110:;111:;
	 *
	 * 		[6:4]    Aux mode of PAD_BT_URXD
	 * 		000:B:GPIO225;
	 * 		001:I1:URXD3;
	 * 		010:I1:UARTHUB_UART_RX;
	 * 		011:O:UTXD3;
	 * 		100:I1:MD_MCIF_URXD0;
	 * 		101:;110:;111:;
	 */

	/* 0x11C0_0000 (IOCFG_RM_BASE)
	 * 	0x0040	PU
	 *		0: 225
	 *		1: 224
	 * 	0x0030	PD
	 *		0: 225
	 *		1: 224
	 */

	if (gpio->gpio_remap_base == NULL) {
		BTMTK_INFO("%s: [%s] gpio_remap_base is NULL", __func__, tag);
		gpio->gpio_remap_base = ioremap(gpio->gpio_base, gpio->remap_len);
	}

	if (gpio->gpio_remap_base == NULL) {
		BTMTK_ERR("%s: [%s] gpio_remap_base failed", __func__, tag);
		return;
	}

	if (gpio->pu_pd_remap_base == NULL)
		gpio->pu_pd_remap_base = ioremap(gpio->pu_pd_base, gpio->remap_len);

	if (gpio->pu_pd_remap_base == NULL) {
		BTMTK_ERR("%s:[%s] pu_pd_remap_base failed", __func__, tag);
		return;
	}

	aux = CONSYS_REG_READ(gpio->gpio_remap_base + gpio->aux.offset);
	dir = CONSYS_REG_READ(gpio->gpio_remap_base + gpio->dir.offset);
	out = CONSYS_REG_READ(gpio->gpio_remap_base + gpio->out.offset);
	pu = CONSYS_REG_READ(gpio->pu_pd_remap_base + gpio->pu.offset);
	pd = CONSYS_REG_READ(gpio->pu_pd_remap_base + gpio->pd.offset);
	BTMTK_DBG("%s: aux[0x%08x] dir[0x%08x] out[0x%08x]", __func__,aux, dir, out);
	if (atomic_read(&g_sbdev->assert_state)) {
		BTMTK_INFO("[%s] num[%d] aux[%d] dir[%s] out[%d] pu[%d] pd[%d]",
				tag, gpio->num,
				(((aux >> gpio->aux.bit) & 0x07)),
				(GET_BIT(dir, gpio->dir.bit) ? "OUT" : "IN "),
				GET_BIT(out, gpio->out.bit),
				GET_BIT(pu, gpio->pu.bit),
				GET_BIT(pd, gpio->pd.bit));
	} else {
		BTMTK_DBG("[%s] num[%d] aux[%d] dir[%s] out[%d] pu[%d] pd[%d]",
				tag, gpio->num,
				(((aux >> gpio->aux.bit) & 0x07)),
				(GET_BIT(dir, gpio->dir.bit) ? "OUT" : "IN "),
				GET_BIT(out, gpio->out.bit),
				GET_BIT(pu, gpio->pu.bit),
				GET_BIT(pd, gpio->pd.bit));
	}
}

void btmtk_dump_gpio_state(void)
{
	if (g_platform_prop == NULL) {
		BTMTK_WARN("%s: g_platform_prop not register yet", __func__);
		return;
	}
	btmtk_dump_gpio_state_(&g_platform_prop->rst_gpio, "RST_PIN");
	btmtk_dump_gpio_state_(&g_platform_prop->tx_gpio, "TX_GPIO");
	btmtk_dump_gpio_state_(&g_platform_prop->rx_gpio, "RX_GPIO");
}

void btmtk_dump_gpio_state_unmap_(struct bt_gpio *gpio, char *tag)
{
	BTMTK_INFO("%s: %s", __func__, tag);
	if (gpio->gpio_remap_base != NULL) {
		iounmap(gpio->gpio_remap_base);
		gpio->gpio_remap_base = NULL;
	}

	if (gpio->pu_pd_remap_base != NULL) {
		iounmap(gpio->pu_pd_remap_base);
		gpio->pu_pd_remap_base = NULL;
	}
}

void btmtk_dump_gpio_state_unmap(void)
{
	if (g_platform_prop == NULL) {
		BTMTK_WARN("%s: g_platform_prop not register yet", __func__);
		return;
	}
	btmtk_dump_gpio_state_unmap_(&g_platform_prop->rst_gpio, "RST_PIN");
	btmtk_dump_gpio_state_unmap_(&g_platform_prop->tx_gpio, "TX_GPIO");
	btmtk_dump_gpio_state_unmap_(&g_platform_prop->rx_gpio, "RX_GPIO");
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

void btmtk_query_fw_schedule_info(struct btmtk_dev *bdev)
{
	u8 cmd[] = { 0x01, 0x3A, 0xFC, 0x02, 0xA0, 0x00 };
	int ret = 0;

	BTMTK_INFO("%s", __func__);
	ret = btmtk_main_send_cmd(g_sbdev, cmd, sizeof(cmd), NULL, 0,
			DELAY_TIMES, RETRY_TIMES, BTMTK_TX_PKT_SEND_NO_ASSERT, CMD_NO_NEED_FILTER);
	if (ret < 0)
		BTMTK_ERR("%s: failed, ret[%d]", __func__, ret);
}

/*******************************************************************************
*                           bt find my phone mode
********************************************************************************
*/

int btmtk_find_my_phone_cmd(u32 hr)
{
	u8 cmd[] = { 0x01, 0x6F, 0xFC, 0x0B, 0x01, 0x17, 0x07, 0x00, 0x01, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00 };
	u8 evt[] = { 0x04, 0xE4, 0x0A, 0x02, 0x17, 0x06, 0x00, 0x00, 0x01, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00 };
	int ret = 0;

	BTMTK_INFO("%s set hr[%u]", __func__, hr);

	/* hr > 65536 means not limit time */
	if (hr < 65536) {
		cmd[13] = (hr & 0xFF);
		cmd[14] = (hr >> 8) & 0xFF;
	}

	if (g_sbdev->dynamic_fwdl_start) {
		BTMTK_INFO("%s: fw doing re-download, skip", __func__);
		return 0;
	}

	ret = btmtk_main_send_cmd(g_sbdev, cmd, sizeof(cmd), evt, sizeof(evt),
			DELAY_TIMES, RETRY_TIMES, BTMTK_TX_PKT_SEND_NO_ASSERT, CMD_NO_NEED_FILTER);
	if (ret < 0)
		BTMTK_ERR("%s: failed, ret[%d]", __func__, ret);
	return ret;
}

int btmtk_set_powered_off_finder_mode(bool enable)
{
        u8 cmd1[] = { 0x01, 0x64, 0xFD, 0x02, 0x01, 0x01 };
#if 0
        u8 evt1[] = { 0x04, 0x0E, 0x04, 0x01, 0x64, 0xFD, 0x00 };
        u8 cmd2[] = { 0x01, 0x64, 0xFD, 0x17, 0x05, 0x03, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
        u8 evt2[] = { 0x04, 0x0E, 0x04, 0x01, 0x64, 0xFD, 0x00 };
#endif
        int ret = 0;

	BTMTK_INFO("%s enable[%d]", __func__, enable);

        if (g_sbdev->dynamic_fwdl_start) {
                BTMTK_INFO("%s: fw doing re-download, skip", __func__);
                return 0;
        }

	/* cmd1 */
        ret = btmtk_main_send_cmd(g_sbdev, cmd1, sizeof(cmd1), NULL, 0,
                        DELAY_TIMES, RETRY_TIMES, BTMTK_TX_PKT_SEND_NO_ASSERT, CMD_NO_NEED_FILTER);
        if (ret < 0) {
                BTMTK_ERR("%s: cmd1 failed, ret[%d]", __func__, ret);
                return ret;
        }
#if 0
	/* cmd 2 */
        ret = btmtk_main_send_cmd(g_sbdev, cmd2, sizeof(cmd2), evt2, sizeof(evt2),
                        DELAY_TIMES, RETRY_TIMES, BTMTK_TX_PKT_SEND_NO_ASSERT, CMD_NO_NEED_FILTER);
        if (ret < 0) {
                BTMTK_ERR("%s: cmd2 failed, ret[%d]", __func__, ret);
                return ret;
        }
#endif
        return ret;
}

int btmtk_uart_launcher_deinit(void)
{
	int ret = 0;
	struct btmtk_uart_dev *cif_dev = NULL;
	u8 retry = 10;
	u8 count = 0;

	BTMTK_INFO("%s: Start", __func__);
	cancel_work_sync(&g_sbdev->pwr_on_uds_work);

	if (g_sbdev != NULL) {
		/* wait poweroff_finder_mode set to 0 in post_fmd_cb and make sure BT state is not DISCONNECT */
		while(atomic_read(&g_sbdev->poweroff_finder_mode) && (count <= retry)) {
			BTMTK_WARN("%s: poweroff_finder_mode = %d", __func__, atomic_read(&g_sbdev->poweroff_finder_mode));
			count++;
			mdelay(200);
		}
	}

	ret = bt_close(g_sbdev->hdev);

	if (ret) {
		BTMTK_ERR("%s: BT turn off fail!", __func__);
		return ret;
	}
	if (g_sbdev != NULL) {
		cif_dev = (struct btmtk_uart_dev *)g_sbdev->cif_dev;
		if (cif_dev && cif_dev->tty && cif_dev->tty->port
			&& cif_dev->tty->port->count == 0)
			cif_dev->tty->port->count = 1;
	}

	BTMTK_INFO("%s: BT turn off OK", __func__);

	return 0;
}

#endif // (USE_DEVICE_NODE == 1)
