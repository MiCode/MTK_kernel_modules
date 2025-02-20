// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file gl_wext_priv.c
 *    \brief This file includes private ioctl support.
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
#include "gl_wext_priv.h"
#include "gl_cmd_validate.h"
#include "gl_hook_api.h"

#if CFG_SUPPORT_QA_TOOL
#include "gl_ate_agent.h"
#include "gl_qa_agent.h"
#endif

#if CFG_SUPPORT_WAPI
#include "gl_sec.h"
#endif
#if CFG_ENABLE_WIFI_DIRECT
#include "gl_p2p_os.h"
#endif

#if CFG_SUPPORT_MET_LOG
#include "gl_met_log.h"
#endif

#if CFG_SUPPORT_NAN
#include "nan_data_engine.h"
#include "nan_sec.h"
#endif

#if CFG_SUPPORT_CSI
#include "gl_csi.h"
#endif

#if (CFG_MTK_SUPPORT_LIGHT_MDDP == 1)
#include "mddp.h"
#endif /* CFG_MTK_SUPPORT_LIGHT_MDDP */
/*
 * #if CFG_SUPPORT_QA_TOOL
 * extern UINT_16 g_u2DumpIndex;
 * #endif
 */

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define	NUM_SUPPORTED_OIDS      ARRAY_SIZE(arWlanOidReqTable)
#if CFG_SUPPORT_NAN
#define CMD_OID_BUF_LENGTH 8000
#else
#define	CMD_OID_BUF_LENGTH	4096
#endif

#if (CFG_SUPPORT_TWT == 1)
#define CMD_TWT_ACTION_SIXTEEN_PARAMS  16
#define CMD_TWT_ACTION_FIFTEEN_PARAMS  15
#define CMD_TWT_ACTION_TWELVE_PARAMS   12
#define CMD_TWT_ACTION_TEN_PARAMS      10
#define CMD_TWT_ACTION_NINE_PARAMS     9
#define CMD_TWT_ACTION_SIX_PARAMS      6
#define CMD_TWT_ACTION_FIVE_PARAMS     5
#define CMD_TWT_ACTION_FOUR_PARAMS     4
#define CMD_TWT_ACTION_THREE_PARAMS    3
#define CMD_TWT_MAX_PARAMS CMD_TWT_ACTION_FIFTEEN_PARAMS
#endif

#define TO_STR(value) #value
#define INT_TO_STR(value) TO_STR(value)

#define CMD_SMPS_ACTION_FOUR_PARAMS       4
#define CMD_SMPS_ACTION_THREE_PARAMS      3
#define CMD_SMPS_ACTION_TWO_PARAMS        2
#define CMD_SMPS_MAX_PARAMS CMD_SMPS_ACTION_FOUR_PARAMS

#if CFG_SUPPORT_PCIE_GEN_SWITCH
#define MAX_PCIE_SPEED		3
#endif
/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

static int
priv_get_ndis(struct net_device *prNetDev,
	      struct NDIS_TRANSPORT_STRUCT *prNdisReq,
	      uint32_t *pu4OutputLen);

static int
priv_set_ndis(struct net_device *prNetDev,
	      struct NDIS_TRANSPORT_STRUCT *prNdisReq,
	      uint32_t *pu4OutputLen);

static u_int8_t reqSearchSupportedOidEntry(uint32_t rOid,
		struct WLAN_REQ_ENTRY **ppWlanReqEntry);

static uint32_t
reqExtSetAcpiDevicePowerState(struct GLUE_INFO
			      *prGlueInfo,
			      void *pvSetBuffer, uint32_t u4SetBufferLen,
			      uint32_t *pu4SetInfoLen);

static uint32_t glWextRfTestSetTestMode(void *ptr,
		void *pvSetBuffer,
		uint32_t u4SetBufferLen,
		uint32_t *pu4SetInfoLen);

static uint32_t glWextRfTestSetAbortTestMode(void *ptr,
		void *pvSetBuffer,
		uint32_t u4SetBufferLen,
		uint32_t *pu4SetInfoLen);

#if (CFG_WIFI_ISO_DETECT == 1)
static int priv_driver_iso_detect(struct GLUE_INFO *prGlueInfo,
				struct COEX_CMD_HANDLER *prCoexCmdHandler,
				signed char *argv[]);
#endif

/*******************************************************************************
 *                       P R I V A T E   D A T A
 *******************************************************************************
 */
static uint8_t aucOidBuf[CMD_OID_BUF_LENGTH] = { 0 };

/* OID processing table */
/* Order is important here because the OIDs should be in order of
 *  increasing value for binary searching.
 */
static struct WLAN_REQ_ENTRY arWlanOidReqTable[] = {
	/* General Operational Characteristics */

	/* Ethernet Operational Characteristics */
	{
		OID_802_3_CURRENT_ADDRESS,
		"OID_802_3_CURRENT_ADDRESS",
		TRUE, TRUE, ENUM_OID_DRIVER_CORE, 6,
		(PFN_OID_HANDLER_FUNC_REQ) wlanoidQueryCurrentAddr,
		NULL
	},

	/* OID_802_3_MULTICAST_LIST */
	/* OID_802_3_MAXIMUM_LIST_SIZE */
	/* Ethernet Statistics */

	/* NDIS 802.11 Wireless LAN OIDs */
	{
		OID_802_11_SUPPORTED_RATES,
		"OID_802_11_SUPPORTED_RATES",
		TRUE, FALSE, ENUM_OID_DRIVER_CORE,
		(sizeof(uint8_t) * PARAM_MAX_LEN_RATES_EX),
		(PFN_OID_HANDLER_FUNC_REQ) wlanoidQuerySupportedRates,
		NULL
	}
	,
	/*
	 *  {OID_802_11_CONFIGURATION,
	 *  "OID_802_11_CONFIGURATION",
	 *  TRUE, TRUE, ENUM_OID_GLUE_EXTENSION,
	 *  sizeof(struct PARAM_802_11_CONFIG),
	 *  (PFN_OID_HANDLER_FUNC_REQ)reqExtQueryConfiguration,
	 *  (PFN_OID_HANDLER_FUNC_REQ)reqExtSetConfiguration},
	 */
	{
		OID_PNP_SET_POWER,
		"OID_PNP_SET_POWER",
		TRUE, FALSE, ENUM_OID_GLUE_EXTENSION,
		sizeof(enum PARAM_DEVICE_POWER_STATE),
		NULL,
		(PFN_OID_HANDLER_FUNC_REQ) reqExtSetAcpiDevicePowerState
	}
	,

	/* Custom OIDs */
	{
		OID_CUSTOM_OID_INTERFACE_VERSION,
		"OID_CUSTOM_OID_INTERFACE_VERSION",
		TRUE, FALSE, ENUM_OID_DRIVER_CORE, 4,
		(PFN_OID_HANDLER_FUNC_REQ) wlanoidQueryOidInterfaceVersion,
		NULL
	}
	,
#if BUILD_QA_DBG
	{
		OID_CUSTOM_MCR_RW,
		"OID_CUSTOM_MCR_RW",
		TRUE, TRUE, ENUM_OID_DRIVER_CORE,
		sizeof(struct PARAM_CUSTOM_MCR_RW_STRUCT),
		(PFN_OID_HANDLER_FUNC_REQ) wlanoidQueryMcrRead,
		(PFN_OID_HANDLER_FUNC_REQ) wlanoidSetMcrWrite}
	,
	{
		OID_CUSTOM_EEPROM_RW,
		"OID_CUSTOM_EEPROM_RW",
		TRUE, TRUE, ENUM_OID_DRIVER_CORE,
		sizeof(struct PARAM_CUSTOM_EEPROM_RW_STRUCT),
		(PFN_OID_HANDLER_FUNC_REQ) wlanoidQueryEepromRead,
		(PFN_OID_HANDLER_FUNC_REQ) wlanoidSetEepromWrite
	}
	,
#endif /* BUILD_QA_DBG */
	{
		OID_CUSTOM_SW_CTRL,
		"OID_CUSTOM_SW_CTRL",
		TRUE, TRUE, ENUM_OID_DRIVER_CORE,
		sizeof(struct PARAM_CUSTOM_SW_CTRL_STRUCT),
		(PFN_OID_HANDLER_FUNC_REQ) wlanoidQuerySwCtrlRead,
		(PFN_OID_HANDLER_FUNC_REQ) wlanoidSetSwCtrlWrite
	}
	,
#if CFG_SUPPORT_QA_TOOL
	{
		OID_CUSTOM_TEST_MODE,
		"OID_CUSTOM_TEST_MODE",
		FALSE, FALSE, ENUM_OID_GLUE_EXTENSION, 0,
		NULL,
		(PFN_OID_HANDLER_FUNC_REQ) glWextRfTestSetTestMode
	}
	,
	{
		OID_CUSTOM_ABORT_TEST_MODE,
		"OID_CUSTOM_ABORT_TEST_MODE",
		FALSE, FALSE, ENUM_OID_GLUE_EXTENSION, 0,
		NULL,
		(PFN_OID_HANDLER_FUNC_REQ) glWextRfTestSetAbortTestMode
	}
	,
	{
		OID_CUSTOM_MTK_WIFI_TEST,
		"OID_CUSTOM_MTK_WIFI_TEST",
		/* PeiHsuan Temp Remove this check for workaround Gen2/Gen3 EM
		 * Mode Modification
		 */
		/* TRUE, TRUE, ENUM_OID_DRIVER_CORE,
		 * sizeof(PARAM_MTK_WIFI_TEST_STRUCT_T),
		 */
		FALSE, FALSE, ENUM_OID_DRIVER_CORE, 0,
		(PFN_OID_HANDLER_FUNC_REQ) wlanoidRftestQueryAutoTest,
		(PFN_OID_HANDLER_FUNC_REQ) wlanoidRftestSetAutoTest
	}
	,
	{
		OID_CUSTOM_TEST_ICAP_MODE,
		"OID_CUSTOM_TEST_ICAP_MODE",
		FALSE, FALSE, ENUM_OID_DRIVER_CORE, 0,
		NULL,
		(PFN_OID_HANDLER_FUNC_REQ) wlanoidRftestSetTestIcapMode
	}
	,
#endif
	/* OID_CUSTOM_EMULATION_VERSION_CONTROL */

	/* BWCS */
#if CFG_SUPPORT_BCM && CFG_SUPPORT_BCM_BWCS
	{
		OID_CUSTOM_BWCS_CMD,
		"OID_CUSTOM_BWCS_CMD",
		FALSE, FALSE, ENUM_OID_DRIVER_CORE, sizeof(struct PTA_IPC),
		(PFN_OID_HANDLER_FUNC_REQ) wlanoidQueryBT,
		(PFN_OID_HANDLER_FUNC_REQ) wlanoidSetBT
	}
	,
#endif
#if BUILD_QA_DBG
	{
		OID_CUSTOM_MTK_NVRAM_RW,
		"OID_CUSTOM_MTK_NVRAM_RW",
		TRUE, TRUE, ENUM_OID_DRIVER_CORE,
		sizeof(struct PARAM_CUSTOM_EEPROM_RW_STRUCT),
		(PFN_OID_HANDLER_FUNC_REQ) wlanoidQueryNvramRead,
		(PFN_OID_HANDLER_FUNC_REQ) wlanoidSetNvramWrite}
	,
#endif
	{
		OID_CUSTOM_CFG_SRC_TYPE,
		"OID_CUSTOM_CFG_SRC_TYPE",
		TRUE, FALSE, ENUM_OID_DRIVER_CORE,
		sizeof(enum ENUM_CFG_SRC_TYPE),
		(PFN_OID_HANDLER_FUNC_REQ) wlanoidQueryCfgSrcType,
		NULL
	}
	,
	{
		OID_CUSTOM_EEPROM_TYPE,
		"OID_CUSTOM_EEPROM_TYPE",
		TRUE, FALSE, ENUM_OID_DRIVER_CORE,
		sizeof(enum ENUM_EEPROM_TYPE),
		(PFN_OID_HANDLER_FUNC_REQ) wlanoidQueryEepromType,
		NULL
	}
	,
#if CFG_SUPPORT_WAPI
	{
		OID_802_11_WAPI_MODE,
		"OID_802_11_WAPI_MODE",
		FALSE, TRUE, ENUM_OID_DRIVER_CORE, 4,
		NULL,
		(PFN_OID_HANDLER_FUNC_REQ) wlanoidSetWapiMode
	}
	,
	{
		OID_802_11_WAPI_ASSOC_INFO,
		"OID_802_11_WAPI_ASSOC_INFO",
		FALSE, FALSE, ENUM_OID_DRIVER_CORE, 0,
		NULL,
		(PFN_OID_HANDLER_FUNC_REQ) wlanoidSetWapiAssocInfo
	}
	,
	{
		OID_802_11_SET_WAPI_KEY,
		"OID_802_11_SET_WAPI_KEY",
		FALSE, FALSE, ENUM_OID_DRIVER_CORE,
		sizeof(struct PARAM_WPI_KEY),
		NULL,
		(PFN_OID_HANDLER_FUNC_REQ) wlanoidSetWapiKey
	}
	,
#endif
#if CFG_SUPPORT_LOWLATENCY_MODE
	/* Note: we should put following code in order */
	{
		OID_CUSTOM_LOWLATENCY_MODE,	/* 0xFFA0CC00 */
		"OID_CUSTOM_LOWLATENCY_MODE",
		FALSE, FALSE, ENUM_OID_DRIVER_CORE, sizeof(uint32_t) * 7,
		NULL,
		(PFN_OID_HANDLER_FUNC_REQ) wlanoidSetLowLatencyMode
	}
	,
#endif /* CFG_SUPPORT_LOWLATENCY_MODE */
	{
		OID_IPC_WIFI_LOG_UI,
		"OID_IPC_WIFI_LOG_UI",
		TRUE, FALSE, ENUM_OID_DRIVER_CORE,
		sizeof(struct PARAM_WIFI_LOG_LEVEL_UI),
		(PFN_OID_HANDLER_FUNC_REQ) wlanoidQueryWifiLogLevelSupport,
		NULL
	}
	,
	{
		OID_IPC_WIFI_LOG_LEVEL,
		"OID_IPC_WIFI_LOG_LEVEL",
		TRUE, FALSE, ENUM_OID_DRIVER_CORE,
		sizeof(struct PARAM_WIFI_LOG_LEVEL),
		(PFN_OID_HANDLER_FUNC_REQ) wlanoidQueryWifiLogLevel,
		(PFN_OID_HANDLER_FUNC_REQ) wlanoidSetWifiLogLevel
	}
	,
#if CFG_SUPPORT_ANT_SWAP
	{
		OID_CUSTOM_QUERY_ANT_SWAP_CAPABILITY,	/* 0xFFA0CD00 */
		"OID_CUSTOM_QUERY_ANT_SWAP_CAPABILITY",
		TRUE, FALSE, ENUM_OID_DRIVER_CORE, sizeof(uint32_t),
		(PFN_OID_HANDLER_FUNC_REQ) wlanoidQueryAntennaSwap,
		NULL
	}
	,
#endif
};

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

static int compat_priv(struct net_device *prNetDev,
	     struct iw_request_info *prIwReqInfo,
		 union iwreq_data *prIwReqData, char *pcExtra,
	     int (*priv_func)(struct net_device *prNetDev,
	     struct iw_request_info *prIwReqInfo,
		 union iwreq_data *prIwReqData, char *pcExtra))
{
	struct iw_point *prIwp;
	int ret = 0;
#ifdef CONFIG_COMPAT
	struct compat_iw_point *iwp_compat = NULL;
	struct iw_point iwp = {0};
#endif

	if (!prIwReqData)
		return -EINVAL;

#ifdef CONFIG_COMPAT
	if (prIwReqInfo->flags & IW_REQUEST_FLAG_COMPAT) {
		iwp_compat = (struct compat_iw_point *) &prIwReqData->data;
		iwp.pointer = compat_ptr(iwp_compat->pointer);
		iwp.length = iwp_compat->length;
		iwp.flags = iwp_compat->flags;
		prIwp = &iwp;
	} else
#endif
	prIwp = &prIwReqData->data;


	ret = priv_func(prNetDev, prIwReqInfo,
				(union iwreq_data *)prIwp, pcExtra);

#ifdef CONFIG_COMPAT
	if (prIwReqInfo->flags & IW_REQUEST_FLAG_COMPAT) {
		iwp_compat->pointer = ptr_to_compat(iwp.pointer);
		iwp_compat->length = iwp.length;
		iwp_compat->flags = iwp.flags;
	}
#endif
	return ret;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Dispatching function for private ioctl region (SIOCIWFIRSTPRIV ~
 *   SIOCIWLASTPRIV).
 *
 * \param[in] prNetDev Net device requested.
 * \param[in] prIfReq Pointer to ifreq structure.
 * \param[in] i4Cmd Command ID between SIOCIWFIRSTPRIV and SIOCIWLASTPRIV.
 *
 * \retval 0 for success.
 * \retval -EOPNOTSUPP If cmd is not supported.
 * \retval -EFAULT For fail.
 *
 */
/*----------------------------------------------------------------------------*/
int priv_support_ioctl(struct net_device *prNetDev,
		       struct ifreq *prIfReq, int i4Cmd)
{
	/* prIfReq is verified in the caller function wlanDoIOCTL() */
	struct iwreq *prIwReq = (struct iwreq *)prIfReq;
	struct iw_request_info rIwReqInfo;

	/* prNetDev is verified in the caller function wlanDoIOCTL() */

	/* Prepare the call */
	rIwReqInfo.cmd = (__u16) i4Cmd;
	rIwReqInfo.flags = 0;

	switch (i4Cmd) {
	case IOCTL_SET_INT:
		/* NOTE(Kevin): 1/3 INT Type <= IFNAMSIZ, so we don't need
		 *              copy_from/to_user()
		 */
		return priv_set_int(prNetDev, &rIwReqInfo, &(prIwReq->u),
				    (char *) &(prIwReq->u));

	case IOCTL_GET_INT:
		/* NOTE(Kevin): 1/3 INT Type <= IFNAMSIZ, so we don't need
		 *              copy_from/to_user()
		 */
		return priv_get_int(prNetDev, &rIwReqInfo, &(prIwReq->u),
				    (char *) &(prIwReq->u));

	case IOCTL_SET_STRUCT:
	case IOCTL_SET_STRUCT_FOR_EM:
		return priv_set_struct(prNetDev, &rIwReqInfo, &prIwReq->u,
				       (char *) &(prIwReq->u));

	case IOCTL_GET_STRUCT:
		return priv_get_struct(prNetDev, &rIwReqInfo, &prIwReq->u,
				       (char *) &(prIwReq->u));

#if (CFG_SUPPORT_QA_TOOL)
	case IOCTL_QA_TOOL_DAEMON:
	case SIOCDEVPRIVATE+2:
		return priv_qa_agent(prNetDev, &rIwReqInfo, &(prIwReq->u),
				     (char *) &(prIwReq->u));
#endif

#if CFG_SUPPORT_NAN_PRIV
	case IOCTL_NAN_STRUCT:
		return priv_nan_struct(prNetDev, &rIwReqInfo, &(prIwReq->u),
				       (char *)&(prIwReq->u));
#endif

#if CFG_ENABLE_WIFI_DIRECT
	/* This case need to fall through */
	case IOC_AP_GET_STA_LIST:
	/* This case need to fall through */
	case IOC_AP_SET_MAC_FLTR:
	/* This case need to fall through */
	case IOC_AP_SET_CFG:
	/* This case need to fall through */
	case IOC_AP_STA_DISASSOC:
	/* This case need to fall through */
	case IOC_AP_SET_NSS:
	/* This case need to fall through */
	case IOC_AP_SET_BW:
		return priv_set_ap(prNetDev, &rIwReqInfo, &(prIwReq->u),
				     (char *) &(prIwReq->u));
#endif
	case IOCTL_GET_STR:

	default:
		return -EOPNOTSUPP;

	}			/* end of switch */

}				/* priv_support_ioctl */

/*----------------------------------------------------------------------------*/
/*!
 * \brief Private ioctl set int handler.
 *
 * \param[in] prNetDev Net device requested.
 * \param[in] prIwReqInfo Pointer to iwreq structure.
 * \param[in] prIwReqData The ioctl data structure, use the field of
 *            sub-command.
 * \param[in] pcExtra The buffer with input value
 *
 * \retval 0 For success.
 * \retval -EOPNOTSUPP If cmd is not supported.
 * \retval -EINVAL If a value is out of range.
 *
 */
/*----------------------------------------------------------------------------*/
int
__priv_set_int(struct net_device *prNetDev,
	     struct iw_request_info *prIwReqInfo,
	     union iwreq_data *prIwReqData, char *pcExtra)
{
	uint32_t u4SubCmd;
	uint32_t *pu4IntBuf;
	struct NDIS_TRANSPORT_STRUCT *prNdisReq;
	struct GLUE_INFO *prGlueInfo;
	uint32_t u4BufLen = 0;
	int status = 0;
	struct PTA_IPC *prPtaIpc;

	ASSERT(prNetDev);
	ASSERT(prIwReqInfo);
	ASSERT(prIwReqData);
	ASSERT(pcExtra);

	if (GLUE_CHK_PR3(prNetDev, prIwReqData, pcExtra) == FALSE)
		return -EINVAL;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	u4SubCmd = (uint32_t) prIwReqData->mode;
	pu4IntBuf = (uint32_t *) pcExtra;

	switch (u4SubCmd) {
	case PRIV_CMD_TEST_MODE:
		prNdisReq = (struct NDIS_TRANSPORT_STRUCT *) &aucOidBuf[0];

		if (pu4IntBuf[1] == PRIV_CMD_TEST_MAGIC_KEY) {
			prNdisReq->ndisOidCmd = OID_CUSTOM_TEST_MODE;
		} else if (pu4IntBuf[1] == 0) {
			prNdisReq->ndisOidCmd = OID_CUSTOM_ABORT_TEST_MODE;
		} else if (pu4IntBuf[1] == PRIV_CMD_TEST_MAGIC_KEY_ICAP) {
			prNdisReq->ndisOidCmd = OID_CUSTOM_TEST_ICAP_MODE;
		} else {
			status = 0;
			break;
		}
		prNdisReq->inNdisOidlength = 0;
		prNdisReq->outNdisOidLength = 0;

		/* Execute this OID */
		status = priv_set_ndis(prNetDev, prNdisReq, &u4BufLen);
		break;

	case PRIV_CMD_TEST_CMD:
		prNdisReq = (struct NDIS_TRANSPORT_STRUCT *) &aucOidBuf[0];

		kalMemCopy(&prNdisReq->ndisOidContent[0], &pu4IntBuf[1], 8);

		prNdisReq->ndisOidCmd = OID_CUSTOM_MTK_WIFI_TEST;
		prNdisReq->inNdisOidlength = 8;
		prNdisReq->outNdisOidLength = 8;

		/* Execute this OID */
		status = priv_set_ndis(prNetDev, prNdisReq, &u4BufLen);
		break;

#if BUILD_QA_DBG
#if CFG_SUPPORT_PRIV_MCR_RW
	case PRIV_CMD_ACCESS_MCR:
		prNdisReq = (struct NDIS_TRANSPORT_STRUCT *) &aucOidBuf[0];

		if (!prGlueInfo->fgMcrAccessAllowed) {
			if (pu4IntBuf[1] == PRIV_CMD_TEST_MAGIC_KEY
			    && pu4IntBuf[2] == PRIV_CMD_TEST_MAGIC_KEY)
				prGlueInfo->fgMcrAccessAllowed = TRUE;
			status = 0;
			break;
		}

		if (pu4IntBuf[1] == PRIV_CMD_TEST_MAGIC_KEY
		    && pu4IntBuf[2] == PRIV_CMD_TEST_MAGIC_KEY) {
			status = 0;
			break;
		}

		kalMemCopy(&prNdisReq->ndisOidContent[0], &pu4IntBuf[1], 8);

		prNdisReq->ndisOidCmd = OID_CUSTOM_MCR_RW;
		prNdisReq->inNdisOidlength = 8;
		prNdisReq->outNdisOidLength = 8;

		/* Execute this OID */
		status = priv_set_ndis(prNetDev, prNdisReq, &u4BufLen);
		break;
#endif
#endif

	case PRIV_CMD_SW_CTRL:
		prNdisReq = (struct NDIS_TRANSPORT_STRUCT *) &aucOidBuf[0];

		kalMemCopy(&prNdisReq->ndisOidContent[0], &pu4IntBuf[1], 8);

		prNdisReq->ndisOidCmd = OID_CUSTOM_SW_CTRL;
		prNdisReq->inNdisOidlength = 8;
		prNdisReq->outNdisOidLength = 8;

		/* Execute this OID */
		status = priv_set_ndis(prNetDev, prNdisReq, &u4BufLen);
		break;

#if CFG_TCP_IP_CHKSUM_OFFLOAD
	case PRIV_CMD_CSUM_OFFLOAD: {
		uint32_t u4CSUMFlags;

		if (pu4IntBuf[1] == 1)
			u4CSUMFlags = CSUM_OFFLOAD_EN_ALL;
		else if (pu4IntBuf[1] == 0)
			u4CSUMFlags = 0;
		else
			return -EINVAL;

		if (kalIoctl(prGlueInfo, wlanoidSetCSUMOffload,
		    (void *)&u4CSUMFlags, sizeof(uint32_t),
		    &u4BufLen) == WLAN_STATUS_SUCCESS) {
			if (pu4IntBuf[1] == 1)
				prNetDev->features |= NETIF_F_IP_CSUM |
						      NETIF_F_IPV6_CSUM |
						      NETIF_F_RXCSUM;
			else if (pu4IntBuf[1] == 0)
				prNetDev->features &= ~(NETIF_F_IP_CSUM |
							NETIF_F_IPV6_CSUM |
							NETIF_F_RXCSUM);
		}
	}
	break;
#endif /* CFG_TCP_IP_CHKSUM_OFFLOAD */

	case PRIV_CMD_POWER_MODE: {
		struct PARAM_POWER_MODE_ rPowerMode;

		rPowerMode.ePowerMode = (enum PARAM_POWER_MODE)
					pu4IntBuf[1];
		rPowerMode.ucBssIdx = wlanGetBssIdx(prNetDev);

		/* pu4IntBuf[0] is used as input SubCmd */
		kalIoctl(prGlueInfo, wlanoidSet802dot11PowerSaveProfile,
			 &rPowerMode, sizeof(struct PARAM_POWER_MODE_),
			 &u4BufLen);
	}
	break;

	case PRIV_CMD_WMM_PS: {
		struct PARAM_CUSTOM_WMM_PS_TEST_STRUCT rWmmPsTest;

		rWmmPsTest.bmfgApsdEnAc = (uint8_t) pu4IntBuf[1];
		rWmmPsTest.ucIsEnterPsAtOnce = (uint8_t) pu4IntBuf[2];
		rWmmPsTest.ucIsDisableUcTrigger = (uint8_t) pu4IntBuf[3];
		rWmmPsTest.reserved = 0;
		rWmmPsTest.ucBssIdx = wlanGetBssIdx(prNetDev);
		kalIoctl(prGlueInfo, wlanoidSetWiFiWmmPsTest,
			 (void *)&rWmmPsTest,
			 sizeof(struct PARAM_CUSTOM_WMM_PS_TEST_STRUCT),
			 &u4BufLen);
	}
	break;

	case PRIV_CUSTOM_BWCS_CMD:

		DBGLOG(REQ, INFO,
		       "pu4IntBuf[1] = %x, size of struct PTA_IPC = %d.\n",
		       pu4IntBuf[1], (uint32_t) sizeof(struct PTA_IPC));

		prPtaIpc = (struct PTA_IPC *) aucOidBuf;
		prPtaIpc->u.aucBTPParams[0] = (uint8_t) (pu4IntBuf[1] >>
					      24);
		prPtaIpc->u.aucBTPParams[1] = (uint8_t) (pu4IntBuf[1] >>
					      16);
		prPtaIpc->u.aucBTPParams[2] = (uint8_t) (pu4IntBuf[1] >> 8);
		prPtaIpc->u.aucBTPParams[3] = (uint8_t) (pu4IntBuf[1]);

		DBGLOG(REQ, INFO,
		       "BCM BWCS CMD : PRIV_CUSTOM_BWCS_CMD : aucBTPParams[0] = %02x, aucBTPParams[1] = %02x.\n",
		       prPtaIpc->u.aucBTPParams[0],
		       prPtaIpc->u.aucBTPParams[1]);
		DBGLOG(REQ, INFO,
		       "BCM BWCS CMD : PRIV_CUSTOM_BWCS_CMD : aucBTPParams[2] = %02x, aucBTPParams[3] = %02x.\n",
		       prPtaIpc->u.aucBTPParams[2],
		       prPtaIpc->u.aucBTPParams[3]);

		status = wlanoidSetBT(prGlueInfo->prAdapter,
				(void *)&aucOidBuf[0], sizeof(struct PTA_IPC),
				&u4BufLen);

		if (status != WLAN_STATUS_SUCCESS)
			status = -EFAULT;

		break;

	case PRIV_CMD_BAND_CONFIG: {
		DBGLOG(INIT, INFO, "CMD set_band = %u\n",
		       (uint32_t) pu4IntBuf[1]);
	}
	break;

#if CFG_ENABLE_WIFI_DIRECT
	case PRIV_CMD_P2P_MODE: {
		struct PARAM_CUSTOM_P2P_SET_WITH_LOCK_STRUCT rSetP2P;
		uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;

		rSetP2P.u4Enable = pu4IntBuf[1];
		rSetP2P.u4Mode = pu4IntBuf[2];
		rSetP2P.fgIsRtnlLockAcquired = TRUE;
#if 1
		if (!rSetP2P.u4Enable)
			p2pNetUnregister(prGlueInfo, TRUE);

		/* pu4IntBuf[0] is used as input SubCmd */
		rWlanStatus = kalIoctl(prGlueInfo, wlanoidSetP2pMode,
			(void *)&rSetP2P,
			sizeof(struct PARAM_CUSTOM_P2P_SET_WITH_LOCK_STRUCT),
			&u4BufLen);

		if ((rSetP2P.u4Enable)
		    && (rWlanStatus == WLAN_STATUS_SUCCESS))
			p2pNetRegister(prGlueInfo, TRUE);
#endif

	}
	break;
#endif

#if (CFG_MET_PACKET_TRACE_SUPPORT == 1)
	case PRIV_CMD_MET_PROFILING: {
		/* PARAM_CUSTOM_WFD_DEBUG_STRUCT_T rWfdDebugModeInfo; */
		/* rWfdDebugModeInfo.ucWFDDebugMode=(UINT_8)pu4IntBuf[1]; */
		/* rWfdDebugModeInfo.u2SNPeriod=(UINT_16)pu4IntBuf[2]; */
		/* DBGLOG(REQ, INFO, ("WFD Debug Mode:%d Period:%d\n",
		 *  rWfdDebugModeInfo.ucWFDDebugMode,
		 *  rWfdDebugModeInfo.u2SNPeriod));
		 */
		prGlueInfo->fgMetProfilingEn = (uint8_t) pu4IntBuf[1];
		prGlueInfo->u2MetUdpPort = (uint16_t) pu4IntBuf[2];
		/* DBGLOG(INIT, INFO, ("MET_PROF: Enable=%d UDP_PORT=%d\n",
		 *  prGlueInfo->fgMetProfilingEn, prGlueInfo->u2MetUdpPort);
		 */

	}
	break;

#endif
	case PRIV_CMD_SET_SER:
		kalIoctl(prGlueInfo, wlanoidSetSer, (void *)&pu4IntBuf[1],
			 sizeof(uint32_t), &u4BufLen);
		break;

	default:
		return -EOPNOTSUPP;
	}

	return status;
}				/* __priv_set_int */

int
priv_set_int(struct net_device *prNetDev,
	     struct iw_request_info *prIwReqInfo,
	     union iwreq_data *prIwReqData, char *pcExtra)
{
#if BUILD_QA_DBG
	DBGLOG(REQ, LOUD, "cmd=%x, flags=%x\n",
	     prIwReqInfo->cmd, prIwReqInfo->flags);
	DBGLOG(REQ, LOUD, "mode=%x, flags=%x\n",
	     prIwReqData->mode, prIwReqData->data.flags);

	return compat_priv(prNetDev, prIwReqInfo,
	     prIwReqData, pcExtra, __priv_set_int);
#else
	DBGLOG(REQ, WARN, "not support in user load");
	return 0;
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Private ioctl get int handler.
 *
 * \param[in] pDev Net device requested.
 * \param[out] pIwReq Pointer to iwreq structure.
 * \param[in] prIwReqData The ioctl req structure, use the field of sub-command.
 * \param[out] pcExtra The buffer with put the return value
 *
 * \retval 0 For success.
 * \retval -EOPNOTSUPP If cmd is not supported.
 * \retval -EFAULT For fail.
 *
 */
/*----------------------------------------------------------------------------*/
int
__priv_get_int(struct net_device *prNetDev,
	     struct iw_request_info *prIwReqInfo,
	     union iwreq_data *prIwReqData, char *pcExtra)
{
	uint32_t u4SubCmd;
	uint32_t *pu4IntBuf;
	struct GLUE_INFO *prGlueInfo;
	uint32_t u4BufLen = 0;
	int status = 0;
	struct NDIS_TRANSPORT_STRUCT *prNdisReq;
	int32_t ch[50] = {0};

	ASSERT(prNetDev);
	ASSERT(prIwReqInfo);
	ASSERT(prIwReqData);
	ASSERT(pcExtra);
	if (GLUE_CHK_PR3(prNetDev, prIwReqData, pcExtra) == FALSE)
		return -EINVAL;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	u4SubCmd = (uint32_t) prIwReqData->mode;
	pu4IntBuf = (uint32_t *) pcExtra;

	switch (u4SubCmd) {
	case PRIV_CMD_TEST_CMD:
		prNdisReq = (struct NDIS_TRANSPORT_STRUCT *) &aucOidBuf[0];

		kalMemCopy(&prNdisReq->ndisOidContent[0], &pu4IntBuf[1], 8);

		prNdisReq->ndisOidCmd = OID_CUSTOM_MTK_WIFI_TEST;
		prNdisReq->inNdisOidlength = 8;
		prNdisReq->outNdisOidLength = 8;

		status = priv_get_ndis(prNetDev, prNdisReq, &u4BufLen);
		if (status == 0) {
			prIwReqData->mode = *(uint32_t *)
					    &prNdisReq->ndisOidContent[4];

		}
		return status;

#if BUILD_QA_DBG
#if CFG_SUPPORT_PRIV_MCR_RW
	case PRIV_CMD_ACCESS_MCR:
		prNdisReq = (struct NDIS_TRANSPORT_STRUCT *) &aucOidBuf[0];

		if (!prGlueInfo->fgMcrAccessAllowed) {
			status = 0;
			return status;
		}

		kalMemCopy(&prNdisReq->ndisOidContent[0], &pu4IntBuf[1], 8);

		prNdisReq->ndisOidCmd = OID_CUSTOM_MCR_RW;
		prNdisReq->inNdisOidlength = 8;
		prNdisReq->outNdisOidLength = 8;

		status = priv_get_ndis(prNetDev, prNdisReq, &u4BufLen);
		if (status == 0) {
			prIwReqData->mode = *(uint32_t *)
					    &prNdisReq->ndisOidContent[4];
		}
		return status;
#endif
#endif

	case PRIV_CMD_DUMP_MEM:
		DBGLOG(INIT, INFO, "No support dump memory\n");
		prIwReqData->mode = 0;

		return status;

	case PRIV_CMD_SW_CTRL:
		prNdisReq = (struct NDIS_TRANSPORT_STRUCT *) &aucOidBuf[0];

		kalMemCopy(&prNdisReq->ndisOidContent[0], &pu4IntBuf[1], 8);

		prNdisReq->ndisOidCmd = OID_CUSTOM_SW_CTRL;
		prNdisReq->inNdisOidlength = 8;
		prNdisReq->outNdisOidLength = 8;

		status = priv_get_ndis(prNetDev, prNdisReq, &u4BufLen);
		if (status == 0) {
			prIwReqData->mode = *(uint32_t *)
					    &prNdisReq->ndisOidContent[4];
		}
		return status;

/* fos_change begin */
	case PRIV_CMD_SHOW_CHANNEL:
	{
		uint32_t freq = 0;

		status = kalIoctl(prGlueInfo,
			wlanoidQueryFrequency,
			&freq, sizeof(uint32_t), &u4BufLen);
		if (status == WLAN_STATUS_SUCCESS)
			prIwReqData->mode = freq/1000; /* Hz->MHz */

		return status;
	}

	case PRIV_CMD_BAND_CONFIG:
		DBGLOG(INIT, INFO, "CMD get_band=\n");
		prIwReqData->mode = 0;
		return status;

	default:
		break;
	}

	u4SubCmd = (uint32_t) prIwReqData->data.flags;

	switch (u4SubCmd) {
	case PRIV_CMD_GET_CH_LIST: {
		uint16_t i, j = 0;
		uint8_t NumOfChannel = 50;
		uint8_t ucMaxChannelNum = 50;
		struct RF_CHANNEL_INFO *aucChannelList;

		DBGLOG(RLM, INFO, "Domain: Query Channel List.\n");
		aucChannelList = (struct RF_CHANNEL_INFO *)
			kalMemAlloc(sizeof(struct RF_CHANNEL_INFO)
				*ucMaxChannelNum, VIR_MEM_TYPE);
		if (!aucChannelList) {
			DBGLOG(REQ, ERROR,
				"Can not alloc memory for rf channel info\n");
			return -ENOMEM;
		}
		kalMemZero(aucChannelList,
			sizeof(struct RF_CHANNEL_INFO)*ucMaxChannelNum);

		kalGetChannelList(prGlueInfo, BAND_NULL, ucMaxChannelNum,
				  &NumOfChannel, aucChannelList);
		if (NumOfChannel > ucMaxChannelNum)
			NumOfChannel = ucMaxChannelNum;

		if (kalIsAPmode(prGlueInfo)) {
			for (i = 0; i < NumOfChannel; i++) {
				if ((aucChannelList[i].ucChannelNum <= 13) ||
				    (aucChannelList[i].ucChannelNum == 36
				     || aucChannelList[i].ucChannelNum == 40
				     || aucChannelList[i].ucChannelNum == 44
				     || aucChannelList[i].ucChannelNum == 48)) {
					ch[j] = (int32_t) aucChannelList[i]
								.ucChannelNum;
					j++;
				}
			}
		} else {
			for (j = 0; j < NumOfChannel; j++)
				ch[j] = (int32_t)aucChannelList[j].ucChannelNum;
		}
		kalMemFree(aucChannelList, VIR_MEM_TYPE,
			sizeof(struct RF_CHANNEL_INFO)*ucMaxChannelNum);

		prIwReqData->data.length = j;
		if (copy_to_user(prIwReqData->data.pointer, ch,
				 NumOfChannel * sizeof(int32_t)))
			return -EFAULT;
		else
			return status;
	}
	case PRIV_CMD_GET_FW_VERSION: {
			uint16_t u2Len = 0;
			struct ADAPTER *prAdapter;

			prAdapter = prGlueInfo->prAdapter;
			if (prAdapter) {
				u2Len = kalStrLen(
					prAdapter->rVerInfo.aucReleaseManifest);
				DBGLOG(REQ, INFO,
					"Get FW manifest version: %d\n", u2Len);
				prIwReqData->data.length = u2Len;
				if (copy_to_user(prIwReqData->data.pointer,
					prAdapter->rVerInfo.aucReleaseManifest,
					u2Len * sizeof(uint8_t)))
					return -EFAULT;
				else
					return status;
			}
			DBGLOG(REQ, WARN, "Fail to get FW manifest version\n");
			return status;
		}
	default:
		return -EOPNOTSUPP;
	}

	return status;
}				/* __priv_get_int */

int
priv_get_int(struct net_device *prNetDev,
	     struct iw_request_info *prIwReqInfo,
	     union iwreq_data *prIwReqData, char *pcExtra)
{
#if BUILD_QA_DBG
	DBGLOG(REQ, LOUD, "cmd=%x, flags=%x\n",
	     prIwReqInfo->cmd, prIwReqInfo->flags);
	DBGLOG(REQ, LOUD, "mode=%x, flags=%x\n",
	     prIwReqData->mode, prIwReqData->data.flags);

	return compat_priv(prNetDev, prIwReqInfo,
	     prIwReqData, pcExtra, __priv_get_int);
#else
	DBGLOG(REQ, WARN, "not support in user load");
	return 0;
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Private ioctl get int array handler.
 *
 * \param[in] pDev Net device requested.
 * \param[out] pIwReq Pointer to iwreq structure.
 * \param[in] prIwReqData The ioctl req structure, use the field of sub-command.
 * \param[out] pcExtra The buffer with put the return value
 *
 * \retval 0 For success.
 * \retval -EOPNOTSUPP If cmd is not supported.
 * \retval -EFAULT For fail.
 *
 */
/*----------------------------------------------------------------------------*/
int
__priv_get_ints(struct net_device *prNetDev,
	      struct iw_request_info *prIwReqInfo,
	      union iwreq_data *prIwReqData, char *pcExtra)
{
	uint32_t u4SubCmd;
	struct GLUE_INFO *prGlueInfo;
	int status = 0;
	int32_t ch[50];

	ASSERT(prNetDev);
	ASSERT(prIwReqInfo);
	ASSERT(prIwReqData);
	ASSERT(pcExtra);
	if (GLUE_CHK_PR3(prNetDev, prIwReqData, pcExtra) == FALSE)
		return -EINVAL;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	u4SubCmd = (uint32_t) prIwReqData->data.flags;

	switch (u4SubCmd) {
	case PRIV_CMD_GET_CH_LIST: {
		uint16_t i;
		uint8_t NumOfChannel = 50;
		uint8_t ucMaxChannelNum = 50;
		struct RF_CHANNEL_INFO *aucChannelList;

		aucChannelList = (struct RF_CHANNEL_INFO *)
			kalMemAlloc(sizeof(struct RF_CHANNEL_INFO)
				*ucMaxChannelNum, VIR_MEM_TYPE);
		if (!aucChannelList) {
			DBGLOG(REQ, ERROR,
				"Can not alloc memory for rf channel info\n");
			return -ENOMEM;
		}
		kalMemZero(aucChannelList,
			sizeof(struct RF_CHANNEL_INFO)*ucMaxChannelNum);

		kalGetChannelList(prGlueInfo, BAND_NULL, ucMaxChannelNum,
				  &NumOfChannel, aucChannelList);
		if (NumOfChannel > ucMaxChannelNum)
			NumOfChannel = ucMaxChannelNum;

		for (i = 0; i < NumOfChannel; i++)
			ch[i] = (int32_t) aucChannelList[i].ucChannelNum;

		kalMemFree(aucChannelList, VIR_MEM_TYPE,
			sizeof(struct RF_CHANNEL_INFO)*ucMaxChannelNum);

		prIwReqData->data.length = NumOfChannel;
		if (copy_to_user(prIwReqData->data.pointer, ch,
				 NumOfChannel * sizeof(int32_t)))
			return -EFAULT;
		else
			return status;
	}
	default:
		break;
	}

	return status;
}				/* __priv_get_ints */

int
priv_get_ints(struct net_device *prNetDev,
	     struct iw_request_info *prIwReqInfo,
	     union iwreq_data *prIwReqData, char *pcExtra)
{
#if BUILD_QA_DBG
	DBGLOG(REQ, LOUD, "cmd=%x, flags=%x\n",
	     prIwReqInfo->cmd, prIwReqInfo->flags);
	DBGLOG(REQ, LOUD, "mode=%x, flags=%x\n",
	     prIwReqData->mode, prIwReqData->data.flags);

	return compat_priv(prNetDev, prIwReqInfo,
	    prIwReqData, pcExtra, __priv_get_ints);
#else
	DBGLOG(REQ, WARN, "not support in user load");
	return 0;
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Private ioctl set structure handler.
 *
 * \param[in] pDev Net device requested.
 * \param[in] prIwReqData Pointer to iwreq_data structure.
 *
 * \retval 0 For success.
 * \retval -EOPNOTSUPP If cmd is not supported.
 * \retval -EINVAL If a value is out of range.
 *
 */
/*----------------------------------------------------------------------------*/
int
__priv_set_struct(struct net_device *prNetDev,
		struct iw_request_info *prIwReqInfo,
		union iwreq_data *prIwReqData, char *pcExtra)
{
	uint32_t u4SubCmd = 0;
	int status = 0;
	/* WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS; */
	uint32_t u4CmdLen = 0;
	struct NDIS_TRANSPORT_STRUCT *prNdisReq;

	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t u4BufLen = 0;
#if CFG_SUPPORT_WPS2
	uint8_t ucBssIndex = wlanGetBssIdx(prNetDev);
#endif

	/* ASSERT(prIwReqInfo); */
	/* ASSERT(pcExtra); */
	if (prNetDev == NULL) {
		DBGLOG(REQ, ERROR, "prNetDev is NULL\n");
		return -EINVAL;
	}
	if (prIwReqData == NULL) {
		DBGLOG(REQ, ERROR, "prIwReqData is NULL\n");
		return -EINVAL;
	}

	kalMemZero(&aucOidBuf[0], sizeof(aucOidBuf));

	if (GLUE_CHK_PR2(prNetDev, prIwReqData) == FALSE)
		return -EINVAL;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	u4SubCmd = (uint32_t) prIwReqData->data.flags;

	switch (u4SubCmd) {

	case PRIV_CUSTOM_BWCS_CMD:
		u4CmdLen = prIwReqData->data.length * sizeof(uint32_t);
		if (sizeof(struct PTA_IPC) < u4CmdLen) {
			DBGLOG(REQ, ERROR,
			       "u4CmdLen: %d > sizeof(struct PTA_IPC): %d\n",
			       u4CmdLen, sizeof(struct PTA_IPC));
			return -EFAULT;
		}
#if CFG_SUPPORT_BCM && CFG_SUPPORT_BCM_BWCS && CFG_SUPPORT_BCM_BWCS_DEBUG
		DBGLOG(REQ, INFO,
		       "ucCmdLen = %d, size of struct PTA_IPC = %d, prIwReqData->data = 0x%x.\n",
		       u4CmdLen, sizeof(struct PTA_IPC), prIwReqData->data);

		DBGLOG(REQ, INFO,
		       "priv_set_struct(): prIwReqInfo->cmd(0x%X), u4SubCmd(%ld)\n",
		       prIwReqInfo->cmd,
		       u4SubCmd);
		DBGLOG(REQ, INFO, "*pcExtra = 0x%x\n", *pcExtra);
#endif

		if (copy_from_user(&aucOidBuf[0], prIwReqData->data.pointer,
				   u4CmdLen)) {
			status = -EFAULT;	/* return -EFAULT; */
			break;
		}
#if CFG_SUPPORT_BCM && CFG_SUPPORT_BCM_BWCS && CFG_SUPPORT_BCM_BWCS_DEBUG
		DBGLOG(REQ, INFO,
		       "priv_set_struct(): BWCS CMD = %02x%02x%02x%02x\n",
		       aucOidBuf[2], aucOidBuf[3],
		       aucOidBuf[4], aucOidBuf[5]);
#endif
		status = wlanoidSetBT(prGlueInfo->prAdapter,
				(void *)&aucOidBuf[0], u4CmdLen, &u4BufLen);

		if (status != WLAN_STATUS_SUCCESS)
			status = -EFAULT;

		break;

#if CFG_SUPPORT_WPS2
	case PRIV_CMD_WSC_PROBE_REQ: {
		struct CONNECTION_SETTINGS *prConnSettings =
			aisGetConnSettings(prGlueInfo->prAdapter,
			ucBssIndex);

		/* retrieve IE for Probe Request */
		u4CmdLen = prIwReqData->data.length;
		if (u4CmdLen > GLUE_INFO_WSCIE_LENGTH) {
			DBGLOG(REQ, ERROR, "Input data length is invalid %u\n",
			       u4CmdLen);
			return -EINVAL;
		}

		if (prIwReqData->data.length > 0) {
			if (copy_from_user(prConnSettings->aucWSCIE,
					   prIwReqData->data.pointer,
					   u4CmdLen)) {
				status = -EFAULT;
				break;
			}
			prConnSettings->u2WSCIELen = u4CmdLen;
		} else {
			prConnSettings->u2WSCIELen = 0;
		}
	}
	break;
#endif
	case PRIV_CMD_OID:
		u4CmdLen = prIwReqData->data.length;
		if (u4CmdLen > CMD_OID_BUF_LENGTH) {
			DBGLOG(REQ, ERROR, "Input data length is invalid %u\n",
			       u4CmdLen);
			return -EINVAL;
		}
		if (copy_from_user(&aucOidBuf[0], prIwReqData->data.pointer,
				   u4CmdLen)) {
			status = -EFAULT;
			break;
		}
		if (!kalMemCmp(&aucOidBuf[0], pcExtra, u4CmdLen)) {
			/* ToDo:: DBGLOG */
			DBGLOG(REQ, INFO, "pcExtra buffer is valid\n");
		} else {
			DBGLOG(REQ, INFO, "pcExtra 0x%p\n", pcExtra);
		}
		/* Execute this OID */
		status = priv_set_ndis(prNetDev,
				(struct NDIS_TRANSPORT_STRUCT *) &aucOidBuf[0],
				&u4BufLen);
		/* Copy result to user space */
		((struct NDIS_TRANSPORT_STRUCT *)
		 &aucOidBuf[0])->outNdisOidLength = u4BufLen;

		if (copy_to_user(prIwReqData->data.pointer, &aucOidBuf[0],
		    OFFSET_OF(struct NDIS_TRANSPORT_STRUCT, ndisOidContent))) {
			DBGLOG(REQ, INFO, "copy_to_user oidBuf fail\n");
			status = -EFAULT;
		}

		break;

	case PRIV_CMD_SW_CTRL:
		u4CmdLen = prIwReqData->data.length;
		prNdisReq = (struct NDIS_TRANSPORT_STRUCT *) &aucOidBuf[0];

		if (u4CmdLen > sizeof(prNdisReq->ndisOidContent)) {
			DBGLOG(REQ, ERROR, "Input data length is invalid %u\n",
			       u4CmdLen);
			return -EINVAL;
		}

		if (copy_from_user(&prNdisReq->ndisOidContent[0],
				   prIwReqData->data.pointer, u4CmdLen)) {
			status = -EFAULT;
			break;
		}
		prNdisReq->ndisOidCmd = OID_CUSTOM_SW_CTRL;
		prNdisReq->inNdisOidlength = 8;
		prNdisReq->outNdisOidLength = 8;

		/* Execute this OID */
		status = priv_set_ndis(prNetDev, prNdisReq, &u4BufLen);
		break;

	case PRIV_CMD_GET_WIFI_TYPE:
		{
			int32_t i4ResultLen;

			u4CmdLen = prIwReqData->data.length;
			if (u4CmdLen >= CMD_OID_BUF_LENGTH) {
				DBGLOG(REQ, ERROR,
				       "u4CmdLen:%u >= CMD_OID_BUF_LENGTH:%d\n",
				       u4CmdLen, CMD_OID_BUF_LENGTH);
				return -EINVAL;
			}
			if (copy_from_user(&aucOidBuf[0],
					   prIwReqData->data.pointer,
					   u4CmdLen)) {
				DBGLOG(REQ, ERROR, "copy_from_user fail\n");
				return -EFAULT;
			}
			aucOidBuf[u4CmdLen] = 0;
			i4ResultLen = priv_driver_cmds(prNetDev, aucOidBuf,
						       u4CmdLen);
			if (i4ResultLen > 1) {
				if (copy_to_user(prIwReqData->data.pointer,
						 &aucOidBuf[0], i4ResultLen)) {
					DBGLOG(REQ, ERROR,
					       "copy_to_user fail\n");
					return -EFAULT;
				}
				prIwReqData->data.length = i4ResultLen;
			} else {
				DBGLOG(REQ, ERROR,
				       "i4ResultLen:%d <= 1\n", i4ResultLen);
				return -EFAULT;
			}
		}
		break; /* case PRIV_CMD_GET_WIFI_TYPE: */

	/* dynamic tx power control */
	case PRIV_CMD_SET_PWR_CTRL:
		{
			char *pCommand = NULL;

			u4CmdLen = prIwReqData->data.length;
			if (u4CmdLen >= CMD_OID_BUF_LENGTH)
				return -EINVAL;
			if (copy_from_user(&aucOidBuf[0],
					   prIwReqData->data.pointer,
					   u4CmdLen)) {
				status = -EFAULT;
				break;
			}
			aucOidBuf[u4CmdLen] = 0;
			if (strlen(aucOidBuf) <= 0) {
				status = -EFAULT;
				break;
			}
			pCommand = kalMemAlloc(u4CmdLen + 1, VIR_MEM_TYPE);
			if (pCommand == NULL) {
				DBGLOG(REQ, INFO, "alloc fail\n");
				return -EINVAL;
			}
			kalMemZero(pCommand, u4CmdLen + 1);
			kalMemCopy(pCommand, aucOidBuf, u4CmdLen);
			pCommand[u4CmdLen] = '\0';
			priv_driver_cmds(prNetDev, pCommand, u4CmdLen);
			kalMemFree(pCommand, VIR_MEM_TYPE, i4TotalLen);
		}
		break;

	default:
		return -EOPNOTSUPP;
	}

	return status;
}				/* __priv_set_struct */

int
priv_set_struct(struct net_device *prNetDev,
	     struct iw_request_info *prIwReqInfo,
	     union iwreq_data *prIwReqData, char *pcExtra)
{
	DBGLOG(REQ, LOUD, "cmd=%x, flags=%x\n",
	     prIwReqInfo->cmd, prIwReqInfo->flags);
	DBGLOG(REQ, LOUD, "mode=%x, flags=%x\n",
	     prIwReqData->mode, prIwReqData->data.flags);

	return compat_priv(prNetDev, prIwReqInfo,
	     prIwReqData, pcExtra, __priv_set_struct);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Private ioctl get struct handler.
 *
 * \param[in] pDev Net device requested.
 * \param[out] pIwReq Pointer to iwreq structure.
 * \param[in] cmd Private sub-command.
 *
 * \retval 0 For success.
 * \retval -EFAULT If copy from user space buffer fail.
 * \retval -EOPNOTSUPP Parameter "cmd" not recognized.
 *
 */
/*----------------------------------------------------------------------------*/
int
__priv_get_struct(struct net_device *prNetDev,
		struct iw_request_info *prIwReqInfo,
		union iwreq_data *prIwReqData, char *pcExtra)
{
	uint32_t u4SubCmd = 0;
	struct NDIS_TRANSPORT_STRUCT *prNdisReq = NULL;

	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t u4BufLen = 0;
	uint32_t u4DataLen = 0;
	/* uint32_t *pu4IntBuf = NULL; */
	int status = 0;

	kalMemZero(&aucOidBuf[0], sizeof(aucOidBuf));

	ASSERT(prNetDev);
	ASSERT(prIwReqData);
	if (!prNetDev || !prIwReqData) {
		DBGLOG(REQ, INFO,
		       "priv_get_struct(): invalid param(0x%p, 0x%p)\n",
		       prNetDev, prIwReqData);
		return -EINVAL;
	}

	u4SubCmd = (uint32_t) prIwReqData->data.flags;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	ASSERT(prGlueInfo);
	if (!prGlueInfo) {
		DBGLOG(REQ, INFO,
		       "priv_get_struct(): invalid prGlueInfo(0x%p, 0x%p)\n",
		       prNetDev,
		       *((struct GLUE_INFO **) netdev_priv(prNetDev)));
		return -EINVAL;
	}
#if 0
	DBGLOG(INIT, INFO,
	       "priv_get_struct(): prIwReqInfo->cmd(0x%X), u4SubCmd(%ld)\n",
	       prIwReqInfo->cmd, u4SubCmd);
#endif
	memset(aucOidBuf, 0, sizeof(aucOidBuf));

	switch (u4SubCmd) {
	case PRIV_CMD_OID:
		if (prIwReqData->data.length >
			sizeof(struct NDIS_TRANSPORT_STRUCT))
			u4DataLen = sizeof(struct NDIS_TRANSPORT_STRUCT);
		else
			u4DataLen = prIwReqData->data.length;
		if (copy_from_user(&aucOidBuf[0], prIwReqData->data.pointer,
				   u4DataLen)) {
			DBGLOG(REQ, INFO,
			       "priv_get_struct() copy_from_user oidBuf fail\n");
			return -EFAULT;
		}

		prNdisReq = (struct NDIS_TRANSPORT_STRUCT *) &aucOidBuf[0];
#if 0
		DBGLOG(INIT, INFO,
		       "\n priv_get_struct cmd 0x%02x len:%d OID:0x%08x OID Len:%d\n",
		       cmd,
		       pIwReq->u.data.length, ndisReq->ndisOidCmd,
		       ndisReq->inNdisOidlength);
#endif
		if (priv_get_ndis(prNetDev, prNdisReq, &u4BufLen) == 0) {
			prNdisReq->outNdisOidLength = u4BufLen;
			if (copy_to_user(prIwReqData->data.pointer,
			    &aucOidBuf[0], u4BufLen +
			    sizeof(struct NDIS_TRANSPORT_STRUCT) -
			    sizeof(prNdisReq->ndisOidContent))) {
				DBGLOG(REQ, INFO,
				       "priv_get_struct() copy_to_user oidBuf fail(1)\n"
				       );
				return -EFAULT;
			}
			return 0;
		}
		prNdisReq->outNdisOidLength = u4BufLen;
		if (copy_to_user(prIwReqData->data.pointer,
		    &aucOidBuf[0], OFFSET_OF(struct NDIS_TRANSPORT_STRUCT,
						 ndisOidContent))) {
			DBGLOG(REQ, INFO,
			       "priv_get_struct() copy_to_user oidBuf fail(2)\n"
			       );
		}
		return -EFAULT;

	case PRIV_CMD_SW_CTRL:
		/* pu4IntBuf = (uint32_t *) prIwReqData->data.pointer; */
		prNdisReq = (struct NDIS_TRANSPORT_STRUCT *) &aucOidBuf[0];

		if (prIwReqData->data.length > (sizeof(aucOidBuf) -
		    OFFSET_OF(struct NDIS_TRANSPORT_STRUCT, ndisOidContent))) {
			DBGLOG(REQ, INFO,
			       "priv_get_struct() exceeds length limit\n");
			return -EFAULT;
		}

		/* if (copy_from_user(&prNdisReq->ndisOidContent[0],
		 *     prIwReqData->data.pointer,
		 */
		/* Coverity uanble to detect real size of ndisOidContent,
		 * it's 4084 bytes instead of 16 bytes
		 */
		if (copy_from_user(&aucOidBuf[OFFSET_OF(struct
		    NDIS_TRANSPORT_STRUCT, ndisOidContent)],
				   prIwReqData->data.pointer,
				   prIwReqData->data.length)) {
			DBGLOG(REQ, INFO,
			       "priv_get_struct() copy_from_user oidBuf fail\n");
			return -EFAULT;
		}

		prNdisReq->ndisOidCmd = OID_CUSTOM_SW_CTRL;
		prNdisReq->inNdisOidlength = 8;
		prNdisReq->outNdisOidLength = 8;

		status = priv_get_ndis(prNetDev, prNdisReq, &u4BufLen);
		if (status == 0) {
			prNdisReq->outNdisOidLength = u4BufLen;

			if (copy_to_user(prIwReqData->data.pointer,
					 &prNdisReq->ndisOidContent[4], 4))
				DBGLOG(REQ, INFO,
				       "priv_get_struct() copy_to_user oidBuf fail(2)\n"
				       );
		}
		return 0;
	default:
		DBGLOG(REQ, WARN, "get struct cmd:0x%x\n", u4SubCmd);
		return -EOPNOTSUPP;
	}
}				/* __priv_get_struct */

int
priv_get_struct(struct net_device *prNetDev,
	     struct iw_request_info *prIwReqInfo,
	     union iwreq_data *prIwReqData, char *pcExtra)
{
	DBGLOG(REQ, LOUD, "cmd=%x, flags=%x\n",
	     prIwReqInfo->cmd, prIwReqInfo->flags);
	DBGLOG(REQ, LOUD, "mode=%x, flags=%x\n",
	     prIwReqData->mode, prIwReqData->data.flags);

	return compat_priv(prNetDev, prIwReqInfo,
	     prIwReqData, pcExtra, __priv_get_struct);
}

#if CFG_SUPPORT_NAN_PRIV
int __priv_nan_struct(struct net_device *prNetDev,
		  struct iw_request_info *prIwReqInfo,
		  union iwreq_data *prIwReqData, char *pcExtra)
{
	uint32_t u4SubCmd = 0;
	int status = 0;
	struct GLUE_INFO *prGlueInfo = NULL;

	if (!prNetDev) {
		DBGLOG(NAN, ERROR, "prNetDev error!\n");
		return -EFAULT;
	}
	if (!prIwReqData) {
		DBGLOG(NAN, ERROR, "prIwReqData error!\n");
		return -EFAULT;
	}

	kalMemZero(&aucOidBuf[0], sizeof(aucOidBuf));

	if (GLUE_CHK_PR2(prNetDev, prIwReqData) == FALSE)
		return -EINVAL;
	prGlueInfo = *((struct GLUE_INFO **)netdev_priv(prNetDev));

	prGlueInfo->prAdapter->fgIsNANfromHAL = FALSE;
	DBGLOG(NAN, LOUD, "NAN fgIsNANfromHAL set %u\n",
	       prGlueInfo->prAdapter->fgIsNANfromHAL);

	u4SubCmd = (uint32_t)prIwReqData->data.flags;
	DBGLOG(INIT, INFO, "DATA len from user %d\n", prIwReqData->data.length);
	if (prIwReqData->data.length > CMD_OID_BUF_LENGTH)
		return -EFAULT;
	if (copy_from_user(&aucOidBuf[0], prIwReqData->data.pointer,
			   prIwReqData->data.length))
		status = -EFAULT; /* return -EFAULT; */

	switch (u4SubCmd) {
	case ENUM_NAN_PUBLISH: {
		uint16_t pid = 0;
#if 0
		UINT_8  *pu1DummyAttrBuf = NULL;
		UINT_32 u4DummyAttrLen = 0;
#endif
		uint8_t ucCipherType;

		struct NanPublishRequest *publishReq =
			(struct NanPublishRequest *)&aucOidBuf[0];

		if (prIwReqData->data.length <
			sizeof(struct NanPublishRequest)) {
			DBGLOG(INIT, INFO,
				"ENUM_NAN_PUBLISH not have enough length(%d)\n",
				prIwReqData->data.length);
			return -EFAULT;
		}

		DBGLOG(NAN, INFO, "[Publish Request]\n");
		DBGLOG(NAN, INFO, "Type: %d\n", publishReq->publish_type);
		DBGLOG(NAN, INFO, "TTL: %d\n", publishReq->ttl);
		DBGLOG(NAN, INFO, "Service Specific Info: %s\n",
		       publishReq->service_specific_info);
		DBGLOG(NAN, INFO, "Period: %d\n", publishReq->period);
		DBGLOG(NAN, INFO, "Publish Count: %d\n",
		       publishReq->publish_count);
		DBGLOG(NAN, INFO, "Match Indicator: %d\n",
		       publishReq->publish_match_indicator);
		DBGLOG(NAN, INFO, "Service Name: %s\n",
		       publishReq->service_name);
		DBGLOG(NAN, INFO, "Rssi threshold: %d\n",
		       publishReq->rssi_threshold_flag);
		DBGLOG(NAN, INFO, "Recv Indication: %d\n",
		       publishReq->recv_indication_cfg);
		DBGLOG(NAN, INFO, "Rx Match Filter: %s\n",
		       publishReq->rx_match_filter);
		DBGLOG(NAN, INFO, "Rx Match Filter len: %u\n",
		       publishReq->rx_match_filter_len);
		DBGLOG(NAN, INFO, "Tx Match Filter: %s\n",
		       publishReq->tx_match_filter);
		DBGLOG(NAN, INFO, "Tx Match Filter len: %u\n",
		       publishReq->tx_match_filter_len);
		DBGLOG(NAN, INFO, "Config Data Path: %d\n",
		       publishReq->sdea_params.config_nan_data_path);
		DBGLOG(NAN, INFO, "NDP Type: %d\n",
		       publishReq->sdea_params.ndp_type);
		DBGLOG(NAN, INFO, "Security Flag: %d\n",
		       publishReq->sdea_params.security_cfg);
		DBGLOG(NAN, INFO, "Ranging State: %d\n",
		       publishReq->sdea_params.ranging_state);
		DBGLOG(NAN, INFO, "Cipher Type: %d\n", publishReq->cipher_type);
		DBGLOG(NAN, INFO, "Key: %s\n",
		       publishReq->key_info.body.passphrase_info.passphrase);
		if (publishReq->scid_len > NAN_SCID_DEFAULT_LEN)
			publishReq->scid_len = NAN_SCID_DEFAULT_LEN;

		nanUtilDump(prGlueInfo->prAdapter, "Publish SCID",
			    publishReq->scid, publishReq->scid_len);

		pid = (uint16_t)nanPublishRequest(prGlueInfo->prAdapter,
						 publishReq);

		DBGLOG(NAN, INFO, "publish PID: %d\n", pid);

		if (publishReq->sdea_params.security_cfg) {
			/* Fixme: supply a cipher suite list */
			ucCipherType = publishReq->cipher_type;
			nanCmdAddCsid(prGlueInfo->prAdapter, pid, 1,
				      &ucCipherType);

			if (publishReq->scid_len) {
				if (publishReq->scid_len > NAN_SCID_DEFAULT_LEN)
					publishReq->scid_len =
						NAN_SCID_DEFAULT_LEN;

				nanCmdManageScid(prGlueInfo->prAdapter, TRUE,
						 pid, publishReq->scid);
			}
		}

		if (copy_to_user(prIwReqData->data.pointer, &pid,
				 sizeof(signed char))) {
			DBGLOG(REQ, INFO, "copy_to_user oidBuf fail\n");
			status = -EFAULT;
		}
		break;
	}
	case ENUM_CANCEL_PUBLISH: {
		uint32_t rStatus;
		struct NanPublishCancelRequest *cslPublish =
			(struct NanPublishCancelRequest *)&aucOidBuf[0];

		if (prIwReqData->data.length <
			sizeof(struct NanPublishCancelRequest)) {
			DBGLOG(INIT, INFO,
				"ENUM_CANCEL_PUBLISH not have enough length(%d)\n",
				prIwReqData->data.length);
			return -EFAULT;
		}

		DBGLOG(NAN, INFO, "CANCEL Publish Enter\n");
		DBGLOG(NAN, INFO, "PID %d\n", cslPublish->publish_id);
		rStatus = nanCancelPublishRequest(prGlueInfo->prAdapter,
						  cslPublish);
		if (rStatus != WLAN_STATUS_SUCCESS)
			DBGLOG(NAN, INFO, "CANCEL Publish Error %X\n", rStatus);
		break;
	}
	case ENUM_NAN_SUBSCIRBE: {
		struct NanSubscribeRequest *subReq =
			(struct NanSubscribeRequest *)&aucOidBuf[0];
		signed char subid = 25;
#if 0
		UINT_8  *pu1DummyAttrBuf = NULL;
		UINT_32 u4DummyAttrLen = 0;
#endif
		if (prIwReqData->data.length <
			sizeof(struct NanSubscribeRequest)) {
			DBGLOG(INIT, INFO,
				"ENUM_NAN_SUBSCIRBE not have enough length(%d)\n",
				prIwReqData->data.length);
			return -EFAULT;
		}

		DBGLOG(NAN, INFO, "subReq->ttl %d\n", subReq->ttl);
		DBGLOG(NAN, INFO, "subReq->period  %d\n", subReq->period);
		DBGLOG(NAN, INFO, "subReq->subscribe_type %d\n",
		       subReq->subscribe_type);
		DBGLOG(NAN, INFO, "subReq->serviceResponseFilter %d\n",
		       subReq->serviceResponseFilter);
		DBGLOG(NAN, INFO, "subReq->serviceResponseInclude %d\n",
		       subReq->serviceResponseInclude);
		DBGLOG(NAN, INFO, "subReq->useServiceResponseFilter %d\n",
		       subReq->useServiceResponseFilter);
		DBGLOG(NAN, INFO, "subReq->ssiRequiredForMatchIndication %d\n",
		       subReq->ssiRequiredForMatchIndication);
		DBGLOG(NAN, INFO, "subReq->subscribe_match_indicator %d\n",
		       subReq->subscribe_match_indicator);
		DBGLOG(NAN, INFO, "subReq->subscribe_count %d\n",
		       subReq->subscribe_count);
		DBGLOG(NAN, INFO, "subReq->service_name  %s\n",
		       subReq->service_name);
		DBGLOG(NAN, INFO, "subReq->service_specific_info %s\n",
		       subReq->service_specific_info);
		DBGLOG(NAN, INFO, "subReq->rssi_threshold_flag %d\n",
		       subReq->rssi_threshold_flag);
		DBGLOG(NAN, INFO, "subReq->recv_indication_cfg %d\n",
		       subReq->recv_indication_cfg);
		DBGLOG(NAN, INFO,
		       "subReq->sdea_params.config_nan_data_path %d\n",
		       subReq->sdea_params.config_nan_data_path);
		DBGLOG(NAN, INFO, "subReq->sdea_params.ndp_type  %d\n",
		       subReq->sdea_params.ndp_type);
		DBGLOG(NAN, INFO, "subReq->sdea_params.security_cfg %d\n",
		       subReq->sdea_params.security_cfg);
		DBGLOG(NAN, INFO, "subReq->sdea_params.ranging_state %d\n",
		       subReq->sdea_params.ranging_state);
		DBGLOG(NAN, INFO, "subReq->cipher_type %d\n",
		       subReq->cipher_type);
		DBGLOG(NAN, INFO, "subReq->key_info.key_type %d\n",
		       subReq->key_info.key_type);
		DBGLOG(NAN, INFO, "subReq->rx_match_filter %s\n",
		       subReq->rx_match_filter);
		DBGLOG(NAN, INFO, "subReq->tx_match_filter %s\n",
		       subReq->tx_match_filter);

		subid = nanSubscribeRequest(prGlueInfo->prAdapter, subReq);

		if (copy_to_user(prIwReqData->data.pointer, &subid,
				 sizeof(signed char))) {
			DBGLOG(REQ, INFO, "copy_to_user oidBuf fail\n");
			status = -EFAULT;
		}
		break;
	}
	case EMUM_NAN_CANCEL_SUBSCRIBE: {
		uint32_t rStatus;
		struct NanSubscribeCancelRequest *cslsubreq =
			(struct NanSubscribeCancelRequest *)&aucOidBuf[0];

		if (prIwReqData->data.length <
			sizeof(struct NanSubscribeCancelRequest)) {
			DBGLOG(INIT, INFO,
				"EMUM_NAN_CANCEL_SUBSCRIBE no enough length(%d)",
				prIwReqData->data.length);
			return -EFAULT;
		}

		DBGLOG(NAN, INFO, "Cancel Subscribe Enter\n");
		DBGLOG(NAN, INFO, "subid %d\n", cslsubreq->subscribe_id);

		rStatus = nanCancelSubscribeRequest(prGlueInfo->prAdapter,
						    cslsubreq);
		if (rStatus != WLAN_STATUS_SUCCESS) {
			DBGLOG(NAN, INFO, "CANCEL Subscribe Error %X\n",
			       rStatus);
		}
		break;
	}
	case ENUM_NAN_TRANSMIT: {
		struct NanTransmitFollowupRequest *followupreq =
			(struct NanTransmitFollowupRequest *)&aucOidBuf[0];

		DBGLOG(NAN, INFO, "Transmit Enter\n");
		if (prIwReqData->data.length <
			sizeof(struct NanTransmitFollowupRequest)) {
			DBGLOG(INIT, INFO,
				"ENUM_NAN_TRANSMIT not have enough length(%d)\n",
				prIwReqData->data.length);
			return -EFAULT;
		}

		nanTransmitRequest(prGlueInfo->prAdapter, followupreq);
		break;
	}
	case ENUM_NAN_UPDATE_PUBLISH: {
		struct NanPublishRequest *publishReq =
			(struct NanPublishRequest *)&aucOidBuf[0];

		DBGLOG(NAN, INFO, "Update Publish\n");
		if (prIwReqData->data.length <
			sizeof(struct NanPublishRequest)) {
			DBGLOG(INIT, INFO,
				"ENUM_NAN_UPDATE_PUBLISH no enough length(%d)\n",
				prIwReqData->data.length);
			return -EFAULT;
		}
		nanUpdatePublishRequest(prGlueInfo->prAdapter, publishReq);
	} break;
	case ENUM_NAN_GAS_SCHEDULE_REQ:
		DBGLOG(NAN, INFO, "ENUM_NAN_GAS_SCHEDULE_REQ Enter\n");
		break;
	case ENUM_NAN_DATA_REQ: {
		struct NanDataPathInitiatorRequest *prDataReq =
			(struct NanDataPathInitiatorRequest *)&aucOidBuf[0];
		struct NanDataReqReceive rDataRcv;
		struct _NAN_CMD_DATA_REQUEST rNanCmdDataRequest;
		uint32_t rStatus;

		if (prIwReqData->data.length <
			sizeof(struct NanDataPathInitiatorRequest)) {
			DBGLOG(INIT, INFO,
				"ENUM_NAN_DATA_REQ no enough length(%d)\n",
				prIwReqData->data.length);
			return -EFAULT;
		}

		kalMemZero(&rNanCmdDataRequest, sizeof(rNanCmdDataRequest));

		/* Retrieve to NDP */
		rNanCmdDataRequest.ucType = prDataReq->type;

		DBGLOG(NAN, INFO, "[Data Req] ReqID:%d\n",
		       prDataReq->requestor_instance_id);
		rNanCmdDataRequest.ucPublishID =
			prDataReq->requestor_instance_id;
		if (rNanCmdDataRequest.ucPublishID == 0)
			rNanCmdDataRequest.ucPublishID = g_u2IndPubId;

		rNanCmdDataRequest.ucSecurity =
			prDataReq->cipher_type; /*chiper type*/
		if (rNanCmdDataRequest.ucSecurity) {
			if (prDataReq->scid_len != NAN_SCID_DEFAULT_LEN) {
				DBGLOG(NAN, ERROR,
					"prDataReq->scid_len != NAN_SCID_DEFAULT_LEN!\n");
				return -EFAULT;
			}
			kalMemCopy(rNanCmdDataRequest.aucScid, prDataReq->scid,
				   NAN_SCID_DEFAULT_LEN);
			if (prDataReq->key_info.body.pmk_info.pmk_len >
				NAN_PMK_INFO_LEN)
				prDataReq->key_info.body.pmk_info.pmk_len =
				NAN_PMK_INFO_LEN;
			kalMemCopy(rNanCmdDataRequest.aucPMK,
				   prDataReq->key_info.body.pmk_info.pmk,
				   prDataReq->key_info.body.pmk_info.pmk_len);
#if (ENABLE_SEC_UT_LOG == 1)
			DBGLOG(NAN, INFO, "PMK from APP\n");
			dumpMemory8(prDataReq->key_info.body.pmk_info.pmk,
				    prDataReq->key_info.body.pmk_info.pmk_len);
#endif
		}

		rNanCmdDataRequest.ucRequireQOS = prDataReq->ndp_cfg.qos_cfg;
		rNanCmdDataRequest.ucMinTimeSlot = prDataReq->ucMinTimeSlot;
		rNanCmdDataRequest.u2MaxLatency = prDataReq->u2MaxLatency;

		kalMemCopy(rNanCmdDataRequest.aucResponderDataAddress,
			   prDataReq->peer_disc_mac_addr, MAC_ADDR_LEN);

		rNanCmdDataRequest.fgCarryIpv6 = prDataReq->fgCarryIpv6;
		if (rNanCmdDataRequest.fgCarryIpv6 == TRUE)
			kalMemCopy(rNanCmdDataRequest.aucIPv6Addr,
				   prDataReq->aucIPv6Addr, IPV6MACLEN);

		if (prDataReq->app_info.ndp_app_info_len >
			NAN_DP_MAX_APP_INFO_LEN) {
			prDataReq->app_info.ndp_app_info_len =
				NAN_DP_MAX_APP_INFO_LEN;
		}
		rNanCmdDataRequest.u2SpecificInfoLength =
			prDataReq->app_info.ndp_app_info_len;
		if (rNanCmdDataRequest.u2SpecificInfoLength >
			NAN_DP_MAX_APP_INFO_LEN)
			rNanCmdDataRequest.u2SpecificInfoLength =
			NAN_DP_MAX_APP_INFO_LEN;
		kalMemCopy(rNanCmdDataRequest.aucSpecificInfo,
			   prDataReq->app_info.ndp_app_info,
			   prDataReq->app_info.ndp_app_info_len);

		rStatus = nanCmdDataRequest(
			prGlueInfo->prAdapter, &rNanCmdDataRequest,
			&rDataRcv.ndpid, rDataRcv.initiator_data_addr);

		/* Return to APP */
		if (rStatus == WLAN_STATUS_SUCCESS) {
			if (copy_to_user(prIwReqData->data.pointer, &rDataRcv,
					 sizeof(struct NanDataReqReceive))) {
				DBGLOG(NAN, WARN, "copy_to_user oidBuf fail\n");
				status = -EFAULT;
			}
		}
		break;
	}
	case ENUM_NAN_DATA_RESP: {
		struct NanDataPathIndicationResponse *prDataRes =
			(struct NanDataPathIndicationResponse *)&aucOidBuf[0];
		struct _NAN_CMD_DATA_RESPONSE rNanCmdDataResponse;
		int32_t rStatus;

		if (prIwReqData->data.length <
			sizeof(struct NanDataPathIndicationResponse)) {
			DBGLOG(INIT, INFO,
				"ENUM_NAN_DATA_RESP no enough length(%d)\n",
				prIwReqData->data.length);
			return -EFAULT;
		}

		rNanCmdDataResponse.ucType = prDataRes->type;
		rNanCmdDataResponse.ucDecisionStatus = NAN_DP_REQUEST_ACCEPT;
		rNanCmdDataResponse.ucNDPId = prDataRes->ndp_instance_id;
		rNanCmdDataResponse.ucRequireQOS = prDataRes->ndp_cfg.qos_cfg;
		rNanCmdDataResponse.ucSecurity = prDataRes->cipher_type;

		if (prDataRes->app_info.ndp_app_info_len >
			NAN_DP_MAX_APP_INFO_LEN) {
			prDataRes->app_info.ndp_app_info_len =
				NAN_DP_MAX_APP_INFO_LEN;
		}

		rNanCmdDataResponse.u2SpecificInfoLength =
			prDataRes->app_info.ndp_app_info_len;
		rNanCmdDataResponse.fgCarryIpv6 = prDataRes->fgCarryIpv6;
		rNanCmdDataResponse.u2PortNum = prDataRes->u2PortNum;
		rNanCmdDataResponse.ucServiceProtocolType =
			prDataRes->ucServiceProtocolType;
		rNanCmdDataResponse.ucMinTimeSlot = prDataRes->ucMinTimeSlot;
		rNanCmdDataResponse.u2MaxLatency = prDataRes->u2MaxLatency;
		rNanCmdDataResponse.u2NdpTransactionId = 0;

		kalMemCopy(rNanCmdDataResponse.aucInitiatorDataAddress,
			   prDataRes->initiator_mac_addr, MAC_ADDR_LEN);
		if (prDataRes->key_info.body.pmk_info.pmk_len >
			NAN_PMK_INFO_LEN)
			prDataRes->key_info.body.pmk_info.pmk_len =
			NAN_PMK_INFO_LEN;
		kalMemCopy(rNanCmdDataResponse.aucPMK,
			   prDataRes->key_info.body.pmk_info.pmk,
			   prDataRes->key_info.body.pmk_info.pmk_len);

		if (rNanCmdDataResponse.u2SpecificInfoLength >
			NAN_DP_MAX_APP_INFO_LEN)
			rNanCmdDataResponse.u2SpecificInfoLength =
			NAN_DP_MAX_APP_INFO_LEN;
		kalMemCopy(rNanCmdDataResponse.aucSpecificInfo,
			   prDataRes->app_info.ndp_app_info,
			   prDataRes->app_info.ndp_app_info_len);
		if (rNanCmdDataResponse.fgCarryIpv6 == TRUE)
			kalMemCopy(rNanCmdDataResponse.aucIPv6Addr,
				   prDataRes->aucIPv6Addr, IPV6MACLEN);
#if (ENABLE_SEC_UT_LOG == 1)
		DBGLOG(NAN, INFO, "PMK from APP\n");
		dumpMemory8(prDataRes->key_info.body.pmk_info.pmk,
			    prDataRes->key_info.body.pmk_info.pmk_len);
#endif

		rStatus = nanCmdDataResponse(prGlueInfo->prAdapter,
					     &rNanCmdDataResponse);
		break;
	}
	case ENUM_NAN_DATA_END: {
		struct NanDataPathEndRequest *prDataEnd =
			(struct NanDataPathEndRequest *)&aucOidBuf[0];
		struct _NAN_CMD_DATA_END rNanCmdDataEnd;
		uint32_t rStatus;

		if (prIwReqData->data.length <
			sizeof(struct NanDataPathEndRequest)) {
			DBGLOG(INIT, INFO,
				"ENUM_NAN_DATA_END not enough length(%d)",
				prIwReqData->data.length);
			return -EFAULT;
		}

		rNanCmdDataEnd.ucType = prDataEnd->type;
		rNanCmdDataEnd.ucNDPId = prDataEnd->ndp_instance_id;
		kalMemCopy(rNanCmdDataEnd.aucInitiatorDataAddress,
			   prDataEnd->initiator_mac_addr, MAC_ADDR_LEN);

		rStatus = nanCmdDataEnd(prGlueInfo->prAdapter, &rNanCmdDataEnd);

		break;
	}
	case ENUM_NAN_DATA_UPDTAE: {
		/* Android struct is not defined */
		struct NanDataPathInitiatorRequest *prDataUpd =
			(struct NanDataPathInitiatorRequest *)&aucOidBuf[0];
		struct _NAN_PARAMETER_NDL_SCH rNanUpdateSchParam;
		uint32_t rStatus;

		if (prIwReqData->data.length <
			sizeof(struct NanDataPathInitiatorRequest)) {
			DBGLOG(INIT, INFO,
				"ENUM_NAN_DATA_UPDTAE no enough length(%d)\n",
				prIwReqData->data.length);
			return -EFAULT;
		}

		rNanUpdateSchParam.ucType = prDataUpd->type;
		rNanUpdateSchParam.ucRequireQOS = prDataUpd->ndp_cfg.qos_cfg;
		rNanUpdateSchParam.ucNDPId = prDataUpd->requestor_instance_id;
		rNanUpdateSchParam.ucMinTimeSlot = prDataUpd->ucMinTimeSlot;
		rNanUpdateSchParam.u2MaxLatency = prDataUpd->u2MaxLatency;
		kalMemCopy(rNanUpdateSchParam.aucPeerDataAddress,
			   prDataUpd->peer_disc_mac_addr, MAC_ADDR_LEN);
		rStatus = nanCmdDataUpdtae(prGlueInfo->prAdapter,
					   &rNanUpdateSchParam);

		break;
	}
	case ENUM_NAN_RG_REQ: {
		struct NanRangeRequest *rgreq =
			(struct NanRangeRequest *)&aucOidBuf[0];
		uint16_t rgId = 0;
		uint32_t rStatus;

		if (prIwReqData->data.length <
			sizeof(struct NanRangeRequest)) {
			DBGLOG(INIT, INFO,
				"ENUM_NAN_RG_REQ not enough length(%d)",
				prIwReqData->data.length);
			return -EFAULT;
		}

		DBGLOG(NAN, INFO, MACSTR
		       " reso %d intev %d indicat %d ING CM %d ENG CM %d\n",
		       MAC2STR(rgreq->peer_addr),
		       rgreq->ranging_cfg.ranging_resolution,
		       rgreq->ranging_cfg.ranging_interval_msec,
		       rgreq->ranging_cfg.config_ranging_indications,
		       rgreq->ranging_cfg.distance_ingress_cm,
		       rgreq->ranging_cfg.distance_egress_cm);

		rStatus =
			nanRangingRequest(prGlueInfo->prAdapter, &rgId, rgreq);

		if (rStatus == WLAN_STATUS_SUCCESS) {
			if (copy_to_user(prIwReqData->data.pointer, &rgId,
					 sizeof(uint16_t))) {
				DBGLOG(NAN, WARN, "copy_to_user oidBuf fail\n");
				status = -EFAULT;
			}
		}
		break;
	}
	case ENUM_NAN_RG_CANCEL: {
		struct NanRangeCancelRequest *rgend =
			(struct NanRangeCancelRequest *)&aucOidBuf[0];
		uint32_t rStatus;

		if (prIwReqData->data.length <
			sizeof(struct NanRangeCancelRequest)) {
			DBGLOG(INIT, INFO,
				"ENUM_NAN_RG_CANCEL not enough length(%d)",
				prIwReqData->data.length);
			return -EFAULT;
		}

		rStatus = nanRangingCancel(prGlueInfo->prAdapter, rgend);

		DBGLOG(NAN, INFO, "ret %d " MACSTR "\n", rStatus,
		       MAC2STR(rgend->peer_addr));
		break;
	}
	case ENUM_NAN_RG_RESP: {
		struct NanRangeResponse *rgrsp =
			(struct NanRangeResponse *)&aucOidBuf[0];
		uint32_t rStatus;

		if (prIwReqData->data.length <
			sizeof(struct NanRangeResponse)) {
			DBGLOG(INIT, INFO,
				"ENUM_NAN_RG_RESP not enough length(%d)",
				prIwReqData->data.length);
			return -EFAULT;
		}

		DBGLOG(NAN, INFO, "rgId %d alt %d rpt %d rsp %d\n",
		       rgrsp->range_id,
		       rgrsp->response_ctl.ranging_auto_response,
		       rgrsp->response_ctl.range_report,
		       rgrsp->response_ctl.ranging_response_code);

		DBGLOG(NAN, INFO,
		       "reso %d intev %d indicat %d ING CM %d ENG CM %d\n",
		       rgrsp->ranging_cfg.ranging_resolution,
		       rgrsp->ranging_cfg.ranging_interval_msec,
		       rgrsp->ranging_cfg.config_ranging_indications,
		       rgrsp->ranging_cfg.distance_ingress_cm,
		       rgrsp->ranging_cfg.distance_egress_cm);

		rStatus = nanRangingResponse(prGlueInfo->prAdapter, rgrsp);

		break;
	}
	case ENUM_NAN_ENABLE_REQ: {
		struct NanEnableRequest *prEnableReq =
			(struct NanEnableRequest *)&aucOidBuf[0];
		enum NanStatusType nanRetStatus;

		if (prIwReqData->data.length <
			sizeof(struct NanEnableRequest)) {
			DBGLOG(INIT, INFO,
				"ENUM_NAN_ENABLE_REQ not enough length(%d)",
				prIwReqData->data.length);
			return -EFAULT;
		}

		nanRetStatus =
			nanDevEnableRequest(prGlueInfo->prAdapter, prEnableReq);
		if (copy_to_user(prIwReqData->data.pointer, &nanRetStatus,
				 sizeof(enum NanStatusType))) {
			DBGLOG(REQ, INFO, "copy_to_user oidBuf fail\n");
			status = -EFAULT;
		}
		break;
	}
	case ENUM_NAN_DISABLE_REQ: {
		enum NanStatusType nanRetStatus;

		nanRetStatus = nanDevDisableRequest(prGlueInfo->prAdapter);
		if (copy_to_user(prIwReqData->data.pointer, &nanRetStatus,
				 sizeof(enum NanStatusType))) {
			DBGLOG(REQ, INFO, "copy_to_user oidBuf fail\n");
			status = -EFAULT;
		}
		break;
	}
	case ENUM_NAN_CONFIG_MP: {
		uint8_t *prMasterPref = (uint8_t *)&aucOidBuf[0];

		nanDevSetMasterPreference(prGlueInfo->prAdapter, *prMasterPref);
		break;
	}
	case ENUM_NAN_CONFIG_HC: {
		break;
	}
	case ENUM_NAN_CONFIG_RANFAC: {
		break;
	}
	default:
		return -EOPNOTSUPP;
	}

	return status;
}

int
priv_nan_struct(struct net_device *prNetDev,
		struct iw_request_info *prIwReqInfo,
		union iwreq_data *prIwReqData, char *pcExtra) {
	DBGLOG(REQ, INFO, "cmd=%x, flags=%x\n", prIwReqInfo->cmd,
	       prIwReqInfo->flags);
	DBGLOG(REQ, INFO, "mode=%x, flags=%x\n", prIwReqData->mode,
	       prIwReqData->data.flags);

	return compat_priv(prNetDev, prIwReqInfo, prIwReqData, pcExtra,
			   __priv_nan_struct);
}

#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief The routine handles a set operation for a single OID.
 *
 * \param[in] pDev Net device requested.
 * \param[in] ndisReq Ndis request OID information copy from user.
 * \param[out] outputLen_p If the call is successful, returns the number of
 *                         bytes written into the query buffer. If the
 *                         call failed due to invalid length of the query
 *                         buffer, returns the amount of storage needed..
 *
 * \retval 0 On success.
 * \retval -EOPNOTSUPP If cmd is not supported.
 *
 */
/*----------------------------------------------------------------------------*/
static int
priv_set_ndis(struct net_device *prNetDev,
	      struct NDIS_TRANSPORT_STRUCT *prNdisReq,
	      uint32_t *pu4OutputLen)
{
	struct WLAN_REQ_ENTRY *prWlanReqEntry = NULL;
	uint32_t status = WLAN_STATUS_SUCCESS;
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t u4SetInfoLen = 0;

	ASSERT(prNetDev);
	ASSERT(prNdisReq);
	ASSERT(pu4OutputLen);

	if (!prNetDev || !prNdisReq || !pu4OutputLen) {
		DBGLOG(REQ, INFO,
		       "priv_set_ndis(): invalid param(0x%p, 0x%p, 0x%p)\n",
		       prNetDev, prNdisReq, pu4OutputLen);
		return -EINVAL;
	}

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	ASSERT(prGlueInfo);
	if (!prGlueInfo) {
		DBGLOG(REQ, INFO,
		       "priv_set_ndis(): invalid prGlueInfo(0x%p, 0x%p)\n",
		       prNetDev,
		       *((struct GLUE_INFO **) netdev_priv(prNetDev)));
		return -EINVAL;
	}
#if 0
	DBGLOG(INIT, INFO,
	       "priv_set_ndis(): prNdisReq->ndisOidCmd(0x%lX)\n",
	       prNdisReq->ndisOidCmd);
#endif

	if (reqSearchSupportedOidEntry(prNdisReq->ndisOidCmd,
				       &prWlanReqEntry) == FALSE) {
		/* WARNLOG(
		 *         ("Set OID: 0x%08lx (unknown)\n",
		 *         prNdisReq->ndisOidCmd));
		 */
		return -EOPNOTSUPP;
	}

	if (prWlanReqEntry->pfOidSetHandler == NULL) {
		/* WARNLOG(
		 *         ("Set %s: Null set handler\n",
		 *         prWlanReqEntry->pucOidName));
		 */
		return -EOPNOTSUPP;
	}
#if 0
	DBGLOG(INIT, INFO, "priv_set_ndis(): %s\n",
	       prWlanReqEntry->pucOidName);
#endif

	if (prWlanReqEntry->fgSetBufLenChecking) {
		if (prNdisReq->inNdisOidlength !=
		    prWlanReqEntry->u4InfoBufLen) {
			DBGLOG(REQ, WARN,
			       "Set %s: Invalid length (current=%d, needed=%d)\n",
			       prWlanReqEntry->pucOidName,
			       prNdisReq->inNdisOidlength,
			       prWlanReqEntry->u4InfoBufLen);

			*pu4OutputLen = prWlanReqEntry->u4InfoBufLen;
			return -EINVAL;
		}
	}

	if (prWlanReqEntry->eOidMethod == ENUM_OID_GLUE_ONLY) {
		/* GLUE sw info only */
		status = prWlanReqEntry->pfOidSetHandler(prGlueInfo,
				prNdisReq->ndisOidContent,
				prNdisReq->inNdisOidlength, &u4SetInfoLen);
	} else if (prWlanReqEntry->eOidMethod ==
		   ENUM_OID_GLUE_EXTENSION) {
		/* multiple sw operations */
		status = prWlanReqEntry->pfOidSetHandler(prGlueInfo,
				prNdisReq->ndisOidContent,
				prNdisReq->inNdisOidlength, &u4SetInfoLen);
	} else if (prWlanReqEntry->eOidMethod ==
		   ENUM_OID_DRIVER_CORE) {
		/* driver core */

		status = kalIoctl(prGlueInfo,
			(PFN_OID_HANDLER_FUNC) prWlanReqEntry->pfOidSetHandler,
			prNdisReq->ndisOidContent,
			prNdisReq->inNdisOidlength, &u4SetInfoLen);
	} else {
		DBGLOG(REQ, INFO,
		       "priv_set_ndis(): unsupported OID method:0x%x\n",
		       prWlanReqEntry->eOidMethod);
		return -EOPNOTSUPP;
	}

	*pu4OutputLen = u4SetInfoLen;

	switch (status) {
	case WLAN_STATUS_SUCCESS:
		break;

	case WLAN_STATUS_INVALID_LENGTH:
		/* WARNLOG(
		 * ("Set %s: Invalid length (current=%ld, needed=%ld)\n",
		 * prWlanReqEntry->pucOidName,
		 * prNdisReq->inNdisOidlength,
		 * u4SetInfoLen));
		 */
		break;
	}

	if (status != WLAN_STATUS_SUCCESS)
		return -EFAULT;

	return 0;
}				/* priv_set_ndis */

/*----------------------------------------------------------------------------*/
/*!
 * \brief The routine handles a query operation for a single OID. Basically we
 *   return information about the current state of the OID in question.
 *
 * \param[in] pDev Net device requested.
 * \param[in] ndisReq Ndis request OID information copy from user.
 * \param[out] outputLen_p If the call is successful, returns the number of
 *                        bytes written into the query buffer. If the
 *                        call failed due to invalid length of the query
 *                        buffer, returns the amount of storage needed..
 *
 * \retval 0 On success.
 * \retval -EOPNOTSUPP If cmd is not supported.
 * \retval -EINVAL invalid input parameters
 *
 */
/*----------------------------------------------------------------------------*/
static int
priv_get_ndis(struct net_device *prNetDev,
	      struct NDIS_TRANSPORT_STRUCT *prNdisReq,
	      uint32_t *pu4OutputLen)
{
	struct WLAN_REQ_ENTRY *prWlanReqEntry = NULL;
	uint32_t u4BufLen = 0;
	uint32_t status = WLAN_STATUS_SUCCESS;
	struct GLUE_INFO *prGlueInfo = NULL;

	ASSERT(prNetDev);
	ASSERT(prNdisReq);
	ASSERT(pu4OutputLen);

	if (!prNetDev || !prNdisReq || !pu4OutputLen) {
		DBGLOG(REQ, INFO,
		       "priv_get_ndis(): invalid param(0x%p, 0x%p, 0x%p)\n",
		       prNetDev, prNdisReq, pu4OutputLen);
		return -EINVAL;
	}

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	ASSERT(prGlueInfo);
	if (!prGlueInfo) {
		DBGLOG(REQ, INFO,
		       "priv_get_ndis(): invalid prGlueInfo(0x%p, 0x%p)\n",
		       prNetDev,
		       *((struct GLUE_INFO **) netdev_priv(prNetDev)));
		return -EINVAL;
	}
#if 0
	DBGLOG(INIT, INFO,
	       "priv_get_ndis(): prNdisReq->ndisOidCmd(0x%lX)\n",
	       prNdisReq->ndisOidCmd);
#endif

	if (reqSearchSupportedOidEntry(prNdisReq->ndisOidCmd,
				       &prWlanReqEntry) == FALSE) {
		/* WARNLOG(
		 *         ("Query OID: 0x%08lx (unknown)\n",
		 *         prNdisReq->ndisOidCmd));
		 */
		return -EOPNOTSUPP;
	}

	if (prWlanReqEntry->pfOidQueryHandler == NULL) {
		/* WARNLOG(
		 *         ("Query %s: Null query handler\n",
		 *         prWlanReqEntry->pucOidName));
		 */
		return -EOPNOTSUPP;
	}
#if 0
	DBGLOG(INIT, INFO, "priv_get_ndis(): %s\n",
	       prWlanReqEntry->pucOidName);
#endif

	if (prWlanReqEntry->fgQryBufLenChecking) {
		if (prNdisReq->inNdisOidlength <
		    prWlanReqEntry->u4InfoBufLen) {
			/* Not enough room in InformationBuffer. Punt */
			/* WARNLOG(
			 * ("Query %s: Buffer too short (current=%ld,
			 * needed=%ld)\n",
			 * prWlanReqEntry->pucOidName,
			 * prNdisReq->inNdisOidlength,
			 * prWlanReqEntry->u4InfoBufLen));
			 */

			*pu4OutputLen = prWlanReqEntry->u4InfoBufLen;

			status = WLAN_STATUS_INVALID_LENGTH;
			return -EINVAL;
		}
	}

	if (prWlanReqEntry->eOidMethod == ENUM_OID_GLUE_ONLY) {
		/* GLUE sw info only */
		status = prWlanReqEntry->pfOidQueryHandler(prGlueInfo,
				prNdisReq->ndisOidContent,
				prNdisReq->inNdisOidlength, &u4BufLen);
	} else if (prWlanReqEntry->eOidMethod ==
		   ENUM_OID_GLUE_EXTENSION) {
		/* multiple sw operations */
		status = prWlanReqEntry->pfOidQueryHandler(prGlueInfo,
				prNdisReq->ndisOidContent,
				prNdisReq->inNdisOidlength, &u4BufLen);
	} else if (prWlanReqEntry->eOidMethod ==
		   ENUM_OID_DRIVER_CORE) {
		/* driver core */

		status = kalIoctl(prGlueInfo,
		    (PFN_OID_HANDLER_FUNC)prWlanReqEntry->pfOidQueryHandler,
		    prNdisReq->ndisOidContent, prNdisReq->inNdisOidlength,
		    &u4BufLen);
	} else {
		DBGLOG(REQ, INFO,
		       "priv_set_ndis(): unsupported OID method:0x%x\n",
		       prWlanReqEntry->eOidMethod);
		return -EOPNOTSUPP;
	}

	*pu4OutputLen = u4BufLen;

	switch (status) {
	case WLAN_STATUS_SUCCESS:
		break;

	case WLAN_STATUS_INVALID_LENGTH:
		/* WARNLOG(
		 * ("Set %s: Invalid length (current=%ld, needed=%ld)\n",
		 *  prWlanReqEntry->pucOidName,
		 *  prNdisReq->inNdisOidlength,
		 *  u4BufLen));
		 */
		break;
	}

	if (status != WLAN_STATUS_SUCCESS)
		return -EOPNOTSUPP;

	return 0;
}				/* priv_get_ndis */

#if CFG_SUPPORT_QA_TOOL
/*----------------------------------------------------------------------------*/
/*!
 * \brief The routine handles ATE set operation.
 *
 * \param[in] pDev Net device requested.
 * \param[in] prIwReqInfo Pointer to iwreq structure.
 * \param[in] prIwReqData The ioctl data structure, use the field of
 *            sub-command.
 * \param[in] pcExtra The buffer with input value
 *
 *
 * \retval 0 On success.
 * \retval -EOPNOTSUPP If cmd is not supported.
 * \retval -EFAULT If copy from user space buffer fail.
 *
 */
/*----------------------------------------------------------------------------*/
int
__priv_ate_set(struct net_device *prNetDev,
	     struct iw_request_info *prIwReqInfo,
	     union iwreq_data *prIwReqData,
	     char *pcExtra)
{
	int32_t i4Status;
	/* uint8_t *InBuf;
	 * uint8_t *addr_str, *value_str;
	 * uint32_t InBufLen;
	 */
	uint32_t u4SubCmd;
	/* u_int8_t isWrite = 0;
	 * uint32_t u4BufLen = 0;
	 * struct NDIS_TRANSPORT_STRUCT *prNdisReq;
	 * uint32_t pu4IntBuf[2];
	 */
	uint32_t u4CopySize = sizeof(aucOidBuf);

	/* sanity check */
	ASSERT(prNetDev);
	ASSERT(prIwReqInfo);
	ASSERT(prIwReqData);
	ASSERT(pcExtra);

	if (GLUE_CHK_PR3(prNetDev, prIwReqData, pcExtra) == FALSE)
		return -EINVAL;

	u4SubCmd = (uint32_t) prIwReqData->data.flags;
	DBGLOG(REQ, INFO, "MT6632: %s, u4SubCmd=%d mode=%d\n", __func__,
	       u4SubCmd, (uint32_t) prIwReqData->mode);

	switch (u4SubCmd) {
	case PRIV_QACMD_SET:
		u4CopySize = (prIwReqData->data.length < u4CopySize)
			     ? prIwReqData->data.length : (u4CopySize - 1);
		if (copy_from_user(&aucOidBuf[0], prIwReqData->data.pointer,
				   u4CopySize))
			return -EFAULT;
		aucOidBuf[u4CopySize] = '\0';
		DBGLOG(REQ, INFO,
		       "PRIV_QACMD_SET: priv_set_string=(%s)(%u,%d)\n",
		       aucOidBuf, u4CopySize,
		       (int32_t)prIwReqData->data.length);
		i4Status = AteCmdSetHandle(prNetDev, &aucOidBuf[0],
					   u4CopySize);
		break;
	default:
		return -EOPNOTSUPP;
	}
	return 0;
}

int
priv_ate_set(struct net_device *prNetDev,
	     struct iw_request_info *prIwReqInfo,
	     union iwreq_data *prIwReqData, char *pcExtra)
{
#if BUILD_QA_DBG
	DBGLOG(REQ, LOUD, "cmd=%x, flags=%x\n",
	     prIwReqInfo->cmd, prIwReqInfo->flags);
	DBGLOG(REQ, LOUD, "mode=%x, flags=%x\n",
	     prIwReqData->mode, prIwReqData->data.flags);

	return compat_priv(prNetDev, prIwReqInfo,
	     prIwReqData, pcExtra, __priv_ate_set);
#else
	DBGLOG(REQ, WARN, "not support in user load");
	return 0;
#endif
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to search desired OID.
 *
 * \param rOid[in]               Desired NDIS_OID
 * \param ppWlanReqEntry[out]    Found registered OID entry
 *
 * \retval TRUE: Matched OID is found
 * \retval FALSE: No matched OID is found
 */
/*----------------------------------------------------------------------------*/
static u_int8_t reqSearchSupportedOidEntry(uint32_t rOid,
		struct WLAN_REQ_ENTRY **ppWlanReqEntry)
{
	uint32_t i, j, k;

	i = 0;
	j = NUM_SUPPORTED_OIDS - 1;

	while (i <= j) {
		k = i + (j - i) / 2;

		if (rOid == arWlanOidReqTable[k].rOid) {
			*ppWlanReqEntry = &arWlanOidReqTable[k];
			return TRUE;
		} else if (rOid < arWlanOidReqTable[k].rOid) {
			j = k - 1;
		} else {
			i = k + 1;
		}

		if (k == 0)
			break;
	}

	return FALSE;
}				/* reqSearchSupportedOidEntry */
/* fos_change begin */
int
priv_get_string(struct net_device *prNetDev,
		struct iw_request_info *prIwReqInfo,
		union iwreq_data *prIwReqData, char *pcExtra)
{
#if BUILD_QA_DBG
	uint32_t u4SubCmd = 0;
	uint32_t u4TotalLen = 2000;
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t u4BufLen = 0;
	int32_t i, pos = 0;
	char *buf = pcExtra;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	if (!prNetDev || !prIwReqData) {
		DBGLOG(REQ, INFO,
			"priv_get_string(): invalid param(0x%p, 0x%p)\n",
			prNetDev, prIwReqData);
		return -EINVAL;
	}


	u4SubCmd = (uint32_t) prIwReqData->data.flags;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	if (!prGlueInfo) {
		DBGLOG(REQ, INFO,
			"priv_get_string(): invalid prGlueInfo(0x%p, 0x%p)\n",
			prNetDev,
			*((struct GLUE_INFO **) netdev_priv(prNetDev)));
		return -EINVAL;
	}

	if (pcExtra)
		pcExtra[u4TotalLen-1] = '\0';

	pos += kalScnprintf(buf + pos, u4TotalLen - pos, "\n");

	switch (u4SubCmd) {
	case PRIV_CMD_CONNSTATUS:
	{
		uint8_t arBssid[PARAM_MAC_ADDR_LEN];
		struct PARAM_SSID rSsid = {0};

		kalMemZero(arBssid, PARAM_MAC_ADDR_LEN);
		rStatus = kalIoctl(prGlueInfo, wlanoidQueryBssid,
				   &arBssid[0], sizeof(arBssid), &u4BufLen);
		if (rStatus == WLAN_STATUS_SUCCESS) {
			kalIoctl(prGlueInfo, wlanoidQuerySsid,
				 &rSsid, sizeof(rSsid), &u4BufLen);

			pos += kalScnprintf(buf + pos, u4TotalLen - pos,
				"connStatus: Connected (AP: %s ",
				rSsid.aucSsid);

			for (i = 0; i < PARAM_MAC_ADDR_LEN; i++) {
				pos += kalScnprintf(buf + pos,
					u4TotalLen - pos, "%02x", arBssid[i]);
				if (i != PARAM_MAC_ADDR_LEN - 1)
					pos += kalScnprintf(buf + pos,
						u4TotalLen - pos, ":");
			}
			pos += kalScnprintf(buf + pos, u4TotalLen - pos, ")");
		} else {
			pos += kalScnprintf(buf + pos,
				u4TotalLen - pos,
				"connStatus: Not connected");
		}
	}
	break;
#if CFG_SUPPORT_STAT_STATISTICS
	case PRIV_CMD_STAT:
	{
		static uint8_t aucBuffer[512];
		struct CMD_SW_DBG_CTRL *pSwDbgCtrl = NULL;
		int32_t i4Rssi = -127;
		uint32_t u4Rate = 0;
		int8_t ucAvgNoise = 0;
		uint8_t arBssid[PARAM_MAC_ADDR_LEN];
		struct BSS_INFO *prBssInfo = NULL;
		struct STA_RECORD *prStaRec = NULL;
		struct RX_CTRL *prRxCtrl = NULL;
		struct PARAM_GET_STA_STATISTICS rQueryStaStatistics;
		struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
		struct MIB_STATS *prMibStats = prAdapter->rMibStats;
#endif
		struct PARAM_HW_MIB_INFO *prHwMibInfo;
		struct PARAM_LINK_SPEED_EX rLinkSpeed;
		uint8_t ucBssIndex = wlanGetBssIdx(prNetDev);

		pSwDbgCtrl = (struct CMD_SW_DBG_CTRL *)aucBuffer;
		prRxCtrl = &prAdapter->rRxCtrl;

		prHwMibInfo = (struct PARAM_HW_MIB_INFO *)kalMemAlloc(
			sizeof(struct PARAM_HW_MIB_INFO), VIR_MEM_TYPE);
		if (!prHwMibInfo)
			return -1;
		kalMemZero(prHwMibInfo, sizeof(struct PARAM_HW_MIB_INFO));
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
		prHwMibInfo->u2Index = 0;
		prHwMibInfo->u2TagCount = (
			(UNI_CMD_MIB_CNT_RX_FIFO_OVERFLOW -
				UNI_CMD_MIB_CNT_RX_FCS_ERR + 1) +
			(UNI_CMD_MIB_CNT_S_CCA_TIME -
				UNI_CMD_MIB_CNT_P_CCA_TIME + 1) +
			1 /* UNI_CMD_MIB_CNT_BSS1_FRAME_RETRY */
		);

		/* 0~1, 17~18 */
		SET_BITMAP_RANGE(prHwMibInfo->au4TagBitmap,
			UNI_CMD_MIB_CNT_RX_FCS_ERR,
			UNI_CMD_MIB_CNT_RX_FIFO_OVERFLOW);
		SET_BITMAP_RANGE(prHwMibInfo->au4TagBitmap,
			UNI_CMD_MIB_CNT_P_CCA_TIME, UNI_CMD_MIB_CNT_S_CCA_TIME);
		/* 74 */
		SET_BITMAP(prHwMibInfo->au4TagBitmap,
			UNI_CMD_MIB_CNT_BSS1_FRAME_RETRY);
#else
		prHwMibInfo->u4Index = 0;
#endif

		kalMemZero(arBssid, MAC_ADDR_LEN);
		kalMemZero(&rQueryStaStatistics, sizeof(rQueryStaStatistics));

		if (kalIoctl(prGlueInfo, wlanoidQueryBssid,
				 &arBssid[0], sizeof(arBssid),
				 &u4BufLen) == WLAN_STATUS_SUCCESS) {

			COPY_MAC_ADDR(rQueryStaStatistics.aucMacAddr, arBssid);
			rQueryStaStatistics.ucReadClear = TRUE;

			rStatus = kalIoctl(prGlueInfo,
				wlanoidQueryStaStatistics,
				&rQueryStaStatistics,
				sizeof(rQueryStaStatistics), &u4BufLen);
		}

		if (kalIoctl(prGlueInfo, wlanoidQueryMibInfo, prHwMibInfo,
			sizeof(struct PARAM_HW_MIB_INFO),
			&u4BufLen) == WLAN_STATUS_SUCCESS) {
			if (prHwMibInfo != NULL && prRxCtrl != NULL) {
				if (pSwDbgCtrl->u4Data == SWCR_DBG_TYPE_ALL) {
					pos += kalScnprintf(buf + pos,
					u4TotalLen - pos,
					"Tx success = %d\n",
					rQueryStaStatistics
					.u4TransmitCount);

					pos += kalScnprintf(buf + pos,
					u4TotalLen - pos,
					"Tx retry count = %d\n",
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
					prMibStats->au4FrameRetryCnt[BSSID_1]
#else
					prHwMibInfo->rHwMibCnt
					.au4FrameRetryCnt[BSSID_1]
#endif
					);

					pos += kalScnprintf(buf + pos,
					u4TotalLen - pos,
					"Tx fail to Rcv ACK after retry = %d\n",
					rQueryStaStatistics
					.u4TxFailCount +
					rQueryStaStatistics
					.u4TxLifeTimeoutCount);

					pos += kalScnprintf(buf + pos,
					u4TotalLen - pos,
					"Rx success = %d\n",
					RX_GET_CNT(prRxCtrl,
					RX_MPDU_TOTAL_COUNT));

					pos += kalScnprintf(buf + pos,
					u4TotalLen - pos,
					"Rx with CRC = %d\n",
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
					prMibStats->u4RxFcsErrCnt
#else
					prHwMibInfo->rHwMibCnt
					.u4RxFcsErrCnt
#endif
					);

					pos += kalScnprintf(buf + pos,
					u4TotalLen - pos,
					"Rx drop due to out of resource = %d\n",
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
					prMibStats->u4RxFifoFullCnt
#else
					prHwMibInfo->rHwMibCnt
					.u4RxFifoFullCnt
#endif
					);

					pos += kalScnprintf(buf + pos,
					u4TotalLen - pos,
					"Rx duplicate frame = %d\n",
					RX_GET_CNT(prRxCtrl,
					SWCR_DBG_ALL_RX_DUP_DROP_CNT));

					pos += kalScnprintf(buf + pos,
					u4TotalLen - pos,
					"False CCA(total) =%d\n",
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
					prMibStats->u4PCcaTime
#else
					prHwMibInfo->rHwMibCnt
					.u4PCcaTime
#endif
					);

					pos += kalScnprintf(buf + pos,
					u4TotalLen - pos,
					"False CCA(one-second) =%d\n",
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
					prMibStats->u4SCcaTime
#else
					prHwMibInfo->rHwMibCnt
					.u4SCcaTime
#endif
					);
				}
			}
		}

		if (kalIoctl(prGlueInfo, wlanoidQueryRssi,
				&rLinkSpeed, sizeof(rLinkSpeed),
				&u4BufLen) == WLAN_STATUS_SUCCESS) {
			i4Rssi = rLinkSpeed.rLq[ucBssIndex].cRssi;
			prStaRec = aisGetDefaultLink(prGlueInfo->prAdapter)
					->prTargetStaRec;
			if (prStaRec)
				ucAvgNoise = prStaRec->ucNoise_avg - 127;

			pos += kalScnprintf(buf + pos, u4TotalLen - pos,
				"RSSI = %d\n", i4Rssi);

			pos += kalScnprintf(buf + pos, u4TotalLen - pos,
				"P2P GO RSSI =\n");

			pos += kalScnprintf(buf + pos, u4TotalLen - pos,
				"SNR-A =\n");

			pos += kalScnprintf(buf + pos, u4TotalLen - pos,
				"SNR-B (if available) =\n");

			pos += kalScnprintf(buf + pos, u4TotalLen - pos,
				"NoiseLevel-A = %d\n", ucAvgNoise);

			pos += kalScnprintf(buf + pos, u4TotalLen - pos,
				"NoiseLevel-B =\n");
		}

		kalIoctl(prGlueInfo, wlanoidQueryLinkSpeed, &rLinkSpeed,
			sizeof(rLinkSpeed), &u4BufLen);
		u4Rate = rLinkSpeed.rLq[ucBssIndex].u2LinkSpeed;

		/* STA stats */
		if (kalIoctl(prGlueInfo, wlanoidQueryBssid,
				 &arBssid[0], sizeof(arBssid),
				 &u4BufLen) == WLAN_STATUS_SUCCESS) {

			prBssInfo =
				&(prGlueInfo->prAdapter->rWifiVar
				.arBssInfoPool[KAL_NETWORK_TYPE_AIS_INDEX]);

			pos += kalScnprintf(buf + pos, u4TotalLen - pos,
				"\n(STA) connected AP MAC Address = ");

			for (i = 0; i < PARAM_MAC_ADDR_LEN; i++) {

				pos += kalScnprintf(buf + pos,
					u4TotalLen - pos, "%02x", arBssid[i]);
				if (i != PARAM_MAC_ADDR_LEN - 1)
					pos += kalScnprintf(buf + pos,
						u4TotalLen - pos, ":");
			}
			pos += kalScnprintf(buf + pos, u4TotalLen - pos, "\n");

			pos += kalScnprintf(buf + pos, u4TotalLen - pos,
				"PhyMode:");
			switch (prBssInfo->ucPhyTypeSet) {
			case PHY_TYPE_SET_802_11B:
				pos += kalScnprintf(buf + pos, u4TotalLen - pos,
					"802.11b\n");
				break;
			case PHY_TYPE_SET_802_11ABG:
			case PHY_TYPE_SET_802_11BG:
				pos += kalScnprintf(buf + pos, u4TotalLen - pos,
					"802.11g\n");
				break;
			case PHY_TYPE_SET_802_11A:
				pos += kalScnprintf(buf + pos, u4TotalLen - pos,
					"802.11a\n");
				break;
			case PHY_TYPE_SET_802_11ABGN:
			case PHY_TYPE_SET_802_11BGN:
			case PHY_TYPE_SET_802_11AN:
			case PHY_TYPE_SET_802_11GN:
				pos += kalScnprintf(buf + pos, u4TotalLen - pos,
					"802.11n\n");
				break;
			case PHY_TYPE_SET_802_11ABGNAC:
			case PHY_TYPE_SET_802_11ANAC:
			case PHY_TYPE_SET_802_11AC:
				pos += kalScnprintf(buf + pos, u4TotalLen - pos,
					"802.11ac\n");
				break;
			default:
				break;
			}

			pos += kalScnprintf(buf + pos, u4TotalLen - pos,
				"RSSI =\n");

			pos += kalScnprintf(buf + pos, u4TotalLen - pos,
				"Last TX Rate = %d\n", u4Rate*100);


			if (prStaRec) {
				pos += kalScnprintf(buf + pos, u4TotalLen - pos,
					"Last RX Rate = %d\n",
					prStaRec->u4LastPhyRate * 100000);
			} else {
				pos += kalScnprintf(buf + pos, u4TotalLen - pos,
					"Last RX Rate =\n");
			}
		} else {
			pos += kalScnprintf(buf + pos, u4TotalLen - pos,
				"\n(STA) Not connected\n");
		}
		kalMemFree(prHwMibInfo, VIR_MEM_TYPE,
			sizeof(struct PARAM_HW_MIB_INFO));
	}
	break;
#endif
#if CFG_SUPPORT_WAKEUP_STATISTICS
	case PRIV_CMD_INT_STAT:
	{
		struct WAKEUP_STATISTIC *prWakeupSta = NULL;

		prWakeupSta = prGlueInfo->prAdapter->arWakeupStatistic;
		pos += kalScnprintf(buf + pos, u4TotalLen - pos,
				"Abnormal Interrupt:%d\n"
				"Software Interrupt:%d\n"
				"TX Interrupt:%d\n"
				"RX data:%d\n"
				"RX Event:%d\n"
				"RX mgmt:%d\n"
				"RX others:%d\n",
				prWakeupSta[0].u2Count,
				prWakeupSta[1].u2Count,
				prWakeupSta[2].u2Count,
				prWakeupSta[3].u2Count,
				prWakeupSta[4].u2Count,
				prWakeupSta[5].u2Count,
				prWakeupSta[6].u2Count);
		for (i = 0; i < EVENT_ID_END; i++) {
			if (prGlueInfo->prAdapter->wake_event_count[i] > 0)
				pos += kalScnprintf(buf + pos, u4TotalLen - pos,
						"RX EVENT(0x%0x):%d\n", i,
						prGlueInfo->prAdapter
						->wake_event_count[i]);
		}
	}
	break;
#endif
#if CFG_SUPPORT_EXCEPTION_STATISTICS
	case PRIV_CMD_EXCEPTION_STAT:
	{
		pos += kalScnprintf(buf + pos, u4TotalLen - pos,
				"TotalBeaconTimeout:%d\n",
				prGlueInfo->prAdapter
				->total_beacon_timeout_count);
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
		for (i = 0; i < UNI_ENUM_BCN_TIMEOUT_REASON_MAX_NUM; i++)
#else
		for (i = 0; i < BEACON_TIMEOUT_REASON_NUM; i++)
#endif
			if (prGlueInfo->prAdapter->beacon_timeout_count[i] > 0)
				pos += kalScnprintf(buf + pos, u4TotalLen - pos,
					"BeaconTimeout Reason(0x%0x):%d\n", i,
					prGlueInfo->prAdapter
					->beacon_timeout_count[i]);


		pos += kalScnprintf(buf + pos, u4TotalLen - pos,
				"TotalTxDoneFail:%d\n",
				prGlueInfo->prAdapter
				->total_tx_done_fail_count);
		for (i = 0; i < TX_RESULT_NUM; i++) {
			if (prGlueInfo->prAdapter->tx_done_fail_count[i] > 0)
				pos += kalScnprintf(buf + pos, u4TotalLen - pos,
					"TxDoneFail Reason(0x%0x):%d\n", i,
					prGlueInfo->prAdapter
					->tx_done_fail_count[i]);
		}
		pos += kalScnprintf(buf + pos, u4TotalLen - pos,
				"TotalRxDeauth:%d\n",
				prGlueInfo->prAdapter->total_deauth_rx_count);
		for (i = 0; i < (REASON_CODE_BEACON_TIMEOUT + 1); i++) {
			if (prGlueInfo->prAdapter->deauth_rx_count[i] > 0)
				pos += kalScnprintf(buf + pos, u4TotalLen - pos,
					"RxDeauth Reason(0x%0x):%d\n", i,
					prGlueInfo->prAdapter
					->deauth_rx_count[i]);
		}
		pos += kalScnprintf(buf + pos, u4TotalLen - pos,
				"TotalScanDoneTimeout:%d\n",
				prGlueInfo->prAdapter
				->total_scandone_timeout_count);
		pos += kalScnprintf(buf + pos, u4TotalLen - pos,
				"TotalTxMgmtTimeout:%d\n",
				prGlueInfo->prAdapter
				->total_mgmtTX_timeout_count);
		pos += kalScnprintf(buf + pos, u4TotalLen - pos,
				"TotalRxMgmtTimeout:%d\n",
				prGlueInfo->prAdapter
				->total_mgmtRX_timeout_count);
	}
	break;
#endif
	default:
		DBGLOG(REQ, WARN, "get string cmd:0x%x\n", u4SubCmd);
		break;
	}

	DBGLOG(REQ, INFO, "%s i4BytesWritten = %d\n", __func__, pos);
	if (pos > 0) {

		if (pos > 2000)
			pos = 2000;
		prIwReqData->data.length = pos;

	} else if (pos == 0) {
		prIwReqData->data.length = pos;
	}
#else
	DBGLOG(REQ, WARN, "not support in user load");
#endif
	return 0;

}


/*----------------------------------------------------------------------------*/
/*!
 * \brief Private ioctl driver handler.
 *
 * \param[in] pDev Net device requested.
 * \param[out] pIwReq Pointer to iwreq structure.
 * \param[in] cmd Private sub-command.
 *
 * \retval 0 For success.
 * \retval -EFAULT If copy from user space buffer fail.
 * \retval -EOPNOTSUPP Parameter "cmd" not recognized.
 *
 */
/*----------------------------------------------------------------------------*/
int
priv_set_driver(struct net_device *prNetDev,
		struct iw_request_info *prIwReqInfo,
		union iwreq_data *prIwReqData, char *pcExtra)
{
	uint32_t u4SubCmd = 0;
	uint16_t u2Cmd = 0;

	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;

	ASSERT(prNetDev);
	ASSERT(prIwReqData);
	if (!prNetDev || !prIwReqData) {
		DBGLOG(REQ, INFO,
		       "priv_set_driver(): invalid param(0x%p, 0x%p)\n",
		       prNetDev, prIwReqData);
		return -EINVAL;
	}

	u2Cmd = prIwReqInfo->cmd;
	DBGLOG(REQ, INFO, "prIwReqInfo->cmd %u\n", u2Cmd);

	u4SubCmd = (uint32_t) prIwReqData->data.flags;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	ASSERT(prGlueInfo);
	if (!prGlueInfo) {
		DBGLOG(REQ, INFO,
		       "priv_set_driver(): invalid prGlueInfo(0x%p, 0x%p)\n",
		       prNetDev,
		       *((struct GLUE_INFO **) netdev_priv(prNetDev)));
		return -EINVAL;
	}

	/* trick,hack in ./net/wireless/wext-priv.c ioctl_private_iw_point */
	/* because the cmd number is odd (get), the input string will not be
	 * copy_to_user
	 */

	DBGLOG(REQ, INFO, "prIwReqData->data.length %u\n",
	       prIwReqData->data.length);

	/* Use GET type becauase large data by iwpriv. */

	ASSERT(IW_IS_GET(u2Cmd));
	if (prIwReqData->data.length != 0) {
		if (!kal_access_ok(VERIFY_READ, prIwReqData->data.pointer,
			       prIwReqData->data.length)) {
			DBGLOG(REQ, INFO,
			       "%s access_ok Read fail written = %d\n",
			       __func__, i4BytesWritten);
			return -EFAULT;
		}
		if (prIwReqData->data.length >= IW_PRIV_SET_BUF_SIZE)
			return -EFAULT;
		if (copy_from_user(pcExtra, prIwReqData->data.pointer,
				   prIwReqData->data.length)) {
			DBGLOG(REQ, INFO,
			       "%s copy_form_user fail written = %d\n",
			       __func__, prIwReqData->data.length);
			return -EFAULT;
		}
		/* prIwReqData->data.length include the terminate '\0' */
		pcExtra[prIwReqData->data.length] = 0;
	}

	if (pcExtra) {
		DBGLOG(REQ, INFO, "pcExtra %s\n", pcExtra);
		/* Please check max length in rIwPrivTable */
		DBGLOG(REQ, INFO, "%s prIwReqData->data.length = %d\n",
		    __func__, prIwReqData->data.length);
		i4BytesWritten = priv_driver_cmds(prNetDev, pcExtra,
			IW_PRIV_GET_BUF_SIZE /*prIwReqData->data.length */);
		DBGLOG(REQ, INFO, "%s i4BytesWritten = %d\n", __func__,
		    i4BytesWritten);
	}

	DBGLOG(REQ, INFO, "pcExtra done\n");

	if (i4BytesWritten > 0) {

		if (i4BytesWritten > IW_PRIV_GET_BUF_SIZE)
			i4BytesWritten = IW_PRIV_GET_BUF_SIZE;
		prIwReqData->data.length =
			i4BytesWritten;	/* the iwpriv will use the length */

	} else if (i4BytesWritten == 0) {
		prIwReqData->data.length = i4BytesWritten;
	}

	return 0;

}				/* priv_set_driver */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set beacon detection function enable/disable
 *        state.
 *        This is mainly designed for usage under BT inquiry state
 *        (disable function).
 *
 * \param[in] pvAdapter Pointer to the Adapter structure
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set
 * \param[in] u4SetBufferLen The length of the set buffer
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *   bytes read from the set buffer. If the call failed due to invalid length of
 *   the set buffer, returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_DATA If new setting value is wrong.
 * \retval WLAN_STATUS_INVALID_LENGTH
 *
 */
/*----------------------------------------------------------------------------*/
static uint32_t
reqExtSetAcpiDevicePowerState(struct GLUE_INFO
			      *prGlueInfo,
			      void *pvSetBuffer, uint32_t u4SetBufferLen,
			      uint32_t *pu4SetInfoLen)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	ASSERT(prGlueInfo);
	ASSERT(pvSetBuffer);
	ASSERT(pu4SetInfoLen);

	/* WIFI is enabled, when ACPI is
	 * D0 (ParamDeviceStateD0 = 1). And vice versa
	 */

	return rStatus;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief glue layer set test mode function via switch mode for private ioctl
 *
 * \param[in] ptr  - void pointer to the GLUE_INFO structure
 * \param[in] pvSetBuffer - dont care
 * \param[in] u4SetBufferLen - dont care
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *   bytes read from the set buffer. If the call failed due to invalid length of
 *   the set buffer, returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_FAILURE
 *
 */
/*----------------------------------------------------------------------------*/
static uint32_t
glWextRfTestSetTestMode(void *ptr,
		void *pvSetBuffer,
		uint32_t u4SetBufferLen,
		uint32_t *pu4SetInfoLen)
{
	uint32_t rStatus;
	struct GLUE_INFO *prGlueInfo = NULL;

	prGlueInfo = (struct GLUE_INFO *) ptr;

	if (!prGlueInfo)
		return WLAN_STATUS_FAILURE;

	rStatus = glSetRFTestMode(prGlueInfo, 1);

	return rStatus;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief glue layer abort test mode function via switch mode for private ioctl
 *
 * \param[in] ptr  - void pointer to the GLUE_INFO structure
 * \param[in] pvSetBuffer - dont care
 * \param[in] u4SetBufferLen - dont care
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *   bytes read from the set buffer. If the call failed due to invalid length of
 *   the set buffer, returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_FAILURE
 *
 */
/*----------------------------------------------------------------------------*/
static uint32_t
glWextRfTestSetAbortTestMode(void *ptr,
		void *pvSetBuffer,
		uint32_t u4SetBufferLen,
		uint32_t *pu4SetInfoLen)
{
	uint32_t rStatus;
	struct GLUE_INFO *prGlueInfo = NULL;

	if (!pu4SetInfoLen)
		return WLAN_STATUS_FAILURE;

	*pu4SetInfoLen = 0;

	prGlueInfo = (struct GLUE_INFO *) ptr;

	if (!prGlueInfo)
		return WLAN_STATUS_FAILURE;

	rStatus = glSetRFTestMode(prGlueInfo, 0);

	return rStatus;
}

#define CMD_START		"START"
#define CMD_STOP		"STOP"
#define CMD_SCAN_ACTIVE		"SCAN-ACTIVE"
#define CMD_SCAN_PASSIVE	"SCAN-PASSIVE"
#define CMD_RXFILTER_START	"RXFILTER-START"
#define CMD_RXFILTER_STOP	"RXFILTER-STOP"
#define CMD_RXFILTER_ADD	"RXFILTER-ADD"
#define CMD_RXFILTER_REMOVE	"RXFILTER-REMOVE"
#define CMD_BTCOEXSCAN_START	"BTCOEXSCAN-START"
#define CMD_BTCOEXSCAN_STOP	"BTCOEXSCAN-STOP"
#define CMD_BTCOEXMODE		"BTCOEXMODE"
#define CMD_P2P_DEV_ADDR	"P2P_DEV_ADDR"
#define CMD_SETFWPATH		"SETFWPATH"
#define CMD_GET_STAT		"GET_STAT"
#define CMD_GET_STA_KEEP_CNT    "KEEPCOUNTER"
#define CMD_STAT_RESET_CNT      "RESETCOUNTER"
#define CMD_STAT_NOISE_SEL      "NOISESELECT"
#define CMD_STAT_GROUP_SEL      "GROUP"
#define CMD_SET_TXPOWER			"SET_TXPOWER"
#define CMD_SET_AP_WPS_P2P_IE		"SET_AP_WPS_P2P_IE"
#define CMD_SETROAMMODE			"SETROAMMODE"
#define CMD_PNODEBUG_SET	"PNODEBUG"
#define CMD_WLS_BATCHING	"WLS_BATCHING"

#define CMD_OKC_SET_PMK		"SET_PMK"
#define CMD_OKC_ENABLE		"OKC_ENABLE"

#define CMD_SETMONITOR		"MONITOR"

/* miracast related definition */
#ifndef MIRACAST_AMPDU_SIZE
#define MIRACAST_AMPDU_SIZE	8
#endif

#ifndef MIRACAST_MCHAN_ALGO
#define MIRACAST_MCHAN_ALGO     1
#endif

#ifndef MIRACAST_MCHAN_BW
#define MIRACAST_MCHAN_BW       25
#endif

/* Mediatek private command */
#define CMD_SET_DBG_LEVEL       "SET_DBG_LEVEL"
#define CMD_GET_DBG_LEVEL       "GET_DBG_LEVEL"
#define PRIV_CMD_SIZE 512

#if CFG_SUPPORT_WFD
static uint8_t g_ucMiracastMode = MIRACAST_MODE_OFF;
#endif

struct cmd_tlv {
	char prefix;
	char version;
	char subver;
	char reserved;
};

struct priv_driver_cmd_s {
	char buf[PRIV_CMD_SIZE];
	int used_len;
	int total_len;
};

#ifdef CFG_ANDROID_AOSP_PRIV_CMD
struct android_wifi_priv_cmd {
	char *buf;
	int used_len;
	int total_len;
};
#endif /* CFG_ANDROID_AOSP_PRIV_CMD */

int priv_driver_get_dbg_level(struct net_device
			      *prNetDev, char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4DbgIdx = DBG_MODULE_NUM, u4DbgMask = 0;
	u_int8_t fgIsCmdAccept = FALSE;
	int32_t u4Ret = 0;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc >= 2) {
		/* u4DbgIdx = kalStrtoul(apcArgv[1], NULL, 0); */
		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4DbgIdx);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
			       u4Ret);

		if (wlanGetDriverDbgLevel(u4DbgIdx, &u4DbgMask) ==
		    WLAN_STATUS_SUCCESS) {
			fgIsCmdAccept = TRUE;
			i4BytesWritten =
				kalSnprintf(pcCommand, i4TotalLen,
					 "Get DBG module[%u] log level => [0x%02x]!",
					 u4DbgIdx,
					 (uint8_t) u4DbgMask);
		}
	}

	if (!fgIsCmdAccept)
		i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen,
					  "Get DBG module log level failed!");

	return i4BytesWritten;

}				/* priv_driver_get_sw_ctrl */

static int priv_cmd_not_support(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen)
{
	DBGLOG(REQ, WARN, "not support priv command: %s\n", pcCommand);

	return -EOPNOTSUPP;
}

#if CFG_SUPPORT_QA_TOOL
int priv_driver_get_rx_statistics(struct net_device *prNetDev,
					 char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	int32_t u4Ret = 0;
	struct PARAM_CUSTOM_ACCESS_RX_STAT rRxStatisticsTest;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	DBGLOG(INIT, ERROR,
	       "MT6632 : priv_driver_get_rx_statistics\n");

	if (i4Argc >= 2) {
		kalMemSet(&rRxStatisticsTest, 0, sizeof(rRxStatisticsTest));
#if (CFG_SUPPORT_CONNAC3X == 0)
		u4Ret = kalkStrtou32(apcArgv[1], 0,
				     &(rRxStatisticsTest.u4SeqNum));
#else
		u4Ret = kalkStrtou16(apcArgv[1], 0,
				     &(rRxStatisticsTest.u2SeqNum));
#endif
		rRxStatisticsTest.u4TotalNum = sizeof(struct
						      PARAM_RX_STAT) / 4;

		rStatus = kalIoctl(prGlueInfo, wlanoidQueryRxStatistics,
				&rRxStatisticsTest, sizeof(rRxStatisticsTest),
				&u4BufLen);

		DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
		if (rStatus != WLAN_STATUS_SUCCESS)
			return -1;
	}

	return i4BytesWritten;
}
#endif /* CFG_SUPPORT_QA_TOOL */

#if CFG_SUPPORT_MSP
int priv_driver_get_sta_statistics(
	struct net_device *prNetDev, char *pcCommand,
	int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	int32_t i4ArgNum = 3;
	struct PARAM_GET_STA_STATISTICS rQueryStaStatistics;
	int32_t rRssi;
	uint16_t u2LinkSpeed;
	uint32_t u4Per;

	ASSERT(prNetDev);

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	kalMemZero(&rQueryStaStatistics,
		   sizeof(rQueryStaStatistics));
	rQueryStaStatistics.ucReadClear = TRUE;

	if (i4Argc >= i4ArgNum) {
		if (strnicmp(apcArgv[1], CMD_GET_STA_KEEP_CNT,
			     strlen(CMD_GET_STA_KEEP_CNT)) == 0) {
			wlanHwAddrToBin(apcArgv[2],
					&rQueryStaStatistics.aucMacAddr[0]);
			rQueryStaStatistics.ucReadClear = FALSE;
		} else if (strnicmp(apcArgv[2], CMD_GET_STA_KEEP_CNT,
				    strlen(CMD_GET_STA_KEEP_CNT)) == 0) {
			wlanHwAddrToBin(apcArgv[1],
					&rQueryStaStatistics.aucMacAddr[0]);
			rQueryStaStatistics.ucReadClear = FALSE;
		}
	} else {
		struct BSS_INFO *prAisBssInfo =   aisGetAisBssInfo(
			prGlueInfo->prAdapter, wlanGetBssIdx(prNetDev));

		/* Get AIS AP address for no argument */
		if (prAisBssInfo->prStaRecOfAP) {
			COPY_MAC_ADDR(rQueryStaStatistics.aucMacAddr,
				prAisBssInfo
				->prStaRecOfAP->aucMacAddr);
			DBGLOG(RSN, INFO, "use ais ap "MACSTR"\n",
			       MAC2STR(prAisBssInfo
			       ->prStaRecOfAP->aucMacAddr));
		} else {
			DBGLOG(RSN, INFO, "not connect to ais ap %p\n",
			       prAisBssInfo
			       ->prStaRecOfAP);
			i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen,
				"%s", "\n\nNo STA Stat:\n");
			return i4BytesWritten;
		}

		if (i4Argc == 2) {
			if (strnicmp(apcArgv[1], CMD_GET_STA_KEEP_CNT,
				     strlen(CMD_GET_STA_KEEP_CNT)) == 0)
				rQueryStaStatistics.ucReadClear = FALSE;
		}
	}

	rStatus = kalIoctl(prGlueInfo, wlanoidQueryStaStatistics,
			   &rQueryStaStatistics, sizeof(rQueryStaStatistics),
			   &u4BufLen);

	if (rStatus == WLAN_STATUS_SUCCESS) {
		rRssi = RCPI_TO_dBm(rQueryStaStatistics.ucRcpi);
		u2LinkSpeed = rQueryStaStatistics.u2LinkSpeed == 0 ? 0 :
			      rQueryStaStatistics.u2LinkSpeed / 2;

		i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen, "%s",
					     "\n\nSTA Stat:\n");

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"Tx total cnt           = %d\n",
			rQueryStaStatistics.u4TransmitCount);

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"Tx success	       = %d\n",
			rQueryStaStatistics.u4TransmitCount -
			rQueryStaStatistics.u4TransmitFailCount);

		u4Per = rQueryStaStatistics.u4TransmitCount == 0 ? 0 :
			(1000 * (rQueryStaStatistics.u4TransmitFailCount)) /
			rQueryStaStatistics.u4TransmitCount;
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"Tx fail count	       = %d, PER=%d.%d%%\n",
			rQueryStaStatistics.u4TransmitFailCount, u4Per / 10,
			u4Per % 10);

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"RSSI		       = %d\n", rRssi);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"LinkSpeed	       = %d\n", u2LinkSpeed);

	} else
		i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen, "%s",
					     "\n\nNo STA Stat:\n");

	return i4BytesWritten;

}


int priv_driver_get_bss_statistics(
	struct net_device *prNetDev, char *pcCommand,
	int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus;
	uint32_t u4BufLen;
	struct PARAM_LINK_SPEED_EX *prLinkSpeed;
	struct PARAM_GET_BSS_STATISTICS rQueryBssStatistics;
	uint8_t ucBssIndex = wlanGetBssIdx(prNetDev);
	int32_t i4BytesWritten = 0;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	ASSERT(prGlueInfo);

	if (!IS_BSS_INDEX_AIS(prGlueInfo->prAdapter, ucBssIndex))
		return WLAN_STATUS_FAILURE;

	/* 2. fill RSSI */
	if (kalGetMediaStateIndicated(prGlueInfo,
		ucBssIndex) !=
	    MEDIA_STATE_CONNECTED) {
		/* not connected */
		DBGLOG(REQ, WARN, "not yet connected\n");
		return WLAN_STATUS_SUCCESS;
	}
	rStatus = kalIoctlByBssIdx(prGlueInfo,
				   wlanoidQueryRssi,
				   &prLinkSpeed, sizeof(prLinkSpeed),
				   &u4BufLen, ucBssIndex);
	if (rStatus != WLAN_STATUS_SUCCESS)
		DBGLOG(REQ, WARN, "unable to retrieve rssi\n");


	/* 3 get per-BSS link statistics */
	if (rStatus == WLAN_STATUS_SUCCESS) {
		kalMemZero(&rQueryBssStatistics,
			   sizeof(rQueryBssStatistics));
		rQueryBssStatistics.ucBssIndex = ucBssIndex;

		rQueryBssStatistics.ucReadClear = TRUE;

		rStatus = kalIoctl(prGlueInfo, wlanoidQueryBssStatistics,
				   &rQueryBssStatistics,
				   sizeof(rQueryBssStatistics), &u4BufLen);

		if (rStatus == WLAN_STATUS_SUCCESS) {
			i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen,
				"%s", "\n\nStat:\n");
			i4BytesWritten += kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"%s", "CurrentTemperature    = -\n");
			i4BytesWritten += kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"Tx success	       = %d\n",
				rQueryBssStatistics.u4TransmitCount -
				rQueryBssStatistics.u4TransmitFailCount);
			i4BytesWritten += kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"Tx fail count	       = %d\n",
				rQueryBssStatistics.u4TransmitFailCount);
		}

	} else {
		DBGLOG(REQ, WARN,
		       "unable to retrieve per-BSS link statistics\n");
	}


	DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__,
	       pcCommand);

	return i4BytesWritten;

}

static u_int8_t priv_driver_get_sgi_info(
					struct PARAM_PEER_CAP *prWtblPeerCap)
{
	if (!prWtblPeerCap)
		return FALSE;

	switch (prWtblPeerCap->ucFrequencyCapability) {
	case BW_20:
		return prWtblPeerCap->fgG2;
	case BW_40:
		return prWtblPeerCap->fgG4;
	case BW_80:
		return prWtblPeerCap->fgG8;
	case BW_160:
		return prWtblPeerCap->fgG16;
	default:
		return FALSE;
	}
}

static u_int8_t priv_driver_get_ldpc_info(
	struct PARAM_TX_CONFIG *prWtblTxConfig)
{
	if (!prWtblTxConfig)
		return FALSE;

	if (prWtblTxConfig->fgIsVHT)
		return prWtblTxConfig->fgVhtLDPC;
	else
		return prWtblTxConfig->fgLDPC;
}

int32_t priv_driver_rate_to_string(char *pcCommand,
				   int i4TotalLen, uint8_t TxRx,
				   struct PARAM_HW_WLAN_INFO *prHwWlanInfo)
{
	uint8_t i, txmode, rate, stbc;
	uint8_t nss;
	int32_t i4BytesWritten = 0;

	for (i = 0; i < AUTO_RATE_NUM; i++) {

		txmode = HW_TX_RATE_TO_MODE(
				 prHwWlanInfo->rWtblRateInfo.au2RateCode[i]);
#if (CFG_SUPPORT_802_11BE == 0)
		if (txmode >= ENUM_TX_MODE_NUM)
			txmode = ENUM_TX_MODE_NUM - 1;
#endif
		rate = HW_TX_RATE_TO_MCS(
			       prHwWlanInfo->rWtblRateInfo.au2RateCode[i]);
		nss = HW_TX_RATE_TO_NSS(
			      prHwWlanInfo->rWtblRateInfo.au2RateCode[i]) + 1;
		stbc = HW_TX_RATE_TO_STBC(
			       prHwWlanInfo->rWtblRateInfo.au2RateCode[i]);

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
					      i4TotalLen - i4BytesWritten,
					      "\tRate index[%d] ", i);

		if (prHwWlanInfo->rWtblRateInfo.ucRateIdx == i) {
			if (TxRx == 0)
				i4BytesWritten += kalSnprintf(
					pcCommand + i4BytesWritten,
					i4TotalLen - i4BytesWritten,
					"%s", "[Last RX Rate] ");
			else
				i4BytesWritten += kalSnprintf(
					pcCommand + i4BytesWritten,
					i4TotalLen - i4BytesWritten,
					"%s", "[Last TX Rate] ");
		} else
			i4BytesWritten += kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"%s", "               ");

		if (txmode == TX_RATE_MODE_CCK)
			i4BytesWritten += kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten, "%s, ",
				rate < 4 ? HW_TX_RATE_CCK_STR[rate] :
					   HW_TX_RATE_CCK_STR[4]);
		else if (txmode == TX_RATE_MODE_OFDM)
			i4BytesWritten += kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten, "%s, ",
				nicHwRateOfdmStr(rate));
		else {
			i4BytesWritten += kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"NSS%d_MCS%d, ", nss, rate);
		}

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten, "%s, ", HW_TX_RATE_BW[
			prHwWlanInfo->rWtblPeerCap.ucFrequencyCapability]);

		if (txmode == TX_RATE_MODE_CCK)
			i4BytesWritten += kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten, "%s, ",
				rate < 4 ? "LP" : "SP");
		else if (txmode == TX_RATE_MODE_OFDM)
			;
		else
			i4BytesWritten += kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten, "%s, ",
				priv_driver_get_sgi_info(
					&prHwWlanInfo->rWtblPeerCap) == 0 ?
					"LGI" : "SGI");
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten, "%s%s %s\n",
			HW_TX_MODE_STR[txmode], stbc ? "STBC" : " ",
			priv_driver_get_ldpc_info(&prHwWlanInfo->rWtblTxConfig)
			== 0 ? "BCC" : "LDPC");
	}

	return i4BytesWritten;
}

static int32_t priv_driver_dump_helper_wtbl_info(char *pcCommand,
		int i4TotalLen, struct PARAM_HW_WLAN_INFO *prHwWlanInfo)
{
	uint8_t i;
	int32_t i4BytesWritten = 0;

	if (!pcCommand) {
		DBGLOG(HAL, ERROR, "%s: pcCommand is NULL.\n",
			__func__);
		return i4BytesWritten;
	}

	i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen, "%s",
		"\n\nwtbl:\n");
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
		i4TotalLen - i4BytesWritten,
		"Dump WTBL info of WLAN_IDX	    = %d\n",
		prHwWlanInfo->u4Index);
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
		i4TotalLen - i4BytesWritten, "\tAddr="MACSTR"\n",
		MAC2STR(prHwWlanInfo->rWtblTxConfig.aucPA));
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
		i4TotalLen - i4BytesWritten, "\tMUAR_Idx	 = %d\n",
		prHwWlanInfo->rWtblSecConfig.ucMUARIdx);
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
		i4TotalLen - i4BytesWritten, "\trc_a1/rc_a2:%d/%d\n",
		prHwWlanInfo->rWtblSecConfig.fgRCA1,
		prHwWlanInfo->rWtblSecConfig.fgRCA2);
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
		i4TotalLen - i4BytesWritten,
		"\tKID:%d/RCID:%d/RKV:%d/RV:%d/IKV:%d/WPI_FLAG:%d\n",
		prHwWlanInfo->rWtblSecConfig.ucKeyID,
		prHwWlanInfo->rWtblSecConfig.fgRCID,
		prHwWlanInfo->rWtblSecConfig.fgRKV,
		prHwWlanInfo->rWtblSecConfig.fgRV,
		prHwWlanInfo->rWtblSecConfig.fgIKV,
		prHwWlanInfo->rWtblSecConfig.fgEvenPN);
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
		i4TotalLen - i4BytesWritten, "%s", "\tGID_SU:NA");
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
		i4TotalLen - i4BytesWritten, "\tsw/DIS_RHTR:%d/%d\n",
		prHwWlanInfo->rWtblTxConfig.fgSW,
		prHwWlanInfo->rWtblTxConfig.fgDisRxHdrTran);
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
		i4TotalLen - i4BytesWritten,
		"\tHT/VHT/HT-LDPC/VHT-LDPC/DYN_BW/MMSS:%d/%d/%d/%d/%d/%d\n",
		prHwWlanInfo->rWtblTxConfig.fgIsHT,
		prHwWlanInfo->rWtblTxConfig.fgIsVHT,
		prHwWlanInfo->rWtblTxConfig.fgLDPC,
		prHwWlanInfo->rWtblTxConfig.fgVhtLDPC,
		prHwWlanInfo->rWtblTxConfig.fgDynBw,
		prHwWlanInfo->rWtblPeerCap.ucMMSS);
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
		i4TotalLen - i4BytesWritten,
		"\tFCAP/G2/G4/G8/G16/CBRN:%d/%d/%d/%d/%d/%d\n",
		prHwWlanInfo->rWtblPeerCap.ucFrequencyCapability,
		prHwWlanInfo->rWtblPeerCap.fgG2,
		prHwWlanInfo->rWtblPeerCap.fgG4,
		prHwWlanInfo->rWtblPeerCap.fgG8,
		prHwWlanInfo->rWtblPeerCap.fgG16,
		prHwWlanInfo->rWtblPeerCap.ucChangeBWAfterRateN);
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
		i4TotalLen - i4BytesWritten,
		"\tHT-TxBF(tibf/tebf):%d/%d, VHT-TxBF(tibf/tebf):%d/%d, PFMU_IDX=%d\n",
		prHwWlanInfo->rWtblTxConfig.fgTIBF,
		prHwWlanInfo->rWtblTxConfig.fgTEBF,
		prHwWlanInfo->rWtblTxConfig.fgVhtTIBF,
		prHwWlanInfo->rWtblTxConfig.fgVhtTEBF,
		prHwWlanInfo->rWtblTxConfig.ucPFMUIdx);
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
		i4TotalLen - i4BytesWritten, "%s", "\tSPE_IDX=NA\n");
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
		i4TotalLen - i4BytesWritten,
		"\tBA Enable:0x%x, BAFail Enable:%d\n",
		prHwWlanInfo->rWtblBaConfig.ucBaEn,
		prHwWlanInfo->rWtblTxConfig.fgBAFEn);
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
		i4TotalLen - i4BytesWritten,
		"\tQoS Enable:%d\n", prHwWlanInfo->rWtblTxConfig.fgIsQoS);
	if (prHwWlanInfo->rWtblTxConfig.fgIsQoS) {
		for (i = 0; i < 8; i += 2) {
			i4BytesWritten += kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"\t\tBA WinSize: TID 0 - %d, TID 1 - %d\n",
				(uint32_t)
				    ((prHwWlanInfo->rWtblBaConfig.u4BaWinSize >>
				    (i * 3)) & BITS(0, 2)),
				(uint32_t)
				    ((prHwWlanInfo->rWtblBaConfig.u4BaWinSize >>
				    ((i + 1) * 3)) & BITS(0, 2)));
		}
	}

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
		i4TotalLen - i4BytesWritten, "\tpartial_aid:%d\n",
		prHwWlanInfo->rWtblTxConfig.u2PartialAID);
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
		i4TotalLen - i4BytesWritten, "\twpi_even:%d\n",
		prHwWlanInfo->rWtblSecConfig.fgEvenPN);
	i4BytesWritten += scnprintf(pcCommand + i4BytesWritten,
		i4TotalLen - i4BytesWritten, "\tAAD_OM/CipherSuit:%d/%d\n",
		prHwWlanInfo->rWtblTxConfig.fgAADOM,
		prHwWlanInfo->rWtblSecConfig.ucCipherSuit);
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
		i4TotalLen - i4BytesWritten, "\taf:%d\n",
		prHwWlanInfo->rWtblPeerCap.ucAmpduFactor);
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
		i4TotalLen - i4BytesWritten, "\trdg_ba:%d/rdg capability:%d\n",
		prHwWlanInfo->rWtblTxConfig.fgRdgBA,
		prHwWlanInfo->rWtblTxConfig.fgRDG);
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
		i4TotalLen - i4BytesWritten, "\tcipher_suit:%d\n",
		prHwWlanInfo->rWtblSecConfig.ucCipherSuit);
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
		i4TotalLen - i4BytesWritten, "\tFromDS:%d\n",
		prHwWlanInfo->rWtblTxConfig.fgIsFromDS);
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
		i4TotalLen - i4BytesWritten, "\tToDS:%d\n",
		prHwWlanInfo->rWtblTxConfig.fgIsToDS);
#if 0
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
		i4TotalLen - i4BytesWritten,
		"\tRCPI = %d %d %d %d\n",
		prHwWlanInfo->rWtblRxCounter.ucRxRcpi0,
		prHwWlanInfo->rWtblRxCounter.ucRxRcpi1,
		prHwWlanInfo->rWtblRxCounter.ucRxRcpi2,
		prHwWlanInfo->rWtblRxCounter.ucRxRcpi3);
#endif
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
		i4TotalLen - i4BytesWritten,
		"\tRSSI = %d %d %d %d\n",
		RCPI_TO_dBm(prHwWlanInfo->rWtblRxCounter.ucRxRcpi0),
		RCPI_TO_dBm(prHwWlanInfo->rWtblRxCounter.ucRxRcpi1),
		RCPI_TO_dBm(prHwWlanInfo->rWtblRxCounter.ucRxRcpi2),
		RCPI_TO_dBm(prHwWlanInfo->rWtblRxCounter.ucRxRcpi3));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
		i4TotalLen - i4BytesWritten, "%s", "\tRate Info\n");

	i4BytesWritten += priv_driver_rate_to_string(pcCommand + i4BytesWritten,
		i4TotalLen - i4BytesWritten, 1, prHwWlanInfo);

#if 0
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen -
				      i4BytesWritten,
		"%s", "\t===Key======\n");
	for (i = 0; i < 32; i += 8) {
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"\t0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
			prHwWlanInfo->rWtblKeyConfig.aucKey[i + 0],
			prHwWlanInfo->rWtblKeyConfig.aucKey[i + 1],
			prHwWlanInfo->rWtblKeyConfig.aucKey[i + 2],
			prHwWlanInfo->rWtblKeyConfig.aucKey[i + 3],
			prHwWlanInfo->rWtblKeyConfig.aucKey[i + 4],
			prHwWlanInfo->rWtblKeyConfig.aucKey[i + 5],
			prHwWlanInfo->rWtblKeyConfig.aucKey[i + 6],
			prHwWlanInfo->rWtblKeyConfig.aucKey[i + 7]);
	}
#endif

	return i4BytesWritten;
}

static int priv_driver_get_wtbl_info_default(
	struct GLUE_INFO *prGlueInfo,
	uint32_t u4Index,
	char *pcCommand,
	int i4TotalLen)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	struct PARAM_HW_WLAN_INFO *prHwWlanInfo;

	prHwWlanInfo = (struct PARAM_HW_WLAN_INFO *)kalMemAlloc(
			sizeof(struct PARAM_HW_WLAN_INFO), VIR_MEM_TYPE);
	if (!prHwWlanInfo)
		return i4BytesWritten;

	prHwWlanInfo->u4Index = u4Index;
	DBGLOG(REQ, INFO, "%s : index = %d\n",
		__func__,
		prHwWlanInfo->u4Index);

	rStatus = kalIoctl(prGlueInfo, wlanoidQueryWlanInfo, prHwWlanInfo,
		sizeof(struct PARAM_HW_WLAN_INFO), &u4BufLen);

	DBGLOG(REQ, INFO, "rStatus %u u4BufLen = %d\n", rStatus, u4BufLen);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		kalMemFree(prHwWlanInfo, VIR_MEM_TYPE,
		sizeof(struct PARAM_HW_WLAN_INFO));
		return i4BytesWritten;
	}
	i4BytesWritten = priv_driver_dump_helper_wtbl_info(
		pcCommand,
		i4TotalLen,
		prHwWlanInfo);

	DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__, pcCommand);

	kalMemFree(prHwWlanInfo, VIR_MEM_TYPE,
		   sizeof(struct PARAM_HW_WLAN_INFO));

	return i4BytesWritten;
}

int priv_driver_get_wtbl_info(struct net_device *prNetDev,
				     char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4Argc = 0;
	int32_t u4Ret = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Index = 0;
	struct CHIP_DBG_OPS *prDbgOps;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc >= 2)
		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Index);

	if (u4Ret)
		return -1;

	prDbgOps = prGlueInfo->prAdapter->chip_info->prDebugOps;

	if (prDbgOps->showWtblInfo)
		return prDbgOps->showWtblInfo(
			prGlueInfo->prAdapter, u4Index, pcCommand, i4TotalLen);
	else
		return priv_driver_get_wtbl_info_default(
			prGlueInfo, u4Index, pcCommand, i4TotalLen);
}

int priv_driver_get_sta_info(struct net_device *prNetDev,
				    char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint8_t aucMacAddr[MAC_ADDR_LEN] = {0};
	uint8_t ucWlanIndex = 0;
	uint8_t *pucMacAddr = NULL;
	struct PARAM_HW_WLAN_INFO *prHwWlanInfo;
	struct PARAM_GET_STA_STATISTICS rQueryStaStatistics;
	int32_t rRssi;
	uint16_t u2LinkSpeed;
	uint32_t u4Per;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	kalMemZero(&rQueryStaStatistics, sizeof(rQueryStaStatistics));
	rQueryStaStatistics.ucReadClear = TRUE;

	/* DBGLOG(RSN, INFO, "MT6632 : priv_driver_get_sta_info\n"); */
	if (i4Argc >= 3) {
		if (strnicmp(apcArgv[1], CMD_GET_STA_KEEP_CNT,
		    strlen(CMD_GET_STA_KEEP_CNT)) == 0) {
			wlanHwAddrToBin(apcArgv[2], &aucMacAddr[0]);
			rQueryStaStatistics.ucReadClear = FALSE;
		} else if (strnicmp(apcArgv[2], CMD_GET_STA_KEEP_CNT,
		    strlen(CMD_GET_STA_KEEP_CNT)) == 0) {
			wlanHwAddrToBin(apcArgv[1], &aucMacAddr[0]);
			rQueryStaStatistics.ucReadClear = FALSE;
		}

		if (!wlanGetWlanIdxByAddress(prGlueInfo->prAdapter,
		    &aucMacAddr[0], &ucWlanIndex))
			return i4BytesWritten;
	} else {
		struct BSS_INFO *prAisBssInfo =   aisGetAisBssInfo(
			prGlueInfo->prAdapter, wlanGetBssIdx(prNetDev));

		/* Get AIS AP address for no argument */
		if (prAisBssInfo->prStaRecOfAP)
			ucWlanIndex = prAisBssInfo
					->prStaRecOfAP->ucWlanIndex;
		else if (!wlanGetWlanIdxByAddress(prGlueInfo->prAdapter, NULL,
		    &ucWlanIndex)) /* try get a peer */
			return i4BytesWritten;

		if (i4Argc == 2) {
			if (strnicmp(apcArgv[1], CMD_GET_STA_KEEP_CNT,
			    strlen(CMD_GET_STA_KEEP_CNT)) == 0)
				rQueryStaStatistics.ucReadClear = FALSE;
		}
	}

	prHwWlanInfo = (struct PARAM_HW_WLAN_INFO *)kalMemAlloc(
			sizeof(struct PARAM_HW_WLAN_INFO), VIR_MEM_TYPE);
	if (prHwWlanInfo == NULL) {
		DBGLOG(REQ, INFO, "prHwWlanInfo is null\n");
		return -1;
	}

	prHwWlanInfo->u4Index = ucWlanIndex;

	DBGLOG(REQ, INFO, "MT6632 : index = %d i4TotalLen = %d\n",
	       prHwWlanInfo->u4Index, i4TotalLen);

	rStatus = kalIoctl(prGlueInfo, wlanoidQueryWlanInfo, prHwWlanInfo,
			   sizeof(struct PARAM_HW_WLAN_INFO), &u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		kalMemFree(prHwWlanInfo, VIR_MEM_TYPE,
			   sizeof(struct PARAM_HW_WLAN_INFO));
		return -1;
	}

	i4BytesWritten = priv_driver_dump_helper_wtbl_info(pcCommand,
						i4TotalLen, prHwWlanInfo);

	pucMacAddr = wlanGetStaAddrByWlanIdx(prGlueInfo->prAdapter,
					     ucWlanIndex);
	if (pucMacAddr) {
		COPY_MAC_ADDR(rQueryStaStatistics.aucMacAddr, pucMacAddr);
		/* i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
		 *      i4TotalLen - i4BytesWritten,
		 *      "\tAddr="MACSTR"\n",
		 *      MAC2STR(rQueryStaStatistics.aucMacAddr));
		 */

		rStatus = kalIoctl(prGlueInfo, wlanoidQueryStaStatistics,
				   &rQueryStaStatistics,
				   sizeof(rQueryStaStatistics), &u4BufLen);

		if (rStatus == WLAN_STATUS_SUCCESS) {
			rRssi = RCPI_TO_dBm(rQueryStaStatistics.ucRcpi);
			u2LinkSpeed = rQueryStaStatistics.u2LinkSpeed == 0 ?
				0 : rQueryStaStatistics.u2LinkSpeed/2;

			i4BytesWritten += kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"%s", "\n\nSTA Stat:\n");

			i4BytesWritten += kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"Tx total cnt           = %d\n",
				rQueryStaStatistics.u4TransmitCount);

			i4BytesWritten += kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"Tx success	       = %d\n",
				rQueryStaStatistics.u4TransmitCount -
				rQueryStaStatistics.u4TransmitFailCount);

			u4Per = rQueryStaStatistics.u4TransmitCount == 0 ? 0 :
				(1000 *
				(rQueryStaStatistics.u4TransmitFailCount)) /
				rQueryStaStatistics.u4TransmitCount;
			i4BytesWritten += kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"Tx fail count	       = %d, PER=%d.%1d%%\n",
				rQueryStaStatistics.u4TransmitFailCount,
				u4Per/10, u4Per%10);

			i4BytesWritten += kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"RSSI		       = %d\n", rRssi);
			i4BytesWritten += kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"LinkSpeed	       = %d\n", u2LinkSpeed);
		}
	}
	DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__, pcCommand);

	kalMemFree(prHwWlanInfo, VIR_MEM_TYPE,
		   sizeof(struct PARAM_HW_WLAN_INFO));

	return i4BytesWritten;
}

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
static int priv_driver_get_mib_info_uni(struct ADAPTER *prAdapter,
	uint32_t u4BandIdx,
	char *pcCommand,
	int32_t i4TotalLen)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	struct PARAM_HW_MIB_INFO *prParamMibInfo;
	uint8_t idx;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	struct RX_CTRL *prRxCtrl;
	struct MIB_STATS *prMibStats = &prAdapter->rMibStats[u4BandIdx];
	uint64_t u4TxAmpdu, u4TxAmpduMpdu, u4TxAmpduAcked;
	uint64_t per;
	uint32_t per_rem;

	_Static_assert(MAX_UNI_CMD_MIB_NUM >= UNI_CMD_MIB_CNT_MAX_NUM,
		"MAX_UNI_CMD_MIB_NUM should >= UNI_CMD_MIB_CNT_MAX_NUM");

	DBGLOG(REQ, INFO, "Band index = %d\n", u4BandIdx);

	prRxCtrl = &prAdapter->rRxCtrl;

	/* make sure prParamMibInfo->au4TagBitmap is enough
	 * to present all mib idx
	 */
	prParamMibInfo = (struct PARAM_HW_MIB_INFO *)kalMemAlloc(
		sizeof(struct PARAM_HW_MIB_INFO), VIR_MEM_TYPE);
	if (!prParamMibInfo)
		return -1;

	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
		i4BytesWritten,
		"Dump MIB info of IDX         = %d\n",
		u4BandIdx);

	kalMemZero(prParamMibInfo, sizeof(struct PARAM_HW_MIB_INFO));

	prParamMibInfo->u2Index = (uint16_t)u4BandIdx;
	prParamMibInfo->u2TagCount = (
		(UNI_CMD_MIB_CNT_BCN_TX -
			UNI_CMD_MIB_CNT_LEN_MISMATCH + 1) +
		(UNI_CMD_MIB_CNT_TX_BW_160MHZ -
			UNI_CMD_MIB_CNT_TX_BW_40MHZ + 1) +
		(UNI_CMD_MIB_CNT_BSS3_FRAME_RETRY_2 -
			UNI_CMD_MIB_CNT_BSS0_RTS_TX_CNT + 1) +
		((UNI_CMD_MIB_CNT_BSS0_TX_DATA + 2) -
			UNI_CMD_MIB_CNT_TX_BW_320MHZ + 1) +
		(UNI_CMD_MIB_CNT_AMPDU_ACKED -
			(UNI_CMD_MIB_CNT_RX_BYTE_MBSS0 - 1) + 1)
	);

	/* 16~21, 23~25 */
	SET_BITMAP_RANGE(prParamMibInfo->au4TagBitmap,
		UNI_CMD_MIB_CNT_LEN_MISMATCH,
		UNI_CMD_MIB_CNT_BCN_TX);
	SET_BITMAP_RANGE(prParamMibInfo->au4TagBitmap,
		UNI_CMD_MIB_CNT_TX_BW_40MHZ,
		UNI_CMD_MIB_CNT_TX_BW_160MHZ);
	/* 57~80 */
	SET_BITMAP_RANGE(prParamMibInfo->au4TagBitmap,
		UNI_CMD_MIB_CNT_BSS0_RTS_TX_CNT,
		UNI_CMD_MIB_CNT_BSS3_FRAME_RETRY_2);
	/* 111~127 */
	SET_BITMAP_RANGE(prParamMibInfo->au4TagBitmap,
		UNI_CMD_MIB_CNT_TX_BW_320MHZ,
		(UNI_CMD_MIB_CNT_BSS0_TX_DATA + 2));
	/* 192~211 */
	SET_BITMAP_RANGE(prParamMibInfo->au4TagBitmap,
		(UNI_CMD_MIB_CNT_RX_BYTE_MBSS0 - 1),
		UNI_CMD_MIB_CNT_AMPDU_ACKED);

	rStatus = kalIoctl(prAdapter->prGlueInfo, wlanoidQueryMibInfo,
		prParamMibInfo, sizeof(struct PARAM_HW_MIB_INFO), &u4BufLen);

	DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		kalMemFree(prParamMibInfo, VIR_MEM_TYPE,
			   sizeof(struct PARAM_HW_MIB_INFO));
		return -1;
	}

	kalMemZero(prParamMibInfo, sizeof(struct PARAM_HW_MIB_INFO));

	prParamMibInfo->u2Index = (uint16_t)u4BandIdx;
	prParamMibInfo->u2TagCount = (
		(UNI_CMD_MIB_CNT_AMDPU_RX_COUNT -
			UNI_CMD_MIB_CNT_RX_FCS_ERR + 1) +
		1 /* UNI_CMD_MIB_CNT_CHANNEL_IDLE */ +
		(UNI_CMD_MIB_CNT_RX_OFDM_GREEN_MDRDY_TIME -
			UNI_CMD_MIB_CNT_VEC_MISMATCH + 1) +
		((UNI_CMD_MIB_CNT_RX_BYTE_MBSS0 - 2) -
			(UNI_CMD_MIB_CNT_BSS0_TX_DATA + 3) + 1)
	);

	/* 0~3, 7, 10~14 */
	SET_BITMAP_RANGE(prParamMibInfo->au4TagBitmap,
		UNI_CMD_MIB_CNT_RX_FCS_ERR,
		UNI_CMD_MIB_CNT_AMDPU_RX_COUNT);
	SET_BITMAP(prParamMibInfo->au4TagBitmap,
		UNI_CMD_MIB_CNT_CHANNEL_IDLE);
	SET_BITMAP_RANGE(prParamMibInfo->au4TagBitmap,
		UNI_CMD_MIB_CNT_VEC_MISMATCH,
		UNI_CMD_MIB_CNT_RX_OFDM_GREEN_MDRDY_TIME);

	/* 128 ~ 191 */
	SET_BITMAP_RANGE(prParamMibInfo->au4TagBitmap,
		(UNI_CMD_MIB_CNT_BSS0_TX_DATA + 3),
		(UNI_CMD_MIB_CNT_RX_BYTE_MBSS0 - 2));

	rStatus = kalIoctl(prAdapter->prGlueInfo, wlanoidQueryMibInfo,
		prParamMibInfo, sizeof(struct PARAM_HW_MIB_INFO), &u4BufLen);

	DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		kalMemFree(prParamMibInfo, VIR_MEM_TYPE,
			   sizeof(struct PARAM_HW_MIB_INFO));
		return -1;
	}

	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"%s", "===Rx Related Counters===\n");
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tRx with CRC=%llu\n", prMibStats->u4RxFcsErrCnt);
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tRx drop due to out of resource=%llu\n",
		prMibStats->u4RxFifoFullCnt);
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tRx Mpdu=%llu\n", prMibStats->u4RxMpduCnt);

	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tRx AMpdu=%llu\n", prMibStats->u4RxAMPDUCnt);
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tRx Vec Q Mismatch=%llu\n",
		prMibStats->u4RxVectorMismatchCnt);
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tRx Len Mismatch=%llu\n",
		prMibStats->u4RxLenMismatchCnt);

	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tRx data indicate total=%llu\n",
		RX_GET_CNT(prRxCtrl, RX_DATA_INDICATION_COUNT));
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tRx data retain total=%llu\n",
		RX_GET_CNT(prRxCtrl, RX_DATA_RETAINED_COUNT));
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tRx drop by SW total=%llu\n",
		RX_GET_CNT(prRxCtrl, RX_DROP_TOTAL_COUNT));
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tRx reorder miss=%llu\n",
		RX_GET_CNT(prRxCtrl, RX_DATA_REORDER_MISS_COUNT));
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tRx reorder within=%llu\n",
		RX_GET_CNT(prRxCtrl, RX_DATA_REORDER_WITHIN_COUNT));
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tRx reorder ahead=%llu\n",
		RX_GET_CNT(prRxCtrl, RX_DATA_REORDER_AHEAD_COUNT));
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tRx reorder behind=%llu\n",
		RX_GET_CNT(prRxCtrl, RX_DATA_REORDER_BEHIND_COUNT));

	do {
		uint32_t u4AmsduCntx100 = 0;

		if (RX_GET_CNT(prRxCtrl, RX_DATA_AMSDU_COUNT))
			u4AmsduCntx100 = (uint32_t)div64_u64(
				RX_GET_CNT(prRxCtrl,
					RX_DATA_MSDU_IN_AMSDU_COUNT) * 100,
				RX_GET_CNT(prRxCtrl, RX_DATA_AMSDU_COUNT));
		i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
			i4BytesWritten,
			"\tRx avg MSDU in AMSDU=%1d.%02d\n",
			u4AmsduCntx100 / 100, u4AmsduCntx100 % 100);

	} while (FALSE);

	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tRx total MSDU in AMSDU=%llu\n",
		RX_GET_CNT(prRxCtrl, RX_DATA_MSDU_IN_AMSDU_COUNT));
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tRx AMSDU=%llu\n",
		RX_GET_CNT(prRxCtrl, RX_DATA_AMSDU_COUNT));
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tRx AMSDU miss=%llu\n",
		RX_GET_CNT(prRxCtrl, RX_DATA_AMSDU_MISS_COUNT));
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tRx no StaRec drop=%llu\n",
		RX_GET_CNT(prRxCtrl, RX_NO_STA_DROP_COUNT));
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tRx inactive BSS drop=%llu\n",
		RX_GET_CNT(prRxCtrl, RX_INACTIVE_BSS_DROP_COUNT));
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tRx HS20 drop=%llu\n",
		RX_GET_CNT(prRxCtrl, RX_HS20_DROP_COUNT));
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tRx low SwRfb drop=%llu\n",
		RX_GET_CNT(prRxCtrl, RX_LESS_SW_RFB_DROP_COUNT));
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tRx dupicate drop=%llu\n",
		RX_GET_CNT(prRxCtrl, RX_DUPICATE_DROP_COUNT));
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tRx MIC err drop=%llu\n",
		RX_GET_CNT(prRxCtrl, RX_MIC_ERROR_DROP_COUNT));
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tRx BAR handle=%llu\n",
		RX_GET_CNT(prRxCtrl, RX_BAR_DROP_COUNT));
#if CFG_SUPPORT_BAR_DELAY_INDICATION
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tRx BAR delayed=%llu\n",
		RX_GET_CNT(prRxCtrl, RX_BAR_DELAY_COUNT));
#endif /* CFG_SUPPORT_BAR_DELAY_INDICATION */
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tRx non-interest drop=%llu\n",
		RX_GET_CNT(prRxCtrl, RX_NO_INTEREST_DROP_COUNT));
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tRx type err drop=%llu\n",
		RX_GET_CNT(prRxCtrl, RX_TYPE_ERR_DROP_COUNT));
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tRx pointer err drop=%llu\n",
		RX_GET_CNT(prRxCtrl, RX_POINTER_ERR_DROP_COUNT));
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tRx class err drop=%llu\n",
		RX_GET_CNT(prRxCtrl, RX_CLASS_ERR_DROP_COUNT));

#if 0
#if defined(BELLWETHER) || defined(MT7990)
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
	"%s", "===MU TX Counters===\n");
	HAL_MCR_RD(prAdapter, BN0_WF_MIB_TOP_BSCR2_ADDR + band_offset,
	&mu_cnt[0]);
	HAL_MCR_RD(prAdapter, BN0_WF_MIB_TOP_TSCR5_ADDR + band_offset,
	&mu_cnt[1]);
	HAL_MCR_RD(prAdapter, BN0_WF_MIB_TOP_TSCR6_ADDR + band_offset,
	&mu_cnt[2]);
	HAL_MCR_RD(prAdapter, BN0_WF_MIB_TOP_TSCR8_ADDR + band_offset,
	&mu_cnt[3]);
	HAL_MCR_RD(prAdapter, BN0_WF_MIB_TOP_TSCR7_ADDR + band_offset,
	&mu_cnt[4]);

	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
	"\tMUBF=%u, MuToMuFail=%u (PPDU)\n",
	mu_cnt[0] & BN0_WF_MIB_TOP_BSCR2_MUBF_TX_COUNT_MASK,
	mu_cnt[3]);
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
	"\tMuTotal=%u, MuOK=%u, SU_OK=%u (MPDU)\n",
	mu_cnt[1],
	mu_cnt[2],
	mu_cnt[4]);
#endif
#endif
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"%s", "===Phy/Timing Related Counters===\n");
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tChannelIdleCnt=%llu\n",
		prMibStats->u4ChannelIdleCnt);
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tCCA_NAV_Tx_Time=%llu\n",
		prMibStats->u4CcaNavTx);
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tRx_MDRDY_CNT=%llu\n",
		prMibStats->u4MdrdyCnt);
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tMdrdyTime CCK=%llu, OFDM=0x%lx, OFDM_GREEN=0x%lx\n",
		prMibStats->u4CCKMdrdyCnt,
		prMibStats->u4OFDMLGMixMdrdy,
		prMibStats->u4OFDMGreenMdrdy);
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tPrim CCA Time=%llu\n",
		prMibStats->u4PCcaTime);
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tSec CCA Time=%llu\n",
		prMibStats->u4SCcaTime);
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tPrim ED Time=%llu\n",
		prMibStats->u4PEDTime);

	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"%s", "===Tx Related Counters(Generic)===\n");
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tBeaconTxCnt=%llu\n",
		prMibStats->u4BeaconTxCnt);
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tTx 40MHz Cnt=%llu\n",
		prMibStats->u4Tx40MHzCnt);
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tTx 80MHz Cnt=%llu\n",
		prMibStats->u4Tx80MHzCnt);
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tTx 160MHz Cnt=%llu\n",
		prMibStats->u4Tx160MHzCnt);
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tTx 320MHz Cnt=%llu\n",
		prMibStats->u4Tx320MHzCnt);

	u4TxAmpdu = prMibStats->u4TxAmpdu;
	u4TxAmpduMpdu = prMibStats->u4TxAmpduMpdu;
	u4TxAmpduAcked = prMibStats->u4TxAmpduAcked;
	per = (u4TxAmpduMpdu == 0 ?
		0 : 1000 * (u4TxAmpduMpdu - u4TxAmpduAcked) / u4TxAmpduMpdu);
	per_rem = do_div(per, 10);

	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tAMPDU[Cnt:MpduCnt:MpduAckCnt:MpduPER]=[%lu:%lu:%lu:%lu.%1u%%]\n",
		u4TxAmpdu, u4TxAmpduMpdu, u4TxAmpduAcked,
		per, per_rem);

	for (idx = 0; idx < BSSID_NUM; idx++) {
		i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
			i4BytesWritten,
			"===BSSID[%d] Related Counters===\n", idx);
		i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
			i4BytesWritten,
			"\tBA Miss Cnt=%llu\n",
			prMibStats->au4BaMissedCnt[idx]);
		i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
			i4BytesWritten,
			"\tRTS Tx Cnt=%llu\n",
			prMibStats->au4RtsTxCnt[idx]);
		i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
			i4BytesWritten,
			"\tFrame Retry Cnt=%llu\n",
			prMibStats->au4FrameRetryCnt[idx]);
		i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
			i4BytesWritten,
			"\tFrame Retry >2 Cnt=%llu\n",
			prMibStats->au4FrameRetry2Cnt[idx]);
		i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
			i4BytesWritten,
			"\tFrame Retry >3 Cnt=%llu\n",
			prMibStats->au4FrameRetry3Cnt[idx]);
		i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
			i4BytesWritten,
			"\tRTS Retry Cnt=%llu\n",
			prMibStats->au4RtsRetryCnt[idx]);
		i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
			i4BytesWritten,
			"\tAck Failed Cnt=%llu\n",
			prMibStats->au4AckFailedCnt[idx]);
		i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
			i4BytesWritten,
			"\tTxOk=%llu, TxData=%llu, TxByte=%llu\n",
			prMibStats->au4TxCnt[idx],
			prMibStats->au4TxData[idx],
			prMibStats->au4TxByte[idx]);
		i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
			i4BytesWritten,
			"\tRxOk=%llu, RxData=%llu, RxByte=%llu\n",
			prMibStats->au4RxOk[idx],
			prMibStats->au4RxData[idx],
			prMibStats->au4RxByte[idx]);
	}

	/* Dummy delimiter insertion result */
	DBGLOG(HAL, INFO, "===Dummy delimiter insertion result===\n");
	DBGLOG(HAL, INFO, "\tRange[0:1:2:3:4]=[%llu:%llu:%llu:%llu:%llu]\n",
		prMibStats->au4TxDdlmtRng[0],
		prMibStats->au4TxDdlmtRng[1],
		prMibStats->au4TxDdlmtRng[2],
		prMibStats->au4TxDdlmtRng[3],
		prMibStats->au4TxDdlmtRng[4]);

	/* Per-MBSS T/RX Counters */
	DBGLOG(HAL, INFO, "===MBSSID Related Counters===\n");
	for (idx = 0; idx < 16; idx++) {
		DBGLOG(HAL, INFO,
			"\tID[%d] TxOk=%llu TxByte=%llu RxOk=%llu RxByte=%llu\n",
			idx, prMibStats->au4MbssTxOk[idx],
			prMibStats->au4MbssTxByte[idx],
			prMibStats->au4MbssRxOk[idx],
			prMibStats->au4MbssRxByte[idx]);
	}

	kalMemFree(prParamMibInfo, VIR_MEM_TYPE,
		sizeof(struct PARAM_HW_MIB_INFO));

	return i4BytesWritten;
}

#else
static int priv_driver_get_mib_info_default(struct ADAPTER *prAdapter,
	uint32_t u4BandIdx,
	char *pcCommand,
	int32_t i4TotalLen)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	struct PARAM_HW_MIB_INFO *prHwMibInfo;
	uint8_t i;
	uint32_t u4Per;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	struct RX_CTRL *prRxCtrl;

	DBGLOG(REQ, INFO, "Band index = %d\n", u4BandIdx);

	prRxCtrl = &prAdapter->rRxCtrl;

	prHwMibInfo = (struct PARAM_HW_MIB_INFO *)kalMemAlloc(
		sizeof(struct PARAM_HW_MIB_INFO), VIR_MEM_TYPE);
	if (!prHwMibInfo)
		return -1;

	prHwMibInfo->u4Index = u4BandIdx;

	rStatus = kalIoctl(prAdapter->prGlueInfo, wlanoidQueryMibInfo, prHwMibInfo,
			   sizeof(struct PARAM_HW_MIB_INFO), &u4BufLen);

	DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		kalMemFree(prHwMibInfo, VIR_MEM_TYPE,
			   sizeof(struct PARAM_HW_MIB_INFO));
		return -1;
	}

	if (prHwMibInfo->u4Index < 2) {
		i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen, "%s",
			"\n\nmib state:\n");
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"Dump MIB info of IDX         = %d\n",
			prHwMibInfo->u4Index);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%s", "===Rx Related Counters===\n");
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten, "\tRx with CRC=%d\n",
			prHwMibInfo->rHwMibCnt.u4RxFcsErrCnt);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"\tRx drop due to out of resource=%d\n",
			prHwMibInfo->rHwMibCnt.u4RxFifoFullCnt);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten, "\tRx Mpdu=%d\n",
			prHwMibInfo->rHwMibCnt.u4RxMpduCnt);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten, "\tRx AMpdu=%d\n",
			prHwMibInfo->rHwMibCnt.u4RxAMPDUCnt);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten, "\tRx PF Drop=%d\n",
			prHwMibInfo->rHwMibCnt.u4PFDropCnt);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"\tRx Len Mismatch=%d\n",
			prHwMibInfo->rHwMibCnt.u4RxLenMismatchCnt);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"\tRx data indicate total=%llu\n",
			RX_GET_CNT(prRxCtrl, RX_DATA_INDICATION_COUNT));
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"\tRx data retain total=%llu\n",
			RX_GET_CNT(prRxCtrl, RX_DATA_RETAINED_COUNT));
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"\tRx drop by SW total=%llu\n",
			RX_GET_CNT(prRxCtrl, RX_DROP_TOTAL_COUNT));
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten, "\tRx reorder miss=%llu\n",
			RX_GET_CNT(prRxCtrl, RX_DATA_REORDER_MISS_COUNT));
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"\tRx reorder within=%llu\n",
			RX_GET_CNT(prRxCtrl, RX_DATA_REORDER_WITHIN_COUNT));
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"\tRx reorder ahead=%llu\n",
			RX_GET_CNT(prRxCtrl, RX_DATA_REORDER_AHEAD_COUNT));
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"\tRx reorder behind=%llu\n", RX_GET_CNT(prRxCtrl,
			RX_DATA_REORDER_BEHIND_COUNT));

		do {
			uint32_t u4AmsduCntx100 = 0;

			if (RX_GET_CNT(prRxCtrl, RX_DATA_AMSDU_COUNT))
				u4AmsduCntx100 =
					(uint32_t)div64_u64(RX_GET_CNT(prRxCtrl,
					    RX_DATA_MSDU_IN_AMSDU_COUNT) * 100,
					    RX_GET_CNT(prRxCtrl,
					    RX_DATA_AMSDU_COUNT));

			i4BytesWritten += kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"\tRx avg MSDU in AMSDU=%1d.%02d\n",
				u4AmsduCntx100 / 100, u4AmsduCntx100 % 100);
		} while (FALSE);

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"\tRx total MSDU in AMSDU=%llu\n", RX_GET_CNT(prRxCtrl,
			RX_DATA_MSDU_IN_AMSDU_COUNT));
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"\tRx AMSDU=%llu\n", RX_GET_CNT(prRxCtrl,
			RX_DATA_AMSDU_COUNT));
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten, "\tRx AMSDU miss=%llu\n",
			RX_GET_CNT(prRxCtrl, RX_DATA_AMSDU_MISS_COUNT));
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"\tRx no StaRec drop=%llu\n",
			RX_GET_CNT(prRxCtrl, RX_NO_STA_DROP_COUNT));
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"\tRx inactive BSS drop=%llu\n",
			RX_GET_CNT(prRxCtrl, RX_INACTIVE_BSS_DROP_COUNT));
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"\tRx HS20 drop=%llu\n",
			RX_GET_CNT(prRxCtrl, RX_HS20_DROP_COUNT));
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"\tRx low SwRfb drop=%llu\n", RX_GET_CNT(prRxCtrl,
			RX_LESS_SW_RFB_DROP_COUNT));
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"\tRx dupicate drop=%llu\n",
			RX_GET_CNT(prRxCtrl, RX_DUPICATE_DROP_COUNT));
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten, "\tRx MIC err drop=%llu\n",
			RX_GET_CNT(prRxCtrl, RX_MIC_ERROR_DROP_COUNT));
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten, "\tRx BAR handle=%llu\n",
			RX_GET_CNT(prRxCtrl, RX_BAR_DROP_COUNT));
#if CFG_SUPPORT_BAR_DELAY_INDICATION
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten, "\tRx BAR delayed=%llu\n",
			RX_GET_CNT(prRxCtrl, RX_BAR_DELAY_COUNT));
#endif /* CFG_SUPPORT_BAR_DELAY_INDICATION */
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"\tRx non-interest drop=%llu\n", RX_GET_CNT(prRxCtrl,
			RX_NO_INTEREST_DROP_COUNT));
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"\tRx type err drop=%llu\n",
			RX_GET_CNT(prRxCtrl, RX_TYPE_ERR_DROP_COUNT));
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"\tRx pointer err drop=%llu\n",
			RX_GET_CNT(prRxCtrl, RX_POINTER_ERR_DROP_COUNT));
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"\tRx class err drop=%llu\n", RX_GET_CNT(prRxCtrl,
			RX_CLASS_ERR_DROP_COUNT));
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%s", "===Phy/Timing Related Counters===\n");
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten, "\tChannelIdleCnt=%d\n",
			prHwMibInfo->rHwMibCnt.u4ChannelIdleCnt);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten, "\tCCA_NAV_Tx_Time=%d\n",
			prHwMibInfo->rHwMibCnt.u4CcaNavTx);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten, "\tRx_MDRDY_CNT=%d\n",
			prHwMibInfo->rHwMibCnt.u4MdrdyCnt);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"\tCCK_MDRDY=%d, OFDM_MDRDY=0x%x, OFDM_GREEN_MDRDY=0x%x\n",
			prHwMibInfo->rHwMibCnt.u4CCKMdrdyCnt,
			prHwMibInfo->rHwMibCnt.u4OFDMLGMixMdrdy,
			prHwMibInfo->rHwMibCnt.u4OFDMGreenMdrdy);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten, "\tPrim CCA Time=%d\n",
			prHwMibInfo->rHwMibCnt.u4PCcaTime);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten, "\tSec CCA Time=%d\n",
			prHwMibInfo->rHwMibCnt.u4SCcaTime);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten, "\tPrim ED Time=%d\n",
			prHwMibInfo->rHwMibCnt.u4PEDTime);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten, "%s",
			"===Tx Related Counters(Generic)===\n");
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten, "\tBeaconTxCnt=%d\n",
			prHwMibInfo->rHwMibCnt.u4BeaconTxCnt);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten, "\tTx 40MHz Cnt=%d\n",
			prHwMibInfo->rHwMib2Cnt.u4Tx40MHzCnt);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten, "\tTx 80MHz Cnt=%d\n",
			prHwMibInfo->rHwMib2Cnt.u4Tx80MHzCnt);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten, "\tTx 160MHz Cnt=%d\n",
			prHwMibInfo->rHwMib2Cnt.u4Tx160MHzCnt);
		for (i = 0; i < BSSID_NUM; i++) {
			i4BytesWritten += kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"\t===BSSID[%d] Related Counters===\n", i);
			i4BytesWritten += kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"\tBA Miss Cnt=%d\n",
				prHwMibInfo->rHwMibCnt.au4BaMissedCnt[i]);
			i4BytesWritten += kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"\tRTS Tx Cnt=%d\n",
				prHwMibInfo->rHwMibCnt.au4RtsTxCnt[i]);
			i4BytesWritten += kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"\tFrame Retry Cnt=%d\n",
				prHwMibInfo->rHwMibCnt.au4FrameRetryCnt[i]);
			i4BytesWritten += kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"\tFrame Retry 2 Cnt=%d\n",
				prHwMibInfo->rHwMibCnt.au4FrameRetry2Cnt[i]);
			i4BytesWritten += kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"\tRTS Retry Cnt=%d\n",
				prHwMibInfo->rHwMibCnt.au4RtsRetryCnt[i]);
			i4BytesWritten += kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"\tAck Failed Cnt=%d\n",
				prHwMibInfo->rHwMibCnt.au4AckFailedCnt[i]);
		}

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%s", "===AMPDU Related Counters===\n");
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten, "\tTx AMPDU_Pkt_Cnt=%d\n",
			prHwMibInfo->rHwTxAmpduMts.u2TxAmpduCnt);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"\tTx AMPDU_MPDU_Pkt_Cnt=%d\n",
			prHwMibInfo->rHwTxAmpduMts.u4TxSfCnt);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten, "\tAMPDU SuccessCnt=%d\n",
			prHwMibInfo->rHwTxAmpduMts.u4TxAckSfCnt);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"\tAMPDU Tx success      = %d\n",
			prHwMibInfo->rHwTxAmpduMts.u4TxAckSfCnt);

		u4Per = prHwMibInfo->rHwTxAmpduMts.u4TxSfCnt == 0 ? 0 :
			(1000 * (prHwMibInfo->rHwTxAmpduMts.u4TxSfCnt -
			prHwMibInfo->rHwTxAmpduMts.u4TxAckSfCnt)) /
			prHwMibInfo->rHwTxAmpduMts.u4TxSfCnt;
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"\tAMPDU Tx fail count   = %d, PER=%d.%1d%%\n",
			prHwMibInfo->rHwTxAmpduMts.u4TxSfCnt -
			prHwMibInfo->rHwTxAmpduMts.u4TxAckSfCnt,
			u4Per/10, u4Per%10);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten, "%s", "\tTx Agg\n");
#if (CFG_SUPPORT_802_11AX == 1)
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%s", "\tRange:  1    2~9   10~18    19~27   ");
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%s", "28~36    37~45    46~54    55~78\n");
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"\t\t%d \t%d \t%d \t%d \t%d \t%d \t%d \t%d\n",
			prHwMibInfo->rHwTxAmpduMts.u2TxRange1AmpduCnt,
			prHwMibInfo->rHwTxAmpduMts.u2TxRange2AmpduCnt,
			prHwMibInfo->rHwTxAmpduMts.u2TxRange3AmpduCnt,
			prHwMibInfo->rHwTxAmpduMts.u2TxRange4AmpduCnt,
			prHwMibInfo->rHwTxAmpduMts.u2TxRange5AmpduCnt,
			prHwMibInfo->rHwTxAmpduMts.u2TxRange6AmpduCnt,
			prHwMibInfo->rHwTxAmpduMts.u2TxRange7AmpduCnt,
			prHwMibInfo->rHwTxAmpduMts.u2TxRange8AmpduCnt);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%s", "\tRange: 79~102 103~126 127~150 151~174 ");
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%s", "174~198 199~222 223~246 247~255\n");
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"\t\t%d \t%d \t%d \t%d \t%d \t%d \t%d \t%d\n",
			prHwMibInfo->rHwTxAmpduMts.u2TxRange9AmpduCnt,
			prHwMibInfo->rHwTxAmpduMts.u2TxRange10AmpduCnt,
			prHwMibInfo->rHwTxAmpduMts.u2TxRange11AmpduCnt,
			prHwMibInfo->rHwTxAmpduMts.u2TxRange12AmpduCnt,
			prHwMibInfo->rHwTxAmpduMts.u2TxRange13AmpduCnt,
			prHwMibInfo->rHwTxAmpduMts.u2TxRange14AmpduCnt,
			prHwMibInfo->rHwTxAmpduMts.u2TxRange15AmpduCnt,
			prHwMibInfo->rHwTxAmpduMts.u2TxRange16AmpduCnt);
#else
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten, "%s",
			"\tRange:  1    2~5   6~15    16~22   23~33    34~49    50~57    58~64\n"
			);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"\t\t%d \t%d \t%d \t%d \t%d \t%d \t%d \t%d\n",
			prHwMibInfo->rHwTxAmpduMts.u2TxRange1AmpduCnt,
			prHwMibInfo->rHwTxAmpduMts.u2TxRange2AmpduCnt,
			prHwMibInfo->rHwTxAmpduMts.u2TxRange3AmpduCnt,
			prHwMibInfo->rHwTxAmpduMts.u2TxRange4AmpduCnt,
			prHwMibInfo->rHwTxAmpduMts.u2TxRange5AmpduCnt,
			prHwMibInfo->rHwTxAmpduMts.u2TxRange6AmpduCnt,
			prHwMibInfo->rHwTxAmpduMts.u2TxRange7AmpduCnt,
			prHwMibInfo->rHwTxAmpduMts.u2TxRange8AmpduCnt);
#endif
	} else
		i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen, "%s",
					     "\nClear All Statistics\n");

	DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__, pcCommand);

	kalMemFree(prHwMibInfo, VIR_MEM_TYPE, sizeof(struct PARAM_HW_MIB_INFO));

	nicRxClearStatistics(prAdapter);

	return i4BytesWritten;
}
#endif

int priv_driver_get_mib_info(struct net_device *prNetDev,
				    char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	struct CHIP_DBG_OPS *prDbgOps;
	int32_t u4Ret = 0;
	uint32_t u4BandIdx = 0;

	DBGLOG(REQ, INFO, "\n");

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prDbgOps = prGlueInfo->prAdapter->chip_info->prDebugOps;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc == 1)
		u4BandIdx = 0;
	else if (i4Argc >= 2)
		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4BandIdx);

	if (u4BandIdx >= ENUM_BAND_NUM)
		return -1;

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	return priv_driver_get_mib_info_uni(
		prGlueInfo->prAdapter, u4BandIdx, pcCommand, i4TotalLen);
#else
	return priv_driver_get_mib_info_default(
		prGlueInfo->prAdapter, u4BandIdx, pcCommand, i4TotalLen);
#endif
}

int priv_driver_set_fw_log(struct net_device *prNetDev,
				  char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4McuDest = 0;
	uint32_t u4LogType = 0;
	struct CMD_FW_LOG_2_HOST_CTRL *prFwLog2HostCtrl;
	uint32_t u4Ret = 0;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(RSN, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	DBGLOG(RSN, INFO, "MT6632 : priv_driver_set_fw_log\n");

	prFwLog2HostCtrl = (struct CMD_FW_LOG_2_HOST_CTRL *)kalMemAlloc(
			sizeof(struct CMD_FW_LOG_2_HOST_CTRL), VIR_MEM_TYPE);
	if (!prFwLog2HostCtrl)
		return -1;

	if (i4Argc == 3) {
		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4McuDest);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse u4McuDest error u4Ret=%d\n",
			       u4Ret);

		u4Ret = kalkStrtou32(apcArgv[2], 0, &u4LogType);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse u4LogType error u4Ret=%d\n",
			       u4Ret);

		prFwLog2HostCtrl->ucMcuDest = (uint8_t)u4McuDest;
		prFwLog2HostCtrl->ucFwLog2HostCtrl = (uint8_t)u4LogType;

		rStatus = kalIoctl(prGlueInfo, wlanoidSetFwLog2Host,
				   prFwLog2HostCtrl,
				   sizeof(struct CMD_FW_LOG_2_HOST_CTRL),
				   &u4BufLen);

		DBGLOG(REQ, INFO, "%s: command result is %s (%d %d)\n",
		       __func__, pcCommand, u4McuDest, u4LogType);
		DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);

		if (rStatus != WLAN_STATUS_SUCCESS) {
			kalMemFree(prFwLog2HostCtrl, VIR_MEM_TYPE,
				   sizeof(struct CMD_FW_LOG_2_HOST_CTRL));
			return -1;
		}
	} else {
		DBGLOG(REQ, ERROR, "argc %i is not equal to 3 or 4\n", i4Argc);
		i4BytesWritten = -1;
	}

	kalMemFree(prFwLog2HostCtrl, VIR_MEM_TYPE,
		   sizeof(struct CMD_FW_LOG_2_HOST_CTRL));
	return i4BytesWritten;
}
#endif

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
int priv_driver_phy_ctrl(struct net_device *prNetDev,
			       char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0, u4Parse = 0;
	uint8_t uParse = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret;
	struct UNI_CMD_PHY_CTRL_LIST_DUMP *prPhyCtrlList;
	struct UNI_CMD_PHY_LIST_DUMP_CR rDumpList = {0};

	DBGLOG(REQ, INFO, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc != 4)
		return 0;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	prPhyCtrlList = kalMemAlloc(
		(sizeof(struct UNI_CMD_PHY_CTRL_LIST_DUMP) +
		sizeof(struct UNI_CMD_PHY_LIST_DUMP_CR)),
		VIR_MEM_TYPE);

	if (!prPhyCtrlList)
		return 0;

	u4Ret = kalkStrtou8(apcArgv[1], 0, &uParse);
	prPhyCtrlList->ucAction = uParse;
	u4Ret = kalkStrtou8(apcArgv[2], 0, &uParse);
	rDumpList.u2Tag = (uint16_t)uParse;
	u4Ret = kalkStrtou32(apcArgv[3], 0, &u4Parse);
	rDumpList.u4Stamp = u4Parse;

	memcpy(prPhyCtrlList->aucTlvBuffer, &rDumpList,
		sizeof(struct UNI_CMD_PHY_LIST_DUMP_CR));

	DBGLOG(REQ, TRACE, "ucAction[%d]u2Tag[%d]u4Stamp[%d]\n",
		prPhyCtrlList->ucAction,
		rDumpList.u2Tag,
		rDumpList.u4Stamp);

	rStatus = kalIoctl(prGlueInfo, wlanoidSetPhyCtrl,
				prPhyCtrlList,
				(sizeof(struct UNI_CMD_PHY_CTRL_LIST_DUMP) +
				sizeof(struct UNI_CMD_PHY_LIST_DUMP_CR)),
				&u4BufLen);

	kalMemFree(prPhyCtrlList, VIR_MEM_TYPE,
			(sizeof(struct UNI_CMD_PHY_CTRL_LIST_DUMP) +
			sizeof(struct UNI_CMD_PHY_LIST_DUMP_CR)));

	return 1;

}				/* priv_driver_get_phycr */
#endif

int priv_driver_get_mcr(struct net_device *prNetDev,
			       char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret;
	int32_t i4ArgNum = 2;
	struct CMD_ACCESS_REG rCmdAccessReg;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc >= i4ArgNum) {
		rCmdAccessReg.u4Address = 0;
		u4Ret = kalkStrtou32(apcArgv[1], 0, &(rCmdAccessReg.u4Address));
		if (u4Ret)
			DBGLOG(REQ, LOUD,
			       "parse get_mcr error (Address) u4Ret=%d\n",
			       u4Ret);

		/* rCmdAccessReg.u4Address = kalStrtoul(apcArgv[1], NULL, 0); */
		rCmdAccessReg.u4Data = 0;

		DBGLOG(REQ, LOUD, "address is %x\n", rCmdAccessReg.u4Address);

		rStatus = kalIoctl(prGlueInfo, wlanoidQueryMcrRead,
				   &rCmdAccessReg, sizeof(rCmdAccessReg),
				   &u4BufLen);

		DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
		if (rStatus != WLAN_STATUS_SUCCESS)
			return -1;

		i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen, "0x%08x",
					  (unsigned int)rCmdAccessReg.u4Data);
		DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__,
		       pcCommand);
	}

	return i4BytesWritten;

}				/* priv_driver_get_mcr */

#if (CFG_SUPPORT_TSF_SYNC == 1)
int priv_driver_get_tsf_value(
	struct net_device *prNetDev,
	char *pcCommand,
	int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	int32_t i4BytesWritten = 0;
	uint32_t u4BufLen = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	uint32_t status = 0;
	struct CMD_TSF_SYNC rTsfSync;
	uint8_t ucBssIdx = 0;

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc != 2) {
		DBGLOG(REQ, ERROR, "argc(%d) is error\n", i4Argc);
		return -1;
	}

	status = kalkStrtou8(apcArgv[1], 0, &ucBssIdx);
	if (status) {
		DBGLOG(REQ, ERROR, "parse ucBssIdx error u4Ret=%d\n", status);
		return -1;
	}

	if (ucBssIdx >= MAX_BSSID_NUM) {
		DBGLOG(REQ, ERROR, "invalid bss index %d\n", ucBssIdx);
		return -1;
	}

	/* get the tsf value and to pull the tsf gpio */
	kalMemZero(&rTsfSync, sizeof(struct CMD_TSF_SYNC));
	rTsfSync.ucCmdVer = 0;
	rTsfSync.u2CmdLen = sizeof(struct CMD_TSF_SYNC);
	rTsfSync.fgIsLatch = 1;
	rTsfSync.ucBssIndex = ucBssIdx;
	rStatus = kalIoctl(prGlueInfo, wlanoidLatchTSF, &rTsfSync,
		sizeof(rTsfSync), &u4BufLen);
	if (rStatus != WLAN_STATUS_SUCCESS)
		i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen, "Fail");
	else {
		DBGLOG(REQ, INFO, "tsfvaluse: %llu\n", rTsfSync.u8TsfValue);
		i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen, "%llu",
			rTsfSync.u8TsfValue);
	}

	/* get the tsf value and to release the tsf gpio */
	kalMemZero(&rTsfSync, sizeof(struct CMD_TSF_SYNC));
	rTsfSync.ucCmdVer = 0;
	rTsfSync.u2CmdLen = sizeof(struct CMD_TSF_SYNC);
	rTsfSync.fgIsLatch = 0;
	rStatus = kalIoctl(prGlueInfo, wlanoidLatchTSF, &rTsfSync,
		sizeof(rTsfSync), &u4BufLen);
	if (rStatus != WLAN_STATUS_SUCCESS)
		i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen, "Fail");
	else
		DBGLOG(REQ, INFO, "tsfvaluse: %llu\n", rTsfSync.u8TsfValue);

	return i4BytesWritten;
}
#endif

int priv_driver_set_mcr(struct net_device *prNetDev, char *pcCommand,
			int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	uint32_t u4Ret;
	int32_t i4ArgNum = 3;
	struct CMD_ACCESS_REG rCmdAccessReg;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc >= i4ArgNum) {
		u4Ret = kalkStrtou32(apcArgv[1], 0, &(rCmdAccessReg.u4Address));
		if (u4Ret)
			DBGLOG(REQ, LOUD,
			       "parse get_mcr error (Address) u4Ret=%d\n",
			       u4Ret);

		u4Ret = kalkStrtou32(apcArgv[2], 0, &(rCmdAccessReg.u4Data));
		if (u4Ret)
			DBGLOG(REQ, LOUD,
			       "parse get_mcr error (Data) u4Ret=%d\n", u4Ret);

		/* rCmdAccessReg.u4Address = kalStrtoul(apcArgv[1], NULL, 0); */
		/* rCmdAccessReg.u4Data = kalStrtoul(apcArgv[2], NULL, 0); */

		rStatus = kalIoctl(prGlueInfo, wlanoidSetMcrWrite,
				   &rCmdAccessReg, sizeof(rCmdAccessReg),
				   &u4BufLen);

		if (rStatus != WLAN_STATUS_SUCCESS)
			return -1;

	}

	return i4BytesWritten;

}

#if CFG_SUPPORT_WIFI_POWER_METRICS
int priv_driver_set_pwr_met(struct net_device *prNetDev, char *pcCommand,
			int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	uint32_t u4Ret;
	int32_t i4ArgNum = 3;
	struct CMD_POWER_METRICS_INFO_T rCmdPowerMetrics;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc >= i4ArgNum) {
		u4Ret =
			kalkStrtou32(apcArgv[1], 0,
				&(rCmdPowerMetrics.u4Enable));
		if (u4Ret)
			DBGLOG(REQ, LOUD,
			       "parse pwr_met error (Enable) u4Ret=%d\n",
			       u4Ret);

		u4Ret =
			kalkStrtou32(apcArgv[2], 0,
				&(rCmdPowerMetrics.u4Value));
		if (u4Ret)
			DBGLOG(REQ, LOUD,
			       "parse pwr_met error (Value) u4Ret=%d\n", u4Ret);

		rStatus = kalIoctl(prGlueInfo, wlanoidSetPowerMetrics,
				   &rCmdPowerMetrics, sizeof(rCmdPowerMetrics),
				   &u4BufLen);

		if (rStatus != WLAN_STATUS_SUCCESS)
			return -1;

	}

	return i4BytesWritten;
}
#endif

int priv_driver_set_atxop(struct net_device *prNetDev, char *pcCommand,
			int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	int i4Ret;
	uint32_t u4Cmd;
	uint32_t au4Param[32] = {0};
	struct CMD_ATXOP_CFG rCmdATXOPCfg;

	if ((prNetDev == NULL) || (pcCommand == NULL))
		return -1;

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	i4Ret = kalkStrtou32(apcArgv[1], 0, &u4Cmd);
	if (i4Ret) {
		DBGLOG(REQ, LOUD,
		       "parse get_mcr error (Address) i4Ret=%d\n", i4Ret);
		rStatus = WLAN_STATUS_FAILURE;
		goto err;
	}

	i4Ret = kalkStrtou32(apcArgv[2], 0, &au4Param[0]);
	if (i4Ret) {
		DBGLOG(REQ, LOUD,
			"parse get_mcr error (Data) i4Ret=%d\n", i4Ret);
		rStatus = WLAN_STATUS_FAILURE;
		goto err;
	}

	i4Ret = kalkStrtou32(apcArgv[3], 0, &au4Param[1]);
	if (i4Ret) {
		DBGLOG(REQ, LOUD,
			"parse get_mcr error (Data) i4Ret=%d\n", i4Ret);
		rStatus = WLAN_STATUS_FAILURE;
		goto err;
	}

	i4Ret = kalkStrtou32(apcArgv[4], 0, &au4Param[2]);
	if (i4Ret) {
		DBGLOG(REQ, LOUD,
			"parse get_mcr error (Data) i4Ret=%d\n", i4Ret);
		rStatus = WLAN_STATUS_FAILURE;
		goto err;
	}

	rCmdATXOPCfg.u4Cmd = u4Cmd;

	memcpy(&rCmdATXOPCfg.au4Param[0], &au4Param[0],
		sizeof(uint32_t)*MAX_ATXOP_PARAM_NUM);

	rStatus = kalIoctl(prGlueInfo, wlanoidSetATXOP,
		   &rCmdATXOPCfg, sizeof(rCmdATXOPCfg),
			   &u4BufLen);
err:
	if (rStatus != WLAN_STATUS_SUCCESS)
		return -1;

	return i4BytesWritten;
}

int priv_driver_set_mdvt(struct net_device *prNetDev, char *pcCommand,
			int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	uint32_t u4Ret;
	int32_t i4ArgNum = 4;
	struct CMD_MDVT_CFG rCmdMdvtCfg;
#if (CFG_SUPPORT_WIFI_SYSDVT == 1)
	uint16_t u2ModuleNum;
	uint16_t u2ShowCmdIdList = 0xFF;  /* 0xFF=255 */
#endif

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc >= i4ArgNum) {
		u4Ret = kalkStrtou32(apcArgv[1], 0, &(rCmdMdvtCfg.u4ModuleId));
		if (u4Ret)
			DBGLOG(REQ, LOUD,
			       "parse u4ModuleId error u4Ret=%d\n",
			       u4Ret);

		u4Ret = kalkStrtou32(apcArgv[2], 0, &(rCmdMdvtCfg.u4CaseId));
		if (u4Ret)
			DBGLOG(REQ, LOUD,
			       "parse u4CaseId error u4Ret=%d\n", u4Ret);

		u4Ret = kalkStrtou8(apcArgv[3], 0, &(rCmdMdvtCfg.ucCapId));
		if (u4Ret)
			DBGLOG(REQ, LOUD,
			       "parse ucCapId error u4Ret=%d\n", u4Ret);

#if (CFG_SUPPORT_WIFI_SYSDVT == 1)
		if (rCmdMdvtCfg.u4ModuleId == MDVT_MODULE_PH_TPUT)
			dvtSetupPhTput(prNetDev, rCmdMdvtCfg.u4CaseId);

		if (rCmdMdvtCfg.u4ModuleId == u2ShowCmdIdList) {
			for (u2ModuleNum = 0;
				u2ModuleNum < u4MdvtTableSize;
				u2ModuleNum++) {
				DBGLOG(REQ, INFO, "Module Name %s = %d\n",
				arMdvtModuleTable[u2ModuleNum].pucParserStr,
				arMdvtModuleTable[u2ModuleNum].eModuleId);
			}
		} else
#endif
		{
			rStatus = kalIoctl(prGlueInfo, wlanoidSetMdvt,
				   &rCmdMdvtCfg, sizeof(rCmdMdvtCfg),
				   &u4BufLen);

			if (rStatus != WLAN_STATUS_SUCCESS)
				return -1;
		}
	}

	return i4BytesWritten;
}

#if (CFG_SUPPORT_802_11BE_MLO == 1)
int priv_driver_dump_mld(struct net_device *prNetDev,
	char *pcCommand,
	int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4Argc = 0;
	int32_t u4Ret = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Index = MLD_GROUP_NONE;

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc >= 2)
		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Index);

	if (u4Ret)
		return -1;

	return mldDump(prGlueInfo->prAdapter, u4Index, pcCommand, i4TotalLen);
}

int priv_driver_dump_mld_bss(struct net_device *prNetDev,
	char *pcCommand,
	int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	if (!prGlueInfo->u4ReadyFlag)
		return i4BytesWritten;

	mldBssDump(prGlueInfo->prAdapter);

	return i4BytesWritten;
}

int priv_driver_dump_mld_sta(struct net_device *prNetDev,
	char *pcCommand,
	int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	if (!prGlueInfo->u4ReadyFlag)
		return i4BytesWritten;

	mldStarecDump(prGlueInfo->prAdapter);

	return i4BytesWritten;
}
int priv_driver_dump_eml(struct net_device *prNetDev,
	char *pcCommand,
	int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus;
	struct PARAM_EML_DEBUG_INFO rQueryEmlInfo;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	uint8_t ucLinkIdx = 0;

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	if (!prGlueInfo->u4ReadyFlag)
		return i4BytesWritten;

	kalMemZero(&rQueryEmlInfo, sizeof(rQueryEmlInfo));

	rStatus = kalIoctl(prGlueInfo, wlanoidQueryEmlInfo,
		&rQueryEmlInfo, sizeof(struct PARAM_EML_DEBUG_INFO),
		&u4BufLen);
	if (rStatus == WLAN_STATUS_SUCCESS) {
		i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen, "%s",
					     "\n\nEML Info:\n");

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"Transition Timeout(ms)	= %d\n",
			rQueryEmlInfo.u4TimeoutMs);

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"MLD STAREC Index	= %d\n",
			rQueryEmlInfo.u2StaRecMldIdx);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"EMLSR bitmap		= 0x%x\n",
			rQueryEmlInfo.ucEmlsrBitMap);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"Current State		= %d\n",
			rQueryEmlInfo.ucCurrentState);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"EML Negotiated		= %d\n",
			rQueryEmlInfo.ucEmlNegotiated);
		for (ucLinkIdx = 0; ucLinkIdx < MLD_LINK_MAX; ucLinkIdx++) {
			i4BytesWritten += kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"LinkId to Band[%d/%d]\n",
				ucLinkIdx,
				rQueryEmlInfo.auMldLinkIdx[ucLinkIdx]);
		}

	} else
		i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen, "%s",
					     "\n\nNo EML Info:\n");
	return i4BytesWritten;
}

#endif

int priv_driver_get_bainfo(struct net_device *prNetDev,
	char *pcCommand,
	int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	uint8_t ucBaIdx = 0;
	struct ADAPTER *prAdapter = NULL;
	struct QUE_MGT *prQM = NULL;

	ASSERT(prNetDev);

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;
	prQM = &prAdapter->rQM;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);

	i4BytesWritten += kalSnprintf(
		pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		(prQM->ucRxBaCount ?
		"\n---------- Valid BA entry ----------\n" :
		"\n---------- NO valid BA entry ----------\n"));

	for (ucBaIdx = 0; ucBaIdx < CFG_NUM_OF_RX_BA_AGREEMENTS; ucBaIdx++) {
		struct RX_BA_ENTRY *prRxBaEntry = &prQM->arRxBaTable[ucBaIdx];

		if (prRxBaEntry && prRxBaEntry->fgIsValid) {
			i4BytesWritten += kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"WlanIdx[%d] TID[%d]\n",
				secGetWlanIdxByStaIdx(prAdapter,
				prRxBaEntry->ucStaRecIdx),
				prRxBaEntry->ucTid);
			i4BytesWritten += kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"\t- Window start[%d]\n",
				prRxBaEntry->u2WinStart);
			i4BytesWritten += kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"\t- Window end[%d]\n",
				prRxBaEntry->u2WinEnd);
			i4BytesWritten += kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"\t- Window size[%d]\n",
				prRxBaEntry->u2WinSize);
			i4BytesWritten += kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"\t- BAR SSN[%d]\n",
				prRxBaEntry->u2BarSSN);
		}
	}
	return i4BytesWritten;
}

#if (CFG_SUPPORT_802_11BE_MLO == 1)
int priv_driver_preset_linkid(struct net_device *prNetDev,
	char *pcCommand,
	int i4TotalLen)
{
	uint32_t u4Param = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	int32_t i4BytesWritten = -1;
	uint32_t u4SetInfoLen = 0;
	uint32_t rStatus;
	struct GLUE_INFO *prGlueInfo = NULL;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(INIT, TRACE, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc >= 2) {
		DBGLOG(REQ, TRACE, "argc is %i, %s\n", i4Argc,
		       apcArgv[1]);
		i4BytesWritten = kalkStrtou32(apcArgv[1], 0, &u4Param);
		if (i4BytesWritten) {
			DBGLOG(REQ, ERROR, "parse u4Param error %d\n",
			       i4BytesWritten);
			i4BytesWritten = -1;
		} else {
			DBGLOG(INIT, TRACE, "set link id %d\n", u4Param);
			rStatus = kalIoctl(prGlueInfo,
				wlanoidPresetLinkId,
				&u4Param, sizeof(uint32_t),
				&u4SetInfoLen);

			if (rStatus != WLAN_STATUS_SUCCESS) {
				i4BytesWritten = -1;
				DBGLOG(INIT, ERROR,
				       "set link id fail 0x%x\n", rStatus);
			} else {
				DBGLOG(INIT, TRACE,
				       "set link id successed\n");
			}
		}
	} else {
		DBGLOG(REQ, ERROR, "set link id failed\n");
	}

	return i4BytesWritten;
}

int priv_driver_set_ml_probereq(struct net_device *prNetDev,
	char *pcCommand,
	int i4TotalLen)
{
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	int32_t i4Argc = 0;
	int32_t i4BytesWritten = -1;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	struct PARAM_SCAN_REQUEST_ADV *prScanRequest;
	uint8_t aucMacAddr[MAC_ADDR_LEN] = {0};
	uint32_t u4BufLen, rStatus, u4Freq, u4PerSta, u4MldId;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	DBGLOG(INIT, INFO, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	/* check if there is any pending scan/sched_scan not yet finished */
	if (prGlueInfo->prScanRequest != NULL) {
		DBGLOG(REQ, ERROR, "prGlueInfo->prScanRequest != NULL\n");
		return -EINVAL;
	}

	if (i4Argc >= 5) {
		struct BSS_DESC *prBssDesc;
		uint32_t u4SsidNum = 0;
		struct PARAM_SSID rSsid;

		DBGLOG(REQ, INFO, "argc %i, cmd [%s]\n", i4Argc, apcArgv[1]);
		wlanHwAddrToBin(apcArgv[1], &aucMacAddr[0]);

		i4BytesWritten = kalkStrtou32(apcArgv[2], 0, &u4Freq);
		if (i4BytesWritten) {
			DBGLOG(REQ, ERROR, "parse ucType error %d\n",
					i4BytesWritten);
			return -1;
		}
		i4BytesWritten = kalkStrtou32(apcArgv[3], 0, &u4PerSta);
		if (i4BytesWritten) {
			DBGLOG(REQ, ERROR, "parse ucType error %d\n",
					i4BytesWritten);
			return -1;
		}
		i4BytesWritten = kalkStrtou32(apcArgv[4], 0, &u4MldId);
		if (i4BytesWritten) {
			DBGLOG(REQ, ERROR, "parse ucType error %d\n",
					i4BytesWritten);
			return -1;
		}

		if (i4Argc >= 6) {
			u4SsidNum = 1;
			COPY_SSID(rSsid.aucSsid, rSsid.u4SsidLen,
				  apcArgv[5], strlen(apcArgv[5]));
		}

		prBssDesc = scanSearchBssDescByBssid(prAdapter, aucMacAddr);
		if (!prBssDesc) {
			DBGLOG(REQ, ERROR, "Can't find "MACSTR"\n",
					MAC2STR(aucMacAddr));
			return -1;
		}

		prScanRequest =	kalMemAlloc(
			sizeof(struct PARAM_SCAN_REQUEST_ADV), VIR_MEM_TYPE);

		if (prScanRequest == NULL) {
			DBGLOG(REQ, ERROR, "alloc scan request fail\n");
			return -ENOMEM;
		}
		kalMemZero(prScanRequest,
			   sizeof(struct PARAM_SCAN_REQUEST_ADV));
		prScanRequest->ucScanType = SCAN_TYPE_ACTIVE_SCAN;
		prScanRequest->u4IELength = mldFillScanIE(prAdapter,
			prBssDesc, prScanRequest->aucIEBuf,
			sizeof(prScanRequest->aucIEBuf), u4PerSta, u4MldId);
		prScanRequest->arChannel[0].ucChannelNum =
					nicFreq2ChannelNum(u4Freq * 1000);
		prScanRequest->ucBssidMatchCh[0] =
					nicFreq2ChannelNum(u4Freq * 1000);
		if (u4Freq >= 2412 && u4Freq <= 2484)
			prScanRequest->arChannel[0].eBand = BAND_2G4;
		else if (u4Freq >= 5180 && u4Freq <= 5900)
			prScanRequest->arChannel[0].eBand = BAND_5G;
#if (CFG_SUPPORT_WIFI_6G == 1)
		else if (u4Freq >= 5955 && u4Freq <= 7115)
			prScanRequest->arChannel[0].eBand = BAND_6G;
#endif
		prScanRequest->u4ChannelNum = 1;
		prScanRequest->ucBssIndex = 0;
		prScanRequest->ucScnFuncMask |= ENUM_SCN_USE_PADDING_AS_BSSID;
		prScanRequest->u4ScnFuncMaskExtend |= ENUM_SCN_ML_PROBE;
		COPY_MAC_ADDR(prScanRequest->aucExtBssid[0], aucMacAddr);
		kalMemSet(prScanRequest->ucBssidMatchSsidInd,
				CFG_SCAN_OOB_MAX_NUM,
				sizeof(prScanRequest->ucBssidMatchSsidInd));
		if (u4SsidNum) {
			prScanRequest->u4SsidNum = 1;
			kalMemCopy(prScanRequest->rSsid, &rSsid, sizeof(rSsid));
			prScanRequest->ucSSIDType =
				SCAN_REQ_SSID_SPECIFIED_ONLY;
			/* notify fw that bssid 0 match SSID 0 */
			prScanRequest->ucBssidMatchSsidInd[0] = 0;
		}

		rStatus = kalIoctl(prGlueInfo, wlanoidSetBssidListScanAdv,
			prScanRequest, sizeof(struct PARAM_SCAN_REQUEST_ADV),
			&u4BufLen);
		if (rStatus != WLAN_STATUS_SUCCESS) {
			DBGLOG(INIT, ERROR, "fail 0x%x\n", rStatus);
			return -1;
		}
	} else {
		DBGLOG(REQ, ERROR, "fail invalid data\n");
	}
	return i4BytesWritten;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set p2p GO mlo link number except
 *        the main link.
 *
 * \param[in] pcCommand Store ucMldBssLinkNum, which means the remaining
 *                      mlo link number except the main link.
 *
 */
/*----------------------------------------------------------------------------*/
int priv_driver_set_ml_bss_num(struct net_device *prNetDev,
	char *pcCommand,
	int i4TotalLen)
{
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	int32_t i4Argc = 0;
	int32_t i4Ret = -1;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	struct BSS_INFO *prBssInfo = NULL;
	struct MLD_BSS_INFO *prMldBssInfo = NULL;
	uint8_t ucBssIdx = 0;
	uint8_t ucRemainMldBssLinkNum;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	ucBssIdx = wlanGetBssIdx(prNetDev);
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);
	prMldBssInfo = mldBssGetByBss(prAdapter, prBssInfo);

	DBGLOG(INIT, INFO, "command is %s\n", pcCommand);

	if (i4Argc == 2) {
		DBGLOG(REQ, TRACE, "argc is %i, %s\n", i4Argc, apcArgv[1]);
		i4Ret = kalkStrtou8(apcArgv[1], 0, &ucRemainMldBssLinkNum);
		if (i4Ret) {
			DBGLOG(REQ, ERROR, "parse u4Param error %d\n", i4Ret);
			i4Ret = -1;
		} else {
			gucRemainMldBssLinkNum = ucRemainMldBssLinkNum;
			if (!mldIsMultiLinkEnabled(prAdapter,
				NETWORK_TYPE_P2P, FALSE) ||
			    !mldBssAllowReconfig(prAdapter, prMldBssInfo))
				gucRemainMldBssLinkNum = 0;
		}
	} else {
		DBGLOG(REQ, ERROR, "invalid data\n");
	}

	return i4Ret;
}

int priv_driver_get_ml_capa(struct net_device *prNetDev,
	char *pcCommand,
	int i4TotalLen)
{
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	int32_t i4Argc = 0;
	int32_t i4BytesWritten = -1;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAd = NULL;
	struct BSS_INFO *prBssInfo = NULL;
	struct MLD_BSS_INFO *prMldBssInfo = NULL;
	uint8_t ucBssIdx = 0;
	uint8_t ucCapa = 0;
	uint32_t u4Param = 0;

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAd = prGlueInfo->prAdapter;
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	ucBssIdx = wlanGetBssIdx(prNetDev);
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAd, ucBssIdx);
	prMldBssInfo = mldBssGetByBss(prAd, prBssInfo);

	DBGLOG(REQ, INFO, "command is %s\n", pcCommand);

	if (i4Argc >= 2) {
		DBGLOG(REQ, TRACE, "argc is %i, %s\n", i4Argc,
		       apcArgv[1]);
		i4BytesWritten = kalkStrtou32(apcArgv[1], 0, &u4Param);
		if (i4BytesWritten) {
			DBGLOG(REQ, ERROR, "parse u4Param error %d\n",
			       i4BytesWritten);
			i4BytesWritten = -1;
		} else {
			/* u4Param: 1: p2p, 0: AP */
			if (mldIsMultiLinkEnabled(prAd,
				NETWORK_TYPE_P2P, !u4Param) &&
			    mldBssAllowReconfig(prAd, prMldBssInfo))
				ucCapa = 1;

			i4BytesWritten = kalSnprintf(
				pcCommand, i4TotalLen, "%d", ucCapa);

			DBGLOG(REQ, INFO, "command result is %s\n", pcCommand);
		}
	} else {
		DBGLOG(REQ, ERROR, "get ml cap failed\n");
	}

	return i4BytesWritten;
}

#if CFG_ENABLE_WIFI_DIRECT
int priv_driver_get_ml_2nd_freq(struct net_device *prNetDev,
	char *pcCommand,
	int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	int32_t i4Argc = 0;
	int32_t i4BytesWritten = 0;
	uint32_t u4PreferFreq = 0, u4MainLinkFreq = 0, u4PeerFreq = 0;
	enum ENUM_BAND eMainLinkBand;
	u_int8_t fgIsApMode = FALSE;

	DBGLOG(REQ, TRACE, "[%s] command is %s\n", prNetDev->name, pcCommand);

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;
	if (!prAdapter)
		return -1;

	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	if (i4Argc < 2) {
		DBGLOG(REQ, LOUD, "Incorrect argc %i\n", i4Argc);
		return -1;
	}

	if (kalkStrtou32(apcArgv[1], 0, &u4MainLinkFreq) ||
	    kalkStrtou32(apcArgv[2], 0, &u4PeerFreq)) {
		DBGLOG(REQ, ERROR, "Integer format error\n");
		return -1;
	}

	fgIsApMode = prNetDev->ieee80211_ptr->iftype == NL80211_IFTYPE_AP ?
		TRUE : FALSE;
	eMainLinkBand = cnmGetBandByFreq(u4MainLinkFreq);

	if (eMainLinkBand == BAND_NULL) {
		DBGLOG(REQ, ERROR, "Invalid band\n");
		return -1;
	}

	p2pLinkGet2ndLinkFreq(prAdapter, fgIsApMode, eMainLinkBand,
			      u4MainLinkFreq, u4PeerFreq, &u4PreferFreq);

	i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen, "%d",
		u4PreferFreq);

	return i4BytesWritten;
}

int priv_driver_get_ml_prefer_freqlist(struct net_device *prNetDev,
	char *pcCommand,
	int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAd = NULL;
	struct BSS_INFO *bss = NULL;
	int32_t i4BytesWritten = 0;

	DBGLOG(REQ, TRACE, "command is %s\n", pcCommand);

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAd = prGlueInfo->prAdapter;
	if (!prAd)
		return -1;

	bss = aisGetConnectedBssInfo(prGlueInfo->prAdapter);

	i4BytesWritten = kalSnprintf(
		pcCommand, i4TotalLen, "%d",
		(bss) ? nicChannelNum2Freq(
			bss->ucPrimaryChannel,
			bss->eBand) / 1000 : 0);

	DBGLOG(REQ, INFO, "command result is %s\n", pcCommand);

	return i4BytesWritten;
}

#endif /* CFG_ENABLE_WIFI_DIRECT */
#endif

#if CFG_SUPPORT_QA_TOOL
int priv_driver_set_test_mode(struct net_device *prNetDev,
				     char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret;
	int32_t u4MagicKey = -1;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	u4Ret = kalkStrtou32(apcArgv[1], 0, &(u4MagicKey));
	if (u4Ret)
		DBGLOG(REQ, LOUD, "parse Magic Key error u4Ret=%d\n", u4Ret);

	DBGLOG(REQ, LOUD, "The Set Test Mode Magic Key is %d\n", u4MagicKey);

	if (u4MagicKey == PRIV_CMD_TEST_MAGIC_KEY) {
		rStatus = glSetRFTestMode(prGlueInfo, 1);
	} else if (u4MagicKey == 0) {
		rStatus = glSetRFTestMode(prGlueInfo, 0);
	}

	DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);

	if (rStatus != WLAN_STATUS_SUCCESS)
		return -1;

	return i4BytesWritten;

}				/* priv_driver_set_test_mode */

int priv_driver_get_test_mode(struct net_device *prNetDev,
				     char *pcCommand, int i4TotalLen)
{
	int32_t i4BytesWritten = 0;
	uint32_t ret = 0;

	if (!prNetDev)
		return i4BytesWritten;

	ret = glIsWifiInTestMode(prNetDev);

	if (ret == TRUE) {
		i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen,
						"Wifi In Test Mode\n");
	} else {
		i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen,
						"Wifi In Normal Mode\n");
	}

	return i4BytesWritten;

}				/* priv_driver_get_test_mode */

int priv_driver_set_test_cmd(struct net_device *prNetDev,
				    char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret;
	int32_t i4ArgNum = 3;
	struct PARAM_MTK_WIFI_TEST_STRUCT rRfATInfo;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc >= i4ArgNum) {
		rRfATInfo.u4FuncIndex = 0;
		u4Ret = kalkStrtou32(apcArgv[1], 0, &(rRfATInfo.u4FuncIndex));
		if (u4Ret)
			DBGLOG(REQ, LOUD,
			       "Parse Test CMD Index error u4Ret=%d\n", u4Ret);

		rRfATInfo.u4FuncData = 0;
		u4Ret = kalkStrtou32(apcArgv[2], 0, &(rRfATInfo.u4FuncData));
		if (u4Ret)
			DBGLOG(REQ, LOUD,
			       "Parse Test CMD Data error u4Ret=%d\n", u4Ret);

		DBGLOG(REQ, LOUD,
		       "Set Test CMD FuncIndex = %d, FuncData = %d\n",
		       rRfATInfo.u4FuncIndex, rRfATInfo.u4FuncData);

		rStatus = kalIoctl(prGlueInfo, wlanoidRftestSetAutoTest,
				   &rRfATInfo, sizeof(rRfATInfo), &u4BufLen);

		DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
		if (rStatus != WLAN_STATUS_SUCCESS)
			return -1;
	}

	return i4BytesWritten;

}				/* priv_driver_set_test_cmd */

int priv_driver_get_test_result(struct net_device *prNetDev,
				       char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret;
	uint32_t u4Data = 0;
	struct PARAM_MTK_WIFI_TEST_STRUCT rRfATInfo;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	rRfATInfo.u4FuncIndex = 0;
	u4Ret = kalkStrtou32(apcArgv[1], 0, &(rRfATInfo.u4FuncIndex));
	if (u4Ret)
		DBGLOG(REQ, LOUD,
		       "Parse Test CMD Index error u4Ret=%d\n", u4Ret);

	rRfATInfo.u4FuncData = 0;
	u4Ret = kalkStrtou32(apcArgv[2], 0, &(rRfATInfo.u4FuncData));
	if (u4Ret)
		DBGLOG(REQ, LOUD,
		       "Parse Test CMD Data error u4Ret=%d\n", u4Ret);

	DBGLOG(REQ, LOUD,
	       "Get Test CMD FuncIndex = %d, FuncData = %d\n",
	       rRfATInfo.u4FuncIndex, rRfATInfo.u4FuncData);

	rStatus = kalIoctl(prGlueInfo, wlanoidRftestQueryAutoTest,
			   &rRfATInfo, sizeof(rRfATInfo), &u4BufLen);

	DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
	if (rStatus != WLAN_STATUS_SUCCESS)
		return -1;
	u4Data = (unsigned int)rRfATInfo.u4FuncData;
	i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen,
					"%d[0x%08x]", u4Data, u4Data);
	DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__,
	       pcCommand);

	return i4BytesWritten;

}				/* priv_driver_get_test_result */
#endif /* #CFG_SUPPORT_QA_TOOL */

#if (CFG_SUPPORT_RA_GEN == 1)
int32_t priv_driver_set_ra_debug_proc(struct net_device *prNetDev,
					char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	int8_t *this_char = NULL;
	int32_t i4Recv = 0;
	uint32_t u4WCID = 0, u4DebugType = 0;
	uint32_t u4Id = 0xa0650000;
	struct PARAM_CUSTOM_SW_CTRL_STRUCT *prSwCtrlInfo;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %d, apcArgv[0] = %s\n\n", i4Argc, *apcArgv);

	this_char = kalStrStr(*apcArgv, "=");
	if (!this_char)
		return -1;
	this_char++;

	DBGLOG(REQ, LOUD, "string = %s\n", this_char);

	i4Recv = sscanf(this_char, "%d:%d", &(u4WCID), &(u4DebugType));
	if (i4Recv < 0)
		return -1;

	prSwCtrlInfo =
		(struct PARAM_CUSTOM_SW_CTRL_STRUCT *)kalMemAlloc(
				sizeof(struct PARAM_CUSTOM_SW_CTRL_STRUCT),
				VIR_MEM_TYPE);
	if (!prSwCtrlInfo)
		return -1;

	if (i4Recv == 2) {
		prSwCtrlInfo->u4Id = u4Id;
		prSwCtrlInfo->u4Data = u4WCID |
					((u4DebugType & BITS(0, 15)) << 16);

		rStatus = kalIoctl(prGlueInfo, wlanoidSetSwCtrlWrite,
				   prSwCtrlInfo,
				   sizeof(struct PARAM_CUSTOM_SW_CTRL_STRUCT),
				   &u4BufLen);

		if (rStatus != WLAN_STATUS_SUCCESS) {
			kalMemFree(prSwCtrlInfo, VIR_MEM_TYPE,
				   sizeof(struct PARAM_CUSTOM_SW_CTRL_STRUCT));
			return -1;
		}

		DBGLOG(REQ, LOUD, "WlanIdx=%d\nDebugType=%d\nu4Data=0x%08x\n",
			u4WCID, u4DebugType, prSwCtrlInfo->u4Data);
	} else {
		DBGLOG(INIT, ERROR,
		       "iwpriv wlanXX driver RaDebug=[wlanIdx]:[debugType]\n");
	}

	kalMemFree(prSwCtrlInfo, VIR_MEM_TYPE,
		   sizeof(struct PARAM_CUSTOM_SW_CTRL_STRUCT));

	return i4BytesWritten;
}

int priv_driver_set_fixed_fallback(struct net_device *prNetDev,
				   char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	/* INT_32 u4Ret = 0; */
	uint32_t u4WCID = 0;
	uint32_t u4Mode = 0, u4Bw = 0, u4Mcs = 0, u4VhtNss = 0, u4Band = 0;
	uint32_t u4SGI = 0, u4Preamble = 0, u4STBC = 0, u4LDPC = 0, u4SpeEn = 0;
	int32_t i4Recv = 0;
	int8_t *this_char = NULL;
	uint32_t u4Id = 0xa0660000;
	uint32_t u4Data = 0x80000000;
	uint32_t u4Id2 = 0xa0600000;
	uint8_t u4Nsts = 1;
	u_int8_t fgStatus = TRUE;
	static uint8_t fgIsUseWCID = FALSE;

	struct PARAM_CUSTOM_SW_CTRL_STRUCT rSwCtrlInfo;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %d, apcArgv[0] = %s\n\n", i4Argc, *apcArgv);

	this_char = kalStrStr(*apcArgv, "=");
	if (!this_char)
		return -1;
	this_char++;

	DBGLOG(REQ, LOUD, "string = %s\n", this_char);

	if (strnicmp(this_char, "auto", strlen("auto")) == 0) {
		i4Recv = 1;
	} else if (strnicmp(this_char, "UseWCID", strlen("UseWCID")) == 0) {
		i4Recv = 2;
		fgIsUseWCID = TRUE;
	} else if (strnicmp(this_char, "ApplyAll", strlen("ApplyAll")) == 0) {
		i4Recv = 3;
		fgIsUseWCID = FALSE;
	} else {
		i4Recv = sscanf(this_char, "%d-%d-%d-%d-%d-%d-%d-%d-%d-%d-%d",
				&(u4WCID), &(u4Mode), &(u4Bw), &(u4Mcs),
				&(u4VhtNss), &(u4SGI), &(u4Preamble), &(u4STBC),
				&(u4LDPC), &(u4SpeEn), &(u4Band));

		DBGLOG(REQ, LOUD, "u4WCID=%d\nu4Mode=%d\nu4Bw=%d\n",
		       u4WCID, u4Mode, u4Bw);
		DBGLOG(REQ, LOUD, "u4Mcs=%d\nu4VhtNss=%d\nu4SGI=%d\n",
		       u4Mcs, u4VhtNss, u4SGI);
		DBGLOG(REQ, LOUD, "u4Preamble=%d\nu4STBC=%d\n",
		       u4Preamble, u4STBC);
		DBGLOG(REQ, LOUD, "u4LDPC=%d\nu4SpeEn=%d\nu4Band=%d\n",
		       u4LDPC, u4SpeEn, u4Band);
		DBGLOG(REQ, LOUD, "fgIsUseWCID=%d\n\n",
		       fgIsUseWCID);
	}

	if (i4Recv == 1) {
		rSwCtrlInfo.u4Id = u4Id2;
		rSwCtrlInfo.u4Data = 0;

		rStatus = kalIoctl(prGlueInfo, wlanoidSetSwCtrlWrite,
				   &rSwCtrlInfo, sizeof(rSwCtrlInfo),
				   &u4BufLen);

		if (rStatus != WLAN_STATUS_SUCCESS)
			return -1;
	} else if (i4Recv == 2 || i4Recv == 3) {
		i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen,
			"Update fgIsUseWCID %d\n", fgIsUseWCID);
	} else if (i4Recv == 11) {
		rSwCtrlInfo.u4Id = u4Id;
		rSwCtrlInfo.u4Data = u4Data;
		if (fgIsUseWCID && u4WCID < WTBL_SIZE &&
			prAdapter->rWifiVar.arWtbl[u4WCID].ucUsed) {
			rSwCtrlInfo.u4Id |= u4WCID;
			rSwCtrlInfo.u4Id |= BIT(8);
			i4BytesWritten = kalSnprintf(
				pcCommand, i4TotalLen,
				"Apply WCID %d\n", u4WCID);
		} else {
			i4BytesWritten = kalSnprintf(
				pcCommand, i4TotalLen, "Apply All\n");
		}

		if (u4SGI)
			rSwCtrlInfo.u4Data |= BIT(30);
		if (u4LDPC)
			rSwCtrlInfo.u4Data |= BIT(29);
		if (u4SpeEn)
			rSwCtrlInfo.u4Data |= BIT(28);
		if (u4Band)
			rSwCtrlInfo.u4Data |= BIT(25);
		if (u4STBC)
			rSwCtrlInfo.u4Data |= BIT(11);

		if (u4Bw <= 3)
			rSwCtrlInfo.u4Data |= ((u4Bw << 26) & BITS(26, 27));
		else {
			fgStatus = FALSE;
			DBGLOG(INIT, ERROR,
			       "Wrong BW! BW20=0, BW40=1, BW80=2,BW160=3\n");
		}
		if (u4Mode <= 4) {
			rSwCtrlInfo.u4Data |= ((u4Mode << 6) & BITS(6, 8));

			switch (u4Mode) {
			case 0:
				if (u4Mcs <= 3)
					rSwCtrlInfo.u4Data |= u4Mcs;
				else {
					fgStatus = FALSE;
					DBGLOG(INIT, ERROR,
					       "CCK mode but wrong MCS!\n");
				}

				if (u4Preamble)
					rSwCtrlInfo.u4Data |= BIT(2);
				else
					rSwCtrlInfo.u4Data &= ~BIT(2);
			break;
			case 1:
				switch (u4Mcs) {
				case 0:
					/* 6'b001011 */
					rSwCtrlInfo.u4Data |= 11;
					break;
				case 1:
					/* 6'b001111 */
					rSwCtrlInfo.u4Data |= 15;
					break;
				case 2:
					/* 6'b001010 */
					rSwCtrlInfo.u4Data |= 10;
					break;
				case 3:
					/* 6'b001110 */
					rSwCtrlInfo.u4Data |= 14;
					break;
				case 4:
					/* 6'b001001 */
					rSwCtrlInfo.u4Data |= 9;
					break;
				case 5:
					/* 6'b001101 */
					rSwCtrlInfo.u4Data |= 13;
					break;
				case 6:
					/* 6'b001000 */
					rSwCtrlInfo.u4Data |= 8;
					break;
				case 7:
					/* 6'b001100 */
					rSwCtrlInfo.u4Data |= 12;
					break;
				default:
					fgStatus = FALSE;
					DBGLOG(INIT, ERROR,
					       "OFDM mode but wrong MCS!\n");
				break;
				}
			break;
			case 2:
			case 3:
				if (u4Mcs <= 32)
					rSwCtrlInfo.u4Data |= u4Mcs;
				else {
					fgStatus = FALSE;
					DBGLOG(INIT, ERROR,
					       "HT mode but wrong MCS!\n");
				}

				if (u4Mcs != 32) {
					u4Nsts += (u4Mcs >> 3);
					if (u4STBC && (u4Nsts == 1))
						u4Nsts++;
				}
				break;
			case 4:
				if (u4Mcs <= 9)
					rSwCtrlInfo.u4Data |= u4Mcs;
				else {
				    fgStatus = FALSE;
				    DBGLOG(INIT, ERROR,
					"VHT mode but wrong MCS!\n");
				}
				if (u4STBC && (u4VhtNss == 1))
					u4Nsts++;
				else if (u4VhtNss >= 1 && u4VhtNss <= 8)
					u4Nsts = u4VhtNss;
			break;
			default:
				break;
			}
		} else {
			fgStatus = FALSE;
			DBGLOG(INIT, ERROR,
			       "Wrong TxMode! CCK=0, OFDM=1, HT=2, GF=3, VHT=4\n");
		}

		rSwCtrlInfo.u4Data |= (((u4Nsts - 1) << 9) & BITS(9, 10));

		if (fgStatus) {
			rStatus = kalIoctl(prGlueInfo, wlanoidSetSwCtrlWrite,
					   &rSwCtrlInfo, sizeof(rSwCtrlInfo),
					   &u4BufLen);
		}

		if (rStatus != WLAN_STATUS_SUCCESS)
			return -1;
	} else {
		DBGLOG(INIT, ERROR, "iwpriv wlanXX driver FixedRate=Option\n");
		DBGLOG(INIT, ERROR,
		       "Option:[WCID]-[Mode]-[BW]-[MCS]-[VhtNss]-[SGI]-[Preamble]-[STBC]-[LDPC]-[SPE_EN]-[BAND]\n");
		DBGLOG(INIT, ERROR, "[WCID]Wireless Client ID\n");
		DBGLOG(INIT, ERROR, "[Mode]CCK=0, OFDM=1, HT=2, GF=3, VHT=4\n");
		DBGLOG(INIT, ERROR, "[BW]BW20=0, BW40=1, BW80=2,BW160=3\n");
		DBGLOG(INIT, ERROR,
		       "[MCS]CCK=0~3, OFDM=0~7, HT=0~32, VHT=0~9\n");
		DBGLOG(INIT, ERROR, "[VhtNss]VHT=1~4, Other=ignore\n");
		DBGLOG(INIT, ERROR, "[Preamble]Long=0, Other=Short\n");
		DBGLOG(INIT, ERROR, "[BAND]2G=0, Other=5G\n");
	}

	return i4BytesWritten;
}
#endif

#if (CFG_SUPPORT_TXPOWER_INFO == 1)
static int32_t priv_driver_dump_txpower_info(struct ADAPTER *prAdapter,
		char *pcCommand, int i4TotalLen,
		struct PARAM_TXPOWER_ALL_RATE_POWER_INFO_T *prTxPowerInfo,
		uint32_t u4DumpType)
{
	int32_t i4BytesWritten = 0;

	if (prTxPowerInfo->ucTxPowerCategory ==
	    TXPOWER_EVENT_SHOW_ALL_RATE_TXPOWER_INFO) {
		uint8_t ucTxPwrIdx = 0, ucTxPwrType = 0;
		uint16_t u2Idx = 0, u2IdxOffset = 0;

		char *rateStr = NULL;
		struct FRAME_POWER_CONFIG_INFO_T rRatePowerInfo;
		char *cckRateStr[MODULATION_SYSTEM_CCK_NUM] = {
			"1M", "2M", "5M", "11M"};
		char *ofdmRateStr[MODULATION_SYSTEM_OFDM_NUM] = {
			"6M", "9M", "12M", "18M", "24M", "36M", "48M", "54M" };
		char *ht40RateStr[MODULATION_SYSTEM_HT40_NUM] = {
			"M0", "M1", "M2", "M3", "M4", "M5", "M6",
			"M7", "M32" };
		char *allRateStr[MODULATION_SYSTEM_EHT_MCS_NUM] = {
			"M0", "M1", "M2", "M3", "M4", "M5", "M6", "M7",
			"M8", "M9", "M10", "M11", "M12", "M13", "M14", "M15"};

		uint8_t *pucStr = NULL;
		uint8_t *POWER_TYPE_STR[] = {
			"CCK", "OFDM", "HT20", "HT40",
			"VHT20", "VHT40", "VHT80",
			"VHT160", "HE26", "HE52", "HE106",
			"HE242", "HE484", "HE996", "HE996x2",
			"EHT26", "EHT52", "EHT106", "EHT242",
			"EHT484", "EHT996", "EHT996X2", "EHT996X4",
			"EHT26_52", "EHT26_106", "EHT484_242",
			"EHT996_484", "EHT996_484_242", "EHT996X2_484",
			"EHT_996X3", "EHT996X3_484"};
		uint8_t ucPwrIdxLen[] = {
			MODULATION_SYSTEM_CCK_NUM,
			MODULATION_SYSTEM_OFDM_NUM,
			MODULATION_SYSTEM_HT20_NUM,
			MODULATION_SYSTEM_HT40_NUM,
			MODULATION_SYSTEM_VHT20_NUM,
			MODULATION_SYSTEM_VHT40_NUM,
			MODULATION_SYSTEM_VHT80_NUM,
			MODULATION_SYSTEM_VHT160_NUM,
			MODULATION_SYSTEM_HE_26_MCS_NUM,
			MODULATION_SYSTEM_HE_52_MCS_NUM,
			MODULATION_SYSTEM_HE_106_MCS_NUM,
			MODULATION_SYSTEM_HE_242_MCS_NUM,
			MODULATION_SYSTEM_HE_484_MCS_NUM,
			MODULATION_SYSTEM_HE_996_MCS_NUM,
			MODULATION_SYSTEM_HE_996X2_MCS_NUM,
			MODULATION_SYSTEM_EHT_26_MCS_NUM,
			MODULATION_SYSTEM_EHT_52_MCS_NUM,
			MODULATION_SYSTEM_EHT_106_MCS_NUM,
			MODULATION_SYSTEM_EHT_242_MCS_NUM,
			MODULATION_SYSTEM_EHT_484_MCS_NUM,
			MODULATION_SYSTEM_EHT_996_MCS_NUM,
			MODULATION_SYSTEM_EHT_996X2_MCS_NUM,
			MODULATION_SYSTEM_EHT_996X4_MCS_NUM,
			MODULATION_SYSTEM_EHT_26_52_MCS_NUM,
			MODULATION_SYSTEM_EHT_26_106_MCS_NUM,
			MODULATION_SYSTEM_EHT_484_242_MCS_NUM,
			MODULATION_SYSTEM_EHT_996_484_MCS_NUM,
			MODULATION_SYSTEM_EHT_996_484_242_MCS_NUM,
			MODULATION_SYSTEM_EHT_996X2_484_MCS_NUM,
			MODULATION_SYSTEM_EHT_996X3_MCS_NUM,
			MODULATION_SYSTEM_EHT_996X3_484_MCS_NUM};

		uint8_t ucBandIdx;
		uint8_t ucFormat;

		if (ARRAY_SIZE(POWER_TYPE_STR) != ARRAY_SIZE(ucPwrIdxLen))
			return i4BytesWritten;

		ucBandIdx = prTxPowerInfo->ucBandIdx;
		ucFormat = prTxPowerInfo->u1Format;

		i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
					i4TotalLen - i4BytesWritten, "%s",
					"\n====== TX POWER INFO ======\n");
		i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
					i4TotalLen - i4BytesWritten,
					"DBDC Index: %d, Channel Band: %s\n",
					ucBandIdx,
					(prTxPowerInfo->ucChBand) ?
						("5G") : ("2G"));
		i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
					i4TotalLen - i4BytesWritten,
					"%s", "===========================\n");

		rRatePowerInfo =
			prTxPowerInfo->rRatePowerInfo;

		if (u4DumpType == 1) {
			/* EHT */
			i4BytesWritten += kalScnprintf(
				 pcCommand + i4BytesWritten,
				 i4TotalLen - i4BytesWritten,
				 "%17s", "");

			for (ucTxPwrIdx = 0;
			     ucTxPwrIdx < MODULATION_SYSTEM_EHT_MCS_NUM;
			     ucTxPwrIdx++) {
				i4BytesWritten += kalScnprintf(
					pcCommand + i4BytesWritten,
					i4TotalLen - i4BytesWritten,
					"%5s", allRateStr[ucTxPwrIdx]);
			}

			i4BytesWritten += kalScnprintf(
				 pcCommand + i4BytesWritten,
				 i4TotalLen - i4BytesWritten,
				 "\n");
		}

		for (ucTxPwrType = 0; ucTxPwrType < ARRAY_SIZE(POWER_TYPE_STR);
		     ucTxPwrType++) {

			pucStr = POWER_TYPE_STR[ucTxPwrType];

			if (u4DumpType == 0) {
				/* LG, HT, VHT, HE */
				if (kalStrnCmp(pucStr, "EHT", 3) == 0) {
					u2IdxOffset += ucPwrIdxLen[ucTxPwrType];
					continue;
				} else
					i4BytesWritten += kalScnprintf(
						pcCommand + i4BytesWritten,
						i4TotalLen - i4BytesWritten,
						"[%7s] > ", pucStr);
			} else if (u4DumpType == 1) {
				/* EHT */
				if (kalStrnCmp(pucStr, "EHT", 3) != 0) {
					u2IdxOffset += ucPwrIdxLen[ucTxPwrType];
					continue;
				} else
					i4BytesWritten += kalScnprintf(
						pcCommand + i4BytesWritten,
						i4TotalLen - i4BytesWritten,
						"[%14s] > ", pucStr);
			}

			for (ucTxPwrIdx = 0;
			     ucTxPwrIdx < ucPwrIdxLen[ucTxPwrType];
			     ucTxPwrIdx++) {
				/*Legcay format*/
				if (kalStrCmp(pucStr, "CCK") == 0)
					rateStr = cckRateStr[ucTxPwrIdx];
				else if (kalStrCmp(pucStr, "OFDM") == 0)
					rateStr = ofdmRateStr[ucTxPwrIdx];
				else if (kalStrCmp(pucStr, "HT20") == 0)
					rateStr = allRateStr[ucTxPwrIdx];
				else if (kalStrCmp(pucStr, "HT40") == 0)
					rateStr = ht40RateStr[ucTxPwrIdx];
				else if (kalStrnCmp(pucStr, "VHT", 3) == 0)
					rateStr = allRateStr[ucTxPwrIdx];
				/*HE format*/
				else if ((kalStrnCmp(pucStr, "HE", 2) == 0)
					&& (ucFormat == TXPOWER_FORMAT_HE))
					rateStr = allRateStr[ucTxPwrIdx];
				else if (kalStrnCmp(pucStr, "EHT", 3) == 0)
					rateStr = allRateStr[ucTxPwrIdx];
				else
					continue;

				u2Idx = ucTxPwrIdx + u2IdxOffset;

				if (u4DumpType == 0)
					i4BytesWritten += kalScnprintf(
					pcCommand + i4BytesWritten,
					i4TotalLen - i4BytesWritten,
					"%3s:%03d, ",
					rateStr,
					rRatePowerInfo.
					aicFramePowerConfig[u2Idx][ucBandIdx].
					icFramePowerDbm);
				else if (u4DumpType == 1)
					i4BytesWritten += kalScnprintf(
					pcCommand + i4BytesWritten,
					i4TotalLen - i4BytesWritten,
					"%03d, ",
					rRatePowerInfo.
					aicFramePowerConfig[u2Idx][ucBandIdx].
					icFramePowerDbm);
			}
			 i4BytesWritten += kalScnprintf(
				 pcCommand + i4BytesWritten,
				 i4TotalLen - i4BytesWritten,
				 "\n");

			u2IdxOffset += ucPwrIdxLen[ucTxPwrType];
		}

		i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"[MAX][Bound]: 0x%02x (%03d)\n",
				prTxPowerInfo->icPwrMaxBnd,
				prTxPowerInfo->icPwrMaxBnd);

		i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"[MIN][Bound]: 0x%02x (%03d)\n",
				prTxPowerInfo->icPwrMinBnd,
				prTxPowerInfo->icPwrMinBnd);
	}

	return i4BytesWritten;
}

int32_t priv_driver_get_txpower_info(struct net_device *prNetDev,
					char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0, u4Size = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	int8_t *this_char = NULL;
	uint32_t u4ParamNum = 0;
	uint8_t ucParam = 0;
	uint8_t ucBandIdx = 0;
	struct PARAM_TXPOWER_ALL_RATE_POWER_INFO_T *prTxPowerInfo = NULL;
	uint32_t u4DumpType = 0;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	DBGLOG(REQ, INFO, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, INFO, "argc is %d, apcArgv[0] = %s\n\n", i4Argc, *apcArgv);

	this_char = kalStrStr(*apcArgv, "=");
	if (!this_char)
		return -1;
	this_char++;

	DBGLOG(REQ, INFO, "string = %s\n", this_char);

	u4ParamNum = sscanf(this_char, "%hhu:%hhu:%hhu",
		&ucParam, &ucBandIdx, &u4DumpType);
	if ((u4ParamNum != 2) && (u4ParamNum != 3))
		return -1;
	DBGLOG(REQ, INFO, "ParamNum=%u,Param=%u,Band=%u,DumpType=%u\n",
		u4ParamNum, ucParam, ucBandIdx, u4DumpType);

	u4Size = sizeof(struct PARAM_TXPOWER_ALL_RATE_POWER_INFO_T);

	prTxPowerInfo =
		(struct PARAM_TXPOWER_ALL_RATE_POWER_INFO_T *)kalMemAlloc(
							u4Size, VIR_MEM_TYPE);
	if (!prTxPowerInfo)
		return -1;

	if (u4ParamNum >= TXPOWER_INFO_INPUT_ARG_NUM) {
		prTxPowerInfo->ucTxPowerCategory = ucParam;
		prTxPowerInfo->ucBandIdx = ucBandIdx;

		rStatus = kalIoctl(prGlueInfo, wlanoidQueryTxPowerInfo,
			prTxPowerInfo,
			sizeof(struct PARAM_TXPOWER_ALL_RATE_POWER_INFO_T),
			&u4BufLen);

		if (rStatus != WLAN_STATUS_SUCCESS) {
			kalMemFree(prTxPowerInfo, VIR_MEM_TYPE,
			    sizeof(struct PARAM_TXPOWER_ALL_RATE_POWER_INFO_T));
			return -1;
		}

		i4BytesWritten = priv_driver_dump_txpower_info(prAdapter,
					pcCommand, i4TotalLen, prTxPowerInfo,
					u4DumpType);
	} else {
		DBGLOG(INIT, ERROR,
		       "[Error]iwpriv wlanXX driver TxPowerInfo=[Param]:[BandIdx]\n");
	}

	kalMemFree(prTxPowerInfo, VIR_MEM_TYPE,
		   sizeof(struct PARAM_TXPOWER_ALL_RATE_POWER_INFO_T));

	return i4BytesWritten;
}
#endif
int32_t priv_driver_txpower_man_set(struct net_device *prNetDev,
					char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0, u4Size = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	int8_t *this_char = NULL;
	uint32_t u4ParamNum = 0;
	uint8_t ucPhyMode = 0;
	uint8_t ucTxRate = 0;
	uint8_t ucBw = 0;
	int8_t iTargetPwr = 0;
	struct PARAM_TXPOWER_BY_RATE_SET_T *prPwrCtl = NULL;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	DBGLOG(REQ, INFO, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, INFO, "argc is %d, apcArgv[0] = %s\n\n", i4Argc, *apcArgv);

	this_char = kalStrStr(*apcArgv, "=");
	if (!this_char)
		return -1;
	this_char++;

	DBGLOG(REQ, INFO, "string = %s\n", this_char);

	u4ParamNum = sscanf(this_char, "%hhu:%hhu:%hhu:%hhu",
		&ucPhyMode, &ucTxRate, &ucBw, &iTargetPwr);
	if (u4ParamNum != 4)
		return -1;

	if (u4ParamNum != 4) {
		DBGLOG(REQ, WARN, "sscanf input fail\n");
		return -1;
	}

	DBGLOG(REQ, INFO, "ParamNum=%d,PhyMod=%d,Rate=%d,Bw=%d,Pwr=%d\n",
		u4ParamNum, ucPhyMode, ucTxRate, ucBw, iTargetPwr);

	u4Size = sizeof(struct PARAM_TXPOWER_BY_RATE_SET_T);

	prPwrCtl =
		(struct PARAM_TXPOWER_BY_RATE_SET_T *)kalMemAlloc(
							u4Size, VIR_MEM_TYPE);
	if (!prPwrCtl)
		return -1;

	if (u4ParamNum == TXPOWER_MAN_SET_INPUT_ARG_NUM) {
		prPwrCtl->u1PhyMode = ucPhyMode;
		prPwrCtl->u1TxRate = ucTxRate;
		prPwrCtl->u1BW = ucBw;
		prPwrCtl->i1TxPower = iTargetPwr;

		rStatus = kalIoctl(prGlueInfo, wlanoidSetTxPowerByRateManual,
			prPwrCtl, sizeof(struct PARAM_TXPOWER_BY_RATE_SET_T),
			&u4BufLen);

		if (rStatus != WLAN_STATUS_SUCCESS) {
			kalMemFree(prPwrCtl, VIR_MEM_TYPE,
			    sizeof(struct PARAM_TXPOWER_BY_RATE_SET_T));
			return -1;
		}

	} else {
		DBGLOG(INIT, ERROR,
			"[Error]iwpriv wlanXX driver TxPwrManualSet=PhyMode:Rate:Bw:Pwr\n");
	}

	kalMemFree(prPwrCtl, VIR_MEM_TYPE,
		   sizeof(struct PARAM_TXPOWER_BY_RATE_SET_T));

	return i4BytesWritten;
}

int priv_driver_get_sta_stat(struct net_device *prNetDev,
				    char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0, u4Ret, u4StatGroup = 0xFFFFFFFF;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint8_t aucMacAddr[MAC_ADDR_LEN] = {0};
	uint8_t ucWlanIndex = 0;
	uint8_t *pucMacAddr = NULL;
	struct PARAM_HW_WLAN_INFO *prHwWlanInfo = NULL;
	struct PARAM_GET_STA_STATISTICS *prQueryStaStatistics = NULL;
	u_int8_t fgResetCnt = FALSE;
	u_int8_t fgRxCCSel = FALSE;
	u_int8_t fgSearchMacAddr = FALSE;
	struct BSS_INFO *prAisBssInfo = NULL;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

#if CFG_SUPPORT_LINK_QUALITY_MONITOR
	prAdapter->u4LastLinkQuality =
		kalGetTimeTick() -
		MSEC_TO_SYSTIME(SEC_TO_MSEC(CFG_LQ_MONITOR_FREQUENCY));
#endif

	prAisBssInfo = aisGetAisBssInfo(
		prGlueInfo->prAdapter, wlanGetBssIdx(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc >= 4) {
		if (strnicmp(apcArgv[2], CMD_STAT_GROUP_SEL,
		    strlen(CMD_STAT_GROUP_SEL)) == 0) {
			u4Ret = kalkStrtou32(apcArgv[3], 0, &(u4StatGroup));

			if (u4Ret)
				DBGLOG(REQ, LOUD,
				       "parse get_sta_stat error (Group) u4Ret=%d\n",
				       u4Ret);
			if (u4StatGroup == 0)
				u4StatGroup = 0xFFFFFFFF;

			wlanHwAddrToBin(apcArgv[1], &aucMacAddr[0]);

			if (!wlanGetWlanIdxByAddress(prGlueInfo->prAdapter,
			    &aucMacAddr[0], &ucWlanIndex)) {
				DBGLOG(REQ, INFO,
				      "wlan index of "MACSTR" is not found!\n",
				      MAC2STR(aucMacAddr));
				goto out;
			}
		} else {
			goto out;
		}
	} else if (i4Argc >= 3) {
		if (strnicmp(apcArgv[1], CMD_STAT_GROUP_SEL,
		    strlen(CMD_STAT_GROUP_SEL)) == 0) {
			u4Ret = kalkStrtou32(apcArgv[2], 0, &(u4StatGroup));

			if (u4Ret)
				DBGLOG(REQ, LOUD,
				       "parse get_sta_stat error (Group) u4Ret=%d\n",
				       u4Ret);
			if (u4StatGroup == 0)
				u4StatGroup = 0xFFFFFFFF;

			if (prAisBssInfo->prStaRecOfAP) {
				ucWlanIndex = prAisBssInfo->prStaRecOfAP
						->ucWlanIndex;
			} else if (!wlanGetWlanIdxByAddress(
				prGlueInfo->prAdapter, NULL,
				&ucWlanIndex)) {
				DBGLOG(REQ, INFO,
				      "wlan index of "MACSTR" is not found!\n",
				      MAC2STR(aucMacAddr));
				goto out;
			}
		} else {
			if (strnicmp(apcArgv[1], CMD_STAT_RESET_CNT,
			    strlen(CMD_STAT_RESET_CNT)) == 0) {
				wlanHwAddrToBin(apcArgv[2], &aucMacAddr[0]);
				fgResetCnt = TRUE;
			} else if (strnicmp(apcArgv[2], CMD_STAT_RESET_CNT,
			    strlen(CMD_STAT_RESET_CNT)) == 0) {
				wlanHwAddrToBin(apcArgv[1], &aucMacAddr[0]);
				fgResetCnt = TRUE;
			} else {
				wlanHwAddrToBin(apcArgv[1], &aucMacAddr[0]);
				fgResetCnt = FALSE;
			}

			if (!wlanGetWlanIdxByAddress(prGlueInfo->prAdapter,
			    &aucMacAddr[0], &ucWlanIndex)) {
				DBGLOG(REQ, INFO,
				      "wlan index of "MACSTR" is not found!\n",
				      MAC2STR(aucMacAddr));
				goto out;
			}
		}
	} else {
		if (i4Argc == 1) {
			fgSearchMacAddr = TRUE;
		} else if (i4Argc == 2) {
			if (strnicmp(apcArgv[1], CMD_STAT_RESET_CNT,
			    strlen(CMD_STAT_RESET_CNT)) == 0) {
				fgResetCnt = TRUE;
				fgSearchMacAddr = TRUE;
			} else if (strnicmp(apcArgv[1], CMD_STAT_NOISE_SEL,
			    strlen(CMD_STAT_NOISE_SEL)) == 0) {
				fgRxCCSel = TRUE;
				fgSearchMacAddr = TRUE;
			} else {
				wlanHwAddrToBin(apcArgv[1], &aucMacAddr[0]);

				if (!wlanGetWlanIdxByAddress(prGlueInfo->
				    prAdapter, &aucMacAddr[0], &ucWlanIndex)) {
					DBGLOG(REQ, INFO,
						"No connected peer found!\n");
					goto out;
				}
			}
		}

		if (fgSearchMacAddr) {
			/* Get AIS AP address for no argument */
			if (prAisBssInfo->prStaRecOfAP) {
				ucWlanIndex = prAisBssInfo->prStaRecOfAP
					->ucWlanIndex;
			} else if (!wlanGetWlanIdxByAddress(prGlueInfo->
				prAdapter, NULL, &ucWlanIndex)) {
				DBGLOG(REQ, INFO, "No connected peer found!\n");
				goto out;
			}
		}
	}

	prHwWlanInfo = (struct PARAM_HW_WLAN_INFO *)kalMemAlloc(
			sizeof(struct PARAM_HW_WLAN_INFO), VIR_MEM_TYPE);
	if (!prHwWlanInfo) {
		DBGLOG(REQ, ERROR,
			"Allocate prHwWlanInfo failed!\n");
		i4BytesWritten = -1;
		goto out;
	}

	prHwWlanInfo->u4Index = ucWlanIndex;
	if (fgRxCCSel == TRUE)
		prHwWlanInfo->rWtblRxCounter.fgRxCCSel = TRUE;
	else
		prHwWlanInfo->rWtblRxCounter.fgRxCCSel = FALSE;

	DBGLOG(REQ, INFO, "index = %d i4TotalLen = %d\n",
	       prHwWlanInfo->u4Index, i4TotalLen);

	/* Get WTBL info */
	rStatus = kalIoctl(prGlueInfo, wlanoidQueryWlanInfo, prHwWlanInfo,
			   sizeof(struct PARAM_HW_WLAN_INFO), &u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, ERROR, "Query prHwWlanInfo failed!\n");
		i4BytesWritten = -1;
		goto out;
	}

	/* Get Statistics info */
	prQueryStaStatistics =
		(struct PARAM_GET_STA_STATISTICS *)kalMemAlloc(
			sizeof(struct PARAM_GET_STA_STATISTICS), VIR_MEM_TYPE);
	if (!prQueryStaStatistics) {
		DBGLOG(REQ, ERROR,
			"Allocate prQueryStaStatistics failed!\n");
		i4BytesWritten = -1;
		goto out;
	}

	kalMemZero(prQueryStaStatistics,
		sizeof(struct PARAM_GET_STA_STATISTICS));

	prQueryStaStatistics->ucResetCounter = fgResetCnt;

	pucMacAddr = wlanGetStaAddrByWlanIdx(prGlueInfo->prAdapter,
					     ucWlanIndex);

	if (!pucMacAddr) {
		DBGLOG(REQ, ERROR, "Addr of WlanIndex %d is not found!\n",
			ucWlanIndex);
		i4BytesWritten = -1;
		goto out;
	}
	COPY_MAC_ADDR(prQueryStaStatistics->aucMacAddr, pucMacAddr);

	rStatus = kalIoctl(prGlueInfo, wlanoidQueryStaStatistics,
			   prQueryStaStatistics,
			   sizeof(struct PARAM_GET_STA_STATISTICS), &u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, ERROR, "Query prQueryStaStatistics failed!\n");
		i4BytesWritten = -1;
		goto out;
	}

	if (pucMacAddr) {
		struct CHIP_DBG_OPS *prChipDbg;

		prChipDbg = prAdapter->chip_info->prDebugOps;

		if (prChipDbg && prChipDbg->show_stat_info)
			i4BytesWritten = prChipDbg->show_stat_info(prAdapter,
				pcCommand, i4TotalLen, prHwWlanInfo,
				prQueryStaStatistics, fgResetCnt, u4StatGroup);
	}
	DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__, pcCommand);

out:
	if (prHwWlanInfo)
		kalMemFree(prHwWlanInfo, VIR_MEM_TYPE,
			sizeof(struct PARAM_HW_WLAN_INFO));

	if (prQueryStaStatistics)
		kalMemFree(prQueryStaStatistics, VIR_MEM_TYPE,
			sizeof(struct PARAM_GET_STA_STATISTICS));


	if (fgResetCnt)
		nicRxClearStatistics(prGlueInfo->prAdapter);

	return i4BytesWritten;
}

#if CFG_SUPPORT_QA_TOOL
#if (CFG_SUPPORT_CONNAC3X == 0)
static int32_t priv_driver_dump_rx_stat_info(struct ADAPTER *prAdapter,
					struct net_device *prNetDev,
					char *pcCommand, int i4TotalLen,
					u_int8_t fgResetCnt)
{
	int32_t i4BytesWritten = 0;
	uint32_t au4RxV[5] = {0};
	uint32_t *prRxV = NULL;
	uint8_t ucStaIdx, ucWlanIndex = 0, cbw;
	u_int8_t fgWlanIdxFound = TRUE, fgSkipRxV = FALSE;
	uint32_t u4FAGCRssiWBR0, u4FAGCRssiIBR0;
	uint32_t u4Value, u4Foe, foe_const;
	static uint32_t au4MacMdrdy[ENUM_BAND_NUM] = {0};
	static uint32_t au4FcsError[ENUM_BAND_NUM] = {0};
	static uint32_t au4OutOfResource[ENUM_BAND_NUM] = {0};
	static uint32_t au4LengthMismatch[ENUM_BAND_NUM] = {0};
	struct STA_RECORD *prStaRecOfAP;

	uint8_t ucBssIndex = wlanGetBssIdx(prNetDev);
	if (ucBssIndex >= MAX_BSSID_NUM) {
		DBGLOG(REQ, ERROR, "ucBssIndex out of range!\n");
		return -1;
	}

	prStaRecOfAP =
		aisGetStaRecOfAP(prAdapter, ucBssIndex);

	au4MacMdrdy[ENUM_BAND_0] += htonl(g_HqaRxStat.MAC_Mdrdy);
	au4MacMdrdy[ENUM_BAND_1] += htonl(g_HqaRxStat.MAC_Mdrdy1);
	au4FcsError[ENUM_BAND_0] += htonl(g_HqaRxStat.MAC_FCS_Err);
	au4FcsError[ENUM_BAND_1] += htonl(g_HqaRxStat.MAC_FCS_Err1);
	au4OutOfResource[ENUM_BAND_0] += htonl(g_HqaRxStat.OutOfResource);
	au4OutOfResource[ENUM_BAND_1] += htonl(g_HqaRxStat.OutOfResource1);
	au4LengthMismatch[ENUM_BAND_0] += htonl(
					g_HqaRxStat.LengthMismatchCount_B0);
	au4LengthMismatch[ENUM_BAND_1] += htonl(
					g_HqaRxStat.LengthMismatchCount_B1);

	DBGLOG(INIT, INFO, "fgResetCnt = %d\n", fgResetCnt);
	if (fgResetCnt) {
		kalMemZero(au4MacMdrdy, sizeof(au4MacMdrdy));
		kalMemZero(au4FcsError, sizeof(au4FcsError));
		kalMemZero(au4OutOfResource, sizeof(au4OutOfResource));
		kalMemZero(au4LengthMismatch, sizeof(au4LengthMismatch));
	}

	if (prStaRecOfAP)
		ucWlanIndex =
			prStaRecOfAP->ucWlanIndex;
	else if (!wlanGetWlanIdxByAddress(prAdapter, NULL, &ucWlanIndex))
		fgWlanIdxFound = FALSE;

	if (fgWlanIdxFound) {
		if (wlanGetStaIdxByWlanIdx(prAdapter, ucWlanIndex, &ucStaIdx)
		    == WLAN_STATUS_SUCCESS) {
			prRxV = prAdapter->arStaRec[ucStaIdx].au4RxV;
			au4RxV[0] = prRxV[0];
			au4RxV[2] = prRxV[2];
			au4RxV[3] = prRxV[3];
			au4RxV[4] = prRxV[4];
		} else{
			fgSkipRxV = TRUE;
		}
	} else{
		fgSkipRxV = TRUE;
	}
	DBGLOG(INIT, INFO, "g_HqaRxStat.MAC_Mdrdy = %d\n",
		htonl(g_HqaRxStat.MAC_Mdrdy));
	DBGLOG(INIT, INFO, "au4MacMdrdy[ENUM_BAND_0] = %d\n",
		au4MacMdrdy[ENUM_BAND_0]);
	DBGLOG(INIT, INFO, "g_HqaRxStat.PhyMdrdyOFDM = %d\n",
		htonl(g_HqaRxStat.PhyMdrdyOFDM));
	DBGLOG(INIT, INFO, "g_HqaRxStat.MAC_FCS_Err= %d\n",
		htonl(g_HqaRxStat.MAC_FCS_Err));
	DBGLOG(INIT, INFO, "au4FcsError[ENUM_BAND_0]= %d\n",
		au4FcsError[ENUM_BAND_0]);
	DBGLOG(INIT, INFO, "g_HqaRxStat.FCSErr_OFDM = %d\n",
		htonl(g_HqaRxStat.FCSErr_OFDM));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%s", "\n\nRX Stat:\n");
#if 1
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "PER0", " = ",
			htonl(g_HqaRxStat.PER0));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "PER1", " = ",
			htonl(g_HqaRxStat.PER1));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "RX OK0", " = ",
			au4MacMdrdy[ENUM_BAND_0] - au4FcsError[ENUM_BAND_0]);

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "RX OK1", " = ",
			au4MacMdrdy[ENUM_BAND_1] - au4FcsError[ENUM_BAND_1]);

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "RCPI0", " = ",
			htonl(g_HqaRxStat.RCPI0));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "RCPI1", " = ",
			htonl(g_HqaRxStat.RCPI1));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "MuRxCnt", " = ",
			htonl(g_HqaRxStat.MRURxCount));
#endif
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "MAC Mdrdy0 diff", " = ",
			htonl(g_HqaRxStat.MAC_Mdrdy));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "MAC Mdrdy0", " = ",
			au4MacMdrdy[ENUM_BAND_0]);

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "MAC Mdrdy1 diff", " = ",
			htonl(g_HqaRxStat.MAC_Mdrdy1));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "MAC Mdrdy1", " = ",
			au4MacMdrdy[ENUM_BAND_1]);

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "FCS Err0", " = ",
			au4FcsError[ENUM_BAND_0]);

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "FCS Err1", " = ",
			au4FcsError[ENUM_BAND_1]);

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "CCK PD Cnt B0", " = ",
			htonl(g_HqaRxStat.CCK_PD));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "CCK PD Cnt B1", " = ",
			htonl(g_HqaRxStat.CCK_PD_Band1));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "CCK SIG Err B0", " = ",
			htonl(g_HqaRxStat.CCK_SIG_Err));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "CCK SIG Err B1", " = ",
			htonl(g_HqaRxStat.CCK_SIG_Err_Band1));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
	i4TotalLen - i4BytesWritten,
		"%-20s%s%d\n", "OFDM PD Cnt B0", " = ",
		htonl(g_HqaRxStat.OFDM_PD));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "OFDM PD Cnt B1", " = ",
			htonl(g_HqaRxStat.OFDM_PD_Band1));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "OFDM TAG Error", " = ",
			htonl(g_HqaRxStat.OFDM_TAG_Err));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "CCK SFD Err B0", " = ",
			htonl(g_HqaRxStat.CCK_SFD_Err));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "CCK SFD Err B1", " = ",
			htonl(g_HqaRxStat.CCK_SFD_Err_Band1));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "OFDM SIG Err B0", " = ",
			htonl(g_HqaRxStat.OFDM_SIG_Err));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "OFDM SIG Err B1", " = ",
			htonl(g_HqaRxStat.OFDM_SIG_Err_Band1));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "CCK FCS Err B0", " = ",
			htonl(g_HqaRxStat.FCSErr_CCK));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "CCK FCS Err B1", " = ",
			htonl(g_HqaRxStat.CCK_FCS_Err_Band1));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "OFDM FCS Err B0", " = ",
			htonl(g_HqaRxStat.FCSErr_OFDM));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "OFDM FCS Err B1", " = ",
			htonl(g_HqaRxStat.OFDM_FCS_Err_Band1));

	if (!fgSkipRxV) {
		u4FAGCRssiIBR0 = (au4RxV[2] & BITS(16, 23)) >> 16;
		u4FAGCRssiWBR0 = (au4RxV[2] & BITS(24, 31)) >> 24;

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "FAGC RSSI W", " = ",
			(u4FAGCRssiWBR0 >= 128) ? (u4FAGCRssiWBR0 - 256) :
				(u4FAGCRssiWBR0));

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "FAGC RSSI I", " = ",
			(u4FAGCRssiIBR0 >= 128) ? (u4FAGCRssiIBR0 - 256) :
				(u4FAGCRssiIBR0));
	} else{
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "FAGC RSSI W", " = ",
			htonl(g_HqaRxStat.FAGCRssiWBR0));

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "FAGC RSSI I", " = ",
			htonl(g_HqaRxStat.FAGCRssiIBR0));
	}

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "CCK MDRDY B0", " = ",
			htonl(g_HqaRxStat.PhyMdrdyCCK));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "CCK MDRDY B1", " = ",
			htonl(g_HqaRxStat.PHY_CCK_MDRDY_Band1));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "OFDM MDRDY B0", " = ",
			htonl(g_HqaRxStat.PhyMdrdyOFDM));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "OFDM MDRDY B1", " = ",
			htonl(g_HqaRxStat.PHY_OFDM_MDRDY_Band1));

	if (!fgSkipRxV) {
#if 0
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "Driver RX Cnt0", " = ",
			htonl(g_HqaRxStat.DriverRxCount));

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "Driver RX Cnt1", " = ",
			htonl(g_HqaRxStat.DriverRxCount1));
#endif
		u4Value = (au4RxV[0] & BITS(12, 14)) >> 12;
		if (u4Value == 0) {
			u4Foe = ((au4RxV[4] & BITS(7, 31)) >> 7) & 0x7ff;
			u4Foe = (u4Foe * 1000) >> 11;
		} else{
			cbw = (au4RxV[0] & BITS(15, 16)) >> 15;
			foe_const = ((1 << (cbw + 1)) & 0xf) * 10000;
			u4Foe = ((au4RxV[4] & BITS(7, 31)) >> 7) & 0xfff;
			u4Foe = (u4Foe * foe_const) >> 15;
		}

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "Freq Offset From RX", " = ", u4Foe);

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "RX SNR (dB)", " = ",
			(uint32_t)(((au4RxV[4] & BITS(26, 31)) >> 26) - 16));

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "uint8_t RX0", " = ",
			(uint32_t)(au4RxV[3] & BITS(0, 7)));

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "uint8_t RX1", " = ",
			(uint32_t)((au4RxV[3] & BITS(8, 15)) >> 8));

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "uint8_t RX2", " = ",
			((au4RxV[3] & BITS(16, 23)) >> 16) == 0xFF ?
			(0) : ((uint32_t)(au4RxV[3] & BITS(16, 23)) >> 16));

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "uint8_t RX3", " = ",
			((au4RxV[3] & BITS(24, 31)) >> 24) == 0xFF ?
			(0) : ((uint32_t)(au4RxV[3] & BITS(24, 31)) >> 24));
	} else{
#if 1
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
					      i4TotalLen - i4BytesWritten,
					      "%-20s%s%d\n", "Driver RX Cnt0",
					      " = ",
					      htonl(g_HqaRxStat.DriverRxCount));

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
					      i4TotalLen - i4BytesWritten,
					      "%-20s%s%d\n", "Driver RX Cnt1",
					      " = ",
					     htonl(g_HqaRxStat.DriverRxCount1));
#endif
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
					      i4TotalLen - i4BytesWritten,
					      "%-20s%s%d\n",
					      "Freq Offset From RX",
					      " = ",
					   htonl(g_HqaRxStat.FreqOffsetFromRX));

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
					      i4TotalLen - i4BytesWritten,
					      "%-20s%s%s\n", "RX SNR (dB)",
					      " = ", "N/A");

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
					      i4TotalLen - i4BytesWritten,
					      "%-20s%s%s\n", "uint8_t RX0",
					      " = ", "N/A");

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
					      i4TotalLen - i4BytesWritten,
					      "%-20s%s%s\n", "uint8_t RX1",
					      " = ", "N/A");

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
					      i4TotalLen - i4BytesWritten,
					      "%-20s%s%s\n", "uint8_t RX2",
					      " = ", "N/A");

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
					      i4TotalLen - i4BytesWritten,
					      "%-20s%s%s\n", "uint8_t RX3",
					      " = ", "N/A");
	}

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
				      i4TotalLen - i4BytesWritten,
				      "%-20s%s%d\n", "Inst RSSI IB R0", " = ",
				      htonl(g_HqaRxStat.InstRssiIBR0));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
				      i4TotalLen - i4BytesWritten,
				      "%-20s%s%d\n", "Inst RSSI WB R0", " = ",
				      htonl(g_HqaRxStat.InstRssiWBR0));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
				      i4TotalLen - i4BytesWritten,
				      "%-20s%s%d\n", "Inst RSSI IB R1", " = ",
				      htonl(g_HqaRxStat.InstRssiIBR1));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
				      i4TotalLen - i4BytesWritten,
				      "%-20s%s%d\n", "Inst RSSI WB R1", " = ",
				      htonl(g_HqaRxStat.InstRssiWBR1));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
				      i4TotalLen - i4BytesWritten,
				      "%-20s%s%d\n", "Inst RSSI IB R2", " = ",
				      htonl(g_HqaRxStat.InstRssiIBR2));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
				      i4TotalLen - i4BytesWritten,
				      "%-20s%s%d\n", "Inst RSSI WB R2", " = ",
				      htonl(g_HqaRxStat.InstRssiWBR2));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
				      i4TotalLen - i4BytesWritten,
				      "%-20s%s%d\n", "Inst RSSI IB R3", " = ",
				      htonl(g_HqaRxStat.InstRssiIBR3));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
				      i4TotalLen - i4BytesWritten,
				      "%-20s%s%d\n", "Inst RSSI WB R3", " = ",
				      htonl(g_HqaRxStat.InstRssiWBR3));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
				      i4TotalLen - i4BytesWritten,
				      "%-20s%s%d\n", "ACI Hit Lower", " = ",
				      htonl(g_HqaRxStat.ACIHitLower));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
				      i4TotalLen - i4BytesWritten,
				      "%-20s%s%d\n", "ACI Hit Higher",
				      " = ", htonl(g_HqaRxStat.ACIHitUpper));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
				      i4TotalLen - i4BytesWritten,
				      "%-20s%s%d\n", "OutOf Resource Pkt0",
				      " = ", au4OutOfResource[ENUM_BAND_0]);

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
				      i4TotalLen - i4BytesWritten,
				      "%-20s%s%d\n", "OutOf Resource Pkt1",
				      " = ", au4OutOfResource[ENUM_BAND_1]);

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
				      i4TotalLen - i4BytesWritten,
				      "%-20s%s%d\n", "Len Mismatch Cnt B0",
				      " = ", au4LengthMismatch[ENUM_BAND_0]);

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
				      i4TotalLen - i4BytesWritten,
				      "%-20s%s%d\n", "Len Mismatch Cnt B1",
				      " = ", au4LengthMismatch[ENUM_BAND_1]);
#if 0
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
				      i4TotalLen - i4BytesWritten,
				      "%-20s%s%d\n", "MU RX Cnt", " = ",
		htonl(g_HqaRxStat.MRURxCount));
#endif
	return i4BytesWritten;
}
#else
static int32_t priv_driver_dump_rx_stat_info_con3(struct ADAPTER *prAdapter,
					struct net_device *prNetDev,
					char *pcCommand, int i4TotalLen,
					u_int8_t fgResetCnt)
{
	int32_t i4BytesWritten = 0;
	uint32_t au4RxV[5] = {0};
	uint32_t *prRxV = NULL;
	uint8_t ucStaIdx, ucWlanIndex = 0, cbw;
	u_int8_t fgWlanIdxFound = TRUE, fgSkipRxV = FALSE;
	uint32_t u4FAGCRssiWBR0, u4FAGCRssiIBR0;
	uint32_t u4Value, u4Foe, foe_const;
	static uint32_t au4MacMdrdy[ENUM_BAND_NUM] = {0};
	static uint32_t au4FcsError[ENUM_BAND_NUM] = {0};
	static uint32_t au4OutOfResource[ENUM_BAND_NUM] = {0};
	static uint32_t au4LengthMismatch[ENUM_BAND_NUM] = {0};
	struct STA_RECORD *prStaRecOfAP;

	uint8_t ucBssIndex = wlanGetBssIdx(prNetDev);
	if (ucBssIndex >= MAX_BSSID_NUM) {
		DBGLOG(REQ, ERROR, "ucBssIndex out of range!\n");
		return -1;
	}

	prStaRecOfAP =
		aisGetStaRecOfAP(prAdapter, ucBssIndex);

	au4MacMdrdy[ENUM_BAND_0] += htonl(
		g_HqaRxStat.rInfoBand[0].u4MacRxMdrdyCnt);
	au4MacMdrdy[ENUM_BAND_1] += htonl(
		g_HqaRxStat.rInfoBand[1].u4MacRxMdrdyCnt);
	au4FcsError[ENUM_BAND_0] += htonl(
		g_HqaRxStat.rInfoBand[0].u4MacRxFcsErrCnt);
	au4FcsError[ENUM_BAND_1] += htonl(
		g_HqaRxStat.rInfoBand[1].u4MacRxFcsErrCnt);
	au4OutOfResource[ENUM_BAND_0] += 0;
	au4OutOfResource[ENUM_BAND_1] += 0;
	au4LengthMismatch[ENUM_BAND_0] += htonl(
		g_HqaRxStat.rInfoBand[0].u4MacRxLenMisMatch);
	au4LengthMismatch[ENUM_BAND_1] += htonl(
		g_HqaRxStat.rInfoBand[1].u4MacRxLenMisMatch);

	DBGLOG(INIT, INFO, "fgResetCnt = %d\n", fgResetCnt);
	if (fgResetCnt) {
		kalMemZero(au4MacMdrdy, sizeof(au4MacMdrdy));
		kalMemZero(au4FcsError, sizeof(au4FcsError));
		kalMemZero(au4OutOfResource, sizeof(au4OutOfResource));
		kalMemZero(au4LengthMismatch, sizeof(au4LengthMismatch));
	}

	if (prStaRecOfAP)
		ucWlanIndex =
			prStaRecOfAP->ucWlanIndex;
	else if (!wlanGetWlanIdxByAddress(prAdapter, NULL, &ucWlanIndex))
		fgWlanIdxFound = FALSE;

	if (fgWlanIdxFound) {
		if (wlanGetStaIdxByWlanIdx(prAdapter, ucWlanIndex, &ucStaIdx)
		    == WLAN_STATUS_SUCCESS) {
			prRxV = prAdapter->arStaRec[ucStaIdx].au4RxV;
			au4RxV[0] = prRxV[0];
			au4RxV[2] = prRxV[2];
			au4RxV[3] = prRxV[3];
			au4RxV[4] = prRxV[4];
		} else{
			fgSkipRxV = TRUE;
		}
	} else{
		fgSkipRxV = TRUE;
	}

	DBGLOG(INIT, INFO, "g_HqaRxStat.MAC_Mdrdy = %d\n",
		htonl(g_HqaRxStat.rInfoBand[0].u4MacRxMdrdyCnt));
	DBGLOG(INIT, INFO, "au4MacMdrdy[ENUM_BAND_0] = %d\n",
		au4MacMdrdy[ENUM_BAND_0]);
	DBGLOG(INIT, INFO, "g_HqaRxStat.PhyMdrdyOFDM = %d\n",
		htonl(g_HqaRxStat.rInfoBand[0].u4PhyRxMdrdyCntOfdm));
	DBGLOG(INIT, INFO, "g_HqaRxStat.MAC_FCS_Err= %d\n",
		htonl(g_HqaRxStat.rInfoBand[0].u4MacRxFcsErrCnt));
	DBGLOG(INIT, INFO, "au4FcsError[ENUM_BAND_0]= %d\n",
		au4FcsError[ENUM_BAND_0]);
	DBGLOG(INIT, INFO, "g_HqaRxStat.FCSErr_OFDM = %d\n",
		htonl(g_HqaRxStat.rInfoBand[0].u4PhyRxFcsErrCntOfdm));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%s", "\n\nRX Stat:\n");

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "PER0", " = ",
			0);

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "PER1", " = ",
			0);

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "RX OK0", " = ",
			au4MacMdrdy[ENUM_BAND_0] - au4FcsError[ENUM_BAND_0]);

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "RX OK1", " = ",
			au4MacMdrdy[ENUM_BAND_1] - au4FcsError[ENUM_BAND_1]);

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "RCPI0", " = ",
			htonl(g_HqaRxStat.rInfoRXV[0].u4Rcpi));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "RCPI1", " = ",
			htonl(g_HqaRxStat.rInfoRXV[1].u4Rcpi));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "MuRxCnt", " = ",
			htonl(g_HqaRxStat.rInfoCommExt1[0].u4MuRxCnt));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "MAC Mdrdy0 diff", " = ",
			htonl(g_HqaRxStat.rInfoBand[0].u4MacRxMdrdyCnt));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "MAC Mdrdy0", " = ",
			au4MacMdrdy[ENUM_BAND_0]);

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "MAC Mdrdy1 diff", " = ",
			htonl(g_HqaRxStat.rInfoBand[1].u4MacRxMdrdyCnt));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "MAC Mdrdy1", " = ",
			au4MacMdrdy[ENUM_BAND_1]);

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "FCS Err0", " = ",
			au4FcsError[ENUM_BAND_0]);

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "FCS Err1", " = ",
			au4FcsError[ENUM_BAND_1]);

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "CCK PD Cnt B0", " = ",
			htonl(g_HqaRxStat.rInfoBand[0].u4PhyRxPdCck));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "CCK PD Cnt B1", " = ",
			htonl(g_HqaRxStat.rInfoBand[1].u4PhyRxPdCck));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "CCK SIG Err B0", " = ",
			htonl(g_HqaRxStat.rInfoBand[0].u4PhyRxSigErrCck));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "CCK SIG Err B1", " = ",
			htonl(g_HqaRxStat.rInfoBand[1].u4PhyRxSigErrCck));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
	i4TotalLen - i4BytesWritten,
		"%-20s%s%d\n", "OFDM PD Cnt B0", " = ",
		htonl(g_HqaRxStat.rInfoBand[0].u4PhyRxPdOfdm));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "OFDM PD Cnt B1", " = ",
			htonl(g_HqaRxStat.rInfoBand[1].u4PhyRxPdOfdm));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "OFDM TAG Error", " = ",
			htonl(g_HqaRxStat.rInfoBand[0].u4PhyRxTagErrOfdm));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "CCK SFD Err B0", " = ",
			htonl(g_HqaRxStat.rInfoBand[0].u4PhyRxSfdErrCck));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "CCK SFD Err B1", " = ",
			htonl(g_HqaRxStat.rInfoBand[1].u4PhyRxSfdErrCck));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "OFDM SIG Err B0", " = ",
			htonl(g_HqaRxStat.rInfoBand[0].u4PhyRxSigErrOfdm));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "OFDM SIG Err B1", " = ",
			htonl(g_HqaRxStat.rInfoBand[1].u4PhyRxSigErrOfdm));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "CCK FCS Err B0", " = ",
			htonl(g_HqaRxStat.rInfoBand[0].u4PhyRxFcsErrCntCck));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "CCK FCS Err B1", " = ",
			htonl(g_HqaRxStat.rInfoBand[1].u4PhyRxFcsErrCntCck));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "OFDM FCS Err B0", " = ",
			htonl(g_HqaRxStat.rInfoBand[0].u4PhyRxFcsErrCntOfdm));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "OFDM FCS Err B1", " = ",
			htonl(g_HqaRxStat.rInfoBand[1].u4PhyRxFcsErrCntOfdm));

	if (!fgSkipRxV) {
		u4FAGCRssiIBR0 = (au4RxV[2] & BITS(16, 23)) >> 16;
		u4FAGCRssiWBR0 = (au4RxV[2] & BITS(24, 31)) >> 24;

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "FAGC RSSI W", " = ",
			(u4FAGCRssiWBR0 >= 128) ? (u4FAGCRssiWBR0 - 256) :
				(u4FAGCRssiWBR0));

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "FAGC RSSI I", " = ",
			(u4FAGCRssiIBR0 >= 128) ? (u4FAGCRssiIBR0 - 256) :
				(u4FAGCRssiIBR0));
	} else{
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "FAGC RSSI W", " = ",
			htonl(g_HqaRxStat.rInfoFagc[0].u4RssiWb));

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "FAGC RSSI I", " = ",
			htonl(g_HqaRxStat.rInfoFagc[0].u4RssiIb));
	}

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "CCK MDRDY B0", " = ",
			htonl(g_HqaRxStat.rInfoBand[0].u4PhyRxMdrdyCntCck));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "CCK MDRDY B1", " = ",
			htonl(g_HqaRxStat.rInfoBand[1].u4PhyRxMdrdyCntCck));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "OFDM MDRDY B0", " = ",
			htonl(g_HqaRxStat.rInfoBand[0].u4PhyRxMdrdyCntOfdm));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "OFDM MDRDY B1", " = ",
			htonl(g_HqaRxStat.rInfoBand[1].u4PhyRxMdrdyCntOfdm));

	if (!fgSkipRxV) {
		u4Value = (au4RxV[0] & BITS(12, 14)) >> 12;
		if (u4Value == 0) {
			u4Foe = ((au4RxV[4] & BITS(7, 31)) >> 7) & 0x7ff;
			u4Foe = (u4Foe * 1000)>>11;
		} else{
			cbw = (au4RxV[0] & BITS(15, 16)) >> 15;
			foe_const = ((1 << (cbw + 1)) & 0xf) * 10000;
			u4Foe = ((au4RxV[4] & BITS(7, 31)) >> 7) & 0xfff;
			u4Foe = (u4Foe * foe_const) >> 15;
		}

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "Freq Offset From RX", " = ", u4Foe);

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "RX SNR (dB)", " = ",
			(uint32_t)(((au4RxV[4] & BITS(26, 31)) >> 26) - 16));

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "uint8_t RX0", " = ",
			(uint32_t)(au4RxV[3] & BITS(0, 7)));

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "uint8_t RX1", " = ",
			(uint32_t)((au4RxV[3] & BITS(8, 15)) >> 8));

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "uint8_t RX2", " = ",
			((au4RxV[3] & BITS(16, 23)) >> 16) == 0xFF ?
			(0) : ((uint32_t)(au4RxV[3] & BITS(16, 23)) >> 16));

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "uint8_t RX3", " = ",
			((au4RxV[3] & BITS(24, 31)) >> 24) == 0xFF ?
			(0) : ((uint32_t)(au4RxV[3] & BITS(24, 31)) >> 24));
	} else{

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "Driver RX Cnt0",
			" = ",
			htonl(g_HqaRxStat.rInfoCommExt1[0].u4DrvRxCnt));

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "Driver RX Cnt1",
			" = ",
			htonl(g_HqaRxStat.rInfoCommExt1[1].u4DrvRxCnt));

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n",
			  "Freq Offset From RX",
			  " = ",
			htonl(g_HqaRxStat.rInfoUser[0].u4FreqOffsetFromRx));

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
					      i4TotalLen - i4BytesWritten,
					      "%-20s%s%s\n", "RX SNR (dB)",
					      " = ", "N/A");

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
					      i4TotalLen - i4BytesWritten,
					      "%-20s%s%s\n", "uint8_t RX0",
					      " = ", "N/A");

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
					      i4TotalLen - i4BytesWritten,
					      "%-20s%s%s\n", "uint8_t RX1",
					      " = ", "N/A");

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
					      i4TotalLen - i4BytesWritten,
					      "%-20s%s%s\n", "uint8_t RX2",
					      " = ", "N/A");

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
					      i4TotalLen - i4BytesWritten,
					      "%-20s%s%s\n", "uint8_t RX3",
					      " = ", "N/A");
	}

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
	      i4TotalLen - i4BytesWritten,
	      "%-20s%s%d\n", "Inst RSSI IB R0", " = ",
	      htonl(g_HqaRxStat.rInfoInst[0].u4RssiIb));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
	      i4TotalLen - i4BytesWritten,
	      "%-20s%s%d\n", "Inst RSSI WB R0", " = ",
	      htonl(g_HqaRxStat.rInfoInst[0].u4RssiWb));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
	      i4TotalLen - i4BytesWritten,
	      "%-20s%s%d\n", "Inst RSSI IB R1", " = ",
	      htonl(g_HqaRxStat.rInfoInst[1].u4RssiIb));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
	      i4TotalLen - i4BytesWritten,
	      "%-20s%s%d\n", "Inst RSSI WB R1", " = ",
	      htonl(g_HqaRxStat.rInfoInst[1].u4RssiWb));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
	      i4TotalLen - i4BytesWritten,
	      "%-20s%s%d\n", "Inst RSSI IB R2", " = ",
	      htonl(g_HqaRxStat.rInfoInst[2].u4RssiIb));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
	      i4TotalLen - i4BytesWritten,
	      "%-20s%s%d\n", "Inst RSSI WB R2", " = ",
	      htonl(g_HqaRxStat.rInfoInst[2].u4RssiWb));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
	      i4TotalLen - i4BytesWritten,
	      "%-20s%s%d\n", "Inst RSSI IB R3", " = ",
	      htonl(g_HqaRxStat.rInfoInst[3].u4RssiIb));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
	      i4TotalLen - i4BytesWritten,
	      "%-20s%s%d\n", "Inst RSSI WB R3", " = ",
	      htonl(g_HqaRxStat.rInfoInst[3].u4RssiWb));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
	      i4TotalLen - i4BytesWritten,
	      "%-20s%s%d\n", "ACI Hit Lower", " = ",
	      htonl(g_HqaRxStat.rInfoComm[0].u4AciHitLow));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
	      i4TotalLen - i4BytesWritten,
	      "%-20s%s%d\n", "ACI Hit Higher",
	      " = ", htonl(g_HqaRxStat.rInfoComm[0].u4AciHitHigh));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
	      i4TotalLen - i4BytesWritten,
	      "%-20s%s%d\n", "OutOf Resource Pkt0",
	      " = ", au4OutOfResource[ENUM_BAND_0]);

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
	      i4TotalLen - i4BytesWritten,
	      "%-20s%s%d\n", "OutOf Resource Pkt1",
	      " = ", au4OutOfResource[ENUM_BAND_1]);

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
	      i4TotalLen - i4BytesWritten,
	      "%-20s%s%d\n", "Len Mismatch Cnt B0",
	      " = ", au4LengthMismatch[ENUM_BAND_0]);

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
	      i4TotalLen - i4BytesWritten,
	      "%-20s%s%d\n", "Len Mismatch Cnt B1",
	      " = ", au4LengthMismatch[ENUM_BAND_1]);

	return i4BytesWritten;
}
#endif

int priv_driver_show_rx_stat(struct net_device *prNetDev,
				    char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	struct PARAM_CUSTOM_ACCESS_RX_STAT *prRxStatisticsTest;
	u_int8_t fgResetCnt = FALSE;
	struct PARAM_CUSTOM_SW_CTRL_STRUCT rSwCtrlInfo;
	uint32_t u4Id = 0x99980000;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	DBGLOG(INIT, ERROR,
		"MT6632 : priv_driver_show_rx_stat %s, i4Argc[%d]\n",
		pcCommand, i4Argc);

	if (i4Argc >= 2) {
		if (strnicmp(apcArgv[1], CMD_STAT_RESET_CNT,
		    strlen(CMD_STAT_RESET_CNT)) == 0)
			fgResetCnt = TRUE;
	}

	if (i4Argc >= 1) {
		if (fgResetCnt) {
			rSwCtrlInfo.u4Id = u4Id;
			rSwCtrlInfo.u4Data = 0;

			rStatus = kalIoctl(prGlueInfo, wlanoidSetSwCtrlWrite,
					   &rSwCtrlInfo, sizeof(rSwCtrlInfo),
					   &u4BufLen);

			if (rStatus != WLAN_STATUS_SUCCESS)
				return -1;
		}

		prRxStatisticsTest =
			(struct PARAM_CUSTOM_ACCESS_RX_STAT *)kalMemAlloc(
			sizeof(struct PARAM_CUSTOM_ACCESS_RX_STAT),
			VIR_MEM_TYPE);
		if (!prRxStatisticsTest)
			return -1;
#if (CFG_SUPPORT_CONNAC3X == 0)
		prRxStatisticsTest->u4SeqNum = u4RxStatSeqNum;
#else
		prRxStatisticsTest->u2SeqNum = u2RxStatSeqNum;
#endif
		prRxStatisticsTest->u4TotalNum =
					sizeof(struct PARAM_RX_STAT) / 4;

		rStatus = kalIoctl(prGlueInfo, wlanoidQueryRxStatistics,
				   prRxStatisticsTest,
				   sizeof(struct PARAM_CUSTOM_ACCESS_RX_STAT),
				   &u4BufLen);

		if (rStatus != WLAN_STATUS_SUCCESS) {
			kalMemFree(prRxStatisticsTest, VIR_MEM_TYPE,
				   sizeof(struct PARAM_CUSTOM_ACCESS_RX_STAT));
			return -1;
		}

#if (CFG_SUPPORT_CONNAC3X == 0)
		i4BytesWritten = priv_driver_dump_rx_stat_info(prAdapter,
			prNetDev, pcCommand, i4TotalLen, fgResetCnt);
#else
		i4BytesWritten = priv_driver_dump_rx_stat_info_con3(prAdapter,
			prNetDev, pcCommand, i4TotalLen, fgResetCnt);
#endif

		kalMemFree(prRxStatisticsTest, VIR_MEM_TYPE,
			   sizeof(struct PARAM_CUSTOM_ACCESS_RX_STAT));
	}

	return i4BytesWritten;
}
#endif

#if CFG_ENABLE_WIFI_DIRECT
/*----------------------------------------------------------------------------*/
/*
 * @ The function will set policy of ACL.
 *  0: disable ACL
 *  1: enable accept list
 *  2: enable deny list
 * example: iwpriv p2p0 driver "set_acl_policy 1"
 */
/*----------------------------------------------------------------------------*/
int priv_driver_set_acl_policy(struct net_device *prNetDev,
				      char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	struct BSS_INFO *prBssInfo = NULL;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	int32_t i4Argc = 0, i4BytesWritten = 0, i4Ret = 0, i4Policy = 0;
	uint8_t ucRoleIdx = 0, ucBssIdx = 0;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	/* get Bss Index from ndev */
	if (mtk_Netdev_To_RoleIdx(prGlueInfo, prNetDev, &ucRoleIdx) != 0)
		return -1;
	if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter, ucRoleIdx, &ucBssIdx) !=
		WLAN_STATUS_SUCCESS)
		return -1;

	prBssInfo = prAdapter->aprBssInfo[ucBssIdx];

	DBGLOG(REQ, LOUD, "ucRoleIdx %hhu ucBssIdx %hhu\n", ucRoleIdx,
	       ucBssIdx);
	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc < 2)
		return -1;

	i4Ret = kalkStrtou32(apcArgv[1], 0, &i4Policy);
	if (i4Ret) {
		DBGLOG(REQ, ERROR, "integer format error i4Ret=%d\n", i4Ret);
		return -1;
	}

	switch (i4Policy) {
	case PARAM_CUSTOM_ACL_POLICY_DISABLE:
	case PARAM_CUSTOM_ACL_POLICY_ACCEPT:
	case PARAM_CUSTOM_ACL_POLICY_DENY:
		prBssInfo->rACL.ePolicy = i4Policy;
		break;
	default: /*Invalid argument */
		DBGLOG(REQ, ERROR, "Invalid ACL Policy=%d\n", i4Policy);
		return -1;
	}

	DBGLOG(REQ, TRACE, "ucBssIdx[%hhu] ACL Policy=%d\n", ucBssIdx,
	       prBssInfo->rACL.ePolicy);

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
				      i4TotalLen - i4BytesWritten,
				      "ucBssIdx[%hhu] ACL Policy=%d\n",
				      ucBssIdx, prBssInfo->rACL.ePolicy);

	/* check if the change in ACL affects any existent association */
	if (prBssInfo->rACL.ePolicy != PARAM_CUSTOM_ACL_POLICY_DISABLE)
		p2pRoleUpdateACLEntry(prAdapter, ucBssIdx);

	DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__, pcCommand);

	p2pFuncSetAclPolicy(prAdapter,
		ucBssIdx,
		prBssInfo->rACL.ePolicy,
		NULL);

	return i4BytesWritten;
} /* priv_driver_set_acl_policy */

static int32_t priv_driver_inspect_mac_addr(char *pcMacAddr)
{
	int32_t i = 0;

	if (pcMacAddr == NULL)
		return -1;

	for (i = 0; i < 17; i++) {
		if ((i % 3 != 2) && (!kalIsXdigit(pcMacAddr[i]))) {
			DBGLOG(REQ, ERROR, "[%c] is not hex digit\n",
			       pcMacAddr[i]);
			return -1;
		}
		if ((i % 3 == 2) && (pcMacAddr[i] != ':')) {
			DBGLOG(REQ, ERROR, "[%c]separate symbol is error\n",
			       pcMacAddr[i]);
			return -1;
		}
	}

	if (pcMacAddr[17] != '\0') {
		DBGLOG(REQ, ERROR, "no null-terminated character\n");
		return -1;
	}

	return 0;
}

/*----------------------------------------------------------------------------*/
/*
 * @ The function will add entry to ACL for accept or deny list.
 *  example: iwpriv p2p0 driver "add_acl_entry 01:02:03:04:05:06"
 */
/*----------------------------------------------------------------------------*/
int priv_driver_add_acl_entry(struct net_device *prNetDev,
				     char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	struct BSS_INFO *prBssInfo = NULL;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint8_t aucMacAddr[MAC_ADDR_LEN] = {0};
	int32_t i = 0, i4Argc = 0, i4BytesWritten = 0, i4Ret = 0;
	uint8_t ucRoleIdx = 0, ucBssIdx = 0;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	/* get Bss Index from ndev */
	if (mtk_Netdev_To_RoleIdx(prGlueInfo, prNetDev, &ucRoleIdx) != 0)
		return -1;
	if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter, ucRoleIdx, &ucBssIdx) !=
		WLAN_STATUS_SUCCESS)
		return -1;

	prBssInfo = prAdapter->aprBssInfo[ucBssIdx];

	DBGLOG(REQ, LOUD, "ucRoleIdx %hhu ucBssIdx %hhu\n", ucRoleIdx,
	       ucBssIdx);
	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc < 2)
		return -1;

	i4Ret = priv_driver_inspect_mac_addr(apcArgv[1]);
	if (i4Ret) {
		DBGLOG(REQ, ERROR, "inspect mac format error u4Ret=%d\n",
		       i4Ret);
		return -1;
	}

	i4Ret = sscanf(apcArgv[1], "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
		&aucMacAddr[0], &aucMacAddr[1], &aucMacAddr[2],
		&aucMacAddr[3], &aucMacAddr[4], &aucMacAddr[5]);

	if (i4Ret != MAC_ADDR_LEN) {
		DBGLOG(REQ, ERROR, "sscanf mac format fail u4Ret=%d\n", i4Ret);
		return -1;
	}

	for (i = 1; i <= prBssInfo->rACL.u4Num; i++) {
		if (memcmp(prBssInfo->rACL.rEntry[i-1].aucAddr, &aucMacAddr,
		    MAC_ADDR_LEN) == 0) {
			DBGLOG(REQ, ERROR, "add this mac [" MACSTR
			       "] is duplicate.\n", MAC2STR(aucMacAddr));
			return -1;
		}
	}

	if (i > MAX_NUMBER_OF_ACL) {
		DBGLOG(REQ, ERROR, "idx[%d] error or ACL is full.\n", i);
		return -1;
	}

	memcpy(prBssInfo->rACL.rEntry[i-1].aucAddr, &aucMacAddr, MAC_ADDR_LEN);
	prBssInfo->rACL.u4Num = i;
	DBGLOG(REQ, TRACE, "add mac addr [" MACSTR "] to ACL(%d).\n",
		MAC2STR(prBssInfo->rACL.rEntry[i-1].aucAddr), i);

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"add mac addr [" MACSTR "] to ACL(%d)\n",
				MAC2STR(prBssInfo->rACL.rEntry[i-1].aucAddr),
				i);

	/* Check if the change in ACL affects any existent association. */
	if (prBssInfo->rACL.ePolicy == PARAM_CUSTOM_ACL_POLICY_DENY)
		p2pRoleUpdateACLEntry(prAdapter, ucBssIdx);

	DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__, pcCommand);

	p2pFuncSetAclPolicy(prAdapter,
		ucBssIdx,
		PARAM_CUSTOM_ACL_POLICY_ADD,
		aucMacAddr);

	return i4BytesWritten;
} /* priv_driver_add_acl_entry */

/*----------------------------------------------------------------------------*/
/*
 * @ The function will delete entry to ACL for accept or deny list.
 *  example: iwpriv p2p0 driver "add_del_entry 01:02:03:04:05:06"
 */
/*----------------------------------------------------------------------------*/
int priv_driver_del_acl_entry(struct net_device *prNetDev,
				     char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	struct BSS_INFO *prBssInfo = NULL;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint8_t aucMacAddr[MAC_ADDR_LEN] = {0};
	int32_t i = 0, j = 0, i4Argc = 0, i4BytesWritten = 0, i4Ret = 0;
	uint8_t ucRoleIdx = 0, ucBssIdx = 0;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	/* get Bss Index from ndev */
	if (mtk_Netdev_To_RoleIdx(prGlueInfo, prNetDev, &ucRoleIdx) != 0)
		return -1;
	if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter, ucRoleIdx, &ucBssIdx) !=
		WLAN_STATUS_SUCCESS)
		return -1;

	prBssInfo = prAdapter->aprBssInfo[ucBssIdx];

	DBGLOG(REQ, LOUD, "ucRoleIdx %hhu ucBssIdx %hhu\n", ucRoleIdx,
	       ucBssIdx);
	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc < 2)
		return -1;

	i4Ret = priv_driver_inspect_mac_addr(apcArgv[1]);
	if (i4Ret) {
		DBGLOG(REQ, ERROR, "inspect mac format error u4Ret=%d\n",
		       i4Ret);
		return -1;
	}

	i4Ret = sscanf(apcArgv[1], "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
		&aucMacAddr[0], &aucMacAddr[1], &aucMacAddr[2],
		&aucMacAddr[3], &aucMacAddr[4], &aucMacAddr[5]);

	if (i4Ret != MAC_ADDR_LEN) {
		DBGLOG(REQ, ERROR, "sscanf mac format fail u4Ret=%d\n", i4Ret);
		return -1;
	}

	for (i = 0; i < prBssInfo->rACL.u4Num; i++) {
		if (memcmp(prBssInfo->rACL.rEntry[i].aucAddr, &aucMacAddr,
		    MAC_ADDR_LEN) == 0) {
			memset(&prBssInfo->rACL.rEntry[i], 0x00,
			       sizeof(struct PARAM_CUSTOM_ACL_ENTRY));
			DBGLOG(REQ, TRACE, "delete this mac [" MACSTR "]\n",
			       MAC2STR(aucMacAddr));

			i4BytesWritten += kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"delete this mac [" MACSTR "] from ACL(%d)\n",
				MAC2STR(aucMacAddr), i+1);
			break;
		}
	}

	if ((prBssInfo->rACL.u4Num == 0) || (i == MAX_NUMBER_OF_ACL)) {
		DBGLOG(REQ, ERROR, "delete entry fail, num of entries=%d\n", i);
		return -1;
	}

	for (j = i+1; j < prBssInfo->rACL.u4Num; j++)
		memcpy(prBssInfo->rACL.rEntry[j-1].aucAddr,
		       prBssInfo->rACL.rEntry[j].aucAddr, MAC_ADDR_LEN);

	prBssInfo->rACL.u4Num = j-1;
	memset(prBssInfo->rACL.rEntry[j-1].aucAddr, 0x00, MAC_ADDR_LEN);

	/* check if the change in ACL affects any existent association */
	if (prBssInfo->rACL.ePolicy == PARAM_CUSTOM_ACL_POLICY_ACCEPT)
		p2pRoleUpdateACLEntry(prAdapter, ucBssIdx);

	DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__, pcCommand);

	p2pFuncSetAclPolicy(prAdapter,
		ucBssIdx,
		PARAM_CUSTOM_ACL_POLICY_REMOVE,
		aucMacAddr);

	return i4BytesWritten;
} /* priv_driver_del_acl_entry */

/*----------------------------------------------------------------------------*/
/*
 * @ The function will show all entries to ACL for accept or deny list.
 *  example: iwpriv p2p0 driver "show_acl_entry"
 */
/*----------------------------------------------------------------------------*/
int priv_driver_show_acl_entry(struct net_device *prNetDev,
				      char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	struct BSS_INFO *prBssInfo = NULL;
	int32_t i = 0, i4BytesWritten = 0;
	uint8_t ucRoleIdx = 0, ucBssIdx = 0;

	ASSERT(prNetDev);
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	/* get Bss Index from ndev */
	if (mtk_Netdev_To_RoleIdx(prGlueInfo, prNetDev, &ucRoleIdx) != 0)
		return -1;
	if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter, ucRoleIdx, &ucBssIdx) !=
	    WLAN_STATUS_SUCCESS)
		return -1;

	prBssInfo = prAdapter->aprBssInfo[ucBssIdx];

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	DBGLOG(REQ, TRACE, "ACL Policy = %d\n", prBssInfo->rACL.ePolicy);
	DBGLOG(REQ, TRACE, "Total ACLs = %d\n", prBssInfo->rACL.u4Num);

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
				      i4TotalLen - i4BytesWritten,
				      "ACL Policy = %d, Total ACLs = %d\n",
				      prBssInfo->rACL.ePolicy,
				      prBssInfo->rACL.u4Num);

	for (i = 0; i < prBssInfo->rACL.u4Num; i++) {
		DBGLOG(REQ, TRACE, "ACL(%d): [" MACSTR "]\n", i+1,
		       MAC2STR(prBssInfo->rACL.rEntry[i].aucAddr));

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten, "ACL(%d): [" MACSTR
			"]\n", i+1, MAC2STR(prBssInfo->rACL.rEntry[i].aucAddr));
	}

	return i4BytesWritten;
} /* priv_driver_show_acl_entry */

/*----------------------------------------------------------------------------*/
/*
 * @ The function will clear all entries to ACL for accept or deny list.
 *  example: iwpriv p2p0 driver "clear_acl_entry"
 */
/*----------------------------------------------------------------------------*/
int priv_driver_clear_acl_entry(struct net_device *prNetDev,
				       char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	struct BSS_INFO *prBssInfo = NULL;
	int32_t i4BytesWritten = 0;
	uint8_t ucRoleIdx = 0, ucBssIdx = 0;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	/* get Bss Index from ndev */
	if (mtk_Netdev_To_RoleIdx(prGlueInfo, prNetDev, &ucRoleIdx) != 0)
		return -1;
	if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter, ucRoleIdx, &ucBssIdx) !=
		WLAN_STATUS_SUCCESS)
		return -1;

	prBssInfo = prAdapter->aprBssInfo[ucBssIdx];

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);

	if (prBssInfo->rACL.u4Num) {
		memset(&prBssInfo->rACL.rEntry[0], 0x00,
		       sizeof(struct PARAM_CUSTOM_ACL_ENTRY) * MAC_ADDR_LEN);
		prBssInfo->rACL.u4Num = 0;
	}

	DBGLOG(REQ, TRACE, "ACL Policy = %d\n", prBssInfo->rACL.ePolicy);
	DBGLOG(REQ, TRACE, "Total ACLs = %d\n", prBssInfo->rACL.u4Num);

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
				      i4TotalLen - i4BytesWritten,
				      "ACL Policy = %d, Total ACLs = %d\n",
				      prBssInfo->rACL.ePolicy,
				      prBssInfo->rACL.u4Num);

	/* check if the change in ACL affects any existent association */
	if (prBssInfo->rACL.ePolicy == PARAM_CUSTOM_ACL_POLICY_ACCEPT)
		p2pRoleUpdateACLEntry(prAdapter, ucBssIdx);

	p2pFuncSetAclPolicy(prAdapter,
		ucBssIdx,
		PARAM_CUSTOM_ACL_POLICY_CLEAR,
		NULL);

	return i4BytesWritten;
} /* priv_driver_clear_acl_entry */

#endif /* CFG_ENABLE_WIFI_DIRECT */

int priv_driver_get_drv_mcr(struct net_device *prNetDev,
				   char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret;

	/* Add Antenna Selection Input */
	/* INT_32 i4ArgNum_with_ant_sel = 3; */

	int32_t i4ArgNum = 2;

	struct CMD_ACCESS_REG rCmdAccessReg;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc >= i4ArgNum) {
		rCmdAccessReg.u4Address = 0;
		u4Ret = kalkStrtou32(apcArgv[1], 0, &(rCmdAccessReg.u4Address));
		if (u4Ret)
			DBGLOG(REQ, LOUD,
			       "parse get_drv_mcr error (Address) u4Ret=%d\n",
			       u4Ret);

		/* rCmdAccessReg.u4Address = kalStrtoul(apcArgv[1], NULL, 0); */
		rCmdAccessReg.u4Data = 0;

		DBGLOG(REQ, LOUD, "address is %x\n", rCmdAccessReg.u4Address);

		rStatus = kalIoctl(prGlueInfo, wlanoidQueryDrvMcrRead,
				   &rCmdAccessReg, sizeof(rCmdAccessReg),
				   &u4BufLen);

		DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
		if (rStatus != WLAN_STATUS_SUCCESS)
			return -1;

		i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen, "0x%08x",
					  (unsigned int)rCmdAccessReg.u4Data);
		DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__,
		       pcCommand);
	}

	return i4BytesWritten;

}				/* priv_driver_get_drv_mcr */

int priv_driver_set_drv_mcr(struct net_device *prNetDev, char *pcCommand,
			    int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	uint32_t u4Ret;

	/* Add Antenna Selection Input */
	/* INT_32 i4ArgNum_with_ant_sel = 4; */

	int32_t i4ArgNum = 3;

	struct CMD_ACCESS_REG rCmdAccessReg;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc >= i4ArgNum) {
		u4Ret = kalkStrtou32(apcArgv[1], 0, &(rCmdAccessReg.u4Address));
		if (u4Ret)
			DBGLOG(REQ, LOUD,
			       "parse get_drv_mcr error (Address) u4Ret=%d\n",
			       u4Ret);

		u4Ret = kalkStrtou32(apcArgv[2], 0, &(rCmdAccessReg.u4Data));
		if (u4Ret)
			DBGLOG(REQ, LOUD,
			       "parse get_drv_mcr error (Data) u4Ret=%d\n",
			       u4Ret);

		rStatus = kalIoctl(prGlueInfo, wlanoidSetDrvMcrWrite,
				   &rCmdAccessReg, sizeof(rCmdAccessReg),
				   &u4BufLen);

		if (rStatus != WLAN_STATUS_SUCCESS)
			return -1;

	}

	return i4BytesWritten;

}

#if CFG_MTK_WIFI_SW_EMI_RING
int priv_driver_get_emi_mcr(struct net_device *prNetDev,
			    char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret;

	/* Add Antenna Selection Input */
	/* INT_32 i4ArgNum_with_ant_sel = 3; */

	int32_t i4ArgNum = 2;

	struct CMD_ACCESS_REG rCmdAccessReg;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc >= i4ArgNum) {
		rCmdAccessReg.u4Address = 0;
		u4Ret = kalkStrtou32(apcArgv[1], 0, &(rCmdAccessReg.u4Address));
		if (u4Ret)
			DBGLOG(REQ, LOUD,
			       "parse get_drv_mcr error (Address) u4Ret=%d\n",
			       u4Ret);

		/* rCmdAccessReg.u4Address = kalStrtoul(apcArgv[1], NULL, 0); */
		rCmdAccessReg.u4Data = 0;

		DBGLOG(REQ, LOUD, "address is %x\n", rCmdAccessReg.u4Address);

		rStatus = kalIoctl(prGlueInfo, wlanoidQueryEmiMcrRead,
				   &rCmdAccessReg, sizeof(rCmdAccessReg),
				   &u4BufLen);

		DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
		if (rStatus != WLAN_STATUS_SUCCESS)
			return -1;

		i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen, "0x%08x",
					  (unsigned int)rCmdAccessReg.u4Data);
		DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__,
		       pcCommand);
	}

	return i4BytesWritten;
}
#endif /* CFG_MTK_WIFI_SW_EMI_RING */

int priv_driver_get_uhw_mcr(struct net_device *prNetDev,
				   char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret;

	/* Add Antenna Selection Input */
	/* INT_32 i4ArgNum_with_ant_sel = 3; */

	int32_t i4ArgNum = 2;

	struct CMD_ACCESS_REG rCmdAccessReg;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc >= i4ArgNum) {

		u4Ret = kalkStrtou32(apcArgv[1], 0, &(rCmdAccessReg.u4Address));
		if (u4Ret)
			DBGLOG(REQ, LOUD,
			       "parse get_drv_mcr error (Address) u4Ret=%d\n",
			       u4Ret);

		/* rCmdAccessReg.u4Address = kalStrtoul(apcArgv[1], NULL, 0); */
		rCmdAccessReg.u4Data = 0;

		DBGLOG(REQ, LOUD, "address is %x\n", rCmdAccessReg.u4Address);

		rStatus = kalIoctl(prGlueInfo, wlanoidQueryUhwMcrRead,
				   &rCmdAccessReg, sizeof(rCmdAccessReg),
				   &u4BufLen);

		DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);

		if (rStatus != WLAN_STATUS_SUCCESS)
			i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen,
						  "IO FAIL");
		else
			i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen,
						  "0x%08x",
					    (unsigned int)rCmdAccessReg.u4Data);

		DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__,
		       pcCommand);
	}

	return i4BytesWritten;

}

int priv_driver_set_uhw_mcr(struct net_device *prNetDev, char *pcCommand,
			    int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	uint32_t u4Ret;

	/* Add Antenna Selection Input */
	/* INT_32 i4ArgNum_with_ant_sel = 4; */

	int32_t i4ArgNum = 3;

	struct CMD_ACCESS_REG rCmdAccessReg;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc >= i4ArgNum) {
		u4Ret = kalkStrtou32(apcArgv[1], 0, &(rCmdAccessReg.u4Address));
		if (u4Ret)
			DBGLOG(REQ, LOUD,
			       "parse get_drv_mcr error (Address) u4Ret=%d\n",
			       u4Ret);

		u4Ret = kalkStrtou32(apcArgv[2], 0, &(rCmdAccessReg.u4Data));
		if (u4Ret)
			DBGLOG(REQ, LOUD,
			       "parse get_drv_mcr error (Data) u4Ret=%d\n",
			       u4Ret);

		rStatus = kalIoctl(prGlueInfo, wlanoidSetUhwMcrWrite,
				   &rCmdAccessReg, sizeof(rCmdAccessReg),
				   &u4BufLen);

		if (rStatus != WLAN_STATUS_SUCCESS)
			i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen,
						  "IO FAIL");
	}

	return i4BytesWritten;

}

int priv_driver_get_sw_ctrl(struct net_device *prNetDev,
				   char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	int32_t u4Ret = 0;

	struct PARAM_CUSTOM_SW_CTRL_STRUCT rSwCtrlInfo;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc >= 2) {
		rSwCtrlInfo.u4Id = 0;
		rSwCtrlInfo.u4Data = 0;
		u4Ret = kalkStrtou32(apcArgv[1], 0, &(rSwCtrlInfo.u4Id));
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse rSwCtrlInfo error u4Ret=%d\n",
			       u4Ret);

		DBGLOG(REQ, LOUD, "id is %x\n", rSwCtrlInfo.u4Id);

		rStatus = kalIoctl(prGlueInfo, wlanoidQuerySwCtrlRead,
				   &rSwCtrlInfo, sizeof(rSwCtrlInfo),
				   &u4BufLen);

		DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
		if (rStatus != WLAN_STATUS_SUCCESS)
			return -1;

		i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen, "0x%08x",
					  (unsigned int)rSwCtrlInfo.u4Data);
		DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__,
		       pcCommand);
	}

	return i4BytesWritten;

}				/* priv_driver_get_sw_ctrl */

int priv_driver_set_sw_ctrl(struct net_device *prNetDev, char *pcCommand,
			    int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	int32_t u4Ret = 0;

	struct PARAM_CUSTOM_SW_CTRL_STRUCT rSwCtrlInfo;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	u4Ret = kalkStrtou32(apcArgv[1], 0, &(rSwCtrlInfo.u4Id));
	if (u4Ret)
		DBGLOG(REQ, LOUD, "parse rSwCtrlInfo error u4Ret=%d\n",
		       u4Ret);
	u4Ret = kalkStrtou32(apcArgv[2], 0, &(rSwCtrlInfo.u4Data));
	if (u4Ret)
		DBGLOG(REQ, LOUD, "parse rSwCtrlInfo error u4Ret=%d\n",
		       u4Ret);

	rStatus = kalIoctl(prGlueInfo, wlanoidSetSwCtrlWrite,
			   &rSwCtrlInfo, sizeof(rSwCtrlInfo),
			   &u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS)
		return -1;

	return i4BytesWritten;

}				/* priv_driver_set_sw_ctrl */

int priv_driver_boostcpu(struct net_device *prNetDev,
				  char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	int32_t i4Recv = 0;
	int8_t *this_char = NULL;
	struct BOOST_INFO rBoostInfo;
	uint32_t u4SkbAllocWorkCoreType = 0;
	uint32_t u4TxFreeSkbWorkCoreType = 0;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %d, apcArgv[0] = %s\n\n", i4Argc, *apcArgv);

	this_char = kalStrStr(*apcArgv, "=");
	if (!this_char)
		return -1;
	this_char++;

	kalMemZero(&rBoostInfo, sizeof(struct BOOST_INFO));
	i4Recv = sscanf(this_char,
		"%d-%d-%02x-%02x-%02x-%02x-%02x-%u-%u-%u-%u-%u-%x-%x-%d-%d-%d-%d-%d-%d-%u-%d-%u-%u-%u",
		&(rBoostInfo.rCpuInfo.i4LittleCpuFreq),
		&(rBoostInfo.rCpuInfo.i4BigCpuFreq),
		&(rBoostInfo.rHifThreadInfo.u4CpuMask),
		&(rBoostInfo.rMainThreadInfo.u4CpuMask),
		&(rBoostInfo.rRxThreadInfo.u4CpuMask),
		&(rBoostInfo.rRxNapiThreadInfo.u4CpuMask),
		&(rBoostInfo.rHifNapiThreadInfo.u4CpuMask),
		&(rBoostInfo.rHifThreadInfo.u4Priority),
		&(rBoostInfo.rMainThreadInfo.u4Priority),
		&(rBoostInfo.rRxThreadInfo.u4Priority),
		&(rBoostInfo.rRxNapiThreadInfo.u4Priority),
		&(rBoostInfo.rHifNapiThreadInfo.u4Priority),
		&(rBoostInfo.u4RpsMap),
		&(rBoostInfo.u4ISRMask),
		&(rBoostInfo.i4DramBoostLv),
		&(rBoostInfo.i4TxFreeMsduWorkCpu),
		&(rBoostInfo.i4RxRfbRetWorkCpu),
		&(rBoostInfo.i4TxWorkCpu),
		&(rBoostInfo.i4RxWorkCpu),
		&(rBoostInfo.fgKeepPcieWakeup),
		&(rBoostInfo.u4WfdmaTh),
		&(rBoostInfo.i4RxNapiWorkCpu),
		&(rBoostInfo.fgWifiNappingForceDis),
		&u4SkbAllocWorkCoreType,
		&u4TxFreeSkbWorkCoreType
		);

	if (i4Recv == 25) {
		/* Disable BoostCpu by PerMon */
		prAdapter->rWifiVar.fgBoostCpuEn = FEATURE_DISABLED;
		prAdapter->rWifiVar.fgBoostCpuPolicyEn
			= FEATURE_DISABLED;

		/* Manually BoostCpu */
		if (rBoostInfo.rCpuInfo.i4LittleCpuFreq != 0)
			rBoostInfo.rCpuInfo.i4LittleCpuFreq *= 1000;
		else
			rBoostInfo.rCpuInfo.i4LittleCpuFreq = -1;
		if (rBoostInfo.rCpuInfo.i4BigCpuFreq != 0)
			rBoostInfo.rCpuInfo.i4BigCpuFreq *= 1000;
		else
			rBoostInfo.rCpuInfo.i4BigCpuFreq = -1;

		if (u4SkbAllocWorkCoreType > CPU_CORE_BIG)
			u4SkbAllocWorkCoreType = CPU_CORE_BIG;
		rBoostInfo.eSkbAllocWorkCoreType = u4SkbAllocWorkCoreType;

		if (u4TxFreeSkbWorkCoreType > CPU_CORE_BIG)
			u4TxFreeSkbWorkCoreType = CPU_CORE_BIG;
		rBoostInfo.eTxFreeSkbWorkCoreType = u4TxFreeSkbWorkCoreType;

		kalSetCpuBoost(prAdapter, &rBoostInfo);
	} else {
		DBGLOG(REQ, ERROR,
			"BoostCpu CMD: Number of PARAMETERS is WRONG\n");
	}
	return i4BytesWritten;
}

#if ((CFG_SUPPORT_ICS == 1) || (CFG_SUPPORT_PHY_ICS == 1))
int priv_driver_sniffer(struct net_device *prNetDev,
				  char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	int32_t i4Recv = 0;
	int8_t *this_char = NULL;
	struct PARAM_CUSTOM_ICS_SNIFFER_INFO_STRUCT rSniffer;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %d, apcArgv[0] = %s\n\n", i4Argc, *apcArgv);

	this_char = kalStrStr(*apcArgv, "=");
	if (!this_char)
		return -1;
	this_char++;

	kalMemZero(&rSniffer,
		sizeof(struct PARAM_CUSTOM_ICS_SNIFFER_INFO_STRUCT));
	i4Recv = sscanf(this_char,
		"%hhu-%hhu-%hhu-%hhu-%hx-%hd-%hd-%hx-%x-%x-%hd",
		&(rSniffer.ucModule),
		&(rSniffer.ucAction),
		&(rSniffer.ucFilter),
		&(rSniffer.ucOperation),
		&(rSniffer.ucCondition[0]),
		&(rSniffer.ucCondition[1]),
		&(rSniffer.ucCondition[2]),
		&(rSniffer.ucCondition[3]),
		&(rSniffer.ucCondition[4]),
		&(rSniffer.ucCondition[5]),
		&(rSniffer.ucCondition[6]));

	if (i4Recv == 10) {
		if (rSniffer.ucModule == MAC_ICS_MODE) {
			DBGLOG(REQ, INFO, "An ICS cmd");
			rStatus = kalIoctl(prGlueInfo, wlanoidSetIcsSniffer,
				&rSniffer, sizeof(rSniffer), &u4BufLen);
			if (rStatus != WLAN_STATUS_SUCCESS)
				return -1;
		} else {
			/* reserve for PSS sniffer and system overall setting*/
			DBGLOG(REQ, ERROR, "Not an ICS cmd");
		}
	} else if (i4Recv == 11) {
#if ((CFG_SUPPORT_ICS == 1) || (CFG_SUPPORT_PHY_ICS == 1))
		if (rSniffer.ucModule == PHY_ICS_MODE) {
			DBGLOG(REQ, INFO, "An PHY ICS cmd");
			if (rSniffer.ucCondition[3] < 256) {
				prAdapter->fgEnPhyICS = TRUE;
				rStatus = kalIoctl(prGlueInfo,
					wlanoidSetIcsSniffer,
					&rSniffer, sizeof(rSniffer),
					&u4BufLen);
				if (rStatus != WLAN_STATUS_SUCCESS)
					return -1;
			} else {
				DBGLOG(REQ, ERROR,
				"PHY ICS partition must less than 0xff");
			}
		}
#endif /* #if CFG_SUPPORT_PHY_ICS */
	} else {
		DBGLOG(REQ, ERROR,
			"SNIFFER CMD: Number of PARAMETERS is WRONG\n");
	}
	return i4BytesWritten;
}
#endif /* #if ((CFG_SUPPORT_ICS == 1) || (CFG_SUPPORT_PHY_ICS == 1)) */

#ifdef CFG_SUPPORT_SNIFFER_RADIOTAP
int priv_driver_set_monitor(struct net_device *prNetDev,
			       char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	int32_t i4Recv = 0;
	int8_t *this_char = NULL;
	uint8_t ucBandIdx, ucBand, ucPriChannel, ucChannelWidth;
	uint8_t ucChannelS1, ucChannelS2, ucSco, fgDropFcsErrorFrame;
	uint16_t u2Aid;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %d, apcArgv[0] = %s\n\n", i4Argc, *apcArgv);

	this_char = kalStrStr(*apcArgv, "=");
	if (!this_char)
		return -1;
	this_char++;

	DBGLOG(REQ, LOUD, "string = %s\n", this_char);

	i4Recv = sscanf(this_char, "%d-%d-%d-%d-%d-%d-%d-%d-%d",
			&ucBandIdx, &ucBand, &ucPriChannel,
			&ucChannelWidth, &ucChannelS1, &ucChannelS2,
			&ucSco, &fgDropFcsErrorFrame, &u2Aid);

	if (i4Recv == 9) {
		prGlueInfo->ucBandIdx = ucBandIdx;
		prGlueInfo->ucBand = ucBand;
		prGlueInfo->ucPriChannel = ucPriChannel;
		prGlueInfo->ucChannelWidth = ucChannelWidth;
		prGlueInfo->ucChannelS1 = ucChannelS1;
		prGlueInfo->ucChannelS2 = ucChannelS2;
		prGlueInfo->ucSco = ucSco;
		prGlueInfo->fgDropFcsErrorFrame = fgDropFcsErrorFrame;
		prGlueInfo->u2Aid = u2Aid;

		i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen,
			"Apply BnIdx %d\n", prGlueInfo->ucBandIdx);

		rStatus = kalIoctl(prGlueInfo, wlanoidSetMonitor,
			NULL, 0, &u4BufLen);

		if (rStatus != WLAN_STATUS_SUCCESS)
			return -1;
	} else {
		DBGLOG(REQ, ERROR, "iwpriv wlanXX driver monitor=Option\n");
		DBGLOG(REQ, ERROR,
			"Option:[BnIdx]-[Bn]-[ChP]-[BW]-[ChS1]-[ChS2]-[Sco]-[dropFcsErr]-[AID]\n");
		DBGLOG(REQ, ERROR, "[BnIdx]Band Index\n");
		DBGLOG(REQ, ERROR, "[Bn]2G=1, 5G=2, 6G=3\n");
		DBGLOG(REQ, ERROR, "[ChP]Primary Channel\n");
		DBGLOG(REQ, ERROR, "[BW]BW20=0, BW40=0, BW80=1, BW160=2, BW80P80=3, BW320=6\n");
		DBGLOG(REQ, ERROR, "[ChS1]Center1 Channel\n");
		DBGLOG(REQ, ERROR, "[ChS2]Center2 Channel\n");
		DBGLOG(REQ, ERROR, "[Sco]Secondary Channel Offset\n");
		DBGLOG(REQ, ERROR, "[dropFcsErr]Drop FCS Error\n");
		DBGLOG(REQ, ERROR, "[AID]OFDMA AID\n");

		i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen,
					"Wrong param\n");
	}

	return i4BytesWritten;
}
#endif

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
int priv_driver_set_unified_fixed_rate(struct net_device *prNetDev,
			       char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4WCID = 0;
	uint32_t u4Mode = 0, u4Bw = 0, u4Mcs = 0, u4Nss = 0, u2HeLtf = 0;
	uint32_t u4SGI = 0, u4Preamble = 0, u4STBC = 0, u4LDPC = 0, u4SpeEn = 0;
	int32_t i4Recv = 0;
	int8_t *this_char = NULL;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %d, apcArgv[0] = %s\n\n", i4Argc, *apcArgv);

	this_char = kalStrStr(*apcArgv, "=");
	if (!this_char)
		return -1;
	this_char++;

	DBGLOG(REQ, LOUD, "string = %s\n", this_char);

	i4Recv = sscanf(this_char, "%d-%d-%d-%d-%d-%d-%d-%d-%d-%d-%d",
			&(u4WCID), &(u4Mode), &(u4Bw), &(u4Mcs),
			&(u4Nss), &(u4SGI), &(u4Preamble), &(u4STBC),
			&(u4LDPC), &(u4SpeEn), &(u2HeLtf));

	DBGLOG(REQ, LOUD, "u4WCID=%d\nu4Mode=%d\nu4Bw=%d\n",
	       u4WCID, u4Mode, u4Bw);
	DBGLOG(REQ, LOUD, "u4Mcs=%d\nu4Nss=%d\nu4SGI=%d\n",
	       u4Mcs, u4Nss, u4SGI);
	DBGLOG(REQ, LOUD, "u4Preamble=%d\nu4STBC=%d\n",
	       u4Preamble, u4STBC);
	DBGLOG(REQ, LOUD, "u4LDPC=%d\nu4SpeEn=%d\nu2HeLtf=%d\n",
	       u4LDPC, u4SpeEn, (u2HeLtf));

	if (i4Recv == 11) {
		struct UNI_CMD_RA_SET_FIXED_RATE_V1 rate;

#define HT_LDPC BIT(0)
#define VHT_LDPC BIT(1)
#define HE_LDPC BIT(2)

		rate.u2WlanIdx = u4WCID;
		rate.u2HeLtf = u2HeLtf;
		rate.u2ShortGi = u4SGI;
		rate.u1PhyMode = u4Mode;
		rate.u1Stbc = u4STBC;
		rate.u1Bw = u4Bw;
		if (u4LDPC)
			rate.u1Ecc = HT_LDPC | VHT_LDPC | HE_LDPC;
		else
			rate.u1Ecc = 0;
		rate.u1Mcs = u4Mcs;
		rate.u1Nss = u4Nss;
		rate.u1Spe = u4SpeEn;
		rate.u1ShortPreamble = u4Preamble;

		i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen,
			"Apply WCID %d\n", u4WCID);

		rStatus = kalIoctl(prGlueInfo, wlanoidSetFixRate,
					   &rate, sizeof(rate), &u4BufLen);

		if (rStatus != WLAN_STATUS_SUCCESS)
			return -1;
	} else {
		DBGLOG(REQ, ERROR, "iwpriv wlanXX driver FixedRate=Option\n");
		DBGLOG(REQ, ERROR,
			"Option:[WCID]-[Mode]-[BW]-[MCS]-[Nss]-[SGI]-[Preamble]-[STBC]-[LDPC]-[SPE_EN]-[HeLtf]\n");
		DBGLOG(REQ, ERROR, "[WCID]Wireless Client ID\n");
		DBGLOG(REQ, ERROR,
			"[Mode]CCK=0, OFDM=1, HT=2, GF=3, VHT=4, PLR=5, HE_SU=8, HE_ER_SU=9, HE_TRIG=10, HE_MU=11, EHT_ER=13, EHT_TB=14, EHT_SU and EHT_MU=15\n");
		DBGLOG(REQ, ERROR, "[BW]BW20=0, BW40=1, BW80=2,BW160=3 BW320=4\n");
		DBGLOG(REQ, ERROR,
		       "[MCS]CCK=0~3, OFDM=0~7, HT=0~32, VHT=0~9 HE=0~11 EHT=0~13 EHT_ER=14~15\n");
		DBGLOG(REQ, ERROR, "[Nss]1~8\n");
		DBGLOG(REQ, ERROR, "[GI]HT/VHT: 0:Long, 1:Short, ");
		DBGLOG(REQ, ERROR,
			"HE: SGI=0(0.8us), MGI=1(1.6us), LGI=2(3.2us)\n");
		DBGLOG(REQ, ERROR, "[Preamble]Long=0, Other=Short\n");
		DBGLOG(REQ, ERROR, "[STBC]Enable=1, Disable=0\n");
		DBGLOG(REQ, ERROR, "[LDPC]BCC=0, LDPC=1\n");
		DBGLOG(REQ, ERROR, "[HE-LTF]1X=0, 2X=1, 4X=2\n");
		DBGLOG(REQ, ERROR, "[HE-ER-DCM]Enable=1, Disable=0\n");
		DBGLOG(REQ, ERROR, "[HE-ER-106]106-tone=1\n");

		i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen,
					"Wrong param\n");
	}

	return i4BytesWritten;
}

int priv_driver_set_unified_auto_rate(struct net_device *prNetDev,
			       char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4WCID = 0;
	uint8_t ucAutoRateEn = 0, ucMode = 0;
	int32_t i4Recv = 0;
	int8_t *this_char = NULL;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %d, apcArgv[0] = %s\n\n", i4Argc, *apcArgv);

	this_char = kalStrStr(*apcArgv, "=");
	if (!this_char)
		return -1;
	this_char++;

	DBGLOG(REQ, LOUD, "string = %s\n", this_char);

	i4Recv = sscanf(this_char, "%d-%d-%d",
			&(u4WCID), &(ucAutoRateEn), &(ucMode));

	DBGLOG(REQ, LOUD, "u4WCID=%d\nucAutoRateEn=%d\nucMode=%d\n",
	       u4WCID, ucAutoRateEn, ucMode);


	if (i4Recv == 3) {
		struct UNI_CMD_RA_SET_AUTO_RATE rate;

		rate.u2WlanIdx = u4WCID;
		rate.u1AutoRateEn = ucAutoRateEn;
		rate.u1Mode = ucMode;

		i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen,
			"Apply WCID %d\n", u4WCID);

		rStatus = kalIoctl(prGlueInfo, wlanoidSetAutoRate,
					   &rate, sizeof(rate), &u4BufLen);
		if (rStatus != WLAN_STATUS_SUCCESS)
			return -1;
	} else {
		DBGLOG(INIT, ERROR, "iwpriv wlanXX driver AutoRate=Option\n");
		DBGLOG(INIT, ERROR,
			"Option:[WCID]-[AutoRateEn]-[Mode]\n");
		DBGLOG(INIT, ERROR, "[WCID]Wireless Client ID\n");
		DBGLOG(INIT, ERROR, "[AutoRateEn]\n");
		DBGLOG(INIT, ERROR, "[Mode]\n");

		i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen,
					"Wrong param\n");
	}

	return i4BytesWritten;
}

#if (CFG_SUPPORT_802_11BE_MLO == 1)
int priv_driver_set_unified_mlo_agc_tx(struct net_device *prNetDev,
			       char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	int8_t *this_char = NULL;
	int32_t i4Recv = 0;
	uint32_t u4MldRecIdx = 0, u4MldRecLinkIdx = 0, u4AcIdx = 0;
	uint32_t u4DispPolTx = 0, u4DispRatioTx = 0, u4DispOrderTx = 0;
	uint32_t u4DispMgfTx = 0;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	struct UNI_CMD_MLO_MLD_REC_LINK_AGC_TX mlo;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %d, apcArgv[0] = %s\n\n", i4Argc, *apcArgv);

	this_char = kalStrStr(*apcArgv, "=");
	if (!this_char)
		return -1;
	this_char++;

	DBGLOG(REQ, LOUD, "string = %s\n", this_char);

	i4Recv = sscanf(this_char, "%d:%d:%d:%d:%d:%d:%d",
			&(u4MldRecIdx), &(u4MldRecLinkIdx), &(u4AcIdx),
			&(u4DispPolTx), &(u4DispMgfTx), &(u4DispRatioTx),
			&(u4DispOrderTx));

	DBGLOG(REQ, ERROR, "MldRecIdx=%d,LinkIdx=%d,AcIdx=%d\n",
	       u4MldRecIdx, u4MldRecLinkIdx, u4AcIdx);
	DBGLOG(REQ, ERROR, "Pol=%d,Mgf=%d,Ratio=%d,Order=%d\n",
	       u4DispPolTx, u4DispMgfTx, u4DispRatioTx, u4DispOrderTx);

	if (i4Recv == 7) {
		if (u4MldRecIdx >= MAX_MLO_MGMT_SUPPORT_MLD_NUM) {
			DBGLOG(REQ, ERROR, "MldIdx should be less than %u\n",
				MAX_MLO_MGMT_SUPPORT_MLD_NUM);
			return -1;
		}
		if (u4MldRecLinkIdx >= MLD_LINK_MAX) {
			DBGLOG(REQ, ERROR, "Link should be less than %u\n",
				MLD_LINK_MAX);
			return -1;
		}
		if (u4AcIdx >= MAX_MLO_MGMT_SUPPORT_AC_NUM) {
			DBGLOG(REQ, ERROR, "Ac should less be than %u\n",
				MAX_MLO_MGMT_SUPPORT_AC_NUM);
			return -1;
		}
		mlo.u1MldRecIdx = (uint8_t)u4MldRecIdx;
		mlo.u1MldRecLinkIdx = (uint8_t)u4MldRecLinkIdx;
		mlo.u1AcIdx = (uint8_t)u4AcIdx;
		mlo.u1DispPolTx = (uint8_t)u4DispPolTx;
		mlo.u1DispRatioTx = (uint8_t)u4DispRatioTx;
		mlo.u1DispOrderTx = (uint8_t)u4DispOrderTx;
		mlo.u2DispMgfTx = (uint16_t)u4DispMgfTx;

		rStatus = kalIoctl(prGlueInfo, wlanoidSetMloAgcTx,
					&mlo, sizeof(mlo), &u4BufLen);

		if (rStatus != WLAN_STATUS_SUCCESS)
			return -1;
	} else {
		DBGLOG(REQ, ERROR, "iwpriv wlanX driver mloagctx=Option\n");
		DBGLOG(REQ, ERROR,
		"Option=[MldRec]:[Link]:[Ac]:[Pol]:[Ratio]:[Order]:[Mgf]\n");
	}

	return i4BytesWritten;
}	/* priv_driver_set_unified_mlo_agc_tx */

int priv_driver_get_unified_mld_rec(struct net_device *prNetDev,
			       char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	int8_t *this_char = NULL;
	struct PARAM_MLD_REC mld;
	uint32_t u4MldRecIdx = 0;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	struct CHIP_DBG_OPS *prChipDbg;
	uint32_t *pu4Handle;
	int rc;

	ASSERT(prNetDev);

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;
	ASSERT(prAdapter);

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %d, apcArgv[0] = %s\n\n", i4Argc, *apcArgv);

	this_char = kalStrStr(*apcArgv, "=");
	if (!this_char)
		return -1;
	this_char++;

	DBGLOG(REQ, LOUD, "string = %s\n", this_char);

	rc = kstrtou32(this_char, 0, &(u4MldRecIdx));

	if (rc)
		return -1;

	DBGLOG(REQ, ERROR, "MldRecIdx=%d\n", u4MldRecIdx);

	if (u4MldRecIdx >= MAX_MLO_MGMT_SUPPORT_MLD_NUM) {
		DBGLOG(REQ, ERROR, "MldIdx should be less than %u\n",
			MAX_MLO_MGMT_SUPPORT_MLD_NUM);
		return -1;
	}

	mld.u1MldRecIdx = (uint8_t)u4MldRecIdx;

	rStatus = kalIoctl(prGlueInfo, wlanoidGetMldRec,
				&mld, sizeof(mld), &u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS)
		return -1;

	wlanGetChipDbgOps(prAdapter, &pu4Handle);
	prChipDbg = (struct CHIP_DBG_OPS *)pu4Handle;

	if (prChipDbg && prChipDbg->show_mld_info)
		i4BytesWritten = prChipDbg->show_mld_info(
			prAdapter, pcCommand, i4TotalLen, &mld);

	return i4BytesWritten;
} /* priv_driver_get_unified_mld_rec */
#endif

#else
int priv_driver_set_fixed_rate(struct net_device *prNetDev,
			       char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	/* INT_32 u4Ret = 0; */
	uint32_t u4WCID = 0;
	int32_t i4Recv = 0;
	int8_t *this_char = NULL;
	uint32_t u4Id = 0xa0610000;
	uint32_t u4Id2 = 0xa0600000;
	static uint8_t fgIsUseWCID = FALSE;
	struct FIXED_RATE_INFO rFixedRate;

	struct PARAM_CUSTOM_SW_CTRL_STRUCT rSwCtrlInfo;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %d, apcArgv[0] = %s\n\n", i4Argc, *apcArgv);

	this_char = kalStrStr(*apcArgv, "=");
	if (!this_char)
		return -1;
	this_char++;
	kalMemZero(&rFixedRate, sizeof(struct FIXED_RATE_INFO));
	DBGLOG(REQ, LOUD, "string = %s\n", this_char);

	if (strnicmp(this_char, "auto", strlen("auto")) == 0) {
		i4Recv = 1;
	} else if (strnicmp(this_char, "UseWCID", strlen("UseWCID")) == 0) {
		i4Recv = 2;
		fgIsUseWCID = TRUE;
	} else if (strnicmp(this_char, "ApplyAll", strlen("ApplyAll")) == 0) {
		i4Recv = 3;
		fgIsUseWCID = FALSE;
	} else {
		i4Recv = sscanf(this_char,
			"%d-%d-%d-%d-%d-%d-%d-%d-%d-%d-%d-%d-%d",
			&(u4WCID),
			&(rFixedRate.u4Mode),
			&(rFixedRate.u4Bw),
			&(rFixedRate.u4Mcs),
			&(rFixedRate.u4VhtNss),
			&(rFixedRate.u4SGI),
			&(rFixedRate.u4Preamble),
			&(rFixedRate.u4STBC),
			&(rFixedRate.u4LDPC),
			&(rFixedRate.u4SpeEn),
			&(rFixedRate.u4HeLTF),
			&(rFixedRate.u4HeErDCM),
			&(rFixedRate.u4HeEr106t));

		DBGLOG(REQ, LOUD, "u4WCID=%d\nu4Mode=%d\nu4Bw=%d\n", u4WCID,
			rFixedRate.u4Mode, rFixedRate.u4Bw);
		DBGLOG(REQ, LOUD, "u4Mcs=%d\nu4VhtNss=%d\nu4GI=%d\n",
			rFixedRate.u4Mcs,
			rFixedRate.u4VhtNss, rFixedRate.u4SGI);
		DBGLOG(REQ, LOUD, "u4Preamble=%d\nu4STBC=%d\n",
			rFixedRate.u4Preamble,
			rFixedRate.u4STBC);
		DBGLOG(REQ, LOUD, "u4LDPC=%d\nu4SpeEn=%d\nu4HeLTF=%d\n",
			rFixedRate.u4LDPC, rFixedRate.u4SpeEn,
			rFixedRate.u4HeLTF);
		DBGLOG(REQ, LOUD, "u4HeErDCM=%d\nu4HeEr106t=%d\n",
			rFixedRate.u4HeErDCM, rFixedRate.u4HeEr106t);
		DBGLOG(REQ, LOUD, "fgIsUseWCID=%d\n", fgIsUseWCID);
	}

	if (i4Recv == 1) {
		rSwCtrlInfo.u4Id = u4Id2;
		rSwCtrlInfo.u4Data = 0;

		rStatus = kalIoctl(prGlueInfo, wlanoidSetSwCtrlWrite,
				   &rSwCtrlInfo, sizeof(rSwCtrlInfo),
				   &u4BufLen);

		if (rStatus != WLAN_STATUS_SUCCESS)
			return -1;
	} else if (i4Recv == 2 || i4Recv == 3) {
		i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen,
			"Update fgIsUseWCID %d\n", fgIsUseWCID);
	} else if (i4Recv == 10 || i4Recv == 13) {
		rSwCtrlInfo.u4Id = u4Id;
		if (fgIsUseWCID && u4WCID < prAdapter->ucWtblEntryNum &&
			prAdapter->rWifiVar.arWtbl[u4WCID].ucUsed) {
			rSwCtrlInfo.u4Id |= u4WCID;
			rSwCtrlInfo.u4Id |= BIT(8);
			i4BytesWritten = kalSnprintf(
				pcCommand, i4TotalLen,
				"Apply WCID %d\n", u4WCID);
		} else {
			i4BytesWritten = kalSnprintf(
				pcCommand, i4TotalLen, "Apply All\n");
		}

		if (rFixedRate.u4Mode >= TX_RATE_MODE_HE_SU) {
			if (i4Recv == 10) {
				/* Give default value */
				rFixedRate.u4HeLTF = HE_LTF_2X;
				rFixedRate.u4HeErDCM = 0;
				rFixedRate.u4HeEr106t = 0;
			}
			/* check HE-LTF and HE GI combinations */
			if (WLAN_STATUS_SUCCESS !=
				nicRateHeLtfCheckGi(&rFixedRate))
				return -1;

			/* check DCM limitation */
			if (rFixedRate.u4HeErDCM) {
				if ((rFixedRate.u4STBC) ||
					(rFixedRate.u4VhtNss > 2))
					return -1;

				if ((rFixedRate.u4Mcs > 4) ||
					(rFixedRate.u4Mcs == 2))
					return -1;
			}

			/* check ER_SU limitation */
			if (rFixedRate.u4Mode == TX_RATE_MODE_HE_ER) {
				if ((rFixedRate.u4Bw > 0) ||
					(rFixedRate.u4VhtNss > 1))
					return -1;

				if (rFixedRate.u4HeEr106t) {
					if (rFixedRate.u4Mcs > 0)
						return -1;
				} else {
					if (rFixedRate.u4Mcs > 2)
						return -1;
				}
			}
		}

		rStatus = nicSetFixedRateData(&rFixedRate, &rSwCtrlInfo.u4Data);

		if (rStatus == WLAN_STATUS_SUCCESS) {
			rStatus = kalIoctl(prGlueInfo, wlanoidSetSwCtrlWrite,
					   &rSwCtrlInfo, sizeof(rSwCtrlInfo),
					   &u4BufLen);
		}

		if (rStatus != WLAN_STATUS_SUCCESS)
			return -1;
	} else {
		DBGLOG(REQ, ERROR, "iwpriv wlanXX driver FixedRate=Option\n");
		DBGLOG(REQ, ERROR, "Option support 10 or 13 parameters\n");
		DBGLOG(REQ, ERROR,
		       "Option:[WCID]-[Mode]-[BW]-[MCS]-[VhtHeNss]-[GI]-[Preamble]-[STBC]-[LDPC]-[SPE_EN]\n");
		DBGLOG(REQ, ERROR,
			"13 param support another [HE-LTF]-[HE-ER-DCM]-[HE-ER-106]\n");
		DBGLOG(REQ, ERROR, "[WCID]Wireless Client ID\n");
		DBGLOG(REQ, ERROR, "[Mode]CCK=0, OFDM=1, HT=2, GF=3, VHT=4");
		DBGLOG(REQ, ERROR,
			"PLR=5, HE_SU=8, HE_ER_SU=9, HE_TRIG=10, HE_MU=11\n");
		DBGLOG(REQ, ERROR, "[BW]BW20=0, BW40=1, BW80=2,BW160=3\n");
		DBGLOG(REQ, ERROR,
		       "[MCS]CCK=0~3, OFDM=0~7, HT=0~32, VHT=0~9, HE=0~11\n");
		DBGLOG(REQ, ERROR, "[VhtHeNss]1~8, Other=ignore\n");
		DBGLOG(REQ, ERROR, "[GI]HT/VHT: 0:Long, 1:Short, ");
		DBGLOG(REQ, ERROR,
			"HE: SGI=0(0.8us), MGI=1(1.6us), LGI=2(3.2us)\n");
		DBGLOG(REQ, ERROR, "[Preamble]Long=0, Other=Short\n");
		DBGLOG(REQ, ERROR, "[STBC]Enable=1, Disable=0\n");
		DBGLOG(REQ, ERROR, "[LDPC]BCC=0, LDPC=1\n");
		DBGLOG(REQ, ERROR, "[HE-LTF]1X=0, 2X=1, 4X=2\n");
		DBGLOG(REQ, ERROR, "[HE-ER-DCM]Enable=1, Disable=0\n");
		DBGLOG(REQ, ERROR, "[HE-ER-106]106-tone=1\n");
	}

	return i4BytesWritten;
}
#endif

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
int priv_driver_set_pp_cap_ctrl(struct net_device *prNetDev,
			       char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4pp_mgmt_en = 0, u4pp_ctrl = 0;
	uint32_t u4pp_bitmap = 0, u4pp_mgmt = 0;
	int32_t i4Recv = 0;
	int8_t *this_char = NULL;
	uint8_t u1DbdcIdx = 0;

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %d, apcArgv[0] = %s\n\n", i4Argc, *apcArgv);

	this_char = kalStrStr(*apcArgv, "=");
	if (!this_char)
		return -1;
	this_char++;

	DBGLOG(REQ, LOUD, "string = %s\n", this_char);

	i4Recv = sscanf(this_char, "%d-%d-%d-%x-%d",
			&(u1DbdcIdx),
			&(u4pp_mgmt_en),
			&(u4pp_ctrl),
			&(u4pp_bitmap),
			&(u4pp_mgmt));

	if (i4Recv == 5) {
		struct UNI_CMD_PP_EN_CTRL_T pp_cap_ctrl;

		pp_cap_ctrl.u1PpMgmtMode = (uint8_t)u4pp_mgmt;
		pp_cap_ctrl.u1PpCtrl     = (uint8_t)u4pp_ctrl;
		pp_cap_ctrl.u1PpBitMap   = (uint16_t)u4pp_bitmap;
		pp_cap_ctrl.u1DbdcIdx    = (uint8_t)u1DbdcIdx;
		pp_cap_ctrl.u1PpMgmtEn = (uint8_t)u4pp_mgmt_en;

		i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen,
			"pp_mgmt_en=%d\npp_ctrl=%d\npp_bitmap=0x%x\npp_mgmt=%d\n",
			u4pp_mgmt_en, u4pp_ctrl, u4pp_bitmap, u4pp_mgmt);

		rStatus = kalIoctl(prGlueInfo, wlanoidSetPpCap,
					   &pp_cap_ctrl, sizeof(pp_cap_ctrl),
					   &u4BufLen);

		if (rStatus != WLAN_STATUS_SUCCESS)
			return -1;
	} else {
		DBGLOG(REQ, ERROR, "iwpriv wlanXX driver ppcapctrl=Option\n");
		DBGLOG(REQ, ERROR,
			"Option:[automod]-[pp_ctrl]-[pp_bitmap]-[pp_mgmt]\n");

		i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen,
					"Wrong param\n");
	}

	return i4BytesWritten;
}

int priv_driver_set_pp_alg_ctrl(struct net_device *prNetDev,
			       char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS, u4BufLen = 0;
	int32_t i4BytesWritten = 0, i4Argc = 0, i4Recv0 = 0, i4Recv1 = 0;
	uint32_t u4PpTimerIntv = 0, u4PpThrX2Val = 0, u4PpThrX2Shf = 0;
	uint32_t u4PpThrX3Val = 0, u4PpThrX3Shf = 0, u4PpThrX4Val = 0;
	uint32_t u4PpThrX4Shf = 0, u4PpThrX5Val = 0, u4PpThrX5Shf = 0;
	uint32_t u4PpThrX6Val = 0, u4PpThrX6Shf = 0, u4PpThrX7Val = 0;
	uint32_t u4PpThrX7Shf = 0, u4PpThrX8Val = 0, u4PpThrX8Shf = 0;
	uint8_t u1PpAction = 0, u1DbdcIdx = 0, u1Reset = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	int8_t *this_char = NULL;
	struct UNI_CMD_PP_ALG_CTRL rPpAlgCtrl;

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, INFO, "\x1b[32m command is %s\x1b[m\n"
		, pcCommand);
	DBGLOG(REQ, INFO, "\x1b[32m argc is %d, apcArgv[0] = %s\x1b[m\n"
		, i4Argc, *apcArgv);

	this_char = kalStrStr(*apcArgv, "=");
	if (!this_char)
		return -1;

	this_char++;
	DBGLOG(REQ, INFO, "\x1b[32m string = %s\x1b[m\n"
		, this_char);

	i4Recv0 = sscanf(this_char, "%d:", &u1PpAction);
	this_char += 2;
	DBGLOG(REQ, INFO, "\x1b[32m u1PpAction = %d, i4Recv0 = %d\x1b[m\n"
		, u1PpAction, i4Recv0);

	if (i4Recv0 != 1)
		goto error;

	kalMemZero(&rPpAlgCtrl, sizeof(struct UNI_CMD_PP_ALG_CTRL));
	rPpAlgCtrl.u1PpAction = u1PpAction;

	switch (u1PpAction) {
	case UNI_CMD_PP_ALG_SET_TIMER:
		i4Recv1 = sscanf(this_char, "%d:%d",
			&u1DbdcIdx,
			&u4PpTimerIntv);

		rPpAlgCtrl.u1DbdcIdx = u1DbdcIdx;
		rPpAlgCtrl.u4PpTimerIntv = u4PpTimerIntv;
		DBGLOG(REQ, INFO, "\x1b[32m u1PpAction = %d\x1b[m\n"
			, rPpAlgCtrl.u1PpAction);
		DBGLOG(REQ, INFO, "\x1b[32m u4PpTimerIntv = %d\x1b[m\n"
			, rPpAlgCtrl.u4PpTimerIntv);
		DBGLOG(REQ, INFO, "\x1b[32m i4Recv1 = %d\x1b[m\n"
			, i4Recv1);

		if (i4Recv1 != 2)
			goto error;

		break;
	case UNI_CMD_PP_ALG_SET_THR:
		i4Recv1 = sscanf(this_char,
			"%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d",
			&u1DbdcIdx,
			&u4PpThrX2Val,
			&u4PpThrX2Shf,
			&u4PpThrX3Val,
			&u4PpThrX3Shf,
			&u4PpThrX4Val,
			&u4PpThrX4Shf,
			&u4PpThrX5Val,
			&u4PpThrX5Shf,
			&u4PpThrX6Val,
			&u4PpThrX6Shf,
			&u4PpThrX7Val,
			&u4PpThrX7Shf,
			&u4PpThrX8Val,
			&u4PpThrX8Shf);

		rPpAlgCtrl.u1DbdcIdx = u1DbdcIdx;
		rPpAlgCtrl.u4ThrX2_Value = u4PpThrX2Val;
		rPpAlgCtrl.u4ThrX2_Shift = u4PpThrX2Shf;
		rPpAlgCtrl.u4ThrX3_Value = u4PpThrX3Val;
		rPpAlgCtrl.u4ThrX3_Shift = u4PpThrX3Shf;
		rPpAlgCtrl.u4ThrX4_Value = u4PpThrX4Val;
		rPpAlgCtrl.u4ThrX4_Shift = u4PpThrX4Shf;
		rPpAlgCtrl.u4ThrX5_Value = u4PpThrX5Val;
		rPpAlgCtrl.u4ThrX5_Shift = u4PpThrX5Shf;
		rPpAlgCtrl.u4ThrX6_Value = u4PpThrX6Val;
		rPpAlgCtrl.u4ThrX6_Shift = u4PpThrX6Shf;
		rPpAlgCtrl.u4ThrX7_Value = u4PpThrX7Val;
		rPpAlgCtrl.u4ThrX7_Shift = u4PpThrX7Shf;
		rPpAlgCtrl.u4ThrX8_Value = u4PpThrX8Val;
		rPpAlgCtrl.u4ThrX8_Shift = u4PpThrX8Shf;


		DBGLOG(REQ, INFO, "\x1b[32m u4PpAction = %d\x1b[m\n"
			, rPpAlgCtrl.u1PpAction);
		DBGLOG(REQ, INFO, "\x1b[32m u1DbdcIdx = %d\x1b[m\n"
			, rPpAlgCtrl.u1DbdcIdx);
		DBGLOG(REQ, INFO, "\x1b[32m u4ThrX2_Value = %d\x1b[m\n"
			, rPpAlgCtrl.u4ThrX2_Value);
		DBGLOG(REQ, INFO, "\x1b[32m u4ThrX2_Shift = %d\x1b[m\n"
			, rPpAlgCtrl.u4ThrX2_Shift);
		DBGLOG(REQ, INFO, "\x1b[32m u4ThrX3_Value = %d\x1b[m\n"
			, rPpAlgCtrl.u4ThrX3_Value);
		DBGLOG(REQ, INFO, "\x1b[32m u4ThrX3_Shift = %d\x1b[m\n"
			, rPpAlgCtrl.u4ThrX3_Shift);
		DBGLOG(REQ, INFO, "\x1b[32m u4ThrX4_Value = %d\x1b[m\n"
			, rPpAlgCtrl.u4ThrX4_Value);
		DBGLOG(REQ, INFO, "\x1b[32m u4ThrX4_Shift = %d\x1b[m\n"
			, rPpAlgCtrl.u4ThrX4_Shift);
		DBGLOG(REQ, INFO, "\x1b[32m u4ThrX5_Value = %d\x1b[m\n"
			, rPpAlgCtrl.u4ThrX5_Value);
		DBGLOG(REQ, INFO, "\x1b[32m u4ThrX5_Shift = %d\x1b[m\n"
			, rPpAlgCtrl.u4ThrX5_Shift);
		DBGLOG(REQ, INFO, "\x1b[32m u4ThrX6_Value = %d\x1b[m\n"
			, rPpAlgCtrl.u4ThrX6_Value);
		DBGLOG(REQ, INFO, "\x1b[32m u4ThrX6_Shift = %d\x1b[m\n"
			, rPpAlgCtrl.u4ThrX6_Shift);
		DBGLOG(REQ, INFO, "\x1b[32m u4ThrX7_Value = %d\x1b[m\n"
			, rPpAlgCtrl.u4ThrX7_Value);
		DBGLOG(REQ, INFO, "\x1b[32m u4ThrX7_Shift = %d\x1b[m\n"
			, rPpAlgCtrl.u4ThrX7_Shift);
		DBGLOG(REQ, INFO, "\x1b[32m u4ThrX8_Value = %d\x1b[m\n"
			, rPpAlgCtrl.u4ThrX8_Value);
		DBGLOG(REQ, INFO, "\x1b[32m u4ThrX8_Shift = %d\x1b[m\n"
			, rPpAlgCtrl.u4ThrX8_Shift);
		DBGLOG(REQ, INFO, "\x1b[32m i4Recv1 = %d\x1b[m\n"
			, i4Recv1);

		if (i4Recv1 != 15)
			goto error;

		break;

	case UNI_CMD_PP_ALG_GET_STATISTICS:
		i4Recv1 = sscanf(this_char, "%d:%d",
			&u1DbdcIdx,
			&u1Reset);
		rPpAlgCtrl.u1DbdcIdx = u1DbdcIdx;
		rPpAlgCtrl.u1Reset = u1Reset;

		DBGLOG(REQ, INFO, "\x1b[32m u4PpAction = %d\x1b[m\n"
			, rPpAlgCtrl.u1PpAction);
		DBGLOG(REQ, INFO, "\x1b[32m u1DbdcIdx = %d\x1b[m\n"
			, rPpAlgCtrl.u1DbdcIdx);
		DBGLOG(REQ, INFO, "\x1b[32m u1Reset = %d\x1b[m\n"
			, rPpAlgCtrl.u1Reset);
		DBGLOG(REQ, INFO, "\x1b[32m i4Recv1 = %d\x1b[m\n"
			, i4Recv1);

		if (i4Recv1 != 2)
			goto error;

		break;

	default:
		DBGLOG(REQ, ERROR,
			"\x1b[31m u4PpAction = %d is not supported !!\x1b[m\n"
			, rPpAlgCtrl.u1PpAction);
		goto error;

		break;
	}

	i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen,
	"PpAction=%d\n", rPpAlgCtrl.u1PpAction);

	rStatus = kalIoctl(prGlueInfo, wlanoidSetPpAlgCtrl,
		&rPpAlgCtrl, sizeof(struct UNI_CMD_PP_ALG_CTRL),
		&u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS)
		return -1;
	else
		return i4BytesWritten;

error:
DBGLOG(REQ, ERROR, "\x1b[31m iwpriv wlanXX driver %s \x1b[m\n"
	, pcCommand);
i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen, "Wrong param\n");

	return i4BytesWritten;
}

int priv_driver_set_hm_alg_ctrl(struct net_device *prNetDev,
					   char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};

	uint8_t u1HmManualModeEn;
	uint8_t u1ForceObss;
	uint8_t u1ForceBT;
	uint8_t u1HmForcePlan;
	uint8_t u1ObssTimePercntg;
	uint8_t u1BTPercntg;

	int32_t i4Recv = 0;
	int8_t *this_char = NULL;

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %d, apcArgv[0] = %s\n\n", i4Argc, *apcArgv);

	this_char = kalStrStr(*apcArgv, "=");
	if (!this_char)
		return -1;
	this_char++;

	DBGLOG(REQ, LOUD, "string = %s\n", this_char);

	i4Recv = sscanf(this_char, "%d-%d-%d-%d-%d-%d",
			&u1HmManualModeEn,
			&u1ForceObss,
			&u1ForceBT,
			&u1HmForcePlan,
			&u1ObssTimePercntg,
			&u1BTPercntg);


	if (i4Recv == 6) {
		struct UNI_CMD_HM_ALG_CTRL_T hm_alg_ctrl;

		hm_alg_ctrl.u1HmManualModeEn = u1HmManualModeEn;
		hm_alg_ctrl.u1ForceObss = u1ForceObss;
		hm_alg_ctrl.u1ForceBT = u1ForceBT;
		hm_alg_ctrl.u1HmForcePlan = u1HmForcePlan;
		hm_alg_ctrl.u1ObssTimePercntg = u1ObssTimePercntg;
		hm_alg_ctrl.u1BTPercntg = u1BTPercntg;

		i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen,
			"u1HmManualModeEn=%d\nu1ForceObss=%d\nu1ForceBT=%d\nu1HmForcePlan=%d\nu1ObssTimePercntg=%d\nu1BTPercntg=%d\n",
			u1HmManualModeEn, u1ForceObss, u1ForceBT,
			u1HmForcePlan, u1ObssTimePercntg, u1BTPercntg);

		rStatus = kalIoctl(prGlueInfo, wlanoidSetHmAlg,
					   &hm_alg_ctrl, sizeof(hm_alg_ctrl),
					   &u4BufLen);

		if (rStatus != WLAN_STATUS_SUCCESS)
			return -1;
	} else {
		DBGLOG(REQ, ERROR, "iwpriv wlanXX driver hmcapctrl=Option\n");
		DBGLOG(REQ, ERROR,
			"Option:[ManualMode]-[ForceObss]-[ForceBT]-[u1HmForcePlan]-[u1ObssTimePercntg]-[u1BTPercntg]\n");

		i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen,
					"Wrong param\n");
	}

	return i4BytesWritten;
}

#endif

int priv_driver_set_em_cfg(struct net_device *prNetDev, char *pcCommand,
			int i4TotalLen)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	uint32_t u4CfgSetNum = 0, u4Ret = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	uint32_t u4BufLen;

	struct PARAM_CUSTOM_KEY_CFG_STRUCT rKeyCfgInfo;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);

	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);
	prAdapter = prGlueInfo->prAdapter;
	if (prAdapter == NULL)
		return -1; /* WLAN_STATUS_ADAPTER_NOT_READY */

	kalMemZero(&rKeyCfgInfo, sizeof(rKeyCfgInfo));

	wlanCleanAllEmCfgSetting(prAdapter);

	if (i4Argc >= 3) {
		uint8_t	i = 0;

		u4Ret = kalkStrtou32(apcArgv[1], 10, &u4CfgSetNum);

		if (u4Ret != 0) {
			DBGLOG(REQ, ERROR,
			       "apcArgv[2] format fail erro code:%d\n",
			       u4Ret);
			return -1;
		}

		/* set_em_cfg n key_1 value_1 key_2 value_2 ... key_n value_n*/
		if (checkMulOverflow(u4CfgSetNum, 2)) {
			DBGLOG(REQ, ERROR,
				"Set Num(%d) with input arg num(%d) has overflow!\n",
				u4CfgSetNum, i4Argc);
		} else if (u4CfgSetNum * 2 > (i4Argc - 2)) {
			DBGLOG(REQ, ERROR,
			       "Set Num(%d) over input arg num(%d)\n",
			       u4CfgSetNum, i4Argc);
			return -1;
		}

		DBGLOG(REQ, INFO, "Total Cfg Num=%d\n", u4CfgSetNum);

		for (i = 0; i < (u4CfgSetNum * 2); i += 2) {

			kalStrnCpy(rKeyCfgInfo.aucKey, apcArgv[2 + i],
				   WLAN_CFG_KEY_LEN_MAX - 1);
			kalStrnCpy(rKeyCfgInfo.aucValue, apcArgv[2 + (i + 1)],
				   WLAN_CFG_VALUE_LEN_MAX - 1);
			rKeyCfgInfo.u4Flag = WLAN_CFG_EM;

			DBGLOG(REQ, INFO,
				"Update to driver EM CFG [%s]:[%s],OP:%d\n",
				rKeyCfgInfo.aucKey,
				rKeyCfgInfo.aucValue,
				rKeyCfgInfo.u4Flag);

			rStatus = kalIoctl(prGlueInfo,
				wlanoidSetKeyCfg,
				&rKeyCfgInfo,
				sizeof(rKeyCfgInfo),
				&u4BufLen);

			if (rStatus != WLAN_STATUS_SUCCESS)
				return -1;
		}
	}

	return i4BytesWritten;

}				/* priv_driver_set_cfg_em  */

int priv_driver_get_em_cfg(struct net_device *prNetDev, char *pcCommand,
			int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	struct WLAN_CFG_ENTRY *prWlanCfgEntry;
	int32_t i = 0;
	uint32_t u4Offset = 0;
	uint32_t u4CfgNum = 0;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, INFO,
		"command is %s, i4TotalLen=%d\n",
		pcCommand,
		i4TotalLen);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, INFO, "argc is %i\n", i4Argc);
	prAdapter = prGlueInfo->prAdapter;

	if (i4Argc >= 1) {

		u4CfgNum = wlanCfgGetTotalCfgNum(prAdapter, WLAN_CFG_EM);

		DBGLOG(REQ, INFO,
			   "Total cfg Num:%d\n", u4CfgNum);

		u4Offset += snprintf(pcCommand + u4Offset,
			(i4TotalLen - u4Offset),
			"%d,",
			u4CfgNum);

		for (i = 0; i < u4CfgNum; i++) {
			prWlanCfgEntry = wlanCfgGetEntryByIndex(
				prAdapter,
				i,
				WLAN_CFG_EM);

			if ((!prWlanCfgEntry)
				|| (prWlanCfgEntry->aucKey[0] == '\0'))
				break;

			DBGLOG(REQ, INFO,
				"cfg dump:(%s,%s)\n",
				prWlanCfgEntry->aucKey,
				prWlanCfgEntry->aucValue);

			if (u4Offset >= i4TotalLen) {
				DBGLOG(REQ, ERROR,
					"out of bound\n");
				break;
			}

			u4Offset += snprintf(pcCommand + u4Offset,
				(i4TotalLen - u4Offset),
				"%s,%s,",
				prWlanCfgEntry->aucKey,
				prWlanCfgEntry->aucValue);

		}

		pcCommand[u4Offset-1] = '\n';
		pcCommand[u4Offset] = '\0';

	}

	return (int32_t)u4Offset;

}				/* priv_driver_get_cfg_em  */


int priv_driver_set_cfg(struct net_device *prNetDev, char *pcCommand,
			int i4TotalLen)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };

	struct PARAM_CUSTOM_KEY_CFG_STRUCT rKeyCfgInfo;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);

	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);
	prAdapter = prGlueInfo->prAdapter;
	if (prAdapter == NULL)
		return -1; /* WLAN_STATUS_ADAPTER_NOT_READY */

	kalMemZero(&rKeyCfgInfo, sizeof(rKeyCfgInfo));

	if (i4Argc >= 3) {

		int8_t ucTmp[WLAN_CFG_VALUE_LEN_MAX];
		uint8_t *pucCurrBuf = ucTmp;
		uint8_t	i = 0;
		uint32_t offset = 0;

		pucCurrBuf = ucTmp;
		kalMemZero(ucTmp, WLAN_CFG_VALUE_LEN_MAX);

		if (i4Argc == 3) {
			/* no space for it, driver can't accept space in the end
			 * of the line
			 */
			/* ToDo: skip the space when parsing */
			u4BufLen = kalStrLen(apcArgv[2]);
			if (offset + u4BufLen > WLAN_CFG_VALUE_LEN_MAX - 1) {
				DBGLOG(INIT, ERROR,
				       "apcArgv[2] length [%d] overrun\n",
				       u4BufLen);
				return -1;
			}
			kalStrnCpy(pucCurrBuf + offset,
					apcArgv[2], u4BufLen + 1);
			offset += u4BufLen;
		} else {
			for (i = 2; i < i4Argc; i++) {
				u4BufLen = kalStrLen(apcArgv[i]);
				if (offset + u4BufLen >
				    WLAN_CFG_VALUE_LEN_MAX - 1) {
					DBGLOG(INIT, ERROR,
					       "apcArgv[%d] length [%d] overrun\n",
					       i, u4BufLen);
					return -1;
				}
				kalStrnCpy(pucCurrBuf + offset, apcArgv[i],
					   u4BufLen + 1);
				offset += u4BufLen;
			}
		}

		DBGLOG(INIT, WARN, "Update to driver temp buffer as [%s]\n",
		       ucTmp);
		if (kalStrLen(apcArgv[1]) > WLAN_CFG_KEY_LEN_MAX - 1) {
			DBGLOG(INIT, ERROR,
				   "apcArgv[1] length [%lu] overrun\n",
				   kalStrLen(apcArgv[1]));
			return -1;
		}

		/* wlanCfgSet(prAdapter, apcArgv[1], apcArgv[2], 0); */
		/* Call by  wlanoid because the set_cfg will trigger callback */
		kalStrnCpy(rKeyCfgInfo.aucKey, apcArgv[1],
			   WLAN_CFG_KEY_LEN_MAX - 1);
		kalStrnCpy(rKeyCfgInfo.aucValue, ucTmp,
			   WLAN_CFG_VALUE_LEN_MAX - 1);

		rKeyCfgInfo.u4Flag = WLAN_CFG_DEFAULT;
		rStatus = kalIoctl(prGlueInfo, wlanoidSetKeyCfg, &rKeyCfgInfo,
				   sizeof(rKeyCfgInfo), &u4BufLen);

		if (rStatus != WLAN_STATUS_SUCCESS)
			return -1;
	}

	return i4BytesWritten;

}				/* priv_driver_set_cfg  */

int priv_driver_get_cfg(struct net_device *prNetDev, char *pcCommand,
			int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	int8_t aucValue[WLAN_CFG_VALUE_LEN_MAX];

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);
	prAdapter = prGlueInfo->prAdapter;

	if (i4Argc >= 2) {
		/* by wlanoid ? */
		if (wlanCfgGet(prAdapter, apcArgv[1], aucValue, NULL,
			       WLAN_CFG_DEFAULT, FEATURE_TO_CUSTOMER)
					== WLAN_STATUS_SUCCESS) {
			kalStrnCpy(pcCommand, aucValue, WLAN_CFG_VALUE_LEN_MAX);
			i4BytesWritten = kalStrnLen(pcCommand,
				WLAN_CFG_VALUE_LEN_MAX);
		} else if (wlanCfgGet(prAdapter, apcArgv[1], aucValue, NULL,
				      WLAN_CFG_REC, FEATURE_TO_CUSTOMER)
						== WLAN_STATUS_SUCCESS) {
			kalStrnCpy(pcCommand, aucValue, WLAN_CFG_VALUE_LEN_MAX);
			i4BytesWritten = kalStrnLen(pcCommand,
				WLAN_CFG_VALUE_LEN_MAX);
		}
	}

	return i4BytesWritten;

}				/* priv_driver_get_cfg  */

int priv_driver_set_chip_config(struct net_device *prNetDev,
				char *pcCommand, int i4TotalLen)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	uint32_t u4CmdLen = 0;
	uint32_t u4PrefixLen = 0;
	struct PARAM_CUSTOM_CHIP_CONFIG_STRUCT rChipConfigInfo = {0};
#if CFG_SUPPORT_MET_LOG
	int32_t i4Ret = 0;
	uint32_t u4EmiMetOffset = 0;
#endif

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	if ((!prGlueInfo) ||
	    (prGlueInfo->u4ReadyFlag == 0) ||
	    kalIsResetting()) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -1;
	}

	prAdapter = prGlueInfo->prAdapter;
	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);

	u4CmdLen = kalStrnLen(pcCommand, i4TotalLen);
	u4PrefixLen = kalStrLen(CMD_SET_CHIP) + 1 /* space */;

	kalMemZero(&rChipConfigInfo, sizeof(rChipConfigInfo));

	if (u4CmdLen > u4PrefixLen) {
		rChipConfigInfo.ucType = CHIP_CONFIG_TYPE_WO_RESPONSE;
		/* rChipConfigInfo.u2MsgSize = kalStrnLen(apcArgv[1],
		 *					CHIP_CONFIG_RESP_SIZE);
		 */
		rChipConfigInfo.u2MsgSize = u4CmdLen - u4PrefixLen;
		/* kalStrnCpy(rChipConfigInfo.aucCmd, apcArgv[1],
		 *	      CHIP_CONFIG_RESP_SIZE);
		 */
		kalStrnCpy(rChipConfigInfo.aucCmd, pcCommand + u4PrefixLen,
			   CHIP_CONFIG_RESP_SIZE - 1);
		rChipConfigInfo.aucCmd[CHIP_CONFIG_RESP_SIZE - 1] = '\0';

#if CFG_SUPPORT_MET_LOG
		if (kalStrnCmp(rChipConfigInfo.aucCmd, "Wf_MET 3", 8) == 0) {
			i4Ret = kstrtouint(rChipConfigInfo.aucCmd + 9, 16,
							&u4EmiMetOffset);
			if (i4Ret) {
				DBGLOG(REQ, ERROR,
					"Convert Emi Met Offset error, ret: %d\n",
					i4Ret);
			} else {
				DBGLOG(REQ, INFO,
					"Set Emi Met Offset: 0x%x\n",
					u4EmiMetOffset);
				kalSetEmiMetOffset(u4EmiMetOffset);
			}
		}
		if (kalStrnCmp(rChipConfigInfo.aucCmd, "Wf_MET 2", 8) == 0) {
			i4Ret = met_log_stop(prGlueInfo);
			if (i4Ret != 0)
				DBGLOG(REQ, ERROR,
					"met_log_stop() error, ret: %d\n",
					i4Ret);
		}
#endif

#if (CFG_SUPPORT_802_11AX == 1)
		if (kalStrnCmp("FrdHeTrig2Host",
			pcCommand, kalStrLen("FrdHeTrig2Host"))) {
			uint32_t idx = kalStrLen("set_chip FrdHeTrig2Host ");

			prAdapter->fgEnShowHETrigger = pcCommand[idx] - 0x30;
			DBGLOG(REQ, STATE, "set flag fgEnShowHETrigger:%x\n",
			prAdapter->fgEnShowHETrigger);
		}
#endif /* CFG_SUPPORT_802_11AX  == 1 */

		rStatus = kalIoctl(prGlueInfo, wlanoidSetChipConfig,
				   &rChipConfigInfo, sizeof(rChipConfigInfo),
				   &u4BufLen);

		if (rStatus != WLAN_STATUS_SUCCESS) {
			DBGLOG(REQ, INFO, "%s: kalIoctl ret=%d\n", __func__,
			       rStatus);
			i4BytesWritten = -1;
		}

#if CFG_SUPPORT_MET_LOG
		if (kalStrnCmp(rChipConfigInfo.aucCmd, "Wf_MET 1", 8) == 0) {
			i4Ret = met_log_start(prGlueInfo);
			if (i4Ret != 0)
				DBGLOG(REQ, ERROR,
					"met_log_start() error, ret: %d\n",
					i4Ret);
		}
#endif
	}

	return i4BytesWritten;

}				/* priv_driver_set_chip_config  */

void
priv_driver_get_chip_config_16(uint8_t *pucStartAddr, uint32_t u4Length,
			       uint32_t u4Line, int i4TotalLen,
			       int32_t i4BytesWritten, char *pcCommand)
{

	while (u4Length >= 16) {
		if (i4TotalLen > i4BytesWritten) {
			i4BytesWritten +=
			    kalSnprintf(pcCommand + i4BytesWritten,
					i4TotalLen - i4BytesWritten,
					"%04x %02x %02x %02x %02x  %02x %02x %02x %02x - %02x %02x %02x %02x  %02x %02x %02x %02x\n",
					u4Line, pucStartAddr[0],
					pucStartAddr[1], pucStartAddr[2],
					pucStartAddr[3], pucStartAddr[4],
					pucStartAddr[5], pucStartAddr[6],
					pucStartAddr[7], pucStartAddr[8],
					pucStartAddr[9], pucStartAddr[10],
					pucStartAddr[11], pucStartAddr[12],
					pucStartAddr[13], pucStartAddr[14],
					pucStartAddr[15]);
		}

		pucStartAddr += 16;
		u4Length -= 16;
		u4Line += 16;
	}			/* u4Length */
}


void
priv_driver_get_chip_config_4(uint32_t *pu4StartAddr, uint32_t u4Length,
			      uint32_t u4Line, int i4TotalLen,
			      int32_t i4BytesWritten, char *pcCommand)
{
	while (u4Length >= 16) {
		if (i4TotalLen > i4BytesWritten) {
			i4BytesWritten +=
			    kalSnprintf(pcCommand + i4BytesWritten,
				     i4TotalLen - i4BytesWritten,
				     "%04x %08x %08x %08x %08x\n", u4Line,
				     pu4StartAddr[0], pu4StartAddr[1],
				     pu4StartAddr[2], pu4StartAddr[3]);
		}

		pu4StartAddr += 4;
		u4Length -= 16;
		u4Line += 4;
	}			/* u4Length */
}

int priv_driver_get_chip_config(struct net_device *prNetDev,
				char *pcCommand, int i4TotalLen)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	int32_t i4BytesWritten = 0;
	uint32_t u4BufLen = 0;
	uint32_t u2MsgSize = 0;
	uint32_t u4CmdLen = 0;
	uint32_t u4PrefixLen = 0;

	struct PARAM_CUSTOM_CHIP_CONFIG_STRUCT rChipConfigInfo = {0};

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);

	u4CmdLen = kalStrnLen(pcCommand, i4TotalLen);
	u4PrefixLen = kalStrLen(CMD_GET_CHIP) + 1 /*space */;

	if (u4CmdLen > u4PrefixLen) {
		rChipConfigInfo.ucType = CHIP_CONFIG_TYPE_ASCII;
		/* rChipConfigInfo.u2MsgSize = kalStrnLen(apcArgv[1],
		 *                             CHIP_CONFIG_RESP_SIZE);
		 */
		rChipConfigInfo.u2MsgSize = u4CmdLen - u4PrefixLen;
		/* kalStrnCpy(rChipConfigInfo.aucCmd, apcArgv[1],
		 *            CHIP_CONFIG_RESP_SIZE);
		 */
		kalStrnCpy(rChipConfigInfo.aucCmd, pcCommand + u4PrefixLen,
			   CHIP_CONFIG_RESP_SIZE - 1);
		rChipConfigInfo.aucCmd[CHIP_CONFIG_RESP_SIZE - 1] = '\0';
		rStatus = kalIoctl(prGlueInfo, wlanoidQueryChipConfig,
				   &rChipConfigInfo, sizeof(rChipConfigInfo),
				   &u4BufLen);

		if (rStatus != WLAN_STATUS_SUCCESS) {
			DBGLOG(REQ, INFO, "%s: kalIoctl ret=%d\n", __func__,
			       rStatus);
			return -1;
		}

		/* Check respType */
		u2MsgSize = rChipConfigInfo.u2MsgSize;
		DBGLOG(REQ, INFO, "%s: RespTyep  %u\n", __func__,
		       rChipConfigInfo.ucRespType);
		DBGLOG(REQ, INFO, "%s: u2MsgSize %u\n", __func__,
		       rChipConfigInfo.u2MsgSize);

		if (u2MsgSize > sizeof(rChipConfigInfo.aucCmd)) {
			DBGLOG(REQ, INFO, "%s: u2MsgSize error ret=%u\n",
			       __func__, rChipConfigInfo.u2MsgSize);
			return -1;
		}

		if (u2MsgSize > 0) {

			if (rChipConfigInfo.ucRespType ==
			    CHIP_CONFIG_TYPE_ASCII) {
				i4BytesWritten =
				    kalSnprintf(pcCommand + i4BytesWritten,
					     i4TotalLen, "%s",
					     rChipConfigInfo.aucCmd);
			} else {
				uint32_t u4Length;
				uint32_t u4Line;

				if (rChipConfigInfo.ucRespType ==
				    CHIP_CONFIG_TYPE_MEM8) {
					uint8_t *pucStartAddr = NULL;

					pucStartAddr = (uint8_t *)
							rChipConfigInfo.aucCmd;
					/* align 16 bytes because one print line
					 * is 16 bytes
					 */
					u4Length = (((u2MsgSize + 15) >> 4))
									<< 4;
					u4Line = 0;
					priv_driver_get_chip_config_16(
						pucStartAddr, u4Length, u4Line,
						i4TotalLen, i4BytesWritten,
						pcCommand);
				} else {
					uint32_t *pu4StartAddr = NULL;

					pu4StartAddr = (uint32_t *)
							rChipConfigInfo.aucCmd;
					/* align 16 bytes because one print line
					 * is 16 bytes
					 */
					u4Length = (((u2MsgSize + 15) >> 4))
									<< 4;
					u4Line = 0;

					if (IS_ALIGN_4(
					    (unsigned long) pu4StartAddr)) {
						priv_driver_get_chip_config_4(
							pu4StartAddr, u4Length,
							u4Line, i4TotalLen,
							i4BytesWritten,
							pcCommand);
					} else {
						DBGLOG(REQ, INFO,
							"%s: rChipConfigInfo.aucCmd is not 4 bytes alignment %p\n",
							__func__,
							rChipConfigInfo.aucCmd);
					}
				}	/* ChipConfigInfo.ucRespType */
			}
		}
		/* u2MsgSize > 0 */
		DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__,
		       pcCommand);
	}
	/* i4Argc */
	return i4BytesWritten;

}				/* priv_driver_get_chip_config  */


#if CFG_ENABLE_WIFI_DIRECT
int priv_driver_set_ap_start_impl(struct net_device *prNetDev, char *pcCommand,
	int i4TotalLen, uint8_t fgIsRtnlLockAcquired)
{
	struct PARAM_CUSTOM_P2P_SET_WITH_LOCK_STRUCT rSetP2P;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	uint32_t u4Ret = 0;
	int32_t i4ArgNum = 2;

	ASSERT(prNetDev);

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc >= i4ArgNum) {
		u4Ret = kalkStrtou32(apcArgv[1], 0, &(rSetP2P.u4Mode));
		if (u4Ret) {
			DBGLOG(REQ, ERROR,
			       "parse ap-start error (u4Enable) u4Ret=%d\n",
			       u4Ret);
			return -EINVAL;
		}

		if (rSetP2P.u4Mode >= RUNNING_P2P_MODE_NUM) {
			rSetP2P.u4Mode = 0;
			rSetP2P.u4Enable = 0;
		} else
			rSetP2P.u4Enable = 1;

		rSetP2P.fgIsRtnlLockAcquired = fgIsRtnlLockAcquired;

		set_p2p_mode_handler(prNetDev, rSetP2P);
	}

	return u4Ret;
}

int priv_driver_set_ap_start(struct net_device *prNetDev, char *pcCommand,
	int i4TotalLen)
{
	return priv_driver_set_ap_start_impl(prNetDev, pcCommand, i4TotalLen,
		TRUE);
}

int priv_driver_proc_set_ap_start(struct net_device *prNetDev, char *pcCommand,
	int i4TotalLen)
{
	return priv_driver_set_ap_start_impl(prNetDev, pcCommand, i4TotalLen,
		FALSE);
}
#endif

#if CFG_SUPPORT_NAN
int
priv_driver_set_nan_start(struct net_device *prNetDev, char *pcCommand,
			  int i4TotalLen) {
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	uint32_t u4Ret;
	uint32_t u4Enable = 0;

	if (!prNetDev) {
		DBGLOG(NAN, ERROR, "prNetDev error!\n");
		return -1;
	}

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **)netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	u4Ret = kalkStrtou32(apcArgv[1], 0, &(u4Enable));
	if (u4Ret)
		DBGLOG(REQ, LOUD,
		       "parse ap-start error (u4Enable) u4Ret=%d\n",
		       u4Ret);

	set_nan_handler(prNetDev, u4Enable, FALSE);

	return 0;
}

int
priv_driver_get_master_ind(struct net_device *prNetDev, char *pcCommand,
			   int i4TotalLen) {
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	uint32_t i4BytesWritten = 0;
	struct _NAN_SPECIFIC_BSS_INFO_T *prNANSpecInfo =
		(struct _NAN_SPECIFIC_BSS_INFO_T *)NULL;
	uint8_t ucMasterPreference = 0;
	uint8_t ucRandomFactor = 0;

	if (!prNetDev) {
		DBGLOG(NAN, ERROR, "prNetDev error!\n");
		return -1;
	}

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **)netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	prAdapter = prGlueInfo->prAdapter;
	prNANSpecInfo = nanGetSpecificBssInfo(prAdapter, NAN_BSS_INDEX_BAND0);

	if (prNANSpecInfo == NULL) {
		i4BytesWritten = -1;
		return i4BytesWritten;
	}

	prNANSpecInfo = nanGetSpecificBssInfo(prAdapter, NAN_BSS_INDEX_BAND0);

	ucMasterPreference = prNANSpecInfo->rMasterIndAttr.ucMasterPreference;
	ucRandomFactor = prNANSpecInfo->rMasterIndAttr.ucRandomFactor;

	LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
	       "\nMasterPreference: %d\n", ucMasterPreference);
	LOGBUF(pcCommand, i4TotalLen, i4BytesWritten, "RandomFactor: %d\n",
	       ucRandomFactor);

	return i4BytesWritten;
}

int
priv_driver_get_range(struct net_device *prNetDev, char *pcCommand,
		      int i4TotalLen) {
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	uint32_t i4BytesWritten = 0;
	struct _NAN_SPECIFIC_BSS_INFO_T *prNANSpecInfo =
		(struct _NAN_SPECIFIC_BSS_INFO_T *)NULL;
	struct dl_list *ranging_list;
	struct _NAN_RANGING_INSTANCE_T *prRanging = NULL;
	int32_t range_measurement_cm;

	if (!prNetDev) {
		DBGLOG(NAN, ERROR, "prNetDev error!\n");
		return -1;
	}

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **)netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	prAdapter = prGlueInfo->prAdapter;
	prNANSpecInfo = nanGetSpecificBssInfo(prAdapter, NAN_BSS_INDEX_BAND0);

	if (prNANSpecInfo == NULL) {
		i4BytesWritten = -1;
		return i4BytesWritten;
	}

	ranging_list = &prAdapter->rRangingInfo.ranging_list;

	dl_list_for_each(prRanging, ranging_list,
			 struct _NAN_RANGING_INSTANCE_T, list) {
		if (prRanging == NULL)
			return -1;

		range_measurement_cm =
			prRanging->ranging_ctrl.range_measurement_cm;

		if (range_measurement_cm) {
			LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
			       "\nPeer Addr: " MACSTR
			       ", Range: %d cm\n",
			       MAC2STR(prRanging->ranging_ctrl
					       .aucPeerAddr),
			       range_measurement_cm);
		} else {
			LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
			       "\nPeer Addr: " MACSTR
			       ", No valid range\n",
			       MAC2STR(prRanging->ranging_ctrl
					       .aucPeerAddr));
		}

	}

	return i4BytesWritten;
}

int
priv_driver_set_faw_config(struct net_device *prNetDev, char *pcCommand,
			   int i4TotalLen) {
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	uint32_t u4Ret;
	int32_t i4ArgNum = 3;
	uint8_t ucChnl = 0;
	uint32_t u4SlotBitmap = 0;
	enum ENUM_BAND eBand;

	if (!prNetDev) {
		DBGLOG(NAN, ERROR, "prNetDev error!\n");
		return -1;
	}

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **)netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc >= i4ArgNum) {
		u4Ret = kalkStrtou32(apcArgv[1], 0, &(eBand));
		if (u4Ret) {
			DBGLOG(REQ, LOUD,
			       "parse FAW CONFIG band error u4Ret=%d\n",
			       u4Ret);
			return -1;
		}

		u4Ret = kalkStrtou8(apcArgv[2], 0, &(ucChnl));
		if (u4Ret) {
			DBGLOG(REQ, LOUD,
			       "parse FAW CONFIG channel error u4Ret=%d\n",
			       u4Ret);
			return -1;
		}

		u4Ret = kalkStrtou32(apcArgv[3], 0, &(u4SlotBitmap));
		if (u4Ret) {
			DBGLOG(REQ, LOUD,
			       "parse FAW CONFIG slotBitmap error u4Ret=%d\n",
			       u4Ret);
			return -1;
		}

		nanSchedNegoCustFawConfigCmd(prGlueInfo->prAdapter,
			ucChnl, eBand,
			u4SlotBitmap);
	}

	return 0;
}

int
priv_driver_set_faw_reset(struct net_device *prNetDev, char *pcCommand,
			  int i4TotalLen) {
	struct GLUE_INFO *prGlueInfo = NULL;

	prGlueInfo = *((struct GLUE_INFO **)netdev_priv(prNetDev));

	nanSchedNegoCustFawResetCmd(prGlueInfo->prAdapter);

	return 0;
}

int
priv_driver_set_faw_apply(struct net_device *prNetDev, char *pcCommand,
			  int i4TotalLen) {
	struct GLUE_INFO *prGlueInfo = NULL;

	prGlueInfo = *((struct GLUE_INFO **)netdev_priv(prNetDev));

	nanSchedNegoCustFawApplyCmd(prGlueInfo->prAdapter);

	return 0;
}

int priv_driver_get_nan_stat(struct net_device *prNetDev, char *pcCommand,
				int i4TotalLen)
{
	struct ADAPTER *prAdapter = NULL;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct _NAN_SPECIFIC_BSS_INFO_T *prNANSpecInfo =
		(struct _NAN_SPECIFIC_BSS_INFO_T *)NULL;
	struct BSS_INFO *prBssInfo = (struct BSS_INFO *)NULL;
	struct _NAN_DATA_PATH_INFO_T *prDataPathInfo;
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
	struct _NAN_NDP_INSTANCE_T *prNDP = NULL;
	struct dl_list *ranging_list = NULL;
	struct _NAN_RANGING_INSTANCE_T *prRng = NULL;
	struct _NAN_RANGING_CTRL_T *prRngCtrl = NULL;
	int32_t i4Argc = 0, i4BytesWritten = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	uint8_t i = 0, j = 0;

	if (!prNetDev) {
		DBGLOG(NAN, ERROR, "prNetDev error!\n");
		return -1;
	}

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **)netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	if (!prAdapter) {
		DBGLOG(REQ, ERROR, "prAdapter error\n");
		return -1;
	}

	prNANSpecInfo = nanGetSpecificBssInfo(prAdapter, NAN_BSS_INDEX_BAND0);

	if (prNANSpecInfo == NULL) {
		DBGLOG(NAN, ERROR, "prNANSpecInfo is NULL\n");
		return 0;
	}
	prBssInfo =
		GET_BSS_INFO_BY_INDEX(prAdapter, prNANSpecInfo->ucBssIndex);

	if (!prBssInfo) {
		DBGLOG(REQ, ERROR, "NAN Invalid BSS_INFO\n");
		return -1;
	}

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
		"\n[NAN Info]\n");
	LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
		"Cluster["MACSTR"] [NSS:%d] %s %s\n",
		MAC2STR(prNANSpecInfo->aucClusterId),
		prAdapter->rWifiVar.ucNSS,
		prBssInfo->ucPhyTypeSet & PHY_TYPE_BIT_HT ? "[HT]" : "",
		prBssInfo->ucPhyTypeSet & PHY_TYPE_BIT_VHT ? "[VHT]" : "");

	LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
		"\nMaster Preference %u, Random Factor %u\n",
		prNANSpecInfo->rMasterIndAttr.ucMasterPreference,
		prNANSpecInfo->rMasterIndAttr.ucRandomFactor);

	/* Data Path */
	LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
		"\n[NDL Info]\n");
	LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
		"Maximum NDL Cache Size : %d\n", NAN_MAX_SUPPORT_NDL_NUM);
	LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
		"Maximum NDP Cache Size : %d\n", NAN_MAX_SUPPORT_NDP_NUM);

	prDataPathInfo = &(prAdapter->rDataPathInfo);
	for (i = 0; i < NAN_MAX_SUPPORT_NDL_NUM; i++) {
		prNDL = &(prDataPathInfo->arNDL[i]);
		if (prNDL->fgNDLValid == FALSE)
			continue;
		LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
			"Peer[" MACSTR "], Idx:%d, %s\n",
			MAC2STR(prNDL->aucPeerMacAddr), i,
			prNDL->eNDLRole == NAN_PROTOCOL_INITIATOR ?
				"[INITIATOR]" : "[RESPONDER]");
		LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
				"--- [NDP Info]\n");
		for (j = 0; j < NAN_MAX_SUPPORT_NDP_NUM; j++) {
			prNDP  = &(prNDL->arNDP[j]);
			if (prNDP->fgNDPValid == FALSE)
				continue;
			LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
				"--- Peer[" MACSTR "], Idx:%d, %s %s State:[%u]->[%u], NdpID:%u, PubID:%u\n",
				MAC2STR(prNDP->aucPeerNDIAddr), j,
				prNDP->fgNDPActive ? "[ACTIVE]" : "[INACTIVE]",
				prNDP->eNDPRole == NAN_PROTOCOL_INITIATOR ?
					"[INITIATOR]" : "[RESPONDER]",
				prNDP->eLastNDPProtocolState,
				prNDP->eCurrentNDPProtocolState,
				prNDP->ucNDPID,
				prNDP->ucPublishId);
		}
	}

	/* Raging */
	LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
			"\n[RANGING Info]\n");

	ranging_list = &prAdapter->rRangingInfo.ranging_list;
	dl_list_for_each(prRng, ranging_list,
			struct _NAN_RANGING_INSTANCE_T, list) {
		if (prRng == NULL)
			return -1;

		prRngCtrl = &prRng->ranging_ctrl;
		LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
			"\nPeer[" MACSTR "] %s State:[%u] Range: %u cm\n",
			MAC2STR(prRng->ranging_ctrl.aucPeerAddr),
			prRngCtrl->ucRole == NAN_PROTOCOL_INITIATOR ?
				"[INITIATOR]" : "[RESPONDER]",
			prRngCtrl->eCurrentState,
			prRngCtrl->range_measurement_cm ?
				prRngCtrl->range_measurement_cm : 0);
	}

	return i4BytesWritten;
}
#endif

int priv_driver_get_linkspeed(struct net_device *prNetDev,
			      char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct PARAM_LINK_SPEED_EX rLinkSpeed;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	uint32_t u4Rate = 0;
	int32_t i4BytesWritten = 0;
	uint8_t ucBssIndex = wlanGetBssIdx(prNetDev);

	ASSERT(prNetDev);

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	if (!netif_carrier_ok(prNetDev))
		return -1;

	if (ucBssIndex >= MAX_BSSID_NUM)
		return -EFAULT;

	kalMemSet(&rLinkSpeed, 0, sizeof(rLinkSpeed));
	DBGLOG(REQ, TRACE, "&rLinkSpeed=%p, sizeof=%zu, &u4BufLen=%p",
		&rLinkSpeed, sizeof(rLinkSpeed), &u4BufLen);
	rStatus = kalIoctlByBssIdx(prGlueInfo, wlanoidQueryLinkSpeed,
			   &rLinkSpeed, sizeof(rLinkSpeed), &u4BufLen,
			   ucBssIndex);
	DBGLOG(REQ, TRACE, "kalIoctlByBssIdx()=%u, prGlueInfo=%p, u4BufLen=%u",
		rStatus, prGlueInfo, u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS)
		return -1;
	u4Rate = rLinkSpeed.rLq[ucBssIndex].u2TxLinkSpeed;
	i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen, "TxLinkSpeed %u",
				  (unsigned int)(u4Rate * 100));
	DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__, pcCommand);
	return i4BytesWritten;

}				/* priv_driver_get_linkspeed */

int priv_driver_set_band(struct net_device *prNetDev, char *pcCommand,
			 int i4TotalLen)
{
	struct ADAPTER *prAdapter = NULL;
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4Argc = 0;
	uint32_t ucBand = 0;
	enum ENUM_BAND eBand = BAND_NULL;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	int32_t u4Ret = 0;
#if CFG_SUPPORT_NCHO
	uint32_t u4BufLen = 0;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
#endif

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	prAdapter = prGlueInfo->prAdapter;
	if (i4Argc >= 2) {
		u4Ret = kalkStrtou32(apcArgv[1], 0, &ucBand);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse ucBand error u4Ret=%d\n",
			       u4Ret);

		eBand = BAND_NULL;
		if (ucBand == CMD_BAND_TYPE_5G)
			eBand = BAND_5G;
		else if (ucBand == CMD_BAND_TYPE_2G)
			eBand = BAND_2G4;

		prAdapter->aePreferBand[KAL_NETWORK_TYPE_AIS_INDEX] = eBand;

#if CFG_SUPPORT_NCHO
		/* Handle NCHO setband */
		rStatus = kalIoctl(prGlueInfo,
			wlanoidSetNchoBand,
			&ucBand, sizeof(ucBand), &u4BufLen);
		if (rStatus != WLAN_STATUS_SUCCESS)
			return -1;
#endif
	}

	return 0;
}

int priv_driver_set_country(struct net_device *prNetDev,
			    char *pcCommand, int i4TotalLen)
{

	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint8_t aucCountry[2];

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (regd_is_single_sku_en()) {
		uint8_t i, count;
		struct COUNTRY_CODE_SETTING prCountrySetting = {0};

		/* command like "COUNTRY US", "COUNTRY US1" and
		 * "COUNTRY US01"
		 */
		count = kalStrnLen(apcArgv[1],
		sizeof(((struct COUNTRY_CODE_SETTING *)0)->aucCountryCode));

		for (i = 0; i < count; i++)
			prCountrySetting.aucCountryCode[i] = apcArgv[1][i];

		prCountrySetting.ucCountryLength = count;
		prCountrySetting.fgNeedHoldRtnlLock = 0;
		rStatus = kalIoctl(prGlueInfo,
					wlanoidSetCountryCode,
					&prCountrySetting,
					sizeof(struct COUNTRY_CODE_SETTING),
					&u4BufLen);

		if (rStatus != WLAN_STATUS_SUCCESS)
			return -1;

		return 0;
	}

	/* command like "COUNTRY US", "COUNTRY EU" and "COUNTRY JP" */
	aucCountry[0] = apcArgv[1][0];
	aucCountry[1] = apcArgv[1][1];

	rStatus = kalIoctl(prGlueInfo, wlanoidSetCountryCode,
			   &aucCountry[0], 2, &u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS)
		return -1;

	return 0;
}

#if CFG_ENABLE_WIFI_DIRECT
int priv_driver_set_csa(struct net_device *prNetDev,
				char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t ch_num = 0;
	uint32_t u4Ret = 0;
	uint8_t ucRoleIdx = 0, ucBssIdx = 0;
	int32_t i4BytesWritten = 0;
	enum ENUM_CSA_STATUS rStatus;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	if (mtk_Netdev_To_RoleIdx(prGlueInfo, prNetDev, &ucRoleIdx) != 0)
		return -1;
	if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter,
		ucRoleIdx, &ucBssIdx) !=
		WLAN_STATUS_SUCCESS)
		return -1;

	DBGLOG(REQ, INFO, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, INFO, "argc is %i\n", i4Argc);

	if (i4Argc >= 2) {
		struct BSS_INFO *bss =
			GET_BSS_INFO_BY_INDEX(
			prGlueInfo->prAdapter,
			ucBssIdx);
		enum ENUM_BAND eBand = BAND_NULL;

		if (bss == NULL)
			return -1;

		u4Ret = kalkStrtou32(apcArgv[1], 0, &ch_num);
		eBand = (ch_num <= 14) ? BAND_2G4 : BAND_5G;

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			    i4TotalLen - i4BytesWritten,
			    "\n[WARNING] This command only support CSA to 2G/5G, and will be deprecated in the future.\n\n");

		if (IS_BSS_APGO(bss)) {
#if CFG_SUPPORT_IDC_CH_SWITCH
			rStatus = p2pFuncIsCsaAllowed(prGlueInfo->prAdapter,
						      bss, ch_num, eBand);
			if (rStatus == CSA_STATUS_DFS_NOT_SUP)
				i4BytesWritten +=
					kalSnprintf(pcCommand + i4BytesWritten,
					    i4TotalLen - i4BytesWritten,
					    "\nNOT support DFS CH.\n");
			else if (rStatus == CSA_STATUS_NON_PSC_NOT_SUP)
				i4BytesWritten +=
					kalSnprintf(pcCommand + i4BytesWritten,
					    i4TotalLen - i4BytesWritten,
					    "\nNOT support 6G non-PSC CH.\n");
			else if (rStatus == CSA_STATUS_NON_SAE_NOT_SUP)
				i4BytesWritten +=
					kalSnprintf(pcCommand + i4BytesWritten,
					    i4TotalLen - i4BytesWritten,
					    "\nNOT support 6G non SAE Authentication.\n");
			else if (rStatus == CSA_STATUS_PEER_NOT_SUP_CSA)
				i4BytesWritten +=
					kalSnprintf(pcCommand + i4BytesWritten,
					    i4TotalLen - i4BytesWritten,
					    "\nPeer NOT support CSA.\n");
			else if (rStatus == CSA_STATUS_PEER_NOT_SUP_CH)
				i4BytesWritten +=
					kalSnprintf(pcCommand + i4BytesWritten,
					    i4TotalLen - i4BytesWritten,
					    "\nPeer NOT support CH.\n");

			if (rStatus != CSA_STATUS_SUCCESS)
				i4BytesWritten +=
					kalSnprintf(pcCommand + i4BytesWritten,
					    i4TotalLen - i4BytesWritten,
					    "\nRunning CSA, but NOT RECOMMENDED, which may cause to disconnect.\n");

			u4Ret = cnmIdcCsaReq(prGlueInfo->prAdapter,
				eBand, ch_num, ucRoleIdx);
#else
			DBGLOG(REQ, WARN, "Not support SAP/GO, do nothing!\n");
#endif
		} else if (IS_BSS_GC(bss))
			u4Ret = cnmOwnGcCsaReq(prGlueInfo->prAdapter,
				eBand, ch_num, ucRoleIdx);
		else
			DBGLOG(REQ, WARN, "Incorrect bss opmode\n");

		DBGLOG(REQ, INFO, "u4Ret is %d\n", u4Ret);
	} else {
		DBGLOG(REQ, INFO, "Input insufficent\n");
	}

	return i4BytesWritten;
}

int priv_driver_set_csa_ex(struct net_device *prNetDev,
				char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t ch_num = 0;
	uint32_t u4Ret = 0;
	uint8_t ucRoleIdx = 0, ucBssIdx = 0;
	enum ENUM_BAND eBand = BAND_NULL;
	int32_t i4BytesWritten = 0;
	enum ENUM_CSA_STATUS rStatus;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	if (mtk_Netdev_To_RoleIdx(prGlueInfo, prNetDev, &ucRoleIdx) != 0)
		return -1;
	if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter,
		ucRoleIdx, &ucBssIdx) !=
		WLAN_STATUS_SUCCESS)
		return -1;

	DBGLOG(REQ, INFO, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, INFO, "argc is %i\n", i4Argc);

	if (i4Argc >= 3) {
		struct BSS_INFO *bss =
			GET_BSS_INFO_BY_INDEX(
			prGlueInfo->prAdapter,
			ucBssIdx);

		if (bss == NULL)
			return -1;

		u4Ret = kalkStrtou32(apcArgv[1], 0, &eBand);
		u4Ret = kalkStrtou32(apcArgv[2], 0, &ch_num);

		if (IS_BSS_APGO(bss)) {
#if CFG_SUPPORT_IDC_CH_SWITCH
			rStatus = p2pFuncIsCsaAllowed(prGlueInfo->prAdapter,
						      bss, ch_num, eBand);
			if (rStatus == CSA_STATUS_DFS_NOT_SUP)
				i4BytesWritten +=
					kalSnprintf(pcCommand + i4BytesWritten,
					    i4TotalLen - i4BytesWritten,
					    "\nNOT support DFS CH.\n");
			else if (rStatus == CSA_STATUS_NON_PSC_NOT_SUP)
				i4BytesWritten +=
					kalSnprintf(pcCommand + i4BytesWritten,
					    i4TotalLen - i4BytesWritten,
					    "\nNOT support 6G non-PSC CH.\n");
			else if (rStatus == CSA_STATUS_NON_SAE_NOT_SUP)
				i4BytesWritten +=
					kalSnprintf(pcCommand + i4BytesWritten,
					    i4TotalLen - i4BytesWritten,
					    "\nNOT support 6G non SAE Authentication.\n");
			else if (rStatus == CSA_STATUS_PEER_NOT_SUP_CSA)
				i4BytesWritten +=
					kalSnprintf(pcCommand + i4BytesWritten,
					    i4TotalLen - i4BytesWritten,
					    "\nPeer NOT support CSA.\n");
			else if (rStatus == CSA_STATUS_PEER_NOT_SUP_CH)
				i4BytesWritten +=
					kalSnprintf(pcCommand + i4BytesWritten,
					    i4TotalLen - i4BytesWritten,
					    "\nPeer NOT support CH.\n");

			if (rStatus != CSA_STATUS_SUCCESS)
				i4BytesWritten +=
					kalSnprintf(pcCommand + i4BytesWritten,
					    i4TotalLen - i4BytesWritten,
					    "Running CSA, but NOT RECOMMENDED, which may cause to disconnect.\n");

			u4Ret = cnmIdcCsaReq(prGlueInfo->prAdapter,
				eBand, ch_num, ucRoleIdx);
#else
			DBGLOG(REQ, WARN, "Not support SAP/GO, do nothing!\n");
#endif
		} else if (IS_BSS_GC(bss))
			u4Ret = cnmOwnGcCsaReq(prGlueInfo->prAdapter,
				eBand, ch_num, ucRoleIdx);
		else
			DBGLOG(REQ, WARN, "Incorrect bss opmode\n");

		DBGLOG(REQ, INFO, "u4Ret is %d\n", u4Ret);
	} else {
		DBGLOG(REQ, INFO, "Input insufficent\n");
	}

	return i4BytesWritten;
}

int priv_driver_set_csa_ex_event(
	struct net_device *prNetDev,
	char *pcCommand,
	int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t ch_num = 0;
	uint32_t u4Ret = 0;
	uint8_t ucRoleIdx = 0, ucBssIdx = 0;
	enum ENUM_BAND eBand = BAND_NULL;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	if (mtk_Netdev_To_RoleIdx(prGlueInfo, prNetDev, &ucRoleIdx) != 0)
		return -1;
	if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter,
		ucRoleIdx, &ucBssIdx) !=
		WLAN_STATUS_SUCCESS)
		return -1;

	DBGLOG(REQ, INFO, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, INFO, "argc is %i\n", i4Argc);

	if (i4Argc >= 3) {
		struct WIFI_EVENT *pEvent;
		struct EVENT_GC_CSA_T *prEventBody;

		u4Ret = kalkStrtou32(apcArgv[1], 0, &eBand);
		u4Ret = kalkStrtou32(apcArgv[2], 0, &ch_num);

		pEvent = (struct WIFI_EVENT *)
			kalMemAlloc(sizeof(struct WIFI_EVENT)+
			sizeof(struct EVENT_GC_CSA_T),
			VIR_MEM_TYPE);
		if (!pEvent)
			return -1;

		prEventBody = (struct EVENT_GC_CSA_T *)
			&(pEvent->aucBuffer[0]);
		prEventBody->ucBssIndex = ucBssIdx;
		prEventBody->ucChannel = ch_num;
		prEventBody->ucBand = eBand;

		cnmPeerGcCsaHandler(prGlueInfo->prAdapter,
			(struct WIFI_EVENT *) pEvent);

		kalMemFree(pEvent,
			VIR_MEM_TYPE, sizeof(struct WIFI_EVENT)+
			sizeof(struct EVENT_GC_CSA_T));

		DBGLOG(REQ, INFO, "u4Ret is %d\n", u4Ret);
	} else {
		DBGLOG(REQ, INFO, "Input insufficent\n");
	}

	return 0;
}
#endif /* CFG_ENABLE_WIFI_DIRECT */

int priv_driver_get_country(struct net_device *prNetDev,
			    char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t i4BytesWritten = 0;
	uint32_t country = 0;
	char acCountryStr[MAX_COUNTRY_CODE_LEN + 1] = {0};

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);

	if (!regd_is_single_sku_en()) {
		LOGBUF(pcCommand, i4TotalLen, i4BytesWritten, "Not Supported.");
		return i4BytesWritten;
	}

	country = rlmDomainGetCountryCode();
	rlmDomainU32ToAlpha(country, acCountryStr);

	LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
		"\nCountry Code: %s (0x%x)", acCountryStr, country);

	return	i4BytesWritten;
}

int priv_driver_get_channels(struct net_device *prNetDev,
			     char *pcCommand, int i4TotalLen)
{
	uint32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
#if (CFG_SUPPORT_SINGLE_SKU == 1)
	uint32_t ch_idx, start_idx, end_idx;
	struct CMD_DOMAIN_CHANNEL *pCh;
	uint32_t ch_num = 0;
	uint8_t maxbw = 160;
	uint32_t u4Ret = 0;
#endif

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (!regd_is_single_sku_en()) {
		LOGBUF(pcCommand, i4TotalLen, i4BytesWritten, "Not Supported.");
		return i4BytesWritten;
	}

#if (CFG_SUPPORT_SINGLE_SKU == 1)
	/**
	 * Usage: iwpriv wlan0 driver "get_channels [2g |5g |ch_num]"
	 **/
	if (i4Argc >= 2 && (apcArgv[1][0] == '2') && (apcArgv[1][1] == 'g')) {
		start_idx = 0;
		end_idx = rlmDomainGetActiveChannelCount(KAL_BAND_2GHZ);
	} else if (i4Argc >= 2 && (apcArgv[1][0] == '5') &&
	    (apcArgv[1][1] == 'g')) {
		start_idx = rlmDomainGetActiveChannelCount(KAL_BAND_2GHZ);
		end_idx = rlmDomainGetActiveChannelCount(KAL_BAND_2GHZ)
				+ rlmDomainGetActiveChannelCount(KAL_BAND_5GHZ);
	}
#if (CFG_SUPPORT_WIFI_6G == 1)
	else if (i4Argc >= 2 && (apcArgv[1][0] == '6') &&
	    (apcArgv[1][1] == 'g')) {
		start_idx = rlmDomainGetActiveChannelCount(KAL_BAND_2GHZ)
				+ rlmDomainGetActiveChannelCount(KAL_BAND_5GHZ);
		end_idx = rlmDomainGetActiveChannelCount(KAL_BAND_2GHZ)
				+ rlmDomainGetActiveChannelCount(KAL_BAND_5GHZ)
				+ rlmDomainGetActiveChannelCount(KAL_BAND_6GHZ);
	}
#endif
	else {
		start_idx = 0;
		end_idx = rlmDomainGetActiveChannelCount(KAL_BAND_2GHZ)
				+ rlmDomainGetActiveChannelCount(KAL_BAND_5GHZ)
#if (CFG_SUPPORT_WIFI_6G == 1)
				+ rlmDomainGetActiveChannelCount(KAL_BAND_6GHZ)
#endif
				;
		if (i4Argc >= 2)
			/* Dump only specified channel */
			u4Ret = kalkStrtou32(apcArgv[1], 0, &ch_num);
	}

	if (regd_is_single_sku_en()) {
		LOGBUF(pcCommand, i4TotalLen, i4BytesWritten, "\n");

		for (ch_idx = start_idx; ch_idx < end_idx; ch_idx++) {

			pCh = (rlmDomainGetActiveChannels() + ch_idx);
			maxbw = 160;

			if (ch_num && (ch_num != pCh->u2ChNum))
				continue; /*show specific channel information*/

			/* Channel number */
			LOGBUF(pcCommand, i4TotalLen, i4BytesWritten, "CH-%d:",
					pCh->u2ChNum);

			/* Active/Passive */
			if (pCh->eFlags & IEEE80211_CHAN_PASSIVE_FLAG) {
				/* passive channel */
				LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
				       " " IEEE80211_CHAN_PASSIVE_STR);
			} else
				LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
				       " ACTIVE");

			/* Max BW */
			if (kalIsChFlagMatch(pCh->eFlags, CHAN_NO_160MHZ))
				maxbw = 80;
			if (kalIsChFlagMatch(pCh->eFlags, CHAN_NO_80MHZ))
				maxbw = 40;
			if (kalIsChFlagMatch(pCh->eFlags, CHAN_NO_HT40))
				maxbw = 20;

			LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
			       " BW_%dMHz", maxbw);

			/* Channel flags */
			if (pCh->eFlags & IEEE80211_CHAN_RADAR)
				LOGBUF(pcCommand, i4TotalLen,
					i4BytesWritten, ", DFS");
			LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
			       "  (flags=0x%x)\n", pCh->eFlags);
		}
	}
#endif

	return i4BytesWritten;
}
#if (CFG_SUPPORT_TRX_LIMITED_CONFIG == 1)
int priv_driver_set_force_trx_config(struct net_device *prNetDev,
				char *pcCommand, int i4TotalLen)
{
	int32_t i4BytesWritten = 0;
	struct GLUE_INFO *prGlueInfo = NULL;
	uint8_t ucRoleIdx = 0, ucBssIdx = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint8_t ucScenarioConfig;
	uint32_t u4Ret = 0;
#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT
	struct PARAM_TX_PWR_CTRL_IOCTL rPwrCtrlParam = { 0 };
	char arNameStr[] = "_SAR_PwrLevel";
	char arSettingStr2G[] = "[2G4,20]";
	char arSettingStr5G[] = "[5G,20]";
	uint32_t u4SetInfoLen = 0;
#endif

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	if (!prGlueInfo)
		return -1;
	if (mtk_Netdev_To_RoleIdx(prGlueInfo, prNetDev, &ucRoleIdx) != 0)
		return -1;
	if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter, ucRoleIdx, &ucBssIdx) !=
		WLAN_STATUS_SUCCESS)
		return -1;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);

	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, INFO, "argc is %i\n", i4Argc);

	u4Ret = kalkStrtou8(apcArgv[1], 0, &ucScenarioConfig);

	p2pFuncSetForceTrxConfig(prGlueInfo->prAdapter,
			ucBssIdx,
			ucScenarioConfig);
#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT
	if (ucScenarioConfig ==
		P2P_FORCE_TRX_CONFIG_MCS7) {
		rPwrCtrlParam.fgApplied = TRUE;
		rPwrCtrlParam.index = 1;
		rPwrCtrlParam.name = arNameStr;
		rPwrCtrlParam.newSetting = arSettingStr2G;

		prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
		kalIoctl(prGlueInfo,
			 wlanoidTxPowerControl,
			 (void *)&rPwrCtrlParam,
			 sizeof(struct PARAM_TX_PWR_CTRL_IOCTL),
			 &u4SetInfoLen);

		rPwrCtrlParam.newSetting = arSettingStr5G;
		kalIoctl(prGlueInfo,
			 wlanoidTxPowerControl,
			 (void *)&rPwrCtrlParam,
			 sizeof(struct PARAM_TX_PWR_CTRL_IOCTL),
			 &u4SetInfoLen);
	} else {
		rPwrCtrlParam.fgApplied = FALSE;
		rPwrCtrlParam.index = 0;
		rPwrCtrlParam.name = arNameStr;
		rPwrCtrlParam.newSetting = NULL;
	}
#endif

	return i4BytesWritten;
}
#endif

#if (CFG_SUPPORT_DFS_MASTER == 1)
int priv_driver_set_rdd_op_mode(struct net_device *prNetDev,
				char *pcCommand, int i4TotalLen)
{
	struct WIFI_EVENT *pEvent = NULL;
	struct EVENT_RDD_OPMODE_CHANGE *prEventBody;
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	uint8_t ucRoleIdx = 0, ucBssIdx = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret = 0;

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	if (!prGlueInfo)
		goto error;
	if (mtk_Netdev_To_RoleIdx(prGlueInfo, prNetDev, &ucRoleIdx) != 0)
		goto error;
	if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter, ucRoleIdx, &ucBssIdx) !=
		WLAN_STATUS_SUCCESS)
		goto error;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	pEvent = (struct WIFI_EVENT *) kalMemAlloc(sizeof(struct WIFI_EVENT)+
		sizeof(struct EVENT_RDD_OPMODE_CHANGE), VIR_MEM_TYPE);
	if (!pEvent)
		goto error;

	prEventBody = (struct EVENT_RDD_OPMODE_CHANGE *)
		&(pEvent->aucBuffer[0]);
	prEventBody->u2Tag = 2;
	prEventBody->u2EvtLen = 0;
	prEventBody->ucBssBitmap = BIT(3);
	prEventBody->ucEnable = 1;
	prEventBody->ucOpTxNss = 1;
	prEventBody->ucOpRxNss = 1;
	prEventBody->ucAction = 0;

	prEventBody->ucReason = 0;
	prEventBody->ucPriChannel = 36;
	prEventBody->ucChBw = 2;

	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, INFO, "argc is %i\n", i4Argc);

	u4Ret = kalkStrtou8(apcArgv[1], 0, &prEventBody->ucPriChannel);
	u4Ret = kalkStrtou8(apcArgv[2], 0, &prEventBody->ucChBw);
	u4Ret = kalkStrtou8(apcArgv[3], 0, &prEventBody->ucOpTxNss);
	u4Ret = kalkStrtou8(apcArgv[4], 0, &prEventBody->ucOpRxNss);
	u4Ret = kalkStrtou8(apcArgv[5], 0, &prEventBody->ucAction);

	DBGLOG(P2P, INFO,
		"prEventBody.ucVersion = %d\n",
		prEventBody->u2Tag);

	cnmRddOpmodeEventHandler(prGlueInfo->prAdapter,
		(struct WIFI_EVENT *) pEvent);
	kalMemFree(pEvent,
		VIR_MEM_TYPE, sizeof(struct WIFI_EVENT)+
		sizeof(struct EVENT_RDD_OPMODE_CHANGE));

	LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
		"\n rdd op change event 1");

	return i4BytesWritten;
error:
	if (pEvent) {
		kalMemFree(pEvent,
			VIR_MEM_TYPE, sizeof(struct WIFI_EVENT)+
			sizeof(struct EVENT_RDD_OPMODE_CHANGE));
	}
	return -1;
}

int priv_driver_set_dfs_channel_available(
				struct net_device *prNetDev,
				char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t u4Ret = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	int32_t i4BytesWritten = 0;
	uint8_t ucChannel = 0;
	uint8_t ucBw = 0;
	uint8_t ucAvailable = 0;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));


	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc >= 4) {
		u4Ret = kalkStrtou8(apcArgv[1], 0, &ucAvailable);
		if (u4Ret) {
			DBGLOG(REQ, ERROR, "parse argc[1] error u4Ret=%d\n",
			       u4Ret);
			return -1;
		}

		u4Ret = kalkStrtou8(apcArgv[2], 0, &ucChannel);
		if (u4Ret) {
			DBGLOG(REQ, ERROR, "parse argc[2] error u4Ret=%d\n",
			       u4Ret);
			return -1;
		}

		u4Ret = kalkStrtou8(apcArgv[3], 0, &ucBw);
		if (u4Ret) {
			DBGLOG(REQ, ERROR, "parse argc[3] error u4Ret=%d\n",
			       u4Ret);
			return -1;
		}

		p2pFuncSetDfsChannelAvailable(prGlueInfo->prAdapter,
			ucAvailable, ucChannel, ucBw);
	} else if (i4Argc >= 3) {
		u4Ret = kalkStrtou8(apcArgv[1], 0, &ucChannel);
		if (u4Ret) {
			DBGLOG(REQ, ERROR, "parse argc[1] error u4Ret=%d\n",
			       u4Ret);
			return -1;
		}

		u4Ret = kalkStrtou8(apcArgv[2], 0, &ucAvailable);
		if (u4Ret) {
			DBGLOG(REQ, ERROR, "parse argc[2] error u4Ret=%d\n",
			       u4Ret);
			return -1;
		}

		p2pFuncSetDfsChannelAvailable(prGlueInfo->prAdapter,
			ucAvailable, ucChannel, MAX_BW_20MHZ);
	}

	return	i4BytesWritten;
}

int priv_driver_show_dfs_state(struct net_device *prNetDev,
			       char *pcCommand, int i4TotalLen)
{
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	int32_t i4BytesWritten = 0;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	LOGBUF(pcCommand, i4TotalLen, i4BytesWritten, "\nDFS State: \"%s\"",
			p2pFuncShowDfsState());

	return	i4BytesWritten;
}

int priv_driver_show_dfs_help(struct net_device *prNetDev,
			      char *pcCommand, int i4TotalLen)
{
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	int32_t i4BytesWritten = 0;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);


	LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
	       "\n--iwpriv wlanX driver \"show_dfs_state\"\n");
	LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
	       "\nINACTIVE: RDD disable or temporary RDD disable");
	LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
	       "\nCHECKING: During CAC time");
	LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
	       "\nACTIVE  : In-serive monitoring");
	LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
	       "\nDETECTED: Has detected radar but hasn't moved to new channel\n");

	LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
	       "\n--iwpriv wlanX driver \"show_dfs_cac_time\"\n");
	LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
	       "\nShow the remaining time of CAC\n");

	LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
	       "\n--iwpriv wlanX set ByPassCac=yy\n");
	LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
	       "\nValue yy: set the time of CAC\n");

	LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
	       "\n--iwpriv wlanX set RDDReport=yy\n");
	LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
	       "\nValue yy is \"0\" or \"1\"");
	LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
	       "\n\"0\": Emulate RDD0 manual radar event");
	LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
	       "\n\"1\": Emulate RDD1 manual radar event\n");

	LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
	       "\n--iwpriv wlanX set RadarDetectMode=yy\n");
	LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
	       "\nValue yy is \"0\" or \"1\"");
	LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
	       "\n\"0\": Switch channel when radar detected (default)");
	LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
	       "\n\"1\": Do not switch channel when radar detected");

	return	i4BytesWritten;
}

int priv_driver_show_dfs_cac_time(struct net_device *prNetDev,
				  char *pcCommand, int i4TotalLen)
{
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	int32_t i4BytesWritten = 0;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (p2pFuncGetDfsState() != DFS_STATE_CHECKING) {
		LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
		       "\nNot in CAC period");
		return i4BytesWritten;
	}

	LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
	       "\nRemaining time of CAC: %dsec, detect cnt: %d",
	       p2pFuncGetCacRemainingTime(),
	       p2pFuncGetRadarDetectCnt());

	return	i4BytesWritten;
}

int priv_driver_dfs_cac_start(struct net_device *prNetDev,
				  char *pcCommand, int i4TotalLen)
{
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	int32_t i4BytesWritten = 0;
	uint32_t u4Ret = 0;
	uint32_t ucBw = 0;
	uint32_t ucBwVht = 0;
	uint32_t ucCh = 0;
	struct GLUE_INFO *prGlueInfo = NULL;
	uint8_t ucRoleIdx = 0, ucBssIdx = 0;
	struct RF_CHANNEL_INFO rRfChnlInfo;
	struct MSG_P2P_DFS_CAC *prP2pStartCacMsg =
		(struct MSG_P2P_DFS_CAC *) NULL;
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo =
			(struct P2P_ROLE_FSM_INFO *) NULL;
	struct WIFI_VAR *prWifiVar;

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	if (mtk_Netdev_To_RoleIdx(prGlueInfo, prNetDev, &ucRoleIdx) != 0)
		return -1;

	if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter,
		ucRoleIdx, &ucBssIdx) !=
		WLAN_STATUS_SUCCESS)
		return -1;
	prWifiVar = &prGlueInfo->prAdapter->rWifiVar;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);


	if (i4Argc >= 3) {
		u4Ret = kalkStrtou32(apcArgv[1], 0, &ucBw);
		u4Ret = kalkStrtou32(apcArgv[2], 0, &ucCh);
	}
	DBGLOG(REQ, INFO, "u4Ret is %d\n", u4Ret);

	kalMemZero(&rRfChnlInfo, sizeof(struct RF_CHANNEL_INFO));

	rRfChnlInfo.ucChannelNum = ucCh;
	rRfChnlInfo.ucChnlBw = ucBw;
	rRfChnlInfo.u2PriChnlFreq =
		nicChannelNum2Freq(ucCh, BAND_5G) / 1000;
	rRfChnlInfo.u4CenterFreq1 =
		nicGetS1Freq(prGlueInfo->prAdapter,
			BAND_5G, ucCh, ucBw);
	rRfChnlInfo.u4CenterFreq2 = 0;

	prP2pRoleFsmInfo =
		P2P_ROLE_INDEX_2_ROLE_FSM_INFO(
		prGlueInfo->prAdapter, ucRoleIdx);
	if (prP2pRoleFsmInfo)
		prP2pRoleFsmInfo->eDfsChnlBw =
			ucBw;

	p2pFuncSetChannel(prGlueInfo->prAdapter,
		ucRoleIdx, &rRfChnlInfo);

	if ((rRfChnlInfo.eBand == BAND_5G) &&
		(p2pFuncGetDfsState() == DFS_STATE_DETECTED))
		p2pFuncSetDfsState(DFS_STATE_INACTIVE);

	p2pFuncSetRadarDetectMode(1);
	p2pFuncResetRadarDetectCnt();

	prP2pStartCacMsg = (struct MSG_P2P_DFS_CAC *)
		cnmMemAlloc(prGlueInfo->prAdapter,
			RAM_TYPE_MSG, sizeof(struct MSG_P2P_DFS_CAC));
	if (prP2pStartCacMsg == NULL)
		return	i4BytesWritten;

	if (ucBw > 1)
		ucBwVht = (enum ENUM_CHANNEL_WIDTH)ucBw-1;
	else
		ucBwVht = CW_20_40MHZ;
	prP2pStartCacMsg->rMsgHdr.eMsgId = MID_MNY_P2P_START_CAC;
	prP2pStartCacMsg->eChannelWidth = (ucBwVht);
	prP2pStartCacMsg->ucRoleIdx = ucRoleIdx;

	mboxSendMsg(prGlueInfo->prAdapter,
		MBOX_ID_0,
		(struct MSG_HDR *) prP2pStartCacMsg,
		MSG_SEND_METHOD_BUF);
	prWifiVar->u4ByPassCacTimeBackup =
		prWifiVar->u4ByPassCacTime;
	prWifiVar->u4ByPassCacTime = 0;

	if (prWifiVar->u4ByPassCacTime) {
		p2pFuncEnableManualCac();
		p2pFuncSetDriverCacTime(prWifiVar->u4ByPassCacTime);
	} else {
		p2pFuncDisableManualCac();
		p2pFuncSetDriverCacTime(prWifiVar->u4ByPassCacTime);
	}

	DBGLOG(P2P, INFO, "start cac with ch %d and bw %d\n",
		ucCh, ucBw);

	return	i4BytesWritten;
}

int priv_driver_dfs_cac_stop(struct net_device *prNetDev,
				  char *pcCommand, int i4TotalLen)
{
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	int32_t i4BytesWritten = 0;
	struct GLUE_INFO *prGlueInfo = NULL;
	uint8_t ucRoleIdx = 0, ucBssIdx = 0;
	struct MSG_P2P_DFS_CAC *prP2pStopCacMsg =
		(struct MSG_P2P_DFS_CAC *) NULL;
	struct WIFI_VAR *prWifiVar;

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	if (mtk_Netdev_To_RoleIdx(prGlueInfo, prNetDev, &ucRoleIdx) != 0)
		return -1;

	if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter,
		ucRoleIdx, &ucBssIdx) !=
		WLAN_STATUS_SUCCESS)
		return -1;
	prWifiVar = &prGlueInfo->prAdapter->rWifiVar;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	prP2pStopCacMsg = (struct MSG_P2P_DFS_CAC *)
		cnmMemAlloc(prGlueInfo->prAdapter,
			RAM_TYPE_MSG, sizeof(struct MSG_P2P_DFS_CAC));
	if (prP2pStopCacMsg == NULL)
		return i4BytesWritten;

	prP2pStopCacMsg->rMsgHdr.eMsgId = MID_MNY_P2P_STOP_CAC;
	prP2pStopCacMsg->ucRoleIdx = ucRoleIdx;

	mboxSendMsg(prGlueInfo->prAdapter,
		MBOX_ID_0,
		(struct MSG_HDR *) prP2pStopCacMsg,
		MSG_SEND_METHOD_BUF);

	prWifiVar->u4ByPassCacTime =
		prWifiVar->u4ByPassCacTimeBackup;

	if (prWifiVar->u4ByPassCacTime) {
		p2pFuncEnableManualCac();
		p2pFuncSetDriverCacTime(prWifiVar->u4ByPassCacTime);
	} else {
		p2pFuncDisableManualCac();
		p2pFuncSetDriverCacTime(prWifiVar->u4ByPassCacTime);
	}

	return	i4BytesWritten;
}

int32_t _SetRddReport(struct net_device *prNetDev,
	uint8_t ucDbdcIdx)
{
	uint32_t u4BufLen = 0;
	struct PARAM_CUSTOM_SET_RDD_REPORT rSetRddReport;
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t i4Status = WLAN_STATUS_SUCCESS;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	kalMemSet(&rSetRddReport, 0,
		sizeof(struct PARAM_CUSTOM_SET_RDD_REPORT));

	/* Set Rdd Report */
	DBGLOG(INIT, INFO, "Set RDD Report - Band: %d\n",
		 ucDbdcIdx);
	rSetRddReport.ucDbdcIdx = ucDbdcIdx;

	i4Status = kalIoctl(prGlueInfo,
		wlanoidQuerySetRddReport, &rSetRddReport,
		sizeof(struct PARAM_CUSTOM_SET_RDD_REPORT), &u4BufLen);

	if (i4Status != WLAN_STATUS_SUCCESS)
		return -EFAULT;

	return i4Status;
}

int priv_driver_rddreport(struct net_device *prNetDev,
				char *pcCommand, int i4TotalLen)
{
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret = 0;
	uint8_t ucDbdcIdx = 0;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	DBGLOG(REQ, INFO, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, INFO, "argc is %i\n", i4Argc);

	if (p2pFuncGetDfsState() == DFS_STATE_INACTIVE
	    || p2pFuncGetDfsState() == DFS_STATE_DETECTED) {
		DBGLOG(REQ, ERROR,
			"RDD Report is not supported in this DFS state (inactive or deteted)\n");
		return -1;
	}

	if (i4Argc >= 2) {
		u4Ret = kalkStrtou8(apcArgv[1], 0, &ucDbdcIdx);
		if (u4Ret)
			DBGLOG(REQ, ERROR, "parse error u4Ret = %d\n", u4Ret);

		if (ucDbdcIdx > 1)
			ucDbdcIdx = 0;

		_SetRddReport(prNetDev, ucDbdcIdx);
	} else {
		DBGLOG(REQ, INFO, "Input insufficent\n");
	}

	return 0;
}

int32_t _SetRadarDetectMode(
	struct net_device *prNetDev,
	uint8_t ucRadarDetectMode)
{
	uint32_t u4BufLen = 0;
	struct PARAM_CUSTOM_SET_RADAR_DETECT_MODE
		rSetRadarDetectMode;
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t i4Status = WLAN_STATUS_SUCCESS;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	kalMemSet(&rSetRadarDetectMode, 0,
		  sizeof(struct PARAM_CUSTOM_SET_RADAR_DETECT_MODE));

	/* Set Rdd Report */
	DBGLOG(INIT, INFO, "Set Radar Detect Mode: %d\n",
		   ucRadarDetectMode);
	rSetRadarDetectMode.ucRadarDetectMode = ucRadarDetectMode;

	i4Status = kalIoctl(prGlueInfo,
		wlanoidQuerySetRadarDetectMode, &rSetRadarDetectMode,
		sizeof(struct PARAM_CUSTOM_SET_RADAR_DETECT_MODE), &u4BufLen);

	if (i4Status != WLAN_STATUS_SUCCESS)
		return -EFAULT;

	return i4Status;
}

int priv_driver_radarmode(struct net_device *prNetDev,
				char *pcCommand, int i4TotalLen)
{
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret = 0;
	uint8_t ucRadarDetectMode = 0;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	DBGLOG(REQ, INFO, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, INFO, "argc is %i\n", i4Argc);

	if (p2pFuncGetDfsState() == DFS_STATE_INACTIVE
		|| p2pFuncGetDfsState() == DFS_STATE_DETECTED) {
		DBGLOG(REQ, ERROR,
			"RDD Report is not supported in this DFS state (inactive or deteted)\n");
		return -1;
	}

	u4Ret = kalkStrtou8(apcArgv[1], 0, &ucRadarDetectMode);
	if (u4Ret)
		DBGLOG(REQ, ERROR, "parse error u4Ret = %d\n", u4Ret);

	if (ucRadarDetectMode >= 1)
		ucRadarDetectMode = 1;

	p2pFuncSetRadarDetectMode(ucRadarDetectMode);

	_SetRadarDetectMode(prNetDev, ucRadarDetectMode);

	return 0;
}

int priv_driver_radarevent(struct net_device *prNetDev,
				char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct EVENT_RDD_REPORT rEventBody;
	int32_t i4BytesWritten = 0;
	uint8_t ucRoleIdx = 0, ucBssIdx = 0;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	if (!prGlueInfo)
		goto error;
	if (mtk_Netdev_To_RoleIdx(prGlueInfo, prNetDev, &ucRoleIdx) != 0)
		goto error;
	if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter, ucRoleIdx, &ucBssIdx) !=
		WLAN_STATUS_SUCCESS)
		goto error;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);

	if (p2pFuncGetDfsState() != DFS_STATE_CHECKING) {
		LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
			"\nNot in CAC period");
	}

	cnmRadarDetectEvent(prGlueInfo->prAdapter,
		(struct WIFI_EVENT *) &rEventBody);

	LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
		"\nRemaining time of CAC: %dsec",
		p2pFuncGetCacRemainingTime());

	return	i4BytesWritten;

error:

	return -1;
}
#endif

#if CFG_SUPPORT_IDC_CH_SWITCH
int priv_driver_set_idc_bmp(struct net_device *prNetDev,
				char *pcCommand, int i4TotalLen)
{
	struct WIFI_EVENT *pEvent;
	struct EVENT_LTE_SAFE_CHN *prEventBody;
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	uint8_t ucRoleIdx = 0, ucBssIdx = 0;
	uint8_t ucIdx;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret = 0;
	uint8_t ucIdcBmpIdx[ENUM_SAFE_CH_MASK_MAX_NUM] = {0};

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	if (!prGlueInfo)
		goto error;
	if (mtk_Netdev_To_RoleIdx(prGlueInfo, prNetDev, &ucRoleIdx) != 0)
		goto error;
	if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter, ucRoleIdx, &ucBssIdx) !=
		WLAN_STATUS_SUCCESS)
		goto error;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	pEvent = (struct WIFI_EVENT *) kalMemAlloc(sizeof(struct WIFI_EVENT)+
		sizeof(struct EVENT_LTE_SAFE_CHN), VIR_MEM_TYPE);
	if (!pEvent)
		return -1;

	prEventBody = (struct EVENT_LTE_SAFE_CHN *) &(pEvent->aucBuffer[0]);
	prEventBody->ucVersion = 2;
	prEventBody->u4Flags = BIT(0);
	DBGLOG(P2P, INFO,
		"prEventBody.ucVersion = %d\n",
		prEventBody->ucVersion);
	DBGLOG(P2P, INFO,
		"prEventBody.u4Flags = 0x%08x\n",
		prEventBody->u4Flags);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, INFO, "argc is %i\n", i4Argc);

	u4Ret = kalkStrtou8(apcArgv[1], 0, &ucIdcBmpIdx[0]);
	u4Ret = kalkStrtou8(apcArgv[2], 0, &ucIdcBmpIdx[1]);
	u4Ret = kalkStrtou8(apcArgv[3], 0, &ucIdcBmpIdx[2]);
	u4Ret = kalkStrtou8(apcArgv[4], 0, &ucIdcBmpIdx[3]);
	DBGLOG(REQ, ERROR, "ucIdcBmpIdx = (%d,%d,%d,%d)\n",
		ucIdcBmpIdx[0],
		ucIdcBmpIdx[1],
		ucIdcBmpIdx[2],
		ucIdcBmpIdx[3]);

	/* Statistics from FW is valid */
	for (ucIdx = 0;
		ucIdx < ENUM_SAFE_CH_MASK_MAX_NUM;
		ucIdx++) {
		prEventBody->rLteSafeChn.
		au4SafeChannelBitmask[ucIdx] = BIT(ucIdcBmpIdx[ucIdx]);
		DBGLOG(P2P, INFO,
			"[CSA]LTE safe channels[%d]=0x%08x\n",
			ucIdx,
			prEventBody->rLteSafeChn.
			au4SafeChannelBitmask[ucIdx]);
	}
	cnmIdcDetectHandler(prGlueInfo->prAdapter,
		(struct WIFI_EVENT *) pEvent);
	kalMemFree(pEvent,
		VIR_MEM_TYPE, sizeof(struct WIFI_EVENT)+
		sizeof(struct EVENT_LTE_SAFE_CHN));

	LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
		"\n idv event 1");

	return i4BytesWritten;
error:

	return -1;
}
#endif

#if CFG_SUPPORT_WFD
int priv_driver_set_miracast(struct net_device *prNetDev,
			     char *pcCommand, int i4TotalLen)
{

	struct ADAPTER *prAdapter = NULL;
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t i4BytesWritten = 0;
	/* WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS; */
	/* UINT_32 u4BufLen = 0; */
	int32_t i4Argc = 0;
	uint32_t ucMode = 0;
	struct WFD_CFG_SETTINGS *prWfdCfgSettings =
				(struct WFD_CFG_SETTINGS *) NULL;
	struct MSG_WFD_CONFIG_SETTINGS_CHANGED *prMsgWfdCfgUpdate =
				(struct MSG_WFD_CONFIG_SETTINGS_CHANGED *) NULL;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	int32_t u4Ret = 0;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	prAdapter = prGlueInfo->prAdapter;
	if (i4Argc >= 2) {
		/* ucMode = kalStrtoul(apcArgv[1], NULL, 0); */
		u4Ret = kalkStrtou32(apcArgv[1], 0, &ucMode);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse ucMode error u4Ret=%d\n",
			       u4Ret);

		if (g_ucMiracastMode == (uint8_t) ucMode) {
			/* XXX: continue or skip */
			/* XXX: continue or skip */
		}

		g_ucMiracastMode = (uint8_t) ucMode;
		prMsgWfdCfgUpdate = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
				sizeof(struct MSG_WFD_CONFIG_SETTINGS_CHANGED));

		if (prMsgWfdCfgUpdate != NULL) {

			prWfdCfgSettings =
				&(prAdapter->rWifiVar.rWfdConfigureSettings);
			prMsgWfdCfgUpdate->rMsgHdr.eMsgId =
						MID_MNY_P2P_WFD_CFG_UPDATE;
			prMsgWfdCfgUpdate->prWfdCfgSettings = prWfdCfgSettings;

			if (ucMode == MIRACAST_MODE_OFF) {
				prWfdCfgSettings->ucWfdEnable = 0;
				kalSnprintf(pcCommand, i4TotalLen,
					 CMD_SET_CHIP " mira 0");
			} else if (ucMode == MIRACAST_MODE_SOURCE) {
				prWfdCfgSettings->ucWfdEnable = 1;
				kalSnprintf(pcCommand, i4TotalLen,
					 CMD_SET_CHIP " mira 1");
			} else if (ucMode == MIRACAST_MODE_SINK) {
				prWfdCfgSettings->ucWfdEnable = 2;
				kalSnprintf(pcCommand, i4TotalLen,
					 CMD_SET_CHIP " mira 2");
			} else {
				prWfdCfgSettings->ucWfdEnable = 0;
				kalSnprintf(pcCommand, i4TotalLen,
					 CMD_SET_CHIP " mira 0");
			}
			/* Customer request: Only scan active channels in
			 * WFD scenario
			*/
			if (prWfdCfgSettings->ucWfdEnable > 0) {
				DBGLOG(REQ, INFO,
					"[mtk] enable Mira, set SkipDFS\n");
				prAdapter->rWifiVar.rScanInfo.fgSkipDFS = 1;
			} else {
				DBGLOG(REQ, INFO,
					"[mtk] Disable Mira, unset SkipDFS\n");
				prAdapter->rWifiVar.rScanInfo.fgSkipDFS = 0;
			}

			/* Set mira mode to FW first */
			priv_driver_set_chip_config(prNetDev, pcCommand,
						    i4TotalLen);

			/* Update WFD settings including WMM parameters */
			mboxSendMsg(prAdapter, MBOX_ID_0, (struct MSG_HDR *)
				    prMsgWfdCfgUpdate, MSG_SEND_METHOD_BUF);
		} /* prMsgWfdCfgUpdate */
		else {
			ASSERT(FALSE);
			i4BytesWritten = -1;
		}
	}

	/* i4Argc */
	return i4BytesWritten;
}
#endif

int parseValueInString(
	char **pcCommand,
	const char *acDelim,
	void *aucValue,
	int u4MaxLen)
{
	uint8_t *pcPtr;
	uint32_t u4Len;
	uint8_t *pucValueHead = NULL;
	uint8_t *pucValueTail = NULL;

	if (*pcCommand
		&& !kalStrnCmp(*pcCommand, acDelim, kalStrLen(acDelim))) {
		pcPtr = kalStrSep(pcCommand, "=,");
		pucValueHead = *pcCommand;
		pcPtr = kalStrSep(pcCommand, "=,");
		DBGLOG(REQ, TRACE, "pucValueHead = %s\n", pucValueHead);
		if (pucValueHead) {
			u4Len = kalStrLen(pucValueHead);
			if (*pcCommand) {
				pucValueTail = *pcCommand - 1;
				u4Len = pucValueTail - pucValueHead;
			}
			if (u4Len > u4MaxLen)
				u4Len = u4MaxLen;

			/* MAC */
			if (!kalStrnCmp(acDelim, "MAC=", kalStrLen(acDelim))) {
				u8 *addr = aucValue;

				wlanHwAddrToBin(pucValueHead, addr);
				DBGLOG(REQ, TRACE, "MAC type");
			} else {
				u8 *addr = aucValue;

				kalStrnCpy(addr, pucValueHead, u4Len);
				*((char *)aucValue + u4Len) = '\0';
				DBGLOG(REQ, TRACE,
					"STR type = %s\n", (char *)aucValue);
			}
			return 0;
		}
	}

	return -1;
}

#if CFG_ENABLE_WIFI_DIRECT
int priv_driver_set_ap_set_mac_acl(struct net_device *prNetDev,
		char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	struct BSS_INFO *prBssInfo = NULL;
	uint8_t aucValue[WLAN_CFG_ARGV_MAX] = {0};
	uint8_t ucRoleIdx = 0, ucBssIdx = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Count = 0, i4Mode = 0;
	int i = 0;

	DBGLOG(REQ, INFO, "command is %s\n", pcCommand);

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		goto error;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	/* get Bss Index from ndev */
	if (mtk_Netdev_To_RoleIdx(prGlueInfo, prNetDev, &ucRoleIdx) != 0)
		goto error;
	if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter, ucRoleIdx, &ucBssIdx) !=
		WLAN_STATUS_SUCCESS)
		goto error;

	DBGLOG(REQ, INFO, "ucRoleIdx = %d\n", ucRoleIdx);
	prBssInfo = prAdapter->aprBssInfo[ucBssIdx];
	if (!prBssInfo) {
		DBGLOG(REQ, WARN, "bss is not active\n");
		goto error;
	}

	/* Mode */
	if (parseValueInString(&pcCommand,
		"MAC_MODE=", &aucValue, WLAN_CFG_ARGV_MAX)) {
		DBGLOG(REQ, ERROR, "[MODE] parse error\n");
		goto error;
	}
	if (kalkStrtou32(aucValue, 0, &i4Mode)) {
		DBGLOG(REQ, ERROR, "[MODE] convert to int error\n");
		goto error;
	}
	if (i4Mode == 0)
		prBssInfo->rACL.ePolicy = PARAM_CUSTOM_ACL_POLICY_DISABLE;
	else if (i4Mode == 1)
		prBssInfo->rACL.ePolicy = PARAM_CUSTOM_ACL_POLICY_DENY;
	else if (i4Mode == 2)
		prBssInfo->rACL.ePolicy = PARAM_CUSTOM_ACL_POLICY_ACCEPT;
	else {
		DBGLOG(REQ, ERROR, "[MODE] invalid ACL policy= %d\n", i4Mode);
		goto error;
	}

	/* Count */
	/* No need to parse count for diabled mode */
	if (prBssInfo->rACL.ePolicy != PARAM_CUSTOM_ACL_POLICY_DISABLE) {
		if (parseValueInString(&pcCommand,
			"MAC_CNT=", &aucValue, WLAN_CFG_ARGV_MAX)) {
			DBGLOG(REQ, ERROR, "[CNT] parse count error\n");
			goto error;
		}
		if (kalkStrtou32(aucValue, 0, &i4Count)) {
			DBGLOG(REQ, ERROR, "[CNT] convert to int error\n");
			goto error;
		}
		if (i4Count > MAX_NUMBER_OF_ACL) {
			DBGLOG(REQ, ERROR, "[CNT] invalid count > max ACL\n");
			goto error;
		}
	}

	/* MAC */
	if (prBssInfo->rACL.u4Num) {
		/* Clear */
		kalMemZero(&prBssInfo->rACL.rEntry[0],
			sizeof(struct PARAM_CUSTOM_ACL_ENTRY) * MAC_ADDR_LEN);
		prBssInfo->rACL.u4Num = 0;
	}

	if (prBssInfo->rACL.ePolicy != PARAM_CUSTOM_ACL_POLICY_DISABLE) {
		for (i = 0; i < i4Count; i++) {
			/* Add */
			if (parseValueInString(&pcCommand,
				"MAC=", &aucValue, WLAN_CFG_ARGV_MAX))
				break;
			kalMemCopy(prBssInfo->rACL.rEntry[i].aucAddr,
				&aucValue, MAC_ADDR_LEN);
			DBGLOG(REQ, INFO,
				"[MAC] add mac addr " MACSTR " to ACL(%d).\n",
				MAC2STR(prBssInfo->rACL.rEntry[i].aucAddr), i);
		}

		prBssInfo->rACL.u4Num = i;
		/* check ACL affects any existent association */
		p2pRoleUpdateACLEntry(prAdapter, ucBssIdx);
		DBGLOG(REQ, INFO,
			"[MAC] Mode = %d, #ACL = %d, count = %d\n",
			i4Mode, i, i4Count);
	}

	return i4BytesWritten;

error:
	return -1;
}

int priv_driver_set_ap_set_cfg(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	struct WIFI_VAR *prWifiVar = NULL;
	uint8_t aucValue[WLAN_CFG_ARGV_MAX] = {0};
	uint8_t ucRoleIdx = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4MaxCount = 0, i4Channel = 0;

	DBGLOG(REQ, INFO, "command is %s\n", pcCommand);

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		goto error;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;
	prWifiVar = &prAdapter->rWifiVar;

	/* get role index from ndev */
	if (mtk_Netdev_To_RoleIdx(prGlueInfo, prNetDev, &ucRoleIdx) != 0)
		goto error;

	DBGLOG(REQ, INFO, "ucRoleIdx = %d\n", ucRoleIdx);

	/* Cfg */
	if (parseValueInString(&pcCommand, "ASCII_CMD=",
		&aucValue, WLAN_CFG_ARGV_MAX)) {
		DBGLOG(REQ, ERROR, "[CFG] cmd parse error\n");
		goto error;
	}
	if (kalStrnCmp(aucValue, "AP_CFG", 6)) {
		DBGLOG(REQ, ERROR, "[CFG] sub cmd parse error\n");
		goto error;
	}

	/* Channel */
	if (parseValueInString(&pcCommand, "CHANNEL=",
		&aucValue, WLAN_CFG_ARGV_MAX)) {
		DBGLOG(REQ, ERROR, "[CH] parse error\n");
		goto error;
	}
	if (kalkStrtou32(aucValue, 0, &i4Channel)) {
		DBGLOG(REQ, ERROR, "[CH] convert to int error\n");
		goto error;
	}

	/* Max SCB */
	if (parseValueInString(&pcCommand, "MAX_SCB=",
		&aucValue, WLAN_CFG_ARGV_MAX)) {
		DBGLOG(REQ, ERROR, "[MAX_SCB] parse error\n");
		goto error;
	}
	if (kalkStrtou32(aucValue, 0, &i4MaxCount)) {
		DBGLOG(REQ, ERROR, "[MAX_SCB] convert to int error\n");
		goto error;
	}

	/* Overwrite AP channel */
	prWifiVar->ucApChannel = i4Channel;

	/* Set max clients of Hotspot */
	kalP2PSetMaxClients(prGlueInfo, i4MaxCount, ucRoleIdx);

	DBGLOG(REQ, INFO,
		"[CFG] CH = %d, MAX_SCB = %d\n",
		i4Channel, i4MaxCount);

	return i4BytesWritten;

error:
	return -1;
}

int priv_driver_set_ap_get_sta_list(struct net_device *prNetDev,
		char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	struct BSS_INFO *prBssInfo = NULL;
	struct LINK *prClientList;
	struct STA_RECORD *prCurrStaRec, *prNextStaRec;
	uint8_t ucRoleIdx = 0, ucBssIdx = 0;
	int32_t i4BytesWritten = 0;

	DBGLOG(REQ, INFO, "command is %s\n", pcCommand);

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		goto error;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	/* get Bss Index from ndev */
	if (mtk_Netdev_To_RoleIdx(prGlueInfo, prNetDev, &ucRoleIdx) != 0)
		goto error;
	if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter, ucRoleIdx, &ucBssIdx) !=
		WLAN_STATUS_SUCCESS)
		goto error;

	DBGLOG(REQ, INFO, "ucRoleIdx = %d\n", ucRoleIdx);
	prBssInfo = prAdapter->aprBssInfo[ucBssIdx];
	if (!prBssInfo) {
		DBGLOG(REQ, WARN, "bss is not active\n");
		goto error;
	}

	prClientList = &prBssInfo->rStaRecOfClientList;
	LINK_FOR_EACH_ENTRY_SAFE(prCurrStaRec,
		prNextStaRec, prClientList, rLinkEntry, struct STA_RECORD) {
		if (!prCurrStaRec) {
			DBGLOG(REQ, WARN, "NULL STA_REC\n");
			break;
		}
		DBGLOG(SW4, INFO, "STA[%u] [" MACSTR "]\n",
			prCurrStaRec->ucIndex,
			MAC2STR(prCurrStaRec->aucMacAddr));
		i4BytesWritten += kalSnprintf(
			pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			""MACSTR"\n",
			MAC2STR(prCurrStaRec->aucMacAddr));
	}

	return i4BytesWritten;

error:
	return -1;
}

int priv_driver_set_ap_sta_disassoc(struct net_device *prNetDev,
				  char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	uint8_t aucValue[WLAN_CFG_ARGV_MAX] = {0};
	uint8_t ucRoleIdx = 0;
	int32_t i4BytesWritten = 0;
	struct MSG_P2P_CONNECTION_ABORT *prDisconnectMsg =
		(struct MSG_P2P_CONNECTION_ABORT *) NULL;

	DBGLOG(REQ, INFO, "command is %s\n", pcCommand);

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		goto error;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	/* get role index from ndev */
	if (mtk_Netdev_To_RoleIdx(prGlueInfo, prNetDev, &ucRoleIdx) != 0)
		goto error;

	DBGLOG(REQ, INFO, "ucRoleIdx = %d\n", ucRoleIdx);

	if (parseValueInString(&pcCommand, "MAC=",
		&aucValue, WLAN_CFG_ARGV_MAX)) {
		DBGLOG(REQ, ERROR, "[MAC] parse error\n");
		goto error;
	}

	prDisconnectMsg =
		(struct MSG_P2P_CONNECTION_ABORT *)
		cnmMemAlloc(prGlueInfo->prAdapter, RAM_TYPE_MSG,
			sizeof(struct MSG_P2P_CONNECTION_ABORT));
	if (prDisconnectMsg == NULL) {
		ASSERT(FALSE);
		goto error;
	}

	prDisconnectMsg->rMsgHdr.eMsgId = MID_MNY_P2P_CONNECTION_ABORT;
	prDisconnectMsg->ucRoleIdx = ucRoleIdx;
	COPY_MAC_ADDR(prDisconnectMsg->aucTargetID, aucValue);
	prDisconnectMsg->u2ReasonCode = REASON_CODE_UNSPECIFIED;
	prDisconnectMsg->fgSendDeauth = TRUE;

	mboxSendMsg(prGlueInfo->prAdapter,
		MBOX_ID_0,
		(struct MSG_HDR *) prDisconnectMsg,
		MSG_SEND_METHOD_BUF);

	return i4BytesWritten;

error:
	return -1;
}
int priv_driver_set_ap_nss(struct net_device *prNetDev,
				char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct BSS_INFO *prBssInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret, u4Parse = 0;
	uint8_t ucNSS = 0;
	uint8_t ucBssIndex = 0;

	ASSERT(prNetDev);

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	if (!prGlueInfo)
		return -1;
	prAdapter = prGlueInfo->prAdapter;
	if (!prAdapter)
		return -1;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	ucBssIndex = wlanGetBssIdx(prNetDev);
	if (ucBssIndex >= MAX_BSSID_NUM)
		return -EFAULT;

	prBssInfo = prAdapter->aprBssInfo[ucBssIndex];
	if (!prBssInfo)
		return -EFAULT;

	if (i4Argc >= 1) {
		u4Ret = kalkStrtou32(apcArgv[i4Argc - 1], 0, &u4Parse);
		if (u4Ret) {
			DBGLOG(REQ, WARN, "parse apcArgv error u4Ret=%d\n",
				u4Ret);
			goto error;
		}
		ucNSS = (uint8_t) u4Parse;

		if ((ucNSS > 0) &&
			(ucNSS <= prAdapter->rWifiVar.ucNSS)) {
#if (CFG_SUPPORT_POWER_THROTTLING == 1 && CFG_SUPPORT_CNM_POWER_CTRL == 1)
			struct WIFI_EVENT *pEvent;
			struct EVENT_OPMODE_CHANGE *prEvtOpMode =
				(struct EVENT_OPMODE_CHANGE *) NULL;

			pEvent = (struct WIFI_EVENT *)
				kalMemAlloc(sizeof(struct WIFI_EVENT)+
				sizeof(struct EVENT_OPMODE_CHANGE),
				VIR_MEM_TYPE);
			if (!pEvent)
				goto error;

			prAdapter->rWifiVar.ucAp6gNSS = ucNSS;
			prAdapter->rWifiVar.ucAp5gNSS = ucNSS;
			prAdapter->rWifiVar.ucAp2gNSS = ucNSS;

			pEvent->ucEID = EVENT_ID_OPMODE_CHANGE;
			pEvent->ucSeqNum = 0;

			prEvtOpMode = (struct EVENT_OPMODE_CHANGE *)
				&(pEvent->aucBuffer[0]);
			prEvtOpMode->ucBssBitmap = BIT(ucBssIndex);
			prEvtOpMode->ucEnable = (ucNSS == 1);
			prEvtOpMode->ucOpTxNss = ucNSS;
			prEvtOpMode->ucOpRxNss = ucNSS;
			prEvtOpMode->ucReason =
				EVENT_OPMODE_CHANGE_REASON_USER_CONFIG;

			if (prEvtOpMode->ucEnable) {
				prAdapter->fgANTCtrl = true;
				prAdapter->ucANTCtrlReason =
					EVENT_OPMODE_CHANGE_REASON_ANT_CTRL;
				prAdapter->ucANTCtrlPendingCount = 0;
			}

			cnmOpmodeEventHandler(
				prAdapter,
				(struct WIFI_EVENT *) pEvent);

			kalMemFree(pEvent, VIR_MEM_TYPE,
				sizeof(struct WIFI_EVENT)+
				sizeof(struct EVENT_OPMODE_CHANGE));
#else
			prAdapter->rWifiVar.ucAp6gNSS = ucNSS;
			prAdapter->rWifiVar.ucAp5gNSS = ucNSS;
			prAdapter->rWifiVar.ucAp2gNSS = ucNSS;
#endif

			DBGLOG(REQ, STATE,
				"ApNSS = %d\n",
				prAdapter->rWifiVar.ucAp2gNSS);
	} else
		DBGLOG(REQ, WARN, "Invalid nss=%d\n",
			ucNSS);
	} else {
		DBGLOG(INIT, ERROR,
			"iwpriv wlanXX AP_SET_NSS <value>\n");
	}

	return i4BytesWritten;

error:
	return -1;
}

int priv_driver_set_ap_bw(struct net_device *prNetDev,
				char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret, u4Parse = 0;
	uint8_t ucBw;

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc >= 1) {
		u4Ret = kalkStrtou32(apcArgv[i4Argc - 1], 0, &u4Parse);
		if (u4Ret) {
			DBGLOG(REQ, WARN, "parse apcArgv error u4Ret=%d\n",
				u4Ret);
			goto error;
		}
		ucBw = (uint8_t) u4Parse;

		if (ucBw < MAX_BW_UNKNOWN) {
			prGlueInfo->prAdapter->rWifiVar.ucAp5gBandwidth = ucBw;

			DBGLOG(REQ, STATE,
				"ApBW = %d\n", ucBw);
	} else
			DBGLOG(REQ, WARN, "Invalid bw=%d\n",
				ucBw);
	} else {
		DBGLOG(INIT, ERROR,
		  "iwpriv apx SET_AP_BW <value>\n");
	}

	return i4BytesWritten;

error:
	return -1;
}

int
__priv_set_ap(struct net_device *prNetDev,
	struct iw_request_info *prIwReqInfo,
	union iwreq_data *prIwReqData, char *pcExtra)
{
	uint32_t u4SubCmd = 0;
	uint16_t u2Cmd = 0;
	int32_t i4TotalFixLen = 1024;
	int32_t i4CmdFound = 0;
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;

	ASSERT(prNetDev);
	ASSERT(prIwReqData);
	if (!prNetDev || !prIwReqData) {
		DBGLOG(REQ, INFO,
			"invalid param(0x%p, 0x%p)\n",
		prNetDev, prIwReqData);
		return -EINVAL;
	}

	u2Cmd = prIwReqInfo->cmd;
	DBGLOG(REQ, INFO, "prIwReqInfo->cmd %x\n", u2Cmd);

	u4SubCmd = (uint32_t) prIwReqData->data.flags;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	ASSERT(prGlueInfo);
	if (!prGlueInfo) {
		DBGLOG(REQ, INFO,
			"invalid prGlueInfo(0x%p, 0x%p)\n",
			prNetDev,
			*((struct GLUE_INFO **) netdev_priv(prNetDev)));
		return -EINVAL;
	}

	DBGLOG(REQ, INFO, "prIwReqData->data.length %u\n",
		prIwReqData->data.length);

	ASSERT(IW_IS_GET(u2Cmd));
	if (prIwReqData->data.length != 0) {
		if (!kal_access_ok(VERIFY_READ, prIwReqData->data.pointer,
			prIwReqData->data.length)) {
			DBGLOG(REQ, INFO,
				"%s access_ok Read fail written = %d\n",
				__func__, i4BytesWritten);
			return -EFAULT;
		}
		if (prIwReqData->data.length >
			CMD_OID_BUF_LENGTH) {
			DBGLOG(REQ, INFO,
				"illegal cmd length\n");
			return -EFAULT;
		}
		if (copy_from_user(aucOidBuf,
			prIwReqData->data.pointer,
			prIwReqData->data.length)) {
			DBGLOG(REQ, INFO,
				"%s copy_form_user fail written = %d\n",
				__func__,
				prIwReqData->data.length);
				return -EFAULT;
		}
		/* prIwReqData->data.length include the terminate '\0' */
		aucOidBuf[prIwReqData->data.length - 1] = 0;
	}

	DBGLOG(REQ, INFO, "%s aucBuf %s\n", __func__, aucOidBuf);

	if (!pcExtra)
		goto exit;

	i4CmdFound = 1;
	switch (u2Cmd) {
	case IOC_AP_GET_STA_LIST:
	i4BytesWritten =
		priv_driver_set_ap_get_sta_list(
		prNetDev,
		pcExtra,
		i4TotalFixLen);
		break;
	case IOC_AP_SET_MAC_FLTR:
	i4BytesWritten =
		priv_driver_set_ap_set_mac_acl(
		prNetDev,
		aucOidBuf,
		i4TotalFixLen);
	  break;
	case IOC_AP_SET_CFG:
	i4BytesWritten =
		priv_driver_set_ap_set_cfg(
		prNetDev,
		aucOidBuf,
		i4TotalFixLen);
	  break;
	case IOC_AP_STA_DISASSOC:
	i4BytesWritten =
		priv_driver_set_ap_sta_disassoc(
		prNetDev,
		aucOidBuf,
		i4TotalFixLen);
	  break;
	case IOC_AP_SET_NSS:
	i4BytesWritten =
		priv_driver_set_ap_nss(
		prNetDev,
		aucOidBuf,
		i4TotalFixLen);
	  break;
	case IOC_AP_SET_BW:
	i4BytesWritten =
		priv_driver_set_ap_bw(
		prNetDev,
		aucOidBuf,
		i4TotalFixLen);
	  break;
	default:
		i4CmdFound = 0;
		break;
	}

	if (i4CmdFound == 0)
		DBGLOG(REQ, INFO,
			"Unknown driver command\n");

	if (i4BytesWritten >= 0) {
		if ((i4BytesWritten == 0) && (i4TotalFixLen > 0)) {
			/* reset the command buffer */
			pcExtra[0] = '\0';
		} else if (i4BytesWritten >= i4TotalFixLen) {
			DBGLOG(REQ, INFO,
				"%s: i4BytesWritten %d > i4TotalFixLen < %d\n",
				__func__, i4BytesWritten, i4TotalFixLen);
			i4BytesWritten = i4TotalFixLen;
		} else {
			pcExtra[i4BytesWritten] = '\0';
			i4BytesWritten++;
		}
	}

	DBGLOG(REQ, INFO, "%s i4BytesWritten = %d\n", __func__,
		i4BytesWritten);

exit:

	DBGLOG(REQ, INFO, "pcExtra done\n");

	if (i4BytesWritten >= 0)
		prIwReqData->data.length = i4BytesWritten;

	return 0;
}

int
priv_set_ap(struct net_device *prNetDev,
	struct iw_request_info *prIwReqInfo,
	union iwreq_data *prIwReqData, char *pcExtra)
{
#if BUILD_QA_DBG
	return __priv_set_ap(prNetDev, prIwReqInfo,
		prIwReqData, pcExtra);
#else
	DBGLOG(REQ, WARN, "not support in user load");
	return 0;
#endif
}
#endif /* CFG_ENABLE_WIFI_DIRECT */

#if CFG_WOW_SUPPORT
int priv_driver_set_wow(struct net_device *prNetDev,
			       char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct WOW_CTRL *pWOW_CTRL = NULL;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	uint32_t u4Ret = 0;
	uint32_t Enable = 0;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	pWOW_CTRL = &prGlueInfo->prAdapter->rWowCtrl;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	u4Ret = kalkStrtou32(apcArgv[1], 0, &Enable);

	if (u4Ret) {
		DBGLOG(REQ, LOUD, "parse bEnable error u4Ret=%d\n", u4Ret);
		return -EINVAL;
	}

	DBGLOG(INIT, INFO, "CMD set_wow_enable = %d\n", Enable);
	DBGLOG(INIT, INFO, "Scenario ID %d\n", pWOW_CTRL->ucScenarioId);
	DBGLOG(INIT, INFO, "ucBlockCount %d\n",
	       pWOW_CTRL->ucBlockCount);
	DBGLOG(INIT, INFO, "interface %d\n",
	       pWOW_CTRL->astWakeHif[0].ucWakeupHif);
	DBGLOG(INIT, INFO, "gpio_pin %d\n",
	       pWOW_CTRL->astWakeHif[0].ucGpioPin);
	DBGLOG(INIT, INFO, "gpio_level 0x%x\n",
	       pWOW_CTRL->astWakeHif[0].ucTriggerLvl);
	DBGLOG(INIT, INFO, "gpio_timer %d\n",
	       pWOW_CTRL->astWakeHif[0].u4GpioInterval);
	kalWowProcess(prGlueInfo, Enable);

	return 0;
}

int priv_driver_set_wow_enable(struct net_device *prNetDev,
				      char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct WOW_CTRL *pWOW_CTRL = NULL;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	uint32_t u4Ret = 0;
	uint8_t ucEnable = 0;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	pWOW_CTRL = &prGlueInfo->prAdapter->rWowCtrl;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc >= 2) {
		u4Ret = kalkStrtou8(apcArgv[1], 0, &ucEnable);

		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse bEnable error u4Ret=%d\n",
				u4Ret);

		pWOW_CTRL->fgWowEnable = ucEnable;

		DBGLOG(PF, INFO, "WOW enable %d\n", pWOW_CTRL->fgWowEnable);

		return 0;
	} else
		return -1;
}

int priv_driver_set_wow_par(struct net_device *prNetDev,
				   char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct WOW_CTRL *pWOW_CTRL = NULL;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	int32_t u4Ret = 0;
	uint8_t	ucWakeupHif = 0, GpioPin = 0, ucGpioLevel = 0, ucBlockCount = 0,
		ucScenario = 0;
	uint32_t u4GpioTimer = 0;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	pWOW_CTRL = &prGlueInfo->prAdapter->rWowCtrl;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc >= 7) {

		u4Ret = kalkStrtou8(apcArgv[1], 0, &ucWakeupHif);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse ucWakeupHif error u4Ret=%d\n",
			       u4Ret);
		pWOW_CTRL->astWakeHif[0].ucWakeupHif = ucWakeupHif;

		u4Ret = kalkStrtou8(apcArgv[2], 0, &GpioPin);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse GpioPin error u4Ret=%d\n",
			       u4Ret);
		pWOW_CTRL->astWakeHif[0].ucGpioPin = GpioPin;

		u4Ret = kalkStrtou8(apcArgv[3], 0, &ucGpioLevel);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse Gpio level error u4Ret=%d\n",
			       u4Ret);
		pWOW_CTRL->astWakeHif[0].ucTriggerLvl = ucGpioLevel;

		u4Ret = kalkStrtou32(apcArgv[4], 0, &u4GpioTimer);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse u4GpioTimer error u4Ret=%d\n",
			       u4Ret);
		pWOW_CTRL->astWakeHif[0].u4GpioInterval = u4GpioTimer;

		u4Ret = kalkStrtou8(apcArgv[5], 0, &ucScenario);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse ucScenario error u4Ret=%d\n",
			       u4Ret);
		pWOW_CTRL->ucScenarioId = ucScenario;

		u4Ret = kalkStrtou8(apcArgv[6], 0, &ucBlockCount);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse ucBlockCnt error u4Ret=%d\n",
			       u4Ret);
		pWOW_CTRL->ucBlockCount = ucBlockCount;

		DBGLOG(INIT, INFO, "gpio_scenario%d\n",
		       pWOW_CTRL->ucScenarioId);
		DBGLOG(INIT, INFO, "interface %d\n",
		       pWOW_CTRL->astWakeHif[0].ucWakeupHif);
		DBGLOG(INIT, INFO, "gpio_pin %d\n",
		       pWOW_CTRL->astWakeHif[0].ucGpioPin);
		DBGLOG(INIT, INFO, "gpio_level %d\n",
		       pWOW_CTRL->astWakeHif[0].ucTriggerLvl);
		DBGLOG(INIT, INFO, "gpio_timer %d\n",
		       pWOW_CTRL->astWakeHif[0].u4GpioInterval);

		return 0;
	} else
		return -1;


}

int priv_driver_set_wow_udpport(struct net_device *prNetDev,
				       char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct WOW_CTRL *pWOW_CTRL = NULL;
	int32_t i4Argc = 0;
	int8_t *apcPortArgv[WLAN_CFG_ARGV_MAX_LONG] = { 0 }; /* to input 20 port
							      */
	int32_t u4Ret = 0, ii;
	uint8_t	ucVer = 0, ucCount;
	uint16_t u2Port = 0;
	uint16_t *pausPortArry;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	pWOW_CTRL = &prGlueInfo->prAdapter->rWowCtrl;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgumentLong(pcCommand, &i4Argc, apcPortArgv);
	DBGLOG(REQ, WARN, "argc is %i\n", i4Argc);

	/* example: set_wow_udp 0 5353,8080 (set) */
	/* example: set_wow_udp 1 (clear) */

	if (i4Argc >= 3) {

		/* Pick Max */
		ucCount = ((i4Argc - 2) > MAX_TCP_UDP_PORT) ? MAX_TCP_UDP_PORT :
			  (i4Argc - 2);
		DBGLOG(PF, INFO, "UDP ucCount=%d\n", ucCount);

		u4Ret = kalkStrtou8(apcPortArgv[1], 0, &ucVer);
		if (u4Ret) {
			DBGLOG(REQ, LOUD, "parse ucWakeupHif error u4Ret=%d\n",
			       u4Ret);
			return -1;
		}

		/* IPv4/IPv6 */
		DBGLOG(PF, INFO, "ucVer=%d\n", ucVer);
		if (ucVer == 0) {
			pWOW_CTRL->stWowPort.ucIPv4UdpPortCnt = ucCount;
			pausPortArry = pWOW_CTRL->stWowPort.ausIPv4UdpPort;
		} else {
			pWOW_CTRL->stWowPort.ucIPv6UdpPortCnt = ucCount;
			pausPortArry = pWOW_CTRL->stWowPort.ausIPv6UdpPort;
		}

		/* Port */
		for (ii = 0; ii < ucCount; ii++) {
			u4Ret = kalkStrtou16(apcPortArgv[ii+2], 0, &u2Port);
			if (u4Ret) {
				DBGLOG(PF, ERROR,
				       "parse u2Port error u4Ret=%d\n", u4Ret);
				return -1;
			}

			pausPortArry[ii] = u2Port;
			DBGLOG(PF, INFO, "ucPort=%d, idx=%d\n", u2Port, ii);
		}

		return 0;
	} else if (i4Argc == 2) {

		u4Ret = kalkStrtou8(apcPortArgv[1], 0, &ucVer);
		if (u4Ret) {
			DBGLOG(REQ, LOUD, "parse ucWakeupHif error u4Ret=%d\n",
			       u4Ret);
			return -1;
		}

		if (ucVer == 0) {
			kalMemZero(prGlueInfo->prAdapter->rWowCtrl.stWowPort
				.ausIPv4UdpPort,
				sizeof(uint16_t) * MAX_TCP_UDP_PORT);
			prGlueInfo->prAdapter->rWowCtrl.stWowPort
				.ucIPv4UdpPortCnt = 0;
		} else {
			kalMemZero(prGlueInfo->prAdapter->rWowCtrl.stWowPort
				.ausIPv6UdpPort,
				sizeof(uint16_t) * MAX_TCP_UDP_PORT);
			prGlueInfo->prAdapter->rWowCtrl.stWowPort
				.ucIPv6UdpPortCnt = 0;
		}

		return 0;
	} else
		return -1;

}

int priv_driver_set_wow_tcpport(struct net_device *prNetDev,
				       char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct WOW_CTRL *pWOW_CTRL = NULL;
	int32_t i4Argc = 0;
	int8_t *apcPortArgv[WLAN_CFG_ARGV_MAX_LONG] = { 0 }; /* to input 20 port
							      */
	int32_t u4Ret = 0, ii;
	uint8_t	ucVer = 0, ucCount;
	uint16_t u2Port = 0;
	uint16_t *pausPortArry;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	pWOW_CTRL = &prGlueInfo->prAdapter->rWowCtrl;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgumentLong(pcCommand, &i4Argc, apcPortArgv);
	DBGLOG(REQ, WARN, "argc is %i\n", i4Argc);

	/* example: set_wow_tcp 0 5353,8080 (Set) */
	/* example: set_wow_tcp 1 (clear) */

	if (i4Argc >= 3) {

		/* Pick Max */
		ucCount = ((i4Argc - 2) > MAX_TCP_UDP_PORT) ? MAX_TCP_UDP_PORT :
			  (i4Argc - 2);
		DBGLOG(PF, INFO, "TCP ucCount=%d\n", ucCount);

		u4Ret = kalkStrtou8(apcPortArgv[1], 0, &ucVer);
		if (u4Ret) {
			DBGLOG(REQ, LOUD, "parse ucWakeupHif error u4Ret=%d\n",
			       u4Ret);
			return -1;
		}

		/* IPv4/IPv6 */
		DBGLOG(PF, INFO, "Ver=%d\n", ucVer);
		if (ucVer == 0) {
			pWOW_CTRL->stWowPort.ucIPv4TcpPortCnt = ucCount;
			pausPortArry = pWOW_CTRL->stWowPort.ausIPv4TcpPort;
		} else {
			pWOW_CTRL->stWowPort.ucIPv6TcpPortCnt = ucCount;
			pausPortArry = pWOW_CTRL->stWowPort.ausIPv6TcpPort;
		}

		/* Port */
		for (ii = 0; ii < ucCount; ii++) {
			u4Ret = kalkStrtou16(apcPortArgv[ii+2], 0, &u2Port);
			if (u4Ret) {
				DBGLOG(PF, ERROR,
				       "parse u2Port error u4Ret=%d\n", u4Ret);
				return -1;
			}

			pausPortArry[ii] = u2Port;
			DBGLOG(PF, INFO, "ucPort=%d, idx=%d\n", u2Port, ii);
		}

		return 0;
	} else if (i4Argc == 2) {

		u4Ret = kalkStrtou8(apcPortArgv[1], 0, &ucVer);
		if (u4Ret) {
			DBGLOG(REQ, LOUD, "parse ucWakeupHif error u4Ret=%d\n",
			       u4Ret);
			return -1;
		}

		if (ucVer == 0) {
			kalMemZero(
				prGlueInfo->prAdapter->rWowCtrl.stWowPort
				.ausIPv4TcpPort,
				sizeof(uint16_t) * MAX_TCP_UDP_PORT);
			prGlueInfo->prAdapter->rWowCtrl.stWowPort
				.ucIPv4TcpPortCnt = 0;
		} else {
			kalMemZero(
				prGlueInfo->prAdapter->rWowCtrl.stWowPort
				.ausIPv6TcpPort,
				sizeof(uint16_t) * MAX_TCP_UDP_PORT);
			prGlueInfo->prAdapter->rWowCtrl.stWowPort
				.ucIPv6TcpPortCnt = 0;
		}

		return 0;
	} else
		return -1;

}

int priv_driver_get_wow_port(struct net_device *prNetDev,
				    char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct WOW_CTRL *pWOW_CTRL = NULL;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	int32_t u4Ret = 0, ii;
	uint8_t	ucVer = 0, ucProto = 0;
	uint16_t ucCount;
	uint16_t *pausPortArry;
	int8_t *aucIp[2] = {"IPv4", "IPv6"};
	int8_t *aucProto[2] = {"UDP", "TCP"};

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	pWOW_CTRL = &prGlueInfo->prAdapter->rWowCtrl;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	/* example: get_wow_port 0 0 (ipv4-udp) */
	/* example: get_wow_port 0 1 (ipv4-tcp) */
	/* example: get_wow_port 1 0 (ipv6-udp) */
	/* example: get_wow_port 1 1 (ipv6-tcp) */

	if (i4Argc >= 3) {

		/* 0=IPv4, 1=IPv6 */
		u4Ret = kalkStrtou8(apcArgv[1], 0, &ucVer);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse argc[1] error u4Ret=%d\n",
			       u4Ret);

		/* 0=UDP, 1=TCP */
		u4Ret = kalkStrtou8(apcArgv[2], 0, &ucProto);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse argc[2] error u4Ret=%d\n",
			       u4Ret);

		if (ucVer > 1)
			ucVer = 0;

		if (ucProto > 1)
			ucProto = 0;

		if (ucVer == 0) {
			if (ucProto == 0) {
				/* IPv4/UDP */
				ucCount = pWOW_CTRL->stWowPort.ucIPv4UdpPortCnt;
				pausPortArry =
					pWOW_CTRL->stWowPort.ausIPv4UdpPort;
			} else {
				/* IPv4/TCP */
				ucCount = pWOW_CTRL->stWowPort.ucIPv4TcpPortCnt;
				pausPortArry =
					pWOW_CTRL->stWowPort.ausIPv4TcpPort;
			}
		} else {
			if (ucProto == 0) {
				/* IPv6/UDP */
				ucCount = pWOW_CTRL->stWowPort.ucIPv6UdpPortCnt;
				pausPortArry =
					pWOW_CTRL->stWowPort.ausIPv6UdpPort;
			} else {
				/* IPv6/TCP */
				ucCount = pWOW_CTRL->stWowPort.ucIPv6TcpPortCnt;
				pausPortArry =
					pWOW_CTRL->stWowPort.ausIPv6TcpPort;
			}
		}

		/* Dunp Port */
		for (ii = 0; ii < ucCount; ii++)
			DBGLOG(PF, INFO, "ucPort=%d, idx=%d\n",
			       pausPortArry[ii], ii);


		DBGLOG(PF, INFO, "[%s/%s] count:%d\n", aucIp[ucVer],
		       aucProto[ucProto], ucCount);

		return 0;
	} else
		return -1;

}

int priv_driver_get_wow_reason(struct net_device *prNetDev,
				   char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4Argc = 0;
	int32_t i4BytesWritten = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	struct WOW_CTRL *pWOW_CTRL = NULL;

	if (prNetDev == NULL)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	pWOW_CTRL = &prGlueInfo->prAdapter->rWowCtrl;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (pWOW_CTRL->ucReason != INVALID_WOW_WAKE_UP_REASON)
		LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
			"\nwakeup_reason:%d", pWOW_CTRL->ucReason);

	return i4BytesWritten;
}


#if (CFG_SUPPORT_MDNS_OFFLOAD && CFG_SUPPORT_MDNS_OFFLOAD_TV)
int priv_support_mdns_offload(struct net_device *prNetDev,
		 struct ifreq *prReq,  int i4Cmd)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct MDNS_INFO_UPLAYER_T *prMdnsUplayerInfo = NULL;
	int ret = 0;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	if (!prReq->ifr_data) {
		DBGLOG(REQ, ERROR, "%s: prReq->ifr_data is NULL.\n", __func__);
		return -EINVAL;
	}

	prMdnsUplayerInfo = kzalloc(sizeof(struct MDNS_INFO_UPLAYER_T),
				GFP_KERNEL);
	if (!prMdnsUplayerInfo) {
		DBGLOG(REQ, WARN, "%s, alloc mem failed\n", __func__);
		return -ENOMEM;
	}

	if (copy_from_user(prMdnsUplayerInfo,
		prReq->ifr_data, sizeof(struct MDNS_INFO_UPLAYER_T))) {
		DBGLOG(REQ, ERROR, "%s: copy_from_user fail\n", __func__);
		ret = -EFAULT;
		goto exit;
	}

	prMdnsUplayerInfo->name[MDNS_QUESTION_NAME_MAX_LEN - 1] = '\0';

	ret = kalMdnsProcess(prGlueInfo, prMdnsUplayerInfo);

	if (ret == WLAN_STATUS_SUCCESS)
		ret = 0;
	else if (ret == WLAN_STATUS_FAILURE)
		ret = -1;
exit:
	kfree(prMdnsUplayerInfo);
	return ret;
}
#endif


#if CFG_SUPPORT_MDNS_OFFLOAD

#if TEST_CODE_FOR_MDNS
/* test code for mdns offload */

/* _googlecast.tcp.local */
uint8_t ptr_name[100] = {
			0x0b, 0x5f, 0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65,
			0x63, 0x61, 0x73, 0x74, 0x04, 0x5f, 0x74, 0x63,
			0x70, 0x05, 0x6c, 0x6f, 0x63, 0x61, 0x6c};

/* _googlezone._tcp.local */
uint8_t ptr_name2[100] = {
			0x0b, 0x5f, 0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65,
			0x7a, 0x6f, 0x6e, 0x65, 0x04, 0x5f, 0x74, 0x63,
			0x70, 0x05, 0x6c, 0x6f, 0x63, 0x61, 0x6c};

uint8_t response[500] = {
		0x00, 0x00, 0x84, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
		0x00, 0x02, 0x0b, 0x5f, 0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65,
		0x63, 0x61, 0x73, 0x74, 0x04, 0x5f, 0x74, 0x63, 0x70, 0x05,
		0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x00, 0x00, 0x0c, 0x00, 0x01,
		0x00, 0x00, 0x00, 0x78, 0x00, 0x18, 0x15, 0x69, 0x73, 0x74,
		0x6f, 0x2d, 0x38, 0x61, 0x61, 0x30, 0x31, 0x38, 0x66, 0x63,
		0x34, 0x30, 0x37, 0x63, 0x64, 0x66, 0x65, 0x64, 0xc0, 0x0c,
		0xc0, 0x2e, 0x00, 0x21, 0x80, 0x01, 0x00, 0x00, 0x00, 0x78,
		0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x49, 0x10, 0x38,
		0x61, 0x61, 0x30, 0x31, 0x38, 0x66, 0x63, 0x34, 0x30, 0x37,
		0x63, 0x64, 0x66, 0x65, 0x64, 0x05, 0x6c, 0x6f, 0x63, 0x61,
		0x6c, 0xc0, 0x1d, 0xc0, 0x2e, 0x00, 0x21, 0x80, 0x01, 0x00,
		0x00, 0x00, 0x78, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x1f,
		0x49, 0xc0, 0x58};

uint8_t response2[500] = {
		0x00, 0x00, 0x84, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
		0x00, 0x03, 0x0b, 0x5f, 0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65,
		0x7a, 0x6f, 0x6e, 0x65, 0x04, 0x5f, 0x74, 0x63, 0x70, 0x05,
		0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x00, 0x00, 0x0c, 0x00, 0x01,
		0x00, 0x00, 0x00, 0x78, 0x00, 0x27, 0x24, 0x61, 0x31, 0x33,
		0x39, 0x36, 0x63, 0x32, 0x66, 0x2d, 0x38, 0x66, 0x30, 0x32,
		0x2d, 0x39, 0x35, 0x33, 0x65, 0x2d, 0x30, 0x31, 0x61, 0x64,
		0x2d, 0x30, 0x61, 0x64, 0x31, 0x33, 0x31, 0x30, 0x64, 0x65,
		0x63, 0x65, 0x32, 0xc0, 0x0c, 0xc0, 0x2e, 0x00, 0x10, 0x80,
		0x01, 0x00, 0x00, 0x11, 0x94, 0x00, 0x38, 0x23, 0x69, 0x64,
		0x3d, 0x35, 0x31, 0x31, 0x32, 0x32, 0x43, 0x34, 0x38, 0x41,
		0x39, 0x38, 0x33, 0x30, 0x33, 0x30, 0x38, 0x30, 0x46, 0x32,
		0x46, 0x44, 0x32, 0x44, 0x43, 0x30, 0x36, 0x34, 0x36, 0x35,
		0x35, 0x37, 0x46, 0x13, 0x5f, 0x5f, 0x63, 0x6f, 0x6d, 0x6d,
		0x6f, 0x6e, 0x5f, 0x74, 0x69, 0x6d, 0x65, 0x5f, 0x5f, 0x3d,
		0x30, 0x7c, 0x30, 0xc0, 0x2e, 0x00, 0x21, 0x80, 0x01, 0x00,
		0x00, 0x00, 0x78, 0x00, 0x2d, 0x00, 0x00, 0x00, 0x00, 0x27,
		0x11, 0x24, 0x61, 0x31, 0x33, 0x39, 0x36, 0x63, 0x32, 0x66,
		0x2d, 0x38, 0x66, 0x30, 0x32, 0x2d, 0x39, 0x35, 0x33, 0x65,
		0x2d, 0x30, 0x31, 0x61, 0x64, 0x2d, 0x30, 0x61, 0x64, 0x31,
		0x33, 0x31, 0x30, 0x64, 0x65, 0x63, 0x65, 0x32, 0xc0, 0x1d,
		0xc0, 0xab, 0x00, 0x01, 0x80, 0x01, 0x00, 0x00, 0x00, 0x78,
		0x00, 0x04, 0xc0, 0xab, 0x1f, 0x44};

uint8_t response3[500] = {
		0x03, 0x00, 0x84, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
		0x00, 0x03, 0x0b, 0x5f, 0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65,
		0x7a, 0x6f, 0x6e, 0x65, 0x04, 0x5f, 0x74, 0x63, 0x70, 0x05,
		0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x00, 0x00, 0x0c, 0x00, 0x01,
		0x00, 0x00, 0x00, 0x78, 0x00, 0x27, 0x24, 0x61, 0x31, 0x33,
		0x39, 0x36, 0x63, 0x32, 0x66, 0x2d, 0x38, 0x66, 0x30, 0x32,
		0x2d, 0x39, 0x35, 0x33, 0x65, 0x2d, 0x30, 0x31, 0x61, 0x64,
		0x2d, 0x30, 0x61, 0x64, 0x31, 0x33, 0x31, 0x30, 0x64, 0x65,
		0x63, 0x65, 0x32, 0xc0, 0x0c, 0xc0, 0x2e, 0x00, 0x10, 0x80,
		0x01, 0x00, 0x00, 0x11, 0x94, 0x00, 0x38, 0x23, 0x69, 0x64,
		0x3d, 0x35, 0x31, 0x31, 0x32, 0x32, 0x43, 0x34, 0x38, 0x41,
		0x39, 0x38, 0x33, 0x30, 0x33, 0x30, 0x38, 0x30, 0x46, 0x32,
		0x46, 0x44, 0x32, 0x44, 0x43, 0x30, 0x36, 0x34, 0x36, 0x35,
		0x35, 0x37, 0x46, 0x13, 0x5f, 0x5f, 0x63, 0x6f, 0x6d, 0x6d,
		0x6f, 0x6e, 0x5f, 0x74, 0x69, 0x6d, 0x65, 0x5f, 0x5f, 0x3d,
		0x30, 0x7c, 0x30, 0xc0, 0x2e, 0x00, 0x21, 0x80, 0x01, 0x00,
		0x00, 0x00, 0x78, 0x00, 0x2d, 0x00, 0x00, 0x00, 0x00, 0x27,
		0x11, 0x24, 0x61, 0x31, 0x33, 0x39, 0x36, 0x63, 0x32, 0x66,
		0x2d, 0x38, 0x66, 0x30, 0x32, 0x2d, 0x39, 0x35, 0x33, 0x65,
		0x2d, 0x30, 0x31, 0x61, 0x64, 0x2d, 0x30, 0x61, 0x64, 0x31,
		0x33, 0x31, 0x30, 0x64, 0x65, 0x63, 0x65, 0x32, 0xc0, 0x1d,
		0xc0, 0xab, 0x00, 0x01, 0x80, 0x01, 0x00, 0x00, 0x00, 0x78,
		0x00, 0x04, 0xc0, 0xab, 0x1f, 0x03};

uint8_t response4[500] = {
		0x04, 0x00, 0x84, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
		0x00, 0x03, 0x0b, 0x5f, 0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65,
		0x7a, 0x6f, 0x6e, 0x65, 0x04, 0x5f, 0x74, 0x63, 0x70, 0x05,
		0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x00, 0x00, 0x0c, 0x00, 0x01,
		0x00, 0x00, 0x00, 0x78, 0x00, 0x27, 0x24, 0x61, 0x31, 0x33,
		0x39, 0x36, 0x63, 0x32, 0x66, 0x2d, 0x38, 0x66, 0x30, 0x32,
		0x2d, 0x39, 0x35, 0x33, 0x65, 0x2d, 0x30, 0x31, 0x61, 0x64,
		0x2d, 0x30, 0x61, 0x64, 0x31, 0x33, 0x31, 0x30, 0x64, 0x65,
		0x63, 0x65, 0x32, 0xc0, 0x0c, 0xc0, 0x2e, 0x00, 0x10, 0x80,
		0x01, 0x00, 0x00, 0x11, 0x94, 0x00, 0x38, 0x23, 0x69, 0x64,
		0x3d, 0x35, 0x31, 0x31, 0x32, 0x32, 0x43, 0x34, 0x38, 0x41,
		0x39, 0x38, 0x33, 0x30, 0x33, 0x30, 0x38, 0x30, 0x46, 0x32,
		0x46, 0x44, 0x32, 0x44, 0x43, 0x30, 0x36, 0x34, 0x36, 0x35,
		0x35, 0x37, 0x46, 0x13, 0x5f, 0x5f, 0x63, 0x6f, 0x6d, 0x6d,
		0x6f, 0x6e, 0x5f, 0x74, 0x69, 0x6d, 0x65, 0x5f, 0x5f, 0x3d,
		0x30, 0x7c, 0x30, 0xc0, 0x2e, 0x00, 0x21, 0x80, 0x01, 0x00,
		0x00, 0x00, 0x78, 0x00, 0x2d, 0x00, 0x00, 0x00, 0x00, 0x27,
		0x11, 0x24, 0x61, 0x31, 0x33, 0x39, 0x36, 0x63, 0x32, 0x66,
		0x2d, 0x38, 0x66, 0x30, 0x32, 0x2d, 0x39, 0x35, 0x33, 0x65,
		0x2d, 0x30, 0x31, 0x61, 0x64, 0x2d, 0x30, 0x61, 0x64, 0x31,
		0x33, 0x31, 0x30, 0x64, 0x65, 0x63, 0x65, 0x32, 0xc0, 0x1d,
		0xc0, 0xab, 0x00, 0x01, 0x80, 0x01, 0x00, 0x00, 0x00, 0x78,
		0x00, 0x04, 0xc0, 0xab, 0x1f, 0x04};

uint8_t test_passthrough[5][100] = {
		"_googlecast.tcp.local  1",
		"_googlecast.tcp.local  2",
		"_googlecast.tcp.local  3",
		"_googlezone._tcp.local 4",
		"_googlezone._tcp.local 5"
		};

int priv_driver_test_add_mdns_record(struct net_device *prNetDev,
		 char *pcCommand,  int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct MDNS_INFO_UPLAYER_T *prMdnsUplayerInfo = NULL;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	uint32_t u4Ret = 0;
	uint8_t ucIndex = 0;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	prMdnsUplayerInfo =
		kalMemAlloc(sizeof(struct MDNS_INFO_UPLAYER_T), PHY_MEM_TYPE);
	if (!prMdnsUplayerInfo) {
		DBGLOG(REQ, WARN, "%s, alloc mem failed\n", __func__);
		return -ENOMEM;
	}

	/* add record 2 */
	kalMemZero(prMdnsUplayerInfo, sizeof(struct MDNS_INFO_UPLAYER_T));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	u4Ret = kalkStrtou8(apcArgv[1], 0, &ucIndex);

	/*ucIndex == 0, ptr_name is _googlecast.tcp.local  */
	if (ucIndex > 0)
		ptr_name[22] = ucIndex + '0';

	/* add record 1 */
	prMdnsUplayerInfo->ucCmd = MDNS_CMD_ADD_RECORD;
	prMdnsUplayerInfo->mdns_param.query[0].type = 12;
	prMdnsUplayerInfo->mdns_param.query[0].class = 1;
	prMdnsUplayerInfo->mdns_param.query[0].name_length = 23;
	kalMemCopy(prMdnsUplayerInfo->mdns_param.query[0].name,
		ptr_name, sizeof(ptr_name));
	prMdnsUplayerInfo->mdns_param.response_len = 133;
	kalMemCopy(prMdnsUplayerInfo->mdns_param.response,
		response, 133);
	kalMdnsProcess(prGlueInfo, prMdnsUplayerInfo);

	kalMemFree(prMdnsUplayerInfo, PHY_MEM_TYPE,
		sizeof(struct MDNS_INFO_UPLAYER_T));

	return 0;
}

int priv_driver_add_mdns_record(struct net_device *prNetDev,
		char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct MDNS_INFO_UPLAYER_T *prMdnsUplayerInfo = NULL;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	prMdnsUplayerInfo =
		kalMemAlloc(sizeof(struct MDNS_INFO_UPLAYER_T), PHY_MEM_TYPE);
	if (!prMdnsUplayerInfo) {
		DBGLOG(REQ, WARN, "%s, alloc mem failed\n", __func__);
		return -ENOMEM;
	}

	/* add record 2 */
	kalMemZero(prMdnsUplayerInfo, sizeof(struct MDNS_INFO_UPLAYER_T));

	prMdnsUplayerInfo->ucCmd = MDNS_CMD_ADD_RECORD;
	prMdnsUplayerInfo->mdns_param.query[0].type = 12;
	prMdnsUplayerInfo->mdns_param.query[0].class = 1;
	prMdnsUplayerInfo->mdns_param.query[0].name_length = 23;
	kalMemCopy(prMdnsUplayerInfo->mdns_param.query[0].name,
		ptr_name2, sizeof(ptr_name2));
	prMdnsUplayerInfo->mdns_param.response_len = 226;
	kalMemCopy(prMdnsUplayerInfo->mdns_param.response,
		response2, 226);
	kalMdnsProcess(prGlueInfo, prMdnsUplayerInfo);

	kalMemZero(prMdnsUplayerInfo, sizeof(struct MDNS_INFO_UPLAYER_T));

	/* add record 1 */
	prMdnsUplayerInfo->ucCmd = MDNS_CMD_ADD_RECORD;
	prMdnsUplayerInfo->mdns_param.query[0].type = 12;
	prMdnsUplayerInfo->mdns_param.query[0].class = 1;
	prMdnsUplayerInfo->mdns_param.query[0].name_length = 23;
	kalMemCopy(prMdnsUplayerInfo->mdns_param.query[0].name,
		ptr_name, sizeof(ptr_name));
	prMdnsUplayerInfo->mdns_param.response_len = 133;
	kalMemCopy(prMdnsUplayerInfo->mdns_param.response,
		response, 133);
	kalMdnsProcess(prGlueInfo, prMdnsUplayerInfo);

	kalMemZero(prMdnsUplayerInfo, sizeof(struct MDNS_INFO_UPLAYER_T));

	/* add record 3 */
	prMdnsUplayerInfo->ucCmd = MDNS_CMD_ADD_RECORD;
	prMdnsUplayerInfo->mdns_param.query[0].type = 12;
	prMdnsUplayerInfo->mdns_param.query[0].class = 1;
	prMdnsUplayerInfo->mdns_param.query[0].name_length = 23;
	kalMemCopy(prMdnsUplayerInfo->mdns_param.query[0].name,
		ptr_name2, sizeof(ptr_name2));
	prMdnsUplayerInfo->mdns_param.response_len = 226;
	kalMemCopy(prMdnsUplayerInfo->mdns_param.response,
		response3, 226);
	kalMdnsProcess(prGlueInfo, prMdnsUplayerInfo);

	kalMemZero(prMdnsUplayerInfo, sizeof(struct MDNS_INFO_UPLAYER_T));

	/* add record 4 */
	prMdnsUplayerInfo->ucCmd = MDNS_CMD_ADD_RECORD;
	prMdnsUplayerInfo->mdns_param.query[0].type = 12;
	prMdnsUplayerInfo->mdns_param.query[0].class = 1;
	prMdnsUplayerInfo->mdns_param.query[0].name_length = 23;
	kalMemCopy(prMdnsUplayerInfo->mdns_param.query[0].name,
		ptr_name, sizeof(ptr_name));
	prMdnsUplayerInfo->mdns_param.response_len = 226;
	kalMemCopy(prMdnsUplayerInfo->mdns_param.response,
		response4, 226);
	kalMdnsProcess(prGlueInfo, prMdnsUplayerInfo);

	kalMemFree(prMdnsUplayerInfo, PHY_MEM_TYPE,
		sizeof(struct MDNS_INFO_UPLAYER_T));

	return 0;
}

static int priv_driver_set_passthtough_forward_all(
	 struct net_device *prNetDev,
	 char *pcCommand,  int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct MDNS_INFO_UPLAYER_T *prMdnsUplayerInfo = NULL;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	prMdnsUplayerInfo =
		kalMemAlloc(sizeof(struct MDNS_INFO_UPLAYER_T), PHY_MEM_TYPE);
	if (!prMdnsUplayerInfo) {
		DBGLOG(REQ, WARN, "%s, alloc mem failed\n", __func__);
		return -ENOMEM;
	}

	/* reset to zero */
	kalMemZero(prMdnsUplayerInfo, sizeof(struct MDNS_INFO_UPLAYER_T));

	prMdnsUplayerInfo->ucCmd = MDNS_CMD_SET_PASSTHTOUGH;
	prMdnsUplayerInfo->passthroughBehavior = MDNS_PASSTHROUGH_FORWARD_ALL;
	kalMdnsProcess(prGlueInfo, prMdnsUplayerInfo);

	kalMemFree(prMdnsUplayerInfo, PHY_MEM_TYPE,
		sizeof(struct MDNS_INFO_UPLAYER_T));

	return 0;
}

static int priv_driver_set_passthtough_drop_all(struct net_device *prNetDev,
		 char *pcCommand,  int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct MDNS_INFO_UPLAYER_T *prMdnsUplayerInfo = NULL;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	prMdnsUplayerInfo =
		kalMemAlloc(sizeof(struct MDNS_INFO_UPLAYER_T), PHY_MEM_TYPE);
	if (!prMdnsUplayerInfo) {
		DBGLOG(REQ, WARN, "%s, alloc mem failed\n", __func__);
		return -ENOMEM;
	}

	/* reset to zero */
	kalMemZero(prMdnsUplayerInfo, sizeof(struct MDNS_INFO_UPLAYER_T));

	prMdnsUplayerInfo->ucCmd = MDNS_CMD_SET_PASSTHTOUGH;
	prMdnsUplayerInfo->passthroughBehavior = MDNS_PASSTHROUGH_DROP_ALL;
	kalMdnsProcess(prGlueInfo, prMdnsUplayerInfo);

	kalMemFree(prMdnsUplayerInfo, PHY_MEM_TYPE,
		sizeof(struct MDNS_INFO_UPLAYER_T));

	return 0;
}

static int priv_driver_set_passthtough_list(struct net_device *prNetDev,
		 char *pcCommand,  int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct MDNS_INFO_UPLAYER_T *prMdnsUplayerInfo = NULL;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	prMdnsUplayerInfo =
		kalMemAlloc(sizeof(struct MDNS_INFO_UPLAYER_T), PHY_MEM_TYPE);
	if (!prMdnsUplayerInfo) {
		DBGLOG(REQ, WARN, "%s, alloc mem failed\n", __func__);
		return -ENOMEM;
	}

	/* reset to zero */
	kalMemZero(prMdnsUplayerInfo, sizeof(struct MDNS_INFO_UPLAYER_T));

	prMdnsUplayerInfo->ucCmd = MDNS_CMD_SET_PASSTHTOUGH;
	prMdnsUplayerInfo->passthroughBehavior = MDNS_PASSTHROUGH_LIST;
	kalMdnsProcess(prGlueInfo, prMdnsUplayerInfo);

	kalMemFree(prMdnsUplayerInfo, PHY_MEM_TYPE,
		sizeof(struct MDNS_INFO_UPLAYER_T));

	return 0;
}

static int priv_driver_add_passthtough(struct net_device *prNetDev,
		 char *pcCommand,  int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct MDNS_INFO_UPLAYER_T *prMdnsUplayerInfo = NULL;
	int i;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	prMdnsUplayerInfo =
		kalMemAlloc(sizeof(struct MDNS_INFO_UPLAYER_T), PHY_MEM_TYPE);
	if (!prMdnsUplayerInfo) {
		DBGLOG(REQ, WARN, "%s, alloc mem failed\n", __func__);
		return -ENOMEM;
	}

	kalMemZero(prMdnsUplayerInfo, sizeof(struct MDNS_INFO_UPLAYER_T));

	prMdnsUplayerInfo->ucCmd = MDNS_CMD_ADD_PASSTHTOUGH;

	for (i = 0; i < 3; i++) {
		kalMemCopy(prMdnsUplayerInfo->name, test_passthrough[i],
			sizeof(test_passthrough[i]));
		prMdnsUplayerInfo->name[MDNS_QUESTION_NAME_MAX_LEN - 1] = 0;
		kalMdnsProcess(prGlueInfo, prMdnsUplayerInfo);
	}

	kalMemCopy(prMdnsUplayerInfo->name, ptr_name, sizeof(ptr_name));
	prMdnsUplayerInfo->name[MDNS_QUESTION_NAME_MAX_LEN - 1] = 0;
	kalMdnsProcess(prGlueInfo, prMdnsUplayerInfo);

	kalMemFree(prMdnsUplayerInfo, PHY_MEM_TYPE,
		sizeof(struct MDNS_INFO_UPLAYER_T));

	return 0;
}

static int priv_driver_del_passthtough(struct net_device *prNetDev,
		 char *pcCommand,  int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct MDNS_INFO_UPLAYER_T *prMdnsUplayerInfo = NULL;
	int i;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	prMdnsUplayerInfo =
		kalMemAlloc(sizeof(struct MDNS_INFO_UPLAYER_T), PHY_MEM_TYPE);
	if (!prMdnsUplayerInfo) {
		DBGLOG(REQ, WARN, "%s, alloc mem failed\n", __func__);
		return -ENOMEM;
	}

	kalMemZero(prMdnsUplayerInfo, sizeof(struct MDNS_INFO_UPLAYER_T));

	prMdnsUplayerInfo->ucCmd = MDNS_CMD_DEL_PASSTHTOUGH;

	for (i = 0; i < 3; i++) {
		kalMemCopy(prMdnsUplayerInfo->name, test_passthrough[i],
			sizeof(test_passthrough[i]));
		kalMdnsProcess(prGlueInfo, prMdnsUplayerInfo);
	}

	kalMemFree(prMdnsUplayerInfo, PHY_MEM_TYPE,
		sizeof(struct MDNS_INFO_UPLAYER_T));

	return 0;
}

int priv_driver_get_hitcounter(struct net_device *prNetDev,
		 char *pcCommand,  int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct MDNS_INFO_UPLAYER_T *prMdnsUplayerInfo = NULL;
	int hit = 0;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	prMdnsUplayerInfo =
		kalMemAlloc(sizeof(struct MDNS_INFO_UPLAYER_T), PHY_MEM_TYPE);
	if (!prMdnsUplayerInfo) {
		DBGLOG(REQ, WARN, "%s, alloc mem failed\n", __func__);
		return -ENOMEM;
	}

	/* reset to zero */
	kalMemZero(prMdnsUplayerInfo, sizeof(struct MDNS_INFO_UPLAYER_T));

	prMdnsUplayerInfo->ucCmd = MDNS_CMD_GET_HITCOUNTER;
	prMdnsUplayerInfo->recordKey = 255;
	hit = kalMdnsProcess(prGlueInfo, prMdnsUplayerInfo);

	DBGLOG(REQ, WARN, "%s, hit= %d\n", __func__, hit);

	kalMemFree(prMdnsUplayerInfo, PHY_MEM_TYPE,
		sizeof(struct MDNS_INFO_UPLAYER_T));

	return hit;
}

int priv_driver_get_misscounter(struct net_device *prNetDev,
		 char *pcCommand,  int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct MDNS_INFO_UPLAYER_T *prMdnsUplayerInfo = NULL;
	int miss = 0;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	prMdnsUplayerInfo =
		kalMemAlloc(sizeof(struct MDNS_INFO_UPLAYER_T), PHY_MEM_TYPE);
	if (!prMdnsUplayerInfo) {
		DBGLOG(REQ, WARN, "%s, alloc mem failed\n", __func__);
		return -ENOMEM;
	}

	/* reset to zero */
	kalMemZero(prMdnsUplayerInfo, sizeof(struct MDNS_INFO_UPLAYER_T));

	prMdnsUplayerInfo->ucCmd = MDNS_CMD_GET_MISSCOUNTER;
	miss = kalMdnsProcess(prGlueInfo, prMdnsUplayerInfo);

	DBGLOG(REQ, WARN, "%s, miss= %d\n", __func__, miss);

	kalMemFree(prMdnsUplayerInfo, PHY_MEM_TYPE,
		sizeof(struct MDNS_INFO_UPLAYER_T));

	return miss;
}

static int priv_driver_resetall_passthrough(struct net_device *prNetDev,
		 char *pcCommand,  int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct MDNS_INFO_UPLAYER_T *prMdnsUplayerInfo = NULL;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	prMdnsUplayerInfo =
		kalMemAlloc(sizeof(struct MDNS_INFO_UPLAYER_T), PHY_MEM_TYPE);
	if (!prMdnsUplayerInfo) {
		DBGLOG(REQ, WARN, "%s, alloc mem failed\n", __func__);
		return -ENOMEM;
	}

	/* reset to zero */
	kalMemZero(prMdnsUplayerInfo, sizeof(struct MDNS_INFO_UPLAYER_T));

	prMdnsUplayerInfo->ucCmd = MDNS_CMD_RESETALL;
	kalMdnsProcess(prGlueInfo, prMdnsUplayerInfo);

	kalMemFree(prMdnsUplayerInfo, PHY_MEM_TYPE,
		sizeof(struct MDNS_INFO_UPLAYER_T));

	return 0;
}

static int priv_driver_show_mdns_passthrough(struct net_device *prNetDev,
		 char *pcCommand,  int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	kalShowMdnsPassthrough(prGlueInfo);

	return 0;
}
#endif

int priv_driver_show_mdns_record(struct net_device *prNetDev,
		char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	kalShowMdnsRecord(prGlueInfo);

	return 0;
}

int priv_driver_enable_mdns_offload(struct net_device *prNetDev,
			char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct MDNS_INFO_UPLAYER_T *prMdnsUplayerInfo;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	prMdnsUplayerInfo =
		kalMemAlloc(sizeof(struct MDNS_INFO_UPLAYER_T), PHY_MEM_TYPE);
	if (!prMdnsUplayerInfo) {
		DBGLOG(REQ, WARN, "%s, alloc mem failed\n", __func__);
		return -ENOMEM;
	}

	kalMemZero(prMdnsUplayerInfo, sizeof(struct MDNS_INFO_UPLAYER_T));

	prMdnsUplayerInfo->ucCmd = MDNS_CMD_ENABLE;

	kalMdnsProcess(prGlueInfo, prMdnsUplayerInfo);

	kalMemFree(prMdnsUplayerInfo, PHY_MEM_TYPE,
		sizeof(struct MDNS_INFO_UPLAYER_T));

	return 0;
}

int priv_driver_disable_mdns_offload(struct net_device *prNetDev,
			char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct MDNS_INFO_UPLAYER_T *prMdnsUplayerInfo;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	prMdnsUplayerInfo =
		kalMemAlloc(sizeof(struct MDNS_INFO_UPLAYER_T), PHY_MEM_TYPE);
	if (!prMdnsUplayerInfo) {
		DBGLOG(REQ, WARN, "%s, alloc mem failed\n", __func__);
		return -ENOMEM;
	}

	kalMemZero(prMdnsUplayerInfo, sizeof(struct MDNS_INFO_UPLAYER_T));

	prMdnsUplayerInfo->ucCmd = MDNS_CMD_DISABLE;

	kalMdnsProcess(prGlueInfo, prMdnsUplayerInfo);

	kalMemFree(prMdnsUplayerInfo, PHY_MEM_TYPE,
		sizeof(struct MDNS_INFO_UPLAYER_T));

	return 0;

}

int priv_driver_set_mdns_wake_flag(struct net_device *prNetDev,
		char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	uint32_t u4Ret = 0;
	uint8_t ucWakeFlag = 0;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	u4Ret = kalkStrtou8(apcArgv[1], 0, &ucWakeFlag);

	if (u4Ret)
		DBGLOG(REQ, LOUD, "parse ucWakeFlag error u4Ret=%u\n", u4Ret);

	prGlueInfo->prAdapter->mdns_wake_flag = ucWakeFlag;

	DBGLOG(REQ, STATE, "set mdns wake flag %u\n", ucWakeFlag);

	return 0;
}

static int priv_driver_enable_mdns_ipv6_wakeup(struct net_device *prNetDev,
		 char *pcCommand,  int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct MDNS_INFO_UPLAYER_T *prMdnsUplayerInfo;
	uint32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	uint32_t u4Ret = 0;
	uint8_t ucWakeFlag = 0;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	prMdnsUplayerInfo =
		kalMemAlloc(sizeof(struct MDNS_INFO_UPLAYER_T), PHY_MEM_TYPE);
	if (!prMdnsUplayerInfo) {
		DBGLOG(REQ, WARN, "%s, alloc mem failed\n", __func__);
		return -ENOMEM;
	}

	kalMemZero(prMdnsUplayerInfo, sizeof(struct MDNS_INFO_UPLAYER_T));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	u4Ret = kalkStrtou8(apcArgv[1], 0, &ucWakeFlag);

	if (u4Ret)
		DBGLOG(REQ, LOUD, "ipv6 ucWakeFlag error u4Ret=%u\n", u4Ret);

	prMdnsUplayerInfo->ucCmd = MDNS_CMD_SET_IPV6_WAKEUP_FLAG;
	prMdnsUplayerInfo->ucIPV6WakeupFlag = ucWakeFlag;

	kalMdnsProcess(prGlueInfo, prMdnsUplayerInfo);

	kalMemFree(prMdnsUplayerInfo, PHY_MEM_TYPE,
		sizeof(struct MDNS_INFO_UPLAYER_T));

	return 0;

}

#endif /* #if CFG_SUPPORT_MDNS_OFFLOAD */
#endif /* #if CFG_WOW_SUPPORT */

int priv_driver_set_adv_pws(struct net_device *prNetDev,
				   char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	uint32_t u4Ret = 0;
	uint8_t ucAdvPws = 0;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc >= 2) {

		u4Ret = kalkStrtou8(apcArgv[1], 0, &ucAdvPws);

		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse bEnable error u4Ret=%d\n",
			       u4Ret);

		prGlueInfo->prAdapter->rWifiVar.ucAdvPws = ucAdvPws;

		DBGLOG(INIT, INFO, "AdvPws:%d\n",
		       prGlueInfo->prAdapter->rWifiVar.ucAdvPws);

		return 0;
	} else
		return -1;
}

int priv_driver_set_mdtim(struct net_device *prNetDev,
				 char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	uint32_t u4Ret = 0;
	uint8_t ucMultiDtim = 0, ucVer = 0;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	/* iwpriv wlan0 driver "set_mdtim 1 3 */
	if (i4Argc >= 3) {

		u4Ret = kalkStrtou8(apcArgv[1], 0, &ucVer);
		if (u4Ret) {
			DBGLOG(REQ, ERROR, "parse apcArgv1 error u4Ret=%d\n",
			       u4Ret);
			return -1;
		}

		u4Ret = kalkStrtou8(apcArgv[2], 0, &ucMultiDtim);
		if (u4Ret) {
			DBGLOG(REQ, ERROR, "parse apcArgv2 error u4Ret=%d\n",
			       u4Ret);
			return -1;
		}

		if (ucVer == 0) {
			prGlueInfo->prAdapter->rWifiVar.ucWowOnMdtim =
								ucMultiDtim;
			DBGLOG(REQ, INFO, "WOW On MDTIM:%d\n",
			       prGlueInfo->prAdapter->rWifiVar.ucWowOnMdtim);
		} else {
			prGlueInfo->prAdapter->rWifiVar.ucWowOffMdtim =
								ucMultiDtim;
			DBGLOG(REQ, INFO, "WOW Off MDTIM:%d\n",
			       prGlueInfo->prAdapter->rWifiVar.ucWowOffMdtim);
		}
	}

	return 0;

}

int priv_driver_set_disablepartial(struct net_device *prNetDev,
				 char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	u_int8_t fgDisable = 0;
	int32_t i4Ret = 0;

	if (!prNetDev) {
		DBGLOG(REQ, ERROR, "prNetDev is NULL\n");
		return -1;
	}

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc >= 2) {
		i4Ret = kalkStrtou8(apcArgv[1], 0, &fgDisable);
		if (i4Ret) {
			DBGLOG(REQ, LOUD, "parse fgDisable error i4Ret=%d\n",
			       i4Ret);
			return -1;
		}
		if (fgDisable == 0 || fgDisable == 1)
			prAdapter->rWifiVar.fgDisablePartialScan = fgDisable;
		else {
			DBGLOG(REQ, ERROR,
				"Invalid Value for DisablePartialScan[%u]\n",
				fgDisable);
			return -1;
		}
		DBGLOG(REQ, INFO, "Set DisablePartialScan mode [%u]\n",
			prAdapter->rWifiVar.fgDisablePartialScan);
	}

	return 0;
}

int priv_driver_set_suspend_mode(struct net_device *prNetDev,
				 char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	u_int8_t fgEnable;
	uint32_t u4Enable = 0;
	int32_t u4Ret = 0;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc >= 2) {
		/* fgEnable = (kalStrtoul(apcArgv[1], NULL, 0) == 1) ? TRUE :
		 *            FALSE;
		 */
		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Enable);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse u4Enable error u4Ret=%d\n",
			       u4Ret);
		if (u4Enable == 1)
			fgEnable = TRUE;
		else
			fgEnable = FALSE;

		if (prGlueInfo->fgIsInSuspendMode == fgEnable) {
			DBGLOG(REQ, INFO,
			       "%s: Already in suspend mode [%u], SKIP!\n",
			       __func__, fgEnable);
			return 0;
		}

		DBGLOG(REQ, INFO, "%s: Set suspend mode [%u]\n", __func__,
		       fgEnable);

		prGlueInfo->fgIsInSuspendMode = fgEnable;

		wlanSetSuspendMode(prGlueInfo, fgEnable);
#if CFG_ENABLE_WIFI_DIRECT
		p2pSetSuspendMode(prGlueInfo, fgEnable);
#endif
	}

	return 0;
}

int priv_driver_set_bf(struct net_device *prNetDev, char *pcCommand,
			 int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret, u4Parse = 0;
	uint8_t ucBfEnable;
	/*UINT_8 ucBssIndex;*/
	/*P_BSS_INFO_T prBssInfo;*/


	ASSERT(prNetDev);

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Parse);
	if (u4Ret)
		DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
		       u4Ret);

	ucBfEnable = (uint8_t) u4Parse;
	prGlueInfo->prAdapter->rWifiVar.ucStaHtBfee = ucBfEnable;
	prGlueInfo->prAdapter->rWifiVar.ucStaVhtBfee = ucBfEnable;
#if (CFG_SUPPORT_802_11AX == 1)
	prGlueInfo->prAdapter->rWifiVar.ucStaHeBfee = ucBfEnable;
#endif /* CFG_SUPPORT_802_11AX == 1 */
#if (CFG_SUPPORT_802_11BE == 1)
	prGlueInfo->prAdapter->rWifiVar.ucStaEhtBfee = ucBfEnable;
#endif /* CFG_SUPPORT_802_11BE == 1 */
	prGlueInfo->prAdapter->rWifiVar.ucStaVhtMuBfee = ucBfEnable;
	DBGLOG(REQ, ERROR, "ucBfEnable = %d\n", ucBfEnable);

	return i4BytesWritten;
}

int priv_driver_set_nss(struct net_device *prNetDev, char *pcCommand,
			 int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret, u4Parse = 0;
	uint8_t ucNSS;

	ASSERT(prNetDev);

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc == 2) {
		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Parse);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
			       u4Ret);

		ucNSS = (uint8_t) u4Parse;
		prGlueInfo->prAdapter->rWifiVar.ucNSS = ucNSS;
		DBGLOG(REQ, LOUD, "ucNSS = %d\n", ucNSS);
	} else {
		DBGLOG(INIT, ERROR, "iwpriv wlanXX driver SET_NSS <nss>\n");
	}

	return i4BytesWritten;
}


int priv_driver_set_amsdu_tx(struct net_device *prNetDev, char *pcCommand,
			 int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret, u4Parse = 0;
	uint8_t ucAmsduTx;

	ASSERT(prNetDev);

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc == 2) {
		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Parse);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
			       u4Ret);

		ucAmsduTx = (uint8_t) u4Parse;
		prGlueInfo->prAdapter->rWifiVar.ucAmsduInAmpduTx = ucAmsduTx;
		prGlueInfo->prAdapter->rWifiVar.ucHtAmsduInAmpduTx = ucAmsduTx;
		prGlueInfo->prAdapter->rWifiVar.ucVhtAmsduInAmpduTx = ucAmsduTx;
#if (CFG_SUPPORT_802_11AX == 1)
		prGlueInfo->prAdapter->rWifiVar.ucHeAmsduInAmpduTx = ucAmsduTx;
#endif
#if (CFG_SUPPORT_802_11BE == 1)
		prGlueInfo->prAdapter->rWifiVar.ucEhtAmsduInAmpduTx = ucAmsduTx;
#endif
		DBGLOG(REQ, LOUD, "ucAmsduTx = %d\n", ucAmsduTx);
	} else {
		DBGLOG(INIT, ERROR, "iwpriv wlanXX driver SET_NSS <nss>\n");
	}

	return i4BytesWritten;
}


int priv_driver_set_amsdu_rx(struct net_device *prNetDev, char *pcCommand,
			 int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret, u4Parse = 0;
	uint8_t ucAmsduRx;

	ASSERT(prNetDev);

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc == 2) {
		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Parse);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
			       u4Ret);

		ucAmsduRx = (uint8_t) u4Parse;
		prGlueInfo->prAdapter->rWifiVar.ucAmsduInAmpduRx = ucAmsduRx;
		prGlueInfo->prAdapter->rWifiVar.ucHtAmsduInAmpduRx = ucAmsduRx;
		prGlueInfo->prAdapter->rWifiVar.ucVhtAmsduInAmpduRx = ucAmsduRx;
#if (CFG_SUPPORT_802_11AX == 1)
		prGlueInfo->prAdapter->rWifiVar.ucHeAmsduInAmpduRx = ucAmsduRx;
#endif
#if (CFG_SUPPORT_802_11BE == 1)
		prGlueInfo->prAdapter->rWifiVar.ucEhtAmsduInAmpduRx = ucAmsduRx;
#endif
		DBGLOG(REQ, LOUD, "ucAmsduRx = %d\n", ucAmsduRx);
	} else {
		DBGLOG(INIT, ERROR, "iwpriv wlanXX driver SET_NSS <nss>\n");
	}

	return i4BytesWritten;
}

int priv_driver_set_ampdu_tx(struct net_device *prNetDev, char *pcCommand,
			 int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret, u4Parse = 0;
	uint8_t ucAmpduEnable;
	/*UINT_8 ucBssIndex;*/
	/*P_BSS_INFO_T prBssInfo;*/


	ASSERT(prNetDev);

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));



	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc == 2) {

		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Parse);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
			       u4Ret);

		ucAmpduEnable = (uint8_t) u4Parse;
		prGlueInfo->prAdapter->rWifiVar.ucAmpduTx = ucAmpduEnable;
		DBGLOG(REQ, ERROR, "ucAmpduTx = %d\n", ucAmpduEnable);
	} else {
		DBGLOG(INIT, ERROR, "iwpriv wlan0 driver SET_AMPDU_TX <en>\n");
		DBGLOG(INIT, ERROR, "<en> 1: enable. 0: disable.\n");
	}

	return i4BytesWritten;
}

int priv_driver_set_ampdu_rx(struct net_device *prNetDev, char *pcCommand,
			 int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret, u4Parse = 0;
	uint8_t ucAmpduEnable;
	/*UINT_8 ucBssIndex;*/
	/*P_BSS_INFO_T prBssInfo;*/


	ASSERT(prNetDev);

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc == 2) {
		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Parse);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
			       u4Ret);

		ucAmpduEnable = (uint8_t) u4Parse;
		prGlueInfo->prAdapter->rWifiVar.ucAmpduRx = ucAmpduEnable;
		DBGLOG(REQ, ERROR, "ucAmpduRx = %d\n",
			prGlueInfo->prAdapter->rWifiVar.ucAmpduRx);
	} else {
		DBGLOG(INIT, ERROR, "iwpriv wlan0 driver SET_AMPDU_RX <en>\n");
		DBGLOG(INIT, ERROR, "<en> 1: enable. 0: disable.\n");
	}

	return i4BytesWritten;
}

int priv_driver_set_tx_ampdu_num(struct net_device *prNetDev, char *pcCommand,
			 int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = -1;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret, u4Parse = 0;
	uint8_t ucBssIndex;
	struct PARAM_SET_TX_AGG_LIMIT_INFO rParam;
	uint32_t u4BufLen = 0;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4AmpduCap;

	if (!prNetDev || GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	ucBssIndex = wlanGetBssIdx(prNetDev);

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);


	if (i4Argc != 3) {
		DBGLOG(INIT, ERROR,
			"iwpriv wlan0 driver SET_TX_AMPDU_NUM <set> <num>\n");
		DBGLOG(INIT, ERROR,
			"<set> 1: set ampdu num. 0: reset to default value.\n");
		DBGLOG(INIT, ERROR, "<num> AMPDU num.\n");
		return -1;
	}

	u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Parse);
	if (u4Ret) {
		DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
				u4Ret);
		return -1;
	}

	rParam.ucSet = (uint8_t) u4Parse;
	u4Ret = kalkStrtou32(apcArgv[2], 0, &u4Parse);
	if (u4Ret) {
		DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
				u4Ret);
		return -1;
	}

	rParam.u2TxAmpduNum = (uint16_t) u4Parse;
#if CFG_SUPPORT_CONNAC3X
	u4AmpduCap = WLAN_EHT_MAX_BA_SIZE;
#elif CFG_SUPPORT_CONNAC2X
	u4AmpduCap = WLAN_HE_MAX_BA_SIZE;
#else
	u4AmpduCap = WLAN_LEGACY_MAX_BA_SIZE;
#endif
	if (rParam.u2TxAmpduNum > u4AmpduCap)
		return -EINVAL;
	else if (rParam.u2TxAmpduNum == u4AmpduCap)
		rParam.u2TxAmpduNum = 0;

	rStatus = kalIoctlByBssIdx(prGlueInfo, wlanoidSetTxAggLimit,
		&rParam, sizeof(rParam), &u4BufLen, ucBssIndex);
	if (rStatus == WLAN_STATUS_SUCCESS)
		i4BytesWritten = 0;

	return i4BytesWritten;
}

int priv_driver_set_tx_amsdu_num(struct net_device *prNetDev, char *pcCommand,
			 int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = -1;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret, u4Parse = 0;
	uint8_t ucBssIndex;
	struct PARAM_SET_TX_AMSDU_NUM_LIMIT_INFO rParam;
	uint32_t u4BufLen = 0;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	if (!prNetDev || GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	ucBssIndex = wlanGetBssIdx(prNetDev);

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);


	if (i4Argc != 3) {
		DBGLOG(INIT, ERROR,
			"iwpriv wlan0 driver SET_TX_AMSDU_NUM <set> <num>\n");
		DBGLOG(INIT, ERROR,
			"<set> 1: set amsdu num. 0: reset to default value.\n");
		DBGLOG(INIT, ERROR, "<num> AMSDU num.\n");
		return -1;
	}

	u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Parse);
	if (u4Ret) {
		DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
				u4Ret);
		return -1;
	}

	rParam.ucSet = (uint8_t) u4Parse;
	u4Ret = kalkStrtou32(apcArgv[2], 0, &u4Parse);
	if (u4Ret) {
		DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
				u4Ret);
		return -1;
	}

	rParam.ucTxAmsduNum = (uint8_t) u4Parse;

	rStatus = kalIoctlByBssIdx(prGlueInfo, wlanoidSetTxAmsduNumLimit,
		&rParam, sizeof(rParam), &u4BufLen, ucBssIndex);
	if (rStatus == WLAN_STATUS_SUCCESS)
		i4BytesWritten = 0;

	return i4BytesWritten;
}

int priv_driver_set_qos(struct net_device *prNetDev, char *pcCommand,
			 int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret, u4Parse = 0;
	uint8_t ucQoSEnable;

	ASSERT(prNetDev);

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc == 2) {
		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Parse);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
				u4Ret);

		ucQoSEnable = (uint8_t) u4Parse;
		prGlueInfo->prAdapter->rWifiVar.ucQoS = ucQoSEnable;
		DBGLOG(REQ, ERROR, "ucQoS = %d\n",
			prGlueInfo->prAdapter->rWifiVar.ucQoS);
	} else {
		DBGLOG(INIT, ERROR, "iwpriv wlanXX driver SET_QOS <enable>\n");
		DBGLOG(INIT, ERROR, "<enable> 1: enable. 0: disable.\n");
	}

	return i4BytesWritten;
}

#if (CFG_SUPPORT_802_11AX == 1)
int priv_driver_muedca_override(
	struct net_device *prNetDev,
	char *pcCommand,
	int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret, u4Parse = 0;
	uint8_t ucMuEdcaOverride;

	ASSERT(prNetDev);

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc == 2) {
		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Parse);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
			       u4Ret);

		ucMuEdcaOverride = (uint8_t) u4Parse;
#if (CFG_SUPPORT_802_11AX == 1)
		prGlueInfo->prAdapter->fgMuEdcaOverride = ucMuEdcaOverride;
#endif
		DBGLOG(REQ, LOUD, "ucMuEdcaOverride = %d\n", ucMuEdcaOverride);
	} else {
		DBGLOG(INIT, ERROR,
			"iwpriv wlanXX driver MUEDCA_OVERRIDE <val>\n");
	}

	return i4BytesWritten;
}

int priv_driver_set_ehtmcsmap(struct net_device *prNetDev,
				char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret, u4Parse = 0, u4ParseBW = 0;
	struct ADAPTER *prAdapter = NULL;

	ASSERT(prNetDev);

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc == 3) {
		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Parse);
		u4Ret = kalkStrtou32(apcArgv[2], 0, &u4ParseBW);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
			       u4Ret);
#if (CFG_SUPPORT_802_11BE == 1)
		/* Figure 9-778ex-EHT-MCS Map (20 MHz-Only STA)
		 * Figure 9-788ey-EHT-MCS Map (BW<=80MHz,Except 20MHz)
		 * Default value: 20MHz: 0x22222222
		 *                80MHz: 0x222222
		 */
		if (u4Parse == 0) {
			prAdapter->fgMcsMapBeenSet &=
				(~(SET_EHT_BW20_MCS_MAP |
					SET_EHT_BW80_MCS_MAP));
			DBGLOG(REQ, INFO,
				"fgMcsMapBeenSet: %d\n",
				prAdapter->fgMcsMapBeenSet);
		} else if (u4Parse == 1) {
			prAdapter->fgMcsMapBeenSet |= SET_EHT_BW20_MCS_MAP;
			prAdapter->u4EhtMcsMap20MHzSetFromSigma = u4ParseBW;
			DBGLOG(REQ, INFO, "Set EhtMcsMap BW20 = 0x%X\n",
				prAdapter->u4EhtMcsMap20MHzSetFromSigma);
		} else if (u4Parse == 2) {
			prAdapter->fgMcsMapBeenSet |= SET_EHT_BW80_MCS_MAP;
			prAdapter->u4EhtMcsMap80MHzSetFromSigma = u4ParseBW;
			DBGLOG(REQ, INFO, "Set EhtMcsMap BW80 = 0x%X\n",
				prAdapter->u4EhtMcsMap80MHzSetFromSigma);
		}
#endif
	} else {
		DBGLOG(REQ, ERROR,
			"iwpriv wlan0 driver SET_TX_EHTMCSMAP:<BW> <Value>\n");
		prAdapter->fgMcsMapBeenSet &=
			(~(SET_EHT_BW20_MCS_MAP | SET_EHT_BW80_MCS_MAP));
	}

	return i4BytesWritten;
}

int priv_driver_set_mcsmap(struct net_device *prNetDev, char *pcCommand,
			 int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret, u4Parse = 0;
	uint8_t ucTxMcsMap;
	struct ADAPTER *prAdapter = NULL;

	ASSERT(prNetDev);

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc == 2) {
		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Parse);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
			       u4Ret);

		ucTxMcsMap = (uint8_t) u4Parse;
#if (CFG_SUPPORT_802_11AX == 1)
		if (ucTxMcsMap == 255) {
			prGlueInfo->prAdapter->fgMcsMapBeenSet &=
				(~SET_HE_MCS_MAP);
		} else {
			prAdapter = prGlueInfo->prAdapter;
			prAdapter->ucMcsMapSetFromSigma = ucTxMcsMap;

			DBGLOG(REQ, ERROR, "ucMcsMapSetFromSigma = %d\n",
				prGlueInfo->prAdapter->ucMcsMapSetFromSigma);

			prGlueInfo->prAdapter->fgMcsMapBeenSet |=
				SET_HE_MCS_MAP;
		}
#endif
	} else {
		DBGLOG(INIT, ERROR, "iwpriv wlan0 driver SET_TX_MCSMAP <en>\n");
		DBGLOG(INIT, ERROR, "<en> 1: enable. 0: disable.\n");
	}

	return i4BytesWritten;
}

int priv_driver_set_ba_size(struct net_device *prNetDev, char *pcCommand,
			 int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret, u4Parse = 0;
	uint16_t u2BaSize;
	/*UINT_8 ucBssIndex;*/
	/*P_BSS_INFO_T prBssInfo;*/


	ASSERT(prNetDev);

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc == 2) {
		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Parse);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
			       u4Ret);

		u2BaSize = (uint16_t) u4Parse;

		prGlueInfo->prAdapter->rWifiVar.u2TxHeBaSize =
			u2BaSize > WLAN_HE_MAX_BA_SIZE ?
			WLAN_HE_MAX_BA_SIZE : u2BaSize;
		prGlueInfo->prAdapter->rWifiVar.u2RxHeBaSize =
			u2BaSize > WLAN_HE_MAX_BA_SIZE ?
			WLAN_HE_MAX_BA_SIZE : u2BaSize;
#if (CFG_SUPPORT_802_11BE == 1)
		prGlueInfo->prAdapter->rWifiVar.u2TxEhtBaSize =
			u2BaSize > WLAN_EHT_MAX_BA_SIZE ?
			WLAN_EHT_MAX_BA_SIZE : u2BaSize;
		prGlueInfo->prAdapter->rWifiVar.u2RxEhtBaSize =
			u2BaSize > WLAN_EHT_MAX_BA_SIZE ?
			WLAN_EHT_MAX_BA_SIZE : u2BaSize;
#endif
	} else {
		DBGLOG(INIT, ERROR, "iwpriv wlanXX driver SET_BA_SIZE\n");
		DBGLOG(INIT, ERROR, "<enable> 1: enable. 0: disable.\n");
	}

	return i4BytesWritten;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to process the SET_RX_BA_SIZE or SET_TX_BA_SIZE
 *
 * \param[in]  fgIsTx		when fgIsTx is 1, process SET_TX_BA_SIZE
 *
 * \retval WLAN_STATUS_SUCCESS
 */
/*----------------------------------------------------------------------------*/
int priv_driver_set_trx_ba_size(struct net_device *prNetDev,
		char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret, u4Parse = 0;
	uint16_t u2BaSize;
	int8_t i4Type = WLAN_TYPE_UNKNOWN;
	uint8_t fgIsTx = FALSE;

	ASSERT(prNetDev);

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	fgIsTx = (strnicmp(pcCommand, CMD_SET_RX_BA_SIZE,
			strlen(CMD_SET_RX_BA_SIZE)) == 0) ? FALSE : TRUE;

	if (i4Argc == 3) {
		if (strnicmp(apcArgv[1], "LEGACY", strlen("LEGACY")) == 0)
			i4Type = WLAN_TYPE_LEGACY;
		else if (strnicmp(apcArgv[1], "HE", strlen("HE")) == 0)
			i4Type = WLAN_TYPE_HE;
		else if (strnicmp(apcArgv[1], "EHT", strlen("EHT")) == 0)
			i4Type = WLAN_TYPE_EHT;

		if (i4Type != WLAN_TYPE_UNKNOWN) {
			u4Ret = kalkStrtou32(apcArgv[2], 0, &u4Parse);
			if (u4Ret)
				DBGLOG(REQ, LOUD,
				       "parse apcArgv error u4Ret=%d\n",
				       u4Ret);

			u2BaSize = (uint16_t) u4Parse;

			if (i4Type == WLAN_TYPE_LEGACY &&
			    u2BaSize <= WLAN_LEGACY_MAX_BA_SIZE ||
			    i4Type == WLAN_TYPE_HE &&
			    u2BaSize <= WLAN_HE_MAX_BA_SIZE ||
			    i4Type == WLAN_TYPE_EHT &&
			    u2BaSize <= WLAN_EHT_MAX_BA_SIZE) {
				/* only with valid ba size enter here */
				if (fgIsTx == 0)
					wlanSetRxBaSize(prGlueInfo,
						i4Type, u2BaSize);
				else
					wlanSetTxBaSize(prGlueInfo,
						i4Type, u2BaSize);

				return i4BytesWritten;
			}
		}
	}

	if (fgIsTx == FALSE)
		DBGLOG(INIT, ERROR,
			"iwpriv wlanXX driver SET_RX_BA_SIZE <type> <number>\n");
	else
		DBGLOG(INIT, ERROR,
			"iwpriv wlanXX driver SET_TX_BA_SIZE <type> <number>\n");

	DBGLOG(INIT, ERROR, "<type> LEGACY or HE\n");
	DBGLOG(INIT, ERROR,
		"<number> the number of ba size, max(Legacy):%d max(HE):%d\n",
		WLAN_LEGACY_MAX_BA_SIZE, WLAN_HE_MAX_BA_SIZE);

	return i4BytesWritten;
}

/* This command is for sigma to disable TpTestMode. */
int priv_driver_set_tp_test_mode(struct net_device *prNetDev,
				 char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret, u4Parse = 0;
	uint8_t ucTpTestMode;

	ASSERT(prNetDev);

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc == 2) {
		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Parse);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
			       u4Ret);

		ucTpTestMode = (uint8_t) u4Parse;

		prGlueInfo->prAdapter->rWifiVar.ucTpTestMode = ucTpTestMode;
	} else {
		DBGLOG(INIT, ERROR, "iwpriv wlanXX driver SET_TP_TEST_MODE\n");
		DBGLOG(INIT, ERROR, "<enable> 1: enable. 0: disable.\n");
	}

	return i4BytesWritten;
}

/* This command is for sigma to disable TxPPDU. */
int priv_driver_set_tx_ppdu(struct net_device *prNetDev, char *pcCommand,
			 int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret, u4Parse = 0;
	struct ADAPTER *prAdapter = NULL;
	struct STA_RECORD *prStaRec;
	uint32_t u4BufLen = 0;
	struct CMD_ACCESS_REG rCmdAccessReg;

	ASSERT(prNetDev);

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;
	prStaRec = cnmGetStaRecByIndex(prAdapter, 0);

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc == 2) {

		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Parse);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
			       u4Ret);
		rCmdAccessReg.u4Address =
			prAdapter->chip_info->arb_ac_mode_addr;

		if (u4Parse) {
			/* HE_SU is allowed. */
			prAdapter->fgTxPPDU = TRUE;
			rCmdAccessReg.u4Data = 0x0;
			u4Ret = kalIoctl(prGlueInfo, wlanoidSetMcrWrite,
					&rCmdAccessReg, sizeof(rCmdAccessReg),
					&u4BufLen);
			if (u4Ret != WLAN_STATUS_SUCCESS)
				return -1;
		} else {
			/* HE_SU is not allowed. */
			prAdapter->fgTxPPDU = FALSE;
			rCmdAccessReg.u4Data = 0xFFFF;
			if (prStaRec && prStaRec->fgIsTxAllowed) {
				u4Ret = kalIoctl(prGlueInfo, wlanoidSetMcrWrite,
					&rCmdAccessReg, sizeof(rCmdAccessReg),
					&u4BufLen);
				if (u4Ret != WLAN_STATUS_SUCCESS)
					return -1;
			}
		}

		DBGLOG(REQ, STATE, "fgTxPPDU is %d\n", prAdapter->fgTxPPDU);
	} else {
		DBGLOG(INIT, ERROR, "iwpriv wlanXX driver TX_PPDU\n");
		DBGLOG(INIT, ERROR, "<enable> 1: enable. 0: disable.\n");
	}

	return i4BytesWritten;
}

/* This command is for sigma to disable LDPC capability. */
int priv_driver_set_ldpc(struct net_device *prNetDev, char *pcCommand,
			 int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret, u4Parse = 0;
	struct ADAPTER *prAdapter = NULL;

	ASSERT(prNetDev);

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc == 2) {
		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Parse);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
			       u4Ret);
		if (u4Parse) {
			/* LDPC is enabled. */
			prAdapter->rWifiVar.ucTxLdpc = TRUE;
			prAdapter->rWifiVar.ucRxLdpc = TRUE;
		} else {
			/* LDPC is disabled. */
			prAdapter->rWifiVar.ucTxLdpc = FALSE;
			prAdapter->rWifiVar.ucRxLdpc = FALSE;
		}

		DBGLOG(REQ, STATE, "prAdapter->rWifiVar.ucTxLdpc is %d\n",
			prAdapter->rWifiVar.ucTxLdpc);
		DBGLOG(REQ, STATE, "prAdapter->rWifiVar.ucRxLdpc is %d\n",
			prAdapter->rWifiVar.ucRxLdpc);
	} else {
		DBGLOG(INIT, ERROR, "iwpriv wlanXX driver TX_PPDU\n");
		DBGLOG(INIT, ERROR, "<enable> 1: enable. 0: disable.\n");
	}

	return i4BytesWritten;
}

/* This command is for sigma to force tx amsdu. */
int priv_driver_set_tx_force_amsdu(struct net_device *prNetDev,
	char *pcCommand,
	int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret, u4Parse = 0;
	struct ADAPTER *prAdapter = NULL;

	ASSERT(prNetDev);

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;


	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc == 2) {
		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Parse);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
			       u4Ret);
		if (u4Parse) {
			/* forceAmsdu is enabled. */
			prAdapter->rWifiVar.ucHeCertForceAmsdu = TRUE;
		} else {
			/* forceAmsdu is disabled. */
			prAdapter->rWifiVar.ucHeCertForceAmsdu = FALSE;
		}

		DBGLOG(REQ, STATE,
			"prAdapter->rWifiVar.ucHeCertForceAmsdu is %d\n",
			prAdapter->rWifiVar.ucHeCertForceAmsdu);
	} else {
		DBGLOG(INIT, ERROR, "iwpriv wlanXX driver FORCE_AMSDU_TX\n");
		DBGLOG(INIT, ERROR, "<enable> 1: enable. 0: disable.\n");
	}

	return i4BytesWritten;
}

/* This command is for sigma to change OM CH BW. */
int priv_driver_set_om_ch_bw(struct net_device *prNetDev, char *pcCommand,
			 int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret, u4Parse = 0;
	struct ADAPTER *prAdapter = NULL;

	ASSERT(prNetDev);

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc == 2) {
		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Parse);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
			       u4Ret);

		DBGLOG(REQ, STATE,
			"priv_driver_set_om_ch_bw:: ch bw = %d\n", u4Parse);
		if (u4Parse <= CH_BW_160) {
#if (CFG_SUPPORT_802_11BE == 1)
			if (prAdapter->fgEhtHtcOM) {
				EHT_SET_HTC_HE_OM_CH_WIDTH(
					prAdapter->u4HeHtcOM, u4Parse);
			} else
#endif
				HE_SET_HTC_HE_OM_CH_WIDTH(
					prAdapter->u4HeHtcOM, u4Parse);
		}
	} else {
		DBGLOG(INIT, ERROR,
			"iwpriv wlanXX driver TX_ACTION <number>\n");
		DBGLOG(INIT, ERROR, "<number> action frame count.\n");
	}

	return i4BytesWritten;
}

/* This command is for sigma to change OM RX NSS. */
int priv_driver_set_om_rx_nss(
	struct net_device *prNetDev,
	char *pcCommand,
	int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret, u4Parse = 0;
	struct ADAPTER *prAdapter = NULL;

	ASSERT(prNetDev);

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;


	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc == 2) {
		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Parse);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
			       u4Ret);

		DBGLOG(REQ, STATE,
			"priv_driver_set_om_rx_nss:: rx nss = %d\n", u4Parse);
#if (CFG_SUPPORT_802_11BE == 1)
		if (prAdapter->fgEhtHtcOM) {
			EHT_SET_HTC_HE_OM_RX_NSS(prAdapter->u4HeHtcOM, u4Parse);
		} else
#endif
			HE_SET_HTC_HE_OM_RX_NSS(prAdapter->u4HeHtcOM, u4Parse);
	} else {
		DBGLOG(INIT, ERROR,
			"iwpriv wlanXX driver TX_ACTION <number>\n");
		DBGLOG(INIT, ERROR, "<number> action frame count.\n");
	}

	return i4BytesWritten;
}

int priv_driver_set_om_tx_nss(
	struct net_device *prNetDev,
	char *pcCommand,
	int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret, u4Parse = 0;
	struct ADAPTER *prAdapter = NULL;

	ASSERT(prNetDev);

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;


	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc == 2) {
		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Parse);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
			       u4Ret);

		DBGLOG(REQ, STATE,
			"priv_driver_set_om_tx_nss:: tx nss = %d\n", u4Parse);
#if (CFG_SUPPORT_802_11BE == 1)
		if (prAdapter->fgEhtHtcOM) {
			EHT_SET_HTC_HE_OM_TX_NSTS(prAdapter->u4HeHtcOM,
						u4Parse);
		} else
#endif
			HE_SET_HTC_HE_OM_TX_NSTS(prAdapter->u4HeHtcOM, u4Parse);
	} else {
		DBGLOG(INIT, ERROR,
			"iwpriv wlanXX driver SET_OM_TXNSTS <number>\n");
		DBGLOG(INIT, ERROR, "<number> action frame count.\n");
	}

	return i4BytesWritten;
}

int priv_driver_set_om_mu_dis(
	struct net_device *prNetDev,
	char *pcCommand,
	int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret, u4Parse = 0;
	struct ADAPTER *prAdapter = NULL;

	ASSERT(prNetDev);

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc == 2) {
		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Parse);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
			       u4Ret);

		DBGLOG(REQ, STATE,
			"priv_driver_set_om_mu_dis:: disable = %d\n", u4Parse);
#if (CFG_SUPPORT_802_11BE == 1)
		if (prAdapter->fgEhtHtcOM) {
			EHT_SET_HTC_HE_OM_UL_MU_DISABLE(prAdapter->u4HeHtcOM,
							u4Parse);
		} else
#endif
			HE_SET_HTC_HE_OM_UL_MU_DISABLE(prAdapter->u4HeHtcOM,
							u4Parse);
	} else {
		DBGLOG(INIT, ERROR,
			"iwpriv wlanXX driver SET_OM_MU_DISABLE <number>\n");
		DBGLOG(INIT, ERROR, "<number> action frame count.\n");
	}

	return i4BytesWritten;
}

int priv_driver_set_om_mu_data_dis(
	struct net_device *prNetDev,
	char *pcCommand,
	int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret, u4Parse = 0;
	struct ADAPTER *prAdapter = NULL;

	ASSERT(prNetDev);

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;


	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc == 2) {
		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Parse);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
			       u4Ret);

		DBGLOG(REQ, STATE,
			"priv_driver_set_om_mu_data_dis:: disable = %d\n",
			u4Parse);
		HE_SET_HTC_HE_OM_UL_MU_DATA_DISABLE(prAdapter->u4HeHtcOM,
			u4Parse);
	} else {
		DBGLOG(INIT, ERROR,
			"iwpriv wlanXX driver SET_OM_MU_DATA_DISABLE <number>\n");
		DBGLOG(INIT, ERROR, "<number> action frame count.\n");
	}

	return i4BytesWritten;
}

#if (CFG_SUPPORT_802_11BE == 1)
int priv_driver_set_eht_om(
	struct net_device *prNetDev,
	char *pcCommand,
	int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	struct ADAPTER *prAdapter = NULL;

	ASSERT(prNetDev);

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc < 2) {
		DBGLOG(REQ, STATE, "priv_driver_set_eht_om\n");
		ehtRlmInitHtcACtrlOM(prAdapter);
	} else {
		DBGLOG(INIT, ERROR, "iwpriv wlanXX driver SET_EHT_OM %d\n",
			i4Argc);
	}

	return i4BytesWritten;
}


int priv_driver_set_eht_om_rx_nss_ext(
	struct net_device *prNetDev,
	char *pcCommand,
	int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret, u4Parse = 0;
	struct ADAPTER *prAdapter = NULL;

	ASSERT(prNetDev);

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc == 2) {
		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Parse);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
			       u4Ret);

		DBGLOG(REQ, STATE,
			"priv_driver_set_eht_om_rx_nss_ext: %d\n", u4Parse);
		EHT_SET_HTC_EHT_OM_RX_NSS_EXT(prAdapter->u4HeHtcOM, u4Parse);
	} else {
		DBGLOG(INIT, ERROR,
			"iwpriv wlanXX driver SET_EHT_OM_RXNSS_EXT <number>\n");
	}

	return i4BytesWritten;
}
int priv_driver_set_eht_om_ch_bw_ext(
	struct net_device *prNetDev,
	char *pcCommand,
	int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret, u4Parse = 0;
	struct ADAPTER *prAdapter = NULL;

	ASSERT(prNetDev);

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc == 2) {
		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Parse);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
			       u4Ret);

		DBGLOG(REQ, STATE,
			"priv_driver_set_eht_om_ch_bw_ext: %d\n", u4Parse);
		EHT_SET_HTC_EHT_OM_CH_WIDTH_EXT(prAdapter->u4HeHtcOM, u4Parse);
	} else {
		DBGLOG(INIT, ERROR,
			"iwpriv wlanXX driver SET_EHT_OM_CHBW_EXT <number>\n");
	}

	return i4BytesWritten;
}

int priv_driver_set_eht_om_tx_nsts_ext(
	struct net_device *prNetDev,
	char *pcCommand,
	int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret, u4Parse = 0;
	struct ADAPTER *prAdapter = NULL;

	ASSERT(prNetDev);

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc == 2) {
		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Parse);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
			       u4Ret);

		DBGLOG(REQ, STATE,
			"priv_driver_set_eht_om_tx_nsts_ext: %d\n", u4Parse);
		EHT_SET_HTC_EHT_OM_TX_NSTS_EXT(prAdapter->u4HeHtcOM, u4Parse);
	} else {
		DBGLOG(INIT, ERROR,
			"iwpriv wlanX driver SET_EHT_OM_TXNSTS_EXT <number>\n");
	}

	return i4BytesWritten;
}

#endif

int priv_driver_set_tx_om_packet(
	struct net_device *prNetDev,
	char *pcCommand,
	int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret, u4Parse = 0;
	struct ADAPTER *prAdapter = NULL;
	int32_t index;
	struct STA_RECORD *prStaRec = NULL;
	uint8_t ucBssIndex;
	struct BSS_INFO *prBssInfo;

	ASSERT(prNetDev);

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;
	ucBssIndex = wlanGetBssIdx(prNetDev);
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc == 2) {
		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Parse);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
			       u4Ret);
		/* Null frame MAX number limit to 10. */
		if (u4Parse > 10)
			u4Parse = 10;

		DBGLOG(REQ, STATE,
			"tx om packet:: Send %d htc null frame\n",
			u4Parse);
		if (u4Parse) {
			if (prBssInfo && IS_BSS_AIS(prBssInfo)
				&& prBssInfo->prStaRecOfAP)
				prStaRec = prBssInfo->prStaRecOfAP;
			if (prStaRec != NULL) {
				for (index = 0; index < u4Parse; index++)
					heRlmSendHtcNullFrame(prAdapter,
						prStaRec, 7, NULL);
			}
		}
	} else {
		DBGLOG(INIT, ERROR,
			"iwpriv wlanXX driver TX_ACTION <number>\n");
		DBGLOG(INIT, ERROR, "<number> action frame count.\n");
	}

	/* Clear EHT setting, reset to HE setting. */
#if (CFG_SUPPORT_802_11BE == 1)
	prAdapter->fgEhtHtcOM = FALSE;
	heRlmInitHeHtcACtrlOMAndUPH(prAdapter);
#endif

	return i4BytesWritten;
}

int priv_driver_set_tx_cck_1m_pwr(
	struct net_device *prNetDev,
	char *pcCommand,
	int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret, u4Parse = 0;
	struct ADAPTER *prAdapter = NULL;
	uint8_t pwr;

	ASSERT(prNetDev);

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;


	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc == 2) {
		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Parse);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
			       u4Ret);

		DBGLOG(REQ, STATE,
			"priv_driver_set_tx_cck_1m_pwr:: set cck pwr %d\n",
			u4Parse);

		if (u4Parse) {
			pwr = u4Parse;

			wlanSendSetQueryCmd(prAdapter,
			CMD_ID_SET_CCK_1M_PWR,
			TRUE,
			FALSE,
			FALSE,
			NULL,
			NULL,
			sizeof(uint8_t),
			(uint8_t *)&pwr, NULL, 0);
		}
	} else {
		DBGLOG(INIT, ERROR,
			"iwpriv wlanXX driver TX_CCK_1M_PWR <pwr>\n");
		DBGLOG(INIT, ERROR, "<pwr> power of CCK 1M.\n");
	}

	return i4BytesWritten;
}

int priv_driver_set_sr_enable(
	struct net_device *prNetDev,
	char *pcCommand,
	int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret, u4Parse = 0;
	struct ADAPTER *prAdapter = NULL;
	struct _SR_CMD_SR_CAP_T *prCmdSrCap = NULL;

	ASSERT(prNetDev);

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	prCmdSrCap = (struct _SR_CMD_SR_CAP_T *)
		kalMemAlloc(sizeof(struct _SR_CMD_SR_CAP_T),
			    VIR_MEM_TYPE);

	if (!prCmdSrCap) {
		DBGLOG(REQ, WARN, "%s, alloc mem failed\n", __func__);
		return -ENOMEM;
	}

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc == 2) {
		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Parse);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
			       u4Ret);

		DBGLOG(REQ, STATE, "set_sr_enable %d\n", u4Parse);

		prCmdSrCap->rSrCmd.u1CmdSubId = SR_CMD_SET_SR_CAP_SREN_CTRL;
		prCmdSrCap->rSrCmd.u1DbdcIdx = 0;
		prCmdSrCap->rSrCap.fgSrEn = u4Parse;

		wlanSendSetQueryExtCmd(prAdapter,
			CMD_ID_LAYER_0_EXT_MAGIC_NUM,
			EXT_CMD_ID_SR_CTRL,
			TRUE,
			FALSE,
			TRUE,
			NULL,
			nicOidCmdTimeoutCommon,
			sizeof(struct _SR_CMD_SR_CAP_T),
			(uint8_t *) (prCmdSrCap),
			NULL, 0);

	} else {
		DBGLOG(INIT, ERROR, "iwpriv wlanXX driver SET_SR_ENABLE\n");
		DBGLOG(INIT, ERROR, "<enable> 1: enable. 0: disable.\n");
	}

	kalMemFree(prCmdSrCap, VIR_MEM_TYPE,
		   sizeof(struct _SR_CMD_SR_CAP_T));

	return i4BytesWritten;
}

int priv_driver_get_sr_cap(
	struct net_device *prNetDev,
	char *pcCommand,
	int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	struct ADAPTER *prAdapter = NULL;
	struct _SR_CMD_SR_CAP_T *prCmdSrCap = NULL;

	ASSERT(prNetDev);

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	prCmdSrCap = (struct _SR_CMD_SR_CAP_T *)
		kalMemAlloc(sizeof(struct _SR_CMD_SR_CAP_T),
			    VIR_MEM_TYPE);

	if (!prCmdSrCap) {
		DBGLOG(REQ, WARN, "%s, alloc mem failed\n", __func__);
		return -ENOMEM;
	}

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	if (i4Argc == 2) {
		uint32_t u4Ret = 0, u4Parse = 0;

		prCmdSrCap->rSrCmd.u1CmdSubId = SR_CMD_GET_SR_CAP_ALL_INFO;
		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Parse);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
					       u4Ret);
		prCmdSrCap->rSrCmd.u1DbdcIdx = (uint8_t) u4Parse;

		wlanSendSetQueryExtCmd(prAdapter,
			CMD_ID_LAYER_0_EXT_MAGIC_NUM,
			EXT_CMD_ID_SR_CTRL,
			FALSE,
			FALSE,
			TRUE,
			NULL,
			nicOidCmdTimeoutCommon,
			sizeof(struct _SR_CMD_SR_CAP_T),
			(uint8_t *) (prCmdSrCap),
			NULL, 0);
	} else
		DBGLOG(INIT, ERROR, "iwpriv wlanXX driver GET_SR_CAP\n");
#else
	if (i4Argc == 1) {

		prCmdSrCap->rSrCmd.u1CmdSubId = SR_CMD_GET_SR_CAP_ALL_INFO;
		prCmdSrCap->rSrCmd.u1DbdcIdx = 0;

		wlanSendSetQueryExtCmd(prAdapter,
			CMD_ID_LAYER_0_EXT_MAGIC_NUM,
			EXT_CMD_ID_SR_CTRL,
			TRUE,
			FALSE,
			TRUE,
			NULL,
			nicOidCmdTimeoutCommon,
			sizeof(struct _SR_CMD_SR_CAP_T),
			(uint8_t *) (prCmdSrCap),
			NULL, 0);

	} else
		DBGLOG(INIT, ERROR, "iwpriv wlanXX driver GET_SR_CAP\n");
#endif
	kalMemFree(prCmdSrCap, VIR_MEM_TYPE,
		   sizeof(struct _SR_CMD_SR_CAP_T));

	return i4BytesWritten;
}

int priv_driver_get_sr_ind(
	struct net_device *prNetDev,
	char *pcCommand,
	int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	struct ADAPTER *prAdapter = NULL;
	struct _SR_CMD_SR_IND_T *prCmdSrInd = NULL;

	ASSERT(prNetDev);

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	prCmdSrInd = (struct _SR_CMD_SR_IND_T *)
		kalMemAlloc(sizeof(struct _SR_CMD_SR_IND_T),
			    VIR_MEM_TYPE);

	if (!prCmdSrInd) {
		DBGLOG(REQ, WARN, "%s, alloc mem failed\n", __func__);
		return -ENOMEM;
	}

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	if (i4Argc == 2) {
		uint32_t u4Ret = 0, u4Parse = 0;

		prCmdSrInd->rSrCmd.u1CmdSubId = SR_CMD_GET_SR_IND_ALL_INFO;
		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Parse);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
					       u4Ret);
		prCmdSrInd->rSrCmd.u1DbdcIdx = (uint8_t) u4Parse;
		wlanSendSetQueryExtCmd(prAdapter,
			CMD_ID_LAYER_0_EXT_MAGIC_NUM,
			EXT_CMD_ID_SR_CTRL,
			FALSE,
			FALSE,
			TRUE,
			NULL,
			nicOidCmdTimeoutCommon,
			sizeof(struct _SR_CMD_SR_IND_T),
			(uint8_t *) (prCmdSrInd),
			NULL, 0);
	} else
		DBGLOG(INIT, ERROR, "iwpriv wlanXX driver GET_SR_IND\n");
#else
	if (i4Argc == 1) {
		prCmdSrInd->rSrCmd.u1CmdSubId = SR_CMD_GET_SR_IND_ALL_INFO;
		prCmdSrInd->rSrCmd.u1DbdcIdx = 0;

		wlanSendSetQueryExtCmd(prAdapter,
			CMD_ID_LAYER_0_EXT_MAGIC_NUM,
			EXT_CMD_ID_SR_CTRL,
			TRUE,
			FALSE,
			TRUE,
			NULL,
			nicOidCmdTimeoutCommon,
			sizeof(struct _SR_CMD_SR_IND_T),
			(uint8_t *) (prCmdSrInd),
			NULL, 0);
	} else
		DBGLOG(INIT, ERROR, "iwpriv wlanXX driver GET_SR_IND\n");
#endif
	kalMemFree(prCmdSrInd, VIR_MEM_TYPE,
		   sizeof(struct _SR_CMD_SR_IND_T));

	return i4BytesWritten;
}

int priv_driver_set_pp_rx(struct net_device *prNetDev, char *pcCommand,
			 int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret = 0, u4Parse = 0;
	uint8_t ucPpRxCap;

	ASSERT(prNetDev);

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc == 2) {
		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Parse);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
			       u4Ret);

		ucPpRxCap = (uint8_t) u4Parse;
		if (ucPpRxCap) {
			/* enable */
			prGlueInfo->prAdapter->rWifiVar.ucStaHePpRx = TRUE;
		} else {
			/* disable */
			prGlueInfo->prAdapter->rWifiVar.ucStaHePpRx = FALSE;
		}

		DBGLOG(REQ, ERROR, "ucPpRxCap = %d\n",
			prGlueInfo->prAdapter->rWifiVar.ucStaHePpRx);
	} else {
		DBGLOG(INIT, ERROR, "iwpriv wlan0 driver SET_PP_RX_CAP <en>\n");
		DBGLOG(INIT, ERROR, "<en> 1: enable. 0: disable.\n");
	}

	return i4BytesWritten;
}

int priv_driver_set_rx_ctrl_to_muti_bss(struct net_device *prNetDev,
		char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret, u4Parse = 0;
	struct ADAPTER *prAdapter = NULL;

	ASSERT(prNetDev);

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;


	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc == 2) {
		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Parse);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
			       u4Ret);
		if (u4Parse) {
			/* RxCtrlToMutiBss is enabled. */
			prAdapter->rWifiVar.ucRxCtrlToMutiBss = TRUE;
		} else {
			/* RxCtrlToMutiBss is disabled. */
			prAdapter->rWifiVar.ucRxCtrlToMutiBss = FALSE;
		}

		DBGLOG(REQ, STATE, "RxCtrlToMutiBss is %d\n",
			prAdapter->rWifiVar.ucRxCtrlToMutiBss);
	} else {
		DBGLOG(INIT, ERROR,
			"iwpriv wlanXX driver SET_RX_CTRL_TO_MUTI_BSS <number>\n");
		DBGLOG(INIT, ERROR, "<enable> 1: enable. 0: disable.\n");
	}

	return i4BytesWritten;
}

#endif /* CFG_SUPPORT_802_11AX == 1 */


int priv_driver_get_sta_index(struct net_device *prNetDev,
				     char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter;
	int32_t i4BytesWritten = 0, i4Argc = 0;
	uint8_t ucStaIdx, ucWlanIndex = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint8_t aucMacAddr[MAC_ADDR_LEN] = {0};

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc >= 2) {
		wlanHwAddrToBin(apcArgv[1], &aucMacAddr[0]);

		if (!wlanGetWlanIdxByAddress(prGlueInfo->prAdapter,
		    &aucMacAddr[0], &ucWlanIndex))
			return i4BytesWritten;

		if (wlanGetStaIdxByWlanIdx(prAdapter, ucWlanIndex, &ucStaIdx)
		    != WLAN_STATUS_SUCCESS)
			return i4BytesWritten;

		i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"StaIdx = %d, WlanIdx = %d\n", ucStaIdx,
				ucWlanIndex);
	}

	return i4BytesWritten;
}

int priv_driver_get_version(struct net_device *prNetDev,
				   char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter;
	int32_t i4BytesWritten = 0;
	uint32_t u4Offset = 0;

	ASSERT(prNetDev);

	prGlueInfo = *((struct GLUE_INFO **)netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	u4Offset += fwDlGetFwdlInfo(prAdapter, pcCommand, i4TotalLen);
	u4Offset += kalSnprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
		"WiFi Driver Version %u.%u.%u %s\n",
		NIC_DRIVER_MAJOR_VERSION,
		NIC_DRIVER_MINOR_VERSION,
		NIC_DRIVER_SERIAL_VERSION,
		DRIVER_BUILD_DATE);

	i4BytesWritten = (int32_t)u4Offset;

	return i4BytesWritten;
}

#if CFG_CHIP_RESET_HANG
int priv_driver_set_rst_hang(struct net_device *prNetDev,
				char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret;


	ASSERT(prNetDev);
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc == 0) {
		DBGLOG(REQ, INFO, "set_rst_hang Argc = %d\n", i4Argc);
		return -EFAULT;
	}

	if (strnicmp(apcArgv[0], CMD_SET_RST_HANG,
				strlen(CMD_SET_RST_HANG)) == 0) {
		if (i4Argc < CMD_SET_RST_HANG_ARG_NUM) {
			DBGLOG(REQ, STATE,
				"[SER][L0] RST_HANG_SET arg num=%d,must be %d\n",
				i4Argc, CMD_SET_RST_HANG_ARG_NUM);
			return -EFAULT;
		}
		u4Ret = kalkStrtou8(apcArgv[1], 0, &fgIsResetHangState);
		if (u4Ret)
			DBGLOG(REQ, ERROR, "u4Ret=%d\n", u4Ret);

		DBGLOG(REQ, STATE, "[SER][L0] set fgIsResetHangState=%d\n",
							fgIsResetHangState);

		if (fgIsResetHangState == SER_L0_HANG_RST_CMD_TRG) {
			DBGLOG(REQ, STATE, "[SER][L0] cmd trigger\n");
			GL_USER_DEFINE_RESET_TRIGGER(NULL, RST_CMD_TRIGGER,
						     RST_FLAG_DO_WHOLE_RESET);
		}

	} else {
		DBGLOG(REQ, STATE, "[SER][L0] get fgIsResetSqcState=%d\n",
							fgIsResetHangState);
		DBGLOG(REQ, ERROR, "[SER][L0] RST HANG subcmd(%s) error !\n",
								apcArgv[0]);

		return -EFAULT;
	}

	return 0;

}
#endif

#if CFG_SUPPORT_DBDC
int priv_driver_set_dbdc(struct net_device *prNetDev, char *pcCommand,
			 int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret, u4Parse = 0;
	uint8_t ucDBDCEnable;

	ASSERT(prNetDev);
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	if (prGlueInfo->prAdapter->rWifiVar.eDbdcMode !=
	    ENUM_DBDC_MODE_DYNAMIC) {
		DBGLOG(REQ, LOUD,
		       "Current DBDC mode %u cannot enable/disable DBDC!!\n",
		       prGlueInfo->prAdapter->rWifiVar.eDbdcMode);
		return -1;
	}

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Parse);
	if (u4Ret)
		DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
		       u4Ret);

	ucDBDCEnable = (uint8_t) u4Parse;
	if ((!prGlueInfo->prAdapter->rWifiVar.fgDbDcModeEn &&
	     !ucDBDCEnable) ||
	    (prGlueInfo->prAdapter->rWifiVar.fgDbDcModeEn &&
	     ucDBDCEnable))
		return i4BytesWritten;

	rStatus = kalIoctl(prGlueInfo, wlanoidSetDbdcEnable,
			   &ucDBDCEnable, 1, &u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS)
		return -1;

	return i4BytesWritten;
}

int priv_driver_set_sta1ss(struct net_device *prNetDev, char *pcCommand,
			 int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret, u4Parse = 0;

	ASSERT(prNetDev);

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Parse);
	if (u4Ret)
		DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
		       u4Ret);

	prGlueInfo->prAdapter->rWifiVar.fgSta1NSS = (uint8_t) u4Parse;

	return i4BytesWritten;
}

#endif /*CFG_SUPPORT_DBDC*/

int priv_driver_get_mcu_info(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct CHIP_DBG_OPS *prChipDbg = NULL;
	uint32_t i4BytesWritten = 0;
	uint8_t result = 0;

	ASSERT(prNetDev);
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prChipDbg = prGlueInfo->prAdapter->chip_info->prDebugOps;

	if (prChipDbg->show_mcu_debug_info) {
		result = prChipDbg->show_mcu_debug_info(prGlueInfo->prAdapter,
			pcCommand, i4TotalLen, DBG_MCU_DBG_ALL,
			&i4BytesWritten);
		if (!result)
			DBGLOG(INIT, WARN,
				"show_mcu_debug_info fail!\n");
	} else {
		DBGLOG(INIT, WARN,
			"Function not defined or not support!\n");
	}

	return i4BytesWritten;
}

#if (CFG_SUPPORT_DEBUG_SOP == 1)
int priv_driver_get_sleep_dbg_info(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct CHIP_DBG_OPS *prChipDbg = NULL;
	uint32_t i4BytesWritten = 0;
	uint8_t result = 0;

	if (prNetDev == NULL) {
		DBGLOG(INIT, WARN, "prNetDev is NULL!\n");

		return i4BytesWritten;
	}

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prChipDbg = prGlueInfo->prAdapter->chip_info->prDebugOps;

	if (prChipDbg->show_debug_sop_info) {
		result = prChipDbg->show_debug_sop_info(prGlueInfo->prAdapter,
			SLEEP);
		if (!result)
			DBGLOG(INIT, WARN,
				"show_debug_sop_info fail!\n");
	} else {
		DBGLOG(INIT, WARN,
			"Function not defined or not support!\n");
	}

	return i4BytesWritten;
}
#endif

/*----------------------------------------------------------------------------*/
/*!
* \brief    The function for command "iwpriv wlan0 driver get_ser".
*
* \param[in] prNetDev
* \param[in/out] pcCommand   input as pointer to command arguments and
*                            output as pointer to command result
* \param[in] i4TotalLen   the maximum length allowed for command result
*
* \return the actual length of command result
*/
/*----------------------------------------------------------------------------*/
int priv_driver_get_ser_info(struct net_device *prNetDev,
				    char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	uint32_t u4Offset = 0;
	struct PARAM_SER_INFO_T rQuerySerInfo;
	uint16_t i, j;

	ASSERT(prNetDev);

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	ASSERT(prGlueInfo);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	rStatus = kalIoctl(prGlueInfo, wlanoidQuerySerInfo,
			   &rQuerySerInfo, sizeof(rQuerySerInfo), &u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, ERROR, "rStatus 0x%8X\n", rStatus);

		return -1;
	}

#if (CFG_CHIP_RESET_SUPPORT == 1)

	if (prGlueInfo->prAdapter->chip_info->fgIsSupportL0p5Reset) {
		u4Offset += kalSnprintf(pcCommand + u4Offset,
					i4TotalLen - u4Offset,
				    "\n======== SER L0.5 reset cnt  ========\n"
			    "== empty result means NO L0.5 reset happens ==\n");

		if (prGlueInfo->prAdapter->u2WfsysResetCnt != 0)
			u4Offset += kalSnprintf(pcCommand + u4Offset,
						i4TotalLen - u4Offset,
						"L0.5 reset cnt %d\n",
					prGlueInfo->prAdapter->u2WfsysResetCnt);
	}

#endif /* CFG_CHIP_RESET_SUPPORT */

	u4Offset += kalSnprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
				"\n======== SER L1~L4 Enable bits ========\n");

	u4Offset += kalSnprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
				"BIT[0]:SER_ENABLE_TRACKING\n");

	u4Offset += kalSnprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
				"BIT[1]:SER_ENABLE_L1_RECOVER\n");

	u4Offset += kalSnprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
				"BIT[2]:SER_ENABLE_L2_RECOVER\n");

	u4Offset += kalSnprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
				"BIT[3]:SER_ENABLE_L3_RX_ABORT\n");

	u4Offset += kalSnprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
				"BIT[4]:SER_ENABLE_L3_TX_ABORT\n");

	u4Offset += kalSnprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
				"BIT[5]:SER_ENABLE_L3_TX_DISABLE\n");

	u4Offset += kalSnprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
				"BIT[6]:SER_ENABLE_L3_BF_RECOVER\n");

	u4Offset += kalSnprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
				"BIT[7]:SER_ENABLE_L4_MDP_RECOVER\n");

	u4Offset += kalSnprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
				"ucEnableSER=0x%08X\n",
				rQuerySerInfo.ucEnableSER);

	u4Offset += kalSnprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
				"\n======== SER L1~L4 reset cnt ========\n"
			    "== empty result mean NO L1~L4 reset happens ==\n");

	if (rQuerySerInfo.ucSerL1RecoverCnt != 0)
		u4Offset += kalSnprintf(pcCommand + u4Offset,
					i4TotalLen - u4Offset,
					"L1 reset cnt %d\n",
					rQuerySerInfo.ucSerL1RecoverCnt);

	if (rQuerySerInfo.ucSerL2RecoverCnt != 0)
		u4Offset += kalSnprintf(pcCommand + u4Offset,
					i4TotalLen - u4Offset,
					"L2 reset cnt %d\n",
					rQuerySerInfo.ucSerL2RecoverCnt);

	if (rQuerySerInfo.ucSerL3BfRecoverCnt != 0)
		u4Offset += kalSnprintf(pcCommand + u4Offset,
					i4TotalLen - u4Offset,
					"L3 BF reset cnt %d\n",
					rQuerySerInfo.ucSerL3BfRecoverCnt);

	for (i = 0; i < EXT_EVENT_SER_RAM_BAND_NUM; i++) {
		if (rQuerySerInfo.ucSerL3RxAbortCnt[i] != 0)
			u4Offset += kalSnprintf(pcCommand + u4Offset,
						i4TotalLen - u4Offset,
					    "[BN%d] L3 RX ABORT reset cnt %d\n",
						i,
					    rQuerySerInfo.ucSerL3RxAbortCnt[i]);

		if (rQuerySerInfo.ucSerL3TxAbortCnt[i] != 0)
			u4Offset += kalSnprintf(pcCommand + u4Offset,
						i4TotalLen - u4Offset,
					    "[BN%d] L3 TX ABORT reset cnt %d\n",
						i,
					    rQuerySerInfo.ucSerL3TxAbortCnt[i]);

		if (rQuerySerInfo.ucSerL3TxDisableCnt[i] != 0)
			u4Offset += kalSnprintf(pcCommand + u4Offset,
						i4TotalLen - u4Offset,
					  "[BN%d] L3 TX DISABLE reset cnt %d\n",
						i,
					  rQuerySerInfo.ucSerL3TxDisableCnt[i]);

		if (rQuerySerInfo.ucSerL4RecoverCnt[i] != 0)
			u4Offset += kalSnprintf(pcCommand + u4Offset,
						i4TotalLen - u4Offset,
						"[BN%d] L4 reset cnt %d\n",
						i,
					    rQuerySerInfo.ucSerL4RecoverCnt[i]);
	}

	u4Offset += kalSnprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
				"\n======== SER HW ERR cnt ========\n"
				"== empty result mean NO HW ERR happens ==\n");

	for (i = 0; i < EXT_EVENT_SER_RAM_BAND_NUM; i++) {
		for (j = 0; j < EXT_EVENT_SER_MAX_HW_ERROR_INT_NUMBER; j++) {
			if (rQuerySerInfo.u2LMACError6Cnt[i][j] != 0)
				u4Offset += kalSnprintf(pcCommand + u4Offset,
							i4TotalLen - u4Offset,
					    "[BN%d] LMAC ERR6 bit[%d] cnt %d\n",
							i, j,
					   rQuerySerInfo.u2LMACError6Cnt[i][j]);
		}

		for (j = 0; j < EXT_EVENT_SER_MAX_HW_ERROR_INT_NUMBER; j++) {
			if (rQuerySerInfo.u2LMACError7Cnt[i][j] != 0)
				u4Offset += kalSnprintf(pcCommand + u4Offset,
							i4TotalLen - u4Offset,
					    "[BN%d] LMAC ERR7 bit[%d] cnt %d\n",
							i, j,
					   rQuerySerInfo.u2LMACError7Cnt[i][j]);
		}
	}

	for (j = 0; j < EXT_EVENT_SER_MAX_HW_ERROR_INT_NUMBER; j++) {
		if (rQuerySerInfo.u2PSEErrorCnt[j] != 0)
			u4Offset += kalSnprintf(pcCommand + u4Offset,
						i4TotalLen - u4Offset,
						"PSE ERR bit[%d] cnt %d\n",
						j,
						rQuerySerInfo.u2PSEErrorCnt[j]);
	}

	for (j = 0; j < EXT_EVENT_SER_MAX_HW_ERROR_INT_NUMBER; j++) {
		if (rQuerySerInfo.u2PSEError1Cnt[j] != 0)
			u4Offset += kalSnprintf(pcCommand + u4Offset,
						i4TotalLen - u4Offset,
						"PSE ERR1 bit[%d] cnt %d\n",
						j,
					       rQuerySerInfo.u2PSEError1Cnt[j]);
	}

	for (j = 0; j < EXT_EVENT_SER_MAX_HW_ERROR_INT_NUMBER; j++) {
		if (rQuerySerInfo.u2PLEErrorCnt[j] != 0)
			u4Offset += kalSnprintf(pcCommand + u4Offset,
						i4TotalLen - u4Offset,
						"PLE ERR bit[%d] cnt %d\n",
						j,
						rQuerySerInfo.u2PLEErrorCnt[j]);
	}

	for (j = 0; j < EXT_EVENT_SER_MAX_HW_ERROR_INT_NUMBER; j++) {
		if (rQuerySerInfo.u2PLEError1Cnt[j] != 0)
			u4Offset += kalSnprintf(pcCommand + u4Offset,
						i4TotalLen - u4Offset,
						"PLE ERR1 bit[%d] cnt %d\n",
						j,
					       rQuerySerInfo.u2PLEError1Cnt[j]);
	}

	for (j = 0; j < EXT_EVENT_SER_MAX_HW_ERROR_INT_NUMBER; j++) {
		if (rQuerySerInfo.u2PLEErrorAmsduCnt[j] != 0)
			u4Offset += kalSnprintf(pcCommand + u4Offset,
						i4TotalLen - u4Offset,
					       "PLE ERR AMSDU bit[%d] cnt %d\n",
						j,
					   rQuerySerInfo.u2PLEErrorAmsduCnt[j]);
	}

	if (rQuerySerInfo.ucEvtVer == 0)
		goto end;

	/* for rQuerySerInfo.ucEvtVer == 1 kalSnprintf implementation ...
	 *
	 *	if (rQuerySerInfo.ucEvtVer == 1)
	 *		goto end;
	 *
	 *      ...
	 */

end:
	i4BytesWritten = (int32_t)u4Offset;

	return i4BytesWritten;

} /* priv_driver_get_ser_info */

int priv_driver_get_emi_info(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint8_t *buf = NULL;
	uint32_t offset = 0, size = 0, idx = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	int32_t i4ArgNum = 3;
	uint32_t u4Ret = 0;

	if (!prNetDev)
		goto exit;

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		goto exit;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc < i4ArgNum) {
		DBGLOG(REQ, ERROR, "Expect arg num %d but %d\n",
			i4ArgNum, i4Argc);
		goto exit;
	}

	u4Ret = kalkStrtou32(apcArgv[1], 0, &offset);
	if (u4Ret)
		DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n", u4Ret);
	u4Ret = kalkStrtou32(apcArgv[2], 0, &size);
	if (u4Ret)
		DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n", u4Ret);

	DBGLOG(REQ, INFO, "offset: 0x%x, size: 0x%x\n",
		offset, size);

	if (size == 0)
		goto exit;

	buf = kalMemAlloc(size, VIR_MEM_TYPE);
	if (!buf)
		goto exit;
	kalMemZero(buf, size);

	if (emi_mem_read(prGlueInfo->prAdapter->chip_info, offset, buf,
			 size)) {
		DBGLOG(REQ, ERROR, "emi_mem_read failed.\n");
		goto exit;
	}
	DBGLOG_MEM32(REQ, INFO, buf, size);
	while (idx < size) {
		if ((idx % 16) == 0)
			i4BytesWritten += kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"\n");
		else if ((idx % 8) == 0)
			i4BytesWritten += kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"  ");
		else if ((idx % 4) == 0)
			i4BytesWritten += kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				" ");

		i4BytesWritten += kalSnprintf(
			pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%02x",
			buf[idx]);

		idx++;
	}

exit:
	if (buf)
		kalMemFree(buf, VIR_MEM_TYPE, size);

	return i4BytesWritten;
}

int priv_driver_query_thermal_temp(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *glue = NULL;
	struct ADAPTER *ad = NULL;
	struct mt66xx_chip_info *chip_info = NULL;
	struct thermal_info *thermal_info = NULL;
	struct THERMAL_TEMP_DATA data;
	uint32_t status = WLAN_STATUS_SUCCESS;
	uint8_t idx = 0;
	int32_t written = 0;

	if (!prNetDev)
		goto exit;

	glue = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	if (glue->u4ReadyFlag == 0) {
		DBGLOG(REQ, INFO, "Skip due to driver NOT ready.\n");
		goto exit;
	}

	ad = glue->prAdapter;
	chip_info = ad->chip_info;
	thermal_info = &chip_info->thermal_info;

	if (thermal_info == NULL) {
		DBGLOG(REQ, INFO, "Skip due to chip NOT supported.\n");
		goto exit;
	}

	written += kalSnprintf(pcCommand + written,
			       i4TotalLen - written,
			       "\n");
	for (idx = 0; idx < thermal_info->sensor_num; idx++) {
		struct thermal_sensor_info *sensor =
			&thermal_info->sensor_info[idx];

		data.eType = sensor->type;
		data.ucIdx = sensor->sendor_idx;

		status = wlanQueryThermalTemp(ad, &data);
		if (status != WLAN_STATUS_SUCCESS)
			break;

		written += kalSnprintf(pcCommand + written,
				       i4TotalLen - written,
				       "[%d][%s] temp=%d\n",
				       idx,
				       sensor->name,
				       data.u4Temperature);
	}

exit:

	return written;
}

int priv_driver_get_que_info(struct net_device *prNetDev,
				    char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;

	ASSERT(prNetDev);
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	return qmDumpQueueStatus(prGlueInfo->prAdapter, pcCommand, i4TotalLen);
}

int priv_driver_get_mem_info(struct net_device *prNetDev,
				    char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;

	ASSERT(prNetDev);
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	return cnmDumpMemoryStatus(prGlueInfo->prAdapter, pcCommand,
				   i4TotalLen);
}

int priv_driver_get_hif_info(struct net_device *prNetDev,
				    char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;

	ASSERT(prNetDev);
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	return halDumpHifStatus(prGlueInfo->prAdapter, pcCommand, i4TotalLen);
}

int priv_driver_get_capab_rsdb(struct net_device *prNetDev,
				 char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	uint32_t u4Offset = 0;
	u_int8_t fgDbDcModeEn = FALSE;

	prGlueInfo = wlanGetGlueInfo();
	if (!prGlueInfo) {
		DBGLOG(REQ, WARN, "prGlueInfo is NULL\n");
		return -EFAULT;
	}
	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);

#if CFG_SUPPORT_DBDC
	if (prGlueInfo->prAdapter->rWifiVar.eDbdcMode !=
	    ENUM_DBDC_MODE_DISABLED)
		fgDbDcModeEn = TRUE;

	if (prGlueInfo->prAdapter->rWifiFemCfg.u2WifiPath ==
	    (WLAN_FLAG_2G4_WF0 | WLAN_FLAG_5G_WF1))
		fgDbDcModeEn = TRUE;
#endif

	DBGLOG(REQ, INFO, "RSDB:%d\n", fgDbDcModeEn);

	u4Offset += kalScnprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
			     "RSDB:%d",
			     fgDbDcModeEn);

	i4BytesWritten = (int32_t)u4Offset;

	return i4BytesWritten;

}

int priv_driver_get_cnm(struct net_device *prNetDev,
			       char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	struct PARAM_GET_CNM_T *prCnmInfo = NULL;
	u_int8_t fgIsApMode = false;

	enum ENUM_MBMC_BN	eDbdcIdx, eDbdcIdxMax;
	uint8_t ucBssIdx;
	struct BSS_INFO *prBssInfo;
	enum ENUM_CNM_NETWORK_TYPE_T eNetworkType;
	uint8_t ucOpRxNss, ucOpTxNss;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);

	prCnmInfo = (struct PARAM_GET_CNM_T *)kalMemAlloc(
				sizeof(struct PARAM_GET_CNM_T), VIR_MEM_TYPE);
	if (prCnmInfo == NULL)
		return -1;

	kalMemZero(prCnmInfo, sizeof(struct PARAM_GET_CNM_T));

	rStatus = kalIoctl(prGlueInfo, wlanoidQueryCnm, prCnmInfo,
			   sizeof(struct PARAM_GET_CNM_T), &u4BufLen);

	if (prGlueInfo->prAdapter->rWifiVar.eDbdcMode == ENUM_DBDC_MODE_STATIC)
		prCnmInfo->fgIsDbdcEnable = TRUE;

	DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__, pcCommand);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		kalMemFree(prCnmInfo,
			VIR_MEM_TYPE, sizeof(struct PARAM_GET_CNM_T));
		return -1;
	}

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
				   i4TotalLen - i4BytesWritten,
				   "\n[CNM Info]\n");
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
				   i4TotalLen - i4BytesWritten,
				   "MBMC Mode : %s\n\n",
				   (prCnmInfo->fgIsDbdcEnable) ?
				   "Enable" : "Disable");

	eDbdcIdxMax = ENUM_BAND_NUM;
	for (eDbdcIdx = ENUM_BAND_0; eDbdcIdx < eDbdcIdxMax; eDbdcIdx++) {
		/* Do not clean history information */
		/* if argc is bigger than 1 */
		if (i4Argc < 2) {
			if (prCnmInfo->ucOpChNum[eDbdcIdx] < 3)
				prCnmInfo->ucChList[eDbdcIdx][2] = 0;
			if (prCnmInfo->ucOpChNum[eDbdcIdx] < 2)
				prCnmInfo->ucChList[eDbdcIdx][1] = 0;
			if (prCnmInfo->ucOpChNum[eDbdcIdx] < 1)
				prCnmInfo->ucChList[eDbdcIdx][0] = 0;
		}

#if (CFG_GET_CNM_INFO_BC == 1)
		/* backward compatible for 7668 format */
		i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
					   i4TotalLen - i4BytesWritten,
					   "BAND %u channels : %u %u %u\n",
					   eDbdcIdx,
					   prCnmInfo->ucChList[eDbdcIdx][0],
					   prCnmInfo->ucChList[eDbdcIdx][1],
					   prCnmInfo->ucChList[eDbdcIdx][2]);
#endif

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
					   i4TotalLen - i4BytesWritten,
					   "Band %u OPCH %d [%u, %u, %u]\n",
					   eDbdcIdx,
					   prCnmInfo->ucOpChNum[eDbdcIdx],
					   prCnmInfo->ucChList[eDbdcIdx][0],
					   prCnmInfo->ucChList[eDbdcIdx][1],
					   prCnmInfo->ucChList[eDbdcIdx][2]);
	}
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
				   i4TotalLen - i4BytesWritten, "\n");

	for (ucBssIdx = BSSID_0; ucBssIdx < (MAX_BSSID_NUM + 1); ucBssIdx++) {

		prBssInfo = prGlueInfo->prAdapter->aprBssInfo[ucBssIdx];
		if (!prBssInfo)
			continue;

		eNetworkType = cnmGetBssNetworkType(prBssInfo);
		if (prCnmInfo->ucBssInuse[ucBssIdx] &&
		    prCnmInfo->ucBssActive[ucBssIdx] &&
		    ((eNetworkType == ENUM_CNM_NETWORK_TYPE_P2P_GO) ||
		     ((eNetworkType == ENUM_CNM_NETWORK_TYPE_AIS ||
		       eNetworkType == ENUM_CNM_NETWORK_TYPE_P2P_GC ||
		       eNetworkType == ENUM_CNM_NETWORK_TYPE_NAN) &&
		      (prCnmInfo->ucBssConnectState[ucBssIdx] ==
		       MEDIA_STATE_CONNECTED)))) {
			ucOpRxNss = prBssInfo->ucOpRxNss;
			if (eNetworkType == ENUM_CNM_NETWORK_TYPE_P2P_GO) {
				struct STA_RECORD *prCurrStaRec =
						(struct STA_RECORD *) NULL;

				prCurrStaRec = LINK_PEEK_HEAD(
						&prBssInfo->rStaRecOfClientList,
						struct STA_RECORD, rLinkEntry);

				if (prCurrStaRec != NULL &&
				    IS_CONNECTION_NSS2(prBssInfo,
				    prCurrStaRec)) {
					ucOpTxNss = 2;
				} else
					ucOpTxNss = 1;

				ucOpRxNss = prBssInfo->ucOpRxNss;
			} else if (prBssInfo->prStaRecOfAP != NULL &&
				   IS_CONNECTION_NSS2(prBssInfo,
				   prBssInfo->prStaRecOfAP)) {
				ucOpTxNss = 2;
			} else
				ucOpTxNss = 1;
		} else {
			eNetworkType = ENUM_CNM_NETWORK_TYPE_OTHER;
			ucOpTxNss = prBssInfo->ucOpTxNss;
			ucOpRxNss = prBssInfo->ucOpRxNss;
			/* Do not show history information */
			/* if argc is 1 */
			if (i4Argc < 2)
				continue;
		}

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"BSS%u Inuse%u Act%u ConnStat%u",
			ucBssIdx,
			prCnmInfo->ucBssInuse[ucBssIdx],
			prCnmInfo->ucBssActive[ucBssIdx],
			prCnmInfo->ucBssConnectState[ucBssIdx]);

		if (eNetworkType == ENUM_CNM_NETWORK_TYPE_P2P_GO)
			fgIsApMode =
			  prGlueInfo->prAdapter->rWifiVar.prP2PConnSettings[
			  prBssInfo->u4PrivateData]->fgIsApMode;

		switch (eNetworkType) {
		case ENUM_CNM_NETWORK_TYPE_OTHER:
			i4BytesWritten +=
			    kalSnprintf(pcCommand + i4BytesWritten,
			    i4TotalLen - i4BytesWritten, " [NetType Other]");
			break;
		case ENUM_CNM_NETWORK_TYPE_AIS:
			i4BytesWritten +=
			    kalSnprintf(pcCommand + i4BytesWritten,
			    i4TotalLen - i4BytesWritten, " [NetType STA]");
			break;
		case ENUM_CNM_NETWORK_TYPE_P2P_GC:
			i4BytesWritten +=
			    kalSnprintf(pcCommand + i4BytesWritten,
			    i4TotalLen - i4BytesWritten, " [NetType GC]");
			break;
		case ENUM_CNM_NETWORK_TYPE_P2P_GO:
			if (fgIsApMode) {
				i4BytesWritten +=
				    kalSnprintf(pcCommand + i4BytesWritten,
				    i4TotalLen - i4BytesWritten,
				    " [NetType SAP]");
			} else {
				i4BytesWritten +=
				    kalSnprintf(pcCommand + i4BytesWritten,
				    i4TotalLen - i4BytesWritten,
				    " [NetType GO]");
			}
			break;
		case ENUM_CNM_NETWORK_TYPE_NAN:
			i4BytesWritten +=
			    kalSnprintf(pcCommand + i4BytesWritten,
			    i4TotalLen - i4BytesWritten, " [NetType NAN]");
			break;
		default:
			break;
		}

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"[CH%3u][Band b%u][BW%s][TxNSS%u][RxNss%u]",
			prCnmInfo->ucBssCh[ucBssIdx],
			prCnmInfo->ucBssDBDCBand[ucBssIdx],
			bssOpBw2Str(prBssInfo),
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
			prCnmInfo->ucBssOpTxNss[ucBssIdx],
			prCnmInfo->ucBssOpRxNss[ucBssIdx]);
#else
			ucOpTxNss,
			ucOpRxNss);
#endif

#if (CFG_SUPPORT_802_11BE_MLO == 1)
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"[MLD Group %d][MLO LinkId %d]",
		prBssInfo->ucGroupMldId, prCnmInfo->ucBssLinkIdx[ucBssIdx]);
#endif

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten, "\n");

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten, "[OMAC_ADDR "MACSTR"]\n",
			MAC2STR(prBssInfo->aucOwnMacAddr));
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten, "[BSSID "MACSTR"]\n\n",
			MAC2STR(prBssInfo->aucBSSID));
	}

	kalMemFree(prCnmInfo, VIR_MEM_TYPE, sizeof(struct PARAM_GET_CNM_T));
	return i4BytesWritten;
}				/* priv_driver_get_sw_ctrl */

int priv_driver_get_ch_rank_list(struct net_device *prNetDev,
					char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t i4BytesWritten = 0;
	struct PARAM_GET_CHN_INFO *prChnLoadInfo = NULL;
	struct RF_CHANNEL_INFO aucChannelList[MAX_PER_BAND_CHN_NUM];
	uint8_t i = 0, ucBandIdx = 0, ucChnIdx = 0, ucNumOfChannel = 0;

	ASSERT(prNetDev);
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prChnLoadInfo = &(prGlueInfo->prAdapter->rWifiVar.rChnLoadInfo);
	kalMemZero(pcCommand, i4TotalLen);
	kalMemZero(aucChannelList, sizeof(aucChannelList));

	for (ucBandIdx = BAND_2G4; ucBandIdx < BAND_NUM; ucBandIdx++) {
		rlmDomainGetChnlList(prGlueInfo->prAdapter, ucBandIdx,
			TRUE, MAX_PER_BAND_CHN_NUM,
			&ucNumOfChannel, aucChannelList);

		for (i = 0; i < ucNumOfChannel; i++) {
			ucChnIdx = wlanGetChannelIndex(
				aucChannelList[i].eBand,
				aucChannelList[i].ucChannelNum);

			pcCommand[i4BytesWritten++] =
				prChnLoadInfo->rChnRankList[ucChnIdx].ucChannel;

			DBGLOG(SCN, TRACE, "band %u, ch %u, dirtiness %d\n",
				prChnLoadInfo->rChnRankList[ucChnIdx].eBand,
				prChnLoadInfo->rChnRankList[ucChnIdx].ucChannel,
				prChnLoadInfo->rChnRankList[ucChnIdx]
					.u4Dirtiness);
		}
	}

	return i4BytesWritten;
}

int priv_driver_get_ch_dirtiness(struct net_device *prNetDev,
					char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t i4BytesWritten = 0, u4Offset = 0;
	struct PARAM_GET_CHN_INFO *prChnLoadInfo = NULL;
	struct RF_CHANNEL_INFO aucChannelList[MAX_PER_BAND_CHN_NUM];
	uint8_t i = 0, ucBandIdx = 0, ucChnIdx = 0, ucNumOfChannel = 0;

	ASSERT(prNetDev);
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prChnLoadInfo = &(prGlueInfo->prAdapter->rWifiVar.rChnLoadInfo);
	kalMemZero(pcCommand, i4TotalLen);
	kalMemZero(aucChannelList, sizeof(aucChannelList));

	for (ucBandIdx = BAND_2G4; ucBandIdx < BAND_NUM; ucBandIdx++) {
		rlmDomainGetChnlList(prGlueInfo->prAdapter, ucBandIdx,
			TRUE, MAX_PER_BAND_CHN_NUM,
			&ucNumOfChannel, aucChannelList);

		for (i = 0; i < ucNumOfChannel; i++) {
			ucChnIdx = wlanGetChannelIndex(
				aucChannelList[i].eBand,
				aucChannelList[i].ucChannelNum);

			u4Offset = kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"\nband %u ch %03u -> dirtiness %u",
				prChnLoadInfo->rChnRankList[ucChnIdx].eBand,
				prChnLoadInfo->rChnRankList[ucChnIdx].ucChannel,
				prChnLoadInfo->rChnRankList[ucChnIdx]
					.u4Dirtiness);

			i4BytesWritten += u4Offset;
		}
	}

	return i4BytesWritten;
}

int priv_driver_efuse_ops(struct net_device *prNetDev,
				 char *pcCommand, int i4TotalLen)
{
	enum EFUSE_OP_MODE {
		EFUSE_READ,
		EFUSE_WRITE,
		EFUSE_FREE,
		EFUSE_INVALID,
	};
	uint8_t ucOpMode = EFUSE_INVALID;
	uint8_t ucOpChar;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret;
	int32_t i4Parameter = 0;
	uint32_t u4Efuse_addr = 0;
	uint8_t ucEfuse_value = 0;
	uint32_t u4Offset = 0;
#if  (CFG_EEPROM_PAGE_ACCESS == 1)
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	uint8_t  u4Index = 0;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct PARAM_CUSTOM_ACCESS_EFUSE rAccessEfuseInfo;
#endif
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	/* Sanity check */
	if (i4Argc < 2)
		goto efuse_op_invalid;

	ucOpChar = (uint8_t)apcArgv[1][0];
	if ((i4Argc == 3) && (ucOpChar == 'r' || ucOpChar == 'R'))
		ucOpMode = EFUSE_READ;
	else if ((i4Argc == 4) && (ucOpChar == 'w' || ucOpChar == 'W'))
		ucOpMode = EFUSE_WRITE;
	else if ((ucOpChar == 'f' || ucOpChar == 'F'))
		ucOpMode = EFUSE_FREE;

	/* Print out help if input format is wrong */
	if (ucOpMode == EFUSE_INVALID)
		goto efuse_op_invalid;

	/* convert address */
	if (ucOpMode == EFUSE_READ || ucOpMode == EFUSE_WRITE) {
		u4Ret = kalkStrtos32(apcArgv[2], 16, &i4Parameter);
		u4Efuse_addr = (uint32_t)i4Parameter;
	}

	/* convert value */
	if (ucOpMode == EFUSE_WRITE) {
		u4Ret = kalkStrtos32(apcArgv[3], 16, &i4Parameter);
		ucEfuse_value = (uint8_t)i4Parameter;
	}

	/* Start operation */
#if  (CFG_EEPROM_PAGE_ACCESS == 1)
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	if (prGlueInfo == NULL || !prGlueInfo->prAdapter) {
		DBGLOG(REQ, ERROR, "prGlueInfo is null\n");
		goto efuse_op_invalid;
	}

	if (prGlueInfo->prAdapter->chip_info &&
	    !prGlueInfo->prAdapter->chip_info->is_support_efuse) {
		u4Offset += kalSnprintf(pcCommand + u4Offset,
				     i4TotalLen - u4Offset,
				     "efuse ops is invalid\n");
		return (int32_t)u4Offset;
	}

	kalMemSet(&rAccessEfuseInfo, 0,
		  sizeof(struct PARAM_CUSTOM_ACCESS_EFUSE));
	rAccessEfuseInfo.u4Address = (u4Efuse_addr / EFUSE_BLOCK_SIZE)
				     * EFUSE_BLOCK_SIZE;

	u4Index = u4Efuse_addr % EFUSE_BLOCK_SIZE;

	if (ucOpMode == EFUSE_READ) {
		rStatus = kalIoctl(prGlueInfo,
				   wlanoidQueryProcessAccessEfuseRead,
				   &rAccessEfuseInfo,
				   sizeof(struct PARAM_CUSTOM_ACCESS_EFUSE),
				   &u4BufLen);

		if (rStatus == WLAN_STATUS_SUCCESS) {
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
			u4Offset += kalSnprintf(pcCommand + u4Offset,
			     i4TotalLen - u4Offset,
			     "Read success 0x%X = 0x%X\n", u4Efuse_addr,
			     rAccessEfuseInfo.aucData[u4Index]);
#else
			u4Offset += kalSnprintf(pcCommand + u4Offset,
			     i4TotalLen - u4Offset,
			     "Read success 0x%X = 0x%X\n", u4Efuse_addr,
			     prGlueInfo->prAdapter->aucEepromVaule[u4Index]);
#endif
		}
	} else if (ucOpMode == EFUSE_WRITE) {

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
		/* update write buffer first */
		rStatus = kalIoctl(prGlueInfo,
				   wlanoidQueryProcessAccessEfuseRead,
				   &rAccessEfuseInfo,
				   sizeof(struct PARAM_CUSTOM_ACCESS_EFUSE),
				   &u4BufLen);
		if (rStatus != WLAN_STATUS_SUCCESS)
			goto efuse_op_invalid;


		rAccessEfuseInfo.aucData[u4Index] = ucEfuse_value;
#else
		prGlueInfo->prAdapter->aucEepromVaule[u4Index] = ucEfuse_value;

		kalMemCopy(rAccessEfuseInfo.aucData,
			   prGlueInfo->prAdapter->aucEepromVaule, 16);
#endif

		rStatus = kalIoctl(prGlueInfo,
				   wlanoidQueryProcessAccessEfuseWrite,
				   &rAccessEfuseInfo,
				   sizeof(struct PARAM_CUSTOM_ACCESS_EFUSE),
				   &u4BufLen);
		if (rStatus == WLAN_STATUS_SUCCESS) {
			u4Offset += kalSnprintf(pcCommand + u4Offset,
					     i4TotalLen - u4Offset,
					     "Write success 0x%X = 0x%X\n",
					     u4Efuse_addr, ucEfuse_value);
		}
	} else if (ucOpMode == EFUSE_FREE) {
		struct PARAM_CUSTOM_EFUSE_FREE_BLOCK rEfuseFreeBlock = {};

		if (prGlueInfo->prAdapter->fgIsSupportGetFreeEfuseBlockCount
		    == FALSE) {
			u4Offset += kalSnprintf(pcCommand + u4Offset,
					     i4TotalLen - u4Offset,
					     "Cannot read free block size\n");
			return (int32_t)u4Offset;
		}
		rStatus = kalIoctl(prGlueInfo, wlanoidQueryEfuseFreeBlock,
				   &rEfuseFreeBlock,
				   sizeof(struct PARAM_CUSTOM_EFUSE_FREE_BLOCK),
				   &u4BufLen);
		if (rStatus == WLAN_STATUS_SUCCESS) {
			u4Offset += kalSnprintf(pcCommand + u4Offset,
				     i4TotalLen - u4Offset,
				     "Free block size 0x%X, Total block 0x%X\n",
				     rEfuseFreeBlock.ucGetFreeBlock,
				     rEfuseFreeBlock.ucGetTotalBlock);
		}
	}
#else
	u4Offset += kalSnprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
					"efuse ops is invalid\n");
#endif

	return (int32_t)u4Offset;

efuse_op_invalid:

#if CFG_SUPPORT_TPENHANCE_MODE
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
#endif /* CFG_SUPPORT_TPENHANCE_MODE */

	u4Offset += kalSnprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
				"\nHelp menu\n");
	u4Offset += kalSnprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
				"\tRead:\t\"efuse read addr_hex\"\n");
	u4Offset += kalSnprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
				"\tWrite:\t\"efuse write addr_hex val_hex\"\n");
	u4Offset += kalSnprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
				"\tFree Blocks:\t\"efuse free\"\n");
	return (int32_t)u4Offset;
}

#if defined(_HIF_SDIO) && (MTK_WCN_HIF_SDIO == 0)
int priv_driver_cccr_ops(struct net_device *prNetDev,
				 char *pcCommand, int i4TotalLen)
{
	enum CCCR_OP_MODE {
		CCCR_READ,
		CCCR_WRITE,
		CCCR_FREE,
		CCCR_INVALID,
	};
	uint8_t ucOpMode = CCCR_INVALID;
	uint8_t ucOpChar;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	int32_t i4Ret;
	int32_t i4Parameter;
	uint32_t u4CCCR_addr = 0;
	uint8_t ucCCCR_value = 0;

	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4Offset = 0;
	struct GLUE_INFO *prGlueInfo = NULL;

	struct sdio_func *func;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	ASSERT(prGlueInfo);

	if(!IS_SDIO_INF(prGlueInfo)){
		u4Offset += kalSnprintf(pcCommand + u4Offset,
				     i4TotalLen - u4Offset,
				     "Not SDIO bus(%d)\n",
				     prGlueInfo->u4InfType);
		return (int32_t)u4Offset;
	}

	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	/* Sanity check */
	if (i4Argc < 2)
		goto cccr_op_invalid;

	ucOpChar = (uint8_t)apcArgv[1][0];
	if ((i4Argc == 3) && (ucOpChar == 'r' || ucOpChar == 'R'))
		ucOpMode = CCCR_READ;
	else if ((i4Argc == 4) && (ucOpChar == 'w' || ucOpChar == 'W'))
		ucOpMode = CCCR_WRITE;

	/* Print out help if input format is wrong */
	if (ucOpMode == CCCR_INVALID)
		goto cccr_op_invalid;

	/* convert address */
	if (ucOpMode == CCCR_READ || ucOpMode == CCCR_WRITE) {
		i4Ret = kalkStrtos32(apcArgv[2], 16, &i4Parameter);
		/* Valid address 0x0~0xFF */
		u4CCCR_addr = (uint32_t)(i4Parameter & 0xFF);
	}

	/* convert value */
	if (ucOpMode == CCCR_WRITE) {
		i4Ret = kalkStrtos32(apcArgv[3], 16, &i4Parameter);
		ucCCCR_value = (uint8_t)i4Parameter;
	}

	/* Set SDIO host reference */
	func = prGlueInfo->rHifInfo.func;

	/* Start operation */
	if (ucOpMode == CCCR_READ) {
		sdio_claim_host(func);
		ucCCCR_value = sdio_f0_readb(func, u4CCCR_addr, &rStatus);
		sdio_release_host(func);

		if (rStatus) /* Fail case */
			u4Offset += kalSnprintf(pcCommand + u4Offset,
					     i4TotalLen - u4Offset,
					     "Read Fail 0x%X (ret=%d)\n",
					     u4CCCR_addr, rStatus);
		else
			u4Offset += kalSnprintf(pcCommand + u4Offset,
					     i4TotalLen - u4Offset,
					     "Read success 0x%X = 0x%X\n",
					     u4CCCR_addr, ucCCCR_value);
	} else if (ucOpMode == CCCR_WRITE) {
		uint32_t quirks_bak;
		sdio_claim_host(func);
		/* Enable capability to write CCCR */
		quirks_bak = func->card->quirks;
		func->card->quirks |= MMC_QUIRK_LENIENT_FN0;
		/* Write CCCR into card */
		sdio_f0_writeb(func, ucCCCR_value, u4CCCR_addr, &rStatus);
		func->card->quirks = quirks_bak;
		sdio_release_host(func);

		if (rStatus) /* Fail case */
			u4Offset += kalSnprintf(pcCommand + u4Offset,
					     i4TotalLen - u4Offset,
					     "Write Fail 0x%X (ret=%d)\n",
					     u4CCCR_addr, rStatus);
		else
			u4Offset += kalSnprintf(pcCommand + u4Offset,
					     i4TotalLen - u4Offset,
					     "Write success 0x%X = 0x%X\n",
					     u4CCCR_addr, ucCCCR_value);
	}

	return (int32_t)u4Offset;

cccr_op_invalid:

	u4Offset += kalSnprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
				"\nHelp menu\n");
	u4Offset += kalSnprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
				"\tRead:\t\"cccr read addr_hex\"\n");
	u4Offset += kalSnprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
				"\tWrite:\t\"cccr write addr_hex val_hex\"\n");
	return (int32_t)u4Offset;
}
#endif /* _HIF_SDIO && (MTK_WCN_HIF_SDIO == 0) */

#if CFG_SUPPORT_ADVANCE_CONTROL
int priv_driver_get_noise(struct net_device *prNetDev,
				 char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	uint32_t u4Id = CMD_SW_DBGCTL_ADVCTL_GET_ID + 1;
	uint32_t u4Offset = 0;
	struct PARAM_CUSTOM_SW_CTRL_STRUCT rSwCtrlInfo;
	int16_t u2Wf0AvgPwr, u2Wf1AvgPwr;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	rSwCtrlInfo.u4Data = 0;
	rSwCtrlInfo.u4Id = u4Id;

	rStatus = kalIoctl(prGlueInfo, wlanoidQuerySwCtrlRead, &rSwCtrlInfo,
			   sizeof(rSwCtrlInfo), &u4BufLen);

	DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
	if (rStatus != WLAN_STATUS_SUCCESS)
		return -1;

	u2Wf0AvgPwr = rSwCtrlInfo.u4Data & 0xFFFF;
	u2Wf1AvgPwr = (rSwCtrlInfo.u4Data >> 16) & 0xFFFF;

	u4Offset += kalSnprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
			     "Noise Idle Avg. Power: WF0:%ddB WF1:%ddB\n",
			     u2Wf0AvgPwr, u2Wf1AvgPwr);

	i4BytesWritten = (int32_t)u4Offset;

	return i4BytesWritten;

}				/* priv_driver_get_sw_ctrl */

int priv_driver_set_pop(struct net_device *prNetDev,
				char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	int32_t u4Ret = 0;
	uint32_t u4Id = CMD_SW_DBGCTL_ADVCTL_SET_ID + 2;
	uint32_t u4Sel = 0, u4CckTh = 0, u4OfdmTh = 0;
	struct PARAM_CUSTOM_SW_CTRL_STRUCT rSwCtrlInfo;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	rSwCtrlInfo.u4Id = u4Id;

	if (i4Argc <= 3) {
		DBGLOG(REQ, ERROR,
		       "Argc(%d) ERR: SET_POP <Sel> <CCK TH> <OFDM TH>\n",
		       i4Argc);
		return -1;
	}

	u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Sel);
	if (u4Ret)
		DBGLOG(REQ, ERROR, "parse rSwCtrlInfo error u4Ret=%d\n", u4Ret);
	u4Ret = kalkStrtou32(apcArgv[2], 0, &u4CckTh);
	if (u4Ret)
		DBGLOG(REQ, ERROR, "parse rSwCtrlInfo error u4Ret=%d\n", u4Ret);
	u4Ret = kalkStrtou32(apcArgv[3], 0, &u4OfdmTh);
	if (u4Ret)
		DBGLOG(REQ, ERROR, "parse rSwCtrlInfo error u4Ret=%d\n", u4Ret);

	rSwCtrlInfo.u4Data = (u4CckTh | (u4OfdmTh<<8) | (u4Sel<<30));
	DBGLOG(REQ, LOUD, "u4Sel=%d u4CckTh=%d u4OfdmTh=%d, u4Data=0x%x,\n",
		u4Sel, u4CckTh, u4OfdmTh, rSwCtrlInfo.u4Data);
	rStatus = kalIoctl(prGlueInfo, wlanoidSetSwCtrlWrite, &rSwCtrlInfo,
			   sizeof(rSwCtrlInfo), &u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, ERROR, "ERR: kalIoctl fail (%d)\n", rStatus);
		return -1;
	}

	return i4BytesWritten;

}

#if (CFG_SUPPORT_DYNAMIC_EDCCA == 1)
int priv_driver_set_ed(struct net_device *prNetDev,
			      char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	int32_t i4Ret = 0;
	uint32_t u4Sel = 0;
	struct ADAPTER *prAdapter = NULL;
	int32_t i4EdVal[2] = { 0 };

	if (!prNetDev) {
		DBGLOG(REQ, ERROR, "prNetDev is NULL\n");
		return WLAN_STATUS_FAILURE;
	}

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return WLAN_STATUS_FAILURE;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	prAdapter = prGlueInfo->prAdapter;
	if (!prAdapter) {
		DBGLOG(REQ, LOUD, "Adapter is NULL!\n");
		return WLAN_STATUS_FAILURE;
	}

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc <= 2) {
		DBGLOG(REQ, ERROR, "Argc(%d) ERR! Parameters for SET_ED:\n",
			i4Argc);
		DBGLOG(REQ, ERROR,
			"<Sel> <2.4G & 5G ED(-49~-81dBm)> or\n");
		DBGLOG(REQ, ERROR,
			"<Sel> <2.4G ED(-49~-81dBm)> <5G ED(-49~-81dBm)>\n");
		return WLAN_STATUS_FAILURE;
	}

	i4Ret = kalkStrtou32(apcArgv[1], 0, &u4Sel);
	if (i4Ret)
		DBGLOG(REQ, ERROR, "parse u4Sel error i4Ret=%d\n", i4Ret);

	i4Ret = kalkStrtos32(apcArgv[2], 0, &i4EdVal[0]);
	if (i4Ret)
		DBGLOG(REQ, ERROR,
			"parse i4EdVal(2.4G) error i4Ret=%d\n", i4Ret);

	i4Ret = kalkStrtos32(apcArgv[3], 0, &i4EdVal[1]);
	if (i4Ret)
		DBGLOG(REQ, ERROR,
			"parse i4EdVal(5G) error u4Ret=%d\n", i4Ret);

	/* Set the 2G & 5G ED with different value */
	u4Status = wlanSetEd(prAdapter, i4EdVal[0], i4EdVal[1], u4Sel);

	if (u4Status != WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, ERROR, "ERR: kalIoctl fail (%d)\n", u4Status);
		return WLAN_STATUS_FAILURE;
	}

	return i4BytesWritten;

}

int priv_driver_get_ed(struct net_device *prNetDev,
				char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	uint32_t u4Id = CMD_SW_DBGCTL_ADVCTL_GET_ID + CMD_ADVCTL_ED_ID;
	uint32_t u4Offset = 0;
	struct PARAM_CUSTOM_SW_CTRL_STRUCT rSwCtrlInfo;
	int8_t iEdVal[2] = { 0 };

	if (!prNetDev) {
		DBGLOG(REQ, ERROR, "prNetDev is NULL\n");
		return WLAN_STATUS_FAILURE;
	}

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return WLAN_STATUS_FAILURE;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	rSwCtrlInfo.u4Data = 0;
	rSwCtrlInfo.u4Id = u4Id;

	u4Status = kalIoctl(prGlueInfo, wlanoidQuerySwCtrlRead,
			&rSwCtrlInfo, sizeof(rSwCtrlInfo), &u4BufLen);

	DBGLOG(REQ, LOUD, "Status %u\n", u4Status);
	if (u4Status != WLAN_STATUS_SUCCESS)
		return WLAN_STATUS_FAILURE;

	iEdVal[0] = rSwCtrlInfo.u4Data & 0xFF;
	iEdVal[1] = (rSwCtrlInfo.u4Data >> 16) & 0xFF;

	u4Offset += snprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
			"ED: 2.4G(%ddB), 5G(%ddB)\n", iEdVal[0], iEdVal[1]);

	i4BytesWritten = (int32_t)u4Offset;

	return i4BytesWritten;
}
#endif /* CFG_SUPPORT_DYNAMIC_EDCCA */

int priv_driver_set_pd(struct net_device *prNetDev,
			      char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	int32_t u4Ret = 0;
	uint32_t u4Id = CMD_SW_DBGCTL_ADVCTL_SET_ID + 4;
	uint32_t u4Sel = 0;
	int32_t u4CckTh = 0, u4OfdmTh = 0;
	struct PARAM_CUSTOM_SW_CTRL_STRUCT rSwCtrlInfo;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	rSwCtrlInfo.u4Id = u4Id;

	if (i4Argc <= 1) {
		DBGLOG(REQ, ERROR,
		       "Argc(%d) ERR: SET_PD <Sel> [CCK TH] [OFDM TH]\n",
		       i4Argc);
		return -1;
	}

	u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Sel);
	if (u4Ret)
		DBGLOG(REQ, ERROR, "parse rSwCtrlInfo error u4Ret=%d\n", u4Ret);

	if (u4Sel == 1) {
		if (i4Argc <= 3) {
			DBGLOG(REQ, ERROR,
			       "Argc(%d) ERR: SET_PD 1 <CCK TH> <OFDM CH>\n",
			       i4Argc);
			return -1;
		}
		u4Ret = kalkStrtos32(apcArgv[2], 0, &u4CckTh);
		if (u4Ret)
			DBGLOG(REQ, ERROR, "parse rSwCtrlInfo error u4Ret=%d\n",
			       u4Ret);
		u4Ret = kalkStrtos32(apcArgv[3], 0, &u4OfdmTh);
		if (u4Ret)
			DBGLOG(REQ, ERROR, "parse rSwCtrlInfo error u4Ret=%d\n",
			       u4Ret);
	}

	rSwCtrlInfo.u4Data = ((u4OfdmTh & 0xFFFF) | ((u4CckTh & 0xFF) << 16) |
			      (u4Sel << 30));
	DBGLOG(REQ, LOUD, "u4Sel=%d u4OfdmTh=%d, u4CckTh=%d, u4Data=0x%x,\n",
		u4Sel, u4OfdmTh, u4CckTh, rSwCtrlInfo.u4Data);

	rStatus = kalIoctl(prGlueInfo, wlanoidSetSwCtrlWrite, &rSwCtrlInfo,
			   sizeof(rSwCtrlInfo), &u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, ERROR, "ERR: kalIoctl fail (%d)\n", rStatus);
		return -1;
	}

	return i4BytesWritten;
}

int priv_driver_set_maxrfgain(struct net_device *prNetDev,
				     char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	int32_t u4Ret = 0;
	uint32_t u4Id = CMD_SW_DBGCTL_ADVCTL_SET_ID + 5;
	uint32_t u4Sel = 0;
	int32_t u4Wf0Gain = 0, u4Wf1Gain = 0;
	struct PARAM_CUSTOM_SW_CTRL_STRUCT rSwCtrlInfo;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	rSwCtrlInfo.u4Id = u4Id;

	if (i4Argc <= 1) {
		DBGLOG(REQ, ERROR,
		       "Argc(%d) ERR: SET_RFGAIN <Sel> <WF0 Gain> <WF1 Gain>\n",
		       i4Argc);
		return -1;
	}

	u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Sel);
	if (u4Ret)
		DBGLOG(REQ, ERROR, "parse rSwCtrlInfo error u4Ret=%d\n", u4Ret);

	if (u4Sel == 1) {
		if (i4Argc <= 3) {
			DBGLOG(REQ, ERROR,
			       "Argc(%d) ERR: SET_RFGAIN 1 <WF0 Gain> <WF1 Gain>\n",
			       i4Argc);
			return -1;
		}
		u4Ret = kalkStrtos32(apcArgv[2], 0, &u4Wf0Gain);
		if (u4Ret)
			DBGLOG(REQ, ERROR, "parse rSwCtrlInfo error u4Ret=%d\n",
			       u4Ret);
		u4Ret = kalkStrtos32(apcArgv[3], 0, &u4Wf1Gain);
		if (u4Ret)
			DBGLOG(REQ, ERROR, "parse rSwCtrlInfo error u4Ret=%d\n",
			       u4Ret);
	}

	rSwCtrlInfo.u4Data = ((u4Wf0Gain & 0xFF) | ((u4Wf1Gain & 0xFF) << 8) |
			      (u4Sel << 31));
	DBGLOG(REQ, LOUD, "u4Sel=%d u4Wf0Gain=%d, u4Wf1Gain=%d, u4Data=0x%x,\n",
		u4Sel, u4Wf0Gain, u4Wf1Gain, rSwCtrlInfo.u4Data);

	rStatus = kalIoctl(prGlueInfo, wlanoidSetSwCtrlWrite, &rSwCtrlInfo,
			   sizeof(rSwCtrlInfo), &u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, ERROR, "ERR: kalIoctl fail (%d)\n", rStatus);
		return -1;
	}

	return i4BytesWritten;
}

#endif

#if (CFG_SURVEY_DUMP_FULL_CHANNEL == 1)
int priv_driver_get_survey_dump(struct net_device *prNetDev,
				    char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo;
	struct ADAPTER *prAdapter;
	int32_t i4BytesWritten = 0;
	uint32_t u4ChIdx, u4StartIdx, u4EndIdx;
	u_int8_t ucChannelNum;
	struct ieee80211_channel *pChannel;
	struct CMD_DOMAIN_CHANNEL *pCh;

	struct PARAM_CUSTOM_SW_CTRL_STRUCT rSwCtrlInfo;
	uint32_t u4Id;
	int16_t u2Wf0AvgPwr;
	int8_t i1Noise;
	uint32_t u4BufLen = 0;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	prGlueInfo = *((struct GLUE_INFO **)netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	DBGLOG(REQ, INFO, "command is %s\n", pcCommand);

	ucChannelNum = 0;
	u4StartIdx = 0;
	u4EndIdx = rlmDomainGetActiveChannelCount(KAL_BAND_2GHZ)
			+ rlmDomainGetActiveChannelCount(KAL_BAND_5GHZ);

	u4StartIdx = 0;
	for (u4ChIdx = u4StartIdx; u4ChIdx < u4EndIdx; u4ChIdx++) {
		pCh = (rlmDomainGetActiveChannels() + u4ChIdx);
		ucChannelNum = pCh->u2ChNum;

		if (ucChannelNum <= 14) {
			pChannel = ieee80211_get_channel(
				wlanGetWiphy(),
				ieee80211_channel_to_frequency
				(ucChannelNum, KAL_BAND_2GHZ));
		} else {
			pChannel = ieee80211_get_channel(
				wlanGetWiphy(),
				ieee80211_channel_to_frequency(ucChannelNum,
				KAL_BAND_5GHZ));
		}

		if (pChannel == NULL)
			return -1;

		u4Id = CMD_SW_DBGCTL_ADVCTL_GET_ID + CMD_ADVCTL_NOISE_ID;
		rSwCtrlInfo.u4Id = u4Id;
		rSwCtrlInfo.u4Data = 0;
		rStatus = kalIoctl(prGlueInfo, wlanoidQuerySwCtrlRead,
				  &rSwCtrlInfo, sizeof(rSwCtrlInfo), &u4BufLen);
		u2Wf0AvgPwr = rSwCtrlInfo.u4Data & 0xFFFF;
		i1Noise = u2Wf0AvgPwr;
		prGlueInfo->u1NoiseLevel = u2Wf0AvgPwr;

		DBGLOG(REQ, INFO, "\tfrequency:\t\t\t%u MHz\n",
			pChannel->center_freq);
		DBGLOG(REQ, INFO, "\tnoise:\t\t\t\t%d dBm\n",
			i1Noise);
		DBGLOG(REQ, INFO, "\tactive_time:\t\t\t %d ms\n",
			prGlueInfo->rChanTimeRecord[u4ChIdx].u4ActiveTime);
		DBGLOG(REQ, INFO, "\tbusy_time:\t\t\t %d us\n",
			prGlueInfo->rChanTimeRecord[u4ChIdx].u4BusyTime);
		DBGLOG(REQ, INFO, "\ttx_time:\t\t\t %d us\n",
			prGlueInfo->rChanTimeRecord[u4ChIdx].u4TxTime);
		DBGLOG(REQ, INFO, "\tchannel_num:\t\t\t %d\n\n",
			prGlueInfo->rChanTimeRecord[u4ChIdx].u2ChannelNum);
		DBGLOG(REQ, INFO, "\t\n\n");

	}

	return i4BytesWritten;
}
#endif

int priv_driver_get_tp_info(struct net_device *prNetDev,
				   char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;

	ASSERT(prNetDev);
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	return kalPerMonGetInfo(prGlueInfo->prAdapter, pcCommand, i4TotalLen);
}


#if (CFG_SUPPORT_TWT == 1)
int priv_driver_set_twtparams(
	struct net_device *prNetDev,
	char *pcCommand,
	int i4TotalLen)
{
	struct ADAPTER *prAdapter = NULL;
	struct WIFI_VAR *prWifiVar = NULL;
#if (CFG_SUPPORT_802_11BE_ML_TWT == 1)
	struct BSS_INFO *prBssInfo = NULL;
	struct MLD_BSS_INFO *prMldBssInfo = NULL;
#endif
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX_LONG] = { 0 };
	struct _TWT_CTRL_T rTWTCtrl = {0};
	struct _TWT_PARAMS_T *prTWTParams;
	uint16_t i;
	int32_t u4Ret = 0;
	uint32_t au4Setting[CMD_TWT_MAX_PARAMS] = {0};
	struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPrivate = NULL;
	struct _MSG_TWT_PARAMS_SET_T *prTWTParamSetMsg;
	uint64_t u8Val = 0x0;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prNetDevPrivate =
		(struct NETDEV_PRIVATE_GLUE_INFO *) netdev_priv(prNetDev);

	DBGLOG(REQ, INFO, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, INFO, "argc is %i\n", i4Argc);

	prAdapter = prNetDevPrivate->prGlueInfo->prAdapter;

	/* Check param number and convert TWT params to integer type */
	if ((i4Argc == CMD_TWT_ACTION_TEN_PARAMS) ||
		(i4Argc == CMD_TWT_ACTION_THREE_PARAMS) ||
		(i4Argc == CMD_TWT_ACTION_FIVE_PARAMS) ||
		(i4Argc == CMD_TWT_ACTION_SIX_PARAMS) ||
		(i4Argc == CMD_TWT_ACTION_NINE_PARAMS) ||
		(i4Argc == CMD_TWT_ACTION_TWELVE_PARAMS) ||
#ifdef CFG_SUPPORT_TWT_EXT
		(i4Argc == CMD_TWT_ACTION_FIFTEEN_PARAMS) ||
#endif
		(i4Argc == CMD_TWT_ACTION_SIXTEEN_PARAMS)) {
		for (i = 0; i < (i4Argc - 1); i++) {
			u4Ret = kalkStrtou32(apcArgv[i + 1],
				0, &(au4Setting[i]));

			if (u4Ret)
				DBGLOG(REQ, ERROR, "Argv error %d\n", u4Ret);
		}
	} else {
		DBGLOG(REQ, ERROR, "set_twtparams wrong argc : %d\n", i4Argc);
		return -1;
	}

	prWifiVar = &prAdapter->rWifiVar;

	if (IS_TWT_PARAM_ACTION_RESUME(au4Setting[0]) &&
		(i4Argc == CMD_TWT_ACTION_SIX_PARAMS)) {
		DBGLOG(REQ, INFO, "Action=%d\n", au4Setting[0]);
		DBGLOG(REQ, INFO, "TWT Flow ID=%d\n", au4Setting[1]);
		DBGLOG(REQ, INFO, "Next TWT size=%d\n", au4Setting[2]);
		DBGLOG(REQ, INFO, "Next TWT=%x %x\n",
			au4Setting[4], au4Setting[3]);

		if (au4Setting[1] >= TWT_MAX_FLOW_NUM) {
			/* Simple sanity check failure */
			DBGLOG(REQ, ERROR, "Invalid TWT Params\n");
			return -1;
		}

		rTWTCtrl.ucBssIdx = prNetDevPrivate->ucBssIdx;
		rTWTCtrl.ucCtrlAction = (uint8_t)au4Setting[0];
		rTWTCtrl.ucTWTFlowId = (uint8_t)au4Setting[1];
		rTWTCtrl.rNextTWT.ucNextTWTSize = au4Setting[2];
		rTWTCtrl.rNextTWT.u8NextTWT = (uint64_t)
			(((((uint64_t)au4Setting[4])<<32) &
			0xFFFFFFFF00000000)|au4Setting[3]);

		u8Val = rTWTCtrl.rNextTWT.u8NextTWT * 1000000;

		if ((u8Val & 0xFFFFFFFF00000000) != 0)
			rTWTCtrl.rNextTWT.ucNextTWTSize = 3;

		rTWTCtrl.rNextTWT.u8NextTWT = u8Val;
	} else if (IS_TWT_PARAM_ACTION_TESTBED_CONFIG(au4Setting[0]) &&
		(i4Argc == CMD_TWT_ACTION_THREE_PARAMS)) {
		DBGLOG(REQ, INFO, "Action=%d\n", au4Setting[0]);
		DBGLOG(REQ, INFO, "IsTestBed=%d\n", au4Setting[1]);

		g_IsWfaTestBed = (uint8_t)au4Setting[1];

		g_IsTwtLogo = 1;

		return 0;
	} else if ((IS_TWT_PARAM_ACTION_DEL(au4Setting[0]) ||
		IS_TWT_PARAM_ACTION_SUSPEND(au4Setting[0]) ||
		IS_TWT_PARAM_ACTION_ADD_BTWT(au4Setting[0]))
		&& (i4Argc == CMD_TWT_ACTION_THREE_PARAMS)) {

		DBGLOG(REQ, INFO, "Action=%d\n", au4Setting[0]);
		DBGLOG(REQ, INFO, "TWT Flow ID=%d\n", au4Setting[1]);

		if (au4Setting[1] >= TWT_MAX_FLOW_NUM) {
			/* Simple sanity check failure */
			DBGLOG(REQ, ERROR, "Invalid TWT Params\n");
			return -1;
		}

		rTWTCtrl.ucBssIdx = prNetDevPrivate->ucBssIdx;
		rTWTCtrl.ucCtrlAction = (uint8_t)au4Setting[0];
		rTWTCtrl.ucTWTFlowId = (uint8_t)au4Setting[1];
	}
#if (CFG_SUPPORT_802_11BE_ML_TWT == 1)
	else if (IS_TWT_PARAM_ACTION_ADD_ML_TWT_ALL_LINKS(au4Setting[0])
		&& (i4Argc == CMD_TWT_ACTION_TEN_PARAMS)) {
		/* Add ML-TWT all links sharing the same TWT param */
		/* Get BSSINFO of ML setup link */
		prBssInfo = GET_BSS_INFO_BY_INDEX(
						prAdapter,
						prNetDevPrivate->ucBssIdx);

		if (!prBssInfo) {
			DBGLOG(REQ, ERROR, "MLTWT Invalid BSS_INFO\n");

			return -1;
		}

		prMldBssInfo = mldBssGetByBss(prAdapter, prBssInfo);

		if (!prMldBssInfo) {
			DBGLOG(REQ, ERROR, "MLTWT Invalid MLD_BSS_INFO\n");

			return -1;
		}

		prBssInfo = mldGetBssInfoByLinkID(
						prAdapter,
						prMldBssInfo,
						0,
						TRUE);

		if (!prBssInfo) {
			DBGLOG(REQ, ERROR, "Find no MLTWT setup link\n");

			return -1;
		}

		DBGLOG(REQ, INFO, "MLTWT Action =%d\n", au4Setting[0]);
		DBGLOG(REQ, INFO,
			"MLTWT Flow ID=%d Setup Command=%d Trig enabled=%d\n",
			au4Setting[1], au4Setting[2], au4Setting[3]);
		DBGLOG(REQ, INFO,
			"MLTWT Unannounced enabled=%d Wake Interval Exponent=%d\n",
			au4Setting[4], au4Setting[5]);
		DBGLOG(REQ, INFO, "MLTWT Protection enabled=%d Duration=%d\n",
			au4Setting[6], au4Setting[7]);
		DBGLOG(REQ, INFO, "MLTWT Wake Interval Mantissa=%d\n",
					au4Setting[8]);
		/*
		 *	au2Setting[0]: MLTWT all in one command
		 *	au2Setting[1]: TWT Flow ID
		 *	au2Setting[2]: TWT Setup Command
		 *	au2Setting[3]: Trigger enabled
		 *	au2Setting[4]: Unannounced enabled
		 *	au2Setting[5]: TWT Wake Interval Exponent
		 *	au2Setting[6]: TWT Protection enabled
		 *	au2Setting[7]: Nominal Minimum TWT Wake Duration
		 *	au2Setting[8]: TWT Wake Interval Mantissa
		 */
		if (au4Setting[1] >= TWT_MAX_FLOW_NUM ||
			au4Setting[2] > TWT_SETUP_CMD_ID_DEMAND ||
			au4Setting[5] > TWT_MAX_WAKE_INTVAL_EXP) {
			/* Simple sanity check failure */
			DBGLOG(REQ, ERROR, "Invalid ML-TWT Params\n");

			return -1;
		}

		prTWTParams = &(rTWTCtrl.rTWTParams);
		kalMemSet(prTWTParams, 0, sizeof(struct _TWT_PARAMS_T));
		prTWTParams->fgReq = TRUE;
		prTWTParams->ucSetupCmd = (uint8_t) au4Setting[2];
		prTWTParams->fgTrigger = (au4Setting[3]) ? TRUE : FALSE;
		prTWTParams->fgUnannounced = (au4Setting[4]) ? TRUE : FALSE;
		prTWTParams->ucWakeIntvalExponent = (uint8_t) au4Setting[5];
		prTWTParams->fgProtect = (au4Setting[6]) ? TRUE : FALSE;
		prTWTParams->ucMinWakeDur = (uint8_t) au4Setting[7];
		prTWTParams->u2WakeIntvalMantiss = au4Setting[8];
		prTWTParams->fgByPassNego = FALSE;

		rTWTCtrl.ucBssIdx = prBssInfo->ucBssIndex;
		rTWTCtrl.ucCtrlAction = au4Setting[0];
		rTWTCtrl.ucTWTFlowId = au4Setting[1];
	} else if (IS_TWT_PARAM_ACTION_ADD_ML_TWT_ONE_BY_ONE(au4Setting[0])
		&& (i4Argc == CMD_TWT_ACTION_TWELVE_PARAMS)) {
		/* Add ML-TWT distinct link one by one */
		/* Get BSSINFO of ML setup link */
		prBssInfo = GET_BSS_INFO_BY_INDEX(
						prAdapter,
						prNetDevPrivate->ucBssIdx);

		if (!prBssInfo) {
			DBGLOG(REQ, ERROR, "MLTWT Invalid BSS_INFO\n");

			return -1;
		}

		prMldBssInfo = mldBssGetByBss(prAdapter, prBssInfo);

		if (!prMldBssInfo) {
			DBGLOG(REQ, ERROR, "MLTWT Invalid MLD_BSS_INFO\n");

			return -1;
		}

		prBssInfo = mldGetBssInfoByLinkID(
						prAdapter,
						prMldBssInfo,
						au4Setting[9],
						TRUE);

		if (!prBssInfo) {
			DBGLOG(REQ, ERROR,
				"Find no MLTWT target link %d\n",
				au4Setting[9]);

			return -1;
		}

		DBGLOG(REQ, INFO, "MLTWT Action =%d\n", au4Setting[0]);
		DBGLOG(REQ, INFO,
			"MLTWT Flow ID=%d Setup Command=%d Trig enabled=%d\n",
			au4Setting[1], au4Setting[2], au4Setting[3]);
		DBGLOG(REQ, INFO,
			"MLTWT Unannounced enabled=%d Wake Interval Exponent=%d\n",
			au4Setting[4], au4Setting[5]);
		DBGLOG(REQ, INFO, "ML Protection enabled=%d Duration=%d\n",
			au4Setting[6], au4Setting[7]);
		DBGLOG(REQ, INFO, "MLTWT Wake Interval Mantissa=%d\n",
					au4Setting[8]);
		DBGLOG(REQ, INFO, "MLTWT target link ID=%d\n", au4Setting[9]);
		DBGLOG(REQ, INFO, "MLTWT param last=%d\n", au4Setting[10]);
		/*
		 *	au2Setting[0]: MLTWT one by one command
		 *	au2Setting[1]: TWT Flow ID
		 *	au2Setting[2]: TWT Setup Command
		 *	au2Setting[3]: Trigger enabled
		 *	au2Setting[4]: Unannounced enabled
		 *	au2Setting[5]: TWT Wake Interval Exponent
		 *	au2Setting[6]: TWT Protection enabled
		 *	au2Setting[7]: Nominal Minimum TWT Wake Duration
		 *	au2Setting[8]: TWT Wake Interval Mantissa
		 *	au2Setting[9]: MLTWT link ID
		 *	au2Setting[10]: MLTWT param last: 0(No)|1(Yes)
		 */
		if (au4Setting[1] >= TWT_MAX_FLOW_NUM ||
			au4Setting[2] > TWT_SETUP_CMD_ID_DEMAND ||
			au4Setting[5] > TWT_MAX_WAKE_INTVAL_EXP) {
			/* Simple sanity check failure */
			DBGLOG(REQ, ERROR, "Invalid ML-TWT Params\n");

			return -1;
		}

		prTWTParams = &(rTWTCtrl.rTWTParams);
		kalMemSet(prTWTParams, 0, sizeof(struct _TWT_PARAMS_T));
		prTWTParams->fgReq = TRUE;
		prTWTParams->ucSetupCmd = (uint8_t) au4Setting[2];
		prTWTParams->fgTrigger = (au4Setting[3]) ? TRUE : FALSE;
		prTWTParams->fgUnannounced = (au4Setting[4]) ? TRUE : FALSE;
		prTWTParams->ucWakeIntvalExponent = (uint8_t) au4Setting[5];
		prTWTParams->fgProtect = (au4Setting[6]) ? TRUE : FALSE;
		prTWTParams->ucMinWakeDur = (uint8_t) au4Setting[7];
		prTWTParams->u2WakeIntvalMantiss = au4Setting[8];
		prTWTParams->fgByPassNego = FALSE;

		rTWTCtrl.ucBssIdx = prBssInfo->ucBssIndex;
		rTWTCtrl.ucCtrlAction = au4Setting[0];
		rTWTCtrl.ucTWTFlowId = au4Setting[1];
		rTWTCtrl.ucMLTWT_Param_Last = au4Setting[10];
	}
#endif
#if (CFG_SUPPORT_RTWT == 1)
	else if ((i4Argc == CMD_TWT_ACTION_SIXTEEN_PARAMS) &&
			IS_TWT_PARAM_ACTION_ADD_RTWT(au4Setting[0])) {
		DBGLOG(REQ, INFO, "RTWT Action=%d\n", au4Setting[0]);
		DBGLOG(REQ, INFO,
			"RTWT Flow ID=%d Setup Command=%d Trig enabled=%d\n",
			au4Setting[1], au4Setting[2], au4Setting[3]);
		DBGLOG(REQ, INFO,
			"RTWT Unannounced enabled=%d Wake Interval Exponent=%d\n",
			au4Setting[4], au4Setting[5]);
		DBGLOG(REQ, INFO, "RTWT Protection enabled=%d Duration=%d\n",
			au4Setting[6], au4Setting[7]);
		DBGLOG(REQ, INFO, "RTWT Interval Mantissa=%d\n", au4Setting[8]);
		DBGLOG(REQ, INFO, "RTWT Target Wake Time=%d\n", au4Setting[9]);
		DBGLOG(REQ, INFO, "RTWT traffic info=%d\n", au4Setting[10]);
		DBGLOG(REQ, INFO, "RTWT TID Bitmap Valid=%d\n", au4Setting[11]);
		DBGLOG(REQ, INFO, "RTWT DL TID Bitmap=%d\n", au4Setting[12]);
		DBGLOG(REQ, INFO, "RTWT UL TID Bitmap=%d\n", au4Setting[13]);
		DBGLOG(REQ, INFO, "RTWT target link ID=%d\n", au4Setting[14]);

		/*
		 * au4Setting[0]: ADD RTWT command
		 * au4Setting[1]: RTWT Flow ID
		 * au4Setting[2]: RTWT Setup Command
		 * au4Setting[3]: RTWT Trigger enabled
		 * au4Setting[4]: RTWT Unannounced enabled
		 * au4Setting[5]: RTWT Wake Interval Exponent
		 * au4Setting[6]: RTWT Protection enabled
		 * au4Setting[7]: RTWT Nominal Minimum TWT Wake Duration
		 * au4Setting[8]: RTWT Wake Interval Mantissa
		 * au4Setting[9]: RTWT Target Wake Time
		 * au4Setting[10]: RTWT traffic info present
		 * au4Setting[11]: RTWT DL/UL TID Bitmap Valid
		 * au4Setting[12]: RTWT DL TID Bitmap
		 * au4Setting[13]: RTWT UL TID Bitmap
		 * au4Setting[14]: RTWT target link ID
		 * , whereas au4Setting[14] = 0 also indicates single link MLD
		 * , since MLO link ID is assigned from AP MLD, for single link
		 * , single link MLD case, link ID 0 implicitly refers to
		 * prNetDevPrivate->ucBssIdx, the BSS_INFO[0] by most default.
		 */

		if (au4Setting[1] >= RTWT_MAX_FLOW_NUM ||
			au4Setting[2] > TWT_SETUP_CMD_ID_DEMAND ||
			au4Setting[5] > TWT_MAX_WAKE_INTVAL_EXP) {
			/* Simple sanity check failure */
			DBGLOG(REQ, INFO, "Invalid RTWT Params\n");

			return -1;
		}

		prBssInfo = GET_BSS_INFO_BY_INDEX(
						prAdapter,
						prNetDevPrivate->ucBssIdx);

		if (!prBssInfo) {
			DBGLOG(REQ, ERROR, "RTWT Invalid BSS_INFO\n");

			return -1;
		}

		prMldBssInfo = mldBssGetByBss(prAdapter, prBssInfo);

		if (!prMldBssInfo) {
			DBGLOG(REQ, ERROR, "RTWT Invalid MLD_BSS_INFO\n");

			return -1;
		}

		prBssInfo = mldGetBssInfoByLinkID(
						prAdapter,
						prMldBssInfo,
						au4Setting[14],
						TRUE);

		if (!prBssInfo) {
			if (au4Setting[14] > 0) {
				DBGLOG(REQ, ERROR,
					"Find no RTWT target link %d\n",
					au4Setting[14]);

				return -1;
			}

			/*
			 * au4Setting[14]: RTWT target link ID
			 * , whereas au4Setting[14] = 0 also indicates
			 * single link MLD, since MLO link ID is assigned
			 * from AP MLD, for single link, single link MLD
			 * case, link ID 0 implicitly refers to
			 * prNetDevPrivate->ucBssIdx, the BSS_INFO[0] by
			 * most default.
			 */
			prBssInfo = GET_BSS_INFO_BY_INDEX(
					prAdapter,
					prNetDevPrivate->ucBssIdx);

			if (!prBssInfo) {
				DBGLOG(REQ, ERROR,
					"RTWT Invalid BSS_INFO add\n");

				return -1;
			}

			DBGLOG(REQ, ERROR,
				"Single link MLD RTWT %d\n", au4Setting[14]);
		}

		DBGLOG(REQ, WARN, "BSS[%d] of RTWT target link ID %d\n",
			prBssInfo->ucBssIndex, au4Setting[14]);

		prTWTParams = &(rTWTCtrl.rTWTParams);
		kalMemSet(prTWTParams, 0, sizeof(struct _TWT_PARAMS_T));
		prTWTParams->fgReq = TRUE;
		prTWTParams->ucSetupCmd = (uint8_t) au4Setting[2];
		prTWTParams->fgTrigger = (au4Setting[3]) ? TRUE : FALSE;
		prTWTParams->fgUnannounced = (au4Setting[4]) ? TRUE : FALSE;
		prTWTParams->ucWakeIntvalExponent = (uint8_t) au4Setting[5];
		prTWTParams->fgProtect = (au4Setting[6]) ? TRUE : FALSE;
		prTWTParams->ucMinWakeDur = (uint8_t) au4Setting[7];
		prTWTParams->u2WakeIntvalMantiss = (uint16_t)au4Setting[8];
		prTWTParams->fgByPassNego = FALSE;
		prTWTParams->u2TWT = (uint16_t)au4Setting[9];
		prTWTParams->u8TWT = 0;
		prTWTParams->ucTrafficInfoPresent = (uint8_t) au4Setting[10];
		prTWTParams->ucDlUlBmpValid = (uint8_t) au4Setting[11];
		prTWTParams->ucDlBmp = (uint8_t) au4Setting[12];
		prTWTParams->ucUlBmp = (uint8_t) au4Setting[13];

		rTWTCtrl.ucBssIdx = prBssInfo->ucBssIndex;
		rTWTCtrl.ucCtrlAction = au4Setting[0];
		rTWTCtrl.ucTWTFlowId = au4Setting[1];
	} else if (
		IS_TWT_PARAM_ACTION_JOIN_RTWT(au4Setting[0])
		&& (i4Argc == CMD_TWT_ACTION_NINE_PARAMS)) {
		/*
		 * au4Setting[0]: RTWT Join command
		 * au4Setting[1]: RTWT Flow ID
		 * au4Setting[2]: RTWT Setup Command
		 * au4Setting[3]: RTWT traffic info present
		 * au4Setting[4]: RTWT DL/UL TID Bitmap Valid
		 * au4Setting[5]: RTWT DL TID Bitmap
		 * au4Setting[6]: RTWT UL TID Bitmap
		 * au4Setting[7]: RTWT target link ID
		 * , whereas au4Setting[7] = 0 also indicates single link MLD
		 * , since MLO link ID is assigned from AP MLD, for single link
		 * , single link MLD case, link ID 0 implicitly refers to
		 * prNetDevPrivate->ucBssIdx, the BSS_INFO[0] by most default.
		 */

		DBGLOG(REQ, INFO, "Action=%d\n", au4Setting[0]);
		DBGLOG(REQ, INFO, "RTWT Flow ID=%d\n", au4Setting[1]);
		DBGLOG(REQ, INFO, "Setup Command=%d\n", au4Setting[2]);
		DBGLOG(REQ, INFO, "RTWT traffic info=%d\n", au4Setting[3]);
		DBGLOG(REQ, INFO, "RTWT TID Bitmap Valid=%d\n", au4Setting[4]);
		DBGLOG(REQ, INFO, "RTWT DL TID Bitmap=%d\n", au4Setting[5]);
		DBGLOG(REQ, INFO, "RTWT UL TID Bitmap=%d\n", au4Setting[6]);
		DBGLOG(REQ, INFO, "RTWT target link ID=%d\n", au4Setting[7]);

		if (au4Setting[1] >= RTWT_MAX_FLOW_NUM) {
			/* Simple sanity check failure */
			DBGLOG(REQ, ERROR, "Invalid RTWT flow ID\n");

			return -1;
		}

		/* Get BSSINFO of ML setup link */
		prBssInfo = GET_BSS_INFO_BY_INDEX(
						prAdapter,
						prNetDevPrivate->ucBssIdx);

		if (!prBssInfo) {
			DBGLOG(REQ, ERROR, "RTWT Invalid BSS_INFO\n");

			return -1;
		}

		prMldBssInfo = mldBssGetByBss(prAdapter, prBssInfo);

		if (!prMldBssInfo) {
			DBGLOG(REQ, ERROR, "RTWT Invalid MLD_BSS_INFO\n");

			return -1;
		}

		prBssInfo = mldGetBssInfoByLinkID(
						prAdapter,
						prMldBssInfo,
						au4Setting[7],
						TRUE);

		if (!prBssInfo) {
			if (au4Setting[7] > 0) {
				DBGLOG(REQ, ERROR,
					"Find no RTWT target link %d\n",
					au4Setting[7]);

				return -1;
			}

			/*
			 * au4Setting[7]: RTWT target link ID
			 * , whereas au4Setting[7] = 0 also indicates
			 * single link MLD, since MLO link ID is assigned
			 * from AP MLD, for single link, single link MLD
			 * case, link ID 0 implicitly refers to
			 * prNetDevPrivate->ucBssIdx, the BSS_INFO[0] by
			 * most default.
			 */
			prBssInfo = GET_BSS_INFO_BY_INDEX(
						prAdapter,
						prNetDevPrivate->ucBssIdx);

			if (!prBssInfo) {
				DBGLOG(REQ, ERROR,
					"RTWT Invalid BSS_INFO join\n");

				return -1;
			}

			DBGLOG(REQ, INFO,
				"Single link MLD RTWT %d\n", au4Setting[7]);
		}

		DBGLOG(REQ, INFO, "BSS[%d] of RTWT target link ID %d\n",
			prBssInfo->ucBssIndex, au4Setting[7]);

		/*
		 * We only need these, as to the left parameters should
		 * be picked up from beacon IE
		 */
		prTWTParams = &(rTWTCtrl.rTWTParams);
		kalMemSet(prTWTParams, 0, sizeof(struct _TWT_PARAMS_T));
		prTWTParams->fgReq = TRUE;
		prTWTParams->ucSetupCmd = (uint8_t) au4Setting[2];
		prTWTParams->fgByPassNego = FALSE;
		prTWTParams->u2TWT = 0;
		prTWTParams->u8TWT = 0;
		prTWTParams->ucTrafficInfoPresent = (uint8_t) au4Setting[3];
		prTWTParams->ucDlUlBmpValid = (uint8_t) au4Setting[4];
		prTWTParams->ucDlBmp = (uint8_t) au4Setting[5];
		prTWTParams->ucUlBmp = (uint8_t) au4Setting[6];

		rTWTCtrl.ucBssIdx = prBssInfo->ucBssIndex;
		rTWTCtrl.ucCtrlAction = (uint8_t)au4Setting[0];
		rTWTCtrl.ucTWTFlowId = (uint8_t)au4Setting[1];
	} else if (IS_TWT_PARAM_ACTION_DEL(au4Setting[0])
		&& (i4Argc == CMD_TWT_ACTION_FIVE_PARAMS)) {
		/*
		 * au4Setting[0]: RTWT teardown command
		 * au4Setting[1]: RTWT Flow ID
		 * au4Setting[2]: RTWT teardown all = 0|1
		 * au4Setting[3]: RTWT target link ID
		 * , whereas au4Setting[3] = 0 also indicates single link MLD
		 * , since MLO link ID is assigned from AP MLD, for single link
		 * , single link MLD case, link ID 0 implicitly refers to
		 * prNetDevPrivate->ucBssIdx, the BSS_INFO[0] by most default.
		 */
		DBGLOG(REQ, INFO, "Action=%d\n", au4Setting[0]);
		DBGLOG(REQ, INFO, "RTWT Flow ID=%d\n", au4Setting[1]);
		DBGLOG(REQ, INFO, "Teardown all=%d\n", au4Setting[2]);
		DBGLOG(REQ, INFO, "RTWT target link ID=%d\n", au4Setting[3]);

		if (au4Setting[1] >= RTWT_MAX_FLOW_NUM) {
			/* Simple sanity check failure */
			DBGLOG(REQ, ERROR, "Invalid RTWT Params\n");

			return -1;
		}

		/* Get BSSINFO of ML setup link */
		prBssInfo = GET_BSS_INFO_BY_INDEX(
						prAdapter,
						prNetDevPrivate->ucBssIdx);

		if (!prBssInfo) {
			DBGLOG(REQ, ERROR, "RTWT Invalid BSS_INFO\n");

			return -1;
		}

		prMldBssInfo = mldBssGetByBss(prAdapter, prBssInfo);

		if (!prMldBssInfo) {
			DBGLOG(REQ, ERROR, "RTWT Invalid MLD_BSS_INFO\n");

			return -1;
		}

		prBssInfo = mldGetBssInfoByLinkID(
						prAdapter,
						prMldBssInfo,
						au4Setting[3],
						TRUE);

		if (!prBssInfo) {
			if (au4Setting[3] > 0) {
				DBGLOG(REQ, ERROR,
					"Find no RTWT target link %d\n",
					au4Setting[3]);

				return -1;
			}

			/*
			 * au4Setting[3]: RTWT target link ID
			 * , whereas au4Setting[3] = 0 also indicates
			 * single link MLD, since MLO link ID is assigned
			 * from AP MLD, for single link, single link MLD
			 * case, link ID 0 implicitly refers to
			 * prNetDevPrivate->ucBssIdx, the BSS_INFO[0] by
			 * most default.
			 */
			prBssInfo = GET_BSS_INFO_BY_INDEX(
						prAdapter,
						prNetDevPrivate->ucBssIdx);

			if (!prBssInfo) {
				DBGLOG(REQ, ERROR,
					"RTWT Invalid BSS_INFO teardown\n");

				return -1;
			}

			DBGLOG(REQ, INFO,
				"Single link MLD RTWT %d\n", au4Setting[3]);
		}

		DBGLOG(REQ, INFO, "BSS[%d] of RTWT target link ID %d\n",
			prBssInfo->ucBssIndex, au4Setting[3]);

		rTWTCtrl.ucBssIdx = prBssInfo->ucBssIndex;
		rTWTCtrl.ucCtrlAction = (uint8_t)au4Setting[0];
		rTWTCtrl.ucTWTFlowId = (uint8_t)au4Setting[1];
		rTWTCtrl.fgTeardownAll = (u_int8_t)au4Setting[2];
	}
#endif
	else if ((IS_TWT_PARAM_ACTION_ENABLE_ITWT(au4Setting[0]) ||
			IS_TWT_PARAM_ACTION_ENABLE_INF_FRAME(au4Setting[0]))
			&& (i4Argc == CMD_TWT_ACTION_THREE_PARAMS)) {
		DBGLOG(REQ, INFO, "iTWT=%d\n", au4Setting[0]);
		DBGLOG(REQ, INFO, "en=%d\n", au4Setting[1]);

		prWifiVar->ucTWTRequester = (uint8_t)au4Setting[1];

		return 0;
	} else if (IS_TWT_PARAM_ACTION_ENABLE_BTWT(au4Setting[0])
			&& (i4Argc == CMD_TWT_ACTION_THREE_PARAMS)) {
		DBGLOG(REQ, INFO, "BTWT=%d\n", au4Setting[0]);
		DBGLOG(REQ, INFO, "en=%d\n", au4Setting[1]);

#if (CFG_SUPPORT_BTWT == 1)
		prWifiVar->ucBTWTSupport = (uint8_t)au4Setting[1];
#endif

		return 0;
	} else if (IS_TWT_PARAM_ACTION_ENABLE_RTWT(au4Setting[0])
			&& (i4Argc == CMD_TWT_ACTION_THREE_PARAMS)) {
		DBGLOG(REQ, INFO, "RTWT=%d\n", au4Setting[0]);
		DBGLOG(REQ, INFO, "en=%d\n", au4Setting[1]);

#if (CFG_SUPPORT_RTWT == 1)
		prWifiVar->ucRTWTSupport = (uint8_t)au4Setting[1];
#endif

		return 0;
	} else if ((i4Argc == CMD_TWT_ACTION_TEN_PARAMS) &&
			(IS_TWT_PARAM_ACTION_ADD_BYPASS(au4Setting[0]) ||
			IS_TWT_PARAM_ACTION_ADD(au4Setting[0]))) {
		DBGLOG(REQ, INFO, "Action bitmap=%d\n", au4Setting[0]);
		DBGLOG(REQ, INFO,
			"TWT Flow ID=%d Setup Command=%d Trig enabled=%d\n",
			au4Setting[1], au4Setting[2], au4Setting[3]);
		DBGLOG(REQ, INFO,
			"Unannounced enabled=%d Wake Interval Exponent=%d\n",
			au4Setting[4], au4Setting[5]);
		DBGLOG(REQ, INFO, "Protection enabled=%d Duration=%d\n",
			au4Setting[6], au4Setting[7]);
		DBGLOG(REQ, INFO, "Wake Interval Mantissa=%d\n", au4Setting[8]);
		/*
		 *  au2Setting[0]: Whether bypassing nego or not
		 *  au2Setting[1]: TWT Flow ID
		 *  au2Setting[2]: TWT Setup Command
		 *  au2Setting[3]: Trigger enabled
		 *  au2Setting[4]: Unannounced enabled
		 *  au2Setting[5]: TWT Wake Interval Exponent
		 *  au2Setting[6]: TWT Protection enabled
		 *  au2Setting[7]: Nominal Minimum TWT Wake Duration
		 *  au2Setting[8]: TWT Wake Interval Mantissa
		 */
		if (au4Setting[1] >= TWT_MAX_FLOW_NUM ||
			au4Setting[2] > TWT_SETUP_CMD_ID_DEMAND ||
			au4Setting[5] > TWT_MAX_WAKE_INTVAL_EXP) {
			/* Simple sanity check failure */
			DBGLOG(REQ, ERROR, "Invalid TWT Params\n");
			return -1;
		}

		prTWTParams = &(rTWTCtrl.rTWTParams);
		kalMemSet(prTWTParams, 0, sizeof(struct _TWT_PARAMS_T));
		prTWTParams->fgReq = TRUE;
		prTWTParams->ucSetupCmd = (uint8_t) au4Setting[2];
		prTWTParams->fgTrigger = (au4Setting[3]) ? TRUE : FALSE;
		prTWTParams->fgUnannounced = (au4Setting[4]) ? TRUE : FALSE;
		prTWTParams->ucWakeIntvalExponent = (uint8_t) au4Setting[5];
		prTWTParams->fgProtect = (au4Setting[6]) ? TRUE : FALSE;
		prTWTParams->ucMinWakeDur = (uint8_t) au4Setting[7];
		prTWTParams->u2WakeIntvalMantiss = au4Setting[8];
		prTWTParams->fgByPassNego =
			IS_TWT_PARAM_ACTION_ADD_BYPASS(au4Setting[0])
				? TRUE : FALSE;

		rTWTCtrl.ucBssIdx = prNetDevPrivate->ucBssIdx;
		rTWTCtrl.ucCtrlAction = au4Setting[0];
		rTWTCtrl.ucTWTFlowId = au4Setting[1];
	}
#ifdef CFG_SUPPORT_TWT_EXT
	else if ((i4Argc == CMD_TWT_ACTION_FIFTEEN_PARAMS) &&
		(IS_TWT_PARAM_ACTION_ADD(au4Setting[0]))) {
		DBGLOG(REQ, INFO, "Action bitmap=%d\n", au4Setting[0]);
		DBGLOG(REQ, INFO,
			"TWT Flow ID=%d Setup Command=%d Trig enabled=%d\n",
			au4Setting[1], au4Setting[2], au4Setting[3]);
		DBGLOG(REQ, INFO,
			"Unannounced enabled=%d Wake Interval Exponent=%d\n",
			au4Setting[4], au4Setting[5]);
		DBGLOG(REQ, INFO, "Protection enabled=%d Duration=%d\n",
			au4Setting[6], au4Setting[7]);
		DBGLOG(REQ, INFO, "Wake Interval Mantissa=%d\n", au4Setting[8]);
		DBGLOG(REQ, INFO, "Desired wake time=%d\n", au4Setting[9]);
		/*
		*  au2Setting[0]: Whether bypassing nego or not
		*  au2Setting[1]: TWT Flow ID
		*  au2Setting[2]: TWT Setup Command
		*  au2Setting[3]: Trigger enabled
		*  au2Setting[4]: Unannounced enabled
		*  au2Setting[5]: TWT Wake Interval Exponent
		*  au2Setting[6]: TWT Protection enabled
		*  au2Setting[7]: Nominal Minimum TWT Wake Duration
		*  au2Setting[8]: TWT Wake Interval Mantissa
		*  au2Setting[9]: Desired wake time:
		*                 This specify when the first TWT starts
		*/
		if (au4Setting[1] >= TWT_MAX_FLOW_NUM ||
			au4Setting[2] > TWT_SETUP_CMD_ID_DEMAND ||
			au4Setting[5] > TWT_MAX_WAKE_INTVAL_EXP) {
			/* Simple sanity check failure */
			DBGLOG(REQ, INFO, "Invalid TWT Params\n");
			return -1;
		}

		prTWTParams = &(rTWTCtrl.rTWTParams);
		kalMemSet(prTWTParams, 0, sizeof(struct _TWT_PARAMS_T));
		prTWTParams->fgReq = TRUE;
		prTWTParams->ucSetupCmd = (uint8_t) au4Setting[2];
		prTWTParams->fgTrigger = (au4Setting[3]) ? TRUE : FALSE;
		prTWTParams->fgUnannounced = (au4Setting[4]) ? TRUE : FALSE;
		prTWTParams->ucWakeIntvalExponent = (uint8_t) au4Setting[5];
		prTWTParams->fgProtect = (au4Setting[6]) ? TRUE : FALSE;
		prTWTParams->ucMinWakeDur = (uint8_t) au4Setting[7];
		prTWTParams->u2WakeIntvalMantiss = au4Setting[8];
		prTWTParams->u4DesiredWakeTime = au4Setting[9];
		prTWTParams->u4WakeIntvalMin = au4Setting[10];
		prTWTParams->u4WakeIntvalMax = au4Setting[11];
		prTWTParams->ucWakeDurMin = (uint8_t) au4Setting[12];
		prTWTParams->ucWakeDurMax = (uint8_t) au4Setting[13];

		rTWTCtrl.ucBssIdx = prNetDevPrivate->ucBssIdx;
		rTWTCtrl.ucCtrlAction = au4Setting[0];
		rTWTCtrl.ucTWTFlowId = au4Setting[1];
	}
#endif
	else {
		DBGLOG(REQ, INFO, "wrong argc for update agrt: %d\n", i4Argc);

		return -1;
	}

	prTWTParamSetMsg = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
		sizeof(struct _MSG_TWT_REQFSM_RESUME_T));
	if (prTWTParamSetMsg) {
		prTWTParamSetMsg->rMsgHdr.eMsgId =
			MID_TWT_PARAMS_SET;
		kalMemCopy(&prTWTParamSetMsg->rTWTCtrl,
			&rTWTCtrl, sizeof(rTWTCtrl));

		mboxSendMsg(prAdapter, MBOX_ID_0,
			(struct MSG_HDR *) prTWTParamSetMsg,
			MSG_SEND_METHOD_BUF);
	} else
		return -1;

	return 0;
}
#endif

#if (CFG_SUPPORT_802_11AX == 1)
int priv_driver_set_smpsparams(
	struct net_device *prNetDev,
	char *pcCommand,
	int i4TotalLen)
{
	struct ADAPTER *prAdapter = NULL;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX_LONG] = { 0 };
	struct _SMPS_CTRL_T rSMPSCtrl = { 0x00 };
	struct _SMPS_PARAMS_T *prSMPSParams = NULL;
	uint16_t i;
	int32_t u4Ret = 0;
	uint32_t au4Setting[CMD_SMPS_MAX_PARAMS] = { 0 };
	struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPrivate = NULL;
	struct _MSG_SMPS_PARAMS_SET_T *prSMPSParamSetMsg;

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prNetDevPrivate =
		(struct NETDEV_PRIVATE_GLUE_INFO *) netdev_priv(prNetDev);

	DBGLOG(REQ, INFO, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, INFO, "argc is %i\n", i4Argc);

	prAdapter = prNetDevPrivate->prGlueInfo->prAdapter;

	/* Check param number and convert SMPS params to integer type */
	if (i4Argc == CMD_SMPS_ACTION_FOUR_PARAMS) {
		for (i = 0; i < (i4Argc - 1); i++) {

			u4Ret = kalkStrtou32(apcArgv[i + 1],
				0, &(au4Setting[i]));

			if (u4Ret)
				DBGLOG(REQ, INFO, "Argv error ret=%d\n", u4Ret);
		}
	} else if (i4Argc == CMD_SMPS_ACTION_THREE_PARAMS) {
		for (i = 0; i < (i4Argc - 1); i++) {

			u4Ret = kalkStrtou32(apcArgv[i + 1],
				0, &(au4Setting[i]));

			if (u4Ret)
				DBGLOG(REQ, INFO, "Argv error ret=%d\n", u4Ret);
		}
	} else if (i4Argc == CMD_SMPS_ACTION_TWO_PARAMS) {
		for (i = 0; i < (i4Argc - 1); i++) {

			u4Ret = kalkStrtou32(apcArgv[i + 1],
				0, &(au4Setting[i]));

			if (u4Ret)
				DBGLOG(REQ, INFO, "Argv error ret=%d\n", u4Ret);
		}
	} else {
		DBGLOG(REQ, INFO, "set_smpsparams wrong argc : %d\n", i4Argc);
		return -1;
	}

	if ((i4Argc == CMD_SMPS_ACTION_FOUR_PARAMS) ||
			(i4Argc == CMD_SMPS_ACTION_THREE_PARAMS)) {
		prSMPSParams = &(rSMPSCtrl.rSMPSParams);
		kalMemSet(prSMPSParams, 0, sizeof(struct _SMPS_PARAMS_T));
		prSMPSParams->ucHtHeCap = (uint8_t) au4Setting[1];
		rSMPSCtrl.ucBssIdx = prNetDevPrivate->ucBssIdx;
		rSMPSCtrl.ucCtrlAction = au4Setting[0];
		if (au4Setting[0] == 0)
			g_fgHTSMPSEnabled =  (uint8_t) au4Setting[1];
	} else if (i4Argc == CMD_SMPS_ACTION_TWO_PARAMS) {
		if (au4Setting[0]) {
			/* HE Dynamic SMPS is enabled */
			prAdapter->rWifiVar.ucHeDynamicSMPS = TRUE;
		} else {
			/* HE Dynamic SMPS is disabled */
			prAdapter->rWifiVar.ucHeDynamicSMPS = FALSE;
		}

		DBGLOG(REQ, STATE, "HE Dynamic SMPS is %d\n",
				prAdapter->rWifiVar.ucHeDynamicSMPS);
	} else {
		DBGLOG(REQ, INFO, "wrong argc for update agrt: %d\n", i4Argc);
		return -1;
	}

	prSMPSParamSetMsg = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
		sizeof(struct _MSG_SMPS_PARAMS_SET_T));

	if (prSMPSParamSetMsg) {

		prSMPSParamSetMsg->rMsgHdr.eMsgId =
			MID_SMPS_ACTION_SET;
		kalMemCopy(&prSMPSParamSetMsg->rSMPSCtrl,
			&rSMPSCtrl, sizeof(rSMPSCtrl));

		mboxSendMsg(prAdapter, MBOX_ID_0,
			(struct MSG_HDR *) prSMPSParamSetMsg,
			MSG_SEND_METHOD_BUF);
	} else
		return -1;

	return 0;
}
#endif

#if (CFG_SUPPORT_802_11AX == 1)
int priv_driver_set_pad_dur(struct net_device *prNetDev, char *pcCommand,
			 int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret, u4Parse = 0;
	uint8_t u1HePadDur;

	ASSERT(prNetDev);

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc == 2) {
		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Parse);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
			       u4Ret);

		u1HePadDur = (uint8_t) u4Parse;

		if (u1HePadDur == 0 || u1HePadDur == 8 ||
			u1HePadDur == 16) {
			prGlueInfo->prAdapter->rWifiVar.ucTrigMacPadDur
				= u1HePadDur/8;
		} else {
			DBGLOG(INIT, ERROR,
				"iwpriv wlanXX driver SET_PAD_DUR <0,1,2>\n");
		}
	} else {
		DBGLOG(INIT, ERROR,
			"iwpriv wlanXX driver SET_PAD_DUR <0,1,2>\n");
	}

	return i4BytesWritten;
}
#endif


int priv_driver_get_wifi_type(struct net_device *prNetDev,
				     char *pcCommand, int i4TotalLen)
{
	struct PARAM_GET_WIFI_TYPE rParamGetWifiType;
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus;
	uint32_t u4BytesWritten = 0;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE) {
		DBGLOG(REQ, ERROR, "GLUE_CHK_PR2 fail\n");
		return -1;
	}

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	rParamGetWifiType.prNetDev = prNetDev;
	rStatus = kalIoctl(prGlueInfo, wlanoidGetWifiType,
			   (void *)&rParamGetWifiType,
			   sizeof(struct PARAM_GET_WIFI_TYPE),
			   &u4BytesWritten);
	if (rStatus == WLAN_STATUS_SUCCESS) {
		if (u4BytesWritten > 0) {
			if (u4BytesWritten > i4TotalLen)
				u4BytesWritten = i4TotalLen;
			kalMemCopy(pcCommand, rParamGetWifiType.arWifiTypeName,
				   u4BytesWritten);
		}
	} else {
		DBGLOG(REQ, ERROR, "rStatus=%x\n", rStatus);
		u4BytesWritten = 0;
	}

	return (int)u4BytesWritten;
}

#if (CFG_WIFI_GET_MCS_INFO == 1)
static int32_t priv_driver_last_sec_mcs_info(struct ADAPTER *prAdapter,
			char *pcCommand, int i4TotalLen,
			struct PARAM_TX_MCS_INFO *prTxMcsInfo)
{
	uint8_t ucBssIdx = 0, i = 0, j = 0, ucCnt = 0, ucPerSum = 0;
#if (CFG_SUPPORT_CONNAC2X == 1)
	uint8_t dcm = 0, ersu106t = 0;
#endif
	uint16_t u2RateCode = 0;
	uint32_t au4RxV0[MCS_INFO_SAMPLE_CNT], au4RxV1[MCS_INFO_SAMPLE_CNT],
		au4RxV2[MCS_INFO_SAMPLE_CNT];
	uint32_t txmode = 0, rate = 0, frmode = 0, sgi = 0,
		nsts = 0, ldpc = 0, stbc = 0, groupid = 0, mu = 0, bw = 0;
	uint32_t u4RxV0 = 0, u4RxV1 = 0, u4RxV2 = 0;
	int32_t i4BytesWritten = 0;
	struct STA_RECORD *prStaRec = NULL;
	struct CHIP_DBG_OPS *prChipDbg;

	ucBssIdx = GET_IOCTL_BSSIDX(prAdapter);
	prStaRec = aisGetTargetStaRec(prAdapter, ucBssIdx);

	if (prStaRec != NULL && prStaRec->fgIsValid && prStaRec->fgIsInUse) {
		kalMemCopy(au4RxV0, prStaRec->au4RxV0, sizeof(au4RxV0));
		kalMemCopy(au4RxV1, prStaRec->au4RxV1, sizeof(au4RxV1));
		kalMemCopy(au4RxV2, prStaRec->au4RxV2, sizeof(au4RxV2));
	} else {
		i4BytesWritten += kalScnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"Not Connect to AP\n");
		return i4BytesWritten;
	}


	/* Output the TX MCS Info */
	i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten, "\nTx MCS:\n");

	for (i = 0; i < MCS_INFO_SAMPLE_CNT; i++) {
		if (prTxMcsInfo->au2TxRateCode[i] == 0xFFFF)
			continue;

		txmode = HW_TX_RATE_TO_MODE(prTxMcsInfo->au2TxRateCode[i]);
		if (txmode >= ENUM_TX_MODE_NUM)
			txmode = ENUM_TX_MODE_NUM - 1;

		rate = HW_TX_RATE_TO_MCS(prTxMcsInfo->au2TxRateCode[i]);
		nsts = HW_TX_RATE_TO_NSS(prTxMcsInfo->au2TxRateCode[i]) + 1;
		stbc = HW_TX_RATE_TO_STBC(prTxMcsInfo->au2TxRateCode[i]);
		bw = prTxMcsInfo->aucTxBw[i];
		sgi = prTxMcsInfo->aucTxSgi[i];
		ldpc = prTxMcsInfo->aucTxLdpc[i];

#if (CFG_SUPPORT_CONNAC2X == 1)
		dcm = HW_TX_RATE_TO_DCM(prTxMcsInfo->au2TxRateCode[i]);
		ersu106t = HW_TX_RATE_TO_106T(prTxMcsInfo->au2TxRateCode[i]);

		if (dcm)
			rate = CONNAC2X_HW_TX_RATE_UNMASK_DCM(rate);
		if (ersu106t)
			rate = CONNAC2X_HW_TX_RATE_UNMASK_106T(rate);
#endif

		ucCnt = 0;
		ucPerSum = 0;
		u2RateCode = prTxMcsInfo->au2TxRateCode[i];
		for (j = 0; j < MCS_INFO_SAMPLE_CNT; j++) {
			if (u2RateCode == prTxMcsInfo->au2TxRateCode[j]) {
				ucPerSum += prTxMcsInfo->aucTxRatePer[j];
				ucCnt++;
				prTxMcsInfo->au2TxRateCode[j] = 0xFFFF;
				prTxMcsInfo->aucTxBw[j] = 0xFF;
				prTxMcsInfo->aucTxSgi[j] = 0xFF;
				prTxMcsInfo->aucTxLdpc[j] = 0xFF;
			}
		}

		if (txmode == TX_RATE_MODE_CCK)
			i4BytesWritten += kalScnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"%s, ", HW_TX_RATE_CCK_STR[rate & 0x3]);
		else if (txmode == TX_RATE_MODE_OFDM)
			i4BytesWritten += kalScnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"%s, ", nicHwRateOfdmStr(rate));
		else if ((txmode == TX_RATE_MODE_HTMIX) ||
			 (txmode == TX_RATE_MODE_HTGF))
			i4BytesWritten += kalScnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"MCS%d, ", rate);
		else
			i4BytesWritten += kalScnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"NSS%d_MCS%d, ", nsts, rate);

		i4BytesWritten += kalScnprintf(
		    pcCommand + i4BytesWritten,
		    i4TotalLen - i4BytesWritten,
		    "%s, ", (bw < 4) ? HW_TX_RATE_BW[bw] : HW_TX_RATE_BW[4]);

		if (txmode == TX_RATE_MODE_CCK)
			i4BytesWritten += kalScnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"%s, ", (rate < 4) ? "LP" : "SP");
		else if (txmode == TX_RATE_MODE_OFDM)
			;
		else if ((txmode == TX_RATE_MODE_HTMIX) ||
				(txmode == TX_RATE_MODE_HTGF) ||
				(txmode == TX_RATE_MODE_VHT) ||
				(txmode == TX_RATE_MODE_PLR))
			i4BytesWritten += kalScnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"%s, ", (sgi == 0) ? "LGI" : "SGI");
		else
			i4BytesWritten += kalScnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"%s, ",
				(sgi == 0) ?
				"SGI" : (sgi == 1 ? "MGI" : "LGI"));

		i4BytesWritten += kalScnprintf(
			pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"%s%s%s%s%s [PER: %02d%%]\t",
			(txmode <= ENUM_TX_MODE_NUM) ?
			HW_TX_MODE_STR[txmode] : "N/A",
#if (CFG_SUPPORT_CONNAC2X == 1)
			dcm ? ", DCM" : "", ersu106t ? ", 106t" : "",
#else
			"", "",
#endif
			stbc ? ", STBC, " : ", ",
			((ldpc == 0) ||
				(txmode == TX_RATE_MODE_CCK) ||
				(txmode == TX_RATE_MODE_OFDM)) ?
			"BCC" : "LDPC", ucPerSum/ucCnt);

		for (j = 0; j < ucCnt; j++)
			i4BytesWritten += kalScnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten, "*");

		i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten, "\n");

	}


	/* Output the RX MCS info */
	i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten, "\nRx MCS:\n");

	/* connac2x_show_rx_rate_info() */
	prChipDbg = prAdapter->chip_info->prDebugOps;

	for (i = 0; i < MCS_INFO_SAMPLE_CNT; i++) {
		if (au4RxV0[i] == 0xFFFFFFFF)
			continue;

		u4RxV0 = au4RxV0[i];
		u4RxV1 = au4RxV1[i];
		u4RxV2 = au4RxV2[i];

		if (prChipDbg &&
			prChipDbg->show_rx_rate_info ==
			connac2x_show_rx_rate_info) {

			if (CONNAC2X_RX_DBG_INFO_GRP3(prAdapter)) {
				/* Rx DBG info from group 3 */
				goto GET_MCS_INFO_BIT_MASK_G3;
			} else {
				/* Rx DBG info from group 3 and group 5 */
				goto GET_MCS_INFO_BIT_MASK_G3_G5;
			}

		} else {
			goto GET_MCS_INFO_BIT_MASK_LEGACY;
		}


GET_MCS_INFO_BIT_MASK_G3:
		/* u4RxV0: Group3 PRXV0[0:31] */
		/* P-RXV0 */
		rate = (u4RxV0 & CONNAC2X_RX_VT_RX_RATE_MASK)
				>> CONNAC2X_RX_VT_RX_RATE_OFFSET;
		nsts = ((u4RxV0 & CONNAC2X_RX_VT_NSTS_MASK)
				>> CONNAC2X_RX_VT_NSTS_OFFSET);
		ldpc = u4RxV0 & CONNAC2X_RX_VT_LDPC;
		frmode =
			(u4RxV0 & CONNAC2X_RX_VT_FR_MODE_MASK_V2)
				>> CONNAC2X_RX_VT_FR_MODE_OFFSET_V2;
		sgi = (u4RxV0 & CONNAC2X_RX_VT_SHORT_GI_MASK_V2)
				>> CONNAC2X_RX_VT_SHORT_GI_OFFSET_V2;
		stbc = (u4RxV0 & CONNAC2X_RX_VT_STBC_MASK_V2)
				>> CONNAC2X_RX_VT_STBC_OFFSET_V2;
		txmode =
			(u4RxV0 & CONNAC2X_RX_VT_RX_MODE_MASK_V2)
				>> CONNAC2X_RX_VT_RX_MODE_OFFSET_V2;
		mu = (u4RxV0 & CONNAC2X_RX_VT_MU);
		dcm = (u4RxV0 & CONNAC2X_RX_VT_DCM);

		if (mu == 0)
			nsts += 1;

		goto GET_MCS_INFO_OUTPUT_RX;


GET_MCS_INFO_BIT_MASK_G3_G5:
		/* u4RxV0: Group3 PRXV0[0:31] */
		/* u4RxV1: Group5 C-B-0[0:31] */
		/* u4RxV2: Group5 C-B-1[0:31] */

		/* P-RXV0 */
		rate = (u4RxV0 & CONNAC2X_RX_VT_RX_RATE_MASK)
				>> CONNAC2X_RX_VT_RX_RATE_OFFSET;
		nsts = ((u4RxV0 & CONNAC2X_RX_VT_NSTS_MASK)
				>> CONNAC2X_RX_VT_NSTS_OFFSET);
		ldpc = u4RxV0 & CONNAC2X_RX_VT_LDPC;

		/* C-B-0 */
		stbc = (u4RxV1 & CONNAC2X_RX_VT_STBC_MASK)
				>> CONNAC2X_RX_VT_STBC_OFFSET;
		txmode =
			(u4RxV1 & CONNAC2X_RX_VT_RX_MODE_MASK)
				>> CONNAC2X_RX_VT_RX_MODE_OFFSET;
		frmode =
			(u4RxV1 & CONNAC2X_RX_VT_FR_MODE_MASK)
				>> CONNAC2X_RX_VT_FR_MODE_OFFSET;
		sgi = (u4RxV1 & CONNAC2X_RX_VT_SHORT_GI_MASK)
				>> CONNAC2X_RX_VT_SHORT_GI_OFFSET;
		/* C-B-1 */
		groupid =
			(u4RxV2 & CONNAC2X_RX_VT_GROUP_ID_MASK)
				>> CONNAC2X_RX_VT_GROUP_ID_OFFSET;

		if (groupid && groupid != 63) {
			mu = 1;
		} else {
			mu = 0;
			nsts += 1;
		}

		goto GET_MCS_INFO_OUTPUT_RX;


GET_MCS_INFO_BIT_MASK_LEGACY:
		/* 1st Cycle RX_VT_RX_MODE : bit 12 - 14 */
		txmode = (u4RxV0 & RX_VT_RX_MODE_MASK)
			>> RX_VT_RX_MODE_OFFSET;

		/* 1st Cycle RX_VT_RX_RATE: bit 0 - 6 */
		rate = (u4RxV0 & RX_VT_RX_RATE_MASK)
			>> RX_VT_RX_RATE_OFFSET;

		/* 1st Cycle RX_VT_RX_FR_MODE: bit 15 - 16 */
		frmode = (u4RxV0 & RX_VT_FR_MODE_MASK)
			>> RX_VT_FR_MODE_OFFSET;

		/* 2nd Cycle RX_VT_NSTS: bit 27 - 29 */
		nsts = ((u4RxV1 & RX_VT_NSTS_MASK)
			>> RX_VT_NSTS_OFFSET);

		/* 1st Cycle RX_VT_STBC: bit 7 - 8 */
		stbc = (u4RxV0 & RX_VT_STBC_MASK)
			>> RX_VT_STBC_OFFSET;

		/* 1st Cycle RX_VT_SHORT_GI: bit 19 */
		sgi = u4RxV0 & RX_VT_SHORT_GI;

		/* 1st Cycle RX_VT_LDPC: bit 9 */
		ldpc = u4RxV0 & RX_VT_LDPC;

		/* 2nd Cycle RX_VT_GROUP_ID_MASK: bit 21 - 26 */
		groupid =
			(u4RxV1 & RX_VT_GROUP_ID_MASK)
				>> RX_VT_GROUP_ID_OFFSET;

		if (groupid && groupid != 63) {
			mu = 1;
		} else {
			mu = 0;
			nsts += 1;
		}

		goto GET_MCS_INFO_OUTPUT_RX;


GET_MCS_INFO_OUTPUT_RX:

		/* Distribution Calculation clear the same sample content */
		ucCnt = 0;
		for (j = 0; j < MCS_INFO_SAMPLE_CNT; j++) {
			if ((u4RxV0 & RX_MCS_INFO_MASK) ==
			    (au4RxV0[j] & RX_MCS_INFO_MASK)) {
				au4RxV0[j] = 0xFFFFFFFF;
				ucCnt++;
			}
		}

		if (prChipDbg &&
			prChipDbg->show_rx_rate_info ==
				connac2x_show_rx_rate_info) {

			if (txmode == TX_RATE_MODE_CCK)
				i4BytesWritten +=
					kalScnprintf(pcCommand + i4BytesWritten,
						i4TotalLen - i4BytesWritten,
						"%s, ", (rate < 4) ?
						HW_TX_RATE_CCK_STR[rate] :
						((rate < 8) ?
						HW_TX_RATE_CCK_STR[rate - 4] :
						HW_TX_RATE_CCK_STR[4]));
			else if (txmode == TX_RATE_MODE_OFDM)
				i4BytesWritten +=
					kalScnprintf(pcCommand + i4BytesWritten,
						i4TotalLen - i4BytesWritten,
						"%s, ",
						nicHwRateOfdmStr(rate));
			else if ((txmode == TX_RATE_MODE_HTMIX) ||
				 (txmode == TX_RATE_MODE_HTGF))
				i4BytesWritten +=
					kalScnprintf(pcCommand + i4BytesWritten,
						i4TotalLen - i4BytesWritten,
						"MCS%d, ", rate);
			else
				i4BytesWritten +=
					kalScnprintf(pcCommand + i4BytesWritten,
						i4TotalLen - i4BytesWritten,
						"%s%d_MCS%d, ",
						(stbc == 1) ? "NSTS" : "NSS",
						nsts, rate);

		} else {

			if (txmode == TX_RATE_MODE_CCK)
				i4BytesWritten += kalScnprintf(
					pcCommand + i4BytesWritten,
					i4TotalLen - i4BytesWritten, "%s, ",
					(rate < 4) ?
						HW_TX_RATE_CCK_STR[rate] :
						HW_TX_RATE_CCK_STR[4]);
			else if (txmode == TX_RATE_MODE_OFDM)
				i4BytesWritten += kalScnprintf(
					pcCommand + i4BytesWritten,
					i4TotalLen - i4BytesWritten, "%s, ",
					nicHwRateOfdmStr(rate));
			else if ((txmode == TX_RATE_MODE_HTMIX) ||
				 (txmode == TX_RATE_MODE_HTGF))
				i4BytesWritten += kalScnprintf(
					pcCommand + i4BytesWritten,
					i4TotalLen - i4BytesWritten,
					"MCS%d, ", rate);
			else
				i4BytesWritten += kalScnprintf(
					pcCommand + i4BytesWritten,
					i4TotalLen - i4BytesWritten,
					"NSS%d_MCS%d, ",
					nsts, rate);
		}

		i4BytesWritten +=
			kalScnprintf(pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten, "%s, ",
				(frmode < 4) ?
				HW_TX_RATE_BW[frmode] : HW_TX_RATE_BW[4]);

		if (txmode == TX_RATE_MODE_CCK)
			i4BytesWritten += kalScnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten, "%s, ",
				(rate < 4) ? "LP" : "SP");
		else if (txmode == TX_RATE_MODE_OFDM)
			;
		else if (txmode == TX_RATE_MODE_HTMIX ||
			 txmode == TX_RATE_MODE_HTGF ||
			 txmode == TX_RATE_MODE_VHT)
			i4BytesWritten += kalScnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten, "%s, ",
				(sgi == 0) ? "LGI" : "SGI");
		else
			i4BytesWritten +=
				kalScnprintf(pcCommand + i4BytesWritten,
					i4TotalLen - i4BytesWritten, "%s, ",
					(sgi == 0) ? "SGI" :
						(sgi == 1 ? "MGI" : "LGI"));

		i4BytesWritten +=
			kalScnprintf(pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten, "%s",
				(stbc == 0) ? "" : "STBC, ");

		if (prChipDbg &&
			prChipDbg->show_rx_rate_info ==
			connac2x_show_rx_rate_info) {

			if (CONNAC2X_RX_DBG_INFO_GRP3(prAdapter))
				i4BytesWritten +=
					kalScnprintf(pcCommand + i4BytesWritten,
						i4TotalLen - i4BytesWritten,
						"%s",
						(dcm == 0) ? "" : "DCM, ");

			if (mu) {
				if (CONNAC2X_RX_DBG_INFO_GRP3(prAdapter))
					i4BytesWritten += kalScnprintf(
						pcCommand + i4BytesWritten,
						i4TotalLen - i4BytesWritten,
						"%s, %s, %s\t",
						(txmode < ENUM_TX_MODE_NUM) ?
							HW_TX_MODE_STR[txmode] :
							"N/A",
						(ldpc == 0) ? "BCC" : "LDPC",
						"MU");
				else
					i4BytesWritten += kalScnprintf(
						pcCommand + i4BytesWritten,
						i4TotalLen - i4BytesWritten,
						"%s, %s, %s (%d)\t",
						(txmode < ENUM_TX_MODE_NUM) ?
						HW_TX_MODE_STR[txmode] : "N/A",
						(ldpc == 0) ?
							"BCC" : "LDPC",
						"MU", groupid);
			} else {
				i4BytesWritten +=
					kalScnprintf(pcCommand + i4BytesWritten,
						i4TotalLen - i4BytesWritten,
						"%s, %s\t",
						(txmode < ENUM_TX_MODE_NUM) ?
							HW_TX_MODE_STR[txmode] :
							"N/A",
						(ldpc == 0) ? "BCC" : "LDPC");
			}

		} else {

			if (mu) {
				i4BytesWritten += kalScnprintf(
					pcCommand + i4BytesWritten,
					i4TotalLen - i4BytesWritten,
					"%s, %s, %s (%d)\t",
					(txmode < ENUM_TX_MODE_NUM) ?
						HW_TX_MODE_STR[txmode] : "N/A",
					(ldpc == 0) ? "BCC" : "LDPC",
					"MU", groupid);
			} else {
				i4BytesWritten += kalScnprintf(
					pcCommand + i4BytesWritten,
					i4TotalLen - i4BytesWritten, "%s, %s\t",
					(txmode < ENUM_TX_MODE_NUM) ?
						HW_TX_MODE_STR[txmode] : "N/A",
					(ldpc == 0) ? "BCC" : "LDPC");
			}

		}

		/* Output the using times of this data rate */
		for (j = 0; j < ucCnt; j++)
			i4BytesWritten += kalScnprintf(
					pcCommand + i4BytesWritten,
					i4TotalLen - i4BytesWritten, "*");

		i4BytesWritten +=
			kalScnprintf(pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten, "\n");
	}

	return i4BytesWritten;
}

int32_t priv_driver_get_mcs_info(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen)
{
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	int32_t i4BytesWritten = 0, i4Argc = 0;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	struct PARAM_TX_MCS_INFO *prTxMcsInfo = NULL;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (prAdapter->rRxMcsInfoTimer.pfMgmtTimeOutFunc == NULL) {
		cnmTimerInitTimer(prAdapter,
			&prAdapter->rRxMcsInfoTimer,
			(PFN_MGMT_TIMEOUT_FUNC) wlanRxMcsInfoMonitor,
			(unsigned long) NULL);
	}

	if (i4Argc >= 2) {

		if (strnicmp(apcArgv[1], "START", strlen("START")) == 0) {
			cnmTimerStartTimer(prAdapter,
			&prAdapter->rRxMcsInfoTimer, MCS_INFO_SAMPLE_PERIOD);
			prAdapter->fgIsMcsInfoValid = TRUE;

			i4BytesWritten += kalScnprintf(
					pcCommand + i4BytesWritten,
					i4TotalLen - i4BytesWritten,
					"\nStart the MCS Info Function\n");
			return i4BytesWritten;

		} else if (strnicmp(apcArgv[1], "STOP", strlen("STOP")) == 0) {
			cnmTimerStopTimer(prAdapter,
				&prAdapter->rRxMcsInfoTimer);
			prAdapter->fgIsMcsInfoValid = FALSE;

			i4BytesWritten += kalScnprintf(
					pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"\nStop the MCS Info Function\n");
			return i4BytesWritten;

		} else
			goto warning;
	}

	if (prGlueInfo->prAdapter->fgIsMcsInfoValid != TRUE)
		goto warning;

	prTxMcsInfo = (struct PARAM_TX_MCS_INFO *)kalMemAlloc(
			sizeof(struct PARAM_TX_MCS_INFO), VIR_MEM_TYPE);
	if (!prTxMcsInfo) {
		DBGLOG(REQ, ERROR, "Allocate prTxMcsInfo failed!\n");
		i4BytesWritten = -1;
		goto out;
	}

	rStatus = kalIoctl(prGlueInfo, wlanoidTxQueryMcsInfo, prTxMcsInfo,
			   sizeof(struct PARAM_TX_MCS_INFO), &u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS)
		goto out;

	i4BytesWritten = priv_driver_last_sec_mcs_info(prGlueInfo->prAdapter,
			pcCommand, i4TotalLen, prTxMcsInfo);

	DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__, pcCommand);

	goto out;

warning:

	i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
		i4TotalLen - i4BytesWritten,
		"\nWARNING: Use GET_MCS_INFO [START/STOP]\n");

out:

	if (prTxMcsInfo)
		kalMemFree(prTxMcsInfo, VIR_MEM_TYPE,
			sizeof(struct PARAM_TX_MCS_INFO));

	return i4BytesWritten;
}
#endif /* CFG_WIFI_GET_MCS_INFO */

#if CFG_ENABLE_WIFI_DIRECT
int priv_driver_set_p2p_ps(struct net_device *prNetDev,
				  char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	struct BSS_INFO *prBssInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	uint32_t ucRoleIdx;
	uint8_t ucBssIdx;
	uint32_t u4CTwindowMs;
	struct P2P_SPECIFIC_BSS_INFO *prP2pSpecBssInfo = NULL;
	struct PARAM_CUSTOM_OPPPS_PARAM_STRUCT *rOppPsParam = NULL;

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc <= 2) {
		DBGLOG(REQ, ERROR,
		 "Expect param: <role_idx> <CTW>. argc=%d now\n", i4Argc);
		return -1;
	}

	if (kalkStrtou32(apcArgv[1], 0, &ucRoleIdx)) {
		DBGLOG(REQ, ERROR, "parse ucRoleIdx error\n");
		return -1;
	}

	if (kalkStrtou32(apcArgv[2], 0, &u4CTwindowMs)) {
		DBGLOG(REQ, ERROR, "parse u4CTwindowMs error\n");
		return -1;
	}

	/* get Bss Index from ndev */
	if (p2pFuncRoleToBssIdx(prAdapter, ucRoleIdx, &ucBssIdx) !=
		WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, ERROR, "can't find ucBssIdx\n");
		return -1;
	}

	prBssInfo = prAdapter->aprBssInfo[ucBssIdx];

	if (!(prBssInfo->fgIsInUse) || (prBssInfo->eIftype != IFTYPE_P2P_GO)) {
		DBGLOG(REQ, ERROR, "wrong bss InUse=%d, iftype=%d\n",
			prBssInfo->fgIsInUse, prBssInfo->eIftype);
		return -1;
	}

	DBGLOG(REQ, INFO, "ucRoleIdx=%d, ucBssIdx=%d, u4CTwindowMs=%d\n",
		ucRoleIdx, ucBssIdx, u4CTwindowMs);

	if (u4CTwindowMs > 0)
		u4CTwindowMs |= BIT(7);	/* FW checks BIT(7) for enable */

	prP2pSpecBssInfo = prAdapter->rWifiVar.prP2pSpecificBssInfo[ucRoleIdx];
	rOppPsParam = &prP2pSpecBssInfo->rOppPsParam;
	rOppPsParam->u4CTwindowMs = u4CTwindowMs;
	rOppPsParam->ucBssIdx = ucBssIdx;

	rStatus = kalIoctl(prGlueInfo, wlanoidSetOppPsParam, rOppPsParam,
			sizeof(struct PARAM_CUSTOM_OPPPS_PARAM_STRUCT),
			&u4BufLen);

	return !(rStatus == WLAN_STATUS_SUCCESS);
}

int priv_driver_set_p2p_noa(struct net_device *prNetDev,
				   char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	struct BSS_INFO *prBssInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	uint8_t ucBssIdx;
	uint32_t ucRoleIdx;
	uint32_t u4NoaDurationMs;
	uint32_t u4NoaIntervalMs;
	uint32_t u4NoaCount;
	struct P2P_SPECIFIC_BSS_INFO *prP2pSpecBssInfo = NULL;
	struct PARAM_CUSTOM_NOA_PARAM_STRUCT *rNoaParam = NULL;

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc <= 4) {
		DBGLOG(REQ, ERROR,
		  "SET_P2P_NOA <role_idx> <count> <interval> <duration>\n");
		return -1;
	}

	if (kalkStrtou32(apcArgv[1], 0, &ucRoleIdx)) {
		DBGLOG(REQ, ERROR, "parse ucRoleIdx error\n");
		return -1;
	}

	if (kalkStrtou32(apcArgv[2], 0, &u4NoaCount)) {
		DBGLOG(REQ, ERROR, "parse u4NoaCount error\n");
		return -1;
	}

	if (kalkStrtou32(apcArgv[3], 0, &u4NoaIntervalMs)) {
		DBGLOG(REQ, ERROR, "parse u4NoaIntervalMs error\n");
		return -1;
	}

	if (kalkStrtou32(apcArgv[4], 0, &u4NoaDurationMs)) {
		DBGLOG(REQ, ERROR, "parse u4NoaDurationMs error\n");
		return -1;
	}

	/* get Bss Index from ndev */
	if (p2pFuncRoleToBssIdx(prAdapter, ucRoleIdx, &ucBssIdx) !=
		WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, ERROR, "can't find ucBssIdx\n");
		return -1;
	}

	prBssInfo = prAdapter->aprBssInfo[ucBssIdx];

	if (!(prBssInfo->fgIsInUse) || (prBssInfo->eIftype != IFTYPE_P2P_GO)) {
		DBGLOG(REQ, ERROR, "wrong bss InUse=%d, iftype=%d\n",
			prBssInfo->fgIsInUse, prBssInfo->eIftype);
		return -1;
	}

	DBGLOG(REQ, INFO,
		"RoleIdx=%d, BssIdx=%d, count=%d, interval=%d, duration=%d\n",
		ucRoleIdx, ucBssIdx, u4NoaCount, u4NoaIntervalMs,
		u4NoaDurationMs);

	prP2pSpecBssInfo = prAdapter->rWifiVar.prP2pSpecificBssInfo[ucRoleIdx];
	rNoaParam = &prP2pSpecBssInfo->rNoaParam;
	rNoaParam->u4NoaCount = u4NoaCount;
	rNoaParam->u4NoaIntervalMs = u4NoaIntervalMs;
	rNoaParam->u4NoaDurationMs = u4NoaDurationMs;
	rNoaParam->ucBssIdx = ucBssIdx;

	rStatus = kalIoctl(prGlueInfo, wlanoidSetNoaParam, rNoaParam,
			sizeof(struct PARAM_CUSTOM_NOA_PARAM_STRUCT),
			&u4BufLen);

	return !(rStatus == WLAN_STATUS_SUCCESS);
}
#endif /* CFG_ENABLE_WIFI_DIRECT */

int priv_driver_set_drv_ser(struct net_device *prNetDev,
				   char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	int32_t i4Argc = 0;
	int32_t i4BytesWritten = 0;
	uint32_t u4Num = 0;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc <= 0) {
		DBGLOG(REQ, ERROR, "Argc(%d) ERR: Set driver SER\n", i4Argc);
		return -1;
	}

	rStatus = kalIoctl(prGlueInfo, wlanoidSetDrvSer,
			   (void *)&u4Num, sizeof(uint32_t), &u4BufLen);

	i4BytesWritten += kalSnprintf(pcCommand, i4TotalLen,
				   "trigger driver SER\n");
	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, ERROR, "ERR: kalIoctl fail (%d)\n", rStatus);
		return -1;
	}

	return i4BytesWritten;
}

#ifdef UT_TEST_MODE
int priv_driver_run_ut(struct net_device *prNetDev,
		       char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4Argc = 0;
	int8_t  *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	int32_t u4Ret = 0;
	uint32_t u4Input = 0;

	ASSERT(prNetDev);

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	if (!prGlueInfo || !prGlueInfo->prAdapter) {
		DBGLOG(REQ, ERROR, "prGlueInfo or prAdapter is NULL\n");
		return -1;
	}

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc < 2) {
		DBGLOG(REQ, ERROR, "Argc(%d) ERR: RUN_UT COMMAND\n", i4Argc);
		return -1;
	}

	if (strlen(apcArgv[1]) == strlen("all") &&
		   strnicmp(apcArgv[1], "all", strlen("all")) == 0)
		testRunAllTestCases(prGlueInfo->prAdapter);
	else if (i4Argc >= 3) {
		if (strlen(apcArgv[1]) == strlen("tc") &&
		    strnicmp(apcArgv[1], "tc", strlen("tc")) == 0) {
			u4Ret = kalkStrtou32(apcArgv[2], 0, &u4Input);
			if (u4Ret) {
				DBGLOG(REQ, ERROR, "parse error u4Ret=%d\n",
				       u4Ret);
				return -1;
			}
			testRunTestCase(prGlueInfo->prAdapter, u4Input);

		} else if (strlen(apcArgv[1]) == strlen("group") &&
			   strnicmp(apcArgv[1], "group",
				    strlen("group")) == 0) {
			testRunGroupTestCases(prGlueInfo->prAdapter,
					      apcArgv[2]);

		} else if (strlen(apcArgv[1]) == strlen("list") &&
			   strnicmp(apcArgv[1], "list", strlen("list")) == 0) {
			if (strlen(apcArgv[2]) == strlen("all") &&
			    strnicmp(apcArgv[2], "all", strlen("all")) == 0) {
				testRunAllTestCaseLists(prGlueInfo->prAdapter);
			} else {
				u4Ret = kalkStrtou32(apcArgv[2], 0, &u4Input);
				if (u4Ret) {
					DBGLOG(REQ, ERROR,
					       "parse error u4Ret=%d\n", u4Ret);
					return -1;
				}
				testRunTestCaseList(prGlueInfo->prAdapter,
						    u4Input);
			}
		}
	}

	return 0;
}
#endif /* UT_TEST_MODE */

int priv_driver_set_amsdu_num(struct net_device *prNetDev,
				     char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	int32_t i4Argc = 0;
	int32_t i4BytesWritten = 0;
	int32_t u4Ret = 0;
	uint32_t u4Num = 0;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc <= 1) {
		DBGLOG(REQ, ERROR, "Argc(%d) ERR: Sw Amsdu Num\n", i4Argc);
		return -1;
	}

	u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Num);
	if (u4Ret)
		DBGLOG(REQ, ERROR, "parse amsdu num error u4Ret=%d\n", u4Ret);

	rStatus = kalIoctl(prGlueInfo, wlanoidSetAmsduNum,
			   (void *)&u4Num, sizeof(uint32_t), &u4BufLen);

	i4BytesWritten += kalSnprintf(pcCommand, i4TotalLen,
				   "Set Sw Amsdu Num:%u\n", u4Num);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, ERROR, "ERR: kalIoctl fail (%d)\n", rStatus);
		return -1;
	}

	return i4BytesWritten;

}

int priv_driver_set_amsdu_size(struct net_device *prNetDev,
				      char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	int32_t i4Argc = 0;
	int32_t i4BytesWritten = 0;
	int32_t u4Ret = 0;
	uint32_t u4Size = 0;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc <= 1) {
		DBGLOG(REQ, ERROR, "Argc(%d) ERR: Sw Amsdu Max Size\n", i4Argc);
		return -1;
	}

	u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Size);
	if (u4Ret)
		DBGLOG(REQ, ERROR, "parse amsdu size error u4Ret=%d\n", u4Ret);

	rStatus = kalIoctl(prGlueInfo, wlanoidSetAmsduSize,
			   (void *)&u4Size, sizeof(uint32_t), &u4BufLen);

	i4BytesWritten += kalSnprintf(pcCommand, i4TotalLen,
				   "Set Sw Amsdu Max Size:%u\n", u4Size);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, ERROR, "ERR: kalIoctl fail (%d)\n", rStatus);
		return -1;
	}

	return i4BytesWritten;

}

#if CFG_WMT_RESET_API_SUPPORT
int priv_driver_trigger_whole_chip_reset(
	struct net_device *prNetDev,
	char *pcCommand,
	int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	if ((!prGlueInfo) ||
	    (prGlueInfo->u4ReadyFlag == 0) ||
	    kalIsResetting()) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -1;
	}

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	glSetRstReason(RST_CMD_TRIGGER);
#if CFG_SUPPORT_CONNAC1X
	GL_USER_DEFINE_RESET_TRIGGER(prGlueInfo->prAdapter,
			RST_CMD_TRIGGER, RST_FLAG_CHIP_RESET);
#else
	glSetRstReasonString(
		"cmd test trigger whole chip reset");
	glResetWholeChipResetTrigger(g_reason);
#endif

	return i4BytesWritten;
}

int priv_driver_trigger_wfsys_reset(
	struct net_device *prNetDev,
	char *pcCommand,
	int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	if ((!prGlueInfo) ||
	    (prGlueInfo->u4ReadyFlag == 0) ||
	    kalIsResetting()) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -1;
	}

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

#if CFG_SUPPORT_CONNAC1X
	GL_USER_DEFINE_RESET_TRIGGER(prGlueInfo->prAdapter,
			RST_CMD_TRIGGER, RST_FLAG_CHIP_RESET);
#else
	GL_USER_DEFINE_RESET_TRIGGER(prGlueInfo->prAdapter,
			RST_CMD_TRIGGER, RST_FLAG_WF_RESET);
#endif

	return i4BytesWritten;
}
#endif

#if (CFG_SUPPORT_CONNAC2X == 1 || CFG_SUPPORT_CONNAC3X == 1)
int priv_driver_get_uwtbl(
	struct net_device *prNetDev,
	char *pcCommand,
	int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4Argc = 0;
	int32_t u4Ret = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Index = 0;
	struct CHIP_DBG_OPS *prDbgOps;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Index);

	if (u4Ret)
		return -1;

	prDbgOps = prGlueInfo->prAdapter->chip_info->prDebugOps;

	if (prDbgOps->showUmacWtblInfo)
		return prDbgOps->showUmacWtblInfo(
			prGlueInfo->prAdapter, u4Index, pcCommand, i4TotalLen);
	else
		return -1;
}
#endif /* CFG_SUPPORT_CONNAC2X == 1 */

int priv_driver_show_txd_info(
		struct net_device *prNetDev,
		char *pcCommand,
		int i4TotalLen
)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t u4Ret = -1;
	int32_t idx = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	struct CHIP_DBG_OPS *prDbgOps;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prDbgOps = prGlueInfo->prAdapter->chip_info->prDebugOps;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	u4Ret = kalkStrtos32(apcArgv[1], 16, &idx);

	if (!u4Ret && prDbgOps && prDbgOps->showTxdInfo) {
		DBGLOG(HAL, INFO, "idx = %d 0x%x\n", idx, idx);
		prDbgOps->showTxdInfo(prGlueInfo->prAdapter, idx);
	}
	return i4BytesWritten;
}
#if (CONFIG_WLAN_SERVICE == 1)
int8_t *RxStatCommonUser[] = {
	/* common user stat info */
	"RxFifoFull:0x%08x\n",
#if (CFG_SUPPORT_CONNAC3X == 0) /* comm_info v1 */
	"AciHitLow:0x%08x\n",
	"AciHitHigh:0x%08x\n",
#endif
	"MuPktCnt:0x%08x\n",
	"SigMcs:0x%08x\n",
	"Sinr:0x%08x\n",
	"DrvRxCnt:0x%08x\n",
#if (CFG_SUPPORT_CONNAC3X == 1) /* comm_info v1 */
	"NeVarDb:0x%08x\n"
#endif
};

int8_t *RxStatPerUser[] = {
   /* per user stat info */
	"FreqOfstFromRx:0x%08x\n",
	"Snr:0x%08x\n",
	"FcsErrCnt:0x%08x\n"
};

int8_t *RxStatPerAnt[] = {
	/* per anternna stat info */
	"Rcpi:%d\n",
	"Rssi:%d\n",
	"FagcIbRssi:%d\n",
	"FagcWbRssi:%d\n",
	"InstIbRssi:%d\n",
	"InstWbRssi:%d\n",
#if (CFG_SUPPORT_CONNAC3X == 1) /* path_info v1 */
	"Adc_Rssi:%d\n"
#endif
};

int8_t *RxStatPerBand[] = {
	/* per band stat info */
	"MacFcsErr:0x%08x\n",
	"MacMdy:0x%08x\n",
	"MacLenMis:0x%08x\n",
	"MacFcsOk:0x%08x\n",
	"PhyFcsErrCck:0x%08x\n",
	"PhyFcsErrOfdm:0x%08x\n",
	"PhyPdCck:0x%08x\n",
	"PhyPdOfdm:0x%08x\n",
	"PhySigErrCck:0x%08x\n",
	"PhySfdErrCck:0x%08x\n",
	"PhySigErrOfdm:0x%08x\n",
	"PhyTagErrOfdm:0x%08x\n",
	"PhyMdyCck:0x%08x\n",
	"PhyMdyOfdm:0x%08x\n",
#if (CFG_SUPPORT_CONNAC3X == 1) /* band info v1*/
	"AciHitLow:0x%08x\n",
	"AciHitHigh:0x%08x\n",
	"PhyPdAlr:0x%08x\n",	/* band info v2*/
	"MacU2mMpdu:0x%08x\n"	/* band info v2*/
#endif
};

int32_t priv_driver_rx_stat_parser(
	uint8_t *dataptr,
	int i4TotalLen,
	char *pcCommand
)
{
#define	RX_STAT_TYPE_LEN_MAX	1000

	int32_t i4BytesWritten = 0;
	int32_t i4tmpContent = 0;
	int32_t i4TypeNum = 0, i4Type = 0, i4Version = 0;
	int32_t i = 0, j = 0, i4ItemMask = 0, i4TypeLen = 0, i4SubLen = 0;
	/*Get type num*/
	i4TypeNum = NTOHL(dataptr[3] << 24 | dataptr[2] << 16 |
	dataptr[1] << 8 | dataptr[0]);
	dataptr += 4;
	DBGLOG(REQ, LOUD, "i4TypeNum is %x\n", i4TypeNum);
	while (i4TypeNum) {
		i4TypeNum--;
		/*Get type*/
		i4Type = NTOHL(dataptr[3] << 24 | dataptr[2] << 16 |
		dataptr[1] << 8 | dataptr[0]);
		dataptr += 4;
		DBGLOG(REQ, LOUD, "i4Type is %x\n", i4Type);

		/*Get Version*/
		i4Version = NTOHL(dataptr[3] << 24 | dataptr[2] << 16 |
		dataptr[1] << 8 | dataptr[0]);
		dataptr += 4;
		DBGLOG(REQ, LOUD, "i4Version is %x\n", i4Version);

		/*Get Item Mask*/
		i4ItemMask = NTOHL(dataptr[3] << 24 | dataptr[2] << 16 |
		dataptr[1] << 8 | dataptr[0]);
		j = i4ItemMask;
		dataptr += 4;
		DBGLOG(REQ, LOUD, "i4ItemMask is %x\n", i4ItemMask);

		/*Get Len*/
		i4TypeLen = NTOHL(dataptr[3] << 24 | dataptr[2] << 16 |
		dataptr[1] << 8 | dataptr[0]);
		dataptr += 4;
		DBGLOG(REQ, LOUD, "i4TypeLen is %x\n", i4TypeLen);

		/* SANITY CHECK */
		if (i4TypeLen > RX_STAT_TYPE_LEN_MAX)
			return 0;

		/*Get Sub Len*/
		if (i4Type != 0) {
			while (j) {
				i++;
				j = j >> 1;
			}

			if (i != 0)
				i4SubLen = i4TypeLen / i;
		} else
			i4SubLen = i4TypeLen;

		i = 0;
		DBGLOG(REQ, LOUD, "i4SubLen is %x\n", i4SubLen);

		while (i4TypeLen) {
			while (i < i4SubLen) {
				/*Get Content*/
				i4tmpContent = NTOHL(dataptr[3] << 24 |
				dataptr[2] << 16 | dataptr[1] << 8 |
				dataptr[0]);
				DBGLOG(REQ, LOUD,
				"i4tmpContent is %x\n", i4tmpContent);

				/*log boundary is 0x7ff*/
				if (i4BytesWritten > 0x7B0) {
					DBGLOG(REQ, WARN,
						"%s:i4BytesWritten is 0x%x, limit is 0x%x\n",
						__func__,
						i4BytesWritten,
						IW_PRIV_GET_BUF_SIZE);
				}

				if (i4Type == 0) {
					if ((i/4) >=
						(sizeof(RxStatPerBand)/
						sizeof(int8_t *)))
						return 0;

					i4BytesWritten += kalSnprintf(
						pcCommand + i4BytesWritten,
						i4TotalLen, RxStatPerBand[i/4],
						i4tmpContent);
				} else if (i4Type == 1) {
					if ((i/4) >=
						(sizeof(RxStatPerAnt)/
						sizeof(int8_t *)))
						return 0;

					i4BytesWritten += kalSnprintf(
						pcCommand + i4BytesWritten,
						i4TotalLen, RxStatPerAnt[i/4],
						i4tmpContent);
				} else if (i4Type == 2) {
					if ((i/4) >=
						(sizeof(RxStatPerUser)/
						sizeof(int8_t *)))
						return 0;

					i4BytesWritten += kalSnprintf(
						pcCommand + i4BytesWritten,
						i4TotalLen, RxStatPerUser[i/4],
						i4tmpContent);
				} else {
					if ((i/4) >=
						(sizeof(RxStatCommonUser)/
						sizeof(int8_t *)))
						return 0;

					i4BytesWritten += kalSnprintf(
						pcCommand + i4BytesWritten,
						i4TotalLen,
						RxStatCommonUser[i/4],
						i4tmpContent);
				}
				i += 4;
				dataptr += 4;
			}
			i = 0;
			i4TypeLen -= i4SubLen;
		}
	}
	return i4BytesWritten;
}
#endif

int priv_driver_run_hqa(
	struct net_device *prNetDev,
	char *pcCommand,
	int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int8_t *this_char = NULL;
#if (CONFIG_WLAN_SERVICE == 1)
	struct hqa_frame_ctrl local_hqa;
	bool IsShowRxStat = FALSE;
	uint8_t *dataptr = NULL;
	uint8_t *oridataptr = NULL;
	int32_t datalen = 0;
	int32_t oridatalen = 0;
	int32_t ret = WLAN_STATUS_FAILURE;
	int16_t i2tmpVal = 0;
	int32_t i4tmpVal = 0;
#endif
	int32_t i = 0;

	ASSERT(prNetDev);
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);

	/*Roll over "HQA ", handle xxx=y,y,y,y,y,y....*/
	/* iwpriv wlan0 driver HQA xxx=y,y,y,y,y,y.....*/
	this_char = kalStrStr(pcCommand, "HQA ");
	if (!this_char)
		return -1;
	this_char += strlen("HQA ");

	/*handle white space*/
	i = strspn(this_char, " ");
	this_char += i;

	DBGLOG(REQ, LOUD, "this_char is %s\n", this_char);
#if (CONFIG_WLAN_SERVICE == 1)
	if (this_char) {
		if (strncasecmp(this_char,
		"GetRXStatisticsAllNew",
		strlen("GetRXStatisticsAllNew")) == 0)
		IsShowRxStat = TRUE;

		local_hqa.type = 1;
		local_hqa.hqa_frame_comm.hqa_frame_string = this_char;

		KAL_ACQUIRE_MUTEX(prGlueInfo->prAdapter, MUTEX_HQA_TEST);
		ret = mt_agent_hqa_cmd_handler(&prGlueInfo->rService,
		&local_hqa);
		KAL_RELEASE_MUTEX(prGlueInfo->prAdapter, MUTEX_HQA_TEST);
	}

	if (ret != WLAN_STATUS_SUCCESS)
		return -1;

	datalen = NTOHS(local_hqa.hqa_frame_comm.hqa_frame_eth->length);

	if (datalen > SERV_IOCTLBUFF)
		datalen = SERV_IOCTLBUFF;

	dataptr = kalMemAlloc(datalen, VIR_MEM_TYPE);
	if (dataptr == NULL)
		return -1;

	/* Backup Original Ptr /Len for mem Free */
	oridataptr = dataptr;
	oridatalen = datalen;
	kalMemCopy(dataptr,
	local_hqa.hqa_frame_comm.hqa_frame_eth->data, datalen);

	DBGLOG(REQ, LOUD,
	"priv_driver_run_hqa datalen is %d\n", datalen);
	DBGLOG_MEM8(REQ, LOUD, dataptr, datalen);

	/*parsing ret 2 bytes*/
	if ((dataptr) && (datalen)) {
		i2tmpVal = dataptr[1] << 8 | dataptr[0];
		i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen,
		"Return : 0x%04x\n", i2tmpVal);

		datalen -= 2;
		dataptr += 2;
	} else {
		DBGLOG(REQ, ERROR,
		"priv_driver_run_hqa not support\n");
		kalMemFree(oridataptr, VIR_MEM_TYPE, oridatalen);
		return -1;
	}

	/*parsing remaining data n bytes ( 4 bytes per parameter)*/
	for (i = 0; i < datalen ; i += 4, dataptr += 4) {
		if ((dataptr) && (datalen)) {
			i4tmpVal = dataptr[3] << 24 | dataptr[2] << 16 |
			dataptr[1] << 8 | dataptr[0];
			if (datalen == 4) {
				i4BytesWritten +=
				kalSnprintf(pcCommand + i4BytesWritten,
				i4TotalLen, "ExtId : 0x%08x\n", i4tmpVal);
			} else if (datalen == 8) {
				i4BytesWritten +=
				kalSnprintf(pcCommand + i4BytesWritten,
				i4TotalLen, "Band%d TX : 0x%08x\n", i/4,
				NTOHL(i4tmpVal));
			} else {
				if (IsShowRxStat) {
				i += datalen;
				i4BytesWritten +=
				priv_driver_rx_stat_parser(dataptr,
				i4TotalLen, pcCommand + i4BytesWritten);
				} else {
				i4BytesWritten +=
				kalSnprintf(pcCommand + i4BytesWritten,
				i4TotalLen, "id%d : 0x%08x\n", i/4,
				NTOHL(i4tmpVal));
				}
			}
		}
	}

	kalMemFree(oridataptr, VIR_MEM_TYPE, oridatalen);
#else
	DBGLOG(REQ, ERROR,
	"wlan_service not support\n");
#endif
	return i4BytesWritten;

}

#if (CFG_WLAN_ASSISTANT_NVRAM == 1)
int priv_driver_get_nvram(struct net_device *prNetDev,
			       char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret;
	int32_t i4ArgNum = 2;
	uint32_t u4Offset = 0;
	uint16_t u2Index = 0;
	struct PARAM_CUSTOM_EEPROM_RW_STRUCT rNvrRwInfo;
	struct WIFI_CFG_PARAM_STRUCT *prNvSet;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	kalMemZero(&rNvrRwInfo, sizeof(rNvrRwInfo));

	prNvSet = prGlueInfo->rRegInfo.prNvramSettings;
	ASSERT(prNvSet);


	DBGLOG(REQ, INFO, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, INFO, "argc is %i\n", i4Argc);


	if (i4Argc == i4ArgNum) {
		u4Ret = kalkStrtou16(apcArgv[1], 0, &u2Index);

		if (u4Ret)
			DBGLOG(REQ, INFO,
			       "parse get_nvram error (Index) u4Ret=%d\n",
			       u4Ret);


		DBGLOG(REQ, INFO, "Index is 0x%x\n", u2Index);

		/* NVRAM Check */
		if (prGlueInfo->fgNvramAvailable == TRUE)
			u4Offset += snprintf(pcCommand + u4Offset,
				(i4TotalLen - u4Offset),
				"NVRAM Version is[%d.%d.%d]\n",
				(prNvSet->u2Part1OwnVersion & 0x00FF),
				(prNvSet->u2Part1OwnVersion & 0xFF00) >> 8,
				(prNvSet->u2Part1PeerVersion & 0xFF));
		else {
			u4Offset += snprintf(pcCommand + u4Offset,
				i4TotalLen - u4Offset,
				"[WARNING] NVRAM is unavailable!\n");

			return (int32_t) u4Offset;
		}

		rNvrRwInfo.ucMethod = PARAM_EEPROM_READ_NVRAM;
		rNvrRwInfo.info.rNvram.u2NvIndex = u2Index;

		rStatus = kalIoctl(prGlueInfo, wlanoidQueryNvramRead,
				   &rNvrRwInfo, sizeof(rNvrRwInfo), &u4BufLen);

		DBGLOG(REQ, INFO, "rStatus %u\n", rStatus);
		if (rStatus != WLAN_STATUS_SUCCESS)
			return -1;

		u4Offset += snprintf(pcCommand + u4Offset,
			(i4TotalLen - u4Offset),
			"NVRAM [0x%02X] = %d (0x%02X)\n",
			(unsigned int)rNvrRwInfo.info.rNvram.u2NvIndex,
			(unsigned int)rNvrRwInfo.info.rNvram.u2NvData,
			(unsigned int)rNvrRwInfo.info.rNvram.u2NvData);

		DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__,
		       pcCommand);
	}

	return (int32_t)u4Offset;

}				/* priv_driver_get_nvram */

int priv_driver_set_nvram(struct net_device *prNetDev, char *pcCommand,
			int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	uint32_t u4Ret;
	int32_t i4ArgNum = 3;

	struct PARAM_CUSTOM_EEPROM_RW_STRUCT rNvRwInfo;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, INFO, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, INFO, "argc is %i\n", i4Argc);

	kalMemZero(&rNvRwInfo, sizeof(rNvRwInfo));

	if (i4Argc >= i4ArgNum) {

		rNvRwInfo.ucMethod = PARAM_EEPROM_WRITE_NVRAM;

		u4Ret = kalkStrtou16(apcArgv[1], 0,
			&(rNvRwInfo.info.rNvram.u2NvIndex));
		if (u4Ret)
			DBGLOG(REQ, ERROR,
			       "parse set_nvram error (Add) Ret=%d\n",
			       u4Ret);

		u4Ret = kalkStrtou16(apcArgv[2], 0,
			&(rNvRwInfo.info.rNvram.u2NvData));
		if (u4Ret)
			DBGLOG(REQ, ERROR,
				"parse set_nvram error (Data) Ret=%d\n",
				u4Ret);

		rStatus = kalIoctl(prGlueInfo, wlanoidSetNvramWrite,
				   &rNvRwInfo, sizeof(rNvRwInfo), &u4BufLen);

		if (rStatus != WLAN_STATUS_SUCCESS)
			return -1;

	} else
		DBGLOG(INIT, ERROR,
					   "[Error]iwpriv wlanXX driver set_nvram [addr] [value]\n");


	return i4BytesWritten;

}
#endif

int priv_driver_support_nvram(
	struct net_device *prNetDev,
	char *pcCommand,
	int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct WIFI_CFG_PARAM_STRUCT *prNvSet;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	int32_t i4Argc = 0;
	int32_t i4ArgNum = 1;
	uint32_t u4Offset = 0;

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, INFO, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, INFO, "argc is %i\n", i4Argc);

	if (i4Argc == i4ArgNum) {
		u4Offset += snprintf(pcCommand + u4Offset,
				(i4TotalLen - u4Offset),
				 "%d\n",
				prGlueInfo->fgNvramAvailable);

		if (prGlueInfo->fgNvramAvailable == TRUE) {
			prNvSet = prGlueInfo->rRegInfo.prNvramSettings;
			u4Offset += snprintf(pcCommand + u4Offset,
				(i4TotalLen - u4Offset),
				"NVRAM Version is[%d.%d.%d]\n",
				(prNvSet->u2Part1OwnVersion & 0x00FF),
				(prNvSet->u2Part1OwnVersion & 0xFF00) >> 8,
				(prNvSet->u2Part1PeerVersion & 0xFF));
		} else {
			u4Offset += snprintf(pcCommand + u4Offset,
					(i4TotalLen - u4Offset),
					"NVRAM nonsupport!\n");
		}
		DBGLOG(REQ, INFO,
			"%s: command result is %s\n", __func__, pcCommand);
	}
	return (int32_t)u4Offset;
}

int priv_driver_thermal_protect_enable(struct net_device *prNetDev,
		char *pcCommand, int i4TotalLen)
{

	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	uint8_t uParse = 0;
	uint32_t u4Parse = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	uint32_t u4Ret;
	struct EXT_CMD_THERMAL_PROTECT_ENABLE *ext_cmd_buf;

	DBGLOG(REQ, INFO, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc != 7)
		return 0;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	ext_cmd_buf = kalMemAlloc(
			sizeof(struct EXT_CMD_THERMAL_PROTECT_ENABLE),
			VIR_MEM_TYPE);

	if (!ext_cmd_buf)
		return 0;

	u4Ret = kalkStrtou8(apcArgv[1], 0, &uParse);
	ext_cmd_buf->band_idx = uParse;
	u4Ret = kalkStrtou8(apcArgv[2], 0, &uParse);
	ext_cmd_buf->protection_type = uParse;
	u4Ret = kalkStrtou8(apcArgv[3], 0, &uParse);
	ext_cmd_buf->trigger_type = uParse;
	u4Ret = kalkStrtou32(apcArgv[4], 0, &u4Parse);
	ext_cmd_buf->trigger_temp = u4Parse;
	u4Ret = kalkStrtou32(apcArgv[5], 0, &u4Parse);
	ext_cmd_buf->restore_temp = u4Parse;
	u4Ret = kalkStrtou32(apcArgv[6], 0, &u4Parse);
	ext_cmd_buf->recheck_time = u4Parse;
	ext_cmd_buf->sub_cmd_id = THERMAL_PROTECT_ENABLE;

	rStatus = kalIoctl(prGlueInfo, wlanoidThermalProtectAct,
				ext_cmd_buf,
				sizeof(struct EXT_CMD_THERMAL_PROTECT_ENABLE),
				&u4BufLen);

	kalMemFree(ext_cmd_buf, VIR_MEM_TYPE,
				sizeof(struct EXT_CMD_THERMAL_PROTECT_ENABLE));
	return 1;
}

int priv_driver_thermal_protect_disable(struct net_device *prNetDev,
		char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	uint8_t uParse = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	uint32_t u4Ret;
	struct EXT_CMD_THERMAL_PROTECT_DISABLE *ext_cmd_buf;

	DBGLOG(REQ, INFO, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc != 4)
		return 0;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	ext_cmd_buf = kalMemAlloc(
			sizeof(struct EXT_CMD_THERMAL_PROTECT_DISABLE),
			VIR_MEM_TYPE);

	if (!ext_cmd_buf)
		return 0;

	u4Ret = kalkStrtou8(apcArgv[1], 0, &uParse);
	ext_cmd_buf->band_idx = uParse;
	u4Ret = kalkStrtou8(apcArgv[2], 0, &uParse);
	ext_cmd_buf->protection_type = uParse;
	u4Ret = kalkStrtou8(apcArgv[3], 0, &uParse);
	ext_cmd_buf->trigger_type = uParse;
	ext_cmd_buf->sub_cmd_id = THERMAL_PROTECT_DISABLE;

	rStatus = kalIoctl(prGlueInfo, wlanoidThermalProtectAct, ext_cmd_buf,
		sizeof(struct EXT_CMD_THERMAL_PROTECT_DISABLE), &u4BufLen);

	kalMemFree(ext_cmd_buf, VIR_MEM_TYPE,
		sizeof(struct EXT_CMD_THERMAL_PROTECT_DISABLE));

	return 1;
}

int priv_driver_thermal_protect_duty_cfg(struct net_device *prNetDev,
		char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	uint8_t uParse = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	uint32_t u4Ret;
	struct EXT_CMD_THERMAL_PROTECT_DUTY_CFG *ext_cmd_buf;

	DBGLOG(REQ, INFO, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc != 4)
		return 0;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	ext_cmd_buf = kalMemAlloc(
			sizeof(struct EXT_CMD_THERMAL_PROTECT_DUTY_CFG),
			VIR_MEM_TYPE);

	if (!ext_cmd_buf)
		return 0;

	u4Ret = kalkStrtou8(apcArgv[1], 0, &uParse);
	ext_cmd_buf->band_idx = uParse;
	u4Ret = kalkStrtou8(apcArgv[2], 0, &uParse);
	ext_cmd_buf->level_idx = uParse;
	u4Ret = kalkStrtou8(apcArgv[3], 0, &uParse);
	ext_cmd_buf->duty = uParse;

	ext_cmd_buf->sub_cmd_id = THERMAL_PROTECT_DUTY_CONFIG;

	rStatus = kalIoctl(prGlueInfo, wlanoidThermalProtectAct, ext_cmd_buf,
				sizeof(struct EXT_CMD_THERMAL_PROTECT_DUTY_CFG),
				&u4BufLen);

	kalMemFree(ext_cmd_buf, VIR_MEM_TYPE,
			sizeof(struct EXT_CMD_THERMAL_PROTECT_DUTY_CFG));
	return 1;
}

int priv_driver_thermal_protect_info(struct net_device *prNetDev,
		char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	uint8_t uParse = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	uint32_t u4Ret;
	struct EXT_CMD_THERMAL_PROTECT_INFO *ext_cmd_buf;

	DBGLOG(REQ, INFO, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc != 2)
		return 0;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	ext_cmd_buf = kalMemAlloc(
			sizeof(struct EXT_CMD_THERMAL_PROTECT_INFO),
			VIR_MEM_TYPE);

	if (!ext_cmd_buf)
		return 0;

	u4Ret = kalkStrtou8(apcArgv[1], 0, &uParse);
	ext_cmd_buf->band_idx = uParse;

	ext_cmd_buf->sub_cmd_id = THERMAL_PROTECT_MECH_INFO;

	rStatus = kalIoctl(prGlueInfo, wlanoidThermalProtectAct, ext_cmd_buf,
				sizeof(struct EXT_CMD_THERMAL_PROTECT_INFO),
				&u4BufLen);

	kalMemFree(ext_cmd_buf, VIR_MEM_TYPE,
			sizeof(struct EXT_CMD_THERMAL_PROTECT_INFO));
	return 1;
}

int priv_driver_thermal_protect_duty_info(struct net_device *prNetDev,
		char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	uint8_t uParse = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	uint32_t u4Ret;
	struct EXT_CMD_THERMAL_PROTECT_DUTY_INFO *ext_cmd_buf;

	DBGLOG(REQ, INFO, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, INFO, "argc %d\n", i4Argc);

	if (i4Argc != 2)
		return 0;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	ext_cmd_buf = kalMemAlloc(
			sizeof(struct EXT_CMD_THERMAL_PROTECT_DUTY_INFO),
			VIR_MEM_TYPE);

	if (!ext_cmd_buf)
		return 0;

	u4Ret = kalkStrtou8(apcArgv[1], 0, &uParse);
	ext_cmd_buf->band_idx = uParse;

	ext_cmd_buf->sub_cmd_id = THERMAL_PROTECT_DUTY_INFO;

	rStatus = kalIoctl(prGlueInfo, wlanoidThermalProtectAct, ext_cmd_buf,
			sizeof(struct EXT_CMD_THERMAL_PROTECT_DUTY_INFO),
			&u4BufLen);

	kalMemFree(ext_cmd_buf, VIR_MEM_TYPE,
			sizeof(struct EXT_CMD_THERMAL_PROTECT_DUTY_INFO));
	return 1;
}

int priv_driver_thermal_protect_state_act(struct net_device *prNetDev,
		char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	uint8_t uParse = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	uint32_t u4Ret;
	struct EXT_CMD_THERMAL_PROTECT_STATE_ACT *ext_cmd_buf;

	DBGLOG(REQ, INFO, "command is %s\n", pcCommand);

	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc != 5)
		return 0;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	ext_cmd_buf = kalMemAlloc(
			sizeof(struct EXT_CMD_THERMAL_PROTECT_STATE_ACT),
			VIR_MEM_TYPE);

	if (!ext_cmd_buf)
		return 0;

	u4Ret = kalkStrtou8(apcArgv[1], 0, &uParse);
	ext_cmd_buf->band_idx = uParse;
	u4Ret = kalkStrtou8(apcArgv[2], 0, &uParse);
	ext_cmd_buf->protect_type = uParse;
	u4Ret = kalkStrtou8(apcArgv[3], 0, &uParse);
	ext_cmd_buf->trig_type = uParse;
	u4Ret = kalkStrtou8(apcArgv[4], 0, &uParse);
	ext_cmd_buf->state = uParse;

	ext_cmd_buf->sub_cmd_id = THERMAL_PROTECT_STATE_ACT;

	rStatus = kalIoctl(prGlueInfo, wlanoidThermalProtectAct, ext_cmd_buf,
			sizeof(struct EXT_CMD_THERMAL_PROTECT_STATE_ACT),
			&u4BufLen);

	kalMemFree(ext_cmd_buf, VIR_MEM_TYPE,
			sizeof(struct EXT_CMD_THERMAL_PROTECT_STATE_ACT));
	return 1;
}

int priv_driver_get_hapd_channel(
	struct net_device *prNetDev,
	char *pcCommand,
	int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	struct BSS_INFO *prAisBssInfo = NULL;
	int32_t i4BytesWritten = 0;
	uint8_t ucChannel;

	DBGLOG(REQ, INFO, "command is %s\n", pcCommand);

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		goto error;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	prAisBssInfo = aisGetConnectedBssInfo(prAdapter);
	if (!prAisBssInfo) {
		DBGLOG(REQ, WARN, "bss is not active\n");
		/* AUTO */
		ucChannel = 0;
	} else {
		DBGLOG(REQ, INFO,
			"STA operating channel: %d, band: %d, conn state: %d",
			prAisBssInfo->ucPrimaryChannel,
			prAisBssInfo->eBand,
			prAisBssInfo->eConnectionState);
		ucChannel = prAisBssInfo->ucPrimaryChannel;
	}

	i4BytesWritten = kalSnprintf(
		pcCommand, i4TotalLen, "%d", ucChannel);

	DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__, pcCommand);

	return i4BytesWritten;

error:
	return -1;
}

#if (CFG_SUPPORT_802_11BE_EPCS == 1)
int priv_driver_epcs_send(struct net_device *prNetDev, char *pcCommand,
		int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	uint8_t *pucType = NULL;
	uint8_t ucBssIndex = 0;

	if ((kalStrnLen(pcCommand, i4TotalLen) > kalStrLen(CMD_EPCS_SEND))
		&& (strnicmp(pcCommand + kalStrLen(CMD_EPCS_SEND),
		" type=", 6) == 0)) {
		pucType = pcCommand + kalStrLen(CMD_EPCS_SEND) + 6;
		DBGLOG(REQ, INFO,
			"SEND_EPCS, type=%s\r\n", pucType);
	} else {
		DBGLOG(REQ, INFO,
			"wrong format! expected foramt: EPCS_SEND type=3");
		return -EFAULT;
	}

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	if (!prGlueInfo)
		return -EFAULT;

	ucBssIndex = wlanGetBssIdx(prNetDev);
	if (!IS_BSS_INDEX_VALID(ucBssIndex))
		return -EINVAL;

	DBGLOG(REQ, INFO, "ucBssIndex = %d\n", ucBssIndex);

	rStatus = kalIoctlByBssIdx(prGlueInfo, wlanoidSendEpcs,
			pucType, kalStrLen(pucType), &u4BufLen, ucBssIndex);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, ERROR, "ERR: kalIoctl fail (%d)\r\n", rStatus);
		return -1;
	}

	return i4BytesWritten;

}
#endif /* CFG_SUPPORT_802_11BE_EPCS */

#if CFG_AP_80211KVR_INTERFACE
int32_t MulAPAgentMontorSendMsg(uint16_t msgtype,
	void *pvmsgbuf, int32_t i4TotalLen)
{
	struct sk_buff *skb;
	struct nlmsghdr *nlh;
	uint32_t u4Ret = 0;

	DBGLOG(REQ, INFO, "send netlink msg start\n");
	DBGLOG(INIT, TRACE, "msg len == %d", i4TotalLen);
	skb = nlmsg_new(i4TotalLen, 0);
	if (!skb) {
		DBGLOG(REQ, ERROR, "netlink alloc failed\n");
		return -1;
	}

	nlh = nlmsg_put(skb, 0, 0, NLMSG_DONE, i4TotalLen, 0);
	if (nlh == NULL) {
		DBGLOG(REQ, ERROR, "netlink msg put failed!\n");
		kfree_skb(skb);
		return -1;
	}

	NETLINK_CB(skb).dst_group = 5;
	NETLINK_CB(skb).portid = 0;
	nlh->nlmsg_type = msgtype;
	memcpy(NLMSG_DATA(nlh), pvmsgbuf, i4TotalLen);
	u4Ret = netlink_broadcast(nl_sk, skb, 0, 5, GFP_KERNEL);

	if (u4Ret < 0) {
		DBGLOG(REQ, ERROR,
			"netlink sendmsg failed,msgtype is %x, ret is %d\n",
			msgtype, u4Ret);
		return u4Ret;
	}

	DBGLOG(REQ, INFO, "send netlink msg success!\n");
	return u4Ret;
}

int32_t priv_driver_MulAPAgent_bss_status_report(
					struct net_device *prNetDev,
					char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	int8_t *this_char = NULL;
	uint32_t u4Ret = 0;
	uint8_t ucParam = 0;
	uint8_t ucRoleIdx = 0;
	uint8_t ucBssIdx = 0;
	struct BSS_INFO *prBssInfo = NULL;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	/* get command */
	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %d, apcArgv[0] = %s\n\n", i4Argc, *apcArgv);

	/* get param */
	this_char = kalStrStr(*apcArgv, "=");
	if (!this_char)
		goto error;
	this_char++;
	DBGLOG(REQ, LOUD, "string = %s\n", this_char);
	u4Ret = kalkStrtou8(this_char, 0, &ucParam);
	DBGLOG(REQ, LOUD, "u4Ret = %d, ucParam = %d\n", u4Ret, ucParam);
	if (u4Ret != 0) {
		DBGLOG(INIT, ERROR,
			"iwpriv wlanXX driver %s=[0|1]\n", CMD_BSS_REPORT_INFO);
		goto error;
	}
	/* get Bss Index from ndev */
	if (mtk_Netdev_To_RoleIdx(prGlueInfo,
			prGlueInfo->prP2PInfo[1]->prDevHandler,
			&ucRoleIdx) != 0)
		goto error;
	if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter,
			ucRoleIdx, &ucBssIdx) != WLAN_STATUS_SUCCESS)
		goto error;
	DBGLOG(REQ, INFO, "ucRoleIdx = %d\n", ucRoleIdx);
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);
	if (!prBssInfo) {
		DBGLOG(REQ, WARN, "bss is not active\n");
		goto error;
	}
	p2pFunMulAPAgentBssStatusNotification(prAdapter, prBssInfo);

	return i4BytesWritten;
error:
	return -1;
}

int32_t priv_driver_MulAPAgent_bss_report_info(
					struct net_device *prNetDev,
					char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	int8_t *this_char = NULL;
	uint32_t u4Ret = 0;
	uint8_t ucParam = 0;
	uint8_t ucRoleIdx = 0;
	uint8_t ucBssIdx = 0;
	int32_t i4Ret = 0;
	struct BSS_INFO *prBssInfo = NULL;
	struct T_MULTI_AP_BSS_METRICS_RESP sBssMetricsResp;

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		goto error;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	/* get command */
	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %d, apcArgv[0] = %s\n\n", i4Argc, *apcArgv);

	/* get param */
	this_char = kalStrStr(*apcArgv, "=");
	if (!this_char)
		goto error;
	this_char++;
	DBGLOG(REQ, LOUD, "string = %s\n", this_char);
	u4Ret = kalkStrtou8(this_char, 0, &ucParam);
	DBGLOG(REQ, LOUD, "u4Ret = %d, ucParam = %d\n", u4Ret, ucParam);
	if (u4Ret != 0) {
		DBGLOG(INIT, ERROR,
			"iwpriv wlanXX driver %s=[0|1]\n", CMD_BSS_REPORT_INFO);
		goto error;
	}

	/* get Bss Index from ndev */
	if (mtk_Netdev_To_RoleIdx(prGlueInfo,
			prGlueInfo->prP2PInfo[1]->prDevHandler,
			&ucRoleIdx) != 0)
		goto error;
	if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter,
			ucRoleIdx, &ucBssIdx) != WLAN_STATUS_SUCCESS)
		goto error;
	DBGLOG(REQ, INFO, "ucRoleIdx = %d\n", ucRoleIdx);

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);
	if (!prBssInfo) {
		DBGLOG(REQ, WARN, "bss is not active\n");
		goto error;
	}

	schedule_delayed_work(
		&prAdapter->prGlueInfo->rChanNoiseControlWork, 0);


	/* 1. BSS Measurement */
	/* Interface Index */
	i4Ret = sscanf(prGlueInfo->prP2PInfo[1]->prDevHandler->name,
		"ap%u", &sBssMetricsResp.uIfIndex);
	if (i4Ret != 1)
		DBGLOG(P2P, WARN, "read sap index fail: %d\n", i4Ret);

	COPY_MAC_ADDR(sBssMetricsResp.mBssid, prBssInfo->aucBSSID);
	sBssMetricsResp.u8Channel = prBssInfo->ucPrimaryChannel;
	sBssMetricsResp.u16AssocStaNum =
		bssGetClientCount(prAdapter, prBssInfo);
	sBssMetricsResp.u8ChanUtil = prBssInfo->u4ChanUtil;
	sBssMetricsResp.iChanNoise = prBssInfo->i4NoiseHistogram;

	DBGLOG(REQ, INFO,
		"[SAP_Test] uIfIndex = %u\n", sBssMetricsResp.uIfIndex);
	DBGLOG(REQ, INFO,
		"[SAP_Test] mBssid = " MACSTR "\n",
		MAC2STR(sBssMetricsResp.mBssid));
	DBGLOG(REQ, INFO,
		"[SAP_Test] u8Channel = %d\n", sBssMetricsResp.u8Channel);
	DBGLOG(REQ, INFO,
		"[SAP_Test] u16AssocStaNum = %d\n",
		sBssMetricsResp.u16AssocStaNum);
	DBGLOG(REQ, INFO,
		"[SAP_Test] u8ChanUtil = %d\n", sBssMetricsResp.u8ChanUtil);
	DBGLOG(REQ, INFO,
		"[SAP_Test] iChanNoise = %d\n", sBssMetricsResp.iChanNoise);

	i4Ret = MulAPAgentMontorSendMsg(
		EV_WLAN_MULTIAP_BSS_METRICS_RESPONSE,
		&sBssMetricsResp, sizeof(sBssMetricsResp));
	if (i4Ret < 0) {
		DBGLOG(AAA, ERROR,
			"EV_WLAN_MULTIAP_BSS_METRICS_RESPONSE nl send msg failed!\n");
		return i4Ret;
	}

	return i4BytesWritten;

error:
	return -1;
}

int32_t priv_driver_MulAPAgent_sta_report_info(
					struct net_device *prNetDev,
					char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int32_t i4Ret = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	int8_t *this_char = NULL;
	uint8_t aucMacAddr[MAC_ADDR_LEN];
	uint8_t ucRoleIdx = 0;
	uint8_t ucBssIdx = 0;
	struct BSS_INFO *prBssInfo = NULL;
	struct STA_RECORD *prStaRec = NULL;
	u_int32_t txmode, rate, frmode, sgi, nsts, groupid;
	u_int32_t au4RxV[2] = {0};
	u_int16_t u2RateCode = 0;
	u_int32_t u4BufLen = 0;
	u_int8_t ucWlanIndex, i;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	struct PARAM_GET_STA_STATISTICS *prQueryStaStatistics = NULL;
	struct PARAM_HW_WLAN_INFO *prHwWlanInfo = NULL;
	struct T_MULTI_AP_STA_ASSOC_METRICS_RESP *sStaAssocMetricsResp = NULL;

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE) {
		i4BytesWritten = -1;
		goto exit;
	}
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	/* get command */
	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD,
		"argc is %d, apcArgv[0] = %s\n\n", i4Argc, *apcArgv);

	/* get param */
	this_char = kalStrStr(*apcArgv, "=");
	if (!this_char) {
		i4BytesWritten = -1;
		goto exit;
	}
	this_char++;
	DBGLOG(REQ, LOUD, "string = %s\n", this_char);
	i4Ret = wlanHwAddrToBin(this_char, &aucMacAddr[0]);
	if (i4Ret < 0) {
		DBGLOG(INIT, ERROR,
				"iwpriv wlanXX driver %s=STA_MAC\n",
				CMD_STA_REPORT_INFO);
		i4BytesWritten = -1;
		goto exit;
	}

	/* get Bss Index from ndev */
	if (mtk_Netdev_To_RoleIdx(prGlueInfo,
			prGlueInfo->prP2PInfo[1]->prDevHandler,
			&ucRoleIdx) != 0) {
		i4BytesWritten = -1;
		goto exit;
	}
	if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter,
			ucRoleIdx, &ucBssIdx) != WLAN_STATUS_SUCCESS) {
		i4BytesWritten = -1;
		goto exit;
	}
	DBGLOG(REQ, INFO, "ucRoleIdx = %d\n", ucRoleIdx);
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);
	if (!prBssInfo) {
		DBGLOG(REQ, WARN, "bss is not active\n");
		i4BytesWritten = -1;
		goto exit;
	}

	/* get Station Record */
	prStaRec = bssGetClientByMac(prGlueInfo->prAdapter,
		prBssInfo, aucMacAddr);
	if (prStaRec == NULL) {
		DBGLOG(REQ, WARN, "can't find station\n");
		i4BytesWritten = -1;
		goto exit;
	}

	/* Get WTBL info */
	prHwWlanInfo = (struct PARAM_HW_WLAN_INFO *)
		kalMemAlloc(sizeof(struct PARAM_HW_WLAN_INFO),
		VIR_MEM_TYPE);
	if (!prHwWlanInfo) {
		DBGLOG(REQ, ERROR,
			"Allocate memory for prHwWlanInfo failed!\n");
		i4BytesWritten = -1;
		goto exit;
	}

	prHwWlanInfo->u4Index = prStaRec->ucWlanIndex;
	rStatus = kalIoctl(prGlueInfo, wlanoidQueryWlanInfo, prHwWlanInfo,
				sizeof(struct PARAM_HW_WLAN_INFO), &u4BufLen);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, ERROR, "Query prHwWlanInfo failed!\n");
		i4BytesWritten = -1;
		goto exit;
	}

	/* Get Statistics info */
	prQueryStaStatistics = (struct PARAM_GET_STA_STATISTICS *)
		kalMemAlloc(sizeof(struct PARAM_GET_STA_STATISTICS),
		VIR_MEM_TYPE);
	if (!prQueryStaStatistics) {
		DBGLOG(REQ, ERROR,
			"Allocate memory for prQueryStaStatistics failed!\n");
		i4BytesWritten = -1;
		goto exit;
	}

	ucWlanIndex = prStaRec->ucWlanIndex;
	COPY_MAC_ADDR(prQueryStaStatistics->aucMacAddr, aucMacAddr);

	DBGLOG(REQ, INFO, "ucWlanIndex = %d\n", ucWlanIndex);
	DBGLOG(REQ, INFO,
		"Query "MACSTR"\n", MAC2STR(prQueryStaStatistics->aucMacAddr));
	rStatus = kalIoctl(prGlueInfo,
				wlanoidQueryStaStatistics, prQueryStaStatistics,
				sizeof(struct PARAM_GET_STA_STATISTICS),
				&u4BufLen);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, ERROR, "Query prQueryStaStatistics failed!\n");
		i4BytesWritten = -1;
		goto exit;
	}

	sStaAssocMetricsResp = (struct T_MULTI_AP_STA_ASSOC_METRICS_RESP *)
			kalMemAlloc(sizeof(
				struct T_MULTI_AP_STA_ASSOC_METRICS_RESP),
			VIR_MEM_TYPE);
	if (sStaAssocMetricsResp == NULL) {
		DBGLOG(INIT, ERROR, "alloc memory fail\n");
		i4BytesWritten = -1;
		goto exit;
	}

	/*2. Associated STA Measurement */
	/* Interface Index */
	i4Ret = sscanf(prGlueInfo->prP2PInfo[1]->prDevHandler->name,
		"ap%u", &sStaAssocMetricsResp->uIfIndex);
	if (i4Ret != 1)
		DBGLOG(P2P, WARN, "read sap index fail: %d\n", i4Ret);

	prRxV = prStaRec->au4RxV;
	COPY_MAC_ADDR(sStaAssocMetricsResp->mBssid, prBssInfo->aucBSSID);
	COPY_MAC_ADDR(sStaAssocMetricsResp->mStaMac, prStaRec->aucMacAddr);
	sStaAssocMetricsResp->uBytesSent = prStaRec->u8TotalTxBytes;
	sStaAssocMetricsResp->uBytesRecv = prStaRec->u8TotalRxBytes;
	sStaAssocMetricsResp->uPktsSent =
		prQueryStaStatistics->u4TransmitCount;
	sStaAssocMetricsResp->uPktsRecv = prStaRec->u8TotalRxPkts;
	sStaAssocMetricsResp->uPktsTxError =
		prQueryStaStatistics->u4TransmitFailCount;
	sStaAssocMetricsResp->uPktsRxError = 0;
	sStaAssocMetricsResp->uRetransCnt = 0;
	sStaAssocMetricsResp->iRssi =
		RCPI_TO_dBm((prRxV[3] & RX_VT_RCPI0_MASK)
		>> RX_VT_RCPI0_OFFSET);

	/* TxPhyRate */
	for (i = 0; i < AUTO_RATE_NUM; i++) {
		txmode =
			HW_TX_RATE_TO_MODE(
			prHwWlanInfo->rWtblRateInfo.au2RateCode[i]);
		if (txmode >= ENUM_TX_MODE_NUM)
			txmode = ENUM_TX_MODE_NUM - 1;

		rate =
			HW_TX_RATE_TO_MCS(
			prHwWlanInfo->rWtblRateInfo.au2RateCode[i]);
		nsts =
			HW_TX_RATE_TO_NSS(
			prHwWlanInfo->rWtblRateInfo.au2RateCode[i]) + 1;
		frmode = prHwWlanInfo->rWtblPeerCap.ucFrequencyCapability;
		sgi = priv_driver_get_sgi_info(&prHwWlanInfo->rWtblPeerCap);

		if (prHwWlanInfo->rWtblRateInfo.ucRateIdx != i)
			continue;
		else {
			u2RateCode = nicRateInfo2RateCode(txmode, rate);
			if (u2RateCode > 0) {
				sStaAssocMetricsResp->uPhyTxRate =
				nicRateCode2PhyRate(u2RateCode,
					frmode + 4, sgi,
					nsts - 1) / 10;
			} else {
				DBGLOG(REQ, WARN, "convert RateInfo fail\n");
				sStaAssocMetricsResp->uPhyTxRate = 0;
			}
		}
	}

	/* RxPhyRate */
	au4RxV[0] = prRxV[0];
	au4RxV[1] = prRxV[1];

	txmode = (au4RxV[0] & RX_VT_RX_MODE_MASK) >> RX_VT_RX_MODE_OFFSET;
	rate = (au4RxV[0] & RX_VT_RX_RATE_MASK) >> RX_VT_RX_RATE_OFFSET;
	frmode = (au4RxV[0] & RX_VT_FR_MODE_MASK) >> RX_VT_FR_MODE_OFFSET;
	nsts = (au4RxV[1] & RX_VT_NSTS_MASK) >> RX_VT_NSTS_OFFSET;
	sgi = (au4RxV[0] & RX_VT_SHORT_GI) >> 19;
	groupid = (au4RxV[1] & RX_VT_GROUP_ID_MASK) >> RX_VT_GROUP_ID_OFFSET;

	if (!(groupid && groupid != 63))
		nsts += 1;

	u2RateCode = nicRateInfo2RateCode(txmode, rate);
	if (u2RateCode > 0) {
		sStaAssocMetricsResp->uPhyRxRate =
			nicRateCode2PhyRate(u2RateCode,
			frmode + 4, sgi, nsts - 1) / 10;
	} else {
		DBGLOG(REQ, WARN, "convert RateInfo fail\n");
		sStaAssocMetricsResp->uPhyRxRate = 0;
	}

	sStaAssocMetricsResp->uAssocRate = 0;
	sStaAssocMetricsResp->uDeltaTime = kalGetTimeTick()
		- prStaRec->u8GetDataRateTime;

	DBGLOG(REQ, INFO,
		"[SAP_Test] uIfIndex = %u\n",
		sStaAssocMetricsResp->uIfIndex);
	DBGLOG(REQ, INFO,
		"[SAP_Test] mBssid = " MACSTR "\n",
		MAC2STR(sStaAssocMetricsResp->mBssid));
	DBGLOG(REQ, INFO,
		"[SAP_Test] mStaMac = " MACSTR "\n",
		MAC2STR(sStaAssocMetricsResp->mStaMac));
	DBGLOG(REQ, INFO,
		"[SAP_Test] uBytesSent = %llu\n",
		sStaAssocMetricsResp->uBytesSent);
	DBGLOG(REQ, INFO,
		"[SAP_Test] uBytesRecv = %llu\n",
		sStaAssocMetricsResp->uBytesRecv);
	DBGLOG(REQ, INFO,
		"[SAP_Test] uPktsSent = %llu\n",
		sStaAssocMetricsResp->uPktsSent);
	DBGLOG(REQ, INFO,
		"[SAP_Test] uPktsRecv = %llu\n",
		sStaAssocMetricsResp->uPktsRecv);
	DBGLOG(REQ, INFO,
		"[SAP_Test] uPktsTxError = %llu\n",
		sStaAssocMetricsResp->uPktsTxError);
	DBGLOG(REQ, INFO,
		"[SAP_Test] uPktsRxError = %u\n",
		sStaAssocMetricsResp->uPktsRxError);
	DBGLOG(REQ, INFO,
		"[SAP_Test] uRetransCnt = %u\n",
		sStaAssocMetricsResp->uRetransCnt);
	DBGLOG(REQ, INFO,
		"[SAP_Test] iRssi = %d\n",
		sStaAssocMetricsResp->iRssi);
	DBGLOG(REQ, INFO,
		"[SAP_Test] uPhyTxRate = %u\n",
		sStaAssocMetricsResp->uPhyTxRate);
	DBGLOG(REQ, INFO,
		"[SAP_Test] uPhyRxRate = %u\n",
		sStaAssocMetricsResp->uPhyRxRate);
	DBGLOG(REQ, INFO,
		"[SAP_Test] uAssocRate = %u\n",
		sStaAssocMetricsResp->uAssocRate);
	DBGLOG(REQ, INFO,
		"[SAP_Test] uDeltaTime = %u\n",
		sStaAssocMetricsResp->uDeltaTime);

	i4Ret = MulAPAgentMontorSendMsg(
		EV_WLAN_MULTIAP_ASSOC_STA_METRICS_RESPONSE,
		sStaAssocMetricsResp, sizeof(*sStaAssocMetricsResp));
	if (i4Ret < 0) {
		DBGLOG(AAA, ERROR,
			"EV_WLAN_MULTIAP_ASSOC_STA_METRICS_RESPONSE nl send msg failed!\n");
		return i4Ret;
	}
	kalMemFree(sStaAssocMetricsResp,
		VIR_MEM_TYPE,
		sizeof(struct T_MULTI_AP_STA_ASSOC_METRICS_RESP));

exit:
	if (prHwWlanInfo)
		kalMemFree(prHwWlanInfo,
			VIR_MEM_TYPE,
			sizeof(PARAM_HW_WLAN_INFO));

	if (prQueryStaStatistics)
		kalMemFree(prQueryStaStatistics,
			VIR_MEM_TYPE,
			sizeof(PARAM_GET_STA_STATISTICS));

	return i4BytesWritten;
}

int32_t priv_driver_MulAPAgent_sta_measurement_control(
					struct net_device *prNetDev,
					char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	int8_t *this_char = NULL;
	uint32_t u4Ret = 0;
	uint8_t ucParam = 0;
	uint8_t ucRoleIdx = 0;
	uint8_t ucBssIdx = 0;
	struct BSS_INFO *prBssInfo = NULL;
	uint16_t ucMeasureDuration = 0;

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		goto error;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	/* get command */
	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %d, apcArgv[0] = %s\n\n", i4Argc, *apcArgv);

	/* get param */
	this_char = kalStrStr(*apcArgv, "=");
	if (!this_char)
		goto error;
	this_char++;
	DBGLOG(REQ, LOUD, "string = %s\n", this_char);
	u4Ret = kalkStrtou8(this_char, 0, &ucParam);
	DBGLOG(REQ, LOUD, "u4Ret = %d, ucParam = %d\n", u4Ret, ucParam);
	if (u4Ret != 0) {
		DBGLOG(INIT, ERROR,
			"iwpriv wlanXX driver %s=[0|1] duration(ms)\n",
			CMD_STA_MEASUREMENT_ENABLE);
		goto error;
	}

	if (ucParam) {
		if (apcArgv[1]) {
			u4Ret = kalkStrtou16(apcArgv[1], 0, &ucMeasureDuration);
			if (u4Ret != 0) {
				DBGLOG(REQ, ERROR,
					"parse ucMeasureDuration error u4Ret=%d\n",
					u4Ret);
				goto error;
			}
		} else {
			DBGLOG(REQ, ERROR, "ucMeasureDuration is NULL\n");
			goto error;
		}
	}

	/* get Bss Index from ndev */
	if (mtk_Netdev_To_RoleIdx(prGlueInfo,
			prGlueInfo->prP2PInfo[1]->prDevHandler,
			&ucRoleIdx) != 0)
		goto error;
	if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter,
			ucRoleIdx, &ucBssIdx) != WLAN_STATUS_SUCCESS)
		goto error;
	DBGLOG(REQ, INFO, "ucRoleIdx = %d\n", ucRoleIdx);
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);
	if (!prBssInfo) {
		DBGLOG(REQ, WARN, "bss is not active\n");
		goto error;
	}

	/* direct probe req to host or FW */
	snprintf(pcCommand, i4TotalLen, "%s %s %d",
		CMD_SET_CHIP, "N9ProbReq2Host", ucParam);
	priv_driver_set_chip_config(prNetDev, pcCommand, i4TotalLen);

	if (ucParam) {
		DBGLOG(REQ, INFO, "Unassociated STA Measurement start...\n");

		if (timerPendingTimer(&prBssInfo->rUnassocStaMeasureTimer)) {
			cnmTimerStopTimer(prAdapter,
				&prBssInfo->rUnassocStaMeasureTimer);
			DBGLOG(REQ, INFO,
				"update UnassocStaMeasureTimer\n");
		}

		cnmTimerInitTimer(prAdapter,
			&prBssInfo->rUnassocStaMeasureTimer,
			(PFN_MGMT_TIMEOUT_FUNC)
				aaaMulAPAgentUnassocStaMeasureTimeout,
			(unsigned long) prBssInfo);

		cnmTimerStartTimer(prAdapter,
			&prBssInfo->rUnassocStaMeasureTimer, ucMeasureDuration);
		DBGLOG(REQ, INFO,
			"ucMeasureDuration = %d ms\n", ucMeasureDuration);

	} else {
		/* reset Rx filter */
		nicUpdateBss(prAdapter, prBssInfo->ucBssIndex);

		kalMemZero(prBssInfo->arUnAssocSTA,
			sizeof(struct T_MULTI_AP_STA_UNASSOC_METRICS)
			* SAP_UNASSOC_METRICS_STA_MAX);
		DBGLOG(REQ, INFO, "Unassociated STA Measurement stop...\n");
	}

	return i4BytesWritten;

error:
	return -1;
}

int32_t priv_driver_MulAPAgent_sta_measurement_info(
					struct net_device *prNetDev,
					char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int32_t i4Ret = 0;
	uint32_t u4Ret = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	int8_t *this_char = NULL;
	uint8_t aucMacAddr[MAC_ADDR_LEN];
	uint8_t ucRoleIdx = 0;
	uint8_t ucBssIdx = 0;
	uint8_t ucMeasureIdx = 0;
	struct BSS_INFO *prBssInfo = NULL;

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE) {
		i4BytesWritten = -1;
		goto exit;
	}
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	/* get command */
	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD,
		"argc is %d, apcArgv[0] = %s\n\n", i4Argc, *apcArgv);

	if (i4Argc != 2) {
		DBGLOG(REQ, ERROR,
			"argc %i is not equal to 2\n", i4Argc);
		i4BytesWritten = -1;
		goto exit;
	}

	/* get param */
	this_char = kalStrStr(*apcArgv, "=");
	if (!this_char) {
		i4BytesWritten = -1;
		goto exit;
	}
	this_char++;
	DBGLOG(REQ, LOUD, "string = %s\n", this_char);
	i4Ret = wlanHwAddrToBin(this_char, &aucMacAddr[0]);
	if (i4Ret < 0) {
		DBGLOG(INIT, ERROR,
			"iwpriv wlanXX driver %s=STA_MAC [IDX]\n",
			CMD_STA_MEASUREMENT_INFO);
		i4BytesWritten = -1;
		goto exit;
	}

	u4Ret = kalkStrtou8(apcArgv[1], 0, &ucMeasureIdx);
	if (u4Ret != 0) {
		DBGLOG(REQ, LOUD,
			"parse ucMeasureIdx error u4Ret=%d\n", u4Ret);
		i4BytesWritten = -1;
		goto exit;
	}

	if (ucMeasureIdx > 15 || ucMeasureIdx < 0) {
		DBGLOG(REQ, ERROR, "ucMeasureIdx number error\n");
		i4BytesWritten = -1;
		goto exit;
	}

	/* get Bss Index from ndev */
	if (mtk_Netdev_To_RoleIdx(prGlueInfo,
			prGlueInfo->prP2PInfo[1]->prDevHandler,
			&ucRoleIdx) != 0) {
		i4BytesWritten = -1;
		goto exit;
	}
	if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter,
			ucRoleIdx, &ucBssIdx) != WLAN_STATUS_SUCCESS) {
		i4BytesWritten = -1;
		goto exit;
	}
	DBGLOG(REQ, INFO, "ucRoleIdx = %d\n", ucRoleIdx);
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);
	if (!prBssInfo) {
		DBGLOG(REQ, WARN, "bss is not active\n");
		i4BytesWritten = -1;
		goto exit;
	}

	COPY_MAC_ADDR(prBssInfo->arUnAssocSTA[ucMeasureIdx].mStaMac,
		aucMacAddr);
	DBGLOG(REQ, INFO,
		"[SAP_Test] "MACSTR" IDX = %d\n",
		MAC2STR(aucMacAddr), ucMeasureIdx);

exit:
	return i4BytesWritten;
}

int32_t priv_driver_MulAPAgent_set_allow_sta(
					struct net_device *prNetDev,
					char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4Ret = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint8_t *this_char = NULL;
	int i = 0;
	uint8_t aucMacAddr[MAC_ADDR_LEN];

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE) {
		i4Ret = -1;
		goto exit;
	}
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	/* get command */
	DBGLOG(INIT, INFO, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(INIT, INFO,
		"argc is %d, apcArgv[0] = %s\n\n",
		i4Argc, *apcArgv);

	/* get param */
	this_char = kalStrStr(*apcArgv, "=");
	if (!this_char) {
		i4Ret = -1;
		goto exit;
	}

	this_char++;
	i4Ret = sscanf(this_char,
		"%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
		&aucMacAddr[0], &aucMacAddr[1],
		&aucMacAddr[2], &aucMacAddr[3],
		&aucMacAddr[4], &aucMacAddr[5]);
	DBGLOG(INIT, INFO, "thisChar=%s\n", this_char);
	DBGLOG(INIT, INFO,
		"Removing MAC="MACSTR" from BlockList !!\n",
		MAC2STR(aucMacAddr));
	for (i = 0; i < KAL_P2P_NUM; i++) {
		DBGLOG(INIT, INFO,
			"Removing MAC="MACSTR
			" from BlockList !! P2P NUM=%d\n",
			MAC2STR(aucMacAddr), i);
		i4Ret |= kalP2PSetBlockList(prGlueInfo,
			aucMacAddr, 0, i);
	}
exit:
	return i4Ret;
}

int32_t priv_driver_MulAPAgent_set_block_sta(
					struct net_device *prNetDev,
					char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4Ret = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint8_t *this_char = NULL;
	int i = 0;
	uint8_t aucMacAddr[MAC_ADDR_LEN];
	struct ADAPTER *prAdapter = NULL;

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE) {
		i4Ret = -1;
		goto exit;
	}
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	/* get command */
	DBGLOG(INIT, INFO, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(INIT, INFO, "argc is %d, apcArgv[0] = %s\n", i4Argc, *apcArgv);

	/* get param */
	this_char = kalStrStr(*apcArgv, "=");
	if (!this_char) {
		i4Ret = -1;
		goto exit;
	}
	this_char++;

	i4Ret = sscanf(this_char,
		"%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
		&aucMacAddr[0], &aucMacAddr[1],
		&aucMacAddr[2], &aucMacAddr[3],
		&aucMacAddr[4], &aucMacAddr[5]);
	DBGLOG(INIT, INFO, "thisChar=%s\n", this_char);
	DBGLOG(INIT, INFO,
		"Adding MAC="MACSTR" to BlockList !!\n",
		MAC2STR(aucMacAddr));

	for (i = 0; i < KAL_P2P_NUM; i++) {
		DBGLOG(INIT, INFO,
			"Adding MAC="MACSTR" to BlockList !! P2P NUM=%d\n",
			MAC2STR(aucMacAddr), i);
		i4Ret |= kalP2PSetBlockList(prGlueInfo, aucMacAddr, 1, i);
	}
exit:
	return i4Ret;
}
#endif /* CFG_AP_80211KVR_INTERFACE */

#if CFG_AP_80211K_SUPPORT
int32_t priv_driver_MulAPAgent_beacon_report_request(
					struct net_device *prNetDev,
					char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	uint32_t u4Ret = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	int8_t *this_char = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint8_t i;
	uint8_t ucTmpReqElemNum = 0;
	struct PARAM_CUSTOM_BCN_REP_REQ_STRUCT *prSetBcnRepReqInfo = NULL;

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE) {
		i4BytesWritten = -1;
		goto exit;
	}
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	/* get command */
	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD,
		"argc is %d, apcArgv[0] = %s\n",
		i4Argc, *apcArgv);

	/* get param */
	this_char = kalStrStr(*apcArgv, "=");
	if (!this_char) {
		i4BytesWritten = -1;
		goto exit;
	}
	this_char++;
	DBGLOG(REQ, LOUD, "string = %s\n", this_char);
	prSetBcnRepReqInfo = (struct PARAM_CUSTOM_BCN_REP_REQ_STRUCT *)
			kalMemAlloc(sizeof(
			struct PARAM_CUSTOM_BCN_REP_REQ_STRUCT),
			VIR_MEM_TYPE);
	if (prSetBcnRepReqInfo == NULL) {
		DBGLOG(INIT, ERROR, "alloc memory fail\n");
		i4BytesWritten = -1;
		goto exit;
	}
	kalMemZero(prSetBcnRepReqInfo,
		sizeof(struct PARAM_CUSTOM_BCN_REP_REQ_STRUCT));
#define TEMP_TEMPLATE \
	"%hhx:%hhx:%hhx:%hhx:%hhx:%hhx-%u-%u-%u-"\
	"%hhx:%hhx:%hhx:%hhx:%hhx:%hhx-%u-%u-%u-%u-%u-%u-%u-%u"\
	"-%" INT_TO_STR(PARAM_MAX_LEN_SSID) "s"

	i4BytesWritten = sscanf(this_char, TEMP_TEMPLATE,
			&prSetBcnRepReqInfo->aucPeerMac[0],
			&prSetBcnRepReqInfo->aucPeerMac[1],
			&prSetBcnRepReqInfo->aucPeerMac[2],
			&prSetBcnRepReqInfo->aucPeerMac[3],
			&prSetBcnRepReqInfo->aucPeerMac[4],
			&prSetBcnRepReqInfo->aucPeerMac[5],
			&prSetBcnRepReqInfo->u2Repetition,
			&prSetBcnRepReqInfo->u2MeasureDuration,
			&prSetBcnRepReqInfo->ucOperClass,
			&prSetBcnRepReqInfo->aucBssid[0],
			&prSetBcnRepReqInfo->aucBssid[1],
			&prSetBcnRepReqInfo->aucBssid[2],
			&prSetBcnRepReqInfo->aucBssid[3],
			&prSetBcnRepReqInfo->aucBssid[4],
			&prSetBcnRepReqInfo->aucBssid[5],
			&prSetBcnRepReqInfo->ucChannel,
			&prSetBcnRepReqInfo->u2RandomInterval,
			&prSetBcnRepReqInfo->ucMeasurementMode,
			&prSetBcnRepReqInfo->ucReportCondition,
			&prSetBcnRepReqInfo->ucReportReference,
			&prSetBcnRepReqInfo->ucReportingDetail,
			&prSetBcnRepReqInfo->ucNumberOfRequest,
			&prSetBcnRepReqInfo->ucNumberOfAPChanReport,
			&prSetBcnRepReqInfo->aucSsid);
#undef TEMP_TEMPLATE

	DBGLOG(REQ, INFO,
		"[SAP_Test] aucPeerMac = " MACSTR"\n",
		MAC2STR(prSetBcnRepReqInfo->aucPeerMac));
	DBGLOG(REQ, INFO,
		"[SAP_Test] u2Repetition = %u\n",
		prSetBcnRepReqInfo->u2Repetition);
	DBGLOG(REQ, INFO,
		"[SAP_Test] u2MeasureDuration = %u\n",
		prSetBcnRepReqInfo->u2MeasureDuration);
	DBGLOG(REQ, INFO,
		"[SAP_Test] ucOperClass = %u\n",
		prSetBcnRepReqInfo->ucOperClass);
	DBGLOG(REQ, INFO,
		"[SAP_Test] aucBssid = " MACSTR "\n",
		MAC2STR(prSetBcnRepReqInfo->aucBssid));
	DBGLOG(REQ, INFO,
		"[SAP_Test] ucChannel = %u\n",
		prSetBcnRepReqInfo->ucChannel);
	DBGLOG(REQ, INFO,
		"[SAP_Test] u2RandomInterval = %u\n",
		prSetBcnRepReqInfo->u2RandomInterval);
	DBGLOG(REQ, INFO,
		"[SAP_Test] ucMeasurementMode = %u\n",
		prSetBcnRepReqInfo->ucMeasurementMode);
	DBGLOG(REQ, INFO,
		"[SAP_Test] ucReportCondition = %u\n",
		prSetBcnRepReqInfo->ucReportCondition);
	DBGLOG(REQ, INFO,
		"[SAP_Test] ucReportReference = %u\n",
		prSetBcnRepReqInfo->ucReportReference);
	DBGLOG(REQ, INFO,
		"[SAP_Test] ucReportingDetail = %u\n",
		prSetBcnRepReqInfo->ucReportingDetail);
	DBGLOG(REQ, INFO,
		"[SAP_Test] ucNumberOfRequest = %u\n",
		prSetBcnRepReqInfo->ucNumberOfRequest);
	DBGLOG(REQ, INFO,
		"[SAP_Test] ucNumberOfAPChanReport = %u\n",
		prSetBcnRepReqInfo->ucNumberOfAPChanReport);
	DBGLOG(REQ, INFO,
		"[SAP_Test] aucSsid = %s\n", prSetBcnRepReqInfo->aucSsid);

	if (i4Argc != prSetBcnRepReqInfo->ucNumberOfRequest +
		prSetBcnRepReqInfo->ucNumberOfAPChanReport + 1) {
		DBGLOG(INIT, ERROR,
			"read Request Element and AP Channel List fail !!\n");
		i4BytesWritten = -1;
		goto exit;
	}

	ucTmpReqElemNum = prSetBcnRepReqInfo->ucNumberOfRequest;

	if (prSetBcnRepReqInfo->ucReportingDetail != 1) {
		prSetBcnRepReqInfo->ucNumberOfRequest = 0;
		kalMemZero(prSetBcnRepReqInfo->ucRequestElemList,
			sizeof(uint8_t) * ELEM_LEN_MAX);
	} else {
		for (i = 1;
			i < prSetBcnRepReqInfo->ucNumberOfRequest + 1;
			i++) {
			u4Ret = kalkStrtou8(apcArgv[i],
				0, &prSetBcnRepReqInfo->ucRequestElemList[i-1]);
			if (u4Ret != 0) {
				DBGLOG(INIT, ERROR,
					"parse Request Element List fail\n");
				i4BytesWritten = -1;
				goto exit;
			}
			DBGLOG(REQ, INFO,
				"[SAP_Test] ucRequestElemList[%d] = %u\n", i-1,
				prSetBcnRepReqInfo->ucRequestElemList[i-1]);
		}
	}

	if (prSetBcnRepReqInfo->ucChannel != 255) {
		prSetBcnRepReqInfo->ucNumberOfAPChanReport = 0;
		kalMemZero(prSetBcnRepReqInfo->ucChanList,
			sizeof(uint8_t) * MAX_CHN_NUM);
	} else {
		for (i = ucTmpReqElemNum + 1; i < i4Argc; i++) {
			u4Ret = kalkStrtou8(apcArgv[i], 0,
				&prSetBcnRepReqInfo
				->ucChanList[i-(ucTmpReqElemNum + 1)]);
			if (u4Ret != 0) {
				DBGLOG(INIT, ERROR,
					"parse AP Channel List fail\n");
				i4BytesWritten = -1;
				goto exit;
			}
			DBGLOG(REQ, INFO,
				"[SAP_Test] ucChanList[%d] = %u\n",
				i-(ucTmpReqElemNum + 1),
				prSetBcnRepReqInfo
				->ucChanList[i-(ucTmpReqElemNum + 1)]);
		}
	}

	/*4. Beacon Report */
	rStatus = kalIoctl(prGlueInfo, wlanoidSendBeaconReportRequest,
				prSetBcnRepReqInfo,
				sizeof(struct PARAM_CUSTOM_BCN_REP_REQ_STRUCT),
				&i4BytesWritten);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, ERROR, "ERR: kalIoctl fail (%d)\r\n", rStatus);
		i4BytesWritten = -1;
		goto exit;
	}

exit:
	if (prSetBcnRepReqInfo)
		kalMemFree(prSetBcnRepReqInfo,
		VIR_MEM_TYPE,
		sizeof(struct PARAM_CUSTOM_BCN_REP_REQ_STRUCT));
	return i4BytesWritten;
}
#endif /* CFG_AP_80211K_SUPPORT */
#if CFG_AP_80211V_SUPPORT
int32_t priv_driver_MulAPAgent_BTM_request(
					struct net_device *prNetDev,
					char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4Ret = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	int8_t *this_char = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint8_t i;
	struct PARAM_CUSTOM_BTM_REQ_STRUCT *prSetBtmReqInfo = NULL;

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE) {
		i4Ret = -1;
		goto exit;
	}
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	/* get command */
	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD,
		"argc is %d, apcArgv[0] = %s\n\n", i4Argc, *apcArgv);

	/* get param */
	this_char = kalStrStr(*apcArgv, "=");
	if (!this_char) {
		i4Ret = -1;
		goto exit;
	}
	this_char++;
	DBGLOG(REQ, LOUD, "string = %s\n", this_char);
	prSetBtmReqInfo = (struct PARAM_CUSTOM_BTM_REQ_STRUCT *)
			kalMemAlloc(sizeof(struct PARAM_CUSTOM_BTM_REQ_STRUCT),
			VIR_MEM_TYPE);
	if (prSetBtmReqInfo == NULL) {
		DBGLOG(INIT, ERROR, "alloc memory fail\n");
		i4Ret = -1;
		goto exit;
	}
	kalMemZero(prSetBtmReqInfo, sizeof(struct PARAM_CUSTOM_BTM_REQ_STRUCT));

#define TEMP_TEMPLATE \
	"%hhx:%hhx:%hhx:%hhx:%hhx:%hhx-%u-%u-%u-%u-%u-%255s"

	i4Ret = sscanf(this_char, TEMP_TEMPLATE,
		&prSetBtmReqInfo->aucPeerMac[0],
		&prSetBtmReqInfo->aucPeerMac[1],
		&prSetBtmReqInfo->aucPeerMac[2],
		&prSetBtmReqInfo->aucPeerMac[3],
		&prSetBtmReqInfo->aucPeerMac[4],
		&prSetBtmReqInfo->aucPeerMac[5],
		&prSetBtmReqInfo->ucEssImm,
		&prSetBtmReqInfo->u2DisassocTimer,
		&prSetBtmReqInfo->ucAbridged,
		&prSetBtmReqInfo->ucValidityInterval,
		&prSetBtmReqInfo->ucTargetBSSIDCnt,
		(char *) &prSetBtmReqInfo->aucSessionUrl);
#undef TEMP_TEMPLATE

	DBGLOG(REQ, INFO,
		"[SAP_Test] aucPeerMac = " MACSTR "\n",
		MAC2STR(prSetBtmReqInfo->aucPeerMac));
	DBGLOG(REQ, INFO,
		"[SAP_Test] u4EssImm = %u\n",
		prSetBtmReqInfo->ucEssImm);
	DBGLOG(REQ, INFO,
		"[SAP_Test] u2DisassocTimer = %u\n",
		prSetBtmReqInfo->u2DisassocTimer);
	DBGLOG(REQ, INFO,
		"[SAP_Test] ucAbridged = %u\n",
		prSetBtmReqInfo->ucAbridged);
	DBGLOG(REQ, INFO,
		"[SAP_Test] ucValidityInterval = %u\n",
		prSetBtmReqInfo->ucValidityInterval);
	DBGLOG(REQ, INFO,
		"[SAP_Test] ucTargetBSSIDCnt = %u\n",
		prSetBtmReqInfo->ucTargetBSSIDCnt);
	DBGLOG(REQ, INFO,
		"[SAP_Test] aucSessionUrl = %s\n",
		prSetBtmReqInfo->aucSessionUrl);

	if (prSetBtmReqInfo->ucTargetBSSIDCnt < 1
		|| prSetBtmReqInfo->ucTargetBSSIDCnt > DEST_BSSID_NUM_MAX) {
		DBGLOG(INIT, ERROR,
			"Candidate BSSID number invalid, skip send BTM Req\n");
		i4Ret = -1;
		goto exit;
	}

	if (i4Argc != prSetBtmReqInfo->ucTargetBSSIDCnt + 1) {
		DBGLOG(INIT, ERROR,
			"Read Candicate BSSID List Fail, count not match !!!\n");
		i4Ret = -1;
		goto exit;
	}

	for (i = 1; i < i4Argc; i++) {
		i4Ret = sscanf(apcArgv[i],
			"%hhx:%hhx:%hhx:%hhx:%hhx:%hhx-%x-%u-%u-%x-%u",
			&prSetBtmReqInfo->ucTargetBSSIDList[i-1].mMac[0],
			&prSetBtmReqInfo->ucTargetBSSIDList[i-1].mMac[1],
			&prSetBtmReqInfo->ucTargetBSSIDList[i-1].mMac[2],
			&prSetBtmReqInfo->ucTargetBSSIDList[i-1].mMac[3],
			&prSetBtmReqInfo->ucTargetBSSIDList[i-1].mMac[4],
			&prSetBtmReqInfo->ucTargetBSSIDList[i-1].mMac[5],
			&prSetBtmReqInfo->ucTargetBSSIDList[i-1].u4BSSIDInfo,
			&prSetBtmReqInfo->ucTargetBSSIDList[i-1].ucOperClass,
			&prSetBtmReqInfo->ucTargetBSSIDList[i-1].ucChannel,
			&prSetBtmReqInfo->ucTargetBSSIDList[i-1].ucPhyType,
			&prSetBtmReqInfo->ucTargetBSSIDList[i-1].ucPreference);
		if (i4Ret != 11) {
			DBGLOG(INIT, ERROR, "parse Target BSSID List fail\n");
			i4Ret = -1;
			goto exit;
		}

		DBGLOG(REQ, INFO,
			"[SAP_Test] TargetBSSIDList[%u] = " MACSTR"\n",
			i-1,
			MAC2STR(prSetBtmReqInfo->ucTargetBSSIDList[i-1].mMac));
		DBGLOG(REQ, INFO, "[SAP_Test] u4BSSIDInfo = %x\n",
			prSetBtmReqInfo->ucTargetBSSIDList[i-1].u4BSSIDInfo);
		DBGLOG(REQ, INFO, "[SAP_Test] ucOperClass = %u\n",
			prSetBtmReqInfo->ucTargetBSSIDList[i-1].ucOperClass);
		DBGLOG(REQ, INFO, "[SAP_Test] ucChannel = %u\n",
			prSetBtmReqInfo->ucTargetBSSIDList[i-1].ucChannel);
		DBGLOG(REQ, INFO, "[SAP_Test] ucPhyType = %x\n",
			prSetBtmReqInfo->ucTargetBSSIDList[i-1].ucPhyType);
		DBGLOG(REQ, INFO, "[SAP_Test] ucPreference = %u\n",
			prSetBtmReqInfo->ucTargetBSSIDList[i-1].ucPreference);
	}

	/*5. BTM Request */
	rStatus = kalIoctl(prGlueInfo, wlanoidSendBTMRequest, prSetBtmReqInfo,
				sizeof(struct PARAM_CUSTOM_BTM_REQ_STRUCT),
				&i4Ret);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, ERROR, "ERR: kalIoctl fail (%d)\r\n", rStatus);
		i4Ret = -1;
		goto exit;
	}

exit:
	if (prSetBtmReqInfo)
		kalMemFree(prSetBtmReqInfo,
		VIR_MEM_TYPE,
		sizeof(struct PARAM_CUSTOM_BTM_REQ_STRUCT));
	return i4Ret;
}
#endif /* CFG_AP_80211V_SUPPORT */

int priv_driver_get_sleep_cnt_info(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	struct PARAM_SLEEP_CNT_INFO *prSlpCntInfo;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	prSlpCntInfo = (struct PARAM_SLEEP_CNT_INFO *)kalMemAlloc(
			sizeof(struct PARAM_SLEEP_CNT_INFO), VIR_MEM_TYPE);

	if (!prSlpCntInfo) {
		DBGLOG(REQ, ERROR, "mem is null\n");
		return -1;
	}

	rStatus = kalIoctl(prGlueInfo, wlanoidGetSleepCntInfo, prSlpCntInfo,
			  sizeof(struct PARAM_SLEEP_CNT_INFO), &u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, ERROR, "rStatus 0x%8X\n", rStatus);
		i4BytesWritten = -1;
		goto end;
	}

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"\n======== Low Power Sleep Count ========\n");
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"Band0 LMAC Sleep Count \t= %d\n",
				prSlpCntInfo->au4LmacSlpCnt[ENUM_BAND_0]);
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"Band1 LMAC Sleep Count \t= %d\n",
				prSlpCntInfo->au4LmacSlpCnt[ENUM_BAND_1]);
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"WFSYS Sleep Count \t= %d\n",
				prSlpCntInfo->u4WfsysSlpCnt);
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"CBINFRA Sleep Count \t= %d\n",
				prSlpCntInfo->u4CbinfraSlpCnt);
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"Whole Chip Sleep Count \t= %d\n",
				prSlpCntInfo->u4ChipSlpCnt);

end:
	kalMemFree(prSlpCntInfo, VIR_MEM_TYPE,
		   sizeof(struct PARAM_SLEEP_CNT_INFO));

	return i4BytesWritten;
}

int priv_driver_set_lp_keep_pwr_ctrl(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	struct CMD_LP_DBG_CTRL rCmdLp = {0};
	int32_t u4BandIdx;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc < 3)
		goto op_invalid;
	else if (i4Argc == 3) {
		rStatus = kalkStrtos32(apcArgv[1], 0, &u4BandIdx);

		if (rStatus ||
		    (u4BandIdx != ENUM_BAND_0 && u4BandIdx != ENUM_BAND_1)) {
			DBGLOG(REQ, ERROR, "invalid band index, u4Ret=%d\n",
				rStatus);
			i4BytesWritten = -1;
			goto op_invalid;
		}
		rCmdLp.ucBandIdx = u4BandIdx;

		if (strnicmp(apcArgv[2], "PHY+LMAC", strlen("PHY+LMAC")) == 0) {
			i4BytesWritten += kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"\nKEEP PHY+LMAC\n");
			rCmdLp.ucKeepPwr = ENUM_KEEP_PWR_PHY_LMAC;
		} else if (strnicmp(apcArgv[2], "PHY", strlen("PHY")) == 0) {
			i4BytesWritten += kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"\nKEEP PHY\n");
			rCmdLp.ucKeepPwr = ENUM_KEEP_PWR_PHY;
		} else if (strnicmp(apcArgv[2], "LMAC", strlen("LMAC")) == 0) {
			i4BytesWritten += kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"\nKEEP LMAC\n");
			rCmdLp.ucKeepPwr = ENUM_KEEP_PWR_LMAC;
		} else if (strnicmp(apcArgv[2], "RFDIG",
				strlen("RFDIG")) == 0) {
			i4BytesWritten += kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"\nKEEP RFDIG\n");
			rCmdLp.ucKeepPwr = ENUM_KEEP_PWR_RFDIG;
		} else
			goto op_invalid;
	} else
		goto op_invalid;

	rStatus = kalIoctl(prGlueInfo, wlanoidSetLpKeepPwrCtrl, &rCmdLp,
			  sizeof(struct CMD_LP_DBG_CTRL), &u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, ERROR, "rStatus 0x%8X\n", rStatus);
		i4BytesWritten = -1;
		goto end;
	}

	if (rCmdLp.ucKeepPwr == ENUM_KEEP_PWR_RFDIG) {
		if (rCmdLp.ucRfdigStatus == 0)
			i4BytesWritten += kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten, "SUCCESS\n");
		else
			i4BytesWritten += kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten, "FAIL\n");
	} else
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten, "SUCCESS\n");

	return i4BytesWritten;

op_invalid:
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"\nHelp menu [Band0/Band1]\n");
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"\tkeep PHY+LMAC:\t\"set_lp_keep_pwr_ctrl [0/1] PHY+LMAC\"\n");
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"\tkeep PHY:\t\"set_lp_keep_pwr_ctrl [0/1] PHY\"\n");
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"\tkeep LMAC:\t\"set_lp_keep_pwr_ctrl [0/1] LMAC\"\n");
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"\tkeep RFDIG:\t\"set_lp_keep_pwr_ctrl [0/1] RFDIG\"\n");

end:

	return i4BytesWritten;
}

int32_t priv_driver_cmds(struct net_device *prNetDev, int8_t *pcCommand,
			 int32_t i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	uint8_t ucCmdFound = FALSE;
	PRIV_CMD_FUNCTION pfHandler = NULL;

	if (g_u4HaltFlag) {
		DBGLOG(REQ, WARN, "wlan is halt, skip priv_driver_cmds\n");
		return -1;
	}

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	pfHandler = get_priv_cmd_handler(pcCommand, i4TotalLen);
	if (pfHandler != NULL) {
		i4BytesWritten = pfHandler(prNetDev, pcCommand, i4TotalLen);
		ucCmdFound = TRUE;
	}

#if CFG_EXT_FEATURE
	if (!ucCmdFound) {
		pfHandler = get_priv_cmd_ext_handler(
			pcCommand, i4TotalLen);
		if (pfHandler != NULL) {
			i4BytesWritten = pfHandler(prNetDev,
				pcCommand, i4TotalLen);
			ucCmdFound = TRUE;
		}
	}
#endif

	/* Can't find suitable command handler function */
	if (ucCmdFound == FALSE)
		i4BytesWritten = priv_cmd_not_support(prNetDev, pcCommand,
						      i4TotalLen);

	if (i4BytesWritten >= 0) {
		if ((i4BytesWritten == 0) && (i4TotalLen > 0)) {
			/* reset the command buffer */
			pcCommand[0] = '\0';
		}

		if (i4BytesWritten >= i4TotalLen) {
			DBGLOG(REQ, INFO,
			       "%s: i4BytesWritten %d > i4TotalLen < %d\n",
			       __func__, i4BytesWritten, i4TotalLen);
			i4BytesWritten = i4TotalLen - 1;
		}
		pcCommand[i4BytesWritten] = '\0';
		i4BytesWritten++;

	}

	return i4BytesWritten;

} /* priv_driver_cmds */

#ifdef CFG_ANDROID_AOSP_PRIV_CMD
int android_private_support_driver_cmd(struct net_device *prNetDev,
	struct ifreq *prReq, int i4Cmd)
{
	struct android_wifi_priv_cmd priv_cmd;
	char *command = NULL;
	int ret = 0, bytes_written = 0;

	if (!prReq->ifr_data)
		return -EINVAL;

	if (copy_from_user(&priv_cmd, prReq->ifr_data, sizeof(priv_cmd)))
		return -EFAULT;
	/* total_len is controlled by the user. need check length */
	if (priv_cmd.total_len <= 0)
		return -EINVAL;

	command = kzalloc(priv_cmd.total_len, GFP_KERNEL);
	if (!command) {
		DBGLOG(REQ, WARN, "%s, alloc mem failed\n", __func__);
		return -ENOMEM;
	}

	if (copy_from_user(command, priv_cmd.buf, priv_cmd.total_len)) {
		ret = -EFAULT;
		goto FREE;
	}

	bytes_written = priv_driver_cmds(prNetDev, command, priv_cmd.total_len);

	if (bytes_written == -EOPNOTSUPP) {
		/* Report positive status */
		bytes_written = kalSnprintf(command, priv_cmd.total_len,
						"%s", "NotSupport");
	}

	if (bytes_written >= 0) {
		/* priv_cmd in but no response */
		if ((bytes_written == 0) && (priv_cmd.total_len > 0))
			command[0] = '\0';

		if (bytes_written >= priv_cmd.total_len)
			bytes_written = priv_cmd.total_len;
		else
			bytes_written++;

		priv_cmd.used_len = bytes_written;

		if (copy_to_user(priv_cmd.buf, command, bytes_written))
			ret = -EFAULT;
	} else
		ret = bytes_written;

FREE:
		kfree(command);

	return ret;
}
#endif /* CFG_ANDROID_AOSP_PRIV_CMD */

int priv_support_driver_cmd(struct net_device *prNetDev,
			    struct ifreq *prReq, int i4Cmd)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int ret = 0;
	char *pcCommand = NULL;
	struct priv_driver_cmd_s *priv_cmd = NULL;
	int i4BytesWritten = 0;
	int i4TotalLen = 0;
	struct ifreq *prOriprReq = NULL;
	uint8_t *prOriprReqData = NULL;

	if (!prReq->ifr_data) {
		ret = -EINVAL;
		goto exit;
	}

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prOriprReq = prReq;
	prOriprReqData = prReq->ifr_data;
	if (!prGlueInfo) {
		DBGLOG(REQ, WARN, "No glue info\n");
		ret = -EFAULT;
		goto exit;
	}
	if (prGlueInfo->u4ReadyFlag == 0) {
		ret = -EINVAL;
		goto exit;
	}

	priv_cmd = kzalloc(sizeof(struct priv_driver_cmd_s), GFP_KERNEL);
	if (!priv_cmd) {
		DBGLOG(REQ, WARN, "%s, alloc mem failed\n", __func__);
		return -ENOMEM;
	}

	if (copy_from_user(priv_cmd, prReq->ifr_data,
	    sizeof(struct priv_driver_cmd_s))) {
		DBGLOG(REQ, INFO, "%s: copy_from_user fail\n", __func__);
		ret = -EFAULT;
		goto exit;
	}

	i4TotalLen = priv_cmd->total_len;

	if (i4TotalLen <= 0 || i4TotalLen > PRIV_CMD_SIZE) {
		ret = -EINVAL;
		DBGLOG(REQ, INFO, "%s: i4TotalLen invalid\n", __func__);
		goto exit;
	}
	priv_cmd->buf[PRIV_CMD_SIZE - 1] = '\0';
	pcCommand = priv_cmd->buf;

	DBGLOG(REQ, INFO, "%s: driver cmd \"%s\" on %s,(%p,%p)\n", __func__,
		pcCommand, prReq->ifr_name, prReq, prReq->ifr_data);

	i4BytesWritten = priv_driver_cmds(prNetDev, pcCommand, i4TotalLen);

	if (i4BytesWritten == -EOPNOTSUPP) {
		/* Report positive status */
		i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen,
					    "%s", "UNSUPPORTED");
		i4BytesWritten++;
	}
	if ((prOriprReq != prReq) || (prOriprReqData != prReq->ifr_data)) {
		DBGLOG(REQ, WARN, "Err!!prReq(%p,%p) prReq->ifr_data(%p,%p)\n",
			prReq, prOriprReq, prReq->ifr_data, prOriprReqData);
		ret = -EFAULT;
		goto exit;
	}

	if (i4BytesWritten >= 0) {
		priv_cmd->used_len = i4BytesWritten;
		if ((i4BytesWritten == 0) && (priv_cmd->total_len > 0))
			pcCommand[0] = '\0';
		if (i4BytesWritten >= priv_cmd->total_len)
			i4BytesWritten = priv_cmd->total_len;
		else
			i4BytesWritten++;
		priv_cmd->used_len = i4BytesWritten;
		if (copy_to_user(prReq->ifr_data, priv_cmd,
				sizeof(struct priv_driver_cmd_s))) {
			ret = -EFAULT;
			DBGLOG(REQ, INFO, "copy fail");
		}
	} else
		ret = i4BytesWritten;

exit:
	kfree(priv_cmd);

	return ret;
}				/* priv_support_driver_cmd */

#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT
/* dynamic tx power control */
int priv_driver_set_power_control(struct net_device *prNetDev,
				  char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct PARAM_TX_PWR_CTRL_IOCTL rPwrCtrlParam = { 0 };
	u_int8_t fgIndex = FALSE;
	char *ptr = pcCommand, *ptr2 = NULL;
	char *str = NULL, *cmd = NULL, *name = NULL, *setting = NULL;
	uint8_t index = 0;
	uint32_t u4SetInfoLen = 0;

	while ((str = strsep(&ptr, " ")) != NULL) {
		if (kalStrLen(str) <= 0)
			continue;
		if (cmd == NULL)
			cmd = str;
		else if (name == NULL)
			name = str;
		else if (fgIndex == FALSE) {
			ptr2 = str;
			if (kalkStrtou8(str, 0, &index) != 0) {
				DBGLOG(REQ, INFO,
				       "index is wrong: %s\n", ptr2);
				return -1;
			}
			fgIndex = TRUE;
		} else if (setting == NULL) {
			setting = str;
			break;
		}
	}

	if ((name == NULL) || (fgIndex == FALSE)) {
		DBGLOG(REQ, INFO, "name(%s) or fgIndex(%d) is wrong\n",
		       name, fgIndex);
		return -1;
	}

	rPwrCtrlParam.fgApplied = (index == 0) ? FALSE : TRUE;
	rPwrCtrlParam.name = name;
	rPwrCtrlParam.index = index;
	rPwrCtrlParam.newSetting = setting;

	DBGLOG(REQ, INFO, "applied=[%d], name=[%s], index=[%u], setting=[%s]\n",
	       rPwrCtrlParam.fgApplied,
	       rPwrCtrlParam.name,
	       rPwrCtrlParam.index,
	       rPwrCtrlParam.newSetting);

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	kalIoctl(prGlueInfo,
		 wlanoidTxPowerControl,
		 (void *)&rPwrCtrlParam,
		 sizeof(struct PARAM_TX_PWR_CTRL_IOCTL),
		 &u4SetInfoLen);

	return 0;
}
#endif

#if CFG_MTK_WIFI_SW_WFDMA
int priv_driver_set_sw_wfdma(
	struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	struct mt66xx_chip_info *prChipInfo;
	struct BUS_INFO *prBusInfo;
	struct SW_WFDMA_INFO *prSwWfdmaInfo;
	uint32_t u4CfgSetNum = 0, u4Ret = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };

	struct PARAM_CUSTOM_KEY_CFG_STRUCT rKeyCfgInfo;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);

	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);
	prAdapter = prGlueInfo->prAdapter;
	if (prAdapter == NULL)
		return -1; /* WLAN_STATUS_ADAPTER_NOT_READY */

	prChipInfo = prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;
	prSwWfdmaInfo = &prBusInfo->rSwWfdmaInfo;

	kalMemZero(&rKeyCfgInfo, sizeof(rKeyCfgInfo));

	wlanCleanAllEmCfgSetting(prAdapter);


	if (i4Argc >= 2) {
		u4Ret = kalkStrtou32(apcArgv[1], 10, &u4CfgSetNum);
		if (u4Ret != 0) {
			DBGLOG(REQ, ERROR,
			       "apcArgv[2] format fail erro code:%d\n",
			       u4Ret);
			return -1;
		}

		DBGLOG(REQ, INFO, "SwWfdma=%d\n", u4CfgSetNum);
		if (prSwWfdmaInfo->rOps.enable)
			prSwWfdmaInfo->rOps.enable(
				prGlueInfo, u4CfgSetNum != 0);
	}
	return i4BytesWritten;
}				/* priv_driver_set_sw_wfdma */
#endif

#if CFG_SUPPORT_CSI
int priv_driver_set_csi(struct net_device *prNetDev,
			char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	int32_t i4Ret = 0;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	signed char *apcArgv[WLAN_CFG_ARGV_MAX] = {NULL};
	uint8_t aucMacAddr[MAC_ADDR_LEN] = {0};
	struct CMD_CSI_CONTROL_T *prCSICtrl = NULL;
	struct CSI_INFO_T *prCSIInfo = NULL;
	enum CSI_OUTPUT_METHOND_COMMAND eOutPut;
	enum CSI_STA_MAC_MODE_T eStaMode;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, INFO, "[CSI] command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "[CSI] argc is %i\n", i4Argc);

	prCSIInfo = glCsiGetCSIInfo();

	prCSICtrl = (struct CMD_CSI_CONTROL_T *) kalMemAlloc(
			sizeof(struct CMD_CSI_CONTROL_T), VIR_MEM_TYPE);
	if (!prCSICtrl) {
		DBGLOG(REQ, LOUD,
			"[CSI] allocate memory for prCSICtrl failed\n");
		i4BytesWritten = -1;
		goto out;
	}

	if (i4Argc < CSI_OPT_1 || i4Argc > CSI_OPT_5) {
		DBGLOG(REQ, ERROR, "[CSI] argc %i is invalid\n", i4Argc);
		i4BytesWritten = -1;
		goto out;
	}

	i4Ret = kalkStrtou8(apcArgv[1], 0, &(prCSICtrl->ucMode));
	if (i4Ret) {
		DBGLOG(REQ, LOUD,
			"[CSI] parse csi mode error u4Ret=%d\n", i4Ret);
		i4BytesWritten = -1;
		goto out;
	}

	if (prCSICtrl->ucMode >= CSI_CONTROL_MODE_NUM) {
		DBGLOG(REQ, ERROR, "[CSI] Invalid csi mode %d\n",
				prCSICtrl->ucMode);
		i4BytesWritten = -1;
		goto out;
	}
	prCSIInfo->ucMode = prCSICtrl->ucMode;
	prCSICtrl->ucBandIdx = glCsiGetBandIdx();

	if (prCSICtrl->ucMode == CSI_CONTROL_MODE_STOP ||
		prCSICtrl->ucMode == CSI_CONTROL_MODE_START) {
		glCsiSetEnable(prGlueInfo,
			       prCSIInfo,
			       prCSICtrl->ucMode == CSI_CONTROL_MODE_START);

		if (prCSICtrl->ucMode == CSI_CONTROL_MODE_STOP)
			glCsiFreeStaList(prGlueInfo);

		goto send_cmd;
	}

	if (i4Argc < CSI_OPT_3) {
		DBGLOG(REQ, ERROR, "[CSI] argc %i is invalid\n", i4Argc);
		i4BytesWritten = -1;
		goto out;
	}

	i4Ret = kalkStrtou8(apcArgv[2], 0, &(prCSICtrl->ucCfgItem));
	if (i4Ret) {
		DBGLOG(REQ, LOUD,
			"[CSI] parse cfg item error i4Ret=%d\n", i4Ret);
		i4BytesWritten = -1;
		goto out;
	}

	if (prCSICtrl->ucCfgItem >= CSI_CONFIG_ITEM_NUM) {
		DBGLOG(REQ, ERROR, "[CSI] Invalid csi cfg_item %u\n",
			prCSICtrl->ucCfgItem);
		i4BytesWritten = -1;
		goto out;
	}

	i4Ret = kalkStrtou8(apcArgv[3], 0, &(prCSICtrl->ucValue1));
	if (i4Ret) {
		DBGLOG(REQ, LOUD,
			"[CSI] parse csi cfg value1 error i4Ret=%d\n", i4Ret);
		i4BytesWritten = -1;
		goto out;
	}
	prCSIInfo->ucValue1[prCSICtrl->ucCfgItem] = prCSICtrl->ucValue1;

	if (prCSICtrl->ucCfgItem == CSI_CONFIG_OUTPUT_METHOD) {
		eOutPut = (enum CSI_OUTPUT_METHOND_COMMAND) prCSICtrl->ucValue1;
		if (eOutPut == CSI_PROC_FILE_COMMAND) {
			prCSIInfo->eCSIOutput = CSI_OUTPUT_PROC_FILE;
			DBGLOG(REQ, INFO,
				"[CSI] Set CSI data output to proc file\n");
		} else if (eOutPut == CSI_VENDOR_EVENT_COMMAND) {
			prCSIInfo->eCSIOutput = CSI_OUTPUT_VENDOR_EVENT;
			DBGLOG(REQ, INFO,
				"[CSI] Set CSI data output to vendor event\n");
		} else {
			DBGLOG(REQ, ERROR,
				"[CSI] Invalid csi output method %d\n",
				eOutPut);
			i4BytesWritten = -1;
		}
		goto out;
	}

	if (i4Argc >= CSI_OPT_4) {
		i4Ret = kalkStrtou32(apcArgv[4], 0, &(prCSICtrl->u4Value2));
		if (i4Ret) {
			DBGLOG(REQ, LOUD,
				"[CSI] parse csi cfg value2 error i4Ret=%d\n",
				i4Ret);
			i4BytesWritten = -1;
			goto out;
		}
		prCSIInfo->u4Value2[prCSICtrl->ucCfgItem] = prCSICtrl->u4Value2;
	}

	if (i4Argc == CSI_OPT_5) {
		i4Ret = wlanHwAddrToBin(apcArgv[5], &aucMacAddr[0]);
		if (i4Ret < 0) {
			DBGLOG(REQ, ERROR,
				"[CSI] Check your mac addr format: xx:xx:xx:xx:xx:xx!! i4Ret=%d\n",
				i4Ret);
			i4BytesWritten = -1;
			goto out;
		}
		COPY_MAC_ADDR(prCSICtrl->aucMacAddr, aucMacAddr);
	}

	/* Check invalid chain number 1~16 */
	if ((prCSICtrl->ucCfgItem == CSI_CONFIG_CHAIN_NUMBER) &&
		  ((prCSICtrl->ucValue1 > 16) || (prCSICtrl->ucValue1 < 1))) {
		DBGLOG(REQ, ERROR,
			"[CSI] Invalid chain number: %d\n",
			prCSICtrl->ucValue1);
		i4BytesWritten = -1;
		goto out;
	}

	if (prCSICtrl->ucCfgItem == CSI_CONFIG_BAND) {
		enum ENUM_MBMC_BN eBand = 0;

		eBand = (enum ENUM_MBMC_BN) prCSICtrl->ucValue1;
		if (eBand < ENUM_BAND_NUM) {
			DBGLOG(REQ, INFO, "[CSI] set band: %d\n", eBand);
			glCsiSetBandIdx(eBand);
			i4BytesWritten = 0;
			goto out;
		} else {
			DBGLOG(REQ, ERROR, "[CSI] Invalid band: %d\n", eBand);
			i4BytesWritten = -1;
			goto out;
		}
	}

	if (prCSICtrl->ucCfgItem == CSI_CONFIG_FILTER_MODE) {
		eStaMode = (enum CSI_STA_MAC_MODE_T) prCSICtrl->ucValue1;
		if (eStaMode == CSI_STA_MAC_ADD) {
			switch (prCSICtrl->u4Value2) {
			case CSI_STA_MAC_ADD:
				i4Ret = glCsiAddSta(prGlueInfo, prCSICtrl);
				if (i4Ret < 0) {
					i4BytesWritten = -1;
					goto out;
				}

				break;
			case CSI_STA_MAC_DEL:
				i4Ret = glCsiDelSta(prGlueInfo, prCSICtrl);
				if (i4Ret < 0) {
					i4BytesWritten = -1;
					goto out;
				}

				break;
			default:
				DBGLOG(REQ, ERROR,
					"[CSI] Invalid STA MAC mode: %d\n",
					prCSICtrl->u4Value2);
				break;
			}
		}
	}

send_cmd:
	DBGLOG(REQ, STATE,
	   "[CSI] Set band idx %d, mode %d, csi cfg item %d, value1 %d, value2 %d",
		prCSICtrl->ucBandIdx,
		prCSICtrl->ucMode, prCSICtrl->ucCfgItem,
		prCSICtrl->ucValue1, prCSICtrl->u4Value2);

	rStatus = kalIoctl(prGlueInfo, wlanoidSetCSIControl, prCSICtrl,
		sizeof(struct CMD_CSI_CONTROL_T), &u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, ERROR,
			"[CSI] send CSI control cmd failed, rStatus %u\n",
			rStatus);
		i4BytesWritten = -1;
	}

out:
	if (prCSICtrl)
		kalMemFree(prCSICtrl, VIR_MEM_TYPE,
				sizeof(struct CMD_CSI_CONTROL_T));

	return i4BytesWritten;
}
#endif

#if CFG_SUPPORT_RTT
int priv_driver_set_rtt(struct net_device *prNetDev,
			char *pcCommand, int i4TotalLen)
{
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	uint8_t aucTestMacAddr[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
	struct GLUE_INFO *prGlueInfo = NULL;
	struct RTT_CAPABILITIES rRttCapabilities;
	struct PARAM_RTT_REQUEST request;
	uint32_t rStatus = WLAN_STATUS_FAILURE;
	uint32_t u4BufLen;
	uint8_t ucType = 0;
	uint32_t u4Freq = 0;
	uint8_t ucWidth = WIFI_CHAN_WIDTH_80;
	uint32_t u4Ret;

	prGlueInfo = *((struct GLUE_INFO **)netdev_priv(prNetDev));

	DBGLOG(REQ, TRACE, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc < 2)
		goto out;

	DBGLOG(REQ, TRACE, "argc %i, cmd [%s]\n", i4Argc, apcArgv[1]);
	u4Ret = kalkStrtou8(apcArgv[1], 0, &ucType);
	if (u4Ret) {
		DBGLOG(REQ, ERROR, "parse ucType error %d\n", u4Ret);
		goto out;
	}

	/* test CMD_ID_RTT_GET_CAPABILITIES */
	if (ucType == 0) {
		kalMemZero(&rRttCapabilities, sizeof(rRttCapabilities));
		rStatus = kalIoctl(prGlueInfo,
			wlanoidGetRttCapabilities,
			&rRttCapabilities,
			sizeof(struct RTT_CAPABILITIES),
			&u4BufLen);
	}

	/* test CMD_ID_RTT_RANGE_REQUEST */
	if (ucType == 1 /* mc */ || ucType == 2 /* az */) {
		kalMemZero(&request, sizeof(request));
		request.fgEnable = true;
		request.ucConfigNum = 1;

		/* Peer mac */
		if (i4Argc > 2)
			wlanHwAddrToBin(apcArgv[2], aucTestMacAddr);

		/* Peer frequency */
		if (i4Argc > 3)
			u4Ret = kalkStrtou32(apcArgv[3], 0, &u4Freq);

		/* Channel width */
		if (i4Argc > 4)
			u4Ret = kalkStrtou8(apcArgv[4], 0, &ucWidth);

		COPY_MAC_ADDR(request.arRttConfigs[0].aucAddr, aucTestMacAddr);
		request.arRttConfigs[0].eType = (ucType == 1) ?
			RTT_TYPE_2_SIDED : RTT_TYPE_2_SIDED_11AZ_NTB;
		request.arRttConfigs[0].ePeer = RTT_PEER_AP;
		request.arRttConfigs[0].rChannel.width =
			(enum WIFI_CHANNEL_WIDTH) ucWidth;
		request.arRttConfigs[0].rChannel.center_freq =
			u4Freq ? u4Freq : 5180;
		request.arRttConfigs[0].rChannel.center_freq0 =
			u4Freq ? u4Freq : 5180;
		request.arRttConfigs[0].rChannel.center_freq1 = 0;
		request.arRttConfigs[0].u2BurstPeriod = 0;
		request.arRttConfigs[0].u2NumBurstExponent = 0;
		request.arRttConfigs[0].u2PreferencePartialTsfTimer = 0;
		request.arRttConfigs[0].ucNumFramesPerBurst = 5;
		request.arRttConfigs[0].ucNumRetriesPerRttFrame = 3;
		request.arRttConfigs[0].ucNumRetriesPerFtmr = 0;
		request.arRttConfigs[0].ucLciRequest = 0;
		request.arRttConfigs[0].ucLcrRequest = 0;
		request.arRttConfigs[0].ucBurstDuration = 11;
		request.arRttConfigs[0].ePreamble = WIFI_RTT_PREAMBLE_VHT;
		request.arRttConfigs[0].eBw = rttBssBwToRttBw(ucWidth);
		request.arRttConfigs[0].ucASAP = 1;
		request.arRttConfigs[0].ucFtmMinDeltaTime = 40;

		rStatus = kalIoctl(prGlueInfo, wlanoidHandleRttRequest,
			&request,
			sizeof(struct PARAM_RTT_REQUEST),
			&u4BufLen);
	}
out:
	return (rStatus == WLAN_STATUS_SUCCESS) ? 0 : -1;
}
#endif


#if (CFG_SUPPORT_POWER_THROTTLING == 1)
int priv_driver_set_pwr_level(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	uint32_t u4Ret;
	int level = 0;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	u4Ret = kalkStrtou32(apcArgv[1], 0, &level);
	if (u4Ret)
		DBGLOG(REQ, LOUD,
		       "parse get_mcr error (Address) u4Ret=%d\n",
		       u4Ret);

	prAdapter->u4PwrLevel = level;

	connsysPowerLevelNotify(prGlueInfo->prAdapter);

	return i4BytesWritten;
}

int priv_driver_set_pwr_temp(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	uint32_t u4Ret;
	int32_t i4ArgNum = 3;
	uint32_t u4MaxTemp = 0;
	uint32_t u4RecoveryTemp = 0;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc >= i4ArgNum) {
		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4MaxTemp);
		if (u4Ret)
			DBGLOG(REQ, LOUD,
			       "parse get_mcr error (Address) u4Ret=%d\n",
			       u4Ret);

		u4Ret = kalkStrtou32(apcArgv[2], 0, &u4RecoveryTemp);
		if (u4Ret)
			DBGLOG(REQ, LOUD,
			       "parse get_mcr error (Address) u4Ret=%d\n",
			       u4Ret);

		(prAdapter->rTempInfo).max_temp = u4MaxTemp;
		(prAdapter->rTempInfo).recovery_temp = u4RecoveryTemp;

		connsysPowerTempNotify(prGlueInfo->prAdapter);
	}

	return i4BytesWritten;
}
#endif

#if (CFG_WIFI_ISO_DETECT == 1)
/* Private Coex Ctrl Subcmd for Isolation Detection */
static int priv_driver_iso_detect(struct GLUE_INFO *prGlueInfo,
				struct COEX_CMD_HANDLER *prCoexCmdHandler,
				signed char *argv[])
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	uint32_t u4Ret = 0;
	uint32_t u4Data = 0;

	struct COEX_CMD_ISO_DETECT rCoexCmdIsoDetect;

	rCoexCmdIsoDetect.u4Isolation = 0;

	u4Ret = kalkStrtou32(argv[2], 0, &(rCoexCmdIsoDetect.u4IsoPath));
	if (u4Ret) {
		DBGLOG(REQ, LOUD,
		"Parse Iso Path failed u4Ret=%d\n", u4Ret);
		return WLAN_STATUS_INVALID_DATA;
	}

	u4Ret = kalkStrtou32(argv[3], 0, &(rCoexCmdIsoDetect.u4Channel));
	if (u4Ret) {
		DBGLOG(REQ, LOUD,
		"Parse channel failed u4Ret = %d\n", u4Ret);
		return WLAN_STATUS_INVALID_DATA;
	}

	u4Ret = kalkStrtou32(argv[4], 0, &u4Data);
	if (u4Ret) {
		DBGLOG(REQ, LOUD,
		"Parse channel failed u4Ret = %d\n", u4Ret);
		return WLAN_STATUS_INVALID_DATA;
	}

	rCoexCmdIsoDetect.u4IsoPath |= ((u4Data << 8) & BITS(8, 15));

	/* Copy Memory */
	kalMemCopy(prCoexCmdHandler->aucBuffer,
			&rCoexCmdIsoDetect,
			sizeof(struct COEX_CMD_ISO_DETECT));

	/* Ioctl Isolation Detect */
	rStatus = kalIoctl(prGlueInfo,
			wlanoidQueryCoexIso,
			prCoexCmdHandler,
			sizeof(struct COEX_CMD_HANDLER),
			&u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS)
		return WLAN_STATUS_FAILURE;

	/* If all pass, return u4Ret to 0 */
	return u4Ret;
}
#endif

/* Private Command for Coex Ctrl */
int priv_driver_coex_ctrl(struct net_device *prNetDev,
				char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int32_t i4ArgNum = 2;
	signed char *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret = 0;
	uint32_t u4Offset = 0;
	enum ENUM_COEX_CMD_CTRL CoexCmdCtrl;
	struct COEX_CMD_HANDLER rCoexCmdHandler;

	if (!prNetDev) {
		DBGLOG(REQ, ERROR,
		"%s null prNetDev\n",
		__func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return WLAN_STATUS_FAILURE;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	/* Prevent Kernel Panic, set default i4ArgNum to 2 */
	if (i4Argc >= i4ArgNum) {

		/* Parse Coex SubCmd */
		u4Ret = kalkStrtou32(apcArgv[1], 0, &rCoexCmdHandler.u4SubCmd);
		if (u4Ret)
			return WLAN_STATUS_INVALID_DATA;

		CoexCmdCtrl =
			(enum ENUM_COEX_CMD_CTRL)rCoexCmdHandler.u4SubCmd;

		switch (CoexCmdCtrl) {
		case COEX_CMD_SET_RX_DATA_INFO:
		{
			break;
		}
		/* Isolation Detection */
		case COEX_CMD_GET_INFO:
		{
			break;
		}
		case COEX_CMD_GET_ISO_DETECT:
		{
#if (CFG_WIFI_ISO_DETECT == 1)
			struct COEX_CMD_ISO_DETECT *prCoexCmdIsoDetect;
			int32_t i4SubArgNum = 5;

			DBGLOG(REQ, ERROR,
			"COEX_CMD_GET_ISO_DETECT : i4Argc not match %d\n",
			i4Argc);

			/* Safely dereference "argv[3]".*/
			if (i4Argc != i4SubArgNum)
				break;

			/* Isolation Detection Method */
			u4Ret = priv_driver_iso_detect(prGlueInfo,
							&rCoexCmdHandler,
							apcArgv);
			if (u4Ret)
				return -1;

			/* Get Isolation value */
			prCoexCmdIsoDetect =
		(struct COEX_CMD_ISO_DETECT *)rCoexCmdHandler.aucBuffer;

			/* Set Return i4BytesWritten Value */
			u4Offset = snprintf(pcCommand, i4TotalLen, "%d",
				(prCoexCmdIsoDetect->u4Isolation/2));
			DBGLOG(REQ, INFO, "Isolation: %d\n",
				(prCoexCmdIsoDetect->u4Isolation/2));
#endif
			break;
		}
		/* Default Coex Cmd */
		default:
			break;
		}

		/* Set Return i4BytesWritten Value */
		i4BytesWritten = (int32_t)u4Offset;
	}
	return i4BytesWritten;
}

#if (CFG_WIFI_GET_DPD_CACHE == 1)
int priv_driver_get_dpd_cache(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen)
{
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	int32_t i4BytesWritten = 0, i4Argc = 0;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	struct PARAM_GET_DPD_CACHE *prDpdCache = NULL;
	uint8_t idx = 0;

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	prDpdCache = (struct PARAM_GET_DPD_CACHE *)kalMemAlloc(
			sizeof(struct PARAM_GET_DPD_CACHE), VIR_MEM_TYPE);
	if (!prDpdCache) {
		DBGLOG(REQ, ERROR, "Allocate prDpdCache failed!\n");
		i4BytesWritten = -1;
		goto out;
	}

	rStatus = kalIoctl(prGlueInfo, wlanoidQueryDpdCache, prDpdCache,
			   sizeof(struct PARAM_GET_DPD_CACHE), &u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS)
		goto out;

	i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
		   i4TotalLen - i4BytesWritten,
		   "\nCurrent DPD Cache:\n");

	for (idx = 0; idx < prDpdCache->ucDpdCacheNum; idx++) {
		i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
		   i4TotalLen - i4BytesWritten,
		   "Cache[%u] = WF%u ch%u\n",
		   idx,
		   prDpdCache->ucDpdCachePath[idx],
		   prDpdCache->u4DpdCacheCh[idx]);
	}

	DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__, pcCommand);

out:

	if (prDpdCache)
		kalMemFree(prDpdCache, VIR_MEM_TYPE,
			sizeof(struct PARAM_GET_DPD_CACHE));

	return i4BytesWritten;
}
#endif /* CFG_WIFI_GET_DPD_CACHE */

#if CFG_SUPPORT_PCIE_GEN_SWITCH
int priv_driver_set_pcie_speed(struct net_device *prNetDev,
				char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	struct mt66xx_chip_info *prChipInfo;
	struct BUS_INFO *prBusInfo;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	uint32_t status = 0;
	uint8_t ucPcieSpeed = 0;

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc != 2) {
		DBGLOG(REQ, ERROR, "argc(%d) is error\n", i4Argc);
		return -1;
	}

	status = kalkStrtou8(apcArgv[1], 0, &ucPcieSpeed);
	if (status) {
		DBGLOG(REQ, ERROR, "parse ucPcieSpeed error u4Ret=%d\n",
			status);
		ucPcieSpeed = 0;
	}

	if (ucPcieSpeed > MAX_PCIE_SPEED) {
		DBGLOG(REQ, ERROR, "invalid pcie speed %d\n", ucPcieSpeed);
		return -1;
	}

	prAdapter = prGlueInfo->prAdapter;
	if (prAdapter == NULL)
		return -1; /* WLAN_STATUS_ADAPTER_NOT_READY */

	prChipInfo = prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;
	if (prBusInfo->setPcieSpeed)
		prBusInfo->setPcieSpeed(prGlueInfo, ucPcieSpeed);

	return i4Argc;
}
#endif
#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
int priv_driver_set_6g_pwr_mode(struct net_device *prNetDev, char *pcCommand,
			int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS, u4BufLen = 0;
	int32_t i4BytesWritten = 0, i4Argc = 0, u4Ret = 0;
	uint8_t ucMode = 0, c = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, INFO, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, INFO, "argc is %i\n", i4Argc);

	for (c = 0; (c < i4Argc) && (c + 1 < i4Argc); c++) {
		if (kalStrCmp(apcArgv[c], "mode") == 0) {
			u4Ret = kalkStrtou8(apcArgv[c+1], 0, &ucMode);
			if (u4Ret) {
				DBGLOG(REQ, ERROR,
					"parse set_6g_pwr_mode error Ret=%d\n",
					u4Ret);
				return WLAN_STATUS_INVALID_DATA;
			}
			rStatus = kalIoctl(prGlueInfo, wlanoidSet6GPwrMode,
				   &ucMode, sizeof(uint8_t), &u4BufLen);

			if (rStatus != WLAN_STATUS_SUCCESS)
				return WLAN_STATUS_FAILURE;
		}
	}

	i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
		   i4TotalLen - i4BytesWritten,
		   "Set 6G Power Mode = [%u] success\n", ucMode);

	return i4BytesWritten;

}
#endif

#if CFG_SUPPORT_WED_PROXY
int priv_driver_set_wed_enable(struct net_device *prNetDev,
				char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	int32_t u4Ret = 0;
	int32_t Enable = -1;

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE) {
		DBGLOG(REQ, ERROR, "input arg is null.\n");
		return -1;
	}

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	u4Ret = kalkStrtos32(apcArgv[1], 0, &Enable);
	if (u4Ret) {
		DBGLOG(REQ, ERROR, "kalkStrtos32 error %d\n", u4Ret);
		return -1;
	}

	if (Enable)
		kalIoctlByBssIdx(prGlueInfo, wlanoidWedAttachWarp,
				prNetDev, sizeof(struct net_device *),
				&Enable, wlanGetBssIdx(prNetDev));
	else
		kalIoctlByBssIdx(prGlueInfo, wlanoidWedDetachWarp,
				prNetDev, sizeof(struct net_device *),
				&Enable, wlanGetBssIdx(prNetDev));

	return 0;
}

int priv_driver_get_drv_mcr_directly(struct net_device *prNetDev,
				   char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4Ret;

	/* Add Antenna Selection Input */
	/* INT_32 i4ArgNum_with_ant_sel = 3; */

	int32_t i4ArgNum = 2;

	struct CMD_ACCESS_REG rCmdAccessReg;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc >= i4ArgNum) {
		rCmdAccessReg.u4Address = 0;
		u4Ret = kalkStrtou32(apcArgv[1], 0, &(rCmdAccessReg.u4Address));
		if (u4Ret)
			DBGLOG(REQ, LOUD,
			       "parse get_drv_mcr error (Address) u4Ret=%d\n",
			       u4Ret);

		/* rCmdAccessReg.u4Address = kalStrtoul(apcArgv[1], NULL, 0); */
		rCmdAccessReg.u4Data = 0;

		DBGLOG(REQ, LOUD, "address is %x\n", rCmdAccessReg.u4Address);

		rStatus = kalIoctl(prGlueInfo, wlanoidQueryDrvMcrReadDirectly,
				   &rCmdAccessReg, sizeof(rCmdAccessReg),
				   &u4BufLen);

		DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
		if (rStatus != WLAN_STATUS_SUCCESS)
			return -1;

		i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen, "0x%08x",
					  (unsigned int)rCmdAccessReg.u4Data);
		DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__,
		       pcCommand);
	}

	return i4BytesWritten;

}				/* priv_driver_get_drv_mcr */

int priv_driver_set_drv_mcr_directly(struct net_device *prNetDev,
				char *pcCommand,
				int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	int32_t i4BytesWritten = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	uint32_t u4Ret;

	/* Add Antenna Selection Input */
	/* INT_32 i4ArgNum_with_ant_sel = 4; */

	int32_t i4ArgNum = 3;

	struct CMD_ACCESS_REG rCmdAccessReg;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc >= i4ArgNum) {
		u4Ret = kalkStrtou32(apcArgv[1], 0, &(rCmdAccessReg.u4Address));
		if (u4Ret)
			DBGLOG(REQ, LOUD,
			       "parse get_drv_mcr error (Address) u4Ret=%d\n",
			       u4Ret);

		u4Ret = kalkStrtou32(apcArgv[2], 0, &(rCmdAccessReg.u4Data));
		if (u4Ret)
			DBGLOG(REQ, LOUD,
			       "parse get_drv_mcr error (Data) u4Ret=%d\n",
			       u4Ret);

		rStatus = kalIoctl(prGlueInfo, wlanoidSetDrvMcrWriteDirectly,
				   &rCmdAccessReg, sizeof(rCmdAccessReg),
				   &u4BufLen);

		if (rStatus != WLAN_STATUS_SUCCESS)
			return -1;

	}

	return i4BytesWritten;

}
#endif /* CFG_SUPPORT_WED_PROXY */

#if (CFG_SUPPORT_PWR_LMT_EMI == 1)
int priv_driver_get_power_limit_emi_data(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen)
{
#define PARSE_POWER_LIMIT_AGRC_NUM 3

	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	int32_t i4BytesWritten = 0, i4Argc = 0;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	uint32_t u4RFBand, u4Channel;

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);


	if (i4Argc != PARSE_POWER_LIMIT_AGRC_NUM) {
		DBGLOG(REQ, ERROR, "argc(%d) is error\n", i4Argc);
		return -1;
	}

	if (kalkStrtou32(apcArgv[1], 0, &u4RFBand)
		|| kalkStrtou32(apcArgv[2], 0, &u4Channel)) {
		DBGLOG(REQ, ERROR, "Parse error\n");
		return -1;
	}

	DBGLOG(REQ, STATE, "u4RFBand :%d, u4Channel :%d\n",
		u4RFBand, u4Channel);

	i4BytesWritten = rlmDomainReadPwrLimitEmiData(prAdapter,
		pcCommand,
		i4TotalLen,
		u4RFBand,
		u4Channel);

	return i4BytesWritten;
}
#endif /* CFG_SUPPORT_PWR_LMT_EMI == 1 */

int priv_driver_show_tr_info(struct net_device *prNetDev,
			     char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo;
	int32_t i4BytesWritten = 0;

	ASSERT(prNetDev);

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	kalIoctl(prGlueInfo, wlanoidShowPdmaInfo, (void *) pcCommand,
		 i4TotalLen, &i4BytesWritten);

	return i4BytesWritten;
}

int priv_driver_show_ple_info(struct net_device *prNetDev,
			      char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo;
	int32_t i4BytesWritten = 0;

	ASSERT(prNetDev);

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	kalIoctl(prGlueInfo, wlanoidShowPleInfo, (void *) pcCommand,
		 i4TotalLen, &i4BytesWritten);

	return i4BytesWritten;
}

int priv_driver_show_pse_info(struct net_device *prNetDev,
			      char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo;
	int32_t i4BytesWritten = 0;

	ASSERT(prNetDev);

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	kalIoctl(prGlueInfo, wlanoidShowPseInfo, (void *) pcCommand,
		 i4TotalLen, &i4BytesWritten);

	return i4BytesWritten;
}

int priv_driver_show_csr_info(struct net_device *prNetDev,
			      char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo;
	int32_t i4BytesWritten = 0;

	ASSERT(prNetDev);

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	kalIoctl(prGlueInfo, wlanoidShowCsrInfo, (void *) pcCommand,
		 i4TotalLen, &i4BytesWritten);

	return i4BytesWritten;
}

int priv_driver_show_dmasch_info(struct net_device *prNetDev,
				 char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo;
	int32_t i4BytesWritten = 0;

	ASSERT(prNetDev);

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	kalIoctl(prGlueInfo, wlanoidShowDmaschInfo, (void *) pcCommand,
		 i4TotalLen, &i4BytesWritten);

	return i4BytesWritten;
}

#if CFG_SUPPORT_EASY_DEBUG
int priv_driver_fw_param(struct net_device *prNetDev,
			 char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo;
	int32_t i4BytesWritten = 0;

	ASSERT(prNetDev);

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	kalIoctl(prGlueInfo, wlanoidSetFwParam, (void *) (pcCommand + 13),
		 i4TotalLen - 13, &i4BytesWritten);

	return i4BytesWritten;
}
#endif /* CFG_SUPPORT_EASY_DEBUG */

#if (CFG_PCIE_GEN_SWITCH == 1)
int priv_driver_set_genswitch(struct net_device *prNetDev,
			 char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo;
	int32_t i4BytesWritten = 0;

	ASSERT(prNetDev);

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	if (!netif_carrier_ok(prNetDev))
		return -1;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	kalIoctl(prGlueInfo, wlanoidSetMddpGenSwitch, (void *) pcCommand,
		 i4TotalLen, &i4BytesWritten);

	return i4BytesWritten;
}
#endif /* CFG_PCIE_GEN_SWITCH */

int priv_driver_tspec_operation(struct net_device *prNetDev,
				char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo;
	int32_t i4BytesWritten = 0;

	ASSERT(prNetDev);

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	kalIoctl(prGlueInfo, wlanoidTspecOperation, (void *) pcCommand,
		 i4TotalLen, &i4BytesWritten);

	return i4BytesWritten;
}

int priv_driver_it_operation(struct net_device *prNetDev,
			     char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo;
	int32_t i4BytesWritten = 0;

	ASSERT(prNetDev);

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	kalIoctl(prGlueInfo, wlanoidPktProcessIT, (void *) pcCommand,
		 i4TotalLen, &i4BytesWritten);

	return i4BytesWritten;
}

int priv_driver_fw_event(struct net_device *prNetDev,
			 char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo;
	int32_t i4BytesWritten = 0;

	ASSERT(prNetDev);

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	kalIoctl(prGlueInfo, wlanoidFwEventIT, (void *) (pcCommand + 9),
		 i4TotalLen - 9, &i4BytesWritten);

	return i4BytesWritten;
}

int priv_driver_uapsd(struct net_device *prNetDev,
		      char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo;
	int32_t i4BytesWritten = 0;

	ASSERT(prNetDev);

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	kalIoctl(prGlueInfo, wlanoidDumpUapsdSetting, (void *) pcCommand,
		 i4TotalLen, &i4BytesWritten);

	return i4BytesWritten;
}

int priv_driver_show_ahdbg(struct net_device *prNetDev,
			   char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo;
	int32_t i4BytesWritten = 0;

	ASSERT(prNetDev);

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	kalIoctl(prGlueInfo, wlanoidShowAhdbgInfo, (void *) pcCommand,
		 i4TotalLen, &i4BytesWritten);

	return i4BytesWritten;
}

#if (CFG_MTK_SUPPORT_LIGHT_MDDP == 1)
void mddp_test_ch_switch_event(struct ADAPTER *prAd, char *pcCmd, int tLen,
						  int32_t argc, int8_t *argv[])
{
	uint8_t ucIdx = 0;
	struct WIFI_EVENT *prWifiEvt = NULL;
	struct EVENT_LTE_SAFE_CHN *prLteSafeChnEvt = NULL;

	uint8_t ucVersion = 2;
	uint32_t u4Flags = 0x1;
	uint32_t aucSafeChnl[4] = {0x2, 0xfffffff, 0x1ff, 0x3fff};

	if (argc >= 8) {
		if (kalkStrtou8(argv[2], 0, &ucVersion))
			return;

		if (kalkStrtou32(argv[3], 0, &u4Flags))
			return;

		for (ucIdx = 0; ucIdx < ENUM_SAFE_CH_MASK_MAX_NUM; ucIdx++) {
			if (kalkStrtou32(argv[ucIdx + 4], 0,
					&aucSafeChnl[ucIdx]))
				return;
		}

		DBGLOG(INIT, INFO,
			"Ver:%u Flag:%u SafeCh:[0x%x, 0x%x, 0x%x, 0x%x]\n",
			ucVersion, u4Flags, aucSafeChnl[0],
			aucSafeChnl[1], aucSafeChnl[2], aucSafeChnl[3]);
	}

	prWifiEvt = kalMemAlloc(sizeof(struct WIFI_EVENT) +
		sizeof(struct EVENT_LTE_SAFE_CHN), VIR_MEM_TYPE);
	if (!prWifiEvt) {
		DBGLOG(INIT, ERROR, "Alloc prWifiEvt fail\n");
		return;
	}

	kalMemZero(prWifiEvt, sizeof(struct WIFI_EVENT) +
		sizeof(struct EVENT_LTE_SAFE_CHN));

	prLteSafeChnEvt = (struct EVENT_LTE_SAFE_CHN *)
		&prWifiEvt->aucBuffer[0];

	if (!prLteSafeChnEvt) {
		kalMemFree(prWifiEvt, VIR_MEM_TYPE, sizeof(struct WIFI_EVENT) +
			sizeof(struct EVENT_LTE_SAFE_CHN));
		DBGLOG(INIT, ERROR, "Alloc prLteSafeChnEvt fail\n");
		return;
	}

	prLteSafeChnEvt->ucVersion = ucVersion;
	prLteSafeChnEvt->u4Flags = u4Flags;
	for (ucIdx = 0; ucIdx < ENUM_SAFE_CH_MASK_MAX_NUM; ucIdx++) {
		prLteSafeChnEvt->rLteSafeChn.au4SafeChannelBitmask[ucIdx]
			= aucSafeChnl[ucIdx];
	}

	cnmIdcDetectHandler(prAd, prWifiEvt);

	kalMemFree(prWifiEvt, VIR_MEM_TYPE, sizeof(struct WIFI_EVENT) +
		sizeof(struct EVENT_LTE_SAFE_CHN));
}

void mddp_test_get_acs_best_ch(struct ADAPTER *prAd, char *pcCmd, int tLen)
{
	uint8_t ucRet = 0;
	int32_t i4BytesWritten = 0;

	ucRet = p2pFunGetAcsBestCh(prAd, BAND_2G4, MAX_BW_80MHZ,
		0x2, 0xfffffff, 0x1ff, 0x3fff);

	i4BytesWritten += kalScnprintf(pcCmd + i4BytesWritten,
		tLen - i4BytesWritten,
		"[TestCase 8: %s] Assert Channel (%u == 1)\n",
		ucRet == 1 ? "PASS" : "FAILED", ucRet);
}

int priv_driver_set_mddp_test(struct net_device *prNetDev,
			char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAd = NULL;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	int32_t u4Ret = 0;
	uint32_t u4TestCase = 0;

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE) {
		DBGLOG(INIT, ERROR, "input arg is null.\n");
		return -1;
	}

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAd = prGlueInfo->prAdapter;

	DBGLOG(INIT, INFO, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(INIT, LOUD, "argc is %i\n", i4Argc);

	u4Ret = kalkStrtou32(apcArgv[1], 0, &u4TestCase);

	switch (u4TestCase) {
	case 0: /* mddpInit */
		mddpInit(NORMAL_BOOT);
		break;
	case 1: /* mddpUninit */
		mddpUninit();
		break;
	case 2: /* pccciOn */
		if (prAd->chip_info->coexpccifon)
			prAd->chip_info->coexpccifon(prAd);
		break;
	case 3: /* pccciOff */
		if (prAd->chip_info->coexpccifoff)
			prAd->chip_info->coexpccifoff(prAd);
		break;
	case 4:
		mddpNotifyWifiOnStart(FALSE);
		break;
	case 5:
		mddpNotifyWifiOnEnd(FALSE);
		break;
	case 6:
		mddpNotifyWifiOffStart();
		break;
	case 7:
		mddpNotifyWifiOffEnd();
		break;
	case 8:
		mddp_test_get_acs_best_ch(prAd, pcCommand,
			i4TotalLen);
		break;
	case 9:
		mddp_test_ch_switch_event(prAd, pcCommand,
			i4TotalLen, i4Argc, apcArgv);
		break;
	default:
		DBGLOG(INIT, ERROR, "unplement testcase(%u)!\n",
			u4TestCase);
		break;
	}
	return 0;
}
#endif /* CFG_MTK_SUPPORT_LIGHT_MDDP */

int priv_driver_dump_wfsys_cpupcr(struct net_device *prNetDev,
				  char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo;
	struct ADAPTER *prAdapter;
	struct mt66xx_chip_info *chip_info;
	struct CHIP_DBG_OPS *debug_ops;
	int32_t i4BytesWritten = 0;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	if (prGlueInfo->u4ReadyFlag == 0 || kalIsResetting()) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -1;
	}

	prAdapter = prGlueInfo->prAdapter;
	chip_info = prAdapter->chip_info;
	debug_ops = chip_info->prDebugOps;

	wlanAcquirePowerControl(prAdapter);
	if (!prAdapter->fgIsFwOwn) {
		if (debug_ops && debug_ops->dumpwfsyscpupcr)
			debug_ops->dumpwfsyscpupcr(prAdapter);
		else
			DBGLOG(REQ, WARN,
				"dumpwfsyscpupcr cb is NOT supported.\n");
	} else {
		DBGLOG(REQ, WARN, "driver own failed.\n");
	}
	wlanReleasePowerControl(prAdapter);

	return i4BytesWritten;
}

#if CFG_SUPPORT_SCAN_EXT_FLAG
/* Xiaom ADD
 * argv: 1 means that force scan on gaming mode
 */
int priv_driver_set_scan_ext_flag(struct net_device *prNetDev,
				char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	int32_t i4Argc = 0;
	int32_t u4Ret = 0;
	uint32_t u4ScanExtFlag = 0;
	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	DBGLOG(REQ, INFO, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);
	if (i4Argc <= 1) {
		DBGLOG(REQ, ERROR, "Argc(%d) ERR: Scan Ext Flag\n", i4Argc);
		return -1;
	}
	u4Ret = kalkStrtou32(apcArgv[1], 0, &u4ScanExtFlag);
	if (u4Ret)
		DBGLOG(REQ, ERROR, "parse scan ext flag error u4Ret=%d\n", u4Ret);
	if (prGlueInfo == NULL) {
		DBGLOG(REQ, ERROR, "prGlueInfo is null\n");
		return -1;
	}
	prGlueInfo->u4ScanExtFlag = u4ScanExtFlag;
	return WLAN_STATUS_SUCCESS;
}
#endif
