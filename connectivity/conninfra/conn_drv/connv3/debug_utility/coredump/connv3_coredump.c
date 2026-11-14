// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/time64.h>
#include <linux/vmalloc.h>

#include "conndump_netlink.h"
#include "connv3_coredump.h"
#include "connv3_debug_utility.h"
#include "connv3_dump_mng.h"
#include "connv3.h"
#include "osal.h"
#include "osal_dbg.h"

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

enum connv3_coredump_state {
	CONNV3_COREDUMP_STATE_INIT = 0,
	CONNV3_COREDUMP_STATE_START,
	CONNV3_COREDUMP_STATE_MEM_REGION,
	CONNV3_COREDUMP_STATE_EMI,		/* EMI dump is triggerred */
	CONNV3_COREDUMP_STATE_EMI_DONE,		/* EMI dump end successfully */
	CONNV3_COREDUMP_STATE_EMI_TIMEOUT,	/* EMI dump timeout */
	CONNV3_COREDUMP_STATE_END,		/* COREDUMP_END command is sent */
	CONNV3_COREDUMP_STATE_END_DONE,		/* Coredump is end succesfully */
	CONNV3_COREDUMP_STATE_END_TIMEOUT,	/* Coredump end command timeout */
	CONNV3_COREDUMP_STATE_MAX, /* should not reach */
};

struct connv3_dump_ctx {
	int conn_type;
	atomic_t state;
	struct connv3_coredump_event_cb cb;
	struct connv3_issue_info issue_info;
	struct completion emi_dump;
	OSAL_SLEEPABLE_LOCK ctx_lock;
};

static atomic_t g_dump_mode = ATOMIC_INIT(CONNV3_DUMP_MODE_DAEMON);
static atomic_t g_memdump_mode = ATOMIC_INIT(0);

static const char* g_type_name[] = {
	"Wi-Fi",
	"BT",
};

struct timespec64 g_dump_start_time;

/*******************************************************************************
*                             MACROS
********************************************************************************
*/
#if defined(CONFIG_FPGA_EARLY_PORTING)
/* For FPGA shorten the timer */
#define CONNV3_EMIDUMP_TIMEOUT		(10*1000)
#else /* defined(CONFIG_FPGA_EARLY_PORTING) */
#define CONNV3_EMIDUMP_TIMEOUT		(60*1000)
#endif

#define EMI_COMMAND_LENGTH	128
/* For MD log packaging */
#define INFO_HEAD ";CONSYS FW CORE,"

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
static unsigned int connv3_coredump_get_memdump_mode(void);

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

static unsigned long timespec64_to_ms(struct timespec64 *begin, struct timespec64 *end)
{
	unsigned long time_diff;

	time_diff = (end->tv_sec - begin->tv_sec) * MSEC_PER_SEC;
	time_diff += (end->tv_nsec - begin->tv_nsec) / NSEC_PER_MSEC;

	return time_diff;
}

static void* connv3_dump_malloc(unsigned int size)
{
	void* p = NULL;

	if (size > (PAGE_SIZE << 1))
		p = vmalloc(size);
	else
		p = kmalloc(size, GFP_KERNEL);

	/* If there is fragment, kmalloc may not get memory when size > one page.
	 * For this case, use vmalloc instead.
	 */
	if (p == NULL && size > PAGE_SIZE)
		p = vmalloc(size);
	return p;
}

static void connv3_dump_free(const void* dst)
{
	kvfree(dst);
}

static const char* connv3_dump_get_type_name(int type)
{
	if (type < CONNV3_DEBUG_TYPE_WIFI || type >= CONNV3_DEBUG_TYPE_SIZE)
		return "TYPE_INVALID";
	return g_type_name[type];
}

static void connv3_dump_set_dump_state(
	struct connv3_dump_ctx *ctx, enum connv3_coredump_state state)
{
	if (ctx)
		atomic_set(&ctx->state, state);
	else
		pr_err("[%s] ctx is null", __func__);
}

static enum connv3_coredump_state connv3_dump_get_dump_state(struct connv3_dump_ctx* ctx)
{
	if (ctx)
		return (enum connv3_coredump_state)atomic_read(&ctx->state);
	pr_err("%s ctx is null\n", __func__);
	return CONNV3_COREDUMP_STATE_MAX;
}

static void connv3_dump_emi_dump_end(void* handler)
{
	struct connv3_dump_ctx* ctx = (struct connv3_dump_ctx*)handler;
	enum connv3_coredump_state state = connv3_dump_get_dump_state(ctx);

	if (state == CONNV3_COREDUMP_STATE_EMI || state == CONNV3_COREDUMP_STATE_END) {
		pr_info("Wake up end command\n");
		complete(&ctx->emi_dump);
	}
}

static void connv3_dump_get_save_emi(void *dump_ctx, phys_addr_t *base,
	size_t *size)
{
	phys_addr_t save_emi_base = 0;
	size_t save_emi_size = 0;
	unsigned int mode = 0;
	struct connv3_dump_ctx* ctx = (struct connv3_dump_ctx*)dump_ctx;

	mode = connv3_coredump_get_memdump_mode();
	pr_info("[%s] memdump mode=[%d]\n", __func__, mode);

	if (mode == CONNV3_COREDUMP_SAVE_TO_EMI && ctx && ctx->cb.get_save_emi)
		ctx->cb.get_save_emi(&save_emi_base, &save_emi_size);

	if (base)
		*base = save_emi_base;
	if (size)
		*size = save_emi_size;
}

