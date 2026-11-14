// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "gcm.c"
 *  \brief
 */

#include "precomp.h"

/* AES definition & structure */
#define AES_STATE_ROWS 4     /* Block size: 4*4*8 = 128 bits */
#define AES_STATE_COLUMNS 4
#define AES_BLOCK_SIZES (AES_STATE_ROWS*AES_STATE_COLUMNS)
#define AES_KEY_ROWS 4
#define AES_KEY_COLUMNS 8    /*Key length: 4*{4,6,8}*8 = 128, 192, 256 bits */
#define AES_KEY128_LENGTH 16
#define AES_KEY192_LENGTH 24
#define AES_KEY256_LENGTH 32
#define AES_CBC_IV_LENGTH 16
#define AES_KEY_COLUMNS1 (AES_KEY_ROWS*((AES_KEY256_LENGTH >> 2) + 6 + 1))

#define AES_BLOCK_SIZE AES_BLOCK_SIZES

struct AES_CTX_STRUCT {
	uint8_t State[AES_STATE_ROWS][AES_STATE_COLUMNS];
	uint8_t KeyWordExpansion[AES_KEY_ROWS][AES_KEY_COLUMNS1];
};

/* ArrayIndex*{02} */
static const uint8_t aes_mul_2[] = {
	/*  0     1     2     3     4     5     6     7     8     9
	 *  a     b     c     d     e     f
	 */
	0x00, 0x02, 0x04, 0x06, 0x08, 0x0a, 0x0c, 0x0e, 0x10, 0x12,
	0x14, 0x16, 0x18, 0x1a, 0x1c, 0x1e, /* 0 */
	0x20, 0x22, 0x24, 0x26, 0x28, 0x2a, 0x2c, 0x2e, 0x30, 0x32,
	0x34, 0x36, 0x38, 0x3a, 0x3c, 0x3e, /* 1 */
	0x40, 0x42, 0x44, 0x46, 0x48, 0x4a, 0x4c, 0x4e, 0x50, 0x52,
	0x54, 0x56, 0x58, 0x5a, 0x5c, 0x5e, /* 2 */
	0x60, 0x62, 0x64, 0x66, 0x68, 0x6a, 0x6c, 0x6e, 0x70, 0x72,
	0x74, 0x76, 0x78, 0x7a, 0x7c, 0x7e, /* 3 */
	0x80, 0x82, 0x84, 0x86, 0x88, 0x8a, 0x8c, 0x8e, 0x90, 0x92,
	0x94, 0x96, 0x98, 0x9a, 0x9c, 0x9e, /* 4 */
	0xa0, 0xa2, 0xa4, 0xa6, 0xa8, 0xaa, 0xac, 0xae, 0xb0, 0xb2,
	0xb4, 0xb6, 0xb8, 0xba, 0xbc, 0xbe, /* 5 */
	0xc0, 0xc2, 0xc4, 0xc6, 0xc8, 0xca, 0xcc, 0xce, 0xd0, 0xd2,
	0xd4, 0xd6, 0xd8, 0xda, 0xdc, 0xde, /* 6 */
	0xe0, 0xe2, 0xe4, 0xe6, 0xe8, 0xea, 0xec, 0xee, 0xf0, 0xf2,
	0xf4, 0xf6, 0xf8, 0xfa, 0xfc, 0xfe, /* 7 */
	0x1b, 0x19, 0x1f, 0x1d, 0x13, 0x11, 0x17, 0x15, 0x0b, 0x09,
	0x0f, 0x0d, 0x03, 0x01, 0x07, 0x05, /* 8 */
	0x3b, 0x39, 0x3f, 0x3d, 0x33, 0x31, 0x37, 0x35, 0x2b, 0x29,
	0x2f, 0x2d, 0x23, 0x21, 0x27, 0x25, /* 9 */
	0x5b, 0x59, 0x5f, 0x5d, 0x53, 0x51, 0x57, 0x55, 0x4b, 0x49,
	0x4f, 0x4d, 0x43, 0x41, 0x47, 0x45, /* a */
	0x7b, 0x79, 0x7f, 0x7d, 0x73, 0x71, 0x77, 0x75, 0x6b, 0x69,
	0x6f, 0x6d, 0x63, 0x61, 0x67, 0x65, /* b */
	0x9b, 0x99, 0x9f, 0x9d, 0x93, 0x91, 0x97, 0x95, 0x8b, 0x89,
	0x8f, 0x8d, 0x83, 0x81, 0x87, 0x85, /* c */
	0xbb, 0xb9, 0xbf, 0xbd, 0xb3, 0xb1, 0xb7, 0xb5, 0xab, 0xa9,
	0xaf, 0xad, 0xa3, 0xa1, 0xa7, 0xa5, /* d */
	0xdb, 0xd9, 0xdf, 0xdd, 0xd3, 0xd1, 0xd7, 0xd5, 0xcb, 0xc9,
	0xcf, 0xcd, 0xc3, 0xc1, 0xc7, 0xc5, /* e */
	0xfb, 0xf9, 0xff, 0xfd, 0xf3, 0xf1, 0xf7, 0xf5, 0xeb, 0xe9,
	0xef, 0xed, 0xe3, 0xe1, 0xe7, 0xe5, /* f */
};

