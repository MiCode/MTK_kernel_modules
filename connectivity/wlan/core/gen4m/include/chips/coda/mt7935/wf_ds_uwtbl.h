/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef __WF_DS_UWTBL_REGS_H__
#define __WF_DS_UWTBL_REGS_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef REG_BASE_C_MODULE
// ----------------- WF_DS_UWTBL Bit Field Definitions -------------------

#define PACKING

typedef PACKING union
{
    PACKING struct
    {
        FIELD PEER_MLD_ADDRESS_47_32_   : 16;
        FIELD OWN_MLD_ID                : 6;
        FIELD rsv_22                    : 10;
    } Bits;
    uint32_t Raw;
} WF_DS_UWTBL_REG_MLO_INFO_DW00, *PWF_DS_UWTBL_REG_MLO_INFO_DW00;

typedef PACKING union
{
    PACKING struct
    {
        FIELD PEER_MLD_ADDRESS_31_0_    : 32;
    } Bits;
    uint32_t Raw;
} WF_DS_UWTBL_REG_MLO_INFO_DW01, *PWF_DS_UWTBL_REG_MLO_INFO_DW01;

typedef PACKING union
{
    PACKING struct
    {
        FIELD PN_31_0_                  : 32;
    } Bits;
    uint32_t Raw;
} WF_DS_UWTBL_REG_PN_SN_DW02, *PWF_DS_UWTBL_REG_PN_SN_DW02;

typedef PACKING union
{
    PACKING struct
    {
        FIELD PN_47_32_                 : 16;
        FIELD COM_SN                    : 12;
        FIELD rsv_28                    : 4;
    } Bits;
    uint32_t Raw;
} WF_DS_UWTBL_REG_PN_SN_DW03, *PWF_DS_UWTBL_REG_PN_SN_DW03;

typedef PACKING union
{
    PACKING struct
    {
        FIELD TID0_SN                   : 12;
        FIELD TID1_SN                   : 12;
        FIELD TID2_SN_7_0_              : 8;
    } Bits;
    uint32_t Raw;
} WF_DS_UWTBL_REG_PN_SN_DW04_0, *PWF_DS_UWTBL_REG_PN_SN_DW04_0;

typedef PACKING union
{
    PACKING struct
    {
        FIELD RX_BIPN_31_0_             : 32;
    } Bits;
    uint32_t Raw;
} WF_DS_UWTBL_REG_PN_SN_DW04_1, *PWF_DS_UWTBL_REG_PN_SN_DW04_1;

typedef PACKING union
{
    PACKING struct
    {
        FIELD TID2_SN_11_8_             : 4;
        FIELD TID3_SN                   : 12;
        FIELD TID4_SN                   : 12;
        FIELD TID5_SN_3_0_              : 4;
    } Bits;
    uint32_t Raw;
} WF_DS_UWTBL_REG_PN_SN_DW05_0, *PWF_DS_UWTBL_REG_PN_SN_DW05_0;

typedef PACKING union
{
    PACKING struct
    {
        FIELD RX_BIPN_47_32_            : 16;
        FIELD rsv_16                    : 16;
    } Bits;
    uint32_t Raw;
} WF_DS_UWTBL_REG_PN_SN_DW05_1, *PWF_DS_UWTBL_REG_PN_SN_DW05_1;

typedef PACKING union
{
    PACKING struct
    {
        FIELD TID5_SN_11_4_             : 8;
        FIELD TID6_SN                   : 12;
        FIELD TID7_SN                   : 12;
    } Bits;
    uint32_t Raw;
} WF_DS_UWTBL_REG_PN_SN_DW06_0, *PWF_DS_UWTBL_REG_PN_SN_DW06_0;

typedef PACKING union
{
    PACKING struct
    {
        FIELD KEY_LOC2                  : 13;
        FIELD rsv_13                    : 19;
    } Bits;
    uint32_t Raw;
} WF_DS_UWTBL_REG_PN_SN_DW06_1, *PWF_DS_UWTBL_REG_PN_SN_DW06_1;

typedef PACKING union
{
    PACKING struct
    {
        FIELD KEY_LOC0                  : 13;
        FIELD rsv_13                    : 3;
        FIELD KEY_LOC1                  : 13;
        FIELD rsv_29                    : 3;
    } Bits;
    uint32_t Raw;
} WF_DS_UWTBL_REG_KEY_INFO_DW07, *PWF_DS_UWTBL_REG_KEY_INFO_DW07;

typedef PACKING union
{
    PACKING struct
    {
        FIELD AMSDU_CFG                 : 12;
        FIELD rsv_12                    : 8;
        FIELD SEC_ADDR_MODE             : 2;
        FIELD SPP_EN                    : 1;
        FIELD WPI_EVEN                  : 1;
        FIELD AAD_OM                    : 1;
        FIELD WMM_Q                     : 2;
        FIELD QOS                       : 1;
        FIELD HT                        : 1;
        FIELD HDRT_MODE                 : 1;
        FIELD BYPASS_RRO                : 1;
        FIELD rsv_31                    : 1;
    } Bits;
    uint32_t Raw;
} WF_DS_UWTBL_REG_MSDU_INFO_DW08, *PWF_DS_UWTBL_REG_MSDU_INFO_DW08;

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
} WF_DS_UWTBL_REG_MLO_INFO_DW09, *PWF_DS_UWTBL_REG_MLO_INFO_DW09;

typedef PACKING union
{
    PACKING struct
    {
        FIELD MSDU_RX_COUNT             : 32;
    } Bits;
    uint32_t Raw;
} WF_DS_UWTBL_REG_MSDU_COUNTER_DW10, *PWF_DS_UWTBL_REG_MSDU_COUNTER_DW10;

