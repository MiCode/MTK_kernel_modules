/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_reserved_mem.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/thermal.h>

#include "osal.h"
#include "conn_adaptor.h"
#include "connv3_hw.h"
#include "connv3_core.h"
#include "connv3_drv.h"
#include "connv3_debug_utility.h"

#ifdef CFG_CONNINFRA_UT_SUPPORT
#include "connv3_test.h"
#endif

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/


/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

extern const struct of_device_id connv3_of_ids[];

static int mtk_connv3_probe(struct platform_device *pdev);
static int mtk_connv3_remove(struct platform_device *pdev);

static struct platform_driver g_mtk_connv3_dev_drv = {
	.probe = mtk_connv3_probe,
	.remove = mtk_connv3_remove,
	.driver = {
		   .name = "mtk_connv3",
		   .owner = THIS_MODULE,
#ifdef CONFIG_OF
		   .of_match_table = connv3_of_ids,
#endif
		   .probe_type = PROBE_FORCE_SYNCHRONOUS,
		   },
};

struct platform_device *g_connv3_drv_dev;

/* PMIC */
/*
 * event:
 * 1: chip reset
 */
struct connv3_pmic_work {
	unsigned int id;
	unsigned int event;
	struct work_struct pmic_work;
};

/* For PMIC callback */
static struct connv3_pmic_work g_connv3_pmic_work;

static int connv3_dev_pmic_event_cb(unsigned int, unsigned int);
static struct connv3_dev_cb g_connv3_dev_cb = {
	.connv3_pmic_event_notifier = connv3_dev_pmic_event_cb,
};

/* suspend/resume */
struct work_struct g_connv3_ap_resume_work;
void connv3_suspend_notify(void);
void connv3_resume_notify(void);

/* adaptor */
u32 connv3_get_chipid(void);
void connv3_set_coredump_mode(int mode);
u32 connv3_detect_adie_chipid(u32 drv_type);

/* screen on/off */
void connv3_power_on_off_notify(int on_off);

/* dump power state */
int connv3_dump_power_state(uint8_t *buf, u32 buf_sz);

struct conn_adaptor_drv_gen_cb g_connv3_drv_gen = {
	.drv_radio_support = 0x0,

	.get_chip_id = connv3_get_chipid,
	.get_adie_id = connv3_detect_adie_chipid,

	/* suspend/resume */
	.plat_suspend_notify = connv3_suspend_notify,
	.plat_resume_notify = connv3_resume_notify,

	/* on/off */
	.power_on_off_notify = connv3_power_on_off_notify,

	/* coredump */
	.set_coredump_mode = connv3_set_coredump_mode,

	.dump_power_state = connv3_dump_power_state,
};

static atomic_t g_connv3_hw_init_done = ATOMIC_INIT(0);

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/
u32 connv3_get_chipid(void)
{
	return connv3_hw_get_chipid();
}

void connv3_set_coredump_mode(int mode)
{
	connv3_coredump_set_dump_mode(mode);
}

u32 connv3_detect_adie_chipid(u32 drv_type)
{
	return connv3_hw_get_adie_chipid();
}

static int connv3_dev_pmic_event_cb(unsigned int id, unsigned int event)
{
	g_connv3_pmic_work.id = id;
	g_connv3_pmic_work.event = event;
	schedule_work(&g_connv3_pmic_work.pmic_work);

	return 0;
}

static void connv3_dev_pmic_event_handler(struct work_struct *work)
{
	unsigned int id, event;
	struct connv3_pmic_work *pmic_work =
		container_of(work, struct connv3_pmic_work, pmic_work);

	if (pmic_work) {
		id = pmic_work->id;
		event = pmic_work->event;
		connv3_core_pmic_event_cb(id, event);
	} else
		pr_info("[%s] pmic_work is null", __func__);

}

void connv3_suspend_notify(void)
{
	/* Do nothing now */
}

void connv3_resume_notify(void)
{
	schedule_work(&g_connv3_ap_resume_work);
}

static void connv3_hw_ap_resume_handler(struct work_struct *work)
{
	connv3_core_reset_and_dump_power_state(NULL, 0, 0);
}

void connv3_power_on_off_notify(int on_off)
{
	pr_info("[%s] on_off=[%d]", __func__, on_off);
	if (on_off == 1)
		connv3_core_screen_on();
	else
		connv3_core_screen_off();
}


int connv3_dump_power_state(uint8_t *buf, u32 buf_sz)
{
#define CONN_DUMP_STATE_BUF_SIZE 1024
	int ret = 0, len;
	char *tmp_buf;

	tmp_buf = osal_malloc(CONN_DUMP_STATE_BUF_SIZE);
	if (!tmp_buf) {
		pr_notice("%s failed to allocate memory\n", __func__);
		return -1;
	}

	memset(tmp_buf, '\0', CONN_DUMP_STATE_BUF_SIZE);
	ret = connv3_core_reset_and_dump_power_state(tmp_buf, CONN_DUMP_STATE_BUF_SIZE, 1);
	if (ret) {
		osal_free(tmp_buf);
		return ret;
	}

	len = strlen(tmp_buf);
	if (len > 0 && len < CONN_DUMP_STATE_BUF_SIZE) {
		if (snprintf(buf, buf_sz, "%s", tmp_buf) < 0)
			pr_notice("[%s] snprintf fail", __func__);
	} else
		len = -1;

	osal_free(tmp_buf);
	return len;
}

