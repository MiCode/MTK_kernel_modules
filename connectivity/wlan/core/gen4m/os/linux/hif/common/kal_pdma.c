// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

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
#if (CFG_MTK_WIFI_CONNV3_SUPPORT == 1)
#include "connv3.h"
#endif

#include "gl_coredump.h"

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#if CFG_MTK_WIFI_MBU
#define MBU_MAX_TIMEOUT_CNT 3
#endif

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
#if CFG_SUPPORT_HIF_REG_WORK
static u_int8_t g_fgIsEnHifRegWork;
#endif

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

#if CFG_MTK_WIFI_PCIE_SR
extern u_int8_t fgIsL2Finished;
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
#if CFG_SUPPORT_CONNAC1X || CFG_SUPPORT_CONNAC2X
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
#endif

static u_int8_t kalIsCbtopRange(struct mt66xx_chip_info *prChipInfo,
				uint32_t reg)
{
	const struct PCIE_CHIP_CR_REMAPPING *remap =
		prChipInfo->bus_info->bus2chip_remap;
	uint32_t u4Idx = 0;

	if (!remap) {
		DBGLOG(INIT, ERROR, "Remapping table NOT supported\n");
		return FALSE;
	}

	if (!remap->cbtop_ranges)
		return FALSE;

	for (u4Idx = 0; remap->cbtop_ranges[u4Idx].end != 0; u4Idx++) {
		if (reg >= remap->cbtop_ranges[u4Idx].start &&
		    reg <= remap->cbtop_ranges[u4Idx].end)
			return TRUE;
	}

	return FALSE;
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

	if (remap->pcie2ap_cbtop && kalIsCbtopRange(prChipInfo, reg))
		pcie2ap = remap->pcie2ap_cbtop;
	else
		pcie2ap = remap->pcie2ap;
	if (!pcie2ap) {
		DBGLOG(INIT, ERROR, "pcie2ap remap NOT supported\n");
		return;
	}

	kalAcquireSpinLock(prGlueInfo, SPIN_LOCK_HIF_REMAP, &flags);
	if (pcie2ap->def_val)
		backup_val = pcie2ap->def_val;
	else
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

	if (remap->pcie2ap_cbtop && kalIsCbtopRange(prChipInfo, reg))
		pcie2ap = remap->pcie2ap_cbtop;
	else
		pcie2ap = remap->pcie2ap;
	if (!pcie2ap) {
		DBGLOG(INIT, ERROR, "pcie2ap remap NOT supported\n");
		return;
	}

	kalAcquireSpinLock(prGlueInfo, SPIN_LOCK_HIF_REMAP, &flags);
	if (pcie2ap->def_val)
		backup_val = pcie2ap->def_val;
	else
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

	DBGLOG(INIT, DEBUG, "reg: 0x%x, total_size: 0x%x\n", reg, total_size);

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

#if (CFG_MTK_WIFI_CONNV3_SUPPORT == 1)
u_int8_t kalDevRegReadViaBT(struct GLUE_INFO *prGlueInfo,
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

	glGetChipInfoByGlue(prGlueInfo, (void **)&prChipInfo);

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
		ret = connv3_hif_dbg_write(CONNV3_DRV_TYPE_WIFI,
			CONNV3_DRV_TYPE_BT,
			AP2WF_DYNAMIC_REMAP_NO_1,
			CONN_MCU_BUS_CR_BASE_ADDR);
		if (ret != 0) {
			DBGLOG(HAL, ERROR,
			       "Write fail: AP2WF_DYNAMIC_REMAP_NO_1\n");
			return FALSE;
		}
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
		ret = connv3_hif_dbg_write(CONNV3_DRV_TYPE_WIFI,
			CONNV3_DRV_TYPE_BT,
			AP2WF_DYNAMIC_REMAP_NO_1,
			WF_MCUSYS_VDNR_GEN_BUS_U_DEBUG_CTRL_START);
		if (ret != 0) {
			DBGLOG(HAL, ERROR,
			       "Write fail: AP2WF_DYNAMIC_REMAP_NO_1\n");
			return FALSE;
		}
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

u_int8_t kalDevRegWriteViaBT(struct GLUE_INFO *prGlueInfo,
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

	glGetChipInfoByGlue(prGlueInfo, (void **)&prChipInfo);

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
		ret = connv3_hif_dbg_write(CONNV3_DRV_TYPE_WIFI,
			CONNV3_DRV_TYPE_BT,
			AP2WF_DYNAMIC_REMAP_NO_1,
			CONN_MCU_BUS_CR_BASE_ADDR);
		if (ret != 0) {
			DBGLOG(HAL, ERROR,
			       "Write fail: AP2WF_DYNAMIC_REMAP_NO_1\n");
			return FALSE;
		}
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
		ret = connv3_hif_dbg_write(CONNV3_DRV_TYPE_WIFI,
			CONNV3_DRV_TYPE_BT,
			AP2WF_DYNAMIC_REMAP_NO_1,
			WF_MCUSYS_VDNR_GEN_BUS_U_DEBUG_CTRL_START);
		if (ret != 0) {
			DBGLOG(HAL, ERROR,
			       "Write fail: AP2WF_DYNAMIC_REMAP_NO_1\n");
			return FALSE;
		}
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
	uint32_t u4BusAddr = u4Register;

	if (!pu4Value) {
		DBGLOG(INIT, ERROR, "pu4Value is NULL.\n");
		return FALSE;
	}

	glGetChipInfoByGlue(prGlueInfo, (void **)&prChipInfo);

	if (!prChipInfo)
		return FALSE;

	if (fgIsBusAccessFailed) {
		DBGLOG_LIMITED(HAL, ERROR, "Bus access failed.\n");
#if (CFG_MTK_WIFI_CONNV3_SUPPORT == 1)
		if (is_wifi_coredump_processing())
			return FALSE;
		else if (fgTriggerDebugSop && kalIsResetting()) {
			if (!prChipInfo->fgDumpViaBtOnlyForDbgSOP)
				return kalDevRegReadViaBT(prGlueInfo,
					u4Register, pu4Value);
		}
#endif
		return FALSE;
	}

	/* Static mapping */
	if (halChipToStaticMapBusAddr(prChipInfo, u4Register, &u4BusAddr))
		RTMP_IO_READ32(prChipInfo, u4BusAddr, pu4Value);

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
static u_int8_t _kalDevRegRead(struct GLUE_INFO *prGlueInfo,
	uint32_t u4Register, uint32_t *pu4Value)
{
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	struct BUS_INFO *prBusInfo = NULL;
	uint32_t u4BusAddr = u4Register;


	if (!pu4Value) {
		DBGLOG(INIT, ERROR, "pu4Value is NULL.\n");
		return FALSE;
	}

	if (prGlueInfo) {
		prHifInfo = &prGlueInfo->rHifInfo;
		prAdapter = prGlueInfo->prAdapter;
		if (!prAdapter) {
			DBGLOG(INIT, ERROR, "prAdapter is NULL.\n");
			return FALSE;
		}
	}

#if (CFG_PCIE_GEN_SWITCH == 1)
	pcie_check_gen_switch_timeout(prAdapter, u4Register);
#endif


	glGetChipInfoByGlue(prGlueInfo, (void **)&prChipInfo);

	if (!prChipInfo)
		return FALSE;

	if (kalIsHostReg(prChipInfo, u4Register)) {
		RTMP_HOST_IO_READ32(prChipInfo, u4Register, pu4Value);
		return TRUE;
	}

	if (fgIsBusAccessFailed) {
		DBGLOG_LIMITED(HAL, ERROR, "Bus access failed.\n");
#if (CFG_MTK_WIFI_CONNV3_SUPPORT == 1)
		if (is_wifi_coredump_processing())
			return FALSE;
		else if (fgTriggerDebugSop && kalIsResetting()) {
			if (!prChipInfo->fgDumpViaBtOnlyForDbgSOP)
				return kalDevRegReadViaBT(prGlueInfo,
					u4Register, pu4Value);
		}
#endif
		return FALSE;
	}

	prBusInfo = prChipInfo->bus_info;

	if (prHifInfo && !prHifInfo->fgIsDumpLog &&
	    prBusInfo->isValidRegAccess &&
	    !prBusInfo->isValidRegAccess(prAdapter, u4Register)) {
		/* Don't print log when resetting */
		if (prAdapter && !wlanIsChipNoAck(prAdapter)) {
			DBGLOG(HAL, ERROR,
			       "Invalid access! Get CR[0x%08x/0x%08x] value[0x%08x]\n",
			       u4Register, u4BusAddr, *pu4Value);
		}
		*pu4Value = HIF_DEADFEED_VALUE;
		return FALSE;
	}

#if defined(_HIF_PCIE)
	if (prGlueInfo &&
	    prGlueInfo->rHifInfo.pdev->current_state != PCI_D0) {
		DBGLOG(HAL, STATE,
			   "Invalid access due to pdev->current_state = %d\n",
			   prGlueInfo->rHifInfo.pdev->current_state);
		*pu4Value = HIF_DEADFEED_VALUE;
		return FALSE;
	}
#endif
	/* Static mapping */
	if (halChipToStaticMapBusAddr(prChipInfo, u4Register, &u4BusAddr)) {
#if CFG_SUPPORT_WED_PROXY
		if (wedMirrorAddrCheck(u4BusAddr)) {
			WARP_PROXY_IO_READ32(prGlueInfo, u4BusAddr, pu4Value);
		} else
#endif
		{
			RTMP_IO_READ32(prChipInfo, u4BusAddr, pu4Value);
		}
	} else {
		if (kalDevRegL1Remap(&u4Register))
			kalDevRegL1Read(prGlueInfo, prChipInfo, u4Register,
				pu4Value);
		else
			kalDevRegL2Read(prGlueInfo, prChipInfo, u4Register,
				pu4Value);
	}

#if CFG_SUPPORT_CONNAC1X || CFG_SUPPORT_CONNAC2X
	if (prGlueInfo && kalIsChipDead(prGlueInfo, u4Register, pu4Value)) {
		/* Don't print log when resetting */
		if (prAdapter && !wlanIsChipNoAck(prAdapter)) {
			DBGLOG(HAL, ERROR, "Read register is deadfeed\n");
			if (in_interrupt())
				DBGLOG(INIT, DEBUG, "Skip reset in tasklet\n");
			else
				GL_DEFAULT_RESET_TRIGGER(prAdapter,
					RST_REG_READ_DEADFEED);
		}
		return FALSE;
	}
#endif

	return TRUE;
}

static u_int8_t kalDevRegWriteStatic(struct GLUE_INFO *prGlueInfo,
	uint32_t u4Register, uint32_t u4Value)
{
	struct mt66xx_chip_info *prChipInfo = NULL;
	uint32_t u4BusAddr = u4Register;

	glGetChipInfoByGlue(prGlueInfo, (void **)&prChipInfo);

	if (!prChipInfo)
		return FALSE;

	if (fgIsBusAccessFailed) {
		DBGLOG_LIMITED(HAL, ERROR, "Bus access failed.\n");
#if (CFG_MTK_WIFI_CONNV3_SUPPORT == 1)
		if (is_wifi_coredump_processing())
			return FALSE;
		else if (fgTriggerDebugSop && kalIsResetting()) {
			if (!prChipInfo->fgDumpViaBtOnlyForDbgSOP)
				return kalDevRegWriteViaBT(prGlueInfo,
					u4Register, u4Value);
		}
#endif
		return FALSE;
	}

#if (CFG_MTK_WIFI_CONNV3_SUPPORT == 1)
	if ((u4Register >= 0x18050000 && u4Register <= 0x18051000) ||
	    (u4Register >= 0x7c050000 && u4Register <= 0x7c051000) ||
	    (u4Register >= 0x7c000000 && u4Register < 0x7c001000) ||
	    (u4Register >= 0x18000000 && u4Register < 0x18001000)) {
		dump_stack();
		kalSendAeeException("WLAN",
			"Corrupt conninfra cmdbt:  reg: 0x%08x, val: 0x%08x\n",
			u4Register, u4Value);
	}
#endif

	/* Static mapping */
#if (CFG_WLAN_ATF_SUPPORT == 1)
	if (halChipToStaticMapBusAddr(prChipInfo, u4Register, &u4BusAddr)) {
		kalSendAtfSmcCmd(
			SMC_WLAN_DEV_REG_WR_CR_OPID,
			(uint32_t)(prChipInfo->u8CsrOffset + u4BusAddr),
			u4Value, 0);
	} else {
		DBGLOG(INIT, ERROR, "Write CONSYS ERROR 0x%08x=0x%08x.\n",
			u4Register, u4Value);
	}
#else
	if (halChipToStaticMapBusAddr(prChipInfo, u4Register, &u4BusAddr))
		RTMP_IO_WRITE32(prChipInfo, u4BusAddr, u4Value);
#endif

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
__no_kcsan
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

#if (CFG_PCIE_GEN_SWITCH == 1)
	pcie_check_gen_switch_timeout(prAdapter, u4Register);
#endif

	glGetChipInfoByGlue(prGlueInfo, (void **)&prChipInfo);

	if (!prChipInfo)
		return FALSE;

	prBusInfo = prChipInfo->bus_info;

	if (kalIsHostReg(prChipInfo, u4Register)) {
		RTMP_HOST_IO_WRITE32(prChipInfo, u4Register, u4Value);
		return TRUE;
	}

	if (fgIsBusAccessFailed) {
		DBGLOG_LIMITED(HAL, ERROR, "Bus access failed.\n");
#if (CFG_MTK_WIFI_CONNV3_SUPPORT == 1)
		if (is_wifi_coredump_processing())
			return FALSE;
		else if (fgTriggerDebugSop && kalIsResetting()) {
			if (!prChipInfo->fgDumpViaBtOnlyForDbgSOP)
				return kalDevRegWriteViaBT(prGlueInfo,
					u4Register, u4Value);
		}
#endif
		return FALSE;
	}

	if (prHifInfo && !prHifInfo->fgIsDumpLog &&
	    prBusInfo->isValidRegAccess &&
	    !prBusInfo->isValidRegAccess(prAdapter, u4Register)) {
		/* Don't print log when resetting */
		if (prAdapter && !wlanIsChipNoAck(prAdapter)) {
			DBGLOG(HAL, ERROR,
			       "Invalid access! Set CR[0x%08x/0x%08x] value[0x%08x]\n",
			       u4Register, u4BusAddr, u4Value);
		}
		return FALSE;
	}

#if (CFG_MTK_WIFI_CONNV3_SUPPORT == 1)
	if ((u4Register >= 0x18050000 && u4Register <= 0x18051000) ||
	    (u4Register >= 0x7c050000 && u4Register <= 0x7c051000) ||
	    (u4Register >= 0x7c000000 && u4Register < 0x7c001000) ||
	    (u4Register >= 0x18000000 && u4Register < 0x18001000)) {
		dump_stack();
		kalSendAeeException("WLAN",
			"Corrupt conninfra cmdbt:  reg: 0x%08x, val: 0x%08x\n",
			u4Register, u4Value);
	}
#endif

	/* Static mapping */
#if (CFG_WLAN_ATF_SUPPORT == 1)
	if (halChipToStaticMapBusAddr(prChipInfo, u4Register, &u4BusAddr)) {
		kalSendAtfSmcCmd(
			SMC_WLAN_DEV_REG_WR_CR_OPID,
			(uint32_t)(prChipInfo->u8CsrOffset + u4BusAddr),
			u4Value, 0);
	} else {
		DBGLOG(INIT, ERROR, "Write CONSYS ERROR 0x%08x=0x%08x.\n",
			u4Register, u4Value);
	}
#else
	if (halChipToStaticMapBusAddr(prChipInfo, u4Register, &u4BusAddr)) {
#if CFG_SUPPORT_WED_PROXY
		if (wedMirrorAddrCheck(u4BusAddr)) {
			WARP_PROXY_IO_WRITE32(prGlueInfo, u4BusAddr, u4Value);
		} else
#endif
		{
			RTMP_IO_WRITE32(prChipInfo, u4BusAddr, u4Value);
		}
	} else {
		if (kalDevRegL1Remap(&u4Register))
			kalDevRegL1Write(prGlueInfo, prChipInfo, u4Register,
				u4Value);
		else
			kalDevRegL2Write(prGlueInfo, prChipInfo, u4Register,
				u4Value);
	}
#endif

	if (prHifInfo)
		prHifInfo->u4HifCnt++;

	return TRUE;
}

static u_int8_t _kalDevRegReadRange(struct GLUE_INFO *glue,
	uint32_t reg, void *buf, uint32_t total_size)
{
	struct mt66xx_chip_info *chip_info;
	uint32_t bus_addr = 0;
	u_int8_t ret = TRUE;

	if (glue && !glue->prAdapter) {
		DBGLOG(INIT, ERROR, "prAdapter is NULL.\n");
		return FALSE;
	}

	glGetChipInfoByGlue(glue, (void **)&chip_info);
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

	if (glue && !glue->prAdapter) {
		DBGLOG(INIT, ERROR, "prAdapter is NULL.\n");
		return FALSE;
	}

	glGetChipInfoByGlue(glue, (void **)&chip_info);
	if (!chip_info) {
		DBGLOG(INIT, ERROR, "chip info is NULL\n");
		return FALSE;
	}

	if (fgIsBusAccessFailed) {
		DBGLOG_LIMITED(HAL, ERROR, "Bus access failed.\n");
		return FALSE;
	}

#if (CFG_MTK_WIFI_CONNV3_SUPPORT == 1)
	if ((reg >= 0x18050000 && reg <= 0x18051000) ||
		(reg >= 0x7c050000 && reg <= 0x7c051000)) {
		dump_stack();
		kalSendAeeException("WLAN",
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

#if CFG_NEW_HIF_DEV_REG_IF
static u_int8_t kalIsValidRead(enum HIF_DEV_REG_REASON eReason,
			       struct GLUE_INFO *prGlueInfo,
			       uint32_t u4Reg,
			       uint32_t u4Mod)
{
	struct mt66xx_chip_info *prChipInfo;
	struct GL_HIF_INFO *prHifInfo;
	struct HIF_DEV_REG_RECORD *prRecord;
	uint32_t u4Idx = 0;

	if (!prGlueInfo)
		goto check;

	prHifInfo = &prGlueInfo->rHifInfo;

	u4Idx = (uint32_t)eReason;
	prHifInfo->u4MmioReadReasonCnt[u4Idx]++;

	u4Idx = prHifInfo->u4MmioReadHistoryIdx;
	if (u4Idx >= HIF_DEV_REG_HISTORY_SIZE)
		u4Idx = 0;

	prRecord = &prHifInfo->arMmioReadHistory[u4Idx++];
	prRecord->eReason = eReason;
	prRecord->u4Reg = u4Reg;
	prRecord->u4Mod = u4Mod;
	prHifInfo->u4MmioReadHistoryIdx = u4Idx;

check:
	glGetChipInfoByGlue(prGlueInfo, (void **)&prChipInfo);

	if (prChipInfo && prChipInfo->isValidMmioReadReason &&
	    !prChipInfo->isValidMmioReadReason(prChipInfo, eReason)) {
		DBGLOG(HAL, ERROR,
		       "Read invalid register. reg[0x%08x] rsn[%d] mod[%u].\n",
		       u4Reg, eReason, u4Mod);

		if (prChipInfo->fgIsWarnInvalidMmioRead)
			WARN_ON_ONCE(TRUE);

		if (prGlueInfo && prChipInfo->fgIsResetInvalidMmioRead) {
			GL_USER_DEFINE_RESET_TRIGGER(
				prGlueInfo->prAdapter,
				RST_MMIO_READ, RST_FLAG_WF_RESET);
		}
		return FALSE;
	}

	return TRUE;
}

static u_int8_t kalIsNoMmioReadReason(enum HIF_DEV_REG_REASON eReason,
		       struct GLUE_INFO *prGlueInfo,
		       uint32_t u4Register, uint32_t *pu4Value)
{
#if CFG_MTK_WIFI_SW_EMI_RING
	struct mt66xx_chip_info *prChipInfo = NULL;

	glGetChipInfo((void **)&prChipInfo);
	if (prChipInfo && prChipInfo->bus_info &&
	    prChipInfo->bus_info->rSwEmiRingInfo.fgIsEnable &&
	    prChipInfo->isNoMmioReadReason &&
	    prChipInfo->isNoMmioReadReason(prChipInfo, eReason)) {
		return kalDevRegReadByEmi(prGlueInfo, u4Register, pu4Value);
	}
#endif
	return FALSE;
}

u_int8_t kalDevRegRead(enum HIF_DEV_REG_REASON eReason,
		       struct GLUE_INFO *prGlueInfo,
		       uint32_t u4Register, uint32_t *pu4Value)
{
	if (!kalIsValidRead(eReason, prGlueInfo, u4Register, 0))
		return FALSE;

	if (kalIsNoMmioReadReason(eReason, prGlueInfo, u4Register, pu4Value))
		return TRUE;

	return _kalDevRegRead(prGlueInfo, u4Register, pu4Value);
}

u_int8_t kalDevRegReadRange(
	enum HIF_DEV_REG_REASON reason, struct GLUE_INFO *glue,
	uint32_t reg, void *buf, uint32_t total_size)
{
	if (!kalIsValidRead(reason, glue, reg, 1))
		return FALSE;

	return _kalDevRegReadRange(glue, reg, buf, total_size);
}
#else
u_int8_t kalDevRegRead(struct GLUE_INFO *prGlueInfo,
		       uint32_t u4Register, uint32_t *pu4Value)
{
	return _kalDevRegRead(prGlueInfo, u4Register, pu4Value);
}

u_int8_t kalDevRegReadRange(
	struct GLUE_INFO *glue, uint32_t reg,
	void *buf, uint32_t total_size)
{
	return _kalDevRegReadRange(glue, reg, buf, total_size);
}
#endif /* CFG_NEW_HIF_DEV_REG_IF */

#if CFG_SUPPORT_WED_PROXY
u_int8_t kalDevRegReadDirectly(struct GLUE_INFO *prGlueInfo,
		       uint32_t u4Register, uint32_t *pu4Value)
{
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	struct BUS_INFO *prBusInfo = NULL;
	uint32_t u4BusAddr = u4Register;

	if (!pu4Value) {
		DBGLOG(INIT, ERROR, "pu4Value is NULL.\n");
		return FALSE;
	}

	if (prGlueInfo) {
		prHifInfo = &prGlueInfo->rHifInfo;
		prAdapter = prGlueInfo->prAdapter;
		if (!prAdapter) {
			DBGLOG(INIT, ERROR, "prAdapter is NULL.\n");
			return FALSE;
		}
	}
	DBGLOG(NIC, TRACE, "enter\n");
	glGetChipInfoByGlue(prGlueInfo, (void **)&prChipInfo);

	if (!prChipInfo)
		return FALSE;

	if (kalIsHostReg(prChipInfo, u4Register)) {
		RTMP_HOST_IO_READ32(prChipInfo, u4Register, pu4Value);
		return TRUE;
	}

	if (fgIsBusAccessFailed) {
		DBGLOG_LIMITED(HAL, ERROR, "Bus access failed.\n");
#if (CFG_MTK_WIFI_CONNV3_SUPPORT == 1)
		if (is_wifi_coredump_processing())
			return FALSE;
		else if (fgTriggerDebugSop && kalIsResetting()) {
			if (!prChipInfo->fgDumpViaBtOnlyForDbgSOP)
				return kalDevRegReadViaBT(prGlueInfo,
					u4Register, pu4Value);
		}
#endif
		return FALSE;
	}

	prBusInfo = prChipInfo->bus_info;

	if (prHifInfo && !prHifInfo->fgIsDumpLog &&
	    prBusInfo->isValidRegAccess &&
	    !prBusInfo->isValidRegAccess(prAdapter, u4Register)) {
		/* Don't print log when resetting */
		if (prAdapter && !wlanIsChipNoAck(prAdapter)) {
			DBGLOG(HAL, ERROR,
			       "Invalid access! Get CR[0x%08x/0x%08x] value[0x%08x]\n",
			       u4Register, u4BusAddr, *pu4Value);
		}
		*pu4Value = HIF_DEADFEED_VALUE;
		return FALSE;
	}

	/* Static mapping */
	if (halChipToStaticMapBusAddr(prChipInfo, u4Register, &u4BusAddr)) {
		RTMP_IO_READ32(prChipInfo, u4BusAddr, pu4Value);
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

u_int8_t kalDevRegWriteDirectly(struct GLUE_INFO *prGlueInfo,
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

	glGetChipInfoByGlue(prGlueInfo, (void **)&prChipInfo);

	if (!prChipInfo)
		return FALSE;

	prBusInfo = prChipInfo->bus_info;

	if (kalIsHostReg(prChipInfo, u4Register)) {
		RTMP_HOST_IO_WRITE32(prChipInfo, u4Register, u4Value);
		return TRUE;
	}

	if (fgIsBusAccessFailed) {
		DBGLOG_LIMITED(HAL, ERROR, "Bus access failed.\n");
#if (CFG_MTK_WIFI_CONNV3_SUPPORT == 1)
		if (is_wifi_coredump_processing())
			return FALSE;
		else if (fgTriggerDebugSop && kalIsResetting()) {
			if (!prChipInfo->fgDumpViaBtOnlyForDbgSOP)
				return kalDevRegWriteViaBT(prGlueInfo,
					u4Register, u4Value);
		}
#endif
		return FALSE;
	}

	if (prHifInfo && !prHifInfo->fgIsDumpLog &&
	    prBusInfo->isValidRegAccess &&
	    !prBusInfo->isValidRegAccess(prAdapter, u4Register)) {
		/* Don't print log when resetting */
		if (prAdapter && !wlanIsChipNoAck(prAdapter)) {
			DBGLOG(HAL, ERROR,
			       "Invalid access! Set CR[0x%08x/0x%08x] value[0x%08x]\n",
			       u4Register, u4BusAddr, u4Value);
		}
		return FALSE;
	}

#if (CFG_MTK_WIFI_CONNV3_SUPPORT == 1)
	if ((u4Register >= 0x18050000 && u4Register <= 0x18051000) ||
	    (u4Register >= 0x7c050000 && u4Register <= 0x7c051000) ||
	    (u4Register >= 0x7c000000 && u4Register < 0x7c001000) ||
	    (u4Register >= 0x18000000 && u4Register < 0x18001000)) {
		dump_stack();
		kalSendAeeException("WLAN",
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
			kalDevRegL1Write(prGlueInfo, prChipInfo, u4Register,
				u4Value);
		else
			kalDevRegL2Write(prGlueInfo, prChipInfo, u4Register,
				u4Value);
	}
#endif

	if (prHifInfo)
		prHifInfo->u4HifCnt++;

	return TRUE;
}
#endif

#if CFG_MTK_WIFI_SW_EMI_RING
u_int8_t kalDevRegReadByEmi(struct GLUE_INFO *prGlueInfo,
			    uint32_t u4Reg, uint32_t *pu4Val)
{
	struct ADAPTER *prAdapter;
	struct mt66xx_chip_info *prChipInfo;
	struct SW_EMI_RING_INFO *prSwEmiRingInfo;
	u_int8_t fgRet = FALSE;

	prAdapter = prGlueInfo->prAdapter;
	prChipInfo = prAdapter->chip_info;

	prSwEmiRingInfo = &prChipInfo->bus_info->rSwEmiRingInfo;
	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.fgEnSwEmiRead) &&
	    prSwEmiRingInfo->rOps.read)
		fgRet = prSwEmiRingInfo->rOps.read(prGlueInfo, u4Reg, pu4Val);

	return fgRet;
}

u_int8_t kalDevRegRead8ByEmi(struct GLUE_INFO *prGlueInfo, uint32_t u4Reg,
			     uint32_t *pu4LowVal, uint32_t *pu4HighVal)
{
	struct ADAPTER *prAdapter;
	struct mt66xx_chip_info *prChipInfo;
	struct SW_EMI_RING_INFO *prSwEmiRingInfo;
	u_int8_t fgRet = FALSE;

	prAdapter = prGlueInfo->prAdapter;
	prChipInfo = prAdapter->chip_info;

	prSwEmiRingInfo = &prChipInfo->bus_info->rSwEmiRingInfo;
	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.fgEnSwEmiRead) &&
	    prSwEmiRingInfo->rOps.read8)
		fgRet = prSwEmiRingInfo->rOps.read8(
			prGlueInfo, u4Reg, pu4LowVal, pu4HighVal);

	return fgRet;
}
#endif /* CFG_MTK_WIFI_SW_EMI_RING */

#if CFG_MTK_WIFI_WFDMA_WB
static void kalWfdmaWriteBackRecovery(
	struct GLUE_INFO *prGlueInfo, struct RTMP_RX_RING *prRxRing,
	uint16_t u2Port)
{
	struct ADAPTER *prAdapter;
	struct CHIP_DBG_OPS *prDbgOps;
	uint32_t u4RxDmaIdx = 0, u4RxEmiDmaIdx = 0;

	if (!prRxRing->fgEnEmiDidx)
		return;

	prAdapter = prGlueInfo->prAdapter;
	prDbgOps = prAdapter->chip_info->prDebugOps;

	HAL_RMCR_RD(HIF_DBG, prAdapter, prRxRing->hw_didx_addr, &u4RxDmaIdx);
	HAL_GET_RING_DIDX(HIF_RING, prAdapter, prRxRing, &u4RxEmiDmaIdx);

	if (u4RxDmaIdx != u4RxEmiDmaIdx) {
		DBGLOG(HAL, DEBUG, "P[%u] DMA[%u] EMI[%u]\n",
		       u2Port, u4RxDmaIdx, u4RxEmiDmaIdx);
		if (prDbgOps && prDbgOps->show_wfdma_wb_info)
			prDbgOps->show_wfdma_wb_info(prAdapter);
		*prRxRing->pu2EmiDidx = (uint16_t)u4RxDmaIdx;
	}
}
#endif /* CFG_MTK_WIFI_WFDMA_WB */

static void kalWaitRxDmaDoneDebug(
	struct GLUE_INFO *prGlueInfo, struct RTMP_RX_RING *prRxRing,
	struct RXD_STRUCT *pRxD, uint16_t u2Port)
{
	uint32_t u4CpuIdx = 0;
	struct RTMP_DMACB *pRxCell;
	struct RXD_STRUCT *pCrRxD;
	struct RTMP_DMABUF *prDmaBuf;
	uint32_t u4Size = 0;

	HAL_RMCR_RD(HIF_DBG, prGlueInfo->prAdapter,
		       prRxRing->hw_didx_addr,
		       &prRxRing->RxDmaIdx);
	prRxRing->RxDmaIdx &= MT_RING_DIDX_MASK;
	DBGLOG(HAL, DEBUG,
	       "Rx DMA done P[%u] DMA[%u] CPU[%u]\n",
	       u2Port, prRxRing->RxDmaIdx, prRxRing->RxCpuIdx);

	u4CpuIdx = prRxRing->RxCpuIdx;
	INC_RING_INDEX(u4CpuIdx, prRxRing->u4RingSize);
	if (prRxRing->RxDmaIdx != u4CpuIdx) {
		pRxCell = &prRxRing->Cell[u4CpuIdx];
		pCrRxD = (struct RXD_STRUCT *)pRxCell->AllocVa;
		DBGLOG(HAL, DEBUG, "Rx DMAD[%u]\n", u4CpuIdx);
		DBGLOG_MEM32(HAL, DEBUG, pCrRxD, sizeof(struct RXD_STRUCT));
		u4Size = pCrRxD->SDLen0;
		if (u4Size > CFG_RX_MAX_PKT_SIZE) {
			DBGLOG(RX, ERROR, "Rx Data too large[%u]\n", u4Size);
		} else {
			DBGLOG(HAL, DEBUG, "RXD+Data[%u] len[%u]\n",
			       u4CpuIdx, u4Size);
			prDmaBuf = &pRxCell->DmaBuf;
			DBGLOG_MEM32(HAL, DEBUG, prDmaBuf->AllocVa, u4Size);
		}
	}

#if CFG_MTK_WIFI_WFDMA_WB
	kalWfdmaWriteBackRecovery(prGlueInfo, prRxRing, u2Port);
#endif
}

static bool kalWaitRxDmaDone(struct GLUE_INFO *prGlueInfo,
			     struct RTMP_RX_RING *prRxRing,
			     struct RXD_STRUCT *pRxD,
			     uint16_t u2Port)
{
	struct CHIP_DBG_OPS *prDbgOps =
		prGlueInfo->prAdapter->chip_info->prDebugOps;
	uint32_t u4Count = 0;

#if CFG_MTK_WIFI_WFDMA_WB
	/* cannot skip it when monitor mode is on */
	if (!prGlueInfo->fgIsEnableMon && prRxRing->fgEnEmiDidx &&
	    halIsDataRing(RX_RING, u2Port))
		return true;
#endif /* CFG_ENABLE_MAWD_MD_RING */

	for (u4Count = 0; pRxD->DMADONE == 0; u4Count++) {
		if (u4Count > DMA_DONE_WAITING_COUNT ||
		    prGlueInfo->prAdapter->fgIsPwrOffProcIST) {
			kalWaitRxDmaDoneDebug(
				prGlueInfo, prRxRing, pRxD, u2Port);
			if (prDbgOps && prDbgOps->showPdmaInfo)
				prDbgOps->showPdmaInfo(prGlueInfo->prAdapter);
			return false;
		}

		kalUdelay(DMA_DONE_WAITING_TIME);
	}
	return true;
}

static void kalWaitRxDmaDoneTimeoutDebug(
	struct GLUE_INFO *prGlueInfo,
	struct RTMP_RX_RING *prRxRing)
{
	uint32_t u4CpuIdx = 0;
	struct RTMP_DMACB *prRxCell;
	struct RXD_STRUCT *prRxD;
	struct RTMP_DMABUF *prDmaBuf;

	u4CpuIdx = prRxRing->RxCpuIdx;
	INC_RING_INDEX(u4CpuIdx, prRxRing->u4RingSize);
	while (prRxRing->RxDmaIdx != u4CpuIdx) {
		prRxCell = &prRxRing->Cell[u4CpuIdx];
		prRxD = (struct RXD_STRUCT *)prRxCell->AllocVa;
		DBGLOG(HAL, DEBUG, "Rx DMAD[%u]\n", u4CpuIdx);
		DBGLOG_MEM32(HAL, DEBUG, prRxD, sizeof(struct RXD_STRUCT));
		prDmaBuf = &prRxCell->DmaBuf;
		DBGLOG_MEM32(HAL, DEBUG, prDmaBuf->AllocVa, 32);
		INC_RING_INDEX(u4CpuIdx, prRxRing->u4RingSize);
	}
}

#if HIF_INT_TIME_DEBUG
static void kalTrackRxReadyTime(struct GLUE_INFO *prGlueInfo, uint16_t u2Port)
{
	struct BUS_INFO *prBusInfo =
		prGlueInfo->prAdapter->chip_info->bus_info;
	uint64_t u8Now, u8Delta;

	u8Now = kalGetBootTime();
	if (prBusInfo->u4EnHifIntUs &&
		TIME_AFTER64(u8Now, prBusInfo->u8HifIntUs)) {
		u8Delta = TIME_ABS_DIFF64(u8Now, prBusInfo->u8HifIntUs);
		DBGLOG(HAL, DEBUG,
		       "RX[%u] done bit ready time[%lld.%.6lld] cnt[%d]\n",
		       u2Port,
		       USEC_TO_SEC(u8Delta), USEC_REM_TO_SEC(u8Delta),
		       prBusInfo->u4HifIntTsCnt);
		prBusInfo->u4EnHifIntUs = 0;
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
		prRxRing->u4RxDmaDoneFailCnt++;
		if (isPollMode)
			return FALSE;

		if (prRxRing->u4RxDmaDoneFailCnt == 1) {
			DBGLOG(HAL, ERROR, "try to wait done again\n");
			return FALSE;
		}

		if (prRxRing->u4RxDmaDoneFailCnt >=
		    HIF_RX_DMA_DONE_MAX_FAIL_CNT) {
			kalWaitRxDmaDoneTimeoutDebug(prGlueInfo, prRxRing);
			prHifInfo->fgIsTriggerRxTimeout = TRUE;
			DBGLOG(HAL, ERROR, "trigger rx timeout EE\n");
			return FALSE;
		}
	} else {
		prRxRing->fgIsWaitRxDmaDoneTimeout = false;
		prRxRing->u4RxDmaDoneFailCnt = 0;
	}

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
		if (prMemOps->dumpRx)
			prMemOps->dumpRx(prHifInfo, prRxRing,
				u4CpuIdx, u4dumpSize);
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
		uint32_t u4Count;
		struct WIFI_EVENT *prEvent;

		for (u4Count = 0; u4Count < 100; u4Count++) {
			prEvent = (struct WIFI_EVENT *)
				(pucBuf + prAdapter->chip_info->rxd_size);

			if ((prEvent->u2PacketLength > 0) ||
			    (pRxD->SDLen0 == prEvent->u2PacketLength))
				goto end;

			kalUdelay(DMA_DONE_WAITING_TIME);

			if (prMemOps->copyEvent &&
			    !prMemOps->copyEvent(prHifInfo, pRxCell, pRxD,
						 prDmaBuf, pucBuf, u4Len))
				return FALSE;
		}
		DBGLOG(RX, ERROR, "Dump RX Event payload len[%d]\n",
		       pRxD->SDLen0);
		DBGLOG_MEM32(RX, ERROR, pucBuf, pRxD->SDLen0);
	}

end:

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
	HAL_SET_RING_CIDX(prGlueInfo->prAdapter, prRxRing, prRxRing->RxCpuIdx);
	prRxRing->fgIsDumpLog = false;

	GLUE_INC_REF_CNT(prGlueInfo->prAdapter->rHifStats.u4EventRxCount);
	prRxRing->u4TotalCnt++;

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
		if (prMemOps->freeCmdBuf)
			prMemOps->freeCmdBuf(pucDst, u4Len);
		return FALSE;
	}

	pTxCell = &prTxRing->Cell[prTxRing->TxCpuIdx];
	pTxD = (struct TXD_STRUCT *)pTxCell->AllocVa;

	pTxCell->pPacket = NULL;
	pTxCell->pBuffer = pucDst;

	if (prMemOps->copyCmd &&
	    !prMemOps->copyCmd(prHifInfo, pTxCell, pucDst,
			       pucBuf, u4Len, NULL, 0)) {
		if (prMemOps->freeCmdBuf)
			prMemOps->freeCmdBuf(pucDst, u4Len);
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
	prTxRing->u4TotalCnt++;

	HAL_SET_RING_CIDX(prGlueInfo->prAdapter, prTxRing, prTxRing->TxCpuIdx);

	GLUE_INC_REF_CNT(prGlueInfo->prAdapter->rHifStats.u4CmdTxCount);

	return TRUE;
}

void kalDevReadIntStatus(struct ADAPTER *prAdapter,
	uint32_t *pu4IntStatus)
{
	uint32_t u4RegValue = 0;
	struct GL_HIF_INFO *prHifInfo = &prAdapter->prGlueInfo->rHifInfo;

	*pu4IntStatus = 0;

	HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter, WPDMA_INT_STA, &u4RegValue);

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
	uint32_t u4TxdSize, u4TxpSize;
	unsigned long flags;
	uint8_t *aucBuff;

	prHifInfo = &prGlueInfo->rHifInfo;

	if (list_empty(&prHifInfo->rTxCmdFreeList)) {
		DBGLOG(HAL, ERROR, "tx cmd free list is empty\n");
		return NULL;
	}

	u4TxdSize = prCmdInfo->u4TxdLen;
	u4TxpSize = prCmdInfo->u4TxpLen;

	if ((u4TxdSize + u4TxpSize) > TX_BUFFER_NORMSIZE) {
		DBGLOG(HAL, ERROR, "u4TxdSize + u4TxpSize size overflow\n");
		return NULL;
	}

	spin_lock_irqsave(&prHifInfo->rTxCmdQLock, flags);
	prNode = prHifInfo->rTxCmdFreeList.next;
	list_del(prNode);
	spin_unlock_irqrestore(&prHifInfo->rTxCmdQLock, flags);

	prCmdReq = list_entry(prNode, struct TX_CMD_REQ, list);
	kalMemCopy(&prCmdReq->rCmdInfo, prCmdInfo, sizeof(struct CMD_INFO));

	aucBuff = prCmdReq->aucBuff;
	kalMemCopy(aucBuff, prCmdInfo->pucTxd, u4TxdSize);
	prCmdReq->rCmdInfo.pucTxd = aucBuff;
	aucBuff += u4TxdSize;

	kalMemCopy(aucBuff, prCmdInfo->pucTxp, u4TxpSize);
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

#if !CFG_TX_CMD_SMART_SEQUENCE
	KAL_SPIN_LOCK_DECLARATION();
#endif /* !CFG_TX_CMD_SMART_SEQUENCE */

	ASSERT(prGlueInfo);
	prHifInfo = &prGlueInfo->rHifInfo;
	prAdapter = prGlueInfo->prAdapter;

	spin_lock_irqsave(&prHifInfo->rTxCmdQLock, flags);
	list_replace_init(&prHifInfo->rTxCmdQ, &rTempQ);
	spin_unlock_irqrestore(&prHifInfo->rTxCmdQLock, flags);

	list_for_each_safe(prCur, prNext, &rTempQ) {
		prTxReq = list_entry(prCur, struct TX_CMD_REQ, list);
#if !CFG_TX_CMD_SMART_SEQUENCE
		KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_CMD_PENDING);
#endif /* !CFG_TX_CMD_SMART_SEQUENCE */
		ret = halWpdmaWriteCmd(prGlueInfo,
				       &prTxReq->rCmdInfo, prTxReq->ucTC);
		if (ret == CMD_TX_RESULT_SUCCESS) {
#if !CFG_TX_CMD_SMART_SEQUENCE
			if (prTxReq->rCmdInfo.pfHifTxCmdDoneCb)
				prTxReq->rCmdInfo.pfHifTxCmdDoneCb(
					prGlueInfo->prAdapter,
					&prTxReq->rCmdInfo);
#endif /* !CFG_TX_CMD_SMART_SEQUENCE */
		} else {
			DBGLOG(HAL, ERROR, "ret: %d\n", ret);
		}
#if !CFG_TX_CMD_SMART_SEQUENCE
		KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_CMD_PENDING);
#endif /* !CFG_TX_CMD_SMART_SEQUENCE */
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
#if ((CFG_MTK_WIFI_DRV_OWN_INT_MODE == 1) && \
	!IS_ENABLED(CFG_SUPPORT_RX_WORK)) || \
	(CFG_MTK_WIFI_DRV_OWN_INT_MODE == 0)

	/* if direct trx,  set drv/fw own will be called
	*  in softirq/tasklet/thread context,
	*  if normal trx, set drv/fw own will only
	*  be called in thread context
	*/
	if (HAL_IS_TX_DIRECT(prAdapter) || HAL_IS_RX_DIRECT(prAdapter))
		spin_lock_bh(
			&prAdapter->prGlueInfo->rSpinLock[SPIN_LOCK_SET_OWN]);
	else
#endif
		KAL_ACQUIRE_MUTEX(prAdapter, MUTEX_SET_OWN);
}

void kalReleaseHifOwnLock(struct ADAPTER *prAdapter)
{
#if ((CFG_MTK_WIFI_DRV_OWN_INT_MODE == 1) && \
	!IS_ENABLED(CFG_SUPPORT_RX_WORK)) || \
	(CFG_MTK_WIFI_DRV_OWN_INT_MODE == 0)

	if (HAL_IS_TX_DIRECT(prAdapter) || HAL_IS_RX_DIRECT(prAdapter))
		spin_unlock_bh(
			&prAdapter->prGlueInfo->rSpinLock[SPIN_LOCK_SET_OWN]);
	else
#endif
		KAL_RELEASE_MUTEX(prAdapter, MUTEX_SET_OWN);
}

