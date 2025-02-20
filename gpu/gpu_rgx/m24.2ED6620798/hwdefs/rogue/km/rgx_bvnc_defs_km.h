/*************************************************************************/ /*!
@Title          Hardware definition file rgx_bvnc_defs_km.h
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/ /**************************************************************************/

/******************************************************************************
 *                 Auto generated file by rgxbvnc_tablegen.py                 *
 *                  This file should not be edited manually                   *
 *****************************************************************************/

#ifndef RGX_BVNC_DEFS_KM_H
#define RGX_BVNC_DEFS_KM_H

#include "img_types.h"
#include "img_defs.h"

#if defined(RGX_BVNC_DEFS_UM_H)
#error "This file should not be included in conjunction with rgx_bvnc_defs_um.h"
#endif

#define BVNC_FIELD_WIDTH (16U)

#define PVR_ARCH_NAME "rogue"


/******************************************************************************
 * Mask and bit-position macros for features without values
 *****************************************************************************/

#define RGX_FEATURE_AXI_ACELITE_POS                                     (0U)
#define RGX_FEATURE_AXI_ACELITE_BIT_MASK                                (IMG_UINT64_C(0x0000000000000001))

#define RGX_FEATURE_CLUSTER_GROUPING_POS                                (1U)
#define RGX_FEATURE_CLUSTER_GROUPING_BIT_MASK                           (IMG_UINT64_C(0x0000000000000002))

#define RGX_FEATURE_COMPUTE_POS                                         (2U)
#define RGX_FEATURE_COMPUTE_BIT_MASK                                    (IMG_UINT64_C(0x0000000000000004))

#define RGX_FEATURE_COMPUTE_MORTON_CAPABLE_POS                          (3U)
#define RGX_FEATURE_COMPUTE_MORTON_CAPABLE_BIT_MASK                     (IMG_UINT64_C(0x0000000000000008))

#define RGX_FEATURE_COMPUTE_OVERLAP_POS                                 (4U)
#define RGX_FEATURE_COMPUTE_OVERLAP_BIT_MASK                            (IMG_UINT64_C(0x0000000000000010))

#define RGX_FEATURE_COREID_PER_OS_POS                                   (5U)
#define RGX_FEATURE_COREID_PER_OS_BIT_MASK                              (IMG_UINT64_C(0x0000000000000020))

#define RGX_FEATURE_DYNAMIC_DUST_POWER_POS                              (6U)
#define RGX_FEATURE_DYNAMIC_DUST_POWER_BIT_MASK                         (IMG_UINT64_C(0x0000000000000040))

#define RGX_FEATURE_FASTRENDER_DM_POS                                   (7U)
#define RGX_FEATURE_FASTRENDER_DM_BIT_MASK                              (IMG_UINT64_C(0x0000000000000080))

#define RGX_FEATURE_GPU_MULTICORE_SUPPORT_POS                           (8U)
#define RGX_FEATURE_GPU_MULTICORE_SUPPORT_BIT_MASK                      (IMG_UINT64_C(0x0000000000000100))

#define RGX_FEATURE_GPU_VIRTUALISATION_POS                              (9U)
#define RGX_FEATURE_GPU_VIRTUALISATION_BIT_MASK                         (IMG_UINT64_C(0x0000000000000200))

#define RGX_FEATURE_GS_RTA_SUPPORT_POS                                  (10U)
#define RGX_FEATURE_GS_RTA_SUPPORT_BIT_MASK                             (IMG_UINT64_C(0x0000000000000400))

#define RGX_FEATURE_IRQ_PER_OS_POS                                      (11U)
#define RGX_FEATURE_IRQ_PER_OS_BIT_MASK                                 (IMG_UINT64_C(0x0000000000000800))

#define RGX_FEATURE_MIPS_POS                                            (12U)
#define RGX_FEATURE_MIPS_BIT_MASK                                       (IMG_UINT64_C(0x0000000000001000))

#define RGX_FEATURE_PBE2_IN_XE_POS                                      (13U)
#define RGX_FEATURE_PBE2_IN_XE_BIT_MASK                                 (IMG_UINT64_C(0x0000000000002000))

#define RGX_FEATURE_PBE_CHECKSUM_2D_POS                                 (14U)
#define RGX_FEATURE_PBE_CHECKSUM_2D_BIT_MASK                            (IMG_UINT64_C(0x0000000000004000))

