/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#ifndef _MTK_CAM_QOF_REGS_H
#define _MTK_CAM_QOF_REGS_H

/* baseaddr 0x3A0A0000 */

/* module: QOF_CAM_TOP_E1A */
#define REG_QOF_CAM_TOP_QOF_TOP_CTL                   0x0
#define F_QOF_CAM_TOP_OUT_LOCK_3_POS                                 29
#define F_QOF_CAM_TOP_OUT_LOCK_3_WIDTH                               1
#define F_QOF_CAM_TOP_OFF_LOCK_3_POS                                 28
#define F_QOF_CAM_TOP_OFF_LOCK_3_WIDTH                               1
#define F_QOF_CAM_TOP_ON_LOCK_3_POS                                  27
#define F_QOF_CAM_TOP_ON_LOCK_3_WIDTH                                1
#define F_QOF_CAM_TOP_OUT_LOCK_2_POS                                 26
#define F_QOF_CAM_TOP_OUT_LOCK_2_WIDTH                               1
#define F_QOF_CAM_TOP_OFF_LOCK_2_POS                                 25
#define F_QOF_CAM_TOP_OFF_LOCK_2_WIDTH                               1
#define F_QOF_CAM_TOP_ON_LOCK_2_POS                                  24
#define F_QOF_CAM_TOP_ON_LOCK_2_WIDTH                                1
#define F_QOF_CAM_TOP_OUT_LOCK_1_POS                                 23
#define F_QOF_CAM_TOP_OUT_LOCK_1_WIDTH                               1
#define F_QOF_CAM_TOP_OFF_LOCK_1_POS                                 22
#define F_QOF_CAM_TOP_OFF_LOCK_1_WIDTH                               1
#define F_QOF_CAM_TOP_ON_LOCK_1_POS                                  21
#define F_QOF_CAM_TOP_ON_LOCK_1_WIDTH                                1
#define F_QOF_CAM_TOP_DCIF_EXP_SOF_SEL_3_POS                         19
#define F_QOF_CAM_TOP_DCIF_EXP_SOF_SEL_3_WIDTH                       2
#define F_QOF_CAM_TOP_DCIF_EXP_SOF_SEL_2_POS                         17
#define F_QOF_CAM_TOP_DCIF_EXP_SOF_SEL_2_WIDTH                       2
#define F_QOF_CAM_TOP_DCIF_EXP_SOF_SEL_1_POS                         15
#define F_QOF_CAM_TOP_DCIF_EXP_SOF_SEL_1_WIDTH                       2
#define F_QOF_CAM_TOP_OTF_DC_MODE_3_POS                              13
#define F_QOF_CAM_TOP_OTF_DC_MODE_3_WIDTH                            2
#define F_QOF_CAM_TOP_OTF_DC_MODE_2_POS                              11
#define F_QOF_CAM_TOP_OTF_DC_MODE_2_WIDTH                            2
#define F_QOF_CAM_TOP_OTF_DC_MODE_1_POS                              9
#define F_QOF_CAM_TOP_OTF_DC_MODE_1_WIDTH                            2
#define F_QOF_CAM_TOP_ITC_SRC_SEL_3_POS                              8
#define F_QOF_CAM_TOP_ITC_SRC_SEL_3_WIDTH                            1
#define F_QOF_CAM_TOP_ITC_SRC_SEL_2_POS                              7
#define F_QOF_CAM_TOP_ITC_SRC_SEL_2_WIDTH                            1
#define F_QOF_CAM_TOP_ITC_SRC_SEL_1_POS                              6
#define F_QOF_CAM_TOP_ITC_SRC_SEL_1_WIDTH                            1
#define F_QOF_CAM_TOP_QOF_SUBC_EN_POS                                5
#define F_QOF_CAM_TOP_QOF_SUBC_EN_WIDTH                              1
#define F_QOF_CAM_TOP_QOF_SUBB_EN_POS                                4
#define F_QOF_CAM_TOP_QOF_SUBB_EN_WIDTH                              1
#define F_QOF_CAM_TOP_QOF_SUBA_EN_POS                                3
#define F_QOF_CAM_TOP_QOF_SUBA_EN_WIDTH                              1
#define F_QOF_CAM_TOP_SEQUENCE_MODE_POS                              2
#define F_QOF_CAM_TOP_SEQUENCE_MODE_WIDTH                            1

#define REG_QOF_CAM_TOP_ITC_STATUS                    0x4
#define REG_QOF_CAM_TOP_QOF_INT_EN                    0x8
#define F_QOF_CAM_TOP_INT_WCLR_EN_POS                                31
#define F_QOF_CAM_TOP_INT_WCLR_EN_WIDTH                              1
#define F_QOF_CAM_TOP_MTC_PROC_OV_TIME_INT_EN_3_POS                  11
#define F_QOF_CAM_TOP_MTC_PROC_OV_TIME_INT_EN_3_WIDTH                1
#define F_QOF_CAM_TOP_MTC_PROC_OV_TIME_INT_EN_2_POS                  10
#define F_QOF_CAM_TOP_MTC_PROC_OV_TIME_INT_EN_2_WIDTH                1
#define F_QOF_CAM_TOP_MTC_PROC_OV_TIME_INT_EN_1_POS                  9
#define F_QOF_CAM_TOP_MTC_PROC_OV_TIME_INT_EN_1_WIDTH                1
#define F_QOF_CAM_TOP_CQ_TRIG_DLY_INT_EN_3_POS                       8
#define F_QOF_CAM_TOP_CQ_TRIG_DLY_INT_EN_3_WIDTH                     1
#define F_QOF_CAM_TOP_CQ_TRIG_DLY_INT_EN_2_POS                       7
#define F_QOF_CAM_TOP_CQ_TRIG_DLY_INT_EN_2_WIDTH                     1
#define F_QOF_CAM_TOP_CQ_TRIG_DLY_INT_EN_1_POS                       6
#define F_QOF_CAM_TOP_CQ_TRIG_DLY_INT_EN_1_WIDTH                     1
#define F_QOF_CAM_TOP_PWR_OFF_ACCESS_INT_EN_3_POS                    5
#define F_QOF_CAM_TOP_PWR_OFF_ACCESS_INT_EN_3_WIDTH                  1
#define F_QOF_CAM_TOP_PWR_OFF_ACCESS_INT_EN_2_POS                    4
#define F_QOF_CAM_TOP_PWR_OFF_ACCESS_INT_EN_2_WIDTH                  1
#define F_QOF_CAM_TOP_PWR_OFF_ACCESS_INT_EN_1_POS                    3
#define F_QOF_CAM_TOP_PWR_OFF_ACCESS_INT_EN_1_WIDTH                  1
#define F_QOF_CAM_TOP_MTC_CYC_OV_INT_EN_3_POS                        2
#define F_QOF_CAM_TOP_MTC_CYC_OV_INT_EN_3_WIDTH                      1
#define F_QOF_CAM_TOP_MTC_CYC_OV_INT_EN_2_POS                        1
#define F_QOF_CAM_TOP_MTC_CYC_OV_INT_EN_2_WIDTH                      1
#define F_QOF_CAM_TOP_MTC_CYC_OV_INT_EN_1_POS                        0
#define F_QOF_CAM_TOP_MTC_CYC_OV_INT_EN_1_WIDTH                      1

