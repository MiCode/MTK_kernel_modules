// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*
 ** Id: /os/linux/gl_sys.c
 */

/*! \file   "gl_sys.c"
 *  \brief  This file defines the interface which can interact with users
 *          in /sys fs.
 *
 *    Detail description.
 */

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "precomp.h"
#include "gl_os.h"
#include "gl_kal.h"
#include "debug.h"
#include "wlan_lib.h"
#include "debug.h"
#include "wlan_oid.h"
#include <linux/rtc.h>
#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/module.h>
#include <linux/init.h>

#if WLAN_INCLUDE_SYS

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define MTK_INFO_MAX_SIZE 128

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

#if (CFG_SUPPORT_MULTI_CARD == 0)
static struct kobject *wifi_kobj;
static struct GLUE_INFO *g_prGlueInfo;
static uint8_t aucMacAddrOverride[] = "FF:FF:FF:FF:FF:FF";
static u_int8_t fgIsMacAddrOverride = FALSE;
static int32_t g_i4PM = -1;
static char acSoftAPInfo[MTK_INFO_MAX_SIZE];
static char acVerInfo[MTK_INFO_MAX_SIZE];

#if BUILD_QA_DBG
static uint32_t g_u4Memdump = 3;
#else
static uint32_t g_u4Memdump = 2;
#endif /* BUILD_QA_DBG */
#endif /* CFG_SUPPORT_MULTI_CARD */

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
#if CFG_SUPPORT_MULTI_CARD
static struct GLUE_INFO *sysGetGlueInfo(struct kobject *kobj)
{
	uint32_t u4Idx = 0;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct net_device *prDev = NULL;

	for (u4Idx = 0; u4Idx < CFG_MAX_WLAN_DEVICES; u4Idx++) {
		prDev = arWlanDevInfo[u4Idx].prDev;
		prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prDev));

		if (prGlueInfo && prGlueInfo->wlan_kobj == kobj)
			return prGlueInfo;
	}

	return NULL;
}
#endif

static ssize_t pm_show(
	struct kobject *kobj,
	struct kobj_attribute *attr,
	char *buf)
{
	int32_t i4PM;
#if CFG_SUPPORT_MULTI_CARD
	struct GLUE_INFO *prGlueInfo = sysGetGlueInfo(kobj);

	if (!prGlueInfo) {
		DBGLOG(INIT, ERROR, "prGlueInfo is null\n");
		return 0;
	}
	i4PM = prGlueInfo->i4PM;
#else
	i4PM = g_i4PM;
#endif

	return snprintf(buf,
		sizeof(i4PM),
		"%d", i4PM);
}

static void pm_EnterCtiaMode(struct GLUE_INFO *prGlueInfo)
{
	int32_t i4PM;

#if CFG_SUPPORT_MULTI_CARD
	i4PM = prGlueInfo->i4PM;
#else
	i4PM = g_i4PM;
#endif

	if (!prGlueInfo)
		DBGLOG(INIT, ERROR, "prGlueInfo is null\n");
	else if (i4PM == -1)
		DBGLOG(INIT, TRACE, "keep default\n");
	else {
		prGlueInfo->prAdapter->fgEnDbgPowerMode = !i4PM;
		nicEnterCtiaMode(prGlueInfo->prAdapter,
			!i4PM,
			FALSE);
	}
}

static ssize_t pm_store(
	struct kobject *kobj,
	struct kobj_attribute *attr,
	const char *buf,
	size_t count)
{
	int32_t i4Ret = 0;
	int32_t *prPM = NULL;
	struct GLUE_INFO *prGlueInfo = NULL;

#if CFG_SUPPORT_MULTI_CARD
	prGlueInfo = sysGetGlueInfo(kobj);

	if (!prGlueInfo) {
		DBGLOG(INIT, ERROR, "prGlueInfo is null\n");
		return 0;
	}
	prPM = &prGlueInfo->i4PM;
#else
	prGlueInfo = g_prGlueInfo;
	prPM = &g_i4PM;
#endif

	i4Ret = kstrtoint(buf, 10, prPM);

	if (i4Ret)
		DBGLOG(INIT, ERROR, "sscanf pm fail u4Ret=%d\n", i4Ret);
	else {
		DBGLOG(INIT, INFO,
			"Set PM to %d.\n",
			*prPM);

		pm_EnterCtiaMode(prGlueInfo);
	}

	return (i4Ret == 0) ? count : 0;
}

