/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*
** Id: include/mgmt/rsn.h#1
*/

/*! \file   rsn.h
*    \brief  The wpa/rsn related define, macro and structure are described here.
*/


#ifndef _RSN_H
#define _RSN_H

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/
/* ----- Definitions for Cipher Suite Selectors ----- */
#define RSN_CIPHER_SUITE_USE_GROUP_KEY  0x00AC0F00
#define RSN_CIPHER_SUITE_WEP40          0x01AC0F00
#define RSN_CIPHER_SUITE_TKIP           0x02AC0F00
#define RSN_CIPHER_SUITE_CCMP           0x04AC0F00
#define RSN_CIPHER_SUITE_WEP104         0x05AC0F00
#if CFG_SUPPORT_802_11W
#define RSN_CIPHER_SUITE_AES_128_CMAC   0x06AC0F00
#endif
#if CFG_SUPPORT_CFG80211_AUTH
#define RSN_CIPHER_SUITE_GROUP_NOT_USED 0x07AC0F00
#define RSN_CIPHER_SUITE_GCMP           0x08AC0F00
#define RSN_CIPHER_SUITE_GCMP_256       0x09AC0F00
#define RSN_CIPHER_SUITE_CCMP_256       0x0AAC0F00
#define RSN_CIPHER_SUITE_BIP_GMAC_128   0x0BAC0F00
#define RSN_CIPHER_SUITE_BIP_GMAC_256   0x0CAC0F00
#define RSN_CIPHER_SUITE_BIP_CMAC_256   0x0DAC0F00
#endif

#define WPA_CIPHER_SUITE_NONE           0x00F25000
#define WPA_CIPHER_SUITE_WEP40          0x01F25000
#define WPA_CIPHER_SUITE_TKIP           0x02F25000
#define WPA_CIPHER_SUITE_CCMP           0x04F25000
#define WPA_CIPHER_SUITE_WEP104         0x05F25000

/* ----- Definitions for Authentication and Key Management Suite Selectors ----- */
#define RSN_AKM_SUITE_NONE              0x00AC0F00
#define RSN_AKM_SUITE_802_1X            0x01AC0F00
#define RSN_AKM_SUITE_PSK               0x02AC0F00

#define RSN_AKM_SUITE_FT_802_1X         0x03AC0F00
#define RSN_AKM_SUITE_FT_PSK            0x04AC0F00

#if KERNEL_VERSION(4, 12, 0) < LINUX_VERSION_CODE
#ifndef LINUX_IEEE80211_H
#define WLAN_AKM_SUITE_FT_8021X         0x000FAC03
#define WLAN_AKM_SUITE_FT_PSK           0x000FAC04
#endif
#else
#define WLAN_AKM_SUITE_FT_8021X         0x000FAC03
#define WLAN_AKM_SUITE_FT_PSK           0x000FAC04
#endif


#if CFG_SUPPORT_802_11W
#define RSN_AKM_SUITE_802_1X_SHA256     0x05AC0F00
#define RSN_AKM_SUITE_PSK_SHA256        0x06AC0F00
#endif
#if CFG_SUPPORT_CFG80211_AUTH
#define RSN_AKM_SUITE_SAE               0x08AC0F00
#define RSN_AKM_SUITE_8021X_SUITE_B     0x0BAC0F00
#define RSN_AKM_SUITE_8021X_SUITE_B_192 0x0CAC0F00
#define RSN_AKM_SUITE_OWE               0x12AC0F00
#endif

#define RSN_AKM_SUITE_DPP               0x029A6F50


#define WPA_AKM_SUITE_NONE              0x00F25000
#define WPA_AKM_SUITE_802_1X            0x01F25000
#define WPA_AKM_SUITE_PSK               0x02F25000

#if CFG_SUPPORT_CFG80211_AUTH
#define WLAN_CIPHER_SUITE_NO_GROUP_ADDR 0x000fac07
#endif
#define WLAN_AKM_SUITE_DPP              0x506F9A02

#define ELEM_ID_RSN_LEN_FIXED           20	/* The RSN IE len for associate request */

