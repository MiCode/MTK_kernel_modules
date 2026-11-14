/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (c) 2024 MediaTek Inc.
 */
#ifndef _NAN_PAIRING_H_
#define _NAN_PAIRING_H_
#include "nanDiscovery.h"

#if CFG_SUPPORT_NAN

#define OOB_TYPE_ACTION         0
#define OOB_TYPE_AUTH           1

#if (CFG_SUPPORT_NAN_R4_VENDOR_PAIRING == 1)
#define NAN_NIK_LEN     16 /* 128 bits */
#define NAN_KCK_MAX_LEN 128
#define NAN_KEK_MAX_LEN 32
#define NAN_KDK_MAX_LEN 32
#define NAN_TK_MAX_LEN  32

struct NIK_KDE_INFO {
	uint8_t type;
	uint8_t length;
	uint8_t oui[3];
	uint8_t data_type;
	uint8_t cipher_ver;
	/* NIK length: 128 bits */
	uint8_t nik[NAN_NIK_LEN];
} __packed;

struct NIK_LIFETIME_KDE_INFO {
	uint8_t type;
	uint8_t length;
	uint8_t oui[3];
	uint8_t data_type;
	uint16_t key_bitmap;
	uint32_t lifetime;
} __packed;

struct nan_ptk {
	u8 kck[NAN_KCK_MAX_LEN]; /* EAPOL-Key Key Confirmation Key (KCK) */
	u8 kek[NAN_KEK_MAX_LEN]; /* EAPOL-Key Key Encryption Key (KEK) */
	u8 kdk[NAN_KDK_MAX_LEN]; /* EAPOL-Key Key Derivation Key (KDK) */
	u8 tk[NAN_TK_MAX_LEN];   /* Temporal Key (TK) */
	size_t kck_len;
	size_t kek_len;
	size_t kdk_len;
	size_t tk_len;
	int installed;
};

struct PAIRING_INFO {
	/* Index of Pairing FSM */
	uint8_t ucIndex;
	/* Pairing FSM is in use or not */
	uint8_t fgIsInUse;
	struct nan_ptk rPtk;
	uint32_t u4SelCipherType;
	struct _NAN_KEY_ENTRY_T *prNmiCxtKey;
};

void
nanPairingInstallTk(struct ADAPTER *prAdapter,
	struct nan_ptk *prPtk,
	uint32_t u4SelCipherType,
	uint8_t *pucPeerNMI,
	uint16_t u2WtblEntry);

struct PAIRING_INFO *
nanPairingInfoSearch(struct ADAPTER *prAdapter,
	uint8_t *pucPeerAddr);

void
nanPairingInit(struct ADAPTER *prAdapter);

void
nanPairingInfoInit(struct ADAPTER *prAdapter,
	struct PAIRING_INFO *prPairingInfo,
	uint8_t *pucPeerAddr);

struct PAIRING_INFO *
nanPairingInfoAlloc(struct ADAPTER *prAdapter,
	uint8_t *pucPeerAddr);

void
nanPairingInfoRelease(struct ADAPTER *prAdapter,
	struct PAIRING_INFO *prPairingInfo);
#endif /* CFG_SUPPORT_NAN_R4_VENDOR_PAIRING */
#endif /* CFG_SUPPORT_NAN */
#endif /* _NAN_PAIRING_H_ */
