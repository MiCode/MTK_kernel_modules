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
#ifndef _BTMTK_BTIF_H_
#define _BTMTK_BTIF_H_

#include <net/bluetooth/bluetooth.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/irqreturn.h>

#include "btmtk_define.h"

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/
/* Preamble length for HCI Command:
**      2-bytes for opcode and 1 byte for length
*/
#define HCI_CMD_PREAMBLE_SIZE   3

/* Preamble length for ACL Data:
**      2-byte for Handle and 2 byte for length
*/
#define HCI_ACL_PREAMBLE_SIZE   4

/* Preamble length for SCO Data:
**      2-byte for Handle and 1 byte for length
*/
#define HCI_SCO_PREAMBLE_SIZE   3

/* Preamble length for HCI Event:
**      1-byte for opcode and 1 byte for length
*/
#define HCI_EVT_PREAMBLE_SIZE   2

#define HCI_CMD_HDR_LEN			(HCI_CMD_PREAMBLE_SIZE + 1)
#define HCI_EVT_HDR_LEN			(HCI_EVT_PREAMBLE_SIZE)

/* WMT Event */
#define WMT_EVT_OFFSET			(HCI_EVT_HDR_LEN)
#define WMT_EVT_HDR_LEN			(4)

#define WAKE_LOCK_NAME_SIZE		(16)


#define MT_BGF2AP_BTIF_WAKEUP_IRQ_ID 	312 /* temp */
#define MT_BGF2AP_SW_IRQ_ID          	271 /* temp */

#define IRQ_NAME_SIZE			(20)
#define MAX_STATE_MONITORS		(2)

enum wmt_evt_result {
	WMT_EVT_SUCCESS,
	WMT_EVT_FAIL,
	WMT_EVT_INVALID
};

typedef enum wmt_pkt_dir {
	WMT_PKT_DIR_HOST_TO_CHIP = 1, /* command */
	WMT_PKT_DIR_CHIP_TO_HOST = 2  /* event */
} WMT_PKT_DIR_T;

typedef enum wmt_opcode {
	WMT_OPCODE_FUNC_CTRL = 0x06,
	WMT_OPCODE_RF_CAL = 0x14,
	WMT_OPCODE_MAX
} WMT_OPCODE_T;

enum bt_irq_type {
	BGF2AP_BTIF_WAKEUP_IRQ,
	BGF2AP_SW_IRQ,
	BGF2AP_IRQ_MAX
};

enum bt_state {
	FUNC_OFF = 0,
	TURNING_ON = 1,
	PRE_ON_AFTER_CAL = 2,
	FUNC_ON = 3,
	RESET_START = 4,
	RESET_END = 5
};

struct bt_irq_ctrl {
	uint32_t irq_num;
	uint8_t name[IRQ_NAME_SIZE];
	u_int8_t active;
	spinlock_t lock;
	unsigned long flags;
};

enum bt_reset_level {
	RESET_LEVEL_NONE,
	RESET_LEVEL_0_5,	/* subsys reset */
	RESET_LEVEL_0,		/* whole chip reset */
	RESET_LEVEL_MAX
};


struct bt_wake_lock {
	struct wakeup_source *ws;
	uint8_t name[WAKE_LOCK_NAME_SIZE];
};

enum bt_psm_state {
	PSM_ST_SLEEP,
	PSM_ST_NORMAL_TR,
	PSM_ST_MAX
};

struct bt_psm_ctrl {
	enum bt_psm_state state;
	u_int8_t sleep_flag;  /* sleep notify */
	u_int8_t wakeup_flag; /* wakeup notify */
	struct completion comp; /* request completion */
	u_int8_t result; /* request result */
	struct bt_wake_lock wake_lock;
	u_int8_t quick_ps;
};

typedef void (*BT_STATE_CHANGE_CB) (enum bt_state state);

struct wmt_pkt_param {
	/* Extensible union */
	union {
		struct {
			uint8_t subsys; /* subsys type:
					 00 - BT
					 01 - FM (obsolete)
					 02 - GPS (obsolete)
					 03 - WIFI (obsolete) */
			uint8_t on; /* 00 - off, 01 - on */
		} func_ctrl_cmd;

		struct {
			uint8_t status;
		} func_ctrl_evt;

		struct {
			uint8_t subop; /* sub operation:
					01 - calibration start (obsolete)
					02 - restore indication (obsolete)
					03 - backup */
		} rf_cal_cmd;

		struct {
			uint8_t status;
			uint8_t subop;
			uint8_t start_addr[4]; /* start address of cal data, little endian */
			uint8_t data_len[2];   /* cal data length, little endian */
			uint8_t ready_addr[4]; /* ready bit address, little endian */
		} rf_cal_evt;
	} u;
};

