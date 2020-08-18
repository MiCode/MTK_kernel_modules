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
#ifndef __GPS_USRT_APB_REGS_H__
#define __GPS_USRT_APB_REGS_H__

#define GPS_USRT_APB_BASE                                      0x80073000

#define GPS_USRT_APB_GPS_APB_DATA_ADDR                         (GPS_USRT_APB_BASE + 0x0000)
#define GPS_USRT_APB_APB_CTRL_ADDR                             (GPS_USRT_APB_BASE + 0x0004)
#define GPS_USRT_APB_APB_INTEN_ADDR                            (GPS_USRT_APB_BASE + 0x0008)
#define GPS_USRT_APB_APB_STA_ADDR                              (GPS_USRT_APB_BASE + 0x000C)
#define GPS_USRT_APB_MONF_ADDR                                 (GPS_USRT_APB_BASE + 0x0010)
#define GPS_USRT_APB_APB_DCODE_ADDR                            (GPS_USRT_APB_BASE + 0x001C)
#define GPS_USRT_APB_MCUB_A2DF_ADDR                            (GPS_USRT_APB_BASE + 0x0020)
#define GPS_USRT_APB_MCUB_D2AF_ADDR                            (GPS_USRT_APB_BASE + 0x0024)
#define GPS_USRT_APB_MCU_A2D0_ADDR                             (GPS_USRT_APB_BASE + 0x0030)
#define GPS_USRT_APB_MCU_A2D1_ADDR                             (GPS_USRT_APB_BASE + 0x0034)
#define GPS_USRT_APB_MCU_D2A0_ADDR                             (GPS_USRT_APB_BASE + 0x0050)
#define GPS_USRT_APB_MCU_D2A1_ADDR                             (GPS_USRT_APB_BASE + 0x0054)


#define GPS_USRT_APB_GPS_APB_DATA_APB_DATA_ADDR                GPS_USRT_APB_GPS_APB_DATA_ADDR
#define GPS_USRT_APB_GPS_APB_DATA_APB_DATA_MASK                0xFFFFFFFF
#define GPS_USRT_APB_GPS_APB_DATA_APB_DATA_SHFT                0

#define GPS_USRT_APB_APB_CTRL_APB_VER_ADDR                     GPS_USRT_APB_APB_CTRL_ADDR
#define GPS_USRT_APB_APB_CTRL_APB_VER_MASK                     0xFFFF0000
#define GPS_USRT_APB_APB_CTRL_APB_VER_SHFT                     16
#define GPS_USRT_APB_APB_CTRL_RST_ADDR                         GPS_USRT_APB_APB_CTRL_ADDR
#define GPS_USRT_APB_APB_CTRL_RST_MASK                         0x00008000
#define GPS_USRT_APB_APB_CTRL_RST_SHFT                         15
#define GPS_USRT_APB_APB_CTRL_CG_RB_ADDR                       GPS_USRT_APB_APB_CTRL_ADDR
#define GPS_USRT_APB_APB_CTRL_CG_RB_MASK                       0x00001000
#define GPS_USRT_APB_APB_CTRL_CG_RB_SHFT                       12
#define GPS_USRT_APB_APB_CTRL_RD_ST_ADDR                       GPS_USRT_APB_APB_CTRL_ADDR
#define GPS_USRT_APB_APB_CTRL_RD_ST_MASK                       0x00000E00
#define GPS_USRT_APB_APB_CTRL_RD_ST_SHFT                       9
#define GPS_USRT_APB_APB_CTRL_USEM_ADDR                        GPS_USRT_APB_APB_CTRL_ADDR
#define GPS_USRT_APB_APB_CTRL_USEM_MASK                        0x00000100
#define GPS_USRT_APB_APB_CTRL_USEM_SHFT                        8
#define GPS_USRT_APB_APB_CTRL_TXACK_EXT_ADDR                   GPS_USRT_APB_APB_CTRL_ADDR
#define GPS_USRT_APB_APB_CTRL_TXACK_EXT_MASK                   0x00000020
#define GPS_USRT_APB_APB_CTRL_TXACK_EXT_SHFT                   5
#define GPS_USRT_APB_APB_CTRL_RXACK_EXT_ADDR                   GPS_USRT_APB_APB_CTRL_ADDR
#define GPS_USRT_APB_APB_CTRL_RXACK_EXT_MASK                   0x00000010
#define GPS_USRT_APB_APB_CTRL_RXACK_EXT_SHFT                   4
#define GPS_USRT_APB_APB_CTRL_BYTEN_ADDR                       GPS_USRT_APB_APB_CTRL_ADDR
#define GPS_USRT_APB_APB_CTRL_BYTEN_MASK                       0x00000008
#define GPS_USRT_APB_APB_CTRL_BYTEN_SHFT                       3
#define GPS_USRT_APB_APB_CTRL_TX_EN_ADDR                       GPS_USRT_APB_APB_CTRL_ADDR
#define GPS_USRT_APB_APB_CTRL_TX_EN_MASK                       0x00000002
#define GPS_USRT_APB_APB_CTRL_TX_EN_SHFT                       1
#define GPS_USRT_APB_APB_CTRL_RX_EN_ADDR                       GPS_USRT_APB_APB_CTRL_ADDR
#define GPS_USRT_APB_APB_CTRL_RX_EN_MASK                       0x00000001
#define GPS_USRT_APB_APB_CTRL_RX_EN_SHFT                       0

