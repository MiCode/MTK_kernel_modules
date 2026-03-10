ccflags-y :=
CFG_DIR ?= $(src)/configs

ifeq ($(CONFIG_MTK_PLATFORM),)
    CONFIG_MTK_PLATFORM := mt$(WLAN_CHIP_ID)
endif
MTK_PLATFORM := $(subst $(quote),,$(CONFIG_MTK_PLATFORM))

ifeq ($(CONFIG_WLAN_PLATFORM),)
    CONFIG_WLAN_PLATFORM := mt$(WLAN_CHIP_ID)
endif

# WM RAM TYPE
# WM_RAM - mobile | ce
WM_RAM ?= mobile

# ---------------------------------------------------
# Kbuild option
# ---------------------------------------------------
ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF),)
ifneq ($(src),)
ifneq ($(srctree),)
hif=pcie
include $(srctree)/$(src)/Makefile.x86
src:=$(srctree)/$(shell echo ${src})
MTK_COMBO_CHIP=MT7961
$(info Kbuild chip: $(MTK_COMBO_CHIP))
$(info Kbuild hif : $(CONFIG_MTK_COMBO_WIFI_HIF))
endif
endif
endif

# ---------------------------------------------------
# OS option
# ---------------------------------------------------
os=$(CONFIG_MTK_SUPPORT_OS)

ifeq ($(os),)
os=linux
endif

ccflags-y += -Wno-unused-value
ccflags-y += -Wno-unused-result
ccflags-y += -Wno-format
ccflags-y += -Wno-parentheses

ccflags-y += -Wno-unused-function

ifeq ($(os), none)
ccflags-y += -I/usr/include/
ccflags-y += -DCFG_VIRTUAL_OS
ccflags-y += -DCFG_REMIND_IMPLEMENT
endif

ifeq ($(CONFIG_SUPPORT_OPENWRT),y)
ccflags-y += -DCONFIG_SUPPORT_OPENWRT
endif

ifndef TOP
    TOP := $(srctree)/..
endif
$(info os option: $(os))

# script for check the kernel version condition
# example check kernel >= 5.4 : $(call kver_ge,5,4)
kver_ge = $(shell \
echo check_ver | awk '{if($(VERSION) < $(1)) {print 0} else { \
if($(VERSION) > $(1)) {print 1} else { \
if($(PATCHLEVEL) < $(2)) {print 0} else {print 1} \
}}}' \
)

# ---------------------------------------------------
# ALPS Setting
# ---------------------------------------------------
ifneq ($(KERNEL_OUT),)
    ccflags-y += -imacros $(KERNEL_OUT)/include/generated/autoconf.h
endif

ifeq ($(KBUILD_MODPOST_FAIL_ON_WARNINGS),)
    # Force build fail on modpost warning
    KBUILD_MODPOST_FAIL_ON_WARNINGS=y
endif

# ---------------------------------------------------
# Compile Options
# ---------------------------------------------------
WLAN_CHIP_LIST:=-UMT6620 -UMT6628 -UMT5931 -UMT6630 -UMT6632 -UMT7663 -UCONNAC -USOC2_1X1 -USOC2_2X2 -UUT_TEST_MODE -UMT7915 -USOC3_0 -UMT7961 -USOC5_0 -USOC7_0 -UBELLWETHER -UMT6639 -UMT6655 -UMT7990
# '-D' and '-U' options are processed in the order they are given on the command line.
# All '-imacros file' and '-include file' options are processed after all '-D' and '-U' options.
ccflags-y += $(WLAN_CHIP_LIST)

ifeq ($(MTK_COMBO_CHIP),)
MTK_COMBO_CHIP = MT6632
endif

$(info $$WM_RAM is [${WM_RAM}])
$(info $$MTK_PLATFORM is [${MTK_PLATFORM}])
$(info $$CONFIG_WLAN_PLATFORM is [${CONFIG_WLAN_PLATFORM}])
$(info $$WLAN_CHIP_ID is [${WLAN_CHIP_ID}])
$(info $$MTK_COMBO_CHIP is [${MTK_COMBO_CHIP}])
$(info $$WLAN_CONNAC3_DEV is [${WLAN_CONNAC3_DEV}])

include $(CFG_DIR)/defconfig
ifneq ($(wildcard $(CFG_DIR)/${MTK_COMBO_CHIP}/defconfig),)
    include $(CFG_DIR)/${MTK_COMBO_CHIP}/defconfig
endif


ifneq ($(CONFIG_MTK_EMI_LEGACY),)
ccflags-y += -DCONFIG_WLAN_MTK_EMI=1
endif

ifneq ($(CONFIG_MTK_EMI_LEGACY),)
ccflags-y += -DCONFIG_WLAN_MTK_EMI=1
endif

ifneq ($(CONFIG_MTK_EMI),)
ccflags-y += -DCONFIG_WLAN_MTK_EMI=1
endif

ifneq ($(CONFIG_MEDIATEK_EMI),)
ccflags-y += -DCONFIG_WLAN_MTK_EMI=1
endif

ifeq ($(WLAN_CHIP_ID),)
WLAN_CHIP_ID=$(word 1, $(MTK_COMBO_CHIP))
endif

ccflags-y += -DCFG_SUPPORT_DEBUG_FS=0
ccflags-y += -DWLAN_INCLUDE_PROC
ccflags-y += -DCFG_SUPPORT_AGPS_ASSIST=0
ccflags-y += -DCFG_SUPPORT_TSF_USING_BOOTTIME=1
ccflags-y += -DARP_MONITER_ENABLE=1
ccflags-y += -Werror $(call cc-disable-warning, unused-but-set-variable)
#ccflags-y:=$(filter-out -U$(WLAN_CHIP_ID),$(ccflags-y))
#ccflags-y += -DLINUX -D$(WLAN_CHIP_ID)
#workaround: also needed for none LINUX system
# because some of common part code is surrounded with this flag
ccflags-y += -DLINUX

ifneq ($(filter MT6632,$(MTK_COMBO_CHIP)),)
ccflags-y:=$(filter-out -UMT6632,$(ccflags-y))
ccflags-y += -DMT6632
endif

ifneq ($(filter MT7668,$(MTK_COMBO_CHIP)),)
ccflags-y:=$(filter-out -UMT7668,$(ccflags-y))
ccflags-y += -DMT7668
endif

ifneq ($(filter MT7663,$(MTK_COMBO_CHIP)),)
ccflags-y:=$(filter-out -UMT7663,$(ccflags-y))
ccflags-y += -DMT7663
endif

ifneq ($(filter CONNAC,$(MTK_COMBO_CHIP)),)
ccflags-y:=$(filter-out -UCONNAC,$(ccflags-y))
ccflags-y += -DCONNAC
endif

ifneq ($(filter SOC2_1X1,$(MTK_COMBO_CHIP)),)
ccflags-y:=$(filter-out -USOC2_1X1,$(ccflags-y))
ccflags-y += -DSOC2_1X1
ccflags-y += -DCONFIG_MTK_WIFI_VHT80
ifneq ($(filter 6835, $(WLAN_CHIP_ID)),)
        CONFIG_RX_NAPI_THREADED=y
	ccflags-y += -DCFG_WLAN_LK_FWDL_SUPPORT=1
	ccflags-y += -DCFG_WLAN_ATF_SUPPORT=0
endif
endif

ifneq ($(filter SOC2_2X2,$(MTK_COMBO_CHIP)),)
ccflags-y:=$(filter-out -USOC2_2X2,$(ccflags-y))
ccflags-y += -DSOC2_2X2
ccflags-y += -DCONFIG_MTK_WIFI_VHT80
endif

ifneq ($(findstring MT7915,$(MTK_COMBO_CHIP)),)
ccflags-y:=$(filter-out -UMT7915,$(ccflags-y))
ccflags-y += -DMT7915
CONFIG_MTK_WIFI_CONNAC2X=y
CONFIG_MTK_WIFI_11AX_SUPPORT=y
CONFIG_MTK_WIFI_TWT_SUPPORT=y
CONFIG_MTK_WIFI_TWT_SMART_STA=n
CONFIG_MTK_TWT_STA_CNM=n
CONFIG_MTK_WIFI_TWT_HOTSPOT_SUPPORT=n
CONFIG_MTK_WIFI_TWT_HOTSPOT_AC_SUPPORT=n
CONFIG_NUM_OF_WFDMA_RX_RING=5
CONFIG_NUM_OF_WFDMA_TX_RING=1
endif

ifneq ($(findstring 3_0,$(MTK_COMBO_CHIP)),)
ccflags-y:=$(filter-out -USOC3_0,$(ccflags-y))
ccflags-y += -DSOC3_0
CONFIG_MTK_WIFI_CONNAC2X=y
CONFIG_MTK_WIFI_11AX_SUPPORT=y
CONFIG_MTK_WIFI_TWT_SUPPORT=y
CONFIG_MTK_WIFI_TWT_SMART_STA=n
CONFIG_MTK_TWT_STA_CNM=n
CONFIG_MTK_WIFI_TWT_HOTSPOT_SUPPORT=n
CONFIG_MTK_WIFI_TWT_HOTSPOT_AC_SUPPORT=n
CONFIG_NUM_OF_WFDMA_RX_RING=3
CONFIG_NUM_OF_WFDMA_TX_RING=0
CONFIG_MTK_WIFI_CONNINFRA_SUPPORT=y
CONFIG_SUPPORT_PRE_ON_PHY_ACTION=y
CONFIG_MTK_WIFI_CONNAC2X_2x2=y
CONFIG_MTK_WIFI_DOWNLOAD_DYN_MEMORY_MAP=y
CFG_WIFI_WORKAROUND_HWITS00012836_WTBL_SEARCH_FAIL=1
CFG_WIFI_WORKAROUND_HWITS00010371_PMF_CIPHER_MISMATCH=1
CONFIG_SUPPORT_FORCE_ALTX=y
ccflags-y += -DCFG_POWER_ON_DOWNLOAD_EMI_ROM_PATCH=1
ccflags-y += -DCFG_ROM_PATCH_NO_SEM_CTRL=1
ccflags-y += -DCFG_MTK_WIFI_SW_WFDMA=1
CONFIG_MTK_WIFI_WFDMA_BK_RS=y
ccflags-y += -DCONFIG_MTK_WIFI_HE80
ifeq ($(WLAN_CONNAC3_DEV), yes)
CONFIG_MTK_WIFI_UNIFIED_COMMND_SUPPORT=y
CONFIG_MTK_WIFI_11BE_SUPPORT=y
CONFIG_MTK_WIFI_11BE_MLO_SUPPORT=y
CONFIG_MTK_WIFI_APS_SUPPORT=y
CONFIG_MLD_LINK_MAX=2
CONFIG_MTK_WIFI_6G_SUPPORT=y
CONFIG_DBDC_MODE=1
endif
endif

ifneq ($(findstring MT7961,$(MTK_COMBO_CHIP)),)
ccflags-y:=$(filter-out -UMT7961,$(ccflags-y))
ccflags-y += -DMT7961
ccflags-y += -DCFG_SDIO_INTR_ENHANCE_FORMAT=2
ccflags-y += -DCFG_SUPPORT_CMD_OVER_WFDMA=1
# MT7961's max cmd tx resource = 3
# so QM_CMD_RESERVED_THRESHOL must be less than 3
ccflags-y += -DQM_CMD_RESERVED_THRESHOLD=1
CONFIG_MTK_WIFI_CONNAC2X=y
CONFIG_MTK_WIFI_11AX_SUPPORT=y
# unless patch wifi 6G setting in kernel before kernel version 5.4,
# it will build fail when enable wifi 6G flag
ifeq ($(call kver_ge,5,4),1)
CONFIG_MTK_WIFI_6G_SUPPORT=y
else
CONFIG_MTK_WIFI_6G_SUPPORT=n
endif
CONFIG_MTK_WIFI_TWT_SUPPORT=y
CONFIG_MTK_WIFI_TWT_SMART_STA=n
CONFIG_MTK_TWT_STA_CNM=n
CONFIG_MTK_WIFI_TWT_HOTSPOT_SUPPORT=n
CONFIG_MTK_WIFI_TWT_HOTSPOT_AC_SUPPORT=n
CONFIG_NUM_OF_WFDMA_RX_RING=5
CONFIG_NUM_OF_WFDMA_TX_RING=1
CONFIG_SUPPORT_FORCE_ALTX=y
CONFIG_MTK_WIFI_DBDC_6G=y
CONFIG_SUPPORT_WIFI_DL_BT_PATCH=y
CONFIG_SUPPORT_PCIE_ASPM=y
CONFIG_WIFI_DRIVER_OWN_DELAY_SUPPORT=y
CFG_SUPPORT_HOST_RX_WM_EVENT_FROM_PSE=y
CONFIG_NOT_CLR_FREE_MSDU_IN_DEACTIVE_NETWORK=y

