/*
 * Copyright (C) 2019 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */
#ifndef __BGF_GPS_DMA_REGS_H__
#define __BGF_GPS_DMA_REGS_H__

#define BGF_GPS_DMA_BASE                                       0x80010000

#define BGF_GPS_DMA_DMA_GLBSTA_ADDR                            (BGF_GPS_DMA_BASE + 0x0000)
#define BGF_GPS_DMA_DMA_GLBLIMITER_ADDR                        (BGF_GPS_DMA_BASE + 0x0028)
#define BGF_GPS_DMA_DMA1_WPPT_ADDR                             (BGF_GPS_DMA_BASE + 0x0108)
#define BGF_GPS_DMA_DMA1_WPTO_ADDR                             (BGF_GPS_DMA_BASE + 0x010C)
#define BGF_GPS_DMA_DMA1_COUNT_ADDR                            (BGF_GPS_DMA_BASE + 0x0110)
#define BGF_GPS_DMA_DMA1_CON_ADDR                              (BGF_GPS_DMA_BASE + 0x0114)
#define BGF_GPS_DMA_DMA1_START_ADDR                            (BGF_GPS_DMA_BASE + 0x0118)
#define BGF_GPS_DMA_DMA1_INTSTA_ADDR                           (BGF_GPS_DMA_BASE + 0x011C)
#define BGF_GPS_DMA_DMA1_ACKINT_ADDR                           (BGF_GPS_DMA_BASE + 0x0120)
#define BGF_GPS_DMA_DMA1_RLCT_ADDR                             (BGF_GPS_DMA_BASE + 0x0124)
#define BGF_GPS_DMA_DMA1_LIMITER_ADDR                          (BGF_GPS_DMA_BASE + 0x0128)
#define BGF_GPS_DMA_DMA1_PGMADDR_ADDR                          (BGF_GPS_DMA_BASE + 0x012C)
#define BGF_GPS_DMA_DMA1_STATE_ADDR                            (BGF_GPS_DMA_BASE + 0x0148)
#define BGF_GPS_DMA_DMA2_WPPT_ADDR                             (BGF_GPS_DMA_BASE + 0x0208)
#define BGF_GPS_DMA_DMA2_WPTO_ADDR                             (BGF_GPS_DMA_BASE + 0x020C)
#define BGF_GPS_DMA_DMA2_COUNT_ADDR                            (BGF_GPS_DMA_BASE + 0x0210)
#define BGF_GPS_DMA_DMA2_CON_ADDR                              (BGF_GPS_DMA_BASE + 0x0214)
#define BGF_GPS_DMA_DMA2_START_ADDR                            (BGF_GPS_DMA_BASE + 0x0218)
#define BGF_GPS_DMA_DMA2_INTSTA_ADDR                           (BGF_GPS_DMA_BASE + 0x021C)
#define BGF_GPS_DMA_DMA2_ACKINT_ADDR                           (BGF_GPS_DMA_BASE + 0x0220)
#define BGF_GPS_DMA_DMA2_RLCT_ADDR                             (BGF_GPS_DMA_BASE + 0x0224)
#define BGF_GPS_DMA_DMA2_LIMITER_ADDR                          (BGF_GPS_DMA_BASE + 0x0228)
#define BGF_GPS_DMA_DMA2_PGMADDR_ADDR                          (BGF_GPS_DMA_BASE + 0x022C)
#define BGF_GPS_DMA_DMA2_STATE_ADDR                            (BGF_GPS_DMA_BASE + 0x0248)
#define BGF_GPS_DMA_DMA3_WPPT_ADDR                             (BGF_GPS_DMA_BASE + 0x0308)
#define BGF_GPS_DMA_DMA3_WPTO_ADDR                             (BGF_GPS_DMA_BASE + 0x030C)
#define BGF_GPS_DMA_DMA3_COUNT_ADDR                            (BGF_GPS_DMA_BASE + 0x0310)
#define BGF_GPS_DMA_DMA3_CON_ADDR                              (BGF_GPS_DMA_BASE + 0x0314)
#define BGF_GPS_DMA_DMA3_START_ADDR                            (BGF_GPS_DMA_BASE + 0x0318)
#define BGF_GPS_DMA_DMA3_INTSTA_ADDR                           (BGF_GPS_DMA_BASE + 0x031C)
#define BGF_GPS_DMA_DMA3_ACKINT_ADDR                           (BGF_GPS_DMA_BASE + 0x0320)
#define BGF_GPS_DMA_DMA3_RLCT_ADDR                             (BGF_GPS_DMA_BASE + 0x0324)
#define BGF_GPS_DMA_DMA3_LIMITER_ADDR                          (BGF_GPS_DMA_BASE + 0x0328)
#define BGF_GPS_DMA_DMA3_PGMADDR_ADDR                          (BGF_GPS_DMA_BASE + 0x032C)
#define BGF_GPS_DMA_DMA3_STATE_ADDR                            (BGF_GPS_DMA_BASE + 0x0348)
#define BGF_GPS_DMA_DMA4_WPPT_ADDR                             (BGF_GPS_DMA_BASE + 0x0408)
#define BGF_GPS_DMA_DMA4_WPTO_ADDR                             (BGF_GPS_DMA_BASE + 0x040C)
#define BGF_GPS_DMA_DMA4_COUNT_ADDR                            (BGF_GPS_DMA_BASE + 0x0410)
#define BGF_GPS_DMA_DMA4_CON_ADDR                              (BGF_GPS_DMA_BASE + 0x0414)
#define BGF_GPS_DMA_DMA4_START_ADDR                            (BGF_GPS_DMA_BASE + 0x0418)
#define BGF_GPS_DMA_DMA4_INTSTA_ADDR                           (BGF_GPS_DMA_BASE + 0x041C)
#define BGF_GPS_DMA_DMA4_ACKINT_ADDR                           (BGF_GPS_DMA_BASE + 0x0420)
#define BGF_GPS_DMA_DMA4_RLCT_ADDR                             (BGF_GPS_DMA_BASE + 0x0424)
#define BGF_GPS_DMA_DMA4_LIMITER_ADDR                          (BGF_GPS_DMA_BASE + 0x0428)
#define BGF_GPS_DMA_DMA4_PGMADDR_ADDR                          (BGF_GPS_DMA_BASE + 0x042C)
#define BGF_GPS_DMA_DMA4_STATE_ADDR                            (BGF_GPS_DMA_BASE + 0x0448)


