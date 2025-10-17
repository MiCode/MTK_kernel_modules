/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "btmtk_fw_log.h"

#if (USE_DEVICE_NODE == 1)
#include "btmtk_proj_sp.h"
#include "conn_dbg.h"
#include "aee.h"
#endif

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

__weak int32_t btmtk_intcmd_wmt_utc_sync(void)
{
	BTMTK_ERR("weak function %s not implement", __func__);
	return -1;
}

__weak int32_t btmtk_intcmd_set_fw_log(uint8_t flag)
{
	BTMTK_ERR("weak function %s not implement", __func__);
	return -1;
}

__weak int btmtk_uart_launcher_deinit(void)
{
	BTMTK_ERR("weak function %s not implement", __func__);
	return -1;
}

__weak int btmtk_intcmd_wmt_blank_status(unsigned char blank_state)
{
	BTMTK_ERR("weak function %s not implement", __func__);
	return -1;
}

void fw_log_bt_state_cb(uint8_t state)
{
	uint8_t on_off;
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();
	struct btmtk_dev **pp_bdev = btmtk_get_pp_bdev();
	int hci_idx = 0;

	/* sp use BTMTK_FOPS_STATE_OPENED to judge state */
	on_off = (state == FUNC_ON) ? BT_FWLOG_ON : BT_FWLOG_OFF;
	BTMTK_INFO_LIMITTED("%s: current_log(0x%x) current_bt_on(0x%x) state_cb(%d) need_on_off(0x%x)",
				__func__, g_log_current, g_bt_on, state, on_off);

	if (pp_bdev[hci_idx] == NULL) {
		BTMTK_WARN("%s: pp_bdev[%d] == NULL", __func__, hci_idx);
		return;
	}

	if (g_bt_on != on_off) {
		// changed
		if (on_off == BT_FWLOG_OFF) { // should turn off
			g_bt_on = BT_FWLOG_OFF;
			g_log_current = g_log_on & g_log_level;
			BTMTK_INFO("BT in off state, no need to send close fw log cmd");
		} else {
			g_bt_on = BT_FWLOG_ON;
			if (g_log_current) {
				btmtk_intcmd_set_fw_log(g_log_current);
				/* if bt open or reset when screen off, still need to notify fw blank status */
				btmtk_intcmd_wmt_blank_status(pp_bdev[hci_idx]->blank_state);
				btmtk_intcmd_wmt_utc_sync();
			}
		}
	}
	bmain_info->fw_log_on = g_log_current;
}

void fw_log_bt_event_cb(void)
{
	wake_up_interruptible(&BT_log_wq);
}

static int btmtk_proc_show(struct seq_file *m, void *v)
{
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();

	if (strlen(bmain_info->fw_version_str))
		(void)seq_printf(m, "patch version:%s\ndriver version:%s\n", bmain_info->fw_version_str, VERSION);
	else
		(void)seq_printf(m, "patch version:null\ndriver version:%s\n", VERSION);
	return 0;
}

