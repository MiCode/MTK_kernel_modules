// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#if SUPPORT_BEIF
#include "btmtk_beif_plat.h"
#include "btmtk_beif.h"

#define BEIF_H2C_GUARD_ADDR ((char *)g_beif_header + g_h2c_buf_offset + g_h2c_buf_size)
#define BEIF_C2H_GUARD_ADDR ((char *)g_beif_header + g_c2h_buf_offset + g_c2h_buf_size)
#define BEIF_H2C_BUF_ADDR(offset) ((char *)g_beif_header + g_h2c_buf_offset + offset)
#define BEIF_C2H_BUF_ADDR(offset) ((char *)g_beif_header + g_c2h_buf_offset + offset)
#define BEIF_HEADER_KEY	(0x55665566)
#define BEIF_HEADER_VERSION (0)
#define	BEIF_GUARD_PATTERN	0xDEADBEEF
#define BEIF_GUARD_PATTERN_SIZE	32
#define BEIF_BYTES_PER_RECORD 16
#define BEIF_DIR_TX 0
#define BEIF_DIR_RX 1
#define BEIF_RECORD_NUM 16
#define BEIF_TEMP_BUF_SIZE 2048

struct beif_header_t {
	int key;
	int version;
	int host_ctrl_offset;
	int fw_ctrl_offset;
	int h2c_buf_offset;
	int h2c_buf_size;
	int c2h_buf_offset;
	int c2h_buf_size;
};

struct beif_ctrl_t {
	int tx_push;
	int rx_pop;
	int dir;		//not used
	int mode;		//not used
	int burst_size;	//not used
	int packet_size;	//not used
	int status;		//not used
	int guardp;
};

struct beif_data_record_t {
	int start;
	int len;
	char data[BEIF_BYTES_PER_RECORD];
	u64 sec;
	unsigned long nsec;
};

static struct beif_header_t *g_beif_header;
static struct beif_ctrl_t   *g_host_header;
static struct beif_ctrl_t   *g_fw_header;
static struct beif_info_t g_info;
static unsigned int g_h2c_buf_size;
static unsigned int g_c2h_buf_size;
static unsigned int g_h2c_buf_offset;
static unsigned int g_c2h_buf_offset;
static unsigned int guard_pattern = BEIF_GUARD_PATTERN;

/* for debugging */
static u64 g_total_send_bytes;
static u64 g_total_receive_bytes;
static struct beif_data_record_t g_rx_record[BEIF_RECORD_NUM];
static struct beif_data_record_t g_tx_record[BEIF_RECORD_NUM];
static int g_rx_rec_idx;
static int g_tx_rec_idx;
static int g_rxd_in_cb;
static unsigned char *g_beif_rx_buf;

static int beif_rx_init(void);
static int beif_rx_deinit(void);
#ifdef BEIF_CTP_LOAD
#ifdef BEIF_MULTI_SENDER
static void *beif_h2c_lock;
#endif
#else
static struct completion rx_comp;
static struct task_struct *p_task;
#ifdef BEIF_MULTI_SENDER
static struct mutex beif_h2c_lock;
#endif
#endif

static int beif_dump_array(const char *string, const char *p_buf, unsigned int len)
{
#define BEIF_LENGTH_PER_LINE 32
	unsigned int idx = 0;
	char str[BEIF_LENGTH_PER_LINE * 3 + 2];
	char *p_str = NULL;

	if (string != NULL)
		pr_info("========dump %s start <length:%d>========\n", string, len);
	p_str = &str[0];
	for (idx = 0; idx < len; idx++) {
		if (snprintf(p_str, 4, "%02x ", p_buf[idx]) < 0)
			return -1;
		p_str += 3;
		if ((BEIF_LENGTH_PER_LINE - 1) == (idx % BEIF_LENGTH_PER_LINE)) {
			*p_str++ = '\n';
			*p_str = '\0';
			pr_info("%s", str);
			p_str = &str[0];
		}
	}
	if (len % BEIF_LENGTH_PER_LINE) {
		*p_str++ = '\n';
		*p_str = '\0';
		pr_info("%s", str);
	}
	if (string != NULL)
		pr_info("========dump %s end========\n", string);
	return 0;
}

