// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>
#include <linux/ratelimit.h>
#include <linux/alarmtimer.h>
#include <linux/suspend.h>
#include <linux/rtc.h>
#include <linux/vmalloc.h>

#include "ring.h"
#include "connv3_mcu_log.h"
#include "connsyslog_to_user.h"

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/* Close debug log */
//#define DEBUG_RING 1

#define CONNLOG_ALARM_STATE_DISABLE	0x0
#define CONNLOG_ALARM_STATE_ENABLE	0x01
#define CONNLOG_ALARM_STATE_RUNNING	0x03

#define BYETES_PER_LINE 16
#define LOG_LINE_SIZE (3*BYETES_PER_LINE + BYETES_PER_LINE + 1)
#define IS_VISIBLE_CHAR(c) ((c) >= 32 && (c) <= 126)

#define LOG_MAX_LEN 1024
#define LOG_HEAD_LENG 16
#define TIMESYNC_LENG 40

static const char log_head[] = {0x55, 0x00, 0x00, 0x62};
static const char timesync_head[] = {0x55, 0x00, 0x25, 0x62};

struct connv3_log_buffer {
	struct ring ring_cache;
	void *cache_base;
};

struct connv3_log_ctrl_block {
	bool enable;
	u32 size;
	struct connv3_log_buffer log_buffer;
	void (*log_data_handler)(void);
};

struct connv3_log_dev {
	int conn_type;
	struct connv3_log_ctrl_block ctrl_block[CONNV3_LOG_TYPE_SIZE];

	spinlock_t irq_lock;
	unsigned long flags;
	void *log_data;

};

static char *type_to_title[CONNV3_DEBUG_TYPE_SIZE] = {
	"wifi_fw", "bt_fw"
};

static struct connv3_log_dev* g_connv3_log_dev[CONNV3_DEBUG_TYPE_SIZE];

#ifdef CONFIG_MTK_CONNSYS_DEDICATED_LOG_PATH
static atomic_t g_connv3_log_mode = ATOMIC_INIT(LOG_TO_FILE);
#else
static atomic_t g_connv3_log_mode = ATOMIC_INIT(PRINT_TO_KERNEL_LOG);
#endif

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

static void connv3_log_event_set(struct connv3_log_ctrl_block *block);
static struct connv3_log_dev* connv3_log_subsys_init(
	int conn_type, u32 primary_buf_size, u32 mcu_buf_size);

static void connv3_log_subsys_deinit(struct connv3_log_dev* handler);
static ssize_t connv3_read_internal(
	struct connv3_log_dev* handler, int conn_type, int block_type,
	char *buf, char __user *userbuf, size_t count, bool to_user);

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

void *connv3_log_cache_allocate(size_t size)
{
	void *pBuffer = NULL;

	if (size > (PAGE_SIZE << 1))
		pBuffer = vmalloc(size);
	else
	pBuffer = kmalloc(size, GFP_KERNEL);

	/* If there is fragment, kmalloc may not get memory when size > one page.
	 * For this case, use vmalloc instead.
	 */
	if (pBuffer == NULL && size > PAGE_SIZE)
		pBuffer = vmalloc(size);

	return pBuffer;
}

void connv3_log_cache_free(const void *dst)
{
	kvfree(dst);
}

/*****************************************************************************
 * FUNCTION
 *  connv3_log_event_set
 * DESCRIPTION
 *  Trigger  event call back to wakeup waitqueue
 * PARAMETERS
 *  conn_type      [IN]        subsys type
 * RETURNS
 *  void
 *****************************************************************************/
static void connv3_log_event_set(struct connv3_log_ctrl_block *block)
{
	if (block->log_data_handler)
		block->log_data_handler();
}


static unsigned int connv3_cal_log_size(unsigned int emi_size)
{
	int position;
	int i;

	if (emi_size > 0) {
		for (i = (emi_size), position = 0; i != 0; ++position)
			i >>= 1;
	} else {
		return 0;
	}

	return (1UL << position);
}

/*****************************************************************************
* FUNCTION
*  connlog_emi_deinit
* DESCRIPTION
*  Do iounmap for log buffer on EMI
* PARAMETERS
*  void
* RETURNS
*  void
*****************************************************************************/

