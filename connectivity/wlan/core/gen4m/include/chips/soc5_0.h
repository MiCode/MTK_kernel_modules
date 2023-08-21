/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

/*! \file  soc5_0.h
*    \brief This file contains the info of soc5_0
*/

#ifdef SOC5_0

#ifndef _SOC5_0_H
#define _SOC5_0_H

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
#define CONN_INFRA_CFG_BASE					0x830C0000
#define CONNAC2x_CONN_CFG_ON_BASE				0x7C060000
#define SOC5_0_TX_DESC_APPEND_LENGTH				32
#define SOC5_0_RX_DESC_LENGTH					24
#define SOC5_0_CHIP_ID						0x7961
#define CONNAC2X_TOP_HCR					0x88000000
#define CONNAC2X_TOP_HVR					0x88000000
#define CONNAC2X_TOP_FVR					0x88000004
#define SOC5_0_TOP_CFG_BASE					CONN_CFG_BASE
#define SOC5_0_PATCH_START_ADDR					0x00900000
#define MTK_CUSTOM_OID_INTERFACE_VERSION     0x00000200	/* for WPDWifi DLL */
#define MTK_EM_INTERFACE_VERSION		0x0001
#define RX_DATA_RING_BASE_IDX					2
#define CONNAC2x_CONN_CFG_ON_CONN_ON_MISC_DRV_FM_STAT_SYNC_SHFT	0
#define CONNAC2x_CONN_CFG_ON_CONN_ON_MISC_ADDR \
		(CONNAC2x_CONN_CFG_ON_BASE + 0xF0)

#define CONN_HOST_CSR_TOP_BASE_ADDR 0x18060000
#define CONN_INFRA_CFG_BASE_ADDR 0x18001000
#define CONN_INFRA_RGU_BASE_ADDR 0x18000000
#define WF_TOP_SLPPROT_ON_BASE_ADDR  0x184C3000
#define WF_TOP_CFG_BASE_ADDR 0x184B0000
#define WF_MCU_CFG_LS_BASE_ADDR 0x184F0000
#define WF_MCU_BUS_CR_BASE_ADDR 0x18400000
#define WF_MCUSYS_INFRA_BUS_BASE_ADDR 0x18500000
#define WF_TOP_CFG_ON_BASE_ADDR 0x184C1000
#define CONN_WT_SLP_CTL_REG_BASE_ADDR 0x18005000
#define AP2WF_CONN_INFRA_ON_CCIF4_BASE_ADDR 0x1803D000

#define CONN_INFRA_WAKEUP_WF_ADDR (CONN_HOST_CSR_TOP_BASE_ADDR + 0x01A4)
#define CONN_HW_VER_ADDR (CONN_INFRA_CFG_BASE_ADDR + 0x0000)
#define WFSYS_CPU_SW_RST_B_ADDR (CONN_INFRA_RGU_BASE_ADDR + 0x0120)
#define CONN_INFRA_WF_SLP_CTRL (CONN_INFRA_CFG_BASE_ADDR + 0x0540)
#define WFSYS_ON_TOP_PWR_CTL_ADDR (CONN_INFRA_RGU_BASE_ADDR + 0x0010)
#define TOP_DBG_DUMMY_3_CONNSYS_PWR_STATUS_ADDR \
	(CONN_HOST_CSR_TOP_BASE_ADDR + 0x02CC)