static void beif_print_record(int dir)
{
	struct beif_data_record_t *t;
	struct beif_data_record_t *r;
	int idx;
	int i;
	int start = 0;
	int num = BEIF_RECORD_NUM;

	if (dir == BEIF_DIR_TX) {
		t = g_tx_record;
		idx = g_tx_rec_idx;
	} else if (dir == BEIF_DIR_RX) {
		t = g_rx_record;
		idx = g_rx_rec_idx;
	} else {
		pr_info("%s dir (%d) is invalid\n", __func__, dir);
		return;
	}

	if (idx > BEIF_RECORD_NUM)
		start = idx;
	else
		num = idx;

	for (i = 0; i < num; i++) {
		r = &t[(start + i) % BEIF_RECORD_NUM];
		pr_info("[beif][%s][%d][%llu.%06lu][start(%d)][len(%d)]",
			dir == BEIF_DIR_TX ? "TX" : "RX",
			i, r->sec, r->nsec, r->start, r->len);
		beif_dump_array(NULL, r->data,
			r->len >= BEIF_BYTES_PER_RECORD ? BEIF_BYTES_PER_RECORD : r->len);
	}
}

void beif_print_tx_log(void)
{
	pr_info("[beif]TX:%llu bytes", g_total_send_bytes);
	beif_print_record(BEIF_DIR_TX);
}

void beif_print_rx_log(void)
{
	pr_info("[beif]RX:%llu bytes", g_total_receive_bytes);
	beif_print_record(BEIF_DIR_RX);
}

static int beif_add_record(const unsigned char *data, int start, int len, int dir)
{
	struct beif_data_record_t *t;
	struct beif_data_record_t *r;
	int idx;
	int i;

	if (dir == BEIF_DIR_TX) {
		t = g_tx_record;
		idx = g_tx_rec_idx;
	} else if (dir == BEIF_DIR_RX) {
		t = g_rx_record;
		idx = g_rx_rec_idx;
	} else {
		pr_info("%s dir (%d) is invalid\n", __func__, dir);
		return -1;
	}

	if (idx < 0) {
		pr_info("%s idx (%d) is invalid\n", __func__, idx);
		return -1;
	}

	if (len <= 0) {
		pr_info("%s len (%d) is invalid\n", __func__, len);
		return -1;
	}

	i = idx % BEIF_RECORD_NUM;
	r = &t[i];
	r->len = len;
	r->start = start;
	memcpy(r->data, data, len >= BEIF_BYTES_PER_RECORD ? BEIF_BYTES_PER_RECORD : len);
	beif_get_local_time(&(r->sec), &(r->nsec));

	if (dir == BEIF_DIR_TX)
		g_tx_rec_idx++;
	else
		g_rx_rec_idx++;

	//pr_info("%s dir = %d, len = %d\n", __func__, dir, len);
	return 0;
}

