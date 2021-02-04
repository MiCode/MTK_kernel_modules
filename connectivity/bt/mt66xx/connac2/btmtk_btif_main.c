/*
 *  Copyright (c) 2016,2017 MediaTek Inc.
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

#include <linux/interrupt.h>
#include <linux/irqreturn.h>
#include <linux/spinlock_types.h>
#include <linux/kthread.h>

#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of.h>

#include "conninfra.h"

#include "mtk_btif_exp.h"
#include "btmtk_config.h"
#include "btmtk_define.h"
#include "btmtk_main.h"
#include "btmtk_btif.h"
#include "btmtk_mt66xx_reg.h"
#include "connsys_debug_utility.h"

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/
#define LOG TRUE
#define MTKBT_BTIF_NAME			"mtkbt_btif"


#define BTIF_TX_RTY_DLY			(5) /*ms*/
#define BTIF_OWNER_NAME 		"CONSYS_BT"

#define POS_POLLING_RTY_LMT     	(10)
#define MAX_RESET_COUNT			(3)
/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/
uint8_t *p_conn_infra_base_addr;
uint8_t *p_bgfsys_base_addr;
static struct btmtk_dev *g_bdev;
static unsigned long g_btif_id; /* The user identifier to operate btif */

static struct platform_driver mtkbt_btif_driver = {
	.driver = {
		.name = MTKBT_BTIF_NAME,
		.owner = THIS_MODULE,
	},
	.probe = NULL,
	.remove = NULL,
};

static struct platform_device *mtkbt_btif_device;

static const uint8_t hci_preamble_table[] = {
	HCI_CMD_PREAMBLE_SIZE,
	HCI_ACL_PREAMBLE_SIZE,
	HCI_SCO_PREAMBLE_SIZE,
	HCI_EVT_PREAMBLE_SIZE
};

const static uint8_t HCI_EVT_HW_ERROR[] = {0x04, 0x10, 0x01, 0x00};

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
extern int32_t bgfsys_fw_own_clr(void);
extern int32_t bgfsys_fw_own_set(void);
extern int32_t btmtk_send_calibration_cmd(struct hci_dev *);

static void bt_report_hw_error()
{
	btmtk_recv(g_bdev->hdev, HCI_EVT_HW_ERROR, sizeof(HCI_EVT_HW_ERROR));
}

static int bt_pre_chip_rst_handler(void)
{
	u_int8_t is_1st_reset = (g_bdev->rst_count == 0) ? TRUE : FALSE;
	uint8_t *infracfg_ao = ioremap_nocache(CONN_INFRA_CFG_AO_BASE,
						    CONN_INFRA_CFG_AO_LENGTH);

	g_bdev->rst_level = RESET_LEVEL_0;

	/*
	 * 1. Trigger BT PAD_EINT to force FW coredump,
	 *    trigger coredump only if the first time reset
	 *    (compare with the case of subsys reset fail)
	 */
	if(is_1st_reset) {
		if (infracfg_ao)
			CLR_BIT(infracfg_ao + BGF_PAD_EINT, BIT(9));

		else {
			BTMTK_ERR("ioremap CONN_INFRA_CFG_AO_BASE fail");
			return -1;
		}

		/* 2. Wait for IRQ */
		if (!wait_for_completion_timeout(&g_bdev->rst_comp, msecs_to_jiffies(2000))) {
			BTMTK_ERR("uanble to get reset IRQ in 2000ms");
			return -1;
		}

		/* 3. Reset PAD_INT CR */
		if (infracfg_ao)
			SET_BIT(infracfg_ao + BGF_PAD_EINT, BIT(9));
	}


	/* 4. Do coredump */
	//connsys_coredump_start(g_bdev->coredump_handle);

	/* 5. Turn off BT */
	return g_bdev->hdev->close(g_bdev->hdev);
}

static int bt_post_chip_rst_handler(void)
{
	uint32_t ret = 0;
	ret = g_bdev->hdev->open(g_bdev->hdev);

	if (!ret)
		BTMTK_ERR("bt_post_chip_rst_handler open fail");
	else
		bt_report_hw_error();

	return ret;
}

static struct sub_drv_ops_cb bt_drv_cbs =
{
	.rst_cb.pre_whole_chip_rst = bt_pre_chip_rst_handler,
	.rst_cb.post_whole_chip_rst = bt_post_chip_rst_handler,
	//.pre_cal_cb.pwr_on_cb = bt_core_pre_power_on,
	//.pre_cal_cb.do_cal_cb = bt_core_do_calibration,
};


