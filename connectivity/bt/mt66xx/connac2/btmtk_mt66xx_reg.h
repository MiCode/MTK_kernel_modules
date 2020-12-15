/*
*  Copyright (C) 2016 MediaTek Inc.
*
* This program is free software: you can redistribute it and/or modify it under the terms of the
* GNU General Public License version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with this program.
* If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _BT_REG_H
#define _BT_REG_H

//#include "typedef.h"

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/
#ifndef BIT
#define BIT(n)                          (1UL << (n))
#endif /* BIT */

#ifndef BITS
/* bits range: for example BITS(16,23) = 0xFF0000
 *   ==>  (BIT(m)-1)   = 0x0000FFFF     ~(BIT(m)-1)   => 0xFFFF0000
 *   ==>  (BIT(n+1)-1) = 0x00FFFFFF
 */
#define BITS(m, n)                      (~(BIT(m)-1) & ((BIT(n) - 1) | BIT(n)))
#endif /* BIT */

/*
 * This macro returns the byte offset of a named field in a known structure
 * type.
 * _type - structure name,
 * _field - field name of the structure
 */
#ifndef OFFSET_OF
#define OFFSET_OF(_type, _field)    ((unsigned long)&(((_type *)0)->_field))
#endif /* OFFSET_OF */

/*
 * This macro returns the base address of an instance of a structure
 * given the type of the structure and the address of a field within the
 * containing structure.
 * _addr_of_field - address of a field within the structure,
 * _type - structure name,
 * _field - field name of the structure
 */
#ifndef ENTRY_OF
#define ENTRY_OF(_addr_of_field, _type, _field) \
	((_type *)((unsigned char *)(_addr_of_field) - (unsigned char *)OFFSET_OF(_type, _field)))
#endif /* ENTRY_OF */



/*********************************************************************
*
*  Connsys Control Register Definition
*
**********************************************************************
*/
#define SET_BIT(addr, bit) \
			(*((volatile uint32_t *)(addr))) |= ((uint32_t)bit)
#define CLR_BIT(addr, bit) \
			(*((volatile uint32_t *)(addr))) &= ~((uint32_t)bit)
#define REG_READL(addr) \
			readl((volatile uint32_t *)(addr))
#define REG_WRITEL(addr, val) \
			writel(val, (volatile uint32_t *)(addr))

/*
 * EMI region
 */
#define BT_EMI_BASE_OFFSET		0x000000000
#define BT_EMI_LENGTH			0x000229400

/*
 * ConnInfra Control Register Region:
 *	  (CONN_INFRA_BASE) ~ (CONN_INFRA_BASE + CONN_INFRA_LENGTH)
 */
#define CONN_INFRA_BASE                     0x18000000 /* variable */
#define CONN_INFRA_LENGTH                   0x003FFFFF

/*
 * Conninfra CFG AO
 *
 */
#define CONN_INFRA_CFG_AO_BASE		0x10001000
#define CONN_INFRA_CFG_AO_LENGTH	0x1000

#define BGF_PAD_EINT			0xF00

/*
 * ConnInfra RGU Region
 */
#define CONN_INFRA_RGU_START                    0x00000000

#define CONN_INFRA_RGU_BGFSYS_ON_TOP_PWR_CTL    (CONN_INFRA_RGU_START + 0x0008)
#define BGF_PWR_CTL_B                           BIT(7)

#define CONN_INFRA_RGU_BGFSYS_CPU_SW_RST        (CONN_INFRA_RGU_START + 0x0014)
#define BGF_CPU_SW_RST_B                        BIT(0)

#define CONN_INFRA_RGU_BGFSYS_ON_TOP_PWR_ACK_ST (CONN_INFRA_RGU_START + 0x0114)
#define BGF_PWR_ACK_B                           BITS(24, 25)

/*
 * ConnInfra CFG Region
 */
#define CONN_INFRA_CFG_START			0x00001000

#define CONN_INFRA_CFG_VERSION			(CONN_INFRA_CFG_START)
#define CONN_INFRA_CFG_ID			(0x20010000)

#define CONN_INFRA_CONN2BT_GALS_SLP_CTL		(CONN_INFRA_CFG_START + 0x0610)
#define CONN2BT_SLP_PROT_TX_EN_B		BIT(0)
#define CONN2BT_SLP_PROT_TX_ACK_B		BIT(3)
#define CONN2BT_SLP_PROT_RX_EN_B		BIT(4)
#define CONN2BT_SLP_PROT_RX_ACK_B		BIT(7)

