/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _NAN_SEC_H_
#define _NAN_SEC_H_

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */
#if CFG_SUPPORT_NAN

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "nan/nan_data_engine.h"
#include "nan/nan_base.h"
#include "wpa_supp/src/utils/common.h"
#include "nan/wpa_supp/src/common/defs.h"

struct wpa_key_replay_counter;
extern int wpa_replay_counter_valid(struct wpa_key_replay_counter *ctr,
				    const u8 *replay_counter);
extern void wpa_replay_counter_mark_invalid(struct wpa_key_replay_counter *ctr,
					    const u8 *replay_counter);
extern void PKCS5_PBKDF2_HMAC(unsigned char *password, size_t plen,
			      unsigned char *salt, size_t slen,
			      const unsigned long iteration_count,
			      const unsigned long key_length,
			      unsigned char *output);

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define MAX_NDP_NUM 8 /* May integrate with NDP */
#define NAN_MAX_KEY_ID 3
#define NAN_SHA384_MAC_LEN 48
#define NAN_AUTH_TOKEN_LEN 16 /*128bit */
#define MAX_WTBL_ENTRY_NUM 128
#define CFG_NAN_SEC_UT 0
#define NCS_SK_128_MIC_LEN 16
#define NCS_SK_256_MIC_LEN 24
#define	NAN_MIC_LEN 24
#define KDE_BUF_SIZE 256
#define NAN_PACKET_NUMBER_LEN 6

#define NO_KEY_EXIST_IND 0
#define IGTK_EXIST_IND   BIT(0)
#define BIGTK_EXIST_IND  BIT(1)

#define NAN_PACKET_NUMBER_LEN 6
#define NAN_NUM_GCR_UR_RX_AFTER_DISCONNECTION 1
#define NAN_MAX_GTK_CIPHER_SUITE_NUM 2
#define NAN_MAX_MC_RX_WTBL_NUM \
	(NAN_MAX_SUPPORT_NDL_FIXED_NUM + \
	NAN_MAX_SUPPORT_NDP_NUM + \
	NAN_NUM_GCR_UR_RX_AFTER_DISCONNECTION)
#define NAN_MAX_MC_TX_WTBL_NUM \
	(NAN_MAX_MULTI_NDI_NUM)
#define NAN_BSS_IDX_FOR_IGTK_TX		NAN_BSS_INDEX_2G_BAND
#define NAN_BSS_IDX_FOR_BIGTK_TX	NAN_BSS_INDEX_5G_BAND

/*
 * [NAN_NUM_NMI_CXT_KEY]
 * The value of this definition is at least NAN_MAX_CONN_CFG (8).
 * And it might be larger than NAN_MAX_CONN_CFG depending on
 * the spec of pairing
 */
#define NAN_NUM_NMI_CXT_KEY 8
#define ENABLE_SEC_UT_LOG 1

#define NAN_BSS_IDX_FOR_IGTK_TX    NAN_BSS_INDEX_2G_BAND
#define NAN_BSS_IDX_FOR_BIGTK_TX   NAN_BSS_INDEX_5G_BAND

enum NAN_SEC_MIC_CAL_STATE {
	NAN_SEC_MIC_CAL_IDLE = 0,
	NAN_SEC_MIC_CAL_WAIT,
	NAN_SEC_MIC_CAL_DONE,
	NAN_SEC_MIC_CAL_ERROR,
	NAN_SEC_MIC_CAL_STATE_NUM
};

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */
struct _NAN_MC_RX_WTBL {
	atomic_t mcRxRefCount;
	uint16_t u2WtblIdx;
	uint8_t aucPeerAddr[MAC_ADDR_LEN];
#if (CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT == 1)
	uint8_t fgIgtkInstalled;
	uint8_t fgBigtkInstalled;
#endif
	struct STA_RECORD rStaRec;
};

