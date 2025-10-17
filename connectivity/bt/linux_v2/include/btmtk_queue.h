/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef __BTMTK_QUEUE_H__
#define __BTMTK_QUEUE_H__

#if (USE_DEVICE_NODE == 1)
/*******************************************************************************
*				  C O N S T A N T
********************************************************************************
*/
#define RING_BUFFER_SIZE		(32768)
#define DRIVER_CMD_CHECK		0

 /*******************************************************************************
 *			       D A T A	 T Y P E S
 ********************************************************************************
 */
typedef void (*BT_RX_EVENT_CB) (void);

struct bt_ring_buffer_mgmt {
	u8 buf[RING_BUFFER_SIZE];
	u32 write_idx;
	u32 read_idx;
	spinlock_t lock;
};

s32 rx_skb_enqueue(struct sk_buff *skb);
u8 btmtk_rx_data_valid(void);
void btmtk_register_rx_event_cb(BT_RX_EVENT_CB cb);

int32_t btmtk_receive_data(struct hci_dev *hdev, u8 *buf, u32 count);
int32_t btmtk_send_data(struct hci_dev *hdev, u8 *buf, u32 count);
void rx_queue_initialize(void);
void rx_queue_destroy(void);

#endif

#endif /* __BTMTK_QUEUE_H__ */
