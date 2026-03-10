/******************************************************************************
 *
 * This file is provided under a dual license.  When you use or
 * distribute this software, you may choose to be licensed under
 * version 2 of the GNU General Public License ("GPLv2 License")
 * or BSD License.
 *
 * GPLv2 License
 *
 * Copyright(C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * BSD LICENSE
 *
 * Copyright(C) 2016 MediaTek Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/
/******************************************************************************
 *[File]             kal_pdma.c
 *[Version]          v1.0
 *[Revision Date]    2010-03-01
 *[Author]
 *[Description]
 *    The program provides PCIE HIF driver
 *[Copyright]
 *    Copyright (C) 2010 MediaTek Incorporation. All Rights Reserved.
 ******************************************************************************/


/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

#include "gl_os.h"

#include "hif_pdma.h"

#include "precomp.h"

#include <linux/kernel.h>
#include <linux/mm.h>
#ifndef CONFIG_X86
#include <asm/memory.h>
#endif

#include "mt66xx_reg.h"
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
#include "connv3.h"
#endif

#if IS_MOBILE_SEGMENT
#include <aee.h>
#endif

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
static enum ENUM_CMD_TX_RESULT kalDevWriteCmdByQueue(
		struct GLUE_INFO *prGlueInfo, struct CMD_INFO *prCmdInfo,
		uint8_t ucTC);
static bool kalDevWriteDataByQueue(struct GLUE_INFO *prGlueInfo,
				   struct MSDU_INFO *prMsduInfo);
static bool kalDevKickMsduData(struct GLUE_INFO *prGlueInfo,
				struct list_head *prHead);
static bool kalDevKickAmsduData(struct GLUE_INFO *prGlueInfo,
				struct list_head *prHead);
static u_int8_t kalDevRegReadStatic(struct GLUE_INFO *prGlueInfo,
	uint32_t u4Register, uint32_t *pu4Value);
static u_int8_t kalDevRegWriteStatic(struct GLUE_INFO *prGlueInfo,
	uint32_t u4Register, uint32_t u4Value);
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
static u_int8_t kalDevRegReadViaBT(struct GLUE_INFO *prGlueInfo,
				uint32_t u4Register, uint32_t *pu4Value);
static u_int8_t kalDevRegWriteViaBT(struct GLUE_INFO *prGlueInfo,
				uint32_t u4Register, uint32_t u4Value);
#endif
/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

/*----------------------------------------------------------------------------*/
/*!
 * \brief Check connsys is dead or not
 *
 * \param[in] prGlueInfo Pointer to the GLUE_INFO_T structure.
 * \param[in] u4Register Register address
 * \param[in] pu4Value   Pointer to read value
 *
 * \retval TRUE          connsys is dead
 * \retval FALSE         connsys is alive
 */
/*----------------------------------------------------------------------------*/
static inline bool kalIsChipDead(struct GLUE_INFO *prGlueInfo,
				 uint32_t u4Register, uint32_t *pu4Value)
{
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct mt66xx_chip_info *prChipInfo = NULL;
	uint32_t u4Value;
	uint32_t u4BusAddr;
	u_int8_t fgCrReadDead = FALSE;

	prHifInfo = &prGlueInfo->rHifInfo;
	prChipInfo = prGlueInfo->prAdapter->chip_info;

#if (CFG_ENABLE_HOST_BUS_TIMEOUT == 1)
	if (*pu4Value == 0xdead0001) {
		DBGLOG(HAL, ERROR, "CR read[0x%08x] = dead0001\n",
				u4Register);
		fgCrReadDead = TRUE;
	}
#endif

	if (*pu4Value != HIF_DEADFEED_VALUE && !fgCrReadDead)
		return FALSE;

	if (!halChipToStaticMapBusAddr(prChipInfo, CONN_CFG_CHIP_ID_ADDR,
				       &u4BusAddr)) {
		DBGLOG(HAL, ERROR, "Not exist CR read[0x%08x]\n", u4Register);
		return FALSE;
	}

	RTMP_IO_READ32(prChipInfo, u4BusAddr, &u4Value);

#if (CFG_ENABLE_HOST_BUS_TIMEOUT == 1)
	if (u4Value == 0xdead0001 && fgCrReadDead) {
		DBGLOG(HAL, ERROR, "Host bus hang timeout, CR[0x%08x]\n",
				u4Register);
		return TRUE;
	}
#endif

#if (CFG_SUPPORT_CONNAC2X == 1)
	if (prGlueInfo->prAdapter->fgIsFwOwn)
		return FALSE;
#endif

	return u4Value == HIF_DEADFEED_VALUE;
}

static void kalDevRegL1Read(struct GLUE_INFO *prGlueInfo,
	struct mt66xx_chip_info *prChipInfo,
	uint32_t reg, uint32_t *val)
{
	const struct PCIE_CHIP_CR_REMAPPING *remap =
		prChipInfo->bus_info->bus2chip_remap;
	const struct pcie2ap_remap *pcie2ap;
	unsigned long flags;
	uint32_t backup_val = 0, tmp_val = 0;

	if (!remap) {
		DBGLOG(INIT, ERROR, "Remapping table NOT supported\n");
		return;
	}

	pcie2ap = remap->pcie2ap;
	if (!pcie2ap) {
		DBGLOG(INIT, ERROR, "pcie2ap remap NOT supported\n");
		return;
	}

	kalAcquireSpinLock(prGlueInfo, SPIN_LOCK_HIF_REMAP, &flags);
	kalDevRegReadStatic(prGlueInfo, pcie2ap->reg_base, &backup_val);
	tmp_val = (backup_val & ~pcie2ap->reg_mask);
	tmp_val |= GET_L1_REMAP_BASE(reg) << pcie2ap->reg_shift;
	kalDevRegWriteStatic(prGlueInfo, pcie2ap->reg_base, tmp_val);
	kalDevRegReadStatic(prGlueInfo, pcie2ap->base_addr +
		GET_L1_REMAP_OFFSET(reg), val);
	kalDevRegWriteStatic(prGlueInfo, pcie2ap->reg_base, backup_val);
	kalReleaseSpinLock(prGlueInfo, SPIN_LOCK_HIF_REMAP, flags);
}

static void kalDevRegL1Write(struct GLUE_INFO *prGlueInfo,
	struct mt66xx_chip_info *prChipInfo,
	uint32_t reg, uint32_t val)
{
	const struct PCIE_CHIP_CR_REMAPPING *remap =
		prChipInfo->bus_info->bus2chip_remap;
	const struct pcie2ap_remap *pcie2ap;
	unsigned long flags;
	uint32_t backup_val = 0, tmp_val = 0;

	if (!remap) {
		DBGLOG(INIT, ERROR, "Remapping table NOT supported\n");
		return;
	}

	pcie2ap = remap->pcie2ap;
	if (!pcie2ap) {
		DBGLOG(INIT, ERROR, "pcie2ap remap NOT supported\n");
		return;
	}

	kalAcquireSpinLock(prGlueInfo, SPIN_LOCK_HIF_REMAP, &flags);
	kalDevRegReadStatic(prGlueInfo, pcie2ap->reg_base, &backup_val);
	tmp_val = (backup_val & ~pcie2ap->reg_mask);
	tmp_val |= GET_L1_REMAP_BASE(reg) << pcie2ap->reg_shift;
	kalDevRegWriteStatic(prGlueInfo, pcie2ap->reg_base, tmp_val);
	kalDevRegWriteStatic(prGlueInfo, pcie2ap->base_addr +
		GET_L1_REMAP_OFFSET(reg), val);
	kalDevRegWriteStatic(prGlueInfo, pcie2ap->reg_base, backup_val);
	kalReleaseSpinLock(prGlueInfo, SPIN_LOCK_HIF_REMAP, flags);
}

static void kalDevRegL2Read(struct GLUE_INFO *prGlueInfo,
	struct mt66xx_chip_info *prChipInfo,
	uint32_t reg, uint32_t *val)
{
	const struct PCIE_CHIP_CR_REMAPPING *remap =
		prChipInfo->bus_info->bus2chip_remap;
	const struct ap2wf_remap *ap2wf;
#if defined(_HIF_PCIE)
	const struct pcie2ap_remap *pcie2ap;
	uint32_t backup_val = 0, tmp_val = 0;
#endif
	unsigned long flags;

	if (!remap) {
		DBGLOG(INIT, ERROR, "Remapping table NOT supported\n");
		return;
	}

#if defined(_HIF_PCIE)
	pcie2ap = remap->pcie2ap;
	if (!pcie2ap) {
		DBGLOG(INIT, ERROR, "pcie2ap remap NOT supported\n");
		return;
	}
#endif

	ap2wf = remap->ap2wf;
	if (!ap2wf) {
		DBGLOG(INIT, ERROR, "ap2wf remap NOT supported\n");
		return;
	}

	kalAcquireSpinLock(prGlueInfo, SPIN_LOCK_HIF_REMAP, &flags);
#if defined(_HIF_PCIE)
	kalDevRegReadStatic(prGlueInfo, pcie2ap->reg_base, &backup_val);
	tmp_val = (backup_val & ~pcie2ap->reg_mask);
	tmp_val |= GET_L1_REMAP_BASE(ap2wf->base_addr -
		CONN_INFRA_MCU_TO_PHY_ADDR_OFFSET) <<
		pcie2ap->reg_shift;
	kalDevRegWriteStatic(prGlueInfo, pcie2ap->reg_base, tmp_val);
#endif

	kalDevRegWriteStatic(prGlueInfo, ap2wf->reg_base, reg);
	kalDevRegReadStatic(prGlueInfo, ap2wf->base_addr, val);

#if defined(_HIF_PCIE)
	kalDevRegWriteStatic(prGlueInfo, pcie2ap->reg_base, backup_val);
#endif
	kalReleaseSpinLock(prGlueInfo, SPIN_LOCK_HIF_REMAP, flags);
}

static void  kalDevRegL2Write(struct GLUE_INFO *prGlueInfo,
	struct mt66xx_chip_info *prChipInfo,
	uint32_t reg, uint32_t val)
{
	const struct PCIE_CHIP_CR_REMAPPING *remap =
		prChipInfo->bus_info->bus2chip_remap;
	const struct ap2wf_remap *ap2wf;
#if defined(_HIF_PCIE)
	const struct pcie2ap_remap *pcie2ap;
	uint32_t backup_val = 0, tmp_val = 0;
#endif
	unsigned long flags;

	if (!remap) {
		DBGLOG(INIT, ERROR, "Remapping table NOT supported\n");
		return;
	}

#if defined(_HIF_PCIE)
	pcie2ap = remap->pcie2ap;
	if (!pcie2ap) {
		DBGLOG(INIT, ERROR, "pcie2ap remap NOT supported\n");
		return;
	}
#endif

	ap2wf = remap->ap2wf;
	if (!ap2wf) {
		DBGLOG(INIT, ERROR, "ap2wf remap NOT supported\n");
		return;
	}

	kalAcquireSpinLock(prGlueInfo, SPIN_LOCK_HIF_REMAP, &flags);
#if defined(_HIF_PCIE)
	kalDevRegReadStatic(prGlueInfo, pcie2ap->reg_base, &backup_val);
	tmp_val = (backup_val & ~pcie2ap->reg_mask);
	tmp_val |= GET_L1_REMAP_BASE(ap2wf->base_addr -
		CONN_INFRA_MCU_TO_PHY_ADDR_OFFSET) <<
		pcie2ap->reg_shift;
	kalDevRegWriteStatic(prGlueInfo, pcie2ap->reg_base, tmp_val);
#endif

	kalDevRegWriteStatic(prGlueInfo, ap2wf->reg_base, reg);
	kalDevRegWriteStatic(prGlueInfo, ap2wf->base_addr, val);

#if defined(_HIF_PCIE)
	kalDevRegWriteStatic(prGlueInfo, pcie2ap->reg_base, backup_val);
#endif
	kalReleaseSpinLock(prGlueInfo, SPIN_LOCK_HIF_REMAP, flags);
}

u_int8_t kalDevRegL1ReadRange(struct GLUE_INFO *glue,
	struct mt66xx_chip_info *chip_info,
	uint32_t reg,
	void *buf,
	uint32_t total_size)
{
	const struct PCIE_CHIP_CR_REMAPPING *remap;
	const struct pcie2ap_remap *pcie2ap;
	uint32_t backup_val = 0, tmp_val = 0;
	uint32_t offset = 0;
	unsigned long flags;

	remap = chip_info->bus_info->bus2chip_remap;
	if (!remap) {
		DBGLOG(INIT, ERROR, "Remapping table NOT supported\n");
		return FALSE;
	}

	pcie2ap = remap->pcie2ap;
	if (!pcie2ap) {
		DBGLOG(INIT, ERROR, "pcie2ap remap NOT supported\n");
		return FALSE;
	}

	kalAcquireSpinLock(glue, SPIN_LOCK_HIF_REMAP, &flags);
	kalDevRegReadStatic(glue, pcie2ap->reg_base, &backup_val);
	tmp_val = (backup_val & ~pcie2ap->reg_mask);
	tmp_val |= GET_L1_REMAP_BASE(reg) << pcie2ap->reg_shift;
	kalDevRegWriteStatic(glue, pcie2ap->reg_base, tmp_val);
	while (true) {
		uint32_t size = 0;

		if (offset >= total_size)
			break;

		size = ((offset + BUS_REMAP_SIZE) <= total_size ?
			BUS_REMAP_SIZE :
			total_size - offset);

		RTMP_IO_READ_RANGE(chip_info,
			pcie2ap->base_addr +
				GET_L1_REMAP_OFFSET(reg),
			(void *)(buf + offset),
			size);

		offset += BUS_REMAP_SIZE;
	}
	kalDevRegWriteStatic(glue, pcie2ap->reg_base, backup_val);
	kalReleaseSpinLock(glue, SPIN_LOCK_HIF_REMAP, flags);
	return TRUE;
}