static int connv3_coredump_info_analysis(
	struct connv3_dump_ctx *ctx,
	const int drv, const char *reason, const char* dump_msg)
{
	char* task_drv_name[CONNV3_DRV_TYPE_MAX] = {
		"Task_DrvBT",
		"Task_DrvWifi",
		"Task_DrvMODEM",
		"Task_DrvConnv3",
	};
	const char* parser_sub_string[] = {
		"<ASSERT> ",
		"id=",
		"isr=",
		"irq=",
	};
	const char* exception_sub_string[] = {
		"<EXCEPTION> ",
		"ipc=",
		"eva=",
		"itype=",
		"etype:",
	};
	char *pStr = (char*)dump_msg;
	char *pDtr = NULL;
	char *pTemp = NULL;
	char *pTemp2 = NULL;
	unsigned int len;
	int remain_array_len = 0, sec_len = 0, idx = 0;
	char tempBuf[CONNV3_ASSERT_TYPE_SIZE] = { '\0' };
	int ret, res;
	char *type_str;
	char *task_end;

	/* reset */
	memset(&ctx->issue_info, 0, sizeof(struct connv3_issue_info));

	if (ctx->conn_type < 0 || ctx->conn_type >= CONNV3_DEBUG_TYPE_SIZE) {
		pr_notice("[%s] conn_type(%d) is not supported", __func__, ctx->conn_type);
		return CONNV3_COREDUMP_ERR_INVALID_INPUT;
	}

	/* Check force dump */
	if (strncmp(dump_msg, CONNV3_COREDUMP_FORCE_DUMP, strlen(CONNV3_COREDUMP_FORCE_DUMP)) == 0) {
		pr_info("[%s] %s", connv3_dump_get_type_name(ctx->conn_type), CONNV3_COREDUMP_FORCE_DUMP);
		ctx->issue_info.issue_type = CONNV3_ISSUE_FORCE_DUMP;
		goto check_driver_assert;
	}

	/* Check <ASSERT> first */
	/* Assert example:
	 * <ASSERT> system/rom/common/cos_task_rom.c #377 - , 0x0, 0x0, 0x0, BTSYS, id=0x0 Tmr Svc, isr=0x8008CE, irq=0xC, lr=0x8011CE, asr_t=273125
	 * tranfer assert message to
	 * "assert@system/rom/common/cos_task_rom.c-#377"
	 */
	memset(&ctx->issue_info.assert_info[0], '\0', CONNV3_ASSERT_INFO_SIZE);
	type_str = (char*)parser_sub_string[0];
	pDtr = strstr(pStr, type_str);
	/* Find "<ASSERT> " */
	if (pDtr != NULL) {
		pDtr += strlen(type_str);
		pTemp = strchr(pDtr, ' ');
		/* Find assert file */
		if (pTemp != NULL) {
			idx = 0;
			remain_array_len = CONNV3_ASSERT_INFO_SIZE -1;
			sec_len = strlen("assert@");
			memcpy(&ctx->issue_info.assert_info[0], "assert@", sec_len);
			idx += sec_len;
			remain_array_len -= sec_len;

			/* Copy assert file */
			len = pTemp - pDtr;
			sec_len = (len < remain_array_len ? len : remain_array_len);
			memcpy(&ctx->issue_info.assert_info[idx], pDtr, sec_len);
			remain_array_len -= sec_len;
			idx += sec_len;

			/* Add "-" */
			sec_len = strlen("_");
			if (sec_len > remain_array_len)
				goto check_task_str;
			ctx->issue_info.assert_info[idx] = '_';
			remain_array_len -= sec_len;
			idx += sec_len;

			/* Try to find */
			pTemp = strchr(pDtr, '#');
			if (pTemp != NULL) {
				pTemp += 1;
				pTemp2 = strchr(pTemp, ' ');
				if (pTemp2 == NULL) {
					pTemp2 = pTemp + 1;
				}
				if ((remain_array_len) > (pTemp2 - pTemp)) {
					memcpy(
						&ctx->issue_info.assert_info[idx],
						pTemp,
						pTemp2 - pTemp);
				} else {
					memcpy(
						&ctx->issue_info.assert_info[idx],
						pTemp,
						remain_array_len);
				}
			}
			pr_info("assert info:%s\n", ctx->issue_info.assert_info);

		}
	} else {
		/* Check exception */
		type_str = (char*)exception_sub_string[0];
		pDtr = strstr(pStr, type_str);
		if (pDtr == NULL)
			goto check_task_str;

		ctx->issue_info.issue_type = CONNV3_ISSUE_FW_EXCEPTION;
		idx = 0;
		remain_array_len = CONNV3_ASSERT_INFO_SIZE;
		sec_len = snprintf(&ctx->issue_info.assert_info[idx], remain_array_len,
			"%s", "Exception:");
		if (sec_len > 0) {
			remain_array_len -= sec_len;
			idx += sec_len;
		}

		/* Check ipc */
		type_str = (char*)exception_sub_string[1];
		pDtr = strstr(pDtr, type_str);
		if (pDtr == NULL) {
			pr_notice("Exception substring (%s) not found\n", type_str);
			goto check_task_str;
		}
		pDtr += strlen(type_str);
		pTemp = strchr(pDtr, ',');
		if (pTemp != NULL) {
			unsigned int exp_ipc = 0;
			len = pTemp - pDtr;
			len = (len >= CONNV3_ASSERT_TYPE_SIZE) ? CONNV3_ASSERT_TYPE_SIZE - 1 : len;
			memcpy(&tempBuf[0], pDtr, len);
			tempBuf[len] = '\0';
			ret = kstrtouint(tempBuf, 16, &res);
			if (ret) {
				pr_notice("Convert to uint fail, ret=%d, buf=%s\n",
					ret, tempBuf);
			} else {
				exp_ipc = res;
				pr_info("exp_ipc=0x%x\n", exp_ipc);
				if (remain_array_len > 0) {
					sec_len = snprintf(&ctx->issue_info.assert_info[idx], remain_array_len,
						" ipc=0x%x", exp_ipc);
					if (sec_len > 0) {
						remain_array_len -= sec_len;
						idx += sec_len;
					}
				}
			}
		}

		/* Check eva */
		type_str = (char*)exception_sub_string[2];
		pDtr = strstr(pDtr, type_str);
		if (pDtr == NULL) {
			pr_notice("substring (%s) not found\n", type_str);
			goto check_task_str;
		}
		pDtr += strlen(type_str);
		pTemp = strchr(pDtr, ',');
		if (pTemp != NULL) {
			unsigned int exp_eva = 0;
			len = pTemp - pDtr;
			len = (len >= CONNV3_ASSERT_TYPE_SIZE) ? CONNV3_ASSERT_TYPE_SIZE - 1 : len;
			memcpy(&tempBuf[0], pDtr, len);
			tempBuf[len] = '\0';
			ret = kstrtouint(tempBuf, 16, &res);
			if (ret) {
				pr_notice("Convert to uint fail, ret=%d, buf=%s\n",
					ret, tempBuf);
			} else {
				exp_eva = res;
				pr_info("eva addr=0x%x\n", exp_eva);
				if (remain_array_len > 0) {
					sec_len = snprintf(
						&ctx->issue_info.assert_info[idx], remain_array_len,
						" eva=0x%x", exp_eva);
					if (sec_len > 0) {
						remain_array_len -= sec_len;
						idx += sec_len;
					}
				}
			}
		}

		/* Check itype */
		type_str = (char*)exception_sub_string[3];
		pTemp2 = strstr(pDtr, type_str);
		if (pTemp2 == NULL) {
			pr_notice("Substring(%s) not found\n", type_str);
		} else {
			pDtr = pTemp2;
			pDtr += strlen(type_str);
			pTemp = strchr(pDtr, ',');
			if (pTemp != NULL) {
				char itype_str[CONNV3_ASSERT_INFO_SIZE] = {'\0'};
				len = pTemp - pDtr;
				len = (len >= CONNV3_ASSERT_TYPE_SIZE) ? CONNV3_ASSERT_TYPE_SIZE - 1 : len;
				memcpy(&itype_str, pDtr, len);
				itype_str[len] = '\0';
				pr_info("itype=%s\n", itype_str);
				if (remain_array_len > 0) {
					sec_len = snprintf(
						&ctx->issue_info.assert_info[idx], remain_array_len,
						" itype=%s", itype_str);
					if (sec_len > 0) {
						remain_array_len -= sec_len;
						idx += sec_len;
					}
				}
			}
		}

		/* Check etype */
		type_str = (char*)exception_sub_string[4];
		pDtr = strstr(pDtr, type_str);
		if (pDtr == NULL) {
			pr_notice("Substring(%s) not found\n", type_str);
			goto check_task_str;
		}
		pDtr += strlen(type_str);
		pTemp = strchr(pDtr, ',');
		if (pTemp != NULL) {
			char etype[CONNV3_ASSERT_INFO_SIZE] = {'\0'};
			len = pTemp - pDtr;
			len = (len >= CONNV3_ASSERT_TYPE_SIZE) ? CONNV3_ASSERT_TYPE_SIZE - 1 : len;
			memcpy(&etype, pDtr, len);
			etype[len] = '\0';
			pr_info("etype=%s\n", etype);
			if (remain_array_len > 0) {
				sec_len = snprintf(
					&ctx->issue_info.assert_info[idx], remain_array_len,
					" etype=%s", etype);
				if (sec_len > 0) {
					remain_array_len -= sec_len;
					idx += sec_len;
				}
			}
		}
	} /* FW exception */

check_task_str:
	/* Example: "id=0x0 Tmr Svc," */
	type_str = (char*)parser_sub_string[1];
	pDtr = strstr(pStr, type_str);
	if (pDtr == NULL) {
		pr_notice("parser str is NULL,substring(%s)\n", type_str);
	} else {
		pDtr += strlen(type_str);
		pTemp = strchr(pDtr, ' ') + 1;

		if (pTemp == NULL) {
			pr_notice("delimiter( ) is not found,substring(%s)\n",
				type_str);
		} else {
			task_end = strchr(pTemp, ',');
			if (task_end != NULL) {
				unsigned int task_len = ((task_end - pTemp) > CONNV3_TASK_NAME_SIZE - 1 ? CONNV3_TASK_NAME_SIZE - 1 : (task_end - pTemp));
				char task_name[CONNV3_TASK_NAME_SIZE];
				strncpy(task_name, pTemp, task_len);
				task_name[task_len] = '\0';

				if (snprintf(ctx->issue_info.task_name, CONNV3_TASK_NAME_SIZE,
					"Task_%s_%s", task_name,
					connv3_dump_mng_get_subsys_tag(ctx->conn_type)) < 0)
					pr_notice("%s snprintf failed\n", __func__);
			}
		}
	}

	/* Check ISR */
	ctx->issue_info.fw_isr = 0;
	type_str = (char*)parser_sub_string[2];
	pDtr = strstr(pStr, type_str);
	if (pDtr == NULL) {
		pr_notice("parser str is NULL,substring(%s)\n", type_str);
	} else {
		pDtr += strlen(type_str);
		pTemp = strchr(pDtr, ',');

		if (pTemp == NULL) {
			pr_notice("delimiter(,) is not found,substring(%s)\n", type_str);
		} else {
			len = pTemp - pDtr;
			len = (len >= CONNV3_ASSERT_TYPE_SIZE) ? CONNV3_ASSERT_TYPE_SIZE - 1 : len;
			memcpy(&tempBuf[0], pDtr, len);
			tempBuf[len] = '\0';

			ret = kstrtouint(tempBuf, 16, &res);
			if (ret) {
				pr_notice("Get fw isr id fail, ret=%d, buf=%s\n", ret, tempBuf);
			} else {
				ctx->issue_info.fw_isr = res;
				pr_info("fw isr str:%x\n", ctx->issue_info.fw_isr);
			}
		}
	}

	/* Check IRQ */
	ctx->issue_info.fw_irq = 0;
	type_str = (char*)parser_sub_string[3];
	pDtr = strstr(pStr, type_str);
	if (pDtr == NULL) {
		pr_notice("parser str is NULL,substring(%s)\n", type_str);
	} else {
		pDtr += strlen(type_str);
		pTemp = strchr(pDtr, ',');
		if (pTemp == NULL) {
			pr_notice("delimiter(,) is not found,substring(%s)\n", type_str);
		} else {
			len = pTemp - pDtr;
			len = (len >= CONNV3_ASSERT_TYPE_SIZE) ? CONNV3_ASSERT_TYPE_SIZE - 1 : len;
			memcpy(&tempBuf[0], pDtr, len);
			tempBuf[len] = '\0';
			ret = kstrtouint(tempBuf, 16, &res);
			if (ret) {
				pr_notice("get fw irq id fail ret=%d, buf=%s\n", ret, tempBuf);
			} else {
				ctx->issue_info.fw_irq = res;
				pr_info("fw irq value:%x\n", ctx->issue_info.fw_irq);
			}
		}
	}

check_driver_assert:
	/* If driver trigger, set issue_type to driver */
	if (drv >= CONNV3_DRV_TYPE_BT && drv < CONNV3_DRV_TYPE_MAX) {
		ctx->issue_info.issue_type = CONNV3_ISSUE_DRIVER_ASSERT;
		ctx->issue_info.drv_type = drv;
		if (snprintf(ctx->issue_info.task_name, CONNV3_TASK_NAME_SIZE, "%s", task_drv_name[drv]) < 0) {
			pr_notice("[%s] snprintf failed, task name = %s\n", __func__, task_drv_name[drv]);
		}
	}
	if (reason != NULL) {
		strncpy(ctx->issue_info.reason, reason, CONNV3_ASSERT_REASON_SIZE - 1);
	}
	if (snprintf(ctx->issue_info.subsys_tag, CONNV3_SUBSYS_TAG_SIZE, "%s", connv3_dump_mng_get_subsys_tag(ctx->conn_type)) < 0) {
		pr_notice("[%s] snprintf failed\n", __func__);
	}

	return 0;
}