# AP KVR
ifeq ($(CFG_MTK_AP_80211KVR_INTERFACE), y)
    CONFIG_WIFI_SUPPORT_GET_NOISE=y
    CFG_WIFI_SUPPORT_NOISE_HISTOGRAM=y
    ccflags-y += -DCFG_AP_80211KVR_INTERFACE=1
    ccflags-y += -DCFG_AP_80211K_SUPPORT=1
    ccflags-y += -DCFG_AP_80211V_SUPPORT=1
endif

ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), sdio)
CONFIG_TX_RSRC_WMM_ENHANCE=y
endif
ccflags-y += -DCFG_WIFI_FWDL_UMAC_RESERVE_SIZE_PARA=128
CONFIG_MTK_DBDC_SW_FOR_P2P_LISTEN=n
# Default disable feature notify country code to bt
CONFIG_SUPPORT_BT_SKU=n
ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), pcie)
CFG_COALESCING_INTERRUPT=y
endif
# For MT7961, only PCIE & USB support
ifneq ($(CONFIG_MTK_COMBO_WIFI_HIF), sdio)
CONFIG_SUPPORT_DEBUG_SOP=y
endif
ifeq ($(CFG_WIFI_GET_MCS_INFO), y)
	ccflags-y += -DCFG_WIFI_GET_MCS_INFO=1
else
	ccflags-y += -DCFG_WIFI_GET_MCS_INFO=0
endif
ifeq ($(CFG_GET_CNM_INFO_BC), y)
    ccflags-y += -DCFG_GET_CNM_INFO_BC=1
else
    ccflags-y += -DCFG_GET_CNM_INFO_BC=0
endif
endif

ifneq ($(filter 6873, $(WLAN_CHIP_ID)),)
    ccflags-y += -DCFG_ENABLE_HOST_BUS_TIMEOUT=1
else
    ccflags-y += -DCFG_ENABLE_HOST_BUS_TIMEOUT=0
endif

#ifneq ($(filter SOC2_1x1 SOC2_2x2, $(MTK_COMBO_CHIP)),)
ifneq ($(filter 6835, $(WLAN_CHIP_ID)),)
    ccflags-y += -DCFG_SUPPORT_MDDP_AOR=0
    ccflags-y += -DCFG_SUPPORT_MDDP_SHM=1
else ifneq ($(filter 6855, $(WLAN_CHIP_ID)),)
    ccflags-y += -DCFG_SUPPORT_MDDP_AOR=1
    ccflags-y += -DCFG_SUPPORT_MDDP_SHM=0
else
    ccflags-y += -DCFG_SUPPORT_MDDP_AOR=0
    ccflags-y += -DCFG_SUPPORT_MDDP_SHM=0
endif
#endif

ifneq ($(findstring 5_0,$(MTK_COMBO_CHIP)),)
ccflags-y:=$(filter-out -USOC5_0,$(ccflags-y))
ccflags-y += -DSOC5_0
CONFIG_MTK_WIFI_CONNAC2X=y
CONFIG_MTK_WIFI_11AX_SUPPORT=y
CONFIG_MTK_WIFI_TWT_SUPPORT=y
CONFIG_MTK_WIFI_TWT_SMART_STA=n
CONFIG_MTK_TWT_STA_CNM=n
CONFIG_MTK_WIFI_TWT_HOTSPOT_SUPPORT=n
CONFIG_MTK_WIFI_TWT_HOTSPOT_AC_SUPPORT=n
CONFIG_NUM_OF_WFDMA_RX_RING=3
CONFIG_NUM_OF_WFDMA_TX_RING=0
CONFIG_MTK_WIFI_CONNINFRA_SUPPORT=y
CONFIG_SUPPORT_PRE_ON_PHY_ACTION=y
CONFIG_MTK_WIFI_CONNAC2X_2x2=y
CONFIG_MTK_WIFI_DOWNLOAD_DYN_MEMORY_MAP=y
ccflags-y += -DCFG_POWER_ON_DOWNLOAD_EMI_ROM_PATCH=1
ccflags-y += -DCFG_ROM_PATCH_NO_SEM_CTRL=1
ccflags-y += -DCFG_MTK_WIFI_SW_WFDMA=1
CONFIG_MTK_WIFI_WFDMA_BK_RS=y
ccflags-y += -DCONFIG_MTK_WIFI_HE80
endif

ifneq ($(findstring 7_0,$(MTK_COMBO_CHIP)),)
ccflags-y:=$(filter-out -USOC7_0,$(ccflags-y))
ccflags-y += -DSOC7_0
CONFIG_MTK_WIFI_CONNAC2X=y
CONFIG_MTK_WIFI_11AX_SUPPORT=y
CONFIG_MTK_WIFI_6G_SUPPORT=y
CONFIG_MTK_WIFI_TWT_SUPPORT=y
CONFIG_MTK_WIFI_TWT_STA_DIRECT_TEARDOWN=y
CONFIG_MTK_WIFI_TWT_SMART_STA=n
CONFIG_MTK_TWT_STA_CNM=n
CONFIG_MTK_WIFI_TWT_HOTSPOT_SUPPORT=n
CONFIG_MTK_WIFI_TWT_HOTSPOT_AC_SUPPORT=n
CONFIG_NUM_OF_WFDMA_RX_RING=2
CONFIG_NUM_OF_WFDMA_TX_RING=0
CONFIG_MTK_WIFI_CONNINFRA_SUPPORT=y
CONFIG_SUPPORT_PRE_ON_PHY_ACTION=y
CONFIG_MTK_WIFI_CONNAC2X_2x2=y
CONFIG_MTK_WIFI_DOWNLOAD_DYN_MEMORY_MAP=y
CONFIG_MTK_WIFI_POWER_THROTTLING=y
CONFIG_MTK_WIFI_PKT_OFLD_SUPPORT=y
CONFIG_MTK_WIFI_APF_SUPPORT=y
CONFIG_MTK_WIFI_NAN=y
CONFIG_MTK_WIFI_CONNFEM_SUPPORT=y
CONFIG_SNIFFER_RADIOTAP=y

ifneq ($(filter 6886, $(WLAN_CHIP_ID)),)
    CONFIG_RX_NAPI_THREADED=y
else
    CONFIG_RX_NAPI_THREADED=n
endif

ifeq ($(WLAN_CONNAC3_DEV), yes)
CONFIG_MTK_WIFI_TRX_DIRECT=y
CONFIG_NOT_CLR_FREE_MSDU_IN_DEACTIVE_NETWORK=y
else
    ifneq ($(filter 6983 6886, $(WLAN_CHIP_ID)),)
        ccflags-y += -DCFG_WLAN_LK_FWDL_SUPPORT=1
        ccflags-y += -DCFG_WLAN_ATF_SUPPORT=1
    endif
endif

ccflags-y += -DCFG_POWER_ON_DOWNLOAD_EMI_ROM_PATCH=1
ccflags-y += -DCFG_ROM_PATCH_NO_SEM_CTRL=1
ccflags-y += -DCFG_SUPPORT_BW160
CONFIG_MTK_WIFI_WFDMA_BK_RS=y
ccflags-y += -DCONFIG_MTK_WIFI_HE160
ccflags-y += -DCFG_SUPPORT_SET_IPV6_NETWORK=1
endif

ifneq ($(filter BELLWETHER,$(MTK_COMBO_CHIP)),)
ccflags-y:=$(filter-out -UBELLWETHER,$(ccflags-y))
ccflags-y += -DBELLWETHER
CONFIG_MTK_MDDP_SUPPORT=
CONFIG_MTK_CONNSYS_DEDICATED_LOG_PATH := n
CONFIG_MTK_WIFI_11AX_SUPPORT=y
CONFIG_MTK_WIFI_11BE_SUPPORT=y
CONFIG_MTK_WIFI_11BE_MLO_SUPPORT=y
CONFIG_MTK_WIFI_APS_SUPPORT=y
CONFIG_MTK_WIFI_CONNAC3X=y
CONFIG_NUM_OF_WFDMA_RX_RING=5
CONFIG_NUM_OF_WFDMA_TX_RING=0
CONFIG_RX_ZERO_COPY=y
ifeq ($(MTK_ANDROID_WMT), y)
ifneq ($(CONFIG_PAGE_POOL),)
CONFIG_RX_PAGE_POOL=y
endif
endif
CONFIG_MTK_WIFI_UNIFIED_COMMND_SUPPORT=y
CONFIG_MTK_WIFI_TWT_SUPPORT=y
CONFIG_MTK_WIFI_TWT_STA_DIRECT_TEARDOWN=y
CONFIG_MTK_WIFI_TWT_SMART_STA=n
CONFIG_MTK_WIFI_TWT_HOTSPOT_SUPPORT=n
CONFIG_MTK_WIFI_TWT_HOTSPOT_AC_SUPPORT=n
CONFIG_MTK_WIFI_BTWT_SUPPORT=y
CONFIG_MTK_WIFI_11BE_ML_TWT_SUPPORT=y
CONFIG_MTK_TWT_STA_CNM=n
CONFIG_ROM_CODE_DOWNLOAD=y
CONFIG_MLD_LINK_MAX=2
CONFIG_DBDC_MODE=1
CONFIG_MTK_WIFI_6G_SUPPORT=y
CONFIG_NSS=4
CONFIG_BAND_NUM=3
CONFIG_SUPPORT_FORCE_ALTX=y
CONFIG_MTK_WIFI_TRX_DIRECT=y
CONFIG_NOT_CLR_FREE_MSDU_IN_DEACTIVE_NETWORK=y
ifeq ($(MTK_ANDROID_WMT), y)
    ifeq ($(WLAN_CONNAC3_DEV), yes)
        CONFIG_MTK_WIFI_PCIE_SUPPORT=y
    endif
endif
ccflags-y += -DCFG_WIFI_SW_WTBL_SEARCH_FAIL=0
ccflags-y += -DCONFIG_MTK_WIFI_BW320
ccflags-y += -DCFG_MLO_LINK_PLAN_MODE=1
CONFIG_MTK_WIFI_WFDMA_BK_RS=y
endif

ifneq ($(filter MT6639,$(MTK_COMBO_CHIP)),)
ccflags-y:=$(filter-out -UMT6639,$(ccflags-y))
ccflags-y += -DMT6639
ifeq ($(MTK_ANDROID_WMT), y)
    CONFIG_MTK_WIFI_CONNFEM_SUPPORT=y
    CONFIG_MTK_WIFI_POWER_THROTTLING=y
    CONFIG_MTK_WIFI_PKT_OFLD_SUPPORT=y
    CONFIG_MTK_WIFI_APF_SUPPORT=y
    CONFIG_SNIFFER_RADIOTAP=y
    ifneq ($(CONFIG_PAGE_POOL),)
        CONFIG_RX_PAGE_POOL=y
    endif
    CONFIG_MTK_WIFI_PCIE_SUPPORT=y
    CONFIG_MTK_WIFI_CONNV3_SUPPORT=y
    CONFIG_MTK_WIFI_PCIE_MSI_SUPPORT=y
    CONFIG_MTK_WIFI_FW_LOG_EMI=y
    CONFIG_SUPPORT_STATS_ONE_CMD=y
    ifeq ($(call kver_ge,5,15),1)
        CONFIG_MTK_WIFI_SUPPORT_VOLT_INFO=y
    endif
    CONFIG_WIFI_COREDUMP_SUPPORT=y
    CONFIG_MTK_WIFI_VOLTAGE_CONTROL_UDS=y
    ccflags-y += -DCFG_ROM_PATCH_NO_SEM_CTRL=1
    ccflags-y += -DCFG_SUPPORT_SET_IPV6_NETWORK=1
    CONFIG_WFDMA_AP_MSI_NUM=8
    ifeq ($(CONFIG_WLAN_MT6985_MP2), y)
        CONFIG_SUPPORT_WIFI_SW_EMI_RING=y
        CONFIG_SUPPORT_WIFI_EN_SW_EMI_READ=y
        CONFIG_SUPPORT_WIFI_WFDMA_TX_RING_BK_RS=y
        CONFIG_P2P_UAPSD_SUPPORT=n
        CONFIG_SUPPORT_PCIE_GEN_SWITCH=y
        CONFIG_SUPPORT_WIFI_MDDP_NO_MMIO_READ=y
        CONFIG_SUPPORT_WIFI_MCIF_NO_MMIO_READ=y
        CONFIG_SUPPORT_WIFI_DYNAMIC_PAGE_POOL=y
        CONFIG_DYNAMIC_RFB_ADJUSTMENT=y
        ccflags-y += -DCFG_WFD_SCC_BALANCE_DEF_ENABLE=1
        CONFIG_SUPPORT_WIFI_SLEEP_COUNT=y
        CONFIG_MTK_WIFI_AER_L05_RESET=y
        ifeq ($(CONFIG_WLAN_MT6985_MP2_LP), y)
        endif
    endif