/* ArrayIndex*{03} */
static const uint8_t aes_mul_3[] = {
	/*  0     1     2     3     4     5     6     7     8     9
	 *  a     b     c     d     e     f
	 */
	0x00, 0x03, 0x06, 0x05, 0x0c, 0x0f, 0x0a, 0x09, 0x18, 0x1b,
	0x1e, 0x1d, 0x14, 0x17, 0x12, 0x11, /* 0 */
	0x30, 0x33, 0x36, 0x35, 0x3c, 0x3f, 0x3a, 0x39, 0x28, 0x2b,
	0x2e, 0x2d, 0x24, 0x27, 0x22, 0x21, /* 1 */
	0x60, 0x63, 0x66, 0x65, 0x6c, 0x6f, 0x6a, 0x69, 0x78, 0x7b,
	0x7e, 0x7d, 0x74, 0x77, 0x72, 0x71, /* 2 */
	0x50, 0x53, 0x56, 0x55, 0x5c, 0x5f, 0x5a, 0x59, 0x48, 0x4b,
	0x4e, 0x4d, 0x44, 0x47, 0x42, 0x41, /* 3 */
	0xc0, 0xc3, 0xc6, 0xc5, 0xcc, 0xcf, 0xca, 0xc9, 0xd8, 0xdb,
	0xde, 0xdd, 0xd4, 0xd7, 0xd2, 0xd1, /* 4 */
	0xf0, 0xf3, 0xf6, 0xf5, 0xfc, 0xff, 0xfa, 0xf9, 0xe8, 0xeb,
	0xee, 0xed, 0xe4, 0xe7, 0xe2, 0xe1, /* 5 */
	0xa0, 0xa3, 0xa6, 0xa5, 0xac, 0xaf, 0xaa, 0xa9, 0xb8, 0xbb,
	0xbe, 0xbd, 0xb4, 0xb7, 0xb2, 0xb1, /* 6 */
	0x90, 0x93, 0x96, 0x95, 0x9c, 0x9f, 0x9a, 0x99, 0x88, 0x8b,
	0x8e, 0x8d, 0x84, 0x87, 0x82, 0x81, /* 7 */
	0x9b, 0x98, 0x9d, 0x9e, 0x97, 0x94, 0x91, 0x92, 0x83, 0x80,
	0x85, 0x86, 0x8f, 0x8c, 0x89, 0x8a, /* 8 */
	0xab, 0xa8, 0xad, 0xae, 0xa7, 0xa4, 0xa1, 0xa2, 0xb3, 0xb0,
	0xb5, 0xb6, 0xbf, 0xbc, 0xb9, 0xba, /* 9 */
	0xfb, 0xf8, 0xfd, 0xfe, 0xf7, 0xf4, 0xf1, 0xf2, 0xe3, 0xe0,
	0xe5, 0xe6, 0xef, 0xec, 0xe9, 0xea, /* a */
	0xcb, 0xc8, 0xcd, 0xce, 0xc7, 0xc4, 0xc1, 0xc2, 0xd3, 0xd0,
	0xd5, 0xd6, 0xdf, 0xdc, 0xd9, 0xda, /* b */
	0x5b, 0x58, 0x5d, 0x5e, 0x57, 0x54, 0x51, 0x52, 0x43, 0x40,
	0x45, 0x46, 0x4f, 0x4c, 0x49, 0x4a, /* c */
	0x6b, 0x68, 0x6d, 0x6e, 0x67, 0x64, 0x61, 0x62, 0x73, 0x70,
	0x75, 0x76, 0x7f, 0x7c, 0x79, 0x7a, /* d */
	0x3b, 0x38, 0x3d, 0x3e, 0x37, 0x34, 0x31, 0x32, 0x23, 0x20,
	0x25, 0x26, 0x2f, 0x2c, 0x29, 0x2a, /* e */
	0x0b, 0x08, 0x0d, 0x0e, 0x07, 0x04, 0x01, 0x02, 0x13, 0x10,
	0x15, 0x16, 0x1f, 0x1c, 0x19, 0x1a, /* f */
};