static ssize_t macaddr_show(
	struct kobject *kobj,
	struct kobj_attribute *attr,
	char *buf)
{
	uint8_t *prMacAddrOverride;
	uint32_t len;
#if CFG_SUPPORT_MULTI_CARD
	struct GLUE_INFO *prGlueInfo = sysGetGlueInfo(kobj);

	if (!prGlueInfo) {
		DBGLOG(INIT, ERROR, "prGlueInfo is null\n");
		return 0;
	}
	prMacAddrOverride = prGlueInfo->aucMacAddrOverride;
	len = sizeof(prGlueInfo->aucMacAddrOverride);
#else
	prMacAddrOverride = aucMacAddrOverride;
	len = sizeof(aucMacAddrOverride);
#endif

	return snprintf(buf, len, "%s", prMacAddrOverride);
}

static ssize_t macaddr_store(
	struct kobject *kobj,
	struct kobj_attribute *attr,
	const char *buf,
	size_t count)
{
	int32_t i4Ret = 0;
	uint8_t *prMacAddrOverride = NULL;
	u_int8_t *prIsMacAddrOverride = NULL;

#if CFG_SUPPORT_MULTI_CARD
	struct GLUE_INFO *prGlueInfo = sysGetGlueInfo(kobj);

	if (prGlueInfo) {
		DBGLOG(INIT, ERROR, "prGlueInfo is null\n");
		return 0;
	}
	prMacAddrOverride   = prGlueInfo->aucMacAddrOverride;
	prIsMacAddrOverride = &prGlueInfo->fgIsSysFsMacAddrOverride;
#else
	prMacAddrOverride   = aucMacAddrOverride;
	prIsMacAddrOverride = &fgIsMacAddrOverride;
#endif

	i4Ret = sscanf(buf, "%18s", (uint8_t *) prMacAddrOverride);

	if (!i4Ret)
		DBGLOG(INIT, ERROR, "sscanf mac format fail u4Ret=%d\n", i4Ret);
	else {
		DBGLOG(INIT, INFO,
			"Set macaddr to %s.\n",
			prMacAddrOverride);
	}

	*prIsMacAddrOverride = TRUE;

	return (i4Ret > 0) ? count : 0;
}

static ssize_t wifiver_show(
	struct kobject *kobj,
	struct kobj_attribute *attr,
	char *buf)
{
	char *prVerInfo;
	size_t len;
#if CFG_SUPPORT_MULTI_CARD
	struct GLUE_INFO *prGlueInfo = sysGetGlueInfo(kobj);

	if (!prGlueInfo) {
		DBGLOG(INIT, ERROR, "prGlueInfo is null\n");
		return 0;
	}
	prVerInfo = prGlueInfo->acVerInfo;
	len = sizeof(prGlueInfo->acVerInfo);
#else
	prVerInfo = acVerInfo;
	len = sizeof(acVerInfo);
#endif

	return snprintf(buf, len, "%s", prVerInfo);
}

static ssize_t softap_show(
	struct kobject *kobj,
	struct kobj_attribute *attr,
	char *buf)
{
	char *prSoftAPInfo;
	size_t len;
#if CFG_SUPPORT_MULTI_CARD
	struct GLUE_INFO *prGlueInfo = sysGetGlueInfo(kobj);

	if (!prGlueInfo) {
		DBGLOG(INIT, ERROR, "prGlueInfo is null\n");
		return 0;
	}
	prSoftAPInfo = prGlueInfo->acSoftAPInfo;
	len = sizeof(prGlueInfo->acSoftAPInfo);
#else
	prSoftAPInfo = acSoftAPInfo;
	len = sizeof(acSoftAPInfo);
#endif

	return snprintf(buf, len, "%s", prSoftAPInfo);
}

