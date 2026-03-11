/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef SHA512_I_H
#define SHA512_I_H

#define SHA512_BLOCK_SIZE 128

struct sha512_state {
	u64 length, state[8];
	u32 curlen;
	u8 buf[SHA512_BLOCK_SIZE];
};

void sha512_init(struct sha512_state *md);
int sha512_process(struct sha512_state *md, const unsigned char *in,
		   unsigned long inlen);
int sha512_done(struct sha512_state *md, unsigned char *out);

#endif /* SHA512_I_H */
