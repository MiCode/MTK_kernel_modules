#ifndef _BT_EXP_H_
#define _BT_EXP_H_

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
#include <linux/device.h>
#include <linux/printk.h>
#include <linux/uio.h>

#include "wmt_exp.h"
#include "stp_exp.h"


#ifdef CONFIG_MTK_CONNSYS_DEDICATED_LOG_PATH
#include "connsys_debug_utility.h"

/* Flags to control BT FW log flow */
#define OFF 0x00
#define ON  0xff

/* *****************************************************************************************
 * BT Logger Tool will send 3 levels(Low, SQC and Debug)
 * Driver will not check its range so we can provide capability of extention.
 ******************************************************************************************/
#define DEFAULT_LEVEL 0x02 /* 0x00:OFF, 0x01: LOW POWER, 0x02: SQC, 0x03: DEBUG */

extern int  fw_log_bt_init(void);
extern void fw_log_bt_exit(void);
extern void bt_state_notify(UINT32 on_off);
extern ssize_t send_hci_frame(const PUINT8 buf, size_t count);

#endif
#endif