static int connv3_log_buffer_init(struct connv3_log_ctrl_block *block, u32 sz)
{
	void *pBuffer = NULL;
	unsigned int cache_size = 0;
	unsigned int cal_size;

	//if (sz > 16 MB) return fail;
	cal_size = connv3_cal_log_size(sz);
	if (cal_size == 0) {
		return -1;
	}

	/* init ring cache */
	block->size = sz;
	cache_size = cal_size * 2;
	pBuffer = connv3_log_cache_allocate(cache_size);

	if (pBuffer == NULL) {
		pr_info("[%s] allocate cache fail.", __func__);
		return -ENOMEM;
	}

	block->log_buffer.cache_base = pBuffer;
	memset(block->log_buffer.cache_base, 0, cache_size);
	ring_init(
		block->log_buffer.cache_base,
		cache_size,
		0,
		0,
		&block->log_buffer.ring_cache);

	return 0;
}

static int connlog_ring_buffer_init(struct connv3_log_dev* handler, u32 primary_size, u32 mcu_size)
{
	int ret;

	if (handler->conn_type < 0 || handler->conn_type >= CONNV3_DEBUG_TYPE_SIZE) {
		pr_notice("%s invalid conn_type %d\n", __func__, handler->conn_type);
		return -1;
	}

	if (primary_size > 0) {
		ret = connv3_log_buffer_init(&handler->ctrl_block[CONNV3_LOG_TYPE_PRIMARY], primary_size);
		if (ret)
			pr_warn("[%s] init primary buffer fail %d", __func__, ret);
	}
	if (mcu_size > 0) {
		ret = connv3_log_buffer_init(&handler->ctrl_block[CONNV3_LOG_TYPE_MCU], mcu_size);
		if (ret)
			pr_warn("[%s] init mcu buffer fail %d", __func__, ret);
	}

	return 0;
}

/*****************************************************************************
* FUNCTION
*  connv3_log_ring_buffer_deinit
* DESCRIPTION
*  Initialize ring buffer setting for subsys
* PARAMETERS
*  void
* RETURNS
*  void
*****************************************************************************/
static void connv3_log_ring_buffer_deinit(struct connv3_log_dev* handler)
{
	int i;
	struct connv3_log_ctrl_block *block;

	for (i = 0; i < CONNV3_LOG_TYPE_SIZE; i++) {
		block = &handler->ctrl_block[i];
		if (block->enable && block->log_buffer.cache_base) {
			connv3_log_cache_free(block->log_buffer.cache_base);
			block->log_buffer.cache_base = NULL;
		}
	}
}


/*****************************************************************************
* FUNCTION
*  connv3_log_get_buf_size
* DESCRIPTION
*  Get ring buffer unread size on EMI.
* PARAMETERS
*  conn_type      [IN]        subsys type
* RETURNS
*  unsigned int    Ring buffer unread size
*****************************************************************************/
unsigned int connv3_log_get_buf_size(int conn_type)
{
	struct connv3_log_dev* handler;

	if (conn_type < CONNV3_DEBUG_TYPE_WIFI || conn_type >= CONNV3_DEBUG_TYPE_SIZE)
		return 0;

	handler = g_connv3_log_dev[conn_type];
	if (handler == NULL) {
		pr_err("[%s][%s] didn't init\n", __func__, type_to_title[conn_type]);
		return 0;
	}

	return RING_SIZE(&handler->ctrl_block[CONNV3_LOG_TYPE_PRIMARY].log_buffer.ring_cache);
}
EXPORT_SYMBOL(connv3_log_get_buf_size);


unsigned int connv3_log_get_mcu_buf_size(int conn_type)
{
	struct connv3_log_dev* handler;

	if (conn_type < CONNV3_DEBUG_TYPE_WIFI || conn_type >= CONNV3_DEBUG_TYPE_SIZE)
		return 0;

	handler = g_connv3_log_dev[conn_type];
	if (handler == NULL) {
		pr_err("[%s][%s] didn't init\n", __func__, type_to_title[conn_type]);
		return 0;
	}

	return RING_SIZE(&handler->ctrl_block[CONNV3_LOG_TYPE_MCU].log_buffer.ring_cache);
}


/*****************************************************************************
 * FUNCTION
 *  connsys_log_read_internal
 * DESCRIPTION
 *  Read log in ring_cache to buf
 * PARAMETERS
 *
 * RETURNS
 *
 *****************************************************************************/