static uint32_t bt_receive_data_cb(const uint8_t *buf,
						uint32_t count)
{
	return btmtk_recv(g_bdev->hdev, buf, count);
}


/* btmtk_btif_open

 *
 *     Called when line discipline changed to HCI_UART.
 *
 * Arguments:
 *     tty    pointer to tty info structure
 * Return Value:
 *     0 if success, otherwise error code
 */
int32_t btmtk_btif_open(void)
{
	int32_t ret = 0;

	/* 1. Open BTIF */
	ret = mtk_wcn_btif_open(BTIF_OWNER_NAME, &g_btif_id);
	if (ret) {
		BTMTK_ERR("BT open BTIF failed(%d)", ret);
		return -1;
	}

	/* 2. Register rx callback */
	ret = mtk_wcn_btif_rx_cb_register(g_btif_id, (MTK_WCN_BTIF_RX_CB)bt_receive_data_cb);
	if (ret) {
		BTMTK_ERR("Register rx cb to BTIF failed(%d)", ret);
		mtk_wcn_btif_close(g_btif_id);
		return -1;
	}

	/* 3. Enter deeple idle */
	ret = mtk_wcn_btif_dpidle_ctrl(g_btif_id, TRUE);
	if (ret) {
		BTMTK_ERR("BTIF enter dpidle failed(%d)", ret);
		mtk_wcn_btif_close(g_btif_id);
		return -1;
	}

	BTMTK_INFO("BT open BTIF OK\n");
	return 0;
}

/* btmtk_btif_close()
 *
 *    Called when the line discipline is changed to something
 *    else, the tty is closed, or the tty detects a hangup.
 */
int32_t btmtk_btif_close()
{
	int32_t ret = 0;

	if (!g_btif_id) {
		BTMTK_ERR("NULL BTIF ID reference!");
		return 0;
	}

	ret = mtk_wcn_btif_close(g_btif_id);
	if (ret) {
		BTMTK_ERR("BT close BTIF failed(%d)", ret);
		return -1;
	}
	g_btif_id = 0;
	BTMTK_INFO("BT close BTIF OK");
	return 0;

}

#if SUPPORT_BT_THREAD
/* btmtk_btif_open

 *
 *     Called when line discipline changed to HCI_UART.
 *
 * Arguments:
 *     tty    pointer to tty info structure
 * Return Value:
 *     0 if success, otherwise error code
 */
static int32_t btmtk_btif_dpidle_ctrl(u_int8_t enable)
{
	int32_t ret = 0;

	if (!g_btif_id) {
		BTMTK_ERR("NULL BTIF ID reference!");
		return -1;
	}

	ret = mtk_wcn_btif_dpidle_ctrl(g_btif_id, (enum _ENUM_BTIF_DPIDLE_)enable);
	if (ret) {
		BTMTK_ERR("BTIF %s dpidle failed(%d)", enable ? "enter" : "exit", ret);
		return -1;
	}

	BTMTK_ERR("BTIF %s dpidle succeed", enable ? "enter" : "exit");
	return 0;
}
#endif