#define REG_QOF_CAM_TOP_QOF_INT_STATUS                0xC
#define F_QOF_CAM_TOP_MTC_PROC_OV_TIME_INT_ST_3_POS                  11
#define F_QOF_CAM_TOP_MTC_PROC_OV_TIME_INT_ST_3_WIDTH                1
#define F_QOF_CAM_TOP_MTC_PROC_OV_TIME_INT_ST_2_POS                  10
#define F_QOF_CAM_TOP_MTC_PROC_OV_TIME_INT_ST_2_WIDTH                1
#define F_QOF_CAM_TOP_MTC_PROC_OV_TIME_INT_ST_1_POS                  9
#define F_QOF_CAM_TOP_MTC_PROC_OV_TIME_INT_ST_1_WIDTH                1
#define F_QOF_CAM_TOP_CQ_TRIG_DLY_INT_ST_3_POS                       8
#define F_QOF_CAM_TOP_CQ_TRIG_DLY_INT_ST_3_WIDTH                     1
#define F_QOF_CAM_TOP_CQ_TRIG_DLY_INT_ST_2_POS                       7
#define F_QOF_CAM_TOP_CQ_TRIG_DLY_INT_ST_2_WIDTH                     1
#define F_QOF_CAM_TOP_CQ_TRIG_DLY_INT_ST_1_POS                       6
#define F_QOF_CAM_TOP_CQ_TRIG_DLY_INT_ST_1_WIDTH                     1
#define F_QOF_CAM_TOP_PWR_OFF_ACCESS_INT_ST_3_POS                    5
#define F_QOF_CAM_TOP_PWR_OFF_ACCESS_INT_ST_3_WIDTH                  1
#define F_QOF_CAM_TOP_PWR_OFF_ACCESS_INT_ST_2_POS                    4
#define F_QOF_CAM_TOP_PWR_OFF_ACCESS_INT_ST_2_WIDTH                  1
#define F_QOF_CAM_TOP_PWR_OFF_ACCESS_INT_ST_1_POS                    3
#define F_QOF_CAM_TOP_PWR_OFF_ACCESS_INT_ST_1_WIDTH                  1
#define F_QOF_CAM_TOP_MTC_CYC_OV_INT_ST_3_POS                        2
#define F_QOF_CAM_TOP_MTC_CYC_OV_INT_ST_3_WIDTH                      1
#define F_QOF_CAM_TOP_MTC_CYC_OV_INT_ST_2_POS                        1
#define F_QOF_CAM_TOP_MTC_CYC_OV_INT_ST_2_WIDTH                      1
#define F_QOF_CAM_TOP_MTC_CYC_OV_INT_ST_1_POS                        0
#define F_QOF_CAM_TOP_MTC_CYC_OV_INT_ST_1_WIDTH                      1

#define REG_QOF_CAM_TOP_QOF_INT_TRIG                 0x10
#define F_QOF_CAM_TOP_QOF_SELF_TRIG_EN_POS                           31
#define F_QOF_CAM_TOP_QOF_SELF_TRIG_EN_WIDTH                         1
#define F_QOF_CAM_TOP_MTC_PROC_OV_TIME_INT_TRIG_3_POS                11
#define F_QOF_CAM_TOP_MTC_PROC_OV_TIME_INT_TRIG_3_WIDTH              1
#define F_QOF_CAM_TOP_MTC_PROC_OV_TIME_INT_TRIG_2_POS                10
#define F_QOF_CAM_TOP_MTC_PROC_OV_TIME_INT_TRIG_2_WIDTH              1
#define F_QOF_CAM_TOP_MTC_PROC_OV_TIME_INT_TRIG_1_POS                9
#define F_QOF_CAM_TOP_MTC_PROC_OV_TIME_INT_TRIG_1_WIDTH              1
#define F_QOF_CAM_TOP_CQ_TRIG_DLY_INT_TRIG_3_POS                     8
#define F_QOF_CAM_TOP_CQ_TRIG_DLY_INT_TRIG_3_WIDTH                   1
#define F_QOF_CAM_TOP_CQ_TRIG_DLY_INT_TRIG_2_POS                     7
#define F_QOF_CAM_TOP_CQ_TRIG_DLY_INT_TRIG_2_WIDTH                   1
#define F_QOF_CAM_TOP_CQ_TRIG_DLY_INT_TRIG_1_POS                     6
#define F_QOF_CAM_TOP_CQ_TRIG_DLY_INT_TRIG_1_WIDTH                   1
#define F_QOF_CAM_TOP_PWR_OFF_ACCESS_INT_TRIG_3_POS                  5
#define F_QOF_CAM_TOP_PWR_OFF_ACCESS_INT_TRIG_3_WIDTH                1
#define F_QOF_CAM_TOP_PWR_OFF_ACCESS_INT_TRIG_2_POS                  4
#define F_QOF_CAM_TOP_PWR_OFF_ACCESS_INT_TRIG_2_WIDTH                1
#define F_QOF_CAM_TOP_PWR_OFF_ACCESS_INT_TRIG_1_POS                  3
#define F_QOF_CAM_TOP_PWR_OFF_ACCESS_INT_TRIG_1_WIDTH                1
#define F_QOF_CAM_TOP_MTC_CYC_OV_INT_TRIG_3_POS                      2
#define F_QOF_CAM_TOP_MTC_CYC_OV_INT_TRIG_3_WIDTH                    1
#define F_QOF_CAM_TOP_MTC_CYC_OV_INT_TRIG_2_POS                      1
#define F_QOF_CAM_TOP_MTC_CYC_OV_INT_TRIG_2_WIDTH                    1
#define F_QOF_CAM_TOP_MTC_CYC_OV_INT_TRIG_1_POS                      0
#define F_QOF_CAM_TOP_MTC_CYC_OV_INT_TRIG_1_WIDTH                    1