static ssize_t connv3_read_internal(
	struct connv3_log_dev* handler, int conn_type, int block_type,
	char *buf, char __user *userbuf, size_t count, bool to_user)
{
	unsigned int written = 0;
	unsigned int cache_buf_size;
	struct ring_segment ring_seg;
	struct ring *ring = NULL;
	unsigned int size = 0;
	int retval;
#ifndef DEBUG_LOG_ON
	static DEFINE_RATELIMIT_STATE(_rs, 10 * HZ, 1);

	ratelimit_set_flags(&_rs, RATELIMIT_MSG_ON_RELEASE);
#endif

	if (conn_type < 0 || conn_type >= CONNV3_DEBUG_TYPE_SIZE) {
		pr_notice("%s conn_type %d is invalid\n", __func__, conn_type);
		return 0;
	}

	if (block_type < 0 || block_type >= CONNV3_LOG_TYPE_SIZE) {
		pr_notice("%s block_type %d is invalid\n", __func__, block_type);
		return 0;
	}

	ring = &handler->ctrl_block[block_type].log_buffer.ring_cache;

	size = count < RING_SIZE(ring) ? count : RING_SIZE(ring);
	if (RING_EMPTY(ring) || !ring_read_prepare(size, &ring_seg, ring)) {
		pr_err("type(%d) no data, possibly taken by concurrent reader.\n", conn_type);
		goto done;
	}
	cache_buf_size = ring_seg.remain;

	RING_READ_FOR_EACH(size, ring_seg, ring) {
		if (to_user) {
			retval = copy_to_user(userbuf + written, ring_seg.ring_pt, ring_seg.sz);
			if (retval) {
		#ifndef DEBUG_LOG_ON
				if (__ratelimit(&_rs))
		#endif
					pr_err("copy to user buffer failed, ret:%d\n", retval);
				goto done;
			}
		} else {
			memcpy(buf + written, ring_seg.ring_pt, ring_seg.sz);;
		}
		cache_buf_size -= ring_seg.sz;
		written += ring_seg.sz;
	}

done:
	return written;
}

/*****************************************************************************
 * FUNCTION
 *  connsys_log_read_to_user
 * DESCRIPTION
 *  Read log in ring_cache to user space buf
 * PARAMETERS
 *
 * RETURNS
 *
 *****************************************************************************/
ssize_t _connv3_log_read_to_user(int conn_type, int block_type, char __user *buf, size_t count)
{
	struct connv3_log_dev* handler;
	unsigned int written = 0;
	static DEFINE_RATELIMIT_STATE(_rs, 10 * HZ, 1);
	static unsigned int written_times[CONNV3_DEBUG_TYPE_SIZE][CONNV3_LOG_TYPE_SIZE];
	static size_t written_cnt[CONNV3_DEBUG_TYPE_SIZE][CONNV3_LOG_TYPE_SIZE];

	ratelimit_set_flags(&_rs, RATELIMIT_MSG_ON_RELEASE);

	if (conn_type < CONNV3_DEBUG_TYPE_WIFI || conn_type >= CONNV3_DEBUG_TYPE_SIZE)
		goto done;
	if (block_type < CONNV3_LOG_TYPE_PRIMARY || block_type >= CONNV3_LOG_TYPE_SIZE)
		goto done;
	if (atomic_read(&g_connv3_log_mode) != LOG_TO_FILE)
		goto done;

	handler = g_connv3_log_dev[conn_type];
	if (handler == NULL) {
		pr_err("[%s][%s] not init\n", __func__, type_to_title[conn_type]);
		goto done;
	}
	written = connv3_read_internal(handler, conn_type, block_type, NULL, buf, count, true);

	written_times[conn_type][block_type]++;
	written_cnt[conn_type][block_type] += written;
	if (__ratelimit(&_rs)) {
		pr_info("[%s] type=[%d] blcok=[%d] write %d times, %d bytes",
			__func__, conn_type, block_type,
			written_times[conn_type][block_type],
			written_cnt[conn_type][block_type]);
		written_times[conn_type][block_type] = 0;
		written_cnt[conn_type][block_type] = 0;
	}

done:
	return written;
}

ssize_t connv3_log_read_to_user(int conn_type, char __user *buf, size_t count)
{
	return _connv3_log_read_to_user(conn_type, CONNV3_LOG_TYPE_PRIMARY, buf, count);
}
EXPORT_SYMBOL(connv3_log_read_to_user);


ssize_t connv3_mcu_log_read_to_user(int conn_type, char __user *buf, size_t count)
{
	return _connv3_log_read_to_user(conn_type, CONNV3_LOG_TYPE_MCU, buf, count);
}

/*****************************************************************************
 * FUNCTION
 *  connsys_log_read
 * DESCRIPTION
 *  Read log in ring_cache to buf
 * PARAMETERS
 *
 * RETURNS
 *
 *****************************************************************************/
ssize_t connv3_log_read(int conn_type, char *buf, size_t count)
{
	unsigned int ret = 0;
	struct connv3_log_dev* handler;

	if (conn_type < CONNV3_DEBUG_TYPE_WIFI || conn_type >= CONNV3_DEBUG_TYPE_SIZE)
		goto done;
	if (atomic_read(&g_connv3_log_mode) != LOG_TO_FILE)
		goto done;

	handler = g_connv3_log_dev[conn_type];
	ret = connv3_read_internal(handler, conn_type, CONNV3_LOG_TYPE_PRIMARY, buf, NULL, count, false);
done:
	return ret;
}
EXPORT_SYMBOL(connv3_log_read);