static int btmtk_btif_probe(struct platform_device *pdev)
{
#ifdef CONFIG_OF
	struct device_node *node = NULL;
	struct resource res;
#endif

	/* 1. allocate global context data */
	if (g_bdev == NULL) {
		g_bdev = kzalloc(sizeof(struct btmtk_dev), GFP_KERNEL);
		if (!g_bdev) {
			BTMTK_ERR("%s: alloc memory fail (g_data)", __func__);
			return -1;
		}
	}

	g_bdev->pdev = pdev;

	/* 2. Init HCI device */
	btmtk_allocate_hci_device(g_bdev, HCI_UART);
#if 1
	/* 3. Request memory mapping for connsys control register operation */
#ifdef CONFIG_OF
	node = of_find_compatible_node(NULL, NULL, "mediatek,bt");
	if (!node) {
		BTMTK_ERR("BT-OF: get bt device node fail");
		return -1;
	}

	if (of_address_to_resource(node, 0, &res)) {
		BTMTK_ERR("BT-OF:  of_address_to_resource 0 fail");
		return -1;
	} else {
		BTMTK_INFO("BT_OF: Conninfra base: start = 0x%08x, size = 0x%08x\n", res.start, resource_size(&res));
		p_conn_infra_base_addr = ioremap(res.start, resource_size(&res));
	}

	if (of_address_to_resource(node, 1, &res)) {
		BTMTK_ERR("BT-OF:  of_address_to_resource 1 fail");
		return -1;
	} else {
		BTMTK_INFO("BT_OF: bgfsys base: start = 0x%08x, size = 0x%08x", res.start, resource_size(&res));
		p_bgfsys_base_addr = ioremap(res.start, resource_size(&res));
	}
#else
	p_conn_infra_base_addr = ioremap(CONN_INFRA_BASE, CONN_INFRA_LENGTH);
	p_bgfsys_base_addr = ioremap(BGFSYS_BASE, BGFSYS_LENGTH);
#endif
#endif
	bt_psm_init(&g_bdev->psm);

	init_completion(&g_bdev->rst_comp);

	/* Finally register callbacks to conninfra driver */
	conninfra_sub_drv_ops_register(CONNDRV_TYPE_BT, &bt_drv_cbs);

	BTMTK_INFO("%s: Done", __func__);
	return 0;
}


static int btmtk_btif_remove(struct platform_device *pdev)
{
	conninfra_sub_drv_ops_unregister(CONNDRV_TYPE_BT);
	iounmap(p_conn_infra_base_addr);
	iounmap(p_bgfsys_base_addr);

	bt_psm_deinit(&g_bdev->psm);
	btmtk_free_hci_device(g_bdev, HCI_UART);

	kfree(g_bdev);
	return 0;
}

int btmtk_cif_register(void)
{
	int ret = -1;

	/* Todo */
	mtkbt_btif_driver.probe = btmtk_btif_probe;
	mtkbt_btif_driver.remove = btmtk_btif_remove;

	ret = platform_driver_register(&mtkbt_btif_driver);
	BTMTK_INFO("platform_driver_register ret = %d\n", ret);

	mtkbt_btif_device = platform_device_alloc(MTKBT_BTIF_NAME, 0);
	if (mtkbt_btif_device == NULL) {
		platform_driver_unregister(&mtkbt_btif_driver);
		BTMTK_ERR("platform_device_alloc device fail");
		return -1;
	}
	ret = platform_device_add(mtkbt_btif_device);
	if (ret) {
		platform_driver_unregister(&mtkbt_btif_driver);
		BTMTK_ERR("platform_device_add fail");

		return -1;
	}
	BTMTK_INFO("%s: Done", __func__);
	return 0;
}

int btmtk_cif_deregister(void)
{
	btmtk_btif_close();
	platform_driver_unregister(&mtkbt_btif_driver);
	platform_device_unregister(mtkbt_btif_device);

	return 0;
}

int32_t btmtk_cif_send_cmd(struct hci_dev *hdev, const uint8_t *cmd,
		const int32_t cmd_len, int32_t retry_limit, int32_t endpoint, uint64_t tx_state)
{
	int32_t ret = -1;
	uint32_t wr_count = 0;
	int32_t tx_len = cmd_len;
	int32_t retry = 0;

	BTMTK_INFO("btmtk_cif_send_cmd\n");
	BTMTK_INFO_RAW(cmd, cmd_len, "%s [%d]: ", __func__,cmd_len);
	if (!g_btif_id) {
		BTMTK_ERR("NULL BTIF ID reference!");
		return -1;
	}

	while (tx_len > 0 && retry < retry_limit) {
		if (retry++ > 0)
			msleep(BTIF_TX_RTY_DLY);

		ret = mtk_wcn_btif_write(g_btif_id, cmd, tx_len);
		BTMTK_INFO("BTIF write ret = (%d)", ret);
		if (ret < 0) {
			BTMTK_ERR("BTIF write failed(%d) on retry(%d)", ret, retry-1);
			return -1;
		}

		tx_len -= ret;
		wr_count += ret;
		cmd += ret;
	}
	BTMTK_INFO("written ret = %d", ret);
	return ret;
}


static int32_t _check_wmt_evt_over_hci(
		uint8_t *buffer,
		uint16_t len,
		uint8_t  expected_op,
		struct wmt_pkt_param *p_evt_params)
{
	struct wmt_pkt *p_wmt_evt;
	uint8_t opcode, sub_opcode;
	uint8_t status = 0xFF; /* reserved value for check error */
	uint16_t param_len;

