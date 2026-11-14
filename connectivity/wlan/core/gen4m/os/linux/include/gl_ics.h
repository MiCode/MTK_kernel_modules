/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _FW_LOG_ICS_H_
#define _FW_LOG_ICS_H_

#include "gl_os.h"
#include "debug.h"
#include "precomp.h"

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
#include "wlan_ring.h"

#if ((CFG_SUPPORT_ICS == 1) || (CFG_SUPPORT_PHY_ICS == 1))

#define ICS_LOG_SIZE (128*1024)
#define ICS_WAIT_READY_MAX_CNT 2000
#define ICS_WAIT_READY_SLEEP_TIME 100
#define FW_LOG_ICS_DRIVER_NAME "fw_log_ics"

#define ICS_LOG_CMD_ON_OFF        0
#define ICS_LOG_CMD_SET_LEVEL     1

#define ICS_FW_LOG_IOC_MAGIC        (0xfc)
#define ICS_FW_LOG_IOCTL_ON_OFF \
	_IOW(ICS_FW_LOG_IOC_MAGIC, ICS_LOG_CMD_ON_OFF, int)
#define ICS_FW_LOG_IOCTL_SET_LEVEL \
	_IOW(ICS_FW_LOG_IOC_MAGIC, ICS_LOG_CMD_SET_LEVEL, int)

#define ICS_LOG_CMD_ON_OFF        0
#define ICS_LOG_CMD_SET_LEVEL     1

enum ENUM_ICS_LOG_LEVEL_T {
	ENUM_ICS_LOG_LEVEL_DISABLE,
	ENUM_ICS_LOG_LEVEL_MAC,
};

struct ICS_LOG_CACHE {
	u_int8_t fgOnOff;
	uint8_t ucLevel;
};

struct ics_ring {
	/* ring related variable */
	struct wlan_ring ring_cache;
	size_t ring_size;
	void *ring_base;
};

typedef void (*ics_fwlog_event_func_cb)(struct GLUE_INFO *, int,
	int, u_int8_t);

struct ics_dev {
	/* device related variable */
	struct cdev cdev;
	dev_t devno;
	struct class *driver_class;
	struct device *class_dev;
	int major;
	/* functional variable */
	struct ics_ring iRing;
	struct semaphore ioctl_mtx;
	ics_fwlog_event_func_cb pfFwEventFuncCB;
	wait_queue_head_t wq;
	struct ICS_LOG_CACHE rIcsLogCache;
};

u_int8_t ics_get_onoff(struct GLUE_INFO *prGlueInfo);
void ics_log_event_notification(struct GLUE_INFO *prGlueInfo,
	int cmd, int value, u_int8_t isOid);

extern ssize_t wifi_ics_fwlog_write(struct GLUE_INFO *prGlueInfo,
	char *buf, size_t count);
extern void wifi_ics_event_func_register(ics_fwlog_event_func_cb pfFwlog);

int IcsInit(void);
int IcsDeInit(struct GLUE_INFO *prGlueInfo);
int IcsDeInitAll(void);
#if CFG_SUPPORT_ICS_TIMER
void IcsTimerInit(struct ADAPTER *prAdapter);
void IcsLogStartWithTimer(struct ADAPTER *prAdapter);
void IcsLogTimeout(struct ADAPTER *prAdapter, uintptr_t ulParamPtr);
#endif /* CFG_SUPPORT_ICS_TIMER */
#endif /* CFG_SUPPORT_ICS */

#endif /*_FW_LOG_ICS_H_*/
