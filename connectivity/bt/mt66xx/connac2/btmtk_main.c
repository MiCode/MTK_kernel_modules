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
#include <linux/notifier.h>
#include <linux/fb.h>

#include "btmtk_define.h"
#include "btmtk_main.h"
#include "btmtk_drv.h"
#include "btmtk_chip_if.h"

#define MTKBT_UNSLEEPABLE_LOCK(x, y)	spin_lock_irqsave(x, y)
#define MTKBT_UNSLEEPABLE_UNLOCK(x, y)	spin_unlock_irqsave(x, y)

/**
 * Global parameters(mtkbt_)
 */
uint8_t btmtk_log_lvl = BTMTK_LOG_LVL_DEF;

static int main_init(void)
{

	return 0;

}

static int main_exit(void)
{
	return 0;
}

/* HCI receive mechnism */


static inline struct sk_buff *h4_recv_buf(struct hci_dev *hdev,
					  struct sk_buff *skb,
					  const unsigned char *buffer,
					  int count,
					  const struct h4_recv_pkt *pkts,
					  int pkts_count)
{
	/* Check for error from previous call */
	if (IS_ERR(skb))
		skb = NULL;

	while (count) {
		int i, len;

		if (!skb) {
			for (i = 0; i < pkts_count; i++) {
				if (buffer[0] != (&pkts[i])->type)
					continue;

				skb = bt_skb_alloc((&pkts[i])->maxlen,
						   GFP_ATOMIC);
				if (!skb)
					return ERR_PTR(-ENOMEM);

				hci_skb_pkt_type(skb) = (&pkts[i])->type;
				hci_skb_expect(skb) = (&pkts[i])->hlen;
				break;
			}

			/* Check for invalid packet type */
			if (!skb)
				return ERR_PTR(-EILSEQ);

			count -= 1;
			buffer += 1;
		}

		len = min_t(uint, hci_skb_expect(skb) - skb->len, count);
		memcpy(skb_put(skb, len), buffer, len);
		/*
			If kernel version > 4.x
			skb_put_data(skb, buffer, len);
		*/

		count -= len;
		buffer += len;

		/* Check for partial packet */
		if (skb->len < hci_skb_expect(skb))
			continue;

		for (i = 0; i < pkts_count; i++) {
			if (hci_skb_pkt_type(skb) == (&pkts[i])->type)
				break;
		}

		if (i >= pkts_count) {
			kfree_skb(skb);
			return ERR_PTR(-EILSEQ);
		}

		if (skb->len == (&pkts[i])->hlen) {
			u16 dlen;

			switch ((&pkts[i])->lsize) {
			case 0:
				/* No variable data length */
				dlen = 0;
				break;
			case 1:
				/* Single octet variable length */
				dlen = skb->data[(&pkts[i])->loff];
				hci_skb_expect(skb) += dlen;

				if (skb_tailroom(skb) < dlen) {
					kfree_skb(skb);
					return ERR_PTR(-EMSGSIZE);
				}
				break;
			case 2:
				/* Double octet variable length */
				dlen = get_unaligned_le16(skb->data +
							  (&pkts[i])->loff);
				/* parse ISO packet len*/
				if ((&pkts[i])->type == HCI_ISODATA_PKT) {
					unsigned char *cp = (unsigned char *)&dlen + 1;
					*cp = *cp & 0x3F;
				}
				hci_skb_expect(skb) += dlen;

				if (skb_tailroom(skb) < dlen) {
					kfree_skb(skb);
					return ERR_PTR(-EMSGSIZE);
				}
				break;
			default:
				/* Unsupported variable length */
				kfree_skb(skb);
				return ERR_PTR(-EILSEQ);
			}

			if (!dlen) {
				/* No more data, complete frame */
				(&pkts[i])->recv(hdev, skb);
				skb = NULL;
			}
		} else {
			/* Complete frame */
			(&pkts[i])->recv(hdev, skb);
			skb = NULL;
		}
	}

	return skb;
}