	/* Sanity check */
	if (len < (HCI_EVT_HDR_LEN + WMT_EVT_HDR_LEN)) {
		BTMTK_ERR("Incomplete packet len %d for WMT event!\n", len);
		goto check_error;
	}

	p_wmt_evt = (struct wmt_pkt *)&buffer[WMT_EVT_OFFSET];
	if (p_wmt_evt->hdr.dir != WMT_PKT_DIR_CHIP_TO_HOST) {
		BTMTK_ERR("WMT direction %x error!\n", p_wmt_evt->hdr.dir);
		goto check_error;
	}

	opcode = p_wmt_evt->hdr.opcode;
	if (opcode != expected_op) {
		BTMTK_ERR("WMT OpCode 0x%x is unexpected!\n", opcode);
		goto check_error;
	}

	param_len = (p_wmt_evt->hdr.param_len[1] << 8) | (p_wmt_evt->hdr.param_len[0]);
	/* Sanity check */
	if (len < (HCI_EVT_HDR_LEN + WMT_EVT_HDR_LEN + param_len)) {
		BTMTK_ERR("Incomplete packet len %d for WMT event!\n", len);
		goto check_error;
	}

	switch (opcode) {
	case WMT_OPCODE_FUNC_CTRL:
		if (param_len != sizeof(p_wmt_evt->params.u.func_ctrl_evt)) {
			BTMTK_ERR("Unexpected event param len %d for WMT OpCode 0x%x!\n",
				      param_len, opcode);
			break;
		}
		status = p_wmt_evt->params.u.func_ctrl_evt.status;
		break;

	case WMT_OPCODE_RF_CAL:
		sub_opcode = p_wmt_evt->params.u.rf_cal_evt.subop;

		if (sub_opcode != 0x03) {
			BTMTK_ERR("Unexpected subop 0x%x for WMT OpCode 0x%x!\n",
				      sub_opcode, opcode);
			break;
		}

		if (param_len != sizeof(p_wmt_evt->params.u.rf_cal_evt)) {
			BTMTK_ERR("Unexpected event param len %d for WMT OpCode 0x%x!\n",
				      param_len, opcode);
			break;
		}
		status = p_wmt_evt->params.u.rf_cal_evt.status;
		break;

	default:
		BTMTK_ERR("Unknown WMT OpCode 0x%x!\n", opcode);
		break;
	}

	if (status != 0xFF) {
		memcpy(p_evt_params, &p_wmt_evt->params, param_len);
		return (status == 0) ? WMT_EVT_SUCCESS : WMT_EVT_FAIL;
	}

check_error:
	BTMTK_DBG_RAW(buffer, len, "Dump data: ");
	return WMT_EVT_INVALID;
}

int32_t btmtk_cif_dispatch_event(struct hci_dev *hdev, struct sk_buff *skb)
{
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);
	struct bt_internal_cmd *p_inter_cmd = &bdev->internal_cmd;
	uint8_t event_code = skb->data[0];

	/* WMT event, it's actualy 0xE4 */
	if (event_code == 0xFF) {
		int ret = _check_wmt_evt_over_hci(skb->data,
					      skb->len,
					      p_inter_cmd->wmt_opcode,
					      &p_inter_cmd->wmt_event_params);

		p_inter_cmd->result = (ret == WMT_EVT_SUCCESS) ? 1: 0;
	}
	return 0;
}

/* btmtk_cif_send_calibration

 *
 *     Prepare calibration data for BT.
 *
 * Arguments:
 *     tty    pointer to device info structure
 * Return Value:
 *     0 if success, otherwise error code
 */

int32_t btmtk_cif_send_calibration(struct hci_dev *hdev)
{
	return btmtk_send_calibration_cmd(hdev);
}


#if SUPPORT_BT_THREAD
static u_int8_t bt_tx_wait_for_msg(struct btmtk_dev *bdev)
{
	return (!skb_queue_empty(&bdev->tx_queue)
		|| bdev->rx_ind
		|| bdev->psm.sleep_flag
		|| bdev->psm.wakeup_flag
		|| kthread_should_stop());
}


