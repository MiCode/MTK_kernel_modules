/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   precomp.h
 *    \brief  Collection of most compiler flags are described here.
 *
 *    In this file we collect all compiler flags and detail the driver behavior
 *    if enable/disable such switch or adjust numeric parameters.
 */

#ifndef _PRECOMP_H
#define _PRECOMP_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

#ifdef __GNUC__
#if (DBG == 0)
#pragma GCC diagnostic ignored "-Wformat"
#endif
#endif

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

#if CFG_MTK_ANDROID_WMT
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
#include "connv3.h"
#endif
#endif

#include "hif_cmm.h"
#include "gl_os.h"		/* Include "config.h" */
#include "gl_sys_lock.h"
#include "gl_cfg80211.h"

#if CFG_ENABLE_WIFI_DIRECT
#include "gl_p2p_os.h"
#endif

#include "debug.h"
#include "fw_log_parser.h"

#include "link.h"
#include "queue.h"
#include "bitmap.h"

/*------------------------------------------------------------------------------
 * .\include\mgmt
 *------------------------------------------------------------------------------
 */
#include "wlan_typedef.h"

#include "mac.h"

/* Dependency:  mac.h (MAC_ADDR_LEN) */
#include "wlan_def.h"

#if (CFG_SUPPORT_802_11AX == 1)
#include "he_ie.h"
#endif

#if CFG_SUPPORT_SWCR
#include "swcr.h"
#endif

#include "rlm_obss.h"
#include "cnm_timer.h"

/*------------------------------------------------------------------------------
 * .\include\nic
 *------------------------------------------------------------------------------
 */
/* Dependency:  wlan_def.h (ENUM_NETWORK_TYPE_T) */
#include "cmd_buf.h"

/* Dependency:  mac.h (MAC_ADDR_LEN) */
#include "nic_cmd_event.h"
#include "nic_ext_cmd_event.h"
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
#include "nic_uni_cmd_event.h"
#endif

/* Dependency:  nic_cmd_event.h (P_EVENT_CONNECTION_STATUS) */
#include "nic.h"

#include "nic_init_cmd_event.h"

#include "hif_rx.h"
#include "hif_tx.h"

#include "nic_connac2x_tx.h"
#include "nic_connac3x_tx.h"
#include "nic_tx.h"
#include "nic_txd_v1.h"
#include "nic_txd_v2.h"
#include "nic_txd_v3.h"

#if (CFG_SUPPORT_CONNAC2X == 1)
#include "nic_rxd_v2.h"
#elif (CFG_SUPPORT_CONNAC3X == 1)
#include "nic_rxd_v3.h"
#else
#include "nic_rxd_v1.h"
#endif

#include "hal.h"
#include "nic_connac2x_rx.h"
#include "nic_connac3x_rx.h"
/* Dependency:  hif_rx.h (P_HIF_RX_HEADER_T) */
/* Dependency:  hal.h (RRO_COUNTER_NUM) */
#include "nic_rx.h"

#include "bss.h"

#include "nic_rate.h"

#if CFG_ENABLE_WIFI_DIRECT
#include "p2p_typedef.h"
#include "p2p_cmd_buf.h"
#include "p2p_nic_cmd_event.h"
#include "p2p_mac.h"
#include "p2p_nic.h"
#endif

/*------------------------------------------------------------------------------
 * .\include\mgmt
 *------------------------------------------------------------------------------
 */
#if (CFG_SUPPORT_TWT == 1)
#include "twt.h"
#endif /* CFG_SUPPORT_802_11AX */

#include "hem_mbox.h"

#include "scan.h"

#include "wlan_lib.h"
#include "wlan_oid.h"
#include "wlan_bow.h"

#include "fw_dl.h"

#if CFG_ENABLE_WIFI_DIRECT
#include "wlan_p2p.h"
#endif

#include "gl_emi.h"
#include "mt66xx_reg.h"

#include "connac_reg.h"
#include "connac_dmashdl.h"
#include "cmm_asic_common.h"
#include "cmm_asic_connac.h"
#include "cmm_asic_connac2x.h"
#include "cmm_asic_connac3x.h"
#include "pre_cal.h"

#if (CFG_SUPPORT_802_11AX == 1)
#include "he_rlm.h"
#include "wlan_he.h"
#endif /* CFG_SUPPORT_802_11AX == 1 */

#if (CFG_SUPPORT_802_11BE == 1)
#include "eht_rlm.h"
#endif

#if (CFG_SUPPORT_TWT == 1)
#include "twt_req_fsm.h"
#include "twt_planner.h"
#endif

#include "rlm.h"
#include "rlm_domain.h"
#include "rlm_protection.h"
#include "rlm_obss.h"
#include "rate.h"
#include "wnm.h"
#include "rrm.h"

#if (CFG_SUPPORT_FILS_SK_OFFLOAD == 1)
#include "fils.h"
#endif /* CFG_SUPPORT_FILS_SK_OFFLOAD */

