/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   hal_dmashdl_mt7999.h
*    \brief  DMASHDL HAL API for mt7999
*
*    This file contains all routines which are exported
     from MediaTek 802.11 Wireless LAN driver stack to GLUE Layer.
*/

#ifndef _HAL_DMASHDL_MT7999_H
#define _HAL_DMASHDL_MT7999_H

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/
/* align MT7663 setting first */

#if defined(_HIF_PCIE) || defined(_HIF_AXI)

#define MT7999_DMASHDL_LMAC_QUEUE_NUM                  (32)
#define MT7999_DMASHDL_SLOT_ARBITER_EN                 (0)
#define MT7999_DMASHDL_PLE_TOTAL_PAGE_SIZE             (0x960)
#define MT7999_DMASHDL_PSE_TOTAL_PAGE_SIZE             (0x20)
#define MT7999_DMASHDL_PKT_PLE_MAX_PAGE                (0x1)
#define MT7999_DMASHDL_PKT_PSE_MAX_PAGE                (0x8)
#define MT7999_DMASHDL_GROUP_0_REFILL_EN               (1)
#define MT7999_DMASHDL_GROUP_1_REFILL_EN               (1)
#define MT7999_DMASHDL_GROUP_2_REFILL_EN               (1)
#define MT7999_DMASHDL_GROUP_3_REFILL_EN               (1)
#define MT7999_DMASHDL_GROUP_4_REFILL_EN               (1)
#define MT7999_DMASHDL_GROUP_5_REFILL_EN               (1)
#define MT7999_DMASHDL_GROUP_6_REFILL_EN               (0)
#define MT7999_DMASHDL_GROUP_7_REFILL_EN               (0)
#define MT7999_DMASHDL_GROUP_8_REFILL_EN               (0)
#define MT7999_DMASHDL_GROUP_9_REFILL_EN               (0)
#define MT7999_DMASHDL_GROUP_10_REFILL_EN              (0)
#define MT7999_DMASHDL_GROUP_11_REFILL_EN              (0)
#define MT7999_DMASHDL_GROUP_12_REFILL_EN              (0)
#define MT7999_DMASHDL_GROUP_13_REFILL_EN              (0)
#define MT7999_DMASHDL_GROUP_14_REFILL_EN              (0)
#define MT7999_DMASHDL_GROUP_15_REFILL_EN              (1)
#define MT7999_DMASHDL_GROUP_0_MAX_QUOTA               (0x300)
#define MT7999_DMASHDL_GROUP_1_MAX_QUOTA               (0x300)
#define MT7999_DMASHDL_GROUP_2_MAX_QUOTA               (0x300)
#define MT7999_DMASHDL_GROUP_3_MAX_QUOTA               (0x300)
#define MT7999_DMASHDL_GROUP_4_MAX_QUOTA               (0x300)
#define MT7999_DMASHDL_GROUP_5_MAX_QUOTA               (0x300)
#define MT7999_DMASHDL_GROUP_6_MAX_QUOTA               (0x0)
#define MT7999_DMASHDL_GROUP_7_MAX_QUOTA               (0x0)
#define MT7999_DMASHDL_GROUP_8_MAX_QUOTA               (0x0)
#define MT7999_DMASHDL_GROUP_9_MAX_QUOTA               (0x0)
#define MT7999_DMASHDL_GROUP_10_MAX_QUOTA              (0x0)
#define MT7999_DMASHDL_GROUP_11_MAX_QUOTA              (0x0)
#define MT7999_DMASHDL_GROUP_12_MAX_QUOTA              (0x0)
#define MT7999_DMASHDL_GROUP_13_MAX_QUOTA              (0x0)
#define MT7999_DMASHDL_GROUP_14_MAX_QUOTA              (0x0)
#define MT7999_DMASHDL_GROUP_15_MAX_QUOTA              (0x30)
#define MT7999_DMASHDL_GROUP_0_MIN_QUOTA               (0x10)
#define MT7999_DMASHDL_GROUP_1_MIN_QUOTA               (0x10)
#define MT7999_DMASHDL_GROUP_2_MIN_QUOTA               (0x10)
#define MT7999_DMASHDL_GROUP_3_MIN_QUOTA               (0x10)
#define MT7999_DMASHDL_GROUP_4_MIN_QUOTA               (0x10)
#define MT7999_DMASHDL_GROUP_5_MIN_QUOTA               (0x10)
#define MT7999_DMASHDL_GROUP_6_MIN_QUOTA               (0x0)
#define MT7999_DMASHDL_GROUP_7_MIN_QUOTA               (0x0)
#define MT7999_DMASHDL_GROUP_8_MIN_QUOTA               (0x0)
#define MT7999_DMASHDL_GROUP_9_MIN_QUOTA               (0x0)
#define MT7999_DMASHDL_GROUP_10_MIN_QUOTA              (0x0)
#define MT7999_DMASHDL_GROUP_11_MIN_QUOTA              (0x0)
#define MT7999_DMASHDL_GROUP_12_MIN_QUOTA              (0x0)
#define MT7999_DMASHDL_GROUP_13_MIN_QUOTA              (0x0)
#define MT7999_DMASHDL_GROUP_14_MIN_QUOTA              (0x0)
#define MT7999_DMASHDL_GROUP_15_MIN_QUOTA              (0x0)
#define MT7999_DMASHDL_QUEUE_0_TO_GROUP                (0x0)   /* LMAC AC00 */
#define MT7999_DMASHDL_QUEUE_1_TO_GROUP                (0x0)   /* LMAC AC01 */
#define MT7999_DMASHDL_QUEUE_2_TO_GROUP                (0x0)   /* LMAC AC02 */
#define MT7999_DMASHDL_QUEUE_3_TO_GROUP                (0x4)   /* LMAC AC03 */
#define MT7999_DMASHDL_QUEUE_4_TO_GROUP                (0x1)   /* LMAC AC10 */
#define MT7999_DMASHDL_QUEUE_5_TO_GROUP                (0x1)   /* LMAC AC11 */
#define MT7999_DMASHDL_QUEUE_6_TO_GROUP                (0x1)   /* LMAC AC12 */
#define MT7999_DMASHDL_QUEUE_7_TO_GROUP                (0x4)   /* LMAC AC13 */
#define MT7999_DMASHDL_QUEUE_8_TO_GROUP                (0x2)   /* LMAC AC20 */
#define MT7999_DMASHDL_QUEUE_9_TO_GROUP                (0x2)   /* LMAC AC21 */
#define MT7999_DMASHDL_QUEUE_10_TO_GROUP               (0x2)   /* LMAC AC22 */
#define MT7999_DMASHDL_QUEUE_11_TO_GROUP               (0x4)   /* LMAC AC23 */
#define MT7999_DMASHDL_QUEUE_12_TO_GROUP               (0x3)   /* LMAC AC30 */
#define MT7999_DMASHDL_QUEUE_13_TO_GROUP               (0x3)   /* LMAC AC31 */
#define MT7999_DMASHDL_QUEUE_14_TO_GROUP               (0x3)   /* LMAC AC32 */
#define MT7999_DMASHDL_QUEUE_15_TO_GROUP               (0x4)   /* LMAC AC33 */
#define MT7999_DMASHDL_QUEUE_16_TO_GROUP               (0x5)   /* ALTX */
#define MT7999_DMASHDL_QUEUE_17_TO_GROUP               (0x0)   /* BMC */
#define MT7999_DMASHDL_QUEUE_18_TO_GROUP               (0x0)   /* BCN */
#define MT7999_DMASHDL_QUEUE_19_TO_GROUP               (0x0)   /* HW Reserved */
#define MT7999_DMASHDL_QUEUE_20_TO_GROUP               (0x3)   /* TGID=1 ALTX */
#define MT7999_DMASHDL_QUEUE_21_TO_GROUP               (0x0)   /* TGID=1 BMC  */
#define MT7999_DMASHDL_QUEUE_22_TO_GROUP               (0x0)   /* TGID=1 BCN  */
#define MT7999_DMASHDL_QUEUE_23_TO_GROUP               (0x0)   /* HW Reserved */
#define MT7999_DMASHDL_QUEUE_24_TO_GROUP               (0x0)   /* NAF */
#define MT7999_DMASHDL_QUEUE_25_TO_GROUP               (0x0)   /* NBCN */
#define MT7999_DMASHDL_QUEUE_26_TO_GROUP               (0x0)   /* FIXFID */
#define MT7999_DMASHDL_QUEUE_27_TO_GROUP               (0x0)   /* Reserved */
#define MT7999_DMASHDL_QUEUE_28_TO_GROUP               (0x3)   /* TGID=2 ALTX */
#define MT7999_DMASHDL_QUEUE_29_TO_GROUP               (0x0)   /* TGID=2 BMC  */
#define MT7999_DMASHDL_QUEUE_30_TO_GROUP               (0x0)   /* TGID=2 BCN  */
#define MT7999_DMASHDL_QUEUE_31_TO_GROUP               (0x0)   /* HW Reserved */
#define MT7999_DMASHDL_PRIORITY0_GROUP                 (0x0)
#define MT7999_DMASHDL_PRIORITY1_GROUP                 (0x1)
#define MT7999_DMASHDL_PRIORITY2_GROUP                 (0x2)
#define MT7999_DMASHDL_PRIORITY3_GROUP                 (0x3)
#define MT7999_DMASHDL_PRIORITY4_GROUP                 (0x4)
#define MT7999_DMASHDL_PRIORITY5_GROUP                 (0x5)
#define MT7999_DMASHDL_PRIORITY6_GROUP                 (0x6)
#define MT7999_DMASHDL_PRIORITY7_GROUP                 (0x7)
#define MT7999_DMASHDL_PRIORITY8_GROUP                 (0x8)
#define MT7999_DMASHDL_PRIORITY9_GROUP                 (0x9)
#define MT7999_DMASHDL_PRIORITY10_GROUP                (0xA)
#define MT7999_DMASHDL_PRIORITY11_GROUP                (0xB)
#define MT7999_DMASHDL_PRIORITY12_GROUP                (0xC)
#define MT7999_DMASHDL_PRIORITY13_GROUP                (0xD)
#define MT7999_DMASHDL_PRIORITY14_GROUP                (0xE)
#define MT7999_DMASHDL_PRIORITY15_GROUP                (0xF)
/* 5 rings are used (4 data + cmd) */
#define MT7999_DMASHDL_HIF_ACK_CNT_TH                  (0x5)
/* Ring 0/1/2/3/15 are used */
#define MT7999_DMASHDL_HIF_GUP_ACT_MAP                 (0x800F)
#endif /* defined(_HIF_PCIE) || defined(_HIF_AXI) */

enum ENUM_CONCURRENT_TYPE {
	CONCURRENT_6G_6G_AND_2G = 0,
	CONCURRENT_2G_6G_AND_2G,
	CONCURRENT_5G_5G_AND_2G,
	CONCURRENT_2G_5G_AND_2G,
	CONCURRENT_6G_6G_AND_5G,
	CONCURRENT_5G_6G_AND_5G,
	CONCURRENT_SAME_BAND,
	CONCURRENT_TYPE_NUM
};
/*******************************************************************************
*                         D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/
extern struct DMASHDL_CFG rMt7999DmashdlCfg;

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

void mt7999DmashdlInit(struct ADAPTER *prAdapter);

#endif /* _HAL_DMASHDL_MT7999_H */
