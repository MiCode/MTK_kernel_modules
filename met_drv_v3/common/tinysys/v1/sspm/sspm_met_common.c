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
#include "core_plf_init.h"
#include "mtk_typedefs.h"
#include "tinysys_sspm.h"
#include "tinysys_mgr.h"

#include "sspm_met_log.h"
#include "sspm_met_ipi_handle.h" /* for met_ipi_to_sspm_command */

#define MAX_KEYLIST_LEN 1024


/*****************************************************************************
 * struct & enum declaration
 *****************************************************************************/
struct sspm_met_event_header {
	unsigned int rts_event_id;
	const char *rts_event_name;
	const char *chart_line_name;
	char key_list[MAX_KEYLIST_LEN];
	int enabled_virt_idx;
};


/*****************************************************************************
 * internal function declaration
 *****************************************************************************/
static void ondiemet_sspm_start(void);
static void ondiemet_sspm_stop(void);
static int ondiemet_sspm_print_help(char *buf, int len);
static int ondiemet_sspm_process_argument(const char *arg, int len);
static int ondiemet_sspm_print_header(char *buf, int len);


/*****************************************************************************
 * external function declaration
 *****************************************************************************/


/*****************************************************************************
 * internal variable
 *****************************************************************************/
#define NR_RTS_STR_ARRAY 50
#define NR_RTS_STR_MIN_SIZE 2
#define MXNR_NODE_NAME 32
#define MXNR_EVENT_NAME 64
#define MAX_MET_RTS_EVENT_ENABLED_NUM 128
static unsigned int MAX_MET_RTS_EVENT_NUM = 0;
static unsigned int *event_id_flag;
static struct sspm_met_event_header *met_event_header;
static char sspm_help[] = "  --sspm_common=rts_event_name\n";
static char header[] = 	"met-info [000] 0.0: sspm_common_header: ";
static bool met_event_header_updated = false;
static int nr_dts_header = 0;
static unsigned int event_enabled_cnt = 0;

MET_DEFINE_DEPENDENCY_BY_NAME(dependencies) = {
	{.symbol=(void**)&met_scmi_api_ready, .init_once=0, .cpu_related=0, .ondiemet_mode=1, .tinysys_type=0},
	{.symbol=(void**)&met_sspm_api_ready, .init_once=0, .cpu_related=0, .ondiemet_mode=1, .tinysys_type=0},
};

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
	MET_DEFINE_METDEVICE_DEPENDENCY_BY_NAME(dependencies)
};


/*****************************************************************************
 * internal function implement
 *****************************************************************************/
static int ondiemet_sspm_print_help(char *buf, int len)
{
	return SNPRINTF(buf, PAGE_SIZE, sspm_help);
}


