/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "mac.h"
 *  \brief  Brief description.
 *
 *  Detail description.
 */


#ifndef _MAC_H
#define _MAC_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
/* 3 --------------- Constants for Ethernet/802.11 MAC --------------- */
/* MAC Address */
#define MAC_ADDR_LEN                            6
#define MAC_OUI_LEN                             (3)

#define MAC_ADDR_LOCAL_ADMIN                    BIT(1)

#define ETH_P_IPV4                              0x0800
#define ETH_P_ARP                               0x0806
#define ETH_P_IPX                               0x8137	/* Novell IPX */
/* AppleTalk Address Resolution Protocol (AARP) */
#define ETH_P_AARP                              0x80F3
#define ETH_P_IPV6                              0x86DD
#define ETH_P_VLAN                              0x8100

#define IP_PRO_ICMP				0x01
#define IP_PRO_UDP				0x11
#define IP_PRO_TCP				0x06

#define UDP_PORT_DHCPS				0x43
#define UDP_PORT_DHCPC				0x44
#define UDP_PORT_DNS				0x35
#define UDP_PORT_MDNS				5353
#define UDP_PORT_NTP				123

#define ETH_P_1X                                0x888E
#define ETH_P_PRE_1X                            0x88C7
#if CFG_SUPPORT_WAPI
#define ETH_WPI_1X                              0x88B4
#endif
#define ETH_EAPOL_KEY                           3

#define ETH_PRO_TDLS                            0x890d

/* 802.3 Frame If Ether Type/Len <= 1500 */
#define ETH_802_3_MAX_LEN                       1500

/* IP Header definition */
#define IP_VERSION_MASK                         BITS(4, 7)
#define IP_VERSION_OFFSET                       4
#define IP_VERSION_4                            4
#define IP_VERSION_6                            6

/* RFC 4443 */
#define ICMP_TYPE_OFFSET			0
#define ICMP_CODE_OFFSET			1
#define ICMP_CHECKSUM_OFFSET			2
#define ICMP_IDENTIFIER_OFFSET			4
#define ICMP_SEQ_NUM_OFFSET			6

/* RFC 792 */
#define ICMPV4_TYPE_ECHO_REPLY			0
#define ICMPV4_TYPE_UNREACHABLE			3
#define ICMPV4_TYPE_SOURCE_QUENCH		4
#define ICMPV4_TYPE_REDIRECT			5
#define ICMPV4_TYPE_ECHO			8
#define ICMPV4_TYPE_TIME_EXCEEDED		11
#define ICMPV4_TYPE_PARAMETER_PROBLEM		12
#define ICMPV4_TYPE_TIMESTAMP			13
#define ICMPV4_TYPE_TIMESTAMP_REPLY		14
#define ICMPV4_TYPE_INFORMATION_REQUEST		15
#define ICMPV4_TYPE_INFORMATION_REPLY		16

/* RFC 4861 */
#define ICMPV6_NS_NA_RESERVED_OFFSET		4
#define ICMPV6_NS_NA_TARGET_OFFSET		8
#define ICMPV6_NS_NA_OPTION_OFFSET		24

/* IPv4 Header definition */
#define IPV4_HDR_VERSION_OFFSET                 0
#define IPV4_HDR_TOS_OFFSET                     1
#define IPV4_HDR_TOS_PREC_MASK                  BITS(5, 7)
#define IPV4_HDR_TOS_PREC_OFFSET                5
#define IPV4_HDR_IP_IDENTIFICATION_OFFSET       4
#define IPV4_HDR_IP_FRAG_OFFSET                 6
#define IPV4_HDR_IP_FRAG_PART1_MASK             BITS(0, 4)
#define IPV4_HDR_IP_FLAGS_MASK                  BITS(5, 7)
#define IPV4_HDR_IP_FLAGS_MF_MASK               BIT(5)
#define IPV4_HDR_IP_PROTOCOL_OFFSET             9
#define IPV4_HDR_IP_CSUM_OFFSET                 10
#define IPV4_HDR_IP_SRC_ADDR_OFFSET             12
#define IPV4_HDR_IP_DST_ADDR_OFFSET             16

#define IPV4_ADDR_LEN                           4

#define IPV6_HDR_PAYLOAD_LEN_OFFSET             4
#define IPV6_HDR_IP_PROTOCOL_OFFSET             6
#define IPV6_HDR_IP_SRC_ADDR_OFFSET             8
#define IPV6_HDR_IP_DST_ADDR_OFFSET             24
#define IPV6_HDR_IP_DST_ADDR_MAC_HIGH_OFFSET    32
#define IPV6_HDR_IP_DST_ADDR_MAC_LOW_OFFSET     37
#define IPV6_PROTOCOL_HOP_BY_HOP                0
#define IPV6_PROTOCOL_ICMPV6                    0x3A


#define IPV6_UDP_PORT_DHCPC			546
#define IPV6_UDP_PORT_DHCPS			547

#define IPV6_HDR_TC_PREC_OFFSET                 1
#define IPV6_HDR_TC_PREC_MASK                   BITS(1, 3)
#define IPV6_HDR_PROTOCOL_OFFSET                6
#define IPV6_HDR_LEN                            40

#define IPV6_ADDR_LEN                           16

#define MAC_ADDR_STR_BUF_SIZE	(MAC_ADDR_LEN * 3)	/* 1 octet -> aa: */
#define IPV4_ADDR_STR_BUF_SIZE	(IPV4_ADDR_LEN * 4)	/* 1 octet -> 255. */
#define IPV6_ADDR_STR_BUF_SIZE	(IPV6_ADDR_LEN / 2 * 5) /* 2 octets -> aabb: */

#define ICMPV6_TYPE_OFFSET                      0
#define ICMPV6_FLAG_OFFSET                      4
#define ICMPV6_TARGET_ADDR_OFFSET		8
#define ICMPV6_TARGET_LL_ADDR_TYPE_OFFSET	24
#define ICMPV6_TARGET_LL_ADDR_LEN_OFFSET	25
#define ICMPV6_TARGET_LL_ADDR_TA_OFFSET		26

#define ICMPV6_FLAG_ROUTER_BIT                  BIT(7)
#define ICMPV6_FLAG_SOLICITED_BIT               BIT(6)
#define ICMPV6_FLAG_OVERWRITE_BIT               BIT(5)

#define ICMPV6_TYPE_RESERVED                    0
#define ICMPV6_TYPE_DESTINATIN_UNREACHABLE      1
#define ICMPV6_TYPE_PACKET_TOO_LONG             2
#define ICMPV6_TYPE_TIME_EXCEEDED               3
#define ICMPV6_TYPE_PARAMETER_PROBLEM           4

#define ICMPV6_TYPE_ECHO_REQUEST                0x80 /* 128 */
#define ICMPV6_TYPE_ECHO_REPLY                  0x81 /* 129 */
#define ICMPV6_TYPE_MULTICAST_LISTENER_QUERY    0x82 /* 130 */
#define ICMPV6_TYPE_MULTICAST_LISTENER_REPORT   0x83 /* 131 */
#define ICMPV6_TYPE_MULTICAST_LISTENER_DONE     0x84 /* 132 */
#define ICMPV6_TYPE_ROUTER_SOLICITATION		0x85 /* 133 */
#define ICMPV6_TYPE_ROUTER_ADVERTISEMENT	0x86 /* 134 */
#define ICMPV6_TYPE_NEIGHBOR_SOLICITATION       0x87 /* 135 */
#define ICMPV6_TYPE_NEIGHBOR_ADVERTISEMENT      0x88 /* 136 */

#define ICMPV6_OPTION_SOURCE_LINK_ADDR		1
#define ICMPV6_OPTION_TARGET_LINK_ADDR		2

#define TCP_HDR_FLAG_OFFSET                     13
#define TCP_HDR_FLAG_FIN_BIT                    BIT(0)
#define TCP_HDR_FLAG_SYN_BIT                    BIT(1)
#define TCP_HDR_FLAG_RST_BIT                    BIT(2)
#define TCP_HDR_FLAG_PSH_BIT                    BIT(3)
#define TCP_HDR_FLAG_ACK_BIT                    BIT(4)
#define TCP_HDR_FLAG_URG_BIT                    BIT(5)
#define TCP_HDR_FLAG_SYN_ACK \
		(TCP_HDR_FLAG_SYN_BIT | TCP_HDR_FLAG_ACK_BIT)
#define TCP_HDR_FLAGS				BITS(0, 5)
#define TCP_HDR_SRC_PORT_OFFSET                 0
#define TCP_HDR_DST_PORT_OFFSET                 2
#define TCP_HDR_SEQ                             4
#define TCP_HDR_ACK                             8
#define TCP_HDR_FLAG_OFFSET                     13
#define TCP_HDR_TCP_CSUM_OFFSET                 16

#define UDP_HDR_LEN                             8

#define UDP_HDR_SRC_PORT_OFFSET                 0
#define UDP_HDR_DST_PORT_OFFSET                 2
#define UDP_HDR_UDP_CSUM_OFFSET                 6

#define IP_PORT_DHCP_SERVER                     67
#define IP_PORT_DHCP_CLIENT                     68

#define DHCP_MAGIC_NUMBER                       0x63825363

#define DHCP_DISCOVER				1
#define DHCP_OFFER				2
#define DHCP_REQUEST				3
#define DHCP_DECLINE				4
#define DHCP_ACK				5
#define DHCP_NAK				6
#define DHCP_RELEASE				7
#define DHCP_INFORM				8

#define ARP_PKT_LEN                             28
#define ARP_OPERATION_OFFSET                    6
#define ARP_SENDER_MAC_OFFSET                   8
#define ARP_SENDER_IP_OFFSET                    14
#define ARP_TARGET_MAC_OFFSET                   18
#define ARP_TARGET_IP_OFFSET                    24
#define ARP_OPERATION_REQUEST                   0x0001
#define ARP_OPERATION_RESPONSE                  0x0002

#define TDLS_ACTION_CODE_OFFSET                 2

/* LLC(3) + SNAP(3) + EtherType(2) */
#define LLC_LEN                                 8

#define NULL_MAC_ADDR     {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
#define BC_MAC_ADDR       {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}

#if 1

#define ETH_HLEN                                14
#define ETH_TYPE_LEN_OFFSET                     12

#define IPVERSION                               4
#define IP_HEADER_LEN                           20
#define IP_PROTO_HLEN                           9

#define IPVH_VERSION_OFFSET                     4	/* For Little-Endian */
#define IPVH_VERSION_MASK                       0xF0
#define IPV4_HEADER_LENGTH_MASK                 0x0F
#define IPTOS_PREC_OFFSET                       5
#define IPTOS_PREC_MASK                         0xE0

#define SOURCE_PORT_LEN                         2

#define LOOK_AHEAD_LEN \
	(ETH_HLEN + IP_HEADER_LEN + SOURCE_PORT_LEN)

#endif

/* Ethernet Frame Field Size, in byte */
#define ETHER_HEADER_LEN                        14
#define ETHER_TYPE_LEN                          2
#define ETHER_MIN_PKT_SZ                        60
#define ETHER_MAX_PKT_SZ                        1514

#define ETHER_TYPE_LEN_OFFSET                   12
#define ETHER_TYPE_MIN                          0x0600

/* 802.1Q (VLAN) */
#define ETH_802_1Q_HEADER_LEN                   4

/* 802.2 LLC/SNAP */
#define ETH_LLC_OFFSET                          (ETHER_HEADER_LEN)
#define ETH_LLC_LEN                             3
#define ETH_LLC_DSAP_SNAP                       0xAA
#define ETH_LLC_SSAP_SNAP                       0xAA
#define ETH_LLC_CONTROL_UNNUMBERED_INFORMATION  0x03
#define ETH_LLC                                 \
	{ETH_LLC_DSAP_SNAP, ETH_LLC_SSAP_SNAP, \
	ETH_LLC_CONTROL_UNNUMBERED_INFORMATION}

/* Bluetooth SNAP */
#define ETH_SNAP_OFFSET                         (ETHER_HEADER_LEN + ETH_LLC_LEN)
#define ETH_SNAP_LEN                            5
#define ETH_SNAP_OUI_LEN                        3
#define ETH_SNAP_BT_SIG_OUI_0                   0x00
#define ETH_SNAP_BT_SIG_OUI_1                   0x19
#define ETH_SNAP_BT_SIG_OUI_2                   0x58
#define ETH_SNAP_BT_SIG_OUI \
	{ETH_SNAP_BT_SIG_OUI_0, ETH_SNAP_BT_SIG_OUI_1, ETH_SNAP_BT_SIG_OUI_2}

#define BOW_PROTOCOL_ID_SECURITY_FRAME          0x0003

/* IEEE 802.11 WLAN Frame Field Size, in byte */
/* Address 4 excluded */
#define WLAN_MAC_HEADER_LEN                     24
/* Address 4 included */
#define WLAN_MAC_HEADER_A4_LEN                  30
/* QoS Control included */
#define WLAN_MAC_HEADER_QOS_LEN                 26
/* QoS Control and HTC included */
#define WLAN_MAC_HEADER_QOS_HTC_LEN             30
/* Address 4 and QoS Control included */
#define WLAN_MAC_HEADER_A4_QOS_LEN              32
/* Address 4, QoS Control and HTC included */
#define WLAN_MAC_HEADER_A4_QOS_HTC_LEN          36
/* Address 4 excluded */
#define WLAN_MAC_MGMT_HEADER_LEN                24
/* HTC included */
#define WLAN_MAC_MGMT_HEADER_HTC_LEN            28

#define QOS_CTRL_LEN                            2
#define HT_CTRL_LEN                             4

#define WLAN_MAC_CTS_ACK_LEN \
	(WLAN_MAC_CTS_ACK_FRAME_HEADER_LEN + FCS_LEN)

/* 6.2.1.1.2 Semantics of the service primitive */
#define MSDU_MAX_LENGTH                         2304

/* 7.1.3.3.3 Broadcast BSSID */
#define BC_BSSID                                BC_MAC_ADDR

/* 7.1.3.7 FCS field */
#define FCS_LEN                                 4

/* 7.3.1.6 Listen Interval field */
/* In unit of AP's DTIM interval, */
#define DEFAULT_LISTEN_INTERVAL_BY_DTIM_PERIOD  2
#define DEFAULT_LISTEN_INTERVAL                 10
#define INVALID_LISTEN_INTERVAL                 0

/* 7.3.2.1 Broadcast(Wildcard) SSID */
#define BC_SSID                                 ""
#define BC_SSID_LEN                             0

/* 7.3.2.2 Data Rate Value */
#define RATE_1M                             2	/* 1M in unit of 500kb/s */
#define RATE_2M                             4	/* 2M */
#define RATE_5_5M                           11	/* 5.5M */
#define RATE_11M                            22	/* 11M */
#define RATE_22M                            44	/* 22M */
#define RATE_33M                            66	/* 33M */
#define RATE_6M                             12	/* 6M */
#define RATE_9M                             18	/* 9M */
#define RATE_12M                            24	/* 12M */
#define RATE_18M                            36	/* 18M */
#define RATE_24M                            48	/* 24M */
#define RATE_36M                            72	/* 36M */
#define RATE_48M                            96	/* 48M */
#define RATE_54M                            108	/* 54M */
/* 7.3.2.14 BSS membership selector */

/* BSS Selector - Clause 22. HT PHY */
#define RATE_VHT_PHY                            126
/* BSS Selector - Clause 20. HT PHY */
#define RATE_HT_PHY         127
/* BSS Selector - Hash to Element only */
#define RATE_H2E_ONLY                           123
#define RATE_H2E_ONLY_VAL                       (0x80 | 123)

/* mask bits for the rate */
#define RATE_MASK                               BITS(0, 6)
/* mask bit for the rate belonging to the BSSBasicRateSet */
#define RATE_BASIC_BIT                          BIT(7)

/* 8.3.2.2 TKIP MPDU formats */
#define TKIP_MIC_LEN                            8

/* 9.2.10 DIFS */
#define DIFS                                    2	/* 2 x aSlotTime */

/* 11.3 STA Authentication and Association */
/* Accept Class 1 frames */
#define STA_STATE_1                             0
/* Accept Class 1 & 2 frames */
#define STA_STATE_2                             1
/* Accept Class 1,2 & 3 frames */
#define STA_STATE_3                             2

/* 15.4.8.5 802.11k RCPI-dBm mapping*/
#define NDBM_LOW_BOUND_FOR_RCPI                 110
#define RCPI_LOW_BOUND                          0
#define RCPI_HIGH_BOUND                         220
#define RCPI_MEASUREMENT_NOT_AVAILABLE          255

/* PHY characteristics */
/* 17.4.4/18.3.3/19.8.4 Slot Time (aSlotTime) */
/* Long Slot Time */
#define SLOT_TIME_LONG                          20
/* Short Slot Time */
#define SLOT_TIME_SHORT                         9

/* 802.11b aSlotTime */
#define SLOT_TIME_HR_DSSS                       SLOT_TIME_LONG
/* 802.11a aSlotTime(20M Spacing) */
#define SLOT_TIME_OFDM                          SLOT_TIME_SHORT
/* 802.11a aSlotTime(10M Spacing) */
#define SLOT_TIME_OFDM_10M_SPACING              13
/* 802.11g aSlotTime(Long) */
#define SLOT_TIME_ERP_LONG                      SLOT_TIME_LONG
/* 802.11g aSlotTime(Short) */
#define SLOT_TIME_ERP_SHORT                     SLOT_TIME_SHORT

/* 17.4.4/18.3.3/19.8.4 Contention Window (aCWmin & aCWmax) */
/* 802.11a aCWmin */
#define CWMIN_OFDM                              15
/* 802.11a aCWmax */
#define CWMAX_OFDM                              1023

/* 802.11b aCWmin */
#define CWMIN_HR_DSSS                           31
/* 802.11b aCWmax */
#define CWMAX_HR_DSSS                           1023

/* 802.11g aCWmin(0) - for only have 1/2/5/11Mbps Rates */
#define CWMIN_ERP_0                             31
/* 802.11g aCWmin(1) */
#define CWMIN_ERP_1                             15
/* 802.11g aCWmax */
#define CWMAX_ERP                               1023

/* Short Inter-Frame Space (aSIFSTime) */
/* 15.3.3 802.11b aSIFSTime */
#define SIFS_TIME_HR_DSSS                       10
/* 17.4.4 802.11a aSIFSTime */
#define SIFS_TIME_OFDM                          16
/* 19.8.4 802.11g aSIFSTime */
#define SIFS_TIME_ERP                           10

/* 15.4.6.2 Number of operating channels */
#define CH_1                                    0x1
#define CH_2                                    0x2
#define CH_3                                    0x3
#define CH_4                                    0x4
#define CH_5                                    0x5
#define CH_6                                    0x6
#define CH_7                                    0x7
#define CH_8                                    0x8
#define CH_9                                    0x9
#define CH_10                                   0xa
#define CH_11                                   0xb
#define CH_12                                   0xc
#define CH_13                                   0xd
#define CH_14                                   0xe

/* 3 --------------- IEEE 802.11 PICS --------------- */
/* Annex D - dot11OperationEntry 2 */
#define DOT11_RTS_THRESHOLD_MIN                 0
#define DOT11_RTS_THRESHOLD_MAX                 2347	/* from Windows DDK */
/* #define DOT11_RTS_THRESHOLD_MAX                 3000 // from Annex D */

#define DOT11_RTS_THRESHOLD_DEFAULT             \
	    DOT11_RTS_THRESHOLD_MAX

/* Annex D - dot11OperationEntry 5 */
#define DOT11_FRAGMENTATION_THRESHOLD_MIN       256
#define DOT11_FRAGMENTATION_THRESHOLD_MAX       2346	/* from Windows DDK */
/* #define DOT11_FRAGMENTATION_THRESHOLD_MAX       3000 // from Annex D */

#define DOT11_FRAGMENTATION_THRESHOLD_DEFAULT   \
	    DOT11_FRAGMENTATION_THRESHOLD_MAX

/* Annex D - dot11OperationEntry 6 */
#define DOT11_TRANSMIT_MSDU_LIFETIME_TU_MIN     1
#define DOT11_TRANSMIT_MSDU_LIFETIME_TU_MAX     0xFFFFffff

/* 802.11 define 512 */
/* MT5921 only aceept N <= 4095 */
#define DOT11_TRANSMIT_MSDU_LIFETIME_TU_DEFAULT 4095

/* Annex D - dot11OperationEntry 7 */
#define DOT11_RECEIVE_LIFETIME_TU_MIN           1
#define DOT11_RECEIVE_LIFETIME_TU_MAX           0xFFFFffff
#define DOT11_RECEIVE_LIFETIME_TU_DEFAULT       4096	/* 802.11 define 512 */

/* Annex D - dot11StationConfigEntry 12 */
#define DOT11_BEACON_PERIOD_MIN                 1	/* TU. */
#define DOT11_BEACON_PERIOD_MAX                 0xffff	/* TU. */
#define DOT11_BEACON_PERIOD_DEFAULT             100	/* TU. */

/* Annex D - dot11StationConfigEntry 13 */
#define DOT11_DTIM_PERIOD_MIN                   1	/* TU. */
#define DOT11_DTIM_PERIOD_MAX                   255	/* TU. */
#define DOT11_DTIM_PERIOD_DEFAULT               1	/* TU. */

/* Annex D - dot11RegDomainsSupportValue */
#define REGULATION_DOMAIN_FCC                   0x10	/* FCC (US) */
#define REGULATION_DOMAIN_IC                    0x20	/* IC or DOC (Canada) */
#define REGULATION_DOMAIN_ETSI                  0x30	/* ETSI (Europe) */
#define REGULATION_DOMAIN_SPAIN                 0x31	/* Spain */
#define REGULATION_DOMAIN_FRANCE                0x32	/* France */
#define REGULATION_DOMAIN_JAPAN                 0x40	/* MKK (Japan) */
#define REGULATION_DOMAIN_CHINA                 0x50	/* China */
#define REGULATION_DOMAIN_OTHER                 0x00	/* Other */

/* 3 --------------- IEEE 802.11 MAC header fields --------------- */
/* 7.1.3.1 Masks for the subfields in the Frame Control field */
#define MASK_FC_PROTOCOL_VER                    BITS(0, 1)
#define MASK_FC_TYPE                            BITS(2, 3)
#define MASK_FC_SUBTYPE                         BITS(4, 7)
#define MASK_FC_SUBTYPE_QOS_DATA                BIT(7)
#define MASK_FC_TO_DS                           BIT(8)
#define MASK_FC_FROM_DS                         BIT(9)
#define MASK_FC_MORE_FRAG                       BIT(10)
#define MASK_FC_RETRY                           BIT(11)
#define MASK_FC_PWR_MGT                         BIT(12)
#define MASK_FC_MORE_DATA                       BIT(13)
#define MASK_FC_PROTECTED_FRAME                 BIT(14)
#define MASK_FC_ORDER                           BIT(15)

#define MASK_FRAME_TYPE                         (MASK_FC_TYPE | MASK_FC_SUBTYPE)
#define MASK_TO_DS_FROM_DS \
	(MASK_FC_TO_DS | MASK_FC_FROM_DS)

#define MAX_NUM_OF_FC_SUBTYPES                  16
#define OFFSET_OF_FC_SUBTYPE                    4

/* 7.1.3.1.2 MAC frame types and subtypes */
#define MAC_FRAME_TYPE_MGT                      0x0000
#define MAC_FRAME_TYPE_CTRL                     BIT(2)
#define MAC_FRAME_TYPE_DATA                     BIT(3)
#define MAC_FRAME_TYPE_QOS_DATA \
	(MAC_FRAME_TYPE_DATA | MASK_FC_SUBTYPE_QOS_DATA)

#define MAC_FRAME_ASSOC_REQ                     (MAC_FRAME_TYPE_MGT | 0x0000)
#define MAC_FRAME_ASSOC_RSP                     (MAC_FRAME_TYPE_MGT | 0x0010)
#define MAC_FRAME_REASSOC_REQ                   (MAC_FRAME_TYPE_MGT | 0x0020)
#define MAC_FRAME_REASSOC_RSP                   (MAC_FRAME_TYPE_MGT | 0x0030)
#define MAC_FRAME_PROBE_REQ                     (MAC_FRAME_TYPE_MGT | 0x0040)
#define MAC_FRAME_PROBE_RSP                     (MAC_FRAME_TYPE_MGT | 0x0050)
#define MAC_FRAME_BEACON                        (MAC_FRAME_TYPE_MGT | 0x0080)
#define MAC_FRAME_ATIM                          (MAC_FRAME_TYPE_MGT | 0x0090)
#define MAC_FRAME_DISASSOC                      (MAC_FRAME_TYPE_MGT | 0x00A0)
#define MAC_FRAME_AUTH                          (MAC_FRAME_TYPE_MGT | 0x00B0)
#define MAC_FRAME_DEAUTH                        (MAC_FRAME_TYPE_MGT | 0x00C0)
#define MAC_FRAME_ACTION                        (MAC_FRAME_TYPE_MGT | 0x00D0)
#define MAC_FRAME_ACTION_NO_ACK                 (MAC_FRAME_TYPE_MGT | 0x00E0)

#define MASK_MAC_FRAME_ASSOC_REQ                BIT(MAC_FRAME_ASSOC_REQ >> 4)
#define MASK_MAC_FRAME_ASSOC_RSP                BIT(MAC_FRAME_ASSOC_RSP >> 4)
#define MASK_MAC_FRAME_REASSOC_REQ              BIT(MAC_FRAME_REASSOC_REQ >> 4)
#define MASK_MAC_FRAME_REASSOC_RSP              BIT(MAC_FRAME_REASSOC_RSP >> 4)
#define MASK_MAC_FRAME_PROBE_REQ                BIT(MAC_FRAME_PROBE_REQ >> 4)
#define MASK_MAC_FRAME_PROBE_RSP                BIT(MAC_FRAME_PROBE_RSP >> 4)
#define MASK_MAC_FRAME_BEACON                   BIT(MAC_FRAME_BEACON >> 4)
#define MASK_MAC_FRAME_ATIM                     BIT(MAC_FRAME_ATIM >> 4)
#define MASK_MAC_FRAME_DISASSOC                 BIT(MAC_FRAME_DISASSOC >> 4)
#define MASK_MAC_FRAME_AUTH                     BIT(MAC_FRAME_AUTH >> 4)
#define MASK_MAC_FRAME_DEAUTH                   BIT(MAC_FRAME_DEAUTH >> 4)
#define MASK_MAC_FRAME_ACTION                   BIT(MAC_FRAME_ACTION >> 4)
#define MASK_MAC_FRAME_ACTION_NO_ACK \
	BIT(MAC_FRAME_ACTION_NO_ACK >> 4)

#define MAC_FRAME_HE_TRIGGER                    (MAC_FRAME_TYPE_CTRL | 0x0020)
#define MAC_FRAME_CONTRL_WRAPPER                (MAC_FRAME_TYPE_CTRL | 0x0070)
#define MAC_FRAME_BLOCK_ACK_REQ                 (MAC_FRAME_TYPE_CTRL | 0x0080)
#define MAC_FRAME_BLOCK_ACK                     (MAC_FRAME_TYPE_CTRL | 0x0090)
#define MAC_FRAME_PS_POLL                       (MAC_FRAME_TYPE_CTRL | 0x00A0)
#define MAC_FRAME_RTS                           (MAC_FRAME_TYPE_CTRL | 0x00B0)
#define MAC_FRAME_CTS                           (MAC_FRAME_TYPE_CTRL | 0x00C0)
#define MAC_FRAME_ACK                           (MAC_FRAME_TYPE_CTRL | 0x00D0)
#define MAC_FRAME_CF_END                        (MAC_FRAME_TYPE_CTRL | 0x00E0)
#define MAC_FRAME_CF_END_CF_ACK                 (MAC_FRAME_TYPE_CTRL | 0x00F0)

#define MAC_FRAME_DATA                          (MAC_FRAME_TYPE_DATA | 0x0000)
#define MAC_FRAME_DATA_CF_ACK                   (MAC_FRAME_TYPE_DATA | 0x0010)
#define MAC_FRAME_DATA_CF_POLL                  (MAC_FRAME_TYPE_DATA | 0x0020)
#define MAC_FRAME_DATA_CF_ACK_CF_POLL           (MAC_FRAME_TYPE_DATA | 0x0030)
#define MAC_FRAME_NULL                          (MAC_FRAME_TYPE_DATA | 0x0040)
#define MAC_FRAME_CF_ACK                        (MAC_FRAME_TYPE_DATA | 0x0050)
#define MAC_FRAME_CF_POLL                       (MAC_FRAME_TYPE_DATA | 0x0060)
#define MAC_FRAME_CF_ACK_CF_POLL                (MAC_FRAME_TYPE_DATA | 0x0070)
#define MAC_FRAME_QOS_DATA                      (MAC_FRAME_TYPE_DATA | 0x0080)
#define MAC_FRAME_QOS_DATA_CF_ACK               (MAC_FRAME_TYPE_DATA | 0x0090)
#define MAC_FRAME_QOS_DATA_CF_POLL              (MAC_FRAME_TYPE_DATA | 0x00A0)
#define MAC_FRAME_QOS_DATA_CF_ACK_CF_POLL       (MAC_FRAME_TYPE_DATA | 0x00B0)
#define MAC_FRAME_QOS_NULL                      (MAC_FRAME_TYPE_DATA | 0x00C0)
#define MAC_FRAME_QOS_CF_POLL                   (MAC_FRAME_TYPE_DATA | 0x00E0)
#define MAC_FRAME_QOS_CF_ACK_CF_POLL            (MAC_FRAME_TYPE_DATA | 0x00F0)

/* 7.1.3.2 Mask for the AID value in the Duration/ID field */
#define MASK_DI_DURATION                        BITS(0, 14)
#define MASK_DI_AID                             BITS(0, 13)
#define MASK_DI_AID_MSB                         BITS(14, 15)
#define MASK_DI_CFP_FIXED_VALUE                 BIT(15)