#if (CFG_NAN_SUPPORT_R4_GROUP_ADDR_DATA == 1)
extern struct _NAN_MC_RX_WTBL g_arNanMcRxWtbl[NAN_MAX_MC_RX_WTBL_NUM];
#endif /* CFG_NAN_SUPPORT_R4_GROUP_ADDR_DATA */

struct _NAN_KEY_ENTRY_T  {
	uint8_t ucIdx;
	uint8_t fgValid;
	uint8_t fgIsKeyExist;
	uint8_t fgIsNmiTk;
	uint8_t aucNmiAddr[MAC_ADDR_LEN];
	uint16_t u2WtblIdx;

	atomic_t ulRefCount;
};

struct _NAN_NDP_SUDO {
	uint8_t u1Role;
	uint8_t u1WtblIdx;
	uint16_t u2PublishId;
	uint8_t au1RemoteAddr[MAC_ADDR_LEN];
} __packed;

struct _NAN_SEC_KDE_ATTR_HDR {
	uint8_t u1AttrId;
	uint16_t u2AttrLen;
	uint8_t u1PublishId;
} __packed;

struct _NAN_SEC_CSID_ATTR_HDR {
	uint8_t u1AttrId;
	uint16_t u2AttrLen;
	uint8_t u1Cap;
} __packed;

struct _NAN_SEC_CSID_ATTR_LIST {
	uint8_t u1CipherType; /* Follow WFA spec */
	uint8_t u1PublishId;
} __packed;

struct _NAN_SEC_SCID_ATTR_HDR {
	uint8_t u1AttrId;
	uint16_t u2AttrLen;
} __packed;

struct _NAN_SEC_SCID_ATTR_ENTRY {
	/* QUE_ENTRY_T rQueEntry; */
	uint16_t u2ScidLen;
	uint8_t u1ScidType;
	uint8_t u1PublishId;
} __packed;

struct _NAN_SEC_CIPHER_ENTRY {
	struct QUE_ENTRY rQueEntry;
	uint32_t u4CipherType;
	uint16_t u2PublishId;
} __packed;

#if (CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT == 1)
enum NAN_VENDOR_ENCRYPTION_TYPE {
	NAN_VENDOR_ENCRYPTION_TYPE_INVAILD = 0,
	NAN_VENDOR_ENCRYPTION_TYPE_AES_CCM,
	NAN_VENDOR_ENCRYPTION_TYPE_AES_CCM_256,
	NAN_VENDOR_ENCRYPTION_TYPE_AES_GCM,
	NAN_VENDOR_ENCRYPTION_TYPE_AES_GCM_256,
	NAN_VENDOR_ENCRYPTION_TYPE_AES_BIP_GMAC,
	NAN_VENDOR_ENCRYPTION_TYPE_AES_BIP_GMAC_256,
	NAN_VENDOR_ENCRYPTION_TYPE_AES_BIP_CMAC,
	NAN_VENDOR_ENCRYPTION_TYPE_AES_BIP_CMAC_256,
	NAN_VENDOR_ENCRYPTION_TYPE_AES_NUM
};
#endif /* CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT */

enum NAN_KEY_TYPE {
	NAN_KEY_TYPE_RSVD = 0,
	NAN_KEY_TYPE_NMI_CXT_MGMT_KEY,
	NAN_KEY_TYPE_MC_TX_KEY,
	NAN_KEY_TYPE_MC_RX_KEY,
	NAN_KEY_TYPE_MC_MGMT_TX_KEY,
	NAN_KEY_TYPE_MC_MGMT_RX_KEY,
	NAN_KEY_TYPE_NUM,
};

enum NAN_KEY_OPERATION {
	NAN_KEY_OP_ACTIVATE = 0,
	NAN_KEY_OP_INACTIVATE,
	NAN_KEY_OP_SET_KEY,
	NAN_KEY_OP_CLS_KEY,
	NAN_KEY_OP_NUM,
};

