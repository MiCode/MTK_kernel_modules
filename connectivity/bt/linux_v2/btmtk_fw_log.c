/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "btmtk_fw_log.h"
#include "btmtk_define.h"
#include "btmtk_main.h"

#if (USE_DEVICE_NODE == 1)
#include "btmtk_proj_sp.h"
#include "conn_dbg.h"
#include "btmtk_queue.h"
#include "aee.h"
#endif

#if (CFG_SUPPORT_TAS_HOST_CONTROL == 1)
#include "btmtk_tasar.h"
#endif // CFG_SUPPORT_TAS_HOST_CONTROL

/*
 * BT Logger Tool will turn on/off Firmware Picus log, and set 3 log levels (Low, SQC and Debug)
 * For extension capability, driver does not check the value range.
 *
 * Combine log state and log level to below settings:
 * - 0x00: OFF
 * - 0x01: Low Power
 * - 0x02: SQC
 * - 0x03: Debug
 */
#define BT_FWLOG_DEFAULT_LEVEL 0x02
#define CONNV3_XML_SIZE	1024	/* according to connv3_dump_test.c */

/* CTD BT log function and log status */
static wait_queue_head_t BT_log_wq;
static uint8_t g_bt_on = BT_FWLOG_OFF;
static uint8_t g_log_current = BT_FWLOG_OFF;
/* For fwlog dev node setting */
static struct btmtk_fops_fwlog *g_fwlog;

#if (STPBTFWLOG_ENABLE == 1)
static struct semaphore ioctl_mtx;
static uint8_t g_log_on = BT_FWLOG_OFF;
static uint8_t g_log_level = BT_FWLOG_DEFAULT_LEVEL;
const struct file_operations BT_fopsfwlog = {
	.open = btmtk_fops_openfwlog,
	.release = btmtk_fops_closefwlog,
	.read = btmtk_fops_readfwlog,
	.write = btmtk_fops_writefwlog,
	.poll = btmtk_fops_pollfwlog,
	.unlocked_ioctl = btmtk_fops_unlocked_ioctlfwlog,
	.compat_ioctl = btmtk_fops_compat_ioctlfwlog
};
#endif

/** read_write for proc */
static int btmtk_proc_show(struct seq_file *m, void *v);
static int btmtk_proc_open(struct inode *inode, struct  file *file);
static int btmtk_proc_chip_reset_count_open(struct inode *inode, struct  file *file);
static int btmtk_proc_chip_reset_count_show(struct seq_file *m, void *v);

#if (USE_DEVICE_NODE == 1)
int btmtk_proc_uart_launcher_notify_open(struct inode *inode, struct file *file);
int btmtk_proc_uart_launcher_notify_close(struct inode *inode, struct file *file);
ssize_t btmtk_proc_uart_launcher_notify_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
#endif

#if (KERNEL_VERSION(5, 6, 0) > LINUX_VERSION_CODE)
static const struct file_operations BT_proc_fops = {
	.open = btmtk_proc_open,
	.read = seq_read,
	.release = single_release,
};

static const struct file_operations BT_proc_chip_reset_count_fops = {
	.open = btmtk_proc_chip_reset_count_open,
	.read = seq_read,
	.release = single_release,
};

#if (USE_DEVICE_NODE == 1)
static const struct file_operations BT_proc_uart_launcher_notify_fops = {
	.open = btmtk_proc_uart_launcher_notify_open,
	.read = btmtk_proc_uart_launcher_notify_read,
	.release = btmtk_proc_uart_launcher_notify_close,
};
#endif

#else
static const struct proc_ops BT_proc_fops = {
	.proc_open = btmtk_proc_open,
	.proc_read = seq_read,
	.proc_release = single_release,
};

static const struct proc_ops BT_proc_chip_reset_count_fops = {
	.proc_open = btmtk_proc_chip_reset_count_open,
	.proc_read = seq_read,
	.proc_release = single_release,
};

#if (USE_DEVICE_NODE == 1)
static const struct proc_ops BT_proc_uart_launcher_notify_fops = {
	.proc_open = btmtk_proc_uart_launcher_notify_open,
	.proc_read = btmtk_proc_uart_launcher_notify_read,
	.proc_release = btmtk_proc_uart_launcher_notify_close,
};
#endif

#endif

#if (CFG_SUPPORT_CONNSYSLOGGER_FWLOG == 1)
void btmtk_connsys_log_init(void (*log_event_cb)(void))
{
	if (connv3_log_init(CONNV3_DEBUG_TYPE_BT, 2048, 2048, log_event_cb))
		BTMTK_ERR("*** %s fail! ***", __func__);
}

void btmtk_connsys_log_deinit(void)
{
	connv3_log_deinit(CONNV3_DEBUG_TYPE_BT);
}

int btmtk_connsys_log_handler(u8 *buf, u32 size)
{
	return connv3_log_handler(CONNV3_DEBUG_TYPE_BT, CONNV3_LOG_TYPE_PRIMARY, buf, size);
}

ssize_t btmtk_connsys_log_read_to_user(char __user *buf, size_t count)
{
	return connv3_log_read_to_user(CONNV3_DEBUG_TYPE_BT, buf, count);
}

unsigned int btmtk_connsys_log_get_buf_size(void)
{
	return connv3_log_get_buf_size(CONNV3_DEBUG_TYPE_BT);
}
#endif // CFG_SUPPORT_CONNSYSLOGGER_FWLOG

__weak int32_t btmtk_intcmd_wmt_utc_sync(void)
{
	BTMTK_WARN("weak function %s not implement", __func__);
	return -1;
}

__weak int32_t btmtk_intcmd_set_fw_log(struct btmtk_dev *bdev, uint8_t flag)
{
	BTMTK_WARN("weak function %s not implement", __func__);
	return -1;
}

__weak int btmtk_uart_launcher_deinit(void)
{
	BTMTK_WARN("weak function %s not implement", __func__);
	return -1;
}

__weak int btmtk_intcmd_wmt_blank_status(unsigned char blank_state)
{
	BTMTK_WARN("weak function %s not implement", __func__);
	return -1;
}

extern struct btmtk_dev *g_sbdev;
void fw_log_bt_state_cb(struct btmtk_dev *bdev, uint8_t state)
{
	uint8_t on_off;

	/* sp use BTMTK_FOPS_STATE_OPENED to judge state */
	on_off = (state == FUNC_ON) ? BT_FWLOG_ON : BT_FWLOG_OFF;
	BTMTK_INFO_LIMITTED("%s: current_log(0x%x) current_bt_on(0x%x) state_cb(%d) need_on_off(0x%x)",
				__func__, g_log_current, g_bt_on, state, on_off);

	if (g_bt_on != on_off) {
		// changed
		if (on_off == BT_FWLOG_OFF) { // should turn off
			g_bt_on = BT_FWLOG_OFF;
#if (STPBTFWLOG_ENABLE == 1)
			g_log_current = g_log_on & g_log_level;
#endif
			BTMTK_INFO("BT in off state, no need to send close fw log cmd");
		} else {
			g_bt_on = BT_FWLOG_ON;
			if (g_log_current) {
				btmtk_intcmd_set_fw_log(g_sbdev, g_log_current);
			}
			/* if bt open or reset when screen off, still need to notify fw blank status */
			btmtk_intcmd_wmt_blank_status(atomic_read(&g_sbdev->blank_state));
			btmtk_intcmd_wmt_utc_sync();
		}
	}
	g_sbdev->main_info.fw_log_on = g_log_current;
}

void fw_log_bt_event_cb(void)
{
	wake_up_interruptible(&BT_log_wq);
}

static int btmtk_proc_show(struct seq_file *m, void *v)
{
	int i;
	char cif_info[64] = {0};

	for (i = 0; i < btmtk_mcu_num; i++) {
		if (g_bdev[i] == NULL)
			continue;

		if (g_bdev[i]->hdev != NULL) {
			if (strlen(g_bdev[i]->main_info.fw_version_str))
				(void)seq_printf(m, "%s(Dongle%d)\n    patch version:%s\n    driver version:%s\n",
					g_bdev[i]->hdev->name, g_bdev[i]->dongle_index,
					g_bdev[i]->main_info.fw_version_str, VERSION);
			else
				(void)seq_printf(m, "%s(Dongle%d)\n    patch version:null\n    driver version:%s\n",
					g_bdev[i]->hdev->name, g_bdev[i]->dongle_index, VERSION);

			if (g_bdev[i]->rom_patch_bin_file_name)
				(void)seq_printf(m, "    fw_file_name:%s\n", g_bdev[i]->rom_patch_bin_file_name);
			else
				(void)seq_puts(m, "    fw_file_name:null\n");
		}
		if (g_bdev[i]->main_info.hif_hook.get_cif_info
				&& g_bdev[i]->main_info.hif_hook.get_cif_info(g_bdev[i],
								cif_info, sizeof(cif_info)) > 0) {
			(void)seq_printf(m, "    %s\n", cif_info);
		}
	}

	return 0;
}

static int btmtk_proc_chip_reset_count_show(struct seq_file *m, void *v)
{
	int i;

	for (i = 0; i < btmtk_mcu_num; i++) {
		if (g_bdev[i]->hdev != NULL) {
			(void)seq_printf(m, "%s(Dongle%d), whole_reset_count=%d subsys_reset_count=%d\n",
				g_bdev[i]->hdev->name, g_bdev[i]->dongle_index,
				atomic_read(&g_bdev[i]->main_info.whole_reset_count),
				atomic_read(&g_bdev[i]->main_info.subsys_reset_count));
		}
	}
	return 0;
}

static int btmtk_proc_open(struct inode *inode, struct  file *file)
{
	return single_open(file, btmtk_proc_show, NULL);
}

static int btmtk_proc_chip_reset_count_open(struct inode *inode, struct  file *file)
{
	return single_open(file, btmtk_proc_chip_reset_count_show, NULL);
}

static struct proc_dir_entry *proc_dir;
static void btmtk_proc_create_new_entry(void)
{
	struct proc_dir_entry *proc_show_entry = NULL;
	struct proc_dir_entry *proc_show_chip_reset_count_entry = NULL;

	BTMTK_DBG("%s, proc initialized", __func__);

	proc_dir = proc_mkdir(PROC_ROOT_DIR, NULL);
	if (proc_dir == NULL) {
		BTMTK_ERR("Unable to creat %s dir", PROC_ROOT_DIR);
		return;
	}

	proc_show_entry =  proc_create(PROC_BT_FW_VERSION, 0600, proc_dir, &BT_proc_fops);
	if (proc_show_entry == NULL) {
		BTMTK_ERR("Unable to creat %s node", PROC_BT_FW_VERSION);
		remove_proc_entry(PROC_ROOT_DIR, NULL);
	}

#if (USE_DEVICE_NODE == 1)
	proc_show_entry =  proc_create(PROC_BT_UART_LAUNCHER_NOTIFY, 0640,
			proc_dir, &BT_proc_uart_launcher_notify_fops);
	if (proc_show_entry == NULL) {
		BTMTK_ERR("Unable to creat %s node", PROC_BT_UART_LAUNCHER_NOTIFY);
		remove_proc_entry(PROC_ROOT_DIR, NULL);
	}
#endif

	proc_show_chip_reset_count_entry = proc_create(PROC_BT_CHIP_RESET_COUNT, 0600,
			proc_dir, &BT_proc_chip_reset_count_fops);
	if (proc_show_chip_reset_count_entry == NULL) {
		BTMTK_ERR("Unable to creat %s node", PROC_BT_CHIP_RESET_COUNT);
		remove_proc_entry(PROC_ROOT_DIR, NULL);
	}

}

static void btmtk_proc_delete_entry(void)
{

	if (proc_dir == NULL)
		return;

	remove_proc_entry(PROC_BT_FW_VERSION, proc_dir);
	BTMTK_INFO("%s, proc device node and folder removed!!", __func__);
	remove_proc_entry(PROC_BT_CHIP_RESET_COUNT, proc_dir);
	BTMTK_INFO("%s, proc device node and folder %s removed!!", __func__, PROC_BT_CHIP_RESET_COUNT);
#if (USE_DEVICE_NODE == 1)
	remove_proc_entry(PROC_BT_UART_LAUNCHER_NOTIFY, proc_dir);
	BTMTK_INFO("%s, proc device node and folder %s removed!!", __func__, PROC_BT_UART_LAUNCHER_NOTIFY);
#endif

	remove_proc_entry(PROC_ROOT_DIR, NULL);
	proc_dir = NULL;
}

