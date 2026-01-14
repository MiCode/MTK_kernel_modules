// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include <linux/platform_device.h>
#include <linux/types.h>

#include "connv3.h"
#include "connv3_hw.h"
#include "connv3_hw_dbg.h"

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/
#define LOG_DUMP_BUF_SZ 512
static struct connv3_platform_dbg_ops *g_connv3_platform_dbg_ops = NULL;
static char g_dump_buf[LOG_DUMP_BUF_SZ] = {'\0'};

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

int connv3_hw_dbg_init(
	struct platform_device *pdev,
	const struct connv3_plat_data* plat_data)
{
	g_connv3_platform_dbg_ops = (struct connv3_platform_dbg_ops*)plat_data->platform_dbg_ops;

	return 0;
}

int connv3_hw_dbg_deinit(void)
{
	g_connv3_platform_dbg_ops = NULL;
	return 0;
}

int connv3_hw_dbg_bus_dump(
	enum connv3_drv_type drv_type, struct connv3_cr_cb *cb)
{
	if (g_connv3_platform_dbg_ops &&
		g_connv3_platform_dbg_ops->dbg_bus_dump)
		return g_connv3_platform_dbg_ops->dbg_bus_dump(drv_type, cb);
	return 0;
}


int connv3_hw_dbg_dump_utility(
	const struct connv3_dump_list *dump_list, struct connv3_cr_cb *cb)
{
#define LOG_TMP_BUF_SZ 32
	int ret = 0, func_ret = 0;
	int i;
	const struct connv3_dbg_command *command;
	unsigned int value;
	char tmp[LOG_TMP_BUF_SZ] = {'\0'};
	int dump_count = 0;
	int dump_line = 0;
	void *data = cb->priv_data;

	memset(g_dump_buf, '\0', sizeof(char)*LOG_DUMP_BUF_SZ);
	for (i = 0; i < dump_list->dump_size; i++) {
		command = &dump_list->cmd_list[i];
		if (command != NULL) {
			/* Write with mask */
			if (command->write && command->mask != 0) {
				ret = cb->write_mask(
					data, command->w_addr, command->mask, command->value);
			} else if (command->write) {
				/* Writ directly */
				ret = cb->write(data, command->w_addr, command->value);
			}
			if (ret) {
				pr_err("[V3 dump][%s][%d] write error: %d",
					dump_list->tag, i, ret);
				func_ret = i;
				break;
			}
			if (command->read) {
				ret = cb->read(data, command->r_addr, &value);
				if (ret) {
					pr_err("[V3 dump][%s][%d] read error: %d",
						dump_list->tag, i, ret);
					func_ret = i;
					break;
				}
				if (snprintf(tmp, LOG_TMP_BUF_SZ, "[0x%08x]", value) >= 0)
					strncat(g_dump_buf, tmp, strlen(tmp));
				dump_count++;
				if ((dump_count % 25) == 0) {
					pr_info("[V3_BUS][%s][%d] %s", dump_list->tag, dump_line, g_dump_buf);
					dump_line++;
					memset(g_dump_buf, '\0', sizeof(char)*LOG_DUMP_BUF_SZ);
				}
			}
		}
	}

	/* only one line */
	if (dump_line == 0)
		pr_info("[V3_BUS][%s] %s", dump_list->tag, g_dump_buf);
	else
		pr_info("[V3_BUS][%s][%d] %s", dump_list->tag, dump_line, g_dump_buf);

	return func_ret;
}


int connv3_hw_dbg_power_info_dump(
	enum connv3_drv_type drv_type, struct connv3_cr_cb *cb,
	char *buf, unsigned int size)
{
	if (g_connv3_platform_dbg_ops &&
		g_connv3_platform_dbg_ops->dbg_power_info_dump)
	return g_connv3_platform_dbg_ops->dbg_power_info_dump(drv_type, cb, buf, size);

	return 0;
}

int connv3_hw_dbg_power_info_reset(
	enum connv3_drv_type drv_type, struct connv3_cr_cb *cb)
{
	if (g_connv3_platform_dbg_ops &&
		g_connv3_platform_dbg_ops->dbg_power_info_reset)
		return g_connv3_platform_dbg_ops->dbg_power_info_reset(drv_type, cb);

	return 0;
}
