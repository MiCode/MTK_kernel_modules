/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef __WF_DS_LWTBL_REGS_H__
#define __WF_DS_LWTBL_REGS_H__


#ifdef __cplusplus
extern "C" {
#endif

#ifndef REG_BASE_C_MODULE
// ----------------- WF_DS_LWTBL Bit Field Definitions -------------------

#define PACKING

typedef PACKING union
{
    PACKING struct
    {
        FIELD PEER_LINK_ADDRESS_47_32_  : 16;
        FIELD MUAR                      : 6;
        FIELD RCA1                      : 1;
        FIELD KID                       : 2;
        FIELD RCID                      : 1;
        FIELD BAND                      : 2;
        FIELD RV                        : 1;
        FIELD RCA2                      : 1;
        FIELD WPI_FLAG                  : 1;
        FIELD rsv_31                    : 1;
    } Bits;
    uint32_t Raw;
} WF_DS_LWTBL_REG_PEER_INFO_DW00, *PWF_DS_LWTBL_REG_PEER_INFO_DW00;

typedef PACKING union
{
    PACKING struct
    {
        FIELD PEER_LINK_ADDRESS_31_0_   : 32;
    } Bits;
    uint32_t Raw;
} WF_DS_LWTBL_REG_PEER_INFO_DW01, *PWF_DS_LWTBL_REG_PEER_INFO_DW01;

typedef PACKING union
{
    PACKING struct
    {
        FIELD AID                       : 12;
        FIELD GID_SU                    : 1;
        FIELD DUAL_PTEC_EN              : 1;
        FIELD DUAL_CTS_CAP              : 1;
        FIELD rsv_15                    : 1;
        FIELD CIPHER_SUIT_PGTK          : 5;
        FIELD FD                        : 1;
        FIELD TD                        : 1;
        FIELD SW                        : 1;
        FIELD UL                        : 1;
        FIELD TX_PS                     : 1;
        FIELD QOS                       : 1;
        FIELD HT                        : 1;
        FIELD VHT                       : 1;
        FIELD HE                        : 1;
        FIELD EHT                       : 1;
        FIELD MESH                      : 1;
    } Bits;
    uint32_t Raw;
} WF_DS_LWTBL_REG_PEER_CAP_DW02, *PWF_DS_LWTBL_REG_PEER_CAP_DW02;

typedef PACKING union
{
    PACKING struct
    {
        FIELD WMM_Q                     : 2;
        FIELD EHT_SIG_MCS               : 2;
        FIELD HDRT_MODE                 : 1;
        FIELD BEAM_CHG                  : 1;
        FIELD EHT_LTF_SYM_NUM_OPT       : 2;
        FIELD PFMU_IDX                  : 8;
        FIELD ULPF_IDX                  : 8;
        FIELD RIBF                      : 1;
        FIELD ULPF                      : 1;
        FIELD BYPASS_TXSMM              : 1;
        FIELD TBF_HT                    : 1;
        FIELD TBF_VHT                   : 1;
        FIELD TBF_HE                    : 1;
        FIELD TBF_EHT                   : 1;
        FIELD IGN_FBK                   : 1;
    } Bits;
    uint32_t Raw;
} WF_DS_LWTBL_REG_PEER_CAP_DW03, *PWF_DS_LWTBL_REG_PEER_CAP_DW03;

typedef PACKING union
{
    PACKING struct
    {
        FIELD NEGOTIATED_WINSIZE0       : 3;
        FIELD NEGOTIATED_WINSIZE1       : 3;
        FIELD NEGOTIATED_WINSIZE2       : 3;
        FIELD NEGOTIATED_WINSIZE3       : 3;
        FIELD NEGOTIATED_WINSIZE4       : 3;
        FIELD NEGOTIATED_WINSIZE5       : 3;
        FIELD NEGOTIATED_WINSIZE6       : 3;
        FIELD NEGOTIATED_WINSIZE7       : 3;
        FIELD PE                        : 2;
        FIELD DIS_RHTR                  : 1;
        FIELD LDPC_HT                   : 1;
        FIELD LDPC_VHT                  : 1;
        FIELD LDPC_HE                   : 1;
        FIELD LDPC_EHT                  : 1;
        FIELD BA_MODE                   : 1;
    } Bits;
    uint32_t Raw;
} WF_DS_LWTBL_REG_PEER_CAP_DW04, *PWF_DS_LWTBL_REG_PEER_CAP_DW04;

typedef PACKING union
{
    PACKING struct
    {
        FIELD AF                        : 4;
        FIELD BSA_EN                    : 1;
        FIELD RTS                       : 1;
        FIELD SMPS                      : 1;
        FIELD DYN_BW                    : 1;
        FIELD MMSS                      : 3;
        FIELD USR                       : 1;
        FIELD SR_R                      : 3;
        FIELD SR_ABORT                  : 1;
        FIELD TX_POWER_OFFSET           : 6;
        FIELD LTF_EHT                   : 2;
        FIELD GI_EHT                    : 2;
        FIELD DOPPL                     : 1;
        FIELD TXOP_PS_CAP               : 1;
        FIELD DU_I_PSM                  : 1;
        FIELD I_PSM                     : 1;
        FIELD PSM                       : 1;
        FIELD SKIP_TX                   : 1;
    } Bits;
    uint32_t Raw;
} WF_DS_LWTBL_REG_PEER_CAP_DW05, *PWF_DS_LWTBL_REG_PEER_CAP_DW05;

typedef PACKING union
{
    PACKING struct
    {
        FIELD CBRN                      : 3;
        FIELD DBNSS_EN                  : 1;
        FIELD BAF_EN                    : 1;
        FIELD RDGBA                     : 1;
        FIELD R                         : 1;
        FIELD SPE_IDX                   : 5;
        FIELD G2                        : 1;
        FIELD G4                        : 1;
        FIELD G8                        : 1;
        FIELD G16                       : 1;
        FIELD G2_LTF                    : 2;
        FIELD G4_LTF                    : 2;
        FIELD G8_LTF                    : 2;
        FIELD G16_LTF                   : 2;
        FIELD G2_HE                     : 2;
        FIELD G4_HE                     : 2;
        FIELD G8_HE                     : 2;
        FIELD G16_HE                    : 2;
    } Bits;
    uint32_t Raw;
} WF_DS_LWTBL_REG_PEER_CAP_DW06, *PWF_DS_LWTBL_REG_PEER_CAP_DW06;

typedef PACKING union
{
    PACKING struct
    {
        FIELD BA_WIN_SIZE0              : 4;
        FIELD BA_WIN_SIZE1              : 4;
        FIELD BA_WIN_SIZE2              : 4;
        FIELD BA_WIN_SIZE3              : 4;
        FIELD BA_WIN_SIZE4              : 4;
        FIELD BA_WIN_SIZE5              : 4;
        FIELD BA_WIN_SIZE6              : 4;
        FIELD BA_WIN_SIZE7              : 4;
    } Bits;
    uint32_t Raw;
} WF_DS_LWTBL_REG_PEER_CAP_DW07, *PWF_DS_LWTBL_REG_PEER_CAP_DW07;

typedef PACKING union
{
    PACKING struct
    {
        FIELD AC0_RTS_FAIL_CNT          : 5;
        FIELD AC1_RTS_FAIL_CNT          : 5;
        FIELD AC2_RTS_FAIL_CNT          : 5;
        FIELD AC3_RTS_FAIL_CNT          : 5;
        FIELD PARTIAL_AID               : 9;
        FIELD rsv_29                    : 2;
        FIELD CHK_PER                   : 1;
    } Bits;
    uint32_t Raw;
} WF_DS_LWTBL_REG_PEER_CAP_DW08, *PWF_DS_LWTBL_REG_PEER_CAP_DW08;

typedef PACKING union
{
    PACKING struct
    {
        FIELD RX_AVG_MPDU_SIZE          : 14;
        FIELD PRITX_SW_MODE             : 1;
        FIELD PRITX_ERSU                : 1;
        FIELD PRITX_PLR                 : 2;
        FIELD PRITX_DCM                 : 1;
        FIELD PRITX_ER106T              : 1;
        FIELD FCAP                      : 3;
        FIELD MPDU_FAIL_CNT             : 3;
        FIELD MPDU_OK_CNT               : 3;
        FIELD RATE_IDX                  : 3;
    } Bits;
    uint32_t Raw;
} WF_DS_LWTBL_REG_PEER_CAP_DW09, *PWF_DS_LWTBL_REG_PEER_CAP_DW09;

typedef PACKING union
{
    PACKING struct
    {
        FIELD RATE1                     : 15;
        FIELD rsv_15                    : 1;
        FIELD RATE2                     : 15;
        FIELD rsv_31                    : 1;
    } Bits;
    uint32_t Raw;
} WF_DS_LWTBL_REG_AUTO_RATE_TABLE_DW10, *PWF_DS_LWTBL_REG_AUTO_RATE_TABLE_DW10;

typedef PACKING union
{
    PACKING struct
    {
        FIELD RATE3                     : 15;
        FIELD rsv_15                    : 1;
        FIELD RATE4                     : 15;
        FIELD rsv_31                    : 1;
    } Bits;
    uint32_t Raw;
} WF_DS_LWTBL_REG_AUTO_RATE_TABLE_DW11, *PWF_DS_LWTBL_REG_AUTO_RATE_TABLE_DW11;

typedef PACKING union
{
    PACKING struct
    {
        FIELD RATE5                     : 15;
        FIELD rsv_15                    : 1;
        FIELD RATE6                     : 15;
        FIELD rsv_31                    : 1;
    } Bits;
    uint32_t Raw;
} WF_DS_LWTBL_REG_AUTO_RATE_TABLE_DW12, *PWF_DS_LWTBL_REG_AUTO_RATE_TABLE_DW12;

typedef PACKING union
{
    PACKING struct
    {
        FIELD RATE7                     : 15;
        FIELD rsv_15                    : 1;
        FIELD RATE8                     : 15;
        FIELD rsv_31                    : 1;
    } Bits;
    uint32_t Raw;
} WF_DS_LWTBL_REG_AUTO_RATE_TABLE_DW13, *PWF_DS_LWTBL_REG_AUTO_RATE_TABLE_DW13;

typedef PACKING union
{
    PACKING struct
    {
        FIELD RATE1_TX_CNT              : 16;
        FIELD RATE1_FAIL_CNT            : 16;
    } Bits;
    uint32_t Raw;
} WF_DS_LWTBL_REG_AUTO_RATE_CTR_DW14_0, *PWF_DS_LWTBL_REG_AUTO_RATE_CTR_DW14_0;

typedef PACKING union
{
    PACKING struct
    {
        FIELD rsv_0                     : 8;
        FIELD CIPHER_SUIT_IGTK          : 3;
        FIELD rsv_11                    : 1;
        FIELD CIPHER_SUIT_BIGTK         : 3;
        FIELD rsv_15                    : 17;
    } Bits;
    uint32_t Raw;
} WF_DS_LWTBL_REG_AUTO_RATE_CTR_DW14_1, *PWF_DS_LWTBL_REG_AUTO_RATE_CTR_DW14_1;

typedef PACKING union
{
    PACKING struct
    {
        FIELD RATE2_OK_CNT              : 16;
        FIELD RATE3_OK_CNT              : 16;
    } Bits;
    uint32_t Raw;
} WF_DS_LWTBL_REG_AUTO_RATE_CTR_DW15_0, *PWF_DS_LWTBL_REG_AUTO_RATE_CTR_DW15_0;

typedef PACKING union
{
    PACKING struct
    {
        FIELD CURRENT_BW_TX_CNT         : 16;
        FIELD CURRENT_BW_FAIL_CNT       : 16;
    } Bits;
    uint32_t Raw;
} WF_DS_LWTBL_REG_AUTO_RATE_CTR_DW16_0, *PWF_DS_LWTBL_REG_AUTO_RATE_CTR_DW16_0;

typedef PACKING union
{
    PACKING struct
    {
        FIELD OTHER_BW_TX_CNT           : 16;
        FIELD OTHER_BW_FAIL_CNT         : 16;
    } Bits;
    uint32_t Raw;
} WF_DS_LWTBL_REG_AUTO_RATE_CTR_DW17_0, *PWF_DS_LWTBL_REG_AUTO_RATE_CTR_DW17_0;

typedef PACKING union
{
    PACKING struct
    {
        FIELD RTS_OK_CNT                : 16;
        FIELD RTS_FAIL_CNT              : 16;
    } Bits;
    uint32_t Raw;
} WF_DS_LWTBL_REG_PPDU_CTR_DW18_0, *PWF_DS_LWTBL_REG_PPDU_CTR_DW18_0;

typedef PACKING union
{
    PACKING struct
    {
        FIELD DATA_RETRY_CNT            : 16;
        FIELD MGNT_RETRY_CNT            : 16;
    } Bits;
    uint32_t Raw;
} WF_DS_LWTBL_REG_PPDU_CTR_DW19_0, *PWF_DS_LWTBL_REG_PPDU_CTR_DW19_0;

typedef PACKING union
{
    PACKING struct
    {
        FIELD AC0_CTT_CDT_CRB__CTT      : 32;
    } Bits;
    uint32_t Raw;
} WF_DS_LWTBL_REG_ADM_CTR_DW20_0, *PWF_DS_LWTBL_REG_ADM_CTR_DW20_0;

typedef PACKING union
{
    PACKING struct
    {
        FIELD AC0_CTB_CRT_CTB__CRT      : 32;
    } Bits;
    uint32_t Raw;
} WF_DS_LWTBL_REG_ADM_CTR_DW21_0, *PWF_DS_LWTBL_REG_ADM_CTR_DW21_0;

typedef PACKING union
{
    PACKING struct
    {
        FIELD AC1_CTT_CDT_CRB__CTMC     : 32;
    } Bits;
    uint32_t Raw;
} WF_DS_LWTBL_REG_ADM_CTR_DW22_0, *PWF_DS_LWTBL_REG_ADM_CTR_DW22_0;

typedef PACKING union
{
    PACKING struct
    {
        FIELD AC1_CTB_CRT_CTB__CRMC     : 32;
    } Bits;
    uint32_t Raw;
} WF_DS_LWTBL_REG_ADM_CTR_DW23_0, *PWF_DS_LWTBL_REG_ADM_CTR_DW23_0;

typedef PACKING union
{
    PACKING struct
    {
        FIELD AC2_CTT_CDT_CRB__CTDB     : 32;
    } Bits;
    uint32_t Raw;
} WF_DS_LWTBL_REG_ADM_CTR_DW24_0, *PWF_DS_LWTBL_REG_ADM_CTR_DW24_0;

typedef PACKING union
{
    PACKING struct
    {
        FIELD AC2_CTB_CRT_CTB__CRDB     : 32;
    } Bits;
    uint32_t Raw;
} WF_DS_LWTBL_REG_ADM_CTR_DW25_0, *PWF_DS_LWTBL_REG_ADM_CTR_DW25_0;

typedef PACKING union
{
    PACKING struct
    {
        FIELD AC3_CTT_CDT_CRB__CTODB    : 32;
    } Bits;
    uint32_t Raw;
} WF_DS_LWTBL_REG_ADM_CTR_DW26_0, *PWF_DS_LWTBL_REG_ADM_CTR_DW26_0;

typedef PACKING union
{
    PACKING struct
    {
        FIELD AC3_CTB_CRT_CTB__         : 32;
    } Bits;
    uint32_t Raw;
} WF_DS_LWTBL_REG_ADM_CTR_DW27_0, *PWF_DS_LWTBL_REG_ADM_CTR_DW27_0;

typedef PACKING union
{
    PACKING struct
    {
        FIELD RELATED_IDX0              : 12;
        FIELD RELATED_BAND0             : 2;
        FIELD PRIMARY_MLD_BAND          : 2;
        FIELD RELATED_IDX1              : 12;
        FIELD RELATED_BAND1             : 2;
        FIELD SECONDARY_MLD_BAND        : 2;
    } Bits;
    uint32_t Raw;
} WF_DS_LWTBL_REG_MLO_INFO_DW28, *PWF_DS_LWTBL_REG_MLO_INFO_DW28;

typedef PACKING union
{
    PACKING struct
    {
        FIELD DISPATCH_POLICY0          : 2;
        FIELD DISPATCH_POLICY1          : 2;
        FIELD DISPATCH_POLICY2          : 2;
        FIELD DISPATCH_POLICY3          : 2;
        FIELD DISPATCH_POLICY4          : 2;
        FIELD DISPATCH_POLICY5          : 2;
        FIELD DISPATCH_POLICY6          : 2;
        FIELD DISPATCH_POLICY7          : 2;
        FIELD OWN_MLD_ID                : 6;
        FIELD EMLSR0                    : 1;
        FIELD EMLMR0                    : 1;
        FIELD EMLSR1                    : 1;
        FIELD EMLMR1                    : 1;
        FIELD EMLSR2                    : 1;
        FIELD EMLMR2                    : 1;
        FIELD rsv_28                    : 1;
        FIELD STR_BITMAP                : 3;
    } Bits;
    uint32_t Raw;
} WF_DS_LWTBL_REG_MLO_INFO_DW29, *PWF_DS_LWTBL_REG_MLO_INFO_DW29;

typedef PACKING union
{
    PACKING struct
    {
        FIELD DISPATCH_ORDER            : 7;
        FIELD DISPATCH_RATIO            : 7;
        FIELD EMLSR_TRANS_DLY_IDX       : 2;
        FIELD LINK_MGF                  : 16;
    } Bits;
    uint32_t Raw;
} WF_DS_LWTBL_REG_MLO_INFO_DW30, *PWF_DS_LWTBL_REG_MLO_INFO_DW30;

typedef PACKING union
{
    PACKING struct
    {
        FIELD rsv_0                     : 23;
        FIELD BFTX_TB                   : 1;
        FIELD DROP                      : 1;
        FIELD CASCAD                    : 1;
        FIELD ALL_ACK                   : 1;
        FIELD MPDU_SIZE                 : 2;
        FIELD RXD_DUP_MODE              : 2;
        FIELD ACK_EN                    : 1;
    } Bits;
    uint32_t Raw;
} WF_DS_LWTBL_REG_RESP_INFO_DW31, *PWF_DS_LWTBL_REG_RESP_INFO_DW31;

typedef PACKING union
{
    PACKING struct
    {
        FIELD OM_INFO                   : 12;
        FIELD OM_INFO_EHT               : 4;
        FIELD RXD_DUP_FOR_OM_CHG        : 1;
        FIELD RXD_DUP_WHITE_LIST        : 12;
        FIELD rsv_29                    : 3;
    } Bits;
    uint32_t Raw;
} WF_DS_LWTBL_REG_RX_DUP_INFO_DW32, *PWF_DS_LWTBL_REG_RX_DUP_INFO_DW32;

typedef PACKING union
{
    PACKING struct
    {
        FIELD USER_RSSI                 : 9;
        FIELD USER_SNR                  : 6;
        FIELD rsv_15                    : 1;
        FIELD RAPID_REACTION_RATE       : 12;
        FIELD rsv_28                    : 2;
        FIELD HT_AMSDU                  : 1;
        FIELD AMSDU_CROSS_LG            : 1;
    } Bits;
    uint32_t Raw;
} WF_DS_LWTBL_REG_RX_STAT_DW33, *PWF_DS_LWTBL_REG_RX_STAT_DW33;

typedef PACKING union
{
    PACKING struct
    {
        FIELD RESP_RCPI0                : 8;
        FIELD RESP_RCPI1                : 8;
        FIELD RESP_RCPI2                : 8;
        FIELD RESP_RCPI3                : 8;
    } Bits;
    uint32_t Raw;
} WF_DS_LWTBL_REG_RX_STAT_DW34, *PWF_DS_LWTBL_REG_RX_STAT_DW34;

typedef PACKING union
{
    PACKING struct
    {
        FIELD SNR_RX0                   : 8;
        FIELD SNR_RX1                   : 8;
        FIELD SNR_RX2                   : 8;
        FIELD SNR_RX3                   : 8;
    } Bits;
    uint32_t Raw;
} WF_DS_LWTBL_REG_RX_STAT_DW35, *PWF_DS_LWTBL_REG_RX_STAT_DW35;

typedef PACKING union
{
    PACKING struct
    {
        FIELD VERSION_CODE              : 32;
    } Bits;
    uint32_t Raw;
} WF_DS_LWTBL_REG_VERSION_CODE, *PWF_DS_LWTBL_REG_VERSION_CODE;

// ----------------- WF_DS_LWTBL  Grouping Definitions -------------------
// ----------------- WF_DS_LWTBL Register Definition -------------------
typedef volatile PACKING struct
{
    WF_DS_LWTBL_REG_PEER_INFO_DW00  PEER_INFO_DW00;   // 8000
    uint32_t                          rsv_8004[7];      // 8004..801C
    WF_DS_LWTBL_REG_PEER_INFO_DW01  PEER_INFO_DW01;   // 8020
    uint32_t                          rsv_8024[7];      // 8024..803C
    WF_DS_LWTBL_REG_PEER_CAP_DW02   PEER_CAP_DW02;    // 8040
    uint32_t                          rsv_8044[7];      // 8044..805C
    WF_DS_LWTBL_REG_PEER_CAP_DW03   PEER_CAP_DW03;    // 8060
    uint32_t                          rsv_8064[7];      // 8064..807C
    WF_DS_LWTBL_REG_PEER_CAP_DW04   PEER_CAP_DW04;    // 8080
    uint32_t                          rsv_8084[7];      // 8084..809C
    WF_DS_LWTBL_REG_PEER_CAP_DW05   PEER_CAP_DW05;    // 80A0
    uint32_t                          rsv_80A4[7];      // 80A4..80BC
    WF_DS_LWTBL_REG_PEER_CAP_DW06   PEER_CAP_DW06;    // 80C0
    uint32_t                          rsv_80C4[7];      // 80C4..80DC
    WF_DS_LWTBL_REG_PEER_CAP_DW07   PEER_CAP_DW07;    // 80E0
    uint32_t                          rsv_80E4[7];      // 80E4..80FC
    WF_DS_LWTBL_REG_PEER_CAP_DW08   PEER_CAP_DW08;    // 8100
    uint32_t                          rsv_8104[7];      // 8104..811C
    WF_DS_LWTBL_REG_PEER_CAP_DW09   PEER_CAP_DW09;    // 8120
    uint32_t                          rsv_8124[7];      // 8124..813C
    WF_DS_LWTBL_REG_AUTO_RATE_TABLE_DW10 AUTO_RATE_TABLE_DW10; // 8140
    uint32_t                          rsv_8144[7];      // 8144..815C
    WF_DS_LWTBL_REG_AUTO_RATE_TABLE_DW11 AUTO_RATE_TABLE_DW11; // 8160
    uint32_t                          rsv_8164[7];      // 8164..817C
    WF_DS_LWTBL_REG_AUTO_RATE_TABLE_DW12 AUTO_RATE_TABLE_DW12; // 8180
    uint32_t                          rsv_8184[7];      // 8184..819C
    WF_DS_LWTBL_REG_AUTO_RATE_TABLE_DW13 AUTO_RATE_TABLE_DW13; // 81A0
    uint32_t                          rsv_81A4[7];      // 81A4..81BC
    WF_DS_LWTBL_REG_AUTO_RATE_CTR_DW14_0 AUTO_RATE_CTR_DW14_0; // 81C0
    WF_DS_LWTBL_REG_AUTO_RATE_CTR_DW14_1 AUTO_RATE_CTR_DW14_1; // 81C4
    uint32_t                          rsv_81C8[6];      // 81C8..81DC
    WF_DS_LWTBL_REG_AUTO_RATE_CTR_DW15_0 AUTO_RATE_CTR_DW15_0; // 81E0
    uint32_t                          rsv_81E4[7];      // 81E4..81FC
    WF_DS_LWTBL_REG_AUTO_RATE_CTR_DW16_0 AUTO_RATE_CTR_DW16_0; // 8200
    uint32_t                          rsv_8204[7];      // 8204..821C
    WF_DS_LWTBL_REG_AUTO_RATE_CTR_DW17_0 AUTO_RATE_CTR_DW17_0; // 8220
    uint32_t                          rsv_8224[7];      // 8224..823C
    WF_DS_LWTBL_REG_PPDU_CTR_DW18_0 PPDU_CTR_DW18_0;  // 8240
    uint32_t                          rsv_8244[7];      // 8244..825C
    WF_DS_LWTBL_REG_PPDU_CTR_DW19_0 PPDU_CTR_DW19_0;  // 8260
    uint32_t                          rsv_8264[7];      // 8264..827C
    WF_DS_LWTBL_REG_ADM_CTR_DW20_0  ADM_CTR_DW20_0;   // 8280
    uint32_t                          rsv_8284[7];      // 8284..829C
    WF_DS_LWTBL_REG_ADM_CTR_DW21_0  ADM_CTR_DW21_0;   // 82A0
    uint32_t                          rsv_82A4[7];      // 82A4..82BC
    WF_DS_LWTBL_REG_ADM_CTR_DW22_0  ADM_CTR_DW22_0;   // 82C0
    uint32_t                          rsv_82C4[7];      // 82C4..82DC
    WF_DS_LWTBL_REG_ADM_CTR_DW23_0  ADM_CTR_DW23_0;   // 82E0
    uint32_t                          rsv_82E4[7];      // 82E4..82FC
    WF_DS_LWTBL_REG_ADM_CTR_DW24_0  ADM_CTR_DW24_0;   // 8300
    uint32_t                          rsv_8304[7];      // 8304..831C
    WF_DS_LWTBL_REG_ADM_CTR_DW25_0  ADM_CTR_DW25_0;   // 8320
    uint32_t                          rsv_8324[7];      // 8324..833C
    WF_DS_LWTBL_REG_ADM_CTR_DW26_0  ADM_CTR_DW26_0;   // 8340
    uint32_t                          rsv_8344[7];      // 8344..835C
    WF_DS_LWTBL_REG_ADM_CTR_DW27_0  ADM_CTR_DW27_0;   // 8360
    uint32_t                          rsv_8364[7];      // 8364..837C
    WF_DS_LWTBL_REG_MLO_INFO_DW28   MLO_INFO_DW28;    // 8380
    uint32_t                          rsv_8384[7];      // 8384..839C
    WF_DS_LWTBL_REG_MLO_INFO_DW29   MLO_INFO_DW29;    // 83A0
    uint32_t                          rsv_83A4[7];      // 83A4..83BC
    WF_DS_LWTBL_REG_MLO_INFO_DW30   MLO_INFO_DW30;    // 83C0
    uint32_t                          rsv_83C4[7];      // 83C4..83DC
    WF_DS_LWTBL_REG_RESP_INFO_DW31  RESP_INFO_DW31;   // 83E0
    uint32_t                          rsv_83E4[7];      // 83E4..83FC
    WF_DS_LWTBL_REG_RX_DUP_INFO_DW32 RX_DUP_INFO_DW32; // 8400
    uint32_t                          rsv_8404[7];      // 8404..841C
    WF_DS_LWTBL_REG_RX_STAT_DW33    RX_STAT_DW33;     // 8420
    uint32_t                          rsv_8424[7];      // 8424..843C
    WF_DS_LWTBL_REG_RX_STAT_DW34    RX_STAT_DW34;     // 8440
    uint32_t                          rsv_8444[7];      // 8444..845C
    WF_DS_LWTBL_REG_RX_STAT_DW35    RX_STAT_DW35;     // 8460
    uint32_t                          rsv_8464[16102];  // 8464..7FF8
    uint8_t                           rsv_7FFC;         // 7FFC
    uint16_t                          rsv_7FFD;         // 7FFD
    WF_DS_LWTBL_REG_VERSION_CODE    VERSION_CODE;     // 7FFF
}WF_DS_LWTBL_REGS, *PWF_DS_LWTBL_REGS;

// ---------- WF_DS_LWTBL Enum Definitions      ----------
// ---------- WF_DS_LWTBL C Macro Definitions   ----------
extern PWF_DS_LWTBL_REGS g_WF_DS_LWTBL_BASE;

#define WF_DS_LWTBL_BASE                                       (g_WF_DS_LWTBL_BASE)

#define WF_DS_LWTBL_PEER_INFO_DW00                             INREG32(&WF_DS_LWTBL_BASE->PEER_INFO_DW00) // 8000
#define WF_DS_LWTBL_PEER_INFO_DW01                             INREG32(&WF_DS_LWTBL_BASE->PEER_INFO_DW01) // 8020
#define WF_DS_LWTBL_PEER_CAP_DW02                              INREG32(&WF_DS_LWTBL_BASE->PEER_CAP_DW02) // 8040
#define WF_DS_LWTBL_PEER_CAP_DW03                              INREG32(&WF_DS_LWTBL_BASE->PEER_CAP_DW03) // 8060
#define WF_DS_LWTBL_PEER_CAP_DW04                              INREG32(&WF_DS_LWTBL_BASE->PEER_CAP_DW04) // 8080
#define WF_DS_LWTBL_PEER_CAP_DW05                              INREG32(&WF_DS_LWTBL_BASE->PEER_CAP_DW05) // 80A0
#define WF_DS_LWTBL_PEER_CAP_DW06                              INREG32(&WF_DS_LWTBL_BASE->PEER_CAP_DW06) // 80C0
#define WF_DS_LWTBL_PEER_CAP_DW07                              INREG32(&WF_DS_LWTBL_BASE->PEER_CAP_DW07) // 80E0
#define WF_DS_LWTBL_PEER_CAP_DW08                              INREG32(&WF_DS_LWTBL_BASE->PEER_CAP_DW08) // 8100
#define WF_DS_LWTBL_PEER_CAP_DW09                              INREG32(&WF_DS_LWTBL_BASE->PEER_CAP_DW09) // 8120
#define WF_DS_LWTBL_AUTO_RATE_TABLE_DW10                       INREG32(&WF_DS_LWTBL_BASE->AUTO_RATE_TABLE_DW10) // 8140
#define WF_DS_LWTBL_AUTO_RATE_TABLE_DW11                       INREG32(&WF_DS_LWTBL_BASE->AUTO_RATE_TABLE_DW11) // 8160
#define WF_DS_LWTBL_AUTO_RATE_TABLE_DW12                       INREG32(&WF_DS_LWTBL_BASE->AUTO_RATE_TABLE_DW12) // 8180
#define WF_DS_LWTBL_AUTO_RATE_TABLE_DW13                       INREG32(&WF_DS_LWTBL_BASE->AUTO_RATE_TABLE_DW13) // 81A0
#define WF_DS_LWTBL_AUTO_RATE_CTR_DW14_0                       INREG32(&WF_DS_LWTBL_BASE->AUTO_RATE_CTR_DW14_0) // 81C0
#define WF_DS_LWTBL_AUTO_RATE_CTR_DW14_1                       INREG32(&WF_DS_LWTBL_BASE->AUTO_RATE_CTR_DW14_1) // 81C4
#define WF_DS_LWTBL_AUTO_RATE_CTR_DW15_0                       INREG32(&WF_DS_LWTBL_BASE->AUTO_RATE_CTR_DW15_0) // 81E0
#define WF_DS_LWTBL_AUTO_RATE_CTR_DW16_0                       INREG32(&WF_DS_LWTBL_BASE->AUTO_RATE_CTR_DW16_0) // 8200
#define WF_DS_LWTBL_AUTO_RATE_CTR_DW17_0                       INREG32(&WF_DS_LWTBL_BASE->AUTO_RATE_CTR_DW17_0) // 8220
#define WF_DS_LWTBL_PPDU_CTR_DW18_0                            INREG32(&WF_DS_LWTBL_BASE->PPDU_CTR_DW18_0) // 8240
#define WF_DS_LWTBL_PPDU_CTR_DW19_0                            INREG32(&WF_DS_LWTBL_BASE->PPDU_CTR_DW19_0) // 8260
#define WF_DS_LWTBL_ADM_CTR_DW20_0                             INREG32(&WF_DS_LWTBL_BASE->ADM_CTR_DW20_0) // 8280
#define WF_DS_LWTBL_ADM_CTR_DW21_0                             INREG32(&WF_DS_LWTBL_BASE->ADM_CTR_DW21_0) // 82A0
#define WF_DS_LWTBL_ADM_CTR_DW22_0                             INREG32(&WF_DS_LWTBL_BASE->ADM_CTR_DW22_0) // 82C0
#define WF_DS_LWTBL_ADM_CTR_DW23_0                             INREG32(&WF_DS_LWTBL_BASE->ADM_CTR_DW23_0) // 82E0
#define WF_DS_LWTBL_ADM_CTR_DW24_0                             INREG32(&WF_DS_LWTBL_BASE->ADM_CTR_DW24_0) // 8300
#define WF_DS_LWTBL_ADM_CTR_DW25_0                             INREG32(&WF_DS_LWTBL_BASE->ADM_CTR_DW25_0) // 8320
#define WF_DS_LWTBL_ADM_CTR_DW26_0                             INREG32(&WF_DS_LWTBL_BASE->ADM_CTR_DW26_0) // 8340
#define WF_DS_LWTBL_ADM_CTR_DW27_0                             INREG32(&WF_DS_LWTBL_BASE->ADM_CTR_DW27_0) // 8360
#define WF_DS_LWTBL_MLO_INFO_DW28                              INREG32(&WF_DS_LWTBL_BASE->MLO_INFO_DW28) // 8380
#define WF_DS_LWTBL_MLO_INFO_DW29                              INREG32(&WF_DS_LWTBL_BASE->MLO_INFO_DW29) // 83A0
#define WF_DS_LWTBL_MLO_INFO_DW30                              INREG32(&WF_DS_LWTBL_BASE->MLO_INFO_DW30) // 83C0
#define WF_DS_LWTBL_RESP_INFO_DW31                             INREG32(&WF_DS_LWTBL_BASE->RESP_INFO_DW31) // 83E0
#define WF_DS_LWTBL_RX_DUP_INFO_DW32                           INREG32(&WF_DS_LWTBL_BASE->RX_DUP_INFO_DW32) // 8400
#define WF_DS_LWTBL_RX_STAT_DW33                               INREG32(&WF_DS_LWTBL_BASE->RX_STAT_DW33) // 8420
#define WF_DS_LWTBL_RX_STAT_DW34                               INREG32(&WF_DS_LWTBL_BASE->RX_STAT_DW34) // 8440
#define WF_DS_LWTBL_RX_STAT_DW35                               INREG32(&WF_DS_LWTBL_BASE->RX_STAT_DW35) // 8460
#define WF_DS_LWTBL_VERSION_CODE                               INREG32(&WF_DS_LWTBL_BASE->VERSION_CODE) // 7FFF

#endif


#define PEER_INFO_DW00_FLD_WPI_FLAG                            REG_FLD(1, 30)
#define PEER_INFO_DW00_FLD_RCA2                                REG_FLD(1, 29)
#define PEER_INFO_DW00_FLD_RV                                  REG_FLD(1, 28)
#define PEER_INFO_DW00_FLD_BAND                                REG_FLD(2, 26)
#define PEER_INFO_DW00_FLD_RCID                                REG_FLD(1, 25)
#define PEER_INFO_DW00_FLD_KID                                 REG_FLD(2, 23)
#define PEER_INFO_DW00_FLD_RCA1                                REG_FLD(1, 22)
#define PEER_INFO_DW00_FLD_MUAR                                REG_FLD(6, 16)
#define PEER_INFO_DW00_FLD_PEER_LINK_ADDRESS_47_32_            REG_FLD(16, 0)

#define PEER_INFO_DW01_FLD_PEER_LINK_ADDRESS_31_0_             REG_FLD(32, 0)

#define PEER_CAP_DW02_FLD_MESH                                 REG_FLD(1, 31)
#define PEER_CAP_DW02_FLD_EHT                                  REG_FLD(1, 30)
#define PEER_CAP_DW02_FLD_HE                                   REG_FLD(1, 29)
#define PEER_CAP_DW02_FLD_VHT                                  REG_FLD(1, 28)
#define PEER_CAP_DW02_FLD_HT                                   REG_FLD(1, 27)
#define PEER_CAP_DW02_FLD_QOS                                  REG_FLD(1, 26)
#define PEER_CAP_DW02_FLD_TX_PS                                REG_FLD(1, 25)
#define PEER_CAP_DW02_FLD_UL                                   REG_FLD(1, 24)
#define PEER_CAP_DW02_FLD_SW                                   REG_FLD(1, 23)
#define PEER_CAP_DW02_FLD_TD                                   REG_FLD(1, 22)
#define PEER_CAP_DW02_FLD_FD                                   REG_FLD(1, 21)
#define PEER_CAP_DW02_FLD_CIPHER_SUIT_PGTK                     REG_FLD(5, 16)
#define PEER_CAP_DW02_FLD_DUAL_CTS_CAP                         REG_FLD(1, 14)
#define PEER_CAP_DW02_FLD_DUAL_PTEC_EN                         REG_FLD(1, 13)
#define PEER_CAP_DW02_FLD_GID_SU                               REG_FLD(1, 12)
#define PEER_CAP_DW02_FLD_AID                                  REG_FLD(12, 0)

#define PEER_CAP_DW03_FLD_IGN_FBK                              REG_FLD(1, 31)
#define PEER_CAP_DW03_FLD_TBF_EHT                              REG_FLD(1, 30)
#define PEER_CAP_DW03_FLD_TBF_HE                               REG_FLD(1, 29)
#define PEER_CAP_DW03_FLD_TBF_VHT                              REG_FLD(1, 28)
#define PEER_CAP_DW03_FLD_TBF_HT                               REG_FLD(1, 27)
#define PEER_CAP_DW03_FLD_BYPASS_TXSMM                         REG_FLD(1, 26)
#define PEER_CAP_DW03_FLD_ULPF                                 REG_FLD(1, 25)
#define PEER_CAP_DW03_FLD_RIBF                                 REG_FLD(1, 24)
#define PEER_CAP_DW03_FLD_ULPF_IDX                             REG_FLD(8, 16)
#define PEER_CAP_DW03_FLD_PFMU_IDX                             REG_FLD(8, 8)
#define PEER_CAP_DW03_FLD_EHT_LTF_SYM_NUM_OPT                  REG_FLD(2, 6)
#define PEER_CAP_DW03_FLD_BEAM_CHG                             REG_FLD(1, 5)
#define PEER_CAP_DW03_FLD_HDRT_MODE                            REG_FLD(1, 4)
#define PEER_CAP_DW03_FLD_EHT_SIG_MCS                          REG_FLD(2, 2)
#define PEER_CAP_DW03_FLD_WMM_Q                                REG_FLD(2, 0)

#define PEER_CAP_DW04_FLD_BA_MODE                              REG_FLD(1, 31)
#define PEER_CAP_DW04_FLD_LDPC_EHT                             REG_FLD(1, 30)
#define PEER_CAP_DW04_FLD_LDPC_HE                              REG_FLD(1, 29)
#define PEER_CAP_DW04_FLD_LDPC_VHT                             REG_FLD(1, 28)
#define PEER_CAP_DW04_FLD_LDPC_HT                              REG_FLD(1, 27)
#define PEER_CAP_DW04_FLD_DIS_RHTR                             REG_FLD(1, 26)
#define PEER_CAP_DW04_FLD_PE                                   REG_FLD(2, 24)
#define PEER_CAP_DW04_FLD_NEGOTIATED_WINSIZE7                  REG_FLD(3, 21)
#define PEER_CAP_DW04_FLD_NEGOTIATED_WINSIZE6                  REG_FLD(3, 18)
#define PEER_CAP_DW04_FLD_NEGOTIATED_WINSIZE5                  REG_FLD(3, 15)
#define PEER_CAP_DW04_FLD_NEGOTIATED_WINSIZE4                  REG_FLD(3, 12)
#define PEER_CAP_DW04_FLD_NEGOTIATED_WINSIZE3                  REG_FLD(3, 9)
#define PEER_CAP_DW04_FLD_NEGOTIATED_WINSIZE2                  REG_FLD(3, 6)
#define PEER_CAP_DW04_FLD_NEGOTIATED_WINSIZE1                  REG_FLD(3, 3)
#define PEER_CAP_DW04_FLD_NEGOTIATED_WINSIZE0                  REG_FLD(3, 0)

#define PEER_CAP_DW05_FLD_SKIP_TX                              REG_FLD(1, 31)
#define PEER_CAP_DW05_FLD_PSM                                  REG_FLD(1, 30)
#define PEER_CAP_DW05_FLD_I_PSM                                REG_FLD(1, 29)
#define PEER_CAP_DW05_FLD_DU_I_PSM                             REG_FLD(1, 28)
#define PEER_CAP_DW05_FLD_TXOP_PS_CAP                          REG_FLD(1, 27)
#define PEER_CAP_DW05_FLD_DOPPL                                REG_FLD(1, 26)
#define PEER_CAP_DW05_FLD_GI_EHT                               REG_FLD(2, 24)
#define PEER_CAP_DW05_FLD_LTF_EHT                              REG_FLD(2, 22)
#define PEER_CAP_DW05_FLD_TX_POWER_OFFSET                      REG_FLD(6, 16)
#define PEER_CAP_DW05_FLD_SR_ABORT                             REG_FLD(1, 15)
#define PEER_CAP_DW05_FLD_SR_R                                 REG_FLD(3, 12)
#define PEER_CAP_DW05_FLD_USR                                  REG_FLD(1, 11)
#define PEER_CAP_DW05_FLD_MMSS                                 REG_FLD(3, 8)
#define PEER_CAP_DW05_FLD_DYN_BW                               REG_FLD(1, 7)
#define PEER_CAP_DW05_FLD_SMPS                                 REG_FLD(1, 6)
#define PEER_CAP_DW05_FLD_RTS                                  REG_FLD(1, 5)
#define PEER_CAP_DW05_FLD_BSA_EN                               REG_FLD(1, 4)
#define PEER_CAP_DW05_FLD_AF                                   REG_FLD(4, 0)

#define PEER_CAP_DW06_FLD_G16_HE                               REG_FLD(2, 30)
#define PEER_CAP_DW06_FLD_G8_HE                                REG_FLD(2, 28)
#define PEER_CAP_DW06_FLD_G4_HE                                REG_FLD(2, 26)
#define PEER_CAP_DW06_FLD_G2_HE                                REG_FLD(2, 24)
#define PEER_CAP_DW06_FLD_G16_LTF                              REG_FLD(2, 22)
#define PEER_CAP_DW06_FLD_G8_LTF                               REG_FLD(2, 20)
#define PEER_CAP_DW06_FLD_G4_LTF                               REG_FLD(2, 18)
#define PEER_CAP_DW06_FLD_G2_LTF                               REG_FLD(2, 16)
#define PEER_CAP_DW06_FLD_G16                                  REG_FLD(1, 15)
#define PEER_CAP_DW06_FLD_G8                                   REG_FLD(1, 14)
#define PEER_CAP_DW06_FLD_G4                                   REG_FLD(1, 13)
#define PEER_CAP_DW06_FLD_G2                                   REG_FLD(1, 12)
#define PEER_CAP_DW06_FLD_SPE_IDX                              REG_FLD(5, 7)
#define PEER_CAP_DW06_FLD_R                                    REG_FLD(1, 6)
#define PEER_CAP_DW06_FLD_RDGBA                                REG_FLD(1, 5)
#define PEER_CAP_DW06_FLD_BAF_EN                               REG_FLD(1, 4)
#define PEER_CAP_DW06_FLD_DBNSS_EN                             REG_FLD(1, 3)
#define PEER_CAP_DW06_FLD_CBRN                                 REG_FLD(3, 0)

#define PEER_CAP_DW07_FLD_BA_WIN_SIZE7                         REG_FLD(4, 28)
#define PEER_CAP_DW07_FLD_BA_WIN_SIZE6                         REG_FLD(4, 24)
#define PEER_CAP_DW07_FLD_BA_WIN_SIZE5                         REG_FLD(4, 20)
#define PEER_CAP_DW07_FLD_BA_WIN_SIZE4                         REG_FLD(4, 16)
#define PEER_CAP_DW07_FLD_BA_WIN_SIZE3                         REG_FLD(4, 12)
#define PEER_CAP_DW07_FLD_BA_WIN_SIZE2                         REG_FLD(4, 8)
#define PEER_CAP_DW07_FLD_BA_WIN_SIZE1                         REG_FLD(4, 4)
#define PEER_CAP_DW07_FLD_BA_WIN_SIZE0                         REG_FLD(4, 0)

#define PEER_CAP_DW08_FLD_CHK_PER                              REG_FLD(1, 31)
#define PEER_CAP_DW08_FLD_PARTIAL_AID                          REG_FLD(9, 20)
#define PEER_CAP_DW08_FLD_AC3_RTS_FAIL_CNT                     REG_FLD(5, 15)
#define PEER_CAP_DW08_FLD_AC2_RTS_FAIL_CNT                     REG_FLD(5, 10)
#define PEER_CAP_DW08_FLD_AC1_RTS_FAIL_CNT                     REG_FLD(5, 5)
#define PEER_CAP_DW08_FLD_AC0_RTS_FAIL_CNT                     REG_FLD(5, 0)

#define PEER_CAP_DW09_FLD_RATE_IDX                             REG_FLD(3, 29)
#define PEER_CAP_DW09_FLD_MPDU_OK_CNT                          REG_FLD(3, 26)
#define PEER_CAP_DW09_FLD_MPDU_FAIL_CNT                        REG_FLD(3, 23)
#define PEER_CAP_DW09_FLD_FCAP                                 REG_FLD(3, 20)
#define PEER_CAP_DW09_FLD_PRITX_ER106T                         REG_FLD(1, 19)
#define PEER_CAP_DW09_FLD_PRITX_DCM                            REG_FLD(1, 18)
#define PEER_CAP_DW09_FLD_PRITX_PLR                            REG_FLD(2, 16)
#define PEER_CAP_DW09_FLD_PRITX_ERSU                           REG_FLD(1, 15)
#define PEER_CAP_DW09_FLD_PRITX_SW_MODE                        REG_FLD(1, 14)
#define PEER_CAP_DW09_FLD_RX_AVG_MPDU_SIZE                     REG_FLD(14, 0)

#define AUTO_RATE_TABLE_DW10_FLD_RATE2                         REG_FLD(15, 16)
#define AUTO_RATE_TABLE_DW10_FLD_RATE1                         REG_FLD(15, 0)

#define AUTO_RATE_TABLE_DW11_FLD_RATE4                         REG_FLD(15, 16)
#define AUTO_RATE_TABLE_DW11_FLD_RATE3                         REG_FLD(15, 0)

#define AUTO_RATE_TABLE_DW12_FLD_RATE6                         REG_FLD(15, 16)
#define AUTO_RATE_TABLE_DW12_FLD_RATE5                         REG_FLD(15, 0)

#define AUTO_RATE_TABLE_DW13_FLD_RATE8                         REG_FLD(15, 16)
#define AUTO_RATE_TABLE_DW13_FLD_RATE7                         REG_FLD(15, 0)

#define AUTO_RATE_CTR_DW14_0_FLD_RATE1_FAIL_CNT                REG_FLD(16, 16)
#define AUTO_RATE_CTR_DW14_0_FLD_RATE1_TX_CNT                  REG_FLD(16, 0)

#define AUTO_RATE_CTR_DW14_1_FLD_CIPHER_SUIT_BIGTK             REG_FLD(3, 12)
#define AUTO_RATE_CTR_DW14_1_FLD_CIPHER_SUIT_IGTK              REG_FLD(3, 8)

#define AUTO_RATE_CTR_DW15_0_FLD_RATE3_OK_CNT                  REG_FLD(16, 16)
#define AUTO_RATE_CTR_DW15_0_FLD_RATE2_OK_CNT                  REG_FLD(16, 0)

#define AUTO_RATE_CTR_DW16_0_FLD_CURRENT_BW_FAIL_CNT           REG_FLD(16, 16)
#define AUTO_RATE_CTR_DW16_0_FLD_CURRENT_BW_TX_CNT             REG_FLD(16, 0)

#define AUTO_RATE_CTR_DW17_0_FLD_OTHER_BW_FAIL_CNT             REG_FLD(16, 16)
#define AUTO_RATE_CTR_DW17_0_FLD_OTHER_BW_TX_CNT               REG_FLD(16, 0)

#define PPDU_CTR_DW18_0_FLD_RTS_FAIL_CNT                       REG_FLD(16, 16)
#define PPDU_CTR_DW18_0_FLD_RTS_OK_CNT                         REG_FLD(16, 0)

#define PPDU_CTR_DW19_0_FLD_MGNT_RETRY_CNT                     REG_FLD(16, 16)
#define PPDU_CTR_DW19_0_FLD_DATA_RETRY_CNT                     REG_FLD(16, 0)

#define ADM_CTR_DW20_0_FLD_AC0_CTT_CDT_CRB__CTT                REG_FLD(32, 0)

#define ADM_CTR_DW21_0_FLD_AC0_CTB_CRT_CTB__CRT                REG_FLD(32, 0)

#define ADM_CTR_DW22_0_FLD_AC1_CTT_CDT_CRB__CTMC               REG_FLD(32, 0)

#define ADM_CTR_DW23_0_FLD_AC1_CTB_CRT_CTB__CRMC               REG_FLD(32, 0)

#define ADM_CTR_DW24_0_FLD_AC2_CTT_CDT_CRB__CTDB               REG_FLD(32, 0)

#define ADM_CTR_DW25_0_FLD_AC2_CTB_CRT_CTB__CRDB               REG_FLD(32, 0)

#define ADM_CTR_DW26_0_FLD_AC3_CTT_CDT_CRB__CTODB              REG_FLD(32, 0)

#define ADM_CTR_DW27_0_FLD_AC3_CTB_CRT_CTB__                   REG_FLD(32, 0)

#define MLO_INFO_DW28_FLD_SECONDARY_MLD_BAND                   REG_FLD(2, 30)
#define MLO_INFO_DW28_FLD_RELATED_BAND1                        REG_FLD(2, 28)
#define MLO_INFO_DW28_FLD_RELATED_IDX1                         REG_FLD(12, 16)
#define MLO_INFO_DW28_FLD_PRIMARY_MLD_BAND                     REG_FLD(2, 14)
#define MLO_INFO_DW28_FLD_RELATED_BAND0                        REG_FLD(2, 12)
#define MLO_INFO_DW28_FLD_RELATED_IDX0                         REG_FLD(12, 0)

#define MLO_INFO_DW29_FLD_STR_BITMAP                           REG_FLD(3, 29)
#define MLO_INFO_DW29_FLD_EMLMR2                               REG_FLD(1, 27)
#define MLO_INFO_DW29_FLD_EMLSR2                               REG_FLD(1, 26)
#define MLO_INFO_DW29_FLD_EMLMR1                               REG_FLD(1, 25)
#define MLO_INFO_DW29_FLD_EMLSR1                               REG_FLD(1, 24)
#define MLO_INFO_DW29_FLD_EMLMR0                               REG_FLD(1, 23)
#define MLO_INFO_DW29_FLD_EMLSR0                               REG_FLD(1, 22)
#define MLO_INFO_DW29_FLD_OWN_MLD_ID                           REG_FLD(6, 16)
#define MLO_INFO_DW29_FLD_DISPATCH_POLICY7                     REG_FLD(2, 14)
#define MLO_INFO_DW29_FLD_DISPATCH_POLICY6                     REG_FLD(2, 12)
#define MLO_INFO_DW29_FLD_DISPATCH_POLICY5                     REG_FLD(2, 10)
#define MLO_INFO_DW29_FLD_DISPATCH_POLICY4                     REG_FLD(2, 8)
#define MLO_INFO_DW29_FLD_DISPATCH_POLICY3                     REG_FLD(2, 6)
#define MLO_INFO_DW29_FLD_DISPATCH_POLICY2                     REG_FLD(2, 4)
#define MLO_INFO_DW29_FLD_DISPATCH_POLICY1                     REG_FLD(2, 2)
#define MLO_INFO_DW29_FLD_DISPATCH_POLICY0                     REG_FLD(2, 0)

#define MLO_INFO_DW30_FLD_LINK_MGF                             REG_FLD(16, 16)
#define MLO_INFO_DW30_FLD_EMLSR_TRANS_DLY_IDX                  REG_FLD(2, 14)
#define MLO_INFO_DW30_FLD_DISPATCH_RATIO                       REG_FLD(7, 7)
#define MLO_INFO_DW30_FLD_DISPATCH_ORDER                       REG_FLD(7, 0)

#define RESP_INFO_DW31_FLD_ACK_EN                              REG_FLD(1, 31)
#define RESP_INFO_DW31_FLD_RXD_DUP_MODE                        REG_FLD(2, 29)
#define RESP_INFO_DW31_FLD_MPDU_SIZE                           REG_FLD(2, 27)
#define RESP_INFO_DW31_FLD_ALL_ACK                             REG_FLD(1, 26)
#define RESP_INFO_DW31_FLD_CASCAD                              REG_FLD(1, 25)
#define RESP_INFO_DW31_FLD_DROP                                REG_FLD(1, 24)
#define RESP_INFO_DW31_FLD_BFTX_TB                             REG_FLD(1, 23)

#define RX_DUP_INFO_DW32_FLD_RXD_DUP_WHITE_LIST                REG_FLD(12, 17)
#define RX_DUP_INFO_DW32_FLD_RXD_DUP_FOR_OM_CHG                REG_FLD(1, 16)
#define RX_DUP_INFO_DW32_FLD_OM_INFO_EHT                       REG_FLD(4, 12)
#define RX_DUP_INFO_DW32_FLD_OM_INFO                           REG_FLD(12, 0)

#define RX_STAT_DW33_FLD_AMSDU_CROSS_LG                        REG_FLD(1, 31)
#define RX_STAT_DW33_FLD_HT_AMSDU                              REG_FLD(1, 30)
#define RX_STAT_DW33_FLD_RAPID_REACTION_RATE                   REG_FLD(12, 16)
#define RX_STAT_DW33_FLD_USER_SNR                              REG_FLD(6, 9)
#define RX_STAT_DW33_FLD_USER_RSSI                             REG_FLD(9, 0)

#define RX_STAT_DW34_FLD_RESP_RCPI3                            REG_FLD(8, 24)
#define RX_STAT_DW34_FLD_RESP_RCPI2                            REG_FLD(8, 16)
#define RX_STAT_DW34_FLD_RESP_RCPI1                            REG_FLD(8, 8)
#define RX_STAT_DW34_FLD_RESP_RCPI0                            REG_FLD(8, 0)

#define RX_STAT_DW35_FLD_SNR_RX3                               REG_FLD(8, 24)
#define RX_STAT_DW35_FLD_SNR_RX2                               REG_FLD(8, 16)
#define RX_STAT_DW35_FLD_SNR_RX1                               REG_FLD(8, 8)
#define RX_STAT_DW35_FLD_SNR_RX0                               REG_FLD(8, 0)

#define VERSION_CODE_FLD_VERSION_CODE                          REG_FLD(32, 0)

#define PEER_INFO_DW00_GET_WPI_FLAG(reg32)                     REG_FLD_GET(PEER_INFO_DW00_FLD_WPI_FLAG, (reg32))
#define PEER_INFO_DW00_GET_RCA2(reg32)                         REG_FLD_GET(PEER_INFO_DW00_FLD_RCA2, (reg32))
#define PEER_INFO_DW00_GET_RV(reg32)                           REG_FLD_GET(PEER_INFO_DW00_FLD_RV, (reg32))
#define PEER_INFO_DW00_GET_BAND(reg32)                         REG_FLD_GET(PEER_INFO_DW00_FLD_BAND, (reg32))
#define PEER_INFO_DW00_GET_RCID(reg32)                         REG_FLD_GET(PEER_INFO_DW00_FLD_RCID, (reg32))
#define PEER_INFO_DW00_GET_KID(reg32)                          REG_FLD_GET(PEER_INFO_DW00_FLD_KID, (reg32))
#define PEER_INFO_DW00_GET_RCA1(reg32)                         REG_FLD_GET(PEER_INFO_DW00_FLD_RCA1, (reg32))
#define PEER_INFO_DW00_GET_MUAR(reg32)                         REG_FLD_GET(PEER_INFO_DW00_FLD_MUAR, (reg32))
#define PEER_INFO_DW00_GET_PEER_LINK_ADDRESS_47_32_(reg32)     REG_FLD_GET(PEER_INFO_DW00_FLD_PEER_LINK_ADDRESS_47_32_, (reg32))

#define PEER_INFO_DW01_GET_PEER_LINK_ADDRESS_31_0_(reg32)      REG_FLD_GET(PEER_INFO_DW01_FLD_PEER_LINK_ADDRESS_31_0_, (reg32))

#define PEER_CAP_DW02_GET_MESH(reg32)                          REG_FLD_GET(PEER_CAP_DW02_FLD_MESH, (reg32))
#define PEER_CAP_DW02_GET_EHT(reg32)                           REG_FLD_GET(PEER_CAP_DW02_FLD_EHT, (reg32))
#define PEER_CAP_DW02_GET_HE(reg32)                            REG_FLD_GET(PEER_CAP_DW02_FLD_HE, (reg32))
#define PEER_CAP_DW02_GET_VHT(reg32)                           REG_FLD_GET(PEER_CAP_DW02_FLD_VHT, (reg32))
#define PEER_CAP_DW02_GET_HT(reg32)                            REG_FLD_GET(PEER_CAP_DW02_FLD_HT, (reg32))
#define PEER_CAP_DW02_GET_QOS(reg32)                           REG_FLD_GET(PEER_CAP_DW02_FLD_QOS, (reg32))
#define PEER_CAP_DW02_GET_TX_PS(reg32)                         REG_FLD_GET(PEER_CAP_DW02_FLD_TX_PS, (reg32))
#define PEER_CAP_DW02_GET_UL(reg32)                            REG_FLD_GET(PEER_CAP_DW02_FLD_UL, (reg32))
#define PEER_CAP_DW02_GET_SW(reg32)                            REG_FLD_GET(PEER_CAP_DW02_FLD_SW, (reg32))
#define PEER_CAP_DW02_GET_TD(reg32)                            REG_FLD_GET(PEER_CAP_DW02_FLD_TD, (reg32))
#define PEER_CAP_DW02_GET_FD(reg32)                            REG_FLD_GET(PEER_CAP_DW02_FLD_FD, (reg32))
#define PEER_CAP_DW02_GET_CIPHER_SUIT_PGTK(reg32)              REG_FLD_GET(PEER_CAP_DW02_FLD_CIPHER_SUIT_PGTK, (reg32))
#define PEER_CAP_DW02_GET_DUAL_CTS_CAP(reg32)                  REG_FLD_GET(PEER_CAP_DW02_FLD_DUAL_CTS_CAP, (reg32))
#define PEER_CAP_DW02_GET_DUAL_PTEC_EN(reg32)                  REG_FLD_GET(PEER_CAP_DW02_FLD_DUAL_PTEC_EN, (reg32))
#define PEER_CAP_DW02_GET_GID_SU(reg32)                        REG_FLD_GET(PEER_CAP_DW02_FLD_GID_SU, (reg32))
#define PEER_CAP_DW02_GET_AID(reg32)                           REG_FLD_GET(PEER_CAP_DW02_FLD_AID, (reg32))

#define PEER_CAP_DW03_GET_IGN_FBK(reg32)                       REG_FLD_GET(PEER_CAP_DW03_FLD_IGN_FBK, (reg32))
#define PEER_CAP_DW03_GET_TBF_EHT(reg32)                       REG_FLD_GET(PEER_CAP_DW03_FLD_TBF_EHT, (reg32))
#define PEER_CAP_DW03_GET_TBF_HE(reg32)                        REG_FLD_GET(PEER_CAP_DW03_FLD_TBF_HE, (reg32))
#define PEER_CAP_DW03_GET_TBF_VHT(reg32)                       REG_FLD_GET(PEER_CAP_DW03_FLD_TBF_VHT, (reg32))
#define PEER_CAP_DW03_GET_TBF_HT(reg32)                        REG_FLD_GET(PEER_CAP_DW03_FLD_TBF_HT, (reg32))
#define PEER_CAP_DW03_GET_BYPASS_TXSMM(reg32)                  REG_FLD_GET(PEER_CAP_DW03_FLD_BYPASS_TXSMM, (reg32))
#define PEER_CAP_DW03_GET_ULPF(reg32)                          REG_FLD_GET(PEER_CAP_DW03_FLD_ULPF, (reg32))
#define PEER_CAP_DW03_GET_RIBF(reg32)                          REG_FLD_GET(PEER_CAP_DW03_FLD_RIBF, (reg32))
#define PEER_CAP_DW03_GET_ULPF_IDX(reg32)                      REG_FLD_GET(PEER_CAP_DW03_FLD_ULPF_IDX, (reg32))
#define PEER_CAP_DW03_GET_PFMU_IDX(reg32)                      REG_FLD_GET(PEER_CAP_DW03_FLD_PFMU_IDX, (reg32))
#define PEER_CAP_DW03_GET_EHT_LTF_SYM_NUM_OPT(reg32)           REG_FLD_GET(PEER_CAP_DW03_FLD_EHT_LTF_SYM_NUM_OPT, (reg32))
#define PEER_CAP_DW03_GET_BEAM_CHG(reg32)                      REG_FLD_GET(PEER_CAP_DW03_FLD_BEAM_CHG, (reg32))
#define PEER_CAP_DW03_GET_HDRT_MODE(reg32)                     REG_FLD_GET(PEER_CAP_DW03_FLD_HDRT_MODE, (reg32))
#define PEER_CAP_DW03_GET_EHT_SIG_MCS(reg32)                   REG_FLD_GET(PEER_CAP_DW03_FLD_EHT_SIG_MCS, (reg32))
#define PEER_CAP_DW03_GET_WMM_Q(reg32)                         REG_FLD_GET(PEER_CAP_DW03_FLD_WMM_Q, (reg32))

#define PEER_CAP_DW04_GET_BA_MODE(reg32)                       REG_FLD_GET(PEER_CAP_DW04_FLD_BA_MODE, (reg32))
#define PEER_CAP_DW04_GET_LDPC_EHT(reg32)                      REG_FLD_GET(PEER_CAP_DW04_FLD_LDPC_EHT, (reg32))
#define PEER_CAP_DW04_GET_LDPC_HE(reg32)                       REG_FLD_GET(PEER_CAP_DW04_FLD_LDPC_HE, (reg32))
#define PEER_CAP_DW04_GET_LDPC_VHT(reg32)                      REG_FLD_GET(PEER_CAP_DW04_FLD_LDPC_VHT, (reg32))
#define PEER_CAP_DW04_GET_LDPC_HT(reg32)                       REG_FLD_GET(PEER_CAP_DW04_FLD_LDPC_HT, (reg32))
#define PEER_CAP_DW04_GET_DIS_RHTR(reg32)                      REG_FLD_GET(PEER_CAP_DW04_FLD_DIS_RHTR, (reg32))
#define PEER_CAP_DW04_GET_PE(reg32)                            REG_FLD_GET(PEER_CAP_DW04_FLD_PE, (reg32))
#define PEER_CAP_DW04_GET_NEGOTIATED_WINSIZE7(reg32)           REG_FLD_GET(PEER_CAP_DW04_FLD_NEGOTIATED_WINSIZE7, (reg32))
#define PEER_CAP_DW04_GET_NEGOTIATED_WINSIZE6(reg32)           REG_FLD_GET(PEER_CAP_DW04_FLD_NEGOTIATED_WINSIZE6, (reg32))
#define PEER_CAP_DW04_GET_NEGOTIATED_WINSIZE5(reg32)           REG_FLD_GET(PEER_CAP_DW04_FLD_NEGOTIATED_WINSIZE5, (reg32))
#define PEER_CAP_DW04_GET_NEGOTIATED_WINSIZE4(reg32)           REG_FLD_GET(PEER_CAP_DW04_FLD_NEGOTIATED_WINSIZE4, (reg32))
#define PEER_CAP_DW04_GET_NEGOTIATED_WINSIZE3(reg32)           REG_FLD_GET(PEER_CAP_DW04_FLD_NEGOTIATED_WINSIZE3, (reg32))
#define PEER_CAP_DW04_GET_NEGOTIATED_WINSIZE2(reg32)           REG_FLD_GET(PEER_CAP_DW04_FLD_NEGOTIATED_WINSIZE2, (reg32))
#define PEER_CAP_DW04_GET_NEGOTIATED_WINSIZE1(reg32)           REG_FLD_GET(PEER_CAP_DW04_FLD_NEGOTIATED_WINSIZE1, (reg32))
#define PEER_CAP_DW04_GET_NEGOTIATED_WINSIZE0(reg32)           REG_FLD_GET(PEER_CAP_DW04_FLD_NEGOTIATED_WINSIZE0, (reg32))

#define PEER_CAP_DW05_GET_SKIP_TX(reg32)                       REG_FLD_GET(PEER_CAP_DW05_FLD_SKIP_TX, (reg32))
#define PEER_CAP_DW05_GET_PSM(reg32)                           REG_FLD_GET(PEER_CAP_DW05_FLD_PSM, (reg32))
#define PEER_CAP_DW05_GET_I_PSM(reg32)                         REG_FLD_GET(PEER_CAP_DW05_FLD_I_PSM, (reg32))
#define PEER_CAP_DW05_GET_DU_I_PSM(reg32)                      REG_FLD_GET(PEER_CAP_DW05_FLD_DU_I_PSM, (reg32))
#define PEER_CAP_DW05_GET_TXOP_PS_CAP(reg32)                   REG_FLD_GET(PEER_CAP_DW05_FLD_TXOP_PS_CAP, (reg32))
#define PEER_CAP_DW05_GET_DOPPL(reg32)                         REG_FLD_GET(PEER_CAP_DW05_FLD_DOPPL, (reg32))
#define PEER_CAP_DW05_GET_GI_EHT(reg32)                        REG_FLD_GET(PEER_CAP_DW05_FLD_GI_EHT, (reg32))
#define PEER_CAP_DW05_GET_LTF_EHT(reg32)                       REG_FLD_GET(PEER_CAP_DW05_FLD_LTF_EHT, (reg32))
#define PEER_CAP_DW05_GET_TX_POWER_OFFSET(reg32)               REG_FLD_GET(PEER_CAP_DW05_FLD_TX_POWER_OFFSET, (reg32))
#define PEER_CAP_DW05_GET_SR_ABORT(reg32)                      REG_FLD_GET(PEER_CAP_DW05_FLD_SR_ABORT, (reg32))
#define PEER_CAP_DW05_GET_SR_R(reg32)                          REG_FLD_GET(PEER_CAP_DW05_FLD_SR_R, (reg32))
#define PEER_CAP_DW05_GET_USR(reg32)                           REG_FLD_GET(PEER_CAP_DW05_FLD_USR, (reg32))
#define PEER_CAP_DW05_GET_MMSS(reg32)                          REG_FLD_GET(PEER_CAP_DW05_FLD_MMSS, (reg32))
#define PEER_CAP_DW05_GET_DYN_BW(reg32)                        REG_FLD_GET(PEER_CAP_DW05_FLD_DYN_BW, (reg32))
#define PEER_CAP_DW05_GET_SMPS(reg32)                          REG_FLD_GET(PEER_CAP_DW05_FLD_SMPS, (reg32))
#define PEER_CAP_DW05_GET_RTS(reg32)                           REG_FLD_GET(PEER_CAP_DW05_FLD_RTS, (reg32))
#define PEER_CAP_DW05_GET_BSA_EN(reg32)                        REG_FLD_GET(PEER_CAP_DW05_FLD_BSA_EN, (reg32))
#define PEER_CAP_DW05_GET_AF(reg32)                            REG_FLD_GET(PEER_CAP_DW05_FLD_AF, (reg32))

#define PEER_CAP_DW06_GET_G16_HE(reg32)                        REG_FLD_GET(PEER_CAP_DW06_FLD_G16_HE, (reg32))
#define PEER_CAP_DW06_GET_G8_HE(reg32)                         REG_FLD_GET(PEER_CAP_DW06_FLD_G8_HE, (reg32))
#define PEER_CAP_DW06_GET_G4_HE(reg32)                         REG_FLD_GET(PEER_CAP_DW06_FLD_G4_HE, (reg32))
#define PEER_CAP_DW06_GET_G2_HE(reg32)                         REG_FLD_GET(PEER_CAP_DW06_FLD_G2_HE, (reg32))
#define PEER_CAP_DW06_GET_G16_LTF(reg32)                       REG_FLD_GET(PEER_CAP_DW06_FLD_G16_LTF, (reg32))
#define PEER_CAP_DW06_GET_G8_LTF(reg32)                        REG_FLD_GET(PEER_CAP_DW06_FLD_G8_LTF, (reg32))
#define PEER_CAP_DW06_GET_G4_LTF(reg32)                        REG_FLD_GET(PEER_CAP_DW06_FLD_G4_LTF, (reg32))
#define PEER_CAP_DW06_GET_G2_LTF(reg32)                        REG_FLD_GET(PEER_CAP_DW06_FLD_G2_LTF, (reg32))
#define PEER_CAP_DW06_GET_G16(reg32)                           REG_FLD_GET(PEER_CAP_DW06_FLD_G16, (reg32))
#define PEER_CAP_DW06_GET_G8(reg32)                            REG_FLD_GET(PEER_CAP_DW06_FLD_G8, (reg32))
#define PEER_CAP_DW06_GET_G4(reg32)                            REG_FLD_GET(PEER_CAP_DW06_FLD_G4, (reg32))
#define PEER_CAP_DW06_GET_G2(reg32)                            REG_FLD_GET(PEER_CAP_DW06_FLD_G2, (reg32))
#define PEER_CAP_DW06_GET_SPE_IDX(reg32)                       REG_FLD_GET(PEER_CAP_DW06_FLD_SPE_IDX, (reg32))
#define PEER_CAP_DW06_GET_R(reg32)                             REG_FLD_GET(PEER_CAP_DW06_FLD_R, (reg32))
#define PEER_CAP_DW06_GET_RDGBA(reg32)                         REG_FLD_GET(PEER_CAP_DW06_FLD_RDGBA, (reg32))
#define PEER_CAP_DW06_GET_BAF_EN(reg32)                        REG_FLD_GET(PEER_CAP_DW06_FLD_BAF_EN, (reg32))
#define PEER_CAP_DW06_GET_DBNSS_EN(reg32)                      REG_FLD_GET(PEER_CAP_DW06_FLD_DBNSS_EN, (reg32))
#define PEER_CAP_DW06_GET_CBRN(reg32)                          REG_FLD_GET(PEER_CAP_DW06_FLD_CBRN, (reg32))

#define PEER_CAP_DW07_GET_BA_WIN_SIZE7(reg32)                  REG_FLD_GET(PEER_CAP_DW07_FLD_BA_WIN_SIZE7, (reg32))
#define PEER_CAP_DW07_GET_BA_WIN_SIZE6(reg32)                  REG_FLD_GET(PEER_CAP_DW07_FLD_BA_WIN_SIZE6, (reg32))
#define PEER_CAP_DW07_GET_BA_WIN_SIZE5(reg32)                  REG_FLD_GET(PEER_CAP_DW07_FLD_BA_WIN_SIZE5, (reg32))
#define PEER_CAP_DW07_GET_BA_WIN_SIZE4(reg32)                  REG_FLD_GET(PEER_CAP_DW07_FLD_BA_WIN_SIZE4, (reg32))
#define PEER_CAP_DW07_GET_BA_WIN_SIZE3(reg32)                  REG_FLD_GET(PEER_CAP_DW07_FLD_BA_WIN_SIZE3, (reg32))
#define PEER_CAP_DW07_GET_BA_WIN_SIZE2(reg32)                  REG_FLD_GET(PEER_CAP_DW07_FLD_BA_WIN_SIZE2, (reg32))
#define PEER_CAP_DW07_GET_BA_WIN_SIZE1(reg32)                  REG_FLD_GET(PEER_CAP_DW07_FLD_BA_WIN_SIZE1, (reg32))
#define PEER_CAP_DW07_GET_BA_WIN_SIZE0(reg32)                  REG_FLD_GET(PEER_CAP_DW07_FLD_BA_WIN_SIZE0, (reg32))

#define PEER_CAP_DW08_GET_CHK_PER(reg32)                       REG_FLD_GET(PEER_CAP_DW08_FLD_CHK_PER, (reg32))
#define PEER_CAP_DW08_GET_PARTIAL_AID(reg32)                   REG_FLD_GET(PEER_CAP_DW08_FLD_PARTIAL_AID, (reg32))
#define PEER_CAP_DW08_GET_AC3_RTS_FAIL_CNT(reg32)              REG_FLD_GET(PEER_CAP_DW08_FLD_AC3_RTS_FAIL_CNT, (reg32))
#define PEER_CAP_DW08_GET_AC2_RTS_FAIL_CNT(reg32)              REG_FLD_GET(PEER_CAP_DW08_FLD_AC2_RTS_FAIL_CNT, (reg32))
#define PEER_CAP_DW08_GET_AC1_RTS_FAIL_CNT(reg32)              REG_FLD_GET(PEER_CAP_DW08_FLD_AC1_RTS_FAIL_CNT, (reg32))
#define PEER_CAP_DW08_GET_AC0_RTS_FAIL_CNT(reg32)              REG_FLD_GET(PEER_CAP_DW08_FLD_AC0_RTS_FAIL_CNT, (reg32))

#define PEER_CAP_DW09_GET_RATE_IDX(reg32)                      REG_FLD_GET(PEER_CAP_DW09_FLD_RATE_IDX, (reg32))
#define PEER_CAP_DW09_GET_MPDU_OK_CNT(reg32)                   REG_FLD_GET(PEER_CAP_DW09_FLD_MPDU_OK_CNT, (reg32))
#define PEER_CAP_DW09_GET_MPDU_FAIL_CNT(reg32)                 REG_FLD_GET(PEER_CAP_DW09_FLD_MPDU_FAIL_CNT, (reg32))
#define PEER_CAP_DW09_GET_FCAP(reg32)                          REG_FLD_GET(PEER_CAP_DW09_FLD_FCAP, (reg32))
#define PEER_CAP_DW09_GET_PRITX_ER106T(reg32)                  REG_FLD_GET(PEER_CAP_DW09_FLD_PRITX_ER106T, (reg32))
#define PEER_CAP_DW09_GET_PRITX_DCM(reg32)                     REG_FLD_GET(PEER_CAP_DW09_FLD_PRITX_DCM, (reg32))
#define PEER_CAP_DW09_GET_PRITX_PLR(reg32)                     REG_FLD_GET(PEER_CAP_DW09_FLD_PRITX_PLR, (reg32))
#define PEER_CAP_DW09_GET_PRITX_ERSU(reg32)                    REG_FLD_GET(PEER_CAP_DW09_FLD_PRITX_ERSU, (reg32))
#define PEER_CAP_DW09_GET_PRITX_SW_MODE(reg32)                 REG_FLD_GET(PEER_CAP_DW09_FLD_PRITX_SW_MODE, (reg32))
#define PEER_CAP_DW09_GET_RX_AVG_MPDU_SIZE(reg32)              REG_FLD_GET(PEER_CAP_DW09_FLD_RX_AVG_MPDU_SIZE, (reg32))

#define AUTO_RATE_TABLE_DW10_GET_RATE2(reg32)                  REG_FLD_GET(AUTO_RATE_TABLE_DW10_FLD_RATE2, (reg32))
#define AUTO_RATE_TABLE_DW10_GET_RATE1(reg32)                  REG_FLD_GET(AUTO_RATE_TABLE_DW10_FLD_RATE1, (reg32))

#define AUTO_RATE_TABLE_DW11_GET_RATE4(reg32)                  REG_FLD_GET(AUTO_RATE_TABLE_DW11_FLD_RATE4, (reg32))
#define AUTO_RATE_TABLE_DW11_GET_RATE3(reg32)                  REG_FLD_GET(AUTO_RATE_TABLE_DW11_FLD_RATE3, (reg32))

#define AUTO_RATE_TABLE_DW12_GET_RATE6(reg32)                  REG_FLD_GET(AUTO_RATE_TABLE_DW12_FLD_RATE6, (reg32))
#define AUTO_RATE_TABLE_DW12_GET_RATE5(reg32)                  REG_FLD_GET(AUTO_RATE_TABLE_DW12_FLD_RATE5, (reg32))

#define AUTO_RATE_TABLE_DW13_GET_RATE8(reg32)                  REG_FLD_GET(AUTO_RATE_TABLE_DW13_FLD_RATE8, (reg32))
#define AUTO_RATE_TABLE_DW13_GET_RATE7(reg32)                  REG_FLD_GET(AUTO_RATE_TABLE_DW13_FLD_RATE7, (reg32))

#define AUTO_RATE_CTR_DW14_0_GET_RATE1_FAIL_CNT(reg32)         REG_FLD_GET(AUTO_RATE_CTR_DW14_0_FLD_RATE1_FAIL_CNT, (reg32))
#define AUTO_RATE_CTR_DW14_0_GET_RATE1_TX_CNT(reg32)           REG_FLD_GET(AUTO_RATE_CTR_DW14_0_FLD_RATE1_TX_CNT, (reg32))

#define AUTO_RATE_CTR_DW14_1_GET_CIPHER_SUIT_BIGTK(reg32)      REG_FLD_GET(AUTO_RATE_CTR_DW14_1_FLD_CIPHER_SUIT_BIGTK, (reg32))
#define AUTO_RATE_CTR_DW14_1_GET_CIPHER_SUIT_IGTK(reg32)       REG_FLD_GET(AUTO_RATE_CTR_DW14_1_FLD_CIPHER_SUIT_IGTK, (reg32))

#define AUTO_RATE_CTR_DW15_0_GET_RATE3_OK_CNT(reg32)           REG_FLD_GET(AUTO_RATE_CTR_DW15_0_FLD_RATE3_OK_CNT, (reg32))
#define AUTO_RATE_CTR_DW15_0_GET_RATE2_OK_CNT(reg32)           REG_FLD_GET(AUTO_RATE_CTR_DW15_0_FLD_RATE2_OK_CNT, (reg32))

#define AUTO_RATE_CTR_DW16_0_GET_CURRENT_BW_FAIL_CNT(reg32)    REG_FLD_GET(AUTO_RATE_CTR_DW16_0_FLD_CURRENT_BW_FAIL_CNT, (reg32))
#define AUTO_RATE_CTR_DW16_0_GET_CURRENT_BW_TX_CNT(reg32)      REG_FLD_GET(AUTO_RATE_CTR_DW16_0_FLD_CURRENT_BW_TX_CNT, (reg32))

#define AUTO_RATE_CTR_DW17_0_GET_OTHER_BW_FAIL_CNT(reg32)      REG_FLD_GET(AUTO_RATE_CTR_DW17_0_FLD_OTHER_BW_FAIL_CNT, (reg32))
#define AUTO_RATE_CTR_DW17_0_GET_OTHER_BW_TX_CNT(reg32)        REG_FLD_GET(AUTO_RATE_CTR_DW17_0_FLD_OTHER_BW_TX_CNT, (reg32))

#define PPDU_CTR_DW18_0_GET_RTS_FAIL_CNT(reg32)                REG_FLD_GET(PPDU_CTR_DW18_0_FLD_RTS_FAIL_CNT, (reg32))
#define PPDU_CTR_DW18_0_GET_RTS_OK_CNT(reg32)                  REG_FLD_GET(PPDU_CTR_DW18_0_FLD_RTS_OK_CNT, (reg32))

#define PPDU_CTR_DW19_0_GET_MGNT_RETRY_CNT(reg32)              REG_FLD_GET(PPDU_CTR_DW19_0_FLD_MGNT_RETRY_CNT, (reg32))
#define PPDU_CTR_DW19_0_GET_DATA_RETRY_CNT(reg32)              REG_FLD_GET(PPDU_CTR_DW19_0_FLD_DATA_RETRY_CNT, (reg32))

#define ADM_CTR_DW20_0_GET_AC0_CTT_CDT_CRB__CTT(reg32)         REG_FLD_GET(ADM_CTR_DW20_0_FLD_AC0_CTT_CDT_CRB__CTT, (reg32))

#define ADM_CTR_DW21_0_GET_AC0_CTB_CRT_CTB__CRT(reg32)         REG_FLD_GET(ADM_CTR_DW21_0_FLD_AC0_CTB_CRT_CTB__CRT, (reg32))

#define ADM_CTR_DW22_0_GET_AC1_CTT_CDT_CRB__CTMC(reg32)        REG_FLD_GET(ADM_CTR_DW22_0_FLD_AC1_CTT_CDT_CRB__CTMC, (reg32))

#define ADM_CTR_DW23_0_GET_AC1_CTB_CRT_CTB__CRMC(reg32)        REG_FLD_GET(ADM_CTR_DW23_0_FLD_AC1_CTB_CRT_CTB__CRMC, (reg32))

#define ADM_CTR_DW24_0_GET_AC2_CTT_CDT_CRB__CTDB(reg32)        REG_FLD_GET(ADM_CTR_DW24_0_FLD_AC2_CTT_CDT_CRB__CTDB, (reg32))

#define ADM_CTR_DW25_0_GET_AC2_CTB_CRT_CTB__CRDB(reg32)        REG_FLD_GET(ADM_CTR_DW25_0_FLD_AC2_CTB_CRT_CTB__CRDB, (reg32))

#define ADM_CTR_DW26_0_GET_AC3_CTT_CDT_CRB__CTODB(reg32)       REG_FLD_GET(ADM_CTR_DW26_0_FLD_AC3_CTT_CDT_CRB__CTODB, (reg32))

#define ADM_CTR_DW27_0_GET_AC3_CTB_CRT_CTB__(reg32)            REG_FLD_GET(ADM_CTR_DW27_0_FLD_AC3_CTB_CRT_CTB__, (reg32))

#define MLO_INFO_DW28_GET_SECONDARY_MLD_BAND(reg32)            REG_FLD_GET(MLO_INFO_DW28_FLD_SECONDARY_MLD_BAND, (reg32))
#define MLO_INFO_DW28_GET_RELATED_BAND1(reg32)                 REG_FLD_GET(MLO_INFO_DW28_FLD_RELATED_BAND1, (reg32))
#define MLO_INFO_DW28_GET_RELATED_IDX1(reg32)                  REG_FLD_GET(MLO_INFO_DW28_FLD_RELATED_IDX1, (reg32))
#define MLO_INFO_DW28_GET_PRIMARY_MLD_BAND(reg32)              REG_FLD_GET(MLO_INFO_DW28_FLD_PRIMARY_MLD_BAND, (reg32))
#define MLO_INFO_DW28_GET_RELATED_BAND0(reg32)                 REG_FLD_GET(MLO_INFO_DW28_FLD_RELATED_BAND0, (reg32))
#define MLO_INFO_DW28_GET_RELATED_IDX0(reg32)                  REG_FLD_GET(MLO_INFO_DW28_FLD_RELATED_IDX0, (reg32))

#define MLO_INFO_DW29_GET_STR_BITMAP(reg32)                    REG_FLD_GET(MLO_INFO_DW29_FLD_STR_BITMAP, (reg32))
#define MLO_INFO_DW29_GET_EMLMR2(reg32)                        REG_FLD_GET(MLO_INFO_DW29_FLD_EMLMR2, (reg32))
#define MLO_INFO_DW29_GET_EMLSR2(reg32)                        REG_FLD_GET(MLO_INFO_DW29_FLD_EMLSR2, (reg32))
#define MLO_INFO_DW29_GET_EMLMR1(reg32)                        REG_FLD_GET(MLO_INFO_DW29_FLD_EMLMR1, (reg32))
#define MLO_INFO_DW29_GET_EMLSR1(reg32)                        REG_FLD_GET(MLO_INFO_DW29_FLD_EMLSR1, (reg32))
#define MLO_INFO_DW29_GET_EMLMR0(reg32)                        REG_FLD_GET(MLO_INFO_DW29_FLD_EMLMR0, (reg32))
#define MLO_INFO_DW29_GET_EMLSR0(reg32)                        REG_FLD_GET(MLO_INFO_DW29_FLD_EMLSR0, (reg32))
#define MLO_INFO_DW29_GET_OWN_MLD_ID(reg32)                    REG_FLD_GET(MLO_INFO_DW29_FLD_OWN_MLD_ID, (reg32))
#define MLO_INFO_DW29_GET_DISPATCH_POLICY7(reg32)              REG_FLD_GET(MLO_INFO_DW29_FLD_DISPATCH_POLICY7, (reg32))
#define MLO_INFO_DW29_GET_DISPATCH_POLICY6(reg32)              REG_FLD_GET(MLO_INFO_DW29_FLD_DISPATCH_POLICY6, (reg32))
#define MLO_INFO_DW29_GET_DISPATCH_POLICY5(reg32)              REG_FLD_GET(MLO_INFO_DW29_FLD_DISPATCH_POLICY5, (reg32))
#define MLO_INFO_DW29_GET_DISPATCH_POLICY4(reg32)              REG_FLD_GET(MLO_INFO_DW29_FLD_DISPATCH_POLICY4, (reg32))
#define MLO_INFO_DW29_GET_DISPATCH_POLICY3(reg32)              REG_FLD_GET(MLO_INFO_DW29_FLD_DISPATCH_POLICY3, (reg32))
#define MLO_INFO_DW29_GET_DISPATCH_POLICY2(reg32)              REG_FLD_GET(MLO_INFO_DW29_FLD_DISPATCH_POLICY2, (reg32))
#define MLO_INFO_DW29_GET_DISPATCH_POLICY1(reg32)              REG_FLD_GET(MLO_INFO_DW29_FLD_DISPATCH_POLICY1, (reg32))
#define MLO_INFO_DW29_GET_DISPATCH_POLICY0(reg32)              REG_FLD_GET(MLO_INFO_DW29_FLD_DISPATCH_POLICY0, (reg32))

#define MLO_INFO_DW30_GET_LINK_MGF(reg32)                      REG_FLD_GET(MLO_INFO_DW30_FLD_LINK_MGF, (reg32))
#define MLO_INFO_DW30_GET_EMLSR_TRANS_DLY_IDX(reg32)           REG_FLD_GET(MLO_INFO_DW30_FLD_EMLSR_TRANS_DLY_IDX, (reg32))
#define MLO_INFO_DW30_GET_DISPATCH_RATIO(reg32)                REG_FLD_GET(MLO_INFO_DW30_FLD_DISPATCH_RATIO, (reg32))
#define MLO_INFO_DW30_GET_DISPATCH_ORDER(reg32)                REG_FLD_GET(MLO_INFO_DW30_FLD_DISPATCH_ORDER, (reg32))

#define RESP_INFO_DW31_GET_ACK_EN(reg32)                       REG_FLD_GET(RESP_INFO_DW31_FLD_ACK_EN, (reg32))
#define RESP_INFO_DW31_GET_RXD_DUP_MODE(reg32)                 REG_FLD_GET(RESP_INFO_DW31_FLD_RXD_DUP_MODE, (reg32))
#define RESP_INFO_DW31_GET_MPDU_SIZE(reg32)                    REG_FLD_GET(RESP_INFO_DW31_FLD_MPDU_SIZE, (reg32))
#define RESP_INFO_DW31_GET_ALL_ACK(reg32)                      REG_FLD_GET(RESP_INFO_DW31_FLD_ALL_ACK, (reg32))
#define RESP_INFO_DW31_GET_CASCAD(reg32)                       REG_FLD_GET(RESP_INFO_DW31_FLD_CASCAD, (reg32))
#define RESP_INFO_DW31_GET_DROP(reg32)                         REG_FLD_GET(RESP_INFO_DW31_FLD_DROP, (reg32))
#define RESP_INFO_DW31_GET_BFTX_TB(reg32)                      REG_FLD_GET(RESP_INFO_DW31_FLD_BFTX_TB, (reg32))

#define RX_DUP_INFO_DW32_GET_RXD_DUP_WHITE_LIST(reg32)         REG_FLD_GET(RX_DUP_INFO_DW32_FLD_RXD_DUP_WHITE_LIST, (reg32))
#define RX_DUP_INFO_DW32_GET_RXD_DUP_FOR_OM_CHG(reg32)         REG_FLD_GET(RX_DUP_INFO_DW32_FLD_RXD_DUP_FOR_OM_CHG, (reg32))
#define RX_DUP_INFO_DW32_GET_OM_INFO_EHT(reg32)                REG_FLD_GET(RX_DUP_INFO_DW32_FLD_OM_INFO_EHT, (reg32))
#define RX_DUP_INFO_DW32_GET_OM_INFO(reg32)                    REG_FLD_GET(RX_DUP_INFO_DW32_FLD_OM_INFO, (reg32))

#define RX_STAT_DW33_GET_AMSDU_CROSS_LG(reg32)                 REG_FLD_GET(RX_STAT_DW33_FLD_AMSDU_CROSS_LG, (reg32))
#define RX_STAT_DW33_GET_HT_AMSDU(reg32)                       REG_FLD_GET(RX_STAT_DW33_FLD_HT_AMSDU, (reg32))
#define RX_STAT_DW33_GET_RAPID_REACTION_RATE(reg32)            REG_FLD_GET(RX_STAT_DW33_FLD_RAPID_REACTION_RATE, (reg32))
#define RX_STAT_DW33_GET_USER_SNR(reg32)                       REG_FLD_GET(RX_STAT_DW33_FLD_USER_SNR, (reg32))
#define RX_STAT_DW33_GET_USER_RSSI(reg32)                      REG_FLD_GET(RX_STAT_DW33_FLD_USER_RSSI, (reg32))

#define RX_STAT_DW34_GET_RESP_RCPI3(reg32)                     REG_FLD_GET(RX_STAT_DW34_FLD_RESP_RCPI3, (reg32))
#define RX_STAT_DW34_GET_RESP_RCPI2(reg32)                     REG_FLD_GET(RX_STAT_DW34_FLD_RESP_RCPI2, (reg32))
#define RX_STAT_DW34_GET_RESP_RCPI1(reg32)                     REG_FLD_GET(RX_STAT_DW34_FLD_RESP_RCPI1, (reg32))
#define RX_STAT_DW34_GET_RESP_RCPI0(reg32)                     REG_FLD_GET(RX_STAT_DW34_FLD_RESP_RCPI0, (reg32))

#define RX_STAT_DW35_GET_SNR_RX3(reg32)                        REG_FLD_GET(RX_STAT_DW35_FLD_SNR_RX3, (reg32))
#define RX_STAT_DW35_GET_SNR_RX2(reg32)                        REG_FLD_GET(RX_STAT_DW35_FLD_SNR_RX2, (reg32))
#define RX_STAT_DW35_GET_SNR_RX1(reg32)                        REG_FLD_GET(RX_STAT_DW35_FLD_SNR_RX1, (reg32))
#define RX_STAT_DW35_GET_SNR_RX0(reg32)                        REG_FLD_GET(RX_STAT_DW35_FLD_SNR_RX0, (reg32))

#define VERSION_CODE_GET_VERSION_CODE(reg32)                   REG_FLD_GET(VERSION_CODE_FLD_VERSION_CODE, (reg32))

#define PEER_INFO_DW00_SET_WPI_FLAG(reg32, val)                REG_FLD_SET(PEER_INFO_DW00_FLD_WPI_FLAG, (reg32), (val))
#define PEER_INFO_DW00_SET_RCA2(reg32, val)                    REG_FLD_SET(PEER_INFO_DW00_FLD_RCA2, (reg32), (val))
#define PEER_INFO_DW00_SET_RV(reg32, val)                      REG_FLD_SET(PEER_INFO_DW00_FLD_RV, (reg32), (val))
#define PEER_INFO_DW00_SET_BAND(reg32, val)                    REG_FLD_SET(PEER_INFO_DW00_FLD_BAND, (reg32), (val))
#define PEER_INFO_DW00_SET_RCID(reg32, val)                    REG_FLD_SET(PEER_INFO_DW00_FLD_RCID, (reg32), (val))
#define PEER_INFO_DW00_SET_KID(reg32, val)                     REG_FLD_SET(PEER_INFO_DW00_FLD_KID, (reg32), (val))
#define PEER_INFO_DW00_SET_RCA1(reg32, val)                    REG_FLD_SET(PEER_INFO_DW00_FLD_RCA1, (reg32), (val))
#define PEER_INFO_DW00_SET_MUAR(reg32, val)                    REG_FLD_SET(PEER_INFO_DW00_FLD_MUAR, (reg32), (val))
#define PEER_INFO_DW00_SET_PEER_LINK_ADDRESS_47_32_(reg32, val) REG_FLD_SET(PEER_INFO_DW00_FLD_PEER_LINK_ADDRESS_47_32_, (reg32), (val))

#define PEER_INFO_DW01_SET_PEER_LINK_ADDRESS_31_0_(reg32, val) REG_FLD_SET(PEER_INFO_DW01_FLD_PEER_LINK_ADDRESS_31_0_, (reg32), (val))

#define PEER_CAP_DW02_SET_MESH(reg32, val)                     REG_FLD_SET(PEER_CAP_DW02_FLD_MESH, (reg32), (val))
#define PEER_CAP_DW02_SET_EHT(reg32, val)                      REG_FLD_SET(PEER_CAP_DW02_FLD_EHT, (reg32), (val))
#define PEER_CAP_DW02_SET_HE(reg32, val)                       REG_FLD_SET(PEER_CAP_DW02_FLD_HE, (reg32), (val))
#define PEER_CAP_DW02_SET_VHT(reg32, val)                      REG_FLD_SET(PEER_CAP_DW02_FLD_VHT, (reg32), (val))
#define PEER_CAP_DW02_SET_HT(reg32, val)                       REG_FLD_SET(PEER_CAP_DW02_FLD_HT, (reg32), (val))
#define PEER_CAP_DW02_SET_QOS(reg32, val)                      REG_FLD_SET(PEER_CAP_DW02_FLD_QOS, (reg32), (val))
#define PEER_CAP_DW02_SET_TX_PS(reg32, val)                    REG_FLD_SET(PEER_CAP_DW02_FLD_TX_PS, (reg32), (val))
#define PEER_CAP_DW02_SET_UL(reg32, val)                       REG_FLD_SET(PEER_CAP_DW02_FLD_UL, (reg32), (val))
#define PEER_CAP_DW02_SET_SW(reg32, val)                       REG_FLD_SET(PEER_CAP_DW02_FLD_SW, (reg32), (val))
#define PEER_CAP_DW02_SET_TD(reg32, val)                       REG_FLD_SET(PEER_CAP_DW02_FLD_TD, (reg32), (val))
#define PEER_CAP_DW02_SET_FD(reg32, val)                       REG_FLD_SET(PEER_CAP_DW02_FLD_FD, (reg32), (val))
#define PEER_CAP_DW02_SET_CIPHER_SUIT_PGTK(reg32, val)         REG_FLD_SET(PEER_CAP_DW02_FLD_CIPHER_SUIT_PGTK, (reg32), (val))
#define PEER_CAP_DW02_SET_DUAL_CTS_CAP(reg32, val)             REG_FLD_SET(PEER_CAP_DW02_FLD_DUAL_CTS_CAP, (reg32), (val))
#define PEER_CAP_DW02_SET_DUAL_PTEC_EN(reg32, val)             REG_FLD_SET(PEER_CAP_DW02_FLD_DUAL_PTEC_EN, (reg32), (val))
#define PEER_CAP_DW02_SET_GID_SU(reg32, val)                   REG_FLD_SET(PEER_CAP_DW02_FLD_GID_SU, (reg32), (val))
#define PEER_CAP_DW02_SET_AID(reg32, val)                      REG_FLD_SET(PEER_CAP_DW02_FLD_AID, (reg32), (val))

#define PEER_CAP_DW03_SET_IGN_FBK(reg32, val)                  REG_FLD_SET(PEER_CAP_DW03_FLD_IGN_FBK, (reg32), (val))
#define PEER_CAP_DW03_SET_TBF_EHT(reg32, val)                  REG_FLD_SET(PEER_CAP_DW03_FLD_TBF_EHT, (reg32), (val))
#define PEER_CAP_DW03_SET_TBF_HE(reg32, val)                   REG_FLD_SET(PEER_CAP_DW03_FLD_TBF_HE, (reg32), (val))
#define PEER_CAP_DW03_SET_TBF_VHT(reg32, val)                  REG_FLD_SET(PEER_CAP_DW03_FLD_TBF_VHT, (reg32), (val))
#define PEER_CAP_DW03_SET_TBF_HT(reg32, val)                   REG_FLD_SET(PEER_CAP_DW03_FLD_TBF_HT, (reg32), (val))
#define PEER_CAP_DW03_SET_BYPASS_TXSMM(reg32, val)             REG_FLD_SET(PEER_CAP_DW03_FLD_BYPASS_TXSMM, (reg32), (val))
#define PEER_CAP_DW03_SET_ULPF(reg32, val)                     REG_FLD_SET(PEER_CAP_DW03_FLD_ULPF, (reg32), (val))
#define PEER_CAP_DW03_SET_RIBF(reg32, val)                     REG_FLD_SET(PEER_CAP_DW03_FLD_RIBF, (reg32), (val))
#define PEER_CAP_DW03_SET_ULPF_IDX(reg32, val)                 REG_FLD_SET(PEER_CAP_DW03_FLD_ULPF_IDX, (reg32), (val))
#define PEER_CAP_DW03_SET_PFMU_IDX(reg32, val)                 REG_FLD_SET(PEER_CAP_DW03_FLD_PFMU_IDX, (reg32), (val))
#define PEER_CAP_DW03_SET_EHT_LTF_SYM_NUM_OPT(reg32, val)      REG_FLD_SET(PEER_CAP_DW03_FLD_EHT_LTF_SYM_NUM_OPT, (reg32), (val))
#define PEER_CAP_DW03_SET_BEAM_CHG(reg32, val)                 REG_FLD_SET(PEER_CAP_DW03_FLD_BEAM_CHG, (reg32), (val))
#define PEER_CAP_DW03_SET_HDRT_MODE(reg32, val)                REG_FLD_SET(PEER_CAP_DW03_FLD_HDRT_MODE, (reg32), (val))
#define PEER_CAP_DW03_SET_EHT_SIG_MCS(reg32, val)              REG_FLD_SET(PEER_CAP_DW03_FLD_EHT_SIG_MCS, (reg32), (val))
#define PEER_CAP_DW03_SET_WMM_Q(reg32, val)                    REG_FLD_SET(PEER_CAP_DW03_FLD_WMM_Q, (reg32), (val))

#define PEER_CAP_DW04_SET_BA_MODE(reg32, val)                  REG_FLD_SET(PEER_CAP_DW04_FLD_BA_MODE, (reg32), (val))
#define PEER_CAP_DW04_SET_LDPC_EHT(reg32, val)                 REG_FLD_SET(PEER_CAP_DW04_FLD_LDPC_EHT, (reg32), (val))
#define PEER_CAP_DW04_SET_LDPC_HE(reg32, val)                  REG_FLD_SET(PEER_CAP_DW04_FLD_LDPC_HE, (reg32), (val))
#define PEER_CAP_DW04_SET_LDPC_VHT(reg32, val)                 REG_FLD_SET(PEER_CAP_DW04_FLD_LDPC_VHT, (reg32), (val))
#define PEER_CAP_DW04_SET_LDPC_HT(reg32, val)                  REG_FLD_SET(PEER_CAP_DW04_FLD_LDPC_HT, (reg32), (val))
#define PEER_CAP_DW04_SET_DIS_RHTR(reg32, val)                 REG_FLD_SET(PEER_CAP_DW04_FLD_DIS_RHTR, (reg32), (val))
#define PEER_CAP_DW04_SET_PE(reg32, val)                       REG_FLD_SET(PEER_CAP_DW04_FLD_PE, (reg32), (val))
#define PEER_CAP_DW04_SET_NEGOTIATED_WINSIZE7(reg32, val)      REG_FLD_SET(PEER_CAP_DW04_FLD_NEGOTIATED_WINSIZE7, (reg32), (val))
#define PEER_CAP_DW04_SET_NEGOTIATED_WINSIZE6(reg32, val)      REG_FLD_SET(PEER_CAP_DW04_FLD_NEGOTIATED_WINSIZE6, (reg32), (val))
#define PEER_CAP_DW04_SET_NEGOTIATED_WINSIZE5(reg32, val)      REG_FLD_SET(PEER_CAP_DW04_FLD_NEGOTIATED_WINSIZE5, (reg32), (val))
#define PEER_CAP_DW04_SET_NEGOTIATED_WINSIZE4(reg32, val)      REG_FLD_SET(PEER_CAP_DW04_FLD_NEGOTIATED_WINSIZE4, (reg32), (val))
#define PEER_CAP_DW04_SET_NEGOTIATED_WINSIZE3(reg32, val)      REG_FLD_SET(PEER_CAP_DW04_FLD_NEGOTIATED_WINSIZE3, (reg32), (val))
#define PEER_CAP_DW04_SET_NEGOTIATED_WINSIZE2(reg32, val)      REG_FLD_SET(PEER_CAP_DW04_FLD_NEGOTIATED_WINSIZE2, (reg32), (val))
#define PEER_CAP_DW04_SET_NEGOTIATED_WINSIZE1(reg32, val)      REG_FLD_SET(PEER_CAP_DW04_FLD_NEGOTIATED_WINSIZE1, (reg32), (val))
#define PEER_CAP_DW04_SET_NEGOTIATED_WINSIZE0(reg32, val)      REG_FLD_SET(PEER_CAP_DW04_FLD_NEGOTIATED_WINSIZE0, (reg32), (val))

#define PEER_CAP_DW05_SET_SKIP_TX(reg32, val)                  REG_FLD_SET(PEER_CAP_DW05_FLD_SKIP_TX, (reg32), (val))
#define PEER_CAP_DW05_SET_PSM(reg32, val)                      REG_FLD_SET(PEER_CAP_DW05_FLD_PSM, (reg32), (val))
#define PEER_CAP_DW05_SET_I_PSM(reg32, val)                    REG_FLD_SET(PEER_CAP_DW05_FLD_I_PSM, (reg32), (val))
#define PEER_CAP_DW05_SET_DU_I_PSM(reg32, val)                 REG_FLD_SET(PEER_CAP_DW05_FLD_DU_I_PSM, (reg32), (val))
#define PEER_CAP_DW05_SET_TXOP_PS_CAP(reg32, val)              REG_FLD_SET(PEER_CAP_DW05_FLD_TXOP_PS_CAP, (reg32), (val))
#define PEER_CAP_DW05_SET_DOPPL(reg32, val)                    REG_FLD_SET(PEER_CAP_DW05_FLD_DOPPL, (reg32), (val))
#define PEER_CAP_DW05_SET_GI_EHT(reg32, val)                   REG_FLD_SET(PEER_CAP_DW05_FLD_GI_EHT, (reg32), (val))
#define PEER_CAP_DW05_SET_LTF_EHT(reg32, val)                  REG_FLD_SET(PEER_CAP_DW05_FLD_LTF_EHT, (reg32), (val))
#define PEER_CAP_DW05_SET_TX_POWER_OFFSET(reg32, val)          REG_FLD_SET(PEER_CAP_DW05_FLD_TX_POWER_OFFSET, (reg32), (val))
#define PEER_CAP_DW05_SET_SR_ABORT(reg32, val)                 REG_FLD_SET(PEER_CAP_DW05_FLD_SR_ABORT, (reg32), (val))
#define PEER_CAP_DW05_SET_SR_R(reg32, val)                     REG_FLD_SET(PEER_CAP_DW05_FLD_SR_R, (reg32), (val))
#define PEER_CAP_DW05_SET_USR(reg32, val)                      REG_FLD_SET(PEER_CAP_DW05_FLD_USR, (reg32), (val))
#define PEER_CAP_DW05_SET_MMSS(reg32, val)                     REG_FLD_SET(PEER_CAP_DW05_FLD_MMSS, (reg32), (val))
#define PEER_CAP_DW05_SET_DYN_BW(reg32, val)                   REG_FLD_SET(PEER_CAP_DW05_FLD_DYN_BW, (reg32), (val))
#define PEER_CAP_DW05_SET_SMPS(reg32, val)                     REG_FLD_SET(PEER_CAP_DW05_FLD_SMPS, (reg32), (val))
#define PEER_CAP_DW05_SET_RTS(reg32, val)                      REG_FLD_SET(PEER_CAP_DW05_FLD_RTS, (reg32), (val))
#define PEER_CAP_DW05_SET_BSA_EN(reg32, val)                   REG_FLD_SET(PEER_CAP_DW05_FLD_BSA_EN, (reg32), (val))
#define PEER_CAP_DW05_SET_AF(reg32, val)                       REG_FLD_SET(PEER_CAP_DW05_FLD_AF, (reg32), (val))

#define PEER_CAP_DW06_SET_G16_HE(reg32, val)                   REG_FLD_SET(PEER_CAP_DW06_FLD_G16_HE, (reg32), (val))
#define PEER_CAP_DW06_SET_G8_HE(reg32, val)                    REG_FLD_SET(PEER_CAP_DW06_FLD_G8_HE, (reg32), (val))
#define PEER_CAP_DW06_SET_G4_HE(reg32, val)                    REG_FLD_SET(PEER_CAP_DW06_FLD_G4_HE, (reg32), (val))
#define PEER_CAP_DW06_SET_G2_HE(reg32, val)                    REG_FLD_SET(PEER_CAP_DW06_FLD_G2_HE, (reg32), (val))
#define PEER_CAP_DW06_SET_G16_LTF(reg32, val)                  REG_FLD_SET(PEER_CAP_DW06_FLD_G16_LTF, (reg32), (val))
#define PEER_CAP_DW06_SET_G8_LTF(reg32, val)                   REG_FLD_SET(PEER_CAP_DW06_FLD_G8_LTF, (reg32), (val))
#define PEER_CAP_DW06_SET_G4_LTF(reg32, val)                   REG_FLD_SET(PEER_CAP_DW06_FLD_G4_LTF, (reg32), (val))
#define PEER_CAP_DW06_SET_G2_LTF(reg32, val)                   REG_FLD_SET(PEER_CAP_DW06_FLD_G2_LTF, (reg32), (val))
#define PEER_CAP_DW06_SET_G16(reg32, val)                      REG_FLD_SET(PEER_CAP_DW06_FLD_G16, (reg32), (val))
#define PEER_CAP_DW06_SET_G8(reg32, val)                       REG_FLD_SET(PEER_CAP_DW06_FLD_G8, (reg32), (val))
#define PEER_CAP_DW06_SET_G4(reg32, val)                       REG_FLD_SET(PEER_CAP_DW06_FLD_G4, (reg32), (val))
#define PEER_CAP_DW06_SET_G2(reg32, val)                       REG_FLD_SET(PEER_CAP_DW06_FLD_G2, (reg32), (val))
#define PEER_CAP_DW06_SET_SPE_IDX(reg32, val)                  REG_FLD_SET(PEER_CAP_DW06_FLD_SPE_IDX, (reg32), (val))
#define PEER_CAP_DW06_SET_R(reg32, val)                        REG_FLD_SET(PEER_CAP_DW06_FLD_R, (reg32), (val))
#define PEER_CAP_DW06_SET_RDGBA(reg32, val)                    REG_FLD_SET(PEER_CAP_DW06_FLD_RDGBA, (reg32), (val))
#define PEER_CAP_DW06_SET_BAF_EN(reg32, val)                   REG_FLD_SET(PEER_CAP_DW06_FLD_BAF_EN, (reg32), (val))
#define PEER_CAP_DW06_SET_DBNSS_EN(reg32, val)                 REG_FLD_SET(PEER_CAP_DW06_FLD_DBNSS_EN, (reg32), (val))
#define PEER_CAP_DW06_SET_CBRN(reg32, val)                     REG_FLD_SET(PEER_CAP_DW06_FLD_CBRN, (reg32), (val))

#define PEER_CAP_DW07_SET_BA_WIN_SIZE7(reg32, val)             REG_FLD_SET(PEER_CAP_DW07_FLD_BA_WIN_SIZE7, (reg32), (val))
#define PEER_CAP_DW07_SET_BA_WIN_SIZE6(reg32, val)             REG_FLD_SET(PEER_CAP_DW07_FLD_BA_WIN_SIZE6, (reg32), (val))
#define PEER_CAP_DW07_SET_BA_WIN_SIZE5(reg32, val)             REG_FLD_SET(PEER_CAP_DW07_FLD_BA_WIN_SIZE5, (reg32), (val))
#define PEER_CAP_DW07_SET_BA_WIN_SIZE4(reg32, val)             REG_FLD_SET(PEER_CAP_DW07_FLD_BA_WIN_SIZE4, (reg32), (val))
#define PEER_CAP_DW07_SET_BA_WIN_SIZE3(reg32, val)             REG_FLD_SET(PEER_CAP_DW07_FLD_BA_WIN_SIZE3, (reg32), (val))
#define PEER_CAP_DW07_SET_BA_WIN_SIZE2(reg32, val)             REG_FLD_SET(PEER_CAP_DW07_FLD_BA_WIN_SIZE2, (reg32), (val))
#define PEER_CAP_DW07_SET_BA_WIN_SIZE1(reg32, val)             REG_FLD_SET(PEER_CAP_DW07_FLD_BA_WIN_SIZE1, (reg32), (val))
#define PEER_CAP_DW07_SET_BA_WIN_SIZE0(reg32, val)             REG_FLD_SET(PEER_CAP_DW07_FLD_BA_WIN_SIZE0, (reg32), (val))

#define PEER_CAP_DW08_SET_CHK_PER(reg32, val)                  REG_FLD_SET(PEER_CAP_DW08_FLD_CHK_PER, (reg32), (val))
#define PEER_CAP_DW08_SET_PARTIAL_AID(reg32, val)              REG_FLD_SET(PEER_CAP_DW08_FLD_PARTIAL_AID, (reg32), (val))
#define PEER_CAP_DW08_SET_AC3_RTS_FAIL_CNT(reg32, val)         REG_FLD_SET(PEER_CAP_DW08_FLD_AC3_RTS_FAIL_CNT, (reg32), (val))
#define PEER_CAP_DW08_SET_AC2_RTS_FAIL_CNT(reg32, val)         REG_FLD_SET(PEER_CAP_DW08_FLD_AC2_RTS_FAIL_CNT, (reg32), (val))
#define PEER_CAP_DW08_SET_AC1_RTS_FAIL_CNT(reg32, val)         REG_FLD_SET(PEER_CAP_DW08_FLD_AC1_RTS_FAIL_CNT, (reg32), (val))
#define PEER_CAP_DW08_SET_AC0_RTS_FAIL_CNT(reg32, val)         REG_FLD_SET(PEER_CAP_DW08_FLD_AC0_RTS_FAIL_CNT, (reg32), (val))

#define PEER_CAP_DW09_SET_RATE_IDX(reg32, val)                 REG_FLD_SET(PEER_CAP_DW09_FLD_RATE_IDX, (reg32), (val))
#define PEER_CAP_DW09_SET_MPDU_OK_CNT(reg32, val)              REG_FLD_SET(PEER_CAP_DW09_FLD_MPDU_OK_CNT, (reg32), (val))
#define PEER_CAP_DW09_SET_MPDU_FAIL_CNT(reg32, val)            REG_FLD_SET(PEER_CAP_DW09_FLD_MPDU_FAIL_CNT, (reg32), (val))
#define PEER_CAP_DW09_SET_FCAP(reg32, val)                     REG_FLD_SET(PEER_CAP_DW09_FLD_FCAP, (reg32), (val))
#define PEER_CAP_DW09_SET_PRITX_ER106T(reg32, val)             REG_FLD_SET(PEER_CAP_DW09_FLD_PRITX_ER106T, (reg32), (val))
#define PEER_CAP_DW09_SET_PRITX_DCM(reg32, val)                REG_FLD_SET(PEER_CAP_DW09_FLD_PRITX_DCM, (reg32), (val))
#define PEER_CAP_DW09_SET_PRITX_PLR(reg32, val)                REG_FLD_SET(PEER_CAP_DW09_FLD_PRITX_PLR, (reg32), (val))
#define PEER_CAP_DW09_SET_PRITX_ERSU(reg32, val)               REG_FLD_SET(PEER_CAP_DW09_FLD_PRITX_ERSU, (reg32), (val))
#define PEER_CAP_DW09_SET_PRITX_SW_MODE(reg32, val)            REG_FLD_SET(PEER_CAP_DW09_FLD_PRITX_SW_MODE, (reg32), (val))
#define PEER_CAP_DW09_SET_RX_AVG_MPDU_SIZE(reg32, val)         REG_FLD_SET(PEER_CAP_DW09_FLD_RX_AVG_MPDU_SIZE, (reg32), (val))

#define AUTO_RATE_TABLE_DW10_SET_RATE2(reg32, val)             REG_FLD_SET(AUTO_RATE_TABLE_DW10_FLD_RATE2, (reg32), (val))
#define AUTO_RATE_TABLE_DW10_SET_RATE1(reg32, val)             REG_FLD_SET(AUTO_RATE_TABLE_DW10_FLD_RATE1, (reg32), (val))

#define AUTO_RATE_TABLE_DW11_SET_RATE4(reg32, val)             REG_FLD_SET(AUTO_RATE_TABLE_DW11_FLD_RATE4, (reg32), (val))
#define AUTO_RATE_TABLE_DW11_SET_RATE3(reg32, val)             REG_FLD_SET(AUTO_RATE_TABLE_DW11_FLD_RATE3, (reg32), (val))

#define AUTO_RATE_TABLE_DW12_SET_RATE6(reg32, val)             REG_FLD_SET(AUTO_RATE_TABLE_DW12_FLD_RATE6, (reg32), (val))
#define AUTO_RATE_TABLE_DW12_SET_RATE5(reg32, val)             REG_FLD_SET(AUTO_RATE_TABLE_DW12_FLD_RATE5, (reg32), (val))

#define AUTO_RATE_TABLE_DW13_SET_RATE8(reg32, val)             REG_FLD_SET(AUTO_RATE_TABLE_DW13_FLD_RATE8, (reg32), (val))
#define AUTO_RATE_TABLE_DW13_SET_RATE7(reg32, val)             REG_FLD_SET(AUTO_RATE_TABLE_DW13_FLD_RATE7, (reg32), (val))

#define AUTO_RATE_CTR_DW14_0_SET_RATE1_FAIL_CNT(reg32, val)    REG_FLD_SET(AUTO_RATE_CTR_DW14_0_FLD_RATE1_FAIL_CNT, (reg32), (val))
#define AUTO_RATE_CTR_DW14_0_SET_RATE1_TX_CNT(reg32, val)      REG_FLD_SET(AUTO_RATE_CTR_DW14_0_FLD_RATE1_TX_CNT, (reg32), (val))

#define AUTO_RATE_CTR_DW14_1_SET_CIPHER_SUIT_BIGTK(reg32, val) REG_FLD_SET(AUTO_RATE_CTR_DW14_1_FLD_CIPHER_SUIT_BIGTK, (reg32), (val))
#define AUTO_RATE_CTR_DW14_1_SET_CIPHER_SUIT_IGTK(reg32, val)  REG_FLD_SET(AUTO_RATE_CTR_DW14_1_FLD_CIPHER_SUIT_IGTK, (reg32), (val))

#define AUTO_RATE_CTR_DW15_0_SET_RATE3_OK_CNT(reg32, val)      REG_FLD_SET(AUTO_RATE_CTR_DW15_0_FLD_RATE3_OK_CNT, (reg32), (val))
#define AUTO_RATE_CTR_DW15_0_SET_RATE2_OK_CNT(reg32, val)      REG_FLD_SET(AUTO_RATE_CTR_DW15_0_FLD_RATE2_OK_CNT, (reg32), (val))

#define AUTO_RATE_CTR_DW16_0_SET_CURRENT_BW_FAIL_CNT(reg32, val) REG_FLD_SET(AUTO_RATE_CTR_DW16_0_FLD_CURRENT_BW_FAIL_CNT, (reg32), (val))
#define AUTO_RATE_CTR_DW16_0_SET_CURRENT_BW_TX_CNT(reg32, val) REG_FLD_SET(AUTO_RATE_CTR_DW16_0_FLD_CURRENT_BW_TX_CNT, (reg32), (val))

#define AUTO_RATE_CTR_DW17_0_SET_OTHER_BW_FAIL_CNT(reg32, val) REG_FLD_SET(AUTO_RATE_CTR_DW17_0_FLD_OTHER_BW_FAIL_CNT, (reg32), (val))
#define AUTO_RATE_CTR_DW17_0_SET_OTHER_BW_TX_CNT(reg32, val)   REG_FLD_SET(AUTO_RATE_CTR_DW17_0_FLD_OTHER_BW_TX_CNT, (reg32), (val))

#define PPDU_CTR_DW18_0_SET_RTS_FAIL_CNT(reg32, val)           REG_FLD_SET(PPDU_CTR_DW18_0_FLD_RTS_FAIL_CNT, (reg32), (val))
#define PPDU_CTR_DW18_0_SET_RTS_OK_CNT(reg32, val)             REG_FLD_SET(PPDU_CTR_DW18_0_FLD_RTS_OK_CNT, (reg32), (val))

#define PPDU_CTR_DW19_0_SET_MGNT_RETRY_CNT(reg32, val)         REG_FLD_SET(PPDU_CTR_DW19_0_FLD_MGNT_RETRY_CNT, (reg32), (val))
#define PPDU_CTR_DW19_0_SET_DATA_RETRY_CNT(reg32, val)         REG_FLD_SET(PPDU_CTR_DW19_0_FLD_DATA_RETRY_CNT, (reg32), (val))

#define ADM_CTR_DW20_0_SET_AC0_CTT_CDT_CRB__CTT(reg32, val)    REG_FLD_SET(ADM_CTR_DW20_0_FLD_AC0_CTT_CDT_CRB__CTT, (reg32), (val))

#define ADM_CTR_DW21_0_SET_AC0_CTB_CRT_CTB__CRT(reg32, val)    REG_FLD_SET(ADM_CTR_DW21_0_FLD_AC0_CTB_CRT_CTB__CRT, (reg32), (val))

#define ADM_CTR_DW22_0_SET_AC1_CTT_CDT_CRB__CTMC(reg32, val)   REG_FLD_SET(ADM_CTR_DW22_0_FLD_AC1_CTT_CDT_CRB__CTMC, (reg32), (val))

#define ADM_CTR_DW23_0_SET_AC1_CTB_CRT_CTB__CRMC(reg32, val)   REG_FLD_SET(ADM_CTR_DW23_0_FLD_AC1_CTB_CRT_CTB__CRMC, (reg32), (val))

#define ADM_CTR_DW24_0_SET_AC2_CTT_CDT_CRB__CTDB(reg32, val)   REG_FLD_SET(ADM_CTR_DW24_0_FLD_AC2_CTT_CDT_CRB__CTDB, (reg32), (val))

#define ADM_CTR_DW25_0_SET_AC2_CTB_CRT_CTB__CRDB(reg32, val)   REG_FLD_SET(ADM_CTR_DW25_0_FLD_AC2_CTB_CRT_CTB__CRDB, (reg32), (val))

#define ADM_CTR_DW26_0_SET_AC3_CTT_CDT_CRB__CTODB(reg32, val)  REG_FLD_SET(ADM_CTR_DW26_0_FLD_AC3_CTT_CDT_CRB__CTODB, (reg32), (val))

#define ADM_CTR_DW27_0_SET_AC3_CTB_CRT_CTB__(reg32, val)       REG_FLD_SET(ADM_CTR_DW27_0_FLD_AC3_CTB_CRT_CTB__, (reg32), (val))

#define MLO_INFO_DW28_SET_SECONDARY_MLD_BAND(reg32, val)       REG_FLD_SET(MLO_INFO_DW28_FLD_SECONDARY_MLD_BAND, (reg32), (val))
#define MLO_INFO_DW28_SET_RELATED_BAND1(reg32, val)            REG_FLD_SET(MLO_INFO_DW28_FLD_RELATED_BAND1, (reg32), (val))
#define MLO_INFO_DW28_SET_RELATED_IDX1(reg32, val)             REG_FLD_SET(MLO_INFO_DW28_FLD_RELATED_IDX1, (reg32), (val))
#define MLO_INFO_DW28_SET_PRIMARY_MLD_BAND(reg32, val)         REG_FLD_SET(MLO_INFO_DW28_FLD_PRIMARY_MLD_BAND, (reg32), (val))
#define MLO_INFO_DW28_SET_RELATED_BAND0(reg32, val)            REG_FLD_SET(MLO_INFO_DW28_FLD_RELATED_BAND0, (reg32), (val))
#define MLO_INFO_DW28_SET_RELATED_IDX0(reg32, val)             REG_FLD_SET(MLO_INFO_DW28_FLD_RELATED_IDX0, (reg32), (val))

#define MLO_INFO_DW29_SET_STR_BITMAP(reg32, val)               REG_FLD_SET(MLO_INFO_DW29_FLD_STR_BITMAP, (reg32), (val))
#define MLO_INFO_DW29_SET_EMLMR2(reg32, val)                   REG_FLD_SET(MLO_INFO_DW29_FLD_EMLMR2, (reg32), (val))
#define MLO_INFO_DW29_SET_EMLSR2(reg32, val)                   REG_FLD_SET(MLO_INFO_DW29_FLD_EMLSR2, (reg32), (val))
#define MLO_INFO_DW29_SET_EMLMR1(reg32, val)                   REG_FLD_SET(MLO_INFO_DW29_FLD_EMLMR1, (reg32), (val))
#define MLO_INFO_DW29_SET_EMLSR1(reg32, val)                   REG_FLD_SET(MLO_INFO_DW29_FLD_EMLSR1, (reg32), (val))
#define MLO_INFO_DW29_SET_EMLMR0(reg32, val)                   REG_FLD_SET(MLO_INFO_DW29_FLD_EMLMR0, (reg32), (val))
#define MLO_INFO_DW29_SET_EMLSR0(reg32, val)                   REG_FLD_SET(MLO_INFO_DW29_FLD_EMLSR0, (reg32), (val))
#define MLO_INFO_DW29_SET_OWN_MLD_ID(reg32, val)               REG_FLD_SET(MLO_INFO_DW29_FLD_OWN_MLD_ID, (reg32), (val))
#define MLO_INFO_DW29_SET_DISPATCH_POLICY7(reg32, val)         REG_FLD_SET(MLO_INFO_DW29_FLD_DISPATCH_POLICY7, (reg32), (val))
#define MLO_INFO_DW29_SET_DISPATCH_POLICY6(reg32, val)         REG_FLD_SET(MLO_INFO_DW29_FLD_DISPATCH_POLICY6, (reg32), (val))
#define MLO_INFO_DW29_SET_DISPATCH_POLICY5(reg32, val)         REG_FLD_SET(MLO_INFO_DW29_FLD_DISPATCH_POLICY5, (reg32), (val))
#define MLO_INFO_DW29_SET_DISPATCH_POLICY4(reg32, val)         REG_FLD_SET(MLO_INFO_DW29_FLD_DISPATCH_POLICY4, (reg32), (val))
#define MLO_INFO_DW29_SET_DISPATCH_POLICY3(reg32, val)         REG_FLD_SET(MLO_INFO_DW29_FLD_DISPATCH_POLICY3, (reg32), (val))
#define MLO_INFO_DW29_SET_DISPATCH_POLICY2(reg32, val)         REG_FLD_SET(MLO_INFO_DW29_FLD_DISPATCH_POLICY2, (reg32), (val))
#define MLO_INFO_DW29_SET_DISPATCH_POLICY1(reg32, val)         REG_FLD_SET(MLO_INFO_DW29_FLD_DISPATCH_POLICY1, (reg32), (val))
#define MLO_INFO_DW29_SET_DISPATCH_POLICY0(reg32, val)         REG_FLD_SET(MLO_INFO_DW29_FLD_DISPATCH_POLICY0, (reg32), (val))

#define MLO_INFO_DW30_SET_LINK_MGF(reg32, val)                 REG_FLD_SET(MLO_INFO_DW30_FLD_LINK_MGF, (reg32), (val))
#define MLO_INFO_DW30_SET_EMLSR_TRANS_DLY_IDX(reg32, val)      REG_FLD_SET(MLO_INFO_DW30_FLD_EMLSR_TRANS_DLY_IDX, (reg32), (val))
#define MLO_INFO_DW30_SET_DISPATCH_RATIO(reg32, val)           REG_FLD_SET(MLO_INFO_DW30_FLD_DISPATCH_RATIO, (reg32), (val))
#define MLO_INFO_DW30_SET_DISPATCH_ORDER(reg32, val)           REG_FLD_SET(MLO_INFO_DW30_FLD_DISPATCH_ORDER, (reg32), (val))

#define RESP_INFO_DW31_SET_ACK_EN(reg32, val)                  REG_FLD_SET(RESP_INFO_DW31_FLD_ACK_EN, (reg32), (val))
#define RESP_INFO_DW31_SET_RXD_DUP_MODE(reg32, val)            REG_FLD_SET(RESP_INFO_DW31_FLD_RXD_DUP_MODE, (reg32), (val))
#define RESP_INFO_DW31_SET_MPDU_SIZE(reg32, val)               REG_FLD_SET(RESP_INFO_DW31_FLD_MPDU_SIZE, (reg32), (val))
#define RESP_INFO_DW31_SET_ALL_ACK(reg32, val)                 REG_FLD_SET(RESP_INFO_DW31_FLD_ALL_ACK, (reg32), (val))
#define RESP_INFO_DW31_SET_CASCAD(reg32, val)                  REG_FLD_SET(RESP_INFO_DW31_FLD_CASCAD, (reg32), (val))
#define RESP_INFO_DW31_SET_DROP(reg32, val)                    REG_FLD_SET(RESP_INFO_DW31_FLD_DROP, (reg32), (val))
#define RESP_INFO_DW31_SET_BFTX_TB(reg32, val)                 REG_FLD_SET(RESP_INFO_DW31_FLD_BFTX_TB, (reg32), (val))

#define RX_DUP_INFO_DW32_SET_RXD_DUP_WHITE_LIST(reg32, val)    REG_FLD_SET(RX_DUP_INFO_DW32_FLD_RXD_DUP_WHITE_LIST, (reg32), (val))
#define RX_DUP_INFO_DW32_SET_RXD_DUP_FOR_OM_CHG(reg32, val)    REG_FLD_SET(RX_DUP_INFO_DW32_FLD_RXD_DUP_FOR_OM_CHG, (reg32), (val))
#define RX_DUP_INFO_DW32_SET_OM_INFO_EHT(reg32, val)           REG_FLD_SET(RX_DUP_INFO_DW32_FLD_OM_INFO_EHT, (reg32), (val))
#define RX_DUP_INFO_DW32_SET_OM_INFO(reg32, val)               REG_FLD_SET(RX_DUP_INFO_DW32_FLD_OM_INFO, (reg32), (val))

#define RX_STAT_DW33_SET_AMSDU_CROSS_LG(reg32, val)            REG_FLD_SET(RX_STAT_DW33_FLD_AMSDU_CROSS_LG, (reg32), (val))
#define RX_STAT_DW33_SET_HT_AMSDU(reg32, val)                  REG_FLD_SET(RX_STAT_DW33_FLD_HT_AMSDU, (reg32), (val))
#define RX_STAT_DW33_SET_RAPID_REACTION_RATE(reg32, val)       REG_FLD_SET(RX_STAT_DW33_FLD_RAPID_REACTION_RATE, (reg32), (val))
#define RX_STAT_DW33_SET_USER_SNR(reg32, val)                  REG_FLD_SET(RX_STAT_DW33_FLD_USER_SNR, (reg32), (val))
#define RX_STAT_DW33_SET_USER_RSSI(reg32, val)                 REG_FLD_SET(RX_STAT_DW33_FLD_USER_RSSI, (reg32), (val))

#define RX_STAT_DW34_SET_RESP_RCPI3(reg32, val)                REG_FLD_SET(RX_STAT_DW34_FLD_RESP_RCPI3, (reg32), (val))
#define RX_STAT_DW34_SET_RESP_RCPI2(reg32, val)                REG_FLD_SET(RX_STAT_DW34_FLD_RESP_RCPI2, (reg32), (val))
#define RX_STAT_DW34_SET_RESP_RCPI1(reg32, val)                REG_FLD_SET(RX_STAT_DW34_FLD_RESP_RCPI1, (reg32), (val))
#define RX_STAT_DW34_SET_RESP_RCPI0(reg32, val)                REG_FLD_SET(RX_STAT_DW34_FLD_RESP_RCPI0, (reg32), (val))

#define RX_STAT_DW35_SET_SNR_RX3(reg32, val)                   REG_FLD_SET(RX_STAT_DW35_FLD_SNR_RX3, (reg32), (val))
#define RX_STAT_DW35_SET_SNR_RX2(reg32, val)                   REG_FLD_SET(RX_STAT_DW35_FLD_SNR_RX2, (reg32), (val))
#define RX_STAT_DW35_SET_SNR_RX1(reg32, val)                   REG_FLD_SET(RX_STAT_DW35_FLD_SNR_RX1, (reg32), (val))
#define RX_STAT_DW35_SET_SNR_RX0(reg32, val)                   REG_FLD_SET(RX_STAT_DW35_FLD_SNR_RX0, (reg32), (val))

#define VERSION_CODE_SET_VERSION_CODE(reg32, val)              REG_FLD_SET(VERSION_CODE_FLD_VERSION_CODE, (reg32), (val))

#define PEER_INFO_DW00_VAL_WPI_FLAG(val)                       REG_FLD_VAL(PEER_INFO_DW00_FLD_WPI_FLAG, (val))
#define PEER_INFO_DW00_VAL_RCA2(val)                           REG_FLD_VAL(PEER_INFO_DW00_FLD_RCA2, (val))
#define PEER_INFO_DW00_VAL_RV(val)                             REG_FLD_VAL(PEER_INFO_DW00_FLD_RV, (val))
#define PEER_INFO_DW00_VAL_BAND(val)                           REG_FLD_VAL(PEER_INFO_DW00_FLD_BAND, (val))
#define PEER_INFO_DW00_VAL_RCID(val)                           REG_FLD_VAL(PEER_INFO_DW00_FLD_RCID, (val))
#define PEER_INFO_DW00_VAL_KID(val)                            REG_FLD_VAL(PEER_INFO_DW00_FLD_KID, (val))
#define PEER_INFO_DW00_VAL_RCA1(val)                           REG_FLD_VAL(PEER_INFO_DW00_FLD_RCA1, (val))
#define PEER_INFO_DW00_VAL_MUAR(val)                           REG_FLD_VAL(PEER_INFO_DW00_FLD_MUAR, (val))
#define PEER_INFO_DW00_VAL_PEER_LINK_ADDRESS_47_32_(val)       REG_FLD_VAL(PEER_INFO_DW00_FLD_PEER_LINK_ADDRESS_47_32_, (val))

#define PEER_INFO_DW01_VAL_PEER_LINK_ADDRESS_31_0_(val)        REG_FLD_VAL(PEER_INFO_DW01_FLD_PEER_LINK_ADDRESS_31_0_, (val))

#define PEER_CAP_DW02_VAL_MESH(val)                            REG_FLD_VAL(PEER_CAP_DW02_FLD_MESH, (val))
#define PEER_CAP_DW02_VAL_EHT(val)                             REG_FLD_VAL(PEER_CAP_DW02_FLD_EHT, (val))
#define PEER_CAP_DW02_VAL_HE(val)                              REG_FLD_VAL(PEER_CAP_DW02_FLD_HE, (val))
#define PEER_CAP_DW02_VAL_VHT(val)                             REG_FLD_VAL(PEER_CAP_DW02_FLD_VHT, (val))
#define PEER_CAP_DW02_VAL_HT(val)                              REG_FLD_VAL(PEER_CAP_DW02_FLD_HT, (val))
#define PEER_CAP_DW02_VAL_QOS(val)                             REG_FLD_VAL(PEER_CAP_DW02_FLD_QOS, (val))
#define PEER_CAP_DW02_VAL_TX_PS(val)                           REG_FLD_VAL(PEER_CAP_DW02_FLD_TX_PS, (val))
#define PEER_CAP_DW02_VAL_UL(val)                              REG_FLD_VAL(PEER_CAP_DW02_FLD_UL, (val))
#define PEER_CAP_DW02_VAL_SW(val)                              REG_FLD_VAL(PEER_CAP_DW02_FLD_SW, (val))
#define PEER_CAP_DW02_VAL_TD(val)                              REG_FLD_VAL(PEER_CAP_DW02_FLD_TD, (val))
#define PEER_CAP_DW02_VAL_FD(val)                              REG_FLD_VAL(PEER_CAP_DW02_FLD_FD, (val))
#define PEER_CAP_DW02_VAL_CIPHER_SUIT_PGTK(val)                REG_FLD_VAL(PEER_CAP_DW02_FLD_CIPHER_SUIT_PGTK, (val))
#define PEER_CAP_DW02_VAL_DUAL_CTS_CAP(val)                    REG_FLD_VAL(PEER_CAP_DW02_FLD_DUAL_CTS_CAP, (val))
#define PEER_CAP_DW02_VAL_DUAL_PTEC_EN(val)                    REG_FLD_VAL(PEER_CAP_DW02_FLD_DUAL_PTEC_EN, (val))
#define PEER_CAP_DW02_VAL_GID_SU(val)                          REG_FLD_VAL(PEER_CAP_DW02_FLD_GID_SU, (val))
#define PEER_CAP_DW02_VAL_AID(val)                             REG_FLD_VAL(PEER_CAP_DW02_FLD_AID, (val))

#define PEER_CAP_DW03_VAL_IGN_FBK(val)                         REG_FLD_VAL(PEER_CAP_DW03_FLD_IGN_FBK, (val))
#define PEER_CAP_DW03_VAL_TBF_EHT(val)                         REG_FLD_VAL(PEER_CAP_DW03_FLD_TBF_EHT, (val))
#define PEER_CAP_DW03_VAL_TBF_HE(val)                          REG_FLD_VAL(PEER_CAP_DW03_FLD_TBF_HE, (val))
#define PEER_CAP_DW03_VAL_TBF_VHT(val)                         REG_FLD_VAL(PEER_CAP_DW03_FLD_TBF_VHT, (val))
#define PEER_CAP_DW03_VAL_TBF_HT(val)                          REG_FLD_VAL(PEER_CAP_DW03_FLD_TBF_HT, (val))
#define PEER_CAP_DW03_VAL_BYPASS_TXSMM(val)                    REG_FLD_VAL(PEER_CAP_DW03_FLD_BYPASS_TXSMM, (val))
#define PEER_CAP_DW03_VAL_ULPF(val)                            REG_FLD_VAL(PEER_CAP_DW03_FLD_ULPF, (val))
#define PEER_CAP_DW03_VAL_RIBF(val)                            REG_FLD_VAL(PEER_CAP_DW03_FLD_RIBF, (val))
#define PEER_CAP_DW03_VAL_ULPF_IDX(val)                        REG_FLD_VAL(PEER_CAP_DW03_FLD_ULPF_IDX, (val))
#define PEER_CAP_DW03_VAL_PFMU_IDX(val)                        REG_FLD_VAL(PEER_CAP_DW03_FLD_PFMU_IDX, (val))
#define PEER_CAP_DW03_VAL_EHT_LTF_SYM_NUM_OPT(val)             REG_FLD_VAL(PEER_CAP_DW03_FLD_EHT_LTF_SYM_NUM_OPT, (val))
#define PEER_CAP_DW03_VAL_BEAM_CHG(val)                        REG_FLD_VAL(PEER_CAP_DW03_FLD_BEAM_CHG, (val))
#define PEER_CAP_DW03_VAL_HDRT_MODE(val)                       REG_FLD_VAL(PEER_CAP_DW03_FLD_HDRT_MODE, (val))
#define PEER_CAP_DW03_VAL_EHT_SIG_MCS(val)                     REG_FLD_VAL(PEER_CAP_DW03_FLD_EHT_SIG_MCS, (val))
#define PEER_CAP_DW03_VAL_WMM_Q(val)                           REG_FLD_VAL(PEER_CAP_DW03_FLD_WMM_Q, (val))

#define PEER_CAP_DW04_VAL_BA_MODE(val)                         REG_FLD_VAL(PEER_CAP_DW04_FLD_BA_MODE, (val))
#define PEER_CAP_DW04_VAL_LDPC_EHT(val)                        REG_FLD_VAL(PEER_CAP_DW04_FLD_LDPC_EHT, (val))
#define PEER_CAP_DW04_VAL_LDPC_HE(val)                         REG_FLD_VAL(PEER_CAP_DW04_FLD_LDPC_HE, (val))
#define PEER_CAP_DW04_VAL_LDPC_VHT(val)                        REG_FLD_VAL(PEER_CAP_DW04_FLD_LDPC_VHT, (val))
#define PEER_CAP_DW04_VAL_LDPC_HT(val)                         REG_FLD_VAL(PEER_CAP_DW04_FLD_LDPC_HT, (val))
#define PEER_CAP_DW04_VAL_DIS_RHTR(val)                        REG_FLD_VAL(PEER_CAP_DW04_FLD_DIS_RHTR, (val))
#define PEER_CAP_DW04_VAL_PE(val)                              REG_FLD_VAL(PEER_CAP_DW04_FLD_PE, (val))
#define PEER_CAP_DW04_VAL_NEGOTIATED_WINSIZE7(val)             REG_FLD_VAL(PEER_CAP_DW04_FLD_NEGOTIATED_WINSIZE7, (val))
#define PEER_CAP_DW04_VAL_NEGOTIATED_WINSIZE6(val)             REG_FLD_VAL(PEER_CAP_DW04_FLD_NEGOTIATED_WINSIZE6, (val))
#define PEER_CAP_DW04_VAL_NEGOTIATED_WINSIZE5(val)             REG_FLD_VAL(PEER_CAP_DW04_FLD_NEGOTIATED_WINSIZE5, (val))
#define PEER_CAP_DW04_VAL_NEGOTIATED_WINSIZE4(val)             REG_FLD_VAL(PEER_CAP_DW04_FLD_NEGOTIATED_WINSIZE4, (val))
#define PEER_CAP_DW04_VAL_NEGOTIATED_WINSIZE3(val)             REG_FLD_VAL(PEER_CAP_DW04_FLD_NEGOTIATED_WINSIZE3, (val))
#define PEER_CAP_DW04_VAL_NEGOTIATED_WINSIZE2(val)             REG_FLD_VAL(PEER_CAP_DW04_FLD_NEGOTIATED_WINSIZE2, (val))
#define PEER_CAP_DW04_VAL_NEGOTIATED_WINSIZE1(val)             REG_FLD_VAL(PEER_CAP_DW04_FLD_NEGOTIATED_WINSIZE1, (val))
#define PEER_CAP_DW04_VAL_NEGOTIATED_WINSIZE0(val)             REG_FLD_VAL(PEER_CAP_DW04_FLD_NEGOTIATED_WINSIZE0, (val))

#define PEER_CAP_DW05_VAL_SKIP_TX(val)                         REG_FLD_VAL(PEER_CAP_DW05_FLD_SKIP_TX, (val))
#define PEER_CAP_DW05_VAL_PSM(val)                             REG_FLD_VAL(PEER_CAP_DW05_FLD_PSM, (val))
#define PEER_CAP_DW05_VAL_I_PSM(val)                           REG_FLD_VAL(PEER_CAP_DW05_FLD_I_PSM, (val))
#define PEER_CAP_DW05_VAL_DU_I_PSM(val)                        REG_FLD_VAL(PEER_CAP_DW05_FLD_DU_I_PSM, (val))
#define PEER_CAP_DW05_VAL_TXOP_PS_CAP(val)                     REG_FLD_VAL(PEER_CAP_DW05_FLD_TXOP_PS_CAP, (val))
#define PEER_CAP_DW05_VAL_DOPPL(val)                           REG_FLD_VAL(PEER_CAP_DW05_FLD_DOPPL, (val))
#define PEER_CAP_DW05_VAL_GI_EHT(val)                          REG_FLD_VAL(PEER_CAP_DW05_FLD_GI_EHT, (val))
#define PEER_CAP_DW05_VAL_LTF_EHT(val)                         REG_FLD_VAL(PEER_CAP_DW05_FLD_LTF_EHT, (val))
#define PEER_CAP_DW05_VAL_TX_POWER_OFFSET(val)                 REG_FLD_VAL(PEER_CAP_DW05_FLD_TX_POWER_OFFSET, (val))
#define PEER_CAP_DW05_VAL_SR_ABORT(val)                        REG_FLD_VAL(PEER_CAP_DW05_FLD_SR_ABORT, (val))
#define PEER_CAP_DW05_VAL_SR_R(val)                            REG_FLD_VAL(PEER_CAP_DW05_FLD_SR_R, (val))
#define PEER_CAP_DW05_VAL_USR(val)                             REG_FLD_VAL(PEER_CAP_DW05_FLD_USR, (val))
#define PEER_CAP_DW05_VAL_MMSS(val)                            REG_FLD_VAL(PEER_CAP_DW05_FLD_MMSS, (val))
#define PEER_CAP_DW05_VAL_DYN_BW(val)                          REG_FLD_VAL(PEER_CAP_DW05_FLD_DYN_BW, (val))
#define PEER_CAP_DW05_VAL_SMPS(val)                            REG_FLD_VAL(PEER_CAP_DW05_FLD_SMPS, (val))
#define PEER_CAP_DW05_VAL_RTS(val)                             REG_FLD_VAL(PEER_CAP_DW05_FLD_RTS, (val))
#define PEER_CAP_DW05_VAL_BSA_EN(val)                          REG_FLD_VAL(PEER_CAP_DW05_FLD_BSA_EN, (val))
#define PEER_CAP_DW05_VAL_AF(val)                              REG_FLD_VAL(PEER_CAP_DW05_FLD_AF, (val))

#define PEER_CAP_DW06_VAL_G16_HE(val)                          REG_FLD_VAL(PEER_CAP_DW06_FLD_G16_HE, (val))
#define PEER_CAP_DW06_VAL_G8_HE(val)                           REG_FLD_VAL(PEER_CAP_DW06_FLD_G8_HE, (val))
#define PEER_CAP_DW06_VAL_G4_HE(val)                           REG_FLD_VAL(PEER_CAP_DW06_FLD_G4_HE, (val))
#define PEER_CAP_DW06_VAL_G2_HE(val)                           REG_FLD_VAL(PEER_CAP_DW06_FLD_G2_HE, (val))
#define PEER_CAP_DW06_VAL_G16_LTF(val)                         REG_FLD_VAL(PEER_CAP_DW06_FLD_G16_LTF, (val))
#define PEER_CAP_DW06_VAL_G8_LTF(val)                          REG_FLD_VAL(PEER_CAP_DW06_FLD_G8_LTF, (val))
#define PEER_CAP_DW06_VAL_G4_LTF(val)                          REG_FLD_VAL(PEER_CAP_DW06_FLD_G4_LTF, (val))
#define PEER_CAP_DW06_VAL_G2_LTF(val)                          REG_FLD_VAL(PEER_CAP_DW06_FLD_G2_LTF, (val))
#define PEER_CAP_DW06_VAL_G16(val)                             REG_FLD_VAL(PEER_CAP_DW06_FLD_G16, (val))
#define PEER_CAP_DW06_VAL_G8(val)                              REG_FLD_VAL(PEER_CAP_DW06_FLD_G8, (val))
#define PEER_CAP_DW06_VAL_G4(val)                              REG_FLD_VAL(PEER_CAP_DW06_FLD_G4, (val))
#define PEER_CAP_DW06_VAL_G2(val)                              REG_FLD_VAL(PEER_CAP_DW06_FLD_G2, (val))
#define PEER_CAP_DW06_VAL_SPE_IDX(val)                         REG_FLD_VAL(PEER_CAP_DW06_FLD_SPE_IDX, (val))
#define PEER_CAP_DW06_VAL_R(val)                               REG_FLD_VAL(PEER_CAP_DW06_FLD_R, (val))
#define PEER_CAP_DW06_VAL_RDGBA(val)                           REG_FLD_VAL(PEER_CAP_DW06_FLD_RDGBA, (val))
#define PEER_CAP_DW06_VAL_BAF_EN(val)                          REG_FLD_VAL(PEER_CAP_DW06_FLD_BAF_EN, (val))
#define PEER_CAP_DW06_VAL_DBNSS_EN(val)                        REG_FLD_VAL(PEER_CAP_DW06_FLD_DBNSS_EN, (val))
#define PEER_CAP_DW06_VAL_CBRN(val)                            REG_FLD_VAL(PEER_CAP_DW06_FLD_CBRN, (val))

#define PEER_CAP_DW07_VAL_BA_WIN_SIZE7(val)                    REG_FLD_VAL(PEER_CAP_DW07_FLD_BA_WIN_SIZE7, (val))
#define PEER_CAP_DW07_VAL_BA_WIN_SIZE6(val)                    REG_FLD_VAL(PEER_CAP_DW07_FLD_BA_WIN_SIZE6, (val))
#define PEER_CAP_DW07_VAL_BA_WIN_SIZE5(val)                    REG_FLD_VAL(PEER_CAP_DW07_FLD_BA_WIN_SIZE5, (val))
#define PEER_CAP_DW07_VAL_BA_WIN_SIZE4(val)                    REG_FLD_VAL(PEER_CAP_DW07_FLD_BA_WIN_SIZE4, (val))
#define PEER_CAP_DW07_VAL_BA_WIN_SIZE3(val)                    REG_FLD_VAL(PEER_CAP_DW07_FLD_BA_WIN_SIZE3, (val))
#define PEER_CAP_DW07_VAL_BA_WIN_SIZE2(val)                    REG_FLD_VAL(PEER_CAP_DW07_FLD_BA_WIN_SIZE2, (val))
#define PEER_CAP_DW07_VAL_BA_WIN_SIZE1(val)                    REG_FLD_VAL(PEER_CAP_DW07_FLD_BA_WIN_SIZE1, (val))
#define PEER_CAP_DW07_VAL_BA_WIN_SIZE0(val)                    REG_FLD_VAL(PEER_CAP_DW07_FLD_BA_WIN_SIZE0, (val))

#define PEER_CAP_DW08_VAL_CHK_PER(val)                         REG_FLD_VAL(PEER_CAP_DW08_FLD_CHK_PER, (val))
#define PEER_CAP_DW08_VAL_PARTIAL_AID(val)                     REG_FLD_VAL(PEER_CAP_DW08_FLD_PARTIAL_AID, (val))
#define PEER_CAP_DW08_VAL_AC3_RTS_FAIL_CNT(val)                REG_FLD_VAL(PEER_CAP_DW08_FLD_AC3_RTS_FAIL_CNT, (val))
#define PEER_CAP_DW08_VAL_AC2_RTS_FAIL_CNT(val)                REG_FLD_VAL(PEER_CAP_DW08_FLD_AC2_RTS_FAIL_CNT, (val))
#define PEER_CAP_DW08_VAL_AC1_RTS_FAIL_CNT(val)                REG_FLD_VAL(PEER_CAP_DW08_FLD_AC1_RTS_FAIL_CNT, (val))
#define PEER_CAP_DW08_VAL_AC0_RTS_FAIL_CNT(val)                REG_FLD_VAL(PEER_CAP_DW08_FLD_AC0_RTS_FAIL_CNT, (val))

#define PEER_CAP_DW09_VAL_RATE_IDX(val)                        REG_FLD_VAL(PEER_CAP_DW09_FLD_RATE_IDX, (val))
#define PEER_CAP_DW09_VAL_MPDU_OK_CNT(val)                     REG_FLD_VAL(PEER_CAP_DW09_FLD_MPDU_OK_CNT, (val))
#define PEER_CAP_DW09_VAL_MPDU_FAIL_CNT(val)                   REG_FLD_VAL(PEER_CAP_DW09_FLD_MPDU_FAIL_CNT, (val))
#define PEER_CAP_DW09_VAL_FCAP(val)                            REG_FLD_VAL(PEER_CAP_DW09_FLD_FCAP, (val))
#define PEER_CAP_DW09_VAL_PRITX_ER106T(val)                    REG_FLD_VAL(PEER_CAP_DW09_FLD_PRITX_ER106T, (val))
#define PEER_CAP_DW09_VAL_PRITX_DCM(val)                       REG_FLD_VAL(PEER_CAP_DW09_FLD_PRITX_DCM, (val))
#define PEER_CAP_DW09_VAL_PRITX_PLR(val)                       REG_FLD_VAL(PEER_CAP_DW09_FLD_PRITX_PLR, (val))
#define PEER_CAP_DW09_VAL_PRITX_ERSU(val)                      REG_FLD_VAL(PEER_CAP_DW09_FLD_PRITX_ERSU, (val))
#define PEER_CAP_DW09_VAL_PRITX_SW_MODE(val)                   REG_FLD_VAL(PEER_CAP_DW09_FLD_PRITX_SW_MODE, (val))
#define PEER_CAP_DW09_VAL_RX_AVG_MPDU_SIZE(val)                REG_FLD_VAL(PEER_CAP_DW09_FLD_RX_AVG_MPDU_SIZE, (val))

#define AUTO_RATE_TABLE_DW10_VAL_RATE2(val)                    REG_FLD_VAL(AUTO_RATE_TABLE_DW10_FLD_RATE2, (val))
#define AUTO_RATE_TABLE_DW10_VAL_RATE1(val)                    REG_FLD_VAL(AUTO_RATE_TABLE_DW10_FLD_RATE1, (val))

#define AUTO_RATE_TABLE_DW11_VAL_RATE4(val)                    REG_FLD_VAL(AUTO_RATE_TABLE_DW11_FLD_RATE4, (val))
#define AUTO_RATE_TABLE_DW11_VAL_RATE3(val)                    REG_FLD_VAL(AUTO_RATE_TABLE_DW11_FLD_RATE3, (val))

#define AUTO_RATE_TABLE_DW12_VAL_RATE6(val)                    REG_FLD_VAL(AUTO_RATE_TABLE_DW12_FLD_RATE6, (val))
#define AUTO_RATE_TABLE_DW12_VAL_RATE5(val)                    REG_FLD_VAL(AUTO_RATE_TABLE_DW12_FLD_RATE5, (val))

#define AUTO_RATE_TABLE_DW13_VAL_RATE8(val)                    REG_FLD_VAL(AUTO_RATE_TABLE_DW13_FLD_RATE8, (val))
#define AUTO_RATE_TABLE_DW13_VAL_RATE7(val)                    REG_FLD_VAL(AUTO_RATE_TABLE_DW13_FLD_RATE7, (val))

#define AUTO_RATE_CTR_DW14_0_VAL_RATE1_FAIL_CNT(val)           REG_FLD_VAL(AUTO_RATE_CTR_DW14_0_FLD_RATE1_FAIL_CNT, (val))
#define AUTO_RATE_CTR_DW14_0_VAL_RATE1_TX_CNT(val)             REG_FLD_VAL(AUTO_RATE_CTR_DW14_0_FLD_RATE1_TX_CNT, (val))

#define AUTO_RATE_CTR_DW14_1_VAL_CIPHER_SUIT_BIGTK(val)        REG_FLD_VAL(AUTO_RATE_CTR_DW14_1_FLD_CIPHER_SUIT_BIGTK, (val))
#define AUTO_RATE_CTR_DW14_1_VAL_CIPHER_SUIT_IGTK(val)         REG_FLD_VAL(AUTO_RATE_CTR_DW14_1_FLD_CIPHER_SUIT_IGTK, (val))

#define AUTO_RATE_CTR_DW15_0_VAL_RATE3_OK_CNT(val)             REG_FLD_VAL(AUTO_RATE_CTR_DW15_0_FLD_RATE3_OK_CNT, (val))
#define AUTO_RATE_CTR_DW15_0_VAL_RATE2_OK_CNT(val)             REG_FLD_VAL(AUTO_RATE_CTR_DW15_0_FLD_RATE2_OK_CNT, (val))

#define AUTO_RATE_CTR_DW16_0_VAL_CURRENT_BW_FAIL_CNT(val)      REG_FLD_VAL(AUTO_RATE_CTR_DW16_0_FLD_CURRENT_BW_FAIL_CNT, (val))
#define AUTO_RATE_CTR_DW16_0_VAL_CURRENT_BW_TX_CNT(val)        REG_FLD_VAL(AUTO_RATE_CTR_DW16_0_FLD_CURRENT_BW_TX_CNT, (val))

#define AUTO_RATE_CTR_DW17_0_VAL_OTHER_BW_FAIL_CNT(val)        REG_FLD_VAL(AUTO_RATE_CTR_DW17_0_FLD_OTHER_BW_FAIL_CNT, (val))
#define AUTO_RATE_CTR_DW17_0_VAL_OTHER_BW_TX_CNT(val)          REG_FLD_VAL(AUTO_RATE_CTR_DW17_0_FLD_OTHER_BW_TX_CNT, (val))

#define PPDU_CTR_DW18_0_VAL_RTS_FAIL_CNT(val)                  REG_FLD_VAL(PPDU_CTR_DW18_0_FLD_RTS_FAIL_CNT, (val))
#define PPDU_CTR_DW18_0_VAL_RTS_OK_CNT(val)                    REG_FLD_VAL(PPDU_CTR_DW18_0_FLD_RTS_OK_CNT, (val))

#define PPDU_CTR_DW19_0_VAL_MGNT_RETRY_CNT(val)                REG_FLD_VAL(PPDU_CTR_DW19_0_FLD_MGNT_RETRY_CNT, (val))
#define PPDU_CTR_DW19_0_VAL_DATA_RETRY_CNT(val)                REG_FLD_VAL(PPDU_CTR_DW19_0_FLD_DATA_RETRY_CNT, (val))

#define ADM_CTR_DW20_0_VAL_AC0_CTT_CDT_CRB__CTT(val)           REG_FLD_VAL(ADM_CTR_DW20_0_FLD_AC0_CTT_CDT_CRB__CTT, (val))

#define ADM_CTR_DW21_0_VAL_AC0_CTB_CRT_CTB__CRT(val)           REG_FLD_VAL(ADM_CTR_DW21_0_FLD_AC0_CTB_CRT_CTB__CRT, (val))

#define ADM_CTR_DW22_0_VAL_AC1_CTT_CDT_CRB__CTMC(val)          REG_FLD_VAL(ADM_CTR_DW22_0_FLD_AC1_CTT_CDT_CRB__CTMC, (val))

#define ADM_CTR_DW23_0_VAL_AC1_CTB_CRT_CTB__CRMC(val)          REG_FLD_VAL(ADM_CTR_DW23_0_FLD_AC1_CTB_CRT_CTB__CRMC, (val))

#define ADM_CTR_DW24_0_VAL_AC2_CTT_CDT_CRB__CTDB(val)          REG_FLD_VAL(ADM_CTR_DW24_0_FLD_AC2_CTT_CDT_CRB__CTDB, (val))

#define ADM_CTR_DW25_0_VAL_AC2_CTB_CRT_CTB__CRDB(val)          REG_FLD_VAL(ADM_CTR_DW25_0_FLD_AC2_CTB_CRT_CTB__CRDB, (val))

#define ADM_CTR_DW26_0_VAL_AC3_CTT_CDT_CRB__CTODB(val)         REG_FLD_VAL(ADM_CTR_DW26_0_FLD_AC3_CTT_CDT_CRB__CTODB, (val))

#define ADM_CTR_DW27_0_VAL_AC3_CTB_CRT_CTB__(val)              REG_FLD_VAL(ADM_CTR_DW27_0_FLD_AC3_CTB_CRT_CTB__, (val))

#define MLO_INFO_DW28_VAL_SECONDARY_MLD_BAND(val)              REG_FLD_VAL(MLO_INFO_DW28_FLD_SECONDARY_MLD_BAND, (val))
#define MLO_INFO_DW28_VAL_RELATED_BAND1(val)                   REG_FLD_VAL(MLO_INFO_DW28_FLD_RELATED_BAND1, (val))
#define MLO_INFO_DW28_VAL_RELATED_IDX1(val)                    REG_FLD_VAL(MLO_INFO_DW28_FLD_RELATED_IDX1, (val))
#define MLO_INFO_DW28_VAL_PRIMARY_MLD_BAND(val)                REG_FLD_VAL(MLO_INFO_DW28_FLD_PRIMARY_MLD_BAND, (val))
#define MLO_INFO_DW28_VAL_RELATED_BAND0(val)                   REG_FLD_VAL(MLO_INFO_DW28_FLD_RELATED_BAND0, (val))
#define MLO_INFO_DW28_VAL_RELATED_IDX0(val)                    REG_FLD_VAL(MLO_INFO_DW28_FLD_RELATED_IDX0, (val))

#define MLO_INFO_DW29_VAL_STR_BITMAP(val)                      REG_FLD_VAL(MLO_INFO_DW29_FLD_STR_BITMAP, (val))
#define MLO_INFO_DW29_VAL_EMLMR2(val)                          REG_FLD_VAL(MLO_INFO_DW29_FLD_EMLMR2, (val))
#define MLO_INFO_DW29_VAL_EMLSR2(val)                          REG_FLD_VAL(MLO_INFO_DW29_FLD_EMLSR2, (val))
#define MLO_INFO_DW29_VAL_EMLMR1(val)                          REG_FLD_VAL(MLO_INFO_DW29_FLD_EMLMR1, (val))
#define MLO_INFO_DW29_VAL_EMLSR1(val)                          REG_FLD_VAL(MLO_INFO_DW29_FLD_EMLSR1, (val))
#define MLO_INFO_DW29_VAL_EMLMR0(val)                          REG_FLD_VAL(MLO_INFO_DW29_FLD_EMLMR0, (val))
#define MLO_INFO_DW29_VAL_EMLSR0(val)                          REG_FLD_VAL(MLO_INFO_DW29_FLD_EMLSR0, (val))
#define MLO_INFO_DW29_VAL_OWN_MLD_ID(val)                      REG_FLD_VAL(MLO_INFO_DW29_FLD_OWN_MLD_ID, (val))
#define MLO_INFO_DW29_VAL_DISPATCH_POLICY7(val)                REG_FLD_VAL(MLO_INFO_DW29_FLD_DISPATCH_POLICY7, (val))
#define MLO_INFO_DW29_VAL_DISPATCH_POLICY6(val)                REG_FLD_VAL(MLO_INFO_DW29_FLD_DISPATCH_POLICY6, (val))
#define MLO_INFO_DW29_VAL_DISPATCH_POLICY5(val)                REG_FLD_VAL(MLO_INFO_DW29_FLD_DISPATCH_POLICY5, (val))
#define MLO_INFO_DW29_VAL_DISPATCH_POLICY4(val)                REG_FLD_VAL(MLO_INFO_DW29_FLD_DISPATCH_POLICY4, (val))
#define MLO_INFO_DW29_VAL_DISPATCH_POLICY3(val)                REG_FLD_VAL(MLO_INFO_DW29_FLD_DISPATCH_POLICY3, (val))
#define MLO_INFO_DW29_VAL_DISPATCH_POLICY2(val)                REG_FLD_VAL(MLO_INFO_DW29_FLD_DISPATCH_POLICY2, (val))
#define MLO_INFO_DW29_VAL_DISPATCH_POLICY1(val)                REG_FLD_VAL(MLO_INFO_DW29_FLD_DISPATCH_POLICY1, (val))
#define MLO_INFO_DW29_VAL_DISPATCH_POLICY0(val)                REG_FLD_VAL(MLO_INFO_DW29_FLD_DISPATCH_POLICY0, (val))

#define MLO_INFO_DW30_VAL_LINK_MGF(val)                        REG_FLD_VAL(MLO_INFO_DW30_FLD_LINK_MGF, (val))
#define MLO_INFO_DW30_VAL_EMLSR_TRANS_DLY_IDX(val)             REG_FLD_VAL(MLO_INFO_DW30_FLD_EMLSR_TRANS_DLY_IDX, (val))
#define MLO_INFO_DW30_VAL_DISPATCH_RATIO(val)                  REG_FLD_VAL(MLO_INFO_DW30_FLD_DISPATCH_RATIO, (val))
#define MLO_INFO_DW30_VAL_DISPATCH_ORDER(val)                  REG_FLD_VAL(MLO_INFO_DW30_FLD_DISPATCH_ORDER, (val))

#define RESP_INFO_DW31_VAL_ACK_EN(val)                         REG_FLD_VAL(RESP_INFO_DW31_FLD_ACK_EN, (val))
#define RESP_INFO_DW31_VAL_RXD_DUP_MODE(val)                   REG_FLD_VAL(RESP_INFO_DW31_FLD_RXD_DUP_MODE, (val))
#define RESP_INFO_DW31_VAL_MPDU_SIZE(val)                      REG_FLD_VAL(RESP_INFO_DW31_FLD_MPDU_SIZE, (val))
#define RESP_INFO_DW31_VAL_ALL_ACK(val)                        REG_FLD_VAL(RESP_INFO_DW31_FLD_ALL_ACK, (val))
#define RESP_INFO_DW31_VAL_CASCAD(val)                         REG_FLD_VAL(RESP_INFO_DW31_FLD_CASCAD, (val))
#define RESP_INFO_DW31_VAL_DROP(val)                           REG_FLD_VAL(RESP_INFO_DW31_FLD_DROP, (val))
#define RESP_INFO_DW31_VAL_BFTX_TB(val)                        REG_FLD_VAL(RESP_INFO_DW31_FLD_BFTX_TB, (val))

#define RX_DUP_INFO_DW32_VAL_RXD_DUP_WHITE_LIST(val)           REG_FLD_VAL(RX_DUP_INFO_DW32_FLD_RXD_DUP_WHITE_LIST, (val))
#define RX_DUP_INFO_DW32_VAL_RXD_DUP_FOR_OM_CHG(val)           REG_FLD_VAL(RX_DUP_INFO_DW32_FLD_RXD_DUP_FOR_OM_CHG, (val))
#define RX_DUP_INFO_DW32_VAL_OM_INFO_EHT(val)                  REG_FLD_VAL(RX_DUP_INFO_DW32_FLD_OM_INFO_EHT, (val))
#define RX_DUP_INFO_DW32_VAL_OM_INFO(val)                      REG_FLD_VAL(RX_DUP_INFO_DW32_FLD_OM_INFO, (val))

#define RX_STAT_DW33_VAL_AMSDU_CROSS_LG(val)                   REG_FLD_VAL(RX_STAT_DW33_FLD_AMSDU_CROSS_LG, (val))
#define RX_STAT_DW33_VAL_HT_AMSDU(val)                         REG_FLD_VAL(RX_STAT_DW33_FLD_HT_AMSDU, (val))
#define RX_STAT_DW33_VAL_RAPID_REACTION_RATE(val)              REG_FLD_VAL(RX_STAT_DW33_FLD_RAPID_REACTION_RATE, (val))
#define RX_STAT_DW33_VAL_USER_SNR(val)                         REG_FLD_VAL(RX_STAT_DW33_FLD_USER_SNR, (val))
#define RX_STAT_DW33_VAL_USER_RSSI(val)                        REG_FLD_VAL(RX_STAT_DW33_FLD_USER_RSSI, (val))

#define RX_STAT_DW34_VAL_RESP_RCPI3(val)                       REG_FLD_VAL(RX_STAT_DW34_FLD_RESP_RCPI3, (val))
#define RX_STAT_DW34_VAL_RESP_RCPI2(val)                       REG_FLD_VAL(RX_STAT_DW34_FLD_RESP_RCPI2, (val))
#define RX_STAT_DW34_VAL_RESP_RCPI1(val)                       REG_FLD_VAL(RX_STAT_DW34_FLD_RESP_RCPI1, (val))
#define RX_STAT_DW34_VAL_RESP_RCPI0(val)                       REG_FLD_VAL(RX_STAT_DW34_FLD_RESP_RCPI0, (val))

#define RX_STAT_DW35_VAL_SNR_RX3(val)                          REG_FLD_VAL(RX_STAT_DW35_FLD_SNR_RX3, (val))
#define RX_STAT_DW35_VAL_SNR_RX2(val)                          REG_FLD_VAL(RX_STAT_DW35_FLD_SNR_RX2, (val))
#define RX_STAT_DW35_VAL_SNR_RX1(val)                          REG_FLD_VAL(RX_STAT_DW35_FLD_SNR_RX1, (val))
#define RX_STAT_DW35_VAL_SNR_RX0(val)                          REG_FLD_VAL(RX_STAT_DW35_FLD_SNR_RX0, (val))

#define VERSION_CODE_VAL_VERSION_CODE(val)                     REG_FLD_VAL(VERSION_CODE_FLD_VERSION_CODE, (val))

#ifdef __cplusplus
}
#endif

#endif // __WF_DS_LWTBL_REGS_H__