static const struct h4_recv_pkt mtk_recv_pkts[] = {
	{ H4_RECV_ACL,      .recv = btmtk_recv_acl },
	{ H4_RECV_SCO,      .recv = hci_recv_frame },
	{ H4_RECV_EVENT,    .recv = btmtk_recv_event },
	{ H4_RECV_ISO,      .recv = btmtk_recv_iso },
};
#if ENABLESTP
static inline struct sk_buff *mtk_add_stp(struct btmtk_dev *bdev, struct sk_buff *skb)
{
	struct mtk_stp_hdr *shdr;
	int dlen, err = 0, type = 0;
	u8 stp_crc[] = {0x00, 0x00};

	if (unlikely(skb_headroom(skb) < sizeof(*shdr)) ||
		(skb_tailroom(skb) < MTK_STP_TLR_SIZE)) {
		BTMTK_DBG("%s, add pskb_expand_head, headroom = %d, tailroom = %d",
				__func__, skb_headroom(skb), skb_tailroom(skb));

		err = pskb_expand_head(skb, sizeof(*shdr), MTK_STP_TLR_SIZE,
					   GFP_ATOMIC);
	}
	dlen = skb->len;
	shdr = (void *) skb_push(skb, sizeof(*shdr));
	shdr->prefix = 0x80;
	shdr->dlen = cpu_to_be16((dlen & 0x0fff) | (type << 12));
	shdr->cs = 0;
	// Add the STP trailer
	// kernel version > 4.20
	// skb_put_zero(skb, MTK_STP_TLR_SIZE);
	// kernel version < 4.20
	skb_put(skb, sizeof(stp_crc));

	return skb;
}

static const unsigned char *
mtk_stp_split(struct btmtk_dev *bdev, const unsigned char *data, int count,
	      int *sz_h4)
{
	struct mtk_stp_hdr *shdr;

	/* The cursor is reset when all the data of STP is consumed out */
	if (!bdev->stp_dlen && bdev->stp_cursor >= 6) {
		bdev->stp_cursor = 0;
		BTMTK_ERR("reset cursor = %d\n", bdev->stp_cursor);
	}

	/* Filling pad until all STP info is obtained */
	while (bdev->stp_cursor < 6 && count > 0) {
		bdev->stp_pad[bdev->stp_cursor] = *data;
		BTMTK_ERR("fill stp format (%02x, %d, %d)\n",
		   bdev->stp_pad[bdev->stp_cursor], bdev->stp_cursor, count);
		bdev->stp_cursor++;
		data++;
		count--;
	}

	/* Retrieve STP info and have a sanity check */
	if (!bdev->stp_dlen && bdev->stp_cursor >= 6) {
		shdr = (struct mtk_stp_hdr *)&bdev->stp_pad[2];
		bdev->stp_dlen = be16_to_cpu(shdr->dlen) & 0x0fff;
		BTMTK_ERR("stp format (%02x, %02x)",
			   shdr->prefix, bdev->stp_dlen);

		/* Resync STP when unexpected data is being read */
		if (shdr->prefix != 0x80 || bdev->stp_dlen > 2048) {
			BTMTK_ERR("stp format unexpect (%02x, %02x)",
				   shdr->prefix, bdev->stp_dlen);
			BTMTK_ERR("reset cursor = %d\n", bdev->stp_cursor);
			bdev->stp_cursor = 2;
			bdev->stp_dlen = 0;
		}
	}

	/* Directly quit when there's no data found for H4 can process */
	if (count <= 0)
		return NULL;

	/* Tranlate to how much the size of data H4 can handle so far */
	*sz_h4 = min_t(int, count, bdev->stp_dlen);

	/* Update the remaining size of STP packet */
	bdev->stp_dlen -= *sz_h4;

	/* Data points to STP payload which can be handled by H4 */
	return data;
}
#endif

int btmtk_recv(struct hci_dev *hdev, const u8 *data, size_t count)
{
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);
	int err;