int beif_reset(void)
{
	struct beif_header_t *s = g_beif_header;
	struct beif_ctrl_t *h;
	struct beif_ctrl_t *f;
	unsigned int i;
	int buffer_size;

	if (s == NULL) {
		pr_info("%s beif_header is NULL", __func__);
		return -1;
	}

	if (g_total_send_bytes || g_total_receive_bytes) {
		beif_check_header();
		pr_info("%s send = %llu, receive = %llu\n",
			__func__, g_total_send_bytes, g_total_receive_bytes);
		g_total_send_bytes = 0;
		g_total_receive_bytes = 0;
	}

	buffer_size = (g_info.size - sizeof(struct beif_header_t) -
			(sizeof(struct beif_ctrl_t) + BEIF_GUARD_PATTERN_SIZE) * 2) / 2;

	pr_info("%s addr = 0x%pa, size = %d, buffer_size = %d\n", __func__,
		&g_info.addr, g_info.size, buffer_size);

	if (buffer_size < 1024) {
		pr_info("buffer size is too small\n");
		return -1;
	}

	/* Initialize beif header */
	EMI_WRITE32(&s->key, BEIF_HEADER_KEY);
	EMI_WRITE32(&s->version, BEIF_HEADER_VERSION);
	EMI_WRITE32(&s->host_ctrl_offset, sizeof(struct beif_header_t));
	EMI_WRITE32(&s->fw_ctrl_offset, sizeof(struct beif_header_t) + sizeof(struct beif_ctrl_t));

	g_h2c_buf_offset = sizeof(struct beif_header_t) + 2 * sizeof(struct beif_ctrl_t);
	EMI_WRITE32(&s->h2c_buf_offset, g_h2c_buf_offset);
	g_h2c_buf_size = buffer_size;
	EMI_WRITE32(&s->h2c_buf_size, g_h2c_buf_size);

	g_c2h_buf_offset = g_h2c_buf_offset + g_h2c_buf_size + BEIF_GUARD_PATTERN_SIZE;
	EMI_WRITE32(&s->c2h_buf_offset, g_c2h_buf_offset);
	g_c2h_buf_size = buffer_size;
	EMI_WRITE32(&s->c2h_buf_size, g_c2h_buf_size);

	pr_info("beif_header [0x%p], key = %x, version = %d, host_ctrl_offset = %d\n",
		s, EMI_READ32(&s->key), EMI_READ32(&s->version), EMI_READ32(&s->host_ctrl_offset));
	pr_info("fw_ctrl_offset = %d, h2c_buf_offset = %d, h2c_buf_size = %d\n",
		EMI_READ32(&s->fw_ctrl_offset), EMI_READ32(&s->h2c_buf_offset), EMI_READ32(&s->h2c_buf_size));
	pr_info("c2h_buf_offset = %d, c2h_buf_size = %d\n",
		EMI_READ32(&s->c2h_buf_offset), EMI_READ32(&s->c2h_buf_size));

	/* Initialize host control header */
	g_host_header = (struct beif_ctrl_t *)((char *)s + sizeof(struct beif_header_t));
	h = g_host_header;
	EMI_WRITE32(&h->tx_push, 0);
	EMI_WRITE32(&h->rx_pop, 0);
	EMI_WRITE32(&h->guardp, BEIF_GUARD_PATTERN);

	pr_info("memset from %p, size %d\n", BEIF_H2C_BUF_ADDR(0), g_h2c_buf_size);
	memset_io(BEIF_H2C_BUF_ADDR(0), 0, g_h2c_buf_size);

	for (i = 0; i < BEIF_GUARD_PATTERN_SIZE/sizeof(guard_pattern); i++)
		EMI_WRITE32(BEIF_H2C_GUARD_ADDR + i * sizeof(guard_pattern), guard_pattern);

	/* Initialize fw control header */
	g_fw_header = (struct beif_ctrl_t *)((char *)s + sizeof(struct beif_header_t) + sizeof(struct beif_ctrl_t));
	f = g_fw_header;
	EMI_WRITE32(&f->tx_push, 0);
	EMI_WRITE32(&f->rx_pop, 0);
	EMI_WRITE32(&f->guardp, BEIF_GUARD_PATTERN);

	memset_io(BEIF_C2H_BUF_ADDR(0), 0, g_c2h_buf_size);

	for (i = 0; i < BEIF_GUARD_PATTERN_SIZE/sizeof(guard_pattern); i++)
		EMI_WRITE32(BEIF_C2H_GUARD_ADDR + i * sizeof(guard_pattern), guard_pattern);

	memset(g_rx_record, 0, sizeof(g_rx_record));
	memset(g_tx_record, 0, sizeof(g_tx_record));
	g_rx_rec_idx = 0;
	g_tx_rec_idx = 0;

	return 0;
}

int beif_init(struct beif_info_t *info)
{
	if (!info || !info->addr || !info->rx_cb || info->size <= 0 || !info->notify_fw_cb) {
		pr_info("%s init failed.\n", __func__);
		return -1;
	}
	memcpy(&g_info, info, sizeof(struct beif_info_t));

	g_beif_header = (struct beif_header_t *)ioremap((unsigned int)info->addr, info->size);
	if (g_beif_header == NULL) {
		pr_info("ioremap is failed. beif_header == NULL\n");
		return -1;
	}

	if (beif_reset() < 0)
		return -1;

#ifdef BEIF_MULTI_SENDER
	mutex_init(&beif_h2c_lock);
#endif
	beif_rx_init();
	return 0;
}

int beif_deinit(void)
{
	beif_rx_deinit();
	if (g_beif_header != NULL) {
		iounmap((void *)g_beif_header);
		g_beif_header = NULL;
	}

#ifdef BEIF_MULTI_SENDER
	mutex_destroy(&beif_h2c_lock);
#endif
	return 0;
}