int connv3_coredump_start(void* handler, const int drv, const char *reason, const char *dump_msg , const char *fw_version)
{
	enum connv3_coredump_mode coredump_mode;
	struct connv3_dump_ctx *ctx = (struct connv3_dump_ctx*)handler;
	int ret = 0;
	enum connv3_coredump_state state;
	char* fw_ver_one_line = NULL;
	int fw_version_len = 0;

	if (ctx == NULL) {
		pr_notice("[%s] invalid input", __func__);
		return CONNV3_COREDUMP_ERR_INVALID_INPUT;
	}

	coredump_mode = connv3_coredump_get_mode();
	if (coredump_mode == CONNV3_DUMP_MODE_RESET_ONLY) {
		pr_info("Chip reset only, skip coredump, only print exception summary\n");
		return CONNV3_COREDUMP_ERR_CHIP_RESET_ONLY;
	}
	state = connv3_dump_get_dump_state(ctx);
	if (state != CONNV3_COREDUMP_STATE_INIT) {
		pr_notice("[%s] state(%d) wrong", __func__, state);
		return CONNV3_COREDUMP_ERR_WRONG_STATUS;
	}

	pr_info("[%s][%s] trigger_drv=%d reason=%s\n",
		__func__, connv3_dump_get_type_name(ctx->conn_type), drv, reason);
	connv3_dump_set_dump_state(ctx, CONNV3_COREDUMP_STATE_START);
	osal_gettimeofday(&g_dump_start_time);

	ret = osal_lock_sleepable_lock(&ctx->ctx_lock);
	if (ret) {
		pr_notice("[%s] get lock fail, ret = %d\n", __func__, ret);
		return CONNV3_COREDUMP_ERR_GET_LOCK_FAIL;
	}

	if (fw_version != NULL)
		fw_version_len = strlen(fw_version);

	/* Log FW version in the first line of CMM */
	if (fw_version_len > 0) {
		/* Add newline symbol in the end */
		fw_version_len += 2;
		fw_ver_one_line = connv3_dump_malloc(fw_version_len*sizeof(char));
		if (fw_ver_one_line != NULL) {
			if (snprintf(fw_ver_one_line, fw_version_len, "%s\n", fw_version) < 0)
				pr_notice("[%s] snprintf fail\n", __func__);
			else
				ret = conndump_netlink_send_to_native(ctx->conn_type, "[M]", (char*)fw_ver_one_line, strlen(fw_ver_one_line));
			connv3_dump_free(fw_ver_one_line);
			fw_ver_one_line = NULL;
		}
	} else {
		pr_notice("[%s] fw_version is null", __func__);
	}
	/* Parse issue info */
	connv3_coredump_info_analysis(ctx, drv, reason, dump_msg);

	/* Send to native */
	ret = conndump_netlink_send_to_native(ctx->conn_type, "[M]", (char*)dump_msg, strlen(dump_msg));

	osal_unlock_sleepable_lock(&ctx->ctx_lock);
	return 0;
}
EXPORT_SYMBOL(connv3_coredump_start);

