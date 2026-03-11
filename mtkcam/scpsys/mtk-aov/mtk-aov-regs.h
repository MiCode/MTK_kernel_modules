/* SPDX-License-Identifier: GPL-2.0 */
// Copyright (c) 2019 MediaTek Inc.

#ifndef __MTK_AOV_REGS_H__
#define __MTK_AOV_REGS_H__

#define DRV_Reg32(addr) readl(addr)
#define DRV_WriteReg32(addr, val) writel(val, addr)
#define DRV_SetReg32(addr, val) DRV_WriteReg32(addr, DRV_Reg32(addr) | (val))
#define DRV_ClrReg32(addr, val) DRV_WriteReg32(addr, DRV_Reg32(addr) & ~(val))

#define DRV_WriteReg32_Mask(addr, val, msk, shift) \
		DRV_WriteReg32((addr), ((DRV_Reg32(addr) & (~(msk))) | ((val << shift) & (msk))))

#endif //__MTK_AOV_REGS_H__
