// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "ie_sort.c"
 *  \brief
 */

/******************************************************************************
 *                         C O M P I L E R   F L A G S
 ******************************************************************************
 */

/******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 ******************************************************************************
 */

#include "precomp.h"

/******************************************************************************
 *                              C O N S T A N T S
 ******************************************************************************
 */

#define SORT_ORDER_UNKNOWN 0xfff0
#define SORT_ORDER_LAST 0xffff

#define MAX_IE_NUM 50

/******************************************************************************
 *                             D A T A   T Y P E S
 ******************************************************************************
 */

struct IE_ORDER_TABLE_INFO {
	uint8_t eid;
	uint8_t extid;
	uint16_t size;
	uint8_t *ie;
};

/******************************************************************************
 *                            P U B L I C   D A T A
 ******************************************************************************
 */

/******************************************************************************
 *                           P R I V A T E   D A T A
 ******************************************************************************
 */
uint16_t AUTH_IE_ORDER[ELEM_ID_MAX_NUM + ELEM_EXT_ID_MAX_NUM] = {
	/* Challenge text */
	[ELEM_ID_CHALLENGE_TEXT] = 10,
	/* RSN */
	[ELEM_ID_RSN] = 11,
	/* Mobility Domain */
	[ELEM_ID_MOBILITY_DOMAIN] = 12,
	/* Fast BSS Transition */
	[ELEM_ID_FAST_TRANSITION] = 13,
	/* 802.11w SA Timeout interval */
	[ELEM_ID_TIMEOUT_INTERVAL] = 14,
	/* Resource Information Container for 802.11 R */
	[ELEM_ID_RESOURCE_INFO_CONTAINER] = 15,
	/* Multi-band */
	[ELEM_ID_MULTI_BAND] = 16,
	/* Neighbor Report */
	[ELEM_ID_NEIGHBOR_REPORT] = 17,
	/* FILS Nonce */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_FILS_NONCE] = 18,
	/* FILS Session */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_FILS_SESSION] = 19,
	/* FILS Wrapped Data */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_FILS_WRAPPED_DATA] = 20,
	/* Association Delay Info */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_ASSOC_DELAY_INFO] = 21,
	/* Rejected Groups */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_REJECTED_GROUPS] = 23,
	/* Anti-Clogging Token Container */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_ACTC] = 24,
	/* Multi-Link element */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_MLD] = 25,
};

/* 802.11-2020: Table 9-34 Association Request frame body */
uint16_t ASSOC_REQ_IE_ORDER[ELEM_ID_MAX_NUM + ELEM_EXT_ID_MAX_NUM] = {
	/* SSID */
	[ELEM_ID_SSID] = 3,
	/* Supported Rates and BSS Membership Selectors */
	[ELEM_ID_SUP_RATES] = 4,
	/* Extended Supported Rates and BSS Membership Selectors */
	[ELEM_ID_EXTENDED_SUP_RATES] = 5,
	/* Power Capability */
	[ELEM_ID_PWR_CAP] = 6,
	/* Supported Channels */
	[ELEM_ID_SUP_CHS] = 7,
	/* RSN */
	[ELEM_ID_RSN] = 8,
	/* QoS Capability */
	[ELEM_ID_QOS_CAP] = 9,
	/* RM Enabled Capabilities */
	[ELEM_ID_RRM_ENABLED_CAP] = 10,
	/* Mobility Domain */
	[ELEM_ID_MOBILITY_DOMAIN] = 11,
	/* Supported Operating Classes */
	[ELEM_ID_SUP_OPERATING_CLASS] = 12,
	/* HT Capabilities */
	[ELEM_ID_HT_CAP] = 13,
	/* 20/40 BSS Coexistence */
	[ELEM_ID_20_40_BSS_COEXISTENCE] = 14,
	/* Extended Capabilities */
	[ELEM_ID_EXTENDED_CAP] = 15,
	/* QoS Traffic Capability */
	[ELEM_ID_QOS_TRAFFIC_CAP] = 16,
	/* TIM Broadcast Request */
	[ELEM_ID_TIM_BROADCAST_REQ] = 17,
	/* Interworking */
	[ELEM_ID_INTERWORKING] = 18,
	/* Multi-band */
	[ELEM_ID_MULTI_BAND] = 19,
	/* DMG Capabilities */
	[ELEM_ID_DMG_CAP] = 20,
	/* Multiple MAC Sublayers */
	[ELEM_ID_MULTI_MAC_SUBLAYERS] = 21,
	/* VHT Capabilities */
	[ELEM_ID_VHT_CAP] = 22,
	/* Operating Mode Notification */
	[ELEM_ID_OP_MODE] = 23,
	/* FILS Session */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_FILS_SESSION] = 24,
	/* FILS Public Key */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_FILS_PUBLIC_KEY] = 25,
	/* FILS Key Confirmation */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_FILS_KEY_CONFIRM] = 26,
	/* FILS HLP Container */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_FILS_HLP_CONTAINER] = 27,
	/* FILS IP Address Assignment */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_FILS_IP_ADDR_ASSIGN] = 28,
	/* TWT */
	[ELEM_ID_TWT] = 29,
	/* AID Request */
	[ELEM_ID_AID_REQ] = 30,
	/* S1G Capabilities */
	[ELEM_ID_S1G_CAP] = 31,
	/* EL Operation */
	[ELEM_ID_EL_OP] = 32,
	/* S1G Relay */
	[ELEM_ID_S1G_RELAY] = 33,
	/* BSS Max Idle Period */
	[ELEM_ID_BSS_MAX_IDLE_PERIOD] = 34,
	/* Header Compression */
	[ELEM_ID_HEADER_COMPRESSION] = 35,
	/* MAD */
	[ELEM_ID_MAD] = 36,
	/* Reachable Address */
	[ELEM_ID_REACHABLE_ADDR] = 37,
	/* S1G Relay Activation */
	[ELEM_ID_S1G_RELAY_ACTIVATION] = 38,
	/* CDMG Capabilities */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_CDMG_CAP] = 39,
	/* CMMG Capabilities */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_CMMG_CAP] = 40,
	/* GLK-GCR Parameter Set */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_GLK_GCR_PARAM_SET] = 41,
	/* Fast BSS Transition */
	[ELEM_ID_FAST_TRANSITION] = 42,
	/* RSN Extension */
	[ELEM_ID_RSNX] = 43,
	/* Supplemental Class 2 Capabilities */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_SUPPLEMENTAL_CLASS2_CAP] = 44,
	/* MSCS Descriptor */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_MSCS_DESCRIPTOR] = 45,
	/* HE Capabilities */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_HE_CAP] = 46,
	/* Channel Switch Timing */
	[ELEM_ID_CH_SWITCH_TIMING] = 47,
	/* HE 6 GHz Band Capabilities */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_HE_6G_BAND_CAP] = 48,
	/* UL MU Power Capabilities */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_UL_MU_Power_CAP] = 49,
	/* Diffie-Hellman Parameter
	 * Note: This field is absent in order table of 802.11-2020
	 * but we need to handle it for OWE connection
	 */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_DIFFIE_HELLMAN_PARAM] = 99,
	/* Multi-Link element */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_MLD] = 100,
	/* EHT Capabilities */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_EHT_CAPS] = 101,
	/* TID2LNK */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_TID2LNK_MAP] = 102,

	/* Vendor Specific: should be present in the last */
	[ELEM_ID_VENDOR] = SORT_ORDER_LAST,
};

