/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _BTMTK_PROJ_SP_H_
#define _BTMTK_PROJ_SP_H_

#if (USE_DEVICE_NODE == 1)

#if defined(BTMTK_PLAT_ALPS) && BTMTK_PLAT_ALPS
#include "conn_power_throttling.h"
#endif

/* connv3 API */
#include "connv3_debug_utility.h"
#include "connv3_mcu_log.h"
#include "connv3.h"
//#include "btmtk_uart_tty.h"

#define QUERY_FW_SCHEDULE_INTERVAL	(60 * 60 * 1000)	// 1 HR interval

#if IS_ENABLED(CONFIG_MTK_UARTHUB)
/* uarthub API */

enum UARTHUB_irq_err_type {
	//uarthub_unknown_irq_err = -1,
	dev0_crc_err = 0,
	dev1_crc_err,
	dev2_crc_err,
	dev0_tx_timeout_err,
	dev1_tx_timeout_err,
	dev2_tx_timeout_err,
	dev0_tx_pkt_type_err,
	dev1_tx_pkt_type_err,
	dev2_tx_pkt_type_err,
	dev0_rx_timeout_err,
	dev1_rx_timeout_err,
	dev2_rx_timeout_err,
	rx_pkt_type_err,
	intfhub_restore_err,
	intfhub_dev_rx_err,
	intfhub_dev0_tx_err,
	intfhub_dev1_tx_err,
	intfhub_dev2_tx_err,

	UARTHUB_irq_err_type_count,
};

typedef void (*UARTHUB_IRQ_CB) (unsigned int err_type);
extern int mtk8250_uart_hub_reset_flow_ctrl(void);
extern int mtk8250_uart_hub_enable_bypass_mode(int bypass);
extern int mtk8250_uart_hub_is_ready(void);
//extern int mtk8250_uart_hub_set_request(struct tty_struct *tty);
extern int mtk8250_uart_hub_clear_request(void);
extern int mtk8250_uart_hub_fifo_ctrl(int ctrl);
extern int mtk8250_uart_hub_dump_with_tag(const char *tag);
extern int mtk8250_uart_hub_reset(void);
extern int mtk8250_uart_hub_register_cb(UARTHUB_IRQ_CB irq_callback);
extern int mtk8250_uart_hub_assert_bit_ctrl(int ctrl);

extern int mtk8250_uart_hub_dev0_set_tx_request(struct tty_struct *tty);
extern int mtk8250_uart_hub_dev0_set_rx_request(void);
extern int mtk8250_uart_hub_dev0_clear_tx_request(void);
extern int mtk8250_uart_hub_dev0_clear_rx_request(struct tty_struct *tty);
extern int mtk8250_uart_hub_get_host_wakeup_status(void);
extern int mtk8250_uart_hub_get_bt_sleep_flow_hw_mech_en(void);
extern int mtk8250_uart_hub_set_bt_sleep_flow_hw_mech_en(int enable);
extern int mtk8250_uart_hub_get_host_awake_sta(int dev_index);
extern int mtk8250_uart_hub_set_host_awake_sta(int dev_index);
extern int mtk8250_uart_hub_clear_host_awake_sta(int dev_index);
extern int mtk8250_uart_hub_get_host_bt_awake_sta(int dev_index);
extern int mtk8250_uart_hub_get_cmm_bt_awake_sta(void);
extern int mtk8250_uart_hub_get_bt_awake_sta(void);
extern int mtk8250_uart_hub_bt_on_count_inc(void);
int btmtk_wakeup_uarthub(void);
void btmtk_release_uarthub(bool force);
#endif

#if IS_ENABLED(CONFIG_SUPPORT_UARTDBG)
extern int mtk8250_uart_dump(struct tty_struct *tty);
void mtk8250_uart_start_record(struct tty_struct *tty);
void mtk8250_uart_end_record(struct tty_struct *tty);
#endif

#define HCI_EVT_COMPLETE_EVT		0x0E
#define HCI_EVT_STATUS_EVT			0x0F
#define HCI_EVT_CC_STATUS_SUCCESS	0x00
#define HCI_CMD_DY_ADJ_PWR_QUERY	0x01
#define HCI_CMD_DY_ADJ_PWR_SET		0x02
#define BT_RHW_MAX_ERR_COUNT		(2)


/* ioctl */
#define COMBO_IOC_MAGIC				0xb0
#define COMBO_IOCTL_BT_HOST_DEBUG	_IOW(COMBO_IOC_MAGIC, 4, void*)
#define COMBO_IOCTL_BT_INTTRX		_IOW(COMBO_IOC_MAGIC, 5, void*)
#define COMBO_IOCTL_BT_FINDER_MODE	_IOW(COMBO_IOC_MAGIC, 6, void*)
#define IOCTL_BT_HOST_DEBUG_BUF_SIZE	(32)
#define IOCTL_BT_HOST_INTTRX_SIZE		(256)

typedef int (*BT_RX_EVT_HANDLER_CB) (uint8_t *buf, int len);

