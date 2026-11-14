/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef __NAN_LINK_H__
#define __NAN_LINK_H__

#if CFG_SUPPORT_NAN

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
void nanGetLinkWmmQueSet(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo);

struct BSS_INFO *nanGetDefaultLinkBssInfo(
	struct ADAPTER *ad,
	struct BSS_INFO *bss);

u_int8_t nanLinkNeedMlo(
	struct ADAPTER *prAdapter);

u_int8_t nanGetStaRecExist(
	struct _NAN_NDP_CONTEXT_T *cxt,
	struct STA_RECORD *sta);

void nanDumpStaRec(
	struct _NAN_NDP_CONTEXT_T *cxt);

struct STA_RECORD *nanGetLinkStaRec(
	struct _NAN_NDP_CONTEXT_T *cxt,
	uint8_t ucLinkIdx);

void nanSetLinkStaRec(
	struct _NAN_NDP_CONTEXT_T *cxt,
	struct STA_RECORD *sta,
	uint8_t ucLinkIdx);

u_int8_t
nanIsTxKeyReady(
	struct _NAN_NDP_CONTEXT_T *cxt);

uint8_t
nanGetBssIdxbyLink(
	struct ADAPTER *prAdapter,
	uint8_t ucLinkIdx);

enum NAN_BSS_ROLE_INDEX
nanGetRoleIndexbyLink(
	uint8_t ucLinkIdx);

uint8_t
nanGetLinkIndexbyRole(
	enum NAN_BSS_ROLE_INDEX eRole);

uint8_t
nanGetLinkIndexbyOpClass(
	struct ADAPTER *prAdapter,
	uint32_t op);

void nanResetStaRec(
	struct _NAN_NDP_CONTEXT_T *cxt);

uint32_t nanLinkResetTk(
	struct ADAPTER *ad,
	struct _NAN_NDP_CONTEXT_T *cxt);

uint32_t nanSetPreferLinkStaRec(
	struct ADAPTER *ad,
	struct _NAN_NDP_CONTEXT_T *cxt,
	uint8_t idx);

struct STA_RECORD *nanGetPreferLinkStaRec(
	struct ADAPTER *ad,
	struct _NAN_NDP_CONTEXT_T *cxt);

uint8_t
nanGetLinkIndexbyBand(
	struct ADAPTER *ad,
	enum ENUM_BAND eBand);

#if (CFG_SUPPORT_NAN_11BE_MLO == 1)
void nanMldBssInit(struct ADAPTER *prAdapter);
void nanMldBssUninit(struct ADAPTER *prAdapter);
void nanMldBssRegister(struct ADAPTER *prAdapter,
	struct BSS_INFO *prNanBssInfo);
void nanMldBssUnregister(struct ADAPTER *prAdapter,
	struct BSS_INFO *prNanBssInfo);
void nanMldStaRecRegister(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucLinkIndex);
#endif /* CFG_SUPPORT_NAN_11BE_MLO */

#endif /* CFG_SUPPORT_NAN */

#endif /* __NAN_LINK_H__ */