endif
ifneq ($(CONFIG_MTK_COMBO_WIFI_HIF), none)
    CONFIG_MTK_WIFI_NAN=y
else
    CONFIG_MTK_WIFI_NAN=n
endif
CONFIG_MTK_WIFI_TRX_DIRECT=y
ccflags-y += -DCFG_WIFI_SW_WTBL_SEARCH_FAIL=0
CONFIG_MTK_WIFI_WFDMA_BK_RS=y
ccflags-y += -DCFG_SUPPORT_GLOBAL_AAD_NONCE
ccflags-y += -DCFG_SUPPORT_PRE_WIFI7
ccflags-y += -DCONFIG_MTK_WIFI_BW320
ccflags-y += -DCFG_USB_RX_PADDING_CSO_LEN=12
ccflags-y += -DCFG_WIFI_TX_FIXED_RATE_NO_VTA=1
endif

ifneq ($(filter MT6655,$(MTK_COMBO_CHIP)),)
ccflags-y:=$(filter-out -UMT6655,$(ccflags-y))
ccflags-y += -DMT6655
ifeq ($(MTK_ANDROID_WMT), y)
    CONFIG_MTK_WIFI_CONNFEM_SUPPORT=y
    ifneq ($(CONFIG_PAGE_POOL),)
        CONFIG_RX_PAGE_POOL=y
    endif
endif
ifneq ($(CONFIG_MTK_COMBO_WIFI_HIF), none)
    CONFIG_MTK_WIFI_NAN=y
else
    CONFIG_MTK_WIFI_NAN=n
endif
ifeq ($(MTK_ANDROID_WMT), y)
    #CONFIG_MTK_WIFI_PCIE_SUPPORT=y
    #CONFIG_MTK_WIFI_CONNV3_SUPPORT=y
endif
ccflags-y += -DCFG_WIFI_SW_WTBL_SEARCH_FAIL=0
CONFIG_MTK_WIFI_WFDMA_BK_RS=y
ccflags-y += -DCONFIG_MTK_WIFI_HE160
ccflags-y += -DCFG_USB_RX_PADDING_CSO_LEN=12
endif

ifneq ($(filter MT7990,$(MTK_COMBO_CHIP)),)
ccflags-y:=$(filter-out -UMT7990,$(ccflags-y))
ccflags-y += -DMT7990
CONFIG_MTK_MDDP_SUPPORT=
CONFIG_MTK_WIFI_11AX_SUPPORT=y
CONFIG_MTK_WIFI_11BE_SUPPORT=y
CONFIG_MTK_WIFI_11BE_MLO_SUPPORT=y
CONFIG_MTK_WIFI_APS_SUPPORT=y
CONFIG_MTK_WIFI_CONNAC3X=y
CONFIG_NUM_OF_WFDMA_RX_RING=5
CONFIG_NUM_OF_WFDMA_TX_RING=0
CONFIG_MTK_WIFI_UNIFIED_COMMND_SUPPORT=y
CONFIG_MTK_WIFI_TWT_SUPPORT=y
CONFIG_MTK_WIFI_TWT_STA_DIRECT_TEARDOWN=y
CONFIG_MTK_WIFI_TWT_SMART_STA=n
CONFIG_MTK_WIFI_TWT_HOTSPOT_SUPPORT=n
CONFIG_MTK_WIFI_TWT_HOTSPOT_AC_SUPPORT=n
CONFIG_MTK_WIFI_BTWT_SUPPORT=y
CONFIG_MTK_WIFI_11BE_ML_TWT_SUPPORT=y
CONFIG_MTK_TWT_STA_CNM=n
CONFIG_MLD_LINK_MAX=2
CONFIG_DBDC_MODE=1
CONFIG_MTK_WIFI_6G_SUPPORT=y
CONFIG_NSS=4
CONFIG_BAND_NUM=3
ccflags-y += -DCONFIG_MTK_WIFI_BW320
ccflags-y += -DCFG_MLO_LINK_PLAN_MODE=1
CONFIG_MTK_WIFI_WFDMA_BK_RS=y
endif

# ===== Before is project setting =====
ifeq ($(WM_RAM),ce)
    ccflags-y += -DCONFIG_WM_RAM_TYPE=1
    ifneq ($(wildcard $(CFG_DIR)/${MTK_COMBO_CHIP}/ce/defconfig),)
        include $(CFG_DIR)/${MTK_COMBO_CHIP}/ce/defconfig
    endif
else
    ccflags-y += -DCONFIG_WM_RAM_TYPE=0
    ifneq ($(wildcard $(CFG_DIR)/${MTK_COMBO_CHIP}/mobile/defconfig),)
        include $(CFG_DIR)/${MTK_COMBO_CHIP}/mobile/defconfig
    endif
endif
# ===== Below will add compile flag based on project setting =====

ifeq ($(CONFIG_SUPPORT_WIFI_SW_EMI_RING), y)
    ccflags-y += -DCFG_MTK_WIFI_SW_EMI_RING=1
    ifeq ($(CONFIG_SUPPORT_WIFI_EN_SW_EMI_READ), y)
        ccflags-y += -DCFG_MTK_WIFI_EN_SW_EMI_READ=1
    else
        ccflags-y += -DCFG_MTK_WIFI_EN_SW_EMI_READ=0
    endif
else
    ccflags-y += -DCFG_MTK_WIFI_SW_EMI_RING=0
    ccflags-y += -DCFG_MTK_WIFI_EN_SW_EMI_READ=0
endif

ifeq ($(CONFIG_SUPPORT_WIFI_WFDMA_TX_RING_BK_RS), y)
    ccflags-y += -DCFG_MTK_WIFI_WFDMA_TX_RING_BK_RS=1
else
    ccflags-y += -DCFG_MTK_WIFI_WFDMA_TX_RING_BK_RS=0
endif

ifeq ($(CONFIG_SUPPORT_WIFI_MDDP_NO_MMIO_READ), y)
    ccflags-y += -DCFG_SUPPORT_WIFI_MDDP_NO_MMIO_READ=1
else
    ccflags-y += -DCFG_SUPPORT_WIFI_MDDP_NO_MMIO_READ=0
endif

ifeq ($(CONFIG_SUPPORT_WIFI_MCIF_NO_MMIO_READ), y)
    ccflags-y += -DCFG_SUPPORT_WIFI_MCIF_NO_MMIO_READ=1
else
    ccflags-y += -DCFG_SUPPORT_WIFI_MCIF_NO_MMIO_READ=0
endif

ifeq ($(CONFIG_SUPPORT_WIFI_SLEEP_COUNT), y)
    ccflags-y += -DCFG_SUPPORT_WIFI_SLEEP_COUNT=1
else
    ccflags-y += -DCFG_SUPPORT_WIFI_SLEEP_COUNT=0
endif

ifeq ($(CONFIG_SUPPORT_TX_DATA_DELAY), y)
    ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), pcie)
        ccflags-y += -DCFG_SUPPORT_TX_DATA_DELAY=1
    else ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), axi)
        ccflags-y += -DCFG_SUPPORT_TX_DATA_DELAY=1
    else
        ccflags-y += -DCFG_SUPPORT_TX_DATA_DELAY=0
    endif
else
    ccflags-y += -DCFG_SUPPORT_TX_DATA_DELAY=0
endif

ifeq ($(CONFIG_SUPPORT_STATS_ONE_CMD), y)
    ccflags-y += -DCFG_SUPPORT_STATS_ONE_CMD=1
else
    ccflags-y += -DCFG_SUPPORT_STATS_ONE_CMD=0
endif

ifeq ($(CONFIG_MTK_WIFI_SUPPORT_VOLT_INFO), y)
    ccflags-y += -DCFG_VOLT_INFO=1
else
    ccflags-y += -DCFG_VOLT_INFO=0
endif

ifeq ($(CONFIG_MTK_WIFI_VOLTAGE_CONTROL_UDS), y)
    ccflags-y += -DCFG_WIFI_VOLTAGE_CONTROL_UDS=1
else
    ccflags-y += -DCFG_WIFI_VOLTAGE_CONTROL_UDS=0
endif

ifeq ($(CONFIG_RFB_TRACK_SUPPORT), y)
    ccflags-y += -DCFG_RFB_TRACK=1
else
    ccflags-y += -DCFG_RFB_TRACK=1
endif

ifeq ($(CONFIG_MTK_WIFI_TRX_DIRECT), y)
    ifeq (,$(filter CFG_RX_DIRECT,$(PLATFORM_FLAGS)))
        ccflags-y += -DCFG_RX_DIRECT=1
    endif
    ifeq (,$(filter CFG_TX_DIRECT,$(PLATFORM_FLAGS)))
        ccflags-y += -DCFG_TX_DIRECT=1
    endif
    ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), pcie)
        ccflags-y += -DCFG_TX_DIRECT_VIA_HIF_THREAD=1
    endif
endif

ifeq ($(CONFIG_TX_WORK_SUPPORT), y)
    ccflags-y += -DCFG_SUPPORT_TX_WORK=1
else
    ccflags-y += -DCFG_SUPPORT_TX_WORK=0
endif

ifeq ($(CONFIG_RX_WORK_SUPPORT), y)
    ccflags-y += -DCFG_SUPPORT_RX_WORK=1
else
    ccflags-y += -DCFG_SUPPORT_RX_WORK=0
endif

ifeq ($(CONFIG_SUPPORT_TX_FREE_MSDU_WORK), y)
    ccflags-y += -DCFG_SUPPORT_TX_FREE_MSDU_WORK=1
else
    ccflags-y += -DCFG_SUPPORT_TX_FREE_MSDU_WORK=0
endif

ifeq ($(CONFIG_DYNAMIC_RFB_ADJUSTMENT), y)
    ccflags-y += -DCFG_DYNAMIC_RFB_ADJUSTMENT=1
else
    ccflags-y += -DCFG_DYNAMIC_RFB_ADJUSTMENT=0
endif

ifeq ($(CONFIG_MTK_WIFI_CONNAC2X), y)
    ccflags-y += -DCFG_SUPPORT_CONNAC2X=1
else
    ccflags-y += -DCFG_SUPPORT_CONNAC2X=0
endif

ifeq ($(CONFIG_MTK_WIFI_CONNAC2X_2x2), y)
    ccflags-y += -DCFG_SUPPORT_CONNAC2X_2x2=1
else
    ccflags-y += -DCFG_SUPPORT_CONNAC2X_2x2=0
endif

ifeq ($(CONFIG_RX_NAPI_THREADED), y)
    ccflags-y += -DCFG_SUPPORT_RX_NAPI_THREADED=1
else
    ccflags-y += -DCFG_SUPPORT_RX_NAPI_THREADED=0
endif

ifeq ($(CONFIG_MTK_WIFI_CONNAC3X), y)
    ccflags-y += -DCFG_SUPPORT_CONNAC3X=1
    ccflags-y += -DCFG_SUPPORT_DBDC_DOWNGRADE_NSS=0
    ccflags-y += -DCFG_SUPPORT_DBDC_DOWNGRADE_BW=0
    ifeq ($(CONFIG_MTK_WIFI_PMIC_QUERY), y)
        ccflags-y += -DCFG_MTK_WIFI_PMIC_QUERY=1
    endif
    ccflags-y += -DCFG_DRIVER_INITIAL_RUNNING_MODE=5
    ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), pcie)
        ifeq ($(CONFIG_MTK_WIFI_SUPPORT_UDS_FWDL), y)
            ccflags-y += -DCFG_MTK_WIFI_SUPPORT_UDS_FWDL=1
        endif
    endif
else
    ccflags-y += -DCFG_SUPPORT_CONNAC3X=0
    ccflags-y += -DCFG_SUPPORT_DBDC_DOWNGRADE_NSS=1
    ccflags-y += -DCFG_SUPPORT_DBDC_DOWNGRADE_BW=1
    ccflags-y += -DCFG_DRIVER_INITIAL_RUNNING_MODE=5
endif

ifneq ($(CONFIG_MTK_WIFI_CONNAC3X), y)
ifneq ($(CONFIG_MTK_WIFI_CONNAC2X), y)
    ccflags-y += -DCFG_SUPPORT_CONNAC1X=1
endif
endif

ifeq ($(CONFIG_MTK_WIFI_11AX_SUPPORT), y)
    ccflags-y += -DCFG_SUPPORT_802_11AX=1
else
    ccflags-y += -DCFG_SUPPORT_802_11AX=0
endif

ifeq ($(CONFIG_MTK_WIFI_11BE_SUPPORT), y)
    ccflags-y += -DCFG_SUPPORT_802_11BE=1
