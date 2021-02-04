
/*
* Copyright (C) 2019 MediaTek Inc.
*
* This program is free software: you can redistribute it and/or modify it under the terms of the
* GNU General Public License version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with this program.
* If not, see <http://www.gnu.org/licenses/>.
*/


#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/kthread.h>
#include "wifi_pwr_on.h"





MODULE_LICENSE("Dual BSD/GPL");

#define PFX                         "[WIFI-FW] "
#define WIFI_FW_LOG_DBG             3
#define WIFI_FW_LOG_INFO            2
#define WIFI_FW_LOG_WARN            1
#define WIFI_FW_LOG_ERR             0

uint32_t DbgLevel = WIFI_FW_LOG_DBG;

#define WIFI_DBG_FUNC(fmt, arg...)	\
	do { \
		if (DbgLevel >= WIFI_FW_LOG_DBG) \
			pr_info(PFX "%s[D]: " fmt, __func__, ##arg); \
	} while (0)
#define WIFI_INFO_FUNC(fmt, arg...)	\
	do { \
		if (DbgLevel >= WIFI_FW_LOG_INFO) \
			pr_info(PFX "%s[I]: " fmt, __func__, ##arg); \
	} while (0)
#define WIFI_INFO_FUNC_LIMITED(fmt, arg...)	\
	do { \
		if (DbgLevel >= WIFI_FW_LOG_INFO) \
			pr_info_ratelimited(PFX "%s[L]: " fmt, __func__, ##arg); \
	} while (0)
#define WIFI_WARN_FUNC(fmt, arg...)	\
	do { \
		if (DbgLevel >= WIFI_FW_LOG_WARN) \
			pr_info(PFX "%s[W]: " fmt, __func__, ##arg); \
	} while (0)
#define WIFI_ERR_FUNC(fmt, arg...)	\
	do { \
		if (DbgLevel >= WIFI_FW_LOG_ERR) \
			pr_info(PFX "%s[E]: " fmt, __func__, ##arg); \
	} while (0)


wlan_probe_cb mtk_wlan_probe_function;
wlan_remove_cb mtk_wlan_remove_function;


struct completion wlan_pendComp;

int g_opId;
int g_data;




static int mtk_wland_thread(void *pvData);

int wifi_pwr_on_init(void)
{
	int result = 0;

	init_completion(&wlan_pendComp);
	WIFI_INFO_FUNC("Do wifi_pwr_on_init.\n");
	return result;
}
EXPORT_SYMBOL(wifi_pwr_on_init);

int wifi_pwr_on_deinit(void)
{
	return 0;
}
EXPORT_SYMBOL(wifi_pwr_on_deinit);
int mtk_wcn_wlan_reg(struct MTK_WCN_WLAN_CB_INFO *pWlanCbInfo)
{
	if (!pWlanCbInfo) {
		WIFI_ERR_FUNC("wlan cb info in null!\n");
		return -1;
	}
	WIFI_INFO_FUNC("wmt wlan cb register\n");
	mtk_wlan_probe_function = pWlanCbInfo->wlan_probe_cb;
	mtk_wlan_remove_function = pWlanCbInfo->wlan_remove_cb;

	return 0;
}
EXPORT_SYMBOL(mtk_wcn_wlan_reg);

int mtk_wcn_wlan_unreg(void)
{

	WIFI_INFO_FUNC("wmt wlan cb unregister\n");
	mtk_wlan_probe_function = NULL;
	mtk_wlan_remove_function = NULL;

	return 0;
}
EXPORT_SYMBOL(mtk_wcn_wlan_unreg);

static int mtk_wland_thread(void *pvData)
{
	g_data = -1;
	switch (g_opId) {
	case WLAN_OPID_FUNC_ON:
		if (mtk_wlan_probe_function != NULL) {
			g_data = (*mtk_wlan_probe_function)();
		} else {
			WIFI_ERR_FUNC("Invalid pointer\n");
			complete(&wlan_pendComp);
		}
		break;
	case WLAN_OPID_FUNC_OFF:
		if (mtk_wlan_remove_function != NULL) {
			g_data = (*mtk_wlan_remove_function)();
		} else {
			WIFI_ERR_FUNC("Invalid pointer\n");
			complete(&wlan_pendComp);
		}
		break;
	default:
		WIFI_ERR_FUNC("Unknown opid\n");
		break;
	}
	complete(&wlan_pendComp);
	return 0;
}
struct task_struct *wland_thread;


static int data;
#define WIFI_PWR_ON_TIMEOUT 4000

int mtk_wcn_wlan_func_ctrl(enum ENUM_WLAN_OPID opId)
{
	bool bRet = MTK_WCN_BOOL_TRUE;
	uint32_t waitRet = 0;

	g_opId = opId;
	wland_thread = kthread_run(mtk_wland_thread, &data, "wland_thread");
	waitRet = wait_for_completion_timeout(&wlan_pendComp, MSEC_TO_JIFFIES(WIFI_PWR_ON_TIMEOUT));
	if (waitRet > 0) {
		/* Case 1: No timeout. */
		if (g_data != 0)
			bRet = MTK_WCN_BOOL_FALSE;
	} else {
		/* Case 2: timeout */
		WIFI_ERR_FUNC("WiFi on/off takes more than 4 seconds\n");
		bRet = MTK_WCN_BOOL_FALSE;
	}
	return bRet;
}
EXPORT_SYMBOL(mtk_wcn_wlan_func_ctrl);