uint16_t ASSOC_RSP_IE_ORDER[ELEM_ID_MAX_NUM + ELEM_EXT_ID_MAX_NUM] = {
	/* Supported Rates and BSS Membership Selectors */
	[ELEM_ID_SUP_RATES] = 4,
	/* Extended Supported Rates and BSS Membership Selectors */
	[ELEM_ID_EXTENDED_SUP_RATES] = 5,
	/* EDCA parameter set */
	[ELEM_ID_EDCA_PARAM_SET] = 6,
	/* RCPI */
	[ELEM_ID_RCPI] = 7,
	/* RSNI */
	[ELEM_ID_RSNI] = 8,
	/* RM Enabled Capabilities */
	[ELEM_ID_RRM_ENABLED_CAP] = 9,
	/* RSN */
	[ELEM_ID_RSN] = 10,
	/* Mobility Domain */
	[ELEM_ID_MOBILITY_DOMAIN] = 11,
	/* Fast Bss Transition for 802.11 R */
	[ELEM_ID_FAST_TRANSITION] = 12,
	/* DSE Registered Location */
	[ELEM_ID_DSE_REG_LOC] = 13,
	/* 802.11w SA Timeout interval */
	[ELEM_ID_TIMEOUT_INTERVAL] = 14,
	/* HT Capabilities subelement */
	[ELEM_ID_HT_CAP] = 15,
	/* HT Operation */
	[ELEM_ID_HT_OP] = 16,
	/* 20/40 BSS Coexistence */
	[ELEM_ID_20_40_BSS_COEXISTENCE] = 17,
	/* Overlapping BSS Scan Parameters */
	[ELEM_ID_OBSS_SCAN_PARAMS] = 18,
	/* Extended Capabilities */
	[ELEM_ID_EXTENDED_CAP] = 19,
	/* BSS Max Idle Period */
	[ELEM_ID_BSS_MAX_IDLE_PERIOD] = 20,
	/* TIM Broadcast Response */
	[ELEM_ID_TIM_BROADCAST_RESP] = 21,
	/* QoS Map Set */
	[ELEM_ID_QOS_MAP_SET] = 22,
	/* QMF Policy */
	[ELEM_ID_QMF_POLICY] = 23,
	/* Multi-band */
	[ELEM_ID_MULTI_BAND] = 24,
	/* DMG Capabilities */
	[ELEM_ID_DMG_CAP] = 25,
	/* DMG Operation */
	[ELEM_ID_DMG_OP] = 26,
	/* Multiple MAC Sublayers */
	[ELEM_ID_MULTI_MAC_SUBLAYERS] = 27,
	/* Neighbor Report */
	[ELEM_ID_NEIGHBOR_REPORT] = 28,
	/* VHT Capabilities subelement */
	[ELEM_ID_VHT_CAP] = 29,
	/* VHT Operation information */
	[ELEM_ID_VHT_OP] = 30,
	/* Operation Mode Notification */
	[ELEM_ID_OP_MODE] = 31,
	/* Future Channel Guidance */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_FUTURE_CHNL_GUIDE] = 32,
	/* FILS Session */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_FILS_SESSION] = 33,
	/* FILS Public Key */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_FILS_PUBLIC_KEY] = 34,
	/* FILS Key Confirmation */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_FILS_KEY_CONFIRM] = 35,
	/* FILS HLP Container */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_FILS_HLP_CONTAINER] = 36,
	/* FILS IP Address Assignment */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_FILS_IP_ADDR_ASSIGN] = 37,
	/* Key Delivery */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_KEY_DELIVERY] = 38,
	/* S1G Sector Operation */
	[ELEM_ID_S1G_SECTOR_OP] = 39,
	/* TWT */
	[ELEM_ID_TWT] = 40,
	/* TSF Timer Accuracy */
	[ELEM_ID_TSF_TIMER_ACCURACY] = 41,
	/* S1G Relay */
	[ELEM_ID_S1G_CAP] = 42,
	/* S1G Operation */
	[ELEM_ID_S1G_OP] = 43,
	/* AID Response */
	[ELEM_ID_AID_RESP] = 44,
	/* Sectorized Group ID List */
	[ELEM_ID_SECTORIZED_GRP_ID_LIST] = 45,
	/* S1G Relay */
	[ELEM_ID_S1G_RELAY] = 46,
	/* Header Compression */
	[ELEM_ID_HEADER_COMPRESSION] = 47,
	/* SST Operation */
	[ELEM_ID_SST_OP] = 48,
	/* MAD */
	[ELEM_ID_MAD] = 49,
	/* S1G Relay Activation */
	[ELEM_ID_S1G_RELAY_ACTIVATION] = 50,
	/* CDMG Capabilities */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_CDMG_CAP] = 51,
	/* CMMG Capabilities */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_CMMG_CAP] = 52,
	/* CMMG Operation */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_CMMG_OP] = 53,
	/* GLK-GCR Parameter Set */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_GLK_GCR_PARAM_SET] = 54,
	/* RSN Extension */
	[ELEM_ID_RSNX] = 55,
	/* MSCS Descriptor */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_MSCS_DESCRIPTOR] = 56,
	/* HE Capabilities */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_HE_CAP] = 57,
	/* HE Operation */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_HE_OP] = 58,
	/* BSS Color Change Announcement */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_BSS_COLOR_CHANGE] = 59,
	/* Spatial Reuse Parameter Set element */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_SR_PARAM] = 60,
	/* MU EDCA Parameter Set element */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_MU_EDCA_PARAM] = 61,
	/* UL OFDMA-based Random Access (UORA) Parameter Set element */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_UORA_PARAM] = 62,
	/* ESS Report */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_ESS_REPORT] = 63,
	/* NDP Feedback Report Parameter Set */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_NDP_FEEDBACK] = 64,
	/* HE 6G Band Capabilities */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_HE_6G_BAND_CAP] = 65,
	/* Multi-Link element */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_MLD] = 66,
	/* EHT Capabilities */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_EHT_CAPS] = 67,
	/* EHT Operation */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_EHT_OP] = 68,
	/* TID2LNK */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_TID2LNK_MAP] = 69,


	/* Vendor Specific: should be present in the last */
	[ELEM_ID_VENDOR] = SORT_ORDER_LAST,
};

