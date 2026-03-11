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

#include "osal.h"
#include "connsyslog.h"
#include "connsyslog_emi.h"
#include "ring.h"
#include "ring_emi.h"
#include "fw_log_wifi_mcu.h"
#include "fw_log_bt_mcu.h"

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

struct connlog_alarm {
	struct alarm alarm_timer;
	unsigned int alarm_state;
	unsigned int blank_state;
	unsigned int alarm_sec;
	spinlock_t alarm_lock;
	unsigned long flags;
};

struct connlog_offset {
	unsigned int emi_base_offset;
	unsigned int emi_size;
	unsigned int emi_read;
	unsigned int emi_write;
	unsigned int emi_buf;
	unsigned int emi_guard_pattern_offset;
	unsigned int emi_idx;
};

struct connlog_buffer {
	struct ring_emi ring_emi;
	struct ring ring_cache;
	void *cache_base;
};

struct connlog_event_cb {
       CONNLOG_EVENT_CB log_data_handler;
};

struct connlog_dev {
	int conn_type;
	phys_addr_t phyAddrEmiBase;
	unsigned int emi_size;
	void __iomem *virAddrEmiLogBase;
	struct connlog_offset log_offset;
	struct connlog_buffer log_buffer;
	bool eirqOn;
	spinlock_t irq_lock;
	unsigned long flags;
	unsigned int irq_counter;
	OSAL_TIMER workTimer;
	struct work_struct logDataWorker;
	void *log_data;
	char log_line[LOG_MAX_LEN];
	struct connlog_event_cb callback;
	unsigned int block_num;
	unsigned int block_type[CONN_EMI_BLOCK_TYPE_END];
};

static char *type_to_title[CONN_DEBUG_TYPE_END] = {
	"wifi_fw", "bt_fw", "wifi_mcu", "bt_mcu"
};

static struct connlog_dev* gLogDev[CONN_DEBUG_TYPE_END];

#ifdef CONFIG_MTK_CONNSYS_DEDICATED_LOG_PATH
static atomic_t g_log_mode = ATOMIC_INIT(LOG_TO_FILE);
#else
static atomic_t g_log_mode = ATOMIC_INIT(PRINT_TO_KERNEL_LOG);
#endif

static phys_addr_t gPhyEmiBase;

/* alarm timer for suspend */
struct connlog_alarm gLogAlarm;

const struct connlog_emi_config* g_connsyslog_config = NULL;

static struct connlog_offset emi_offset_table[CONN_DEBUG_TYPE_END];
static bool is_mcu_block_existed = false;

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
static void connlog_do_schedule_work(struct connlog_dev* handler, bool count);
static void work_timer_handler(timer_handler_arg data);
static void connlog_event_set(struct connlog_dev* handler);
static struct connlog_dev* connlog_subsys_init(
	int conn_type,
	phys_addr_t emiaddr,
	unsigned int emi_size);
static void connlog_subsys_deinit(struct connlog_dev* handler);
static ssize_t connlog_read_internal(
	struct connlog_dev* handler, int conn_type,
	char *buf, char __user *userbuf, size_t count, bool to_user);
static void connlog_dump_emi(struct connlog_dev* handler, int offset, int size);

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

const struct connlog_emi_config* get_connsyslog_platform_config(int conn_type)
{
	if (conn_type < 0 || conn_type >= CONN_DEBUG_TYPE_END) {
		pr_err("Incorrect type: %d\n", conn_type);
		return NULL;
	}
	return &g_connsyslog_config[conn_type];
}

void *connlog_cache_allocate(size_t size)
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

void connlog_cache_free(const void *dst)
{
	kvfree(dst);
}

/*****************************************************************************
 * FUNCTION
 *  connlog_event_set
 * DESCRIPTION
 *  Trigger  event call back to wakeup waitqueue
 * PARAMETERS
 *  conn_type      [IN]        subsys type
 * RETURNS
 *  void
 *****************************************************************************/
static void connlog_event_set(struct connlog_dev* handler)
{
	if (handler->callback.log_data_handler)
		handler->callback.log_data_handler();
}


/*****************************************************************************
* FUNCTION
*  connlog_set_ring_ready
* DESCRIPTION
*  set reserved bit be EMIFWLOG to indicate that init is ready.
* PARAMETERS
*  void
* RETURNS
*  void
*****************************************************************************/
static void connlog_set_ring_ready(struct connlog_dev* handler)
{
	const char ready_str[] = "EMIFWLOG";

	memcpy_toio(handler->virAddrEmiLogBase + CONNLOG_READY_PATTERN_BASE,
			ready_str, CONNLOG_READY_PATTERN_BASE_SIZE);
}

static unsigned int connlog_cal_log_size(unsigned int emi_size)
{
	int position;
	int i;

	if (emi_size > 0) {
		for (i = (emi_size >> 1), position = 0; i != 0; ++position)
			i >>= 1;
	} else {
		return 0;
	}

	return (1UL << position);
}

