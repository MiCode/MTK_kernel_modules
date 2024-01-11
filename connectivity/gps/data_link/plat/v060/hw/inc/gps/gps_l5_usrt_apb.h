/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef __GPS_L5_USRT_APB_REGS_H__
#define __GPS_L5_USRT_APB_REGS_H__

#define GPS_L5_USRT_APB_BASE                                   0x80083000

#define GPS_L5_USRT_APB_GPS_APB_DATA_ADDR                      (GPS_L5_USRT_APB_BASE + 0x000)
#define GPS_L5_USRT_APB_APB_CTRL_ADDR                          (GPS_L5_USRT_APB_BASE + 0x004)
#define GPS_L5_USRT_APB_APB_INTEN_ADDR                         (GPS_L5_USRT_APB_BASE + 0x008)
#define GPS_L5_USRT_APB_APB_STA_ADDR                           (GPS_L5_USRT_APB_BASE + 0x00C)
#define GPS_L5_USRT_APB_MONF_ADDR                              (GPS_L5_USRT_APB_BASE + 0x010)
#define GPS_L5_USRT_APB_APB_DCODE_ADDR                         (GPS_L5_USRT_APB_BASE + 0x01c)
#define GPS_L5_USRT_APB_MCUB_A2DF_ADDR                         (GPS_L5_USRT_APB_BASE + 0x020)
#define GPS_L5_USRT_APB_MCUB_D2AF_ADDR                         (GPS_L5_USRT_APB_BASE + 0x024)
#define GPS_L5_USRT_APB_MCU_A2D0_ADDR                          (GPS_L5_USRT_APB_BASE + 0x030)
#define GPS_L5_USRT_APB_MCU_A2D1_ADDR                          (GPS_L5_USRT_APB_BASE + 0x034)
#define GPS_L5_USRT_APB_MCU_D2A0_ADDR                          (GPS_L5_USRT_APB_BASE + 0x050)
#define GPS_L5_USRT_APB_MCU_D2A1_ADDR                          (GPS_L5_USRT_APB_BASE + 0x054)


#define GPS_L5_USRT_APB_APB_CTRL_BYTEN_ADDR                    GPS_L5_USRT_APB_APB_CTRL_ADDR
#define GPS_L5_USRT_APB_APB_CTRL_BYTEN_MASK                    0x00000008
#define GPS_L5_USRT_APB_APB_CTRL_BYTEN_SHFT                    3
#define GPS_L5_USRT_APB_APB_CTRL_TX_EN_ADDR                    GPS_L5_USRT_APB_APB_CTRL_ADDR
#define GPS_L5_USRT_APB_APB_CTRL_TX_EN_MASK                    0x00000002
#define GPS_L5_USRT_APB_APB_CTRL_TX_EN_SHFT                    1
#define GPS_L5_USRT_APB_APB_CTRL_RX_EN_ADDR                    GPS_L5_USRT_APB_APB_CTRL_ADDR
#define GPS_L5_USRT_APB_APB_CTRL_RX_EN_MASK                    0x00000001
#define GPS_L5_USRT_APB_APB_CTRL_RX_EN_SHFT                    0

#define GPS_L5_USRT_APB_APB_INTEN_NODAIEN_ADDR                 GPS_L5_USRT_APB_APB_INTEN_ADDR
#define GPS_L5_USRT_APB_APB_INTEN_NODAIEN_MASK                 0x00000002
#define GPS_L5_USRT_APB_APB_INTEN_NODAIEN_SHFT                 1
#define GPS_L5_USRT_APB_APB_INTEN_TXIEN_ADDR                   GPS_L5_USRT_APB_APB_INTEN_ADDR
#define GPS_L5_USRT_APB_APB_INTEN_TXIEN_MASK                   0x00000001
#define GPS_L5_USRT_APB_APB_INTEN_TXIEN_SHFT                   0

#define GPS_L5_USRT_APB_APB_STA_RX_EMP_ADDR                    GPS_L5_USRT_APB_APB_STA_ADDR
#define GPS_L5_USRT_APB_APB_STA_RX_EMP_MASK                    0x20000000
#define GPS_L5_USRT_APB_APB_STA_RX_EMP_SHFT                    29
#define GPS_L5_USRT_APB_APB_STA_TX_IND_ADDR                    GPS_L5_USRT_APB_APB_STA_ADDR
#define GPS_L5_USRT_APB_APB_STA_TX_IND_MASK                    0x00000008
#define GPS_L5_USRT_APB_APB_STA_TX_IND_SHFT                    3
#define GPS_L5_USRT_APB_APB_STA_NODAINTB_ADDR                  GPS_L5_USRT_APB_APB_STA_ADDR
#define GPS_L5_USRT_APB_APB_STA_NODAINTB_MASK                  0x00000002
#define GPS_L5_USRT_APB_APB_STA_NODAINTB_SHFT                  1

#define GPS_L5_USRT_APB_MCUB_A2DF_A2DF3_ADDR                   GPS_L5_USRT_APB_MCUB_A2DF_ADDR
#define GPS_L5_USRT_APB_MCUB_A2DF_A2DF3_MASK                   0x00000008
#define GPS_L5_USRT_APB_MCUB_A2DF_A2DF3_SHFT                   3

#define GPS_L5_USRT_APB_MCUB_D2AF_D2AF3_ADDR                   GPS_L5_USRT_APB_MCUB_D2AF_ADDR
#define GPS_L5_USRT_APB_MCUB_D2AF_D2AF3_MASK                   0x00000008
#define GPS_L5_USRT_APB_MCUB_D2AF_D2AF3_SHFT                   3
#define GPS_L5_USRT_APB_MCUB_D2AF_D2AF0_ADDR                   GPS_L5_USRT_APB_MCUB_D2AF_ADDR
#define GPS_L5_USRT_APB_MCUB_D2AF_D2AF0_MASK                   0x00000001
#define GPS_L5_USRT_APB_MCUB_D2AF_D2AF0_SHFT                   0

#define GPS_L5_USRT_APB_MCU_A2D1_A2D_1_ADDR                    GPS_L5_USRT_APB_MCU_A2D1_ADDR
#define GPS_L5_USRT_APB_MCU_A2D1_A2D_1_MASK                    0x0000FFFF
#define GPS_L5_USRT_APB_MCU_A2D1_A2D_1_SHFT                    0

#define GPS_L5_USRT_APB_MCU_D2A0_D2A_0_ADDR                    GPS_L5_USRT_APB_MCU_D2A0_ADDR
#define GPS_L5_USRT_APB_MCU_D2A0_D2A_0_MASK                    0x0000FFFF
#define GPS_L5_USRT_APB_MCU_D2A0_D2A_0_SHFT                    0


#endif /* __GPS_L5_USRT_APB_REGS_H__*/