/* 7.1.3.4 Masks for the subfields in the Sequence Control field */
#define MASK_SC_SEQ_NUM                         BITS(4, 15)
#define MASK_SC_SEQ_NUM_OFFSET                  4
#define MASK_SC_FRAG_NUM                        BITS(0, 3)

/* According to 6.2.1.1.2
 * FRAG_NUM won't equal to 15
 */
#define INVALID_SEQ_CTRL_NUM                    0x000F

/* 7.1.3.5 QoS Control field */
#define TID_NUM                                 16
#define TID_MASK                                BITS(0, 3)
#define EOSP                                    BIT(4)
#define ACK_POLICY                              BITS(5, 6)
#define A_MSDU_PRESENT                          BIT(7)

#define MASK_QC_TID                  BITS(0, 3)
#define MASK_QC_EOSP                 BIT(4)
#define MASK_QC_EOSP_OFFSET          4
#define MASK_QC_ACK_POLICY           BITS(5, 6)
#define MASK_QC_ACK_POLICY_OFFSET    5
#define MASK_QC_A_MSDU_PRESENT       BIT(7)

/* 7.1.3.5a HT Control field */
#define HT_CTRL_LINK_ADAPTATION_CTRL            BITS(0, 15)
#define HT_CTRL_CALIBRATION_POSITION            BITS(16, 17)
#define HT_CTRL_CALIBRATION_SEQUENCE            BITS(18, 19)
#define HT_CTRL_CSI_STEERING                    BITS(22, 23)
#define HT_CTRL_NDP_ANNOUNCEMENT                BIT(24)
#define HT_CTRL_AC_CONSTRAINT                   BIT(30)
#define HT_CTRL_RDG_MORE_PPDU                   BIT(31)

#define LINK_ADAPTATION_CTRL_TRQ                BIT(1)
#define LINK_ADAPTATION_CTRL_MAI_MRQ            BIT(2)
#define LINK_ADAPTATION_CTRL_MAI_MSI            BITS(3, 5)
#define LINK_ADAPTATION_CTRL_MFSI               BITS(6, 8)
#define LINK_ADAPTATION_CTRL_MFB_ASELC_CMD      BITS(9, 11)
#define LINK_ADAPTATION_CTRL_MFB_ASELC_DATA     BITS(12, 15)

/* 7.1.3.5.3 Ack Policy subfield*/
#define ACK_POLICY_NORMAL_ACK_IMPLICIT_BA_REQ 0
#define ACK_POLICY_NO_ACK 1
#define ACK_POLICY_NO_EXPLICIT_ACK_PSMP_ACK 2
#define ACK_POLICY_BA 3

/* 7.1.3.7 FCS field */
#define FCS_LEN                                 4

/* 7.2.1.4 WLAN Control Frame - PS-POLL Frame */
#define PSPOLL_FRAME_LEN                        16	/* w/o FCS */

/* 7.2.7.1 BAR */
#define OFFSET_BAR_SSC_SN                       4

/* 8.3.2.2 TKIP MPDU formats */
#define TKIP_MIC_LEN                            8

#define BA_POLICY_IMMEDIATE                     BIT(1)

/* Block Ack Starting Sequence Control field */
#define BA_START_SEQ_CTL_FRAG_NUM               BITS(0, 3)
#define BA_START_SEQ_CTL_SSN                    BITS(4, 15)

/* BAR Control field */
#define BAR_CONTROL_NO_ACK_POLICY               BIT(0)
#define BAR_CONTROL_MULTI_TID                   BIT(1)
#define BAR_CONTROL_COMPRESSED_BA               BIT(2)
#define BAR_CONTROL_TID_INFO                    BITS(12, 15)
#define BAR_CONTROL_TID_INFO_OFFSET             12

/* TID Value */
#define BAR_INFO_TID_VALUE                      BITS(12, 15)

#define BAR_COMPRESSED_VARIANT_FRAME_LEN        (16 + 4)

/* 3 --------------- IEEE 802.11 frame body fields --------------- */
/* 3 Management frame body components (I): Fixed Fields. */
/* 7.3.1.1 Authentication Algorithm Number field */
#define AUTH_ALGORITHM_NUM_FIELD_LEN                2

#define AUTH_ALGORITHM_NUM_OPEN_SYSTEM          0 /* Open System */
#define AUTH_ALGORITHM_NUM_SHARED_KEY           1 /* Shared Key */
#define AUTH_ALGORITHM_NUM_FT			2 /* Fast BSS Transition */
#define AUTH_ALGORITHM_NUM_SAE                  3 /* WPA3 - SAE */
#define AUTH_ALGORITHM_NUM_FILS_SK		4 /* FILS - Shared key */
#define AUTH_ALGORITHM_NUM_FILS_SK_PFS		5 /* FILS - SK with PFS */
#define AUTH_ALGORITHM_NUM_FILS_PK		6 /* FILS - Public key */
#define AUTH_ALGORITHM_NUM                      0xFF

/* 7.3.1.2 Authentication Transaction Sequence Number field */
#define AUTH_TRANSACTION_SEQENCE_NUM_FIELD_LEN      2
#define AUTH_TRANSACTION_SEQ_1                      1
#define AUTH_TRANSACTION_SEQ_2                      2
#define AUTH_TRANSACTION_SEQ_3                      3
#define AUTH_TRANSACTION_SEQ_4                      4

/* 7.3.1.3 Beacon Interval field */
#define BEACON_INTERVAL_FIELD_LEN                   2

/* 7.3.1.4 Capability Information field */
#define CAP_INFO_FIELD_LEN                          2
#define CAP_INFO_ESS                                BIT(0)
#define CAP_INFO_IBSS                               BIT(1)
#define CAP_INFO_BSS_TYPE \
	(CAP_INFO_ESS | CAP_INFO_IBSS)
#define CAP_INFO_CF_POLLABLE                        BIT(2)
#define CAP_INFO_CF_POLL_REQ                        BIT(3)
#define CAP_INFO_CF \
	(CAP_INFO_CF_POLLABLE | CAP_INFO_CF_POLL_REQ)
#define CAP_INFO_PRIVACY                            BIT(4)
#define CAP_INFO_SHORT_PREAMBLE                     BIT(5)
#define CAP_INFO_PBCC                               BIT(6)
#define CAP_INFO_CH_AGILITY                         BIT(7)
#define CAP_INFO_SPEC_MGT                           BIT(8)
#define CAP_INFO_QOS                                BIT(9)
#define CAP_INFO_SHORT_SLOT_TIME                    BIT(10)
#define CAP_INFO_APSD                               BIT(11)
#define CAP_INFO_RADIO_MEASUREMENT                  BIT(12)
#define CAP_INFO_DSSS_OFDM                          BIT(13)
#define CAP_INFO_DELAYED_BLOCK_ACK                  BIT(14)
#define CAP_INFO_IMM_BLOCK_ACK                      BIT(15)
/* STA usage of CF-Pollable and CF-Poll Request subfields */
/* STA: not CF-Pollable */
#define CAP_CF_STA_NOT_POLLABLE                     0x0000
/* STA: CF-Pollable, not requesting on the CF-Polling list */
#define CAP_CF_STA_NOT_ON_LIST                      CAP_INFO_CF_POLL_REQ
/* STA: CF-Pollable, requesting on the CF-Polling list */
#define CAP_CF_STA_ON_LIST                          CAP_INFO_CF_POLLABLE
/* STA: CF-Pollable, requesting never to be polled */
#define CAP_CF_STA_NEVER_POLLED \
	(CAP_INFO_CF_POLLABLE | CAP_INFO_CF_POLL_REQ)

/* AP usage of CF-Pollable and CF-Poll Request subfields */
/* AP: No point coordinator (PC) */
#define CAP_CF_AP_NO_PC                             0x0000
/* AP: PC at AP for delivery only (no polling) */
#define CAP_CF_AP_DELIVERY_ONLY                     CAP_INFO_CF_POLL_REQ
/* AP: PC at AP for delivery and polling */
#define CAP_CF_AP_DELIVERY_POLLING                  CAP_INFO_CF_POLLABLE

/* 7.3.1.5 Current AP Address field */
#define CURR_AP_ADDR_FIELD_LEN                      MAC_ADDR_LEN

/* 7.3.1.6 Listen Interval field */
#define LISTEN_INTERVAL_FIELD_LEN                   2

/* 7.3.1.7 Reason Code field */
#define REASON_CODE_FIELD_LEN                       2

/* Reseved */
#define REASON_CODE_RESERVED                        0
/* Unspecified reason */
#define REASON_CODE_UNSPECIFIED                     1
/* Previous auth no longer valid */
#define REASON_CODE_PREV_AUTH_INVALID               2
/* Deauth because sending STA is leaving BSS */
#define REASON_CODE_DEAUTH_LEAVING_BSS              3
/* Disassoc due to inactivity */
#define REASON_CODE_DISASSOC_INACTIVITY             4
/* Disassoc because AP is unable to handle all assoc STAs */
#define REASON_CODE_DISASSOC_AP_OVERLOAD            5
/* Class 2 frame rx from nonauth STA */
#define REASON_CODE_CLASS_2_ERR                     6
/* Class 3 frame rx from nonassoc STA */
#define REASON_CODE_CLASS_3_ERR                     7
/* Disassoc because sending STA is leaving BSS */
#define REASON_CODE_DISASSOC_LEAVING_BSS            8
/* STA requesting (re)assoc is not auth with responding STA */
#define REASON_CODE_ASSOC_BEFORE_AUTH               9
/* Disassoc because the info in Power Capability is unacceptable */
#define REASON_CODE_DISASSOC_PWR_CAP_UNACCEPTABLE   10
/* Disassoc because the info in Supported Channels is unacceptable */
#define REASON_CODE_DISASSOC_SUP_CHS_UNACCEPTABLE   11
/* Invalid information element */
#define REASON_CODE_INVALID_INFO_ELEM               13
/* MIC failure */
#define REASON_CODE_MIC_FAILURE                     14
/* 4-way handshake timeout */
#define REASON_CODE_4_WAY_HANDSHAKE_TIMEOUT         15
/* Group key update timeout */
#define REASON_CODE_GROUP_KEY_UPDATE_TIMEOUT        16
/* Info element in 4-way handshake different from */
/* (Re-)associate request/Probe response/Beacon */
#define REASON_CODE_DIFFERENT_INFO_ELEM             17
/* Multicast Cipher is not valid */
#define REASON_CODE_MULTICAST_CIPHER_NOT_VALID      18
/* Unicast Cipher is not valid */
#define REASON_CODE_UNICAST_CIPHER_NOT_VALID        19
/* AKMP is not valid */
#define REASON_CODE_AKMP_NOT_VALID                  20
/* Unsupported RSNE version */
#define REASON_CODE_UNSUPPORTED_RSNE_VERSION        21
/* Invalid RSNE Capabilities */
#define REASON_CODE_INVALID_RSNE_CAPABILITIES       22
/* IEEE 802.1X Authentication failed */
#define REASON_CODE_IEEE_802_1X_AUTH_FAILED         23
/* Cipher suite rejected because of the security policy */
#define REASON_CODE_CIPHER_REJECT_SEC_POLICY        24
/* Disassoc for unspecified, QoS-related reason */
#define REASON_CODE_DISASSOC_UNSPECIFIED_QOS        32
/* Disassoc because QAP lacks sufficient bandwidth for this QSTA */
#define REASON_CODE_DISASSOC_LACK_OF_BANDWIDTH      33
/* Disassoc because of too many ACKs lost for AP transmissions */
/* and/or poor channel conditions */
#define REASON_CODE_DISASSOC_ACK_LOST_POOR_CHANNEL  34
/* Disassoc because QSTA is transmitting outside the limits of its TXOPs */
#define REASON_CODE_DISASSOC_TX_OUTSIDE_TXOP_LIMIT  35
/* QSTA is leaving the QBSS or resetting */
#define REASON_CODE_PEER_WHILE_LEAVING              36
/* Peer does not want to use this mechanism */
#define REASON_CODE_PEER_REFUSE_DLP                 37
/* Frames received but a setup is reqired */
#define REASON_CODE_PEER_SETUP_REQUIRED             38
/* Time out */
#define REASON_CODE_PEER_TIME_OUT                   39
/* Peer does not support the requested cipher suite */
#define REASON_CODE_PEER_CIPHER_UNSUPPORTED         45
/* for beacon timeout, defined by mediatek */
#define REASON_CODE_BEACON_TIMEOUT		    100
/* for power control, op mode change fail neeed to disconnect */
#define REASON_CODE_OP_MODE_CHANGE_FAIL		    101
/* For ARP no response detection */
#define REASON_CODE_ARP_NO_RESPONSE                 102

/* 7.3.1.8 AID field */
#define AID_FIELD_LEN                               2
#define AID_MASK                                    BITS(0, 13)
#define AID_MSB                                     BITS(14, 15)
#define AID_MIN_VALUE                               1
#define AID_MAX_VALUE                               2007

/* 7.3.1.9 Status Code field */
#define STATUS_CODE_FIELD_LEN                       2
/* Reserved - Used by TX Auth */
#define STATUS_CODE_RESERVED                        0
/* Successful */
#define STATUS_CODE_SUCCESSFUL                      0
/* Unspecified failure */
#define STATUS_CODE_UNSPECIFIED_FAILURE             1
/* Cannot support all requested cap in the Cap Info field */
#define STATUS_CODE_CAP_NOT_SUPPORTED               10
/* Reassoc denied due to inability to confirm that assoc exists */
#define STATUS_CODE_REASSOC_DENIED_WITHOUT_ASSOC    11
/* Assoc denied due to reason outside the scope of this std. */
#define STATUS_CODE_ASSOC_DENIED_OUTSIDE_STANDARD   12
/* Responding STA does not support the specified auth algorithm */
#define STATUS_CODE_AUTH_ALGORITHM_NOT_SUPPORTED    13
/* Rx an auth frame with auth transaction seq num out of expected seq */
#define STATUS_CODE_AUTH_OUT_OF_SEQ                 14
/* Auth rejected because of challenge failure */
#define STATUS_CODE_AUTH_REJECTED_CHAL_FAIL         15
/* Auth rejected due to timeout waiting for next frame in sequence */
#define STATUS_CODE_AUTH_REJECTED_TIMEOUT           16
/* Assoc denied because AP is unable to handle additional assoc STAs */
#define STATUS_CODE_ASSOC_DENIED_AP_OVERLOAD        17
/* Assoc denied due to requesting STA not supporting all of basic rates */
#define STATUS_CODE_ASSOC_DENIED_RATE_NOT_SUPPORTED 18
/* Assoc denied due to requesting STA not supporting short preamble */
#define STATUS_CODE_ASSOC_DENIED_NO_SHORT_PREAMBLE  19
/* Assoc denied due to requesting STA not supporting PBCC */
#define STATUS_CODE_ASSOC_DENIED_NO_PBCC            20
/* Assoc denied due to requesting STA not supporting channel agility */
#define STATUS_CODE_ASSOC_DENIED_NO_CH_AGILITY      21
/* Assoc rejected because Spectrum Mgt capability is required */
#define STATUS_CODE_ASSOC_REJECTED_NO_SPEC_MGT      22
/* Assoc rejected because the info in Power Capability is unacceptable */
#define STATUS_CODE_ASSOC_REJECTED_PWR_CAP          23
/* Assoc rejected because the info in Supported Channels is unacceptable */
#define STATUS_CODE_ASSOC_REJECTED_SUP_CHS          24
/* Assoc denied due to requesting STA not supporting short slot time */
#define STATUS_CODE_ASSOC_DENIED_NO_SHORT_SLOT_TIME 25
/* Assoc denied due to requesting STA not supporting DSSS-OFDM */
#define STATUS_CODE_ASSOC_DENIED_NO_DSSS_OFDM       26
/* R0KH unreachable */
#define STATUS_CODE_R0KH_UNREACHABLE                28
#if CFG_SUPPORT_802_11W
/*  IEEE 802.11w, Assoc denied due to the SA query */
#define STATUS_CODE_ASSOC_REJECTED_TEMPORARILY      30
/* IEEE 802.11w, Assoc denied due to the MFP select policy */
#define STATUS_CODE_ROBUST_MGMT_FRAME_POLICY_VIOLATION 31
#endif
/* Unspecified, QoS-related failure */
#define STATUS_CODE_UNSPECIFIED_QOS_FAILURE         32
/* Assoc denied due to insufficient bandwidth to handle another QSTA */
#define STATUS_CODE_ASSOC_DENIED_BANDWIDTH          33
/* Assoc denied due to excessive frame loss
 * rates and/or poor channel conditions
 */
#define STATUS_CODE_ASSOC_DENIED_POOR_CHANNEL       34
/* Assoc denied due to requesting STA not supporting QoS facility */
#define STATUS_CODE_ASSOC_DENIED_NO_QOS_FACILITY    35
/* Request has been declined */
#define STATUS_CODE_REQ_DECLINED                    37
/* Request has not been successful as one
 * or more parameters have invalid values
 */
#define STATUS_CODE_REQ_INVALID_PARAMETER_VALUE     38
/* TS not created because request cannot be honored. */
/* Suggested TSPEC provided. */
#define STATUS_CODE_REQ_NOT_HONORED_TSPEC           39
/* Invalid information element */
#define STATUS_CODE_INVALID_INFO_ELEMENT            40
/* Invalid group cipher */
#define STATUS_CODE_INVALID_GROUP_CIPHER            41
/* Invalid pairwise cipher */
#define STATUS_CODE_INVALID_PAIRWISE_CIPHER         42
/* Invalid AKMP */
#define STATUS_CODE_INVALID_AKMP                    43
/* Unsupported RSN information element version */
#define STATUS_CODE_UNSUPPORTED_RSN_IE_VERSION      44
/* Invalid RSN information element capabilities */
#define STATUS_CODE_INVALID_RSN_IE_CAP              45
/* Cipher suite rejected because of security policy */
#define STATUS_CODE_CIPHER_SUITE_REJECTED           46
/* TS not created because request cannot be honored. */
/* Attempt to create a TS later. */
#define STATUS_CODE_REQ_NOT_HONORED_TS_DELAY        47
/* Direct Link is not allowed in the BSS by policy */
#define STATUS_CODE_DIRECT_LINK_NOT_ALLOWED         48
/* Destination STA is not present within this QBSS */
#define STATUS_CODE_DESTINATION_STA_NOT_PRESENT     49
/* Destination STA is not a QSTA */
#define STATUS_CODE_DESTINATION_STA_NOT_QSTA        50
/* Association denied because the ListenInterval is too large */
#define STATUS_CODE_ASSOC_DENIED_LARGE_LIS_INTERVAL 51
/* Invalid pairwise master key identifier (PMKID) */
#define STATUS_INVALID_PMKID                        53
/* Invalid MDE */
#define STATUS_INVALID_MDE                          54
/* Invalid FTE */
#define STATUS_INVALID_FTE                          55
/* not support DH group */
#define STATUS_FINITE_CYCLIC_GROUP_NOT_SUPPORTED    77

#define STATUS_FILS_AUTH_FAILURE		    112
#define STATUS_UNKNOWN_AUTH_SERVER		    113
#define WLAN_STATUS_SAE_HASH_TO_ELEMENT             126
/* Denied because the requesting STA is afflicated with a
 * non-AP MLD that is associated with the AP MLD
 */
#define STATUS_CODE_DENIED_EXISTING_MLD_ASSOC       130
/* Denied non-AP MLD or non-AP EHT STA is not authorized to use the service */
#define STATUS_CODE_EPCS_DENIED_UNAUTHORIZED        131
/* Denied due to reason outside the scope of 11be */
#define STATUS_CODE_EPCS_DENIED_OTHER_REASON        132
/* Denied because the requested TID-to-link mapping is unacceptable */
#define STATUS_CODE_DENIED_TID_TO_LINK_MAPPING      133
/* Preferred TID-to-link mapping suggested */
#define STATUS_CODE_TID_TO_LINK_MAP_SUGGESTED       134
/* Denied because the requesting STA does not support EHT features */
#define STATUS_CODE_DENIED_EHT_NOT_SUPPORTED        135
/* Link not accepted because the link on which the (Re)Association
 * Request frame is transmitted is not accepted.
 */
#define STATUS_CODE_DENIED_LINK_NOT_ACCEPTED        136
/* Temporarily denied because unable to verify if STA is authorized */
#define STATUS_CODE_EPCS_DENIED_VERIFICATION_FAILURE 140

/* proprietary definition of reserved field of Status Code */
/* Join failure */
#define STATUS_CODE_JOIN_FAILURE                    0xFFF0
/* Join timeout */
#define STATUS_CODE_JOIN_TIMEOUT                    0xFFF1
/* Authentication timeout */
#define STATUS_CODE_AUTH_TIMEOUT                    0xFFF2
/* (Re)Association timeout */
#define STATUS_CODE_ASSOC_TIMEOUT                   0xFFF3
/* CCX CCKM reassociation failure */
#define STATUS_CODE_CCX_CCKM_REASSOC_FAILURE        0xFFF4
/* (Re)Association but receive deauth with REASON_CODE_PREV_AUTH_INVALID */
#define STATUS_CODE_ASSOC_PREV_AUTH_INVALID	    0xFFF5

/* Initalization value */
#define STATUS_CODE_INVALID                         0xFFFF

/* 7.3.1.10 Timestamp field */
#define TIMESTAMP_FIELD_LEN                         8

/* 80211-2016 Table 9-47 Category of Action field */
#define CATEGORY_SPEC_MGT                           0
/* QoS action */
#define CATEGORY_QOS_ACTION                         1
/* Direct Link Protocol (DLP) action */
#define CATEGORY_DLS_ACTION                         2
/* Block ack action */
#define CATEGORY_BLOCK_ACK_ACTION                   3
/* Public action */
#define CATEGORY_PUBLIC_ACTION                      4
/* Radio measurement action */
#define CATEGORY_RM_ACTION                          5
/* Fast BSS Transition */
#define CATEGORY_FT_ACTION                          6
/* HT */
#define CATEGORY_HT_ACTION                          7
/* SA Qurery */
#define CATEGORY_SA_QUERY_ACTION                    8
/* Protected Dual of Public Action */
#define CATEGORY_PROTECTED_DUAL_OF_PUBLIC_ACTION    9
/* 802.11v Wireless Network Management */
#define CATEGORY_WNM_ACTION                         10
/* 802.11v Unprotected Wireless Network Management */
#define CATEGORY_UNPROTECTED_WNM_ACTION             11
/* TDLS */
#define CATEGORY_TDLS_ACTION                        12
/* Mesh */
#define CATEGORY_MESH_ACTION                        13
/* Multihop */
#define CATEGORY_MULTIHOP_ACTION                    14
/* Self-protected */
#define CATEGORY_SELF_PROTECTED_ACTION              15
/* Directional Multi-Gigabit  */
#define CATEGORY_DMG_ACTION                         16
/* WME management notification */
#define CATEGORY_WME_MGT_NOTIFICATION               17
/* Fast Session Transfer */
#define CATEGORY_FST_ACTION                         18
/* Robust AV Streaming */
#define CATEGORY_ROBUST_AV_STREAMING_ACTION         19
/* Unprotected DMG */
#define CATEGORY_UNPROTECTED_DMG_ACTION             20
/* VHT action */
#define CATEGORY_VHT_ACTION                         21
#if (CFG_SUPPORT_TWT == 1)
#define CATEGORY_S1G_ACTION                         22  /* S1G action */
#endif
/* FILS action */
#define CATEGORY_FILS_ACTION                        26
/* EHT */
#define CATEGORY_EHT_ACTION			    36
/* Protected EHT */
#define CATEGORY_PROTECTED_EHT_ACTION		    37
/* Vendor-specific Protected */
#define CATEGORY_VENDOR_SPECIFIC_PROTECTED_ACTION   126
/* Vendor-specific */
#define CATEGORY_VENDOR_SPECIFIC_ACTION             127

/* 7.3.1.14 Block Ack Parameter Set field */
#define BA_PARAM_SET_ACK_POLICY_MASK                BIT(1)
#define BA_PARAM_SET_ACK_POLICY_MASK_OFFSET         1
#define BA_PARAM_SET_TID_MASK                       BITS(2, 5)
#define BA_PARAM_SET_TID_MASK_OFFSET                2
#define BA_PARAM_SET_BUFFER_SIZE_MASK               BITS(6, 15)
#define BA_PARAM_SET_BUFFER_SIZE_MASK_OFFSET        6

#define BA_PARAM_SET_ACK_POLICY_IMMEDIATE_BA        1
#define BA_PARAM_SET_ACK_POLICY_DELAYED_BA          0

/* 3 Management frame body components (II): Information Elements. */
/* 7.3.2 Element IDs of information elements */
#define ELEM_HDR_LEN                                2U

#define ELEM_ID_SSID \
	0 /* SSID */
#define ELEM_ID_SUP_RATES \
	1 /* Supported rates */
#define ELEM_ID_FH_PARAM_SET \
	2 /* FH parameter set */
#define ELEM_ID_DS_PARAM_SET \
	3 /* DS parameter set */
#define ELEM_ID_CF_PARAM_SET \
	4 /* CF parameter set */
#define ELEM_ID_TIM \
	5 /* TIM */
#define ELEM_ID_IBSS_PARAM_SET \
	6 /* IBSS parameter set */
#define ELEM_ID_COUNTRY_INFO \
	7 /* Country information */
#define ELEM_ID_HOPPING_PATTERN_PARAM \
	8 /* Hopping pattern parameters */
#define ELEM_ID_HOPPING_PATTERN_TABLE \
	9 /* Hopping pattern table */
#define ELEM_ID_REQUEST \
	10 /* Request */
#define ELEM_ID_BSS_LOAD \
	11 /* BSS load */
#define ELEM_ID_EDCA_PARAM_SET \
	12 /* EDCA parameter set */
#define ELEM_ID_TSPEC \
	13 /* Traffic specification (TSPEC) */
#define ELEM_ID_TCLAS \
	14 /* Traffic classification (TCLAS) */
#define ELEM_ID_SCHEDULE \
	15 /* Schedule */
#define ELEM_ID_CHALLENGE_TEXT \
	16 /* Challenge text */
#define ELEM_ID_PWR_CONSTRAINT \
	32 /* Power constraint */
#define ELEM_ID_PWR_CAP \
	33 /* Power capability */
#define ELEM_ID_TPC_REQ \
	34 /* TPC request */
#define ELEM_ID_TPC_REPORT \
	35 /* TPC report */
#define ELEM_ID_SUP_CHS \
	36 /* Supported channels */
#define ELEM_ID_CH_SW_ANNOUNCEMENT \
	37 /* Channel switch announcement */
#define ELEM_ID_MEASUREMENT_REQ \
	38 /* Measurement request */
#define ELEM_ID_MEASUREMENT_REPORT \
	39 /* Measurement report */
#define ELEM_ID_QUIET \
	40 /* Quiet */
#define ELEM_ID_IBSS_DFS \
	41 /* IBSS DFS */
#define ELEM_ID_ERP_INFO \
	42 /* ERP information */
#define ELEM_ID_TS_DELAY \
	43 /* TS delay */
#define ELEM_ID_TCLAS_PROCESSING \
	44 /* TCLAS processing */
#define ELEM_ID_HT_CAP \
	45 /* HT Capabilities subelement */
#define ELEM_ID_QOS_CAP \
	46 /* QoS capability */
#define ELEM_ID_RSN \
	48 /* RSN IE */
#define ELEM_ID_EXTENDED_SUP_RATES \
	50 /* Extended supported rates */
#define ELEM_ID_AP_CHANNEL_REPORT \
	51 /* AP Channel Report Element */
#define ELEM_ID_NEIGHBOR_REPORT \
	52 /* Neighbor Report */
#define ELEM_ID_RCPI \
	53 /* RCPI */
#define ELEM_ID_MOBILITY_DOMAIN \
	54 /* Mobility Domain for 802.11R */
#define ELEM_ID_FAST_TRANSITION \
	55 /* Fast Bss Transition for 802.11 R */
#define ELEM_ID_TIMEOUT_INTERVAL \
	56 /* 802.11w SA Timeout interval */
#define ELEM_ID_RESOURCE_INFO_CONTAINER \
	57 /* Resource Information Container for 802.11 R */
#define ELEM_ID_DSE_REG_LOC \
	58 /* DSE Registered Location */
#define ELEM_ID_SUP_OPERATING_CLASS \
	59 /* Supported Operating Classes */
#define ELEM_ID_EX_CH_SW_ANNOUNCEMENT \
	60 /* Extended Channel Switch Announcement */
#define ELEM_ID_HT_OP \
	61 /* HT Operation */
#define ELEM_ID_SCO \
	62 /* Secondary Channel Offset */
#define ELEM_ID_BSS_AVG_ACCESS_DELAY \
	63 /* BSS Average Access Delay */
#define	ELEM_ID_ANTENNA \
	64 /* Antenna */
#define	ELEM_ID_RSNI \
	65 /* RSNI */
#define	ELEM_ID_MPT \
	66 /* Measurement Pilot Transmission */
#define	ELEM_ID_BSS_AVAILABLE_ADMIN_CAP \
	67 /* BSS Available Admission Capacity */
#define	ELEM_ID_BSS_AC_ACCESS_DELAY \
	68 /* BSS AC Access Delay */
#define	ELEM_ID_TIME_AD \
	69 /* Time Advertisement */
#define ELEM_ID_RRM_ENABLED_CAP \
	70 /* Radio Resource Management Enabled Capabilities */
#define ELEM_ID_MBSSID  \
	71 /* Multiple BSSID element */
#define ELEM_ID_20_40_BSS_COEXISTENCE \
	72 /* 20/40 BSS Coexistence */