static int connlog_emi_init(struct connlog_dev* handler, phys_addr_t emiaddr, unsigned int emi_size)
{
	int conn_type = handler->conn_type;

	if (conn_type < 0 || conn_type >= CONN_DEBUG_TYPE_END) {
		pr_notice("%s conn_type %d is invalid\n", __func__, conn_type);
		return -1;
	}

	if (emiaddr == 0) {
		pr_notice("[%s] consys emi memory address invalid emi_addr=%llx emi_size=%d\n",
			type_to_title[conn_type], emiaddr, emi_size);
		return -1;
	}

	handler->phyAddrEmiBase = emiaddr;
	handler->emi_size = emi_size;

	if (conn_type == CONN_DEBUG_TYPE_BT_MCU) {
		handler->virAddrEmiLogBase = gLogDev[CONN_DEBUG_TYPE_BT]->virAddrEmiLogBase;
	} else if (conn_type == CONN_DEBUG_TYPE_WIFI_MCU) {
		handler->virAddrEmiLogBase = gLogDev[CONN_DEBUG_TYPE_WIFI]->virAddrEmiLogBase;
	} else {
		handler->virAddrEmiLogBase = ioremap(handler->phyAddrEmiBase, emi_size);
	}

	handler->log_offset.emi_base_offset = emi_offset_table[conn_type].emi_base_offset; // start of each subheader
	handler->log_offset.emi_size = emi_offset_table[conn_type].emi_size;
	handler->log_offset.emi_read = emi_offset_table[conn_type].emi_read;
	handler->log_offset.emi_write = emi_offset_table[conn_type].emi_write;
	handler->log_offset.emi_buf = emi_offset_table[conn_type].emi_buf;
	handler->log_offset.emi_guard_pattern_offset =
		emi_offset_table[conn_type].emi_guard_pattern_offset;
	handler->log_offset.emi_idx = emi_offset_table[conn_type].emi_idx;

	if (handler->virAddrEmiLogBase) {
		pr_info("[%s] EMI mapping OK virtual(0x%p) physical(0x%x) size=%d\n",
			type_to_title[conn_type],
			handler->virAddrEmiLogBase,
			(unsigned int)handler->phyAddrEmiBase,
			handler->emi_size);

		if (conn_type == CONN_DEBUG_TYPE_WIFI || conn_type == CONN_DEBUG_TYPE_BT) {
			memset_io(handler->virAddrEmiLogBase, 0xff, handler->emi_size);
			/* Set state to resume initially */
			EMI_WRITE32(handler->virAddrEmiLogBase + 32, 1);
		}

		/* Clean control block as 0; subheader */
		memset_io(handler->virAddrEmiLogBase + handler->log_offset.emi_read, 0x0, CONNLOG_EMI_32_BYTE_ALIGNED);
		/* Setup header */
		EMI_WRITE32(handler->virAddrEmiLogBase + (handler->log_offset.emi_idx * 8), handler->log_offset.emi_base_offset);
		EMI_WRITE32(handler->virAddrEmiLogBase + (handler->log_offset.emi_idx * 8) + 4, handler->log_offset.emi_size);

		/* Setup end pattern */
		memcpy_toio(handler->virAddrEmiLogBase + handler->log_offset.emi_guard_pattern_offset,
			CONNLOG_EMI_END_PATTERN, CONNLOG_EMI_END_PATTERN_SIZE);
	} else {
		pr_err("[%s] EMI mapping fail\n", type_to_title[conn_type]);
		return -1;
	}

	return 0;
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
static void connlog_emi_deinit(struct connlog_dev* handler)
{
	if (handler->virAddrEmiLogBase)
		iounmap(handler->virAddrEmiLogBase);
}

static int connlog_buffer_init(struct connlog_dev* handler)
{
	void *pBuffer = NULL;
	unsigned int cache_size = 0;

	/* Init ring emi */
	ring_emi_init(
		handler->virAddrEmiLogBase + handler->log_offset.emi_buf,
		handler->log_offset.emi_size,
		handler->virAddrEmiLogBase + handler->log_offset.emi_read,
		handler->virAddrEmiLogBase + handler->log_offset.emi_write,
		&handler->log_buffer.ring_emi);

	/* init ring cache */
	/* TODO: use emi size. Need confirm */
	cache_size = handler->log_offset.emi_size * 2;
	pBuffer = connlog_cache_allocate(cache_size);

	if (pBuffer == NULL) {
		pr_info("[%s] allocate cache fail.", __func__);
		return -ENOMEM;
	}

	handler->log_buffer.cache_base = pBuffer;
	memset(handler->log_buffer.cache_base, 0, cache_size);
	ring_init(
		handler->log_buffer.cache_base,
		cache_size,
		0,
		0,
		&handler->log_buffer.ring_cache);

	return 0;
}

static int connlog_ring_buffer_init(struct connlog_dev* handler)
{
	void *pBuffer = NULL;
	unsigned int cache_size = 0;

	if (handler->conn_type < 0 || handler->conn_type >= CONN_DEBUG_TYPE_END) {
		pr_notice("%s invalid conn_type %d\n", __func__, handler->conn_type);
		return -1;
	}

	if (!handler->virAddrEmiLogBase) {
		pr_err("[%s] consys emi memory address phyAddrEmiBase invalid\n",
			type_to_title[handler->conn_type]);
		return -1;
	}
	connlog_buffer_init(handler);
	/* TODO: use emi size. Need confirm */
	cache_size = handler->log_offset.emi_size * 2;
	pBuffer = connlog_cache_allocate(cache_size);

	if (pBuffer == NULL) {
		pr_notice("[%s] allocate ring buffer fail\n", __func__);
		return -ENOMEM;
	}
	handler->log_data = pBuffer;

	if (is_mcu_block_existed) {
		// Make sure mcu ring buffer also init before
		// setting ring ready flag in emi header
		if (handler->conn_type >= CONN_DEBUG_TYPE_WIFI_MCU)
			connlog_set_ring_ready(handler);
	} else {
		connlog_set_ring_ready(handler);
	}

	return 0;
}

/*****************************************************************************
* FUNCTION
*  connlog_ring_buffer_deinit
* DESCRIPTION
*  Initialize ring buffer setting for subsys
* PARAMETERS
*  void
* RETURNS
*  void
*****************************************************************************/
static void connlog_ring_buffer_deinit(struct connlog_dev* handler)
{
	if (handler->log_buffer.cache_base) {
		connlog_cache_free(handler->log_buffer.cache_base);
		handler->log_buffer.cache_base = NULL;
	}

	if (handler->log_data) {
		connlog_cache_free(handler->log_data);
		handler->log_data = NULL;
	}
}

/*****************************************************************************
* FUNCTION
*  work_timer_handler
* DESCRIPTION
*  IRQ is still on, do schedule_work again
* PARAMETERS
*  data      [IN]        input data
* RETURNS
*  void
*****************************************************************************/
static void work_timer_handler(timer_handler_arg arg)
{
	unsigned long data;
	struct connlog_dev* handler;

	GET_HANDLER_DATA(arg, data);
	handler = (struct connlog_dev*)data;
	connlog_do_schedule_work(handler, false);
}

/*****************************************************************************
* FUNCTION
*  connlog_dump_buf
* DESCRIPTION
*  Dump EMI content. Output format:
* xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx ................
* 3 digits hex * 16 + 16 single char + 1 NULL terminate = 64+1 bytes
* PARAMETERS
*
* RETURNS
*  void
*****************************************************************************/
void connsys_log_dump_buf(const char *title, const char *buf, ssize_t sz)
{
	int i;
	char line[LOG_LINE_SIZE];

	i = 0;
	line[LOG_LINE_SIZE-1] = 0;
	while (sz--) {
		if (snprintf(line + i*3, 3, "%02x", *buf) < 0) {
			pr_notice("%s snprint failed\n", __func__);
			return;
		}
		line[i*3 + 2] = ' ';

		if (IS_VISIBLE_CHAR(*buf))
			line[3*BYETES_PER_LINE + i] = *buf;
		else
			line[3*BYETES_PER_LINE + i] = '`';

		i++;
		buf++;

		if (i >= BYETES_PER_LINE || !sz) {
			if (i < BYETES_PER_LINE) {
				memset(line+i*3, ' ', (BYETES_PER_LINE-i)*3);
				memset(line+3*BYETES_PER_LINE+i, '.', BYETES_PER_LINE-i);
			}
			pr_info("%s: %s\n", title, line);
			i = 0;
		}
	}
}
EXPORT_SYMBOL(connsys_log_dump_buf);

/*****************************************************************************
* FUNCTION
*  connlog_dump_emi
* DESCRIPTION
*  dump EMI buffer for debug.
* PARAMETERS
*  offset          [IN]        buffer offset
*  size            [IN]        dump buffer size
* RETURNS
*  void
*****************************************************************************/
void connlog_dump_emi(struct connlog_dev* handler, int offset, int size)
{
	char title[100];
	memset(title, 0, 100);
	if (sprintf(title, "%s(%p)", "emi", handler->virAddrEmiLogBase + offset) < 0)
		pr_notice("%s snprintf failed\n", __func__);
	connsys_log_dump_buf(title, handler->virAddrEmiLogBase + offset, size);
}

/*****************************************************************************
* FUNCTION
*  connlog_ring_emi_check
* DESCRIPTION
*
* PARAMETERS
*
* RETURNS
*  void
*****************************************************************************/
static bool connlog_ring_emi_check(struct connlog_dev* handler)
{
	struct ring_emi *ring_emi = &handler->log_buffer.ring_emi;
	char line[CONNLOG_EMI_END_PATTERN_SIZE + 1];

	if (handler->conn_type < 0 || handler->conn_type >= CONN_DEBUG_TYPE_END) {
		pr_notice("%s conn_type %d is invalid\n", __func__, handler->conn_type);
		return false;
	}

	memcpy_fromio(
		line,
		handler->virAddrEmiLogBase + handler->log_offset.emi_guard_pattern_offset,
		CONNLOG_EMI_END_PATTERN_SIZE);
	line[CONNLOG_EMI_END_PATTERN_SIZE] = '\0';

	/* Check ring_emi buffer memory. Dump EMI data if it is corruption. */
	if (EMI_READ32(ring_emi->read) > handler->log_offset.emi_size ||
		EMI_READ32(ring_emi->write) > handler->log_offset.emi_size ||
		strncmp(line, CONNLOG_EMI_END_PATTERN, CONNLOG_EMI_END_PATTERN_SIZE) != 0) {
		pr_err("[connlog] %s out of bound or guard pattern overwrited. Read(pos=%p)=[0x%x] write(pos=%p)=[0x%x] size=[0x%x]\n",
			type_to_title[handler->conn_type],
			ring_emi->read, EMI_READ32(ring_emi->read),
			ring_emi->write, EMI_READ32(ring_emi->write),
			handler->log_offset.emi_size);
		connlog_dump_emi(handler, 0x0, 0x60);
		connlog_dump_emi(handler, CONNLOG_EMI_BASE_OFFSET, 0x20);
		connlog_dump_emi(
			handler,
			handler->log_offset.emi_guard_pattern_offset,
			CONNLOG_EMI_END_PATTERN_SIZE);
		return false;
	}

	return true;
}

/*****************************************************************************
* FUNCTION
*  connlog_ring_emi_to_cache
* DESCRIPTION
*
* PARAMETERS
*
* RETURNS
*  void
*****************************************************************************/
static void connlog_ring_emi_to_cache(struct connlog_dev* handler)
{
	struct ring_emi_segment ring_emi_seg;
	struct ring_emi *ring_emi = &handler->log_buffer.ring_emi;
	struct ring *ring_cache = &handler->log_buffer.ring_cache;
	int total_size = 0;
	int count = 0;
	unsigned int cache_max_size = 0;
#ifndef DEBUG_LOG_ON
	static DEFINE_RATELIMIT_STATE(_rs, 10 * HZ, 1);

	ratelimit_set_flags(&_rs, RATELIMIT_MSG_ON_RELEASE);
#endif
	if (handler->conn_type < 0 || handler->conn_type >= CONN_DEBUG_TYPE_END) {
		pr_notice("%s conn_type %d is invalid\n", __func__, handler->conn_type);
		return;
	}

	if (RING_FULL(ring_cache)) {
	#ifndef DEBUG_LOG_ON
		if (__ratelimit(&_rs))
	#endif
			pr_warn("[connlog] %s cache is full.\n", type_to_title[handler->conn_type]);
		return;
	}

	cache_max_size = RING_WRITE_REMAIN_SIZE(ring_cache);
	if (RING_EMI_EMPTY(ring_emi) || !ring_emi_read_prepare(cache_max_size, &ring_emi_seg, ring_emi)) {
	#ifndef DEBUG_LOG_ON
		if(__ratelimit(&_rs))
	#endif
			pr_err("[connlog] %s no data.\n", type_to_title[handler->conn_type]);
		return;
	}

	/* Check ring_emi buffer memory. Dump EMI data if it is corruption. */
	if (connlog_ring_emi_check(handler) == false) {
		pr_err("[connlog] %s emi check fail\n", type_to_title[handler->conn_type]);
		/* TODO: trigger assert by callback? */
		return;
	}

	RING_EMI_READ_ALL_FOR_EACH(ring_emi_seg, ring_emi) {
		struct ring_segment ring_cache_seg;
		unsigned int emi_buf_size = ring_emi_seg.sz;
		unsigned int written = 0;

#ifdef DEBUG_RING
		ring_emi_dump(__func__, ring_emi);
		ring_emi_dump_segment(__func__, &ring_emi_seg);
#endif
		RING_WRITE_FOR_EACH(ring_emi_seg.sz, ring_cache_seg, &handler->log_buffer.ring_cache) {
#ifdef DEBUG_RING
			ring_dump(__func__, &handler->log_buffer.ring_cache);
			ring_dump_segment(__func__, &ring_cache_seg);
#endif
			memcpy_fromio(ring_cache_seg.ring_pt, ring_emi_seg.ring_emi_pt + ring_cache_seg.data_pos,
				ring_cache_seg.sz);
			emi_buf_size -= ring_cache_seg.sz;
			written += ring_cache_seg.sz;
		}

		total_size += ring_emi_seg.sz;
		count++;
	}
}


/*****************************************************************************
 * FUNCTION
 *  connlog_fw_log_parser
 * DESCRIPTION
 *  Parse fw log and print to kernel
 * PARAMETERS
 *  conn_type      [IN]        log type
 *  buf            [IN]        buffer to prase
 *  sz             [IN]        buffer size
 * RETURNS
 *  void
 *****************************************************************************/
static void connlog_fw_log_parser(struct connlog_dev* handler, ssize_t sz)
{
	unsigned int systime = 0;
	unsigned int utc_s = 0;
	unsigned int utc_us = 0;
	unsigned int buf_len = 0;
	unsigned int print_len = 0;
	char* log_line = handler->log_line;
	const char* buf = handler->log_data;
	int conn_type = handler->conn_type;

	if (conn_type < 0 || conn_type >= CONN_DEBUG_TYPE_END) {
		pr_notice("%s conn_type %d is invalid\n", __func__, conn_type);
		return;
	}

	while (sz > LOG_HEAD_LENG) {
		if (*buf == log_head[0]) {
			if (!memcmp(buf, log_head, sizeof(log_head))) {
				buf_len = buf[14] + (buf[15] << 8);
				print_len = buf_len >= LOG_MAX_LEN ? LOG_MAX_LEN - 1 : buf_len;
				memcpy(log_line, buf + LOG_HEAD_LENG, print_len);
				log_line[print_len] = 0;
				pr_info("%s: %s\n", type_to_title[conn_type], log_line);
				sz -= (LOG_HEAD_LENG + buf_len);
				buf += (LOG_HEAD_LENG + buf_len);
				continue;
			} else if (sz >= TIMESYNC_LENG &&
				!memcmp(buf, timesync_head, sizeof(timesync_head))) {
				memcpy(&systime, buf + 28, sizeof(systime));
				memcpy(&utc_s, buf + 32, sizeof(utc_s));
				memcpy(&utc_us, buf + 36, sizeof(utc_us));
				pr_info("%s: timesync :  (%u) %u.%06u\n",
					type_to_title[conn_type], systime, utc_s, utc_us);
				sz -= TIMESYNC_LENG;
				buf += TIMESYNC_LENG;
				continue;
			}
		}
		sz--;
		buf++;
	}
}

/*****************************************************************************
 * FUNCTION
 *  connlog_ring_print
 * DESCRIPTION
 *  print log data on kernel log
 * PARAMETERS
 *  handler      [IN]        log handler
 * RETURNS
 *  void
 *****************************************************************************/
static void connlog_ring_print(struct connlog_dev* handler)
{
	unsigned int written = 0;
	unsigned int buf_size;
	struct ring_emi_segment ring_emi_seg;
	struct ring_emi *ring_emi = &handler->log_buffer.ring_emi;
	int conn_type = handler->conn_type;

	if (conn_type < 0 || conn_type >= CONN_DEBUG_TYPE_END) {
		pr_notice("%s conn_type %d is invalid\n", __func__, conn_type);
		return;
	}

	if (RING_EMI_EMPTY(ring_emi) || !ring_emi_read_all_prepare(&ring_emi_seg, ring_emi)) {
		pr_err("type(%s) no data, possibly taken by concurrent reader.\n", type_to_title[conn_type]);
		return;
	}
	buf_size = ring_emi_seg.remain;
	memset(handler->log_data, 0, handler->emi_size);

	/* Check ring_emi buffer memory. Dump EMI data if it is corruption. */
	if (connlog_ring_emi_check(handler) == false) {
		pr_err("[connlog] %s emi check fail\n", type_to_title[handler->conn_type]);
		/* TODO: trigger assert by callback? */
		return;
	}

	RING_EMI_READ_ALL_FOR_EACH(ring_emi_seg, ring_emi) {
		memcpy_fromio(handler->log_data + written, ring_emi_seg.ring_emi_pt, ring_emi_seg.sz);
		buf_size -= ring_emi_seg.sz;
		written += ring_emi_seg.sz;
	}

	if (conn_type != CONN_DEBUG_TYPE_BT)
		connlog_fw_log_parser(handler, written);
}

/*****************************************************************************
* FUNCTION
*  connlog_log_data_handler
* DESCRIPTION
*
* PARAMETERS
*
* RETURNS
*  void
*****************************************************************************/
static void connlog_log_data_handler(struct work_struct *work)
{
	unsigned int i = 0;
	unsigned int conn_type_block;
	struct connlog_dev* handler =
		container_of(work, struct connlog_dev, logDataWorker);
#ifndef DEBUG_LOG_ON
	static DEFINE_RATELIMIT_STATE(_rs, 10 * HZ, 1);
	static DEFINE_RATELIMIT_STATE(_rs2, 2 * HZ, 1);

	ratelimit_set_flags(&_rs, RATELIMIT_MSG_ON_RELEASE);
	ratelimit_set_flags(&_rs2, RATELIMIT_MSG_ON_RELEASE);
#endif

	if (handler == NULL) {
		pr_notice("%s handler is NULL\n", __func__);
		return;
	}

	if (handler->conn_type < 0 || handler->conn_type >= CONN_DEBUG_TYPE_END) {
		pr_notice("%s handler->conn_type %d is invalid\n", __func__, handler->conn_type);
		return;
	}

	for (i = 0; i < handler->block_num; i++) {
		conn_type_block = handler->block_type[i];
		if (conn_type_block >= CONN_DEBUG_TYPE_END) {
			pr_notice("%s conn_type %d is invalid\n", __func__, conn_type_block);
			return;
		}

		if (!RING_EMI_EMPTY(&gLogDev[conn_type_block]->log_buffer.ring_emi)) {
			if (atomic_read(&g_log_mode) == LOG_TO_FILE)
				connlog_ring_emi_to_cache(gLogDev[conn_type_block]);
			else
				connlog_ring_print(gLogDev[conn_type_block]);
			connlog_event_set(gLogDev[conn_type_block]);
		} else {
	#ifndef DEBUG_LOG_ON
			if (__ratelimit(&_rs))
	#endif
				pr_info("[connlog] %s emi ring is empty!\n",
					type_to_title[conn_type_block]);
		}
	}

#ifndef DEBUG_LOG_ON
	if (__ratelimit(&_rs2))
#endif
		pr_info("[connlog] %s irq counter = %d\n",
			type_to_title[handler->conn_type],
			EMI_READ32(handler->virAddrEmiLogBase + CONNLOG_IRQ_COUNTER_BASE));

	spin_lock_irqsave(&handler->irq_lock, handler->flags);
	if (handler->eirqOn)
		osal_timer_modify(&handler->workTimer, 1);
	spin_unlock_irqrestore(&handler->irq_lock, handler->flags);
}

static void connlog_do_schedule_work(struct connlog_dev* handler, bool count)
{
	spin_lock_irqsave(&handler->irq_lock, handler->flags);
	if (count) {
		handler->irq_counter++;
		EMI_WRITE32(
			handler->virAddrEmiLogBase + CONNLOG_IRQ_COUNTER_BASE,
			handler->irq_counter);
	}
	handler->eirqOn = !schedule_work(&handler->logDataWorker);
	spin_unlock_irqrestore(&handler->irq_lock, handler->flags);
}

/*****************************************************************************
* FUNCTION
*  connsys_log_get_buf_size
* DESCRIPTION
*  Get ring buffer unread size on EMI.
* PARAMETERS
*  conn_type      [IN]        subsys type
* RETURNS
*  unsigned int    Ring buffer unread size
*****************************************************************************/
unsigned int connsys_log_get_buf_size(int conn_type)
{
	struct connlog_dev* handler;

	if (conn_type < CONN_DEBUG_TYPE_WIFI || conn_type >= CONN_DEBUG_TYPE_END)
		return 0;

	handler = gLogDev[conn_type];
	if (handler == NULL) {
		pr_err("[%s][%s] didn't init\n", __func__, type_to_title[conn_type]);
		return 0;
	}

	return RING_SIZE(&handler->log_buffer.ring_cache);
}
EXPORT_SYMBOL(connsys_log_get_buf_size);

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
static ssize_t connlog_read_internal(
	struct connlog_dev* handler, int conn_type,
	char *buf, char __user *userbuf, size_t count, bool to_user)
{
	unsigned int written = 0;
	unsigned int cache_buf_size;
	struct ring_segment ring_seg;
	struct ring *ring = &handler->log_buffer.ring_cache;
	unsigned int size = 0;
	int retval;
#ifndef DEBUG_LOG_ON
	static DEFINE_RATELIMIT_STATE(_rs, 10 * HZ, 1);

	ratelimit_set_flags(&_rs, RATELIMIT_MSG_ON_RELEASE);
#endif

	if (conn_type < 0 || conn_type >= CONN_DEBUG_TYPE_END) {
		pr_notice("%s conn_type %d is invalid\n", __func__, conn_type);
		return 0;
	}

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
			memcpy(buf + written, ring_seg.ring_pt, ring_seg.sz);
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
ssize_t connsys_log_read_to_user(int conn_type, char __user *buf, size_t count)
{
	struct connlog_dev* handler;
	unsigned int written = 0;

	if (conn_type < CONN_DEBUG_TYPE_WIFI || conn_type >= CONN_DEBUG_TYPE_END)
		goto done;
	if (atomic_read(&g_log_mode) != LOG_TO_FILE)
		goto done;

	handler = gLogDev[conn_type];
	if (handler == NULL) {
		pr_err("[%s][%s] not init\n", __func__, type_to_title[conn_type]);
		goto done;
	}
	written = connlog_read_internal(handler, conn_type, NULL, buf, count, true);
done:
	return written;
}
EXPORT_SYMBOL(connsys_log_read_to_user);

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
ssize_t connsys_log_read(int conn_type, char *buf, size_t count)
{
	unsigned int ret = 0;
	struct connlog_dev* handler;

	if (conn_type < CONN_DEBUG_TYPE_WIFI || conn_type >= CONN_DEBUG_TYPE_END)
		goto done;
	if (atomic_read(&g_log_mode) != LOG_TO_FILE)
		goto done;

	handler = gLogDev[conn_type];
	ret = connlog_read_internal(handler, conn_type, buf, NULL, count, false);
done:
	return ret;
}
EXPORT_SYMBOL(connsys_log_read);


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
void connsys_dedicated_log_set_log_mode(int mode)
{
	atomic_set(&g_log_mode, (mode > 0 ? LOG_TO_FILE : PRINT_TO_KERNEL_LOG));
}
EXPORT_SYMBOL(connsys_dedicated_log_set_log_mode);

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
int connsys_dedicated_log_get_log_mode(void)
{
	return atomic_read(&g_log_mode);
}
EXPORT_SYMBOL(connsys_dedicated_log_get_log_mode);

/*****************************************************************************
* FUNCTION
*  connsys_log_irq_handler
* DESCRIPTION
*
* PARAMETERS
*  void
* RETURNS
*  int
*****************************************************************************/
int connsys_log_irq_handler(int conn_type)
{
	struct connlog_dev* handler;
	if (conn_type < CONN_DEBUG_TYPE_WIFI || conn_type >= CONN_DEBUG_TYPE_END)
		return -1;

	handler = gLogDev[conn_type];
	if (handler == NULL) {
		pr_err("[%s][%s] didn't init\n", __func__, type_to_title[conn_type]);
		return -1;
	}

	connlog_do_schedule_work(handler, true);
	return 0;
}
EXPORT_SYMBOL(connsys_log_irq_handler);

/*****************************************************************************
* FUNCTION
*  connsys_log_register_event_cb
* DESCRIPTION
*·
* PARAMETERS
*  void
* RETURNS
* 
*****************************************************************************/
int connsys_log_register_event_cb(int conn_type, CONNLOG_EVENT_CB func)
{
	struct connlog_dev* handler;
	if (conn_type < CONN_DEBUG_TYPE_WIFI || conn_type >= CONN_DEBUG_TYPE_END)
		return -1;

	handler = gLogDev[conn_type];
	if (handler == NULL) {
		pr_err("[%s][%s] didn't init\n", __func__, type_to_title[conn_type]);
		return -1;
	}

	handler->callback.log_data_handler = func;
	return 0;
}
EXPORT_SYMBOL(connsys_log_register_event_cb);

/*****************************************************************************
* FUNCTION
*  connlog_subsys_init
* DESCRIPTION
* 
* PARAMETERS
*  conn_type	[IN]	subsys type
*  emi_addr	[IN]	physical emi
*  emi_size	[IN]	emi size
* RETURNS
*  struct connlog_dev* the handler 
*****************************************************************************/
static struct connlog_dev* connlog_subsys_init(
	int conn_type,
	phys_addr_t emi_addr,
	unsigned int emi_size)
{
	struct connlog_dev* handler = 0;

	if (conn_type < CONN_DEBUG_TYPE_WIFI || conn_type >= CONN_DEBUG_TYPE_END)
		return 0;

	handler = (struct connlog_dev*)kzalloc(sizeof(struct connlog_dev), GFP_KERNEL);
	if (!handler)
		return 0;

	memset(handler, 0, sizeof(struct connlog_dev));

	handler->conn_type = conn_type;
	if (connlog_emi_init(handler, emi_addr, emi_size)) {
		pr_err("[%s] EMI init failed\n", type_to_title[conn_type]);
		goto error_exit;
	}

	if (connlog_ring_buffer_init(handler)) {
		pr_err("[%s] Ring buffer init failed\n", type_to_title[conn_type]);
		goto error_exit;
	}

	// Only WiFi or BT (primary) handler can receive irq
	if (handler->conn_type >= CONN_DEBUG_PRIMARY_END)
		return handler;

	handler->workTimer.timeroutHandlerData = (unsigned long)handler;
	handler->workTimer.timeoutHandler = work_timer_handler;
	osal_timer_create(&handler->workTimer);
	handler->irq_counter = 0;
	spin_lock_init(&handler->irq_lock);
	INIT_WORK(&handler->logDataWorker, connlog_log_data_handler);

	/* alarm timer */
	return handler;

error_exit:
	if (handler)
		connlog_subsys_deinit(handler);
	return 0;

}

static int construct_emi_offset_table(int conn_type, const struct connlog_emi_config* emi_config)
{
	int i = 0;

#define INIT_EMI_OFFSET_TABLE(index, base, size, read_offset, write_offset, buf_offset, guard_offset, idx) \
	emi_offset_table[index].emi_base_offset = base; \
	emi_offset_table[index].emi_size = size; \
	emi_offset_table[index].emi_read = read_offset; \
	emi_offset_table[index].emi_write = write_offset; \
	emi_offset_table[index].emi_buf = buf_offset; \
	emi_offset_table[index].emi_guard_pattern_offset = guard_offset; \
	emi_offset_table[index].emi_idx = idx;

	if (conn_type < 0 || conn_type >= CONN_DEBUG_PRIMARY_END) {
		pr_notice("%s conn_type %d is invalid\n", __func__, conn_type);
		return -1;
	}

	if (is_mcu_block_existed) {
		unsigned int primary_base = 0;
		unsigned int primary_size = 0;
		unsigned int conn_type_mcu = 0;
		unsigned int mcu_base = 0;
		unsigned int mcu_size = 0;

		primary_base = CONNLOG_EMI_BASE_OFFSET; // after header size
		primary_size = emi_config->block[CONN_EMI_BLOCK_PRIMARY].size;

		conn_type_mcu = emi_config->block[CONN_EMI_BLOCK_MCU].type;
		mcu_base = primary_base + CONNLOG_EMI_SUB_HEADER
						+ emi_config->block[CONN_EMI_BLOCK_PRIMARY].size
						+ CONNLOG_CONTROL_RING_BUFFER_RESERVE_SIZE;
		mcu_size = emi_config->block[CONN_EMI_BLOCK_MCU].size;

		INIT_EMI_OFFSET_TABLE(
			conn_type,
			primary_base,
			primary_size,
			primary_base + 0,
			primary_base + 4,
			primary_base + 32,
			primary_base + CONNLOG_EMI_SUB_HEADER + primary_size,
			CONN_EMI_BLOCK_PRIMARY);

		INIT_EMI_OFFSET_TABLE(
			conn_type_mcu,
			mcu_base,
			mcu_size,
			mcu_base + 0,
			mcu_base + 4,
			mcu_base + 32,
			mcu_base + CONNLOG_EMI_SUB_HEADER + mcu_size,
			CONN_EMI_BLOCK_MCU);
	} else {
		unsigned int cal_log_size = connlog_cal_log_size(
		emi_config->log_size - CONNLOG_EMI_BASE_OFFSET - CONNLOG_EMI_END_PATTERN_SIZE);

		if (cal_log_size == 0) {
			pr_notice("[%s] consys emi memory invalid emi_size=%d\n",
				type_to_title[conn_type], emi_config->log_size);
			return -1;
		}

		INIT_EMI_OFFSET_TABLE(
			conn_type,
			CONNLOG_EMI_BASE_OFFSET,
			cal_log_size,
			CONNLOG_EMI_READ,
			CONNLOG_EMI_WRITE,
			CONNLOG_EMI_BUF,
			CONNLOG_EMI_BUF + cal_log_size,
			0);
	}

	while (i < 2) {
		pr_info("[%s] emi_offset_table[%d]: emi_base=[0x%x], emi_size=[0x%x], emi_read=[0x%x], emi_write=[0x%x], emi_buf=[0x%x], emi_guard=[0x%x], emi_idx=[%d]\n",
		__func__, conn_type + (i * 2),
		emi_offset_table[conn_type + (i * 2)].emi_base_offset,
		emi_offset_table[conn_type + (i * 2)].emi_size,
		emi_offset_table[conn_type + (i * 2)].emi_read,
		emi_offset_table[conn_type + (i * 2)].emi_write,
		emi_offset_table[conn_type + (i * 2)].emi_buf,
		emi_offset_table[conn_type + (i * 2)].emi_guard_pattern_offset,
		emi_offset_table[conn_type + (i * 2)].emi_idx);
		i++;
	}

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
int connsys_log_init(int conn_type)
{
	struct connlog_dev* handler;
	struct connlog_dev* mcu_handler;
	phys_addr_t log_start_addr;
	unsigned int log_size;
	const struct connlog_emi_config* emi_config;
	int ret = 0;

	if (conn_type < CONN_DEBUG_TYPE_WIFI || conn_type >= CONN_DEBUG_PRIMARY_END) {
		pr_err("[%s] invalid type:%d\n", __func__, conn_type);
		return -1;
	}
	if (gLogDev[conn_type] != NULL) {
		pr_err("[%s][%s] double init.\n", __func__, type_to_title[conn_type]);
		return 0;
	}

	emi_config = get_connsyslog_platform_config(conn_type);
	if (!emi_config) {
		pr_err("[%s] get emi config fail.\n", __func__);
		return -1;
	}

	log_start_addr = emi_config->log_offset + gPhyEmiBase;
	log_size = emi_config->log_size;
	pr_info("%s init. Base=%llx size=%d\n",
		type_to_title[conn_type], log_start_addr, log_size);

	// Check if emi layout contains mcu block
	is_mcu_block_existed = (emi_config->block[CONN_EMI_BLOCK_MCU].size != 0) ? true : false;

	// Construct emi offset table
	ret = construct_emi_offset_table(conn_type, emi_config);

	if (ret != 0) {
		return ret;
	}

	handler = connlog_subsys_init(conn_type, log_start_addr, log_size);

	if (handler == NULL) {
		pr_notice("[%s] Fail to construct %s handler\n", __func__, type_to_title[conn_type]);
		return -1;
	}

	if (is_mcu_block_existed) {
		// Contain both itself and mcu block
		handler->block_num = 2;
		handler->block_type[CONN_EMI_BLOCK_PRIMARY] = conn_type;
		handler->block_type[CONN_EMI_BLOCK_MCU]
			= emi_config->block[CONN_EMI_BLOCK_MCU].type;
	} else {
		// Contain no mcu block, only contain itself
		handler->block_num = 1;
		handler->block_type[CONN_EMI_BLOCK_PRIMARY] = conn_type;
	}

	gLogDev[conn_type] = handler;

	if (is_mcu_block_existed) {
		// Construct mcu handler
		unsigned int conn_type_mcu = emi_config->block[CONN_EMI_BLOCK_MCU].type;
		mcu_handler = connlog_subsys_init(conn_type_mcu, log_start_addr, log_size);

		if (mcu_handler == NULL) {
			pr_notice("[%s] Fail to construct %s handler\n", __func__, type_to_title[conn_type_mcu]);
			return -1;
		} else {
			pr_info("[%s] Construct %s handler success\n", __func__, type_to_title[conn_type_mcu]);
		}

		gLogDev[conn_type_mcu] = mcu_handler;

		if (conn_type == CONN_DEBUG_TYPE_WIFI) {
			fw_log_wifi_mcu_register_event_cb();
		} else if (conn_type == CONN_DEBUG_TYPE_BT) {
			fw_log_bt_mcu_register_event_cb();
		}
	}

	return 0;
}
EXPORT_SYMBOL(connsys_log_init);

/*****************************************************************************
* Function
*  connlog_subsys_deinit
* DESCRIPTION
*
* PARAMETERS
*
* RETURNS
*
*****************************************************************************/
static void connlog_subsys_deinit(struct connlog_dev* handler)
{
	if (handler == NULL)
		return;

	connlog_emi_deinit(handler);
	connlog_ring_buffer_deinit(handler);
	kfree(handler);
}

/*****************************************************************************
* Function
*  connsys_log_deinit
* DESCRIPTION
*
* PARAMETERS
*
* RETURNS
*
*****************************************************************************/
int connsys_log_deinit(int conn_type)
{
	struct connlog_dev* handler;
	if (conn_type < CONN_DEBUG_TYPE_WIFI || conn_type >= CONN_DEBUG_TYPE_END)
		return -1;

	handler = gLogDev[conn_type];
	if (handler == NULL) {
		pr_err("[%s][%s] didn't init\n", __func__, type_to_title[conn_type]);
		return -1;
	}

	connlog_subsys_deinit(gLogDev[conn_type]);
	gLogDev[conn_type] = NULL;
	return 0;
}
EXPORT_SYMBOL(connsys_log_deinit);

/*****************************************************************************
* FUNCTION
*  connsys_log_get_utc_time
* DESCRIPTION
*  Return UTC time
* PARAMETERS
*  second         [IN]        UTC seconds
*  usecond        [IN]        UTC usecons
* RETURNS
*  void
*****************************************************************************/
void connsys_log_get_utc_time(
	unsigned int *second, unsigned int *usecond)
{
	struct timespec64 time;

	osal_gettimeofday(&time);
	*second = (unsigned int)time.tv_sec; /* UTC time second unit */
	*usecond = (unsigned int)time.tv_nsec / NSEC_PER_USEC; /* UTC time microsecond unit */
}
EXPORT_SYMBOL(connsys_log_get_utc_time);

static inline bool connlog_is_alarm_enable(void)
{
	if ((gLogAlarm.alarm_state & CONNLOG_ALARM_STATE_ENABLE) > 0)
		return true;
	return false;
}

static int connlog_set_alarm_timer(void)
{
	ktime_t kt;

	kt = ktime_set(gLogAlarm.alarm_sec, 0);
	alarm_start_relative(&gLogAlarm.alarm_timer, kt);

	pr_info("[connsys_log_alarm] alarm timer enabled timeout=[%d]", gLogAlarm.alarm_sec);
	return 0;
}

static int connlog_cancel_alarm_timer(void)
{
	pr_info("[connsys_log_alarm] alarm timer cancel");
	return alarm_cancel(&gLogAlarm.alarm_timer);
}


static enum alarmtimer_restart connlog_alarm_timer_handler(struct alarm *alarm,
	ktime_t now)
{
	ktime_t kt;
	struct rtc_time tm;
	unsigned int tsec, tusec;
	int i;

	connsys_log_get_utc_time(&tsec, &tusec);
	rtc_time64_to_tm(tsec, &tm);
	pr_info("[connsys_log_alarm] alarm_timer triggered [%d-%02d-%02d %02d:%02d:%02d.%09u]"
			, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday
			, tm.tm_hour, tm.tm_min, tm.tm_sec, tusec);

	for (i = 0; i < CONN_DEBUG_TYPE_END; i++) {
		if (gLogDev[i]) {
			connlog_do_schedule_work(gLogDev[i], false);
		}
	}

	spin_lock_irqsave(&gLogAlarm.alarm_lock, gLogAlarm.flags);
	kt = ktime_set(gLogAlarm.alarm_sec, 0);
	alarm_start_relative(&gLogAlarm.alarm_timer, kt);
	spin_unlock_irqrestore(&gLogAlarm.alarm_lock, gLogAlarm.flags);

	return ALARMTIMER_NORESTART;
}

static int connlog_alarm_init(void)
{
	alarm_init(&gLogAlarm.alarm_timer, ALARM_REALTIME, connlog_alarm_timer_handler);
	gLogAlarm.alarm_state = CONNLOG_ALARM_STATE_DISABLE;
	spin_lock_init(&gLogAlarm.alarm_lock);

	return 0;
}

/*****************************************************************************
* FUNCTION
*  connsys_dedicated_log_path_alarm_enable
* DESCRIPTION
*  Enable log timer.
*  When log timer is enable, it starts every sec seconds to fetch log from EMI
*  to file.
*  Usually enable log timer for debug.
* PARAMETERS
*  sec         [IN]       timer config 
* RETURNS
*  int
*****************************************************************************/
int connsys_dedicated_log_path_alarm_enable(unsigned int sec)
{
	if (!gPhyEmiBase)
		return -1;

	spin_lock_irqsave(&gLogAlarm.alarm_lock, gLogAlarm.flags);

	gLogAlarm.alarm_sec = sec;
	if (!connlog_is_alarm_enable()) {
		gLogAlarm.alarm_state = CONNLOG_ALARM_STATE_ENABLE;
		pr_info("[connsys_log_alarm] alarm timer enabled timeout=[%d]", sec);
	}
	if (gLogAlarm.blank_state == 0)
		connlog_set_alarm_timer();

	spin_unlock_irqrestore(&gLogAlarm.alarm_lock, gLogAlarm.flags);
	return 0;
}
EXPORT_SYMBOL(connsys_dedicated_log_path_alarm_enable);

/*****************************************************************************
* FUNCTION
*  connsys_dedicated_log_path_alarm_disable
* DESCRIPTION
*  Disable log timer
* PARAMETERS
*
* RETURNS
*  int
*****************************************************************************/
int connsys_dedicated_log_path_alarm_disable(void)
{
	int ret;

	if (!gPhyEmiBase)
		return -1;

	spin_lock_irqsave(&gLogAlarm.alarm_lock, gLogAlarm.flags);

	if (connlog_is_alarm_enable()) {
		ret = connlog_cancel_alarm_timer();
		gLogAlarm.alarm_state = CONNLOG_ALARM_STATE_ENABLE;
		pr_info("[connsys_log_alarm] alarm timer disable ret=%d", ret);
	}
	spin_unlock_irqrestore(&gLogAlarm.alarm_lock, gLogAlarm.flags);
	return 0;
}
EXPORT_SYMBOL(connsys_dedicated_log_path_alarm_disable);

/****************************************************************************
* FUNCTION
*  connsys_dedicated_log_path_blank_state_changed
* DESCRIPTION
* 
* PARAMETERS
*
* RETURNS
*  int
*****************************************************************************/
int connsys_dedicated_log_path_blank_state_changed(int blank_state)
{
	int ret = 0;

	if (!gPhyEmiBase)
		return -1;
	spin_lock_irqsave(&gLogAlarm.alarm_lock, gLogAlarm.flags);
	gLogAlarm.blank_state = blank_state;
	if (connlog_is_alarm_enable()) {
		if (blank_state == 0)
			ret = connlog_set_alarm_timer();
		else
			ret = connlog_cancel_alarm_timer();
	}

	spin_unlock_irqrestore(&gLogAlarm.alarm_lock, gLogAlarm.flags);

	return ret;
}
EXPORT_SYMBOL(connsys_dedicated_log_path_blank_state_changed);

/*****************************************************************************
* FUNCTION
*  connsys_dedicated_log_path_apsoc_init
* DESCRIPTION
*  Initialize API for common driver to initialize connsys dedicated log
*  for APSOC platform
* PARAMETERS
*  emiaddr      [IN]        EMI physical base address
*  config       [IN]        platform config
* RETURNS
*  void
****************************************************************************/
int connsys_dedicated_log_path_apsoc_init(phys_addr_t emiaddr, const struct connlog_emi_config* config)
{
	if (gPhyEmiBase != 0 || emiaddr == 0) {
		pr_notice("Connsys log double init or invalid parameter(emiaddr=%llx)\n", emiaddr);
		return -1;
	}

	gPhyEmiBase = emiaddr;

	connlog_alarm_init();

	// Set up connsyslog config
	if (config == NULL) {
		pr_err("Fail to set up connsys log platform config\n");
		return -1;
	} else {
		g_connsyslog_config = config;
	}

#ifdef CONFIG_MTK_CONNSYS_DEDICATED_LOG_PATH
	fw_log_wifi_mcu_init();
	fw_log_bt_mcu_init();
#endif

	return 0;
}
EXPORT_SYMBOL(connsys_dedicated_log_path_apsoc_init);

/*****************************************************************************
* FUNCTION
*  connsys_dedicated_log_path_apsoc_deinit
* DESCRIPTION
*  De-Initialize API for common driver to release cache, un-remap emi and free
*  irq for APSOC platform
* PARAMETERS
*  void
* RETURNS
*  void
*****************************************************************************/
int connsys_dedicated_log_path_apsoc_deinit(void)
{
	int i;

	/* Check subsys */
	for (i = 0; i < CONN_DEBUG_TYPE_END; i++) {
		if (gLogDev[i] != NULL) {
			pr_err("[%s] subsys %s should be deinit first.\n",
				__func__, type_to_title[i]);
			return -1;
		}
	}

	gPhyEmiBase = 0;
	g_connsyslog_config = NULL;

#ifdef CONFIG_MTK_CONNSYS_DEDICATED_LOG_PATH
	fw_log_wifi_mcu_deinit();
	fw_log_bt_mcu_deinit();
#endif

	return 0;
}
EXPORT_SYMBOL(connsys_dedicated_log_path_apsoc_deinit);

/*****************************************************************************
* FUNCTION
*  connsys_dedicated_log_set_ap_state
* DESCRIPTION
*  set ap state
* PARAMETERS
*  int state  0:suspend, 1:resume
* RETURNS
*  0: successfuly, negative if error
*****************************************************************************/
int connsys_dedicated_log_set_ap_state(int state)
{
	int i;
	struct connlog_dev* handler;

	if (state < 0 || state > 1) {
		pr_notice("%s state = %d is unexpected\n", __func__, state);
		return -1;
	}

	for (i = CONN_DEBUG_TYPE_WIFI; i < CONN_DEBUG_PRIMARY_END; i++) {
		handler = gLogDev[i];

		if (handler == NULL || handler->virAddrEmiLogBase == 0) {
			pr_notice("[%s][%s] didn't init\n", __func__, type_to_title[i]);
			continue;
		}

		EMI_WRITE32(handler->virAddrEmiLogBase + 32, state);
	}

	return 0;
}

