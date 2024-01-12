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
};

struct connlog_buffer {
	struct ring_emi ring_emi;
	struct ring ring_cache;
	void *cache_base;
};

struct connlog_print_buffer {
	void *log_buf;
	u32 log_buf_sz;
};

struct connlog_ctrl_block {
	bool enable;
	struct connlog_offset log_offset;
	struct connlog_buffer log_buffer;
	struct connlog_print_buffer log_print_buffer;

	CONNLOG_EVENT_CB log_data_handler;
};

struct connlog_dev {
	int conn_type;
	phys_addr_t phyAddrEmiBase;
	unsigned int emi_size;
	void __iomem *virAddrEmiLogBase;

	struct connlog_ctrl_block ctrl_block[CONN_EMI_BLOCK_TYPE_SIZE];

	bool eirqOn;
	spinlock_t irq_lock;
	unsigned long flags;
	unsigned int irq_counter;
	OSAL_TIMER workTimer;
	struct work_struct logDataWorker;
	char log_line[LOG_MAX_LEN];
};

static char *type_to_title[CONN_DEBUG_TYPE_END] = {
	"wifi_fw", "bt_fw", "gps_fw"
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

const struct connlog_emi_config *g_connsyslog_config = NULL;

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
static void connlog_do_schedule_work(struct connlog_dev* handler, bool count);
static void work_timer_handler(timer_handler_arg data);
static void connlog_event_set(struct connlog_ctrl_block *block);
static struct connlog_dev* connlog_subsys_init(
	int conn_type,
	const struct connlog_emi_config *emi_config);

static void connlog_subsys_deinit(struct connlog_dev* handler);
static ssize_t connlog_read_internal(
	struct connlog_dev* handler, int conn_type, int block_type,
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

	if (g_connsyslog_config == NULL) {
		pr_err("g_connsyslog_config not init");
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
static void connlog_event_set(struct connlog_ctrl_block *block)
{
	if (block->log_data_handler)
		block->log_data_handler();
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

static int connlog_emi_init(struct connlog_dev* handler,
		const struct connlog_emi_config *emi_config)
{
	int conn_type = handler->conn_type;
	phys_addr_t emi_addr;
	u32 emi_size, valid_size, base_addr;
	u32 block_emi_size;
	struct connlog_ctrl_block *block;
	int i;

	if (conn_type < 0 || conn_type >= CONN_DEBUG_TYPE_END) {
		pr_notice("%s conn_type %d is invalid\n", __func__, conn_type);
		return -1;
	}

	/* size validation */
	block_emi_size = emi_config->block[CONN_EMI_BLOCK_PRIMARY].size;
	valid_size = connlog_cal_log_size(block_emi_size);
	if (valid_size == 0 || valid_size != block_emi_size) {
		pr_err("[%s][primary] consys emi memory size invalid: config emi size=[%d] cal size=[%d]\n",
			type_to_title[conn_type], block_emi_size, valid_size);
		return -1;
	}
	pr_info("[%s][primary] block size=[%d]",
		type_to_title[conn_type], emi_config->block[CONN_EMI_BLOCK_PRIMARY].size);
	handler->ctrl_block[CONN_EMI_BLOCK_PRIMARY].enable = true;

	for (i = 1; i < CONN_EMI_BLOCK_TYPE_SIZE; i++) {
		block_emi_size = emi_config->block[i].size;
		valid_size = connlog_cal_log_size(block_emi_size);
		if (valid_size == 0) {
			pr_warn("[%s][%s][block-%d] consys emi memory size invalid: emi_size=[%d] cal size=[%d]\n",
				__func__, type_to_title[conn_type], i, emi_config->block[i].size, valid_size);
			continue;
		}
		pr_info("[%s][%s] [block-%d] block size=[%d]", __func__, type_to_title[conn_type], i, block_emi_size);
		handler->ctrl_block[i].enable = true;
	}

	emi_addr = emi_config->log_offset + gPhyEmiBase;
	emi_size = emi_config->log_size;
	pr_info("[%s][%s] Base offset=%p size=0x%x\n", __func__, type_to_title[conn_type], emi_addr, emi_size);

	handler->phyAddrEmiBase = emi_addr;
	handler->emi_size = emi_size;
	handler->virAddrEmiLogBase = ioremap(handler->phyAddrEmiBase, emi_size);

	if (handler->virAddrEmiLogBase == NULL) {
		pr_err("[%s] EMI mapping fail\n", type_to_title[conn_type]);
		return -1;
	}
	pr_info("[%s][%s] EMI mapping OK virtual(0x%p) (0x%x) physical(0x%x) size=%d\n",
		__func__, type_to_title[conn_type],
		handler->virAddrEmiLogBase, handler->virAddrEmiLogBase,
		(unsigned int)handler->phyAddrEmiBase,
		handler->emi_size);

	base_addr = CONNLOG_EMI_BASE_OFFSET;
	for (i = 0; i < CONN_EMI_BLOCK_TYPE_SIZE; i++) {
		block = &handler->ctrl_block[i];
		if (!block->enable)
			continue;

		valid_size = emi_config->block[i].size;
		block->log_offset.emi_base_offset = base_addr;
		block->log_offset.emi_size = valid_size;

		block->log_offset.emi_read = CONNLOG_EMI_READ_OFFSET(base_addr);
		block->log_offset.emi_write = CONNLOG_EMI_WRITE_OFFSET(base_addr);
		block->log_offset.emi_buf = CONNLOG_EMI_BUF_OFFSET(base_addr);
		block->log_offset.emi_guard_pattern_offset =
				block->log_offset.emi_buf + valid_size;

		pr_info("[%s][%s] [%d] [%x] sz=[%d] ptr=[%x][%x] buf=[%x]", __func__, type_to_title[conn_type], i,
					block->log_offset.emi_base_offset, block->log_offset.emi_size,
					block->log_offset.emi_read, block->log_offset.emi_write, block->log_offset.emi_buf);
		base_addr += (CONNLOG_EMI_SUB_HEADER + valid_size +
						CONNLOG_CONTROL_RING_BUFFER_RESERVE_SIZE);
	}

	/* reset */
	memset_io(handler->virAddrEmiLogBase, 0xff, handler->emi_size);
	/* clean header */
	memset_io(handler->virAddrEmiLogBase, 0x0, CONNLOG_CONTROL_RING_BUFFER_BASE_SIZE);
	/* Set state to resume initially */
	EMI_WRITE32(CONNLOG_EMI_AP_STATE_OFFSET(handler->virAddrEmiLogBase), 1);

	for (i = 0; i < CONN_EMI_BLOCK_TYPE_SIZE; i++) {

		block = &handler->ctrl_block[i];
		if (!block->enable)
			continue;
		/* Clean control block as 0; subheader */
		memset_io(handler->virAddrEmiLogBase + block->log_offset.emi_read,
							0x0, CONNLOG_EMI_32_BYTE_ALIGNED);
		/* Setup header */
		EMI_WRITE32(handler->virAddrEmiLogBase + (i * 8), block->log_offset.emi_base_offset);
		EMI_WRITE32(handler->virAddrEmiLogBase + (i * 8) + 4, block->log_offset.emi_size);

		/* Setup end pattern */
		memcpy_toio(handler->virAddrEmiLogBase + block->log_offset.emi_guard_pattern_offset,
			CONNLOG_EMI_END_PATTERN, CONNLOG_EMI_END_PATTERN_SIZE);
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

static int connlog_buffer_init(struct connlog_dev* handler, int idx)
{
	void *pBuffer = NULL;
	unsigned int cache_size = 0;
	struct connlog_ctrl_block *block = &handler->ctrl_block[idx];

	if (!handler || handler->conn_type < 0 || handler->conn_type >= CONN_DEBUG_TYPE_END)
		return -1;

	pr_info("[%s][%s][%d] [%p] sz=[%d] [%p][%p]", __func__, type_to_title[handler->conn_type], idx,
		(handler->virAddrEmiLogBase + block->log_offset.emi_buf),
		block->log_offset.emi_size,
		(handler->virAddrEmiLogBase + block->log_offset.emi_read),
		(handler->virAddrEmiLogBase + block->log_offset.emi_write));


	/* Init ring emi */
	ring_emi_init(
		handler->virAddrEmiLogBase + block->log_offset.emi_buf,
		block->log_offset.emi_size,
		handler->virAddrEmiLogBase + block->log_offset.emi_read,
		handler->virAddrEmiLogBase + block->log_offset.emi_write,
		&block->log_buffer.ring_emi);

	/* init ring cache */
	/* TODO: use emi size. Need confirm */
	cache_size = block->log_offset.emi_size * 2;
	pBuffer = connlog_cache_allocate(cache_size);

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


	/* init print log buffer */
	pBuffer = connlog_cache_allocate(cache_size);
	if (pBuffer == NULL) {
		pr_notice("[%s] allocate ring buffer fail\n", __func__);
		return -ENOMEM;
	}
	block->log_print_buffer.log_buf = pBuffer;
	block->log_print_buffer.log_buf_sz = cache_size;

	return 0;
}

static int connlog_ring_buffer_init(struct connlog_dev* handler)
{
	int i;

	if (handler->conn_type < 0 || handler->conn_type >= CONN_DEBUG_TYPE_END) {
		pr_notice("%s invalid conn_type %d\n", __func__, handler->conn_type);
		return -1;
	}

	if (!handler->virAddrEmiLogBase) {
		pr_err("[%s] consys emi memory address phyAddrEmiBase invalid\n",
			type_to_title[handler->conn_type]);
		return -1;
	}

	for (i = 0; i < CONN_EMI_BLOCK_TYPE_SIZE; i++) {
		if (handler->ctrl_block[i].enable == false)
			continue;
		if (connlog_buffer_init(handler, i))
			goto err_exit;
	}

	connlog_set_ring_ready(handler);

	return 0;
err_exit:
	for (i = 0; i < CONN_EMI_BLOCK_TYPE_SIZE; i++) {
		if (handler->ctrl_block[i].log_buffer.cache_base) {
			connlog_cache_free(handler->ctrl_block[i].log_buffer.cache_base);
			handler->ctrl_block[i].log_buffer.cache_base = NULL;
		}
	}
	return -1;
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
	int i;
	struct connlog_ctrl_block *block;

	for (i = 0; i < CONN_EMI_BLOCK_TYPE_SIZE; i++) {
		block = &handler->ctrl_block[i];
		if (block->log_buffer.cache_base) {
			connlog_cache_free(block->log_buffer.cache_base);
			block->log_buffer.cache_base = NULL;
		}
		if (block->log_print_buffer.log_buf) {
			connlog_cache_free(block->log_print_buffer.log_buf);
			block->log_print_buffer.log_buf = NULL;
		}
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
static bool connlog_ring_emi_check(struct connlog_dev* handler,
			struct connlog_ctrl_block *block)
{
	struct ring_emi *ring_emi = &block->log_buffer.ring_emi;
	char line[CONNLOG_EMI_END_PATTERN_SIZE + 1];

	if (handler->conn_type < 0 || handler->conn_type >= CONN_DEBUG_TYPE_END) {
		pr_notice("%s conn_type %d is invalid\n", __func__, handler->conn_type);
		return false;
	}

	memcpy_fromio(
		line,
		handler->virAddrEmiLogBase + block->log_offset.emi_guard_pattern_offset,
		CONNLOG_EMI_END_PATTERN_SIZE);
	line[CONNLOG_EMI_END_PATTERN_SIZE] = '\0';

	/* Check ring_emi buffer memory. Dump EMI data if it is corruption. */
	if (EMI_READ32(ring_emi->read) > block->log_offset.emi_size ||
		EMI_READ32(ring_emi->write) > block->log_offset.emi_size ||
		strncmp(line, CONNLOG_EMI_END_PATTERN, CONNLOG_EMI_END_PATTERN_SIZE) != 0) {
		pr_err("[connlog] %s out of bound or guard pattern overwrited. Read(pos=%p)=[0x%x] write(pos=%p)=[0x%x] size=[0x%x]\n",
			type_to_title[handler->conn_type],
			ring_emi->read, EMI_READ32(ring_emi->read),
			ring_emi->write, EMI_READ32(ring_emi->write),
			block->log_offset.emi_size);

		connlog_dump_emi(handler, 0x0, 0x60);
		connlog_dump_emi(handler, block->log_offset.emi_base_offset, 0x20);
		connlog_dump_emi(
			handler,
			block->log_offset.emi_guard_pattern_offset,
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
static void connlog_ring_emi_to_cache(struct connlog_dev* handler, int block_idx)
{
	struct ring_emi_segment ring_emi_seg;
	struct ring_emi *ring_emi = NULL;
	struct ring *ring_cache = NULL;
	int total_size = 0;
	int count = 0;
	unsigned int cache_max_size = 0;
	struct connlog_ctrl_block *block;
#ifndef DEBUG_LOG_ON
	static DEFINE_RATELIMIT_STATE(_rs, 10 * HZ, 1);

	ratelimit_set_flags(&_rs, RATELIMIT_MSG_ON_RELEASE);
#endif
	if (block_idx < 0 || block_idx >= CONN_EMI_BLOCK_TYPE_SIZE) {
		pr_notice("%s block dix %d is invalid\n", __func__, block_idx);
		return;
	}
	if (handler->conn_type < 0 || handler->conn_type >= CONN_DEBUG_TYPE_END) {
		pr_notice("%s conn_type %d is invalid\n", __func__, handler->conn_type);
		return;
	}

	block = &handler->ctrl_block[block_idx];

	ring_emi = &block->log_buffer.ring_emi;
	ring_cache = &block->log_buffer.ring_cache;

	if (RING_FULL(ring_cache)) {
	#ifndef DEBUG_LOG_ON
		if (__ratelimit(&_rs))
	#endif
			pr_warn("[connlog] %s [%d] cache is full.\n", type_to_title[handler->conn_type], block_idx);
		return;
	}

	cache_max_size = RING_WRITE_REMAIN_SIZE(ring_cache);
	if (RING_EMI_EMPTY(ring_emi) || !ring_emi_read_prepare(cache_max_size, &ring_emi_seg, ring_emi)) {
	#ifndef DEBUG_LOG_ON
		if(__ratelimit(&_rs))
	#endif
			pr_err("[connlog] %s [%d] no data.\n", type_to_title[handler->conn_type], block_idx);
		return;
	}

	/* Check ring_emi buffer memory. Dump EMI data if it is corruption. */
	if (connlog_ring_emi_check(handler, block) == false) {
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
		RING_WRITE_FOR_EACH(ring_emi_seg.sz, ring_cache_seg, &block->log_buffer.ring_cache) {
#ifdef DEBUG_RING
			ring_dump(__func__, &block->log_buffer.ring_cache);
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
	pr_info("[%s] [%s] block=[%d] size=[%d]", __func__, type_to_title[handler->conn_type], block_idx, total_size);
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
	char* log_line = NULL;
	const char* buf = NULL;
	int conn_type = handler->conn_type;

	if (conn_type < 0 || conn_type >= CONN_DEBUG_TYPE_END) {
		pr_notice("%s conn_type %d is invalid\n", __func__, conn_type);
		return;
	}

	log_line = handler->log_line;
	buf = handler->ctrl_block[CONN_EMI_BLOCK_PRIMARY].log_print_buffer.log_buf;

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
static void connlog_ring_print(struct connlog_dev* handler, int block_idx)
{
	unsigned int written = 0;
	unsigned int buf_size;
	struct ring_emi_segment ring_emi_seg;
	struct ring_emi *ring_emi = NULL;
	int conn_type = handler->conn_type;
	struct connlog_ctrl_block *block = NULL;

	if (conn_type < 0 || conn_type >= CONN_DEBUG_TYPE_END) {
		pr_notice("%s conn_type %d is invalid\n", __func__, conn_type);
		return;
	}

	block = &handler->ctrl_block[block_idx];
	ring_emi = &block->log_buffer.ring_emi;

	if (RING_EMI_EMPTY(ring_emi) || !ring_emi_read_all_prepare(&ring_emi_seg, ring_emi)) {
		pr_err("type(%s) no data, possibly taken by concurrent reader.\n", type_to_title[conn_type]);
		return;
	}

	if (block->log_print_buffer.log_buf == NULL) {
		pr_notice("type(%s) no buffer", type_to_title[conn_type]);
		return;
	}

	buf_size = ring_emi_seg.remain;
	memset(block->log_print_buffer.log_buf, 0, block->log_print_buffer.log_buf_sz);

	/* Check ring_emi buffer memory. Dump EMI data if it is corruption. */
	if (connlog_ring_emi_check(handler, block) == false) {
		pr_err("[connlog] %s emi check fail\n", type_to_title[handler->conn_type]);
		/* TODO: trigger assert by callback? */
		return;
	}

	RING_EMI_READ_ALL_FOR_EACH(ring_emi_seg, ring_emi) {
		memcpy_fromio(block->log_print_buffer.log_buf + written, ring_emi_seg.ring_emi_pt, ring_emi_seg.sz);
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
	struct connlog_dev* handler =
		container_of(work, struct connlog_dev, logDataWorker);
	struct connlog_ctrl_block *block;
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

	for (i = 0; i < CONN_EMI_BLOCK_TYPE_SIZE; i++) {
		block = &handler->ctrl_block[i];
		if (block->enable == false)
			continue;

		if (!RING_EMI_EMPTY(&block->log_buffer.ring_emi)) {
			if (atomic_read(&g_log_mode) == LOG_TO_FILE)
				connlog_ring_emi_to_cache(handler, i);
			else
				connlog_ring_print(handler, i);
			connlog_event_set(block);
		} else {
	#ifndef DEBUG_LOG_ON
			if (__ratelimit(&_rs))
	#endif
				pr_info("[connlog] %s [%d] emi ring is empty!\n",
					type_to_title[handler->conn_type], i);
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

	return RING_SIZE(&handler->ctrl_block[CONN_EMI_BLOCK_PRIMARY].log_buffer.ring_cache);
}
EXPORT_SYMBOL(connsys_log_get_buf_size);


unsigned int connsys_log_get_mcu_buf_size(int conn_type)
{
	struct connlog_dev* handler;

	if (conn_type < CONN_DEBUG_TYPE_WIFI || conn_type >= CONN_DEBUG_TYPE_END)
		return 0;

	handler = gLogDev[conn_type];
	if (handler == NULL) {
		pr_err("[%s][%s] didn't init\n", __func__, type_to_title[conn_type]);
		return 0;
	}

	return RING_SIZE(&handler->ctrl_block[CONN_EMI_BLOCK_MCU].log_buffer.ring_cache);
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
static ssize_t connlog_read_internal(
	struct connlog_dev* handler, int conn_type, int block_type,
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

	if (conn_type < 0 || conn_type >= CONN_DEBUG_TYPE_END) {
		pr_notice("%s conn_type %d is invalid\n", __func__, conn_type);
		return 0;
	}

	if (block_type < 0 || block_type >= CONN_EMI_BLOCK_TYPE_SIZE) {
		pr_notice("[%s][%s] block_type %d is invalid\n",
			__func__, type_to_title[conn_type], block_type);
		return 0;
	}
	if (handler->ctrl_block[block_type].enable == false) {
		pr_notice("[%s][%s] block_type %d is not enable\n",
			__func__, type_to_title[conn_type], block_type);
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
ssize_t _connsys_log_read_to_user(int conn_type, int block_type, char __user *buf, size_t count)
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
	written = connlog_read_internal(handler, conn_type, block_type, NULL, buf, count, true);
done:
	return written;
}

ssize_t connsys_log_read_to_user(int conn_type, char __user *buf, size_t count)
{
	return _connsys_log_read_to_user(conn_type, CONN_EMI_BLOCK_PRIMARY, buf, count);
}
EXPORT_SYMBOL(connsys_log_read_to_user);


ssize_t connsys_mcu_log_read_to_user(int conn_type, char __user *buf, size_t count)
{
	return _connsys_log_read_to_user(conn_type, CONN_EMI_BLOCK_MCU, buf, count);
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
ssize_t connsys_log_read(int conn_type, char *buf, size_t count)
{
	unsigned int ret = 0;
	struct connlog_dev* handler;

	if (conn_type < CONN_DEBUG_TYPE_WIFI || conn_type >= CONN_DEBUG_TYPE_END)
		goto done;
	if (atomic_read(&g_log_mode) != LOG_TO_FILE)
		goto done;

	handler = gLogDev[conn_type];
	ret = connlog_read_internal(handler, conn_type, CONN_EMI_BLOCK_PRIMARY, buf, NULL, count, false);
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
int _connsys_log_register_event_cb(int conn_type, int block_type, CONNLOG_EVENT_CB func)
{
	struct connlog_dev* handler;
	if (conn_type < CONN_DEBUG_TYPE_WIFI || conn_type >= CONN_DEBUG_TYPE_END) {
		pr_warn("[%s] conn_type=[%d]", __func__, conn_type);
		return -1;
	}
	if (block_type < CONN_EMI_BLOCK_PRIMARY || block_type >= CONN_EMI_BLOCK_TYPE_SIZE) {
		pr_warn("[%s] block_type=[%d]", __func__, block_type);
		return -1;
	}

	handler = gLogDev[conn_type];
	if (handler == NULL) {
		pr_err("[%s][%s] didn't init\n", __func__, type_to_title[conn_type]);
		return -1;
	}

	handler->ctrl_block[block_type].log_data_handler = func;
	return 0;
}

int connsys_log_register_event_cb(int conn_type, CONNLOG_EVENT_CB func)
{
	return _connsys_log_register_event_cb(conn_type, CONN_EMI_BLOCK_PRIMARY, func);
}
EXPORT_SYMBOL(connsys_log_register_event_cb);


int connsys_mcu_log_register_event_cb(int conn_type, CONNLOG_EVENT_CB func)
{
	return _connsys_log_register_event_cb(conn_type, CONN_EMI_BLOCK_MCU, func);
}

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
	const struct connlog_emi_config *emi_config)
{
	struct connlog_dev* handler = 0;

	if (conn_type < CONN_DEBUG_TYPE_WIFI || conn_type >= CONN_DEBUG_TYPE_END)
		return 0;

	handler = (struct connlog_dev*)kzalloc(sizeof(struct connlog_dev), GFP_KERNEL);
	if (!handler)
		return 0;

	handler->conn_type = conn_type;

	if (connlog_emi_init(handler, emi_config)) {
		pr_err("[%s] EMI init failed\n", type_to_title[conn_type]);
		goto error_exit;
	}

	if (connlog_ring_buffer_init(handler)) {
		pr_err("[%s] Ring buffer init failed\n", type_to_title[conn_type]);
		goto error_exit;
	}

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
	const struct connlog_emi_config* emi_config;
	int ret;

	if (conn_type < CONN_DEBUG_TYPE_WIFI || conn_type >= CONN_DEBUG_TYPE_END) {
		pr_err("[%s] invalid type:%d\n", __func__, conn_type);
		return -1;
	}
	if (gLogDev[conn_type] != NULL) {
		pr_err("[%s][%s] double init.\n", __func__, type_to_title[conn_type]);
		return 0;
	}

	emi_config = get_connsyslog_platform_config(conn_type);
	if (!emi_config) {
		pr_err("[%s][%s] get emi config fail.\n", __func__, type_to_title[conn_type]);
		return -1;
	}

	pr_info("[%s][%s] emi=[%x][%x] block size=[%x][%x]",
				__func__, type_to_title[conn_type],
				emi_config->log_offset, emi_config->log_size,
				emi_config->block[0].size, emi_config->block[1].size);
	handler = connlog_subsys_init(conn_type, emi_config);

	gLogDev[conn_type] = handler;

	if (emi_config->block[CONN_EMI_BLOCK_MCU].size > 0) {
		struct connlog_to_user_cb user_cb = {
			.conn_type_id = conn_type,
			.register_evt_cb = connsys_mcu_log_register_event_cb,
			.read_to_user_cb = connsys_mcu_log_read_to_user,
			.get_buf_size = connsys_log_get_mcu_buf_size,
		};

		ret = connlog_to_user_register(conn_type, &user_cb);
		if (ret)
			pr_err("[%s] register fail", __func__);
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
static void connlog_subsys_deinit(struct connlog_dev *handler)
{
	if (handler == NULL)
		return;

	if (handler->ctrl_block[CONN_EMI_BLOCK_MCU].enable)
		connlog_to_user_unregister(handler->conn_type);

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
		pr_err("Connsys log double init or invalid parameter(emiaddr=%p)\n", emiaddr);
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

	for (i = CONN_DEBUG_TYPE_WIFI; i < CONN_DEBUG_TYPE_END; i++) {
		handler = gLogDev[i];

		if (handler == NULL || handler->virAddrEmiLogBase == 0) {
			continue;
		}

		EMI_WRITE32(CONNLOG_EMI_AP_STATE_OFFSET(handler->virAddrEmiLogBase), state);
	}

	return 0;
}