else
    ccflags-y += -DCFG_SUPPORT_802_11BE=0
endif

ifeq ($(CONFIG_MTK_WIFI_11BE_MLO_SUPPORT), y)
    ccflags-y += -DCFG_SUPPORT_802_11BE_MLO=1
    ifneq ($(CONFIG_MLD_LINK_MAX),)
        ccflags-y += -DCFG_MLD_LINK_MAX=$(CONFIG_MLD_LINK_MAX)
    endif
else
    ccflags-y += -DCFG_SUPPORT_802_11BE_MLO=0
endif

ifeq ($(CONFIG_SUPPORT_BW320), y)
    ccflags-y += -DCFG_SUPPORT_BW320=1
endif

ifneq ($(CONFIG_DBDC_MODE),)
    ccflags-y += -DCFG_DBDC_MODE=$(CONFIG_DBDC_MODE)
endif

ifneq ($(CONFIG_NSS),)
    ccflags-y += -DCFG_NSS=$(CONFIG_NSS)
endif

ifneq ($(CONFIG_BAND_NUM),)
    ccflags-y += -DCONFIG_BAND_NUM=$(CONFIG_BAND_NUM)
else
    ccflags-y += -DCONFIG_BAND_NUM=2
endif

ifeq ($(CONFIG_MTK_WIFI_11AX_SUPPORT), y)
    ifeq ($(CONFIG_MTK_WIFI_6G_SUPPORT), y)
        ccflags-y += -DCFG_SUPPORT_WIFI_6G=1
        ifeq ($(CONFIG_MTK_WIFI_DBDC_6G), y)
            ccflags-y += -DCFG_SUPPORT_WIFI_DBDC6G=1
        else
            ccflags-y += -DCFG_SUPPORT_WIFI_DBDC6G=0
        endif
    else
        ccflags-y += -DCFG_SUPPORT_WIFI_6G=0
        ccflags-y += -DCFG_SUPPORT_WIFI_DBDC6G=0
    endif
else
    ccflags-y += -DCFG_SUPPORT_WIFI_6G=0
    ccflags-y += -DCFG_SUPPORT_WIFI_DBDC6G=0
endif

ifeq ($(CONFIG_MTK_DBDC_SW_FOR_P2P_LISTEN), y)
    ccflags-y += -DCFG_DBDC_SW_FOR_P2P_LISTEN=1
endif

ifeq ($(CONFIG_P2P_UAPSD_SUPPORT), n)
    ccflags-y += -DCFG_P2P_UAPSD_SUPPORT=0
else
    ccflags-y += -DCFG_P2P_UAPSD_SUPPORT=1
endif

ifeq ($(CONFIG_MTK_WIFI_TWT_SUPPORT), y)
    ccflags-y += -DCFG_SUPPORT_TWT=1
    ifeq ($(CONFIG_MTK_WIFI_TWT_HOTSPOT_SUPPORT), y)
        ccflags-y += -DCFG_SUPPORT_TWT_HOTSPOT=1
    else
        ccflags-y += -DCFG_SUPPORT_TWT_HOTSPOT=0
    endif
    ifeq ($(CONFIG_MTK_WIFI_TWT_HOTSPOT_AC_SUPPORT), y)
        ccflags-y += -DCFG_SUPPORT_TWT_HOTSPOT_AC=1
    else
        ccflags-y += -DCFG_SUPPORT_TWT_HOTSPOT_AC=0
    endif
    ifeq ($(CONFIG_MTK_WIFI_TWT_SMART_STA), y)
        ccflags-y += -DCFG_TWT_SMART_STA=1
    else
        ccflags-y += -DCFG_TWT_SMART_STA=0
    endif
    ifeq ($(CONFIG_MTK_WIFI_BTWT_SUPPORT), y)
        ccflags-y += -DCFG_SUPPORT_BTWT=1
    else
        ccflags-y += -DCFG_SUPPORT_BTWT=0
    endif
    ifeq ($(CONFIG_MTK_WIFI_TWT_STA_DIRECT_TEARDOWN), y)
        ccflags-y += -DCFG_TWT_STA_DIRECT_TEARDOWN=1
    else
        ccflags-y += -DCFG_TWT_STA_DIRECT_TEARDOWN=0
    endif
    ifeq ($(CONFIG_MTK_WIFI_11BE_MLO_SUPPORT), y)
        ifeq ($(CONFIG_MTK_WIFI_11BE_ML_TWT_SUPPORT), y)
            ccflags-y += -DCFG_SUPPORT_802_11BE_ML_TWT=1
        else
            ccflags-y += -DCFG_SUPPORT_802_11BE_ML_TWT=0
        endif
    else
        ccflags-y += -DCFG_SUPPORT_802_11BE_ML_TWT=0
    endif
    ifeq ($(CONFIG_MTK_TWT_STA_CNM), y)
        ccflags-y += -DCFG_SUPPORT_TWT_STA_CNM=1
    else
        ccflags-y += -DCFG_SUPPORT_TWT_STA_CNM=0
    endif
else
    ccflags-y += -DCFG_SUPPORT_TWT=0
    ccflags-y += -DCFG_SUPPORT_TWT_HOTSPOT=0
    ccflags-y += -DCFG_SUPPORT_TWT_HOTSPOT_AC=0
    ccflags-y += -DCFG_TWT_SMART_STA=0
    ccflags-y += -DCFG_SUPPORT_BTWT=0
    ccflags-y += -DCFG_SUPPORT_802_11BE_ML_TWT=0
    ccflags-y += -DCFG_SUPPORT_TWT_STA_CNM=0
endif

# TODO
ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), usb)
CONFIG_MTK_HOST_OFFLOAD_SUPPORT=n
endif
ifeq ($(CONFIG_MTK_HOST_OFFLOAD_SUPPORT), y)
    CONFIG_WFDMA_AP_MSI_NUM=8
    ccflags-y += -DCFG_SUPPORT_HOST_OFFLOAD=1
else
    ccflags-y += -DCFG_SUPPORT_HOST_OFFLOAD=0
endif

ifneq ($(CONFIG_NUM_OF_WFDMA_TX_RING),)
    ccflags-y += -DCONFIG_NUM_OF_WFDMA_TX_RING=$(CONFIG_NUM_OF_WFDMA_TX_RING)
endif

ifneq ($(CONFIG_NUM_OF_WFDMA_RX_RING),)
    ccflags-y += -DCONFIG_NUM_OF_WFDMA_RX_RING=$(CONFIG_NUM_OF_WFDMA_RX_RING)
endif

ifneq ($(CONFIG_MTK_COMBO_WIFI_HIF), pcie)
CONFIG_SUPPORT_PCIE_ASPM=n
endif
ifeq ($(CONFIG_SUPPORT_PCIE_ASPM), y)
    ccflags-y += -DCFG_SUPPORT_PCIE_ASPM=1
else
    ccflags-y += -DCFG_SUPPORT_PCIE_ASPM=0
endif
ifeq ($(CONFIG_SUPPORT_PCIE_GEN_SWITCH), y)
    ccflags-y += -DCFG_SUPPORT_PCIE_GEN_SWITCH=1
else
    ccflags-y += -DCFG_SUPPORT_PCIE_GEN_SWITCH=0
endif

ifeq ($(CONFIG_WIFI_DRIVER_OWN_DELAY_SUPPORT), y)
    ccflags-y += -DCFG_MTK_DRIVER_OWN_DELAY=1
else
    ccflags-y += -DCFG_MTK_DRIVER_OWN_DELAY=0
endif

ifeq ($(CONFIG_DEVICE_SUSPEND_BY_MOBILE), y)
    ccflags-y += -DCFG_DEVICE_SUSPEND_BY_MOBILE=1
else
    ccflags-y += -DCFG_DEVICE_SUSPEND_BY_MOBILE=0
endif

ifeq ($(CONFIG_CONTROL_ASPM_BY_FW), y)
    ccflags-y += -DCFG_CONTROL_ASPM_BY_FW=1
else
    ccflags-y += -DCFG_CONTROL_ASPM_BY_FW=0
endif

ifeq ($(CONFIG_RX_ZERO_COPY), y)
    ccflags-y += -DCFG_SUPPORT_RX_ZERO_COPY=1
    ifeq ($(CONFIG_RX_PAGE_POOL), y)
        ccflags-y += -DCFG_SUPPORT_RX_PAGE_POOL=1
        ifeq ($(CONFIG_SUPPORT_WIFI_DYNAMIC_PAGE_POOL), y)
            ccflags-y += -DCFG_SUPPORT_DYNAMIC_PAGE_POOL=1
        else
            ccflags-y += -DCFG_SUPPORT_DYNAMIC_PAGE_POOL=0
        endif
    else
        ccflags-y += -DCFG_SUPPORT_RX_PAGE_POOL=0
        ccflags-y += -DCFG_SUPPORT_DYNAMIC_PAGE_POOL=0
    endif
else
    ccflags-y += -DCFG_SUPPORT_RX_ZERO_COPY=0
    ccflags-y += -DCFG_SUPPORT_RX_PAGE_POOL=0
    ccflags-y += -DCFG_SUPPORT_DYNAMIC_PAGE_POOL=0
endif

ifeq ($(CONFIG_SUPPORT_FLUSH_RX_REORDERING), y)
    ccflags-y += -DCFG_SUPPORT_RX_FLUSH_REORDERING=1
else
    ccflags-y += -DCFG_SUPPORT_RX_FLUSH_REORDERING=0
endif

ifeq ($(WIFI_ENABLE_GCOV), y)
    GCOV_PROFILE := y
endif

ifeq ($(CONFIG_SNIFFER_RADIOTAP), y)
    ccflags-y += -DCFG_SUPPORT_SNIFFER_RADIOTAP
    ccflags-y += -DCFG_SUPPORT_PDMA_SCATTER
endif

ifneq ($(filter 6765, $(WLAN_CHIP_ID)),)
    ccflags-y += -DCFG_SUPPORT_DUAL_STA=0
else ifeq ($(CONFIG_MTK_TC10_FEATURE), y)
    ccflags-y += -DCFG_SUPPORT_DUAL_STA=0
else
    ccflags-y += -DCFG_SUPPORT_DUAL_STA=1
endif

ifneq ($(filter 6779, $(WLAN_CHIP_ID)),)
    ccflags-y += -DCFG_FORCE_AP1NSS
endif

ifeq ($(MTK_ANDROID_WMT), y)
    ccflags-y += -DCFG_MTK_ANDROID_WMT=1
    WMT_SUPPORT := y
    CONFIG_AOSP_LINK_LAYER_STATS_SUPPORT=y
    CONFIG_AOSP_MDDP_LINK_LAYER_STATS_SUPPORT=y
    CONFIG_TX_DELAY_STATS_SUPPORT=y
    CONFIG_SUPPORT_SEPARATE_TXS_PID_POOL=y
    CONFIG_MSCS_FAST_PATH_SUPPORT=y
    ccflags-y += -DCFG_MTK_WIFI_CUSTOM_MDNS_MC_ADDR
else ifneq ($(filter MT6632,$(MTK_COMBO_CHIP)),)
    ccflags-y += -DCFG_MTK_ANDROID_WMT=1
    WMT_SUPPORT := y
else
    ccflags-y += -DCFG_MTK_ANDROID_WMT=0
    WMT_SUPPORT := n
endif

ifeq ($(CONFIG_MTK_WIFI_UNIFIED_COMMND_SUPPORT), y)
    ccflags-y += -DCFG_SUPPORT_UNIFIED_COMMAND
endif

ifeq ($(CONFIG_MTK_FPGA_PLATFORM), y)
    ccflags-y += -DCFG_MTK_FPGA_PLATFORM=1
else
    ccflags-y += -DCFG_MTK_FPGA_PLATFORM=0
endif

ifeq ($(CONFIG_MTK_WIFI_CONNFEM_SUPPORT), y)
ccflags-y += -DCFG_SUPPORT_CONNFEM=1
ccflags-y += -I$(src)/../connfem/include
else
ccflags-y += -DCFG_SUPPORT_CONNFEM=0
endif

ifeq ($(CONFIG_MTK_WIFI_CONNV3_SUPPORT), y)
    ccflags-y += -DCFG_MTK_WIFI_CONNV3_SUPPORT=1
    ifneq ($(wildcard $(src)/../conninfra),)
        CONFIG_MTK_WIFI_CONNV3_PATH ?= $(src)/../conninfra
        KBUILD_EXTRA_SYMBOLS += $(CONFIG_MTK_WIFI_CONNV3_PATH)/Module.symvers
    else
        CONFIG_MTK_WIFI_CONNV3_PATH ?= $(TOP)/vendor/mediatek/kernel_modules/connectivity/conninfra
    endif
    $(info $$CONFIG_MTK_WIFI_CONNV3_PATH is [${CONFIG_MTK_WIFI_CONNV3_PATH}])
    ccflags-y += -I$(CONFIG_MTK_WIFI_CONNV3_PATH)/include
    ccflags-y += -I$(CONFIG_MTK_WIFI_CONNV3_PATH)/base/include
    ccflags-y += -I$(CONFIG_MTK_WIFI_CONNV3_PATH)/conn_drv/connv3/debug_utility
    ccflags-y += -I$(CONFIG_MTK_WIFI_CONNV3_PATH)/conn_drv/connv3/debug_utility/include
    ccflags-y += -I$(CONFIG_MTK_WIFI_CONNV3_PATH)/conn_drv/connv3/debug_utility/connsyslog

    ccflags-y += -I$(TOP)/vendor/mediatek/kernel_modules/connectivity/conninfra/include