static int btmtk_fops_initfwlog(void)
{
#if (STPBTFWLOG_ENABLE == 1)
#ifdef STATIC_REGISTER_FWLOG_NODE
	static int BT_majorfwlog = FIXED_STPBT_MAJOR_DEV_ID + 1;
	dev_t devIDfwlog = MKDEV(BT_majorfwlog, 1);
#else
	static int BT_majorfwlog;
	dev_t devIDfwlog = MKDEV(BT_majorfwlog, 0);
#endif
	int ret = 0;
	int cdevErr = 0;
#endif
	struct btmtk_dev **pp_bdev = btmtk_get_pp_bdev();
	struct btmtk_dev *bdev = NULL;
	int i = 0;

#if (STPBTFWLOG_ENABLE == 1)
	BTMTK_DBG("%s: Start %s", __func__, BT_FWLOG_DEV_NODE);
#endif

	if (g_fwlog == NULL) {
		g_fwlog = kzalloc(sizeof(*g_fwlog), GFP_KERNEL);
		if (!g_fwlog) {
			BTMTK_ERR("%s: alloc memory fail (g_data)", __func__);
			return -1;
		}
	}

#if (STPBTFWLOG_ENABLE == 1)
#ifdef STATIC_REGISTER_FWLOG_NODE
	ret = register_chrdev_region(devIDfwlog, 1, "BT_chrdevfwlog");
	if (ret) {
		BTMTK_ERR("%s: fail to register chrdev(%x)", __func__, devIDfwlog);
		goto alloc_error;
	}
#else
	ret = alloc_chrdev_region(&devIDfwlog, 0, 1, "BT_chrdevfwlog");
	if (ret) {
		BTMTK_ERR("%s: fail to allocate chrdev", __func__);
		goto alloc_error;
	}
#endif
	BT_majorfwlog = MAJOR(devIDfwlog);

	cdev_init(&g_fwlog->BT_cdevfwlog, &BT_fopsfwlog);
	g_fwlog->BT_cdevfwlog.owner = THIS_MODULE;

	cdevErr = cdev_add(&g_fwlog->BT_cdevfwlog, devIDfwlog, 1);
	if (cdevErr)
		goto cdv_error;

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 4, 0)
	g_fwlog->pBTClass = class_create(THIS_MODULE, BT_FWLOG_DEV_NODE);
#else
	g_fwlog->pBTClass = class_create(BT_FWLOG_DEV_NODE);
#endif
	if (IS_ERR(g_fwlog->pBTClass)) {
		BTMTK_ERR("%s: class create fail, error code(%ld)", __func__, PTR_ERR(g_fwlog->pBTClass));
		goto create_node_error;
	}

	/* move node create after waitqueue init, incase of fw log open first */
	g_fwlog->pBTDevfwlog = device_create(g_fwlog->pBTClass, NULL, devIDfwlog, NULL,
		"%s", BT_FWLOG_DEV_NODE);
	if (IS_ERR(g_fwlog->pBTDevfwlog)) {
		BTMTK_ERR("%s: device(stpbtfwlog) create fail, error code(%ld)", __func__,
			PTR_ERR(g_fwlog->pBTDevfwlog));
		goto create_node_error;
	}
	BTMTK_INFO("%s: BT_majorfwlog %d, devIDfwlog %d", __func__, BT_majorfwlog, devIDfwlog);

	g_fwlog->g_devIDfwlog = devIDfwlog;
	sema_init(&ioctl_mtx, 1);
#endif

	//if (is_mt66xx(g_sbdev->chip_id)) {
	for (i = 0; i < btmtk_mcu_num; i++) {
		bdev = pp_bdev[i];
		if (bdev == NULL)
			continue;

#if (CFG_SUPPORT_CONNSYSLOGGER_FWLOG == 1)
		btmtk_connsys_log_init(fw_log_bt_event_cb);
		init_waitqueue_head(&BT_log_wq);
#else
		spin_lock_init(&g_fwlog->fwlog_lock);
		skb_queue_head_init(&g_fwlog->fwlog_queue);
		skb_queue_head_init(&g_fwlog->dumplog_queue_first);
		skb_queue_head_init(&g_fwlog->dumplog_queue_latest);
		skb_queue_head_init(&g_fwlog->dumplog_queue_wf);
		skb_queue_head_init(&g_fwlog->usr_opcode_queue);//opcode
		init_waitqueue_head(&(g_fwlog->fw_log_inq));
#endif
		atomic_set(&bdev->main_info.fwlog_ref_cnt, 0);
	}

	BTMTK_INFO("%s: End", __func__);
	return 0;

#if (STPBTFWLOG_ENABLE == 1)
create_node_error:
	if (g_fwlog->pBTClass) {
		class_destroy(g_fwlog->pBTClass);
		g_fwlog->pBTClass = NULL;
	}

cdv_error:
	if (cdevErr == 0)
		cdev_del(&g_fwlog->BT_cdevfwlog);

	if (ret == 0)
		unregister_chrdev_region(devIDfwlog, 1);

alloc_error:
	kfree(g_fwlog);
	g_fwlog = NULL;

	return -1;
#endif
}

static int btmtk_fops_exitfwlog(void)
{
#if (STPBTFWLOG_ENABLE == 1)
	dev_t devIDfwlog = g_fwlog->g_devIDfwlog;
#endif
	struct btmtk_dev **pp_bdev = btmtk_get_pp_bdev();
	struct btmtk_dev *bdev = NULL;
	int i = 0;

	BTMTK_INFO("%s: Start", __func__);

	//if (is_mt66xx(g_sbdev->chip_id))
	for (i = 0; i < btmtk_mcu_num; i++) {
		bdev = pp_bdev[i];
		if (bdev->hdev == NULL)
			continue;

#if (CFG_SUPPORT_CONNSYSLOGGER_FWLOG == 1)
		btmtk_connsys_log_deinit();
#endif // CFG_SUPPORT_CONNSYSLOGGER_FWLOG
	}

#if (STPBTFWLOG_ENABLE == 1)
	if (g_fwlog->pBTDevfwlog) {
		device_destroy(g_fwlog->pBTClass, devIDfwlog);
		g_fwlog->pBTDevfwlog = NULL;
	}

	if (g_fwlog->pBTClass) {
		class_destroy(g_fwlog->pBTClass);
		g_fwlog->pBTClass = NULL;
	}
	BTMTK_INFO("%s: pBTDevfwlog, pBTClass done", __func__);

	cdev_del(&g_fwlog->BT_cdevfwlog);
	unregister_chrdev_region(devIDfwlog, 1);
	BTMTK_INFO("%s: BT_chrdevfwlog driver removed", __func__);
#endif
	kfree(g_fwlog);

	return 0;
}

static int flag;
void btmtk_init_node(void)
{
	if (flag == 1)
		return;

	flag = 1;
	btmtk_proc_create_new_entry();
	if (btmtk_fops_initfwlog() < 0)
		BTMTK_ERR("%s: create stpbtfwlog failed", __func__);
}

void btmtk_deinit_node(void)
{
	if (flag != 1)
		return;

	flag = 0;
	btmtk_proc_delete_entry();
	if (btmtk_fops_exitfwlog() < 0)
		BTMTK_ERR("%s: release stpbtfwlog failed", __func__);
}

#if (STPBTFWLOG_ENABLE == 1)
ssize_t btmtk_fops_readfwlog(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	int copyLen = 0;
	ulong flags = 0;
	struct sk_buff *skb = NULL;
	struct btmtk_dev **pp_bdev = btmtk_get_pp_bdev();
	struct btmtk_dev *bdev = NULL;
	int i = 0;

	//if (is_mt66xx(g_sbdev->chip_id)) {
	for (i = 0; i < btmtk_mcu_num; i++) {
		bdev = pp_bdev[i];
		if (bdev->hdev == NULL)
			continue;
#if (CFG_SUPPORT_CONNSYSLOGGER_FWLOG == 1)
		copyLen = btmtk_connsys_log_read_to_user(buf, count);
		return copyLen;
#endif // CFG_SUPPORT_CONNSYSLOGGER_FWLOG
	}

	/* picus read coredump first */
	spin_lock_irqsave(&g_fwlog->fwlog_lock, flags);
	if (skb == NULL && skb_queue_len(&g_fwlog->dumplog_queue_first)) {
		skb = skb_dequeue(&g_fwlog->dumplog_queue_first);
	}

	if (skb == NULL && skb_queue_len(&g_fwlog->dumplog_queue_latest)) {
		skb = skb_dequeue(&g_fwlog->dumplog_queue_latest);
	}

	/* picus read a queue, it may occur performace issue */
	if (skb == NULL && skb_queue_len(&g_fwlog->fwlog_queue)) {
		skb = skb_dequeue(&g_fwlog->fwlog_queue);
	}

	/* picus read wifi coredump */
	if (skb == NULL && skb_queue_len(&g_fwlog->dumplog_queue_wf)) {
		skb = skb_dequeue(&g_fwlog->dumplog_queue_wf);
	}
	spin_unlock_irqrestore(&g_fwlog->fwlog_lock, flags);

	if (skb == NULL)
		return 0;

	if (skb->len <= count) {
		if (copy_to_user(buf, skb->data, skb->len))
			BTMTK_ERR("%s: copy_to_user failed!", __func__);
		copyLen = skb->len;
	} else {
		BTMTK_DBG("%s: socket buffer length error(count: %d, skb.len: %d)",
			__func__, (int)count, skb->len);
	}
	kfree_skb(skb);
	skb = NULL;
	return copyLen;
}

int btmtk_cmd_length_check(u8 *data, u16 length, u8 type)
{
	u16 data_len = 0;

	if (type == HCI_COMMAND_PKT) {
		data_len = data[3] + 4;
		if (data_len != length) {
			BTMTK_DBG("length check fail, recv len = %u, data len = %u, type is 0x%02x",
						length, data_len, HCI_COMMAND_PKT);
			return -1;
		}
	} else if (type == FWLOG_TYPE) {
		data_len = data[1] + ((data[2] << 8) & 0xff00) + 3;
		if (data_len != length) {
			BTMTK_DBG("length check fail, recv len = %u, data len = %u, type is 0x%02x",
						 length, data_len, FWLOG_TYPE);
			return -1;
		}
	}

	return 0;
}