int connv3_coredump_send(void *handler, char *tag, char *content, unsigned int length)
{
	struct connv3_dump_ctx* ctx = (struct connv3_dump_ctx*)handler;
	int ret = 0;
	enum connv3_coredump_state state = connv3_dump_get_dump_state(ctx);

	if (ctx == NULL) {
		pr_notice("[%s] invalid input", __func__);
		return CONNV3_COREDUMP_ERR_INVALID_INPUT;
	}

	if (ctx->conn_type < 0 || ctx->conn_type >= CONNV3_DEBUG_TYPE_SIZE) {
		pr_notice("[%s] conn_type(%d) is not supported", __func__, ctx->conn_type);
		return CONNV3_COREDUMP_ERR_INVALID_INPUT;
	}

	if (state >= CONNV3_COREDUMP_STATE_START && state <= CONNV3_COREDUMP_STATE_MEM_REGION) {
		ret = osal_lock_sleepable_lock(&ctx->ctx_lock);
		if (ret) {
			pr_notice("[%s] get lock fail, ret = %d\n", __func__, ret);
			return CONNV3_COREDUMP_ERR_GET_LOCK_FAIL;
		}
		ret = conndump_netlink_send_to_native(ctx->conn_type, tag, content, length);
		osal_unlock_sleepable_lock(&ctx->ctx_lock);
	} else {
		pr_notice("[%s][%s] tag=%s, wrong state %d",
			__func__, connv3_dump_get_type_name(ctx->conn_type), tag, state);
		return CONNV3_COREDUMP_ERR_WRONG_STATUS;
	}

	return 0;
}
EXPORT_SYMBOL(connv3_coredump_send);

