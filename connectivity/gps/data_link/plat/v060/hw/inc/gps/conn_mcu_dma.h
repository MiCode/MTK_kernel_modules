/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef __CONN_MCU_DMA_REGS_H__
#define __CONN_MCU_DMA_REGS_H__

#define CONN_MCU_DMA_BASE                                      0x80010000

#define CONN_MCU_DMA_DMA7_WPPT_ADDR                            (CONN_MCU_DMA_BASE + 0x0708)
#define CONN_MCU_DMA_DMA7_WPTO_ADDR                            (CONN_MCU_DMA_BASE + 0x070C)
#define CONN_MCU_DMA_DMA7_COUNT_ADDR                           (CONN_MCU_DMA_BASE + 0x0710)
#define CONN_MCU_DMA_DMA7_CON_ADDR                             (CONN_MCU_DMA_BASE + 0x0714)
#define CONN_MCU_DMA_DMA7_START_ADDR                           (CONN_MCU_DMA_BASE + 0x0718)
#define CONN_MCU_DMA_DMA7_INTSTA_ADDR                          (CONN_MCU_DMA_BASE + 0x071C)
#define CONN_MCU_DMA_DMA7_ACKINT_ADDR                          (CONN_MCU_DMA_BASE + 0x0720)
#define CONN_MCU_DMA_DMA7_RLCT_ADDR                            (CONN_MCU_DMA_BASE + 0x0724)
#define CONN_MCU_DMA_DMA7_LIMITER_ADDR                         (CONN_MCU_DMA_BASE + 0x0728)
#define CONN_MCU_DMA_DMA7_PGMADDR_ADDR                         (CONN_MCU_DMA_BASE + 0x072C)
#define CONN_MCU_DMA_DMA8_WPPT_ADDR                            (CONN_MCU_DMA_BASE + 0x0808)
#define CONN_MCU_DMA_DMA8_WPTO_ADDR                            (CONN_MCU_DMA_BASE + 0x080C)
#define CONN_MCU_DMA_DMA8_COUNT_ADDR                           (CONN_MCU_DMA_BASE + 0x0810)
#define CONN_MCU_DMA_DMA8_CON_ADDR                             (CONN_MCU_DMA_BASE + 0x0814)
#define CONN_MCU_DMA_DMA8_START_ADDR                           (CONN_MCU_DMA_BASE + 0x0818)
#define CONN_MCU_DMA_DMA8_INTSTA_ADDR                          (CONN_MCU_DMA_BASE + 0x081C)
#define CONN_MCU_DMA_DMA8_ACKINT_ADDR                          (CONN_MCU_DMA_BASE + 0x0820)
#define CONN_MCU_DMA_DMA8_RLCT_ADDR                            (CONN_MCU_DMA_BASE + 0x0824)
#define CONN_MCU_DMA_DMA8_LIMITER_ADDR                         (CONN_MCU_DMA_BASE + 0x0828)
#define CONN_MCU_DMA_DMA8_PGMADDR_ADDR                         (CONN_MCU_DMA_BASE + 0x082C)
#define CONN_MCU_DMA_DMA9_WPPT_ADDR                            (CONN_MCU_DMA_BASE + 0x0908)
#define CONN_MCU_DMA_DMA9_WPTO_ADDR                            (CONN_MCU_DMA_BASE + 0x090C)
#define CONN_MCU_DMA_DMA9_COUNT_ADDR                           (CONN_MCU_DMA_BASE + 0x0910)
#define CONN_MCU_DMA_DMA9_CON_ADDR                             (CONN_MCU_DMA_BASE + 0x0914)
#define CONN_MCU_DMA_DMA9_START_ADDR                           (CONN_MCU_DMA_BASE + 0x0918)
#define CONN_MCU_DMA_DMA9_INTSTA_ADDR                          (CONN_MCU_DMA_BASE + 0x091C)
#define CONN_MCU_DMA_DMA9_ACKINT_ADDR                          (CONN_MCU_DMA_BASE + 0x0920)
#define CONN_MCU_DMA_DMA9_RLCT_ADDR                            (CONN_MCU_DMA_BASE + 0x0924)
#define CONN_MCU_DMA_DMA9_LIMITER_ADDR                         (CONN_MCU_DMA_BASE + 0x0928)
#define CONN_MCU_DMA_DMA9_PGMADDR_ADDR                         (CONN_MCU_DMA_BASE + 0x092C)
#define CONN_MCU_DMA_DMA10_WPPT_ADDR                           (CONN_MCU_DMA_BASE + 0x0A08)
#define CONN_MCU_DMA_DMA10_WPTO_ADDR                           (CONN_MCU_DMA_BASE + 0x0A0C)
#define CONN_MCU_DMA_DMA10_COUNT_ADDR                          (CONN_MCU_DMA_BASE + 0x0A10)
#define CONN_MCU_DMA_DMA10_CON_ADDR                            (CONN_MCU_DMA_BASE + 0x0A14)
#define CONN_MCU_DMA_DMA10_START_ADDR                          (CONN_MCU_DMA_BASE + 0x0A18)
#define CONN_MCU_DMA_DMA10_INTSTA_ADDR                         (CONN_MCU_DMA_BASE + 0x0A1C)
#define CONN_MCU_DMA_DMA10_ACKINT_ADDR                         (CONN_MCU_DMA_BASE + 0x0A20)
#define CONN_MCU_DMA_DMA10_RLCT_ADDR                           (CONN_MCU_DMA_BASE + 0x0A24)
#define CONN_MCU_DMA_DMA10_LIMITER_ADDR                        (CONN_MCU_DMA_BASE + 0x0A28)
#define CONN_MCU_DMA_DMA10_PGMADDR_ADDR                        (CONN_MCU_DMA_BASE + 0x0A2C)