#define BGF_GPS_DMA_DMA_GLBSTA_IT7_ADDR                        BGF_GPS_DMA_DMA_GLBSTA_ADDR
#define BGF_GPS_DMA_DMA_GLBSTA_IT7_MASK                        0x00002000
#define BGF_GPS_DMA_DMA_GLBSTA_IT7_SHFT                        13
#define BGF_GPS_DMA_DMA_GLBSTA_RUN7_ADDR                       BGF_GPS_DMA_DMA_GLBSTA_ADDR
#define BGF_GPS_DMA_DMA_GLBSTA_RUN7_MASK                       0x00001000
#define BGF_GPS_DMA_DMA_GLBSTA_RUN7_SHFT                       12
#define BGF_GPS_DMA_DMA_GLBSTA_IT6_ADDR                        BGF_GPS_DMA_DMA_GLBSTA_ADDR
#define BGF_GPS_DMA_DMA_GLBSTA_IT6_MASK                        0x00000800
#define BGF_GPS_DMA_DMA_GLBSTA_IT6_SHFT                        11
#define BGF_GPS_DMA_DMA_GLBSTA_RUN6_ADDR                       BGF_GPS_DMA_DMA_GLBSTA_ADDR
#define BGF_GPS_DMA_DMA_GLBSTA_RUN6_MASK                       0x00000400
#define BGF_GPS_DMA_DMA_GLBSTA_RUN6_SHFT                       10
#define BGF_GPS_DMA_DMA_GLBSTA_IT5_ADDR                        BGF_GPS_DMA_DMA_GLBSTA_ADDR
#define BGF_GPS_DMA_DMA_GLBSTA_IT5_MASK                        0x00000200
#define BGF_GPS_DMA_DMA_GLBSTA_IT5_SHFT                        9
#define BGF_GPS_DMA_DMA_GLBSTA_RUN5_ADDR                       BGF_GPS_DMA_DMA_GLBSTA_ADDR
#define BGF_GPS_DMA_DMA_GLBSTA_RUN5_MASK                       0x00000100
#define BGF_GPS_DMA_DMA_GLBSTA_RUN5_SHFT                       8
#define BGF_GPS_DMA_DMA_GLBSTA_IT4_ADDR                        BGF_GPS_DMA_DMA_GLBSTA_ADDR
#define BGF_GPS_DMA_DMA_GLBSTA_IT4_MASK                        0x00000080
#define BGF_GPS_DMA_DMA_GLBSTA_IT4_SHFT                        7
#define BGF_GPS_DMA_DMA_GLBSTA_RUN4_ADDR                       BGF_GPS_DMA_DMA_GLBSTA_ADDR
#define BGF_GPS_DMA_DMA_GLBSTA_RUN4_MASK                       0x00000040
#define BGF_GPS_DMA_DMA_GLBSTA_RUN4_SHFT                       6
#define BGF_GPS_DMA_DMA_GLBSTA_IT3_ADDR                        BGF_GPS_DMA_DMA_GLBSTA_ADDR
#define BGF_GPS_DMA_DMA_GLBSTA_IT3_MASK                        0x00000020
#define BGF_GPS_DMA_DMA_GLBSTA_IT3_SHFT                        5
#define BGF_GPS_DMA_DMA_GLBSTA_RUN3_ADDR                       BGF_GPS_DMA_DMA_GLBSTA_ADDR
#define BGF_GPS_DMA_DMA_GLBSTA_RUN3_MASK                       0x00000010
#define BGF_GPS_DMA_DMA_GLBSTA_RUN3_SHFT                       4
#define BGF_GPS_DMA_DMA_GLBSTA_IT2_ADDR                        BGF_GPS_DMA_DMA_GLBSTA_ADDR
#define BGF_GPS_DMA_DMA_GLBSTA_IT2_MASK                        0x00000008
#define BGF_GPS_DMA_DMA_GLBSTA_IT2_SHFT                        3
#define BGF_GPS_DMA_DMA_GLBSTA_RUN2_ADDR                       BGF_GPS_DMA_DMA_GLBSTA_ADDR
#define BGF_GPS_DMA_DMA_GLBSTA_RUN2_MASK                       0x00000004
#define BGF_GPS_DMA_DMA_GLBSTA_RUN2_SHFT                       2
#define BGF_GPS_DMA_DMA_GLBSTA_IT1_ADDR                        BGF_GPS_DMA_DMA_GLBSTA_ADDR
#define BGF_GPS_DMA_DMA_GLBSTA_IT1_MASK                        0x00000002
#define BGF_GPS_DMA_DMA_GLBSTA_IT1_SHFT                        1
#define BGF_GPS_DMA_DMA_GLBSTA_RUN1_ADDR                       BGF_GPS_DMA_DMA_GLBSTA_ADDR
#define BGF_GPS_DMA_DMA_GLBSTA_RUN1_MASK                       0x00000001
#define BGF_GPS_DMA_DMA_GLBSTA_RUN1_SHFT                       0