#include "qosmap.h"

#include "aa_fsm.h"

#include "que_mgt.h"

#include "arp_mon.h"

#include "wmm.h"
#if CFG_ENABLE_BT_OVER_WIFI
#include "bow.h"
#include "bow_fsm.h"
#endif

#include "pwr_mgt.h"

#include "stats.h"

#include "cnm.h"
/* Dependency:  aa_fsm.h (ENUM_AA_STATE_T), p2p_fsm.h
 *   (WPS_ATTRI_MAX_LEN_DEVICE_NAME)
 */
#if (CFG_SUPPORT_802_11BE_T2LM == 1)
#include "t2lm.h"
#endif

#include "cnm_mem.h"
#include "cnm_scan.h"

#if CFG_ENABLE_WIFI_DIRECT
#include "p2p_rlm_obss.h"
#include "p2p_bss.h"
#include "p2p.h"

#include "p2p_rlm.h"
#include "p2p_assoc.h"
#include "p2p_ie.h"
#include "p2p_role.h"

#include "p2p_func.h"
#include "p2p_scan.h"
#include "p2p_dev.h"
#include "p2p_fsm.h"
#include "p2p_link.h"
#endif

#include "privacy.h"

#include "mib.h"

#include "auth.h"
#include "assoc.h"

#if CFG_SUPPORT_NAN
#include "gl_nan_os.h"
#include "gl_vendor_nan.h"
#include "nanDiscovery.h"
#include "nanScheduler.h"
#include "nanReg.h"
#include "nan_base.h"
#if CFG_SUPPORT_NAN_EXT
#include "nan_ext.h"
#endif
#include "nan_data_engine.h"
#include "nan_dev.h"
#include "nan_intf.h"
#include "nan_ranging.h"
#endif

#if CFG_SUPPORT_ROAMING
#include "roaming_fsm.h"
#endif /* CFG_SUPPORT_ROAMING */

#if CFG_SUPPORT_PASSPOINT
#include "hs20.h"
#endif /* CFG_SUPPORT_PASSPOINT */

/* Support AP Selection */
#if (CFG_SUPPORT_APS == 1)
#include "aps.h"
#else
#include "ap_selection.h"
#endif

#include "ais_fsm.h"
#include "rtt.h"

#include "gcm.h"

#include "fw_log.h"

#include "mscs.h"

#include "ccm.h"

#include "adapter.h"
#include "ccif.h"

#if (CFG_SUPPORT_802_11BE == 1)
#include "eht_ie.h"
#endif
#if (CFG_SUPPORT_802_11BE_MLO == 1)
#include "mlo.h"
#if (CFG_SUPPORT_802_11BE_EPCS == 1)
#include "epcs.h"
#endif
#endif

#include "que_mgt.h"
#include "rftest.h"

#include "rsn.h"

#if CFG_SUPPORT_WAPI
#include "wapi.h"
#endif

#include "thrm.h"

#if CFG_SUPPORT_MLR
#include "mlr.h"
#endif

/*------------------------------------------------------------------------------
 * NVRAM structure
 *------------------------------------------------------------------------------
 */
#include "CFG_Wifi_File.h"

#if CFG_ENABLE_WIFI_DIRECT
#include "gl_p2p_kal.h"
#endif

#if CFG_SUPPORT_TDLS
#include "tdls.h"
#endif

#if CFG_SUPPORT_QA_TOOL
#include "gl_qa_agent.h"
#include "gl_ate_agent.h"
#endif

#if CFG_SUPPORT_WIFI_SYSDVT
#include "dvt_common.h"
#include "dvt_phtput.h"
#if (CFG_SUPPORT_DMASHDL_SYSDVT)
#include "dvt_dmashdl.h"
#endif
#endif

#ifdef UT_TEST_MODE
#include "ut_lib.h"
#endif

#include "gl_wext_priv.h"

#include "ie_sort.h"

#if CFG_EXT_FEATURE
#include "log_ext.h"
#include "p2p_ext.h"
#include "roaming_ext.h"
#include "twt_ext.h"
#endif

#if CFG_SUPPORT_TDLS_AUTO
#include "tdls_auto.h"
#endif
/*------------------------------------------------------------------------------
 * Memory Prealloc
 *------------------------------------------------------------------------------
 */
#ifdef CFG_PREALLOC_MEMORY
#include "prealloc.h"
#endif

/*------------------------------------------------------------------------------
 * chip reset
 *------------------------------------------------------------------------------
 */
#if CFG_CHIP_RESET_KO_SUPPORT
#include "reset.h"
#endif

/*------------------------------------------------------------------------------
 * wed (wifi ethernet dispatch)
 *------------------------------------------------------------------------------
 */
#if CFG_SUPPORT_WED_PROXY
#include "hal_wed.h"
#endif

/*******************************************************************************
 *                              C O N S T A N T S
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
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#endif /* _PRECOMP_H */