#define REG_QOF_CAM_TOP_QOF_SPARE1_TOP               0x14
#define REG_QOF_CAM_TOP_QOF_SPARE2_TOP               0x18
#define REG_QOF_CAM_TOP_QOF_EVENT_TRIG               0x1C
#define F_QOF_CAM_TOP_QOF_SELF_EVENT_TRIG_EN_POS                     31
#define F_QOF_CAM_TOP_QOF_SELF_EVENT_TRIG_EN_WIDTH                   1
#define F_QOF_CAM_TOP_RESTORE_EVENT_TRIG_3_POS                       23
#define F_QOF_CAM_TOP_RESTORE_EVENT_TRIG_3_WIDTH                     1
#define F_QOF_CAM_TOP_RESTORE_EVENT_TRIG_2_POS                       22
#define F_QOF_CAM_TOP_RESTORE_EVENT_TRIG_2_WIDTH                     1
#define F_QOF_CAM_TOP_RESTORE_EVENT_TRIG_1_POS                       21
#define F_QOF_CAM_TOP_RESTORE_EVENT_TRIG_1_WIDTH                     1
#define F_QOF_CAM_TOP_SAVE_EVENT_TRIG_3_POS                          20
#define F_QOF_CAM_TOP_SAVE_EVENT_TRIG_3_WIDTH                        1
#define F_QOF_CAM_TOP_SAVE_EVENT_TRIG_2_POS                          19
#define F_QOF_CAM_TOP_SAVE_EVENT_TRIG_2_WIDTH                        1
#define F_QOF_CAM_TOP_SAVE_EVENT_TRIG_1_POS                          18
#define F_QOF_CAM_TOP_SAVE_EVENT_TRIG_1_WIDTH                        1
#define F_QOF_CAM_TOP_PWR_OFF_EVENT_TRIG_3_POS                       17
#define F_QOF_CAM_TOP_PWR_OFF_EVENT_TRIG_3_WIDTH                     1
#define F_QOF_CAM_TOP_PWR_OFF_EVENT_TRIG_2_POS                       16
#define F_QOF_CAM_TOP_PWR_OFF_EVENT_TRIG_2_WIDTH                     1
#define F_QOF_CAM_TOP_PWR_OFF_EVENT_TRIG_1_POS                       15
#define F_QOF_CAM_TOP_PWR_OFF_EVENT_TRIG_1_WIDTH                     1
#define F_QOF_CAM_TOP_PWR_ON_EVENT_TRIG_3_POS                        14
#define F_QOF_CAM_TOP_PWR_ON_EVENT_TRIG_3_WIDTH                      1
#define F_QOF_CAM_TOP_PWR_ON_EVENT_TRIG_2_POS                        13
#define F_QOF_CAM_TOP_PWR_ON_EVENT_TRIG_2_WIDTH                      1
#define F_QOF_CAM_TOP_PWR_ON_EVENT_TRIG_1_POS                        12
#define F_QOF_CAM_TOP_PWR_ON_EVENT_TRIG_1_WIDTH                      1
#define F_QOF_CAM_TOP_ACK_EVENT_TRIG_12_POS                          11
#define F_QOF_CAM_TOP_ACK_EVENT_TRIG_12_WIDTH                        1
#define F_QOF_CAM_TOP_ACK_EVENT_TRIG_11_POS                          10
#define F_QOF_CAM_TOP_ACK_EVENT_TRIG_11_WIDTH                        1
#define F_QOF_CAM_TOP_ACK_EVENT_TRIG_10_POS                          9
#define F_QOF_CAM_TOP_ACK_EVENT_TRIG_10_WIDTH                        1
#define F_QOF_CAM_TOP_ACK_EVENT_TRIG_9_POS                           8
#define F_QOF_CAM_TOP_ACK_EVENT_TRIG_9_WIDTH                         1
#define F_QOF_CAM_TOP_ACK_EVENT_TRIG_8_POS                           7
#define F_QOF_CAM_TOP_ACK_EVENT_TRIG_8_WIDTH                         1
#define F_QOF_CAM_TOP_ACK_EVENT_TRIG_7_POS                           6
#define F_QOF_CAM_TOP_ACK_EVENT_TRIG_7_WIDTH                         1
#define F_QOF_CAM_TOP_ACK_EVENT_TRIG_6_POS                           5
#define F_QOF_CAM_TOP_ACK_EVENT_TRIG_6_WIDTH                         1
#define F_QOF_CAM_TOP_ACK_EVENT_TRIG_5_POS                           4
#define F_QOF_CAM_TOP_ACK_EVENT_TRIG_5_WIDTH                         1
#define F_QOF_CAM_TOP_ACK_EVENT_TRIG_4_POS                           3
#define F_QOF_CAM_TOP_ACK_EVENT_TRIG_4_WIDTH                         1
#define F_QOF_CAM_TOP_ACK_EVENT_TRIG_3_POS                           2
#define F_QOF_CAM_TOP_ACK_EVENT_TRIG_3_WIDTH                         1
#define F_QOF_CAM_TOP_ACK_EVENT_TRIG_2_POS                           1
#define F_QOF_CAM_TOP_ACK_EVENT_TRIG_2_WIDTH                         1
#define F_QOF_CAM_TOP_ACK_EVENT_TRIG_1_POS                           0
#define F_QOF_CAM_TOP_ACK_EVENT_TRIG_1_WIDTH                         1

#define REG_QOF_CAM_TOP_QOF_SW_RST                   0x20
#define F_QOF_CAM_TOP_QOF_SW_RST_POS                                 0
#define F_QOF_CAM_TOP_QOF_SW_RST_WIDTH                               14

/* baseaddr 0x3A0B0000 */

/* module: QOF_CAM_A_E1A */
#define REG_QOF_CAM_A_QOF_CTL_1                       0x0
#define F_QOF_CAM_A_QOF_CQ_EN_1_POS                                  27
#define F_QOF_CAM_A_QOF_CQ_EN_1_WIDTH                                1
#define F_QOF_CAM_A_OFF_SEL_1_POS                                    26
#define F_QOF_CAM_A_OFF_SEL_1_WIDTH                                  1
#define F_QOF_CAM_A_ON_SEL_1_POS                                     25
#define F_QOF_CAM_A_ON_SEL_1_WIDTH                                   1
#define F_QOF_CAM_A_BW_QOS_SW_1_POS                                  24
#define F_QOF_CAM_A_BW_QOS_SW_1_WIDTH                                1
#define F_QOF_CAM_A_BW_QOS_HW_EN_1_POS                               23
#define F_QOF_CAM_A_BW_QOS_HW_EN_1_WIDTH                             1
#define F_QOF_CAM_A_BW_QOS_SW_CLR_1_POS                              22
#define F_QOF_CAM_A_BW_QOS_SW_CLR_1_WIDTH                            1
#define F_QOF_CAM_A_BW_QOS_SW_SET_1_POS                              21
#define F_QOF_CAM_A_BW_QOS_SW_SET_1_WIDTH                            1
#define F_QOF_CAM_A_DDREN_SW_1_POS                                   20
#define F_QOF_CAM_A_DDREN_SW_1_WIDTH                                 1
#define F_QOF_CAM_A_DDREN_HW_EN_1_POS                                19
#define F_QOF_CAM_A_DDREN_HW_EN_1_WIDTH                              1
#define F_QOF_CAM_A_OPT_MTC_ACT_1_POS                                18
#define F_QOF_CAM_A_OPT_MTC_ACT_1_WIDTH                              1
#define F_QOF_CAM_A_SW_CQ_OFF_1_POS                                  17
#define F_QOF_CAM_A_SW_CQ_OFF_1_WIDTH                                1
#define F_QOF_CAM_A_SW_RAW_OFF_1_POS                                 16
#define F_QOF_CAM_A_SW_RAW_OFF_1_WIDTH                               1
#define F_QOF_CAM_A_CQ_HW_CLR_EN_1_POS                               15
#define F_QOF_CAM_A_CQ_HW_CLR_EN_1_WIDTH                             1
#define F_QOF_CAM_A_CQ_HW_SET_EN_1_POS                               14
#define F_QOF_CAM_A_CQ_HW_SET_EN_1_WIDTH                             1
#define F_QOF_CAM_A_RAW_HW_CLR_EN_1_POS                              13
#define F_QOF_CAM_A_RAW_HW_CLR_EN_1_WIDTH                            1
#define F_QOF_CAM_A_RAW_HW_SET_EN_1_POS                              12
#define F_QOF_CAM_A_RAW_HW_SET_EN_1_WIDTH                            1
#define F_QOF_CAM_A_OFF_ITC_W_EN_1_POS                               11
#define F_QOF_CAM_A_OFF_ITC_W_EN_1_WIDTH                             1
#define F_QOF_CAM_A_ON_ITC_W_EN_1_POS                                10
#define F_QOF_CAM_A_ON_ITC_W_EN_1_WIDTH                              1
#define F_QOF_CAM_A_HW_SEQ_EN_1_POS                                  9
#define F_QOF_CAM_A_HW_SEQ_EN_1_WIDTH                                1
#define F_QOF_CAM_A_RTC_EN_1_POS                                     8
#define F_QOF_CAM_A_RTC_EN_1_WIDTH                                   1
#define F_QOF_CAM_A_DDREN_SW_CLR_1_POS                               7
#define F_QOF_CAM_A_DDREN_SW_CLR_1_WIDTH                             1
#define F_QOF_CAM_A_DDREN_SW_SET_1_POS                               6
#define F_QOF_CAM_A_DDREN_SW_SET_1_WIDTH                             1
#define F_QOF_CAM_A_CQ_START_1_POS                                   5
#define F_QOF_CAM_A_CQ_START_1_WIDTH                                 1
#define F_QOF_CAM_A_SCP_CLR_1_POS                                    3
#define F_QOF_CAM_A_SCP_CLR_1_WIDTH                                  1
#define F_QOF_CAM_A_SCP_SET_1_POS                                    2
#define F_QOF_CAM_A_SCP_SET_1_WIDTH                                  1
#define F_QOF_CAM_A_APMCU_CLR_1_POS                                  1
#define F_QOF_CAM_A_APMCU_CLR_1_WIDTH                                1
#define F_QOF_CAM_A_APMCU_SET_1_POS                                  0
#define F_QOF_CAM_A_APMCU_SET_1_WIDTH                                1