uint16_t REASSOC_REQ_IE_ORDER[ELEM_ID_MAX_NUM + ELEM_EXT_ID_MAX_NUM] = {
	/* SSID */
	[ELEM_ID_SSID] = 4,
	/* Supported Rates and BSS Membership Selectors */
	[ELEM_ID_SUP_RATES] = 5,
	/* Extended Supported Rates and BSS Membership Selectors */
	[ELEM_ID_EXTENDED_SUP_RATES] = 6,
	/* Power Capability */
	[ELEM_ID_PWR_CAP] = 7,
	/* Supported Channels */
	[ELEM_ID_SUP_CHS] = 8,
	/* RSN */
	[ELEM_ID_RSN] = 9,
	/* QoS Capability */
	[ELEM_ID_QOS_CAP] = 10,
	/* RM Enabled Capabilities */
	[ELEM_ID_RRM_ENABLED_CAP] = 11,
	/* Mobility Domain */
	[ELEM_ID_MOBILITY_DOMAIN] = 12,
	/* Fast Bss Transition for 802.11 R */
	[ELEM_ID_FAST_TRANSITION] = 13,
	/* Resource Information Container for 802.11 R */
	[ELEM_ID_RESOURCE_INFO_CONTAINER] = 14,
	/* Supported Operating Classes */
	[ELEM_ID_SUP_OPERATING_CLASS] = 15,
	/* HT Capabilities */
	[ELEM_ID_HT_CAP] = 16,
	/* 20/40 BSS Coexistence */
	[ELEM_ID_20_40_BSS_COEXISTENCE] = 17,
	/* Extended Capabilities */
	[ELEM_ID_EXTENDED_CAP] = 18,
	/* QoS Traffic Capability */
	[ELEM_ID_QOS_TRAFFIC_CAP] = 19,
	/* TIM Broadcast Request */
	[ELEM_ID_TIM_BROADCAST_REQ] = 20,
	/* FMS Request */
	[ELEM_ID_FMS_REQ] = 21,
	/* DMS Request */
	[ELEM_ID_DMS_REQ] = 22,
	/* Interworking */
	[ELEM_ID_INTERWORKING] = 23,
	/* Multi-band */
	[ELEM_ID_MULTI_BAND] = 24,
	/* DMG Capabilities */
	[ELEM_ID_DMG_CAP] = 25,
	/* Multiple MAC Sublayers */
	[ELEM_ID_MULTI_MAC_SUBLAYERS] = 26,
	/* VHT Capabilities */
	[ELEM_ID_VHT_CAP] = 27,
	/* Operating Mode Notification */
	[ELEM_ID_OP_MODE] = 28,
	/* FILS Session */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_FILS_SESSION] = 29,
	/* FILS Public Key */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_FILS_PUBLIC_KEY] = 30,
	/* FILS Key Confirmation */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_FILS_KEY_CONFIRM] = 31,
	/* FILS HLP Container */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_FILS_HLP_CONTAINER] = 32,
	/* FILS IP Address Assignment */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_FILS_IP_ADDR_ASSIGN] = 33,
	/* TWT */
	[ELEM_ID_TWT] = 34,
	/* AID Request */
	[ELEM_ID_AID_REQ] = 35,
	/* S1G Capabilities */
	[ELEM_ID_S1G_CAP] = 36,
	/* EL Operation */
	[ELEM_ID_EL_OP] = 37,
	/* BSS Max Idle Period */
	[ELEM_ID_BSS_MAX_IDLE_PERIOD] = 38,
	/* S1G Relay */
	[ELEM_ID_S1G_RELAY] = 39,
	/* Header Compression */
	[ELEM_ID_HEADER_COMPRESSION] = 40,
	/* MAD */
	[ELEM_ID_MAD] = 41,
	/* Reachable Address */
	[ELEM_ID_REACHABLE_ADDR] = 42,
	/* S1G Relay Activation */
	[ELEM_ID_S1G_RELAY_ACTIVATION] = 43,
	/* CDMG Capabilities */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_CDMG_CAP] = 44,
	/* CMMG Capabilities */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_CMMG_CAP] = 45,
	/* Operating Channel Information */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_OCI] = 46,
	/* GLK-GCR Parameter Set */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_GLK_GCR_PARAM_SET] = 47,
	/* RSN Extension */
	[ELEM_ID_RSNX] = 48,
	/* Supplemental Class 2 Capabilities */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_SUPPLEMENTAL_CLASS2_CAP] = 49,
	/* MSCS Descriptor */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_MSCS_DESCRIPTOR] = 50,
	/* HE Capabilities */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_HE_CAP] = 51,
	/* Channel Switch Timing */
	[ELEM_ID_CH_SWITCH_TIMING] = 52,
	/* HE 6 GHz Band Capabilities */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_HE_6G_BAND_CAP] = 53,
	/* UL MU Power Capabilities */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_UL_MU_Power_CAP] = 54,
	/* Multi-Link element */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_MLD] = 55,
	/* EHT Capabilities */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_EHT_CAPS] = 56,
	/* TID2LNK */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_TID2LNK_MAP] = 57,

	/* Vendor Specific: should be present in the last */
	[ELEM_ID_VENDOR] = SORT_ORDER_LAST,
};