/*****************************************************************************
 * FUNCTION
 *  connsys_dedicated_log_set_log_mode
 * DESCRIPTION
 *  set log mode.
 * PARAMETERS
 *  mode            [IN]        log mode
 * RETURNS
 *  void
 *****************************************************************************/
void connv3_log_set_log_mode(int mode)
{
	atomic_set(&g_connv3_log_mode, (mode > 0 ? LOG_TO_FILE : PRINT_TO_KERNEL_LOG));
}
EXPORT_SYMBOL(connv3_log_set_log_mode);

/*****************************************************************************
* FUNCTION
*  connsys_dedicated_log_get_log_mode
* DESCRIPTION
*  get log mode.
* PARAMETERS
*  void
* RETURNS
* int    log mode
*****************************************************************************/
int connv3_log_get_log_mode(void)
{
	return atomic_read(&g_connv3_log_mode);
}
EXPORT_SYMBOL(connv3_log_get_log_mode);

u32 connv3_log_handler(int conn_type, enum connv3_log_type type, u8 *buf, u32 size)
{
	struct connv3_log_dev* handler;
	struct connv3_log_ctrl_block *block;
	struct ring_segment ring_cache_seg;
	u32 written = 0;
	static DEFINE_RATELIMIT_STATE(_rs, 10 * HZ, 1);
	static unsigned int read_times[CONNV3_DEBUG_TYPE_SIZE][CONNV3_LOG_TYPE_SIZE];
	static size_t read_cnt[CONNV3_DEBUG_TYPE_SIZE][CONNV3_LOG_TYPE_SIZE];

	ratelimit_set_flags(&_rs, RATELIMIT_MSG_ON_RELEASE);

	if (conn_type < 0 || conn_type >= CONNV3_DEBUG_TYPE_SIZE)
		return 0;

	handler = g_connv3_log_dev[conn_type];
	if (handler == NULL) {
		pr_err("[%s][%s] didn't init\n", __func__, type_to_title[conn_type]);
		return 0;
	}

	block = &handler->ctrl_block[type];

	RING_WRITE_FOR_EACH(size, ring_cache_seg, &block->log_buffer.ring_cache) {
		memcpy(ring_cache_seg.ring_pt, buf + written,
				ring_cache_seg.sz);
		size -= ring_cache_seg.sz;
		written += ring_cache_seg.sz;
	}

	read_times[conn_type][type]++;
	read_cnt[conn_type][type] += written;
	if (__ratelimit(&_rs)) {
		pr_info("[%s] type=[%d] blcok=[%d] read %d times, %d bytes",
			__func__, conn_type, type,
			read_times[conn_type][type],
			read_cnt[conn_type][type]);
		read_times[conn_type][type] = 0;
		read_cnt[conn_type][type] = 0;
	}

	connv3_log_event_set(block);
	return written;
}
EXPORT_SYMBOL(connv3_log_handler);

/*****************************************************************************
* FUNCTION
*  connv3_log_register_event_cb
* DESCRIPTION
*·
* PARAMETERS
*  void
* RETURNS
* 
*****************************************************************************/
int _connv3_log_register_event_cb(int conn_type, int block_type, void (*log_event_cb)(void))
{
	struct connv3_log_dev* handler;
	if (conn_type < CONNV3_DEBUG_TYPE_WIFI || conn_type >= CONNV3_DEBUG_TYPE_SIZE) {
		pr_warn("[%s] conn_type=[%d]", __func__, conn_type);
		return -1;
	}
	if (block_type < CONNV3_LOG_TYPE_PRIMARY || block_type >= CONNV3_LOG_TYPE_SIZE) {
		pr_warn("[%s] block_type=[%d]", __func__, block_type);
		return -1;
	}

	handler = g_connv3_log_dev[conn_type];
	if (handler == NULL) {
		pr_err("[%s][%s] didn't init\n", __func__, type_to_title[conn_type]);
		return -1;
	}

	pr_info("[%s] type=[%d] block=[%d] ", __func__, conn_type, block_type);
	handler->ctrl_block[block_type].log_data_handler = log_event_cb;
	return 0;
}

int connv3_log_register_event_cb(int conn_type, void (*log_event_cb)(void))
{
	return _connv3_log_register_event_cb(conn_type, CONNV3_LOG_TYPE_PRIMARY, log_event_cb);
}
EXPORT_SYMBOL(connv3_log_register_event_cb);


