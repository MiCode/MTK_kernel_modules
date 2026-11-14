// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
/*****************************************************************************
 * headers
 *****************************************************************************/
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/of.h>

#include "met_drv.h"
#include "mtk_typedefs.h"
#include "tinysys_mcupm.h"
#include "tinysys_mgr.h"

#include "mcupm_met_log.h"
#include "mcupm_met_ipi_handle.h" /* for met_ipi_to_mcupm_command */

#define MAX_KEYLIST_LEN 1024
/*****************************************************************************
 * struct & enum declaration
 *****************************************************************************/
struct mcupm_met_event_header {
	unsigned int rts_event_id;
	const char *rts_event_name;
	const char *chart_line_name;
	char key_list[MAX_KEYLIST_LEN];
};


/*****************************************************************************
 * internal function declaration
 *****************************************************************************/
static void ondiemet_mcupm_start(void);
static void ondiemet_mcupm_stop(void);
static int ondiemet_mcupm_print_help(char *buf, int len);
static int ondiemet_mcupm_process_argument(const char *arg, int len);
static int ondiemet_mcupm_print_header(char *buf, int len);


/*****************************************************************************
 * external function declaration
 *****************************************************************************/
extern int *mcupm_buf_available;
extern unsigned int mcupm_count;


/*****************************************************************************
 * internal variable
 *****************************************************************************/
#define NR_RTS_STR_ARRAY 50
#define NR_RTS_STR_MIN_SIZE 2
#define MXNR_NODE_NAME 32
#define MXNR_EVENT_NAME 64
#define MAX_MET_RTS_EVENT_NUM 128
static unsigned int event_id_flag[MAX_MET_RTS_EVENT_NUM / 32];
static struct mcupm_met_event_header met_event_header[MAX_MET_RTS_EVENT_NUM];
static char mcupm_help[] = "  --mcupm_common=rts_event_name\n";
static char header[] = 	"met-info [000] 0.0: mcupm_common_header: ";
static bool met_event_header_updated = false;
static int nr_dts_header = 0;
static int nr_mcupm_list = 0;
static int mcupm_list_to_send[32] = {};

struct metdevice met_mcupm_common = {
	.name = "mcupm_common",
	.owner = THIS_MODULE,
	.type = MET_TYPE_BUS,
	.cpu_related = 0,
	.ondiemet_mode = 1,
	.ondiemet_start = ondiemet_mcupm_start,
	.ondiemet_stop = ondiemet_mcupm_stop,
	.ondiemet_process_argument = ondiemet_mcupm_process_argument,
	.ondiemet_print_help = ondiemet_mcupm_print_help,
	.ondiemet_print_header = ondiemet_mcupm_print_header,
};


/*****************************************************************************
 * internal function implement
 *****************************************************************************/
static int ondiemet_mcupm_print_help(char *buf, int len)
{
	return snprintf(buf, PAGE_SIZE, mcupm_help);
}


static int get_rts_header_from_dts_table(struct mcupm_met_event_header* __met_event_header)
{
	int idx,key_list_idx;
	struct device_node *np;
	const char *rts_string[NR_RTS_STR_ARRAY] = {};
	int nr_str = 0;
	int ret = 0;

	/*get rts root node*/
	np = of_find_node_by_name(NULL, "mcupm-rts-header");
	if (!np) {
		pr_debug("unable to find mcupm-rts-header\n");
		return 0;
	}

	/*get string array*/
	for (idx = 0; idx < MAX_MET_RTS_EVENT_NUM; idx++) {
		char node_name[MXNR_NODE_NAME];
		SNPRINTF(node_name, sizeof(node_name), "node-%d", idx);
		nr_str = of_property_read_string_array(np,
				node_name, &rts_string[0], NR_RTS_STR_ARRAY);
		if (nr_str < NR_RTS_STR_MIN_SIZE)
			break;
		__met_event_header[idx].rts_event_id = idx;
		__met_event_header[idx].rts_event_name = rts_string[0];
		__met_event_header[idx].chart_line_name = rts_string[0];
		__met_event_header[idx].key_list[0] = '\0'; /*clear the string buf*/
		ret = 0;
		for (key_list_idx=1; key_list_idx<nr_str; key_list_idx++)
		{
			ret += snprintf(__met_event_header[idx].key_list + ret, MAX_KEYLIST_LEN - ret,
				"%s", rts_string[key_list_idx]);
		}
		pr_debug("__met_event_header[%d] rts_event_name[%s] key_list[%s] \n",
			idx,
			__met_event_header[idx].rts_event_name,
			__met_event_header[idx].key_list);
	}
	return idx;
}