else ifeq ($(CONFIG_MTK_WIFI_CONNINFRA_SUPPORT), y)
    CONFIG_WIFI_COREDUMP_SUPPORT=y
    ccflags-y += -DCFG_SUPPORT_CONNINFRA=1
    ccflags-y += -DCFG_ANDORID_CONNINFRA_SUPPORT=1
    ccflags-y += -I$(TOP)/vendor/mediatek/kernel_modules/connectivity/conninfra/include
    ccflags-y += -DCFG_ANDORID_CONNINFRA_COREDUMP_SUPPORT=1
    ccflags-y += -DCFG_MTK_CONNSYS_DEDICATED_LOG_PATH
    ccflags-y += -I$(TOP)/vendor/mediatek/kernel_modules/connectivity/conninfra/include
    ccflags-y += -I$(TOP)/vendor/mediatek/kernel_modules/connectivity/conninfra/platform/include
    ccflags-y += -I$(TOP)/vendor/mediatek/kernel_modules/connectivity/conninfra/base/include

    # conninfra path
    ccflags-y += -I$(TOP)/vendor/mediatek/kernel_modules/connectivity/conninfra/debug_utility
    ccflags-y += -I$(TOP)/vendor/mediatek/kernel_modules/connectivity/conninfra/debug_utility/include
    ccflags-y += -I$(TOP)/vendor/mediatek/kernel_modules/connectivity/conninfra/debug_utility/connsyslog
    ccflags-y += -I$(TOP)/vendor/mediatek/kernel_modules/connectivity/conninfra/debug_utility/coredump
    ccflags-y += -I$(TOP)/vendor/mediatek/kernel_modules/connectivity/conninfra/debug_utility/coredump/platform/include
    ccflags-y += -I$(TOP)/vendor/mediatek/kernel_modules/connectivity/conninfra/debug_utility/metlog

    # connv2 path
    ccflags-y += -I$(TOP)/vendor/mediatek/kernel_modules/connectivity/conninfra/conn_drv/connv2/debug_utility
    ccflags-y += -I$(TOP)/vendor/mediatek/kernel_modules/connectivity/conninfra/conn_drv/connv2/debug_utility/include
    ccflags-y += -I$(TOP)/vendor/mediatek/kernel_modules/connectivity/conninfra/conn_drv/connv2/debug_utility/connsyslog
    ccflags-y += -I$(TOP)/vendor/mediatek/kernel_modules/connectivity/conninfra/conn_drv/connv2/debug_utility/coredump
    ccflags-y += -I$(TOP)/vendor/mediatek/kernel_modules/connectivity/conninfra/conn_drv/connv2/debug_utility/coredump/platform/include
    ccflags-y += -I$(TOP)/vendor/mediatek/kernel_modules/connectivity/conninfra/conn_drv/connv2/debug_utility/metlog
    ccflags-y += -I$(TOP)/vendor/mediatek/kernel_modules/connectivity/wlan/adaptor
else
    CONFIG_WIFI_COREDUMP_SUPPORT=n
    ccflags-y += -DCFG_SUPPORT_CONNINFRA=0
    ccflags-y += -DCFG_ANDORID_CONNINFRA_SUPPORT=0
    ccflags-y += -DCFG_ANDORID_CONNINFRA_COREDUMP_SUPPORT=0
    ifeq ($(WMT_SUPPORT), y)
        ccflags-y += -I$(TOP)/vendor/mediatek/kernel_modules/connectivity/common/common_main/include
        ccflags-y += -I$(TOP)/vendor/mediatek/kernel_modules/connectivity/common/common_main/linux/include
        ccflags-y += -I$(TOP)/vendor/mediatek/kernel_modules/connectivity/common/debug_utility
        ifeq ($(CONFIG_MTK_CONN_LTE_IDC_SUPPORT),y)
            ccflags-y += -DWMT_IDC_SUPPORT=1
        else
            ccflags-y += -DWMT_IDC_SUPPORT=0
        endif
        ccflags-y += -DMTK_WCN_WMT_STP_EXP_SYMBOL_ABSTRACT
    endif
endif

ifeq ($(CONFIG_MTK_CONNSYS_DEDICATED_LOG_PATH), y)
    ccflags-y += -DCFG_MTK_CONNSYS_DEDICATED_LOG_PATH
endif

ifeq ($(CONFIG_WIFI_COREDUMP_SUPPORT), y)
    ccflags-y += -DCFG_WIFI_COREDUMP_SUPPORT=1
else
    ccflags-y += -DCFG_WIFI_COREDUMP_SUPPORT=0
endif

ifeq ($(CONFIG_SUPPORT_THERMAL_QUERY), y)
    ccflags-y += -DCFG_SUPPORT_THERMAL_QUERY=1
else
    ccflags-y += -DCFG_SUPPORT_THERMAL_QUERY=0
endif

ifeq ($(CONFIG_SUPPORT_BTM_OFFLOAD), y)
    ccflags-y += -DCFG_SUPPORT_BTM_OFFLOAD=1
else
    ccflags-y += -DCFG_SUPPORT_BTM_OFFLOAD=0
endif

ifeq ($(CONFIG_MTK_WIFI_POWER_THROTTLING), y)
ccflags-y += -DCFG_SUPPORT_POWER_THROTTLING=1
ccflags-y += -I$(srctree)/drivers/misc/mediatek/connectivity/power_throttling
else
ccflags-y += -DCFG_SUPPORT_POWER_THROTTLING=0
endif

ifeq ($(MTK_ANDROID_EMI), y)
    ccflags-y += -DCFG_MTK_ANDROID_EMI=1
else
    ccflags-y += -DCFG_MTK_ANDROID_EMI=0
endif

ifeq ($(CONFIG_SUPPORT_SINGLE_FW_BINARY), y)
    ccflags-y += -DCFG_SUPPORT_SINGLE_FW_BINARY=1
else
    ccflags-y += -DCFG_SUPPORT_SINGLE_FW_BINARY=0
endif

ifneq ($(WIFI_IP_SET),)
    ccflags-y += -DCFG_WIFI_IP_SET=$(WIFI_IP_SET)
else
    ccflags-y += -DCFG_WIFI_IP_SET=1
endif

ifneq ($(filter MTK_WCN_REMOVE_KERNEL_MODULE,$(KBUILD_SUBDIR_CCFLAGS)),)
    ccflags-y += -DCFG_BUILT_IN_DRIVER=1
else
    ccflags-y += -DCFG_BUILT_IN_DRIVER=0
endif

ifneq ($(findstring UT_TEST_MODE,$(MTK_COMBO_CHIP)),)
ccflags-y:=$(filter-out -UUT_TEST_MODE,$(ccflags-y))
ccflags-y += -DUT_TEST_MODE
endif

CONFIG_MTK_WIFI_MCC_SUPPORT=y
ifeq ($(CONFIG_MTK_WIFI_MCC_SUPPORT), y)
    ccflags-y += -DCFG_SUPPORT_CHNL_CONFLICT_REVISE=0
else
    ccflags-y += -DCFG_SUPPORT_CHNL_CONFLICT_REVISE=1
endif

ifeq ($(CONFIG_MTK_AEE_FEATURE), y)
    ccflags-y += -DCFG_SUPPORT_AEE=1
else
    ccflags-y += -DCFG_SUPPORT_AEE=0
endif

# Disable ASSERT() for user load, enable for others
ifneq ($(TARGET_BUILD_VARIANT),user)
    ccflags-y += -DBUILD_QA_DBG=1
else
    ccflags-y += -DBUILD_QA_DBG=0
endif

ifeq ($(CONFIG_MTK_COMBO_WIFI),y)
    ccflags-y += -DCFG_WPS_DISCONNECT=1
endif

ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), sdio)
    ccflags-y += -D_HIF_SDIO=1
else ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), pcie)
    ccflags-y += -D_HIF_PCIE=1
    ifeq ($(call kver_ge,4,8),1)
        ifeq ($(CONFIG_MTK_WIFI_PCIE_MSI_SUPPORT), y)
            ccflags-y += -DCFG_MTK_WIFI_PCIE_MSI_SUPPORT=1
            ifneq ($(CONFIG_WFDMA_AP_MSI_NUM),)
                ccflags-y += -DCFG_WFDMA_AP_MSI_NUM=$(CONFIG_WFDMA_AP_MSI_NUM)
            endif
        endif
    endif
    ifeq ($(CONFIG_MTK_WIFI_PCIE_SUPPORT), y)
        ccflags-y += -DCFG_MTK_WIFI_PCIE_SUPPORT
    endif
else ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), usb)
    ccflags-y += -D_HIF_USB=1
else ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), axi)
    ccflags-y += -D_HIF_AXI=1
else ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), ut)
    # Increase frame size to 2048 because of 'cfg80211_connect_result' exceed stack size
    ccflags-y += -D_HIF_UT=1 -Wno-unused-function -Wno-unused-variable -Wframe-larger-than=2048
else ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), none)
	ccflags-y += -D_HIF_NONE=1
else
    $(error Unsuppoted HIF=$(CONFIG_MTK_COMBO_WIFI_HIF)!!)
endif

ifeq ($(CONFIG_MTK_WIFI_DOWNLOAD_DYN_MEMORY_MAP), y)
    ccflags-y += -DCFG_DOWNLOAD_DYN_MEMORY_MAP=1
else
    ccflags-y += -DCFG_DOWNLOAD_DYN_MEMORY_MAP=0
endif

ifeq ($(CONFIG_ROM_CODE_DOWNLOAD), y)
    ccflags-y += -DCONFIG_ROM_CODE_DOWNLOAD=1
else
    ccflags-y += -DCONFIG_ROM_CODE_DOWNLOAD=0
endif

