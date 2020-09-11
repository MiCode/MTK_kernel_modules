/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#ifndef __CONN_UART_PTA_REGS_H__
#define __CONN_UART_PTA_REGS_H__

#define CONN_UART_PTA_BASE                                     0x1800D000

#define CONN_UART_PTA_RBR_ADDR                                 (CONN_UART_PTA_BASE + 0x0000)
#define CONN_UART_PTA_THR_ADDR                                 (CONN_UART_PTA_BASE + 0x0000)
#define CONN_UART_PTA_IER_ADDR                                 (CONN_UART_PTA_BASE + 0x0004)
#define CONN_UART_PTA_IIR_ADDR                                 (CONN_UART_PTA_BASE + 0x0008)
#define CONN_UART_PTA_FCR_ADDR                                 (CONN_UART_PTA_BASE + 0x0008)
#define CONN_UART_PTA_LCR_ADDR                                 (CONN_UART_PTA_BASE + 0x000C)
#define CONN_UART_PTA_MCR_ADDR                                 (CONN_UART_PTA_BASE + 0x0010)
#define CONN_UART_PTA_LSR_ADDR                                 (CONN_UART_PTA_BASE + 0x0014)
#define CONN_UART_PTA_MSR_ADDR                                 (CONN_UART_PTA_BASE + 0x0018)
#define CONN_UART_PTA_SCR_ADDR                                 (CONN_UART_PTA_BASE + 0x001C)
#define CONN_UART_PTA_DLL_ADDR                                 (CONN_UART_PTA_BASE + 0x0000)
#define CONN_UART_PTA_DLM_ADDR                                 (CONN_UART_PTA_BASE + 0x0004)
#define CONN_UART_PTA_EFR_ADDR                                 (CONN_UART_PTA_BASE + 0x0008)
#define CONN_UART_PTA_XON1_ADDR                                (CONN_UART_PTA_BASE + 0x0010)
#define CONN_UART_PTA_XON2_ADDR                                (CONN_UART_PTA_BASE + 0x0014)
#define CONN_UART_PTA_XOFF1_ADDR                               (CONN_UART_PTA_BASE + 0x0018)
#define CONN_UART_PTA_XOFF2_ADDR                               (CONN_UART_PTA_BASE + 0x001C)
#define CONN_UART_PTA_HIGHSPEED_ADDR                           (CONN_UART_PTA_BASE + 0x0024)
#define CONN_UART_PTA_SAMPLE_COUNT_ADDR                        (CONN_UART_PTA_BASE + 0x0028)
#define CONN_UART_PTA_SAMPLE_POINT_ADDR                        (CONN_UART_PTA_BASE + 0x002C)
#define CONN_UART_PTA_RATE_FIX_ADDR                            (CONN_UART_PTA_BASE + 0x0034)
#define CONN_UART_PTA_GUARD_ADDR                               (CONN_UART_PTA_BASE + 0x003C)
#define CONN_UART_PTA_ESCAPE_DAT_ADDR                          (CONN_UART_PTA_BASE + 0x0040)
#define CONN_UART_PTA_ESCAPE_EN_ADDR                           (CONN_UART_PTA_BASE + 0x0044)
#define CONN_UART_PTA_SLEEP_EN_ADDR                            (CONN_UART_PTA_BASE + 0x0048)
#define CONN_UART_PTA_VFIFO_EN_ADDR                            (CONN_UART_PTA_BASE + 0x004C)
#define CONN_UART_PTA_RXTRIG_ADDR                              (CONN_UART_PTA_BASE + 0x0050)
#define CONN_UART_PTA_FRACDIV_L_ADDR                           (CONN_UART_PTA_BASE + 0x0054)
#define CONN_UART_PTA_FRACDIV_M_ADDR                           (CONN_UART_PTA_BASE + 0x0058)
#define CONN_UART_PTA_FCR_RD_ADDR                              (CONN_UART_PTA_BASE + 0x005C)
#define CONN_UART_PTA_TX_ACTIVE_EN_ADDR                        (CONN_UART_PTA_BASE + 0x0060)
#define CONN_UART_PTA_RX_OFFSET_ADDR                           (CONN_UART_PTA_BASE + 0x0068)
#define CONN_UART_PTA_TX_OFFSET_ADDR                           (CONN_UART_PTA_BASE + 0x006C)


