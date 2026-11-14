
src:=$(if $(filter /%,$(src)),,$(srctree)/)$(src)
$(info src=$(src))


# ---------------------------------------------------
# Kbuild option
# ---------------------------------------------------
ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF),)
ifneq ($(src),)
ifneq ($(srctree),)
CONFIG_MTK_COMBO_WIFI_HIF=pcie
include $(srctree)/$(src)/Makefile.x86
src:=$(srctree)/$(shell echo ${src})
MTK_COMBO_CHIP=MT7961
$(info Kbuild chip: $(MTK_COMBO_CHIP))
$(info Kbuild hif : $(CONFIG_MTK_COMBO_WIFI_HIF))
endif
endif
endif

WM_RAM ?=
CFG_PROJECT ?=
CFG_DIR ?= $(src)/configs

CONFIG_BUILD_MTPREALLOC ?= y
CONFIG_BUILD_WLAN ?= y
CONFIG_BUILD_RESETKO ?= y

# ---------------------------------------------------
# OS option
# ---------------------------------------------------
os=$(CONFIG_MTK_SUPPORT_OS)

ifeq ($(os),)
os=linux
endif

ccflags-y += $(call cc-option,-ffile-prefix-map=$(src)/=)

ifeq ($(os), none)
ccflags-y += -I/usr/include/
ccflags-y += -DCFG_VIRTUAL_OS
ccflags-y += -DCFG_REMIND_IMPLEMENT
endif

$(info os option: $(os))

ifeq ($(os), none)
kver_ge = $(shell echo 1)
else
# script for check the kernel version condition
# example check kernel >= 5.4 : $(call kver_ge,5,4)
kver_ge = $(shell \
echo check_ver | awk '{if($(VERSION) < $(1)) {print 0} else { \
if($(VERSION) > $(1)) {print 1} else { \
if($(PATCHLEVEL) < $(2)) {print 0} else { \
if($(PATCHLEVEL) > $(2)) {print 1} else { \
if($(SUBLEVEL) < $(3)) {print 0} else {print 1} \
}}}}}' \
)
endif
$(info ker ver : $(VERSION), PATCHLEVEL : $(PATCHLEVEL), SUBLEVEL : $(SUBLEVEL))
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

ifeq ($(DRIVER_BUILD_DATE),)
DRIVER_BUILD_DATE=$(shell date +%Y%m%d%H%M%S)
endif
ccflags-y += -DDRIVER_BUILD_DATE='"$(DRIVER_BUILD_DATE)"'
# ---------------------------------------------------
# Compile Options
# ---------------------------------------------------
WLAN_CHIP_LIST:=-UMT6620 -UMT6628 -UMT5931 -UMT6630 -UMT6632 -UMT7663 -UCONNAC -UCONNAC2X2 -UUT_TEST_MODE -UMT7915 -USOC3_0 -UMT7961 -UMT7922 -USOC5_0 -UMT7902 -UMT7926 -UMT6639
# '-D' and '-U' options are processed in the order they are given on the command line.
# All '-imacros file' and '-include file' options are processed after all '-D' and '-U' options.
ccflags-y += $(WLAN_CHIP_LIST)
ccflags-y += -Werror
ccflags-y += -Wall
#workaround: also needed for none LINUX system
# because some of common part code is surrounded with this flag
ccflags-y += -DLINUX

ifneq ($(ANDROID),)
ccflags-y += -DANDROID=$(ANDROID)
endif

ifeq ($(MTK_COMBO_CHIP),)
    override MTK_COMBO_CHIP := MT7961
else ifeq ($(MTK_COMBO_CHIP),MT)
    override MTK_COMBO_CHIP := MT7961
endif

MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))

ifneq ($(MTK_PLATFORM),)
$(info $$MTK_PLATFORM is [${MTK_PLATFORM}])
endif
ifneq ($(WM_RAM),)
$(info $$WM_RAM is [${WM_RAM}])
endif
ifneq ($(CFG_PROJECT),)
$(info $$CFG_PROJECT is [${CFG_PROJECT}])
endif


# ===== Before is project setting =====
ifneq ($(wildcard $(CFG_DIR)/defconfig),)
    include $(CFG_DIR)/defconfig
endif
ifneq ($(wildcard $(CFG_DIR)/$(MTK_COMBO_CHIP)/defconfig),)
    include $(CFG_DIR)/$(MTK_COMBO_CHIP)/defconfig
endif
ifneq ($(WM_RAM),)
ifneq ($(wildcard $(CFG_DIR)/$(MTK_COMBO_CHIP)/$(WM_RAM)/defconfig),)
    include $(CFG_DIR)/$(MTK_COMBO_CHIP)/$(WM_RAM)/defconfig
endif
endif
ifneq ($(CFG_PROJECT),)
ifneq ($(wildcard $(CFG_DIR)/$(MTK_COMBO_CHIP)/$(WM_RAM)/$(CFG_PROJECT)/defconfig),)
    include $(CFG_DIR)/$(MTK_COMBO_CHIP)/$(WM_RAM)/$(CFG_PROJECT)/defconfig
endif
endif

# AP KVR
ifeq ($(CONFIG_MTK_AP_80211KVR_INTERFACE), y)
    CONFIG_WIFI_SUPPORT_GET_NOISE=y
    CONFIG_WIFI_SUPPORT_NOISE_HISTOGRAM=y
    CONFIG_AP_80211KVR_INTERFACE=y
    CONFIG_AP_80211K_SUPPORT=y
    CONFIG_AP_80211V_SUPPORT=y
endif

# ====== Memory Shrink======
ifeq ($(CONFIG_MEM_SHRINK_LEVEL), 1)
    CONFIG_SUPPORT_NVRAM=n
    CONFIG_SUPPORT_ADHOC=n
    CONFIG_BUILD_MOBILE_SKU=n
    CONFIG_SUPPORT_NFC_BEAM_PLUS=n
    CONFIG_MTK_WIFI_TWT_HOTSPOT_SUPPORT=n
    ifneq ($(CONFIG_SUPPORT_SNIFFER_RADIOTAP), y)
        ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), usb)
            CONFIG_SUPPORT_MULTI_SKB_SIZE=y
        endif
    endif
else ifeq ($(CONFIG_MEM_SHRINK_LEVEL), 2)
    CONFIG_SUPPORT_NVRAM=n
    CONFIG_SUPPORT_ADHOC=n
    CONFIG_BUILD_MOBILE_SKU=n
    CONFIG_SUPPORT_NFC_BEAM_PLUS=n
    CONFIG_MTK_WIFI_CSI_DYNAMIC_BUFFER=y
    CONFIG_MTK_WIFI_TWT_HOTSPOT_SUPPORT=n
    ifneq ($(CONFIG_SUPPORT_SNIFFER_RADIOTAP), y)
        ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), usb)
            CONFIG_SUPPORT_MULTI_SKB_SIZE=y
        endif
    endif
else ifeq ($(CONFIG_MEM_SHRINK_LEVEL), 3)
    CONFIG_SUPPORT_NVRAM=n
    CONFIG_SUPPORT_ADHOC=n
    CONFIG_BUILD_MOBILE_SKU=n
    CONFIG_SUPPORT_NFC_BEAM_PLUS=n
    CONFIG_WIFI_ULTRA_RADIO_OFF_CTRL=n
    CONFIG_TEST_PS=n
    CONFIG_MTK_WIFI_CSI_DYNAMIC_BUFFER=y
    CONFIG_MTK_WIFI_TWT_HOTSPOT_SUPPORT=n
    CONFIG_DISABLE_MTK_MET_PROFILING=y
    ifneq ($(CONFIG_SUPPORT_SNIFFER_RADIOTAP), y)
        ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), usb)
            CONFIG_SUPPORT_MULTI_SKB_SIZE=y
        endif
    endif
endif

# ===== Below will add compile flag based on project setting =====
# Example of configuration syntax with default DISABLED:
#     ifeq ($(CONFIG_XXXX), y)
#         ccflags-y += -DCFG_XXXX=1
#     else
#         ccflags-y += -DCFG_XXXX=0
#     endif

# Example of configuration syntax with default ENABLED:
#     ifeq ($(CONFIG_XXXX), n)
#         ccflags-y += -DCFG_XXXX=0
#     else
#         ccflags-y += -DCFG_XXXX=1
#     endif

# Example of configuration syntax with values other than y/n:
#     ifneq ($(CONFIG_XXXX),)
#         ccflags-y += -DCFG_XXXX=$(CONFIG_XXXX)
#     endif

ifeq ($(CONFIG_MTK_WIFI_CSI_DYNAMIC_BUFFER), y)
    ccflags-y += -DCFG_CSI_DYNAMIC_BUFFER=1
else
    ccflags-y += -DCFG_CSI_DYNAMIC_BUFFER=0
endif
ifeq ($(CONFIG_DISABLE_MTK_MET_PROFILING), y)
    ccflags-y += -DCFG_MET_PACKET_TRACE_SUPPORT=0
else
    ccflags-y += -DCFG_MET_PACKET_TRACE_SUPPORT=1
endif
ifeq ($(CONFIG_BUILD_MOBILE_SKU), n)
    ccflags-y += -DCFG_BUILD_MOBILE_SKU=0
else
    ccflags-y += -DCFG_BUILD_MOBILE_SKU=1
endif

ifeq ($(CONFIG_SUPPORT_NVRAM), n)
    ccflags-y += -DCFG_SUPPORT_NVRAM=0
endif

ifeq ($(CONFIG_SUPPORT_ADHOC), n)
    ccflags-y += -DCFG_SUPPORT_ADHOC=0
endif

ifeq ($(CONFIG_SUPPORT_NFC_BEAM_PLUS), n)
    ccflags-y += -DCFG_SUPPORT_NFC_BEAM_PLUS=0
endif
ifeq ($(CONFIG_WIFI_ULTRA_RADIO_OFF_CTRL), n)
    ccflags-y += -DCFG_WIFI_ULTRA_RADIO_OFF_CTRL=0
endif
ifeq ($(CONFIG_TEST_PS), n)
    ccflags-y += -DCFG_TEST_PS=0
endif
ifeq ($(CONFIG_SUPPORT_MULTI_SKB_SIZE), y)
    ccflags-y += -DCFG_SUPPORT_MULTI_SKB_SIZE=1
else
    ccflags-y += -DCFG_SUPPORT_MULTI_SKB_SIZE=0
endif

ifeq ($(CONFIG_SUPPORT_MAGIC_PKT_VENDOR_EVENT), y)
    ccflags-y += -DCFG_SUPPORT_MAGIC_PKT_VENDOR_EVENT=1
else
    ccflags-y += -DCFG_SUPPORT_MAGIC_PKT_VENDOR_EVENT=0
endif

ifeq ($(CONFIG_MTK_COMBO_PLATFORM), x86)
    ccflags-y += -DCFG_BUILD_X86_PLATFORM=1
else
    ccflags-y += -DCFG_BUILD_X86_PLATFORM=0
endif

