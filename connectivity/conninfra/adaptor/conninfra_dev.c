// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/delay.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
#include <linux/mm.h>
#if IS_ENABLED(CONFIG_DEVICE_MODULES_DRM_MEDIATEK)
#include "mtk_disp_notify.h"
#endif
#else
#include <linux/fb.h>
#endif

#include <linux/workqueue.h>
#include <linux/suspend.h>
#include "osal.h"
#include "conninfra.h"
#include "conninfra_conf.h"
#include "connv2_drv.h"
#include "connv3_drv.h"
#include "conn_adaptor.h"
#include "conn_kern_adaptor.h"

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

#define CONNINFRA_DEV_MAJOR 164
#define CONNINFRA_DEV_NUM 1
#define CONNINFRA_DRVIER_NAME "conninfra_drv"
#define CONNINFRA_DEVICE_NAME "conninfra_dev"

#define CONNINFRA_DEV_IOC_MAGIC 0xc2
#define CONNINFRA_IOCTL_GET_CHIP_ID _IOR(CONNINFRA_DEV_IOC_MAGIC, 0, int)
#define CONNINFRA_IOCTL_SET_COREDUMP_MODE _IOW(CONNINFRA_DEV_IOC_MAGIC, 1, unsigned int)
#define CONNINFRA_IOCTL_DO_MODULE_INIT _IOR(CONNINFRA_DEV_IOC_MAGIC, 2, int)
#define CONNINFRA_IOCTL_GET_ADIE_CHIP_ID _IOR(CONNINFRA_DEV_IOC_MAGIC, 3, int)

#define CONNINFRA_DEV_INIT_TO_MS (2 * 1000)

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

enum conn_adaptor_init_status {
	CONN_ADAPTOR_INIT_NOT_START,
	CONN_ADAPTOR_INIT_START,
	CONN_ADAPTOR_INIT_DONE,
};

static int g_conn_adaptor_init_status = CONN_ADAPTOR_INIT_NOT_START;
static wait_queue_head_t g_conn_adaptor_init_wq;

static struct notifier_block conn_adaptor_pm_notifier;


struct conn_adaptor_drv_func_support {
	u32 (*get_adie_id)(u32 drv_type);
};

struct conn_adaptor_drv_gen_inst {
	atomic_t enable;
	struct conn_adaptor_drv_gen_cb drv_gen_cb;
};

static struct conn_adaptor_drv_gen_inst g_drv_gen_inst[CONN_ADAPTOR_DRV_SIZE];
static struct conn_adaptor_drv_func_support g_drv_func_support[CONN_ADAPTOR_DRV_SIZE];

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

static int conn_adaptor_dev_fb_notifier_callback(struct notifier_block *self,
				unsigned long event, void *data);

/* device file operation callback */
static int conn_adaptor_dev_open(struct inode *inode, struct file *file);
static int conn_adaptor_dev_close(struct inode *inode, struct file *file);
static ssize_t conn_adaptor_dev_read(struct file *filp, char __user *buf,
				size_t count, loff_t *f_pos);
static ssize_t conn_adaptor_dev_write(struct file *filp,
				const char __user *buf, size_t count,
				loff_t *f_pos);
static long conn_adaptor_dev_unlocked_ioctl(
		struct file *filp, unsigned int cmd, unsigned long arg);
#ifdef CONFIG_COMPAT
static long conn_adaptor_dev_compat_ioctl(
		struct file *filp, unsigned int cmd, unsigned long arg);
#endif

static int conn_adaptor_dev_do_drv_init(void);

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

struct class *p_conn_adaptor_class;
struct device *p_conn_adaptor_dev;
static struct cdev g_conn_adaptor_cdev;

static const struct file_operations g_conn_adaptor_dev_fops = {
	.open = conn_adaptor_dev_open,
	.release = conn_adaptor_dev_close,
	.read = conn_adaptor_dev_read,
	.write = conn_adaptor_dev_write,
	.unlocked_ioctl = conn_adaptor_dev_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = conn_adaptor_dev_compat_ioctl,
#endif
};