int kalHifOwnTryLock(struct ADAPTER *prAdapter)
{
	if (!prAdapter || !prAdapter->prGlueInfo)
		return 0;
#if ((CFG_MTK_WIFI_DRV_OWN_INT_MODE == 1) && \
	(CFG_SUPPORT_RX_WORK == 0)) || \
	(CFG_MTK_WIFI_DRV_OWN_INT_MODE == 0)

	if (HAL_IS_TX_DIRECT(prAdapter) || HAL_IS_RX_DIRECT(prAdapter))
		return spin_trylock(
			&prAdapter->prGlueInfo->rSpinLock[SPIN_LOCK_SET_OWN]);
	else
#endif
		return mutex_trylock(
			&prAdapter->prGlueInfo->arMutex[MUTEX_SET_OWN]);
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
#if (CFG_SUPPORT_HIF_TX_NAPI == 1)
	if (IS_FEATURE_DISABLED(
		    prGlueInfo->prAdapter->rWifiVar.fgHifTxNapiEn))
#endif /* CFG_SUPPORT_HIF_TX_NAPI */
	/* force tx data */
		if (prMsduInfo->pfTxDoneHandler)
			KAL_SET_BIT(HIF_TX_DATA_DELAY_TIMEOUT_BIT,
				    prHifInfo->ulTxDataTimeout);
