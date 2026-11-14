// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

#include "precomp.h"

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

#if (CFG_SUPPORT_FILS_SK_OFFLOAD == 1)

#include "wpa_supp/FourWayHandShake.h"

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

enum {
	EAP_CODE_REQUEST = 1,
	EAP_CODE_RESPONSE = 2,
	EAP_CODE_SUCCESS = 3,
	EAP_CODE_FAILURE = 4,
	EAP_CODE_INITIATE = 5,
	EAP_CODE_FINISH = 6
};

/* Type field in EAP-Initiate and EAP-Finish messages */
enum eap_erp_type {
	EAP_ERP_TYPE_REAUTH_START = 1,
	EAP_ERP_TYPE_REAUTH = 2,
};

/* ERP TV/TLV types */
enum eap_erp_tlv_type {
	EAP_ERP_TLV_KEYNAME_NAI = 1,
	EAP_ERP_TV_RRK_LIFETIME = 2,
	EAP_ERP_TV_RMSK_LIFETIME = 3,
	EAP_ERP_TLV_DOMAIN_NAME = 4,
	EAP_ERP_TLV_CRYPTOSUITES = 5,
	EAP_ERP_TLV_AUTHORIZATION_INDICATION = 6,
	EAP_ERP_TLV_CALLED_STATION_ID = 128,
	EAP_ERP_TLV_CALLING_STATION_ID = 129,
	EAP_ERP_TLV_NAS_IDENTIFIER = 130,
	EAP_ERP_TLV_NAS_IP_ADDRESS = 131,
	EAP_ERP_TLV_NAS_IPV6_ADDRESS = 132,
};

/* ERP Cryptosuite */
enum eap_erp_cryptosuite {
	EAP_ERP_CS_HMAC_SHA256_64 = 1,
	EAP_ERP_CS_HMAC_SHA256_128 = 2,
	EAP_ERP_CS_HMAC_SHA256_256 = 3,
};

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