ifeq ($(CONFIG_SUPPORT_PROC_GET_WAKEUP_REASON), y)
    ccflags-y += -DCFG_SUPPORT_PROC_GET_WAKEUP_REASON=1
else
    ccflags-y += -DCFG_SUPPORT_PROC_GET_WAKEUP_REASON=0
endif

ifneq ($(CONFIG_DFS_CHSW_FORCE_BW20),)
    ccflags-y += -DCFG_DFS_CHSW_FORCE_BW20=1
else
    ccflags-y += -DCFG_DFS_CHSW_FORCE_BW20=0
endif

ifeq ($(CONFIG_SUPPORT_ROAMING), n)
    ccflags-y += -DCFG_SUPPORT_ROAMING=0
else
    ccflags-y += -DCFG_SUPPORT_ROAMING=1
endif

ifeq ($(CONFIG_SUPPORT_REG_BY_USER), n)
    ccflags-y += -DCFG_SUPPORT_REG_BY_USER=0
else
    ccflags-y += -DCFG_SUPPORT_REG_BY_USER=1
endif

ifeq ($(CONFIG_DRIVER_INF_NAME_CHANGE), y)
    ccflags-y += -DCFG_DRIVER_INF_NAME_CHANGE=1
else
    ccflags-y += -DCFG_DRIVER_INF_NAME_CHANGE=0
endif

ifeq ($(CONFIG_ANDROID_AOSP_PRIV_CMD), y)
    ccflags-y += -DCFG_ANDROID_AOSP_PRIV_CMD=1
else
    ccflags-y += -DCFG_ANDROID_AOSP_PRIV_CMD=0
endif

ifeq ($(CONFIG_SUPPORT_DFS_MASTER), n)
    ccflags-y += -DCFG_SUPPORT_DFS_MASTER=0
else
    ccflags-y += -DCFG_SUPPORT_DFS_MASTER=1
endif

ifdef CONFIG_ANDROID
    ifeq ($(CONFIG_ENABLE_WAKE_LOCK), y)
        ccflags-y += -DCFG_ENABLE_WAKE_LOCK=1
    else
        ccflags-y += -DCFG_ENABLE_WAKE_LOCK=0
    endif
else
    ccflags-y += -DCFG_ENABLE_WAKE_LOCK=0
endif

ifeq ($(CONFIG_CHIP_RESET_USE_MSTAR_GPIO_API), y)
    ccflags-y += -DCFG_CHIP_RESET_USE_MSTAR_GPIO_API=1
else
    ccflags-y += -DCFG_CHIP_RESET_USE_MSTAR_GPIO_API=0
endif

ifeq ($(CONFIG_DC_USB_WOW_CALLBACK), y)
    ccflags-y += -DCFG_DC_USB_WOW_CALLBACK=1
else
    ccflags-y += -DCFG_DC_USB_WOW_CALLBACK=0
endif

ifeq ($(CONFIG_TX_DIRECT), n)
    ccflags-y += -DCFG_TX_DIRECT=0
else
    ccflags-y += -DCFG_TX_DIRECT=1
endif

ifeq ($(CONFIG_RX_DIRECT), n)
    ccflags-y += -DCFG_RX_DIRECT=0
else
    ccflags-y += -DCFG_RX_DIRECT=1
endif

ifeq ($(CONFIG_ENABLE_EFUSE_MAC_ADDR), y)
    ccflags-y += -DCFG_ENABLE_EFUSE_MAC_ADDR=1
else
    ccflags-y += -DCFG_ENABLE_EFUSE_MAC_ADDR=0
endif

ifeq ($(CONFIG_SDIO_RX_AGG_WORKQUE), y)
    ccflags-y += -DCFG_SDIO_RX_AGG_WORKQUE=1
else
    ccflags-y += -DCFG_SDIO_RX_AGG_WORKQUE=0
endif

ifeq ($(CONFIG_SUPPORT_SINGLE_SKU_LOCAL_DB), n)
    ccflags-y += -DCFG_SUPPORT_SINGLE_SKU_LOCAL_DB=0
else
    ccflags-y += -DCFG_SUPPORT_SINGLE_SKU_LOCAL_DB=1
endif

ifeq ($(CONFIG_SUPPORT_TSF_SYNC), y)
    ccflags-y += -DCFG_SUPPORT_TSF_SYNC=1
else
    ccflags-y += -DCFG_SUPPORT_TSF_SYNC=0
endif

ifeq ($(CONFIG_CHIP_RESET_USB_DISCONNECT), y)
    ccflags-y += -DCFG_CHIP_RESET_USB_DISCONNECT=1
else
    ccflags-y += -DCFG_CHIP_RESET_USB_DISCONNECT=0
endif

ifeq ($(CONFIG_POWER_OFF_CTRL_SUPPORT), y)
    ccflags-y += -DCFG_POWER_OFF_CTRL_SUPPORT=1
else
    ccflags-y += -DCFG_POWER_OFF_CTRL_SUPPORT=0
endif

ifeq ($(CONFIG_SUPPORT_P2P_CSA), y)
    ccflags-y += -DCFG_SUPPORT_P2P_CSA=1
else
    ccflags-y += -DCFG_SUPPORT_P2P_CSA=0
endif

ifeq ($(CONFIG_SUPPORT_DPP), y)
    ccflags-y += -DCFG_SUPPORT_DPP=1
else
    ccflags-y += -DCFG_SUPPORT_DPP=0
endif

ifeq ($(CONFIG_CHIP_RESET_USE_DTS_GPIO_NUM), y)
    ccflags-y += -DCFG_CHIP_RESET_USE_DTS_GPIO_NUM=1
else
    ccflags-y += -DCFG_CHIP_RESET_USE_DTS_GPIO_NUM=0
endif

ifeq ($(CONFIG_CHIP_RESET_USE_LINUX_GPIO_API), y)
    ccflags-y += -DCFG_CHIP_RESET_USE_LINUX_GPIO_API=1
else
    ccflags-y += -DCFG_CHIP_RESET_USE_LINUX_GPIO_API=0
endif

ifeq ($(CONFIG_SUPPORT_MTK_SCAN_EVENT), y)
    ccflags-y += -DCFG_SUPPORT_MTK_SCAN_EVENT=1
else
    ccflags-y += -DCFG_SUPPORT_MTK_SCAN_EVENT=0
endif

ifeq ($(CONFIG_DBG_PRINT_RTP_INFO), y)
    ccflags-y += -DCFG_DBG_PRINT_RTP_INFO=1
else
    ccflags-y += -DCFG_DBG_PRINT_RTP_INFO=0
endif

ifeq ($(CONFIG_TEST_CODE_FOR_MDNS), y)
    ccflags-y += -DTEST_CODE_FOR_MDNS=1
else
    ccflags-y += -DTEST_CODE_FOR_MDNS=0
endif

ifeq ($(CONFIG_SUPPORT_TSF_USING_BOOTTIME), y)
    ccflags-y += -DCFG_SUPPORT_TSF_USING_BOOTTIME=1
else
    ccflags-y += -DCFG_SUPPORT_TSF_USING_BOOTTIME=0
endif

ifeq ($(CONFIG_SUPPORT_DEBUG_FS), y)
    ccflags-y += -DCFG_SUPPORT_DEBUG_FS=1
else
    ccflags-y += -DCFG_SUPPORT_DEBUG_FS=0
endif

ifeq ($(CONFIG_SUPPORT_AGPS_ASSIST), y)
    ccflags-y += -DCFG_SUPPORT_AGPS_ASSIST=1
else
    ccflags-y += -DCFG_SUPPORT_AGPS_ASSIST=0
endif

ifeq ($(CONFIG_ARP_MONITER_ENABLE), y)
    ccflags-y += -DARP_MONITER_ENABLE=1
else
    ccflags-y += -DARP_MONITER_ENABLE=0
endif

ifeq ($(CONFIG_SUPPORT_PROC_FS), n)
    ccflags-y += -DWLAN_INCLUDE_PROC=0
else
    ccflags-y += -DWLAN_INCLUDE_PROC=1
endif

ifeq ($(CONFIG_SUPPORT_SCHED_SCAN), y)
    ccflags-y += -DCFG_SUPPORT_SCHED_SCAN=1
else
    ccflags-y += -DCFG_SUPPORT_SCHED_SCAN=0
endif

ifeq ($(CONFIG_SUPPORT_MDNS_OFFLOAD), y)
    ccflags-y += -DCFG_SUPPORT_MDNS_OFFLOAD=1
else
    ccflags-y += -DCFG_SUPPORT_MDNS_OFFLOAD=0
endif

ifeq ($(CONFIG_SUPPORT_SET_IPV6_NETWORK), y)
    ccflags-y += -DCFG_SUPPORT_SET_IPV6_NETWORK=1
else
    ccflags-y += -DCFG_SUPPORT_SET_IPV6_NETWORK=0
endif

ifeq ($(CONFIG_SUPPORT_WOW_EINT), y)
    ccflags-y += -DCFG_SUPPORT_WOW_EINT=1
else
    ccflags-y += -DCFG_SUPPORT_WOW_EINT=0
endif

ifeq ($(CONFIG_AP_80211KVR_INTERFACE), y)
    ccflags-y += -DCFG_AP_80211KVR_INTERFACE=1
else
    ccflags-y += -DCFG_AP_80211KVR_INTERFACE=0
endif

ifeq ($(CONFIG_AP_80211K_SUPPORT), y)
    ccflags-y += -DCFG_AP_80211K_SUPPORT=1
else
    ccflags-y += -DCFG_AP_80211K_SUPPORT=0
endif

ifeq ($(CONFIG_AP_80211V_SUPPORT), y)
    ccflags-y += -DCFG_AP_80211V_SUPPORT=1
else
    ccflags-y += -DCFG_AP_80211V_SUPPORT=0
endif

ifeq ($(CONFIG_SUPPORT_11B_DYNAMIC), y)
    ccflags-y += -DCFG_SUPPORT_11B_DYNAMIC=1
else
    ccflags-y += -DCFG_SUPPORT_11B_DYNAMIC=0
endif

ifeq ($(CONFIG_WIFI_ANTENNA_REPORT), y)
    ccflags-y += -DCONFIG_WIFI_ANTENNA_REPORT=1
else
    ccflags-y += -DCONFIG_WIFI_ANTENNA_REPORT=0
endif

ifeq ($(CONFIG_WIFI_SUPPORT_GET_NOISE), y)
    ccflags-y += -DCFG_WIFI_SUPPORT_GET_NOISE=1
else
    ccflags-y += -DCFG_WIFI_SUPPORT_GET_NOISE=0
endif

ifeq ($(CONFIG_WIFI_SUPPORT_NOISE_HISTOGRAM), y)
    ccflags-y += -DCFG_WIFI_SUPPORT_NOISE_HISTOGRAM=1
else
    ccflags-y += -DCFG_WIFI_SUPPORT_NOISE_HISTOGRAM=0
endif

ifeq ($(CFG_IPI_2CHAIN_SUPPORT), y)
    ccflags-y += -DCFG_IPI_2CHAIN_SUPPORT=1