/* The value given by [x^(i-1),{00},{00},{00}],
 * with x^(i-1) being powers of x in the field GF(2^8).
 */
const uint32_t aes_rcon[] = {
	0x00000000, 0x01000000, 0x02000000, 0x04000000,
	0x08000000, 0x10000000, 0x20000000, 0x40000000,
	0x80000000, 0x1B000000, 0x36000000
};

const uint8_t aes_sbox_enc[] = {
	/*  0     1     2     3     4     5     6     7     8     9
	 *  a     b     c     d     e     f
	 */
	0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01,
	0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76, /* 0 */
	0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4,
	0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0, /* 1 */
	0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5,
	0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15, /* 2 */
	0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12,
	0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75, /* 3 */
	0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b,
	0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84, /* 4 */
	0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb,
	0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf, /* 5 */
	0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9,
	0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8, /* 6 */
	0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6,
	0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2, /* 7 */
	0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7,
	0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73, /* 8 */
	0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee,
	0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb, /* 9 */
	0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3,
	0xac, 0x62, 0x91, 0x95, 0xe4, 0x79, /* a */
	0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56,
	0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08, /* b */
	0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd,
	0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a, /* c */
	0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35,
	0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e, /* d */
	0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e,
	0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf, /* e */
	0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99,
	0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16, /* f */
};

/*
 *  Routine Description:
 *    AES key expansion (key schedule)
 *
 *  Arguments:
 *    Key              Cipher key, it may be 16, 24,
 *                     or 32 bytes (128, 192, or 256 bits)
 *    KeyLength        The length of cipher key in bytes
 *    paes_ctx         Pointer to AES_CTX_STRUCT
 *
 *  Return Value:
 *    paes_ctx         Retrun the KeyWordExpansion of AES_CTX_STRUCT
 *
 *  Note:
 *    Pseudo code for key expansion
 *  ------------------------------------------
 *      Nk = (key length/4);
 *
 *      while (i < Nk)
 *	   KeyWordExpansion[i] = word(key[4*i], key[4*i + 1],
 *                   key[4*i + 2], key[4*i + 3]);
 *	   i++;
 *       end while
 *
 *       while (i < ((key length/4 + 6 + 1)*4) )
 *	   temp = KeyWordExpansion[i - 1];
 *	   if (i % Nk ==0)
 *	       temp = SubWord(RotWord(temp)) ^ Rcon[i/Nk];
 *	   else if ((Nk > 6) && (i % 4 == 4))
 *	       temp = SubWord(temp);
 *	   end if
 *
 *	   KeyWordExpansion[i] = KeyWordExpansion[i - Nk]^ temp;
 *	   i++;
 *      end while
 */
void AES_KeyExpansion(
	uint8_t Key[],
	uint32_t KeyLength,
	struct AES_CTX_STRUCT *paes_ctx)
{
	uint32_t KeyIndex = 0;
	uint32_t NumberOfWordOfKey, NumberOfWordOfKeyExpansion;
	uint8_t  TempWord[AES_KEY_ROWS], Temp;
	uint32_t Temprcon;

	NumberOfWordOfKey = KeyLength >> 2;

	while (KeyIndex < NumberOfWordOfKey) {
		paes_ctx->KeyWordExpansion[0][KeyIndex] =
			Key[4 * KeyIndex];
		paes_ctx->KeyWordExpansion[1][KeyIndex] =
			Key[4 * KeyIndex + 1];
		paes_ctx->KeyWordExpansion[2][KeyIndex] =
			Key[4 * KeyIndex + 2];
		paes_ctx->KeyWordExpansion[3][KeyIndex] =
			Key[4 * KeyIndex + 3];
		KeyIndex++;
	}

	NumberOfWordOfKeyExpansion =
		((uint32_t) AES_KEY_ROWS) * ((KeyLength >> 2) + 6 + 1);

