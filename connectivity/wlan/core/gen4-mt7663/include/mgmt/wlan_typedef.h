/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*
 * Id: //Department/DaVinci/BRANCHES/MT6620_WIFI_DRIVER_V2_3
 *     /include/mgmt/wlan_typedef.h#1
 */

/*! \file   wlan_typedef.h
 *    \brief  Declaration of data type and return values of internal protocol
 *    stack.
 *
 *    In this file we declare the data type and return values which will be
 *    exported to all MGMT Protocol Stack.
 */


#ifndef _WLAN_TYPEDEF_H
#define _WLAN_TYPEDEF_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */
/* Type definition for BSS_INFO structure, to describe the
 * attributes used in a common BSS.
 */
struct BSS_INFO;	/* declare BSS_INFO_T */
struct BSS_INFO;	/* declare P2P_DEV_INFO_T */


struct AIS_SPECIFIC_BSS_INFO;	/* declare AIS_SPECIFIC_BSS_INFO_T */
struct P2P_SPECIFIC_BSS_INFO;	/* declare P2P_SPECIFIC_BSS_INFO_T */
struct BOW_SPECIFIC_BSS_INFO;	/* declare BOW_SPECIFIC_BSS_INFO_T */
/* CFG_SUPPORT_WFD */
struct WFD_CFG_SETTINGS;	/* declare WFD_CFG_SETTINGS_T */

/* BSS related structures */
/* Type definition for BSS_DESC structure, to describe parameter
 * sets of a particular BSS
 */
struct BSS_DESC;	/* declare BSS_DESC */

#if CFG_SUPPORT_ROAMING_SKIP_ONE_AP
struct ROAM_BSS_DESC;	/* declare ROAM_BSS_DESC_T */
#endif

#if CFG_SUPPORT_PASSPOINT
struct HS20_INFO;	/* declare HS20_INFO_T */
#endif /* CFG_SUPPORT_PASSPOINT */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
#endif /* _WLAN_TYPEDEF_H */
