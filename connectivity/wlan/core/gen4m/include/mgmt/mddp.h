/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
/*
 ** Id: //Department/DaVinci/BRANCHES/
 *      MT6620_WIFI_DRIVER_V2_3/include/mgmt/mddp.h#1
 */

/*! \file   "mddp.h"
 *    \brief  The declaration of nic functions
 *
 *    Detail description.
 */


#ifndef _MDDP_H
#define _MDDP_H

#if CFG_MTK_MDDP_SUPPORT

#include "mddp_export.h"

#if CFG_MTK_CCCI_SUPPORT
#include "mtk_ccci_common.h"
#endif

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define MD_ON_OFF_TIMEOUT			1000
#define MD_ON_OFF_TIMEOUT_CASAN		3000
#if (CFG_SUPPORT_CONNAC3X == 1)
#define MD_LPCTL_ADDR 0x7C001614
#define MD_STATUS_SYNC_CR 0
#elif (CFG_SUPPORT_CONNAC2X == 1)
#define MD_STATUS_SYNC_CR 0x180600F4
#define MD_LPCTL_ADDR 0x7C060050
#else
#define MD_STATUS_SYNC_CR 0x1800701C
#define MD_LPCTL_ADDR 0x7030
#endif
#define MD_STATUS_INIT_SYNC_BIT BIT(0)
#define MD_STATUS_OFF_SYNC_BIT  BIT(1)
#define MD_STATUS_ON_SYNC_BIT   BIT(2)

#define MD_AOR_SET_CR_ADDR 0x10001BEC
#define MD_AOR_CLR_CR_ADDR 0x10001BF0
#define MD_AOR_RD_CR_ADDR  0x10001BF4
#define MD_AOR_MD_INIT_BIT BIT(8)
#define MD_AOR_MD_OFF_BIT  BIT(9)
#define MD_AOR_MD_ON_BIT   BIT(10)
#define MD_AOR_WIFI_ON_BIT BIT(11)

#define MD_SHM_AP_STAT_BIT BIT(4)
#define MD_SHM_MD_INIT_BIT BIT(8) /* md_stat */
#define MD_SHM_MD_OFF_BIT  BIT(9)
#define MD_SHM_MD_ON_BIT   BIT(10)
#define MD_SHM_WIFI_ON_BIT (BIT(11) | MD_SHM_AP_STAT_BIT) /* ap_stat */

#if (CFG_SUPPORT_CONNAC2X == 0 && CFG_SUPPORT_CONNAC3X == 0)
/* Use SER dummy register for mddp support flag */
#define MDDP_SUPPORT_CR 0x820600d0
#define MDDP_SUPPORT_CR_BIT BIT(23)
#endif

#define MDDP_LPCR_MD_SET_FW_OWN BIT(0)

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */
struct MDDP_SETTINGS;

enum ENUM_MDDP_SUPPORT_MODE {
	MDDP_SUPPORT_SHM = 0,
	MDDP_SUPPORT_AOP,
	MDDP_SUPPORT_NUM
};


struct MDDP_STATUS_SYNC_OPS {
	void (*rd)(struct MDDP_SETTINGS *prSettings, uint32_t *pu4Val);
	void (*set)(struct MDDP_SETTINGS *prSettings, uint32_t u4Bit);
	void (*clr)(struct MDDP_SETTINGS *prSettings, uint32_t u4Bit);
};

struct MDDP_SETTINGS {
	struct MDDP_STATUS_SYNC_OPS rOps;
	uint32_t u4SyncAddr;
	uint32_t u4SyncSetAddr;
	uint32_t u4SyncClrAddr;
	uint32_t u4MdInitBit;
	uint32_t u4MdOnBit;
	uint32_t u4MdOffBit;
	uint32_t u4WifiOnBit;
	enum ENUM_MDDP_SUPPORT_MODE u4MDDPSupportMode;
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
void mddpInit(void);
void mddpUninit(void);
int32_t mddpMdNotifyInfo(struct mddpw_md_notify_info_t *prMdInfo);
int32_t mddpChangeState(enum mddp_state_e event, void *buf, uint32_t *buf_len);
int32_t mddpGetMdStats(struct net_device *prDev);
#if CFG_SUPPORT_LLS && CFG_SUPPORT_LLS_MDDP
int32_t mddpGetMdLlsStats(struct ADAPTER *prAdapter);
#endif
void mddpUpdateReorderQueParm(struct ADAPTER *prAdapter,
			      struct RX_BA_ENTRY *prReorderQueParm,
			      struct SW_RFB *prSwRfb);
int32_t mddpNotifyDrvTxd(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t fgActivate);
int32_t mddpNotifyStaTxd(struct ADAPTER *prAdapter);
void mddpNotifyWifiOnStart(void);
int32_t mddpNotifyWifiOnEnd(void);
void mddpNotifyWifiOffStart(void);
void mddpNotifyWifiOffEnd(void);
void mddpNotifyWifiReset(void);
void setMddpSupportRegister(struct ADAPTER *prAdapter);
#if CFG_MTK_CCCI_SUPPORT
void mddpMdStateChangedCb(enum MD_STATE old_state,
		enum MD_STATE new_state);
#endif
void mddpSetMDFwOwn(void);
u_int8_t mddpIsMDFwOwn(void);
void mddpEnableMddpSupport(void);
void mddpDisableMddpSupport(void);
bool mddpIsSupportMcifWifi(void);
bool mddpIsSupportMddpWh(void);
#if defined(_HIF_PCIE)
#if CFG_SUPPORT_PCIE_ASPM
int32_t mddpNotifyMDPCIeL12Status(uint32_t u32Enable);
#endif
#endif

#endif /* CFG_MTK_MDDP_SUPPORT */

#endif /* _MDDP_H */