	while (KeyIndex < NumberOfWordOfKeyExpansion && KeyIndex > 0) {
		TempWord[0] = paes_ctx->KeyWordExpansion[0][KeyIndex - 1];
		TempWord[1] = paes_ctx->KeyWordExpansion[1][KeyIndex - 1];
		TempWord[2] = paes_ctx->KeyWordExpansion[2][KeyIndex - 1];
		TempWord[3] = paes_ctx->KeyWordExpansion[3][KeyIndex - 1];

		if ((KeyIndex % NumberOfWordOfKey) == 0) {
			Temprcon = aes_rcon[KeyIndex / NumberOfWordOfKey];
			Temp = aes_sbox_enc[TempWord[1]]
				^ ((Temprcon >> 24) & 0xff);
			TempWord[1] = aes_sbox_enc[TempWord[2]]
				^ ((Temprcon >> 16) & 0xff);
			TempWord[2] = aes_sbox_enc[TempWord[3]]
				^ ((Temprcon >>  8) & 0xff);
			TempWord[3] = aes_sbox_enc[TempWord[0]]
				^ ((Temprcon) & 0xff);
			TempWord[0] = Temp;
		} else if ((NumberOfWordOfKey > 6) &&
			((KeyIndex % NumberOfWordOfKey) == 4)) {
			Temp = aes_sbox_enc[TempWord[0]];
			TempWord[1] = aes_sbox_enc[TempWord[1]];
			TempWord[2] = aes_sbox_enc[TempWord[2]];
			TempWord[3] = aes_sbox_enc[TempWord[3]];
			TempWord[0] = Temp;
		}

		paes_ctx->KeyWordExpansion[0][KeyIndex] =
			paes_ctx->KeyWordExpansion[
			0][KeyIndex - NumberOfWordOfKey] ^ TempWord[0];
		paes_ctx->KeyWordExpansion[1][KeyIndex] =
			paes_ctx->KeyWordExpansion[
			1][KeyIndex - NumberOfWordOfKey] ^ TempWord[1];
		paes_ctx->KeyWordExpansion[2][KeyIndex] =
			paes_ctx->KeyWordExpansion[
			2][KeyIndex - NumberOfWordOfKey] ^ TempWord[2];
		paes_ctx->KeyWordExpansion[3][KeyIndex] =
			paes_ctx->KeyWordExpansion[
			3][KeyIndex - NumberOfWordOfKey] ^ TempWord[3];
		KeyIndex++;
	}
}

/*
 * Routine Description:
 *   AES encryption
 *
 * Arguments:
 *   PlainBlock       The block of plain text, 16 bytes(128 bits) each block
 *   PlainBlockSize   The length of block of plain text in bytes
 *   Key              Cipher key, it may be 16, 24, or 32 bytes
 *                    (128, 192, or 256 bits)
 *   KeyLength        The length of cipher key in bytes
 *   CipherBlockSize  The length of allocated cipher block in bytes
 *
 * Return Value:
 *   CipherBlock      Return cipher text
 *   CipherBlockSize  Return the length of real used cipher block in bytes
 *
 * Note:
 *   Reference to FIPS-PUB 197
 *   1. Check if block size is 16 bytes(128 bits) and if key length
 *      is 16, 24, or 32 bytes(128, 192, or 256 bits)
 *   2. Transfer the plain block to state block
 *   3. Main encryption rounds
 *   4. Transfer the state block to cipher block
 *   ------------------------------------------
 *      NumberOfRound = (key length / 4) + 6;
 *      state block = plain block;
 *
 *      AddRoundKey(state block, key);
 *      for round = 1 to NumberOfRound
 *	   SubBytes(state block)
 *	   ShiftRows(state block)
 *	   MixColumns(state block)
 *	   AddRoundKey(state block, key);
 *      end for
 *
 *      SubBytes(state block)
 *      ShiftRows(state block)
 *      AddRoundKey(state block, key);
 *
 *      cipher block = state block;
 */