#if (CFG_ENABLE_DEBUG_WRITE != 0)
static int btmtk_cmd_handler_fwlog(char *hci_dev, char *cmd, size_t count)
{
	int i = 0, ret;
	u8 val = 0;
	char *ptr = NULL;
	struct btmtk_dev **pp_bdev = btmtk_get_pp_bdev();
	struct btmtk_dev *bdev = NULL;
	int nvsz = 0;
	int state = 0;
	u8 *connxo = NULL;

	/* For log level, EX: echo log_lvl=1 > /dev/stpbtfwlog */
	if (strncmp(cmd, "log_lvl=", strlen("log_lvl=")) == 0) {
		val = *(cmd + strlen("log_lvl=")) - '0';

		if (val > BTMTK_LOG_LVL_MAX || val <= 0) {
			BTMTK_ERR("Got incorrect value for log level(%d)", val);
		} else {
			btmtk_log_lvl = val;
			BTMTK_INFO("btmtk_log_lvl = %d", btmtk_log_lvl);
		}
		return 0;
	}

	/* For bperf, EX: echo bperf=1 > /dev/stpbtfwlog */
	if (strncmp(cmd, "bperf=", strlen("bperf=")) == 0) {
		val = *(cmd + strlen("bperf=")) - '0';

		g_fwlog->btmtk_bluetooth_kpi = val;
		BTMTK_INFO("%s: set bluetooth KPI feature(bperf) to %d", __func__, g_fwlog->btmtk_bluetooth_kpi);
		return 0;
	}
#ifdef BTMTK_DEBUG_SOP
	if (strncmp(cmd, "fwdump", strlen("fwdump")) == 0) {
		BTMTK_INFO("dumplog_queue_first1, skb_queue_len = %d",
				skb_queue_len(&g_fwlog->dumplog_queue_first));
		BTMTK_INFO("dumplog_queue_latest, skb_queue_len = %d",
				skb_queue_len(&g_fwlog->dumplog_queue_latest));

		return 0;
	}
#endif

	if (strncmp(cmd, "dumpclean", strlen("dumpclean")) == 0) {
		skb_queue_purge(&g_fwlog->dumplog_queue_first);
		skb_queue_purge(&g_fwlog->dumplog_queue_latest);
		return 0;
	}

	for (i = 0; i < btmtk_mcu_num; i++) {
		bdev = pp_bdev[i];
		if (bdev == NULL || bdev->hdev == NULL)
			continue;

		if (hci_dev != NULL && bdev->hdev->name &&
				strncmp(bdev->hdev->name, hci_dev, strlen(hci_dev)) != 0)
			continue;

		ptr = NULL;

		ptr = strstr(cmd, "chip_reset=");
		if (ptr != NULL) {
			val = *(ptr + strlen("chip_reset=")) - '0';
			bdev->main_info.chip_reset_flag = val;
			BTMTK_INFO_DEV(bdev, "%s: set %s chip reset flag to %d",
					__func__, bdev->hdev->name, bdev->main_info.chip_reset_flag);
			continue;
		}

		ptr = strstr(cmd, "whole chip reset");
		if (ptr != NULL) {
			if (btmtk_fops_get_state(bdev) != BTMTK_FOPS_STATE_OPENED) {
				BTMTK_INFO_DEV(bdev, "%s: bt is not opened, skip FW_LOG_BT trigger whole chip reset", __func__);
				continue;
			}
			BTMTK_INFO_DEV(bdev, " %s whole chip reset start", bdev->hdev->name);
			if (bdev->assert_reason[0] == '\0') {
				memset(bdev->assert_reason, 0, ASSERT_REASON_SIZE);
				strncpy(bdev->assert_reason, "FW_LOG_BT trigger whole chip reset", strlen("FW_LOG_BT trigger whole chip reset") + 1);
			}
			bdev->main_info.chip_reset_flag = 1;
			btmtk_reset_trigger(bdev);
			continue;
		}

		ptr = strstr(cmd, "subsys chip reset");
		if (ptr != NULL) {
			if (btmtk_fops_get_state(bdev) != BTMTK_FOPS_STATE_OPENED) {
				BTMTK_INFO_DEV(bdev, "%s: bt is not opened, skip FW_LOG_BT trigger subsys chip reset", __func__);
				continue;
			}

			BTMTK_INFO_DEV(bdev, " %s subsys chip reset start", bdev->hdev->name);
			if (bdev->assert_reason[0] == '\0') {
				memset(bdev->assert_reason, 0, ASSERT_REASON_SIZE);
				strncpy(bdev->assert_reason, "FW_LOG_BT trigger subsys chip reset", strlen("FW_LOG_BT trigger subsys chip reset") + 1);
			}
			if (bdev->main_info.hif_hook.trigger_assert)
				bdev->main_info.hif_hook.trigger_assert(bdev);
			else {
				bdev->main_info.chip_reset_flag = 0;
				btmtk_reset_trigger(bdev);
			}
			continue;
		}

		ptr = strstr(cmd, "dump btsnoop");
		if (ptr != NULL) {
			BTMTK_INFO_DEV(bdev, " %s dump btsnoop", bdev->hdev->name);
			btmtk_hci_snoop_print_to_log(bdev);
			continue;
		}

		ptr = strstr(cmd, "set_para=");
		if (ptr != NULL) {
			val = *(ptr + strlen("set_para=")) - '0';
			if (bdev->main_info.hif_hook.set_para)
				bdev->main_info.hif_hook.set_para(bdev, val);
			else
				BTMTK_WARN_DEV(bdev, "%s: not support set_para", __func__);
			continue;
		}

		ptr = strstr(cmd, "set_xonv=");
		if (ptr != NULL) {
			connxo = ptr + strlen("set_xonv=");
			nvsz = count - strlen("set_xonv=");
			if (bdev->main_info.hif_hook.set_xonv)
				bdev->main_info.hif_hook.set_xonv(bdev, connxo, nvsz);
			else
				BTMTK_WARN_DEV(bdev, "%s: not support set_xonv", __func__);
			continue;
		}

		ptr = strstr(cmd, "direct trigger whole chip reset");
		if (ptr != NULL) {
			if (btmtk_fops_get_state(bdev) != BTMTK_FOPS_STATE_OPENED) {
				BTMTK_INFO_DEV(bdev, "%s: bt is not opened, skip direct trigger whole chip reset", __func__);
				continue;
			}

			BTMTK_INFO_DEV(bdev, "direct trigger whole chip reset");
			if (bdev->assert_reason[0] == '\0') {
				memset(bdev->assert_reason, 0, ASSERT_REASON_SIZE);
				strncpy(bdev->assert_reason, "FW_LOG_BT direct trigger whole chip reset",
						strlen("FW_LOG_BT direct trigger whole chip reset") + 1);
			}
			if (bdev->main_info.hif_hook.whole_reset)
				bdev->main_info.hif_hook.whole_reset(bdev);
			else
				BTMTK_INFO_DEV(bdev, "not support direct trigger whole chip reset");
			continue;
		}

		ptr = strstr(cmd, "dump chip reset");
		if (ptr != NULL) {
			BTMTK_INFO_DEV(bdev, "subsys chip reset = %d",
					atomic_read(&bdev->main_info.subsys_reset_count));
			BTMTK_INFO_DEV(bdev, "whole chip reset = %d",
					atomic_read(&bdev->main_info.whole_reset_count));
			continue;
		}

#if (CFG_SUPPORT_TAS_HOST_CONTROL == 1)
		/* tasar countryCode=TW, eci=*/
		ptr = strstr(cmd, "tasar countrycode=");
		if (ptr != NULL) {
			unsigned short int countryCode = 0;
			unsigned int eci = 0;

			/* parse contry code */
			ptr += strlen("tasar countrycode=");
			countryCode = ptr[0] << 8 | ptr[1];
			BTMTK_INFO_DEV(bdev, "%s: set countrycode[%c%c] = 0x%x",
						__func__, ptr[0], ptr[1], countryCode);

			/* parse eci */
			ptr = strstr(cmd, "eci=");
			if (ptr != NULL) {
				ptr += strlen("eci=");
				ret = kstrtou32(ptr, 0, &eci);
				if (ret) {
					BTMTK_WARN_DEV(bdev, "%s: convert eci string failed ret[%d]", __func__, ret);
					continue;
				}
				BTMTK_INFO_DEV(bdev, "%s: set eci = %d", __func__, eci);
				btmtk_set_tasar_from_host(bdev, countryCode, eci);
				continue;
			} else {
				BTMTK_WARN_DEV(bdev, "%s: set countrycode without eci", __func__);
				continue;
			}
		}
#endif // CFG_SUPPORT_TAS_HOST_CONTROL


#if (USE_DEVICE_NODE == 1)
		ptr = strstr(cmd, "FindMyPhone=");
		if (ptr != NULL) {
			ret = kstrtou32(ptr + strlen("FindMyPhone="), 0, &bdev->main_info.find_my_phone_mode);

			if (ret) {
				BTMTK_WARN_DEV(bdev, "%s: convert string failed ret[%d]", __func__, ret);
				continue;
			}

			BTMTK_INFO_DEV(bdev, "%s: FindMyPhone[%u]", __func__, bdev->main_info.find_my_phone_mode);
			continue;
		}

		ptr = strstr(cmd, "PowerOffFinderMode=");
		if (ptr != NULL) {
			val = *(ptr + strlen("PowerOffFinderMode=")) - '0';
			val = !!val;

			atomic_set(&bdev->poweroff_finder_mode, val);

			BTMTK_INFO_DEV(bdev, "%s: set PowerOffFinderMode[%u]", __func__,
						atomic_read(&bdev->poweroff_finder_mode));
			continue;
		}
#endif
		ptr = strstr(cmd, "hif_debug_sop");
		if (ptr != NULL) {
			state = btmtk_get_chip_state(bdev);
			//&& state == BTMTK_STATE_WORKING
			if (bdev->main_info.hif_hook.dump_hif_debug_sop)
				bdev->main_info.hif_hook.dump_hif_debug_sop(bdev);
			else
				BTMTK_INFO_DEV(bdev, "%s: not support hif_debug_sop or chip_state[%d]",
						__func__, state);
			continue;
		}

		/* The spider tool hopes not to intercept these events */
		ptr = strstr(cmd, "evt_intercept_disable=");
		if (ptr != NULL) {
			val = *(ptr + strlen("evt_intercept_disable=")) - '0';
			val = !!val;

			bdev->is_evt_intercept_disable = val;

			BTMTK_INFO_DEV(bdev, "%s: set is_evt_intercept_disable[%d]", __func__,
						bdev->is_evt_intercept_disable);
			continue;
		}

		ptr = strstr(cmd, "bk-rs=");
		if (ptr != NULL) {
			val = *(ptr + strlen("bk-rs=")) - '0';
			bdev->main_info.bk_rs_flag = val;
			BTMTK_INFO_DEV(bdev, "%s: set bk_rs_flag to %d",
					__func__, bdev->main_info.find_my_phone_mode);
			continue;
		}

		ptr = strstr(cmd, "dump btsnoop");
		if (ptr != NULL) {
			btmtk_hci_snoop_print_to_log(bdev);
			continue;
		}

#ifdef BTMTK_DEBUG_SOP
		ptr = strstr(cmd, "dump test");
		if (ptr != NULL) {
			btmtk_load_debug_sop_register(bdev);
			continue;
		}

		ptr = strstr(cmd, "dump clean");
		if (ptr != NULL) {
			btmtk_clean_debug_reg_file(bdev);
			continue;
		}
#endif
		ptr = strstr(cmd, "dump_debug=");
		if (ptr != NULL) {
			val = *(ptr + strlen("dump_debug=")) - '0';
			if (bdev->main_info.hif_hook.dump_debug_sop) {
				BTMTK_INFO_DEV(bdev, "%s: dump_debug(%s)", __func__,
					(val == 0) ? "SLEEP" :
					((val == 1) ? "WAKEUP" :
					((val == 2) ? "NO_RESPONSE" : "ERROR")));
				if (btmtk_fops_get_state(bdev) != BTMTK_FOPS_STATE_OPENED) {
					ret = bdev->main_info.hif_hook.open(bdev->hdev);
					if (ret < 0) {
						BTMTK_ERR_DEV(bdev, "%s, cif_open failed", __func__);
						return 0;
					}
				}
				bdev->main_info.hif_hook.dump_debug_sop(bdev);
				if (btmtk_fops_get_state(bdev) != BTMTK_FOPS_STATE_OPENED) {
					ret = bdev->main_info.hif_hook.close(bdev->hdev);
					if (ret < 0) {
						BTMTK_ERR_DEV(bdev, "%s, cif_close failed", __func__);
						return 0;
					}
				}
			} else {
				BTMTK_INFO_DEV(bdev, "%s: not support", __func__);
			}
			continue;
		}
		BTMTK_INFO_DEV(bdev, "%s: %s not support", __func__, cmd);
		return -1;
	}
	return 0;
}
#endif

/* Format for BTX, X=0,1
 * echo hciX 01 03 0C 00 > /dev/stpbtfwlog
 * echo hciX dump btsnoop > /dev/stpbtfwlog
 */