int connv3_mcu_log_register_event_cb(int conn_type, void (*log_event_cb)(void))
{
	return _connv3_log_register_event_cb(conn_type, CONNV3_LOG_TYPE_MCU, log_event_cb);
}

/*****************************************************************************
* FUNCTION
*  connv3_log_subsys_init
* DESCRIPTION
* 
* PARAMETERS
*  conn_type	[IN]	subsys type
*  emi_addr	[IN]	physical emi
*  emi_size	[IN]	emi size
* RETURNS
*  struct connv3_log_dev* the handler 
*****************************************************************************/
static struct connv3_log_dev* connv3_log_subsys_init(
	int conn_type, u32 primary_buf_size, u32 mcu_buf_size)
{
	struct connv3_log_dev* handler = 0;

	if (conn_type < 0 || conn_type >= CONNV3_DEBUG_TYPE_SIZE)
		return 0;

	handler = (struct connv3_log_dev*)kzalloc(sizeof(struct connv3_log_dev), GFP_KERNEL);
	if (!handler)
		return 0;

	handler->conn_type = conn_type;

	if (connlog_ring_buffer_init(handler, primary_buf_size, mcu_buf_size)) {
		pr_err("[%s] Ring buffer init failed\n", type_to_title[conn_type]);
		goto error_exit;
	}

	return handler;

error_exit:
	if (handler)
		connv3_log_subsys_deinit(handler);
	return 0;

}

/*****************************************************************************
* FUNCTION
*  connsys_log_init
* DESCRIPTION
*
* PARAMETERS
*  void
* RETURNS
*  int
*****************************************************************************/
int connv3_log_init(int conn_type, int primary_size, int mcu_size, void (*log_event_cb)(void))
{
	struct connv3_log_dev* handler;
	int ret = 0;

	if (conn_type < CONNV3_DEBUG_TYPE_WIFI || conn_type >= CONNV3_DEBUG_TYPE_SIZE) {
		pr_err("[%s] invalid type:%d\n", __func__, conn_type);
		return -1;
	}
	if (g_connv3_log_dev[conn_type] != NULL) {
		pr_err("[%s][%s] double init.\n", __func__, type_to_title[conn_type]);
		return 0;
	}

	pr_info("[%s] type=[%d]", __func__, conn_type);
	handler = connv3_log_subsys_init(conn_type, primary_size, mcu_size);
	if (!handler) {
		pr_notice("[%s] handler == NULL\n", __func__);
		return -1;
	}

	handler->ctrl_block[CONNV3_LOG_TYPE_PRIMARY].log_data_handler = log_event_cb;
	g_connv3_log_dev[conn_type] = handler;

	if (mcu_size > 0) {
		struct connlog_to_user_cb user_cb = {
			.conn_type_id = conn_type,
			.register_evt_cb = connv3_mcu_log_register_event_cb,
			.read_to_user_cb = connv3_mcu_log_read_to_user,
			.get_buf_size = connv3_log_get_mcu_buf_size,
		};

		ret = connlog_to_user_register(conn_type, &user_cb);
		if (ret)
			pr_err("[%s] register fail", __func__);
	}

	return 0;
}
EXPORT_SYMBOL(connv3_log_init);

/*****************************************************************************
* Function
*  connv3_log_subsys_deinit
* DESCRIPTION
*
* PARAMETERS
*
* RETURNS
*
*****************************************************************************/
static void connv3_log_subsys_deinit(struct connv3_log_dev* handler)
{
	if (handler == NULL)
		return;

	if (handler->ctrl_block[CONNV3_LOG_TYPE_MCU].enable)
		connlog_to_user_unregister(handler->conn_type);

	connv3_log_ring_buffer_deinit(handler);
	kfree(handler);
}

/*****************************************************************************
* Function
*  connv3_log_deinit
* DESCRIPTION
*
* PARAMETERS
*
* RETURNS
*
*****************************************************************************/
int connv3_log_deinit(int conn_type)
{
	struct connv3_log_dev* handler;

	if (conn_type < 0 || conn_type >= CONNV3_DEBUG_TYPE_SIZE)
		return -1;

	handler = g_connv3_log_dev[conn_type];
	if (handler == NULL) {
		pr_err("[%s][%s] didn't init\n", __func__, type_to_title[conn_type]);
		return -1;
	}

	connv3_log_subsys_deinit(g_connv3_log_dev[conn_type]);
	g_connv3_log_dev[conn_type] = NULL;
	return 0;
}
EXPORT_SYMBOL(connv3_log_deinit);