typedef PACKING union
{
    PACKING struct
    {
        FIELD MSDU_TX_COUNT             : 32;
    } Bits;
    uint32_t Raw;
} WF_DS_UWTBL_REG_MSDU_COUNTER_DW11, *PWF_DS_UWTBL_REG_MSDU_COUNTER_DW11;

typedef PACKING union
{
    PACKING struct
    {
        FIELD MSDU_TX_RETRY_CONT        : 32;
    } Bits;
    uint32_t Raw;
} WF_DS_UWTBL_REG_MSDU_COUNTER_DW12, *PWF_DS_UWTBL_REG_MSDU_COUNTER_DW12;

typedef PACKING union
{
    PACKING struct
    {
        FIELD MSDU_TX_DROP_COUNT        : 32;
    } Bits;
    uint32_t Raw;
} WF_DS_UWTBL_REG_MSDU_COUNTER_DW13, *PWF_DS_UWTBL_REG_MSDU_COUNTER_DW13;

typedef PACKING union
{
    PACKING struct
    {
        FIELD MSDU_TX_HAD_RETRIED_COUNT : 32;
    } Bits;
    uint32_t Raw;
} WF_DS_UWTBL_REG_MSDU_COUNTER_DW14, *PWF_DS_UWTBL_REG_MSDU_COUNTER_DW14;

typedef PACKING union
{
    PACKING struct
    {
        FIELD MSDU_TX_RETRY_LARGE_THAN_ONE_COUNT : 32;
    } Bits;
    uint32_t Raw;
} WF_DS_UWTBL_REG_MSDU_COUNTER_DW15, *PWF_DS_UWTBL_REG_MSDU_COUNTER_DW15;

typedef PACKING union
{
    PACKING struct
    {
        FIELD VERSION_CODE              : 32;
    } Bits;
    uint32_t Raw;
} WF_DS_UWTBL_REG_VERSION_CODE, *PWF_DS_UWTBL_REG_VERSION_CODE;

// ----------------- WF_DS_UWTBL  Grouping Definitions -------------------
// ----------------- WF_DS_UWTBL Register Definition -------------------
typedef volatile PACKING struct
{
    WF_DS_UWTBL_REG_MLO_INFO_DW00   MLO_INFO_DW00;    // 6000
    uint32_t                          rsv_6004[7];      // 6004..601C
    WF_DS_UWTBL_REG_MLO_INFO_DW01   MLO_INFO_DW01;    // 6020
    uint32_t                          rsv_6024[7];      // 6024..603C
    WF_DS_UWTBL_REG_PN_SN_DW02      PN_SN_DW02;       // 6040
    uint32_t                          rsv_6044[7];      // 6044..605C
    WF_DS_UWTBL_REG_PN_SN_DW03      PN_SN_DW03;       // 6060
    uint32_t                          rsv_6064[7];      // 6064..607C
    WF_DS_UWTBL_REG_PN_SN_DW04_0    PN_SN_DW04_0;     // 6080
    WF_DS_UWTBL_REG_PN_SN_DW04_1    PN_SN_DW04_1;     // 6084
    uint32_t                          rsv_6088[6];      // 6088..609C
    WF_DS_UWTBL_REG_PN_SN_DW05_0    PN_SN_DW05_0;     // 60A0
    WF_DS_UWTBL_REG_PN_SN_DW05_1    PN_SN_DW05_1;     // 60A4
    uint32_t                          rsv_60A8[6];      // 60A8..60BC
    WF_DS_UWTBL_REG_PN_SN_DW06_0    PN_SN_DW06_0;     // 60C0
    WF_DS_UWTBL_REG_PN_SN_DW06_1    PN_SN_DW06_1;     // 60C4
    uint32_t                          rsv_60C8[6];      // 60C8..60DC
    WF_DS_UWTBL_REG_KEY_INFO_DW07   KEY_INFO_DW07;    // 60E0
    uint32_t                          rsv_60E4[7];      // 60E4..60FC
    WF_DS_UWTBL_REG_MSDU_INFO_DW08  MSDU_INFO_DW08;   // 6100
    uint32_t                          rsv_6104[7];      // 6104..611C
    WF_DS_UWTBL_REG_MLO_INFO_DW09   MLO_INFO_DW09;    // 6120
    uint32_t                          rsv_6124[7];      // 6124..613C
    WF_DS_UWTBL_REG_MSDU_COUNTER_DW10 MSDU_COUNTER_DW10; // 6140
    uint32_t                          rsv_6144[7];      // 6144..615C
    WF_DS_UWTBL_REG_MSDU_COUNTER_DW11 MSDU_COUNTER_DW11; // 6160
    uint32_t                          rsv_6164[7];      // 6164..617C
    WF_DS_UWTBL_REG_MSDU_COUNTER_DW12 MSDU_COUNTER_DW12; // 6180
    uint32_t                          rsv_6184[7];      // 6184..619C
    WF_DS_UWTBL_REG_MSDU_COUNTER_DW13 MSDU_COUNTER_DW13; // 61A0
    uint32_t                          rsv_61A4[7];      // 61A4..61BC
    WF_DS_UWTBL_REG_MSDU_COUNTER_DW14 MSDU_COUNTER_DW14; // 61C0
    uint32_t                          rsv_61C4[7];      // 61C4..61DC
    WF_DS_UWTBL_REG_MSDU_COUNTER_DW15 MSDU_COUNTER_DW15; // 61E0
    uint32_t                          rsv_61E4[16262];  // 61E4..5FF8
    uint8_t                           rsv_5FFC;         // 5FFC
    uint16_t                          rsv_5FFD;         // 5FFD
    WF_DS_UWTBL_REG_VERSION_CODE    VERSION_CODE;     // 5FFF
}WF_DS_UWTBL_REGS, *PWF_DS_UWTBL_REGS;