int beif_send_data(const unsigned char *p_buf, unsigned int len)
{
	struct beif_ctrl_t *h = g_host_header;
	struct beif_ctrl_t *f = g_fw_header;
	unsigned int space;
	int retry = 0;
	unsigned int push, pop;
	unsigned int space_to_end;

	if (!p_buf || !h || !f) {
		pr_info("%s, failed. p_buf = %p, g_host_header = %p, g_fw_header = %p\n",
			__func__, p_buf, h, f);
		return -1;
	}
#ifdef BEIF_MULTI_SENDER
	if (mutex_lock_killable(&beif_h2c_lock)) {
		pr_info("%s get h2c_lock fail\n", __func__);
		return -1;
	}
#endif
	do {
		push = EMI_READ32(&h->tx_push);
		pop = EMI_READ32(&f->rx_pop);
		space = (push >= pop) ?
			(g_h2c_buf_size - (push - pop) - 1) : (pop - push - 1);

		if (space >= len) {
			if (push >= pop) {
				space_to_end = g_h2c_buf_size - push;
				if (space_to_end >= len) {
					memcpy_toio(BEIF_H2C_BUF_ADDR(push), p_buf, len);
					EMI_WRITE32(&h->tx_push, (push + len) % g_h2c_buf_size);
				} else {
					memcpy_toio(BEIF_H2C_BUF_ADDR(push), p_buf, space_to_end);
					memcpy_toio(BEIF_H2C_BUF_ADDR(0), p_buf + space_to_end,
							len - space_to_end);
					EMI_WRITE32(&h->tx_push, len - space_to_end);
				}
			} else {
				memcpy_toio(BEIF_H2C_BUF_ADDR(push), p_buf, len);
				EMI_WRITE32(&h->tx_push, push + len);
			}

			g_info.notify_fw_cb();
			g_total_send_bytes += len;
			beif_add_record(p_buf, push, len, BEIF_DIR_TX);
#ifdef BEIF_MULTI_SENDER
			mutex_unlock(&beif_h2c_lock);
#endif
			return len;
		} else {
			retry++;
			udelay(100);
			pr_info("buffer is full, wait for 100 us. retry = %d\n", retry);
		}
	} while(retry < 100);

#ifdef BEIF_MULTI_SENDER
	mutex_unlock(&beif_h2c_lock);
#endif
	return 0;
}

static int beif_data_consummer(void)
{
	struct beif_ctrl_t *h = g_host_header;
	struct beif_ctrl_t *f = g_fw_header;
	unsigned int data_size = 0;
	BEIF_RX_CB *rx_cb = &g_info.rx_cb;
	int push;
	int pop;
	int retry = 0;

	pop = EMI_READ32(&h->rx_pop);
	do {
		push = EMI_READ32(&f->tx_push);
		if (push > pop) {
			data_size = (push - pop) > BEIF_TEMP_BUF_SIZE ? BEIF_TEMP_BUF_SIZE : push - pop;
			memcpy_fromio(g_beif_rx_buf, BEIF_C2H_BUF_ADDR(pop), data_size);
			g_rxd_in_cb = 1;
			(*rx_cb)(g_beif_rx_buf, data_size);
			g_rxd_in_cb = 0;
			beif_add_record(g_beif_rx_buf, pop, data_size, BEIF_DIR_RX);
		} else if (push < pop) {
			data_size = (g_c2h_buf_size - pop) > BEIF_TEMP_BUF_SIZE ?
				BEIF_TEMP_BUF_SIZE : g_c2h_buf_size - pop;
			memcpy_fromio(g_beif_rx_buf, BEIF_C2H_BUF_ADDR(pop), data_size);
			g_rxd_in_cb = 1;
			(*rx_cb)(g_beif_rx_buf, data_size);
			g_rxd_in_cb = 0;
			beif_add_record(g_beif_rx_buf, pop, data_size, BEIF_DIR_RX);
		} else
			break;

		pop = (pop + data_size) % g_c2h_buf_size;
		g_total_receive_bytes += data_size;
		retry++;
	} while(retry < 10);

	if (retry >= 8)
		pr_info("%s retry time (%d)\n", __func__, retry);

	EMI_WRITE32(&h->rx_pop, pop);
	return 0;
}

