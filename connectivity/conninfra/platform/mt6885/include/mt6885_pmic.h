/*
 * Copyright (C) 2016 MediaTek Inc.
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
/*! \file
*    \brief  Declaration of library functions
*
*    Any definitions in this file will be shared among GLUE Layer and internal Driver Stack.
*/

#ifndef _PLATFORM_MT6885_PMIC_H_
#define _PLATFORM_MT6885_PMIC_H_

#include "osal.h"
#include "pmic_mng.h"
/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
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
/**********************************************************************/
/* PMIC mt6359P define for Quark project only*/
/**********************************************************************/
#ifndef MT6359_PMIC_REG_BASE

#define MT6359_PMIC_REG_BASE                 ((unsigned int)(0x0))

#define MT6359_BUCK_VS2_CON1                 (MT6359_PMIC_REG_BASE+0x188e)
#define MT6359_BUCK_VS2_VOTER_SET            (MT6359_PMIC_REG_BASE+0x18ac)
#define MT6359_BUCK_VS2_VOTER_CLR            (MT6359_PMIC_REG_BASE+0x18ae)
#define MT6359_BUCK_VS2_ELR0                 (MT6359_PMIC_REG_BASE+0x18b4)
#define MT6359_LDO_VCN33_1_CON0              (MT6359_PMIC_REG_BASE+0x1be2)
#define MT6359_LDO_VCN33_1_OP_EN_SET         (MT6359_PMIC_REG_BASE+0x1bea)
#define MT6359_LDO_VCN33_1_OP_CFG_SET        (MT6359_PMIC_REG_BASE+0x1bf0)
#define MT6359_LDO_VCN33_2_CON0              (MT6359_PMIC_REG_BASE+0x1c08)
#define MT6359_LDO_VCN33_2_OP_EN_SET         (MT6359_PMIC_REG_BASE+0x1c10)
#define MT6359_LDO_VCN33_2_OP_CFG_SET        (MT6359_PMIC_REG_BASE+0x1c16)
#define MT6359_LDO_VCN13_CON0                (MT6359_PMIC_REG_BASE+0x1c1c)
#define MT6359_LDO_VCN13_OP_EN_SET           (MT6359_PMIC_REG_BASE+0x1c24)
#define MT6359_LDO_VCN13_OP_CFG_SET          (MT6359_PMIC_REG_BASE+0x1c2a)
#define MT6359_LDO_VCN18_CON0                (MT6359_PMIC_REG_BASE+0x1c2e)
#define MT6359_LDO_VCN18_OP_EN_SET           (MT6359_PMIC_REG_BASE+0x1c36)
#define MT6359_LDO_VCN18_OP_CFG_SET          (MT6359_PMIC_REG_BASE+0x1c3c)
#define MT6359_VCN13_ANA_CON0                (MT6359_PMIC_REG_BASE+0x202e)

#define PMIC_RG_BUCK_VS2_VOTER_EN_SET_ADDR                  \
    MT6359_BUCK_VS2_VOTER_SET
#define PMIC_RG_BUCK_VS2_VOTER_EN_SET_MASK                  0xFFF
#define PMIC_RG_BUCK_VS2_VOTER_EN_SET_SHIFT                 0
#define PMIC_RG_BUCK_VS2_VOTER_EN_CLR_ADDR                  \
    MT6359_BUCK_VS2_VOTER_CLR
#define PMIC_RG_BUCK_VS2_VOTER_EN_CLR_MASK                  0xFFF
#define PMIC_RG_BUCK_VS2_VOTER_EN_CLR_SHIFT                 0
#define PMIC_RG_BUCK_VS2_VOSEL_ADDR                         \
    MT6359_BUCK_VS2_ELR0
#define PMIC_RG_BUCK_VS2_VOSEL_MASK                         0x7F
#define PMIC_RG_BUCK_VS2_VOSEL_SHIFT                        0
#define PMIC_RG_BUCK_VS2_VOSEL_SLEEP_ADDR                   \
    MT6359_BUCK_VS2_CON1
#define PMIC_RG_BUCK_VS2_VOSEL_SLEEP_MASK                   0x7F
#define PMIC_RG_BUCK_VS2_VOSEL_SLEEP_SHIFT                  0
#define PMIC_RG_LDO_VCN33_1_LP_ADDR                         \
    MT6359_LDO_VCN33_1_CON0
#define PMIC_RG_LDO_VCN33_1_LP_MASK                         0x1
#define PMIC_RG_LDO_VCN33_1_LP_SHIFT                        1
#define PMIC_RG_LDO_VCN33_1_OP_EN_SET_ADDR                  \
    MT6359_LDO_VCN33_1_OP_EN_SET
#define PMIC_RG_LDO_VCN33_1_OP_CFG_SET_ADDR                 \
    MT6359_LDO_VCN33_1_OP_CFG_SET
#define PMIC_RG_LDO_VCN33_2_LP_ADDR                         \
    MT6359_LDO_VCN33_2_CON0
#define PMIC_RG_LDO_VCN33_2_LP_MASK                         0x1
#define PMIC_RG_LDO_VCN33_2_LP_SHIFT                        1
#define PMIC_RG_LDO_VCN33_2_OP_EN_SET_ADDR                  \
    MT6359_LDO_VCN33_2_OP_EN_SET
#define PMIC_RG_LDO_VCN33_2_OP_CFG_SET_ADDR                 \
    MT6359_LDO_VCN33_2_OP_CFG_SET
#define PMIC_RG_LDO_VCN13_LP_ADDR                           \
    MT6359_LDO_VCN13_CON0
#define PMIC_RG_LDO_VCN13_LP_MASK                           0x1
#define PMIC_RG_LDO_VCN13_LP_SHIFT                          1
#define PMIC_RG_LDO_VCN13_OP_EN_SET_ADDR                    \
    MT6359_LDO_VCN13_OP_EN_SET
#define PMIC_RG_LDO_VCN13_OP_CFG_SET_ADDR                   \
    MT6359_LDO_VCN13_OP_CFG_SET
#define PMIC_RG_LDO_VCN18_LP_ADDR                           \
    MT6359_LDO_VCN18_CON0
#define PMIC_RG_LDO_VCN18_LP_MASK                           0x1
#define PMIC_RG_LDO_VCN18_LP_SHIFT                          1
#define PMIC_RG_LDO_VCN18_OP_EN_SET_ADDR                    \
    MT6359_LDO_VCN18_OP_EN_SET
#define PMIC_RG_LDO_VCN18_OP_CFG_SET_ADDR                   \
    MT6359_LDO_VCN18_OP_CFG_SET
#define PMIC_RG_VCN13_VOCAL_ADDR                            \
    MT6359_VCN13_ANA_CON0
#define PMIC_RG_VCN13_VOCAL_MASK                            0xF
#define PMIC_RG_VCN13_VOCAL_SHIFT                           0

#endif

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
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

int consys_plt_pmic_ctrl_dump(const char* tag);

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

#endif				/* _PLATFORM_MT6885_PMIC_H_ */