ssize_t btmtk_fops_writefwlog(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
#if (CFG_ENABLE_DEBUG_WRITE == 0)
	BTMTK_WARN("%s: CFG_ENABLE_DEBUG_WRITE not enable", __func__);
	return -ENODEV;
#else
	int i = 0, len = 0, ret = 0;
	int hci_idx = 0;
	int vlen = 0, index = 3;
	struct sk_buff *skb = NULL;
#if (USE_DEVICE_NODE == 0)
	struct sk_buff *skb_opcode = NULL;
#endif
	int state = 0;
	unsigned char fstate = 0;
	u8 *i_fwlog_buf = NULL;
	u8 *o_fwlog_buf = NULL;
	struct btmtk_dev **pp_bdev = btmtk_get_pp_bdev();
	struct btmtk_dev *bdev = NULL;

	char *p_hci = NULL;
	char hci_dev[8] = {0};
	u8 hci_len = 0;
	int hci_start_offset = 0;

	i_fwlog_buf = kmalloc(HCI_MAX_COMMAND_BUF_SIZE, GFP_KERNEL);
	if (!i_fwlog_buf) {
		BTMTK_ERR("%s: alloc i_fwlog_buf failed", __func__);
		return -ENOMEM;
	}

	/* allocate 16 more bytes for header part */
	o_fwlog_buf = kmalloc(HCI_MAX_COMMAND_SIZE + 16, GFP_KERNEL);
	if (!o_fwlog_buf) {
		BTMTK_ERR("%s: alloc o_fwlog_buf failed", __func__);
		ret = -ENOMEM;
		kfree(i_fwlog_buf);
		return -ENOMEM;
	}

	if (count > HCI_MAX_COMMAND_BUF_SIZE) {
		BTMTK_ERR("%s: your command is larger than maximum length, count = %zd",
				__func__, count);
		goto exit;
	}

	memset(i_fwlog_buf, 0, HCI_MAX_COMMAND_BUF_SIZE);
	memset(o_fwlog_buf, 0, HCI_MAX_COMMAND_SIZE + 16);

	if (buf == NULL || count == 0) {
		BTMTK_ERR("%s: worng input data", __func__);
		goto exit;
	}

	if (copy_from_user(i_fwlog_buf, buf, count) != 0) {
		BTMTK_ERR("%s: Failed to copy data", __func__);
		goto exit;
	}

    /* Ensure that i_fwlog_buf is null-terminated */
	if (count < HCI_MAX_COMMAND_BUF_SIZE) {
		i_fwlog_buf[count] = '\0';
	} else {
		i_fwlog_buf[HCI_MAX_COMMAND_BUF_SIZE - 1] = '\0';
	}

	p_hci = strstr(i_fwlog_buf, "hci");
	if (p_hci != NULL) {
		char *p = strstr(p_hci, " ");
		if (p == NULL) {
			BTMTK_ERR("%s: hci string compare failed, can't find blank, input: %s", __func__, i_fwlog_buf);
			goto exit;
		}
		if ((p - p_hci) > (sizeof(hci_dev) - 1)) {
			BTMTK_ERR("%s: hci string compare failed, input: %s", __func__, i_fwlog_buf);
			goto exit;
		}
		hci_len = p - p_hci;
		memcpy(hci_dev, p_hci, hci_len);
		hci_dev[hci_len] = '\0';
		hci_start_offset = p_hci - (char *)i_fwlog_buf;
		p_hci = hci_dev;
		BTMTK_INFO("%s, set hci dev(%d): %s, cmd: %s",
			__func__, hci_len, p_hci, i_fwlog_buf + hci_start_offset + hci_len);
	} else {
		BTMTK_INFO("%s, set all hci dev", __func__);
		hci_len = 0;
	}

	if (btmtk_cmd_handler_fwlog(p_hci, i_fwlog_buf, count) == 0) {
		ret = count;
		goto exit;
	}

	/* hci input command format : echo 01 be fc 01 05 > /dev/stpbtfwlog */
	/* We take the data from index three to end. */
	for (i = 0; i < count - hci_len; i++) {
		char *pos = i_fwlog_buf + hci_start_offset + hci_len + i;
		char temp_str[3] = {'\0'};
		long res = 0;

		if (*pos == ' ' || *pos == '\t' || *pos == '\r' || *pos == '\n') {
			continue;
		} else if (*pos == '0' && (*(pos + 1) == 'x' || *(pos + 1) == 'X')) {
			i++;
			continue;
		} else if (!(*pos >= '0' && *pos <= '9') && !(*pos >= 'A' && *pos <= 'F')
			&& !(*pos >= 'a' && *pos <= 'f')) {
			BTMTK_ERR("%s: There is an invalid input(%c)", __func__, *pos);
			goto exit;
		} else if (len + 1 >= HCI_MAX_COMMAND_SIZE) {
			BTMTK_ERR("%s: input data exceed maximum command length (%d)", __func__, len);
			goto exit;
		}

		temp_str[0] = *pos;
		temp_str[1] = *(pos + 1);
		i++;
		ret = kstrtol(temp_str, 16, &res);
		if (ret == 0)
			o_fwlog_buf[len++] = (u8)res;
		else {
			BTMTK_ERR_DEV(pp_bdev[hci_idx], "%s: Convert %s failed(%d)", __func__, temp_str, ret);
			goto exit;
		}
	}

	if (o_fwlog_buf[0] != HCI_COMMAND_PKT && o_fwlog_buf[0] != FWLOG_TYPE) {
		BTMTK_ERR_DEV(pp_bdev[hci_idx], "%s: Not support 0x%02X yet", __func__, o_fwlog_buf[0]);
		goto exit;
	}

	if (btmtk_cmd_length_check(o_fwlog_buf, len, o_fwlog_buf[0])) {
		goto exit;
	}

	/* check HCI command length */
	if (len < HCI_CMD_HEADER_SIZE) {
		BTMTK_ERR_DEV(pp_bdev[hci_idx], "%s: command is too short for hci cmd, length = %d", __func__, len);
		goto exit;
	} else if (len > HCI_MAX_COMMAND_SIZE) {
		BTMTK_ERR_DEV(pp_bdev[hci_idx], "%s: command is larger than max buf size, length = %d", __func__, len);
		goto exit;
	}

	if (o_fwlog_buf[0] == HCI_COMMAND_PKT) {
		BTMTK_DBG_DEV(pp_bdev[hci_idx], "%s: hci_cmd_len[%d], input_len[%d]", __func__, o_fwlog_buf[3], len);
		if (o_fwlog_buf[3] + HCI_CMD_HEADER_SIZE != len) {
			BTMTK_ERR_DEV(pp_bdev[hci_idx], "%s: pkt format error, hci_cmd_len[%d], input_len[%d]",
						__func__, o_fwlog_buf[3], len);
			goto exit;
		}
	}
	/* won't send command if g_bdev not define */
	for (i = 0; i < btmtk_mcu_num; i++) {
		bdev = pp_bdev[i];
		if (bdev->hdev == NULL)
			continue;

		if (p_hci != NULL && strncmp(bdev->hdev->name, p_hci, strlen(p_hci)) != 0)
			continue;

		state = btmtk_get_chip_state(bdev);
		if (state != BTMTK_STATE_WORKING) {
			BTMTK_WARN_DEV(bdev, "%s: current is in suspend/resume/standby/dump/disconnect (%d).",
				__func__, state);
			continue;
		}

		fstate = btmtk_fops_get_state(bdev);
		if (fstate != BTMTK_FOPS_STATE_OPENED) {
			BTMTK_WARN_DEV(bdev, "%s: fops is not open yet(%d)!", __func__, fstate);
			continue;
		}

		if (bdev->power_state == BTMTK_DONGLE_STATE_POWER_OFF) {
			BTMTK_WARN_DEV(bdev, "%s: dongle state already power off, do not write", __func__);
			continue;
		}

		skb = alloc_skb(count + BT_SKB_RESERVE, GFP_KERNEL);
		if (!skb) {
			BTMTK_ERR_DEV(bdev, "%s allocate skb failed!!", __func__);
			continue;
		}

		/* send HCI command */
		bt_cb(skb)->pkt_type = HCI_COMMAND_PKT;

		/* Picus -> Driver
		 * F0 XX XX 00 01 AA 10 BB CC CC CC ...
		 * F0 : specific header between picus and driver
		 * XX XX total command length
		 * 00 : hci interface setting type
		 * 01 AA length of this segment is 1, AA is dongle index
		 * 10 : raw data type, FWLOG_TX
		 * BB CC CC ... : BB is raw data's length (<256)
		 *			   : CC CC ... is raw data
		 */
		vlen = 0;
		index = 3;
		if (o_fwlog_buf[0] == FWLOG_TYPE) {
			while (skb && (index < ((o_fwlog_buf[2] << 8) + o_fwlog_buf[1]))) {
				switch (o_fwlog_buf[index]) {
				case FWLOG_HCI_IDX:	/* hci index */
					vlen = o_fwlog_buf[index + 1];
					hci_idx = o_fwlog_buf[index + 2];
					BTMTK_INFO_DEV(bdev, "%s: send to hci%d, vlen %d",
							__func__, hci_idx, vlen);
					(void)snprintf(hci_dev, sizeof(hci_dev), "hci%d", hci_idx);
					if (strncmp(hci_dev, bdev->hdev->name, strlen(hci_dev)) != 0) {
						kfree_skb(skb);
						skb = NULL;
					}
					index += (FWLOG_ATTR_TL_SIZE + vlen);
					break;
				case FWLOG_TX:	  /* tx raw data */
					vlen = o_fwlog_buf[index + 1];
					memcpy(skb->data, o_fwlog_buf + index + FWLOG_ATTR_TL_SIZE, vlen);
					skb->len = vlen;
					index = index + FWLOG_ATTR_TL_SIZE + vlen;
					break;
				default:
					BTMTK_WARN_DEV(bdev, "%s: Invalid opcode(%d)",
							__func__, o_fwlog_buf[index]);
					kfree_skb(skb);
				}
			}
		} else {
			memcpy(skb->data, o_fwlog_buf, len);
			skb->len = len;
#if defined(DRV_RETURN_SPECIFIC_HCE_ONLY) && (DRV_RETURN_SPECIFIC_HCE_ONLY == 1) && (USE_DEVICE_NODE == 0)
			// 0xFC26 is get link & profile information command.
			// 0xFCF5 is get iso perf link info command.
			if (*(uint16_t *)(o_fwlog_buf + 1) != 0xFC26) {
#endif
#if (USE_DEVICE_NODE == 0)
				skb_opcode = alloc_skb(len + FWLOG_PRSV_LEN, GFP_KERNEL);
				if (!skb_opcode) {
					BTMTK_ERR_DEV(bdev, "%s allocate skb failed!!", __func__);
					kfree_skb(skb);
					continue;
				}
				memcpy(skb_opcode->data, (o_fwlog_buf + 1), 2);
				BTMTK_INFO_DEV(bdev, "opcode is %02x,%02x",
						skb_opcode->data[0], skb_opcode->data[1]);
				skb_queue_tail(&g_fwlog->usr_opcode_queue, skb_opcode);
#endif
#if defined(DRV_RETURN_SPECIFIC_HCE_ONLY) && (DRV_RETURN_SPECIFIC_HCE_ONLY == 1) && (USE_DEVICE_NODE == 0)
			}
#endif
		}

		if (skb == NULL)
			continue;

		/* clean fwlog queue before enable picus log */
		if (skb_queue_len(&g_fwlog->fwlog_queue) && skb->data[0] == 0x01
				&& skb->data[1] == 0x5d && skb->data[2] == 0xfc && skb->data[4] == 0x00) {
			skb_queue_purge(&g_fwlog->fwlog_queue);
			BTMTK_INFO_DEV(bdev, "clean fwlog_queue, skb_queue_len = %d",
					skb_queue_len(&g_fwlog->fwlog_queue));
		}

		btmtk_dispatch_fwlog_bluetooth_kpi(bdev, skb->data, skb->len, KPI_WITHOUT_TYPE);
		if (skb->len > 2) {
			bdev->main_info.dbg_send = 1;
			if (skb->data[1] == 0x6F && skb->data[2] == 0xFC) {
				if (skb->len < 6) {
					kfree_skb(skb);
					BTMTK_ERR_DEV(bdev, "%s invalid wmt cmd format", __func__);
					goto exit;
				}
				bdev->main_info.dbg_send_opcode[0] = skb->data[5];
				BTMTK_INFO_DEV(bdev, "wmt dbg_node_send_opcode is [%02x]",
						bdev->main_info.dbg_send_opcode[0]);
			} else {
				memcpy(bdev->main_info.dbg_send_opcode, (skb->data + 1), 2);
				BTMTK_INFO_DEV(bdev, "dbg_node_send_opcode is [%02x, %02x]",
						bdev->main_info.dbg_send_opcode[0],
						bdev->main_info.dbg_send_opcode[1]);
			}
		}

		ret = bdev->main_info.hif_hook.send_cmd(bdev, skb, 0, 0,
				(int)BTMTK_TX_PKT_FROM_HOST, CMD_NEED_FILTER);
		if (ret < 0) {
			BTMTK_ERR_DEV(bdev, "%s failed!!", __func__);
			kfree_skb(skb);
#if (USE_DEVICE_NODE == 0)
			/* clean opcode queue if bt is disable */
			skb_queue_purge(&g_fwlog->usr_opcode_queue);
#endif
		} else
			BTMTK_INFO_DEV(bdev, "%s: OK", __func__);
	}

exit:
	BTMTK_INFO("%s: Write end(len: %d)", __func__, len);
	ret = count;

	kfree(i_fwlog_buf);
	kfree(o_fwlog_buf);

	return ret; /* If input is correct should return the same length */

#endif // CFG_ENABLE_DEBUG_WRITE == 0
}