#define ELEM_ID_WPA_LEN_FIXED           22	/* The RSN IE len for associate request */

#define MASK_RSNIE_CAP_PREAUTH          BIT(0)

#define GET_SELECTOR_TYPE(x)           ((UINT_8)(((x) >> 24) & 0x000000FF))
#define SET_SELECTOR_TYPE(x, y)		(x = (((x) & 0x00FFFFFF) | (((UINT_32)(y) << 24) & 0xFF000000)))

#define AUTH_CIPHER_CCMP                0x00000008

/* Cihpher suite flags */
#define CIPHER_FLAG_NONE                        0x00000000
#define CIPHER_FLAG_WEP40                       0x00000001	/* BIT 1 */
#define CIPHER_FLAG_TKIP                        0x00000002	/* BIT 2 */
#define CIPHER_FLAG_CCMP                        0x00000008	/* BIT 4 */
#define CIPHER_FLAG_WEP104                      0x00000010	/* BIT 5 */
#define CIPHER_FLAG_WEP128                      0x00000020	/* BIT 6 */
#if CFG_SUPPORT_SUITB
#define CIPHER_FLAG_GCMP256                     0x00000080  /* BIT 7 */
#endif

#define WAIT_TIME_IND_PMKID_CANDICATE_SEC       6	/* seconds */
#define TKIP_COUNTERMEASURE_SEC                 60	/* seconds */

#if CFG_SUPPORT_802_11W
#define RSN_AUTH_MFP_DISABLED   0	/* MFP disabled */
#define RSN_AUTH_MFP_OPTIONAL   1	/* MFP optional */
#define RSN_AUTH_MFP_REQUIRED   2	/* MFP required */
#endif


#define GTK_REKEY_CMD_MODE_OFFLOAD_ON		0
#define GTK_REKEY_CMD_MODE_OFLOAD_OFF		1
#define GTK_REKEY_CMD_MODE_SET_BCMC_PN		2
#define GTK_REKEY_CMD_MODE_GET_BCMC_PN		3
#define GTK_REKEY_CMD_MODE_RPY_OFFLOAD_ON	4
#define GTK_REKEY_CMD_MODE_RPY_OFFLOAD_OFF	5

#define SA_QUERY_RETRY_TIMEOUT	3000
#define SA_QUERY_TIMEOUT	501

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/* Flags for PMKID Candidate list structure */
#define EVENT_PMKID_CANDIDATE_PREAUTH_ENABLED   0x01

#define CONTROL_FLAG_UC_MGMT_NO_ENC             BIT(5)

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/
#define RSN_IE(fp)              ((P_RSN_INFO_ELEM_T) fp)
#define WPA_IE(fp)              ((P_WPA_INFO_ELEM_T) fp)

#define ELEM_MAX_LEN_ASSOC_RSP_WSC_IE          (32 - ELEM_HDR_LEN)
#define ELEM_MAX_LEN_TIMEOUT_IE          (5)

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
BOOLEAN rsnParseRsnIE(IN P_ADAPTER_T prAdapter, IN P_RSN_INFO_ELEM_T prInfoElem, OUT P_RSN_INFO_T prRsnInfo);

BOOLEAN rsnParseWpaIE(IN P_ADAPTER_T prAdapter, IN P_WPA_INFO_ELEM_T prInfoElem, OUT P_RSN_INFO_T prWpaInfo);

BOOLEAN rsnSearchSupportedCipher(IN P_ADAPTER_T prAdapter, IN UINT_32 u4Cipher, OUT PUINT_32 pu4Index);

BOOLEAN rsnIsSuitableBSS(IN P_ADAPTER_T prAdapter, IN P_RSN_INFO_T prBssRsnInfo);

BOOLEAN rsnSearchAKMSuite(IN P_ADAPTER_T prAdapter, IN UINT_32 u4AkmSuite, OUT PUINT_32 pu4Index);

BOOLEAN rsnPerformPolicySelection(IN P_ADAPTER_T prAdapter, IN P_BSS_DESC_T prBss);

VOID rsnGenerateWpaNoneIE(IN P_ADAPTER_T prAdapter, IN P_MSDU_INFO_T prMsduInfo);

