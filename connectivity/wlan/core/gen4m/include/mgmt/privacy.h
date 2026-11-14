/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   privacy.h
 *  \brief This file contains the function declaration for privacy.c.
 */


#ifndef _PRIVACY_H
#define _PRIVACY_H

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
/* 0: unicast, 1-3: GTK, 4-5: IGTK, 6-7: BIGTK */
#define MAX_KEY_NUM                             8
#define WEP_40_LEN                              5
#define WEP_104_LEN                             13
#define WEP_128_LEN                             16
#define LEGACY_KEY_MAX_LEN                      16
#define CCMP_KEY_LEN                            16
#define TKIP_KEY_LEN                            32
#define MAX_KEY_LEN                             32
#define MIC_RX_KEY_OFFSET                       16
#define MIC_TX_KEY_OFFSET                       24
#define MIC_KEY_LEN                             8

#define WEP_KEY_ID_FIELD      BITS(0, 29)
#define KEY_ID_FIELD          BITS(0, 7)

#define IS_TRANSMIT_KEY       BIT(31)
#define IS_UNICAST_KEY        BIT(30)
#define IS_AUTHENTICATOR      BIT(28)

/* WTBL cipher selector, sync with HAL RX from hal_hw_def_rom.h */
#define CIPHER_SUITE_NONE               0
#define CIPHER_SUITE_WEP40              1
#define CIPHER_SUITE_TKIP               2
#define CIPHER_SUITE_TKIP_WO_MIC        3
#define CIPHER_SUITE_CCMP               4
#define CIPHER_SUITE_WEP104             5
#define CIPHER_SUITE_BIP                6
#define CIPHER_SUITE_BIP_CMAC_128       CIPHER_SUITE_BIP
#define CIPHER_SUITE_WEP128             7
#define CIPHER_SUITE_WPI                8
#define CIPHER_SUITE_CCMP_W_CCX         9 /* CCMP-128 for DFP or CCX MFP */
#define CIPHER_SUITE_CCMP_256           10
#define CIPHER_SUITE_GCMP_128           11
#define CIPHER_SUITE_GCMP_256           12
#define CIPHER_SUITE_GCM_WPI_128        13
#define CIPHER_SUITE_BIP_CMAC_256       14
#define CIPHER_SUITE_BCN_PROT_CMAC_128  15
#define CIPHER_SUITE_BCN_PROT_CMAC_256  16
#define CIPHER_SUITE_BIP_GMAC_256       17
#define CIPHER_SUITE_BCN_PROT_GMAC_128  18
#define CIPHER_SUITE_BCN_PROT_GMAC_256  19


/* Todo:: Move to register */
#if defined(MT6630)
#define WTBL_RESERVED_ENTRY             255
#else
#define WTBL_RESERVED_ENTRY             255
#endif
/* Todo:: By chip capability */
/* Max wlan table size, the max+1 used for probe request,... mgmt frame */
/*sending use basic rate and no security */
#ifdef CFG_WTBL_MAXSIZE
#define WTBL_SIZE                   CFG_WTBL_MAXSIZE
#else
#define WTBL_SIZE                   32
#endif


#define WTBL_ALLOC_FAIL                 WTBL_RESERVED_ENTRY
#define WTBL_DEFAULT_ENTRY              0
#define DUMP_WTBL_BUF_SIZE              900
#define ROW_OF_WTBL_SIZE                50
/*******************************************************************************
 *                         D A T A   T Y P E S
 *******************************************************************************
 */

struct IEEE_802_1X_HDR {
	uint8_t ucVersion;
	uint8_t ucType;
	uint16_t u2Length;
	/* followed by length octets of data */
};

struct EAPOL_KEY {
	uint8_t ucType;
	/* Note: key_info, key_length, and key_data_length are unaligned */
	uint8_t aucKeyInfo[2];	/* big endian */
	uint8_t aucKeyLength[2];	/* big endian */
	uint8_t aucReplayCounter[8];
	uint8_t aucKeyNonce[16];
	uint8_t aucKeyIv[16];
	uint8_t aucKeyRsc[8];
	uint8_t aucKeyId[8];	/* Reserved in IEEE 802.11i/RSN */
	uint8_t aucKeyMic[16];
	uint8_t aucKeyDataLength[2];	/* big endian */
	/* followed by key_data_length bytes of key_data */
};

/* WPA2 PMKID candicate structure */
struct PMKID_CANDICATE {
	uint8_t aucBssid[MAC_ADDR_LEN];
	uint32_t u4PreAuthFlags;
};

#if 0
/* WPA2 PMKID cache structure */
struct PMKID_ENTRY {
	struct PARAM_BSSID_INFO rBssidInfo;
	u_int8_t fgPmkidExist;
};
#endif

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */
/* TRUE if CM error || Unprotected UC frame */
#define IS_INCORRECT_SEC_RX_FRAME(_prSwRfb, _aucDestAddr, u2FrameCtrl) \
		(_prSwRfb->fgIsCipherMS || \
		(IS_UCAST_MAC_ADDR(_aucDestAddr) && \
			!(u2FrameCtrl & MASK_FC_PROTECTED_FRAME)))

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