void AES_Encrypt(
	uint8_t PlainBlock[],
	uint32_t PlainBlockSize,
	uint8_t Key[],
	uint32_t KeyLength,
	uint8_t CipherBlock[],
	uint32_t *CipherBlockSize)
{
	/*    AES_CTX_STRUCT aes_ctx;
	*/
	struct AES_CTX_STRUCT *paes_ctx = NULL;
	uint32_t RowIndex, ColumnIndex;
	uint32_t RoundIndex, NumberOfRound = 0;
	uint8_t Temp, Row0, Row1, Row2, Row3;
	uint8_t AesCtxBuffer[sizeof(struct AES_CTX_STRUCT)];

	/*
	 * 1. Check if block size is 16 bytes(128 bits) and if key length is
	 *    16, 24, or 32 bytes(128, 192, or 256 bits)
	 */
	if (PlainBlockSize != AES_BLOCK_SIZES) {
		DBGLOG(AIS, ERROR,
			"AES_Encrypt: plain block size is %d bytes, it must be %d bytes(128 bits).\n",
		PlainBlockSize, AES_BLOCK_SIZES);
		return;
	}

	if ((KeyLength != AES_KEY128_LENGTH) && (KeyLength != AES_KEY192_LENGTH)
		&& (KeyLength != AES_KEY256_LENGTH)) {
		DBGLOG(AIS, ERROR,
			"AES_Encrypt: key length is %d bytes, it must be %d, %d, or %d bytes(128, 192, or 256 bits).\n",
		KeyLength, AES_KEY128_LENGTH,
		AES_KEY192_LENGTH, AES_KEY256_LENGTH);
		return;
	}

	if (*CipherBlockSize < AES_BLOCK_SIZES) {
		DBGLOG(AIS, ERROR,
			"AES_Encrypt: cipher block size is %d bytes, it must be %d bytes(128 bits).\n",
			*CipherBlockSize, AES_BLOCK_SIZES);
		return;
	}

	paes_ctx = (struct AES_CTX_STRUCT *)AesCtxBuffer;

	/*
	 * 2. Transfer the plain block to state block
	 */
	for (RowIndex = 0; RowIndex < AES_STATE_ROWS; RowIndex++)
	for (ColumnIndex = 0; ColumnIndex < AES_STATE_COLUMNS;
		ColumnIndex++)
		paes_ctx->State[RowIndex][ColumnIndex] =
			PlainBlock[RowIndex + 4 * ColumnIndex];

	/*
	 *  3. Main encryption rounds
	 */
	AES_KeyExpansion(Key, KeyLength, paes_ctx);
	NumberOfRound = (KeyLength >> 2) + 6;
	/* AES_AddRoundKey */
	RoundIndex = 0;

	for (RowIndex = 0; RowIndex < AES_STATE_ROWS; RowIndex++)
	for (ColumnIndex = 0; ColumnIndex < AES_STATE_COLUMNS; ColumnIndex++)
		paes_ctx->State[RowIndex][ColumnIndex] ^=
			paes_ctx->KeyWordExpansion[
		RowIndex][(RoundIndex * ((uint32_t) AES_STATE_COLUMNS))
			+ ColumnIndex];

	for (RoundIndex = 1; RoundIndex < NumberOfRound; RoundIndex++) {
		/* AES_SubBytes */
		for (RowIndex = 0; RowIndex < AES_STATE_ROWS;
			RowIndex++)
		for (ColumnIndex = 0; ColumnIndex < AES_STATE_COLUMNS;
			ColumnIndex++)
		paes_ctx->State[RowIndex][ColumnIndex] =
		aes_sbox_enc[paes_ctx->State[RowIndex][ColumnIndex]];

		/* AES_ShiftRows */
		Temp = paes_ctx->State[1][0];
		paes_ctx->State[1][0] = paes_ctx->State[1][1];
		paes_ctx->State[1][1] = paes_ctx->State[1][2];
		paes_ctx->State[1][2] = paes_ctx->State[1][3];
		paes_ctx->State[1][3] = Temp;
		Temp = paes_ctx->State[2][0];
		paes_ctx->State[2][0] = paes_ctx->State[2][2];
		paes_ctx->State[2][2] = Temp;
		Temp = paes_ctx->State[2][1];
		paes_ctx->State[2][1] = paes_ctx->State[2][3];
		paes_ctx->State[2][3] = Temp;
		Temp = paes_ctx->State[3][3];
		paes_ctx->State[3][3] = paes_ctx->State[3][2];
		paes_ctx->State[3][2] = paes_ctx->State[3][1];
		paes_ctx->State[3][1] = paes_ctx->State[3][0];
		paes_ctx->State[3][0] = Temp;

		/* AES_MixColumns */
		for (ColumnIndex = 0; ColumnIndex < AES_STATE_COLUMNS;
			ColumnIndex++) {
		Row0 = paes_ctx->State[0][ColumnIndex];
		Row1 = paes_ctx->State[1][ColumnIndex];
		Row2 = paes_ctx->State[2][ColumnIndex];
		Row3 = paes_ctx->State[3][ColumnIndex];
		paes_ctx->State[0][ColumnIndex] =
			aes_mul_2[Row0] ^ aes_mul_3[Row1] ^ Row2 ^ Row3;
		paes_ctx->State[1][ColumnIndex] =
			Row0 ^ aes_mul_2[Row1] ^ aes_mul_3[Row2] ^ Row3;
		paes_ctx->State[2][ColumnIndex] =
			Row0 ^ Row1 ^ aes_mul_2[Row2] ^ aes_mul_3[Row3];
		paes_ctx->State[3][ColumnIndex] =
			aes_mul_3[Row0] ^ Row1 ^ Row2 ^ aes_mul_2[Row3];
		}

		/* AES_AddRoundKey */
		for (RowIndex = 0; RowIndex < AES_STATE_ROWS; RowIndex++)
		for (ColumnIndex = 0; ColumnIndex < AES_STATE_COLUMNS;
			ColumnIndex++)
		paes_ctx->State[RowIndex][ColumnIndex] ^=
			paes_ctx->KeyWordExpansion[
		RowIndex][(RoundIndex * ((uint32_t) AES_STATE_COLUMNS))
			+ ColumnIndex];
	} /* End of loop for RoundIndex */

	/* AES_SubBytes */
	for (RowIndex = 0; RowIndex < AES_STATE_ROWS; RowIndex++)
	for (ColumnIndex = 0; ColumnIndex < AES_STATE_COLUMNS; ColumnIndex++)
		paes_ctx->State[RowIndex][ColumnIndex] =
		aes_sbox_enc[paes_ctx->State[RowIndex][ColumnIndex]];

	/* AES_ShiftRows */
	Temp = paes_ctx->State[1][0];
	paes_ctx->State[1][0] = paes_ctx->State[1][1];
	paes_ctx->State[1][1] = paes_ctx->State[1][2];
	paes_ctx->State[1][2] = paes_ctx->State[1][3];
	paes_ctx->State[1][3] = Temp;
	Temp = paes_ctx->State[2][0];
	paes_ctx->State[2][0] = paes_ctx->State[2][2];
	paes_ctx->State[2][2] = Temp;
	Temp = paes_ctx->State[2][1];
	paes_ctx->State[2][1] = paes_ctx->State[2][3];
	paes_ctx->State[2][3] = Temp;
	Temp = paes_ctx->State[3][3];
	paes_ctx->State[3][3] = paes_ctx->State[3][2];
	paes_ctx->State[3][2] = paes_ctx->State[3][1];
	paes_ctx->State[3][1] = paes_ctx->State[3][0];
	paes_ctx->State[3][0] = Temp;

	/* AES_AddRoundKey */
	for (RowIndex = 0; RowIndex < AES_STATE_ROWS; RowIndex++)
	for (ColumnIndex = 0; ColumnIndex < AES_STATE_COLUMNS; ColumnIndex++)
		paes_ctx->State[RowIndex][ColumnIndex] ^=
			paes_ctx->KeyWordExpansion[
		RowIndex][(RoundIndex * ((uint32_t) AES_STATE_COLUMNS))
			+ ColumnIndex];

	/*
	 * 4. Transfer the state block to cipher block
	 */
	for (RowIndex = 0; RowIndex < AES_STATE_ROWS; RowIndex++)
	for (ColumnIndex = 0; ColumnIndex < AES_STATE_COLUMNS; ColumnIndex++)
		CipherBlock[RowIndex + 4 * ColumnIndex] =
		paes_ctx->State[RowIndex][ColumnIndex];

	*CipherBlockSize = ((uint32_t) AES_STATE_ROWS) *
		((uint32_t) AES_STATE_COLUMNS);
}