static int g_conn_adaptor_major = CONNINFRA_DEV_MAJOR;

/* screen on/off notification */
static struct notifier_block conn_adaptor_fb_notifier;
static struct work_struct g_pwr_on_off_work;
static atomic_t g_es_lr_flag_for_blank = ATOMIC_INIT(0); /* for ctrl blank flag */

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

bool __weak conn_adaptor_is_internal(void)
{
	return false;
}

static u32 conn_adaptor_get_chipid(void)
{
	int i;

	for (i = 0; i < CONN_ADAPTOR_DRV_SIZE; i++) {
		if (atomic_read(&g_drv_gen_inst[i].enable) &&
			g_drv_gen_inst[i].drv_gen_cb.get_chip_id)
			return (*(g_drv_gen_inst[i].drv_gen_cb.get_chip_id))();
	}
	return 0;
}

static void conn_adaptor_set_coredump_mode(int mode)
{
	int i;

	for (i = 0; i < CONN_ADAPTOR_DRV_SIZE; i++) {
		if (atomic_read(&g_drv_gen_inst[i].enable) &&
		    (g_drv_gen_inst[i].drv_gen_cb.set_coredump_mode != NULL))
			(*(g_drv_gen_inst[i].drv_gen_cb.set_coredump_mode))(mode);
	}
}

static u32 conn_adaptor_detect_adie_chipid(u32 drv_type)
{

	if (drv_type >= CONN_ADAPTOR_DRV_SIZE) {
		pr_warn("[%s] incorrect mode [%d]", __func__, drv_type);
		return 0;
	}

	if (g_drv_func_support[drv_type].get_adie_id == NULL) {
		pr_warn("[%s] get_adie not found", __func__);
		return 0;
	}

	return (*(g_drv_func_support[drv_type].get_adie_id))(drv_type);
}


int conn_adaptor_dev_open(struct inode *inode, struct file *file)
{
	static DEFINE_RATELIMIT_STATE(_rs, HZ, 1);

	if (!wait_event_timeout(
		g_conn_adaptor_init_wq,
		g_conn_adaptor_init_status == CONN_ADAPTOR_INIT_DONE,
		msecs_to_jiffies(CONNINFRA_DEV_INIT_TO_MS))) {
		if (__ratelimit(&_rs)) {
			pr_warn("wait_event_timeout (%d)ms,(%lu)jiffies,return -EIO\n",
				CONNINFRA_DEV_INIT_TO_MS, msecs_to_jiffies(CONNINFRA_DEV_INIT_TO_MS));
		}
		return -EIO;
	}
	pr_info("open major %d minor %d (pid %d)\n",
			imajor(inode), iminor(inode), current->pid);

	return 0;
}

int conn_adaptor_dev_close(struct inode *inode, struct file *file)
{
	pr_info("close major %d minor %d (pid %d)\n",
			imajor(inode), iminor(inode), current->pid);

	return 0;
}

ssize_t conn_adaptor_dev_read(struct file *filp, char __user *buf,
					size_t count, loff_t *f_pos)
{

	if (atomic_read(&g_drv_gen_inst[CONN_ADAPTOR_DRV_GEN_CONNAC_2].enable) &&
		g_drv_gen_inst[CONN_ADAPTOR_DRV_GEN_CONNAC_2].drv_gen_cb.coredump_emi_read)
		return (*(g_drv_gen_inst[CONN_ADAPTOR_DRV_GEN_CONNAC_2].drv_gen_cb.coredump_emi_read))(filp, buf, count, f_pos);


	return 0;
}

ssize_t conn_adaptor_dev_write(struct file *filp,
			const char __user *buf, size_t count, loff_t *f_pos)
{
	return 0;
}

static const char *g_adp_drv_name[CONN_ADAPTOR_DRV_SIZE] = {
	"WIFI", "BT", "GPS", "FM"
};