uint16_t REASSOC_RSP_IE_ORDER[ELEM_ID_MAX_NUM + ELEM_EXT_ID_MAX_NUM] = {
	/* Supported Rates and BSS Membership Selectors */
	[ELEM_ID_SUP_RATES] = 4,
	/* Extended Supported Rates and BSS Membership Selectors */
	[ELEM_ID_EXTENDED_SUP_RATES] = 5,
	/* EDCA parameter set */
	[ELEM_ID_EDCA_PARAM_SET] = 6,
	/* RCPI */
	[ELEM_ID_RCPI] = 7,
	/* RSNI */
	[ELEM_ID_RSNI] = 8,
	/* RM Enabled Capabilities */
	[ELEM_ID_RRM_ENABLED_CAP] = 9,
	/* RSN */
	[ELEM_ID_RSN] = 10,
	/* Mobility Domain */
	[ELEM_ID_MOBILITY_DOMAIN] = 11,
	/* Fast Bss Transition for 802.11 R */
	[ELEM_ID_FAST_TRANSITION] = 12,
	/* Resource Information Container for 802.11 R */
	[ELEM_ID_RESOURCE_INFO_CONTAINER] = 13,
	/* DSE Registered Location */
	[ELEM_ID_DSE_REG_LOC] = 14,
	/* 802.11w SA Timeout interval */
	[ELEM_ID_TIMEOUT_INTERVAL] = 15,
	/* HT Capabilities subelement */
	[ELEM_ID_HT_CAP] = 16,
	/* HT Operation */
	[ELEM_ID_HT_OP] = 17,
	/* 20/40 BSS Coexistence */
	[ELEM_ID_20_40_BSS_COEXISTENCE] = 18,
	/* Overlapping BSS Scan Parameters */
	[ELEM_ID_OBSS_SCAN_PARAMS] = 19,
	/* Extended Capabilities */
	[ELEM_ID_EXTENDED_CAP] = 20,
	/* BSS Max Idle Period */
	[ELEM_ID_BSS_MAX_IDLE_PERIOD] = 21,
	/* TIM Broadcast Response */
	[ELEM_ID_TIM_BROADCAST_RESP] = 22,
	/* FMS Response */
	[ELEM_ID_FMS_RSP] = 23,
	/* DMS Response */
	[ELEM_ID_DMS_RSP] = 24,
	/* QoS Map Set */
	[ELEM_ID_QOS_MAP_SET] = 25,
	/* QMF Policy */
	[ELEM_ID_QMF_POLICY] = 26,
	/* Multi-band */
	[ELEM_ID_MULTI_BAND] = 27,
	/* DMG Capabilities */
	[ELEM_ID_DMG_CAP] = 28,
	/* DMG Operation */
	[ELEM_ID_DMG_OP] = 29,
	/* Multiple MAC Sublayers */
	[ELEM_ID_MULTI_MAC_SUBLAYERS] = 30,
	/* FILS HLP Container */
	[ELEM_ID_NEIGHBOR_REPORT] = 31,
	/* VHT Capabilities subelement */
	[ELEM_ID_VHT_CAP] = 32,
	/* VHT Operation information */
	[ELEM_ID_VHT_OP] = 33,
	/* Operation Mode Notification */
	[ELEM_ID_OP_MODE] = 34,
	/* Future Channel Guidance */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_FUTURE_CHNL_GUIDE] = 35,
	/* FILS Session */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_FILS_SESSION] = 36,
	/* FILS Public Key */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_FILS_PUBLIC_KEY] = 37,
	/* FILS Key Confirmation */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_FILS_KEY_CONFIRM] = 38,
	/* FILS HLP Container */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_FILS_HLP_CONTAINER] = 39,
	/* FILS IP Address Assignment */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_FILS_IP_ADDR_ASSIGN] = 40,
	/* Key Delivery */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_KEY_DELIVERY] = 41,
	/* S1G Sector Operation */
	[ELEM_ID_S1G_SECTOR_OP] = 42,
	/* TWT */
	[ELEM_ID_TWT] = 43,
	/* TSF Timer Accuracy */
	[ELEM_ID_TSF_TIMER_ACCURACY] = 44,
	/* S1G Relay */
	[ELEM_ID_S1G_CAP] = 45,
	/* S1G Operation */
	[ELEM_ID_S1G_OP] = 46,
	/* AID Response */
	[ELEM_ID_AID_RESP] = 47,
	/* Sectorized Group ID List */
	[ELEM_ID_SECTORIZED_GRP_ID_LIST] = 48,
	/* S1G Relay */
	[ELEM_ID_S1G_RELAY] = 49,
	/* Header Compression */
	[ELEM_ID_HEADER_COMPRESSION] = 50,
	/* SST Operation */
	[ELEM_ID_SST_OP] = 51,
	/* MAD */
	[ELEM_ID_MAD] = 52,
	/* S1G Relay Activation */
	[ELEM_ID_S1G_RELAY_ACTIVATION] = 53,
	/* CDMG Capabilities */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_CDMG_CAP] = 54,
	/* CMMG Capabilities */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_CMMG_CAP] = 55,
	/* CMMG Operation */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_CMMG_OP] = 56,
	/* Operating Channel Information */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_OCI] = 57,
	/* GLK-GCR Parameter Set */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_GLK_GCR_PARAM_SET] = 58,
	/* RSN Extension */
	[ELEM_ID_RSNX] = 59,
	/* MSCS Descriptor */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_MSCS_DESCRIPTOR] = 60,
	/* HE Capabilities */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_HE_CAP] = 61,
	/* HE Operation */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_HE_OP] = 62,
	/* BSS Color Change Announcement */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_BSS_COLOR_CHANGE] = 63,
	/* Spatial Reuse Parameter Set element */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_SR_PARAM] = 64,
	/* MU EDCA Parameter Set element */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_MU_EDCA_PARAM] = 65,
	/* UL OFDMA-based Random Access (UORA) Parameter Set element */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_UORA_PARAM] = 66,
	/* ESS Report */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_ESS_REPORT] = 67,
	/* NDP Feedback Report Parameter Set */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_NDP_FEEDBACK] = 68,
	/* HE 6G Band Capabilities */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_HE_6G_BAND_CAP] = 69,
	/* Multi-Link element */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_MLD] = 70,
	/* EHT Capabilities */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_EHT_CAPS] = 71,
	/* EHT Operation */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_EHT_OP] = 72,
	/* TID2LNK */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_TID2LNK_MAP] = 73,

	/* Vendor Specific: should be present in the last */
	[ELEM_ID_VENDOR] = SORT_ORDER_LAST,
};

