/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _PLATFORM_MT6877_PMIC_H_
#define _PLATFORM_MT6877_PMIC_H_

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

#endif /* _PLATFORM_MT6877_PMIC_H_ */