static long conn_adaptor_dev_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int retval = 0;

	/* Special process for module init command */
	if (cmd == CONNINFRA_IOCTL_DO_MODULE_INIT) {
		pr_info("[%s] KO mode", __func__);
		return 0;
	}

	switch (cmd) {
	case CONNINFRA_IOCTL_GET_CHIP_ID:
		retval = conn_adaptor_get_chipid();
		pr_info("[%s] get chip id: 0x%x\n", __func__, retval);
		break;
	case CONNINFRA_IOCTL_SET_COREDUMP_MODE:
		pr_info("[%s] set coredump as: %d\n", __func__, arg);
		conn_adaptor_set_coredump_mode(arg);
		break;
	case CONNINFRA_IOCTL_GET_ADIE_CHIP_ID:
		if (arg >= CONN_ADAPTOR_DRV_SIZE)
			pr_notice("[%s][CONNINFRA_IOCTL_GET_ADIE_CHIP_ID] invalid input: %lu",
				__func__, arg);
		else {
			retval = conn_adaptor_detect_adie_chipid(arg);
			pr_info("[%s] get adie [%s]=[0x%04x]", __func__, g_adp_drv_name[arg], retval);
		}
		break;
	}

	return retval;
}

#ifdef CONFIG_COMPAT
static long conn_adaptor_dev_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long ret;

	ret = conn_adaptor_dev_unlocked_ioctl(filp, cmd, arg);
	return ret;
}
#endif

