/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

/*! \file   hal_dmashdl_mt7935.h
*    \brief  DMASHDL HAL API for mt7935
*
*    This file contains all routines which are exported
     from MediaTek 802.11 Wireless LAN driver stack to GLUE Layer.
*/

#ifndef _HAL_DMASHDL_MT7935_H
#define _HAL_DMASHDL_MT7935_H

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

/* === DMASHDL LITE SETTING === */
#define MT7935_DMASHDL_LMAC_QUEUE_NUM   DMASHDL_LITE_LMAC_QUEUE_MAX_NUM

#if defined(_HIF_PCIE) || defined(_HIF_AXI)
#define MT7935_DMASHDL_PLE_TOTAL_PAGE_SIZE             (0x7E0)
#define MT7935_DMASHDL_PSE_TOTAL_PAGE_SIZE             (0x10)
#define MT7935_DMASHDL_PKT_PLE_MAX_PAGE                (0x1)
#define MT7935_DMASHDL_PKT_PSE_MAX_PAGE                (0x8)

/* Group Control Enable (0-63) */
#define MT7935_DMASHDL_GROUP_0_CONTROL_EN              (1)
#define MT7935_DMASHDL_GROUP_1_CONTROL_EN              (1)
#define MT7935_DMASHDL_GROUP_2_CONTROL_EN              (1)
#define MT7935_DMASHDL_GROUP_3_CONTROL_EN              (1)
#define MT7935_DMASHDL_GROUP_4_CONTROL_EN              (1)
#define MT7935_DMASHDL_GROUP_5_CONTROL_EN              (1)
#define MT7935_DMASHDL_GROUP_6_CONTROL_EN              (0)
#define MT7935_DMASHDL_GROUP_7_CONTROL_EN              (0)
#define MT7935_DMASHDL_GROUP_8_CONTROL_EN              (0)
#define MT7935_DMASHDL_GROUP_9_CONTROL_EN              (0)
#define MT7935_DMASHDL_GROUP_10_CONTROL_EN             (0)
#define MT7935_DMASHDL_GROUP_11_CONTROL_EN             (0)
#define MT7935_DMASHDL_GROUP_12_CONTROL_EN             (0)
#define MT7935_DMASHDL_GROUP_13_CONTROL_EN             (0)
#define MT7935_DMASHDL_GROUP_14_CONTROL_EN             (0)
#define MT7935_DMASHDL_GROUP_15_CONTROL_EN             (1)
#define MT7935_DMASHDL_GROUP_16_63_CONTROL_EN          (0)

