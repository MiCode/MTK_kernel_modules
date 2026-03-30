/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2006 MediaTek Inc.
 */


#ifndef AES_H
#define AES_H

#define AES_BLOCK_SIZE 16

void *aes_encrypt_init_wpa(const u8 *key, size_t len);
void aes_encrypt_wpa(void *ctx, const u8 *plain, u8 *crypt);
void aes_encrypt_deinit_wpa(void *ctx);
void *aes_decrypt_init(const u8 *key, size_t len);
void aes_decrypt(void *ctx, const u8 *crypt, u8 *plain);
void aes_decrypt_deinit(void *ctx);

#endif /* AES_H */