static int conn_adaptor_dev_get_disp_blank_state(void)
{
	return atomic_read(&g_es_lr_flag_for_blank);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
int conn_adaptor_dev_fb_notifier_callback(struct notifier_block *self, unsigned long event, void *v)
{
#if IS_ENABLED(CONFIG_DEVICE_MODULES_DRM_MEDIATEK)
	int *data = (int *)v;

	pr_debug("[%s] event=[%u]\n", __func__, event);

	if (event != MTK_DISP_EVENT_BLANK) {
		return 0;
	}

	pr_info("%s+\n", __func__);

	if (*data == MTK_DISP_BLANK_UNBLANK) {
		atomic_set(&g_es_lr_flag_for_blank, 1);
		pr_info("@@@@@@@@@@ Conninfra enter UNBLANK @@@@@@@@@@@@@@\n");
		schedule_work(&g_pwr_on_off_work);
	} else if (*data == MTK_DISP_BLANK_POWERDOWN) {
		atomic_set(&g_es_lr_flag_for_blank, 0);
		pr_info("@@@@@@@@@@ Conninfra enter early POWERDOWN @@@@@@@@@@@@@@\n");
		schedule_work(&g_pwr_on_off_work);
	}

	pr_info("%s-\n", __func__);
#endif
	return 0;
}
#else
int conn_adaptor_dev_fb_notifier_callback(struct notifier_block *self,
				unsigned long event, void *data)
{
#if defined(CONFIG_FB)
	struct fb_event *evdata = data;
	int blank;

	pr_debug("[%s] event=[%u]\n", __func__, event);

	/* If we aren't interested in this event, skip it immediately ... */
	if (event != FB_EVENT_BLANK)
		return 0;

	blank = *(int *)evdata->data;

	switch (blank) {
	case FB_BLANK_UNBLANK:
		atomic_set(&g_es_lr_flag_for_blank, 1);
		pr_info("@@@@@@@@@@ Conninfra enter UNBLANK @@@@@@@@@@@@@@\n");
		schedule_work(&g_pwr_on_off_work);
		break;
	case FB_BLANK_POWERDOWN:
		atomic_set(&g_es_lr_flag_for_blank, 0);
		pr_info("@@@@@@@@@@ Conninfra enter early POWERDOWN @@@@@@@@@@@@@@\n");
		schedule_work(&g_pwr_on_off_work);
		break;
	default:
		break;
	}
#endif
	return 0;
}
#endif

static void conn_adaptor_dev_pwr_on_off_handler(struct work_struct *work)
{
	int i;

	/* Update blank on status after wmt power on */
	for (i = 0; i < CONN_ADAPTOR_DRV_SIZE; i++) {
		if (atomic_read(&g_drv_gen_inst[i].enable) &&
			g_drv_gen_inst[i].drv_gen_cb.power_on_off_notify)
			(*(g_drv_gen_inst[i].drv_gen_cb.power_on_off_notify))(conn_adaptor_dev_get_disp_blank_state());
	}
}

static int mtk_conn_adaptor_suspend(void)
{
	int ret = 0;
	int i;

	/* suspend callback is in atomic context */
	for (i = 0; i < CONN_ADAPTOR_DRV_SIZE; i++) {
		if (atomic_read(&g_drv_gen_inst[i].enable) &&
			g_drv_gen_inst[i].drv_gen_cb.plat_suspend_notify)
			(*(g_drv_gen_inst[i].drv_gen_cb.plat_suspend_notify))();
	}

	return ret;
}

static int mtk_conn_adaptor_resume(void)
{
	int i;
	/* suspend callback is in atomic context */
	for (i = 0; i < CONN_ADAPTOR_DRV_SIZE; i++) {
		if (atomic_read(&g_drv_gen_inst[i].enable) &&
			g_drv_gen_inst[i].drv_gen_cb.plat_resume_notify)
			(*(g_drv_gen_inst[i].drv_gen_cb.plat_resume_notify))();
	}

	return 0;
}

static int conn_adaptor_pm_notifier_callback(struct notifier_block *nb,
		unsigned long event, void *dummy)
{
	switch (event) {
		case PM_SUSPEND_PREPARE:
			mtk_conn_adaptor_suspend();
			break;
		case PM_POST_SUSPEND:
			mtk_conn_adaptor_resume();
			break;
		default:
			break;
	}
	return NOTIFY_DONE;
}

int conn_adaptor_kern_dbg_handler(int x, int y, int z, char* buf, int buf_sz)
{
	int offset = 0, sz, i;


	switch (x) {
	case 0x40:
		for (i = 0; i < CONN_ADAPTOR_DRV_SIZE; i++) {
			if (atomic_read(&g_drv_gen_inst[i].enable) &&
				g_drv_gen_inst[i].drv_gen_cb.dump_power_state) {
				sz = (*(g_drv_gen_inst[i].drv_gen_cb.dump_power_state))(buf + offset, buf_sz - offset);
				if (sz > 0)
					offset += sz;
				if (offset >= buf_sz) {
					pr_notice("[%s] buf full", __func__);
					break;
				}
			}
		}
		break;
#if CONNINFRA_DBG_SUPPORT
	case 0x50:
		for (i = 0; i < CONN_ADAPTOR_DRV_SIZE; i++) {
			if (atomic_read(&g_drv_gen_inst[i].enable) &&
				g_drv_gen_inst[i].drv_gen_cb.factory_testcase) {
				sz = (*(g_drv_gen_inst[i].drv_gen_cb.factory_testcase))(buf + offset, buf_sz - offset);
				if (sz > 0)
					offset += sz;
				if (offset >= buf_sz) {
					pr_notice("[%s] buf full", __func__);
					break;
				}
			}
		}
		break;
	case 0x60:
		for (i = 0; i < CONN_ADAPTOR_DRV_SIZE; i++) {
			if (atomic_read(&g_drv_gen_inst[i].enable) &&
				g_drv_gen_inst[i].drv_gen_cb.get_chip_info) {
				sz = (*(g_drv_gen_inst[i].drv_gen_cb.get_chip_info))(buf + offset, buf_sz - offset);
				if (sz > 0)
					offset += sz;
				if (offset >= buf_sz) {
					pr_notice("[%s] buf full", __func__);
					break;
				}
			}
		}
		break;
#endif
	case 0x13:
		pr_info("[%s] set coredump as: %d\n", __func__, y);
		conn_adaptor_set_coredump_mode(y);
		break;
	}
	return offset;
}

int conn_adaptor_register_drv_gen(enum conn_adaptor_drv_gen drv_gen, struct conn_adaptor_drv_gen_cb* cb)
{
	int i;

	if (drv_gen < 0 || drv_gen >= CONN_ADAPTOR_DRV_GEN_SIZE) {
		pr_warn("[%s] invalid drv gen %d", __func__, drv_gen);
		return -1;
	}
	if (cb == NULL) {
		pr_warn("[%s] drv_gen=[%d] cb is null", __func__, drv_gen);
		return -1;
	}

	/* TODO: mutex */
	memcpy(&g_drv_gen_inst[drv_gen].drv_gen_cb, cb, sizeof(*cb));

	atomic_set(&g_drv_gen_inst[drv_gen].enable, 1);

	for (i = 0; i < CONN_ADAPTOR_DRV_SIZE; i++) {
		if (cb->drv_radio_support & (0x1<<i))
			g_drv_func_support[i].get_adie_id = cb->get_adie_id;
	}

	return 0;
}


int conn_adaptor_unregister_drv_gen(enum conn_adaptor_drv_gen drv_gen)
{
	if (drv_gen < 0 || drv_gen >= CONN_ADAPTOR_DRV_GEN_SIZE) {
		pr_warn("[%s] invalid drv gen %d", __func__, drv_gen);
		return -1;
	}

	atomic_set(&g_drv_gen_inst[drv_gen].enable, 0);
	memset(&g_drv_gen_inst[drv_gen].drv_gen_cb, 0, sizeof(struct conn_adaptor_drv_gen_cb));

	return 0;
}



/************************************************************************/
static int conn_adaptor_dev_do_drv_init(void)
{
	static int init_done = 0;
	int iret = 0;

	if (init_done) {
		pr_info("%s already init, return.", __func__);
		return 0;
	}
	init_done = 1;


	memset(&g_drv_func_support, 0,
			CONN_ADAPTOR_DRV_SIZE * sizeof(struct conn_adaptor_drv_func_support));

	iret = conninfra_conf_init();
	if (iret)
		pr_warn("init conf fail\n");

	/* connv2 (conninfra init) */
	iret = connv2_drv_init();
	if (iret)
		pr_err("[%s] connv2 init fail %d", __func__, iret);

	pr_info("[%s] ========================== start connv3", __func__);
	/* connv3 */
	iret = connv3_drv_init();
	if (iret)
		pr_err("[%s] connv3 init fail %d", __func__, iret);

	/* init power on off handler */
	INIT_WORK(&g_pwr_on_off_work, conn_adaptor_dev_pwr_on_off_handler);
	conn_adaptor_fb_notifier.notifier_call
				= conn_adaptor_dev_fb_notifier_callback;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
#if IS_ENABLED(CONFIG_DEVICE_MODULES_DRM_MEDIATEK)
#ifndef CONFIG_FPGA_EARLY_PORTING
	iret = mtk_disp_notifier_register("conninfra_driver", &conn_adaptor_fb_notifier);
#endif
#endif
#else
	iret = fb_register_client(&conn_adaptor_fb_notifier);
#endif
	if (iret)
		pr_info("register fb_notifier fail");
	else
		pr_info("register fb_notifier success");

	/* suspend/resume callback */
	conn_adaptor_pm_notifier.notifier_call = conn_adaptor_pm_notifier_callback;
	iret = register_pm_notifier(&conn_adaptor_pm_notifier);
	if (iret < 0)
		pr_notice("%s register_pm_notifier fail %d\n", iret);

	pr_info("ConnInfra Dev: init (%d)\n", iret);
	g_conn_adaptor_init_status = CONN_ADAPTOR_INIT_DONE;

	return 0;
}


static int conninfra_dev_init(void)
{
	dev_t devID = MKDEV(g_conn_adaptor_major, 0);
	int cdevErr = -1;
	int iret = 0;

	g_conn_adaptor_init_status = CONN_ADAPTOR_INIT_START;
	init_waitqueue_head((wait_queue_head_t *)&g_conn_adaptor_init_wq);

	iret = register_chrdev_region(devID, CONNINFRA_DEV_NUM,
						CONNINFRA_DRVIER_NAME);
	if (iret) {
		pr_notice("[%s] fail to register chrdev, iret = %d\n", __func__, iret);
		g_conn_adaptor_init_status = CONN_ADAPTOR_INIT_NOT_START;
		return -1;
	}

	cdev_init(&g_conn_adaptor_cdev, &g_conn_adaptor_dev_fops);
	g_conn_adaptor_cdev.owner = THIS_MODULE;

	cdevErr = cdev_add(&g_conn_adaptor_cdev, devID, CONNINFRA_DEV_NUM);
	if (cdevErr) {
		pr_err("cdev_add() fails (%d)\n", cdevErr);
		goto err1;
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6,4,0))
        p_conn_adaptor_class = class_create(CONNINFRA_DEVICE_NAME);
#else
	p_conn_adaptor_class = class_create(THIS_MODULE, CONNINFRA_DEVICE_NAME);
#endif
	if (IS_ERR(p_conn_adaptor_class)) {
		pr_err("class create fail, error code(%ld)\n",
						PTR_ERR(p_conn_adaptor_class));
		goto err1;
	}

	p_conn_adaptor_dev = device_create(p_conn_adaptor_class, NULL, devID,
						NULL, CONNINFRA_DEVICE_NAME);
	if (IS_ERR(p_conn_adaptor_dev)) {
		pr_err("device create fail, error code(%ld)\n",
						PTR_ERR(p_conn_adaptor_dev));
		goto err2;
	}

	iret = conn_adaptor_dev_do_drv_init();
	if (iret) {
		pr_err("conninfra_do_drv_init fail, iret = %d", iret);
		return iret;
	}

	iret = conn_kern_adaptor_init(conn_adaptor_kern_dbg_handler);
	if (iret)
		pr_err("kern adaptor init fail [%d]", iret);

	return 0;
err2:

	pr_err("[conninfra_dev_init] err2");
	if (p_conn_adaptor_class) {
		class_destroy(p_conn_adaptor_class);
		p_conn_adaptor_class = NULL;
	}
err1:
	pr_err("[conninfra_dev_init] err1");
	if (cdevErr == 0)
		cdev_del(&g_conn_adaptor_cdev);

	if (iret == 0) {
		unregister_chrdev_region(devID, CONNINFRA_DEV_NUM);
		g_conn_adaptor_major = -1;
	}

	g_conn_adaptor_init_status = CONN_ADAPTOR_INIT_NOT_START;
	return -2;
}