struct _NAN_CMD_KEY_MANAGEMENT_T  {
	uint8_t ucOp;		/* a.k.a enum NAN_KEY_OPERATION */
	uint8_t ucKeyType;	/* a.k.a enum NAN_KEY_TYPE  */
	/*
	 * 0: reserved
	 * 1: NMI Context key for RA/TA=NMI
	 * (UC Mgmt)
	 * 2: MC Tx key for <local NDI/NMI, NDC ID>
	 * (MC Data)
	 * 3: MC Rx key for <BC_MAC_ADDR, peer NDI/NMI>
	 * (MC Mgmt/Data)
	 */
	uint16_t u2WtblIdx;
	uint8_t aucLocalAddr[MAC_ADDR_LEN];
	uint8_t aucPeerAddr[MAC_ADDR_LEN];

	uint8_t ucNmiKeyIdx;		/* for key type = 1 */
	uint8_t ucNdcIdx;		/* for key type = 2 */
	uint8_t ucNdiIdx;		/* for key type = 2 */
	uint8_t ucMcRxIdx;		/* for key type = 3 */

	uint8_t fgInit;			/* for NAN_KEY_OP_ACTIVATE */
	uint8_t fgKeyExist;		/* for NAN_KEY_OP_ACTIVATE */
	uint8_t fgIsNmiTk;		/* for key type = 1 */
	uint8_t aucRsvd1[1];

	/* for key op = NAN_KEY_SET_KEY/NAN_KEY_CLS_KEY */
	uint8_t ucAlgorithmId;	/* WPA_ALG_XXX -> CIPHER_SUITE_XXX */
	uint8_t ucKeyId;
	uint8_t ucKeyLen;
	uint8_t ucRsvd;
	uint8_t aucKeyMaterial[32];
	uint8_t aucRsvd[16];		/* aucKeyRsc[16] */
} __KAL_ATTRIB_PACKED__ __KAL_ATTRIB_ALIGN_4__;

/* ======For other module building pass */
struct wpa_authenticator;
struct wpa_state_machine;
struct wpa_ptk;
struct wpa_eapol_key;
struct wpa_sm;

struct _NAN_NDP_INSTANCE_T;
/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */
#if (CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT == 1)
extern uint8_t g_aucNanGtkCipherSuiteList[NAN_MAX_GTK_CIPHER_SUITE_NUM];
extern uint8_t g_aucNanIgtkPn[NAN_PACKET_NUMBER_LEN];
extern uint8_t g_aucNanBigtkPn[NAN_PACKET_NUMBER_LEN];
#endif /* CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT */

extern struct _NAN_KEY_ENTRY_T g_arNanNmiCxtKey[NAN_NUM_NMI_CXT_KEY];
/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/************************************************
 *               MC WTBL Maintain Related
 ************************************************
 */

#if (CFG_NAN_SUPPORT_R4_GROUP_ADDR_DATA == 1)
struct _NAN_MC_RX_WTBL *
nanGetMcRxWtblIdx(
	uint8_t *pucPeerAddr
);
/*
 * NAN Use Case:
 * For UC: pucLocalAddr = local NDI/NMI, pucPeerAddr = peer NDI/NMI
 * For MC Rx: pucLocalAddr = BC_MAC_ADDR, pucPeerAddr = peer NDI/NMI
 * For MC Tx: pucLocalAddr = local NDI/NMI, pucPeerAddr = NDC ID
 */
uint8_t
nanGetIgtkBigtkInMcRxWtbl(
	IN struct ADAPTER *prAdapter,
	IN uint8_t *pucPeerNmiAddr);

uint32_t
nanSetIgtkBigtkInMcRxWtbl(
	IN struct ADAPTER *prAdapter,
	IN uint8_t *pucPeerNdiAddr,
	IN uint8_t ucKeyInstallStatus);

uint32_t
nanRegisterMcRxWtblIdx(
	IN struct ADAPTER *prAdapter,
	IN uint8_t *pucPeerAddr,
	IN uint16_t u2WtblIdx,
	IN uint8_t fgWtblReUsed);