/* Max/Min Quota initial value (Quota can be changed at runtime) */
/* Group Max Quota (0-63) */
#define MT7935_DMASHDL_GROUP_0_MAX_QUOTA               (0x3F0)
#define MT7935_DMASHDL_GROUP_1_MAX_QUOTA               (0x3F0)
#define MT7935_DMASHDL_GROUP_2_MAX_QUOTA               (0x3F0)
#define MT7935_DMASHDL_GROUP_3_MAX_QUOTA               (0x3F0)
#define MT7935_DMASHDL_GROUP_4_MAX_QUOTA               (0x3F0)
#define MT7935_DMASHDL_GROUP_5_MAX_QUOTA               (0x3F0)
#define MT7935_DMASHDL_GROUP_6_MAX_QUOTA               (0x0)
#define MT7935_DMASHDL_GROUP_7_MAX_QUOTA               (0x0)
#define MT7935_DMASHDL_GROUP_8_MAX_QUOTA               (0x0)
#define MT7935_DMASHDL_GROUP_9_MAX_QUOTA               (0x0)
#define MT7935_DMASHDL_GROUP_10_MAX_QUOTA              (0x0)
#define MT7935_DMASHDL_GROUP_11_MAX_QUOTA              (0x0)
#define MT7935_DMASHDL_GROUP_12_MAX_QUOTA              (0x0)
#define MT7935_DMASHDL_GROUP_13_MAX_QUOTA              (0x0)
#define MT7935_DMASHDL_GROUP_14_MAX_QUOTA              (0x0)
#define MT7935_DMASHDL_GROUP_15_MAX_QUOTA              (0x30)
#define MT7935_DMASHDL_GROUP_16_63_MAX_QUOTA           (0x0)
/* Group Min Quota (0-63) */
#define MT7935_DMASHDL_GROUP_0_MIN_QUOTA               (0x10)
#define MT7935_DMASHDL_GROUP_1_MIN_QUOTA               (0x10)
#define MT7935_DMASHDL_GROUP_2_MIN_QUOTA               (0x10)
#define MT7935_DMASHDL_GROUP_3_MIN_QUOTA               (0x10)
#define MT7935_DMASHDL_GROUP_4_MIN_QUOTA               (0x10)
#define MT7935_DMASHDL_GROUP_5_MIN_QUOTA               (0x10)
#define MT7935_DMASHDL_GROUP_6_MIN_QUOTA               (0x0)
#define MT7935_DMASHDL_GROUP_7_MIN_QUOTA               (0x0)
#define MT7935_DMASHDL_GROUP_8_MIN_QUOTA               (0x0)
#define MT7935_DMASHDL_GROUP_9_MIN_QUOTA               (0x0)
#define MT7935_DMASHDL_GROUP_10_MIN_QUOTA              (0x0)
#define MT7935_DMASHDL_GROUP_11_MIN_QUOTA              (0x0)
#define MT7935_DMASHDL_GROUP_12_MIN_QUOTA              (0x0)
#define MT7935_DMASHDL_GROUP_13_MIN_QUOTA              (0x0)
#define MT7935_DMASHDL_GROUP_14_MIN_QUOTA              (0x0)
#define MT7935_DMASHDL_GROUP_15_MIN_QUOTA              (0x0)
#define MT7935_DMASHDL_GROUP_16_63_MIN_QUOTA           (0x0)