static int ondiemet_mcupm_print_header(char *buf, int len)
{
	int i;
	int write_len;
	int flag = 0;
	int mask = 0;
	int group;
	static int is_dump_header = 0;
	static int read_idx = 0;

	len = 0;
	met_mcupm_common.header_read_again = 0;
	if (is_dump_header == 0) {
		len = SNPRINTF(buf, PAGE_SIZE, "%s", header);
		is_dump_header = 1;
	}

	for (i = read_idx; i < nr_dts_header; i++) {
		if (met_event_header[i].chart_line_name) {
			group = i / 32;
			mask = 1 << (i - group * 32);
			flag = event_id_flag[group] & mask;
			if (flag == 0) {
				continue;
			}

			write_len = strlen(met_event_header[i].chart_line_name) + strlen(met_event_header[i].key_list) + 3;
			if ((len + write_len) < PAGE_SIZE) {
				len += snprintf(buf+len, PAGE_SIZE-len, "%u,%s,%s;",
					met_event_header[i].rts_event_id,
					met_event_header[i].chart_line_name,
					met_event_header[i].key_list);
			} else {
				met_mcupm_common.header_read_again = 1;
				read_idx = i;
				return len;
			}
		}
	}

	if (i == nr_dts_header) {
		is_dump_header = 0;
		read_idx = 0;
		buf[len - 1] = '\n';
		met_mcupm_common.mode = 0;
		for (i = 0 ; i < MAX_MET_RTS_EVENT_NUM / 32; i++) {
			event_id_flag[i] = 0;
		}
	}

	return len;
}


static void ondiemet_mcupm_start(void)
{
	unsigned int mcupm_no = 0;

	for (mcupm_no = 0; mcupm_no < mcupm_count; mcupm_no++) {
		if (mcupm_buffer_size[mcupm_no] == 0) {
			return;
		}
	}

	ondiemet_module[ONDIEMET_MCUPM] |= ID_COMMON;
}


static void ondiemet_mcupm_stop(void)
{
	return;
}


static void update_event_id_flag(int event_id)
{
	unsigned int ipi_buf[4] = {0};
	unsigned int rdata = 0;
	unsigned int ret = 0;
	unsigned int group = 0;
	unsigned int mcupm_no = 0;
	int i = 0;

	group = event_id / 32;
	event_id_flag[group] |= 1 << (event_id - group * 32);
	ipi_buf[0] = MET_MAIN_ID | MET_ARGU | MID_COMMON<<MID_BIT_SHIFT | 1;
	ipi_buf[1] = group;
	ipi_buf[2] = event_id_flag[group];
	ipi_buf[3] = 0;

	for (i = 0; i < mcupm_count; i++) {
		if (nr_mcupm_list != 0)	{// Send event to specific MCUPMs
			if (mcupm_list_to_send[i] == -1)
				break;
			mcupm_no = mcupm_list_to_send[i];
		} else {// Broadcate event to all MCUPMs
			mcupm_no = i;
		}

		if (mcupm_buffer_size[mcupm_no] == 0)
			return ;
		if (mcupm_buf_available[mcupm_no] == 1) {
			ret = met_ipi_to_mcupm_command(mcupm_no, (void *)ipi_buf, 0, &rdata, 1);
		}
	}

	met_mcupm_common.mode = 1;
}

static int met_parse_num_list(char *arg, int len, int *list, int list_cnt)
{
	int	nr_num = 0;
	char	*num;
	int	num_len;
	int	ret;

	/* search ',' as the splitter */
	while (len) {
		num = arg;
		num_len = 0;
		if (list_cnt <= 0)
			return -1;

		while (len) {
			len--;
			if (*arg == ',') {
				*(arg++) = '\0';
				break;
			}
			arg++;
			num_len++;
		}

		ret = kstrtoint(num, 10, list);

		list++;
		list_cnt--;
		nr_num++;
	}

	return nr_num;
}

static int ondiemet_mcupm_process_argument(const char *arg, int len)
{
	int i = 0;
	int rts_event_id = -1;
	char *arg1 = (char*)arg;
	int len1 = len;
	nr_mcupm_list = 0;
	memset(mcupm_list_to_send, 0xFF, sizeof(mcupm_list_to_send));

	/*
	 * split cpu_list and event_list by ':'
	 *   arg, len: cpu_list when found (i < len)
	 *   arg1, len1: event_list
	 */
	for (i = 0; i < len; i++) {
		if (arg[i] == ':') {
			arg1[i] = '\0';
			arg1 += i+1;
			len1 = len - i - 1;
			len = i;
			break;
		}
	}

	if (arg1 != arg) {
		nr_mcupm_list = met_parse_num_list((char*)arg, len, mcupm_list_to_send, mcupm_count);
	} else {
		nr_mcupm_list = 0;
	}

	if (!met_event_header_updated) {
		nr_dts_header = get_rts_header_from_dts_table(met_event_header);
		met_event_header_updated = true;
	}

	for (i = 0; met_event_header[i].rts_event_name && i < nr_dts_header; i++) {
		if (strncmp(met_event_header[i].rts_event_name, arg1, MXNR_EVENT_NAME) == 0) {
			rts_event_id = i;
			break;
		}
	}

	if (rts_event_id >= 0) {
		update_event_id_flag(rts_event_id);
	}

	return 0;
}
EXPORT_SYMBOL(met_mcupm_common);