#if (USE_DEVICE_NODE == 1)
int btmtk_proc_uart_launcher_notify_open(struct inode *inode, struct file *file){
	BTMTK_INFO("%s: Start.", __func__);
	return 0;
}

int btmtk_proc_uart_launcher_notify_close(struct inode *inode, struct file *file){
	BTMTK_INFO("%s: Start.", __func__);
	btmtk_uart_launcher_deinit();
	BTMTK_INFO("%s: End.", __func__);
	return 0;
}

ssize_t btmtk_proc_uart_launcher_notify_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos){
	BTMTK_INFO("%s: Start.", __func__);
	return 0;
}
#endif

int btmtk_fops_openfwlog(struct inode *inode, struct file *file)
{
	struct btmtk_dev **pp_bdev = btmtk_get_pp_bdev();
	struct btmtk_dev *bdev = NULL;
	int i = 0;

	for (i = 0; i < btmtk_mcu_num; i++) {
		bdev = pp_bdev[i];
		if (bdev->hdev == NULL)
			continue;
		atomic_dec(&bdev->main_info.fwlog_ref_cnt);
	}

	BTMTK_DBG("%s: Start.", __func__);

	return 0;
}

int btmtk_fops_closefwlog(struct inode *inode, struct file *file)
{
	struct btmtk_dev **pp_bdev = btmtk_get_pp_bdev();
	struct btmtk_dev *bdev = NULL;
	int i = 0;

	for (i = 0; i < btmtk_mcu_num; i++) {
		bdev = pp_bdev[i];
		if (bdev->hdev == NULL)
			continue;
		atomic_dec(&bdev->main_info.fwlog_ref_cnt);
	}

	BTMTK_DBG("%s: Start.", __func__);

	return 0;
}

long btmtk_fops_unlocked_ioctlfwlog(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long retval = 0;
	int i = 0;
	uint8_t log_tmp = BT_FWLOG_OFF;
	struct btmtk_dev **pp_bdev = btmtk_get_pp_bdev();
	struct btmtk_dev *bdev = NULL;

	u32 in = 0, out = 0;
	struct st_hw_addr hw_addr = {0};

	if (cmd == BT_FWLOG_IOC_GET_DONGLE) {
		if (copy_from_user(&in, (u32 __user *)arg, sizeof(u32)))
			return -EIO;
	}

	down(&ioctl_mtx);
	/* mutex with bt open/close/disconnect flow */
	for (i = 0; i < btmtk_mcu_num; i++) {
		bdev = pp_bdev[i];
		if (bdev == NULL || bdev->hdev == NULL)
			continue;

		if (bdev->main_info.hif_hook.cif_mutex_lock)
			bdev->main_info.hif_hook.cif_mutex_lock(bdev);

		switch (cmd) {
		case BT_FWLOG_IOC_ON_OFF:
			/* Connsyslogger daemon dynamically enable/disable Picus log */
			BTMTK_INFO_DEV(bdev, "[ON_OFF]arg(%lu) bt_on(0x%x) log_on(0x%x) level(0x%x) log_cur(0x%x)",
					   arg, g_bt_on, g_log_on, g_log_level, g_log_current);

			log_tmp = (arg == 0) ? BT_FWLOG_OFF : BT_FWLOG_ON;
			if (log_tmp != g_log_on) { // changed
				g_log_on = log_tmp;
				g_log_current = g_log_on & g_log_level;
				if (g_bt_on) {
					retval = btmtk_intcmd_set_fw_log(bdev, g_log_current);
					btmtk_intcmd_wmt_utc_sync();
				}
			}
			break;
		case BT_FWLOG_IOC_SET_LEVEL:
			/* Connsyslogger daemon dynamically set Picus log level */
			BTMTK_INFO_DEV(bdev, "[SET_LEVEL]arg(%lu) bt_on(0x%x) log_on(0x%x) level(0x%x) log_cur(0x%x)",
					   arg, g_bt_on, g_log_on, g_log_level, g_log_current);

			log_tmp = (uint8_t)arg;
			if (log_tmp != g_log_level) {
				g_log_level = log_tmp;
				g_log_current = g_log_on & g_log_level;
				if (g_bt_on & g_log_on) {
					// driver on and log on
					retval = btmtk_intcmd_set_fw_log(bdev, g_log_current);
					btmtk_intcmd_wmt_utc_sync();
				}
			}
			break;
		case BT_FWLOG_IOC_GET_LEVEL:
			retval = g_log_level;
			BTMTK_INFO_DEV(bdev, "[GET_LEVEL]return");
			break;
		case BT_FWLOG_IOC_GET_DONGLE:
			hw_addr.hw_info = in & 0xFFFF;
			hw_addr.bt_info = (in & 0xC0000000) >> 30;

			BTMTK_INFO_DEV(bdev, "in: 0x%08x, hw_info=0x%04X, bt_info=%u", in, bdev->hw_addr.hw_info, bdev->hw_addr.bt_info);
			if (hw_addr.hw_info == bdev->hw_addr.hw_info) {
				if (bdev->hw_addr.bt_info == 0) {
					out |= bdev->hw_addr.hci;
					BTMTK_INFO_DEV(bdev, "find hci%d", bdev->hw_addr.hci);
				} else {
					if (hw_addr.bt_info == bdev->hw_addr.bt_info) {
						out |= bdev->hw_addr.hci << 8;
						BTMTK_INFO_DEV(bdev, "find hci%d", bdev->hw_addr.hci);
					}
				}
			}
			break;
		default:
			BTMTK_ERR_DEV(bdev, "Unknown cmd: 0x%08x", cmd);
			retval = -EOPNOTSUPP;
			break;
		}

		bdev->main_info.fw_log_on = g_log_current;
		if (bdev->main_info.hif_hook.cif_mutex_unlock)
			bdev->main_info.hif_hook.cif_mutex_unlock(bdev);
	}

	if (cmd == BT_FWLOG_IOC_GET_DONGLE) {
		BTMTK_INFO("Get dongle info(%08x)", out);
		if (copy_to_user((u32 __user *)arg, &out, sizeof(u32)))
			retval = -EIO;
	}

	up(&ioctl_mtx);
	return retval;
}

long btmtk_fops_compat_ioctlfwlog(struct file *filp, unsigned int cmd, unsigned long arg)
{
	BTMTK_DBG("%s: Start.", __func__);
	return btmtk_fops_unlocked_ioctlfwlog(filp, cmd, arg);
}

unsigned int btmtk_fops_pollfwlog(struct file *file, poll_table *wait)
{
	unsigned int mask = 0;
	int i = 0;
	struct btmtk_dev **pp_bdev = btmtk_get_pp_bdev();
	struct btmtk_dev *bdev = NULL;

	//if (is_mt66xx(g_sbdev->chip_id)) {

	for (i = 0; i < btmtk_mcu_num; i++) {
		bdev = pp_bdev[i];
		if (bdev == NULL || bdev->hdev == NULL)
			continue;

#if (CFG_SUPPORT_CONNSYSLOGGER_FWLOG == 1)
		poll_wait(file, &BT_log_wq, wait);
		if (btmtk_connsys_log_get_buf_size() > 0)
			mask = (POLLIN | POLLRDNORM);
#else
		poll_wait(file, &g_fwlog->fw_log_inq, wait);
		if (skb_queue_len(&g_fwlog->fwlog_queue) > 0 ||
				skb_queue_len(&g_fwlog->dumplog_queue_first) > 0 ||
				skb_queue_len(&g_fwlog->dumplog_queue_latest) > 0 ||
				skb_queue_len(&g_fwlog->dumplog_queue_wf) > 0)
			mask |= POLLIN | POLLRDNORM;			/* readable */
#endif
	}

	return mask;
}
#endif

static int btmtk_skb_enq_fwlog(struct btmtk_dev *bdev, void *src, u32 len, u8 type, struct sk_buff_head *queue)
{
	struct sk_buff *skb_tmp = NULL;
	ulong flags = 0;
	int retry = 10, index = FWLOG_TL_SIZE;

	do {
		skb_tmp = alloc_skb(len + FWLOG_PRSV_LEN, GFP_ATOMIC);
		if (skb_tmp != NULL)
			break;
		else if (retry <= 0) {
			BTMTK_ERR_DEV(bdev, "%s: alloc_skb return 0, error", __func__);
			return -ENOMEM;
		}
		BTMTK_ERR_DEV(bdev, "%s: alloc_skb return 0, error, retry = %d", __func__, retry);
	} while (retry-- > 0);

	if (type) {
		skb_tmp->data[0] = FWLOG_TYPE;
		/* 01 for dongle index */
		skb_tmp->data[index] = FWLOG_DONGLE_IDX;
		skb_tmp->data[index + 1] = sizeof(bdev->dongle_index);
		skb_tmp->data[index + 2] = bdev->dongle_index;
		index += (FWLOG_ATTR_RX_LEN_LEN + FWLOG_ATTR_TYPE_LEN);
		/* 11 for rx data*/
		skb_tmp->data[index] = FWLOG_RX;
		if (type == HCI_ACLDATA_PKT || type == HCI_EVENT_PKT || type == HCI_COMMAND_PKT) {
			skb_tmp->data[index + 1] = len & 0x00FF;
			skb_tmp->data[index + 2] = (len & 0xFF00) >> 8;
			skb_tmp->data[index + 3] = type;
			index += (HCI_TYPE_SIZE + FWLOG_ATTR_RX_LEN_LEN + FWLOG_ATTR_TYPE_LEN);
		} else {
			skb_tmp->data[index + 1] = len & 0x00FF;
			skb_tmp->data[index + 2] = (len & 0xFF00) >> 8;
			index += (FWLOG_ATTR_RX_LEN_LEN + FWLOG_ATTR_TYPE_LEN);
		}
		memcpy(&skb_tmp->data[index], src, len);
		skb_tmp->data[1] = (len + index - FWLOG_TL_SIZE) & 0x00FF;
		skb_tmp->data[2] = ((len + index - FWLOG_TL_SIZE) & 0xFF00) >> 8;
		skb_tmp->len = len + index;
	} else {
		memcpy(skb_tmp->data, src, len);
		skb_tmp->len = len;
	}

	spin_lock_irqsave(&g_fwlog->fwlog_lock, flags);
	skb_queue_tail(queue, skb_tmp);
	spin_unlock_irqrestore(&g_fwlog->fwlog_lock, flags);
	return 0;
}

int btmtk_update_bt_status(struct btmtk_dev *bdev, u8 onoff_flag)
{
	struct sk_buff *skb_onoff = NULL;
	int ret = 0;
	ulong flags = 0;

	skb_onoff = alloc_skb(FWLOG_PRSV_LEN, GFP_ATOMIC);
	if (!skb_onoff) {
		BTMTK_ERR_DEV(bdev, "%s allocate skb failed!!", __func__);
		ret = -ENOMEM;
		return ret;
	}

	BTMTK_INFO_DEV(bdev, "%s enter, falg is %d", __func__, onoff_flag);

	skb_onoff->data[0] = 0xFF;
	skb_onoff->data[1] = 0xFF;
	skb_onoff->data[2] = onoff_flag;
	skb_onoff->len = 3;
	spin_lock_irqsave(&g_fwlog->fwlog_lock, flags);
	skb_queue_tail(&g_fwlog->fwlog_queue, skb_onoff);
	spin_unlock_irqrestore(&g_fwlog->fwlog_lock, flags);
	wake_up_interruptible(&g_fwlog->fw_log_inq);

	return ret;
}