#define BGF_GPS_DMA_DMA_GLBLIMITER_GLBLIMITER_ADDR             BGF_GPS_DMA_DMA_GLBLIMITER_ADDR
#define BGF_GPS_DMA_DMA_GLBLIMITER_GLBLIMITER_MASK             0x000000FF
#define BGF_GPS_DMA_DMA_GLBLIMITER_GLBLIMITER_SHFT             0

#define BGF_GPS_DMA_DMA1_WPPT_WPPT_ADDR                        BGF_GPS_DMA_DMA1_WPPT_ADDR
#define BGF_GPS_DMA_DMA1_WPPT_WPPT_MASK                        0x0000FFFF
#define BGF_GPS_DMA_DMA1_WPPT_WPPT_SHFT                        0

#define BGF_GPS_DMA_DMA1_WPTO_WPTO_ADDR                        BGF_GPS_DMA_DMA1_WPTO_ADDR
#define BGF_GPS_DMA_DMA1_WPTO_WPTO_MASK                        0xFFFFFFFF
#define BGF_GPS_DMA_DMA1_WPTO_WPTO_SHFT                        0

#define BGF_GPS_DMA_DMA1_COUNT_LEN_ADDR                        BGF_GPS_DMA_DMA1_COUNT_ADDR
#define BGF_GPS_DMA_DMA1_COUNT_LEN_MASK                        0x0000FFFF
#define BGF_GPS_DMA_DMA1_COUNT_LEN_SHFT                        0

#define BGF_GPS_DMA_DMA1_CON_MAS_ADDR                          BGF_GPS_DMA_DMA1_CON_ADDR
#define BGF_GPS_DMA_DMA1_CON_MAS_MASK                          0x00300000
#define BGF_GPS_DMA_DMA1_CON_MAS_SHFT                          20
#define BGF_GPS_DMA_DMA1_CON_DIR_ADDR                          BGF_GPS_DMA_DMA1_CON_ADDR
#define BGF_GPS_DMA_DMA1_CON_DIR_MASK                          0x00040000
#define BGF_GPS_DMA_DMA1_CON_DIR_SHFT                          18
#define BGF_GPS_DMA_DMA1_CON_WPEN_ADDR                         BGF_GPS_DMA_DMA1_CON_ADDR
#define BGF_GPS_DMA_DMA1_CON_WPEN_MASK                         0x00020000
#define BGF_GPS_DMA_DMA1_CON_WPEN_SHFT                         17
#define BGF_GPS_DMA_DMA1_CON_WPSD_ADDR                         BGF_GPS_DMA_DMA1_CON_ADDR
#define BGF_GPS_DMA_DMA1_CON_WPSD_MASK                         0x00010000
#define BGF_GPS_DMA_DMA1_CON_WPSD_SHFT                         16
#define BGF_GPS_DMA_DMA1_CON_ITEN_ADDR                         BGF_GPS_DMA_DMA1_CON_ADDR
#define BGF_GPS_DMA_DMA1_CON_ITEN_MASK                         0x00008000
#define BGF_GPS_DMA_DMA1_CON_ITEN_SHFT                         15
#define BGF_GPS_DMA_DMA1_CON_APB_POST_WRITE_EN_ADDR            BGF_GPS_DMA_DMA1_CON_ADDR
#define BGF_GPS_DMA_DMA1_CON_APB_POST_WRITE_EN_MASK            0x00001000
#define BGF_GPS_DMA_DMA1_CON_APB_POST_WRITE_EN_SHFT            12
#define BGF_GPS_DMA_DMA1_CON_BURST_ADDR                        BGF_GPS_DMA_DMA1_CON_ADDR
#define BGF_GPS_DMA_DMA1_CON_BURST_MASK                        0x00000700
#define BGF_GPS_DMA_DMA1_CON_BURST_SHFT                        8
#define BGF_GPS_DMA_DMA1_CON_W2B_ADDR                          BGF_GPS_DMA_DMA1_CON_ADDR
#define BGF_GPS_DMA_DMA1_CON_W2B_MASK                          0x00000040
#define BGF_GPS_DMA_DMA1_CON_W2B_SHFT                          6
#define BGF_GPS_DMA_DMA1_CON_B2W_ADDR                          BGF_GPS_DMA_DMA1_CON_ADDR
#define BGF_GPS_DMA_DMA1_CON_B2W_MASK                          0x00000020
#define BGF_GPS_DMA_DMA1_CON_B2W_SHFT                          5
#define BGF_GPS_DMA_DMA1_CON_DREQ_ADDR                         BGF_GPS_DMA_DMA1_CON_ADDR
#define BGF_GPS_DMA_DMA1_CON_DREQ_MASK                         0x00000010
#define BGF_GPS_DMA_DMA1_CON_DREQ_SHFT                         4
#define BGF_GPS_DMA_DMA1_CON_DINC_ADDR                         BGF_GPS_DMA_DMA1_CON_ADDR
#define BGF_GPS_DMA_DMA1_CON_DINC_MASK                         0x00000008
#define BGF_GPS_DMA_DMA1_CON_DINC_SHFT                         3
#define BGF_GPS_DMA_DMA1_CON_SINC_ADDR                         BGF_GPS_DMA_DMA1_CON_ADDR
#define BGF_GPS_DMA_DMA1_CON_SINC_MASK                         0x00000004
#define BGF_GPS_DMA_DMA1_CON_SINC_SHFT                         2
#define BGF_GPS_DMA_DMA1_CON_SIZE_ADDR                         BGF_GPS_DMA_DMA1_CON_ADDR
#define BGF_GPS_DMA_DMA1_CON_SIZE_MASK                         0x00000003
#define BGF_GPS_DMA_DMA1_CON_SIZE_SHFT                         0