#define CONN_UART_PTA_RBR_RBR_ADDR                             CONN_UART_PTA_RBR_ADDR
#define CONN_UART_PTA_RBR_RBR_MASK                             0x000000FF
#define CONN_UART_PTA_RBR_RBR_SHFT                             0

#define CONN_UART_PTA_THR_THR_ADDR                             CONN_UART_PTA_THR_ADDR
#define CONN_UART_PTA_THR_THR_MASK                             0x000000FF
#define CONN_UART_PTA_THR_THR_SHFT                             0

#define CONN_UART_PTA_IER_CTSI_ADDR                            CONN_UART_PTA_IER_ADDR
#define CONN_UART_PTA_IER_CTSI_MASK                            0x00000080
#define CONN_UART_PTA_IER_CTSI_SHFT                            7
#define CONN_UART_PTA_IER_RTSI_ADDR                            CONN_UART_PTA_IER_ADDR
#define CONN_UART_PTA_IER_RTSI_MASK                            0x00000040
#define CONN_UART_PTA_IER_RTSI_SHFT                            6
#define CONN_UART_PTA_IER_XOFFI_ADDR                           CONN_UART_PTA_IER_ADDR
#define CONN_UART_PTA_IER_XOFFI_MASK                           0x00000020
#define CONN_UART_PTA_IER_XOFFI_SHFT                           5
#define CONN_UART_PTA_IER_EDSSI_ADDR                           CONN_UART_PTA_IER_ADDR
#define CONN_UART_PTA_IER_EDSSI_MASK                           0x00000008
#define CONN_UART_PTA_IER_EDSSI_SHFT                           3
#define CONN_UART_PTA_IER_ELSI_ADDR                            CONN_UART_PTA_IER_ADDR
#define CONN_UART_PTA_IER_ELSI_MASK                            0x00000004
#define CONN_UART_PTA_IER_ELSI_SHFT                            2
#define CONN_UART_PTA_IER_ETBEI_ADDR                           CONN_UART_PTA_IER_ADDR
#define CONN_UART_PTA_IER_ETBEI_MASK                           0x00000002
#define CONN_UART_PTA_IER_ETBEI_SHFT                           1
#define CONN_UART_PTA_IER_ERBFI_ADDR                           CONN_UART_PTA_IER_ADDR
#define CONN_UART_PTA_IER_ERBFI_MASK                           0x00000001
#define CONN_UART_PTA_IER_ERBFI_SHFT                           0

#define CONN_UART_PTA_IIR_FIFOE_ADDR                           CONN_UART_PTA_IIR_ADDR
#define CONN_UART_PTA_IIR_FIFOE_MASK                           0x000000C0
#define CONN_UART_PTA_IIR_FIFOE_SHFT                           6
#define CONN_UART_PTA_IIR_IIR_ID_ADDR                          CONN_UART_PTA_IIR_ADDR
#define CONN_UART_PTA_IIR_IIR_ID_MASK                          0x0000003F
#define CONN_UART_PTA_IIR_IIR_ID_SHFT                          0

#define CONN_UART_PTA_FCR_RFTL_ADDR                            CONN_UART_PTA_FCR_ADDR
#define CONN_UART_PTA_FCR_RFTL_MASK                            0x000000C0
#define CONN_UART_PTA_FCR_RFTL_SHFT                            6
#define CONN_UART_PTA_FCR_TFTL_ADDR                            CONN_UART_PTA_FCR_ADDR
#define CONN_UART_PTA_FCR_TFTL_MASK                            0x00000030
#define CONN_UART_PTA_FCR_TFTL_SHFT                            4
#define CONN_UART_PTA_FCR_DMA1_ADDR                            CONN_UART_PTA_FCR_ADDR
#define CONN_UART_PTA_FCR_DMA1_MASK                            0x00000008
#define CONN_UART_PTA_FCR_DMA1_SHFT                            3
#define CONN_UART_PTA_FCR_CLRT_ADDR                            CONN_UART_PTA_FCR_ADDR
#define CONN_UART_PTA_FCR_CLRT_MASK                            0x00000004
#define CONN_UART_PTA_FCR_CLRT_SHFT                            2
#define CONN_UART_PTA_FCR_CLRR_ADDR                            CONN_UART_PTA_FCR_ADDR
#define CONN_UART_PTA_FCR_CLRR_MASK                            0x00000002
#define CONN_UART_PTA_FCR_CLRR_SHFT                            1
#define CONN_UART_PTA_FCR_FIFOE_ADDR                           CONN_UART_PTA_FCR_ADDR
#define CONN_UART_PTA_FCR_FIFOE_MASK                           0x00000001
#define CONN_UART_PTA_FCR_FIFOE_SHFT                           0