#define REG_QOF_CAM_A_QOF_DONE_STATUS_1               0x4
#define F_QOF_CAM_A_CQ_ITCW_DONE_1_POS                               4
#define F_QOF_CAM_A_CQ_ITCW_DONE_1_WIDTH                             1
#define F_QOF_CAM_A_CFG_OFF_DONE_1_POS                               3
#define F_QOF_CAM_A_CFG_OFF_DONE_1_WIDTH                             1
#define F_QOF_CAM_A_CFG_ON_DONE_1_POS                                2
#define F_QOF_CAM_A_CFG_ON_DONE_1_WIDTH                              1
#define F_QOF_CAM_A_ITC_DONE_1_POS                                   1
#define F_QOF_CAM_A_ITC_DONE_1_WIDTH                                 1
#define F_QOF_CAM_A_RTC_DONE_1_POS                                   0
#define F_QOF_CAM_A_RTC_DONE_1_WIDTH                                 1

#define REG_QOF_CAM_A_QOF_RTC_DLY_CNT_1               0x8
#define REG_QOF_CAM_A_QOF_VOTER_DBG_1                 0xC
#define F_QOF_CAM_A_TRG_STATUS_1_POS                                 20
#define F_QOF_CAM_A_TRG_STATUS_1_WIDTH                               2
#define F_QOF_CAM_A_VOTE_CHECK_1_POS                                 4
#define F_QOF_CAM_A_VOTE_CHECK_1_WIDTH                               16
#define F_QOF_CAM_A_VOTE_1_POS                                       0
#define F_QOF_CAM_A_VOTE_1_WIDTH                                     4

#define REG_QOF_CAM_A_QOF_TRIG_CNT_1                 0x10
#define F_QOF_CAM_A_TRG_OFF_CNT_1_POS                                16
#define F_QOF_CAM_A_TRG_OFF_CNT_1_WIDTH                              16
#define F_QOF_CAM_A_TRG_ON_CNT_1_POS                                 0
#define F_QOF_CAM_A_TRG_ON_CNT_1_WIDTH                               16

#define REG_QOF_CAM_A_QOF_TRIG_CNT_CLR_1             0x14
#define F_QOF_CAM_A_TRG_OFF_CNT_CLR_1_POS                            1
#define F_QOF_CAM_A_TRG_OFF_CNT_CLR_1_WIDTH                          1
#define F_QOF_CAM_A_TRG_ON_CNT_CLR_1_POS                             0
#define F_QOF_CAM_A_TRG_ON_CNT_CLR_1_WIDTH                           1

#define REG_QOF_CAM_A_QOF_TIME_STAMP_1               0x18
#define REG_QOF_CAM_A_QOF_MTC_CYC_MAX_1              0x1C
#define REG_QOF_CAM_A_QOF_PWR_OFF_MAX_1              0x20
#define REG_QOF_CAM_A_QOF_CQ_START_MAX_1             0x24
#define REG_QOF_CAM_A_QOF_GCE_CTL_1                  0x28
#define F_QOF_CAM_A_GCE_RESTORE_DONE_1_POS                           3
#define F_QOF_CAM_A_GCE_RESTORE_DONE_1_WIDTH                         1
#define F_QOF_CAM_A_GCE_RESTORE_EN_1_POS                             2
#define F_QOF_CAM_A_GCE_RESTORE_EN_1_WIDTH                           1
#define F_QOF_CAM_A_GCE_SAVE_DONE_1_POS                              1
#define F_QOF_CAM_A_GCE_SAVE_DONE_1_WIDTH                            1
#define F_QOF_CAM_A_GCE_SAVE_EN_1_POS                                0
#define F_QOF_CAM_A_GCE_SAVE_EN_1_WIDTH                              1

#define REG_QOF_CAM_A_QOF_VSYNC_PRD_1                0x2C
#define REG_QOF_CAM_A_QOF_STATE_DBG_1                0x30
#define F_QOF_CAM_A_ACK_DBG_1_POS                                    19
#define F_QOF_CAM_A_ACK_DBG_1_WIDTH                                  10
#define F_QOF_CAM_A_HW_SEQ_ST_1_POS                                  12
#define F_QOF_CAM_A_HW_SEQ_ST_1_WIDTH                                7
#define F_QOF_CAM_A_POWER_STATE_1_POS                                0
#define F_QOF_CAM_A_POWER_STATE_1_WIDTH                              8

#define REG_QOF_CAM_A_QOF_RETENTION_CYCLE_1          0x34
#define F_QOF_CAM_A_RTFF_SAVE_RES_TH_1_POS                           16
#define F_QOF_CAM_A_RTFF_SAVE_RES_TH_1_WIDTH                         16
#define F_QOF_CAM_A_RTFF_CLK_DIS_TH_1_POS                            0
#define F_QOF_CAM_A_RTFF_CLK_DIS_TH_1_WIDTH                          16

#define REG_QOF_CAM_A_QOF_POWER_ACK_CYCLE_1          0x38
#define F_QOF_CAM_A_PWR_ACK_2ND_WAIT_TH_1_POS                        16
#define F_QOF_CAM_A_PWR_ACK_2ND_WAIT_TH_1_WIDTH                      16
#define F_QOF_CAM_A_PWR_ACK_WAIT_TH_1_POS                            0
#define F_QOF_CAM_A_PWR_ACK_WAIT_TH_1_WIDTH                          16

#define REG_QOF_CAM_A_QOF_POWER_ISO_CYCLE_1          0x3C
#define F_QOF_CAM_A_PWR_ISO_0_TH_1_POS                               16
#define F_QOF_CAM_A_PWR_ISO_0_TH_1_WIDTH                             16
#define F_QOF_CAM_A_PWR_ISO_1_TH_1_POS                               0
#define F_QOF_CAM_A_PWR_ISO_1_TH_1_WIDTH                             16

#define REG_QOF_CAM_A_QOF_POWER_CYCLE_1              0x40
#define F_QOF_CAM_A_SRAM_WAIT_TH_1_POS                               16
#define F_QOF_CAM_A_SRAM_WAIT_TH_1_WIDTH                             16
#define F_QOF_CAM_A_PWR_CLK_DIS_TH_1_POS                             0
#define F_QOF_CAM_A_PWR_CLK_DIS_TH_1_WIDTH                           16

#define REG_QOF_CAM_A_INT1_STATUS_ADDR_1             0x44
#define F_QOF_CAM_A_INT1_STATUS_ADDR_1_POS                           0
#define F_QOF_CAM_A_INT1_STATUS_ADDR_1_WIDTH                         24

#define REG_QOF_CAM_A_INT2_STATUS_ADDR_1             0x48
#define F_QOF_CAM_A_INT2_STATUS_ADDR_1_POS                           0
#define F_QOF_CAM_A_INT2_STATUS_ADDR_1_WIDTH                         24

#define REG_QOF_CAM_A_INT3_STATUS_ADDR_1             0x4C
#define F_QOF_CAM_A_INT3_STATUS_ADDR_1_POS                           0
#define F_QOF_CAM_A_INT3_STATUS_ADDR_1_WIDTH                         24