#define BGF_GPS_DMA_DMA1_START_STR_ADDR                        BGF_GPS_DMA_DMA1_START_ADDR
#define BGF_GPS_DMA_DMA1_START_STR_MASK                        0x00008000
#define BGF_GPS_DMA_DMA1_START_STR_SHFT                        15

#define BGF_GPS_DMA_DMA1_INTSTA_INT_ADDR                       BGF_GPS_DMA_DMA1_INTSTA_ADDR
#define BGF_GPS_DMA_DMA1_INTSTA_INT_MASK                       0x00008000
#define BGF_GPS_DMA_DMA1_INTSTA_INT_SHFT                       15

#define BGF_GPS_DMA_DMA1_ACKINT_ACK_ADDR                       BGF_GPS_DMA_DMA1_ACKINT_ADDR
#define BGF_GPS_DMA_DMA1_ACKINT_ACK_MASK                       0x00008000
#define BGF_GPS_DMA_DMA1_ACKINT_ACK_SHFT                       15

#define BGF_GPS_DMA_DMA1_RLCT_RLCT_ADDR                        BGF_GPS_DMA_DMA1_RLCT_ADDR
#define BGF_GPS_DMA_DMA1_RLCT_RLCT_MASK                        0x0000FFFF
#define BGF_GPS_DMA_DMA1_RLCT_RLCT_SHFT                        0

#define BGF_GPS_DMA_DMA1_LIMITER_LIMITER_ADDR                  BGF_GPS_DMA_DMA1_LIMITER_ADDR
#define BGF_GPS_DMA_DMA1_LIMITER_LIMITER_MASK                  0x000000FF
#define BGF_GPS_DMA_DMA1_LIMITER_LIMITER_SHFT                  0

#define BGF_GPS_DMA_DMA1_PGMADDR_PGMADDR_ADDR                  BGF_GPS_DMA_DMA1_PGMADDR_ADDR
#define BGF_GPS_DMA_DMA1_PGMADDR_PGMADDR_MASK                  0xFFFFFFFF
#define BGF_GPS_DMA_DMA1_PGMADDR_PGMADDR_SHFT                  0

#define BGF_GPS_DMA_DMA1_STATE_STATE_ADDR                      BGF_GPS_DMA_DMA1_STATE_ADDR
#define BGF_GPS_DMA_DMA1_STATE_STATE_MASK                      0x0000007F
#define BGF_GPS_DMA_DMA1_STATE_STATE_SHFT                      0

#define BGF_GPS_DMA_DMA2_WPPT_WPPT_ADDR                        BGF_GPS_DMA_DMA2_WPPT_ADDR
#define BGF_GPS_DMA_DMA2_WPPT_WPPT_MASK                        0x0000FFFF
#define BGF_GPS_DMA_DMA2_WPPT_WPPT_SHFT                        0

#define BGF_GPS_DMA_DMA2_WPTO_WPTO_ADDR                        BGF_GPS_DMA_DMA2_WPTO_ADDR
#define BGF_GPS_DMA_DMA2_WPTO_WPTO_MASK                        0xFFFFFFFF
#define BGF_GPS_DMA_DMA2_WPTO_WPTO_SHFT                        0

#define BGF_GPS_DMA_DMA2_COUNT_LEN_ADDR                        BGF_GPS_DMA_DMA2_COUNT_ADDR
#define BGF_GPS_DMA_DMA2_COUNT_LEN_MASK                        0x0000FFFF
#define BGF_GPS_DMA_DMA2_COUNT_LEN_SHFT                        0