else
    ccflags-y += -DCFG_IPI_2CHAIN_SUPPORT=0
endif

ifeq ($(CFG_WIFI_PHY_SUPPORT_GET_BCNTH), y)
    ccflags-y += -DCFG_WIFI_PHY_SUPPORT_GET_BCNTH=1
else
    ccflags-y += -DCFG_WIFI_PHY_SUPPORT_GET_BCNTH=0
endif

ifeq ($(CFG_GET_CNM_INFO_BC), y)
    ccflags-y += -DCFG_GET_CNM_INFO_BC=1
else
    ccflags-y += -DCFG_GET_CNM_INFO_BC=0
endif

ifeq ($(CFG_WIFI_GET_MCS_INFO), y)
    ccflags-y += -DCFG_WIFI_GET_MCS_INFO=1
else
    ccflags-y += -DCFG_WIFI_GET_MCS_INFO=0
endif

ifeq ($(CFG_ENABLE_PS_INTV_CTRL), y)
    ccflags-y += -DCFG_ENABLE_PS_INTV_CTRL=1
else
    ccflags-y += -DCFG_ENABLE_PS_INTV_CTRL=0
endif

ifeq ($(CFG_SUPPORT_COEX_NON_COTX), y)
    ccflags-y += -DCFG_SUPPORT_COEX_NON_COTX=1
else
    ccflags-y += -DCFG_SUPPORT_COEX_NON_COTX=0
endif

ifeq ($(CFG_ADM_CTRL), y)
    ccflags-y += -DCFG_ADM_CTRL=1
else
    ccflags-y += -DCFG_ADM_CTRL=0
endif

ifeq ($(CFG_SUPPORT_SW_BIP_GMAC), n)
    ccflags-y += -DCFG_SUPPORT_SW_BIP_GMAC=0
else
    ccflags-y += -DCFG_SUPPORT_SW_BIP_GMAC=1
endif

ifeq ($(CONFIG_SUPPORT_SAP_WPA3_H2E), y)
    ccflags-y += -DCFG_SAP_SUPPORT_WPA3_H2E=1
else
    ccflags-y += -DCFG_SAP_SUPPORT_WPA3_H2E=0
endif

ifeq ($(CONFIG_SUPPORT_BW160), y)
    ccflags-y += -DCFG_SUPPORT_BW160=1
else
    ccflags-y += -DCFG_SUPPORT_BW160=0
endif

ifeq ($(CONFIG_WIFI_RAM_COEX_SPDT_SHR_ANT_CTRL), y)
    ccflags-y += -DCFG_WIFI_RAM_COEX_SPDT_SHR_ANT_CTRL=1
else
    ccflags-y += -DCFG_WIFI_RAM_COEX_SPDT_SHR_ANT_CTRL=0
endif

ifeq ($(CONFIG_SUPPORT_SUPPLICANT_SME), y)
    ccflags-y += -DCFG_SUPPORT_SUPPLICANT_SME=1
else
    ccflags-y += -DCFG_SUPPORT_SUPPLICANT_SME=0
endif

ifeq ($(CONFIG_SUPPORT_OWE), y)
    ccflags-y += -DCFG_SUPPORT_OWE=1
else
    ccflags-y += -DCFG_SUPPORT_OWE=0
endif

ifeq ($(CONFIG_REDIRECT_OID_SUPPORT), y)
    ccflags-y += -DCFG_REDIRECT_OID_SUPPORT=1
else
    ifeq ($(CONFIG_REDIRECT_OID_SUPPORT), n)
        ccflags-y += -DCFG_REDIRECT_OID_SUPPORT=0
    else
        ifeq ($(CONFIG_SUPPORT_SUPPLICANT_SME), y)
            ccflags-y += -DCFG_REDIRECT_OID_SUPPORT=1
        else
            ccflags-y += -DCFG_REDIRECT_OID_SUPPORT=0
        endif
    endif
endif

ifeq ($(CONFIG_SUPPORT_CFG80211_QUEUE), y)
    ccflags-y += -DCFG_SUPPORT_CFG80211_QUEUE=1
else
    ccflags-y += -DCFG_SUPPORT_CFG80211_QUEUE=0
endif

ifeq ($(CONFIG_WIFI_TX_ETH_CHK_EMPTY_PAYLOAD), y)
    ccflags-y += -DCFG_WIFI_TX_ETH_CHK_EMPTY_PAYLOAD=1
else
    ccflags-y += -DCFG_WIFI_TX_ETH_CHK_EMPTY_PAYLOAD=0
endif

ifeq ($(CONFIG_SUPPORT_WAC), y)
    ccflags-y += -DCFG_SUPPORT_WAC=1
else
    ccflags-y += -DCFG_SUPPORT_WAC=0
endif

ifeq ($(CONFIG_SUPPORT_SDIO_FORCE_DRV_OWN), y)
    ccflags-y += -DCFG_SUPPORT_SDIO_FORCE_DRV_OWN=1
endif

ifeq ($(CONFIG_SUPPORT_DUAL_WTBL_GTK_REKEY_OFFLOAD), n)
    ccflags-y += -DCFG_SUPPORT_DUAL_WTBL_GTK_REKEY_OFFLOAD=0
else
    ccflags-y += -DCFG_SUPPORT_DUAL_WTBL_GTK_REKEY_OFFLOAD=1
endif

ifneq ($(CONFIG_SDIO_INTR_ENHANCE_FORMAT),)
    ccflags-y += -DCFG_SDIO_INTR_ENHANCE_FORMAT=$(CONFIG_SDIO_INTR_ENHANCE_FORMAT)
else
    ccflags-y += -DCFG_SDIO_INTR_ENHANCE_FORMAT=1
endif

ifneq ($(CONFIG_DEFAULT_DBG_LEVEL),)
    ccflags-y += -DCFG_DEFAULT_DBG_LEVEL=$(CONFIG_DEFAULT_DBG_LEVEL)
endif

ifneq ($(CONFIG_NUM_DIFFERENT_CHANNELS_STA),)
    ccflags-y += -DCFG_NUM_DIFFERENT_CHANNELS_STA=$(CONFIG_NUM_DIFFERENT_CHANNELS_STA)
endif

ifneq ($(CONFIG_NUM_DIFFERENT_CHANNELS_P2P),)
    ccflags-y += -DCFG_NUM_DIFFERENT_CHANNELS_P2P=$(CONFIG_NUM_DIFFERENT_CHANNELS_P2P)
endif

ifneq ($(CONFIG_DRIVER_INITIAL_RUNNING_MODE),)
    ccflags-y += -DCFG_DRIVER_INITIAL_RUNNING_MODE=$(CONFIG_DRIVER_INITIAL_RUNNING_MODE)
endif

ifneq ($(CONFIG_USB_REQ_TX_DATA_FFA_CNT),)
    ccflags-y += -DCFG_USB_REQ_TX_DATA_FFA_CNT=$(CONFIG_USB_REQ_TX_DATA_FFA_CNT)
endif

ifneq ($(CONFIG_USB_REQ_TX_DATA_CNT),)
    ccflags-y += -DCFG_USB_REQ_TX_DATA_CNT=$(CONFIG_USB_REQ_TX_DATA_CNT)
endif

ifneq ($(CONFIG_USB_REQ_RX_DATA_CNT),)
    ccflags-y += -DCFG_USB_REQ_RX_DATA_CNT=$(CONFIG_USB_REQ_RX_DATA_CNT)
endif

ifneq ($(CONFIG_P2P_SCAN_REPORT_ALL_BSS),)
    ccflags-y += -DCFG_P2P_SCAN_REPORT_ALL_BSS=$(CONFIG_P2P_SCAN_REPORT_ALL_BSS)
endif

ifneq ($(CONFIG_WIFI_CFG_FN),)
    ccflags-y += -DCFG_WIFI_CFG_FN=$(CONFIG_WIFI_CFG_FN)
endif

ifneq ($(CONFIG_CHIP_RESET_DTS_COMPATIBLE_NAME),)
    ccflags-y += -DCHIP_RESET_DTS_COMPATIBLE_NAME=$(CONFIG_CHIP_RESET_DTS_COMPATIBLE_NAME)
endif

ifneq ($(CONFIG_CHIP_RESET_GPIO_PROPERTY_NAME),)
    ccflags-y += -DCHIP_RESET_GPIO_PROPERTY_NAME=$(CONFIG_CHIP_RESET_GPIO_PROPERTY_NAME)
endif

ifneq ($(CONFIG_CHIP_RESET_INVERT_PROPERTY_NAME),)
    ccflags-y += -DCHIP_RESET_INVERT_PROPERTY_NAME=$(CONFIG_CHIP_RESET_INVERT_PROPERTY_NAME)
endif

ifneq ($(CONFIG_CHIP_RESET_DEFAULT_VAL_PROPERTY_NAME),)
    ccflags-y += -DCHIP_RESET_DEFAULT_VAL_PROPERTY_NAME=$(CONFIG_CHIP_RESET_DEFAULT_VAL_PROPERTY_NAME)
endif

ifneq ($(CONFIG_QM_CMD_RESERVED_THRESHOLD),)
    ccflags-y += -DQM_CMD_RESERVED_THRESHOLD=$(CONFIG_QM_CMD_RESERVED_THRESHOLD)
else
    ccflags-y += -DQM_CMD_RESERVED_THRESHOLD=4
endif

ifneq ($(CONFIG_RX_REPORT_FORMAT),)
    ccflags-y += -DCFG_RX_REPORT_FORMAT=$(CONFIG_RX_REPORT_FORMAT)
else
    ccflags-y += -DCFG_RX_REPORT_FORMAT=1
endif

ifneq ($(CONFIG_WIFI_FWDL_UMAC_RESERVE_SIZE_PARA),)
    ccflags-y += -DCFG_WIFI_FWDL_UMAC_RESERVE_SIZE_PARA=$(CONFIG_WIFI_FWDL_UMAC_RESERVE_SIZE_PARA)
else
    ccflags-y += -DCFG_WIFI_FWDL_UMAC_RESERVE_SIZE_PARA=0
endif

ifneq ($(CONFIG_SYS_DVT_SUPPORT),)
    ccflags-y += -DCFG_SUPPORT_WIFI_SYSDVT=$(CONFIG_SYS_DVT_SUPPORT)
    ifneq ($(CONFIG_SYS_DVT_DMASCHDL_SUPPORT),)
        ccflags-y += -DCFG_SUPPORT_DMASHDL_SYSDVT=$(CONFIG_SYS_DVT_DMASCHDL_SUPPORT)
    endif
endif

ifneq ($(CONFIG_SUPPORT_PERMON),)
    ccflags-y += -DCFG_SUPPORT_PERMON=$(CONFIG_SUPPORT_PERMON)
endif

ifeq ($(CONFIG_MTK_CONNSYS_DEDICATED_LOG_PATH), y)
    ccflags-y += -DCFG_MTK_CONNSYS_DEDICATED_LOG_PATH=1
else
    ccflags-y += -DCFG_MTK_CONNSYS_DEDICATED_LOG_PATH=0
endif