u_int8_t kalDevRegL1WriteRange(struct GLUE_INFO *glue,
	struct mt66xx_chip_info *chip_info,
	uint32_t reg,
	void *buf,
	uint32_t total_size)
{
	const struct PCIE_CHIP_CR_REMAPPING *remap;
	const struct pcie2ap_remap *pcie2ap;
	uint32_t backup_val = 0, tmp_val = 0;
	uint32_t offset = 0;
	unsigned long flags;

	remap = chip_info->bus_info->bus2chip_remap;
	if (!remap) {
		DBGLOG(INIT, ERROR, "Remapping table NOT supported\n");
		return FALSE;
	}

	pcie2ap = remap->pcie2ap;
	if (!pcie2ap) {
		DBGLOG(INIT, ERROR, "pcie2ap remap NOT supported\n");
		return FALSE;
	}

	kalAcquireSpinLock(glue, SPIN_LOCK_HIF_REMAP, &flags);
	kalDevRegReadStatic(glue, pcie2ap->reg_base, &backup_val);
	tmp_val = (backup_val & ~pcie2ap->reg_mask);
	tmp_val |= GET_L1_REMAP_BASE(reg) << pcie2ap->reg_shift;
	kalDevRegWriteStatic(glue, pcie2ap->reg_base, tmp_val);
	while (true) {
		uint32_t size = 0;

		if (offset >= total_size)
			break;

		size = ((offset + BUS_REMAP_SIZE) <= total_size ?
			BUS_REMAP_SIZE :
			total_size - offset);

		RTMP_IO_WRITE_RANGE(chip_info,
			pcie2ap->base_addr +
				GET_L1_REMAP_OFFSET(reg),
			(void *)(buf + offset),
			size);

		offset += BUS_REMAP_SIZE;
	}
	kalDevRegWriteStatic(glue, pcie2ap->reg_base, backup_val);
	kalReleaseSpinLock(glue, SPIN_LOCK_HIF_REMAP, flags);
	return TRUE;
}

u_int8_t kalDevRegL2ReadRange(struct GLUE_INFO *glue,
	struct mt66xx_chip_info *chip_info,
	uint32_t reg,
	void *buf,
	uint32_t total_size)
{
	const struct PCIE_CHIP_CR_REMAPPING *remap;
	const struct ap2wf_remap *ap2wf;
#if defined(_HIF_PCIE)
	const struct pcie2ap_remap *pcie2ap;
	uint32_t value = 0, backup_val = 0;
#endif
	uint32_t offset_addr = 0;
	uint32_t offset = 0;
	u_int8_t ret = TRUE;
	unsigned long flags;

	remap = chip_info->bus_info->bus2chip_remap;
	if (!remap) {
		DBGLOG(INIT, ERROR, "Remapping table NOT supported\n");
		return FALSE;
	}

#if defined(_HIF_PCIE)
	pcie2ap = remap->pcie2ap;
	if (!pcie2ap) {
		DBGLOG(INIT, ERROR, "pcie2ap remap NOT supported\n");
		return FALSE;
	}
#endif

	ap2wf = remap->ap2wf;
	if (!ap2wf) {
		DBGLOG(INIT, ERROR, "ap2wf remap NOT supported\n");
		return FALSE;
	}

	kalAcquireSpinLock(glue, SPIN_LOCK_HIF_REMAP, &flags);
#if defined(_HIF_PCIE)
	kalDevRegReadStatic(glue, pcie2ap->reg_base, &value);
	backup_val = value;

	value &= ~pcie2ap->reg_mask;
	value |= (GET_L1_REMAP_BASE(ap2wf->base_addr -
		CONN_INFRA_MCU_TO_PHY_ADDR_OFFSET) <<
		pcie2ap->reg_shift);
	kalDevRegWriteStatic(glue, pcie2ap->reg_base, value);
#endif

	if (!halChipToStaticMapBusAddr(chip_info,
			ap2wf->base_addr,
			&offset_addr)) {
		DBGLOG(INIT, ERROR, "map bus address fail.\n");
		ret = FALSE;
		goto exit;
	}

	while (true) {
		uint32_t size = 0;

		if (offset >= total_size)
			break;

		size = ((offset + BUS_REMAP_SIZE) <= total_size ?
			BUS_REMAP_SIZE :
			total_size - offset);

		kalDevRegWriteStatic(glue, ap2wf->reg_base,
			(reg + offset));

		RTMP_IO_READ_RANGE(chip_info,
			offset_addr,
			(void *)(buf + offset),
			size);

		offset += BUS_REMAP_SIZE;
	}

exit:
#if defined(_HIF_PCIE)
	kalDevRegWriteStatic(glue, pcie2ap->reg_base, backup_val);
#endif
	kalReleaseSpinLock(glue, SPIN_LOCK_HIF_REMAP, flags);

	return ret;
}

u_int8_t kalDevRegL2WriteRange(struct GLUE_INFO *glue,
	struct mt66xx_chip_info *chip_info,
	uint32_t reg,
	void *buf,
	uint32_t total_size)
{
	const struct PCIE_CHIP_CR_REMAPPING *remap;
	const struct ap2wf_remap *ap2wf;
#if defined(_HIF_PCIE)
	const struct pcie2ap_remap *pcie2ap;
	uint32_t value = 0, backup_val = 0;
#endif
	uint32_t offset_addr = 0;
	uint32_t offset = 0;
	u_int8_t ret = TRUE;
	unsigned long flags;

	DBGLOG(INIT, INFO, "reg: 0x%x, total_size: 0x%x\n", reg, total_size);

	remap = chip_info->bus_info->bus2chip_remap;
	if (!remap) {
		DBGLOG(INIT, ERROR, "Remapping table NOT supported\n");
		return FALSE;
	}

#if defined(_HIF_PCIE)
	pcie2ap = remap->pcie2ap;
	if (!pcie2ap) {
		DBGLOG(INIT, ERROR, "pcie2ap remap NOT supported\n");
		return FALSE;
	}
#endif

	ap2wf = remap->ap2wf;
	if (!ap2wf) {
		DBGLOG(INIT, ERROR, "ap2wf remap NOT supported\n");
		return FALSE;
	}

	kalAcquireSpinLock(glue, SPIN_LOCK_HIF_REMAP, &flags);
#if defined(_HIF_PCIE)
	kalDevRegReadStatic(glue, pcie2ap->reg_base, &value);
	backup_val = value;

	value &= ~pcie2ap->reg_mask;
	value |= (GET_L1_REMAP_BASE(ap2wf->base_addr -
		CONN_INFRA_MCU_TO_PHY_ADDR_OFFSET) <<
		pcie2ap->reg_shift);
	kalDevRegWriteStatic(glue, pcie2ap->reg_base, value);
#endif

	if (!halChipToStaticMapBusAddr(chip_info,
			ap2wf->base_addr,
			&offset_addr)) {
		DBGLOG(INIT, ERROR, "map bus address fail.\n");
		ret = FALSE;
		goto exit;
	}

	while (true) {
		uint32_t size = 0;

		if (offset >= total_size)
			break;

		size = ((offset + BUS_REMAP_SIZE) <= total_size ?
			BUS_REMAP_SIZE :
			total_size - offset);

		kalDevRegWriteStatic(glue, ap2wf->reg_base,
			(reg + offset));

		RTMP_IO_WRITE_RANGE(chip_info,
			offset_addr,
			(void *)(buf + offset),
			size);

		offset += BUS_REMAP_SIZE;
	}

exit:
#if defined(_HIF_PCIE)
	kalDevRegWriteStatic(glue, pcie2ap->reg_base, backup_val);
#endif
	kalReleaseSpinLock(glue, SPIN_LOCK_HIF_REMAP, flags);

	return ret;
}

static u_int8_t kalDevRegL1Remap(uint32_t *reg)
{
#if defined(_HIF_PCIE)
	if (IS_PHY_ADDR(*reg))
		return TRUE;

	if (IS_CONN_INFRA_MCU_ADDR(*reg)) {
		(*reg) -= CONN_INFRA_MCU_TO_PHY_ADDR_OFFSET;
		return TRUE;
	}
#endif

	return FALSE;
}

static u_int8_t kalIsHostReg(struct mt66xx_chip_info *prChipInfo,
			     uint32_t u4Reg)
{
	if (prChipInfo->HostCSRBaseAddress == NULL ||
	    prChipInfo->u4HostCsrOffset == 0 ||
	    prChipInfo->u4HostCsrSize == 0)
		return FALSE;

	return (u4Reg >= prChipInfo->u4HostCsrOffset) &&
		(u4Reg < (prChipInfo->u4HostCsrOffset +
			  prChipInfo->u4HostCsrSize));
}

#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
static u_int8_t kalDevRegReadViaBT(struct GLUE_INFO *prGlueInfo,
	uint32_t u4Register, uint32_t *pu4Value)
{
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	uint32_t u4BusAddr = 0;
	int ret = 0;

	if (prGlueInfo) {
		prHifInfo = &prGlueInfo->rHifInfo;
		prAdapter = prGlueInfo->prAdapter;
		if (!prAdapter) {
			DBGLOG(INIT, ERROR, "prAdapter is NULL.\n");
			return FALSE;
		}
	}

	glGetChipInfo((void **)&prChipInfo);
	if (!prChipInfo)
		return FALSE;

	if (halChipToStaticMapBusAddr(prChipInfo, u4Register, &u4BusAddr) ||
		IS_CONN_INFRA_MCU_ADDR(u4Register) ||
		IS_CBTOP_PHY_ADDR(u4Register)) {
		ret = connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI,
			CONNV3_DRV_TYPE_BT, u4Register, pu4Value);
		if (ret == 0) {
			DBGLOG(HAL, INFO,
				"Read success: CR[0x%08x] value[0x%08x]\n",
				u4Register, *pu4Value);
			return TRUE;
		}
		DBGLOG(HAL, ERROR,
			"Read fail: CR[0x%08x] value[0x%08x]\n",
			u4Register, *pu4Value);
		return FALSE;
	} else if (IS_CONN_INFRA_PHY_ADDR(u4Register) ||
		IS_WFSYS_PHY_ADDR(u4Register) ||
		IS_BGFSYS_PHY_ADDR(u4Register)) {

		/* Mapping to FW view addr */
		u4Register += CONN_INFRA_MCU_TO_PHY_ADDR_OFFSET;

		ret = connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI,
			CONNV3_DRV_TYPE_BT, u4Register, pu4Value);
		if (ret == 0) {
			DBGLOG(HAL, INFO,
				"Read success: CR[0x%08x] value[0x%08x]\n",
				u4Register, *pu4Value);
			return TRUE;
		}
		DBGLOG(HAL, ERROR,
			"Read fail: CR[0x%08x] value[0x%08x]\n",
			u4Register, *pu4Value);
		return FALSE;
	} else if (IS_CONN_MCU_CONFG_CFG_DBG1_ADDR(u4Register)) {
		/* AP2WF fixed remap */
		u4Register -= AP2WF_FIXED_REMAP_OFFSET;

		ret = connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI,
			CONNV3_DRV_TYPE_BT, u4Register, pu4Value);
		if (ret == 0) {
			DBGLOG(HAL, INFO,
				"Read success: CR[0x%08x] value[0x%08x]\n",
				u4Register, *pu4Value);
			return TRUE;
		}
		DBGLOG(HAL, ERROR,
			"Read fail: CR[0x%08x] value[0x%08x]\n",
			u4Register, *pu4Value);
		return FALSE;
	} else if (IS_CONN_MCU_BUS_CR_ADDR(u4Register)) {
		/* Dynamic remap */
		connv3_hif_dbg_write(CONNV3_DRV_TYPE_WIFI,
			CONNV3_DRV_TYPE_BT,
			AP2WF_DYNAMIC_REMAP_NO_1,
			CONN_MCU_BUS_CR_BASE_ADDR);
		u4Register &= BITS(0, 15);
		u4Register |= AP2WF_DYNAMIC_REMAP_NO_1_BASE_ADDR;

		ret = connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI,
			CONNV3_DRV_TYPE_BT, u4Register, pu4Value);
		if (ret == 0) {
			DBGLOG(HAL, INFO,
				"Read success: CR[0x%08x] value[0x%08x]\n",
				u4Register, *pu4Value);
			return TRUE;
		}
		DBGLOG(HAL, ERROR,
			"Read fail: CR[0x%08x] value[0x%08x]\n",
			u4Register, *pu4Value);
		return FALSE;
	} else if (IS_WF_MCUSYS_VDNR_ADDR(u4Register)) {
		/* Dynamic remap */
		connv3_hif_dbg_write(CONNV3_DRV_TYPE_WIFI,
			CONNV3_DRV_TYPE_BT,
			AP2WF_DYNAMIC_REMAP_NO_1,
			WF_MCUSYS_VDNR_GEN_BUS_U_DEBUG_CTRL_START);
		u4Register &= BITS(0, 15);
		u4Register |= AP2WF_DYNAMIC_REMAP_NO_1_BASE_ADDR;

		ret = connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI,
			CONNV3_DRV_TYPE_BT, u4Register, pu4Value);
		if (ret == 0) {
			DBGLOG(HAL, INFO,
				"Read success: CR[0x%08x] value[0x%08x]\n",
				u4Register, *pu4Value);
			return TRUE;
		}
		DBGLOG(HAL, ERROR,
			"Read fail: CR[0x%08x] value[0x%08x]\n",
			u4Register, *pu4Value);
		return FALSE;
	}
	DBGLOG(HAL, ERROR,
		"Invalid address: CR[0x%08x] value[0x%08x]\n",
		u4Register, *pu4Value);

	return FALSE;
}