// ---------- WF_DS_UWTBL Enum Definitions      ----------
// ---------- WF_DS_UWTBL C Macro Definitions   ----------
extern PWF_DS_UWTBL_REGS g_WF_DS_UWTBL_BASE;

#define WF_DS_UWTBL_BASE                                       (g_WF_DS_UWTBL_BASE)

#define WF_DS_UWTBL_MLO_INFO_DW00                              INREG32(&WF_DS_UWTBL_BASE->MLO_INFO_DW00) // 6000
#define WF_DS_UWTBL_MLO_INFO_DW01                              INREG32(&WF_DS_UWTBL_BASE->MLO_INFO_DW01) // 6020
#define WF_DS_UWTBL_PN_SN_DW02                                 INREG32(&WF_DS_UWTBL_BASE->PN_SN_DW02) // 6040
#define WF_DS_UWTBL_PN_SN_DW03                                 INREG32(&WF_DS_UWTBL_BASE->PN_SN_DW03) // 6060
#define WF_DS_UWTBL_PN_SN_DW04_0                               INREG32(&WF_DS_UWTBL_BASE->PN_SN_DW04_0) // 6080
#define WF_DS_UWTBL_PN_SN_DW04_1                               INREG32(&WF_DS_UWTBL_BASE->PN_SN_DW04_1) // 6084
#define WF_DS_UWTBL_PN_SN_DW05_0                               INREG32(&WF_DS_UWTBL_BASE->PN_SN_DW05_0) // 60A0
#define WF_DS_UWTBL_PN_SN_DW05_1                               INREG32(&WF_DS_UWTBL_BASE->PN_SN_DW05_1) // 60A4
#define WF_DS_UWTBL_PN_SN_DW06_0                               INREG32(&WF_DS_UWTBL_BASE->PN_SN_DW06_0) // 60C0
#define WF_DS_UWTBL_PN_SN_DW06_1                               INREG32(&WF_DS_UWTBL_BASE->PN_SN_DW06_1) // 60C4
#define WF_DS_UWTBL_KEY_INFO_DW07                              INREG32(&WF_DS_UWTBL_BASE->KEY_INFO_DW07) // 60E0
#define WF_DS_UWTBL_MSDU_INFO_DW08                             INREG32(&WF_DS_UWTBL_BASE->MSDU_INFO_DW08) // 6100
#define WF_DS_UWTBL_MLO_INFO_DW09                              INREG32(&WF_DS_UWTBL_BASE->MLO_INFO_DW09) // 6120
#define WF_DS_UWTBL_MSDU_COUNTER_DW10                          INREG32(&WF_DS_UWTBL_BASE->MSDU_COUNTER_DW10) // 6140
#define WF_DS_UWTBL_MSDU_COUNTER_DW11                          INREG32(&WF_DS_UWTBL_BASE->MSDU_COUNTER_DW11) // 6160
#define WF_DS_UWTBL_MSDU_COUNTER_DW12                          INREG32(&WF_DS_UWTBL_BASE->MSDU_COUNTER_DW12) // 6180
#define WF_DS_UWTBL_MSDU_COUNTER_DW13                          INREG32(&WF_DS_UWTBL_BASE->MSDU_COUNTER_DW13) // 61A0
#define WF_DS_UWTBL_MSDU_COUNTER_DW14                          INREG32(&WF_DS_UWTBL_BASE->MSDU_COUNTER_DW14) // 61C0
#define WF_DS_UWTBL_MSDU_COUNTER_DW15                          INREG32(&WF_DS_UWTBL_BASE->MSDU_COUNTER_DW15) // 61E0
#define WF_DS_UWTBL_VERSION_CODE                               INREG32(&WF_DS_UWTBL_BASE->VERSION_CODE) // 5FFF

#endif


#define MLO_INFO_DW00_FLD_OWN_MLD_ID                           REG_FLD(6, 16)
#define MLO_INFO_DW00_FLD_PEER_MLD_ADDRESS_47_32_              REG_FLD(16, 0)

#define MLO_INFO_DW01_FLD_PEER_MLD_ADDRESS_31_0_               REG_FLD(32, 0)

#define PN_SN_DW02_FLD_PN_31_0_                                REG_FLD(32, 0)

#define PN_SN_DW03_FLD_COM_SN                                  REG_FLD(12, 16)
#define PN_SN_DW03_FLD_PN_47_32_                               REG_FLD(16, 0)

#define PN_SN_DW04_0_FLD_TID2_SN_7_0_                          REG_FLD(8, 24)
#define PN_SN_DW04_0_FLD_TID1_SN                               REG_FLD(12, 12)
#define PN_SN_DW04_0_FLD_TID0_SN                               REG_FLD(12, 0)

#define PN_SN_DW04_1_FLD_RX_BIPN_31_0_                         REG_FLD(32, 0)

#define PN_SN_DW05_0_FLD_TID5_SN_3_0_                          REG_FLD(4, 28)
#define PN_SN_DW05_0_FLD_TID4_SN                               REG_FLD(12, 16)
#define PN_SN_DW05_0_FLD_TID3_SN                               REG_FLD(12, 4)
#define PN_SN_DW05_0_FLD_TID2_SN_11_8_                         REG_FLD(4, 0)

#define PN_SN_DW05_1_FLD_RX_BIPN_47_32_                        REG_FLD(16, 0)

#define PN_SN_DW06_0_FLD_TID7_SN                               REG_FLD(12, 20)
#define PN_SN_DW06_0_FLD_TID6_SN                               REG_FLD(12, 8)
#define PN_SN_DW06_0_FLD_TID5_SN_11_4_                         REG_FLD(8, 0)

#define PN_SN_DW06_1_FLD_KEY_LOC2                              REG_FLD(13, 0)

