/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
//[File]            : conn_wt_slp_ctl_reg.h
//[Revision time]   : Mon Aug 30 16:01:06 2021

#ifndef __CONN_WT_SLP_CTL_REG_REGS_H__
#define __CONN_WT_SLP_CTL_REG_REGS_H__

//****************************************************************************
//
//                     CONN_WT_SLP_CTL_REG CR Definitions                     
//
//****************************************************************************

#define CONN_WT_SLP_CTL_REG_BASE                               (CONN_REG_CONN_WT_SLP_CTL_REG_ADDR)

#define CONN_WT_SLP_CTL_REG_WB_SLP_TRG_ADDR                    (CONN_WT_SLP_CTL_REG_BASE + 0x000) // 3000
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_ADDR                    (CONN_WT_SLP_CTL_REG_BASE + 0x004) // 3004
#define CONN_WT_SLP_CTL_REG_WB_STA_ADDR                        (CONN_WT_SLP_CTL_REG_BASE + 0x008) // 3008
#define CONN_WT_SLP_CTL_REG_WB_SLP_TMOUT_ADDR                  (CONN_WT_SLP_CTL_REG_BASE + 0x00C) // 300C
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR1_ADDR                   (CONN_WT_SLP_CTL_REG_BASE + 0x010) // 3010
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR2_ADDR                   (CONN_WT_SLP_CTL_REG_BASE + 0x014) // 3014
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR3_ADDR                   (CONN_WT_SLP_CTL_REG_BASE + 0x018) // 3018
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR4_ADDR                   (CONN_WT_SLP_CTL_REG_BASE + 0x01C) // 301C
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR5_ADDR                   (CONN_WT_SLP_CTL_REG_BASE + 0x020) // 3020
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR6_ADDR                   (CONN_WT_SLP_CTL_REG_BASE + 0x024) // 3024
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR7_ADDR                   (CONN_WT_SLP_CTL_REG_BASE + 0x028) // 3028
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR8_ADDR                   (CONN_WT_SLP_CTL_REG_BASE + 0x02C) // 302C
#define CONN_WT_SLP_CTL_REG_WB_BG_ON1_ADDR                     (CONN_WT_SLP_CTL_REG_BASE + 0x030) // 3030
#define CONN_WT_SLP_CTL_REG_WB_BG_ON2_ADDR                     (CONN_WT_SLP_CTL_REG_BASE + 0x034) // 3034
#define CONN_WT_SLP_CTL_REG_WB_BG_ON3_ADDR                     (CONN_WT_SLP_CTL_REG_BASE + 0x038) // 3038
#define CONN_WT_SLP_CTL_REG_WB_BG_ON4_ADDR                     (CONN_WT_SLP_CTL_REG_BASE + 0x03C) // 303C
#define CONN_WT_SLP_CTL_REG_WB_BG_ON5_ADDR                     (CONN_WT_SLP_CTL_REG_BASE + 0x040) // 3040
#define CONN_WT_SLP_CTL_REG_WB_BG_ON6_ADDR                     (CONN_WT_SLP_CTL_REG_BASE + 0x044) // 3044
#define CONN_WT_SLP_CTL_REG_WB_BG_ON7_ADDR                     (CONN_WT_SLP_CTL_REG_BASE + 0x048) // 3048
#define CONN_WT_SLP_CTL_REG_WB_BG_ON8_ADDR                     (CONN_WT_SLP_CTL_REG_BASE + 0x04C) // 304C
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF1_ADDR                    (CONN_WT_SLP_CTL_REG_BASE + 0x050) // 3050
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF2_ADDR                    (CONN_WT_SLP_CTL_REG_BASE + 0x054) // 3054
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF3_ADDR                    (CONN_WT_SLP_CTL_REG_BASE + 0x058) // 3058
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF4_ADDR                    (CONN_WT_SLP_CTL_REG_BASE + 0x05C) // 305C
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF5_ADDR                    (CONN_WT_SLP_CTL_REG_BASE + 0x060) // 3060
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF6_ADDR                    (CONN_WT_SLP_CTL_REG_BASE + 0x064) // 3064
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF7_ADDR                    (CONN_WT_SLP_CTL_REG_BASE + 0x068) // 3068
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF8_ADDR                    (CONN_WT_SLP_CTL_REG_BASE + 0x06C) // 306C
#define CONN_WT_SLP_CTL_REG_WB_WF_CK_ADDR_ADDR                 (CONN_WT_SLP_CTL_REG_BASE + 0x070) // 3070
#define CONN_WT_SLP_CTL_REG_WB_WF_WAKE_ADDR_ADDR               (CONN_WT_SLP_CTL_REG_BASE + 0x074) // 3074
#define CONN_WT_SLP_CTL_REG_WB_WF_ZPS_ADDR_ADDR                (CONN_WT_SLP_CTL_REG_BASE + 0x078) // 3078
#define CONN_WT_SLP_CTL_REG_WB_BT_CK_ADDR_ADDR                 (CONN_WT_SLP_CTL_REG_BASE + 0x07C) // 307C
#define CONN_WT_SLP_CTL_REG_WB_BT_WAKE_ADDR_ADDR               (CONN_WT_SLP_CTL_REG_BASE + 0x080) // 3080
#define CONN_WT_SLP_CTL_REG_WB_TOP_CK_ADDR_ADDR                (CONN_WT_SLP_CTL_REG_BASE + 0x084) // 3084
#define CONN_WT_SLP_CTL_REG_WB_GPS_CK_ADDR_ADDR                (CONN_WT_SLP_CTL_REG_BASE + 0x088) // 3088
#define CONN_WT_SLP_CTL_REG_WB_WF_B0_CMD_ADDR_ADDR             (CONN_WT_SLP_CTL_REG_BASE + 0x08c) // 308C
#define CONN_WT_SLP_CTL_REG_WB_WF_B1_CMD_ADDR_ADDR             (CONN_WT_SLP_CTL_REG_BASE + 0x090) // 3090
#define CONN_WT_SLP_CTL_REG_WB_GPS_RFBUF_ADR_ADDR              (CONN_WT_SLP_CTL_REG_BASE + 0x094) // 3094
#define CONN_WT_SLP_CTL_REG_WB_GPS_L5_EN_ADDR_ADDR             (CONN_WT_SLP_CTL_REG_BASE + 0x098) // 3098
#define CONN_WT_SLP_CTL_REG_WB_STA1_ADDR                       (CONN_WT_SLP_CTL_REG_BASE + 0x0A0) // 30A0
#define CONN_WT_SLP_CTL_REG_WB_RSV_ADDR                        (CONN_WT_SLP_CTL_REG_BASE + 0x0A4) // 30A4
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_ADDR                     (CONN_WT_SLP_CTL_REG_BASE + 0x0A8) // 30A8
#define CONN_WT_SLP_CTL_REG_WB_FAKE_CK_EN_TOP_ADDR             (CONN_WT_SLP_CTL_REG_BASE + 0x100) // 3100
#define CONN_WT_SLP_CTL_REG_WB_FAKE_CK_EN_WF_ADDR              (CONN_WT_SLP_CTL_REG_BASE + 0x104) // 3104
#define CONN_WT_SLP_CTL_REG_WB_FAKE_CK_EN_BT_ADDR              (CONN_WT_SLP_CTL_REG_BASE + 0x108) // 3108
#define CONN_WT_SLP_CTL_REG_WB_BT1_CK_ADDR_ADDR                (CONN_WT_SLP_CTL_REG_BASE + 0x10C) // 310C
#define CONN_WT_SLP_CTL_REG_WB_BT1_WAKE_ADDR_ADDR              (CONN_WT_SLP_CTL_REG_BASE + 0x110) // 3110
#define CONN_WT_SLP_CTL_REG_WB_FAKE_CK_EN_BT1_ADDR             (CONN_WT_SLP_CTL_REG_BASE + 0x114) // 3114
#define CONN_WT_SLP_CTL_REG_WB_STA2_ADDR                       (CONN_WT_SLP_CTL_REG_BASE + 0x118) // 3118
#define CONN_WT_SLP_CTL_REG_WB_SLP_DEBUG_ADDR                  (CONN_WT_SLP_CTL_REG_BASE + 0X11c) // 311C
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_0_ADDR               (CONN_WT_SLP_CTL_REG_BASE + 0x120) // 3120
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_1_ADDR               (CONN_WT_SLP_CTL_REG_BASE + 0x124) // 3124
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_2_ADDR               (CONN_WT_SLP_CTL_REG_BASE + 0x128) // 3128
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_3_ADDR               (CONN_WT_SLP_CTL_REG_BASE + 0x12C) // 312C
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_4_ADDR               (CONN_WT_SLP_CTL_REG_BASE + 0x130) // 3130
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_5_ADDR               (CONN_WT_SLP_CTL_REG_BASE + 0x134) // 3134
#define CONN_WT_SLP_CTL_REG_WB_SLP_MULT_TOP_CK_0_ADDR_ADDR     (CONN_WT_SLP_CTL_REG_BASE + 0x138) // 3138
#define CONN_WT_SLP_CTL_REG_WB_SLP_MULT_TOP_CK_1_ADDR_ADDR     (CONN_WT_SLP_CTL_REG_BASE + 0x13C) // 313C
#define CONN_WT_SLP_CTL_REG_WB_SLP_MULT_TOP_CK_2_ADDR_ADDR     (CONN_WT_SLP_CTL_REG_BASE + 0x140) // 3140
#define CONN_WT_SLP_CTL_REG_WB_SLP_MULT_TOP_CK_3_ADDR_ADDR     (CONN_WT_SLP_CTL_REG_BASE + 0x144) // 3144
#define CONN_WT_SLP_CTL_REG_WB_SLP_MULT_TOP_CK_4_ADDR_ADDR     (CONN_WT_SLP_CTL_REG_BASE + 0x148) // 3148
#define CONN_WT_SLP_CTL_REG_WB_SLP_MULT_TOP_CK_5_ADDR_ADDR     (CONN_WT_SLP_CTL_REG_BASE + 0x14C) // 314C
#define CONN_WT_SLP_CTL_REG_WB_STA3_ADDR                       (CONN_WT_SLP_CTL_REG_BASE + 0x150) // 3150
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR9_ADDR                   (CONN_WT_SLP_CTL_REG_BASE + 0x154) // 3154
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR10_ADDR                  (CONN_WT_SLP_CTL_REG_BASE + 0x158) // 3158
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR11_ADDR                  (CONN_WT_SLP_CTL_REG_BASE + 0x15c) // 315C
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR12_ADDR                  (CONN_WT_SLP_CTL_REG_BASE + 0x160) // 3160
#define CONN_WT_SLP_CTL_REG_WB_BG_ON9_ADDR                     (CONN_WT_SLP_CTL_REG_BASE + 0x164) // 3164
#define CONN_WT_SLP_CTL_REG_WB_BG_ON10_ADDR                    (CONN_WT_SLP_CTL_REG_BASE + 0x168) // 3168
#define CONN_WT_SLP_CTL_REG_WB_BG_ON11_ADDR                    (CONN_WT_SLP_CTL_REG_BASE + 0x16C) // 316C
#define CONN_WT_SLP_CTL_REG_WB_BG_ON12_ADDR                    (CONN_WT_SLP_CTL_REG_BASE + 0x170) // 3170
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF9_ADDR                    (CONN_WT_SLP_CTL_REG_BASE + 0x174) // 3174
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF10_ADDR                   (CONN_WT_SLP_CTL_REG_BASE + 0x178) // 3178
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF11_ADDR                   (CONN_WT_SLP_CTL_REG_BASE + 0x17C) // 317C
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF12_ADDR                   (CONN_WT_SLP_CTL_REG_BASE + 0x180) // 3180
#define CONN_WT_SLP_CTL_REG_WB_GPS_CTL_ADDR                    (CONN_WT_SLP_CTL_REG_BASE + 0x200) // 3200