static u_int8_t kalDevRegWriteViaBT(struct GLUE_INFO *prGlueInfo,
	uint32_t u4Register, uint32_t u4Value)
{
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	uint32_t u4BusAddr = 0;
	int ret = 0;

	if (prGlueInfo) {
		prHifInfo = &prGlueInfo->rHifInfo;
		prAdapter = prGlueInfo->prAdapter;
		if (!prAdapter) {
			DBGLOG(INIT, ERROR, "prAdapter is NULL.\n");
			return FALSE;
		}
	}

	glGetChipInfo((void **)&prChipInfo);
	if (!prChipInfo)
		return FALSE;

	if (halChipToStaticMapBusAddr(prChipInfo, u4Register, &u4BusAddr) ||
		IS_CONN_INFRA_MCU_ADDR(u4Register) ||
		IS_CBTOP_PHY_ADDR(u4Register)) {
		ret = connv3_hif_dbg_write(CONNV3_DRV_TYPE_WIFI,
			CONNV3_DRV_TYPE_BT, u4Register, u4Value);
		if (ret == 0) {
			DBGLOG(HAL, INFO,
				"Write success: CR[0x%08x] value[0x%08x]\n",
				u4Register, u4Value);
			return TRUE;
		}
		DBGLOG(HAL, ERROR,
			"Write fail: CR[0x%08x] value[0x%08x]\n",
			u4Register, u4Value);
		return FALSE;
	} else if (IS_CONN_INFRA_PHY_ADDR(u4Register) ||
		IS_WFSYS_PHY_ADDR(u4Register) ||
		IS_BGFSYS_PHY_ADDR(u4Register)) {

		/* Mapping to FW view addr */
		u4Register += CONN_INFRA_MCU_TO_PHY_ADDR_OFFSET;

		ret = connv3_hif_dbg_write(CONNV3_DRV_TYPE_WIFI,
			CONNV3_DRV_TYPE_BT, u4Register, u4Value);
		if (ret == 0) {
			DBGLOG(HAL, INFO,
				"Write success: CR[0x%08x] value[0x%08x]\n",
				u4Register, u4Value);
			return TRUE;
		}
		DBGLOG(HAL, ERROR,
			"Write fail: CR[0x%08x] value[0x%08x]\n",
			u4Register, u4Value);
		return FALSE;
	} else if (IS_CONN_MCU_CONFG_CFG_DBG1_ADDR(u4Register)) {
		/* AP2WF fixed remap */
		u4Register -= AP2WF_FIXED_REMAP_OFFSET;

		ret = connv3_hif_dbg_write(CONNV3_DRV_TYPE_WIFI,
			CONNV3_DRV_TYPE_BT, u4Register, u4Value);
		if (ret == 0) {
			DBGLOG(HAL, INFO,
				"Write success: CR[0x%08x] value[0x%08x]\n",
				u4Register, u4Value);
			return TRUE;
		}
		DBGLOG(HAL, ERROR,
			"Write fail: CR[0x%08x] value[0x%08x]\n",
			u4Register, u4Value);
		return FALSE;
	} else if (IS_CONN_MCU_BUS_CR_ADDR(u4Register)) {
		/* Dynamic remap */
		connv3_hif_dbg_write(CONNV3_DRV_TYPE_WIFI,
			CONNV3_DRV_TYPE_BT,
			AP2WF_DYNAMIC_REMAP_NO_1,
			CONN_MCU_BUS_CR_BASE_ADDR);
		u4Register &= BITS(0, 15);
		u4Register |= AP2WF_DYNAMIC_REMAP_NO_1_BASE_ADDR;

		ret = connv3_hif_dbg_write(CONNV3_DRV_TYPE_WIFI,
			CONNV3_DRV_TYPE_BT, u4Register, u4Value);
		if (ret == 0) {
			DBGLOG(HAL, INFO,
				"Write success: CR[0x%08x] value[0x%08x]\n",
				u4Register, u4Value);
			return TRUE;
		}
		DBGLOG(HAL, ERROR,
			"Write fail: CR[0x%08x] value[0x%08x]\n",
			u4Register, u4Value);
		return FALSE;
	} else if (IS_WF_MCUSYS_VDNR_ADDR(u4Register)) {
		/* Dynamic remap */
		connv3_hif_dbg_write(CONNV3_DRV_TYPE_WIFI,
			CONNV3_DRV_TYPE_BT,
			AP2WF_DYNAMIC_REMAP_NO_1,
			WF_MCUSYS_VDNR_GEN_BUS_U_DEBUG_CTRL_START);
		u4Register &= BITS(0, 15);
		u4Register |= AP2WF_DYNAMIC_REMAP_NO_1_BASE_ADDR;

		ret = connv3_hif_dbg_write(CONNV3_DRV_TYPE_WIFI,
			CONNV3_DRV_TYPE_BT, u4Register, u4Value);
		if (ret == 0) {
			DBGLOG(HAL, INFO,
				"Write success: CR[0x%08x] value[0x%08x]\n",
				u4Register, u4Value);
			return TRUE;
		}
		DBGLOG(HAL, ERROR,
			"Write fail: CR[0x%08x] value[0x%08x]\n",
			u4Register, u4Value);
		return FALSE;
	}
	DBGLOG(HAL, ERROR,
		"Invalid address: CR[0x%08x] value[0x%08x]\n",
		u4Register, u4Value);

	return FALSE;
}
#endif

