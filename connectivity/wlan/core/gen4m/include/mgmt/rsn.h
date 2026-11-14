/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   rsn.h
 *  \brief  The wpa/rsn related define, macro and structure are described here.
 */

#ifndef _RSN_H
#define _RSN_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
/**/

/* Indicates maximum number of AKM suites allowed for NL80211_CMD_CONNECT,
 * NL80211_CMD_ASSOCIATE and NL80211_CMD_START_AP in NL80211_CMD_GET_WIPHY
 * response.
 */
#define RSN_MAX_NR_AKN_SUITES		3

/* Add AKM SUITE since kernel haven't defined it. */
#ifndef WLAN_AKM_SUITE_FT_8021X
#define WLAN_AKM_SUITE_FT_8021X         0x000FAC03
#endif
#ifndef WLAN_AKM_SUITE_FT_PSK
#define WLAN_AKM_SUITE_FT_PSK           0x000FAC04
#endif
#ifndef WLAN_AKM_SUITE_8021X_SUITE_B
#define WLAN_AKM_SUITE_8021X_SUITE_B    0x000FAC0B
#endif
#ifndef WLAN_AKM_SUITE_FT_OVER_SAE
#define WLAN_AKM_SUITE_FT_OVER_SAE      0x000FAC09
#endif
#ifndef WLAN_AKM_SUITE_8021X_SUITE_B_192
#define WLAN_AKM_SUITE_8021X_SUITE_B_192 0x000FAC0C
#endif
#ifndef WLAN_AKM_SUITE_FILS_SHA256
#define WLAN_AKM_SUITE_FILS_SHA256	0x000FAC0E
#endif
#ifndef WLAN_AKM_SUITE_FILS_SHA384
#define WLAN_AKM_SUITE_FILS_SHA384	0x000FAC0F
#endif
#ifndef WLAN_AKM_SUITE_FT_FILS_SHA256
#define WLAN_AKM_SUITE_FT_FILS_SHA256	0x000FAC10
#endif
#ifndef WLAN_AKM_SUITE_FT_FILS_SHA384
#define WLAN_AKM_SUITE_FT_FILS_SHA384	0x000FAC11
#endif
#ifndef WLAN_AKM_SUITE_OWE
#define WLAN_AKM_SUITE_OWE              0x000FAC12
#endif
#ifndef WLAN_AKM_SUITE_FT_802_1X_SHA384_UNRESTRICTED
#define WLAN_AKM_SUITE_FT_802_1X_SHA384_UNRESTRICTED	0x000FAC16
#endif
#ifndef WLAN_AKM_SUITE_8021X_SHA384
#define WLAN_AKM_SUITE_8021X_SHA384	0x000FAC17
#endif
#ifndef WLAN_AKM_SUITE_SAE_EXT_KEY
#define WLAN_AKM_SUITE_SAE_EXT_KEY	0x000FAC18
#endif
#ifndef WLAN_AKM_SUITE_FT_SAE_EXT_KEY
#define WLAN_AKM_SUITE_FT_SAE_EXT_KEY	0x000FAC19
#endif

/* ----- Definitions for Cipher Suite Selectors ----- */
#define RSN_CIPHER_SUITE_USE_GROUP_KEY  0x00AC0F00
#define RSN_CIPHER_SUITE_WEP40          0x01AC0F00
#define RSN_CIPHER_SUITE_TKIP           0x02AC0F00
#define RSN_CIPHER_SUITE_CCMP           0x04AC0F00
#define RSN_CIPHER_SUITE_WEP104         0x05AC0F00
#define RSN_CIPHER_SUITE_AES_128_CMAC   0x06AC0F00
#define RSN_CIPHER_SUITE_BIP_CMAC_128   0x06AC0F00
#define RSN_CIPHER_SUITE_GROUP_NOT_USED 0x07AC0F00
#define RSN_CIPHER_SUITE_GCMP           0x08AC0F00
#define RSN_CIPHER_SUITE_GCMP_256       0x09AC0F00
#define RSN_CIPHER_SUITE_CCMP_256       0x0AAC0F00
#define RSN_CIPHER_SUITE_BIP_GMAC_128   0x0BAC0F00
#define RSN_CIPHER_SUITE_BIP_GMAC_256   0x0CAC0F00
#define RSN_CIPHER_SUITE_BIP_CMAC_256   0x0DAC0F00