static int btmtk_proc_chip_reset_count_show(struct seq_file *m, void *v)
{
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();

	(void)seq_printf(m, "whole_reset_count=%d subsys_reset_count=%d\n",
		atomic_read(&bmain_info->whole_reset_count),
		atomic_read(&bmain_info->subsys_reset_count));
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

static void btmtk_proc_create_new_entry(void)
{
	struct proc_dir_entry *proc_show_entry = NULL;
	struct proc_dir_entry *proc_show_chip_reset_count_entry = NULL;
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();

	BTMTK_DBG("%s, proc initialized", __func__);

	bmain_info->proc_dir = proc_mkdir(PROC_ROOT_DIR, NULL);
	if (bmain_info->proc_dir == NULL) {
		BTMTK_ERR("Unable to creat %s dir", PROC_ROOT_DIR);
		return;
	}

	proc_show_entry =  proc_create(PROC_BT_FW_VERSION, 0600, bmain_info->proc_dir, &BT_proc_fops);
	if (proc_show_entry == NULL) {
		BTMTK_ERR("Unable to creat %s node", PROC_BT_FW_VERSION);
		remove_proc_entry(PROC_ROOT_DIR, NULL);
	}

#if (USE_DEVICE_NODE == 1)
	proc_show_entry =  proc_create(PROC_BT_UART_LAUNCHER_NOTIFY, 0640, bmain_info->proc_dir, &BT_proc_uart_launcher_notify_fops);
	if (proc_show_entry == NULL) {
		BTMTK_ERR("Unable to creat %s node", PROC_BT_UART_LAUNCHER_NOTIFY);
		remove_proc_entry(PROC_ROOT_DIR, NULL);
	}
#endif

	proc_show_chip_reset_count_entry = proc_create(PROC_BT_CHIP_RESET_COUNT, 0600,
			bmain_info->proc_dir, &BT_proc_chip_reset_count_fops);
	if (proc_show_chip_reset_count_entry == NULL) {
		BTMTK_ERR("Unable to creat %s node", PROC_BT_CHIP_RESET_COUNT);
		remove_proc_entry(PROC_ROOT_DIR, NULL);
	}

}

static void btmtk_proc_delete_entry(void)
{
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();

	if (bmain_info->proc_dir == NULL)
		return;

	remove_proc_entry(PROC_BT_FW_VERSION, bmain_info->proc_dir);
	BTMTK_INFO("%s, proc device node and folder removed!!", __func__);
	remove_proc_entry(PROC_BT_CHIP_RESET_COUNT, bmain_info->proc_dir);
	BTMTK_INFO("%s, proc device node and folder %s removed!!", __func__, PROC_BT_CHIP_RESET_COUNT);
#if (USE_DEVICE_NODE == 1)
	remove_proc_entry(PROC_BT_UART_LAUNCHER_NOTIFY, bmain_info->proc_dir);
	BTMTK_INFO("%s, proc device node and folder %s removed!!", __func__, PROC_BT_UART_LAUNCHER_NOTIFY);
#endif

	remove_proc_entry(PROC_ROOT_DIR, NULL);
	bmain_info->proc_dir = NULL;
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
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();

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
	if (bmain_info->hif_hook.log_init) {
		bmain_info->hif_hook.log_init(fw_log_bt_event_cb);
		//bmain_info->hif_hook.log_register_cb(fw_log_bt_event_cb);
		init_waitqueue_head(&BT_log_wq);
	} else {
		spin_lock_init(&g_fwlog->fwlog_lock);
		skb_queue_head_init(&g_fwlog->fwlog_queue);
		skb_queue_head_init(&g_fwlog->dumplog_queue_first);
		skb_queue_head_init(&g_fwlog->dumplog_queue_latest);
		skb_queue_head_init(&g_fwlog->usr_opcode_queue);//opcode
		init_waitqueue_head(&(g_fwlog->fw_log_inq));
	}

	atomic_set(&bmain_info->fwlog_ref_cnt, 0);
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
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();

	BTMTK_INFO("%s: Start", __func__);

	//if (is_mt66xx(g_sbdev->chip_id))
	if (bmain_info->hif_hook.log_deinit)
		bmain_info->hif_hook.log_deinit();

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
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();

	//if (is_mt66xx(g_sbdev->chip_id)) {
	if (bmain_info->hif_hook.log_read_to_user) {
		copyLen = bmain_info->hif_hook.log_read_to_user(buf, count);
		return copyLen;
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
ssize_t btmtk_fops_writefwlog(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
#if (CFG_ENABLE_DEBUG_WRITE == 0)
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
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();
	struct btmtk_dev **pp_bdev = btmtk_get_pp_bdev();

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
		ret = -ENOMEM;
		goto exit;
	}

	memset(i_fwlog_buf, 0, HCI_MAX_COMMAND_BUF_SIZE);
	memset(o_fwlog_buf, 0, HCI_MAX_COMMAND_SIZE + 16);

	if (buf == NULL || count == 0) {
		BTMTK_ERR("%s: worng input data", __func__);
		ret = -ENODATA;
		goto exit;
	}

	if (copy_from_user(i_fwlog_buf, buf, count) != 0) {
		BTMTK_ERR("%s: Failed to copy data", __func__);
		ret = -ENODATA;
		goto exit;
	}

	/* For log level, EX: echo log_lvl=1 > /dev/stpbtfwlog */
	if (strncmp(i_fwlog_buf, "log_lvl=", strlen("log_lvl=")) == 0) {
		u8 val = *(i_fwlog_buf + strlen("log_lvl=")) - '0';

		if (val > BTMTK_LOG_LVL_MAX || val <= 0) {
			BTMTK_ERR("Got incorrect value for log level(%d)", val);
			ret = -EINVAL;
			goto exit;
		}
		btmtk_log_lvl = val;
		BTMTK_INFO("btmtk_log_lvl = %d", btmtk_log_lvl);
		ret = count;
		goto exit;
	}

#if CFG_SUPPORT_LEAUDIO_CLK
	if (strncmp(i_fwlog_buf, "le_audio_clk=", strlen("le_audio_clk=")) == 0) {
		u8 val = *(i_fwlog_buf + strlen("le_audio_clk=")) - '0';

		pp_bdev[hci_idx]->bt_cfg.le_audio_clk_fw_gpio = val;
		BTMTK_INFO("%s: set le_audio_irq to %d", __func__, pp_bdev[hci_idx]->bt_cfg.le_audio_clk_fw_gpio);
		ret = count;
		goto exit;
	}
#endif

	/* For bperf, EX: echo bperf=1 > /dev/stpbtfwlog */
	if (strncmp(i_fwlog_buf, "bperf=", strlen("bperf=")) == 0) {
		u8 val = *(i_fwlog_buf + strlen("bperf=")) - '0';

		g_fwlog->btmtk_bluetooth_kpi = val;
		BTMTK_INFO("%s: set bluetooth KPI feature(bperf) to %d", __func__, g_fwlog->btmtk_bluetooth_kpi);
		ret = count;
		goto exit;
	}

	if (strncmp(i_fwlog_buf, "set_para=", strlen("set_para=")) == 0) {
		u8 val = *(i_fwlog_buf + strlen("set_para=")) - '0';

		if (bmain_info->hif_hook.set_para)
			bmain_info->hif_hook.set_para(pp_bdev[hci_idx], val);
		else
			BTMTK_WARN("%s: not support set_para", __func__);
		ret = count;
		goto exit;
	}

	/* For update sConnxo_cfg */
	if (strncmp(i_fwlog_buf, "set_xonv=", strlen("set_xonv=")) == 0) {
		u8 *val = i_fwlog_buf + strlen("set_xonv=");
		int nvsz = count - strlen("set_xonv=");

		if (bmain_info->hif_hook.set_xonv)
			bmain_info->hif_hook.set_xonv(pp_bdev[hci_idx], val, nvsz);
		else
			BTMTK_WARN("%s: not support set_xonv", __func__);
		ret = count;
		goto exit;
	}

	if (strncmp(i_fwlog_buf, "chip_reset=", strlen("chip_reset=")) == 0) {
		u8 val = *(i_fwlog_buf + strlen("chip_reset=")) - '0';

		bmain_info->chip_reset_flag = val;
		BTMTK_INFO("%s: set chip reset flag to %d", __func__, bmain_info->chip_reset_flag);
		ret = count;
		goto exit;
	}
	if (strncmp(i_fwlog_buf, "whole chip reset", strlen("whole chip reset")) == 0) {
		BTMTK_INFO("whole chip reset start");
                if (pp_bdev[hci_idx]->assert_reason[0] == '\0') {
                        memset(pp_bdev[hci_idx]->assert_reason, 0, ASSERT_REASON_SIZE);
                        strncpy(pp_bdev[hci_idx]->assert_reason, "FW_LOG_BT trigger whole chip reset", strlen("FW_LOG_BT trigger whole chip reset") + 1);
                }
		bmain_info->chip_reset_flag = 1;
		btmtk_reset_trigger(pp_bdev[hci_idx]);
		ret = count;
		goto exit;
	}

	if (strncmp(i_fwlog_buf, "subsys chip reset", strlen("subsys chip reset")) == 0) {
		BTMTK_INFO("subsys chip reset");
		if (pp_bdev[hci_idx]->assert_reason[0] == '\0') {
			memset(pp_bdev[hci_idx]->assert_reason, 0, ASSERT_REASON_SIZE);
			strncpy(pp_bdev[hci_idx]->assert_reason, "FW_LOG_BT trigger subsys chip reset", strlen("FW_LOG_BT trigger subsys chip reset") + 1);
		}
		if (bmain_info->hif_hook.trigger_assert)
			bmain_info->hif_hook.trigger_assert(pp_bdev[hci_idx]);
		else {
			bmain_info->chip_reset_flag = 0;
			btmtk_reset_trigger(pp_bdev[hci_idx]);
		}
		ret = count;
		goto exit;
	}

	if (strncmp(i_fwlog_buf, "direct trigger whole chip reset", strlen("direct trigger whole chip reset")) == 0) {
		BTMTK_INFO("direct trigger whole chip reset");
		if (bmain_info->hif_hook.whole_reset)
			bmain_info->hif_hook.whole_reset(pp_bdev[hci_idx]);
		else
			BTMTK_INFO("not support direct trigger whole chip reset");
		ret = count;
		goto exit;
	}

	if (strncmp(i_fwlog_buf, "dump chip reset", strlen("dump chip reset")) == 0) {
		BTMTK_INFO("subsys chip reset = %d", atomic_read(&bmain_info->subsys_reset_count));
		BTMTK_INFO("whole chip reset = %d", atomic_read(&bmain_info->whole_reset_count));
		ret = count;
		goto exit;
	}

	if (strncmp(i_fwlog_buf, "dump btsnoop", strlen("dump btsnoop")) == 0) {
		btmtk_hci_snoop_print_to_log();
		ret = count;
		goto exit;
	}

	if (strncmp(i_fwlog_buf, "FindMyPhone=", strlen("FindMyPhone=")) == 0) {
		ret = kstrtou32(i_fwlog_buf + strlen("FindMyPhone="), 0, &bmain_info->find_my_phone_mode);

		if (ret) {
			BTMTK_WARN("%s: convert string failed ret[%d]", __func__, ret);
			goto exit;
		}

		BTMTK_INFO("%s: FindMyPhone[%u]", __func__, bmain_info->find_my_phone_mode);
		ret = count;
		goto exit;
	}

        if (strncmp(i_fwlog_buf, "FindMyPhoneCliff=", strlen("FindMyPhoneCliff=")) == 0) {
                ret = kstrtou32(i_fwlog_buf + strlen("FindMyPhoneCliff="), 0, &bmain_info->find_my_phone_mode_extend);

                if (ret) {
                        BTMTK_WARN("%s: convert string failed ret[%d]", __func__, ret);
                        goto exit;
                }

                BTMTK_INFO("%s: FindMyPhoneCliff[%u]", __func__, bmain_info->find_my_phone_mode_extend);
                ret = count;
                goto exit;
        }

	if (strncmp(i_fwlog_buf, "hif_debug_sop", strlen("hif_debug_sop")) == 0) {
		state = btmtk_get_chip_state(pp_bdev[hci_idx]);
		//&& state == BTMTK_STATE_WORKING
		if (bmain_info->hif_hook.dump_hif_debug_sop) {
			bmain_info->hif_hook.dump_hif_debug_sop(pp_bdev[hci_idx]);
		} else {
			BTMTK_INFO("%s: not support hif_debug_sop or chip_state[%d]", __func__, state);
		}
		ret = count;
		goto exit;
	}

	if (strncmp(i_fwlog_buf, "bk-rs=", strlen("bk-rs=")) == 0) {
		u8 val = *(i_fwlog_buf + strlen("bk-rs=")) - '0';

		bmain_info->bk_rs_flag = val;
		BTMTK_INFO("%s: set bk_rs_flag to %d", __func__, bmain_info->bk_rs_flag);
		ret = count;
		goto exit;
	}

#ifdef BTMTK_DEBUG_SOP
	if (strncmp(i_fwlog_buf, "fwdump", strlen("fwdump")) == 0) {
		BTMTK_INFO("dumplog_queue_first1, skb_queue_len = %d",
				skb_queue_len(&g_fwlog->dumplog_queue_first));
		BTMTK_INFO("dumplog_queue_latest, skb_queue_len = %d",
				skb_queue_len(&g_fwlog->dumplog_queue_latest));

		ret = count;
		goto exit;
	}

	if (strncmp(i_fwlog_buf, "dumpclean", strlen("dumpclean")) == 0) {
		skb_queue_purge(&g_fwlog->dumplog_queue_first);
		skb_queue_purge(&g_fwlog->dumplog_queue_latest);

		ret = count;
		goto exit;
	}

	if (strncmp(i_fwlog_buf, "dump test", strlen("dump test")) == 0) {
		btmtk_load_debug_sop_register(pp_bdev[hci_idx]->debug_sop_file_name,
				pp_bdev[hci_idx]->intf_dev, pp_bdev[hci_idx]);
		ret = count;
		goto exit;
	}

	if (strncmp(i_fwlog_buf, "dump clean", strlen("dump clean")) == 0) {
		btmtk_clean_debug_reg_file(pp_bdev[hci_idx]);
		ret = count;
		goto exit;
	}
#endif

	if (strncmp(i_fwlog_buf, "dump_debug=", strlen("dump_debug")) == 0) {
		u8 val = *(i_fwlog_buf + strlen("dump_debug=")) - '0';
		fstate = btmtk_fops_get_state(pp_bdev[hci_idx]);

		if (bmain_info->hif_hook.dump_debug_sop) {
			BTMTK_INFO("%s: dump_debug(%s)", __func__,
				(val == 0) ? "SLEEP" :
				((val == 1) ? "WAKEUP" :
				((val == 2) ? "NO_RESPONSE" : "ERROR")));
			pp_bdev[hci_idx]->bt_cfg.debug_sop_mode = val;
			if (fstate != BTMTK_FOPS_STATE_OPENED) {
				ret = bmain_info->hif_hook.open(pp_bdev[hci_idx]->hdev);
				if (ret < 0) {
					BTMTK_ERR("%s, cif_open failed", __func__);
					ret = count;
					goto exit;
				}
			}
			bmain_info->hif_hook.dump_debug_sop(pp_bdev[hci_idx]);
			if (fstate != BTMTK_FOPS_STATE_OPENED) {
				ret = bmain_info->hif_hook.close(pp_bdev[hci_idx]->hdev);
				if (ret < 0) {
					BTMTK_ERR("%s, cif_close failed", __func__);
					ret = count;
					goto exit;
				}
			}
		} else {
			BTMTK_INFO("%s: not support", __func__);
		}
		ret = count;
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
			BTMTK_ERR("%s: There is an invalid input(%c)", __func__, *pos);
			ret = -EINVAL;
			goto exit;
		} else if (len + 1 >= HCI_MAX_COMMAND_SIZE) {
			BTMTK_ERR("%s: input data exceed maximum command length (%d)", __func__, len);
			ret = -EINVAL;
			goto exit;
		}

		temp_str[0] = *pos;
		temp_str[1] = *(pos + 1);
		i++;
		ret = kstrtol(temp_str, 16, &res);
		if (ret == 0)
			o_fwlog_buf[len++] = (u8)res;
		else
			BTMTK_ERR("%s: Convert %s failed(%d)", __func__, temp_str, ret);
	}

	if (o_fwlog_buf[0] != HCI_COMMAND_PKT && o_fwlog_buf[0] != FWLOG_TYPE) {
		BTMTK_ERR("%s: Not support 0x%02X yet", __func__, o_fwlog_buf[0]);
		ret = -EPROTONOSUPPORT;
		goto exit;
	}

#if 0   /* for fuzzing */
	/* check HCI command length */
	if (len < HCI_CMD_HEADER_SIZE) {
		BTMTK_ERR("%s: command is too short for hci cmd, length = %d", __func__, len);
		ret = -EINVAL;
		goto exit;
	} else if (len > HCI_MAX_COMMAND_SIZE) {
		BTMTK_ERR("%s: command is larger than max buf size, length = %d", __func__, len);
		ret = -ENOMEM;
		goto exit;
	}
#endif

	skb = alloc_skb(count + BT_SKB_RESERVE, GFP_KERNEL);
	if (!skb) {
		BTMTK_ERR("%s allocate skb failed!!", __func__);
		ret = -ENOMEM;
		goto exit;
	}

	/* send HCI command */
	bt_cb(skb)->pkt_type = HCI_COMMAND_PKT;

	/* format */
	/* 0xF0 XX XX 00 01 AA 10 BB CC CC CC CC ... */
	/* XX XX total length */
	/* 00 : hci index setting type */
	/* AA hci index to indicate which hci send following command*/
	/* 10 : raw data type*/
	/* BB command length */
	/* CC command */
	if (o_fwlog_buf[0] == FWLOG_TYPE) {
		while (index < ((o_fwlog_buf[2] << 8) + o_fwlog_buf[1])) {
			switch (o_fwlog_buf[index]) {
			case FWLOG_HCI_IDX:    /* hci index */
				vlen = o_fwlog_buf[index + 1];
				hci_idx = o_fwlog_buf[index + 2];
				BTMTK_DBG("%s: send to hci%d, vlen %d", __func__, hci_idx, vlen);
				index += (FWLOG_ATTR_TL_SIZE + vlen);
				break;
			case FWLOG_TX:    /* tx raw data */
				vlen = o_fwlog_buf[index + 1];
				memcpy(skb->data, o_fwlog_buf + index + FWLOG_ATTR_TL_SIZE, vlen);
				skb->len = vlen;
				index = index + FWLOG_ATTR_TL_SIZE + vlen;
				break;
			default:
				BTMTK_WARN("%s: Invalid opcode(0x%x)", __func__, o_fwlog_buf[index]);
				ret = -1;
				goto free_skb;
			}
		}
	} else {
		memcpy(skb->data, o_fwlog_buf, len);
		skb->len = len;
#if defined(DRV_RETURN_SPECIFIC_HCE_ONLY) && (DRV_RETURN_SPECIFIC_HCE_ONLY == 1) && (USE_DEVICE_NODE == 0)
		// 0xFC26 is get link & profile information command.
		// 0xFCF5 is get iso perf link info command.
		if (*(uint16_t *)(o_fwlog_buf + 1) != 0xFC26) {
			skb_opcode = alloc_skb(len + FWLOG_PRSV_LEN, GFP_KERNEL);
			if (!skb_opcode) {
				BTMTK_ERR("%s allocate skb failed!!", __func__);
				ret = -ENOMEM;
				goto exit;
			}
			memcpy(skb_opcode->data, (o_fwlog_buf + 1), 2);
			BTMTK_INFO("opcode is %02x,%02x", skb_opcode->data[0], skb_opcode->data[1]);
			skb_queue_tail(&g_fwlog->usr_opcode_queue, skb_opcode);
		}
#endif
	}

	/* won't send command if g_bdev not define */
	if (pp_bdev[hci_idx]->hdev == NULL) {
		BTMTK_DBG("%s: pp_bdev[%d] not define", __func__, hci_idx);
		ret = count;
		goto free_skb;
	}

	state = btmtk_get_chip_state(pp_bdev[hci_idx]);
	if (state != BTMTK_STATE_WORKING) {
		BTMTK_WARN("%s: current is in suspend/resume/standby/dump/disconnect (%d).",
			__func__, state);
		ret = -EBADFD;
		goto free_skb;
	}

	fstate = btmtk_fops_get_state(pp_bdev[hci_idx]);
	if (fstate != BTMTK_FOPS_STATE_OPENED) {
		BTMTK_WARN("%s: fops is not open yet(%d)!", __func__, fstate);
		ret = -ENODEV;
		goto free_skb;
	}

	if (pp_bdev[hci_idx]->power_state == BTMTK_DONGLE_STATE_POWER_OFF) {
		BTMTK_WARN("%s: dongle state already power off, do not write", __func__);
		ret = -EFAULT;
		goto free_skb;
	}

#if (USE_DEVICE_NODE == 0)
	/* clean fwlog queue before enable picus log */
	if (skb_queue_len(&g_fwlog->fwlog_queue) && skb->data[0] == 0x01
			&& skb->data[1] == 0x5d && skb->data[2] == 0xfc && skb->data[4] == 0x00) {
		skb_queue_purge(&g_fwlog->fwlog_queue);
		BTMTK_INFO("clean fwlog_queue, skb_queue_len = %d", skb_queue_len(&g_fwlog->fwlog_queue));
	}

	btmtk_dispatch_fwlog_bluetooth_kpi(pp_bdev[hci_idx], skb->data, skb->len, KPI_WITHOUT_TYPE);
#endif
	/* intercept dbg event */
	if (skb->len > 2) {
		bmain_info->dbg_send = 1;
		if (skb->data[1] == 0x6F && skb->data[2] == 0xFC) {
			if (skb->len < 6) {
				ret = -1;
				BTMTK_ERR("%s invalid wmt cmd format", __func__);
				goto free_skb;
			}
			bmain_info->dbg_send_opcode[0] = skb->data[5];
			BTMTK_INFO("wmt dbg_node_send_opcode is [%02x]", bmain_info->dbg_send_opcode[0]);
		} else {
			memcpy(bmain_info->dbg_send_opcode, (skb->data + 1), 2);
			BTMTK_INFO("dbg_node_send_opcode is [%02x, %02x]", bmain_info->dbg_send_opcode[0],
					bmain_info->dbg_send_opcode[1]);
		}
	}
        ret = bmain_info->hif_hook.send_cmd(pp_bdev[hci_idx], skb, 0, 0, (int)BTMTK_TX_PKT_FROM_HOST, CMD_NEED_FILTER);

	if (ret < 0) {
		BTMTK_ERR("%s failed!!", __func__);
		goto free_skb;
	} else
		BTMTK_INFO("%s: OK", __func__);

	BTMTK_INFO("%s: Write end(len: %d)", __func__, len);
	ret = count;
	goto exit;

free_skb:
	kfree_skb(skb);
	skb = NULL;
#if (USE_DEVICE_NODE == 0)
	/* clean opcode queue if bt is disable */
	skb_queue_purge(&g_fwlog->usr_opcode_queue);
#endif
exit:
	kfree(i_fwlog_buf);
	kfree(o_fwlog_buf);

	return ret;	/* If input is correct should return the same length */
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
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();

	atomic_inc(&bmain_info->fwlog_ref_cnt);
	BTMTK_INFO("%s: Start.", __func__);

	return 0;
}

int btmtk_fops_closefwlog(struct inode *inode, struct file *file)
{
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();

	atomic_dec(&bmain_info->fwlog_ref_cnt);
	BTMTK_INFO("%s: Start.", __func__);

	return 0;
}

long btmtk_fops_unlocked_ioctlfwlog(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long retval = 0;
	uint8_t log_tmp = BT_FWLOG_OFF;
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();
	struct btmtk_dev **pp_bdev = btmtk_get_pp_bdev();
	int hci_idx = 0;

	down(&ioctl_mtx);
	/* mutex with bt open/close/disconnect flow */
	if (bmain_info->hif_hook.cif_mutex_lock)
		bmain_info->hif_hook.cif_mutex_lock(pp_bdev[hci_idx]);

	switch (cmd) {
	case BT_FWLOG_IOC_ON_OFF:
		/* Connsyslogger daemon dynamically enable/disable Picus log */
		BTMTK_INFO("[ON_OFF]arg(%lu) bt_on(0x%x) log_on(0x%x) level(0x%x) log_cur(0x%x)",
			       arg, g_bt_on, g_log_on, g_log_level, g_log_current);

		log_tmp = (arg == 0) ? BT_FWLOG_OFF : BT_FWLOG_ON;
		if (log_tmp != g_log_on) { // changed
			g_log_on = log_tmp;
			g_log_current = g_log_on & g_log_level;
			if (g_bt_on) {
				retval = btmtk_intcmd_set_fw_log(g_log_current);
				/* if bt open or reset when screen off, still need to notify fw blank status */
				btmtk_intcmd_wmt_blank_status(pp_bdev[hci_idx]->blank_state);
				btmtk_intcmd_wmt_utc_sync();
			}
		}
		break;
	case BT_FWLOG_IOC_SET_LEVEL:
		/* Connsyslogger daemon dynamically set Picus log level */
		BTMTK_INFO("[SET_LEVEL]arg(%lu) bt_on(0x%x) log_on(0x%x) level(0x%x) log_cur(0x%x)",
			       arg, g_bt_on, g_log_on, g_log_level, g_log_current);

		log_tmp = (uint8_t)arg;
		if (log_tmp != g_log_level) {
			g_log_level = log_tmp;
			g_log_current = g_log_on & g_log_level;
			if (g_bt_on & g_log_on) {
				// driver on and log on
				retval = btmtk_intcmd_set_fw_log(g_log_current);
				/* if bt open or reset when screen off, still need to notify fw blank status */
				btmtk_intcmd_wmt_blank_status(pp_bdev[hci_idx]->blank_state);
				btmtk_intcmd_wmt_utc_sync();
			}
		}
		break;
	case BT_FWLOG_IOC_GET_LEVEL:
		retval = g_log_level;
		BTMTK_INFO("[GET_LEVEL]return %ld", retval);
		break;
	default:
		BTMTK_ERR("Unknown cmd: 0x%08x", cmd);
		retval = -EOPNOTSUPP;
		break;
	}

	bmain_info->fw_log_on = g_log_current;

	if (bmain_info->hif_hook.cif_mutex_unlock)
		bmain_info->hif_hook.cif_mutex_unlock(pp_bdev[hci_idx]);

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
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();

	//if (is_mt66xx(g_sbdev->chip_id)) {
	if (bmain_info->hif_hook.log_get_buf_size) {
		poll_wait(file, &BT_log_wq, wait);
		if (bmain_info->hif_hook.log_get_buf_size() > 0)
			mask = (POLLIN | POLLRDNORM);
	} else {
		poll_wait(file, &g_fwlog->fw_log_inq, wait);
		if (skb_queue_len(&g_fwlog->fwlog_queue) > 0 ||
				skb_queue_len(&g_fwlog->dumplog_queue_first) > 0 ||
				skb_queue_len(&g_fwlog->dumplog_queue_latest) > 0)
			mask |= POLLIN | POLLRDNORM;			/* readable */
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
			BTMTK_ERR("%s: alloc_skb return 0, error", __func__);
			return -ENOMEM;
		}
		BTMTK_ERR("%s: alloc_skb return 0, error, retry = %d", __func__, retry);
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

int btmtk_update_bt_status(u8 onoff_flag)
{
	struct sk_buff *skb_onoff = NULL;
	int ret = 0;
	ulong flags = 0;

	skb_onoff = alloc_skb(FWLOG_PRSV_LEN, GFP_ATOMIC);
	if (!skb_onoff) {
		BTMTK_ERR("%s allocate skb failed!!", __func__);
		ret = -ENOMEM;
		return ret;
	}

	BTMTK_INFO("%s enter, falg is %d", __func__, onoff_flag);

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

int btmtk_dump_timestamp_add(struct sk_buff_head *queue)
{
	struct sk_buff *skb_tmp = NULL;
	int ret = 0;
	ulong flags = 0;
	struct bt_utc_struct utc;

	skb_tmp = alloc_skb(FWLOG_PRSV_LEN, GFP_ATOMIC);
	if (!skb_tmp) {
		BTMTK_ERR("%s allocate skb failed!!", __func__);
		ret = -ENOMEM;
		return ret;
	}

	skb_tmp->data[0] = 0x6f;
	skb_tmp->data[1] = 0xfc;
	skb_tmp->data[2] = 0xff;

	BTMTK_INFO("%s: Add timestamp", __func__);

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
			BTMTK_WARN("btmtk_usb fwlog queue size is full(bluetooth_kpi)");
		}
	}
	return ret;
}

/* if modify the common part, please sync to another btmtk_dispatch_fwlog */
#if (USE_DEVICE_NODE == 0)
int btmtk_dispatch_fwlog(struct btmtk_dev *bdev, struct sk_buff *skb)
{
	static u8 fwlog_picus_blocking_warn;
	static u8 fwlog_fwdump_blocking_warn;
	int state = 0;
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();
	struct sk_buff *skb_opcode = NULL;
	struct data_struct hci_reset_event = {0};
	static struct sk_buff_head *dumplog_queue = NULL;

	BTMTK_DBG("%s enter", __func__);

	if (!g_fwlog)
		return 0;

	state = btmtk_get_chip_state(bdev);
	if (state == BTMTK_STATE_DISCONNECT) {
		BTMTK_ERR("%s: state is disconnect, return", __func__);
		return 0;
	}

	BTMTK_GET_CMD_OR_EVENT_DATA(bdev, HCI_RESET_EVT, hci_reset_event);

	if ((bt_cb(skb)->pkt_type == HCI_ACLDATA_PKT) &&
			skb->data[0] == 0x6f &&
			skb->data[1] == 0xfc) {
		static int dump_data_counter;
		static int dump_data_length;

		state = btmtk_get_chip_state(bdev);
		if (state != BTMTK_STATE_FW_DUMP) {
			BTMTK_INFO("%s: FW dump begin", __func__);
			DUMP_TIME_STAMP("FW_dump_start");
			btmtk_hci_snoop_print_to_log();
			/* Print too much log, it may cause kernel panic. */
			dump_data_counter = 0;
			dump_data_length = 0;
			btmtk_set_chip_state(bdev, BTMTK_STATE_FW_DUMP);
			if (!btmtk_assert_wake_lock_check())
				btmtk_assert_wake_lock();
			if (!btmtk_reset_timer_check(bdev))
				btmtk_reset_timer_update(bdev);

			BTMTK_INFO("dumplog_queue_first, skb_queue_len = %d",
					skb_queue_len(&g_fwlog->dumplog_queue_first));

			if (skb_queue_len(&g_fwlog->dumplog_queue_first) == 0) {
				BTMTK_INFO("first coredump, save...");
				dumplog_queue = &g_fwlog->dumplog_queue_first;
			} else {
				BTMTK_INFO("clean dumplog_queue_latest, skb_queue_len = %d",
						skb_queue_len(&g_fwlog->dumplog_queue_latest));
				skb_queue_purge(&g_fwlog->dumplog_queue_latest);
				dumplog_queue = &g_fwlog->dumplog_queue_latest;
			}
		}

		if ((bdev->chip_reset_signal & 0x8) == 0) {
			btmtk_dump_timestamp_add(dumplog_queue);
			bdev->chip_reset_signal |= 0x08;
		}

		dump_data_counter++;
		dump_data_length += skb->len - 4;

		/* coredump */
		/* print dump data to console */
		if (dump_data_counter % 1000 == 0) {
			BTMTK_INFO("%s: FW dump on-going, total_packet = %d, total_length = %d",
					__func__, dump_data_counter, dump_data_length);
		}

		/* print dump data to console */
		if (dump_data_counter < 20)
			BTMTK_INFO("%s: FW dump data (%d): %s",
					__func__, dump_data_counter, &skb->data[4]);

		/* In the new generation, we will check the keyword of coredump (; coredump end)
		 * Such as : 79xx
		 */
		if (skb->data[skb->len - 4] == 'e' &&
			skb->data[skb->len - 3] == 'n' &&
			skb->data[skb->len - 2] == 'd') {
			/* This is the latest coredump packet. */
			BTMTK_INFO("%s: FW dump end, dump_data_counter = %d, total_length = %d",
					__func__, dump_data_counter, dump_data_length);
			/* TODO: Chip reset*/
			bmain_info->reset_stack_flag = HW_ERR_CODE_CORE_DUMP;
			DUMP_TIME_STAMP("FW_dump_end");
			if (bmain_info->hif_hook.waker_notify)
				bmain_info->hif_hook.waker_notify(bdev);

			/* Clear 4th bit when received coredump end  */
			bdev->chip_reset_signal &= ~0x08;

#ifdef CHIP_IF_USB
			/* USB hif: recv both 0xff and fw dump end, then do chip reset */
			if (bdev->dualBT) {
				/* If it is dual BT,
				 * the condition for FW dump end is that both BT0 and BT1 are completed.
				 */
				if (bdev->chip_reset_signal & (1 << 1)) {
					bdev->chip_reset_signal |= (1 << 2);
					BTMTK_INFO("%s, BT1 FW dump end, chip_reset_signal = %02x",
							__func__, bdev->chip_reset_signal);
					if (bdev->chip_reset_signal == 0x07) {
						bdev->chip_reset_signal = 0x00;
						DUMP_TIME_STAMP("notify_chip_reset");
						btmtk_reset_trigger(bdev);
					}
				} else {
					bdev->chip_reset_signal |= (1 << 1);
					BTMTK_INFO("%s, BT0 FW dump end, chip_reset_signal = %02x",
							__func__, bdev->chip_reset_signal);
				}
			} else {
				bdev->chip_reset_signal |= (1 << 1);
				BTMTK_INFO("%s ,chip_reset_signal = %02x", __func__, bdev->chip_reset_signal);
				if (bdev->chip_reset_signal == 0x03) {
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
				BTMTK_WARN("btmtk fwlog queue size is full(coredump)");
			}
		}

		/* change coredump's ACL handle to FF F0 */
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
				BTMTK_INFO("btmtk fwlog queue size is full(picus)");
			}
		}
		return 1;
	} else if (memcmp(skb->data, &hci_reset_event.content[1], hci_reset_event.len - 1) == 0) {
		BTMTK_INFO("%s: Get RESET_EVENT", __func__);
		bdev->get_hci_reset = 1;
		atomic_set(&bmain_info->subsys_reset_conti_count, 0);
	}

	/* filter event from usr cmd */
	if ((bt_cb(skb)->pkt_type == HCI_EVENT_PKT) &&
			skb->data[0] == 0x0E) {
		if (skb_queue_len(&g_fwlog->usr_opcode_queue)) {
			BTMTK_INFO("%s: opcode queue len is %d", __func__,
					skb_queue_len(&g_fwlog->usr_opcode_queue));
			skb_opcode = skb_dequeue(&g_fwlog->usr_opcode_queue);
		}

		if (skb_opcode == NULL)
			return 0;

		if (skb_opcode->data[0] == skb->data[3] &&
					skb_opcode->data[1] == skb->data[4]) {
			BTMTK_INFO_RAW(skb->data, skb->len, "%s: recv event from user hci command - ", __func__);
			// should return to upper layer tool
			if (btmtk_skb_enq_fwlog(bdev, skb->data, skb->len, bt_cb(skb)->pkt_type,
						&g_fwlog->fwlog_queue) == 0) {
				wake_up_interruptible(&g_fwlog->fw_log_inq);
			}
			kfree_skb(skb_opcode);
			return 1;
		}

		BTMTK_DBG("%s: check opcode fail!", __func__);
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
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();
	struct connv3_issue_info issue_info;
	int ret = 0, line = 0;
	static unsigned int fwlog_count;
	int drv = CONNV3_DRV_TYPE_BT;

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
			BTMTK_INFO("%s: msg: %s len[%d]", __func__, skb->data, skb->len);
			/* drop "Disable Cache" */
			if (skb->len > 6 &&
				skb->data[skb->len - 6] == 'C' &&
				skb->data[skb->len - 5] == 'a' &&
				skb->data[skb->len - 4] == 'c' &&
				skb->data[skb->len - 3] == 'h' &&
				skb->data[skb->len - 2] == 'e') {
				BTMTK_INFO("%s: drop Cache", __func__);
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
				BTMTK_INFO("%s: drop radio off", __func__);
				return 1;
			}

			/* fw trigger
				reset[0]: subsys reset
				reset[1]: PMIC => call connv3_trigger_pmic_irq
				reset[2]: whole chip reset (DFD)
			*/
			if (skb->len > 9 &&
				skb->data[skb->len - 9] == 'r' &&
				skb->data[skb->len - 8] == 'e' &&
				skb->data[skb->len - 7] == 's' &&
				skb->data[skb->len - 6] == 'e' &&
				skb->data[skb->len - 5] == 't' &&
				skb->data[skb->len - 4] == '[') {
					if (skb->data[skb->len - 3] == '0') {
						BTMTK_INFO("%s: drop subsys type", __func__);
					} else if (skb->data[skb->len - 3] == '1') {
						BTMTK_INFO("%s: FW trigger PMIC fault", __func__);
						bmain_info->chip_reset_flag = 1;
						bdev->reset_type = CONNV3_CHIP_RST_TYPE_PMIC_FAULT_B;
						if (bdev->assert_reason[0] == '\0') {
							memset(bdev->assert_reason, 0, ASSERT_REASON_SIZE);
							strncpy(bdev->assert_reason, "[BT_FW assert] trigger PMIC fault",
								strlen("[BT_FW assert] trigger PMIC fault") + 1);
						}
						BTMTK_ERR("%s: [assert_reason] %s", __func__, bdev->assert_reason);
					} else {
						bmain_info->chip_reset_flag = 1;
						bdev->reset_type = CONNV3_CHIP_RST_TYPE_DFD_DUMP;
						if (bdev->assert_reason[0] == '\0') {
							memset(bdev->assert_reason, 0, ASSERT_REASON_SIZE);
							strncpy(bdev->assert_reason, "[BT_FW assert] trigger whole chip reset",
									strlen("[BT_FW assert] trigger whole chip reset") + 1);
						}
						BTMTK_ERR("%s: [assert_reason] %s", __func__, bdev->assert_reason);
					}
					return 1;
			}

			DUMP_TIME_STAMP("FW_dump_start");
			if (bdev->assert_reason[0] == '\0') {
				memset(bdev->assert_reason, 0, ASSERT_REASON_SIZE);
				if (snprintf(bdev->assert_reason, ASSERT_REASON_SIZE, "[BT_FW assert] %s", skb->data) < 0)
					strncpy(bdev->assert_reason, "[BT_FW assert]", strlen("[BT_FW assert]") + 1);
				BTMTK_ERR("%s: [assert_reason] %s", __func__, bdev->assert_reason);
				btmtk_hci_snoop_print_to_log();
			}

			/* Print too much log, it may cause kernel panic. */
			dump_data_counter = 0;
			dump_data_length = 0;
			if (bmain_info->hif_hook.coredump_handler == NULL) {
				BTMTK_ERR("%s: coredump_handler is NULL", __func__);
				return 1;
			}

			btmtk_sp_coredump_start();

			btmtk_set_chip_state(bdev, BTMTK_STATE_FW_DUMP);
			if (!btmtk_assert_wake_lock_check())
				btmtk_assert_wake_lock();
			if (!btmtk_reset_timer_check(bdev))
				btmtk_reset_timer_update(bdev);
			line = __LINE__;
			bdev->collect_fwdump = TRUE;
			if (strstr(bdev->assert_reason, "[BT_FW assert]"))
				drv = CONNV3_DRV_TYPE_MAX;

			ret = connv3_coredump_start(
					bmain_info->hif_hook.coredump_handler, drv,
					bdev->assert_reason, skb->data, bmain_info->fw_version_str);

			if(ret == CONNV3_COREDUMP_ERR_CHIP_RESET_ONLY) {
				BTMTK_ERR("%s: not collect fw dump, only do reset", __func__);
				bdev->collect_fwdump = FALSE;
				return 1;
			}
			if (ret == CONNV3_COREDUMP_ERR_WRONG_STATUS) {
				BTMTK_ERR("%s: BT previous not end", __func__);
				connv3_coredump_end(bmain_info->hif_hook.coredump_handler, "BT previous not end");
				if (strstr(bdev->assert_reason, "[BT_FW assert]"))
					drv = CONNV3_DRV_TYPE_MAX;

				ret = connv3_coredump_start(
					bmain_info->hif_hook.coredump_handler, drv,
					bdev->assert_reason, skb->data, bmain_info->fw_version_str);
			}
			if (ret)
				goto coredump_fail_unlock;
		}

		dump_data_counter++;
		dump_data_length += skb->len;

		/* coredump content*/
		/* print dump data to console */
		if (dump_data_counter % 1000 == 0) {
			BTMTK_INFO("%s: FW dump on-going, total_packet = %d, total_length = %d",
					__func__, dump_data_counter, dump_data_length);
		}

		/* print dump data to console */
		if (dump_data_counter < 20)
			BTMTK_INFO("%s: FW dump data (%d): %s",
					__func__, dump_data_counter, skb->data);

		line = __LINE__;
		if (bdev->collect_fwdump)
			ret = connv3_coredump_send(bmain_info->hif_hook.coredump_handler, "[M]", skb->data, skb->len);
		if (ret) {
			BTMTK_INFO_RAW(skb->data, skb->len, "%s: send fail, len[%d]", __func__, skb->len);
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
			bmain_info->reset_stack_flag = HW_ERR_CODE_CORE_DUMP;
			DUMP_TIME_STAMP("FW_dump_end");
			line = __LINE__;
                        ret = connv3_coredump_get_issue_info(bmain_info->hif_hook.coredump_handler,
                                                                &issue_info, xml_log, CONNV3_XML_SIZE);
                        if (ret)
                                goto coredump_fail_unlock;
			BTMTK_INFO("%s: xml_log: %s, assert_info: %s, task_name: %s ", __func__, xml_log, issue_info.assert_info, issue_info.task_name);
			line = __LINE__;
			ret = connv3_coredump_send(bmain_info->hif_hook.coredump_handler,
							"INFO", xml_log, strlen(xml_log));
			if (ret)
				goto coredump_fail_unlock;
			/* This is the latest coredump packet. */
			BTMTK_INFO("%s: FW dump end, dump_data_counter[%d], dump_data_length[%d]",
						__func__, dump_data_counter, dump_data_length);

			if (bdev->reset_type != CONNV3_CHIP_RST_TYPE_DFD_DUMP) {
				btmtk_sp_coredump_end();

				/* if do complete and bt close with btmtk_reset_waker start
				* no need to wait hw_err event, cause bt already start close
				*/

				BTMTK_INFO("%s: complete dump_comp , coredump_end", __func__);
				complete_all(&bdev->dump_comp);

				if (bmain_info->hif_hook.waker_notify)
					bmain_info->hif_hook.waker_notify(bdev);
			} else {
				BTMTK_INFO("%s: Not call coredump_end, DFD pre-dump, set bt assert_state end", __func__);
				atomic_set(&bdev->assert_state, BTMTK_ASSERT_END);
				BTMTK_INFO("%s: DFD pre-dump, complete dump_dfd_comp , coredump_end", __func__);
				complete_all(&bdev->dump_dfd_comp);
			}
		}

		return 1;

coredump_fail_unlock:
		BTMTK_ERR("%s: coredump API fail ret[%d] line[%d]", __func__, ret, line);
		return 1;
	} else if ((bt_cb(skb)->pkt_type == HCI_ACLDATA_PKT) &&
				(skb->data[0] == 0xff || skb->data[0] == 0xfe) &&
				skb->data[1] == 0x05 && bmain_info->hif_hook.log_handler) {
		/* picus or syslog */
		/* remove acl header (FF 05 LL LL)*/
		skb_pull(skb, 4);
		fwlog_count++;
		BTMTK_INFO_LIMITTED("fw log counter[%d]", fwlog_count);
		bmain_info->hif_hook.log_handler(skb->data, skb->len);
		return 1;
	} else if ((bt_cb(skb)->pkt_type == HCI_ACLDATA_PKT) &&
				(skb->data[0] == 0xff || skb->data[0] == 0xfe) &&
				skb->data[1] == 0x06 && bmain_info->hif_hook.met_log_handler) {
		skb_pull(skb, 4);
		bmain_info->hif_hook.met_log_handler(bdev, skb->data, skb->len);
		return 1;
	} else if ((bt_cb(skb)->pkt_type == HCI_EVENT_PKT) &&
				skb->len > 7 && skb->data[0] == 0xFF &&
				skb->data[3] == 0x3A && skb->data[4] == 0xFC) {
		BTMTK_INFO_RAW(skb->data, skb->len, "%s: FW Schedule Event:", __func__);
                /* Send Fw Schedule Event to host */
		return 0;
	} else if ((bt_cb(skb)->pkt_type == HCI_EVENT_PKT) &&
				skb->len > 7 && skb->data[0] == 0xFF &&
				skb->data[1] == 0x02 && skb->data[2] == 0x20) {
		BTMTK_INFO_RAW(skb->data, skb->len, "%s: Tx power abnormal Event:", __func__);
		switch (skb->data[3]) {
		case 0x00:
			BTMTK_INFO("%s: RF0 TSSI offset too large", __func__);
			conn_dbg_add_log(CONN_DBG_LOG_TYPE_HW_ERR, "[bt] RF0 TSSI offset too large");
			break;
                case 0x01:
                        BTMTK_INFO("%s: RF1 TSSI offset too large", __func__);
			conn_dbg_add_log(CONN_DBG_LOG_TYPE_HW_ERR, "[bt] RF1 TSSI offset too large");
                        break;
                case 0x02:
                        BTMTK_INFO("%s: RF0 TX chipout power abnormal", __func__);
			conn_dbg_add_log(CONN_DBG_LOG_TYPE_HW_ERR, "[bt] RF0 TX chipout power abnormal");
                        break;
                case 0x03:
                        BTMTK_INFO("%s: RF1 TX chipout power abnormal", __func__);
			conn_dbg_add_log(CONN_DBG_LOG_TYPE_HW_ERR, "[bt] RF1 TX chipout power abnormal");
                        break;
                case 0x04:
                        BTMTK_INFO("%s: RF0 TX voltage abnormal", __func__);
			conn_dbg_add_log(CONN_DBG_LOG_TYPE_HW_ERR, "[bt] RF0 TX voltage abnormal");
                        break;
                case 0x05:
                        BTMTK_INFO("%s: RF1 TX voltage abnormal", __func__);
			conn_dbg_add_log(CONN_DBG_LOG_TYPE_HW_ERR, "[bt] RF1 TX voltage abnormal");
                        break;
		}
		aee_kernel_warning("bt thermal", "BT power abnormal\n");
		return 1;
        } else if ((bt_cb(skb)->pkt_type == HCI_EVENT_PKT) &&
		skb->data[0] == 0xFF && skb->data[2] == 0x21) {
		BTMTK_INFO_RAW(skb->data, skb->len, "%s: Tx power abnormal Event 2:", __func__);
		switch (skb->data[3] & 0x0F) {
		case 0x00:
			BTMTK_INFO_RAW(skb->data, skb->len, "%s: [0x00] power abnormal Event", __func__);
			conn_dbg_add_log(CONN_DBG_LOG_TYPE_HW_ERR, "[bt] RF0 TSSI offset too large");
			break;
		case 0x01:
			BTMTK_INFO_RAW(skb->data, skb->len, "%s: [0x01] power abnormal Event", __func__);
			conn_dbg_add_log(CONN_DBG_LOG_TYPE_HW_ERR, "[bt] RF1 TSSI offset too large");
			break;
		case 0x02:
			BTMTK_INFO_RAW(skb->data, skb->len, "%s: [0x02] power abnormal Event", __func__);
			conn_dbg_add_log(CONN_DBG_LOG_TYPE_HW_ERR, "[bt] RF0 TX chipout power abnormal");
			break;
		}
		if (skb->data[3] & 0x80)
			aee_kernel_warning("combo_bt", "BT power abnormal!\n");
		return 1;
	} else if (bt_cb(skb)->pkt_type == HCI_EVENT_PKT && bmain_info->dbg_send
				&& skb->len > 4 &&
				skb->data[3] == bmain_info->dbg_send_opcode[0] &&
				skb->data[4] == bmain_info->dbg_send_opcode[1]) {
		bmain_info->dbg_send = 0;
		BTMTK_INFO_RAW(skb->data, skb->len, "%s: dbg_node_event len[%d] %02x", __func__,
					skb->len + 1, hci_skb_pkt_type(skb));
		return 1;
	}

	return 0;
}
#endif // #else (USE_DEVICE_NODE == 1)