#define BGF_GPS_DMA_DMA2_CON_MAS_ADDR                          BGF_GPS_DMA_DMA2_CON_ADDR
#define BGF_GPS_DMA_DMA2_CON_MAS_MASK                          0x00300000
#define BGF_GPS_DMA_DMA2_CON_MAS_SHFT                          20
#define BGF_GPS_DMA_DMA2_CON_DIR_ADDR                          BGF_GPS_DMA_DMA2_CON_ADDR
#define BGF_GPS_DMA_DMA2_CON_DIR_MASK                          0x00040000
#define BGF_GPS_DMA_DMA2_CON_DIR_SHFT                          18
#define BGF_GPS_DMA_DMA2_CON_WPEN_ADDR                         BGF_GPS_DMA_DMA2_CON_ADDR
#define BGF_GPS_DMA_DMA2_CON_WPEN_MASK                         0x00020000
#define BGF_GPS_DMA_DMA2_CON_WPEN_SHFT                         17
#define BGF_GPS_DMA_DMA2_CON_WPSD_ADDR                         BGF_GPS_DMA_DMA2_CON_ADDR
#define BGF_GPS_DMA_DMA2_CON_WPSD_MASK                         0x00010000
#define BGF_GPS_DMA_DMA2_CON_WPSD_SHFT                         16
#define BGF_GPS_DMA_DMA2_CON_ITEN_ADDR                         BGF_GPS_DMA_DMA2_CON_ADDR
#define BGF_GPS_DMA_DMA2_CON_ITEN_MASK                         0x00008000
#define BGF_GPS_DMA_DMA2_CON_ITEN_SHFT                         15
#define BGF_GPS_DMA_DMA2_CON_APB_POST_WRITE_EN_ADDR            BGF_GPS_DMA_DMA2_CON_ADDR
#define BGF_GPS_DMA_DMA2_CON_APB_POST_WRITE_EN_MASK            0x00001000
#define BGF_GPS_DMA_DMA2_CON_APB_POST_WRITE_EN_SHFT            12
#define BGF_GPS_DMA_DMA2_CON_BURST_ADDR                        BGF_GPS_DMA_DMA2_CON_ADDR
#define BGF_GPS_DMA_DMA2_CON_BURST_MASK                        0x00000700
#define BGF_GPS_DMA_DMA2_CON_BURST_SHFT                        8
#define BGF_GPS_DMA_DMA2_CON_W2B_ADDR                          BGF_GPS_DMA_DMA2_CON_ADDR
#define BGF_GPS_DMA_DMA2_CON_W2B_MASK                          0x00000040
#define BGF_GPS_DMA_DMA2_CON_W2B_SHFT                          6
#define BGF_GPS_DMA_DMA2_CON_B2W_ADDR                          BGF_GPS_DMA_DMA2_CON_ADDR
#define BGF_GPS_DMA_DMA2_CON_B2W_MASK                          0x00000020
#define BGF_GPS_DMA_DMA2_CON_B2W_SHFT                          5
#define BGF_GPS_DMA_DMA2_CON_DREQ_ADDR                         BGF_GPS_DMA_DMA2_CON_ADDR
#define BGF_GPS_DMA_DMA2_CON_DREQ_MASK                         0x00000010
#define BGF_GPS_DMA_DMA2_CON_DREQ_SHFT                         4
#define BGF_GPS_DMA_DMA2_CON_DINC_ADDR                         BGF_GPS_DMA_DMA2_CON_ADDR
#define BGF_GPS_DMA_DMA2_CON_DINC_MASK                         0x00000008
#define BGF_GPS_DMA_DMA2_CON_DINC_SHFT                         3
#define BGF_GPS_DMA_DMA2_CON_SINC_ADDR                         BGF_GPS_DMA_DMA2_CON_ADDR
#define BGF_GPS_DMA_DMA2_CON_SINC_MASK                         0x00000004
#define BGF_GPS_DMA_DMA2_CON_SINC_SHFT                         2
#define BGF_GPS_DMA_DMA2_CON_SIZE_ADDR                         BGF_GPS_DMA_DMA2_CON_ADDR
#define BGF_GPS_DMA_DMA2_CON_SIZE_MASK                         0x00000003
#define BGF_GPS_DMA_DMA2_CON_SIZE_SHFT                         0

#define BGF_GPS_DMA_DMA2_START_STR_ADDR                        BGF_GPS_DMA_DMA2_START_ADDR
#define BGF_GPS_DMA_DMA2_START_STR_MASK                        0x00008000
#define BGF_GPS_DMA_DMA2_START_STR_SHFT                        15

#define BGF_GPS_DMA_DMA2_INTSTA_INT_ADDR                       BGF_GPS_DMA_DMA2_INTSTA_ADDR
#define BGF_GPS_DMA_DMA2_INTSTA_INT_MASK                       0x00008000
#define BGF_GPS_DMA_DMA2_INTSTA_INT_SHFT                       15

#define BGF_GPS_DMA_DMA2_ACKINT_ACK_ADDR                       BGF_GPS_DMA_DMA2_ACKINT_ADDR
#define BGF_GPS_DMA_DMA2_ACKINT_ACK_MASK                       0x00008000
#define BGF_GPS_DMA_DMA2_ACKINT_ACK_SHFT                       15

#define BGF_GPS_DMA_DMA2_RLCT_RLCT_ADDR                        BGF_GPS_DMA_DMA2_RLCT_ADDR
#define BGF_GPS_DMA_DMA2_RLCT_RLCT_MASK                        0x0000FFFF
#define BGF_GPS_DMA_DMA2_RLCT_RLCT_SHFT                        0

#define BGF_GPS_DMA_DMA2_LIMITER_LIMITER_ADDR                  BGF_GPS_DMA_DMA2_LIMITER_ADDR
#define BGF_GPS_DMA_DMA2_LIMITER_LIMITER_MASK                  0x000000FF
#define BGF_GPS_DMA_DMA2_LIMITER_LIMITER_SHFT                  0

#define BGF_GPS_DMA_DMA2_PGMADDR_PGMADDR_ADDR                  BGF_GPS_DMA_DMA2_PGMADDR_ADDR
#define BGF_GPS_DMA_DMA2_PGMADDR_PGMADDR_MASK                  0xFFFFFFFF
#define BGF_GPS_DMA_DMA2_PGMADDR_PGMADDR_SHFT                  0

