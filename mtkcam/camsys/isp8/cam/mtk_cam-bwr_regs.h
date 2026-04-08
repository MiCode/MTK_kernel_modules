/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef _MTK_CAM_BWR_REGS_H
#define _MTK_CAM_BWR_REGS_H

#define FMASK(pos, width)	(((1 << (width)) - 1) << (pos))

static inline u32 _read_field(u32 val, int pos, int width)
{
	return (val & FMASK(pos, width)) >> pos;
}

static inline u32 _set_field(u32 *val, int pos, int width, u32 fval)
{
	u32 fmask = FMASK(pos, width);

	*val &= ~fmask;
	*val |= ((fval << pos) & fmask);
	return *val;
}

#define FBIT(field)	BIT(F_ ## field ## _POS)

#define READ_FIELD(val, field)	\
	_read_field(val, F_ ## field ## _POS, F_ ## field ## _WIDTH)

#define SET_FIELD(val, field, fval)	\
	_set_field(val, F_ ## field ## _POS, F_ ## field ## _WIDTH, fval)

/* baseaddr 0x3A00D000 */

/* module: BWR_CAM_E1A */
#define REG_BWR_CAM_RPT_CTRL                          0x0
#define F_BWR_CAM_RPT_RST_POS                                        2
#define F_BWR_CAM_RPT_RST_WIDTH                                      1
#define F_BWR_CAM_RPT_END_POS                                        1
#define F_BWR_CAM_RPT_END_WIDTH                                      1
#define F_BWR_CAM_RPT_START_POS                                      0
#define F_BWR_CAM_RPT_START_WIDTH                                    1

#define REG_BWR_CAM_RPT_TIMER                         0x4
#define REG_BWR_CAM_RPT_STATE                         0x8
#define F_BWR_CAM_RPT_STATE_POS                                      0
#define F_BWR_CAM_RPT_STATE_WIDTH                                    3

#define REG_BWR_CAM_MTCMOS_EN_VLD                     0xC
#define F_BWR_CAM_MTCMOS_EN_VLD_POS                                  0
#define F_BWR_CAM_MTCMOS_EN_VLD_WIDTH                                11

#define REG_BWR_CAM_DBC_CYC                          0x10
#define REG_BWR_CAM_DCM_DIS                          0x14
#define F_BWR_CAM_DCM_DIS_CORE_POS                                   1
#define F_BWR_CAM_DCM_DIS_CORE_WIDTH                                 1
#define F_BWR_CAM_DCM_DIS_CTRL_POS                                   0
#define F_BWR_CAM_DCM_DIS_CTRL_WIDTH                                 1

#define REG_BWR_CAM_DCM_ST                           0x18
#define F_BWR_CAM_DCM_ST_CORE_POS                                    1
#define F_BWR_CAM_DCM_ST_CORE_WIDTH                                  1
#define F_BWR_CAM_DCM_ST_CTRL_POS                                    0
#define F_BWR_CAM_DCM_ST_CTRL_WIDTH                                  1

#define REG_BWR_CAM_BW_TYPE                          0x20
#define F_BWR_CAM_DVFS_AVG_PEAK_POS                                  0
#define F_BWR_CAM_DVFS_AVG_PEAK_WIDTH                                1

#define REG_BWR_CAM_SRT_TTL_OCC_FACTOR               0x24
#define F_BWR_CAM_SRT_TTL_OCC_FACTOR_POS                             0
#define F_BWR_CAM_SRT_TTL_OCC_FACTOR_WIDTH                           10

#define REG_BWR_CAM_SRT_TTL_DVFS_FREQ                0x28
#define F_BWR_CAM_SRT_TTL_DVFS_FREQ_POS                              0
#define F_BWR_CAM_SRT_TTL_DVFS_FREQ_WIDTH                            10

#define REG_BWR_CAM_SRT_RW_OCC_FACTOR                0x2C
#define F_BWR_CAM_SRT_RW_OCC_FACTOR_POS                              0
#define F_BWR_CAM_SRT_RW_OCC_FACTOR_WIDTH                            10

#define REG_BWR_CAM_SRT_RW_DVFS_FREQ                 0x30
#define F_BWR_CAM_SRT_RW_DVFS_FREQ_POS                               0
#define F_BWR_CAM_SRT_RW_DVFS_FREQ_WIDTH                             10

#define REG_BWR_CAM_HRT_TTL_OCC_FACTOR               0x34
#define F_BWR_CAM_HRT_TTL_OCC_FACTOR_POS                             0
#define F_BWR_CAM_HRT_TTL_OCC_FACTOR_WIDTH                           10

#define REG_BWR_CAM_HRT_TTL_DVFS_FREQ                0x38
#define F_BWR_CAM_HRT_TTL_DVFS_FREQ_POS                              0
#define F_BWR_CAM_HRT_TTL_DVFS_FREQ_WIDTH                            10

#define REG_BWR_CAM_HRT_RW_OCC_FACTOR                0x3C
#define F_BWR_CAM_HRT_RW_OCC_FACTOR_POS                              0
#define F_BWR_CAM_HRT_RW_OCC_FACTOR_WIDTH                            10

#define REG_BWR_CAM_HRT_RW_DVFS_FREQ                 0x40
#define F_BWR_CAM_HRT_RW_DVFS_FREQ_POS                               0
#define F_BWR_CAM_HRT_RW_DVFS_FREQ_WIDTH                             10

#define REG_BWR_CAM_PROTOCOL_SET_EN                  0x44
#define F_BWR_CAM_MANUAL_SET_POS                                     0
#define F_BWR_CAM_MANUAL_SET_WIDTH                                   1

#define REG_BWR_CAM_PROTOCOL0                        0x48
#define F_BWR_CAM_CH_TTL3_POS                                        29
#define F_BWR_CAM_CH_TTL3_WIDTH                                      1
#define F_BWR_CAM_RD_WD3_POS                                         28
#define F_BWR_CAM_RD_WD3_WIDTH                                       1
#define F_BWR_CAM_SRT_HRT3_POS                                       27
#define F_BWR_CAM_SRT_HRT3_WIDTH                                     1
#define F_BWR_CAM_DRAM_SLB3_POS                                      26
#define F_BWR_CAM_DRAM_SLB3_WIDTH                                    1
#define F_BWR_CAM_AXI3_POS                                           24
#define F_BWR_CAM_AXI3_WIDTH                                         2
#define F_BWR_CAM_CH_TTL2_POS                                        21
#define F_BWR_CAM_CH_TTL2_WIDTH                                      1
#define F_BWR_CAM_RD_WD2_POS                                         20
#define F_BWR_CAM_RD_WD2_WIDTH                                       1
#define F_BWR_CAM_SRT_HRT2_POS                                       19
#define F_BWR_CAM_SRT_HRT2_WIDTH                                     1
#define F_BWR_CAM_DRAM_SLB2_POS                                      18
#define F_BWR_CAM_DRAM_SLB2_WIDTH                                    1
#define F_BWR_CAM_AXI2_POS                                           16
#define F_BWR_CAM_AXI2_WIDTH                                         2
#define F_BWR_CAM_CH_TTL1_POS                                        13
#define F_BWR_CAM_CH_TTL1_WIDTH                                      1
#define F_BWR_CAM_RD_WD1_POS                                         12
#define F_BWR_CAM_RD_WD1_WIDTH                                       1
#define F_BWR_CAM_SRT_HRT1_POS                                       11
#define F_BWR_CAM_SRT_HRT1_WIDTH                                     1
#define F_BWR_CAM_DRAM_SLB1_POS                                      10
#define F_BWR_CAM_DRAM_SLB1_WIDTH                                    1
#define F_BWR_CAM_AXI1_POS                                           8
#define F_BWR_CAM_AXI1_WIDTH                                         2
#define F_BWR_CAM_CH_TTL0_POS                                        5
#define F_BWR_CAM_CH_TTL0_WIDTH                                      1
#define F_BWR_CAM_RD_WD0_POS                                         4
#define F_BWR_CAM_RD_WD0_WIDTH                                       1
#define F_BWR_CAM_SRT_HRT0_POS                                       3
#define F_BWR_CAM_SRT_HRT0_WIDTH                                     1
#define F_BWR_CAM_DRAM_SLB0_POS                                      2
#define F_BWR_CAM_DRAM_SLB0_WIDTH                                    1
#define F_BWR_CAM_AXI0_POS                                           0
#define F_BWR_CAM_AXI0_WIDTH                                         2

#define REG_BWR_CAM_PROTOCOL1                        0x4C
#define F_BWR_CAM_CH_TTL7_POS                                        29
#define F_BWR_CAM_CH_TTL7_WIDTH                                      1
#define F_BWR_CAM_RD_WD7_POS                                         28
#define F_BWR_CAM_RD_WD7_WIDTH                                       1
#define F_BWR_CAM_SRT_HRT7_POS                                       27
#define F_BWR_CAM_SRT_HRT7_WIDTH                                     1
#define F_BWR_CAM_DRAM_SLB7_POS                                      26
#define F_BWR_CAM_DRAM_SLB7_WIDTH                                    1
#define F_BWR_CAM_AXI7_POS                                           24
#define F_BWR_CAM_AXI7_WIDTH                                         2
#define F_BWR_CAM_CH_TTL6_POS                                        21
#define F_BWR_CAM_CH_TTL6_WIDTH                                      1
#define F_BWR_CAM_RD_WD6_POS                                         20
#define F_BWR_CAM_RD_WD6_WIDTH                                       1
#define F_BWR_CAM_SRT_HRT6_POS                                       19
#define F_BWR_CAM_SRT_HRT6_WIDTH                                     1
#define F_BWR_CAM_DRAM_SLB6_POS                                      18
#define F_BWR_CAM_DRAM_SLB6_WIDTH                                    1
#define F_BWR_CAM_AXI6_POS                                           16
#define F_BWR_CAM_AXI6_WIDTH                                         2
#define F_BWR_CAM_CH_TTL5_POS                                        13
#define F_BWR_CAM_CH_TTL5_WIDTH                                      1
#define F_BWR_CAM_RD_WD5_POS                                         12
#define F_BWR_CAM_RD_WD5_WIDTH                                       1
#define F_BWR_CAM_SRT_HRT5_POS                                       11
#define F_BWR_CAM_SRT_HRT5_WIDTH                                     1
#define F_BWR_CAM_DRAM_SLB5_POS                                      10
#define F_BWR_CAM_DRAM_SLB5_WIDTH                                    1
#define F_BWR_CAM_AXI5_POS                                           8
#define F_BWR_CAM_AXI5_WIDTH                                         2
#define F_BWR_CAM_CH_TTL4_POS                                        5
#define F_BWR_CAM_CH_TTL4_WIDTH                                      1
#define F_BWR_CAM_RD_WD4_POS                                         4
#define F_BWR_CAM_RD_WD4_WIDTH                                       1
#define F_BWR_CAM_SRT_HRT4_POS                                       3
#define F_BWR_CAM_SRT_HRT4_WIDTH                                     1
#define F_BWR_CAM_DRAM_SLB4_POS                                      2
#define F_BWR_CAM_DRAM_SLB4_WIDTH                                    1
#define F_BWR_CAM_AXI4_POS                                           0
#define F_BWR_CAM_AXI4_WIDTH                                         2

#define REG_BWR_CAM_PROTOCOL2                        0x50
#define F_BWR_CAM_CH_TTL11_POS                                       29
#define F_BWR_CAM_CH_TTL11_WIDTH                                     1
#define F_BWR_CAM_RD_WD11_POS                                        28
#define F_BWR_CAM_RD_WD11_WIDTH                                      1
#define F_BWR_CAM_SRT_HRT11_POS                                      27
#define F_BWR_CAM_SRT_HRT11_WIDTH                                    1
#define F_BWR_CAM_DRAM_SLB11_POS                                     26
#define F_BWR_CAM_DRAM_SLB11_WIDTH                                   1
#define F_BWR_CAM_AXI11_POS                                          24
#define F_BWR_CAM_AXI11_WIDTH                                        2
#define F_BWR_CAM_CH_TTL10_POS                                       21
#define F_BWR_CAM_CH_TTL10_WIDTH                                     1
#define F_BWR_CAM_RD_WD10_POS                                        20
#define F_BWR_CAM_RD_WD10_WIDTH                                      1
#define F_BWR_CAM_SRT_HRT10_POS                                      19
#define F_BWR_CAM_SRT_HRT10_WIDTH                                    1
#define F_BWR_CAM_DRAM_SLB10_POS                                     18
#define F_BWR_CAM_DRAM_SLB10_WIDTH                                   1
#define F_BWR_CAM_AXI10_POS                                          16
#define F_BWR_CAM_AXI10_WIDTH                                        2
#define F_BWR_CAM_CH_TTL9_POS                                        13
#define F_BWR_CAM_CH_TTL9_WIDTH                                      1
#define F_BWR_CAM_RD_WD9_POS                                         12
#define F_BWR_CAM_RD_WD9_WIDTH                                       1
#define F_BWR_CAM_SRT_HRT9_POS                                       11
#define F_BWR_CAM_SRT_HRT9_WIDTH                                     1
#define F_BWR_CAM_DRAM_SLB9_POS                                      10
#define F_BWR_CAM_DRAM_SLB9_WIDTH                                    1
#define F_BWR_CAM_AXI9_POS                                           8
#define F_BWR_CAM_AXI9_WIDTH                                         2
#define F_BWR_CAM_CH_TTL8_POS                                        5
#define F_BWR_CAM_CH_TTL8_WIDTH                                      1
#define F_BWR_CAM_RD_WD8_POS                                         4
#define F_BWR_CAM_RD_WD8_WIDTH                                       1
#define F_BWR_CAM_SRT_HRT8_POS                                       3
#define F_BWR_CAM_SRT_HRT8_WIDTH                                     1
#define F_BWR_CAM_DRAM_SLB8_POS                                      2
#define F_BWR_CAM_DRAM_SLB8_WIDTH                                    1
#define F_BWR_CAM_AXI8_POS                                           0
#define F_BWR_CAM_AXI8_WIDTH                                         2

#define REG_BWR_CAM_PROTOCOL3                        0x54
#define F_BWR_CAM_CH_TTL15_POS                                       29
#define F_BWR_CAM_CH_TTL15_WIDTH                                     1
#define F_BWR_CAM_RD_WD15_POS                                        28
#define F_BWR_CAM_RD_WD15_WIDTH                                      1
#define F_BWR_CAM_SRT_HRT15_POS                                      27
#define F_BWR_CAM_SRT_HRT15_WIDTH                                    1
#define F_BWR_CAM_DRAM_SLB15_POS                                     26
#define F_BWR_CAM_DRAM_SLB15_WIDTH                                   1
#define F_BWR_CAM_AXI15_POS                                          24
#define F_BWR_CAM_AXI15_WIDTH                                        2
#define F_BWR_CAM_CH_TTL14_POS                                       21
#define F_BWR_CAM_CH_TTL14_WIDTH                                     1
#define F_BWR_CAM_RD_WD14_POS                                        20
#define F_BWR_CAM_RD_WD14_WIDTH                                      1
#define F_BWR_CAM_SRT_HRT14_POS                                      19
#define F_BWR_CAM_SRT_HRT14_WIDTH                                    1
#define F_BWR_CAM_DRAM_SLB14_POS                                     18
#define F_BWR_CAM_DRAM_SLB14_WIDTH                                   1
#define F_BWR_CAM_AXI14_POS                                          16
#define F_BWR_CAM_AXI14_WIDTH                                        2
#define F_BWR_CAM_CH_TTL13_POS                                       13
#define F_BWR_CAM_CH_TTL13_WIDTH                                     1
#define F_BWR_CAM_RD_WD13_POS                                        12
#define F_BWR_CAM_RD_WD13_WIDTH                                      1
#define F_BWR_CAM_SRT_HRT13_POS                                      11
#define F_BWR_CAM_SRT_HRT13_WIDTH                                    1
#define F_BWR_CAM_DRAM_SLB13_POS                                     10
#define F_BWR_CAM_DRAM_SLB13_WIDTH                                   1
#define F_BWR_CAM_AXI13_POS                                          8
#define F_BWR_CAM_AXI13_WIDTH                                        2
#define F_BWR_CAM_CH_TTL12_POS                                       5
#define F_BWR_CAM_CH_TTL12_WIDTH                                     1
#define F_BWR_CAM_RD_WD12_POS                                        4
#define F_BWR_CAM_RD_WD12_WIDTH                                      1
#define F_BWR_CAM_SRT_HRT12_POS                                      3
#define F_BWR_CAM_SRT_HRT12_WIDTH                                    1
#define F_BWR_CAM_DRAM_SLB12_POS                                     2
#define F_BWR_CAM_DRAM_SLB12_WIDTH                                   1
#define F_BWR_CAM_AXI12_POS                                          0
#define F_BWR_CAM_AXI12_WIDTH                                        2

#define REG_BWR_CAM_PROTOCOL4                        0x58
#define F_BWR_CAM_CH_TTL17_POS                                       13
#define F_BWR_CAM_CH_TTL17_WIDTH                                     1
#define F_BWR_CAM_RD_WD17_POS                                        12
#define F_BWR_CAM_RD_WD17_WIDTH                                      1
#define F_BWR_CAM_SRT_HRT17_POS                                      11
#define F_BWR_CAM_SRT_HRT17_WIDTH                                    1
#define F_BWR_CAM_DRAM_SLB17_POS                                     10
#define F_BWR_CAM_DRAM_SLB17_WIDTH                                   1
#define F_BWR_CAM_AXI17_POS                                          8
#define F_BWR_CAM_AXI17_WIDTH                                        2
#define F_BWR_CAM_CH_TTL16_POS                                       5
#define F_BWR_CAM_CH_TTL16_WIDTH                                     1
#define F_BWR_CAM_RD_WD16_POS                                        4
#define F_BWR_CAM_RD_WD16_WIDTH                                      1
#define F_BWR_CAM_SRT_HRT16_POS                                      3
#define F_BWR_CAM_SRT_HRT16_WIDTH                                    1
#define F_BWR_CAM_DRAM_SLB16_POS                                     2
#define F_BWR_CAM_DRAM_SLB16_WIDTH                                   1
#define F_BWR_CAM_AXI16_POS                                          0
#define F_BWR_CAM_AXI16_WIDTH                                        2

#define REG_BWR_CAM_SEND_BW_ZERO                     0x5C
#define F_BWR_CAM_SEND_BW_ZERO_POS                                   0
#define F_BWR_CAM_SEND_BW_ZERO_WIDTH                                 1

#define REG_BWR_CAM_SEND_VLD_ST                      0x60
#define F_BWR_CAM_SEND_VLD_ST_POS                                    0
#define F_BWR_CAM_SEND_VLD_ST_WIDTH                                  18

#define REG_BWR_CAM_SEND_DONE_ST                     0x64
#define F_BWR_CAM_SEND_DONE_ST_POS                                   0
#define F_BWR_CAM_SEND_DONE_ST_WIDTH                                 18

#define REG_BWR_CAM_DBG_SEL                          0x68
#define F_BWR_CAM_DBG_SEL_POS                                        0
#define F_BWR_CAM_DBG_SEL_WIDTH                                      5

#define REG_BWR_CAM_DBG_DATA                         0x6C
#define REG_BWR_CAM_SRT_TTL_BW_QOS_SEL               0x70
#define F_BWR_CAM_SRT_TTL_BW_QOS_SEL10_POS                           10
#define F_BWR_CAM_SRT_TTL_BW_QOS_SEL10_WIDTH                         1
#define F_BWR_CAM_SRT_TTL_BW_QOS_SEL9_POS                            9
#define F_BWR_CAM_SRT_TTL_BW_QOS_SEL9_WIDTH                          1
#define F_BWR_CAM_SRT_TTL_BW_QOS_SEL8_POS                            8
#define F_BWR_CAM_SRT_TTL_BW_QOS_SEL8_WIDTH                          1
#define F_BWR_CAM_SRT_TTL_BW_QOS_SEL7_POS                            7
#define F_BWR_CAM_SRT_TTL_BW_QOS_SEL7_WIDTH                          1
#define F_BWR_CAM_SRT_TTL_BW_QOS_SEL6_POS                            6
#define F_BWR_CAM_SRT_TTL_BW_QOS_SEL6_WIDTH                          1
#define F_BWR_CAM_SRT_TTL_BW_QOS_SEL5_POS                            5
#define F_BWR_CAM_SRT_TTL_BW_QOS_SEL5_WIDTH                          1
#define F_BWR_CAM_SRT_TTL_BW_QOS_SEL4_POS                            4
#define F_BWR_CAM_SRT_TTL_BW_QOS_SEL4_WIDTH                          1
#define F_BWR_CAM_SRT_TTL_BW_QOS_SEL3_POS                            3
#define F_BWR_CAM_SRT_TTL_BW_QOS_SEL3_WIDTH                          1
#define F_BWR_CAM_SRT_TTL_BW_QOS_SEL2_POS                            2
#define F_BWR_CAM_SRT_TTL_BW_QOS_SEL2_WIDTH                          1
#define F_BWR_CAM_SRT_TTL_BW_QOS_SEL1_POS                            1
#define F_BWR_CAM_SRT_TTL_BW_QOS_SEL1_WIDTH                          1
#define F_BWR_CAM_SRT_TTL_BW_QOS_SEL0_POS                            0
#define F_BWR_CAM_SRT_TTL_BW_QOS_SEL0_WIDTH                          1

#define REG_BWR_CAM_SRT_TTL_SW_QOS_TRIG              0x74
#define F_BWR_CAM_SRT_TTL_SW_QOS_TRIG10_POS                          10
#define F_BWR_CAM_SRT_TTL_SW_QOS_TRIG10_WIDTH                        1
#define F_BWR_CAM_SRT_TTL_SW_QOS_TRIG9_POS                           9
#define F_BWR_CAM_SRT_TTL_SW_QOS_TRIG9_WIDTH                         1
#define F_BWR_CAM_SRT_TTL_SW_QOS_TRIG8_POS                           8
#define F_BWR_CAM_SRT_TTL_SW_QOS_TRIG8_WIDTH                         1
#define F_BWR_CAM_SRT_TTL_SW_QOS_TRIG7_POS                           7
#define F_BWR_CAM_SRT_TTL_SW_QOS_TRIG7_WIDTH                         1
#define F_BWR_CAM_SRT_TTL_SW_QOS_TRIG6_POS                           6
#define F_BWR_CAM_SRT_TTL_SW_QOS_TRIG6_WIDTH                         1
#define F_BWR_CAM_SRT_TTL_SW_QOS_TRIG5_POS                           5
#define F_BWR_CAM_SRT_TTL_SW_QOS_TRIG5_WIDTH                         1
#define F_BWR_CAM_SRT_TTL_SW_QOS_TRIG4_POS                           4
#define F_BWR_CAM_SRT_TTL_SW_QOS_TRIG4_WIDTH                         1
#define F_BWR_CAM_SRT_TTL_SW_QOS_TRIG3_POS                           3
#define F_BWR_CAM_SRT_TTL_SW_QOS_TRIG3_WIDTH                         1
#define F_BWR_CAM_SRT_TTL_SW_QOS_TRIG2_POS                           2
#define F_BWR_CAM_SRT_TTL_SW_QOS_TRIG2_WIDTH                         1
#define F_BWR_CAM_SRT_TTL_SW_QOS_TRIG1_POS                           1
#define F_BWR_CAM_SRT_TTL_SW_QOS_TRIG1_WIDTH                         1
#define F_BWR_CAM_SRT_TTL_SW_QOS_TRIG0_POS                           0
#define F_BWR_CAM_SRT_TTL_SW_QOS_TRIG0_WIDTH                         1

#define REG_BWR_CAM_SRT_TTL_SW_QOS_EN                0x78
#define F_BWR_CAM_SRT_TTL_SW_QOS_EN10_POS                            10
#define F_BWR_CAM_SRT_TTL_SW_QOS_EN10_WIDTH                          1
#define F_BWR_CAM_SRT_TTL_SW_QOS_EN9_POS                             9
#define F_BWR_CAM_SRT_TTL_SW_QOS_EN9_WIDTH                           1
#define F_BWR_CAM_SRT_TTL_SW_QOS_EN8_POS                             8
#define F_BWR_CAM_SRT_TTL_SW_QOS_EN8_WIDTH                           1
#define F_BWR_CAM_SRT_TTL_SW_QOS_EN7_POS                             7
#define F_BWR_CAM_SRT_TTL_SW_QOS_EN7_WIDTH                           1
#define F_BWR_CAM_SRT_TTL_SW_QOS_EN6_POS                             6
#define F_BWR_CAM_SRT_TTL_SW_QOS_EN6_WIDTH                           1
#define F_BWR_CAM_SRT_TTL_SW_QOS_EN5_POS                             5
#define F_BWR_CAM_SRT_TTL_SW_QOS_EN5_WIDTH                           1
#define F_BWR_CAM_SRT_TTL_SW_QOS_EN4_POS                             4
#define F_BWR_CAM_SRT_TTL_SW_QOS_EN4_WIDTH                           1
#define F_BWR_CAM_SRT_TTL_SW_QOS_EN3_POS                             3
#define F_BWR_CAM_SRT_TTL_SW_QOS_EN3_WIDTH                           1
#define F_BWR_CAM_SRT_TTL_SW_QOS_EN2_POS                             2
#define F_BWR_CAM_SRT_TTL_SW_QOS_EN2_WIDTH                           1
#define F_BWR_CAM_SRT_TTL_SW_QOS_EN1_POS                             1
#define F_BWR_CAM_SRT_TTL_SW_QOS_EN1_WIDTH                           1
#define F_BWR_CAM_SRT_TTL_SW_QOS_EN0_POS                             0
#define F_BWR_CAM_SRT_TTL_SW_QOS_EN0_WIDTH                           1

#define REG_BWR_CAM_SRT_TTL_ENG_BW0                  0x7C
#define F_BWR_CAM_SRT_TTL_ENG_BW0_POS                                0
#define F_BWR_CAM_SRT_TTL_ENG_BW0_WIDTH                              18

#define REG_BWR_CAM_SRT_TTL_ENG_BW1                  0x80
#define F_BWR_CAM_SRT_TTL_ENG_BW1_POS                                0
#define F_BWR_CAM_SRT_TTL_ENG_BW1_WIDTH                              18

#define REG_BWR_CAM_SRT_TTL_ENG_BW2                  0x84
#define F_BWR_CAM_SRT_TTL_ENG_BW2_POS                                0
#define F_BWR_CAM_SRT_TTL_ENG_BW2_WIDTH                              18

#define REG_BWR_CAM_SRT_TTL_ENG_BW3                  0x88
#define F_BWR_CAM_SRT_TTL_ENG_BW3_POS                                0
#define F_BWR_CAM_SRT_TTL_ENG_BW3_WIDTH                              18

#define REG_BWR_CAM_SRT_TTL_ENG_BW4                  0x8C
#define F_BWR_CAM_SRT_TTL_ENG_BW4_POS                                0
#define F_BWR_CAM_SRT_TTL_ENG_BW4_WIDTH                              18

#define REG_BWR_CAM_SRT_TTL_ENG_BW5                  0x90
#define F_BWR_CAM_SRT_TTL_ENG_BW5_POS                                0
#define F_BWR_CAM_SRT_TTL_ENG_BW5_WIDTH                              18

#define REG_BWR_CAM_SRT_TTL_ENG_BW6                  0x94
#define F_BWR_CAM_SRT_TTL_ENG_BW6_POS                                0
#define F_BWR_CAM_SRT_TTL_ENG_BW6_WIDTH                              18

#define REG_BWR_CAM_SRT_TTL_ENG_BW7                  0x98
#define F_BWR_CAM_SRT_TTL_ENG_BW7_POS                                0
#define F_BWR_CAM_SRT_TTL_ENG_BW7_WIDTH                              18

#define REG_BWR_CAM_SRT_TTL_ENG_BW8                  0x9C
#define F_BWR_CAM_SRT_TTL_ENG_BW8_POS                                0
#define F_BWR_CAM_SRT_TTL_ENG_BW8_WIDTH                              18

#define REG_BWR_CAM_SRT_TTL_ENG_BW9                  0xA0
#define F_BWR_CAM_SRT_TTL_ENG_BW9_POS                                0
#define F_BWR_CAM_SRT_TTL_ENG_BW9_WIDTH                              18

#define REG_BWR_CAM_SRT_TTL_ENG_BW10                 0xA4
#define F_BWR_CAM_SRT_TTL_ENG_BW10_POS                               0
#define F_BWR_CAM_SRT_TTL_ENG_BW10_WIDTH                             18

#define REG_BWR_CAM_SRT_TTL_ENG_BW_RAT0              0xA8
#define F_BWR_CAM_SRT_TTL_ENG_BW_RAT0_POS                            0
#define F_BWR_CAM_SRT_TTL_ENG_BW_RAT0_WIDTH                          12

#define REG_BWR_CAM_SRT_TTL_ENG_BW_RAT1              0xAC
#define F_BWR_CAM_SRT_TTL_ENG_BW_RAT1_POS                            0
#define F_BWR_CAM_SRT_TTL_ENG_BW_RAT1_WIDTH                          12

#define REG_BWR_CAM_SRT_TTL_ENG_BW_RAT2              0xB0
#define F_BWR_CAM_SRT_TTL_ENG_BW_RAT2_POS                            0
#define F_BWR_CAM_SRT_TTL_ENG_BW_RAT2_WIDTH                          12

#define REG_BWR_CAM_SRT_TTL_ENG_BW_RAT3              0xB4
#define F_BWR_CAM_SRT_TTL_ENG_BW_RAT3_POS                            0
#define F_BWR_CAM_SRT_TTL_ENG_BW_RAT3_WIDTH                          12

#define REG_BWR_CAM_SRT_TTL_ENG_BW_RAT4              0xB8
#define F_BWR_CAM_SRT_TTL_ENG_BW_RAT4_POS                            0
#define F_BWR_CAM_SRT_TTL_ENG_BW_RAT4_WIDTH                          12

#define REG_BWR_CAM_SRT_TTL_ENG_BW_RAT5              0xBC
#define F_BWR_CAM_SRT_TTL_ENG_BW_RAT5_POS                            0
#define F_BWR_CAM_SRT_TTL_ENG_BW_RAT5_WIDTH                          12

#define REG_BWR_CAM_SRT_TTL_ENG_BW_RAT6              0xC0
#define F_BWR_CAM_SRT_TTL_ENG_BW_RAT6_POS                            0
#define F_BWR_CAM_SRT_TTL_ENG_BW_RAT6_WIDTH                          12

#define REG_BWR_CAM_SRT_TTL_ENG_BW_RAT7              0xC4
#define F_BWR_CAM_SRT_TTL_ENG_BW_RAT7_POS                            0
#define F_BWR_CAM_SRT_TTL_ENG_BW_RAT7_WIDTH                          12

#define REG_BWR_CAM_SRT_TTL_ENG_BW_RAT8              0xC8
#define F_BWR_CAM_SRT_TTL_ENG_BW_RAT8_POS                            0
#define F_BWR_CAM_SRT_TTL_ENG_BW_RAT8_WIDTH                          12

#define REG_BWR_CAM_SRT_TTL_ENG_BW_RAT9              0xCC
#define F_BWR_CAM_SRT_TTL_ENG_BW_RAT9_POS                            0
#define F_BWR_CAM_SRT_TTL_ENG_BW_RAT9_WIDTH                          12

#define REG_BWR_CAM_SRT_TTL_ENG_BW_RAT10             0xD0
#define F_BWR_CAM_SRT_TTL_ENG_BW_RAT10_POS                           0
#define F_BWR_CAM_SRT_TTL_ENG_BW_RAT10_WIDTH                         12

#define REG_BWR_CAM_SRT_R0_BW_QOS_SEL0               0xF0
#define F_BWR_CAM_SRT_R0_BW_QOS_SEL0_10_POS                          10
#define F_BWR_CAM_SRT_R0_BW_QOS_SEL0_10_WIDTH                        1
#define F_BWR_CAM_SRT_R0_BW_QOS_SEL0_9_POS                           9
#define F_BWR_CAM_SRT_R0_BW_QOS_SEL0_9_WIDTH                         1
#define F_BWR_CAM_SRT_R0_BW_QOS_SEL0_8_POS                           8
#define F_BWR_CAM_SRT_R0_BW_QOS_SEL0_8_WIDTH                         1
#define F_BWR_CAM_SRT_R0_BW_QOS_SEL0_7_POS                           7
#define F_BWR_CAM_SRT_R0_BW_QOS_SEL0_7_WIDTH                         1
#define F_BWR_CAM_SRT_R0_BW_QOS_SEL0_6_POS                           6
#define F_BWR_CAM_SRT_R0_BW_QOS_SEL0_6_WIDTH                         1
#define F_BWR_CAM_SRT_R0_BW_QOS_SEL0_5_POS                           5
#define F_BWR_CAM_SRT_R0_BW_QOS_SEL0_5_WIDTH                         1
#define F_BWR_CAM_SRT_R0_BW_QOS_SEL0_4_POS                           4
#define F_BWR_CAM_SRT_R0_BW_QOS_SEL0_4_WIDTH                         1
#define F_BWR_CAM_SRT_R0_BW_QOS_SEL0_3_POS                           3
#define F_BWR_CAM_SRT_R0_BW_QOS_SEL0_3_WIDTH                         1
#define F_BWR_CAM_SRT_R0_BW_QOS_SEL0_2_POS                           2
#define F_BWR_CAM_SRT_R0_BW_QOS_SEL0_2_WIDTH                         1
#define F_BWR_CAM_SRT_R0_BW_QOS_SEL0_1_POS                           1
#define F_BWR_CAM_SRT_R0_BW_QOS_SEL0_1_WIDTH                         1
#define F_BWR_CAM_SRT_R0_BW_QOS_SEL0_0_POS                           0
#define F_BWR_CAM_SRT_R0_BW_QOS_SEL0_0_WIDTH                         1

#define REG_BWR_CAM_SRT_R0_SW_QOS_TRIG0              0xF4
#define F_BWR_CAM_SRT_R0_SW_QOS_TRIG0_10_POS                         10
#define F_BWR_CAM_SRT_R0_SW_QOS_TRIG0_10_WIDTH                       1
#define F_BWR_CAM_SRT_R0_SW_QOS_TRIG0_9_POS                          9
#define F_BWR_CAM_SRT_R0_SW_QOS_TRIG0_9_WIDTH                        1
#define F_BWR_CAM_SRT_R0_SW_QOS_TRIG0_8_POS                          8
#define F_BWR_CAM_SRT_R0_SW_QOS_TRIG0_8_WIDTH                        1
#define F_BWR_CAM_SRT_R0_SW_QOS_TRIG0_7_POS                          7
#define F_BWR_CAM_SRT_R0_SW_QOS_TRIG0_7_WIDTH                        1
#define F_BWR_CAM_SRT_R0_SW_QOS_TRIG0_6_POS                          6
#define F_BWR_CAM_SRT_R0_SW_QOS_TRIG0_6_WIDTH                        1
#define F_BWR_CAM_SRT_R0_SW_QOS_TRIG0_5_POS                          5
#define F_BWR_CAM_SRT_R0_SW_QOS_TRIG0_5_WIDTH                        1
#define F_BWR_CAM_SRT_R0_SW_QOS_TRIG0_4_POS                          4
#define F_BWR_CAM_SRT_R0_SW_QOS_TRIG0_4_WIDTH                        1
#define F_BWR_CAM_SRT_R0_SW_QOS_TRIG0_3_POS                          3
#define F_BWR_CAM_SRT_R0_SW_QOS_TRIG0_3_WIDTH                        1
#define F_BWR_CAM_SRT_R0_SW_QOS_TRIG0_2_POS                          2
#define F_BWR_CAM_SRT_R0_SW_QOS_TRIG0_2_WIDTH                        1
#define F_BWR_CAM_SRT_R0_SW_QOS_TRIG0_1_POS                          1
#define F_BWR_CAM_SRT_R0_SW_QOS_TRIG0_1_WIDTH                        1
#define F_BWR_CAM_SRT_R0_SW_QOS_TRIG0_0_POS                          0
#define F_BWR_CAM_SRT_R0_SW_QOS_TRIG0_0_WIDTH                        1

#define REG_BWR_CAM_SRT_R0_SW_QOS_EN0                0xF8
#define F_BWR_CAM_SRT_R0_SW_QOS_EN0_10_POS                           10
#define F_BWR_CAM_SRT_R0_SW_QOS_EN0_10_WIDTH                         1
#define F_BWR_CAM_SRT_R0_SW_QOS_EN0_9_POS                            9
#define F_BWR_CAM_SRT_R0_SW_QOS_EN0_9_WIDTH                          1
#define F_BWR_CAM_SRT_R0_SW_QOS_EN0_8_POS                            8
#define F_BWR_CAM_SRT_R0_SW_QOS_EN0_8_WIDTH                          1
#define F_BWR_CAM_SRT_R0_SW_QOS_EN0_7_POS                            7
#define F_BWR_CAM_SRT_R0_SW_QOS_EN0_7_WIDTH                          1
#define F_BWR_CAM_SRT_R0_SW_QOS_EN0_6_POS                            6
#define F_BWR_CAM_SRT_R0_SW_QOS_EN0_6_WIDTH                          1
#define F_BWR_CAM_SRT_R0_SW_QOS_EN0_5_POS                            5
#define F_BWR_CAM_SRT_R0_SW_QOS_EN0_5_WIDTH                          1
#define F_BWR_CAM_SRT_R0_SW_QOS_EN0_4_POS                            4
#define F_BWR_CAM_SRT_R0_SW_QOS_EN0_4_WIDTH                          1
#define F_BWR_CAM_SRT_R0_SW_QOS_EN0_3_POS                            3
#define F_BWR_CAM_SRT_R0_SW_QOS_EN0_3_WIDTH                          1
#define F_BWR_CAM_SRT_R0_SW_QOS_EN0_2_POS                            2
#define F_BWR_CAM_SRT_R0_SW_QOS_EN0_2_WIDTH                          1
#define F_BWR_CAM_SRT_R0_SW_QOS_EN0_1_POS                            1
#define F_BWR_CAM_SRT_R0_SW_QOS_EN0_1_WIDTH                          1
#define F_BWR_CAM_SRT_R0_SW_QOS_EN0_0_POS                            0
#define F_BWR_CAM_SRT_R0_SW_QOS_EN0_0_WIDTH                          1

#define REG_BWR_CAM_SRT_R0_ENG_BW0_0                 0xFC
#define F_BWR_CAM_SRT_R0_ENG_BW0_0_POS                               0
#define F_BWR_CAM_SRT_R0_ENG_BW0_0_WIDTH                             18

#define REG_BWR_CAM_SRT_R0_ENG_BW0_1                0x100
#define F_BWR_CAM_SRT_R0_ENG_BW0_1_POS                               0
#define F_BWR_CAM_SRT_R0_ENG_BW0_1_WIDTH                             18

#define REG_BWR_CAM_SRT_R0_ENG_BW0_2                0x104
#define F_BWR_CAM_SRT_R0_ENG_BW0_2_POS                               0
#define F_BWR_CAM_SRT_R0_ENG_BW0_2_WIDTH                             18

#define REG_BWR_CAM_SRT_R0_ENG_BW0_3                0x108
#define F_BWR_CAM_SRT_R0_ENG_BW0_3_POS                               0
#define F_BWR_CAM_SRT_R0_ENG_BW0_3_WIDTH                             18

#define REG_BWR_CAM_SRT_R0_ENG_BW0_4                0x10C
#define F_BWR_CAM_SRT_R0_ENG_BW0_4_POS                               0
#define F_BWR_CAM_SRT_R0_ENG_BW0_4_WIDTH                             18

#define REG_BWR_CAM_SRT_R0_ENG_BW0_5                0x110
#define F_BWR_CAM_SRT_R0_ENG_BW0_5_POS                               0
#define F_BWR_CAM_SRT_R0_ENG_BW0_5_WIDTH                             18

#define REG_BWR_CAM_SRT_R0_ENG_BW0_6                0x114
#define F_BWR_CAM_SRT_R0_ENG_BW0_6_POS                               0
#define F_BWR_CAM_SRT_R0_ENG_BW0_6_WIDTH                             18

#define REG_BWR_CAM_SRT_R0_ENG_BW0_7                0x118
#define F_BWR_CAM_SRT_R0_ENG_BW0_7_POS                               0
#define F_BWR_CAM_SRT_R0_ENG_BW0_7_WIDTH                             18

#define REG_BWR_CAM_SRT_R0_ENG_BW0_8                0x11C
#define F_BWR_CAM_SRT_R0_ENG_BW0_8_POS                               0
#define F_BWR_CAM_SRT_R0_ENG_BW0_8_WIDTH                             18

#define REG_BWR_CAM_SRT_R0_ENG_BW0_9                0x120
#define F_BWR_CAM_SRT_R0_ENG_BW0_9_POS                               0
#define F_BWR_CAM_SRT_R0_ENG_BW0_9_WIDTH                             18

#define REG_BWR_CAM_SRT_R0_ENG_BW0_10               0x124
#define F_BWR_CAM_SRT_R0_ENG_BW0_10_POS                              0
#define F_BWR_CAM_SRT_R0_ENG_BW0_10_WIDTH                            18

#define REG_BWR_CAM_SRT_R0_ENG_BW_RAT0_0            0x128
#define F_BWR_CAM_SRT_R0_ENG_BW_RAT0_0_POS                           0
#define F_BWR_CAM_SRT_R0_ENG_BW_RAT0_0_WIDTH                         12

#define REG_BWR_CAM_SRT_R0_ENG_BW_RAT0_1            0x12C
#define F_BWR_CAM_SRT_R0_ENG_BW_RAT0_1_POS                           0
#define F_BWR_CAM_SRT_R0_ENG_BW_RAT0_1_WIDTH                         12

#define REG_BWR_CAM_SRT_R0_ENG_BW_RAT0_2            0x130
#define F_BWR_CAM_SRT_R0_ENG_BW_RAT0_2_POS                           0
#define F_BWR_CAM_SRT_R0_ENG_BW_RAT0_2_WIDTH                         12

#define REG_BWR_CAM_SRT_R0_ENG_BW_RAT0_3            0x134
#define F_BWR_CAM_SRT_R0_ENG_BW_RAT0_3_POS                           0
#define F_BWR_CAM_SRT_R0_ENG_BW_RAT0_3_WIDTH                         12

#define REG_BWR_CAM_SRT_R0_ENG_BW_RAT0_4            0x138
#define F_BWR_CAM_SRT_R0_ENG_BW_RAT0_4_POS                           0
#define F_BWR_CAM_SRT_R0_ENG_BW_RAT0_4_WIDTH                         12

#define REG_BWR_CAM_SRT_R0_ENG_BW_RAT0_5            0x13C
#define F_BWR_CAM_SRT_R0_ENG_BW_RAT0_5_POS                           0
#define F_BWR_CAM_SRT_R0_ENG_BW_RAT0_5_WIDTH                         12

#define REG_BWR_CAM_SRT_R0_ENG_BW_RAT0_6            0x140
#define F_BWR_CAM_SRT_R0_ENG_BW_RAT0_6_POS                           0
#define F_BWR_CAM_SRT_R0_ENG_BW_RAT0_6_WIDTH                         12

#define REG_BWR_CAM_SRT_R0_ENG_BW_RAT0_7            0x144
#define F_BWR_CAM_SRT_R0_ENG_BW_RAT0_7_POS                           0
#define F_BWR_CAM_SRT_R0_ENG_BW_RAT0_7_WIDTH                         12

#define REG_BWR_CAM_SRT_R0_ENG_BW_RAT0_8            0x148
#define F_BWR_CAM_SRT_R0_ENG_BW_RAT0_8_POS                           0
#define F_BWR_CAM_SRT_R0_ENG_BW_RAT0_8_WIDTH                         12

#define REG_BWR_CAM_SRT_R0_ENG_BW_RAT0_9            0x14C
#define F_BWR_CAM_SRT_R0_ENG_BW_RAT0_9_POS                           0
#define F_BWR_CAM_SRT_R0_ENG_BW_RAT0_9_WIDTH                         12

#define REG_BWR_CAM_SRT_R0_ENG_BW_RAT0_10           0x150
#define F_BWR_CAM_SRT_R0_ENG_BW_RAT0_10_POS                          0
#define F_BWR_CAM_SRT_R0_ENG_BW_RAT0_10_WIDTH                        12

#define REG_BWR_CAM_SRT_R1_BW_QOS_SEL0              0x170
#define F_BWR_CAM_SRT_R1_BW_QOS_SEL0_10_POS                          10
#define F_BWR_CAM_SRT_R1_BW_QOS_SEL0_10_WIDTH                        1
#define F_BWR_CAM_SRT_R1_BW_QOS_SEL0_9_POS                           9
#define F_BWR_CAM_SRT_R1_BW_QOS_SEL0_9_WIDTH                         1
#define F_BWR_CAM_SRT_R1_BW_QOS_SEL0_8_POS                           8
#define F_BWR_CAM_SRT_R1_BW_QOS_SEL0_8_WIDTH                         1
#define F_BWR_CAM_SRT_R1_BW_QOS_SEL0_7_POS                           7
#define F_BWR_CAM_SRT_R1_BW_QOS_SEL0_7_WIDTH                         1
#define F_BWR_CAM_SRT_R1_BW_QOS_SEL0_6_POS                           6
#define F_BWR_CAM_SRT_R1_BW_QOS_SEL0_6_WIDTH                         1
#define F_BWR_CAM_SRT_R1_BW_QOS_SEL0_5_POS                           5
#define F_BWR_CAM_SRT_R1_BW_QOS_SEL0_5_WIDTH                         1
#define F_BWR_CAM_SRT_R1_BW_QOS_SEL0_4_POS                           4
#define F_BWR_CAM_SRT_R1_BW_QOS_SEL0_4_WIDTH                         1
#define F_BWR_CAM_SRT_R1_BW_QOS_SEL0_3_POS                           3
#define F_BWR_CAM_SRT_R1_BW_QOS_SEL0_3_WIDTH                         1
#define F_BWR_CAM_SRT_R1_BW_QOS_SEL0_2_POS                           2
#define F_BWR_CAM_SRT_R1_BW_QOS_SEL0_2_WIDTH                         1
#define F_BWR_CAM_SRT_R1_BW_QOS_SEL0_1_POS                           1
#define F_BWR_CAM_SRT_R1_BW_QOS_SEL0_1_WIDTH                         1
#define F_BWR_CAM_SRT_R1_BW_QOS_SEL0_0_POS                           0
#define F_BWR_CAM_SRT_R1_BW_QOS_SEL0_0_WIDTH                         1

#define REG_BWR_CAM_SRT_R1_SW_QOS_TRIG0             0x174
#define F_BWR_CAM_SRT_R1_SW_QOS_TRIG0_10_POS                         10
#define F_BWR_CAM_SRT_R1_SW_QOS_TRIG0_10_WIDTH                       1
#define F_BWR_CAM_SRT_R1_SW_QOS_TRIG0_9_POS                          9
#define F_BWR_CAM_SRT_R1_SW_QOS_TRIG0_9_WIDTH                        1
#define F_BWR_CAM_SRT_R1_SW_QOS_TRIG0_8_POS                          8
#define F_BWR_CAM_SRT_R1_SW_QOS_TRIG0_8_WIDTH                        1
#define F_BWR_CAM_SRT_R1_SW_QOS_TRIG0_7_POS                          7
#define F_BWR_CAM_SRT_R1_SW_QOS_TRIG0_7_WIDTH                        1
#define F_BWR_CAM_SRT_R1_SW_QOS_TRIG0_6_POS                          6
#define F_BWR_CAM_SRT_R1_SW_QOS_TRIG0_6_WIDTH                        1
#define F_BWR_CAM_SRT_R1_SW_QOS_TRIG0_5_POS                          5
#define F_BWR_CAM_SRT_R1_SW_QOS_TRIG0_5_WIDTH                        1
#define F_BWR_CAM_SRT_R1_SW_QOS_TRIG0_4_POS                          4
#define F_BWR_CAM_SRT_R1_SW_QOS_TRIG0_4_WIDTH                        1
#define F_BWR_CAM_SRT_R1_SW_QOS_TRIG0_3_POS                          3
#define F_BWR_CAM_SRT_R1_SW_QOS_TRIG0_3_WIDTH                        1
#define F_BWR_CAM_SRT_R1_SW_QOS_TRIG0_2_POS                          2
#define F_BWR_CAM_SRT_R1_SW_QOS_TRIG0_2_WIDTH                        1
#define F_BWR_CAM_SRT_R1_SW_QOS_TRIG0_1_POS                          1
#define F_BWR_CAM_SRT_R1_SW_QOS_TRIG0_1_WIDTH                        1
#define F_BWR_CAM_SRT_R1_SW_QOS_TRIG0_0_POS                          0
#define F_BWR_CAM_SRT_R1_SW_QOS_TRIG0_0_WIDTH                        1

#define REG_BWR_CAM_SRT_R1_SW_QOS_EN0               0x178
#define F_BWR_CAM_SRT_R1_SW_QOS_EN0_10_POS                           10
#define F_BWR_CAM_SRT_R1_SW_QOS_EN0_10_WIDTH                         1
#define F_BWR_CAM_SRT_R1_SW_QOS_EN0_9_POS                            9
#define F_BWR_CAM_SRT_R1_SW_QOS_EN0_9_WIDTH                          1
#define F_BWR_CAM_SRT_R1_SW_QOS_EN0_8_POS                            8
#define F_BWR_CAM_SRT_R1_SW_QOS_EN0_8_WIDTH                          1
#define F_BWR_CAM_SRT_R1_SW_QOS_EN0_7_POS                            7
#define F_BWR_CAM_SRT_R1_SW_QOS_EN0_7_WIDTH                          1
#define F_BWR_CAM_SRT_R1_SW_QOS_EN0_6_POS                            6
#define F_BWR_CAM_SRT_R1_SW_QOS_EN0_6_WIDTH                          1
#define F_BWR_CAM_SRT_R1_SW_QOS_EN0_5_POS                            5
#define F_BWR_CAM_SRT_R1_SW_QOS_EN0_5_WIDTH                          1
#define F_BWR_CAM_SRT_R1_SW_QOS_EN0_4_POS                            4
#define F_BWR_CAM_SRT_R1_SW_QOS_EN0_4_WIDTH                          1
#define F_BWR_CAM_SRT_R1_SW_QOS_EN0_3_POS                            3
#define F_BWR_CAM_SRT_R1_SW_QOS_EN0_3_WIDTH                          1
#define F_BWR_CAM_SRT_R1_SW_QOS_EN0_2_POS                            2
#define F_BWR_CAM_SRT_R1_SW_QOS_EN0_2_WIDTH                          1
#define F_BWR_CAM_SRT_R1_SW_QOS_EN0_1_POS                            1
#define F_BWR_CAM_SRT_R1_SW_QOS_EN0_1_WIDTH                          1
#define F_BWR_CAM_SRT_R1_SW_QOS_EN0_0_POS                            0
#define F_BWR_CAM_SRT_R1_SW_QOS_EN0_0_WIDTH                          1

#define REG_BWR_CAM_SRT_R1_ENG_BW0_0                0x17C
#define F_BWR_CAM_SRT_R1_ENG_BW0_0_POS                               0
#define F_BWR_CAM_SRT_R1_ENG_BW0_0_WIDTH                             18

#define REG_BWR_CAM_SRT_R1_ENG_BW0_1                0x180
#define F_BWR_CAM_SRT_R1_ENG_BW0_1_POS                               0
#define F_BWR_CAM_SRT_R1_ENG_BW0_1_WIDTH                             18

#define REG_BWR_CAM_SRT_R1_ENG_BW0_2                0x184
#define F_BWR_CAM_SRT_R1_ENG_BW0_2_POS                               0
#define F_BWR_CAM_SRT_R1_ENG_BW0_2_WIDTH                             18

#define REG_BWR_CAM_SRT_R1_ENG_BW0_3                0x188
#define F_BWR_CAM_SRT_R1_ENG_BW0_3_POS                               0
#define F_BWR_CAM_SRT_R1_ENG_BW0_3_WIDTH                             18

#define REG_BWR_CAM_SRT_R1_ENG_BW0_4                0x18C
#define F_BWR_CAM_SRT_R1_ENG_BW0_4_POS                               0
#define F_BWR_CAM_SRT_R1_ENG_BW0_4_WIDTH                             18

#define REG_BWR_CAM_SRT_R1_ENG_BW0_5                0x190
#define F_BWR_CAM_SRT_R1_ENG_BW0_5_POS                               0
#define F_BWR_CAM_SRT_R1_ENG_BW0_5_WIDTH                             18

#define REG_BWR_CAM_SRT_R1_ENG_BW0_6                0x194
#define F_BWR_CAM_SRT_R1_ENG_BW0_6_POS                               0
#define F_BWR_CAM_SRT_R1_ENG_BW0_6_WIDTH                             18

#define REG_BWR_CAM_SRT_R1_ENG_BW0_7                0x198
#define F_BWR_CAM_SRT_R1_ENG_BW0_7_POS                               0
#define F_BWR_CAM_SRT_R1_ENG_BW0_7_WIDTH                             18

#define REG_BWR_CAM_SRT_R1_ENG_BW0_8                0x19C
#define F_BWR_CAM_SRT_R1_ENG_BW0_8_POS                               0
#define F_BWR_CAM_SRT_R1_ENG_BW0_8_WIDTH                             18

#define REG_BWR_CAM_SRT_R1_ENG_BW0_9                0x1A0
#define F_BWR_CAM_SRT_R1_ENG_BW0_9_POS                               0
#define F_BWR_CAM_SRT_R1_ENG_BW0_9_WIDTH                             18

#define REG_BWR_CAM_SRT_R1_ENG_BW0_10               0x1A4
#define F_BWR_CAM_SRT_R1_ENG_BW0_10_POS                              0
#define F_BWR_CAM_SRT_R1_ENG_BW0_10_WIDTH                            18

#define REG_BWR_CAM_SRT_R1_ENG_BW_RAT0_0            0x1A8
#define F_BWR_CAM_SRT_R1_ENG_BW_RAT0_0_POS                           0
#define F_BWR_CAM_SRT_R1_ENG_BW_RAT0_0_WIDTH                         12

#define REG_BWR_CAM_SRT_R1_ENG_BW_RAT0_1            0x1AC
#define F_BWR_CAM_SRT_R1_ENG_BW_RAT0_1_POS                           0
#define F_BWR_CAM_SRT_R1_ENG_BW_RAT0_1_WIDTH                         12

#define REG_BWR_CAM_SRT_R1_ENG_BW_RAT0_2            0x1B0
#define F_BWR_CAM_SRT_R1_ENG_BW_RAT0_2_POS                           0
#define F_BWR_CAM_SRT_R1_ENG_BW_RAT0_2_WIDTH                         12

#define REG_BWR_CAM_SRT_R1_ENG_BW_RAT0_3            0x1B4
#define F_BWR_CAM_SRT_R1_ENG_BW_RAT0_3_POS                           0
#define F_BWR_CAM_SRT_R1_ENG_BW_RAT0_3_WIDTH                         12

#define REG_BWR_CAM_SRT_R1_ENG_BW_RAT0_4            0x1B8
#define F_BWR_CAM_SRT_R1_ENG_BW_RAT0_4_POS                           0
#define F_BWR_CAM_SRT_R1_ENG_BW_RAT0_4_WIDTH                         12

#define REG_BWR_CAM_SRT_R1_ENG_BW_RAT0_5            0x1BC
#define F_BWR_CAM_SRT_R1_ENG_BW_RAT0_5_POS                           0
#define F_BWR_CAM_SRT_R1_ENG_BW_RAT0_5_WIDTH                         12

#define REG_BWR_CAM_SRT_R1_ENG_BW_RAT0_6            0x1C0
#define F_BWR_CAM_SRT_R1_ENG_BW_RAT0_6_POS                           0
#define F_BWR_CAM_SRT_R1_ENG_BW_RAT0_6_WIDTH                         12

#define REG_BWR_CAM_SRT_R1_ENG_BW_RAT0_7            0x1C4
#define F_BWR_CAM_SRT_R1_ENG_BW_RAT0_7_POS                           0
#define F_BWR_CAM_SRT_R1_ENG_BW_RAT0_7_WIDTH                         12

#define REG_BWR_CAM_SRT_R1_ENG_BW_RAT0_8            0x1C8
#define F_BWR_CAM_SRT_R1_ENG_BW_RAT0_8_POS                           0
#define F_BWR_CAM_SRT_R1_ENG_BW_RAT0_8_WIDTH                         12

#define REG_BWR_CAM_SRT_R1_ENG_BW_RAT0_9            0x1CC
#define F_BWR_CAM_SRT_R1_ENG_BW_RAT0_9_POS                           0
#define F_BWR_CAM_SRT_R1_ENG_BW_RAT0_9_WIDTH                         12

#define REG_BWR_CAM_SRT_R1_ENG_BW_RAT0_10           0x1D0
#define F_BWR_CAM_SRT_R1_ENG_BW_RAT0_10_POS                          0
#define F_BWR_CAM_SRT_R1_ENG_BW_RAT0_10_WIDTH                        12

#define REG_BWR_CAM_SRT_R2_BW_QOS_SEL0              0x1F0
#define F_BWR_CAM_SRT_R2_BW_QOS_SEL0_10_POS                          10
#define F_BWR_CAM_SRT_R2_BW_QOS_SEL0_10_WIDTH                        1
#define F_BWR_CAM_SRT_R2_BW_QOS_SEL0_9_POS                           9
#define F_BWR_CAM_SRT_R2_BW_QOS_SEL0_9_WIDTH                         1
#define F_BWR_CAM_SRT_R2_BW_QOS_SEL0_8_POS                           8
#define F_BWR_CAM_SRT_R2_BW_QOS_SEL0_8_WIDTH                         1
#define F_BWR_CAM_SRT_R2_BW_QOS_SEL0_7_POS                           7
#define F_BWR_CAM_SRT_R2_BW_QOS_SEL0_7_WIDTH                         1
#define F_BWR_CAM_SRT_R2_BW_QOS_SEL0_6_POS                           6
#define F_BWR_CAM_SRT_R2_BW_QOS_SEL0_6_WIDTH                         1
#define F_BWR_CAM_SRT_R2_BW_QOS_SEL0_5_POS                           5
#define F_BWR_CAM_SRT_R2_BW_QOS_SEL0_5_WIDTH                         1
#define F_BWR_CAM_SRT_R2_BW_QOS_SEL0_4_POS                           4
#define F_BWR_CAM_SRT_R2_BW_QOS_SEL0_4_WIDTH                         1
#define F_BWR_CAM_SRT_R2_BW_QOS_SEL0_3_POS                           3
#define F_BWR_CAM_SRT_R2_BW_QOS_SEL0_3_WIDTH                         1
#define F_BWR_CAM_SRT_R2_BW_QOS_SEL0_2_POS                           2
#define F_BWR_CAM_SRT_R2_BW_QOS_SEL0_2_WIDTH                         1
#define F_BWR_CAM_SRT_R2_BW_QOS_SEL0_1_POS                           1
#define F_BWR_CAM_SRT_R2_BW_QOS_SEL0_1_WIDTH                         1
#define F_BWR_CAM_SRT_R2_BW_QOS_SEL0_0_POS                           0
#define F_BWR_CAM_SRT_R2_BW_QOS_SEL0_0_WIDTH                         1

#define REG_BWR_CAM_SRT_R2_SW_QOS_TRIG0             0x1F4
#define F_BWR_CAM_SRT_R2_SW_QOS_TRIG0_10_POS                         10
#define F_BWR_CAM_SRT_R2_SW_QOS_TRIG0_10_WIDTH                       1
#define F_BWR_CAM_SRT_R2_SW_QOS_TRIG0_9_POS                          9
#define F_BWR_CAM_SRT_R2_SW_QOS_TRIG0_9_WIDTH                        1
#define F_BWR_CAM_SRT_R2_SW_QOS_TRIG0_8_POS                          8
#define F_BWR_CAM_SRT_R2_SW_QOS_TRIG0_8_WIDTH                        1
#define F_BWR_CAM_SRT_R2_SW_QOS_TRIG0_7_POS                          7
#define F_BWR_CAM_SRT_R2_SW_QOS_TRIG0_7_WIDTH                        1
#define F_BWR_CAM_SRT_R2_SW_QOS_TRIG0_6_POS                          6
#define F_BWR_CAM_SRT_R2_SW_QOS_TRIG0_6_WIDTH                        1
#define F_BWR_CAM_SRT_R2_SW_QOS_TRIG0_5_POS                          5
#define F_BWR_CAM_SRT_R2_SW_QOS_TRIG0_5_WIDTH                        1
#define F_BWR_CAM_SRT_R2_SW_QOS_TRIG0_4_POS                          4
#define F_BWR_CAM_SRT_R2_SW_QOS_TRIG0_4_WIDTH                        1
#define F_BWR_CAM_SRT_R2_SW_QOS_TRIG0_3_POS                          3
#define F_BWR_CAM_SRT_R2_SW_QOS_TRIG0_3_WIDTH                        1
#define F_BWR_CAM_SRT_R2_SW_QOS_TRIG0_2_POS                          2
#define F_BWR_CAM_SRT_R2_SW_QOS_TRIG0_2_WIDTH                        1
#define F_BWR_CAM_SRT_R2_SW_QOS_TRIG0_1_POS                          1
#define F_BWR_CAM_SRT_R2_SW_QOS_TRIG0_1_WIDTH                        1
#define F_BWR_CAM_SRT_R2_SW_QOS_TRIG0_0_POS                          0
#define F_BWR_CAM_SRT_R2_SW_QOS_TRIG0_0_WIDTH                        1

#define REG_BWR_CAM_SRT_R2_SW_QOS_EN0               0x1F8
#define F_BWR_CAM_SRT_R2_SW_QOS_EN0_10_POS                           10
#define F_BWR_CAM_SRT_R2_SW_QOS_EN0_10_WIDTH                         1
#define F_BWR_CAM_SRT_R2_SW_QOS_EN0_9_POS                            9
#define F_BWR_CAM_SRT_R2_SW_QOS_EN0_9_WIDTH                          1
#define F_BWR_CAM_SRT_R2_SW_QOS_EN0_8_POS                            8
#define F_BWR_CAM_SRT_R2_SW_QOS_EN0_8_WIDTH                          1
#define F_BWR_CAM_SRT_R2_SW_QOS_EN0_7_POS                            7
#define F_BWR_CAM_SRT_R2_SW_QOS_EN0_7_WIDTH                          1
#define F_BWR_CAM_SRT_R2_SW_QOS_EN0_6_POS                            6
#define F_BWR_CAM_SRT_R2_SW_QOS_EN0_6_WIDTH                          1
#define F_BWR_CAM_SRT_R2_SW_QOS_EN0_5_POS                            5
#define F_BWR_CAM_SRT_R2_SW_QOS_EN0_5_WIDTH                          1
#define F_BWR_CAM_SRT_R2_SW_QOS_EN0_4_POS                            4
#define F_BWR_CAM_SRT_R2_SW_QOS_EN0_4_WIDTH                          1
#define F_BWR_CAM_SRT_R2_SW_QOS_EN0_3_POS                            3
#define F_BWR_CAM_SRT_R2_SW_QOS_EN0_3_WIDTH                          1
#define F_BWR_CAM_SRT_R2_SW_QOS_EN0_2_POS                            2
#define F_BWR_CAM_SRT_R2_SW_QOS_EN0_2_WIDTH                          1
#define F_BWR_CAM_SRT_R2_SW_QOS_EN0_1_POS                            1
#define F_BWR_CAM_SRT_R2_SW_QOS_EN0_1_WIDTH                          1
#define F_BWR_CAM_SRT_R2_SW_QOS_EN0_0_POS                            0
#define F_BWR_CAM_SRT_R2_SW_QOS_EN0_0_WIDTH                          1

#define REG_BWR_CAM_SRT_R2_ENG_BW0_0                0x1FC
#define F_BWR_CAM_SRT_R2_ENG_BW0_0_POS                               0
#define F_BWR_CAM_SRT_R2_ENG_BW0_0_WIDTH                             18

#define REG_BWR_CAM_SRT_R2_ENG_BW0_1                0x200
#define F_BWR_CAM_SRT_R2_ENG_BW0_1_POS                               0
#define F_BWR_CAM_SRT_R2_ENG_BW0_1_WIDTH                             18

#define REG_BWR_CAM_SRT_R2_ENG_BW0_2                0x204
#define F_BWR_CAM_SRT_R2_ENG_BW0_2_POS                               0
#define F_BWR_CAM_SRT_R2_ENG_BW0_2_WIDTH                             18

#define REG_BWR_CAM_SRT_R2_ENG_BW0_3                0x208
#define F_BWR_CAM_SRT_R2_ENG_BW0_3_POS                               0
#define F_BWR_CAM_SRT_R2_ENG_BW0_3_WIDTH                             18

#define REG_BWR_CAM_SRT_R2_ENG_BW0_4                0x20C
#define F_BWR_CAM_SRT_R2_ENG_BW0_4_POS                               0
#define F_BWR_CAM_SRT_R2_ENG_BW0_4_WIDTH                             18

#define REG_BWR_CAM_SRT_R2_ENG_BW0_5                0x210
#define F_BWR_CAM_SRT_R2_ENG_BW0_5_POS                               0
#define F_BWR_CAM_SRT_R2_ENG_BW0_5_WIDTH                             18

#define REG_BWR_CAM_SRT_R2_ENG_BW0_6                0x214
#define F_BWR_CAM_SRT_R2_ENG_BW0_6_POS                               0
#define F_BWR_CAM_SRT_R2_ENG_BW0_6_WIDTH                             18

#define REG_BWR_CAM_SRT_R2_ENG_BW0_7                0x218
#define F_BWR_CAM_SRT_R2_ENG_BW0_7_POS                               0
#define F_BWR_CAM_SRT_R2_ENG_BW0_7_WIDTH                             18

#define REG_BWR_CAM_SRT_R2_ENG_BW0_8                0x21C
#define F_BWR_CAM_SRT_R2_ENG_BW0_8_POS                               0
#define F_BWR_CAM_SRT_R2_ENG_BW0_8_WIDTH                             18

#define REG_BWR_CAM_SRT_R2_ENG_BW0_9                0x220
#define F_BWR_CAM_SRT_R2_ENG_BW0_9_POS                               0
#define F_BWR_CAM_SRT_R2_ENG_BW0_9_WIDTH                             18

#define REG_BWR_CAM_SRT_R2_ENG_BW0_10               0x224
#define F_BWR_CAM_SRT_R2_ENG_BW0_10_POS                              0
#define F_BWR_CAM_SRT_R2_ENG_BW0_10_WIDTH                            18

#define REG_BWR_CAM_SRT_R2_ENG_BW_RAT0_0            0x228
#define F_BWR_CAM_SRT_R2_ENG_BW_RAT0_0_POS                           0
#define F_BWR_CAM_SRT_R2_ENG_BW_RAT0_0_WIDTH                         12

#define REG_BWR_CAM_SRT_R2_ENG_BW_RAT0_1            0x22C
#define F_BWR_CAM_SRT_R2_ENG_BW_RAT0_1_POS                           0
#define F_BWR_CAM_SRT_R2_ENG_BW_RAT0_1_WIDTH                         12

#define REG_BWR_CAM_SRT_R2_ENG_BW_RAT0_2            0x230
#define F_BWR_CAM_SRT_R2_ENG_BW_RAT0_2_POS                           0
#define F_BWR_CAM_SRT_R2_ENG_BW_RAT0_2_WIDTH                         12

#define REG_BWR_CAM_SRT_R2_ENG_BW_RAT0_3            0x234
#define F_BWR_CAM_SRT_R2_ENG_BW_RAT0_3_POS                           0
#define F_BWR_CAM_SRT_R2_ENG_BW_RAT0_3_WIDTH                         12

#define REG_BWR_CAM_SRT_R2_ENG_BW_RAT0_4            0x238
#define F_BWR_CAM_SRT_R2_ENG_BW_RAT0_4_POS                           0
#define F_BWR_CAM_SRT_R2_ENG_BW_RAT0_4_WIDTH                         12

#define REG_BWR_CAM_SRT_R2_ENG_BW_RAT0_5            0x23C
#define F_BWR_CAM_SRT_R2_ENG_BW_RAT0_5_POS                           0
#define F_BWR_CAM_SRT_R2_ENG_BW_RAT0_5_WIDTH                         12

#define REG_BWR_CAM_SRT_R2_ENG_BW_RAT0_6            0x240
#define F_BWR_CAM_SRT_R2_ENG_BW_RAT0_6_POS                           0
#define F_BWR_CAM_SRT_R2_ENG_BW_RAT0_6_WIDTH                         12

#define REG_BWR_CAM_SRT_R2_ENG_BW_RAT0_7            0x244
#define F_BWR_CAM_SRT_R2_ENG_BW_RAT0_7_POS                           0
#define F_BWR_CAM_SRT_R2_ENG_BW_RAT0_7_WIDTH                         12

#define REG_BWR_CAM_SRT_R2_ENG_BW_RAT0_8            0x248
#define F_BWR_CAM_SRT_R2_ENG_BW_RAT0_8_POS                           0
#define F_BWR_CAM_SRT_R2_ENG_BW_RAT0_8_WIDTH                         12

#define REG_BWR_CAM_SRT_R2_ENG_BW_RAT0_9            0x24C
#define F_BWR_CAM_SRT_R2_ENG_BW_RAT0_9_POS                           0
#define F_BWR_CAM_SRT_R2_ENG_BW_RAT0_9_WIDTH                         12

#define REG_BWR_CAM_SRT_R2_ENG_BW_RAT0_10           0x250
#define F_BWR_CAM_SRT_R2_ENG_BW_RAT0_10_POS                          0
#define F_BWR_CAM_SRT_R2_ENG_BW_RAT0_10_WIDTH                        12

#define REG_BWR_CAM_SRT_R3_BW_QOS_SEL0              0x270
#define F_BWR_CAM_SRT_R3_BW_QOS_SEL0_10_POS                          10
#define F_BWR_CAM_SRT_R3_BW_QOS_SEL0_10_WIDTH                        1
#define F_BWR_CAM_SRT_R3_BW_QOS_SEL0_9_POS                           9
#define F_BWR_CAM_SRT_R3_BW_QOS_SEL0_9_WIDTH                         1
#define F_BWR_CAM_SRT_R3_BW_QOS_SEL0_8_POS                           8
#define F_BWR_CAM_SRT_R3_BW_QOS_SEL0_8_WIDTH                         1
#define F_BWR_CAM_SRT_R3_BW_QOS_SEL0_7_POS                           7
#define F_BWR_CAM_SRT_R3_BW_QOS_SEL0_7_WIDTH                         1
#define F_BWR_CAM_SRT_R3_BW_QOS_SEL0_6_POS                           6
#define F_BWR_CAM_SRT_R3_BW_QOS_SEL0_6_WIDTH                         1
#define F_BWR_CAM_SRT_R3_BW_QOS_SEL0_5_POS                           5
#define F_BWR_CAM_SRT_R3_BW_QOS_SEL0_5_WIDTH                         1
#define F_BWR_CAM_SRT_R3_BW_QOS_SEL0_4_POS                           4
#define F_BWR_CAM_SRT_R3_BW_QOS_SEL0_4_WIDTH                         1
#define F_BWR_CAM_SRT_R3_BW_QOS_SEL0_3_POS                           3
#define F_BWR_CAM_SRT_R3_BW_QOS_SEL0_3_WIDTH                         1
#define F_BWR_CAM_SRT_R3_BW_QOS_SEL0_2_POS                           2
#define F_BWR_CAM_SRT_R3_BW_QOS_SEL0_2_WIDTH                         1
#define F_BWR_CAM_SRT_R3_BW_QOS_SEL0_1_POS                           1
#define F_BWR_CAM_SRT_R3_BW_QOS_SEL0_1_WIDTH                         1
#define F_BWR_CAM_SRT_R3_BW_QOS_SEL0_0_POS                           0
#define F_BWR_CAM_SRT_R3_BW_QOS_SEL0_0_WIDTH                         1

#define REG_BWR_CAM_SRT_R3_SW_QOS_TRIG0             0x274
#define F_BWR_CAM_SRT_R3_SW_QOS_TRIG0_10_POS                         10
#define F_BWR_CAM_SRT_R3_SW_QOS_TRIG0_10_WIDTH                       1
#define F_BWR_CAM_SRT_R3_SW_QOS_TRIG0_9_POS                          9
#define F_BWR_CAM_SRT_R3_SW_QOS_TRIG0_9_WIDTH                        1
#define F_BWR_CAM_SRT_R3_SW_QOS_TRIG0_8_POS                          8
#define F_BWR_CAM_SRT_R3_SW_QOS_TRIG0_8_WIDTH                        1
#define F_BWR_CAM_SRT_R3_SW_QOS_TRIG0_7_POS                          7
#define F_BWR_CAM_SRT_R3_SW_QOS_TRIG0_7_WIDTH                        1
#define F_BWR_CAM_SRT_R3_SW_QOS_TRIG0_6_POS                          6
#define F_BWR_CAM_SRT_R3_SW_QOS_TRIG0_6_WIDTH                        1
#define F_BWR_CAM_SRT_R3_SW_QOS_TRIG0_5_POS                          5
#define F_BWR_CAM_SRT_R3_SW_QOS_TRIG0_5_WIDTH                        1
#define F_BWR_CAM_SRT_R3_SW_QOS_TRIG0_4_POS                          4
#define F_BWR_CAM_SRT_R3_SW_QOS_TRIG0_4_WIDTH                        1
#define F_BWR_CAM_SRT_R3_SW_QOS_TRIG0_3_POS                          3
#define F_BWR_CAM_SRT_R3_SW_QOS_TRIG0_3_WIDTH                        1
#define F_BWR_CAM_SRT_R3_SW_QOS_TRIG0_2_POS                          2
#define F_BWR_CAM_SRT_R3_SW_QOS_TRIG0_2_WIDTH                        1
#define F_BWR_CAM_SRT_R3_SW_QOS_TRIG0_1_POS                          1
#define F_BWR_CAM_SRT_R3_SW_QOS_TRIG0_1_WIDTH                        1
#define F_BWR_CAM_SRT_R3_SW_QOS_TRIG0_0_POS                          0
#define F_BWR_CAM_SRT_R3_SW_QOS_TRIG0_0_WIDTH                        1

#define REG_BWR_CAM_SRT_R3_SW_QOS_EN0               0x278
#define F_BWR_CAM_SRT_R3_SW_QOS_EN0_10_POS                           10
#define F_BWR_CAM_SRT_R3_SW_QOS_EN0_10_WIDTH                         1
#define F_BWR_CAM_SRT_R3_SW_QOS_EN0_9_POS                            9
#define F_BWR_CAM_SRT_R3_SW_QOS_EN0_9_WIDTH                          1
#define F_BWR_CAM_SRT_R3_SW_QOS_EN0_8_POS                            8
#define F_BWR_CAM_SRT_R3_SW_QOS_EN0_8_WIDTH                          1
#define F_BWR_CAM_SRT_R3_SW_QOS_EN0_7_POS                            7
#define F_BWR_CAM_SRT_R3_SW_QOS_EN0_7_WIDTH                          1
#define F_BWR_CAM_SRT_R3_SW_QOS_EN0_6_POS                            6
#define F_BWR_CAM_SRT_R3_SW_QOS_EN0_6_WIDTH                          1
#define F_BWR_CAM_SRT_R3_SW_QOS_EN0_5_POS                            5
#define F_BWR_CAM_SRT_R3_SW_QOS_EN0_5_WIDTH                          1
#define F_BWR_CAM_SRT_R3_SW_QOS_EN0_4_POS                            4
#define F_BWR_CAM_SRT_R3_SW_QOS_EN0_4_WIDTH                          1
#define F_BWR_CAM_SRT_R3_SW_QOS_EN0_3_POS                            3
#define F_BWR_CAM_SRT_R3_SW_QOS_EN0_3_WIDTH                          1
#define F_BWR_CAM_SRT_R3_SW_QOS_EN0_2_POS                            2
#define F_BWR_CAM_SRT_R3_SW_QOS_EN0_2_WIDTH                          1
#define F_BWR_CAM_SRT_R3_SW_QOS_EN0_1_POS                            1
#define F_BWR_CAM_SRT_R3_SW_QOS_EN0_1_WIDTH                          1
#define F_BWR_CAM_SRT_R3_SW_QOS_EN0_0_POS                            0
#define F_BWR_CAM_SRT_R3_SW_QOS_EN0_0_WIDTH                          1

#define REG_BWR_CAM_SRT_R3_ENG_BW0_0                0x27C
#define F_BWR_CAM_SRT_R3_ENG_BW0_0_POS                               0
#define F_BWR_CAM_SRT_R3_ENG_BW0_0_WIDTH                             18

#define REG_BWR_CAM_SRT_R3_ENG_BW0_1                0x280
#define F_BWR_CAM_SRT_R3_ENG_BW0_1_POS                               0
#define F_BWR_CAM_SRT_R3_ENG_BW0_1_WIDTH                             18

#define REG_BWR_CAM_SRT_R3_ENG_BW0_2                0x284
#define F_BWR_CAM_SRT_R3_ENG_BW0_2_POS                               0
#define F_BWR_CAM_SRT_R3_ENG_BW0_2_WIDTH                             18

#define REG_BWR_CAM_SRT_R3_ENG_BW0_3                0x288
#define F_BWR_CAM_SRT_R3_ENG_BW0_3_POS                               0
#define F_BWR_CAM_SRT_R3_ENG_BW0_3_WIDTH                             18

#define REG_BWR_CAM_SRT_R3_ENG_BW0_4                0x28C
#define F_BWR_CAM_SRT_R3_ENG_BW0_4_POS                               0
#define F_BWR_CAM_SRT_R3_ENG_BW0_4_WIDTH                             18

#define REG_BWR_CAM_SRT_R3_ENG_BW0_5                0x290
#define F_BWR_CAM_SRT_R3_ENG_BW0_5_POS                               0
#define F_BWR_CAM_SRT_R3_ENG_BW0_5_WIDTH                             18

#define REG_BWR_CAM_SRT_R3_ENG_BW0_6                0x294
#define F_BWR_CAM_SRT_R3_ENG_BW0_6_POS                               0
#define F_BWR_CAM_SRT_R3_ENG_BW0_6_WIDTH                             18

#define REG_BWR_CAM_SRT_R3_ENG_BW0_7                0x298
#define F_BWR_CAM_SRT_R3_ENG_BW0_7_POS                               0
#define F_BWR_CAM_SRT_R3_ENG_BW0_7_WIDTH                             18

#define REG_BWR_CAM_SRT_R3_ENG_BW0_8                0x29C
#define F_BWR_CAM_SRT_R3_ENG_BW0_8_POS                               0
#define F_BWR_CAM_SRT_R3_ENG_BW0_8_WIDTH                             18

#define REG_BWR_CAM_SRT_R3_ENG_BW0_9                0x2A0
#define F_BWR_CAM_SRT_R3_ENG_BW0_9_POS                               0
#define F_BWR_CAM_SRT_R3_ENG_BW0_9_WIDTH                             18

#define REG_BWR_CAM_SRT_R3_ENG_BW0_10               0x2A4
#define F_BWR_CAM_SRT_R3_ENG_BW0_10_POS                              0
#define F_BWR_CAM_SRT_R3_ENG_BW0_10_WIDTH                            18

#define REG_BWR_CAM_SRT_R3_ENG_BW_RAT0_0            0x2A8
#define F_BWR_CAM_SRT_R3_ENG_BW_RAT0_0_POS                           0
#define F_BWR_CAM_SRT_R3_ENG_BW_RAT0_0_WIDTH                         12

#define REG_BWR_CAM_SRT_R3_ENG_BW_RAT0_1            0x2AC
#define F_BWR_CAM_SRT_R3_ENG_BW_RAT0_1_POS                           0
#define F_BWR_CAM_SRT_R3_ENG_BW_RAT0_1_WIDTH                         12

#define REG_BWR_CAM_SRT_R3_ENG_BW_RAT0_2            0x2B0
#define F_BWR_CAM_SRT_R3_ENG_BW_RAT0_2_POS                           0
#define F_BWR_CAM_SRT_R3_ENG_BW_RAT0_2_WIDTH                         12

#define REG_BWR_CAM_SRT_R3_ENG_BW_RAT0_3            0x2B4
#define F_BWR_CAM_SRT_R3_ENG_BW_RAT0_3_POS                           0
#define F_BWR_CAM_SRT_R3_ENG_BW_RAT0_3_WIDTH                         12

#define REG_BWR_CAM_SRT_R3_ENG_BW_RAT0_4            0x2B8
#define F_BWR_CAM_SRT_R3_ENG_BW_RAT0_4_POS                           0
#define F_BWR_CAM_SRT_R3_ENG_BW_RAT0_4_WIDTH                         12

#define REG_BWR_CAM_SRT_R3_ENG_BW_RAT0_5            0x2BC
#define F_BWR_CAM_SRT_R3_ENG_BW_RAT0_5_POS                           0
#define F_BWR_CAM_SRT_R3_ENG_BW_RAT0_5_WIDTH                         12

#define REG_BWR_CAM_SRT_R3_ENG_BW_RAT0_6            0x2C0
#define F_BWR_CAM_SRT_R3_ENG_BW_RAT0_6_POS                           0
#define F_BWR_CAM_SRT_R3_ENG_BW_RAT0_6_WIDTH                         12

#define REG_BWR_CAM_SRT_R3_ENG_BW_RAT0_7            0x2C4
#define F_BWR_CAM_SRT_R3_ENG_BW_RAT0_7_POS                           0
#define F_BWR_CAM_SRT_R3_ENG_BW_RAT0_7_WIDTH                         12

#define REG_BWR_CAM_SRT_R3_ENG_BW_RAT0_8            0x2C8
#define F_BWR_CAM_SRT_R3_ENG_BW_RAT0_8_POS                           0
#define F_BWR_CAM_SRT_R3_ENG_BW_RAT0_8_WIDTH                         12

#define REG_BWR_CAM_SRT_R3_ENG_BW_RAT0_9            0x2CC
#define F_BWR_CAM_SRT_R3_ENG_BW_RAT0_9_POS                           0
#define F_BWR_CAM_SRT_R3_ENG_BW_RAT0_9_WIDTH                         12

#define REG_BWR_CAM_SRT_R3_ENG_BW_RAT0_10           0x2D0
#define F_BWR_CAM_SRT_R3_ENG_BW_RAT0_10_POS                          0
#define F_BWR_CAM_SRT_R3_ENG_BW_RAT0_10_WIDTH                        12

#define REG_BWR_CAM_SRT_W0_BW_QOS_SEL0              0x2F0
#define F_BWR_CAM_SRT_W0_BW_QOS_SEL0_10_POS                          10
#define F_BWR_CAM_SRT_W0_BW_QOS_SEL0_10_WIDTH                        1
#define F_BWR_CAM_SRT_W0_BW_QOS_SEL0_9_POS                           9
#define F_BWR_CAM_SRT_W0_BW_QOS_SEL0_9_WIDTH                         1
#define F_BWR_CAM_SRT_W0_BW_QOS_SEL0_8_POS                           8
#define F_BWR_CAM_SRT_W0_BW_QOS_SEL0_8_WIDTH                         1
#define F_BWR_CAM_SRT_W0_BW_QOS_SEL0_7_POS                           7
#define F_BWR_CAM_SRT_W0_BW_QOS_SEL0_7_WIDTH                         1
#define F_BWR_CAM_SRT_W0_BW_QOS_SEL0_6_POS                           6
#define F_BWR_CAM_SRT_W0_BW_QOS_SEL0_6_WIDTH                         1
#define F_BWR_CAM_SRT_W0_BW_QOS_SEL0_5_POS                           5
#define F_BWR_CAM_SRT_W0_BW_QOS_SEL0_5_WIDTH                         1
#define F_BWR_CAM_SRT_W0_BW_QOS_SEL0_4_POS                           4
#define F_BWR_CAM_SRT_W0_BW_QOS_SEL0_4_WIDTH                         1
#define F_BWR_CAM_SRT_W0_BW_QOS_SEL0_3_POS                           3
#define F_BWR_CAM_SRT_W0_BW_QOS_SEL0_3_WIDTH                         1
#define F_BWR_CAM_SRT_W0_BW_QOS_SEL0_2_POS                           2
#define F_BWR_CAM_SRT_W0_BW_QOS_SEL0_2_WIDTH                         1
#define F_BWR_CAM_SRT_W0_BW_QOS_SEL0_1_POS                           1
#define F_BWR_CAM_SRT_W0_BW_QOS_SEL0_1_WIDTH                         1
#define F_BWR_CAM_SRT_W0_BW_QOS_SEL0_0_POS                           0
#define F_BWR_CAM_SRT_W0_BW_QOS_SEL0_0_WIDTH                         1

#define REG_BWR_CAM_SRT_W0_SW_QOS_TRIG0             0x2F4
#define F_BWR_CAM_SRT_W0_SW_QOS_TRIG0_10_POS                         10
#define F_BWR_CAM_SRT_W0_SW_QOS_TRIG0_10_WIDTH                       1
#define F_BWR_CAM_SRT_W0_SW_QOS_TRIG0_9_POS                          9
#define F_BWR_CAM_SRT_W0_SW_QOS_TRIG0_9_WIDTH                        1
#define F_BWR_CAM_SRT_W0_SW_QOS_TRIG0_8_POS                          8
#define F_BWR_CAM_SRT_W0_SW_QOS_TRIG0_8_WIDTH                        1
#define F_BWR_CAM_SRT_W0_SW_QOS_TRIG0_7_POS                          7
#define F_BWR_CAM_SRT_W0_SW_QOS_TRIG0_7_WIDTH                        1
#define F_BWR_CAM_SRT_W0_SW_QOS_TRIG0_6_POS                          6
#define F_BWR_CAM_SRT_W0_SW_QOS_TRIG0_6_WIDTH                        1
#define F_BWR_CAM_SRT_W0_SW_QOS_TRIG0_5_POS                          5
#define F_BWR_CAM_SRT_W0_SW_QOS_TRIG0_5_WIDTH                        1
#define F_BWR_CAM_SRT_W0_SW_QOS_TRIG0_4_POS                          4
#define F_BWR_CAM_SRT_W0_SW_QOS_TRIG0_4_WIDTH                        1
#define F_BWR_CAM_SRT_W0_SW_QOS_TRIG0_3_POS                          3
#define F_BWR_CAM_SRT_W0_SW_QOS_TRIG0_3_WIDTH                        1
#define F_BWR_CAM_SRT_W0_SW_QOS_TRIG0_2_POS                          2
#define F_BWR_CAM_SRT_W0_SW_QOS_TRIG0_2_WIDTH                        1
#define F_BWR_CAM_SRT_W0_SW_QOS_TRIG0_1_POS                          1
#define F_BWR_CAM_SRT_W0_SW_QOS_TRIG0_1_WIDTH                        1
#define F_BWR_CAM_SRT_W0_SW_QOS_TRIG0_0_POS                          0
#define F_BWR_CAM_SRT_W0_SW_QOS_TRIG0_0_WIDTH                        1

#define REG_BWR_CAM_SRT_W0_SW_QOS_EN0               0x2F8
#define F_BWR_CAM_SRT_W0_SW_QOS_EN0_10_POS                           10
#define F_BWR_CAM_SRT_W0_SW_QOS_EN0_10_WIDTH                         1
#define F_BWR_CAM_SRT_W0_SW_QOS_EN0_9_POS                            9
#define F_BWR_CAM_SRT_W0_SW_QOS_EN0_9_WIDTH                          1
#define F_BWR_CAM_SRT_W0_SW_QOS_EN0_8_POS                            8
#define F_BWR_CAM_SRT_W0_SW_QOS_EN0_8_WIDTH                          1
#define F_BWR_CAM_SRT_W0_SW_QOS_EN0_7_POS                            7
#define F_BWR_CAM_SRT_W0_SW_QOS_EN0_7_WIDTH                          1
#define F_BWR_CAM_SRT_W0_SW_QOS_EN0_6_POS                            6
#define F_BWR_CAM_SRT_W0_SW_QOS_EN0_6_WIDTH                          1
#define F_BWR_CAM_SRT_W0_SW_QOS_EN0_5_POS                            5
#define F_BWR_CAM_SRT_W0_SW_QOS_EN0_5_WIDTH                          1
#define F_BWR_CAM_SRT_W0_SW_QOS_EN0_4_POS                            4
#define F_BWR_CAM_SRT_W0_SW_QOS_EN0_4_WIDTH                          1
#define F_BWR_CAM_SRT_W0_SW_QOS_EN0_3_POS                            3
#define F_BWR_CAM_SRT_W0_SW_QOS_EN0_3_WIDTH                          1
#define F_BWR_CAM_SRT_W0_SW_QOS_EN0_2_POS                            2
#define F_BWR_CAM_SRT_W0_SW_QOS_EN0_2_WIDTH                          1
#define F_BWR_CAM_SRT_W0_SW_QOS_EN0_1_POS                            1
#define F_BWR_CAM_SRT_W0_SW_QOS_EN0_1_WIDTH                          1
#define F_BWR_CAM_SRT_W0_SW_QOS_EN0_0_POS                            0
#define F_BWR_CAM_SRT_W0_SW_QOS_EN0_0_WIDTH                          1

#define REG_BWR_CAM_SRT_W0_ENG_BW0_0                0x2FC
#define F_BWR_CAM_SRT_W0_ENG_BW0_0_POS                               0
#define F_BWR_CAM_SRT_W0_ENG_BW0_0_WIDTH                             18

#define REG_BWR_CAM_SRT_W0_ENG_BW0_1                0x300
#define F_BWR_CAM_SRT_W0_ENG_BW0_1_POS                               0
#define F_BWR_CAM_SRT_W0_ENG_BW0_1_WIDTH                             18

#define REG_BWR_CAM_SRT_W0_ENG_BW0_2                0x304
#define F_BWR_CAM_SRT_W0_ENG_BW0_2_POS                               0
#define F_BWR_CAM_SRT_W0_ENG_BW0_2_WIDTH                             18

#define REG_BWR_CAM_SRT_W0_ENG_BW0_3                0x308
#define F_BWR_CAM_SRT_W0_ENG_BW0_3_POS                               0
#define F_BWR_CAM_SRT_W0_ENG_BW0_3_WIDTH                             18

#define REG_BWR_CAM_SRT_W0_ENG_BW0_4                0x30C
#define F_BWR_CAM_SRT_W0_ENG_BW0_4_POS                               0
#define F_BWR_CAM_SRT_W0_ENG_BW0_4_WIDTH                             18

#define REG_BWR_CAM_SRT_W0_ENG_BW0_5                0x310
#define F_BWR_CAM_SRT_W0_ENG_BW0_5_POS                               0
#define F_BWR_CAM_SRT_W0_ENG_BW0_5_WIDTH                             18

#define REG_BWR_CAM_SRT_W0_ENG_BW0_6                0x314
#define F_BWR_CAM_SRT_W0_ENG_BW0_6_POS                               0
#define F_BWR_CAM_SRT_W0_ENG_BW0_6_WIDTH                             18

#define REG_BWR_CAM_SRT_W0_ENG_BW0_7                0x318
#define F_BWR_CAM_SRT_W0_ENG_BW0_7_POS                               0
#define F_BWR_CAM_SRT_W0_ENG_BW0_7_WIDTH                             18

#define REG_BWR_CAM_SRT_W0_ENG_BW0_8                0x31C
#define F_BWR_CAM_SRT_W0_ENG_BW0_8_POS                               0
#define F_BWR_CAM_SRT_W0_ENG_BW0_8_WIDTH                             18

#define REG_BWR_CAM_SRT_W0_ENG_BW0_9                0x320
#define F_BWR_CAM_SRT_W0_ENG_BW0_9_POS                               0
#define F_BWR_CAM_SRT_W0_ENG_BW0_9_WIDTH                             18

#define REG_BWR_CAM_SRT_W0_ENG_BW0_10               0x324
#define F_BWR_CAM_SRT_W0_ENG_BW0_10_POS                              0
#define F_BWR_CAM_SRT_W0_ENG_BW0_10_WIDTH                            18

#define REG_BWR_CAM_SRT_W0_ENG_BW_RAT0_0            0x328
#define F_BWR_CAM_SRT_W0_ENG_BW_RAT0_0_POS                           0
#define F_BWR_CAM_SRT_W0_ENG_BW_RAT0_0_WIDTH                         12

#define REG_BWR_CAM_SRT_W0_ENG_BW_RAT0_1            0x32C
#define F_BWR_CAM_SRT_W0_ENG_BW_RAT0_1_POS                           0
#define F_BWR_CAM_SRT_W0_ENG_BW_RAT0_1_WIDTH                         12

#define REG_BWR_CAM_SRT_W0_ENG_BW_RAT0_2            0x330
#define F_BWR_CAM_SRT_W0_ENG_BW_RAT0_2_POS                           0
#define F_BWR_CAM_SRT_W0_ENG_BW_RAT0_2_WIDTH                         12

#define REG_BWR_CAM_SRT_W0_ENG_BW_RAT0_3            0x334
#define F_BWR_CAM_SRT_W0_ENG_BW_RAT0_3_POS                           0
#define F_BWR_CAM_SRT_W0_ENG_BW_RAT0_3_WIDTH                         12

#define REG_BWR_CAM_SRT_W0_ENG_BW_RAT0_4            0x338
#define F_BWR_CAM_SRT_W0_ENG_BW_RAT0_4_POS                           0
#define F_BWR_CAM_SRT_W0_ENG_BW_RAT0_4_WIDTH                         12

#define REG_BWR_CAM_SRT_W0_ENG_BW_RAT0_5            0x33C
#define F_BWR_CAM_SRT_W0_ENG_BW_RAT0_5_POS                           0
#define F_BWR_CAM_SRT_W0_ENG_BW_RAT0_5_WIDTH                         12

#define REG_BWR_CAM_SRT_W0_ENG_BW_RAT0_6            0x340
#define F_BWR_CAM_SRT_W0_ENG_BW_RAT0_6_POS                           0
#define F_BWR_CAM_SRT_W0_ENG_BW_RAT0_6_WIDTH                         12

#define REG_BWR_CAM_SRT_W0_ENG_BW_RAT0_7            0x344
#define F_BWR_CAM_SRT_W0_ENG_BW_RAT0_7_POS                           0
#define F_BWR_CAM_SRT_W0_ENG_BW_RAT0_7_WIDTH                         12

#define REG_BWR_CAM_SRT_W0_ENG_BW_RAT0_8            0x348
#define F_BWR_CAM_SRT_W0_ENG_BW_RAT0_8_POS                           0
#define F_BWR_CAM_SRT_W0_ENG_BW_RAT0_8_WIDTH                         12

#define REG_BWR_CAM_SRT_W0_ENG_BW_RAT0_9            0x34C
#define F_BWR_CAM_SRT_W0_ENG_BW_RAT0_9_POS                           0
#define F_BWR_CAM_SRT_W0_ENG_BW_RAT0_9_WIDTH                         12

#define REG_BWR_CAM_SRT_W0_ENG_BW_RAT0_10           0x350
#define F_BWR_CAM_SRT_W0_ENG_BW_RAT0_10_POS                          0
#define F_BWR_CAM_SRT_W0_ENG_BW_RAT0_10_WIDTH                        12

#define REG_BWR_CAM_SRT_W1_BW_QOS_SEL0              0x370
#define F_BWR_CAM_SRT_W1_BW_QOS_SEL0_10_POS                          10
#define F_BWR_CAM_SRT_W1_BW_QOS_SEL0_10_WIDTH                        1
#define F_BWR_CAM_SRT_W1_BW_QOS_SEL0_9_POS                           9
#define F_BWR_CAM_SRT_W1_BW_QOS_SEL0_9_WIDTH                         1
#define F_BWR_CAM_SRT_W1_BW_QOS_SEL0_8_POS                           8
#define F_BWR_CAM_SRT_W1_BW_QOS_SEL0_8_WIDTH                         1
#define F_BWR_CAM_SRT_W1_BW_QOS_SEL0_7_POS                           7
#define F_BWR_CAM_SRT_W1_BW_QOS_SEL0_7_WIDTH                         1
#define F_BWR_CAM_SRT_W1_BW_QOS_SEL0_6_POS                           6
#define F_BWR_CAM_SRT_W1_BW_QOS_SEL0_6_WIDTH                         1
#define F_BWR_CAM_SRT_W1_BW_QOS_SEL0_5_POS                           5
#define F_BWR_CAM_SRT_W1_BW_QOS_SEL0_5_WIDTH                         1
#define F_BWR_CAM_SRT_W1_BW_QOS_SEL0_4_POS                           4
#define F_BWR_CAM_SRT_W1_BW_QOS_SEL0_4_WIDTH                         1
#define F_BWR_CAM_SRT_W1_BW_QOS_SEL0_3_POS                           3
#define F_BWR_CAM_SRT_W1_BW_QOS_SEL0_3_WIDTH                         1
#define F_BWR_CAM_SRT_W1_BW_QOS_SEL0_2_POS                           2
#define F_BWR_CAM_SRT_W1_BW_QOS_SEL0_2_WIDTH                         1
#define F_BWR_CAM_SRT_W1_BW_QOS_SEL0_1_POS                           1
#define F_BWR_CAM_SRT_W1_BW_QOS_SEL0_1_WIDTH                         1
#define F_BWR_CAM_SRT_W1_BW_QOS_SEL0_0_POS                           0
#define F_BWR_CAM_SRT_W1_BW_QOS_SEL0_0_WIDTH                         1

#define REG_BWR_CAM_SRT_W1_SW_QOS_TRIG0             0x374
#define F_BWR_CAM_SRT_W1_SW_QOS_TRIG0_10_POS                         10
#define F_BWR_CAM_SRT_W1_SW_QOS_TRIG0_10_WIDTH                       1
#define F_BWR_CAM_SRT_W1_SW_QOS_TRIG0_9_POS                          9
#define F_BWR_CAM_SRT_W1_SW_QOS_TRIG0_9_WIDTH                        1
#define F_BWR_CAM_SRT_W1_SW_QOS_TRIG0_8_POS                          8
#define F_BWR_CAM_SRT_W1_SW_QOS_TRIG0_8_WIDTH                        1
#define F_BWR_CAM_SRT_W1_SW_QOS_TRIG0_7_POS                          7
#define F_BWR_CAM_SRT_W1_SW_QOS_TRIG0_7_WIDTH                        1
#define F_BWR_CAM_SRT_W1_SW_QOS_TRIG0_6_POS                          6
#define F_BWR_CAM_SRT_W1_SW_QOS_TRIG0_6_WIDTH                        1
#define F_BWR_CAM_SRT_W1_SW_QOS_TRIG0_5_POS                          5
#define F_BWR_CAM_SRT_W1_SW_QOS_TRIG0_5_WIDTH                        1
#define F_BWR_CAM_SRT_W1_SW_QOS_TRIG0_4_POS                          4
#define F_BWR_CAM_SRT_W1_SW_QOS_TRIG0_4_WIDTH                        1
#define F_BWR_CAM_SRT_W1_SW_QOS_TRIG0_3_POS                          3
#define F_BWR_CAM_SRT_W1_SW_QOS_TRIG0_3_WIDTH                        1
#define F_BWR_CAM_SRT_W1_SW_QOS_TRIG0_2_POS                          2
#define F_BWR_CAM_SRT_W1_SW_QOS_TRIG0_2_WIDTH                        1
#define F_BWR_CAM_SRT_W1_SW_QOS_TRIG0_1_POS                          1
#define F_BWR_CAM_SRT_W1_SW_QOS_TRIG0_1_WIDTH                        1
#define F_BWR_CAM_SRT_W1_SW_QOS_TRIG0_0_POS                          0
#define F_BWR_CAM_SRT_W1_SW_QOS_TRIG0_0_WIDTH                        1

#define REG_BWR_CAM_SRT_W1_SW_QOS_EN0               0x378
#define F_BWR_CAM_SRT_W1_SW_QOS_EN0_10_POS                           10
#define F_BWR_CAM_SRT_W1_SW_QOS_EN0_10_WIDTH                         1
#define F_BWR_CAM_SRT_W1_SW_QOS_EN0_9_POS                            9
#define F_BWR_CAM_SRT_W1_SW_QOS_EN0_9_WIDTH                          1
#define F_BWR_CAM_SRT_W1_SW_QOS_EN0_8_POS                            8
#define F_BWR_CAM_SRT_W1_SW_QOS_EN0_8_WIDTH                          1
#define F_BWR_CAM_SRT_W1_SW_QOS_EN0_7_POS                            7
#define F_BWR_CAM_SRT_W1_SW_QOS_EN0_7_WIDTH                          1
#define F_BWR_CAM_SRT_W1_SW_QOS_EN0_6_POS                            6
#define F_BWR_CAM_SRT_W1_SW_QOS_EN0_6_WIDTH                          1
#define F_BWR_CAM_SRT_W1_SW_QOS_EN0_5_POS                            5
#define F_BWR_CAM_SRT_W1_SW_QOS_EN0_5_WIDTH                          1
#define F_BWR_CAM_SRT_W1_SW_QOS_EN0_4_POS                            4
#define F_BWR_CAM_SRT_W1_SW_QOS_EN0_4_WIDTH                          1
#define F_BWR_CAM_SRT_W1_SW_QOS_EN0_3_POS                            3
#define F_BWR_CAM_SRT_W1_SW_QOS_EN0_3_WIDTH                          1
#define F_BWR_CAM_SRT_W1_SW_QOS_EN0_2_POS                            2
#define F_BWR_CAM_SRT_W1_SW_QOS_EN0_2_WIDTH                          1
#define F_BWR_CAM_SRT_W1_SW_QOS_EN0_1_POS                            1
#define F_BWR_CAM_SRT_W1_SW_QOS_EN0_1_WIDTH                          1
#define F_BWR_CAM_SRT_W1_SW_QOS_EN0_0_POS                            0
#define F_BWR_CAM_SRT_W1_SW_QOS_EN0_0_WIDTH                          1

#define REG_BWR_CAM_SRT_W1_ENG_BW0_0                0x37C
#define F_BWR_CAM_SRT_W1_ENG_BW0_0_POS                               0
#define F_BWR_CAM_SRT_W1_ENG_BW0_0_WIDTH                             18

#define REG_BWR_CAM_SRT_W1_ENG_BW0_1                0x380
#define F_BWR_CAM_SRT_W1_ENG_BW0_1_POS                               0
#define F_BWR_CAM_SRT_W1_ENG_BW0_1_WIDTH                             18

#define REG_BWR_CAM_SRT_W1_ENG_BW0_2                0x384
#define F_BWR_CAM_SRT_W1_ENG_BW0_2_POS                               0
#define F_BWR_CAM_SRT_W1_ENG_BW0_2_WIDTH                             18

#define REG_BWR_CAM_SRT_W1_ENG_BW0_3                0x388
#define F_BWR_CAM_SRT_W1_ENG_BW0_3_POS                               0
#define F_BWR_CAM_SRT_W1_ENG_BW0_3_WIDTH                             18

#define REG_BWR_CAM_SRT_W1_ENG_BW0_4                0x38C
#define F_BWR_CAM_SRT_W1_ENG_BW0_4_POS                               0
#define F_BWR_CAM_SRT_W1_ENG_BW0_4_WIDTH                             18

#define REG_BWR_CAM_SRT_W1_ENG_BW0_5                0x390
#define F_BWR_CAM_SRT_W1_ENG_BW0_5_POS                               0
#define F_BWR_CAM_SRT_W1_ENG_BW0_5_WIDTH                             18

#define REG_BWR_CAM_SRT_W1_ENG_BW0_6                0x394
#define F_BWR_CAM_SRT_W1_ENG_BW0_6_POS                               0
#define F_BWR_CAM_SRT_W1_ENG_BW0_6_WIDTH                             18

#define REG_BWR_CAM_SRT_W1_ENG_BW0_7                0x398
#define F_BWR_CAM_SRT_W1_ENG_BW0_7_POS                               0
#define F_BWR_CAM_SRT_W1_ENG_BW0_7_WIDTH                             18

#define REG_BWR_CAM_SRT_W1_ENG_BW0_8                0x39C
#define F_BWR_CAM_SRT_W1_ENG_BW0_8_POS                               0
#define F_BWR_CAM_SRT_W1_ENG_BW0_8_WIDTH                             18

#define REG_BWR_CAM_SRT_W1_ENG_BW0_9                0x3A0
#define F_BWR_CAM_SRT_W1_ENG_BW0_9_POS                               0
#define F_BWR_CAM_SRT_W1_ENG_BW0_9_WIDTH                             18

#define REG_BWR_CAM_SRT_W1_ENG_BW0_10               0x3A4
#define F_BWR_CAM_SRT_W1_ENG_BW0_10_POS                              0
#define F_BWR_CAM_SRT_W1_ENG_BW0_10_WIDTH                            18

#define REG_BWR_CAM_SRT_W1_ENG_BW_RAT0_0            0x3A8
#define F_BWR_CAM_SRT_W1_ENG_BW_RAT0_0_POS                           0
#define F_BWR_CAM_SRT_W1_ENG_BW_RAT0_0_WIDTH                         12

#define REG_BWR_CAM_SRT_W1_ENG_BW_RAT0_1            0x3AC
#define F_BWR_CAM_SRT_W1_ENG_BW_RAT0_1_POS                           0
#define F_BWR_CAM_SRT_W1_ENG_BW_RAT0_1_WIDTH                         12

#define REG_BWR_CAM_SRT_W1_ENG_BW_RAT0_2            0x3B0
#define F_BWR_CAM_SRT_W1_ENG_BW_RAT0_2_POS                           0
#define F_BWR_CAM_SRT_W1_ENG_BW_RAT0_2_WIDTH                         12

#define REG_BWR_CAM_SRT_W1_ENG_BW_RAT0_3            0x3B4
#define F_BWR_CAM_SRT_W1_ENG_BW_RAT0_3_POS                           0
#define F_BWR_CAM_SRT_W1_ENG_BW_RAT0_3_WIDTH                         12

#define REG_BWR_CAM_SRT_W1_ENG_BW_RAT0_4            0x3B8
#define F_BWR_CAM_SRT_W1_ENG_BW_RAT0_4_POS                           0
#define F_BWR_CAM_SRT_W1_ENG_BW_RAT0_4_WIDTH                         12

#define REG_BWR_CAM_SRT_W1_ENG_BW_RAT0_5            0x3BC
#define F_BWR_CAM_SRT_W1_ENG_BW_RAT0_5_POS                           0
#define F_BWR_CAM_SRT_W1_ENG_BW_RAT0_5_WIDTH                         12

#define REG_BWR_CAM_SRT_W1_ENG_BW_RAT0_6            0x3C0
#define F_BWR_CAM_SRT_W1_ENG_BW_RAT0_6_POS                           0
#define F_BWR_CAM_SRT_W1_ENG_BW_RAT0_6_WIDTH                         12

#define REG_BWR_CAM_SRT_W1_ENG_BW_RAT0_7            0x3C4
#define F_BWR_CAM_SRT_W1_ENG_BW_RAT0_7_POS                           0
#define F_BWR_CAM_SRT_W1_ENG_BW_RAT0_7_WIDTH                         12

#define REG_BWR_CAM_SRT_W1_ENG_BW_RAT0_8            0x3C8
#define F_BWR_CAM_SRT_W1_ENG_BW_RAT0_8_POS                           0
#define F_BWR_CAM_SRT_W1_ENG_BW_RAT0_8_WIDTH                         12

#define REG_BWR_CAM_SRT_W1_ENG_BW_RAT0_9            0x3CC
#define F_BWR_CAM_SRT_W1_ENG_BW_RAT0_9_POS                           0
#define F_BWR_CAM_SRT_W1_ENG_BW_RAT0_9_WIDTH                         12

#define REG_BWR_CAM_SRT_W1_ENG_BW_RAT0_10           0x3D0
#define F_BWR_CAM_SRT_W1_ENG_BW_RAT0_10_POS                          0
#define F_BWR_CAM_SRT_W1_ENG_BW_RAT0_10_WIDTH                        12

#define REG_BWR_CAM_SRT_W2_BW_QOS_SEL0              0x3F0
#define F_BWR_CAM_SRT_W2_BW_QOS_SEL0_10_POS                          10
#define F_BWR_CAM_SRT_W2_BW_QOS_SEL0_10_WIDTH                        1
#define F_BWR_CAM_SRT_W2_BW_QOS_SEL0_9_POS                           9
#define F_BWR_CAM_SRT_W2_BW_QOS_SEL0_9_WIDTH                         1
#define F_BWR_CAM_SRT_W2_BW_QOS_SEL0_8_POS                           8
#define F_BWR_CAM_SRT_W2_BW_QOS_SEL0_8_WIDTH                         1
#define F_BWR_CAM_SRT_W2_BW_QOS_SEL0_7_POS                           7
#define F_BWR_CAM_SRT_W2_BW_QOS_SEL0_7_WIDTH                         1
#define F_BWR_CAM_SRT_W2_BW_QOS_SEL0_6_POS                           6
#define F_BWR_CAM_SRT_W2_BW_QOS_SEL0_6_WIDTH                         1
#define F_BWR_CAM_SRT_W2_BW_QOS_SEL0_5_POS                           5
#define F_BWR_CAM_SRT_W2_BW_QOS_SEL0_5_WIDTH                         1
#define F_BWR_CAM_SRT_W2_BW_QOS_SEL0_4_POS                           4
#define F_BWR_CAM_SRT_W2_BW_QOS_SEL0_4_WIDTH                         1
#define F_BWR_CAM_SRT_W2_BW_QOS_SEL0_3_POS                           3
#define F_BWR_CAM_SRT_W2_BW_QOS_SEL0_3_WIDTH                         1
#define F_BWR_CAM_SRT_W2_BW_QOS_SEL0_2_POS                           2
#define F_BWR_CAM_SRT_W2_BW_QOS_SEL0_2_WIDTH                         1
#define F_BWR_CAM_SRT_W2_BW_QOS_SEL0_1_POS                           1
#define F_BWR_CAM_SRT_W2_BW_QOS_SEL0_1_WIDTH                         1
#define F_BWR_CAM_SRT_W2_BW_QOS_SEL0_0_POS                           0
#define F_BWR_CAM_SRT_W2_BW_QOS_SEL0_0_WIDTH                         1

#define REG_BWR_CAM_SRT_W2_SW_QOS_TRIG0             0x3F4
#define F_BWR_CAM_SRT_W2_SW_QOS_TRIG0_10_POS                         10
#define F_BWR_CAM_SRT_W2_SW_QOS_TRIG0_10_WIDTH                       1
#define F_BWR_CAM_SRT_W2_SW_QOS_TRIG0_9_POS                          9
#define F_BWR_CAM_SRT_W2_SW_QOS_TRIG0_9_WIDTH                        1
#define F_BWR_CAM_SRT_W2_SW_QOS_TRIG0_8_POS                          8
#define F_BWR_CAM_SRT_W2_SW_QOS_TRIG0_8_WIDTH                        1
#define F_BWR_CAM_SRT_W2_SW_QOS_TRIG0_7_POS                          7
#define F_BWR_CAM_SRT_W2_SW_QOS_TRIG0_7_WIDTH                        1
#define F_BWR_CAM_SRT_W2_SW_QOS_TRIG0_6_POS                          6
#define F_BWR_CAM_SRT_W2_SW_QOS_TRIG0_6_WIDTH                        1
#define F_BWR_CAM_SRT_W2_SW_QOS_TRIG0_5_POS                          5
#define F_BWR_CAM_SRT_W2_SW_QOS_TRIG0_5_WIDTH                        1
#define F_BWR_CAM_SRT_W2_SW_QOS_TRIG0_4_POS                          4
#define F_BWR_CAM_SRT_W2_SW_QOS_TRIG0_4_WIDTH                        1
#define F_BWR_CAM_SRT_W2_SW_QOS_TRIG0_3_POS                          3
#define F_BWR_CAM_SRT_W2_SW_QOS_TRIG0_3_WIDTH                        1
#define F_BWR_CAM_SRT_W2_SW_QOS_TRIG0_2_POS                          2
#define F_BWR_CAM_SRT_W2_SW_QOS_TRIG0_2_WIDTH                        1
#define F_BWR_CAM_SRT_W2_SW_QOS_TRIG0_1_POS                          1
#define F_BWR_CAM_SRT_W2_SW_QOS_TRIG0_1_WIDTH                        1
#define F_BWR_CAM_SRT_W2_SW_QOS_TRIG0_0_POS                          0
#define F_BWR_CAM_SRT_W2_SW_QOS_TRIG0_0_WIDTH                        1

#define REG_BWR_CAM_SRT_W2_SW_QOS_EN0               0x3F8
#define F_BWR_CAM_SRT_W2_SW_QOS_EN0_10_POS                           10
#define F_BWR_CAM_SRT_W2_SW_QOS_EN0_10_WIDTH                         1
#define F_BWR_CAM_SRT_W2_SW_QOS_EN0_9_POS                            9
#define F_BWR_CAM_SRT_W2_SW_QOS_EN0_9_WIDTH                          1
#define F_BWR_CAM_SRT_W2_SW_QOS_EN0_8_POS                            8
#define F_BWR_CAM_SRT_W2_SW_QOS_EN0_8_WIDTH                          1
#define F_BWR_CAM_SRT_W2_SW_QOS_EN0_7_POS                            7
#define F_BWR_CAM_SRT_W2_SW_QOS_EN0_7_WIDTH                          1
#define F_BWR_CAM_SRT_W2_SW_QOS_EN0_6_POS                            6
#define F_BWR_CAM_SRT_W2_SW_QOS_EN0_6_WIDTH                          1
#define F_BWR_CAM_SRT_W2_SW_QOS_EN0_5_POS                            5
#define F_BWR_CAM_SRT_W2_SW_QOS_EN0_5_WIDTH                          1
#define F_BWR_CAM_SRT_W2_SW_QOS_EN0_4_POS                            4
#define F_BWR_CAM_SRT_W2_SW_QOS_EN0_4_WIDTH                          1
#define F_BWR_CAM_SRT_W2_SW_QOS_EN0_3_POS                            3
#define F_BWR_CAM_SRT_W2_SW_QOS_EN0_3_WIDTH                          1
#define F_BWR_CAM_SRT_W2_SW_QOS_EN0_2_POS                            2
#define F_BWR_CAM_SRT_W2_SW_QOS_EN0_2_WIDTH                          1
#define F_BWR_CAM_SRT_W2_SW_QOS_EN0_1_POS                            1
#define F_BWR_CAM_SRT_W2_SW_QOS_EN0_1_WIDTH                          1
#define F_BWR_CAM_SRT_W2_SW_QOS_EN0_0_POS                            0
#define F_BWR_CAM_SRT_W2_SW_QOS_EN0_0_WIDTH                          1

#define REG_BWR_CAM_SRT_W2_ENG_BW0_0                0x3FC
#define F_BWR_CAM_SRT_W2_ENG_BW0_0_POS                               0
#define F_BWR_CAM_SRT_W2_ENG_BW0_0_WIDTH                             18

#define REG_BWR_CAM_SRT_W2_ENG_BW0_1                0x400
#define F_BWR_CAM_SRT_W2_ENG_BW0_1_POS                               0
#define F_BWR_CAM_SRT_W2_ENG_BW0_1_WIDTH                             18

#define REG_BWR_CAM_SRT_W2_ENG_BW0_2                0x404
#define F_BWR_CAM_SRT_W2_ENG_BW0_2_POS                               0
#define F_BWR_CAM_SRT_W2_ENG_BW0_2_WIDTH                             18

#define REG_BWR_CAM_SRT_W2_ENG_BW0_3                0x408
#define F_BWR_CAM_SRT_W2_ENG_BW0_3_POS                               0
#define F_BWR_CAM_SRT_W2_ENG_BW0_3_WIDTH                             18

#define REG_BWR_CAM_SRT_W2_ENG_BW0_4                0x40C
#define F_BWR_CAM_SRT_W2_ENG_BW0_4_POS                               0
#define F_BWR_CAM_SRT_W2_ENG_BW0_4_WIDTH                             18

#define REG_BWR_CAM_SRT_W2_ENG_BW0_5                0x410
#define F_BWR_CAM_SRT_W2_ENG_BW0_5_POS                               0
#define F_BWR_CAM_SRT_W2_ENG_BW0_5_WIDTH                             18

#define REG_BWR_CAM_SRT_W2_ENG_BW0_6                0x414
#define F_BWR_CAM_SRT_W2_ENG_BW0_6_POS                               0
#define F_BWR_CAM_SRT_W2_ENG_BW0_6_WIDTH                             18

#define REG_BWR_CAM_SRT_W2_ENG_BW0_7                0x418
#define F_BWR_CAM_SRT_W2_ENG_BW0_7_POS                               0
#define F_BWR_CAM_SRT_W2_ENG_BW0_7_WIDTH                             18

#define REG_BWR_CAM_SRT_W2_ENG_BW0_8                0x41C
#define F_BWR_CAM_SRT_W2_ENG_BW0_8_POS                               0
#define F_BWR_CAM_SRT_W2_ENG_BW0_8_WIDTH                             18

#define REG_BWR_CAM_SRT_W2_ENG_BW0_9                0x420
#define F_BWR_CAM_SRT_W2_ENG_BW0_9_POS                               0
#define F_BWR_CAM_SRT_W2_ENG_BW0_9_WIDTH                             18

#define REG_BWR_CAM_SRT_W2_ENG_BW0_10               0x424
#define F_BWR_CAM_SRT_W2_ENG_BW0_10_POS                              0
#define F_BWR_CAM_SRT_W2_ENG_BW0_10_WIDTH                            18

#define REG_BWR_CAM_SRT_W2_ENG_BW_RAT0_0            0x428
#define F_BWR_CAM_SRT_W2_ENG_BW_RAT0_0_POS                           0
#define F_BWR_CAM_SRT_W2_ENG_BW_RAT0_0_WIDTH                         12

#define REG_BWR_CAM_SRT_W2_ENG_BW_RAT0_1            0x42C
#define F_BWR_CAM_SRT_W2_ENG_BW_RAT0_1_POS                           0
#define F_BWR_CAM_SRT_W2_ENG_BW_RAT0_1_WIDTH                         12

#define REG_BWR_CAM_SRT_W2_ENG_BW_RAT0_2            0x430
#define F_BWR_CAM_SRT_W2_ENG_BW_RAT0_2_POS                           0
#define F_BWR_CAM_SRT_W2_ENG_BW_RAT0_2_WIDTH                         12

#define REG_BWR_CAM_SRT_W2_ENG_BW_RAT0_3            0x434
#define F_BWR_CAM_SRT_W2_ENG_BW_RAT0_3_POS                           0
#define F_BWR_CAM_SRT_W2_ENG_BW_RAT0_3_WIDTH                         12

#define REG_BWR_CAM_SRT_W2_ENG_BW_RAT0_4            0x438
#define F_BWR_CAM_SRT_W2_ENG_BW_RAT0_4_POS                           0
#define F_BWR_CAM_SRT_W2_ENG_BW_RAT0_4_WIDTH                         12

#define REG_BWR_CAM_SRT_W2_ENG_BW_RAT0_5            0x43C
#define F_BWR_CAM_SRT_W2_ENG_BW_RAT0_5_POS                           0
#define F_BWR_CAM_SRT_W2_ENG_BW_RAT0_5_WIDTH                         12

#define REG_BWR_CAM_SRT_W2_ENG_BW_RAT0_6            0x440
#define F_BWR_CAM_SRT_W2_ENG_BW_RAT0_6_POS                           0
#define F_BWR_CAM_SRT_W2_ENG_BW_RAT0_6_WIDTH                         12

#define REG_BWR_CAM_SRT_W2_ENG_BW_RAT0_7            0x444
#define F_BWR_CAM_SRT_W2_ENG_BW_RAT0_7_POS                           0
#define F_BWR_CAM_SRT_W2_ENG_BW_RAT0_7_WIDTH                         12

#define REG_BWR_CAM_SRT_W2_ENG_BW_RAT0_8            0x448
#define F_BWR_CAM_SRT_W2_ENG_BW_RAT0_8_POS                           0
#define F_BWR_CAM_SRT_W2_ENG_BW_RAT0_8_WIDTH                         12

#define REG_BWR_CAM_SRT_W2_ENG_BW_RAT0_9            0x44C
#define F_BWR_CAM_SRT_W2_ENG_BW_RAT0_9_POS                           0
#define F_BWR_CAM_SRT_W2_ENG_BW_RAT0_9_WIDTH                         12

#define REG_BWR_CAM_SRT_W2_ENG_BW_RAT0_10           0x450
#define F_BWR_CAM_SRT_W2_ENG_BW_RAT0_10_POS                          0
#define F_BWR_CAM_SRT_W2_ENG_BW_RAT0_10_WIDTH                        12

#define REG_BWR_CAM_SRT_W3_BW_QOS_SEL0              0x470
#define F_BWR_CAM_SRT_W3_BW_QOS_SEL0_10_POS                          10
#define F_BWR_CAM_SRT_W3_BW_QOS_SEL0_10_WIDTH                        1
#define F_BWR_CAM_SRT_W3_BW_QOS_SEL0_9_POS                           9
#define F_BWR_CAM_SRT_W3_BW_QOS_SEL0_9_WIDTH                         1
#define F_BWR_CAM_SRT_W3_BW_QOS_SEL0_8_POS                           8
#define F_BWR_CAM_SRT_W3_BW_QOS_SEL0_8_WIDTH                         1
#define F_BWR_CAM_SRT_W3_BW_QOS_SEL0_7_POS                           7
#define F_BWR_CAM_SRT_W3_BW_QOS_SEL0_7_WIDTH                         1
#define F_BWR_CAM_SRT_W3_BW_QOS_SEL0_6_POS                           6
#define F_BWR_CAM_SRT_W3_BW_QOS_SEL0_6_WIDTH                         1
#define F_BWR_CAM_SRT_W3_BW_QOS_SEL0_5_POS                           5
#define F_BWR_CAM_SRT_W3_BW_QOS_SEL0_5_WIDTH                         1
#define F_BWR_CAM_SRT_W3_BW_QOS_SEL0_4_POS                           4
#define F_BWR_CAM_SRT_W3_BW_QOS_SEL0_4_WIDTH                         1
#define F_BWR_CAM_SRT_W3_BW_QOS_SEL0_3_POS                           3
#define F_BWR_CAM_SRT_W3_BW_QOS_SEL0_3_WIDTH                         1
#define F_BWR_CAM_SRT_W3_BW_QOS_SEL0_2_POS                           2
#define F_BWR_CAM_SRT_W3_BW_QOS_SEL0_2_WIDTH                         1
#define F_BWR_CAM_SRT_W3_BW_QOS_SEL0_1_POS                           1
#define F_BWR_CAM_SRT_W3_BW_QOS_SEL0_1_WIDTH                         1
#define F_BWR_CAM_SRT_W3_BW_QOS_SEL0_0_POS                           0
#define F_BWR_CAM_SRT_W3_BW_QOS_SEL0_0_WIDTH                         1

#define REG_BWR_CAM_SRT_W3_SW_QOS_TRIG0             0x474
#define F_BWR_CAM_SRT_W3_SW_QOS_TRIG0_10_POS                         10
#define F_BWR_CAM_SRT_W3_SW_QOS_TRIG0_10_WIDTH                       1
#define F_BWR_CAM_SRT_W3_SW_QOS_TRIG0_9_POS                          9
#define F_BWR_CAM_SRT_W3_SW_QOS_TRIG0_9_WIDTH                        1
#define F_BWR_CAM_SRT_W3_SW_QOS_TRIG0_8_POS                          8
#define F_BWR_CAM_SRT_W3_SW_QOS_TRIG0_8_WIDTH                        1
#define F_BWR_CAM_SRT_W3_SW_QOS_TRIG0_7_POS                          7
#define F_BWR_CAM_SRT_W3_SW_QOS_TRIG0_7_WIDTH                        1
#define F_BWR_CAM_SRT_W3_SW_QOS_TRIG0_6_POS                          6
#define F_BWR_CAM_SRT_W3_SW_QOS_TRIG0_6_WIDTH                        1
#define F_BWR_CAM_SRT_W3_SW_QOS_TRIG0_5_POS                          5
#define F_BWR_CAM_SRT_W3_SW_QOS_TRIG0_5_WIDTH                        1
#define F_BWR_CAM_SRT_W3_SW_QOS_TRIG0_4_POS                          4
#define F_BWR_CAM_SRT_W3_SW_QOS_TRIG0_4_WIDTH                        1
#define F_BWR_CAM_SRT_W3_SW_QOS_TRIG0_3_POS                          3
#define F_BWR_CAM_SRT_W3_SW_QOS_TRIG0_3_WIDTH                        1
#define F_BWR_CAM_SRT_W3_SW_QOS_TRIG0_2_POS                          2
#define F_BWR_CAM_SRT_W3_SW_QOS_TRIG0_2_WIDTH                        1
#define F_BWR_CAM_SRT_W3_SW_QOS_TRIG0_1_POS                          1
#define F_BWR_CAM_SRT_W3_SW_QOS_TRIG0_1_WIDTH                        1
#define F_BWR_CAM_SRT_W3_SW_QOS_TRIG0_0_POS                          0
#define F_BWR_CAM_SRT_W3_SW_QOS_TRIG0_0_WIDTH                        1

#define REG_BWR_CAM_SRT_W3_SW_QOS_EN0               0x478
#define F_BWR_CAM_SRT_W3_SW_QOS_EN0_10_POS                           10
#define F_BWR_CAM_SRT_W3_SW_QOS_EN0_10_WIDTH                         1
#define F_BWR_CAM_SRT_W3_SW_QOS_EN0_9_POS                            9
#define F_BWR_CAM_SRT_W3_SW_QOS_EN0_9_WIDTH                          1
#define F_BWR_CAM_SRT_W3_SW_QOS_EN0_8_POS                            8
#define F_BWR_CAM_SRT_W3_SW_QOS_EN0_8_WIDTH                          1
#define F_BWR_CAM_SRT_W3_SW_QOS_EN0_7_POS                            7
#define F_BWR_CAM_SRT_W3_SW_QOS_EN0_7_WIDTH                          1
#define F_BWR_CAM_SRT_W3_SW_QOS_EN0_6_POS                            6
#define F_BWR_CAM_SRT_W3_SW_QOS_EN0_6_WIDTH                          1
#define F_BWR_CAM_SRT_W3_SW_QOS_EN0_5_POS                            5
#define F_BWR_CAM_SRT_W3_SW_QOS_EN0_5_WIDTH                          1
#define F_BWR_CAM_SRT_W3_SW_QOS_EN0_4_POS                            4
#define F_BWR_CAM_SRT_W3_SW_QOS_EN0_4_WIDTH                          1
#define F_BWR_CAM_SRT_W3_SW_QOS_EN0_3_POS                            3
#define F_BWR_CAM_SRT_W3_SW_QOS_EN0_3_WIDTH                          1
#define F_BWR_CAM_SRT_W3_SW_QOS_EN0_2_POS                            2
#define F_BWR_CAM_SRT_W3_SW_QOS_EN0_2_WIDTH                          1
#define F_BWR_CAM_SRT_W3_SW_QOS_EN0_1_POS                            1
#define F_BWR_CAM_SRT_W3_SW_QOS_EN0_1_WIDTH                          1
#define F_BWR_CAM_SRT_W3_SW_QOS_EN0_0_POS                            0
#define F_BWR_CAM_SRT_W3_SW_QOS_EN0_0_WIDTH                          1

#define REG_BWR_CAM_SRT_W3_ENG_BW0_0                0x47C
#define F_BWR_CAM_SRT_W3_ENG_BW0_0_POS                               0
#define F_BWR_CAM_SRT_W3_ENG_BW0_0_WIDTH                             18

#define REG_BWR_CAM_SRT_W3_ENG_BW0_1                0x480
#define F_BWR_CAM_SRT_W3_ENG_BW0_1_POS                               0
#define F_BWR_CAM_SRT_W3_ENG_BW0_1_WIDTH                             18

#define REG_BWR_CAM_SRT_W3_ENG_BW0_2                0x484
#define F_BWR_CAM_SRT_W3_ENG_BW0_2_POS                               0
#define F_BWR_CAM_SRT_W3_ENG_BW0_2_WIDTH                             18

#define REG_BWR_CAM_SRT_W3_ENG_BW0_3                0x488
#define F_BWR_CAM_SRT_W3_ENG_BW0_3_POS                               0
#define F_BWR_CAM_SRT_W3_ENG_BW0_3_WIDTH                             18

#define REG_BWR_CAM_SRT_W3_ENG_BW0_4                0x48C
#define F_BWR_CAM_SRT_W3_ENG_BW0_4_POS                               0
#define F_BWR_CAM_SRT_W3_ENG_BW0_4_WIDTH                             18

#define REG_BWR_CAM_SRT_W3_ENG_BW0_5                0x490
#define F_BWR_CAM_SRT_W3_ENG_BW0_5_POS                               0
#define F_BWR_CAM_SRT_W3_ENG_BW0_5_WIDTH                             18

#define REG_BWR_CAM_SRT_W3_ENG_BW0_6                0x494
#define F_BWR_CAM_SRT_W3_ENG_BW0_6_POS                               0
#define F_BWR_CAM_SRT_W3_ENG_BW0_6_WIDTH                             18

#define REG_BWR_CAM_SRT_W3_ENG_BW0_7                0x498
#define F_BWR_CAM_SRT_W3_ENG_BW0_7_POS                               0
#define F_BWR_CAM_SRT_W3_ENG_BW0_7_WIDTH                             18

#define REG_BWR_CAM_SRT_W3_ENG_BW0_8                0x49C
#define F_BWR_CAM_SRT_W3_ENG_BW0_8_POS                               0
#define F_BWR_CAM_SRT_W3_ENG_BW0_8_WIDTH                             18

#define REG_BWR_CAM_SRT_W3_ENG_BW0_9                0x4A0
#define F_BWR_CAM_SRT_W3_ENG_BW0_9_POS                               0
#define F_BWR_CAM_SRT_W3_ENG_BW0_9_WIDTH                             18

#define REG_BWR_CAM_SRT_W3_ENG_BW0_10               0x4A4
#define F_BWR_CAM_SRT_W3_ENG_BW0_10_POS                              0
#define F_BWR_CAM_SRT_W3_ENG_BW0_10_WIDTH                            18

#define REG_BWR_CAM_SRT_W3_ENG_BW_RAT0_0            0x4A8
#define F_BWR_CAM_SRT_W3_ENG_BW_RAT0_0_POS                           0
#define F_BWR_CAM_SRT_W3_ENG_BW_RAT0_0_WIDTH                         12

#define REG_BWR_CAM_SRT_W3_ENG_BW_RAT0_1            0x4AC
#define F_BWR_CAM_SRT_W3_ENG_BW_RAT0_1_POS                           0
#define F_BWR_CAM_SRT_W3_ENG_BW_RAT0_1_WIDTH                         12

#define REG_BWR_CAM_SRT_W3_ENG_BW_RAT0_2            0x4B0
#define F_BWR_CAM_SRT_W3_ENG_BW_RAT0_2_POS                           0
#define F_BWR_CAM_SRT_W3_ENG_BW_RAT0_2_WIDTH                         12

#define REG_BWR_CAM_SRT_W3_ENG_BW_RAT0_3            0x4B4
#define F_BWR_CAM_SRT_W3_ENG_BW_RAT0_3_POS                           0
#define F_BWR_CAM_SRT_W3_ENG_BW_RAT0_3_WIDTH                         12

#define REG_BWR_CAM_SRT_W3_ENG_BW_RAT0_4            0x4B8
#define F_BWR_CAM_SRT_W3_ENG_BW_RAT0_4_POS                           0
#define F_BWR_CAM_SRT_W3_ENG_BW_RAT0_4_WIDTH                         12

#define REG_BWR_CAM_SRT_W3_ENG_BW_RAT0_5            0x4BC
#define F_BWR_CAM_SRT_W3_ENG_BW_RAT0_5_POS                           0
#define F_BWR_CAM_SRT_W3_ENG_BW_RAT0_5_WIDTH                         12

#define REG_BWR_CAM_SRT_W3_ENG_BW_RAT0_6            0x4C0
#define F_BWR_CAM_SRT_W3_ENG_BW_RAT0_6_POS                           0
#define F_BWR_CAM_SRT_W3_ENG_BW_RAT0_6_WIDTH                         12

#define REG_BWR_CAM_SRT_W3_ENG_BW_RAT0_7            0x4C4
#define F_BWR_CAM_SRT_W3_ENG_BW_RAT0_7_POS                           0
#define F_BWR_CAM_SRT_W3_ENG_BW_RAT0_7_WIDTH                         12

#define REG_BWR_CAM_SRT_W3_ENG_BW_RAT0_8            0x4C8
#define F_BWR_CAM_SRT_W3_ENG_BW_RAT0_8_POS                           0
#define F_BWR_CAM_SRT_W3_ENG_BW_RAT0_8_WIDTH                         12

#define REG_BWR_CAM_SRT_W3_ENG_BW_RAT0_9            0x4CC
#define F_BWR_CAM_SRT_W3_ENG_BW_RAT0_9_POS                           0
#define F_BWR_CAM_SRT_W3_ENG_BW_RAT0_9_WIDTH                         12

#define REG_BWR_CAM_SRT_W3_ENG_BW_RAT0_10           0x4D0
#define F_BWR_CAM_SRT_W3_ENG_BW_RAT0_10_POS                          0
#define F_BWR_CAM_SRT_W3_ENG_BW_RAT0_10_WIDTH                        12

#define REG_BWR_CAM_HRT_TTL_BW_QOS_SEL              0x4F0
#define F_BWR_CAM_HRT_TTL_BW_QOS_SEL10_POS                           10
#define F_BWR_CAM_HRT_TTL_BW_QOS_SEL10_WIDTH                         1
#define F_BWR_CAM_HRT_TTL_BW_QOS_SEL9_POS                            9
#define F_BWR_CAM_HRT_TTL_BW_QOS_SEL9_WIDTH                          1
#define F_BWR_CAM_HRT_TTL_BW_QOS_SEL8_POS                            8
#define F_BWR_CAM_HRT_TTL_BW_QOS_SEL8_WIDTH                          1
#define F_BWR_CAM_HRT_TTL_BW_QOS_SEL7_POS                            7
#define F_BWR_CAM_HRT_TTL_BW_QOS_SEL7_WIDTH                          1
#define F_BWR_CAM_HRT_TTL_BW_QOS_SEL6_POS                            6
#define F_BWR_CAM_HRT_TTL_BW_QOS_SEL6_WIDTH                          1
#define F_BWR_CAM_HRT_TTL_BW_QOS_SEL5_POS                            5
#define F_BWR_CAM_HRT_TTL_BW_QOS_SEL5_WIDTH                          1
#define F_BWR_CAM_HRT_TTL_BW_QOS_SEL4_POS                            4
#define F_BWR_CAM_HRT_TTL_BW_QOS_SEL4_WIDTH                          1
#define F_BWR_CAM_HRT_TTL_BW_QOS_SEL3_POS                            3
#define F_BWR_CAM_HRT_TTL_BW_QOS_SEL3_WIDTH                          1
#define F_BWR_CAM_HRT_TTL_BW_QOS_SEL2_POS                            2
#define F_BWR_CAM_HRT_TTL_BW_QOS_SEL2_WIDTH                          1
#define F_BWR_CAM_HRT_TTL_BW_QOS_SEL1_POS                            1
#define F_BWR_CAM_HRT_TTL_BW_QOS_SEL1_WIDTH                          1
#define F_BWR_CAM_HRT_TTL_BW_QOS_SEL0_POS                            0
#define F_BWR_CAM_HRT_TTL_BW_QOS_SEL0_WIDTH                          1

#define REG_BWR_CAM_HRT_TTL_SW_QOS_TRIG             0x4F4
#define F_BWR_CAM_HRT_TTL_SW_QOS_TRIG10_POS                          10
#define F_BWR_CAM_HRT_TTL_SW_QOS_TRIG10_WIDTH                        1
#define F_BWR_CAM_HRT_TTL_SW_QOS_TRIG9_POS                           9
#define F_BWR_CAM_HRT_TTL_SW_QOS_TRIG9_WIDTH                         1
#define F_BWR_CAM_HRT_TTL_SW_QOS_TRIG8_POS                           8
#define F_BWR_CAM_HRT_TTL_SW_QOS_TRIG8_WIDTH                         1
#define F_BWR_CAM_HRT_TTL_SW_QOS_TRIG7_POS                           7
#define F_BWR_CAM_HRT_TTL_SW_QOS_TRIG7_WIDTH                         1
#define F_BWR_CAM_HRT_TTL_SW_QOS_TRIG6_POS                           6
#define F_BWR_CAM_HRT_TTL_SW_QOS_TRIG6_WIDTH                         1
#define F_BWR_CAM_HRT_TTL_SW_QOS_TRIG5_POS                           5
#define F_BWR_CAM_HRT_TTL_SW_QOS_TRIG5_WIDTH                         1
#define F_BWR_CAM_HRT_TTL_SW_QOS_TRIG4_POS                           4
#define F_BWR_CAM_HRT_TTL_SW_QOS_TRIG4_WIDTH                         1
#define F_BWR_CAM_HRT_TTL_SW_QOS_TRIG3_POS                           3
#define F_BWR_CAM_HRT_TTL_SW_QOS_TRIG3_WIDTH                         1
#define F_BWR_CAM_HRT_TTL_SW_QOS_TRIG2_POS                           2
#define F_BWR_CAM_HRT_TTL_SW_QOS_TRIG2_WIDTH                         1
#define F_BWR_CAM_HRT_TTL_SW_QOS_TRIG1_POS                           1
#define F_BWR_CAM_HRT_TTL_SW_QOS_TRIG1_WIDTH                         1
#define F_BWR_CAM_HRT_TTL_SW_QOS_TRIG0_POS                           0
#define F_BWR_CAM_HRT_TTL_SW_QOS_TRIG0_WIDTH                         1

#define REG_BWR_CAM_HRT_TTL_SW_QOS_EN               0x4F8
#define F_BWR_CAM_HRT_TTL_SW_QOS_EN10_POS                            10
#define F_BWR_CAM_HRT_TTL_SW_QOS_EN10_WIDTH                          1
#define F_BWR_CAM_HRT_TTL_SW_QOS_EN9_POS                             9
#define F_BWR_CAM_HRT_TTL_SW_QOS_EN9_WIDTH                           1
#define F_BWR_CAM_HRT_TTL_SW_QOS_EN8_POS                             8
#define F_BWR_CAM_HRT_TTL_SW_QOS_EN8_WIDTH                           1
#define F_BWR_CAM_HRT_TTL_SW_QOS_EN7_POS                             7
#define F_BWR_CAM_HRT_TTL_SW_QOS_EN7_WIDTH                           1
#define F_BWR_CAM_HRT_TTL_SW_QOS_EN6_POS                             6
#define F_BWR_CAM_HRT_TTL_SW_QOS_EN6_WIDTH                           1
#define F_BWR_CAM_HRT_TTL_SW_QOS_EN5_POS                             5
#define F_BWR_CAM_HRT_TTL_SW_QOS_EN5_WIDTH                           1
#define F_BWR_CAM_HRT_TTL_SW_QOS_EN4_POS                             4
#define F_BWR_CAM_HRT_TTL_SW_QOS_EN4_WIDTH                           1
#define F_BWR_CAM_HRT_TTL_SW_QOS_EN3_POS                             3
#define F_BWR_CAM_HRT_TTL_SW_QOS_EN3_WIDTH                           1
#define F_BWR_CAM_HRT_TTL_SW_QOS_EN2_POS                             2
#define F_BWR_CAM_HRT_TTL_SW_QOS_EN2_WIDTH                           1
#define F_BWR_CAM_HRT_TTL_SW_QOS_EN1_POS                             1
#define F_BWR_CAM_HRT_TTL_SW_QOS_EN1_WIDTH                           1
#define F_BWR_CAM_HRT_TTL_SW_QOS_EN0_POS                             0
#define F_BWR_CAM_HRT_TTL_SW_QOS_EN0_WIDTH                           1

#define REG_BWR_CAM_HRT_TTL_ENG_BW0                 0x4FC
#define F_BWR_CAM_HRT_TTL_ENG_BW0_POS                                0
#define F_BWR_CAM_HRT_TTL_ENG_BW0_WIDTH                              18

#define REG_BWR_CAM_HRT_TTL_ENG_BW1                 0x500
#define F_BWR_CAM_HRT_TTL_ENG_BW1_POS                                0
#define F_BWR_CAM_HRT_TTL_ENG_BW1_WIDTH                              18

#define REG_BWR_CAM_HRT_TTL_ENG_BW2                 0x504
#define F_BWR_CAM_HRT_TTL_ENG_BW2_POS                                0
#define F_BWR_CAM_HRT_TTL_ENG_BW2_WIDTH                              18

#define REG_BWR_CAM_HRT_TTL_ENG_BW3                 0x508
#define F_BWR_CAM_HRT_TTL_ENG_BW3_POS                                0
#define F_BWR_CAM_HRT_TTL_ENG_BW3_WIDTH                              18

#define REG_BWR_CAM_HRT_TTL_ENG_BW4                 0x50C
#define F_BWR_CAM_HRT_TTL_ENG_BW4_POS                                0
#define F_BWR_CAM_HRT_TTL_ENG_BW4_WIDTH                              18

#define REG_BWR_CAM_HRT_TTL_ENG_BW5                 0x510
#define F_BWR_CAM_HRT_TTL_ENG_BW5_POS                                0
#define F_BWR_CAM_HRT_TTL_ENG_BW5_WIDTH                              18

#define REG_BWR_CAM_HRT_TTL_ENG_BW6                 0x514
#define F_BWR_CAM_HRT_TTL_ENG_BW6_POS                                0
#define F_BWR_CAM_HRT_TTL_ENG_BW6_WIDTH                              18

#define REG_BWR_CAM_HRT_TTL_ENG_BW7                 0x518
#define F_BWR_CAM_HRT_TTL_ENG_BW7_POS                                0
#define F_BWR_CAM_HRT_TTL_ENG_BW7_WIDTH                              18

#define REG_BWR_CAM_HRT_TTL_ENG_BW8                 0x51C
#define F_BWR_CAM_HRT_TTL_ENG_BW8_POS                                0
#define F_BWR_CAM_HRT_TTL_ENG_BW8_WIDTH                              18

#define REG_BWR_CAM_HRT_TTL_ENG_BW9                 0x520
#define F_BWR_CAM_HRT_TTL_ENG_BW9_POS                                0
#define F_BWR_CAM_HRT_TTL_ENG_BW9_WIDTH                              18

#define REG_BWR_CAM_HRT_TTL_ENG_BW10                0x524
#define F_BWR_CAM_HRT_TTL_ENG_BW10_POS                               0
#define F_BWR_CAM_HRT_TTL_ENG_BW10_WIDTH                             18

#define REG_BWR_CAM_HRT_TTL_ENG_BW_RAT0             0x528
#define F_BWR_CAM_HRT_TTL_ENG_BW_RAT0_POS                            0
#define F_BWR_CAM_HRT_TTL_ENG_BW_RAT0_WIDTH                          12

#define REG_BWR_CAM_HRT_TTL_ENG_BW_RAT1             0x52C
#define F_BWR_CAM_HRT_TTL_ENG_BW_RAT1_POS                            0
#define F_BWR_CAM_HRT_TTL_ENG_BW_RAT1_WIDTH                          12

#define REG_BWR_CAM_HRT_TTL_ENG_BW_RAT2             0x530
#define F_BWR_CAM_HRT_TTL_ENG_BW_RAT2_POS                            0
#define F_BWR_CAM_HRT_TTL_ENG_BW_RAT2_WIDTH                          12

#define REG_BWR_CAM_HRT_TTL_ENG_BW_RAT3             0x534
#define F_BWR_CAM_HRT_TTL_ENG_BW_RAT3_POS                            0
#define F_BWR_CAM_HRT_TTL_ENG_BW_RAT3_WIDTH                          12

#define REG_BWR_CAM_HRT_TTL_ENG_BW_RAT4             0x538
#define F_BWR_CAM_HRT_TTL_ENG_BW_RAT4_POS                            0
#define F_BWR_CAM_HRT_TTL_ENG_BW_RAT4_WIDTH                          12

#define REG_BWR_CAM_HRT_TTL_ENG_BW_RAT5             0x53C
#define F_BWR_CAM_HRT_TTL_ENG_BW_RAT5_POS                            0
#define F_BWR_CAM_HRT_TTL_ENG_BW_RAT5_WIDTH                          12

#define REG_BWR_CAM_HRT_TTL_ENG_BW_RAT6             0x540
#define F_BWR_CAM_HRT_TTL_ENG_BW_RAT6_POS                            0
#define F_BWR_CAM_HRT_TTL_ENG_BW_RAT6_WIDTH                          12

#define REG_BWR_CAM_HRT_TTL_ENG_BW_RAT7             0x544
#define F_BWR_CAM_HRT_TTL_ENG_BW_RAT7_POS                            0
#define F_BWR_CAM_HRT_TTL_ENG_BW_RAT7_WIDTH                          12

#define REG_BWR_CAM_HRT_TTL_ENG_BW_RAT8             0x548
#define F_BWR_CAM_HRT_TTL_ENG_BW_RAT8_POS                            0
#define F_BWR_CAM_HRT_TTL_ENG_BW_RAT8_WIDTH                          12

#define REG_BWR_CAM_HRT_TTL_ENG_BW_RAT9             0x54C
#define F_BWR_CAM_HRT_TTL_ENG_BW_RAT9_POS                            0
#define F_BWR_CAM_HRT_TTL_ENG_BW_RAT9_WIDTH                          12

#define REG_BWR_CAM_HRT_TTL_ENG_BW_RAT10            0x550
#define F_BWR_CAM_HRT_TTL_ENG_BW_RAT10_POS                           0
#define F_BWR_CAM_HRT_TTL_ENG_BW_RAT10_WIDTH                         12

#define REG_BWR_CAM_HRT_R0_BW_QOS_SEL0              0x570
#define F_BWR_CAM_HRT_R0_BW_QOS_SEL0_10_POS                          10
#define F_BWR_CAM_HRT_R0_BW_QOS_SEL0_10_WIDTH                        1
#define F_BWR_CAM_HRT_R0_BW_QOS_SEL0_9_POS                           9
#define F_BWR_CAM_HRT_R0_BW_QOS_SEL0_9_WIDTH                         1
#define F_BWR_CAM_HRT_R0_BW_QOS_SEL0_8_POS                           8
#define F_BWR_CAM_HRT_R0_BW_QOS_SEL0_8_WIDTH                         1
#define F_BWR_CAM_HRT_R0_BW_QOS_SEL0_7_POS                           7
#define F_BWR_CAM_HRT_R0_BW_QOS_SEL0_7_WIDTH                         1
#define F_BWR_CAM_HRT_R0_BW_QOS_SEL0_6_POS                           6
#define F_BWR_CAM_HRT_R0_BW_QOS_SEL0_6_WIDTH                         1
#define F_BWR_CAM_HRT_R0_BW_QOS_SEL0_5_POS                           5
#define F_BWR_CAM_HRT_R0_BW_QOS_SEL0_5_WIDTH                         1
#define F_BWR_CAM_HRT_R0_BW_QOS_SEL0_4_POS                           4
#define F_BWR_CAM_HRT_R0_BW_QOS_SEL0_4_WIDTH                         1
#define F_BWR_CAM_HRT_R0_BW_QOS_SEL0_3_POS                           3
#define F_BWR_CAM_HRT_R0_BW_QOS_SEL0_3_WIDTH                         1
#define F_BWR_CAM_HRT_R0_BW_QOS_SEL0_2_POS                           2
#define F_BWR_CAM_HRT_R0_BW_QOS_SEL0_2_WIDTH                         1
#define F_BWR_CAM_HRT_R0_BW_QOS_SEL0_1_POS                           1
#define F_BWR_CAM_HRT_R0_BW_QOS_SEL0_1_WIDTH                         1
#define F_BWR_CAM_HRT_R0_BW_QOS_SEL0_0_POS                           0
#define F_BWR_CAM_HRT_R0_BW_QOS_SEL0_0_WIDTH                         1

#define REG_BWR_CAM_HRT_R0_SW_QOS_TRIG0             0x574
#define F_BWR_CAM_HRT_R0_SW_QOS_TRIG0_10_POS                         10
#define F_BWR_CAM_HRT_R0_SW_QOS_TRIG0_10_WIDTH                       1
#define F_BWR_CAM_HRT_R0_SW_QOS_TRIG0_9_POS                          9
#define F_BWR_CAM_HRT_R0_SW_QOS_TRIG0_9_WIDTH                        1
#define F_BWR_CAM_HRT_R0_SW_QOS_TRIG0_8_POS                          8
#define F_BWR_CAM_HRT_R0_SW_QOS_TRIG0_8_WIDTH                        1
#define F_BWR_CAM_HRT_R0_SW_QOS_TRIG0_7_POS                          7
#define F_BWR_CAM_HRT_R0_SW_QOS_TRIG0_7_WIDTH                        1
#define F_BWR_CAM_HRT_R0_SW_QOS_TRIG0_6_POS                          6
#define F_BWR_CAM_HRT_R0_SW_QOS_TRIG0_6_WIDTH                        1
#define F_BWR_CAM_HRT_R0_SW_QOS_TRIG0_5_POS                          5
#define F_BWR_CAM_HRT_R0_SW_QOS_TRIG0_5_WIDTH                        1
#define F_BWR_CAM_HRT_R0_SW_QOS_TRIG0_4_POS                          4
#define F_BWR_CAM_HRT_R0_SW_QOS_TRIG0_4_WIDTH                        1
#define F_BWR_CAM_HRT_R0_SW_QOS_TRIG0_3_POS                          3
#define F_BWR_CAM_HRT_R0_SW_QOS_TRIG0_3_WIDTH                        1
#define F_BWR_CAM_HRT_R0_SW_QOS_TRIG0_2_POS                          2
#define F_BWR_CAM_HRT_R0_SW_QOS_TRIG0_2_WIDTH                        1
#define F_BWR_CAM_HRT_R0_SW_QOS_TRIG0_1_POS                          1
#define F_BWR_CAM_HRT_R0_SW_QOS_TRIG0_1_WIDTH                        1
#define F_BWR_CAM_HRT_R0_SW_QOS_TRIG0_0_POS                          0
#define F_BWR_CAM_HRT_R0_SW_QOS_TRIG0_0_WIDTH                        1

#define REG_BWR_CAM_HRT_R0_SW_QOS_EN0               0x578
#define F_BWR_CAM_HRT_R0_SW_QOS_EN0_10_POS                           10
#define F_BWR_CAM_HRT_R0_SW_QOS_EN0_10_WIDTH                         1
#define F_BWR_CAM_HRT_R0_SW_QOS_EN0_9_POS                            9
#define F_BWR_CAM_HRT_R0_SW_QOS_EN0_9_WIDTH                          1
#define F_BWR_CAM_HRT_R0_SW_QOS_EN0_8_POS                            8
#define F_BWR_CAM_HRT_R0_SW_QOS_EN0_8_WIDTH                          1
#define F_BWR_CAM_HRT_R0_SW_QOS_EN0_7_POS                            7
#define F_BWR_CAM_HRT_R0_SW_QOS_EN0_7_WIDTH                          1
#define F_BWR_CAM_HRT_R0_SW_QOS_EN0_6_POS                            6
#define F_BWR_CAM_HRT_R0_SW_QOS_EN0_6_WIDTH                          1
#define F_BWR_CAM_HRT_R0_SW_QOS_EN0_5_POS                            5
#define F_BWR_CAM_HRT_R0_SW_QOS_EN0_5_WIDTH                          1
#define F_BWR_CAM_HRT_R0_SW_QOS_EN0_4_POS                            4
#define F_BWR_CAM_HRT_R0_SW_QOS_EN0_4_WIDTH                          1
#define F_BWR_CAM_HRT_R0_SW_QOS_EN0_3_POS                            3
#define F_BWR_CAM_HRT_R0_SW_QOS_EN0_3_WIDTH                          1
#define F_BWR_CAM_HRT_R0_SW_QOS_EN0_2_POS                            2
#define F_BWR_CAM_HRT_R0_SW_QOS_EN0_2_WIDTH                          1
#define F_BWR_CAM_HRT_R0_SW_QOS_EN0_1_POS                            1
#define F_BWR_CAM_HRT_R0_SW_QOS_EN0_1_WIDTH                          1
#define F_BWR_CAM_HRT_R0_SW_QOS_EN0_0_POS                            0
#define F_BWR_CAM_HRT_R0_SW_QOS_EN0_0_WIDTH                          1

#define REG_BWR_CAM_HRT_R0_ENG_BW0_0                0x57C
#define F_BWR_CAM_HRT_R0_ENG_BW0_0_POS                               0
#define F_BWR_CAM_HRT_R0_ENG_BW0_0_WIDTH                             18

#define REG_BWR_CAM_HRT_R0_ENG_BW0_1                0x580
#define F_BWR_CAM_HRT_R0_ENG_BW0_1_POS                               0
#define F_BWR_CAM_HRT_R0_ENG_BW0_1_WIDTH                             18

#define REG_BWR_CAM_HRT_R0_ENG_BW0_2                0x584
#define F_BWR_CAM_HRT_R0_ENG_BW0_2_POS                               0
#define F_BWR_CAM_HRT_R0_ENG_BW0_2_WIDTH                             18

#define REG_BWR_CAM_HRT_R0_ENG_BW0_3                0x588
#define F_BWR_CAM_HRT_R0_ENG_BW0_3_POS                               0
#define F_BWR_CAM_HRT_R0_ENG_BW0_3_WIDTH                             18

#define REG_BWR_CAM_HRT_R0_ENG_BW0_4                0x58C
#define F_BWR_CAM_HRT_R0_ENG_BW0_4_POS                               0
#define F_BWR_CAM_HRT_R0_ENG_BW0_4_WIDTH                             18

#define REG_BWR_CAM_HRT_R0_ENG_BW0_5                0x590
#define F_BWR_CAM_HRT_R0_ENG_BW0_5_POS                               0
#define F_BWR_CAM_HRT_R0_ENG_BW0_5_WIDTH                             18

#define REG_BWR_CAM_HRT_R0_ENG_BW0_6                0x594
#define F_BWR_CAM_HRT_R0_ENG_BW0_6_POS                               0
#define F_BWR_CAM_HRT_R0_ENG_BW0_6_WIDTH                             18

#define REG_BWR_CAM_HRT_R0_ENG_BW0_7                0x598
#define F_BWR_CAM_HRT_R0_ENG_BW0_7_POS                               0
#define F_BWR_CAM_HRT_R0_ENG_BW0_7_WIDTH                             18

#define REG_BWR_CAM_HRT_R0_ENG_BW0_8                0x59C
#define F_BWR_CAM_HRT_R0_ENG_BW0_8_POS                               0
#define F_BWR_CAM_HRT_R0_ENG_BW0_8_WIDTH                             18

#define REG_BWR_CAM_HRT_R0_ENG_BW0_9                0x5A0
#define F_BWR_CAM_HRT_R0_ENG_BW0_9_POS                               0
#define F_BWR_CAM_HRT_R0_ENG_BW0_9_WIDTH                             18

#define REG_BWR_CAM_HRT_R0_ENG_BW0_10               0x5A4
#define F_BWR_CAM_HRT_R0_ENG_BW0_10_POS                              0
#define F_BWR_CAM_HRT_R0_ENG_BW0_10_WIDTH                            18

#define REG_BWR_CAM_HRT_R0_ENG_BW_RAT0_0            0x5A8
#define F_BWR_CAM_HRT_R0_ENG_BW_RAT0_0_POS                           0
#define F_BWR_CAM_HRT_R0_ENG_BW_RAT0_0_WIDTH                         12

#define REG_BWR_CAM_HRT_R0_ENG_BW_RAT0_1            0x5AC
#define F_BWR_CAM_HRT_R0_ENG_BW_RAT0_1_POS                           0
#define F_BWR_CAM_HRT_R0_ENG_BW_RAT0_1_WIDTH                         12

#define REG_BWR_CAM_HRT_R0_ENG_BW_RAT0_2            0x5B0
#define F_BWR_CAM_HRT_R0_ENG_BW_RAT0_2_POS                           0
#define F_BWR_CAM_HRT_R0_ENG_BW_RAT0_2_WIDTH                         12

#define REG_BWR_CAM_HRT_R0_ENG_BW_RAT0_3            0x5B4
#define F_BWR_CAM_HRT_R0_ENG_BW_RAT0_3_POS                           0
#define F_BWR_CAM_HRT_R0_ENG_BW_RAT0_3_WIDTH                         12

#define REG_BWR_CAM_HRT_R0_ENG_BW_RAT0_4            0x5B8
#define F_BWR_CAM_HRT_R0_ENG_BW_RAT0_4_POS                           0
#define F_BWR_CAM_HRT_R0_ENG_BW_RAT0_4_WIDTH                         12

#define REG_BWR_CAM_HRT_R0_ENG_BW_RAT0_5            0x5BC
#define F_BWR_CAM_HRT_R0_ENG_BW_RAT0_5_POS                           0
#define F_BWR_CAM_HRT_R0_ENG_BW_RAT0_5_WIDTH                         12

#define REG_BWR_CAM_HRT_R0_ENG_BW_RAT0_6            0x5C0
#define F_BWR_CAM_HRT_R0_ENG_BW_RAT0_6_POS                           0
#define F_BWR_CAM_HRT_R0_ENG_BW_RAT0_6_WIDTH                         12

#define REG_BWR_CAM_HRT_R0_ENG_BW_RAT0_7            0x5C4
#define F_BWR_CAM_HRT_R0_ENG_BW_RAT0_7_POS                           0
#define F_BWR_CAM_HRT_R0_ENG_BW_RAT0_7_WIDTH                         12

#define REG_BWR_CAM_HRT_R0_ENG_BW_RAT0_8            0x5C8
#define F_BWR_CAM_HRT_R0_ENG_BW_RAT0_8_POS                           0
#define F_BWR_CAM_HRT_R0_ENG_BW_RAT0_8_WIDTH                         12

#define REG_BWR_CAM_HRT_R0_ENG_BW_RAT0_9            0x5CC
#define F_BWR_CAM_HRT_R0_ENG_BW_RAT0_9_POS                           0
#define F_BWR_CAM_HRT_R0_ENG_BW_RAT0_9_WIDTH                         12

#define REG_BWR_CAM_HRT_R0_ENG_BW_RAT0_10           0x5D0
#define F_BWR_CAM_HRT_R0_ENG_BW_RAT0_10_POS                          0
#define F_BWR_CAM_HRT_R0_ENG_BW_RAT0_10_WIDTH                        12

#define REG_BWR_CAM_HRT_R1_BW_QOS_SEL0              0x5F0
#define F_BWR_CAM_HRT_R1_BW_QOS_SEL0_10_POS                          10
#define F_BWR_CAM_HRT_R1_BW_QOS_SEL0_10_WIDTH                        1
#define F_BWR_CAM_HRT_R1_BW_QOS_SEL0_9_POS                           9
#define F_BWR_CAM_HRT_R1_BW_QOS_SEL0_9_WIDTH                         1
#define F_BWR_CAM_HRT_R1_BW_QOS_SEL0_8_POS                           8
#define F_BWR_CAM_HRT_R1_BW_QOS_SEL0_8_WIDTH                         1
#define F_BWR_CAM_HRT_R1_BW_QOS_SEL0_7_POS                           7
#define F_BWR_CAM_HRT_R1_BW_QOS_SEL0_7_WIDTH                         1
#define F_BWR_CAM_HRT_R1_BW_QOS_SEL0_6_POS                           6
#define F_BWR_CAM_HRT_R1_BW_QOS_SEL0_6_WIDTH                         1
#define F_BWR_CAM_HRT_R1_BW_QOS_SEL0_5_POS                           5
#define F_BWR_CAM_HRT_R1_BW_QOS_SEL0_5_WIDTH                         1
#define F_BWR_CAM_HRT_R1_BW_QOS_SEL0_4_POS                           4
#define F_BWR_CAM_HRT_R1_BW_QOS_SEL0_4_WIDTH                         1
#define F_BWR_CAM_HRT_R1_BW_QOS_SEL0_3_POS                           3
#define F_BWR_CAM_HRT_R1_BW_QOS_SEL0_3_WIDTH                         1
#define F_BWR_CAM_HRT_R1_BW_QOS_SEL0_2_POS                           2
#define F_BWR_CAM_HRT_R1_BW_QOS_SEL0_2_WIDTH                         1
#define F_BWR_CAM_HRT_R1_BW_QOS_SEL0_1_POS                           1
#define F_BWR_CAM_HRT_R1_BW_QOS_SEL0_1_WIDTH                         1
#define F_BWR_CAM_HRT_R1_BW_QOS_SEL0_0_POS                           0
#define F_BWR_CAM_HRT_R1_BW_QOS_SEL0_0_WIDTH                         1

#define REG_BWR_CAM_HRT_R1_SW_QOS_TRIG0             0x5F4
#define F_BWR_CAM_HRT_R1_SW_QOS_TRIG0_10_POS                         10
#define F_BWR_CAM_HRT_R1_SW_QOS_TRIG0_10_WIDTH                       1
#define F_BWR_CAM_HRT_R1_SW_QOS_TRIG0_9_POS                          9
#define F_BWR_CAM_HRT_R1_SW_QOS_TRIG0_9_WIDTH                        1
#define F_BWR_CAM_HRT_R1_SW_QOS_TRIG0_8_POS                          8
#define F_BWR_CAM_HRT_R1_SW_QOS_TRIG0_8_WIDTH                        1
#define F_BWR_CAM_HRT_R1_SW_QOS_TRIG0_7_POS                          7
#define F_BWR_CAM_HRT_R1_SW_QOS_TRIG0_7_WIDTH                        1
#define F_BWR_CAM_HRT_R1_SW_QOS_TRIG0_6_POS                          6
#define F_BWR_CAM_HRT_R1_SW_QOS_TRIG0_6_WIDTH                        1
#define F_BWR_CAM_HRT_R1_SW_QOS_TRIG0_5_POS                          5
#define F_BWR_CAM_HRT_R1_SW_QOS_TRIG0_5_WIDTH                        1
#define F_BWR_CAM_HRT_R1_SW_QOS_TRIG0_4_POS                          4
#define F_BWR_CAM_HRT_R1_SW_QOS_TRIG0_4_WIDTH                        1
#define F_BWR_CAM_HRT_R1_SW_QOS_TRIG0_3_POS                          3
#define F_BWR_CAM_HRT_R1_SW_QOS_TRIG0_3_WIDTH                        1
#define F_BWR_CAM_HRT_R1_SW_QOS_TRIG0_2_POS                          2
#define F_BWR_CAM_HRT_R1_SW_QOS_TRIG0_2_WIDTH                        1
#define F_BWR_CAM_HRT_R1_SW_QOS_TRIG0_1_POS                          1
#define F_BWR_CAM_HRT_R1_SW_QOS_TRIG0_1_WIDTH                        1
#define F_BWR_CAM_HRT_R1_SW_QOS_TRIG0_0_POS                          0
#define F_BWR_CAM_HRT_R1_SW_QOS_TRIG0_0_WIDTH                        1

#define REG_BWR_CAM_HRT_R1_SW_QOS_EN0               0x5F8
#define F_BWR_CAM_HRT_R1_SW_QOS_EN0_10_POS                           10
#define F_BWR_CAM_HRT_R1_SW_QOS_EN0_10_WIDTH                         1
#define F_BWR_CAM_HRT_R1_SW_QOS_EN0_9_POS                            9
#define F_BWR_CAM_HRT_R1_SW_QOS_EN0_9_WIDTH                          1
#define F_BWR_CAM_HRT_R1_SW_QOS_EN0_8_POS                            8
#define F_BWR_CAM_HRT_R1_SW_QOS_EN0_8_WIDTH                          1
#define F_BWR_CAM_HRT_R1_SW_QOS_EN0_7_POS                            7
#define F_BWR_CAM_HRT_R1_SW_QOS_EN0_7_WIDTH                          1
#define F_BWR_CAM_HRT_R1_SW_QOS_EN0_6_POS                            6
#define F_BWR_CAM_HRT_R1_SW_QOS_EN0_6_WIDTH                          1
#define F_BWR_CAM_HRT_R1_SW_QOS_EN0_5_POS                            5
#define F_BWR_CAM_HRT_R1_SW_QOS_EN0_5_WIDTH                          1
#define F_BWR_CAM_HRT_R1_SW_QOS_EN0_4_POS                            4
#define F_BWR_CAM_HRT_R1_SW_QOS_EN0_4_WIDTH                          1
#define F_BWR_CAM_HRT_R1_SW_QOS_EN0_3_POS                            3
#define F_BWR_CAM_HRT_R1_SW_QOS_EN0_3_WIDTH                          1
#define F_BWR_CAM_HRT_R1_SW_QOS_EN0_2_POS                            2
#define F_BWR_CAM_HRT_R1_SW_QOS_EN0_2_WIDTH                          1
#define F_BWR_CAM_HRT_R1_SW_QOS_EN0_1_POS                            1
#define F_BWR_CAM_HRT_R1_SW_QOS_EN0_1_WIDTH                          1
#define F_BWR_CAM_HRT_R1_SW_QOS_EN0_0_POS                            0
#define F_BWR_CAM_HRT_R1_SW_QOS_EN0_0_WIDTH                          1

#define REG_BWR_CAM_HRT_R1_ENG_BW0_0                0x5FC
#define F_BWR_CAM_HRT_R1_ENG_BW0_0_POS                               0
#define F_BWR_CAM_HRT_R1_ENG_BW0_0_WIDTH                             18

#define REG_BWR_CAM_HRT_R1_ENG_BW0_1                0x600
#define F_BWR_CAM_HRT_R1_ENG_BW0_1_POS                               0
#define F_BWR_CAM_HRT_R1_ENG_BW0_1_WIDTH                             18

#define REG_BWR_CAM_HRT_R1_ENG_BW0_2                0x604
#define F_BWR_CAM_HRT_R1_ENG_BW0_2_POS                               0
#define F_BWR_CAM_HRT_R1_ENG_BW0_2_WIDTH                             18

#define REG_BWR_CAM_HRT_R1_ENG_BW0_3                0x608
#define F_BWR_CAM_HRT_R1_ENG_BW0_3_POS                               0
#define F_BWR_CAM_HRT_R1_ENG_BW0_3_WIDTH                             18

#define REG_BWR_CAM_HRT_R1_ENG_BW0_4                0x60C
#define F_BWR_CAM_HRT_R1_ENG_BW0_4_POS                               0
#define F_BWR_CAM_HRT_R1_ENG_BW0_4_WIDTH                             18

#define REG_BWR_CAM_HRT_R1_ENG_BW0_5                0x610
#define F_BWR_CAM_HRT_R1_ENG_BW0_5_POS                               0
#define F_BWR_CAM_HRT_R1_ENG_BW0_5_WIDTH                             18

#define REG_BWR_CAM_HRT_R1_ENG_BW0_6                0x614
#define F_BWR_CAM_HRT_R1_ENG_BW0_6_POS                               0
#define F_BWR_CAM_HRT_R1_ENG_BW0_6_WIDTH                             18

#define REG_BWR_CAM_HRT_R1_ENG_BW0_7                0x618
#define F_BWR_CAM_HRT_R1_ENG_BW0_7_POS                               0
#define F_BWR_CAM_HRT_R1_ENG_BW0_7_WIDTH                             18

#define REG_BWR_CAM_HRT_R1_ENG_BW0_8                0x61C
#define F_BWR_CAM_HRT_R1_ENG_BW0_8_POS                               0
#define F_BWR_CAM_HRT_R1_ENG_BW0_8_WIDTH                             18

#define REG_BWR_CAM_HRT_R1_ENG_BW0_9                0x620
#define F_BWR_CAM_HRT_R1_ENG_BW0_9_POS                               0
#define F_BWR_CAM_HRT_R1_ENG_BW0_9_WIDTH                             18

#define REG_BWR_CAM_HRT_R1_ENG_BW0_10               0x624
#define F_BWR_CAM_HRT_R1_ENG_BW0_10_POS                              0
#define F_BWR_CAM_HRT_R1_ENG_BW0_10_WIDTH                            18

#define REG_BWR_CAM_HRT_R1_ENG_BW_RAT0_0            0x628
#define F_BWR_CAM_HRT_R1_ENG_BW_RAT0_0_POS                           0
#define F_BWR_CAM_HRT_R1_ENG_BW_RAT0_0_WIDTH                         12

#define REG_BWR_CAM_HRT_R1_ENG_BW_RAT0_1            0x62C
#define F_BWR_CAM_HRT_R1_ENG_BW_RAT0_1_POS                           0
#define F_BWR_CAM_HRT_R1_ENG_BW_RAT0_1_WIDTH                         12

#define REG_BWR_CAM_HRT_R1_ENG_BW_RAT0_2            0x630
#define F_BWR_CAM_HRT_R1_ENG_BW_RAT0_2_POS                           0
#define F_BWR_CAM_HRT_R1_ENG_BW_RAT0_2_WIDTH                         12

#define REG_BWR_CAM_HRT_R1_ENG_BW_RAT0_3            0x634
#define F_BWR_CAM_HRT_R1_ENG_BW_RAT0_3_POS                           0
#define F_BWR_CAM_HRT_R1_ENG_BW_RAT0_3_WIDTH                         12

#define REG_BWR_CAM_HRT_R1_ENG_BW_RAT0_4            0x638
#define F_BWR_CAM_HRT_R1_ENG_BW_RAT0_4_POS                           0
#define F_BWR_CAM_HRT_R1_ENG_BW_RAT0_4_WIDTH                         12

#define REG_BWR_CAM_HRT_R1_ENG_BW_RAT0_5            0x63C
#define F_BWR_CAM_HRT_R1_ENG_BW_RAT0_5_POS                           0
#define F_BWR_CAM_HRT_R1_ENG_BW_RAT0_5_WIDTH                         12

#define REG_BWR_CAM_HRT_R1_ENG_BW_RAT0_6            0x640
#define F_BWR_CAM_HRT_R1_ENG_BW_RAT0_6_POS                           0
#define F_BWR_CAM_HRT_R1_ENG_BW_RAT0_6_WIDTH                         12

#define REG_BWR_CAM_HRT_R1_ENG_BW_RAT0_7            0x644
#define F_BWR_CAM_HRT_R1_ENG_BW_RAT0_7_POS                           0
#define F_BWR_CAM_HRT_R1_ENG_BW_RAT0_7_WIDTH                         12

#define REG_BWR_CAM_HRT_R1_ENG_BW_RAT0_8            0x648
#define F_BWR_CAM_HRT_R1_ENG_BW_RAT0_8_POS                           0
#define F_BWR_CAM_HRT_R1_ENG_BW_RAT0_8_WIDTH                         12

#define REG_BWR_CAM_HRT_R1_ENG_BW_RAT0_9            0x64C
#define F_BWR_CAM_HRT_R1_ENG_BW_RAT0_9_POS                           0
#define F_BWR_CAM_HRT_R1_ENG_BW_RAT0_9_WIDTH                         12

#define REG_BWR_CAM_HRT_R1_ENG_BW_RAT0_10           0x650
#define F_BWR_CAM_HRT_R1_ENG_BW_RAT0_10_POS                          0
#define F_BWR_CAM_HRT_R1_ENG_BW_RAT0_10_WIDTH                        12

#define REG_BWR_CAM_HRT_R2_BW_QOS_SEL0              0x670
#define F_BWR_CAM_HRT_R2_BW_QOS_SEL0_10_POS                          10
#define F_BWR_CAM_HRT_R2_BW_QOS_SEL0_10_WIDTH                        1
#define F_BWR_CAM_HRT_R2_BW_QOS_SEL0_9_POS                           9
#define F_BWR_CAM_HRT_R2_BW_QOS_SEL0_9_WIDTH                         1
#define F_BWR_CAM_HRT_R2_BW_QOS_SEL0_8_POS                           8
#define F_BWR_CAM_HRT_R2_BW_QOS_SEL0_8_WIDTH                         1
#define F_BWR_CAM_HRT_R2_BW_QOS_SEL0_7_POS                           7
#define F_BWR_CAM_HRT_R2_BW_QOS_SEL0_7_WIDTH                         1
#define F_BWR_CAM_HRT_R2_BW_QOS_SEL0_6_POS                           6
#define F_BWR_CAM_HRT_R2_BW_QOS_SEL0_6_WIDTH                         1
#define F_BWR_CAM_HRT_R2_BW_QOS_SEL0_5_POS                           5
#define F_BWR_CAM_HRT_R2_BW_QOS_SEL0_5_WIDTH                         1
#define F_BWR_CAM_HRT_R2_BW_QOS_SEL0_4_POS                           4
#define F_BWR_CAM_HRT_R2_BW_QOS_SEL0_4_WIDTH                         1
#define F_BWR_CAM_HRT_R2_BW_QOS_SEL0_3_POS                           3
#define F_BWR_CAM_HRT_R2_BW_QOS_SEL0_3_WIDTH                         1
#define F_BWR_CAM_HRT_R2_BW_QOS_SEL0_2_POS                           2
#define F_BWR_CAM_HRT_R2_BW_QOS_SEL0_2_WIDTH                         1
#define F_BWR_CAM_HRT_R2_BW_QOS_SEL0_1_POS                           1
#define F_BWR_CAM_HRT_R2_BW_QOS_SEL0_1_WIDTH                         1
#define F_BWR_CAM_HRT_R2_BW_QOS_SEL0_0_POS                           0
#define F_BWR_CAM_HRT_R2_BW_QOS_SEL0_0_WIDTH                         1

#define REG_BWR_CAM_HRT_R2_SW_QOS_TRIG0             0x674
#define F_BWR_CAM_HRT_R2_SW_QOS_TRIG0_10_POS                         10
#define F_BWR_CAM_HRT_R2_SW_QOS_TRIG0_10_WIDTH                       1
#define F_BWR_CAM_HRT_R2_SW_QOS_TRIG0_9_POS                          9
#define F_BWR_CAM_HRT_R2_SW_QOS_TRIG0_9_WIDTH                        1
#define F_BWR_CAM_HRT_R2_SW_QOS_TRIG0_8_POS                          8
#define F_BWR_CAM_HRT_R2_SW_QOS_TRIG0_8_WIDTH                        1
#define F_BWR_CAM_HRT_R2_SW_QOS_TRIG0_7_POS                          7
#define F_BWR_CAM_HRT_R2_SW_QOS_TRIG0_7_WIDTH                        1
#define F_BWR_CAM_HRT_R2_SW_QOS_TRIG0_6_POS                          6
#define F_BWR_CAM_HRT_R2_SW_QOS_TRIG0_6_WIDTH                        1
#define F_BWR_CAM_HRT_R2_SW_QOS_TRIG0_5_POS                          5
#define F_BWR_CAM_HRT_R2_SW_QOS_TRIG0_5_WIDTH                        1
#define F_BWR_CAM_HRT_R2_SW_QOS_TRIG0_4_POS                          4
#define F_BWR_CAM_HRT_R2_SW_QOS_TRIG0_4_WIDTH                        1
#define F_BWR_CAM_HRT_R2_SW_QOS_TRIG0_3_POS                          3
#define F_BWR_CAM_HRT_R2_SW_QOS_TRIG0_3_WIDTH                        1
#define F_BWR_CAM_HRT_R2_SW_QOS_TRIG0_2_POS                          2
#define F_BWR_CAM_HRT_R2_SW_QOS_TRIG0_2_WIDTH                        1
#define F_BWR_CAM_HRT_R2_SW_QOS_TRIG0_1_POS                          1
#define F_BWR_CAM_HRT_R2_SW_QOS_TRIG0_1_WIDTH                        1
#define F_BWR_CAM_HRT_R2_SW_QOS_TRIG0_0_POS                          0
#define F_BWR_CAM_HRT_R2_SW_QOS_TRIG0_0_WIDTH                        1

#define REG_BWR_CAM_HRT_R2_SW_QOS_EN0               0x678
#define F_BWR_CAM_HRT_R2_SW_QOS_EN0_10_POS                           10
#define F_BWR_CAM_HRT_R2_SW_QOS_EN0_10_WIDTH                         1
#define F_BWR_CAM_HRT_R2_SW_QOS_EN0_9_POS                            9
#define F_BWR_CAM_HRT_R2_SW_QOS_EN0_9_WIDTH                          1
#define F_BWR_CAM_HRT_R2_SW_QOS_EN0_8_POS                            8
#define F_BWR_CAM_HRT_R2_SW_QOS_EN0_8_WIDTH                          1
#define F_BWR_CAM_HRT_R2_SW_QOS_EN0_7_POS                            7
#define F_BWR_CAM_HRT_R2_SW_QOS_EN0_7_WIDTH                          1
#define F_BWR_CAM_HRT_R2_SW_QOS_EN0_6_POS                            6
#define F_BWR_CAM_HRT_R2_SW_QOS_EN0_6_WIDTH                          1
#define F_BWR_CAM_HRT_R2_SW_QOS_EN0_5_POS                            5
#define F_BWR_CAM_HRT_R2_SW_QOS_EN0_5_WIDTH                          1
#define F_BWR_CAM_HRT_R2_SW_QOS_EN0_4_POS                            4
#define F_BWR_CAM_HRT_R2_SW_QOS_EN0_4_WIDTH                          1
#define F_BWR_CAM_HRT_R2_SW_QOS_EN0_3_POS                            3
#define F_BWR_CAM_HRT_R2_SW_QOS_EN0_3_WIDTH                          1
#define F_BWR_CAM_HRT_R2_SW_QOS_EN0_2_POS                            2
#define F_BWR_CAM_HRT_R2_SW_QOS_EN0_2_WIDTH                          1
#define F_BWR_CAM_HRT_R2_SW_QOS_EN0_1_POS                            1
#define F_BWR_CAM_HRT_R2_SW_QOS_EN0_1_WIDTH                          1
#define F_BWR_CAM_HRT_R2_SW_QOS_EN0_0_POS                            0
#define F_BWR_CAM_HRT_R2_SW_QOS_EN0_0_WIDTH                          1

#define REG_BWR_CAM_HRT_R2_ENG_BW0_0                0x67C
#define F_BWR_CAM_HRT_R2_ENG_BW0_0_POS                               0
#define F_BWR_CAM_HRT_R2_ENG_BW0_0_WIDTH                             18

#define REG_BWR_CAM_HRT_R2_ENG_BW0_1                0x680
#define F_BWR_CAM_HRT_R2_ENG_BW0_1_POS                               0
#define F_BWR_CAM_HRT_R2_ENG_BW0_1_WIDTH                             18

#define REG_BWR_CAM_HRT_R2_ENG_BW0_2                0x684
#define F_BWR_CAM_HRT_R2_ENG_BW0_2_POS                               0
#define F_BWR_CAM_HRT_R2_ENG_BW0_2_WIDTH                             18

#define REG_BWR_CAM_HRT_R2_ENG_BW0_3                0x688
#define F_BWR_CAM_HRT_R2_ENG_BW0_3_POS                               0
#define F_BWR_CAM_HRT_R2_ENG_BW0_3_WIDTH                             18

#define REG_BWR_CAM_HRT_R2_ENG_BW0_4                0x68C
#define F_BWR_CAM_HRT_R2_ENG_BW0_4_POS                               0
#define F_BWR_CAM_HRT_R2_ENG_BW0_4_WIDTH                             18

#define REG_BWR_CAM_HRT_R2_ENG_BW0_5                0x690
#define F_BWR_CAM_HRT_R2_ENG_BW0_5_POS                               0
#define F_BWR_CAM_HRT_R2_ENG_BW0_5_WIDTH                             18

#define REG_BWR_CAM_HRT_R2_ENG_BW0_6                0x694
#define F_BWR_CAM_HRT_R2_ENG_BW0_6_POS                               0
#define F_BWR_CAM_HRT_R2_ENG_BW0_6_WIDTH                             18

#define REG_BWR_CAM_HRT_R2_ENG_BW0_7                0x698
#define F_BWR_CAM_HRT_R2_ENG_BW0_7_POS                               0
#define F_BWR_CAM_HRT_R2_ENG_BW0_7_WIDTH                             18

#define REG_BWR_CAM_HRT_R2_ENG_BW0_8                0x69C
#define F_BWR_CAM_HRT_R2_ENG_BW0_8_POS                               0
#define F_BWR_CAM_HRT_R2_ENG_BW0_8_WIDTH                             18

#define REG_BWR_CAM_HRT_R2_ENG_BW0_9                0x6A0
#define F_BWR_CAM_HRT_R2_ENG_BW0_9_POS                               0
#define F_BWR_CAM_HRT_R2_ENG_BW0_9_WIDTH                             18

#define REG_BWR_CAM_HRT_R2_ENG_BW0_10               0x6A4
#define F_BWR_CAM_HRT_R2_ENG_BW0_10_POS                              0
#define F_BWR_CAM_HRT_R2_ENG_BW0_10_WIDTH                            18

#define REG_BWR_CAM_HRT_R2_ENG_BW_RAT0_0            0x6A8
#define F_BWR_CAM_HRT_R2_ENG_BW_RAT0_0_POS                           0
#define F_BWR_CAM_HRT_R2_ENG_BW_RAT0_0_WIDTH                         12

#define REG_BWR_CAM_HRT_R2_ENG_BW_RAT0_1            0x6AC
#define F_BWR_CAM_HRT_R2_ENG_BW_RAT0_1_POS                           0
#define F_BWR_CAM_HRT_R2_ENG_BW_RAT0_1_WIDTH                         12

#define REG_BWR_CAM_HRT_R2_ENG_BW_RAT0_2            0x6B0
#define F_BWR_CAM_HRT_R2_ENG_BW_RAT0_2_POS                           0
#define F_BWR_CAM_HRT_R2_ENG_BW_RAT0_2_WIDTH                         12

#define REG_BWR_CAM_HRT_R2_ENG_BW_RAT0_3            0x6B4
#define F_BWR_CAM_HRT_R2_ENG_BW_RAT0_3_POS                           0
#define F_BWR_CAM_HRT_R2_ENG_BW_RAT0_3_WIDTH                         12

#define REG_BWR_CAM_HRT_R2_ENG_BW_RAT0_4            0x6B8
#define F_BWR_CAM_HRT_R2_ENG_BW_RAT0_4_POS                           0
#define F_BWR_CAM_HRT_R2_ENG_BW_RAT0_4_WIDTH                         12

#define REG_BWR_CAM_HRT_R2_ENG_BW_RAT0_5            0x6BC
#define F_BWR_CAM_HRT_R2_ENG_BW_RAT0_5_POS                           0
#define F_BWR_CAM_HRT_R2_ENG_BW_RAT0_5_WIDTH                         12

#define REG_BWR_CAM_HRT_R2_ENG_BW_RAT0_6            0x6C0
#define F_BWR_CAM_HRT_R2_ENG_BW_RAT0_6_POS                           0
#define F_BWR_CAM_HRT_R2_ENG_BW_RAT0_6_WIDTH                         12

#define REG_BWR_CAM_HRT_R2_ENG_BW_RAT0_7            0x6C4
#define F_BWR_CAM_HRT_R2_ENG_BW_RAT0_7_POS                           0
#define F_BWR_CAM_HRT_R2_ENG_BW_RAT0_7_WIDTH                         12

#define REG_BWR_CAM_HRT_R2_ENG_BW_RAT0_8            0x6C8
#define F_BWR_CAM_HRT_R2_ENG_BW_RAT0_8_POS                           0
#define F_BWR_CAM_HRT_R2_ENG_BW_RAT0_8_WIDTH                         12

#define REG_BWR_CAM_HRT_R2_ENG_BW_RAT0_9            0x6CC
#define F_BWR_CAM_HRT_R2_ENG_BW_RAT0_9_POS                           0
#define F_BWR_CAM_HRT_R2_ENG_BW_RAT0_9_WIDTH                         12

#define REG_BWR_CAM_HRT_R2_ENG_BW_RAT0_10           0x6D0
#define F_BWR_CAM_HRT_R2_ENG_BW_RAT0_10_POS                          0
#define F_BWR_CAM_HRT_R2_ENG_BW_RAT0_10_WIDTH                        12

#define REG_BWR_CAM_HRT_R3_BW_QOS_SEL0              0x6F0
#define F_BWR_CAM_HRT_R3_BW_QOS_SEL0_10_POS                          10
#define F_BWR_CAM_HRT_R3_BW_QOS_SEL0_10_WIDTH                        1
#define F_BWR_CAM_HRT_R3_BW_QOS_SEL0_9_POS                           9
#define F_BWR_CAM_HRT_R3_BW_QOS_SEL0_9_WIDTH                         1
#define F_BWR_CAM_HRT_R3_BW_QOS_SEL0_8_POS                           8
#define F_BWR_CAM_HRT_R3_BW_QOS_SEL0_8_WIDTH                         1
#define F_BWR_CAM_HRT_R3_BW_QOS_SEL0_7_POS                           7
#define F_BWR_CAM_HRT_R3_BW_QOS_SEL0_7_WIDTH                         1
#define F_BWR_CAM_HRT_R3_BW_QOS_SEL0_6_POS                           6
#define F_BWR_CAM_HRT_R3_BW_QOS_SEL0_6_WIDTH                         1
#define F_BWR_CAM_HRT_R3_BW_QOS_SEL0_5_POS                           5
#define F_BWR_CAM_HRT_R3_BW_QOS_SEL0_5_WIDTH                         1
#define F_BWR_CAM_HRT_R3_BW_QOS_SEL0_4_POS                           4
#define F_BWR_CAM_HRT_R3_BW_QOS_SEL0_4_WIDTH                         1
#define F_BWR_CAM_HRT_R3_BW_QOS_SEL0_3_POS                           3
#define F_BWR_CAM_HRT_R3_BW_QOS_SEL0_3_WIDTH                         1
#define F_BWR_CAM_HRT_R3_BW_QOS_SEL0_2_POS                           2
#define F_BWR_CAM_HRT_R3_BW_QOS_SEL0_2_WIDTH                         1
#define F_BWR_CAM_HRT_R3_BW_QOS_SEL0_1_POS                           1
#define F_BWR_CAM_HRT_R3_BW_QOS_SEL0_1_WIDTH                         1
#define F_BWR_CAM_HRT_R3_BW_QOS_SEL0_0_POS                           0
#define F_BWR_CAM_HRT_R3_BW_QOS_SEL0_0_WIDTH                         1

#define REG_BWR_CAM_HRT_R3_SW_QOS_TRIG0             0x6F4
#define F_BWR_CAM_HRT_R3_SW_QOS_TRIG0_10_POS                         10
#define F_BWR_CAM_HRT_R3_SW_QOS_TRIG0_10_WIDTH                       1
#define F_BWR_CAM_HRT_R3_SW_QOS_TRIG0_9_POS                          9
#define F_BWR_CAM_HRT_R3_SW_QOS_TRIG0_9_WIDTH                        1
#define F_BWR_CAM_HRT_R3_SW_QOS_TRIG0_8_POS                          8
#define F_BWR_CAM_HRT_R3_SW_QOS_TRIG0_8_WIDTH                        1
#define F_BWR_CAM_HRT_R3_SW_QOS_TRIG0_7_POS                          7
#define F_BWR_CAM_HRT_R3_SW_QOS_TRIG0_7_WIDTH                        1
#define F_BWR_CAM_HRT_R3_SW_QOS_TRIG0_6_POS                          6
#define F_BWR_CAM_HRT_R3_SW_QOS_TRIG0_6_WIDTH                        1
#define F_BWR_CAM_HRT_R3_SW_QOS_TRIG0_5_POS                          5
#define F_BWR_CAM_HRT_R3_SW_QOS_TRIG0_5_WIDTH                        1
#define F_BWR_CAM_HRT_R3_SW_QOS_TRIG0_4_POS                          4
#define F_BWR_CAM_HRT_R3_SW_QOS_TRIG0_4_WIDTH                        1
#define F_BWR_CAM_HRT_R3_SW_QOS_TRIG0_3_POS                          3
#define F_BWR_CAM_HRT_R3_SW_QOS_TRIG0_3_WIDTH                        1
#define F_BWR_CAM_HRT_R3_SW_QOS_TRIG0_2_POS                          2
#define F_BWR_CAM_HRT_R3_SW_QOS_TRIG0_2_WIDTH                        1
#define F_BWR_CAM_HRT_R3_SW_QOS_TRIG0_1_POS                          1
#define F_BWR_CAM_HRT_R3_SW_QOS_TRIG0_1_WIDTH                        1
#define F_BWR_CAM_HRT_R3_SW_QOS_TRIG0_0_POS                          0
#define F_BWR_CAM_HRT_R3_SW_QOS_TRIG0_0_WIDTH                        1

#define REG_BWR_CAM_HRT_R3_SW_QOS_EN0               0x6F8
#define F_BWR_CAM_HRT_R3_SW_QOS_EN0_10_POS                           10
#define F_BWR_CAM_HRT_R3_SW_QOS_EN0_10_WIDTH                         1
#define F_BWR_CAM_HRT_R3_SW_QOS_EN0_9_POS                            9
#define F_BWR_CAM_HRT_R3_SW_QOS_EN0_9_WIDTH                          1
#define F_BWR_CAM_HRT_R3_SW_QOS_EN0_8_POS                            8
#define F_BWR_CAM_HRT_R3_SW_QOS_EN0_8_WIDTH                          1
#define F_BWR_CAM_HRT_R3_SW_QOS_EN0_7_POS                            7
#define F_BWR_CAM_HRT_R3_SW_QOS_EN0_7_WIDTH                          1
#define F_BWR_CAM_HRT_R3_SW_QOS_EN0_6_POS                            6
#define F_BWR_CAM_HRT_R3_SW_QOS_EN0_6_WIDTH                          1
#define F_BWR_CAM_HRT_R3_SW_QOS_EN0_5_POS                            5
#define F_BWR_CAM_HRT_R3_SW_QOS_EN0_5_WIDTH                          1
#define F_BWR_CAM_HRT_R3_SW_QOS_EN0_4_POS                            4
#define F_BWR_CAM_HRT_R3_SW_QOS_EN0_4_WIDTH                          1
#define F_BWR_CAM_HRT_R3_SW_QOS_EN0_3_POS                            3
#define F_BWR_CAM_HRT_R3_SW_QOS_EN0_3_WIDTH                          1
#define F_BWR_CAM_HRT_R3_SW_QOS_EN0_2_POS                            2
#define F_BWR_CAM_HRT_R3_SW_QOS_EN0_2_WIDTH                          1
#define F_BWR_CAM_HRT_R3_SW_QOS_EN0_1_POS                            1
#define F_BWR_CAM_HRT_R3_SW_QOS_EN0_1_WIDTH                          1
#define F_BWR_CAM_HRT_R3_SW_QOS_EN0_0_POS                            0
#define F_BWR_CAM_HRT_R3_SW_QOS_EN0_0_WIDTH                          1

#define REG_BWR_CAM_HRT_R3_ENG_BW0_0                0x6FC
#define F_BWR_CAM_HRT_R3_ENG_BW0_0_POS                               0
#define F_BWR_CAM_HRT_R3_ENG_BW0_0_WIDTH                             18

#define REG_BWR_CAM_HRT_R3_ENG_BW0_1                0x700
#define F_BWR_CAM_HRT_R3_ENG_BW0_1_POS                               0
#define F_BWR_CAM_HRT_R3_ENG_BW0_1_WIDTH                             18

#define REG_BWR_CAM_HRT_R3_ENG_BW0_2                0x704
#define F_BWR_CAM_HRT_R3_ENG_BW0_2_POS                               0
#define F_BWR_CAM_HRT_R3_ENG_BW0_2_WIDTH                             18

#define REG_BWR_CAM_HRT_R3_ENG_BW0_3                0x708
#define F_BWR_CAM_HRT_R3_ENG_BW0_3_POS                               0
#define F_BWR_CAM_HRT_R3_ENG_BW0_3_WIDTH                             18

#define REG_BWR_CAM_HRT_R3_ENG_BW0_4                0x70C
#define F_BWR_CAM_HRT_R3_ENG_BW0_4_POS                               0
#define F_BWR_CAM_HRT_R3_ENG_BW0_4_WIDTH                             18

#define REG_BWR_CAM_HRT_R3_ENG_BW0_5                0x710
#define F_BWR_CAM_HRT_R3_ENG_BW0_5_POS                               0
#define F_BWR_CAM_HRT_R3_ENG_BW0_5_WIDTH                             18

#define REG_BWR_CAM_HRT_R3_ENG_BW0_6                0x714
#define F_BWR_CAM_HRT_R3_ENG_BW0_6_POS                               0
#define F_BWR_CAM_HRT_R3_ENG_BW0_6_WIDTH                             18

#define REG_BWR_CAM_HRT_R3_ENG_BW0_7                0x718
#define F_BWR_CAM_HRT_R3_ENG_BW0_7_POS                               0
#define F_BWR_CAM_HRT_R3_ENG_BW0_7_WIDTH                             18

#define REG_BWR_CAM_HRT_R3_ENG_BW0_8                0x71C
#define F_BWR_CAM_HRT_R3_ENG_BW0_8_POS                               0
#define F_BWR_CAM_HRT_R3_ENG_BW0_8_WIDTH                             18

#define REG_BWR_CAM_HRT_R3_ENG_BW0_9                0x720
#define F_BWR_CAM_HRT_R3_ENG_BW0_9_POS                               0
#define F_BWR_CAM_HRT_R3_ENG_BW0_9_WIDTH                             18

#define REG_BWR_CAM_HRT_R3_ENG_BW0_10               0x724
#define F_BWR_CAM_HRT_R3_ENG_BW0_10_POS                              0
#define F_BWR_CAM_HRT_R3_ENG_BW0_10_WIDTH                            18

#define REG_BWR_CAM_HRT_R3_ENG_BW_RAT0_0            0x728
#define F_BWR_CAM_HRT_R3_ENG_BW_RAT0_0_POS                           0
#define F_BWR_CAM_HRT_R3_ENG_BW_RAT0_0_WIDTH                         12

#define REG_BWR_CAM_HRT_R3_ENG_BW_RAT0_1            0x72C
#define F_BWR_CAM_HRT_R3_ENG_BW_RAT0_1_POS                           0
#define F_BWR_CAM_HRT_R3_ENG_BW_RAT0_1_WIDTH                         12

#define REG_BWR_CAM_HRT_R3_ENG_BW_RAT0_2            0x730
#define F_BWR_CAM_HRT_R3_ENG_BW_RAT0_2_POS                           0
#define F_BWR_CAM_HRT_R3_ENG_BW_RAT0_2_WIDTH                         12

#define REG_BWR_CAM_HRT_R3_ENG_BW_RAT0_3            0x734
#define F_BWR_CAM_HRT_R3_ENG_BW_RAT0_3_POS                           0
#define F_BWR_CAM_HRT_R3_ENG_BW_RAT0_3_WIDTH                         12

#define REG_BWR_CAM_HRT_R3_ENG_BW_RAT0_4            0x738
#define F_BWR_CAM_HRT_R3_ENG_BW_RAT0_4_POS                           0
#define F_BWR_CAM_HRT_R3_ENG_BW_RAT0_4_WIDTH                         12

#define REG_BWR_CAM_HRT_R3_ENG_BW_RAT0_5            0x73C
#define F_BWR_CAM_HRT_R3_ENG_BW_RAT0_5_POS                           0
#define F_BWR_CAM_HRT_R3_ENG_BW_RAT0_5_WIDTH                         12

#define REG_BWR_CAM_HRT_R3_ENG_BW_RAT0_6            0x740
#define F_BWR_CAM_HRT_R3_ENG_BW_RAT0_6_POS                           0
#define F_BWR_CAM_HRT_R3_ENG_BW_RAT0_6_WIDTH                         12

#define REG_BWR_CAM_HRT_R3_ENG_BW_RAT0_7            0x744
#define F_BWR_CAM_HRT_R3_ENG_BW_RAT0_7_POS                           0
#define F_BWR_CAM_HRT_R3_ENG_BW_RAT0_7_WIDTH                         12

#define REG_BWR_CAM_HRT_R3_ENG_BW_RAT0_8            0x748
#define F_BWR_CAM_HRT_R3_ENG_BW_RAT0_8_POS                           0
#define F_BWR_CAM_HRT_R3_ENG_BW_RAT0_8_WIDTH                         12

#define REG_BWR_CAM_HRT_R3_ENG_BW_RAT0_9            0x74C
#define F_BWR_CAM_HRT_R3_ENG_BW_RAT0_9_POS                           0
#define F_BWR_CAM_HRT_R3_ENG_BW_RAT0_9_WIDTH                         12

#define REG_BWR_CAM_HRT_R3_ENG_BW_RAT0_10           0x750
#define F_BWR_CAM_HRT_R3_ENG_BW_RAT0_10_POS                          0
#define F_BWR_CAM_HRT_R3_ENG_BW_RAT0_10_WIDTH                        12

#define REG_BWR_CAM_HRT_W0_BW_QOS_SEL0              0x770
#define F_BWR_CAM_HRT_W0_BW_QOS_SEL0_10_POS                          10
#define F_BWR_CAM_HRT_W0_BW_QOS_SEL0_10_WIDTH                        1
#define F_BWR_CAM_HRT_W0_BW_QOS_SEL0_9_POS                           9
#define F_BWR_CAM_HRT_W0_BW_QOS_SEL0_9_WIDTH                         1
#define F_BWR_CAM_HRT_W0_BW_QOS_SEL0_8_POS                           8
#define F_BWR_CAM_HRT_W0_BW_QOS_SEL0_8_WIDTH                         1
#define F_BWR_CAM_HRT_W0_BW_QOS_SEL0_7_POS                           7
#define F_BWR_CAM_HRT_W0_BW_QOS_SEL0_7_WIDTH                         1
#define F_BWR_CAM_HRT_W0_BW_QOS_SEL0_6_POS                           6
#define F_BWR_CAM_HRT_W0_BW_QOS_SEL0_6_WIDTH                         1
#define F_BWR_CAM_HRT_W0_BW_QOS_SEL0_5_POS                           5
#define F_BWR_CAM_HRT_W0_BW_QOS_SEL0_5_WIDTH                         1
#define F_BWR_CAM_HRT_W0_BW_QOS_SEL0_4_POS                           4
#define F_BWR_CAM_HRT_W0_BW_QOS_SEL0_4_WIDTH                         1
#define F_BWR_CAM_HRT_W0_BW_QOS_SEL0_3_POS                           3
#define F_BWR_CAM_HRT_W0_BW_QOS_SEL0_3_WIDTH                         1
#define F_BWR_CAM_HRT_W0_BW_QOS_SEL0_2_POS                           2
#define F_BWR_CAM_HRT_W0_BW_QOS_SEL0_2_WIDTH                         1
#define F_BWR_CAM_HRT_W0_BW_QOS_SEL0_1_POS                           1
#define F_BWR_CAM_HRT_W0_BW_QOS_SEL0_1_WIDTH                         1
#define F_BWR_CAM_HRT_W0_BW_QOS_SEL0_0_POS                           0
#define F_BWR_CAM_HRT_W0_BW_QOS_SEL0_0_WIDTH                         1

#define REG_BWR_CAM_HRT_W0_SW_QOS_TRIG0             0x774
#define F_BWR_CAM_HRT_W0_SW_QOS_TRIG0_10_POS                         10
#define F_BWR_CAM_HRT_W0_SW_QOS_TRIG0_10_WIDTH                       1
#define F_BWR_CAM_HRT_W0_SW_QOS_TRIG0_9_POS                          9
#define F_BWR_CAM_HRT_W0_SW_QOS_TRIG0_9_WIDTH                        1
#define F_BWR_CAM_HRT_W0_SW_QOS_TRIG0_8_POS                          8
#define F_BWR_CAM_HRT_W0_SW_QOS_TRIG0_8_WIDTH                        1
#define F_BWR_CAM_HRT_W0_SW_QOS_TRIG0_7_POS                          7
#define F_BWR_CAM_HRT_W0_SW_QOS_TRIG0_7_WIDTH                        1
#define F_BWR_CAM_HRT_W0_SW_QOS_TRIG0_6_POS                          6
#define F_BWR_CAM_HRT_W0_SW_QOS_TRIG0_6_WIDTH                        1
#define F_BWR_CAM_HRT_W0_SW_QOS_TRIG0_5_POS                          5
#define F_BWR_CAM_HRT_W0_SW_QOS_TRIG0_5_WIDTH                        1
#define F_BWR_CAM_HRT_W0_SW_QOS_TRIG0_4_POS                          4
#define F_BWR_CAM_HRT_W0_SW_QOS_TRIG0_4_WIDTH                        1
#define F_BWR_CAM_HRT_W0_SW_QOS_TRIG0_3_POS                          3
#define F_BWR_CAM_HRT_W0_SW_QOS_TRIG0_3_WIDTH                        1
#define F_BWR_CAM_HRT_W0_SW_QOS_TRIG0_2_POS                          2
#define F_BWR_CAM_HRT_W0_SW_QOS_TRIG0_2_WIDTH                        1
#define F_BWR_CAM_HRT_W0_SW_QOS_TRIG0_1_POS                          1
#define F_BWR_CAM_HRT_W0_SW_QOS_TRIG0_1_WIDTH                        1
#define F_BWR_CAM_HRT_W0_SW_QOS_TRIG0_0_POS                          0
#define F_BWR_CAM_HRT_W0_SW_QOS_TRIG0_0_WIDTH                        1

#define REG_BWR_CAM_HRT_W0_SW_QOS_EN0               0x778
#define F_BWR_CAM_HRT_W0_SW_QOS_EN0_10_POS                           10
#define F_BWR_CAM_HRT_W0_SW_QOS_EN0_10_WIDTH                         1
#define F_BWR_CAM_HRT_W0_SW_QOS_EN0_9_POS                            9
#define F_BWR_CAM_HRT_W0_SW_QOS_EN0_9_WIDTH                          1
#define F_BWR_CAM_HRT_W0_SW_QOS_EN0_8_POS                            8
#define F_BWR_CAM_HRT_W0_SW_QOS_EN0_8_WIDTH                          1
#define F_BWR_CAM_HRT_W0_SW_QOS_EN0_7_POS                            7
#define F_BWR_CAM_HRT_W0_SW_QOS_EN0_7_WIDTH                          1
#define F_BWR_CAM_HRT_W0_SW_QOS_EN0_6_POS                            6
#define F_BWR_CAM_HRT_W0_SW_QOS_EN0_6_WIDTH                          1
#define F_BWR_CAM_HRT_W0_SW_QOS_EN0_5_POS                            5
#define F_BWR_CAM_HRT_W0_SW_QOS_EN0_5_WIDTH                          1
#define F_BWR_CAM_HRT_W0_SW_QOS_EN0_4_POS                            4
#define F_BWR_CAM_HRT_W0_SW_QOS_EN0_4_WIDTH                          1
#define F_BWR_CAM_HRT_W0_SW_QOS_EN0_3_POS                            3
#define F_BWR_CAM_HRT_W0_SW_QOS_EN0_3_WIDTH                          1
#define F_BWR_CAM_HRT_W0_SW_QOS_EN0_2_POS                            2
#define F_BWR_CAM_HRT_W0_SW_QOS_EN0_2_WIDTH                          1
#define F_BWR_CAM_HRT_W0_SW_QOS_EN0_1_POS                            1
#define F_BWR_CAM_HRT_W0_SW_QOS_EN0_1_WIDTH                          1
#define F_BWR_CAM_HRT_W0_SW_QOS_EN0_0_POS                            0
#define F_BWR_CAM_HRT_W0_SW_QOS_EN0_0_WIDTH                          1

#define REG_BWR_CAM_HRT_W0_ENG_BW0_0                0x77C
#define F_BWR_CAM_HRT_W0_ENG_BW0_0_POS                               0
#define F_BWR_CAM_HRT_W0_ENG_BW0_0_WIDTH                             18

#define REG_BWR_CAM_HRT_W0_ENG_BW0_1                0x780
#define F_BWR_CAM_HRT_W0_ENG_BW0_1_POS                               0
#define F_BWR_CAM_HRT_W0_ENG_BW0_1_WIDTH                             18

#define REG_BWR_CAM_HRT_W0_ENG_BW0_2                0x784
#define F_BWR_CAM_HRT_W0_ENG_BW0_2_POS                               0
#define F_BWR_CAM_HRT_W0_ENG_BW0_2_WIDTH                             18

#define REG_BWR_CAM_HRT_W0_ENG_BW0_3                0x788
#define F_BWR_CAM_HRT_W0_ENG_BW0_3_POS                               0
#define F_BWR_CAM_HRT_W0_ENG_BW0_3_WIDTH                             18

#define REG_BWR_CAM_HRT_W0_ENG_BW0_4                0x78C
#define F_BWR_CAM_HRT_W0_ENG_BW0_4_POS                               0
#define F_BWR_CAM_HRT_W0_ENG_BW0_4_WIDTH                             18

#define REG_BWR_CAM_HRT_W0_ENG_BW0_5                0x790
#define F_BWR_CAM_HRT_W0_ENG_BW0_5_POS                               0
#define F_BWR_CAM_HRT_W0_ENG_BW0_5_WIDTH                             18

#define REG_BWR_CAM_HRT_W0_ENG_BW0_6                0x794
#define F_BWR_CAM_HRT_W0_ENG_BW0_6_POS                               0
#define F_BWR_CAM_HRT_W0_ENG_BW0_6_WIDTH                             18

#define REG_BWR_CAM_HRT_W0_ENG_BW0_7                0x798
#define F_BWR_CAM_HRT_W0_ENG_BW0_7_POS                               0
#define F_BWR_CAM_HRT_W0_ENG_BW0_7_WIDTH                             18

#define REG_BWR_CAM_HRT_W0_ENG_BW0_8                0x79C
#define F_BWR_CAM_HRT_W0_ENG_BW0_8_POS                               0
#define F_BWR_CAM_HRT_W0_ENG_BW0_8_WIDTH                             18

#define REG_BWR_CAM_HRT_W0_ENG_BW0_9                0x7A0
#define F_BWR_CAM_HRT_W0_ENG_BW0_9_POS                               0
#define F_BWR_CAM_HRT_W0_ENG_BW0_9_WIDTH                             18

#define REG_BWR_CAM_HRT_W0_ENG_BW0_10               0x7A4
#define F_BWR_CAM_HRT_W0_ENG_BW0_10_POS                              0
#define F_BWR_CAM_HRT_W0_ENG_BW0_10_WIDTH                            18

#define REG_BWR_CAM_HRT_W0_ENG_BW_RAT0_0            0x7A8
#define F_BWR_CAM_HRT_W0_ENG_BW_RAT0_0_POS                           0
#define F_BWR_CAM_HRT_W0_ENG_BW_RAT0_0_WIDTH                         12

#define REG_BWR_CAM_HRT_W0_ENG_BW_RAT0_1            0x7AC
#define F_BWR_CAM_HRT_W0_ENG_BW_RAT0_1_POS                           0
#define F_BWR_CAM_HRT_W0_ENG_BW_RAT0_1_WIDTH                         12

#define REG_BWR_CAM_HRT_W0_ENG_BW_RAT0_2            0x7B0
#define F_BWR_CAM_HRT_W0_ENG_BW_RAT0_2_POS                           0
#define F_BWR_CAM_HRT_W0_ENG_BW_RAT0_2_WIDTH                         12

#define REG_BWR_CAM_HRT_W0_ENG_BW_RAT0_3            0x7B4
#define F_BWR_CAM_HRT_W0_ENG_BW_RAT0_3_POS                           0
#define F_BWR_CAM_HRT_W0_ENG_BW_RAT0_3_WIDTH                         12

#define REG_BWR_CAM_HRT_W0_ENG_BW_RAT0_4            0x7B8
#define F_BWR_CAM_HRT_W0_ENG_BW_RAT0_4_POS                           0
#define F_BWR_CAM_HRT_W0_ENG_BW_RAT0_4_WIDTH                         12

#define REG_BWR_CAM_HRT_W0_ENG_BW_RAT0_5            0x7BC
#define F_BWR_CAM_HRT_W0_ENG_BW_RAT0_5_POS                           0
#define F_BWR_CAM_HRT_W0_ENG_BW_RAT0_5_WIDTH                         12

#define REG_BWR_CAM_HRT_W0_ENG_BW_RAT0_6            0x7C0
#define F_BWR_CAM_HRT_W0_ENG_BW_RAT0_6_POS                           0
#define F_BWR_CAM_HRT_W0_ENG_BW_RAT0_6_WIDTH                         12

#define REG_BWR_CAM_HRT_W0_ENG_BW_RAT0_7            0x7C4
#define F_BWR_CAM_HRT_W0_ENG_BW_RAT0_7_POS                           0
#define F_BWR_CAM_HRT_W0_ENG_BW_RAT0_7_WIDTH                         12

#define REG_BWR_CAM_HRT_W0_ENG_BW_RAT0_8            0x7C8
#define F_BWR_CAM_HRT_W0_ENG_BW_RAT0_8_POS                           0
#define F_BWR_CAM_HRT_W0_ENG_BW_RAT0_8_WIDTH                         12

#define REG_BWR_CAM_HRT_W0_ENG_BW_RAT0_9            0x7CC
#define F_BWR_CAM_HRT_W0_ENG_BW_RAT0_9_POS                           0
#define F_BWR_CAM_HRT_W0_ENG_BW_RAT0_9_WIDTH                         12

#define REG_BWR_CAM_HRT_W0_ENG_BW_RAT0_10           0x7D0
#define F_BWR_CAM_HRT_W0_ENG_BW_RAT0_10_POS                          0
#define F_BWR_CAM_HRT_W0_ENG_BW_RAT0_10_WIDTH                        12

#define REG_BWR_CAM_HRT_W1_BW_QOS_SEL0              0x7F0
#define F_BWR_CAM_HRT_W1_BW_QOS_SEL0_10_POS                          10
#define F_BWR_CAM_HRT_W1_BW_QOS_SEL0_10_WIDTH                        1
#define F_BWR_CAM_HRT_W1_BW_QOS_SEL0_9_POS                           9
#define F_BWR_CAM_HRT_W1_BW_QOS_SEL0_9_WIDTH                         1
#define F_BWR_CAM_HRT_W1_BW_QOS_SEL0_8_POS                           8
#define F_BWR_CAM_HRT_W1_BW_QOS_SEL0_8_WIDTH                         1
#define F_BWR_CAM_HRT_W1_BW_QOS_SEL0_7_POS                           7
#define F_BWR_CAM_HRT_W1_BW_QOS_SEL0_7_WIDTH                         1
#define F_BWR_CAM_HRT_W1_BW_QOS_SEL0_6_POS                           6
#define F_BWR_CAM_HRT_W1_BW_QOS_SEL0_6_WIDTH                         1
#define F_BWR_CAM_HRT_W1_BW_QOS_SEL0_5_POS                           5
#define F_BWR_CAM_HRT_W1_BW_QOS_SEL0_5_WIDTH                         1
#define F_BWR_CAM_HRT_W1_BW_QOS_SEL0_4_POS                           4
#define F_BWR_CAM_HRT_W1_BW_QOS_SEL0_4_WIDTH                         1
#define F_BWR_CAM_HRT_W1_BW_QOS_SEL0_3_POS                           3
#define F_BWR_CAM_HRT_W1_BW_QOS_SEL0_3_WIDTH                         1
#define F_BWR_CAM_HRT_W1_BW_QOS_SEL0_2_POS                           2
#define F_BWR_CAM_HRT_W1_BW_QOS_SEL0_2_WIDTH                         1
#define F_BWR_CAM_HRT_W1_BW_QOS_SEL0_1_POS                           1
#define F_BWR_CAM_HRT_W1_BW_QOS_SEL0_1_WIDTH                         1
#define F_BWR_CAM_HRT_W1_BW_QOS_SEL0_0_POS                           0
#define F_BWR_CAM_HRT_W1_BW_QOS_SEL0_0_WIDTH                         1

#define REG_BWR_CAM_HRT_W1_SW_QOS_TRIG0             0x7F4
#define F_BWR_CAM_HRT_W1_SW_QOS_TRIG0_10_POS                         10
#define F_BWR_CAM_HRT_W1_SW_QOS_TRIG0_10_WIDTH                       1
#define F_BWR_CAM_HRT_W1_SW_QOS_TRIG0_9_POS                          9
#define F_BWR_CAM_HRT_W1_SW_QOS_TRIG0_9_WIDTH                        1
#define F_BWR_CAM_HRT_W1_SW_QOS_TRIG0_8_POS                          8
#define F_BWR_CAM_HRT_W1_SW_QOS_TRIG0_8_WIDTH                        1
#define F_BWR_CAM_HRT_W1_SW_QOS_TRIG0_7_POS                          7
#define F_BWR_CAM_HRT_W1_SW_QOS_TRIG0_7_WIDTH                        1
#define F_BWR_CAM_HRT_W1_SW_QOS_TRIG0_6_POS                          6
#define F_BWR_CAM_HRT_W1_SW_QOS_TRIG0_6_WIDTH                        1
#define F_BWR_CAM_HRT_W1_SW_QOS_TRIG0_5_POS                          5
#define F_BWR_CAM_HRT_W1_SW_QOS_TRIG0_5_WIDTH                        1
#define F_BWR_CAM_HRT_W1_SW_QOS_TRIG0_4_POS                          4
#define F_BWR_CAM_HRT_W1_SW_QOS_TRIG0_4_WIDTH                        1
#define F_BWR_CAM_HRT_W1_SW_QOS_TRIG0_3_POS                          3
#define F_BWR_CAM_HRT_W1_SW_QOS_TRIG0_3_WIDTH                        1
#define F_BWR_CAM_HRT_W1_SW_QOS_TRIG0_2_POS                          2
#define F_BWR_CAM_HRT_W1_SW_QOS_TRIG0_2_WIDTH                        1
#define F_BWR_CAM_HRT_W1_SW_QOS_TRIG0_1_POS                          1
#define F_BWR_CAM_HRT_W1_SW_QOS_TRIG0_1_WIDTH                        1
#define F_BWR_CAM_HRT_W1_SW_QOS_TRIG0_0_POS                          0
#define F_BWR_CAM_HRT_W1_SW_QOS_TRIG0_0_WIDTH                        1

#define REG_BWR_CAM_HRT_W1_SW_QOS_EN0               0x7F8
#define F_BWR_CAM_HRT_W1_SW_QOS_EN0_10_POS                           10
#define F_BWR_CAM_HRT_W1_SW_QOS_EN0_10_WIDTH                         1
#define F_BWR_CAM_HRT_W1_SW_QOS_EN0_9_POS                            9
#define F_BWR_CAM_HRT_W1_SW_QOS_EN0_9_WIDTH                          1
#define F_BWR_CAM_HRT_W1_SW_QOS_EN0_8_POS                            8
#define F_BWR_CAM_HRT_W1_SW_QOS_EN0_8_WIDTH                          1
#define F_BWR_CAM_HRT_W1_SW_QOS_EN0_7_POS                            7
#define F_BWR_CAM_HRT_W1_SW_QOS_EN0_7_WIDTH                          1
#define F_BWR_CAM_HRT_W1_SW_QOS_EN0_6_POS                            6
#define F_BWR_CAM_HRT_W1_SW_QOS_EN0_6_WIDTH                          1
#define F_BWR_CAM_HRT_W1_SW_QOS_EN0_5_POS                            5
#define F_BWR_CAM_HRT_W1_SW_QOS_EN0_5_WIDTH                          1
#define F_BWR_CAM_HRT_W1_SW_QOS_EN0_4_POS                            4
#define F_BWR_CAM_HRT_W1_SW_QOS_EN0_4_WIDTH                          1
#define F_BWR_CAM_HRT_W1_SW_QOS_EN0_3_POS                            3
#define F_BWR_CAM_HRT_W1_SW_QOS_EN0_3_WIDTH                          1
#define F_BWR_CAM_HRT_W1_SW_QOS_EN0_2_POS                            2
#define F_BWR_CAM_HRT_W1_SW_QOS_EN0_2_WIDTH                          1
#define F_BWR_CAM_HRT_W1_SW_QOS_EN0_1_POS                            1
#define F_BWR_CAM_HRT_W1_SW_QOS_EN0_1_WIDTH                          1
#define F_BWR_CAM_HRT_W1_SW_QOS_EN0_0_POS                            0
#define F_BWR_CAM_HRT_W1_SW_QOS_EN0_0_WIDTH                          1

#define REG_BWR_CAM_HRT_W1_ENG_BW0_0                0x7FC
#define F_BWR_CAM_HRT_W1_ENG_BW0_0_POS                               0
#define F_BWR_CAM_HRT_W1_ENG_BW0_0_WIDTH                             18

#define REG_BWR_CAM_HRT_W1_ENG_BW0_1                0x800
#define F_BWR_CAM_HRT_W1_ENG_BW0_1_POS                               0
#define F_BWR_CAM_HRT_W1_ENG_BW0_1_WIDTH                             18

#define REG_BWR_CAM_HRT_W1_ENG_BW0_2                0x804
#define F_BWR_CAM_HRT_W1_ENG_BW0_2_POS                               0
#define F_BWR_CAM_HRT_W1_ENG_BW0_2_WIDTH                             18

#define REG_BWR_CAM_HRT_W1_ENG_BW0_3                0x808
#define F_BWR_CAM_HRT_W1_ENG_BW0_3_POS                               0
#define F_BWR_CAM_HRT_W1_ENG_BW0_3_WIDTH                             18

#define REG_BWR_CAM_HRT_W1_ENG_BW0_4                0x80C
#define F_BWR_CAM_HRT_W1_ENG_BW0_4_POS                               0
#define F_BWR_CAM_HRT_W1_ENG_BW0_4_WIDTH                             18

#define REG_BWR_CAM_HRT_W1_ENG_BW0_5                0x810
#define F_BWR_CAM_HRT_W1_ENG_BW0_5_POS                               0
#define F_BWR_CAM_HRT_W1_ENG_BW0_5_WIDTH                             18

#define REG_BWR_CAM_HRT_W1_ENG_BW0_6                0x814
#define F_BWR_CAM_HRT_W1_ENG_BW0_6_POS                               0
#define F_BWR_CAM_HRT_W1_ENG_BW0_6_WIDTH                             18

#define REG_BWR_CAM_HRT_W1_ENG_BW0_7                0x818
#define F_BWR_CAM_HRT_W1_ENG_BW0_7_POS                               0
#define F_BWR_CAM_HRT_W1_ENG_BW0_7_WIDTH                             18

#define REG_BWR_CAM_HRT_W1_ENG_BW0_8                0x81C
#define F_BWR_CAM_HRT_W1_ENG_BW0_8_POS                               0
#define F_BWR_CAM_HRT_W1_ENG_BW0_8_WIDTH                             18

#define REG_BWR_CAM_HRT_W1_ENG_BW0_9                0x820
#define F_BWR_CAM_HRT_W1_ENG_BW0_9_POS                               0
#define F_BWR_CAM_HRT_W1_ENG_BW0_9_WIDTH                             18

#define REG_BWR_CAM_HRT_W1_ENG_BW0_10               0x824
#define F_BWR_CAM_HRT_W1_ENG_BW0_10_POS                              0
#define F_BWR_CAM_HRT_W1_ENG_BW0_10_WIDTH                            18

#define REG_BWR_CAM_HRT_W1_ENG_BW_RAT0_0            0x828
#define F_BWR_CAM_HRT_W1_ENG_BW_RAT0_0_POS                           0
#define F_BWR_CAM_HRT_W1_ENG_BW_RAT0_0_WIDTH                         12

#define REG_BWR_CAM_HRT_W1_ENG_BW_RAT0_1            0x82C
#define F_BWR_CAM_HRT_W1_ENG_BW_RAT0_1_POS                           0
#define F_BWR_CAM_HRT_W1_ENG_BW_RAT0_1_WIDTH                         12

#define REG_BWR_CAM_HRT_W1_ENG_BW_RAT0_2            0x830
#define F_BWR_CAM_HRT_W1_ENG_BW_RAT0_2_POS                           0
#define F_BWR_CAM_HRT_W1_ENG_BW_RAT0_2_WIDTH                         12

#define REG_BWR_CAM_HRT_W1_ENG_BW_RAT0_3            0x834
#define F_BWR_CAM_HRT_W1_ENG_BW_RAT0_3_POS                           0
#define F_BWR_CAM_HRT_W1_ENG_BW_RAT0_3_WIDTH                         12

#define REG_BWR_CAM_HRT_W1_ENG_BW_RAT0_4            0x838
#define F_BWR_CAM_HRT_W1_ENG_BW_RAT0_4_POS                           0
#define F_BWR_CAM_HRT_W1_ENG_BW_RAT0_4_WIDTH                         12

#define REG_BWR_CAM_HRT_W1_ENG_BW_RAT0_5            0x83C
#define F_BWR_CAM_HRT_W1_ENG_BW_RAT0_5_POS                           0
#define F_BWR_CAM_HRT_W1_ENG_BW_RAT0_5_WIDTH                         12

#define REG_BWR_CAM_HRT_W1_ENG_BW_RAT0_6            0x840
#define F_BWR_CAM_HRT_W1_ENG_BW_RAT0_6_POS                           0
#define F_BWR_CAM_HRT_W1_ENG_BW_RAT0_6_WIDTH                         12

#define REG_BWR_CAM_HRT_W1_ENG_BW_RAT0_7            0x844
#define F_BWR_CAM_HRT_W1_ENG_BW_RAT0_7_POS                           0
#define F_BWR_CAM_HRT_W1_ENG_BW_RAT0_7_WIDTH                         12

#define REG_BWR_CAM_HRT_W1_ENG_BW_RAT0_8            0x848
#define F_BWR_CAM_HRT_W1_ENG_BW_RAT0_8_POS                           0
#define F_BWR_CAM_HRT_W1_ENG_BW_RAT0_8_WIDTH                         12

#define REG_BWR_CAM_HRT_W1_ENG_BW_RAT0_9            0x84C
#define F_BWR_CAM_HRT_W1_ENG_BW_RAT0_9_POS                           0
#define F_BWR_CAM_HRT_W1_ENG_BW_RAT0_9_WIDTH                         12

#define REG_BWR_CAM_HRT_W1_ENG_BW_RAT0_10           0x850
#define F_BWR_CAM_HRT_W1_ENG_BW_RAT0_10_POS                          0
#define F_BWR_CAM_HRT_W1_ENG_BW_RAT0_10_WIDTH                        12

#define REG_BWR_CAM_HRT_W2_BW_QOS_SEL0              0x870
#define F_BWR_CAM_HRT_W2_BW_QOS_SEL0_10_POS                          10
#define F_BWR_CAM_HRT_W2_BW_QOS_SEL0_10_WIDTH                        1
#define F_BWR_CAM_HRT_W2_BW_QOS_SEL0_9_POS                           9
#define F_BWR_CAM_HRT_W2_BW_QOS_SEL0_9_WIDTH                         1
#define F_BWR_CAM_HRT_W2_BW_QOS_SEL0_8_POS                           8
#define F_BWR_CAM_HRT_W2_BW_QOS_SEL0_8_WIDTH                         1
#define F_BWR_CAM_HRT_W2_BW_QOS_SEL0_7_POS                           7
#define F_BWR_CAM_HRT_W2_BW_QOS_SEL0_7_WIDTH                         1
#define F_BWR_CAM_HRT_W2_BW_QOS_SEL0_6_POS                           6
#define F_BWR_CAM_HRT_W2_BW_QOS_SEL0_6_WIDTH                         1
#define F_BWR_CAM_HRT_W2_BW_QOS_SEL0_5_POS                           5
#define F_BWR_CAM_HRT_W2_BW_QOS_SEL0_5_WIDTH                         1
#define F_BWR_CAM_HRT_W2_BW_QOS_SEL0_4_POS                           4
#define F_BWR_CAM_HRT_W2_BW_QOS_SEL0_4_WIDTH                         1
#define F_BWR_CAM_HRT_W2_BW_QOS_SEL0_3_POS                           3
#define F_BWR_CAM_HRT_W2_BW_QOS_SEL0_3_WIDTH                         1
#define F_BWR_CAM_HRT_W2_BW_QOS_SEL0_2_POS                           2
#define F_BWR_CAM_HRT_W2_BW_QOS_SEL0_2_WIDTH                         1
#define F_BWR_CAM_HRT_W2_BW_QOS_SEL0_1_POS                           1
#define F_BWR_CAM_HRT_W2_BW_QOS_SEL0_1_WIDTH                         1
#define F_BWR_CAM_HRT_W2_BW_QOS_SEL0_0_POS                           0
#define F_BWR_CAM_HRT_W2_BW_QOS_SEL0_0_WIDTH                         1

#define REG_BWR_CAM_HRT_W2_SW_QOS_TRIG0             0x874
#define F_BWR_CAM_HRT_W2_SW_QOS_TRIG0_10_POS                         10
#define F_BWR_CAM_HRT_W2_SW_QOS_TRIG0_10_WIDTH                       1
#define F_BWR_CAM_HRT_W2_SW_QOS_TRIG0_9_POS                          9
#define F_BWR_CAM_HRT_W2_SW_QOS_TRIG0_9_WIDTH                        1
#define F_BWR_CAM_HRT_W2_SW_QOS_TRIG0_8_POS                          8
#define F_BWR_CAM_HRT_W2_SW_QOS_TRIG0_8_WIDTH                        1
#define F_BWR_CAM_HRT_W2_SW_QOS_TRIG0_7_POS                          7
#define F_BWR_CAM_HRT_W2_SW_QOS_TRIG0_7_WIDTH                        1
#define F_BWR_CAM_HRT_W2_SW_QOS_TRIG0_6_POS                          6
#define F_BWR_CAM_HRT_W2_SW_QOS_TRIG0_6_WIDTH                        1
#define F_BWR_CAM_HRT_W2_SW_QOS_TRIG0_5_POS                          5
#define F_BWR_CAM_HRT_W2_SW_QOS_TRIG0_5_WIDTH                        1
#define F_BWR_CAM_HRT_W2_SW_QOS_TRIG0_4_POS                          4
#define F_BWR_CAM_HRT_W2_SW_QOS_TRIG0_4_WIDTH                        1
#define F_BWR_CAM_HRT_W2_SW_QOS_TRIG0_3_POS                          3
#define F_BWR_CAM_HRT_W2_SW_QOS_TRIG0_3_WIDTH                        1
#define F_BWR_CAM_HRT_W2_SW_QOS_TRIG0_2_POS                          2
#define F_BWR_CAM_HRT_W2_SW_QOS_TRIG0_2_WIDTH                        1
#define F_BWR_CAM_HRT_W2_SW_QOS_TRIG0_1_POS                          1
#define F_BWR_CAM_HRT_W2_SW_QOS_TRIG0_1_WIDTH                        1
#define F_BWR_CAM_HRT_W2_SW_QOS_TRIG0_0_POS                          0
#define F_BWR_CAM_HRT_W2_SW_QOS_TRIG0_0_WIDTH                        1

#define REG_BWR_CAM_HRT_W2_SW_QOS_EN0               0x878
#define F_BWR_CAM_HRT_W2_SW_QOS_EN0_10_POS                           10
#define F_BWR_CAM_HRT_W2_SW_QOS_EN0_10_WIDTH                         1
#define F_BWR_CAM_HRT_W2_SW_QOS_EN0_9_POS                            9
#define F_BWR_CAM_HRT_W2_SW_QOS_EN0_9_WIDTH                          1
#define F_BWR_CAM_HRT_W2_SW_QOS_EN0_8_POS                            8
#define F_BWR_CAM_HRT_W2_SW_QOS_EN0_8_WIDTH                          1
#define F_BWR_CAM_HRT_W2_SW_QOS_EN0_7_POS                            7
#define F_BWR_CAM_HRT_W2_SW_QOS_EN0_7_WIDTH                          1
#define F_BWR_CAM_HRT_W2_SW_QOS_EN0_6_POS                            6
#define F_BWR_CAM_HRT_W2_SW_QOS_EN0_6_WIDTH                          1
#define F_BWR_CAM_HRT_W2_SW_QOS_EN0_5_POS                            5
#define F_BWR_CAM_HRT_W2_SW_QOS_EN0_5_WIDTH                          1
#define F_BWR_CAM_HRT_W2_SW_QOS_EN0_4_POS                            4
#define F_BWR_CAM_HRT_W2_SW_QOS_EN0_4_WIDTH                          1
#define F_BWR_CAM_HRT_W2_SW_QOS_EN0_3_POS                            3
#define F_BWR_CAM_HRT_W2_SW_QOS_EN0_3_WIDTH                          1
#define F_BWR_CAM_HRT_W2_SW_QOS_EN0_2_POS                            2
#define F_BWR_CAM_HRT_W2_SW_QOS_EN0_2_WIDTH                          1
#define F_BWR_CAM_HRT_W2_SW_QOS_EN0_1_POS                            1
#define F_BWR_CAM_HRT_W2_SW_QOS_EN0_1_WIDTH                          1
#define F_BWR_CAM_HRT_W2_SW_QOS_EN0_0_POS                            0
#define F_BWR_CAM_HRT_W2_SW_QOS_EN0_0_WIDTH                          1

#define REG_BWR_CAM_HRT_W2_ENG_BW0_0                0x87C
#define F_BWR_CAM_HRT_W2_ENG_BW0_0_POS                               0
#define F_BWR_CAM_HRT_W2_ENG_BW0_0_WIDTH                             18

#define REG_BWR_CAM_HRT_W2_ENG_BW0_1                0x880
#define F_BWR_CAM_HRT_W2_ENG_BW0_1_POS                               0
#define F_BWR_CAM_HRT_W2_ENG_BW0_1_WIDTH                             18

#define REG_BWR_CAM_HRT_W2_ENG_BW0_2                0x884
#define F_BWR_CAM_HRT_W2_ENG_BW0_2_POS                               0
#define F_BWR_CAM_HRT_W2_ENG_BW0_2_WIDTH                             18

#define REG_BWR_CAM_HRT_W2_ENG_BW0_3                0x888
#define F_BWR_CAM_HRT_W2_ENG_BW0_3_POS                               0
#define F_BWR_CAM_HRT_W2_ENG_BW0_3_WIDTH                             18

#define REG_BWR_CAM_HRT_W2_ENG_BW0_4                0x88C
#define F_BWR_CAM_HRT_W2_ENG_BW0_4_POS                               0
#define F_BWR_CAM_HRT_W2_ENG_BW0_4_WIDTH                             18

#define REG_BWR_CAM_HRT_W2_ENG_BW0_5                0x890
#define F_BWR_CAM_HRT_W2_ENG_BW0_5_POS                               0
#define F_BWR_CAM_HRT_W2_ENG_BW0_5_WIDTH                             18

#define REG_BWR_CAM_HRT_W2_ENG_BW0_6                0x894
#define F_BWR_CAM_HRT_W2_ENG_BW0_6_POS                               0
#define F_BWR_CAM_HRT_W2_ENG_BW0_6_WIDTH                             18

#define REG_BWR_CAM_HRT_W2_ENG_BW0_7                0x898
#define F_BWR_CAM_HRT_W2_ENG_BW0_7_POS                               0
#define F_BWR_CAM_HRT_W2_ENG_BW0_7_WIDTH                             18

#define REG_BWR_CAM_HRT_W2_ENG_BW0_8                0x89C
#define F_BWR_CAM_HRT_W2_ENG_BW0_8_POS                               0
#define F_BWR_CAM_HRT_W2_ENG_BW0_8_WIDTH                             18

#define REG_BWR_CAM_HRT_W2_ENG_BW0_9                0x8A0
#define F_BWR_CAM_HRT_W2_ENG_BW0_9_POS                               0
#define F_BWR_CAM_HRT_W2_ENG_BW0_9_WIDTH                             18

#define REG_BWR_CAM_HRT_W2_ENG_BW0_10               0x8A4
#define F_BWR_CAM_HRT_W2_ENG_BW0_10_POS                              0
#define F_BWR_CAM_HRT_W2_ENG_BW0_10_WIDTH                            18

#define REG_BWR_CAM_HRT_W2_ENG_BW_RAT0_0            0x8A8
#define F_BWR_CAM_HRT_W2_ENG_BW_RAT0_0_POS                           0
#define F_BWR_CAM_HRT_W2_ENG_BW_RAT0_0_WIDTH                         12

#define REG_BWR_CAM_HRT_W2_ENG_BW_RAT0_1            0x8AC
#define F_BWR_CAM_HRT_W2_ENG_BW_RAT0_1_POS                           0
#define F_BWR_CAM_HRT_W2_ENG_BW_RAT0_1_WIDTH                         12

#define REG_BWR_CAM_HRT_W2_ENG_BW_RAT0_2            0x8B0
#define F_BWR_CAM_HRT_W2_ENG_BW_RAT0_2_POS                           0
#define F_BWR_CAM_HRT_W2_ENG_BW_RAT0_2_WIDTH                         12

#define REG_BWR_CAM_HRT_W2_ENG_BW_RAT0_3            0x8B4
#define F_BWR_CAM_HRT_W2_ENG_BW_RAT0_3_POS                           0
#define F_BWR_CAM_HRT_W2_ENG_BW_RAT0_3_WIDTH                         12

#define REG_BWR_CAM_HRT_W2_ENG_BW_RAT0_4            0x8B8
#define F_BWR_CAM_HRT_W2_ENG_BW_RAT0_4_POS                           0
#define F_BWR_CAM_HRT_W2_ENG_BW_RAT0_4_WIDTH                         12

#define REG_BWR_CAM_HRT_W2_ENG_BW_RAT0_5            0x8BC
#define F_BWR_CAM_HRT_W2_ENG_BW_RAT0_5_POS                           0
#define F_BWR_CAM_HRT_W2_ENG_BW_RAT0_5_WIDTH                         12

#define REG_BWR_CAM_HRT_W2_ENG_BW_RAT0_6            0x8C0
#define F_BWR_CAM_HRT_W2_ENG_BW_RAT0_6_POS                           0
#define F_BWR_CAM_HRT_W2_ENG_BW_RAT0_6_WIDTH                         12

#define REG_BWR_CAM_HRT_W2_ENG_BW_RAT0_7            0x8C4
#define F_BWR_CAM_HRT_W2_ENG_BW_RAT0_7_POS                           0
#define F_BWR_CAM_HRT_W2_ENG_BW_RAT0_7_WIDTH                         12

#define REG_BWR_CAM_HRT_W2_ENG_BW_RAT0_8            0x8C8
#define F_BWR_CAM_HRT_W2_ENG_BW_RAT0_8_POS                           0
#define F_BWR_CAM_HRT_W2_ENG_BW_RAT0_8_WIDTH                         12

#define REG_BWR_CAM_HRT_W2_ENG_BW_RAT0_9            0x8CC
#define F_BWR_CAM_HRT_W2_ENG_BW_RAT0_9_POS                           0
#define F_BWR_CAM_HRT_W2_ENG_BW_RAT0_9_WIDTH                         12

#define REG_BWR_CAM_HRT_W2_ENG_BW_RAT0_10           0x8D0
#define F_BWR_CAM_HRT_W2_ENG_BW_RAT0_10_POS                          0
#define F_BWR_CAM_HRT_W2_ENG_BW_RAT0_10_WIDTH                        12

#define REG_BWR_CAM_HRT_W3_BW_QOS_SEL0              0x8F0
#define F_BWR_CAM_HRT_W3_BW_QOS_SEL0_10_POS                          10
#define F_BWR_CAM_HRT_W3_BW_QOS_SEL0_10_WIDTH                        1
#define F_BWR_CAM_HRT_W3_BW_QOS_SEL0_9_POS                           9
#define F_BWR_CAM_HRT_W3_BW_QOS_SEL0_9_WIDTH                         1
#define F_BWR_CAM_HRT_W3_BW_QOS_SEL0_8_POS                           8
#define F_BWR_CAM_HRT_W3_BW_QOS_SEL0_8_WIDTH                         1
#define F_BWR_CAM_HRT_W3_BW_QOS_SEL0_7_POS                           7
#define F_BWR_CAM_HRT_W3_BW_QOS_SEL0_7_WIDTH                         1
#define F_BWR_CAM_HRT_W3_BW_QOS_SEL0_6_POS                           6
#define F_BWR_CAM_HRT_W3_BW_QOS_SEL0_6_WIDTH                         1
#define F_BWR_CAM_HRT_W3_BW_QOS_SEL0_5_POS                           5
#define F_BWR_CAM_HRT_W3_BW_QOS_SEL0_5_WIDTH                         1
#define F_BWR_CAM_HRT_W3_BW_QOS_SEL0_4_POS                           4
#define F_BWR_CAM_HRT_W3_BW_QOS_SEL0_4_WIDTH                         1
#define F_BWR_CAM_HRT_W3_BW_QOS_SEL0_3_POS                           3
#define F_BWR_CAM_HRT_W3_BW_QOS_SEL0_3_WIDTH                         1
#define F_BWR_CAM_HRT_W3_BW_QOS_SEL0_2_POS                           2
#define F_BWR_CAM_HRT_W3_BW_QOS_SEL0_2_WIDTH                         1
#define F_BWR_CAM_HRT_W3_BW_QOS_SEL0_1_POS                           1
#define F_BWR_CAM_HRT_W3_BW_QOS_SEL0_1_WIDTH                         1
#define F_BWR_CAM_HRT_W3_BW_QOS_SEL0_0_POS                           0
#define F_BWR_CAM_HRT_W3_BW_QOS_SEL0_0_WIDTH                         1

#define REG_BWR_CAM_HRT_W3_SW_QOS_TRIG0             0x8F4
#define F_BWR_CAM_HRT_W3_SW_QOS_TRIG0_10_POS                         10
#define F_BWR_CAM_HRT_W3_SW_QOS_TRIG0_10_WIDTH                       1
#define F_BWR_CAM_HRT_W3_SW_QOS_TRIG0_9_POS                          9
#define F_BWR_CAM_HRT_W3_SW_QOS_TRIG0_9_WIDTH                        1
#define F_BWR_CAM_HRT_W3_SW_QOS_TRIG0_8_POS                          8
#define F_BWR_CAM_HRT_W3_SW_QOS_TRIG0_8_WIDTH                        1
#define F_BWR_CAM_HRT_W3_SW_QOS_TRIG0_7_POS                          7
#define F_BWR_CAM_HRT_W3_SW_QOS_TRIG0_7_WIDTH                        1
#define F_BWR_CAM_HRT_W3_SW_QOS_TRIG0_6_POS                          6
#define F_BWR_CAM_HRT_W3_SW_QOS_TRIG0_6_WIDTH                        1
#define F_BWR_CAM_HRT_W3_SW_QOS_TRIG0_5_POS                          5
#define F_BWR_CAM_HRT_W3_SW_QOS_TRIG0_5_WIDTH                        1
#define F_BWR_CAM_HRT_W3_SW_QOS_TRIG0_4_POS                          4
#define F_BWR_CAM_HRT_W3_SW_QOS_TRIG0_4_WIDTH                        1
#define F_BWR_CAM_HRT_W3_SW_QOS_TRIG0_3_POS                          3
#define F_BWR_CAM_HRT_W3_SW_QOS_TRIG0_3_WIDTH                        1
#define F_BWR_CAM_HRT_W3_SW_QOS_TRIG0_2_POS                          2
#define F_BWR_CAM_HRT_W3_SW_QOS_TRIG0_2_WIDTH                        1
#define F_BWR_CAM_HRT_W3_SW_QOS_TRIG0_1_POS                          1
#define F_BWR_CAM_HRT_W3_SW_QOS_TRIG0_1_WIDTH                        1
#define F_BWR_CAM_HRT_W3_SW_QOS_TRIG0_0_POS                          0
#define F_BWR_CAM_HRT_W3_SW_QOS_TRIG0_0_WIDTH                        1

#define REG_BWR_CAM_HRT_W3_SW_QOS_EN0               0x8F8
#define F_BWR_CAM_HRT_W3_SW_QOS_EN0_10_POS                           10
#define F_BWR_CAM_HRT_W3_SW_QOS_EN0_10_WIDTH                         1
#define F_BWR_CAM_HRT_W3_SW_QOS_EN0_9_POS                            9
#define F_BWR_CAM_HRT_W3_SW_QOS_EN0_9_WIDTH                          1
#define F_BWR_CAM_HRT_W3_SW_QOS_EN0_8_POS                            8
#define F_BWR_CAM_HRT_W3_SW_QOS_EN0_8_WIDTH                          1
#define F_BWR_CAM_HRT_W3_SW_QOS_EN0_7_POS                            7
#define F_BWR_CAM_HRT_W3_SW_QOS_EN0_7_WIDTH                          1
#define F_BWR_CAM_HRT_W3_SW_QOS_EN0_6_POS                            6
#define F_BWR_CAM_HRT_W3_SW_QOS_EN0_6_WIDTH                          1
#define F_BWR_CAM_HRT_W3_SW_QOS_EN0_5_POS                            5
#define F_BWR_CAM_HRT_W3_SW_QOS_EN0_5_WIDTH                          1
#define F_BWR_CAM_HRT_W3_SW_QOS_EN0_4_POS                            4
#define F_BWR_CAM_HRT_W3_SW_QOS_EN0_4_WIDTH                          1
#define F_BWR_CAM_HRT_W3_SW_QOS_EN0_3_POS                            3
#define F_BWR_CAM_HRT_W3_SW_QOS_EN0_3_WIDTH                          1
#define F_BWR_CAM_HRT_W3_SW_QOS_EN0_2_POS                            2
#define F_BWR_CAM_HRT_W3_SW_QOS_EN0_2_WIDTH                          1
#define F_BWR_CAM_HRT_W3_SW_QOS_EN0_1_POS                            1
#define F_BWR_CAM_HRT_W3_SW_QOS_EN0_1_WIDTH                          1
#define F_BWR_CAM_HRT_W3_SW_QOS_EN0_0_POS                            0
#define F_BWR_CAM_HRT_W3_SW_QOS_EN0_0_WIDTH                          1

#define REG_BWR_CAM_HRT_W3_ENG_BW0_0                0x8FC
#define F_BWR_CAM_HRT_W3_ENG_BW0_0_POS                               0
#define F_BWR_CAM_HRT_W3_ENG_BW0_0_WIDTH                             18

#define REG_BWR_CAM_HRT_W3_ENG_BW0_1                0x900
#define F_BWR_CAM_HRT_W3_ENG_BW0_1_POS                               0
#define F_BWR_CAM_HRT_W3_ENG_BW0_1_WIDTH                             18

#define REG_BWR_CAM_HRT_W3_ENG_BW0_2                0x904
#define F_BWR_CAM_HRT_W3_ENG_BW0_2_POS                               0
#define F_BWR_CAM_HRT_W3_ENG_BW0_2_WIDTH                             18

#define REG_BWR_CAM_HRT_W3_ENG_BW0_3                0x908
#define F_BWR_CAM_HRT_W3_ENG_BW0_3_POS                               0
#define F_BWR_CAM_HRT_W3_ENG_BW0_3_WIDTH                             18

#define REG_BWR_CAM_HRT_W3_ENG_BW0_4                0x90C
#define F_BWR_CAM_HRT_W3_ENG_BW0_4_POS                               0
#define F_BWR_CAM_HRT_W3_ENG_BW0_4_WIDTH                             18

#define REG_BWR_CAM_HRT_W3_ENG_BW0_5                0x910
#define F_BWR_CAM_HRT_W3_ENG_BW0_5_POS                               0
#define F_BWR_CAM_HRT_W3_ENG_BW0_5_WIDTH                             18

#define REG_BWR_CAM_HRT_W3_ENG_BW0_6                0x914
#define F_BWR_CAM_HRT_W3_ENG_BW0_6_POS                               0
#define F_BWR_CAM_HRT_W3_ENG_BW0_6_WIDTH                             18

#define REG_BWR_CAM_HRT_W3_ENG_BW0_7                0x918
#define F_BWR_CAM_HRT_W3_ENG_BW0_7_POS                               0
#define F_BWR_CAM_HRT_W3_ENG_BW0_7_WIDTH                             18

#define REG_BWR_CAM_HRT_W3_ENG_BW0_8                0x91C
#define F_BWR_CAM_HRT_W3_ENG_BW0_8_POS                               0
#define F_BWR_CAM_HRT_W3_ENG_BW0_8_WIDTH                             18

#define REG_BWR_CAM_HRT_W3_ENG_BW0_9                0x920
#define F_BWR_CAM_HRT_W3_ENG_BW0_9_POS                               0
#define F_BWR_CAM_HRT_W3_ENG_BW0_9_WIDTH                             18

#define REG_BWR_CAM_HRT_W3_ENG_BW0_10               0x924
#define F_BWR_CAM_HRT_W3_ENG_BW0_10_POS                              0
#define F_BWR_CAM_HRT_W3_ENG_BW0_10_WIDTH                            18

#define REG_BWR_CAM_HRT_W3_ENG_BW_RAT0_0            0x928
#define F_BWR_CAM_HRT_W3_ENG_BW_RAT0_0_POS                           0
#define F_BWR_CAM_HRT_W3_ENG_BW_RAT0_0_WIDTH                         12

#define REG_BWR_CAM_HRT_W3_ENG_BW_RAT0_1            0x92C
#define F_BWR_CAM_HRT_W3_ENG_BW_RAT0_1_POS                           0
#define F_BWR_CAM_HRT_W3_ENG_BW_RAT0_1_WIDTH                         12

#define REG_BWR_CAM_HRT_W3_ENG_BW_RAT0_2            0x930
#define F_BWR_CAM_HRT_W3_ENG_BW_RAT0_2_POS                           0
#define F_BWR_CAM_HRT_W3_ENG_BW_RAT0_2_WIDTH                         12

#define REG_BWR_CAM_HRT_W3_ENG_BW_RAT0_3            0x934
#define F_BWR_CAM_HRT_W3_ENG_BW_RAT0_3_POS                           0
#define F_BWR_CAM_HRT_W3_ENG_BW_RAT0_3_WIDTH                         12

#define REG_BWR_CAM_HRT_W3_ENG_BW_RAT0_4            0x938
#define F_BWR_CAM_HRT_W3_ENG_BW_RAT0_4_POS                           0
#define F_BWR_CAM_HRT_W3_ENG_BW_RAT0_4_WIDTH                         12

#define REG_BWR_CAM_HRT_W3_ENG_BW_RAT0_5            0x93C
#define F_BWR_CAM_HRT_W3_ENG_BW_RAT0_5_POS                           0
#define F_BWR_CAM_HRT_W3_ENG_BW_RAT0_5_WIDTH                         12

#define REG_BWR_CAM_HRT_W3_ENG_BW_RAT0_6            0x940
#define F_BWR_CAM_HRT_W3_ENG_BW_RAT0_6_POS                           0
#define F_BWR_CAM_HRT_W3_ENG_BW_RAT0_6_WIDTH                         12

#define REG_BWR_CAM_HRT_W3_ENG_BW_RAT0_7            0x944
#define F_BWR_CAM_HRT_W3_ENG_BW_RAT0_7_POS                           0
#define F_BWR_CAM_HRT_W3_ENG_BW_RAT0_7_WIDTH                         12

#define REG_BWR_CAM_HRT_W3_ENG_BW_RAT0_8            0x948
#define F_BWR_CAM_HRT_W3_ENG_BW_RAT0_8_POS                           0
#define F_BWR_CAM_HRT_W3_ENG_BW_RAT0_8_WIDTH                         12

#define REG_BWR_CAM_HRT_W3_ENG_BW_RAT0_9            0x94C
#define F_BWR_CAM_HRT_W3_ENG_BW_RAT0_9_POS                           0
#define F_BWR_CAM_HRT_W3_ENG_BW_RAT0_9_WIDTH                         12

#define REG_BWR_CAM_HRT_W3_ENG_BW_RAT0_10           0x950
#define F_BWR_CAM_HRT_W3_ENG_BW_RAT0_10_POS                          0
#define F_BWR_CAM_HRT_W3_ENG_BW_RAT0_10_WIDTH                        12


#endif	/* _MTK_CAM_BWR_REGS_H */