#endif /* (CFG_SUPPORT_TX_DATA_DELAY == 1) */

	u4Port = halTxRingDataSelect(prGlueInfo->prAdapter, prMsduInfo);
	prTxReq = &prMsduInfo->rTxReq;
	prTxReq->prMsduInfo = prMsduInfo;
	prTxReq->u4PktCnt = GET_TX_PKT_CNT(prMsduInfo);

	KAL_HIF_TXDATAQ_LOCK(prHifInfo, u4Port);
	list_add_tail(&prTxReq->list, &prHifInfo->rTxDataQ[u4Port]);
	prHifInfo->u4TxDataQLen[u4Port] += prTxReq->u4PktCnt;
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
	int32_t *prRingLock;
	uint32_t u4Idx;

	KAL_HIF_TXDATAQ_LOCK_DECLARATION();
#if !CFG_TX_DIRECT_VIA_HIF_THREAD
	KAL_HIF_TXRING_LOCK_DECLARATION();
#endif /* !CFG_TX_DIRECT_VIA_HIF_THREAD */

	ASSERT(prGlueInfo);

	prChipInfo = prGlueInfo->prAdapter->chip_info;
	prHifInfo = &prGlueInfo->rHifInfo;
	prWifiVar = &prGlueInfo->prAdapter->rWifiVar;
	prRingLock = prGlueInfo->i4RingLock;

