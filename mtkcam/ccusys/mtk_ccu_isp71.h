/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef __RPOC_MTK_CCU_IPS7_H
#define __RPOC_MTK_CCU_IPS7_H

#include <linux/kernel.h>
#include <linux/remoteproc.h>
#include <linux/wait.h>
#include <linux/types.h>

#include "mtk_ccu_common.h"

#define MTK_CCU_CORE_PMEM_BASE  (0x00000000)
#define MTK_CCU_CORE_DMEM_BASE  (0x00020000)
#define MTK_CCU_CORE_DMEM_BASE_ISP7SP  (0x00040000)
#define MTK_CCU_PMEM_BASE  (0x1B000000)
#define MTK_CCU_DMEM_BASE  (0x1B020000)
#define MTK_CCU_PMEM_SIZE  (0x20000)
#define MTK_CCU_DMEM_SIZE  (0x20000)
#define MTK_CCU_ISR_LOG_SIZE  (0x400)
#define MTK_CCU_LOG_SIZE  (0x800)
#define MTK_CCU_CACHE_SIZE  (0x100000)
#define MTK_CCU_CACHE_BASE (0x40000000)
#define MTK_CCU_SHARED_BUF_OFFSET 0 //at DCCM start
#define MTK_CCU_BASE_MASK  (0xFFF00000)

#define MTK_CCU_REG_RESET    (0x0)
#define MTK_CCU_HW_RESET_BIT (0x000d0100)
#define MTK_CCU_HW_RESET_BIT_ISP7SP (0x000d0300)
#define MTK_CCU_REG_CTRL     (0x0c)
#define MTK_CCU_REG_AXI_REMAP  (0x24)
#define MTK_CCU_REG_CORE_CTRL  (0x28)
#define MTK_CCU_RUN_BIT      (0x00000010)
#define MTK_CCU_REG_CORE_STATUS     (0x28)
#define MTK_CCU_INT_TRG         (0x8010)
#define MTK_CCU_INT_TRG_ISP7SP  (0x80C0)
#define MTK_CCU_INT_TRG_ISP8    (0x00A0)
#define MTK_CCU_INT_CLR         (0x5C)
#define MTK_CCU_INT_CLR_EXCH    (0x80A4)
#define MTK_CCU_INT_CLR_ISP8    (0x84)
#define MTK_CCU_INT_ST          (0x60)
#define MTK_CCU_MON_ST          (0x78)
#define HALT_MASK_RV33          (0x30)
#define HALT_MASK_RV55          (0x2100)

#define CCU_EXCH_OFFSET       (0x8000)
#define CCU_EXCH_SIZE         (0x1000)
#define CCU_EXCH_SIZE_IPC     (0x10000)
#define CCU_IPC_SRAM_OFFSET   (0xC000)
#define CCU_IPC_SRAM_SIZE     (0x1000)
#define SPM_BASE              (0x1C001000)
#define SPM_BASE_ISP8         (0x1C004000)
#define SPM_SIZE              (0x1000)
#define MMPC_BASE             (0x31B50000)
#define MMPC_SIZE             (0x1000)
#define CCU_SLEEP_SRAM_CON    (0xF54)
#define CCU_SLEEP_SRAM_PDN    (0x1 << 8)
#define CCU_RESOURCE_OFFSET   (0x82C)
#define CCU_RESOURCE_BITS     (0x0B000000)

#define MTK_CCU_SPARE_REG00   (0x00)
#define MTK_CCU_SPARE_REG01   (0x04)
#define MTK_CCU_SPARE_REG02   (0x08)
#define MTK_CCU_SPARE_REG03   (0x0C)
#define MTK_CCU_SPARE_REG04   (0x10)
#define MTK_CCU_SPARE_REG05   (0x14)
#define MTK_CCU_SPARE_REG06   (0x18)
#define MTK_CCU_SPARE_REG07   (0x1C)
#define MTK_CCU_SPARE_REG08   (0x20)
#define MTK_CCU_SPARE_REG09   (0x24)
#define MTK_CCU_SPARE_REG10   (0x28)
#define MTK_CCU_SPARE_REG11   (0x2C)
#define MTK_CCU_SPARE_REG12   (0x30)
#define MTK_CCU_SPARE_REG13   (0x34)
#define MTK_CCU_SPARE_REG14   (0x38)
#define MTK_CCU_SPARE_REG15   (0x3C)
#define MTK_CCU_SPARE_REG16   (0x40)
#define MTK_CCU_SPARE_REG17   (0x44)
#define MTK_CCU_SPARE_REG18   (0x48)
#define MTK_CCU_SPARE_REG19   (0x4C)
#define MTK_CCU_SPARE_REG20   (0x50)
#define MTK_CCU_SPARE_REG21   (0x54)
#define MTK_CCU_SPARE_REG22   (0x58)
#define MTK_CCU_SPARE_REG23   (0x5C)
#define MTK_CCU_SPARE_REG24   (0x60)
#define MTK_CCU_SPARE_REG25   (0x64)
#define MTK_CCU_SPARE_REG26   (0x68)
#define MTK_CCU_SPARE_REG27   (0x6C)
#define MTK_CCU_SPARE_REG28   (0x70)
#define MTK_CCU_SPARE_REG29   (0x74)
#define MTK_CCU_SPARE_REG30   (0x78)
#define MTK_CCU_SPARE_REG31   (0x7C)

#define CCU_STATUS_INIT_DONE              0xffff0000
#define CCU_STATUS_INIT_DONE_2            0xffff00a5
#define CCU_GO_TO_LOAD                    0x10AD10AD
#define CCU_GO_TO_RUN                     0x17172ACE
#define CCU_GO_TO_STOP                    0x8181DEAD

uint32_t read_ccu_info_regd(struct mtk_ccu *ccu, uint32_t addr);

#endif //__RPOC_MTK_CCU_IPS7_H