static inline uint32_t get_be32(const uint8_t *a)
{
	return ((uint32_t) a[0] << 24) | (a[1] << 16) | (a[2] << 8) | a[3];
}

static inline void put_be32(uint8_t *a, uint32_t val)
{
	a[0] = (val >> 24) & 0xff;
	a[1] = (val >> 16) & 0xff;
	a[2] = (val >> 8) & 0xff;
	a[3] = val & 0xff;
}

static inline uint64_t get_be64(const uint8_t *a)
{
	return (((uint64_t) a[0]) << 56) | (((uint64_t) a[1]) << 48) |
		(((uint64_t) a[2]) << 40) | (((uint64_t) a[3]) << 32) |
		(((uint64_t) a[4]) << 24) | (((uint64_t) a[5]) << 16) |
		(((uint64_t) a[6]) << 8) | ((uint64_t) a[7]);
}

static inline void put_be64(uint8_t *a, uint64_t val)
{
	a[0] = val >> 56;
	a[1] = val >> 48;
	a[2] = val >> 40;
	a[3] = val >> 32;
	a[4] = val >> 24;
	a[5] = val >> 16;
	a[6] = val >> 8;
	a[7] = val & 0xff;
}

static void inc32(uint8_t *block)
{
	uint32_t val;

	val = get_be32(block + AES_BLOCK_SIZE - 4);
	val++;
	put_be32(block + AES_BLOCK_SIZE - 4, val);
}