/* BT, WIFI, GPS, FM */
const int radio_support_map[CONNV3_DRV_TYPE_MAX] = {CONNV3_DRV_TYPE_BT, CONNV3_DRV_TYPE_WIFI, -1};

const char *radio_match_str[CONNV3_DRV_TYPE_MAX] = {"bt", "wifi", "md", "connv3"};

int mtk_connv3_probe(struct platform_device *pdev)
{
	int ret, i;
	struct device_node *node;
	u32 support = 0;

	pr_info("[%s] ++++++++++", __func__);

	node = pdev->dev.of_node;
	for (i = 0; i < CONNV3_DRV_TYPE_MAX; i++) {
		ret = of_property_match_string(node, "radio-support", radio_match_str[i]);
		if (ret < 0)
			pr_info("[%s] match str [%s] not found", __func__, radio_match_str[i]);
		else {
			support |= (0x1 << i);
			pr_info("[%s] match str [%s] found", __func__, radio_match_str[i]);
		}
	}
	pr_info("[%s] radio support=[%d]", __func__, support);

	if (support == 0) {
		pr_info("[%s] not support any radio", __func__);
		return 0;
	}

	g_connv3_drv_gen.drv_radio_support =
		(support & ((0x1 << CONNV3_DRV_TYPE_BT) | (0x1 << CONNV3_DRV_TYPE_WIFI)));
	pr_info("[%s] v3 radio support=0x%x, adap radio support=0x%x",
		__func__, support, g_connv3_drv_gen.drv_radio_support);

	/* hw init */
	ret = connv3_hw_init(pdev, &g_connv3_dev_cb);

	g_connv3_drv_dev = pdev;

	ret = connv3_core_init();
	if (ret)
		pr_notice("[%s] connv3 core init fail %d", __func__, ret);

	/* resume worker */
	INIT_WORK(&g_connv3_ap_resume_work, connv3_hw_ap_resume_handler);

	ret = conn_adaptor_register_drv_gen(CONN_ADAPTOR_DRV_GEN_CONNAC_3, &g_connv3_drv_gen);

#ifdef CFG_CONNINFRA_UT_SUPPORT
	ret = connv3_test_setup();
	if (ret)
		pr_notice("init connv3_test_setup fail, ret = %d\n", ret);
#endif

	atomic_set(&g_connv3_hw_init_done, 1);

	pr_info("[%s] --------------", __func__);
	return 0;
}

int mtk_connv3_remove(struct platform_device *pdev)
{
	atomic_set(&g_connv3_hw_init_done, 0);

	connv3_hw_deinit();

	if (g_connv3_drv_dev)
		g_connv3_drv_dev = NULL;

	return 0;
}

/* put log/variable to file node */
static OSAL_SLEEPABLE_LOCK g_log_node_lock;
static struct proc_dir_entry *g_connv3_log_node_entry;
#define CONNV3_LOG_NODE_PROCNAME "driver/connv3_chip_info"

#define CONN_LOG_NODE_BUF_SIZE	128
static char g_conn_log_node_buf[CONN_LOG_NODE_BUF_SIZE];
static char *g_log_node_buf_ptr;
static int g_log_node_buf_len;

enum log_node_type {
	LOG_NODE_CONNSYS_PMIC_ECID = 0,
	LOG_NODE_CONNSYS_IC_ECID = 1,
	LOG_NODE_CONNSYS_ADIE_CHIP_INFO = 2,
};