#define KEY_INFO_DW07_FLD_KEY_LOC1                             REG_FLD(13, 16)
#define KEY_INFO_DW07_FLD_KEY_LOC0                             REG_FLD(13, 0)

#define MSDU_INFO_DW08_FLD_BYPASS_RRO                          REG_FLD(1, 30)
#define MSDU_INFO_DW08_FLD_HDRT_MODE                           REG_FLD(1, 29)
#define MSDU_INFO_DW08_FLD_HT                                  REG_FLD(1, 28)
#define MSDU_INFO_DW08_FLD_QOS                                 REG_FLD(1, 27)
#define MSDU_INFO_DW08_FLD_WMM_Q                               REG_FLD(2, 25)
#define MSDU_INFO_DW08_FLD_AAD_OM                              REG_FLD(1, 24)
#define MSDU_INFO_DW08_FLD_WPI_EVEN                            REG_FLD(1, 23)
#define MSDU_INFO_DW08_FLD_SPP_EN                              REG_FLD(1, 22)
#define MSDU_INFO_DW08_FLD_SEC_ADDR_MODE                       REG_FLD(2, 20)
#define MSDU_INFO_DW08_FLD_AMSDU_CFG                           REG_FLD(12, 0)

#define MLO_INFO_DW09_FLD_SECONDARY_MLD_BAND                   REG_FLD(2, 30)
#define MLO_INFO_DW09_FLD_RELATED_BAND1                        REG_FLD(2, 28)
#define MLO_INFO_DW09_FLD_RELATED_IDX1                         REG_FLD(12, 16)
#define MLO_INFO_DW09_FLD_PRIMARY_MLD_BAND                     REG_FLD(2, 14)
#define MLO_INFO_DW09_FLD_RELATED_BAND0                        REG_FLD(2, 12)
#define MLO_INFO_DW09_FLD_RELATED_IDX0                         REG_FLD(12, 0)

#define MSDU_COUNTER_DW10_FLD_MSDU_RX_COUNT                    REG_FLD(32, 0)

#define MSDU_COUNTER_DW11_FLD_MSDU_TX_COUNT                    REG_FLD(32, 0)

#define MSDU_COUNTER_DW12_FLD_MSDU_TX_RETRY_CONT               REG_FLD(32, 0)

#define MSDU_COUNTER_DW13_FLD_MSDU_TX_DROP_COUNT               REG_FLD(32, 0)

#define MSDU_COUNTER_DW14_FLD_MSDU_TX_HAD_RETRIED_COUNT        REG_FLD(32, 0)

#define MSDU_COUNTER_DW15_FLD_MSDU_TX_RETRY_LARGE_THAN_ONE_COUNT REG_FLD(32, 0)

#define VERSION_CODE_FLD_VERSION_CODE                          REG_FLD(32, 0)

#define MLO_INFO_DW00_GET_OWN_MLD_ID(reg32)                    REG_FLD_GET(MLO_INFO_DW00_FLD_OWN_MLD_ID, (reg32))
#define MLO_INFO_DW00_GET_PEER_MLD_ADDRESS_47_32_(reg32)       REG_FLD_GET(MLO_INFO_DW00_FLD_PEER_MLD_ADDRESS_47_32_, (reg32))

#define MLO_INFO_DW01_GET_PEER_MLD_ADDRESS_31_0_(reg32)        REG_FLD_GET(MLO_INFO_DW01_FLD_PEER_MLD_ADDRESS_31_0_, (reg32))

#define PN_SN_DW02_GET_PN_31_0_(reg32)                         REG_FLD_GET(PN_SN_DW02_FLD_PN_31_0_, (reg32))

#define PN_SN_DW03_GET_COM_SN(reg32)                           REG_FLD_GET(PN_SN_DW03_FLD_COM_SN, (reg32))
#define PN_SN_DW03_GET_PN_47_32_(reg32)                        REG_FLD_GET(PN_SN_DW03_FLD_PN_47_32_, (reg32))

#define PN_SN_DW04_0_GET_TID2_SN_7_0_(reg32)                   REG_FLD_GET(PN_SN_DW04_0_FLD_TID2_SN_7_0_, (reg32))
#define PN_SN_DW04_0_GET_TID1_SN(reg32)                        REG_FLD_GET(PN_SN_DW04_0_FLD_TID1_SN, (reg32))
#define PN_SN_DW04_0_GET_TID0_SN(reg32)                        REG_FLD_GET(PN_SN_DW04_0_FLD_TID0_SN, (reg32))

#define PN_SN_DW04_1_GET_RX_BIPN_31_0_(reg32)                  REG_FLD_GET(PN_SN_DW04_1_FLD_RX_BIPN_31_0_, (reg32))

#define PN_SN_DW05_0_GET_TID5_SN_3_0_(reg32)                   REG_FLD_GET(PN_SN_DW05_0_FLD_TID5_SN_3_0_, (reg32))
#define PN_SN_DW05_0_GET_TID4_SN(reg32)                        REG_FLD_GET(PN_SN_DW05_0_FLD_TID4_SN, (reg32))
#define PN_SN_DW05_0_GET_TID3_SN(reg32)                        REG_FLD_GET(PN_SN_DW05_0_FLD_TID3_SN, (reg32))
#define PN_SN_DW05_0_GET_TID2_SN_11_8_(reg32)                  REG_FLD_GET(PN_SN_DW05_0_FLD_TID2_SN_11_8_, (reg32))

#define PN_SN_DW05_1_GET_RX_BIPN_47_32_(reg32)                 REG_FLD_GET(PN_SN_DW05_1_FLD_RX_BIPN_47_32_, (reg32))