#define ELEM_ID_20_40_INTOLERANT_CHNL_REPORT \
	73 /* 20/40 BSS Intolerant Channel Report */
#define ELEM_ID_OBSS_SCAN_PARAMS \
	74 /* Overlapping BSS Scan Parameters */
#define ELEM_ID_NON_TX_CAP \
	83 /* Nontransmitted BSSID Capability element*/
#define ELEM_ID_MBSSID_INDEX \
	85 /* Multiple BSSID-Index element */
#define ELEM_ID_FMS_DESC \
	86 /* FMS Descriptor */
#define ELEM_ID_FMS_REQ \
	87 /* FMS Request */
#define ELEM_ID_FMS_RSP \
	88 /* FMS Response */
#define ELEM_ID_QOS_TRAFFIC_CAP \
	89  /* QoS Traffic Capability */
#define ELEM_ID_BSS_MAX_IDLE_PERIOD \
	90 /* BSS Max Idle Period */
#define ELEM_ID_TIM_BROADCAST_REQ \
	94 /* TIM Broadcast Request */
#define ELEM_ID_TIM_BROADCAST_RESP \
	95 /* TIM Broadcast Response */
#define ELEM_ID_CHNNEL_USAGE \
	97 /* Channel Usage */
#define ELEM_ID_TIME_ZONE \
	98 /* Time Zone */
#define ELEM_ID_DMS_REQ \
	99 /* DMS Request */
#define ELEM_ID_DMS_RSP \
	100 /* DMS Response */
#define ELEM_ID_CH_SWITCH_TIMING \
	104 /* Channel Switch Timing */
#define ELEM_ID_INTERWORKING \
	107 /* Interworking with External Network */
#define ELEM_ID_ADVERTISEMENT_PROTOCOL \
	108 /* Advertisement Protocol */
#define ELEM_ID_QOS_MAP_SET \
	110 /* QoS Map Set */
#define ELEM_ID_ROAMING_CONSORTIUM \
	111 /* Roaming Consortium */
#define ELEM_ID_EAI \
	112 /* Emergency Alert Identifier */
#define ELEM_ID_MESH_CONFIG \
	113 /* Mesh Configuration */
#define ELEM_ID_MESH_ID \
	114 /* Mesh ID */
#define ELEM_ID_MESH_CH_SW_PARAM \
	118 /* Mesh Channel Switch Parameters */
#define ELEM_ID_MESH_AWAKE_WINDOW \
	119 /* Mesh Awake Window */
#define ELEM_ID_BEACON_TIMING \
	120 /* Beacon Timing */
#define ELEM_ID_MCCAOP_AD \
	123 /* MCCAOP Advertisement */
#define ELEM_ID_EXTENDED_CAP \
	127 /* Extended capabilities */
#define ELEM_ID_DMG_CAP \
	148 /* DMG Capabilities */
#define ELEM_ID_DMG_OP \
	151 /* DMG Operation */
#define ELEM_ID_MULTI_BAND \
	158 /* Multi-band */
#define ELEM_ID_ADDBA_EXT \
	159 /* ADDBA Extension */
#define ELEM_ID_RELAY_CAP \
	167 /* Relay Capabilities */
#define ELEM_ID_MULTI_MAC_SUBLAYERS \
	170 /* Multiple MAC Sublayers */
#define ELEM_ID_MCCAOP_AD_OVERVIEW \
	123 /* MCCAOP Advertisement Overview */
#define ELEM_ID_QMF_POLICY \
	181 /* QMF Policy */
#define ELEM_ID_QLOAD_REPORT \
	186 /* QLoad Report */
#define ELEM_ID_HCCA_TXOP_UPDATE_COUNT \
	187 /* HCCA TXOP Update Count */
#define ELEM_ID_ANT_SECTOR_ID_PATTERN \
	190 /* Antenna Sector ID Pattern */
#define ELEM_ID_VHT_CAP \
	191 /* VHT Capabilities subelement */
#define ELEM_ID_VHT_OP \
	192 /* VHT Operation information */
#define ELEM_ID_EX_BSS_LOAD \
	193 /* Extended BSS Load */
#define ELEM_ID_WIDE_BAND_CHANNEL_SWITCH \
	194 /* Wide Bandwidth Channel Switch */
#define ELEM_ID_TX_PWR_ENVELOPE \
	195 /* Transmit Power Envelope */
#define ELEM_ID_CH_SW_WRAPPER \
	196 /* Channel Switch Wrapper */
#define ELEM_ID_QUIET_CHANNEL \
	198 /* Quiet Channel */
#define ELEM_ID_OP_MODE \
	199 /* Operation Mode Notification */
#define ELEM_ID_RNR \
	201 /* Reduced Neighbor Report */
#define ELEM_ID_TVHT_OP \
	202 /* TVHT Operation */
#define ELEM_ID_S1G_OLLM_INDEX \
	207 /* S1G Open-Loop Link Margin Index */
#define ELEM_ID_RPS \
	208 /* RPS */
#define ELEM_ID_PAGE_SLICE \
	209	/* Page Slice */
#define ELEM_ID_AID_REQ \
	210 /* AID Request */
#define ELEM_ID_AID_RESP \
	211 /* AID Response */
#define ELEM_ID_S1G_SECTOR_OP \
	212 /* S1G Sector Operation */
#define ELEM_ID_SHORT_BEACON_INTERVAL \
	214 /* Short Beacon Interval */
#define ELEM_ID_CHANGE_SEQ \
	215 /* Change Sequence */
#define ELEM_ID_TWT \
	216 /* Target Wake Time (TWT) @11ah/11ax */
#define ELEM_ID_S1G_CAP \
	217 /* S1G Capabilities */
#define ELEM_ID_VENDOR \
	221 /* Vendor specific IE */
#define ELEM_ID_WPA \
	ELEM_ID_VENDOR	/* WPA IE */
#define ELEM_ID_WMM \
	ELEM_ID_VENDOR	/* WMM IE */
#define ELEM_ID_P2P \
	ELEM_ID_VENDOR	/* WiFi Direct */
#define ELEM_ID_WSC \
	ELEM_ID_VENDOR	/* WSC IE */
#define ELEM_ID_TSF_TIMER_ACCURACY \
	223 /* TSF Timer Accuracy */
#define ELEM_ID_S1G_RELAY \
	224 /* S1G Relay */
#define ELEM_ID_REACHABLE_ADDR \
	225 /* Reachable Address */
#define ELEM_ID_S1G_RELAY_DISCOVERY \
	226 /* S1G Relay Discovery */
#define ELEM_ID_EL_OP \
	230 /* EL Operation */
#define ELEM_ID_SECTORIZED_GRP_ID_LIST \
	231 /* Sectorized Group ID List */
#define ELEM_ID_S1G_OP \
	232 /* S1G Operation */
#define ELEM_ID_HEADER_COMPRESSION \
	233 /* Header Compression */
#define ELEM_ID_SST_OP \
	234 /* SST Operation */
#define ELEM_ID_MAD \
	235 /* MAD */
#define ELEM_ID_S1G_RELAY_ACTIVATION \
	236 /* S1G Relay Activation */
#define ELEM_ID_CAG_NUM \
	237 /* CAG Number */
#define ELEM_ID_AP_CSN \
	239 /* AP-CSN */
#define ELEM_ID_FILS_INDICATION \
	240 /* FILS Indication */
#define ELEM_ID_DILS \
	241 /* DILS */
#define ELEM_ID_FRAGMENT \
	242 /* FRAGMENT */
#define ELEM_ID_RSNX \
	244 /* RSN Extension */
#define ELEM_ID_RESERVED \
	255 /* Reserved */
#define	ELEM_ID_EXTENSION \
	255
#define ELEM_ID_MAX_NUM \
	256 /* EID: 0-255 */

#define ELEM_EXT_ID_ASSOC_DELAY_INFO \
	1 /* Association Delay Info */
#define ELEM_EXT_ID_FILS_REQUEST_PARA \
	2 /* FILS Request Parameters */
#define ELEM_EXT_ID_FILS_KEY_CONFIRM \
	3 /* FILS Key Confirmation */
#define ELEM_EXT_ID_FILS_SESSION \
	4 /* FILS Session */
#define ELEM_EXT_ID_FILS_HLP_CONTAINER \
	5 /* FILS HLP Container */
#define ELEM_EXT_ID_FILS_IP_ADDR_ASSIGN \
	6 /* FILS IP Address Assignment */
#define ELEM_EXT_ID_KEY_DELIVERY \
	7 /* Key Delivery */
#define ELEM_EXT_ID_FILS_WRAPPED_DATA \
	8 /* FILS Wrapped Data */
#define ELEM_EXT_ID_FILS_SYNC_INFO \
	9 /* FTM Synchronization Information */
#define ELEM_EXT_ID_ESP	\
	11 /* Estimated Service Parameters Inbound */
#define ELEM_EXT_ID_FILS_PUBLIC_KEY \
	12 /* FILS Public Key */
#define ELEM_EXT_ID_FILS_NONCE \
	13 /* FILS Nonce */
#define ELEM_EXT_ID_FUTURE_CHNL_GUIDE \
	14 /* Future Channel Guidance */
#define ELEM_EXT_ID_SERVICE_HINT \
	15 /* Service Hint */
#define ELEM_EXT_ID_SERVICE_HASH \
	16 /* Service Hash */
#define ELEM_EXT_ID_CDMG_CAP \
	17 /* CDMG Capabilities */
#define ELEM_EXT_ID_EX_CLUSTER_REPORT \
	22 /* Extended Cluster Report */
#define ELEM_EXT_ID_CMMG_CAP \
	27 /* CMMG Capabilities */
#define ELEM_EXT_ID_CMMG_OP \
	28 /* CMMG Operation */
#define ELEM_EXT_ID_DIFFIE_HELLMAN_PARAM \
	32 /* OWE: Diffie-Hellman Parameter */
#define ELEM_EXT_ID_GLK_GCR_PARAM_SET \
	34 /* GLK-GCR Parameter Set */
#define ELEM_EXT_ID_HE_CAP \
	35 /* HE Capabilities */
#define ELEM_EXT_ID_HE_OP \
	36 /* HE Operation */
#define ELEM_EXT_ID_UORA_PARAM \
	37 /* UL OFDMA-based Random Access (UORA) Parameter Set element */
#define ELEM_EXT_ID_MU_EDCA_PARAM \
	38 /* MU EDCA Parameter Set element */
#define ELEM_EXT_ID_SR_PARAM \
	39 /* Spatial Reuse Parameter Set element */
#define ELEM_EXT_ID_NDP_FEEDBACK \
	41 /* NDP Feedback Report Parameter Set  */
#define ELEM_EXT_ID_BSS_COLOR_CHANGE \
	42 /* BSS Color Change Announcement */
#define ELEM_EXT_ID_ESS_REPORT \
	45 /* ESS Report */
#define ELEM_EXT_ID_HE_BSS_LOAD \
	47 /* HE BSS Load */
#define ELEM_EXT_ID_MAX_CH_SW_TIME \
	52 /* Max Channel Switch Time */
#define ELEM_EXT_ID_ESP_OUTBOUND \
	53 /* Estimated Service Parameters Outbound */
#define ELEM_EXT_ID_OCI \
	54 /* Operating Channel Information */
#define ELEM_EXT_ID_MBSS_CONFIG \
	55 /* Multiple BSSD Configuration */
#define ELEM_EXT_ID_NON_INHERITANCE \
	56 /* Non-Inheritance */
#define ELEM_EXT_ID_HE_6G_BAND_CAP \
	59 /* HE 6G Band Capabilities */
#define ELEM_EXT_ID_UL_MU_Power_CAP \
	60 /* UL MU Power Capabilities */
#define ELEM_EXT_ID_MSCS_DESCRIPTOR \
	88 /* MSCS Descriptor */
#define ELEM_EXT_ID_SUPPLEMENTAL_CLASS2_CAP \
	90 /* Supplemental Class 2 Capabilities */
#define ELEM_EXT_ID_REJECTED_GROUPS \
	92 /* Rejected Groups */
#define ELEM_EXT_ID_ACTC \
	93 /* Anti-Clogging Token Container */
#define ELEM_EXT_ID_EHT_OP \
	106 /* EHT Operation */
#define ELEM_EXT_ID_MLD \
	107 /* Multi-Link element */
#define ELEM_EXT_ID_EHT_CAPS \
	108 /* EHT Capabilities */
#define ELEM_EXT_ID_TID2LNK_MAP \
	109 /* TID2LNK */
#define ELEM_EXT_ID_MLT_INDICATION \
	110 /* Multi-link Traffic Indication */
#define ELEM_EXT_ID_QOS_CHAR \
	113 /* QoS Characteristics */
#define ELEM_EXT_ID_MAX_NUM \
	256 /* EXT_ID: 0-255 */


#define MBO_IE_VENDOR_TYPE 0x506f9a16
#define MBO_OUI_TYPE 22

/* MBO v0.0_r19, 4.2: MBO Attributes */
/* Table 4-5: MBO Attributes */
/* OCE v0.0.10, Table 4-3: OCE Attributes */
enum MBO_ATTR_ID {
	MBO_ATTR_ID_AP_CAPA_IND = 1,
	MBO_ATTR_ID_NON_PREF_CHAN_REPORT = 2,
	MBO_ATTR_ID_CELL_DATA_CAPA = 3,
	MBO_ATTR_ID_ASSOC_DISALLOW = 4,
	MBO_ATTR_ID_CELL_DATA_PREF = 5,
	MBO_ATTR_ID_TRANSITION_REASON = 6,
	MBO_ATTR_ID_TRANSITION_REJECT_REASON = 7,
	MBO_ATTR_ID_ASSOC_RETRY_DELAY = 8,
	OCE_ATTR_ID_CAPA_IND = 101,
	OCE_ATTR_ID_RSSI_BASED_ASSOC_REJECT = 102,
	OCE_ATTR_ID_REDUCED_WAN_METRICS = 103,
	OCE_ATTR_ID_RNR_COMPLETENESS = 104,
	OCE_ATTR_ID_SUPPRESSION_BSSID = 105,
	OCE_ATTR_ID_SUPPRESSION_SSID = 106,
	OCE_ATTR_ID_TRANSMIT_POWER = 107,
};

/* MBO-OCE */
#define OCE_ATTIBUTE_REDUCED_WAN_METRICS_MASK       BITS(0, 3)

/* MBO v0.0_r19, 4.2.7: Transition Rejection Reason Code Attribute */
/* Table 4-21: Transition Rejection Reason Code Field Values */
enum MBO_TRANSITION_REJECT_REASON {
	MBO_TRANSITION_REJECT_REASON_UNSPECIFIED = 0,
	MBO_TRANSITION_REJECT_REASON_FRAME_LOSS = 1,
	MBO_TRANSITION_REJECT_REASON_DELAY = 2,
	MBO_TRANSITION_REJECT_REASON_QOS_CAPACITY = 3,
	MBO_TRANSITION_REJECT_REASON_RSSI = 4,
	MBO_TRANSITION_REJECT_REASON_INTERFERENCE = 5,
	MBO_TRANSITION_REJECT_REASON_SERVICES = 6,
};

/* 802.11be, Protested EHT */
/* D3.0 Table 9-623c Protected EHT Action Field Values */
enum PROTECTED_EHT_ACTION {
	TID2LINK_REQUEST,
	TID2LINK_RESPONSE,
	TID2LINK_TEARDOWN,
	EPCS_ENABLE_REQUEST,
	EPCS_ENABLE_RESPONSE,
	EPCS_TEARDOWN,
	PROTECTED_EHT_ACTION_NUM,
};

#define FILS_INFO_NUM_PUB_KEY_ID	BITS(0, 2)
#define FILS_INFO_NUM_REALM_ID		BITS(3, 5)
#define FILS_INFO_IP_ADDR_CONFIG	BIT(6)
#define FILS_INFO_CACHE_ID_INCLUDED	BIT(7)
#define FILS_INFO_HESSID_INCLUDED	BIT(8)
#define FILS_INFO_SK_SUPPORTED		BIT(9)
#define FILS_INFO_SK_PFS_SUPPORTED	BIT(10)
#define FILS_INFO_PK_SUPPORTED		BIT(11)

/* 7.3.2.1 SSID element */
#define ELEM_MAX_LEN_SSID                           32

/* 7.3.2.2 Supported Rates */
#define ELEM_MAX_LEN_SUP_RATES                      8

#define ELEM_MAX_LEN_SUP_RATES_IOT		    16

/* 7.3.2.4 DS Parameter Set */
#define ELEM_MAX_LEN_DS_PARAMETER_SET               1

/* 7.3.2.5 CF Parameter Set */
#define ELEM_CF_PARM_LEN                            8

/* 7.3.2.6 TIM */
#define ELEM_MIX_LEN_TIM                            4
#define ELEM_MAX_LEN_TIM                            254

/* 7.3.2.7 IBSS Parameter Set element */
#define ELEM_MAX_LEN_IBSS_PARAMETER_SET             2

/* 7.3.2.8 Challenge Text element */
#define ELEM_MIN_LEN_CHALLENGE_TEXT                 1
#define ELEM_MAX_LEN_CHALLENGE_TEXT                 253

/* 7.3.2.9 Country Information element */
/* Country IE should contain at least 3-bytes country
 * code string and one subband triplet.
 */
#define ELEM_MIN_LEN_COUNTRY_INFO                   6
#define ELEM_MAX_LEN_COUNTRY_INFO                   254

#define ELEM_ID_COUNTRY_INFO_TRIPLET_LEN_FIXED              3
#define ELEM_ID_COUNTRY_INFO_SUBBAND_TRIPLET_LEN_FIXED      3
#define ELEM_ID_COUNTRY_INFO_REGULATORY_TRIPLET_LEN_FIXED   3

/* 7.3.2.13 ERP Information element */
#define ELEM_MAX_LEN_ERP                            1
/* -- bits in the ERP Information element */
/* NonERP_Present bit */
#define ERP_INFO_NON_ERP_PRESENT                    BIT(0)
/* Use_Protection bit */
#define ERP_INFO_USE_PROTECTION                     BIT(1)
/* Barker_Preamble_Mode bit */
#define ERP_INFO_BARKER_PREAMBLE_MODE               BIT(2)

#define ELEM_MAX_LEN_SUPPORTED_CHANNELS            (MAX_CHN_NUM * 2)

/* 7.3.2.14 Extended Supported Rates */
#define ELEM_MAX_LEN_EXTENDED_SUP_RATES             255

/* 7.3.2.16 Power Capability element */
#define ELEM_MAX_LEN_POWER_CAP                      2


/* 7.3.2.21 Measurement Request element */
#define ELEM_RM_TYPE_BASIC_REQ                      0
#define ELEM_RM_TYPE_CCA_REQ                        1
#define ELEM_RM_TYPE_RPI_HISTOGRAM_REQ              2
#define ELEM_RM_TYPE_CHNL_LOAD_REQ                  3
#define ELEM_RM_TYPE_NOISE_HISTOGRAM_REQ            4
#define ELEM_RM_TYPE_BEACON_REQ                     5
#define ELEM_RM_TYPE_FRAME_REQ                      6
#define ELEM_RM_TYPE_STA_STATISTICS_REQ             7
#define ELEM_RM_TYPE_LCI_REQ                        8
#define ELEM_RM_TYPE_TSM_REQ                        9
#define ELEM_RM_TYPE_MEASURE_PAUSE_REQ              255

/* 7.3.2.22 Measurement Report element */
#define ELEM_RM_TYPE_BASIC_REPORT                   0
#define ELEM_RM_TYPE_CCA_REPORT                     1
#define ELEM_RM_TYPE_RPI_HISTOGRAM_REPORT           2
#define ELEM_RM_TYPE_CHNL_LOAD_REPORT               3
#define ELEM_RM_TYPE_NOISE_HISTOGRAM_REPORT         4
#define ELEM_RM_TYPE_BEACON_REPORT                  5
#define ELEM_RM_TYPE_FRAME_REPORT                   6
#define ELEM_RM_TYPE_STA_STATISTICS_REPORT          7
#define ELEM_RM_TYPE_LCI_REPORT                     8
#define ELEM_RM_TYPE_TSM_REPORT                     9

/* 7.3.2.37 Subelement IDs for Neighbor Report,  Table 7-43b  */
#define ELEM_ID_NR_BSS_TRANSITION_CAND_PREF	    3
#define ELEM_ID_NR_BSS_TERMINATION_DURATION	    4
#define ELEM_ID_NR_EHT_CAPABILITIES		    199
#define ELEM_ID_NR_EHT_OPERATION		    200
#define ELEM_ID_NR_BASIC_MULTI_LINK		    201


/*
 * IEEE Std 802.11-2016, Table 9-87 - Measurement Mode definitions for Beacon
 * request
 */
enum BEACON_REPORT_MODE {
	BEACON_REPORT_MODE_PASSIVE = 0,
	BEACON_REPORT_MODE_ACTIVE = 1,
	BEACON_REPORT_MODE_TABLE = 2,
};

/* IEEE Std 802.11-2016, Table 9-88 - Beacon Request subelement IDs
 * IEEE P802.11-REVmd/D2.0, Table 9-106 - Optional subelement IDs for
 * Beacon request
 */
#define BEACON_REQUEST_SUBELEM_SSID		0
#define BEACON_REQUEST_SUBELEM_INFO		1 /* Beacon Reporting */
#define BEACON_REQUEST_SUBELEM_DETAIL		2 /* Reporting Detail */
#define BEACON_REQUEST_SUBELEM_REQUEST		10
#define BEACON_REQUEST_SUBELEM_EXT_REQUEST	11
#define BEACON_REQUEST_SUBELEM_AP_CHANNEL	51 /* AP Channel Report */
#define BEACON_REQUEST_SUBELEM_LAST_INDICATION	164
#define BEACON_REQUEST_SUBELEM_VENDOR	221

/*
 * IEEE Std 802.11-2016, Table 9-90 - Reporting Detail values
 */
enum BEACON_REPORT_DETAIL {
	/* No fixed-length fields or elements */
	BEACON_REPORT_DETAIL_NONE = 0,
	/* All fixed-length fields and any requested elements in the Request
	 * element if resent
	 */
	BEACON_REPORT_DETAIL_REQUESTED_ONLY = 1,
	/* All fixed-length fields and elements (default, used when Reporting
	 * Detail subelement is not included in a Beacon request)
	 */
	BEACON_REPORT_DETAIL_ALL_FIELDS_AND_ELEMENTS = 2,
};

/* IEEE Std 802.11-2016, Table 9-112 - Beacon report Subelement IDs
 * IEEE P802.11-REVmd/D2.0, Table 9-130 - Optional subelement IDs for
 * Beacon report
 */
#define BEACON_REPORT_SUBELEM_FRAME_BODY	1
#define BEACON_REPORT_SUBELEM_FRAME_BODY_FRAGMENT_ID	2
#define BEACON_REPORT_SUBELEM_LAST_INDICATION	164
#define BEACON_REPORT_SUBELEM_VENDOR	221

/* IEEE P802.11-REVmd/D2.0, Table 9-232 - Data field format of the
 * Reported Frame Body Fragment ID subelement
 */
#define REPORTED_FRAME_BODY_SUBELEM_LEN		4
#define REPORTED_FRAME_BODY_MORE_FRAGMENTS	BIT(7)

/* IEEE P802.11-REVmd/D2.0, 9.4.2.21.7 - Beacon report  */
#define BEACON_REPORT_LAST_INDICATION_SUBELEM_LEN	3

/* IEEE Std 802.11-2016, Figure 9-192 - Measurement Report Mode field */
#define MEASUREMENT_REPORT_MODE_ACCEPT 0
#define MEASUREMENT_REPORT_MODE_REJECT_LATE BIT(0)
#define MEASUREMENT_REPORT_MODE_REJECT_INCAPABLE BIT(1)
#define MEASUREMENT_REPORT_MODE_REJECT_REFUSED BIT(2)

/* 7.3.2.25 RSN information element */
/* two pairwise, one AKM suite, one PMKID */
#define ELEM_MAX_LEN_WPA                            38
/* two pairwise, one AKM suite, one PMKID */
#define ELEM_MAX_LEN_RSN                            42
/* one pairwise, one AKM suite, one BKID */
#define ELEM_MAX_LEN_WAPI                           38
/* one pairwise, one AKM suite, one BKID */
#define ELEM_MAX_LEN_WSC                            200

#if CFG_SUPPORT_802_11W
#define ELEM_WPA_CAP_MFPR                           BIT(6)
#define ELEM_WPA_CAP_MFPC                           BIT(7)
#endif

/* 7.3.2.27 Extended Capabilities information element */
#define ELEM_EXT_CAP_20_40_COEXIST_SUPPORT          BIT(0)
#define ELEM_EXT_CAP_ECSA_CAP                       BIT(2)
#define ELEM_EXT_CAP_PSMP_CAP                       BIT(4)
#define ELEM_EXT_CAP_SERVICE_INTERVAL_GRANULARITY   BIT(5)
#define ELEM_EXT_CAP_SCHEDULE_PSMP                  BIT(6)

#define ELEM_EXT_CAP_20_40_COEXIST_SUPPORT_BIT      0
#define ELEM_EXT_CAP_ECSA_CAP_BIT                   2
#define ELEM_EXT_CAP_BSS_TRANSITION_BIT             19
#define ELEM_EXT_CAP_MBSSID_BIT                     22
#if CFG_STAINFO_FEATURE
#define ELEM_EXT_CAP_PROXY_ARP_BIT                  12
#define ELEM_EXT_CAP_TFS_BIT                        16
#define ELEM_EXT_CAP_WNM_SLEEP_BIT                  17
#define ELEM_EXT_CAP_TIM_BCAST_BIT                  18
#define ELEM_EXT_CAP_DMS_BIT                        26
#endif
#define ELEM_EXT_CAP_UTC_TSF_OFFSET_BIT             27
#define ELEM_EXT_CAP_INTERWORKING_BIT               31
#define ELEM_EXT_CAP_QOSMAPSET_BIT                  32
#define ELEM_EXT_CAP_WNM_NOTIFICATION_BIT           46
#define ELEM_EXT_CAP_WNM_NOTIFICATION_BIT           46
#define ELEM_EXT_CAP_OP_MODE_NOTIFICATION_BIT       62
#if (CFG_SUPPORT_TX_PWR_ENV == 1)
#define ELEM_EXT_CAP_EXT_SPEC_MGMT_CAPABLE_BIT      73
#endif
#if (CFG_SUPPORT_TWT == 1)
#define ELEM_EXT_CAP_TWT_REQUESTER_BIT              77
#endif
#define ELEM_EXT_CAP_SAE_PW_USED_BIT                81
#define ELEM_EXT_CAP_SAE_PW_EX_BIT                  82
#define ELEM_EXT_CAP_BCN_PROT_BIT                   84
#define ELEM_EXT_CAP_MSCS_BIT                       85
#define ELEM_EXT_CAP_SAE_PK_BIT                     88

#define ELEM_MAX_LEN_EXT_CAP_11ABGNAC               (8)

#if (CFG_SUPPORT_802_11BE == 1)
/* TODO */
#endif

/* This length should synchronize with wpa_supplicant */
#define ELEM_MAX_LEN_EXT_CAP                        (11)

/* 7.3.2.30 TSPEC element */
/* WMM: 0 (Asynchronous TS of low-duty cycles) */
#define TS_INFO_TRAFFIC_TYPE_MASK                   BIT(0)
#define TS_INFO_TID_OFFSET                          1
#define TS_INFO_TID_MASK                            BITS(1, 4)
#define TS_INFO_DIRECTION_OFFSET                    5
#define TS_INFO_DIRECTION_MASK                      BITS(5, 6)
#define TS_INFO_ACCESS_POLICY_OFFSET                7
#define TS_INFO_ACCESS_POLICY_MASK                  BITS(7, 8)
#define TS_INFO_AGGREGATION_MASK                    BIT(9)	/* WMM: 0 */
#define TS_INFO_APSD_MASK                           BIT(10)
#define TS_INFO_UP_OFFSET                           11
#define TS_INFO_UP_MASK                             BITS(11, 13)
#define TS_INFO_ACK_POLICY_OFFSET                   14
#define TS_INFO_ACK_POLICY_MASK                     BITS(14, 15)
#define TS_INFO_SCHEDULE_MASK                       16

/* 7.3.2.45 RRM Enabled Capbility element */
#define ELEM_MAX_LEN_RRM_CAP                        5
#define RRM_CAP_INFO_LINK_MEASURE_BIT               0
#define RRM_CAP_INFO_NEIGHBOR_REPORT_BIT            1
#define RRM_CAP_INFO_REPEATED_MEASUREMENT           3
#define RRM_CAP_INFO_BEACON_PASSIVE_MEASURE_BIT     4
#define RRM_CAP_INFO_BEACON_ACTIVE_MEASURE_BIT      5
#define RRM_CAP_INFO_BEACON_TABLE_BIT               6
#define RRM_CAP_INFO_CHANNEL_LOAD_MEASURE_BIT       9
#define RRM_CAP_INFO_NOISE_HISTOGRAM_MEASURE_BIT    10
#define RRM_CAP_INFO_TSM_BIT                        14
#define RRM_CAP_INFO_RRM_BIT                        17
#if CFG_STAINFO_FEATURE
#define RRM_CAP_INFO_QBSS_LOAD_BIT                  9
#define RRM_CAP_INFO_BSS_AVG_DELAY_BIT              31
#endif

/* 7.3.2.56 HT capabilities element */
#define ELEM_MAX_LEN_HT_CAP \
	(28 - ELEM_HDR_LEN)	/* sizeof(IE_HT_CAP_T)-2 */