ifneq ($(CFG_CFG80211_VERSION),)
VERSION_STR = $(subst \",,$(subst ., , $(subst -, ,$(subst v,,$(CFG_CFG80211_VERSION)))))
$(info VERSION_STR=$(VERSION_STR))
X = $(firstword $(VERSION_STR))
Y = $(word 2 ,$(VERSION_STR))
Z = $(word 3 ,$(VERSION_STR))
ifeq ($Z, )
    Z = 0
endif
VERSION := $(shell echo "$$(( $X * 65536 + $Y * 256 + $Z))" )
ccflags-y += -DCFG_CFG80211_VERSION=$(VERSION)
$(info DCFG_CFG80211_VERSION=$(VERSION))
endif

ccflags-y += -DCFG_SUPPORT_PASSPOINT=1
ccflags-y += -DCFG_HS20_DEBUG=1

MTK_MET_PROFILING_SUPPORT = yes
ifeq ($(MTK_MET_PROFILING_SUPPORT), yes)
    ccflags-y += -DCFG_MET_PACKET_TRACE_SUPPORT=1
else
    ccflags-y += -DCFG_MET_PACKET_TRACE_SUPPORT=0
endif

MTK_MET_TAG_SUPPORT = no
ifeq ($(MTK_MET_TAG_SUPPORT), yes)
    ccflags-y += -DMET_USER_EVENT_SUPPORT
    ccflags-y += -DCFG_MET_TAG_SUPPORT=1
else
    ccflags-y += -DCFG_MET_TAG_SUPPORT=0
endif

MTK_TC3_SUPPORT = no
ifeq ($(MTK_TC3_SUPPORT), yes)
    ccflags-y += -DCFG_TC3_FEATURE=1
    ccflags-y += -DCFG_P2P_CONNECT_ALL_BSS=1
    ccflags-y += -DCFG_P2P_DEFAULT_CLIENT_COUNT=1
    ccflags-y += -DCFG_P2P_SCAN_REPORT_ALL_BSS=1
else
    ccflags-y += -DCFG_TC3_FEATURE=0
endif

ifeq ($(CONFIG_MTK_TC10_FEATURE), y)
    ccflags-y += -DCFG_TC10_FEATURE=1
    ccflags-y += -DCFG_SUPPORT_BTM_OFFLOAD=1
else
    ccflags-y += -DCFG_TC10_FEATURE=0
endif

ifneq ($(CONFIG_MTK_MD1_SUPPORT), )
    ccflags-y += -DCONFIG_MTK_MD_SUPPORT=1
else
    ccflags-y += -DCONFIG_MTK_MD_SUPPORT=0
endif

ifeq ($(CONFIG_MTK_TC1_FEATURE), y)
    ccflags-y += -I$(srctree)/drivers/misc/mediatek/tc1_interface
    ccflags-y += -DCFG_TC1_FEATURE=1
else
    ccflags-y += -DCFG_TC1_FEATURE=0
endif

ifeq ($(CONFIG_MTK_WIFI_NAN), y)
ccflags-y += -DCFG_SUPPORT_NAN=1
else
ccflags-y += -DCFG_SUPPORT_NAN=0
endif

ifeq ($(CFG_NL80211_TESTMODE), y)
ccflags-y += -DCONFIG_NL80211_TESTMODE
endif

ifeq ($(CONFIG_SUPPORT_WIFI_DL_BT_PATCH), y)
    ccflags-y += -DCFG_SUPPORT_WIFI_DL_BT_PATCH=1
else
    ccflags-y += -DCFG_SUPPORT_WIFI_DL_BT_PATCH=0
endif

ifeq ($(CONFIG_SUPPORT_WIFI_DL_ZB_PATCH), y)
    ccflags-y += -DCFG_SUPPORT_WIFI_DL_ZB_PATCH=1
else
    ccflags-y += -DCFG_SUPPORT_WIFI_DL_ZB_PATCH=0
endif

ifeq ($(CFG_SUPPORT_HOST_RX_WM_EVENT_FROM_PSE), y)
    ccflags-y += -DCFG_SUPPORT_HOST_RX_WM_EVENT_FROM_PSE=1
else
    ccflags-y += -DCFG_SUPPORT_HOST_RX_WM_EVENT_FROM_PSE=0
endif

ifeq ($(CONFIG_TX_RSRC_WMM_ENHANCE), y)
    ccflags-y += -DCFG_TX_RSRC_WMM_ENHANCE=1
else
    ccflags-y += -DCFG_TX_RSRC_WMM_ENHANCE=0
endif

ifeq ($(CONFIG_SUPPORT_FORCE_ALTX), y)
ccflags-y += -DCFG_SUPPORT_FORCE_ALTX=1
endif

ifeq ($(CONFIG_SUPPORT_BT_SKU), y)
    ccflags-y += -DCFG_SUPPORT_BT_SKU
endif

ifeq ($(CFG_COALESCING_INTERRUPT), y)
    ccflags-y += -DCFG_COALESCING_INTERRUPT=1
else
    ccflags-y += -DCFG_COALESCING_INTERRUPT=0
endif

ifeq ($(CONFIG_MTK_WIFI_FW_LOG_MMIO), y)
    ccflags-y += -DCFG_MTK_WIFI_FW_LOG_MMIO=1
else
    ccflags-y += -DCFG_MTK_WIFI_FW_LOG_MMIO=0
endif

ifeq ($(CONFIG_MTK_WIFI_FW_LOG_EMI), y)
    ccflags-y += -DCFG_MTK_WIFI_FW_LOG_EMI=1
else
    ccflags-y += -DCFG_MTK_WIFI_FW_LOG_EMI=0
endif

ifeq ($(CONFIG_SUPPORT_DEBUG_SOP), y)
    ccflags-y += -DCFG_SUPPORT_DEBUG_SOP=1
else
    ccflags-y += -DCFG_SUPPORT_DEBUG_SOP=0
endif

ifeq ($(CFG_SUPPORT_HIDDEN_SW_AP), y)
    ccflags-y += -DCFG_SUPPORT_HIDDEN_SW_AP=1
else
    ccflags-y += -DCFG_SUPPORT_HIDDEN_SW_AP=0
endif

ifeq ($(CONFIG_NOT_CLR_FREE_MSDU_IN_DEACTIVE_NETWORK), y)
    ccflags-y += -DCFG_NOT_CLR_FREE_MSDU_IN_DEACTIVE_NETWORK=1
else
    ccflags-y += -DCFG_NOT_CLR_FREE_MSDU_IN_DEACTIVE_NETWORK=0
endif

ifeq ($(WLAN_CONNAC3_DEV), yes)
    ccflags-y += -DCFG_WLAN_CONNAC3_DEV=1
else
    ccflags-y += -DCFG_WLAN_CONNAC3_DEV=0
endif

ifeq ($(call kver_ge,4,4),1)
    ccflags-y += -DCFG_SUPPORT_DATA_STALL=1
    ccflags-y += -DCFG_SUPPORT_BIGDATA_PIP=1
else
    ccflags-y += -DCFG_SUPPORT_DATA_STALL=0
    ccflags-y += -DCFG_SUPPORT_BIGDATA_PIP=0
endif

ifeq ($(call kver_ge,4,17),1)
    ccflags-y += -DCFG_SUPPORT_WPA3=1
else
    ccflags-y += -DCFG_SUPPORT_WPA3=0
endif

ifeq ($(CONFIG_GKI_SUPPORT), y)
    ccflags-y += -DCFG_ENABLE_GKI_SUPPORT=1
else
    ccflags-y += -DCFG_ENABLE_GKI_SUPPORT=0
endif

ifeq ($(CONFIG_MTK_WIFI_AER_L05_RESET), y)
    ccflags-y += -DCFG_MTK_WIFI_AER_L05_RESET=1
else
    ccflags-y += -DCFG_MTK_WIFI_AER_L05_RESET=0
endif

ifeq ($(MODULE_NAME),)
MODULE_NAME := wlan_$(shell echo $(strip $(WLAN_CHIP_ID)) | tr A-Z a-z)_$(CONFIG_MTK_COMBO_WIFI_HIF)
endif

ifeq ($(CONFIG_MTK_COMBO_SLT), golden)
slt_postfix = _mc
MODULE_NAME := $(MODULE_NAME)$(slt_postfix)
endif

ifeq ($(CONFIG_CHIP_RESET_SUPPORT), n)
    ccflags-y += -DCFG_CHIP_RESET_SUPPORT=0
endif

ccflags-y += -DDBG=0
ccflags-y += -I$(src)/os -I$(src)/os/$(os)/include
ccflags-y += -I$(src)/include -I$(src)/include/nic -I$(src)/include/mgmt -I$(src)/include/chips
ifeq ($(CONFIG_MTK_WIFI_NAN), y)
ccflags-y += -I$(src)/include/nan -I$(src)/include/nan/wpa_supp
endif
ifeq ($(CFG_SUPPORT_WIFI_SYSDVT), 1)
ccflags-y += -I$(src)/include/dvt
endif
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/performance/include/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/emi/$(MTK_PLATFORM)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/emi/submodule
ccflags-y += -I$(srctree)/drivers/misc/mediatek/pmic/include/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/power_throttling/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/connectivity/common
ccflags-y += -I$(srctree)/drivers/gpu/drm/mediatek/mediatek_v2/
ccflags-y += -I$(srctree)/drivers/devfreq/
ccflags-y += -I$(srctree)/net
ccflags-y += -I$(TOP)/vendor/mediatek/kernel_modules/connectivity/connfem/include/

ifeq ($(CONFIG_MTK_WIFI_CCCI_SUPPORT), y)
CONFIG_MTK_ECCCI_DRIVER := y
else ifeq ($(CONFIG_MTK_WIFI_CCCI_SUPPORT), n)
CONFIG_MTK_ECCCI_DRIVER :=
endif

ifneq ($(CONFIG_MTK_ECCCI_DRIVER),)
ccflags-y += -DCFG_MTK_CCCI_SUPPORT=1
else
ccflags-y += -DCFG_MTK_CCCI_SUPPORT=0
endif

ifeq ($(CONFIG_MTK_WIFI_MDDP_SUPPORT), y)
CONFIG_MTK_MDDP_SUPPORT := y
else ifeq ($(CONFIG_MTK_WIFI_MDDP_SUPPORT), n)
CONFIG_MTK_MDDP_SUPPORT :=
endif

ifneq ($(CONFIG_MTK_MDDP_SUPPORT),)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/mddp/include/
ccflags-y += -DCFG_MTK_MDDP_SUPPORT=1
else
ccflags-y += -DCFG_MTK_MDDP_SUPPORT=0
endif

ifneq ($(CONFIG_MTK_ADVANCED_80211_MLO),)
ccflags-y += -DCFG_ADVANCED_80211_MLO=1
else
ccflags-y += -DCFG_ADVANCED_80211_MLO=0
endif

ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), sdio)
ccflags-y += -I$(src)/os/$(os)/hif/sdio/include
else ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), pcie)
ccflags-y += -I$(src)/os/$(os)/hif/common/include
ccflags-y += -I$(src)/os/$(os)/hif/pcie/include
ifneq ($(findstring 3_0,$(MTK_COMBO_CHIP)),)
ccflags-y += -I$(src)/include/chips/coda/soc3_0
endif
else ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), axi)
ccflags-y += -I$(src)/os/$(os)/hif/common/include
ccflags-y += -I$(src)/os/$(os)/hif/axi/include
else ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), usb)
ccflags-y += -I$(src)/os/$(os)/hif/usb/include
else ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), ut)
ccflags-y += -I$(src)/test -I$(src)/test/lib/include -I$(src)/test/testcases -I$(src)/test/lib/hif
else ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), none)
ccflags-y += -I$(src)/os/$(os)/hif/none/include
endif

ifneq ($(PLATFORM_FLAGS), )
    ccflags-y += $(PLATFORM_FLAGS)
endif

ifeq ($(CONFIG_MTK_WIFI_ONLY),$(filter $(CONFIG_MTK_WIFI_ONLY),m y))
obj-$(CONFIG_MTK_WIFI_ONLY) += $(MODULE_NAME).o
else
obj-$(CONFIG_MTK_COMBO_WIFI) += $(MODULE_NAME).o
#obj-y += $(MODULE_NAME).o
endif

ifeq ($(CONFIG_WLAN_DRV_BUILD_IN),y)
$(warning $(MODULE_NAME) build-in boot.img)
obj-y += $(MODULE_NAME).o
ccflags-y += -DCONFIG_WLAN_DRV_BUILD_IN=1
else
$(warning $(MODULE_NAME) is kernel module)
obj-m += $(MODULE_NAME).o
ccflags-y += -DCONFIG_WLAN_DRV_BUILD_IN=0
endif

ifeq ($(CONFIG_MTK_WIFI_PKT_OFLD_SUPPORT), y)
ccflags-y += -DCFG_SUPPORT_PKT_OFLD=1
ifeq ($(CONFIG_MTK_WIFI_APF_SUPPORT), y)
ccflags-y += -DCFG_SUPPORT_APF=1
else
ccflags-y += -DCFG_SUPPORT_APF=0
endif
else
ccflags-y += -DCFG_SUPPORT_PKT_OFLD=0
endif

ifeq ($(CONFIG_TX_HIF_PORT_QUEUE_SUPPORT), y)
ccflags-y += -DCFG_TX_HIF_PORT_QUEUE=1
else
ccflags-y += -DCFG_TX_HIF_PORT_QUEUE=0
endif

ifeq ($(CONFIG_AOSP_LINK_LAYER_STATS_SUPPORT), y)
ccflags-y += -DCFG_SUPPORT_LLS=1
endif

ifeq ($(CONFIG_SUPPORT_SEPARATE_TXS_PID_POOL), y)
ccflags-y += -DCFG_SUPPORT_SEPARATE_TXS_PID_POOL=1
endif

ifeq ($(CONFIG_AOSP_MDDP_LINK_LAYER_STATS_SUPPORT), y)
    ifneq ($(filter mt6983, $(CONFIG_WLAN_PLATFORM)),)
        ccflags-y += -DCFG_SUPPORT_LLS_MDDP=0
    else ifeq ($(WLAN_CONNAC3_DEV), yes)
        ccflags-y += -DCFG_SUPPORT_LLS_MDDP=0
    else
        ccflags-y += -DCFG_SUPPORT_LLS_MDDP=1
    endif
else
    ccflags-y += -DCFG_SUPPORT_LLS_MDDP=0
endif

ifeq ($(CONFIG_SUPPORT_TX_HIF_CREDIT_FEATURE), y)
ccflags-y += -DCFG_TX_HIF_CREDIT_FEATURE=1
else
ccflags-y += -DCFG_TX_HIF_CREDIT_FEATURE=0
endif

ifeq ($(CONFIG_TX_DELAY_STATS_SUPPORT), y)
ccflags-y += -DCFG_SUPPORT_TX_LATENCY_STATS=1
endif

ifeq ($(CONFIG_MTK_WIFI_WFD_SCC_BALANCE), y)
ccflags-y += -DCFG_WFD_SCC_BALANCE_SUPPORT=1
else
ccflags-y += -DCFG_WFD_SCC_BALANCE_SUPPORT=0
endif