int beif_receive_data(void)
{
#ifndef BEIF_CTP_LOAD
	complete(&rx_comp);
#else
	beif_data_consummer();
#endif
	return 0;
}

int beif_check_header(void)
{
	struct beif_header_t *s = g_beif_header;
	unsigned int val, val2;
	int ret = 0;
	unsigned int i;

	if (g_rxd_in_cb)
		pr_info("%s rxd thread might be blocked in cb\n", __func__);

	if (s == NULL) {
		pr_info("%s beif_header is NULL", __func__);
		ret = -1;
		goto end;
	}
	beif_dump_array("beif header", (const char *)s,
		sizeof(struct beif_header_t) + 2 * sizeof(struct beif_ctrl_t));

	val = EMI_READ32(&s->key);
	if (val != BEIF_HEADER_KEY) {
		pr_info("%s s->key(%d) != BEIF_HEADER_KEY(%d)", __func__, val, BEIF_HEADER_KEY);
		ret = -1;
	}

	val = EMI_READ32(&s->version);
	if (val != BEIF_HEADER_VERSION) {
		pr_info("%s s->version(%d) != BEIF_HEADER_VERSION(%d)", __func__, val, BEIF_HEADER_VERSION);
		ret = -1;
	}

	val = EMI_READ32(&s->host_ctrl_offset);
	if (val != sizeof(struct beif_header_t)) {
		pr_info("%s s->host_ctrl_offset(%u) != sizeof(struct beif_header_t) %lu",
			__func__, val, sizeof(struct beif_header_t));
		ret = -1;
	}

	val = EMI_READ32(&s->fw_ctrl_offset);
	if (val != (sizeof(struct beif_header_t) + sizeof(struct beif_ctrl_t))) {
		pr_info("%s s->fw_ctrl_offset(%u) != %lu",
			__func__, val, sizeof(struct beif_header_t) + sizeof(struct beif_ctrl_t));
		ret = -1;
	}

	val = EMI_READ32(&s->h2c_buf_offset);
	if (val != g_h2c_buf_offset) {
		pr_info("%s s->h2c_buf_offset(%u) != g_h2c_buf_offset(%u)",
			__func__, val, g_h2c_buf_offset);
		ret = -1;
	}

	val = EMI_READ32(&s->h2c_buf_size);
	if (val != g_h2c_buf_size) {
		pr_info("%s s->h2c_buf_size(%u) != g_h2c_buf_size(%u)",
			__func__, val, g_h2c_buf_size);
		ret = -1;
	}

	val = EMI_READ32(&s->c2h_buf_offset);
	if (val != g_c2h_buf_offset) {
		pr_info("%s s->c2h_buf_offset(%u) != g_c2h_buf_offset(%u)",
			__func__, val, g_c2h_buf_offset);
		ret = -1;
	}

	val = EMI_READ32(&s->c2h_buf_size);
	if (val != g_c2h_buf_size) {
		pr_info("%s s->c2h_buf_size(%u) != g_c2h_buf_size(%u)",
			__func__, val, g_c2h_buf_size);
		ret = -1;
	}

	for (i = 0; i < BEIF_GUARD_PATTERN_SIZE/sizeof(guard_pattern); i++) {
		val = EMI_READ32(BEIF_H2C_GUARD_ADDR + i * sizeof(guard_pattern));
		val2 = EMI_READ32(BEIF_C2H_GUARD_ADDR + i * sizeof(guard_pattern));
		if (val != BEIF_GUARD_PATTERN || val2 != BEIF_GUARD_PATTERN) {
			pr_info("%s guard pattern is corrupted.", __func__);
			ret = -1;
		}
	}

end:
	if (ret < 0)
		pr_info("%s, ERROR!! Please check above log", __func__);
	else
		pr_info("%s, no error is found", __func__);

	return ret;
}

int beif_is_tx_complete(void)
{
	struct beif_ctrl_t *h = g_host_header;
	struct beif_ctrl_t *f = g_fw_header;

	if (!h || !f) {
		pr_info("%s failed, g_host_header=%p, g_fw_header=%p",__func__, h, f);
		return -1;
	}

	if (h->tx_push != f->rx_pop || h->rx_pop != f->tx_push)
		return 0;

	return 1;
}