uint16_t PROBE_RSP_IE_ORDER[ELEM_ID_MAX_NUM + ELEM_EXT_ID_MAX_NUM] = {
	/* SSID */
	[ELEM_ID_SSID] = 4,
	/* Supported Rates and BSS Membership Selectors */
	[ELEM_ID_SUP_RATES] = 5,
	/* DS parameter set  */
	[ELEM_ID_DS_PARAM_SET] = 6,
	/* IBSS parameter set  */
	[ELEM_ID_IBSS_PARAM_SET] = 7,
	/* Country information */
	[ELEM_ID_COUNTRY_INFO] = 8,
	/* Power constraint */
	[ELEM_ID_PWR_CONSTRAINT] = 9,
	/* Channel switch announcement */
	[ELEM_ID_CH_SW_ANNOUNCEMENT] = 10,
	/* Quiet */
	[ELEM_ID_QUIET] = 11,
	/* IBSS DFS */
	[ELEM_ID_IBSS_DFS] = 12,
	/* TPC report */
	[ELEM_ID_TPC_REPORT] = 13,
	/* ERP information */
	[ELEM_ID_ERP_INFO] = 14,
	/* Extended Supported Rates and BSS Membership Selectors */
	[ELEM_ID_EXTENDED_SUP_RATES] = 15,
	/* RSN */
	[ELEM_ID_RSN] = 16,
	/* BSS load */
	[ELEM_ID_BSS_LOAD] = 17,
	/* EDCA parameter set */
	[ELEM_ID_EDCA_PARAM_SET] = 18,
	/* Measurement Pilot Transmission */
	[ELEM_ID_MPT] = 19,
	/* Multiple BSSID element */
	[ELEM_ID_MBSSID] = 20,
	/* RM Enabled Capabilities */
	[ELEM_ID_RRM_ENABLED_CAP] = 21,
	/*  AP Channel Report Element */
	[ELEM_ID_AP_CHANNEL_REPORT] = 22,
	/* BSS Average Access Delay */
	[ELEM_ID_BSS_AVG_ACCESS_DELAY] = 23,
	/* Antenna */
	[ELEM_ID_ANTENNA] = 24,
	/* BSS Available Admission Capacity */
	[ELEM_ID_BSS_AVAILABLE_ADMIN_CAP] = 25,
	/* BSS AC Access Delay */
	[ELEM_ID_BSS_AC_ACCESS_DELAY] = 26,
	/* Mobility Domain */
	[ELEM_ID_MOBILITY_DOMAIN] = 27,
	/* DSE Registered Location */
	[ELEM_ID_DSE_REG_LOC] = 28,
	/* Extended Channel Switch Announcement */
	[ELEM_ID_EX_CH_SW_ANNOUNCEMENT] = 29,
	/* Supported Operating Classes */
	[ELEM_ID_SUP_OPERATING_CLASS] = 30,
	/* HT Capabilities subelement */
	[ELEM_ID_HT_CAP] = 31,
	/* HT Operation */
	[ELEM_ID_HT_OP] = 32,
	/* 20/40 BSS Coexistence */
	[ELEM_ID_20_40_BSS_COEXISTENCE] = 33,
	/* Overlapping BSS Scan Parameters */
	[ELEM_ID_OBSS_SCAN_PARAMS] = 34,
	/* Extended Capabilities */
	[ELEM_ID_EXTENDED_CAP] = 35,
	/* QoS Traffic Capability */
	[ELEM_ID_QOS_TRAFFIC_CAP] = 36,
	/* Channel Usage */
	[ELEM_ID_CHNNEL_USAGE] = 37,
	/* Time Advertisement */
	[ELEM_ID_TIME_AD] = 38,
	/* Time Zone */
	[ELEM_ID_TIME_ZONE] = 39,
	/* Interworking */
	[ELEM_ID_INTERWORKING] = 40,
	/* Advertisement Protocol */
	[ELEM_ID_ADVERTISEMENT_PROTOCOL] = 41,
	/*  Roaming Consortium */
	[ELEM_ID_ROAMING_CONSORTIUM] = 42,
	/* Emergency Alert Identifier */
	[ELEM_ID_EAI] = 43,
	/* Mesh ID */
	[ELEM_ID_MESH_ID] = 44,
	/* Mesh Configuration */
	[ELEM_ID_MESH_CONFIG] = 45,
	/* Mesh Awake Window */
	[ELEM_ID_MESH_AWAKE_WINDOW] = 46,
	/* Beacon Timing */
	[ELEM_ID_BEACON_TIMING] = 47,
	/* MCCAOP Advertisement Overview */
	[ELEM_ID_MCCAOP_AD_OVERVIEW] = 48,
	/* MCCAOP Advertisement */
	[ELEM_ID_MCCAOP_AD] = 49,
	/* Mesh Channel Switch Parameters */
	[ELEM_ID_MESH_CH_SW_PARAM] = 50,
	/* QMF Policy */
	[ELEM_ID_QMF_POLICY] = 51,
	/* QLoad Report */
	[ELEM_ID_QLOAD_REPORT] = 52,
	/* Multi-band */
	[ELEM_ID_MULTI_BAND] = 53,
	/* DMG Capabilities */
	[ELEM_ID_DMG_CAP] = 54,
	/* DMG Operation */
	[ELEM_ID_DMG_OP] = 55,
	/* Multiple MAC Sublayers */
	[ELEM_ID_MULTI_MAC_SUBLAYERS] = 56,
	/* Antenna Sector ID Pattern */
	[ELEM_ID_ANT_SECTOR_ID_PATTERN] = 57,
	/* VHT Capabilities */
	[ELEM_ID_VHT_CAP] = 58,
	/* VHT Operation information */
	[ELEM_ID_VHT_OP] = 59,
	/* Transmit Power Envelope */
	[ELEM_ID_TX_PWR_ENVELOPE] = 60,
	/* Channel Switch Wrapper */
	[ELEM_ID_CH_SW_WRAPPER] = 61,
	/* Extended BSS Load */
	[ELEM_ID_EX_BSS_LOAD] = 62,
	/* Quiet Channel */
	[ELEM_ID_QUIET_CHANNEL] = 63,
	/* Operating Mode Notification */
	[ELEM_ID_OP_MODE] = 64,
	/* Reduced Neighbor Report */
	[ELEM_ID_RNR] = 65,
	/* TVHT Operation */
	[ELEM_ID_TVHT_OP] = 66,
	/* Estimated Service Parameters */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_ESP] = 67,
	/* Relay Capabilities */
	[ELEM_ID_RELAY_CAP] = 68,
	/* CAG Number */
	[ELEM_ID_CAG_NUM] = 69,
	/* FILS Indication */
	[ELEM_ID_FILS_INDICATION] = 70,
	/* AP-CSN */
	[ELEM_ID_AP_CSN] = 71,
	/* DILS */
	[ELEM_ID_DILS] = 72,
	/* EL Operation */
	[ELEM_ID_RPS] = 73,
	/* Page Slice */
	[ELEM_ID_PAGE_SLICE] = 74,
	/* Change Sequence */
	[ELEM_ID_CHANGE_SEQ] = 75,
	/* TSF Timer Accuracy */
	[ELEM_ID_TSF_TIMER_ACCURACY] = 76,
	/* S1G Relay Discovery */
	[ELEM_ID_S1G_RELAY_DISCOVERY] = 77,
	/* S1G Capabilities */
	[ELEM_ID_S1G_CAP] = 78,
	/* S1G Operation */
	[ELEM_ID_S1G_OP] = 79,
	/* MAD */
	[ELEM_ID_MAD] = 80,
	/* Short Beacon Interval */
	[ELEM_ID_SHORT_BEACON_INTERVAL] = 81,
	/* S1G Open-Loop Link Margin Index */
	[ELEM_ID_S1G_OLLM_INDEX] = 82,
	/* S1G Relay */
	[ELEM_ID_S1G_RELAY] = 83,
	/* Max Channel Switch Time */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_MAX_CH_SW_TIME] = 84,
	/* CDMG Capabilities */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_CDMG_CAP] = 85,
	/* Extended Cluster Report */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_EX_CLUSTER_REPORT] = 86,
	/* CMMG Capabilities */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_CMMG_CAP] = 87,
	/* CMMG Operation */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_CMMG_OP] = 88,
	/* Estimated Service Parameters Outbound */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_ESP_OUTBOUND] = 89,
	/* Service Hint */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_SERVICE_HINT] = 90,
	/* Service Hash */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_SERVICE_HASH] = 91,
	/* RSN Extension */
	[ELEM_ID_RSNX] = 92,

	/* Multiple BSSD Configuration */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_MBSS_CONFIG] = 93,
	/* HE Capabilities */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_HE_CAP] = 94,
	/* HE Operation */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_HE_OP] = 95,
	/* TWT */
	[ELEM_ID_TWT] = 96,
	/* UL OFDMA-based Random Access (UORA) Parameter Set element */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_UORA_PARAM] = 97,
	/* BSS Color Change Announcement */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_BSS_COLOR_CHANGE] = 98,
	/* Spatial Reuse Parameter Set element */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_SR_PARAM] = 99,
	/* MU EDCA Parameter Set element */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_MU_EDCA_PARAM] = 100,
	/* ESS Report */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_ESS_REPORT] = 101,
	/* NDP Feedback Report Parameter Set */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_NDP_FEEDBACK] = 102,
	/* NDP Feedback Report Parameter Set */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_HE_BSS_LOAD] = 103,
	/* HE 6G Band Capabilities */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_HE_6G_BAND_CAP] = 104,
	/* Multi-Link element */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_MLD] = 105,
	/* EHT Capabilities */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_EHT_CAPS] = 106,
	/* EHT Operation */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_EHT_OP] = 107,

	/* Vendor Specific: should be present in the last */
	[ELEM_ID_VENDOR] = SORT_ORDER_LAST - 1,
	/* Request */
	[ELEM_ID_REQUEST] = SORT_ORDER_LAST,
};