#define CONN_UART_PTA_LCR_DLAB_ADDR                            CONN_UART_PTA_LCR_ADDR
#define CONN_UART_PTA_LCR_DLAB_MASK                            0x00000080
#define CONN_UART_PTA_LCR_DLAB_SHFT                            7
#define CONN_UART_PTA_LCR_SB_ADDR                              CONN_UART_PTA_LCR_ADDR
#define CONN_UART_PTA_LCR_SB_MASK                              0x00000040
#define CONN_UART_PTA_LCR_SB_SHFT                              6
#define CONN_UART_PTA_LCR_SP_ADDR                              CONN_UART_PTA_LCR_ADDR
#define CONN_UART_PTA_LCR_SP_MASK                              0x00000020
#define CONN_UART_PTA_LCR_SP_SHFT                              5
#define CONN_UART_PTA_LCR_EPS_ADDR                             CONN_UART_PTA_LCR_ADDR
#define CONN_UART_PTA_LCR_EPS_MASK                             0x00000010
#define CONN_UART_PTA_LCR_EPS_SHFT                             4
#define CONN_UART_PTA_LCR_PEN_ADDR                             CONN_UART_PTA_LCR_ADDR
#define CONN_UART_PTA_LCR_PEN_MASK                             0x00000008
#define CONN_UART_PTA_LCR_PEN_SHFT                             3
#define CONN_UART_PTA_LCR_STB_ADDR                             CONN_UART_PTA_LCR_ADDR
#define CONN_UART_PTA_LCR_STB_MASK                             0x00000004
#define CONN_UART_PTA_LCR_STB_SHFT                             2
#define CONN_UART_PTA_LCR_WLS_ADDR                             CONN_UART_PTA_LCR_ADDR
#define CONN_UART_PTA_LCR_WLS_MASK                             0x00000003
#define CONN_UART_PTA_LCR_WLS_SHFT                             0

#define CONN_UART_PTA_MCR_XOFF_STATUS_ADDR                     CONN_UART_PTA_MCR_ADDR
#define CONN_UART_PTA_MCR_XOFF_STATUS_MASK                     0x00000080
#define CONN_UART_PTA_MCR_XOFF_STATUS_SHFT                     7
#define CONN_UART_PTA_MCR_DCM_EN_ADDR                          CONN_UART_PTA_MCR_ADDR
#define CONN_UART_PTA_MCR_DCM_EN_MASK                          0x00000010
#define CONN_UART_PTA_MCR_DCM_EN_SHFT                          4
#define CONN_UART_PTA_MCR_OUT2_ADDR                            CONN_UART_PTA_MCR_ADDR
#define CONN_UART_PTA_MCR_OUT2_MASK                            0x00000008
#define CONN_UART_PTA_MCR_OUT2_SHFT                            3
#define CONN_UART_PTA_MCR_OUT1_ADDR                            CONN_UART_PTA_MCR_ADDR
#define CONN_UART_PTA_MCR_OUT1_MASK                            0x00000004
#define CONN_UART_PTA_MCR_OUT1_SHFT                            2
#define CONN_UART_PTA_MCR_RTS_ADDR                             CONN_UART_PTA_MCR_ADDR
#define CONN_UART_PTA_MCR_RTS_MASK                             0x00000002
#define CONN_UART_PTA_MCR_RTS_SHFT                             1
#define CONN_UART_PTA_MCR_DTR_ADDR                             CONN_UART_PTA_MCR_ADDR
#define CONN_UART_PTA_MCR_DTR_MASK                             0x00000001
#define CONN_UART_PTA_MCR_DTR_SHFT                             0