ifeq ($(CONFIG_MSCS_FAST_PATH_SUPPORT), y)
ccflags-y += -DCFG_FAST_PATH_SUPPORT=1
endif

ifeq ($(CONFIG_MTK_WIFI_WFDMA_BK_RS), y)
    ccflags-y += -DCFG_MTK_WIFI_WFDMA_BK_RS=1
else
    ccflags-y += -DCFG_MTK_WIFI_WFDMA_BK_RS=0
endif

# ---------------------------------------------------
# Directory List
# ---------------------------------------------------
COMMON_DIR  := common/
OS_DIR      := os/$(os)/
HIF_COMMON_DIR := $(OS_DIR)hif/common/
ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), sdio)
HIF_DIR	    := os/$(os)/hif/sdio/
else ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), pcie)
HIF_DIR     := os/$(os)/hif/pcie/
else ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), axi)
HIF_DIR	    := os/$(os)/hif/axi/
else ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), usb)
HIF_DIR	    := os/$(os)/hif/usb/
else ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), ut)
HIF_DIR	    := test/lib/hif/
else ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), none)
HIF_DIR	    := os/$(os)/hif/none/
endif
NIC_DIR     := nic/
MGMT_DIR    := mgmt/
NAN_DIR     := nan/
CHIPS       := chips/
CHIPS_CMM   := $(CHIPS)common/

ifneq ($(CONFIG_WLAN_PLATFORM),)
PLAT_DIR    := os/$(os)/plat/$(CONFIG_WLAN_PLATFORM)/
$(info $$PLAT_DIR is [$(PLAT_DIR)])
endif
SYSDVT_DIR  := dvt/

# ---------------------------------------------------
# Objects List
# ---------------------------------------------------

COMMON_OBJS := 	$(COMMON_DIR)dump.o \
		$(COMMON_DIR)wlan_lib.o \
		$(COMMON_DIR)wlan_oid.o \
		$(COMMON_DIR)wlan_bow.o \
		$(COMMON_DIR)debug.o

NIC_OBJS := 	$(NIC_DIR)nic.o \
		$(NIC_DIR)nic_tx.o \
		$(NIC_DIR)nic_rx.o \
		$(NIC_DIR)nic_pwr_mgt.o \
		$(NIC_DIR)nic_rate.o \
		$(NIC_DIR)cmd_buf.o \
		$(NIC_DIR)que_mgt.o \
		$(NIC_DIR)nic_cmd_event.o \
		$(NIC_DIR)nic_umac.o \
		$(NIC_DIR)radiotap.o

ifeq ($(CONFIG_MTK_WIFI_UNIFIED_COMMND_SUPPORT), y)
NIC_OBJS += $(NIC_DIR)nic_uni_cmd_event.o
endif

ifeq ($(os), none)
OS_OBJS := 	$(OS_DIR)gl_dependent.o \
		$(OS_DIR)gl_init.o \
		$(OS_DIR)gl_kal.o \
		$(OS_DIR)gl_ate_agent.o \
		$(OS_DIR)gl_qa_agent.o \
		$(OS_DIR)gl_csi.o
else
OS_OBJS := 	$(OS_DIR)gl_init.o \
		$(OS_DIR)gl_kal.o \
		$(OS_DIR)gl_bow.o \
		$(OS_DIR)gl_wext.o \
		$(OS_DIR)gl_wext_priv.o \
		$(OS_DIR)gl_ate_agent.o \
		$(OS_DIR)gl_qa_agent.o \
		$(OS_DIR)gl_hook_api.o \
		$(OS_DIR)gl_rst.o \
		$(OS_DIR)gl_cfg80211.o \
		$(OS_DIR)gl_proc.o \
		$(OS_DIR)gl_sys.o \
		$(OS_DIR)gl_vendor.o \
		$(OS_DIR)gl_custom.o \
		$(OS_DIR)platform.o \
		$(OS_DIR)gl_emi.o \
		$(OS_DIR)gl_csi.o \
		$(OS_DIR)gl_sys_lock.o
    ifeq ($(CONFIG_MTK_CONNSYS_DEDICATED_LOG_PATH), y)
        OS_OBJS += $(OS_DIR)gl_fw_log.o
    endif
    ifeq ($(CONFIG_WIFI_COREDUMP_SUPPORT), y)
        OS_OBJS += $(OS_DIR)gl_coredump.o
    endif
endif

MGMT_OBJS := 	$(MGMT_DIR)ais_fsm.o \
		$(MGMT_DIR)aaa_fsm.o \
		$(MGMT_DIR)assoc.o \
		$(MGMT_DIR)auth.o \
		$(MGMT_DIR)bss.o \
		$(MGMT_DIR)cnm.o \
		$(MGMT_DIR)cnm_timer.o \
		$(MGMT_DIR)cnm_mem.o \
		$(MGMT_DIR)hem_mbox.o \
		$(MGMT_DIR)mib.o \
		$(MGMT_DIR)privacy.o \
		$(MGMT_DIR)rate.o \
		$(MGMT_DIR)rlm.o \
		$(MGMT_DIR)rlm_domain.o \
		$(MGMT_DIR)reg_rule.o \
		$(MGMT_DIR)rlm_obss.o \
		$(MGMT_DIR)rlm_protection.o \
		$(MGMT_DIR)rrm.o \
		$(MGMT_DIR)rsn.o \
		$(MGMT_DIR)saa_fsm.o \
		$(MGMT_DIR)scan.o \
		$(MGMT_DIR)scan_fsm.o \
		$(MGMT_DIR)scan_cache.o \
		$(MGMT_DIR)swcr.o \
		$(MGMT_DIR)roaming_fsm.o \
		$(MGMT_DIR)tkip_mic.o \
		$(MGMT_DIR)hs20.o \
		$(MGMT_DIR)tdls.o \
		$(MGMT_DIR)wnm.o \
		$(MGMT_DIR)qosmap.o \
		$(MGMT_DIR)wmm.o \
		$(MGMT_DIR)mddp.o \
		$(MGMT_DIR)thrm.o \
		$(MGMT_DIR)ie_sort.o \
		$(MGMT_DIR)mscs.o \
		$(MGMT_DIR)rtt.o \
		$(MGMT_DIR)mlr.o \
		$(MGMT_DIR)gcm.o \

ifneq ($(os), none)
MGMT_OBJS +=	$(MGMT_DIR)wlan_ring.o \
		$(OS_DIR)gl_sa_log.o \
		$(MGMT_DIR)ics.o
endif


# ---------------------------------------------------
# Chips Objects List
# ---------------------------------------------------
MGMT_OBJS += $(MGMT_DIR)stats.o

CHIPS_OBJS += $(CHIPS_CMM)cmm_asic_common.o \
              $(CHIPS_CMM)fw_log.o

ifeq ($(CONFIG_MTK_WIFI_CONNAC2X), y)
CHIPS_OBJS += $(CHIPS_CMM)cmm_asic_connac2x.o \
              $(CHIPS_CMM)dbg_connac2x.o
NIC_OBJS += $(NIC_DIR)nic_ext_cmd_event.o \
            $(NIC_DIR)nic_txd_v2.o \
            $(NIC_DIR)nic_rxd_v2.o
else ifeq ($(CONFIG_MTK_WIFI_CONNAC3X), y)
CHIPS_OBJS += $(CHIPS_CMM)cmm_asic_connac3x.o \
              $(CHIPS_CMM)dbg_connac3x.o \
              $(CHIPS_CMM)dbg_wtbl_connac3x.o
    ifeq ($(CONFIG_MTK_WIFI_FW_LOG_MMIO), y)
        CHIPS_OBJS += $(CHIPS_CMM)fw_log_mmio.o
    endif
    ifeq ($(CONFIG_MTK_WIFI_FW_LOG_EMI), y)
        CHIPS_OBJS += $(CHIPS_CMM)fw_log_emi.o
    endif
NIC_OBJS += $(NIC_DIR)nic_ext_cmd_event.o \
            $(NIC_DIR)nic_txd_v3.o \
            $(NIC_DIR)nic_rxd_v3.o
else
CHIPS_OBJS += $(CHIPS_CMM)cmm_asic_connac.o \
              $(CHIPS_CMM)dbg_connac.o
NIC_OBJS += $(NIC_DIR)nic_txd_v1.o \
            $(NIC_DIR)nic_rxd_v1.o
endif

ifeq ($(CONFIG_MTK_WIFI_APS_SUPPORT), y)
ccflags-y += -DCFG_SUPPORT_APS=1
MGMT_OBJS += $(MGMT_DIR)aps.o
else
ccflags-y += -DCFG_SUPPORT_APS=0
MGMT_OBJS += $(MGMT_DIR)ap_selection.o
endif

ifeq ($(CONFIG_SUPPORT_PRE_ON_PHY_ACTION), y)
    CHIPS_OBJS += $(CHIPS_CMM)pre_cal.o
    ccflags-y += -DCFG_SUPPORT_PRE_ON_PHY_ACTION=1
else
    ccflags-y += -DCFG_SUPPORT_PRE_ON_PHY_ACTION=0
endif

CHIPS_OBJS += $(CHIPS_CMM)fw_dl.o

ifneq ($(filter MT6632,$(MTK_COMBO_CHIP)),)
CHIPS_OBJS += $(CHIPS)mt6632/mt6632.o
endif
ifneq ($(filter MT7668,$(MTK_COMBO_CHIP)),)
CHIPS_OBJS += $(CHIPS)mt7668/mt7668.o
endif
ifneq ($(filter MT7663,$(MTK_COMBO_CHIP)),)
CHIPS_OBJS += $(CHIPS)mt7663/mt7663.o
endif
ifneq ($(filter CONNAC,$(MTK_COMBO_CHIP)),)
CHIPS_OBJS += $(CHIPS)connac/connac.o
endif
ifneq ($(filter SOC2_1X1,$(MTK_COMBO_CHIP)),)
CHIPS_OBJS += $(CHIPS)soc2_1x1/soc2_1x1.o
endif
ifneq ($(filter SOC2_2X2,$(MTK_COMBO_CHIP)),)
CHIPS_OBJS += $(CHIPS)soc2_2x2/soc2_2x2.o
endif
ifneq ($(findstring MT7915,$(MTK_COMBO_CHIP)),)
CHIPS_OBJS += $(CHIPS)mt7915/mt7915.o
CHIPS_OBJS +=  $(CHIPS)mt7915/dbg_mt7915.o
endif
ifneq ($(findstring 3_0,$(MTK_COMBO_CHIP)),)
CHIPS_OBJS += $(CHIPS)soc3_0/soc3_0.o
CHIPS_OBJS += $(CHIPS)soc3_0/dbg_soc3_0.o
CHIPS_OBJS += $(CHIPS)soc3_0/hal_dmashdl_soc3_0.o
endif
ifneq ($(findstring MT7961,$(MTK_COMBO_CHIP)),)
CHIPS_OBJS += $(CHIPS)mt7961/mt7961.o
CHIPS_OBJS += $(CHIPS)mt7961/dbg_mt7961.o
CHIPS_OBJS += $(CHIPS)mt7961/hal_dmashdl_mt7961.o
CHIPS_OBJS += $(CHIPS)mt7961/hal_wfsys_reset_mt7961.o
endif
ifneq ($(findstring 5_0,$(MTK_COMBO_CHIP)),)
CHIPS_OBJS += $(CHIPS)soc5_0/soc5_0.o
CHIPS_OBJS += $(CHIPS)soc5_0/dbg_soc5_0.o
CHIPS_OBJS += $(CHIPS)soc5_0/hal_dmashdl_soc5_0.o
endif
ifneq ($(findstring 7_0,$(MTK_COMBO_CHIP)),)
CHIPS_OBJS += $(CHIPS)soc7_0/soc7_0.o
CHIPS_OBJS += $(CHIPS)soc7_0/dbg_soc7_0.o
CHIPS_OBJS += $(CHIPS)soc7_0/hal_dmashdl_soc7_0.o
endif

ifneq ($(filter BELLWETHER,$(MTK_COMBO_CHIP)),)
CHIPS_OBJS += $(CHIPS)bellwether/bellwether.o
CHIPS_OBJS += $(CHIPS)bellwether/dbg_bellwether.o
CHIPS_OBJS += $(CHIPS)bellwether/hal_dmashdl_bellwether.o
endif

ifneq ($(filter MT6639,$(MTK_COMBO_CHIP)),)
CHIPS_OBJS += $(CHIPS)mt6639/mt6639.o
CHIPS_OBJS += $(CHIPS)mt6639/dbg_mt6639.o
CHIPS_OBJS += $(CHIPS)mt6639/hal_dmashdl_mt6639.o
endif