static ssize_t memdump_show(
	struct kobject *kobj,
	struct kobj_attribute *attr,
	char *buf)
{
	uint32_t *prMemdump = NULL;
	size_t len = 0;
#if CFG_SUPPORT_MULTI_CARD
	struct GLUE_INFO *prGlueInfo = sysGetGlueInfo(kobj);

	if (!prGlueInfo) {
		DBGLOG(INIT, ERROR, "prGlueInfo is null\n");
		return 0;
	}
	prMemdump = &prGlueInfo->u4Memdump;
	len = sizeof(prGlueInfo->u4Memdump);
#else
	prMemdump = &g_u4Memdump;
	len = sizeof(g_u4Memdump);
#endif
	return snprintf(buf, len, "%d", *prMemdump);
}

static ssize_t memdump_store(
	struct kobject *kobj,
	struct kobj_attribute *attr,
	const char *buf,
	size_t count)
{
	int32_t i4Ret = 0;
	uint32_t *prMemdump = NULL;

#if CFG_SUPPORT_MULTI_CARD
	struct GLUE_INFO *prGlueInfo = sysGetGlueInfo(kobj);

	if (!prGlueInfo) {
		DBGLOG(INIT, ERROR, "prGlueInfo is null\n");
		return 0;
	}

	prMemdump = &prGlueInfo->u4Memdump;
	i4Ret = kstrtouint(buf, 10, &prGlueInfo->u4Memdump);
#else
	prMemdump = &g_u4Memdump;
	i4Ret = kstrtouint(buf, 10, &g_u4Memdump);
#endif

	if (i4Ret)
		DBGLOG(INIT, ERROR, "sscanf memdump fail u4Ret=%d\n", i4Ret);
	else {
		DBGLOG(INIT, INFO,
			"Set memdump to %d.\n",
			*prMemdump);
	}

	return (i4Ret == 0) ? count : 0;
}


/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

static struct kobj_attribute macaddr_attr
	= __ATTR(mac_addr, 0664, macaddr_show, macaddr_store);

static struct kobj_attribute wifiver_attr
	= __ATTR(wifiver, 0664, wifiver_show, NULL);

static struct kobj_attribute softap_attr
	= __ATTR(softap, 0664, softap_show, NULL);

static struct kobj_attribute pm_attr
	= __ATTR(pm, 0664, pm_show, pm_store);

static struct kobj_attribute memdump_attr
	= __ATTR(memdump, 0664, memdump_show, memdump_store);

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
void sysCreateMacAddr(struct GLUE_INFO *prGlueInfo)
{
	uint8_t *prMacAddrOverride = NULL;
	size_t len = 0;

	if (prGlueInfo) {
		uint8_t rMacAddr[MAC_ADDR_LEN];

		COPY_MAC_ADDR(rMacAddr,
			prGlueInfo->prAdapter->rWifiVar.aucMacAddress);

#if CFG_SUPPORT_MULTI_CARD
		prMacAddrOverride = prGlueInfo->aucMacAddrOverride;
		len = sizeof(prGlueInfo->aucMacAddrOverride);
#else
		prMacAddrOverride = aucMacAddrOverride;
		len = sizeof(aucMacAddrOverride);
#endif

		kalSnprintf(prMacAddrOverride,
			len,
			MACSTR,
			MAC2STR(rMacAddr));

		DBGLOG(INIT, TRACE,
			"Init macaddr to " MACSTR ".\n",
			MAC2STR(rMacAddr));
	}
}

void sysInitMacAddr(struct kobject *prKobj)
{
	int32_t i4Ret = 0;

	if (!prKobj) {
		DBGLOG(INIT, ERROR, "wifi_kobj is null\n");
		return;
	}

	i4Ret = sysfs_create_file(prKobj, &macaddr_attr.attr);
	if (i4Ret)
		DBGLOG(INIT, ERROR, "Unable to create macaddr entry\n");
}

void sysUninitMacAddr(struct kobject *prKobj)
{
	if (!prKobj) {
		DBGLOG(INIT, ERROR, "wifi_kobj is null\n");
		return;
	}

	sysfs_remove_file(prKobj, &macaddr_attr.attr);
}