int btmtk_dump_timestamp_add(struct btmtk_dev *bdev, struct sk_buff_head *queue)
{
	struct sk_buff *skb_tmp = NULL;
	int ret = 0;
	ulong flags = 0;
	struct bt_utc_struct utc;

	skb_tmp = alloc_skb(FWLOG_PRSV_LEN, GFP_KERNEL);
	if (!skb_tmp) {
		BTMTK_ERR_DEV(bdev, "%s allocate skb failed!!", __func__);
		ret = -ENOMEM;
		return ret;
	}

	skb_tmp->data[0] = 0x6f;
	skb_tmp->data[1] = 0xfc;
	skb_tmp->data[2] = 0xff;

	BTMTK_INFO_DEV(bdev, "%s: Add timestamp", __func__);

	btmtk_getUTCtime(&utc);
	(void)snprintf((skb_tmp->data + 3), FWLOG_PRSV_LEN,
			"%d%02d%02d%02d%02d%02d",
			utc.tm.tm_year, utc.tm.tm_mon, utc.tm.tm_mday,
			utc.tm.tm_hour, utc.tm.tm_min, utc.tm.tm_sec);
	skb_tmp->len = 17;

	spin_lock_irqsave(&g_fwlog->fwlog_lock, flags);
	skb_queue_tail(queue, skb_tmp);
	spin_unlock_irqrestore(&g_fwlog->fwlog_lock, flags);
	wake_up_interruptible(&g_fwlog->fw_log_inq);
	return ret;
}

int btmtk_dispatch_fwlog_bluetooth_kpi(struct btmtk_dev *bdev, u8 *buf, int len, u8 type)
{
	static u8 fwlog_blocking_warn;
	int ret = 0;

	if (!g_fwlog)
		return ret;

	if (g_fwlog->btmtk_bluetooth_kpi &&
		skb_queue_len(&g_fwlog->fwlog_queue) < FWLOG_BLUETOOTH_KPI_QUEUE_COUNT) {
		/* sent event to queue, picus tool will log it for bluetooth KPI feature */
		if (btmtk_skb_enq_fwlog(bdev, buf, len, type, &g_fwlog->fwlog_queue) == 0) {
			wake_up_interruptible(&g_fwlog->fw_log_inq);
			fwlog_blocking_warn = 0;
		}
	} else {
		if (fwlog_blocking_warn == 0) {
			fwlog_blocking_warn = 1;
			BTMTK_WARN_DEV(bdev, "fwlog queue size is full(bluetooth_kpi)");
		}
	}
	return ret;
}

#if BT_SWIP_SUPPORT
int btmtk_recv_event_to_vendor(struct btmtk_dev *bdev, struct sk_buff *skb)
{
	int ret = 0;
	struct sk_buff *skb_temp;

	/* Copy LE audio event send to bt vendor ko*/
	if (atomic_read(&vnd_module_registered) == BTMTK_VND_KO_REG) {
		skb_temp = skb_copy(skb, GFP_ATOMIC);
		if (!skb_temp) {
			BTMTK_ERR_DEV(bdev, "%s, skb_copy event is fail!", __func__);
			return -ENOMEM;
		}

		/*filter LE audio event, only complete event clear opcode value*/
		if (atomic_read(&swip_opcode_flag) == 1 &&
			skb->data[3] == swip_opcode[0] && skb->data[4] == swip_opcode[1]) {
			if (skb->data[0] == 0x0E) {
				memset(swip_opcode, 0, sizeof(swip_opcode));
				atomic_set(&swip_opcode_flag, 0);
			}
			BTMTK_DBG_DEV(bdev, "%s, don't send to host! Opcode=%02x%02x", __func__,
				skb->data[3], skb->data[4]);
			ret = 1;
		}
		if (evt_cb)
			evt_cb(skb_temp);
		kfree_skb(skb_temp);
	}
	return ret;
}
#endif

/* if modify the common part, please sync to another btmtk_dispatch_fwlog */
#if (USE_DEVICE_NODE == 0)
int btmtk_dispatch_fwlog(struct btmtk_dev *bdev, struct sk_buff *skb)
{
	static u8 fwlog_picus_blocking_warn;
	static u8 fwlog_fwdump_blocking_warn;
	static u8 wf_fwlog_fwdump_blocking_warn;
	int state = 0, header_len;
	struct sk_buff *skb_opcode = NULL;
	struct data_struct hci_reset_event = {0};
	static struct sk_buff_head *dumplog_queue = NULL, *wf_dumplog_queue = NULL;

	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return -1;
	}

	BTMTK_DBG_DEV(bdev, "%s enter", __func__);

	if (!g_fwlog)
		return 0;

	state = btmtk_get_chip_state(bdev);
	if (state == BTMTK_STATE_DISCONNECT) {
		BTMTK_ERR_DEV(bdev, "%s: state is disconnect, return", __func__);
		return 0;
	}

	BTMTK_GET_CMD_OR_EVENT_DATA(bdev, HCI_RESET_EVT, hci_reset_event);
	if (hci_reset_event.len < MTK_HCI_EVENT_HEAD_SIZE || hci_reset_event.len > HCI_MAX_FRAME_SIZE) {
		BTMTK_INFO_RAW(bdev, hci_reset_event.content, hci_reset_event.len,
					"%s: get hci reset event failed:", __func__);
		return -1;
	}

	if ((bt_cb(skb)->pkt_type == HCI_ACLDATA_PKT) &&
			skb->data[0] == 0x6f &&
			skb->data[1] == 0xfc) {
		static int dump_data_counter;
		static int dump_data_length;

		state = btmtk_get_chip_state(bdev);
		if (state != BTMTK_STATE_FW_DUMP) {
			BTMTK_INFO_DEV(bdev, "%s: FW dump begin", __func__);
			DUMP_TIME_STAMP("FW_dump_start");
			btmtk_hci_snoop_print_to_log(bdev);
			/* Print too much log, it may cause kernel panic. */
			dump_data_counter = 0;
			dump_data_length = 0;
			btmtk_set_chip_state(bdev, BTMTK_STATE_FW_DUMP);
			if (!btmtk_assert_wake_lock_check(bdev))
				btmtk_assert_wake_lock(bdev);
			if (!btmtk_reset_timer_check(bdev))
				btmtk_reset_timer_update(bdev);

			BTMTK_INFO_DEV(bdev, "dumplog_queue_first, skb_queue_len = %d",
					skb_queue_len(&g_fwlog->dumplog_queue_first));

			if (skb_queue_len(&g_fwlog->dumplog_queue_first) == 0) {
				BTMTK_INFO_DEV(bdev, "first coredump, save...");
				dumplog_queue = &g_fwlog->dumplog_queue_first;
			} else {
				BTMTK_INFO_DEV(bdev, "clean dumplog_queue_latest, skb_queue_len = %d",
						skb_queue_len(&g_fwlog->dumplog_queue_latest));
				skb_queue_purge(&g_fwlog->dumplog_queue_latest);
				dumplog_queue = &g_fwlog->dumplog_queue_latest;
			}
		}
#ifdef CHIP_IF_USB
		if ((bdev->chip_reset_signal & 0x8) == 0) {
			btmtk_dump_timestamp_add(bdev, dumplog_queue);
			bdev->chip_reset_signal |= 0x08;
		}
#endif
		dump_data_counter++;
		dump_data_length += skb->len - 4;

		/* coredump */
		/* print dump data to console */
		if (dump_data_counter % 1000 == 0) {
			BTMTK_INFO_DEV(bdev, "%s: FW dump on-going, total_packet = %d, total_length = %d",
					__func__, dump_data_counter, dump_data_length);
		}

		/* print dump data to console */
		if (dump_data_counter < 20)
			BTMTK_INFO_DEV(bdev, "%s: FW dump data (%d): %s",
					__func__, dump_data_counter, &skb->data[4]);

		/* In the new generation, we will check the keyword of coredump (; coredump end)
		 * Such as : 79xx
		 */
		if (is_mt7663(bdev->chip_id))
			header_len = 5;
		else
			header_len = 4;

		if (skb->data[skb->len - header_len] == 'e' &&
			skb->data[skb->len - header_len + 1] == 'n' &&
			skb->data[skb->len - header_len + 2] == 'd') {
			/* This is the latest coredump packet. */
			BTMTK_INFO_DEV(bdev, "%s: FW dump end, dump_data_counter = %d, total_length = %d",
					__func__, dump_data_counter, dump_data_length);
			/* TODO: Chip reset*/
			bdev->main_info.reset_stack_flag = HW_ERR_CODE_CORE_DUMP;
			DUMP_TIME_STAMP("FW_dump_end");
			if (bdev->main_info.hif_hook.waker_notify)
				bdev->main_info.hif_hook.waker_notify(bdev);

#ifdef CHIP_IF_USB
			/* Clear 4th bit when received coredump end  */
			bdev->chip_reset_signal &= ~0x08;

			/* USB hif: recv both 0xff and fw dump end, then do chip reset */
			if (bdev->dualBT) {
				/* If it is dual BT,
				 * the condition for FW dump end is that both BT0 and BT1 are completed.
				 */
				if (bdev->chip_reset_signal & (1 << 1)) {
					bdev->chip_reset_signal |= (1 << 2);
					BTMTK_INFO_DEV(bdev, "%s, BT1 FW dump end, chip_reset_signal = %02x",
							__func__, bdev->chip_reset_signal);
					if (bdev->chip_reset_signal == 0x07) {
						bdev->chip_reset_signal = 0x00;
						DUMP_TIME_STAMP("notify_chip_reset");
						btmtk_reset_trigger(bdev);
					}
				} else {
					bdev->chip_reset_signal |= (1 << 1);
					BTMTK_INFO_DEV(bdev, "%s, BT0 FW dump end, chip_reset_signal = %02x",
							__func__, bdev->chip_reset_signal);
				}
			} else {
				bdev->chip_reset_signal |= (1 << 1);
				BTMTK_INFO_DEV(bdev, "%s ,chip_reset_signal = %02x", __func__, bdev->chip_reset_signal);
				if (bdev->chip_reset_signal == 0x03 || is_mt7663(bdev->chip_id)) {
					bdev->chip_reset_signal = 0x00;
					DUMP_TIME_STAMP("notify_chip_reset");
					btmtk_reset_trigger(bdev);
				}
			}

#endif
		}

		if (skb_queue_len(dumplog_queue) < FWLOG_ASSERT_QUEUE_COUNT) {
			/* sent picus data to queue, picus tool will log it */
			if (btmtk_skb_enq_fwlog(bdev, skb->data, skb->len, 0, dumplog_queue) == 0) {
				wake_up_interruptible(&g_fwlog->fw_log_inq);
				fwlog_fwdump_blocking_warn = 0;
			}
		} else {
			if (fwlog_fwdump_blocking_warn == 0) {
				fwlog_fwdump_blocking_warn = 1;
				BTMTK_WARN_DEV(bdev, "btmtk fwlog queue size is full(coredump)");
			}
		}

		/* change coredump's ACL handle to FF F0 */
		skb->data[0] = 0xFF;
		skb->data[1] = 0xF0;
	} else if ((bt_cb(skb)->pkt_type == HCI_ACLDATA_PKT) &&
			skb->data[0] == 0x6e &&
			skb->data[1] == 0xfc) {
		BTMTK_INFO_DEV(bdev, "get WF coredump");

		wf_dumplog_queue = &g_fwlog->dumplog_queue_wf;
		if (btmtk_skb_enq_fwlog(bdev, skb->data, skb->len, 0, wf_dumplog_queue) == 0) {
			wf_fwlog_fwdump_blocking_warn = 0;
		} else {
			if (wf_fwlog_fwdump_blocking_warn == 0) {
				wf_fwlog_fwdump_blocking_warn = 1;
				BTMTK_WARN_DEV(bdev, "btmtk wf fwlog queue size is full(coredump)");
			}
		}

		/* change coredump's ACL handle to FF F1 */
		skb->data[0] = 0xFF;
		skb->data[1] = 0xF0;
	} else if ((bt_cb(skb)->pkt_type == HCI_ACLDATA_PKT) &&
				(skb->data[0] == 0xff || skb->data[0] == 0xfe) &&
				skb->data[1] == 0x05 &&
				!bdev->bt_cfg.support_picus_to_host) {
		/* picus or syslog */
		if (skb_queue_len(&g_fwlog->fwlog_queue) < FWLOG_QUEUE_COUNT) {
			if (btmtk_skb_enq_fwlog(bdev, skb->data, skb->len,
				FWLOG_TYPE, &g_fwlog->fwlog_queue) == 0) {
				wake_up_interruptible(&g_fwlog->fw_log_inq);
				fwlog_picus_blocking_warn = 0;
			}
		} else {
			if (fwlog_picus_blocking_warn == 0) {
				fwlog_picus_blocking_warn = 1;
				BTMTK_INFO_DEV(bdev, "btmtk fwlog queue size is full(picus)");
			}
		}
		return 1;
	} else if (memcmp(skb->data, &hci_reset_event.content[1], hci_reset_event.len - 1) == 0) {
		BTMTK_INFO_DEV(bdev, "%s: Get RESET_EVENT", __func__);
		bdev->get_hci_reset = 1;
		atomic_set(&bdev->main_info.subsys_reset_conti_count, 0);
	}