/* =====================================================================================

  ---WB_SLP_TRG (0x18003000 + 0x000)---

    WB_SLP_RST[0]                - (W1T) WB Sleep Control local reset. Wite "1" to reset. This is 1T trigger signals. No need to write "0" to release.
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_SLP_TRG_WB_SLP_RST_ADDR         CONN_WT_SLP_CTL_REG_WB_SLP_TRG_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_TRG_WB_SLP_RST_MASK         0x00000001                // WB_SLP_RST[0]
#define CONN_WT_SLP_CTL_REG_WB_SLP_TRG_WB_SLP_RST_SHFT         0

/* =====================================================================================

  ---WB_SLP_CTL (0x18003000 + 0x004)---

    CMD_LENGTH[4..0]             - (RW) Output length of BG ON/OFF commands.
                                     "0" : Means no command will be assert.
                                     "1" : Means send 1 set of ON/OFF command.
                                     "2" : Means send 2 set of ON/OFF command.
                                     "3" : Means send 3 set of ON/OFF command.
                                     "4" : Means send 4 set of ON/OFF command.
                                     "5" : Means send 5 set of ON/OFF command.
                                     "6" : Means send 6 set of ON/OFF command.
                                     "7" : Means send 7 set of ON/OFF command.
                                     "8" : Means send 8 set of ON/OFF command.
                                     others are reserved.
    RESERVED5[8..5]              - (RO) Reserved bits
    TOP_SG_HW_MODE_EN[9]         - (RW) TOP_SG hardware mode enable.
    BT1_HW_MODE_EN[10]           - (RW) BT1 hardware mode enable.
    BG_HW_MODE_EN[11]            - (RW) Bandgap hardware mode enable.
    TOP_HW_MODE_EN[12]           - (RW) TOP hardware mode enable.
    GPS_HW_MODE_EN[13]           - (RW) GPS hardware mode enable.
    BT_HW_MODE_EN[14]            - (RW) BT hardware mode enable.
    WF_HW_MODE_EN[15]            - (RW) WF hardware mode enable.
    WF_CMD_EN[16]                - (RW) WF LP_CMD enable.
    TOP_MULT_HW_MODE_EN_5[17]    - (RW) TOP_MULT_GPS hardware mode enable.
    TOP_MULT_HW_MODE_EN_4[18]    - (RW) TOP_MULT_FM hardware mode enable.
    TOP_MULT_HW_MODE_EN_3[19]    - (RW) TOP_MULT_BT1 hardware mode enable.
    TOP_MULT_HW_MODE_EN_2[20]    - (RW) TOP_MULT_BT hardware mode enable.
    TOP_MULT_HW_MODE_EN_1[21]    - (RW) TOP_MULT_WF hardware mode enable.
    TOP_MULT_HW_MODE_EN_0[22]    - (RW) TOP_MULT hardware mode enable.
    RESERVED23[31..23]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_TOP_MULT_HW_MODE_EN_0_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_CTL_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_TOP_MULT_HW_MODE_EN_0_MASK 0x00400000                // TOP_MULT_HW_MODE_EN_0[22]
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_TOP_MULT_HW_MODE_EN_0_SHFT 22
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_TOP_MULT_HW_MODE_EN_1_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_CTL_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_TOP_MULT_HW_MODE_EN_1_MASK 0x00200000                // TOP_MULT_HW_MODE_EN_1[21]
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_TOP_MULT_HW_MODE_EN_1_SHFT 21
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_TOP_MULT_HW_MODE_EN_2_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_CTL_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_TOP_MULT_HW_MODE_EN_2_MASK 0x00100000                // TOP_MULT_HW_MODE_EN_2[20]
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_TOP_MULT_HW_MODE_EN_2_SHFT 20
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_TOP_MULT_HW_MODE_EN_3_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_CTL_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_TOP_MULT_HW_MODE_EN_3_MASK 0x00080000                // TOP_MULT_HW_MODE_EN_3[19]
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_TOP_MULT_HW_MODE_EN_3_SHFT 19
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_TOP_MULT_HW_MODE_EN_4_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_CTL_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_TOP_MULT_HW_MODE_EN_4_MASK 0x00040000                // TOP_MULT_HW_MODE_EN_4[18]
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_TOP_MULT_HW_MODE_EN_4_SHFT 18
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_TOP_MULT_HW_MODE_EN_5_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_CTL_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_TOP_MULT_HW_MODE_EN_5_MASK 0x00020000                // TOP_MULT_HW_MODE_EN_5[17]
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_TOP_MULT_HW_MODE_EN_5_SHFT 17
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_WF_CMD_EN_ADDR          CONN_WT_SLP_CTL_REG_WB_SLP_CTL_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_WF_CMD_EN_MASK          0x00010000                // WF_CMD_EN[16]
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_WF_CMD_EN_SHFT          16
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_WF_HW_MODE_EN_ADDR      CONN_WT_SLP_CTL_REG_WB_SLP_CTL_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_WF_HW_MODE_EN_MASK      0x00008000                // WF_HW_MODE_EN[15]
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_WF_HW_MODE_EN_SHFT      15
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_BT_HW_MODE_EN_ADDR      CONN_WT_SLP_CTL_REG_WB_SLP_CTL_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_BT_HW_MODE_EN_MASK      0x00004000                // BT_HW_MODE_EN[14]
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_BT_HW_MODE_EN_SHFT      14
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_GPS_HW_MODE_EN_ADDR     CONN_WT_SLP_CTL_REG_WB_SLP_CTL_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_GPS_HW_MODE_EN_MASK     0x00002000                // GPS_HW_MODE_EN[13]
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_GPS_HW_MODE_EN_SHFT     13
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_TOP_HW_MODE_EN_ADDR     CONN_WT_SLP_CTL_REG_WB_SLP_CTL_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_TOP_HW_MODE_EN_MASK     0x00001000                // TOP_HW_MODE_EN[12]
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_TOP_HW_MODE_EN_SHFT     12
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_BG_HW_MODE_EN_ADDR      CONN_WT_SLP_CTL_REG_WB_SLP_CTL_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_BG_HW_MODE_EN_MASK      0x00000800                // BG_HW_MODE_EN[11]
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_BG_HW_MODE_EN_SHFT      11
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_BT1_HW_MODE_EN_ADDR     CONN_WT_SLP_CTL_REG_WB_SLP_CTL_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_BT1_HW_MODE_EN_MASK     0x00000400                // BT1_HW_MODE_EN[10]
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_BT1_HW_MODE_EN_SHFT     10
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_TOP_SG_HW_MODE_EN_ADDR  CONN_WT_SLP_CTL_REG_WB_SLP_CTL_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_TOP_SG_HW_MODE_EN_MASK  0x00000200                // TOP_SG_HW_MODE_EN[9]
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_TOP_SG_HW_MODE_EN_SHFT  9
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_CMD_LENGTH_ADDR         CONN_WT_SLP_CTL_REG_WB_SLP_CTL_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_CMD_LENGTH_MASK         0x0000001F                // CMD_LENGTH[4..0]
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_CMD_LENGTH_SHFT         0

/* =====================================================================================

  ---WB_STA (0x18003000 + 0x008)---

    B1_WF_ZPS_BUSY[0]            - (RO) WB WiFi B1 zps mode busy bit
                                     "1" mean buys.
    GPS_L5_CK_BUSY[1]            - (RO) WB GPS L5 clock control busy bit
                                     "1" mean buys.
    B1_WF_CK_BUSY[2]             - (RO) WB WiFi B1 clock control busy bit
                                     "1" mean buys.
    B1_WF_SLP_BUSY[3]            - (RO) WB WiFi B1 sleep control busy bit
                                     "1" mean buys.
    GPS_L1_RFBUF_BUSY[4]         - (RO) WB GPS L1 rfbuf control busy bit
                                     "1" mean buys.
    GPS_L5_RFBUF_BUSY[5]         - (RO) WB GPS L5 rfbuf control busy bit
                                     "1" mean buys.
    GPS_L5_EN_BUSY[6]            - (RO) WB GPS L5 sel control busy bit
                                     "1" mean buys.
    WB_SLP_TOP_CK_5_BSY[7]       - (RO) WB SG TOP EN GPS clock control busy bit
                                     "1" mean busy.
    WB_SLP_TOP_CK_4_BSY[8]       - (RO) WB SG TOP EN FM clock control busy bit
                                     "1" mean busy.
    WB_SLP_TOP_CK_3_BSY[9]       - (RO) WB SG TOP EN BT1 clock control busy bit
                                     "1" mean busy.
    WB_SLP_TOP_CK_2_BSY[10]      - (RO) WB SG TOP EN BT clock control busy bit
                                     "1" mean busy.
    BT1_CK_BUSY[11]              - (RO) WB BT1 clock control busy bit
                                     "1" mean buys.
    BT1_SLP_BUSY[12]             - (RO) WB BT1 sleep control busy bit
                                     "1" mean buys.
    WB_SLP_TOP_CK_1_BSY[13]      - (RO) WB SG TOP EN WF clock control busy bit
                                     "1" mean busy.
    WB_SLP_TOP_CK_0_BSY[14]      - (RO) WB SG TOP EN clock control busy bit
                                     "1" mean busy.
    WB_SLP_MULT_TOP_CK_5_BSY[15] - (RO) WB MULT TOP EN GPS clock control busy bit
                                     "1" mean busy.
    WB_SLP_MULT_TOP_CK_4_BSY[16] - (RO) WB MULT TOP EN FM clock control busy bit
                                     "1" mean busy.
    WB_SLP_MULT_TOP_CK_3_BSY[17] - (RO) WB MULT TOP EN BT1 clock control busy bit
                                     "1" mean busy.
    WB_SLP_MULT_TOP_CK_2_BSY[18] - (RO) WB MULT TOP EN BT clock control busy bit
                                     "1" mean busy.
    WB_SLP_MULT_TOP_CK_1_BSY[19] - (RO) WB MULT TOP EN WF clock control busy bit
                                     "1" mean busy.
    WB_SLP_MULT_TOP_CK_0_BSY[20] - (RO) WB MULT TOP EN  clock control busy bit
                                     "1" mean busy.
    B1_WF_CMD_BUSY[21]           - (RO) WB WF B1 CMD busy bit
                                     "1" mean buys.
    B0_WF_CMD_BUSY[22]           - (RO) WB WF CMD busy bit
                                     "1" mean buys.
    WF_ZPS_BUSY[23]              - (RO) WB WiFi zps mode busy bit
                                     "1" mean buys.
    BG_CK_OFF_BUSY[24]           - (RO) WB BG clock off control busy bit
                                     "1" mean buys.
    BG_CK_ON_BUSY[25]            - (RO) WB BG clock on control busy bit
                                     "1" mean buys.
    TOP_CK_BUSY[26]              - (RO) WB TOP clock control busy bit
                                     "1" mean buys.
    GPS_CK_BUSY[27]              - (RO) WB GPS clock control busy bit
                                     "1" mean buys.
    BT_CK_BUSY[28]               - (RO) WB BT clock control busy bit
                                     "1" mean buys.
    WF_CK_BUSY[29]               - (RO) WB WiFi clock control busy bit
                                     "1" mean buys.
    BT_SLP_BUSY[30]              - (RO) WB BT sleep control busy bit
                                     "1" mean buys.
    WF_SLP_BUSY[31]              - (RO) WB WiFi sleep control busy bit
                                     "1" mean buys.

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_STA_WF_SLP_BUSY_ADDR            CONN_WT_SLP_CTL_REG_WB_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA_WF_SLP_BUSY_MASK            0x80000000                // WF_SLP_BUSY[31]
#define CONN_WT_SLP_CTL_REG_WB_STA_WF_SLP_BUSY_SHFT            31
#define CONN_WT_SLP_CTL_REG_WB_STA_BT_SLP_BUSY_ADDR            CONN_WT_SLP_CTL_REG_WB_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA_BT_SLP_BUSY_MASK            0x40000000                // BT_SLP_BUSY[30]
#define CONN_WT_SLP_CTL_REG_WB_STA_BT_SLP_BUSY_SHFT            30
#define CONN_WT_SLP_CTL_REG_WB_STA_WF_CK_BUSY_ADDR             CONN_WT_SLP_CTL_REG_WB_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA_WF_CK_BUSY_MASK             0x20000000                // WF_CK_BUSY[29]
#define CONN_WT_SLP_CTL_REG_WB_STA_WF_CK_BUSY_SHFT             29
#define CONN_WT_SLP_CTL_REG_WB_STA_BT_CK_BUSY_ADDR             CONN_WT_SLP_CTL_REG_WB_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA_BT_CK_BUSY_MASK             0x10000000                // BT_CK_BUSY[28]
#define CONN_WT_SLP_CTL_REG_WB_STA_BT_CK_BUSY_SHFT             28
#define CONN_WT_SLP_CTL_REG_WB_STA_GPS_CK_BUSY_ADDR            CONN_WT_SLP_CTL_REG_WB_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA_GPS_CK_BUSY_MASK            0x08000000                // GPS_CK_BUSY[27]
#define CONN_WT_SLP_CTL_REG_WB_STA_GPS_CK_BUSY_SHFT            27
#define CONN_WT_SLP_CTL_REG_WB_STA_TOP_CK_BUSY_ADDR            CONN_WT_SLP_CTL_REG_WB_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA_TOP_CK_BUSY_MASK            0x04000000                // TOP_CK_BUSY[26]
#define CONN_WT_SLP_CTL_REG_WB_STA_TOP_CK_BUSY_SHFT            26
#define CONN_WT_SLP_CTL_REG_WB_STA_BG_CK_ON_BUSY_ADDR          CONN_WT_SLP_CTL_REG_WB_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA_BG_CK_ON_BUSY_MASK          0x02000000                // BG_CK_ON_BUSY[25]
#define CONN_WT_SLP_CTL_REG_WB_STA_BG_CK_ON_BUSY_SHFT          25
#define CONN_WT_SLP_CTL_REG_WB_STA_BG_CK_OFF_BUSY_ADDR         CONN_WT_SLP_CTL_REG_WB_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA_BG_CK_OFF_BUSY_MASK         0x01000000                // BG_CK_OFF_BUSY[24]
#define CONN_WT_SLP_CTL_REG_WB_STA_BG_CK_OFF_BUSY_SHFT         24
#define CONN_WT_SLP_CTL_REG_WB_STA_WF_ZPS_BUSY_ADDR            CONN_WT_SLP_CTL_REG_WB_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA_WF_ZPS_BUSY_MASK            0x00800000                // WF_ZPS_BUSY[23]
#define CONN_WT_SLP_CTL_REG_WB_STA_WF_ZPS_BUSY_SHFT            23
#define CONN_WT_SLP_CTL_REG_WB_STA_B0_WF_CMD_BUSY_ADDR         CONN_WT_SLP_CTL_REG_WB_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA_B0_WF_CMD_BUSY_MASK         0x00400000                // B0_WF_CMD_BUSY[22]
#define CONN_WT_SLP_CTL_REG_WB_STA_B0_WF_CMD_BUSY_SHFT         22
#define CONN_WT_SLP_CTL_REG_WB_STA_B1_WF_CMD_BUSY_ADDR         CONN_WT_SLP_CTL_REG_WB_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA_B1_WF_CMD_BUSY_MASK         0x00200000                // B1_WF_CMD_BUSY[21]
#define CONN_WT_SLP_CTL_REG_WB_STA_B1_WF_CMD_BUSY_SHFT         21
#define CONN_WT_SLP_CTL_REG_WB_STA_WB_SLP_MULT_TOP_CK_0_BSY_ADDR CONN_WT_SLP_CTL_REG_WB_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA_WB_SLP_MULT_TOP_CK_0_BSY_MASK 0x00100000                // WB_SLP_MULT_TOP_CK_0_BSY[20]
#define CONN_WT_SLP_CTL_REG_WB_STA_WB_SLP_MULT_TOP_CK_0_BSY_SHFT 20
#define CONN_WT_SLP_CTL_REG_WB_STA_WB_SLP_MULT_TOP_CK_1_BSY_ADDR CONN_WT_SLP_CTL_REG_WB_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA_WB_SLP_MULT_TOP_CK_1_BSY_MASK 0x00080000                // WB_SLP_MULT_TOP_CK_1_BSY[19]
#define CONN_WT_SLP_CTL_REG_WB_STA_WB_SLP_MULT_TOP_CK_1_BSY_SHFT 19
#define CONN_WT_SLP_CTL_REG_WB_STA_WB_SLP_MULT_TOP_CK_2_BSY_ADDR CONN_WT_SLP_CTL_REG_WB_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA_WB_SLP_MULT_TOP_CK_2_BSY_MASK 0x00040000                // WB_SLP_MULT_TOP_CK_2_BSY[18]
#define CONN_WT_SLP_CTL_REG_WB_STA_WB_SLP_MULT_TOP_CK_2_BSY_SHFT 18
#define CONN_WT_SLP_CTL_REG_WB_STA_WB_SLP_MULT_TOP_CK_3_BSY_ADDR CONN_WT_SLP_CTL_REG_WB_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA_WB_SLP_MULT_TOP_CK_3_BSY_MASK 0x00020000                // WB_SLP_MULT_TOP_CK_3_BSY[17]
#define CONN_WT_SLP_CTL_REG_WB_STA_WB_SLP_MULT_TOP_CK_3_BSY_SHFT 17
#define CONN_WT_SLP_CTL_REG_WB_STA_WB_SLP_MULT_TOP_CK_4_BSY_ADDR CONN_WT_SLP_CTL_REG_WB_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA_WB_SLP_MULT_TOP_CK_4_BSY_MASK 0x00010000                // WB_SLP_MULT_TOP_CK_4_BSY[16]
#define CONN_WT_SLP_CTL_REG_WB_STA_WB_SLP_MULT_TOP_CK_4_BSY_SHFT 16
#define CONN_WT_SLP_CTL_REG_WB_STA_WB_SLP_MULT_TOP_CK_5_BSY_ADDR CONN_WT_SLP_CTL_REG_WB_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA_WB_SLP_MULT_TOP_CK_5_BSY_MASK 0x00008000                // WB_SLP_MULT_TOP_CK_5_BSY[15]
#define CONN_WT_SLP_CTL_REG_WB_STA_WB_SLP_MULT_TOP_CK_5_BSY_SHFT 15
#define CONN_WT_SLP_CTL_REG_WB_STA_WB_SLP_TOP_CK_0_BSY_ADDR    CONN_WT_SLP_CTL_REG_WB_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA_WB_SLP_TOP_CK_0_BSY_MASK    0x00004000                // WB_SLP_TOP_CK_0_BSY[14]
#define CONN_WT_SLP_CTL_REG_WB_STA_WB_SLP_TOP_CK_0_BSY_SHFT    14
#define CONN_WT_SLP_CTL_REG_WB_STA_WB_SLP_TOP_CK_1_BSY_ADDR    CONN_WT_SLP_CTL_REG_WB_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA_WB_SLP_TOP_CK_1_BSY_MASK    0x00002000                // WB_SLP_TOP_CK_1_BSY[13]
#define CONN_WT_SLP_CTL_REG_WB_STA_WB_SLP_TOP_CK_1_BSY_SHFT    13
#define CONN_WT_SLP_CTL_REG_WB_STA_BT1_SLP_BUSY_ADDR           CONN_WT_SLP_CTL_REG_WB_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA_BT1_SLP_BUSY_MASK           0x00001000                // BT1_SLP_BUSY[12]
#define CONN_WT_SLP_CTL_REG_WB_STA_BT1_SLP_BUSY_SHFT           12
#define CONN_WT_SLP_CTL_REG_WB_STA_BT1_CK_BUSY_ADDR            CONN_WT_SLP_CTL_REG_WB_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA_BT1_CK_BUSY_MASK            0x00000800                // BT1_CK_BUSY[11]
#define CONN_WT_SLP_CTL_REG_WB_STA_BT1_CK_BUSY_SHFT            11
#define CONN_WT_SLP_CTL_REG_WB_STA_WB_SLP_TOP_CK_2_BSY_ADDR    CONN_WT_SLP_CTL_REG_WB_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA_WB_SLP_TOP_CK_2_BSY_MASK    0x00000400                // WB_SLP_TOP_CK_2_BSY[10]
#define CONN_WT_SLP_CTL_REG_WB_STA_WB_SLP_TOP_CK_2_BSY_SHFT    10
#define CONN_WT_SLP_CTL_REG_WB_STA_WB_SLP_TOP_CK_3_BSY_ADDR    CONN_WT_SLP_CTL_REG_WB_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA_WB_SLP_TOP_CK_3_BSY_MASK    0x00000200                // WB_SLP_TOP_CK_3_BSY[9]
#define CONN_WT_SLP_CTL_REG_WB_STA_WB_SLP_TOP_CK_3_BSY_SHFT    9
#define CONN_WT_SLP_CTL_REG_WB_STA_WB_SLP_TOP_CK_4_BSY_ADDR    CONN_WT_SLP_CTL_REG_WB_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA_WB_SLP_TOP_CK_4_BSY_MASK    0x00000100                // WB_SLP_TOP_CK_4_BSY[8]
#define CONN_WT_SLP_CTL_REG_WB_STA_WB_SLP_TOP_CK_4_BSY_SHFT    8
#define CONN_WT_SLP_CTL_REG_WB_STA_WB_SLP_TOP_CK_5_BSY_ADDR    CONN_WT_SLP_CTL_REG_WB_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA_WB_SLP_TOP_CK_5_BSY_MASK    0x00000080                // WB_SLP_TOP_CK_5_BSY[7]
#define CONN_WT_SLP_CTL_REG_WB_STA_WB_SLP_TOP_CK_5_BSY_SHFT    7
#define CONN_WT_SLP_CTL_REG_WB_STA_GPS_L5_EN_BUSY_ADDR         CONN_WT_SLP_CTL_REG_WB_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA_GPS_L5_EN_BUSY_MASK         0x00000040                // GPS_L5_EN_BUSY[6]
#define CONN_WT_SLP_CTL_REG_WB_STA_GPS_L5_EN_BUSY_SHFT         6
#define CONN_WT_SLP_CTL_REG_WB_STA_GPS_L5_RFBUF_BUSY_ADDR      CONN_WT_SLP_CTL_REG_WB_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA_GPS_L5_RFBUF_BUSY_MASK      0x00000020                // GPS_L5_RFBUF_BUSY[5]
#define CONN_WT_SLP_CTL_REG_WB_STA_GPS_L5_RFBUF_BUSY_SHFT      5
#define CONN_WT_SLP_CTL_REG_WB_STA_GPS_L1_RFBUF_BUSY_ADDR      CONN_WT_SLP_CTL_REG_WB_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA_GPS_L1_RFBUF_BUSY_MASK      0x00000010                // GPS_L1_RFBUF_BUSY[4]
#define CONN_WT_SLP_CTL_REG_WB_STA_GPS_L1_RFBUF_BUSY_SHFT      4
#define CONN_WT_SLP_CTL_REG_WB_STA_B1_WF_SLP_BUSY_ADDR         CONN_WT_SLP_CTL_REG_WB_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA_B1_WF_SLP_BUSY_MASK         0x00000008                // B1_WF_SLP_BUSY[3]
#define CONN_WT_SLP_CTL_REG_WB_STA_B1_WF_SLP_BUSY_SHFT         3
#define CONN_WT_SLP_CTL_REG_WB_STA_B1_WF_CK_BUSY_ADDR          CONN_WT_SLP_CTL_REG_WB_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA_B1_WF_CK_BUSY_MASK          0x00000004                // B1_WF_CK_BUSY[2]
#define CONN_WT_SLP_CTL_REG_WB_STA_B1_WF_CK_BUSY_SHFT          2
#define CONN_WT_SLP_CTL_REG_WB_STA_GPS_L5_CK_BUSY_ADDR         CONN_WT_SLP_CTL_REG_WB_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA_GPS_L5_CK_BUSY_MASK         0x00000002                // GPS_L5_CK_BUSY[1]
#define CONN_WT_SLP_CTL_REG_WB_STA_GPS_L5_CK_BUSY_SHFT         1
#define CONN_WT_SLP_CTL_REG_WB_STA_B1_WF_ZPS_BUSY_ADDR         CONN_WT_SLP_CTL_REG_WB_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA_B1_WF_ZPS_BUSY_MASK         0x00000001                // B1_WF_ZPS_BUSY[0]
#define CONN_WT_SLP_CTL_REG_WB_STA_B1_WF_ZPS_BUSY_SHFT         0

/* =====================================================================================

  ---WB_SLP_TMOUT (0x18003000 + 0x00C)---

    TM_USCNT[7..0]               - (RW) WB sleep control timeout us count.
                                     Programming this field to be (TM_USCNT+1) / spi_uspi_ck = 1us.
                                     (The default value assumes spi_uspi_ck clock frequency is 26MHz)
    TM_CNT[12..8]                - (RW) WB sleep control timeout counter. The delay time before next CLK on command being issued.
                                     Time out time (us) =TM_CNT * (TM_USCNT+1) / spi_uspi_ck
    RESERVED13[15..13]           - (RO) Reserved bits
    OFF_TM_USCNT[20..16]         - (RW) WB sleep control timeout us count for BG OFF
                                     Programming this field to be (TM_USCNT+1) / spi_uspi_ck = 1us.
                                     (The default value assumes spi_uspi_ck clock frequency is 26MHz)
    RESERVED21[31..21]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_SLP_TMOUT_OFF_TM_USCNT_ADDR     CONN_WT_SLP_CTL_REG_WB_SLP_TMOUT_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_TMOUT_OFF_TM_USCNT_MASK     0x001F0000                // OFF_TM_USCNT[20..16]
#define CONN_WT_SLP_CTL_REG_WB_SLP_TMOUT_OFF_TM_USCNT_SHFT     16
#define CONN_WT_SLP_CTL_REG_WB_SLP_TMOUT_TM_CNT_ADDR           CONN_WT_SLP_CTL_REG_WB_SLP_TMOUT_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_TMOUT_TM_CNT_MASK           0x00001F00                // TM_CNT[12..8]
#define CONN_WT_SLP_CTL_REG_WB_SLP_TMOUT_TM_CNT_SHFT           8
#define CONN_WT_SLP_CTL_REG_WB_SLP_TMOUT_TM_USCNT_ADDR         CONN_WT_SLP_CTL_REG_WB_SLP_TMOUT_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_TMOUT_TM_USCNT_MASK         0x000000FF                // TM_USCNT[7..0]
#define CONN_WT_SLP_CTL_REG_WB_SLP_TMOUT_TM_USCNT_SHFT         0

/* =====================================================================================

  ---WB_BG_ADDR1 (0x18003000 + 0x010)---

    WB_BG_ADDR1[15..0]           - (RW) Bandgap SPI Address 1

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR1_WB_BG_ADDR1_ADDR       CONN_WT_SLP_CTL_REG_WB_BG_ADDR1_ADDR
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR1_WB_BG_ADDR1_MASK       0x0000FFFF                // WB_BG_ADDR1[15..0]
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR1_WB_BG_ADDR1_SHFT       0

/* =====================================================================================

  ---WB_BG_ADDR2 (0x18003000 + 0x014)---

    WB_BG_ADDR2[15..0]           - (RW) Bandgap SPI Address 2

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR2_WB_BG_ADDR2_ADDR       CONN_WT_SLP_CTL_REG_WB_BG_ADDR2_ADDR
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR2_WB_BG_ADDR2_MASK       0x0000FFFF                // WB_BG_ADDR2[15..0]
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR2_WB_BG_ADDR2_SHFT       0

/* =====================================================================================

  ---WB_BG_ADDR3 (0x18003000 + 0x018)---

    WB_BG_ADDR3[15..0]           - (RW) Bandgap SPI Address 3

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR3_WB_BG_ADDR3_ADDR       CONN_WT_SLP_CTL_REG_WB_BG_ADDR3_ADDR
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR3_WB_BG_ADDR3_MASK       0x0000FFFF                // WB_BG_ADDR3[15..0]
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR3_WB_BG_ADDR3_SHFT       0

/* =====================================================================================

  ---WB_BG_ADDR4 (0x18003000 + 0x01C)---

    WB_BG_ADDR4[15..0]           - (RW) Bandgap SPI Address 4

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR4_WB_BG_ADDR4_ADDR       CONN_WT_SLP_CTL_REG_WB_BG_ADDR4_ADDR
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR4_WB_BG_ADDR4_MASK       0x0000FFFF                // WB_BG_ADDR4[15..0]
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR4_WB_BG_ADDR4_SHFT       0

/* =====================================================================================

  ---WB_BG_ADDR5 (0x18003000 + 0x020)---

    WB_BG_ADDR5[15..0]           - (RW) Bandgap SPI Address 5

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR5_WB_BG_ADDR5_ADDR       CONN_WT_SLP_CTL_REG_WB_BG_ADDR5_ADDR
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR5_WB_BG_ADDR5_MASK       0x0000FFFF                // WB_BG_ADDR5[15..0]
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR5_WB_BG_ADDR5_SHFT       0

/* =====================================================================================

  ---WB_BG_ADDR6 (0x18003000 + 0x024)---

    WB_BG_ADDR6[15..0]           - (RW) Bandgap SPI Address 6

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR6_WB_BG_ADDR6_ADDR       CONN_WT_SLP_CTL_REG_WB_BG_ADDR6_ADDR
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR6_WB_BG_ADDR6_MASK       0x0000FFFF                // WB_BG_ADDR6[15..0]
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR6_WB_BG_ADDR6_SHFT       0

/* =====================================================================================

  ---WB_BG_ADDR7 (0x18003000 + 0x028)---

    WB_BG_ADDR7[15..0]           - (RW) Bandgap SPI Address 7

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR7_WB_BG_ADDR7_ADDR       CONN_WT_SLP_CTL_REG_WB_BG_ADDR7_ADDR
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR7_WB_BG_ADDR7_MASK       0x0000FFFF                // WB_BG_ADDR7[15..0]
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR7_WB_BG_ADDR7_SHFT       0

/* =====================================================================================

  ---WB_BG_ADDR8 (0x18003000 + 0x02C)---

    WB_BG_ADDR8[15..0]           - (RW) Bandgap SPI Address 8

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR8_WB_BG_ADDR8_ADDR       CONN_WT_SLP_CTL_REG_WB_BG_ADDR8_ADDR
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR8_WB_BG_ADDR8_MASK       0x0000FFFF                // WB_BG_ADDR8[15..0]
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR8_WB_BG_ADDR8_SHFT       0

/* =====================================================================================

  ---WB_BG_ON1 (0x18003000 + 0x030)---

    WB_BG_ON1[31..0]             - (RW) Bandgap ON setting 1.
                                     Aligning with WB_BG_ADDR1

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_BG_ON1_WB_BG_ON1_ADDR           CONN_WT_SLP_CTL_REG_WB_BG_ON1_ADDR
#define CONN_WT_SLP_CTL_REG_WB_BG_ON1_WB_BG_ON1_MASK           0xFFFFFFFF                // WB_BG_ON1[31..0]
#define CONN_WT_SLP_CTL_REG_WB_BG_ON1_WB_BG_ON1_SHFT           0

/* =====================================================================================

  ---WB_BG_ON2 (0x18003000 + 0x034)---

    WB_BG_ON2[31..0]             - (RW) Bandgap ON setting 2.
                                     Aligning with WB_BG_ADDR2

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_BG_ON2_WB_BG_ON2_ADDR           CONN_WT_SLP_CTL_REG_WB_BG_ON2_ADDR
#define CONN_WT_SLP_CTL_REG_WB_BG_ON2_WB_BG_ON2_MASK           0xFFFFFFFF                // WB_BG_ON2[31..0]
#define CONN_WT_SLP_CTL_REG_WB_BG_ON2_WB_BG_ON2_SHFT           0

/* =====================================================================================

  ---WB_BG_ON3 (0x18003000 + 0x038)---

    WB_BG_ON3[31..0]             - (RW) Bandgap ON setting 3.
                                     Aligning with WB_BG_ADDR3

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_BG_ON3_WB_BG_ON3_ADDR           CONN_WT_SLP_CTL_REG_WB_BG_ON3_ADDR
#define CONN_WT_SLP_CTL_REG_WB_BG_ON3_WB_BG_ON3_MASK           0xFFFFFFFF                // WB_BG_ON3[31..0]
#define CONN_WT_SLP_CTL_REG_WB_BG_ON3_WB_BG_ON3_SHFT           0

/* =====================================================================================

  ---WB_BG_ON4 (0x18003000 + 0x03C)---

    WB_BG_ON4[31..0]             - (RW) Bandgap ON setting 4.
                                     Aligning with WB_BG_ADDR4

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_BG_ON4_WB_BG_ON4_ADDR           CONN_WT_SLP_CTL_REG_WB_BG_ON4_ADDR
#define CONN_WT_SLP_CTL_REG_WB_BG_ON4_WB_BG_ON4_MASK           0xFFFFFFFF                // WB_BG_ON4[31..0]
#define CONN_WT_SLP_CTL_REG_WB_BG_ON4_WB_BG_ON4_SHFT           0

/* =====================================================================================

  ---WB_BG_ON5 (0x18003000 + 0x040)---

    WB_BG_ON5[31..0]             - (RW) Bandgap ON setting 5.
                                     Aligning with WB_BG_ADDR5

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_BG_ON5_WB_BG_ON5_ADDR           CONN_WT_SLP_CTL_REG_WB_BG_ON5_ADDR
#define CONN_WT_SLP_CTL_REG_WB_BG_ON5_WB_BG_ON5_MASK           0xFFFFFFFF                // WB_BG_ON5[31..0]
#define CONN_WT_SLP_CTL_REG_WB_BG_ON5_WB_BG_ON5_SHFT           0

/* =====================================================================================

  ---WB_BG_ON6 (0x18003000 + 0x044)---

    WB_BG_ON6[31..0]             - (RW) Bandgap ON setting 6.
                                     Aligning with WB_BG_ADDR6

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_BG_ON6_WB_BG_ON6_ADDR           CONN_WT_SLP_CTL_REG_WB_BG_ON6_ADDR
#define CONN_WT_SLP_CTL_REG_WB_BG_ON6_WB_BG_ON6_MASK           0xFFFFFFFF                // WB_BG_ON6[31..0]
#define CONN_WT_SLP_CTL_REG_WB_BG_ON6_WB_BG_ON6_SHFT           0

/* =====================================================================================

  ---WB_BG_ON7 (0x18003000 + 0x048)---

    WB_BG_ON7[31..0]             - (RW) Bandgap ON setting 7.
                                     Aligning with WB_BG_ADDR7

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_BG_ON7_WB_BG_ON7_ADDR           CONN_WT_SLP_CTL_REG_WB_BG_ON7_ADDR
#define CONN_WT_SLP_CTL_REG_WB_BG_ON7_WB_BG_ON7_MASK           0xFFFFFFFF                // WB_BG_ON7[31..0]
#define CONN_WT_SLP_CTL_REG_WB_BG_ON7_WB_BG_ON7_SHFT           0

/* =====================================================================================

  ---WB_BG_ON8 (0x18003000 + 0x04C)---

    WB_BG_ON8[31..0]             - (RW) Bandgap ON setting 8.
                                     Aligning with WB_BG_ADDR8

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_BG_ON8_WB_BG_ON8_ADDR           CONN_WT_SLP_CTL_REG_WB_BG_ON8_ADDR
#define CONN_WT_SLP_CTL_REG_WB_BG_ON8_WB_BG_ON8_MASK           0xFFFFFFFF                // WB_BG_ON8[31..0]
#define CONN_WT_SLP_CTL_REG_WB_BG_ON8_WB_BG_ON8_SHFT           0

/* =====================================================================================

  ---WB_BG_OFF1 (0x18003000 + 0x050)---

    WB_BG_OFF1[31..0]            - (RW) Bandgap OFF setting 1.
                                     Aligning with WB_BG_ADDR1

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF1_WB_BG_OFF1_ADDR         CONN_WT_SLP_CTL_REG_WB_BG_OFF1_ADDR
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF1_WB_BG_OFF1_MASK         0xFFFFFFFF                // WB_BG_OFF1[31..0]
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF1_WB_BG_OFF1_SHFT         0

/* =====================================================================================

  ---WB_BG_OFF2 (0x18003000 + 0x054)---

    WB_BG_OFF2[31..0]            - (RW) Bandgap OFF setting 2.
                                     Aligning with WB_BG_ADDR2

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF2_WB_BG_OFF2_ADDR         CONN_WT_SLP_CTL_REG_WB_BG_OFF2_ADDR
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF2_WB_BG_OFF2_MASK         0xFFFFFFFF                // WB_BG_OFF2[31..0]
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF2_WB_BG_OFF2_SHFT         0

/* =====================================================================================

  ---WB_BG_OFF3 (0x18003000 + 0x058)---

    WB_BG_OFF3[31..0]            - (RW) Bandgap OFF setting 3.
                                     Aligning with WB_BG_ADDR3

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF3_WB_BG_OFF3_ADDR         CONN_WT_SLP_CTL_REG_WB_BG_OFF3_ADDR
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF3_WB_BG_OFF3_MASK         0xFFFFFFFF                // WB_BG_OFF3[31..0]
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF3_WB_BG_OFF3_SHFT         0

/* =====================================================================================

  ---WB_BG_OFF4 (0x18003000 + 0x05C)---

    WB_BG_OFF4[31..0]            - (RW) Bandgap OFF setting 4.
                                     Aligning with WB_BG_ADDR4

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF4_WB_BG_OFF4_ADDR         CONN_WT_SLP_CTL_REG_WB_BG_OFF4_ADDR
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF4_WB_BG_OFF4_MASK         0xFFFFFFFF                // WB_BG_OFF4[31..0]
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF4_WB_BG_OFF4_SHFT         0

/* =====================================================================================

  ---WB_BG_OFF5 (0x18003000 + 0x060)---

    WB_BG_OFF5[31..0]            - (RW) Bandgap OFF setting 5.
                                     Aligning with WB_BG_ADDR5

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF5_WB_BG_OFF5_ADDR         CONN_WT_SLP_CTL_REG_WB_BG_OFF5_ADDR
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF5_WB_BG_OFF5_MASK         0xFFFFFFFF                // WB_BG_OFF5[31..0]
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF5_WB_BG_OFF5_SHFT         0

/* =====================================================================================

  ---WB_BG_OFF6 (0x18003000 + 0x064)---

    WB_BG_OFF6[31..0]            - (RW) Bandgap OFF setting 6.
                                     Aligning with WB_BG_ADDR6

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF6_WB_BG_OFF6_ADDR         CONN_WT_SLP_CTL_REG_WB_BG_OFF6_ADDR
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF6_WB_BG_OFF6_MASK         0xFFFFFFFF                // WB_BG_OFF6[31..0]
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF6_WB_BG_OFF6_SHFT         0

/* =====================================================================================

  ---WB_BG_OFF7 (0x18003000 + 0x068)---

    WB_BG_OFF7[31..0]            - (RW) Bandgap OFF setting 7.
                                     Aligning with WB_BG_ADDR7

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF7_WB_BG_OFF7_ADDR         CONN_WT_SLP_CTL_REG_WB_BG_OFF7_ADDR
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF7_WB_BG_OFF7_MASK         0xFFFFFFFF                // WB_BG_OFF7[31..0]
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF7_WB_BG_OFF7_SHFT         0

/* =====================================================================================

  ---WB_BG_OFF8 (0x18003000 + 0x06C)---

    WB_BG_OFF8[31..0]            - (RW) Bandgap OFF setting 8.
                                     Aligning with WB_BG_ADDR8

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF8_WB_BG_OFF8_ADDR         CONN_WT_SLP_CTL_REG_WB_BG_OFF8_ADDR
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF8_WB_BG_OFF8_MASK         0xFFFFFFFF                // WB_BG_OFF8[31..0]
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF8_WB_BG_OFF8_SHFT         0

/* =====================================================================================

  ---WB_WF_CK_ADDR (0x18003000 + 0x070)---

    WB_WF_CK_ADDR[11..0]         - (RW) WB_WF_CK_ADDR
    RESERVED12[15..12]           - (RO) Reserved bits
    WB_WF_B1_CK_ADDR[27..16]     - (RW) WB_WF_B1_CK_ADDR
    RESERVED28[31..28]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_WF_CK_ADDR_WB_WF_B1_CK_ADDR_ADDR CONN_WT_SLP_CTL_REG_WB_WF_CK_ADDR_ADDR
#define CONN_WT_SLP_CTL_REG_WB_WF_CK_ADDR_WB_WF_B1_CK_ADDR_MASK 0x0FFF0000                // WB_WF_B1_CK_ADDR[27..16]
#define CONN_WT_SLP_CTL_REG_WB_WF_CK_ADDR_WB_WF_B1_CK_ADDR_SHFT 16
#define CONN_WT_SLP_CTL_REG_WB_WF_CK_ADDR_WB_WF_CK_ADDR_ADDR   CONN_WT_SLP_CTL_REG_WB_WF_CK_ADDR_ADDR
#define CONN_WT_SLP_CTL_REG_WB_WF_CK_ADDR_WB_WF_CK_ADDR_MASK   0x00000FFF                // WB_WF_CK_ADDR[11..0]
#define CONN_WT_SLP_CTL_REG_WB_WF_CK_ADDR_WB_WF_CK_ADDR_SHFT   0

/* =====================================================================================

  ---WB_WF_WAKE_ADDR (0x18003000 + 0x074)---

    WB_WF_WAKE_ADDR[11..0]       - (RW) WB_WF_WAKE_ADDR
    RESERVED12[15..12]           - (RO) Reserved bits
    WB_WF_B1_WAKE_ADDR[27..16]   - (RW) WB_WF_B1_WAKE_ADDR
    RESERVED28[31..28]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_WF_WAKE_ADDR_WB_WF_B1_WAKE_ADDR_ADDR CONN_WT_SLP_CTL_REG_WB_WF_WAKE_ADDR_ADDR
#define CONN_WT_SLP_CTL_REG_WB_WF_WAKE_ADDR_WB_WF_B1_WAKE_ADDR_MASK 0x0FFF0000                // WB_WF_B1_WAKE_ADDR[27..16]
#define CONN_WT_SLP_CTL_REG_WB_WF_WAKE_ADDR_WB_WF_B1_WAKE_ADDR_SHFT 16
#define CONN_WT_SLP_CTL_REG_WB_WF_WAKE_ADDR_WB_WF_WAKE_ADDR_ADDR CONN_WT_SLP_CTL_REG_WB_WF_WAKE_ADDR_ADDR
#define CONN_WT_SLP_CTL_REG_WB_WF_WAKE_ADDR_WB_WF_WAKE_ADDR_MASK 0x00000FFF                // WB_WF_WAKE_ADDR[11..0]
#define CONN_WT_SLP_CTL_REG_WB_WF_WAKE_ADDR_WB_WF_WAKE_ADDR_SHFT 0

/* =====================================================================================

  ---WB_WF_ZPS_ADDR (0x18003000 + 0x078)---

    WB_WF_ZPS_ADDR[11..0]        - (RW) WB_WF_ZPS_ADDR
    RESERVED12[15..12]           - (RO) Reserved bits
    WB_WF_B1_ZPS_ADDR[27..16]    - (RW) WB_WF_B1_ZPS_ADDR
    RESERVED28[31..28]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_WF_ZPS_ADDR_WB_WF_B1_ZPS_ADDR_ADDR CONN_WT_SLP_CTL_REG_WB_WF_ZPS_ADDR_ADDR
#define CONN_WT_SLP_CTL_REG_WB_WF_ZPS_ADDR_WB_WF_B1_ZPS_ADDR_MASK 0x0FFF0000                // WB_WF_B1_ZPS_ADDR[27..16]
#define CONN_WT_SLP_CTL_REG_WB_WF_ZPS_ADDR_WB_WF_B1_ZPS_ADDR_SHFT 16
#define CONN_WT_SLP_CTL_REG_WB_WF_ZPS_ADDR_WB_WF_ZPS_ADDR_ADDR CONN_WT_SLP_CTL_REG_WB_WF_ZPS_ADDR_ADDR
#define CONN_WT_SLP_CTL_REG_WB_WF_ZPS_ADDR_WB_WF_ZPS_ADDR_MASK 0x00000FFF                // WB_WF_ZPS_ADDR[11..0]
#define CONN_WT_SLP_CTL_REG_WB_WF_ZPS_ADDR_WB_WF_ZPS_ADDR_SHFT 0

/* =====================================================================================

  ---WB_BT_CK_ADDR (0x18003000 + 0x07C)---

    WB_BT_CK_ADDR[11..0]         - (RW) A-DIE BT_CK_EN address
    RESERVED12[31..12]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_BT_CK_ADDR_WB_BT_CK_ADDR_ADDR   CONN_WT_SLP_CTL_REG_WB_BT_CK_ADDR_ADDR
#define CONN_WT_SLP_CTL_REG_WB_BT_CK_ADDR_WB_BT_CK_ADDR_MASK   0x00000FFF                // WB_BT_CK_ADDR[11..0]
#define CONN_WT_SLP_CTL_REG_WB_BT_CK_ADDR_WB_BT_CK_ADDR_SHFT   0

/* =====================================================================================

  ---WB_BT_WAKE_ADDR (0x18003000 + 0x080)---

    WB_BT_WAKE_ADDR[11..0]       - (RW) A-DIE BT_WAKE_EN address
    RESERVED12[31..12]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_BT_WAKE_ADDR_WB_BT_WAKE_ADDR_ADDR CONN_WT_SLP_CTL_REG_WB_BT_WAKE_ADDR_ADDR
#define CONN_WT_SLP_CTL_REG_WB_BT_WAKE_ADDR_WB_BT_WAKE_ADDR_MASK 0x00000FFF                // WB_BT_WAKE_ADDR[11..0]
#define CONN_WT_SLP_CTL_REG_WB_BT_WAKE_ADDR_WB_BT_WAKE_ADDR_SHFT 0

/* =====================================================================================

  ---WB_TOP_CK_ADDR (0x18003000 + 0x084)---

    WB_TOP_CK_ADDR[11..0]        - (RW) A-DIE TOP_CK_EN address
    RESERVED12[31..12]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_TOP_CK_ADDR_WB_TOP_CK_ADDR_ADDR CONN_WT_SLP_CTL_REG_WB_TOP_CK_ADDR_ADDR
#define CONN_WT_SLP_CTL_REG_WB_TOP_CK_ADDR_WB_TOP_CK_ADDR_MASK 0x00000FFF                // WB_TOP_CK_ADDR[11..0]
#define CONN_WT_SLP_CTL_REG_WB_TOP_CK_ADDR_WB_TOP_CK_ADDR_SHFT 0

/* =====================================================================================

  ---WB_GPS_CK_ADDR (0x18003000 + 0x088)---

    WB_GPS_CK_ADDR[11..0]        - (RW) A-DIE GPS_CK_EN address
    RESERVED12[15..12]           - (RO) Reserved bits
    WB_GPS_L5_CK_ADDR[27..16]    - (RW) A-DIE GPS_L5_CK_EN address
    RESERVED28[31..28]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_GPS_CK_ADDR_WB_GPS_L5_CK_ADDR_ADDR CONN_WT_SLP_CTL_REG_WB_GPS_CK_ADDR_ADDR
#define CONN_WT_SLP_CTL_REG_WB_GPS_CK_ADDR_WB_GPS_L5_CK_ADDR_MASK 0x0FFF0000                // WB_GPS_L5_CK_ADDR[27..16]
#define CONN_WT_SLP_CTL_REG_WB_GPS_CK_ADDR_WB_GPS_L5_CK_ADDR_SHFT 16
#define CONN_WT_SLP_CTL_REG_WB_GPS_CK_ADDR_WB_GPS_CK_ADDR_ADDR CONN_WT_SLP_CTL_REG_WB_GPS_CK_ADDR_ADDR
#define CONN_WT_SLP_CTL_REG_WB_GPS_CK_ADDR_WB_GPS_CK_ADDR_MASK 0x00000FFF                // WB_GPS_CK_ADDR[11..0]
#define CONN_WT_SLP_CTL_REG_WB_GPS_CK_ADDR_WB_GPS_CK_ADDR_SHFT 0

/* =====================================================================================

  ---WB_WF_B0_CMD_ADDR (0x18003000 + 0x08c)---

    WB_WF_B0_CMD_ADDR[11..0]     - (RW) A-DIE WF_B0_CMD address
    RESERVED12[31..12]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_WF_B0_CMD_ADDR_WB_WF_B0_CMD_ADDR_ADDR CONN_WT_SLP_CTL_REG_WB_WF_B0_CMD_ADDR_ADDR
#define CONN_WT_SLP_CTL_REG_WB_WF_B0_CMD_ADDR_WB_WF_B0_CMD_ADDR_MASK 0x00000FFF                // WB_WF_B0_CMD_ADDR[11..0]
#define CONN_WT_SLP_CTL_REG_WB_WF_B0_CMD_ADDR_WB_WF_B0_CMD_ADDR_SHFT 0

/* =====================================================================================

  ---WB_WF_B1_CMD_ADDR (0x18003000 + 0x090)---

    WB_WF_B1_CMD_ADDR[11..0]     - (RW) A-DIE WF_B1_CMD address
    RESERVED12[31..12]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_WF_B1_CMD_ADDR_WB_WF_B1_CMD_ADDR_ADDR CONN_WT_SLP_CTL_REG_WB_WF_B1_CMD_ADDR_ADDR
#define CONN_WT_SLP_CTL_REG_WB_WF_B1_CMD_ADDR_WB_WF_B1_CMD_ADDR_MASK 0x00000FFF                // WB_WF_B1_CMD_ADDR[11..0]
#define CONN_WT_SLP_CTL_REG_WB_WF_B1_CMD_ADDR_WB_WF_B1_CMD_ADDR_SHFT 0

/* =====================================================================================

  ---WB_GPS_RFBUF_ADR (0x18003000 + 0x094)---

    WB_GPS_RFBUF_ADR[11..0]      - (RW) A-DIE GPS RFBUF EN address
    RESERVED12[31..12]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_GPS_RFBUF_ADR_WB_GPS_RFBUF_ADR_ADDR CONN_WT_SLP_CTL_REG_WB_GPS_RFBUF_ADR_ADDR
#define CONN_WT_SLP_CTL_REG_WB_GPS_RFBUF_ADR_WB_GPS_RFBUF_ADR_MASK 0x00000FFF                // WB_GPS_RFBUF_ADR[11..0]
#define CONN_WT_SLP_CTL_REG_WB_GPS_RFBUF_ADR_WB_GPS_RFBUF_ADR_SHFT 0

/* =====================================================================================

  ---WB_GPS_L5_EN_ADDR (0x18003000 + 0x098)---

    WB_GPS_L5_EN_ADDR[11..0]     - (RW) A-DIE GPS L5 EN address
    RESERVED12[31..12]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_GPS_L5_EN_ADDR_WB_GPS_L5_EN_ADDR_ADDR CONN_WT_SLP_CTL_REG_WB_GPS_L5_EN_ADDR_ADDR
#define CONN_WT_SLP_CTL_REG_WB_GPS_L5_EN_ADDR_WB_GPS_L5_EN_ADDR_MASK 0x00000FFF                // WB_GPS_L5_EN_ADDR[11..0]
#define CONN_WT_SLP_CTL_REG_WB_GPS_L5_EN_ADDR_WB_GPS_L5_EN_ADDR_SHFT 0

/* =====================================================================================

  ---WB_STA1 (0x18003000 + 0x0A0)---

    WB_SLP_STATE[31..0]          - (RO) wt_slp_ctrl [31:0] FSM state

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_STA1_WB_SLP_STATE_ADDR          CONN_WT_SLP_CTL_REG_WB_STA1_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA1_WB_SLP_STATE_MASK          0xFFFFFFFF                // WB_SLP_STATE[31..0]
#define CONN_WT_SLP_CTL_REG_WB_STA1_WB_SLP_STATE_SHFT          0

/* =====================================================================================

  ---WB_RSV (0x18003000 + 0x0A4)---

    WB_RSV[15..0]                - (RW) Reserved CR
    RESERVED16[31..16]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_RSV_WB_RSV_ADDR                 CONN_WT_SLP_CTL_REG_WB_RSV_ADDR
#define CONN_WT_SLP_CTL_REG_WB_RSV_WB_RSV_MASK                 0x0000FFFF                // WB_RSV[15..0]
#define CONN_WT_SLP_CTL_REG_WB_RSV_WB_RSV_SHFT                 0

/* =====================================================================================

  ---WB_CK_STA (0x18003000 + 0x0A8)---

    TOP_CK_EN[0]                 - (RO) adie top ck enable
    B0_WF_CK_EN[1]               - (RO) band0 wf ck enable
    B1_WF_CK_EN[2]               - (RO) band1 wf ck enable
    BT_CK_EN[3]                  - (RO) bt ck enalbe
    GPS_L1_CK_EN[4]              - (RO) gps l1 ck enable
    GPS_L5_CK_EN[5]              - (RO) gps l5 ck enable
    GPS_L1_RFBUF_EN[6]           - (RO) gps l1 rfbuf enable
    GPS_L5_RFBUF_EN[7]           - (RO) gps l5 rfbuf enable
    B0_WF_WAKE_EN[8]             - (RO) band0 wf wake up enable
    B1_WF_WAKE_EN[9]             - (RO) band1 wf wake up enable
    B0_WF_ZPS_EN[10]             - (RO) band0 wf zps enable
    B1_WF_ZPS_EN[11]             - (RO) band1 wf zps enable
    BT_WAKE_EN[12]               - (RO) bt wake up enable
    GPS_L5_EN[13]                - (RO) GPS L5 slelection
    RESERVED14[15..14]           - (RO) Reserved bits
    B0_WF_LP_CMD[19..16]         - (RO) band0 wf lowpower command
    B1_WF_LP_CMD[23..20]         - (RO) band1 wf low power command
    RESERVED24[31..24]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_B1_WF_LP_CMD_ADDR        CONN_WT_SLP_CTL_REG_WB_CK_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_B1_WF_LP_CMD_MASK        0x00F00000                // B1_WF_LP_CMD[23..20]
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_B1_WF_LP_CMD_SHFT        20
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_B0_WF_LP_CMD_ADDR        CONN_WT_SLP_CTL_REG_WB_CK_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_B0_WF_LP_CMD_MASK        0x000F0000                // B0_WF_LP_CMD[19..16]
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_B0_WF_LP_CMD_SHFT        16
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_GPS_L5_EN_ADDR           CONN_WT_SLP_CTL_REG_WB_CK_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_GPS_L5_EN_MASK           0x00002000                // GPS_L5_EN[13]
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_GPS_L5_EN_SHFT           13
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_BT_WAKE_EN_ADDR          CONN_WT_SLP_CTL_REG_WB_CK_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_BT_WAKE_EN_MASK          0x00001000                // BT_WAKE_EN[12]
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_BT_WAKE_EN_SHFT          12
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_B1_WF_ZPS_EN_ADDR        CONN_WT_SLP_CTL_REG_WB_CK_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_B1_WF_ZPS_EN_MASK        0x00000800                // B1_WF_ZPS_EN[11]
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_B1_WF_ZPS_EN_SHFT        11
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_B0_WF_ZPS_EN_ADDR        CONN_WT_SLP_CTL_REG_WB_CK_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_B0_WF_ZPS_EN_MASK        0x00000400                // B0_WF_ZPS_EN[10]
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_B0_WF_ZPS_EN_SHFT        10
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_B1_WF_WAKE_EN_ADDR       CONN_WT_SLP_CTL_REG_WB_CK_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_B1_WF_WAKE_EN_MASK       0x00000200                // B1_WF_WAKE_EN[9]
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_B1_WF_WAKE_EN_SHFT       9
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_B0_WF_WAKE_EN_ADDR       CONN_WT_SLP_CTL_REG_WB_CK_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_B0_WF_WAKE_EN_MASK       0x00000100                // B0_WF_WAKE_EN[8]
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_B0_WF_WAKE_EN_SHFT       8
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_GPS_L5_RFBUF_EN_ADDR     CONN_WT_SLP_CTL_REG_WB_CK_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_GPS_L5_RFBUF_EN_MASK     0x00000080                // GPS_L5_RFBUF_EN[7]
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_GPS_L5_RFBUF_EN_SHFT     7
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_GPS_L1_RFBUF_EN_ADDR     CONN_WT_SLP_CTL_REG_WB_CK_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_GPS_L1_RFBUF_EN_MASK     0x00000040                // GPS_L1_RFBUF_EN[6]
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_GPS_L1_RFBUF_EN_SHFT     6
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_GPS_L5_CK_EN_ADDR        CONN_WT_SLP_CTL_REG_WB_CK_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_GPS_L5_CK_EN_MASK        0x00000020                // GPS_L5_CK_EN[5]
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_GPS_L5_CK_EN_SHFT        5
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_GPS_L1_CK_EN_ADDR        CONN_WT_SLP_CTL_REG_WB_CK_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_GPS_L1_CK_EN_MASK        0x00000010                // GPS_L1_CK_EN[4]
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_GPS_L1_CK_EN_SHFT        4
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_BT_CK_EN_ADDR            CONN_WT_SLP_CTL_REG_WB_CK_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_BT_CK_EN_MASK            0x00000008                // BT_CK_EN[3]
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_BT_CK_EN_SHFT            3
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_B1_WF_CK_EN_ADDR         CONN_WT_SLP_CTL_REG_WB_CK_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_B1_WF_CK_EN_MASK         0x00000004                // B1_WF_CK_EN[2]
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_B1_WF_CK_EN_SHFT         2
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_B0_WF_CK_EN_ADDR         CONN_WT_SLP_CTL_REG_WB_CK_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_B0_WF_CK_EN_MASK         0x00000002                // B0_WF_CK_EN[1]
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_B0_WF_CK_EN_SHFT         1
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_TOP_CK_EN_ADDR           CONN_WT_SLP_CTL_REG_WB_CK_STA_ADDR
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_TOP_CK_EN_MASK           0x00000001                // TOP_CK_EN[0]
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_TOP_CK_EN_SHFT           0

/* =====================================================================================

  ---WB_FAKE_CK_EN_TOP (0x18003000 + 0x100)---

    TOP_FAKE_CK_EN[0]            - (RW) TOP fake ck_en -> don't trun off BG
    WB_RSV[15..1]                - (RW) Reserved CR
    RESERVED16[31..16]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_FAKE_CK_EN_TOP_WB_RSV_ADDR      CONN_WT_SLP_CTL_REG_WB_FAKE_CK_EN_TOP_ADDR
#define CONN_WT_SLP_CTL_REG_WB_FAKE_CK_EN_TOP_WB_RSV_MASK      0x0000FFFE                // WB_RSV[15..1]
#define CONN_WT_SLP_CTL_REG_WB_FAKE_CK_EN_TOP_WB_RSV_SHFT      1
#define CONN_WT_SLP_CTL_REG_WB_FAKE_CK_EN_TOP_TOP_FAKE_CK_EN_ADDR CONN_WT_SLP_CTL_REG_WB_FAKE_CK_EN_TOP_ADDR
#define CONN_WT_SLP_CTL_REG_WB_FAKE_CK_EN_TOP_TOP_FAKE_CK_EN_MASK 0x00000001                // TOP_FAKE_CK_EN[0]
#define CONN_WT_SLP_CTL_REG_WB_FAKE_CK_EN_TOP_TOP_FAKE_CK_EN_SHFT 0

/* =====================================================================================

  ---WB_FAKE_CK_EN_WF (0x18003000 + 0x104)---

    WF_FAKE_CK_EN[0]             - (RW) WF fake ck_en -> don't trun off BG
    WB_RSV[15..1]                - (RW) Reserved CR
    RESERVED16[31..16]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_FAKE_CK_EN_WF_WB_RSV_ADDR       CONN_WT_SLP_CTL_REG_WB_FAKE_CK_EN_WF_ADDR
#define CONN_WT_SLP_CTL_REG_WB_FAKE_CK_EN_WF_WB_RSV_MASK       0x0000FFFE                // WB_RSV[15..1]
#define CONN_WT_SLP_CTL_REG_WB_FAKE_CK_EN_WF_WB_RSV_SHFT       1
#define CONN_WT_SLP_CTL_REG_WB_FAKE_CK_EN_WF_WF_FAKE_CK_EN_ADDR CONN_WT_SLP_CTL_REG_WB_FAKE_CK_EN_WF_ADDR
#define CONN_WT_SLP_CTL_REG_WB_FAKE_CK_EN_WF_WF_FAKE_CK_EN_MASK 0x00000001                // WF_FAKE_CK_EN[0]
#define CONN_WT_SLP_CTL_REG_WB_FAKE_CK_EN_WF_WF_FAKE_CK_EN_SHFT 0

/* =====================================================================================

  ---WB_FAKE_CK_EN_BT (0x18003000 + 0x108)---

    BT_FAKE_CK_EN[0]             - (RW) BT fake ck_en -> don't trun off BG
    WB_RSV[15..1]                - (RW) Reserved CR
    RESERVED16[31..16]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_FAKE_CK_EN_BT_WB_RSV_ADDR       CONN_WT_SLP_CTL_REG_WB_FAKE_CK_EN_BT_ADDR
#define CONN_WT_SLP_CTL_REG_WB_FAKE_CK_EN_BT_WB_RSV_MASK       0x0000FFFE                // WB_RSV[15..1]
#define CONN_WT_SLP_CTL_REG_WB_FAKE_CK_EN_BT_WB_RSV_SHFT       1
#define CONN_WT_SLP_CTL_REG_WB_FAKE_CK_EN_BT_BT_FAKE_CK_EN_ADDR CONN_WT_SLP_CTL_REG_WB_FAKE_CK_EN_BT_ADDR
#define CONN_WT_SLP_CTL_REG_WB_FAKE_CK_EN_BT_BT_FAKE_CK_EN_MASK 0x00000001                // BT_FAKE_CK_EN[0]
#define CONN_WT_SLP_CTL_REG_WB_FAKE_CK_EN_BT_BT_FAKE_CK_EN_SHFT 0

/* =====================================================================================

  ---WB_BT1_CK_ADDR (0x18003000 + 0x10C)---

    WB_BT1_CK_ADDR[11..0]        - (RO) A-DIE BT1_CK_EN address
    RESERVED12[31..12]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_BT1_CK_ADDR_WB_BT1_CK_ADDR_ADDR CONN_WT_SLP_CTL_REG_WB_BT1_CK_ADDR_ADDR
#define CONN_WT_SLP_CTL_REG_WB_BT1_CK_ADDR_WB_BT1_CK_ADDR_MASK 0x00000FFF                // WB_BT1_CK_ADDR[11..0]
#define CONN_WT_SLP_CTL_REG_WB_BT1_CK_ADDR_WB_BT1_CK_ADDR_SHFT 0

/* =====================================================================================

  ---WB_BT1_WAKE_ADDR (0x18003000 + 0x110)---

    WB_BT1_WAKE_ADDR[11..0]      - (RO) A-DIE BT1_WAKE_EN address
    RESERVED12[31..12]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_BT1_WAKE_ADDR_WB_BT1_WAKE_ADDR_ADDR CONN_WT_SLP_CTL_REG_WB_BT1_WAKE_ADDR_ADDR
#define CONN_WT_SLP_CTL_REG_WB_BT1_WAKE_ADDR_WB_BT1_WAKE_ADDR_MASK 0x00000FFF                // WB_BT1_WAKE_ADDR[11..0]
#define CONN_WT_SLP_CTL_REG_WB_BT1_WAKE_ADDR_WB_BT1_WAKE_ADDR_SHFT 0

/* =====================================================================================

  ---WB_FAKE_CK_EN_BT1 (0x18003000 + 0x114)---

    BT1_FAKE_CK_EN[0]            - (RO) BT1 fake ck_en -> don't trun off BG
    WB_RSV[15..1]                - (RO) Reserved CR
    RESERVED16[31..16]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_FAKE_CK_EN_BT1_WB_RSV_ADDR      CONN_WT_SLP_CTL_REG_WB_FAKE_CK_EN_BT1_ADDR
#define CONN_WT_SLP_CTL_REG_WB_FAKE_CK_EN_BT1_WB_RSV_MASK      0x0000FFFE                // WB_RSV[15..1]
#define CONN_WT_SLP_CTL_REG_WB_FAKE_CK_EN_BT1_WB_RSV_SHFT      1
#define CONN_WT_SLP_CTL_REG_WB_FAKE_CK_EN_BT1_BT1_FAKE_CK_EN_ADDR CONN_WT_SLP_CTL_REG_WB_FAKE_CK_EN_BT1_ADDR
#define CONN_WT_SLP_CTL_REG_WB_FAKE_CK_EN_BT1_BT1_FAKE_CK_EN_MASK 0x00000001                // BT1_FAKE_CK_EN[0]
#define CONN_WT_SLP_CTL_REG_WB_FAKE_CK_EN_BT1_BT1_FAKE_CK_EN_SHFT 0

/* =====================================================================================

  ---WB_STA2 (0x18003000 + 0x118)---

    WB_SLP_STATE_1[11..0]        - (RO) wt_slp_ctrl FSM state for bit [43:32]
    BG_STA[18..12]               - (RO) WB control bangdgap FSM
                                     "00_0000" : BG_BUF_OFF_IDLE
                                     "00_0001" : BG_BUF_OFF_TRG
                                     "00_0010" : BG_BUF_OFF_BSY
                                     "00_0100" : BG_BUF_ON_TRG
                                     "00_1000" : BG_BUF_ON_BSY
                                     "01_0000" : BG_BUF_ON_WAIT
                                     "10_0000" : BG_BUF_ON_IDLE
    WB_SLP_STATE_2[30..19]       - (RO) wt_slp_ctrl FSM state for bit [55:44]
    RESERVED31[31]               - (RO) Reserved bits

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_STA2_WB_SLP_STATE_2_ADDR        CONN_WT_SLP_CTL_REG_WB_STA2_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA2_WB_SLP_STATE_2_MASK        0x7FF80000                // WB_SLP_STATE_2[30..19]
#define CONN_WT_SLP_CTL_REG_WB_STA2_WB_SLP_STATE_2_SHFT        19
#define CONN_WT_SLP_CTL_REG_WB_STA2_BG_STA_ADDR                CONN_WT_SLP_CTL_REG_WB_STA2_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA2_BG_STA_MASK                0x0007F000                // BG_STA[18..12]
#define CONN_WT_SLP_CTL_REG_WB_STA2_BG_STA_SHFT                12
#define CONN_WT_SLP_CTL_REG_WB_STA2_WB_SLP_STATE_1_ADDR        CONN_WT_SLP_CTL_REG_WB_STA2_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA2_WB_SLP_STATE_1_MASK        0x00000FFF                // WB_SLP_STATE_1[11..0]
#define CONN_WT_SLP_CTL_REG_WB_STA2_WB_SLP_STATE_1_SHFT        0

/* =====================================================================================

  ---WB_SLP_DEBUG (0x18003000 + 0X11c)---

    WB_SLP_DEBUG_ADDR[11..0]     - (RW) Debug select: 0 -> wt_slp_dbg0 ; 1: wt_slp_dbg1(with busy state)
    RESERVED12[31..12]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_SLP_DEBUG_WB_SLP_DEBUG_ADDR_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_DEBUG_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_DEBUG_WB_SLP_DEBUG_ADDR_MASK 0x00000FFF                // WB_SLP_DEBUG_ADDR[11..0]
#define CONN_WT_SLP_CTL_REG_WB_SLP_DEBUG_WB_SLP_DEBUG_ADDR_SHFT 0

/* =====================================================================================

  ---WB_SLP_TOP_CK_0 (0x18003000 + 0x120)---

    WB_SLP_TOP_CK_0[0]           - (RW) adie single top ck en  enable
    WB_SLP_TOP_CK_0_BSY[1]       - (RO) WB SG TOP EN  clock control busy bit
                                     "1" mean busy.
    WB_SLP_TOP_CK_0_RSV[3..2]    - (RW) Reserved CR
    WB_SLP_MULT_TOP_CK_0[4]      - (RW) adie mult top ck en  enable
    WB_SLP_MULT_TOP_CK_0_BSY[5]  - (RO) WB MULT TOP EN  clock control busy bit
                                     "1" mean busy.
    WB_SLP_MULT_TOP_CK_0_RSV[7..6] - (RW) Reserved CR
    RESERVED8[31..8]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_0_WB_SLP_MULT_TOP_CK_0_RSV_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_0_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_0_WB_SLP_MULT_TOP_CK_0_RSV_MASK 0x000000C0                // WB_SLP_MULT_TOP_CK_0_RSV[7..6]
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_0_WB_SLP_MULT_TOP_CK_0_RSV_SHFT 6
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_0_WB_SLP_MULT_TOP_CK_0_BSY_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_0_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_0_WB_SLP_MULT_TOP_CK_0_BSY_MASK 0x00000020                // WB_SLP_MULT_TOP_CK_0_BSY[5]
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_0_WB_SLP_MULT_TOP_CK_0_BSY_SHFT 5
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_0_WB_SLP_MULT_TOP_CK_0_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_0_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_0_WB_SLP_MULT_TOP_CK_0_MASK 0x00000010                // WB_SLP_MULT_TOP_CK_0[4]
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_0_WB_SLP_MULT_TOP_CK_0_SHFT 4
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_0_WB_SLP_TOP_CK_0_RSV_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_0_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_0_WB_SLP_TOP_CK_0_RSV_MASK 0x0000000C                // WB_SLP_TOP_CK_0_RSV[3..2]
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_0_WB_SLP_TOP_CK_0_RSV_SHFT 2
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_0_WB_SLP_TOP_CK_0_BSY_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_0_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_0_WB_SLP_TOP_CK_0_BSY_MASK 0x00000002                // WB_SLP_TOP_CK_0_BSY[1]
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_0_WB_SLP_TOP_CK_0_BSY_SHFT 1
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_0_WB_SLP_TOP_CK_0_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_0_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_0_WB_SLP_TOP_CK_0_MASK 0x00000001                // WB_SLP_TOP_CK_0[0]
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_0_WB_SLP_TOP_CK_0_SHFT 0

/* =====================================================================================

  ---WB_SLP_TOP_CK_1 (0x18003000 + 0x124)---

    WB_SLP_TOP_CK_1[0]           - (RW) adie single top ck en wf enable
    WB_SLP_TOP_CK_1_BSY[1]       - (RO) WB SG TOP EN WF clock control busy bit
                                     "1" mean busy.
    WB_SLP_TOP_CK_1_RSV[3..2]    - (RW) Reserved CR
    WB_SLP_MULT_TOP_CK_1[4]      - (RW) adie mult top ck en wf enable
    WB_SLP_MULT_TOP_CK_1_BSY[5]  - (RO) WB MULT TOP EN WF clock control busy bit
                                     "1" mean busy.
    WB_SLP_MULT_TOP_CK_1_RSV[7..6] - (RW) Reserved CR
    RESERVED8[31..8]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_1_WB_SLP_MULT_TOP_CK_1_RSV_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_1_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_1_WB_SLP_MULT_TOP_CK_1_RSV_MASK 0x000000C0                // WB_SLP_MULT_TOP_CK_1_RSV[7..6]
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_1_WB_SLP_MULT_TOP_CK_1_RSV_SHFT 6
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_1_WB_SLP_MULT_TOP_CK_1_BSY_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_1_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_1_WB_SLP_MULT_TOP_CK_1_BSY_MASK 0x00000020                // WB_SLP_MULT_TOP_CK_1_BSY[5]
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_1_WB_SLP_MULT_TOP_CK_1_BSY_SHFT 5
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_1_WB_SLP_MULT_TOP_CK_1_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_1_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_1_WB_SLP_MULT_TOP_CK_1_MASK 0x00000010                // WB_SLP_MULT_TOP_CK_1[4]
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_1_WB_SLP_MULT_TOP_CK_1_SHFT 4
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_1_WB_SLP_TOP_CK_1_RSV_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_1_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_1_WB_SLP_TOP_CK_1_RSV_MASK 0x0000000C                // WB_SLP_TOP_CK_1_RSV[3..2]
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_1_WB_SLP_TOP_CK_1_RSV_SHFT 2
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_1_WB_SLP_TOP_CK_1_BSY_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_1_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_1_WB_SLP_TOP_CK_1_BSY_MASK 0x00000002                // WB_SLP_TOP_CK_1_BSY[1]
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_1_WB_SLP_TOP_CK_1_BSY_SHFT 1
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_1_WB_SLP_TOP_CK_1_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_1_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_1_WB_SLP_TOP_CK_1_MASK 0x00000001                // WB_SLP_TOP_CK_1[0]
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_1_WB_SLP_TOP_CK_1_SHFT 0

/* =====================================================================================

  ---WB_SLP_TOP_CK_2 (0x18003000 + 0x128)---

    WB_SLP_TOP_CK_2[0]           - (RW) adie single top ck en bt enable
    WB_SLP_TOP_CK_2_BSY[1]       - (RO) WB SG TOP EN BT clock control busy bit
                                     "1" mean busy.
    WB_SLP_TOP_CK_2_RSV[3..2]    - (RW) Reserved CR
    WB_SLP_MULT_TOP_CK_2[4]      - (RW) adie mult top ck en bt enable
    WB_SLP_MULT_TOP_CK_2_BSY[5]  - (RO) WB MULT TOP EN BT clock control busy bit
                                     "1" mean busy.
    WB_SLP_MULT_TOP_CK_2_RSV[7..6] - (RW) Reserved CR
    RESERVED8[31..8]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_2_WB_SLP_MULT_TOP_CK_2_RSV_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_2_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_2_WB_SLP_MULT_TOP_CK_2_RSV_MASK 0x000000C0                // WB_SLP_MULT_TOP_CK_2_RSV[7..6]
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_2_WB_SLP_MULT_TOP_CK_2_RSV_SHFT 6
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_2_WB_SLP_MULT_TOP_CK_2_BSY_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_2_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_2_WB_SLP_MULT_TOP_CK_2_BSY_MASK 0x00000020                // WB_SLP_MULT_TOP_CK_2_BSY[5]
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_2_WB_SLP_MULT_TOP_CK_2_BSY_SHFT 5
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_2_WB_SLP_MULT_TOP_CK_2_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_2_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_2_WB_SLP_MULT_TOP_CK_2_MASK 0x00000010                // WB_SLP_MULT_TOP_CK_2[4]
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_2_WB_SLP_MULT_TOP_CK_2_SHFT 4
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_2_WB_SLP_TOP_CK_2_RSV_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_2_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_2_WB_SLP_TOP_CK_2_RSV_MASK 0x0000000C                // WB_SLP_TOP_CK_2_RSV[3..2]
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_2_WB_SLP_TOP_CK_2_RSV_SHFT 2
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_2_WB_SLP_TOP_CK_2_BSY_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_2_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_2_WB_SLP_TOP_CK_2_BSY_MASK 0x00000002                // WB_SLP_TOP_CK_2_BSY[1]
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_2_WB_SLP_TOP_CK_2_BSY_SHFT 1
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_2_WB_SLP_TOP_CK_2_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_2_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_2_WB_SLP_TOP_CK_2_MASK 0x00000001                // WB_SLP_TOP_CK_2[0]
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_2_WB_SLP_TOP_CK_2_SHFT 0

/* =====================================================================================

  ---WB_SLP_TOP_CK_3 (0x18003000 + 0x12C)---

    WB_SLP_TOP_CK_3[0]           - (RO) adie single top ck en bt1 enable
    WB_SLP_TOP_CK_3_BSY[1]       - (RO) WB SG TOP EN BT1 clock control busy bit
                                     "1" mean busy.
    WB_SLP_TOP_CK_3_RSV[3..2]    - (RO) Reserved CR
    WB_SLP_MULT_TOP_CK_3[4]      - (RO) adie mult top ck en bt1 enable
    WB_SLP_MULT_TOP_CK_3_BSY[5]  - (RO) WB MULT TOP EN BT1 clock control busy bit
                                     "1" mean busy.
    WB_SLP_MULT_TOP_CK_3_RSV[7..6] - (RO) Reserved CR
    RESERVED8[31..8]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_3_WB_SLP_MULT_TOP_CK_3_RSV_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_3_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_3_WB_SLP_MULT_TOP_CK_3_RSV_MASK 0x000000C0                // WB_SLP_MULT_TOP_CK_3_RSV[7..6]
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_3_WB_SLP_MULT_TOP_CK_3_RSV_SHFT 6
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_3_WB_SLP_MULT_TOP_CK_3_BSY_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_3_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_3_WB_SLP_MULT_TOP_CK_3_BSY_MASK 0x00000020                // WB_SLP_MULT_TOP_CK_3_BSY[5]
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_3_WB_SLP_MULT_TOP_CK_3_BSY_SHFT 5
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_3_WB_SLP_MULT_TOP_CK_3_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_3_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_3_WB_SLP_MULT_TOP_CK_3_MASK 0x00000010                // WB_SLP_MULT_TOP_CK_3[4]
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_3_WB_SLP_MULT_TOP_CK_3_SHFT 4
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_3_WB_SLP_TOP_CK_3_RSV_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_3_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_3_WB_SLP_TOP_CK_3_RSV_MASK 0x0000000C                // WB_SLP_TOP_CK_3_RSV[3..2]
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_3_WB_SLP_TOP_CK_3_RSV_SHFT 2
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_3_WB_SLP_TOP_CK_3_BSY_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_3_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_3_WB_SLP_TOP_CK_3_BSY_MASK 0x00000002                // WB_SLP_TOP_CK_3_BSY[1]
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_3_WB_SLP_TOP_CK_3_BSY_SHFT 1
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_3_WB_SLP_TOP_CK_3_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_3_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_3_WB_SLP_TOP_CK_3_MASK 0x00000001                // WB_SLP_TOP_CK_3[0]
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_3_WB_SLP_TOP_CK_3_SHFT 0

/* =====================================================================================

  ---WB_SLP_TOP_CK_4 (0x18003000 + 0x130)---

    WB_SLP_TOP_CK_4[0]           - (RW) adie single top ck en fm enable
    WB_SLP_TOP_CK_4_BSY[1]       - (RO) WB SG TOP EN FM clock control busy bit
                                     "1" mean busy.
    WB_SLP_TOP_CK_4_RSV[3..2]    - (RW) Reserved CR
    WB_SLP_MULT_TOP_CK_4[4]      - (RW) adie mult top ck en fm enable
    WB_SLP_MULT_TOP_CK_4_BSY[5]  - (RO) WB MULT TOP EN FM clock control busy bit
                                     "1" mean busy.
    WB_SLP_MULT_TOP_CK_4_RSV[7..6] - (RW) Reserved CR
    RESERVED8[31..8]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_4_WB_SLP_MULT_TOP_CK_4_RSV_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_4_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_4_WB_SLP_MULT_TOP_CK_4_RSV_MASK 0x000000C0                // WB_SLP_MULT_TOP_CK_4_RSV[7..6]
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_4_WB_SLP_MULT_TOP_CK_4_RSV_SHFT 6
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_4_WB_SLP_MULT_TOP_CK_4_BSY_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_4_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_4_WB_SLP_MULT_TOP_CK_4_BSY_MASK 0x00000020                // WB_SLP_MULT_TOP_CK_4_BSY[5]
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_4_WB_SLP_MULT_TOP_CK_4_BSY_SHFT 5
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_4_WB_SLP_MULT_TOP_CK_4_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_4_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_4_WB_SLP_MULT_TOP_CK_4_MASK 0x00000010                // WB_SLP_MULT_TOP_CK_4[4]
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_4_WB_SLP_MULT_TOP_CK_4_SHFT 4
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_4_WB_SLP_TOP_CK_4_RSV_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_4_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_4_WB_SLP_TOP_CK_4_RSV_MASK 0x0000000C                // WB_SLP_TOP_CK_4_RSV[3..2]
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_4_WB_SLP_TOP_CK_4_RSV_SHFT 2
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_4_WB_SLP_TOP_CK_4_BSY_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_4_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_4_WB_SLP_TOP_CK_4_BSY_MASK 0x00000002                // WB_SLP_TOP_CK_4_BSY[1]
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_4_WB_SLP_TOP_CK_4_BSY_SHFT 1
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_4_WB_SLP_TOP_CK_4_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_4_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_4_WB_SLP_TOP_CK_4_MASK 0x00000001                // WB_SLP_TOP_CK_4[0]
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_4_WB_SLP_TOP_CK_4_SHFT 0

/* =====================================================================================

  ---WB_SLP_TOP_CK_5 (0x18003000 + 0x134)---

    WB_SLP_TOP_CK_5[0]           - (RW) adie single top ck en gps enable
    WB_SLP_TOP_CK_5_BSY[1]       - (RO) WB SG TOP EN GPs clock control busy bit
                                     "1" mean busy.
    WB_SLP_TOP_CK_5_RSV[3..2]    - (RW) Reserved CR
    WB_SLP_MULT_TOP_CK_5[4]      - (RW) adie mult top ck en gps enable
    WB_SLP_MULT_TOP_CK_5_BSY[5]  - (RO) WB MULT TOP EN GPS clock control busy bit
                                     "1" mean busy.
    WB_SLP_MULT_TOP_CK_5_RSV[7..6] - (RW) Reserved CR
    RESERVED8[31..8]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_5_WB_SLP_MULT_TOP_CK_5_RSV_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_5_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_5_WB_SLP_MULT_TOP_CK_5_RSV_MASK 0x000000C0                // WB_SLP_MULT_TOP_CK_5_RSV[7..6]
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_5_WB_SLP_MULT_TOP_CK_5_RSV_SHFT 6
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_5_WB_SLP_MULT_TOP_CK_5_BSY_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_5_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_5_WB_SLP_MULT_TOP_CK_5_BSY_MASK 0x00000020                // WB_SLP_MULT_TOP_CK_5_BSY[5]
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_5_WB_SLP_MULT_TOP_CK_5_BSY_SHFT 5
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_5_WB_SLP_MULT_TOP_CK_5_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_5_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_5_WB_SLP_MULT_TOP_CK_5_MASK 0x00000010                // WB_SLP_MULT_TOP_CK_5[4]
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_5_WB_SLP_MULT_TOP_CK_5_SHFT 4
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_5_WB_SLP_TOP_CK_5_RSV_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_5_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_5_WB_SLP_TOP_CK_5_RSV_MASK 0x0000000C                // WB_SLP_TOP_CK_5_RSV[3..2]
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_5_WB_SLP_TOP_CK_5_RSV_SHFT 2
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_5_WB_SLP_TOP_CK_5_BSY_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_5_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_5_WB_SLP_TOP_CK_5_BSY_MASK 0x00000002                // WB_SLP_TOP_CK_5_BSY[1]
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_5_WB_SLP_TOP_CK_5_BSY_SHFT 1
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_5_WB_SLP_TOP_CK_5_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_5_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_5_WB_SLP_TOP_CK_5_MASK 0x00000001                // WB_SLP_TOP_CK_5[0]
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_5_WB_SLP_TOP_CK_5_SHFT 0

/* =====================================================================================

  ---WB_SLP_MULT_TOP_CK_0_ADDR (0x18003000 + 0x138)---

    WB_SLP_MULT_TOP_CK_0_ADDR[11..0] - (RW) A-DIE TOP_CK_EN address (used in the mode of multi top ck )
    RESERVED12[31..12]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_SLP_MULT_TOP_CK_0_ADDR_WB_SLP_MULT_TOP_CK_0_ADDR_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_MULT_TOP_CK_0_ADDR_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_MULT_TOP_CK_0_ADDR_WB_SLP_MULT_TOP_CK_0_ADDR_MASK 0x00000FFF                // WB_SLP_MULT_TOP_CK_0_ADDR[11..0]
#define CONN_WT_SLP_CTL_REG_WB_SLP_MULT_TOP_CK_0_ADDR_WB_SLP_MULT_TOP_CK_0_ADDR_SHFT 0

/* =====================================================================================

  ---WB_SLP_MULT_TOP_CK_1_ADDR (0x18003000 + 0x13C)---

    WB_SLP_MULT_TOP_CK_1_ADDR[11..0] - (RW) A-DIE TOP_CK_EN_WF address (used in the mode of multi top ck )
    RESERVED12[31..12]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_SLP_MULT_TOP_CK_1_ADDR_WB_SLP_MULT_TOP_CK_1_ADDR_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_MULT_TOP_CK_1_ADDR_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_MULT_TOP_CK_1_ADDR_WB_SLP_MULT_TOP_CK_1_ADDR_MASK 0x00000FFF                // WB_SLP_MULT_TOP_CK_1_ADDR[11..0]
#define CONN_WT_SLP_CTL_REG_WB_SLP_MULT_TOP_CK_1_ADDR_WB_SLP_MULT_TOP_CK_1_ADDR_SHFT 0

/* =====================================================================================

  ---WB_SLP_MULT_TOP_CK_2_ADDR (0x18003000 + 0x140)---

    WB_SLP_MULT_TOP_CK_2_ADDR[11..0] - (RW) A-DIE TOP_CK_EN_BT address (used in the mode of multi top ck )
    RESERVED12[31..12]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_SLP_MULT_TOP_CK_2_ADDR_WB_SLP_MULT_TOP_CK_2_ADDR_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_MULT_TOP_CK_2_ADDR_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_MULT_TOP_CK_2_ADDR_WB_SLP_MULT_TOP_CK_2_ADDR_MASK 0x00000FFF                // WB_SLP_MULT_TOP_CK_2_ADDR[11..0]
#define CONN_WT_SLP_CTL_REG_WB_SLP_MULT_TOP_CK_2_ADDR_WB_SLP_MULT_TOP_CK_2_ADDR_SHFT 0

/* =====================================================================================

  ---WB_SLP_MULT_TOP_CK_3_ADDR (0x18003000 + 0x144)---

    WB_SLP_MULT_TOP_CK_3_ADDR[11..0] - (RO) A-DIE TOP_CK_EN_BT1 address (used in the mode of multi top ck )
    RESERVED12[31..12]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_SLP_MULT_TOP_CK_3_ADDR_WB_SLP_MULT_TOP_CK_3_ADDR_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_MULT_TOP_CK_3_ADDR_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_MULT_TOP_CK_3_ADDR_WB_SLP_MULT_TOP_CK_3_ADDR_MASK 0x00000FFF                // WB_SLP_MULT_TOP_CK_3_ADDR[11..0]
#define CONN_WT_SLP_CTL_REG_WB_SLP_MULT_TOP_CK_3_ADDR_WB_SLP_MULT_TOP_CK_3_ADDR_SHFT 0

/* =====================================================================================

  ---WB_SLP_MULT_TOP_CK_4_ADDR (0x18003000 + 0x148)---

    WB_SLP_MULT_TOP_CK_4_ADDR[11..0] - (RW) A-DIE TOP_CK_EN_FM address (used in the mode of multi top ck )
    RESERVED12[31..12]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_SLP_MULT_TOP_CK_4_ADDR_WB_SLP_MULT_TOP_CK_4_ADDR_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_MULT_TOP_CK_4_ADDR_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_MULT_TOP_CK_4_ADDR_WB_SLP_MULT_TOP_CK_4_ADDR_MASK 0x00000FFF                // WB_SLP_MULT_TOP_CK_4_ADDR[11..0]
#define CONN_WT_SLP_CTL_REG_WB_SLP_MULT_TOP_CK_4_ADDR_WB_SLP_MULT_TOP_CK_4_ADDR_SHFT 0

/* =====================================================================================

  ---WB_SLP_MULT_TOP_CK_5_ADDR (0x18003000 + 0x14C)---

    WB_SLP_MULT_TOP_CK_5_ADDR[11..0] - (RW) A-DIE TOP_CK_EN_GPS address (used in the mode of multi top ck )
    RESERVED12[31..12]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_SLP_MULT_TOP_CK_5_ADDR_WB_SLP_MULT_TOP_CK_5_ADDR_ADDR CONN_WT_SLP_CTL_REG_WB_SLP_MULT_TOP_CK_5_ADDR_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_MULT_TOP_CK_5_ADDR_WB_SLP_MULT_TOP_CK_5_ADDR_MASK 0x00000FFF                // WB_SLP_MULT_TOP_CK_5_ADDR[11..0]
#define CONN_WT_SLP_CTL_REG_WB_SLP_MULT_TOP_CK_5_ADDR_WB_SLP_MULT_TOP_CK_5_ADDR_SHFT 0

/* =====================================================================================

  ---WB_STA3 (0x18003000 + 0x150)---

    TOP_STATE[6..0]              - (RO) TOP STATE
    RSPI_TOP_OFF_CMD_DONE[7]     - (RO) Finish the cmd of top ck off
    RSPI_TOP_ON_CMD_DONE[8]      - (RO) Finish the cmd of top ck on
    TOP_CK_SG_OFF_BSY[9]         - (RO) TOP STATE at OFF_BSY
    TOP_CK_SG_ON_BSY[10]         - (RO) TOP STATE at ON_BSY
    TOP_CLK_OFF_TRG[11]          - (RO) TOP STATE at OFF_TRG
    TOP_CLK_ON_TRG[12]           - (RO) TOP STATE at ON_TRG
    TOP_CK_OFF_PENDING[13]       - (RO) TOP_CK_OFF_PENDING 
                                     "1" mean off pending
    TOP_CK_ON_PENDING[14]        - (RO) TOP_CK_ON_PENDING 
                                     "1" mean on pending
    SG_TOPCLK_OFF_BSY[15]        - (RO) SG_TOP_CK_EN OFF BSY (ALL TOP CK EN used in mode of single top  ck)
    SG_TOPCLK_ON_BSY[16]         - (RO) SG_TOP_CK_EN ON BSY (ALL TOP CK EN used in mode of single top  ck)
    SG_TOPCLK_OFF_TRIG[17]       - (RO) SG_TOP_CK_EN OFF TRIG (ALL TOP CK EN used in mode of single top  ck)
    SG_TOPCLK_ON_TRIG[18]        - (RO) SG_TOP_CK_EN ON TRIG (ALL TOP CK EN used in mode of single top  ck)
    TOP_CK_EN_SG[19]             - (RO) SG_TOP_CK_EN (ALL TOP CK EN used in mode of single top  ck)
    WB_SLP_TOP_CK_5_OFF_STATE_BSY[20] - (RO) TOP_CK_4_OFF_STATE_BSY ( Disable top ck enable of  GPS)
                                     "1" mean busy
    WB_SLP_TOP_CK_5_ON_STATE_BSY[21] - (RO) TOP_CK_4_ON_STATE_BSY ( Enable top ck enable of GPS)
                                     "1" mean busy
    WB_SLP_TOP_CK_4_OFF_STATE_BSY[22] - (RO) TOP_CK_4_OFF_STATE_BSY ( Disable top ck enable of  FM)
                                     "1" mean busy
    WB_SLP_TOP_CK_4_ON_STATE_BSY[23] - (RO) TOP_CK_4_ON_STATE_BSY ( Enable top ck enable of FM)
                                     "1" mean busy
    WB_SLP_TOP_CK_3_OFF_STATE_BSY[24] - (RO) TOP_CK_3_OFF_STATE_BSY ( Disable top ck enable of  BT1)
                                     "1" mean busy
    WB_SLP_TOP_CK_3_ON_STATE_BSY[25] - (RO) TOP_CK_3_ON_STATE_BSY ( Enable top ck enable of BT1)
                                     "1" mean busy
    WB_SLP_TOP_CK_2_OFF_STATE_BSY[26] - (RO) TOP_CK_2_OFF_STATE_BSY ( Disable top ck enable of  BT)
                                     "1" mean busy
    WB_SLP_TOP_CK_2_ON_STATE_BSY[27] - (RO) TOP_CK_2_ON_STATE_BSY ( Enable top ck enable of BT)
                                     "1" mean busy
    WB_SLP_TOP_CK_1_OFF_STATE_BSY[28] - (RO) TOP_CK_1_OFF_STATE_BSY ( Disable top ck enable of WF )
                                     "1" mean busy
    WB_SLP_TOP_CK_1_ON_STATE_BSY[29] - (RO) TOP_CK_1_ON_STATE_BSY ( Enable top ck enable of WF)
                                     "1" mean busy
    WB_SLP_TOP_CK_0_OFF_STATE_BSY[30] - (RO) TOP_CK_0_OFF_STATE_BSY (Disable top ck )
                                     "1" mean busy
    WB_SLP_TOP_CK_0_ON_STATE_BSY[31] - (RO) TOP_CK_0_ON_STATE_BSY (Enable top ck)
                                     "1" mean busy

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_STA3_WB_SLP_TOP_CK_0_ON_STATE_BSY_ADDR CONN_WT_SLP_CTL_REG_WB_STA3_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA3_WB_SLP_TOP_CK_0_ON_STATE_BSY_MASK 0x80000000                // WB_SLP_TOP_CK_0_ON_STATE_BSY[31]
#define CONN_WT_SLP_CTL_REG_WB_STA3_WB_SLP_TOP_CK_0_ON_STATE_BSY_SHFT 31
#define CONN_WT_SLP_CTL_REG_WB_STA3_WB_SLP_TOP_CK_0_OFF_STATE_BSY_ADDR CONN_WT_SLP_CTL_REG_WB_STA3_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA3_WB_SLP_TOP_CK_0_OFF_STATE_BSY_MASK 0x40000000                // WB_SLP_TOP_CK_0_OFF_STATE_BSY[30]
#define CONN_WT_SLP_CTL_REG_WB_STA3_WB_SLP_TOP_CK_0_OFF_STATE_BSY_SHFT 30
#define CONN_WT_SLP_CTL_REG_WB_STA3_WB_SLP_TOP_CK_1_ON_STATE_BSY_ADDR CONN_WT_SLP_CTL_REG_WB_STA3_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA3_WB_SLP_TOP_CK_1_ON_STATE_BSY_MASK 0x20000000                // WB_SLP_TOP_CK_1_ON_STATE_BSY[29]
#define CONN_WT_SLP_CTL_REG_WB_STA3_WB_SLP_TOP_CK_1_ON_STATE_BSY_SHFT 29
#define CONN_WT_SLP_CTL_REG_WB_STA3_WB_SLP_TOP_CK_1_OFF_STATE_BSY_ADDR CONN_WT_SLP_CTL_REG_WB_STA3_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA3_WB_SLP_TOP_CK_1_OFF_STATE_BSY_MASK 0x10000000                // WB_SLP_TOP_CK_1_OFF_STATE_BSY[28]
#define CONN_WT_SLP_CTL_REG_WB_STA3_WB_SLP_TOP_CK_1_OFF_STATE_BSY_SHFT 28
#define CONN_WT_SLP_CTL_REG_WB_STA3_WB_SLP_TOP_CK_2_ON_STATE_BSY_ADDR CONN_WT_SLP_CTL_REG_WB_STA3_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA3_WB_SLP_TOP_CK_2_ON_STATE_BSY_MASK 0x08000000                // WB_SLP_TOP_CK_2_ON_STATE_BSY[27]
#define CONN_WT_SLP_CTL_REG_WB_STA3_WB_SLP_TOP_CK_2_ON_STATE_BSY_SHFT 27
#define CONN_WT_SLP_CTL_REG_WB_STA3_WB_SLP_TOP_CK_2_OFF_STATE_BSY_ADDR CONN_WT_SLP_CTL_REG_WB_STA3_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA3_WB_SLP_TOP_CK_2_OFF_STATE_BSY_MASK 0x04000000                // WB_SLP_TOP_CK_2_OFF_STATE_BSY[26]
#define CONN_WT_SLP_CTL_REG_WB_STA3_WB_SLP_TOP_CK_2_OFF_STATE_BSY_SHFT 26
#define CONN_WT_SLP_CTL_REG_WB_STA3_WB_SLP_TOP_CK_3_ON_STATE_BSY_ADDR CONN_WT_SLP_CTL_REG_WB_STA3_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA3_WB_SLP_TOP_CK_3_ON_STATE_BSY_MASK 0x02000000                // WB_SLP_TOP_CK_3_ON_STATE_BSY[25]
#define CONN_WT_SLP_CTL_REG_WB_STA3_WB_SLP_TOP_CK_3_ON_STATE_BSY_SHFT 25
#define CONN_WT_SLP_CTL_REG_WB_STA3_WB_SLP_TOP_CK_3_OFF_STATE_BSY_ADDR CONN_WT_SLP_CTL_REG_WB_STA3_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA3_WB_SLP_TOP_CK_3_OFF_STATE_BSY_MASK 0x01000000                // WB_SLP_TOP_CK_3_OFF_STATE_BSY[24]
#define CONN_WT_SLP_CTL_REG_WB_STA3_WB_SLP_TOP_CK_3_OFF_STATE_BSY_SHFT 24
#define CONN_WT_SLP_CTL_REG_WB_STA3_WB_SLP_TOP_CK_4_ON_STATE_BSY_ADDR CONN_WT_SLP_CTL_REG_WB_STA3_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA3_WB_SLP_TOP_CK_4_ON_STATE_BSY_MASK 0x00800000                // WB_SLP_TOP_CK_4_ON_STATE_BSY[23]
#define CONN_WT_SLP_CTL_REG_WB_STA3_WB_SLP_TOP_CK_4_ON_STATE_BSY_SHFT 23
#define CONN_WT_SLP_CTL_REG_WB_STA3_WB_SLP_TOP_CK_4_OFF_STATE_BSY_ADDR CONN_WT_SLP_CTL_REG_WB_STA3_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA3_WB_SLP_TOP_CK_4_OFF_STATE_BSY_MASK 0x00400000                // WB_SLP_TOP_CK_4_OFF_STATE_BSY[22]
#define CONN_WT_SLP_CTL_REG_WB_STA3_WB_SLP_TOP_CK_4_OFF_STATE_BSY_SHFT 22
#define CONN_WT_SLP_CTL_REG_WB_STA3_WB_SLP_TOP_CK_5_ON_STATE_BSY_ADDR CONN_WT_SLP_CTL_REG_WB_STA3_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA3_WB_SLP_TOP_CK_5_ON_STATE_BSY_MASK 0x00200000                // WB_SLP_TOP_CK_5_ON_STATE_BSY[21]
#define CONN_WT_SLP_CTL_REG_WB_STA3_WB_SLP_TOP_CK_5_ON_STATE_BSY_SHFT 21
#define CONN_WT_SLP_CTL_REG_WB_STA3_WB_SLP_TOP_CK_5_OFF_STATE_BSY_ADDR CONN_WT_SLP_CTL_REG_WB_STA3_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA3_WB_SLP_TOP_CK_5_OFF_STATE_BSY_MASK 0x00100000                // WB_SLP_TOP_CK_5_OFF_STATE_BSY[20]
#define CONN_WT_SLP_CTL_REG_WB_STA3_WB_SLP_TOP_CK_5_OFF_STATE_BSY_SHFT 20
#define CONN_WT_SLP_CTL_REG_WB_STA3_TOP_CK_EN_SG_ADDR          CONN_WT_SLP_CTL_REG_WB_STA3_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA3_TOP_CK_EN_SG_MASK          0x00080000                // TOP_CK_EN_SG[19]
#define CONN_WT_SLP_CTL_REG_WB_STA3_TOP_CK_EN_SG_SHFT          19
#define CONN_WT_SLP_CTL_REG_WB_STA3_SG_TOPCLK_ON_TRIG_ADDR     CONN_WT_SLP_CTL_REG_WB_STA3_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA3_SG_TOPCLK_ON_TRIG_MASK     0x00040000                // SG_TOPCLK_ON_TRIG[18]
#define CONN_WT_SLP_CTL_REG_WB_STA3_SG_TOPCLK_ON_TRIG_SHFT     18
#define CONN_WT_SLP_CTL_REG_WB_STA3_SG_TOPCLK_OFF_TRIG_ADDR    CONN_WT_SLP_CTL_REG_WB_STA3_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA3_SG_TOPCLK_OFF_TRIG_MASK    0x00020000                // SG_TOPCLK_OFF_TRIG[17]
#define CONN_WT_SLP_CTL_REG_WB_STA3_SG_TOPCLK_OFF_TRIG_SHFT    17
#define CONN_WT_SLP_CTL_REG_WB_STA3_SG_TOPCLK_ON_BSY_ADDR      CONN_WT_SLP_CTL_REG_WB_STA3_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA3_SG_TOPCLK_ON_BSY_MASK      0x00010000                // SG_TOPCLK_ON_BSY[16]
#define CONN_WT_SLP_CTL_REG_WB_STA3_SG_TOPCLK_ON_BSY_SHFT      16
#define CONN_WT_SLP_CTL_REG_WB_STA3_SG_TOPCLK_OFF_BSY_ADDR     CONN_WT_SLP_CTL_REG_WB_STA3_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA3_SG_TOPCLK_OFF_BSY_MASK     0x00008000                // SG_TOPCLK_OFF_BSY[15]
#define CONN_WT_SLP_CTL_REG_WB_STA3_SG_TOPCLK_OFF_BSY_SHFT     15
#define CONN_WT_SLP_CTL_REG_WB_STA3_TOP_CK_ON_PENDING_ADDR     CONN_WT_SLP_CTL_REG_WB_STA3_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA3_TOP_CK_ON_PENDING_MASK     0x00004000                // TOP_CK_ON_PENDING[14]
#define CONN_WT_SLP_CTL_REG_WB_STA3_TOP_CK_ON_PENDING_SHFT     14
#define CONN_WT_SLP_CTL_REG_WB_STA3_TOP_CK_OFF_PENDING_ADDR    CONN_WT_SLP_CTL_REG_WB_STA3_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA3_TOP_CK_OFF_PENDING_MASK    0x00002000                // TOP_CK_OFF_PENDING[13]
#define CONN_WT_SLP_CTL_REG_WB_STA3_TOP_CK_OFF_PENDING_SHFT    13
#define CONN_WT_SLP_CTL_REG_WB_STA3_TOP_CLK_ON_TRG_ADDR        CONN_WT_SLP_CTL_REG_WB_STA3_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA3_TOP_CLK_ON_TRG_MASK        0x00001000                // TOP_CLK_ON_TRG[12]
#define CONN_WT_SLP_CTL_REG_WB_STA3_TOP_CLK_ON_TRG_SHFT        12
#define CONN_WT_SLP_CTL_REG_WB_STA3_TOP_CLK_OFF_TRG_ADDR       CONN_WT_SLP_CTL_REG_WB_STA3_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA3_TOP_CLK_OFF_TRG_MASK       0x00000800                // TOP_CLK_OFF_TRG[11]
#define CONN_WT_SLP_CTL_REG_WB_STA3_TOP_CLK_OFF_TRG_SHFT       11
#define CONN_WT_SLP_CTL_REG_WB_STA3_TOP_CK_SG_ON_BSY_ADDR      CONN_WT_SLP_CTL_REG_WB_STA3_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA3_TOP_CK_SG_ON_BSY_MASK      0x00000400                // TOP_CK_SG_ON_BSY[10]
#define CONN_WT_SLP_CTL_REG_WB_STA3_TOP_CK_SG_ON_BSY_SHFT      10
#define CONN_WT_SLP_CTL_REG_WB_STA3_TOP_CK_SG_OFF_BSY_ADDR     CONN_WT_SLP_CTL_REG_WB_STA3_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA3_TOP_CK_SG_OFF_BSY_MASK     0x00000200                // TOP_CK_SG_OFF_BSY[9]
#define CONN_WT_SLP_CTL_REG_WB_STA3_TOP_CK_SG_OFF_BSY_SHFT     9
#define CONN_WT_SLP_CTL_REG_WB_STA3_RSPI_TOP_ON_CMD_DONE_ADDR  CONN_WT_SLP_CTL_REG_WB_STA3_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA3_RSPI_TOP_ON_CMD_DONE_MASK  0x00000100                // RSPI_TOP_ON_CMD_DONE[8]
#define CONN_WT_SLP_CTL_REG_WB_STA3_RSPI_TOP_ON_CMD_DONE_SHFT  8
#define CONN_WT_SLP_CTL_REG_WB_STA3_RSPI_TOP_OFF_CMD_DONE_ADDR CONN_WT_SLP_CTL_REG_WB_STA3_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA3_RSPI_TOP_OFF_CMD_DONE_MASK 0x00000080                // RSPI_TOP_OFF_CMD_DONE[7]
#define CONN_WT_SLP_CTL_REG_WB_STA3_RSPI_TOP_OFF_CMD_DONE_SHFT 7
#define CONN_WT_SLP_CTL_REG_WB_STA3_TOP_STATE_ADDR             CONN_WT_SLP_CTL_REG_WB_STA3_ADDR
#define CONN_WT_SLP_CTL_REG_WB_STA3_TOP_STATE_MASK             0x0000007F                // TOP_STATE[6..0]
#define CONN_WT_SLP_CTL_REG_WB_STA3_TOP_STATE_SHFT             0

/* =====================================================================================

  ---WB_BG_ADDR9 (0x18003000 + 0x154)---

    WB_BG_ADDR9[15..0]           - (RW) Bandgap SPI Address 9

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR9_WB_BG_ADDR9_ADDR       CONN_WT_SLP_CTL_REG_WB_BG_ADDR9_ADDR
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR9_WB_BG_ADDR9_MASK       0x0000FFFF                // WB_BG_ADDR9[15..0]
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR9_WB_BG_ADDR9_SHFT       0

/* =====================================================================================

  ---WB_BG_ADDR10 (0x18003000 + 0x158)---

    WB_BG_ADDR10[15..0]          - (RW) Bandgap SPI Address 10

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR10_WB_BG_ADDR10_ADDR     CONN_WT_SLP_CTL_REG_WB_BG_ADDR10_ADDR
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR10_WB_BG_ADDR10_MASK     0x0000FFFF                // WB_BG_ADDR10[15..0]
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR10_WB_BG_ADDR10_SHFT     0

/* =====================================================================================

  ---WB_BG_ADDR11 (0x18003000 + 0x15c)---

    WB_BG_ADDR11[15..0]          - (RW) Bandgap SPI Address 11

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR11_WB_BG_ADDR11_ADDR     CONN_WT_SLP_CTL_REG_WB_BG_ADDR11_ADDR
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR11_WB_BG_ADDR11_MASK     0x0000FFFF                // WB_BG_ADDR11[15..0]
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR11_WB_BG_ADDR11_SHFT     0

/* =====================================================================================

  ---WB_BG_ADDR12 (0x18003000 + 0x160)---

    WB_BG_ADDR12[15..0]          - (RW) Bandgap SPI Address 12

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR12_WB_BG_ADDR12_ADDR     CONN_WT_SLP_CTL_REG_WB_BG_ADDR12_ADDR
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR12_WB_BG_ADDR12_MASK     0x0000FFFF                // WB_BG_ADDR12[15..0]
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR12_WB_BG_ADDR12_SHFT     0

/* =====================================================================================

  ---WB_BG_ON9 (0x18003000 + 0x164)---

    WB_BG_ON9[31..0]             - (RW) Bandgap ON setting 9.
                                     Aligning with WB_BG_ADDR9

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_BG_ON9_WB_BG_ON9_ADDR           CONN_WT_SLP_CTL_REG_WB_BG_ON9_ADDR
#define CONN_WT_SLP_CTL_REG_WB_BG_ON9_WB_BG_ON9_MASK           0xFFFFFFFF                // WB_BG_ON9[31..0]
#define CONN_WT_SLP_CTL_REG_WB_BG_ON9_WB_BG_ON9_SHFT           0

/* =====================================================================================

  ---WB_BG_ON10 (0x18003000 + 0x168)---

    WB_BG_ON10[31..0]            - (RW) Bandgap ON setting 10.
                                     Aligning with WB_BG_ADDR10

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_BG_ON10_WB_BG_ON10_ADDR         CONN_WT_SLP_CTL_REG_WB_BG_ON10_ADDR
#define CONN_WT_SLP_CTL_REG_WB_BG_ON10_WB_BG_ON10_MASK         0xFFFFFFFF                // WB_BG_ON10[31..0]
#define CONN_WT_SLP_CTL_REG_WB_BG_ON10_WB_BG_ON10_SHFT         0

/* =====================================================================================

  ---WB_BG_ON11 (0x18003000 + 0x16C)---

    WB_BG_ON11[31..0]            - (RW) Bandgap ON setting 11.
                                     Aligning with WB_BG_ADDR11

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_BG_ON11_WB_BG_ON11_ADDR         CONN_WT_SLP_CTL_REG_WB_BG_ON11_ADDR
#define CONN_WT_SLP_CTL_REG_WB_BG_ON11_WB_BG_ON11_MASK         0xFFFFFFFF                // WB_BG_ON11[31..0]
#define CONN_WT_SLP_CTL_REG_WB_BG_ON11_WB_BG_ON11_SHFT         0

/* =====================================================================================

  ---WB_BG_ON12 (0x18003000 + 0x170)---

    WB_BG_ON12[31..0]            - (RW) Bandgap ON setting 12.
                                     Aligning with WB_BG_ADDR12

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_BG_ON12_WB_BG_ON12_ADDR         CONN_WT_SLP_CTL_REG_WB_BG_ON12_ADDR
#define CONN_WT_SLP_CTL_REG_WB_BG_ON12_WB_BG_ON12_MASK         0xFFFFFFFF                // WB_BG_ON12[31..0]
#define CONN_WT_SLP_CTL_REG_WB_BG_ON12_WB_BG_ON12_SHFT         0

/* =====================================================================================

  ---WB_BG_OFF9 (0x18003000 + 0x174)---

    WB_BG_OFF9[31..0]            - (RW) Bandgap OFF setting 9.
                                     Aligning with WB_BG_ADDR9

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF9_WB_BG_OFF9_ADDR         CONN_WT_SLP_CTL_REG_WB_BG_OFF9_ADDR
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF9_WB_BG_OFF9_MASK         0xFFFFFFFF                // WB_BG_OFF9[31..0]
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF9_WB_BG_OFF9_SHFT         0

/* =====================================================================================

  ---WB_BG_OFF10 (0x18003000 + 0x178)---

    WB_BG_OFF10[31..0]           - (RW) Bandgap OFF setting 10.
                                     Aligning with WB_BG_ADDR10

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF10_WB_BG_OFF10_ADDR       CONN_WT_SLP_CTL_REG_WB_BG_OFF10_ADDR
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF10_WB_BG_OFF10_MASK       0xFFFFFFFF                // WB_BG_OFF10[31..0]
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF10_WB_BG_OFF10_SHFT       0

/* =====================================================================================

  ---WB_BG_OFF11 (0x18003000 + 0x17C)---

    WB_BG_OFF11[31..0]           - (RW) Bandgap OFF setting 11.
                                     Aligning with WB_BG_ADDR11

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF11_WB_BG_OFF11_ADDR       CONN_WT_SLP_CTL_REG_WB_BG_OFF11_ADDR
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF11_WB_BG_OFF11_MASK       0xFFFFFFFF                // WB_BG_OFF11[31..0]
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF11_WB_BG_OFF11_SHFT       0

/* =====================================================================================

  ---WB_BG_OFF12 (0x18003000 + 0x180)---

    WB_BG_OFF12[31..0]           - (RW) Bandgap OFF setting 12.
                                     Aligning with WB_BG_ADDR12

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF12_WB_BG_OFF12_ADDR       CONN_WT_SLP_CTL_REG_WB_BG_OFF12_ADDR
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF12_WB_BG_OFF12_MASK       0xFFFFFFFF                // WB_BG_OFF12[31..0]
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF12_WB_BG_OFF12_SHFT       0

/* =====================================================================================

  ---WB_GPS_CTL (0x18003000 + 0x200)---

    GPS_HW_MODE_EN[0]            - (RW) GPS hardware mode enable.
    GPS_FAKE_CK_EN[1]            - (RW) GPS fake ck_en -> don't trun off BG
    WB_GPS_RSV[15..2]            - (RW) Reserved CR
    RESERVED16[31..16]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_GPS_CTL_WB_GPS_RSV_ADDR         CONN_WT_SLP_CTL_REG_WB_GPS_CTL_ADDR
#define CONN_WT_SLP_CTL_REG_WB_GPS_CTL_WB_GPS_RSV_MASK         0x0000FFFC                // WB_GPS_RSV[15..2]
#define CONN_WT_SLP_CTL_REG_WB_GPS_CTL_WB_GPS_RSV_SHFT         2
#define CONN_WT_SLP_CTL_REG_WB_GPS_CTL_GPS_FAKE_CK_EN_ADDR     CONN_WT_SLP_CTL_REG_WB_GPS_CTL_ADDR
#define CONN_WT_SLP_CTL_REG_WB_GPS_CTL_GPS_FAKE_CK_EN_MASK     0x00000002                // GPS_FAKE_CK_EN[1]
#define CONN_WT_SLP_CTL_REG_WB_GPS_CTL_GPS_FAKE_CK_EN_SHFT     1
#define CONN_WT_SLP_CTL_REG_WB_GPS_CTL_GPS_HW_MODE_EN_ADDR     CONN_WT_SLP_CTL_REG_WB_GPS_CTL_ADDR
#define CONN_WT_SLP_CTL_REG_WB_GPS_CTL_GPS_HW_MODE_EN_MASK     0x00000001                // GPS_HW_MODE_EN[0]
#define CONN_WT_SLP_CTL_REG_WB_GPS_CTL_GPS_HW_MODE_EN_SHFT     0

#endif // __CONN_WT_SLP_CTL_REG_REGS_H__