VOID rsnGenerateWPAIE(IN P_ADAPTER_T prAdapter, IN P_MSDU_INFO_T prMsduInfo);

VOID rsnGenerateRSNIE(IN P_ADAPTER_T prAdapter, IN P_MSDU_INFO_T prMsduInfo);

BOOLEAN
rsnParseCheckForWFAInfoElem(IN P_ADAPTER_T prAdapter,
			    IN PUINT_8 pucBuf, OUT PUINT_8 pucOuiType, OUT PUINT_16 pu2SubTypeVersion);

#if CFG_SUPPORT_AAA
void rsnParserCheckForRSNCCMPPSK(P_ADAPTER_T prAdapter, P_RSN_INFO_ELEM_T prIe, P_STA_RECORD_T prStaRec,
	PUINT_16 pu2StatusCode);
#endif

VOID rsnTkipHandleMICFailure(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prSta, IN BOOLEAN fgErrorKeyType);

VOID rsnSelectPmkidCandidateList(IN P_ADAPTER_T prAdapter, IN P_BSS_DESC_T prBssDesc);

VOID rsnUpdatePmkidCandidateList(IN P_ADAPTER_T prAdapter, IN P_BSS_DESC_T prBssDesc);

BOOLEAN rsnSearchPmkidEntry(IN P_ADAPTER_T prAdapter, IN PUINT_8 pucBssid, OUT PUINT_32 pu4EntryIndex);

BOOLEAN rsnCheckPmkidCandicate(IN P_ADAPTER_T prAdapter);

VOID rsnCheckPmkidCache(IN P_ADAPTER_T prAdapter, IN P_BSS_DESC_T prBss);

VOID rsnGeneratePmkidIndication(IN P_ADAPTER_T prAdapter);

VOID rsnIndicatePmkidCand(IN P_ADAPTER_T prAdapter, IN ULONG ulParamPtr);
#if CFG_SUPPORT_WPS2
VOID rsnGenerateWSCIE(IN P_ADAPTER_T prAdapter, IN P_MSDU_INFO_T prMsduInfo);
#endif

#if CFG_SUPPORT_802_11W
UINT_32 rsnCheckBipKeyInstalled(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prStaRec);

UINT_8 rsnCheckSaQueryTimeout(IN P_ADAPTER_T prAdapter);

void rsnStartSaQueryTimer(IN P_ADAPTER_T prAdapter, IN ULONG ulParamPtr);

void rsnStartSaQuery(IN P_ADAPTER_T prAdapter);

void rsnStopSaQuery(IN P_ADAPTER_T prAdapter);

void rsnSaQueryRequest(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb);

void rsnSaQueryAction(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb);

UINT_16 rsnPmfCapableValidation(IN P_ADAPTER_T prAdapter, IN P_BSS_INFO_T prBssInfo, IN P_STA_RECORD_T prStaRec);

VOID rsnPmfGenerateTimeoutIE(P_ADAPTER_T prAdapter, P_MSDU_INFO_T prMsduInfo);

void rsnApStartSaQuery(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prStaRec);

void rsnApSaQueryAction(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb);

#endif /* CFG_SUPPORT_802_11W */

#if CFG_SUPPORT_AAA
VOID rsnGenerateWSCIEForAssocRsp(P_ADAPTER_T prAdapter, P_MSDU_INFO_T prMsduInfo);
#endif

#if CFG_SUPPORT_OWE
void rsnGenerateOWEIE(IN P_ADAPTER_T prAdapter,
			IN P_MSDU_INFO_T prMsduInfo);

UINT_32 rsnCalOweIELen(IN P_ADAPTER_T prAdapter,
	IN UINT_8 ucBssIndex, P_STA_RECORD_T prStaRec);
#endif

void rsnGenerateRSNXE(IN P_ADAPTER_T prAdapter,
	IN OUT P_MSDU_INFO_T prMsduInfo);

uint32_t rsnCalRSNXELen(IN P_ADAPTER_T prAdapter,
	IN uint8_t ucBssIndex, P_STA_RECORD_T prStaRec);

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

#endif /* _RSN_H */