/* 7.3.2.56.2 HT capabilities Info field */
#define HT_CAP_INFO_LDPC_CAP                        BIT(0)
#define HT_CAP_INFO_SUP_CHNL_WIDTH                  BIT(1)
#define HT_CAP_INFO_SM_POWER_SAVE                   BITS(2, 3)
#define HT_CAP_INFO_SM_POWER_SAVE_OFFSET            2
#define HT_CAP_INFO_HT_GF                           BIT(4)
#define HT_CAP_INFO_SHORT_GI_20M                    BIT(5)
#define HT_CAP_INFO_SHORT_GI_40M                    BIT(6)
#define HT_CAP_INFO_TX_STBC                         BIT(7)
#define HT_CAP_INFO_RX_STBC                         BITS(8, 9)
#define HT_CAP_INFO_HT_DELAYED_BA                   BIT(10)
#define HT_CAP_INFO_MAX_AMSDU_LEN                   BIT(11)
#define HT_CAP_INFO_DSSS_CCK_IN_40M                 BIT(12)
#define HT_CAP_INFO_40M_INTOLERANT                  BIT(14)
#define HT_CAP_INFO_LSIG_TXOP_SUPPORT               BIT(15)

#define HT_CAP_INFO_RX_STBC_NO_SUPPORTED            0
#define HT_CAP_INFO_RX_STBC_1_SS                    BIT(8)
#define HT_CAP_INFO_RX_STBC_2_SS                    BIT(9)
#define HT_CAP_INFO_RX_STBC_3_SS                    HT_CAP_INFO_RX_STBC

#define ELEM_MAX_LEN_VHT_CAP \
	(14 - ELEM_HDR_LEN)	/* sizeof(IE_VHT_CAP_T)-2 */
/* 8.4.2.161 VHT Operation element */
#define ELEM_MAX_LEN_VHT_OP \
	(7 - ELEM_HDR_LEN)	/* sizeof(IE_VHT_OP_T)-2 */

#define ELEM_MAX_LEN_VHT_OP_MODE_NOTIFICATION \
	(3 - ELEM_HDR_LEN)	/* sizeof(IE_VHT_OP_MODE_T)-2 */

#define ELEM_MAX_LEN_TPE \
	(8 - ELEM_HDR_LEN)	/* sizeof(IE_VHT_TPE)-2 */

#define ELEM_MAX_LEN_RNR \
	(19 - ELEM_HDR_LEN)	/* sizeof(IE_RNR)-2 */

#define ELEM_MAX_LEN_BSS_MAX_IDLE \
	(5 - ELEM_HDR_LEN)

#define BSS_MAX_IDLE_PERIOD_VALUE	30

/*8.4.2.160.3 VHT Supported MCS Set field*/

/*8.4.2.160.2 VHT Capabilities Info field*/
#define VHT_CAP_INFO_MAX_MPDU_LEN_3K          0
#define VHT_CAP_INFO_MAX_MPDU_LEN_8K          BIT(0)
#define VHT_CAP_INFO_MAX_MPDU_LEN_11K         BIT(1)
#define VHT_CAP_INFO_MAX_MPDU_LEN_MASK        BITS(0, 1)

#define VHT_CAP_INFO_MAX_SUP_CHANNEL_WIDTH_SET_NONE       0
#define VHT_CAP_INFO_MAX_SUP_CHANNEL_WIDTH_SET_160        BIT(2)
#define VHT_CAP_INFO_MAX_SUP_CHANNEL_WIDTH_SET_160_80P80  BIT(3)
#define VHT_CAP_INFO_MAX_SUP_CHANNEL_WIDTH_MASK           BITS(2, 3)

#define VHT_CAP_INFO_RX_LDPC                  BIT(4)
#define VHT_CAP_INFO_SHORT_GI_80              BIT(5)
#define VHT_CAP_INFO_SHORT_GI_160_80P80       BIT(6)
#define VHT_CAP_INFO_TX_STBC                  BIT(7)

#define VHT_CAP_INFO_RX_STBC_NONE             0
#define VHT_CAP_INFO_RX_STBC_MASK             BITS(8, 10)
#define VHT_CAP_INFO_RX_STBC_OFFSET           8
#define VHT_CAP_INFO_RX_STBC_ONE_STREAM       BIT(8)
#define VHT_CAP_INFO_RX_STBC_TWO_STREAM       BIT(9)
#define VHT_CAP_INFO_RX_STBC_THREE_STREAM     BITS(8, 9)
#define VHT_CAP_INFO_RX_STBC_FOUR_STREAM      BIT(10)

#define VHT_CAP_INFO_SU_BEAMFORMER_CAPABLE    BIT(11)
#define VHT_CAP_INFO_SU_BEAMFORMEE_CAPABLE    BIT(12)
#define VHT_CAP_INFO_SU_BEAMFORMEE_CAPABLE_OFFSET    12

/* VHT_CAP_INFO_COMPRESSED_STEERING_NUMBER_OF
 * _BEAMFORMER_ANTENNAS_SUPPOERTED
 */
#define VHT_CAP_INFO_COMPRESSED_STEERING_NUMBER_OF_BEAMFORMER_ANTENNAS_SUP_OFF \
	13
#define VHT_CAP_INFO_COMPRESSED_STEERING_NUMBER_OF_BEAMFORMER_ANTENNAS_SUP \
	BITS(13, 15)
#define VHT_CAP_INFO_COMPRESSED_STEERING_NUMBER_OF_BEAMFORMER_ANTENNAS_2_SUP \
	BIT(13)
#define VHT_CAP_INFO_COMPRESSED_STEERING_NUMBER_OF_BEAMFORMER_ANTENNAS_3_SUP \
	BIT(14)
#define VHT_CAP_INFO_COMPRESSED_STEERING_NUMBER_OF_BEAMFORMER_ANTENNAS_4_SUP \
	BITS(13, 14)

#define VHT_CAP_INFO_NUMBER_OF_SOUNDING_DIMENSIONS_OFFSET      16
#define VHT_CAP_INFO_NUMBER_OF_SOUNDING_DIMENSIONS             BITS(16, 18)
#define VHT_CAP_INFO_NUMBER_OF_SOUNDING_DIMENSIONS_2_SUPPORTED BIT(16)
#define VHT_CAP_INFO_NUMBER_OF_SOUNDING_DIMENSIONS_3_SUPPORTED BIT(17)
#define VHT_CAP_INFO_NUMBER_OF_SOUNDING_DIMENSIONS_4_SUPPORTED BITS(16, 17)

#define	VHT_CAP_INFO_MU_BEAMFOMER_CAPABLE       BIT(19)
#define VHT_CAP_INFO_MU_BEAMFOMEE_CAPABLE       BIT(20)
#define VHT_CAP_INFO_VHT_TXOP_PS                BIT(21)
#define VHT_CAP_INFO_HTC_VHT_CAPABLE            BIT(22)

#define VHT_CAP_INFO_MAX_AMPDU_LENGTH_OFFSET                  23
#define VHT_CAP_INFO_MAX_AMPDU_LENGTH_MASK                    BITS(23, 25)

#define VHT_CAP_INFO_VHT_LINK_ADAPTATION_CAPABLE_NOFEEDBACK   0
#define VHT_CAP_INFO_VHT_LINK_ADAPTATION_CAPABLE_UNSOLICITED  BITS(27)
#define VHT_CAP_INFO_VHT_LINK_ADAPTATION_CAPABLE_BOTH         BITS(26, 27)

#define VHT_CAP_INFO_RX_ANTENNA_PATTERN_CONSISTENCY			BIT(28)
#define VHT_CAP_INFO_TX_ANTENNA_PATTERN_CONSISTENCY			BIT(29)

/* refer to Table 9-272 Extended NSS BW Support subfield */
#define VHT_CAP_INFO_EXT_NSS_BW_SUPPORT			0
#define VHT_CAP_INFO_EXT_NSS_BW_SUPPORT_OFFSET		30
#define VHT_CAP_INFO_EXT_NSS_BW_SUPPORT_MASK		BITS(30, 31)

#define VHT_CAP_INFO_MCS_MAP_MCS7           0
#define VHT_CAP_INFO_MCS_MAP_MCS8           BIT(0)
#define VHT_CAP_INFO_MCS_MAP_MCS9           BIT(1)
#define VHT_CAP_INFO_MCS_NOT_SUPPORTED      BITS(0, 1)

#define VHT_CAP_INFO_MCS_1SS_OFFSET         0
#define VHT_CAP_INFO_MCS_2SS_OFFSET         2
#define VHT_CAP_INFO_MCS_3SS_OFFSET         4
#define VHT_CAP_INFO_MCS_4SS_OFFSET         6
#define VHT_CAP_INFO_MCS_5SS_OFFSET         8
#define VHT_CAP_INFO_MCS_6SS_OFFSET         10
#define VHT_CAP_INFO_MCS_7SS_OFFSET         12
#define VHT_CAP_INFO_MCS_8SS_OFFSET         14

#define VHT_CAP_INFO_MCS_1SS_MASK           BITS(0, 1)
#define VHT_CAP_INFO_MCS_2SS_MASK           BITS(2, 3)
#define VHT_CAP_INFO_MCS_3SS_MASK           BITS(4, 5)
#define VHT_CAP_INFO_MCS_4SS_MASK           BITS(6, 7)
#define VHT_CAP_INFO_MCS_5SS_MASK           BITS(8, 9)
#define VHT_CAP_INFO_MCS_6SS_MASK           BITS(10, 11)
#define VHT_CAP_INFO_MCS_7SS_MASK           BITS(12, 13)
#define VHT_CAP_INFO_MCS_8SS_MASK           BITS(14, 15)

#define VHT_OP_CHANNEL_WIDTH_20_40          0
#define VHT_OP_CHANNEL_WIDTH_80             1
#define VHT_OP_CHANNEL_WIDTH_160            2
#define VHT_OP_CHANNEL_WIDTH_80P80          3
#define VHT_OP_CHANNEL_WIDTH_320_1          4
#define VHT_OP_CHANNEL_WIDTH_320_2          5
#define VHT_MAX_BW_INVALID                  255

/*8.4.1.50 Operating Mode Field*/
#define VHT_OP_MODE_CHANNEL_WIDTH                   BITS(0, 1)
#define VHT_OP_MODE_CHANNEL_WIDTH_80P80_160         BIT(2)
#define VHT_OP_MODE_RX_NSS                          BITS(4, 6)
#define VHT_OP_MODE_RX_NSS_TYPE                     BIT(7)

#define VHT_OP_MODE_NSS_1	0x00
#define VHT_OP_MODE_NSS_2	0x01

#define VHT_OP_MODE_CHANNEL_WIDTH_OFFSET                   0
#define VHT_OP_MODE_RX_NSS_OFFSET                   4
#define VHT_OP_MODE_RX_NSS_TYPE_OFFSET              7

/* 9.4.2.158, Table 9-274 - VHT Operation Information subfields */
#define VHT_OP_MODE_CHANNEL_WIDTH_20                0
#define VHT_OP_MODE_CHANNEL_WIDTH_40                1
#define VHT_OP_MODE_CHANNEL_WIDTH_80                2
#define VHT_OP_MODE_CHANNEL_WIDTH_160_80P80         3

/* 8.4.1.22 SM Power Control field*/
#define HT_SM_POWER_SAVE_CONTROL_ENABLED            BIT(0)
/* 0:static, 1:dynamic */
#define HT_SM_POWER_SAVE_CONTROL_SM_MODE            BIT(1)
#define HT_SM_POWER_SAVE_CONTROL_SM_MODE_OFFSET     1

/* 8.4.1.21 Channel Width field */
#define HT_NOTIFY_CHANNEL_WIDTH_20				0
#define HT_NOTIFY_CHANNEL_WIDTH_ANY_SUPPORT_CAHNNAEL_WIDTH	1

/* 7.3.2.56.3 A-MPDU Parameters field */
#define AMPDU_PARAM_MAX_AMPDU_LEN_EXP               BITS(0, 1)
#define AMPDU_PARAM_MIN_START_SPACING               BITS(2, 4)

#define AMPDU_PARAM_MAX_AMPDU_LEN_8K                0
#define AMPDU_PARAM_MAX_AMPDU_LEN_16K               BIT(0)
#define AMPDU_PARAM_MAX_AMPDU_LEN_32K               BIT(1)
#define AMPDU_PARAM_MAX_AMPDU_LEN_64K               BITS(0, 1)
#define AMPDU_PARAM_MAX_AMPDU_LEN_128K              BIT(2)
#define AMPDU_PARAM_MAX_AMPDU_LEN_256K              (BIT(2) | BIT(0))
#define AMPDU_PARAM_MAX_AMPDU_LEN_512K              BITS(1, 2)
#define AMPDU_PARAM_MAX_AMPDU_LEN_1024K             BITS(0, 2)

#define AMPDU_PARAM_MSS_NO_RESTRICIT                0
#define AMPDU_PARAM_MSS_1_4_US                      BIT(2)
#define AMPDU_PARAM_MSS_1_2_US                      BIT(3)
#define AMPDU_PARAM_MSS_1_US                        BITS(2, 3)
#define AMPDU_PARAM_MSS_2_US                        BIT(4)
#define AMPDU_PARAM_MSS_4_US                        (BIT(4) | BIT(2))
#define AMPDU_PARAM_MSS_8_US                        (BIT(4) | BIT(3))
#define AMPDU_PARAM_MSS_16_US                       BITS(2, 4)

/* 7.3.2.56.4 Supported MCS Set field (TX rate: octects 12~15) */
#define SUP_MCS_TX_SET_DEFINED                      BIT(0)
#define SUP_MCS_TX_RX_SET_NOT_EQUAL                 BIT(1)
#define SUP_MCS_TX_MAX_NUM_SS                       BITS(2, 3)
#define SUP_MCS_TX_UNEQUAL_MODULATION               BIT(4)

#define SUP_MCS_TX_MAX_NUM_1_SS                     0
#define SUP_MCS_TX_MAX_NUM_2_SS                     BIT(2)
#define SUP_MCS_TX_MAX_NUM_3_SS                     BIT(3)
#define SUP_MCS_TX_MAX_NUM_4_SS                     BITS(2, 3)

#define SUP_MCS_RX_BITMASK_OCTET_NUM                10
#define SUP_MCS_RX_DEFAULT_HIGHEST_RATE             0	/* Not specify */

/* 7.3.2.56.5 HT Extended Capabilities field */
#define HT_EXT_CAP_PCO                              BIT(0)
#define HT_EXT_CAP_PCO_TRANSITION_TIME              BITS(1, 2)
#define HT_EXT_CAP_MCS_FEEDBACK                     BITS(8, 9)
#define HT_EXT_CAP_HTC_SUPPORT                      BIT(10)
#define HT_EXT_CAP_RD_RESPONDER                     BIT(11)

#define HT_EXT_CAP_PCO_TRANS_TIME_NONE              0
#define HT_EXT_CAP_PCO_TRANS_TIME_400US             BIT(1)
#define HT_EXT_CAP_PCO_TRANS_TIME_1_5MS             BIT(2)
#define HT_EXT_CAP_PCO_TRANS_TIME_5MS               BITS(1, 2)

#define HT_EXT_CAP_MCS_FEEDBACK_NO_FB               0
#define HT_EXT_CAP_MCS_FEEDBACK_UNSOLICITED         BIT(9)
#define HT_EXT_CAP_MCS_FEEDBACK_BOTH                BITS(8, 9)

/* 7.3.2.56.6 Transmit Beamforming Capabilities field */
#define TXBF_IMPLICIT_RX_CAPABLE                    BIT(0)
#define TXBF_RX_STAGGERED_SOUNDING_CAPABLE          BIT(1)
#define TXBF_TX_STAGGERED_SOUNDING_CAPABLE          BIT(2)
#define TXBF_RX_NDP_CAPABLE_OFFSET                  3
#define TXBF_RX_NDP_CAPABLE                         BIT(3)
#define TXBF_TX_NDP_CAPABLE                         BIT(4)
#define TXBF_IMPLICIT_TX_CAPABLE                    BIT(5)
#define TXBF_CALIBRATION_CAPABLE                    BITS(6, 7)
#define TXBF_EXPLICIT_CSI_TX_CAPABLE                BIT(8)
#define TXBF_EXPLICIT_NONCOMPRESSED_TX_CAPABLE      BIT(9)
#define TXBF_EXPLICIT_COMPRESSED_TX_CAPAB           BIT(10)
#define TXBF_EXPLICIT_CSI_FEEDBACK_CAPABLE          BITS(11, 12)
#define TXBF_EXPLICIT_NONCOMPRESSED_FEEDBACK_CAPABLE BITS(13, 14)

#define TXBF_EXPLICIT_COMPRESSED_FEEDBACK_CAPABLE_OFFSET 15
#define TXBF_EXPLICIT_COMPRESSED_FEEDBACK_CAPABLE   BITS(15, 16)
#define TXBF_EXPLICIT_COMPRESSED_FEEDBACK_IMMEDIATE_CAPABLE BIT(16)

#define TXBF_MINIMAL_GROUPING_CAPABLE               BITS(17, 18)
#define TXBF_MINIMAL_GROUPING_1_2_3_CAPABLE         BITS(17, 18)

#define TXBF_CSI_BFER_ANTENNANUM_SUPPORTED          BITS(19, 20)
#define TXBF_NONCOMPRESSED_TX_ANTENNANUM_SUPPORTED  BITS(21, 22)

#define TXBF_COMPRESSED_TX_ANTENNANUM_SUPPORTED_OFFSET  23
#define TXBF_COMPRESSED_TX_ANTENNANUM_SUPPORTED     BITS(23, 24)
#define TXBF_COMPRESSED_TX_ANTENNANUM_4_SUPPORTED   BITS(23, 24)

#define TXBF_CSI_MAX_ROWS_BFER_SUPPORTED            BITS(25, 26)

#define TXBF_CHANNEL_ESTIMATION_CAPABILITY          BITS(27, 28)
#define TXBF_CHANNEL_ESTIMATION_4STS_CAPABILITY     BITS(27, 28)

/* 7.3.2.56.7 Antenna Selection Capability field */
#define ASEL_CAP_CAPABLE                            BIT(0)
#define ASEL_CAP_CSI_FB_BY_TX_ASEL_CAPABLE          BIT(1)
#define ASEL_CAP_ANT_INDICES_FB_BY_TX_ASEL_CAPABLE  BIT(2)
#define ASEL_CAP_EXPLICIT_CSI_FB_CAPABLE            BIT(3)
#define ASEL_CAP_ANT_INDICES_CAPABLE                BIT(4)
#define ASEL_CAP_RX_ASEL_CAPABLE                    BIT(5)
#define ASEL_CAP_TX_SOUNDING_CAPABLE                BIT(6)

/* 7.3.2.57 HT Operation element */
/* sizeof(IE_HT_OP_T)-2 */
#define ELEM_MAX_LEN_HT_OP                          (24 - ELEM_HDR_LEN)

#define HT_OP_INFO1_SCO                             BITS(0, 1)
#define HT_OP_INFO1_STA_CHNL_WIDTH                  BIT(2)
#define HT_OP_INFO1_RIFS_MODE                       BIT(3)

#define HT_OP_INFO1_STA_CHNL_WIDTH_OFFSET		2

#define HT_OP_INFO2_HT_PROTECTION                   BITS(0, 1)
#define HT_OP_INFO2_NON_GF_HT_STA_PRESENT           BIT(2)
#define HT_OP_INFO2_OBSS_NON_HT_STA_PRESENT         BIT(4)
#define HT_OP_INFO2_CH_CENTER_FREQ_SEG2_OFFSET      5
#define HT_OP_INFO2_CH_CENTER_FREQ_SEG2             BITS(5, 12)

#define HT_OP_INFO3_DUAL_BEACON                     BIT(6)
#define HT_OP_INFO3_DUAL_CTS_PROTECTION             BIT(7)
#define HT_OP_INFO3_STBC_BEACON                     BIT(8)
#define HT_OP_INFO3_LSIG_TXOP_FULL_SUPPORT          BIT(9)
#define HT_OP_INFO3_PCO_ACTIVE                      BIT(10)
#define HT_OP_INFO3_PCO_PHASE                       BIT(11)

#define HT_GET_OP_INFO2_CH_CENTER_FREQ_SEG2(_aucHtOp) \
	((_aucHtOp & HT_OP_INFO2_CH_CENTER_FREQ_SEG2) \
	>> HT_OP_INFO2_CH_CENTER_FREQ_SEG2_OFFSET)

/* 7.3.2.59 OBSS Scan Parameter element */
#define ELEM_MAX_LEN_OBSS_SCAN                      (16 - ELEM_HDR_LEN)

/* 7.3.2.60 20/40 BSS Coexistence element */
#define ELEM_MAX_LEN_20_40_BSS_COEXIST              (3 - ELEM_HDR_LEN)

#define BSS_COEXIST_INFO_REQ                        BIT(0)
#define BSS_COEXIST_40M_INTOLERANT                  BIT(1)
#define BSS_COEXIST_20M_REQ                         BIT(2)
#define BSS_COEXIST_OBSS_SCAN_EXEMPTION_REQ         BIT(3)
#define BSS_COEXIST_OBSS_SCAN_EXEMPTION_GRANT       BIT(4)

/* 802.11u 7.3.2.92 Interworking IE */
#define ELEM_MAX_LEN_INTERWORKING                   (11 - ELEM_HDR_LEN)

/* 802.11u 7.3.2.93 Advertisement Protocol IE */
#define ELEM_MAX_LEN_ADV_PROTOCOL                   (4 - ELEM_HDR_LEN)

/* 802.11u 7.3.2.96 Roaming Consortium IE */
#define ELEM_MAX_LEN_ROAMING_CONSORTIUM             (19 - ELEM_HDR_LEN)

#define IW_IE_LENGTH_ANO                            1
#define IW_IE_LENGTH_ANO_VENUE                      3
#define IW_IE_LENGTH_ANO_HESSID                     7
#define IW_IE_LENGTH_ANO_VENUE_HESSID               9

#if CFG_SUPPORT_PASSPOINT
/* HOTSPOT 2.0 Indication IE*/
#define ELEM_MAX_LEN_HS20_INDICATION                5
#define ELEM_MIN_LEN_HS20_INDICATION                4

/* Hotspot Configuration*/
/* Downstream Group-Addressed Forwarding */
#define ELEM_HS_CONFIG_DGAF_DISABLED_MASK           BIT(0)
#endif /* CFG_SUPPORT_PASSPOINT */

/* MTK Vendor Specific OUI */
#define ELEM_MIN_LEN_MTK_OUI			    7
#define VENDOR_OUI_MTK                              { 0x00, 0x0C, 0xE7 }
#define MTK_SYNERGY_CAP_SUPPORT_TLV                 BIT(0)
#define MTK_SYNERGY_CAP_SUPPORT_24G_MCS89           BIT(3)
#define MTK_SYNERGY_CAP_SUPPORT_24G_MCS89_PROBING   BIT(4)
#define MTK_SYNERGY_CAP0 \
	(MTK_SYNERGY_CAP_SUPPORT_24G_MCS89)
#define MTK_SYNERGY_CAP1                            0x0
#define MTK_SYNERGY_CAP2                            0x0
#define MTK_SYNERGY_CAP3                            0x0

#define MTK_SYNERGY_CAP_SUPPORT_TWT_HOTSPOT_AC	    BIT(1)
#define MTK_SYNERGY_CAP_SUPPORT_GC_CSA		    BIT(1)

#define MTK_OUI_ID_MLR				    1
#define MTK_OUI_ID_PRE_WIFI7			    2
#define MTK_OUI_ID_ICI				    3
#define MTK_OUI_ID_CHIP_CAP			    4

enum ENUM_MTK_OUI_CHIP_CAP {
	CHIP_CAP_ICV_V1 = BIT(0),
	CHIP_CAP_ICV_V2 = BIT(1),
	CHIP_CAP_ICV_V1_1 = BIT(2),
};

#ifdef CFG_SUPPORT_GLOBAL_AAD_NONCE
#define MTK_OUI_CHIP_CAP			    (CHIP_CAP_ICV_V1_1)
#else
#define MTK_OUI_CHIP_CAP			    (CHIP_CAP_ICV_V2)
#endif

#if CFG_SUPPORT_RXSMM_ALLOWLIST
#define VENDOR_OUI_RXSMM_OUI_IE_NUM                 3
#endif

/* 3 Management frame body components (III): 7.4 Action frame format details. */
/* 7.4.1 Spectrum Measurement Action frame details */
/* Spectrum measurement request */
#define ACTION_MEASUREMENT_REQ                      0
/* Spectrum measurement report */
#define ACTION_MEASUREMENT_REPORT                   1
/* TPC request */
#define ACTION_TPC_REQ                              2
/* TPC report */
#define ACTION_TPC_REPORT                           3
/* Channel Switch Announcement */
#define ACTION_CHNL_SWITCH                          4

#define ACTION_SM_TPC_REQ_LEN                       5
#define ACTION_SM_TPC_REPORT_LEN                    7
#define ACTION_SM_MEASURE_REQ_LEN                   19
#define ACTION_SM_MEASURE_REPORT_LEN                8
#define ACTION_SM_BASIC_REPORT_LEN                  12
#define ACTION_SM_CCA_REPORT_LEN                    12
#define ACTION_SM_PRI_REPORT_LEN                    19
/* Negative value ((dBm) */
#define MIN_RCV_PWR                                 100

/* 7.4.2 QoS Action frame details */
#define ACTION_ADDTS_REQ                            0	/* ADDTS request */
#define ACTION_ADDTS_RSP                            1	/* ADDTS response */
#define ACTION_DELTS                                2	/* DELTS */
#define ACTION_SCHEDULE                             3	/* Schedule */
#define ACTION_QOS_MAP_CONFIGURE                    4	/*Qos Map Configure*/

/* WMM TSPEC IE: 63 */
#define ACTION_ADDTS_REQ_FRAME_LEN                  (24+3+63)
/* WMM Status Code: 1; WMM TSPEC IE: 63 */
#define ACTION_ADDTS_RSP_FRAME_LEN                  (24 + 4 + 63)
/*category + action + WMM TSinfo:3 + reason:2*/
#define ACTION_DELTS_FRAME_LEN                      (24 + 7)

/* 7.4.3 DLS Action frame details */
#define ACTION_DLS_REQ                              0	/* DLS request */
#define ACTION_DLS_RSP                              1	/* DLS response */
#define ACTION_DLS_TEARDOWN                         2	/* DLS teardown */

/* 7.4.4 Block ack  Action frame details */
#define ACTION_ADDBA_REQ                            0	/* ADDBA request */
#define ACTION_ADDBA_RSP                            1	/* ADDBA response */
#define ACTION_DELBA                                2	/* DELBA */

#define ACTION_ADDBA_REQ_FRAME_LEN                  (24+9)
#define ACTION_ADDBA_RSP_FRAME_LEN                  (24+9)

#define ACTION_DELBA_INITIATOR_MASK                 BIT(11)
#define ACTION_DELBA_TID_MASK                       BITS(12, 15)
#define ACTION_DELBA_TID_OFFSET                     12
#define ACTION_DELBA_FRAME_LEN                      (24+6)

/* 7.4.6 Radio Measurement Action frame details */
/* Radio measurement request */
#define ACTION_RM_REQ                               0
/* Radio measurement report */
#define ACTION_RM_REPORT                            1
/* Link measurement request */
#define ACTION_LM_REQ                               2
/* Link measurement report */
#define ACTION_LM_REPORT                            3
/* Neighbor report request */
#define ACTION_NEIGHBOR_REPORT_REQ                  4
/* Neighbor report response */
#define ACTION_NEIGHBOR_REPORT_RSP                  5

/* 7.4.7 Public Action frame details */
/* 20/40 BSS coexistence */
#define ACTION_PUBLIC_20_40_COEXIST                 0
/* Extended channel switch announcment */
#define ACTION_PUBLIC_EX_CH_SW_ANNOUNCEMENT         4
/* Vendor specific */
#define ACTION_PUBLIC_VENDOR_SPECIFIC               9

#if CFG_SUPPORT_802_11W
/* SA Query Action frame (IEEE 802.11w/D8.0, 7.4.9) */
#define ACTION_SA_QUERY_REQUEST                     0
#define ACTION_SA_QUERY_RESPONSE                    1

#define ACTION_SA_QUERY_TR_ID_LEN                   2

/* Timeout Interval Type */
#define ACTION_SA_TIMEOUT_REASSOC_DEADLINE          1
#define ACTION_SA_TIMEOUT_KEY_LIFETIME              2
#define ACTION_SA_TIMEOUT_ASSOC_COMEBACK            3
#endif

/* 7.4.10.1 HT action frame details */
/* Notify Channel Width */
#define ACTION_HT_NOTIFY_CHANNEL_WIDTH              0
/* SM Power Save */
#define ACTION_HT_SM_POWER_SAVE                     1
/* PSMP */
#define ACTION_HT_PSMP                              2
/* Set PCO Phase */
#define ACTION_HT_SET_PCO_PHASE                     3
/* CSI */
#define ACTION_HT_CSI                               4
/* Non-compressed Beamforming */
#define ACTION_HT_NON_COMPRESSED_BEAMFORM           5
/* Compressed Beamforming */
#define ACTION_HT_COMPRESSED_BEAMFORM               6
/* Antenna Selection Indices Feedback */
#define ACTION_HT_ANT_SEL_INDICES_FB                7

#define ACTION_WNM_NOTIFICATION_REQUEST			26
/* 802.11v Wireless Network Management */
#define ACTION_WNM_TIMING_MEASUREMENT_REQUEST       27

#define ACTION_FT_REQUEST			    1
#define ACTION_FT_RESPONSE			    2

#define ACTION_UNPROTECTED_WNM_TIM                  0
#define ACTION_UNPROTECTED_WNM_TIMING_MEASUREMENT   1
#define ACTION_WNM_BSS_TRANSITION_MANAGEMENT_QUERY  6
#define ACTION_WNM_BSS_TRANSITION_MANAGEMENT_REQ    7
#define ACTION_WNM_BSS_TRANSITION_MANAGEMENT_RSP    8
#define ACTION_UNPROTECTED_WNM_TIMING_MEAS_LEN      12