void secInit(struct ADAPTER *prAdapter,
	     uint8_t ucBssIndex);

void secSetPortBlocked(struct ADAPTER *prAdapter,
		       struct STA_RECORD *prSta, u_int8_t fgPort);

u_int8_t secCheckClassError(struct ADAPTER *prAdapter,
			    struct SW_RFB *prSwRfb,
			    struct STA_RECORD *prStaRec);

u_int8_t secTxPortControlCheck(struct ADAPTER *prAdapter,
			       struct MSDU_INFO *prMsduInfo,
			       struct STA_RECORD *prStaRec);

u_int8_t secRxPortControlCheck(struct ADAPTER *prAdapter,
			       struct SW_RFB *prSWRfb);

void secSetCipherSuite(struct ADAPTER *prAdapter,
		       uint32_t u4CipherSuitesFlags,
		       uint8_t ucBssIndex);

u_int8_t secIsProtectedFrame(struct ADAPTER *prAdapter,
			     struct MSDU_INFO *prMsdu,
			     struct STA_RECORD *prStaRec);

u_int8_t secRsnKeyHandshakeEnabled(struct ADAPTER
				   *prAdapter);

uint8_t secGetBmcWlanIndex(struct ADAPTER *prAdapter,
			   enum ENUM_NETWORK_TYPE eNetType,
			   struct STA_RECORD *prStaRec);

u_int8_t secTransmitKeyExist(struct ADAPTER *prAdapter,
			     struct STA_RECORD *prSta);

u_int8_t secEnabledInAis(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex);

u_int8_t secPrivacySeekForEntry(struct ADAPTER
				*prAdapter, struct STA_RECORD *prSta);

void secPrivacyFreeForEntry(struct ADAPTER *prAdapter,
			    uint8_t ucEntry);

void secPrivacyResetForEntry(struct ADAPTER *prAdapter,
			    uint8_t ucEntry);

void secPrivacyFreeSta(struct ADAPTER *prAdapter,
		       struct STA_RECORD *prStaRec);

void secRemoveBssBcEntry(struct ADAPTER *prAdapter, struct BSS_INFO *prBssInfo);

uint8_t
secPrivacySeekForBcEntry(struct ADAPTER *prAdapter,
			 uint8_t ucBssIndex,
			 uint8_t *pucAddr, uint8_t ucStaIdx,
			 uint8_t ucAlg, uint8_t ucKeyId);

uint8_t secGetStaIdxByWlanIdx(struct ADAPTER *prAdapter,
			      uint8_t ucWlanIdx);

uint8_t secGetWlanIdxByStaIdx(struct ADAPTER *prAdapter,
				uint8_t ucStaIndex);

uint8_t secGetBssIdxByWlanIdx(struct ADAPTER *prAdapter,
			      uint8_t ucWlanIdx);

uint8_t secGetBssIdxByRfb(struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb);

struct BSS_INFO *secGetBssByRfb(struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb);

uint8_t secLookupStaRecIndexFromTA(struct ADAPTER
				   *prAdapter, uint8_t *pucMacAddress);

void secPrivacyDumpWTBL(struct ADAPTER *prAdapter);

uint8_t secCheckWTBLwlanIdxInUseByOther(struct ADAPTER *prAdapter,
			uint8_t ucWlanIdx, uint8_t ucBssIndex);

u_int8_t secCheckWTBLAssign(struct ADAPTER *prAdapter);

u_int8_t secIsProtectedBss(struct ADAPTER *prAdapter,
			   struct BSS_INFO *prBssInfo);

u_int8_t secIsRobustActionFrame(struct ADAPTER *prAdapter,
			   void *prPacket);

u_int8_t secIsRobustMgmtFrame(struct ADAPTER *prAdapter,
			void *prPacket);

u_int8_t secIsWepBss(struct ADAPTER *prAdapter,
			struct BSS_INFO *prBssInfo);

u_int8_t tkipMicDecapsulate(struct SW_RFB *prSwRfb,
			    uint8_t *pucMicKey);
u_int8_t tkipMicDecapsulateInRxHdrTransMode(
	struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb,
	uint8_t *pucMicKey);

void secPostUpdateAddr(struct ADAPTER *prAdapter,
		       struct BSS_INFO *prBssInfo);

enum ENUM_EAPOL_KEY_TYPE_T secGetEapolKeyType(
	uint8_t *pucPacket);

uint8_t secIsEapPacket(uint8_t *pucPkt);

void secHandleNoWtbl(struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb);

void secCheckRxEapolPacketEncryption(struct ADAPTER *prAdapter,
	struct SW_RFB *prRetSwRfb,
	struct STA_RECORD *prStaRec);
/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#endif /* _PRIVACY_H */