#define CONN_INFRA_WF_SLP_STATUS (CONN_INFRA_CFG_BASE_ADDR + 0x0544)
#define WF_TOP_SLPPROT_ON_STATUS (WF_TOP_SLPPROT_ON_BASE_ADDR + 0x000C)
#define WFSYS_VERSION_ID_ADDR (WF_TOP_CFG_BASE_ADDR + 0x10)
#define BUSHANGCR_BUS_HANG (WF_MCU_CFG_LS_BASE_ADDR + 0x0440)
#define WF_MCU_BUS_CR_AP2WF_REMAP_1 (WF_MCU_BUS_CR_BASE_ADDR + 0x0120)
#define DEBUG_CTRL_AO_WFMCU_PWA_CTRL0 (WF_MCUSYS_INFRA_BUS_BASE_ADDR + 0x0000)
#define WF_ROM_CODE_INDEX_ADDR (WF_TOP_CFG_ON_BASE_ADDR + 0x0604)
#define WFSYS_SW_RST_B_ADDR (CONN_INFRA_RGU_BASE_ADDR + 0x0018)
#define WB_SLP_TOP_CK_1 (CONN_WT_SLP_CTL_REG_BASE_ADDR + 0x0124)
#define CONN_INFRA_WFSYS_EMI_REQ_ADDR (CONN_INFRA_CFG_BASE_ADDR + 0x0414)
#define AP2WF_PCCIF_RCHNUM (AP2WF_CONN_INFRA_ON_CCIF4_BASE_ADDR + 0x0010)
#define AP2WF_PCCIF_ACK (AP2WF_CONN_INFRA_ON_CCIF4_BASE_ADDR + 0x0014)
#define AP2WF_PCCIF_RCHNUM_ 0x3D010
#define AP2WF_PCCIF_ACK_ 0x3D014

#define CONNSYS_VERSION_ID  0x02060002
#define WFSYS_VERSION_ID  0x02050002
#define WF_MCUSYS_INFRA_BUS_FULL_U_DEBUG_CTRL_AO_BASE 0x810F0000
#define CONNSYS_ROM_DONE_CHECK  0x00001D1E

#define WF_TRIGGER_AP2CONN_EINT 0x10001F00
#define CONN_MCU_CONFG_HS_BASE 0x89040000

#define WMMCU_ROM_PATCH_DATE_ADDR 0xF04780D0
#define WMMCU_MCU_ROM_EMI_DATE_ADDR 0xF04780E0
#define WMMCU_WIFI_ROM_EMI_DATE_ADDR 0xF04780F0
#define DATE_CODE_SIZE 16

#define CONN_INFRA_CFG_AP2WF_REMAP_1_ADDR \
	(CONN_INFRA_CFG_BASE + 0x0120)

#define CONN_INFRA_CFG_PCIE2AP_REMAP_2_ADDR \
	(0x7C00E000 + 0x24C)

#define WF_CONN_INFA_BUS_CLOCK_RATE 0x18009A00


/*******************************************************************************
*                         D A T A   T Y P E S
********************************************************************************
*/
enum ENUM_WLAN_POWER_ON_DOWNLOAD {
	ENUM_WLAN_POWER_ON_DOWNLOAD_EMI = 0,
	ENUM_WLAN_POWER_ON_DOWNLOAD_ROM_PATCH = 1,
	ENUM_WLAN_POWER_ON_DOWNLOAD_WIFI_RAM_CODE = 2
};

struct ROM_EMI_HEADER {
	uint8_t ucDateTime[16];
	uint8_t ucPLat[4];
	uint16_t u2HwVer;
	uint16_t u2SwVer;
	uint32_t u4PatchAddr;
	uint32_t u4PatchType;
	uint32_t u4CRC[4];
};

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/
#if (CFG_SUPPORT_CONNINFRA == 1)
extern u_int8_t g_IsWfsysBusHang;
extern struct completion g_triggerComp;
extern bool g_IsTriggerTimeout;
extern u_int8_t fgIsResetting;
extern u_int8_t g_fgRstRecover;
#endif

#if (CFG_ANDORID_CONNINFRA_COREDUMP_SUPPORT == 1)
extern u_int8_t g_IsNeedWaitCoredump;
#endif

#if CFG_MTK_ANDROID_EMI
extern phys_addr_t gConEmiPhyBaseFinal;
extern unsigned long long gConEmiSizeFinal;
#endif

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
void soc5_0_show_wfdma_info(struct ADAPTER *prAdapter);
void soc5_0_show_ple_info(struct ADAPTER *prAdapter, u_int8_t fgDumpTxd);
void soc5_0_show_pse_info(struct ADAPTER *prAdapter);
bool soc5_0_show_host_csr_info(struct ADAPTER *prAdapter);
void soc5_0_show_wfdma_dbg_probe_info(IN struct ADAPTER *prAdapter,
	IN enum _ENUM_WFDMA_TYPE_T enum_wfdma_type);