#if ENABLESTP
	const unsigned char *p_left = data, *p_h4 = NULL;
	int sz_left = count, sz_h4 = 0, adv = 0;

	while (sz_left > 0) {
		/*  The serial data received from MT7622 BT controller is
		 *  at all time padded around with the STP header and tailer.
		 *
		 *  A full STP packet is looking like
		 *   -----------------------------------
		 *  | STP header  |  H:4   | STP tailer |
		 *   -----------------------------------
		 *  but it doesn't guarantee to contain a full H:4 packet which
		 *  means that it's possible for multiple STP packets forms a
		 *  full H:4 packet that means extra STP header + length doesn't
		 *  indicate a full H:4 frame, things can fragment. Whose length
		 *  recorded in STP header just shows up the most length the
		 *  H:4 engine can handle currently.
		 */
		p_h4 = mtk_stp_split(bdev, p_left, sz_left, &sz_h4);
		if (!p_h4)
			break;

		adv = p_h4 - p_left;
		sz_left -= adv;
		p_left += adv;
		bdev->rx_skb = h4_recv_buf(bdev->hdev, bdev->rx_skb, p_h4,
					   sz_h4, mtk_recv_pkts,
					   ARRAY_SIZE(mtk_recv_pkts));

		if (IS_ERR(bdev->rx_skb)) {
			err = PTR_ERR(bdev->rx_skb);
			BTMTK_ERR("Frame reassembly failed (%d)", err);
			bdev->rx_skb = NULL;
			return err;
		}

		sz_left -= sz_h4;
		p_left += sz_h4;
	}
#else
	bdev->rx_skb = h4_recv_buf(hdev, bdev->rx_skb, data,
				   count, mtk_recv_pkts,
				   ARRAY_SIZE(mtk_recv_pkts));

	if (IS_ERR(bdev->rx_skb)) {
		err = PTR_ERR(bdev->rx_skb);
		BTMTK_ERR("Frame reassembly failed (%d)", err);
		bdev->rx_skb = NULL;
		return err;
	}
#endif

	return 0;
}
int btmtk_dispatch_acl(struct hci_dev *hdev, struct sk_buff *skb)
{
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);

	if (skb->data[0]== 0x6f && skb->data[1]== 0xfc && skb->len > 12) {
		/* coredump data done
		 * For Example : TotalTimeForDump=0xxxxxxx, (xx secs)
		 */
		if (skb->data[4]== 0x54 && skb->data[5] == 0x6F &&
			skb->data[6]== 0x74 && skb->data[7] == 0x61 &&
			skb->data[8]== 0x6C && skb->data[9] == 0x54 &&
			skb->data[10]== 0x69 && skb->data[11] == 0x6D &&
			skb->data[12]== 0x65) {
			/* coredump end, do reset */
			BTMTK_INFO("%s coredump done", __func__);
			msleep(3000);
			bdev->subsys_reset= 1;
		}
		return 1;
	} else if (skb->data[0]== 0xff && skb->data[1] == 0x05) {
		BTMTK_DBG("%s correct picus log by ACL", __func__);
	}
	return 0;
}

int btmtk_dispatch_event(struct hci_dev *hdev, struct sk_buff *skb)
{

	/* For Picus */
	if (skb->data[0]== 0xff && skb->data[2] == 0x50) {
		BTMTK_DBG("%s correct picus log format by EVT", __func__);
	}
	btmtk_cif_dispatch_event(hdev, skb);

	return 0;
}

int btmtk_recv_acl(struct hci_dev *hdev, struct sk_buff *skb)
{
	int err = 0, skip_pkt = 0;

	skip_pkt = btmtk_dispatch_acl(hdev, skb);
	if(skip_pkt == 0)
		err = hci_recv_frame(hdev, skb);

	return 0;
}


int btmtk_recv_event(struct hci_dev *hdev, struct sk_buff *skb)
{
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);
	struct hci_event_hdr *hdr = (void *)skb->data;
	int err = 0, skip_pkt = 0;

	/* Fix up the vendor event id with 0xff for vendor specific instead
	 * of 0xe4 so that event send via monitoring socket can be parsed
	 * properly.
	 */
	if (hdr->evt == 0xe4) {
		BTMTK_DBG("%s hdr->evt is %02x", __func__, hdr->evt);
		hdr->evt = HCI_EV_VENDOR;
	}

	/* When someone waits for the WMT event, the skb is being cloned
	 * and being processed the events from there then.
	 */
	if (test_bit(BTMTKUART_TX_WAIT_VND_EVT, &bdev->tx_state)) {
		bdev->evt_skb = skb_clone(skb, GFP_KERNEL);

		if (!bdev->evt_skb) {
			err = -ENOMEM;
			BTMTK_ERR("%s WMT event, clone to evt_skb failed, err = %d", __func__, err);
			goto err_out;
		}

		if (test_and_clear_bit(BTMTKUART_TX_WAIT_VND_EVT, &bdev->tx_state)) {
			BTMTK_DBG("%s clear bit BTMTKUART_TX_WAIT_VND_EVT", __func__);
			skip_pkt = btmtk_dispatch_event(hdev, skb);
			wake_up(&bdev->p_wait_event_q);
			BTMTK_DBG("%s wake_up p_wait_event_q", __func__);
		}
		goto err_out;
	}
	BTMTK_DBG_RAW(skb->data, skb->len, "%s, recv evt(hci_recv_frame)", __func__);

	if(skip_pkt == 0)
		err = hci_recv_frame(hdev, skb);

	if (err < 0) {
		BTMTK_ERR("%s hci_recv_failed, err = %d", __func__, err);
		goto err_free_skb;
	}

	return 0;