#define CONN_UART_PTA_LSR_FIFOERR_ADDR                         CONN_UART_PTA_LSR_ADDR
#define CONN_UART_PTA_LSR_FIFOERR_MASK                         0x00000080
#define CONN_UART_PTA_LSR_FIFOERR_SHFT                         7
#define CONN_UART_PTA_LSR_TEMT_ADDR                            CONN_UART_PTA_LSR_ADDR
#define CONN_UART_PTA_LSR_TEMT_MASK                            0x00000040
#define CONN_UART_PTA_LSR_TEMT_SHFT                            6
#define CONN_UART_PTA_LSR_THRE_ADDR                            CONN_UART_PTA_LSR_ADDR
#define CONN_UART_PTA_LSR_THRE_MASK                            0x00000020
#define CONN_UART_PTA_LSR_THRE_SHFT                            5
#define CONN_UART_PTA_LSR_BI_ADDR                              CONN_UART_PTA_LSR_ADDR
#define CONN_UART_PTA_LSR_BI_MASK                              0x00000010
#define CONN_UART_PTA_LSR_BI_SHFT                              4
#define CONN_UART_PTA_LSR_FE_ADDR                              CONN_UART_PTA_LSR_ADDR
#define CONN_UART_PTA_LSR_FE_MASK                              0x00000008
#define CONN_UART_PTA_LSR_FE_SHFT                              3
#define CONN_UART_PTA_LSR_PE_ADDR                              CONN_UART_PTA_LSR_ADDR
#define CONN_UART_PTA_LSR_PE_MASK                              0x00000004
#define CONN_UART_PTA_LSR_PE_SHFT                              2
#define CONN_UART_PTA_LSR_OE_ADDR                              CONN_UART_PTA_LSR_ADDR
#define CONN_UART_PTA_LSR_OE_MASK                              0x00000002
#define CONN_UART_PTA_LSR_OE_SHFT                              1
#define CONN_UART_PTA_LSR_DR_ADDR                              CONN_UART_PTA_LSR_ADDR
#define CONN_UART_PTA_LSR_DR_MASK                              0x00000001
#define CONN_UART_PTA_LSR_DR_SHFT                              0

#define CONN_UART_PTA_MSR_DCD_ADDR                             CONN_UART_PTA_MSR_ADDR
#define CONN_UART_PTA_MSR_DCD_MASK                             0x00000080
#define CONN_UART_PTA_MSR_DCD_SHFT                             7
#define CONN_UART_PTA_MSR_RI_ADDR                              CONN_UART_PTA_MSR_ADDR
#define CONN_UART_PTA_MSR_RI_MASK                              0x00000040
#define CONN_UART_PTA_MSR_RI_SHFT                              6
#define CONN_UART_PTA_MSR_DSR_ADDR                             CONN_UART_PTA_MSR_ADDR
#define CONN_UART_PTA_MSR_DSR_MASK                             0x00000020
#define CONN_UART_PTA_MSR_DSR_SHFT                             5
#define CONN_UART_PTA_MSR_CTS_ADDR                             CONN_UART_PTA_MSR_ADDR
#define CONN_UART_PTA_MSR_CTS_MASK                             0x00000010
#define CONN_UART_PTA_MSR_CTS_SHFT                             4
#define CONN_UART_PTA_MSR_DDCD_ADDR                            CONN_UART_PTA_MSR_ADDR
#define CONN_UART_PTA_MSR_DDCD_MASK                            0x00000008
#define CONN_UART_PTA_MSR_DDCD_SHFT                            3
#define CONN_UART_PTA_MSR_TERI_ADDR                            CONN_UART_PTA_MSR_ADDR
#define CONN_UART_PTA_MSR_TERI_MASK                            0x00000004
#define CONN_UART_PTA_MSR_TERI_SHFT                            2
#define CONN_UART_PTA_MSR_DDSR_ADDR                            CONN_UART_PTA_MSR_ADDR
#define CONN_UART_PTA_MSR_DDSR_MASK                            0x00000002
#define CONN_UART_PTA_MSR_DDSR_SHFT                            1
#define CONN_UART_PTA_MSR_DCTS_ADDR                            CONN_UART_PTA_MSR_ADDR
#define CONN_UART_PTA_MSR_DCTS_MASK                            0x00000001
#define CONN_UART_PTA_MSR_DCTS_SHFT                            0