#define WPA_CIPHER_SUITE_NONE           0x00F25000
#define WPA_CIPHER_SUITE_WEP40          0x01F25000
#define WPA_CIPHER_SUITE_TKIP           0x02F25000
#define WPA_CIPHER_SUITE_CCMP           0x04F25000
#define WPA_CIPHER_SUITE_WEP104         0x05F25000

/* sync with dot11RSNAConfigPairwiseCiphersTable */
#define WPA_CIPHER_SUITE_WEP40_BIT		BIT(0)
#define	WPA_CIPHER_SUITE_TKIP_BIT		BIT(1)
#define WPA_CIPHER_SUITE_CCMP_BIT		BIT(2)
#define WPA_CIPHER_SUITE_WEP104_BIT		BIT(3)
#define RSN_CIPHER_SUITE_WEP40_BIT		BIT(4)
#define RSN_CIPHER_SUITE_TKIP_BIT		BIT(5)
#define RSN_CIPHER_SUITE_CCMP_BIT		BIT(6)
#define RSN_CIPHER_SUITE_WEP104_BIT		BIT(7)
#define RSN_CIPHER_SUITE_GROUP_NOT_USED_BIT	BIT(8)
#define RSN_CIPHER_SUITE_GCMP_256_BIT		BIT(9)
#define RSN_CIPHER_SUITE_GCMP_BIT		BIT(10)


/* Definitions for Authentication and Key Management Suite Selectors */
#define WPA_AKM_SUITE_NONE              0x00F25000
#define WPA_AKM_SUITE_802_1X            0x01F25000
#define WPA_AKM_SUITE_PSK               0x02F25000

#define RSN_AKM_SUITE_NONE              0x00AC0F00
#define RSN_AKM_SUITE_802_1X            0x01AC0F00
#define RSN_AKM_SUITE_PSK               0x02AC0F00
#define RSN_AKM_SUITE_FT_802_1X         0x03AC0F00
#define RSN_AKM_SUITE_FT_PSK            0x04AC0F00
#define RSN_AKM_SUITE_802_1X_SHA256     0x05AC0F00
#define RSN_AKM_SUITE_PSK_SHA256        0x06AC0F00
#define RSN_AKM_SUITE_TDLS              0x07AC0F00
#define RSN_AKM_SUITE_SAE               0x08AC0F00
#define RSN_AKM_SUITE_FT_OVER_SAE       0x09AC0F00
#define RSN_AKM_SUITE_8021X_SUITE_B     0x0BAC0F00
#define RSN_AKM_SUITE_8021X_SUITE_B_192 0x0CAC0F00
#define RSN_AKM_SUITE_FT_802_1X_SHA384  0x0DAC0F00
#define RSN_AKM_SUITE_FILS_SHA256       0x0EAC0F00
#define RSN_AKM_SUITE_FILS_SHA384       0x0FAC0F00
#define RSN_AKM_SUITE_FT_FILS_SHA256    0x10AC0F00
#define RSN_AKM_SUITE_FT_FILS_SHA384    0x11AC0F00
#define RSN_AKM_SUITE_OWE               0x12AC0F00
#define RSN_AKM_SUITE_FT_PSK_SHA384     0x13AC0F00
#define RSN_AKM_SUITE_PSK_SHA384        0x14AC0F00
#define RSN_AKM_SUITE_PASN              0x15AC0F00
#define RSN_AKM_SUITE_FT_802_1X_SHA384_UNRESTRICTED      0x16AC0F00
#define RSN_AKM_SUITE_8021X_SHA384      0x17AC0F00
#define RSN_AKM_SUITE_SAE_EXT_KEY       0x18AC0F00
#define RSN_AKM_SUITE_FT_SAE_EXT_KEY    0x19AC0F00
#define RSN_AKM_SUITE_OSEN              0x019A6F50
#define RSN_AKM_SUITE_DPP               0x029A6F50