static void xor_block(uint8_t *dst, const uint8_t *src)
{
	uint32_t *d = (uint32_t *) dst;
	uint32_t *s = (uint32_t *) src;

	*d++ ^= *s++;
	*d++ ^= *s++;
	*d++ ^= *s++;
	*d++ ^= *s++;
}


static void shift_right_block(uint8_t *v)
{
	uint32_t val;

	val = get_be32(v + 12);
	val >>= 1;
	if (v[11] & 0x01)
		val |= 0x80000000;
	put_be32(v + 12, val);

	val = get_be32(v + 8);
	val >>= 1;
	if (v[7] & 0x01)
		val |= 0x80000000;
	put_be32(v + 8, val);

	val = get_be32(v + 4);
	val >>= 1;
	if (v[3] & 0x01)
		val |= 0x80000000;
	put_be32(v + 4, val);

	val = get_be32(v);
	val >>= 1;
	put_be32(v, val);
}


/* Multiplication in GF(2^128) */
static void gf_mult(const uint8_t *x, const uint8_t *y, uint8_t *z)
{
	uint8_t v[16];
	int i, j;

	memset(z, 0, 16); /* Z_0 = 0^128 */
	memcpy(v, y, 16); /* V_0 = Y */

	for (i = 0; i < 16; i++) {
		for (j = 0; j < 8; j++) {
			if (x[i] & BIT(7 - j)) {
				/* Z_(i + 1) = Z_i XOR V_i */
				xor_block(z, v);
			} else {
				/* Z_(i + 1) = Z_i */
			}

			if (v[15] & 0x01) {
				/* V_(i + 1) = (V_i >> 1) XOR R */
				shift_right_block(v);
				/* R = 11100001 || 0^120 */
				v[0] ^= 0xe1;
			} else {
				/* V_(i + 1) = V_i >> 1 */
				shift_right_block(v);
			}
		}
	}
}

static void ghash(const uint8_t *h, const uint8_t *x, size_t xlen, uint8_t *y)
{
	size_t m, i;
	const uint8_t *xpos = x;
	uint8_t tmp[16];

	m = xlen / 16;

	for (i = 0; i < m; i++) {
		/* Y_i = (Y^(i-1) XOR X_i) dot H */
		xor_block(y, xpos);
		xpos += 16;

		/* dot operation:
		 * multiplication operation for binary Galois (finite) field of
		 * 2^128 elements
		 */
		gf_mult(y, h, tmp);
		memcpy(y, tmp, 16);
	}

	if (x + xlen > xpos) {
		/* Add zero padded last block */
		size_t last = x + xlen - xpos;

		memcpy(tmp, xpos, last);
		memset(tmp + last, 0, sizeof(tmp) - last);

		/* Y_i = (Y^(i-1) XOR X_i) dot H */
		xor_block(y, tmp);

		/* dot operation:
		 * multiplication operation for binary Galois (finite) field of
		 * 2^128 elements
		 */
		gf_mult(y, h, tmp);
		memcpy(y, tmp, 16);
	}

	/* Return Y_m */
}

static void aes_gctr(
	uint8_t *key,
	uint32_t key_len,
	const uint8_t *icb,
	const uint8_t *x,
	uint32_t xlen,
	uint8_t *y)
{
	size_t i, n, last;
	uint8_t cb[AES_BLOCK_SIZE], tmp[AES_BLOCK_SIZE];
	const uint8_t *xpos = x;
	uint8_t *ypos = y;
	uint32_t y_size = AES_BLOCK_SIZES;
	uint32_t tmp_size = AES_BLOCK_SIZES;

	if (xlen == 0)
		return;

	n = xlen / 16;

	memcpy(cb, icb, AES_BLOCK_SIZE);
	/* Full blocks */
	for (i = 0; i < n; i++) {
		/* aes_encrypt(aes, cb, ypos); */
		AES_Encrypt(cb, AES_BLOCK_SIZES,
			key, key_len, ypos, &y_size);
		xor_block(ypos, xpos);
		xpos += AES_BLOCK_SIZE;
		ypos += AES_BLOCK_SIZE;
		inc32(cb);
	}

	kalMemSet(tmp, 0, AES_BLOCK_SIZE);
	last = x + xlen - xpos;
	if (last) {
		/* Last, partial block */
		/* aes_encrypt(aes, cb, tmp); */
		AES_Encrypt(cb, AES_BLOCK_SIZES,
			key, key_len, tmp, &tmp_size);
		for (i = 0; i < last; i++)
			*ypos++ = *xpos++ ^ tmp[i];
	}
}

/**
 * aes_gcm_ae - GCM-AE_K(IV, P, A)
 * OUT: tag
 */