#define BGF_GPS_DMA_DMA2_STATE_STATE_ADDR                      BGF_GPS_DMA_DMA2_STATE_ADDR
#define BGF_GPS_DMA_DMA2_STATE_STATE_MASK                      0x0000007F
#define BGF_GPS_DMA_DMA2_STATE_STATE_SHFT                      0

#define BGF_GPS_DMA_DMA3_WPPT_WPPT_ADDR                        BGF_GPS_DMA_DMA3_WPPT_ADDR
#define BGF_GPS_DMA_DMA3_WPPT_WPPT_MASK                        0x0000FFFF
#define BGF_GPS_DMA_DMA3_WPPT_WPPT_SHFT                        0

#define BGF_GPS_DMA_DMA3_WPTO_WPTO_ADDR                        BGF_GPS_DMA_DMA3_WPTO_ADDR
#define BGF_GPS_DMA_DMA3_WPTO_WPTO_MASK                        0xFFFFFFFF
#define BGF_GPS_DMA_DMA3_WPTO_WPTO_SHFT                        0

#define BGF_GPS_DMA_DMA3_COUNT_LEN_ADDR                        BGF_GPS_DMA_DMA3_COUNT_ADDR
#define BGF_GPS_DMA_DMA3_COUNT_LEN_MASK                        0x0000FFFF
#define BGF_GPS_DMA_DMA3_COUNT_LEN_SHFT                        0

#define BGF_GPS_DMA_DMA3_CON_MAS_ADDR                          BGF_GPS_DMA_DMA3_CON_ADDR
#define BGF_GPS_DMA_DMA3_CON_MAS_MASK                          0x00300000
#define BGF_GPS_DMA_DMA3_CON_MAS_SHFT                          20
#define BGF_GPS_DMA_DMA3_CON_DIR_ADDR                          BGF_GPS_DMA_DMA3_CON_ADDR
#define BGF_GPS_DMA_DMA3_CON_DIR_MASK                          0x00040000
#define BGF_GPS_DMA_DMA3_CON_DIR_SHFT                          18
#define BGF_GPS_DMA_DMA3_CON_WPEN_ADDR                         BGF_GPS_DMA_DMA3_CON_ADDR
#define BGF_GPS_DMA_DMA3_CON_WPEN_MASK                         0x00020000
#define BGF_GPS_DMA_DMA3_CON_WPEN_SHFT                         17
#define BGF_GPS_DMA_DMA3_CON_WPSD_ADDR                         BGF_GPS_DMA_DMA3_CON_ADDR
#define BGF_GPS_DMA_DMA3_CON_WPSD_MASK                         0x00010000
#define BGF_GPS_DMA_DMA3_CON_WPSD_SHFT                         16
#define BGF_GPS_DMA_DMA3_CON_ITEN_ADDR                         BGF_GPS_DMA_DMA3_CON_ADDR
#define BGF_GPS_DMA_DMA3_CON_ITEN_MASK                         0x00008000
#define BGF_GPS_DMA_DMA3_CON_ITEN_SHFT                         15
#define BGF_GPS_DMA_DMA3_CON_APB_POST_WRITE_EN_ADDR            BGF_GPS_DMA_DMA3_CON_ADDR
#define BGF_GPS_DMA_DMA3_CON_APB_POST_WRITE_EN_MASK            0x00001000
#define BGF_GPS_DMA_DMA3_CON_APB_POST_WRITE_EN_SHFT            12
#define BGF_GPS_DMA_DMA3_CON_BURST_ADDR                        BGF_GPS_DMA_DMA3_CON_ADDR
#define BGF_GPS_DMA_DMA3_CON_BURST_MASK                        0x00000700
#define BGF_GPS_DMA_DMA3_CON_BURST_SHFT                        8
#define BGF_GPS_DMA_DMA3_CON_W2B_ADDR                          BGF_GPS_DMA_DMA3_CON_ADDR
#define BGF_GPS_DMA_DMA3_CON_W2B_MASK                          0x00000040
#define BGF_GPS_DMA_DMA3_CON_W2B_SHFT                          6
#define BGF_GPS_DMA_DMA3_CON_B2W_ADDR                          BGF_GPS_DMA_DMA3_CON_ADDR
#define BGF_GPS_DMA_DMA3_CON_B2W_MASK                          0x00000020
#define BGF_GPS_DMA_DMA3_CON_B2W_SHFT                          5
#define BGF_GPS_DMA_DMA3_CON_DREQ_ADDR                         BGF_GPS_DMA_DMA3_CON_ADDR
#define BGF_GPS_DMA_DMA3_CON_DREQ_MASK                         0x00000010
#define BGF_GPS_DMA_DMA3_CON_DREQ_SHFT                         4
#define BGF_GPS_DMA_DMA3_CON_DINC_ADDR                         BGF_GPS_DMA_DMA3_CON_ADDR
#define BGF_GPS_DMA_DMA3_CON_DINC_MASK                         0x00000008
#define BGF_GPS_DMA_DMA3_CON_DINC_SHFT                         3
#define BGF_GPS_DMA_DMA3_CON_SINC_ADDR                         BGF_GPS_DMA_DMA3_CON_ADDR
#define BGF_GPS_DMA_DMA3_CON_SINC_MASK                         0x00000004
#define BGF_GPS_DMA_DMA3_CON_SINC_SHFT                         2
#define BGF_GPS_DMA_DMA3_CON_SIZE_ADDR                         BGF_GPS_DMA_DMA3_CON_ADDR
#define BGF_GPS_DMA_DMA3_CON_SIZE_MASK                         0x00000003
#define BGF_GPS_DMA_DMA3_CON_SIZE_SHFT                         0

