/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file  uni_fw_dl.h
 */

#ifndef _MULTIADDR_HEADER_H_
#define _MULTIADDR_HEADER_H_

#define SEC_TYPE_SEGMENT_MASK			0xFF000000
#define SEC_TYPE_SEGMENT_CE			0x03000000

#define SEC_TYPE_SUBSYS_MASK			0x00FF0000
#define SEC_TYPE_SUBSYS_CBMCU			0x00010000

#define SEC_TYPE_SUBSYS_SEC_MASK		0x0000FFFF
#define SEC_TYPE_SUBSYS_SEC_RELEASE_INFO	0x00000001
#define SEC_TYPE_SUBSYS_SEC_BIN_INFO		0x00000002
#define SEC_TYPE_SUBSYS_SEC_IDX_LOG		0x00000003
#define SEC_TYPE_SUBSYS_SEC_TX_PWR_LIMIT	0x00000004
#define SEC_TYPE_SUBSYS_SEC_IMG_SIGN		0x00000005


__KAL_ATTRIB_PACKED_FRONT__
struct MULTI_HEADER_V2_SEC_RAW_FORMAT {
	uint32_t u4Type;
	uint32_t u4Offset;
	uint32_t u4Size;
	uint8_t aucSpecific[52];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct MULTI_HEADER_V2_RAW_FORMAT {
	uint8_t aucPackTime[20];
	uint8_t aucChipIdNEcoVer[8];
	uint32_t u4PatchVer;
	uint8_t aucReserved1[4];
	uint32_t u4GlobalSubsys;
	uint8_t aucReserved2[4];
	uint32_t u4SecNum;
	uint8_t aucReserved3[48];
	struct MULTI_HEADER_V2_SEC_RAW_FORMAT aucSections[];
} __KAL_ATTRIB_PACKED__;


#endif /* _MULTIADDR_HEADER_H_ */
