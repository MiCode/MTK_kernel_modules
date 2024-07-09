/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*
** Id: //Department/DaVinci/BRANCHES/MT6620_WIFI_DRIVER_V2_3/os/version.h#1
*/

/*! \file   "version.h"
*    \brief  Driver's version definition
*
*/


#ifndef _VERSION_H
#define _VERSION_H
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

#ifndef NIC_AUTHOR
#define NIC_AUTHOR      "NIC_AUTHOR"
#endif
#ifndef NIC_DESC
#define NIC_DESC        "NIC_DESC"
#endif

#ifndef NIC_NAME
#define NIC_NAME            "MT6632"
#define NIC_DEVICE_ID       "MT6632"
#define NIC_DEVICE_ID_LOW   "mt6632"
#endif

/* NIC driver information */
#define NIC_VENDOR                      "MediaTek Inc."
#define NIC_VENDOR_OUI                  {0x00, 0x0C, 0xE7}

#define NIC_PRODUCT_NAME                "MediaTek Inc. Wireless LAN Adapter"
#define NIC_DRIVER_NAME                 "MediaTek Inc. Wireless LAN Adapter Driver"

/* Define our driver version */
#define NIC_DRIVER_MAJOR_VERSION        2
#define NIC_DRIVER_MINOR_VERSION        3
#define NIC_DRIVER_SERIAL_VERSION       1

#define STR(s)                          #s
#define XSTR(x)                         STR(x)
#define NDV(v)                          XSTR(NIC_DRIVER_##v##_VERSION)
#define NDV_STR(a, i, s)                NDV(a) "." NDV(i) "." NDV(s)
#define NIC_DRIVER_VERSION_STRING       NDV_STR(MAJOR, MINOR, SERIAL)

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

#endif /* _VERSION_H */
