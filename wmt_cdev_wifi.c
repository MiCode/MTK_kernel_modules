/*
* Copyright (C) 2016 MediaTek Inc.
*
* This program is free software: you can redistribute it and/or modify it under the terms of the
* GNU General Public License version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with this program.
* If not, see <http://www.gnu.org/licenses/>.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <asm/current.h>
#include <linux/uaccess.h>
#include <linux/fcntl.h>
#include <linux/poll.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/netdevice.h>
#include <linux/inetdevice.h>
#include <linux/string.h>

#include "fw_log_wifi.h"
#include "wmt_exp.h"
#include "stp_exp.h"

MODULE_LICENSE("Dual BSD/GPL");

#define WIFI_DRIVER_NAME "mtk_wmt_wifi_chrdev"
#define WIFI_DEV_MAJOR 153

#define PFX                         "[MTK-WIFI] "
#define WIFI_LOG_DBG                  3
#define WIFI_LOG_INFO                 2
#define WIFI_LOG_WARN                 1
#define WIFI_LOG_ERR                  0

uint32_t gDbgLevel = WIFI_LOG_DBG;

#define WIFI_DBG_FUNC(fmt, arg...)	\
	do { \
		if (gDbgLevel >= WIFI_LOG_DBG) \
			pr_info(PFX "%s[D]: " fmt, __func__, ##arg); \
	} while (0)
#define WIFI_INFO_FUNC(fmt, arg...)	\
	do { \
		if (gDbgLevel >= WIFI_LOG_INFO) \
			pr_info(PFX "%s[I]: " fmt, __func__, ##arg); \
	} while (0)
#define WIFI_INFO_FUNC_LIMITED(fmt, arg...)	\
	do { \
		if (gDbgLevel >= WIFI_LOG_INFO) \
			pr_info_ratelimited(PFX "%s[L]: " fmt, __func__, ##arg); \
	} while (0)
#define WIFI_WARN_FUNC(fmt, arg...)	\
	do { \
		if (gDbgLevel >= WIFI_LOG_WARN) \
			pr_info(PFX "%s[W]: " fmt, __func__, ##arg); \
	} while (0)
#define WIFI_ERR_FUNC(fmt, arg...)	\
	do { \
		if (gDbgLevel >= WIFI_LOG_ERR) \
			pr_info(PFX "%s[E]: " fmt, __func__, ##arg); \
	} while (0)

#define VERSION "2.0"

static int32_t WIFI_devs = 1;
static int32_t WIFI_major = WIFI_DEV_MAJOR;
module_param(WIFI_major, uint, 0);
static struct cdev WIFI_cdev;
#if CREATE_NODE_DYNAMIC
static struct class *wmtwifi_class;
static struct device *wmtwifi_dev;
#endif

static struct semaphore wr_mtx;

#define WLAN_IFACE_NAME "wlan0"

enum {
	WLAN_MODE_HALT,
	WLAN_MODE_AP,
	WLAN_MODE_STA_P2P,
	WLAN_MODE_STA_AP_P2P,
	WLAN_MODE_MAX
};
static int32_t wlan_mode = WLAN_MODE_HALT;
static int32_t powered;
static int32_t isconcurrent;
static char *ifname = WLAN_IFACE_NAME;

/*******************************************************************
 */
enum ENUM_RESET_STATUS {
	RESET_FAIL,
	RESET_SUCCESS
};

/*
 *  enable = 1, mode = 0  => init P2P network
 *  enable = 1, mode = 1  => init Soft AP network
 *  enable = 0  => uninit P2P/AP network
 */
struct PARAM_CUSTOM_P2P_SET_STRUCT {
	uint32_t u4Enable;
	uint32_t u4Mode;
};
typedef int32_t(*set_p2p_mode) (struct net_device *netdev, struct PARAM_CUSTOM_P2P_SET_STRUCT p2pmode);

static set_p2p_mode pf_set_p2p_mode;
VOID register_set_p2p_mode_handler(set_p2p_mode handler)
{
	WIFI_INFO_FUNC("(pid %d) register set p2p mode handler %p\n", current->pid, handler);
	pf_set_p2p_mode = handler;
}
EXPORT_SYMBOL(register_set_p2p_mode_handler);

enum ENUM_WLAN_DRV_BUF_TYPE_T {
	BUF_TYPE_NVRAM,
	BUF_TYPE_DRV_CFG,
	BUF_TYPE_FW_CFG,
	BUF_TYPE_NUM
};