#define PN_SN_DW06_0_GET_TID7_SN(reg32)                        REG_FLD_GET(PN_SN_DW06_0_FLD_TID7_SN, (reg32))
#define PN_SN_DW06_0_GET_TID6_SN(reg32)                        REG_FLD_GET(PN_SN_DW06_0_FLD_TID6_SN, (reg32))
#define PN_SN_DW06_0_GET_TID5_SN_11_4_(reg32)                  REG_FLD_GET(PN_SN_DW06_0_FLD_TID5_SN_11_4_, (reg32))

#define PN_SN_DW06_1_GET_KEY_LOC2(reg32)                       REG_FLD_GET(PN_SN_DW06_1_FLD_KEY_LOC2, (reg32))

#define KEY_INFO_DW07_GET_KEY_LOC1(reg32)                      REG_FLD_GET(KEY_INFO_DW07_FLD_KEY_LOC1, (reg32))
#define KEY_INFO_DW07_GET_KEY_LOC0(reg32)                      REG_FLD_GET(KEY_INFO_DW07_FLD_KEY_LOC0, (reg32))

#define MSDU_INFO_DW08_GET_BYPASS_RRO(reg32)                   REG_FLD_GET(MSDU_INFO_DW08_FLD_BYPASS_RRO, (reg32))
#define MSDU_INFO_DW08_GET_HDRT_MODE(reg32)                    REG_FLD_GET(MSDU_INFO_DW08_FLD_HDRT_MODE, (reg32))
#define MSDU_INFO_DW08_GET_HT(reg32)                           REG_FLD_GET(MSDU_INFO_DW08_FLD_HT, (reg32))
#define MSDU_INFO_DW08_GET_QOS(reg32)                          REG_FLD_GET(MSDU_INFO_DW08_FLD_QOS, (reg32))
#define MSDU_INFO_DW08_GET_WMM_Q(reg32)                        REG_FLD_GET(MSDU_INFO_DW08_FLD_WMM_Q, (reg32))
#define MSDU_INFO_DW08_GET_AAD_OM(reg32)                       REG_FLD_GET(MSDU_INFO_DW08_FLD_AAD_OM, (reg32))
#define MSDU_INFO_DW08_GET_WPI_EVEN(reg32)                     REG_FLD_GET(MSDU_INFO_DW08_FLD_WPI_EVEN, (reg32))
#define MSDU_INFO_DW08_GET_SPP_EN(reg32)                       REG_FLD_GET(MSDU_INFO_DW08_FLD_SPP_EN, (reg32))
#define MSDU_INFO_DW08_GET_SEC_ADDR_MODE(reg32)                REG_FLD_GET(MSDU_INFO_DW08_FLD_SEC_ADDR_MODE, (reg32))
#define MSDU_INFO_DW08_GET_AMSDU_CFG(reg32)                    REG_FLD_GET(MSDU_INFO_DW08_FLD_AMSDU_CFG, (reg32))

#define MLO_INFO_DW09_GET_SECONDARY_MLD_BAND(reg32)            REG_FLD_GET(MLO_INFO_DW09_FLD_SECONDARY_MLD_BAND, (reg32))
#define MLO_INFO_DW09_GET_RELATED_BAND1(reg32)                 REG_FLD_GET(MLO_INFO_DW09_FLD_RELATED_BAND1, (reg32))
#define MLO_INFO_DW09_GET_RELATED_IDX1(reg32)                  REG_FLD_GET(MLO_INFO_DW09_FLD_RELATED_IDX1, (reg32))
#define MLO_INFO_DW09_GET_PRIMARY_MLD_BAND(reg32)              REG_FLD_GET(MLO_INFO_DW09_FLD_PRIMARY_MLD_BAND, (reg32))
#define MLO_INFO_DW09_GET_RELATED_BAND0(reg32)                 REG_FLD_GET(MLO_INFO_DW09_FLD_RELATED_BAND0, (reg32))
#define MLO_INFO_DW09_GET_RELATED_IDX0(reg32)                  REG_FLD_GET(MLO_INFO_DW09_FLD_RELATED_IDX0, (reg32))

#define MSDU_COUNTER_DW10_GET_MSDU_RX_COUNT(reg32)             REG_FLD_GET(MSDU_COUNTER_DW10_FLD_MSDU_RX_COUNT, (reg32))

#define MSDU_COUNTER_DW11_GET_MSDU_TX_COUNT(reg32)             REG_FLD_GET(MSDU_COUNTER_DW11_FLD_MSDU_TX_COUNT, (reg32))

#define MSDU_COUNTER_DW12_GET_MSDU_TX_RETRY_CONT(reg32)        REG_FLD_GET(MSDU_COUNTER_DW12_FLD_MSDU_TX_RETRY_CONT, (reg32))

#define MSDU_COUNTER_DW13_GET_MSDU_TX_DROP_COUNT(reg32)        REG_FLD_GET(MSDU_COUNTER_DW13_FLD_MSDU_TX_DROP_COUNT, (reg32))

#define MSDU_COUNTER_DW14_GET_MSDU_TX_HAD_RETRIED_COUNT(reg32) REG_FLD_GET(MSDU_COUNTER_DW14_FLD_MSDU_TX_HAD_RETRIED_COUNT, (reg32))

#define MSDU_COUNTER_DW15_GET_MSDU_TX_RETRY_LARGE_THAN_ONE_COUNT(reg32) REG_FLD_GET(MSDU_COUNTER_DW15_FLD_MSDU_TX_RETRY_LARGE_THAN_ONE_COUNT, (reg32))

#define VERSION_CODE_GET_VERSION_CODE(reg32)                   REG_FLD_GET(VERSION_CODE_FLD_VERSION_CODE, (reg32))

#define MLO_INFO_DW00_SET_OWN_MLD_ID(reg32, val)               REG_FLD_SET(MLO_INFO_DW00_FLD_OWN_MLD_ID, (reg32), (val))
#define MLO_INFO_DW00_SET_PEER_MLD_ADDRESS_47_32_(reg32, val)  REG_FLD_SET(MLO_INFO_DW00_FLD_PEER_MLD_ADDRESS_47_32_, (reg32), (val))