static int connv3_coredump_gen_issue_info_xml(struct connv3_dump_ctx *ctx, char *buf, unsigned int max_len)
{
#define FORMAT_STRING(buf, len, max_len, sec_len, fmt, arg...) \
do { \
	sec_len = snprintf(buf + len, max_len, fmt, ##arg); \
	if (sec_len > 0) { \
		max_len -= sec_len; \
		len += sec_len; \
		if (max_len <= 0) \
			goto format_finish; \
	} \
} while (0)

	int len = 0;
	int sec_len;
	char* drv_name[CONNV3_DRV_TYPE_MAX] = {
		"DRV_BT",
		"DRV_WIFI",
		"DRV_MODEM",
	};

	/* Init buffer */
	memset(buf, '\0', max_len);
	FORMAT_STRING(buf, len, max_len, sec_len, "<main>\n");
	FORMAT_STRING(buf, len, max_len, sec_len, "\t<chipid>MT%x</chipid>\n", connv3_dump_mng_get_platform_chipid());

	/* <issue> section */
	FORMAT_STRING(buf, len, max_len, sec_len,
		"\t<issue>\n\t\t<classification>\n\t\t\t%s\n\t\t</classification>\n",
		ctx->issue_info.assert_info);
	if (ctx->issue_info.issue_type == CONNV3_ISSUE_FW_EXCEPTION) {
		FORMAT_STRING(buf, len, max_len, sec_len,
			"\t\t<rc>\n\t\t\tFW Exception\n\t\t</rc>\n");
	} else if (ctx->issue_info.issue_type == CONNV3_ISSUE_FW_ASSERT) {
		FORMAT_STRING(buf, len, max_len, sec_len,
			"\t\t<rc>\n\t\t\tFW Assert\n\t\t</rc>\n");
	} else if (ctx->issue_info.issue_type == CONNV3_ISSUE_DRIVER_ASSERT &&
		ctx->issue_info.drv_type < CONNV3_DRV_TYPE_MAX) {
		FORMAT_STRING(buf, len, max_len, sec_len,
			"\t\t<rc>\n\t\t\t%s trigger assert\n\t\t</rc>\n", drv_name[ctx->issue_info.drv_type]);
	} else if (ctx->issue_info.issue_type == CONNV3_ISSUE_FORCE_DUMP) {
		FORMAT_STRING(buf, len, max_len, sec_len,
			"\t\t<rc>\n\t\t\tForce coredump\n\t\t</rc>\n");
	} else {
		FORMAT_STRING(buf, len, max_len, sec_len,
			"\t\t<rc>\n\t\t\tUnknown\n\t\t</rc>\n");
	}
	/* <hint><client> section @{*/
	FORMAT_STRING(buf, len, max_len, sec_len,
		"\t</issue>\n\t<hint>\n\t\t<client>\n");

	if (strlen(ctx->issue_info.subsys_tag) != 0) {
		FORMAT_STRING(buf, len, max_len, sec_len,
			"\t\t\t<subsys>%s</subsys>\n",
			ctx->issue_info.subsys_tag);
	}

	if (ctx->issue_info.issue_type == CONNV3_ISSUE_DRIVER_ASSERT) {
		/* Driver trigger assert */
		FORMAT_STRING(buf, len, max_len, sec_len,
			"\t\t\t<task>%s</task>\n",
			ctx->issue_info.task_name);
		FORMAT_STRING(buf, len, max_len, sec_len,
			"\t\t\t<irqx>NULL</irqx>\n");
		FORMAT_STRING(buf, len, max_len, sec_len,
			"\t\t\t<isr>NULL</isr>\n");
		if (ctx->issue_info.drv_type >= 0 && ctx->issue_info.drv_type < CONNV3_DRV_TYPE_MAX)
			FORMAT_STRING(buf, len, max_len, sec_len,
				"\t\t\t<drv_type>%s</drv_type>\n",
				drv_name[ctx->issue_info.drv_type]);
	} else if (ctx->issue_info.issue_type == CONNV3_ISSUE_FORCE_DUMP) {
		/* For force dump case, assign task as driver */
		FORMAT_STRING(buf, len, max_len, sec_len,
			"\t\t\t<task>%s</task>\n",
			drv_name[ctx->issue_info.drv_type]);
	} else {
		FORMAT_STRING(buf, len, max_len, sec_len,
			"\t\t\t<task>%s</task>\n",
			ctx->issue_info.task_name);
		FORMAT_STRING(buf, len, max_len, sec_len,
			"\t\t\t<irqx>IRQ_0x%x</irqx>\n",
			ctx->issue_info.fw_irq);
		FORMAT_STRING(buf, len, max_len, sec_len,
			"\t\t\t<isr>0x%x</isr>\n",
			ctx->issue_info.fw_isr);
	}
	if (strlen(ctx->issue_info.reason) != 0)
		FORMAT_STRING(buf, len, max_len, sec_len,
			"\t\t\t<reason>%s</reason>\n",
			ctx->issue_info.reason);

	if (ctx->issue_info.issue_type == CONNV3_ISSUE_DRIVER_ASSERT)
		FORMAT_STRING(buf, len, max_len, sec_len,
			"\t\t\t<keyword>%s</keyword>\n", ctx->issue_info.reason);
	else
		FORMAT_STRING(buf, len, max_len, sec_len,
			"\t\t\t<keyword>NULL</keyword>\n");

	FORMAT_STRING(buf, len, max_len, sec_len,
		"\t\t</client>\n\t</hint>\n");
	/*<hint><client> section @}*/
	FORMAT_STRING(buf, len, max_len, sec_len, "</main>\n");
format_finish:
	pr_info("== Issue info ==\n");
	pr_info("%s\n", buf);
	pr_info("===== END =====\n");

	return len;
}

int connv3_coredump_get_issue_info(void *handler, struct connv3_issue_info *issue_info, char *xml_str, unsigned int xml_str_size)
{
	struct connv3_dump_ctx *ctx = (struct connv3_dump_ctx*)handler;
	enum connv3_coredump_state state;
	int ret;

	if (issue_info == NULL || ctx == NULL)
		return CONNV3_COREDUMP_ERR_INVALID_INPUT;

	state = connv3_dump_get_dump_state(ctx);
	if (state < CONNV3_COREDUMP_STATE_START) {
		pr_notice("[%s] state(%d) wrong", __func__, state);
		return CONNV3_COREDUMP_ERR_WRONG_STATUS;
	}

	ret = osal_lock_sleepable_lock(&ctx->ctx_lock);
	if (ret) {
		pr_notice("[%s] get lock fail, ret = %d\n", __func__, ret);
		return CONNV3_COREDUMP_ERR_GET_LOCK_FAIL;
	}

	memcpy(issue_info, &ctx->issue_info, sizeof(struct connv3_issue_info));
	if (xml_str != NULL && xml_str_size > 0)
		connv3_coredump_gen_issue_info_xml(ctx, xml_str, xml_str_size);

	osal_unlock_sleepable_lock(&ctx->ctx_lock);
	return 0;
}
EXPORT_SYMBOL(connv3_coredump_get_issue_info);

static int connv3_dump_end_dump(struct connv3_dump_ctx *ctx)
{
	int ret;
	unsigned long comp_ret;
	// format: dev=/dev/conninfra_dev,emi_size=aaaaaaaa,mcif_emi_size=bbbbbbbb
	char cmd_str[EMI_COMMAND_LENGTH] = {'\0'};
	char *cmd_tag = "[COREDUMP_END]";

	if (snprintf(cmd_str, EMI_COMMAND_LENGTH, "coredump_end") < 0) {
		pr_notice("[%s][%s] coredump end snprintf failed", __func__, connv3_dump_get_type_name(ctx->conn_type));
		return -1;
	}
	connv3_dump_set_dump_state(ctx, CONNV3_COREDUMP_STATE_END);

	pr_info("[%s] tag=[%s] dump command=[%s] cmd length=[%d]\n", __func__, cmd_tag, cmd_str, strlen(cmd_str));
	ret = conndump_netlink_send_to_native(ctx->conn_type, cmd_tag, cmd_str, strlen(cmd_str));

	if (ret <= 0) {
		pr_err("Send end or emi command fail, ret = %d\n", ret);
		connv3_dump_set_dump_state(ctx, CONNV3_COREDUMP_STATE_END_TIMEOUT);
		return -1;
	}

	comp_ret = wait_for_completion_timeout(
		&ctx->emi_dump,
		msecs_to_jiffies(CONNV3_EMIDUMP_TIMEOUT));

	if (comp_ret == 0) {
		pr_err("COREDUMP_END timeout\n");
		connv3_dump_set_dump_state(ctx, CONNV3_COREDUMP_STATE_END_TIMEOUT);
	} else {
		pr_info("COREDUMP_END done");
		connv3_dump_set_dump_state(ctx, CONNV3_COREDUMP_STATE_END_DONE);
	}

	return 0;
}

static int connv3_send_emi_dump(struct connv3_dump_ctx *ctx, bool need_end)
{
	int ret;
	unsigned long comp_ret;
	// format: dev=/dev/conninfra_dev,emi_size=aaaaaaaa,mcif_emi_size=bbbbbbbb,emi2_size=cccccccc
	char cmd_str[EMI_COMMAND_LENGTH] = {'\0'};
	char *cmd_tag;

	/* EMI dump */
	if (snprintf(cmd_str, EMI_COMMAND_LENGTH, "dev=%s,emi_size=%d,mcif_emi_size=%d,emi2_size=%d",
		ctx->cb.dev_node, ctx->cb.emi_size, ctx->cb.mcif_emi_size, ctx->cb.emi2_size) < 0) {
		pr_notice("%s snprintf failed\n", __func__);
		return -1;
	}
	connv3_dump_set_dump_state(ctx, CONNV3_COREDUMP_STATE_EMI);

	if (need_end)
		cmd_tag = "[EMI]"; // dump emi and end coredump
	else
		cmd_tag = "[EMI_ONLY]"; // dump emi only

	pr_info("[%s] tag=[%s] dump command=[%s] cmd length=[%d]\n", __func__, cmd_tag, cmd_str, strlen(cmd_str));
	ret = conndump_netlink_send_to_native(ctx->conn_type, cmd_tag, cmd_str, strlen(cmd_str));

	if (ret <= 0) {
		pr_err("Send end or emi command fail, ret = %d\n", ret);
		connv3_dump_set_dump_state(ctx, CONNV3_COREDUMP_STATE_EMI_TIMEOUT);
		return -1;
	}

	comp_ret = wait_for_completion_timeout(
		&ctx->emi_dump,
		msecs_to_jiffies(CONNV3_EMIDUMP_TIMEOUT));

	if (comp_ret == 0) {
		pr_err("EMI dump timeout\n");
		connv3_dump_set_dump_state(ctx, CONNV3_COREDUMP_STATE_EMI_TIMEOUT);
	} else {
		pr_info("EMI dump end");
		connv3_dump_set_dump_state(ctx, CONNV3_COREDUMP_STATE_EMI_DONE);
	}
	return 0;
}

int connv3_coredump_emi(void *handler)
{
	struct connv3_dump_ctx *ctx = (struct connv3_dump_ctx*)handler;
	int ret;
	enum connv3_coredump_state state;

	if (ctx == NULL)
		return CONNV3_COREDUMP_ERR_INVALID_INPUT;
	if (strlen(ctx->cb.dev_node) == 0 || ctx->cb.emi_size == 0) {
		pr_notice("[%s][%s] not support EMI dump\n", __func__, connv3_dump_get_type_name(ctx->conn_type));
		return CONNV3_COREDUMP_ERR_INVALID_INPUT;
	}

	state = connv3_dump_get_dump_state(ctx);
	if (state < CONNV3_COREDUMP_STATE_START ||
	    state >= CONNV3_COREDUMP_STATE_EMI) {
		pr_notice("[%s] state(%d) wrong", __func__, state);
		return CONNV3_COREDUMP_ERR_WRONG_STATUS;
	}

	ret = osal_lock_sleepable_lock(&ctx->ctx_lock);
	if (ret) {
		pr_notice("[%s] get lock fail, ret = %d\n", __func__, ret);
		return CONNV3_COREDUMP_ERR_GET_LOCK_FAIL;
	}

	/* Do emi dump only, don't end coredump. */
	ret = connv3_send_emi_dump(ctx, false);

	osal_unlock_sleepable_lock(&ctx->ctx_lock);

	return ret;
}
EXPORT_SYMBOL(connv3_coredump_emi);

static int connv3_dump_exception_show(struct connv3_dump_ctx *ctx, char *customized_string)
{
	char *exception_log;
	char *exp_tag_name = connv3_dump_mng_get_exception_tag_name(ctx->conn_type);
	int check;

	if (customized_string != NULL && strlen(customized_string) != 0) {
		pr_info("[%s] use customized_string=%s\n", __func__, customized_string);
		exception_log = customized_string;
	} else {
		pr_info("[%s] use assert_info=%s\n", __func__, ctx->issue_info.assert_info);
		exception_log = ctx->issue_info.assert_info;
	}

	if (snprintf(ctx->issue_info.exception_log, CONNV3_AEE_INFO_SIZE - 1,
		"%s %s", INFO_HEAD, exception_log) < 0)
		pr_notice("%s snprintf issue_info.exception_log failed\n", __func__);

	pr_info("par1: [%s] par2: [%s] par3: [%d]\n",
		exp_tag_name,
		ctx->issue_info.exception_log,
		strlen(ctx->issue_info.exception_log));

	check = connv3_dump_mng_exception_filter(ctx->issue_info.exception_log);
	if (check) {
		pr_notice("[%s] exception ignore, check = %d\n", __func__, check);
		return 0;
	}

	/* Call debug API */
	osal_dbg_common_exception_api(
		exp_tag_name,
		NULL, 0,
		(const int*)ctx->issue_info.exception_log, strlen(ctx->issue_info.exception_log),
		ctx->issue_info.exception_log, 0);
	return 0;
}

int connv3_coredump_end(void *handler, char *customized_string)
{
	struct connv3_dump_ctx *ctx = (struct connv3_dump_ctx*)handler;
	struct timespec64 pre_end, end;
	enum connv3_coredump_state state;
	int ret;

	if (ctx == NULL)
		return CONNV3_COREDUMP_ERR_INVALID_INPUT;

	state = connv3_dump_get_dump_state(ctx);
	if (state < CONNV3_COREDUMP_STATE_START) {
		pr_notice("[%s] state(%d) wrong", __func__, state);
		return CONNV3_COREDUMP_ERR_WRONG_STATUS;
	}

	ret = osal_lock_sleepable_lock(&ctx->ctx_lock);
	if (ret) {
		pr_notice("[%s] get lock fail, ret = %d\n", __func__, ret);
		return CONNV3_COREDUMP_ERR_GET_LOCK_FAIL;
	}

	osal_gettimeofday(&pre_end);

	if (strlen(ctx->cb.dev_node) == 0 || ctx->cb.emi_size == 0) {
		/* Send end command */
		connv3_dump_end_dump(ctx);
	} else {
		/* EMI dump has been executed */
		if (state >= CONNV3_COREDUMP_STATE_EMI) {
			/* Send end command */
			connv3_dump_end_dump(ctx);
		} else {
			/* Do EMI dump and end coredump */
			ret = connv3_send_emi_dump(ctx, true);
			if (ret)
				pr_notice("[%s] Send emi dump and end coredump error, ret = %d\n",
					__func__, ret);
		}
	}

	/* All process finished, set to init status */
	connv3_dump_set_dump_state(ctx, CONNV3_COREDUMP_STATE_INIT);

	connv3_dump_exception_show(ctx, customized_string);
	osal_gettimeofday(&end);

	pr_info("[V3 coredump][%s] Summary: total time=[%lu] end/emi dump=[%lu]",
		connv3_dump_mng_get_exception_tag_name(ctx->conn_type),
		timespec64_to_ms(&g_dump_start_time, &end),
		timespec64_to_ms(&pre_end, &end));

	osal_unlock_sleepable_lock(&ctx->ctx_lock);

	return 0;
}
EXPORT_SYMBOL(connv3_coredump_end);

void* connv3_coredump_init(int conn_type, const struct connv3_coredump_event_cb *cb)
{
	struct connv3_dump_ctx *ctx = NULL;
	struct netlink_event_cb nl_cb;

	if (conn_type < 0 || conn_type >= CONNV3_DEBUG_TYPE_SIZE) {
		pr_notice("[%s] wrong type: %d", __func__, conn_type);
		goto error_exit;
	}
	ctx = (struct connv3_dump_ctx*)connv3_dump_malloc(sizeof(struct connv3_dump_ctx));
	if (!ctx) {
		pr_notice("[%s][%s] Allocate connv3_dump_ctx fail",
			__func__, connv3_dump_get_type_name(conn_type));
		goto error_exit;
	}
	/* Clean */
	memset(ctx, 0, sizeof(struct connv3_dump_ctx));
	if (cb != 0)
		memcpy(&ctx->cb, cb, sizeof(struct connv3_coredump_event_cb));
	ctx->conn_type = conn_type;
	connv3_dump_set_dump_state(ctx, CONNV3_COREDUMP_STATE_INIT);
	init_completion(&ctx->emi_dump);
	osal_sleepable_lock_init(&ctx->ctx_lock);

	/* Register to netlink */
	nl_cb.coredump_end = connv3_dump_emi_dump_end;
	nl_cb.coredump_get_save_emi = connv3_dump_get_save_emi;
	if (conndump_netlink_init(ctx->conn_type, ctx, &nl_cb) < 0)
		goto error_exit;

	return ctx;

error_exit:
	if (ctx)
		connv3_coredump_deinit(ctx);
	return NULL;
}
EXPORT_SYMBOL(connv3_coredump_init);

void connv3_coredump_deinit(void *handler)
{
	struct connv3_dump_ctx *ctx = (struct connv3_dump_ctx*)handler;

	if (handler == NULL)
		return;
	osal_sleepable_lock_deinit(&ctx->ctx_lock);
	connv3_dump_free(ctx);
}
EXPORT_SYMBOL(connv3_coredump_deinit);

enum connv3_coredump_mode connv3_coredump_get_mode(void)
{
	return atomic_read(&g_dump_mode);
}


void connv3_coredump_set_dump_mode(enum connv3_coredump_mode mode)
{
	pr_info("[%s] mode=%d\n", __func__, mode);
	if (mode < CONNV3_DUMP_MODE_MAX)
		atomic_set(&g_dump_mode, mode);
}

static unsigned int connv3_coredump_get_memdump_mode(void)
{
	return atomic_read(&g_memdump_mode);
}

void connv3_coredump_set_memdump_mode(unsigned int mode)
{
	pr_info("[%s] mode=%d\n", __func__, mode);
	atomic_set(&g_memdump_mode, mode);
}
EXPORT_SYMBOL(connv3_coredump_set_memdump_mode);