void sysInitPM(struct kobject *prKobj)
{
	int32_t i4Ret = 0;

	if (!prKobj) {
		DBGLOG(INIT, ERROR, "wifi_kobj is null\n");
		return;
	}

	i4Ret = sysfs_create_file(prKobj, &pm_attr.attr);
	if (i4Ret)
		DBGLOG(INIT, ERROR, "Unable to create macaddr entry\n");
}

void sysUninitPM(struct kobject *prKobj)
{
	if (!prKobj) {
		DBGLOG(INIT, ERROR, "wifi_kobj is null\n");
		return;
	}

	sysfs_remove_file(prKobj, &pm_attr.attr);
}

void sysCreateWifiVer(struct GLUE_INFO *prGlueInfo)
{
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

	char aucDriverVersionStr[] = STR(NIC_DRIVER_MAJOR_VERSION) "_"
		STR(NIC_DRIVER_MINOR_VERSION) "_"
		STR(NIC_DRIVER_SERIAL_VERSION) "-"
		DRIVER_BUILD_DATE;
	uint16_t u2NvramVer = 0;
	uint8_t ucOffset = 0;
	char *prVerInfo = NULL;

#if CFG_SUPPORT_MULTI_CARD
	prVerInfo = prGlueInfo->acVerInfo;
#else
	prGlueInfo = g_prGlueInfo;
	prVerInfo = acVerInfo;
#endif

	kalMemZero(prVerInfo, MTK_INFO_MAX_SIZE);

	ucOffset += kalSnprintf(prVerInfo + ucOffset
		, MTK_INFO_MAX_SIZE - ucOffset
		, "%s\n", "Mediatek");

	ucOffset += kalSnprintf(prVerInfo + ucOffset
		, MTK_INFO_MAX_SIZE - ucOffset
		, "DRIVER_VER: %s\n", aucDriverVersionStr);

	if (prGlueInfo)
		ucOffset += kalSnprintf(prVerInfo + ucOffset
			, MTK_INFO_MAX_SIZE - ucOffset
			, "FW_VER: %s\n"
			, prGlueInfo->prAdapter->rVerInfo.aucReleaseManifest);
	else {
		ucOffset += kalSnprintf(prVerInfo + ucOffset
			, MTK_INFO_MAX_SIZE - ucOffset
			, "FW_VER: Unknown\n");
	}

	if (prGlueInfo) {
		kalCfgDataRead16(prGlueInfo,
			OFFSET_OF(struct WIFI_CFG_PARAM_STRUCT,
			u2Part1OwnVersion), &u2NvramVer);
		ucOffset += kalSnprintf(prVerInfo + ucOffset
			, MTK_INFO_MAX_SIZE - ucOffset
			, "NVRAM: 0x%x\n", u2NvramVer);
	} else {
		ucOffset += kalSnprintf(prVerInfo + ucOffset
			, MTK_INFO_MAX_SIZE - ucOffset
			, "NVRAM: Unknown\n");
	}
}

void sysInitWifiVer(struct kobject *prKobj)
{
	int32_t i4Ret = 0;
#if CFG_SUPPORT_MULTI_CARD
	struct GLUE_INFO *prGlueInfo = sysGetGlueInfo(prKobj);
#else
	struct GLUE_INFO *prGlueInfo = g_prGlueInfo;
#endif

	if (!prKobj) {
		DBGLOG(INIT, ERROR, "wifi_kobj is null\n");
		return;
	}

	i4Ret = sysfs_create_file(prKobj, &wifiver_attr.attr);
	if (i4Ret)
		DBGLOG(INIT, ERROR, "Unable to create wifiver entry\n");

	sysCreateWifiVer(prGlueInfo);
}

void sysUninitWifiVer(struct kobject *prKobj)
{
	if (!prKobj) {
		DBGLOG(INIT, ERROR, "wifi_kobj is null\n");
		return;
	}

	sysfs_remove_file(prKobj, &wifiver_attr.attr);
}