uint32_t
nanReleaseMcRxWtblIdx(
	IN struct ADAPTER *prAdapter,
	IN uint8_t *pucPeerAddr);

uint32_t
nanRegisterMcTxWtblIdx(
	IN struct ADAPTER *prAdapter,
	IN uint8_t ucNdiIdx,
	IN struct _NAN_NDC_MGMT_T *prNdcMgmt,
	OUT uint8_t *pfgFirst);

uint32_t
nanReleaseMcTxWtblIdx(
	IN struct ADAPTER *prAdapter,
	IN uint8_t ucNdiIndex,
	IN struct _NAN_NDC_CTRL_T *prNdcCtrl);
#endif /* CFG_NAN_SUPPORT_R4_GROUP_ADDR_DATA */

/************************************************
 *               Export API Related
 ************************************************
 */

uint32_t
nanSecManageKeyCmd(IN struct ADAPTER *prAdapter,
	IN enum NAN_KEY_OPERATION eKeyOp,
	IN enum NAN_KEY_TYPE eKeyType, IN uint16_t u2WtblEntry,
	IN uint8_t *pucLocalAddr, IN uint8_t *pucPeerAddr,
	IN uint8_t ucAlgoId, IN uint8_t ucKeyId,
	IN uint8_t ucKeyLen, IN uint8_t *pucKeyData, IN uint8_t *pucPn,
	IN uint8_t fgInit, IN uint8_t fgKeyExist);

void
nanSecConfigNmiTk(IN struct ADAPTER *prAdapter,
	IN uint8_t fgSet, IN uint16_t u2WtblEntry,
	IN uint8_t *pucLocalNMI, IN uint8_t *pucPeerNMI,
	IN struct _NAN_NDP_INSTANCE_T *prNdp);

void
nanSecConfigNdiGtk(IN struct ADAPTER *prAdapter,
	IN uint8_t fgSet, IN uint16_t u2WtblEntry,
	IN uint8_t *pucLocalAddr, IN uint8_t *pucPeerAddr,
	IN struct _NAN_NDP_INSTANCE_T *prNdp);

struct _NAN_KEY_ENTRY_T *nanSecGetNmiCxtKey(IN struct ADAPTER *prAdapter,
	IN uint8_t *pucPeerNMI);

#if (CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT == 1)
uint32_t
nanSecCheckGtkKde(struct wpa_sm *sm,
	const u8 *key_data, size_t key_data_len);

uint8_t
nanSecGtkCipherWfaToNan(uint16_t ucCipherId);

void
nanSecPacketNumberUpdate(uint8_t *pucIgtkPn, uint8_t *pucBigtkPn);

void
nanSecSetIgtkToSm(
	struct ADAPTER *prAdapter, uint8_t *pucMacAddr,
	uint8_t ucCipherId, uint8_t ucKeyLen,
	uint8_t *pucKey, uint8_t fgSelfKey,
	uint16_t u2WtblIdx);

void
nanSecSetBigtkToSm(
	struct ADAPTER *prAdapter, uint8_t *pucMacAddr,
	uint8_t ucCipherId, uint8_t ucKeyLen,
	uint8_t *pucKey, uint8_t fgSelfKey,
	uint16_t u2WtblIdx);

void
nanSecSetGtkToSm(
	struct ADAPTER *prAdapter, uint8_t *pucMacAddr,
	uint8_t ucCipherId, uint8_t ucKeyLen,
	uint8_t *pucKey, uint8_t ucNdiIdx);
void
nanSecGetTxIgtkBigtk(uint32_t *pucIgtkCipher,
	uint8_t *pucIgtkKey, size_t *pucIgtkLen,
	uint32_t *pucBigtkCipher, uint8_t *pucBigtkKey,
	size_t *pucBigtkLen);

