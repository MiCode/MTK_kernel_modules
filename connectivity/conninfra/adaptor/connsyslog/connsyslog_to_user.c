/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/delay.h>
#include "connsyslog_to_user.h"
#include "fw_log_mcu.h"

struct connlog_user {
	bool inited;
	int (*init) (int conn_type, struct connlog_to_user_cb *cb);
	void (*deinit) (void);
	void (*evt_cb) (void);
};

struct connlog_user g_connlog_user[CONN_ADAPTOR_DRV_SIZE] = {
	/* wifi */
	{ false, fw_log_wifi_mcu_init, fw_log_wifi_mcu_deinit, fw_log_wifi_mcu_event_cb},
	/* bt */
	{ false, fw_log_bt_mcu_init, fw_log_bt_mcu_deinit, fw_log_bt_mcu_event_cb},
	/* gps */
	{ false, fw_log_gps_mcu_init, fw_log_gps_mcu_deinit, fw_log_gps_mcu_event_cb},
};


int connlog_to_user_register(int conn_type, struct connlog_to_user_cb *cb)
{
	int ret;

	pr_info("[%s] type=[%d]", __func__, conn_type);

	if (conn_type < 0 || conn_type >= CONN_ADAPTOR_DRV_SIZE)
		return -1;

	if (g_connlog_user[conn_type].inited || g_connlog_user[conn_type].init == NULL) {
		pr_info("[%s] conn [%d] was inited", __func__, conn_type);
		return -1;
	}

	(*(g_connlog_user[conn_type].init))(conn_type, cb);
	ret = (*(cb->register_evt_cb))(cb->conn_type_id, g_connlog_user[conn_type].evt_cb);
	if (ret)
		pr_err("[%s] register_evt fail", __func__);
	g_connlog_user[conn_type].inited = true;

	return 0;
}

int connlog_to_user_unregister(int conn_type)
{
	if (conn_type < 0 || conn_type >= CONN_ADAPTOR_DRV_SIZE)
		return -1;

	(*(g_connlog_user[conn_type].deinit))();
	g_connlog_user[conn_type].inited = false;

	return 0;
}
