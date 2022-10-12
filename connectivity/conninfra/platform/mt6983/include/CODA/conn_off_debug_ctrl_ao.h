/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
//[File]            : conn_off_debug_ctrl_ao.h
//[Revision time]   : Tue Apr 13 16:35:48 2021

#ifndef __CONN_OFF_DEBUG_CTRL_AO_REGS_H__
#define __CONN_OFF_DEBUG_CTRL_AO_REGS_H__

//****************************************************************************
//
//                     CONN_OFF_DEBUG_CTRL_AO CR Definitions                     
//
//****************************************************************************
#define CONN_OFF_DEBUG_CTRL_AO_BASE                            (CONN_REG_CONN_INFRA_OFF_DEBUG_CTRL_AO_ADDR)

#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL0_ADDR (CONN_OFF_DEBUG_CTRL_AO_BASE + 0x000) // D000
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL1_ADDR (CONN_OFF_DEBUG_CTRL_AO_BASE + 0x004) // D004
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL2_ADDR (CONN_OFF_DEBUG_CTRL_AO_BASE + 0x008) // D008
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT0_ADDR (CONN_OFF_DEBUG_CTRL_AO_BASE + 0x400) // D400
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT1_ADDR (CONN_OFF_DEBUG_CTRL_AO_BASE + 0x404) // D404
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT2_ADDR (CONN_OFF_DEBUG_CTRL_AO_BASE + 0x408) // D408
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT3_ADDR (CONN_OFF_DEBUG_CTRL_AO_BASE + 0x40C) // D40C
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT4_ADDR (CONN_OFF_DEBUG_CTRL_AO_BASE + 0x410) // D410
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT5_ADDR (CONN_OFF_DEBUG_CTRL_AO_BASE + 0x414) // D414
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT6_ADDR (CONN_OFF_DEBUG_CTRL_AO_BASE + 0x418) // D418




/* =====================================================================================

  ---CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL0 (0x1804D000 + 0x000)---

    timeout[0]                   - (RO) Timeout
    timeout_type[1]              - (RW) Method of timeout record
    debug_en[2]                  - (RW)  xxx 
    debug_cken[3]                - (RW)  xxx 
    debug_en_debugtop[4]         - (RW)  xxx 
    clk_detect[7..5]             - (RO)  xxx 
    timeout_irq[8]               - (RO)  xxx 
    timeout_clr[9]               - (RW)  xxx 
    RESERVED10[15..10]           - (RO) Reserved bits
    timeout_thres[31..16]        - (RW) Setting timeout threshold

 =====================================================================================*/
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL0_timeout_thres_ADDR CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL0_ADDR
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL0_timeout_thres_MASK 0xFFFF0000                // timeout_thres[31..16]
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL0_timeout_thres_SHFT 16
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL0_timeout_clr_ADDR CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL0_ADDR
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL0_timeout_clr_MASK 0x00000200                // timeout_clr[9]
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL0_timeout_clr_SHFT 9
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL0_timeout_irq_ADDR CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL0_ADDR
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL0_timeout_irq_MASK 0x00000100                // timeout_irq[8]
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL0_timeout_irq_SHFT 8
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL0_clk_detect_ADDR CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL0_ADDR
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL0_clk_detect_MASK 0x000000E0                // clk_detect[7..5]
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL0_clk_detect_SHFT 5
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL0_debug_en_debugtop_ADDR CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL0_ADDR
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL0_debug_en_debugtop_MASK 0x00000010                // debug_en_debugtop[4]
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL0_debug_en_debugtop_SHFT 4
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL0_debug_cken_ADDR CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL0_ADDR
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL0_debug_cken_MASK 0x00000008                // debug_cken[3]
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL0_debug_cken_SHFT 3
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL0_debug_en_ADDR CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL0_ADDR
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL0_debug_en_MASK 0x00000004                // debug_en[2]
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL0_debug_en_SHFT 2
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL0_timeout_type_ADDR CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL0_ADDR
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL0_timeout_type_MASK 0x00000002                // timeout_type[1]
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL0_timeout_type_SHFT 1
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL0_timeout_ADDR CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL0_ADDR
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL0_timeout_MASK 0x00000001                // timeout[0]
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL0_timeout_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL1 (0x1804D000 + 0x004)---

    idle0_mask[31..0]            - (RW) Mask idle0 trigger

 =====================================================================================*/
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL1_idle0_mask_ADDR CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL1_ADDR
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL1_idle0_mask_MASK 0xFFFFFFFF                // idle0_mask[31..0]
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL1_idle0_mask_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL2 (0x1804D000 + 0x008)---

    idle1_mask[0]                - (RW) Mask idle1 trigger
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL2_idle1_mask_ADDR CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL2_ADDR
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL2_idle1_mask_MASK 0x00000001                // idle1_mask[0]
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL2_idle1_mask_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT0 (0x1804D000 + 0x400)---

    sys_timer_value_0[31..0]     - (RO) Time stamp values

 =====================================================================================*/
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT0_sys_timer_value_0_ADDR CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT0_ADDR
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT0_sys_timer_value_0_MASK 0xFFFFFFFF                // sys_timer_value_0[31..0]
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT0_sys_timer_value_0_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT1 (0x1804D000 + 0x404)---

    sys_timer_value_1[31..0]     - (RO) Time stamp values

 =====================================================================================*/
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT1_sys_timer_value_1_ADDR CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT1_ADDR
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT1_sys_timer_value_1_MASK 0xFFFFFFFF                // sys_timer_value_1[31..0]
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT1_sys_timer_value_1_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT2 (0x1804D000 + 0x408)---

    debug_result2[31..0]         - (RO) debug signal values

 =====================================================================================*/
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT2_debug_result2_ADDR CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT2_ADDR
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT2_debug_result2_MASK 0xFFFFFFFF                // debug_result2[31..0]
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT2_debug_result2_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT3 (0x1804D000 + 0x40C)---

    debug_result3[31..0]         - (RO) debug signal values

 =====================================================================================*/
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT3_debug_result3_ADDR CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT3_ADDR
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT3_debug_result3_MASK 0xFFFFFFFF                // debug_result3[31..0]
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT3_debug_result3_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT4 (0x1804D000 + 0x410)---

    debug_result4[31..0]         - (RO) debug signal values

 =====================================================================================*/
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT4_debug_result4_ADDR CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT4_ADDR
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT4_debug_result4_MASK 0xFFFFFFFF                // debug_result4[31..0]
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT4_debug_result4_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT5 (0x1804D000 + 0x414)---

    debug_result5[31..0]         - (RO) debug signal values

 =====================================================================================*/
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT5_debug_result5_ADDR CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT5_ADDR
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT5_debug_result5_MASK 0xFFFFFFFF                // debug_result5[31..0]
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT5_debug_result5_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT6 (0x1804D000 + 0x418)---

    debug_result6[31..0]         - (RO) debug signal values

 =====================================================================================*/
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT6_debug_result6_ADDR CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT6_ADDR
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT6_debug_result6_MASK 0xFFFFFFFF                // debug_result6[31..0]
#define CONN_OFF_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT6_debug_result6_SHFT 0

#endif // __CONN_OFF_DEBUG_CTRL_AO_REGS_H__