typedef uint8_t(*file_buf_handler)(void *ctx, const char __user *buf, uint16_t length);
static file_buf_handler buf_handler[BUF_TYPE_NUM];
static void *buf_handler_ctx[BUF_TYPE_NUM];
void register_file_buf_handler(file_buf_handler handler, void *handler_ctx, uint8_t ucType)
{
	if (ucType < BUF_TYPE_NUM) {
		buf_handler[ucType] = handler;
		buf_handler_ctx[ucType] = handler_ctx;
	}
}
EXPORT_SYMBOL(register_file_buf_handler);

/*******************************************************************
 *  WHOLE CHIP RESET PROCEDURE:
 *
 *  WMTRSTMSG_RESET_START callback
 *  -> wlanRemove
 *  -> WMTRSTMSG_RESET_END callback
 *
 *******************************************************************
*/
/*-----------------------------------------------------------------*/
/*
 *  Receiving RESET_START message
 */
/*-----------------------------------------------------------------*/
int32_t wifi_reset_start(VOID)
{
	struct net_device *netdev = NULL;
	struct PARAM_CUSTOM_P2P_SET_STRUCT p2pmode;

	down(&wr_mtx);

	if (powered == 1) {
		netdev = dev_get_by_name(&init_net, ifname);
		if (netdev == NULL) {
			WIFI_ERR_FUNC("Fail to get %s net device\n", ifname);
		} else {
			p2pmode.u4Enable = 0;
			p2pmode.u4Mode = 0;

			if (pf_set_p2p_mode) {
				if (pf_set_p2p_mode(netdev, p2pmode) != 0)
					WIFI_ERR_FUNC("Turn off p2p/ap mode fail");
				else
					WIFI_INFO_FUNC("Turn off p2p/ap mode");
			}
			dev_put(netdev);
			netdev = NULL;
		}
	} else {
		/* WIFI is off before whole chip reset, do nothing */
	}

	return 0;
}
EXPORT_SYMBOL(wifi_reset_start);

/*-----------------------------------------------------------------*/
/*
 *  Receiving RESET_END/RESET_END_FAIL message
 */
/*-----------------------------------------------------------------*/
int32_t wifi_reset_end(enum ENUM_RESET_STATUS status)
{
	struct net_device *netdev = NULL;
	struct PARAM_CUSTOM_P2P_SET_STRUCT p2pmode;
	int32_t wait_cnt = 0;
	int32_t ret = -1;

	if (status == RESET_FAIL) {
		/* whole chip reset fail, donot recover WIFI */
		ret = 0;
		up(&wr_mtx);
	} else if (status == RESET_SUCCESS) {
		WIFI_WARN_FUNC("WIFI state recovering...\n");

		if (powered == 1) {
			/* WIFI is on before whole chip reset, reopen it now */
			if (mtk_wcn_wmt_func_on(WMTDRV_TYPE_WIFI) == MTK_WCN_BOOL_FALSE) {
				WIFI_ERR_FUNC("WMT turn on WIFI fail!\n");
				goto done;
			} else {
				WIFI_INFO_FUNC("WMT turn on WIFI success!\n");
			}

			if (pf_set_p2p_mode == NULL) {
				WIFI_ERR_FUNC("Set p2p mode handler is NULL\n");
				goto done;
			}

			netdev = dev_get_by_name(&init_net, ifname);
			while (netdev == NULL && wait_cnt < 10) {
				WIFI_ERR_FUNC("Fail to get %s net device, sleep 300ms\n", ifname);
				msleep(300);
				wait_cnt++;
				netdev = dev_get_by_name(&init_net, ifname);
			}
			if (wait_cnt >= 10) {
				WIFI_ERR_FUNC("Get %s net device timeout\n", ifname);
				goto done;
			}

			if (wlan_mode == WLAN_MODE_STA_P2P) {
				p2pmode.u4Enable = 1;
				p2pmode.u4Mode = 0;
				if (pf_set_p2p_mode(netdev, p2pmode) != 0) {
					WIFI_ERR_FUNC("Set wlan mode 0 fail\n");
				} else {
					WIFI_WARN_FUNC("Set wlan mode %d\n", WLAN_MODE_STA_P2P);
					ret = 0;
				}
			} else if (wlan_mode == WLAN_MODE_AP) {
				p2pmode.u4Enable = 1;
				p2pmode.u4Mode = 1;
				if (pf_set_p2p_mode(netdev, p2pmode) != 0) {
					WIFI_ERR_FUNC("Set wlan mode 1 fail\n");
				} else {
					WIFI_WARN_FUNC("Set wlan mode %d\n", WLAN_MODE_AP);
					ret = 0;
				}
			} else if (wlan_mode == WLAN_MODE_STA_AP_P2P) {
				p2pmode.u4Enable = 1;
				p2pmode.u4Mode = 3;
				if (pf_set_p2p_mode(netdev, p2pmode) != 0) {
					WIFI_ERR_FUNC("Set wlan mode 3 fail\n");
				} else {
					WIFI_WARN_FUNC("Set wlan mode %d\n", WLAN_MODE_STA_AP_P2P);
					ret = 0;
				}
			}
done:
			if (netdev != NULL)
				dev_put(netdev);
		} else {
			/* WIFI is off before whole chip reset, do nothing */
			ret = 0;
		}
		up(&wr_mtx);
	}

	return ret;
}
EXPORT_SYMBOL(wifi_reset_end);

