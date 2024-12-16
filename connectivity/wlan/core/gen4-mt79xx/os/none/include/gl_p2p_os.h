/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 * Copyright (c) 2016 MediaTek Inc.
 */
/*
 ** Id:
 * //Department/DaVinci/TRUNK/MT6620_5931_WiFi_Driver/
 * os/linux/include/gl_p2p_os.h#28
 */

/*! \file   gl_p2p_os.h
 *    \brief  List the external reference to OS for p2p GLUE Layer.
 *
 *    In this file we define the data structure - GLUE_INFO_T to
 *    store those objects we acquired from OS -
 *    e.g. TIMER, SPINLOCK, NET DEVICE ... . And all the
 *    external reference (header file, extern func() ..) to OS
 *    for GLUE Layer should also list down here.
 */

#ifndef _GL_P2P_OS_H
#define _GL_P2P_OS_H

/******************************************************************************
 *                         C O M P I L E R   F L A G S
 ******************************************************************************
 */

/******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 ******************************************************************************
 */

/******************************************************************************
 *                    E X T E R N A L   V A R I A B L E
 ******************************************************************************
 */
#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
extern const struct net_device_ops p2p_netdev_ops;
#endif

/******************************************************************************
 *                              C O N S T A N T S
 ******************************************************************************
 */

/******************************************************************************
 *                                 M A C R O S
 ******************************************************************************
 */

/* For SET_STRUCT/GET_STRUCT */
#define OID_SET_GET_STRUCT_LENGTH		4096

#define MAX_P2P_IE_SIZE	5

#define P2P_MAXIMUM_CLIENT_COUNT                    16
#define P2P_DEFAULT_CLIENT_COUNT 4

/******************************************************************************
 *                             D A T A   T Y P E S
 ******************************************************************************
 */

/******************************************************************************
 *                            P U B L I C   D A T A
 ******************************************************************************
 */

extern struct net_device *g_P2pPrDev;
extern struct wireless_dev *gprP2pWdev;
extern struct wireless_dev *gprP2pRoleWdev[KAL_P2P_NUM];

/******************************************************************************
 *                           P R I V A T E   D A T A
 ******************************************************************************
 */

/******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 ******************************************************************************
 */

struct GL_P2P_INFO {

	/* P2P Device interface handle */
	/*only first p2p have this devhandler*/
	struct net_device *prDevHandler;
	/*struct net_device *prRoleDevHandler;*//* TH3 multiple P2P */

	struct net_device *aprRoleHandler;

	/* Todo : should move to the glueinfo or not*/
	/*UINT_8 ucRoleInterfaceNum;*//* TH3 multiple P2P */

#if CFG_ENABLE_WIFI_DIRECT_CFG_80211
	/* cfg80211 */
	struct wireless_dev *prWdev;
	/*struct wireless_dev *prRoleWdev[KAL_P2P_NUM];*//* TH3 multiple P2P */

	/*struct cfg80211_scan_request *prScanRequest;*//* TH3 multiple P2P */

	/*UINT_64 u8Cookie;*//* TH3 multiple P2P */

	/* Generation for station list update. */
	int32_t i4Generation;

	/*UINT_32 u4OsMgmtFrameFilter;*//* TH3 multiple P2P */

#endif

	/* Device statistics */
	/*struct net_device_stats rNetDevStats;*//* TH3 multiple P2P */

	/* glue layer variables */
	/*move to glueinfo->adapter */
	/* BOOLEAN                     fgIsRegistered; */
	/*UINT_32 u4FreqInKHz;*//* TH3 multiple P2P */	/* frequency */
	/* 0: P2P Device, 1: Group Client, 2: Group Owner */
	uint8_t ucRole;
	/*UINT_8 ucIntent;*//* TH3 multiple P2P */	/* range: 0-15 */
	/* 0: Search & Listen, 1: Scan without probe response */
	/*UINT_8 ucScanMode;*//* TH3 multiple P2P */

	/*ENUM_PARAM_MEDIA_STATE_T eState;*//* TH3 multiple P2P */
	/*UINT_32 u4PacketFilter;*//* TH3 multiple P2P */
	/* TH3 multiple P2P */
	/*PARAM_MAC_ADDRESS aucMCAddrList[MAX_NUM_GROUP_ADDR];*/