/* 8.5.23.1 VHT Action */
#define ACTION_VHT_COMPRESSED_BFEAMFORMING          0
#define ACTION_GROUP_ID_MANAGEMENT                  1
#define ACTION_OPERATING_MODE_NOTIFICATION          2

#if (CFG_SUPPORT_TWT == 1)
/* S1G Action */
#define ACTION_S1G_TWT_SETUP                        6
#define ACTION_S1G_TWT_TEARDOWN                     7
#define ACTION_S1G_TWT_INFORMATION                  11
#endif

/* 9.6.18 Robust AV streaming Robust Action */
#define ACTION_MSCS_REQ                             4
#define ACTION_MSCS_RSP                             5
#define ELEM_ID_EXT_MSCS_DESC                       88
#define ELEM_ID_EXT_TCLAS_MASK                      89
#define MSCS_DESC_REQ_TYPE_ADD                      0
#define MSCS_DESC_REQ_TYPE_REMOVE                   1
#define MSCS_DESC_MANDATORY_LEN                     8

/* 3  --------------- WFA  frame body fields --------------- */
#define VENDOR_OUI_WFA                              { 0x00, 0x50, 0xF2 }
#define VENDOR_OUI_WFA_SPECIFIC                     { 0x50, 0x6F, 0x9A }
#define VENDOR_OUI_TYPE_WPA                         1
#define VENDOR_OUI_TYPE_WMM                         2
#define VENDOR_OUI_TYPE_WPS                         4
#define VENDOR_OUI_TYPE_P2P                         9
#define VENDOR_OUI_TYPE_WFD                         10
#define VENDOR_OUI_TYPE_MBO                         22
#define VENDOR_OUI_TYPE_OWE                         28

#define VENDOR_IE_TYPE_MBO                          0x506f9a16

/* Epigram IE */
#define VENDOR_IE_EPIGRAM_OUI                      0x00904c
#define VENDOR_IE_EPIGRAM_VHTTYPE1                  0x0400
#define VENDOR_IE_EPIGRAM_VHTTYPE2                  0x0408
#define VENDOR_IE_EPIGRAM_VHTTYPE3                  0x0418

/* Cisco IE */
#define VENDOR_IE_CISCO_OUI                        0x004096
#define VENDOR_IE_CISCO_TYPE                       0x2C
#define VENDOR_IE_CISCO_TYPE_CCX                   0x03

/* Customer Vendor Specific IE*/
#define VENDOR_IE_SAMSUNG_OUI                      0x0000F0

#if CFG_SUPPORT_PASSPOINT
#define VENDOR_OUI_TYPE_HS20                        16
#endif /* CFG_SUPPORT_PASSPOINT */

/* Length of OUI and Type */
#define VENDOR_OUI_TYPE_LEN                         4

/* VERSION(2 octets for WPA) / SUBTYPE(1 octet)-VERSION(1 octet)
 * fields for WMM in WFA IE
 */
/* Little Endian Format */
#define VERSION_WPA                             0x0001
#define VENDOR_OUI_SUBTYPE_VERSION_WMM_INFO     0x0100
#define VENDOR_OUI_SUBTYPE_VERSION_WMM_PARAM    0x0101

/* SUBTYPE(1 octet) for WMM */
/* WMM Spec version 1.1 */
#define VENDOR_OUI_SUBTYPE_WMM_INFO             0x00
#define VENDOR_OUI_SUBTYPE_WMM_PARAM            0x01
#define VENDOR_OUI_SUBTYPE_WMM_TSPEC            0x02

/* VERSION(1 octet) for WMM */
/* WMM Spec version 1.1 */
#define VERSION_WMM                             0x01

/* WMM-2.1.6 QoS Control Field */
#define WMM_QC_UP_MASK                          BITS(0, 2)
#define WMM_QC_EOSP                             BIT(4)
#define WMM_QC_ACK_POLICY_MASK                  BITS(5, 6)
#define WMM_QC_ACK_POLICY_OFFSET                5
#define WMM_QC_ACK_POLICY_ACKNOWLEDGE           0
#define WMM_QC_ACK_POLICY_NOT_ACKNOWLEDGE	\
	(1 << WMM_QC_ACK_POLICY_OFFSET)

/* WMM-2.2.1 WMM Information Element */
#define ELEM_MIN_LEN_WFA_OUI_TYPE_SUBTYPE       6

/* 3 Control frame body */
/* 7.2.1.7 BlockAckReq */
#define CTRL_BAR_BAR_CONTROL_OFFSET             16
#define CTRL_BAR_BAR_CONTROL_TID_OFFSET         12
#define CTRL_BAR_BAR_INFORMATION_OFFSET         18
#define CTRL_BAR_BAR_INFORMATION_SSN_OFFSET     4

/* 802.11-2012, 8.5.7 Radio Measurement action fields, table 8-206 */
#define RM_ACTION_RM_REQUEST                        0
#define RM_ACTION_RM_REPORT                         1
#define RM_ACTION_LM_REQUEST                        2
#define RM_ACTION_LM_REPORT                         3
#define RM_ACTION_NEIGHBOR_REQUEST                  4
#define RM_ACTION_REIGHBOR_RESPONSE                 5

#if (CFG_SUPPORT_TWT == 1)
/* TWT element */
#define TWT_CTRL_NDP_PAGING_INDICATOR               BIT(0)
#define TWT_CTRL_RESPONDER_PM_MODE                  BIT(1)
#define TWT_CTRL_BROADCAST                          BIT(2)
#define TWT_CTRL_WAKE_TBTT_NEGOTIATION              BIT(3)

#define TWT_REQ_TYPE_TWT_REQUEST                    BIT(0)
#define TWT_REQ_TYPE_TWT_SETUP_COMMAND              BITS(1, 3)
#define TWT_REQ_TYPE_TRIGGER                        BIT(4)
#define TWT_REQ_TYPE_IMPLICIT_LAST_BCAST_PARAM      BIT(5)
#define TWT_REQ_TYPE_FLOWTYPE                       BIT(6)
#define TWT_REQ_TYPE_TWT_FLOW_IDENTIFIER            BITS(7, 9)
#define TWT_REQ_TYPE_TWT_WAKE_INTVAL_EXP            BITS(10, 14)
#define TWT_REQ_TYPE_TWT_PROTECTION                 BIT(15)

#define TWT_REQ_TYPE_TWT_REQUEST_OFFSET                0
#define TWT_REQ_TYPE_TWT_SETUP_COMMAND_OFFSET          1
#define TWT_REQ_TYPE_TRIGGER_OFFSET                    4
#define TWT_REQ_TYPE_IMPLICIT_LAST_BCAST_PARAM_OFFSET  5
#define TWT_REQ_TYPE_FLOWTYPE_OFFSET                   6
#define TWT_REQ_TYPE_TWT_FLOW_IDENTIFIER_OFFSET        7
#define TWT_REQ_TYPE_TWT_WAKE_INTVAL_EXP_OFFSET        10
#define TWT_REQ_TYPE_TWT_PROTECTION_OFFSET             15

#define TWT_SETUP_CMD_ID_REQUEST                       0
#define TWT_SETUP_CMD_ID_SUGGEST                       1
#define TWT_SETUP_CMD_ID_DEMAND                        2
#define TWT_SETUP_CMD_ID_GROUPING                      3
#define TWT_SETUP_CMD_ID_ACCEPT                        4
#define TWT_SETUP_CMD_ID_ALTERNATE                     5
#define TWT_SETUP_CMD_ID_DICTATE                       6
#define TWT_SETUP_CMD_ID_REJECT                        7

/* TWT Flow Field in teardown frame */
#define TWT_TEARDOWN_FLOW_ID                        BITS(0, 2)
#define TWT_TEARDOWN_NEGO                           BITS(5, 6)
#define TWT_TEARDOWN_NEGO_OFFSET                    5
#define TWT_TEARDOWN_ALL                            BIT(7)
#define TWT_TEARDOWN_ALL_OFFSET                     7


/* TWT Information Field */
#define TWT_INFO_FLOW_ID                            BITS(0, 2)
#define TWT_INFO_RESP_REQUESTED                     BIT(3)
#define TWT_INFO_NEXT_TWT_REQ                       BIT(4)
#define TWT_INFO_NEXT_TWT_SIZE                      BITS(5, 6)
#define TWT_INFO_BCAST_RESCHED                      BIT(7)

#define TWT_INFO_FLOW_ID_OFFSET                     0
#define TWT_INFO_RESP_REQUESTED_OFFSET              3
#define TWT_INFO_NEXT_TWT_REQ_OFFSET                4
#define TWT_INFO_NEXT_TWT_SIZE_OFFSET               5
#define TWT_INFO_BCAST_RESCHED_OFFSET               7

#define NEXT_TWT_SUBFIELD_ZERO_BIT                  0
#define NEXT_TWT_SUBFIELD_32_BITS                   1
#define NEXT_TWT_SUBFIELD_48_BITS                   2
#define NEXT_TWT_SUBFIELD_64_BITS                   3

#define TWT_HOTSPOT_NO_MORE_FLOW_ID                 0xFF

#if (CFG_SUPPORT_BTWT == 1)
#define BTWT_REQ_TYPE_LAST_BCAST_PARAM              BIT(5)
#define BTWT_REQ_TYPE_LAST_BCAST_PARAM_OFFSET       5
#define BTWT_REQ_TYPE_RECOMMENDATION_OFFSET         7
#define BTWT_REQ_TYPE_RECOMMENDATION                BITS(7, 9)
#define BTWT_REQ_TYPE_RESERVED_OFFSET               15
#define BTWT_REQ_TYPE_RESERVED                      BIT(15)
#define BTWT_CTRL_NEGOTIATION_OFFSET                2
#define BTWT_CTRL_NEGOTIATION                       BITS(2, 3)
#define BTWT_INFO_BROADCAST_OFFSET                  3
#define BTWT_INFO_BROADCAST                         BITS(3, 7)
#define BTWT_INFO_PERSISTENCE_OFFSET                8
#define BTWT_INFO_PERSISTENCE                       BITS(8, 15)
#endif

#if (CFG_SUPPORT_RTWT == 1)
#define BTWT_INFO_RTWT_TRAFFIC_INFO_PRESENT_OFFSET  0
#define BTWT_INFO_RTWT_TRAFFIC_INFO_PRESENT         BIT(0)
#define BTWT_INFO_RTWT_SCHEDULE_INFO_OFFSET         1
#define BTWT_INFO_RTWT_SCHEDULE_INFO                BITS(1, 2)

#define RTWT_TRAFFIC_INFO_DL_TID_BITMAP_VALID_OFFSET 0
#define RTWT_TRAFFIC_INFO_DL_TID_BITMAP_VALID        BIT(0)
#define RTWT_TRAFFIC_INFO_UL_TID_BITMAP_VALID_OFFSET 1
#define RTWT_TRAFFIC_INFO_UL_TID_BITMAP_VALID        BIT(1)
#define RTWT_TRAFFIC_INFO_DL_TID_BITMAP_OFFSET       0
#define RTWT_TRAFFIC_INFO_DL_TID_BITMAP              BITS(0, 7)
#define RTWT_TRAFFIC_INFO_UL_TID_BITMAP_OFFSET       0
#define RTWT_TRAFFIC_INFO_UL_TID_BITMAP              BITS(0, 7)

#define RTWT_TRAFFIC_INFO_BYTE_LENGTH                  3
#define RTWT_TRAFFIC_INFO_DL_UL_BMP_VALID_BYTE_LENGTH  1
#define RTWT_TRAFFIC_INFO_DL_TID_BMP_BYTE_LENGTH       1
#define RTWT_TRAFFIC_INFO_UL_TID_BMP_BYTE_LENGTH       1
#endif

#endif

/* 9.4.2.46 Multiple BSSID element */
/* Nontransmitted BSSID Profile */
#define NON_TX_BSSID_PROFILE                        0

/* 9.4.2.170 Reduced Neighbor Report element */
#define TBTT_INFO_HDR_FIELD_TYPE                    BITS(0, 1)
#define TBTT_INFO_HDR_FILTERED_NIEGHBOR_AP          BIT(2)
#define TBTT_INFO_HDR_COLOCATED_AP                  BIT(3)
#define TBTT_INFO_HDR_COUNT                         BITS(4, 7)
#define TBTT_INFO_HDR_COUNT_OFFSET                  4
#define TBTT_INFO_HDR_LENGTH                        BITS(8, 15)
#define TBTT_INFO_HDR_LENGTH_OFFSET                 8

/*D5.0 9.4.2.248 HE Operation element */
#define HE_OP_CHANNEL_WIDTH_20				0
#define HE_OP_CHANNEL_WIDTH_40				1
#define HE_OP_CHANNEL_WIDTH_80				2
#define HE_OP_CHANNEL_WIDTH_80P80_160			3

#define TBTT_INFO_BSS_PARAM_SAME_SSID               BIT(1)
#define TBTT_INFO_BSS_PARAM_CO_LOCATED_AP           BIT(6)

/* 9.4.2.260 Short SSID List element */
#define ELEM_EXT_ID_SHORT_SSID_LIST                 58

#define MAX_LEN_OF_MLIE					(255)
#define MAX_LEN_OF_FRAGMENT				(255)

#define MAX_NUM_MLO_LINKS				15

/*802.11be D3.0 Figure 9-709c - MLD Parameters subfield format*/
#define MLD_PARAM_MLD_ID_MASK				BITS(0, 7)
#define MLD_PARAM_LINK_ID_MASK				BITS(8, 11)
#define MLD_PARAM_LINK_ID_SHIFT				8
#define MLD_PARAM_BSS_PARAM_CHANGE_COUNT_MASK		BITS(12, 19)
#define MLD_PARAM_BSS_PARAM_CHANGE_COUNT_SHIFT		12
#define MLD_PARAM_ALL_UPDATES_INCLUDED			BIT(20)
#define MLD_PARAM_DISABLED_LINK				BIT(21)

/* 9.4.2.312 Multi-Link element */
#define ML_CTRL_TYPE_MASK				BITS(0, 2)
#define ML_CTRL_TYPE_SHIFT				0
#define ML_CTRL_TYPE_BASIC				0
#define ML_CTRL_TYPE_PROBE_REQ				1
#define ML_CTRL_TYPE_RECONFIG				2
#define ML_CTRL_TYPE_TDLS				3
#define ML_CTRL_TYPE_PRIORITY_ACCESS			4
#define ML_CTRL_PRE_BMP_MASK				BITS(4, 15)
#define ML_CTRL_PRE_BMP_SHIFT				4
/* Macro to get ML control type */
#define ML_GET_CTRL_TYPE(_prMlInfoIe) \
	(_prMlInfoIe->u2Ctrl & ML_CTRL_TYPE_MASK)

/* 9.4.2.312.2 Basic Multi-Link element */
#define ML_CTRL_LINK_ID_INFO_PRESENT			BIT(0)
#define ML_CTRL_BSS_PARA_CHANGE_COUNT_PRESENT		BIT(1)
#define ML_CTRL_MEDIUM_SYN_DELAY_INFO_PRESENT		BIT(2)
#define ML_CTRL_EML_CAPA_PRESENT			BIT(3)
#define ML_CTRL_MLD_CAPA_PRESENT			BIT(4)
#define ML_CTRL_MLD_ID_PRESENT				BIT(5)
#define ML_CTRL_EXT_MLD_CAP_OP_PRESENT			BIT(6)

/* EML Capability */
#define ML_CTRL_EML_CAPA_EMLSR_SUPPORT_MASK BIT(0)

/* 9.4.2.314 TID-To-LINK Mapping element */
#define MAX_NUM_T2LM_TIDS				8
#define T2LM_DIRECTION_DL				0x00
#define T2LM_DIRECTION_UL				0x01
#define T2LM_DIRECTION_DL_UL				0x02
#define T2LM_CTRL_DIRECTION				BITS(0, 1)
#define T2LM_CTRL_DEFAULT_LINK_MAPPING			BIT(2)
#define T2LM_CTRL_DEFAULT_LINK_MAPPING_SHIFT		2
#define T2LM_CTRL_MAPPING_SWITCH_TIME_PRESENT		BIT(3)
#define T2LM_CTRL_MAPPING_SWITCH_TIME_PRESENT_SHIFT	3
#define T2LM_CTRL_EXPECTED_DURATION_PRESENT		BIT(4)
#define T2LM_CTRL_EXPECTED_DURATION_PRESENT_SHIFT	4
#define T2LM_CTRL_LINK_MAPPING_SIZE			BIT(5)
#define T2LM_CTRL_LINK_MAPPING_SIZE_SHIFT		5
#define T2LM_CTRL_RESERVED				BITS(6, 7)
#define T2LM_CTRL_LINK_MAPPING_PRESENT_INDICATOR	BITS(8, 15)

/* Figure 9-788eo - STA Control field format */
#define SUB_IE_MLD_PER_STA_PROFILE			0
#define ML_STA_CTRL_LINK_ID_MASK			BITS(0, 3)
#define ML_STA_CTRL_LINK_ID_SHIFT			0
#define ML_STA_CTRL_COMPLETE_PROFILE			BIT(4)
#define ML_STA_CTRL_MAC_ADDR_PRESENT			BIT(5)
#define ML_STA_CTRL_BCN_INTV_PRESENT			BIT(6)
#define ML_STA_CTRL_TSF_OFFSET_PRESENT			BIT(7)
#define ML_STA_CTRL_DTIM_INFO_PRESENT			BIT(8)
#define ML_STA_CTRL_NSTR_LINK_PAIR_PRESENT		BIT(9)
#define ML_STA_CTRL_NSTR_BMP_SIZE			BIT(10)
#define ML_STA_CTRL_NSTR_BMP_SIZE_SHIFT			10
#define ML_STA_CTRL_BSS_PARA_CHANGE_COUNT_PRESENT	BIT(11)

/* BE D3.0 9.4.2.312.2.3 Common info field of the Basic Multi-Link Element */
/* Figure 9-1002k - MLD Capabilities and Operations subfield format */
#define MLD_CAP_MAX_SIMULTANEOUS_LINK_MASK		BITS(0, 3)
#define MLD_CAP_MAX_SIMULTANEOUS_LINK_SHIFT		0
#define MLD_CAP_SRS_SUPPORT				BIT(4)
#define MLD_CAP_TID_TO_LINK_NEGO_MASK			BITS(5, 6)
#define MLD_CAP_TID_TO_LINK_NEGO_SHIFT			5
#define MLD_CAP_FREQ_SEPARATION_MASK			BITS(7, 11)
#define MLD_CAP_FREQ_SEPARATION_SHIFT			7
#define MLD_CAP_AAR					BIT(12)
#define MLD_CAP_LINK_RECONFIG_OP_SUPPORT		BIT(13)

/* Figure 9-1002n - Presence Bitmap field of the Probe Request ML element */
#define ML_PRBREQ_CTRL_MLD_ID_PRESENT			BIT(0)

/* 9.4.312.4 Reconfiguration Multi-Link element */
#define ML_RECFG_MLD_ADDR_PRESENT			BIT(0)
#define ML_RECFG_EML_CAP_PRESENT			BIT(1)
#define ML_RECFG_MLD_CAP_OP_PRESENT			BIT(2)

#define ML_RECFG_STA_CTRL_LINK_ID_MASK			BITS(0, 3)
#define ML_RECFG_STA_CTRL_LINK_ID_SHIFT			0
#define ML_RECFG_STA_CTRL_COMPLETE_PROFILE		BIT(4)
#define ML_RECFG_STA_CTRL_MAC_ADDR_PRESENT		BIT(5)
#define ML_RECFG_STA_CTRL_DELETE_TIMER_PRESENT		BIT(6)
#define ML_RECFG_STA_CTRL_OP_TYPE_MASK			BITS(7, 10)
#define ML_RECFG_STA_CTRL_OP_TYPE_SHIFT			7
#define ML_RECFG_STA_CTRL_OP_PARAM_PRESENT		BIT(11)
#define ML_RECFG_STA_CTRL_NSTR_BMAP_SIZE		BIT(12)

#define ML_RECFG_STA_CTRL_OP_TYPE_AP_REMOVAL		0
#define ML_RECFG_STA_CTRL_OP_TYPE_UPDATE		1
#define ML_RECFG_STA_CTRL_OP_TYPE_ADD_LINK		2
#define ML_RECFG_STA_CTRL_OP_TYPE_DEL_LINK		3

/* BE D3.0 9.4.312.6 Priority Access Multi-Link element */
/* BE D3.0 Figure 9-1002af - STA Control field format for the Priority Access
 * Multi-Link element
 */
#define ML_PRIACC_STA_CTRL_LINK_ID_MASK			BITS(0, 3)

#define SUB_IE_MLD_VENDOR_SPECIFIC			221
#define SUB_IE_MLD_FRAGMENT				254


#if CFG_AP_80211K_SUPPORT
/* Optional subelement IDs for Beacon request (Table 9-88) */
#define BCN_REQ_ELEM_SUBID_SSID              0
#define BCN_REQ_ELEM_SUBID_BEACON_REPORTING  1
#define BCN_REQ_ELEM_SUBID_REPORTING_DETAIL  2
#define BCN_REQ_ELEM_SUBID_REQUEST           10
#define BCN_REQ_ELEM_SUBID_AP_CHANNEL_REPORT 51
#define BCN_REQ_ELEM_SUBID_WIDE_BW_CH_SWITCH 163
#endif /* CFG_AP_80211K_SUPPORT */

#if (CFG_SUPPORT_TX_PWR_ENV == 1)

/* TxPower Envelope Max TxPower Info Subfield */
#define TX_PWR_ENV_INFO_TXPWR_COUNT_MAX       8
#define TX_PWR_ENV_INFO_TXPWR_COUNT_MASK      BITS(0, 2)
#define TX_PWR_ENV_INFO_TXPWR_COUNT_OFFSET    0

#define TX_PWR_ENV_INFO_TXPWR_INTRPT_MASK     BITS(3, 5)
#define TX_PWR_ENV_INFO_TXPWR_INTRPT_OFFSET   3

#define TX_PWR_ENV_INFO_TXPWR_CATEGORY_MASK   BITS(6, 7)
#define TX_PWR_ENV_INFO_TXPWR_CATEGORY_OFFSET 6

#define TX_PWR_ENV_INFO_GET_TXPWR_COUNT(_ucTxPwrInfo) \
	((_ucTxPwrInfo & TX_PWR_ENV_INFO_TXPWR_COUNT_MASK) \
	>> TX_PWR_ENV_INFO_TXPWR_COUNT_OFFSET)
#define TX_PWR_ENV_INFO_GET_TXPWR_INTRPT(_ucTxPwrInfo) \
	((_ucTxPwrInfo & TX_PWR_ENV_INFO_TXPWR_INTRPT_MASK) \
	>> TX_PWR_ENV_INFO_TXPWR_INTRPT_OFFSET)
#define TX_PWR_ENV_INFO_GET_TXPWR_CATEGORY(_ucTxPwrInfo) \
	((_ucTxPwrInfo & TX_PWR_ENV_INFO_TXPWR_CATEGORY_MASK) \
	>> TX_PWR_ENV_INFO_TXPWR_CATEGORY_OFFSET)

enum TX_PWR_ENV_MAX_TXPWR_INTRPT_TYPE {
	TX_PWR_ENV_LOCAL_EIRP = 0,
	TX_PWR_ENV_LOCAL_EIRP_PSD = 1,
	TX_PWR_ENV_REG_CLIENT_EIRP = 2,
	TX_PWR_ENV_REG_CLIENT_EIRP_PSD = 3,
};
enum TX_PWR_ENV_MAX_TXPWR_BW_TYPE {
	TX_PWR_ENV_MAX_TXPWR_BW20 = 0,
	TX_PWR_ENV_MAX_TXPWR_BW40 = 1,
	TX_PWR_ENV_MAX_TXPWR_BW80 = 2,
	TX_PWR_ENV_MAX_TXPWR_BW160 = 3,
	TX_PWR_ENV_MAX_TXPWR_BW_NUM
};
#endif /* CFG_SUPPORT_TX_PWR_ENV */

enum CUS_BLOCKLIST_TYPE {
	CUS_BLOCKLIST_TYPE_SSID = 0,
	CUS_BLOCKLIST_TYPE_BSSID,
	CUS_BLOCKLIST_TYPE_FREQUENCY,
	CUS_BLOCKLIST_TYPE_BAND
};

enum CUS_BLOCKLIST_LIMIT_TYPE {
	LIMIT_FIRST_CONNECTION = 0,
	LIMIT_ROAMING,
};

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

__KAL_ATTRIB_PACKED_FRONT__
struct LLC_SNAP_HEADER {
	uint8_t ucDSAP;
	uint8_t ucSSAP;
	uint8_t ucControl;
	uint8_t aucCode[3];
	uint16_t u2Type;
} __KAL_ATTRIB_PACKED__;

/* Ethernet Frame Structure */
__KAL_ATTRIB_PACKED_FRONT__
struct ETH_FRAME {
	uint8_t aucDestAddr[MAC_ADDR_LEN];
	uint8_t aucSrcAddr[MAC_ADDR_LEN];
	uint16_t u2TypeLen;
	uint8_t aucData[];
} __KAL_ATTRIB_PACKED__;

/* RFC 826 */
__KAL_ATTRIB_PACKED_FRONT__
struct ARP_HEADER {
	uint16_t u2HwType;
	uint16_t u2ProtocolType;
	uint8_t ucHwLength;
	uint8_t ucProtocolLength;
	uint16_t u2OpCode;
	uint8_t aucSenderMACaddr[MAC_ADDR_LEN];
	uint8_t aucSenderIPaddr[IPV4_ADDR_LEN];
	uint8_t aucTargetMACaddr[MAC_ADDR_LEN];
	uint8_t aucTargetIPaddr[IPV4_ADDR_LEN];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct IPV4_HEADER {
	uint8_t ucVersionAndHeaderLength; /* 4-bit Version + 4-bit IHL in DW */
	uint8_t ucTos;
	uint16_t u2TotalLength;
	uint16_t u2Identifier;
	uint16_t u2FragmentOffset; /* 0, DF, MF + 13-bit Fragment offset */
	uint8_t ucTtl;
	uint8_t ucProtocol;
	uint16_t u2Checksum;
	uint8_t aucSourceAddr[IPV4_ADDR_LEN];
	uint8_t aucDestinationAddr[IPV4_ADDR_LEN];
	uint8_t aucL4[];
} __KAL_ATTRIB_PACKED__;

#define IPV4_HDR_LEN			(sizeof(struct IPV4_HEADER))

#define GET_IP_VERSION(_pucIPHeader) \
	((((uint8_t *)(_pucIPHeader))[0] & IPVH_VERSION_MASK) \
	 >> IPVH_VERSION_OFFSET)

/**
 * Get IPv4 header length in bytes;
 * if the length is invalid, return 0 with a warning log
 */
#define GET_IPV4_HEADER_SIZE(_pucIPv4) ({				       \
	uint8_t ucIPv4HeaderLength;					       \
									       \
	ucIPv4HeaderLength =						       \
		(((struct IPV4_HEADER *)(_pucIPv4))->ucVersionAndHeaderLength &\
		 IPV4_HEADER_LENGTH_MASK) << 2;				       \
	if (ucIPv4HeaderLength < IPV4_HDR_LEN) {			       \
		DBGLOG_LIMITED(RX, WARN, "Unexpected IP header length %u\n",   \
		       ucIPv4HeaderLength);				       \
		ucIPv4HeaderLength = 0;					       \
	}								       \
	ucIPv4HeaderLength;						       \
})