#define BGF_GPS_DMA_DMA3_START_STR_ADDR                        BGF_GPS_DMA_DMA3_START_ADDR
#define BGF_GPS_DMA_DMA3_START_STR_MASK                        0x00008000
#define BGF_GPS_DMA_DMA3_START_STR_SHFT                        15

#define BGF_GPS_DMA_DMA3_INTSTA_INT_ADDR                       BGF_GPS_DMA_DMA3_INTSTA_ADDR
#define BGF_GPS_DMA_DMA3_INTSTA_INT_MASK                       0x00008000
#define BGF_GPS_DMA_DMA3_INTSTA_INT_SHFT                       15

#define BGF_GPS_DMA_DMA3_ACKINT_ACK_ADDR                       BGF_GPS_DMA_DMA3_ACKINT_ADDR
#define BGF_GPS_DMA_DMA3_ACKINT_ACK_MASK                       0x00008000
#define BGF_GPS_DMA_DMA3_ACKINT_ACK_SHFT                       15

#define BGF_GPS_DMA_DMA3_RLCT_RLCT_ADDR                        BGF_GPS_DMA_DMA3_RLCT_ADDR
#define BGF_GPS_DMA_DMA3_RLCT_RLCT_MASK                        0x0000FFFF
#define BGF_GPS_DMA_DMA3_RLCT_RLCT_SHFT                        0

#define BGF_GPS_DMA_DMA3_LIMITER_LIMITER_ADDR                  BGF_GPS_DMA_DMA3_LIMITER_ADDR
#define BGF_GPS_DMA_DMA3_LIMITER_LIMITER_MASK                  0x000000FF
#define BGF_GPS_DMA_DMA3_LIMITER_LIMITER_SHFT                  0

#define BGF_GPS_DMA_DMA3_PGMADDR_PGMADDR_ADDR                  BGF_GPS_DMA_DMA3_PGMADDR_ADDR
#define BGF_GPS_DMA_DMA3_PGMADDR_PGMADDR_MASK                  0xFFFFFFFF
#define BGF_GPS_DMA_DMA3_PGMADDR_PGMADDR_SHFT                  0

#define BGF_GPS_DMA_DMA3_STATE_STATE_ADDR                      BGF_GPS_DMA_DMA3_STATE_ADDR
#define BGF_GPS_DMA_DMA3_STATE_STATE_MASK                      0x0000007F
#define BGF_GPS_DMA_DMA3_STATE_STATE_SHFT                      0

#define BGF_GPS_DMA_DMA4_WPPT_WPPT_ADDR                        BGF_GPS_DMA_DMA4_WPPT_ADDR
#define BGF_GPS_DMA_DMA4_WPPT_WPPT_MASK                        0x0000FFFF
#define BGF_GPS_DMA_DMA4_WPPT_WPPT_SHFT                        0

#define BGF_GPS_DMA_DMA4_WPTO_WPTO_ADDR                        BGF_GPS_DMA_DMA4_WPTO_ADDR
#define BGF_GPS_DMA_DMA4_WPTO_WPTO_MASK                        0xFFFFFFFF
#define BGF_GPS_DMA_DMA4_WPTO_WPTO_SHFT                        0

#define BGF_GPS_DMA_DMA4_COUNT_LEN_ADDR                        BGF_GPS_DMA_DMA4_COUNT_ADDR
#define BGF_GPS_DMA_DMA4_COUNT_LEN_MASK                        0x0000FFFF
#define BGF_GPS_DMA_DMA4_COUNT_LEN_SHFT                        0