#define REG_QOF_CAM_A_INT4_STATUS_ADDR_1             0x50
#define F_QOF_CAM_A_INT4_STATUS_ADDR_1_POS                           0
#define F_QOF_CAM_A_INT4_STATUS_ADDR_1_WIDTH                         24

#define REG_QOF_CAM_A_INT5_STATUS_ADDR_1             0x54
#define F_QOF_CAM_A_INT5_STATUS_ADDR_1_POS                           0
#define F_QOF_CAM_A_INT5_STATUS_ADDR_1_WIDTH                         24

#define REG_QOF_CAM_A_INT6_STATUS_ADDR_1             0x58
#define F_QOF_CAM_A_INT6_STATUS_ADDR_1_POS                           0
#define F_QOF_CAM_A_INT6_STATUS_ADDR_1_WIDTH                         24

#define REG_QOF_CAM_A_INT7_STATUS_ADDR_1             0x5C
#define F_QOF_CAM_A_INT7_STATUS_ADDR_1_POS                           0
#define F_QOF_CAM_A_INT7_STATUS_ADDR_1_WIDTH                         24

#define REG_QOF_CAM_A_INT8_STATUS_ADDR_1             0x60
#define F_QOF_CAM_A_INT8_STATUS_ADDR_1_POS                           0
#define F_QOF_CAM_A_INT8_STATUS_ADDR_1_WIDTH                         24

#define REG_QOF_CAM_A_INT9_STATUS_ADDR_1             0x64
#define F_QOF_CAM_A_INT9_STATUS_ADDR_1_POS                           0
#define F_QOF_CAM_A_INT9_STATUS_ADDR_1_WIDTH                         24

#define REG_QOF_CAM_A_INT10_STATUS_ADDR_1            0x68
#define F_QOF_CAM_A_INT10_STATUS_ADDR_1_POS                          0
#define F_QOF_CAM_A_INT10_STATUS_ADDR_1_WIDTH                        24

#define REG_QOF_CAM_A_INT11_STATUS_ADDR_1            0x6C
#define F_QOF_CAM_A_INT11_STATUS_ADDR_1_POS                          0
#define F_QOF_CAM_A_INT11_STATUS_ADDR_1_WIDTH                        24

#define REG_QOF_CAM_A_INT12_STATUS_ADDR_1            0x70
#define F_QOF_CAM_A_INT12_STATUS_ADDR_1_POS                          0
#define F_QOF_CAM_A_INT12_STATUS_ADDR_1_WIDTH                        24

#define REG_QOF_CAM_A_INT13_STATUS_ADDR_1            0x74
#define F_QOF_CAM_A_INT13_STATUS_ADDR_1_POS                          0
#define F_QOF_CAM_A_INT13_STATUS_ADDR_1_WIDTH                        24

#define REG_QOF_CAM_A_INT14_STATUS_ADDR_1            0x78
#define F_QOF_CAM_A_INT14_STATUS_ADDR_1_POS                          0
#define F_QOF_CAM_A_INT14_STATUS_ADDR_1_WIDTH                        24

#define REG_QOF_CAM_A_INT15_STATUS_ADDR_1            0x7C
#define F_QOF_CAM_A_INT15_STATUS_ADDR_1_POS                          0
#define F_QOF_CAM_A_INT15_STATUS_ADDR_1_WIDTH                        24

#define REG_QOF_CAM_A_INT16_STATUS_ADDR_1            0x80
#define F_QOF_CAM_A_INT16_STATUS_ADDR_1_POS                          0
#define F_QOF_CAM_A_INT16_STATUS_ADDR_1_WIDTH                        24

#define REG_QOF_CAM_A_INT17_STATUS_ADDR_1            0x84
#define F_QOF_CAM_A_INT17_STATUS_ADDR_1_POS                          0
#define F_QOF_CAM_A_INT17_STATUS_ADDR_1_WIDTH                        24

#define REG_QOF_CAM_A_INT18_STATUS_ADDR_1            0x88
#define F_QOF_CAM_A_INT18_STATUS_ADDR_1_POS                          0
#define F_QOF_CAM_A_INT18_STATUS_ADDR_1_WIDTH                        24

#define REG_QOF_CAM_A_INT19_STATUS_ADDR_1            0x8C
#define F_QOF_CAM_A_INT19_STATUS_ADDR_1_POS                          0
#define F_QOF_CAM_A_INT19_STATUS_ADDR_1_WIDTH                        24

#define REG_QOF_CAM_A_INT20_STATUS_ADDR_1            0x90
#define F_QOF_CAM_A_INT20_STATUS_ADDR_1_POS                          0
#define F_QOF_CAM_A_INT20_STATUS_ADDR_1_WIDTH                        24

#define REG_QOF_CAM_A_INT21_STATUS_ADDR_1            0x94
#define F_QOF_CAM_A_INT21_STATUS_ADDR_1_POS                          0
#define F_QOF_CAM_A_INT21_STATUS_ADDR_1_WIDTH                        24

#define REG_QOF_CAM_A_INT22_STATUS_ADDR_1            0x98
#define F_QOF_CAM_A_INT22_STATUS_ADDR_1_POS                          0
#define F_QOF_CAM_A_INT22_STATUS_ADDR_1_WIDTH                        24

#define REG_QOF_CAM_A_INT23_STATUS_ADDR_1            0x9C
#define F_QOF_CAM_A_INT23_STATUS_ADDR_1_POS                          0
#define F_QOF_CAM_A_INT23_STATUS_ADDR_1_WIDTH                        24

#define REG_QOF_CAM_A_INT24_STATUS_ADDR_1            0xA0
#define F_QOF_CAM_A_INT24_STATUS_ADDR_1_POS                          0
#define F_QOF_CAM_A_INT24_STATUS_ADDR_1_WIDTH                        24

#define REG_QOF_CAM_A_INT25_STATUS_ADDR_1            0xA4
#define F_QOF_CAM_A_INT25_STATUS_ADDR_1_POS                          0
#define F_QOF_CAM_A_INT25_STATUS_ADDR_1_WIDTH                        24

#define REG_QOF_CAM_A_INT26_STATUS_ADDR_1            0xA8
#define F_QOF_CAM_A_INT26_STATUS_ADDR_1_POS                          0
#define F_QOF_CAM_A_INT26_STATUS_ADDR_1_WIDTH                        24

#define REG_QOF_CAM_A_INT27_STATUS_ADDR_1            0xAC
#define F_QOF_CAM_A_INT27_STATUS_ADDR_1_POS                          0
#define F_QOF_CAM_A_INT27_STATUS_ADDR_1_WIDTH                        24

#define REG_QOF_CAM_A_INT28_STATUS_ADDR_1            0xB0
#define F_QOF_CAM_A_INT28_STATUS_ADDR_1_POS                          0
#define F_QOF_CAM_A_INT28_STATUS_ADDR_1_WIDTH                        24

#define REG_QOF_CAM_A_INT29_STATUS_ADDR_1            0xB4
#define F_QOF_CAM_A_INT29_STATUS_ADDR_1_POS                          0
#define F_QOF_CAM_A_INT29_STATUS_ADDR_1_WIDTH                        24

#define REG_QOF_CAM_A_INT30_STATUS_ADDR_1            0xB8
#define F_QOF_CAM_A_INT30_STATUS_ADDR_1_POS                          0
#define F_QOF_CAM_A_INT30_STATUS_ADDR_1_WIDTH                        24

#define REG_QOF_CAM_A_INT31_STATUS_ADDR_1            0xBC
#define F_QOF_CAM_A_INT31_STATUS_ADDR_1_POS                          0
#define F_QOF_CAM_A_INT31_STATUS_ADDR_1_WIDTH                        24