#if (CFG_SUPPORT_NAN_R4_VENDOR_PAIRING == 1)
uint16_t
nanSecAppendKde(uint8_t fgNik, uint8_t nik_length,
	uint8_t *nik,
	uint8_t cipher_version,
	uint8_t fgNikLifetime,
	uint16_t key_bitmap,
	uint32_t key_lifttime,
	uint8_t fgGroupKey, uint8_t *kek,
	size_t kek_len, uint8_t *payload);

void
nanSecComposeEapolKey(struct _NAN_ATTR_SKDA_T *prAttrSkda,
	uint8_t *prKdes, uint16_t u2KeyLength,
	uint8_t *kck, size_t kck_len,
	uint32_t u4CipherType);
void
nanSecRxSkdaToHostFormat(uint8_t *prKeyDescriptor, uint32_t u4CipherType);

#endif /* CFG_SUPPORT_NAN_R4_VENDOR_PAIRING */

void
nanSecConfigNmiIgtk(IN struct ADAPTER *prAdapter,
	IN uint8_t fgSet, IN uint16_t u2WtblEntry,
	IN uint8_t *pucLocalNMI, IN uint8_t *pucPeerNMI,
	IN struct _NAN_NDP_INSTANCE_T *prNdp);

void
nanSecConfigNmiBigtk(IN struct ADAPTER *prAdapter,
	IN uint8_t fgSet, IN uint16_t u2WtblEntry,
	IN uint8_t *pucLocalNMI, IN uint8_t *pucPeerNMI,
	IN struct _NAN_NDP_INSTANCE_T *prNdp);
#endif /* CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT */


uint32_t nanSecGetCsidAttr(OUT uint32_t *pu4CsidAttrLen,
			   OUT uint8_t **ppu1CsidAttrBuf);
uint32_t nanSecGetNdpScidAttr(IN struct _NAN_NDP_INSTANCE_T *prNdp,
			      OUT uint32_t *pu4ScidAttrLen,
			      OUT uint8_t **ppu1ScidAttrBuf);
uint32_t nanSecGetNdpCsidAttr(IN struct _NAN_NDP_INSTANCE_T *prNdp,
			      OUT uint32_t *pu4CsidAttrLen,
			      OUT uint8_t **ppu1CsidAttrBuf);
uint32_t nanSecSetCipherType(IN struct _NAN_NDP_INSTANCE_T *prNdp,
			     IN uint32_t u4CipherType);
#if (CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT == 1)
u_int8_t
nanSecIsGroupCipherSupported(uint8_t ucGtkCipherType);

uint8_t
nanSecIsDevSupportGroupSecurity(IN struct ADAPTER *prAdapter);

uint8_t
nanSecGetGroupSecurityCap(IN struct ADAPTER *prAdapter,
	struct _NAN_NDP_INSTANCE_T *prNDP);

uint8_t nanSecGetCipherWpaToHw(IN uint16_t u2GroupCipher);

uint32_t
nanSecGetGroupCipherType(
	IN uint8_t ucCap, uint8_t ucGtkCipherType,
	OUT u_int8_t *pfgGtk, OUT u_int8_t *pfgIgtk, OUT u_int8_t *pfgBigtk,
	OUT int32_t *pi4TmpGroupCipher, OUT int32_t *pi4TmpGroupMgmtCipher);

uint8_t
nanSecGetGtkCipherByNdi(uint8_t ucNdiIdx);

uint32_t
nanSecSetGroupCipherType(
	IN struct _NAN_NDP_INSTANCE_T *prNdp, u_int8_t fgIsTx);
uint32_t
nanSecSetGroupSA(
	IN struct _NAN_NDP_INSTANCE_T *prNdp);
#endif /* CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT */
uint32_t nanSecSetPmk(IN struct _NAN_NDP_INSTANCE_T *prNdp,
		IN uint32_t u4PmkLen, IN uint8_t *pu1Pmk);