int aes_gcm_ae(
	uint8_t *aad,
	uint32_t aad_len,
	uint8_t *key,
	uint32_t key_len,
	uint8_t *nonce,
	uint32_t nonce_len,
	uint8_t *tag)
{
	uint8_t PH[AES_BLOCK_SIZES];
	uint8_t H[AES_BLOCK_SIZE];
	uint8_t J0[AES_BLOCK_SIZE];
	uint8_t S[16];
	uint8_t len_buf[16];
	uint32_t h_size = AES_BLOCK_SIZES;

	/* Generate hash subkey H = AES_K(0^128) */
	kalMemSet(PH, 0, AES_BLOCK_SIZE);
	kalMemSet(H, 0, AES_BLOCK_SIZE);
	AES_Encrypt(PH, AES_BLOCK_SIZES, key, key_len, H, &h_size);

	/* if (iv_len == 12) */
	/* Prepare block J_0 = IV || 0^31 || 1 [len(IV) = 96] */
	memmove(J0, nonce, nonce_len);
	kalMemSet(J0 + nonce_len, 0,  AES_BLOCK_SIZES - nonce_len);
	J0[AES_BLOCK_SIZES - 1] = 0x01;

	/* C = GCTR_K(inc_32(J_0), P) */
	kalMemSet(S, 0, 16);
	ghash(H, aad, aad_len, S);
	put_be64(len_buf, aad_len * 8);
	put_be64(len_buf + 8, 0);
	ghash(H, len_buf, sizeof(len_buf), S);
	DBGLOG(AIS, INFO, "S = GHASH_H(...)");
	DBGLOG_MEM8(AIS, INFO, S, AES_BLOCK_SIZE);

	/* T = MSB_t(GCTR_K(J_0, S)) */
	aes_gctr(key, key_len, J0, S, sizeof(S), tag);

	DBGLOG(AIS, INFO, "Show tag");
	DBGLOG_MEM8(AIS, INFO, tag, 16);

	return 0;
}


/**
 * aes_gcm_ad - GCM-AD_K(IV, C, A, T)
 * IN: tag
 * if tag == T, return 0 (check pass)
 * if tag != T, return -1 (check fail)
 */
int aes_gcm_ad_impl(
	uint8_t *aad,
	uint32_t aad_len,
	uint8_t *key,
	uint32_t key_len,
	uint8_t *nonce,
	uint32_t nonce_len,
	uint8_t *tag)
{
	uint8_t PH[AES_BLOCK_SIZES];
	uint8_t H[AES_BLOCK_SIZE];
	uint8_t J0[AES_BLOCK_SIZE];
	uint8_t S[16], T[16];
	uint8_t len_buf[16];
	uint32_t h_size = AES_BLOCK_SIZES;

	kalMemSet(PH, 0, AES_BLOCK_SIZE);
	kalMemSet(H, 0, AES_BLOCK_SIZE);
	AES_Encrypt(PH, AES_BLOCK_SIZES, key, key_len, H, &h_size);

	/* Prepare block J_0 = IV || 0^31 || 1 [len(IV) = 96] */
	memmove(J0, nonce, nonce_len);
	kalMemSet(J0 + nonce_len, 0, AES_BLOCK_SIZES - nonce_len);
	J0[AES_BLOCK_SIZES - 1] = 0x01;

	/* P = GCTR_K(inc_32(J_0), C) */
	kalMemSet(S, 0, 16);
	ghash(H, aad, aad_len, S);
	put_be64(len_buf, aad_len * 8);
	put_be64(len_buf + 8, 0);
	ghash(H, len_buf, sizeof(len_buf), S);
	/* DBGLOG(AIS, INFO, "S = GHASH_H(...)"); */
	/* DBGLOG_MEM8(AIS, INFO, S, AES_BLOCK_SIZE); */

	/* T' = MSB_t(GCTR_K(J_0, S)) */
	aes_gctr(key, key_len, J0, S, sizeof(S), T);
	DBGLOG(AIS, INFO, "Show tag");
	DBGLOG_MEM8(AIS, INFO, T, 16);

	if (kalMemCmp(tag, T, 16) != 0) {
		DBGLOG(AIS, WARN, "GCM: Tag mismatch");
		return -1;
	}

	return 0;
}

int aes_gmac_impl(uint8_t *key, size_t key_len, uint8_t *iv, size_t iv_len,
		uint8_t *aad, size_t aad_len, uint8_t *tag)
{
	return aes_gcm_ae(aad, aad_len, key, key_len, iv, iv_len, tag);
}