#define REG_QOF_CAM_A_INT32_STATUS_ADDR_1            0xC0
#define F_QOF_CAM_A_INT32_STATUS_ADDR_1_POS                          0
#define F_QOF_CAM_A_INT32_STATUS_ADDR_1_WIDTH                        24

#define REG_QOF_CAM_A_INT33_STATUS_ADDR_1            0xC4
#define F_QOF_CAM_A_INT33_STATUS_ADDR_1_POS                          0
#define F_QOF_CAM_A_INT33_STATUS_ADDR_1_WIDTH                        24

#define REG_QOF_CAM_A_INT34_STATUS_ADDR_1            0xC8
#define F_QOF_CAM_A_INT34_STATUS_ADDR_1_POS                          0
#define F_QOF_CAM_A_INT34_STATUS_ADDR_1_WIDTH                        24

#define REG_QOF_CAM_A_INT35_STATUS_ADDR_1            0xCC
#define F_QOF_CAM_A_INT35_STATUS_ADDR_1_POS                          0
#define F_QOF_CAM_A_INT35_STATUS_ADDR_1_WIDTH                        24

#define REG_QOF_CAM_A_INT36_STATUS_ADDR_1            0xD0
#define F_QOF_CAM_A_INT36_STATUS_ADDR_1_POS                          0
#define F_QOF_CAM_A_INT36_STATUS_ADDR_1_WIDTH                        24

#define REG_QOF_CAM_A_INT37_STATUS_ADDR_1            0xD4
#define F_QOF_CAM_A_INT37_STATUS_ADDR_1_POS                          0
#define F_QOF_CAM_A_INT37_STATUS_ADDR_1_WIDTH                        24

#define REG_QOF_CAM_A_INT38_STATUS_ADDR_1            0xD8
#define F_QOF_CAM_A_INT38_STATUS_ADDR_1_POS                          0
#define F_QOF_CAM_A_INT38_STATUS_ADDR_1_WIDTH                        24

#define REG_QOF_CAM_A_INT39_STATUS_ADDR_1            0xDC
#define F_QOF_CAM_A_INT39_STATUS_ADDR_1_POS                          0
#define F_QOF_CAM_A_INT39_STATUS_ADDR_1_WIDTH                        24

#define REG_QOF_CAM_A_INT1_STATUS_1                  0xE0
#define REG_QOF_CAM_A_INT2_STATUS_1                  0xE4
#define REG_QOF_CAM_A_INT3_STATUS_1                  0xE8
#define REG_QOF_CAM_A_INT4_STATUS_1                  0xEC
#define REG_QOF_CAM_A_INT5_STATUS_1                  0xF0
#define REG_QOF_CAM_A_INT6_STATUS_1                  0xF4
#define REG_QOF_CAM_A_INT7_STATUS_1                  0xF8
#define REG_QOF_CAM_A_INT8_STATUS_1                  0xFC
#define REG_QOF_CAM_A_INT9_STATUS_1                 0x100
#define REG_QOF_CAM_A_INT10_STATUS_1                0x104
#define REG_QOF_CAM_A_INT11_STATUS_1                0x108
#define REG_QOF_CAM_A_INT12_STATUS_1                0x10C
#define REG_QOF_CAM_A_INT13_STATUS_1                0x110
#define REG_QOF_CAM_A_INT14_STATUS_1                0x114
#define REG_QOF_CAM_A_INT15_STATUS_1                0x118
#define REG_QOF_CAM_A_INT16_STATUS_1                0x11C
#define REG_QOF_CAM_A_INT17_STATUS_1                0x120
#define REG_QOF_CAM_A_INT18_STATUS_1                0x124
#define REG_QOF_CAM_A_INT19_STATUS_1                0x128
#define REG_QOF_CAM_A_INT20_STATUS_1                0x12C
#define REG_QOF_CAM_A_INT21_STATUS_1                0x130
#define REG_QOF_CAM_A_INT22_STATUS_1                0x134
#define REG_QOF_CAM_A_INT23_STATUS_1                0x138
#define REG_QOF_CAM_A_INT24_STATUS_1                0x13C
#define REG_QOF_CAM_A_INT25_STATUS_1                0x140
#define REG_QOF_CAM_A_INT26_STATUS_1                0x144
#define REG_QOF_CAM_A_INT27_STATUS_1                0x148
#define REG_QOF_CAM_A_INT28_STATUS_1                0x14C
#define REG_QOF_CAM_A_INT29_STATUS_1                0x150
#define REG_QOF_CAM_A_INT30_STATUS_1                0x154
#define REG_QOF_CAM_A_INT31_STATUS_1                0x158
#define REG_QOF_CAM_A_INT32_STATUS_1                0x15C
#define REG_QOF_CAM_A_INT33_STATUS_1                0x160
#define REG_QOF_CAM_A_INT34_STATUS_1                0x164
#define REG_QOF_CAM_A_INT35_STATUS_1                0x168
#define REG_QOF_CAM_A_INT36_STATUS_1                0x16C
#define REG_QOF_CAM_A_INT37_STATUS_1                0x170
#define REG_QOF_CAM_A_INT38_STATUS_1                0x174
#define REG_QOF_CAM_A_INT39_STATUS_1                0x178
#define REG_QOF_CAM_A_TRANS1_ADDR_1                 0x180
#define F_QOF_CAM_A_TRANS1_ADDR_1_POS                                0
#define F_QOF_CAM_A_TRANS1_ADDR_1_WIDTH                              24

#define REG_QOF_CAM_A_TRANS2_ADDR_1                 0x184
#define F_QOF_CAM_A_TRANS2_ADDR_1_POS                                0
#define F_QOF_CAM_A_TRANS2_ADDR_1_WIDTH                              24

#define REG_QOF_CAM_A_TRANS3_ADDR_1                 0x188
#define F_QOF_CAM_A_TRANS3_ADDR_1_POS                                0
#define F_QOF_CAM_A_TRANS3_ADDR_1_WIDTH                              24

#define REG_QOF_CAM_A_TRANS4_ADDR_1                 0x18C
#define F_QOF_CAM_A_TRANS4_ADDR_1_POS                                0
#define F_QOF_CAM_A_TRANS4_ADDR_1_WIDTH                              24

#define REG_QOF_CAM_A_TRANS5_ADDR_1                 0x190
#define F_QOF_CAM_A_TRANS5_ADDR_1_POS                                0
#define F_QOF_CAM_A_TRANS5_ADDR_1_WIDTH                              24

#define REG_QOF_CAM_A_TRANS6_ADDR_1                 0x194
#define F_QOF_CAM_A_TRANS6_ADDR_1_POS                                0
#define F_QOF_CAM_A_TRANS6_ADDR_1_WIDTH                              24

#define REG_QOF_CAM_A_TRANS7_ADDR_1                 0x198
#define F_QOF_CAM_A_TRANS7_ADDR_1_POS                                0
#define F_QOF_CAM_A_TRANS7_ADDR_1_WIDTH                              24

#define REG_QOF_CAM_A_TRANS8_ADDR_1                 0x19C
#define F_QOF_CAM_A_TRANS8_ADDR_1_POS                                0
#define F_QOF_CAM_A_TRANS8_ADDR_1_WIDTH                              24

#define REG_QOF_CAM_A_TRANS9_ADDR_1                 0x1A0
#define F_QOF_CAM_A_TRANS9_ADDR_1_POS                                0
#define F_QOF_CAM_A_TRANS9_ADDR_1_WIDTH                              24

#define REG_QOF_CAM_A_TRANS10_ADDR_1                0x1A4
#define F_QOF_CAM_A_TRANS10_ADDR_1_POS                               0
#define F_QOF_CAM_A_TRANS10_ADDR_1_WIDTH                             24