#if BT_SWIP_SUPPORT
	if (bt_cb(skb)->pkt_type == HCI_EVENT_PKT) {
		/* Copy LE audio event send to bt vendor ko*/
		if (btmtk_recv_event_to_vendor(bdev, skb) != 0)
			return 1;
	}
#endif

	/* filter event from usr cmd */
	if ((bt_cb(skb)->pkt_type == HCI_EVENT_PKT) &&
			skb->data[0] == 0x0E) {
		if (skb_queue_len(&g_fwlog->usr_opcode_queue)) {
			BTMTK_INFO_DEV(bdev, "%s: opcode queue len is %d", __func__,
					skb_queue_len(&g_fwlog->usr_opcode_queue));
			skb_opcode = skb_dequeue(&g_fwlog->usr_opcode_queue);
		}

		if (skb_opcode == NULL)
			return 0;

		if (skb_opcode->data[0] == skb->data[3] &&
					skb_opcode->data[1] == skb->data[4]) {
			BTMTK_INFO_RAW(bdev, skb->data, skb->len, "%s: recv event from user hci command - ", __func__);
			// should return to upper layer tool
			if (btmtk_skb_enq_fwlog(bdev, skb->data, skb->len, bt_cb(skb)->pkt_type,
						&g_fwlog->fwlog_queue) == 0) {
				wake_up_interruptible(&g_fwlog->fw_log_inq);
			}
			kfree_skb(skb_opcode);
			return 1;
		}

		BTMTK_DBG_DEV(bdev, "%s: check opcode fail!", __func__);
		skb_queue_head(&g_fwlog->usr_opcode_queue, skb_opcode);
	}

	return 0;
}

#else // #if (USE_DEVICE_NODE == 0)