#define CONN_UART_PTA_SCR_SCR_ADDR                             CONN_UART_PTA_SCR_ADDR
#define CONN_UART_PTA_SCR_SCR_MASK                             0x000000FF
#define CONN_UART_PTA_SCR_SCR_SHFT                             0

#define CONN_UART_PTA_DLL_DLL_ADDR                             CONN_UART_PTA_DLL_ADDR
#define CONN_UART_PTA_DLL_DLL_MASK                             0x000000FF
#define CONN_UART_PTA_DLL_DLL_SHFT                             0

#define CONN_UART_PTA_DLM_DLM_ADDR                             CONN_UART_PTA_DLM_ADDR
#define CONN_UART_PTA_DLM_DLM_MASK                             0x000000FF
#define CONN_UART_PTA_DLM_DLM_SHFT                             0

#define CONN_UART_PTA_EFR_AUTO_CTS_ADDR                        CONN_UART_PTA_EFR_ADDR
#define CONN_UART_PTA_EFR_AUTO_CTS_MASK                        0x00000080
#define CONN_UART_PTA_EFR_AUTO_CTS_SHFT                        7
#define CONN_UART_PTA_EFR_AUTO_RTS_ADDR                        CONN_UART_PTA_EFR_ADDR
#define CONN_UART_PTA_EFR_AUTO_RTS_MASK                        0x00000040
#define CONN_UART_PTA_EFR_AUTO_RTS_SHFT                        6
#define CONN_UART_PTA_EFR_ENABLE_E_ADDR                        CONN_UART_PTA_EFR_ADDR
#define CONN_UART_PTA_EFR_ENABLE_E_MASK                        0x00000010
#define CONN_UART_PTA_EFR_ENABLE_E_SHFT                        4
#define CONN_UART_PTA_EFR_SW_FLOW_CONT_ADDR                    CONN_UART_PTA_EFR_ADDR
#define CONN_UART_PTA_EFR_SW_FLOW_CONT_MASK                    0x0000000F
#define CONN_UART_PTA_EFR_SW_FLOW_CONT_SHFT                    0

#define CONN_UART_PTA_XON1_XON1_ADDR                           CONN_UART_PTA_XON1_ADDR
#define CONN_UART_PTA_XON1_XON1_MASK                           0x000000FF
#define CONN_UART_PTA_XON1_XON1_SHFT                           0

#define CONN_UART_PTA_XON2_XON2_ADDR                           CONN_UART_PTA_XON2_ADDR
#define CONN_UART_PTA_XON2_XON2_MASK                           0x000000FF
#define CONN_UART_PTA_XON2_XON2_SHFT                           0

#define CONN_UART_PTA_XOFF1_XOFF1_ADDR                         CONN_UART_PTA_XOFF1_ADDR
#define CONN_UART_PTA_XOFF1_XOFF1_MASK                         0x000000FF
#define CONN_UART_PTA_XOFF1_XOFF1_SHFT                         0

#define CONN_UART_PTA_XOFF2_XOFF2_ADDR                         CONN_UART_PTA_XOFF2_ADDR
#define CONN_UART_PTA_XOFF2_XOFF2_MASK                         0x000000FF
#define CONN_UART_PTA_XOFF2_XOFF2_SHFT                         0

#define CONN_UART_PTA_HIGHSPEED_SPEED_ADDR                     CONN_UART_PTA_HIGHSPEED_ADDR
#define CONN_UART_PTA_HIGHSPEED_SPEED_MASK                     0x00000003
#define CONN_UART_PTA_HIGHSPEED_SPEED_SHFT                     0

#define CONN_UART_PTA_SAMPLE_COUNT_SAMPLE_COUNT_ADDR           CONN_UART_PTA_SAMPLE_COUNT_ADDR
#define CONN_UART_PTA_SAMPLE_COUNT_SAMPLE_COUNT_MASK           0x000000FF
#define CONN_UART_PTA_SAMPLE_COUNT_SAMPLE_COUNT_SHFT           0