#define GPS_USRT_APB_APB_INTEN_EINT_EN_ADDR                    GPS_USRT_APB_APB_INTEN_ADDR
#define GPS_USRT_APB_APB_INTEN_EINT_EN_MASK                    0x00000080
#define GPS_USRT_APB_APB_INTEN_EINT_EN_SHFT                    7
#define GPS_USRT_APB_APB_INTEN_DINT_EN_ADDR                    GPS_USRT_APB_APB_INTEN_ADDR
#define GPS_USRT_APB_APB_INTEN_DINT_EN_MASK                    0x00000040
#define GPS_USRT_APB_APB_INTEN_DINT_EN_SHFT                    6
#define GPS_USRT_APB_APB_INTEN_INT_MODE_ADDR                   GPS_USRT_APB_APB_INTEN_ADDR
#define GPS_USRT_APB_APB_INTEN_INT_MODE_MASK                   0x00000010
#define GPS_USRT_APB_APB_INTEN_INT_MODE_SHFT                   4
#define GPS_USRT_APB_APB_INTEN_NODAIEN_ADDR                    GPS_USRT_APB_APB_INTEN_ADDR
#define GPS_USRT_APB_APB_INTEN_NODAIEN_MASK                    0x00000002
#define GPS_USRT_APB_APB_INTEN_NODAIEN_SHFT                    1
#define GPS_USRT_APB_APB_INTEN_TXIEN_ADDR                      GPS_USRT_APB_APB_INTEN_ADDR
#define GPS_USRT_APB_APB_INTEN_TXIEN_MASK                      0x00000001
#define GPS_USRT_APB_APB_INTEN_TXIEN_SHFT                      0