struct bt_rf_cal_data_backup {
	uint32_t start_addr; /* calibration data start address on sysram */
	uint32_t ready_addr; /* ready bit address on sysram to mark calibration data is ready */
	uint8_t *p_cache_buf; /* buffer to cache the calibration data */
	uint16_t cache_len; /* cached data length */
};

/*
 * WMT Packet Format
 *
 * NOTICE:
 *   It is recommanded to define each field in bytes, otherwise we should consider the
 *   memory alignment by compiler and use __attribute__((__packed__)) if needed.
 */
struct wmt_pkt_hdr {
	uint8_t dir;
	uint8_t opcode;
	uint8_t param_len[2]; /* 2 bytes length, little endian */
};


struct wmt_pkt {
	struct wmt_pkt_hdr hdr;
	struct wmt_pkt_param params;
};

struct bt_internal_cmd {
	uint8_t waiting_event;
	uint16_t pending_cmd_opcode;
	uint8_t wmt_opcode;
	struct completion comp; /* command completion */
	u_int8_t result; /* command result */
	struct wmt_pkt_param wmt_event_params;
};


struct btmtk_dev {
	struct hci_dev		*hdev;
	unsigned long		hdev_flags;

	/* BT state machine */
	enum bt_state		bt_state;

	/* Power state */
	struct bt_psm_ctrl	psm;

	/* Reset relative */
	int32_t			subsys_reset;
	struct completion 	rst_comp;
	enum bt_reset_level	rst_level;
	u_int8_t		rst_count;

	struct bt_rf_cal_data_backup cal_data;

	 unsigned long		tx_state;
#ifdef SUPPORT_BT_THREAD
	/* thread */
	struct task_struct	*tx_thread;
	wait_queue_head_t	tx_waitq;

	/* Tx quque */
	struct sk_buff_head	tx_queue;
#endif
	/* For rx queue */
	struct sk_buff		*rx_skb;
	u_int8_t		rx_ind; /* RX indication from Firmware */

	/* context for current pending command */
	struct bt_internal_cmd	internal_cmd;
	/* For internal command (generated in driver) handling */
	u_int8_t		event_intercept;

	/* event queue */
	struct sk_buff		*evt_skb;
	wait_queue_head_t	p_wait_event_q;

	/* cif info */
	struct platform_device	*pdev;

	/* coredump handle */
	void 			*coredump_handle;

	/* state change callback */
	BT_STATE_CHANGE_CB	state_change_cb[MAX_STATE_MONITORS];
};

#define BTMTK_GET_DEV(bdev) (&bdev->pdev->dev)

/* IRQ APIs */
int bt_request_irq(enum bt_irq_type irq_type);
void bt_enable_irq(enum bt_irq_type irq_type);
void bt_disable_irq(enum bt_irq_type irq_type);
void bt_free_irq(enum bt_irq_type irq_type);

/* external functions */
int32_t btmtk_btif_open(void);
int32_t btmtk_btif_close(void);
int32_t btmtk_tx_thread(void * arg);
int32_t btmtk_cif_dispatch_event(struct hci_dev *hdev, struct sk_buff *skb);


void btmtk_reset_init(void);



static inline void bt_wake_lock_init(struct bt_wake_lock *plock)
{
	if (plock) {
		plock->ws = wakeup_source_register(plock->name);
		if (!plock->ws)
			BTMTK_ERR("ERROR NO MEM\n");
	}
}

static inline void bt_wake_lock_deinit(struct bt_wake_lock *plock)
{
	if (plock && plock->ws)
		wakeup_source_unregister(plock->ws);
}

static inline void bt_hold_wake_lock(struct bt_wake_lock *plock)
{
	if (plock && plock->ws)
		__pm_stay_awake(plock->ws);
}

static inline void bt_hold_wake_lock_timeout(struct bt_wake_lock *plock, uint32_t ms)
{
	if (plock && plock->ws)
		__pm_wakeup_event(plock->ws, ms);
}

static inline void bt_release_wake_lock(struct bt_wake_lock *plock)
{
	if (plock && plock->ws)
		__pm_relax(plock->ws);
}

static inline void bt_psm_init(struct bt_psm_ctrl *psm)
{
	init_completion(&psm->comp);
	strncpy(psm->wake_lock.name, "bt_psm", 6);
	psm->wake_lock.name[6] = '\0';
	bt_wake_lock_init(&psm->wake_lock);
}

static inline void bt_psm_deinit(struct bt_psm_ctrl *psm)
{
	bt_wake_lock_deinit(&psm->wake_lock);
}

/**
 * Send cmd dispatch evt
 */
#define RETRY_TIMES 10
#define HCI_EV_VENDOR			0xff


int btmtk_cif_send_calibration(struct hci_dev *hdev);

int btmtk_cif_send_cmd(struct hci_dev *hdev, const uint8_t *cmd,
		const int32_t cmd_len, int32_t retry, int32_t endpoint, uint64_t tx_state);

#endif