void sysCreateSoftap(struct GLUE_INFO *prGlueInfo)
{
	struct REG_INFO *prRegInfo = NULL;
	char *prSoftAPInfo = NULL;

	uint8_t ucOffset = 0;
	u_int8_t fgDbDcModeEn = FALSE;

#if (CFG_SUPPORT_MULTI_CARD == 0)
	prGlueInfo = g_prGlueInfo;
#endif

	/* Log SoftAP/hotspot information into .softap.info
	 * #Support wifi and hotspot at the same time?
	 * DualBandConcurrency=no
	 * # Supporting 5Ghz
	 * 5G=check NVRAM ucEnable5GBand
	 * # Max support client count
	 * maxClient=P2P_MAXIMUM_CLIENT_COUNT
	 * #Supporting android_net_wifi_set_Country_Code_Hal
	 * HalFn_setCountryCodeHal=yes ,
	 * call mtk_cfg80211_vendor_set_country_code
	 * #Supporting android_net_wifi_getValidChannels
	 * HalFn_getValidChannels=yes,
	 * call mtk_cfg80211_vendor_get_channel_list
	*/

	if (prGlueInfo) {
		prRegInfo = &(prGlueInfo->rRegInfo);
#if CFG_SUPPORT_DBDC
		fgDbDcModeEn = prGlueInfo->prAdapter->rWifiVar.fgDbDcModeEn;
#endif
	}

#if CFG_SUPPORT_MULTI_CARD
	prSoftAPInfo = prGlueInfo->acSoftAPInfo;
#else
	prSoftAPInfo = acSoftAPInfo;
#endif

	kalMemZero(prSoftAPInfo, MTK_INFO_MAX_SIZE);

	ucOffset = 0;

	if (prGlueInfo) {
		ucOffset += kalSnprintf(prSoftAPInfo + ucOffset
			, MTK_INFO_MAX_SIZE - ucOffset
			, "DualBandConcurrency=%s\n"
			, fgDbDcModeEn ? "yes" : "no");
	} else
		ucOffset += kalSnprintf(prSoftAPInfo + ucOffset
			, MTK_INFO_MAX_SIZE - ucOffset
			, "DualBandConcurrency=no\n");

	if (prRegInfo)
		ucOffset += kalSnprintf(prSoftAPInfo + ucOffset
			, MTK_INFO_MAX_SIZE - ucOffset
			, "5G=%s\n", prRegInfo->ucEnable5GBand ? "yes" : "no");
	else
		ucOffset += kalSnprintf(prSoftAPInfo + ucOffset
			, MTK_INFO_MAX_SIZE - ucOffset
			, "5G=yes\n");

	ucOffset += kalSnprintf(prSoftAPInfo + ucOffset
		, MTK_INFO_MAX_SIZE - ucOffset
		, "maxClient=%d\n", P2P_MAXIMUM_CLIENT_COUNT);

	ucOffset += kalSnprintf(prSoftAPInfo + ucOffset
		, MTK_INFO_MAX_SIZE - ucOffset
		, "HalFn_setCountryCodeHal=%s\n", "yes");

	ucOffset += kalSnprintf(prSoftAPInfo + ucOffset
		, MTK_INFO_MAX_SIZE - ucOffset
		, "HalFn_getValidChannels=%s\n", "yes");

	ucOffset += kalSnprintf(prSoftAPInfo + ucOffset
		, MTK_INFO_MAX_SIZE - ucOffset
		, "DualInterface=%s\n", "yes");
}

void sysInitSoftap(struct kobject *prKobj)
{
	int32_t i4Ret = 0;
#if CFG_SUPPORT_MULTI_CARD
	struct GLUE_INFO *prGlueInfo = sysGetGlueInfo(prKobj);
#else
	struct GLUE_INFO *prGlueInfo = g_prGlueInfo;
#endif

	if (!prKobj) {
		DBGLOG(INIT, ERROR, "wifi_kobj is null\n");
		return;
	}

	i4Ret = sysfs_create_file(prKobj, &softap_attr.attr);
	if (i4Ret)
		DBGLOG(INIT, ERROR, "Unable to create softap entry\n");

	sysCreateSoftap(prGlueInfo);
}