/* this define should be in ieee80211.h, but kernel didn't update it.
 * so we define here temporary
 */
#define WLAN_AKM_SUITE_OSEN             0x506f9a01
#define WLAN_CIPHER_SUITE_NO_GROUP_ADDR 0x000fac07
#define WLAN_AKM_SUITE_DPP              0x506F9A02

/* sync with dot11RSNAConfigAuthenticationSuitesTable */
#define WPA_AKM_SUITE_NONE_BIT			BIT(0)
#define WPA_AKM_SUITE_802_1X_BIT		BIT(1)
#define WPA_AKM_SUITE_PSK_BIT			BIT(2)
#define RSN_AKM_SUITE_NONE_BIT			BIT(3)
#define RSN_AKM_SUITE_802_1X_BIT		BIT(4)
#define RSN_AKM_SUITE_PSK_BIT			BIT(5)
#define RSN_AKM_SUITE_FT_802_1X_BIT		BIT(6)
#define RSN_AKM_SUITE_FT_PSK_BIT		BIT(7)
#define RSN_AKM_SUITE_OSEN_BIT			BIT(8)
#define RSN_AKM_SUITE_SAE_BIT			BIT(9)
#define RSN_AKM_SUITE_OWE_BIT			BIT(10)
#define RSN_AKM_SUITE_DPP_BIT			BIT(11)
#define RSN_AKM_SUITE_8021X_SUITE_B_192_BIT	BIT(12)
#define RSN_AKM_SUITE_SAE_EXT_KEY_BIT		BIT(13)
#define RSN_AKM_SUITE_802_1X_SHA256_BIT		BIT(14)
#define RSN_AKM_SUITE_PSK_SHA256_BIT		BIT(15)
#define RSN_AKM_SUITE_FT_OVER_SAE_BIT		BIT(16)
#define RSN_AKM_SUITE_FT_SAE_EXT_KEY_BIT	BIT(17)

/* The RSN IE len for associate request */
#define ELEM_ID_RSN_LEN_FIXED           20

/* The WPA IE len for associate request */
#define ELEM_ID_WPA_LEN_FIXED           22

#define MASK_RSNIE_CAP_PREAUTH          BIT(0)

#define GET_SELECTOR_TYPE(x)           ((uint8_t)(((x) >> 24) & 0x000000FF))
#define SET_SELECTOR_TYPE(x, y)		(x = (((x) & 0x00FFFFFF) | \
					(((uint32_t)(y) << 24) & 0xFF000000)))

#define AUTH_CIPHER_CCMP                0x00000008

/* Cihpher suite flags */
#define CIPHER_FLAG_NONE                        0x00000000
#define CIPHER_FLAG_WEP40                       0x00000001	/* BIT 1 */
#define CIPHER_FLAG_TKIP                        0x00000002	/* BIT 2 */
#define CIPHER_FLAG_CCMP                        0x00000008	/* BIT 4 */
#define CIPHER_FLAG_WEP104                      0x00000010	/* BIT 5 */
#define CIPHER_FLAG_WEP128                      0x00000020	/* BIT 6 */
#define CIPHER_FLAG_GCMP128                     0x00000040      /* BIT 7 */
#define CIPHER_FLAG_GCMP256                     0x00000080	/* BIT 8 */

#define TKIP_COUNTERMEASURE_SEC                 60	/* seconds */

#if CFG_SUPPORT_802_11W
/* sync with IW_AUTH_MFP_XXXX */
#define RSN_AUTH_MFP_DISABLED		0	/* MFP disabled */
#define RSN_AUTH_MFP_OPTIONAL		1	/* MFP optional */
#define RSN_AUTH_MFP_REQUIRED		2	/* MFP required */
#endif