#if (CFG_SUPPORT_TX_DATA_DELAY == 1)
#if (CFG_SUPPORT_HIF_TX_NAPI == 1)
	if (IS_FEATURE_DISABLED(
		    prGlueInfo->prAdapter->rWifiVar.fgHifTxNapiEn))
#endif /* CFG_SUPPORT_HIF_TX_NAPI */
		if (halCheckAndStartTxDelayTimer(prGlueInfo->prAdapter))
			return 0;
#endif /* CFG_SUPPORT_TX_DATA_DELAY */

#if !CFG_SUPPORT_RX_WORK
	/* disable softirq to improve processing efficiency */
	KAL_HIF_BH_DISABLE(prGlueInfo);
#endif /* !CFG_SUPPORT_RX_WORK */

	for (u4Idx = 0; u4Idx < NUM_OF_TX_RING; u4Idx++) {
		if (!halIsDataRing(TX_RING, u4Idx))
			continue;

		if (unlikely(GLUE_INC_REF_CNT(prRingLock[u4Idx]) > 1)) {
			/* Single user allowed per port read */
			DBGLOG(TX, WARN, "Single user only R[%u] [%d]\n",
				u4Idx,
				GLUE_GET_REF_CNT(prRingLock[u4Idx]));
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
		HAL_SET_RING_CIDX(prGlueInfo->prAdapter, prTxRing,
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
		GLUE_DEC_REF_CNT(prRingLock[u4Idx]);
	}

#if CFG_SUPPORT_TX_FREE_MSDU_WORK
	kalTxFreeMsduWorkSchedule(prGlueInfo);
#endif /* CFG_SUPPORT_TX_FREE_MSDU_WORK */

#if !CFG_SUPPORT_RX_WORK
	KAL_HIF_BH_ENABLE(prGlueInfo);
#endif /* !CFG_SUPPORT_RX_WORK */

#if (CFG_SUPPORT_TX_DATA_DELAY == 1)
#if (CFG_SUPPORT_HIF_TX_NAPI == 1)
	if (IS_FEATURE_DISABLED(prWifiVar->fgHifTxNapiEn))
#endif /* CFG_SUPPORT_HIF_TX_NAPI */
		halCancleTxDelayTimer(prGlueInfo->prAdapter);
#endif /* CFG_SUPPORT_TX_DATA_DELAY */

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

#if KERNEL_VERSION(5, 13, 0) <= LINUX_VERSION_CODE
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
	struct ADAPTER *prAdapter = NULL;
	struct WIFI_VAR *prWifiVar = NULL;
	struct list_head *prCur, *prNext;
	struct TX_DATA_REQ *prTxReq;
	struct MSDU_INFO *prMsduInfo;
	uint32_t u4Num = 0, u4Idx;
	uint16_t u2Size = 0;
	bool fgRet = true;

	ASSERT(prGlueInfo);

	prHifInfo = &prGlueInfo->rHifInfo;
	prBusInfo = prGlueInfo->prAdapter->chip_info->bus_info;
	prAdapter = prGlueInfo->prAdapter;
	prWifiVar = &prAdapter->rWifiVar;

	list_for_each_safe(prCur, prNext, prHead) {
		prTxReq = list_entry(prCur, struct TX_DATA_REQ, list);
		prMsduInfo = prTxReq->prMsduInfo;
		if (!kalIsAggregatedMsdu(prGlueInfo, prMsduInfo)) {
			if (!halWpdmaWriteMsdu(prGlueInfo, prMsduInfo, prCur))
				return false;
		}
	}

	/*
	 * Only Connac1.x supports SW AMSDU
	 * the purpose for msdu sorting is to make more packet do amsdu
	 * But do msdu sorting may cause packet out-of-order
	 * add config and default disable is expected default setting,
	 * but make this switchable just in case.
	 * For the long time, this option and sorting can be removed.
	 */
	if (prWifiVar->fgEnSwAmsduSorting == FEATURE_ENABLED)
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

static void kalDevDebugSegment(struct ADAPTER *ad, struct SW_RFB *prSwRfb,
	enum ENUM_RX_SEGMENT_TYPE eType, uint32_t u4Len)
{
	struct WIFI_VAR *prWifiVar = &ad->rWifiVar;
#if CFG_DEBUG_RX_SEGMENT
	OS_SYSTIME now, last;
#endif /* CFG_DEBUG_RX_SEGMENT */
	void *pvPayload;

	if (eType == RX_SEGMENT_NONE)
		return;

	/* fgDumpRxDsegment always TRUE when CFG_DEBUG_RX_SEGMENT is enabled */
	if (!prWifiVar->fgDumpRxDsegment)
		return;

#if CFG_DEBUG_RX_SEGMENT
	/* only dump one of them before timeout (default: 10s) */
	GET_BOOT_SYSTIME(&now);
	last = ad->rLastRxSegmentTime;

	if (eType == RX_SEGMENT_FIRST &&
		CHECK_FOR_TIMEOUT(now, last,
			SEC_TO_SYSTIME(prWifiVar->u4RxSegmentDebugTimeout)))
		ad->fgDumpRxSegment = TRUE;

	if (!ad->fgDumpRxSegment)
		return;

	ad->rLastRxSegmentTime = now;
#endif /* CFG_DEBUG_RX_SEGMENT */

	/* only first segment has rxd */
	if (eType == RX_SEGMENT_FIRST) {
		nicRxFillRFB(ad, prSwRfb);

		pvPayload = prSwRfb->pvHeader;
		/* use payload length in rxd instead */
		u4Len = prSwRfb->u2PacketLen;

		DBGLOG(HAL, DEBUG, "Dump RXD:\n");
		DBGLOG_MEM8(HAL, DEBUG, prSwRfb->prRxStatus,
			ad->chip_info->rxd_size);
	} else
		pvPayload = prSwRfb->pucRecvBuff;

	/* boundary protection */
	if (u4Len >= CFG_RX_MAX_PKT_SIZE)
		u4Len = CFG_RX_MAX_PKT_SIZE;

	DBGLOG(HAL, DEBUG, "Dump RXP:\n");
	DBGLOG_MEM8(HAL, DEBUG, pvPayload, u4Len);

#if CFG_DEBUG_RX_SEGMENT
	if (eType == RX_SEGMENT_LAST) {
		ad->fgDumpRxSegment = FALSE;
	}
#endif /* CFG_DEBUG_RX_SEGMENT */
}

#if (CFG_SUPPORT_PDMA_SCATTER == 1)
static u_int8_t kalDevPdmaScatterAlloc(struct GLUE_INFO *pr,
	struct RTMP_RX_RING *prRxRing, uint16_t u2Port, uint32_t u4StartIdx)
{
	struct ADAPTER *ad = pr->prAdapter;
	struct RTMP_DMACB *pRxCell;
	struct RXD_STRUCT *pRxD;
	uint32_t u4CurrIdx;
	uint8_t ucScatterCnt = 0;
	uint8_t *pucRecvBuff;

	u4CurrIdx = u4StartIdx;
	do {
		pRxCell = &prRxRing->Cell[u4CurrIdx];
		pRxD = (struct RXD_STRUCT *)pRxCell->AllocVa;

		/* need to wait until WFDMA is ready */
		if (!kalWaitRxDmaDone(pr, prRxRing, pRxD, u2Port))
			return FALSE;

		ucScatterCnt++;

		if (pRxD->LastSec0 == 1)
			break;

		INC_RING_INDEX(u4CurrIdx, prRxRing->u4RingSize);
	} while (TRUE);

	/* Avoid memory leakage */
	if (prRxRing->pvSegPkt) {
		DBGLOG(HAL, WARN,
			"pvPacket[%p] not NULL [%u:%u:%u:%u]\n",
			prRxRing->pvSegPkt,
			prRxRing->u4SegPktLenMax, prRxRing->u4SegPktLen,
			prRxRing->u4SegPktIdxMax, prRxRing->u4SegPktIdx);
		kalPacketFree(pr, prRxRing->pvSegPkt);
	}

	prRxRing->u4SegPktIdx = 0;
	prRxRing->u4SegPktIdxMax = ucScatterCnt;
	prRxRing->u4SegPktLen = 0;
	prRxRing->u4SegPktLenMax = ucScatterCnt * CFG_RX_MAX_MPDU_SIZE;
	prRxRing->pvSegPkt = kalPacketAlloc(pr, prRxRing->u4SegPktLenMax,
				FALSE, &pucRecvBuff);

	RX_ADD_CNT(&ad->rRxCtrl, RX_PDMA_SCATTER_DATA_COUNT, ucScatterCnt);

	return TRUE;
}

static u_int8_t kalDevPdmaScatterCheck(struct GLUE_INFO *pr,
	struct RTMP_RX_RING *prRxRing)
{
	struct ADAPTER *ad = pr->prAdapter;
	struct RX_DESC_OPS_T *prRxDescOps;
	uint8_t *pucRecvBuff;
	void *prRxStatus;
	uint16_t u2RxByteCount;

	prRxDescOps = ad->chip_info->prRxDescOps;
	if (!prRxRing->pvSegPkt)
		goto end;

	pucRecvBuff = ((struct sk_buff *)prRxRing->pvSegPkt)->data;
	prRxStatus = pucRecvBuff;

	/* RxByteCount = sizeof(RXD) + sizeof(Payload) */
	u2RxByteCount = prRxDescOps->nic_rxd_get_rx_byte_count(prRxStatus);
	if (u2RxByteCount <= prRxRing->u4SegPktLenMax)
		return TRUE;

	DBGLOG(HAL, ERROR,
		"Error Detected. PacketIdx[%u/%u] PacketLen[%u/%u] RxByteCnt[%u]\n",
		prRxRing->u4SegPktIdx, prRxRing->u4SegPktIdxMax,
		prRxRing->u4SegPktLen, prRxRing->u4SegPktLenMax,
		u2RxByteCount);
	DBGLOG(HAL, ERROR, "Dump RXD and Payload:\n");
	DBGLOG_MEM8(HAL, ERROR, pucRecvBuff, prRxRing->u4SegPktLenMax);

	kalPacketFree(pr, prRxRing->pvSegPkt);
	prRxRing->pvSegPkt = NULL;
end:
	return FALSE;
}

static u_int8_t kalDevPdmaScatterCopy(struct GLUE_INFO *pr,
	struct RTMP_RX_RING *prRxRing, struct SW_RFB *prSwRfb,
	enum ENUM_RX_SEGMENT_TYPE eType, uint32_t u4Len)
{
	struct ADAPTER *ad = pr->prAdapter;
	uint8_t *pucRecvBuff;

	if (!prRxRing->pvSegPkt)
		goto end;

	/* boundary protection */
	if (u4Len >= CFG_RX_MAX_PKT_SIZE)
		u4Len = CFG_RX_MAX_PKT_SIZE;

	if ((++prRxRing->u4SegPktIdx > prRxRing->u4SegPktIdxMax)
		|| u4Len > (prRxRing->u4SegPktLenMax - prRxRing->u4SegPktLen)) {
		DBGLOG(HAL, ERROR,
			"eType[%u] PacketIdx[%u/%u] PacketLen[%u/%u/%u]\n",
			eType, u4Len,
			prRxRing->u4SegPktIdx, prRxRing->u4SegPktIdxMax,
			u4Len, prRxRing->u4SegPktLen, prRxRing->u4SegPktLenMax);
		goto end;
	}

	/* copy current segment to buffer of pdma scatter */
	pucRecvBuff = ((struct sk_buff *)prRxRing->pvSegPkt)->data;
	pucRecvBuff += prRxRing->u4SegPktLen;
	kalMemCopy(pucRecvBuff, prSwRfb->pucRecvBuff, u4Len);
	prRxRing->u4SegPktLen += u4Len;

	if (eType == RX_SEGMENT_LAST) {
		if (!kalDevPdmaScatterCheck(pr, prRxRing))
			goto end;

		RX_INC_CNT(&ad->rRxCtrl, RX_PDMA_SCATTER_INDICATION_COUNT);
		kalPacketFree(pr, prSwRfb->pvPacket);
		prSwRfb->pvPacket = prRxRing->pvSegPkt;
		prSwRfb->pucRecvBuff =
			((struct sk_buff *)prSwRfb->pvPacket)->data;
		prSwRfb->prRxStatus = (void *)prSwRfb->pucRecvBuff;
		prRxRing->pvSegPkt = NULL;
		return TRUE;
	}

end:
	return FALSE;
}
#endif /* CFG_SUPPORT_PDMA_SCATTER */

bool kalDevReadData(struct GLUE_INFO *prGlueInfo, uint16_t u2Port,
		    struct SW_RFB *prSwRfb)
{
	struct ADAPTER *prAdapter = NULL;
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct HIF_MEM_OPS *prMemOps;
	struct RXD_STRUCT rRxD, *pRxD;
	struct RTMP_RX_RING *prRxRing;
	struct RTMP_DMACB *pRxCell;
	u_int8_t fgRet = TRUE;
	uint32_t u4CpuIdx = 0;
	enum ENUM_RX_SEGMENT_TYPE eType = RX_SEGMENT_NONE;
	u_int8_t fgDebugSegment = FALSE;

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

	pRxD = &rRxD;
	kalMemCopyFromIo(pRxD, pRxCell->AllocVa, sizeof(struct RXD_STRUCT));

#if HIF_INT_TIME_DEBUG
	kalTrackRxReadyTime(prGlueInfo, u2Port);
#endif

	if (pRxD->LastSec0 == 0 || prRxRing->fgRxSegPkt) {
		/*
		 * We should not return data when it is segmented unless the
		 * last segment is received
		 */
		fgRet = FALSE;

		/* Rx segmented packet */
		if (!prGlueInfo->fgIsEnableMon) {
			DBGLOG(HAL, WARN,
				"Skip Segmented Data, Port[%u] SDL0[%u] LS0[%u] Mo[%u]\n",
				u2Port,
				pRxD->SDLen0, pRxD->LastSec0,
				prGlueInfo->fgIsEnableMon);
			fgDebugSegment = TRUE;
		}

		if (pRxD->LastSec0 == 0 && prRxRing->fgRxSegPkt == FALSE) {
			/* First segmented packet */
			eType = RX_SEGMENT_FIRST;
			prRxRing->fgRxSegPkt = TRUE;
		} else if (pRxD->LastSec0 == 1) {
			/* Last segmented packet */
			eType = RX_SEGMENT_LAST;
			prRxRing->fgRxSegPkt = FALSE;
		} else
			eType = RX_SEGMENT_MIDDLE;

#if (CFG_SUPPORT_PDMA_SCATTER == 1)
		/*
		 * only alloc packet when it is the first segment
		 * Note: need to wait until all segment ready
		 */
		if (prGlueInfo->fgIsEnableMon && eType == RX_SEGMENT_FIRST) {
			if (!kalDevPdmaScatterAlloc(prGlueInfo, prRxRing,
				u2Port, u4CpuIdx))
				return false;
		}
#endif
	}

	/* PTR is updated inside copyRxData */
	if (prMemOps->copyRxData &&
	    !prMemOps->copyRxData(prHifInfo, pRxCell, pRxD, prSwRfb, TRUE)) {
		/* If it encounter copy Rx data Fail, it will trigger EE */
		GL_USER_DEFINE_RESET_TRIGGER(prAdapter,
			RST_WFDMA_MAP_FAIL, RST_FLAG_WF_RESET);
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

	if (fgDebugSegment) {
		kalDevDebugSegment(prAdapter, prSwRfb, eType, pRxD->SDLen0);
		goto skip;
	}

#if (CFG_SUPPORT_PDMA_SCATTER == 1)
	if (prGlueInfo->fgIsEnableMon && fgRet == FALSE) {
		fgRet = kalDevPdmaScatterCopy(prGlueInfo, prRxRing, prSwRfb,
				eType, pRxD->SDLen0);
	}
#endif
skip:
	pRxD->SDLen0 = prRxRing->u4BufSize;
	pRxD->DMADONE = 0;

	/* update rxdmad */
	kalMemCopyToIo(pRxCell->AllocVa, pRxD, sizeof(struct RXD_STRUCT));

	prRxRing->RxCpuIdx = u4CpuIdx;
	prRxRing->fgIsDumpLog = false;

	prRxRing->u4TotalCnt++;
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
		       __func__, &addr);
		return -1;
	}

	*val = readl(vir_addr);
	iounmap(vir_addr);
	DBGLOG(INIT, TRACE, "Read CONSYS 0x%08x=0x%08x.\n",
	       (uint32_t)addr, *val);

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
		DBGLOG(INIT, ERROR, "%s: Cannot remap address[0x%08x].\n",
		       __func__, (uint32_t)addr);
		return -1;
	}

	writel(val, vir_addr);
	iounmap(vir_addr);
	DBGLOG(INIT, TRACE, "Write CONSYS 0x%08x=0x%08x.\n",
	       (uint32_t)addr, val);
#endif
	return 0;
}