void sysUninitSoftap(struct kobject *prKobj)
{
	if (!prKobj) {
		DBGLOG(INIT, ERROR, "wifi_kobj is null\n");
		return;
	}

	sysfs_remove_file(prKobj, &softap_attr.attr);
}

void sysInitMemdump(struct kobject *prKobj)
{
	int32_t i4Ret = 0;

	if (!prKobj) {
		DBGLOG(INIT, ERROR, "wifi_kobj is null\n");
		return;
	}

	i4Ret = sysfs_create_file(prKobj, &memdump_attr.attr);
	if (i4Ret)
		DBGLOG(INIT, ERROR, "Unable to create softap entry\n");
}

void sysUninitMemdump(struct kobject *prKobj)
{
	if (!prKobj) {
		DBGLOG(INIT, ERROR, "wifi_kobj is null\n");
		return;
	}

	sysfs_remove_file(prKobj, &memdump_attr.attr);
}

int32_t sysCreateFsEntry(struct GLUE_INFO *prGlueInfo)
{
	DBGLOG(INIT, TRACE, "[%s]\n", __func__);

#if (CFG_SUPPORT_MULTI_CARD == 0)
	g_prGlueInfo = prGlueInfo;
#endif

	sysCreateMacAddr(prGlueInfo);
	pm_EnterCtiaMode(prGlueInfo);
	sysCreateWifiVer(prGlueInfo);
	sysCreateSoftap(prGlueInfo);

	return 0;
}

int32_t sysRemoveSysfs(void)
{
#if (CFG_SUPPORT_MULTI_CARD == 0)
	g_prGlueInfo = NULL;
#endif

	return 0;
}

int32_t sysInitFs(struct GLUE_INFO *prGlueInfo)
{
#if CFG_SUPPORT_MULTI_CARD
	struct net_device *prDev = prGlueInfo->prDevHandler;
	uint32_t u4Idx = 0;
	uint8_t rootDir[10] = "";
#endif
	struct kobject *prKobj = NULL;

	DBGLOG(INIT, TRACE, "[%s]\n", __func__);

#if CFG_SUPPORT_MULTI_CARD
	u4Idx = wlanGetDevIdx(prDev);

	if (u4Idx >= CFG_MAX_WLAN_DEVICES)
		return -1;

	// Initialize the sysFs info
	kalSprintf(prGlueInfo->aucMacAddrOverride, "%s", DEFAULT_MAC_ADDRESS);
	prGlueInfo->fgIsSysFsMacAddrOverride = FALSE;
	prGlueInfo->i4PM = -1;
#if BUILD_QA_DBG
	prGlueInfo->u4Memdump = 3;
#else
	prGlueInfo->u4Memdump = 2;
#endif

	kalSprintf(rootDir, "wifi%d", u4Idx);

	prGlueInfo->wlan_kobj = kobject_create_and_add(rootDir, NULL);
	prKobj = prGlueInfo->wlan_kobj;
#else
	wifi_kobj = kobject_create_and_add("wifi", NULL);
	prKobj = wifi_kobj;
#endif

	kobject_get(prKobj);
	kobject_uevent(prKobj, KOBJ_ADD);

	sysInitMacAddr(prKobj);
	sysInitWifiVer(prKobj);
	sysInitSoftap(prKobj);
	sysInitPM(prKobj);
	sysInitMemdump(prKobj);

	return 0;
}

int32_t sysUninitSysFs(struct GLUE_INFO *prGlueInfo)
{
#if CFG_SUPPORT_MULTI_CARD
	struct kobject **pprKobj = &prGlueInfo->wlan_kobj;
#else
	struct kobject **pprKobj = &wifi_kobj;
#endif

	DBGLOG(INIT, TRACE, "[%s]\n", __func__);

	sysUninitMemdump(*pprKobj);
	sysUninitPM(*pprKobj);
	sysUninitSoftap(*pprKobj);
	sysUninitWifiVer(*pprKobj);
	sysUninitMacAddr(*pprKobj);

	kobject_uevent(*pprKobj, KOBJ_REMOVE);
	kobject_put(*pprKobj);
	*pprKobj = NULL;

	return 0;
}