/* Mapping HW Queue to group (0-99) */
#define MT7935_DMASHDL_QUEUE_0_TO_GROUP                (0x0)   /* LMAC AC00 */
#define MT7935_DMASHDL_QUEUE_1_TO_GROUP                (0x0)   /* LMAC AC01 */
#define MT7935_DMASHDL_QUEUE_2_TO_GROUP                (0x0)   /* LMAC AC02 */
#define MT7935_DMASHDL_QUEUE_3_TO_GROUP                (0x4)   /* LMAC AC03 */
#define MT7935_DMASHDL_QUEUE_4_TO_GROUP                (0x1)   /* LMAC AC10 */
#define MT7935_DMASHDL_QUEUE_5_TO_GROUP                (0x1)   /* LMAC AC11 */
#define MT7935_DMASHDL_QUEUE_6_TO_GROUP                (0x1)   /* LMAC AC12 */
#define MT7935_DMASHDL_QUEUE_7_TO_GROUP                (0x4)   /* LMAC AC13 */
#define MT7935_DMASHDL_QUEUE_8_TO_GROUP                (0x2)   /* LMAC AC20 */
#define MT7935_DMASHDL_QUEUE_9_TO_GROUP                (0x2)   /* LMAC AC21 */
#define MT7935_DMASHDL_QUEUE_10_TO_GROUP               (0x2)   /* LMAC AC22 */
#define MT7935_DMASHDL_QUEUE_11_TO_GROUP               (0x4)   /* LMAC AC23 */
#define MT7935_DMASHDL_QUEUE_12_TO_GROUP               (0x3)   /* LMAC AC30 */
#define MT7935_DMASHDL_QUEUE_13_TO_GROUP               (0x3)   /* LMAC AC31 */
#define MT7935_DMASHDL_QUEUE_14_TO_GROUP               (0x3)   /* LMAC AC32 */
#define MT7935_DMASHDL_QUEUE_15_TO_GROUP               (0x4)   /* LMAC AC33 */
#define MT7935_DMASHDL_QUEUE_16_TO_GROUP               (0x5)   /* ALTX */
#define MT7935_DMASHDL_QUEUE_17_TO_GROUP               (0x0)   /* BMC */
#define MT7935_DMASHDL_QUEUE_18_TO_GROUP               (0x0)   /* BCN */
#define MT7935_DMASHDL_QUEUE_19_TO_GROUP               (0x0)   /* HW Reserved */
#define MT7935_DMASHDL_QUEUE_20_TO_GROUP               (0x5)   /* TGID=1 ALTX */
#define MT7935_DMASHDL_QUEUE_21_TO_GROUP               (0x0)   /* TGID=1 BMC  */
#define MT7935_DMASHDL_QUEUE_22_TO_GROUP               (0x0)   /* TGID=1 BCN  */
#define MT7935_DMASHDL_QUEUE_23_TO_GROUP               (0x0)   /* HW Reserved */
#define MT7935_DMASHDL_QUEUE_24_TO_GROUP               (0x0)   /* NAF */
#define MT7935_DMASHDL_QUEUE_25_TO_GROUP               (0x0)   /* NBCN */
#define MT7935_DMASHDL_QUEUE_26_TO_GROUP               (0x0)   /* FIXFID */
#define MT7935_DMASHDL_QUEUE_27_TO_GROUP               (0x0)   /* Reserved */
#define MT7935_DMASHDL_QUEUE_28_TO_GROUP               (0x5)   /* TGID=2 ALTX */
#define MT7935_DMASHDL_QUEUE_29_TO_GROUP               (0x0)   /* TGID=2 BMC  */
#define MT7935_DMASHDL_QUEUE_30_TO_GROUP               (0x0)   /* TGID=2 BCN  */
#define MT7935_DMASHDL_QUEUE_31_TO_GROUP               (0x0)   /* HW Reserved */
#define MT7935_DMASHDL_QUEUE_32_35_TO_GROUP            (0x0)   /* HW Reserved */
#define MT7935_DMASHDL_QUEUE_36_99_TO_GROUP            (0x0)   /* WLAN_ID Dec */


#elif defined(_HIF_USB)
#define MT7935_DMASHDL_PLE_TOTAL_PAGE_SIZE             (0x7E0)
#define MT7935_DMASHDL_PSE_TOTAL_PAGE_SIZE             (0x10)
#define MT7935_DMASHDL_PKT_PLE_MAX_PAGE                (0x1)
#define MT7935_DMASHDL_PKT_PSE_MAX_PAGE                (0x8)

/* Group Control Enable (0-63) */
#define MT7935_DMASHDL_GROUP_0_CONTROL_EN              (1)
#define MT7935_DMASHDL_GROUP_1_CONTROL_EN              (1)
#define MT7935_DMASHDL_GROUP_2_CONTROL_EN              (1)
#define MT7935_DMASHDL_GROUP_3_CONTROL_EN              (1)
#define MT7935_DMASHDL_GROUP_4_CONTROL_EN              (1)
#define MT7935_DMASHDL_GROUP_5_CONTROL_EN              (0)
#define MT7935_DMASHDL_GROUP_6_CONTROL_EN              (0)
#define MT7935_DMASHDL_GROUP_7_CONTROL_EN              (0)
#define MT7935_DMASHDL_GROUP_8_CONTROL_EN              (0)
#define MT7935_DMASHDL_GROUP_9_CONTROL_EN              (0)
#define MT7935_DMASHDL_GROUP_10_CONTROL_EN             (0)
#define MT7935_DMASHDL_GROUP_11_CONTROL_EN             (0)
#define MT7935_DMASHDL_GROUP_12_CONTROL_EN             (0)
#define MT7935_DMASHDL_GROUP_13_CONTROL_EN             (0)
#define MT7935_DMASHDL_GROUP_14_CONTROL_EN             (0)
#define MT7935_DMASHDL_GROUP_15_CONTROL_EN             (1)
#define MT7935_DMASHDL_GROUP_16_63_CONTROL_EN          (0)