#define CONN_INFRA_BT2CONN_GALS_SLP_CTL		(CONN_INFRA_CFG_START + 0x0614)
#define BT2CONN_SLP_PROT_TX_EN_B		BIT(0)
#define BT2CONN_SLP_PROT_TX_ACK_B		BIT(3)
#define BT2CONN_SLP_PROT_RX_EN_B		BIT(4)
#define BT2CONN_SLP_PROT_RX_ACK_B		BIT(7)

#define CONN_INFRA_CFG_BT_PWRCTLCR0		(CONN_INFRA_CFG_START + 0x0874)
#define BT_FUNC_EN_B				BIT(0)

/*
 * Connsys Thermal Region
 */
#define CONN_THERM_START			0x00002000

/*
 * ConnInfra Sysram Region
 */
#define CONN_INFRA_SYSRAM_START             0x00050000

/*
 * Connsys Host CSR Top Region
 */
#define CONN_HOST_CSR_TOP_START             0x00060000

#define CONN_INFRA_WAKEUP_BT		(CONN_HOST_CSR_TOP_START + 0x01A8)
#define CONN_HOST_CSR_TOP_CONN_SLP_PROT_CTRL \
					(CONN_HOST_CSR_TOP_START + 0x0184)

#define CONN_INFRA_ON2OFF_SLP_PROT_ACK	BIT(5)


#define BGF_LPCTL                           (CONN_HOST_CSR_TOP_START + 0x0030)
#define BGF_HOST_SET_FW_OWN_B               BIT(0)
#define BGF_HOST_CLR_FW_OWN_B               BIT(1)
#define BGF_OWNER_STATE_SYNC_B              BIT(2)

#define BGF_IRQ_STAT                        (CONN_HOST_CSR_TOP_START + 0x0034)
#define BGF_IRQ_FW_OWN_CLR_B                BIT(0)

#define BGF_IRQ_EN                          (CONN_HOST_CSR_TOP_START + 0x0038)
#define BGF_IRQ_DRV_OWN_EN_B                BIT(0)
#define BGF_IRQ_FW_OWN_EN_B                 BIT(1)

#define BGF_IRQ_STAT2                       (CONN_HOST_CSR_TOP_START + 0x003C)
#define BGF_IRQ_FW_OWN_SET_B                BIT(0)

/*
 * BGFSYS Control Register Region:
 *     (BGFSYS_BASE) ~ (BGFSYS_BASE + BGFSYS_LENGTH)
 */
#define BGFSYS_BASE			0x18800000
#define BGFSYS_LENGTH			0x003FFFFF

#define BGF_MCCR			0x0100
#define BGF_CON_CR_AHB_AUTO_DIS		BIT(31)

#define BGF_MCCR_SET			0x0104
#define BGF_CON_CR_AHB_STOP		BIT(2)

#define BGF_SW_IRQ_RESET_ADDR		0x014C
#define BGF_SW_IRQ_STATUS		0x0150
#define BGF_WHOLE_CHIP_RESET		BIT(26)
#define BGF_SUBSYS_CHIP_RESET		BIT(25)
#define BGF_FW_LOG_NOTIFY		BIT(24)

#define BGF_MCU_CFG_SW_DBG_CTL		0x016C

#define BGF_HW_VERSION			0x00012000
#define BGF_HW_VER_ID			0x00008A00

#define BGF_FW_VERSION			0x00012004
#define BGF_FW_VER_ID			0x00008A00

#define BGF_HW_CODE			0x00012008
#define BGF_HW_CODE_ID			0x00000000

#define BGF_IP_VERSION			0x00012010
#define BGF_IP_VER_ID			0x20010000

extern uint8_t *p_conn_infra_base_addr;
extern uint8_t *p_bgfsys_base_addr;

#define CONN_INFRA_REG_READL(addr) \
		REG_READL((p_conn_infra_base_addr + addr))
#define CONN_INFRA_REG_WRITEL(addr, val) \
		REG_WRITEL((p_conn_infra_base_addr + addr), val)
#define CONN_INFRA_SET_BIT(addr, bit) \
		SET_BIT((p_conn_infra_base_addr + addr), bit)
#define CONN_INFRA_CLR_BIT(addr, bit) \
		CLR_BIT((p_conn_infra_base_addr + addr), bit)

#define BGFSYS_REG_READL(addr) \
		REG_READL((p_bgfsys_base_addr + addr))
#define BGFSYS_REG_WRITEL(addr, val) \
		REG_WRITEL((p_bgfsys_base_addr + addr), val)
#define BGFSYS_SET_BIT(addr, bit) \
		SET_BIT((p_bgfsys_base_addr + addr), bit)
#define BGFSYS_CLR_BIT(addr, bit) \
		CLR_BIT((p_bgfsys_base_addr + addr), bit)


#endif
