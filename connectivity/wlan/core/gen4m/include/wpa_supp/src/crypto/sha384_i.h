/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*
 * SHA-384 internal definitions
 * Copyright (c) 2015, Pali Rohár <pali.rohar@gmail.com>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef SHA384_I_H
#define SHA384_I_H

#include "sha512_i.h"

#define NAN_RDF_SHA384_BLOCK_SIZE SHA512_BLOCK_SIZE

#define sha384_state nan_rdf_sha512_state

void sha384_init(struct sha384_state *md);
int sha384_process(struct sha384_state *md, const unsigned char *in,
		   unsigned long inlen);
int sha384_done(struct sha384_state *md, unsigned char *out);

#endif /* SHA384_I_H */