ifeq ($(CONFIG_SUPPORT_HE_ER), y)
    ccflags-y += -DCFG_SUPPORT_HE_ER=1
else
    ccflags-y += -DCFG_SUPPORT_HE_ER=0
endif

ifeq ($(CONFIG_SUPPORT_DBDC), n)
    ccflags-y += -DCFG_SUPPORT_DBDC=0
else
    ccflags-y += -DCFG_SUPPORT_DBDC=1
endif

ifeq ($(CONFIG_MTK_WIFI_CONNAC2X), y)
    ccflags-y += -DCFG_SUPPORT_CONNAC2X=1
else
    ccflags-y += -DCFG_SUPPORT_CONNAC2X=0
endif

ifeq ($(CONFIG_MTK_WIFI_CONNAC3X), y)
    ccflags-y += -DCFG_SUPPORT_CONNAC3X=1
else
    ccflags-y += -DCFG_SUPPORT_CONNAC3X=0
endif

ifeq ($(CONFIG_MTK_WIFI_11AX_SUPPORT), y)
    ccflags-y += -DCFG_SUPPORT_802_11AX=1
else
    ccflags-y += -DCFG_SUPPORT_802_11AX=0
endif

ifeq ($(CONFIG_MTK_WIFI_11AX_SUPPORT)_$(CONFIG_CONNAC2_LISTMODE_ENABLE), y_y)
    ccflags-y += -DCFG_CONNAC2_LISTMODE_ENABLE=1
else
    ccflags-y += -DCFG_CONNAC2_LISTMODE_ENABLE=0
endif

ifeq ($(CONFIG_MTK_WIFI_11BE_SUPPORT), y)
    ccflags-y += -DCFG_SUPPORT_802_11BE=1
else
    ccflags-y += -DCFG_SUPPORT_802_11BE=0
endif

ifeq ($(CONFIG_MTK_WIFI_TXPWR_DAT_FN), y)
    ccflags-y += -DCFG_WIFI_TXPWR_FN='"TxPwrLimit_$(MTK_COMBO_CHIP).dat"'
    ccflags-y += -DCFG_WIFI_TXPWR_6G_LPI_FN='"TxPwrLimit6G_$(MTK_COMBO_CHIP).dat"'
    ccflags-y += -DCFG_WIFI_TXPWR_6G_VLP_FN='"TxPwrLimit6G_$(MTK_COMBO_CHIP)_VLP.dat"'
    ccflags-y += -DCFG_WIFI_TXPWR_6G_SP_FN='"TxPwrLimit6G_$(MTK_COMBO_CHIP)_SP.dat"'
else
    ccflags-y += -DCFG_WIFI_TXPWR_FN='"TxPwrLimit_MT79x1.dat"'
    ccflags-y += -DCFG_WIFI_TXPWR_6G_LPI_FN='"TxPwrLimit6G_MT79x1.dat"'
    ccflags-y += -DCFG_WIFI_TXPWR_6G_VLP_FN='"TxPwrLimit6G_MT79x1_VLP.dat"'
    ccflags-y += -DCFG_WIFI_TXPWR_6G_SP_FN='"TxPwrLimit6G_MT79x1_SP.dat"'
endif

ifeq ($(CONFIG_MTK_WIFI_11AX_SUPPORT), y)
ifeq ($(CONFIG_MTK_WIFI_6G_SUPPORT), y)
    ccflags-y += -DCFG_SUPPORT_WIFI_6G=1
    ccflags-y += -DCFG_SUPPORT_WIFI_DBDC6G=1
    ccflags-y += -DCFG_SUPPORT_TX_MGMT_USE_DATAQ=1
    ifeq ($(CONFIG_MTK_WIFI_SUPPORT_6G_PWR_REGS), y)
        ccflags-y += -DCFG_SUPPORT_6G_PWR_REGS=1
    else
        ccflags-y += -DCFG_SUPPORT_6G_PWR_REGS=0
    endif
else
    ccflags-y += -DCFG_SUPPORT_WIFI_6G=0
    ccflags-y += -DCFG_SUPPORT_WIFI_DBDC6G=0
    ccflags-y += -DCFG_SUPPORT_TX_MGMT_USE_DATAQ=0
    ccflags-y += -DCFG_SUPPORT_6G_PWR_REGS=0
endif
else
    ccflags-y += -DCFG_SUPPORT_WIFI_6G=0
    ccflags-y += -DCFG_SUPPORT_WIFI_DBDC6G=0
    ccflags-y += -DCFG_SUPPORT_TX_MGMT_USE_DATAQ=0
    ccflags-y += -DCFG_SUPPORT_6G_PWR_REGS=0
endif

ifeq ($(CONFIG_MTK_DBDC_SW_FOR_P2P_LISTEN), y)
    ccflags-y += -DCFG_DBDC_SW_FOR_P2P_LISTEN=1
endif

ifeq ($(CONFIG_MTK_WIFI_TWT_SUPPORT), y)
    ccflags-y += -DCFG_SUPPORT_TWT=1
    ifeq ($(CONFIG_WIFI_TWT_STAPS_DISABLE_SCAN), y)
        ccflags-y += -DCFG_DISABLE_SCAN_UNDER_TWT=1
    else
        ccflags-y += -DCFG_DISABLE_SCAN_UNDER_TWT=0
    endif
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
    ifeq ($(CONIFG_WIFI_TWT_WIFI_6E_CERTIFICATION), y)
        ccflags-y += -DCFG_KEEPFULLPOWER_BEFORE_TWT_NEGO=1
    else
        ccflags-y += -DCFG_KEEPFULLPOWER_BEFORE_TWT_NEGO=0
    endif
else
    ccflags-y += -DCFG_SUPPORT_TWT=0
    ccflags-y += -DCFG_SUPPORT_TWT_HOTSPOT=0
    ccflags-y += -DCFG_SUPPORT_TWT_HOTSPOT_AC=0
    ccflags-y += -DCFG_KEEPFULLPOWER_BEFORE_TWT_NEGO=0
endif

ifneq ($(CONFIG_MTK_COMBO_WIFI_HIF), pcie)
CONFIG_SUPPORT_PCIE_ASPM=n
endif
ifeq ($(CONFIG_SUPPORT_PCIE_ASPM), y)
    ccflags-y += -DCFG_SUPPORT_PCIE_ASPM=1
else
    ccflags-y += -DCFG_SUPPORT_PCIE_ASPM=0
endif

ifeq ($(CONFIG_SUPPORT_PCIE_ASPM_IMPROVE), y)
    ccflags-y += -DCFG_SUPPORT_PCIE_ASPM_IMPROVE=1
else
    ccflags-y += -DCFG_SUPPORT_PCIE_ASPM_IMPROVE=0
endif

ifeq ($(CONFIG_CONTROL_ASPM_BY_FW), y)
    ccflags-y += -DCFG_CONTROL_ASPM_BY_FW=1
else
    ccflags-y += -DCFG_CONTROL_ASPM_BY_FW=0
endif

ifeq ($(CONFIG_SUPPORT_IPC_WITHOUT_MMIO_READ), y)
    ccflags-y += -DCFG_SUPPORT_IPC_WITHOUT_MMIO_READ=1
else
    ccflags-y += -DCFG_SUPPORT_IPC_WITHOUT_MMIO_READ=0
endif

ifeq ($(CONFIG_SUPPORT_DISABLE_DATA_DDONE_INTR), y)
    ccflags-y += -DCFG_SUPPORT_DISABLE_DATA_DDONE_INTR=1
else
    ccflags-y += -DCFG_SUPPORT_DISABLE_DATA_DDONE_INTR=0
endif

ifeq ($(WIFI_ENABLE_GCOV), y)
GCOV_PROFILE := y
endif

ifeq ($(CONFIG_SUPPORT_SNIFFER_RADIOTAP), y)
    ccflags-y += -DCFG_SUPPORT_SNIFFER_RADIOTAP=1
    ccflags-y += -DCFG_SUPPORT_PDMA_SCATTER=1
else
    ccflags-y += -DCFG_SUPPORT_SNIFFER_RADIOTAP=0
    ccflags-y += -DCFG_SUPPORT_PDMA_SCATTER=0
endif

ifeq ($(CONFIG_MTK_WIFI_NAN), y)
    ccflags-y += -DCFG_SUPPORT_DUAL_STA=0
else ifeq ($(CONFIG_MTK_TC10_FEATURE), y)
    ccflags-y += -DCFG_SUPPORT_DUAL_STA=0
else
    ccflags-y += -DCFG_SUPPORT_DUAL_STA=1
endif

ccflags-y += -DCFG_MTK_ANDROID_WMT=0

ifeq ($(MTK_ANDROID_EMI), y)
    ccflags-y += -DCFG_MTK_ANDROID_EMI=1
else
    ccflags-y += -DCFG_MTK_ANDROID_EMI=0
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

ifeq ($(CONFIG_MTK_COMBO_WIFI),y)
    ccflags-y += -DCFG_WPS_DISCONNECT=1
endif

ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), sdio)
    ccflags-y += -D_HIF_SDIO=1
else ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), pcie)
    ccflags-y += -D_HIF_PCIE=1
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

ifeq ($(CONFIG_MTK_WIFI_POWER_ON_DOWNLOAD_EMI_ROM_PATCH), y)
    ccflags-y += -DCFG_POWER_ON_DOWNLOAD_EMI_ROM_PATCH=1
else
    ccflags-y += -DCFG_POWER_ON_DOWNLOAD_EMI_ROM_PATCH=0
endif

ifeq ($(CONFIG_MTK_WIFI_DOWNLOAD_DYN_MEMORY_MAP), y)
    ccflags-y += -DCFG_DOWNLOAD_DYN_MEMORY_MAP=1
else
    ccflags-y += -DCFG_DOWNLOAD_DYN_MEMORY_MAP=0
endif

ifeq ($(CONFIG_MTK_WIFI_ROM_PATCH_NO_SEM_CTRL), y)
    ccflags-y += -DCFG_ROM_PATCH_NO_SEM_CTRL=1
else
    ccflags-y += -DCFG_ROM_PATCH_NO_SEM_CTRL=0
endif