int32_t btmtk_tx_thread(void * arg)
{
	struct btmtk_dev *bdev = (struct btmtk_dev *)arg;
	struct bt_psm_ctrl *psm = &bdev->psm;
	int32_t sleep_ret = 0, wakeup_ret = 0, ret;
	struct sk_buff *skb;
	int skb_len;

	BTMTK_INFO("btmtk_tx_thread start running...\n");

	do {
		BTMTK_INFO("entering  wait_event_interruptible\n");
		wait_event_interruptible(bdev->tx_waitq, bt_tx_wait_for_msg(bdev));
		BTMTK_INFO("btmtk_tx_thread wakeup\n");
		if (kthread_should_stop()) {
			BTMTK_INFO("bt_tx_worker_thread should stop now...\n");
			if (psm->state == PSM_ST_NORMAL_TR)
				bt_release_wake_lock(&psm->wake_lock);
			break;
		}

		switch (psm->state) {
		case PSM_ST_SLEEP:
			if (psm->sleep_flag) {
				psm->sleep_flag = FALSE;
				complete(&psm->comp);
				psm->result = sleep_ret;
			}
			/*
			 *  TX queue has pending data to send,
			 *    or F/W pull interrupt to indicate there's data to host,
			 *    or there's a explicit wakeup request.
			 *
			 *  All need to execute the Wakeup procedure.
			 */
			bt_hold_wake_lock(&psm->wake_lock);
			btmtk_btif_dpidle_ctrl(FALSE);

			bt_disable_irq(BGF2AP_BTIF_WAKEUP_IRQ);
			wakeup_ret = bgfsys_fw_own_clr();
			if (wakeup_ret) {
				BTMTK_ERR("FATAL: bgfsys_fw_own_clr error!! remain on SLEEP state");
				bt_enable_irq(BGF2AP_BTIF_WAKEUP_IRQ);
				btmtk_btif_dpidle_ctrl(TRUE);
				bt_release_wake_lock(&psm->wake_lock);
				break;
			} else {
				/* BGFSYS is awake and ready for data transmission */
				psm->state = PSM_ST_NORMAL_TR;
			}
#ifdef __SIMFEX__
		msleep(2000);
#endif
			break;

		case PSM_ST_NORMAL_TR:
			if (bdev->psm.wakeup_flag) {
				psm->wakeup_flag = TRUE;
				complete(&psm->comp);
				psm->result = wakeup_ret;
			}

			if (bdev->rx_ind) {
				/* Just reset the flag, F/W will send data to host after FW own clear */
				bdev->rx_ind = FALSE;
			}

			/*
			 *  Dequeue and send TX pending packets to bus
			 */
			 while(!skb_queue_empty(&bdev->tx_queue)) {
				skb = skb_dequeue(&bdev->tx_queue);
				if(skb == NULL)
					continue;

				/*
				 * make a copy of skb->len ot prevent skb being
				 * free after sending and recv event from FW
				 */
				skb_len = skb->len;
				ret = btmtk_cif_send_cmd(bdev->hdev, skb->data, skb->len, 5, 0, 0);
				if ((ret < 0) || (ret != skb_len)) {
					BTMTK_ERR("FATAL: TX packet error!! (%u/%d)", skb_len, ret);
					//if (p_hw_if)
						//p_hw_if->debug();
						break;
				}
			}
			/*
			 *  If Quick PS mode is enabled,
			 *    or there's a explicit sleep request.
			 *
			 *  We need to excecute the Sleep procedure.
			 *
			 *  For performance consideration, donot try to enter sleep during BT func on.
			 */
			if (bdev->bt_state == FUNC_ON &&
			    (psm->sleep_flag || psm->quick_ps)) {
				sleep_ret = bgfsys_fw_own_set();
				if (sleep_ret) {
					BTMTK_ERR("FATAL: bgfsys_fw_own_set error!! remain on NORMAL_TR state");
					break;
				} else {
					bt_enable_irq(BGF2AP_BTIF_WAKEUP_IRQ);
					mtk_wcn_btif_dpidle_ctrl(g_btif_id, TRUE);
					bt_release_wake_lock(&bdev->psm.wake_lock);
					psm->state = PSM_ST_SLEEP;
				}
			}
			break;

		default:
			BTMTK_ERR("FATAL: Unknown state %d!!", psm->state);
			break;
		}
	} while(1);

	return 0;
}
#endif