/* Extended RSN Capabilities */
/* bits 0-3: Field length (n-1) */
#define WLAN_RSNX_CAPAB_PROTECTED_TWT 4
#define WLAN_RSNX_CAPAB_SAE_H2E 5
#define WLAN_RSNX_CAPAB_SAE_PK 6
#define WLAN_RSNX_CAPAB_SECURE_LTF 8
#define WLAN_RSNX_CAPAB_SECURE_RTT 9
#define WLAN_RSNX_CAPAB_PROT_RANGE_NEG 10

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
#define GTK_REKEY_CMD_MODE_OFFLOAD_ON           0
#define GTK_REKEY_CMD_MODE_OFLOAD_OFF           1
#define GTK_REKEY_CMD_MODE_OFFLOAD_UPDATE       2
#define GTK_REKEY_CMD_MODE_SET_BCMC_PN          3
#define GTK_REKEY_CMD_MODE_GET_BCMC_PN          4
#define GTK_REKEY_CMD_MODE_RPY_OFFLOAD_ON       5
#define GTK_REKEY_CMD_MODE_RPY_OFFLOAD_OFF      6

/* sub-mode for GTK_REKEY_CMD_MODE_OFFLOAD_UPDATE */
#define GTK_REKEY_UPDATE_AND_ON                 0
#define GTK_REKEY_UPDATE_ONLY                   1
#else
/* CFG_BC_MC_RP_DETECTION_FWOFFLOAD:             */
/* for GTK rekey and BC/MC replay detection use. */
#define GTK_REKEY_CMD_MODE_OFFLOAD_ON           0
#define GTK_REKEY_CMD_MODE_OFLOAD_OFF           1
#define GTK_REKEY_CMD_MODE_SET_BCMC_PN          2
#define GTK_REKEY_CMD_MODE_GET_BCMC_PN          3
#define GTK_REKEY_CMD_MODE_RPY_OFFLOAD_ON       4
#define GTK_REKEY_CMD_MODE_RPY_OFFLOAD_OFF      5
#endif

#define SA_QUERY_RETRY_TIMEOUT	3000
#define SA_QUERY_TIMEOUT	501

#define PMK_REFRESH_THRESHOLD_SEC	600

#ifndef WPA_NONCE_LEN
#define WPA_NONCE_LEN   32
#endif
#ifndef WPA_EAPOL_KEY_FIELD_SIZE
#define WPA_EAPOL_KEY_FIELD_SIZE  95  /* struct wpa_eapol_key */
#endif
#ifndef WPA_KEY_INFO_KEY_TYPE
#define WPA_KEY_INFO_KEY_TYPE  BIT(3) /* 1 = Pairwise, 0 = Group key */
#endif
#ifndef WPA_KEY_INFO_INSTALL
#define WPA_KEY_INFO_INSTALL   BIT(6)
#endif
#ifndef WPA_KEY_INFO_ACK
#define WPA_KEY_INFO_ACK       BIT(7)
#endif
#ifndef WPA_KEY_INFO_SECURE
#define WPA_KEY_INFO_SECURE    BIT(9)
#endif
#ifndef ieee802_1x_hdr_size
/* struct ieee802_1x_hdr in wpa_supplicant */
#define ieee802_1x_hdr_size 4
#endif
#ifndef wpa_eapol_key_key_info_offset
/* struct wpa_eapol_key in wpa_supplicant */
#define wpa_eapol_key_key_info_offset 1
#endif
#ifndef wpa_eapol_key_nonce_info_offset
/* struct wpa_eapol_key in wpa_supplicant */
#define wpa_eapol_key_nonce_info_offset 13
#endif /* wpa_eapol_key_nonce_info_offset */
#ifndef wpa_eapol_key_fixed_field_size
#define wpa_eapol_key_fixed_field_size 77
#endif

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/* Flags for PMKID Candidate list structure */
#define EVENT_PMKID_CANDIDATE_PREAUTH_ENABLED   0x01

#define CONTROL_FLAG_UC_MGMT_NO_ENC             BIT(5)

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

struct MSG_MIC_ERROR {
	struct MSG_HDR rMsgHdr; /* Must be the first member */
	struct STA_RECORD *prStaRec;
	u_int8_t fgFlags;
};

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */
#define RSN_IE(fp)              ((struct RSN_INFO_ELEM *) fp)
#define WPA_IE(fp)              ((struct WPA_INFO_ELEM *) fp)
#define RSNX_IE(fp)             ((struct RSNX_INFO_ELEM *) fp)