err_free_skb:
	kfree_skb(bdev->evt_skb);
	bdev->evt_skb = NULL;

err_out:
	return err;
}

int btmtk_recv_iso(struct hci_dev *hdev, struct sk_buff *skb)
{
	int err = 0;

#if (USE_DEVICE_NODE == 0)
	err = hci_recv_frame(hdev, skb);
#else
	err = rx_skb_enqueue(skb);
#endif
	return err;
}


int btmtk_main_send_cmd(struct hci_dev *hdev, const uint8_t *cmd, const int cmd_len, const int tx_state)
{
	struct sk_buff *skb = NULL;
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);
	int ret = 0;

	skb = alloc_skb(cmd_len + BT_SKB_RESERVE, GFP_ATOMIC);
	if (skb == NULL) {
		BTMTK_ERR("%s allocate skb failed!!", __func__);
		goto err_free_skb;
	}
	/* Reserv for core and drivers use */
	skb_reserve(skb , 7);
	bt_cb(skb)->pkt_type = HCI_COMMAND_PKT;
	memcpy(skb->data, cmd, cmd_len);
	skb->len = cmd_len;

#if ENABLESTP
	skb = mtk_add_stp(bdev, skb);
#endif

	if (skb->len < 30)
		BTMTK_DBG_RAW(skb->data, skb->len, "%s, send, len = %d", __func__, skb->len);

	set_bit(tx_state, &bdev->tx_state);
#if SUPPORT_BT_THREAD
	skb_queue_tail(&bdev->tx_queue, skb);
	wake_up_interruptible(&bdev->tx_waitq);
#else
	btmtk_cif_send_cmd(hdev, skb->data, skb->len, 5, 0, 0);
#endif
	ret = wait_event_timeout(bdev->p_wait_event_q,
			bdev->evt_skb != NULL || tx_state == BTMTKUART_TX_SKIP_VENDOR_EVT, 2*HZ);

err_free_skb:
	kfree_skb(skb);
	kfree_skb(bdev->evt_skb);
	bdev->evt_skb = NULL;
	return ret;
}

void btmtk_load_code_from_bin(u8 **image, char *bin_name,
					 struct device *dev, u32 *code_len)
{
	const struct firmware *fw_entry;
	int err = 0;
	int retry = 10;

	do {
		err = request_firmware(&fw_entry, bin_name, dev);
		if (err == 0) {
			break;
		} else if (retry <= 0) {
			*image = NULL;
			BTMTK_ERR("%s: request_firmware %d times fail!!! err = %d", __func__, 10, err);
			return;
		}
		BTMTK_ERR("%s: request_firmware fail!!! err = %d, retry = %d", __func__, err, retry);
		msleep(100);
	} while (retry-- > 0);

	*image = kzalloc(fw_entry->size, GFP_KERNEL);
	if (*image == NULL) {
		BTMTK_ERR("%s: kzalloc failed!! error code = %d", __func__, err);
		return;
	}

	memcpy(*image, fw_entry->data, fw_entry->size);
	*code_len = fw_entry->size;

	release_firmware(fw_entry);
}


static int btmtk_calibration_flow(struct hci_dev *hdev)
{
	btmtk_cif_send_calibration(hdev);
	BTMTK_INFO("%s done", __func__);
	return 0;
}

