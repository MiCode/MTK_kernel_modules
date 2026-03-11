/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2015 MediaTek Inc.
 */

#ifndef SHA384_H
#define SHA384_H

#define SHA384_MAC_LEN 48

int hmac_sha384_vector(const u8 *key, size_t key_len, size_t num_elem,
		       const u8 *addr[], const size_t *len, u8 *mac);
int hmac_sha384(const u8 *key, size_t key_len, const u8 *data, size_t data_len,
		u8 *mac);
void sha384_prf(const u8 *key, size_t key_len, const char *label,
		const u8 *data, size_t data_len, u8 *buf, size_t buf_len);
void sha384_prf_bits(const u8 *key, size_t key_len, const char *label,
		     const u8 *data, size_t data_len, u8 *buf,
		     size_t buf_len_bits);

#endif /* SHA384_H */
