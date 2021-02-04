/*
 * Copyright (C) 2019 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include "plat.h"

static int (*whole_chip_reset)(signed int sta);

static void WCNfm_wholechip_rst_cb(
	ENUM_WMTDRV_TYPE_T src, ENUM_WMTDRV_TYPE_T dst,
	ENUM_WMTMSG_TYPE_T type, void *buf, unsigned int sz)
{
	/* To handle reset procedure please */
	ENUM_WMTRSTMSG_TYPE_T rst_msg;

	if (sz > sizeof(ENUM_WMTRSTMSG_TYPE_T)) {
		/*message format invalid */
		WCN_DBG(FM_WAR | LINK, "message format invalid!\n");
		return;
	}

	memcpy((char *)&rst_msg, (char *)buf, sz);
	WCN_DBG(FM_WAR | LINK,
		"[src=%d], [dst=%d], [type=%d], [buf=0x%x], [sz=%d], [max=%d]\n",
		src, dst, type, rst_msg, sz, WMTRSTMSG_RESET_MAX);

	if ((src == WMTDRV_TYPE_WMT) && (dst == WMTDRV_TYPE_FM)
	    && (type == WMTMSG_TYPE_RESET)) {

		if (rst_msg == WMTRSTMSG_RESET_START) {
			WCN_DBG(FM_WAR | LINK, "FM restart start!\n");
			if (whole_chip_reset)
				whole_chip_reset(1);
		} else if (rst_msg == WMTRSTMSG_RESET_END_FAIL) {
			WCN_DBG(FM_WAR | LINK, "FM restart end fail!\n");
			if (whole_chip_reset)
				whole_chip_reset(2);
		} else if (rst_msg == WMTRSTMSG_RESET_END) {
			WCN_DBG(FM_WAR | LINK, "FM restart end!\n");
			if (whole_chip_reset)
				whole_chip_reset(0);
		}
	}
}

static void fw_eint_handler(void)
{
	fm_event_parser(fm_rds_parser);
}

static int fm_stp_send_data(unsigned char *buf, unsigned int len)
{
	return mtk_wcn_stp_send_data(buf, len, FM_TASK_INDX);
}

static int fm_stp_recv_data(unsigned char *buf, unsigned int len)
{
	return mtk_wcn_stp_receive_data(buf, len, FM_TASK_INDX);
}

static int fm_stp_register_event_cb(void *cb)
{
	return mtk_wcn_stp_register_event_cb(FM_TASK_INDX, cb);
}

static int fm_wmt_msgcb_reg(void *data)
{
	/* get whole chip reset cb */
	whole_chip_reset = data;
	return mtk_wcn_wmt_msgcb_reg(
		WMTDRV_TYPE_FM, WCNfm_wholechip_rst_cb);
}

static int fm_wmt_func_on(void)
{
	int ret = 0;

	ret = mtk_wcn_wmt_func_on(WMTDRV_TYPE_FM) != MTK_WCN_BOOL_FALSE;

	return ret;
}

static int fm_wmt_func_off(void)
{
	int ret = 0;

	ret = mtk_wcn_wmt_func_off(WMTDRV_TYPE_FM) != MTK_WCN_BOOL_FALSE;

	return ret;
}

static int fm_wmt_ic_info_get(void)
{
	return mtk_wcn_wmt_ic_info_get(1);
}

static int fm_wmt_chipid_query(void)
{
	return mtk_wcn_wmt_chipid_query();
}

static unsigned char drv_get_top_index(void)
{
#if defined(MT6635_FM)
	return 5;
#else
	int chipid = fm_wmt_chipid_query();

	if (chipid == 0x6779 || chipid == 0x6885 || chipid == 0x6873)
		return 5;
	return 4;
#endif
}

void register_fw_ops_init(void)
{
	fm_wcn_ops.ei.eint_handler = fw_eint_handler;
	fm_wcn_ops.ei.stp_send_data = fm_stp_send_data;
	fm_wcn_ops.ei.stp_recv_data = fm_stp_recv_data;
	fm_wcn_ops.ei.stp_register_event_cb = fm_stp_register_event_cb;
	fm_wcn_ops.ei.wmt_msgcb_reg = fm_wmt_msgcb_reg;
	fm_wcn_ops.ei.wmt_func_on = fm_wmt_func_on;
	fm_wcn_ops.ei.wmt_func_off = fm_wmt_func_off;
	fm_wcn_ops.ei.wmt_ic_info_get = fm_wmt_ic_info_get;
	fm_wcn_ops.ei.wmt_chipid_query = fm_wmt_chipid_query;
	fm_wcn_ops.ei.get_top_index = drv_get_top_index;
}

void register_fw_ops_uninit(void)
{
}

int fm_register_irq(struct platform_driver *drv)
{
	return 0;
}

int fm_wcn_ops_register(void)
{
	register_fw_ops_init();

	return 0;
}

int fm_wcn_ops_unregister(void)
{
	register_fw_ops_uninit();

	return 0;
}