uint16_t BEACON_IE_ORDER[ELEM_ID_MAX_NUM + ELEM_EXT_ID_MAX_NUM] = {
	/* SSID */
	[ELEM_ID_SSID] = 4,
	/* Supported Rates and BSS Membership Selectors */
	[ELEM_ID_SUP_RATES] = 5,
	/* DS parameter set  */
	[ELEM_ID_DS_PARAM_SET] = 6,
	/* IBSS parameter set  */
	[ELEM_ID_IBSS_PARAM_SET] = 7,
	/* TIM */
	[ELEM_ID_TIM] = 8,
	/* Country information */
	[ELEM_ID_COUNTRY_INFO] = 9,
	/* Power constraint */
	[ELEM_ID_PWR_CONSTRAINT] = 10,
	/* Channel switch announcement */
	[ELEM_ID_CH_SW_ANNOUNCEMENT] = 11,
	/* Quiet */
	[ELEM_ID_QUIET] = 112,
	/* IBSS DFS */
	[ELEM_ID_IBSS_DFS] = 13,
	/* TPC report */
	[ELEM_ID_TPC_REPORT] = 14,
	/* ERP information */
	[ELEM_ID_ERP_INFO] = 15,
	/* Extended Supported Rates and BSS Membership Selectors */
	[ELEM_ID_EXTENDED_SUP_RATES] = 16,
	/* RSN */
	[ELEM_ID_RSN] = 17,
	/* BSS load */
	[ELEM_ID_BSS_LOAD] = 18,
	/* EDCA parameter set */
	[ELEM_ID_EDCA_PARAM_SET] = 19,
	/* QoS capability */
	[ELEM_ID_QOS_CAP] = 20,
	/*  AP Channel Report Element */
	[ELEM_ID_AP_CHANNEL_REPORT] = 21,
	/* BSS Average Access Delay */
	[ELEM_ID_BSS_AVG_ACCESS_DELAY] = 22,
	/* Antenna */
	[ELEM_ID_ANTENNA] = 23,
	/* BSS Available Admission Capacity */
	[ELEM_ID_BSS_AVAILABLE_ADMIN_CAP] = 24,
	/* BSS AC Access Delay */
	[ELEM_ID_BSS_AC_ACCESS_DELAY] = 25,
	/* Measurement Pilot Transmission */
	[ELEM_ID_MPT] = 26,
	/* Multiple BSSID element */
	[ELEM_ID_MBSSID] = 27,
	/* RM Enabled Capabilities */
	[ELEM_ID_RRM_ENABLED_CAP] = 28,
	/* Mobility Domain */
	[ELEM_ID_MOBILITY_DOMAIN] = 29,
	/* DSE Registered Location */
	[ELEM_ID_DSE_REG_LOC] = 30,
	/* Extended Channel Switch Announcement */
	[ELEM_ID_EX_CH_SW_ANNOUNCEMENT] = 31,
	/* Supported Operating Classes */
	[ELEM_ID_SUP_OPERATING_CLASS] = 32,
	/* HT Capabilities subelement */
	[ELEM_ID_HT_CAP] = 33,
	/* HT Operation */
	[ELEM_ID_HT_OP] = 34,
	/* 20/40 BSS Coexistence */
	[ELEM_ID_20_40_BSS_COEXISTENCE] = 35,
	/* Overlapping BSS Scan Parameters */
	[ELEM_ID_OBSS_SCAN_PARAMS] = 36,
	/* Extended Capabilities */
	[ELEM_ID_EXTENDED_CAP] = 37,
	/* FMS Descriptor */
	[ELEM_ID_FMS_DESC] = 38,
	/* QoS Traffic Capability */
	[ELEM_ID_QOS_TRAFFIC_CAP] = 39,
	/* Time Advertisement */
	[ELEM_ID_TIME_AD] = 40,
	/* Interworking */
	[ELEM_ID_INTERWORKING] = 41,
	/* Advertisement Protocol */
	[ELEM_ID_ADVERTISEMENT_PROTOCOL] = 42,
	/*  Roaming Consortium */
	[ELEM_ID_ROAMING_CONSORTIUM] = 43,
	/* Emergency Alert Identifier */
	[ELEM_ID_EAI] = 44,
	/* Mesh ID */
	[ELEM_ID_MESH_ID] = 45,
	/* Mesh Configuration */
	[ELEM_ID_MESH_CONFIG] = 46,
	/* Mesh Awake Window */
	[ELEM_ID_MESH_AWAKE_WINDOW] = 47,
	/* Beacon Timing */
	[ELEM_ID_BEACON_TIMING] = 48,
	/* MCCAOP Advertisement Overview */
	[ELEM_ID_MCCAOP_AD_OVERVIEW] = 49,
	/* MCCAOP Advertisement */
	[ELEM_ID_MCCAOP_AD] = 50,
	/* Mesh Channel Switch Parameters */
	[ELEM_ID_MESH_CH_SW_PARAM] = 51,
	/* QMF Policy */
	[ELEM_ID_QMF_POLICY] = 52,
	/* QLoad Report */
	[ELEM_ID_QLOAD_REPORT] = 53,
	/* HCCA TXOP Update Count */
	[ELEM_ID_HCCA_TXOP_UPDATE_COUNT] = 54,
	/* Multi-band */
	[ELEM_ID_MULTI_BAND] = 55,
	/* VHT Capabilities */
	[ELEM_ID_VHT_CAP] = 56,
	/* VHT Operation information */
	[ELEM_ID_VHT_OP] = 57,
	/* Transmit Power Envelope */
	[ELEM_ID_TX_PWR_ENVELOPE] = 58,
	/* Channel Switch Wrapper */
	[ELEM_ID_CH_SW_WRAPPER] = 59,
	/* Extended BSS Load */
	[ELEM_ID_EX_BSS_LOAD] = 60,
	/* Quiet Channel */
	[ELEM_ID_QUIET_CHANNEL] = 61,
	/* Operating Mode Notification */
	[ELEM_ID_OP_MODE] = 62,
	/* Reduced Neighbor Report */
	[ELEM_ID_RNR] = 63,
	/* TVHT Operation */
	[ELEM_ID_TVHT_OP] = 64,
	/* Estimated Service Parameters */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_ESP] = 65,
	/* Future Channel Guidance */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_FUTURE_CHNL_GUIDE] = 66,
	/* CAG Number */
	[ELEM_ID_CAG_NUM] = 67,
	/* FILS Indication */
	[ELEM_ID_FILS_INDICATION] = 68,
	/* AP-CSN */
	[ELEM_ID_AP_CSN] = 69,
	/* DILS */
	[ELEM_ID_DILS] = 70,
	/* Max Channel Switch Time */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_MAX_CH_SW_TIME] = 71,
	/* Estimated Service Parameters Outbound */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_ESP_OUTBOUND] = 72,
	/* Service Hint */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_SERVICE_HINT] = 73,
	/* Service Hash */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_SERVICE_HASH] = 74,
	/* RSN Extension */
	[ELEM_ID_RSNX] = 75,

	/* Multiple BSSD Configuration */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_MBSS_CONFIG] = 76,
	/* HE Capabilities */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_HE_CAP] = 77,
	/* HE Operation */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_HE_OP] = 78,
	/* TWT */
	[ELEM_ID_TWT] = 79,
	/* UL OFDMA-based Random Access (UORA) Parameter Set element */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_UORA_PARAM] = 80,
	/* BSS Color Change Announcement */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_BSS_COLOR_CHANGE] = 81,
	/* Spatial Reuse Parameter Set element */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_SR_PARAM] = 82,
	/* MU EDCA Parameter Set element */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_MU_EDCA_PARAM] = 83,
	/* ESS Report */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_ESS_REPORT] = 84,
	/* NDP Feedback Report Parameter Set */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_NDP_FEEDBACK] = 85,
	/* NDP Feedback Report Parameter Set */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_HE_BSS_LOAD] = 86,
	/* HE 6G Band Capabilities */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_HE_6G_BAND_CAP] = 87,
	/* Multi-Link element */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_MLD] = 88,
	/* EHT Capabilities */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_EHT_CAPS] = 89,
	/* EHT Operation */
	[ELEM_ID_RESERVED + ELEM_EXT_ID_EHT_OP] = 90,


	/* Vendor Specific: should be present in the last */
	[ELEM_ID_VENDOR] = SORT_ORDER_LAST,
};