#define RGX_FEATURE_PBVNC_COREID_REG_POS                                (15U)
#define RGX_FEATURE_PBVNC_COREID_REG_BIT_MASK                           (IMG_UINT64_C(0x0000000000008000))

#define RGX_FEATURE_PERFBUS_POS                                         (16U)
#define RGX_FEATURE_PERFBUS_BIT_MASK                                    (IMG_UINT64_C(0x0000000000010000))

#define RGX_FEATURE_PERF_COUNTER_BATCH_POS                              (17U)
#define RGX_FEATURE_PERF_COUNTER_BATCH_BIT_MASK                         (IMG_UINT64_C(0x0000000000020000))

#define RGX_FEATURE_PM_MMU_VFP_POS                                      (18U)
#define RGX_FEATURE_PM_MMU_VFP_BIT_MASK                                 (IMG_UINT64_C(0x0000000000040000))

#define RGX_FEATURE_RISCV_FW_PROCESSOR_POS                              (19U)
#define RGX_FEATURE_RISCV_FW_PROCESSOR_BIT_MASK                         (IMG_UINT64_C(0x0000000000080000))

#define RGX_FEATURE_ROGUEXE_POS                                         (20U)
#define RGX_FEATURE_ROGUEXE_BIT_MASK                                    (IMG_UINT64_C(0x0000000000100000))

#define RGX_FEATURE_SAFETY_IRQ_POS                                      (21U)
#define RGX_FEATURE_SAFETY_IRQ_BIT_MASK                                 (IMG_UINT64_C(0x0000000000200000))

#define RGX_FEATURE_SIMPLE_INTERNAL_PARAMETER_FORMAT_POS                (22U)
#define RGX_FEATURE_SIMPLE_INTERNAL_PARAMETER_FORMAT_BIT_MASK           (IMG_UINT64_C(0x0000000000400000))

#define RGX_FEATURE_SIMPLE_INTERNAL_PARAMETER_FORMAT_V1_POS             (23U)
#define RGX_FEATURE_SIMPLE_INTERNAL_PARAMETER_FORMAT_V1_BIT_MASK        (IMG_UINT64_C(0x0000000000800000))

#define RGX_FEATURE_SIMPLE_INTERNAL_PARAMETER_FORMAT_V2_POS             (24U)
#define RGX_FEATURE_SIMPLE_INTERNAL_PARAMETER_FORMAT_V2_BIT_MASK        (IMG_UINT64_C(0x0000000001000000))

#define RGX_FEATURE_SINGLE_BIF_POS                                      (25U)
#define RGX_FEATURE_SINGLE_BIF_BIT_MASK                                 (IMG_UINT64_C(0x0000000002000000))

#define RGX_FEATURE_SLC_SIZE_CONFIGURABLE_POS                           (26U)
#define RGX_FEATURE_SLC_SIZE_CONFIGURABLE_BIT_MASK                      (IMG_UINT64_C(0x0000000004000000))

#define RGX_FEATURE_SOC_TIMER_POS                                       (27U)
#define RGX_FEATURE_SOC_TIMER_BIT_MASK                                  (IMG_UINT64_C(0x0000000008000000))

#define RGX_FEATURE_SYS_BUS_SECURE_RESET_POS                            (28U)
#define RGX_FEATURE_SYS_BUS_SECURE_RESET_BIT_MASK                       (IMG_UINT64_C(0x0000000010000000))

#define RGX_FEATURE_TDM_PDS_CHECKSUM_POS                                (29U)
#define RGX_FEATURE_TDM_PDS_CHECKSUM_BIT_MASK                           (IMG_UINT64_C(0x0000000020000000))

#define RGX_FEATURE_TFBC_DELTA_CORRELATION_POS                          (30U)
#define RGX_FEATURE_TFBC_DELTA_CORRELATION_BIT_MASK                     (IMG_UINT64_C(0x0000000040000000))

#define RGX_FEATURE_TFBC_LOSSY_37_PERCENT_POS                           (31U)
#define RGX_FEATURE_TFBC_LOSSY_37_PERCENT_BIT_MASK                      (IMG_UINT64_C(0x0000000080000000))