/* Max/Min Quota initial value (Quota can be changed at runtime) */
/* Group Max Quota (0-63) */
#define MT7935_DMASHDL_GROUP_0_MAX_QUOTA               (0xFFF)
#define MT7935_DMASHDL_GROUP_1_MAX_QUOTA               (0xFFF)
#define MT7935_DMASHDL_GROUP_2_MAX_QUOTA               (0xFFF)
#define MT7935_DMASHDL_GROUP_3_MAX_QUOTA               (0xFFF)
#define MT7935_DMASHDL_GROUP_4_MAX_QUOTA               (0xFFF)
#define MT7935_DMASHDL_GROUP_5_MAX_QUOTA               (0x0)
#define MT7935_DMASHDL_GROUP_6_MAX_QUOTA               (0x0)
#define MT7935_DMASHDL_GROUP_7_MAX_QUOTA               (0x0)
#define MT7935_DMASHDL_GROUP_8_MAX_QUOTA               (0x0)
#define MT7935_DMASHDL_GROUP_9_MAX_QUOTA               (0x0)
#define MT7935_DMASHDL_GROUP_10_MAX_QUOTA              (0x0)
#define MT7935_DMASHDL_GROUP_11_MAX_QUOTA              (0x0)
#define MT7935_DMASHDL_GROUP_12_MAX_QUOTA              (0x0)
#define MT7935_DMASHDL_GROUP_13_MAX_QUOTA              (0x0)
#define MT7935_DMASHDL_GROUP_14_MAX_QUOTA              (0x0)
#define MT7935_DMASHDL_GROUP_15_MAX_QUOTA              (0x40)
#define MT7935_DMASHDL_GROUP_16_63_MAX_QUOTA           (0x0)
/* Group Min Quota (0-63) */
#define MT7935_DMASHDL_GROUP_0_MIN_QUOTA               (0x3)
#define MT7935_DMASHDL_GROUP_1_MIN_QUOTA               (0x3)
#define MT7935_DMASHDL_GROUP_2_MIN_QUOTA               (0x3)
#define MT7935_DMASHDL_GROUP_3_MIN_QUOTA               (0x3)
#define MT7935_DMASHDL_GROUP_4_MIN_QUOTA               (0x3)
#define MT7935_DMASHDL_GROUP_5_MIN_QUOTA               (0x0)
#define MT7935_DMASHDL_GROUP_6_MIN_QUOTA               (0x0)
#define MT7935_DMASHDL_GROUP_7_MIN_QUOTA               (0x0)
#define MT7935_DMASHDL_GROUP_8_MIN_QUOTA               (0x0)
#define MT7935_DMASHDL_GROUP_9_MIN_QUOTA               (0x0)
#define MT7935_DMASHDL_GROUP_10_MIN_QUOTA              (0x0)
#define MT7935_DMASHDL_GROUP_11_MIN_QUOTA              (0x0)
#define MT7935_DMASHDL_GROUP_12_MIN_QUOTA              (0x0)
#define MT7935_DMASHDL_GROUP_13_MIN_QUOTA              (0x0)
#define MT7935_DMASHDL_GROUP_14_MIN_QUOTA              (0x0)
#define MT7935_DMASHDL_GROUP_15_MIN_QUOTA              (0x40)
#define MT7935_DMASHDL_GROUP_16_63_MIN_QUOTA           (0x0)

