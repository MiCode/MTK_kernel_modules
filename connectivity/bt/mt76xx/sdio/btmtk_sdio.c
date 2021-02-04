/*
 *  Copyright (c) 2016 MediaTek Inc.
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

#include "btmtk_config.h"
#include <linux/version.h>
#include <linux/firmware.h>
#include <linux/slab.h>

#include <linux/mmc/sdio_ids.h>
#include <linux/mmc/sdio_func.h>
#include <linux/module.h>

#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>

#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/sdio_func.h>
#include <linux/cdev.h>
#include <linux/spinlock.h>
#include <linux/kallsyms.h>
#include <linux/device.h>

/* Define for proce node */
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "btmtk_define.h"
#include "btmtk_drv.h"
#include "btmtk_sdio.h"

/* Used for WoBLE on EINT */
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/input.h>

#include <linux/of.h>
#include <linux/of_irq.h>

typedef int (*sdio_card_probe)(struct sdio_func *func,
					const struct sdio_device_id *id);

static struct bt_stereo_clk stereo_clk;
static u64 sys_clk_tmp;
static unsigned int stereo_irq;
struct _OSAL_UNSLEEPABLE_LOCK_ stereo_spin_lock;

static dev_t g_devIDfwlog;
static struct class *pBTClass;
static struct device *pBTDev;
struct device *pBTDevfwlog;
static wait_queue_head_t inq;
static wait_queue_head_t fw_log_inq;
static struct fasync_struct *fasync;
/*static int btmtk_woble_state = BTMTK_WOBLE_STATE_UNKNOWN;*/

static int need_reset_stack;
static int get_hci_reset;
static int need_reopen;
static int wlan_remove_done;

static u8 user_rmmod;
static int need_retry_load_woble;

struct completion g_done;
unsigned char probe_counter;
struct btmtk_private *g_priv;
#define STR_COREDUMP_END "coredump end\n\n"
const u8 READ_ADDRESS_EVENT[] = { 0x0e, 0x0a, 0x01, 0x09, 0x10, 0x00 };

static struct ring_buffer metabuffer;
static struct ring_buffer fwlog_metabuffer;
u8 probe_ready;
/* record firmware version */
static char fw_version_str[FW_VERSION_BUF_SIZE];
static struct proc_dir_entry *g_proc_dir;

/** read_write for proc */
static int btmtk_proc_show(struct seq_file *m, void *v);
static int btmtk_proc_open(struct inode *inode, struct  file *file);
static void btmtk_proc_create_new_entry(void);
static int btmtk_sdio_trigger_fw_assert(void);

static int btmtk_sdio_RegisterBTIrq(struct btmtk_sdio_card *data);
static int btmtk_sdio_woble_input_init(struct btmtk_sdio_card *data);
/* bluetooth KPI feautre, bperf */
u8 btmtk_bluetooth_kpi;

static char event_need_compare[EVENT_COMPARE_SIZE] = {0};
static char event_need_compare_len;
static char event_compare_status;
/*add special header in the beginning of even, stack won't recognize these event*/


/* timer for coredump end */
struct task_struct *wait_dump_complete_tsk;
struct task_struct *wait_wlan_remove_tsk;
int wlan_status = WLAN_STATUS_DEFAULT;

static int dump_data_counter;
static int dump_data_length;
static struct file *fw_dump_file;

const struct file_operations BT_proc_fops = {
	.open = btmtk_proc_open,
	.read = seq_read,
	.release = single_release,
};

static const struct btmtk_sdio_card_reg btmtk_reg_6630 = {
	.cfg = 0x03,
	.host_int_mask = 0x04,
	.host_intstatus = 0x05,
	.card_status = 0x20,
	.sq_read_base_addr_a0 = 0x10,
	.sq_read_base_addr_a1 = 0x11,
	.card_fw_status0 = 0x40,
	.card_fw_status1 = 0x41,
	.card_rx_len = 0x42,
	.card_rx_unit = 0x43,
	.io_port_0 = 0x00,
	.io_port_1 = 0x01,
	.io_port_2 = 0x02,
	.int_read_to_clear = false,
	.func_num = 2,
	.chip_id = 0x6630,
};

static const struct btmtk_sdio_card_reg btmtk_reg_6632 = {
	.cfg = 0x03,
	.host_int_mask = 0x04,
	.host_intstatus = 0x05,
	.card_status = 0x20,
	.sq_read_base_addr_a0 = 0x10,
	.sq_read_base_addr_a1 = 0x11,
	.card_fw_status0 = 0x40,
	.card_fw_status1 = 0x41,
	.card_rx_len = 0x42,
	.card_rx_unit = 0x43,
	.io_port_0 = 0x00,
	.io_port_1 = 0x01,
	.io_port_2 = 0x02,
	.int_read_to_clear = false,
	.func_num = 2,
	.chip_id = 0x6632,
};

static const struct btmtk_sdio_card_reg btmtk_reg_7668 = {
	.cfg = 0x03,
	.host_int_mask = 0x04,
	.host_intstatus = 0x05,
	.card_status = 0x20,
	.sq_read_base_addr_a0 = 0x10,
	.sq_read_base_addr_a1 = 0x11,
	.card_fw_status0 = 0x40,
	.card_fw_status1 = 0x41,
	.card_rx_len = 0x42,
	.card_rx_unit = 0x43,
	.io_port_0 = 0x00,
	.io_port_1 = 0x01,
	.io_port_2 = 0x02,
	.int_read_to_clear = false,
	.func_num = 2,
	.chip_id = 0x7668,
};

static const struct btmtk_sdio_card_reg btmtk_reg_7663 = {
	.cfg = 0x03,
	.host_int_mask = 0x04,
	.host_intstatus = 0x05,
	.card_status = 0x20,
	.sq_read_base_addr_a0 = 0x10,
	.sq_read_base_addr_a1 = 0x11,
	.card_fw_status0 = 0x40,
	.card_fw_status1 = 0x41,
	.card_rx_len = 0x42,
	.card_rx_unit = 0x43,
	.io_port_0 = 0x00,
	.io_port_1 = 0x01,
	.io_port_2 = 0x02,
	.int_read_to_clear = false,
	.func_num = 2,
	.chip_id = 0x7663,
};

static const struct btmtk_sdio_card_reg btmtk_reg_7666 = {
	.cfg = 0x03,
	.host_int_mask = 0x04,
	.host_intstatus = 0x05,
	.card_status = 0x20,
	.sq_read_base_addr_a0 = 0x10,
	.sq_read_base_addr_a1 = 0x11,
	.card_fw_status0 = 0x40,
	.card_fw_status1 = 0x41,
	.card_rx_len = 0x42,
	.card_rx_unit = 0x43,
	.io_port_0 = 0x00,
	.io_port_1 = 0x01,
	.io_port_2 = 0x02,
	.int_read_to_clear = false,
	.func_num = 2,
	.chip_id = 0x7666,
};

static const struct btmtk_sdio_device btmtk_sdio_6630 = {
	.helper = "mtmk/sd8688_helper.bin",
	.reg = &btmtk_reg_6630,
	.support_pscan_win_report = false,
	.sd_blksz_fw_dl = 64,
	.supports_fw_dump = false,
};

static const struct btmtk_sdio_device btmtk_sdio_6632 = {
	.helper = "mtmk/sd8688_helper.bin",
	.reg = &btmtk_reg_6632,
	.support_pscan_win_report = false,
	.sd_blksz_fw_dl = 64,
	.supports_fw_dump = false,
};

static const struct btmtk_sdio_device btmtk_sdio_7668 = {
	.helper = "mtmk/sd8688_helper.bin",
	.reg = &btmtk_reg_7668,
	.support_pscan_win_report = false,
	.sd_blksz_fw_dl = 64,
	.supports_fw_dump = false,
};

static const struct btmtk_sdio_device btmtk_sdio_7663 = {
	.helper = "mtmk/sd8688_helper.bin",
	.reg = &btmtk_reg_7663,
	.support_pscan_win_report = false,
	.sd_blksz_fw_dl = 64,
	.supports_fw_dump = false,
};

static const struct btmtk_sdio_device btmtk_sdio_7666 = {
	.helper = "mtmk/sd8688_helper.bin",
	.reg = &btmtk_reg_7666,
	.support_pscan_win_report = false,
	.sd_blksz_fw_dl = 64,
	.supports_fw_dump = false,
};

static u8 hci_cmd_snoop_buf[HCI_SNOOP_ENTRY_NUM][HCI_SNOOP_BUF_SIZE];
static u8 hci_cmd_snoop_len[HCI_SNOOP_ENTRY_NUM];
static unsigned int hci_cmd_snoop_timestamp[HCI_SNOOP_ENTRY_NUM];

static u8 hci_event_snoop_buf[HCI_SNOOP_ENTRY_NUM][HCI_SNOOP_BUF_SIZE];
static u8 hci_event_snoop_len[HCI_SNOOP_ENTRY_NUM];
static unsigned int hci_event_snoop_timestamp[HCI_SNOOP_ENTRY_NUM];

static u8 hci_acl_snoop_buf[HCI_SNOOP_ENTRY_NUM][HCI_SNOOP_BUF_SIZE];
static u8 hci_acl_snoop_len[HCI_SNOOP_ENTRY_NUM];
static unsigned int hci_acl_snoop_timestamp[HCI_SNOOP_ENTRY_NUM];

static int hci_cmd_snoop_index;
static int hci_event_snoop_index;
static int hci_acl_snoop_index;

unsigned char *txbuf;
static unsigned char *rxbuf;
static unsigned char *userbuf;
static unsigned char *userbuf_fwlog;
static u32 rx_length;
static struct btmtk_sdio_card *g_card;

static u32 reg_CHISR; /* Add for debug, need remove later */

#define SDIO_VENDOR_ID_MEDIATEK 0x037A

static const struct sdio_device_id btmtk_sdio_ids[] = {
	/* Mediatek SD8688 Bluetooth device */
	{ SDIO_DEVICE(SDIO_VENDOR_ID_MEDIATEK, 0x6630),
			.driver_data = (unsigned long) &btmtk_sdio_6630 },

	{ SDIO_DEVICE(SDIO_VENDOR_ID_MEDIATEK, 0x6632),
			.driver_data = (unsigned long) &btmtk_sdio_6632 },

	{ SDIO_DEVICE(SDIO_VENDOR_ID_MEDIATEK, 0x7668),
			.driver_data = (unsigned long) &btmtk_sdio_7668 },

	{ SDIO_DEVICE(SDIO_VENDOR_ID_MEDIATEK, 0x7663),
			.driver_data = (unsigned long) &btmtk_sdio_7663 },

	{ SDIO_DEVICE(SDIO_VENDOR_ID_MEDIATEK, 0x7666),
			.driver_data = (unsigned long) &btmtk_sdio_7666 },

	{ }	/* Terminating entry */
};
MODULE_DEVICE_TABLE(sdio, btmtk_sdio_ids);

static int btmtk_clean_queue(void);
static void btmtk_sdio_do_reset_or_wait_wlan_remove_done(void);
static int btmtk_sdio_download_partial_rom_patch(u8 *fwbuf, int firmwarelen);
static int btmtk_sdio_probe(struct sdio_func *func,
					const struct sdio_device_id *id);
static void btmtk_sdio_L0_hook_new_probe(sdio_card_probe pFn_Probe);
static void btmtk_eeprom_bin_file(struct btmtk_sdio_card *card);
static int btmtk_sdio_set_sleep(void);
static int btmtk_sdio_set_audio(void);
static int btmtk_sdio_send_hci_cmd(u8 cmd_type, u8 *cmd, int cmd_len,
		const u8 *event, const int event_len,
		int total_timeout, bool wait_until);


void btmtk_sdio_stop_wait_wlan_remove_tsk(void)
{
	if (wait_wlan_remove_tsk == NULL)
		pr_info("%s wait_wlan_remove_tsk is NULL\n", __func__);
	else if (IS_ERR(wait_wlan_remove_tsk))
		pr_info("%s wait_wlan_remove_tsk is error\n", __func__);
	else {
		pr_info("%s call kthread_stop wait_wlan_remove_tsk\n",
			__func__);
		kthread_stop(wait_wlan_remove_tsk);
		wait_wlan_remove_tsk = NULL;
	}
}

int btmtk_sdio_notify_wlan_remove_start(void)
{
	/* notify_wlan_remove_start */
	int ret = 0;
	typedef void (*pnotify_wlan_remove_start) (int reserved);
	char *notify_wlan_remove_start_func_name;
	pnotify_wlan_remove_start pnotify_wlan_remove_start_func;

	pr_info("%s: wlan_status %d\n", __func__, wlan_status);
	if (wlan_status == WLAN_STATUS_CALL_REMOVE_START) {
		/* do notify before, just return */
		return ret;
	}

	if (is_mt7663(g_card))
		notify_wlan_remove_start_func_name =
			"BT_rst_L0_notify_WF_step1";
	else
		notify_wlan_remove_start_func_name =
			"notify_wlan_remove_start";

	pnotify_wlan_remove_start_func =
		(pnotify_wlan_remove_start)kallsyms_lookup_name
			(notify_wlan_remove_start_func_name);

	btmtk_sdio_stop_wait_wlan_remove_tsk();
	pr_info(L0_RESET_TAG "%s\n", __func__);
	/* void notify_wlan_remove_start(void) */
	if (pnotify_wlan_remove_start_func) {
		pr_info("%s: do notify %s\n", __func__, notify_wlan_remove_start_func_name);
		wlan_remove_done = 0;
		if (is_mt7663(g_card))
			pnotify_wlan_remove_start_func(0);
		else
			pnotify_wlan_remove_start_func(1);
		wlan_status = WLAN_STATUS_CALL_REMOVE_START;
	} else {
		ret = -1;
		pr_info("%s: do not get %s\n", __func__, notify_wlan_remove_start_func_name);
		wlan_status = WLAN_STATUS_IS_NOT_LOAD;
		wlan_remove_done = 1;
	}
	return ret;
}

/*============================================================================*/
/* Interface Functions : timer for coredump inform wifi */
/*============================================================================*/
static void btmtk_sdio_wakeup_mainthread_do_reset(void)
{
	if (g_priv) {
		g_priv->btmtk_dev.reset_dongle = 1;
		pr_info("%s: set reset_dongle %d", __func__, g_priv->btmtk_dev.reset_dongle);
		wake_up_interruptible(&g_priv->main_thread.wait_q);
	} else
		pr_info("%s: g_priv is NULL", __func__);
}

static int btmtk_sdio_wait_wlan_remove_thread(void *ptr)
{
	int  i = 0;

	pr_info("%s: begin", __func__);
	if (g_priv == NULL) {
		pr_info("%s: g_priv is NULL, return", __func__);
		return 0;
	}

	g_priv->btmtk_dev.reset_progress = 1;
	for (i = 0; i < 30; i++) {
		if ((wait_wlan_remove_tsk && kthread_should_stop()) || wlan_remove_done) {
			pr_info("%s: break wlan_remove_done %d\n", __func__, wlan_remove_done);
			break;
		}
		msleep(500);
	}

	btmtk_sdio_wakeup_mainthread_do_reset();

	while (!kthread_should_stop()) {
		pr_info("%s: no one call stop", __func__);
		msleep(500);
	}

	pr_info("%s: end", __func__);
	return 0;
}

static void btmtk_sdio_start_reset_dongle_progress(void)
{
	if (!g_card->bt_cfg.support_dongle_reset) {
		pr_info("%s: debug mode do not do reset", __func__);
	} else {
		pr_info("%s: user mode do reset", __func__);
		btmtk_sdio_notify_wlan_remove_start();
		btmtk_sdio_do_reset_or_wait_wlan_remove_done();
	}
}

/*============================================================================*/
/* Interface Functions : timer for uncomplete coredump */
/*============================================================================*/
static void btmtk_sdio_do_reset_or_wait_wlan_remove_done(void)
{
	pr_info("%s: wlan_remove_done %d\n", __func__, wlan_remove_done);
	if (wlan_remove_done || (wlan_status == WLAN_STATUS_IS_NOT_LOAD))
		/* wifi inform bt already, reset chip */
		btmtk_sdio_wakeup_mainthread_do_reset();
	else {
		/* create thread wait wifi inform bt */
		pr_info("%s: create btmtk_sdio_wait_wlan_remove_thread\n", __func__);
		wait_wlan_remove_tsk = kthread_run(
			btmtk_sdio_wait_wlan_remove_thread, NULL,
			"btmtk_sdio_wait_wlan_remove_thread");
		if (wait_wlan_remove_tsk == NULL)
			pr_info("%s: btmtk_sdio_wait_wlan_remove_thread create fail\n", __func__);
	}
}

static int btmtk_sdio_wait_dump_complete_thread(void *ptr)
{
	int  i = 0;

	pr_info("%s: begin", __func__);
	for (i = 0; i < 60; i++) {
		if (wait_dump_complete_tsk && kthread_should_stop()) {
			pr_info("%s: thread is stopped, break\n", __func__);
			break;
		}
		msleep(500);
	}

	if (!g_card->bt_cfg.support_dongle_reset) {
		pr_info("%s: debug mode don't do reset\n", __func__);
	} else {
		pr_info("%s: user mode call do reset\n", __func__);
		btmtk_sdio_do_reset_or_wait_wlan_remove_done();
	}

	if (i >= 60)
		pr_info("%s: wait dump complete timeout\n", __func__);
	wait_dump_complete_tsk = NULL;

	pr_info("%s: end", __func__);
	return 0;
}

static void btmtk_sdio_free_bt_cfg(void)
{
	pr_info("%s begin", __func__);
	if (g_card == NULL) {
		pr_info("%s: g_card == NULL", __func__);
		return;
	}

	kfree(g_card->bt_cfg.sys_log_file_name);
	g_card->bt_cfg.sys_log_file_name = NULL;

	kfree(g_card->bt_cfg.fw_dump_file_name);
	g_card->bt_cfg.fw_dump_file_name = NULL;

	kfree(g_card->bt_cfg_file_name);
	g_card->bt_cfg_file_name = NULL;

	memset(&g_card->bt_cfg, 0, sizeof(g_card->bt_cfg));

	pr_info("%s end", __func__);
}

static void btmtk_sdio_woble_free_setting_struct(struct woble_setting_struct *woble_struct, int count)
{
	int i = 0;

	for (i = 0; i < count; i++) {
		if (woble_struct[i].content) {
			pr_info("%s:kfree %d", __func__, i);
			kfree(woble_struct[i].content);
			woble_struct[i].content = NULL;
			woble_struct[i].length = 0;
		} else
			woble_struct[i].length = 0;
	}
}

static void btmtk_sdio_woble_free_setting(void)
{
	pr_info("%s begin", __func__);
	if (g_card == NULL) {
		pr_info("%s: g_data == NULL", __func__);
		return;
	}

	btmtk_sdio_woble_free_setting_struct(g_card->woble_setting_apcf, WOBLE_SETTING_COUNT);
	btmtk_sdio_woble_free_setting_struct(g_card->woble_setting_apcf_fill_mac, WOBLE_SETTING_COUNT);
	btmtk_sdio_woble_free_setting_struct(g_card->woble_setting_apcf_fill_mac_location, WOBLE_SETTING_COUNT);
	btmtk_sdio_woble_free_setting_struct(g_card->woble_setting_radio_off, WOBLE_SETTING_COUNT);
	btmtk_sdio_woble_free_setting_struct(g_card->woble_setting_radio_off_status_event, WOBLE_SETTING_COUNT);
	btmtk_sdio_woble_free_setting_struct(g_card->woble_setting_radio_off_comp_event, WOBLE_SETTING_COUNT);
	btmtk_sdio_woble_free_setting_struct(g_card->woble_setting_radio_on, WOBLE_SETTING_COUNT);
	btmtk_sdio_woble_free_setting_struct(g_card->woble_setting_radio_on_status_event, WOBLE_SETTING_COUNT);
	btmtk_sdio_woble_free_setting_struct(g_card->woble_setting_radio_on_comp_event, WOBLE_SETTING_COUNT);

	kfree(g_card->woble_setting_file_name);
	g_card->woble_setting_file_name = NULL;

	if (g_card->bt_cfg.support_woble_by_eint) {
		if (g_card->wobt_irq != 0 && atomic_read(&(g_card->irq_enable_count)) == 1) {
			pr_info("%s:disable BT IRQ:%d\n", __func__, g_card->wobt_irq);
			atomic_dec(&(g_card->irq_enable_count));
			disable_irq_nosync(g_card->wobt_irq);
		} else
			pr_info("%s:irq_enable count:%d\n", __func__, atomic_read(&(g_card->irq_enable_count)));
		free_irq(g_card->wobt_irq, g_card);
	}

	pr_info("%s end", __func__);
}

static void btmtk_sdio_initialize_cfg_items(void)
{
	pr_info("%s begin\n", __func__);
	if (g_card == NULL) {
		pr_info("%s: g_card == NULL", __func__);
		return;
	}

	g_card->bt_cfg.dongle_reset_gpio_pin = 0;
	g_card->bt_cfg.save_fw_dump_in_kernel = 0;
	g_card->bt_cfg.support_dongle_reset = 1;
	g_card->bt_cfg.support_full_fw_dump = 1;
	g_card->bt_cfg.support_legacy_woble = 0;
	g_card->bt_cfg.support_unify_woble = 0;
	g_card->bt_cfg.support_woble_by_eint = 0;
	g_card->bt_cfg.support_woble_for_bt_disable = 0;
	g_card->bt_cfg.support_woble_wakelock = 0;
	g_card->bt_cfg.sys_log_file_name = NULL;
	g_card->bt_cfg.fw_dump_file_name = NULL;

	pr_info("%s end\n", __func__);
}

static bool btmtk_sdio_parse_bt_cfg_file(char *item_name, char *text,
					char *searchcontent)
{
	bool ret = true;
	int temp_len = 0;
	char search[32];
	char *ptr = NULL, *p = NULL;
	char *temp = text;

	if (text == NULL) {
		pr_info("%s: text param is invalid!\n", __func__);
		ret = false;
		goto out;
	}

	memset(search, 0, sizeof(search));
	snprintf(search, sizeof(search), "%s", item_name); /* EX: SUPPORT_UNIFY_WOBLE */
	p = ptr = strstr(searchcontent, search);

	if (!ptr) {
		pr_info("%s: Can't find %s\n", __func__, item_name);
		ret = false;
		goto out;
	}

	if (p > searchcontent) {
		p--;
		while ((*p == ' ') && (p != searchcontent))
			p--;
		if (*p == '#') {
			pr_notice("%s: It's invalid bt cfg item\n", __func__);
			ret = false;
			goto out;
		}
	}

	p = ptr + strlen(item_name) + 1;
	ptr = p;

	for (;;) {
		switch (*p) {
		case '\n':
			goto textdone;
		default:
			*temp++ = *p++;
			break;
		}
	}

textdone:
	temp_len = p - ptr;
	*temp = '\0';

out:
	return ret;
}

static void btmtk_sdio_bt_cfg_item_value_to_bool(char *item_value, bool *value)
{
	unsigned long text_value = 0;

	if (item_value == NULL) {
		pr_info("%s: item_value is NULL!\n", __func__);
		return;
	}

	if (kstrtoul(item_value, 10, &text_value) == 0) {
		if (text_value == 1)
			*value = true;
		else
			*value = false;
	} else {
		pr_info("%s: kstrtoul failed!\n", __func__);
	}
}

static bool btmtk_sdio_load_bt_cfg_item(struct bt_cfg_struct *bt_cfg_content,
					char *searchcontent)
{
	bool ret = true;
	char text[128]; /* save for search text */
	unsigned long text_value = 0;

	memset(text, 0, sizeof(text));
	ret = btmtk_sdio_parse_bt_cfg_file(BT_UNIFY_WOBLE, text, searchcontent);
	if (ret) {
		btmtk_sdio_bt_cfg_item_value_to_bool(text, &bt_cfg_content->support_unify_woble);
		pr_info("%s: bt_cfg_content->support_unify_woble = %d\n", __func__,
				bt_cfg_content->support_unify_woble);
	} else {
		pr_info("%s: search item %s is invalid!\n", __func__, BT_UNIFY_WOBLE);
	}

	ret = btmtk_sdio_parse_bt_cfg_file(BT_LEGACY_WOBLE, text, searchcontent);
	if (ret) {
		btmtk_sdio_bt_cfg_item_value_to_bool(text, &bt_cfg_content->support_legacy_woble);
		pr_info("%s: bt_cfg_content->support_legacy_woble = %d\n", __func__,
			bt_cfg_content->support_legacy_woble);
	} else {
		pr_info("%s: search item %s is invalid!\n", __func__, BT_LEGACY_WOBLE);
	}

	ret = btmtk_sdio_parse_bt_cfg_file(BT_WOBLE_BY_EINT, text, searchcontent);
	if (ret) {
		btmtk_sdio_bt_cfg_item_value_to_bool(text, &bt_cfg_content->support_woble_by_eint);
		pr_info("%s: bt_cfg_content->support_woble_by_eint = %d\n", __func__,
					bt_cfg_content->support_woble_by_eint);
	} else {
		pr_info("%s: search item %s is invalid!\n", __func__, BT_WOBLE_BY_EINT);
	}

	ret = btmtk_sdio_parse_bt_cfg_file(BT_DONGLE_RESET_PIN, text, searchcontent);
	if (ret) {
		if (kstrtoul(text, 10, &text_value) == 0)
			bt_cfg_content->dongle_reset_gpio_pin = text_value;
		else
			pr_info("%s: kstrtoul failed %s!\n", __func__, BT_DONGLE_RESET_PIN);
	} else {
		pr_info("%s: search item %s is invalid!\n", __func__, BT_DONGLE_RESET_PIN);
	}

	pr_info("%s: bt_cfg_content->dongle_reset_gpio_pin = %d\n", __func__,
			bt_cfg_content->dongle_reset_gpio_pin);

	ret = btmtk_sdio_parse_bt_cfg_file(BT_SAVE_FW_DUMP_IN_KERNEL, text, searchcontent);
	if (ret) {
		btmtk_sdio_bt_cfg_item_value_to_bool(text, &bt_cfg_content->save_fw_dump_in_kernel);
		pr_info("%s: bt_cfg_content->save_fw_dump_in_kernel = %d\n", __func__,
				bt_cfg_content->save_fw_dump_in_kernel);
	} else {
		pr_info("%s: search item %s is invalid!\n", __func__, BT_SAVE_FW_DUMP_IN_KERNEL);
	}

	ret = btmtk_sdio_parse_bt_cfg_file(BT_RESET_DONGLE, text, searchcontent);
	if (ret) {
		btmtk_sdio_bt_cfg_item_value_to_bool(text, &bt_cfg_content->support_dongle_reset);
		pr_info("%s: bt_cfg_content->support_dongle_reset = %d\n", __func__,
				bt_cfg_content->support_dongle_reset);
	} else {
		pr_info("%s: search item %s is invalid!\n", __func__, BT_RESET_DONGLE);
	}

	ret = btmtk_sdio_parse_bt_cfg_file(BT_FULL_FW_DUMP, text, searchcontent);
	if (ret) {
		btmtk_sdio_bt_cfg_item_value_to_bool(text, &bt_cfg_content->support_full_fw_dump);
		pr_info("%s: bt_cfg_content->support_full_fw_dump = %d\n", __func__,
				bt_cfg_content->support_full_fw_dump);
	} else {
		pr_info("%s: search item %s is invalid!\n", __func__, BT_FULL_FW_DUMP);
	}

	ret = btmtk_sdio_parse_bt_cfg_file(BT_WOBLE_WAKELOCK, text, searchcontent);
	if (ret) {
		btmtk_sdio_bt_cfg_item_value_to_bool(text, &bt_cfg_content->support_woble_wakelock);
		pr_info("%s: bt_cfg_content->support_woble_wakelock = %d\n", __func__,
				bt_cfg_content->support_woble_wakelock);
	} else {
		pr_info("%s: search item %s is invalid!\n", __func__, BT_WOBLE_WAKELOCK);
	}

	ret = btmtk_sdio_parse_bt_cfg_file(BT_WOBLE_FOR_BT_DISABLE, text, searchcontent);
	if (ret) {
		btmtk_sdio_bt_cfg_item_value_to_bool(text, &bt_cfg_content->support_woble_for_bt_disable);
		pr_info("%s: bt_cfg_content->support_woble_for_bt_disable = %d\n", __func__,
				bt_cfg_content->support_woble_for_bt_disable);
	} else {
		pr_info("%s: search item %s is invalid!\n", __func__, BT_WOBLE_FOR_BT_DISABLE);
	}

	ret = btmtk_sdio_parse_bt_cfg_file(BT_SYS_LOG_FILE, text, searchcontent);
	if (ret) {
		if (bt_cfg_content->sys_log_file_name != NULL) {
			kfree(bt_cfg_content->sys_log_file_name);
			bt_cfg_content->sys_log_file_name = NULL;
		}
		bt_cfg_content->sys_log_file_name = kzalloc(strlen(text) + 1, GFP_KERNEL);
		if (bt_cfg_content->sys_log_file_name == NULL) {
			pr_info("%s: Allocate memory fail\n", __func__);
			ret = false;
			return ret;
		}
		memcpy(bt_cfg_content->sys_log_file_name, text, strlen(text));
		bt_cfg_content->sys_log_file_name[strlen(text)] = '\0';
		pr_info("%s: bt_cfg_content->sys_log_file_name = %s\n", __func__,
				bt_cfg_content->sys_log_file_name);
	} else {
		pr_info("%s: search item %s is invalid!\n", __func__, BT_SYS_LOG_FILE);
	}

	ret = btmtk_sdio_parse_bt_cfg_file(BT_FW_DUMP_FILE, text, searchcontent);
	if (ret) {
		if (bt_cfg_content->fw_dump_file_name != NULL) {
			kfree(bt_cfg_content->fw_dump_file_name);
			bt_cfg_content->fw_dump_file_name = NULL;
		}
		bt_cfg_content->fw_dump_file_name = kzalloc(strlen(text) + 1, GFP_KERNEL);
		if (bt_cfg_content->fw_dump_file_name == NULL) {
			pr_info("%s: Allocate memory fail\n", __func__);
			ret = false;
			return ret;
		}
		memcpy(bt_cfg_content->fw_dump_file_name, text, strlen(text));
		bt_cfg_content->fw_dump_file_name[strlen(text)] = '\0';
		pr_info("%s: bt_cfg_content->fw_dump_file_name = %s\n", __func__,
				bt_cfg_content->fw_dump_file_name);
	} else {
		pr_info("%s: search item %s is invalid!\n", __func__, BT_FW_DUMP_FILE);
	}

	/* release setting file memory */
	if (g_card) {
		kfree(g_card->setting_file);
		g_card->setting_file = NULL;
	}
	return ret;
}

static int btmtk_sdio_load_woble_block_setting(char *block_name, struct woble_setting_struct *save_content,
			int save_content_count, u8 *searchconetnt)
{
	int ret = 0;
	int i = 0;
	long parsing_result = 0;
	u8 *search_result = NULL;
	u8 *search_end = NULL;
	u8 search[32];
	u8 temp[260]; /* save for total hex number */
	u8 *next_number = NULL;
	u8 *next_block = NULL;
	u8 number[8];
	int temp_len;

	memset(search, 0, sizeof(search));
	memset(temp, 0, sizeof(temp));
	memset(number, 0, sizeof(number));

	/* search block name */
	for (i = 0; i < WOBLE_SETTING_COUNT; i++) {
		temp_len = 0;
		snprintf(search, sizeof(search), "%s%02d:", block_name, i); /* ex APCF01 */
		search_result = strstr(searchconetnt, search);
		if (search_result) {
			memset(temp, 0, sizeof(temp));
			temp_len = 0;
			search_result += strlen(search); /* move to first number */

			do {
				next_number = NULL;
				search_end = strstr(search_result, ",");
				if ((search_end - search_result) <= 0) {
					pr_info("%s can not find search end, break", __func__);
					break;
				}

				if ((search_end - search_result) > sizeof(number))
					break;

				memset(number, 0, sizeof(number));
				memcpy(number, search_result, search_end - search_result);

				if (number[0] == 0x20) /* space */
					ret = kstrtol(number + 1, 0, &parsing_result);
				else
					ret = kstrtol(number, 0, &parsing_result);

				if (ret == 0) {
					if (temp_len >= sizeof(temp)) {
						pr_info("%s: %s data over %zu", __func__, search, sizeof(temp));
						break;
					}
					temp[temp_len] = parsing_result;
					temp_len++;
					/* find next number */
					next_number = strstr(search_end, "0x");

					/* find next block */
					next_block = strstr(search_end, ":");
				} else {
					pr_debug("%s:kstrtol ret = %d, search %s", __func__, ret, search);
					break;
				}

				if (next_number == NULL) {
					pr_debug("%s: not find next apcf number temp_len %d, break, search %s",
						__func__, temp_len, search);
					break;
				}

				if ((next_number > next_block) && (next_block != 0)) {
					pr_debug("%s: find next apcf number is over to next block ", __func__);
					pr_debug("%s: temp_len %d, break, search %s",
						__func__, temp_len, search);
					break;
				}

				search_result = search_end + 1;
			} while (1);
		} else
			pr_debug("%s: %s is not found", __func__, search);

		if (temp_len) {
			pr_info("%s: %s found\n", __func__, search);
			pr_debug("%s: kzalloc i=%d temp_len=%d", __func__, i, temp_len);
			save_content[i].content = kzalloc(temp_len, GFP_KERNEL);
			memcpy(save_content[i].content, temp, temp_len);
			save_content[i].length = temp_len;
			pr_debug("%s:x  save_content[%d].length %d temp_len=%d",
				__func__, i, save_content[i].length, temp_len);
		}

	}
	return ret;
}