#define CONN_MCU_DMA_DMA7_WPPT_WPPT_ADDR                       CONN_MCU_DMA_DMA7_WPPT_ADDR
#define CONN_MCU_DMA_DMA7_WPPT_WPPT_MASK                       0x000FFFFF
#define CONN_MCU_DMA_DMA7_WPPT_WPPT_SHFT                       0

#define CONN_MCU_DMA_DMA7_WPTO_WPTO_ADDR                       CONN_MCU_DMA_DMA7_WPTO_ADDR
#define CONN_MCU_DMA_DMA7_WPTO_WPTO_MASK                       0xFFFFFFFF
#define CONN_MCU_DMA_DMA7_WPTO_WPTO_SHFT                       0

#define CONN_MCU_DMA_DMA7_COUNT_LEN_ADDR                       CONN_MCU_DMA_DMA7_COUNT_ADDR
#define CONN_MCU_DMA_DMA7_COUNT_LEN_MASK                       0x000FFFFF
#define CONN_MCU_DMA_DMA7_COUNT_LEN_SHFT                       0

#define CONN_MCU_DMA_DMA7_CON_MAS_ADDR                         CONN_MCU_DMA_DMA7_CON_ADDR
#define CONN_MCU_DMA_DMA7_CON_MAS_MASK                         0x01F00000
#define CONN_MCU_DMA_DMA7_CON_MAS_SHFT                         20
#define CONN_MCU_DMA_DMA7_CON_W2B_ADDR                         CONN_MCU_DMA_DMA7_CON_ADDR
#define CONN_MCU_DMA_DMA7_CON_W2B_MASK                         0x00000040
#define CONN_MCU_DMA_DMA7_CON_W2B_SHFT                         6
#define CONN_MCU_DMA_DMA7_CON_B2W_ADDR                         CONN_MCU_DMA_DMA7_CON_ADDR
#define CONN_MCU_DMA_DMA7_CON_B2W_MASK                         0x00000020
#define CONN_MCU_DMA_DMA7_CON_B2W_SHFT                         5
#define CONN_MCU_DMA_DMA7_CON_SIZE_ADDR                        CONN_MCU_DMA_DMA7_CON_ADDR
#define CONN_MCU_DMA_DMA7_CON_SIZE_MASK                        0x00000003
#define CONN_MCU_DMA_DMA7_CON_SIZE_SHFT                        0

#define CONN_MCU_DMA_DMA7_START_STR_ADDR                       CONN_MCU_DMA_DMA7_START_ADDR
#define CONN_MCU_DMA_DMA7_START_STR_MASK                       0x00008000
#define CONN_MCU_DMA_DMA7_START_STR_SHFT                       15

#define CONN_MCU_DMA_DMA7_INTSTA_INT_ADDR                      CONN_MCU_DMA_DMA7_INTSTA_ADDR
#define CONN_MCU_DMA_DMA7_INTSTA_INT_MASK                      0x00008000
#define CONN_MCU_DMA_DMA7_INTSTA_INT_SHFT                      15

#define CONN_MCU_DMA_DMA7_ACKINT_ACK_ADDR                      CONN_MCU_DMA_DMA7_ACKINT_ADDR
#define CONN_MCU_DMA_DMA7_ACKINT_ACK_MASK                      0x00008000
#define CONN_MCU_DMA_DMA7_ACKINT_ACK_SHFT                      15