#define MLO_INFO_DW01_SET_PEER_MLD_ADDRESS_31_0_(reg32, val)   REG_FLD_SET(MLO_INFO_DW01_FLD_PEER_MLD_ADDRESS_31_0_, (reg32), (val))

#define PN_SN_DW02_SET_PN_31_0_(reg32, val)                    REG_FLD_SET(PN_SN_DW02_FLD_PN_31_0_, (reg32), (val))

#define PN_SN_DW03_SET_COM_SN(reg32, val)                      REG_FLD_SET(PN_SN_DW03_FLD_COM_SN, (reg32), (val))
#define PN_SN_DW03_SET_PN_47_32_(reg32, val)                   REG_FLD_SET(PN_SN_DW03_FLD_PN_47_32_, (reg32), (val))

#define PN_SN_DW04_0_SET_TID2_SN_7_0_(reg32, val)              REG_FLD_SET(PN_SN_DW04_0_FLD_TID2_SN_7_0_, (reg32), (val))
#define PN_SN_DW04_0_SET_TID1_SN(reg32, val)                   REG_FLD_SET(PN_SN_DW04_0_FLD_TID1_SN, (reg32), (val))
#define PN_SN_DW04_0_SET_TID0_SN(reg32, val)                   REG_FLD_SET(PN_SN_DW04_0_FLD_TID0_SN, (reg32), (val))

#define PN_SN_DW04_1_SET_RX_BIPN_31_0_(reg32, val)             REG_FLD_SET(PN_SN_DW04_1_FLD_RX_BIPN_31_0_, (reg32), (val))

#define PN_SN_DW05_0_SET_TID5_SN_3_0_(reg32, val)              REG_FLD_SET(PN_SN_DW05_0_FLD_TID5_SN_3_0_, (reg32), (val))
#define PN_SN_DW05_0_SET_TID4_SN(reg32, val)                   REG_FLD_SET(PN_SN_DW05_0_FLD_TID4_SN, (reg32), (val))
#define PN_SN_DW05_0_SET_TID3_SN(reg32, val)                   REG_FLD_SET(PN_SN_DW05_0_FLD_TID3_SN, (reg32), (val))
#define PN_SN_DW05_0_SET_TID2_SN_11_8_(reg32, val)             REG_FLD_SET(PN_SN_DW05_0_FLD_TID2_SN_11_8_, (reg32), (val))

#define PN_SN_DW05_1_SET_RX_BIPN_47_32_(reg32, val)            REG_FLD_SET(PN_SN_DW05_1_FLD_RX_BIPN_47_32_, (reg32), (val))

#define PN_SN_DW06_0_SET_TID7_SN(reg32, val)                   REG_FLD_SET(PN_SN_DW06_0_FLD_TID7_SN, (reg32), (val))
#define PN_SN_DW06_0_SET_TID6_SN(reg32, val)                   REG_FLD_SET(PN_SN_DW06_0_FLD_TID6_SN, (reg32), (val))
#define PN_SN_DW06_0_SET_TID5_SN_11_4_(reg32, val)             REG_FLD_SET(PN_SN_DW06_0_FLD_TID5_SN_11_4_, (reg32), (val))

#define PN_SN_DW06_1_SET_KEY_LOC2(reg32, val)                  REG_FLD_SET(PN_SN_DW06_1_FLD_KEY_LOC2, (reg32), (val))

#define KEY_INFO_DW07_SET_KEY_LOC1(reg32, val)                 REG_FLD_SET(KEY_INFO_DW07_FLD_KEY_LOC1, (reg32), (val))
#define KEY_INFO_DW07_SET_KEY_LOC0(reg32, val)                 REG_FLD_SET(KEY_INFO_DW07_FLD_KEY_LOC0, (reg32), (val))

#define MSDU_INFO_DW08_SET_BYPASS_RRO(reg32, val)              REG_FLD_SET(MSDU_INFO_DW08_FLD_BYPASS_RRO, (reg32), (val))
#define MSDU_INFO_DW08_SET_HDRT_MODE(reg32, val)               REG_FLD_SET(MSDU_INFO_DW08_FLD_HDRT_MODE, (reg32), (val))
#define MSDU_INFO_DW08_SET_HT(reg32, val)                      REG_FLD_SET(MSDU_INFO_DW08_FLD_HT, (reg32), (val))
#define MSDU_INFO_DW08_SET_QOS(reg32, val)                     REG_FLD_SET(MSDU_INFO_DW08_FLD_QOS, (reg32), (val))
#define MSDU_INFO_DW08_SET_WMM_Q(reg32, val)                   REG_FLD_SET(MSDU_INFO_DW08_FLD_WMM_Q, (reg32), (val))
#define MSDU_INFO_DW08_SET_AAD_OM(reg32, val)                  REG_FLD_SET(MSDU_INFO_DW08_FLD_AAD_OM, (reg32), (val))
#define MSDU_INFO_DW08_SET_WPI_EVEN(reg32, val)                REG_FLD_SET(MSDU_INFO_DW08_FLD_WPI_EVEN, (reg32), (val))
#define MSDU_INFO_DW08_SET_SPP_EN(reg32, val)                  REG_FLD_SET(MSDU_INFO_DW08_FLD_SPP_EN, (reg32), (val))
#define MSDU_INFO_DW08_SET_SEC_ADDR_MODE(reg32, val)           REG_FLD_SET(MSDU_INFO_DW08_FLD_SEC_ADDR_MODE, (reg32), (val))
#define MSDU_INFO_DW08_SET_AMSDU_CFG(reg32, val)               REG_FLD_SET(MSDU_INFO_DW08_FLD_AMSDU_CFG, (reg32), (val))