void sysMacAddrOverride(struct GLUE_INFO *prGlueInfo, uint8_t *prMacAddr)
{
#if CFG_SUPPORT_MULTI_CARD
	u_int8_t fgIsOverride = prGlueInfo->fgIsSysFsMacAddrOverride;
	uint8_t *prMacAddrOverride = prGlueInfo->aucMacAddrOverride;
#else
	u_int8_t fgIsOverride = fgIsMacAddrOverride;
	uint8_t *prMacAddrOverride = aucMacAddrOverride;
#endif

	DBGLOG(INIT, TRACE,
		"Override=%d\n", fgIsOverride);

	if (!fgIsOverride)
		return;

	wlanHwAddrToBin(
		prMacAddrOverride,
		prMacAddr);

	DBGLOG(INIT, TRACE,
		"Init macaddr to " MACSTR ".\n",
		MAC2STR(prMacAddr));
}

#endif

#if (CFG_SUPPORT_SNIFFER_RADIOTAP == 1)
#if (CFG_SUPPORT_MULTI_CARD == 0)
static struct dentry *dbgFsDir;
#endif

static int debugfs_u8_get(void *data, uint64_t *val)
{
	*val = *(uint8_t *)data;
	return 0;
}

static int debugfs_u8_set(void *data, uint64_t val)
{
	*(uint8_t *)data = val;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(fops_u8, debugfs_u8_get, debugfs_u8_set, "%llu\n");

static int debugfs_u16_get(void *data, uint64_t *val)
{
	*val = *(uint16_t *)data;
	return 0;
}

static int debugfs_u16_set(void *data, uint64_t val)
{
	*(uint16_t *)data = val;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(fops_u16, debugfs_u16_get, debugfs_u16_set, "%llu\n");

int32_t sysCreateMonDbgFs(struct GLUE_INFO *prGlueInfo)
{
	struct dentry *dbgFsFile;
	uint8_t rootDir[50] = { 0 };
#if CFG_SUPPORT_MULTI_CARD
	struct dentry **pprDbgFsDir = &prGlueInfo->dbgFsDir;
	struct net_device *prDev = prGlueInfo->prDevHandler;
	uint32_t u4Idx = 0;
#else
	struct dentry **pprDbgFsDir = &dbgFsDir;
#endif

#if CFG_SUPPORT_MULTI_CARD
	u4Idx = wlanGetDevIdx(prDev);

	if (u4Idx < CFG_MAX_WLAN_DEVICES)
		kalSprintf(rootDir, "mtk_mon_dbgfs%d", u4Idx);
	else
#endif
		kalSprintf(rootDir, "mtk_mon_dbgfs");

	*pprDbgFsDir = debugfs_create_dir(rootDir, NULL);
	if (!(*pprDbgFsDir)) {
		DBGLOG(INIT, ERROR,
				"dbgFsDir is null for mtk_mon_dbgfs\n");
		return 0;
	}

	/* /sys/kernel/debug/mtk_mon_dbgfs/hemu_aid, mode: wr */
	dbgFsFile = debugfs_create_file("hemu_aid",
		0666, *pprDbgFsDir, &prGlueInfo->u2Aid, &fops_u16);

	/* /sys/kernel/debug/mtk_mon_dbgfs/band_idx, mode: wr */
	dbgFsFile = debugfs_create_file("band_idx",
		0666, *pprDbgFsDir, &prGlueInfo->ucBandIdx, &fops_u8);

	/* /sys/kernel/debug/mtk_mon_dbgfs/drop_fcs_err, mode: wr */
	dbgFsFile = debugfs_create_file("drop_fcs_err",
		0666, *pprDbgFsDir, &prGlueInfo->fgDropFcsErrorFrame, &fops_u8);

	return 0;
}

void sysRemoveMonDbgFs(struct GLUE_INFO *prGlueInfo)
{
#if CFG_SUPPORT_MULTI_CARD
	struct dentry **pprDbgFsDir = &prGlueInfo->dbgFsDir;
#else
	struct dentry **pprDbgFsDir = &dbgFsDir;
#endif
	debugfs_remove_recursive(*pprDbgFsDir);
}
#endif