#define REG_QOF_CAM_A_TRANS11_ADDR_1                0x1A8
#define F_QOF_CAM_A_TRANS11_ADDR_1_POS                               0
#define F_QOF_CAM_A_TRANS11_ADDR_1_WIDTH                             24

#define REG_QOF_CAM_A_TRANS12_ADDR_1                0x1AC
#define F_QOF_CAM_A_TRANS12_ADDR_1_POS                               0
#define F_QOF_CAM_A_TRANS12_ADDR_1_WIDTH                             24

#define REG_QOF_CAM_A_TRANS13_ADDR_1                0x1B0
#define F_QOF_CAM_A_TRANS13_ADDR_1_POS                               0
#define F_QOF_CAM_A_TRANS13_ADDR_1_WIDTH                             24

#define REG_QOF_CAM_A_TRANS14_ADDR_1                0x1B4
#define F_QOF_CAM_A_TRANS14_ADDR_1_POS                               0
#define F_QOF_CAM_A_TRANS14_ADDR_1_WIDTH                             24

#define REG_QOF_CAM_A_TRANS15_ADDR_1                0x1B8
#define F_QOF_CAM_A_TRANS15_ADDR_1_POS                               0
#define F_QOF_CAM_A_TRANS15_ADDR_1_WIDTH                             24

#define REG_QOF_CAM_A_TRANS16_ADDR_1                0x1BC
#define F_QOF_CAM_A_TRANS16_ADDR_1_POS                               0
#define F_QOF_CAM_A_TRANS16_ADDR_1_WIDTH                             24

#define REG_QOF_CAM_A_TRANS1_DATA_1                 0x1D0
#define REG_QOF_CAM_A_TRANS2_DATA_1                 0x1D4
#define REG_QOF_CAM_A_TRANS3_DATA_1                 0x1D8
#define REG_QOF_CAM_A_TRANS4_DATA_1                 0x1DC
#define REG_QOF_CAM_A_TRANS5_DATA_1                 0x1E0
#define REG_QOF_CAM_A_TRANS6_DATA_1                 0x1E4
#define REG_QOF_CAM_A_TRANS7_DATA_1                 0x1E8
#define REG_QOF_CAM_A_TRANS8_DATA_1                 0x1EC
#define REG_QOF_CAM_A_TRANS9_DATA_1                 0x1F0
#define REG_QOF_CAM_A_TRANS10_DATA_1                0x1F4
#define REG_QOF_CAM_A_TRANS11_DATA_1                0x1F8
#define REG_QOF_CAM_A_TRANS12_DATA_1                0x1FC
#define REG_QOF_CAM_A_TRANS13_DATA_1                0x200
#define REG_QOF_CAM_A_TRANS14_DATA_1                0x204
#define REG_QOF_CAM_A_TRANS15_DATA_1                0x208
#define REG_QOF_CAM_A_TRANS16_DATA_1                0x20C
#define REG_QOF_CAM_A_CONFIG1_ADDR_1                0x230
#define F_QOF_CAM_A_CONFIG1_ADDR_1_POS                               0
#define F_QOF_CAM_A_CONFIG1_ADDR_1_WIDTH                             24

#define REG_QOF_CAM_A_CONFIG2_ADDR_1                0x234
#define F_QOF_CAM_A_CONFIG2_ADDR_1_POS                               0
#define F_QOF_CAM_A_CONFIG2_ADDR_1_WIDTH                             24

#define REG_QOF_CAM_A_CONFIG3_ADDR_1                0x238
#define F_QOF_CAM_A_CONFIG3_ADDR_1_POS                               0
#define F_QOF_CAM_A_CONFIG3_ADDR_1_WIDTH                             24

#define REG_QOF_CAM_A_CONFIG4_ADDR_1                0x23C
#define F_QOF_CAM_A_CONFIG4_ADDR_1_POS                               0
#define F_QOF_CAM_A_CONFIG4_ADDR_1_WIDTH                             24

#define REG_QOF_CAM_A_CONFIG5_ADDR_1                0x240
#define F_QOF_CAM_A_CONFIG5_ADDR_1_POS                               0
#define F_QOF_CAM_A_CONFIG5_ADDR_1_WIDTH                             24

#define REG_QOF_CAM_A_CONFIG6_ADDR_1                0x244
#define F_QOF_CAM_A_CONFIG6_ADDR_1_POS                               0
#define F_QOF_CAM_A_CONFIG6_ADDR_1_WIDTH                             24

#define REG_QOF_CAM_A_CONFIG7_ADDR_1                0x248
#define F_QOF_CAM_A_CONFIG7_ADDR_1_POS                               0
#define F_QOF_CAM_A_CONFIG7_ADDR_1_WIDTH                             24

#define REG_QOF_CAM_A_CONFIG8_ADDR_1                0x24C
#define F_QOF_CAM_A_CONFIG8_ADDR_1_POS                               0
#define F_QOF_CAM_A_CONFIG8_ADDR_1_WIDTH                             24

#define REG_QOF_CAM_A_CONFIG9_ADDR_1                0x250
#define F_QOF_CAM_A_CONFIG9_ADDR_1_POS                               0
#define F_QOF_CAM_A_CONFIG9_ADDR_1_WIDTH                             24

#define REG_QOF_CAM_A_CONFIG10_ADDR_1               0x254
#define F_QOF_CAM_A_CONFIG10_ADDR_1_POS                              0
#define F_QOF_CAM_A_CONFIG10_ADDR_1_WIDTH                            24

#define REG_QOF_CAM_A_CONFIG1_DATA_1                0x270
#define REG_QOF_CAM_A_CONFIG2_DATA_1                0x274
#define REG_QOF_CAM_A_CONFIG3_DATA_1                0x278
#define REG_QOF_CAM_A_CONFIG4_DATA_1                0x27C
#define REG_QOF_CAM_A_CONFIG5_DATA_1                0x280
#define REG_QOF_CAM_A_CONFIG6_DATA_1                0x284
#define REG_QOF_CAM_A_CONFIG7_DATA_1                0x288
#define REG_QOF_CAM_A_CONFIG8_DATA_1                0x28C
#define REG_QOF_CAM_A_CONFIG9_DATA_1                0x290
#define REG_QOF_CAM_A_CONFIG10_DATA_1               0x294
#define REG_QOF_CAM_A_CONFIG_SECURE_EN_1            0x2A0
#define F_QOF_CAM_A_CONFIG_SECURE_EN_1_POS                           0
#define F_QOF_CAM_A_CONFIG_SECURE_EN_1_WIDTH                         10

#define REG_QOF_CAM_A_QOF_SPARE1_1                  0x2B0
#define REG_QOF_CAM_A_QOF_SPARE2_1                  0x2B4
#define REG_QOF_CAM_A_QOF_SPARE3_1                  0x2B8
#define REG_QOF_CAM_A_QOF_SPARE4_1                  0x2BC
#define REG_QOF_CAM_A_CONFIG1_ADDR_OFF_1            0x2D0
#define F_QOF_CAM_A_CONFIG1_ADDR_OFF_1_POS                           0
#define F_QOF_CAM_A_CONFIG1_ADDR_OFF_1_WIDTH                         24

#define REG_QOF_CAM_A_CONFIG2_ADDR_OFF_1            0x2D4
#define F_QOF_CAM_A_CONFIG2_ADDR_OFF_1_POS                           0
#define F_QOF_CAM_A_CONFIG2_ADDR_OFF_1_WIDTH                         24