/* Mapping HW Queue to group (0-99) */
#define MT7935_DMASHDL_QUEUE_0_TO_GROUP                (0x0)   /* LMAC AC00 */
#define MT7935_DMASHDL_QUEUE_1_TO_GROUP                (0x1)   /* LMAC AC01 */
#define MT7935_DMASHDL_QUEUE_2_TO_GROUP                (0x2)   /* LMAC AC02 */
#define MT7935_DMASHDL_QUEUE_3_TO_GROUP                (0x3)   /* LMAC AC03 */
#define MT7935_DMASHDL_QUEUE_4_TO_GROUP                (0x4)   /* LMAC AC10 */
#define MT7935_DMASHDL_QUEUE_5_TO_GROUP                (0x4)   /* LMAC AC11 */
#define MT7935_DMASHDL_QUEUE_6_TO_GROUP                (0x4)   /* LMAC AC12 */
#define MT7935_DMASHDL_QUEUE_7_TO_GROUP                (0x4)   /* LMAC AC13 */
#define MT7935_DMASHDL_QUEUE_8_TO_GROUP                (0x4)   /* LMAC AC20 */
#define MT7935_DMASHDL_QUEUE_9_TO_GROUP                (0x4)   /* LMAC AC21 */
#define MT7935_DMASHDL_QUEUE_10_TO_GROUP               (0x4)   /* LMAC AC22 */
#define MT7935_DMASHDL_QUEUE_11_TO_GROUP               (0x4)   /* LMAC AC23 */
#define MT7935_DMASHDL_QUEUE_12_TO_GROUP               (0x4)   /* LMAC AC30 */
#define MT7935_DMASHDL_QUEUE_13_TO_GROUP               (0x4)   /* LMAC AC31 */
#define MT7935_DMASHDL_QUEUE_14_TO_GROUP               (0x4)   /* LMAC AC32 */
#define MT7935_DMASHDL_QUEUE_15_TO_GROUP               (0x4)   /* LMAC AC33 */
#define MT7935_DMASHDL_QUEUE_16_TO_GROUP               (0x3)   /* ALTX */
#define MT7935_DMASHDL_QUEUE_17_TO_GROUP               (0x0)   /* BMC */
#define MT7935_DMASHDL_QUEUE_18_TO_GROUP               (0x0)   /* BCN */
#define MT7935_DMASHDL_QUEUE_19_TO_GROUP               (0x1)   /* HW Reserved */
#define MT7935_DMASHDL_QUEUE_20_TO_GROUP               (0x3)   /* TGID=1 ALTX */
#define MT7935_DMASHDL_QUEUE_21_TO_GROUP               (0x1)   /* TGID=1 BMC  */
#define MT7935_DMASHDL_QUEUE_22_TO_GROUP               (0x1)   /* TGID=1 BCN  */
#define MT7935_DMASHDL_QUEUE_23_TO_GROUP               (0x1)   /* HW Reserved */
#define MT7935_DMASHDL_QUEUE_24_TO_GROUP               (0x0)   /* NAF */
#define MT7935_DMASHDL_QUEUE_25_TO_GROUP               (0x0)   /* NBCN */
#define MT7935_DMASHDL_QUEUE_26_TO_GROUP               (0x0)   /* FIXFID */
#define MT7935_DMASHDL_QUEUE_27_TO_GROUP               (0x1)   /* Reserved */
#define MT7935_DMASHDL_QUEUE_28_TO_GROUP               (0x1)   /* TGID=2 ALTX */
#define MT7935_DMASHDL_QUEUE_29_TO_GROUP               (0x1)   /* TGID=2 BMC  */
#define MT7935_DMASHDL_QUEUE_30_TO_GROUP               (0x1)   /* TGID=2 BCN  */
#define MT7935_DMASHDL_QUEUE_31_TO_GROUP               (0x1)   /* HW Reserved */
#define MT7935_DMASHDL_QUEUE_32_35_TO_GROUP            (0x0)   /* HW Reserved */
#define MT7935_DMASHDL_QUEUE_36_99_TO_GROUP            (0x0)   /* WLAN_ID Dec */

#endif /* defined(_HIF_USB)*/

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
extern struct DMASHDL_CFG rMt7935DmashdlCfg;

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

void mt7935DmashdlInit(struct ADAPTER *prAdapter);

#endif /* _HAL_DMASHDL_MT7935_H */