#if ENABLESTP
static int btmtk_send_set_stp_cmd(struct hci_dev *hdev)
{
	u8 cmd[] = { 0x01, 0x6F, 0xFC, 0x09, 0x01, 0x04, 0x05, 0x00, 0x03, 0x11, 0x0E, 0x00, 0x00};
	/* To-Do, for event check */
	/* u8 event[] = { 0x04, 0xE4, 0x06, 0x02, 0x04, 0x02, 0x00, 0x00, 0x03}; */
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);

	btmtk_main_send_cmd(hdev, cmd, sizeof(cmd), BTMTKUART_TX_WAIT_VND_EVT);

	BTMTK_INFO("%s done", __func__);
	return 0;
}

static int btmtk_send_set_stp1_cmd(struct hci_dev *hdev)
{
	u8 cmd[] = {0x01, 0x6F, 0xFC, 0x0C, 0x01, 0x08, 0x08, 0x00, 0x02, 0x01, 0x00, 0x01, 0x08, 0x00, 0x00, 0x80};
	/* To-Do, for event check */
	/* u8 event[] = {0x04, 0xE4, 0x10, 0x02, 0x08,
			0x0C, 0x00, 0x00, 0x00, 0x00, 0x01, 0x08, 0x00, 0x00, 0x80, 0x63, 0x76, 0x00, 0x00}; */
	//struct btmtk_dev *bdev = hci_get_drvdata(hdev);

	btmtk_main_send_cmd(hdev, cmd, sizeof(cmd), BTMTKUART_TX_WAIT_VND_EVT);

	BTMTK_INFO("%s done", __func__);
	return 0;
}
#endif

static int btmtk_fb_notifier_callback(struct notifier_block
				*self, unsigned long event, void *data)
{
	struct fb_event *evdata = data;
	int32_t blank = 0;

	if ((event != FB_EVENT_BLANK))
		return 0;

	blank = *(int32_t *)evdata->data;
	switch (blank) {
	case FB_BLANK_UNBLANK:
		break;
	case FB_BLANK_POWERDOWN:
		break;
	default:
		break;

	}

	return 0;
}

static struct notifier_block bt_fb_notifier;
static int btmtk_fb_notify_register(struct hci_dev *hdev)
{
	int32_t ret;

	bt_fb_notifier.notifier_call = btmtk_fb_notifier_callback;

	ret = fb_register_client(&bt_fb_notifier);
	if (ret)
		BTMTK_ERR("Register wlan_fb_notifier failed:%d\n", ret);
	else
		BTMTK_DBG("Register wlan_fb_notifier succeed\n");

	return ret;
}

static void btmtk_fb_notify_unregister(void)
{
	fb_unregister_client(&bt_fb_notifier);
}

/**
 * Kernel HCI Interface Registeration
 */
static int bt_flush(struct hci_dev *hdev)
{
#if SUPPORT_BT_THREAD
	struct btmtk_dev *bdev =  hci_get_drvdata(hdev);

	skb_queue_purge(&bdev->tx_queue);
#endif
	return 0;
}

int bt_close(struct hci_dev *hdev)
{
	BTMTK_INFO("%s", __func__);
	/* 1. Check state */

	/* 2. Ungreister screen on/off notify */
	btmtk_fb_notify_unregister();

	/* 3. Send WMT cmd to set BT off */
	btmtk_send_wmt_power_off_cmd(hdev);
	clear_bit(HCI_RUNNING, &hdev->flags);

	/* 4. Power off */
	btmtk_set_power_off(hdev);

	return 0;
}

static int bt_setup(struct hci_dev *hdev)
{
	int ret = 0;

	BTMTK_INFO("%s\n", __func__);

	/* 1. Power on */
	ret = btmtk_set_power_on(hdev);
	if (ret) {
		BTMTK_ERR("btmtk_set_power_on fail");
		return ret;
	}
#if ENABLESTP
	btmtk_send_set_stp_cmd(hdev);
	btmtk_send_set_stp1_cmd(hdev);
#endif
	/* 2. send WMT cmd to set BT on */
	ret = btmtk_send_wmt_power_on_cmd(hdev);
	if (ret) {
		BTMTK_ERR("btmtk_send_wmt_power_on_cmd fail");
		goto func_on_fail;
	}

	/* 3. Do calibration */
	ret = btmtk_calibration_flow(hdev);
	if (ret) {
		BTMTK_ERR("btmtk_calibration_flow fail");
		goto func_on_fail;
	}

	/* 4. Register screen on/off notify callback */
	btmtk_fb_notify_register(hdev);

	/* Set bt to sleep mode */
	btmtk_set_sleep(hdev);
	return 0;

func_on_fail:
	btmtk_set_power_off(hdev);
	return ret;
}