#define ELEM_MAX_LEN_ASSOC_RSP_WSC_IE          (32 - ELEM_HDR_LEN)
#define ELEM_MAX_LEN_TIMEOUT_IE          (5)

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
u_int8_t rsnParseRsnIE(struct ADAPTER *prAdapter,
		       uint8_t *prInfoElem,
		       struct RSN_INFO *prRsnInfo);

u_int8_t rsnParseWpaIE(struct ADAPTER *prAdapter,
		       struct WPA_INFO_ELEM *prInfoElem,
		       struct RSN_INFO *prWpaInfo);

u_int8_t rsnSearchSupportedCipher(struct ADAPTER *prAdapter,
				  uint32_t u4Cipher,
				  uint8_t ucBssIndex);

void rsnDumpSupportedCipher(struct ADAPTER *prAdapter, uint8_t ucBssIndex);

u_int8_t rsnIsSuitableBSS(struct ADAPTER *prAdapter,
			  struct BSS_DESC *prBss,
			  struct RSN_INFO *prBssRsnInfo,
			  uint8_t ucBssIndex);

u_int8_t rsnSearchAKMSuite(struct ADAPTER *prAdapter,
			   uint32_t u4AkmSuite, uint8_t ucBssIndex);

void rsnDumpSupportedAKMSuite(struct ADAPTER *prAdapter, uint8_t ucBssIndex);

uint8_t rsnSearchFTSuite(struct ADAPTER *prAdapter, uint8_t ucBssIndex);

uint8_t rsnAuthModeRsn(enum ENUM_PARAM_AUTH_MODE eAuthMode);

u_int8_t rsnPerformPolicySelection(struct ADAPTER
				   *prAdapter,
				   struct BSS_DESC *prBss,
				   uint8_t ucBssIndex);

void rsnGenerateWpaNoneIE(struct ADAPTER *prAdapter,
			  struct MSDU_INFO *prMsduInfo);

void rsnGenerateWPAIE(struct ADAPTER *prAdapter,
		      struct MSDU_INFO *prMsduInfo);

void rsnGenerateRSNIE(struct ADAPTER *prAdapter,
		      struct MSDU_INFO *prMsduInfo);

void rsnGenerateRSNIEImpl(struct ADAPTER *prAdapter,
		      struct MSDU_INFO *prMsduInfo);

#if (CFG_SUPPORT_RSNO == 1)
void rsnGenerateRsnSelectionIE(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo);
u_int8_t rsnSupportRSNOverride(struct ADAPTER *prAdapter, uint8_t ucBssIndex);
#endif /* CFG_SUPPORT_RSNO */

#if CFG_SUPPORT_AAA
void rsnGenerateRSNXIE(struct ADAPTER *prAdapter,
		      struct MSDU_INFO *prMsduInfo);

void rsnGenerateOWEIE(struct ADAPTER *prAdapter,
		      struct MSDU_INFO *prMsduInfo);

#if (CFG_SUPPORT_RSNO == 1)
void rsnGenerateRSNOIE(struct ADAPTER *prAdapter,
			struct MSDU_INFO *prMsduInfo);

void rsnGenerateRSNO2IE(struct ADAPTER *prAdapter,
			struct MSDU_INFO *prMsduInfo);

void rsnGenerateRSNXOIE(struct ADAPTER *prAdapter,
			struct MSDU_INFO *prMsduInfo);
#endif /* CFG_SUPPORT_RSNO */
#endif /* CFG_SUPPORT_AAA */

u_int8_t
rsnParseCheckForWFAInfoElem(struct ADAPTER *prAdapter,
			    uint8_t *pucBuf, uint8_t *pucOuiType,
			    uint16_t *pu2SubTypeVersion);

u_int8_t
rsnParseCheckForWFASpecificElem(struct ADAPTER *prAdapter,
			    uint8_t *pucBuf, uint8_t *pucOuiType);

#if CFG_SUPPORT_AAA
void rsnParserCheckForRSNCCMPPSK(struct ADAPTER *prAdapter,
				 struct RSN_INFO_ELEM *prIe,
				 struct STA_RECORD *prStaRec,
				 uint16_t *pu2StatusCode);
