/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef SHA384_I_H
#define SHA384_I_H

#include "sha512_i.h"

#define SHA384_BLOCK_SIZE SHA512_BLOCK_SIZE

#define sha384_state sha512_state

void sha384_init(struct sha384_state *md);
int sha384_process(struct sha384_state *md, const unsigned char *in,
		   unsigned long inlen);
int sha384_done(struct sha384_state *md, unsigned char *out);

#endif /* SHA384_I_H */