#define GPS_USRT_APB_APB_STA_RX_UNDR_ADDR                      GPS_USRT_APB_APB_STA_ADDR
#define GPS_USRT_APB_APB_STA_RX_UNDR_MASK                      0x80000000
#define GPS_USRT_APB_APB_STA_RX_UNDR_SHFT                      31
#define GPS_USRT_APB_APB_STA_RX_OVF_ADDR                       GPS_USRT_APB_APB_STA_ADDR
#define GPS_USRT_APB_APB_STA_RX_OVF_MASK                       0x40000000
#define GPS_USRT_APB_APB_STA_RX_OVF_SHFT                       30
#define GPS_USRT_APB_APB_STA_RX_EMP_ADDR                       GPS_USRT_APB_APB_STA_ADDR
#define GPS_USRT_APB_APB_STA_RX_EMP_MASK                       0x20000000
#define GPS_USRT_APB_APB_STA_RX_EMP_SHFT                       29
#define GPS_USRT_APB_APB_STA_RX_FULL_ADDR                      GPS_USRT_APB_APB_STA_ADDR
#define GPS_USRT_APB_APB_STA_RX_FULL_MASK                      0x10000000
#define GPS_USRT_APB_APB_STA_RX_FULL_SHFT                      28
#define GPS_USRT_APB_APB_STA_TX_UNDR_ADDR                      GPS_USRT_APB_APB_STA_ADDR
#define GPS_USRT_APB_APB_STA_TX_UNDR_MASK                      0x08000000
#define GPS_USRT_APB_APB_STA_TX_UNDR_SHFT                      27
#define GPS_USRT_APB_APB_STA_TX_OVF_ADDR                       GPS_USRT_APB_APB_STA_ADDR
#define GPS_USRT_APB_APB_STA_TX_OVF_MASK                       0x04000000
#define GPS_USRT_APB_APB_STA_TX_OVF_SHFT                       26
#define GPS_USRT_APB_APB_STA_TX_EMP_ADDR                       GPS_USRT_APB_APB_STA_ADDR
#define GPS_USRT_APB_APB_STA_TX_EMP_MASK                       0x02000000
#define GPS_USRT_APB_APB_STA_TX_EMP_SHFT                       25
#define GPS_USRT_APB_APB_STA_TX_FULL_ADDR                      GPS_USRT_APB_APB_STA_ADDR
#define GPS_USRT_APB_APB_STA_TX_FULL_MASK                      0x01000000
#define GPS_USRT_APB_APB_STA_TX_FULL_SHFT                      24
#define GPS_USRT_APB_APB_STA_TX_ST_ADDR                        GPS_USRT_APB_APB_STA_ADDR
#define GPS_USRT_APB_APB_STA_TX_ST_MASK                        0x00700000
#define GPS_USRT_APB_APB_STA_TX_ST_SHFT                        20
#define GPS_USRT_APB_APB_STA_RX_ST_ADDR                        GPS_USRT_APB_APB_STA_ADDR
#define GPS_USRT_APB_APB_STA_RX_ST_MASK                        0x00070000
#define GPS_USRT_APB_APB_STA_RX_ST_SHFT                        16
#define GPS_USRT_APB_APB_STA_REGE_ADDR                         GPS_USRT_APB_APB_STA_ADDR
#define GPS_USRT_APB_APB_STA_REGE_MASK                         0x00000020
#define GPS_USRT_APB_APB_STA_REGE_SHFT                         5
#define GPS_USRT_APB_APB_STA_URAME_ADDR                        GPS_USRT_APB_APB_STA_ADDR
#define GPS_USRT_APB_APB_STA_URAME_MASK                        0x00000010
#define GPS_USRT_APB_APB_STA_URAME_SHFT                        4
#define GPS_USRT_APB_APB_STA_TX_IND_ADDR                       GPS_USRT_APB_APB_STA_ADDR
#define GPS_USRT_APB_APB_STA_TX_IND_MASK                       0x00000008
#define GPS_USRT_APB_APB_STA_TX_IND_SHFT                       3
#define GPS_USRT_APB_APB_STA_DSP_WK_INT_ADDR                   GPS_USRT_APB_APB_STA_ADDR
#define GPS_USRT_APB_APB_STA_DSP_WK_INT_MASK                   0x00000004
#define GPS_USRT_APB_APB_STA_DSP_WK_INT_SHFT                   2
#define GPS_USRT_APB_APB_STA_NODAINTB_ADDR                     GPS_USRT_APB_APB_STA_ADDR
#define GPS_USRT_APB_APB_STA_NODAINTB_MASK                     0x00000002
#define GPS_USRT_APB_APB_STA_NODAINTB_SHFT                     1
#define GPS_USRT_APB_APB_STA_TXINTB_ADDR                       GPS_USRT_APB_APB_STA_ADDR
#define GPS_USRT_APB_APB_STA_TXINTB_MASK                       0x00000001
#define GPS_USRT_APB_APB_STA_TXINTB_SHFT                       0

#define GPS_USRT_APB_MONF_MONF_ADDR                            GPS_USRT_APB_MONF_ADDR
#define GPS_USRT_APB_MONF_MONF_MASK                            0xFFFFFFFF
#define GPS_USRT_APB_MONF_MONF_SHFT                            0

#define GPS_USRT_APB_APB_DCODE_APB_Data_code_ADDR              GPS_USRT_APB_APB_DCODE_ADDR
#define GPS_USRT_APB_APB_DCODE_APB_Data_code_MASK              0xFFFFFFFF
#define GPS_USRT_APB_APB_DCODE_APB_Data_code_SHFT              0

