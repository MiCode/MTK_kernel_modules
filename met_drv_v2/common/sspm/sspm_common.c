// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/string.h>
#include <linux/slab.h>
#include <linux/kernel.h>

#include "met_drv.h"
#include "ondiemet_sspm.h"


struct sspm_met_evnet_header {
	unsigned int rts_event_id;
	char *rts_event_name;
	char *chart_line_name;
	char *key_list;
};


enum {
	#ifdef MET_SSPM_RTS_EVNET
	#undef MET_SSPM_RTS_EVNET
	#endif
	#define MET_SSPM_RTS_EVNET(rts_event_id, key_list) rts_event_id,
	#include "met_sspm_rts_event.h"

	/**********************/
	CUR_MET_RTS_EVENT_NUM,
	MAX_MET_RTS_EVENT_NUM = 128
};


struct sspm_met_evnet_header met_evnet_header[MAX_MET_RTS_EVENT_NUM] = {
	#ifdef MET_SSPM_RTS_EVNET
	#undef MET_SSPM_RTS_EVNET
	#endif
	#define MET_SSPM_RTS_EVNET(rts_event_id, key_list) {rts_event_id, #rts_event_id, #rts_event_id, key_list},
	#include "met_sspm_rts_event.h"
};


static void ondiemet_sspm_start(void);
static void ondiemet_sspm_stop(void);
static int ondiemet_sspm_print_help(char *buf, int len);
static int ondiemet_sspm_process_argument(const char *arg, int len);
static int ondiemet_sspm_print_header(char *buf, int len);


static unsigned int event_id_flag0;
static unsigned int event_id_flag1;
static unsigned int event_id_flag2;
static unsigned int event_id_flag3;
static char *update_rts_event_tbl[MAX_MET_RTS_EVENT_NUM];
static char sspm_help[] = "  --sspm_common=rts_event_name\n";
static char header[] = 	"met-info [000] 0.0: sspm_common_header: ";

struct metdevice met_sspm_common = {
	.name = "sspm_common",
	.owner = THIS_MODULE,
	.type = MET_TYPE_BUS,
	.cpu_related = 0,
	.ondiemet_mode = 1,
	.ondiemet_start = ondiemet_sspm_start,
	.ondiemet_stop = ondiemet_sspm_stop,
	.ondiemet_process_argument = ondiemet_sspm_process_argument,
	.ondiemet_print_help = ondiemet_sspm_print_help,
	.ondiemet_print_header = ondiemet_sspm_print_header,
};


static int ondiemet_sspm_print_help(char *buf, int len)
{
	return snprintf(buf, PAGE_SIZE, sspm_help);
}


static int ondiemet_sspm_print_header(char *buf, int len)
{
	int i;
	int write_len;
	int flag = 0;
	static int is_dump_header = 0;
	static int read_idx = 0;

	len = 0;
	met_sspm_common.header_read_again = 0;
	if (is_dump_header == 0) {
		len = snprintf(buf, PAGE_SIZE, "%s", header);
		is_dump_header = 1;
	}

	for (i=read_idx; i<MAX_MET_RTS_EVENT_NUM; i++) {
		if (met_evnet_header[i].chart_line_name) {
			if (i <32) {
				flag = 1<<i;
				flag = event_id_flag0 & flag;
			} else if (i >=32 && i < 64) {
				flag = 1<<(i-32);
				flag = event_id_flag1 & flag;
			} else if (i >=64 && i < 96) {
				flag = 1<<(i-64);
				flag = event_id_flag2 & flag;
			} else if (i >=96 && i < 128) {
				flag = 1<<(i-96);
				flag = event_id_flag3 & flag;
			}
			if (flag == 0)
				continue;

			write_len = strlen(met_evnet_header[i].chart_line_name) + strlen(met_evnet_header[i].key_list) + 3;
			if ((len+write_len) < PAGE_SIZE) {
				len += snprintf(buf+len, PAGE_SIZE-len, "%u,%s,%s;",
					met_evnet_header[i].rts_event_id,
					met_evnet_header[i].chart_line_name,
					met_evnet_header[i].key_list);
			} else {
				met_sspm_common.header_read_again = 1;
				read_idx = i;
				return len;
			}
		}
	}

	if (i == MAX_MET_RTS_EVENT_NUM) {
		is_dump_header = 0;
		read_idx = 0;
		buf[len-1] = '\n';
		for (i=0; i<MAX_MET_RTS_EVENT_NUM; i++) {
			if (update_rts_event_tbl[i]) {
				kfree(update_rts_event_tbl[i]);
				update_rts_event_tbl[i] = NULL;
			}
		}
		met_sspm_common.mode = 0;
		event_id_flag0 = 0;
		event_id_flag1 = 0;
		event_id_flag2 = 0;
		event_id_flag3 = 0;
	}

	return len;
}


static void ondiemet_sspm_start(void)
{
	if (sspm_buf_available == 0)
		return ;

	/* ID_COMMON = 1<<MID_COMMON */
	ondiemet_module[ONDIEMET_SSPM] |= ID_COMMON;
}


static void ondiemet_sspm_stop(void)
{
	if (sspm_buf_available == 0)
		return ;
}