static int get_rts_header_from_dts_table(struct sspm_met_event_header* __met_event_header)
{
	int idx, key_list_idx;
	struct device_node *np;
	const char *rts_string[NR_RTS_STR_ARRAY] = {};
	int nr_str = 0;
	int ret = 0;

	/*get rts root node*/
	np = of_find_node_by_name(NULL, "sspm-rts-header");
	if (!np) {
		pr_debug("unable to find sspm-rts-header\n");
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
		__met_event_header[idx].enabled_virt_idx = -1;

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


static int ondiemet_sspm_print_header(char *buf, int len)
{
	int i;
	int write_len;
	int flag = 0;
	int mask = 0;
	int group;
	static int is_dump_header = 0;
	static int read_idx = 0;

	len = 0;
	met_sspm_common.header_read_again = 0;
	if (is_dump_header == 0) {
		len = SNPRINTF(buf, PAGE_SIZE, "%s", header);
		is_dump_header = 1;
	}

	for (i = read_idx; i < nr_dts_header; i++) {
		if (met_event_header[i].chart_line_name) {

            if (_is_new_RTS_mode == 1)
            {
                if (met_event_header[i].enabled_virt_idx == -1)
                {
                    continue;
                }
            }
            else
            {
                group = i / 32;
                mask = 1 << (i - group * 32);
                flag = event_id_flag[group] & mask;
                if (flag == 0)
                {
                    continue;
                }
            }

            write_len = strlen(met_event_header[i].chart_line_name) + strlen(met_event_header[i].key_list) + 3;
            if ((len + write_len) < PAGE_SIZE) 
            {
                if (_is_new_RTS_mode == 1)
                {
                        len += SNPRINTF(buf+len, PAGE_SIZE-len, "%u,%s,%s;",
                        met_event_header[i].enabled_virt_idx,
                        met_event_header[i].chart_line_name,
                        met_event_header[i].key_list);
                }
                else
                {
                    len += SNPRINTF(buf+len, PAGE_SIZE-len, "%u,%s,%s;",
                        met_event_header[i].rts_event_id,
                        met_event_header[i].chart_line_name,
                        met_event_header[i].key_list);
                }
            } 
            else
			{
				met_sspm_common.header_read_again = 1;
				read_idx = i;
				return len;
			}
		}
	}

	if (i == nr_dts_header) {
		is_dump_header = 0;
		read_idx = 0;
		buf[len - 1] = '\n';
		met_sspm_common.mode = 0;

        if (_is_new_RTS_mode == 1)
        {
            for (i = 0; i < MAX_MET_RTS_EVENT_NUM; i++)
            {
                met_event_header[i].enabled_virt_idx = -1;
            }
            event_enabled_cnt = 0;
        }
        else
        {
            for (i = 0 ; i < MAX_MET_RTS_EVENT_NUM / 32; i++)
            {
                event_id_flag[i] = 0;
            }
        }
	}

	return len;
}


static void ondiemet_sspm_start(void)
{
	if (sspm_buffer_size == 0) {
		return;
	}

	ondiemet_module[ONDIEMET_SSPM] |= ID_COMMON;
}


static void ondiemet_sspm_stop(void)
{
	return;
}

static void update_event_id_flag(int event_id)
{
	unsigned int ipi_buf[4] = {0, 0, 0, 0};
	unsigned int rdata = 0;
	unsigned int res = 0;
	unsigned int group = 0;

	if (sspm_buffer_size == 0)
		return ;

	group = event_id / 32;
	event_id_flag[group] |= 1 << (event_id - group * 32);
	ipi_buf[0] = MET_MAIN_ID | MET_ARGU | MID_COMMON<<MID_BIT_SHIFT | 1;
	ipi_buf[1] = group;
	ipi_buf[2] = event_id_flag[group];

#ifdef MET_SCMI
	res = met_scmi_to_sspm_command((void *)ipi_buf, sizeof(ipi_buf)/sizeof(unsigned int), &rdata, 1);
#else
	res = met_ipi_to_sspm_command((void *)ipi_buf, 0, &rdata, 1);
#endif
	met_sspm_common.mode = 1;
}


#include "linux/crypto.h"
#include "crypto/hash.h"

#define SHA1_LENGTH 20
int calculate_sha1_first_4_bytes(const char *str, unsigned int *result) 
{
	unsigned char hash[SHA1_LENGTH];
	struct crypto_shash *tfm;
	struct shash_desc *desc;
	int err;
//	int i = 0;

	tfm = crypto_alloc_shash("sha1", 0, CRYPTO_ALG_ASYNC);
	if (IS_ERR(tfm)) {
		pr_debug(KERN_ERR "Failed to allocate hash transform\n");
		return PTR_ERR(tfm);
	}

	desc = kmalloc(sizeof(*desc) + crypto_shash_descsize(tfm), GFP_KERNEL);
	if (!desc) {
		pr_debug(KERN_ERR "Failed to allocate hash descriptor\n");
		crypto_free_shash(tfm);
		return -ENOMEM;
	}

	desc->tfm = tfm;

	err = crypto_shash_init(desc);
	if (err) {
		pr_debug(KERN_ERR "Failed to initialize hash\n");
		goto err_out;
	}

	err = crypto_shash_update(desc, str, strlen(str));
	if (err) {
		pr_debug(KERN_ERR "Failed to update hash\n");
		goto err_out;
	}

	err = crypto_shash_final(desc, hash);
	if (err) {
		pr_debug(KERN_ERR "Failed to finalize hash\n");
		goto err_out;
	}

	*result = ((unsigned int)hash[0] << 24) + ((unsigned int)hash[1] << 16) + ((unsigned int)hash[2] << 8) + (unsigned int)hash[3];

	kfree(desc);
	crypto_free_shash(tfm);

	return 0;

err_out:
	kfree(desc);
	crypto_free_shash(tfm);
	return err;
}

static void update_SHA1_code_and_virt_id(unsigned int event_name_sha1, int enabled_virt_idx)
{	
	unsigned int ipi_buf[3] = {0, 0, 0};
	unsigned int rdata = 0;
	unsigned int res = 0;

	if (sspm_buffer_size == 0)
		return ;

	ipi_buf[0] = MET_MAIN_ID | MET_ARGU | MID_COMMON<<MID_BIT_SHIFT | 1;
	ipi_buf[1] = enabled_virt_idx;
	ipi_buf[2] = event_name_sha1;

#ifdef MET_SCMI
	res = met_scmi_to_sspm_command((void *)ipi_buf, sizeof(ipi_buf)/sizeof(unsigned int), &rdata, 1);
#else
	res = met_ipi_to_sspm_command((void *)ipi_buf, 0, &rdata, 1);
#endif
	met_sspm_common.mode = 1;
}

static int ondiemet_sspm_process_argument(const char *arg, int len)
{
	int i = 0;
    unsigned int event_name_sha1 = 0;
    unsigned int res = 0;

    if (!met_event_header_updated) 
    {
        met_event_header_updated = true;
        
        if (_is_new_RTS_mode == 1)
        {
            MAX_MET_RTS_EVENT_NUM = 512;
        }
        else
        {
            MAX_MET_RTS_EVENT_NUM = 128;
        }
        met_event_header = kmalloc_array(MAX_MET_RTS_EVENT_NUM, sizeof(struct sspm_met_event_header), GFP_KERNEL);
        event_id_flag = kmalloc_array(MAX_MET_RTS_EVENT_NUM / 32, sizeof(unsigned int), GFP_KERNEL);

        nr_dts_header = get_rts_header_from_dts_table(met_event_header);

        if (nr_dts_header > MAX_MET_RTS_EVENT_NUM)
        {
            pr_debug(KERN_ERR "The number of MET RTS event in dts (%d) is over the limit(%d)\n",
                    nr_dts_header,
                    MAX_MET_RTS_EVENT_NUM);
            return -EINVAL;
        }
    }

    if (event_enabled_cnt > MAX_MET_RTS_EVENT_ENABLED_NUM)
    {
        pr_debug(KERN_ERR "The number of enabled MET RTS event in mtxxxx.sh (%d) is over the limit(%d)\n",
                event_enabled_cnt,
                MAX_MET_RTS_EVENT_ENABLED_NUM);
        return -EINVAL;
    }

	for (i = 0; i < nr_dts_header; i++) {
		if(met_event_header[i].rts_event_name) {
			if (strncmp(met_event_header[i].rts_event_name, arg, MXNR_EVENT_NAME) == 0) {
				if (_is_new_RTS_mode == 1) {
					if (met_event_header[i].enabled_virt_idx != -1) {
						pr_debug("\x1b[1;31m ==> event-- %s has been enabled just now! \033[0m\n", 
								met_event_header[i].rts_event_name);
						break;
					}
					met_event_header[i].enabled_virt_idx = event_enabled_cnt;
					event_enabled_cnt++;

					res = calculate_sha1_first_4_bytes(met_event_header[i].rts_event_name, &event_name_sha1);
					pr_debug("\x1b[1;33m ==> event-- %s, SHA1: 0x%08x \033[0m\n", 
							met_event_header[i].rts_event_name, event_name_sha1);
					update_SHA1_code_and_virt_id(event_name_sha1, met_event_header[i].enabled_virt_idx);
				}
				else {
					update_event_id_flag(i);
				}
				break;
			}
		}
	}


	return 0;
}
EXPORT_SYMBOL(met_sspm_common);