#define GPS_USRT_APB_MCUB_A2DF_A2DF3_ADDR                      GPS_USRT_APB_MCUB_A2DF_ADDR
#define GPS_USRT_APB_MCUB_A2DF_A2DF3_MASK                      0x00000008
#define GPS_USRT_APB_MCUB_A2DF_A2DF3_SHFT                      3
#define GPS_USRT_APB_MCUB_A2DF_A2DF2_ADDR                      GPS_USRT_APB_MCUB_A2DF_ADDR
#define GPS_USRT_APB_MCUB_A2DF_A2DF2_MASK                      0x00000004
#define GPS_USRT_APB_MCUB_A2DF_A2DF2_SHFT                      2
#define GPS_USRT_APB_MCUB_A2DF_A2DF1_ADDR                      GPS_USRT_APB_MCUB_A2DF_ADDR
#define GPS_USRT_APB_MCUB_A2DF_A2DF1_MASK                      0x00000002
#define GPS_USRT_APB_MCUB_A2DF_A2DF1_SHFT                      1
#define GPS_USRT_APB_MCUB_A2DF_A2DF0_ADDR                      GPS_USRT_APB_MCUB_A2DF_ADDR
#define GPS_USRT_APB_MCUB_A2DF_A2DF0_MASK                      0x00000001
#define GPS_USRT_APB_MCUB_A2DF_A2DF0_SHFT                      0

#define GPS_USRT_APB_MCUB_D2AF_D2AF3_ADDR                      GPS_USRT_APB_MCUB_D2AF_ADDR
#define GPS_USRT_APB_MCUB_D2AF_D2AF3_MASK                      0x00000008
#define GPS_USRT_APB_MCUB_D2AF_D2AF3_SHFT                      3
#define GPS_USRT_APB_MCUB_D2AF_D2AF2_ADDR                      GPS_USRT_APB_MCUB_D2AF_ADDR
#define GPS_USRT_APB_MCUB_D2AF_D2AF2_MASK                      0x00000004
#define GPS_USRT_APB_MCUB_D2AF_D2AF2_SHFT                      2
#define GPS_USRT_APB_MCUB_D2AF_D2AF1_ADDR                      GPS_USRT_APB_MCUB_D2AF_ADDR
#define GPS_USRT_APB_MCUB_D2AF_D2AF1_MASK                      0x00000002
#define GPS_USRT_APB_MCUB_D2AF_D2AF1_SHFT                      1
#define GPS_USRT_APB_MCUB_D2AF_D2AF0_ADDR                      GPS_USRT_APB_MCUB_D2AF_ADDR
#define GPS_USRT_APB_MCUB_D2AF_D2AF0_MASK                      0x00000001
#define GPS_USRT_APB_MCUB_D2AF_D2AF0_SHFT                      0

#define GPS_USRT_APB_MCU_A2D0_A2D_0_ADDR                       GPS_USRT_APB_MCU_A2D0_ADDR
#define GPS_USRT_APB_MCU_A2D0_A2D_0_MASK                       0x0000FFFF
#define GPS_USRT_APB_MCU_A2D0_A2D_0_SHFT                       0

#define GPS_USRT_APB_MCU_A2D1_A2D_1_ADDR                       GPS_USRT_APB_MCU_A2D1_ADDR
#define GPS_USRT_APB_MCU_A2D1_A2D_1_MASK                       0x0000FFFF
#define GPS_USRT_APB_MCU_A2D1_A2D_1_SHFT                       0

#define GPS_USRT_APB_MCU_D2A0_D2A_0_ADDR                       GPS_USRT_APB_MCU_D2A0_ADDR
#define GPS_USRT_APB_MCU_D2A0_D2A_0_MASK                       0x0000FFFF
#define GPS_USRT_APB_MCU_D2A0_D2A_0_SHFT                       0

#define GPS_USRT_APB_MCU_D2A1_D2A_1_ADDR                       GPS_USRT_APB_MCU_D2A1_ADDR
#define GPS_USRT_APB_MCU_D2A1_D2A_1_MASK                       0x0000FFFF
#define GPS_USRT_APB_MCU_D2A1_D2A_1_SHFT                       0

#endif /* __GPS_USRT_APB_REGS_H__ */