#define RGX_FEATURE_TFBC_NATIVE_YUV10_POS                               (32U)
#define RGX_FEATURE_TFBC_NATIVE_YUV10_BIT_MASK                          (IMG_UINT64_C(0x0000000100000000))

#define RGX_FEATURE_TILE_REGION_PROTECTION_POS                          (33U)
#define RGX_FEATURE_TILE_REGION_PROTECTION_BIT_MASK                     (IMG_UINT64_C(0x0000000200000000))

#define RGX_FEATURE_TLA_POS                                             (34U)
#define RGX_FEATURE_TLA_BIT_MASK                                        (IMG_UINT64_C(0x0000000400000000))

#define RGX_FEATURE_TPU_CEM_DATAMASTER_GLOBAL_REGISTERS_POS             (35U)
#define RGX_FEATURE_TPU_CEM_DATAMASTER_GLOBAL_REGISTERS_BIT_MASK        (IMG_UINT64_C(0x0000000800000000))

#define RGX_FEATURE_TPU_DM_GLOBAL_REGISTERS_POS                         (36U)
#define RGX_FEATURE_TPU_DM_GLOBAL_REGISTERS_BIT_MASK                    (IMG_UINT64_C(0x0000001000000000))

#define RGX_FEATURE_TPU_FILTERING_MODE_CONTROL_POS                      (37U)
#define RGX_FEATURE_TPU_FILTERING_MODE_CONTROL_BIT_MASK                 (IMG_UINT64_C(0x0000002000000000))

#define RGX_FEATURE_VOLCANIC_TB_POS                                     (38U)
#define RGX_FEATURE_VOLCANIC_TB_BIT_MASK                                (IMG_UINT64_C(0x0000004000000000))

#define RGX_FEATURE_WATCHDOG_TIMER_POS                                  (39U)
#define RGX_FEATURE_WATCHDOG_TIMER_BIT_MASK                             (IMG_UINT64_C(0x0000008000000000))

#define RGX_FEATURE_WORKGROUP_PROTECTION_POS                            (40U)
#define RGX_FEATURE_WORKGROUP_PROTECTION_BIT_MASK                       (IMG_UINT64_C(0x0000010000000000))

#define RGX_FEATURE_XE_MEMORY_HIERARCHY_POS                             (41U)
#define RGX_FEATURE_XE_MEMORY_HIERARCHY_BIT_MASK                        (IMG_UINT64_C(0x0000020000000000))

#define RGX_FEATURE_XT_TOP_INFRASTRUCTURE_POS                           (42U)
#define RGX_FEATURE_XT_TOP_INFRASTRUCTURE_BIT_MASK                      (IMG_UINT64_C(0x0000040000000000))


/******************************************************************************
 * Defines for each feature with values used
 * for handling the corresponding values
 *****************************************************************************/

