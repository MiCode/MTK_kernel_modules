// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */
#include <linux/delay.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

#include "connv3.h"
#include "connv3_debug_utility.h"

#define CONNV3_XML_SIZE	1024

const char wifi_assert[] = "<EXCEPTION> , WFSYS, id=0x0 WIFI, isr=0x90211C, irq=0x0, lr=0x90213E, swid=0x99, mcause=0xB, exp_t=7866911\n;etype:ABT, sp=0x220EE80, lp=0x90213E\n;mcause=0xB, mtval=0x0, mepc=0x90218C, mdcause=0x0\n;exp_main: jump from exception";
const char wifi_fw_version[] = "2022-03-07 wifi test";

const char bt_exception[] = "<EXCEPTION> , BTSYS, id=0x0 idle, isr=0x0, irq=0x31, lr=0x903AC0, ipc=0x903BE2, eva=0x903BE2, swid=0x66, p1=0x0, p2=0x0, itype=0x6600E2, exp_t=4970306";
const char bt_assert[] = "<ASSERT> system/rom/common/cos_task_rom.c #377 - , 0x0, 0x0, 0x0, BTSYS, id=0x0 Tmr Svc, isr=0x8008CE, irq=0xC, lr=0x8011CE, asr_t=273125";
const char bt_fw_version[] = "2022-03-07 bt test";
char xml_log[CONNV3_XML_SIZE];

int connv3_dump_test(int par1, int par2, int par3)
{
	void *wifi_handler;
	void *bt_handler;
	struct connv3_issue_info issue_info;

	wifi_handler = connv3_coredump_init(CONNV3_DEBUG_TYPE_WIFI, NULL);
	bt_handler = connv3_coredump_init(CONNV3_DEBUG_TYPE_BT, NULL);
	/* Wait 5 sec to make sure dump service is bind. */
	msleep(5000);

	if (wifi_handler == NULL)
		pr_notice("[%s] wifi init fail", __func__);
	else {
		memset(xml_log, '\0', CONNV3_XML_SIZE);
		memset(&issue_info, 0, sizeof(struct connv3_issue_info));
		pr_info("[%s][Test-1] msg=%s", __func__, wifi_assert);
		connv3_coredump_start(
			wifi_handler, -1,
			"WIFI dump assert test", wifi_assert, wifi_fw_version);
		connv3_coredump_get_issue_info(wifi_handler, &issue_info, xml_log, CONNV3_XML_SIZE);
		connv3_coredump_send(wifi_handler, "INFO", xml_log, strlen(xml_log));
		connv3_coredump_end(wifi_handler, "WIFI dump assert test");

		connv3_coredump_deinit(wifi_handler);
		wifi_handler = NULL;
	}

	if (bt_handler == NULL)
		pr_notice("[%s] bt init fail", __func__);
	else {
		memset(xml_log, '\0', CONNV3_XML_SIZE);
		memset(&issue_info, 0, sizeof(struct connv3_issue_info));
		pr_info("[%s][Test-2] msg=%s", __func__, bt_exception);
		connv3_coredump_start(
			bt_handler, -1,
			"BT exception test", bt_exception, bt_fw_version);
		connv3_coredump_get_issue_info(bt_handler, &issue_info, xml_log, CONNV3_XML_SIZE);
		connv3_coredump_send(bt_handler, "INFO", xml_log, strlen(xml_log));
		connv3_coredump_end(bt_handler, "BT exception test");

		memset(xml_log, '\0', CONNV3_XML_SIZE);
		memset(&issue_info, 0, sizeof(struct connv3_issue_info));
		pr_info("[%s][Test-3] msg=%s", __func__, bt_assert);
		connv3_coredump_start(
			bt_handler, CONNV3_DRV_TYPE_WIFI,
			"BT assert test", bt_assert, bt_fw_version);
		connv3_coredump_get_issue_info(bt_handler, &issue_info, xml_log, CONNV3_XML_SIZE);
		connv3_coredump_send(bt_handler, "INFO", xml_log, strlen(xml_log));
		connv3_coredump_end(bt_handler, "BT assert test");

		connv3_coredump_deinit(bt_handler);
		bt_handler = NULL;
	}

	return 0;
}