#define CONN_UART_PTA_SAMPLE_POINT_SAMPLE_POINT_ADDR           CONN_UART_PTA_SAMPLE_POINT_ADDR
#define CONN_UART_PTA_SAMPLE_POINT_SAMPLE_POINT_MASK           0x000000FF
#define CONN_UART_PTA_SAMPLE_POINT_SAMPLE_POINT_SHFT           0

#define CONN_UART_PTA_RATE_FIX_RATE_FIX_ADDR                   CONN_UART_PTA_RATE_FIX_ADDR
#define CONN_UART_PTA_RATE_FIX_RATE_FIX_MASK                   0x00000001
#define CONN_UART_PTA_RATE_FIX_RATE_FIX_SHFT                   0

#define CONN_UART_PTA_GUARD_GUARD_EN_ADDR                      CONN_UART_PTA_GUARD_ADDR
#define CONN_UART_PTA_GUARD_GUARD_EN_MASK                      0x00000010
#define CONN_UART_PTA_GUARD_GUARD_EN_SHFT                      4
#define CONN_UART_PTA_GUARD_GUARD_CNT_ADDR                     CONN_UART_PTA_GUARD_ADDR
#define CONN_UART_PTA_GUARD_GUARD_CNT_MASK                     0x0000000F
#define CONN_UART_PTA_GUARD_GUARD_CNT_SHFT                     0

#define CONN_UART_PTA_ESCAPE_DAT_ESCAPE_DAT_ADDR               CONN_UART_PTA_ESCAPE_DAT_ADDR
#define CONN_UART_PTA_ESCAPE_DAT_ESCAPE_DAT_MASK               0x000000FF
#define CONN_UART_PTA_ESCAPE_DAT_ESCAPE_DAT_SHFT               0

#define CONN_UART_PTA_ESCAPE_EN_ESC_EN_ADDR                    CONN_UART_PTA_ESCAPE_EN_ADDR
#define CONN_UART_PTA_ESCAPE_EN_ESC_EN_MASK                    0x00000001
#define CONN_UART_PTA_ESCAPE_EN_ESC_EN_SHFT                    0

#define CONN_UART_PTA_SLEEP_EN_SLEEP_EN_ADDR                   CONN_UART_PTA_SLEEP_EN_ADDR
#define CONN_UART_PTA_SLEEP_EN_SLEEP_EN_MASK                   0x00000001
#define CONN_UART_PTA_SLEEP_EN_SLEEP_EN_SHFT                   0

#define CONN_UART_PTA_VFIFO_EN_RX_TIME_EN_ADDR                 CONN_UART_PTA_VFIFO_EN_ADDR
#define CONN_UART_PTA_VFIFO_EN_RX_TIME_EN_MASK                 0x00000080
#define CONN_UART_PTA_VFIFO_EN_RX_TIME_EN_SHFT                 7
#define CONN_UART_PTA_VFIFO_EN_DBG_FLAG_SEL_ADDR               CONN_UART_PTA_VFIFO_EN_ADDR
#define CONN_UART_PTA_VFIFO_EN_DBG_FLAG_SEL_MASK               0x00000030
#define CONN_UART_PTA_VFIFO_EN_DBG_FLAG_SEL_SHFT               4
#define CONN_UART_PTA_VFIFO_EN_PTA_RX_FE_EN_ADDR               CONN_UART_PTA_VFIFO_EN_ADDR
#define CONN_UART_PTA_VFIFO_EN_PTA_RX_FE_EN_MASK               0x00000008
#define CONN_UART_PTA_VFIFO_EN_PTA_RX_FE_EN_SHFT               3
#define CONN_UART_PTA_VFIFO_EN_PTA_RX_MODE_ADDR                CONN_UART_PTA_VFIFO_EN_ADDR
#define CONN_UART_PTA_VFIFO_EN_PTA_RX_MODE_MASK                0x00000004
#define CONN_UART_PTA_VFIFO_EN_PTA_RX_MODE_SHFT                2
#define CONN_UART_PTA_VFIFO_EN_DMA_VFIFO_EN_ADDR               CONN_UART_PTA_VFIFO_EN_ADDR
#define CONN_UART_PTA_VFIFO_EN_DMA_VFIFO_EN_MASK               0x00000001
#define CONN_UART_PTA_VFIFO_EN_DMA_VFIFO_EN_SHFT               0

