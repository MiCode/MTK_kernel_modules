/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _FILS_H
#define _FILS_H


/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */


#include "config.h"
#include "precomp.h"


/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

#define PMK_LEN_MAX 64
#define FILS_NONCE_LEN 16
#define FILS_SESSION_LEN 8
#define FILS_ICK_MAX_LEN 48
#define FILS_FT_MAX_LEN 48
#define FILS_AUTH_MAX_LEN 287 /* nonce + session + wrapped */
#define FILS_ASSOC_MAX_LEN 62 /* session + key confirm */
#define FILS_MAX_KEY_AUTH_LEN 48
#define FILS_KCK_MAX_LEN 128
#define FILS_KEK_MAX_LEN 64
#define FILS_TK_MAX_LEN 32
#define FILS_KDK_MAX_LEN 32
#define FILS_GTK_MAX_LEN 32
#define FILS_IGTK_MAX_LEN 32
#define FILS_BIGTK_MAX_LEN 32

#define FILS_CACHE_ID_LEN 2
#define ERP_MAX_KEY_LEN 64

#define IGTK_KDE_PREFIX_LEN (2 + 6)
#define BIGTK_KDE_PREFIX_LEN (2 + 6)

/*
 * keyName-NAI has a maximum length of 253 octet to fit in
 * RADIUS attributes.
 */
#define FILS_MAX_KEY_NAME_NAI_LEN 253
#define FILS_MAX_REALM_LEN 255

#define FILS_AES_BLOCK_SIZE 16

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

struct EAP_ERP_KEY {
	uint8_t fgValid;
	uint16_t rRKLen;
	uint16_t rIKLen;
	uint8_t rRK[ERP_MAX_KEY_LEN];
	uint8_t rIK[ERP_MAX_KEY_LEN];
	uint32_t nextSeq; /* next seq */
	uint8_t keynameNai[FILS_MAX_KEY_NAME_NAI_LEN + 1]; /* username@realm */
	uint16_t keynameNaiLen;
};

struct FILS_INFO {
	struct EAP_ERP_KEY *prErpKey;
	struct PMKID_ENTRY *prCurPmksa;

	uint32_t u4ErpSeq; /* current erp seq */
	uint8_t aucFilsKeyAuthAP[FILS_MAX_KEY_AUTH_LEN];
	uint8_t aucFilsKeyAuthSTA[FILS_MAX_KEY_AUTH_LEN];
	uint16_t u2FilsKeyAuthLen;

	uint8_t kck[FILS_KCK_MAX_LEN]; /* EAPOL-Key Key Confirmation Key(KCK) */
	uint8_t kek[FILS_KEK_MAX_LEN]; /* EAPOL-Key Key Encryption Key (KEK) */
	uint8_t tk[FILS_TK_MAX_LEN]; /* Temporal Key (TK) */
	uint8_t kdk[FILS_KDK_MAX_LEN]; /* Key Derivation Key */
	uint8_t gtk_kde[FILS_GTK_MAX_LEN + 2];
	uint8_t igtk_kde[FILS_IGTK_MAX_LEN + IGTK_KDE_PREFIX_LEN];
	uint8_t bigtk_kde[FILS_IGTK_MAX_LEN + BIGTK_KDE_PREFIX_LEN];
	uint16_t kck_len;
	uint16_t kek_len;
	uint16_t tk_len;
	uint16_t kdk_len;
	uint16_t gtk_kde_len;
	uint16_t igtk_kde_len;
	uint16_t bigtk_kde_len;
	uint8_t rsc[8];

	uint8_t aucFilsNonce[FILS_NONCE_LEN];
	uint8_t aucFilsANonce[FILS_NONCE_LEN];
	uint8_t aucFilsSession[FILS_SESSION_LEN];
	uint8_t aucFilsErpPmkid[IW_PMKID_LEN];
	uint8_t aucFilsPmk[PMK_LEN_MAX];
	uint16_t u2FilsPmkLen;
	uint8_t fgFilsErpPmkidSet:1;
	uint8_t fgFilsSessionMatch:1;
	uint8_t fgFilsANonceSet:1;
	uint8_t fgFilsPmkSet:1;
};

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

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */


/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

uint32_t filsUpdateFilsInfo(struct ADAPTER *ad, struct EAP_ERP_KEY *erp,
			    struct PARAM_FILS *param);
void filsConnectionFailure(struct ADAPTER *ad, struct STA_RECORD *sta,
			   struct SW_RFB *rfb);


void filsBuildAuthIE(struct ADAPTER *ad, struct MSDU_INFO *msdu);
uint32_t filsProcessAuth(struct ADAPTER *ad, struct STA_RECORD *sta);
uint32_t filsRxAuthRSNE(struct ADAPTER *ad,
		     struct SW_RFB *rfb, struct IE_HDR *ie);
uint32_t filsRxAuthNonce(struct ADAPTER *ad,
		     struct SW_RFB *rfb, struct IE_HDR *ie);
uint32_t filsRxAuthSession(struct ADAPTER *ad,
		       struct SW_RFB *rfb, struct IE_HDR *ie);
uint32_t filsRxAuthWrapped(struct ADAPTER *ad,
		       struct SW_RFB *rfb, struct IE_HDR *ie);


void filsBuildAssocIE(struct ADAPTER *ad, struct MSDU_INFO *msdu);
uint32_t filsProcessAssocResp(struct ADAPTER *ad, struct SW_RFB *rfb);
uint32_t filsEncryptAssocReq(struct ADAPTER *ad, struct MSDU_INFO *msdu);
uint32_t filsDecryptAssocResp(struct ADAPTER *ad, struct SW_RFB *rfb,
	struct FILS_INFO *fils);

uint32_t filsInstallKey(struct ADAPTER *ad, struct STA_RECORD *sta);
uint32_t filsRemoveAllKeys(struct ADAPTER *ad, uint8_t bssidx);

#endif /* _FILS_H */