static int WIFI_open(struct inode *inode, struct file *file)
{
	WIFI_INFO_FUNC("major %d minor %d (pid %d)\n", imajor(inode), iminor(inode), current->pid);

	return 0;
}

static int WIFI_close(struct inode *inode, struct file *file)
{
	WIFI_INFO_FUNC("major %d minor %d (pid %d)\n", imajor(inode), iminor(inode), current->pid);

	return 0;
}

ssize_t WIFI_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	INT32 retval = -EIO;
	INT8 local[20] = { 0 };
	struct net_device *netdev = NULL;
	struct PARAM_CUSTOM_P2P_SET_STRUCT p2pmode;
	int32_t wait_cnt = 0;
	int copy_size = 0;

	down(&wr_mtx);
	if (count <= 0) {
		WIFI_ERR_FUNC("WIFI_write invalid param\n");
		goto done;
	}

	copy_size = min(sizeof(local) - 1, count);
	if (copy_from_user(local, buf, copy_size) == 0) {
		WIFI_INFO_FUNC("WIFI_write %s, length %zu\n", local, count);

		if (local[0] == '0') {
			if (powered == 0) {
				WIFI_INFO_FUNC("WIFI is already power off!\n");
				retval = count;
				wlan_mode = WLAN_MODE_HALT;
				goto done;
			}

			netdev = dev_get_by_name(&init_net, ifname);
			if (netdev == NULL) {
				WIFI_ERR_FUNC("Fail to get %s net device\n", ifname);
			} else {
				p2pmode.u4Enable = 0;
				p2pmode.u4Mode = 0;

				if (pf_set_p2p_mode) {
					if (pf_set_p2p_mode(netdev, p2pmode) != 0) {
						WIFI_ERR_FUNC("Turn off p2p/ap mode fail");
					} else {
						WIFI_INFO_FUNC("Turn off p2p/ap mode");
						wlan_mode = WLAN_MODE_HALT;
					}
				}
				dev_put(netdev);
				netdev = NULL;
			}

			if (mtk_wcn_wmt_func_off(WMTDRV_TYPE_WIFI) == MTK_WCN_BOOL_FALSE) {
				WIFI_ERR_FUNC("WMT turn off WIFI fail!\n");
			} else {
				WIFI_INFO_FUNC("WMT turn off WIFI OK!\n");
				powered = 0;
				retval = count;
				wlan_mode = WLAN_MODE_HALT;
			}
		} else if (local[0] == '1') {
			if (powered == 1) {
				WIFI_INFO_FUNC("WIFI is already power on!\n");
				retval = count;
				goto done;
			}

			if (mtk_wcn_wmt_func_on(WMTDRV_TYPE_WIFI) == MTK_WCN_BOOL_FALSE) {
				WIFI_ERR_FUNC("WMT turn on WIFI fail!\n");
			} else {
				powered = 1;
				retval = count;
				WIFI_INFO_FUNC("WMT turn on WIFI success!\n");
				wlan_mode = WLAN_MODE_HALT;
			}
		} else if (!strncmp(local, "WR-BUF:", 7)) {
			file_buf_handler handler = NULL;
			VOID *ctx = NULL;

			if (!strncmp(&local[7], "NVRAM", 5)) {
				copy_size = count - 12;
				buf += 12;
				handler = buf_handler[BUF_TYPE_NVRAM];
				ctx = buf_handler_ctx[BUF_TYPE_NVRAM];
			} else if (!strncmp(&local[7], "DRVCFG", 6)) {
				copy_size = count - 13;
				buf += 13;
				handler = buf_handler[BUF_TYPE_DRV_CFG];
				ctx = buf_handler_ctx[BUF_TYPE_DRV_CFG];
			} else if (!strncmp(&local[7], "FWCFG", 5)) {
				copy_size = count - 12;
				buf += 12;
				handler = buf_handler[BUF_TYPE_FW_CFG];
				ctx = buf_handler_ctx[BUF_TYPE_FW_CFG];
			}
			if (handler && !handler(ctx, buf, (UINT16)copy_size))
				retval = count;
			else
				retval = -ENOTSUPP;
		} else if (!strncmp(local, "RM-BUF:", 7)) {
			file_buf_handler handler = NULL;
			VOID *ctx = NULL;

			if (!strncmp(&local[7], "DRVCFG", 6)) {
				handler = buf_handler[BUF_TYPE_DRV_CFG];
				ctx = buf_handler_ctx[BUF_TYPE_DRV_CFG];
			} else if (!strncmp(&local[7], "FWCFG", 5)) {
				handler = buf_handler[BUF_TYPE_FW_CFG];
				ctx = buf_handler_ctx[BUF_TYPE_FW_CFG];
			}

			if (handler && !handler(ctx, NULL, 0))
				retval = count;
			else
				retval = -ENOTSUPP;
		} else if (local[0] == 'S' || local[0] == 'P' || local[0] == 'A') {
			if (powered == 0) {
				/* If WIFI is off, turn on WIFI first */
				if (mtk_wcn_wmt_func_on(WMTDRV_TYPE_WIFI) == MTK_WCN_BOOL_FALSE) {
					WIFI_ERR_FUNC("WMT turn on WIFI fail!\n");
					goto done;
				} else {
					powered = 1;
					WIFI_INFO_FUNC("WMT turn on WIFI success!\n");
					wlan_mode = WLAN_MODE_HALT;
				}
			}

			if (pf_set_p2p_mode == NULL) {
				WIFI_ERR_FUNC("Set p2p mode handler is NULL\n");
				goto done;
			}

			netdev = dev_get_by_name(&init_net, ifname);
			while (netdev == NULL && wait_cnt < 10) {
				WIFI_ERR_FUNC("Fail to get %s net device, sleep 300ms\n", ifname);
				msleep(300);
				wait_cnt++;
				netdev = dev_get_by_name(&init_net, ifname);
			}
			if (wait_cnt >= 10) {
				WIFI_ERR_FUNC("Get %s net device timeout\n", ifname);
				goto done;
			}

			if ((wlan_mode == WLAN_MODE_STA_P2P && (local[0] == 'S' || local[0] == 'P')) ||
			    (wlan_mode == WLAN_MODE_AP && (local[0] == 'A')) ||
			    (wlan_mode == WLAN_MODE_STA_AP_P2P)) {
				WIFI_INFO_FUNC("WIFI is already in mode %d!\n", wlan_mode);
				retval = count;
				goto done;
			}

			if (isconcurrent) {
				p2pmode.u4Enable = 1;
				p2pmode.u4Mode = 3;
				if (pf_set_p2p_mode(netdev, p2pmode) != 0) {
					WIFI_ERR_FUNC("Set wlan mode fail\n");
				} else {
					WIFI_INFO_FUNC("Set wlan mode %d --> %d\n", wlan_mode, WLAN_MODE_STA_AP_P2P);
					wlan_mode = WLAN_MODE_STA_AP_P2P;
					retval = count;
					goto done;
				}
			}

			if ((wlan_mode == WLAN_MODE_AP && (local[0] == 'S' || local[0] == 'P')) ||
			    (wlan_mode == WLAN_MODE_STA_P2P && (local[0] == 'A'))) {
				p2pmode.u4Enable = 0;
				p2pmode.u4Mode = 0;
				if (pf_set_p2p_mode(netdev, p2pmode) != 0) {
					WIFI_ERR_FUNC("Turn off p2p/ap mode fail");
					goto done;
				}
			}

			if (local[0] == 'S' || local[0] == 'P') {
				p2pmode.u4Enable = 1;
				p2pmode.u4Mode = 0;
				if (pf_set_p2p_mode(netdev, p2pmode) != 0) {
					WIFI_ERR_FUNC("Set wlan mode fail\n");
				} else {
					WIFI_INFO_FUNC("Set wlan mode %d --> %d\n", wlan_mode, WLAN_MODE_STA_P2P);
					wlan_mode = WLAN_MODE_STA_P2P;
					retval = count;
				}
			} else if (local[0] == 'A') {
				p2pmode.u4Enable = 1;
				p2pmode.u4Mode = 1;
				if (pf_set_p2p_mode(netdev, p2pmode) != 0) {
					WIFI_ERR_FUNC("Set wlan mode fail\n");
				} else {
					WIFI_INFO_FUNC("Set wlan mode %d --> %d\n", wlan_mode, WLAN_MODE_AP);
					wlan_mode = WLAN_MODE_AP;
					retval = count;
				}
			}
			dev_put(netdev);
			netdev = NULL;
		} else if (local[0] == 'C') {
			isconcurrent = 1;
			WIFI_INFO_FUNC("Enable concurrent mode\n");
			retval = count;
		} else if (local[0] == 'N') {
			isconcurrent = 0;
			WIFI_INFO_FUNC("Disable concurrent mode\n");
			retval = count;
		}
	}