#define CONN_MCU_DMA_DMA7_RLCT_RLCT_ADDR                       CONN_MCU_DMA_DMA7_RLCT_ADDR
#define CONN_MCU_DMA_DMA7_RLCT_RLCT_MASK                       0x000FFFFF
#define CONN_MCU_DMA_DMA7_RLCT_RLCT_SHFT                       0

#define CONN_MCU_DMA_DMA7_PGMADDR_PGMADDR_ADDR                 CONN_MCU_DMA_DMA7_PGMADDR_ADDR
#define CONN_MCU_DMA_DMA7_PGMADDR_PGMADDR_MASK                 0xFFFFFFFF
#define CONN_MCU_DMA_DMA7_PGMADDR_PGMADDR_SHFT                 0

#define CONN_MCU_DMA_DMA8_WPPT_WPPT_ADDR                       CONN_MCU_DMA_DMA8_WPPT_ADDR
#define CONN_MCU_DMA_DMA8_WPPT_WPPT_MASK                       0x000FFFFF
#define CONN_MCU_DMA_DMA8_WPPT_WPPT_SHFT                       0

#define CONN_MCU_DMA_DMA8_WPTO_WPTO_ADDR                       CONN_MCU_DMA_DMA8_WPTO_ADDR
#define CONN_MCU_DMA_DMA8_WPTO_WPTO_MASK                       0xFFFFFFFF
#define CONN_MCU_DMA_DMA8_WPTO_WPTO_SHFT                       0

#define CONN_MCU_DMA_DMA8_COUNT_LEN_ADDR                       CONN_MCU_DMA_DMA8_COUNT_ADDR
#define CONN_MCU_DMA_DMA8_COUNT_LEN_MASK                       0x000FFFFF
#define CONN_MCU_DMA_DMA8_COUNT_LEN_SHFT                       0

#define CONN_MCU_DMA_DMA8_START_STR_ADDR                       CONN_MCU_DMA_DMA8_START_ADDR
#define CONN_MCU_DMA_DMA8_START_STR_MASK                       0x00008000
#define CONN_MCU_DMA_DMA8_START_STR_SHFT                       15

#define CONN_MCU_DMA_DMA8_INTSTA_INT_ADDR                      CONN_MCU_DMA_DMA8_INTSTA_ADDR
#define CONN_MCU_DMA_DMA8_INTSTA_INT_MASK                      0x00008000
#define CONN_MCU_DMA_DMA8_INTSTA_INT_SHFT                      15

#define CONN_MCU_DMA_DMA8_ACKINT_ACK_ADDR                      CONN_MCU_DMA_DMA8_ACKINT_ADDR
#define CONN_MCU_DMA_DMA8_ACKINT_ACK_MASK                      0x00008000
#define CONN_MCU_DMA_DMA8_ACKINT_ACK_SHFT                      15

#define CONN_MCU_DMA_DMA8_RLCT_RLCT_ADDR                       CONN_MCU_DMA_DMA8_RLCT_ADDR
#define CONN_MCU_DMA_DMA8_RLCT_RLCT_MASK                       0x000FFFFF
#define CONN_MCU_DMA_DMA8_RLCT_RLCT_SHFT                       0

#define CONN_MCU_DMA_DMA8_PGMADDR_PGMADDR_ADDR                 CONN_MCU_DMA_DMA8_PGMADDR_ADDR
#define CONN_MCU_DMA_DMA8_PGMADDR_PGMADDR_MASK                 0xFFFFFFFF
#define CONN_MCU_DMA_DMA8_PGMADDR_PGMADDR_SHFT                 0

#define CONN_MCU_DMA_DMA9_WPPT_WPPT_ADDR                       CONN_MCU_DMA_DMA9_WPPT_ADDR
#define CONN_MCU_DMA_DMA9_WPPT_WPPT_MASK                       0x000FFFFF
#define CONN_MCU_DMA_DMA9_WPPT_WPPT_SHFT                       0

#define CONN_MCU_DMA_DMA9_WPTO_WPTO_ADDR                       CONN_MCU_DMA_DMA9_WPTO_ADDR
#define CONN_MCU_DMA_DMA9_WPTO_WPTO_MASK                       0xFFFFFFFF
#define CONN_MCU_DMA_DMA9_WPTO_WPTO_SHFT                       0

#define CONN_MCU_DMA_DMA9_COUNT_LEN_ADDR                       CONN_MCU_DMA_DMA9_COUNT_ADDR
#define CONN_MCU_DMA_DMA9_COUNT_LEN_MASK                       0x000FFFFF
#define CONN_MCU_DMA_DMA9_COUNT_LEN_SHFT                       0