/******************************************************************************
 *                                 M A C R O S
 ******************************************************************************
 */

/******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 ******************************************************************************
 */

/******************************************************************************
 *                              F U N C T I O N S
 ******************************************************************************
 */

void sortSwap(struct IE_ORDER_TABLE_INFO *a, struct IE_ORDER_TABLE_INFO *b)
{
	struct IE_ORDER_TABLE_INFO temp;

	kalMemCopy(&temp, a, sizeof(temp));
	kalMemCopy(a, b, sizeof(temp));
	kalMemCopy(b, &temp, sizeof(temp));
}

int sortPartition(struct IE_ORDER_TABLE_INFO *arr,
	uint16_t *order, int front, int end)
{
	int pivot = order[arr[end].eid + arr[end].extid];
	int i = front -1, j;

	for (j = front; j < end; j++) {
		if (order[arr[j].eid + arr[j].extid] < pivot) {
			i++;
			sortSwap(&arr[i], &arr[j]);
		}
	}
	i++;
	sortSwap(&arr[i], &arr[end]);

	return i;
}

void sortQuickSort(struct IE_ORDER_TABLE_INFO *arr,
	uint16_t *order, int front, int end)
{
	if (front < end) {
		int pivot = sortPartition(arr, order, front, end);

		sortQuickSort(arr, order, front, pivot - 1);
		sortQuickSort(arr, order, pivot + 1, end);
	}
}

void sortIE(struct ADAPTER *prAdapter,
	    struct MSDU_INFO *prMsduInfo,
	    uint16_t *apu2OrderTable,
	    const char *pucIeDesc)
{
	uint16_t u2Offset = 0, u2IEsBufLen, u2LastLen;
	uint8_t *pucBuf, *pucDst;
	struct IE_ORDER_TABLE_INFO *info = NULL;
	uint8_t num = 0, i;
	struct MSDU_INFO *prMsduInfoInOrder = NULL;
	int offset = sortMsduPayloadOffset(prAdapter, prMsduInfo);
	const uint8_t *pos, *end;

	if (offset < 0) {
		DBGLOG(TX, ERROR, "Unsupported mgmt frame\n");
		return;
	} else if (prMsduInfo->u2FrameLength < offset) {
		DBGLOG(TX, ERROR, "frame len %d < offset %d\n",
			prMsduInfo->u2FrameLength, offset);
		return;
	} else if (prMsduInfo->u2FrameLength == offset) {
		DBGLOG(TX, DEBUG, "No payload, skip sorting\n");
		return;
	}

	prMsduInfoInOrder = cnmMgtPktAlloc(prAdapter,
		prMsduInfo->u2FrameLength - offset);
	if (prMsduInfoInOrder == NULL) {
		DBGLOG(TX, WARN, "No PKT_INFO_T for sort.\n");
		goto done;
	}

	info = kalMemAlloc(MAX_IE_NUM * sizeof(struct IE_ORDER_TABLE_INFO),
		VIR_MEM_TYPE);
	if (info == NULL) {
		DBGLOG(TX, WARN, "No table info for sort.\n");
		goto done;
	}

	pucBuf = (uint8_t *)prMsduInfo->prPacket + offset;
	u2IEsBufLen = prMsduInfo->u2FrameLength - offset;
	end = pucBuf + u2IEsBufLen;

#if DBG
	DBGLOG(TX, LOUD, "%s IE, length = %d\n", pucIeDesc, u2IEsBufLen);
	dumpMemory8(pucBuf, u2IEsBufLen);
#endif

	/* prepare ie info table */
	IE_FOR_EACH(pucBuf, u2IEsBufLen, u2Offset) {
		if (IE_ID(pucBuf) == ELEM_ID_FRAGMENT)
			continue;

		if (num > MAX_IE_NUM) {
			DBGLOG(TX, ERROR, "too many IE > %d\n", MAX_IE_NUM);
			break;
		}
		info[num].eid = IE_ID(pucBuf);
		info[num].extid = info[num].eid == ELEM_ID_RESERVED ?
			IE_ID_EXT(pucBuf) : 0;
		info[num].ie = pucBuf;
		info[num].size = IE_SIZE(pucBuf);

		/* start from next ie to find fragments */
		pos = pucBuf + IE_SIZE(pucBuf);
		u2LastLen = IE_SIZE(pucBuf);
		if (IE_ID(pucBuf) == ELEM_ID_VENDOR) {
			/* Teate all vendor/fragment IE as one element
			 * to avoid IOT problem.
			 */
			while (end - pos >= 2 &&
			       (IE_ID(pos) == ELEM_ID_VENDOR ||
				IE_ID(pos) == ELEM_ID_FRAGMENT) &&
			       IE_SIZE(pos) <= end - pos) {
				info[num].size += IE_SIZE(pos);
				u2LastLen = IE_SIZE(pos);
				pucBuf = (uint8_t *)pos;
				pos += IE_SIZE(pos);
			}
			u2Offset += info[num].size - u2LastLen;
		} else {
			while (end - pos >= 2 &&
			       IE_ID(pos) == ELEM_ID_FRAGMENT &&
			       IE_SIZE(pos) <= end - pos) {
				info[num].size += IE_SIZE(pos);
				u2LastLen = IE_SIZE(pos);
				pucBuf = (uint8_t *)pos;
				pos += IE_SIZE(pos);
			}
			u2Offset += info[num].size - u2LastLen;
		}

		num++;
	}

	/* sort ie info table */
	sortQuickSort(info, apu2OrderTable, 0, num - 1);

	/* compose IE by sorted ie table */
	pucBuf = pucDst = (uint8_t *) prMsduInfoInOrder->prPacket;
	for (i = 0; i < num; i++) {
		DBGLOG(TX, LOUD, "#%d: IE(%d, %d) size=%d\n",
			apu2OrderTable[info[i].eid + info[i].extid],
			info[i].eid, info[i].extid, info[i].size);

		if (pucBuf - pucDst < u2IEsBufLen) {
			kalMemCopy(pucBuf, info[i].ie, info[i].size);
			pucBuf += info[i].size;
		} else {
			DBGLOG(TX, ERROR, "sorted IE len is not matched\n");
		}
	}

	if ((pucBuf - pucDst) != u2IEsBufLen) {
		DBGLOG(TX, ERROR, "Wrong length=%ld, expect=%d\n",
				pucBuf - pucDst, u2IEsBufLen);
		goto done;
	}

#if DBG
	DBGLOG(TX, DEBUG,
	       "Sorted %s IE, length = %d\n", pucIeDesc, u2IEsBufLen);
	dumpMemory8(pucDst, u2IEsBufLen);
#endif

	/* copy ordered frame back to prMsduInfo */
	kalMemCopy((uint8_t *) prMsduInfo->prPacket + offset,
		pucDst, u2IEsBufLen);

done:
	cnmMgtPktFree(prAdapter, prMsduInfoInOrder);
	kalMemFree(info, VIR_MEM_TYPE,
		MAX_IE_NUM * sizeof(struct IE_ORDER_TABLE_INFO));
}

