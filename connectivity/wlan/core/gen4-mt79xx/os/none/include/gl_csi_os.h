/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
/*
 * Id: @(#) gl_csi_os.h@@
 */

/*! \file   gl_csi_os.h
 *    \brief  List the external reference to OS for csi GLUE Layer.
 *
 *    In this file we define the data structure - _GL_CSI_INFO_T to store
 *    those objects
 *    we acquired from OS - e.g. TIMER, SPINLOCK, NET DEVICE ... . And all the
 *    external reference (header file, extern func() ..) to OS for GLUE Layer
 *    should also list down here.
 */

#ifndef _GL_CSI_OS_H
#define _GL_CSI_OS_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   V A R I A B L E
 *******************************************************************************
 */

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

struct _GL_CSI_INFO_T {
	struct net_device *prDevHandler;
};

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */
u_int8_t csiFreeInfo(struct GLUE_INFO *prGlueInfo);

u_int8_t csiLaunch(struct GLUE_INFO *prGlueInfo);

u_int8_t csiRemove(struct GLUE_INFO *prGlueInfo);

u_int8_t glRegisterCSI(struct GLUE_INFO *prGlueInfo,
	const char *prDevName);

u_int8_t glUnregisterCSI(struct GLUE_INFO *prGlueInfo);

int glSetupCSI(struct GLUE_INFO *prGlueInfo,
	struct wireless_dev *prCsiWdev,
	struct net_device *prCsiDev);

u_int8_t csiNetRegister(struct GLUE_INFO *prGlueInfo);

u_int8_t csiNetUnregister(struct GLUE_INFO *prGlueInfo);

uint32_t csiInitAtReset(struct GLUE_INFO *prGlueInfo,
	struct net_device *prNetDev);

u_int8_t glCsiCreateWirelessDevice(struct GLUE_INFO *prGlueInfo);

#endif