ifneq ($(CFG_CFG80211_VERSION),)
VERSION_STR = $(subst \",,$(subst ., , $(subst -, ,$(subst v,,$(CFG_CFG80211_VERSION)))))
$(info VERSION_STR=$(VERSION_STR))
X = $(firstword $(VERSION_STR))
Y = $(word 2 ,$(VERSION_STR))
Z = $(word 3 ,$(VERSION_STR))
VERSION := $(shell echo "$$(( $X * 65536 + $Y * 256 + $Z))" )
ccflags-y += -DCFG_CFG80211_VERSION=$(VERSION)
$(info DCFG_CFG80211_VERSION=$(VERSION))
endif


ifeq ($(CONFIG_MTK_PASSPOINT_R2_SUPPORT), y)
    ccflags-y += -DCFG_SUPPORT_PASSPOINT=1
    ccflags-y += -DCFG_HS20_DEBUG=1
    ccflags-y += -DCFG_ENABLE_GTK_FRAME_FILTER=1
else
    ccflags-y += -DCFG_SUPPORT_PASSPOINT=0
    ccflags-y += -DCFG_HS20_DEBUG=0
    ccflags-y += -DCFG_ENABLE_GTK_FRAME_FILTER=0
endif


MTK_MET_TAG_SUPPORT = no
ifeq ($(MTK_MET_TAG_SUPPORT), yes)
    ccflags-y += -DMET_USER_EVENT_SUPPORT
    ccflags-y += -DCFG_MET_TAG_SUPPORT=1
else
    ccflags-y += -DCFG_MET_TAG_SUPPORT=0
endif

ifeq ($(CONFIG_MTK_TC10_FEATURE), y)
    ccflags-y += -DCFG_TC10_FEATURE=1
else
    ccflags-y += -DCFG_TC10_FEATURE=0
endif

ifeq ($(CONFIG_MTK_TC1_FEATURE), y)
    ccflags-y += -I$(srctree)/drivers/misc/mediatek/tc1_interface
    ccflags-y += -DCFG_TC1_FEATURE=1
else
    ccflags-y += -DCFG_TC1_FEATURE=0
endif

ifeq ($(CONFIG_MTK_WIFI_NAN), y)
    ccflags-y += -DCFG_SUPPORT_NAN=1
ifeq ($(CONFIG_MTK_WIFI_NAN_P2P_CONCURRENT_SUPPORT), y)
        ccflags-y += -DCFG_NAN_P2P_CONCURRENT_SUPPORT=1
    else
        ccflags-y += -DCFG_NAN_P2P_CONCURRENT_SUPPORT=0
    endif
    ifeq ($(CONFIG_MTK_WIFI_NAN_SUPPORT_R4), y)
        ccflags-y += -DCFG_SUPPORT_NAN_R4=1
        ccflags-y += -DCFG_SUPPORT_NAN_R4_VENDOR_PAIRING=1
        ccflags-y += -DCFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT=1
        ccflags-y += -DCFG_NAN_SUPPORT_R4_GROUP_ADDR_DATA=1
        ccflags-y += -DCFG_SUPPORT_NAN_R4_VENDOR_HOST_KEY=1
    else
        ccflags-y += -DCFG_SUPPORT_NAN_R4=0
        ccflags-y += -DCFG_SUPPORT_NAN_R4_VENDOR_PAIRING=0
        ccflags-y += -DCFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT=0
        ccflags-y += -DCFG_NAN_SUPPORT_R4_GROUP_ADDR_DATA=0
        ccflags-y += -DCFG_SUPPORT_NAN_R4_VENDOR_HOST_KEY=0
    endif
else
    ccflags-y += -DCFG_SUPPORT_NAN=0
    ccflags-y += -DCFG_SUPPORT_NAN_R4=0
    ccflags-y += -DCFG_SUPPORT_NAN_R4_VENDOR_PAIRING=0
    ccflags-y += -DCFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT=0
    ccflags-y += -DCFG_NAN_SUPPORT_R4_GROUP_ADDR_DATA=0
    ccflags-y += -DCFG_SUPPORT_NAN_R4_VENDOR_HOST_KEY=0
ccflags-y += -DCFG_NAN_P2P_CONCURRENT_SUPPORT=0
endif

# For CSI support
# 1: enable, 0: disable
ifeq ($(CONFIG_MTK_WIFI_CSI), y)
    ccflags-y += -DCFG_SUPPORT_CSI=1
    ifeq ($(CONFIG_MTK_WIFI_CSI_DEBUG), y)
        ccflags-y += -DCFG_SUPPORT_CSI_DEBUG=1
    else
        ccflags-y += -DCFG_SUPPORT_CSI_DEBUG=0
    endif
    ifeq ($(CONFIG_MTK_WIFI_CSI_NF), y)
        ccflags-y += -DCFG_SUPPORT_CSI_NF=1
        ifeq ($(CONFIG_MTK_WIFI_CSI_BFADJ), y)
            ccflags-y += -DCFG_SUPPORT_CSI_BFADJ=1
        else
            ccflags-y += -DCFG_SUPPORT_CSI_BFADJ=0
        endif
        ifeq ($(CONFIG_MTK_WIFI_CSI_BWDF), y)
            ccflags-y += -DCFG_SUPPORT_CSI_BWDF=1
        else
            ccflags-y += -DCFG_SUPPORT_CSI_BWDF=0
        endif
    else
        ccflags-y += -DCFG_SUPPORT_CSI_NF=0
        ccflags-y += -DCFG_SUPPORT_CSI_BFADJ=0
        ccflags-y += -DCFG_SUPPORT_CSI_BWDF=0
    endif
    # For CSI Virtual Network Interface support
    # 1: enable, 0: disable
    ifeq ($(CONFIG_MTK_WIFI_CSI_VNI), y)
        ccflags-y += -DCFG_SUPPORT_CSI_VNI=1
    else
        ccflags-y += -DCFG_SUPPORT_CSI_VNI=0
    endif
else
    ccflags-y += -DCFG_SUPPORT_CSI=0
    ccflags-y += -DCFG_CSI_DEBUG=0
    ccflags-y += -DCFG_SUPPORT_CSI_NF=0
    ccflags-y += -DCFG_SUPPORT_CSI_BFADJ=0
    ccflags-y += -DCFG_SUPPORT_CSI_BWDF=0
    ccflags-y += -DCFG_SUPPORT_CSI_VNI=0
endif

# For Radar support
ifeq ($(CONFIG_SUPPORT_FMCW_RADAR), y)
    ifeq ($(CONFIG_MTK_WIFI_FMCW_VNI), y)
        ccflags-y += -DCFG_SUPPORT_FMCW_VNI=1
    else
        ccflags-y += -DCFG_SUPPORT_FMCW_VNI=0
    endif
    ccflags-y += -DCFG_SUPPORT_FMCW_RADAR=1
    ccflags-y += -DCFG_SUPPORT_RADAR_CMD=1
    ccflags-y += -DCFG_WIFI_FMCW_RADAR_CTS2SELF=1
    ccflags-y += -DCFG_SUPPORT_DYNAMIC_EDCCA=1
    ccflags-y += -DCFG_WIFI_FMCW_RADAR_TRAFFIC_REPORT=1
else
    ccflags-y += -DCFG_SUPPORT_FMCW_VNI=0
    ccflags-y += -DCFG_SUPPORT_FMCW_RADAR=0
    ccflags-y += -DCFG_SUPPORT_RADAR_CMD=0
    ccflags-y += -DCFG_WIFI_FMCW_RADAR_CTS2SELF=0
    ccflags-y += -DCFG_SUPPORT_DYNAMIC_EDCCA=0
    ccflags-y += -DCFG_WIFI_FMCW_RADAR_TRAFFIC_REPORT=0
endif

ifeq ($(CONFIG_MTK_WIFI_SW_FAST_PATH), y)
ccflags-y += -DCFG_SUPPORT_SW_FAST_PATH=1
else
ccflags-y += -DCFG_SUPPORT_SW_FAST_PATH=0
endif

ccflags-y += -DCFG_FLAVOR_FIRMWARE=0

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

ifeq ($(CONFIG_SUPPORT_WIFI_DL_TEST_MODE), y)
    ccflags-y += -DCFG_SUPPORT_WIFI_DL_TEST_MODE=1
else
    ccflags-y += -DCFG_SUPPORT_WIFI_DL_TEST_MODE=0
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

ifeq ($(CONFIG_SUPPORT_SDIO_DB_DELAY), y)
    ccflags-y += -DCFG_SUPPORT_SDIO_DB_DELAY=1
else
    ccflags-y += -DCFG_SUPPORT_SDIO_DB_DELAY=0
endif

ifeq ($(CONFIG_SUPPORT_MAILBOX_ACK), y)
    ccflags-y += -DCFG_SUPPORT_MAILBOX_ACK=1
else
    ccflags-y += -DCFG_SUPPORT_MAILBOX_ACK=0
endif

ifeq ($(CFG_COALESCING_INTERRUPT), y)
    ccflags-y += -DCFG_COALESCING_INTERRUPT=1
else
    ccflags-y += -DCFG_COALESCING_INTERRUPT=0
endif

ifeq ($(CONFIG_SUPPORT_TX_SG), y)
    ccflags-y += -DCFG_SUPPORT_TX_SG=1
else
    ccflags-y += -DCFG_SUPPORT_TX_SG=0
endif

ifeq ($(CONFIG_SUPPORT_WIFI_RSV_MEM), y)
    ccflags-y += -DCFG_SUPPORT_WIFI_RSV_MEM=1
else
    ccflags-y += -DCFG_SUPPORT_WIFI_RSV_MEM=0
endif

ifeq ($(CONFIG_SUPPORT_DEBUG_SOP), y)
    ccflags-y += -DCFG_SUPPORT_DEBUG_SOP=1
else
    ccflags-y += -DCFG_SUPPORT_DEBUG_SOP=0
endif

ifeq ($(CONFIG_SUPPORT_CMD_OVER_WFDMA), y)
    ccflags-y += -DCFG_SUPPORT_CMD_OVER_WFDMA=1
else
    ccflags-y += -DCFG_SUPPORT_CMD_OVER_WFDMA=0
endif

ifeq ($(CONFIG_SUPPORT_WFDMA_REALLOC), y)
    ccflags-y += -DCFG_SUPPORT_WFDMA_REALLOC=1
else
    ccflags-y += -DCFG_SUPPORT_WFDMA_REALLOC=0
endif

ifeq ($(CONFIG_SUPPORT_RETURN_TASK), y)
    ccflags-y += -DCFG_SUPPORT_RETURN_TASK=1
else
    ccflags-y += -DCFG_SUPPORT_RETURN_TASK=0
endif

ifeq ($(CONFIG_SUPPORT_TX_TSO_SW), y)
    ccflags-y += -DCFG_SUPPORT_TX_TSO_SW=1
else
    ccflags-y += -DCFG_SUPPORT_TX_TSO_SW=0
endif

ifeq ($(CONFIG_SUPPORT_RX_NAPI), y)
    ccflags-y += -DCFG_SUPPORT_RX_NAPI=1
else
    ccflags-y += -DCFG_SUPPORT_RX_NAPI=0
endif

ifeq ($(CONFIG_RX_WORK_SUPPORT), y)
    ccflags-y += -DCFG_SUPPORT_RX_WORK=1
else
    ccflags-y += -DCFG_SUPPORT_RX_WORK=0
endif

ifeq ($(CONFIG_SUPPORT_PCIE_WFDMA_WMM_SCH), y)
    ccflags-y += -DCFG_SUPPORT_PCIE_WFDMA_WMM=1
    ccflags-y += -D_PCIE_WMM_DMASH=1
else
    ccflags-y += -DCFG_SUPPORT_PCIE_WFDMA_WMM=0
endif

ifeq ($(CONFIG_HIF_TX_PREALLOC_DATA_BUFFER), n)
    ccflags-y += -DCFG_HIF_TX_PREALLOC_DATA_BUFFER=0
else
    ccflags-y += -DCFG_HIF_TX_PREALLOC_DATA_BUFFER=1
endif

ifneq ($(CFG_TX_MAX_PKT_NUM),)
    ccflags-y += -DCFG_TX_MAX_PKT_NUM=$(CFG_TX_MAX_PKT_NUM)
endif

ifneq ($(HIF_NUM_OF_QM_RX_PKT_NUM),)
    ccflags-y += -DHIF_NUM_OF_QM_RX_PKT_NUM=$(HIF_NUM_OF_QM_RX_PKT_NUM)
endif

ifeq ($(CFG_SUPPORT_HIDDEN_SW_AP), y)
    ccflags-y += -DCFG_SUPPORT_HIDDEN_SW_AP=1
else
    ccflags-y += -DCFG_SUPPORT_HIDDEN_SW_AP=0
endif

# RSNO Support
# CFG_SUPPORT_RSNO
#    1:support; 0: not support
ifeq ($(CONFIG_SUPPORT_RSNO), y)
    ccflags-y += -DCFG_SUPPORT_RSNO=1
else
    ccflags-y += -DCFG_SUPPORT_RSNO=0
endif

# FW Log Idx
ifeq ($(CONFIG_SUPPORT_FW_IDX_LOG_TRANS), y)
    ccflags-y += -DCFG_SUPPORT_FW_IDX_LOG_TRANS=1
else
    ccflags-y += -DCFG_SUPPORT_FW_IDX_LOG_TRANS=0
endif

# FW Log Timesync
ifeq ($(CONFIG_SUPPORT_FW_LOG_TIMESYNC), y)
    ccflags-y += -DCFG_SUPPORT_FW_LOG_TIMESYNC=1
else
    ccflags-y += -DCFG_SUPPORT_FW_LOG_TIMESYNC=0
endif

ifeq ($(CONFIG_NOT_CLR_FREE_MSDU_IN_ACTIVE_NETWORK), y)
    ccflags-y += -DCFG_NOT_CLR_FREE_MSDU_IN_ACTIVE_NETWORK=1
else
    ccflags-y += -DCFG_NOT_CLR_FREE_MSDU_IN_ACTIVE_NETWORK=0
endif

# Tp Enhance
ifeq ($(CONFIG_MTK_TPENHANCE_MODE), y)
    ccflags-y += -DCFG_SUPPORT_TPENHANCE_MODE=1
    ccflags-y += -DCFG_FORCE_ENABLE_PERF_MONITOR=1
else
    ccflags-y += -DCFG_SUPPORT_TPENHANCE_MODE=0
endif

# GKI Support(can't use filp_open/close,kernel_read/write in GKI kernel, can't use kallsyms_lookup_name)
ifeq ($(CONFIG_GKI_SUPPORT), y)
    ccflags-y += -DCFG_ENABLE_GKI_SUPPORT=1
else
    ccflags-y += -DCFG_ENABLE_GKI_SUPPORT=0
endif

ifeq ($(CONFIG_SUPPORT_BT_SKU), y)
    ccflags-y += -DCFG_SUPPORT_BT_SKU
endif

ifeq ($(CONFIG_SUPPORT_ZB_SKU), y)
    ccflags-y += -DCFG_SUPPORT_ZB_SKU
endif

ifeq ($(CONFIG_SUPPORT_APF), y)
    ccflags-y += -DCFG_SUPPORT_APF=1
endif

ifeq ($(CONFIG_SUPPORT_APFV6), y)
    ccflags-y += -DCFG_SUPPORT_APFV6=1
endif

ifeq ($(CONFIG_USB_RECYCLE_REQ_IN_IRQ), y)
    ccflags-y += -DCFG_USB_RECYCLE_REQ_IN_IRQ=1
endif

ifeq ($(CONFIG_SUPPORT_DRV_OWN_MODE_CONTROL), y)
    ccflags-y += -DCFG_MTK_WIFI_DRV_OWN_MODE_CONTROL=1
else
    ccflags-y += -DCFG_MTK_WIFI_DRV_OWN_MODE_CONTROL=0
endif

# Unified Command Support
ifeq ($(CONFIG_MTK_WIFI_UNIFIED_COMMND_SUPPORT), y)
ccflags-y += -DCFG_SUPPORT_UNIFIED_COMMAND
endif

# Debug Config for userdebug and eng load
ifneq ($(filter userdebug eng, $(TARGET_BUILD_VARIANT)),)
    ccflags-y += -DBUILD_QA_DBG=1
    ifeq ($(CONFIG_SUPPORT_SER_DEBUGFS), y)
        ccflags-y += -DCFG_SUPPORT_SER_DEBUGFS=1
    else
        ccflags-y += -DCFG_SUPPORT_SER_DEBUGFS=0
    endif
else
    ccflags-y += -DBUILD_QA_DBG=0
    ccflags-y += -DCFG_SUPPORT_SER_DEBUGFS=0
endif

ifeq ($(CONFIG_WIFI_RAM_MQM_BA_DELAY_SUPPORT), y)
ccflags-y += -DCFG_WIFI_RAM_MQM_BA_DELAY_SUPPORT=1
endif

# For Android Vendor Hal
# kernel >= 5.4: SHALL use 0, which means you must use the ALPS Android S vendor hal or patch your old hal with CL:5188442
# kernel < 5.4: may use 0 or 1, it depends on your vendor hal version:
#        a) if your hal is ALPS Android S hal, use 0;
#        b) if your hal version is before ALPS Android S hal, use 1;
ifeq ($(CFG_SUPPORT_OLD_VENDOR_HAL), y)
ccflags-y += -DCFG_SUPPORT_OLD_VENDOR_HAL=1
else
ccflags-y += -DCFG_SUPPORT_OLD_VENDOR_HAL=0
endif

ifeq ($(CONFIG_SUPPORT_DTIM_SKIP), y)
ccflags-y += -DCFG_SUPPORT_DTIM_SKIP=1
ccflags-y += -DMAX_SLEEP_TIME=600
endif

# For Single sku DFS channel set default flag
ifeq ($(CONFIG_SUPPORT_SINGLE_SKU_DFS_PROTECT), n)
    ccflags-y += -DCFG_SUPPORT_SINGLE_SKU_DFS_PROTECT=0
else
    ccflags-y += -DCFG_SUPPORT_SINGLE_SKU_DFS_PROTECT=1
endif

ifeq ($(CONFIG_SUPPORT_6G_PASSIVE_SCAN), n)
    ccflags-y += -DCFG_SUPPORT_6G_PASSIVE_SCAN=0
else
    ccflags-y += -DCFG_SUPPORT_6G_PASSIVE_SCAN=1
endif

ifeq ($(CONFIG_SUPPORT_RTT), y)
    ccflags-y += -DCFG_SUPPORT_RTT=1
ccflags-y += -DCFG_WIFI_RTT_BIAS_TABLE='"RttBiasTable_${MTK_COMBO_CHIP}.bin"'
    ifeq ($(CONFIG_SUPPORT_RTT_RSTA), y)
        ccflags-y += -DCFG_SUPPORT_RTT_RSTA=1
    else
        ccflags-y += -DCFG_SUPPORT_RTT_RSTA=0
    endif
else
    ccflags-y += -DCFG_SUPPORT_RTT=0
    ccflags-y += -DCFG_SUPPORT_RTT_RSTA=0
endif

# For One driver multi-card support
# 1: enable, 0: disable
ifeq ($(CONFIG_SUPPORT_MULTI_CARD), y)
    ccflags-y += -DCFG_SUPPORT_MULTI_CARD=1
else
    ccflags-y += -DCFG_SUPPORT_MULTI_CARD=0
endif


# For WF dump BT coredump when BT HIF halt
ifeq ($(CONFIG_SUPPORT_WF_DUMP_BT_COREDUMP), y)
    ccflags-y += -DCFG_SUPPORT_WF_DUMP_BT_COREDUMP=1
else
    ccflags-y += -DCFG_SUPPORT_WF_DUMP_BT_COREDUMP=0
endif

# For save fw idx log by wifi_dump
ifeq ($(CONFIG_SUPPORT_FW_IDX_LOG_SAVE), y)
    ccflags-y += -DCFG_SUPPORT_FW_IDX_LOG_SAVE=1
else
    ccflags-y += -DCFG_SUPPORT_FW_IDX_LOG_SAVE=0
endif

# For Roaming Enhance Feature
$(info CONFIG_MTK_SUPPLICANT_ROAMING_ENHANCE=$(CONFIG_MTK_SUPPLICANT_ROAMING_ENHANCE))
ifeq ($(CONFIG_MTK_SUPPLICANT_ROAMING_ENHANCE), y)
    ccflags-y += -DCFG_MTK_SUPPLICANT_ROAMING_ENHANCE=1
else
    ccflags-y += -DCFG_MTK_SUPPLICANT_ROAMING_ENHANCE=0
endif

# For Connac2 sec fwdl protection
ifeq ($(CONFIG_CONNAC2_PROTECT_SEC_FWDL), y)
    ifneq ($(findstring MT7961,$(MTK_COMBO_CHIP)),)
        ifneq ($(findstring $(CONFIG_MTK_COMBO_WIFI_HIF),usbpcie),)
            ccflags-y += -DCFG_PROTECT_SEC_FWDL=1
        else
            ccflags-y += -DCFG_PROTECT_SEC_FWDL=0
        endif
    else
        ccflags-y += -DCFG_PROTECT_SEC_FWDL=0
    endif
else
    ccflags-y += -DCFG_PROTECT_SEC_FWDL=0
endif

# For SAP TX Fairness Feature
ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), pcie)
    ifeq ($(CONFIG_SUPPORT_SAP_TX_FAIRNESS), y)
        ccflags-y += -DCFG_SUPPORT_SAP_TX_FAIRNESS=1
    else
        ccflags-y += -DCFG_SUPPORT_SAP_TX_FAIRNESS=0
    endif