#define RGX_FEATURE_CDM_CONTROL_STREAM_FORMAT_MAX_VALUE_IDX             (2U)
#define RGX_FEATURE_ECC_RAMS_MAX_VALUE_IDX                              (3U)
#define RGX_FEATURE_FAULT_DECODE_VERSION_MAX_VALUE_IDX                  (2U)
#define RGX_FEATURE_FBCDC_MAX_VALUE_IDX                                 (4U)
#define RGX_FEATURE_FBCDC_ALGORITHM_MAX_VALUE_IDX                       (6U)
#define RGX_FEATURE_FBCDC_ARCHITECTURE_MAX_VALUE_IDX                    (4U)
#define RGX_FEATURE_FBC_MAX_DEFAULT_DESCRIPTORS_MAX_VALUE_IDX           (2U)
#define RGX_FEATURE_FBC_MAX_LARGE_DESCRIPTORS_MAX_VALUE_IDX             (2U)
#define RGX_FEATURE_LAYOUT_MARS_MAX_VALUE_IDX                           (3U)
#define RGX_FEATURE_META_MAX_VALUE_IDX                                  (4U)
#define RGX_FEATURE_META_COREMEM_SIZE_MAX_VALUE_IDX                     (3U)
#define RGX_FEATURE_NUM_CLUSTERS_MAX_VALUE_IDX                          (5U)
#define RGX_FEATURE_NUM_ISP_IPP_PIPES_MAX_VALUE_IDX                     (9U)
#define RGX_FEATURE_NUM_MEMBUS_MAX_VALUE_IDX                            (2U)
#define RGX_FEATURE_NUM_OSIDS_MAX_VALUE_IDX                             (3U)
#define RGX_FEATURE_NUM_RASTER_PIPES_MAX_VALUE_IDX                      (3U)
#define RGX_FEATURE_PHYS_BUS_WIDTH_MAX_VALUE_IDX                        (4U)
#define RGX_FEATURE_SIMPLE_PARAMETER_FORMAT_VERSION_MAX_VALUE_IDX       (3U)
#define RGX_FEATURE_SLC_BANKS_MAX_VALUE_IDX                             (4U)
#define RGX_FEATURE_SLC_CACHE_LINE_SIZE_BITS_MAX_VALUE_IDX              (2U)
#define RGX_FEATURE_SLC_SIZE_IN_KILOBYTES_MAX_VALUE_IDX                 (6U)
#define RGX_FEATURE_TB_GPU_COUNT_MAX_VALUE_IDX                          (2U)
#define RGX_FEATURE_TFBC_VERSION_MAX_VALUE_IDX                          (4U)
#define RGX_FEATURE_TILE_SIZE_X_MAX_VALUE_IDX                           (3U)
#define RGX_FEATURE_TILE_SIZE_Y_MAX_VALUE_IDX                           (3U)
#define RGX_FEATURE_VIRTUAL_ADDRESS_SPACE_BITS_MAX_VALUE_IDX            (2U)
#define RGX_FEATURE_XE_ARCHITECTURE_MAX_VALUE_IDX                       (2U)
#define RGX_FEATURE_XPU_MAX_REGBANKS_ADDR_WIDTH_MAX_VALUE_IDX           (2U)
#define RGX_FEATURE_XPU_MAX_SLAVES_MAX_VALUE_IDX                        (2U)
#define RGX_FEATURE_XPU_REGISTER_BROADCAST_MAX_VALUE_IDX                (2U)

/******************************************************************************
 * Features with values indexes
 *****************************************************************************/

typedef enum _RGX_FEATURE_WITH_VALUE_INDEX_ {
	RGX_FEATURE_CDM_CONTROL_STREAM_FORMAT_IDX,
	RGX_FEATURE_ECC_RAMS_IDX,
	RGX_FEATURE_FAULT_DECODE_VERSION_IDX,
	RGX_FEATURE_FBCDC_IDX,
	RGX_FEATURE_FBCDC_ALGORITHM_IDX,
	RGX_FEATURE_FBCDC_ARCHITECTURE_IDX,
	RGX_FEATURE_FBC_MAX_DEFAULT_DESCRIPTORS_IDX,
	RGX_FEATURE_FBC_MAX_LARGE_DESCRIPTORS_IDX,
	RGX_FEATURE_LAYOUT_MARS_IDX,
	RGX_FEATURE_META_IDX,
	RGX_FEATURE_META_COREMEM_SIZE_IDX,
	RGX_FEATURE_NUM_CLUSTERS_IDX,
	RGX_FEATURE_NUM_ISP_IPP_PIPES_IDX,
	RGX_FEATURE_NUM_MEMBUS_IDX,
	RGX_FEATURE_NUM_OSIDS_IDX,
	RGX_FEATURE_NUM_RASTER_PIPES_IDX,
	RGX_FEATURE_PHYS_BUS_WIDTH_IDX,
	RGX_FEATURE_SIMPLE_PARAMETER_FORMAT_VERSION_IDX,
	RGX_FEATURE_SLC_BANKS_IDX,
	RGX_FEATURE_SLC_CACHE_LINE_SIZE_BITS_IDX,
	RGX_FEATURE_SLC_SIZE_IN_KILOBYTES_IDX,
	RGX_FEATURE_TB_GPU_COUNT_IDX,
	RGX_FEATURE_TFBC_VERSION_IDX,
	RGX_FEATURE_TILE_SIZE_X_IDX,
	RGX_FEATURE_TILE_SIZE_Y_IDX,
	RGX_FEATURE_VIRTUAL_ADDRESS_SPACE_BITS_IDX,
	RGX_FEATURE_XE_ARCHITECTURE_IDX,
	RGX_FEATURE_XPU_MAX_REGBANKS_ADDR_WIDTH_IDX,
	RGX_FEATURE_XPU_MAX_SLAVES_IDX,
	RGX_FEATURE_XPU_REGISTER_BROADCAST_IDX,
	RGX_FEATURE_WITH_VALUES_MAX_IDX,
} RGX_FEATURE_WITH_VALUE_INDEX;