#if CFG_SUPPORT_HIF_REG_WORK
void wf_reg_enable(u_int8_t fgEn)
{
	g_fgIsEnHifRegWork = fgEn;
}

int32_t wf_reg_handle_req(struct GLUE_INFO *glue, struct WF_REG_REQ *prReq)
{
	struct WF_REG_REQ *prOutReq = NULL;
	int32_t ret = 0, i;

	if (kalIsResetting()) {
		DBGLOG_LIMITED(HAL, WARN, "is resetting\n");
		ret = -EFAULT;
		goto exit;
	}

	GLUE_INC_REF_CNT(glue->u4HifRegReqCnt);

	if (!glue->prHifRegFifoBuf) {
		DBGLOG(HAL, ERROR, "fifo is free\n");
		ret = -EFAULT;
		goto exit;
	}

	prReq->eStatus = WF_REG_PENDING;
	if (KAL_FIFO_IN_LOCKED(&glue->rHifRegFifo, prReq,
			       &glue->rHifRegFifoLock)) {
		kalHifRegWorkSchedule(glue);
	} else {
		DBGLOG_LIMITED(HAL, WARN,
			"op: %d cr fifo full addr: %X, value: %X\n",
			prReq->eOp, prReq->u4Addr, prReq->u4Val);
		ret = -EFAULT;
		goto exit;
	}

	for (i = 0; i < HIF_REG_WORK_WAIT_CNT; i++) {
		if (prReq->eStatus != WF_REG_PENDING)
			break;

		kalUsleep(HIF_REG_WORK_WAIT_TIME);
	}

	while (KAL_FIFO_OUT_LOCKED(
		       &glue->rHifRegFifo, prOutReq,
		       &glue->rHifRegFifoLock) == sizeof(prOutReq)) {
		if (prReq == prOutReq)
			break;
	}

	if (i >= HIF_REG_WORK_WAIT_CNT)
		prReq->eStatus = WF_REG_DROP;

	if (prReq->eStatus != WF_REG_SUCCESS) {
		DBGLOG_LIMITED(HAL, WARN,
			"op: %d cr timeout addr: %X, value: %X, status: %d\n",
			prReq->eOp, prReq->u4Addr, prReq->u4Val,
			prReq->eStatus);
		ret = -EFAULT;
		goto exit;
	}

exit:
	GLUE_DEC_REF_CNT(glue->u4HifRegReqCnt);

	return ret;
}

