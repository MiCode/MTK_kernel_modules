/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "wpa_supp/FourWayHandShake.h"

#include "wpa_supp/src/crypto/aes.h"
#include "wpa_supp/src/crypto/aes_wrap.h"
#include "wpa_supp/src/utils/common.h"

/**
 * aes_unwrap - Unwrap key with AES Key Wrap Algorithm (RFC3394)
 * @kek: Key encryption key (KEK)
 * @kek_len: Length of KEK in octets
 * @n: Length of the plaintext key in 64-bit units; e.g., 2 = 128-bit = 16
 * bytes
 * @cipher: Wrapped key to be unwrapped, (n + 1) * 64 bits
 * @plain: Plaintext key, n * 64 bits
 * Returns: 0 on success, -1 on failure (e.g., integrity verification failed)
 */
int
aes_unwrap_supp(const u8 *kek, size_t kek_len, int n, const u8 *cipher,
		u8 *plain) {
	u8 a[8], *r, b[AES_BLOCK_SIZE];
	int i, j;
	void *ctx;
	unsigned int t;

	/* 1) Initialize variables. */
	os_memcpy(a, cipher, 8);
	r = plain;
	os_memcpy(r, cipher + 8, 8 * n);

	ctx = aes_decrypt_init(kek, kek_len);
	if (ctx == NULL)
		return -1;

	/* 2) Compute intermediate values.
	 * For j = 5 to 0
	 *     For i = n to 1
	 *         B = AES-1(K, (A ^ t) | R[i]) where t = n*j+i
	 *         A = MSB(64, B)
	 *         R[i] = LSB(64, B)
	 */
	for (j = 5; j >= 0; j--) {
		r = plain + (n - 1) * 8;
		for (i = n; i >= 1; i--) {
			os_memcpy(b, a, 8);
			t = n * j + i;
			b[7] ^= t;
			b[6] ^= t >> 8;
			b[5] ^= t >> 16;
			b[4] ^= t >> 24;

			os_memcpy(b + 8, r, 8);
			aes_decrypt(ctx, b, b);
			os_memcpy(a, b, 8);
			os_memcpy(r, b + 8, 8);
			r -= 8;
		}
	}
	aes_decrypt_deinit(ctx);

	/* 3) Output results.
	 *
	 * These are already in @plain due to the location of temporary
	 * variables. Just verify that the IV matches with the expected value.
	 */
	for (i = 0; i < 8; i++) {
		if (a[i] != 0xa6)
			return -1;
	}

	return 0;
}
