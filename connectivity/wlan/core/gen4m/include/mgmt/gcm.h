/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "gcm.h"
 *  \brief
 */

#ifndef _GCM_H
#define _GCM_H

int aes_gmac_impl(uint8_t *key, size_t key_len, uint8_t *iv, size_t iv_len,
	uint8_t *aad, size_t aad_len, uint8_t *tag);

int aes_gcm_ad_impl(
	uint8_t *aad,
	uint32_t aad_len,
	uint8_t *key,
	uint32_t key_len,
	uint8_t *nonce,
	uint32_t nonce_len,
	uint8_t *tag);

#endif