static void btmtk_sdio_load_woble_setting_callback(const struct firmware *fw_data, void *context)
{
	struct btmtk_sdio_card *card = (struct btmtk_sdio_card *)context;
	int err = 0;
	unsigned char *image = NULL;

	if (!fw_data) {
		pr_info("%s: fw_data is NULL callback request_firmware fail or can't find file!!\n",
			__func__);

		/* Request original woble_setting.bin */
		memcpy(g_card->woble_setting_file_name,
				WOBLE_SETTING_FILE_NAME,
				sizeof(WOBLE_SETTING_FILE_NAME));
		pr_info("%s: begin load orignial woble_setting_file_name = %s\n",
				__func__, g_card->woble_setting_file_name);
		if (need_retry_load_woble < BTMTK_LOAD_WOBLE_RETRY_COUNT) {
			need_retry_load_woble++;
			err = request_firmware_nowait(THIS_MODULE, true, g_card->woble_setting_file_name,
				&g_card->func->dev, GFP_KERNEL, g_card, btmtk_sdio_load_woble_setting_callback);
			if (err < 0) {
				pr_info("%s: request %s file fail(%d)\n",
					__func__, g_card->woble_setting_file_name, err);
			}
		} else {
			pr_info("%s: request %s file fail(%d), need_load_origin_woble = %d\n",
				__func__, g_card->woble_setting_file_name, err, need_retry_load_woble);
		}
		return;
	}

	pr_info("%s: woble_setting callback request_firmware size %zu success\n", __func__, fw_data->size);
	image = kzalloc(fw_data->size + 1, GFP_KERNEL); /* w:move to btmtk_usb_free_memory */
	if (image == NULL) {
		pr_info("%s: kzalloc size %zu failed!!", __func__, fw_data->size);
		goto LOAD_END;
	}

	memcpy(image, fw_data->data, fw_data->size);
	image[fw_data->size] = '\0';

	err = btmtk_sdio_load_woble_block_setting("APCF",
			card->woble_setting_apcf, WOBLE_SETTING_COUNT, image);
	if (err)
		goto LOAD_END;

	err = btmtk_sdio_load_woble_block_setting("APCF_ADD_MAC",
			card->woble_setting_apcf_fill_mac, WOBLE_SETTING_COUNT, image);
	if (err)
		goto LOAD_END;

	err = btmtk_sdio_load_woble_block_setting("APCF_ADD_MAC_LOCATION",
			card->woble_setting_apcf_fill_mac_location, WOBLE_SETTING_COUNT, image);
	if (err)
		goto LOAD_END;

	err = btmtk_sdio_load_woble_block_setting("RADIOOFF",
			card->woble_setting_radio_off, WOBLE_SETTING_COUNT, image);
	if (err)
		goto LOAD_END;

	err = btmtk_sdio_load_woble_block_setting("RADIOOFF_STATUS_EVENT",
			card->woble_setting_radio_off_status_event, WOBLE_SETTING_COUNT, image);
	if (err)
		goto LOAD_END;

	err = btmtk_sdio_load_woble_block_setting("RADIOOFF_COMPLETE_EVENT",
			card->woble_setting_radio_off_comp_event, WOBLE_SETTING_COUNT, image);
	if (err)
		goto LOAD_END;

	err = btmtk_sdio_load_woble_block_setting("RADIOON",
			card->woble_setting_radio_on, WOBLE_SETTING_COUNT, image);
	if (err)
		goto LOAD_END;

	err = btmtk_sdio_load_woble_block_setting("RADIOON_STATUS_EVENT",
			card->woble_setting_radio_on_status_event, WOBLE_SETTING_COUNT, image);
	if (err)
		goto LOAD_END;

	err = btmtk_sdio_load_woble_block_setting("RADIOON_COMPLETE_EVENT",
			card->woble_setting_radio_on_comp_event, WOBLE_SETTING_COUNT, image);
	if (err)
		goto LOAD_END;

	err = btmtk_sdio_load_woble_block_setting("APCF_RESMUE",
		card->woble_setting_apcf_resume, WOBLE_SETTING_COUNT, image);
	if (err)
		goto LOAD_END;

	err = btmtk_sdio_load_woble_block_setting("APCF_COMPLETE_EVENT",
			card->woble_setting_apcf_resume_event, WOBLE_SETTING_COUNT, image);

LOAD_END:
	kfree(image);
	image = NULL;
	release_firmware(fw_data);
	if (err)
		pr_info("%s: result fail\n", __func__);
	else
		pr_info("%s: result success\n", __func__);
}

static int btmtk_sdio_load_code_from_setting_files(char *setting_file_name,
				struct btmtk_sdio_card *data, u32 *code_len)
{
	int err = 0;
	const struct firmware *fw_entry = NULL;
	*code_len = 0;

	if (data == NULL || setting_file_name == NULL) {
		pr_info("%s: invalid parameter!!\n", __func__);
		err = -1;
		return err;
	}

	err = request_firmware(&fw_entry, setting_file_name, &data->func->dev);
	if (err != 0) {
		pr_info("%s: request %s file fail(%d)\n", __func__, setting_file_name, err);
		return err;
	}

	if (data->setting_file != NULL) {
		kfree(data->setting_file);
		data->setting_file = NULL;
	}

	if (fw_entry) {
		/* alloc setting file memory */
		data->setting_file = kzalloc(fw_entry->size + 1, GFP_KERNEL);
		pr_info("%s: setting file request_firmware size %zu success\n", __func__, fw_entry->size);
	} else {
		pr_info("%s: fw_entry is NULL request_firmware may fail!! error code = %d\n", __func__, err);
		return err;
	}

	if (data->setting_file == NULL) {
		pr_info("%s: kzalloc size %zu failed!\n!", __func__, fw_entry->size);
		release_firmware(fw_entry);
		err = -1;
		return err;
	}

	memcpy(data->setting_file, fw_entry->data, fw_entry->size);
	data->setting_file[fw_entry->size] = '\0';

	*code_len = fw_entry->size;
	release_firmware(fw_entry);

	pr_info("%s: cfg_file len (%d) assign done\n", __func__, *code_len);
	return err;
}

static int btmtk_sdio_load_setting_files(char *bin_name, struct device *dev,
					struct btmtk_sdio_card *data)
{
	int err = 0;
	char *ptr_name = NULL;
	u32 code_len = 0;

	pr_info("%s: begin setting_file_name = %s\n", __func__, bin_name);
	ptr_name = strstr(bin_name, "woble_setting");
	if (ptr_name) {
		err = request_firmware_nowait(THIS_MODULE, true, bin_name,
			&data->func->dev, GFP_KERNEL, data, btmtk_sdio_load_woble_setting_callback);

		if (err < 0)
			pr_info("%s: request %s file fail(%d)\n", __func__, bin_name, err);
		else
			pr_notice("%s: request %s file success(%d)\n", __func__, bin_name, err);
	} else if (strcmp(bin_name, BT_CFG_NAME) == 0) {
		err = btmtk_sdio_load_code_from_setting_files(bin_name, data, &code_len);
		if (err != 0) {
			pr_info("%s, btmtk_sdio_load_code_from_cfg_files failed!!\n", __func__);
			return err;
		}

		if (!btmtk_sdio_load_bt_cfg_item(&data->bt_cfg, data->setting_file)) {
			pr_info("%s, btmtk_sdio_load_bt_cfg_item error!!\n", __func__);
			err = -1;
			return err;
		}
	} else
		pr_info("%s: bin_name is not defined\n", __func__);

	return err;
}

static inline void btmtk_sdio_woble_wake_lock(struct btmtk_sdio_card *data)
{
	if (data->bt_cfg.support_unify_woble && data->bt_cfg.support_woble_wakelock) {
		pr_info("%s:", __func__);
		__pm_stay_awake(&data->woble_ws);
	}
}

static inline void btmtk_sdio_woble_wake_unlock(struct btmtk_sdio_card *data)
{
	if (data->bt_cfg.support_unify_woble && data->bt_cfg.support_woble_wakelock) {
		pr_info("%s:", __func__);
		__pm_relax(&data->woble_ws);
	}
}

u32 LOCK_UNSLEEPABLE_LOCK(struct _OSAL_UNSLEEPABLE_LOCK_ *pUSL)
{
	spin_lock_irqsave(&(pUSL->lock), pUSL->flag);
	return 0;
}

u32 UNLOCK_UNSLEEPABLE_LOCK(struct _OSAL_UNSLEEPABLE_LOCK_ *pUSL)
{
	spin_unlock_irqrestore(&(pUSL->lock), pUSL->flag);
	return 0;
}

static int btmtk_sdio_writesb(u32 offset, u8 *val, int len)
{
	u32 ret = 0;
	u32 retry_count = 0;

	if (g_card->func == NULL) {
		pr_info("%s g_card->func is NULL\n", __func__);
		return -EIO;
	}

	do {
		sdio_claim_host(g_card->func);
		ret = sdio_writesb(g_card->func, offset, val, len);
		sdio_release_host(g_card->func);
		retry_count++;
		if (retry_count > BTMTK_SDIO_RETRY_COUNT) {
			pr_info(" %s, ret:%d\n", __func__, ret);
			break;
		}
	} while (ret);

	return ret;
}


static int btmtk_sdio_readsb(u32 offset, u8 *val, int len)
{
	u32 ret = 0;
	u32 retry_count = 0;

	if (g_card->func == NULL) {
		pr_info("%s g_card->func is NULL\n", __func__);
		return -EIO;
	}

	do {
		sdio_claim_host(g_card->func);
		ret = sdio_readsb(g_card->func, val, offset, len);
		sdio_release_host(g_card->func);
		retry_count++;
		if (retry_count > BTMTK_SDIO_RETRY_COUNT) {
			pr_info(" %s, ret:%d\n", __func__, ret);
			break;
		}
	} while (ret);

	return ret;
}

static int btmtk_sdio_writeb(u32 offset, u8 val)
{
	u32 ret = 0;
	u32 retry_count = 0;

	if (g_card->func == NULL) {
		pr_info("%s g_card->func is NULL\n", __func__);
		return -EIO;
	}

	do {
		sdio_claim_host(g_card->func);
		sdio_writeb(g_card->func, val, offset, &ret);
		sdio_release_host(g_card->func);
		retry_count++;
		if (retry_count > BTMTK_SDIO_RETRY_COUNT) {
			pr_info(" %s, ret:%d\n", __func__, ret);
			break;
		}
	} while (ret);

	return ret;
}

static int btmtk_sdio_readb(u32 offset, u8 *val)
{
	u32 ret = 0;
	u32 retry_count = 0;

	if (g_card->func == NULL) {
		pr_info("%s g_card->func is NULL\n", __func__);
		return -EIO;
	}

	do {
		sdio_claim_host(g_card->func);
		*val = sdio_readb(g_card->func, offset, &ret);
		sdio_release_host(g_card->func);
		retry_count++;
		if (retry_count > BTMTK_SDIO_RETRY_COUNT) {
			pr_info(" %s, ret:%d\n", __func__, ret);
			break;
		}
	} while (ret);

	return ret;
}

static int btmtk_sdio_writel(u32 offset, u32 val)
{
	u32 ret = 0;
	u32 retry_count = 0;

	if (g_card->func == NULL) {
		pr_info("%s g_card->func is NULL\n", __func__);
		return -EIO;
	}

	do {
		sdio_claim_host(g_card->func);
		sdio_writel(g_card->func, val, offset, &ret);
		sdio_release_host(g_card->func);
		retry_count++;
		if (retry_count > BTMTK_SDIO_RETRY_COUNT) {
			pr_info(" %s, ret:%d\n", __func__, ret);
			break;
		}
	} while (ret);

	return ret;
}

static int btmtk_sdio_readl(u32 offset,  u32 *val)
{
	u32 ret = 0;
	u32 retry_count = 0;

	if (g_card->func == NULL) {
		pr_info("g_card->func is NULL\n");
		return -EIO;
	}

	do {
		sdio_claim_host(g_card->func);
		*val = sdio_readl(g_card->func, offset, &ret);
		sdio_release_host(g_card->func);
		retry_count++;
		if (retry_count > BTMTK_SDIO_RETRY_COUNT) {
			pr_info(" %s, ret:%d\n", __func__, ret);
			break;
		}
	} while (ret);

	return ret;
}

static void btmtk_sdio_print_debug_sr(void)
{
	u32 ret = 0;
	u32 CCIR_Value = 0;
	u32 CHLPCR_Value = 0;
	u32 CSDIOCSR_Value = 0;
	u32 CHISR_Value = 0;
	u32 CHIER_Value = 0;
	u32 CTFSR_Value = 0;
	u32 CRPLR_Value = 0;
	u32 SWPCDBGR_Value = 0;
	unsigned char X0_Value = 0;
	unsigned char F8_Value = 0;
	unsigned char F9_Value = 0;
	unsigned char FA_Value = 0;
	unsigned char FB_Value = 0;
	unsigned char FC_Value = 0;
	unsigned char FD_Value = 0;
	unsigned char FE_Value = 0;
	unsigned char FF_Value = 0;

	ret = btmtk_sdio_readl(CCIR, &CCIR_Value);
	ret = btmtk_sdio_readl(CHLPCR, &CHLPCR_Value);
	ret = btmtk_sdio_readl(CSDIOCSR, &CSDIOCSR_Value);
	ret = btmtk_sdio_readl(CHISR, &CHISR_Value);
	ret = btmtk_sdio_readl(CHIER, &CHIER_Value);
	ret = btmtk_sdio_readl(CTFSR, &CTFSR_Value);
	ret = btmtk_sdio_readl(CRPLR, &CRPLR_Value);
	ret = btmtk_sdio_readl(SWPCDBGR, &SWPCDBGR_Value);
	sdio_claim_host(g_card->func);
	X0_Value = sdio_f0_readb(g_card->func, 0x00, &ret);
	F8_Value = sdio_f0_readb(g_card->func, 0xF8, &ret);
	F9_Value = sdio_f0_readb(g_card->func, 0xF9, &ret);
	FA_Value = sdio_f0_readb(g_card->func, 0xFA, &ret);
	FB_Value = sdio_f0_readb(g_card->func, 0xFB, &ret);
	FC_Value = sdio_f0_readb(g_card->func, 0xFC, &ret);
	FD_Value = sdio_f0_readb(g_card->func, 0xFD, &ret);
	FE_Value = sdio_f0_readb(g_card->func, 0xFE, &ret);
	FF_Value = sdio_f0_readb(g_card->func, 0xFF, &ret);
	sdio_release_host(g_card->func);
	pr_info("%s: CCIR: 0x%x, CHLPCR: 0x%x, CSDIOCSR: 0x%x, CHISR: 0x%x, CHIER: 0x%x, CTFSR: 0x%x, CRPLR: 0x%x, SWPCDBGR: 0x%x\n",
		__func__,
		CCIR_Value, CHLPCR_Value, CSDIOCSR_Value, CHISR_Value,
		CHIER_Value, CTFSR_Value, CRPLR_Value, SWPCDBGR_Value);
	pr_info("%s: CCCR 00: 0x%x, F8: 0x%x, F9: 0x%x, FA: 0x%x, FB: 0x%x, FC: 0x%x, FD: 0x%x, FE: 0x%x, FE: 0x%x\n",
		__func__, X0_Value,
		F8_Value, F9_Value, FA_Value, FB_Value,
		FC_Value, FD_Value, FE_Value, FF_Value);
}

static unsigned int btmtk_sdio_get_microseconds(void)
{
	struct timeval now;

	do_gettimeofday(&now);
	return now.tv_sec * 1000000 + now.tv_usec;
}

static void btmtk_sdio_hci_snoop_save(u8 type, u8 *buf, u32 len)
{
	u32 copy_len = HCI_SNOOP_BUF_SIZE;

	if (buf == NULL || len == 0)
		return;

	if (len < HCI_SNOOP_BUF_SIZE)
		copy_len = len;

	switch (type) {
	case HCI_COMMAND_PKT:
		hci_cmd_snoop_len[hci_cmd_snoop_index] = copy_len & 0xff;
		memset(hci_cmd_snoop_buf[hci_cmd_snoop_index], 0, HCI_SNOOP_BUF_SIZE);
		memcpy(hci_cmd_snoop_buf[hci_cmd_snoop_index], buf, copy_len & 0xff);
		hci_cmd_snoop_timestamp[hci_cmd_snoop_index] = btmtk_sdio_get_microseconds();

		hci_cmd_snoop_index--;
		if (hci_cmd_snoop_index < 0)
			hci_cmd_snoop_index = HCI_SNOOP_ENTRY_NUM - 1;
		break;
	case HCI_ACLDATA_PKT:
		hci_acl_snoop_len[hci_acl_snoop_index] = copy_len & 0xff;
		memset(hci_acl_snoop_buf[hci_acl_snoop_index], 0, HCI_SNOOP_BUF_SIZE);
		memcpy(hci_acl_snoop_buf[hci_acl_snoop_index], buf, copy_len & 0xff);
		hci_acl_snoop_timestamp[hci_acl_snoop_index] = btmtk_sdio_get_microseconds();

		hci_acl_snoop_index--;
		if (hci_acl_snoop_index < 0)
			hci_acl_snoop_index = HCI_SNOOP_ENTRY_NUM - 1;
		break;
	case HCI_EVENT_PKT:
		hci_event_snoop_len[hci_event_snoop_index] = copy_len;
		memset(hci_event_snoop_buf[hci_event_snoop_index], 0,
			HCI_SNOOP_BUF_SIZE);
		memcpy(hci_event_snoop_buf[hci_event_snoop_index], buf, copy_len);
		hci_event_snoop_timestamp[hci_event_snoop_index] = btmtk_sdio_get_microseconds();

		hci_event_snoop_index--;
		if (hci_event_snoop_index < 0)
			hci_event_snoop_index = HCI_SNOOP_ENTRY_NUM - 1;
		break;
	default:
		BTSDIO_INFO_RAW(buf, len,
			"%s: type(%d):", __func__, type);
	}
}

static void btmtk_sdio_hci_snoop_print(void)
{
	int counter, index;

	pr_info("%s: HCI Command Dump\n", __func__);
	index = hci_cmd_snoop_index + 1;
	if (index >= HCI_SNOOP_ENTRY_NUM)
		index = 0;
	for (counter = 0; counter < HCI_SNOOP_ENTRY_NUM; counter++) {
		if (hci_cmd_snoop_len[index] > 0)
			BTSDIO_INFO_RAW(hci_cmd_snoop_buf[index], hci_cmd_snoop_len[index],
				"	time(%u):", hci_cmd_snoop_timestamp[index]);
		index++;
		if (index >= HCI_SNOOP_ENTRY_NUM)
			index = 0;
	}

	pr_info("%s: HCI Event Dump", __func__);
	index = hci_event_snoop_index + 1;
	if (index >= HCI_SNOOP_ENTRY_NUM)
		index = 0;
	for (counter = 0; counter < HCI_SNOOP_ENTRY_NUM; counter++) {
		if (hci_event_snoop_len[index] > 0)
			BTSDIO_INFO_RAW(hci_event_snoop_buf[index], hci_event_snoop_len[index],
				"	time(%u):", hci_event_snoop_timestamp[index]);
		index++;
		if (index >= HCI_SNOOP_ENTRY_NUM)
			index = 0;
	}

	pr_info("%s: HCI ACL Dump", __func__);
	index = hci_acl_snoop_index + 1;
	if (index >= HCI_SNOOP_ENTRY_NUM)
		index = 0;
	for (counter = 0; counter < HCI_SNOOP_ENTRY_NUM; counter++) {
		if (hci_acl_snoop_len[index] > 0)
			BTSDIO_INFO_RAW(hci_acl_snoop_buf[index], hci_acl_snoop_len[index],
				"	time(%u):", hci_acl_snoop_timestamp[index]);
		index++;
		if (index >= HCI_SNOOP_ENTRY_NUM)
			index = 0;
	}
}

static void btmtk_usb_hci_snoop_init(void)
{
	hci_cmd_snoop_index = HCI_SNOOP_ENTRY_NUM - 1;
	hci_event_snoop_index = HCI_SNOOP_ENTRY_NUM - 1;
	hci_acl_snoop_index = HCI_SNOOP_ENTRY_NUM - 1;

	memset(hci_cmd_snoop_buf, 0, HCI_SNOOP_ENTRY_NUM * HCI_SNOOP_BUF_SIZE * sizeof(u8));
	memset(hci_cmd_snoop_len, 0, HCI_SNOOP_ENTRY_NUM * sizeof(u8));
	memset(hci_cmd_snoop_timestamp, 0, HCI_SNOOP_ENTRY_NUM * sizeof(unsigned int));

	memset(hci_event_snoop_buf, 0, HCI_SNOOP_ENTRY_NUM * HCI_SNOOP_BUF_SIZE * sizeof(u8));
	memset(hci_event_snoop_len, 0, HCI_SNOOP_ENTRY_NUM * sizeof(u8));
	memset(hci_event_snoop_timestamp, 0, HCI_SNOOP_ENTRY_NUM * sizeof(unsigned int));

	memset(hci_acl_snoop_buf, 0, HCI_SNOOP_ENTRY_NUM * HCI_SNOOP_BUF_SIZE * sizeof(u8));
	memset(hci_acl_snoop_len, 0, HCI_SNOOP_ENTRY_NUM * sizeof(u8));
	memset(hci_acl_snoop_timestamp, 0, HCI_SNOOP_ENTRY_NUM * sizeof(unsigned int));
}

struct sk_buff *btmtk_create_send_data(struct sk_buff *skb)
{
	struct sk_buff *queue_skb = NULL;
	u32 sdio_header_len = skb->len + BTM_HEADER_LEN;

	if (skb_headroom(skb) < (BTM_HEADER_LEN)) {
		queue_skb = bt_skb_alloc(sdio_header_len, GFP_ATOMIC);
		if (queue_skb == NULL) {
			pr_info("bt_skb_alloc fail return\n");
			return 0;
		}

		queue_skb->data[0] = (sdio_header_len & 0x0000ff);
		queue_skb->data[1] = (sdio_header_len & 0x00ff00) >> 8;
		queue_skb->data[2] = 0;
		queue_skb->data[3] = 0;
		queue_skb->data[4] = bt_cb(skb)->pkt_type;
		queue_skb->len = sdio_header_len;
		memcpy(&queue_skb->data[5], &skb->data[0], skb->len);
		kfree_skb(skb);
	} else {
		queue_skb = skb;
		skb_push(queue_skb, BTM_HEADER_LEN);
		queue_skb->data[0] = (sdio_header_len & 0x0000ff);
		queue_skb->data[1] = (sdio_header_len & 0x00ff00) >> 8;
		queue_skb->data[2] = 0;
		queue_skb->data[3] = 0;
		queue_skb->data[4] = bt_cb(skb)->pkt_type;
	}

	pr_info("%s end\n", __func__);
	return queue_skb;
}

static void btmtk_sdio_set_no_fw_own(struct btmtk_private *priv, bool no_fw_own)
{
	if (priv) {
		if (priv->no_fw_own == no_fw_own)
			return;
		priv->no_fw_own = no_fw_own;
		pr_info("btmtk_sdio_set_no_fw_own set no_fw_own %d\n", priv->no_fw_own);
	} else
		pr_info("btmtk_sdio_set_no_fw_own priv is NULL\n");
}

static int btmtk_sdio_set_own_back(int owntype)
{
	/*Set driver own*/
	int ret = 0, retry_ret = 0;
	u32 u32LoopCount = 0;
	u32 u32ReadCRValue = 0;
	u32 ownValue = 0;
	u32 set_checkretry = 30;
	int i = 0;

	pr_debug("%s owntype %d\n", __func__, owntype);

	if (user_rmmod)
		set_checkretry = 1;


	if (owntype == FW_OWN && (g_priv)) {
		if (g_priv->no_fw_own) {
			ret = btmtk_sdio_readl(SWPCDBGR, &u32ReadCRValue);
			pr_info("%s no_fw_own is on, just return, u32ReadCRValue = 0x%08X, ret = %d\n",
				__func__, u32ReadCRValue, ret);
			return ret;
		}
	}

	ret = btmtk_sdio_readl(CHLPCR, &u32ReadCRValue);

	pr_debug("%s btmtk_sdio_readl  CHLPCR done\n", __func__);
	if (owntype == DRIVER_OWN) {
		if ((u32ReadCRValue&0x100) == 0x100) {
			pr_debug("%s already driver own 0x%0x, return\n",
				__func__, u32ReadCRValue);
			goto set_own_end;
		}
	} else if (owntype == FW_OWN) {
		if ((u32ReadCRValue&0x100) == 0) {
			pr_debug("%s already FW own 0x%0x, return\n",
				__func__, u32ReadCRValue);
			goto set_own_end;
		}
	}

setretry:

	if (owntype == DRIVER_OWN)
		ownValue = 0x00000200;
	else
		ownValue = 0x00000100;

	pr_debug("%s write CHLPCR 0x%x\n", __func__, ownValue);
	ret = btmtk_sdio_writel(CHLPCR, ownValue);
	if (ret) {
		ret = -EINVAL;
		goto done;
	}
	pr_debug("%s write CHLPCR 0x%x done\n", __func__, ownValue);

	u32LoopCount = 1000;

	if (owntype == DRIVER_OWN) {
		do {
			usleep_range(100, 200);
			ret = btmtk_sdio_readl(CHLPCR, &u32ReadCRValue);
			u32LoopCount--;
			pr_debug("%s DRIVER_OWN btmtk_sdio_readl CHLPCR 0x%x\n",
				__func__, u32ReadCRValue);
		} while ((u32LoopCount > 0) &&
			((u32ReadCRValue&0x100) != 0x100));

		if ((u32LoopCount == 0) && (0x100 != (u32ReadCRValue&0x100))
				&& (set_checkretry > 0)) {
			pr_info("%s retry set_check driver own, CHLPCR 0x%x\n",
				__func__, u32ReadCRValue);
			for (i = 0; i < 3; i++) {
				ret = btmtk_sdio_readl(SWPCDBGR, &u32ReadCRValue);
				pr_info("%s ret %d,SWPCDBGR 0x%x, and not sleep!\n",
					__func__, ret, u32ReadCRValue);
			}
			btmtk_sdio_print_debug_sr();

			set_checkretry--;
			mdelay(20);
			goto setretry;
		}
	} else {
		do {
			usleep_range(100, 200);
			ret = btmtk_sdio_readl(CHLPCR, &u32ReadCRValue);
			u32LoopCount--;
			pr_debug("%s FW_OWN btmtk_sdio_readl CHLPCR 0x%x\n",
				__func__, u32ReadCRValue);
		} while ((u32LoopCount > 0) && ((u32ReadCRValue&0x100) != 0));

		if ((u32LoopCount == 0) &&
				((u32ReadCRValue&0x100) != 0) &&
				(set_checkretry > 0)) {
			pr_info("%s retry set_check FW own, CHLPCR 0x%x\n",
				__func__, u32ReadCRValue);
			set_checkretry--;
			goto setretry;
		}
	}

	pr_debug("%s CHLPCR(0x%x), is 0x%x\n",
		__func__, CHLPCR, u32ReadCRValue);

	if (owntype == DRIVER_OWN) {
		if ((u32ReadCRValue&0x100) == 0x100)
			pr_debug("%s check %04x, is 0x100 driver own success\n",
				__func__, CHLPCR);
		else {
			pr_debug("%s check %04x, is %x shuld be 0x100\n",
				__func__, CHLPCR, u32ReadCRValue);
			ret = -EINVAL;
			goto done;
		}
	} else {
		if (0x0 == (u32ReadCRValue&0x100))
			pr_debug("%s check %04x, bit 8 is 0 FW own success\n",
				__func__, CHLPCR);
		else{
			pr_debug("%s bit 8 should be 0, %04x bit 8 is %04x\n",
				__func__, u32ReadCRValue,
				(u32ReadCRValue&0x100));
			ret = -EINVAL;
			goto done;
		}
	}

done:
	if (owntype == DRIVER_OWN) {
		if (ret) {
			pr_info("%s set driver own fail\n", __func__);
			for (i = 0; i < 8; i++) {
				retry_ret = btmtk_sdio_readl(SWPCDBGR, &u32ReadCRValue);
				pr_info("%s ret %d,SWPCDBGR 0x%x, then sleep 200ms\n",
					__func__, retry_ret, u32ReadCRValue);
				msleep(200);
			}
		} else
			pr_debug("%s set driver own success\n", __func__);
	} else if (owntype == FW_OWN) {
		if (ret)
			pr_info("%s set FW own fail\n", __func__);
		else
			pr_debug("%s set FW own success\n", __func__);
	} else
		pr_info("%s unknown type %d\n", __func__, owntype);

set_own_end:
	if (ret)
		btmtk_sdio_print_debug_sr();

	return ret;
}

static int btmtk_sdio_enable_interrupt(int enable)
{
	u32 ret = 0;
	u32 cr_value = 0;

	if (enable)
		cr_value |= C_FW_INT_EN_SET;
	else
		cr_value |= C_FW_INT_EN_CLEAR;

	ret = btmtk_sdio_writel(CHLPCR, cr_value);
	pr_debug("%s enable %d write CHLPCR 0x%08x\n",
			__func__, enable, cr_value);

	return ret;
}

static int btmtk_sdio_get_rx_unit(struct btmtk_sdio_card *card)
{
	u8 reg;
	int ret;

	ret = btmtk_sdio_readb(card->reg->card_rx_unit, &reg);
	if (!ret)
		card->rx_unit = reg;

	return ret;
}

static int btmtk_sdio_enable_host_int_mask(
				struct btmtk_sdio_card *card,
				u8 mask)
{
	int ret;

	ret = btmtk_sdio_writeb(card->reg->host_int_mask, mask);
	if (ret) {
		pr_info("Unable to enable the host interrupt!\n");
		ret = -EIO;
	}

	return ret;
}

static int btmtk_sdio_disable_host_int_mask(
				struct btmtk_sdio_card *card,
				u8 mask)
{
	u8 host_int_mask;
	int ret;

	ret = btmtk_sdio_readb(card->reg->host_int_mask, &host_int_mask);
	if (ret)
		return -EIO;

	host_int_mask &= ~mask;

	ret = btmtk_sdio_writeb(card->reg->host_int_mask, host_int_mask);
	if (ret < 0) {
		pr_info("Unable to disable the host interrupt!\n");
		return -EIO;
	}

	return 0;
}

/*for debug*/
int btmtk_print_buffer_conent(u8 *buf, u32 Datalen)
{
	int i = 0;
	int print_finish = 0;
	/*Print out txbuf data for debug*/
	for (i = 0; i <= (Datalen-1); i += 16) {
		if ((i+16) <= Datalen) {
			pr_debug("%s: %02X%02X%02X%02X%02X %02X%02X%02X%02X%02X %02X%02X%02X%02X%02X %02X\n",
				__func__,
				buf[i], buf[i+1], buf[i+2], buf[i+3],
				buf[i+4], buf[i+5], buf[i+6], buf[i+7],
				buf[i+8], buf[i+9], buf[i+10], buf[i+11],
				buf[i+12], buf[i+13], buf[i+14], buf[i+15]);
		} else {
			for (; i < (Datalen); i++)
				pr_debug("%s: %02X\n", __func__, buf[i]);

			print_finish = 1;
		}

		if (print_finish)
			break;
	}
	return 0;
}

static int btmtk_sdio_send_tx_data(u8 *buffer, int tx_data_len)
{
	int ret = 0;
	u8 MultiBluckCount = 0;
	u8 redundant = 0;

	MultiBluckCount = tx_data_len/SDIO_BLOCK_SIZE;
	redundant = tx_data_len % SDIO_BLOCK_SIZE;

	if (redundant)
		tx_data_len = (MultiBluckCount+1)*SDIO_BLOCK_SIZE;

	ret = btmtk_sdio_writesb(CTDR, buffer, tx_data_len);
	return ret;
}