uint32_t nanSecNotify4wayBegin(IN struct _NAN_NDP_INSTANCE_T *prNdp);
uint32_t nanSecNotify4wayTerminate(IN struct _NAN_NDP_INSTANCE_T *prNdp);
uint32_t nanSecTxKdeAttrDone(IN struct _NAN_NDP_INSTANCE_T *prNdp,
			     IN uint8_t u1DstMsg);
uint32_t nanSecRxKdeAttr(IN struct _NAN_NDP_INSTANCE_T *prNdp,
			 IN uint8_t u1SrcMsg, IN uint32_t u4KdeAttrLen,
			 IN uint8_t *pu1KdeAttrBuf, IN uint32_t u4RxMsgLen,
			 IN uint8_t *pu1RxMsgBuf);

uint32_t nanSecNotifyMsgBodyRdy(IN struct _NAN_NDP_INSTANCE_T *prNdp,
				IN uint8_t u1SrcMsg, IN OUT uint32_t u4TxMsgLen,
				IN OUT uint8_t *pu1TxMsgBuf);

void nan_sec_wpa_supplicant_start(struct GLUE_INFO *prGlueInfo);

int32_t
nan_sec_ndi_wpa_auth_init(size_t szNdiIdx, uint8_t ucGtkCipherType);

void nan_sec_hostapd_deinit(void);
uint32_t nanSecInsertCipherList(IN uint32_t u4CipherType,
				IN uint16_t u2PublishId);
uint32_t nanSecFlushCipherList(void);
uint16_t nanSecCalKdeAttrLenFunc(struct _NAN_NDP_INSTANCE_T *prNdp);
void nanSecAppendKdeAttrFunc(struct _NAN_NDP_INSTANCE_T *prNdp,
			     struct MSDU_INFO *prMsduInfo);

struct wpa_state_machine *nanSecGetInitiatorSm(uint8_t u1Index);
struct wpa_sm *nanSecGetResponderSm(uint8_t u1Index);
void nanInitSecResponderSm(struct wpa_sm *prSm);
void nanInitSecInitiatorSm(struct wpa_state_machine *prStaMac);

uint32_t nan_sec_wpa_sm_rx_eapol(struct wpa_state_machine *wpaStateMachine,
	struct wpa_sm *sm, const u8 *src_addr);

void nanSecResetTk(struct STA_RECORD *prStaRec);
void nanSecInstallTk(struct _NAN_NDP_INSTANCE_T *prNdp,
		     struct STA_RECORD *prStaRec);
void nanSecUpdatePeerNDI(struct _NAN_NDP_INSTANCE_T *prNdp,
			 uint8_t *au1PeerNdiAddr);
int32_t
nanSecCompareSA(IN struct ADAPTER *prAdapter,
		IN struct _NAN_NDP_INSTANCE_T *prNdp1,
		IN struct _NAN_NDP_INSTANCE_T *prNdp2);

/************************************************
 *               NDP Sudo Related
 ************************************************
 */
uint32_t nanNdpNotifySecAttrRdy(IN uint8_t u1NdpIdx);

uint32_t nanNdpGetNdiAddr(uint8_t u1NdpIdx,
	uint8_t u1Role, uint8_t *pu1MacAddr);
uint32_t nanNdpGetPublishId(uint8_t *u1NdpIdx);
uint32_t nanNdpNotifySecStatus(uint8_t u1NdpIdx, uint8_t u1Status,
			       uint8_t u1Reason, uint8_t u1Msg);

uint8_t nanNdpGetWlanIdx(uint8_t u1NdpIdx);

/************************************************
 *               Self-Use API Related
 ************************************************
 */
uint8_t nanSecSelPtkKeyId(struct _NAN_NDP_INSTANCE_T *prNdp,
			 uint8_t *pu1PeerAddr);
uint32_t nanSecUpdatePmk(struct _NAN_NDP_INSTANCE_T *prNdp);

void nanSecUpdateAttrCmd(IN struct ADAPTER *prAdapter, uint8_t aucAttrId,
			 uint8_t *aucAttrBuf, uint16_t aucAttrLen);