#define REG_QOF_CAM_A_CONFIG3_ADDR_OFF_1            0x2D8
#define F_QOF_CAM_A_CONFIG3_ADDR_OFF_1_POS                           0
#define F_QOF_CAM_A_CONFIG3_ADDR_OFF_1_WIDTH                         24

#define REG_QOF_CAM_A_CONFIG4_ADDR_OFF_1            0x2DC
#define F_QOF_CAM_A_CONFIG4_ADDR_OFF_1_POS                           0
#define F_QOF_CAM_A_CONFIG4_ADDR_OFF_1_WIDTH                         24

#define REG_QOF_CAM_A_CONFIG5_ADDR_OFF_1            0x2E0
#define F_QOF_CAM_A_CONFIG5_ADDR_OFF_1_POS                           0
#define F_QOF_CAM_A_CONFIG5_ADDR_OFF_1_WIDTH                         24

#define REG_QOF_CAM_A_CONFIG6_ADDR_OFF_1            0x2E4
#define F_QOF_CAM_A_CONFIG6_ADDR_OFF_1_POS                           0
#define F_QOF_CAM_A_CONFIG6_ADDR_OFF_1_WIDTH                         24

#define REG_QOF_CAM_A_CONFIG7_ADDR_OFF_1            0x2E8
#define F_QOF_CAM_A_CONFIG7_ADDR_OFF_1_POS                           0
#define F_QOF_CAM_A_CONFIG7_ADDR_OFF_1_WIDTH                         24

#define REG_QOF_CAM_A_CONFIG8_ADDR_OFF_1            0x2EC
#define F_QOF_CAM_A_CONFIG8_ADDR_OFF_1_POS                           0
#define F_QOF_CAM_A_CONFIG8_ADDR_OFF_1_WIDTH                         24

#define REG_QOF_CAM_A_CONFIG9_ADDR_OFF_1            0x2F0
#define F_QOF_CAM_A_CONFIG9_ADDR_OFF_1_POS                           0
#define F_QOF_CAM_A_CONFIG9_ADDR_OFF_1_WIDTH                         24

#define REG_QOF_CAM_A_CONFIG10_ADDR_OFF_1           0x2F4
#define F_QOF_CAM_A_CONFIG10_ADDR_OFF_1_POS                          0
#define F_QOF_CAM_A_CONFIG10_ADDR_OFF_1_WIDTH                        24

#define REG_QOF_CAM_A_CONFIG1_DATA_OFF_1            0x310
#define REG_QOF_CAM_A_CONFIG2_DATA_OFF_1            0x314
#define REG_QOF_CAM_A_CONFIG3_DATA_OFF_1            0x318
#define REG_QOF_CAM_A_CONFIG4_DATA_OFF_1            0x31C
#define REG_QOF_CAM_A_CONFIG5_DATA_OFF_1            0x320
#define REG_QOF_CAM_A_CONFIG6_DATA_OFF_1            0x324
#define REG_QOF_CAM_A_CONFIG7_DATA_OFF_1            0x328
#define REG_QOF_CAM_A_CONFIG8_DATA_OFF_1            0x32C
#define REG_QOF_CAM_A_CONFIG9_DATA_OFF_1            0x330
#define REG_QOF_CAM_A_CONFIG10_DATA_OFF_1           0x334
#define REG_QOF_CAM_A_CONFIG_SECURE_OFF_EN_1        0x340
#define F_QOF_CAM_A_CONFIG_SECURE_OFF_EN_1_POS                       0
#define F_QOF_CAM_A_CONFIG_SECURE_OFF_EN_1_WIDTH                     10

#define REG_QOF_CAM_A_QOF_DDREN_CYC_MAX_1           0x350
#define REG_QOF_CAM_A_QOF_BWQOS_CYC_MAX_1           0x354
#define REG_QOF_CAM_A_QOF_TIMEOUT_MAX_1             0x358
#define REG_QOF_CAM_A_QOF_DEBOUNCE_CYC_1            0x35C
#define REG_QOF_CAM_A_QOF_CQ1_ADDR_1                0x360
#define F_QOF_CAM_A_QOF_CQ1_ADDR_1_POS                               0
#define F_QOF_CAM_A_QOF_CQ1_ADDR_1_WIDTH                             24

#define REG_QOF_CAM_A_QOF_CQ2_ADDR_1                0x364
#define F_QOF_CAM_A_QOF_CQ2_ADDR_1_POS                               0
#define F_QOF_CAM_A_QOF_CQ2_ADDR_1_WIDTH                             24

#define REG_QOF_CAM_A_QOF_CQ3_ADDR_1                0x368
#define F_QOF_CAM_A_QOF_CQ3_ADDR_1_POS                               0
#define F_QOF_CAM_A_QOF_CQ3_ADDR_1_WIDTH                             24

#define REG_QOF_CAM_A_QOF_CQ4_ADDR_1                0x36C
#define F_QOF_CAM_A_QOF_CQ4_ADDR_1_POS                               0
#define F_QOF_CAM_A_QOF_CQ4_ADDR_1_WIDTH                             24

#define REG_QOF_CAM_A_QOF_CQ5_ADDR_1                0x370
#define F_QOF_CAM_A_QOF_CQ5_ADDR_1_POS                               0
#define F_QOF_CAM_A_QOF_CQ5_ADDR_1_WIDTH                             24

#define REG_QOF_CAM_A_QOF_CQ6_ADDR_1                0x374
#define F_QOF_CAM_A_QOF_CQ6_ADDR_1_POS                               0
#define F_QOF_CAM_A_QOF_CQ6_ADDR_1_WIDTH                             24

#define REG_QOF_CAM_A_QOF_CQ1_DATA_1                0x378
#define REG_QOF_CAM_A_QOF_CQ2_DATA_1                0x37C
#define REG_QOF_CAM_A_QOF_CQ3_DATA_1                0x380
#define REG_QOF_CAM_A_QOF_CQ4_DATA_1                0x384
#define REG_QOF_CAM_A_QOF_CQ5_DATA_1                0x388
#define REG_QOF_CAM_A_QOF_CQ6_DATA_1                0x38C
#define REG_QOF_CAM_A_QOF_MTC_ST_RMS_LSB_1          0x394
#define REG_QOF_CAM_A_QOF_MTC_ST_RMS_MSB2_1         0x398
#define F_QOF_CAM_A_MTCMOS_ST_RMS_MSB2_1_POS                         0
#define F_QOF_CAM_A_MTCMOS_ST_RMS_MSB2_1_WIDTH                       2

#define REG_QOF_CAM_A_QOF_MTC_ST_RAW_LSB_1          0x39C
#define REG_QOF_CAM_A_QOF_MTC_ST_RAW_MSB2_1         0x3A0
#define F_QOF_CAM_A_MTCMOS_ST_RAW_MSB2_1_POS                         0
#define F_QOF_CAM_A_MTCMOS_ST_RAW_MSB2_1_WIDTH                       2

/* following added manually */
#define QOF_SEQ_MODE_RTC_THEN_ITC                   0
#define F_QOF_CAM_TOP_QOF_SW_RST_RAWA_SW_RST_POS    6
#define F_QOF_CAM_TOP_QOF_SW_RST_RAWA_SW_RST_WIDTH    1
#define F_QOF_CAM_TOP_QOF_SW_RST_RAWB_SW_RST_POS    7
#define F_QOF_CAM_TOP_QOF_SW_RST_RAWB_SW_RST_WIDTH    1
#define F_QOF_CAM_TOP_QOF_SW_RST_RAWC_SW_RST_POS    8
#define F_QOF_CAM_TOP_QOF_SW_RST_RAWC_SW_RST_WIDTH    1

#endif	/* _MTK_CAM_QOF_REGS_H */