	/* connection-requested peer information *//* TH3 multiple P2P */
	/*UINT_8 aucConnReqDevName[32];*//* TH3 multiple P2P */
	/*INT_32 u4ConnReqNameLength;*//* TH3 multiple P2P */
	/*PARAM_MAC_ADDRESS rConnReqPeerAddr;*//* TH3 multiple P2P */
	/* For invitation group. */
	/*PARAM_MAC_ADDRESS rConnReqGroupAddr;*//* TH3 multiple P2P */
	/*UINT_8 ucConnReqDevType;*//* TH3 multiple P2P */
	/*INT_32 i4ConnReqConfigMethod;*//* TH3 multiple P2P */
	/*INT_32 i4ConnReqActiveConfigMethod;*//* TH3 multiple P2P */

	uint32_t u4CipherPairwise;
	/*UINT_8 ucWSCRunning;*//* TH3 multiple P2P */

	/* 0: beacon, 1: probe req, 2:probe response, 3: assoc response */
	uint8_t aucWSCIE[4][400];
	uint16_t u2WSCIELen[4];

	uint8_t aucP2PIE[MAX_P2P_IE_SIZE][400];
	uint16_t u2P2PIELen[MAX_P2P_IE_SIZE];

#if CFG_SUPPORT_WFD
	/* 0 for beacon, 1 for probe req, 2 for probe response */
	uint8_t aucWFDIE[400];
	uint16_t u2WFDIELen;
	/* Save the other IE for probe resp */
#if CFG_SUPPORT_CUSTOM_VENDOR_IE
	uint8_t aucVenderIE[1024];
	uint16_t u2VenderIELen;
#endif
#endif

	/*UINT_8 ucOperatingChnl;*//* TH3 multiple P2P */
	/*UINT_8 ucInvitationType;*//* TH3 multiple P2P */

	/*UINT_32 u4InvStatus;*//* TH3 multiple P2P */

	/* For SET_STRUCT/GET_STRUCT */
	/*UINT_8 aucOidBuf[OID_SET_GET_STRUCT_LENGTH];*//* TH3 multiple P2P */

#if 1				/* CFG_SUPPORT_ANTI_PIRACY */
	/*UINT_8 aucSecCheck[256];*//* TH3 multiple P2P */
	/*UINT_8 aucSecCheckRsp[256];*//* TH3 multiple P2P */
#endif

#if (CFG_SUPPORT_DFS_MASTER == 1)
	struct cfg80211_chan_def *chandef;
	uint32_t cac_time_ms;
#endif

#if CFG_SUPPORT_HOTSPOT_WPS_MANAGER
	uint8_t aucblackMACList[P2P_MAXIMUM_CLIENT_COUNT][PARAM_MAC_ADDR_LEN];
	uint8_t ucMaxClients;
#endif

#if CFG_SUPPORT_HOTSPOT_OPTIMIZATION
	/*BOOLEAN fgEnableHotspotOptimization;*//* TH3 multiple P2P */
	/*UINT_32 u4PsLevel;*//* TH3 multiple P2P */
#endif

	enum ENUM_CHNL_SWITCH_POLICY eChnlSwitchPolicy;
};

struct GL_P2P_DEV_INFO {
#if CFG_ENABLE_WIFI_DIRECT_CFG_80211
	struct cfg80211_scan_request *prScanRequest;
#if 0
	struct cfg80211_scan_request rBackupScanRequest;
#endif
	uint64_t u8Cookie;
	uint32_t u4OsMgmtFrameFilter;
#endif
	uint32_t u4PacketFilter;
	uint8_t aucMCAddrList[MAX_NUM_GROUP_ADDR][PARAM_MAC_ADDR_LEN];
	uint8_t ucWSCRunning;
};

#ifdef CONFIG_NL80211_TESTMODE
struct NL80211_DRIVER_TEST_PRE_PARAMS {
	uint16_t idx_mode;
	uint16_t idx;
	uint32_t value;
};

struct NL80211_DRIVER_TEST_PARAMS {
	uint32_t index;
	uint32_t buflen;
};

/* P2P Sigma*/
struct NL80211_DRIVER_P2P_SIGMA_PARAMS {
	struct NL80211_DRIVER_TEST_PARAMS hdr;
	uint32_t idx;
	uint32_t value;
};

/* Hotspot Client Management */
struct NL80211_DRIVER_hotspot_block_PARAMS {
	struct NL80211_DRIVER_TEST_PARAMS hdr;
	uint8_t ucblocked;
	uint8_t aucBssid[MAC_ADDR_LEN];
};