else
    ccflags-y += -DCFG_SUPPORT_SAP_TX_FAIRNESS=0
endif

# For SAP OWE mode support
ifeq ($(CONFIG_SUPPORT_SAP_OWE), y)
    ccflags-y += -DCFG_SUPPORT_SOFTAP_OWE=1
else
    ccflags-y += -DCFG_SUPPORT_SOFTAP_OWE=0
endif

# RX page pool
ifeq ($(CONFIG_RX_PAGE_POOL), y)
    ifeq ($(call kver_ge,5,15,0),1)
        ccflags-y += -DCFG_SUPPORT_RX_PAGE_POOL=1
ifeq ($(CONFIG_RX_PAGE_POOL_USE_CMA), y)
            ccflags-y += -DCFG_SUPPORT_RX_PAGE_POOL_USE_CMA=1
        else
            ccflags-y += -DCFG_SUPPORT_RX_PAGE_POOL_USE_CMA=0
        endif
    else
        ccflags-y += -DCFG_SUPPORT_RX_PAGE_POOL=0
ccflags-y += -DCFG_SUPPORT_RX_PAGE_POOL_USE_CMA=0
    endif
else
    ccflags-y += -DCFG_SUPPORT_RX_PAGE_POOL=0
ccflags-y += -DCFG_SUPPORT_RX_PAGE_POOL_USE_CMA=0
endif

