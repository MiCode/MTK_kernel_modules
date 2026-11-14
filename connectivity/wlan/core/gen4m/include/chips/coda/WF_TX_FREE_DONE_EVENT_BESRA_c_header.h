/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef __WF_TX_FREE_DONE_EVENT_REGS_H__
#define __WF_TX_FREE_DONE_EVENT_REGS_H__

static __KAL_INLINE__ uint32_t _READ_FIELD(uint32_t reg,
	uint32_t mask, uint32_t shift)
{
	return (reg & mask) >> shift;
}

#define READ_FIELD(_reg, _field)	\
	_READ_FIELD(_reg, _field##_MASK, _field##_SHIFT)

// DW0
#define WF_TX_FREE_DONE_EVENT_RX_BYTE_COUNT_DW                                   0
#define WF_TX_FREE_DONE_EVENT_RX_BYTE_COUNT_ADDR                                 0
#define WF_TX_FREE_DONE_EVENT_RX_BYTE_COUNT_MASK                                 0x0000ffff // 15- 0
#define WF_TX_FREE_DONE_EVENT_RX_BYTE_COUNT_SHIFT                                0
#define WF_TX_FREE_DONE_EVENT_MSDU_ID_COUNT_DW                                   0
#define WF_TX_FREE_DONE_EVENT_MSDU_ID_COUNT_ADDR                                 0
#define WF_TX_FREE_DONE_EVENT_MSDU_ID_COUNT_MASK                                 0x03ff0000 // 25-16
#define WF_TX_FREE_DONE_EVENT_MSDU_ID_COUNT_SHIFT                                16
#define WF_TX_FREE_DONE_EVENT_PKT_TYPE_DW                                        0
#define WF_TX_FREE_DONE_EVENT_PKT_TYPE_ADDR                                      0
#define WF_TX_FREE_DONE_EVENT_PKT_TYPE_MASK                                      0xf8000000 // 31-27
#define WF_TX_FREE_DONE_EVENT_PKT_TYPE_SHIFT                                     27
// DW1
#define WF_TX_FREE_DONE_EVENT_TXD_COUNT_DW                                       1
#define WF_TX_FREE_DONE_EVENT_TXD_COUNT_ADDR                                     4
#define WF_TX_FREE_DONE_EVENT_TXD_COUNT_MASK                                     0x000000ff //  7- 0
#define WF_TX_FREE_DONE_EVENT_TXD_COUNT_SHIFT                                    0
#define WF_TX_FREE_DONE_EVENT_SERIAL_ID_DW                                       1
#define WF_TX_FREE_DONE_EVENT_SERIAL_ID_ADDR                                     4
#define WF_TX_FREE_DONE_EVENT_SERIAL_ID_MASK                                     0x0000ff00 // 15- 8
#define WF_TX_FREE_DONE_EVENT_SERIAL_ID_SHIFT                                    8
#define WF_TX_FREE_DONE_EVENT_VER_DW                                             1
#define WF_TX_FREE_DONE_EVENT_VER_ADDR                                           4
#define WF_TX_FREE_DONE_EVENT_VER_MASK                                           0x00070000 // 18-16
#define WF_TX_FREE_DONE_EVENT_VER_SHIFT                                          16
#define WF_TX_FREE_DONE_EVENT_PSE_FID_DW                                         1
#define WF_TX_FREE_DONE_EVENT_PSE_FID_ADDR                                       4
#define WF_TX_FREE_DONE_EVENT_PSE_FID_MASK                                       0xfff00000 // 31-20
#define WF_TX_FREE_DONE_EVENT_PSE_FID_SHIFT                                      20
// DW2
#define WF_TX_FREE_DONE_EVENT_WLAN_ID_DW                                         2
#define WF_TX_FREE_DONE_EVENT_WLAN_ID_ADDR                                       8
#define WF_TX_FREE_DONE_EVENT_WLAN_ID_MASK                                       0x00fff000 // 23-12
#define WF_TX_FREE_DONE_EVENT_WLAN_ID_SHIFT                                      12
#define WF_TX_FREE_DONE_EVENT_QID_DW                                             2
#define WF_TX_FREE_DONE_EVENT_QID_ADDR                                           8
#define WF_TX_FREE_DONE_EVENT_QID_MASK                                           0x7f000000 // 30-24
#define WF_TX_FREE_DONE_EVENT_QID_SHIFT                                          24
#define WF_TX_FREE_DONE_EVENT_P2_DW                                              2
#define WF_TX_FREE_DONE_EVENT_P2_ADDR                                            8
#define WF_TX_FREE_DONE_EVENT_P2_MASK                                            0x80000000 // 31-31
#define WF_TX_FREE_DONE_EVENT_P2_SHIFT                                           31
// DW3
#define WF_TX_FREE_DONE_EVENT_TRANSMIT_DELAY_DW                                  3
#define WF_TX_FREE_DONE_EVENT_TRANSMIT_DELAY_ADDR                                12
#define WF_TX_FREE_DONE_EVENT_TRANSMIT_DELAY_MASK                                0x00000fff // 11- 0
#define WF_TX_FREE_DONE_EVENT_TRANSMIT_DELAY_SHIFT                               0
#define WF_TX_FREE_DONE_EVENT_AIR_DELAY_DW                                       3
#define WF_TX_FREE_DONE_EVENT_AIR_DELAY_ADDR                                     12
#define WF_TX_FREE_DONE_EVENT_AIR_DELAY_MASK                                     0x00fff000 // 23-12
#define WF_TX_FREE_DONE_EVENT_AIR_DELAY_SHIFT                                    12
#define WF_TX_FREE_DONE_EVENT_TX_COUNT_DW                                        3
#define WF_TX_FREE_DONE_EVENT_TX_COUNT_ADDR                                      12
#define WF_TX_FREE_DONE_EVENT_TX_COUNT_MASK                                      0x0f000000 // 27-24
#define WF_TX_FREE_DONE_EVENT_TX_COUNT_SHIFT                                     24
#define WF_TX_FREE_DONE_EVENT_STAT_DW                                            3
#define WF_TX_FREE_DONE_EVENT_STAT_ADDR                                          12
#define WF_TX_FREE_DONE_EVENT_STAT_MASK                                          0x30000000 // 29-28
#define WF_TX_FREE_DONE_EVENT_STAT_SHIFT                                         28
#define WF_TX_FREE_DONE_EVENT_H3_DW                                              3
#define WF_TX_FREE_DONE_EVENT_H3_ADDR                                            12
#define WF_TX_FREE_DONE_EVENT_H3_MASK                                            0x40000000 // 30-30
#define WF_TX_FREE_DONE_EVENT_H3_SHIFT                                           30
#define WF_TX_FREE_DONE_EVENT_P3_DW                                              3
#define WF_TX_FREE_DONE_EVENT_P3_ADDR                                            12
#define WF_TX_FREE_DONE_EVENT_P3_MASK                                            0x80000000 // 31-31
#define WF_TX_FREE_DONE_EVENT_P3_SHIFT                                           31
// DW4
#define WF_TX_FREE_DONE_EVENT_MSDU_ID0_DW                                        4
#define WF_TX_FREE_DONE_EVENT_MSDU_ID0_ADDR                                      16
#define WF_TX_FREE_DONE_EVENT_MSDU_ID0_MASK                                      0x00007fff // 14- 0
#define WF_TX_FREE_DONE_EVENT_MSDU_ID0_SHIFT                                     0
#define WF_TX_FREE_DONE_EVENT_MSDU_ID1_DW                                        4
#define WF_TX_FREE_DONE_EVENT_MSDU_ID1_ADDR                                      16
#define WF_TX_FREE_DONE_EVENT_MSDU_ID1_MASK                                      0x3fff8000 // 29-15
#define WF_TX_FREE_DONE_EVENT_MSDU_ID1_SHIFT                                     15
#define WF_TX_FREE_DONE_EVENT_H4_DW                                              4
#define WF_TX_FREE_DONE_EVENT_H4_ADDR                                            16
#define WF_TX_FREE_DONE_EVENT_H4_MASK                                            0x40000000 // 30-30
#define WF_TX_FREE_DONE_EVENT_H4_SHIFT                                           30
#define WF_TX_FREE_DONE_EVENT_P4_DW                                              4
#define WF_TX_FREE_DONE_EVENT_P4_ADDR                                            16
#define WF_TX_FREE_DONE_EVENT_P4_MASK                                            0x80000000 // 31-31
#define WF_TX_FREE_DONE_EVENT_P4_SHIFT                                           31

// DW0
#define HAL_TX_FREE_DONE_GET_RX_BYTE_COUNT(reg32)                   READ_FIELD(reg32, WF_TX_FREE_DONE_EVENT_RX_BYTE_COUNT)
#define HAL_TX_FREE_DONE_GET_MSDU_ID_COUNT(reg32)                   READ_FIELD(reg32, WF_TX_FREE_DONE_EVENT_MSDU_ID_COUNT)
#define HAL_TX_FREE_DONE_GET_PKT_TYPE(reg32)                        READ_FIELD(reg32, WF_TX_FREE_DONE_EVENT_PKT_TYPE)
// DW1
#define HAL_TX_FREE_DONE_GET_TXD_COUNT(reg32)                       READ_FIELD(reg32, WF_TX_FREE_DONE_EVENT_TXD_COUNT)
#define HAL_TX_FREE_DONE_GET_SERIAL_ID(reg32)                       READ_FIELD(reg32, WF_TX_FREE_DONE_EVENT_SERIAL_ID)
#define HAL_TX_FREE_DONE_GET_VER(reg32)                             READ_FIELD(reg32, WF_TX_FREE_DONE_EVENT_VER)
#define HAL_TX_FREE_DONE_GET_PSE_FID(reg32)                         READ_FIELD(reg32, WF_TX_FREE_DONE_EVENT_PSE_FID)
// DW2
#define HAL_TX_FREE_DONE_GET_WLAN_ID(reg32)                         READ_FIELD(reg32, WF_TX_FREE_DONE_EVENT_WLAN_ID)
#define HAL_TX_FREE_DONE_GET_QID(reg32)                             READ_FIELD(reg32, WF_TX_FREE_DONE_EVENT_QID)
#define HAL_TX_FREE_DONE_GET_P2(reg32)                              READ_FIELD(reg32, WF_TX_FREE_DONE_EVENT_P2)
// DW3
#define HAL_TX_FREE_DONE_GET_TRANSMIT_DELAY(reg32)                  READ_FIELD(reg32, WF_TX_FREE_DONE_EVENT_TRANSMIT_DELAY)
#define HAL_TX_FREE_DONE_GET_AIR_DELAY(reg32)                       READ_FIELD(reg32, WF_TX_FREE_DONE_EVENT_AIR_DELAY)
#define HAL_TX_FREE_DONE_GET_TX_COUNT(reg32)                        READ_FIELD(reg32, WF_TX_FREE_DONE_EVENT_TX_COUNT)
#define HAL_TX_FREE_DONE_GET_STAT(reg32)                            READ_FIELD(reg32, WF_TX_FREE_DONE_EVENT_STAT)
#define HAL_TX_FREE_DONE_GET_H3(reg32)                              READ_FIELD(reg32, WF_TX_FREE_DONE_EVENT_H3)
#define HAL_TX_FREE_DONE_GET_P3(reg32)                              READ_FIELD(reg32, WF_TX_FREE_DONE_EVENT_P3)
// DW4
#define HAL_TX_FREE_DONE_GET_MSDU_ID0(reg32)                        READ_FIELD(reg32, WF_TX_FREE_DONE_EVENT_MSDU_ID0)
#define HAL_TX_FREE_DONE_GET_MSDU_ID1(reg32)                        READ_FIELD(reg32, WF_TX_FREE_DONE_EVENT_MSDU_ID1)
#define HAL_TX_FREE_DONE_GET_H4(reg32)                              READ_FIELD(reg32, WF_TX_FREE_DONE_EVENT_H4)
#define HAL_TX_FREE_DONE_GET_P4(reg32)                              READ_FIELD(reg32, WF_TX_FREE_DONE_EVENT_P4)

/* v6 DW3 */
#define WF_TX_FREE_DONE_EVENT_RTS_TX_CNT_MASK         0x0000001f /* 4-0 */
#define WF_TX_FREE_DONE_EVENT_RTS_TX_CNT_SHIFT        0
#define WF_TX_FREE_DONE_EVENT_TID_MASK                0x0000f000 /* 15-12 */
#define WF_TX_FREE_DONE_EVENT_TID_SHIFT               12
#define WF_TX_FREE_DONE_EVENT_RLS_PERIOD_MASK         0x0fff0000 /* 27-16 */
#define WF_TX_FREE_DONE_EVENT_RLS_PERIOD_SHIFT        16
#define WF_TX_FREE_DONE_EVENT_TX_BN_MASK              0x30000000 /* 29-28 */
#define WF_TX_FREE_DONE_EVENT_TX_BN_SHIFT             28

#define HAL_TX_FREE_DONE_GET_RTS_TX_CNT(reg32) \
	READ_FIELD(reg32, WF_TX_FREE_DONE_EVENT_RTS_TX_CNT)
#define HAL_TX_FREE_DONE_GET_TID(reg32) \
	READ_FIELD(reg32, WF_TX_FREE_DONE_EVENT_TID)
#define HAL_TX_FREE_DONE_GET_RLS_PERIOD(reg32) \
	READ_FIELD(reg32, WF_TX_FREE_DONE_EVENT_RLS_PERIOD)
#define HAL_TX_FREE_DONE_GET_TX_BN(reg32) \
	READ_FIELD(reg32, WF_TX_FREE_DONE_EVENT_TX_BN)

/* v7 DW2 */
#define WF_TX_FREE_DONE_EVENT_TID_V7_MASK                0x00000f00 /* 11-8 */
#define WF_TX_FREE_DONE_EVENT_TID_V7_SHIFT               8
/* v7 DW3 */
#define WF_TX_FREE_DONE_EVENT_DROP_REASON_MASK           0x0000001f /* 4-0 */
#define WF_TX_FREE_DONE_EVENT_DROP_REASON_SHIFT          0

#define HAL_TX_FREE_DONE_GET_DROP_REASON(reg32)			\
	READ_FIELD(reg32, WF_TX_FREE_DONE_EVENT_DROP_REASON)
#define HAL_TX_FREE_DONE_GET_TID_V7(reg32) \
	READ_FIELD(reg32, WF_TX_FREE_DONE_EVENT_TID_V7)

#endif // __WF_TX_FREE_DONE_EVENT_REGS_H__