static struct work_struct rst_trigger_work;
static void bt_trigger_reset(struct work_struct *work)
{
	//u_int8_t is_1st_reset = (g_bdev->rst_count == 0) ? TRUE : FALSE;
	uint32_t ret = 0;
	uint8_t *infracfg_ao = ioremap_nocache(CONN_INFRA_CFG_AO_BASE,
						    CONN_INFRA_CFG_AO_LENGTH);

	/* 1. Reset PAD_INT CR */
	if (infracfg_ao)
		SET_BIT(infracfg_ao + BGF_PAD_EINT, BIT(9));
	else
		BTMTK_ERR("ioremap CONN_INFRA_CFG_AO_BASE fail");

	/* 2. Do coredump */
	//if (is_1st_reset)
	//connsys_coredump_start(g_bdev->coredump_handle);

	/* 3. BT off */
	ret = g_bdev->hdev->close(g_bdev->hdev);
	if (ret) {
		BTMTK_ERR("subsys reset close fail");
		goto error_reset;
	}

	/* 4. Bt On */
	ret = g_bdev->hdev->open(g_bdev->hdev);
	if (ret) {
		BTMTK_ERR("subsys reset open fail");
		goto error_reset;
	}

	bt_report_hw_error();
	return;

error_reset:
	if (g_bdev->rst_count < MAX_RESET_COUNT) {
		g_bdev->rst_count++;
		schedule_work(&rst_trigger_work);
	} else
		conninfra_trigger_whole_chip_rst(CONNDRV_TYPE_BT, "subsys reset fail");

}

void btmtk_reset_init(void)
{
	INIT_WORK(&rst_trigger_work, bt_trigger_reset);
}