#define CONN_UART_PTA_RXTRIG_RXTRIG_ADDR                       CONN_UART_PTA_RXTRIG_ADDR
#define CONN_UART_PTA_RXTRIG_RXTRIG_MASK                       0x0000000F
#define CONN_UART_PTA_RXTRIG_RXTRIG_SHFT                       0

#define CONN_UART_PTA_FRACDIV_L_FRACDIV_L_ADDR                 CONN_UART_PTA_FRACDIV_L_ADDR
#define CONN_UART_PTA_FRACDIV_L_FRACDIV_L_MASK                 0x000000FF
#define CONN_UART_PTA_FRACDIV_L_FRACDIV_L_SHFT                 0

#define CONN_UART_PTA_FRACDIV_M_FRACDIV_M_ADDR                 CONN_UART_PTA_FRACDIV_M_ADDR
#define CONN_UART_PTA_FRACDIV_M_FRACDIV_M_MASK                 0x00000003
#define CONN_UART_PTA_FRACDIV_M_FRACDIV_M_SHFT                 0

#define CONN_UART_PTA_FCR_RD_RFTL_ADDR                         CONN_UART_PTA_FCR_RD_ADDR
#define CONN_UART_PTA_FCR_RD_RFTL_MASK                         0x000000C0
#define CONN_UART_PTA_FCR_RD_RFTL_SHFT                         6
#define CONN_UART_PTA_FCR_RD_TFTL_ADDR                         CONN_UART_PTA_FCR_RD_ADDR
#define CONN_UART_PTA_FCR_RD_TFTL_MASK                         0x00000030
#define CONN_UART_PTA_FCR_RD_TFTL_SHFT                         4
#define CONN_UART_PTA_FCR_RD_DMA1_ADDR                         CONN_UART_PTA_FCR_RD_ADDR
#define CONN_UART_PTA_FCR_RD_DMA1_MASK                         0x00000008
#define CONN_UART_PTA_FCR_RD_DMA1_SHFT                         3
#define CONN_UART_PTA_FCR_RD_FIFOE_ADDR                        CONN_UART_PTA_FCR_RD_ADDR
#define CONN_UART_PTA_FCR_RD_FIFOE_MASK                        0x00000001
#define CONN_UART_PTA_FCR_RD_FIFOE_SHFT                        0

#define CONN_UART_PTA_TX_ACTIVE_EN_TX_OE_EN_ADDR               CONN_UART_PTA_TX_ACTIVE_EN_ADDR
#define CONN_UART_PTA_TX_ACTIVE_EN_TX_OE_EN_MASK               0x00000002
#define CONN_UART_PTA_TX_ACTIVE_EN_TX_OE_EN_SHFT               1
#define CONN_UART_PTA_TX_ACTIVE_EN_TX_PU_EN_ADDR               CONN_UART_PTA_TX_ACTIVE_EN_ADDR
#define CONN_UART_PTA_TX_ACTIVE_EN_TX_PU_EN_MASK               0x00000001
#define CONN_UART_PTA_TX_ACTIVE_EN_TX_PU_EN_SHFT               0

#define CONN_UART_PTA_RX_OFFSET_RX_OFFSET_ADDR                 CONN_UART_PTA_RX_OFFSET_ADDR
#define CONN_UART_PTA_RX_OFFSET_RX_OFFSET_MASK                 0x0000003F
#define CONN_UART_PTA_RX_OFFSET_RX_OFFSET_SHFT                 0

#define CONN_UART_PTA_TX_OFFSET_TX_OFFSET_ADDR                 CONN_UART_PTA_TX_OFFSET_ADDR
#define CONN_UART_PTA_TX_OFFSET_TX_OFFSET_MASK                 0x0000001F
#define CONN_UART_PTA_TX_OFFSET_TX_OFFSET_SHFT                 0

#endif /* __CONN_UART_PTA_REGS_H__ */