/************************************************
 *               Tx Related
 ************************************************
 */
int nan_sec_wpa_eapol_key_mic(const u8 *key, size_t key_len, u32 cipher,
			      const u8 *buf, size_t len, u8 *mic);
int nan_sec_wpa_supplicant_send_2_of_4(struct wpa_sm *sm,
				       const unsigned char *dst,
				       const struct wpa_eapol_key *key, int ver,
				       const u8 *nonce, const u8 *wpa_ie,
				       size_t wpa_ie_len, struct wpa_ptk *ptk);

int nan_sec_wpa_supplicant_send_4_of_4(struct wpa_sm *sm,
				       const unsigned char *dst,
				       const struct wpa_eapol_key *key, u16 ver,
				       u16 key_info, struct wpa_ptk *ptk);

int nan_sec_wpa_send_eapol(
	struct wpa_authenticator *wpa_auth, /*AP: KDE compose, MIC, and send*/
	struct wpa_state_machine *sm, int key_info, const u8 *key_rsc,
	const u8 *nonce, const u8 *kde, size_t kde_len, int keyidx, int encr,
	int force_version);

/************************************************
 *               MIC Related
 ************************************************
 */
uint32_t nanSecMicCalStaSmStep(struct wpa_sm *sm);
uint32_t nanSecMicCalApSmStep(struct wpa_state_machine *sm);

uint32_t nanSecStaSmBufReset(struct wpa_sm *sm);
uint32_t nanSecApSmBufReset(struct wpa_state_machine *sm);

uint32_t nanSecGenAuthToken(u32 cipher, const u8 *auth_token_data,
			    size_t auth_token_data_len, u8 *auth_token);
uint32_t nanSecGenM3MicMaterial(IN uint8_t *pu1AuthTokenBuf,
				IN const u8 *pu1M3bodyBuf,
				IN uint32_t u4M3BodyLen,
				OUT uint8_t **ppu1M3MicMaterialBuf,
				OUT uint32_t *pu4M3MicMaterialLen);

u_int8_t
nanSecIsPMFApply(struct ADAPTER *prAdapter,
	uint8_t *pucLocalAddr, uint8_t *pucPeerAddr, uint8_t *pucWtblEntry);
struct _NAN_KEY_ENTRY_T *nanSecGetNmiCxtKey(IN struct ADAPTER *prAdapter,
	IN uint8_t *pucPeerNMI);
void nanSecReleaseNmiCxtKey(IN struct ADAPTER *prAdapter,
	IN struct _NAN_KEY_ENTRY_T *prNmiKey);

void nanSecReleaseAllNmiCxtKey(IN struct ADAPTER *prAdapter);

struct _NAN_KEY_ENTRY_T *nanSecAcquireNmiCxtKey(IN struct ADAPTER *prAdapter,
	uint8_t *pucPeerNmiAddr);

void nanSecNmiCxtKeyInit(IN struct ADAPTER *prAdapter);
/************************************************
 *               UT Related
 ************************************************
 */
uint32_t nanSecUtMain(void);
void nanSecDumpEapolKey(struct wpa_eapol_key *key);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
struct _NAN_NDC_CTRL_T *
nanSchedGetNdcCtrl(struct ADAPTER *prAdapter, uint8_t *pucNdcId);

int32_t
nan_sec_wpas_setkey_glue(bool fgIsAp, size_t szBssIdx, enum wpa_alg alg,
	const u8 *addr, int key_idx, const u8 *key, size_t key_len);

#if (CFG_SUPPORT_NAN_R4_VENDOR_PAIRING == 1)
int32_t
nanSecSetKeyHandler(struct ADAPTER *prAdapter,
	struct NanPairingSetKey *prNanPairingKey);
#endif /* CFG_SUPPORT_NAN_R4_VENDOR_PAIRING */
#endif
#endif /* _NAN_SEC_H_ */