static struct bt_irq_ctrl bgf2ap_btif_wakeup_irq = {.name = "BTIF_WAKEUP_IRQ"};
static struct bt_irq_ctrl bgf2ap_sw_irq = {.name = "BGF_SW_IRQ"};
static struct bt_irq_ctrl *bt_irq_table[BGF2AP_IRQ_MAX];

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/
static irqreturn_t btmtk_irq_handler(int irq, void * arg)
{
	if (irq == bgf2ap_btif_wakeup_irq.irq_num) {
		bt_disable_irq(BGF2AP_BTIF_WAKEUP_IRQ);
		g_bdev->rx_ind = TRUE;
		wake_up_interruptible(&g_bdev->tx_waitq);
		return IRQ_HANDLED;
	} else if (irq == bgf2ap_sw_irq.irq_num) {
		int32_t bgf_status = 0;

		bt_disable_irq(BGF2AP_SW_IRQ);
		g_bdev->bt_state = RESET_START;

		/* Read IRQ status CR to identify what happens */
		bgf_status = BGFSYS_REG_READL(BGF_SW_IRQ_STATUS);
#if 0 /* Confirm from FW, doesn't have such case */
		if (bgf_status & BGF_WHOLE_CHIP_RESET) {
			if(g_bdev->rst_level == RESET_LEVEL_0) {
				/* already whole chip resetting, discard */
				BTMTK_INFO("Already whole chip resetting, discard");
			} else {
				g_bdev->rst_level = RESET_LEVEL_0;
				schedule_work(&rst_trigger_work);
			}
		} else if(bgf_status & BGF_SUBSYS_CHIP_RESET) {
#endif

		/* FW notify host to get FW log */
		BTMTK_INFO("bgf_status = %d", bgf_status);
#ifdef __SIMFEX__
		if(bgf_status & BGF_FW_LOG_NOTIFY) {
			BGFSYS_CLR_BIT(BGF_SW_IRQ_RESET_ADDR, BGF_FW_LOG_NOTIFY);
			//connsys_log_irq_handler(CONN_DEBUG_TYPE_BT);
		} else if(bgf_status & BGF_SUBSYS_CHIP_RESET) {
#else
		if (g_bdev->rst_level != RESET_LEVEL_0) {
#endif
			/* FW trigger bt subsys reset */
			BTMTK_INFO("Trigger subsys reset from FW");
			BGFSYS_CLR_BIT(BGF_SW_IRQ_RESET_ADDR, BGF_SUBSYS_CHIP_RESET);
			g_bdev->rst_level = RESET_LEVEL_0_5;
			schedule_work(&rst_trigger_work);
		} else if (g_bdev->rst_level == RESET_LEVEL_0) {
			/* Trigger by other subsys case */
			complete(&g_bdev->rst_comp);
		}

		return IRQ_HANDLED;
	}
	return IRQ_NONE;
}

int32_t bt_request_irq(enum bt_irq_type irq_type)
{
	uint32_t irq_num = 0;
	int32_t ret = 0;
	unsigned long irq_flags = 0;
	struct bt_irq_ctrl *pirq = NULL;
#ifdef CONFIG_OF
	struct device_node *node = NULL;
#endif

	switch (irq_type) {
	case BGF2AP_BTIF_WAKEUP_IRQ:
#ifdef CONFIG_OF
		node = of_find_compatible_node(NULL, NULL, "mediatek,bt");
		if (node)
			irq_num = irq_of_parse_and_map(node, 0);
		else
			BTMTK_ERR("WIFI-OF: get bt device node fail");
#else
		irq_num = MT_BGF2AP_BTIF_WAKEUP_IRQ_ID;
#endif
		irq_flags = IRQF_SHARED; //IRQF_TRIGGER_LOW;
		pirq = &bgf2ap_btif_wakeup_irq;
		break;
	case BGF2AP_SW_IRQ:
#ifdef CONFIG_OF
		node = of_find_compatible_node(NULL, NULL, "mediatek,bt");
		if (node)
			irq_num = irq_of_parse_and_map(node, 1);
		else
			BTMTK_ERR("WIFI-OF: get bt device node fail");
#else
		irq_num = MT_BGF2AP_SW_IRQ_ID;
#endif
		irq_flags = IRQF_SHARED; //IRQF_TRIGGER_LOW;
		pirq = &bgf2ap_sw_irq;
		break;
	default:
		BTMTK_ERR("Invalid irq_type %d!", irq_type);
		return -EINVAL;
	}

	ret = request_irq(irq_num, btmtk_irq_handler, irq_flags,
			  pirq->name, pirq);
	if (ret) {
		BTMTK_ERR("Request %s (%u) failed! ret(%d)", pirq->name, irq_num, ret);
		return ret;
	}

	BTMTK_INFO("Request %s (%u) succeed", pirq->name, irq_num);
	bt_irq_table[irq_type] = pirq;
	pirq->irq_num = irq_num;
	pirq->active = TRUE;
	spin_lock_init(&pirq->lock);
	return 0;
}

void bt_enable_irq(enum bt_irq_type irq_type)
{
	struct bt_irq_ctrl *pirq;

	if (irq_type >= BGF2AP_IRQ_MAX) {
		BTMTK_ERR("Invalid irq_type %d!", irq_type);
		return;
	}

	pirq = bt_irq_table[irq_type];
	if (pirq) {
		spin_lock_irqsave(&pirq->lock, pirq->flags);
		if (pirq->active)
			BTMTK_DBG("%s (%u) has been enabled", pirq->name, pirq->irq_num);
		else {
			enable_irq(pirq->irq_num);
			pirq->active = TRUE;
			BTMTK_DBG("%s (%u) enabled", pirq->name, pirq->irq_num);
		}
		spin_unlock_irqrestore(&pirq->lock, pirq->flags);
	}
}

void bt_disable_irq(enum bt_irq_type irq_type)
{
	struct bt_irq_ctrl *pirq;

	if (irq_type >= BGF2AP_IRQ_MAX) {
		BTMTK_ERR("Invalid irq_type %d!", irq_type);
		return;
	}

	pirq = bt_irq_table[irq_type];
	if (pirq) {
		spin_lock_irqsave(&pirq->lock, pirq->flags);
		if (!pirq->active)
			BTMTK_DBG("%s (%u) has been disabled\n", pirq->name, pirq->irq_num);
		else {
			disable_irq_nosync(pirq->irq_num);
			pirq->active = FALSE;
			BTMTK_DBG("%s (%u) disabled\n", pirq->name, pirq->irq_num);
		}
		spin_unlock_irqrestore(&pirq->lock, pirq->flags);
	}
}

void bt_free_irq(enum bt_irq_type irq_type)
{
	struct bt_irq_ctrl *pirq;

	if (irq_type >= BGF2AP_IRQ_MAX) {
		BTMTK_ERR("Invalid irq_type %d!", irq_type);
		return;
	}

	pirq = bt_irq_table[irq_type];
	if (pirq) {
		free_irq(pirq->irq_num, pirq);
		pirq->active = FALSE;
		bt_irq_table[irq_type] = NULL;
	}
}