#define MLO_INFO_DW09_SET_SECONDARY_MLD_BAND(reg32, val)       REG_FLD_SET(MLO_INFO_DW09_FLD_SECONDARY_MLD_BAND, (reg32), (val))
#define MLO_INFO_DW09_SET_RELATED_BAND1(reg32, val)            REG_FLD_SET(MLO_INFO_DW09_FLD_RELATED_BAND1, (reg32), (val))
#define MLO_INFO_DW09_SET_RELATED_IDX1(reg32, val)             REG_FLD_SET(MLO_INFO_DW09_FLD_RELATED_IDX1, (reg32), (val))
#define MLO_INFO_DW09_SET_PRIMARY_MLD_BAND(reg32, val)         REG_FLD_SET(MLO_INFO_DW09_FLD_PRIMARY_MLD_BAND, (reg32), (val))
#define MLO_INFO_DW09_SET_RELATED_BAND0(reg32, val)            REG_FLD_SET(MLO_INFO_DW09_FLD_RELATED_BAND0, (reg32), (val))
#define MLO_INFO_DW09_SET_RELATED_IDX0(reg32, val)             REG_FLD_SET(MLO_INFO_DW09_FLD_RELATED_IDX0, (reg32), (val))

#define MSDU_COUNTER_DW10_SET_MSDU_RX_COUNT(reg32, val)        REG_FLD_SET(MSDU_COUNTER_DW10_FLD_MSDU_RX_COUNT, (reg32), (val))

#define MSDU_COUNTER_DW11_SET_MSDU_TX_COUNT(reg32, val)        REG_FLD_SET(MSDU_COUNTER_DW11_FLD_MSDU_TX_COUNT, (reg32), (val))

#define MSDU_COUNTER_DW12_SET_MSDU_TX_RETRY_CONT(reg32, val)   REG_FLD_SET(MSDU_COUNTER_DW12_FLD_MSDU_TX_RETRY_CONT, (reg32), (val))

#define MSDU_COUNTER_DW13_SET_MSDU_TX_DROP_COUNT(reg32, val)   REG_FLD_SET(MSDU_COUNTER_DW13_FLD_MSDU_TX_DROP_COUNT, (reg32), (val))

#define MSDU_COUNTER_DW14_SET_MSDU_TX_HAD_RETRIED_COUNT(reg32, val) REG_FLD_SET(MSDU_COUNTER_DW14_FLD_MSDU_TX_HAD_RETRIED_COUNT, (reg32), (val))

#define MSDU_COUNTER_DW15_SET_MSDU_TX_RETRY_LARGE_THAN_ONE_COUNT(reg32, val) REG_FLD_SET(MSDU_COUNTER_DW15_FLD_MSDU_TX_RETRY_LARGE_THAN_ONE_COUNT, (reg32), (val))

#define VERSION_CODE_SET_VERSION_CODE(reg32, val)              REG_FLD_SET(VERSION_CODE_FLD_VERSION_CODE, (reg32), (val))

#define MLO_INFO_DW00_VAL_OWN_MLD_ID(val)                      REG_FLD_VAL(MLO_INFO_DW00_FLD_OWN_MLD_ID, (val))
#define MLO_INFO_DW00_VAL_PEER_MLD_ADDRESS_47_32_(val)         REG_FLD_VAL(MLO_INFO_DW00_FLD_PEER_MLD_ADDRESS_47_32_, (val))

#define MLO_INFO_DW01_VAL_PEER_MLD_ADDRESS_31_0_(val)          REG_FLD_VAL(MLO_INFO_DW01_FLD_PEER_MLD_ADDRESS_31_0_, (val))

#define PN_SN_DW02_VAL_PN_31_0_(val)                           REG_FLD_VAL(PN_SN_DW02_FLD_PN_31_0_, (val))

#define PN_SN_DW03_VAL_COM_SN(val)                             REG_FLD_VAL(PN_SN_DW03_FLD_COM_SN, (val))
#define PN_SN_DW03_VAL_PN_47_32_(val)                          REG_FLD_VAL(PN_SN_DW03_FLD_PN_47_32_, (val))

#define PN_SN_DW04_0_VAL_TID2_SN_7_0_(val)                     REG_FLD_VAL(PN_SN_DW04_0_FLD_TID2_SN_7_0_, (val))
#define PN_SN_DW04_0_VAL_TID1_SN(val)                          REG_FLD_VAL(PN_SN_DW04_0_FLD_TID1_SN, (val))
#define PN_SN_DW04_0_VAL_TID0_SN(val)                          REG_FLD_VAL(PN_SN_DW04_0_FLD_TID0_SN, (val))

#define PN_SN_DW04_1_VAL_RX_BIPN_31_0_(val)                    REG_FLD_VAL(PN_SN_DW04_1_FLD_RX_BIPN_31_0_, (val))

#define PN_SN_DW05_0_VAL_TID5_SN_3_0_(val)                     REG_FLD_VAL(PN_SN_DW05_0_FLD_TID5_SN_3_0_, (val))
#define PN_SN_DW05_0_VAL_TID4_SN(val)                          REG_FLD_VAL(PN_SN_DW05_0_FLD_TID4_SN, (val))
#define PN_SN_DW05_0_VAL_TID3_SN(val)                          REG_FLD_VAL(PN_SN_DW05_0_FLD_TID3_SN, (val))
#define PN_SN_DW05_0_VAL_TID2_SN_11_8_(val)                    REG_FLD_VAL(PN_SN_DW05_0_FLD_TID2_SN_11_8_, (val))

#define PN_SN_DW05_1_VAL_RX_BIPN_47_32_(val)                   REG_FLD_VAL(PN_SN_DW05_1_FLD_RX_BIPN_47_32_, (val))