__KAL_ATTRIB_PACKED_FRONT__
struct IPV6_HEADER {
	uint8_t ucVersionAndPriority; /* 4-bit Version + half 8-bit Priority */
	uint8_t ucPriorityAndFlow[3]; /* half 8-bit Priority + 20-bit Flow */
	uint16_t u2PayloadLength;
	uint8_t ucNextHeader;
	uint8_t ucHopLimit;
	uint8_t aucSourceAddr[IPV6_ADDR_LEN];
	uint8_t aucDestinationAddr[IPV6_ADDR_LEN];
	uint8_t aucL4[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct ICMPV6_HEADER {
	uint8_t ucType;
	uint8_t ucCode;
	uint16_t u2Checksum;
	uint8_t aucICMPv6Body[];
} __KAL_ATTRIB_PACKED__;

/* RFC 792, RFC 4443 4.1 4.2 */
__KAL_ATTRIB_PACKED_FRONT__
struct ICMP_ECHO_HEADER {
	uint8_t ucType;
	uint8_t ucCode;
	uint16_t u2Checksum;
	uint16_t u2Identifier;
	uint16_t u2SequenceNumber;
	uint8_t aucData[];
} __KAL_ATTRIB_PACKED__;

#define GET_ICMP_TYPE(_pucIcmp) (((uint8_t *)(_pucIcmp))[0])
#define GET_ICMP_CODE(_pucIcmp) (((uint8_t *)(_pucIcmp))[1])

/* RFC 4861 4.3 4.4 */
__KAL_ATTRIB_PACKED_FRONT__
struct ICMPV6_NSNA_HEADER {
	uint8_t ucType;
	uint8_t ucCode;
	uint16_t u2Checksum;
	uint32_t u4Reserved;
	uint8_t aucTargetAddress[IPV6_ADDR_LEN];
	uint8_t aucOption[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UDP_HEADER {
	uint16_t u2SrcPort;
	uint16_t u2DstPort;
	uint16_t u2Length;
	uint16_t u2Checksum;
	uint8_t aucData[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct DHCP_PROTOCOL {
	uint8_t ucOperation;
	uint8_t ucHdrType;
	uint8_t ucHdrLen;
	uint8_t ucHops;
	uint32_t u4TransId;
	uint16_t u2Seconds;
	uint16_t u2Flags;
	uint32_t u4CIAddr;
	uint32_t u4YIAddr;
	uint32_t u4SIAddr;
	uint32_t u4GIAddr;
	uint8_t aucCHAddr[16];
	uint8_t aucServerName[64];
	uint8_t aucFileName[128];
	uint32_t u4MagicCookie;
	uint8_t aucDhcpOption[]; /* after fixed cookie opt */
} __KAL_ATTRIB_PACKED__;

/* DHCP options from RFC 2132, only enumerate used ones */
enum DHCP_OPTION {
	DHCP_OPTION_PAD = 0,
	DHCP_OPTION_ROUTER = 3,
	DHCP_OPTION_MESSAGE_TYPE = 53,
	DHCP_OPTION_END = 255,
};


/* IEEE 802.11 WLAN Frame Structure */
/* WLAN MAC Header (without Address 4 and QoS Control fields) */
__KAL_ATTRIB_PACKED_FRONT__
struct WLAN_MAC_HEADER {
	uint16_t u2FrameCtrl;
	uint16_t u2DurationID;
	uint8_t aucAddr1[MAC_ADDR_LEN];
	uint8_t aucAddr2[MAC_ADDR_LEN];
	uint8_t aucAddr3[MAC_ADDR_LEN];
	uint16_t u2SeqCtrl;
} __KAL_ATTRIB_PACKED__;

/* WLAN MAC Header (QoS Control fields included) */
__KAL_ATTRIB_PACKED_FRONT__
struct WLAN_MAC_HEADER_QOS {
	uint16_t u2FrameCtrl;
	uint16_t u2DurationID;
	uint8_t aucAddr1[MAC_ADDR_LEN];
	uint8_t aucAddr2[MAC_ADDR_LEN];
	uint8_t aucAddr3[MAC_ADDR_LEN];
	uint16_t u2SeqCtrl;
	uint16_t u2QosCtrl;
} __KAL_ATTRIB_PACKED__;

/* WLAN MAC Header (HT Control fields included) */
__KAL_ATTRIB_PACKED_FRONT__
struct WLAN_MAC_HEADER_HT {
	uint16_t u2FrameCtrl;
	uint16_t u2DurationID;
	uint8_t aucAddr1[MAC_ADDR_LEN];
	uint8_t aucAddr2[MAC_ADDR_LEN];
	uint8_t aucAddr3[MAC_ADDR_LEN];
	uint16_t u2SeqCtrl;
	uint16_t u2QosCtrl;
	uint32_t u4HtCtrl;
} __KAL_ATTRIB_PACKED__;

/* WLAN MAC Header (Address 4 included) */
__KAL_ATTRIB_PACKED_FRONT__
struct WLAN_MAC_HEADER_A4 {
	uint16_t u2FrameCtrl;
	uint16_t u2DurationID;
	uint8_t aucAddr1[MAC_ADDR_LEN];
	uint8_t aucAddr2[MAC_ADDR_LEN];
	uint8_t aucAddr3[MAC_ADDR_LEN];
	uint16_t u2SeqCtrl;
	uint8_t aucAddr4[MAC_ADDR_LEN];
} __KAL_ATTRIB_PACKED__;

/* WLAN MAC Header (Address 4 and QoS Control fields included) */
__KAL_ATTRIB_PACKED_FRONT__
struct WLAN_MAC_HEADER_A4_QOS {
	uint16_t u2FrameCtrl;
	uint16_t u2DurationID;
	uint8_t aucAddr1[MAC_ADDR_LEN];
	uint8_t aucAddr2[MAC_ADDR_LEN];
	uint8_t aucAddr3[MAC_ADDR_LEN];
	uint16_t u2SeqCtrl;
	uint8_t aucAddr4[MAC_ADDR_LEN];
	uint16_t u2QosCtrl;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct WLAN_MAC_HEADER_A4_HT {
	uint16_t u2FrameCtrl;
	uint16_t u2DurationID;
	uint8_t aucAddr1[MAC_ADDR_LEN];
	uint8_t aucAddr2[MAC_ADDR_LEN];
	uint8_t aucAddr3[MAC_ADDR_LEN];
	uint16_t u2SeqCtrl;
	uint8_t aucAddr4[MAC_ADDR_LEN];
	uint16_t u2QosCtrl;
	uint32_t u4HtCtrl;
} __KAL_ATTRIB_PACKED__;

/* 7.2.3 WLAN MAC Header for Management Frame - MMPDU */
__KAL_ATTRIB_PACKED_FRONT__
struct WLAN_MAC_MGMT_HEADER {
	uint16_t u2FrameCtrl;
	uint16_t u2Duration;
	uint8_t aucDestAddr[MAC_ADDR_LEN];
	uint8_t aucSrcAddr[MAC_ADDR_LEN];
	uint8_t aucBSSID[MAC_ADDR_LEN];
	uint16_t u2SeqCtrl;
} __KAL_ATTRIB_PACKED__;

/* WLAN MAC Header for Management Frame (HT Control fields included) */
__KAL_ATTRIB_PACKED_FRONT__
struct WLAN_MAC_MGMT_HEADER_HT {
	uint16_t u2FrameCtrl;
	uint16_t u2DurationID;
	uint8_t aucAddr1[MAC_ADDR_LEN];
	uint8_t aucAddr2[MAC_ADDR_LEN];
	uint8_t aucAddr3[MAC_ADDR_LEN];
	uint16_t u2SeqCtrl;
	uint32_t u4HtCtrl;
} __KAL_ATTRIB_PACKED__;

/* 3 WLAN CONTROL Frame */
/* 7.2.1.4 WLAN Control Frame - PS-POLL Frame */
__KAL_ATTRIB_PACKED_FRONT__
struct CTRL_PSPOLL_FRAME {
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2AID;		/* AID */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint8_t aucTA[MAC_ADDR_LEN];	/* TA */
} __KAL_ATTRIB_PACKED__;

/* 802.11 2020 Figure
 * 9.3.1.7 BlockAckReq frame format
 * Figure 9-35 BlockAckReq frame format
 */
__KAL_ATTRIB_PACKED_FRONT__
struct CTRL_BAR_FRAME {
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2DurationID;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* RA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* TA */
	uint16_t u2BarControl;
	uint8_t aucBarInfo[];	/* Variable size */
} __KAL_ATTRIB_PACKED__;

/* 3 WLAN Management Frame. */
/* 7.2.3.1 WLAN Management Frame - Beacon Frame */
__KAL_ATTRIB_PACKED_FRONT__
struct WLAN_BEACON_FRAME {
	/* Beacon header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2DurationID;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* Beacon frame body */
	uint32_t au4Timestamp[2];	/* Timestamp */
	uint16_t u2BeaconInterval;	/* Beacon Interval */
	uint16_t u2CapInfo;	/* Capability */
	uint8_t aucInfoElem[];	/* Various IEs, start from SSID */
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct WLAN_BEACON_FRAME_BODY {
	/* Beacon frame body */
	uint32_t au4Timestamp[2];	/* Timestamp */
	uint16_t u2BeaconInterval;	/* Beacon Interval */
	uint16_t u2CapInfo;	/* Capability */
	uint8_t aucInfoElem[];	/* Various IEs, start from SSID */
} __KAL_ATTRIB_PACKED__;

/* 7.2.3.3 WLAN Management Frame - Disassociation Frame */
__KAL_ATTRIB_PACKED_FRONT__
struct WLAN_DISASSOC_FRAME {
	/* Authentication MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2DurationID;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* Disassociation frame body */
	uint16_t u2ReasonCode;	/* Reason code */
	uint8_t aucInfoElem[];	/* Various IEs, possible no. */
} __KAL_ATTRIB_PACKED__;

/* 7.2.3.4 WLAN Management Frame - Association Request frame */
__KAL_ATTRIB_PACKED_FRONT__
struct WLAN_ASSOC_REQ_FRAME {
	/* Association Request MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2DurationID;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* Association Request frame body */
	uint16_t u2CapInfo;	/* Capability information */
	uint16_t u2ListenInterval;	/* Listen interval */
	uint8_t aucInfoElem[];	/* Information elements, include WPA IE */
} __KAL_ATTRIB_PACKED__;

/* 7.2.3.5 WLAN Management Frame - Association Response frame */
__KAL_ATTRIB_PACKED_FRONT__
struct WLAN_ASSOC_RSP_FRAME {
	/* Association Response MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2DurationID;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* Association Response frame body */
	uint16_t u2CapInfo;	/* Capability information */
	uint16_t u2StatusCode;	/* Status code */
	uint16_t u2AssocId;	/* Association ID */
	uint8_t aucInfoElem[1];	/* Information elements, such as */
				/* supported rates, and etc. */
} __KAL_ATTRIB_PACKED__;

/* 7.2.3.6 WLAN Management Frame - Reassociation Request frame */
__KAL_ATTRIB_PACKED_FRONT__
struct WLAN_REASSOC_REQ_FRAME {
	/* Reassociation Request MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2DurationID;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* Reassociation Request frame body */
	uint16_t u2CapInfo;	/* Capability information */
	uint16_t u2ListenInterval;	/* Listen interval */
	uint8_t aucCurrentAPAddr[MAC_ADDR_LEN];	/* Current AP address */
	uint8_t aucInfoElem[];	/* Information elements, include WPA IE */
} __KAL_ATTRIB_PACKED__;

/* 7.2.3.7 WLAN Management Frame - Reassociation Response frame */
/*   (the same as Association Response frame) */

/* 7.2.3.9 WLAN Management Frame - Probe Response Frame */

/* 7.2.3.10 WLAN Management Frame - Authentication Frame */
__KAL_ATTRIB_PACKED_FRONT__
struct WLAN_AUTH_FRAME {
	/* Authentication MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2DurationID;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* Authentication frame body */
	uint16_t u2AuthAlgNum;	/* Authentication algorithm number */
	/* Authentication transaction sequence number */
	uint16_t u2AuthTransSeqNo;
	uint16_t u2StatusCode;	/* Status code */
	uint8_t aucInfoElem[];	/* Various IEs for Fast BSS Transition */
} __KAL_ATTRIB_PACKED__;

/* 7.2.3.11 WLAN Management Frame - Deauthentication Frame */
__KAL_ATTRIB_PACKED_FRONT__
struct WLAN_DEAUTH_FRAME {
	/* Authentication MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2DurationID;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* Deauthentication frame body */
	uint16_t u2ReasonCode;	/* Reason code */
	uint8_t aucInfoElem[];	/* Various IEs, possible no. */
} __KAL_ATTRIB_PACKED__;

/* 3 Information Elements. */
/* 7.3.2 Generic element format */
__KAL_ATTRIB_PACKED_FRONT__
struct IE_HDR {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t aucInfo[];
} __KAL_ATTRIB_PACKED__;

/* 7.3.2.1 SSID element */
__KAL_ATTRIB_PACKED_FRONT__
struct IE_SSID {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t aucSSID[ELEM_MAX_LEN_SSID];
} __KAL_ATTRIB_PACKED__;

/* 7.3.2.2 Supported Rates element */
__KAL_ATTRIB_PACKED_FRONT__
struct IE_SUPPORTED_RATE {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t aucSupportedRates[ELEM_MAX_LEN_SUP_RATES];
} __KAL_ATTRIB_PACKED__;

/* Some IOT AP will carry Rates > 8*/
__KAL_ATTRIB_PACKED_FRONT__
struct IE_SUPPORTED_RATE_IOT {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t aucSupportedRates[ELEM_MAX_LEN_SUP_RATES_IOT];
} __KAL_ATTRIB_PACKED__;

/* 7.3.2.4 DS Parameter Set element */
__KAL_ATTRIB_PACKED_FRONT__
struct IE_DS_PARAM_SET {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t ucCurrChnl;
} __KAL_ATTRIB_PACKED__;

/* 7.3.2.5 CF Parameter Set element */
__KAL_ATTRIB_PACKED_FRONT__
struct IE_CF_PARAM_SET {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t ucCFPCount;
	uint8_t ucCFPPeriod;
	uint16_t u2CFPMaxDur;
	uint16_t u2DurRemaining;
} __KAL_ATTRIB_PACKED__;

/* IEEE 802.11 2020 9.4.2.5 TIM */
__KAL_ATTRIB_PACKED_FRONT__
struct IE_TIM {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t ucDTIMCount;
	uint8_t ucDTIMPeriod;
	uint8_t ucBitmapControl;
	uint8_t aucPartialVirtualMap[];
} __KAL_ATTRIB_PACKED__;

/* 7.3.2.7 IBSS Parameter Set element */
__KAL_ATTRIB_PACKED_FRONT__
struct IE_IBSS_PARAM_SET {
	uint8_t ucId;
	uint8_t ucLength;
	uint16_t u2ATIMWindow;
} __KAL_ATTRIB_PACKED__;

/* 7.3.2.8 Challenge Text element */
__KAL_ATTRIB_PACKED_FRONT__
struct IE_CHALLENGE_TEXT {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t aucChallengeText[ELEM_MAX_LEN_CHALLENGE_TEXT];
} __KAL_ATTRIB_PACKED__;

/* 7.3.2.9 Country information element */
#if CFG_SUPPORT_802_11D
/*! \brief COUNTRY_INFO_TRIPLET
 * is defined for the COUNTRY_INFO_ELEM structure.
 */
__KAL_ATTRIB_PACKED_FRONT__
struct COUNTRY_INFO_TRIPLET {
	/*!< If param1 >= 201, this triplet is referred to as
	 * Regulatory Triplet in 802_11J.
	 */
	uint8_t ucParam1;
	uint8_t ucParam2;
	uint8_t ucParam3;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct COUNTRY_INFO_SUBBAND_TRIPLET {
	uint8_t ucFirstChnlNum;	/*!< First Channel Number */
	uint8_t ucNumOfChnl;	/*!< Number of Channels */
	int8_t cMaxTxPwrLv;	/*!< Maximum Transmit Power Level */
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct COUNTRY_INFO_REGULATORY_TRIPLET {
	uint8_t ucRegExtId;	/*!< Regulatory Extension Identifier, should */
				/* be greater than or equal to 201 */
	uint8_t ucRegClass;	/*!< Regulatory Class */
	/*!< Coverage Class, unsigned 1-octet value 0~31 */
	uint8_t ucCoverageClass;
				/* , 32~255 reserved */
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct IE_COUNTRY {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t aucCountryStr[3];
	struct COUNTRY_INFO_SUBBAND_TRIPLET arCountryStr[1];
} __KAL_ATTRIB_PACKED__;
#endif /* CFG_SUPPORT_802_11D */

/* 7.3.2.13 ERP element */
__KAL_ATTRIB_PACKED_FRONT__
struct IE_ERP {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t ucERP;
} __KAL_ATTRIB_PACKED__;

/* 7.3.2.14 Extended Supported Rates element */
__KAL_ATTRIB_PACKED_FRONT__
struct IE_EXT_SUPPORTED_RATE {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t aucExtSupportedRates[ELEM_MAX_LEN_EXTENDED_SUP_RATES];
} __KAL_ATTRIB_PACKED__;

/* 7.3.2.15 Power Constraint element */
__KAL_ATTRIB_PACKED_FRONT__
struct IE_POWER_CONSTRAINT {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t ucLocalPowerConstraint;	/* Unit: dBm */
} __KAL_ATTRIB_PACKED__;

/* 7.3.2.16 Power Capability element */
__KAL_ATTRIB_PACKED_FRONT__
struct IE_POWER_CAP {
	uint8_t ucId;
	uint8_t ucLength;
	int8_t cMinTxPowerCap;	/* Unit: dBm */
	int8_t cMaxTxPowerCap;	/* Unit: dBm */
} __KAL_ATTRIB_PACKED__;

/* 7.3.2.17 TPC request element */
__KAL_ATTRIB_PACKED_FRONT__
struct IE_TPC_REQ {
	uint8_t ucId;
	uint8_t ucLength;
} __KAL_ATTRIB_PACKED__;

/* 7.3.2.18 TPC report element */
__KAL_ATTRIB_PACKED_FRONT__
struct IE_TPC_REPORT {
	uint8_t ucId;
	uint8_t ucLength;
	int8_t cTxPower;		/* Unit: dBm */
	int8_t cLinkMargin;	/* Unit: dB */
} __KAL_ATTRIB_PACKED__;

/* 7.3.2.19 Supported Channels element*/
__KAL_ATTRIB_PACKED_FRONT__
struct IE_SUPPORTED_CHANNELS {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t ucChannelNum[];
} __KAL_ATTRIB_PACKED__;

/* 7.3.2.20 Channel Switch Announcement element*/
__KAL_ATTRIB_PACKED_FRONT__
struct IE_CHANNEL_SWITCH {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t ucChannelSwitchMode;
	uint8_t ucNewChannelNum;
	uint8_t ucChannelSwitchCount;
} __KAL_ATTRIB_PACKED__;

/* Extended Channel Switch Announcement element */
__KAL_ATTRIB_PACKED_FRONT__
struct IE_EX_CHANNEL_SWITCH {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t ucChannelSwitchMode;
	uint8_t ucNewOperatingClass;
	uint8_t ucNewChannelNum;
	uint8_t ucChannelSwitchCount;
} __KAL_ATTRIB_PACKED__;

/* Channel Switch Wrapper element */
__KAL_ATTRIB_PACKED_FRONT__
struct IE_CHANNEL_SWITCH_WRAPPER {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t aucInfoElem[];
} __KAL_ATTRIB_PACKED__;

/* 9.4.2.217 Max Channel Switch Time element*/
__KAL_ATTRIB_PACKED_FRONT__
struct IE_MAX_CHANNEL_SWITCH_TIME {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t ucExtId;
	uint8_t ucChannelSwitchTime[3];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct IE_TIMEOUT_INTERVAL {
	uint8_t ucId;
	uint8_t ucLength;
#define IE_TIMEOUT_INTERVAL_TYPE_RESERVED			0
#define IE_TIMEOUT_INTERVAL_TYPE_REASSOC			1
#define IE_TIMEOUT_INTERVAL_TYPE_KEY_LIFETIME		43200
#define IE_TIMEOUT_INTERVAL_TYPE_ASSOC_COMEBACK		3
	uint8_t ucType;
	uint32_t u4Value;
} __KAL_ATTRIB_PACKED__;

/* 7.3.2.20 Channel Switch Announcement element */
__KAL_ATTRIB_PACKED_FRONT__
struct IE_CHNL_SWITCH {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t ucSwitchMode;
	uint8_t ucNewChannel;
	uint8_t ucSwitchCount;
} __KAL_ATTRIB_PACKED__;

/* 7.3.2.21 Measurement Request element */
__KAL_ATTRIB_PACKED_FRONT__
struct IE_MEASUREMENT_REQ {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t ucToken;
	uint8_t ucRequestMode;
	uint8_t ucMeasurementType;
	uint8_t aucRequestFields[];
} __KAL_ATTRIB_PACKED__;

/* 7.3.2.60 20/40 BSS Coexistence element */
__KAL_ATTRIB_PACKED_FRONT__
struct IE_SUP_OPERATING_CLASS {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t ucCur;
	uint8_t ucSup[255];
} __KAL_ATTRIB_PACKED__;

/* 8.4.2.30 BSS Load element */
__KAL_ATTRIB_PACKED_FRONT__
struct IE_BSS_LOAD {
	uint8_t ucId;
	uint8_t ucLength;
	uint16_t u2StaCnt;
	uint8_t ucChnlUtilizaion;
	uint16_t u2AvailabeAC;
} __KAL_ATTRIB_PACKED__;

/* 8.4.2.39 Neighbor Report Element */
__KAL_ATTRIB_PACKED_FRONT__
struct IE_NEIGHBOR_REPORT {
	uint8_t ucId;		/* Element ID */
	uint8_t ucLength;	/* Length */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* OUI */
	uint32_t u4BSSIDInfo;		/* Type */
	uint8_t ucOperClass; /* Hotspot Configuration */
	uint8_t ucChnlNumber;
	uint8_t ucPhyType;
	uint8_t aucSubElem[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct IE_MBO_OCE {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t aucOui[3];
	uint8_t ucOuiType;
	uint8_t aucSubElements[];
} __KAL_ATTRIB_PACKED__;

/* 802.11-2020 9.6.6.6 Figure 9-864 Neighbor Report Request frame
 * 802.11-2020 9.6.6.7 Figure 9-865 Neighbor Report Response frame
 */
__KAL_ATTRIB_PACKED_FRONT__
struct ACTION_NEIGHBOR_REPORT_FRAME {
	/* Neighbor Report Request/Response MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2Duration;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* Neighbor Report Request/Response frame body */
	uint8_t ucCategory;	/* Category */
	uint8_t ucAction;	/* Action Value */
	uint8_t ucDialogToken;	/* Dialog Token */
	uint8_t aucInfoElem[];	/* subelements */
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct SUB_ELEMENT {
	uint8_t ucSubID;
	uint8_t ucLength;
	uint8_t aucOptInfo[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct SM_BASIC_REQ {
	uint8_t ucChannel;
	uint32_t au4StartTime[2];
	uint16_t u2Duration;
} __KAL_ATTRIB_PACKED__;

/* SM_COMMON_REQ_T is not specified in Spec. Use it as common structure of SM */

__KAL_ATTRIB_PACKED_FRONT__
struct RM_CHNL_LOAD_REQ {
	uint8_t ucRegulatoryClass;
	uint8_t ucChannel;
	uint16_t u2RandomInterval;
	uint16_t u2Duration;
	uint8_t aucSubElements[];
} __KAL_ATTRIB_PACKED__;


__KAL_ATTRIB_PACKED_FRONT__
struct RM_BCN_REQ {
	uint8_t ucRegulatoryClass;
	uint8_t ucChannel;
	uint16_t u2RandomInterval;
	uint16_t u2Duration;
	uint8_t ucMeasurementMode;
	uint8_t aucBssid[6];
	uint8_t aucSubElements[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct RM_FRAME_REQ {
	uint8_t ucRegulatoryClass;
	uint8_t ucChannel;
	uint16_t u2RandomInterval;
	uint16_t u2Duration;
	uint8_t ucFrameReqType;
	uint8_t aucMacAddr[6];
	uint8_t aucSubElements[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct RM_STA_STATS_REQ {
	uint8_t aucPeerMacAddr[6];
	uint16_t u2RandomInterval;
	uint16_t u2Duration;
	uint8_t ucGroupID;
	uint8_t aucSubElements[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct RM_LCI_REQ {
	uint8_t ucLocationSubject;
	uint8_t ucLatitudeResolution;
	uint8_t ucLongitudeResolution;
	uint8_t ucAltitudeResolution;
	uint8_t aucSubElements[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct RM_TS_MEASURE_REQ {
	uint16_t u2RandomInterval;
	uint16_t u2Duration;
	uint8_t aucPeerStaAddr[6];
	uint8_t ucTrafficID;
	uint8_t ucBin0Range;
	uint8_t aucSubElements[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct RM_MEASURE_PAUSE_REQ {
	uint16_t u2PauseTime;
	uint8_t aucSubElements[];
} __KAL_ATTRIB_PACKED__;

/* 7.3.2.22 Measurement Report element */
__KAL_ATTRIB_PACKED_FRONT__
struct IE_MEASUREMENT_REPORT {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t ucToken;
	uint8_t ucReportMode;
	uint8_t ucMeasurementType;
	uint8_t aucReportFields[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct SM_BASIC_REPORT {
	uint8_t ucChannel;
	uint32_t u4StartTime[2];
	uint16_t u2Duration;
	uint8_t ucMap;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct SM_CCA_REPORT {
	uint8_t ucChannel;
	uint32_t u4StartTime[2];
	uint16_t u2Duration;
	uint8_t ucCcaBusyFraction;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct SM_RPI_REPORT {
	uint8_t ucChannel;
	uint32_t u4StartTime[2];
	uint16_t u2Duration;
	uint8_t aucRPI[8];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct RM_CHNL_LOAD_REPORT {
	uint8_t ucRegulatoryClass;
	uint8_t ucChannel;
	uint32_t u4StartTime[2];
	uint16_t u2Duration;
	uint8_t ucChnlLoad;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct RM_IPI_REPORT {
	uint8_t ucRegulatoryClass;
	uint8_t ucChannel;
	uint32_t u4StartTime[2];
	uint16_t u2Duration;
	uint8_t ucAntennaId;
	int8_t cANPI;
	uint8_t aucIPI[11];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct RM_BCN_REPORT {
	uint8_t ucRegulatoryClass;
	uint8_t ucChannel;
	uint8_t aucStartTime[8];
	uint16_t u2Duration;
	uint8_t ucReportInfo;
	uint8_t ucRCPI;
	uint8_t ucRSNI;
	uint8_t aucBSSID[MAC_ADDR_LEN];
	uint8_t ucAntennaID;
	uint8_t aucParentTSF[4];
	uint8_t aucOptElem[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct RM_TSM_REPORT {
	uint64_t u8ActualStartTime;
	uint16_t u2Duration;
	uint8_t aucPeerAddress[MAC_ADDR_LEN];
	uint8_t ucTID;
	uint8_t ucReason;
	uint32_t u4TransmittedMsduCnt;
	uint32_t u4DiscardedMsduCnt;
	uint32_t u4FailedMsduCnt;
	uint32_t u4MultiRetryCnt;
	uint32_t u4CfPollLostCnt;
	uint32_t u4AvgQueDelay;
	uint32_t u4AvgDelay;
	uint8_t ucBin0Range;
	uint32_t u4Bin[6];
	uint8_t aucOptSubElems[];
} __KAL_ATTRIB_PACKED__;

/* 7.3.2.23 Quiet element */
__KAL_ATTRIB_PACKED_FRONT__
struct IE_QUIET {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t ucCount;
	uint8_t ucPeriod;
	uint16_t u2Duration;
	uint16_t u2Offset;
} __KAL_ATTRIB_PACKED__;

/* 7.3.2.27 Extended Capabilities element */
__KAL_ATTRIB_PACKED_FRONT__
struct IE_EXT_CAP {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t aucCapabilities[ELEM_MAX_LEN_EXT_CAP];
} __KAL_ATTRIB_PACKED__;

/* 7.3.2.27 Extended Capabilities element */
__KAL_ATTRIB_PACKED_FRONT__
struct IE_RRM_ENABLED_CAP {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t aucCap[5];
} __KAL_ATTRIB_PACKED__;

/* 7.3.2.56 HT Capabilities element */
__KAL_ATTRIB_PACKED_FRONT__
struct SUP_MCS_SET_FIELD {
	uint8_t aucRxMcsBitmask[SUP_MCS_RX_BITMASK_OCTET_NUM];
	uint16_t u2RxHighestSupportedRate;
	uint32_t u4TxRateInfo;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct IE_HT_CAP {
	uint8_t ucId;
	uint8_t ucLength;
	uint16_t u2HtCapInfo;
	uint8_t ucAmpduParam;
	struct SUP_MCS_SET_FIELD rSupMcsSet;
	uint16_t u2HtExtendedCap;
	uint32_t u4TxBeamformingCap;
	uint8_t ucAselCap;
} __KAL_ATTRIB_PACKED__;

/* 7.3.2.57 HT Operation element */
__KAL_ATTRIB_PACKED_FRONT__
struct IE_HT_OP {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t ucPrimaryChannel;
	uint8_t ucInfo1;
	uint16_t u2Info2;
	uint16_t u2Info3;
	uint8_t aucBasicMcsSet[16];
} __KAL_ATTRIB_PACKED__;

/*8.4.2.160.3 VHT Supported MCS Set field*/
__KAL_ATTRIB_PACKED_FRONT__
struct VHT_SUPPORTED_MCS_FIELD {
	uint16_t u2RxMcsMap;
	uint16_t u2RxHighestSupportedDataRate;
	uint16_t u2TxMcsMap;
	uint16_t u2TxHighestSupportedDataRate;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct IE_VHT_CAP {
	uint8_t ucId;
	uint8_t ucLength;
	uint32_t u4VhtCapInfo;
	struct VHT_SUPPORTED_MCS_FIELD rVhtSupportedMcsSet;
} __KAL_ATTRIB_PACKED__;

/*8.4.2.161 VHT Operation element*/
__KAL_ATTRIB_PACKED_FRONT__
struct IE_VHT_OP {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t ucVhtOperation[3];
	uint16_t u2VhtBasicMcsSet;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct IE_VENDOR_EPIGRAM_IE {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t aucOui[3];
	uint8_t aucVendorType[2];
	uint8_t pucData[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct IE_VENDOR_ADAPTIVE_11R_IE {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t aucOui[3];
	uint8_t aucVendorType[1];
	uint8_t pucData[];
} __KAL_ATTRIB_PACKED__;

/*8.4.1.50 Operating Mode field*/
__KAL_ATTRIB_PACKED_FRONT__
struct IE_VHT_OP_MODE_NOTIFICATION {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t ucOperatingMode;
} __KAL_ATTRIB_PACKED__;


/*8.4.2.22 Secondary Channel Offset element*/
__KAL_ATTRIB_PACKED_FRONT__
struct IE_SECONDARY_OFFSET {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t ucSecondaryOffset;
} __KAL_ATTRIB_PACKED__;

/*8.4.2.105 Mesh Channel Switch Parameters element*/
__KAL_ATTRIB_PACKED_FRONT__
struct IE_MESH_CHANNEL {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t ucTimetoLive;
	uint8_t ucFlags;
	uint16_t u2ReasonCodes;
	uint16_t u2ProcedenceValue;
} __KAL_ATTRIB_PACKED__;

/*8.4.2.163 Wide Bandwidth Channel Switch element*/
__KAL_ATTRIB_PACKED_FRONT__
struct IE_WIDE_BAND_CHANNEL {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t ucNewChannelWidth;
	uint8_t ucChannelS1;
	uint8_t ucChannelS2;
} __KAL_ATTRIB_PACKED__;

/*8.4.2.168 Operating Mode Notification element*/
__KAL_ATTRIB_PACKED_FRONT__
struct IE_OP_MODE_NOTIFICATION {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t ucOpMode;
} __KAL_ATTRIB_PACKED__;

/* 7.3.2.25 RSN Information element format */
__KAL_ATTRIB_PACKED_FRONT__
struct RSN_INFO_ELEM {
	uint8_t ucElemId;
	uint8_t ucLength;
	uint16_t u2Version;
	uint32_t u4GroupKeyCipherSuite;
	uint16_t u2PairwiseKeyCipherSuiteCount;
	/* Meet the needs of variable length structure.
	 * There are many variables of variable length
	 * follow up, such as RSNCap, AKMSuite...
	 */
	uint8_t aucPairwiseKeyCipherSuite1[];
} __KAL_ATTRIB_PACKED__;

/* 7.3.2.26 WPA Information element format */
__KAL_ATTRIB_PACKED_FRONT__
struct WPA_INFO_ELEM {
	uint8_t ucElemId;
	uint8_t ucLength;
	uint8_t aucOui[3];
	uint8_t ucOuiType;
	uint16_t u2Version;
	uint32_t u4GroupKeyCipherSuite;
	uint16_t u2PairwiseKeyCipherSuiteCount;
	uint8_t aucPairwiseKeyCipherSuite1[4];
} __KAL_ATTRIB_PACKED__;

/* 7.3.2.58 20/40 BSS Intolerant Channel Report element */
__KAL_ATTRIB_PACKED_FRONT__
struct IE_INTOLERANT_CHNL_REPORT {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t ucRegulatoryClass;
	uint8_t aucChannelList[];
} __KAL_ATTRIB_PACKED__;

/* 7.3.2.59 OBSS Scan Parameters element */
__KAL_ATTRIB_PACKED_FRONT__
struct IE_OBSS_SCAN_PARAM {
	uint8_t ucId;
	uint8_t ucLength;
	uint16_t u2ScanPassiveDwell;
	uint16_t u2ScanActiveDwell;
	uint16_t u2TriggerScanInterval;
	uint16_t u2ScanPassiveTotalPerChnl;
	uint16_t u2ScanActiveTotalPerChnl;
	uint16_t u2WidthTransDelayFactor;
	uint16_t u2ScanActivityThres;
} __KAL_ATTRIB_PACKED__;

/* 7.3.2.60 20/40 BSS Coexistence element */
__KAL_ATTRIB_PACKED_FRONT__
struct IE_20_40_COEXIST {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t ucData;
} __KAL_ATTRIB_PACKED__;

/* 9.4.2.78 BSS Max Idle Period element */
__KAL_ATTRIB_PACKED_FRONT__
struct IE_BSS_MAX_IDLE_PERIOD {
	uint8_t ucId;
	uint8_t ucLength;
	uint16_t u2MaxIdlePeriod;
	uint8_t ucIdleOptions;
} __KAL_ATTRIB_PACKED__;

/* 9.4.2.183 FILS Indication element */
__KAL_ATTRIB_PACKED_FRONT__
struct IE_FILS_INDICATION_FRAME {
	uint8_t ucId;
	uint8_t ucLength;
	uint16_t u2Info;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct IE_FILS_NONCE {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t ucExtId;
	uint8_t aucNonce[16];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct IE_FILS_SESSION {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t ucExtId;
	uint8_t aucSession[8];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct IE_FILS_WRAPPED_DATA {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t ucExtId;
	uint8_t aucData[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct IE_FILS_KEY_CONFIRMATION {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t ucExtId;
	uint8_t aucKeyAuth[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct IE_KEY_DELIVERY {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t ucExtId;
	uint8_t aucKeyRSC[8];
	uint8_t aucKDEList[];
} __KAL_ATTRIB_PACKED__;

#if (CFG_SUPPORT_TWT == 1)
/* 11ax TWT element */
__KAL_ATTRIB_PACKED_FRONT__
struct _IE_TWT_T {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t ucCtrl;	/* Control */
	uint16_t u2ReqType;	/* Request Type */
	uint64_t u8TWT;	/* Target Wake Time 64 bits */
	uint8_t ucMinWakeDur;	/* Nominal Minimum TWT Wake Duration */
	uint16_t u2WakeIntvalMantiss;	/* TWT Wake Interval Mantissa */
	uint8_t ucReserved;	/* TWT Channel for 11ah. Reserved for 11ax */
} __KAL_ATTRIB_PACKED__;
#endif

__KAL_ATTRIB_PACKED_FRONT__
struct IE_HT_TPE {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t u8TxPowerInfo;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct IE_VHT_TPE {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t u8TxPowerInfo;
	uint8_t u8TxPowerBw[4];
} __KAL_ATTRIB_PACKED__;

#if (CFG_SUPPORT_TX_PWR_ENV == 1)
struct IE_TX_PWR_ENV_FRAME {
	uint8_t ucId; /* ELEM_ID_TX_PWR_ENVELOPE */
	uint8_t ucLength; /* Variable */
	uint8_t ucTxPwrInfo;   /* Bits[0:2] : Max TxPwr Count
				* Bits[3:5] : Max TxPwr Interpret
				* Bits[6:7] : Max TxPwr Category
				*/
	int8_t aicMaxTxPwr[TX_PWR_ENV_INFO_TXPWR_COUNT_MAX];
} __KAL_ATTRIB_PACKED__;
#endif

#if (CFG_SUPPORT_BTWT == 1)
__KAL_ATTRIB_PACKED_FRONT__
struct _IE_BTWT_T {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t ucCtrl;	/* Control */
	uint16_t u2ReqType;	/* Request Type */
	uint16_t u2TWT;	/* Target Wake Time 16 bits */
	uint8_t ucMinWakeDur;	/* Nominal Minimum TWT Wake Duration */
	uint16_t u2WakeIntvalMantiss;	/* TWT Wake Interval Mantissa */
	uint16_t u2BTWTInfo;	/* BTWT Info */
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct _IE_BTWT_PARAMS_T {
	uint16_t u2ReqType;	/* Request Type */
	uint16_t u2TWT;	/* Target Wake Time 16 bits */
	uint8_t ucMinWakeDur;	/* Nominal Minimum TWT Wake Duration */
	uint16_t u2WakeIntvalMantiss;	/* TWT Wake Interval Mantissa */
	uint16_t u2BTWTInfo;	/* BTWT Info */
} __KAL_ATTRIB_PACKED__;
#endif

#if (CFG_SUPPORT_RTWT == 1)
__KAL_ATTRIB_PACKED_FRONT__
struct _IE_RTWT_T {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t ucCtrl;	/* Control */
	uint16_t u2ReqType;	/* Request Type */
	uint16_t u2TWT;	/* Target Wake Time 16 bits */
	uint8_t ucMinWakeDur;	/* Nominal Minimum TWT Wake Duration */
	uint16_t u2WakeIntvalMantiss;	/* TWT Wake Interval Mantissa */
	uint16_t u2BTWTInfo;	/* BTWT Info */
	uint8_t uc_arRTWTTrafficInfo[0];  /* optional RTWT traffic info */
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct _IE_RTWT_PARAMS_T {
	uint16_t u2ReqType;	/* Request Type */
	uint16_t u2TWT;	/* Target Wake Time 16 bits */
	uint8_t ucMinWakeDur;	/* Nominal Minimum TWT Wake Duration */
	uint16_t u2WakeIntvalMantiss;	/* TWT Wake Interval Mantissa */
	uint16_t u2BTWTInfo;	/* BTWT Info */
	uint8_t uc_arRTWTTrafficInfo[0];  /* optional RTWT traffic info */
} __KAL_ATTRIB_PACKED__;

#endif

#if (CFG_SUPPORT_802_11BE_ML_TWT == 1)
/* 11be ML-TWT element */
__KAL_ATTRIB_PACKED_FRONT__
struct IE_ML_TWT_T {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t ucCtrl;	/* Control */
	uint16_t u2ReqType;	/* Request Type */
	uint64_t u8TWT;	/* Target Wake Time 64 bits */
	uint8_t ucMinWakeDur;	/* Nominal Minimum TWT Wake Duration */
	uint16_t u2WakeIntvalMantiss;	/* TWT Wake Interval Mantissa */
	uint8_t ucReserved;	/* TWT Channel for 11ah. Reserved for 11ax */
	uint16_t u2LinkIdBitmap; /* Link ID bitmap */
} __KAL_ATTRIB_PACKED__;
#endif

/* 3 7.4 Action Frame. */
/* 7.4 Action frame format */
__KAL_ATTRIB_PACKED_FRONT__
struct WLAN_ACTION_FRAME {
	/* Action MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2DurationID;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* Action frame body */
	uint8_t ucCategory;	/* Category */
	uint8_t ucAction;	/* Action Value */
	uint8_t ucActionDetails[];	/* Action details */
} __KAL_ATTRIB_PACKED__;

/* public Action frame format */
__KAL_ATTRIB_PACKED_FRONT__
struct WLAN_PUBLIC_VENDOR_ACTION_FRAME {
	/* Action MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2DurationID;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* Action frame body */
	uint8_t ucCategory;	/* Category: should be 0x4 */
	uint8_t ucAction;	/* Action Value: should be 0x9 */
	uint8_t ucOUI[3];
	uint8_t ucSubType;
	uint8_t ucPubSubType;
} __KAL_ATTRIB_PACKED__;


/* 802.11-2020 9.6.2 Figure 9-854 Spectrum Measurement Request frame
 * The Measurement Request Element field contains one or more of the
 * Measurement Request elements
 */
__KAL_ATTRIB_PACKED_FRONT__
struct ACTION_SM_REQ_FRAME {
	/* ADDTS Request MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2Duration;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* ADDTS Request frame body */
	uint8_t ucCategory;	/* Category */
	uint8_t ucAction;	/* Action Value */
	uint8_t ucDialogToken;	/* Dialog Token */
	uint8_t aucInfoElem[];	/* Information elements  */
} __KAL_ATTRIB_PACKED__;

/* 7.4.1.2 Spectrum Measurement Report frame format */

/* 7.4.1.3 Spectrum TPC Request frame format */
__KAL_ATTRIB_PACKED_FRONT__
struct ACTION_TPC_REQ_FRAME {
	/* ADDTS Request MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2Duration;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* ADDTS Request frame body */
	uint8_t ucCategory;	/* Category */
	uint8_t ucAction;	/* Action Value */
	uint8_t ucDialogToken;	/* Dialog Token */
	uint8_t ucElemId;	/* Element ID */
	uint8_t ucLength;	/* Length */
} __KAL_ATTRIB_PACKED__;

/* 7.4.1.4 Spectrum TPC Report frame format */
__KAL_ATTRIB_PACKED_FRONT__
struct ACTION_TPC_REPORT_FRAME {
	/* ADDTS Request MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2Duration;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* ADDTS Request frame body */
	uint8_t ucCategory;	/* Category */
	uint8_t ucAction;	/* Action Value */
	uint8_t ucDialogToken;	/* Dialog Token */
	uint8_t ucElemId;	/* Element ID */
	uint8_t ucLength;	/* Length */
	uint8_t ucTransPwr;	/* Transmit Power */
	uint8_t ucLinkMargin;	/* Link Margin */
} __KAL_ATTRIB_PACKED__;

/* 7.4.1.5 Channel Switch Announcement frame format */
/* 802.11-2020 9.6.2.6 Channel Switch Announcement frame */
__KAL_ATTRIB_PACKED_FRONT__
struct ACTION_CHANNEL_SWITCH_FRAME {
	/* ADDTS Request MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2Duration;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* ADDTS Request frame body */
	uint8_t ucCategory;	/* Category */
	uint8_t ucAction;	/* Action Value */
	uint8_t aucInfoElem[]; /* Information elements */
} __KAL_ATTRIB_PACKED__;

/* 802.11-2020 9.6.7.7 Fig. 9-871 Extended Channel Switch Announcement frame */
__KAL_ATTRIB_PACKED_FRONT__
struct ACTION_EX_CHANNEL_SWITCH_FRAME {
	/* MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2Duration;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* Frame body */
	uint8_t ucCategory;	/* Category */
	uint8_t ucAction;	/* Action Value */
	uint8_t ucChannelSwitchMode;
	uint8_t ucNewOperatingClass;
	uint8_t ucNewChannelNum;
	uint8_t ucChannelSwitchCount;
	uint8_t aucInfoElem[]; /* Information elements */
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct ACTION_MSCS_REQ_FRAME {
	/* Action MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2DurationID;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* Action frame body */
	uint8_t ucCategory;	/* Category: 19 */
	/* Robust Action Value: [4]: MSCS Request [5]: MSCS Response */
	uint8_t ucAction;
	uint8_t ucDialogToken;	/* Dialog Token */
	uint8_t aucMSCSDesc[]; /* MSCS Descriptor Element */
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct ACTION_MSCS_RSP_FRAME {
	/* Action MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2DurationID;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* Action frame body */
	uint8_t ucCategory;	/* Category: 19 */
	/* Robust Action Value: [4]: MSCS Request [5]: MSCS Response */
	uint8_t ucAction;
	uint8_t ucDialogToken;	/* Dialog Token */
	uint16_t u2StatusCode;	/* Status code */
	uint8_t aucMSCSDesc[]; /* MSCS Descriptor Element */
} __KAL_ATTRIB_PACKED__;

/* BE D3.0 9.6.35.5 EPCS Priority Access Enable Request frame format
 * Table 9-623g - EPCS Priority Access Enable Request frame Action field format
 */
__KAL_ATTRIB_PACKED_FRONT__
struct ACTION_EPCS_REQ_FRAME {
	/* Action MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2DurationID;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* Action frame body */
	uint8_t ucCategory;	/* Category: 37 protected EHT */
	/* Protected EHT Action Value: [3]: EPCS Request */
	uint8_t ucAction;
	uint8_t ucDialogToken;	/* Dialog Token */
	uint8_t aucMultiLink[]; /* Priority Access Element */
} __KAL_ATTRIB_PACKED__;

/* BE D3.0 9.6.35.6 EPCS Priority Access Enable Response frame format
 * Table 9-623h - EPCS Priority Access Enable Response frame Action field format
 */
__KAL_ATTRIB_PACKED_FRONT__
struct ACTION_EPCS_RSP_FRAME {
	/* Action MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2DurationID;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* Action frame body */
	uint8_t ucCategory;	/* Category: 37 protected EHT */
	/* Protected EHT Action Value: [4]: EPCS Response  */
	uint8_t ucAction;
	uint8_t ucDialogToken;	/* Dialog Token */
	uint16_t u2StatusCode;	/* Status code */
	uint8_t aucMultiLink[]; /* Priority Access Element */
} __KAL_ATTRIB_PACKED__;

/* 7.4.2.1 ADDTS Request frame format */
__KAL_ATTRIB_PACKED_FRONT__
struct ACTION_ADDTS_REQ_FRAME {
	/* ADDTS Request MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2Duration;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* ADDTS Request frame body */
	uint8_t ucCategory;	/* Category */
	uint8_t ucAction;	/* Action Value */
	uint8_t ucDialogToken;	/* Dialog Token */
	uint8_t aucInfoElem[1];	/* Information elements, such as */
				/* TS Delay, and etc. */
} __KAL_ATTRIB_PACKED__;

/* 7.4.2.2 ADDTS Response frame format */
__KAL_ATTRIB_PACKED_FRONT__
struct ACTION_ADDTS_RSP_FRAME {
	/* ADDTS Response MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2Duration;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* ADDTS Response frame body */
	uint8_t ucCategory;	/* Category */
	uint8_t ucAction;	/* Action Value */
	uint8_t ucDialogToken;	/* Dialog Token */
	uint8_t ucStatusCode;	/* WMM Status Code is of one byte */
	uint8_t aucInfoElem[1];	/* Information elements, such as */
				/* TS Delay, and etc. */
} __KAL_ATTRIB_PACKED__;

/* 7.4.2.3 DELTS frame format */
__KAL_ATTRIB_PACKED_FRONT__
struct ACTION_DELTS_FRAME {
	/* DELTS MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2DurationID;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* DELTS frame body */
	uint8_t ucCategory;	/* Category */
	uint8_t ucAction;	/* Action Value */
	uint8_t aucTsInfo[3];	/* TS Info */
} __KAL_ATTRIB_PACKED__;

/* 7.4.2.3 QOSMAPSET CONFIGURATE frame format */
/* 802.11-2020 9.6.3.6 Table 9-356 QoS Map Configure frame
 * QoS Map element (located at qosMapSet) is defined in 9.4.2.94,
 * Element ID(1) 110, Length(1), DSCP Exception List (nx2), UP0(2), ... UP7(2)
 */
struct _ACTION_QOS_MAP_CONFIGURE_FRAME {
	/* QOSMAP CONFIGURE MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2DurationID;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* QoS Map element body */
	uint8_t ucCategory;	/* Category */
	uint8_t ucAction;	/* Action Value */
	uint8_t qosMapSet[];	/* qosmapset IE */
};

/* 7.4.4.1 ADDBA Request frame format */
__KAL_ATTRIB_PACKED_FRONT__
struct ACTION_ADDBA_REQ_FRAME {
	/* Action MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2DurationID;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* Action frame body */
	uint8_t ucCategory;	/* Category */
	uint8_t ucAction;	/* Action Value */
	uint8_t ucDialogToken;	/* Dialog Token chosen by the sender */
	uint8_t aucBAParameterSet[2];	/* BA policy, TID, buffer size */
	uint8_t aucBATimeoutValue[2];
	uint8_t aucBAStartSeqCtrl[2];	/* SSN */
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct ACTION_ADDBA_REQ_BODY {
	uint16_t u2BAParameterSet;	/* BA policy, TID, buffer size */
	uint16_t u2BATimeoutValue;
	uint16_t u2BAStartSeqCtrl;	/* SSN */
} __KAL_ATTRIB_PACKED__;

/* 7.4.4.2 ADDBA Response frame format */
__KAL_ATTRIB_PACKED_FRONT__
struct ACTION_ADDBA_RSP_FRAME {
	/* Action MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2DurationID;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* Action frame body */
	uint8_t ucCategory;	/* Category */
	uint8_t ucAction;	/* Action Value */
	uint8_t ucDialogToken;	/* Dialog Token chosen by the sender */
	uint8_t aucStatusCode[2];
	uint8_t aucBAParameterSet[2];	/* BA policy, TID, buffer size */
	uint8_t aucBATimeoutValue[2];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct ACTION_ADDBA_RSP_BODY {
	uint16_t u2StatusCode;
	uint16_t u2BAParameterSet;	/* BA policy, TID, buffer size */
	uint16_t u2BATimeoutValue;
} __KAL_ATTRIB_PACKED__;

/* 7.4.4.3 DELBA frame format */
__KAL_ATTRIB_PACKED_FRONT__
struct ACTION_DELBA_FRAME {
	/* Action MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2DurationID;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* Action frame body */
	uint8_t ucCategory;	/* Category */
	uint8_t ucAction;	/* Action Value */
	uint16_t u2DelBaParameterSet;	/* Bit 11 Initiator, Bits 12-15 TID */
	uint16_t u2ReasonCode;	/* 7.3.1.7 */
} __KAL_ATTRIB_PACKED__;

#if CFG_SUPPORT_NCHO
/* 7.4.5.1 vendor-specific frame format */
struct _ACTION_VENDOR_SPEC_FRAME_T {
	/* Action MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2Duration;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	uint8_t aucElemInfo[];	/* Pointer to frame data */
};
#endif

/* 802.11-2020 9.6.6.2 Figure 9-860 Radio Measurement Request frame
 * The Measurement Request Elements fields contains zero or more Measurement
 * Request elements.
 */
__KAL_ATTRIB_PACKED_FRONT__
struct ACTION_RM_REQ_FRAME {
	/* MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2Duration;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* Radio Measurement Request frame body */
	uint8_t ucCategory;	/* Category */
	uint8_t ucAction;	/* Action Value */
	uint8_t ucDialogToken;	/* Dialog Token */
	uint16_t u2Repetitions;	/* Number of repetitions */
	uint8_t aucInfoElem[];	/* Measurement Request elements, such as */
				/* channel load request, and etc. */
} __KAL_ATTRIB_PACKED__;

/* 802.11-2020 9.6.6.3 Figure 9-861 Radio Measurement Response frame
 * The Measurement Report Elements fields contains one or more Measurement
 * Report.
 */
__KAL_ATTRIB_PACKED_FRONT__
struct ACTION_RM_REPORT_FRAME {
	/* MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2Duration;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* Radio Measurement Report frame body */
	uint8_t ucCategory;	/* Category */
	uint8_t ucAction;	/* Action Value */
	uint8_t ucDialogToken;	/* Dialog Token */
	uint8_t aucInfoElem[];	/* Measurement Report elements, such as */
				/* channel load report, and etc. */
} __KAL_ATTRIB_PACKED__;

/* 7.4.7.1a 20/40 BSS Coexistence Management frame format */
__KAL_ATTRIB_PACKED_FRONT__
struct ACTION_20_40_COEXIST_FRAME {
	/* MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2Duration;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* BSS Coexistence Management frame body */
	uint8_t ucCategory;	/* Category */
	uint8_t ucAction;	/* Action Value */

	/* 20/40 BSS coexistence element */
	struct IE_20_40_COEXIST rBssCoexist;
	/* Intolerant channel report */
	struct IE_INTOLERANT_CHNL_REPORT rChnlReport;

} __KAL_ATTRIB_PACKED__;

#if CFG_SUPPORT_802_11W
/* 7.4.9 SA Query Management frame format */
__KAL_ATTRIB_PACKED_FRONT__
struct ACTION_SA_QUERY_FRAME {
	/* MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2Duration;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* BSS Coexistence Management frame body */
	uint8_t ucCategory;	/* Category */
	uint8_t ucAction;	/* Action Value */

	uint8_t ucTransId[ACTION_SA_QUERY_TR_ID_LEN];	/* Transaction id */

} __KAL_ATTRIB_PACKED__;
#endif

/* 7.4.10 Notify Channel Width Management frame format */
__KAL_ATTRIB_PACKED_FRONT__
struct ACTION_NOTIFY_CHNL_WIDTH_FRAME {
	/* MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2Duration;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* BSS Coexistence Management frame body */
	uint8_t ucCategory;	/* Category */
	uint8_t ucAction;	/* Action Value */
	uint8_t ucChannelWidth;	/* Channel Width */
} __KAL_ATTRIB_PACKED__;

/* 802.11v Wireless Network Management: Timing Measurement Request */
__KAL_ATTRIB_PACKED_FRONT__
struct ACTION_WNM_TIMING_MEAS_REQ_FRAME {
	/* MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2Duration;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* Timing Measurement Request Management frame body */
	uint8_t ucCategory;	/* Category */
	uint8_t ucAction;	/* Action Value */
	uint8_t ucTrigger;	/* Trigger */
} __KAL_ATTRIB_PACKED__;

/* 802.11v Wireless Network Management: Timing Measurement */
__KAL_ATTRIB_PACKED_FRONT__
struct ACTION_UNPROTECTED_WNM_TIMING_MEAS_FRAME {
	/* MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2Duration;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* Timing Measurement Management frame body */
	uint8_t ucCategory;	/* Category */
	uint8_t ucAction;	/* Action Value */
	uint8_t ucDialogToken;	/* Dialog Token */
	uint8_t ucFollowUpDialogToken;	/* Follow Up Dialog Token */
	uint32_t u4ToD;		/* Timestamp of Departure [10ns] */
	uint32_t u4ToA;		/* Timestamp of Arrival [10ns] */
	uint8_t ucMaxToDErr;	/* Maximum of ToD Error [10ns] */
	uint8_t ucMaxToAErr;	/* Maximum of ToA Error [10ns] */
} __KAL_ATTRIB_PACKED__;

struct IE_WFA_OSEN {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t aucOui[3];
	uint8_t ucOuiType;
	uint32_t u4GroupKeyCipherSuite;
	uint16_t u2PairwiseKeyCipherSuiteCount;
	uint8_t aucPairwiseKeyCipherSuite1[4];
};

/* 8.5.23.4 Operating Mode Notification frame format */
__KAL_ATTRIB_PACKED_FRONT__
struct ACTION_OP_MODE_NOTIFICATION_FRAME {
	/* MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2Duration;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* Operating Mode Notification frame body */
	uint8_t ucCategory;	/* Category */
	uint8_t ucAction;	/* Action Value */
	uint8_t ucOperatingMode;	/* Operating Mode */
} __KAL_ATTRIB_PACKED__;

/* 8.5.12.3 SM Power Save frame format */
__KAL_ATTRIB_PACKED_FRONT__
struct ACTION_SM_POWER_SAVE_FRAME {
	/* MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2Duration;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* SM power save frame body */
	uint8_t ucCategory;	/* Category */
	uint8_t ucAction;	/* Action Value */
	uint8_t ucSmPowerCtrl;	/* SM Power Control (see 8.4.1.22) */
} __KAL_ATTRIB_PACKED__;

/* 8.5.12.2 Notify Channel Width frame format */
__KAL_ATTRIB_PACKED_FRONT__
struct ACTION_NOTIFY_CHANNEL_WIDTH_FRAME {
	/* MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2Duration;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* SM power save frame body */
	uint8_t ucCategory;	/* Category */
	uint8_t ucAction;	/* Action Value */
	uint8_t ucChannelWidth;	/* Channel Width (see 8.4.1.21) */
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct ACTION_FT_REQ_ACTION_FRAME {
	/* MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2Duration;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* FT request frame body */
	uint8_t ucCategory;	/* Category */
	uint8_t ucAction;	/* Action Value */
	uint8_t aucStaAddr[MAC_ADDR_LEN];
	uint8_t aucTargetApAddr[MAC_ADDR_LEN];
	uint8_t aucInfoElem[];	/* Various IEs, possible no. */
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct ACTION_FT_RESP_ACTION_FRAME {
	/* MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2Duration;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* FT response frame body */
	uint8_t ucCategory;	/* Category */
	uint8_t ucAction;	/* Action Value */
	uint8_t aucStaAddr[MAC_ADDR_LEN];
	uint8_t aucTargetApAddr[MAC_ADDR_LEN];
	uint16_t u2StatusCode;
	uint8_t aucInfoElem[];	/* Various IEs, possible no. */
} __KAL_ATTRIB_PACKED__;

#if (CFG_SUPPORT_TWT == 1)
/* 11ax TWT Setup frame format */
__KAL_ATTRIB_PACKED_FRONT__
struct _ACTION_TWT_SETUP_FRAME {
	/* MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2Duration;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* TWT Setup frame body */
	uint8_t ucCategory;	/* Category */
	uint8_t ucAction;	/* Action Value */
	uint8_t ucDialogToken;	/* Dialog Token */
	struct _IE_TWT_T rTWT;	/* TWT element */
} __KAL_ATTRIB_PACKED__;

/* 11ax TWT Teardown frame format */
__KAL_ATTRIB_PACKED_FRONT__
struct _ACTION_TWT_TEARDOWN_FRAME {
	/* MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2Duration;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* TWT Teardown frame body */
	uint8_t ucCategory;	/* Category */
	uint8_t ucAction;	/* Action Value */
	uint8_t ucTWTFlow;	/* TWT Flow */
} __KAL_ATTRIB_PACKED__;

/* 11ax TWT Information frame format */
/* 802.11-2020 9.6.24.12 Table 9-505 TWT Information frame
 * TWT Information 9.4.1.60, B0-B7 are fixed fields
 * B8-Bn, Next TWT, are variable in length of 0, 32, 48, or 64 bits.
 */
__KAL_ATTRIB_PACKED_FRONT__
struct _ACTION_TWT_INFO_FRAME {
	/* MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2Duration;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* TWT Information frame body */
	uint8_t ucCategory;	/* Category */
	uint8_t ucAction;	/* Action Value */
	uint8_t ucNextTWTCtrl;
	uint8_t aucNextTWT[];
} __KAL_ATTRIB_PACKED__;
#endif

#if (CFG_SUPPORT_BTWT == 1)
/* 11ax BTWT Setup frame format */
__KAL_ATTRIB_PACKED_FRONT__
struct _ACTION_BTWT_SETUP_FRAME {
	/* MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2Duration;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* TWT Setup frame body */
	uint8_t ucCategory;	/* Category */
	uint8_t ucAction;	/* Action Value */
	uint8_t ucDialogToken;	/* Dialog Token */
	struct _IE_BTWT_T rTWT;	/* BTWT element */
} __KAL_ATTRIB_PACKED__;
#endif

#if (CFG_SUPPORT_RTWT == 1)
/* 11be RTWT Setup frame format */
__KAL_ATTRIB_PACKED_FRONT__
struct _ACTION_RTWT_SETUP_FRAME {
	/* MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2Duration;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* TWT Setup frame body */
	uint8_t ucCategory;	/* Category */
	uint8_t ucAction;	/* Action Value */
	uint8_t ucDialogToken;	/* Dialog Token */
	struct _IE_RTWT_T rTWT;	/* RTWT element */
} __KAL_ATTRIB_PACKED__;
#endif

#if (CFG_SUPPORT_802_11BE_ML_TWT == 1)
/* 11be ML-TWT Setup frame format */
__KAL_ATTRIB_PACKED_FRONT__
struct _ACTION_ML_TWT_SETUP_FRAME {
	/* MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2Duration;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* TWT Setup frame body */
	uint8_t ucCategory;	/* Category */
	uint8_t ucAction;	/* Action Value */
	uint8_t ucDialogToken;	/* Dialog Token */
	struct IE_ML_TWT_T rTWT;	/* TWT element */
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct _ACTION_ML_TWT_SETUP_FRAME_PER_LINK_DISTINCT {
	/* MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2Duration;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* TWT Setup frame body */
	uint8_t ucCategory;	/* Category */
	uint8_t ucAction;	/* Action Value */
	uint8_t ucDialogToken;	/* Dialog Token */
	uint8_t rTWT[];	/* TWT element */
} __KAL_ATTRIB_PACKED__;
#endif

/* 3 Information Elements from WFA. */
__KAL_ATTRIB_PACKED_FRONT__
struct IE_WFA {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t aucOui[3];
	uint8_t ucOuiType;
	uint8_t aucOuiSubTypeVersion[2];
	/*!< Please be noted. WPA defines a 16 bit field version */
	/* instead of one subtype field and one version field */
} __KAL_ATTRIB_PACKED__;

#if CFG_SUPPORT_PASSPOINT
/* HS20 3.1 - HS 2.0 Indication Information Element */
__KAL_ATTRIB_PACKED_FRONT__
struct IE_HS20_INDICATION {
	uint8_t ucId;		/* Element ID */
	uint8_t ucLength;	/* Length */
	uint8_t aucOui[3];	/* OUI */
	uint8_t ucType;		/* Type */
	uint8_t ucHotspotConfig;	/* Hotspot Configuration */
} __KAL_ATTRIB_PACKED__;
#endif /* CFG_SUPPORT_PASSPOINT */

/* WAPI Information element format */
__KAL_ATTRIB_PACKED_FRONT__
struct WAPI_INFO_ELEM {
	uint8_t ucElemId;
	uint8_t ucLength;
	uint16_t u2Version;
	uint16_t u2AKMSuiteCount;
	uint32_t u4AKMSuite;
	uint16_t u2PairSuiteCount;
	uint32_t u4PairSuite;
	uint32_t u4GroupSuite;
	uint16_t u2WapiCap;
} __KAL_ATTRIB_PACKED__;

/* Information Elements from MTK Synergies.*/
__KAL_ATTRIB_PACKED_FRONT__
struct IE_MTK_OUI {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t aucOui[3];
	uint8_t aucCapability[4];
	uint8_t aucInfoElem[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct IE_MTK_MLR {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t ucLRBitMap;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct IE_MTK_PRE_WIFI7 {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t ucVersion0;
	uint8_t ucVersion1;
	uint8_t aucInfoElem[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct IE_MTK_CHIP_CAP {
	uint8_t ucId;
	uint8_t ucLength;
	uint64_t u8ChipCap;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct SUB_IE_BSS_TERM_DURATION {
	uint8_t ucSubId;
	uint8_t ucLength;
	uint8_t aucTermTsf[8];
	uint16_t u2Duration;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct SESSION_INFO_URL {
	uint8_t ucURLLength;
	uint8_t aucURL[255];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct SUB_IE_BSS_CAND_PREFERENCE {
	uint8_t ucSubId;
	uint8_t ucLength;
	uint8_t ucPreference;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct ACTION_BTM_QUERY_FRAME {
	/* MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2Duration; /* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* BSS Coexistence Management frame body */
	uint8_t ucCategory;	/* Category */
	uint8_t ucAction;	/* Action Value */

	uint8_t ucDialogToken;
	uint8_t ucQueryReason;
	uint8_t *pucNeighborBss;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct ACTION_BTM_REQ_FRAME {
	/* MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2Duration; /* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* BSS Coexistence Management frame body */
	uint8_t ucCategory;	/* Category */
	uint8_t ucAction;	/* Action Value */

	uint8_t ucDialogToken;
	uint8_t ucRequestMode;
	uint16_t u2DisassocTimer;
	uint8_t ucValidityInterval;
	uint8_t aucOptInfo[];
	/* Optional: Bss Termination Duration(0~12 bytes),
	** Session Information URL, Bss Transition Candidate List
	*/
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct ACTION_BTM_RSP_FRAME {
	/* MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2Duration; /* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* BSS Coexistence Management frame body */
	uint8_t ucCategory;	/* Category */
	uint8_t ucAction;	/* Action Value */

	uint8_t ucDialogToken;
	uint8_t ucStatusCode;
	uint8_t ucBssTermDelay;
	uint8_t aucOptInfo[];
	/* Optional Target BSSID and Transition Candidate Entry list */
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct IE_MOBILITY_DOMAIN {
	uint8_t ucId; /* Element ID = 54 */
	uint8_t ucLength; /* Length is 3 */
	uint16_t u2MDID;
	/* Bit 0: FT over DS; Bit 1: Resource Request Protocol Capbility,
	** others: reserved
	*/
	uint8_t ucBitMap;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct IE_FAST_TRANSITION {
	uint8_t ucId; /* Element ID = 55 */
	uint8_t ucLength; /* Length is variable */
	/* Bit 0 ~ Bit 7: reserved; Bit 8 ~ Bit 15: IE count
	** used to calculate MIC, 0 means No MIC
	*/
	uint8_t ucMicCtrl;
	uint8_t aucMic[16]; /*  */
	uint8_t aucANonce[32]; /* Nonce of R1KH */
	uint8_t aucSNonce[32]; /* Nonce of S1KH */
	uint8_t aucOptParam[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct SUB_IE_FAST_TRANSITION {
	uint8_t ucSubId;  /* 0, 4-255: reserved; 1: R1KH-ID; 2: GTK; 3: R0KH-ID
			     */
	uint8_t ucLength; /* bytes, R1KH-ID: 6; GTK: 15-42; R0KH-ID: 1-48 */
	uint8_t aucData[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct SUB_IE_GTK {
	uint8_t ucSubId; /*  subId=2 */
	uint8_t ucLength; /*  length is 15-42 */
	uint16_t u2KeyInfo; /* Bit0-Bit1: Key ID; Bit2-Bit15: reserved */
	uint8_t ucKeyLength;
	uint8_t aucRsc[8];
	uint8_t aucKey[5];
} __KAL_ATTRIB_PACKED__;

/* 8.5.7.4 Link Measurement Request frame format */
__KAL_ATTRIB_PACKED_FRONT__
struct ACTION_LM_REQUEST_FRAME {
	/* Link Measurement Request MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2Duration;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* Link Measurement Request frame body */
	uint8_t ucCategory;	/* Category */
	uint8_t ucAction;	/* Action Value */
	uint8_t ucDialogToken;	/* Dialog Token */
	uint8_t ucTxPowerUsed;	/*  */
	uint8_t ucTxPowerMax;	/*  */
	uint8_t aucInfoElem[];	/* subelements */
} __KAL_ATTRIB_PACKED__;

/* 8.5.7.5 Link Measurement Report frame format */
__KAL_ATTRIB_PACKED_FRONT__
struct ACTION_LM_REPORT_FRAME {
	/* Link Measurement Report MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2Duration;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* Link Measurement Report frame body */
	uint8_t ucCategory;	/* Category */
	uint8_t ucAction;	/* Action Value */
	uint8_t ucDialogToken;	/* Dialog Token */
	uint8_t aucTpcReportIE[4];	/*   */
	uint8_t ucRxAntennaID;
	uint8_t ucTxAntennaID;
	uint8_t ucRCPI;
	uint8_t ucRSNI;
	uint8_t aucInfoElem[];	/* subelements */
} __KAL_ATTRIB_PACKED__;

#if (CFG_SUPPORT_802_11BE_T2LM == 1)
__KAL_ATTRIB_PACKED_FRONT__
struct ACTION_T2LM_REQ_FRAME {
	/* DELTS MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2DurationID;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	 /* Sequence Control */
	/* Action frame body */
	uint8_t ucCategory;	/* Category: 37 protected EHT */
	/* Protected EHT Action Value: [0]: T2LM Request */
	uint8_t ucAction;
	uint8_t ucDialogToken;	/* Dialog Token */
	uint8_t aucT2LM[];	/* TID-TO-LINK Mapping Element */
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct ACTION_T2LM_RSP_FRAME {
	/* DELTS MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2DurationID;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	 /* Sequence Control */
	/* Action frame body */
	uint8_t ucCategory;	/* Category: 37 protected EHT */
	/* Protected EHT Action Value: [1]: T2LM Response */
	uint8_t ucAction;
	uint8_t ucDialogToken;	/* Dialog Token */
	uint8_t ucStatusCode;	/* Status Code*/
	uint8_t aucT2LM[];	/* TID-TO-LINK Mapping Element */
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct ACTION_T2LM_TEARDOWN_FRAME {
	/* DELTS MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2DurationID;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	 /* Sequence Control */
	/* Action frame body */
	uint8_t ucCategory;	/* Category: 37 protected EHT */
	/* Protected EHT Action Value: [2]: T2LM Teardown */
	uint8_t ucAction;
} __KAL_ATTRIB_PACKED__;
#endif

__KAL_ATTRIB_PACKED_FRONT__
struct IE_REQUEST {
	uint8_t ucId; /* ELEM_ID_REQUEST */
	uint8_t ucLength; /* 0 to 237 */
	uint8_t aucReqIds[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct IE_AP_CHNL_REPORT {
	uint8_t ucId; /* ELEM_ID_AP_CHANNEL_REPORT */
	uint8_t ucLength; /* 1 to 237 */
	uint8_t ucOpClass;
	uint8_t aucChnlList[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct SUB_IE_REPORTING_DETAIL {
	uint8_t ucSubID; /* 2 */
	uint8_t ucLength;
	/* 0: No fixed length fields or elemets. 1: all fixed length fields and
	** requested IEs 2:
	*/
	uint8_t ucDetailValue;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct IE_TSPEC_BODY {
	uint8_t ucId;                   /* ELEM_ID_TSPEC */
	uint8_t ucLength;
	uint8_t	aucTsInfo[3];           /* TS info field */
	uint16_t u2NominalMSDUSize;     /* nominal MSDU size */
	uint16_t u2MaxMSDUsize;         /* maximum MSDU size */
	uint32_t u4MinSvcIntv;          /* minimum service interval */
	uint32_t u4MaxSvcIntv;          /* maximum service interval */
	uint32_t u4InactIntv;           /* inactivity interval */
	uint32_t u4SpsIntv;             /* suspension interval */
	uint32_t u4SvcStartTime;        /* service start time */
	uint32_t u4MinDataRate;         /* minimum Data rate */
	uint32_t u4MeanDataRate;        /* mean data rate */
	uint32_t u4PeakDataRate;        /* peak data rate */
	uint32_t u4MaxBurstSize;        /* maximum burst size */
	uint32_t u4DelayBound;          /* delay bound */
	uint32_t u4MinPHYRate;          /* minimum PHY rate */
	uint16_t u2Sba;                 /* surplus bandwidth allowance */
	uint16_t u2MediumTime;          /* medium time */
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct WMM_ACTION_TSPEC_FRAME {
	/* DELTS MAC header */
	uint16_t     u2FrameCtrl;                /* Frame Control */
	uint16_t     u2DurationID;               /* Duration */
	uint8_t      aucDestAddr[MAC_ADDR_LEN];  /* DA */
	uint8_t      aucSrcAddr[MAC_ADDR_LEN];   /* SA */
	uint8_t      aucBSSID[MAC_ADDR_LEN];     /* BSSID */
	uint16_t     u2SeqCtrl;                  /* Sequence Control */
	/* DELTS frame body */
	uint8_t      ucCategory;                 /* Category, value is 17  */
	uint8_t      ucAction;   /* Action Value, value: 2, delts */
	uint8_t      ucDlgToken;
	uint8_t      ucStatusCode;
	uint8_t      aucInfoElem[];
} __KAL_ATTRIB_PACKED__;

/* 9.4.2.46 Multiple BSSID element */
__KAL_ATTRIB_PACKED_FRONT__
struct IE_MBSSID {
	uint8_t      ucId;
	uint8_t      ucLength;
	uint8_t      ucMaxBSSIDIndicator;
	uint8_t      ucSubelements[];
} __KAL_ATTRIB_PACKED__;

/* 9.4.2.74 Multiple BSSID-Index element */
__KAL_ATTRIB_PACKED_FRONT__
struct IE_MBSSID_INDEX {
	uint8_t      ucId;
	uint8_t      ucLength;
	uint8_t      ucBSSIDIndex;
	uint8_t      ucDtimPeriod;
	uint8_t      ucDtimCount;
} __KAL_ATTRIB_PACKED__;

/* 9.4.2.72 Nontransmitted BSSID Capability element */
__KAL_ATTRIB_PACKED_FRONT__
struct IE_NON_TX_CAP {
	uint8_t      ucId;
	uint8_t      ucLength;
	uint16_t     u2Cap;
	uint8_t      ucDmgBssCntl;
	uint8_t      ucDmgCapElem[19];
} __KAL_ATTRIB_PACKED__;

/** 9.4.2.178 FILS Request Parameters element */
__KAL_ATTRIB_PACKED_FRONT__
struct IE_FILS_REQ_FRAME {
	uint8_t ucId;             /* Element ID */
	uint8_t ucLength;         /* Length */
	uint8_t ucExtId;          /* Element ID Extension */
	uint8_t ucCtrlBitMap;     /* Parameter Control Bitmap */
	uint8_t ucMaxChannelTime; /* Max Channel Time */
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct IE_OCE_SUPPRESSION_BSSID {
	uint8_t ucAttrId;		/* Attribute ID */
	uint8_t ucAttrLength;		/* Attribute Length */
	uint8_t aucAttrBssIds[];	/* Suppression BSSIDs */
} __KAL_ATTRIB_PACKED__;

/* 9.4.2.170 Reduced Neighbor Report element */
__KAL_ATTRIB_PACKED_FRONT__
struct IE_RNR {
	uint8_t      ucId;
	uint8_t      ucLength;
	uint8_t      aucInfoField[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct NEIGHBOR_AP_INFO_FIELD {
	uint16_t     u2TbttInfoHdr;
	uint8_t      ucOpClass;
	uint8_t      ucChannelNum;
	uint8_t      aucTbttInfoSet[];
} __KAL_ATTRIB_PACKED__;

/* 9.4.2.260 Short SSID List element */
__KAL_ATTRIB_PACKED_FRONT__
struct IE_SHORT_SSID_LIST {
	uint8_t      ucId;
	uint8_t      ucLength;
	uint8_t      ucIdExt;
	uint8_t      aucShortSsidList[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct RSNX_INFO {
	uint8_t ucElemId;
	uint8_t ucLength;
	uint16_t u2Cap;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct RSNX_INFO_ELEM {
	uint8_t ucElemId;
	uint8_t ucLength;
	uint8_t aucCap[];
} __KAL_ATTRIB_PACKED__;

#if CFG_AP_80211K_SUPPORT
__KAL_ATTRIB_PACKED_FRONT__
struct SUB_IE_BEACON_REPORTING {
	uint8_t ucId; /* BCN_REQ_ELEM_SUBID_BEACON_REPORTING */
	uint8_t ucLength;
	uint8_t ucReportingCond;
	uint8_t ucReportingRef;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct SUB_IE_REQUEST {
	uint8_t ucId; /* BCN_REQ_ELEM_SUBID_REQUEST */
	uint8_t ucLength;
	uint8_t aucElems[]; /* requested element ids */
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct SUB_IE_AP_CHANNEL_REPORT {
	uint8_t ucId; /* BCN_REQ_ELEM_SUBID_AP_CHANNEL_REPORT */
	uint8_t ucLength;
	uint8_t ucOpClass;
	uint8_t aucElems[]; /* channel lists */
} __KAL_ATTRIB_PACKED__;

/* IEEE 802.11 2020
 * 9.4.2.160 Wide Bandwidth Channel Switch element
 * Figure 9-615 Wide Bandwidth Channel Switch element format
 */
__KAL_ATTRIB_PACKED_FRONT__
struct SUB_IE_WIDE_BW_CH_SWITCH {
	uint8_t ucId; /* BCN_REQ_ELEM_SUBID_WIDE_BW_CH_SWITCH */
	uint8_t ucLength;
	uint8_t ucNewChWidth;
	uint8_t aucNewChCenterFreq[2];
} __KAL_ATTRIB_PACKED__;
#endif /* CFG_AP_80211K_SUPPORT */

__KAL_ATTRIB_PACKED_FRONT__
struct IE_TCLAS_MASK {
	uint8_t      ucId;
	uint8_t      ucLength;
	uint8_t      ucIdExt;
	uint8_t      aucFrameClassifier[];  /* Frame Classifier Field */
} __KAL_ATTRIB_PACKED__;

/* Frame Classifier Field */
__KAL_ATTRIB_PACKED_FRONT__
struct IE_TCLAS_CLASS_TYPE_4 {
	uint8_t      ucClassifierType;
	uint8_t      ucClassifierMask;
	uint8_t      ucVersion;
	uint32_t     u4SrcIp;
	uint32_t     u4DestIp;
	uint16_t     u2SrcPort;
	uint16_t     u2DestPort;
	uint8_t      ucDSCP;
	uint8_t      ucProtocol;
	uint8_t      ucReserved;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct IE_MSCS_DESC {
	uint8_t      ucId;
	uint8_t      ucLength;
	uint8_t      ucIdExt;
	uint8_t      ucReqType;
	uint16_t     u2UserPriorityCtrl; /* bitmap: 4567, limit: 7 */
	uint32_t     u4StreamTimeout; /* 60s */
	uint8_t      aucData[]; /* TCLAS */
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct IE_PRIORITY_ACCESS_DESC {
	uint8_t      ucLength; /* Common Info Length */
	uint8_t      aucAPAddr[MAC_ADDR_LEN]; /* AP MLD MAC address */
	uint8_t      aucData[]; /* Link info */
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct ACTION_VENDOR_SPEC_PROTECTED_FRAME {
	/* MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2Duration;	/* Duration */
	uint8_t  aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t  aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t  aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* Vendor Specific Management frame body */
	uint8_t  ucCategory;	/* Category */
	uint8_t  aucOui[3];
	uint8_t  ucFastPathVersion;
	uint8_t  ucFastPathStatus;
	uint8_t  ucTransactionId;
	uint8_t  ucFastPathType;
	uint16_t u2RandomNum;
	uint16_t u2Mic;
	uint16_t u2KeyNum;
	uint16_t u2Reserved;
	uint32_t u4KeyBitmap[4];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct WLAN_DEAUTH_FRAME_WITH_MIC {
	/* Authentication MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2DurationID;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* Deauthentication frame body */
	uint16_t u2ReasonCode;	/* Reason code */
	uint8_t aucMicTag;
	uint8_t aucMicLen;
	uint16_t u2KeyId;
	uint8_t aucIPN[6];
	uint8_t aucMIC[16];
} __KAL_ATTRIB_PACKED__;

#if defined(WINDOWS_DDK) || defined(WINDOWS_CE)
#pragma pack()
#endif

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
/* Convert the ECWmin(max) to CWmin(max) */
#define ECW_TO_CW(_ECW)         ((1 << (_ECW)) - 1)

/* Convert the RCPI to dBm */
#define RCPI_TO_dBm(_rcpi)                          \
	((int32_t)(((_rcpi) > RCPI_HIGH_BOUND ? \
	RCPI_HIGH_BOUND : (_rcpi)) >> 1) - NDBM_LOW_BOUND_FOR_RCPI)

/* Convert the dBm to RCPI */
#define dBm_TO_RCPI(_dbm)                           \
	(uint8_t)(((((int32_t)(_dbm) + NDBM_LOW_BOUND_FOR_RCPI) << 1) \
	> RCPI_HIGH_BOUND) ? RCPI_HIGH_BOUND : \
	((((int32_t)(_dbm) + NDBM_LOW_BOUND_FOR_RCPI) << 1) \
	< RCPI_LOW_BOUND ? RCPI_LOW_BOUND : \
	(((int32_t)(_dbm) + NDBM_LOW_BOUND_FOR_RCPI) << 1)))

/* Convert an unsigned char pointer to an information element pointer */
#define IE_ID(fp)               (((struct IE_HDR *) fp)->ucId)
#define IE_LEN(fp)              (((struct IE_HDR *) fp)->ucLength)
#define IE_ID_EXT(fp)           (((struct IE_HDR *) fp)->aucInfo[0])
#define IE_DATA(fp)             (((struct IE_HDR *) fp)->aucInfo[0])
#define IE_SIZE(fp)             (ELEM_HDR_LEN + IE_LEN(fp))
#define IE_SIZE_MAX             257 /* 257 = ELEM_HDR_LEN + IE Body Len */
#define IE_TAIL(fp)             ((uint8_t *)fp + IE_SIZE(fp))

#define SSID_IE(fp)             ((struct IE_SSID *) fp)

#define SUP_RATES_IE(fp)        ((struct IE_SUPPORTED_RATE *) fp)

#define SUP_RATES_IOT_IE(fp)    ((struct IE_SUPPORTED_RATE_IOT *) fp)

#define DS_PARAM_IE(fp)         ((struct IE_DS_PARAM_SET *) fp)

#define TIM_IE(fp)              ((struct IE_TIM *) fp)

#define IBSS_PARAM_IE(fp)       ((struct IE_IBSS_PARAM_SET *) fp)

#define ERP_INFO_IE(fp)         ((struct IE_ERP *) fp)

#define EXT_SUP_RATES_IE(fp)    ((struct IE_EXT_SUPPORTED_RATE *) fp)

#define WFA_IE(fp)              ((struct IE_WFA *) fp)

#if CFG_SUPPORT_802_11D
#define COUNTRY_IE(fp)          ((struct IE_COUNTRY *) fp)
#endif

#define EXT_CAP_IE(fp)          ((struct IE_EXT_CAP *) fp)

#define HT_CAP_IE(fp)           ((struct IE_HT_CAP *) fp)

#define POWER_CAP_IE(fp)        ((struct IE_POWER_CAP *) fp)

#define SUP_CH_IE(fp)           ((struct IE_SUPPORTED_CHANNELS *) fp)

#define HT_OP_IE(fp)            ((struct IE_HT_OP *) fp)

#define VHT_CAP_IE(fp)           ((struct IE_VHT_CAP *) fp)

#define VHT_OP_IE(fp)            ((struct IE_VHT_OP *) fp)

#define OBSS_SCAN_PARAM_IE(fp)  ((struct IE_OBSS_SCAN_PARAM *) fp)

#define BSS_20_40_COEXIST_IE(fp) ((struct IE_20_40_COEXIST *) fp)

#define SUP_OPERATING_CLASS_IE(fp) ((struct IE_SUP_OPERATING_CLASS *) fp)

#define QUIET_IE(fp)            ((struct IE_QUIET *) fp)

#define MTK_OUI_IE(fp)          ((struct IE_MTK_OUI *) fp)
#define RNR_IE(fp)          ((struct IE_RNR *) fp)

#define CSA_IE(fp)		((struct IE_CHANNEL_SWITCH *) fp)
#define EX_CSA_IE(fp)		((struct IE_EX_CHANNEL_SWITCH *) fp)
#define SEC_OFFSET_IE(fp)	((struct IE_SECONDARY_OFFSET *) fp)
#define WIDE_BW_IE(fp)		((struct IE_WIDE_BAND_CHANNEL *) fp)
#define CSA_WRAPPER_IE(fp)	((struct IE_CHANNEL_SWITCH_WRAPPER *) fp)

#define SUPPORTED_CHANNELS_IE(fp) ((struct IE_SUPPORTED_CHANNELS *)fp)
#define TIMEOUT_INTERVAL_IE(fp)	((struct IE_TIMEOUT_INTERVAL *)fp)

#define SM_TPC_REQ_IE(fp) ((struct IE_TPC_REQ *) fp)
#define SM_TPC_REP_IE(fp) ((struct IE_TPC_REPORT *) fp)
#define SM_MEASUREMENT_REQ_IE(fp) ((struct IE_MEASUREMENT_REQ *) fp)
#define SM_MEASUREMENT_REP_IE(fp) ((struct IE_MEASUREMENT_REPORT *) fp)
#define SM_BASIC_REQ_IE(fp) ((struct SM_BASIC_REQ *) fp)

#define MBSSID_IE(fp)                 ((struct IE_MBSSID *) fp)
#define MBSSID_INDEX_IE(fp)           ((struct IE_MBSSID_INDEX *) fp)
#define NON_TX_CAP_IE(_fp)            ((struct IE_NON_TX_CAP *) _fp);
#define NON_INHERITANCE_IE(_fp)	      ((struct IE_NON_INHERITANCE *) _fp);

#define OCE_OUI_SUP_BSSID(fp)	((struct IE_OCE_SUPPRESSION_BSSID *) fp)
#define OCE_IE_OUI_TYPE(fp)	(((struct IE_MBO_OCE *)(fp))->ucOuiType)
#define OCE_IE_OUI(fp)		(((struct IE_MBO_OCE *)(fp))->aucOui)

/* The macro to copy the IPv4 address */
#define COPY_IP_ADDR(_pucDestAddr, _pucSrcAddr)    \
	kalMemCopy(_pucDestAddr, _pucSrcAddr, IPV4_ADDR_LEN)

/* The macro to check if the IP address is ALL ZERO */
#define IS_NONZERO_IP_ADDR(_pucIpAddr) \
	((_pucIpAddr)[0] || (_pucIpAddr)[1] || \
	 (_pucIpAddr)[2] || (_pucIpAddr)[3])

/* The macro to check if the MAC address is B/MCAST Address */
#define IS_BMCAST_MAC_ADDR(_pucDestAddr)            \
	((u_int8_t) (((uint8_t *)(_pucDestAddr))[0] & BIT(0)))

/* The macro to check if the MAC address is UCAST Address */
#define IS_UCAST_MAC_ADDR(_pucDestAddr)             \
	((u_int8_t) !(((uint8_t *)(_pucDestAddr))[0] & BIT(0)))

/* The macro to copy the MAC address */
#define COPY_MAC_ADDR(_pucDestAddr, _pucSrcAddr)    \
	kalMemCopy(_pucDestAddr, _pucSrcAddr, MAC_ADDR_LEN)

/* The macro to check if two MAC addresses are equal */
#define EQUAL_MAC_ADDR(_pucDestAddr, _pucSrcAddr)   \
	(!kalMemCmp(_pucDestAddr, _pucSrcAddr, MAC_ADDR_LEN))

/* The macro to check if two MAC addresses are not equal */
#define UNEQUAL_MAC_ADDR(_pucDestAddr, _pucSrcAddr) \
	(kalMemCmp(_pucDestAddr, _pucSrcAddr, MAC_ADDR_LEN))

/* The macro to check whether two SSIDs are equal */
#define EQUAL_SSID(pucSsid1, ucSsidLen1, pucSsid2, ucSsidLen2) \
	((ucSsidLen1 <= ELEM_MAX_LEN_SSID) && \
	(ucSsidLen2 <= ELEM_MAX_LEN_SSID) && \
	((ucSsidLen1) == (ucSsidLen2)) && \
	!kalMemCmp(pucSsid1, pucSsid2, ucSsidLen1))

/* The macro to check whether two SSIDs are equal */
#define UNEQUAL_SSID(pucSsid1, ucSsidLen1, pucSsid2, ucSsidLen2) \
	((ucSsidLen1 > ELEM_MAX_LEN_SSID) || \
	(ucSsidLen2 > ELEM_MAX_LEN_SSID) || \
	((ucSsidLen1) != (ucSsidLen2)) || \
	kalMemCmp(pucSsid1, pucSsid2, ucSsidLen1))

/* The macro to copy the SSID, the length of pucDestSsid
 * should have at least 32 bytes
 */
#define COPY_SSID(pucDestSsid, ucDestSsidLen, pucSrcSsid, ucSrcSsidLen) \
	do { \
		ucDestSsidLen = (ucSrcSsidLen > ELEM_MAX_LEN_SSID) \
			? ELEM_MAX_LEN_SSID : ucSrcSsidLen; \
		if (ucDestSsidLen) { \
			kalMemCopy(pucDestSsid, pucSrcSsid, ucDestSsidLen); \
		} \
	} while (FALSE)

/* The macro to copy the IE */
#define COPY_IE(pucDestIE, pucSrcIE) \
	do { \
		kalMemCopy((uint8_t *)pucDestIE, \
		(uint8_t *)pucSrcIE,\
		IE_SIZE(pucSrcIE)); \
	} while (FALSE)

#define IE_FOR_EACH(_pucIEsBuf, _u2IEsBufLen, _u2Offset) \
for ((_u2Offset) = 0U;	\
	((((_u2Offset) + 2U) <= (_u2IEsBufLen)) && \
	(((_u2Offset) + IE_SIZE(_pucIEsBuf)) <= (_u2IEsBufLen))); \
	(_u2Offset) += IE_SIZE(_pucIEsBuf), (_pucIEsBuf) += IE_SIZE(_pucIEsBuf))

#define SET_EXT_CAP(_aucField, _ucFieldLength, _ucBit) \
do { \
	if ((_ucBit) < ((_ucFieldLength) * 8)) { \
		uint8_t *aucExtCap = (uint8_t *)(_aucField); \
		((aucExtCap)[(_ucBit) / 8]) |= BIT((_ucBit) % 8); \
	} \
} while (FALSE)

#define CLEAR_EXT_CAP(_aucField, _ucFieldLength, _ucBit) \
do { \
	if ((_ucBit) < ((_ucFieldLength) * 8)) { \
		uint8_t *aucExtCap = (uint8_t *)(_aucField); \
		((aucExtCap)[(_ucBit) / 8]) &= ~BIT((_ucBit) % 8); \
	} \
} while (FALSE)

#define GET_EXT_CAP(_aucField, _ucFieldLength, _ucBit, _ucVal) \
do { \
	if ((_ucBit) < ((_ucFieldLength) * 8)) { \
		uint8_t *aucExtCap = (uint8_t *)(_aucField); \
		_ucVal = !!(((aucExtCap)[(_ucBit) / 8]) &= BIT((_ucBit) % 8)); \
	} \
} while (FALSE)
/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                               F U N C T I O N S
 *******************************************************************************
 */

#endif /* _MAC_H */