#define CONN_MCU_DMA_DMA9_START_STR_ADDR                       CONN_MCU_DMA_DMA9_START_ADDR
#define CONN_MCU_DMA_DMA9_START_STR_MASK                       0x00008000
#define CONN_MCU_DMA_DMA9_START_STR_SHFT                       15

#define CONN_MCU_DMA_DMA9_INTSTA_INT_ADDR                      CONN_MCU_DMA_DMA9_INTSTA_ADDR
#define CONN_MCU_DMA_DMA9_INTSTA_INT_MASK                      0x00008000
#define CONN_MCU_DMA_DMA9_INTSTA_INT_SHFT                      15

#define CONN_MCU_DMA_DMA9_ACKINT_ACK_ADDR                      CONN_MCU_DMA_DMA9_ACKINT_ADDR
#define CONN_MCU_DMA_DMA9_ACKINT_ACK_MASK                      0x00008000
#define CONN_MCU_DMA_DMA9_ACKINT_ACK_SHFT                      15

#define CONN_MCU_DMA_DMA9_RLCT_RLCT_ADDR                       CONN_MCU_DMA_DMA9_RLCT_ADDR
#define CONN_MCU_DMA_DMA9_RLCT_RLCT_MASK                       0x000FFFFF
#define CONN_MCU_DMA_DMA9_RLCT_RLCT_SHFT                       0

#define CONN_MCU_DMA_DMA9_PGMADDR_PGMADDR_ADDR                 CONN_MCU_DMA_DMA9_PGMADDR_ADDR
#define CONN_MCU_DMA_DMA9_PGMADDR_PGMADDR_MASK                 0xFFFFFFFF
#define CONN_MCU_DMA_DMA9_PGMADDR_PGMADDR_SHFT                 0

#define CONN_MCU_DMA_DMA10_WPPT_WPPT_ADDR                      CONN_MCU_DMA_DMA10_WPPT_ADDR
#define CONN_MCU_DMA_DMA10_WPPT_WPPT_MASK                      0x000FFFFF
#define CONN_MCU_DMA_DMA10_WPPT_WPPT_SHFT                      0

#define CONN_MCU_DMA_DMA10_WPTO_WPTO_ADDR                      CONN_MCU_DMA_DMA10_WPTO_ADDR
#define CONN_MCU_DMA_DMA10_WPTO_WPTO_MASK                      0xFFFFFFFF
#define CONN_MCU_DMA_DMA10_WPTO_WPTO_SHFT                      0

#define CONN_MCU_DMA_DMA10_COUNT_LEN_ADDR                      CONN_MCU_DMA_DMA10_COUNT_ADDR
#define CONN_MCU_DMA_DMA10_COUNT_LEN_MASK                      0x000FFFFF
#define CONN_MCU_DMA_DMA10_COUNT_LEN_SHFT                      0

#define CONN_MCU_DMA_DMA10_START_STR_ADDR                      CONN_MCU_DMA_DMA10_START_ADDR
#define CONN_MCU_DMA_DMA10_START_STR_MASK                      0x00008000
#define CONN_MCU_DMA_DMA10_START_STR_SHFT                      15

#define CONN_MCU_DMA_DMA10_INTSTA_INT_ADDR                     CONN_MCU_DMA_DMA10_INTSTA_ADDR
#define CONN_MCU_DMA_DMA10_INTSTA_INT_MASK                     0x00008000
#define CONN_MCU_DMA_DMA10_INTSTA_INT_SHFT                     15

#define CONN_MCU_DMA_DMA10_ACKINT_ACK_ADDR                     CONN_MCU_DMA_DMA10_ACKINT_ADDR
#define CONN_MCU_DMA_DMA10_ACKINT_ACK_MASK                     0x00008000
#define CONN_MCU_DMA_DMA10_ACKINT_ACK_SHFT                     15

#define CONN_MCU_DMA_DMA10_RLCT_RLCT_ADDR                      CONN_MCU_DMA_DMA10_RLCT_ADDR
#define CONN_MCU_DMA_DMA10_RLCT_RLCT_MASK                      0x000FFFFF
#define CONN_MCU_DMA_DMA10_RLCT_RLCT_SHFT                      0

#define CONN_MCU_DMA_DMA10_PGMADDR_PGMADDR_ADDR                CONN_MCU_DMA_DMA10_PGMADDR_ADDR
#define CONN_MCU_DMA_DMA10_PGMADDR_PGMADDR_MASK                0xFFFFFFFF
#define CONN_MCU_DMA_DMA10_PGMADDR_PGMADDR_SHFT                0

#endif /* __CONN_MCU_DMA_REGS_H__*/
