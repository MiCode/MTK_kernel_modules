/*
 *  Copyright (c) 2023 MediaTek Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#ifndef _BTMTK_PROJ_SP_PLATFORM_H_
#define _BTMTK_PROJ_SP_PLATFORM_H_

struct platform_prop MT6985_prop = {
	////////// tx_gpio //////////
	{224,
	 0x10005000,	// gpio_base	(GPIO_BASE)
	 0x11C00000,	// pu_pd_base	(IOCFG_RM_BASE)
	 0x500,		// remap_len
	 NULL, NULL,	// ioremap
	 {0x04C0, 0},	// aux
	 {0x0070, 0},	// dir
	 {0x0170, 0},	// out
	 {0x0040, 1},	// pu
	 {0x0030, 1}	// pd
	},
	////////// rx_gpio //////////
	{225,
	 0x10005000,	// gpio_base	(GPIO_BASE)
	 0x11C00000,	// pu_pd_base	(IOCFG_RM_BASE)
	 0x500,		// remap_len
	 NULL, NULL,	// ioremap
	 {0x04C0, 4},	// aux
	 {0x0070, 1},	// dir
	 {0x0170, 1},	// out
	 {0x0040, 0},	// pu
	 {0x0030, 0}	// pd
	},
	////////// rst_gpio //////////
	{240,
	 0x10005000,	// gpio_base	(GPIO_BASE)
	 0x11B20000,	// pu_pd_base	(IOCFG_RT_BASE)
	 0x500,		// remap_len
	 NULL, NULL,	// ioremap
	 {0x04E0, 0},	// aux
	 {0x0070, 16},	// dir
	 {0x0170, 16},	// out
	 {0x0060, 0},	// pu
	 {0x0040, 0}	// pd
	}
};

struct platform_prop MT6989_prop = {
	////////// tx_gpio //////////
	{139,
	 0x10005000,	// gpio_base	(GPIO_BASE)
	 0x11C30000,	// pu_pd_base	(IOCFG_RM_BASE)
	 0x500,		// remap_len
	 NULL, NULL,	// ioremap
	 {0x0410, 12},	// aux
	 {0x0040, 11},	// dir
	 {0x0140, 11},	// out
	 {0x0090, 5},	// pu
	 {0x0080, 5}	// pd
	},
	////////// rx_gpio //////////
	{140,
	 0x10005000,	// gpio_base	(GPIO_BASE)
	 0x11C30000,	// pu_pd_base	(IOCFG_RM_BASE)
	 0x500,		// remap_len
	 NULL, NULL,	// ioremap
	 {0x0410, 16},	// aux
	 {0x0040, 12},	// dir
	 {0x0140, 12},	// out
	 {0x0090, 4},	// pu
	 {0x0080, 4}	// pd
	},
	////////// rst_gpio //////////
	{150,
	 0x10005000,	// gpio_base	(GPIO_BASE)
	 0x11B30000,	// pu_pd_base	(IOCFG_TR_BASE)
	 0x500,		// remap_len
	 NULL, NULL,	// ioremap
	 {0x0420, 24},	// aux
	 {0x0040, 22},	// dir
	 {0x0140, 22},	// out
	 {0x00a0, 0},	// pu
	 {0x0080, 0}	// pd
	}
};

struct platform_prop MT6991_prop = {
        ////////// tx_gpio //////////
        {237,
         0x1002D000,    // gpio_base    (GPIO_BASE)
         0x13860000,    // pu_pd_base   (IOCFG_TM3_BASE)
         0x500,         // remap_len
         NULL, NULL,    // ioremap
         {0x04d0, 20},  // aux
         {0x0070, 13},  // dir
         {0x0170, 13},  // out
         {0x0090, 6},   // pu
         {0x0070, 6}    // pd
        },
        ////////// rx_gpio //////////
        {238,
         0x1002D000,    // gpio_base    (GPIO_BASE)
         0x13860000,    // pu_pd_base   (IOCFG_TM3_BASE)
         0x500,         // remap_len
         NULL, NULL,    // ioremap
         {0x04d0, 24},  // aux
         {0x0070, 14},  // dir
         {0x0170, 14},  // out
         {0x0090, 5},   // pu
         {0x0070, 5}    // pd
        },
        ////////// rst_gpio //////////
        {248,
         0x1002D000,    // gpio_base    (GPIO_BASE)
         0x13860000,    // pu_pd_base   (IOCFG_TM3_BASE)
         0x500,         // remap_len
         NULL, NULL,    // ioremap
         {0x04f0, 0},   // aux
         {0x0070, 24},  // dir
         {0x0170, 24},  // out
         {0x0090, 4},   // pu
         {0x0070, 4}    // pd
        }
};


#endif