#endif

void rsnMicErrorHandleMsg(struct ADAPTER *prAdapter,
				struct MSG_HDR *prMsgHdr);

void rsnTkipHandleMICFailure(struct ADAPTER *prAdapter,
			     struct STA_RECORD *prSta,
			     u_int8_t fgErrorKeyType);

struct PMKID_ENTRY *rsnSearchPmkidEntry(struct ADAPTER *prAdapter,
					uint8_t *pucBssid,
					uint8_t ucBssIndex);

struct PMKID_ENTRY *rsnSearchPmkidEntryEx(struct ADAPTER *prAdapter,
					uint8_t *pucBssid,
					struct PARAM_SSID *prSsid,
					uint8_t *pucFilsCacheId,
					uint8_t ucBssIndex);

void rsnGeneratePmkidIndication(struct ADAPTER *prAdapter,
				struct PARAM_PMKID_CANDIDATE *prCandi,
				uint8_t ucBssIndex);

uint32_t rsnSetPmkid(struct ADAPTER *prAdapter,
		     struct PARAM_PMKID *prPmkid);

uint32_t rsnDelPmkid(struct ADAPTER *prAdapter,
		     struct PARAM_PMKID *prPmkid);

uint32_t rsnFlushPmkid(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

uint8_t rsnCheckPmkExpiration(struct ADAPTER *prAdapter,
				struct PMKID_ENTRY *targetEntry,
				uint8_t ucBssIndex);

#if CFG_SUPPORT_802_11W
uint32_t rsnCheckBipKeyInstalled(struct ADAPTER
				 *prAdapter, struct STA_RECORD *prStaRec);

uint32_t rsnCheckBipGmacKeyInstall(struct ADAPTER
				 *prAdapter, struct STA_RECORD *prStaRec);

uint8_t rsnCheckBipGmac(struct ADAPTER *prAdapter,
			struct SW_RFB *prSwRfb);

void rsnUpdateCombackBssDesc(struct ADAPTER *prAdapter, uint8_t ucBssIndex);

uint8_t rsnCheckCombackBssDesc(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec, uint8_t ucBssIndex);

void rsnResetCombackBssDesc(struct ADAPTER *prAdapter, uint8_t ucBssIndex);

uint8_t rsnCheckSaQueryTimeout(
	struct ADAPTER *prAdapter, uint8_t ucBssIdx);

void rsnStartSaQueryTimer(struct ADAPTER *prAdapter,
			  uintptr_t ulParamPtr);

void rsnStartSaQuery(struct ADAPTER *prAdapter,
	uint8_t ucBssIdx);

void rsnStopSaQuery(struct ADAPTER *prAdapter,
	uint8_t ucBssIdx);

void rsnSaQueryRequest(struct ADAPTER *prAdapter,
		       struct SW_RFB *prSwRfb);

void rsnSaQueryAction(struct ADAPTER *prAdapter,
		      struct SW_RFB *prSwRfb);

uint16_t rsnPmfCapableValidation(struct ADAPTER
				 *prAdapter, struct BSS_INFO *prBssInfo,
				 struct STA_RECORD *prStaRec);

void rsnPmfGenerateTimeoutIE(struct ADAPTER *prAdapter,
			     struct MSDU_INFO *prMsduInfo);
#endif /* CFG_SUPPORT_802_11W */

#if CFG_SUPPORT_AAA
#if CFG_SUPPORT_802_11W
void rsnApStartSaQuery(struct ADAPTER *prAdapter,
		       struct STA_RECORD *prStaRec);

void rsnApSaQueryAction(struct ADAPTER *prAdapter,
			struct SW_RFB *prSwRfb);
#endif


void rsnGenerateWSCIEForAssocRsp(struct ADAPTER *prAdapter,
				 struct MSDU_INFO *prMsduInfo);
#endif

u_int8_t rsnParseOsenIE(struct ADAPTER *prAdapter,
			struct IE_WFA_OSEN *prInfoElem,
			struct RSN_INFO *prOsenInfo);

#if CFG_SUPPORT_DETECT_SECURITY_MODE_CHANGE
u_int8_t rsnCheckSecurityModeChanged(struct ADAPTER
				     *prAdapter, struct BSS_INFO *prBssInfo,
				     struct BSS_DESC *prBssDesc);
#endif

uint32_t rsnCalculateFTIELen(struct ADAPTER *prAdapter, uint8_t ucBssIdx,
			     struct STA_RECORD *prStaRec);

void rsnGenerateFTIE(struct ADAPTER *prAdapter,
		     struct MSDU_INFO *prMsduInfo);

u_int8_t rsnIsFtOverTheAir(struct ADAPTER *prAdapter,
			uint8_t ucBssIdx, uint8_t ucStaRecIdx);

u_int8_t rsnParseRsnxIE(struct ADAPTER *prAdapter,
		       uint8_t *pucInfoElem,
		       struct RSNX_INFO *prRsnxeInfo);

uint8_t rsnKeyMgmtSae(uint32_t akm);
uint8_t rsnKeyMgmtFT(uint32_t akm);
uint32_t rsnKeyMgmtToAuthMode(enum ENUM_PARAM_AUTH_MODE eOriAuthMode,
	uint32_t version, uint32_t akm);
void rsnAllowCrossAkm(struct ADAPTER *prAdapter, uint8_t ucBssIndex);
uint32_t rsnCipherToBit(uint32_t cipher);
uint32_t rsnKeyMgmtToBit(uint32_t akm);
uint8_t rsnCipherSuiteSelectorToCipher(uint32_t selector);
uint32_t rsnCipherToCipherSuiteSelector(uint8_t cipher);
uint8_t rsnApOverload(uint16_t status, uint16_t reason);
uint8_t rsnApInvalidPMK(uint16_t status, uint16_t reason,
	enum ENUM_PARAM_AUTH_MODE AuthMode);
uint8_t rsnIsFilsAuthAlg(uint8_t alg);
uint8_t rsnKeyMgmtFils(uint32_t akm);
uint8_t rsnIsKeyMgmtSha256(uint32_t akm);
uint8_t rsnIsKeyMgmtSha384(uint32_t akm);
uint8_t rsnIsKeyMgmtIeee8021x(uint32_t akm);
uint8_t rsnIsKeyMgmtForEht(struct ADAPTER *ad,
	struct BSS_DESC *prBss);
uint8_t rsnIsKeyMgmtFor6g(struct ADAPTER *ad,
	uint32_t u4AkmSuite, struct BSS_DESC *prBss);
uint8_t rsnKekLen(uint32_t akmp, uint16_t pmk_len);
uint8_t rsnCipherKeyLen(uint32_t cipher);
void rsnReqDumpWTBL(struct ADAPTER *prAdapter,
	enum ENUM_FW_DUMP eType, uint8_t ucBssIndex);
void rsnTriggerDumpWTBL(struct ADAPTER *prAdapter,
	struct MSG_HDR *prMsgHdr);
void rsnDumpWTBL(struct ADAPTER *prAdapter);
bool rsnFwDumpIsLimited(struct ADAPTER *prAdapter);

#if (CFG_SUPPORT_SAP_BCN_PROT == 1)
uint32_t rsnCalculateMMIELen(struct ADAPTER *prAdapter,
			       uint8_t ucBssIndex, struct STA_RECORD *prStaRec);
void rsnGenerateMMIE(struct ADAPTER *prAdapter,
		     struct MSDU_INFO *prMsduInfo);
#endif

#if CFG_SUPPORT_802_11W
void rsnApStartSaQueryTimer(struct ADAPTER *prAdapter,
			    uintptr_t ulParamPtr);
#endif /* CFG_SUPPORT_802_11W */

u_int8_t rsnHasNonce(const uint8_t *pucNonceAddr);
uint8_t rsnGetEapolMicLen(uint32_t akmp);
uint16_t rsnGetEapolDataLen(uint8_t *pucEapol, uint8_t mic_len);
u_int8_t rsnIsEapolM2(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex, uint8_t *pucEapol);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#endif /* _RSN_H */
