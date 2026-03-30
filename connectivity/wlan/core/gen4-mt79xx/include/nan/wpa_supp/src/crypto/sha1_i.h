/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2005 MediaTek Inc.
 */

#ifndef SHA1_I_H
#define SHA1_I_H

/*#include "../../../../ROM/include/mgmt/crypto_i.h"*/

#if 1
struct SHA1Context {
	u32 state[5];
	u32 count[2];
	unsigned char buffer[64];
};
#endif

void SHA1Init(struct SHA1Context *context);
void SHA1Update(struct SHA1Context *context, const void *data, u32 len);
void SHA1Final(unsigned char digest[20], struct SHA1Context *context);
void SHA1Transform(u32 state[5], const unsigned char buffer[64]);

#endif /* SHA1_I_H */