static void update_event_id_flag(int event_id)
{
	unsigned int ipi_buf[4];
	unsigned int rdata;
	unsigned int res;

	if (sspm_buf_available == 0)
		return ;

	/* main func ID: bit[31-24]; sub func ID: bit[23-18]; argu 0: bit[17-0]
	   #define MET_MAIN_ID_MASK		0xff000000
	   #define FUNC_BIT_SHIFT		  18
	   #define MET_ARGU				(4 << FUNC_BIT_SHIFT)
	   #define MID_BIT_SHIFT		   9
	*/
	if (event_id <32) {
		event_id_flag0 |= 1<<event_id;
		ipi_buf[0] = MET_MAIN_ID | MET_ARGU | MID_COMMON<<MID_BIT_SHIFT | 1;
		ipi_buf[1] = 0;
		ipi_buf[2] = event_id_flag0;
		ipi_buf[3] = 0;
		res = met_ipi_to_sspm_command((void *)ipi_buf, 0, &rdata, 1);
	} else if (event_id >=32 && event_id < 64) {
		event_id_flag1 |= 1<<(event_id-32);
		ipi_buf[0] = MET_MAIN_ID | MET_ARGU | MID_COMMON<<MID_BIT_SHIFT | 1;
		ipi_buf[1] = 1;
		ipi_buf[2] = event_id_flag1;
		ipi_buf[3] = 0;
		res = met_ipi_to_sspm_command((void *)ipi_buf, 0, &rdata, 1);
	} else if (event_id >=64 && event_id < 96) {
		event_id_flag2 |= 1<<(event_id-64);
		ipi_buf[0] = MET_MAIN_ID | MET_ARGU | MID_COMMON<<MID_BIT_SHIFT | 1;
		ipi_buf[1] = 2;
		ipi_buf[2] = event_id_flag2;
		ipi_buf[3] = 0;
		res = met_ipi_to_sspm_command((void *)ipi_buf, 0, &rdata, 1);
	} else if (event_id >=96 && event_id < 128) {
		event_id_flag3 = 1<<(event_id-96);
		ipi_buf[0] |= MET_MAIN_ID | MET_ARGU | MID_COMMON<<MID_BIT_SHIFT | 1;
		ipi_buf[1] = 3;
		ipi_buf[2] = event_id_flag3;
		ipi_buf[3] = 0;
		res = met_ipi_to_sspm_command((void *)ipi_buf, 0, &rdata, 1);
	}
	met_sspm_common.mode = 1;
}


static char *strdup(const char *s)
{
	char *p = kmalloc(strlen(s) + 1, GFP_KERNEL);

	if (p)
		strcpy(p, s);
	return p;
}


static int ondiemet_sspm_process_argument(const char *arg, int len)
{
	int i = 0;
	int rts_event_id = -1;
	int res = 0;
	int ret = 0;
	int reset_data = 0;
	char *line = NULL;
	char *token = NULL;
	struct sspm_met_evnet_header old_data;

	for (i = 0; (i < MAX_MET_RTS_EVENT_NUM) && met_evnet_header[i].rts_event_name; i++) {
		if (strcmp(met_evnet_header[i].rts_event_name, arg) == 0) {
			rts_event_id = i;
			break;
		}
	}

	if (strstarts(arg, "update_rts_event_tbl")) {
		char *ptr = NULL;

		/* update_rts_event_tbl=rts_event_id;rts_event_name;chart_line_name;key_list*/
		line = strdup(arg);
		if (line == NULL)
			return -1;
		ptr = line;
		token = strsep(&line, "=");
		if (token == NULL) {
			ret = -1;
			goto parsing_fail;
		}

		/* rts_event_id, */
		token = strsep(&line, ";");
		if (token == NULL) {
			ret = -1;
			goto parsing_fail;
		}

		res = kstrtoint(token, 10, &rts_event_id);
		if (rts_event_id >= 0) {
			memcpy(&old_data, &met_evnet_header[rts_event_id], sizeof(struct sspm_met_evnet_header));
			met_evnet_header[rts_event_id].rts_event_id = rts_event_id;

			/* rts_event_name */
			token = strsep(&line, ";");
			if (token == NULL) {
				ret = -1;
				reset_data = 1;
				goto parsing_fail;
			}

			met_evnet_header[rts_event_id].rts_event_name = token;

			/* chart_line_name */
			token = strsep(&line, ";");
			if (token == NULL) {
				ret = -1;
				reset_data = 1;
				goto parsing_fail;
			}

			met_evnet_header[rts_event_id].chart_line_name = token;

			/* key_list */
			token = strsep(&line, ";\n");
			if (token == NULL) {
				ret = -1;
				reset_data = 1;
				goto parsing_fail;
			}

			met_evnet_header[rts_event_id].key_list = token;

			update_rts_event_tbl[rts_event_id] = ptr;
		}
	}

	if (rts_event_id >= 0)
		update_event_id_flag(rts_event_id);

	return ret;

parsing_fail:
	if (line)
		kfree(line);

	if (reset_data)
		memcpy(&met_evnet_header[rts_event_id], &old_data, sizeof(struct sspm_met_evnet_header));

	return ret;
}
EXPORT_SYMBOL(met_sspm_common);