/*
 * Cannot use range designated assign ... in array
 * for porting to other system
 * mgmt is common part
 * not all compiler support ... assign array initialize. Only c99 extension
 */
static void _initSortTable(uint16_t *ieTable, uint32_t size)
{
	uint32_t i = 0;

	for (i = 0; i < size; i++) {
		if (ieTable[i] == 0)
			ieTable[i] = SORT_ORDER_UNKNOWN;
	}
}

static void initSortTable(void)
{
	/*
	 * All variables with static storage duration are guaranteed
	 * to be initailized to their respective zero value and assigned once
	 */
	static uint8_t ucSortTblInited;

#define INIT_SORT(x) _initSortTable(x##_IE_ORDER, ARRAY_SIZE(x##_IE_ORDER))

	if (ucSortTblInited == 1)
		return;

	INIT_SORT(AUTH);
	INIT_SORT(ASSOC_REQ);
	INIT_SORT(ASSOC_RSP);
	INIT_SORT(REASSOC_REQ);
	INIT_SORT(REASSOC_RSP);
	INIT_SORT(PROBE_RSP);
	INIT_SORT(BEACON);

	ucSortTblInited = 1;
}

void sortMgmtFrameIE(struct ADAPTER *prAdapter,
		    struct MSDU_INFO *prMsduInfo)
{
#define SORT(x) \
	case MAC_FRAME_##x: sortIE(prAdapter, prMsduInfo, x##_IE_ORDER, #x); \
	break

	struct WLAN_MAC_MGMT_HEADER *prMgmtFrame;
	uint16_t u2TxFrameCtrl;

	initSortTable();

	prMgmtFrame = (struct WLAN_MAC_MGMT_HEADER *)(prMsduInfo->prPacket);
	u2TxFrameCtrl = prMgmtFrame->u2FrameCtrl;
	u2TxFrameCtrl &= MASK_FRAME_TYPE;
	switch (u2TxFrameCtrl) {
	SORT(AUTH);
	SORT(ASSOC_REQ);
	SORT(ASSOC_RSP);
	SORT(REASSOC_REQ);
	SORT(REASSOC_RSP);
	SORT(PROBE_RSP);
	SORT(BEACON);
	}
}

int sortMsduPayloadOffset(struct ADAPTER *prAdapter,
		    struct MSDU_INFO *prMsduInfo)
{
	if (!prMsduInfo)
		return -1;
	return sortGetPayloadOffset(prAdapter, prMsduInfo->prPacket);
}

int sortGetPayloadOffset(struct ADAPTER *prAdapter,
		    uint8_t *pucFrame)
{
	struct WLAN_MAC_MGMT_HEADER *prMgmtFrame;
	uint16_t u2TxFrameCtrl;

	if (!pucFrame)
		return -1;

	prMgmtFrame = (struct WLAN_MAC_MGMT_HEADER *)(pucFrame);
	u2TxFrameCtrl = prMgmtFrame->u2FrameCtrl & MASK_FRAME_TYPE;
	switch (u2TxFrameCtrl) {
	case MAC_FRAME_AUTH:
		return OFFSET_OF(struct WLAN_AUTH_FRAME, aucInfoElem[0]);
	case MAC_FRAME_ASSOC_REQ:
		return OFFSET_OF(struct WLAN_ASSOC_REQ_FRAME, aucInfoElem[0]);
	case MAC_FRAME_ASSOC_RSP:
		return OFFSET_OF(struct WLAN_ASSOC_RSP_FRAME, aucInfoElem[0]);
	case MAC_FRAME_REASSOC_REQ:
		return OFFSET_OF(struct WLAN_REASSOC_REQ_FRAME, aucInfoElem[0]);
	case MAC_FRAME_REASSOC_RSP:
		return OFFSET_OF(struct WLAN_ASSOC_RSP_FRAME, aucInfoElem[0]);
	case MAC_FRAME_PROBE_RSP:
		return OFFSET_OF(struct WLAN_BEACON_FRAME, aucInfoElem[0]);
	case MAC_FRAME_BEACON:
		return OFFSET_OF(struct WLAN_BEACON_FRAME, aucInfoElem[0]);
	}

	return -1;
}

uint8_t *sortBuildFragmentIE(uint8_t *dest_ie, uint8_t frag_eid,
	uint8_t *src, uint16_t src_size)
{
	uint8_t *pos = IE_TAIL(dest_ie), *cp = src;
	uint16_t left = src_size, len;

	DBGLOG(TX, LOUD, "dest=%p pos=%p src=%p src_size=%d\n",
		dest_ie, pos, src, src_size);

	while (left > 0) {
		DBGLOG(TX, LOUD, "left=%d\n", left);

		if (IE_LEN(dest_ie) + left > 255) {
			len = 255 - IE_LEN(dest_ie);

			kalMemCopy(pos, cp, len);
			cp += len;
			pos += len;
			left -= len;
			IE_LEN(dest_ie) += len;

			dest_ie = pos;
			dest_ie[0] = frag_eid;
			dest_ie[1] = 0;

			pos += 2;
		} else {
			len = left;

			kalMemCopy(pos, cp, len);
			cp += len;
			pos += len;
			left -= len;
			IE_LEN(dest_ie) += len;
		}
	}

	return dest_ie;
}

