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