# For FPGA Environment Only
# Usage: {build cmd} CONFIG_MTK_FPGA_PLATFORM=y
ifneq ($(CONFIG_MTK_FPGA_PLATFORM),)
    ifeq ($(CONFIG_MTK_FPGA_PLATFORM), y)
        ccflags-y += -DCFG_MTK_FPGA_PLATFORM=1
    else ifeq ($(CONFIG_MTK_FPGA_PLATFORM), n)
        ccflags-y += -DCFG_MTK_FPGA_PLATFORM=0
    else
        ccflags-y += -DCFG_MTK_FPGA_PLATFORM=$(CONFIG_MTK_FPGA_PLATFORM)
    endif
else
    ccflags-y += -DCFG_MTK_FPGA_PLATFORM=0
endif

# For PCIe FW Own bypass on Probe & L0.5 case
ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), pcie)
    ifeq ($(CONFIG_PCIE_FW_OWN_BYPASS), y)
        ccflags-y += -DCFG_PCIE_FW_OWN_BYPASS=1
    else
        ccflags-y += -DCFG_PCIE_FW_OWN_BYPASS=0
    endif
else
    ccflags-y += -DCFG_PCIE_FW_OWN_BYPASS=0
endif

# For USB CODL FW State Transition Issue
ifeq ($(CONFIG_USB_DRIVEROWN_POLLING_BY_ACKCR), y)
    ccflags-y += -DCFG_USB_DRIVEROWN_POLLING_BY_ACKCR=1
else
    ccflags-y += -DCFG_USB_DRIVEROWN_POLLING_BY_ACKCR=0
endif

# Google ANU and ANT kernel-5.15 have many differences in cfg80211 API
# So, we need configurations to distinguish between them
# IMB use $(BRANCH)
#    T: android13-5.15
#    U: android14-5.15
#    V: android15-*
# DTV use CONFIG_MTK_KERNEL_ANDROID_15 to identify android15 Kernel-5.15
# DTV use CONFIG_MTK_KERNEL_ANDROID_14 to identify android14 Kernel-5.15
# DTV use CONFIG_MTK_KERNEL_ANDROID_13 to identify android13 Kernel-5.15
BRANCH_VERSION := $(patsubst android%,%,$(BRANCH))
BRANCH_MAJOR_VERSION := $(word 1, $(subst -, ,$(BRANCH_VERSION)))

CONFIG_KERNEL_AN15_OR_LATER ?= 0
ifeq ($(BRANCH), android13-5.15)
    CONFIG_KERNEL_AN13_515=1
    CONFIG_KERNEL_AN14_515=0
else ifeq ($(BRANCH), android14-5.15)
    CONFIG_KERNEL_AN13_515=0
    CONFIG_KERNEL_AN14_515=1
else ifeq ($(CONFIG_MTK_KERNEL_ANDROID_13), y)
    CONFIG_KERNEL_AN13_515=1
    CONFIG_KERNEL_AN14_515=0
else ifeq ($(CONFIG_MTK_KERNEL_ANDROID_14), y)
    CONFIG_KERNEL_AN13_515=0
    CONFIG_KERNEL_AN14_515=1
else ifeq ($(CONFIG_MTK_KERNEL_ANDROID_15), y)
    CONFIG_KERNEL_AN13_515=0
    CONFIG_KERNEL_AN14_515=0
    CONFIG_KERNEL_AN15_OR_LATER=1
else
    CONFIG_KERNEL_AN13_515=0
    CONFIG_KERNEL_AN14_515=0
endif

ifeq ($(shell [ $(BRANCH_MAJOR_VERSION) -ge 15 ] && echo true), true)
    CONFIG_KERNEL_AN15_OR_LATER =1
endif

ccflags-y += -DCFG_KERNEL_AN13_515=$(CONFIG_KERNEL_AN13_515)
ccflags-y += -DCFG_KERNEL_AN14_515=$(CONFIG_KERNEL_AN14_515)
ccflags-y += -DCFG_ANV_OR_LATER=$(CONFIG_KERNEL_AN15_OR_LATER)

# kernel 5.15 cfg80211 para is changed for mlo
ifeq ($(call kver_ge,5,15,41),1)
    ifeq ($(filter 1,$(CONFIG_KERNEL_AN13_515) $(CONFIG_KERNEL_AN14_515)),1)
        ccflags-y += -DCFG_ADVANCED_80211_MLO=1
    else
        ccflags-y += -DCFG_ADVANCED_80211_MLO=0
    endif
else
    ccflags-y += -DCFG_ADVANCED_80211_MLO=0
endif
ifeq ($(CONFIG_SUPPORT_DFS_OFFLOAD), y)
ifeq ($(call kver_ge,4,19,0),1)
ccflags-y += -DCFG_SUPPORT_DFS_OFFLOAD=1
else
$(info "kernel ver unsuppoted DFS_OFFLOAD")
ccflags-y += -DCFG_SUPPORT_DFS_OFFLOAD=0
endif
else
ccflags-y += -DCFG_SUPPORT_DFS_OFFLOAD=0
endif


# iwpriv wlan0 driver get_ch_radioinfo
# Trigger scan get channel radioinfo
ifeq ($(CONFIG_SCAN_RADIOINFO), y)
ccflags-y +=-DCFG_SUPPORT_SCAN_RADIOINFO=1
else
ccflags-y +=-DCFG_SUPPORT_SCAN_RADIOINFO=0
endif

# for connac2 support STA 4Addr
ifeq ($(CONFIG_4ADDR_STATION), y)
ccflags-y +=-DCFG_SUPPORT_4ADDR_STATION=1
else
ccflags-y +=-DCFG_SUPPORT_4ADDR_STATION=0
endif

ifeq ($(CONFIG_ANT_SMART_DIVERSITY), y)
ccflags-y +=-DCFG_SUPPORT_ANT_SMART_DIVERSITY=1
else
ccflags-y +=-DCFG_SUPPORT_ANT_SMART_DIVERSITY=0
endif

ifeq ($(CONFIG_WIFI_TESTMODE_FW_REDOWNLOAD), y)
ccflags-y += -DCFG_WIFI_TESTMODE_FW_REDOWNLOAD=1
endif

# For secefuse access support
ifeq ($(CONFIG_SUPPORT_SECEFUSE_ACCESS),y)
ccflags-y += -DCFG_SECEFUSE_ACCESS=1
else
ccflags-y += -DCFG_SECEFUSE_ACCESS=0
endif

ifeq ($(CONFIG_SUPPORT_UNII4), y)
    ccflags-y += -DCFG_SUPPORT_UNII4=1
else
    ccflags-y += -DCFG_SUPPORT_UNII4=0
endif

ifeq ($(MODULE_NAME),)
    MODULE_NAME := wlan_$(shell echo $(strip $(MTK_COMBO_CHIP)) | tr A-Z a-z)_$(CONFIG_MTK_COMBO_WIFI_HIF)
endif

ccflags-y += -DDBG=0
ccflags-y += -I$(src)/os -I$(src)/os/$(os)/include
ccflags-y += -I$(src)/include -I$(src)/include/nic -I$(src)/include/mgmt -I$(src)/include/chips
ifeq ($(CONFIG_MTK_WIFI_NAN), y)
ccflags-y += -I$(src)/include/nan -I$(src)/include/nan/wpa_supp
endif
ifeq ($(CONFIG_SYS_DVT_SUPPORT), 1)
ccflags-y += -I$(src)/include/dvt
endif
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/performance/include/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/emi/$(MTK_PLATFORM)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/emi/submodule
ccflags-y += -I$(srctree)/drivers/devfreq/
ccflags-y += -I$(srctree)/net

ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), sdio)
ccflags-y += -I$(src)/os/$(os)/hif/sdio/include
else ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), pcie)
ccflags-y += -I$(src)/os/$(os)/hif/common/include
ccflags-y += -I$(src)/os/$(os)/hif/pcie/include
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

ifeq ($(CONFIG_BUILD_WLAN),y)
ifeq ($(CONFIG_MTK_WIFI_ONLY),$(filter $(CONFIG_MTK_WIFI_ONLY),m y))
obj-$(CONFIG_MTK_WIFI_ONLY) += $(MODULE_NAME).o
else
obj-$(CONFIG_MTK_COMBO_WIFI) += $(MODULE_NAME).o
#obj-y += $(MODULE_NAME).o
endif

ifeq ($(CONFIG_WLAN_DRV_BUILD_IN),y)
$(warning $(MODULE_NAME) build-in boot.img)
obj-y += $(MODULE_NAME).o
else
$(warning $(MODULE_NAME) is kernel module)
obj-m += $(MODULE_NAME).o
endif
endif

# ---------------------------------------------------
# Directory List
# ---------------------------------------------------
COMMON_DIR  := common/
OS_DIR      := os/$(os)/
HIF_COMMON_DIR := $(OS_DIR)hif/common/
ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), sdio)
HIF_DIR        := os/$(os)/hif/sdio/
else ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), pcie)
HIF_DIR     := os/$(os)/hif/pcie/
else ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), axi)
HIF_DIR        := os/$(os)/hif/axi/
else ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), usb)
HIF_DIR        := os/$(os)/hif/usb/
else ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), ut)
HIF_DIR        := test/lib/hif/
else ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), none)
HIF_DIR        := os/$(os)/hif/none/
endif
NIC_DIR     := nic/
MGMT_DIR    := mgmt/
NAN_DIR     := nan/
CHIPS       := chips/
CHIPS_CMM   := $(CHIPS)common/

ifneq ($(MTK_PLATFORM),)
PLAT_DIR    := os/$(os)/plat/$(MTK_PLATFORM)/
endif
SYSDVT_DIR  := dvt/

# ---------------------------------------------------
# Objects List
# ---------------------------------------------------

COMMON_OBJS :=     $(COMMON_DIR)dump.o \
        $(COMMON_DIR)wlan_lib.o \
        $(COMMON_DIR)wlan_oid.o \
        $(COMMON_DIR)wlan_bow.o \
        $(COMMON_DIR)debug.o

NIC_OBJS :=     $(NIC_DIR)nic.o \
        $(NIC_DIR)nic_tx.o \
        $(NIC_DIR)nic_txd_v1.o \
        $(NIC_DIR)nic_rxd_v1.o \
        $(NIC_DIR)nic_rx.o \
        $(NIC_DIR)nic_pwr_mgt.o \
        $(NIC_DIR)nic_rate.o \
        $(NIC_DIR)cmd_buf.o \
        $(NIC_DIR)que_mgt.o \
        $(NIC_DIR)nic_cmd_event.o \
        $(NIC_DIR)nic_umac.o \
        $(NIC_DIR)radiotap.o

ifeq ($(os), none)
OS_OBJS :=     $(OS_DIR)gl_dependent.o \
        $(OS_DIR)gl_init.o \
        $(OS_DIR)gl_kal.o \
        $(OS_DIR)gl_ate_agent.o \
        $(OS_DIR)gl_qa_agent.o