int32_t wf_reg_sanity_check(struct GLUE_INFO *glue)
{
	struct ADAPTER *ad;
	struct CHIP_DBG_OPS *prDebugOps;
	bool dumpViaBt = FALSE;
	int32_t ret = 0;

	if (!g_fgIsEnHifRegWork) {
		DBGLOG_LIMITED(HAL, WARN, "work is uninit\n");
		ret = -EFAULT;
		goto exit;
	}

	if (!glue) {
		DBGLOG_LIMITED(HAL, WARN, "NULL GLUE.\n");
		ret = -EFAULT;
		goto exit;
	}

	if (!glue->prHifRegFifoBuf) {
		DBGLOG(HAL, ERROR, "fifo is free\n");
		ret = -EFAULT;
		goto exit;
	}

	ad = glue->prAdapter;
	if (!ad) {
		DBGLOG_LIMITED(HAL, WARN, "NULL ADAPTER.\n");
		ret = -EFAULT;
		goto exit;
	}

	if (!wlanIsDriverReady(glue,
			       WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG_LIMITED(HAL, WARN, "HIF is not ready.\n");
		ret = -EFAULT;
		goto exit;
	}

#if defined(_HIF_PCIE)
	if (!halPcieIsPcieProbed()) {
		DBGLOG_LIMITED(HAL, WARN, "PCIe not ready\n");
		ret = -EFAULT;
		goto exit;
	}

	if (pcie_check_status_is_linked() == FALSE) {
		ret = -EFAULT;
		goto exit;
	}
#endif

	prDebugOps = ad->chip_info->prDebugOps;
	if (prDebugOps && prDebugOps->checkDumpViaBt)
		dumpViaBt = prDebugOps->checkDumpViaBt(ad);

	if (dumpViaBt) {
		DBGLOG_LIMITED(HAL, WARN, "PCIe AER.\n");
		ret = -EFAULT;
		goto exit;
	}
exit:
	return ret;
}

int32_t wf_reg_read_wrapper(void *priv, uint32_t addr, uint32_t *value)
{
	struct GLUE_INFO *glue = priv;
	struct WF_REG_REQ rReq = {0}, *prReq = &rReq;
	int32_t ret = 0;

	ret = wf_reg_sanity_check(glue);
	if (ret)
		goto exit;

	prReq->eOp = WF_REG_READ;
	prReq->u4Addr = addr;
	prReq->u4Val = 0;
	ret = wf_reg_handle_req(glue, prReq);
	*value = prReq->u4Val;

exit:
	return ret;
}

int32_t wf_reg_write_wrapper(void *priv, uint32_t addr, uint32_t value)
{
	struct GLUE_INFO *glue = priv;
	struct WF_REG_REQ rReq = {0}, *prReq = &rReq;
	int32_t ret = 0;

	ret = wf_reg_sanity_check(glue);
	if (ret)
		goto exit;

	prReq->eOp = WF_REG_WRITE;
	prReq->u4Addr = addr;
	prReq->u4Val = value;
	ret = wf_reg_handle_req(glue, prReq);

exit:
	return ret;
}

int32_t wf_reg_write_mask_wrapper(
	void *priv, uint32_t addr, uint32_t mask, uint32_t value)
{
	struct GLUE_INFO *glue = priv;
	struct WF_REG_REQ rReq = {0}, *prReq = &rReq;
	int32_t ret = 0;

	ret = wf_reg_sanity_check(glue);
	if (ret)
		goto exit;

	prReq->eOp = WF_REG_READ;
	prReq->u4Addr = addr;
	ret = wf_reg_handle_req(glue, prReq);
	if (ret)
		goto exit;

	prReq->eOp = WF_REG_WRITE;
	prReq->u4Val &= ~mask;
	prReq->u4Val |= value;
	ret = wf_reg_handle_req(glue, prReq);

exit:
	return ret;
}

int32_t wf_reg_start_wrapper(enum connv3_drv_type from_drv, void *priv_data)
{
	struct GLUE_INFO *prGlueInfo = priv_data;
	int32_t ret = 0;

	ret = wf_reg_sanity_check(prGlueInfo);
	if (ret)
		goto exit;

#if CFG_MTK_WIFI_PCIE_SR
	if (!fgIsL2Finished) {
		DBGLOG_LIMITED(HAL, WARN, "L2 Not finished.\n");
		ret = -EFAULT;
		goto exit;
	}
#endif

	if (kalIsResetting() && glGetRstReason() == RST_DRV_OWN_FAIL) {
		DBGLOG_LIMITED(HAL, WARN, "Reset Reason: RST_DRV_OWN_FAIL\n");
		ret = -EFAULT;
		goto exit;
	}

	ACQUIRE_POWER_CONTROL_FROM_PM(prGlueInfo->prAdapter,
		DRV_OWN_SRC_REG_START_WRAPPER);
	if (prGlueInfo->prAdapter->fgIsFwOwn == TRUE) {
		DBGLOG_LIMITED(HAL, WARN,
			"Driver own fail.\n");
		ret = -EFAULT;
		goto exit;
	}

	GLUE_INC_REF_CNT(prGlueInfo->u4HifRegStartCnt);
	DBGLOG(HAL, DEBUG, "PwrCtrlBlockCnt[%u] HifRegStartCnt[%u]\n",
	       prGlueInfo->prAdapter->u4PwrCtrlBlockCnt,
	       prGlueInfo->u4HifRegStartCnt);

exit:
	return ret;
}

int32_t wf_reg_end_wrapper(enum connv3_drv_type from_drv, void *priv_data)
{
	struct GLUE_INFO *prGlueInfo = priv_data;
	int32_t ret = 0;

	ret = wf_reg_sanity_check(prGlueInfo);
	if (ret)
		goto exit;

	RECLAIM_POWER_CONTROL_TO_PM(prGlueInfo->prAdapter,
		FALSE,
		DRV_OWN_SRC_REG_START_WRAPPER);

	GLUE_DEC_REF_CNT(prGlueInfo->u4HifRegStartCnt);
	DBGLOG(HAL, DEBUG, "PwrCtrlBlockCnt[%u] HifRegStartCnt[%u]\n",
	       prGlueInfo->prAdapter->u4PwrCtrlBlockCnt,
	       prGlueInfo->u4HifRegStartCnt);
exit:
	return ret;
}

void halHandleHifRegReq(struct GLUE_INFO *prGlueInfo)
{
	struct WF_REG_REQ *prReq = NULL;
	struct kfifo *prHifRegFifo = NULL;
	spinlock_t *prHifRegFifoLock;

	if (!prGlueInfo) {
		DBGLOG_LIMITED(HAL, WARN, "glue is null\n");
		return;
	}
	prHifRegFifo = &prGlueInfo->rHifRegFifo;
	prHifRegFifoLock = &prGlueInfo->rHifRegFifoLock;

	while (KAL_FIFO_OUT_LOCKED(prHifRegFifo, prReq, prHifRegFifoLock) ==
		sizeof(prReq)) {
		if (!prReq) {
			DBGLOG(HAL, ERROR, "prReq is null\n");
			break;
		}

		if (!prGlueInfo) {
			prReq->eStatus = WF_REG_FAILURE;
			DBGLOG_LIMITED(HAL, WARN, "glue is null\n");
			continue;
		}

		if (fgIsBusAccessFailed) {
			prReq->eStatus = WF_REG_FAILURE;
			DBGLOG_LIMITED(HAL, WARN, "BusAccessFailed\n");
			continue;
		}

		if (prReq->eStatus == WF_REG_DROP) {
			prReq->eStatus = WF_REG_FAILURE;
			DBGLOG_LIMITED(HAL, WARN, "req drop\n");
			continue;
		}

		if (prReq->eOp == WF_REG_READ) {
#if CFG_MTK_WIFI_MBU
			u_int8_t fgRet = TRUE;

			if (prGlueInfo->u4MbuTimeoutCnt < MBU_MAX_TIMEOUT_CNT) {
				HAL_MCR_EMI_RD(
					prGlueInfo->prAdapter,
					prReq->u4Addr,
					&prReq->u4Val,
					&fgRet);
				if (!fgRet)
					prGlueInfo->u4MbuTimeoutCnt++;
			} else {
				fgRet = FALSE;
			}
			if (!fgRet)
#endif
				HAL_RMCR_RD(HIF_BT_DBG, prGlueInfo->prAdapter,
					    prReq->u4Addr, &prReq->u4Val);
		} else if (prReq->eOp == WF_REG_WRITE) {
			HAL_MCR_WR(prGlueInfo->prAdapter,
				   prReq->u4Addr, prReq->u4Val);
		}
		prReq->eStatus = WF_REG_SUCCESS;
	}
}
#endif /* CFG_SUPPORT_HIF_REG_WORK */