int beif_dump_tx_last_data(unsigned int len)
{
	struct beif_ctrl_t *h = g_host_header;
	struct beif_ctrl_t *f = g_fw_header;
	unsigned int push;
	unsigned int pop;

	if (!h || !f) {
		pr_info("%s failed, g_host_header=%p, g_fw_header=%p",__func__, h, f);
		return -1;
	}
	push = EMI_READ32(&h->tx_push);
	pop = EMI_READ32(&f->rx_pop);

	if (push == pop)
		pr_info("%s push = %d, pop = %d, all data is received.\n", __func__, push, pop);
	else
		pr_info("%s push = %d, pop = %d, some data is not received.\n", __func__, push, pop);

	if (len < push)
		beif_dump_array("last TX data", BEIF_H2C_BUF_ADDR(push) - len, len);
	else {
		beif_dump_array("last TX data-1", BEIF_H2C_BUF_ADDR(g_h2c_buf_size - len + push), len - push);
		beif_dump_array("last TX data-2", BEIF_H2C_BUF_ADDR(0), push);
	}
	return 0;
}

int beif_dump_rx_last_data(unsigned int len)
{
	struct beif_ctrl_t *h = g_host_header;
	struct beif_ctrl_t *f = g_fw_header;
	unsigned int push;
	unsigned int pop;

	if (!h || !f) {
		pr_info("%s failed, g_host_header=%p, g_fw_header=%p",__func__, h, f);
		return -1;
	}
	push = EMI_READ32(&f->tx_push);
	pop = EMI_READ32(&h->rx_pop);

	if (push == pop)
		pr_info("%s push = %d, pop = %d, all data is received.\n", __func__, push, pop);
	else
		pr_info("%s push = %d, pop = %d, some data is not received.\n", __func__, push, pop);

	if (len < push)
		beif_dump_array("last RX data", BEIF_C2H_BUF_ADDR(push) - len, len);
	else {
		beif_dump_array("last RX data-1", BEIF_C2H_BUF_ADDR(g_c2h_buf_size - len + push), len - push);
		beif_dump_array("last RX data-2", BEIF_C2H_BUF_ADDR(0), push);
	}
	return 0;
}

#ifndef BEIF_CTP_LOAD
static int beif_rx_thread(void *p_data)
{
	struct sched_param sch_param;
	int ret;

	sch_param.sched_priority = MAX_RT_PRIO - 10;
	ret = sched_setscheduler(current, SCHED_RR, &sch_param);
	pr_info("%s prio = %d, ret = %d\n", __func__, sch_param.sched_priority, ret);

	while (1) {
		wait_for_completion_interruptible(&rx_comp);
		if (kthread_should_stop()) {
			break;
		}
		beif_data_consummer();
	}
	return 0;
}
#endif

static int beif_rx_init(void)
{
	int ret = 0;

#ifdef BEIF_CTP_LOAD
	g_beif_rx_buf = MEM_Allocate_Align_NC(BEIF_TEMP_BUF_SIZE, SZ_64K, MEM_USER_BT);
#else
	init_completion(&rx_comp);

	g_beif_rx_buf = vmalloc(BEIF_TEMP_BUF_SIZE);
	if (g_beif_rx_buf == NULL) {
		pr_info("g_beif_rx_buf is NULL\n");
		return -1;
	}

	/*create kernel thread for later rx data handle*/
	p_task = kthread_create(beif_rx_thread, NULL,
				"beif_rxd");
	if (IS_ERR_OR_NULL(p_task)) {
		pr_info("kthread_create fail\n");
		return -1;
	}

	wake_up_process(p_task);
#endif
	return ret;
}

static int beif_rx_deinit(void)
{
#ifdef BEIF_CTP_LOAD
	if (g_beif_rx_buf) {
		MEM_Release_NC(g_beif_rx_buf);
		g_beif_rx_buf = NULL;
	}
#else
	int ret = 0;

	if (!IS_ERR_OR_NULL(p_task)) {
		ret = kthread_stop(p_task);
		if (ret) {
			pr_info("thread_stop fail(%d)\n", ret);
			return -1;
		}
	}

	if (g_beif_rx_buf) {
		vfree(g_beif_rx_buf);
		g_beif_rx_buf = NULL;
	}
#endif
	return 0;
}
#endif