#define BGF_GPS_DMA_DMA4_CON_MAS_ADDR                          BGF_GPS_DMA_DMA4_CON_ADDR
#define BGF_GPS_DMA_DMA4_CON_MAS_MASK                          0x00300000
#define BGF_GPS_DMA_DMA4_CON_MAS_SHFT                          20
#define BGF_GPS_DMA_DMA4_CON_DIR_ADDR                          BGF_GPS_DMA_DMA4_CON_ADDR
#define BGF_GPS_DMA_DMA4_CON_DIR_MASK                          0x00040000
#define BGF_GPS_DMA_DMA4_CON_DIR_SHFT                          18
#define BGF_GPS_DMA_DMA4_CON_WPEN_ADDR                         BGF_GPS_DMA_DMA4_CON_ADDR
#define BGF_GPS_DMA_DMA4_CON_WPEN_MASK                         0x00020000
#define BGF_GPS_DMA_DMA4_CON_WPEN_SHFT                         17
#define BGF_GPS_DMA_DMA4_CON_WPSD_ADDR                         BGF_GPS_DMA_DMA4_CON_ADDR
#define BGF_GPS_DMA_DMA4_CON_WPSD_MASK                         0x00010000
#define BGF_GPS_DMA_DMA4_CON_WPSD_SHFT                         16
#define BGF_GPS_DMA_DMA4_CON_ITEN_ADDR                         BGF_GPS_DMA_DMA4_CON_ADDR
#define BGF_GPS_DMA_DMA4_CON_ITEN_MASK                         0x00008000
#define BGF_GPS_DMA_DMA4_CON_ITEN_SHFT                         15
#define BGF_GPS_DMA_DMA4_CON_APB_POST_WRITE_EN_ADDR            BGF_GPS_DMA_DMA4_CON_ADDR
#define BGF_GPS_DMA_DMA4_CON_APB_POST_WRITE_EN_MASK            0x00001000
#define BGF_GPS_DMA_DMA4_CON_APB_POST_WRITE_EN_SHFT            12
#define BGF_GPS_DMA_DMA4_CON_BURST_ADDR                        BGF_GPS_DMA_DMA4_CON_ADDR
#define BGF_GPS_DMA_DMA4_CON_BURST_MASK                        0x00000700
#define BGF_GPS_DMA_DMA4_CON_BURST_SHFT                        8
#define BGF_GPS_DMA_DMA4_CON_W2B_ADDR                          BGF_GPS_DMA_DMA4_CON_ADDR
#define BGF_GPS_DMA_DMA4_CON_W2B_MASK                          0x00000040
#define BGF_GPS_DMA_DMA4_CON_W2B_SHFT                          6
#define BGF_GPS_DMA_DMA4_CON_B2W_ADDR                          BGF_GPS_DMA_DMA4_CON_ADDR
#define BGF_GPS_DMA_DMA4_CON_B2W_MASK                          0x00000020
#define BGF_GPS_DMA_DMA4_CON_B2W_SHFT                          5
#define BGF_GPS_DMA_DMA4_CON_DREQ_ADDR                         BGF_GPS_DMA_DMA4_CON_ADDR
#define BGF_GPS_DMA_DMA4_CON_DREQ_MASK                         0x00000010
#define BGF_GPS_DMA_DMA4_CON_DREQ_SHFT                         4
#define BGF_GPS_DMA_DMA4_CON_DINC_ADDR                         BGF_GPS_DMA_DMA4_CON_ADDR
#define BGF_GPS_DMA_DMA4_CON_DINC_MASK                         0x00000008
#define BGF_GPS_DMA_DMA4_CON_DINC_SHFT                         3
#define BGF_GPS_DMA_DMA4_CON_SINC_ADDR                         BGF_GPS_DMA_DMA4_CON_ADDR
#define BGF_GPS_DMA_DMA4_CON_SINC_MASK                         0x00000004
#define BGF_GPS_DMA_DMA4_CON_SINC_SHFT                         2
#define BGF_GPS_DMA_DMA4_CON_SIZE_ADDR                         BGF_GPS_DMA_DMA4_CON_ADDR
#define BGF_GPS_DMA_DMA4_CON_SIZE_MASK                         0x00000003
#define BGF_GPS_DMA_DMA4_CON_SIZE_SHFT                         0

#define BGF_GPS_DMA_DMA4_START_STR_ADDR                        BGF_GPS_DMA_DMA4_START_ADDR
#define BGF_GPS_DMA_DMA4_START_STR_MASK                        0x00008000
#define BGF_GPS_DMA_DMA4_START_STR_SHFT                        15

#define BGF_GPS_DMA_DMA4_INTSTA_INT_ADDR                       BGF_GPS_DMA_DMA4_INTSTA_ADDR
#define BGF_GPS_DMA_DMA4_INTSTA_INT_MASK                       0x00008000
#define BGF_GPS_DMA_DMA4_INTSTA_INT_SHFT                       15

#define BGF_GPS_DMA_DMA4_ACKINT_ACK_ADDR                       BGF_GPS_DMA_DMA4_ACKINT_ADDR
#define BGF_GPS_DMA_DMA4_ACKINT_ACK_MASK                       0x00008000
#define BGF_GPS_DMA_DMA4_ACKINT_ACK_SHFT                       15

#define BGF_GPS_DMA_DMA4_RLCT_RLCT_ADDR                        BGF_GPS_DMA_DMA4_RLCT_ADDR
#define BGF_GPS_DMA_DMA4_RLCT_RLCT_MASK                        0x0000FFFF
#define BGF_GPS_DMA_DMA4_RLCT_RLCT_SHFT                        0

#define BGF_GPS_DMA_DMA4_LIMITER_LIMITER_ADDR                  BGF_GPS_DMA_DMA4_LIMITER_ADDR
#define BGF_GPS_DMA_DMA4_LIMITER_LIMITER_MASK                  0x000000FF
#define BGF_GPS_DMA_DMA4_LIMITER_LIMITER_SHFT                  0

#define BGF_GPS_DMA_DMA4_PGMADDR_PGMADDR_ADDR                  BGF_GPS_DMA_DMA4_PGMADDR_ADDR
#define BGF_GPS_DMA_DMA4_PGMADDR_PGMADDR_MASK                  0xFFFFFFFF
#define BGF_GPS_DMA_DMA4_PGMADDR_PGMADDR_SHFT                  0

#define BGF_GPS_DMA_DMA4_STATE_STATE_ADDR                      BGF_GPS_DMA_DMA4_STATE_ADDR
#define BGF_GPS_DMA_DMA4_STATE_STATE_MASK                      0x0000007F
#define BGF_GPS_DMA_DMA4_STATE_STATE_SHFT                      0

#endif /* __BGF_GPS_DMA_REGS_H__ */