static void conninfra_dev_deinit(void)
{
	dev_t dev = MKDEV(g_conn_adaptor_major, 0);
	int iret = 0;

	g_conn_adaptor_init_status = CONN_ADAPTOR_INIT_NOT_START;

	/* deinit connv2 */
	connv2_drv_deinit();

	/* deinit connv3 */
	connv3_drv_deinit();

	conn_kern_adaptor_deinit();

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
#if IS_ENABLED(CONFIG_DEVICE_MODULES_DRM_MEDIATEK)
#ifndef CONFIG_FPGA_EARLY_PORTING
	mtk_disp_notifier_unregister(&conn_adaptor_fb_notifier);
#endif
#endif
#else
	fb_unregister_client(&conn_adaptor_fb_notifier);
#endif

	unregister_pm_notifier(&conn_adaptor_pm_notifier);

	iret = conninfra_conf_deinit();

	if (p_conn_adaptor_dev) {
		device_destroy(p_conn_adaptor_class, dev);
		p_conn_adaptor_dev = NULL;
	}

	if (p_conn_adaptor_class) {
		class_destroy(p_conn_adaptor_class);
		p_conn_adaptor_class = NULL;
	}

	cdev_del(&g_conn_adaptor_cdev);
	unregister_chrdev_region(dev, CONNINFRA_DEV_NUM);

	pr_info("ConnInfra: ALPS platform init (%d)\n", iret);
}

module_init(conninfra_dev_init);
module_exit(conninfra_dev_deinit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Willy.Yu @ CTD/SE5/CS5");

module_param(g_conn_adaptor_major, uint, 0644);