/* if modify the common part, please sync to another btmtk_dispatch_fwlog*/
int btmtk_dispatch_fwlog(struct btmtk_dev *bdev, struct sk_buff *skb)
{
	char xml_log[CONNV3_XML_SIZE] = {0};
	int state = BTMTK_STATE_INIT;
	struct connv3_issue_info issue_info;
	int ret = 0, line = 0;
	static unsigned int fwlog_count;
	int drv = CONNV3_DRV_TYPE_BT;

	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return -1;
	}

	if ((bt_cb(skb)->pkt_type == HCI_ACLDATA_PKT) &&
			skb->data[0] == 0x6f &&
			skb->data[1] == 0xfc) {
		static int dump_data_counter;
		static int dump_data_length;

		if (atomic_read(&bdev->assert_state) == BTMTK_ASSERT_END) {
			BTMTK_INFO_LIMITTED("%s: coredump already end, skip data", __func__);
			return 1;
		}

		/* remove acl header 6F FC LL LL */
		skb_pull(skb, 4);

		state = btmtk_get_chip_state(bdev);
		/* BTMTK_STATE_FW_DUMP for coredump start
		 * BTMTK_STATE_SUBSYS_RESET for coredump end
		 */
		if (state != BTMTK_STATE_FW_DUMP && state != BTMTK_STATE_SUBSYS_RESET) {
			BTMTK_INFO_DEV(bdev, "%s: msg: %s len[%d]", __func__, skb->data, skb->len);
			/* drop "Disable Cache" */
			if (skb->len > 6 &&
				skb->data[skb->len - 6] == 'C' &&
				skb->data[skb->len - 5] == 'a' &&
				skb->data[skb->len - 4] == 'c' &&
				skb->data[skb->len - 3] == 'h' &&
				skb->data[skb->len - 2] == 'e') {
				BTMTK_INFO_DEV(bdev, "%s: drop Cache", __func__);
				return 1;
			}

			/* drop "bt radio off" */
			if (skb->len > 10 &&
				skb->data[skb->len - 10] == 'r' &&
				skb->data[skb->len - 9] == 'a' &&
				skb->data[skb->len - 8] == 'd' &&
				skb->data[skb->len - 7] == 'i' &&
				skb->data[skb->len - 6] == 'o' &&
				skb->data[skb->len - 5] == ' ' &&
				skb->data[skb->len - 4] == 'o' &&
				skb->data[skb->len - 3] == 'f' &&
				skb->data[skb->len - 2] == 'f') {
				BTMTK_INFO_DEV(bdev, "%s: drop radio off", __func__);
				return 1;
			}

			/* fw trigger
			 *	reset[0]: subsys reset
			 *	reset[1]: PMIC => call connv3_trigger_pmic_irq
			 *	reset[2]: whole chip reset (DFD)
			 */
			if (skb->len > 9 &&
				skb->data[skb->len - 9] == 'r' &&
				skb->data[skb->len - 8] == 'e' &&
				skb->data[skb->len - 7] == 's' &&
				skb->data[skb->len - 6] == 'e' &&
				skb->data[skb->len - 5] == 't' &&
				skb->data[skb->len - 4] == '[') {
					if (skb->data[skb->len - 3] == '0') {
						BTMTK_INFO_DEV(bdev, "%s: drop subsys type", __func__);
					} else if (skb->data[skb->len - 3] == '1') {
						BTMTK_INFO_DEV(bdev, "%s: FW trigger PMIC fault", __func__);
						bdev->main_info.chip_reset_flag = 1;
						bdev->reset_type = CONNV3_CHIP_RST_TYPE_PMIC_FAULT_B;
						if (bdev->assert_reason[0] == '\0') {
							memset(bdev->assert_reason, 0, ASSERT_REASON_SIZE);
							strncpy(bdev->assert_reason, "[BT_FW assert] trigger PMIC fault",
								strlen("[BT_FW assert] trigger PMIC fault") + 1);
						}
						BTMTK_ERR_DEV(bdev, "%s: [assert_reason] %s",
									__func__, bdev->assert_reason);
					} else {
						bdev->main_info.chip_reset_flag = 1;
						bdev->reset_type = CONNV3_CHIP_RST_TYPE_DFD_DUMP;
						if (bdev->assert_reason[0] == '\0') {
							memset(bdev->assert_reason, 0, ASSERT_REASON_SIZE);
							strncpy(bdev->assert_reason, "[BT_FW assert] trigger whole chip reset",
									strlen("[BT_FW assert] trigger whole chip reset") + 1);
						}
						BTMTK_ERR_DEV(bdev, "%s: [assert_reason] %s",
									__func__, bdev->assert_reason);
					}
					return 1;
			}

			DUMP_TIME_STAMP("FW_dump_start");
			if (bdev->assert_reason[0] == '\0') {
				memset(bdev->assert_reason, 0, ASSERT_REASON_SIZE);
				if (snprintf(bdev->assert_reason, ASSERT_REASON_SIZE, "[BT_FW assert] %s", skb->data) < 0)
					strncpy(bdev->assert_reason, "[BT_FW assert]", strlen("[BT_FW assert]") + 1);
				BTMTK_ERR_DEV(bdev, "%s: [assert_reason] %s", __func__, bdev->assert_reason);
				btmtk_hci_snoop_print_to_log(bdev);
			}

			/* Print too much log, it may cause kernel panic. */
			dump_data_counter = 0;
			dump_data_length = 0;
			if (bdev->main_info.hif_hook.coredump_handler == NULL) {
				BTMTK_ERR_DEV(bdev, "%s: coredump_handler is NULL", __func__);
				return 1;
			}

			btmtk_sp_coredump_start();

			btmtk_set_chip_state(bdev, BTMTK_STATE_FW_DUMP);
			if (!btmtk_assert_wake_lock_check(bdev))
				btmtk_assert_wake_lock(bdev);
			if (!btmtk_reset_timer_check(bdev))
				btmtk_reset_timer_update(bdev);
			line = __LINE__;
			bdev->collect_fwdump = TRUE;
			if (strstr(bdev->assert_reason, "[BT_FW assert]"))
				drv = CONNV3_DRV_TYPE_MAX;

			ret = connv3_coredump_start(
					bdev->main_info.hif_hook.coredump_handler, drv,
					bdev->assert_reason, skb->data, bdev->main_info.fw_version_str);

			if(ret == CONNV3_COREDUMP_ERR_CHIP_RESET_ONLY) {
				BTMTK_ERR_DEV(bdev, "%s: not collect fw dump, only do reset", __func__);
				bdev->collect_fwdump = FALSE;
				return 1;
			}
			if (ret == CONNV3_COREDUMP_ERR_WRONG_STATUS) {
				BTMTK_ERR_DEV(bdev, "%s: BT previous not end", __func__);
				connv3_coredump_end(bdev->main_info.hif_hook.coredump_handler, "BT previous not end");
				if (strstr(bdev->assert_reason, "[BT_FW assert]"))
					drv = CONNV3_DRV_TYPE_MAX;

				ret = connv3_coredump_start(
					bdev->main_info.hif_hook.coredump_handler, drv,
					bdev->assert_reason, skb->data, bdev->main_info.fw_version_str);
			}
			if (ret)
				goto coredump_fail_unlock;
		}

		dump_data_counter++;
		dump_data_length += skb->len;

		/* coredump content*/
		/* print dump data to console */
		if (dump_data_counter % 1000 == 0) {
			BTMTK_INFO_DEV(bdev, "%s: FW dump on-going, total_packet = %d, total_length = %d",
					__func__, dump_data_counter, dump_data_length);
		}

		/* print dump data to console */
		if (dump_data_counter < 20)
			BTMTK_INFO_DEV(bdev, "%s: FW dump data (%d): %s",
					__func__, dump_data_counter, skb->data);

		line = __LINE__;
		if (bdev->collect_fwdump)
			ret = connv3_coredump_send(bdev->main_info.hif_hook.coredump_handler,
				"[M]", skb->data, skb->len);
		if (ret) {
			BTMTK_INFO_RAW(bdev, skb->data, skb->len, "%s: send fail, len[%d]", __func__, skb->len);
			goto coredump_fail_unlock;
		}
		/* In the new generation, we will check the keyword of coredump (; coredump end)
		 * Such as : 79xx
		 */
		if (skb->len > 6 &&
			skb->data[skb->len - 6] == 'p' &&
			skb->data[skb->len - 5] == ' ' &&
			skb->data[skb->len - 4] == 'e' &&
			skb->data[skb->len - 3] == 'n' &&
			skb->data[skb->len - 2] == 'd') {
			/* TODO: Chip reset*/
			bdev->main_info.reset_stack_flag = HW_ERR_CODE_CORE_DUMP;
			DUMP_TIME_STAMP("FW_dump_end");
			line = __LINE__;
			ret = connv3_coredump_get_issue_info(bdev->main_info.hif_hook.coredump_handler,
								&issue_info, xml_log, CONNV3_XML_SIZE);
			if (ret)
				goto coredump_fail_unlock;
			BTMTK_INFO_DEV(bdev, "%s: xml_log: %s, assert_info: %s, task_name: %s ",
						__func__, xml_log, issue_info.assert_info, issue_info.task_name);
			line = __LINE__;
			ret = connv3_coredump_send(bdev->main_info.hif_hook.coredump_handler,
							"INFO", xml_log, strlen(xml_log));
			if (ret)
				goto coredump_fail_unlock;
			/* This is the latest coredump packet. */
			BTMTK_INFO_DEV(bdev, "%s: FW dump end, dump_data_counter[%d], dump_data_length[%d]",
						__func__, dump_data_counter, dump_data_length);

			if (bdev->reset_type != CONNV3_CHIP_RST_TYPE_DFD_DUMP) {
				btmtk_sp_coredump_end();

				/* if do complete and bt close with btmtk_reset_waker start
				 * no need to wait hw_err event, cause bt already start close
				 */

				BTMTK_INFO_DEV(bdev, "%s: complete dump_comp , coredump_end", __func__);
				complete_all(&bdev->dump_comp);

				if (bdev->main_info.hif_hook.waker_notify)
					bdev->main_info.hif_hook.waker_notify(bdev);
			} else {
				BTMTK_INFO_DEV(bdev, "%s: Not call coredump_end, DFD pre-dump, set bt assert_state end",
							__func__);
				atomic_set(&bdev->assert_state, BTMTK_ASSERT_END);
				BTMTK_INFO_DEV(bdev, "%s: DFD pre-dump, complete dump_dfd_comp , coredump_end",
							__func__);
				complete_all(&bdev->dump_dfd_comp);
			}
		}

		return 1;

coredump_fail_unlock:
		BTMTK_ERR_DEV(bdev, "%s: coredump API fail ret[%d] line[%d]", __func__, ret, line);
		return 1;
#if (CFG_SUPPORT_CONNSYSLOGGER_FWLOG == 1)
	} else if ((bt_cb(skb)->pkt_type == HCI_ACLDATA_PKT) &&
				(skb->data[0] == 0xff || skb->data[0] == 0xfe) && skb->data[1] == 0x05) {

		unsigned long start_time = jiffies;
		/* send to log handler, and remove acl header (FF 05 LL LL) */
		btmtk_connsys_log_handler(skb->data + 4, skb->len - 4);
		if (time_after(jiffies, start_time + msecs_to_jiffies(100)))
			BTMTK_INFO("%s btmtk_connsys_log_handler, it costs %u ms", __func__, jiffies_to_msecs(jiffies - start_time));

		/* picus or syslog */
		fwlog_count++;


		/* host buffer is not enough, reduce payload to 8 bytes */
		if (!is_rx_queue_res_available(RING_BUFFER_SIZE >> 2)) {
			u8 log_lost_payload[] = {0x08, 0x00, 0x01, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00};

			memcpy(&skb->data[2], &log_lost_payload[0], sizeof(log_lost_payload));
			/* Add acl header len: (FF 05) 08 00 ... */
			skb->len = sizeof(log_lost_payload) + 2;
			BTMTK_DBG_RAW(bdev, skb->data, skb->len, "%s: FW log lost:", __func__);
		}

		/* Drop the firmware log during the suspend flow to avoid waking up the system. */
		if (atomic_read(&bdev->main_info.suspend_entry)) {
			BTMTK_INFO_RAW(bdev, skb->data, skb->len, "%s: drop FW log in suspend flow:", __func__);
			return 1;
		}

#if (CFG_SUPPORT_FWLOG_TO_HOST == 1)
		/* send fw log to host */
		BTMTK_INFO_LIMITTED("fw log counter[%d], log_level[%d], send to hci log",
				fwlog_count, bdev->main_info.fw_log_on);
		return 0;
#else
		/* not send fw log to host */
		BTMTK_INFO_LIMITTED("fw log counter[%d], log_level[%d], store in fw log",
				fwlog_count, bdev->main_info.fw_log_on);
		return 1;
#endif // CFG_SUPPORT_FWLOG_TO_HOST
#endif // CFG_SUPPORT_CONNSYSLOGGER_FWLOG
	} else if ((bt_cb(skb)->pkt_type == HCI_ACLDATA_PKT) &&
				(skb->data[0] == 0xff || skb->data[0] == 0xfe) &&
				skb->data[1] == 0x06 && bdev->main_info.hif_hook.met_log_handler) {
		skb_pull(skb, 4);
		bdev->main_info.hif_hook.met_log_handler(bdev, skb->data, skb->len);
		return 1;
	} else if ((bt_cb(skb)->pkt_type == HCI_EVENT_PKT) &&
				skb->len > 7 && skb->data[0] == 0xFF &&
				skb->data[3] == 0x3A && skb->data[4] == 0xFC) {
		BTMTK_INFO_RAW(bdev, skb->data, skb->len, "%s: FW Schedule Event:", __func__);
				/* Send Fw Schedule Event to host */
		return 0;
	} else if ((bt_cb(skb)->pkt_type == HCI_EVENT_PKT) &&
				skb->len > 7 && skb->data[0] == 0xFF &&
				skb->data[1] == 0x02 && skb->data[2] == 0x20) {
		BTMTK_INFO_RAW(bdev, skb->data, skb->len, "%s: Tx power abnormal Event:", __func__);
		switch (skb->data[3]) {
		case 0x00:
			BTMTK_INFO_DEV(bdev, "%s: RF0 TSSI offset too large", __func__);
			conn_dbg_add_log(CONN_DBG_LOG_TYPE_HW_ERR, "[bt] RF0 TSSI offset too large");
			break;
		case 0x01:
			BTMTK_INFO_DEV(bdev, "%s: RF1 TSSI offset too large", __func__);
			conn_dbg_add_log(CONN_DBG_LOG_TYPE_HW_ERR, "[bt] RF1 TSSI offset too large");
			break;
		case 0x02:
			BTMTK_INFO_DEV(bdev, "%s: RF0 TX chipout power abnormal", __func__);
			conn_dbg_add_log(CONN_DBG_LOG_TYPE_HW_ERR, "[bt] RF0 TX chipout power abnormal");
			break;
		case 0x03:
			BTMTK_INFO_DEV(bdev, "%s: RF1 TX chipout power abnormal", __func__);
			conn_dbg_add_log(CONN_DBG_LOG_TYPE_HW_ERR, "[bt] RF1 TX chipout power abnormal");
			break;
		case 0x04:
			BTMTK_INFO_DEV(bdev, "%s: RF0 TX voltage abnormal", __func__);
			conn_dbg_add_log(CONN_DBG_LOG_TYPE_HW_ERR, "[bt] RF0 TX voltage abnormal");
			break;
		case 0x05:
			BTMTK_INFO_DEV(bdev, "%s: RF1 TX voltage abnormal", __func__);
			conn_dbg_add_log(CONN_DBG_LOG_TYPE_HW_ERR, "[bt] RF1 TX voltage abnormal");
			break;
		}
		aee_kernel_warning("bt thermal", "BT power abnormal\n");
		return 1;
	} else if ((bt_cb(skb)->pkt_type == HCI_EVENT_PKT) &&
		skb->data[0] == 0xFF && skb->data[2] == 0x21) {
		BTMTK_INFO_RAW(bdev, skb->data, skb->len, "%s: Tx power abnormal Event 2:", __func__);
		switch (skb->data[3] & 0x0F) {
		case 0x00:
			BTMTK_INFO_RAW(bdev, skb->data, skb->len, "%s: [0x00] power abnormal Event", __func__);
			conn_dbg_add_log(CONN_DBG_LOG_TYPE_HW_ERR, "[bt] RF0 TSSI offset too large");
			break;
		case 0x01:
			BTMTK_INFO_RAW(bdev, skb->data, skb->len, "%s: [0x01] power abnormal Event", __func__);
			conn_dbg_add_log(CONN_DBG_LOG_TYPE_HW_ERR, "[bt] RF1 TSSI offset too large");
			break;
		case 0x02:
			BTMTK_INFO_RAW(bdev, skb->data, skb->len, "%s: [0x02] power abnormal Event", __func__);
			conn_dbg_add_log(CONN_DBG_LOG_TYPE_HW_ERR, "[bt] RF0 TX chipout power abnormal");
			break;
		}
		if (skb->data[3] & 0x80)
			aee_kernel_warning("combo_bt", "BT power abnormal!\n");
		return 1;
	} else if (bt_cb(skb)->pkt_type == HCI_EVENT_PKT && bdev->main_info.dbg_send
				&& skb->len > 4 &&
				skb->data[3] == bdev->main_info.dbg_send_opcode[0] &&
				skb->data[4] == bdev->main_info.dbg_send_opcode[1]) {
		bdev->main_info.dbg_send = 0;
		BTMTK_INFO_RAW(bdev, skb->data, skb->len, "%s: dbg_node_event len[%d] %02x ", __func__,
					skb->len + 1, hci_skb_pkt_type(skb));
		return 1;
	} else if (bt_cb(skb)->pkt_type == HCI_EVENT_PKT && skb->len > 4
				&& skb->data[0] == 0xE4 && skb->data[1] == 0x05 &&
				skb->data[2] == 0x02 &&  skb->data[3] == 0x56 && bdev->main_info.hif_hook.fw_sleep_handler) {
		BTMTK_DBG_RAW(bdev, skb->data, skb->len, "%s: fw_sleep_event len[%d] %02x ", __func__,
					skb->len + 1, hci_skb_pkt_type(skb));
		bdev->main_info.hif_hook.fw_sleep_handler(bdev);
		return 1;
	} else if (bt_cb(skb)->pkt_type == HCI_EVENT_PKT && skb->data[0] == 0xE3 && skb->data[2] == 0x01) {
		unsigned long start_time = jiffies;
		/* fw log flow control feature */
		BTMTK_DBG_RAW(bdev, skb->data, skb->len, "%s: fwlog_flow_ctrl evt len[%d] %02x ",
				__func__, skb->len + 1, hci_skb_pkt_type(skb));

		mutex_lock(&bdev->main_info.fwlog_ctrl_lock);
		/* parse payload length */
		bdev->main_info.fwlog_flow_ctrl_data_len = skb->data[1];

		/* error handle */
		if (bdev->main_info.fwlog_flow_ctrl_data_len > FWLOG_FLOW_CTRL_DATA_MAX_LEN) {
			BTMTK_WARN_DEV(bdev, "%s: fwlog_flow_ctrl_data_len(%d) more than %d, skip",
					__func__, bdev->main_info.fwlog_flow_ctrl_data_len, FWLOG_FLOW_CTRL_DATA_MAX_LEN);
			mutex_unlock(&bdev->main_info.fwlog_ctrl_lock);
			return 1;
		}

		/* record packet data and send ack cmd to fw */
		memcpy(&bdev->main_info.fwlog_flow_ctrl_data[0], &skb->data[2],
				bdev->main_info.fwlog_flow_ctrl_data_len);
		mutex_unlock(&bdev->main_info.fwlog_ctrl_lock);

		/* ack cmd to fw */
		ret = schedule_work(&bdev->main_info.fwlog_flow_ctrl_work);

		if (!ret)
			BTMTK_DBG_DEV(bdev, "%s: schedule_work already in the queue or executing", __func__);

		if (time_after(jiffies, start_time + msecs_to_jiffies(100)))
			BTMTK_INFO("%s fw log flow control, it costs %u ms", __func__, jiffies_to_msecs(jiffies - start_time));
		return 1;
	} else if (bt_cb(skb)->pkt_type == HCI_EVENT_PKT && skb->data[0] == 0xE3) {
		BTMTK_INFO_RAW(bdev, skb->data, skb->len, "%s: get E3 event and drop it, len[%d] %02x",
				__func__, skb->len + 1, hci_skb_pkt_type(skb));

		return 1;
	} else if ((bt_cb(skb)->pkt_type == HCI_EVENT_PKT) &&
				skb->len == 7 && skb->data[0] == 0x0E &&
				skb->data[1] == 0x05 && skb->data[3] == 0x64 && skb->data[4] == 0xFD) {
		BTMTK_INFO_RAW(bdev, skb->data, skb->len, "%s: FW FMD reset complete event: %02x ",
				__func__, hci_skb_pkt_type(skb));
		bdev->fmd_reset_state = FMD_RESET_STATE_DONE;
		return 1;
	}
	return 0;
}
#endif // #else (USE_DEVICE_NODE == 1)