/* Hotspot Management set config */
struct NL80211_DRIVER_HOTSPOT_CONFIG_PARAMS {
	struct NL80211_DRIVER_TEST_PARAMS hdr;
	uint32_t idx;
	uint32_t value;
};

#if CFG_SUPPORT_WFD
struct NL80211_DRIVER_WFD_PARAMS {
	struct NL80211_DRIVER_TEST_PARAMS hdr;
	uint32_t WfdCmdType;
	uint8_t WfdEnable;
	uint8_t WfdCoupleSinkStatus;
	uint8_t WfdSessionAvailable;
	uint8_t WfdSigmaMode;
	uint16_t WfdDevInfo;
	uint16_t WfdControlPort;
	uint16_t WfdMaximumTp;
	uint16_t WfdExtendCap;
	uint8_t WfdCoupleSinkAddress[MAC_ADDR_LEN];
	uint8_t WfdAssociatedBssid[MAC_ADDR_LEN];
	uint8_t WfdVideoIp[4];
	uint8_t WfdAudioIp[4];
	uint16_t WfdVideoPort;
	uint16_t WfdAudioPort;
	uint32_t WfdFlag;
	uint32_t WfdPolicy;
	uint32_t WfdState;
	/* Include Subelement ID, length */
	uint8_t WfdSessionInformationIE[24 * 8];
	uint16_t WfdSessionInformationIELen;
	uint8_t aucReserved1[2];
	uint8_t aucWfdPrimarySinkMac[MAC_ADDR_LEN];
	uint8_t aucWfdSecondarySinkMac[MAC_ADDR_LEN];
	uint32_t WfdAdvanceFlag;
	/* Group 1 64 bytes */
	uint8_t aucWfdLocalIp[4];
	uint16_t WfdLifetimeAc2;	/* Unit is 2 TU */
	uint16_t WfdLifetimeAc3;	/* Unit is 2 TU */
	uint16_t WfdCounterThreshold;	/* Unit is ms */
	uint8_t aucReserved2[54];
	/* Group 3 64 bytes */
	uint8_t aucReserved3[64];
	/* Group 3 64 bytes */
	uint8_t aucReserved4[64];
};
#endif
#endif

/******************************************************************************
 *                            P U B L I C   D A T A
 ******************************************************************************
 */

/******************************************************************************
 *                           P R I V A T E   D A T A
 ******************************************************************************
 */

u_int8_t p2pRegisterToWlan(struct GLUE_INFO *prGlueInfo);

u_int8_t p2pUnregisterToWlan(struct GLUE_INFO *prGlueInfo);

#ifdef CFG_REMIND_IMPLEMENT
#define p2pLaunch(_prGlueInfo) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define p2pRemove(_prGlueInfo) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define p2pSetMode(_ucAPMode) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#else
u_int8_t p2pLaunch(struct GLUE_INFO *prGlueInfo);

u_int8_t p2pRemove(struct GLUE_INFO *prGlueInfo);

void p2pSetMode(IN uint8_t ucAPMode);
#endif

u_int8_t glRegisterP2P(struct GLUE_INFO *prGlueInfo,
		const char *prDevName,
		const char *prDevName2,
		uint8_t ucApMode);

int glSetupP2P(struct GLUE_INFO *prGlueInfo,
		struct wireless_dev *prP2pWdev,
		struct net_device *prP2pDev,
		int u4Idx,
		u_int8_t fgIsApMode);

u_int8_t glUnregisterP2P(struct GLUE_INFO *prGlueInfo, uint8_t ucIdx);

u_int8_t p2pNetRegister(struct GLUE_INFO *prGlueInfo,
		u_int8_t fgIsRtnlLockAcquired);

u_int8_t p2pNetUnregister(struct GLUE_INFO *prGlueInfo,
		u_int8_t fgIsRtnlLockAcquired);


u_int8_t p2PAllocInfo(IN struct GLUE_INFO *prGlueInfo, IN uint8_t ucIdex);
u_int8_t p2PFreeInfo(struct GLUE_INFO *prGlueInfo, uint8_t ucIdx);

void p2pSetSuspendMode(struct GLUE_INFO *prGlueInfo, u_int8_t fgEnable);
u_int8_t glP2pCreateWirelessDevice(struct GLUE_INFO *prGlueInfo);
void glP2pDestroyWirelessDevice(void);
void p2pUpdateChannelTableByDomain(struct GLUE_INFO *prGlueInfo);
#endif