ssize_t connv3_log_node_write(struct file *filp, const char __user *buffer, size_t count, loff_t *f_pos)
{
	size_t len = count;
	char buf[256];
	char* pBuf;
	int x = 0, y = 0, z = 0;
	char* pToken = NULL;
	char* pDelimiter = " \t";
	long res = 0;
	int ret = 0;

	pr_info("write parameter len = %d\n\r", (int) len);
	if (len >= osal_sizeof(buf)) {
		pr_info("input handling fail!\n");
		return -1;
	}

	if (copy_from_user(buf, buffer, len))
		return -EFAULT;

	buf[len] = '\0';
	pr_info("write parameter data = %s\n\r", buf);

	pBuf = buf;
	pToken = osal_strsep(&pBuf, pDelimiter);
	if (pToken != NULL) {
		osal_strtol(pToken, 16, &res);
		x = (int)res;
	} else {
		x = 0;
	}

	pToken = osal_strsep(&pBuf, "\t\n ");
	if (pToken != NULL) {
		osal_strtol(pToken, 16, &res);
		y = (int)res;
		pr_info("y = 0x%08x\n\r", y);
	}

	pToken = osal_strsep(&pBuf, "\t\n ");
	if (pToken != NULL) {
		osal_strtol(pToken, 16, &res);
		z = (int)res;
	}
	pr_info("[%s] x(0x%08x), y(0x%08x), z(0x%08x)", __func__, x, y, z);

	ret = osal_lock_sleepable_lock(&g_log_node_lock);
	if (ret) {
		pr_err("g_log_node_lock fail!!");
		return ret;
	}

	switch(x) {
		case LOG_NODE_CONNSYS_PMIC_ECID:
			{
				memset(g_conn_log_node_buf, '\0', CONN_LOG_NODE_BUF_SIZE);
				connv3_hw_get_pmic_ic_info(g_conn_log_node_buf, CONN_LOG_NODE_BUF_SIZE);
				g_conn_log_node_buf[CONN_LOG_NODE_BUF_SIZE-1] = '\0';

				g_log_node_buf_len = strlen(g_conn_log_node_buf);
			}
			break;

		case LOG_NODE_CONNSYS_IC_ECID:
			{
				memset(g_conn_log_node_buf, '\0', CONN_LOG_NODE_BUF_SIZE);
				connv3_hw_get_connsys_ic_info(g_conn_log_node_buf, CONN_LOG_NODE_BUF_SIZE);
				g_conn_log_node_buf[CONN_LOG_NODE_BUF_SIZE-1] = '\0';

				g_log_node_buf_len = strlen(g_conn_log_node_buf);
			}
			break;
		case LOG_NODE_CONNSYS_ADIE_CHIP_INFO:
			{
				memset(g_conn_log_node_buf, '\0', CONN_LOG_NODE_BUF_SIZE);
				connv3_hw_get_connsys_adie_ic_info(g_conn_log_node_buf, CONN_LOG_NODE_BUF_SIZE);
				g_conn_log_node_buf[CONN_LOG_NODE_BUF_SIZE-1] = '\0';

				g_log_node_buf_len = strlen(g_conn_log_node_buf);
			}
			break;
	}
	osal_unlock_sleepable_lock(&g_log_node_lock);

	return len;
}

ssize_t connv3_log_node_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	int ret = 0;
	int dump_len;

	ret = osal_lock_sleepable_lock(&g_log_node_lock);
	if (ret) {
		pr_err("g_log_node_lock fail!!");
		return ret;
	}

	if (g_log_node_buf_len == 0)
		goto exit;

	if (*f_pos == 0)
		g_log_node_buf_ptr = g_conn_log_node_buf;

	dump_len = g_log_node_buf_len >= count ? count : g_log_node_buf_len;
	ret = copy_to_user(buf, g_log_node_buf_ptr, dump_len);
	if (ret) {
		pr_err("copy to dump info buffer failed, ret:%d\n", ret);
		ret = -EFAULT;
		goto exit;
	}

	*f_pos += dump_len;
	g_log_node_buf_len -= dump_len;
	g_log_node_buf_ptr += dump_len;
	pr_info("conn_dbg:after read,wmt for dump info buffer len(%d)\n", g_log_node_buf_len);

	ret = dump_len;
exit:
	osal_unlock_sleepable_lock(&g_log_node_lock);
	return ret;
}

int connv3_drv_init(void)
{
	int iret = 0, retry = 0;
	static DEFINE_RATELIMIT_STATE(_rs, HZ, 1);
	static const struct proc_ops connv3_log_node_fops = {
		.proc_read = connv3_log_node_read,
		.proc_write = connv3_log_node_write,
	};

	iret = platform_driver_probe(&g_mtk_connv3_dev_drv, mtk_connv3_probe);
	pr_info("[%s] driver register [%d]", __func__, iret);
	if (iret) {
		pr_err("Connv3 platform driver registered failed(%d)\n", iret);
		return -1;
	} else {
		while (atomic_read(&g_connv3_hw_init_done) == 0) {
			osal_sleep_ms(50);
			retry++;
			if (__ratelimit(&_rs))
				pr_info("g_connv3_hw_init_done = 0, retry = %d", retry);
		}
	}

	INIT_WORK(&g_connv3_pmic_work.pmic_work, connv3_dev_pmic_event_handler);

	g_connv3_log_node_entry = proc_create(CONNV3_LOG_NODE_PROCNAME, 0664, NULL, &connv3_log_node_fops);

	osal_sleepable_lock_init(&g_log_node_lock);

	pr_info("[%s] result [%d]\n", __func__, iret);
	return iret;
}

int connv3_drv_deinit(void)
{
	int ret;

	osal_sleepable_lock_deinit(&g_log_node_lock);

#ifdef CFG_CONNINFRA_UT_SUPPORT
	ret = connv3_test_remove();
#endif
	ret = conn_adaptor_unregister_drv_gen(CONN_ADAPTOR_DRV_GEN_CONNAC_3);
	ret = connv3_core_deinit();

	platform_driver_unregister(&g_mtk_connv3_dev_drv);
	return 0;
}
