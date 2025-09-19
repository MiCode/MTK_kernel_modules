/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef __CONN_CFG_REGS_H__
#define __CONN_CFG_REGS_H__


#define CONN_CFG_BASE                                          0x18011000

#define CONN_CFG_IP_VERSION_ADDR                               (CONN_CFG_BASE + 0x000)
#define CONN_CFG_EMI_CTL_TOP_ADDR                              (CONN_CFG_BASE + 0x110)
#define CONN_CFG_EMI_CTL_WF_ADDR                               (CONN_CFG_BASE + 0x114)
#define CONN_CFG_EMI_CTL_BT_ADDR                               (CONN_CFG_BASE + 0x118)
#define CONN_CFG_EMI_CTL_GPS_ADDR                              (CONN_CFG_BASE + 0x11C)
#define CONN_CFG_EMI_CTL_GPS_L1_ADDR                           (CONN_CFG_BASE + 0X120)


#define CONN_CFG_IP_VERSION_IP_VERSION_ADDR                    CONN_CFG_IP_VERSION_ADDR
#define CONN_CFG_IP_VERSION_IP_VERSION_MASK                    0xFFFFFFFF
#define CONN_CFG_IP_VERSION_IP_VERSION_SHFT                    0

#define CONN_CFG_EMI_CTL_TOP_EMI_REQ_TOP_ADDR                  CONN_CFG_EMI_CTL_TOP_ADDR
#define CONN_CFG_EMI_CTL_TOP_EMI_REQ_TOP_MASK                  0x00000001
#define CONN_CFG_EMI_CTL_TOP_EMI_REQ_TOP_SHFT                  0

#define CONN_CFG_EMI_CTL_GPS_EMI_REQ_GPS_ADDR                  CONN_CFG_EMI_CTL_GPS_ADDR
#define CONN_CFG_EMI_CTL_GPS_EMI_REQ_GPS_MASK                  0x00000001
#define CONN_CFG_EMI_CTL_GPS_EMI_REQ_GPS_SHFT                  0

#define CONN_CFG_EMI_CTL_GPS_L1_INFRA_REQ_GPS_L1_ADDR          CONN_CFG_EMI_CTL_GPS_L1_ADDR
#define CONN_CFG_EMI_CTL_GPS_L1_INFRA_REQ_GPS_L1_MASK          0x00000020
#define CONN_CFG_EMI_CTL_GPS_L1_INFRA_REQ_GPS_L1_SHFT          5

#define CONN_CFG_EMI_CTL_GPS_SW_CONN_SRCCLKENA_GPS_ADDR        CONN_CFG_EMI_CTL_GPS_ADDR
#define CONN_CFG_EMI_CTL_GPS_SW_CONN_SRCCLKENA_GPS_MASK        0x00000002
#define CONN_CFG_EMI_CTL_GPS_SW_CONN_SRCCLKENA_GPS_SHFT        1

#endif /* __CONN_CFG_REGS_H__*/