/******************************************************************************
 * Mask and bit-position macros for ERNs and BRNs
 *****************************************************************************/

#define FIX_HW_BRN_38344_POS                                            (0U)
#define FIX_HW_BRN_38344_BIT_MASK                                       (IMG_UINT64_C(0x0000000000000001))

#define HW_ERN_42290_POS                                                (1U)
#define HW_ERN_42290_BIT_MASK                                           (IMG_UINT64_C(0x0000000000000002))

#define FIX_HW_BRN_42321_POS                                            (2U)
#define FIX_HW_BRN_42321_BIT_MASK                                       (IMG_UINT64_C(0x0000000000000004))

#define HW_ERN_42606_POS                                                (3U)
#define HW_ERN_42606_BIT_MASK                                           (IMG_UINT64_C(0x0000000000000008))

#define HW_ERN_47025_POS                                                (4U)
#define HW_ERN_47025_BIT_MASK                                           (IMG_UINT64_C(0x0000000000000010))

#define FIX_HW_BRN_50767_POS                                            (5U)
#define FIX_HW_BRN_50767_BIT_MASK                                       (IMG_UINT64_C(0x0000000000000020))

#define HW_ERN_57596_POS                                                (6U)
#define HW_ERN_57596_BIT_MASK                                           (IMG_UINT64_C(0x0000000000000040))

#define FIX_HW_BRN_60084_POS                                            (7U)
#define FIX_HW_BRN_60084_BIT_MASK                                       (IMG_UINT64_C(0x0000000000000080))

#define HW_ERN_61389_POS                                                (8U)
#define HW_ERN_61389_BIT_MASK                                           (IMG_UINT64_C(0x0000000000000100))

#define FIX_HW_BRN_61450_POS                                            (9U)
#define FIX_HW_BRN_61450_BIT_MASK                                       (IMG_UINT64_C(0x0000000000000200))

#define FIX_HW_BRN_63142_POS                                            (10U)
#define FIX_HW_BRN_63142_BIT_MASK                                       (IMG_UINT64_C(0x0000000000000400))

#define FIX_HW_BRN_63553_POS                                            (11U)
#define FIX_HW_BRN_63553_BIT_MASK                                       (IMG_UINT64_C(0x0000000000000800))

#define FIX_HW_BRN_64502_POS                                            (12U)
#define FIX_HW_BRN_64502_BIT_MASK                                       (IMG_UINT64_C(0x0000000000001000))

#define FIX_HW_BRN_65101_POS                                            (13U)
#define FIX_HW_BRN_65101_BIT_MASK                                       (IMG_UINT64_C(0x0000000000002000))

#define FIX_HW_BRN_65273_POS                                            (14U)
#define FIX_HW_BRN_65273_BIT_MASK                                       (IMG_UINT64_C(0x0000000000004000))

#define HW_ERN_66622_POS                                                (15U)
#define HW_ERN_66622_BIT_MASK                                           (IMG_UINT64_C(0x0000000000008000))

#define FIX_HW_BRN_66927_POS                                            (16U)
#define FIX_HW_BRN_66927_BIT_MASK                                       (IMG_UINT64_C(0x0000000000010000))

#define FIX_HW_BRN_68186_POS                                            (17U)
#define FIX_HW_BRN_68186_BIT_MASK                                       (IMG_UINT64_C(0x0000000000020000))

#define FIX_HW_BRN_71242_POS                                            (18U)
#define FIX_HW_BRN_71242_BIT_MASK                                       (IMG_UINT64_C(0x0000000000040000))

#define FIX_HW_BRN_71317_POS                                            (19U)
#define FIX_HW_BRN_71317_BIT_MASK                                       (IMG_UINT64_C(0x0000000000080000))

/* Macro used for padding the unavailable values for features with values */
#define RGX_FEATURE_VALUE_INVALID	(0xFFFFFFFEU)

/* Macro used for marking a feature with value as disabled for a specific bvnc */
#define RGX_FEATURE_VALUE_DISABLED	(0xFFFFFFFFU)

#endif /* RGX_BVNC_DEFS_KM_H */