static int btmtk_sdio_recv_rx_data(void)
{
	int ret = 0;
	u32 u32ReadCRValue = 0;
	int retry_count = 500;
	u32 sdio_header_length = 0;

	memset(rxbuf, 0, MTK_RXDATA_SIZE);

	do {
		ret = btmtk_sdio_readl(CHISR, &u32ReadCRValue);
		pr_debug("%s: loop Get CHISR 0x%08X\n",
			__func__, u32ReadCRValue);
		reg_CHISR = u32ReadCRValue;
		rx_length = (u32ReadCRValue & RX_PKT_LEN) >> 16;
		if (rx_length == 0xFFFF) {
			pr_info("%s: 0xFFFF==rx_length, error return -EIO\n",
				__func__);
			ret = -EIO;
			break;
		}

		if ((RX_DONE&u32ReadCRValue) && rx_length) {
			pr_debug("%s: u32ReadCRValue = %08X\n",
				__func__, u32ReadCRValue);
			u32ReadCRValue &= 0xFFFB;
			ret = btmtk_sdio_writel(CHISR, u32ReadCRValue);
			pr_debug("%s: write = %08X\n",
				__func__, u32ReadCRValue);


			ret = btmtk_sdio_readsb(CRDR, rxbuf, rx_length);
			sdio_header_length = (rxbuf[1] << 8);
			sdio_header_length |= rxbuf[0];

			if (sdio_header_length != rx_length) {
				pr_info("%s sdio header length %d, rx_length %d mismatch\n",
					__func__, sdio_header_length,
					rx_length);
				break;
			}

			if (sdio_header_length == 0) {
				pr_info("%s: get sdio_header_length = %d\n",
					__func__, sdio_header_length);
				continue;
			}

			break;
		}

		retry_count--;
		if (retry_count <= 0) {
			pr_info("%s: retry_count = %d,timeout\n",
				__func__, retry_count);
			btmtk_sdio_print_debug_sr();
			ret = -EIO;
			break;
		}

		/* msleep(1); */
		mdelay(3);
		pr_debug("%s: retry_count = %d,wait\n", __func__, retry_count);

		if (ret)
			break;
	} while (1);

	if (ret)
		return -EIO;

	return ret;
}

static int btmtk_sdio_send_wmt_reset(void)
{
	int ret = 0;
	u8 wmt_event[8] = {4, 0xE4, 5, 2, 7, 1, 0, 0};
	u8 mtksdio_packet_header[MTK_SDIO_PACKET_HEADER_SIZE] = {0};
	u8 mtksdio_wmt_reset[9] = {1, 0x6F, 0xFC, 5, 1, 7, 1, 0, 4};

	pr_info("%s:\n", __func__);
	mtksdio_packet_header[0] = sizeof(mtksdio_packet_header) +
		sizeof(mtksdio_wmt_reset);

	memcpy(txbuf, mtksdio_packet_header, MTK_SDIO_PACKET_HEADER_SIZE);
	memcpy(txbuf+MTK_SDIO_PACKET_HEADER_SIZE, mtksdio_wmt_reset,
		sizeof(mtksdio_wmt_reset));

	btmtk_sdio_send_tx_data(txbuf,
		MTK_SDIO_PACKET_HEADER_SIZE+sizeof(mtksdio_wmt_reset));
	btmtk_sdio_recv_rx_data();

	/*compare rx data is wmt reset correct response or not*/
	if (memcmp(wmt_event, rxbuf+MTK_SDIO_PACKET_HEADER_SIZE,
			sizeof(wmt_event)) != 0) {
		ret = -EIO;
		pr_info("%s: fail\n", __func__);
	}

	return ret;
}

static u32 btmtk_sdio_bt_memRegister_read(u32 cr)
{
	int retrytime = 3;
	u32 result = 0;
	u8 wmt_event[15] = {0x04, 0xE4, 0x10, 0x02,
				0x08, 0x0C/*0x1C*/, 0x00, 0x00,
				0x00, 0x00, 0x01, 0x00,
				0x00, 0x00, 0x80};
	/* msleep(1000); */
	u8 mtksdio_packet_header[MTK_SDIO_PACKET_HEADER_SIZE] = {0};
	u8 mtksdio_wmt_cmd[16] = {0x1, 0x6F, 0xFC, 0x0C,
				0x01, 0x08, 0x08, 0x00,
				0x02, 0x01, 0x00, 0x01,
				0x00, 0x00, 0x00, 0x00};
	mtksdio_packet_header[0] = sizeof(mtksdio_packet_header)
				+ sizeof(mtksdio_wmt_cmd);
	pr_info("%s: read cr %x\n", __func__, cr);

	memcpy(&mtksdio_wmt_cmd[12], &cr, sizeof(cr));

	memcpy(txbuf, mtksdio_packet_header, MTK_SDIO_PACKET_HEADER_SIZE);
	memcpy(txbuf + MTK_SDIO_PACKET_HEADER_SIZE, mtksdio_wmt_cmd,
		sizeof(mtksdio_wmt_cmd));

	btmtk_sdio_send_tx_data(txbuf,
		MTK_SDIO_PACKET_HEADER_SIZE + sizeof(mtksdio_wmt_cmd));
	btmtk_print_buffer_conent(txbuf,
		MTK_SDIO_PACKET_HEADER_SIZE + sizeof(mtksdio_wmt_cmd));

	do {
		usleep_range(10*1000, 15*1000);
		btmtk_sdio_recv_rx_data();
		retrytime--;
		if (retrytime <= 0)
			break;

		pr_info("%s: retrytime %d\n", __func__, retrytime);
	} while (!rxbuf[0]);

	btmtk_print_buffer_conent(rxbuf, rx_length);
	/* compare rx data is wmt reset correct response or not */
#if 0
	if (memcmp(wmt_event,
			rxbuf+MTK_SDIO_PACKET_HEADER_SIZE,
			sizeof(wmt_event)) != 0) {
		ret = -EIO;
		pr_info("%s: fail\n", __func__);
	}
#endif
	memcpy(&result, rxbuf+MTK_SDIO_PACKET_HEADER_SIZE + sizeof(wmt_event),
		sizeof(result));
	pr_info("%s: ger cr 0x%x value 0x%x\n", __func__, cr, result);
	return result;
}

/* 1:on ,  0:off */
static int btmtk_sdio_bt_set_power(u8 onoff)
{
	int ret = 0;
	u8 event[] = {0xE4, 0x05, 0x02, 0x06, 0x01, 0x00, 0x00};
	u8 cmd[] = {0x6F, 0xFC, 0x06, 0x01, 0x06, 0x02, 0x00, 0x00, 0x01};

	pr_info("%s: onoff %d\n", __func__, onoff);
	cmd[8] = onoff;

	ret = btmtk_sdio_send_hci_cmd(HCI_COMMAND_PKT, cmd, sizeof(cmd),
		event, sizeof(event), COMP_EVENT_TIMO, 1);
	if (ret)
		g_card->dongle_state = BT_SDIO_DONGLE_STATE_ERROR;
	else
		g_card->dongle_state =
			onoff ? BT_SDIO_DONGLE_STATE_POWER_ON : BT_SDIO_DONGLE_STATE_POWER_OFF;

	return ret;
}

static int btmtk_sdio_send_init_cmds(struct btmtk_sdio_card *card)
{
	if (btmtk_sdio_bt_set_power(1)) {
		pr_info("%s power on failed\n", __func__);
		return -EIO;
	}

	/* Send hci cmd before sleep */
	btmtk_eeprom_bin_file(card);

	if (btmtk_sdio_set_sleep()) {
		pr_info("%s set sleep failed\n", __func__);
		return -EIO;
	}

	if (btmtk_sdio_set_audio()) {
		pr_info("%s set audio failed\n", __func__);
		return -EIO;
	}

	return 0;
}

static int btmtk_sdio_send_deinit_cmds(void)
{
	if (btmtk_sdio_bt_set_power(0)) {
		pr_info("%s power off failed\n", __func__);
		return -EIO;
	}
	return 0;
}

#if 0
static int btmtk_sdio_send_and_check(u8 *cmd, u16 cmd_len,
						u8 *event, u16 event_len)
{
	int ret = 0;
	int retrytime = 60;
	int len = 0;
	u8 mtksdio_packet_header[MTK_SDIO_PACKET_HEADER_SIZE] = {0};

	len = MTK_SDIO_PACKET_HEADER_SIZE + cmd_len;

	mtksdio_packet_header[0] = (len & 0x0000ff);
	mtksdio_packet_header[1] = (len & 0x00ff00) >> 8;

	memcpy(txbuf, mtksdio_packet_header, MTK_SDIO_PACKET_HEADER_SIZE);
	memcpy(txbuf + MTK_SDIO_PACKET_HEADER_SIZE, cmd, cmd_len);

	btmtk_sdio_send_tx_data(txbuf, len);

	if (event && (event_len != 0)) {
		do {
			msleep(100);
			btmtk_sdio_recv_rx_data();
			retrytime--;
			if (retrytime <= 0)
				break;

			if (retrytime < 40)
				pr_info("%s: retry over 2s, retrytime %d\n",
					__func__, retrytime);
		} while (!rxbuf[0]);

		if (memcmp(event, rxbuf + MTK_SDIO_PACKET_HEADER_SIZE,
				event_len) != 0) {
			ret = -EIO;
			pr_info("%s: fail\n", __func__);
		}
	}

	return ret;
}
#endif

/*get 1 byte only*/
static int btmtk_efuse_read(u16 addr, u8 *value)
{
	uint8_t efuse_r[] = {0x6F, 0xFC, 0x0E,
				0x01, 0x0D, 0x0A, 0x00, 0x02, 0x04,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00};/*4 sub block number(sub block 0~3)*/

	uint8_t efuse_r_event[] = {0xE4, 0x1E, 0x02, 0x0D, 0x1A, 0x00, 02, 04};
	/*check event
	 *04 E4 LEN(1B) 02 0D LEN(2Byte) 02 04 ADDR(2Byte) VALUE(4B) ADDR(2Byte) VALUE(4Byte)
	 *ADDR(2Byte) VALUE(4B)  ADDR(2Byte) VALUE(4Byte)
	 */
	int ret = 0;
	uint8_t sub_block_addr_in_event = 0;
	uint16_t sub_block = (addr / 16) * 4;
	uint8_t temp = 0;

	efuse_r[9] = sub_block & 0xFF;
	efuse_r[10] = (sub_block & 0xFF00) >> 8;
	efuse_r[11] = (sub_block + 1) & 0xFF;
	efuse_r[12] = ((sub_block + 1) & 0xFF00) >> 8;
	efuse_r[13] = (sub_block + 2) & 0xFF;
	efuse_r[14] = ((sub_block + 2) & 0xFF00) >> 8;
	efuse_r[15] = (sub_block + 3) & 0xFF;
	efuse_r[16] = ((sub_block + 3) & 0xFF00) >> 8;

	ret = btmtk_sdio_send_hci_cmd(HCI_COMMAND_PKT, efuse_r,
		sizeof(efuse_r),
		efuse_r_event, sizeof(efuse_r_event), COMP_EVENT_TIMO, 1);
	if (ret) {
		pr_info("%s: btmtk_sdio_send_hci_cmd error\n", __func__);
		pr_info("%s: rx_length %d\n", __func__, rx_length);
		return ret;
	}

	if (memcmp(rxbuf + MTK_SDIO_PACKET_HEADER_SIZE + 1, efuse_r_event, sizeof(efuse_r_event)) == 0) {
		/*compare rxbuf format ok, compare addr*/
		pr_debug("%s: compare rxbuf format ok\n", __func__);
		if (efuse_r[9] == rxbuf[9 + MTK_SDIO_PACKET_HEADER_SIZE] &&
			efuse_r[10] == rxbuf[10 + MTK_SDIO_PACKET_HEADER_SIZE] &&
			efuse_r[11] == rxbuf[15 + MTK_SDIO_PACKET_HEADER_SIZE] &&
			efuse_r[12] == rxbuf[16 + MTK_SDIO_PACKET_HEADER_SIZE] &&
			efuse_r[13] == rxbuf[21 + MTK_SDIO_PACKET_HEADER_SIZE] &&
			efuse_r[14] == rxbuf[22 + MTK_SDIO_PACKET_HEADER_SIZE] &&
			efuse_r[15] == rxbuf[27 + MTK_SDIO_PACKET_HEADER_SIZE] &&
			efuse_r[16] == rxbuf[28 + MTK_SDIO_PACKET_HEADER_SIZE]) {

			pr_debug("%s: address compare ok\n", __func__);
			/*Get value*/
			sub_block_addr_in_event = ((addr / 16) / 4);/*cal block num*/
			temp = addr % 16;
			pr_debug("%s: address in block %d\n", __func__, temp);
			switch (temp) {
			case 0:
			case 1:
			case 2:
			case 3:
				*value = rxbuf[11 + temp + MTK_SDIO_PACKET_HEADER_SIZE];
				break;
			case 4:
			case 5:
			case 6:
			case 7:
				*value = rxbuf[17 + temp + MTK_SDIO_PACKET_HEADER_SIZE];
				break;
			case 8:
			case 9:
			case 10:
			case 11:
				*value = rxbuf[22 + temp + MTK_SDIO_PACKET_HEADER_SIZE];
				break;

			case 12:
			case 13:
			case 14:
			case 15:
				*value = rxbuf[34 + temp + MTK_SDIO_PACKET_HEADER_SIZE];
				break;
			}


		} else {
			pr_info("%s: address compare fail\n", __func__);
			ret = -1;
		}


	} else {
		pr_info("%s: compare rxbuf format fail\n", __func__);
		ret = -1;
	}

	return ret;
}


static bool btmtk_is_bin_file_mode(uint8_t *buf, size_t buf_size)
{
	char *p_buf = NULL;
	char *ptr = NULL, *p = NULL;
	bool ret = true;
	u16 addr = 1;
	u8 value = 0;

	if (!buf) {
		pr_info("%s: buf is null\n", __func__);
		return false;
	} else if (buf_size < (strlen(E2P_MODE) + 2)) {
		pr_info("%s: incorrect buf size(%d)\n",
			__func__, (int)buf_size);
		return false;
	}

	p_buf = kmalloc(buf_size + 1, GFP_KERNEL);
	if (!p_buf)
		return false;
	memcpy(p_buf, buf, buf_size);
	p_buf[buf_size] = '\0';

	/* find string */
	p = ptr = strstr(p_buf, E2P_MODE);
	if (!ptr) {
		pr_notice("%s: Can't find %s\n", __func__, E2P_MODE);
		ret = false;
		goto out;
	}

	if (p > p_buf) {
		p--;
		while ((*p == ' ') && (p != p_buf))
			p--;
		if (*p == '#') {
			pr_notice("%s: It's not EEPROM - Bin file mode\n", __func__);
			ret = false;
			goto out;
		}
	}

	/* check access mode */
	ptr += (strlen(E2P_MODE) + 1);

	if (*ptr == AUTO_MODE) {
		pr_notice("%s: It's Auto mode: %c\n", __func__, *ptr);
		ret = btmtk_efuse_read(addr, &value);
		if (ret) {
			pr_notice("%s: btmtk_efuse_read fail\n", __func__);
			pr_notice("%s: Use EEPROM Bin file mode\n", __func__);
			ret = true;
		} else if (value == 0x76) {
			pr_notice("%s: get efuse[1]: 0x%02x\n", __func__, value);
			pr_notice("%s: use efuse mode", __func__);
			ret = false;
		} else {
			pr_notice("%s: get efuse[1]: 0x%02x\n", __func__, value);
			pr_notice("%s: Use EEPROM Bin file mode\n", __func__);
			ret = true;
		}
		goto out;
	} else if (*ptr == BIN_FILE_MODE)
		pr_notice("%s: It's EEPROM bin mode: %c\n", __func__, *ptr);
	else {
		pr_info("%s: It's not Auto/EEPROM mode %c\n", __func__, *ptr);
		ret = false;
	}

out:
	kfree(p_buf);
	return ret;
}