ifneq ($(filter MT6655,$(MTK_COMBO_CHIP)),)
CHIPS_OBJS += $(CHIPS)mt6655/mt6655.o
CHIPS_OBJS += $(CHIPS)mt6655/dbg_mt6655.o
CHIPS_OBJS += $(CHIPS)mt6655/hal_dmashdl_mt6655.o
endif

ifneq ($(filter MT7990,$(MTK_COMBO_CHIP)),)
CHIPS_OBJS += $(CHIPS)mt7990/mt7990.o
CHIPS_OBJS += $(CHIPS)mt7990/dbg_mt7990.o
CHIPS_OBJS += $(CHIPS)mt7990/hal_dmashdl_mt7990.o
endif

# ---------------------------------------------------
# P2P Objects List
# ---------------------------------------------------

COMMON_OBJS += $(COMMON_DIR)wlan_p2p.o

NIC_OBJS += $(NIC_DIR)p2p_nic.o

ifneq ($(os), none)
OS_OBJS += $(OS_DIR)gl_p2p.o \
           $(OS_DIR)gl_p2p_cfg80211.o \
           $(OS_DIR)gl_p2p_init.o
endif
OS_OBJS += $(OS_DIR)gl_p2p_kal.o

MGMT_OBJS += $(MGMT_DIR)p2p_dev_fsm.o\
            $(MGMT_DIR)p2p_dev_state.o\
            $(MGMT_DIR)p2p_role_fsm.o\
            $(MGMT_DIR)p2p_role_state.o\
            $(MGMT_DIR)p2p_func.o\
            $(MGMT_DIR)p2p_scan.o\
            $(MGMT_DIR)p2p_ie.o\
            $(MGMT_DIR)p2p_rlm.o\
            $(MGMT_DIR)p2p_assoc.o\
            $(MGMT_DIR)p2p_bss.o\
            $(MGMT_DIR)p2p_rlm_obss.o\
            $(MGMT_DIR)p2p_fsm.o\
            $(MGMT_DIR)p2p_link.o


MGMT_OBJS += $(MGMT_DIR)wapi.o

# ---------------------------------------------------
# NAN Objects List
# ---------------------------------------------------
ifeq ($(CONFIG_MTK_WIFI_NAN), y)
OS_OBJS  += $(OS_DIR)gl_nan.o \
            $(OS_DIR)gl_vendor_nan.o \
            $(OS_DIR)gl_vendor_ndp.o
NAN_OBJS := $(NAN_DIR)nan_dev.o \
            $(NAN_DIR)nanDiscovery.o\
            $(NAN_DIR)nanScheduler.o\
            $(NAN_DIR)nanReg.o\
            $(NAN_DIR)nan_data_engine.o\
            $(NAN_DIR)nan_data_engine_util.o\
            $(NAN_DIR)nan_ranging.o\
            $(NAN_DIR)nan_txm.o

NAN_SEC_OBJS := $(NAN_DIR)nan_sec.o\
                $(NAN_DIR)wpa_supp/FourWayHandShake.o\
                $(NAN_DIR)wpa_supp/src/ap/wpa_auth.o\
                $(NAN_DIR)wpa_supp/src/crypto/sha1.o\
                $(NAN_DIR)wpa_supp/src/crypto/sha1-internal.o\
                $(NAN_DIR)wpa_supp/src/crypto/sha1-prf.o\
                $(NAN_DIR)wpa_supp/src/common/wpa_common.o\
                $(NAN_DIR)wpa_supp/src/crypto/aes-wrap.o\
                $(NAN_DIR)wpa_supp/src/crypto/aes-internal.o\
                $(NAN_DIR)wpa_supp/src/utils/common.o\
                $(NAN_DIR)wpa_supp/src/rsn_supp/wpa.o\
                $(NAN_DIR)wpa_supp/src/crypto/aes-unwrap.o\
                $(NAN_DIR)wpa_supp/src/crypto/aes-internal-enc.o\
                $(NAN_DIR)wpa_supp/src/crypto/aes-internal-dec.o\
                $(NAN_DIR)wpa_supp/src/crypto/sha256.o\
                $(NAN_DIR)wpa_supp/src/crypto/sha256-prf.o\
                $(NAN_DIR)wpa_supp/src/crypto/sha256-internal.o\
                $(NAN_DIR)wpa_supp/wpa_supplicant/wpas_glue.o\
                $(NAN_DIR)wpa_supp/wpa_supplicant/wpa_supplicant.o\
                $(NAN_DIR)wpa_supp/src/ap/wpa_auth_glue.o\
                $(NAN_DIR)wpa_supp/src/crypto/pbkdf2-sha256.o\
                $(NAN_DIR)wpa_supp/src/crypto/sha384-internal.o\
                $(NAN_DIR)wpa_supp/src/crypto/sha512-internal.o\
                $(NAN_DIR)wpa_supp/src/crypto/sha384-prf.o\
                $(NAN_DIR)wpa_supp/src/crypto/sha384.o
endif

# ---------------------------------------------------
# HE Objects List
# ---------------------------------------------------

COMMON_OBJS += $(COMMON_DIR)wlan_he.o

ifeq ($(CONFIG_MTK_WIFI_11AX_SUPPORT), y)
MGMT_OBJS += $(MGMT_DIR)he_ie.o \
             $(MGMT_DIR)he_rlm.o
endif

ifeq ($(CONFIG_MTK_WIFI_11BE_SUPPORT), y)
MGMT_OBJS += $(MGMT_DIR)eht_rlm.o
endif

ifeq ($(CONFIG_MTK_WIFI_11BE_MLO_SUPPORT), y)
MGMT_OBJS += $(MGMT_DIR)mlo.o
endif

ifeq ($(CONFIG_MTK_WIFI_TWT_SUPPORT), y)
MGMT_OBJS += $(MGMT_DIR)twt_req_fsm.o \
             $(MGMT_DIR)twt.o \
             $(MGMT_DIR)twt_planner.o
endif

ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), sdio)
HIF_OBJS :=  $(HIF_DIR)arm.o \
             $(HIF_DIR)sdio.o \
             $(HIF_DIR)hal_api.o \
             $(HIF_DIR)sdio_test_driver_core.o \
             $(HIF_DIR)sdio_test_driver_ops.o
else ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), pcie)
HIF_OBJS :=  $(HIF_COMMON_DIR)hal_pdma.o \
             $(HIF_COMMON_DIR)kal_pdma.o \
             $(HIF_COMMON_DIR)dbg_pdma.o \
             $(HIF_COMMON_DIR)hif_mem.o \
             $(HIF_COMMON_DIR)sw_emi_ring.o \
             $(HIF_DIR)pcie.o \
             $(HIF_DIR)pcie_msi.o
ifeq ($(CONFIG_MTK_HOST_OFFLOAD_SUPPORT), y)
HIF_OBJS +=  $(HIF_COMMON_DIR)hal_offload.o
endif
else ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), axi)
HIF_OBJS :=  $(HIF_COMMON_DIR)hal_pdma.o \
             $(HIF_COMMON_DIR)kal_pdma.o \
             $(HIF_COMMON_DIR)dbg_pdma.o \
             $(HIF_COMMON_DIR)sw_wfdma.o \
             $(HIF_COMMON_DIR)hif_mem.o \
             $(HIF_DIR)axi.o
else ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), usb)
HIF_OBJS :=  $(HIF_DIR)usb.o \
             $(HIF_DIR)hal_api.o
else ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), ut)
HIF_OBJS :=  $(HIF_DIR)ut.o \
             $(HIF_DIR)hal_api.o
else ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), none)
HIF_OBJS :=  $(HIF_DIR)none.o
endif

# ---------------------------------------------------
# Platform Objects List
# ---------------------------------------------------
ifneq ($(PLAT_DIR),)

PLAT_PRIV_C = $(src)/$(PLAT_DIR)plat_priv.c

# search path (out of kernel tree)
IS_EXIST_PLAT_PRIV_C := $(wildcard $(PLAT_PRIV_C))
# search path (build-in kernel tree)
IS_EXIST_PLAT_PRIV_C += $(wildcard $(srctree)/$(PLAT_PRIV_C))

ifneq ($(strip $(IS_EXIST_PLAT_PRIV_C)),)
PLAT_OBJS := $(PLAT_DIR)plat_priv.o
$(MODULE_NAME)-objs  += $(PLAT_OBJS)
endif
endif

# ---------------------------------------------------
# System Dvt Objects List
# ---------------------------------------------------
ifeq ($(CFG_SUPPORT_WIFI_SYSDVT), 1)
SYSDVT_OBJS += $(SYSDVT_DIR)dvt_common.o \
               $(SYSDVT_DIR)dvt_phtput.o

ifeq ($(CFG_SUPPORT_DMASHDL_SYSDVT), 1)
SYSDVT_OBJS += $(SYSDVT_DIR)dvt_dmashdl.o
endif
endif

# ---------------------------------------------------
# Service git List
# ---------------------------------------------------
MTK_WLAN_SERVICE_PATH  := wlan_service/

ifneq ($(CONFIG_MTK_COMBO_WIFI_HIF), none)
ifneq ($(findstring wlan_service,$(MTK_WLAN_SERVICE_PATH)),)
MTK_WLAN_SERVICE=yes
SERVICE_DIR  := $(MTK_WLAN_SERVICE_PATH)
$(info SERVICE_DIR is [{$(MTK_WLAN_SERVICE_PATH)}])
endif
endif

ifeq ($(MTK_WLAN_SERVICE), yes)
ccflags-y += -DCONFIG_WLAN_SERVICE=1
ccflags-y += -DCONFIG_TEST_ENGINE_OFFLOAD=1
ccflags-y += -I$(src)/$(SERVICE_DIR)include
ccflags-y += -I$(src)/$(SERVICE_DIR)service/include
ccflags-y += -I$(src)/$(SERVICE_DIR)glue/osal/include
ccflags-y += -I$(src)/$(SERVICE_DIR)glue/hal/include
$(info $$CCFLAG is [{$(ccflags-y)}])
SERVICE_OBJS := $(SERVICE_DIR)agent/agent.o \
                $(SERVICE_DIR)service/service_test.o \
                $(SERVICE_DIR)service/test_engine.o \
                $(SERVICE_DIR)glue/osal/gen4m/sys_adaption_gen4m.o \
                $(SERVICE_DIR)glue/osal/gen4m/net_adaption_gen4m.o \
                $(SERVICE_DIR)glue/hal/gen4m/operation_gen4m.o
$(MODULE_NAME)-objs  += $(SERVICE_OBJS)
$(info $$MTK_WLAN_SERVICE is [{$(SERVICE_OBJS)}])
else
ccflags-y += -DCONFIG_WLAN_SERVICE=0
ccflags-y += -DCONFIG_TEST_ENGINE_OFFLOAD=0
endif

$(MODULE_NAME)-objs  += $(COMMON_OBJS)
$(MODULE_NAME)-objs  += $(NIC_OBJS)
$(MODULE_NAME)-objs  += $(OS_OBJS)
$(MODULE_NAME)-objs  += $(HIF_OBJS)
$(MODULE_NAME)-objs  += $(MGMT_OBJS)
$(MODULE_NAME)-objs  += $(CHIPS_OBJS)
$(MODULE_NAME)-objs  += $(SYSDVT_OBJS)
$(MODULE_NAME)-objs  += $(NAN_OBJS)
$(MODULE_NAME)-objs  += $(NAN_SEC_OBJS)

ifneq ($(findstring UT_TEST_MODE,$(MTK_COMBO_CHIP)),)
include $(src)/test/ut.make
endif

#
# mtprealloc
#
ifeq ($(CONFIG_MTK_PREALLOC_MEMORY), y)
ccflags-y += -DCFG_PREALLOC_MEMORY
ccflags-y += -I$(src)/prealloc/include
MODULE_NAME_PREALLOC = $(MODULE_NAME)_prealloc
PREALLOC_OBJS := prealloc/prealloc.o
$(MODULE_NAME_PREALLOC)-objs += $(PREALLOC_OBJS)
obj-m += $(MODULE_NAME_PREALLOC).o
endif

#
# mtreset
#
ifeq ($(CONFIG_GKI_SUPPORT), y)
ccflags-y += -DCFG_CHIP_RESET_KO_SUPPORT
ccflags-y += -I$(src)/reset/include
MODULE_NAME_RESET = $(MODULE_NAME)_reset
RESET_OBJS := reset/reset.o
$(MODULE_NAME_RESET)-objs += $(RESET_OBJS)
obj-m += $(MODULE_NAME_RESET).o
endif

#
# Frame size
#
ifeq ($(WMT_SUPPORT), y)
    ifeq ($(CONFIG_FRAME_WARN),)
        ccflags-y += -Wframe-larger-than=2048
    endif
else
    ccflags-y += -Wframe-larger-than=2048
endif