done:
	if (netdev != NULL)
		dev_put(netdev);

	up(&wr_mtx);
	return retval;
}

const struct file_operations WIFI_fops = {
	.open = WIFI_open,
	.release = WIFI_close,
	.write = WIFI_write,
};

static int WIFI_init(void)
{
	dev_t dev = MKDEV(WIFI_major, 0);
	int32_t alloc_ret = 0;
	int32_t cdev_err = 0;

	/* Allocate char device */
	alloc_ret = register_chrdev_region(dev, WIFI_devs, WIFI_DRIVER_NAME);
	if (alloc_ret) {
		WIFI_ERR_FUNC("Fail to register device numbers\n");
		return alloc_ret;
	}

	cdev_init(&WIFI_cdev, &WIFI_fops);
	WIFI_cdev.owner = THIS_MODULE;

	cdev_err = cdev_add(&WIFI_cdev, dev, WIFI_devs);
	if (cdev_err)
		goto error;

#if CREATE_NODE_DYNAMIC	/* mknod replace */
	wmtwifi_class = class_create(THIS_MODULE, "wmtWifi");
	if (IS_ERR(wmtwifi_class))
		goto error;
	wmtwifi_dev = device_create(wmtwifi_class, NULL, dev, NULL, "wmtWifi");
	if (IS_ERR(wmtwifi_dev))
		goto error;
#endif

	sema_init(&wr_mtx, 1);

	WIFI_INFO_FUNC("%s driver(major %d) installed\n", WIFI_DRIVER_NAME, WIFI_major);

#ifdef CONFIG_MTK_CONNSYS_DEDICATED_LOG_PATH
	if (fw_log_wifi_init() < 0) {
		WIFI_INFO_FUNC("connsys debug node init failed!!\n");
		return -1;
	}
#endif

	return 0;

error:
#if CREATE_NODE_DYNAMIC
	if (wmtwifi_dev && !IS_ERR(wmtwifi_dev)) {
		device_destroy(wmtwifi_class, dev);
		wmtwifi_dev = NULL;
	}
	if (wmtwifi_class && !IS_ERR(wmtwifi_class)) {
		class_destroy(wmtwifi_class);
		wmtwifi_class = NULL;
	}
#endif
	if (cdev_err == 0)
		cdev_del(&WIFI_cdev);

	if (alloc_ret == 0)
		unregister_chrdev_region(dev, WIFI_devs);

	return -1;
}

