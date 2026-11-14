// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "dump.c"
 *    \brief  Provide memory dump function for debugging.
 *
 *    Provide memory dump function for debugging.
 */


/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "precomp.h"

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to dump a segment of memory in bytes with
 *        32 bytes in each line.
 *
 * \param[in] pucStartAddr   Pointer to the starting address of the memory
 *                           to be dumped.
 * \param[in] u2Length       Length of the memory to be dumped.
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void dumpHex(const void *pBuffer, uint16_t u2Length)
{
#if !DBG_DISABLE_ALL_LOG
#define BUFSIZE 100
#define TEXTSIZE 40
	const uint8_t *pucStartAddr = pBuffer;
	uint8_t output[BUFSIZE] = {0};
	uint8_t text[2][TEXTSIZE / 2];
	uint32_t i = 0;
	uint32_t printed = 0;
	uint32_t offset = 0;

	LOG_FUNC("DUMPHEX ADDRESS: 0x%p, Length: %d\n", pucStartAddr, u2Length);

	if (!pBuffer)
		return;

	while (u2Length > 0) {
		kalMemZero(text, sizeof(text));

		for (i = 0, offset = 0; i < 32 && u2Length; i++) {
			offset += snprintf(output + offset, BUFSIZE - offset,
					"%02x %s",
					pucStartAddr[printed + i],
					i + 1 == 16 ? "- " :
					(i + 1) % 16 == 8 ? " " : "");
			text[i / 16][i % 16] =
				kalIsPrint(pucStartAddr[printed + i]) ?
					   pucStartAddr[printed + i] : '.';
			u2Length--;
		}
		LOG_FUNC("%04x: %-100s %s %s",
			 printed, output, text[0], text[1]);
		printed += 32;
	}
#undef BUFSIZE
#undef TEXTSIZE
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to dump a segment of memory in bytes.
 *
 * \param[in] pucStartAddr   Pointer to the starting address of the memory
 *                           to be dumped.
 * \param[in] u4Length       Length of the memory to be dumped.
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void dumpMemory8(uint8_t *pucStartAddr,
		 uint32_t u4Length)
{
	ASSERT(pucStartAddr);

	LOG_FUNC("DUMP8 ADDRESS: 0x%p, Length: %d\n", pucStartAddr,
		 u4Length);

#define case16 "(0x%p) %02x %02x %02x %02x  %02x %02x %02x %02x" \
			" - %02x %02x %02x %02x  %02x %02x %02x %02x\n"

#define case07 "(0x%p) %02x %02x %02x %02x  %02x %02x %02x\n"

#define case08 "(0x%p) %02x %02x %02x %02x  %02x %02x %02x %02x\n"

#define case09 "(0x%p) %02x %02x %02x %02x  %02x %02x %02x %02x" \
			" - %02x\n"

#define case10 "(0x%p) %02x %02x %02x %02x  %02x %02x %02x %02x" \
			" - %02x %02x\n"

#define case11 "(0x%p) %02x %02x %02x %02x  %02x %02x %02x %02x" \
			" - %02x %02x %02x\n"

#define case12 "(0x%p) %02x %02x %02x %02x  %02x %02x %02x %02x" \
			" - %02x %02x %02x %02x\n"

#define case13 "(0x%p) %02x %02x %02x %02x  %02x %02x %02x %02x" \
			" - %02x %02x %02x %02x  %02x\n"

#define case14 "(0x%p) %02x %02x %02x %02x  %02x %02x %02x %02x" \
			" - %02x %02x %02x %02x  %02x %02x\n"

#define case15 "(0x%p) %02x %02x %02x %02x  %02x %02x %02x %02x" \
			" - %02x %02x %02x %02x  %02x %02x %02x\n"

	while (u4Length > 0) {
		if (u4Length >= 16) {
			LOG_FUNC
			(case16,
			 pucStartAddr, pucStartAddr[0], pucStartAddr[1],
			 pucStartAddr[2], pucStartAddr[3], pucStartAddr[4],
			 pucStartAddr[5], pucStartAddr[6], pucStartAddr[7],
			 pucStartAddr[8], pucStartAddr[9], pucStartAddr[10],
			 pucStartAddr[11], pucStartAddr[12],
			 pucStartAddr[13], pucStartAddr[14],
			 pucStartAddr[15]);
			u4Length -= 16;
			pucStartAddr += 16;
		} else {
			switch (u4Length) {
			case 1:
				LOG_FUNC("(0x%p) %02x\n", pucStartAddr,
					 pucStartAddr[0]);
				break;
			case 2:
				LOG_FUNC("(0x%p) %02x %02x\n", pucStartAddr,
					 pucStartAddr[0], pucStartAddr[1]);
				break;
			case 3:
				LOG_FUNC("(0x%p) %02x %02x %02x\n",
					 pucStartAddr, pucStartAddr[0],
					 pucStartAddr[1], pucStartAddr[2]);
				break;
			case 4:
				LOG_FUNC("(0x%p) %02x %02x %02x %02x\n",
					 pucStartAddr,
					 pucStartAddr[0], pucStartAddr[1],
					 pucStartAddr[2], pucStartAddr[3]);
				break;
			case 5:
				LOG_FUNC("(0x%p) %02x %02x %02x %02x  %02x\n",
					 pucStartAddr,
					 pucStartAddr[0], pucStartAddr[1],
					 pucStartAddr[2], pucStartAddr[3],
					 pucStartAddr[4]);
				break;
			case 6:
				LOG_FUNC
				("(0x%p) %02x %02x %02x %02x  %02x %02x\n",
				 pucStartAddr, pucStartAddr[0],
				 pucStartAddr[1], pucStartAddr[2],
				 pucStartAddr[3], pucStartAddr[4],
				 pucStartAddr[5]);
				break;
			case 7:
				LOG_FUNC
				(case07,
				 pucStartAddr, pucStartAddr[0],
				 pucStartAddr[1], pucStartAddr[2],
				 pucStartAddr[3], pucStartAddr[4],
				 pucStartAddr[5], pucStartAddr[6]);
				break;
			case 8:
				LOG_FUNC
				(case08,
				 pucStartAddr, pucStartAddr[0],
				 pucStartAddr[1], pucStartAddr[2],
				 pucStartAddr[3], pucStartAddr[4],
				 pucStartAddr[5], pucStartAddr[6],
				 pucStartAddr[7]);
				break;
			case 9:
				LOG_FUNC
				(case09,
				 pucStartAddr, pucStartAddr[0],
				 pucStartAddr[1], pucStartAddr[2],
				 pucStartAddr[3], pucStartAddr[4],
				 pucStartAddr[5], pucStartAddr[6],
				 pucStartAddr[7], pucStartAddr[8]);
				break;
			case 10:
				LOG_FUNC
				(case10,
				 pucStartAddr, pucStartAddr[0],
				 pucStartAddr[1], pucStartAddr[2],
				 pucStartAddr[3], pucStartAddr[4],
				 pucStartAddr[5], pucStartAddr[6],
				 pucStartAddr[7], pucStartAddr[8],
				 pucStartAddr[9]);
				break;
			case 11:
				LOG_FUNC
				(case11,
				 pucStartAddr, pucStartAddr[0],
				 pucStartAddr[1], pucStartAddr[2],
				 pucStartAddr[3], pucStartAddr[4],
				 pucStartAddr[5], pucStartAddr[6],
				 pucStartAddr[7], pucStartAddr[8],
				 pucStartAddr[9], pucStartAddr[10]);
				break;
			case 12:
				LOG_FUNC
				(case12,
				 pucStartAddr, pucStartAddr[0],
				 pucStartAddr[1], pucStartAddr[2],
				 pucStartAddr[3], pucStartAddr[4],
				 pucStartAddr[5], pucStartAddr[6],
				 pucStartAddr[7], pucStartAddr[8],
				 pucStartAddr[9], pucStartAddr[10],
				 pucStartAddr[11]);
				break;
			case 13:
				LOG_FUNC
				(case13,
				 pucStartAddr, pucStartAddr[0],
				 pucStartAddr[1], pucStartAddr[2],
				 pucStartAddr[3], pucStartAddr[4],
				 pucStartAddr[5], pucStartAddr[6],
				 pucStartAddr[7], pucStartAddr[8],
				 pucStartAddr[9], pucStartAddr[10],
				 pucStartAddr[11], pucStartAddr[12]);
				break;
			case 14:
				LOG_FUNC
				(case14,
				 pucStartAddr, pucStartAddr[0],
				 pucStartAddr[1], pucStartAddr[2],
				 pucStartAddr[3], pucStartAddr[4],
				 pucStartAddr[5], pucStartAddr[6],
				 pucStartAddr[7], pucStartAddr[8],
				 pucStartAddr[9], pucStartAddr[10],
				 pucStartAddr[11], pucStartAddr[12],
				 pucStartAddr[13]);
				break;
			case 15:
			default:
				LOG_FUNC
				(case15,
				 pucStartAddr, pucStartAddr[0],
				 pucStartAddr[1], pucStartAddr[2],
				 pucStartAddr[3], pucStartAddr[4],
				 pucStartAddr[5], pucStartAddr[6],
				 pucStartAddr[7], pucStartAddr[8],
				 pucStartAddr[9], pucStartAddr[10],
				 pucStartAddr[11], pucStartAddr[12],
				 pucStartAddr[13], pucStartAddr[14]);
				break;
			}
			u4Length = 0;
		}
	}
}				/* end of dumpMemory8() */


/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to dump a segment of memory in double words.
 *
 * \param[in] pucStartAddr   Pointer to the starting address of the memory
 *                           to be dumped.
 * \param[in] u4Length       Length of the memory to be dumped.
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void dumpMemory32(uint32_t *pu4StartAddr,
		  uint32_t u4Length)
{
	uint8_t *pucAddr;
#define DM32_CASE15 "(0x%p) %08x %08x %08x --%02x%02x%02x\n"

	ASSERT(pu4StartAddr);

	LOG_FUNC("DUMP32 ADDRESS: 0x%p, Length: %d\n", pu4StartAddr,
		 u4Length);

	if (IS_NOT_ALIGN_4((uintptr_t)pu4StartAddr)) {
		uint32_t u4ProtrudeLen =
			sizeof(uint32_t) - ((uintptr_t)pu4StartAddr % 4);

		u4ProtrudeLen =
			((u4Length < u4ProtrudeLen) ? u4Length : u4ProtrudeLen);
		LOG_FUNC("pu4StartAddr is not at DW boundary.\n");
		pucAddr = (uint8_t *) &pu4StartAddr[0];

		switch (u4ProtrudeLen) {
		case 1:
			LOG_FUNC("(0x%p) %02x------\n", pu4StartAddr,
				pucAddr[0]);
			break;
		case 2:
			LOG_FUNC("(0x%p) %02x%02x----\n", pu4StartAddr,
				 pucAddr[1], pucAddr[0]);
			break;
		case 3:
			LOG_FUNC("(0x%p) %02x%02x%02x--\n", pu4StartAddr,
				 pucAddr[2], pucAddr[1], pucAddr[0]);
			break;
		default:
			break;
		}

		u4Length -= u4ProtrudeLen;
		pu4StartAddr = (uint32_t *)
			       ((uintptr_t)pu4StartAddr + u4ProtrudeLen);
	}

	while (u4Length > 0) {
		if (u4Length >= 16) {
			LOG_FUNC("(0x%p) %08x %08x %08x %08x\n",
				 pu4StartAddr,
				 pu4StartAddr[0], pu4StartAddr[1],
				 pu4StartAddr[2], pu4StartAddr[3]);
			pu4StartAddr += 4;
			u4Length -= 16;
		} else {
			switch (u4Length) {
			case 1:
				pucAddr = (uint8_t *) &pu4StartAddr[0];
				LOG_FUNC("(0x%p) ------%02x\n",
					 pu4StartAddr, pucAddr[0]);
				break;
			case 2:
				pucAddr = (uint8_t *) &pu4StartAddr[0];
				LOG_FUNC("(0x%p) ----%02x%02x\n", pu4StartAddr,
					 pucAddr[1], pucAddr[0]);
				break;
			case 3:
				pucAddr = (uint8_t *) &pu4StartAddr[0];
				LOG_FUNC("(0x%p) --%02x%02x%02x\n",
					pu4StartAddr,
					pucAddr[2], pucAddr[1], pucAddr[0]);
				break;
			case 4:
				LOG_FUNC("(0x%p) %08x\n", pu4StartAddr,
					 pu4StartAddr[0]);
				break;
			case 5:
				pucAddr = (uint8_t *) &pu4StartAddr[1];
				LOG_FUNC("(0x%p) %08x ------%02x\n",
					pu4StartAddr,
					pu4StartAddr[0], pucAddr[0]);
				break;
			case 6:
				pucAddr = (uint8_t *) &pu4StartAddr[1];
				LOG_FUNC("(0x%p) %08x ----%02x%02x\n",
					 pu4StartAddr, pu4StartAddr[0],
					 pucAddr[1], pucAddr[0]);
				break;
			case 7:
				pucAddr = (uint8_t *) &pu4StartAddr[1];
				LOG_FUNC("(0x%p) %08x --%02x%02x%02x\n",
					 pu4StartAddr, pu4StartAddr[0],
					 pucAddr[2], pucAddr[1], pucAddr[0]);
				break;
			case 8:
				LOG_FUNC("(0x%p) %08x %08x\n", pu4StartAddr,
					 pu4StartAddr[0], pu4StartAddr[1]);
				break;
			case 9:
				pucAddr = (uint8_t *) &pu4StartAddr[2];
				LOG_FUNC("(0x%p) %08x %08x ------%02x\n",
					 pu4StartAddr, pu4StartAddr[0],
					 pu4StartAddr[1], pucAddr[0]);
				break;
			case 10:
				pucAddr = (uint8_t *) &pu4StartAddr[2];
				LOG_FUNC("(0x%p) %08x %08x ----%02x%02x\n",
					 pu4StartAddr, pu4StartAddr[0],
					 pu4StartAddr[1], pucAddr[1],
					 pucAddr[0]);
				break;
			case 11:
				pucAddr = (uint8_t *) &pu4StartAddr[2];
				LOG_FUNC("(0x%p) %08x %08x --%02x%02x%02x\n",
					 pu4StartAddr,
					 pu4StartAddr[0], pu4StartAddr[1],
					 pucAddr[2], pucAddr[1], pucAddr[0]);
				break;
			case 12:
				LOG_FUNC("(0x%p) %08x %08x %08x\n",
					 pu4StartAddr,
					 pu4StartAddr[0], pu4StartAddr[1],
					 pu4StartAddr[2]);
				break;
			case 13:
				pucAddr = (uint8_t *) &pu4StartAddr[3];
				LOG_FUNC("(0x%p) %08x %08x %08x ------%02x\n",
					 pu4StartAddr,
					 pu4StartAddr[0], pu4StartAddr[1],
					 pu4StartAddr[2], pucAddr[0]);
				break;
			case 14:
				pucAddr = (uint8_t *) &pu4StartAddr[3];
				LOG_FUNC("(0x%p) %08x %08x %08x ----%02x%02x\n",
					 pu4StartAddr,
					 pu4StartAddr[0], pu4StartAddr[1],
					 pu4StartAddr[2],
					 pucAddr[1], pucAddr[0]);
				break;
			case 15:
			default:
				pucAddr = (uint8_t *) &pu4StartAddr[3];
				LOG_FUNC(DM32_CASE15,
					 pu4StartAddr,
					 pu4StartAddr[0], pu4StartAddr[1],
					 pu4StartAddr[2],
					 pucAddr[2], pucAddr[1], pucAddr[0]);
				break;
			}
			u4Length = 0;
		}
	}
}				/* end of dumpMemory32() */


/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to dump a segment of memory in 4 double words.
 *
 * \param[in] pucStartAddr   Pointer to the starting address of the memory
 *                           to be dumped.
 * \param[in] u4Length       Length of the memory to be dumped.
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void dumpMemory128(uint32_t *pu4StartAddr,
		  uint32_t u4Length)
{
	uint8_t *pucAddr;
#define DM128_LEN64 "(0x%p) %08x %08x %08x %08x" \
					" %08x %08x %08x %08x" \
					" %08x %08x %08x %08x" \
					" %08x %08x %08x %08x\n"
#define DM128_CASE15 "(0x%p) %08x %08x %08x --%02x%02x%02x\n"

	ASSERT(pu4StartAddr);

	LOG_FUNC("DUMP32 ADDRESS: 0x%p, Length: %d ", pu4StartAddr,
		 u4Length);

	if (IS_NOT_ALIGN_4((uintptr_t)pu4StartAddr)) {
		uint32_t u4ProtrudeLen =
			sizeof(uint32_t) - ((uintptr_t)pu4StartAddr % 4);

		u4ProtrudeLen =
			((u4Length < u4ProtrudeLen) ? u4Length : u4ProtrudeLen);
		LOG_FUNC("pu4StartAddr is not at DW boundary.\n");
		pucAddr = (uint8_t *) &pu4StartAddr[0];

		switch (u4ProtrudeLen) {
		case 1:
			LOG_FUNC("(0x%p) %02x------\n", pu4StartAddr,
				pucAddr[0]);
			break;
		case 2:
			LOG_FUNC("(0x%p) %02x%02x----\n", pu4StartAddr,
				 pucAddr[1], pucAddr[0]);
			break;
		case 3:
			LOG_FUNC("(0x%p) %02x%02x%02x--\n", pu4StartAddr,
				 pucAddr[2], pucAddr[1], pucAddr[0]);
			break;
		default:
			break;
		}

		u4Length -= u4ProtrudeLen;
		pu4StartAddr = (uint32_t *)
			       ((uintptr_t)pu4StartAddr + u4ProtrudeLen);
	}

	while (u4Length > 0) {
		if (u4Length >= 64) {
			LOG_FUNC(DM128_LEN64,
				 pu4StartAddr,
				 pu4StartAddr[0], pu4StartAddr[1],
				 pu4StartAddr[2], pu4StartAddr[3],
				 pu4StartAddr[4], pu4StartAddr[5],
				 pu4StartAddr[6], pu4StartAddr[7],
				 pu4StartAddr[8], pu4StartAddr[9],
				 pu4StartAddr[10], pu4StartAddr[11],
				 pu4StartAddr[12], pu4StartAddr[13],
				 pu4StartAddr[14], pu4StartAddr[15]);
			pu4StartAddr += 16;
			u4Length -= 64;
		} else if (u4Length >= 16) {
			LOG_FUNC("(0x%p) %08x %08x %08x %08x\n",
				 pu4StartAddr,
				 pu4StartAddr[0], pu4StartAddr[1],
				 pu4StartAddr[2], pu4StartAddr[3]);
			pu4StartAddr += 4;
			u4Length -= 16;
		} else {
			switch (u4Length) {
			case 1:
				pucAddr = (uint8_t *) &pu4StartAddr[0];
				LOG_FUNC("(%p) ------%02x\n",
					 pu4StartAddr, pucAddr[0]);
				break;
			case 2:
				pucAddr = (uint8_t *) &pu4StartAddr[0];
				LOG_FUNC("(0x%p) ----%02x%02x\n", pu4StartAddr,
					 pucAddr[1], pucAddr[0]);
				break;
			case 3:
				pucAddr = (uint8_t *) &pu4StartAddr[0];
				LOG_FUNC("(0x%p) --%02x%02x%02x\n",
					pu4StartAddr,
					pucAddr[2], pucAddr[1], pucAddr[0]);
				break;
			case 4:
				LOG_FUNC("(0x%p) %08x\n", pu4StartAddr,
					 pu4StartAddr[0]);
				break;
			case 5:
				pucAddr = (uint8_t *) &pu4StartAddr[1];
				LOG_FUNC("(0x%p) %08x ------%02x\n",
					pu4StartAddr,
					pu4StartAddr[0], pucAddr[0]);
				break;
			case 6:
				pucAddr = (uint8_t *) &pu4StartAddr[1];
				LOG_FUNC("(0x%p) %08x ----%02x%02x\n",
					 pu4StartAddr, pu4StartAddr[0],
					 pucAddr[1], pucAddr[0]);
				break;
			case 7:
				pucAddr = (uint8_t *) &pu4StartAddr[1];
				LOG_FUNC("(0x%p) %08x --%02x%02x%02x\n",
					 pu4StartAddr, pu4StartAddr[0],
					 pucAddr[2], pucAddr[1], pucAddr[0]);
				break;
			case 8:
				LOG_FUNC("(0x%p) %08x %08x\n", pu4StartAddr,
					 pu4StartAddr[0], pu4StartAddr[1]);
				break;
			case 9:
				pucAddr = (uint8_t *) &pu4StartAddr[2];
				LOG_FUNC("(0x%p) %08x %08x ------%02x\n",
					 pu4StartAddr, pu4StartAddr[0],
					 pu4StartAddr[1], pucAddr[0]);
				break;
			case 10:
				pucAddr = (uint8_t *) &pu4StartAddr[2];
				LOG_FUNC("(0x%p) %08x %08x ----%02x%02x\n",
					 pu4StartAddr, pu4StartAddr[0],
					 pu4StartAddr[1], pucAddr[1],
					 pucAddr[0]);
				break;
			case 11:
				pucAddr = (uint8_t *) &pu4StartAddr[2];
				LOG_FUNC("(%p) %08x %08x --%02x%02x%02x\n",
					 pu4StartAddr,
					 pu4StartAddr[0], pu4StartAddr[1],
					 pucAddr[2], pucAddr[1], pucAddr[0]);
				break;
			case 12:
				LOG_FUNC("(0x%p) %08x %08x %08x\n",
					 pu4StartAddr,
					 pu4StartAddr[0], pu4StartAddr[1],
					 pu4StartAddr[2]);
				break;
			case 13:
				pucAddr = (uint8_t *) &pu4StartAddr[3];
				LOG_FUNC("(%p) %08x %08x %08x ------%02x\n",
					 pu4StartAddr,
					 pu4StartAddr[0], pu4StartAddr[1],
					 pu4StartAddr[2], pucAddr[0]);
				break;
			case 14:
				pucAddr = (uint8_t *) &pu4StartAddr[3];
				LOG_FUNC("(0x%p) %08x %08x %08x ----%02x%02x\n",
					 pu4StartAddr,
					 pu4StartAddr[0], pu4StartAddr[1],
					 pu4StartAddr[2],
					 pucAddr[1], pucAddr[0]);
				break;
			case 15:
			default:
				pucAddr = (uint8_t *) &pu4StartAddr[3];
				LOG_FUNC(DM128_CASE15,
					 pu4StartAddr,
					 pu4StartAddr[0], pu4StartAddr[1],
					 pu4StartAddr[2],
					 pucAddr[2], pucAddr[1], pucAddr[0]);
				break;
			}
			u4Length = 0;
		}
	}
}				/* end of dumpMemory32() */