__KAL_ATTRIB_PACKED_FRONT__
struct EAP_HDR {
	uint8_t code;
	uint8_t identifier;
	uint16_t length; /* BE */
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct ERP_TLVS {
	const uint8_t *keyname;
	const uint8_t *domain;
	uint8_t keynameLen;
	uint8_t domainLen;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct IGTK_KDE {
	uint8_t keyid[2];
	uint8_t pn[6];
	uint8_t igtk[FILS_IGTK_MAX_LEN];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct BIGTK_KDE {
	uint8_t keyid[2];
	uint8_t pn[6];
	uint8_t bigtk[FILS_BIGTK_MAX_LEN];
} __KAL_ATTRIB_PACKED__;

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
	struct PARAM_FILS *param)
{
	uint8_t ctx[3];
	uint8_t *pos;

	/* Next seq */
	erp->nextSeq = param->u4ErpNextSeqNum;

	/* rRK */
	kalMemCopy(erp->rRK, param->pucErpRrk, param->u2ErpRrkLen);
	erp->rRKLen = param->u2ErpRrkLen;

	/* NAI */
	pos = erp->keynameNai;
	kalMemCopy(pos, param->pucErpUsername, param->u2ErpUsernameLen);
	pos += param->u2ErpUsernameLen;
	*pos++ = '@';
	kalMemCopy(pos, param->pucErpRealm, param->pucErpRealmLen);
	pos += param->pucErpRealmLen;
	erp->keynameNaiLen = pos - erp->keynameNai;
	*pos++ = '\0'; /* add null termination for log */

	DBGLOG(FILS, INFO, "EAP: Realm for ERP keyName-NAI(len=%d): %s",
		erp->keynameNaiLen, erp->keynameNai);

	ctx[0] = EAP_ERP_CS_HMAC_SHA256_128;
	WLAN_SET_FIELD_BE16(&ctx[1], erp->rRKLen);
	if (hmac_sha256_kdf(erp->rRK, erp->rRKLen,
		"Re-authentication Integrity Key@ietf.org",
		ctx, sizeof(ctx), erp->rIK, erp->rRKLen) < 0) {
		DBGLOG(FILS, ERROR, "EAP: Could not derive rIK for ERP");
		return WLAN_STATUS_FAILURE;
	}
	erp->rIKLen = erp->rRKLen;
	DBGDUMP_MEM8(FILS, TRACE, "EAP: ERP rIK\n", erp->rIK, erp->rIKLen);

	erp->fgValid = TRUE;

	return WLAN_STATUS_SUCCESS;
}

uint32_t filsBuildErpReauthStart(struct ADAPTER *ad, struct FILS_INFO *fils,
	struct EAP_ERP_KEY *erp, uint8_t *buf, uint32_t len)
{
	struct EAP_HDR *hdr;
	u8 hash[SHA256_MAC_LEN];
	uint8_t *cp;
	uint32_t min_len = sizeof(struct EAP_HDR) +
			  2 /* type + flags */ +
			  2 /* seq */ +
			  2 + erp->keynameNaiLen /* type, length, data */ +
			  1 /* cryptosuite */ +
			  16 /* hash */;

	if (len < min_len) {
		DBGLOG(FILS, ERROR, "buf too small for erp\n");
		return 0;
	}

	/* EAP-Initiate/Re-auth Packet
	 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 * | Code          | Identifier    | Length                        |
	 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 * | Type          |R|B|L| Reserved| SEQ                           |
	 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 * | 1 or more TVs or TLVs                                         ~
	 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 * | Cryptosuite   | Authentication Tag                            ~
	 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 */
	cp = buf;
	hdr = (struct EAP_HDR *) buf;
	hdr->code = EAP_CODE_INITIATE;
	hdr->identifier = 0;
	WLAN_SET_FIELD_BE16(&hdr->length, min_len);
	cp += sizeof(struct EAP_HDR);
	/* type */
	*cp = EAP_ERP_TYPE_REAUTH;
	cp++;
	/* Flags: R=0 B=0 L=1 */
	*cp = 0x20;
	cp++;
	/* seq */
	WLAN_SET_FIELD_BE16(cp, erp->nextSeq);
	cp += 2;
	/* tlv */
	*cp = EAP_ERP_TLV_KEYNAME_NAI;
	cp++;
	*cp = erp->keynameNaiLen;
	cp++;
	kalMemCopy(cp, erp->keynameNai, erp->keynameNaiLen);
	cp += erp->keynameNaiLen;
	/* cryptosuite */
	*cp = EAP_ERP_CS_HMAC_SHA256_128;
	cp++;

	if (hmac_sha256(erp->rIK, erp->rIKLen, buf, cp - buf, hash) < 0) {
		DBGLOG(FILS, ERROR, "sha256 failed\n");
		return 0;
	}
	kalMemCopy(cp, hash, 16);
	cp += 16;

	fils->u4ErpSeq = erp->nextSeq;
	erp->nextSeq++;

	DBGDUMP_MEM8(FILS, TRACE, "ERP: EAP-Initiate/Re-auth\n", buf, cp - buf);

	return cp - buf;
}

int filsPmkidErp(int akmp, const uint8_t *reauth,
	size_t reauth_len, uint8_t *pmkid)
{
	const uint8_t *addr[1];
	size_t len[1];
	uint8_t hash[SHA384_MAC_LEN];
	int res;

	/* PMKID = Truncate-128(Hash(EAP-Initiate/Reauth)) */
	addr[0] = reauth;
	len[0] = reauth_len;
	if (rsnIsKeyMgmtSha384(akmp))
		res = sha384_vector(1, addr, len, hash);
	else if (rsnIsKeyMgmtSha256(akmp))
		res = sha256_vector(1, addr, len, hash);
	else
		return -1;
	if (res)
		return res;

	kalMemCopy(pmkid, hash, IW_PMKID_LEN);
	DBGDUMP_MEM8(FILS, INFO, "PMKID\n", pmkid, IW_PMKID_LEN);

	return 0;
}

void filsBuildAuthIE(struct ADAPTER *ad, struct MSDU_INFO *msdu)
{
	struct STA_RECORD *sta;
	struct BSS_INFO *bss;
	uint8_t bssidx;
	struct EAP_ERP_KEY *erpKey = NULL;
	struct PMKID_ENTRY *entry = NULL;
	struct FILS_INFO *fils = NULL;
	uint8_t *cp, *buf;
	struct IE_FILS_NONCE *nonce;
	struct IE_FILS_SESSION *session;
	struct IE_FILS_WRAPPED_DATA *wrapped;

	bssidx = msdu->ucBssIndex;
	bss = GET_BSS_INFO_BY_INDEX(ad, bssidx);
	sta = cnmGetStaRecByIndex(ad, msdu->ucStaRecIndex);

	if (!IS_BSS_AIS(bss) || !sta || !rsnIsFilsAuthAlg(sta->ucAuthAlgNum))
		return;

	erpKey = aisGetErpKey(ad, bssidx);
	entry = aisSearchPmkidEntry(ad, sta, bssidx);
	if (!erpKey && !entry) {
		DBGLOG(FILS, ERROR,
			"Neither ERP EAP-Initiate/Re-auth nor PMKSA cache entry is available - skip FILS\n");
		return;
	}

	DBGLOG(FILS, INFO, "Try to use FILS (erp=%d pmksa_cache=%d)\n",
		erpKey != NULL, entry != NULL);

	if (sta->prFilsInfo) {
		kalMemZFree(sta->prFilsInfo, VIR_MEM_TYPE,
			sizeof(struct FILS_INFO));
		sta->prFilsInfo = NULL;
	}

	sta->prFilsInfo = kalMemZAlloc(sizeof(struct FILS_INFO), VIR_MEM_TYPE);
	if (!sta->prFilsInfo) {
		DBGLOG(FILS, ERROR, "no memory\n");
		return;
	}

	fils = sta->prFilsInfo;
	fils->prErpKey = erpKey;
	fils->prCurPmksa = entry;

	/* generate nonce and session first */
	kalRandomGetBytes(fils->aucFilsNonce, FILS_NONCE_LEN);
	kalRandomGetBytes(fils->aucFilsSession, FILS_SESSION_LEN);

	buf = (uint8_t *)msdu->prPacket + msdu->u2FrameLength;
	cp = buf;

	nonce = (struct IE_FILS_NONCE *) cp;
	nonce->ucId = ELEM_ID_EXTENSION;
	nonce->ucLength = 1 + FILS_NONCE_LEN;
	nonce->ucExtId = ELEM_EXT_ID_FILS_NONCE;
	kalMemCopy(nonce->aucNonce, fils->aucFilsNonce, FILS_NONCE_LEN);
	cp += IE_SIZE(nonce);

	DBGDUMP_MEM8(FILS, INFO, "NONCE\n", nonce, IE_SIZE(nonce));

	session = (struct IE_FILS_SESSION *) cp;
	session->ucId = ELEM_ID_EXTENSION;
	session->ucLength = 1 + FILS_SESSION_LEN;
	session->ucExtId = ELEM_EXT_ID_FILS_SESSION;
	kalMemCopy(session->aucSession, fils->aucFilsSession, FILS_SESSION_LEN);
	cp += IE_SIZE(session);

	DBGDUMP_MEM8(FILS, INFO, "SESSION\n", session, IE_SIZE(session));

	if (erpKey) {
		uint8_t erp_msg[256];
		uint8_t msg_len = 0;

		msg_len = filsBuildErpReauthStart(
			ad, fils, erpKey, erp_msg, sizeof(erp_msg));
		wrapped = (struct IE_FILS_WRAPPED_DATA *) cp;
		wrapped->ucId = ELEM_ID_EXTENSION;
		wrapped->ucLength = 1 + msg_len;
		wrapped->ucExtId = ELEM_EXT_ID_FILS_WRAPPED_DATA;
		kalMemCopy(wrapped->aucData, erp_msg, msg_len);
		cp += IE_SIZE(wrapped);

		DBGDUMP_MEM8(FILS, TRACE, "WRAPPED\n",
			wrapped, IE_SIZE(wrapped));

		/* Calculate pending PMKID here so that we do not need to
		 * maintain a copy of the EAP-Initiate/Reauth message.
		 */
		if (filsPmkidErp(bss->u4RsnSelectedAKMSuite, erp_msg,
				   msg_len, fils->aucFilsErpPmkid) == 0)
			fils->fgFilsErpPmkidSet = TRUE;
	}

	msdu->u2FrameLength += cp - buf;
}

int filsPmkToPtk(uint8_t *pmk, uint16_t pmk_len, uint8_t *spa,
	uint8_t *aa, uint8_t *snonce, uint8_t *anonce, uint32_t akmp,
	uint32_t cipher, uint8_t *ick, uint16_t *ick_len,
	struct FILS_INFO *fils)
{
	uint8_t *data, *pos;
	uint16_t data_len;
	uint8_t tmp[FILS_ICK_MAX_LEN + FILS_KEK_MAX_LEN + FILS_TK_MAX_LEN];
	uint16_t key_data_len;
	const char *label = "FILS PTK Derivation";
	int ret = -1;
	uint16_t offset;

	/*
	 * FILS-Key-Data = PRF-X(PMK, "FILS PTK Derivation",
	 *			 SPA || AA || SNonce || ANonce [ || DHss ])
	 * ICK = L(FILS-Key-Data, 0, ICK_bits)
	 * KEK = L(FILS-Key-Data, ICK_bits, KEK_bits)
	 * TK = L(FILS-Key-Data, ICK_bits + KEK_bits, TK_bits)
	 * If doing FT initial mobility domain association:
	 * FILS-FT = L(FILS-Key-Data, ICK_bits + KEK_bits + TK_bits,
	 *	       FILS-FT_bits)
	 * When a KDK is derived:
	 * KDK = L(FILS-Key-Data, ICK_bits + KEK_bits + TK_bits + FILS-FT_bits,
	 *	   KDK_bits)
	 */
	data_len = 2 * ETH_ALEN + 2 * FILS_NONCE_LEN;
	data = kalMemAlloc(data_len, VIR_MEM_TYPE);
	if (!data)
		goto err;
	pos = data;
	os_memcpy(pos, spa, ETH_ALEN);
	pos += ETH_ALEN;
	os_memcpy(pos, aa, ETH_ALEN);
	pos += ETH_ALEN;
	os_memcpy(pos, snonce, FILS_NONCE_LEN);
	pos += FILS_NONCE_LEN;
	os_memcpy(pos, anonce, FILS_NONCE_LEN);
	pos += FILS_NONCE_LEN;


	fils->kck_len = 0;
	fils->kek_len = rsnKekLen(akmp, pmk_len);
	fils->tk_len = rsnCipherKeyLen(cipher);
	if (rsnIsKeyMgmtSha384(akmp))
		*ick_len = 48;
	else if (rsnIsKeyMgmtSha256(akmp))
		*ick_len = 32;
	else
		goto err;

	key_data_len = *ick_len + fils->kek_len + fils->tk_len;
	fils->kdk_len = 0;

	if (rsnIsKeyMgmtSha384(akmp)) {
		DBGLOG(FILS, INFO, "PTK derivation using PRF(SHA384)");
		if (sha384_prf(pmk, pmk_len, label, data, data_len,
			       tmp, key_data_len) < 0)
			goto err;
	} else {
		DBGLOG(FILS, INFO, "PTK derivation using PRF(SHA256)");
		if (sha256_prf(pmk, pmk_len, label, data, data_len,
			       tmp, key_data_len) < 0)
			goto err;
	}

	DBGLOG(FILS, INFO, "PTK derivation - SPA=" MACSTR
		   " AA=" MACSTR "\n", MAC2STR(spa), MAC2STR(aa));
	DBGDUMP_MEM8(FILS, TRACE, "SNonce\n", snonce, FILS_NONCE_LEN);
	DBGDUMP_MEM8(FILS, TRACE, "ANonce\n", anonce, FILS_NONCE_LEN);
	DBGDUMP_MEM8(FILS, TRACE, "PMK\n", pmk, pmk_len);
	DBGDUMP_MEM8(FILS, TRACE, "FILS-Key-Data\n", tmp, key_data_len);

	kalMemCopy(ick, tmp, *ick_len);
	offset = *ick_len;
	DBGDUMP_MEM8(FILS, TRACE, "ICK\n", ick, *ick_len);

	kalMemCopy(fils->kek, tmp + offset, fils->kek_len);
	DBGDUMP_MEM8(FILS, TRACE, "KEK\n", fils->kek, fils->kek_len);
	offset += fils->kek_len;

	os_memcpy(fils->tk, tmp + offset, fils->tk_len);
	DBGDUMP_MEM8(FILS, TRACE, "TK\n", fils->tk, fils->tk_len);
	offset += fils->tk_len;

	kalMemZero(tmp, sizeof(tmp));
	ret = 0;
err:
	if (data)
		kalMemZFree(data, VIR_MEM_TYPE, data_len);
	return ret;
}

int filsKeyAuthSk(uint8_t *ick, uint16_t ick_len, uint8_t *snonce,
	uint8_t *anonce, uint8_t *staAddr, uint8_t *bssid,
	uint32_t akmp, uint8_t *key_auth_sta, uint8_t *key_auth_ap,
	uint16_t *key_auth_len)
{
	const uint8_t *addr[6];
	size_t len[6];
	size_t num_elem = 4;
	int res;

	DBGLOG(FILS, INFO, "FILS: Key-Auth derivation: STA-MAC=" MACSTR
		   " AP-BSSID=" MACSTR, MAC2STR(staAddr), MAC2STR(bssid));
	DBGDUMP_MEM8(FILS, TRACE, "ICK\n", ick, ick_len);
	DBGDUMP_MEM8(FILS, TRACE, "SNonce\n", snonce, FILS_NONCE_LEN);
	DBGDUMP_MEM8(FILS, TRACE, "ANonce\n", anonce, FILS_NONCE_LEN);

	/*
	 * For (Re)Association Request frame (STA->AP):
	 * Key-Auth = HMAC-Hash(ICK, SNonce || ANonce || STA-MAC || AP-BSSID
	 *                      [ || gSTA || gAP ])
	 */
	addr[0] = snonce;
	len[0] = FILS_NONCE_LEN;
	addr[1] = anonce;
	len[1] = FILS_NONCE_LEN;
	addr[2] = staAddr;
	len[2] = MAC_ADDR_LEN;
	addr[3] = bssid;
	len[3] = MAC_ADDR_LEN;

	if (rsnIsKeyMgmtSha384(akmp)) {
		*key_auth_len = 48;
		res = hmac_sha384_vector(ick, ick_len, num_elem, addr, len,
					 key_auth_sta);
	} else if (rsnIsKeyMgmtSha256(akmp)) {
		*key_auth_len = 32;
		res = hmac_sha256_vector(ick, ick_len, num_elem, addr, len,
					 key_auth_sta);
	} else {
		return -1;
	}
	if (res < 0)
		return res;
	/*
	 * For (Re)Association Response frame (AP->STA):
	 * Key-Auth = HMAC-Hash(ICK, ANonce || SNonce || AP-BSSID || STA-MAC
	 *                      [ || gAP || gSTA ])
	 */
	addr[0] = anonce;
	addr[1] = snonce;
	addr[2] = bssid;
	addr[3] = staAddr;

	if (rsnIsKeyMgmtSha384(akmp))
		res = hmac_sha384_vector(ick, ick_len, num_elem, addr, len,
					 key_auth_ap);
	else if (rsnIsKeyMgmtSha256(akmp))
		res = hmac_sha256_vector(ick, ick_len, num_elem, addr, len,
					 key_auth_ap);
	if (res < 0)
		return res;
	DBGDUMP_MEM8(FILS, INFO, "Key-Auth (STA)\n",
		key_auth_sta, *key_auth_len);
	DBGDUMP_MEM8(FILS, INFO, "Key-Auth (AP)\n",
		key_auth_ap, *key_auth_len);

	return 0;
}

uint32_t filsProcessAuth(struct ADAPTER *ad, struct STA_RECORD *sta)
{
	struct BSS_INFO *bss;
	struct FILS_INFO *fils = NULL;
	uint8_t ick[FILS_ICK_MAX_LEN];
	uint16_t ick_len;

	if (!sta || !sta->prFilsInfo)
		return WLAN_STATUS_NOT_ACCEPTED;

	fils = sta->prFilsInfo;
	bss = GET_BSS_INFO_BY_INDEX(ad, sta->ucBssIndex);
	if (!bss)
		return WLAN_STATUS_FAILURE;

	if (!fils->fgFilsANonceSet) {
		DBGLOG(FILS, ERROR, "ANonce not set\n");
		return WLAN_STATUS_FAILURE;
	}

	if (!fils->fgFilsSessionMatch) {
		DBGLOG(FILS, ERROR, "Sesson not matched\n");
		return WLAN_STATUS_FAILURE;
	}

	if (!fils->fgFilsPmkSet) {
		DBGLOG(FILS, ERROR, "PMK not set\n");
		return WLAN_STATUS_FAILURE;
	}

	if (filsPmkToPtk(fils->aucFilsPmk, fils->u2FilsPmkLen,
		bss->aucOwnMacAddr, cnmStaRecAuthAddr(ad, sta),
		fils->aucFilsNonce, fils->aucFilsANonce,
		bss->u4RsnSelectedAKMSuite,
		bss->u4RsnSelectedPairwiseCipher, ick, &ick_len, fils) < 0) {
		DBGLOG(FILS, ERROR, "FILS: Failed to derive PTK");
		return WLAN_STATUS_FAILURE;
	}

	if (filsKeyAuthSk(ick, ick_len, fils->aucFilsNonce,
			fils->aucFilsANonce, bss->aucOwnMacAddr,
			cnmStaRecAuthAddr(ad, sta),
			bss->u4RsnSelectedAKMSuite, fils->aucFilsKeyAuthSTA,
			fils->aucFilsKeyAuthAP, &fils->u2FilsKeyAuthLen) < 0) {
		DBGLOG(FILS, ERROR, "FILS: Failed to derive SK");
		forced_memzero(ick, sizeof(ick));
		return WLAN_STATUS_FAILURE;
	}

	forced_memzero(ick, sizeof(ick));

	return WLAN_STATUS_SUCCESS;
}

void filsConnectionFailure(struct ADAPTER *ad,
			struct STA_RECORD *sta, struct SW_RFB *rfb)
{
}

uint32_t filsRxAuthRSNE(struct ADAPTER *ad,
		     struct SW_RFB *rfb, struct IE_HDR *ie)
{
	struct STA_RECORD *sta;
	struct FILS_INFO *fils = NULL;
	struct RSN_INFO rRsnInfo = {0};
	struct PMKID_ENTRY *entry = NULL;
	uint8_t bssidx;

	sta = cnmGetStaRecByIndex(ad, rfb->ucStaRecIdx);
	if (!sta) {
		DBGLOG(FILS, ERROR, "No starec=%p\n");
		return WLAN_STATUS_FAILURE;
	}

	if (!rsnIsFilsAuthAlg(sta->ucAuthAlgNum))
		return WLAN_STATUS_SUCCESS;

	fils = sta->prFilsInfo;
	if (!fils) {
		DBGLOG(FILS, ERROR, "No fils info\n");
		return WLAN_STATUS_FAILURE;
	}

	if (!rsnParseRsnIE(ad, (uint8_t *)ie, &rRsnInfo)) {
		DBGLOG(FILS, ERROR, "Parse RSNE failed\n");
		return WLAN_STATUS_FAILURE;
	}

	bssidx = sta->ucBssIndex;
	if (!IS_BSS_INDEX_AIS(ad, bssidx)) {
		DBGLOG(FILS, ERROR, "bss%d but only support ais\n", bssidx);
		return WLAN_STATUS_NOT_ACCEPTED;
	}

	if (rRsnInfo.u2PmkidCount == 0)
		return WLAN_STATUS_SUCCESS;

	if (rRsnInfo.u2PmkidCount != 1) {
		DBGLOG(FILS, ERROR, "Wrong PMKID count %d\n",
			rRsnInfo.u2PmkidCount);
		return WLAN_STATUS_FAILURE;
	}

	entry = fils->prCurPmksa;
	if (!entry) {
		DBGLOG(FILS, ERROR, "No entry\n");
		return WLAN_STATUS_FAILURE;
	}

	DBGDUMP_MEM8(FILS, INFO, "PMKID\n", rRsnInfo.aucPmkid, IW_PMKID_LEN);

	if (kalMemCmp(entry->rBssidInfo.arPMKID,
		rRsnInfo.aucPmkid, IW_PMKID_LEN) != 0) {
		DBGLOG(FILS, ERROR, "PMKID mismatch\n");
		DBGDUMP_MEM8(FILS, ERROR, "Expected PMKID\n",
			entry->rBssidInfo.arPMKID, IW_PMKID_LEN);
		return WLAN_STATUS_FAILURE;
	}

	kalMemCopy(fils->aucFilsPmk, entry->rBssidInfo.arPMK,
		entry->rBssidInfo.u2PMKLen);
	fils->u2FilsPmkLen = entry->rBssidInfo.u2PMKLen;
	fils->fgFilsPmkSet = TRUE;

	DBGDUMP_MEM8(FILS, TRACE, "PMK\n",
		fils->aucFilsPmk, fils->u2FilsPmkLen);

	return WLAN_STATUS_SUCCESS;
}

uint32_t filsRxAuthNonce(struct ADAPTER *ad,
		     struct SW_RFB *rfb, struct IE_HDR *ie)
{
	struct STA_RECORD *sta;
	struct FILS_INFO *fils = NULL;
	struct IE_FILS_NONCE *nonce;

	sta = cnmGetStaRecByIndex(ad, rfb->ucStaRecIdx);
	if (!sta) {
		DBGLOG(FILS, ERROR, "No starec\n");
		return WLAN_STATUS_FAILURE;
	}

	if (!rsnIsFilsAuthAlg(sta->ucAuthAlgNum))
		return WLAN_STATUS_SUCCESS;

	fils = sta->prFilsInfo;
	if (!fils) {
		DBGLOG(FILS, ERROR, "No fils info\n");
		return WLAN_STATUS_FAILURE;
	}

	nonce = (struct IE_FILS_NONCE *) ie;

	DBGDUMP_MEM8(FILS, INFO, "ANonce\n",
		nonce->aucNonce, FILS_NONCE_LEN);

	kalMemCopy(fils->aucFilsANonce, nonce->aucNonce, FILS_NONCE_LEN);

	fils->fgFilsANonceSet = TRUE;

	return WLAN_STATUS_SUCCESS;
}

uint32_t filsRxAuthSession(struct ADAPTER *ad,
		       struct SW_RFB *rfb, struct IE_HDR *ie)
{
	struct STA_RECORD *sta;
	struct FILS_INFO *fils = NULL;
	struct IE_FILS_SESSION *session;

	sta = cnmGetStaRecByIndex(ad, rfb->ucStaRecIdx);
	if (!sta) {
		DBGLOG(FILS, ERROR, "No starec\n");
		return WLAN_STATUS_FAILURE;
	}

	if (!rsnIsFilsAuthAlg(sta->ucAuthAlgNum))
		return WLAN_STATUS_SUCCESS;

	fils = sta->prFilsInfo;
	if (!fils) {
		DBGLOG(FILS, ERROR, "No fils info\n");
		return WLAN_STATUS_FAILURE;
	}

	session = (struct IE_FILS_SESSION *) ie;

	DBGDUMP_MEM8(FILS, INFO, "Session\n",
		session->aucSession, FILS_SESSION_LEN);

	if (kalMemCmp(fils->aucFilsSession,
		session->aucSession, FILS_SESSION_LEN) != 0) {
		DBGLOG(FILS, ERROR, "Session mismatch\n");
		DBGDUMP_MEM8(FILS, ERROR, "Expected session\n",
			fils->aucFilsSession, FILS_SESSION_LEN);
		return WLAN_STATUS_FAILURE;
	}

	fils->fgFilsSessionMatch = TRUE;

	return WLAN_STATUS_SUCCESS;
}

int32_t filsErpParseTlvs(const uint8_t *pos, const uint8_t *end,
	struct ERP_TLVS *tlvs, uint8_t stop_at_keyname)
{
	memset(tlvs, 0, sizeof(*tlvs));

	while (pos < end) {
		u8 tlv_type, tlv_len;

		tlv_type = *pos++;
		switch (tlv_type) {
		case EAP_ERP_TV_RRK_LIFETIME:
		case EAP_ERP_TV_RMSK_LIFETIME:
			/* 4-octet TV */
			if (pos + 4 > end) {
				DBGLOG(FILS, INFO, "EAP: Too short TV");
				return -1;
			}
			pos += 4;
			break;
		case EAP_ERP_TLV_DOMAIN_NAME:
		case EAP_ERP_TLV_KEYNAME_NAI:
		case EAP_ERP_TLV_CRYPTOSUITES:
		case EAP_ERP_TLV_AUTHORIZATION_INDICATION:
		case EAP_ERP_TLV_CALLED_STATION_ID:
		case EAP_ERP_TLV_CALLING_STATION_ID:
		case EAP_ERP_TLV_NAS_IDENTIFIER:
		case EAP_ERP_TLV_NAS_IP_ADDRESS:
		case EAP_ERP_TLV_NAS_IPV6_ADDRESS:
			if (pos >= end) {
				DBGLOG(FILS, INFO, "EAP: Too short TLV\n");
				return -1;
			}
			tlv_len = *pos++;
			if (tlv_len > (unsigned) (end - pos)) {
				DBGLOG(FILS, INFO, "EAP: Truncated TLV\n");
				return -1;
			}
			if (tlv_type == EAP_ERP_TLV_KEYNAME_NAI) {
				if (tlvs->keyname) {
					DBGLOG(FILS, INFO,
						   "EAP: More than one keyName-NAI\n");
					return -1;
				}
				tlvs->keyname = pos;
				tlvs->keynameLen = tlv_len;
				if (stop_at_keyname)
					return 0;
			} else if (tlv_type == EAP_ERP_TLV_DOMAIN_NAME) {
				tlvs->domain = pos;
				tlvs->domainLen = tlv_len;
			}
			pos += tlv_len;
			break;
		default:
			if (tlv_type >= 128 && tlv_type <= 191) {
				/* Undefined TLV */
				if (pos >= end) {
					DBGLOG(FILS, INFO,
						   "EAP: Too short TLV\n");
					return -1;
				}
				tlv_len = *pos++;
				if (tlv_len > (unsigned) (end - pos)) {
					DBGLOG(FILS, INFO,
						   "EAP: Truncated TLV\n");
					return -1;
				}
				pos += tlv_len;
				break;
			}
			DBGLOG(FILS, INFO, "EAP: Unknown TV/TLV type %u\n",
				   tlv_type);
			pos = end;
			break;
		}
	}

	return 0;

}

uint32_t filsProcessErpFinish(struct ADAPTER *ad, struct FILS_INFO *fils,
	struct EAP_HDR *hdr, uint32_t len, uint8_t *rmsk, uint16_t *rmsk_len)
{
	const uint8_t *pos = (const uint8_t *) (hdr + 1);
	const uint8_t *end = ((const u8 *) hdr) + len;
	const uint8_t *start;
	struct ERP_TLVS parse;
	uint8_t flags;
	uint16_t seq;
	uint8_t hash[SHA256_MAC_LEN];
	uint16_t hash_len;
	int32_t max_len;
	uint8_t nai[FILS_MAX_KEY_NAME_NAI_LEN + 1];
	uint8_t seed[4];
	int32_t auth_tag_ok = 0;
	struct EAP_ERP_KEY *erp;

	if (!fils) {
		DBGLOG(FILS, ERROR, "EAP: No fils info\n");
		return WLAN_STATUS_NOT_ACCEPTED;
	}

	if (len < sizeof(*hdr) + 1) {
		DBGLOG(FILS, ERROR, "EAP: Ignored too short EAP-Finish\n");
		return WLAN_STATUS_NOT_ACCEPTED;
	}

	if (*pos != EAP_ERP_TYPE_REAUTH) {
		DBGLOG(FILS, ERROR,
			"EAP: Ignored unexpected EAP-Finish Type=%u\n", *pos);
		return WLAN_STATUS_NOT_ACCEPTED;
	}

	if (len < sizeof(*hdr) + 4) {
		DBGLOG(FILS, ERROR,
			"EAP: Ignored too short EAP-Finish/Re-auth\n");
		return WLAN_STATUS_NOT_ACCEPTED;
	}

	erp = fils->prErpKey;

	/* EAP-Finish/Re-auth Packet
	 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 * | Code          | Identifier    | Length                        |
	 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 * | Type          |R|B|L| Reserved| SEQ                           |
	 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 * | 1 or more TVs or TLVs                                         ~
	 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 * | Cryptosuite   | Authentication Tag                            ~
	 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 */

	pos++;
	flags = *pos++;
	WLAN_GET_FIELD_BE16(pos, &seq);
	pos += 2;
	DBGLOG(FILS, INFO, "EAP: Flags=0x%x SEQ=%u\n", flags, seq);

	if (seq != fils->u4ErpSeq) {
		DBGLOG(FILS, INFO,
			   "EAP: Unexpected EAP-Finish/Re-auth SEQ=%u", seq);
		return WLAN_STATUS_FAILURE;
	}

	if (filsErpParseTlvs(pos, end, &parse, 1) < 0) {
		DBGLOG(FILS, ERROR,
			"EAP: Parse Tlv failed, stop=1\n");
		return WLAN_STATUS_FAILURE;
	}

	if (!parse.keyname) {
		DBGLOG(FILS, ERROR,
			"EAP: No keyName-NAI in EAP-Finish/Re-auth Packet\n");
		return WLAN_STATUS_FAILURE;
	}

	if (parse.keynameLen > 253) {
		DBGLOG(FILS, ERROR,
			"EAP: Too long keyName-NAI in EAP-Finish/Re-auth\n");
		return WLAN_STATUS_FAILURE;
	}

	if (parse.keynameLen != erp->keynameNaiLen ||
	   kalMemCmp(parse.keyname, erp->keynameNai, erp->keynameNaiLen)) {
		kalMemCopy(nai, parse.keyname, parse.keynameLen);
		nai[parse.keynameLen] = '\0';
		DBGLOG(FILS, ERROR, "Unmatched keyname NAI req=%s, resp=%s\n",
			erp->keynameNai, nai);
		return WLAN_STATUS_FAILURE;
	}

	/* Is there enough room for Cryptosuite and Authentication Tag? */
	start = parse.keyname + parse.keynameLen;
	max_len = end - start;
	hash_len = 16;
	if (max_len < 1 + (int) hash_len) {
		DBGLOG(FILS, ERROR,
			"EAP: Not enough room for Authentication Tag\n");
		if (flags & 0x80)
			goto no_auth_tag;
		return WLAN_STATUS_FAILURE;
	}
	if (end[-17] != EAP_ERP_CS_HMAC_SHA256_128) {
		DBGLOG(FILS, ERROR, "EAP: Different Cryptosuite used\n");
		if (flags & 0x80)
			goto no_auth_tag;
		return WLAN_STATUS_FAILURE;
	}

	if (hmac_sha256(erp->rIK, erp->rIKLen, (const u8 *) hdr,
			end - ((const u8 *) hdr) - hash_len, hash) < 0) {
		DBGLOG(FILS, ERROR,
			   "EAP: hmac_sha256 failed\n");
		return WLAN_STATUS_FAILURE;
	}

	if (kalMemCmp(end - hash_len, hash, hash_len) != 0) {
		DBGLOG(FILS, ERROR,
			   "EAP: Authentication Tag mismatch\n");
		return WLAN_STATUS_FAILURE;
	}
	auth_tag_ok = 1;
	end -= 1 + hash_len;
no_auth_tag:
	/*
	 * Parse TVs/TLVs again now that we know the exact part of the buffer
	 * that contains them.
	 */
	DBGDUMP_MEM8(FILS, TRACE, "EAP: EAP-Finish/Re-Auth TVs/TLVs\n",
		    pos, end - pos);
	if (filsErpParseTlvs(pos, end, &parse, 0) < 0) {
		DBGLOG(FILS, ERROR,
			"EAP: Parse Tlv failed, stop=0\n");
		return WLAN_STATUS_FAILURE;
	}

	/* R=0: success, R=1: failure */
	if (flags & 0x80 || !auth_tag_ok) {
		DBGLOG(FILS, ERROR,
			"EAP: EAP-Finish/Re-auth indicated failure");
		return WLAN_STATUS_FAILURE;
	}

	/* generate rMSK = KDF(rRK, rMSK Label | "\0" | SEQ | length) */
	*rmsk_len = erp->rRKLen;
	WLAN_SET_FIELD_BE16(seed, seq);
	WLAN_SET_FIELD_BE16(&seed[2], erp->rRKLen);
	if (hmac_sha256_kdf(erp->rRK, erp->rRKLen,
			    "Re-authentication Master Session Key@ietf.org",
			    seed, sizeof(seed),
			    rmsk, *rmsk_len) < 0) {
		DBGLOG(FILS, ERROR, "EAP: Could not derive rMSK for ERP");
		return WLAN_STATUS_FAILURE;
	}
	DBGDUMP_MEM8(FILS, TRACE, "EAP: ERP rMSK\n", rmsk, *rmsk_len);

	return WLAN_STATUS_SUCCESS;
}

int filsRmskToPmk(int akmp, const uint8_t *rmsk, uint16_t rmskLen,
	const uint8_t *snonce, const uint8_t *anonce,
	uint8_t *pmk, uint16_t *pmkLen)
{
	uint8_t nonces[2 * FILS_NONCE_LEN];
	const uint8_t *addr[2];
	size_t len[2];
	size_t num_elem;
	int res;

	/* PMK = HMAC-Hash(SNonce || ANonce, rMSK [ || DHss ]) */
	DBGLOG(FILS, INFO, "rMSK to PMK derivation");

	if (rsnIsKeyMgmtSha384(akmp))
		*pmkLen = SHA384_MAC_LEN;
	else if (rsnIsKeyMgmtSha256(akmp))
		*pmkLen = SHA256_MAC_LEN;
	else
		return -1;

	kalMemCopy(nonces, snonce, FILS_NONCE_LEN);
	kalMemCopy(&nonces[FILS_NONCE_LEN], anonce, FILS_NONCE_LEN);
	addr[0] = rmsk;
	len[0] = rmskLen;
	num_elem = 1;
	if (rsnIsKeyMgmtSha384(akmp))
		res = hmac_sha384_vector(nonces, 2 * FILS_NONCE_LEN, num_elem,
					 addr, len, pmk);
	else
		res = hmac_sha256_vector(nonces, 2 * FILS_NONCE_LEN, num_elem,
					 addr, len, pmk);

	if (res != 0)
		*pmkLen = 0;

	if (*pmkLen)
		DBGDUMP_MEM8(FILS, TRACE, "FILS: PMK\n", pmk, *pmkLen);

	return res;
}

uint32_t filsRxAuthWrapped(struct ADAPTER *ad,
		       struct SW_RFB *rfb, struct IE_HDR *ie)
{
	struct BSS_INFO *bss;
	struct STA_RECORD *sta;
	struct FILS_INFO *fils = NULL;
	struct IE_FILS_WRAPPED_DATA *wrapped;
	uint16_t left;
	const uint8_t *head, *end;
	uint8_t *tmp = NULL;
	uint8_t rmsk[ERP_MAX_KEY_LEN];
	uint16_t rmsk_len;

	sta = cnmGetStaRecByIndex(ad, rfb->ucStaRecIdx);
	if (!sta) {
		DBGLOG(FILS, ERROR, "No starec\n");
		return WLAN_STATUS_FAILURE;
	}

	if (!rsnIsFilsAuthAlg(sta->ucAuthAlgNum))
		return WLAN_STATUS_SUCCESS;

	fils = sta->prFilsInfo;
	if (!fils) {
		DBGLOG(FILS, ERROR, "No fils info\n");
		return WLAN_STATUS_FAILURE;
	}

	bss = GET_BSS_INFO_BY_INDEX(ad, sta->ucBssIndex);
	if (!bss) {
		DBGLOG(FILS, ERROR, "no bss%d", sta->ucBssIndex);
		return WLAN_STATUS_FAILURE;
	}

	if (fils->fgFilsPmkSet) {
		DBGLOG(FILS, ERROR, "PMK matchd, ignore wrapped data\n");
		return WLAN_STATUS_SUCCESS;
	}

	if (!fils->fgFilsErpPmkidSet) {
		DBGLOG(FILS, ERROR, "FILS: PMKID not available");
		return WLAN_STATUS_FAILURE;
	}

	/* |             left            |
	 * | wrapped data ie | ......... |
	 * |                 |           |
	 * head             end
	 */
	head = (uint8_t *)ie;
	end = head + IE_SIZE(ie);
	left = (uint8_t *)rfb->pvHeader + rfb->u2PacketLen - head;
	wrapped = (struct IE_FILS_WRAPPED_DATA *) ie;

	if (left > IE_SIZE(wrapped)) {
		const uint8_t *tmp_pos, *tmp_end;
		uint8_t *p;
		uint8_t found = FALSE;

		tmp_pos = end; /* traverse original buffer */
		tmp_end = head + left;
		while (tmp_end - tmp_pos >= 2 &&
		       IE_ID(tmp_pos) == ELEM_ID_FRAGMENT &&
		       IE_SIZE(tmp_pos) <= tmp_end - tmp_pos) {
			found = TRUE;
			tmp_pos += IE_SIZE(tmp_pos);
			break;
		}

		if (!found)
			goto process;

		tmp = kalMemAlloc(left, VIR_MEM_TYPE);
		if (!tmp) {
			DBGLOG(ML, WARN, "No resource left=%d\n", left);
			goto process;
		}

		/* copy wrapped data ie */
		kalMemCopy(tmp, head, end - head);

		/* pos to copy fragement */
		p = tmp + (uint32_t)(end - head);
		tmp_pos = end; /* traverse original buffer */
		tmp_end = head + left;

		/* Add possible fragments */
		while (tmp_end - tmp_pos >= 2 &&
		       IE_ID(tmp_pos) == ELEM_ID_FRAGMENT &&
		       IE_SIZE(tmp_pos) <= tmp_end - tmp_pos) {
			kalMemCopy(p, IE_DATA(tmp_pos), IE_LEN(tmp_pos));
			p += IE_LEN(tmp_pos);
			tmp_pos += IE_SIZE(tmp_pos);
		}

		/* | wrapped data ie + frag(no hdr) + frag(no hdr) + ... |
		 * head							end
		 */
		head = tmp;
		end = p;

		DBGLOG(FILS, INFO, "Found fragment\n");
	}
process:
	DBGDUMP_MEM8(FILS, TRACE, "Wrapped Data", head, end - head);

	if (filsProcessErpFinish(ad, sta->prFilsInfo,
		(struct EAP_HDR *)(head + 3), end - head - 3,
		rmsk, &rmsk_len) != WLAN_STATUS_SUCCESS)
		goto fail;

	/* rmsk is available */
	if (filsRmskToPmk(bss->u4RsnSelectedAKMSuite,
			rmsk, rmsk_len,
			fils->aucFilsNonce, fils->aucFilsANonce,
			fils->aucFilsPmk, &fils->u2FilsPmkLen) < 0) {
		DBGLOG(FILS, ERROR, "RMSK to PMK failed\n");
		return WLAN_STATUS_FAILURE;
	}

	fils->fgFilsPmkSet = TRUE;
	forced_memzero(rmsk, rmsk_len);

	DBGLOG(FILS, INFO, "ERP processing succeeded\n");
fail:
	if (tmp)
		kalMemFree(tmp, left, VIR_MEM_TYPE);

	return WLAN_STATUS_SUCCESS;
}

void filsBuildAssocIE(struct ADAPTER *ad, struct MSDU_INFO *msdu)
{
	struct STA_RECORD *sta;
	struct FILS_INFO *fils = NULL;
	uint8_t *cp, *buf;
	struct IE_FILS_SESSION *session;
	struct IE_FILS_KEY_CONFIRMATION *confirm;

	sta = cnmGetStaRecByIndex(ad, msdu->ucStaRecIndex);
	if (!sta) {
		DBGLOG(FILS, ERROR, "No starec\n");
		return;
	}

	if (!rsnIsFilsAuthAlg(sta->ucAuthAlgNum))
		return;

	fils = sta->prFilsInfo;
	if (!fils) {
		DBGLOG(FILS, ERROR, "No fils info\n");
		return;
	}

	buf = (uint8_t *)msdu->prPacket + msdu->u2FrameLength;
	cp = buf;

	session = (struct IE_FILS_SESSION *) cp;
	session->ucId = ELEM_ID_EXTENSION;
	session->ucLength = 1 + FILS_SESSION_LEN;
	session->ucExtId = ELEM_EXT_ID_FILS_SESSION;
	kalMemCopy(session->aucSession, fils->aucFilsSession, FILS_SESSION_LEN);
	cp += IE_SIZE(session);

	DBGDUMP_MEM8(FILS, INFO, "SESSION\n", session, IE_SIZE(session));

	confirm = (struct IE_FILS_KEY_CONFIRMATION *) cp;
	confirm->ucId = ELEM_ID_EXTENSION;
	confirm->ucLength = 1 + fils->u2FilsKeyAuthLen;
	confirm->ucExtId = ELEM_EXT_ID_FILS_KEY_CONFIRM;
	kalMemCopy(confirm->aucKeyAuth,	fils->aucFilsKeyAuthSTA,
		fils->u2FilsKeyAuthLen);
	cp += IE_SIZE(confirm);

	DBGDUMP_MEM8(FILS, INFO, "CONFIRM\n", confirm, IE_SIZE(confirm));

	msdu->u2FrameLength += cp - buf;
}

uint16_t filsFindIeAfterSession(struct ADAPTER *ad, uint8_t *buf,
	uint16_t buf_len, uint8_t **pos)
{
	int offset = 0;
	const uint8_t *session;

	offset = sortGetPayloadOffset(ad, buf);
	if (offset < 0)
		return 0;

	session = kalFindIeExtIE(ELEM_ID_EXTENSION, ELEM_EXT_ID_FILS_SESSION,
		buf + offset, buf_len - offset);
	if (session) {
		/* plaintext starts from ie after fils session */
		*pos = (uint8_t *)session + IE_SIZE(session);
		return buf + buf_len - session - IE_SIZE(session);
	}

	return 0;
}

uint32_t filsEncryptAssocReq(struct ADAPTER *ad, struct MSDU_INFO *msdu)
{
	struct STA_RECORD *sta;
	struct FILS_INFO *fils = NULL;
	struct WLAN_ASSOC_REQ_FRAME *mgmt;
	uint8_t *plain, *pos = NULL;
	uint16_t plain_len;
	const u8 *aad[5];
	size_t aad_len[5];

	sta = cnmGetStaRecByIndex(ad, msdu->ucStaRecIndex);
	if (!sta) {
		DBGLOG(FILS, ERROR, "No starec\n");
		return WLAN_STATUS_FAILURE;
	}

	if (!rsnIsFilsAuthAlg(sta->ucAuthAlgNum))
		return WLAN_STATUS_SUCCESS;

	fils = sta->prFilsInfo;
	if (!fils) {
		DBGLOG(FILS, ERROR, "No fils info\n");
		return WLAN_STATUS_FAILURE;
	}

	plain_len = filsFindIeAfterSession(ad,
		msdu->prPacket, msdu->u2FrameLength, &pos);
	plain = kalMemAlloc(plain_len, VIR_MEM_TYPE);
	if (!plain) {
		DBGLOG(FILS, ERROR, "No memory\n");
		return WLAN_STATUS_RESOURCES;
	}

	if (!pos) {
		kalMemFree(plain, plain_len, VIR_MEM_TYPE);
		return WLAN_STATUS_FAILURE;
	}

	kalMemCopy(plain, pos, plain_len);

	mgmt = (struct WLAN_ASSOC_REQ_FRAME *)(msdu->prPacket);

	/* AES-SIV AAD vectors */

	/* The STA's MAC address */
	aad[0] = mgmt->aucSrcAddr;
	aad_len[0] = ETH_ALEN;
	/* The AP's BSSID */
	aad[1] = mgmt->aucDestAddr;
	aad_len[1] = ETH_ALEN;
	/* The STA's nonce */
	aad[2] = fils->aucFilsNonce;
	aad_len[2] = FILS_NONCE_LEN;
	/* The AP's nonce */
	aad[3] = fils->aucFilsANonce;
	aad_len[3] = FILS_NONCE_LEN;
	/*
	 * The (Re)Association Request frame from the Capability Information
	 * field (the same offset in both Association and Reassociation
	 * Response frames) to the FILS Session element (both inclusive).
	 */
	aad[4] = (const uint8_t *) &mgmt->u2CapInfo;
	aad_len[4] = pos - aad[4];

	DBGDUMP_MEM8(FILS, TRACE, "Plain text\n", plain, plain_len);

	if (aes_siv_encrypt(fils->kek, fils->kek_len,
			    plain, plain_len,
			    5, aad, aad_len, pos) < 0) {
		kalMemFree(plain, plain_len, VIR_MEM_TYPE);
		return WLAN_STATUS_FAILURE;
	}

	DBGDUMP_MEM8(FILS, TRACE, "Crypt text\n",
		    pos, FILS_AES_BLOCK_SIZE + plain_len);

	/* AEAD encrypted output (crypt text) has
	 * length = plain_len + AES_BLOCK_SIZE (AEAD encrption header)
	 */
	msdu->u2FrameLength += FILS_AES_BLOCK_SIZE;

	kalMemFree(plain, plain_len, VIR_MEM_TYPE);

	return WLAN_STATUS_SUCCESS;
}

uint32_t filsDecryptAssocResp(struct ADAPTER *ad, struct SW_RFB *rfb,
	struct FILS_INFO *fils)
{
	uint8_t *pos, *crypt, *end;
	uint16_t crypt_len;
	const uint8_t *aad[5];
	size_t aad_len[5];
	struct WLAN_ASSOC_RSP_FRAME *mgmt;

	pos = end = (uint8_t *)rfb->pvHeader + rfb->u2PacketLen;
	crypt_len = filsFindIeAfterSession(ad,
		rfb->pvHeader, rfb->u2PacketLen, &pos);

	if (end - pos < FILS_AES_BLOCK_SIZE) {
		DBGLOG(FILS, ERROR, "Too short to include AES-SIV data\n");
		return WLAN_STATUS_FAILURE;
	}

	crypt = kalMemAlloc(crypt_len, VIR_MEM_TYPE);
	if (!crypt) {
		DBGLOG(FILS, ERROR, "No memory\n");
		return WLAN_STATUS_RESOURCES;
	}

	kalMemCopy(crypt, pos, crypt_len);

	mgmt = (struct WLAN_ASSOC_RSP_FRAME *)(rfb->pvHeader);

	/* AES-SIV AAD vectors */

	/* The AP's BSSID */
	aad[0] = mgmt->aucSrcAddr;
	aad_len[0] = ETH_ALEN;
	/* The STA's MAC address */
	aad[1] = mgmt->aucDestAddr;
	aad_len[1] = ETH_ALEN;
	/* The AP's nonce */
	aad[2] = fils->aucFilsANonce;
	aad_len[2] = FILS_NONCE_LEN;
	/* The STA's nonce */
	aad[3] = fils->aucFilsNonce;
	aad_len[3] = FILS_NONCE_LEN;
	/*
	 * The (Re)Association Resp frame from the Capability Information
	 * field to the FILS Session element (both inclusive).
	 */
	aad[4] = (const uint8_t *) &mgmt->u2CapInfo;
	aad_len[4] = pos - aad[4];

	DBGDUMP_MEM8(FILS, TRACE, "Crypt text\n", crypt, crypt_len);

	if (aes_siv_decrypt(fils->kek, fils->kek_len, crypt, crypt_len,
			    5, aad, aad_len, pos) < 0) {
		DBGLOG(FILS, INFO,
			   "FILS: Invalid AES-SIV data in the frame");
		kalMemFree(crypt, crypt_len, VIR_MEM_TYPE);
		return -1;
	}

	DBGDUMP_MEM8(FILS, TRACE, "Plain text\n",
		    pos, crypt_len - FILS_AES_BLOCK_SIZE);

	rfb->u2PacketLen -= FILS_AES_BLOCK_SIZE;

	kalMemFree(crypt, crypt_len, VIR_MEM_TYPE);
	return WLAN_STATUS_SUCCESS;
}

uint32_t filsValidateAssocResp(struct ADAPTER *ad, struct SW_RFB *rfb,
	struct FILS_INFO *fils)
{
	uint8_t *ie;
	uint16_t ie_len, ie_offset;
	int offset;
	uint8_t confirmed = FALSE;
	uint8_t key_delivery = FALSE;

	offset = sortGetPayloadOffset(ad, rfb->pvHeader);
	if (offset < 0) {
		DBGLOG(FILS, ERROR, "unknown packet");
		return WLAN_STATUS_FAILURE;
	}

	ie = rfb->pvHeader + offset;
	ie_len = rfb->u2PacketLen - offset;

	IE_FOR_EACH(ie, ie_len, ie_offset) {
		if (IE_ID(ie) == ELEM_ID_EXTENSION &&
		    IE_ID_EXT(ie) == ELEM_EXT_ID_FILS_SESSION) {
			if (kalMemCmp(fils->aucFilsSession, ie + 3,
				FILS_SESSION_LEN) != 0) {
				DBGLOG(FILS, ERROR, "Session mismatch");
				DBGDUMP_MEM8(FILS, ERROR,
					"Expected FILS Session\n",
					fils->aucFilsSession, FILS_SESSION_LEN);
				DBGDUMP_MEM8(FILS, ERROR,
					"Received FILS Session",
					ie + 3, FILS_SESSION_LEN);
				return WLAN_STATUS_FAILURE;
			}
		}

		if (IE_ID(ie) == ELEM_ID_EXTENSION &&
		    IE_ID_EXT(ie) == ELEM_EXT_ID_FILS_KEY_CONFIRM) {
			if (IE_LEN(ie) - 1 != fils->u2FilsKeyAuthLen ||
			    kalMemCmp(fils->aucFilsKeyAuthAP, ie + 3,
				fils->u2FilsKeyAuthLen) != 0) {
				DBGLOG(FILS, ERROR, "Key-Auth mismatch");
				DBGDUMP_MEM8(FILS, ERROR, "Expected Key-Auth\n",
					fils->aucFilsKeyAuthAP,
					fils->u2FilsKeyAuthLen);
				DBGDUMP_MEM8(FILS, ERROR, "Received Key-Auth",
					ie + 3, fils->u2FilsKeyAuthLen);
				return WLAN_STATUS_FAILURE;
			}
			confirmed = TRUE;
		}

		if (IE_ID(ie) == ELEM_ID_EXTENSION &&
		    IE_ID_EXT(ie) == ELEM_EXT_ID_KEY_DELIVERY) {
			struct IE_KEY_DELIVERY *key =
				(struct IE_KEY_DELIVERY *) ie;
			const uint8_t *pos, *end, *p;
			size_t dlen = 0, left;
			uint32_t selector;

			kalMemCopy(fils->rsc, key->aucKeyRSC, 8);

			for (pos = key->aucKDEList, end = ie + IE_SIZE(ie);
				end - pos > 1; pos += dlen) {
				if (pos[0] == 0xdd &&
				    ((pos == end - 1) || pos[1] == 0)) {
					/* Ignore padding */
					break;
				}
				dlen = 2 + pos[1];
				if ((int) dlen > end - pos) {
					DBGLOG(FILS, ERROR,
						"Key Data underflow (ie=%d len=%d pos=%d)",
						pos[0], pos[1],
						(int) (pos - key->aucKDEList));
					return WLAN_STATUS_FAILURE;
				}

				if (*pos != WLAN_EID_VENDOR_SPECIFIC)
					continue;

				if (pos[1] < 4) {
					DBGLOG(FILS, ERROR,
						"Invalid KDE len=%d\n",	pos[1]);
					return WLAN_STATUS_FAILURE;
				}

				p = pos + 2;
				WLAN_GET_FIELD_BE32(p, &selector);
				p += 4;
				left = pos[1] - 4;

				if (selector == RSN_KEY_DATA_GROUPKEY) {
					if (left > FILS_GTK_MAX_LEN + 2) {
						DBGLOG(FILS, ERROR,
							"wrong gtk len=%d",
							left);
						continue;
					}
					fils->gtk_kde_len = left;
					kalMemCopy(fils->gtk_kde, p, left);
				} else if (selector == RSN_KEY_DATA_IGTK) {
					if (left > FILS_IGTK_MAX_LEN +
						IGTK_KDE_PREFIX_LEN) {
						DBGLOG(FILS, ERROR,
							"wrong igtk len=%d",
							left);
						continue;
					}
					fils->igtk_kde_len = left;
					kalMemCopy(fils->igtk_kde, p, left);
				} else if (selector == RSN_KEY_DATA_BIGTK) {
					if (left > FILS_BIGTK_MAX_LEN +
						BIGTK_KDE_PREFIX_LEN) {
						DBGLOG(FILS, ERROR,
							"wrong bigtk len=%d",
							left);
						continue;
					}
					fils->bigtk_kde_len = left;
					kalMemCopy(fils->bigtk_kde, p, left);
				} else {
					/* TODO: MLO KDE */
					DBGLOG(FILS, ERROR,
						"unknown kde 0x%x\n", selector);
				}
			}

			key_delivery = TRUE;
		}
	}

	if (!confirmed) {
		DBGDUMP_MEM8(FILS, ERROR, "No FILS Key Confirm element\n",
			rfb->pvHeader + offset, rfb->u2PacketLen - offset);
		return WLAN_STATUS_FAILURE;
	}

	if (!key_delivery) {
		DBGDUMP_MEM8(FILS, ERROR, "No Key Delivery element\n",
			rfb->pvHeader + offset, rfb->u2PacketLen - offset);
		return WLAN_STATUS_FAILURE;
	}

	return WLAN_STATUS_SUCCESS;
}

uint32_t filsProcessAssocResp(struct ADAPTER *ad, struct SW_RFB *rfb)
{
	struct STA_RECORD *sta;
	struct FILS_INFO *fils = NULL;

	sta = cnmGetStaRecByIndex(ad, rfb->ucStaRecIdx);
	if (!sta) {
		DBGLOG(FILS, ERROR, "No starec\n");
		return WLAN_STATUS_FAILURE;
	}

	if (!rsnIsFilsAuthAlg(sta->ucAuthAlgNum))
		return WLAN_STATUS_SUCCESS;

	fils = sta->prFilsInfo;
	if (!fils) {
		DBGLOG(FILS, ERROR, "No fils info\n");
		return WLAN_STATUS_FAILURE;
	}

	if (filsDecryptAssocResp(ad, rfb, fils) != WLAN_STATUS_SUCCESS) {
		DBGLOG(FILS, ERROR, "Decrypt fils info failed\n");
		return WLAN_STATUS_FAILURE;
	}

	if (filsValidateAssocResp(ad, rfb, fils)) {
		DBGLOG(FILS, ERROR, "Validate fils info failed\n");
		return WLAN_STATUS_FAILURE;
	}

	return WLAN_STATUS_SUCCESS;
}

uint32_t filsFillParamKey(struct PARAM_KEY *param, uint8_t *bssid,
	uint8_t *key, uint16_t key_len, uint8_t *seq, uint16_t seq_len,
	uint8_t pairwise, uint32_t key_index, uint32_t cipher, uint8_t bssidx)
{
	uint8_t aucBCAddr[] = BC_MAC_ADDR;

	kalMemZero(param, sizeof(struct PARAM_KEY));

	param->ucBssIdx = bssidx;
	param->u4KeyIndex = key_index;

	if (pairwise && bssid) {
		param->u4KeyIndex |= BIT(31);
		param->u4KeyIndex |= BIT(30);
		COPY_MAC_ADDR(param->arBSSID, bssid);
	} else {		/* Group key */
		COPY_MAC_ADDR(param->arBSSID, aucBCAddr);
	}

	switch (cipher) {
	case RSN_CIPHER_SUITE_WEP40:
		param->ucCipher = CIPHER_SUITE_WEP40;
		break;
	case RSN_CIPHER_SUITE_WEP104:
		param->ucCipher = CIPHER_SUITE_WEP104;
		break;
	case RSN_CIPHER_SUITE_TKIP:
		param->ucCipher = CIPHER_SUITE_TKIP;
		break;
	case RSN_CIPHER_SUITE_CCMP:
		param->ucCipher = CIPHER_SUITE_CCMP;
		break;
	case RSN_CIPHER_SUITE_GCMP:
		param->ucCipher = CIPHER_SUITE_GCMP_128;
		break;
	case RSN_CIPHER_SUITE_GCMP_256:
		param->ucCipher = CIPHER_SUITE_GCMP_256;
		break;
	case RSN_CIPHER_SUITE_AES_128_CMAC:
		param->ucCipher = CIPHER_SUITE_BIP_CMAC_128;
		break;
	case RSN_CIPHER_SUITE_BIP_GMAC_256:
		param->ucCipher = CIPHER_SUITE_BIP_GMAC_256;
		break;
	default:
		DBGLOG(FILS, WARN, "invalid cipher (0x%x)\n", cipher);
		return WLAN_STATUS_FAILURE;
	}

	if (key) {
		if (key_len > sizeof(param->aucKeyMaterial)) {
			DBGLOG(FILS, WARN, "key too long %d\n", key_len);
			return WLAN_STATUS_RESOURCES;
		}

		kalMemCopy(param->aucKeyMaterial, key, key_len);

		if (param->ucCipher == CIPHER_SUITE_TKIP) {
			uint8_t tmp1[8], tmp2[8];

			kalMemCopy(tmp1, &key[16], 8);
			kalMemCopy(tmp2, &key[24], 8);
			kalMemCopy(&param->aucKeyMaterial[16], tmp2, 8);
			kalMemCopy(&param->aucKeyMaterial[24], tmp1, 8);
		}
	}

	param->u4KeyLength = key_len;
	param->u4Length = OFFSET_OF(struct PARAM_KEY, aucKeyMaterial) +
			  param->u4KeyLength;

	if (seq_len == 6) /* IGTK Package Number */
		kalMemCopy(param->aucKeyPn, seq, seq_len);

	DBGLOG(FILS, INFO,
		"keyidx=0x%x,keylen=%d,bssid="MACSTR
		",rsc=0x%08x,bssidx=%d,cipher=%d\n",
		param->u4KeyIndex,
		param->u4KeyLength,
		MAC2STR(param->arBSSID),
		param->rKeyRSC,
		param->ucBssIdx,
		param->ucCipher);

	return WLAN_STATUS_SUCCESS;
}

void filsRemoveKey(struct ADAPTER *ad, uint32_t keyidx, uint8_t bssidx)
{
	struct BSS_INFO *bss = GET_BSS_INFO_BY_INDEX(ad, bssidx);
	uint32_t len;
	struct PARAM_REMOVE_KEY param;

	if (!bss)
		return;

	param.u4Length = sizeof(struct PARAM_REMOVE_KEY);
	param.u4KeyIndex = keyidx;
	param.ucBssIdx = bssidx;
	COPY_MAC_ADDR(param.arBSSID, bss->aucBSSID);
	if (bss->aucBSSID[0] != '\0')
		param.u4KeyIndex |= BIT(30);

	DBGLOG(FILS, INFO, "Bss%d BSSID[" MACSTR "] remove key %d\n",
		bssidx, MAC2STR(param.arBSSID), keyidx);

	wlanSetRemoveKey(ad,
		(void *)&param,
		sizeof(struct PARAM_REMOVE_KEY),
		&len, FALSE);
}

uint32_t filsInstallPTK(struct ADAPTER *ad, struct STA_RECORD *sta)
{
	uint8_t bssidx = sta->ucBssIndex;
	struct BSS_INFO *bss = GET_BSS_INFO_BY_INDEX(ad, bssidx);
	struct FILS_INFO *fils = sta->prFilsInfo;
	struct PARAM_KEY param;
	uint8_t keyidx = 0;
	uint8_t pairwise = TRUE;
	uint32_t cipher;
	uint32_t len = 0;

	if (!bss)
		return WLAN_STATUS_FAILURE;

	if (bss->filskeyUsed[keyidx]) {
		DBGLOG(FILS, WARN, "Bss%d key %d not cleard\n",	bssidx, keyidx);
		filsRemoveKey(ad, keyidx, bssidx);
		bss->filskeyUsed[keyidx] = FALSE;
	}

	cipher = bss->u4RsnSelectedPairwiseCipher;
	if (filsFillParamKey(&param,
		sta->aucMacAddr,
		fils->tk, fils->tk_len,
		NULL, 0,
		pairwise,
		keyidx,
		cipher,
		bssidx)) {
		DBGLOG(FILS, ERROR, "BSSID[" MACSTR "] fill key failed\n",
			MAC2STR(sta->aucMacAddr));
		return WLAN_STATUS_FAILURE;
	}

	bss->filskeyUsed[keyidx] = TRUE;
	wlanSetAddKey(ad, &param, sizeof(param), &len, FALSE);

	return WLAN_STATUS_SUCCESS;
}

uint32_t filsInstallGTK(struct ADAPTER *ad, struct STA_RECORD *sta)
{
	uint8_t bssidx = sta->ucBssIndex;
	struct BSS_INFO *bss = GET_BSS_INFO_BY_INDEX(ad, bssidx);
	struct FILS_INFO *fils = sta->prFilsInfo;
	struct PARAM_KEY param;
	uint8_t keyidx = 0;
	uint8_t pairwise = FALSE;
	uint32_t cipher;
	uint32_t len = 0;
	uint16_t rsc_len, gtk_len;
	uint8_t *gtk;

	if (!bss)
		return WLAN_STATUS_FAILURE;

	cipher = bss->u4RsnSelectedGroupCipher;
	switch (cipher) {
	case RSN_CIPHER_SUITE_CCMP:
	case RSN_CIPHER_SUITE_GCMP:
	case RSN_CIPHER_SUITE_CCMP_256:
	case RSN_CIPHER_SUITE_GCMP_256:
	case RSN_CIPHER_SUITE_TKIP:
		rsc_len = 6;
		break;
	default:
		rsc_len = 0;
	}

	/*
	 * IEEE Std 802.11i-2004 - 8.5.2 EAPOL-Key frames - Figure 43x
	 * GTK KDE format:
	 * KeyID[bits 0-1], Tx [bit 2], Reserved [bits 3-7]
	 * Reserved [bits 0-7]
	 * GTK
	 */
	gtk = fils->gtk_kde;
	gtk_len = fils->gtk_kde_len;

	keyidx = gtk[0] & 0x3;

	gtk += 2;
	gtk_len -= 2;

	if (keyidx > MAX_KEY_NUM) {
		DBGLOG(FILS, ERROR, "wrong keyidx=%d\n", keyidx);
		return WLAN_STATUS_FAILURE;
	}

	if (bss->filskeyUsed[keyidx]) {
		DBGLOG(FILS, WARN, "Bss%d key %d not cleard\n",	bssidx, keyidx);
		filsRemoveKey(ad, keyidx, bssidx);
		bss->filskeyUsed[keyidx] = FALSE;
	}

	if (filsFillParamKey(&param,
		sta->aucMacAddr,
		gtk, gtk_len,
		fils->rsc, rsc_len,
		pairwise,
		keyidx,
		cipher,
		bssidx)) {
		DBGLOG(FILS, ERROR, "STA[" MACSTR "] fill key failed\n",
			MAC2STR(sta->aucMacAddr));
		return WLAN_STATUS_FAILURE;
	}

	bss->filskeyUsed[keyidx] = TRUE;
	wlanSetAddKey(ad, &param, sizeof(param), &len, FALSE);

	return WLAN_STATUS_SUCCESS;
}

uint32_t filsInstallIGTK(struct ADAPTER *ad, struct STA_RECORD *sta)
{
	uint8_t bssidx = sta->ucBssIndex;
	struct BSS_INFO *bss = GET_BSS_INFO_BY_INDEX(ad, bssidx);
	struct GL_WPA_INFO *wpa = aisGetWpaInfo(ad, bssidx);
	struct FILS_INFO *fils = sta->prFilsInfo;
	struct PARAM_KEY param;
	struct IGTK_KDE *igtk;
	uint16_t keyidx = 0;
	uint8_t pairwise = FALSE;
	uint32_t cipher = wpa->u4CipherGroupMgmt;
	uint32_t len = 0, key_len;

	igtk = (struct IGTK_KDE *) fils->igtk_kde;
	key_len = rsnCipherKeyLen(cipher);

	if (fils->igtk_kde_len != IGTK_KDE_PREFIX_LEN + key_len) {
		DBGLOG(FILS, ERROR, "wrong igtk kde len=%d, key_len=%d",
			fils->igtk_kde_len, key_len);
		return WLAN_STATUS_FAILURE;
	}

	WLAN_GET_FIELD_16(igtk->keyid, &keyidx);

	if (keyidx >= MAX_KEY_NUM) {
		DBGLOG(FILS, ERROR, "wrong keyidx=%d\n", keyidx);
		return WLAN_STATUS_FAILURE;
	}

	if (!bss)
		return WLAN_STATUS_FAILURE;

	if (bss->filskeyUsed[keyidx]) {
		DBGLOG(FILS, WARN, "Bss%d key %d not cleard\n",	bssidx, keyidx);
		filsRemoveKey(ad, keyidx, bssidx);
		bss->filskeyUsed[keyidx] = FALSE;
	}

	if (filsFillParamKey(&param,
		sta->aucMacAddr,
		igtk->igtk, key_len,
		igtk->pn, sizeof(igtk->pn),
		pairwise,
		keyidx,
		cipher,
		bssidx)) {
		DBGLOG(FILS, ERROR, "STA[" MACSTR "] fill key failed\n",
			MAC2STR(sta->aucMacAddr));
		return WLAN_STATUS_FAILURE;
	}

	bss->filskeyUsed[keyidx] = TRUE;
	wlanSetAddKey(ad, &param, sizeof(param), &len, FALSE);

	return WLAN_STATUS_SUCCESS;
}

uint32_t filsInstallKey(struct ADAPTER *ad, struct STA_RECORD *sta)
{
	uint8_t bssidx = sta->ucBssIndex;
	struct FILS_INFO *fils = NULL;

	if (!rsnIsFilsAuthAlg(sta->ucAuthAlgNum))
		return WLAN_STATUS_SUCCESS;

	if (!IS_BSS_INDEX_AIS(ad, bssidx)) {
		DBGLOG(FILS, ERROR, "bss%d but only support ais\n", bssidx);
		return WLAN_STATUS_NOT_ACCEPTED;
	}

	fils = sta->prFilsInfo;
	if (!fils) {
		DBGLOG(FILS, ERROR, "No fils info\n");
		return WLAN_STATUS_FAILURE;
	}

	if (fils->tk_len)
		filsInstallPTK(ad, sta);

	if (fils->gtk_kde_len)
		filsInstallGTK(ad, sta);

	if (fils->igtk_kde_len)
		filsInstallIGTK(ad, sta);

	return WLAN_STATUS_SUCCESS;
}

uint32_t filsRemoveAllKeys(struct ADAPTER *ad, uint8_t bssidx)
{
	struct BSS_INFO *bss = GET_BSS_INFO_BY_INDEX(ad, bssidx);
	uint8_t i;

	if (!bss)
		return WLAN_STATUS_FAILURE;

	for (i = 0; i < MAX_KEY_NUM; i++) {
		if (!bss->filskeyUsed[i])
			continue;
		filsRemoveKey(ad, i, bssidx);
		bss->filskeyUsed[i] = FALSE;
	}

	return WLAN_STATUS_SUCCESS;
}

#endif /* CFG_SUPPORT_FILS_SK_OFFLOAD */