static void WIFI_exit(void)
{
	dev_t dev = MKDEV(WIFI_major, 0);

#if CREATE_NODE_DYNAMIC
	if (wmtwifi_dev && !IS_ERR(wmtwifi_dev)) {
		device_destroy(wmtwifi_class, dev);
		wmtwifi_dev = NULL;
	}
	if (wmtwifi_class && !IS_ERR(wmtwifi_class)) {
		class_destroy(wmtwifi_class);
		wmtwifi_class = NULL;
	}
#endif

	cdev_del(&WIFI_cdev);
	unregister_chrdev_region(dev, WIFI_devs);

	WIFI_INFO_FUNC("%s driver removed\n", WIFI_DRIVER_NAME);

#ifdef CONFIG_MTK_CONNSYS_DEDICATED_LOG_PATH
	fw_log_wifi_deinit();
#endif
}

#ifdef MTK_WCN_BUILT_IN_DRIVER

int mtk_wcn_wmt_wifi_init(void)
{
	return WIFI_init();
}
EXPORT_SYMBOL(mtk_wcn_wmt_wifi_init);

void mtk_wcn_wmt_wifi_exit(void)
{
	return WIFI_exit();
}
EXPORT_SYMBOL(mtk_wcn_wmt_wifi_exit);

#else

module_init(WIFI_init);
module_exit(WIFI_exit);

#endif