struct btmtk_dypwr_st {
	/* Power Throttling Feature */
	uint8_t buf[16];
	uint8_t len;
	int8_t set_val;
	int8_t dy_max_dbm;
	int8_t dy_min_dbm;
	int8_t lp_bdy_dbm;
	int8_t fw_sel_dbm;
	BT_RX_EVT_HANDLER_CB cb;
#if defined(BTMTK_PLAT_ALPS) && BTMTK_PLAT_ALPS
	enum conn_pwr_low_battery_level lp_cur_lv;
#else
	int8_t lp_cur_lv;
#endif
};

struct gpio_info {
	u16	offset;
	u8	bit;
};

struct bt_gpio {
	u8			num;
	u32			gpio_base;	/* for aux/dir/out */
	u32			pu_pd_base;	/* for pu/pd */
	u16			remap_len;
	void __iomem		*gpio_remap_base;
	void __iomem		*pu_pd_remap_base;
	struct gpio_info	aux;
	struct gpio_info	dir;
	struct gpio_info	out;
	struct gpio_info	pu;
	struct gpio_info	pd;
};

struct platform_prop {
	struct bt_gpio		tx_gpio;
	struct bt_gpio		rx_gpio;
	struct bt_gpio		rst_gpio;
};

typedef struct _CONNXO_CFG_PARAM_STRUCT_T
{
	uint8_t ucDataLenLSB;
	uint8_t ucDataLenMSB;
	uint8_t ucFreqC1Axm52m;
	uint8_t ucFreqC2Axm52m;
	uint8_t ucFreqC1Btm52m;
	uint8_t ucFreqC2Btm52m;
	uint8_t ucFreqC1Axm80m;
	uint8_t ucFreqC2Axm80m;
	uint8_t ucFreqC1Btm80m;
	uint8_t ucFreqC2Btm80m;
	uint8_t ucFreqC1Lpm52m;
	uint8_t ucFreqC2Lpm52m;
	uint8_t ucFreqC1Lpm80m;
	uint8_t ucFreqC2Lpm80m;
	uint8_t ucCompC1Axm52m;
	uint8_t ucCompC2Axm52m;
	uint8_t ucCompC1Btm52m;
	uint8_t ucCompC2Btm52m;
	uint8_t ucCompC1Axm80m;
	uint8_t ucCompC2Axm80m;
	uint8_t ucCompC1Btm80m;
	uint8_t ucCompC2Btm80m;
	uint8_t ucCompC1Lpm52m;
	uint8_t ucCompC2Lpm52m;
	uint8_t ucCompC1Lpm80m;
	uint8_t ucCompC2Lpm80m;
	uint8_t ucExt32Cal;
} CONNXO_CFG_PARAM_STRUCT, *P_CONNXO_CFG_PARAM_STRUCT;

void btmtk_async_trx_work(struct work_struct *work);
void btmtk_pwr_on_uds_work(struct work_struct *work);
int btmtk_pwrctrl_pre_on(struct btmtk_dev *bdev);
void btmtk_pwrctrl_post_off(void);
void btmtk_pwrctrl_register_evt(void);
int btmtk_query_tx_power(struct btmtk_dev *bdev, BT_RX_EVT_HANDLER_CB cb);
int btmtk_set_tx_power(struct btmtk_dev *bdev, int8_t req_val, BT_RX_EVT_HANDLER_CB cb);

void btmtk_query_fw_schedule_info(struct btmtk_dev *bdev);
int btmtk_read_pmic_state(struct btmtk_dev *bdev);

int btmtk_set_pcm_pin_mux(void);
int btmtk_set_uart_rx_aux(void);

int btmtk_set_gpio_default(void);
int btmtk_set_gpio_default_for_close(void);

int btmtk_pre_power_on_handler(void);

int btmtk_sp_whole_chip_reset(struct btmtk_dev *bdev);
int btmtk_sp_close(void);

int btmtk_connv3_sub_drv_init(struct btmtk_dev *bdev);
//int btmtk_connv3_sub_drv_init(struct platform_device *pdev);

int btmtk_connv3_sub_drv_deinit(void);

void btmtk_sp_coredump_start(void);
void btmtk_sp_coredump_end(void);
void btmtk_platform_driver_init(void);
void btmtk_platform_driver_deinit(void);


/* Debug sop api */
void btmtk_uart_sp_dump_debug_sop(struct btmtk_dev *bdev);
void btmtk_hif_sp_dump_debug_sop(struct btmtk_dev *bdev);
void btmtk_hif_dump_work(struct work_struct *work);

/* find my phone mode api */
int btmtk_find_my_phone_cmd(u32 hr);
int btmtk_set_powered_off_finder_mode(bool enable);
int btmtk_send_finder_eids(u8 *buf, u32 len);

void btmtk_dump_gpio_state(void);
void btmtk_dump_gpio_state_unmap(void);

int btmtk_uart_launcher_deinit(void);

int btmtk_intcmd_wmt_blank_status(unsigned char blank_state);

#endif // (USE_DEVICE_NODE == 1)
#endif