#define PN_SN_DW06_0_VAL_TID7_SN(val)                          REG_FLD_VAL(PN_SN_DW06_0_FLD_TID7_SN, (val))
#define PN_SN_DW06_0_VAL_TID6_SN(val)                          REG_FLD_VAL(PN_SN_DW06_0_FLD_TID6_SN, (val))
#define PN_SN_DW06_0_VAL_TID5_SN_11_4_(val)                    REG_FLD_VAL(PN_SN_DW06_0_FLD_TID5_SN_11_4_, (val))

#define PN_SN_DW06_1_VAL_KEY_LOC2(val)                         REG_FLD_VAL(PN_SN_DW06_1_FLD_KEY_LOC2, (val))

#define KEY_INFO_DW07_VAL_KEY_LOC1(val)                        REG_FLD_VAL(KEY_INFO_DW07_FLD_KEY_LOC1, (val))
#define KEY_INFO_DW07_VAL_KEY_LOC0(val)                        REG_FLD_VAL(KEY_INFO_DW07_FLD_KEY_LOC0, (val))

#define MSDU_INFO_DW08_VAL_BYPASS_RRO(val)                     REG_FLD_VAL(MSDU_INFO_DW08_FLD_BYPASS_RRO, (val))
#define MSDU_INFO_DW08_VAL_HDRT_MODE(val)                      REG_FLD_VAL(MSDU_INFO_DW08_FLD_HDRT_MODE, (val))
#define MSDU_INFO_DW08_VAL_HT(val)                             REG_FLD_VAL(MSDU_INFO_DW08_FLD_HT, (val))
#define MSDU_INFO_DW08_VAL_QOS(val)                            REG_FLD_VAL(MSDU_INFO_DW08_FLD_QOS, (val))
#define MSDU_INFO_DW08_VAL_WMM_Q(val)                          REG_FLD_VAL(MSDU_INFO_DW08_FLD_WMM_Q, (val))
#define MSDU_INFO_DW08_VAL_AAD_OM(val)                         REG_FLD_VAL(MSDU_INFO_DW08_FLD_AAD_OM, (val))
#define MSDU_INFO_DW08_VAL_WPI_EVEN(val)                       REG_FLD_VAL(MSDU_INFO_DW08_FLD_WPI_EVEN, (val))
#define MSDU_INFO_DW08_VAL_SPP_EN(val)                         REG_FLD_VAL(MSDU_INFO_DW08_FLD_SPP_EN, (val))
#define MSDU_INFO_DW08_VAL_SEC_ADDR_MODE(val)                  REG_FLD_VAL(MSDU_INFO_DW08_FLD_SEC_ADDR_MODE, (val))
#define MSDU_INFO_DW08_VAL_AMSDU_CFG(val)                      REG_FLD_VAL(MSDU_INFO_DW08_FLD_AMSDU_CFG, (val))

#define MLO_INFO_DW09_VAL_SECONDARY_MLD_BAND(val)              REG_FLD_VAL(MLO_INFO_DW09_FLD_SECONDARY_MLD_BAND, (val))
#define MLO_INFO_DW09_VAL_RELATED_BAND1(val)                   REG_FLD_VAL(MLO_INFO_DW09_FLD_RELATED_BAND1, (val))
#define MLO_INFO_DW09_VAL_RELATED_IDX1(val)                    REG_FLD_VAL(MLO_INFO_DW09_FLD_RELATED_IDX1, (val))
#define MLO_INFO_DW09_VAL_PRIMARY_MLD_BAND(val)                REG_FLD_VAL(MLO_INFO_DW09_FLD_PRIMARY_MLD_BAND, (val))
#define MLO_INFO_DW09_VAL_RELATED_BAND0(val)                   REG_FLD_VAL(MLO_INFO_DW09_FLD_RELATED_BAND0, (val))
#define MLO_INFO_DW09_VAL_RELATED_IDX0(val)                    REG_FLD_VAL(MLO_INFO_DW09_FLD_RELATED_IDX0, (val))

#define MSDU_COUNTER_DW10_VAL_MSDU_RX_COUNT(val)               REG_FLD_VAL(MSDU_COUNTER_DW10_FLD_MSDU_RX_COUNT, (val))

#define MSDU_COUNTER_DW11_VAL_MSDU_TX_COUNT(val)               REG_FLD_VAL(MSDU_COUNTER_DW11_FLD_MSDU_TX_COUNT, (val))

#define MSDU_COUNTER_DW12_VAL_MSDU_TX_RETRY_CONT(val)          REG_FLD_VAL(MSDU_COUNTER_DW12_FLD_MSDU_TX_RETRY_CONT, (val))

#define MSDU_COUNTER_DW13_VAL_MSDU_TX_DROP_COUNT(val)          REG_FLD_VAL(MSDU_COUNTER_DW13_FLD_MSDU_TX_DROP_COUNT, (val))

#define MSDU_COUNTER_DW14_VAL_MSDU_TX_HAD_RETRIED_COUNT(val)   REG_FLD_VAL(MSDU_COUNTER_DW14_FLD_MSDU_TX_HAD_RETRIED_COUNT, (val))

#define MSDU_COUNTER_DW15_VAL_MSDU_TX_RETRY_LARGE_THAN_ONE_COUNT(val) REG_FLD_VAL(MSDU_COUNTER_DW15_FLD_MSDU_TX_RETRY_LARGE_THAN_ONE_COUNT, (val))

#define VERSION_CODE_VAL_VERSION_CODE(val)                     REG_FLD_VAL(VERSION_CODE_FLD_VERSION_CODE, (val))

#ifdef __cplusplus
}
#endif

#endif // __WF_DS_UWTBL_REGS_H__
