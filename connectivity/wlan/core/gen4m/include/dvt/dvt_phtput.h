/******************************************************************************
 *
 * This file is provided under a dual license.  When you use or
 * distribute this software, you may choose to be licensed under
 * version 2 of the GNU General Public License ("GPLv2 License")
 * or BSD License.
 *
 * GPLv2 License
 *
 * Copyright(C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * BSD LICENSE
 *
 * Copyright(C) 2016 MediaTek Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/

/*! \file   "dvt_phtput.h"
 *    \brief  The declaration of nic ph-tput functions
 *
 *    Detail description.
 */


#ifndef _DVT_PHTPUT_H
#define _DVT_PHTPUT_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
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

struct PhTputSetting {
	uint16_t u2CmdId;
	uint8_t fgIsSec;
};

enum ENUM_PHPTUT_SETTING {
	/*STA*/
	ENUM_PHTPUT_LEGACY_OPEN_STA		= 1,
	ENUM_PHTPUT_LEGACY_SEC_GCMP256_STA	= 2,
	ENUM_PHTPUT_LEGACY_SEC_WEP128_STA	= 3,
	ENUM_PHTPUT_LEGACY_SEC_TKIP_STA		= 4,
	ENUM_PHTPUT_LEGACY_SEC_AES_STA		= 5,
	ENUM_PHTPUT_MLO_OPEN_BN0_BN2_STA	= 101,
	ENUM_PHTPUT_MLO_SEC_BN0_BN2_STA		= 102,
	ENUM_PHTPUT_MLO_OPEN_BN0_BN1_STA	= 103,
	ENUM_PHTPUT_MLO_SEC_BN0_BN1_STA		= 104,
	ENUM_PHTPUT_DBDC_OPEN_BN0_BN2_STA	= 201,
	ENUM_PHTPUT_DBDC_SEC_BN0_BN2_STA	= 202,
	ENUM_PHTPUT_DBDC_OPEN_BN0_BN1_STA	= 203,
	ENUM_PHTPUT_DBDC_SEC_BN0_BN1_STA	= 204,
	/*AP*/
	ENUM_PHTPUT_LEGACY_OPEN_AP		= 1001,
	ENUM_PHTPUT_LEGACY_SEC_GCMP256_AP	= 1002,
	ENUM_PHTPUT_LEGACY_SEC_WEP128_AP	= 1003,
	ENUM_PHTPUT_LEGACY_SEC_TKIP_AP		= 1004,
	ENUM_PHTPUT_LEGACY_SEC_AES_AP		= 1005,
	ENUM_PHTPUT_MLO_OPEN_BN0_BN2_AP		= 1101,
	ENUM_PHTPUT_MLO_SEC_BN0_BN2_AP		= 1102,
	ENUM_PHTPUT_MLO_OPEN_BN0_BN1_AP		= 1103,
	ENUM_PHTPUT_MLO_SEC_BN0_BN1_AP		= 1104,
	ENUM_PHTPUT_DBDC_OPEN_BN0_BN2_AP	= 1201,
	ENUM_PHTPUT_DBDC_SEC_BN0_BN2_AP		= 1202,
	ENUM_PHTPUT_DBDC_OPEN_BN0_BN1_AP	= 1203,
	ENUM_PHTPUT_DBDC_SEC_BN0_BN1_AP		= 1204
};

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
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
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

uint32_t dvtSetupPhTput(struct net_device *prNetDev,
			uint32_t u4CaseIndex);
uint32_t dvtActivateNetworkPhTput(struct net_device *prNetDev,
			uint8_t ucBssIndex,
			struct PhTputSetting *prPhtputSetting);
uint32_t dvtDeactivateNetworkPhTput(struct net_device *prNetDev,
			uint8_t ucBssIndex);

#endif /* _DVT_PHTPUT_H */