void soc5_0_show_wfdma_wrapper_info(IN struct ADAPTER *prAdapter,
	IN enum _ENUM_WFDMA_TYPE_T enum_wfdma_type);

extern void kalConstructDefaultFirmwarePrio(
				struct GLUE_INFO	*prGlueInfo,
				uint8_t **apucNameTable,
				uint8_t **apucName,
				uint8_t *pucNameIdx,
				uint8_t ucMaxNameIdx);

extern uint32_t kalFirmwareOpen(
				IN struct GLUE_INFO *prGlueInfo,
				IN uint8_t **apucNameTable);

extern uint32_t kalFirmwareSize(
				IN struct GLUE_INFO *prGlueInfo,
				OUT uint32_t *pu4Size);

extern uint32_t kalFirmwareLoad(
			IN struct GLUE_INFO *prGlueInfo,
			OUT void *prBuf, IN uint32_t u4Offset,
			OUT uint32_t *pu4Size);

extern uint32_t kalFirmwareClose(
			IN struct GLUE_INFO *prGlueInfo);

extern void wlanWakeLockInit(
	struct GLUE_INFO *prGlueInfo);

extern void wlanWakeLockUninit(
	struct GLUE_INFO *prGlueInfo);

extern struct wireless_dev *wlanNetCreate(
		void *pvData,
		void *pvDriverData);

extern void wlanNetDestroy(
	struct wireless_dev *prWdev);

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/
int hifWmmcuPwrOn(void);
int hifWmmcuPwrOff(void);
int wf_ioremap_read(size_t addr, unsigned int *val);
int wf_ioremap_write(phys_addr_t addr, unsigned int val);
int soc5_0_Trigger_fw_assert(void);

#if (CFG_SUPPORT_CONNINFRA == 1)
int wlanConnacPccifon(void);
int wlanConnacPccifoff(void);
int soc5_0_Trigger_whole_chip_rst(char *reason);
void soc5_0_Sw_interrupt_handler(struct ADAPTER *prAdapter);
void soc5_0_Conninfra_cb_register(void);
extern void update_driver_reset_status(uint8_t fgIsResetting);
extern int32_t get_wifi_process_status(void);
extern int32_t get_wifi_powered_status(void);
extern void update_pre_cal_status(uint8_t fgIsPreCal);
extern int8_t get_pre_cal_status(void);
#endif

#if (CFG_POWER_ON_DOWNLOAD_EMI_ROM_PATCH == 1)
void *soc5_0_kalFirmwareImageMapping(IN struct GLUE_INFO *prGlueInfo,
	OUT void **ppvMapFileBuf, OUT uint32_t *pu4FileLength,
	IN enum ENUM_IMG_DL_IDX_T eDlIdx);
uint32_t soc5_0_wlanImageSectionDownloadStage(
	IN struct ADAPTER *prAdapter, IN void *pvFwImageMapFile,
	IN uint32_t u4FwImageFileLength, IN uint8_t ucSectionNumber,
	IN enum ENUM_IMG_DL_IDX_T eDlIdx);
uint32_t soc5_0_wlanPowerOnDownload(
	IN struct ADAPTER *prAdapter,
	IN uint8_t ucDownloadItem);
int32_t soc5_0_wlanPowerOnInit(void);
#endif

void soc5_0_icapRiseVcoreClockRate(void);
void soc5_0_icapDownVcoreClockRate(void);

#if (CFG_SUPPORT_PRE_ON_PHY_ACTION == 1)
extern struct platform_device *g_prPlatDev;
uint32_t soc5_0_wlanPhyAction(IN struct ADAPTER *prAdapter);
int soc5_0_wlanPreCalPwrOn(void);
int soc5_0_wlanPreCal(void);
uint8_t *soc5_0_wlanGetCalResult(uint32_t *prCalSize);
void soc5_0_wlanCalDebugCmd(uint32_t cmd, uint32_t para);
#endif /* (CFG_SUPPORT_PRE_ON_PHY_ACTION == 1) */
#endif /* _SOC5_0_H */

#endif  /* soc5_0 */