int bt_open(struct hci_dev *hdev)
{
	int ret = 0;
	BTMTK_INFO("%s\n", __func__);

	if (test_bit(HCI_RUNNING, &hdev->flags)) {
		BTMTK_WARN("BT already on!\n");
		return 0;
	}
#if BLUEDROID
	ret = bt_setup(hdev);
#endif
	if (!ret)
		set_bit(HCI_RUNNING, &hdev->flags);

	BTMTK_INFO("HCI running bit = %d", test_bit(HCI_RUNNING, &hdev->flags));
	return ret;
}

static int bt_send_frame(struct hci_dev *hdev, struct sk_buff *skb)
{
#if SUPPORT_BT_THREAD
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);
#endif

	BTMTK_INFO("%s\n", __func__);
	memcpy(skb_push(skb, 1), &bt_cb(skb)->pkt_type, 1);
#if ENABLESTP
	skb = mtk_add_stp(bdev, skb);
#endif

#if SUPPORT_BT_THREAD
	skb_queue_tail(&bdev->tx_queue, skb);
	wake_up_interruptible(&bdev->tx_waitq);
#else
	btmtk_cif_send_cmd(hdev, skb->data,	skb->len, 5, 0, 0);
#endif

	return 0;
}

int btmtk_allocate_hci_device(struct btmtk_dev *bdev, int hci_bus_type)
{
	struct hci_dev *hdev;
	int err = 0;

	/* Add hci device */
	hdev = hci_alloc_dev();
	if (!hdev)
		return -ENOMEM;
	hdev->bus = hci_bus_type;

	bdev->hdev = hdev;
	hci_set_drvdata(hdev, bdev);

	/* register hci callback */
	hdev->open	= bt_open;
	hdev->close	= bt_close;
	hdev->flush	= bt_flush;
	hdev->send	= bt_send_frame;
	hdev->setup	= bt_setup;

	init_waitqueue_head(&bdev->p_wait_event_q);
#if SUPPORT_BT_THREAD
	skb_queue_head_init(&bdev->tx_queue);
#endif
	SET_HCIDEV_DEV(hdev, BTMTK_GET_DEV(bdev));

	err = hci_register_dev(hdev);
	/* After hci_register_dev completed
	 * It will set dev_flags to HCI_SETUP
	 * That cause vendor_lib create socket failed
	 */
	if (err < 0) {
		BTMTK_INFO("%s can't register", __func__);
		hci_free_dev(hdev);
		return err;
	}

	/*set_bit(HCI_RUNNING, &hdev->flags);
	set_bit(HCI_QUIRK_RAW_DEVICE, &hdev->quirks);*/
#if BLUEDROID
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,4,0))
	clear_bit(HCI_SETUP, &hdev->dev_flags);
#else
	hci_dev_clear_flag(hdev, HCI_SETUP);
#endif
#endif
	set_bit(BTMTKUART_REQUIRED_DOWNLOAD, &bdev->tx_state);

	BTMTK_INFO("%s done", __func__);
	return 0;
}

int32_t btmtk_free_hci_device(struct btmtk_dev *bdev, int hci_bus_type)
{
	hci_unregister_dev(bdev->hdev);
	hci_free_dev(bdev->hdev);

	bdev->hdev = NULL;
	return 0;
}

/**
 * Kernel Module init/exit Functions
 */
static int __init main_driver_init(void)
{
	int ret = -1;

	BTMTK_INFO("%s", __func__);
	ret = main_init();
	if (ret < 0)
		return ret;

	ret = btmtk_cif_register();
	if (ret < 0) {
		BTMTK_ERR("*** USB registration failed(%d)! ***", ret);
		return ret;
	}
	BTMTK_INFO("%s: Done", __func__);
	return 0;
}

static void __exit main_driver_exit(void)
{
	BTMTK_INFO("%s", __func__);
	btmtk_cif_deregister();
	main_exit();
}
module_init(main_driver_init);
module_exit(main_driver_exit);

/**
 * Module Common Information
 */
MODULE_DESCRIPTION("Mediatek Bluetooth Driver");
MODULE_VERSION(VERSION SUBVER);
MODULE_LICENSE("GPL");