static u_int8_t kalDevRegReadStatic(struct GLUE_INFO *prGlueInfo,
	uint32_t u4Register, uint32_t *pu4Value)
{
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	struct BUS_INFO *prBusInfo = NULL;
	uint32_t u4BusAddr = u4Register;

	ASSERT(pu4Value);

	if (prGlueInfo) {
		prHifInfo = &prGlueInfo->rHifInfo;
		prAdapter = prGlueInfo->prAdapter;
		if (!prAdapter) {
			DBGLOG(INIT, ERROR, "prAdapter is NULL.\n");
			return FALSE;
		}
	}

	glGetChipInfo((void **)&prChipInfo);
	if (!prChipInfo)
		return FALSE;

	if (kalIsHostReg(prChipInfo, u4Register)) {
		RTMP_HOST_IO_READ32(prChipInfo, u4Register, pu4Value);
		return TRUE;
	}

	if (fgIsBusAccessFailed) {
		DBGLOG_LIMITED(HAL, ERROR, "Bus access failed.\n");
		return FALSE;
	}

	prBusInfo = prChipInfo->bus_info;
	if (prHifInfo && !prHifInfo->fgForceReadWriteReg &&
	    prBusInfo->isValidRegAccess &&
	    !prBusInfo->isValidRegAccess(prAdapter, u4Register)) {
		/* Don't print log when resetting */
		if (prAdapter && !wlanIsChipNoAck(prAdapter)) {
			DBGLOG(HAL, ERROR,
			       "Invalid access! Get CR[0x%08x/0x%08x] value[0x%08x]\n",
			       u4Register, u4BusAddr, *pu4Value);
		}
		KAL_WARN_ON(TRUE);
		*pu4Value = HIF_DEADFEED_VALUE;
		return FALSE;
	}

	/* Static mapping */
	if (halChipToStaticMapBusAddr(prChipInfo, u4Register, &u4BusAddr)) {
		RTMP_IO_READ32(prChipInfo, u4BusAddr, pu4Value);
#if IS_MOBILE_SEGMENT && (CFG_SUPPORT_CONNAC3X == 0)
		if (prGlueInfo &&
		    kalIsChipDead(prGlueInfo, u4Register, pu4Value)) {
			/* Don't print log when resetting */
			if (prAdapter && !wlanIsChipNoAck(prAdapter)) {
				DBGLOG(HAL, ERROR,
				       "Read register is deadfeed\n");
				GL_DEFAULT_RESET_TRIGGER(prAdapter,
							 RST_REG_READ_DEADFEED);
			}
			return FALSE;
		}
#endif
	}

	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Read a 32-bit device register
 *
 * \param[in] prGlueInfo Pointer to the GLUE_INFO_T structure.
 * \param[in] u4Register Register offset
 * \param[in] pu4Value   Pointer to variable used to store read value
 *
 * \retval TRUE          operation success
 * \retval FALSE         operation fail
 */
/*----------------------------------------------------------------------------*/
u_int8_t kalDevRegRead(struct GLUE_INFO *prGlueInfo,
	uint32_t u4Register, uint32_t *pu4Value)
{
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	struct BUS_INFO *prBusInfo = NULL;
	uint32_t u4BusAddr = u4Register;

	ASSERT(pu4Value);

	if (prGlueInfo) {
		prHifInfo = &prGlueInfo->rHifInfo;
		prAdapter = prGlueInfo->prAdapter;
		if (!prAdapter) {
			DBGLOG(INIT, ERROR, "prAdapter is NULL.\n");
			return FALSE;
		}
	}

	glGetChipInfo((void **)&prChipInfo);
	if (!prChipInfo)
		return FALSE;

	if (kalIsHostReg(prChipInfo, u4Register)) {
		RTMP_HOST_IO_READ32(prChipInfo, u4Register, pu4Value);
		return TRUE;
	}

	if (fgIsBusAccessFailed) {
		DBGLOG_LIMITED(HAL, ERROR, "Bus access failed.\n");
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
		if (fgTriggerDebugSop && kalIsResetting()) {
			return kalDevRegReadViaBT(prGlueInfo,
				u4Register, pu4Value);
		}
#endif
		return FALSE;
	}

	prBusInfo = prChipInfo->bus_info;
	if (prHifInfo && !prHifInfo->fgForceReadWriteReg &&
	    prBusInfo->isValidRegAccess &&
	    !prBusInfo->isValidRegAccess(prAdapter, u4Register)) {
		/* Don't print log when resetting */
		if (prAdapter && !wlanIsChipNoAck(prAdapter)) {
			DBGLOG(HAL, ERROR,
			       "Invalid access! Get CR[0x%08x/0x%08x] value[0x%08x]\n",
			       u4Register, u4BusAddr, *pu4Value);
		}
		KAL_WARN_ON(TRUE);
		*pu4Value = HIF_DEADFEED_VALUE;
		return FALSE;
	}

	/* Static mapping */
	if (halChipToStaticMapBusAddr(prChipInfo, u4Register, &u4BusAddr)) {
		RTMP_IO_READ32(prChipInfo, u4BusAddr, pu4Value);
#if IS_MOBILE_SEGMENT && (CFG_SUPPORT_CONNAC3X == 0)
		if (prGlueInfo &&
		    kalIsChipDead(prGlueInfo, u4Register, pu4Value)) {
			/* Don't print log when resetting */
			if (prAdapter && !wlanIsChipNoAck(prAdapter)) {
				DBGLOG(HAL, ERROR,
				       "Read register is deadfeed\n");
				GL_DEFAULT_RESET_TRIGGER(prAdapter,
							 RST_REG_READ_DEADFEED);
			}
			return FALSE;
		}
#endif
	} else {
		if (kalDevRegL1Remap(&u4Register))
			kalDevRegL1Read(prGlueInfo, prChipInfo, u4Register,
				pu4Value);
		else
			kalDevRegL2Read(prGlueInfo, prChipInfo, u4Register,
				pu4Value);
	}

	return TRUE;
}

static u_int8_t kalDevRegWriteStatic(struct GLUE_INFO *prGlueInfo,
	uint32_t u4Register, uint32_t u4Value)
{
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	struct BUS_INFO *prBusInfo = NULL;
	uint32_t u4BusAddr = u4Register;

	if (prGlueInfo) {
		prHifInfo = &prGlueInfo->rHifInfo;
		prAdapter = prGlueInfo->prAdapter;
		if (!prAdapter) {
			DBGLOG(INIT, ERROR, "prAdapter is NULL.\n");
			return FALSE;
		}
	}

	glGetChipInfo((void **)&prChipInfo);
	if (!prChipInfo)
		return FALSE;

	if (kalIsHostReg(prChipInfo, u4Register)) {
		RTMP_HOST_IO_WRITE32(prChipInfo, u4Register, u4Value);
		return TRUE;
	}

	if (fgIsBusAccessFailed) {
		DBGLOG_LIMITED(HAL, ERROR, "Bus access failed.\n");
		return FALSE;
	}

	prBusInfo = prChipInfo->bus_info;
	if (prHifInfo && !prHifInfo->fgForceReadWriteReg &&
	    prBusInfo->isValidRegAccess &&
	    !prBusInfo->isValidRegAccess(prAdapter, u4Register)) {
		/* Don't print log when resetting */
		if (prAdapter && !wlanIsChipNoAck(prAdapter)) {
			DBGLOG(HAL, ERROR,
			       "Invalid access! Set CR[0x%08x/0x%08x] value[0x%08x]\n",
			       u4Register, u4BusAddr, u4Value);
		}
		KAL_WARN_ON(TRUE);
		return FALSE;
	}

#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
	if ((u4Register >= 0x18050000 && u4Register <= 0x18051000) ||
	    (u4Register >= 0x7c050000 && u4Register <= 0x7c051000) ||
	    (u4Register >= 0x7c000000 && u4Register < 0x7c001000) ||
	    (u4Register >= 0x18000000 && u4Register < 0x18001000)) {
		dump_stack();
		aee_kernel_exception("WLAN",
			"Corrupt conninfra cmdbt:  reg: 0x%08x, val: 0x%08x\n",
			u4Register, u4Value);
	}
#endif

	/* Static mapping */
#if (CFG_WLAN_ATF_SUPPORT == 1)
	if (halChipToStaticMapBusAddr(prChipInfo, u4Register, &u4BusAddr)) {
		kalSendAtfSmcCmd(SMC_WLAN_DEV_REG_WR_CR_OPID,
			prChipInfo->u4CsrOffset + u4BusAddr,
			u4Value, 0);
	} else {
		DBGLOG(INIT, ERROR, "Write CONSYS ERROR 0x%08x=0x%08x.\n",
			u4Register, u4Value);
	}
#else
	if (halChipToStaticMapBusAddr(prChipInfo, u4Register, &u4BusAddr))
		RTMP_IO_WRITE32(prChipInfo, u4BusAddr, u4Value);
#endif

	if (prHifInfo)
		prHifInfo->u4HifCnt++;

	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Write a 32-bit device register
 *
 * \param[in] prGlueInfo Pointer to the GLUE_INFO_T structure.
 * \param[in] u4Register Register offset
 * \param[in] u4Value    Value to be written
 *
 * \retval TRUE          operation success
 * \retval FALSE         operation fail
 */
/*----------------------------------------------------------------------------*/
u_int8_t kalDevRegWrite(struct GLUE_INFO *prGlueInfo,
	uint32_t u4Register, uint32_t u4Value)
{
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	struct BUS_INFO *prBusInfo = NULL;
	uint32_t u4BusAddr = u4Register;

	if (prGlueInfo) {
		prHifInfo = &prGlueInfo->rHifInfo;
		prAdapter = prGlueInfo->prAdapter;
		if (!prAdapter) {
			DBGLOG(INIT, ERROR, "prAdapter is NULL.\n");
			return FALSE;
		}
	}

	glGetChipInfo((void **)&prChipInfo);
	if (!prChipInfo)
		return FALSE;

	if (kalIsHostReg(prChipInfo, u4Register)) {
		RTMP_HOST_IO_WRITE32(prChipInfo, u4Register, u4Value);
		return TRUE;
	}

	if (fgIsBusAccessFailed) {
		DBGLOG_LIMITED(HAL, ERROR, "Bus access failed.\n");
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
		if (fgTriggerDebugSop && kalIsResetting()) {
			return kalDevRegWriteViaBT(prGlueInfo,
				u4Register, u4Value);
		}
#endif
		return FALSE;
	}

	prBusInfo = prChipInfo->bus_info;
	if (prHifInfo && !prHifInfo->fgForceReadWriteReg &&
	    prBusInfo->isValidRegAccess &&
	    !prBusInfo->isValidRegAccess(prAdapter, u4Register)) {
		/* Don't print log when resetting */
		if (prAdapter && !wlanIsChipNoAck(prAdapter)) {
			DBGLOG(HAL, ERROR,
			       "Invalid access! Set CR[0x%08x/0x%08x] value[0x%08x]\n",
			       u4Register, u4BusAddr, u4Value);
		}
		KAL_WARN_ON(TRUE);
		return FALSE;
	}

#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
	if ((u4Register >= 0x18050000 && u4Register <= 0x18051000) ||
	    (u4Register >= 0x7c050000 && u4Register <= 0x7c051000) ||
	    (u4Register >= 0x7c000000 && u4Register < 0x7c001000) ||
	    (u4Register >= 0x18000000 && u4Register < 0x18001000)) {
		dump_stack();
		aee_kernel_exception("WLAN",
			"Corrupt conninfra cmdbt:  reg: 0x%08x, val: 0x%08x\n",
			u4Register, u4Value);
	}
#endif

	/* Static mapping */
#if (CFG_WLAN_ATF_SUPPORT == 1)
	if (halChipToStaticMapBusAddr(prChipInfo, u4Register, &u4BusAddr)) {
		kalSendAtfSmcCmd(SMC_WLAN_DEV_REG_WR_CR_OPID,
			prChipInfo->u4CsrOffset + u4BusAddr,
			u4Value, 0);
	} else {
		DBGLOG(INIT, ERROR, "Write CONSYS ERROR 0x%08x=0x%08x.\n",
			u4Register, u4Value);
	}
#else
	if (halChipToStaticMapBusAddr(prChipInfo, u4Register, &u4BusAddr)) {
		RTMP_IO_WRITE32(prChipInfo, u4BusAddr, u4Value);
	} else {
		if (kalDevRegL1Remap(&u4Register))
			kalDevRegL1Write(prGlueInfo, prChipInfo, u4Register, u4Value);
		else
			kalDevRegL2Write(prGlueInfo, prChipInfo, u4Register, u4Value);
	}
#endif

	if (prHifInfo)
		prHifInfo->u4HifCnt++;

	return TRUE;
}

u_int8_t kalDevRegReadRange(struct GLUE_INFO *glue,
	uint32_t reg, void *buf, uint32_t total_size)
{
	struct mt66xx_chip_info *chip_info;
	uint32_t bus_addr = 0;
	u_int8_t ret = TRUE;

	glGetChipInfo((void **)&chip_info);
	if (!chip_info) {
		DBGLOG(INIT, ERROR, "chip info is NULL\n");
		return FALSE;
	}

	if (fgIsBusAccessFailed) {
		DBGLOG_LIMITED(HAL, ERROR, "Bus access failed.\n");
		return FALSE;
	}

	if (halChipToStaticMapBusAddr(chip_info, reg, &bus_addr)) {
		uint32_t offset = 0;

		while (true) {
			uint32_t size = 0;

			if (offset >= total_size)
				break;

			size = ((offset + BUS_REMAP_SIZE) <= total_size ?
				BUS_REMAP_SIZE :
				total_size - offset);

			RTMP_IO_READ_RANGE(chip_info,
				bus_addr,
				(void *)(buf + offset),
				size);

			offset += BUS_REMAP_SIZE;
		}
	} else {
		if (kalDevRegL1Remap(&reg))
			ret = kalDevRegL1ReadRange(glue,
						   chip_info,
						   reg,
						   buf,
						   total_size);
		else
			ret = kalDevRegL2ReadRange(glue,
						   chip_info,
						   reg,
						   buf,
						   total_size);
	}

	return ret;
}

u_int8_t kalDevRegWriteRange(struct GLUE_INFO *glue,
	uint32_t reg, void *buf, uint32_t total_size)
{
	struct mt66xx_chip_info *chip_info;
	uint32_t bus_addr = 0;
	u_int8_t ret = TRUE;

	glGetChipInfo((void **)&chip_info);
	if (!chip_info) {
		DBGLOG(INIT, ERROR, "chip info is NULL\n");
		return FALSE;
	}

	if (fgIsBusAccessFailed) {
		DBGLOG_LIMITED(HAL, ERROR, "Bus access failed.\n");
		return FALSE;
	}

#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
	if ((reg >= 0x18050000 && reg <= 0x18051000) ||
	    (reg >= 0x7c050000 && reg <= 0x7c051000) ||
	    (reg >= 0x7c000000 && reg < 0x7c001000) ||
	    (reg >= 0x18000000 && reg < 0x18001000)) {
		dump_stack();
		aee_kernel_exception("WLAN",
			"Corrupt conninfra cmdbt:  reg: [0x%08x~0x%08x]\n",
			reg, reg + total_size);
	}
#endif

	if (halChipToStaticMapBusAddr(chip_info, reg, &bus_addr)) {
		uint32_t offset = 0;

		while (true) {
			uint32_t size = 0;

			if (offset >= total_size)
				break;

			size = ((offset + BUS_REMAP_SIZE) <= total_size ?
				BUS_REMAP_SIZE :
				total_size - offset);

			RTMP_IO_WRITE_RANGE(chip_info,
				bus_addr,
				(void *)(buf + offset),
				size);

			offset += BUS_REMAP_SIZE;
		}
	} else {
		if (kalDevRegL1Remap(&reg))
			ret = kalDevRegL1WriteRange(glue,
						    chip_info,
						    reg,
						    buf,
						    total_size);
		else
			ret = kalDevRegL2WriteRange(glue,
						    chip_info,
						    reg,
						    buf,
						    total_size);
	}

	return ret;
}

static bool kalWaitRxDmaDone(struct GLUE_INFO *prGlueInfo,
			     struct RTMP_RX_RING *prRxRing,
			     struct RXD_STRUCT *pRxD,
			     uint16_t u2Port)
{
	uint32_t u4Count = 0;
	uint32_t u4CpuIdx = 0;
	struct RTMP_DMACB *pRxCell;
	struct RXD_STRUCT *pCrRxD;
	struct RTMP_DMABUF *prDmaBuf;
	uint32_t u4Size = 0;

	for (u4Count = 0; pRxD->DMADONE == 0; u4Count++) {
		if (u4Count > DMA_DONE_WAITING_COUNT) {
			kalDevRegRead(prGlueInfo, prRxRing->hw_didx_addr,
				      &prRxRing->RxDmaIdx);
			DBGLOG(HAL, INFO,
			       "Rx DMA done P[%u] DMA[%u] CPU[%u]\n",
			       u2Port, prRxRing->RxDmaIdx, prRxRing->RxCpuIdx);

			u4CpuIdx = prRxRing->RxCpuIdx;
			INC_RING_INDEX(u4CpuIdx, prRxRing->u4RingSize);
			if (prRxRing->RxDmaIdx != u4CpuIdx) {
				pRxCell = &prRxRing->Cell[u4CpuIdx];
				pCrRxD = (struct RXD_STRUCT *)pRxCell->AllocVa;
				if (!pCrRxD) {
					DBGLOG(HAL, ERROR,
						"pCrRxD is null\n");
					return false;
				}
				DBGLOG(HAL, INFO, "Rx DMAD[%u]\n", u4CpuIdx);
				DBGLOG_MEM32(HAL, INFO, pCrRxD,
					sizeof(struct RXD_STRUCT));
				u4Size = pCrRxD->SDLen0;
				if (u4Size > CFG_RX_MAX_PKT_SIZE) {
					DBGLOG(RX, ERROR,
						"Rx Data too large[%u]\n",
						u4Size);
				} else {
					DBGLOG(HAL, INFO,
						"RXD+Data[%u] len[%u]\n",
						u4CpuIdx, u4Size);
					prDmaBuf = &pRxCell->DmaBuf;
					DBGLOG_MEM32(HAL, INFO,
						prDmaBuf->AllocVa, u4Size);
				}
			}

			return false;
		}

		kalUdelay(DMA_DONE_WAITING_TIME);
	}
	return true;
}

#if HIF_INT_TIME_DEBUG
static void kalTrackRxReadyTime(struct GLUE_INFO *prGlueInfo, uint16_t u2Port)
{
	struct BUS_INFO *prBusInfo =
		prGlueInfo->prAdapter->chip_info->bus_info;
	struct timespec64 rNowTs, rTime;

	ktime_get_ts64(&rNowTs);
	if (prBusInfo->u4EnHifIntTs &&
	    halGetDeltaTime(&rNowTs, &prBusInfo->rHifIntTs, &rTime)) {
		DBGLOG(HAL, INFO,
#if KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
		       "RX[%u] done bit ready time[%lld.%.9ld] cnt[%d]\n",
#else
		       "RX[%u] done bit ready time[%lld.%.6ld] cnt[%d]\n",
#endif
		       u2Port,
		       (long long)rTime.tv_sec,
		       KAL_GET_TIME_OF_USEC_OR_NSEC(rTime),
		       prBusInfo->u4HifIntTsCnt);
		prBusInfo->u4EnHifIntTs = 0;
		prBusInfo->u4HifIntTsCnt = 0;
	}
}
#endif /* HIF_INT_TIME_DEBUG */

/*----------------------------------------------------------------------------*/
/*!
 * \brief Read device I/O port
 *
 * \param[in] prGlueInfo         Pointer to the GLUE_INFO_T structure.
 * \param[in] u2Port             I/O port offset
 * \param[in] u2Len              Length to be read
 * \param[out] pucBuf            Pointer to read buffer
 * \param[in] u2ValidOutBufSize  Length of the buffer valid to be accessed
 *
 * \retval TRUE          operation success
 * \retval FALSE         operation fail
 */
/*----------------------------------------------------------------------------*/
u_int8_t kalDevPortRead(struct GLUE_INFO *prGlueInfo,
	uint16_t u2Port, uint32_t u4Len,
	uint8_t *pucBuf, uint32_t u4ValidOutBufSize, u_int8_t isPollMode)
{
	struct ADAPTER *prAdapter = NULL;
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct HIF_MEM_OPS *prMemOps;
	struct RTMP_RX_RING *prRxRing;
	struct RTMP_DMACB *pRxCell;
	struct RXD_STRUCT *pRxD;
	struct RTMP_DMABUF *prDmaBuf;
	u_int8_t fgRet = TRUE;
	uint32_t u4CpuIdx = 0;

	ASSERT(prGlueInfo);
	ASSERT(pucBuf);
	ASSERT(u4Len <= u4ValidOutBufSize);

	prAdapter = prGlueInfo->prAdapter;
	prHifInfo = &prGlueInfo->rHifInfo;
	prMemOps = &prHifInfo->rMemOps;
	prRxRing = &prHifInfo->RxRing[u2Port];

	u4CpuIdx = prRxRing->RxCpuIdx;
	INC_RING_INDEX(u4CpuIdx, prRxRing->u4RingSize);

	pRxCell = &prRxRing->Cell[u4CpuIdx];
	pRxD = (struct RXD_STRUCT *)pRxCell->AllocVa;
	prDmaBuf = &pRxCell->DmaBuf;

	prRxRing->u4LastRxEventWaitDmaDoneCnt =
		halWpdmaGetRxDmaDoneCnt(prGlueInfo, u2Port);
	if (prRxRing->u4LastRxEventWaitDmaDoneCnt == 0)
		return FALSE;

	if (!kalWaitRxDmaDone(prGlueInfo, prRxRing, pRxD, u2Port)) {
		if (!prRxRing->fgIsDumpLog) {
			DBGLOG(HAL, ERROR, "RX Done bit not ready(PortRead)\n");
		}
		prRxRing->fgIsDumpLog = true;
		prRxRing->fgIsWaitRxDmaDoneTimeout = true;
		return FALSE;
	} else
		prRxRing->fgIsWaitRxDmaDoneTimeout = false;

#if HIF_INT_TIME_DEBUG
	kalTrackRxReadyTime(prGlueInfo, u2Port);
#endif

	if (pRxD->LastSec0 == 0 || prRxRing->fgRxSegPkt) {
		/* Rx segmented packet */
		DBGLOG(HAL, WARN,
			"Skip Rx segmented packet, SDL0[%u] LS0[%u]\n",
			pRxD->SDLen0, pRxD->LastSec0);
		if (pRxD->LastSec0 == 1) {
			/* Last segmented packet */
			prRxRing->fgRxSegPkt = FALSE;
		} else {
			/* Segmented packet */
			prRxRing->fgRxSegPkt = TRUE;
		}

		fgRet = FALSE;
		goto skip;
	}

	if (pRxD->SDLen0 > u4Len || prAdapter->rWifiVar.fgDumpRxEvt) {
		uint8_t *prBuffer = NULL;
		uint32_t u4dumpSize = 0;

		if (pRxD->SDLen0 > u4Len) {
			DBGLOG(HAL, WARN,
				"Skip Rx packet, SDL0[%u] > SwRfb max len[%u]\n",
				pRxD->SDLen0, u4Len);
		}
		DBGLOG(RX, ERROR, "Dump RX Event RxD\n");
		dumpMemory8((uint8_t *)pRxD, sizeof(struct RXD_STRUCT));
		u4dumpSize = pRxD->SDLen0;
		if (u4dumpSize > BITS(0, 13))
			u4dumpSize = BITS(0, 13);
		prBuffer = kalMemAlloc(u4dumpSize, VIR_MEM_TYPE);
		if (prBuffer) {
			if (prMemOps->copyEvent &&
			    prMemOps->copyEvent(prHifInfo, pRxCell, pRxD,
						prDmaBuf, prBuffer,
						u4dumpSize)) {
				DBGLOG(RX, ERROR, "Dump RX Event payload\n");
				DBGLOG_MEM8(RX, ERROR, prBuffer,
						u4dumpSize);
			}
			kalMemFree(prBuffer, VIR_MEM_TYPE, sizeof(prBuffer));
		}
		if (pRxD->SDLen0 > u4Len)
			goto skip;
	}

	NIC_DUMP_RXDMAD_HEADER(prAdapter, "Dump RXDMAD:\n");
	NIC_DUMP_RXDMAD(prAdapter, (uint8_t *)pRxD, sizeof(struct RXD_STRUCT));

	if (prMemOps->copyEvent &&
	    !prMemOps->copyEvent(prHifInfo, pRxCell, pRxD,
				 prDmaBuf, pucBuf, u4Len)) {
		ASSERT(0);
		return FALSE;
	}

	if (isPollMode) {
		struct WIFI_EVENT *prEvent = (struct WIFI_EVENT *)
			(pucBuf + prAdapter->chip_info->rxd_size);

		if ((prEvent->u2PacketLength == 0) &&
		    (pRxD->SDLen0 != prEvent->u2PacketLength)) {
			DBGLOG(RX, ERROR, "Dump RX Event payload len[%d]\n",
			       pRxD->SDLen0);
			DBGLOG_MEM8(RX, ERROR, pucBuf, pRxD->SDLen0);
			return FALSE;
		}
	}

	pRxD->SDPtr0 = (uint64_t)prDmaBuf->AllocPa & DMA_LOWER_32BITS_MASK;
#ifdef CONFIG_PHYS_ADDR_T_64BIT
	pRxD->SDPtr1 = ((uint64_t)prDmaBuf->AllocPa >> DMA_BITS_OFFSET) &
		DMA_HIGHER_4BITS_MASK;
#else
	pRxD->SDPtr1 = 0;
#endif
skip:
	pRxD->SDLen0 = prRxRing->u4BufSize;
	pRxD->DMADONE = 0;

	prRxRing->RxCpuIdx = u4CpuIdx;
	kalDevRegWrite(prGlueInfo, prRxRing->hw_cidx_addr, prRxRing->RxCpuIdx);
	prRxRing->fgIsDumpLog = false;

	GLUE_INC_REF_CNT(prGlueInfo->prAdapter->rHifStats.u4EventRxCount);

	return fgRet;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Write device I/O port
 *
 * \param[in] prGlueInfo         Pointer to the GLUE_INFO_T structure.
 * \param[in] u2Port             I/O port offset
 * \param[in] u2Len              Length to be write
 * \param[in] pucBuf             Pointer to write buffer
 * \param[in] u2ValidInBufSize   Length of the buffer valid to be accessed
 *
 * \retval TRUE          operation success
 * \retval FALSE         operation fail
 */
/*----------------------------------------------------------------------------*/
u_int8_t
kalDevPortWrite(struct GLUE_INFO *prGlueInfo,
	uint16_t u2Port, uint32_t u4Len, uint8_t *pucBuf,
	uint32_t u4ValidInBufSize)
{
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct HIF_MEM_OPS *prMemOps;
	struct RTMP_TX_RING *prTxRing;
	struct RTMP_DMACB *pTxCell;
	struct TXD_STRUCT *pTxD;
	struct mt66xx_chip_info *prChipInfo;
	void *pucDst = NULL;
	struct ADAPTER *prAdapter;

	ASSERT(prGlueInfo);
	ASSERT(pucBuf);
	ASSERT(u4Len <= u4ValidInBufSize);

	prHifInfo = &prGlueInfo->rHifInfo;
	prChipInfo = prGlueInfo->prAdapter->chip_info;
	prMemOps = &prHifInfo->rMemOps;
	prTxRing = &prHifInfo->TxRing[u2Port];
	prAdapter = prGlueInfo->prAdapter;

	if (prTxRing->u4UsedCnt + 1 >= prTxRing->u4RingSize) {
		DBGLOG(HAL, TRACE, "Force recycle port %d DMA resource.\n",
			u2Port);
		halWpdmaProcessCmdDmaDone(prGlueInfo, u2Port);
	}

	if (prTxRing->u4UsedCnt + 1 >= prTxRing->u4RingSize) {
		DBGLOG(HAL, ERROR, "Port %d TX resource is NOT enough.\n",
			u2Port);
		return FALSE;
	}

	if (prMemOps->allocRuntimeMem)
		pucDst = prMemOps->allocRuntimeMem(u4Len);

	if (prTxRing->TxCpuIdx >= prTxRing->u4RingSize) {
		DBGLOG(HAL, ERROR, "Error TxCpuIdx[%u]\n", prTxRing->TxCpuIdx);
		if (prMemOps->freeBuf)
			prMemOps->freeBuf(pucDst, u4Len);
		return FALSE;
	}

	pTxCell = &prTxRing->Cell[prTxRing->TxCpuIdx];
	pTxD = (struct TXD_STRUCT *)pTxCell->AllocVa;

	pTxCell->pPacket = NULL;
	pTxCell->pBuffer = pucDst;

	if (prMemOps->copyCmd &&
	    !prMemOps->copyCmd(prHifInfo, pTxCell, pucDst,
			       pucBuf, u4Len, NULL, 0)) {
		if (prMemOps->freeBuf)
			prMemOps->freeBuf(pucDst, u4Len);
		ASSERT(0);
		return FALSE;
	}

	pTxD->LastSec0 = 1;
	pTxD->LastSec1 = 0;
	pTxD->SDLen0 = u4Len;
	pTxD->SDLen1 = 0;
	pTxD->SDPtr0 = (uint64_t)pTxCell->PacketPa & DMA_LOWER_32BITS_MASK;
#ifdef CONFIG_PHYS_ADDR_T_64BIT
	pTxD->SDPtr0Ext = ((uint64_t)pTxCell->PacketPa >> DMA_BITS_OFFSET) &
		DMA_HIGHER_4BITS_MASK;
#else
	pTxD->SDPtr0Ext = 0;
#endif
	pTxD->SDPtr1 = 0;
	pTxD->Burst = 0;
	pTxD->DMADONE = 0;

	if (u2Port == prChipInfo->u2TxInitCmdPort) {
		NIC_DUMP_TXDMAD_HEADER(prAdapter, "Dump CMD TXDMAD:\n");
		NIC_DUMP_TXDMAD(prAdapter,
				(uint8_t *)pTxD, sizeof(struct TXD_STRUCT));

		NIC_DUMP_TXD_HEADER(prAdapter, "Dump CMD TXD:\n");
		NIC_DUMP_TXD(prAdapter, (uint8_t *)pucBuf, u4Len);
	}

	/* Increase TX_CTX_IDX, but write to register later. */
	INC_RING_INDEX(prTxRing->TxCpuIdx, prTxRing->u4RingSize);

	prTxRing->u4UsedCnt++;

	kalDevRegWrite(prGlueInfo, prTxRing->hw_cidx_addr, prTxRing->TxCpuIdx);

	GLUE_INC_REF_CNT(prGlueInfo->prAdapter->rHifStats.u4CmdTxCount);

	return TRUE;
}

void kalDevReadIntStatus(struct ADAPTER *prAdapter,
	uint32_t *pu4IntStatus)
{
	uint32_t u4RegValue = 0;
	struct GL_HIF_INFO *prHifInfo = &prAdapter->prGlueInfo->rHifInfo;

	*pu4IntStatus = 0;

	HAL_MCR_RD(prAdapter, WPDMA_INT_STA, &u4RegValue);

	if (HAL_IS_RX_DONE_INTR(u4RegValue))
		*pu4IntStatus |= WHISR_RX0_DONE_INT;

	if (HAL_IS_TX_DONE_INTR(u4RegValue))
		*pu4IntStatus |= WHISR_TX_DONE_INT;

	if (u4RegValue & CONNAC_MCU_SW_INT)
		*pu4IntStatus |= WHISR_D2H_SW_INT;

	prHifInfo->u4IntStatus = u4RegValue;

	/* clear interrupt */
	HAL_MCR_WR(prAdapter, WPDMA_INT_STA, u4RegValue);

}

enum ENUM_CMD_TX_RESULT kalDevWriteCmd(struct GLUE_INFO *prGlueInfo,
	struct CMD_INFO *prCmdInfo, uint8_t ucTC)
{
	struct GL_HIF_INFO *prHifInfo = NULL;

	ASSERT(prGlueInfo);
	prHifInfo = &prGlueInfo->rHifInfo;

	if (nicSerIsTxStop(prGlueInfo->prAdapter))
		return kalDevWriteCmdByQueue(prGlueInfo, prCmdInfo, ucTC);

	return halWpdmaWriteCmd(prGlueInfo, prCmdInfo, ucTC);
}

static struct TX_CMD_REQ *kalCloneCmd(struct GLUE_INFO *prGlueInfo,
				      struct CMD_INFO *prCmdInfo)
{
	struct GL_HIF_INFO *prHifInfo;
	struct list_head *prNode;
	struct TX_CMD_REQ *prCmdReq;
	uint32_t u4Size;
	unsigned long flags;
	uint8_t *aucBuff;

	prHifInfo = &prGlueInfo->rHifInfo;

	if (list_empty(&prHifInfo->rTxCmdFreeList)) {
		DBGLOG(HAL, ERROR, "tx cmd free list is empty\n");
		return NULL;
	}

	spin_lock_irqsave(&prHifInfo->rTxCmdQLock, flags);
	prNode = prHifInfo->rTxCmdFreeList.next;
	list_del(prNode);
	spin_unlock_irqrestore(&prHifInfo->rTxCmdQLock, flags);

	prCmdReq = list_entry(prNode, struct TX_CMD_REQ, list);
	kalMemCopy(&prCmdReq->rCmdInfo, prCmdInfo, sizeof(struct CMD_INFO));

	aucBuff = prCmdReq->aucBuff;
	u4Size = prCmdInfo->u4TxdLen;
	if (u4Size > TX_BUFFER_NORMSIZE)
		u4Size = TX_BUFFER_NORMSIZE;
	kalMemCopy(aucBuff, prCmdInfo->pucTxd, u4Size);
	prCmdReq->rCmdInfo.pucTxd = aucBuff;
	aucBuff += u4Size;

	u4Size = prCmdInfo->u4TxpLen;
	if ((u4Size + prCmdInfo->u4TxdLen) > TX_BUFFER_NORMSIZE)
		u4Size = TX_BUFFER_NORMSIZE - prCmdInfo->u4TxdLen;
	if (!aucBuff)
		kalMemCopy(aucBuff, prCmdInfo->pucTxp, u4Size);
	prCmdReq->rCmdInfo.pucTxp = aucBuff;

	return prCmdReq;
}

static enum ENUM_CMD_TX_RESULT kalDevWriteCmdByQueue(
		struct GLUE_INFO *prGlueInfo, struct CMD_INFO *prCmdInfo,
		uint8_t ucTC)
{
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct TX_CMD_REQ *prTxReq;
	unsigned long flags;

	ASSERT(prGlueInfo);
	prHifInfo = &prGlueInfo->rHifInfo;

	prTxReq = kalCloneCmd(prGlueInfo, prCmdInfo);
	if (!prTxReq) {
		DBGLOG(HAL, ERROR, "alloc TxCmdReq fail!");
		return CMD_TX_RESULT_FAILED;
	}
	prTxReq->ucTC = ucTC;
	spin_lock_irqsave(&prHifInfo->rTxCmdQLock, flags);
	list_add_tail(&prTxReq->list, &prHifInfo->rTxCmdQ);
	spin_unlock_irqrestore(&prHifInfo->rTxCmdQLock, flags);

	return CMD_TX_RESULT_QUEUED;
}

bool kalDevKickCmd(struct GLUE_INFO *prGlueInfo)
{
	struct ADAPTER *prAdapter = NULL;
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct list_head *prCur, *prNext;
	struct TX_CMD_REQ *prTxReq;
	enum ENUM_CMD_TX_RESULT ret;
	unsigned long flags;
	struct list_head rTempQ;

	ASSERT(prGlueInfo);
	prHifInfo = &prGlueInfo->rHifInfo;
	prAdapter = prGlueInfo->prAdapter;

	spin_lock_irqsave(&prHifInfo->rTxCmdQLock, flags);
	list_replace_init(&prHifInfo->rTxCmdQ, &rTempQ);
	spin_unlock_irqrestore(&prHifInfo->rTxCmdQLock, flags);

	list_for_each_safe(prCur, prNext, &rTempQ) {
		prTxReq = list_entry(prCur, struct TX_CMD_REQ, list);
		ret = halWpdmaWriteCmd(prGlueInfo,
				       &prTxReq->rCmdInfo, prTxReq->ucTC);
		if (ret != CMD_TX_RESULT_SUCCESS)
			DBGLOG(HAL, ERROR, "ret: %d\n", ret);
		list_del(prCur);
		list_add_tail(prCur, &prHifInfo->rTxCmdFreeList);
	}

	return true;
}

static uint8_t kalGetSwAmsduNum(struct GLUE_INFO *prGlueInfo,
				struct MSDU_INFO *prMsduInfo)
{
	struct ADAPTER *prAdapter;
	struct STA_RECORD *prStaRec;
	uint8_t ucTid, ucStaRecIndex;
	struct TX_DESC_OPS_T *prTxDescOps;

	ASSERT(prGlueInfo);
	ASSERT(prMsduInfo);

	prAdapter = prGlueInfo->prAdapter;
	prTxDescOps = prAdapter->chip_info->prTxDescOps;

	ucTid = prMsduInfo->ucUserPriority;
	ucStaRecIndex = prMsduInfo->ucStaRecIndex;
	if (ucStaRecIndex >= CFG_STA_REC_NUM || ucTid >= TX_DESC_TID_NUM)
		return 0;

	prStaRec = cnmGetStaRecByIndex(prAdapter, ucStaRecIndex);
	if (!prStaRec || !(prStaRec->ucAmsduEnBitmap & BIT(ucTid)))
		return 0;

	return prStaRec->ucMaxMpduCount;
}

void kalBhDisable(struct GLUE_INFO *prGlueInfo)
{
	if (!HAL_IS_TX_DIRECT(prGlueInfo->prAdapter) &&
		!HAL_IS_RX_DIRECT(prGlueInfo->prAdapter))
		return;

	local_bh_disable();
}

void kalBhEnable(struct GLUE_INFO *prGlueInfo)
{
	if (!HAL_IS_TX_DIRECT(prGlueInfo->prAdapter) &&
		!HAL_IS_RX_DIRECT(prGlueInfo->prAdapter))
		return;

	local_bh_enable();
}

void kalAcquireHifTxDataQLock(struct GL_HIF_INFO *prHifInfo,
		uint32_t u4Port,
		unsigned long *plHifTxDataQFlags)
{
	unsigned long ulHifTxDataQFlags = 0;

	if (!HAL_IS_TX_DIRECT(prGlueInfo->prAdapter) &&
		!HAL_IS_RX_DIRECT(prGlueInfo->prAdapter))
		return;

	if (unlikely(prHifInfo == NULL)) {
		DBGLOG(INIT, ERROR, "prHifInfo is NULL\n");
		return;
	}

	spin_lock_irqsave(&prHifInfo->rTxDataQLock[u4Port],
			ulHifTxDataQFlags);
	*plHifTxDataQFlags = ulHifTxDataQFlags;
}

void kalReleaseHifTxDataQLock(struct GL_HIF_INFO *prHifInfo,
		uint32_t u4Port,
		unsigned long ulHifTxDataQFlags)
{
	if (!HAL_IS_TX_DIRECT(prGlueInfo->prAdapter) &&
		!HAL_IS_RX_DIRECT(prGlueInfo->prAdapter))
		return;


	if (unlikely(prHifInfo == NULL)) {
		DBGLOG(INIT, ERROR, "prHifInfo is NULL\n");
		return;
	}

	spin_unlock_irqrestore(&prHifInfo->rTxDataQLock[u4Port],
			ulHifTxDataQFlags);
}

void kalAcquireHifTxRingLock(struct RTMP_TX_RING *prTxRing,
		unsigned long *plHifTxRingFlags)
{
	unsigned long ulHifTxRingFlags = 0;

	if (!HAL_IS_TX_DIRECT(prGlueInfo->prAdapter) &&
		!HAL_IS_RX_DIRECT(prGlueInfo->prAdapter))
		return;

	if (unlikely(prTxRing == NULL)) {
		DBGLOG(INIT, ERROR, "prTxRing is NULL\n");
		return;
	}

	spin_lock_irqsave(&prTxRing->rTxDmaQLock,
			ulHifTxRingFlags);
	*plHifTxRingFlags = ulHifTxRingFlags;
}

void kalReleaseHifTxRingLock(struct RTMP_TX_RING *prTxRing,
		unsigned long ulHifTxRingFlags)
{
	if (!HAL_IS_TX_DIRECT(prGlueInfo->prAdapter) &&
		!HAL_IS_RX_DIRECT(prGlueInfo->prAdapter))
		return;

	if (unlikely(prTxRing == NULL)) {
		DBGLOG(INIT, ERROR, "prTxRing is NULL\n");
		return;
	}

	spin_unlock_irqrestore(&prTxRing->rTxDmaQLock,
		ulHifTxRingFlags);
}

void kalAcquireHifOwnLock(struct ADAPTER *prAdapter)
{

	/* if direct trx,  set drv/fw own will be called
	*  in softirq/tasklet/thread context,
	*  if normal trx, set drv/fw own will only
	*  be called in thread context
	*/
#if !CFG_SUPPORT_RX_WORK
	if (HAL_IS_TX_DIRECT(prAdapter) || HAL_IS_RX_DIRECT(prAdapter)) {
		spin_lock_bh(
			&prAdapter->prGlueInfo->rSpinLock[
			SPIN_LOCK_SET_OWN]);
	} else
#endif /* !CFG_SUPPORT_RX_WORK */
		KAL_ACQUIRE_MUTEX(prAdapter, MUTEX_SET_OWN);
}

void kalReleaseHifOwnLock(struct ADAPTER *prAdapter)
{
#if !CFG_SUPPORT_RX_WORK
	if (HAL_IS_TX_DIRECT(prAdapter) || HAL_IS_RX_DIRECT(prAdapter)) {
		spin_unlock_bh(
			&prAdapter->prGlueInfo->rSpinLock[
			SPIN_LOCK_SET_OWN]);
	} else
#endif /* !CFG_SUPPORT_RX_WORK */
		KAL_RELEASE_MUTEX(prAdapter, MUTEX_SET_OWN);
}

u_int8_t kalDevWriteData(struct GLUE_INFO *prGlueInfo,
	struct MSDU_INFO *prMsduInfo)
{
	ASSERT(prGlueInfo);

	return kalDevWriteDataByQueue(prGlueInfo, prMsduInfo);
}

static bool kalDevWriteDataByQueue(struct GLUE_INFO *prGlueInfo,
				   struct MSDU_INFO *prMsduInfo)
{
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct TX_DATA_REQ *prTxReq;
	uint32_t u4Port = 0;

	KAL_HIF_TXDATAQ_LOCK_DECLARATION();

	ASSERT(prGlueInfo);
	prHifInfo = &prGlueInfo->rHifInfo;

#if (CFG_SUPPORT_TX_DATA_DELAY == 1)
	/* force tx data */
	if (prMsduInfo->pfTxDoneHandler)
		KAL_SET_BIT(HIF_TX_DATA_DELAY_TIMEOUT_BIT,
			    prHifInfo->ulTxDataTimeout);
#endif /* (CFG_SUPPORT_TX_DATA_DELAY == 1) */

	u4Port = halTxRingDataSelect(prGlueInfo->prAdapter, prMsduInfo);
	prTxReq = &prMsduInfo->rTxReq;
	prTxReq->prMsduInfo = prMsduInfo;

	KAL_HIF_TXDATAQ_LOCK(prHifInfo, u4Port);
	list_add_tail(&prTxReq->list, &prHifInfo->rTxDataQ[u4Port]);
	prHifInfo->u4TxDataQLen[u4Port]++;
	KAL_HIF_TXDATAQ_UNLOCK(prHifInfo, u4Port);

	return true;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Kick Tx data to device
 *
 * \param[in] prGlueInfo         Pointer to the GLUE_INFO_T structure.
 *
 * \retval TRUE          operation success
 * \retval FALSE         operation fail
 */
/*----------------------------------------------------------------------------*/
u_int8_t kalDevKickData(struct GLUE_INFO *prGlueInfo)
{
	struct mt66xx_chip_info *prChipInfo;
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct WIFI_VAR *prWifiVar;
	struct RTMP_TX_RING *prTxRing;
	struct list_head rTempList;
	static int32_t ai4RingLock[NUM_OF_TX_RING];
	uint32_t u4Idx;
#if (CFG_SUPPORT_TX_DATA_DELAY == 1)
	uint32_t u4DataCnt = 0;
#endif

	KAL_HIF_TXDATAQ_LOCK_DECLARATION();
#if !CFG_TX_DIRECT_VIA_HIF_THREAD
	KAL_HIF_TXRING_LOCK_DECLARATION();
#endif /* !CFG_TX_DIRECT_VIA_HIF_THREAD */

	ASSERT(prGlueInfo);

	prChipInfo = prGlueInfo->prAdapter->chip_info;
	prHifInfo = &prGlueInfo->rHifInfo;
	prWifiVar = &prGlueInfo->prAdapter->rWifiVar;

#if (CFG_SUPPORT_TX_DATA_DELAY == 1)
	if (prGlueInfo->prAdapter->fgEnLowLatencyMode ||
		wlanWfdEnabled(prGlueInfo->prAdapter))
		goto tx_data;

	if (KAL_TEST_AND_CLEAR_BIT(
		    HIF_TX_DATA_DELAY_TIMEOUT_BIT,
		    prHifInfo->ulTxDataTimeout))
		goto tx_data;

	for (u4Idx = 0; u4Idx < NUM_OF_TX_RING; u4Idx++)
		u4DataCnt += prHifInfo->u4TxDataQLen[u4Idx];
	if (u4DataCnt >= prWifiVar->u4TxDataDelayCnt)
		goto tx_data;

	halStartTxDelayTimer(prGlueInfo->prAdapter);
	return 0;
tx_data:
#endif

#if !CFG_SUPPORT_RX_WORK
	/* disable softirq to improve processing efficiency */
	KAL_HIF_BH_DISABLE(prGlueInfo);
#endif /* !CFG_SUPPORT_RX_WORK */

	for (u4Idx = 0; u4Idx < NUM_OF_TX_RING; u4Idx++) {
		if (!halIsDataRing(TX_RING, u4Idx))
			continue;

		if (unlikely(GLUE_INC_REF_CNT(ai4RingLock[u4Idx]) > 1)) {
			/* Single user allowed per port read */
			DBGLOG(TX, WARN, "Single user only R[%u] [%d]\n",
				u4Idx,
				GLUE_GET_REF_CNT(ai4RingLock[u4Idx]));
			goto end;
		}

		if (list_empty(&prHifInfo->rTxDataQ[u4Idx]))
			goto end;

		/* move list entry of rTxDataQ to rTempList */
		INIT_LIST_HEAD(&rTempList);
		KAL_HIF_TXDATAQ_LOCK(prHifInfo, u4Idx);
		list_replace_init(&prHifInfo->rTxDataQ[u4Idx], &rTempList);
		KAL_HIF_TXDATAQ_UNLOCK(prHifInfo, u4Idx);

		prTxRing = &prHifInfo->TxRing[u4Idx];

#if !CFG_TX_DIRECT_VIA_HIF_THREAD
		KAL_HIF_TXRING_LOCK(prTxRing);
#endif /* !CFG_TX_DIRECT_VIA_HIF_THREAD */

		if (prChipInfo->ucMaxSwAmsduNum > 1)
			kalDevKickAmsduData(prGlueInfo, &rTempList);
		else
			kalDevKickMsduData(prGlueInfo, &rTempList);
		kalDevRegWrite(prGlueInfo, prTxRing->hw_cidx_addr,
			       prTxRing->TxCpuIdx);

#if !CFG_TX_DIRECT_VIA_HIF_THREAD
		KAL_HIF_TXRING_UNLOCK(prTxRing);
#endif /* !CFG_TX_DIRECT_VIA_HIF_THREAD */

		/* move list entries of rTempList to rTxDataQ */
		if (list_empty(&rTempList))
			goto end;

		KAL_HIF_TXDATAQ_LOCK(prHifInfo, u4Idx);
		list_splice_tail_init(&prHifInfo->rTxDataQ[u4Idx],
			&rTempList);
		list_replace(&rTempList, &prHifInfo->rTxDataQ[u4Idx]);
		KAL_HIF_TXDATAQ_UNLOCK(prHifInfo, u4Idx);
end:
		GLUE_DEC_REF_CNT(ai4RingLock[u4Idx]);
	}

#if !CFG_SUPPORT_RX_WORK
	KAL_HIF_BH_ENABLE(prGlueInfo);
#endif /* !CFG_SUPPORT_RX_WORK */

#if (CFG_SUPPORT_TX_DATA_DELAY == 1)
	del_timer_sync(&prHifInfo->rTxDelayTimer);
	KAL_CLR_BIT(HIF_TX_DATA_DELAY_TIMER_RUNNING_BIT,
		    prHifInfo->ulTxDataTimeout);
#endif

	return 0;
}

static uint16_t kalGetPaddingSize(uint16_t u2TxByteCount)
{
	uint16_t u2Size = 0;

	if (u2TxByteCount & 3)
		u2Size = 4 - (u2TxByteCount & 3);
	return u2Size;
}

static uint16_t kalGetMoreSizeForAmsdu(uint32_t u4TxdDW1)
{
	/*
	 * ETYPE=0/VLAN=0/RMVL=X PLlength = PL length
	 * ETYPE=0/VLAN=1/RMVL=1 PLlength = PL length - 4
	 * ETYPE=0/VLAN=1/RMVL=0 PLlength = PL length + 6
	 * ETYPE=1/VLAN=0/RMVL=X PLlength = PL length + 8
	 * ETYPE=1/VLAN=1/RMVL=1 PLlength = PL length + 4
	 * ETYPE=1/VLAN=1/RMVL=0 PLlength = PL length + 14
	 */
	uint16_t u2Size = 0;

	if (u4TxdDW1 & TXD_DW1_ETYP)
		u2Size += 8;
	if (u4TxdDW1 & TXD_DW1_VLAN) {
		if (u4TxdDW1 & TXD_DW1_RMVL) {
			if (u2Size >= 4)
				u2Size -= 4;
		} else
			u2Size += 6;
	}
	return u2Size;
}

static bool kalDevKickMsduData(struct GLUE_INFO *prGlueInfo,
				struct list_head *prHead)
{
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct BUS_INFO *prBusInfo = NULL;
	struct list_head *prCur, *prNext;
	struct TX_DATA_REQ *prTxReq;
	struct MSDU_INFO *prMsduInfo;
	bool fgRet = true;

	ASSERT(prGlueInfo);

	prHifInfo = &prGlueInfo->rHifInfo;
	prBusInfo = prGlueInfo->prAdapter->chip_info->bus_info;

	list_for_each_safe(prCur, prNext, prHead) {
		prTxReq = list_entry(prCur, struct TX_DATA_REQ, list);
		prMsduInfo = prTxReq->prMsduInfo;
		if (!prMsduInfo ||
		    !halWpdmaWriteMsdu(prGlueInfo, prMsduInfo, prCur)) {
			fgRet = false;
			break;
		}
	}

	return fgRet;
}

#if KERNEL_VERSION(5, 10, 70) <= LINUX_VERSION_CODE && \
	!defined(CONFIG_SUPPORT_OPENWRT)
static int kalAmsduTxDCmp(void *prPriv, const struct list_head *prList1,
			  const struct list_head *prList2)
#else
static int kalAmsduTxDCmp(void *prPriv, struct list_head *prList1,
			  struct list_head *prList2)
#endif
{
	struct TX_DATA_REQ *prTxReq1, *prTxReq2;
	struct sk_buff *prSkb1, *prSkb2;
	struct AMSDU_MAC_TX_DESC *prTxD1, *prTxD2;

	prTxReq1 = list_entry(prList1, struct TX_DATA_REQ, list);
	prTxReq2 = list_entry(prList2, struct TX_DATA_REQ, list);
	prSkb1 = (struct sk_buff *)prTxReq1->prMsduInfo->prPacket;
	prSkb2 = (struct sk_buff *)prTxReq2->prMsduInfo->prPacket;
	prTxD1 = (struct AMSDU_MAC_TX_DESC *)prSkb1->data;
	prTxD2 = (struct AMSDU_MAC_TX_DESC *)prSkb2->data;

	if (prTxD1->u2DW0 != prTxD2->u2DW0)
		return prTxD2->u2DW0 - prTxD1->u2DW0;

	return prTxD1->u4DW1 - prTxD2->u4DW1;
}

static bool kalIsAggregatedMsdu(struct GLUE_INFO *prGlueInfo,
				struct MSDU_INFO *prMsduInfo)
{
	struct sk_buff *prSkb;
	struct AMSDU_MAC_TX_DESC *prTxD;

	prSkb = (struct sk_buff *)prMsduInfo->prPacket;
	prTxD = (struct AMSDU_MAC_TX_DESC *)prSkb->data;

	if (prTxD->u4FR || prTxD->u4TXS)
		return false;
	return true;
}

static uint32_t kalGetNumOfAmsdu(struct GLUE_INFO *prGlueInfo,
				 struct list_head *prTarget,
				 struct list_head *prHead,
				 uint16_t *pu2Size)
{
	struct TX_DATA_REQ *prTxReq;
	struct MSDU_INFO *prMsduInfo;
	struct sk_buff *prSkb;
	struct AMSDU_MAC_TX_DESC *prTxD;
	struct STA_RECORD *prStaRec;
	struct list_head *prCur;
	uint16_t u2TotalSize, u2Size;
	uint32_t u4Cnt = 1;
	uint8_t ucStaRecIndex;

	prTxReq = list_entry(prTarget, struct TX_DATA_REQ, list);
	prMsduInfo = prTxReq->prMsduInfo;
	prSkb = (struct sk_buff *)prMsduInfo->prPacket;
	prTxD = (struct AMSDU_MAC_TX_DESC *)prSkb->data;

	ucStaRecIndex = prMsduInfo->ucStaRecIndex;
	prStaRec = cnmGetStaRecByIndex(prGlueInfo->prAdapter, ucStaRecIndex);
	if (!prStaRec)
		return 1;

	u2TotalSize = NIC_TX_DESC_LONG_FORMAT_LENGTH;
	u2TotalSize += prMsduInfo->u2FrameLength;
	u2TotalSize += kalGetMoreSizeForAmsdu(prTxD->u4DW1);

	for (prCur = prTarget->next; prCur != prHead; prCur = prCur->next) {
		if (u4Cnt >= kalGetSwAmsduNum(prGlueInfo, prMsduInfo) ||
		    kalAmsduTxDCmp((void *)prGlueInfo, prTarget, prCur))
			break;

		prTxReq = list_entry(prCur, struct TX_DATA_REQ, list);
		prMsduInfo = prTxReq->prMsduInfo;
		prSkb = (struct sk_buff *)prMsduInfo->prPacket;
		prTxD = (struct AMSDU_MAC_TX_DESC *)prSkb->data;

		u2Size = prMsduInfo->u2FrameLength;
		u2Size += kalGetMoreSizeForAmsdu(prTxD->u4DW1);
		u2Size += kalGetPaddingSize(u2TotalSize);
		if ((u2TotalSize + u2Size) > prStaRec->u4MaxMpduLen)
			break;

		u2TotalSize += u2Size;
		u4Cnt++;
	}

	if (u2TotalSize < prStaRec->u4MinMpduLen)
		return 1;

	*pu2Size = u2TotalSize;
	return u4Cnt;
}

static bool kalDevKickAmsduData(struct GLUE_INFO *prGlueInfo,
				struct list_head *prHead)
{
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct BUS_INFO *prBusInfo = NULL;
	struct list_head *prCur, *prNext;
	struct TX_DATA_REQ *prTxReq;
	struct MSDU_INFO *prMsduInfo;
	uint32_t u4Num = 0, u4Idx;
	uint16_t u2Size = 0;
	bool fgRet = true;

	ASSERT(prGlueInfo);

	prHifInfo = &prGlueInfo->rHifInfo;
	prBusInfo = prGlueInfo->prAdapter->chip_info->bus_info;

	list_for_each_safe(prCur, prNext, prHead) {
		prTxReq = list_entry(prCur, struct TX_DATA_REQ, list);
		prMsduInfo = prTxReq->prMsduInfo;
		if (!kalIsAggregatedMsdu(prGlueInfo, prMsduInfo)) {
			if (!halWpdmaWriteMsdu(prGlueInfo, prMsduInfo, prCur))
				return false;
		}
	}

	list_sort((void *)prGlueInfo, prHead, kalAmsduTxDCmp);

	for (prCur = prHead->next; prCur != prHead; prCur = prNext) {
		u4Num = kalGetNumOfAmsdu(prGlueInfo, prCur, prHead, &u2Size);
		prNext = prCur->next;
		if (u4Num > 1) {
			for (u4Idx = 1; u4Idx < u4Num; u4Idx++)
				prNext = prNext->next;
			fgRet = halWpdmaWriteAmsdu(prGlueInfo, prCur,
						   u4Num, u2Size);
		} else {
			prTxReq = list_entry(prCur, struct TX_DATA_REQ, list);
			prMsduInfo = prTxReq->prMsduInfo;
			fgRet = halWpdmaWriteMsdu(prGlueInfo, prMsduInfo,
						  prCur);
		}
		if (!fgRet)
			break;
	}

	return fgRet;
}

bool kalDevReadData(struct GLUE_INFO *prGlueInfo, uint16_t u2Port,
		    struct SW_RFB *prSwRfb)
{
	struct ADAPTER *prAdapter = NULL;
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct HIF_MEM_OPS *prMemOps;
	struct RXD_STRUCT *pRxD;
	struct RTMP_RX_RING *prRxRing;
	struct RTMP_DMACB *pRxCell;
	struct RTMP_DMABUF *prDmaBuf;
	u_int8_t fgRet = TRUE;
	uint32_t u4CpuIdx = 0;
#ifdef CFG_SUPPORT_PDMA_SCATTER
	struct RTMP_DMACB *pRxCellScatter;
	struct RXD_STRUCT *pRxDScatter;
	uint32_t u4CpuIdxScatter = 0;
	uint8_t ucScatterCnt = 0;
	uint8_t *pucRecvBuff;
#endif
	u_int8_t fgSkip = FALSE;
	u_int8_t fgSegmentFirst = FALSE;

	ASSERT(prGlueInfo);

	prAdapter = prGlueInfo->prAdapter;
	prHifInfo = &prGlueInfo->rHifInfo;
	prMemOps = &prHifInfo->rMemOps;
	prRxRing = &prHifInfo->RxRing[u2Port];

	u4CpuIdx = prRxRing->RxCpuIdx;
	INC_RING_INDEX(u4CpuIdx, prRxRing->u4RingSize);

	pRxCell = &prRxRing->Cell[u4CpuIdx];
	pRxD = (struct RXD_STRUCT *)pRxCell->AllocVa;

	if (!kalWaitRxDmaDone(prGlueInfo, prRxRing, pRxD, u2Port)) {
		if (!prRxRing->fgIsDumpLog) {
			DBGLOG(HAL, ERROR, "RX Done bit not ready(ReadData)\n");
		}
		prRxRing->fgIsDumpLog = true;
		return false;
	}

#if HIF_INT_TIME_DEBUG
	kalTrackRxReadyTime(prGlueInfo, u2Port);
#endif

	if (pRxD->LastSec0 == 0 || prRxRing->fgRxSegPkt) {
		/* Rx segmented packet */
		if (!prGlueInfo->fgIsEnableMon) {
			DBGLOG(HAL, WARN,
				"Skip Rx segmented data packet, SDL0[%u] LS0[%u] Mo[%u]\n",
				pRxD->SDLen0, pRxD->LastSec0,
				prGlueInfo->fgIsEnableMon);
		}

		if (prAdapter->rWifiVar.fgDumpRxDsegment &&
		    prRxRing->fgRxSegPkt == FALSE)
			fgSegmentFirst = TRUE;

#ifdef CFG_SUPPORT_PDMA_SCATTER
		if (prGlueInfo->fgIsEnableMon &&
			prRxRing->fgRxSegPkt == FALSE) {
			u4CpuIdxScatter = u4CpuIdx;
			do {
				pRxCellScatter = &prRxRing->Cell[u4CpuIdxScatter];
				pRxDScatter = (struct RXD_STRUCT *)pRxCellScatter->AllocVa;
				ucScatterCnt++;

				if (pRxDScatter->LastSec0 == 1)
					break;

				INC_RING_INDEX(u4CpuIdxScatter, prRxRing->u4RingSize);
			} while (TRUE);

			prRxRing->pvPacket = kalPacketAlloc(
				prGlueInfo,
				(ucScatterCnt * CFG_RX_MAX_MPDU_SIZE),
				FALSE, &pucRecvBuff);
			prRxRing->u4PacketLen = 0;

			RX_ADD_CNT(&prAdapter->rRxCtrl,
				RX_PDMA_SCATTER_DATA_COUNT, ucScatterCnt);
		}
#endif
		if (pRxD->LastSec0 == 1) {
			/* Last segmented packet */
			prRxRing->fgRxSegPkt = FALSE;
		} else {
			/* Segmented packet */
			prRxRing->fgRxSegPkt = TRUE;
		}

		fgRet = false;
#ifdef CFG_SUPPORT_PDMA_SCATTER
		if (prRxRing->pvPacket == NULL)
#endif
			if (prAdapter->rWifiVar.fgDumpRxDsegment)
				fgSkip = TRUE;
			else
				goto skip;
	}

	prDmaBuf = &pRxCell->DmaBuf;

	if (prMemOps->copyRxData &&
	    !prMemOps->copyRxData(prHifInfo, pRxCell, prDmaBuf, prSwRfb)) {
		fgRet = false;
		goto skip;
	}

	prSwRfb->pucRecvBuff = ((struct sk_buff *)prSwRfb->pvPacket)->data;
	prSwRfb->prRxStatus = (void *)prSwRfb->pucRecvBuff;

#if CFG_TCP_IP_CHKSUM_OFFLOAD
	prSwRfb->u4TcpUdpIpCksStatus = pRxD->RXINFO;
#endif /* CFG_TCP_IP_CHKSUM_OFFLOAD */

	NIC_DUMP_RXDMAD_HEADER(prAdapter, "Dump RXDMAD:\n");
	NIC_DUMP_RXDMAD(prAdapter, (uint8_t *)pRxD, sizeof(struct RXD_STRUCT));

	/* dump rxd for large pkt */
	if (!prGlueInfo->fgIsEnableMon &&
		prAdapter->rWifiVar.fgDumpRxDsegment && fgSkip) {
		void *pvPayload = prSwRfb->pucRecvBuff;
		uint32_t u4PayloadLen = pRxD->SDLen0;

		/* only first segment has rxd */
		if (fgSegmentFirst) {
			nicRxFillRFB(prAdapter, prSwRfb);

			pvPayload = prSwRfb->pvHeader;
			u4PayloadLen = prSwRfb->u2PacketLen;

			NIC_DUMP_ICV_RXD(prAdapter, prSwRfb->prRxStatus);
		}

		u4PayloadLen = u4PayloadLen < CFG_RX_MAX_PKT_SIZE ?
				u4PayloadLen : CFG_RX_MAX_PKT_SIZE;
		/* dump payload */
		NIC_DUMP_ICV_RXP(pvPayload, u4PayloadLen);
		goto skip;
	}

	pRxD->SDPtr0 = (uint64_t)prDmaBuf->AllocPa & DMA_LOWER_32BITS_MASK;
#ifdef CONFIG_PHYS_ADDR_T_64BIT
	pRxD->SDPtr1 = ((uint64_t)prDmaBuf->AllocPa >>
		DMA_BITS_OFFSET) & DMA_HIGHER_4BITS_MASK;
#else
	pRxD->SDPtr1 = 0;
#endif

#ifdef CFG_SUPPORT_PDMA_SCATTER
	if (prGlueInfo->fgIsEnableMon && fgRet == FALSE) {
		pucRecvBuff = ((struct sk_buff *)prRxRing->pvPacket)->data;
		pucRecvBuff += prRxRing->u4PacketLen;
		kalMemCopy(pucRecvBuff, prSwRfb->pucRecvBuff, pRxD->SDLen0);
		prRxRing->u4PacketLen += pRxD->SDLen0;

		if (prRxRing->fgRxSegPkt == FALSE) {
			RX_INC_CNT(&prAdapter->rRxCtrl,
				RX_PDMA_SCATTER_INDICATION_COUNT);
			kalPacketFree(prGlueInfo, prSwRfb->pvPacket);
			prSwRfb->pvPacket = prRxRing->pvPacket;
			prSwRfb->pucRecvBuff =
				((struct sk_buff *)prSwRfb->pvPacket)->data;
			prSwRfb->prRxStatus = (void *)prSwRfb->pucRecvBuff;
			prRxRing->pvPacket = NULL;
			fgRet = TRUE;
		}
	}
#endif
skip:
	pRxD->SDLen0 = prRxRing->u4BufSize;
	pRxD->DMADONE = 0;

	prRxRing->RxCpuIdx = u4CpuIdx;
	prRxRing->fgIsDumpLog = false;

	GLUE_INC_REF_CNT(prGlueInfo->prAdapter->rHifStats.u4DataRxCount);

#if CFG_TCP_IP_CHKSUM_OFFLOAD
	if (fgRet)
		DBGLOG(RX, LOUD, "u4TcpUdpIpCksStatus[0x%02x]\n",
		       prSwRfb->u4TcpUdpIpCksStatus);
#endif /* CFG_TCP_IP_CHKSUM_OFFLOAD */

	return fgRet;
}

int wf_ioremap_read(phys_addr_t addr, unsigned int *val)
{
	void *vir_addr = NULL;

	vir_addr = ioremap(addr, 0x10);
	if (!vir_addr) {
		DBGLOG(INIT, ERROR, "%s: Cannot remap address[%pa].\n",
		       __func__, addr);
		return -1;
	}

	*val = readl(vir_addr);
	iounmap(vir_addr);
	DBGLOG(INIT, TRACE, "Read CONSYS 0x%08x=0x%08x.\n", addr, *val);

	return 0;
}

int wf_ioremap_write(phys_addr_t addr, unsigned int val)
{
#if (CFG_WLAN_ATF_SUPPORT == 1)
	DBGLOG(INIT, TRACE, "Write CONSYS 0x%08x=0x%08x.\n",
		addr, val);

	kalSendAtfSmcCmd(SMC_WLAN_IOREMAP_WR_CR_OPID,
		(uint32_t)addr, (uint32_t)val, 0);
#else
	void *vir_addr = NULL;
	vir_addr = ioremap(addr, 0x10);
	if (!vir_addr) {
		DBGLOG(INIT, ERROR, "%s: Cannot remap address[%pa].\n",
		       __func__, addr);
		return -1;
	}

	writel(val, vir_addr);
	iounmap(vir_addr);
	DBGLOG(INIT, TRACE, "Write CONSYS 0x%08x=0x%08x.\n", addr, val);
#endif
	return 0;
}


#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
int32_t wf_reg_read_wrapper(void *priv,
	uint32_t addr, uint32_t *value)
{
	struct GLUE_INFO *glue = NULL;
	struct ADAPTER *ad = NULL;
	struct CHIP_DBG_OPS *prDebugOps = NULL;
	bool dumpViaBt = FALSE;
	int32_t ret = 0;

	if (!priv) {
		DBGLOG_LIMITED(HAL, WARN, "NULL GLUE.\n");
		ret = -EFAULT;
		goto exit;
	}
	glue = priv;
	ad = glue->prAdapter;

	if (!ad) {
		DBGLOG_LIMITED(HAL, WARN, "NULL ADAPTER.\n");
		ret = -EFAULT;
		goto exit;
	}

	if (!wlanIsDriverReady(glue,
			       WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG_LIMITED(HAL, WARN,
			"HIF is not ready.\n");
		ret = -EFAULT;
		goto exit;
	}

	prDebugOps = ad->chip_info->prDebugOps;
	if (prDebugOps && prDebugOps->checkDumpViaBt)
		dumpViaBt = prDebugOps->checkDumpViaBt();

	if (dumpViaBt) {
		DBGLOG_LIMITED(HAL, WARN,
			"PCIe AER.\n");
		ret = -EFAULT;
		goto exit;
	}

	HAL_MCR_RD(ad, addr, value);

exit:
	return ret;
}

int32_t wf_reg_write_wrapper(void *priv,
	uint32_t addr, uint32_t value)
{
	struct GLUE_INFO *glue = NULL;
	struct ADAPTER *ad = NULL;
	struct CHIP_DBG_OPS *prDebugOps = NULL;
	bool dumpViaBt = FALSE;
	int32_t ret = 0;

	if (!priv) {
		DBGLOG_LIMITED(HAL, WARN, "NULL GLUE.\n");
		ret = -EFAULT;
		goto exit;
	}
	glue = priv;
	ad = glue->prAdapter;

	if (!ad) {
		DBGLOG_LIMITED(HAL, WARN, "NULL ADAPTER.\n");
		ret = -EFAULT;
		goto exit;
	}

	if (!wlanIsDriverReady(glue,
			       WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG_LIMITED(HAL, WARN,
			"HIF is not ready\n");
		ret = -EFAULT;
		goto exit;
	}

	prDebugOps = ad->chip_info->prDebugOps;
	if (prDebugOps && prDebugOps->checkDumpViaBt)
		dumpViaBt = prDebugOps->checkDumpViaBt();

	if (dumpViaBt) {
		DBGLOG_LIMITED(HAL, WARN,
			"PCIe AER.\n");
		ret = -EFAULT;
		goto exit;
	}

	HAL_MCR_WR(ad, addr, value);

exit:
	return ret;
}

int32_t wf_reg_write_mask_wrapper(void *priv,
	uint32_t addr, uint32_t mask, uint32_t value)
{
	struct GLUE_INFO *glue = NULL;
	struct ADAPTER *ad = NULL;
	struct CHIP_DBG_OPS *prDebugOps = NULL;
	bool dumpViaBt = FALSE;
	uint32_t val = 0;
	int32_t ret = 0;

	if (!priv) {
		DBGLOG_LIMITED(HAL, WARN, "NULL GLUE.\n");
		ret = -EFAULT;
		goto exit;
	}
	glue = priv;
	ad = glue->prAdapter;

	if (!ad) {
		DBGLOG_LIMITED(HAL, WARN, "NULL ADAPTER.\n");
		ret = -EFAULT;
		goto exit;
	}

	if (!wlanIsDriverReady(glue,
			       WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG_LIMITED(HAL, WARN,
			"HIF is not ready\n");
		ret = -EFAULT;
		goto exit;
	}

	prDebugOps = ad->chip_info->prDebugOps;
	if (prDebugOps && prDebugOps->checkDumpViaBt)
		dumpViaBt = prDebugOps->checkDumpViaBt();

	if (dumpViaBt) {
		DBGLOG_LIMITED(HAL, WARN,
			"PCIe AER.\n");
		ret = -EFAULT;
		goto exit;
	}

	HAL_MCR_RD(ad, addr, &val);
	val &= ~mask;
	val |= value;
	HAL_MCR_WR(ad, addr, val);

exit:
	return ret;
}

int32_t wf_reg_start_wrapper(enum connv3_drv_type from_drv,
	void *priv_data)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	struct CHIP_DBG_OPS *prDebugOps = NULL;
	bool dumpViaBt = FALSE;
	int32_t ret = 0;

	WIPHY_PRIV(wlanGetWiphy(), prGlueInfo);
	if (prGlueInfo == NULL) {
		DBGLOG(INIT, ERROR, "prGlueInfo is NULL.\n");
		ret = -EFAULT;
		goto exit;
	}

	prAdapter = prGlueInfo->prAdapter;
	if (prAdapter == NULL) {
		DBGLOG(INIT, ERROR, "prAdapter is NULL.\n");
		ret = -EFAULT;
		goto exit;
	}

	if (kalIsHalted()) {
		DBGLOG_LIMITED(HAL, WARN,
			"Driver in halted state.\n");
		ret = -EFAULT;
		goto exit;
	}

	prDebugOps = prAdapter->chip_info->prDebugOps;
	if (prDebugOps && prDebugOps->checkDumpViaBt)
		dumpViaBt = prDebugOps->checkDumpViaBt();

	if (dumpViaBt) {
		DBGLOG_LIMITED(HAL, WARN,
			"PCIe AER.\n");
		ret = -EFAULT;
		goto exit;
	}

	strlcpy(prAdapter->prGlueInfo->drv_own_caller,
		__func__, CALLER_LENGTH);
	halSetDriverOwn(prAdapter);
	if (prAdapter->fgIsFwOwn == TRUE) {
		DBGLOG_LIMITED(HAL, WARN,
			"Driver own fail.\n");
		ret = -EFAULT;
		goto exit;
	}

	DBGLOG(INIT, INFO, "prAdapter->u4PwrCtrlBlockCnt = %u\n",
			prAdapter->u4PwrCtrlBlockCnt);

exit:
	return ret;
}

int32_t wf_reg_end_wrapper(enum connv3_drv_type from_drv,
	void *priv_data)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	struct CHIP_DBG_OPS *prDebugOps = NULL;
	bool dumpViaBt = FALSE;
	int32_t ret = 0;

	WIPHY_PRIV(wlanGetWiphy(), prGlueInfo);
	if (prGlueInfo == NULL) {
		DBGLOG(INIT, ERROR, "prGlueInfo is NULL.\n");
		ret = -EFAULT;
		goto exit;
	}

	prAdapter = prGlueInfo->prAdapter;
	if (prAdapter == NULL) {
		DBGLOG(INIT, ERROR, "prAdapter is NULL.\n");
		ret = -EFAULT;
		goto exit;
	}

	if (kalIsHalted()) {
		DBGLOG_LIMITED(HAL, WARN,
			"Driver in halted state.\n");
		ret = -EFAULT;
		goto exit;
	}

	prDebugOps = prAdapter->chip_info->prDebugOps;
	if (prDebugOps && prDebugOps->checkDumpViaBt)
		dumpViaBt = prDebugOps->checkDumpViaBt();

	if (dumpViaBt) {
		DBGLOG_LIMITED(HAL, WARN,
			"PCIe AER.\n");
		ret = -EFAULT;
		goto exit;
	}

	strlcpy(prAdapter->prGlueInfo->fw_own_caller,
		__func__, CALLER_LENGTH);
	halSetFWOwn(prAdapter, FALSE);
	DBGLOG(INIT, INFO, "prAdapter->u4PwrCtrlBlockCnt = %u\n",
			prAdapter->u4PwrCtrlBlockCnt);

exit:
	return ret;
}
#endif