else
OS_OBJS :=     $(OS_DIR)gl_init.o \
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
        $(OS_DIR)platform.o
ifeq ($(CONFIG_SUPPORT_FW_IDX_LOG_SAVE), y)
    OS_OBJS += $(OS_DIR)gl_fw_dev.o
endif
endif

MGMT_OBJS :=     $(MGMT_DIR)ais_fsm.o \
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
        $(MGMT_DIR)ap_selection.o \
        $(MGMT_DIR)wmm.o \
        $(MGMT_DIR)gcm.o \
        $(MGMT_DIR)rtt.o

# ---------------------------------------------------
# Chips Objects List
# ---------------------------------------------------
MGMT_OBJS += $(MGMT_DIR)stats.o


CHIPS_OBJS += $(CHIPS_CMM)cmm_asic_connac.o
CHIPS_OBJS +=  $(CHIPS_CMM)dbg_connac.o
ifeq ($(CONFIG_MTK_WIFI_CONNAC2X), y)
CHIPS_OBJS +=  $(CHIPS_CMM)dbg_connac2x.o
endif

ifeq ($(CONFIG_MTK_WIFI_CONNAC2X), y)
CHIPS_OBJS += $(CHIPS_CMM)cmm_asic_connac2x.o
NIC_OBJS += $(NIC_DIR)nic_ext_cmd_event.o \
            $(NIC_DIR)nic_txd_v2.o \
            $(NIC_DIR)nic_rxd_v2.o
endif
ifeq ($(CONFIG_MTK_WIFI_CONNAC3X), y)
CHIPS_OBJS += $(CHIPS_CMM)cmm_asic_common.o
CHIPS_OBJS += $(CHIPS_CMM)cmm_asic_connac3x.o \
              $(CHIPS_CMM)dbg_connac3x.o \
              $(CHIPS_CMM)dbg_wtbl_connac3x.o
NIC_OBJS += $(NIC_DIR)nic_ext_cmd_event.o \
            $(NIC_DIR)nic_txd_v3.o \
            $(NIC_DIR)nic_rxd_v3.o
endif
CHIPS_OBJS += $(CHIPS_CMM)fw_dl.o

ifeq ($(CONFIG_MTK_WIFI_UNIFIED_COMMND_SUPPORT), y)
NIC_OBJS += $(NIC_DIR)nic_uni_cmd_event.o
endif

ifneq ($(findstring MT7961,$(MTK_COMBO_CHIP)),)
CHIPS_OBJS += $(CHIPS)mt7961/mt7961.o
CHIPS_OBJS += $(CHIPS)mt7961/dbg_mt7961.o
CHIPS_OBJS += $(CHIPS)mt7961/hal_dmashdl_mt7961.o
CHIPS_OBJS += $(CHIPS)mt7961/hal_wfsys_reset_mt7961.o
endif
ifneq ($(findstring MT7922,$(MTK_COMBO_CHIP)),)
CHIPS_OBJS += $(CHIPS)mt7922/mt7922.o
CHIPS_OBJS += $(CHIPS)mt7961/mt7961.o
CHIPS_OBJS += $(CHIPS)mt7961/dbg_mt7961.o
CHIPS_OBJS += $(CHIPS)mt7922/dbg_mt7922.o
CHIPS_OBJS += $(CHIPS)mt7961/hal_dmashdl_mt7961.o
CHIPS_OBJS += $(CHIPS)mt7961/hal_wfsys_reset_mt7961.o
endif
ifneq ($(findstring MT7902,$(MTK_COMBO_CHIP)),)
CHIPS_OBJS += $(CHIPS)mt7902/mt7902.o
CHIPS_OBJS += $(CHIPS)mt7961/mt7961.o
CHIPS_OBJS += $(CHIPS)mt7961/dbg_mt7961.o
CHIPS_OBJS += $(CHIPS)mt7902/dbg_mt7902.o
CHIPS_OBJS += $(CHIPS)mt7902/hal_dmashdl_mt7902.o
CHIPS_OBJS += $(CHIPS)mt7961/hal_wfsys_reset_mt7961.o
endif
ifneq ($(findstring MT7926,$(MTK_COMBO_CHIP)),)
CHIPS_OBJS += $(CHIPS)mt7926/mt7926.o
CHIPS_OBJS += $(CHIPS)mt7902/mt7902.o
CHIPS_OBJS += $(CHIPS)mt7902/dbg_mt7902.o
CHIPS_OBJS += $(CHIPS)mt7961/mt7961.o
CHIPS_OBJS += $(CHIPS)mt7961/dbg_mt7961.o
CHIPS_OBJS += $(CHIPS)mt7926/dbg_mt7926.o
CHIPS_OBJS += $(CHIPS)mt7926/hal_wfsys_reset_mt7926.o
CHIPS_OBJS += $(CHIPS)mt7961/hal_wfsys_reset_mt7961.o
CHIPS_OBJS += $(CHIPS)mt7902/hal_dmashdl_mt7902.o
endif
ifeq ($(CONFIG_SUPPORT_FW_IDX_LOG_TRANS), y)
CHIPS_OBJS += $(CHIPS_CMM)fw_log_parser.o
endif

# ---------------------------------------------------
# P2P Objects List
# ---------------------------------------------------

COMMON_OBJS += $(COMMON_DIR)wlan_p2p.o

NIC_OBJS += $(NIC_DIR)p2p_nic.o

ifneq ($(os), none)
OS_OBJS += $(OS_DIR)gl_p2p.o \
           $(OS_DIR)gl_p2p_cfg80211.o \
           $(OS_DIR)gl_p2p_init.o \
           $(OS_DIR)gl_p2p_kal.o
endif

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
            $(MGMT_DIR)p2p_fsm.o

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
            $(NAN_DIR)nan_txm.o \
            $(NAN_DIR)nan_pairing.o

NAN_SEC_OBJS := $(NAN_DIR)nan_sec.o\
                $(NAN_DIR)wpa_supp/FourWayHandShake.o\
                $(NAN_DIR)wpa_supp/src/ap/wpa_auth_ie.o\
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
# CSI Objects List
# ---------------------------------------------------
ifeq ($(CONFIG_MTK_WIFI_CSI), y)
    ifeq ($(CONFIG_MTK_WIFI_CSI_VNI), y)
        OS_OBJS += $(OS_DIR)gl_csi.o
    endif
endif

# ---------------------------------------------------
# Fmcw Objects List
# ---------------------------------------------------
ifeq ($(CONFIG_SUPPORT_FMCW_RADAR), y)
    ifeq ($(CONFIG_MTK_WIFI_FMCW_VNI), y)
        OS_OBJS += $(OS_DIR)gl_fmcw.o
    endif
endif

# ---------------------------------------------------
# HE Objects List
# ---------------------------------------------------

COMMON_OBJS += $(COMMON_DIR)wlan_he.o

ifeq ($(CONFIG_MTK_WIFI_11AX_SUPPORT), y)
MGMT_OBJS += $(MGMT_DIR)he_ie.o \
             $(MGMT_DIR)he_rlm.o
endif

ifeq ($(CONFIG_MTK_WIFI_TWT_SUPPORT), y)
MGMT_OBJS += $(MGMT_DIR)twt_req_fsm.o \
             $(MGMT_DIR)twt.o \
             $(MGMT_DIR)twt_planner.o
endif

ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), sdio)
HIF_OBJS :=  $(HIF_DIR)arm.o \
             $(HIF_DIR)sdio.o \
             $(HIF_DIR)hal_api.o
else ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), pcie)
HIF_OBJS :=  $(HIF_COMMON_DIR)hal_pdma.o \
             $(HIF_COMMON_DIR)kal_pdma.o \
             $(HIF_COMMON_DIR)dbg_pdma.o \
             $(HIF_COMMON_DIR)hif_mem.o \
             $(HIF_DIR)pcie.o
ifeq ($(CONFIG_RX_PAGE_POOL_USE_CMA), y)
HIF_OBJS += $(HIF_COMMON_DIR)wifi_page_pool.o
endif
else ifeq ($(CONFIG_MTK_COMBO_WIFI_HIF), axi)
HIF_OBJS :=  $(HIF_COMMON_DIR)hal_pdma.o \
             $(HIF_COMMON_DIR)kal_pdma.o \
             $(HIF_COMMON_DIR)dbg_pdma.o \
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
ifneq ($(MTK_PLATFORM),)

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
ifeq ($(CONFIG_SYS_DVT_SUPPORT), 1)
SYSDVT_OBJS += $(SYSDVT_DIR)dvt_common.o

ifeq ($(CFG_SUPPORT_DMASHDL_SYSDVT), 1)
SYSDVT_OBJS += $(SYSDVT_DIR)dvt_dmashdl.o
endif
endif

# ---------------------------------------------------
# Service git List
# ---------------------------------------------------
SERVICE_DIR  := wlan_service/

ifneq ($(findstring wlan_service,$(MTK_WLAN_SERVICE_PATH)),)
MTK_WLAN_SERVICE=yes
SERVICE_DIR  := $(MTK_WLAN_SERVICE_PATH)
$(info SERVICE_DIR is [{$(MTK_WLAN_SERVICE_PATH)}])
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

#
# mtprealloc: driver support prealloc or not
#
ifeq ($(CONFIG_MTK_PREALLOC_MEMORY), y)
    ccflags-y += -DCFG_PREALLOC_MEMORY
    ccflags-y += -I$(src)/prealloc/include
endif

#
# mtprealloc: build mtprealloc.ko
#
ifeq ($(CONFIG_BUILD_MTPREALLOC), y)
    ifeq ($(MODULE_NAME_PREALLOC),)
        MODULE_NAME_PREALLOC = mtprealloc
    endif
    ccflags-y += -I$(src)/prealloc/include
    PREALLOC_OBJS := prealloc/prealloc.o
    $(MODULE_NAME_PREALLOC)-objs += $(PREALLOC_OBJS)
    obj-m += $(MODULE_NAME_PREALLOC).o
endif

#
# mtreset: driver support resetko or not
#
ifeq ($(CONFIG_CHIP_RESET_KO_SUPPORT), y)
    ccflags-y += -DCFG_CHIP_RESET_KO_SUPPORT=1
    ccflags-y += -I$(src)/reset/include
else
    ccflags-y += -DCFG_CHIP_RESET_KO_SUPPORT=0
endif

#
# mtreset: build mtreset.ko
#
ifeq ($(CONFIG_BUILD_RESETKO), y)
    ifeq ($(MODULE_NAME_RESET),)
        MODULE_NAME_RESET = mtreset
    endif
    ccflags-y += -I$(src)/reset/include
    RESET_OBJS += reset/reset.o
    RESET_OBJS += reset/reset_fsm.o
    RESET_OBJS += reset/reset_fsm_def.o
    RESET_OBJS += reset/reset_hif.o
    $(MODULE_NAME_RESET)-objs += $(RESET_OBJS)
    obj-m += $(MODULE_NAME_RESET).o
endif