static void btmtk_set_eeprom2ctrler(uint8_t *buf,
						size_t buf_size,
						int device)
{
	int ret = -1;
	uint8_t set_bdaddr[] = {0x1A, 0xFC, 0x06,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	uint8_t set_bdaddr_e[] = {0x0E, 0x04, 0x01,
			0x1A, 0xFC, 0x00};
	uint8_t set_radio[] = {0x79, 0xFC, 0x08,
			0x07, 0x80, 0x00, 0x06, 0x07, 0x07, 0x00, 0x00};
	uint8_t set_radio_e[] = {0x0E, 0x04, 0x01,
			0x79, 0xFC, 0x00};
	uint8_t set_pwr_offset[] = {0x93, 0xFC, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	uint8_t set_pwr_offset_e[] = {0x0E, 0x04, 0x01,
			0x93, 0xFC, 0x00};
	uint8_t set_xtal[] = {0x0E, 0xFC, 0x02, 0x00, 0x00};
	uint8_t set_xtal_e[] = {0x0E, 0x04, 0x01,
			0x0E, 0xFC, 0x00};
	uint16_t offset = 0;

	pr_notice("%s start, device: 0x%x\n", __func__, device);

	if (!buf) {
		pr_info("%s: buf is null\n", __func__);
		return;
	} else if (device == 0x7668 && buf_size < 0x389) {
		pr_info("%s: incorrect buf size(%d)\n",
			__func__, (int)buf_size);
		return;
	} else if (device == 0x7663 && buf_size < 0x389) {
		pr_info("%s: incorrect buf size(%d)\n",
			__func__, (int)buf_size);
		return;
	}

	/* set BD address */
	if (device == 0x7668)
		offset = 0x384;
	else if (device == 0x7663)
		offset = 0x131;
	else
		offset = 0x1A;

	set_bdaddr[3] = *(buf + offset);
	set_bdaddr[4] = *(buf + offset + 1);
	set_bdaddr[5] = *(buf + offset + 2);
	set_bdaddr[6] = *(buf + offset + 3);
	set_bdaddr[7] = *(buf + offset + 4);
	set_bdaddr[8] = *(buf + offset + 5);
	if (0x0 == set_bdaddr[3] && 0x0 == set_bdaddr[4]
			&& 0x0 == set_bdaddr[5] && 0x0 == set_bdaddr[6]
			&& 0x0 == set_bdaddr[7] && 0x0 == set_bdaddr[8]) {
		pr_notice("%s: BDAddr is Zero, not set\n", __func__);
	} else {
		ret = btmtk_sdio_send_hci_cmd(HCI_COMMAND_PKT, set_bdaddr,
			set_bdaddr[2] + 3,
			set_bdaddr_e, sizeof(set_bdaddr_e), COMP_EVENT_TIMO, 1);
		pr_notice("%s: set BDAddress(%02X-%02X-%02X-%02X-%02X-%02X) %s\n",
				__func__,
				set_bdaddr[8], set_bdaddr[7], set_bdaddr[6],
				set_bdaddr[5], set_bdaddr[4], set_bdaddr[3],
				ret < 0 ? "fail" : "OK");
	}
	/* radio setting - BT power */
	if (device == 0x7668) {
		offset = 0x382;
		/* BT default power */
		set_radio[3] = (*(buf + offset) & 0x07);
		/* BLE default power */
		set_radio[7] = (*(buf + offset + 1) & 0x07);
		/* TX MAX power */
		set_radio[8] = (*(buf + offset) & 0x70) >> 4;
		/* TX power sub level */
		set_radio[9] = (*(buf + offset + 1) & 0x30) >> 4;
		/* BR/EDR power diff mode */
		set_radio[10] = (*(buf + offset + 1) & 0xc0) >> 6;
	} else if (device == 0x7663) {
		offset = 0x137;
		/* BT default power */
		set_radio[3] = (*(buf + offset) & 0x07);
		/* BLE default power */
		set_radio[7] = (*(buf + offset + 1) & 0x07);
		/* TX MAX power */
		set_radio[8] = (*(buf + offset) & 0x70) >> 4;
		/* TX power sub level */
		set_radio[9] = (*(buf + offset + 1) & 0x30) >> 4;
		/* BR/EDR power diff mode */
		set_radio[10] = (*(buf + offset + 1) & 0xc0) >> 6;
	} else {
		offset = 0x132;
		/* BT default power */
		set_radio[3] = *(buf + offset);
		/* BLE default power(no this for 7662 in table) */
		set_radio[7] = *(buf + offset);
		/* TX MAX power */
		set_radio[8] = *(buf + offset + 1);
	}
	ret = btmtk_sdio_send_hci_cmd(HCI_COMMAND_PKT, set_radio,
		set_radio[2] + 3,
		set_radio_e, sizeof(set_radio_e), COMP_EVENT_TIMO, 1);
	pr_notice("%s: set radio(BT/BLE default power: %d/%d MAX power: %d) %s\n",
			__func__,
			set_radio[3], set_radio[7], set_radio[8],
			ret < 0 ? "fail" : "OK");

	/*
	 * BT TX power compensation for low, middle and high
	 * channel
	 */
	if (device == 0x7668) {
		offset = 0x36D;
		/* length */
		set_pwr_offset[2] = 6;
		/* Group 0 CH 0 ~ CH14 */
		set_pwr_offset[3] = *(buf + offset);
		/* Group 1 CH15 ~ CH27 */
		set_pwr_offset[4] = *(buf + offset + 1);
		/* Group 2 CH28 ~ CH40 */
		set_pwr_offset[5] = *(buf + offset + 2);
		/* Group 3 CH41 ~ CH53 */
		set_pwr_offset[6] = *(buf + offset + 3);
		/* Group 4 CH54 ~ CH66 */
		set_pwr_offset[7] = *(buf + offset + 4);
		/* Group 5 CH67 ~ CH84 */
		set_pwr_offset[8] = *(buf + offset + 5);
	} else if (device == 0x7663) {
		offset = 0x180;
		/* length */
		set_pwr_offset[2] = 16;
		/* Group 0 CH 0 ~ CH6 */
		set_pwr_offset[3] = *(buf + offset);
		/* Group 1 CH7 ~ CH11 */
		set_pwr_offset[4] = *(buf + offset + 1);
		/* Group 2 CH12 ~ CH16 */
		set_pwr_offset[5] = *(buf + offset + 2);
		/* Group 3 CH17 ~ CH21 */
		set_pwr_offset[6] = *(buf + offset + 3);
		/* Group 4 CH22 ~ CH26 */
		set_pwr_offset[7] = *(buf + offset + 4);
		/* Group 5 CH27 ~ CH31 */
		set_pwr_offset[8] = *(buf + offset + 5);
		/* Group 6 CH32 ~ CH36 */
		set_pwr_offset[9] = *(buf + offset + 6);
		/* Group 7 CH37 ~ CH41 */
		set_pwr_offset[10] = *(buf + offset + 7);
		/* Group 8 CH42 ~ CH46 */
		set_pwr_offset[11] = *(buf + offset + 8);
		/* Group 9 CH47 ~ CH51 */
		set_pwr_offset[12] = *(buf + offset + 9);
		/* Group 10 CH52 ~ CH56 */
		set_pwr_offset[13] = *(buf + offset + 10);
		/* Group 11 CH57 ~ CH61 */
		set_pwr_offset[14] = *(buf + offset + 11);
		/* Group 12 CH62 ~ CH66 */
		set_pwr_offset[15] = *(buf + offset + 12);
		/* Group 13 CH67 ~ CH71 */
		set_pwr_offset[16] = *(buf + offset + 13);
		/* Group 14 CH72 ~ CH76 */
		set_pwr_offset[17] = *(buf + offset + 14);
		/* Group 15 CH77 ~ CH78 */
		set_pwr_offset[18] = *(buf + offset + 15);
	} else {
		offset = 0x139;
		/* length */
		set_pwr_offset[2] = 3;
		/* low channel */
		set_pwr_offset[3] = *(buf + offset);
		/* middle channel */
		set_pwr_offset[4] = *(buf + offset + 1);
		/* high channel */
		set_pwr_offset[5] = *(buf + offset + 2);
	}
	ret = btmtk_sdio_send_hci_cmd(HCI_COMMAND_PKT, set_pwr_offset,
		set_pwr_offset[2] + 3,
		set_pwr_offset_e, sizeof(set_pwr_offset_e), COMP_EVENT_TIMO, 1);
	pr_notice("%s: set power offset(%02X %02X %02X %02X %02X %02X) %s\n",
			__func__,
			set_pwr_offset[3], set_pwr_offset[4],
			set_pwr_offset[5], set_pwr_offset[6],
			set_pwr_offset[7], set_pwr_offset[8],
			ret < 0 ? "fail" : "OK");

	/* XTAL setting */
	if (device == 0x7668) {
		offset = 0xF4;
		/* BT default power */
		set_xtal[3] = *(buf + offset);
		set_xtal[4] = *(buf + offset + 1);
		ret = btmtk_sdio_send_hci_cmd(HCI_COMMAND_PKT, set_xtal,
			set_xtal[2] + 3,
			set_xtal_e, sizeof(set_xtal_e), COMP_EVENT_TIMO, 1);
		pr_notice("%s: set XTAL(0x%02X %02X) %s\n",
				__func__,
				set_xtal[3], set_xtal[4],
				ret < 0 ? "fail" : "OK");
	}
	pr_notice("%s end\n", __func__);
}

static void btmtk_set_pa(uint8_t pa)
{
	int ret = -1;
	uint8_t epa[] = {0x70, 0xFD, 0x09,
		0x00,
		0x07, 0x00, 0x00, 0x00, 0x07, 0x07, 0x00, 0x00};
	uint8_t epa_e[] = {0x0E, 0x04, 0x01,
		0x70, 0xFD, 0x00};

	epa[3] = pa;
	if (pa > 1 || pa < 0) {
		pr_info("%s: Incorrect format", __func__);
		return;
	}
	if (pa == 1) {
		pr_notice("ePA mode, change power level to level 9.");
		epa[4] = 0x09;
		epa[8] = 0x09;
		epa[9] = 0x09;
	}

	ret = btmtk_sdio_send_hci_cmd(HCI_COMMAND_PKT, epa, sizeof(epa),
		epa_e, sizeof(epa_e), COMP_EVENT_TIMO, 1);
	pr_notice("%s: set PA(%d) %s\n",
		__func__,
		pa,
		ret < 0 ? "fail" : "OK");
}

static void btmtk_set_duplex(uint8_t duplex)
{
	int ret = -1;
	uint8_t ant[] = {0x71, 0xFD, 0x01, 0x00};
	uint8_t ant_e[] = {0x0E, 0x04, 0x01,
		0x71, 0xFD, 0x00};

	ant[3] = duplex;
	if (duplex > 1 || duplex < 0) {
		pr_info("%s: Incorrect format", __func__);
		return;
	}

	ret = btmtk_sdio_send_hci_cmd(HCI_COMMAND_PKT, ant, sizeof(ant),
		ant_e, sizeof(ant_e), COMP_EVENT_TIMO, 1);
	pr_notice("%s: set Duplex(%d) %s\n",
		__func__,
		duplex,
		ret < 0 ? "fail" : "OK");
}

static void btmtk_set_pa_and_duplex(uint8_t *buf, size_t buf_size)
{
	char *p_buf = NULL;
	char *ptr = NULL, *p = NULL;

	if (!buf) {
		pr_info("%s: buf is null\n", __func__);
		return;
	} else if (buf_size < (strlen(E2P_MODE) + 2)) {
		pr_info("%s: incorrect buf size(%d)\n",
			__func__, (int)buf_size);
		return;
	}
	p_buf = kmalloc(buf_size + 1, GFP_KERNEL);
	if (!p_buf)
		return;

	memcpy(p_buf, buf, buf_size);
	p_buf[buf_size] = '\0';
	/* find string */
	p = ptr = strstr(p_buf, E2P_ACCESS_EPA);
	if (!ptr) {
		pr_info("%s: Can't find %s\n", __func__, E2P_ACCESS_EPA);
		g_card->pa_setting = -1;
		g_card->duplex_setting = -1;
		goto out;
	}
	if (p > p_buf) {
		p--;
		while ((*p == ' ') && (p != p_buf))
			p--;
		if (*p == '#') {
			pr_info("%s: It's no pa setting\n", __func__);
			g_card->pa_setting = -1;
			g_card->duplex_setting = -1;
			goto out;
		}
	}
	/* check access mode */
	ptr += (strlen(E2P_ACCESS_EPA) + 1);
	if (*ptr != '0') {
		pr_info("%s: ePA mode: %c\n", __func__, *ptr);
		g_card->pa_setting = 1;
	} else {
		pr_info("%s: iPA mode: %c\n", __func__, *ptr);
		g_card->pa_setting = 0;
	}

	p = ptr = strstr(p_buf, E2P_ACCESS_DUPLEX);
	if (!ptr) {
		pr_info("%s: Can't find %s\n", __func__, E2P_ACCESS_DUPLEX);
		g_card->duplex_setting = -1;
		goto out;
	}
	if (p > p_buf) {
		p--;
		while ((*p == ' ') && (p != p_buf))
			p--;
		if (*p == '#') {
			pr_info("%s: It's no duplex setting\n", __func__);
			g_card->duplex_setting = -1;
			goto out;
		}
	}
	/* check access mode */
	ptr += (strlen(E2P_ACCESS_DUPLEX) + 1);
	if (*ptr != '0') {
		pr_info("%s: TDD mode: %c\n", __func__, *ptr);
		g_card->duplex_setting = 1;
	} else {
		pr_info("%s: FDD mode: %c\n", __func__, *ptr);
		g_card->duplex_setting = 0;
	}

out:
	kfree(p_buf);
}

void btmtk_set_keep_full_pwr(uint8_t *buf, size_t buf_size)
{
	char *p_buf = NULL;
	char *ptr = NULL;

	g_card->is_KeepFullPwr = false;

	if (!buf) {
		pr_info("%s: buf is null\n", __func__);
		return;
	} else if (buf_size < (strlen(KEEP_FULL_PWR) + 2)) {
		pr_info("%s: incorrect buf size(%d)\n",
			__func__, (int)buf_size);
		return;
	}

	p_buf = kmalloc(buf_size + 1, GFP_KERNEL);
	if (!p_buf)
		return;

	memcpy(p_buf, buf, buf_size);
	p_buf[buf_size] = '\0';

	/* find string */
	ptr = strstr(p_buf, KEEP_FULL_PWR);
	if (!ptr) {
		pr_cont("%s: Can't find %s\n", __func__, KEEP_FULL_PWR);
		goto out;
	}

	/* check always driver own */
	ptr += (strlen(KEEP_FULL_PWR) + 1);

	if (*ptr == PWR_KEEP_NO_FW_OWN) {
		/* always driver own is set*/
		pr_cont("%s: Read KeepFullPwr on: %c\n", __func__, *ptr);
		g_card->is_KeepFullPwr = true;
	} else if (*ptr == PWR_SWITCH_DRIVER_FW_OWN) {
		/* always driver own is not set */
		pr_cont("%s: Read KeepFullPwr off: %c\n", __func__, *ptr);
	} else {
		pr_info("%s: It's not the correct own setting: %c\n", __func__, *ptr);
	}

out:
	kfree(p_buf);
}

static void btmtk_set_power_limit(uint8_t *buf,
						size_t buf_size,
						bool is7668)
{
	int ret = -1;
	uint8_t set_radio[] = {0x79, 0xFC, 0x08,
		0x07, 0x80, 0x00, 0x06, 0x07, 0x07, 0x00, 0x00};
	uint8_t set_radio_e[] = {0x0E, 0x04, 0x01,
		0x79, 0xFC, 0x00};
	uint16_t offset = 0;

	if (!buf) {
		pr_info("%s: buf is null\n", __func__);
		return;
	} else if (is7668 == false) {
		pr_info("%s: only support mt7668 right now\n",
			__func__);
		return;
	}

	/* radio setting - BT power */
	if (is7668) {
		/* BT default power */
		set_radio[3] = (*(buf + offset));
		/* BLE default power */
		set_radio[7] = (*(buf + offset + 1));
		/* TX MAX power */
		set_radio[8] = (*(buf + offset + 2));
		/* TX power sub level */
		set_radio[9] = 0;
		/* BR/EDR power diff mode */
		set_radio[10] = (*(buf + offset + 3));
	}

	ret = btmtk_sdio_send_hci_cmd(HCI_COMMAND_PKT, set_radio, sizeof(set_radio),
		set_radio_e, sizeof(set_radio_e), COMP_EVENT_TIMO, 1);
	pr_notice("%s: set radio(BT/BLE default power: %d/%d MAX power: %d) %s\n",
		__func__,
		set_radio[3], set_radio[7], set_radio[8],
		ret < 0 ? "fail" : "OK");

}

static void btmtk_requset_power_limit_callback(const struct firmware *pwr_fw, void *context)
{
	struct btmtk_sdio_card *card = (struct btmtk_sdio_card *)context;

	pr_notice("%s\n", __func__);
	if (pwr_fw != NULL) {
		/* set parameters to controller */
		btmtk_set_power_limit((uint8_t *)pwr_fw->data,
			pwr_fw->size,
			(card->func->device == 0x7668 ? true : false));
		release_firmware(pwr_fw);
	}
}

static void btmtk_eeprom_bin_file(struct btmtk_sdio_card *card)
{
	char *cfg_file = NULL;
	char bin_file[32];

	const struct firmware *cfg_fw = NULL;
	const struct firmware *bin_fw = NULL;

	int ret = -1;
	int chipid = card->func->device;

	pr_notice("%s: %X series\n", __func__, chipid);
	cfg_file = E2P_ACCESS_MODE_SWITCHER;
	sprintf(bin_file, E2P_BIN_FILE, chipid);

	usleep_range(10*1000, 15*1000);

	/* request configure file */
	ret = request_firmware(&cfg_fw, cfg_file, &card->func->dev);
	if (ret < 0) {
		if (ret == -ENOENT)
			pr_notice("%s: Configure file not found, ignore EEPROM bin file\n",
				__func__);
		else
			pr_notice("%s: request configure file fail(%d)\n",
				__func__, ret);
		return;
	}

	if (cfg_fw) {
		btmtk_set_pa_and_duplex((uint8_t *)cfg_fw->data, cfg_fw->size);
		btmtk_set_keep_full_pwr((uint8_t *)cfg_fw->data, cfg_fw->size);
	} else {
		pr_info("%s: cfg_fw is null\n", __func__);
		return;
	}

	if (btmtk_is_bin_file_mode((uint8_t *)cfg_fw->data, cfg_fw->size) == false) {
		if (card->bin_file_buffer != NULL) {
			kfree(card->bin_file_buffer);
			card->bin_file_buffer = NULL;
			card->bin_file_size = 0;
		}
		goto exit2;
	}

	usleep_range(10*1000, 15*1000);

	/* open bin file for EEPROM */
	ret = request_firmware(&bin_fw, bin_file, &card->func->dev);
	if (ret < 0) {
		pr_notice("%s: request bin file fail(%d)\n",
			__func__, ret);
		goto exit2;
	}

	card->bin_file_buffer = kmalloc(bin_fw->size, GFP_KERNEL);
	memcpy(card->bin_file_buffer, bin_fw->data, bin_fw->size);
	card->bin_file_size = bin_fw->size;

exit2:
	/* open power limit */
	ret = request_firmware_nowait(THIS_MODULE, true, TX_PWR_LIMIT,
		&card->func->dev, GFP_KERNEL, g_card, btmtk_requset_power_limit_callback);
	if (ret < 0)
		pr_notice("%s: request power limit file fail(%d)\n",
			__func__, ret);

	if (cfg_fw)
		release_firmware(cfg_fw);
	if (bin_fw)
		release_firmware(bin_fw);
}

/* 1:on ,  0:off */
static int btmtk_sdio_set_sleep(void)
{
	int ret = 0;
	u8 event[] = {0x0E, 0x04, 0x01, 0x7A, 0xFC, 0x00};
	u8 cmd[] = {0x7A, 0xFC, 0x07,
		/*3:sdio, 5:usb*/0x03,
		/*host non sleep duration*/0x80, 0x02,
		/*host non sleep duration*/0x80, 0x02, 0x00, 0x00};
	pr_info("%s begin\n", __func__);

	ret = btmtk_sdio_send_hci_cmd(HCI_COMMAND_PKT, cmd, sizeof(cmd),
		event, sizeof(event), COMP_EVENT_TIMO, 1);

	return ret;
}

static int btmtk_sdio_skb_enq_fwlog(void *src, u32 len, u8 type, struct sk_buff_head *queue)
{
	struct sk_buff *skb_tmp = NULL;
	int retry = 10;

	do {
		/* If we need hci type, len + 1 */
		skb_tmp = alloc_skb(type ? len + 1 : len, GFP_ATOMIC);
		if (skb_tmp != NULL)
			break;
		else if (retry <= 0) {
			pr_info("%s: alloc_skb return 0, error", __func__);
			return -ENOMEM;
		}
		pr_info("%s: alloc_skb return 0, error, retry = %d", __func__, retry);
	} while (retry-- > 0);

	if (type) {
		memcpy(&skb_tmp->data[0], &type, 1);
		memcpy(&skb_tmp->data[1], src, len);
		skb_tmp->len = len + 1;
	} else {
		memcpy(skb_tmp->data, src, len);
		skb_tmp->len = len;
	}

	LOCK_UNSLEEPABLE_LOCK(&(fwlog_metabuffer.spin_lock));
	skb_queue_tail(queue, skb_tmp);
	UNLOCK_UNSLEEPABLE_LOCK(&(fwlog_metabuffer.spin_lock));
	return 0;
}

static int btmtk_sdio_dispatch_fwlog(u8 *buf, int len)
{
	static u8 fwlog_picus_blocking_warn;
	static u8 fwlog_fwdump_blocking_warn;
	int ret = 0;

	if ((buf[0] == 0xFF && buf[2] == 0x50) ||
		(buf[0] == 0xFF && buf[1] == 0x05)) {
		if (skb_queue_len(&g_priv->adapter->fwlog_fops_queue) < FWLOG_QUEUE_COUNT) {
			pr_debug("%s : This is picus data", __func__);
			if (btmtk_sdio_skb_enq_fwlog(buf, len, 0, &g_priv->adapter->fwlog_fops_queue) == 0)
				wake_up_interruptible(&fw_log_inq);

			fwlog_picus_blocking_warn = 0;
		} else {
			if (fwlog_picus_blocking_warn == 0) {
				fwlog_picus_blocking_warn = 1;
				pr_info("%s fwlog queue size is full(picus)\n", __func__);
			}
		}
	} else if (buf[0] == 0x6f && buf[1] == 0xfc) {
		/* Coredump */
		if (skb_queue_len(&g_priv->adapter->fwlog_fops_queue) < FWLOG_ASSERT_QUEUE_COUNT) {
			pr_debug("%s : Receive coredump, move data to fwlogqueue for picus", __func__);
			if (btmtk_sdio_skb_enq_fwlog(buf, len, 0, &g_priv->adapter->fwlog_fops_queue) == 0)
				wake_up_interruptible(&fw_log_inq);

			fwlog_fwdump_blocking_warn = 0;
		} else {
			if (fwlog_fwdump_blocking_warn == 0) {
				fwlog_fwdump_blocking_warn = 1;
				pr_info("%s fwlog queue size is full(coredump)\n", __func__);
			}
		}
	}
	return ret;
}

static int btmtk_usb_dispatch_data_bluetooth_kpi(u8 *buf, int len, u8 type)
{
	static u8 fwlog_blocking_warn;
	int ret = 0;

	if (btmtk_bluetooth_kpi &&
		skb_queue_len(&g_priv->adapter->fwlog_fops_queue) < FWLOG_BLUETOOTH_KPI_QUEUE_COUNT) {
		/* sent event to queue, picus tool will log it for bluetooth KPI feature */
		if (btmtk_sdio_skb_enq_fwlog(buf, len, type, &g_priv->adapter->fwlog_fops_queue) == 0) {
			wake_up_interruptible(&fw_log_inq);
			fwlog_blocking_warn = 0;
		}
	} else {
		if (fwlog_blocking_warn == 0) {
			fwlog_blocking_warn = 1;
			pr_info("%s fwlog queue size is full(bluetooth_kpi)\n", __func__);
		}
	}
	return ret;
}

static int btmtk_sdio_host_to_card(struct btmtk_private *priv,
				u8 *payload, u16 nb)
{
	struct btmtk_sdio_card *card = priv->btmtk_dev.card;
	int ret = 0;
	int i = 0;
	u8 MultiBluckCount = 0;
	u8 redundant = 0;
	int len = 0;

	if (payload != txbuf) {
		memset(txbuf, 0, MTK_TXDATA_SIZE);
		memcpy(txbuf, payload, nb);
	}

	if (!card || !card->func) {
		pr_info("card or function is NULL!\n");
		return -EINVAL;
	}

	len = nb - MTK_SDIO_PACKET_HEADER_SIZE;

	btmtk_usb_dispatch_data_bluetooth_kpi(&txbuf[MTK_SDIO_PACKET_HEADER_SIZE], len, 0);

	MultiBluckCount = nb/SDIO_BLOCK_SIZE;
	redundant = nb % SDIO_BLOCK_SIZE;

	if (redundant)
		nb = (MultiBluckCount+1)*SDIO_BLOCK_SIZE;

	if (nb < 16)
		btmtk_print_buffer_conent(txbuf, nb);
	else
		btmtk_print_buffer_conent(txbuf, 16);

	do {
		/* Transfer data to card */
		ret = btmtk_sdio_writesb(CTDR, txbuf, nb);
		if (ret < 0) {
			i++;
			pr_info("i=%d writesb failed: %d\n", i, ret);
			ret = -EIO;
			if (i > MAX_WRITE_IOMEM_RETRY)
				goto exit;
		}
	} while (ret);

	priv->btmtk_dev.tx_dnld_rdy = false;

exit:

	return ret;
}

static int btmtk_sdio_set_audio_slave(void)
{
	int ret = 0;
	u8 *cmd = NULL;
	u8 event[] = { 0x0E, 0x04, 0x01, 0x72, 0xFC, 0x00 };
#ifdef MTK_CHIP_PCM /* For PCM setting */
	u8 cmd_pcm[] = { 0x72, 0xFC, 0x04, 0x03, 0x10, 0x00, 0x4A };
#else /* For I2S setting */
#if SUPPORT_MT7668
	u8 cmd_7668[] = { 0x72, 0xFC, 0x04, 0x03, 0x10, 0x00, 0x02 };
#endif
#if SUPPORT_MT7663
	u8 cmd_7663[] = { 0x72, 0xFC, 0x04, 0x49, 0x00, 0x80, 0x00 };
#endif
#endif

	pr_info("%s\n", __func__);
#ifdef MTK_CHIP_PCM
	cmd = cmd_pcm;
#else
#if SUPPORT_MT7668
	if (is_mt7668(g_card))
		cmd = cmd_7668;
#endif
#if SUPPORT_MT7663
	if (is_mt7663(g_card))
		cmd = cmd_7663;
#endif
#endif
	if (!cmd) {
		pr_info("%s not supported\n", __func__);
		return 0;
	}

	ret = btmtk_sdio_send_hci_cmd(HCI_COMMAND_PKT, cmd, cmd[2] + 3,
		event, sizeof(event), COMP_EVENT_TIMO, 1);

	return ret;
}

static int btmtk_sdio_read_pin_mux_setting(u32 *value)
{
	int ret = 0;
	u8 cmd[] = { 0xD1, 0xFC, 0x04, 0x54, 0x30, 0x02, 0x81 };
	u8 event[] = { 0x0E, 0x08, 0x01, 0xD1, 0xFC };

	pr_info("%s\n", __func__);

	ret = btmtk_sdio_send_hci_cmd(HCI_COMMAND_PKT, cmd, sizeof(cmd),
		event, sizeof(event), COMP_EVENT_TIMO, 1);

	if (ret)
		return ret;

	*value = (rxbuf[14] << 24) + (rxbuf[13] << 16) + (rxbuf[12] << 8) + rxbuf[11];
	pr_debug("%s value=%08x\n", __func__, *value);
	return ret;
}

static int btmtk_sdio_write_pin_mux_setting(u32 value)
{
	int ret = 0;
	u8 *cmd = NULL;
	u8 event[] = {0x0E, 0x04, 0x01, 0xD0, 0xFC, 0x00};
	u8 cmd_7668[] = {0xD0, 0xFC, 0x08,
		0x54, 0x30, 0x02, 0x81, 0x00, 0x00, 0x00, 0x00};
	u8 cmd_7663[] = {0xD0, 0xFC, 0x08,
		0x54, 0x50, 0x00, 0x78, 0x00, 0x10, 0x11, 0x01};

#if SUPPORT_MT7668
	if (is_mt7668(g_card))
		cmd = cmd_7668;
#endif
#if SUPPORT_MT7663
	if (is_mt7663(g_card))
		cmd = cmd_7663;
#endif
	if (!cmd) {
		pr_info("%s not supported\n", __func__);
		return 0;
	}

	pr_info("%s begin, value = 0x%08x\n", __func__, value);

	cmd[7] = (value & 0x000000FF);
	cmd[8] = ((value & 0x0000FF00) >> 8);
	cmd[9] = ((value & 0x00FF0000) >> 16);
	cmd[10] = ((value & 0xFF000000) >> 24);

	ret = btmtk_sdio_send_hci_cmd(HCI_COMMAND_PKT, cmd, cmd[2] + 3,
		event, sizeof(event), COMP_EVENT_TIMO, 1);

	return ret;
}

static int btmtk_sdio_set_audio_pin_mux(void)
{
	int ret = 0;
	u32 pinmux = 0;

	ret = btmtk_sdio_read_pin_mux_setting(&pinmux);
	if (ret) {
		pr_info("btmtk_sdio_read_pin_mux_setting error(%d)\n", ret);
		return ret;
	}

#if SUPPORT_MT7668
	if (is_mt7668(g_card)) {
		pinmux &= 0x0000FFFF;
		pinmux |= 0x22220000;
	}
#endif
#if SUPPORT_MT7663
	if (is_mt7663(g_card)) {
		pinmux &= 0xF0000FFF;
		pinmux |= 0x01111000;
	}
#endif
	ret = btmtk_sdio_write_pin_mux_setting(pinmux);

	if (ret) {
		pr_info("btmtk_sdio_write_pin_mux_setting error(%d)\n", ret);
		return ret;
	}

	pinmux = 0;
	ret = btmtk_sdio_read_pin_mux_setting(&pinmux);
	if (ret) {
		pr_info("btmtk_sdio_read_pin_mux_setting error(%d)\n", ret);
		return ret;
	}
	pr_info("confirm pinmux %04x\n", pinmux);

	return ret;
}

static int btmtk_send_rom_patch(u8 *fwbuf, u32 fwlen, int mode)
{
	int ret = 0;
	u8 mtksdio_packet_header[MTK_SDIO_PACKET_HEADER_SIZE] = {0};
	int stp_len = 0;
	u8 mtkdata_header[MTKDATA_HEADER_SIZE] = {0};

	int copy_len = 0;
	int Datalen = fwlen;
	u32 u32ReadCRValue = 0;


	pr_debug("%s fwlen %d, mode = %d\n", __func__, fwlen, mode);
	if (fwlen < Datalen) {
		pr_info("%s file size = %d,is not corect\n", __func__, fwlen);
		return -ENOENT;
	}

	stp_len = Datalen + MTKDATA_HEADER_SIZE;


	mtkdata_header[0] = 0x2;/*ACL data*/
	mtkdata_header[1] = 0x6F;
	mtkdata_header[2] = 0xFC;

	mtkdata_header[3] = ((Datalen+4+1)&0x00FF);
	mtkdata_header[4] = ((Datalen+4+1)&0xFF00)>>8;

	mtkdata_header[5] = 0x1;
	mtkdata_header[6] = 0x1;

	mtkdata_header[7] = ((Datalen+1)&0x00FF);
	mtkdata_header[8] = ((Datalen+1)&0xFF00)>>8;

	mtkdata_header[9] = mode;

/* 0 and 1 is packet length, include MTKSTP_HEADER_SIZE */
	mtksdio_packet_header[0] =
		(Datalen+4+MTKSTP_HEADER_SIZE+6)&0xFF;
	mtksdio_packet_header[1] =
		((Datalen+4+MTKSTP_HEADER_SIZE+6)&0xFF00)>>8;
	mtksdio_packet_header[2] = 0;
	mtksdio_packet_header[3] = 0;

/*
 * mtksdio_packet_header[2] and mtksdio_packet_header[3]
 * are reserved
 */
	pr_debug("%s result %02x  %02x\n", __func__,
		((Datalen+4+MTKSTP_HEADER_SIZE+6)&0xFF00)>>8,
		(Datalen+4+MTKSTP_HEADER_SIZE+6));

	memcpy(txbuf+copy_len, mtksdio_packet_header,
		MTK_SDIO_PACKET_HEADER_SIZE);
	copy_len += MTK_SDIO_PACKET_HEADER_SIZE;

	memcpy(txbuf+copy_len, mtkdata_header, MTKDATA_HEADER_SIZE);
	copy_len += MTKDATA_HEADER_SIZE;

	memcpy(txbuf+copy_len, fwbuf, Datalen);
	copy_len += Datalen;

	pr_debug("%s txbuf %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
		__func__,
		txbuf[0], txbuf[1], txbuf[2], txbuf[3], txbuf[4],
		txbuf[5], txbuf[6], txbuf[7], txbuf[8], txbuf[9]);


	ret = btmtk_sdio_readl(CHIER, &u32ReadCRValue);
	pr_debug("%s: CHIER u32ReadCRValue %x, ret %d\n",
		__func__, u32ReadCRValue, ret);

	ret = btmtk_sdio_readl(CHLPCR, &u32ReadCRValue);
	pr_debug("%s: CHLPCR u32ReadCRValue %x, ret %d\n",
		__func__, u32ReadCRValue, ret);

	ret = btmtk_sdio_readl(CHISR, &u32ReadCRValue);
	pr_debug("%s: 0CHISR u32ReadCRValue %x, ret %d\n",
		__func__, u32ReadCRValue, ret);
	ret = btmtk_sdio_readl(CHISR, &u32ReadCRValue);
	pr_debug("%s: 00CHISR u32ReadCRValue %x, ret %d\n",
		__func__, u32ReadCRValue, ret);

	btmtk_sdio_send_tx_data(txbuf, copy_len);

	ret = btmtk_sdio_recv_rx_data();

	return ret;
}



/*
 * type: cmd:1, ACL:2
 * -------------------------------------------------
 * mtksdio hedaer 4 byte| wmt header  |
 *
 *
 * data len should less than 512-4-4
 */
static int btmtk_sdio_send_wohci(u8 type, u32 len, u8 *data)
{
	u32 ret = 0;
	u32 push_in_data_len = 0;
	u32 MultiBluckCount = 0;
	u32 redundant = 0;
	u8 mtk_wmt_header[MTKWMT_HEADER_SIZE] = {0};
	u8 mtksdio_packet_header[MTK_SDIO_PACKET_HEADER_SIZE] = {0};
	u8 mtk_tx_data[512] = {0};

	mtk_wmt_header[0] = type;
	mtk_wmt_header[1] = 0x6F;
	mtk_wmt_header[2] = 0xFC;
	mtk_wmt_header[3] = len;

	mtksdio_packet_header[0] =
		(len+MTKWMT_HEADER_SIZE+MTK_SDIO_PACKET_HEADER_SIZE)&0xFF;
	mtksdio_packet_header[1] =
		((len+MTKWMT_HEADER_SIZE+MTK_SDIO_PACKET_HEADER_SIZE)&0xFF00)
		>>8;
	mtksdio_packet_header[2] = 0;
	mtksdio_packet_header[3] = 0;
/*
 * mtksdio_packet_header[2] and mtksdio_packet_header[3]
 * are reserved
 */

	memcpy(mtk_tx_data, mtksdio_packet_header,
		sizeof(mtksdio_packet_header));
	push_in_data_len += sizeof(mtksdio_packet_header);

	memcpy(mtk_tx_data+push_in_data_len, mtk_wmt_header,
		sizeof(mtk_wmt_header));
	push_in_data_len += sizeof(mtk_wmt_header);

	memcpy(mtk_tx_data+push_in_data_len, data, len);
	push_in_data_len += len;
	memcpy(txbuf, mtk_tx_data, push_in_data_len);

	MultiBluckCount = push_in_data_len/4;
	redundant = push_in_data_len % 4;
	if (redundant)
		push_in_data_len = (MultiBluckCount+1)*4;

	ret = btmtk_sdio_writesb(CTDR, txbuf, push_in_data_len);
	pr_info("%s return  0x%0x\n", __func__, ret);
	return ret;
}

/*
 * data event:
 * return
 * 0:
 * patch download is not complete/get patch semaphore fail
 * 1:
 * patch download is complete by others
 * 2
 * patch download is not coplete
 * 3:(for debug)
 * release patch semaphore success
 */
static int btmtk_sdio_need_load_rom_patch(void)
{
	u32 ret = -1;
	u8 cmd[] = {0x1, 0x17, 0x1, 0x0, 0x1};
	u8 event[] = {0x2, 0x17, 0x1, 0x0};

	do {
		ret = btmtk_sdio_send_wohci(HCI_COMMAND_PKT, sizeof(cmd), cmd);

		if (ret) {
			pr_info("%s btmtk_sdio_send_wohci return fail ret %d\n",
					__func__, ret);
			break;
		}

		ret = btmtk_sdio_recv_rx_data();
		if (ret)
			break;

		if (rx_length == 12) {
			if (memcmp(rxbuf+7, event, sizeof(event)) == 0)
				return rxbuf[11];

			pr_info("%s receive event content is not correct, print receive data\n",
				__func__);
			btmtk_print_buffer_conent(rxbuf, rx_length);
		}
	} while (0);
	pr_info("%s return ret %d\n", __func__, ret);
	return ret;
}
static int btmtk_sdio_set_write_clear(void)
{
	u32 u32ReadCRValue = 0;
	u32 ret = 0;

	ret = btmtk_sdio_readl(CHCR, &u32ReadCRValue);
	if (ret) {
		pr_info("%s read CHCR error\n", __func__);
		ret = EINVAL;
		return ret;
	}

	u32ReadCRValue |= 0x00000002;
	btmtk_sdio_writel(CHCR, u32ReadCRValue);
	pr_info("%s write CHCR 0x%08X\n", __func__, u32ReadCRValue);
	ret = btmtk_sdio_readl(CHCR, &u32ReadCRValue);
	pr_info("%s read CHCR 0x%08X\n", __func__, u32ReadCRValue);
	if (u32ReadCRValue&0x00000002)
		pr_info("%s write clear\n", __func__);
	else
		pr_info("%s read clear\n", __func__);

	return ret;
}

static int btmtk_sdio_set_audio(void)
{
	int ret = 0;

	ret = btmtk_sdio_set_audio_slave();
	if (ret) {
		pr_info("btmtk_sdio_set_audio_slave error(%d)\n", ret);
		return ret;
	}

	ret = btmtk_sdio_set_audio_pin_mux();
	if (ret) {
		pr_info("btmtk_sdio_set_audio_pin_mux error(%d)\n", ret);
		return ret;
	}

	return ret;
}

static int btmtk_sdio_send_audio_slave(void)
{
	int ret = 0;

	ret = btmtk_sdio_set_audio_slave();
	if (ret) {
		pr_info("btmtk_sdio_set_audio_slave error(%d)\n", ret);
		return ret;
	}

	if (is_mt7663(g_card)) {
		ret = btmtk_sdio_set_audio_pin_mux();
		if (ret) {
			pr_info("btmtk_sdio_set_audio_pin_mux error(%d)\n", ret);
			return ret;
		}
	}

	return ret;
}

static int btmtk_sdio_download_rom_patch(struct btmtk_sdio_card *card)
{
	const struct firmware *fw_firmware = NULL;
	int firmwarelen, ret = 0;
	u8 *fwbuf;
	struct _PATCH_HEADER *patchHdr;
	u8 *cDateTime = NULL;
	u16 u2HwVer = 0;
	u16 u2SwVer = 0;
	u32 u4PatchVer = 0;
	u32 u4FwVersion = 0;
	u32 u4ChipId = 0;
	u32 u32ReadCRValue = 0;
	u8  patch_status = 0;
	bool load_sysram3 = false;
	int retry = 20;

	ret = btmtk_sdio_set_own_back(DRIVER_OWN);
	if (ret)
		return ret;

	u4FwVersion = btmtk_sdio_bt_memRegister_read(FW_VERSION);
	pr_info("%s Fw Version 0x%x\n", __func__, u4FwVersion);
	u4ChipId = btmtk_sdio_bt_memRegister_read(CHIP_ID);
	pr_info("%s Chip Id 0x%x\n", __func__, u4ChipId);

	if ((u4FwVersion & 0xff) == 0xff) {
		pr_info("%s failed ! wrong fw version : 0x%x\n", __func__, u4FwVersion);
		return -1;

	} else {
		u8 uFirmwareName[MAX_BIN_FILE_NAME_LEN] = {0};

		/* Bin filename format : "mt$$$$_patch_e%.bin" */
		/*     $$$$ : chip id */
		/*     % : fw version + 1 (in HEX) */
		snprintf(uFirmwareName, MAX_BIN_FILE_NAME_LEN, "mt%04x_patch_e%x_hdr.bin",
				u4ChipId & 0xffff, (u4FwVersion & 0x0ff) + 1);
		pr_info("%s request_firmware(firmware name %s)\n",
				__func__, uFirmwareName);
		ret = request_firmware(&fw_firmware, uFirmwareName,
				&card->func->dev);

		if ((ret < 0) || !fw_firmware) {
			pr_info("request_firmware(firmware name %s) failed, error code = %d\n",
					uFirmwareName,
					ret);
			ret = -ENOENT;
			goto done;
		}
	}
	memset(fw_version_str, 0, FW_VERSION_BUF_SIZE);
	if ((fw_firmware->data[8] >= '0') && (fw_firmware->data[8] <= '9'))
		memcpy(fw_version_str, fw_firmware->data, FW_VERSION_SIZE - 1);
	else
		sprintf(fw_version_str, "%.4s-%.2s-%.2s.%.1s.%.2s.%.1s.%.1s.%.2s",
			fw_firmware->data, fw_firmware->data + 4, fw_firmware->data + 6,
			fw_firmware->data + 8, fw_firmware->data + 9,
			fw_firmware->data + 11, fw_firmware->data + 12,
			fw_firmware->data + 13);

#if SUPPORT_MT7668
	if (is_mt7668(g_card))
		load_sysram3 =
			(fw_firmware->size > (PATCH_INFO_SIZE + PATCH_LEN_ILM))
				? true : false;
#endif

	do {
		patch_status = btmtk_sdio_need_load_rom_patch();
		pr_debug("%s patch_status %d\n", __func__, patch_status);

		if (patch_status > PATCH_NEED_DOWNLOAD || patch_status < 0) {
			pr_info("%s patch_status error\n", __func__);
			return -1;
		} else if (patch_status == PATCH_READY) {
			pr_info("%s patch is ready no need load patch again\n",
					__func__);
			if (!load_sysram3)
				goto patch_end;
			else
				goto sysram3;
		} else if (patch_status == PATCH_IS_DOWNLOAD_BY_OTHER) {
			msleep(100);
			retry--;
		} else if (patch_status == PATCH_NEED_DOWNLOAD) {
			break;  /* Download ROM patch directly */
		}
	} while (retry > 0);

	if (patch_status == PATCH_IS_DOWNLOAD_BY_OTHER) {
		pr_info("%s Hold by another fun more than 2 seconds\n",
			__func__);
		return -1;
	}

	fwbuf = (u8 *)fw_firmware->data;

	/*Display rom patch info*/
	patchHdr =  (struct _PATCH_HEADER *)fwbuf;
	cDateTime = patchHdr->ucDateTime;
	u2HwVer = patchHdr->u2HwVer;
	u2SwVer = patchHdr->u2SwVer;
	u4PatchVer = patchHdr->u4PatchVer;

	pr_info("[btmtk] =============== Patch Info ==============\n");
	pr_info("[btmtk] Built Time = %s\n", cDateTime);
	pr_info("[btmtk] Hw Ver = 0x%x\n",
			((u2HwVer & 0x00ff) << 8) | ((u2HwVer & 0xff00) >> 8));
	pr_info("[btmtk] Sw Ver = 0x%x\n",
			((u2SwVer & 0x00ff) << 8) | ((u2SwVer & 0xff00) >> 8));
	pr_info("[btmtk] Patch Ver = 0x%04x\n",
			((u4PatchVer & 0xff000000) >> 24) |
			((u4PatchVer & 0x00ff0000) >> 16));

	pr_info("[btmtk] Platform = %c%c%c%c\n",
			patchHdr->ucPlatform[0],
			patchHdr->ucPlatform[1],
			patchHdr->ucPlatform[2],
			patchHdr->ucPlatform[3]);
	pr_info("[btmtk] Patch start addr = %02x\n", patchHdr->u2PatchStartAddr);
	pr_info("[btmtk] =========================================\n");


	firmwarelen = load_sysram3 ?
			PATCH_LEN_ILM :	(fw_firmware->size - PATCH_INFO_SIZE);

	pr_info("%s loading ILM rom patch...\n", __func__);
	ret = btmtk_sdio_download_partial_rom_patch(fwbuf, firmwarelen);
	pr_info("%s loading ILM rom patch... Done\n", __func__);

	if (btmtk_sdio_need_load_rom_patch() == PATCH_READY) {
		pr_info("%s patchdownload is done by BT\n", __func__);
	} else {
		/* TODO: Need error handling here*/
		pr_info("%s patchdownload download by BT, not ready\n",
			__func__);
	}

	/* CHIP_RESET, ROM patch would be reactivated.
	 * Currently, wmt reset is only for ILM rom patch, and there are also
	 * some preparations need to be done in FW for loading sysram3 patch...
	 */
	ret = btmtk_sdio_send_wmt_reset();
	if (ret)
		goto done;

sysram3:
	if (load_sysram3) {
		firmwarelen = fw_firmware->size - PATCH_INFO_SIZE
			- PATCH_LEN_ILM - PATCH_INFO_SIZE;
		fwbuf = (u8 *)fw_firmware->data + PATCH_INFO_SIZE
			+ PATCH_LEN_ILM;
		pr_info("%s loading sysram3 rom patch...\n", __func__);
		ret = btmtk_sdio_download_partial_rom_patch(fwbuf, firmwarelen);
		pr_info("%s loading sysram3 rom patch... Done\n", __func__);
	}

patch_end:
	ret = btmtk_sdio_readl(0, &u32ReadCRValue);
	pr_info("%s read chipid =  %x\n", __func__, u32ReadCRValue);

	/*Set interrupt output*/
	ret = btmtk_sdio_writel(CHIER, FIRMWARE_INT|TX_FIFO_OVERFLOW |
			FW_INT_IND_INDICATOR | TX_COMPLETE_COUNT |
			TX_UNDER_THOLD | TX_EMPTY | RX_DONE);
	if (ret) {
		pr_info("Set interrupt output fail(%d)\n", ret);
		ret = -EIO;
		goto done;
	}

	/*enable interrupt output*/
	ret = btmtk_sdio_writel(CHLPCR, C_FW_INT_EN_SET);
	if (ret) {
		pr_info("enable interrupt output fail(%d)\n", ret);
		ret = -EIO;
		goto done;
	}

	btmtk_sdio_set_write_clear();

done:

	release_firmware(fw_firmware);

	if (!ret)
		pr_info("%s success\n", __func__);
	else
		pr_info("%s fail\n", __func__);

	return ret;
}

static int btmtk_sdio_download_partial_rom_patch(u8 *fwbuf, int firmwarelen)
{
	int ret = 0;
	int RedundantSize = 0;
	u32 bufferOffset = 0;

	pr_info("Downloading FW image (%d bytes)\n", firmwarelen);

	fwbuf += PATCH_INFO_SIZE;
	pr_debug("%s PATCH_HEADER size %d\n",
			__func__, PATCH_INFO_SIZE);

	RedundantSize = firmwarelen;
	pr_debug("%s firmwarelen %d\n", __func__, firmwarelen);

	do {
		bufferOffset = firmwarelen - RedundantSize;

		if (RedundantSize == firmwarelen &&
				RedundantSize >= PATCH_DOWNLOAD_SIZE)
			ret = btmtk_send_rom_patch(fwbuf + bufferOffset,
					PATCH_DOWNLOAD_SIZE,
					SDIO_PATCH_DOWNLOAD_FIRST);
		else if (RedundantSize == firmwarelen)
			ret = btmtk_send_rom_patch(fwbuf + bufferOffset,
					RedundantSize,
					SDIO_PATCH_DOWNLOAD_FIRST);
		else if (RedundantSize < PATCH_DOWNLOAD_SIZE) {
			ret = btmtk_send_rom_patch(fwbuf + bufferOffset,
					RedundantSize,
					SDIO_PATCH_DOWNLOAD_END);
			pr_debug("%s patch downoad last patch part\n",
					__func__);
		} else
			ret = btmtk_send_rom_patch(fwbuf + bufferOffset,
					PATCH_DOWNLOAD_SIZE,
					SDIO_PATCH_DOWNLOAD_CON);

		RedundantSize -= PATCH_DOWNLOAD_SIZE;

		if (ret) {
			pr_info("%s btmtk_send_rom_patch fail\n", __func__);
			return ret;
		}
		pr_debug("%s RedundantSize %d\n", __func__, RedundantSize);
		if (RedundantSize <= 0) {
			pr_debug("%s patch downoad finish\n", __func__);
			break;
		}
	} while (1);

	return ret;
}

static void btmtk_sdio_close_coredump_file(void)
{
	pr_debug("%s  vfs_fsync\n", __func__);

	if (g_card->bt_cfg.save_fw_dump_in_kernel && fw_dump_file)
		vfs_fsync(fw_dump_file, 0);

	if (fw_dump_file) {
		pr_info("%s: close file  %s\n",
			__func__, g_card->bt_cfg.fw_dump_file_name);
		if (g_card->bt_cfg.save_fw_dump_in_kernel)
			filp_close(fw_dump_file, NULL);
		fw_dump_file = NULL;
	} else {
		pr_info("%s: fw_dump_file is NULL can't close file %s\n",
			__func__, g_card->bt_cfg.fw_dump_file_name);
	}
}

static void btmtk_sdio_stop_wait_dump_complete_thread(void)
{
	if (IS_ERR(wait_dump_complete_tsk) || wait_dump_complete_tsk == NULL)
		pr_info("%s wait_dump_complete_tsk is error", __func__);
	else {
		kthread_stop(wait_dump_complete_tsk);
		wait_dump_complete_tsk = NULL;
	}
}

static int btmtk_sdio_card_to_host(struct btmtk_private *priv, const u8 *event, const int event_len,
	int add_spec_header)
/*event: check event which want to compare*/
/*return value: -x fail, 0 success*/
{
	u16 buf_len = 0;
	int ret = 0;
	struct sk_buff *skb = NULL;
	struct sk_buff *fops_skb = NULL;
	u32 type;
	u32 fourbalignment_len = 0;
	u32 dump_len = 0;
	char *core_dump_end = NULL;
	int i = 0;
	u16 retry = 0;
	u32 u32ReadCRValue = 0;
	u8 is_picus_or_dump = 0;
	static u8 picus_blocking_warn;
	static u8 fwdump_blocking_warn;

	if (rx_length > (MTK_SDIO_PACKET_HEADER_SIZE + 1)) {
		buf_len = rx_length - (MTK_SDIO_PACKET_HEADER_SIZE + 1);
	} else {
		pr_info("%s, rx_length error(%d)\n", __func__, rx_length);
		return -EINVAL;
	}

	if (rx_length > (SDIO_HEADER_LEN + 8) && rxbuf[SDIO_HEADER_LEN] == 0x80 &&
		rxbuf[SDIO_HEADER_LEN + 5] == 0x6F && rxbuf[SDIO_HEADER_LEN + 6] == 0xFC) {
		dump_len = (rxbuf[SDIO_HEADER_LEN + 1] & 0x0F) * 256
				+ rxbuf[SDIO_HEADER_LEN + 2];
		pr_debug("%s: get dump length %d\n", __func__, dump_len);

		dump_data_counter++;
		dump_data_length += dump_len;
		is_picus_or_dump = 1;

		if (dump_data_counter % 1000 == 0)
			pr_info("%s: coredump on-going, total_packet = %d, total_length = %d\n",
					__func__,
					dump_data_counter, dump_data_length);

		if (dump_data_counter < PRINT_DUMP_COUNT) {
			pr_info("%s: dump %d %s\n",
				__func__,
				dump_data_counter,
				&rxbuf[SDIO_HEADER_LEN + 9]);
		/* release mode do reset dongle if print dump finish */
		} else if (!g_card->bt_cfg.support_full_fw_dump &&
			dump_data_counter == PRINT_DUMP_COUNT) {
			/* create dump file fail and is user mode */
			pr_info("%s: user mode, do reset after print dump done %d\n",
				__func__, dump_data_counter);
			picus_blocking_warn = 0;
			fwdump_blocking_warn = 0;
			btmtk_sdio_close_coredump_file();
			btmtk_sdio_stop_wait_dump_complete_thread();
			goto exit;
		}

		if (dump_data_counter == 1) {
			btmtk_sdio_hci_snoop_print();
			pr_info("%s: create btmtk_sdio_wait_dump_complete_thread\n", __func__);
			wait_dump_complete_tsk = kthread_run(btmtk_sdio_wait_dump_complete_thread,
				NULL, "btmtk_sdio_wait_dump_complete_thread");

			msleep(100);
			if (!wait_dump_complete_tsk)
				pr_info("%s: wait_dump_complete_tsk is NULL\n", __func__);

			btmtk_sdio_notify_wlan_remove_start();
			btmtk_sdio_set_no_fw_own(g_priv, TRUE);

			if (g_card->bt_cfg.save_fw_dump_in_kernel) {
				pr_info("%s : open file %s\n", __func__,
					g_card->bt_cfg.fw_dump_file_name);
				fw_dump_file = filp_open(g_card->bt_cfg.fw_dump_file_name, O_RDWR | O_CREAT, 0644);

				if (!(IS_ERR(fw_dump_file))) {
					pr_info("%s : open file %s success\n", __func__,
						g_card->bt_cfg.fw_dump_file_name);
				} else {
					pr_info("%s : open file %s fail\n", __func__,
						g_card->bt_cfg.fw_dump_file_name);
					fw_dump_file = NULL;
				}

				if (fw_dump_file && fw_dump_file->f_op == NULL) {
					pr_info("%s : %s fw_dump_file->f_op is NULL, close\n",
						g_card->bt_cfg.fw_dump_file_name, __func__);
					filp_close(fw_dump_file, NULL);
					fw_dump_file = NULL;
				}

				if (fw_dump_file && fw_dump_file->f_op->write == NULL) {
					pr_info("%s : %s fw_dump_file->f_op->write is NULL, close\n",
						g_card->bt_cfg.fw_dump_file_name, __func__);
					filp_close(fw_dump_file, NULL);
					fw_dump_file = NULL;
				}
			}
		}

		if (g_card->bt_cfg.save_fw_dump_in_kernel && (dump_len > 0)
			&& fw_dump_file && fw_dump_file->f_op && fw_dump_file->f_op->write)
			fw_dump_file->f_op->write(fw_dump_file, &rxbuf[SDIO_HEADER_LEN + 9],
				dump_len, &fw_dump_file->f_pos);

		if (skb_queue_len(&g_priv->adapter->fwlog_fops_queue) < FWLOG_ASSERT_QUEUE_COUNT) {
			/* This is coredump data, save coredump data to picus_queue */
			pr_debug("%s: Receive coredump data, move data to fwlog queue for picus",
				__func__);
			btmtk_sdio_dispatch_fwlog(&rxbuf[MTK_SDIO_PACKET_HEADER_SIZE + 5], dump_len + 3);
			fwdump_blocking_warn = 0;
		} else if (fwdump_blocking_warn == 0) {
			fwdump_blocking_warn = 1;
			pr_info("btmtk_sdio FW dump queue size is full");
		}

		/* Modify header to ACL format, handle is 0xFFF0
		 * Core dump header:
		 * 80 AA BB CC DD 6F FC XX XX XX ......
		 * 80	-> 02 (ACL TYPE)
		 * AA BB -> FF F0
		 * CC DD -> Data length
		 */
		rxbuf[SDIO_HEADER_LEN] = HCI_ACLDATA_PKT;
		rxbuf[SDIO_HEADER_LEN + 3] = (buf_len - 4) % 256;
		rxbuf[SDIO_HEADER_LEN + 4] = (buf_len - 4) / 256;
		rxbuf[SDIO_HEADER_LEN + 1] = 0xFF;
		rxbuf[SDIO_HEADER_LEN + 2] = 0xF0;

		if (dump_len >= strlen(FW_DUMP_END_EVENT)) {
			core_dump_end = strstr(&rxbuf[SDIO_HEADER_LEN + 10],
					FW_DUMP_END_EVENT);

			if (core_dump_end) {
				pr_info("%s: core_dump_end %s, total_packet = %d, total_length = %d, rxbuf = %02x %02x %02x\n",
					__func__, core_dump_end,
					dump_data_counter, dump_data_length,
					rxbuf[4], rxbuf[5], rxbuf[6]);
				sdio_claim_host(g_card->func);
				sdio_release_irq(g_card->func);
				sdio_release_host(g_card->func);
				dump_data_counter = 0;
				dump_data_length = 0;
				picus_blocking_warn = 0;
				fwdump_blocking_warn = 0;
				btmtk_sdio_close_coredump_file();
				btmtk_sdio_stop_wait_dump_complete_thread();
			}
		}
	} else if (rx_length > (SDIO_HEADER_LEN + 4) &&
			((rxbuf[SDIO_HEADER_LEN] == 0x04 &&
			  rxbuf[SDIO_HEADER_LEN + 1] == 0xFF &&
			  rxbuf[SDIO_HEADER_LEN + 3] == 0x50) ||
			(rxbuf[SDIO_HEADER_LEN] == 0x02 &&
			 rxbuf[SDIO_HEADER_LEN + 1] == 0xFF &&
			 rxbuf[SDIO_HEADER_LEN + 2] == 0x05))) {
		is_picus_or_dump = 1;
		 /*receive picus data to fwlog_queue*/
		if (rxbuf[SDIO_HEADER_LEN] == 0x04) {
			dump_len = rxbuf[SDIO_HEADER_LEN + 2] - 1;
			buf_len = dump_len + 3;
		} else {
			dump_len = (rxbuf[SDIO_HEADER_LEN + 4] & 0x0F) * 256 + rxbuf[SDIO_HEADER_LEN + 3];
			buf_len = dump_len + 4;
		}
		pr_debug("%s This is debug log data, length = %d", __func__, dump_len);
		if (rx_length < (buf_len + MTK_SDIO_PACKET_HEADER_SIZE + 1))
			goto data_err;
		btmtk_sdio_dispatch_fwlog(&rxbuf[MTK_SDIO_PACKET_HEADER_SIZE + 1], buf_len);
		goto exit;
	} else if (rxbuf[SDIO_HEADER_LEN] == 0x04
			&& rxbuf[SDIO_HEADER_LEN + 1] == 0x0E
			&& rxbuf[SDIO_HEADER_LEN + 2] == 0x04
			&& rxbuf[SDIO_HEADER_LEN + 3] == 0x01
			&& rxbuf[SDIO_HEADER_LEN + 4] == 0x02
			&& rxbuf[SDIO_HEADER_LEN + 5] == 0xFD) {
		pr_info("%s: This is btclk event, status:%02x\n",
			__func__, rxbuf[SDIO_HEADER_LEN + 6]);
		buf_len = rx_length - (MTK_SDIO_PACKET_HEADER_SIZE + 1);
		goto exit;
	} else if (rx_length >= (SDIO_HEADER_LEN + 13)
			&& rxbuf[SDIO_HEADER_LEN] == 0x04
			&& rxbuf[SDIO_HEADER_LEN + 1] == 0xFF
			&& rxbuf[SDIO_HEADER_LEN + 3] == 0x41) {
		/* receive BT clock data */
		pr_debug("%s: This is btclk data - %d, %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
			__func__, rx_length,
			rxbuf[SDIO_HEADER_LEN + 0], rxbuf[SDIO_HEADER_LEN + 1], rxbuf[SDIO_HEADER_LEN + 2],
			rxbuf[SDIO_HEADER_LEN + 3], rxbuf[SDIO_HEADER_LEN + 4], rxbuf[SDIO_HEADER_LEN + 5],
			rxbuf[SDIO_HEADER_LEN + 6], rxbuf[SDIO_HEADER_LEN + 7], rxbuf[SDIO_HEADER_LEN + 8],
			rxbuf[SDIO_HEADER_LEN + 9], rxbuf[SDIO_HEADER_LEN + 10], rxbuf[SDIO_HEADER_LEN + 11],
			rxbuf[SDIO_HEADER_LEN + 12], rxbuf[SDIO_HEADER_LEN + 13], rxbuf[SDIO_HEADER_LEN + 14],
			rxbuf[SDIO_HEADER_LEN + 15], rxbuf[SDIO_HEADER_LEN + 16], rxbuf[SDIO_HEADER_LEN + 17]);

		if (rxbuf[SDIO_HEADER_LEN + 12] == 0x0) {
			u32 intra_clk = 0, clk = 0;

			memcpy(&intra_clk, &rxbuf[SDIO_HEADER_LEN + 6], 2);
			memcpy(&clk, &rxbuf[SDIO_HEADER_LEN + 8], 4);

			LOCK_UNSLEEPABLE_LOCK(&stereo_spin_lock);
			stereo_clk.fw_clk = (u64)(intra_clk + (clk & 0x0FFFFFFC) * 3125 / 10);
			stereo_clk.sys_clk = sys_clk_tmp;
			UNLOCK_UNSLEEPABLE_LOCK(&stereo_spin_lock);
			pr_debug("%s: btclk intra:%x, clk:%x, fw_clk:%llu, sysclk: %llu\n",
				__func__, intra_clk, clk, stereo_clk.fw_clk, stereo_clk.sys_clk);
		} else {
			pr_info("%s: No ACL CONNECTION(%d), disable event and interrupt\n",
				__func__, rxbuf[SDIO_HEADER_LEN + 12]);
		}

		buf_len = rx_length - (MTK_SDIO_PACKET_HEADER_SIZE + 1);
		goto exit;
	} else if (rxbuf[SDIO_HEADER_LEN] == HCI_EVENT_PKT &&
			rxbuf[SDIO_HEADER_LEN + 1] == 0x0E &&
			rxbuf[SDIO_HEADER_LEN + 4] == 0x03 &&
			rxbuf[SDIO_HEADER_LEN + 5] == 0x0C &&
			rxbuf[SDIO_HEADER_LEN + 6] == 0x00) {
		pr_info("%s get hci reset\n", __func__);
		get_hci_reset = 1;
	}

	type = rxbuf[MTK_SDIO_PACKET_HEADER_SIZE];

	btmtk_print_buffer_conent(rxbuf, rx_length);

	/* Read the length of data to be transferred , not include pkt type*/
	buf_len = rx_length - (MTK_SDIO_PACKET_HEADER_SIZE + 1);

	pr_debug("%s buf_len : %d\n", __func__, buf_len);
	if (rx_length <= SDIO_HEADER_LEN) {
		pr_info("invalid packet length: %d\n", buf_len);
		ret = -EINVAL;
		goto exit;
	}

	/* Allocate buffer */
	/* rx_length = num_blocks * blksz + BTSDIO_DMA_ALIGN*/
	skb = bt_skb_alloc(rx_length, GFP_ATOMIC);
	if (skb == NULL) {
		pr_info("No free skb\n");
		ret = -ENOMEM;
		goto exit;
	}

	pr_debug("%s rx_length %d,buf_len %d\n", __func__, rx_length, buf_len);

	memcpy(skb->data, &rxbuf[MTK_SDIO_PACKET_HEADER_SIZE + 1], buf_len);

	switch (type) {
	case HCI_ACLDATA_PKT:
		pr_debug("%s data[2] 0x%02x, data[3] 0x%02x\n",
				__func__, skb->data[2], skb->data[3]);
		buf_len = skb->data[2] + skb->data[3] * 256 + 4;
		pr_debug("%s acl buf_len %d\n", __func__, buf_len);
		break;
	case HCI_SCODATA_PKT:
		buf_len = skb->data[3] + 3;
		break;
	case HCI_EVENT_PKT:
		buf_len = skb->data[1] + 2;
		break;
	default:
		BTSDIO_INFO_RAW(skb->data, buf_len, "%s: CHISR(0x%08X) skb->data(type %d):", __func__, reg_CHISR, type);

		for (retry = 0; retry < 5; retry++) {
			ret = btmtk_sdio_readl(SWPCDBGR, &u32ReadCRValue);
			pr_info("%s ret %d, SWPCDBGR 0x%x, and not sleep!\n",
				__func__, ret, u32ReadCRValue);
		}
		btmtk_sdio_print_debug_sr();

		/* trigger fw core dump */
		if (g_priv->adapter->fops_mode == true)
			btmtk_sdio_trigger_fw_assert();
		ret = -EINVAL;
		goto exit;
	}

	if ((buf_len >= sizeof(READ_ADDRESS_EVENT))
		&& (event_compare_status == BTMTK_SDIO_EVENT_COMPARE_STATE_NEED_COMPARE)) {
		if ((memcmp(skb->data, READ_ADDRESS_EVENT, sizeof(READ_ADDRESS_EVENT)) == 0) && (buf_len == 12)) {
			for (i = 0; i < BD_ADDRESS_SIZE; i++)
				g_card->bdaddr[i] = skb->data[6 + i];

			pr_debug("%s: GET TV BDADDR = %02X:%02X:%02X:%02X:%02X:%02X", __func__,
			g_card->bdaddr[0], g_card->bdaddr[1], g_card->bdaddr[2],
			g_card->bdaddr[3], g_card->bdaddr[4], g_card->bdaddr[5]);

			/*
			 * event_compare_status =
			 * BTMTK_SDIO_EVENT_COMPARE_STATE_COMPARE_SUCCESS;
			 */
		} else
			pr_debug("%s READ_ADDRESS_EVENT compare fail buf_len %d\n", __func__, buf_len);
	}

	if (event_compare_status == BTMTK_SDIO_EVENT_COMPARE_STATE_NEED_COMPARE) {
		if (buf_len >= event_need_compare_len) {
			if (memcmp(skb->data, event_need_compare, event_need_compare_len) == 0) {
				event_compare_status = BTMTK_SDIO_EVENT_COMPARE_STATE_COMPARE_SUCCESS;
				pr_debug("%s compare success\n", __func__);
				/* Drop by driver, don't send to stack */
				goto exit;

			} else {
				pr_debug("%s compare fail\n", __func__);
				BTSDIO_INFO_RAW(event_need_compare, event_need_compare_len,
					"%s: event_need_compare:", __func__);
				BTSDIO_INFO_RAW(skb->data, buf_len,
					"%s: skb->data:", __func__);
			}
		}
	}

	if (is_picus_or_dump == 0) {
		btmtk_sdio_hci_snoop_save(type, skb->data, buf_len);
		btmtk_usb_dispatch_data_bluetooth_kpi(&rxbuf[MTK_SDIO_PACKET_HEADER_SIZE], buf_len + 1, 0);
	}

	fops_skb = bt_skb_alloc(buf_len, GFP_ATOMIC);
	if (fops_skb == NULL) {
		pr_info("%s No free fops_skb\n", __func__);
		ret = -ENOMEM;
		goto exit;
	}

	bt_cb(fops_skb)->pkt_type = type;
	memcpy(fops_skb->data, skb->data, buf_len);

	fops_skb->len = buf_len;
	LOCK_UNSLEEPABLE_LOCK(&(metabuffer.spin_lock));
	skb_queue_tail(&g_priv->adapter->fops_queue, fops_skb);
	if (skb_queue_empty(&g_priv->adapter->fops_queue))
		pr_info("%s fops_queue is empty\n", __func__);
	UNLOCK_UNSLEEPABLE_LOCK(&(metabuffer.spin_lock));

	wake_up_interruptible(&inq);

exit:
	if (skb) {
		pr_debug("%s fail free skb\n", __func__);
		kfree_skb(skb);
	}

	buf_len += 1;
	if (buf_len % 4)
		fourbalignment_len = buf_len + 4 - (buf_len % 4);
	else
		fourbalignment_len = buf_len;

	if (rx_length < fourbalignment_len)
		goto data_err;

	rx_length -= fourbalignment_len;

	if (rx_length > (MTK_SDIO_PACKET_HEADER_SIZE)) {
		memcpy(&rxbuf[MTK_SDIO_PACKET_HEADER_SIZE],
		&rxbuf[MTK_SDIO_PACKET_HEADER_SIZE+fourbalignment_len],
		rx_length-MTK_SDIO_PACKET_HEADER_SIZE);
	}

	pr_debug("%s ret %d, rx_length, %d,fourbalignment_len %d <--\n",
		__func__, ret, rx_length, fourbalignment_len);

	return ret;

data_err:
	pr_info("%s, data error!!! discard rxbuf:\n", __func__);
	BTSDIO_INFO_RAW(rxbuf, rx_length, "rxbuf");
	rx_length = MTK_SDIO_PACKET_HEADER_SIZE;
	return -EINVAL;
}

static int btmtk_sdio_process_int_status(
		struct btmtk_private *priv)
{
	int ret = 0;
	u32 u32rxdatacount = 0;
	u32 u32ReadCRValue = 0;
	static int fw_flag;

	ret = btmtk_sdio_readl(CHISR, &u32ReadCRValue);
	pr_debug("%s CHISR 0x%08x\n", __func__, u32ReadCRValue);
	if (u32ReadCRValue & FIRMWARE_INT_BIT15) {
		if (fw_flag == 0) {
			btmtk_sdio_set_no_fw_own(g_priv, TRUE);
			btmtk_sdio_writel(CHISR, FIRMWARE_INT_BIT15);
			pr_info("%s CHISR 0x%08x\n", __func__, u32ReadCRValue);
			fw_flag = 1;
		}
	} else {
		fw_flag = 0;
	}

	pr_debug("%s check TX_EMPTY CHISR 0x%08x\n", __func__, u32ReadCRValue);
	if (TX_EMPTY&u32ReadCRValue) {
		ret = btmtk_sdio_writel(CHISR, (TX_EMPTY | TX_COMPLETE_COUNT));
		priv->btmtk_dev.tx_dnld_rdy = true;
		pr_debug("%s set tx_dnld_rdy 1\n", __func__);
	}

	if (RX_DONE&u32ReadCRValue)
		ret = btmtk_sdio_recv_rx_data();

	if (ret == 0) {
		while (rx_length > (MTK_SDIO_PACKET_HEADER_SIZE)) {
			btmtk_sdio_card_to_host(priv, NULL, -1, 0);
			u32rxdatacount++;
			pr_debug("%s u32rxdatacount %d, rx_length %d\n",
				__func__, u32rxdatacount, rx_length);
		}
	}


	ret = btmtk_sdio_enable_interrupt(1);

	return ret;
}

static void btmtk_sdio_interrupt(struct sdio_func *func)
{
	struct btmtk_private *priv;
	struct btmtk_sdio_card *card;

	card = sdio_get_drvdata(func);

	if (!card)
		return;


	if (!card->priv)
		return;

	priv = card->priv;
	btmtk_sdio_enable_interrupt(0);

	btmtk_interrupt(priv);
}

static int btmtk_sdio_register_dev(struct btmtk_sdio_card *card)
{
	struct sdio_func *func;
	u8	u8ReadCRValue = 0;
	u8 reg;
	int ret = 0;

	if (!card || !card->func) {
		pr_info("Error: card or function is NULL!\n");
		ret = -EINVAL;
		goto failed;
	}

	func = card->func;

	sdio_claim_host(func);

	ret = sdio_enable_func(func);
	sdio_release_host(g_card->func);
	if (ret) {
		pr_info("sdio_enable_func() failed: ret=%d\n", ret);
		ret = -EIO;
		goto failed;
	}

	btmtk_sdio_readb(SDIO_CCCR_IENx, &u8ReadCRValue);
	pr_info("before claim irq read SDIO_CCCR_IENx %x, func num %d\n",
		u8ReadCRValue, func->num);

	sdio_claim_host(g_card->func);
	ret = sdio_claim_irq(func, btmtk_sdio_interrupt);
	sdio_release_host(g_card->func);
	if (ret) {
		pr_info("sdio_claim_irq failed: ret=%d\n", ret);
		ret = -EIO;
		goto disable_func;
	}
	pr_info("sdio_claim_irq success: ret=%d\n", ret);

	btmtk_sdio_readb(SDIO_CCCR_IENx, &u8ReadCRValue);
	pr_info("after claim irq read SDIO_CCCR_IENx %x\n", u8ReadCRValue);

	sdio_claim_host(g_card->func);
	ret = sdio_set_block_size(card->func, SDIO_BLOCK_SIZE);
	sdio_release_host(g_card->func);
	if (ret) {
		pr_info("cannot set SDIO block size\n");
		ret = -EIO;
		goto release_irq;
	}

	ret = btmtk_sdio_readb(card->reg->io_port_0, &reg);
	if (ret < 0) {
		ret = -EIO;
		goto release_irq;
	}
	card->ioport = reg;

	ret = btmtk_sdio_readb(card->reg->io_port_1, &reg);
	if (ret < 0) {
		ret = -EIO;
		goto release_irq;
	}
	card->ioport |= (reg << 8);

	ret = btmtk_sdio_readb(card->reg->io_port_2, &reg);
	if (ret < 0) {
		ret = -EIO;
		goto release_irq;
	}

	card->ioport |= (reg << 16);

	pr_info("SDIO FUNC%d IO port: 0x%x\n", func->num, card->ioport);

	if (card->reg->int_read_to_clear) {
		ret = btmtk_sdio_readb(card->reg->host_int_rsr, &reg);
		if (ret < 0) {
			ret = -EIO;
			goto release_irq;
		}
		ret = btmtk_sdio_writeb(card->reg->host_int_rsr, reg | 0x3f);
		if (ret < 0) {
			ret = -EIO;
			goto release_irq;
		}

		ret = btmtk_sdio_readb(card->reg->card_misc_cfg, &reg);
		if (ret < 0) {
			ret = -EIO;
			goto release_irq;
		}
		ret = btmtk_sdio_writeb(card->reg->card_misc_cfg, reg | 0x10);
		if (ret < 0) {
			ret = -EIO;
			goto release_irq;
		}
	}

	sdio_set_drvdata(func, card);

	return 0;

release_irq:
	sdio_release_irq(func);

disable_func:
	sdio_disable_func(func);

failed:
	pr_info("%s fail\n", __func__);
	return ret;
}

static int btmtk_sdio_unregister_dev(struct btmtk_sdio_card *card)
{
	if (card && card->func) {
		sdio_claim_host(card->func);
		sdio_release_irq(card->func);
		sdio_disable_func(card->func);
		sdio_release_host(card->func);
		sdio_set_drvdata(card->func, NULL);
	}

	return 0;
}

static int btmtk_sdio_enable_host_int(struct btmtk_sdio_card *card)
{
	int ret;
	u32 read_data = 0;

	if (!card || !card->func)
		return -EINVAL;

	ret = btmtk_sdio_enable_host_int_mask(card, HIM_ENABLE);

	btmtk_sdio_get_rx_unit(card);

	if (0) {
		typedef int (*fp_sdio_hook)(struct mmc_host *host,
						unsigned int width);
		fp_sdio_hook func_sdio_hook =
			(fp_sdio_hook)kallsyms_lookup_name("mmc_set_bus_width");
		unsigned char data = 0;

		sdio_claim_host(g_card->func);
		data = sdio_f0_readb(card->func, SDIO_CCCR_IF, &ret);
		if (ret)
			pr_info("%s sdio_f0_readb ret %d\n", __func__, ret);

		pr_info("%s sdio_f0_readb data 0x%X!\n", __func__, data);

		data  &= ~SDIO_BUS_WIDTH_MASK;
		data  |= SDIO_BUS_ASYNC_INT;
		card->func->card->quirks |= MMC_QUIRK_LENIENT_FN0;

		sdio_f0_writeb(card->func, data, SDIO_CCCR_IF, &ret);
		if (ret)
			pr_info("%s sdio_f0_writeb ret %d\n", __func__, ret);

		pr_info("%s func_sdio_hook at 0x%p!\n",
			__func__, func_sdio_hook);
		if (func_sdio_hook)
			func_sdio_hook(card->func->card->host, MMC_BUS_WIDTH_1);

		data = sdio_f0_readb(card->func, SDIO_CCCR_IF, &ret);
		if (ret)
			pr_info("%s sdio_f0_readb 2 ret %d\n",
				__func__, ret);
		sdio_release_host(g_card->func);

		pr_info("%s sdio_f0_readb2 data 0x%X\n", __func__, data);
	}

/* workaround for some platform no host clock sometimes */

	btmtk_sdio_readl(CSDIOCSR, &read_data);
	pr_info("%s read CSDIOCSR is 0x%X\n", __func__, read_data);
	read_data |= 0x4;
	btmtk_sdio_writel(CSDIOCSR, read_data);
	pr_info("%s write CSDIOCSR is 0x%X\n", __func__, read_data);

	return ret;
}

static int btmtk_sdio_disable_host_int(struct btmtk_sdio_card *card)
{
	int ret;

	if (!card || !card->func)
		return -EINVAL;

	ret = btmtk_sdio_disable_host_int_mask(card, HIM_DISABLE);

	return ret;
}

static int btmtk_sdio_download_fw(struct btmtk_sdio_card *card)
{
	int ret = 0;

	pr_info("%s begin\n", __func__);
	if (!card || !card->func) {
		pr_info("card or function is NULL!\n");
		return -EINVAL;
	}

	sdio_claim_host(card->func);

	if (btmtk_sdio_download_rom_patch(card)) {
		pr_info("Failed to download firmware!\n");
		ret = -EIO;
	}
	sdio_release_host(card->func);

	return ret;
}

static int btmtk_sdio_push_data_to_metabuffer(
						struct ring_buffer *metabuffer,
						char *data,
						int len,
						u8 type,
						bool use_type)
{
	int remainLen = 0;

	if (metabuffer->write_p >= metabuffer->read_p)
		remainLen = metabuffer->write_p - metabuffer->read_p;
	else
		remainLen = META_BUFFER_SIZE -
			(metabuffer->read_p - metabuffer->write_p);

	if ((remainLen + 1 + len) >= META_BUFFER_SIZE) {
		pr_info("%s copy copyLen %d > META_BUFFER_SIZE(%d), push back to queue\n",
			__func__,
			(remainLen + 1 + len),
			META_BUFFER_SIZE);
		return -1;
	}

	if (use_type) {
		metabuffer->buffer[metabuffer->write_p] = type;
		metabuffer->write_p++;
	}
	if (metabuffer->write_p >= META_BUFFER_SIZE)
		metabuffer->write_p = 0;

	if (metabuffer->write_p + len <= META_BUFFER_SIZE)
		memcpy(&metabuffer->buffer[metabuffer->write_p],
			data,
			len);
	else {
		memcpy(&metabuffer->buffer[metabuffer->write_p],
			data,
			META_BUFFER_SIZE - metabuffer->write_p);
		memcpy(metabuffer->buffer,
			&data[META_BUFFER_SIZE - metabuffer->write_p],
			len - (META_BUFFER_SIZE - metabuffer->write_p));
	}

	metabuffer->write_p += len;
	if (metabuffer->write_p >= META_BUFFER_SIZE)
		metabuffer->write_p -= META_BUFFER_SIZE;

	remainLen += (1 + len);
	return 0;
}

static int btmtk_sdio_pull_data_from_metabuffer(
						struct ring_buffer *metabuffer,
						char __user *buf,
						size_t count)
{
	int copyLen = 0;
	unsigned long ret = 0;

	if (metabuffer->write_p >= metabuffer->read_p)
		copyLen = metabuffer->write_p - metabuffer->read_p;
	else
		copyLen = META_BUFFER_SIZE -
			(metabuffer->read_p - metabuffer->write_p);

	if (copyLen > count)
		copyLen = count;

	if (metabuffer->read_p + copyLen <= META_BUFFER_SIZE)
		ret = copy_to_user(buf,
				&metabuffer->buffer[metabuffer->read_p],
				copyLen);
	else {
		ret = copy_to_user(buf,
				&metabuffer->buffer[metabuffer->read_p],
				META_BUFFER_SIZE - metabuffer->read_p);
		if (!ret)
			ret = copy_to_user(
				&buf[META_BUFFER_SIZE - metabuffer->read_p],
				metabuffer->buffer,
				copyLen - (META_BUFFER_SIZE-metabuffer->read_p));
	}

	if (ret)
		pr_info("%s copy to user fail, ret %d\n", __func__, (int)ret);

	metabuffer->read_p += (copyLen - ret);
	if (metabuffer->read_p >= META_BUFFER_SIZE)
		metabuffer->read_p -= META_BUFFER_SIZE;

	return (copyLen - ret);
}

static int btmtk_sdio_reset_dev(struct btmtk_sdio_card *card)
{
	struct sdio_func *func = NULL;
	u8 reg  = 0;
	int ret = 0;

	if (!card || !card->func) {
		pr_info("Error: card or function is NULL!\n");
		return -1;
	}

	func = card->func;

	sdio_claim_host(func);

	ret = sdio_enable_func(func);
	if (ret) {
		pr_info("sdio_enable_func() failed: ret=%d\n", ret);
		goto reset_dev_end;
	}

	reg = sdio_f0_readb(func, SDIO_CCCR_IENx, &ret);
	if (ret) {
		pr_info("%s read SDIO_CCCR_IENx %x, func num %d, ret %d\n",
			__func__, reg, func->num, ret);
		goto reset_dev_end;
	}

	/*return negative value due to inturrept function is register before*/
	ret = sdio_claim_irq(func, btmtk_sdio_interrupt);
	pr_info("%s: sdio_claim_irq return %d\n", __func__, ret);

	reg |= 1 << func->num;
	reg |= 1;

	/* for bt driver can write SDIO_CCCR_IENx */
	func->card->quirks |= MMC_QUIRK_LENIENT_FN0;
	ret = 0;
	sdio_f0_writeb(func, reg, SDIO_CCCR_IENx, &ret);
	if (ret) {
		pr_info("%s f0_writeb SDIO_CCCR_IENx %x, func num %d, ret %d error\n",
			__func__, reg, func->num, ret);
		goto reset_dev_end;
	}

	reg = sdio_f0_readb(func, SDIO_CCCR_IENx, &ret);
	if (ret) {
		pr_info("%s f0_readb SDIO_CCCR_IENx %x, func num %d, ret %d error\n",
			__func__, reg, func->num, ret);
		goto reset_dev_end;
	}

	ret = sdio_set_block_size(card->func, SDIO_BLOCK_SIZE);
	if (ret) {
		pr_info("%s cannot set SDIO block size\n", __func__);
		goto reset_dev_end;
	}

	reg = sdio_readb(func, card->reg->io_port_0, &ret);
	if (ret < 0) {
		pr_info("%s read io port0 fail\n", __func__);
		goto reset_dev_end;
	}

	card->ioport = reg;

	reg = sdio_readb(func, card->reg->io_port_1, &ret);
	if (ret < 0) {
		pr_info("%s read io port1 fail\n", __func__);
		goto reset_dev_end;
	}

	card->ioport |= (reg << 8);

	reg = sdio_readb(func, card->reg->io_port_2, &ret);
	if (ret < 0) {
		pr_info("%s read io port2 fail\n", __func__);
		goto reset_dev_end;
	}

	card->ioport |= (reg << 16);

	pr_info("%s SDIO FUNC%d IO port: 0x%x\n", __func__,
		func->num, card->ioport);

	if (card->reg->int_read_to_clear) {
		reg = sdio_readb(func, card->reg->host_int_rsr, &ret);
		if (ret < 0) {
			pr_info("%s read init rsr fail\n", __func__);
			goto reset_dev_end;
		}
		sdio_writeb(func, reg | 0x3f, card->reg->host_int_rsr, &ret);
		if (ret < 0) {
			pr_info("%s write init rsr fail\n", __func__);
			goto reset_dev_end;
		}

		reg = sdio_readb(func, card->reg->card_misc_cfg, &ret);
		if (ret < 0) {
			pr_info("%s read misc cfg fail\n", __func__);
			goto reset_dev_end;
		}
		sdio_writeb(func, reg | 0x10, card->reg->card_misc_cfg, &ret);
		if (ret < 0)
			pr_info("write misc cfg fail\n");
	}

	sdio_set_drvdata(func, card);
reset_dev_end:
	sdio_release_host(func);

	return ret;
}

static int btmtk_sdio_reset_fw(struct btmtk_sdio_card *card)
{
	int ret = 0;

	pr_info("%s Mediatek Bluetooth driver Version=%s\n",
			__func__, VERSION);

	if (card->bt_cfg.support_woble_by_eint) {
		btmtk_sdio_RegisterBTIrq(card);
		btmtk_sdio_woble_input_init(card);
	}

	pr_debug("%s func device %X\n", __func__, card->func->device);
	pr_debug("%s Call btmtk_sdio_register_dev\n", __func__);
	ret = btmtk_sdio_reset_dev(card);
	if (ret) {
		pr_info("%s btmtk_sdio_reset_dev failed!\n", __func__);
		return ret;
	}

	pr_debug("%s btmtk_sdio_register_dev success\n", __func__);
	btmtk_sdio_enable_host_int(card);
	if (btmtk_sdio_download_fw(card)) {
		pr_info("%s Downloading firmware failed!\n", __func__);
		ret = -ENODEV;
	}

	return ret;
}

static int btmtk_sdio_set_card_clkpd(int on)
{
	int ret = -1;
	/* call sdio_set_card_clkpd in sdio host driver */
	typedef void (*psdio_set_card_clkpd) (int on, struct sdio_func *func);
	char *sdio_set_card_clkpd_func_name = "sdio_set_card_clkpd";
	psdio_set_card_clkpd psdio_set_card_clkpd_func =
		(psdio_set_card_clkpd)kallsyms_lookup_name
				(sdio_set_card_clkpd_func_name);

	if (psdio_set_card_clkpd_func) {
		pr_info("%s: get  %s\n", __func__,
				sdio_set_card_clkpd_func_name);
		psdio_set_card_clkpd_func(on, g_card->func);
		ret = 0;
	} else
		pr_info("%s: do not get %s\n",
			__func__,
			sdio_set_card_clkpd_func_name);
	return ret;
}


/*toggle PMU enable*/
static int btmtk_sdio_toggle_rst_pin(void)
{
	uint32_t pmu_en_delay = MT76x8_PMU_EN_DEFAULT_DELAY;
	int pmu_en;
	struct device *prDev;

	if (g_card == NULL) {
		pr_info("g_card is NULL return\n");
		return -1;
	}
	sdio_claim_host(g_card->func);
	btmtk_sdio_set_card_clkpd(0);
	sdio_release_host(g_card->func);
	prDev = mmc_dev(g_card->func->card->host);
	if (!prDev) {
		pr_info("unable to get struct dev for BT\n");
		return -1;
	}
	pmu_en = of_get_named_gpio(prDev->of_node, MT76x8_PMU_EN_PIN_NAME, 0);
	pr_info("%s pmu_en %d\n", __func__, pmu_en);
	if (gpio_is_valid(pmu_en)) {
		gpio_direction_output(pmu_en, 0);
		mdelay(pmu_en_delay);
		gpio_direction_output(pmu_en, 1);
		pr_info("%s: %s pull low/high done\n", __func__,
				MT76x8_PMU_EN_PIN_NAME);
	} else {
		pr_info("%s: *** Invalid GPIO %s ***\n", __func__,
				MT76x8_PMU_EN_PIN_NAME);
		return -1;
	}
	return 0;
}

int btmtk_sdio_notify_wlan_remove_end(void)
{
	pr_info("%s begin\n", __func__);
	wlan_remove_done = 1;
	btmtk_sdio_stop_wait_wlan_remove_tsk();

	pr_info("%s done\n", __func__);
	return 0;
}
EXPORT_SYMBOL(btmtk_sdio_notify_wlan_remove_end);

int btmtk_sdio_bt_trigger_core_dump(int trigger_dump)
{
	struct sk_buff *skb = NULL;
	u8 coredump_cmd[] = {0x6F, 0xFC, 0x05,
			0x00, 0x01, 0x02, 0x01, 0x00, 0x08};

	if (g_priv == NULL) {
		pr_info("%s g_priv is NULL return\n", __func__);
		return 0;
	}

	if (wait_dump_complete_tsk) {
		pr_info("%s wait_dump_complete_tsk is working, return\n",
			__func__);
		return 0;
	}

	if (wait_wlan_remove_tsk) {
		pr_info("%s wait_wlan_remove_tsk is working, return\n",
			__func__);
		return 0;
	}

	if (g_priv->btmtk_dev.reset_dongle) {
		pr_info("%s reset_dongle is true, return\n", __func__);
		return 0;
	}

	if (!probe_ready) {
		pr_info("%s probe_ready %d, return -1\n",
			__func__,
			probe_ready);
		return -1;/*BT driver is not ready, ask wifi do coredump*/
	}

	pr_info("%s: trigger_dump %d\n", __func__, trigger_dump);
	if (trigger_dump) {
		if (is_mt7663(g_card))
			wlan_status = WLAN_STATUS_CALL_REMOVE_START;
		skb = bt_skb_alloc(sizeof(coredump_cmd), GFP_ATOMIC);
		bt_cb(skb)->pkt_type = HCI_COMMAND_PKT;
		memcpy(&skb->data[0], &coredump_cmd[0], sizeof(coredump_cmd));
		skb->len = sizeof(coredump_cmd);
		skb_queue_tail(&g_priv->adapter->tx_queue, skb);
		wake_up_interruptible(&g_priv->main_thread.wait_q);
	} else {
		wait_wlan_remove_tsk =
			kthread_run(btmtk_sdio_wait_wlan_remove_thread,
				NULL,
				"btmtk_sdio_wait_dump_complete_thread");

		msleep(100);
		btmtk_sdio_notify_wlan_remove_start();
	}

	return 0;
}
EXPORT_SYMBOL(btmtk_sdio_bt_trigger_core_dump);

void btmtk_sdio_notify_wlan_toggle_rst_end(void)
{
	typedef void (*pnotify_wlan_toggle_rst_end) (int reserved);
	char *notify_wlan_toggle_rst_end_func_name =
			"notify_wlan_toggle_rst_end";
	/*void notify_wlan_toggle_rst_end(void)*/
	pnotify_wlan_toggle_rst_end pnotify_wlan_toggle_rst_end_func =
		(pnotify_wlan_toggle_rst_end) kallsyms_lookup_name
				(notify_wlan_toggle_rst_end_func_name);

	pr_info(L0_RESET_TAG "%s\n", __func__);
	if (pnotify_wlan_toggle_rst_end_func) {
		pr_info("%s: do notify %s\n",
			__func__,
			notify_wlan_toggle_rst_end_func_name);
		pnotify_wlan_toggle_rst_end_func(1);
	} else
		pr_info("%s: do not get %s\n",
			__func__,
			notify_wlan_toggle_rst_end_func_name);
}

int btmtk_sdio_driver_reset_dongle(void)
{
	int ret = 0;
	int retry = 3;

	pr_info("%s: begin\n", __func__);
	if (g_priv == NULL) {
		pr_info("%s: g_priv = NULL, return\n", __func__);
		return -1;
	}

	need_reset_stack = 1;
	wlan_remove_done = 0;

retry_reset:
	retry--;
	if (retry < 0) {
		pr_info("%s retry overtime fail\n", __func__);
		goto rst_dongle_err;
	}
	pr_info("%s run %d\n", __func__, retry);
	ret = 0;
	if (btmtk_sdio_toggle_rst_pin()) {
		ret = -1;
		goto rst_dongle_err;
	}

	btmtk_sdio_set_no_fw_own(g_priv, FALSE);
	msleep(100);
	sdio_claim_host(g_card->func);

	do {
		typedef int (*func) (struct mmc_card *card);
		char *func_name = "sdio_reset_comm";
		func pFunc = (func) kallsyms_lookup_name(func_name);

		if (pFunc) {
			pr_info("%s: Invoke %s\n", __func__, func_name);
			ret = pFunc(g_card->func->card);
		} else
			pr_info("%s: No Exported Func Found [%s]\n", __func__, func_name);
	} while (0);

	sdio_release_host(g_card->func);
	if (ret) {
		pr_info("%s: sdio_reset_comm error %d\n", __func__, ret);
		goto retry_reset;
	}
	pr_info("%s: sdio_reset_comm done\n", __func__);
	msleep(100);
	ret = btmtk_sdio_reset_fw(g_card);
	if (ret) {
		pr_info("%s reset fw fail\n", __func__);
		goto retry_reset;
	} else
		pr_info("%s reset fw done\n", __func__);

rst_dongle_err:
	btmtk_sdio_notify_wlan_toggle_rst_end();

	g_priv->btmtk_dev.tx_dnld_rdy = 1;
	g_priv->btmtk_dev.reset_dongle = 0;

	wlan_status = WLAN_STATUS_DEFAULT;
	btmtk_clean_queue();
	g_priv->btmtk_dev.reset_progress = 0;
	dump_data_counter = 0;
	pr_info("%s return ret = %d\n", __func__, ret);
	return ret;
}

int WF_rst_L0_notify_BT_step2(void)
{
	int ret = -1;

	if (is_mt7663(g_card)) {
		pr_info(L0_RESET_TAG "%s begin\n", __func__);
		btmtk_sdio_notify_wlan_remove_end();
		pr_info(L0_RESET_TAG "%s done\n", __func__);
		ret = 0;
	} else {
		pr_info(L0_RESET_TAG "%s is not MT7663\n", __func__);
	}
	return ret;
}
EXPORT_SYMBOL(WF_rst_L0_notify_BT_step2);

int WF_rst_L0_notify_BT_step1(int reserved)
{
	int ret = -1;

	if (is_mt7663(g_card)) {
		pr_info(L0_RESET_TAG "%s begin\n", __func__);
		btmtk_sdio_bt_trigger_core_dump(true);
		pr_info(L0_RESET_TAG "%s done\n", __func__);
		ret = 0;
	} else {
		pr_info(L0_RESET_TAG "%s is not MT7663\n", __func__);
	}
	return ret;
}
EXPORT_SYMBOL(WF_rst_L0_notify_BT_step1);

#ifdef MTK_KERNEL_DEBUG
static int btmtk_sdio_L0_hang_thread(void *data)
{
	do {
		pr_info(L0_RESET_TAG "Whole Chip Reset was triggered\n");
		msleep(3000);
	} while (1);
	return 0;
}

static int btmtk_sdio_L0_debug_probe(struct sdio_func *func,
					const struct sdio_device_id *id)
{
	int ret = 0;
	struct task_struct *task = NULL;
	struct btmtk_sdio_card *card = NULL;
	struct btmtk_sdio_device *data = (void *) id->driver_data;
	u32 u32ReadCRValue = 0;
	u8 fw_download_fail = 0;

	pr_info(L0_RESET_TAG "%s flow end\n", __func__);
	probe_counter++;
	pr_info(L0_RESET_TAG "%s Mediatek Bluetooth driver Version=%s\n",
			__func__, VERSION);
	pr_info(L0_RESET_TAG "vendor=0x%x, device=0x%x, class=%d, fn=%d, support func_num %d\n",
			id->vendor, id->device, id->class,
			func->num, data->reg->func_num);

	if (func->num != data->reg->func_num) {
		pr_info(L0_RESET_TAG "func num is not match\n");
		return -ENODEV;
		}

	card = devm_kzalloc(&func->dev, sizeof(*card), GFP_KERNEL);
	if (!card)
		return -ENOMEM;

	card->func = func;
	g_card = card;

	if (id->driver_data) {
		card->helper = data->helper;
		card->reg = data->reg;
		card->sd_blksz_fw_dl = data->sd_blksz_fw_dl;
		card->support_pscan_win_report = data->support_pscan_win_report;
		card->supports_fw_dump = data->supports_fw_dump;
		card->chip_id = data->reg->chip_id;
		card->suspend_count = 0;
		pr_info(L0_RESET_TAG "chip_id %x\n", data->reg->chip_id);
	}

	if (btmtk_sdio_register_dev(card) < 0) {
		pr_info(L0_RESET_TAG "Failed to register BT device!\n");
		return -ENODEV;
	}
	pr_info("%s btmtk_sdio_register_dev success\n", __func__);

	/* Disable the interrupts on the card */
	btmtk_sdio_enable_host_int(card);
	pr_debug(L0_RESET_TAG "call btmtk_sdio_enable_host_int done\n");
	if (btmtk_sdio_download_fw(card)) {
		pr_info(L0_RESET_TAG "Downloading firmware failed!\n");
		fw_download_fail = 1;
	}

	task = kthread_run(btmtk_sdio_L0_hang_thread,
					NULL, "btmtk_sdio_L0_hang_thread");
	if (IS_ERR(task)) {
		pr_info(L0_RESET_TAG "%s create thread fail", __func__);
		goto unreg_dev;
	}

	ret = btmtk_sdio_readl(CHLPCR, &u32ReadCRValue);
	pr_debug(L0_RESET_TAG "%s chipid (0x%X)\n", __func__, g_card->chip_id);

	pr_info(L0_RESET_TAG "%s PASS!!\n", __func__);

	if (fw_download_fail)
		btmtk_sdio_start_reset_dongle_progress();

	return 0;

unreg_dev:
	btmtk_sdio_unregister_dev(card);

	pr_info(L0_RESET_TAG "%s fail end\n", __func__);
	return ret;
}
#endif

static int btmtk_sdio_L0_probe(struct sdio_func *func,
					const struct sdio_device_id *id)
{
	/* Set flags/functions here to leave HW reset mark before probe. */
	need_reset_stack = 1;

	/* Now, ready to branch onto true sdio card probe. */
	return btmtk_sdio_probe(func, id);
}

static int btmtk_sdio_L0_reset_host_config(struct mmc_host *host)
{

	if (host == NULL) {
		pr_info(L0_RESET_TAG "%s mmc host is NULL\n", __func__);
		return -1;
	}

	if (host->rescan_entered != 0) {
		host->rescan_entered = 0;
		pr_info(L0_RESET_TAG "set mmc_host rescan to 0\n");
	}

	pr_debug(L0_RESET_TAG "%s done\n", __func__);
		return 0;
}

static int btmtk_sdio_L0_reset(struct mmc_card *card)
{
	int ret = -1;
	struct mmc_host *host = NULL;

	if ((card == NULL) || (card->host  == NULL)) {
		pr_info(L0_RESET_TAG "%s mmc structs are NULL\n", __func__);
		return ret;
	}

	host = card->host;
	ret = btmtk_sdio_L0_reset_host_config(host);
	if (ret != 0) {
		pr_info(L0_RESET_TAG "%s set SDIO host failed\n", __func__);
		return ret;
	}

	pr_info(L0_RESET_TAG "%s mmc_remove_host\n", __func__);
	mmc_remove_host(host);

	/* Replace hooked SDIO driver probe to new API;
	 * 1. It will be new kthread(state) after mmc_add_host;
	 * 2. Extend flexibility to notify us that HW reset was triggered,
	 * more flexiable on reviving in exchanging old/new kthread(state).
	 */
#ifdef MTK_KERNEL_DEBUG
	/* For DBG purpose only, replace to customized probe.
	 * Will only re-probe SDIO card function then hang for warning.
	 */
	btmtk_sdio_L0_hook_new_probe(btmtk_sdio_L0_debug_probe);
#else
	btmtk_sdio_L0_hook_new_probe(btmtk_sdio_L0_probe);
#endif

	pr_info(L0_RESET_TAG "%s mmc_add_host\n", __func__);
	ret = mmc_add_host(host);

	pr_info(L0_RESET_TAG "%s: mmc_add_host return %d\n", __func__, ret);
	return ret;
}

int btmtk_sdio_host_reset_dongle(void)
{
	int ret = -1;

	pr_info("%s: begin\n", __func__);
	if (g_priv == NULL) {
		pr_info("%s: g_priv = NULL, return\n", __func__);
		goto rst_dongle_done;
	}
	if ((!g_card) || (!g_card->func) || (!g_card->func->card)) {
		pr_info(L0_RESET_TAG "data corrupted\n");
		goto rst_dongle_done;
	}

	need_reset_stack = 1;
	wlan_remove_done = 0;

	ret = btmtk_sdio_L0_reset(g_card->func->card);
	pr_info(L0_RESET_TAG "%s HW Reset status <%d>.\n",
			__func__, ret);

rst_dongle_done:
	btmtk_sdio_notify_wlan_toggle_rst_end();

	g_priv->btmtk_dev.tx_dnld_rdy = 1;
	g_priv->btmtk_dev.reset_dongle = 0;

	wlan_status = WLAN_STATUS_DEFAULT;
	btmtk_clean_queue();
	g_priv->btmtk_dev.reset_progress = 0;
	dump_data_counter = 0;
	dump_data_length = 0;
	return ret;
}

int btmtk_sdio_reset_dongle(void)
{
	if (is_mt7663(g_card))
		return btmtk_sdio_host_reset_dongle();
	else
		return btmtk_sdio_driver_reset_dongle();
}

static irqreturn_t btmtk_sdio_woble_isr(int irq, void *dev)
{
	struct btmtk_sdio_card *data = (struct btmtk_sdio_card *)dev;

	pr_info("%s begin\n", __func__);
	disable_irq_nosync(data->wobt_irq);
	atomic_dec(&(data->irq_enable_count));
	pr_info("%s:disable BT IRQ\n", __func__);
	pr_info("%s:call wake lock\n", __func__);
	__pm_wakeup_event(&data->eint_ws, WAIT_POWERKEY_TIMEOUT);

	input_report_key(data->WoBLEInputDev, KEY_WAKEUP, 1);
	input_sync(data->WoBLEInputDev);
	input_report_key(data->WoBLEInputDev, KEY_WAKEUP, 0);
	input_sync(data->WoBLEInputDev);
	pr_info("%s end\n", __func__);
	return IRQ_HANDLED;
}

static int btmtk_sdio_RegisterBTIrq(struct btmtk_sdio_card *data)
{
	struct device_node *eint_node = NULL;
	int interrupts[2];

	eint_node = of_find_compatible_node(NULL, NULL, "mediatek,mt7668_bt_ctrl");
	pr_info("%s begin\n", __func__);
	if (eint_node) {
		pr_info("%s Get mt76xx_bt_ctrl compatible node\n", __func__);
		data->wobt_irq = irq_of_parse_and_map(eint_node, 0);
		pr_info("%s wobt_irq number:%d", __func__, data->wobt_irq);
		if (data->wobt_irq) {
			of_property_read_u32_array(eint_node, "interrupts",
						   interrupts, ARRAY_SIZE(interrupts));
			data->wobt_irqlevel = interrupts[1];
			if (request_irq(data->wobt_irq, btmtk_sdio_woble_isr,
					data->wobt_irqlevel, "mt7668_bt_ctrl-eint", data))
				pr_info("%s WOBTIRQ LINE NOT AVAILABLE!!\n", __func__);
			else {
				pr_info("%s disable BT IRQ\n", __func__);
				disable_irq_nosync(data->wobt_irq);
			}

		} else
			pr_info("%s can't find mt76xx_bt_ctrl irq\n", __func__);

	} else {
		data->wobt_irq = 0;
		pr_info("%s can't find mt76xx_bt_ctrl compatible node\n", __func__);
	}


	pr_info("btmtk:%s: end\n", __func__);
	return 0;
}

static int btmtk_sdio_woble_input_init(struct btmtk_sdio_card *data)
{
	int ret = 0;

	data->WoBLEInputDev = input_allocate_device();
	if (IS_ERR(data->WoBLEInputDev)) {
		pr_info("%s: input_allocate_device error\n", __func__);
		return -ENOMEM;
	}

	data->WoBLEInputDev->name = "WOBLE_INPUT_DEVICE";
	data->WoBLEInputDev->id.bustype = BUS_HOST;
	data->WoBLEInputDev->id.vendor = 0x0002;
	data->WoBLEInputDev->id.product = 0x0002;
	data->WoBLEInputDev->id.version = 0x0002;

	__set_bit(EV_KEY, data->WoBLEInputDev->evbit);
	__set_bit(KEY_WAKEUP, data->WoBLEInputDev->keybit);

	ret = input_register_device(data->WoBLEInputDev);
	if (ret < 0) {
		input_free_device(data->WoBLEInputDev);
		pr_info("%s: input_register_device %d\n", __func__, ret);
		return ret;
	}

	return ret;
}

static int btmtk_stereo_irq_handler(int irq, void *dev)
{
	/* Get sys clk */
	struct timeval tv;

	do_gettimeofday(&tv);
	sys_clk_tmp = tv.tv_sec * 1000000 + tv.tv_usec;
	pr_debug("%s: tv_sec %d, tv_usec %d, sys_clk %llu\n", __func__,
		(int)tv.tv_sec, (int)tv.tv_usec, sys_clk_tmp);
	return 0;
}

static int btmtk_stereo_reg_irq(void)
{
	int ret = 0;
	struct device_node *node;
	int stereo_gpio;

	pr_info("%s start\n", __func__);
	node = of_find_compatible_node(NULL, NULL, "mediatek,connectivity-combo");
	if (node) {
		stereo_gpio = of_get_named_gpio(node, "gpio_bt_stereo_pin", 0);
		pr_info("%s pmu_en %d\n", __func__, stereo_gpio);
		if (gpio_is_valid(stereo_gpio))
			gpio_direction_input(stereo_gpio);
		else
			pr_info("%s invalid stereo gpio\n", __func__);

		stereo_irq = irq_of_parse_and_map(node, 0);
		ret = request_irq(stereo_irq, (irq_handler_t)btmtk_stereo_irq_handler,
						IRQF_TRIGGER_RISING, "BTSTEREO_ISR_Handler", NULL);
		if (ret) {
			pr_info("%s fail(%d)!!! irq_number=%d\n", __func__, ret, stereo_irq);
			stereo_irq = -1;
		}
	} else {
		pr_info("%s of_find_compatible_node fail!!!\n", __func__);
		ret = -1;
		stereo_irq = -1;
	}
	return ret;
}

static void btmtk_stereo_unreg_irq(void)
{
	pr_info("%s enter\n", __func__);
	if (stereo_irq != -1)
		free_irq(stereo_irq, NULL);
	stereo_irq = -1;
	pr_info("%s exit\n", __func__);
}

static int btmtk_sdio_probe(struct sdio_func *func,
					const struct sdio_device_id *id)
{
	int ret = 0;
	struct btmtk_private *priv = NULL;
	struct btmtk_sdio_card *card = NULL;
	struct btmtk_sdio_device *data = (void *) id->driver_data;
	u32 u32ReadCRValue = 0;
	u8 fw_download_fail = 0;

	probe_counter++;
	pr_info("%s Mediatek Bluetooth driver Version=%s\n",
			__func__, VERSION);
	pr_info("vendor=0x%x, device=0x%x, class=%d, fn=%d, support func_num %d\n",
			id->vendor, id->device, id->class,
			func->num, data->reg->func_num);

	if (func->num != data->reg->func_num) {
		pr_info("func num is not match\n");
		return -ENODEV;
	}

	card = devm_kzalloc(&func->dev, sizeof(*card), GFP_KERNEL);
	if (!card)
		return -ENOMEM;

	card->func = func;
	card->bin_file_buffer = NULL;
	g_card = card;

	if (id->driver_data) {
		card->helper = data->helper;
		card->reg = data->reg;
		card->sd_blksz_fw_dl = data->sd_blksz_fw_dl;
		card->support_pscan_win_report = data->support_pscan_win_report;
		card->supports_fw_dump = data->supports_fw_dump;
		card->chip_id = data->reg->chip_id;
		card->suspend_count = 0;
		pr_info("%s chip_id is %x\n", __func__, data->reg->chip_id);
		/*allocate memory for woble_setting_file*/
		g_card->woble_setting_file_name = kzalloc(MAX_BIN_FILE_NAME_LEN, GFP_KERNEL);
		if (!g_card->woble_setting_file_name)
			return -1;
		need_retry_load_woble = 0;
#if SUPPORT_MT7663
		if (is_mt7668(g_card)) {
			memcpy(g_card->woble_setting_file_name,
					WOBLE_SETTING_FILE_NAME_7668,
					sizeof(WOBLE_SETTING_FILE_NAME_7668));
		}
#endif

#if SUPPORT_MT7668
		if (is_mt7663(g_card)) {
			memcpy(g_card->woble_setting_file_name,
					WOBLE_SETTING_FILE_NAME_7663,
					sizeof(WOBLE_SETTING_FILE_NAME_7663));
		}
#endif

		/*allocate memory for bt_cfg_file_name*/
		g_card->bt_cfg_file_name = kzalloc(MAX_BIN_FILE_NAME_LEN, GFP_KERNEL);
		if (!g_card->bt_cfg_file_name)
			return -1;

		memcpy(g_card->bt_cfg_file_name, BT_CFG_NAME, sizeof(BT_CFG_NAME));
	}

	btmtk_sdio_initialize_cfg_items();
	btmtk_sdio_load_setting_files(g_card->bt_cfg_file_name, &g_card->func->dev, g_card);

	pr_debug("%s func device %X\n", __func__, card->func->device);
	pr_debug("%s Call btmtk_sdio_register_dev\n", __func__);
	if (btmtk_sdio_register_dev(card) < 0) {
		pr_info("Failed to register BT device!\n");
		return -ENODEV;
	}

	pr_debug("%s btmtk_sdio_register_dev success\n", __func__);

	/* Disable the interrupts on the card */
	btmtk_sdio_enable_host_int(card);
	pr_debug("call btmtk_sdio_enable_host_int done\n");
	if (btmtk_sdio_download_fw(card)) {
		pr_info("Downloading firmware failed!\n");
		fw_download_fail = 1;
	}

	/* Move from btmtk_fops_open() */
	spin_lock_init(&(metabuffer.spin_lock.lock));
	spin_lock_init(&(fwlog_metabuffer.spin_lock.lock));

	spin_lock_init(&(stereo_spin_lock.lock));

	pr_debug("%s spin_lock_init end\n", __func__);

	priv = btmtk_add_card(card);
	if (!priv) {
		pr_info("Initializing card failed!\n");
		ret = -ENODEV;
		goto unreg_dev;
	}
	pr_debug("btmtk_add_card success\n");
	card->priv = priv;
	pr_debug("assign priv done\n");
	/* Initialize the interface specific function pointers */
	priv->hw_host_to_card = btmtk_sdio_host_to_card;
	priv->hw_process_int_status = btmtk_sdio_process_int_status;
	priv->hw_set_own_back =  btmtk_sdio_set_own_back;
	priv->hw_sdio_reset_dongle = btmtk_sdio_reset_dongle;
	priv->start_reset_dongle_progress = btmtk_sdio_start_reset_dongle_progress;
	priv->hci_snoop_save = btmtk_sdio_hci_snoop_save;
	g_priv = priv;
	btmtk_sdio_set_no_fw_own(g_priv, g_card->is_KeepFullPwr);

	memset(&metabuffer.buffer, 0, META_BUFFER_SIZE);

	fw_dump_file = NULL;

	ret = btmtk_sdio_readl(CHLPCR, &u32ReadCRValue);
	pr_debug("%s read CHLPCR (0x%08X)\n", __func__, u32ReadCRValue);
	pr_debug("%s chipid is  (0x%X)\n", __func__, g_card->chip_id);
	if (is_support_unify_woble(g_card)) {
		memset(g_card->bdaddr, 0, BD_ADDRESS_SIZE);
		btmtk_sdio_load_setting_files(g_card->woble_setting_file_name,
			&g_card->func->dev,
			g_card);
	}

	if (g_card->bt_cfg.support_woble_by_eint) {
		btmtk_sdio_RegisterBTIrq(card);
		btmtk_sdio_woble_input_init(card);
	}

	if (g_card->bt_cfg.support_unify_woble && g_card->bt_cfg.support_woble_wakelock)
		wakeup_source_init(&g_card->woble_ws, "btevent_woble");

	if (g_card->bt_cfg.support_woble_by_eint)
		wakeup_source_init(&g_card->eint_ws, "btevent_eint");

	pr_info("%s normal end\n", __func__);
	probe_ready = true;
	if (fw_download_fail)
		btmtk_sdio_start_reset_dongle_progress();

	return 0;

unreg_dev:
	btmtk_sdio_unregister_dev(card);

	pr_info("%s fail end\n", __func__);
	return ret;
}

static void btmtk_sdio_remove(struct sdio_func *func)
{
	struct btmtk_sdio_card *card;

	pr_info("%s begin user_rmmod %d\n", __func__, user_rmmod);
	probe_ready = false;

	btmtk_sdio_set_no_fw_own(g_priv, FALSE);
	if (func) {
		card = sdio_get_drvdata(func);
		if (card) {
			pr_info(L0_RESET_TAG "%s begin reset_dongle <%d>\n",
				__func__, card->priv->btmtk_dev.reset_dongle);
			/* Send SHUTDOWN command & disable interrupt
			 * if user removes the module.
			 */
			if (user_rmmod) {
				pr_info("%s begin user_rmmod %d in user mode\n",
					__func__, user_rmmod);
				btmtk_sdio_set_own_back(DRIVER_OWN);
				btmtk_sdio_enable_interrupt(0);
				btmtk_sdio_set_own_back(FW_OWN);
				btmtk_sdio_disable_host_int(card);
			}

			if (card->bt_cfg.support_unify_woble && card->bt_cfg.support_woble_wakelock)
				wakeup_source_trash(&card->woble_ws);

			if (card->bt_cfg.support_woble_by_eint)
				wakeup_source_trash(&card->eint_ws);

			btmtk_sdio_woble_free_setting();
			btmtk_sdio_free_bt_cfg();
			pr_debug("unregister dev\n");
			card->priv->surprise_removed = true;
			if (!card->priv->btmtk_dev.reset_dongle)
				btmtk_remove_card(card->priv);
			btmtk_sdio_unregister_dev(card);
			if (card->bin_file_buffer != NULL) {
				kfree(card->bin_file_buffer);
				card->bin_file_buffer = NULL;
			}
			need_reset_stack = 1;
		}
	}
	pr_info("%s end\n", __func__);
}

/*
 * cmd_type:
 * #define HCI_COMMAND_PKT 0x01
 * #define HCI_ACLDATA_PKT 0x02
 * #define HCI_SCODATA_PKT 0x03
 * #define HCI_EVENT_PKT 0x04
 * #define HCI_VENDOR_PKT 0xff
 */
static int btmtk_sdio_send_hci_cmd(u8 cmd_type, u8 *cmd, int cmd_len,
		const u8 *event, const int event_len,
		int total_timeout, bool wait_until)
		/*cmd: if cmd is null, don't compare event, just return 0 if send cmd success*/
		/* total_timeout: -1 */
		/* add_spec_header:0 hci event, 1 use vend specic event header*/
		/* return 0 if compare successfully and no need to compare */
		/* return < 0 if error*/
		/*wait_until: 0:need compare with first event after cmd*/
		/*return value: 0 or positive success, -x fail*/
{
	int ret = -1;
	unsigned long comp_event_timo = 0, start_time = 0;
	struct sk_buff *skb = NULL;

	if (cmd_len == 0) {
		pr_info("%s cmd_len (%d) error return\n", __func__, cmd_len);
		return -EINVAL;
	}


	skb = bt_skb_alloc(cmd_len, GFP_ATOMIC);
	if (skb == NULL) {
		pr_info("skb is null\n");
		return -ENOMEM;
	}
	bt_cb(skb)->pkt_type = cmd_type;
	memcpy(&skb->data[0], cmd, cmd_len);
	skb->len = cmd_len;
	if (event) {
		event_compare_status = BTMTK_SDIO_EVENT_COMPARE_STATE_NEED_COMPARE;
		memcpy(event_need_compare, event, event_len);
		event_need_compare_len = event_len;
	}
	skb_queue_tail(&g_priv->adapter->tx_queue, skb);
	wake_up_interruptible(&g_priv->main_thread.wait_q);


	if (event == NULL)
		return 0;

	if (event_len > EVENT_COMPARE_SIZE) {
		pr_info("%s event_len (%d) > EVENT_COMPARE_SIZE(%d), error\n", __func__, event_len, EVENT_COMPARE_SIZE);
		return -1;
	}

	start_time = jiffies;
	/* check HCI event */
	comp_event_timo = jiffies + msecs_to_jiffies(total_timeout);
	ret = -1;
	pr_debug("%s event_need_compare_len %d\n", __func__, event_need_compare_len);
	pr_debug("%s event_compare_status %d\n", __func__, event_compare_status);
	do {
		/* check if event_compare_success */
		if (event_compare_status == BTMTK_SDIO_EVENT_COMPARE_STATE_COMPARE_SUCCESS) {
			pr_debug("%s compare success\n", __func__);
			ret = 0;
			break;
		}

		msleep(100);
	} while (time_before(jiffies, comp_event_timo));
	event_compare_status = BTMTK_SDIO_EVENT_COMPARE_STATE_NOTHING_NEED_COMPARE;
	pr_debug("%s ret %d\n", __func__, ret);
	return ret;
}

static int btmtk_sdio_trigger_fw_assert(void)
{
	int ret = 0;
	/*
	 * fw dump has 2 types
	 * 1. Assert: trigger by hci cmd "5b fd 00" defined by bluedroid,
	 *    FW support assert now
	 *    And supply a workaround cmd "03 0c 00 00"
	 * 2. Exception: trigger by wmt cmd "6F FC 05 01 02 01 00 08"
	 *    FW not support now
	 */
	u8 cmd_7668[] = { 0x5b, 0xfd, 0x00 };
	u8 cmd_7663[] = { 0x03, 0x0c, 0x00, 0x00 };

	pr_info("%s begin\n", __func__);
	if (is_mt7668(g_card))
		ret = btmtk_sdio_send_hci_cmd(HCI_COMMAND_PKT, cmd_7668,
			sizeof(cmd_7668),
			NULL, 0, WOBLE_COMP_EVENT_TIMO, 1);
	if (is_mt7663(g_card))
		ret = btmtk_sdio_send_hci_cmd(HCI_COMMAND_PKT, cmd_7663,
			sizeof(cmd_7663),
			NULL, 0, WOBLE_COMP_EVENT_TIMO, 1);

	if (ret != 0)
		pr_info("%s ret = %d\n", __func__, ret);
	return ret;
}

static int btmtk_sdio_send_get_vendor_cap(void)
{
	int ret = -1;
	u8 get_vendor_cap_cmd[] = { 0x53, 0xFD, 0x00 };
	u8 get_vendor_cap_event[] = { 0x0e, 0x12, 0x01, 0x53, 0xFD, 0x00};

	pr_debug("%s: begin", __func__);
	BTSDIO_DEBUG_RAW(get_vendor_cap_cmd, (unsigned int)sizeof(get_vendor_cap_cmd),
					"%s: send vendor_cap_cmd is:", __func__);
	ret = btmtk_sdio_send_hci_cmd(HCI_COMMAND_PKT, get_vendor_cap_cmd, sizeof(get_vendor_cap_cmd),
		get_vendor_cap_event, sizeof(get_vendor_cap_event),
				WOBLE_COMP_EVENT_TIMO, 1);

	pr_debug("%s: ret %d", __func__, ret);
	return ret;
}

static int btmtk_sdio_send_read_BDADDR_cmd(void)
{
	u8 cmd[] = { 0x09, 0x10, 0x00 };
	int ret = -1;
	unsigned char zero[BD_ADDRESS_SIZE];

	pr_debug("%s: begin", __func__);
	if (g_card == NULL) {
		pr_info("%s: g_card == NULL!", __func__);
		return -1;
	}

	memset(zero, 0, sizeof(zero));
	if (memcmp(g_card->bdaddr, zero, BD_ADDRESS_SIZE) != 0) {
		pr_debug("%s: already got bdaddr %02x%02x%02x%02x%02x%02x, return 0", __func__,
		g_card->bdaddr[0], g_card->bdaddr[1], g_card->bdaddr[2],
		g_card->bdaddr[3], g_card->bdaddr[4], g_card->bdaddr[5]);
		return 0;
	}
	BTSDIO_DEBUG_RAW(cmd, (unsigned int)sizeof(cmd), "%s: send read bd address cmd is:", __func__);
	ret = btmtk_sdio_send_hci_cmd(HCI_COMMAND_PKT, cmd, sizeof(cmd),
		READ_ADDRESS_EVENT, sizeof(READ_ADDRESS_EVENT), WOBLE_COMP_EVENT_TIMO, 1);
	/*BD address will get in btmtk_sdio_host_to_card*/
	pr_debug("%s: ret = %d", __func__, ret);

	return ret;
}

static int btmtk_sdio_set_Woble_APCF_filter_parameter(void)
{
	int ret = -1;
	u8 cmd[] = { 0x57, 0xfd, 0x0a, 0x01, 0x00, 0x5a, 0x20, 0x00, 0x20, 0x00, 0x01, 0x80, 0x00 };
	u8 event[] = { 0x0e, 0x07, 0x01, 0x57, 0xfd, 0x00, 0x01/*, 00, 63*/ };

	pr_debug("%s: begin", __func__);
	ret = btmtk_sdio_send_hci_cmd(HCI_COMMAND_PKT, cmd, sizeof(cmd),
		event, sizeof(event),
		WOBLE_COMP_EVENT_TIMO, 1);
	if (ret < 0)
		pr_info("%s: end ret %d", __func__, ret);
	else
		ret = 0;

	pr_info("%s: end ret=%d", __func__, ret);
	return ret;
}


/**
 * Set APCF manufacturer data and filter parameter
 */
static int btmtk_sdio_set_Woble_APCF(void)
{
	int ret = -1;
	int i = 0;
	u8 manufactur_data[] = { 0x57, 0xfd, 0x27, 0x06, 0x00, 0x5a,
		0x46, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x43, 0x52, 0x4B, 0x54, 0x4D,
		0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
	u8 event_complete[] = { 0x0e, 0x07, 0x01, 0x57, 0xfd};

	pr_debug("%s: begin", __func__);
	if (!g_card) {
		pr_info("%s: g_card is NULL, return -1", __func__);
		return -1;
	}

	pr_debug("%s: g_card->woble_setting_apcf[0].length %d",
		__func__, g_card->woble_setting_apcf[0].length);

	/* start to send apcf cmd from woble setting  file */
	if (g_card->woble_setting_apcf[0].length) {
		for (i = 0; i < WOBLE_SETTING_COUNT; i++) {
			if (!g_card->woble_setting_apcf[i].length)
				continue;

			pr_info("%s: g_data->woble_setting_apcf_fill_mac[%d].content[0] = 0x%02x",
				__func__, i,
				g_card->woble_setting_apcf_fill_mac[i].content[0]);
			pr_info("%s: g_data->woble_setting_apcf_fill_mac_location[%d].length = %d",
				__func__, i,
				g_card->woble_setting_apcf_fill_mac_location[i].length);

			if ((g_card->woble_setting_apcf_fill_mac[i].content[0] == 1) &&
				g_card->woble_setting_apcf_fill_mac_location[i].length) {
				/* need add BD addr to apcf cmd */
				memcpy(g_card->woble_setting_apcf[i].content +
					(*g_card->woble_setting_apcf_fill_mac_location[i].content),
					g_card->bdaddr, BD_ADDRESS_SIZE);
				pr_info("%s: apcf %d ,add mac to location %d",
					__func__, i,
					(*g_card->woble_setting_apcf_fill_mac_location[i].content));
			}

			pr_info("%s: send APCF %d", __func__, i);
			BTSDIO_INFO_RAW(g_card->woble_setting_apcf[i].content, g_card->woble_setting_apcf[i].length,
				"woble_setting_apcf");

			ret = btmtk_sdio_send_hci_cmd(HCI_COMMAND_PKT, g_card->woble_setting_apcf[i].content,
				g_card->woble_setting_apcf[i].length,
				event_complete, sizeof(event_complete), WOBLE_COMP_EVENT_TIMO, 1);

			if (ret < 0) {
				pr_info("%s: apcf %d error ret %d", __func__, i, ret);
				return ret;
			}

		}
	} else { /* use default */
		pr_info("%s: use default manufactur data", __func__);
		memcpy(manufactur_data + 9, g_card->bdaddr, BD_ADDRESS_SIZE);
		BTSDIO_DEBUG_RAW(manufactur_data, (unsigned int)sizeof(manufactur_data),
						"send manufactur_data ");

		ret = btmtk_sdio_send_hci_cmd(HCI_COMMAND_PKT, manufactur_data,
				sizeof(manufactur_data),
				event_complete, sizeof(event_complete), WOBLE_COMP_EVENT_TIMO, 1);
		if (ret < 0) {
			pr_info("%s: manufactur_data error ret %d", __func__, ret);
			return ret;
		}

		ret = btmtk_sdio_set_Woble_APCF_filter_parameter();
	}

	pr_info("%s: end ret=%d", __func__, ret);
	return ret;
}



static int btmtk_sdio_send_woble_settings(struct woble_setting_struct *settings_cmd,
	struct woble_setting_struct *settings_event, char *message)
{
	int ret = -1;
	int i = 0;

	pr_info("%s: %s length %d",
		__func__, message, settings_cmd->length);
	if (g_card->woble_setting_radio_on[0].length) {
		for (i = 0; i < WOBLE_SETTING_COUNT; i++) {
			if (settings_cmd[i].length) {
				pr_info("%s: send %s %d", __func__, message, i);
				BTSDIO_INFO_RAW(settings_cmd[i].content,
					settings_cmd[i].length, "Raw");

				ret = btmtk_sdio_send_hci_cmd(HCI_COMMAND_PKT, settings_cmd[i].content,
				settings_cmd[i].length,
				settings_event[i].content,
				settings_event[i].length, WOBLE_COMP_EVENT_TIMO, 1);

				if (ret) {
					pr_info("%s: %s %d return error",
							__func__, message, i);
					return ret;
				}
			}
		}
	}
	return ret;
}
static int btmtk_sdio_send_unify_woble_suspend_default_cmd(void)
{
	int ret = 0;	/* if successful, 0 */
	/* Turn off WOBLE, FW go into low power mode only */
	u8 cmd[] = { 0xC9, 0xFC, 0x14, 0x01, 0x20, 0x02, 0x00, 0x00,
		0x02, 0x01, 0x00, 0x05, 0x10, 0x01, 0x00, 0x40, 0x06,
		0x02, 0x40, 0x5A, 0x02, 0x41, 0x0F };
	/*u8 status[] = { 0x0F, 0x04, 0x00, 0x01, 0xC9, 0xFC };*/
	u8 event[] = { 0xE6, 0x02, 0x08, 0x00 };

	pr_debug("%s: begin", __func__);

	ret = btmtk_sdio_send_hci_cmd(HCI_COMMAND_PKT, cmd,
				sizeof(cmd),
				event, sizeof(event), WOBLE_COMP_EVENT_TIMO, 1);
	if (ret)
		pr_info("%s: comp_event return error(%d)", __func__, ret);

	return ret;
}
/**
 * Set APCF manufacturer data and filter parameter
 *
 * WoBLE test command(TCI_TRIGGER_GPIO, 0xFD77) define:
 * b3 GPIO pin (1 or 9)
 * b4 active mode (0: low active, 1: high active)
 * b5 duration (slots)
 *
 */
static int btmtk_sdio_set_Woble_radio_off(u8 is_suspend)
{
	int ret = -1;
	u8 cmd[] = { 0x77, 0xFD, 0x03, 0x01, 0x00, 0xA0 };

	if (is_suspend) {
		pr_debug("%s: g_data->woble_setting_radio_off[0].length %d",
				__func__,
				g_card->woble_setting_radio_off[0].length);
		pr_debug("%s: g_card->woble_setting_radio_off_comp_event[0].length %d",
				__func__,
				g_card->woble_setting_radio_off_comp_event[0].length);

		if (g_card->woble_setting_radio_off[0].length) {
			if (g_card->woble_setting_radio_off_comp_event[0].length &&
					is_support_unify_woble(g_card)) {
				ret = btmtk_sdio_send_woble_settings(g_card->woble_setting_radio_off,
						g_card->woble_setting_radio_off_comp_event,
						"radio off");
				if (ret) {
					pr_info("%s: radio off error", __func__);
					return ret;
				}
			} else
				pr_info("%s: woble_setting_radio_off length is %d",
					__func__,
					g_card->woble_setting_radio_off[0].length);
		} else {/* use default */
			pr_info("%s: use default radio off cmd", __func__);
			ret = btmtk_sdio_send_unify_woble_suspend_default_cmd();
		}
	} else {
		pr_info("%s: begin", __func__);

		ret = btmtk_sdio_send_hci_cmd(HCI_COMMAND_PKT, cmd, sizeof(cmd),
					NULL, 0, WOBLE_COMP_EVENT_TIMO, 1);
		if (ret)
			pr_info("%s: comp_event return error(%d)",
				__func__, ret);
	}
	pr_info("%s: end ret=%d", __func__, ret);
	return ret;
}

static int btmtk_sdio_handle_entering_WoBLE_state(u8 is_suspend)
{
	int ret = -1;

	pr_debug("%s: begin", __func__);

	if (g_card->dongle_state != BT_SDIO_DONGLE_STATE_POWER_ON) {
		if (!g_card->bt_cfg.support_woble_for_bt_disable) {
			pr_info("%s: BT is off, not support WoBLE\n", __func__);
			return 0;
		}

		if (btmtk_sdio_bt_set_power(1)) {
			pr_info("%s power on failed\n", __func__);
			return -EIO;
		}
		g_card->dongle_state = BT_SDIO_DONGLE_STATE_POWER_ON_FOR_WOBLE;
	} else {
		g_card->dongle_state = BT_SDIO_DONGLE_STATE_WOBLE;
	}

	if (is_support_unify_woble(g_card)) {
		if (is_suspend) {
			ret = btmtk_sdio_send_get_vendor_cap();
			if (ret < 0) {
				pr_info("%s: btmtk_sdio_send_get_vendor_cap fail ret = %d",
					__func__, ret);
				goto Finish;
			}

			ret = btmtk_sdio_send_read_BDADDR_cmd();
			if (ret < 0) {
				pr_info("%s: btmtk_sdio_send_read_BDADDR_cmd fail ret = %d",
					__func__, ret);
				goto Finish;
			}

			ret = btmtk_sdio_set_Woble_APCF();
			if (ret < 0) {
				pr_info("%s: btmtk_sdio_set_Woble_APCF fail %d",
					__func__, ret);
				goto Finish;
			}
		}
		ret = btmtk_sdio_set_Woble_radio_off(is_suspend);
		if (ret < 0) {
			pr_info("%s: btmtk_sdio_set_Woble_radio_off return fail %d", __func__, ret);
			goto Finish;
		}
	}

Finish:
	if (ret)
		btmtk_sdio_woble_wake_lock(g_card);

	pr_info("%s: end", __func__);
	return ret;
}

static int btmtk_sdio_send_leave_woble_suspend_cmd(void)
{
	int ret = 0;	/* if successful, 0 */
	u8 cmd[] = { 0xC9, 0xFC, 0x05, 0x01, 0x21, 0x02, 0x00, 0x00 };
	u8 comp_event[] = { 0xe6, 0x02, 0x08, 0x01 };

	BTSDIO_DEBUG_RAW(cmd, (unsigned int)sizeof(cmd), "cmd ");
	ret = btmtk_sdio_send_hci_cmd(HCI_COMMAND_PKT, cmd,	sizeof(cmd),
				comp_event, sizeof(comp_event), WOBLE_COMP_EVENT_TIMO, 1);

	if (ret < 0) {
		pr_info("%s: failed(%d)", __func__, ret);
	} else {
		pr_info("%s: OK", __func__);
		ret = 0;
	}
	return ret;
}
static int btmtk_sdio_del_Woble_APCF_inde(void)
{
	int ret = -1;
	u8 cmd[] = { 0x57, 0xfd, 0x03, 0x01, 0x01, 0x5a };
	u8 event[] = { 0x0e, 0x07, 0x01, 0x57, 0xfd, 0x00, 0x01, /* 00, 63 */ };

	pr_debug("%s", __func__);
	ret = btmtk_sdio_send_hci_cmd(HCI_COMMAND_PKT, cmd, sizeof(cmd),
		event, sizeof(event), WOBLE_COMP_EVENT_TIMO, 1);

	if (ret < 0)
		pr_info("%s: Got error %d", __func__, ret);

	pr_info("%s end ret = %d", __func__, ret);
	return ret;
}

static int btmtk_sdio_handle_leaving_WoBLE_state(void)
{
	int ret = -1;

	pr_debug("%s: begin", __func__);

	if (g_card == NULL) {
		pr_info("%s: g_card is NULL return", __func__);
		return 0;
	}

	if (!is_support_unify_woble(g_card)) {
		pr_info("%s: do nothing", __func__);
		return 0;
	}

	if (g_card->woble_setting_radio_on[0].length &&
		g_card->woble_setting_radio_on_comp_event[0].length &&
		g_card->woble_setting_apcf_resume[0].length &&
		g_card->woble_setting_apcf_resume_event[0].length) {
			/* start to send radio off cmd from woble setting file */
		ret = btmtk_sdio_send_woble_settings(g_card->woble_setting_radio_on,
			g_card->woble_setting_radio_on_comp_event, "radio on");
		if (ret) {
			pr_info("%s: woble radio on error", __func__);
			goto finish;
		}

		ret = btmtk_sdio_send_woble_settings(g_card->woble_setting_apcf_resume,
			g_card->woble_setting_apcf_resume_event, "apcf resume");
		if (ret) {
			pr_info("%s: apcf resume error", __func__);
			goto finish;
		}

	} else { /* use default */
		ret = btmtk_sdio_send_leave_woble_suspend_cmd();
		if (ret) {
			pr_info("%s: radio on error", __func__);
			goto finish;
		}

		ret = btmtk_sdio_del_Woble_APCF_inde();
		if (ret) {
			pr_info("%s: del apcf index error", __func__);
			goto finish;
		}
	}

finish:
	if (g_card->dongle_state == BT_SDIO_DONGLE_STATE_POWER_ON_FOR_WOBLE) {
		if (btmtk_sdio_bt_set_power(0)) {
			pr_info("%s power off failed\n", __func__);
			return -EIO;
		}
	} else {
		g_card->dongle_state = BT_SDIO_DONGLE_STATE_POWER_ON;
	}

	pr_info("%s: end", __func__);
	return ret;
}

static int btmtk_sdio_send_apcf_reserved(void)
{
	int ret = -1;
	u8 reserve_apcf_cmd_7668[] = { 0x5C, 0xFC, 0x01, 0x0A };
	u8 reserve_apcf_event_7668[] = { 0x0e, 0x06, 0x01, 0x5C, 0xFC, 0x00 };

	u8 reserve_apcf_cmd_7663[] = { 0x85, 0xFC, 0x01, 0x0A };
	u8 reserve_apcf_event_7663[] = { 0x0e, 0x06, 0x01, 0x85, 0xFC, 0x00, 0x0A, 0x08};

	if (g_card->func->device == 0x7668)
		ret = btmtk_sdio_send_hci_cmd(HCI_COMMAND_PKT,
			reserve_apcf_cmd_7668, sizeof(reserve_apcf_cmd_7668),
			reserve_apcf_event_7668, sizeof(reserve_apcf_event_7668),
			WOBLE_COMP_EVENT_TIMO, 1);
	else if (g_card->func->device == 0x7663)
		ret = btmtk_sdio_send_hci_cmd(HCI_COMMAND_PKT,
			reserve_apcf_cmd_7663, sizeof(reserve_apcf_cmd_7663),
			reserve_apcf_event_7663, sizeof(reserve_apcf_event_7663),
			WOBLE_COMP_EVENT_TIMO, 1);
	else
		pr_info("%s: not support for 0x%x\n", __func__, g_card->func->device);

	pr_info("%s: ret %d\n", __func__, ret);
	return ret;
}

static int btmtk_sdio_suspend(struct device *dev)
{
	struct sdio_func *func = dev_to_sdio_func(dev);
	int ret = 0;
	mmc_pm_flag_t pm_flags;

	pr_info("%s begin\n", __func__);

	if (g_card == NULL) {
		pr_info("%s: g_card is NULL return", __func__);
		return 0;
	}

	if ((g_card->suspend_count++)) {
		pr_info("%s: Has suspended. suspend_count: %d", __func__, g_card->suspend_count);
		pr_info("%s: end", __func__);
		return 0;
	}
	pr_debug("%s start to send DRIVER_OWN\n", __func__);
	ret = btmtk_sdio_set_own_back(DRIVER_OWN);
	if (ret)
		pr_info("%s set driver own fail\n", __func__);

	if (!is_support_unify_woble(g_card))
		pr_info("%s: no support", __func__);
	else
		btmtk_sdio_handle_entering_WoBLE_state(1);

	if (g_card->bt_cfg.support_woble_by_eint) {
		if (g_card->wobt_irq != 0 && atomic_read(&(g_card->irq_enable_count)) == 0) {
			pr_info("%s:enable BT IRQ:%d\n", __func__, g_card->wobt_irq);
			irq_set_irq_wake(g_card->wobt_irq, 1);
			enable_irq(g_card->wobt_irq);
			atomic_inc(&(g_card->irq_enable_count));
		} else
			pr_info("%s:irq_enable count:%d\n", __func__, atomic_read(&(g_card->irq_enable_count)));
	}

	ret = btmtk_sdio_set_own_back(FW_OWN);
	if (ret)
		pr_info("%s set fw own fail\n", __func__);

	if (func) {
		pm_flags = sdio_get_host_pm_caps(func);
		if (!(pm_flags & MMC_PM_KEEP_POWER)) {
			pr_info("%s:%s cannot remain alive while suspended(0x%x)\n",
				__func__, sdio_func_id(func), pm_flags);
		}

		pm_flags = MMC_PM_KEEP_POWER;
		ret = sdio_set_host_pm_flags(func, pm_flags);
		if (ret) {
			pr_info("%s: set flag 0x%x err %d\n", __func__, pm_flags, (int)ret);
			return -ENOSYS;
		}
	} else {
		pr_info("sdio_func is not specified\n");
		return 0;
	}

	return 0;
}

static int btmtk_sdio_resume(struct device *dev)
{
	u8 ret = 0;

	if (g_card == NULL) {
		pr_info("%s: g_card is NULL return", __func__);
		return 0;
	}

	g_card->suspend_count--;
	if (g_card->suspend_count) {
		pr_info("%s: data->suspend_count %d, return 0", __func__, g_card->suspend_count);
		return 0;
	}

	if (g_card->bt_cfg.support_woble_by_eint) {
		if (g_card->wobt_irq != 0 && atomic_read(&(g_card->irq_enable_count)) == 1) {
			pr_info("%s:disable BT IRQ:%d\n", __func__, g_card->wobt_irq);
			atomic_dec(&(g_card->irq_enable_count));
			disable_irq_nosync(g_card->wobt_irq);
		} else
			pr_info("%s:irq_enable count:%d\n", __func__, atomic_read(&(g_card->irq_enable_count)));
	}

	ret = btmtk_sdio_handle_leaving_WoBLE_state();
	if (ret)
		pr_info("%s: btmtk_sdio_handle_leaving_WoBLE_state return fail  %d", __func__, ret);

	pr_info("%s end\n", __func__);
	return 0;
}

static const struct dev_pm_ops btmtk_sdio_pm_ops = {
	.suspend = btmtk_sdio_suspend,
	.resume = btmtk_sdio_resume,
};

static struct sdio_driver bt_mtk_sdio = {
	.name = "btmtk_sdio",
	.id_table = btmtk_sdio_ids,
	.probe = btmtk_sdio_probe,
	.remove = btmtk_sdio_remove,
	.drv = {
		.owner = THIS_MODULE,
		.pm = &btmtk_sdio_pm_ops,
	}
};

static void btmtk_sdio_L0_hook_new_probe(sdio_card_probe pFn_Probe)
{
	if (pFn_Probe == NULL) {
		pr_info(L0_RESET_TAG "%s new probe is NULL", __func__);
		return;
	}
	bt_mtk_sdio.probe = pFn_Probe;
}

static int btmtk_clean_queue(void)
{
	struct sk_buff *skb = NULL;

	LOCK_UNSLEEPABLE_LOCK(&(metabuffer.spin_lock));
	if (!skb_queue_empty(&g_priv->adapter->fops_queue)) {
		pr_info("%s clean data in fops_queue\n", __func__);
		do {
			skb = skb_dequeue(&g_priv->adapter->fops_queue);
			if (skb == NULL) {
				pr_info("%s skb=NULL error break\n", __func__);
				break;
			}

			kfree_skb(skb);
		} while (!skb_queue_empty(&g_priv->adapter->fops_queue));
	}
	UNLOCK_UNSLEEPABLE_LOCK(&(metabuffer.spin_lock));
	return 0;
}

static int btmtk_fops_open(struct inode *inode, struct file *file)
{
	pr_info("%s begin\n", __func__);

	pr_info("%s: Mediatek Bluetooth SDIO driver ver %s\n", __func__, VERSION);
	pr_info("%s: major %d minor %d (pid %d), probe counter: %d\n",
			__func__, imajor(inode), iminor(inode), current->pid, probe_counter);

	if (!probe_ready) {
		pr_info("%s probe_ready is %d return\n",
			__func__, probe_ready);
		return -EFAULT;
	}

	metabuffer.read_p = 0;
	metabuffer.write_p = 0;
#if 0 /* Move to btmtk_sdio_probe() */
	spin_lock_init(&(metabuffer.spin_lock.lock));
	pr_info("%s spin_lock_init end\n", __func__);
#endif
	if (g_priv == NULL) {
		pr_info("%s g_priv is NULL\n", __func__);
		return -ENOENT;
	}

	if (g_priv->btmtk_dev.reset_progress) {
		pr_info("%s reset_progress is %d\n",
			__func__, g_priv->btmtk_dev.reset_progress);
		return -ENOENT;
	}

	if (g_priv->adapter == NULL) {
		pr_info("%s g_priv->adapter is NULL\n", __func__);
		return -ENOENT;
	}

	btmtk_usb_hci_snoop_init();

	btmtk_sdio_send_init_cmds(g_card);

	if (g_card) {
		if (is_support_unify_woble(g_card))
			btmtk_sdio_send_apcf_reserved();
	} else
		pr_info("%s g_card is NULL\n", __func__);

	btmtk_clean_queue();

	if (g_priv)
		g_priv->adapter->fops_mode = true;

	need_reset_stack = 0;
	need_reopen = 0;
	stereo_irq = -1;
	pr_info("%s fops_mode=%d end\n", __func__, g_priv->adapter->fops_mode);
	return 0;
}

#if 0
static int btmtk_sdio_send_hci_reset(void)
{
	int ret = 0;
	u8 event[] = { 0x0e, 0x04, 0x01, 0x03, 0x0c, 0x00 };
	u8 cmd[] = { 0x03, 0x0c, 0x00 };

	pr_info("%s begin\n", __func__);
	ret = btmtk_sdio_send_hci_cmd(HCI_COMMAND_PKT, cmd,
		sizeof(cmd),
		event, sizeof(event), COMP_EVENT_TIMO, 1);
	if (ret != 0)
		pr_info("%s ret = %d\n", __func__, ret);
	return ret;
}
#endif

static int btmtk_fops_close(struct inode *inode, struct file *file)
{
	pr_info("%s begin\n", __func__);

	pr_info("%s: Mediatek Bluetooth SDIO driver ver %s\n", __func__, VERSION);
	pr_info("%s: major %d minor %d (pid %d), probe counter: %d\n",
			__func__, imajor(inode), iminor(inode), current->pid, probe_counter);

	if (!probe_ready) {
		pr_info("%s probe_ready is %d return\n",
			__func__, probe_ready);
		return -EFAULT;
	}

	if (g_priv && g_priv->adapter)
		g_priv->adapter->fops_mode = false;

	if (g_card->dongle_state != BT_SDIO_DONGLE_STATE_WOBLE)
		btmtk_sdio_send_deinit_cmds();

	btmtk_stereo_unreg_irq();
	btmtk_clean_queue();
	need_reopen = 0;

	if (g_priv && g_priv->adapter)
		pr_info("%s fops_mode=%d end\n", __func__, g_priv->adapter->fops_mode);
	else
		pr_info("%s end g_priv or adapter is null\n", __func__);
	return 0;
}

ssize_t btmtk_fops_write(struct file *filp, const char __user *buf,
			size_t count, loff_t *f_pos)
{
	int retval = 0;
	struct sk_buff *skb = NULL;
	u32 crAddr = 0, crValue = 0, crMask = 0;
	static u8 waiting_for_hci_without_packet_type; /* INITIALISED_STATIC: do not initialise statics to 0 */
	static u8 hci_packet_type = 0xff;
	u32 copy_size = 0;
	unsigned long c_result = 0;

	if (!probe_ready) {
		pr_info("%s probe_ready is %d return\n",
			__func__, probe_ready);
		return -EFAULT;
	}

	if (g_priv == NULL) {
		pr_info("%s g_priv is NULL\n", __func__);
		return -EFAULT;
	}

	if (g_priv->adapter->fops_mode == 0) {
		pr_info("%s fops_mode is 0\n", __func__);
		return -EFAULT;
	}

	if (need_reopen) {
		pr_info("%s: need_reopen (%d)!", __func__, need_reopen);
		return -EFAULT;
	}
#if 0
	pr_info("%s : (%d) %02X %02X %02X %02X "
			%"02X %02X %02X %02X\n",
			__func__, (int)count,
			buf[0], buf[1], buf[2], buf[3],
			buf[4], buf[5], buf[6], buf[7]);

	pr_info("%s print write data", __func__);
	if (count > 10)
		pr_info("  %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
				buf[0], buf[1], buf[2], buf[3], buf[4],
				buf[5], buf[6], buf[7], buf[8], buf[9]);
	else {
		for (i = 0; i < count; i++)
			pr_info("%d %02X", i, buf[i]);
	}
#endif
	if (count > 0) {
		memset(userbuf, 0, MTK_TXDATA_SIZE);
		c_result = copy_from_user(userbuf, buf, count);
	} else {
		pr_info("%s: target packet length:%zu is not allowed", __func__, count);
		return -EFAULT;
	}

	if (c_result != 0) {
		pr_info("%s copy_from_user failed!\n", __func__);
		return -EFAULT;
	}

	if (userbuf[0] == 0x7 && waiting_for_hci_without_packet_type == 0) {
		/* write CR */
		if (count < 15) {
			pr_info("%s count=%zd less than 15, error\n",
				__func__, count);
			return -EFAULT;
		}

		crAddr = (userbuf[3]&0xff) + ((userbuf[4]&0xff)<<8)
			+ ((userbuf[5]&0xff)<<16) + ((userbuf[6]&0xff)<<24);
		crValue = (userbuf[7]&0xff) + ((userbuf[8]&0xff)<<8)
			+ ((userbuf[9]&0xff)<<16) + ((userbuf[10]&0xff)<<24);
		crMask = (userbuf[11]&0xff) + ((userbuf[12]&0xff)<<8)
			+ ((userbuf[13]&0xff)<<16) + ((userbuf[14]&0xff)<<24);

		pr_info("%s crAddr=0x%08x crValue=0x%08x crMask=0x%08x\n",
			__func__, crAddr, crValue, crMask);
		crValue &= crMask;

		pr_info("%s write crAddr=0x%08x crValue=0x%08x\n", __func__,
			crAddr, crValue);
		btmtk_sdio_writel(crAddr, crValue);
		retval = count;
	} else if (userbuf[0] == 0x8 && waiting_for_hci_without_packet_type == 0) {
		/* read CR */
		if (count < 16) {
			pr_info("%s count=%zd less than 15, error\n",
				__func__, count);
			return -EFAULT;
		}

		crAddr = (userbuf[3]&0xff) + ((userbuf[4]&0xff)<<8) +
			((userbuf[5]&0xff)<<16) + ((userbuf[6]&0xff)<<24);
		crMask = (userbuf[11]&0xff) + ((userbuf[12]&0xff)<<8) +
			((userbuf[13]&0xff)<<16) + ((userbuf[14]&0xff)<<24);

		btmtk_sdio_readl(crAddr, &crValue);
		pr_info("%s read crAddr=0x%08x crValue=0x%08x crMask=0x%08x\n",
				__func__, crAddr, crValue, crMask);
		retval = count;
	} else {
		if (waiting_for_hci_without_packet_type == 1 && count == 1) {
			pr_info("%s: Waiting for hci_without_packet_type, but receive data count is 1!", __func__);
			pr_info("%s: Treat this packet as packet_type", __func__);
			memcpy(&hci_packet_type, &userbuf[0], 1);
			waiting_for_hci_without_packet_type = 1;
			retval = 1;
			goto OUT;
		}

		if (waiting_for_hci_without_packet_type == 0) {
			if (count == 1) {
				memcpy(&hci_packet_type, &userbuf[0], 1);
				waiting_for_hci_without_packet_type = 1;
				retval = 1;
				goto OUT;
			}
		}

		if (waiting_for_hci_without_packet_type) {
			copy_size = count + 1;
			skb = bt_skb_alloc(copy_size-1, GFP_ATOMIC);
			if (skb == NULL) {
				pr_info("skb is null\n");
				retval = -ENOMEM;
				goto OUT;
			}
			bt_cb(skb)->pkt_type = hci_packet_type;
			memcpy(&skb->data[0], &userbuf[0], copy_size-1);
		} else {
			copy_size = count;
			skb = bt_skb_alloc(copy_size-1, GFP_ATOMIC);
			if (skb == NULL) {
				pr_info("skb is null\n");
				retval = -ENOMEM;
				goto OUT;
			}
			bt_cb(skb)->pkt_type = userbuf[0];
			memcpy(&skb->data[0], &userbuf[1], copy_size-1);
		}

		skb->len = copy_size-1;
		skb_queue_tail(&g_priv->adapter->tx_queue, skb);

		if (bt_cb(skb)->pkt_type == HCI_COMMAND_PKT) {
			u8 fw_assert_cmd[] = { 0x6F, 0xFC, 0x05, 0x01, 0x02, 0x01, 0x00, 0x08 };
			u8 reset_cmd[] = { 0x03, 0x0C, 0x00 };
			u8 read_ver_cmd[] = { 0x01, 0x10, 0x00 };

			if (skb->len == sizeof(fw_assert_cmd) &&
				!memcmp(&skb->data[0], fw_assert_cmd, sizeof(fw_assert_cmd)))
				pr_info("%s: Donge FW Assert Triggered by upper layer\n", __func__);
			else if (skb->len == sizeof(reset_cmd) &&
				!memcmp(&skb->data[0], reset_cmd, sizeof(reset_cmd)))
				pr_info("%s: got command: 0x03 0C 00 (HCI_RESET)\n", __func__);
			else if (skb->len == sizeof(read_ver_cmd) &&
				!memcmp(&skb->data[0], read_ver_cmd, sizeof(read_ver_cmd)))
				pr_info("%s: got command: 0x01 10 00 (READ_LOCAL_VERSION)\n", __func__);
		}

		wake_up_interruptible(&g_priv->main_thread.wait_q);

		retval = copy_size;

		if (waiting_for_hci_without_packet_type) {
			hci_packet_type = 0xff;
			waiting_for_hci_without_packet_type = 0;
			if (retval > 0)
				retval -= 1;
		}
	}

OUT:

	pr_debug("%s end\n", __func__);
	return retval;
}

ssize_t btmtk_fops_read(struct file *filp, char __user *buf,
			size_t count, loff_t *f_pos)
{
	struct sk_buff *skb = NULL;
	int copyLen = 0;
	u8 hwerr_event[] = { 0x04, 0x10, 0x01, 0xff };
	static int send_hw_err_event_count;

	if (g_priv == NULL) {
		pr_info("%s g_priv is NULL\n", __func__);
		return -EFAULT;
	}

	if ((g_priv->adapter->fops_mode == 0) && (need_reset_stack == 0)) {
		pr_info("%s fops_mode is 0\n", __func__);
		return -EFAULT;
	}

	if (need_reset_stack == 1) {
		pr_info("%s: Reset BT stack", __func__);
		pr_info("%s: go if send_hw_err_event_count %d", __func__, send_hw_err_event_count);
		if (send_hw_err_event_count < sizeof(hwerr_event)) {
			if (count < (sizeof(hwerr_event) - send_hw_err_event_count)) {
				copyLen = count;
				pr_info("call wake_up_interruptible");
				wake_up_interruptible(&inq);
			} else
				copyLen = (sizeof(hwerr_event) - send_hw_err_event_count);
			pr_info("%s: in if copyLen = %d", __func__, copyLen);
			if (copy_to_user(buf, hwerr_event + send_hw_err_event_count, copyLen)) {
				pr_info("send_hw_err_event_count %d copy to user fail, count = %d, go out",
					send_hw_err_event_count, copyLen);
				copyLen = -EFAULT;
				goto OUT;
			}
			send_hw_err_event_count += copyLen;
			pr_info("%s: in if send_hw_err_event_count = %d", __func__, send_hw_err_event_count);
			if (send_hw_err_event_count >= sizeof(hwerr_event)) {
				send_hw_err_event_count  = 0;
				pr_info("%s: set need_reset_stack=0", __func__);
				need_reset_stack = 0;
				need_reopen = 1;
				kill_fasync(&fasync, SIGIO, POLL_IN);
			}
			pr_info("%s: set call up", __func__);
			goto OUT;
		} else {
			pr_info("%s: xx set copyLen = -EFAULT", __func__);
			copyLen = -EFAULT;
			goto OUT;
		}
	}

	if (get_hci_reset == 1) {
		pr_info("%s get reset complete and set audio!\n", __func__);
		btmtk_sdio_send_audio_slave();
		if (g_card->pa_setting != -1)
			btmtk_set_pa(g_card->pa_setting);
		if (g_card->duplex_setting != -1)
			btmtk_set_duplex(g_card->duplex_setting);
		btmtk_set_eeprom2ctrler((uint8_t *)g_card->bin_file_buffer,
					g_card->bin_file_size,
					g_card->func->device);

		get_hci_reset = 0;
	}

	LOCK_UNSLEEPABLE_LOCK(&(metabuffer.spin_lock));
	if (skb_queue_empty(&g_priv->adapter->fops_queue)) {
		/* if (filp->f_flags & O_NONBLOCK) { */
		if (metabuffer.write_p == metabuffer.read_p) {
			UNLOCK_UNSLEEPABLE_LOCK(&(metabuffer.spin_lock));
			return 0;
		}
	}

	if (need_reset_stack == 1) {
		kill_fasync(&fasync, SIGIO, POLL_IN);
		need_reset_stack = 0;
		pr_info("%s Call kill_fasync and set reset_stack 0\n",
			__func__);
		UNLOCK_UNSLEEPABLE_LOCK(&(metabuffer.spin_lock));
		return -ENODEV;
	}

	do {
		skb = skb_dequeue(&g_priv->adapter->fops_queue);
		if (skb == NULL) {
			pr_debug("%s skb=NULL break\n", __func__);
			break;
		}

#if 0
		pr_debug("%s pkt_type %d metabuffer.buffer %d",
			__func__, bt_cb(skb)->pkt_type,
			metabuffer.buffer[copyLen]);
#endif

		btmtk_print_buffer_conent(skb->data, skb->len);

		if (btmtk_sdio_push_data_to_metabuffer(&metabuffer, skb->data,
				skb->len, bt_cb(skb)->pkt_type, true) < 0) {
			skb_queue_head(&g_priv->adapter->fops_queue, skb);
			break;
		}
		kfree_skb(skb);
	} while (!skb_queue_empty(&g_priv->adapter->fops_queue));
	UNLOCK_UNSLEEPABLE_LOCK(&(metabuffer.spin_lock));

	return btmtk_sdio_pull_data_from_metabuffer(&metabuffer, buf, count);

OUT:
	return copyLen;
}

static int btmtk_fops_fasync(int fd, struct file *file, int on)
{
	pr_info("%s: fd = 0x%X, flag = 0x%X\n", __func__, fd, on);
	return fasync_helper(fd, file, on, &fasync);
}

unsigned int btmtk_fops_poll(struct file *filp, poll_table *wait)
{
	unsigned int mask = 0;

	if (g_priv == NULL) {
		pr_info("%s g_priv is NULL\n", __func__);
		return -ENODEV;
	}

	poll_wait(filp, &inq, wait);

	if (metabuffer.write_p != metabuffer.read_p || need_reset_stack)
		mask |= (POLLIN | POLLRDNORM);

	if (!skb_queue_empty(&g_priv->adapter->fops_queue)) {
		mask |= (POLLIN | POLLRDNORM);
		/* pr_info("%s poll done\n", __func__); */
	}

	mask |= (POLLOUT | POLLWRNORM);

	/* pr_info("%s poll mask 0x%x\n", __func__,mask); */
	return mask;
}

static long btmtk_fops_unlocked_ioctl(struct file *filp,
				unsigned int cmd, unsigned long arg)
{
	u32 ret = 0;
	struct bt_stereo_para stereo_para;
	struct sk_buff *skb = NULL;
	u8 set_btclk[] = {0x01, 0x02, 0xFD, 0x0B,
		0x00, 0x00,			/* Handle */
		0x00,				/* Method */
						/* bit0~3 = 0x0:CVSD remove, 0x1:GPIO, 0x2:In-band with transport*/
						/* bit4~7 = 0x0:Shared memory, 0x1:auto event */
		0x00, 0x00, 0x00, 0x00,		/* Period = value * 0.625ms */
		0x09,				/* GPIO num - 0x01:BGF_INT_B, 0x09:GPIO0 */
		0x01,				/* trigger mode - 0x00:Low, 0x01:High */
		0x00, 0x00};			/* active slots = value * 0.625ms */


	switch (cmd) {
	case BTMTK_IOCTL_STEREO_GET_CLK:
		pr_debug("%s: BTMTK_IOCTL_STEREO_GET_CLK cmd\n", __func__);

		LOCK_UNSLEEPABLE_LOCK(&stereo_spin_lock);
		if (copy_to_user((struct bt_stereo_clk __user *)arg, &stereo_clk, sizeof(struct bt_stereo_clk)))
			ret = -ENOMEM;
		UNLOCK_UNSLEEPABLE_LOCK(&stereo_spin_lock);
		break;
	case BTMTK_IOCTL_STEREO_SET_PARA:
		pr_debug("%s: BTMTK_IOCTL_STEREO_SET_PARA cmd\n", __func__);
		if (copy_from_user(&stereo_para, (struct bt_stereo_para __user *)arg,
						sizeof(struct bt_stereo_para)))
			return -ENOMEM;

		if (stereo_para.period != 0)
			btmtk_stereo_reg_irq();

		/* Send and check HCI cmd */
		memcpy(&set_btclk[4], &stereo_para.handle, sizeof(stereo_para.handle));
		memcpy(&set_btclk[6], &stereo_para.method, sizeof(stereo_para.method));
		memcpy(&set_btclk[7], &stereo_para.period, sizeof(stereo_para.period));
		memcpy(&set_btclk[13], &stereo_para.active_slots, sizeof(stereo_para.active_slots));
#if SUPPORT_MT7668
		if (is_mt7668(g_card))
			set_btclk[11] = 0x09;
#endif
#if SUPPORT_MT7663
		if (is_mt7663(g_card))
			set_btclk[11] = 0x0B;
#endif

		skb = bt_skb_alloc(sizeof(set_btclk) - 1, GFP_ATOMIC);
		bt_cb(skb)->pkt_type = set_btclk[0];
		memcpy(&skb->data[0], &set_btclk[1], sizeof(set_btclk) - 1);

		skb->len = sizeof(set_btclk) - 1;
		skb_queue_tail(&g_priv->adapter->tx_queue, skb);
		wake_up_interruptible(&g_priv->main_thread.wait_q);

		if (stereo_para.period == 0)
			btmtk_stereo_unreg_irq();
		break;
	default:
		return -EINVAL;
	}
	return ret;
}

static int btmtk_fops_openfwlog(struct inode *inode,
					struct file *file)
{
	if (g_priv == NULL) {
		pr_info("%s: ERROR, g_data is NULL!\n", __func__);
		return -ENODEV;
	}

	pr_info("%s: OK\n", __func__);
	return 0;
}

static int btmtk_fops_closefwlog(struct inode *inode,
					struct file *file)
{
	if (g_priv == NULL) {
		pr_info("%s: ERROR, g_data is NULL!\n", __func__);
		return -ENODEV;
	}

	pr_info("%s: OK\n", __func__);
	return 0;
}

static ssize_t btmtk_fops_readfwlog(struct file *filp,
			char __user *buf,
			size_t count,
			loff_t *f_pos)
{
	struct sk_buff *skb = NULL;
	int copyLen = 0;

	if (g_priv == NULL) {
		pr_info("%s g_priv is NULL\n", __func__);
		return -EFAULT;
	}

	/* picus read a queue, it may occur performace issue */
	LOCK_UNSLEEPABLE_LOCK(&(fwlog_metabuffer.spin_lock));
	if (skb_queue_len(&g_priv->adapter->fwlog_fops_queue))
		skb = skb_dequeue(&g_priv->adapter->fwlog_fops_queue);
	UNLOCK_UNSLEEPABLE_LOCK(&(fwlog_metabuffer.spin_lock));

	if (skb == NULL)
		return 0;

	if (skb->len <= count) {
		if (copy_to_user(buf, skb->data, skb->len)) {
			pr_info("%s: copy_to_user failed!", __func__);
			/* copy_to_user failed, add skb to fwlog_fops_queue */
			skb_queue_head(&g_priv->adapter->fwlog_fops_queue, skb);
			copyLen = -EFAULT;
			goto OUT;
		}
		copyLen = skb->len;
	} else {
		pr_info("%s: socket buffer length error(count: %d, skb.len: %d)", __func__, (int)count, skb->len);
	}
	kfree_skb(skb);
OUT:
	return copyLen;
}

static ssize_t btmtk_fops_writefwlog(
			struct file *filp, const char __user *buf,
			size_t count, loff_t *f_pos)
{
	struct sk_buff *skb = NULL;
	int i = 0, len = 0, ret = -1;
	/*+1 is for i_fwlog_buf[count] = 0, end string byte*/
	int i_fwlog_buf_size = HCI_MAX_COMMAND_BUF_SIZE + 1;
	u8 *i_fwlog_buf = kmalloc(i_fwlog_buf_size, GFP_KERNEL);
	u8 *o_fwlog_buf = kmalloc(HCI_MAX_COMMAND_SIZE, GFP_KERNEL);
	u32 crAddr = 0, crValue = 0;

	if (g_priv == NULL) {
		pr_info("%s g_priv is NULL\n", __func__);
		goto exit;
	}

	if (g_priv->adapter->fops_mode == 0) {
		pr_info("%s fops_mode is 0\n", __func__);
		count = -EFAULT;
		goto exit;
	}

	memset(i_fwlog_buf, 0, i_fwlog_buf_size);
	memset(o_fwlog_buf, 0, HCI_MAX_COMMAND_SIZE);

	if (count > HCI_MAX_COMMAND_BUF_SIZE) {
		pr_info("%s: your command is larger than maximum length, count = %zd\n",
		__func__, count);
		count = -EFAULT;
		goto exit;
	}

	if (copy_from_user(i_fwlog_buf, buf, count) != 0) {
		pr_info("%s copy_from_user failed!\n", __func__);
		count = -EFAULT;
		goto exit;
	}

	i_fwlog_buf[count] = 0;

	if (strstr(i_fwlog_buf, FW_OWN_OFF)) {
		pr_info("%s set FW_OWN_OFF\n", __func__);
		btmtk_sdio_set_no_fw_own(g_priv, true);
		len = count;
		wake_up_interruptible(&g_priv->main_thread.wait_q);
		goto exit;
	}

	if (strstr(i_fwlog_buf, FW_OWN_ON)) {
		pr_info("%s set FW_OWN_ON\n", __func__);
		btmtk_sdio_set_no_fw_own(g_priv, false);
		len = count;
		wake_up_interruptible(&g_priv->main_thread.wait_q);
		goto exit;
	}

	if (strstr(i_fwlog_buf, WOBLE_ON)) {
		pr_info("%s set WOBLE_ON\n", __func__);
		btmtk_sdio_handle_entering_WoBLE_state(0);
		len = count;
		if (g_card->bt_cfg.support_woble_by_eint) {
			if (g_card->wobt_irq != 0 &&
				atomic_read(&(g_card->irq_enable_count)) == 0) {
				pr_info("%s:enable BT IRQ:%d\n",
					__func__, g_card->wobt_irq);
				irq_set_irq_wake(g_card->wobt_irq, 1);
				enable_irq(g_card->wobt_irq);
				atomic_inc(&(g_card->irq_enable_count));
			} else
				pr_info("%s:irq_enable count:%d\n",
					__func__,
					atomic_read(&(g_card->irq_enable_count)));
		}
		goto exit;
	}

	if (strstr(i_fwlog_buf, WOBLE_OFF)) {
		pr_info("%s set WOBLE_OFF\n", __func__);
		if (g_card->bt_cfg.support_woble_by_eint) {
			if (g_card->wobt_irq != 0 &&
				atomic_read(&(g_card->irq_enable_count)) == 1) {
				pr_info("%s:disable BT IRQ:%d\n",
					__func__, g_card->wobt_irq);
				atomic_dec(&(g_card->irq_enable_count));
				disable_irq_nosync(g_card->wobt_irq);
			} else
				pr_info("%s:irq_enable count:%d\n",
					__func__,
					atomic_read(&(g_card->irq_enable_count)));
		}
		btmtk_sdio_send_leave_woble_suspend_cmd();
		len = count;
		goto exit;
	}

	if (strstr(i_fwlog_buf, RELOAD_SETTING)) {
		pr_info("%s set RELOAD_SETTING\n", __func__);
		btmtk_eeprom_bin_file(g_card);
		len = count;
		goto exit;
	}

	/* For btmtk_bluetooth_kpi, EX: echo bperf=1 > /dev/stpbtfwlog */
	if (strcmp(i_fwlog_buf, "bperf=") >= 0) {
		u8 val = *(i_fwlog_buf + strlen("bperf=")) - 48;

		btmtk_bluetooth_kpi = val;
		pr_info("%s: set bluetooth KPI feature(bperf) to %d", __func__, btmtk_bluetooth_kpi);
		goto exit;
	}

	/* hci input command format : echo 01 be fc 01 05 > /dev/stpbtfwlog */
	/* We take the data from index three to end. */
	for (i = 0; i < count; i++) {
		char *pos = i_fwlog_buf + i;
		char temp_str[3] = {'\0'};
		long res = 0;

		if (*pos == ' ' || *pos == '\t' || *pos == '\r' || *pos == '\n') {
			continue;
		} else if (*pos == '0' && (*(pos + 1) == 'x' || *(pos + 1) == 'X')) {
			i++;
			continue;
		} else if (!(*pos >= '0' && *pos <= '9') && !(*pos >= 'A' && *pos <= 'F')
			&& !(*pos >= 'a' && *pos <= 'f')) {
			pr_info("%s: There is an invalid input(%c)", __func__, *pos);
			return -EINVAL;
		}
		temp_str[0] = *pos;
		temp_str[1] = *(pos + 1);
		i++;
		ret = kstrtol(temp_str, 16, &res);
		if (ret == 0)
			o_fwlog_buf[len++] = (u8)res;
		else
			pr_info("%s: Convert %s failed(%d)", __func__, temp_str, ret);
	}

	/*
	 * Receive command from stpbtfwlog, then Sent hci command
	 * to controller
	 */
	pr_debug("%s: hci buff is %02x%02x%02x%02x%02x, length %d\n",
		__func__, o_fwlog_buf[0], o_fwlog_buf[1],
		o_fwlog_buf[2], o_fwlog_buf[3], o_fwlog_buf[4], len);

	/* check HCI command length */
	if (len > HCI_MAX_COMMAND_SIZE) {
		pr_info("%s: your command is larger than maximum length, length = %d\n",
			__func__, len);
		goto exit;
	}

	if (len == 0) {
		pr_debug("%s: command length = %d\n", __func__, len);
		goto exit;
	}

	pr_debug("%s: hci buff is %02x%02x%02x%02x%02x\n",
		__func__, o_fwlog_buf[0], o_fwlog_buf[1],
		o_fwlog_buf[2], o_fwlog_buf[3], o_fwlog_buf[4]);

	switch (o_fwlog_buf[0]) {
	case MTK_HCI_READ_CR_PKT:
		if (len == MTK_HCI_READ_CR_PKT_LENGTH) {
			crAddr = (o_fwlog_buf[1] << 24) + (o_fwlog_buf[2] << 16) +
			(o_fwlog_buf[3] << 8) + (o_fwlog_buf[4]);
			btmtk_sdio_readl(crAddr, &crValue);
			pr_info("%s read crAddr=0x%08x crValue=0x%08x\n",
				__func__, crAddr, crValue);
		} else
			pr_info("%s read length=%d is incorrect, should be %d\n",
				__func__, len, MTK_HCI_READ_CR_PKT_LENGTH);
		break;

	case MTK_HCI_WRITE_CR_PKT:
		if (len == MTK_HCI_WRITE_CR_PKT_LENGTH) {
			crAddr = (o_fwlog_buf[1] << 24) + (o_fwlog_buf[2] << 16) +
			(o_fwlog_buf[3] << 8) + (o_fwlog_buf[4]);
			crValue = (o_fwlog_buf[5] << 24) + (o_fwlog_buf[6] << 16) +
			(o_fwlog_buf[7] << 8) + (o_fwlog_buf[8]);
			pr_info("%s write crAddr=0x%08x crValue=0x%08x\n",
				__func__, crAddr, crValue);
			btmtk_sdio_writel(crAddr, crValue);
		} else
			pr_info("%s write length=%d is incorrect, should be %d\n",
				__func__, len, MTK_HCI_WRITE_CR_PKT_LENGTH);


		break;

	default:
		/*
		 * Receive command from stpbtfwlog, then Sent hci command
		 * to Stack
		 */
		skb = bt_skb_alloc(len - 1, GFP_ATOMIC);
		if (skb == NULL) {
			pr_info("skb is null\n");
			count = -ENOMEM;
			goto exit;
		}
		bt_cb(skb)->pkt_type = o_fwlog_buf[0];
		memcpy(&skb->data[0], &o_fwlog_buf[1], len - 1);
		skb->len = len - 1;
		skb_queue_tail(&g_priv->adapter->tx_queue, skb);
		wake_up_interruptible(&g_priv->main_thread.wait_q);
		break;
	}



	pr_info("%s write end\n", __func__);
exit:
	pr_info("%s exit, length = %d\n", __func__, len);
	kfree(i_fwlog_buf);
	kfree(o_fwlog_buf);
	return count;
}

static unsigned int btmtk_fops_pollfwlog(
			struct file *file, poll_table *wait)
{
	unsigned int mask = 0;

	if (g_priv == NULL) {
		pr_info("%s g_priv is NULL\n", __func__);
		return -ENODEV;
	}

	poll_wait(file, &fw_log_inq, wait);

	if (fwlog_metabuffer.write_p != fwlog_metabuffer.read_p)
		mask |= (POLLIN | POLLRDNORM);

	if (!skb_queue_empty(&g_priv->adapter->fwlog_fops_queue)) {
		mask |= (POLLIN | POLLRDNORM);
		/* pr_info("%s poll done\n", __func__); */
	}

	mask |= (POLLOUT | POLLWRNORM);

	/* pr_info("%s poll mask 0x%x\n", __func__,mask); */
	return mask;
}

static long btmtk_fops_unlocked_ioctlfwlog(
			struct file *filp, unsigned int cmd,
			unsigned long arg)
{
	int retval = 0;

	pr_info("%s: ->\n", __func__);
	if (g_priv == NULL) {
		pr_info("%s: ERROR, g_data is NULL!\n", __func__);
		return -ENODEV;
	}

	return retval;
}

/*============================================================================*/
/* Interface Functions : Proc */
/*============================================================================*/
#define __________________________________________Interface_Function_for_Proc
static int btmtk_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "patch version:%s  driver version:%s\n", fw_version_str, VERSION);
	return 0;
}

static int btmtk_proc_open(struct inode *inode, struct  file *file)
{
	return single_open(file, btmtk_proc_show, NULL);
}

void btmtk_proc_create_new_entry(void)
{
	struct proc_dir_entry *proc_show_entry;

	pr_info("proc initialized");

	g_proc_dir = proc_mkdir("stpbt", 0);
	if (g_proc_dir == 0) {
		pr_info("Unable to creat dir");
		return;
	}
	proc_show_entry =  proc_create("bt_fw_version", 0644, g_proc_dir, &BT_proc_fops);
}


static int BTMTK_major;
static int BT_majorfwlog;
static struct cdev BTMTK_cdev;
static struct cdev BT_cdevfwlog;
static int BTMTK_devs = 1;

static wait_queue_head_t inq;
const struct file_operations BTMTK_fops = {
	.open = btmtk_fops_open,
	.release = btmtk_fops_close,
	.read = btmtk_fops_read,
	.write = btmtk_fops_write,
	.poll = btmtk_fops_poll,
	.unlocked_ioctl = btmtk_fops_unlocked_ioctl,
	.fasync = btmtk_fops_fasync
};

const struct file_operations BT_fopsfwlog = {
	.open = btmtk_fops_openfwlog,
	.release = btmtk_fops_closefwlog,
	.read = btmtk_fops_readfwlog,
	.write = btmtk_fops_writefwlog,
	.poll = btmtk_fops_pollfwlog,
	.unlocked_ioctl = btmtk_fops_unlocked_ioctlfwlog

};

static int BTMTK_init(void)
{
	dev_t devID = MKDEV(BTMTK_major, 0);
	dev_t devIDfwlog = MKDEV(BT_majorfwlog, 1);
	int ret = 0;
	int cdevErr = 0;
	int major = 0;
	int majorfwlog = 0;

	pr_info("BTMTK_init\n");

	g_card = NULL;
	txbuf = NULL;
	rxbuf = NULL;
	userbuf = NULL;
	rx_length = 0;
	fw_dump_file = NULL;
	g_priv = NULL;
	probe_counter = 0;

	btmtk_proc_create_new_entry();

	ret = alloc_chrdev_region(&devID, 0, 1, "BT_chrdev");
	if (ret) {
		pr_info("fail to allocate chrdev\n");
		return ret;
	}

	ret = alloc_chrdev_region(&devIDfwlog, 0, 1, "BT_chrdevfwlog");
	if (ret) {
		pr_info("fail to allocate chrdev\n");
		return ret;
	}

	BTMTK_major = major = MAJOR(devID);
	pr_info("major number:%d\n", BTMTK_major);
	BT_majorfwlog = majorfwlog = MAJOR(devIDfwlog);
	pr_info("BT_majorfwlog number: %d\n", BT_majorfwlog);

	cdev_init(&BTMTK_cdev, &BTMTK_fops);
	BTMTK_cdev.owner = THIS_MODULE;

	cdev_init(&BT_cdevfwlog, &BT_fopsfwlog);
	BT_cdevfwlog.owner = THIS_MODULE;

	cdevErr = cdev_add(&BTMTK_cdev, devID, BTMTK_devs);
	if (cdevErr)
		goto error;

	cdevErr = cdev_add(&BT_cdevfwlog, devIDfwlog, 1);
	if (cdevErr)
		goto error;

	pr_info("%s driver(major %d) installed.\n",
			"BT_chrdev", BTMTK_major);
	pr_info("%s driver(major %d) installed.\n",
			"BT_chrdevfwlog", BT_majorfwlog);

	pBTClass = class_create(THIS_MODULE, "BT_chrdev");
	if (IS_ERR(pBTClass)) {
		pr_info("class create fail, error code(%ld)\n",
			PTR_ERR(pBTClass));
		goto error;
	}

	pBTDev = device_create(pBTClass, NULL, devID, NULL, BT_NODE);
	if (IS_ERR(pBTDev)) {
		pr_info("device create fail, error code(%ld)\n",
			PTR_ERR(pBTDev));
		goto err2;
	}

	pBTDevfwlog = device_create(pBTClass, NULL,
				devIDfwlog, NULL, "stpbtfwlog");
	if (IS_ERR(pBTDevfwlog)) {
		pr_info("device(stpbtfwlog) create fail, error code(%ld)\n",
			PTR_ERR(pBTDevfwlog));
		goto err2;
	}

	pr_info("%s: BT_major %d, BT_majorfwlog %d\n",
			__func__, BTMTK_major, BT_majorfwlog);
	pr_info("%s: devID %d, devIDfwlog %d\n", __func__, devID, devIDfwlog);

	/* init wait queue */
	g_devIDfwlog = devIDfwlog;
	init_waitqueue_head(&(fw_log_inq));
	init_waitqueue_head(&(inq));

	return 0;

 err2:
	if (pBTClass) {
		class_destroy(pBTClass);
		pBTClass = NULL;
	}

 error:
	if (cdevErr == 0)
		cdev_del(&BTMTK_cdev);

	if (ret == 0)
		unregister_chrdev_region(devID, BTMTK_devs);

	return -1;
}

static void BTMTK_exit(void)
{
	dev_t dev = MKDEV(BTMTK_major, 0);
	dev_t devIDfwlog = g_devIDfwlog;

	pr_info("%s\n", __func__);

	if (g_proc_dir != NULL) {
		remove_proc_entry("bt_fw_version", g_proc_dir);
		remove_proc_entry("stpbt", NULL);
		g_proc_dir = NULL;
		pr_info("proc device node and folder removed!!");
	}

	pr_info("%s 5\n", __func__);
	if (pBTDevfwlog) {
		pr_info("%s 6\n", __func__);
		device_destroy(pBTClass, devIDfwlog);
		pBTDevfwlog = NULL;
	}
	pr_info("%s 7\n", __func__);
	if (pBTDev) {
		device_destroy(pBTClass, dev);
		pBTDev = NULL;
	}
	pr_info("%s 8\n", __func__);
	if (pBTClass) {
		class_destroy(pBTClass);
		pBTClass = NULL;
	}
	pr_info("%s 9\n", __func__);
	cdev_del(&BTMTK_cdev);
	pr_info("%s 10\n", __func__);
	unregister_chrdev_region(dev, 1);

	cdev_del(&BT_cdevfwlog);
	unregister_chrdev_region(devIDfwlog, 1);
	pr_info("%s driver removed.\n", BT_DRIVER_NAME);
}

static int btmtk_sdio_allocate_memory(void)
{
	if (txbuf == NULL) {
		txbuf = kmalloc(MTK_TXDATA_SIZE, GFP_ATOMIC);
		memset(txbuf, 0, MTK_TXDATA_SIZE);
	}

	if (rxbuf == NULL) {
		rxbuf = kmalloc(MTK_RXDATA_SIZE, GFP_ATOMIC);
		memset(rxbuf, 0, MTK_RXDATA_SIZE);
	}

	if (userbuf == NULL) {
		userbuf = kmalloc(MTK_TXDATA_SIZE, GFP_ATOMIC);
		memset(userbuf, 0, MTK_TXDATA_SIZE);
	}

	if (userbuf_fwlog == NULL) {
		userbuf_fwlog = kmalloc(MTK_TXDATA_SIZE, GFP_ATOMIC);
		memset(userbuf_fwlog, 0, MTK_TXDATA_SIZE);
	}

	return 0;
}

static int btmtk_sdio_free_memory(void)
{
	kfree(txbuf);

	kfree(rxbuf);

	kfree(userbuf);

	kfree(userbuf_fwlog);

	return 0;
}

static int __init btmtk_sdio_init_module(void)
{
	int ret = 0;

	ret = BTMTK_init();
	if (ret) {
		pr_info("%s: BTMTK_init failed!", __func__);
		return ret;
	}

	if (btmtk_sdio_allocate_memory() < 0) {
		pr_info("%s: allocate memory failed!", __func__);
		return -ENOMEM;
	}

	if (sdio_register_driver(&bt_mtk_sdio) != 0) {
		pr_info("SDIO Driver Registration Failed\n");
		return -ENODEV;
	}

	pr_info("SDIO Driver Registration Success\n");

	/* Clear the flag in case user removes the card. */
	user_rmmod = 0;

	return 0;
}

static void __exit btmtk_sdio_exit_module(void)
{
	/* Set the flag as user is removing this module. */
	user_rmmod = 1;
	BTMTK_exit();
	sdio_unregister_driver(&bt_mtk_sdio);
	btmtk_sdio_free_memory();
}

module_init(btmtk_sdio_init_module);
module_exit(btmtk_sdio_exit_module);
