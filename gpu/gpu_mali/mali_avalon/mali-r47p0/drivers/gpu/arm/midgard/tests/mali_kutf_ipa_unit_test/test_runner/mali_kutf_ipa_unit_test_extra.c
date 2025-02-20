// SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note
/*
 *
 * (C) COPYRIGHT 2017-2023 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 *
 */

#include <tpi/mali_tpi.h>
#include <utf/include/mali_utf_suite.h>
#include <utf/include/mali_utf_mem.h>
#include <utf/include/mali_utf_resultset.h>
#include <utf/include/mali_utf_helpers.h>
#include <utf/include/mali_utf_main.h>
#include "mali_kutf_ipa_unit_test_extra.h"
#include "mali_kutf_test_helpers.h"

#include "../mali_kutf_ipa_unit_test.h"
#include "../../../../../../include/uapi/gpu/arm/midgard/backend/gpu/mali_kbase_model_dummy.h"

#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

/* One thousand-million nanoseconds per second. */
#define NANOSEC_PER_SEC ((uint64_t)1000000000)

/* One million microseconds per second. */
#define MICROSEC_PER_SEC ((uint64_t)1000000)

/* Maximum absolute error permitted for power consumption values, in mW. At
 * lower power usages (e.g. 0-100mW), being 1mW out (i.e. the smallest
 * representable change) would be a significant percentage error. Skip the
 * relative error check if the difference is within the specified maximum
 * absolute error.
 */
#define MAX_ABSOLUTE_ERROR_MW ((double)4)
/*
 * Maximum percentage error permitted for power consumption values.
 * The kernel calculates its values using integers. The tests calculate their
 * values using doubles. Manual testing has shown that the difference can be as
 * high as 1.6 percent
 */
#define MAX_RELATIVE_ERROR ((double)1.7)

/* Coefficients for the temperature scaling factor. */
#define TS0 ((double)0.020000) /* constant term of the cubic polynomial */
#define TS1 ((double)0.002000) /* per degrees celsius */
#define TS2 ((double)-0.000020) /* per degrees celsius squared */
#define TS3 ((double)0.000002) /* per degrees celsius cubed */

static const int64_t ts[4] = { (int64_t)(TS0 * 1000000.0), (int64_t)(TS1 * 1000000.0),
			       (int64_t)(TS2 * 1000000.0), (int64_t)(TS3 * 1000000.0) };

/* Dynamic energy coefficients, in units of J/((V^2)*(10^6)*cycles).
 * Higher frequencies use more energy irrespective of the actual number
 * of cycles for which performance counters are accumulated.
 */
#define G72_C1 ((double)0.3930) /* Energy per level 2 cache lookup */
#define G72_C2 ((double)0.2270) /* Energy per instruction tuple executed */
#define G72_C3 ((double)0.1819) /* Energy per thread issued to tex coord stage */
#define G72_C4 ((double)-0.1202) /* Energy per write beat for the tile buffers */
#define G72_C5 ((double)0.1331) /* Energy per cycle the GPU was active. */
#define G72_REF_VOLTAGE 0.8 /* Voltage used to derive the coefficients */

#define G71_C1 ((double)0.5263) /* Energy per level 2 cache lookup */
#define G71_C2 ((double)0.3011) /* Energy per instruction tuple executed */
#define G71_C3 ((double)0.1974) /* Energy per thread issued to tex coord stage */
#define G71_C4 ((double)-0.1564) /* Energy per write beat for the tile buffers */
#define G71_C5 ((double)0.1158) /* Energy per cycle the GPU was active. */
#define G71_REF_VOLTAGE 0.8 /* Voltage used to derive the coefficients */

#define TNOX_C1 ((double)0.1220) /* JMCounters.GPU_ACTIVE */
#define TNOX_C2 ((double)0.4889) /* SCCounters.EXEC_INSTR_COUNT */
#define TNOX_C3 ((double)0.2121) /* SCCounters.VARY_INSTR */
#define TNOX_C4 ((double)0.2880) /* SCCounters.TEX_TFCH_NUM_OPERATIONS */
#define TNOX_C5 ((double)0.3781) /* MemSysCounters.L2_ANY_LOOKUP */
#define TNOX_REF_VOLTAGE 0.8 /* Voltage used to derive the coefficients */

#define TGOX_R1_C1 ((double)0.2242) /* JMCounters.GPU_ACTIVE */
#define TGOX_R1_C2 ((double)0.3847) /* SCCounters.EXEC_INSTR_COUNT */
#define TGOX_R1_C3 ((double)0.2719) /* SCCounters.VARY_INSTR */
#define TGOX_R1_C4 ((double)0.4777) /* SCCounters.TEX_TFCH_NUM_OPERATIONS */
#define TGOX_R1_C5 ((double)0.5514) /* MemSysCounters.L2_ANY_LOOKUP */
#define TGOX_R1_REF_VOLTAGE 1.0 /* Voltage used to derive the coefficients */

#define G51_C1 ((double)0.2014) /* JMCounters.GPU_ACTIVE */
#define G51_C2 ((double)0.3927) /* SCCounters.EXEC_INSTR_COUNT */
#define G51_C3 ((double)0.2740) /* SCCounters.VARY_INSTR */
#define G51_C4 ((double)0.5280) /* SCCounters.TEX_TFCH_NUM_OPERATIONS */
#define G51_C5 ((double)0.5064) /* MemSysCounters.L2_ANY_LOOKUP */
#define G51_REF_VOLTAGE 1.0 /* Voltage used to derive the coefficients */

#define G77_C1 ((double)0.7108) /* MemSysCounters.L2_ANY_LOOKUP */
#define G77_C2 ((double)2.3753) /* SCCounters.EXEC_INSTR_MSG */
#define G77_C3 ((double)0.6561) /* SCCounters.EXEC_INSTR_FMA */
#define G77_C4 ((double)0.3188) /* SCCounters.TEX_FILT_NUM_OPERATIONS */
#define G77_C5 ((double)0.1728) /* JMCounters.GPU_ACTIVE */
#define G77_REF_VOLTAGE ((double)1.0) /* Voltage used to derive the coefficients */

#define TBEX_C1 ((double)0.5998) /* MemSysCounters.L2_ANY_LOOKUP */
#define TBEX_C2 ((double)1.8302) /* SCCounters.EXEC_INSTR_MSG */
#define TBEX_C3 ((double)0.4073) /* SCCounters.EXEC_INSTR_FMA */
#define TBEX_C4 ((double)0.2245) /* SCCounters.TEX_FILT_NUM_OPERATIONS */
#define TBEX_C5 ((double)0.1538) /* JMCounters.GPU_ACTIVE */
#define TBEX_REF_VOLTAGE ((double)1.0) /* Voltage used to derive the coefficients */

#define TODX_PREFETCH_STALL ((double)0.14543508124040833) /* TilerCounters */
#define TODX_IDVS_VAR_SHAD_STALL ((double)-0.1719168157408611) /* TilerCounters */
#define TODX_IDVS_POS_SHAD_STALL ((double)0.10998014230603903) /* TilerCounters */
#define TODX_VFETCH_POS_READ_WAIT ((double)-0.11911824213531796) /* TilerCounters */
#define TODX_L2_RD_MSG_IN ((double)0.2956314822150039) /* MemSysCounters */
#define TODX_L2_EXT_WRITE_NOSNP_FULL ((double)0.32516835407377076) /* MemSysCounters */

#define TODX_EXEC_INSTR_FMA ((double)0.5054491197298137) /* SCCcounters */
#define TODX_TEX_FILT_NUM_OPERATIONS ((double)0.5748690652927142) /* SCCcounters */
#define TODX_LS_MEM_READ_SHORT ((double)0.060917381896780864) /* SCCcounters */
#define TODX_FRAG_QUADS_EZS_UPDATE ((double)0.6945554738932764) /* SCCcounters */
#define TODX_LS_MEM_WRITE_SHORT ((double)0.6982902247714962) /* SCCcounters */
#define TODX_VARY_SLOT_16 ((double)0.18106893921824396) /* SCCcounters */

#define TODX_REF_VOLTAGE ((double)0.75) /* Voltage used to derive the coefficients */

#define TGRX_PREFETCH_STALL ((double)0.14543508124040833) /* TilerCounters */
#define TGRX_IDVS_VAR_SHAD_STALL ((double)-0.1719168157408611) /* TilerCounters */
#define TGRX_IDVS_POS_SHAD_STALL ((double)0.10998014230603903) /* TilerCounters */
#define TGRX_VFETCH_POS_READ_WAIT ((double)-0.11911824213531796) /* TilerCounters */
#define TGRX_L2_RD_MSG_IN ((double)0.2956314822150039) /* MemSysCounters */
#define TGRX_L2_EXT_WRITE_NOSNP_FULL ((double)0.32516835407377076) /* MemSysCounters */

#define TGRX_EXEC_INSTR_FMA ((double)0.5054491197298137) /* SCCcounters */
#define TGRX_TEX_FILT_NUM_OPERATIONS ((double)0.5748690652927142) /* SCCcounters */
#define TGRX_LS_MEM_READ_SHORT ((double)0.060917381896780864) /* SCCcounters */
#define TGRX_FRAG_QUADS_EZS_UPDATE ((double)0.6945554738932764) /* SCCcounters */
#define TGRX_LS_MEM_WRITE_SHORT ((double)0.6982902247714962) /* SCCcounters */
#define TGRX_VARY_SLOT_16 ((double)0.18106893921824396) /* SCCcounters */

#define TGRX_REF_VOLTAGE ((double)0.75) /* Voltage used to derive the coefficients */

#define TVAX_ITER_STALL ((double)0.8933241168013539) /* TilerCounters */
#define TVAX_PMGR_PTR_RD_STALL ((double)-0.9751174346816697) /* TilerCounters */
#define TVAX_IDVS_POS_SHAD_STALL ((double)0.022555759557123904) /* TilerCounters */
#define TVAX_L2_RD_MSG_OUT ((double)0.4914142221798743) /* MemSysCounters */
#define TVAX_L2_WR_MSG_IN ((double)0.40864508220952545) /* MemSysCounters */

#define TVAX_TEX_FILT_NUM_OPERATIONS ((double)0.14253682951425667) /* SCCounters */
#define TVAX_EXEC_INSTR_FMA ((double)0.24349798338757134) /* SCCounters */
#define TVAX_EXEC_INSTR_MSG ((double)1.3444108976406195) /* SCCounters */
#define TVAX_VARY_SLOT_16 ((double)-0.11961293167490894) /* SCCounters */
#define TVAX_FRAG_PARTIAL_QUADS_RAST ((double)0.6762019255440199) /* SCCounters */
#define TVAX_FRAG_STARVING ((double)0.06242120067780044) /* SCCounters */

#define TVAX_REF_VOLTAGE ((double)0.75) /* Voltage used to derive the coefficients */

#define TTUX_L2_RD_MSG_IN ((double)0.8008369167581743) /* MemSysCounters */
#define TTUX_IDVS_POS_SHAD_STALL ((double)0.11735836497186226) /* TilerCounters */
#define TTUX_L2_WR_MSG_IN ((double)0.4155798108301736) /* MemSysCounters */
#define TTUX_L2_READ_LOOKUP ((double)-0.19812470308026103) /* MemSysCounters */
#define TTUX_VFETCH_VERTEX_WAIT ((double)-0.3919642704490445) /* TilerCounters */

#define TTUX_EXEC_INSTR_FMA ((double)0.45701292553197226) /* SCCcounters */
#define TTUX_TEX_FILT_NUM_OPERATIONS ((double)0.44191190469287045) /* SCCcounters */
#define TTUX_LS_MEM_READ_SHORT ((double)0.32252539199350366) /* SCCcounters */
#define TTUX_FULL_QUAD_WARPS ((double)0.8441248074135418) /* SCCcounters */
#define TTUX_EXEC_INSTR_CVT ((double)0.2264118742236789) /* SCCcounters */
#define TTUX_FRAG_QUADS_EZS_UPDATE ((double)0.372032605967732) /* SCCcounters */

#define TTUX_REF_VOLTAGE ((double)0.75) /* Voltage used to derive the coefficients */

#define TTIX_PRIMASSY_STALL ((double)0.4719537144185968) /* TilerCounters */
#define TTIX_IDVS_VAR_SHAD_STALL ((double)-0.46055973489388485) /* TilerCounters */
#define TTIX_L2_RD_MSG_IN_CU ((double)-6.189604163700025) /* MemSysCounters */
#define TTIX_L2_SNP_MSG_IN ((double)6.2896095552396325) /* MemSysCounters */
#define TTIX_L2_EXT_READ_NOSNP ((double)0.5123411234291195) /* MemSysCounters */

#define TTIX_EXEC_INSTR_FMA ((double)0.1926428619553101) /* SCCcounters */
#define TTIX_EXEC_INSTR_MSG ((double)1.3264650722155844) /* SCCcounters */
#define TTIX_BEATS_RD_TEX ((double)0.1635183357164967) /* SCCcounters */
#define TTIX_BEATS_RD_LSC_EXT ((double)0.12747558233918477) /* SCCcounters */
#define TTIX_FRAG_QUADS_COARSE ((double)-0.03624787352369718) /* SCCcounters */
#define TTIX_LS_MEM_WRITE_SHORT ((double)0.05154739737269829) /* SCCcounters */
#define TTIX_BEATS_RD_TEX_EXT ((double)-0.04337067598637756) /* SCCcounters */
#define TTIX_EXEC_INSTR_SFU ((double)0.03158335686831765) /* SCCcounters */

#define TTIX_REF_VOLTAGE ((double)0.55) /* Voltage used to derive the coefficients */

#define TKRX_PRIMASSY_POS_SHADER_WAIT ((double)0.09388324329984976) /* TilerCounters */
#define TKRX_IDVS_POS_SHAD_STALL ((double)-0.06919768413303622) /* TilerCounters */
#define TKRX_L2_RD_MSG_OUT ((double)0.1765028289128799) /* MemSysCounters */
#define TKRX_L2_EXT_WRITE_NOSNP_FULL ((double)0.5103519356038035) /* MemSysCounters */
#define TKRX_L2_EXT_WRITE ((double)-0.402377970552962) /* MemSysCounters */
#define TKRX_L2_RD_MSG_IN_STALL ((double)-0.0665450268477262) /* MemSysCounters */

#define TKRX_EXEC_ISSUE_SLOT_ANY ((double)0.29967479334961933) /* SCCcounters */
#define TKRX_EXEC_STARVE_ARITH ((double)0.02681737816055024) /* SCCcounters */
#define TKRX_TEX_CFCH_NUM_L1_CT_OPERATIONS ((double)0.22679790973316588) /* SCCcounters */
#define TKRX_EXEC_INSTR_SLOT1 ((double)-1.1857762785455792) /* SCCcounters */
#define TKRX_TEX_TFCH_CLK_STALLED ((double)-0.14772983516078328) /* SCCcounters */
#define TKRX_EXEC_INSTR_FMA ((double)0.06196820144613242) /* SCCcounters */
#define TKRX_RT_RAYS_STARTED ((double)-0.14903896682770298) /* SCCcounters */

#define TKRX_REF_VOLTAGE ((double)0.55) /* Voltage used to derive the coefficients */


/* Coefficient, in watts per voltage^3, which is multiplied by
 * v^3 to calculate the static power consumption.
 */
#define STATIC_COEFF ((double)2.427750)

/* Coefficient, in watts per voltage^2 per Hz, which is multiplied by
 * v^3 to calculate the dynamic power consumption.
 */
#define DYNAMIC_COEFF ((double)0.000000004687)

/* Maximum dynamic power to allow from the counter-based power model,
 * in W/(V^2 Mhz)
 */
#define MAX_DYNAMIC_POWER ((double)0.065536)

/**
 * enum kbase_ipa_block_type - Type of block for which power estimation is done.
 *
 * @KBASE_IPA_BLOCK_TYPE_TOP_LEVEL:    Top-level block, that covers CSHW,
 *                                     MEMSYS, Tiler.
 * @KBASE_IPA_BLOCK_TYPE_SHADER_CORES: All Shader cores.
 * @KBASE_IPA_BLOCK_TYPE_NUM:          Number of blocks.
 */
enum kbase_ipa_block_type {
	KBASE_IPA_BLOCK_TYPE_TOP_LEVEL,
	KBASE_IPA_BLOCK_TYPE_SHADER_CORES,
	KBASE_IPA_BLOCK_TYPE_NUM
};

struct ipa_fixture {
	unsigned long freq; /* GPU clock frequency, in hertz. */
	unsigned long voltage; /* GPU voltage, in millivolts. */
	uint32_t num_l2_slices;
	uint32_t num_cores;
	/* Performance counter values */
	/* Number of level 2 cache lookups. */
	uint64_t l2_access[KBASE_DUMMY_MODEL_MAX_MEMSYS_BLOCKS];
	/* Number of instruction tuples executed per core. */
	uint64_t exec_instr_count[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	/* Number of threads issued to the texel coordinate stage per core. */
	uint64_t tex_issue[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	/* Number of write beats for the tile buffers per core. */
	uint64_t tile_wb[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	/* Number of operations executed in the texel fetcher hit path */
	uint64_t tex_tfch_num_operations[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	/* Number of varying instructions */
	uint64_t vary_instr[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	/* Number of message instruction per Processing Unit */
	uint64_t exec_instr_msg[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	/* Number of FMA arithmetic operations per Processing Unit */
	uint64_t exec_instr_fma[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	/* Number of CVT arithmetic operations per Processing Unit */
	uint64_t exec_instr_cvt[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	/* Number of SFU arithmetic operations per Processing Unit */
	uint64_t exec_instr_sfu[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	/* Processing Unit starvation */
	uint64_t exec_starve_arith[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	/* Number of cycles a quad is stalled waiting to enter the texel fetcher */
	uint64_t tex_tfch_clk_stalled[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	/* Number of operations executed in the filtering unit */
	uint64_t tex_filt_num_operations[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	/* Number of cycles the GPU was active. */
	uint64_t gpu_active;
	/* Number of cycles stalled on IDVS interface requesting position shading */
	uint64_t idvs_pos_shad_stall;
	/* Number of cycles the prefetcher has valid data but is stalled by the vertex fetcher */
	uint64_t prefetch_stall;
	/* Number of cycles spent reading in positions */
	uint64_t vfetch_pos_read_wait;
	/* Number of cycles waiting for a valid vertex in the vertex fetcher */
	uint64_t vfetch_vertex_wait;
	/* Number of cycles the primitive assembly output is stalled */
	uint64_t primassy_stall;
	/* Number of cycles stalled on IDVS interface requesting varying shading */
	uint64_t idvs_var_shad_stall;
	/* Number of cycles the iterator output is stalled */
	uint64_t iter_stall;
	/* Number of cycles waiting for a valid pointer in the pointer manager */
	uint64_t pmgr_ptr_rd_stall;
	/* Number of cycles late primitive assembly is waiting for position shading */
	uint64_t primassy_pos_shader_wait;
	/* Number of clean unique messages received by the L2C from internal requesters */
	uint64_t l2_rd_msg_in_cu[KBASE_DUMMY_MODEL_MAX_MEMSYS_BLOCKS];
	/* Number of read messages received by the L2C from internal masters */
	uint64_t l2_rd_msg_in[KBASE_DUMMY_MODEL_MAX_MEMSYS_BLOCKS];
	/* Number of write messages received by the L2C from internal masters */
	uint64_t l2_wr_msg_in[KBASE_DUMMY_MODEL_MAX_MEMSYS_BLOCKS];
	/* Number of snoop messages received by the L2C from internal requesters */
	uint64_t l2_snp_msg_in[KBASE_DUMMY_MODEL_MAX_MEMSYS_BLOCKS];
	/* Number of read messages sent by the L2C to internal masters */
	uint64_t l2_rd_msg_out[KBASE_DUMMY_MODEL_MAX_MEMSYS_BLOCKS];
	/* Number of L2C lookups performed by read transactions */
	uint64_t l2_read_lookup[KBASE_DUMMY_MODEL_MAX_MEMSYS_BLOCKS];
	/* Number of external ReadNoSnoop transactions */
	uint64_t l2_ext_read_nosnp[KBASE_DUMMY_MODEL_MAX_MEMSYS_BLOCKS];
	/* Number of cycles input read messages are stalled by the L2C */
	uint64_t l2_rd_msg_in_stall[KBASE_DUMMY_MODEL_MAX_MEMSYS_BLOCKS];
	/* Number of external write transactions */
	uint64_t l2_ext_write[KBASE_DUMMY_MODEL_MAX_MEMSYS_BLOCKS];
	/* Number of external WriteNoSnpFull transactions */
	uint64_t l2_ext_write_nosnp_full[KBASE_DUMMY_MODEL_MAX_MEMSYS_BLOCKS];
	/* Number of memory read accesses, partial cache line */
	uint64_t ls_mem_read_short[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	/* Number of cycles the fragment front end is starving the execution engine of new threads */
	uint64_t frag_starving[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	/* Number of partial fragment quads rasterized */
	uint64_t frag_partial_quads_rast[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	/* Number of quads doing early ZS update */
	uint64_t frag_quads_ezs_update[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	/* Number of warps created with all quads active */
	uint64_t full_quad_warps[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	/* Number of memory write accesses, partial cache line */
	uint64_t ls_mem_write_short[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	/* Number of 16-bit varying slots */
	uint64_t vary_slot_16[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	/* Number of external read beats for the load/store cache */
	uint64_t beats_rd_lsc_ext[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	/* Number of read beats for the texture cache */
	uint64_t beats_rd_tex[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	/* Number of external read beats for the texture cache */
	uint64_t beats_rd_tex_ext[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	/* Number of coarse quads before early culling */
	uint64_t frag_quads_coarse[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	/* Number of rays that tested the root node of a top level acceleration structure */
	uint64_t rt_rays_started[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	/* Number of cycles the texture L1 cache is being accessed */
	uint64_t tex_cfch_num_l1_ct_operations[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	/* Instructions issued in slot 1 */
	uint64_t exec_instr_slot1[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	/* Clock cycles where instructions are issued in any slot */
	uint64_t exec_issue_slot_any[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	/* Interval since last performance counters dump, in nanoseconds.
	 * Used for counter based models.
	 */
	uint64_t interval_ns;
	/* User-specified power scaling factor. Used for counter based models.
	 * This is interpreted as a fraction where the denominator is 1000.
	 */
	uint32_t scale;
	/* Temperature of the thermal zone, in millidegrees celsius. */
	int temp;
	/* GPU utilization over the IPA period, where 0.0 = GPU not used and
	 * 1.0 = GPU fully utilized. The counter dump may be from a shorter
	 * period that the IPA period, if the GPU was turned off, so this does
	 * not need to correspond to GPU_ACTIVE.
	 */
	double util;
};

/* Common values to be used when constructing test fixtures. */
#define FREQ_HZ ((unsigned long)50000000)
#define VOLTAGE_MV ((unsigned long)820)
#define INTERVAL_NS ((uint64_t)NANOSEC_PER_SEC / 10)
#define SCALE ((uint32_t)10)
#define TEMP ((int)55000)

/* Clock frequency for each Operating Performance Point, in Hertz. */
#define F1 ((unsigned long)400000000)
#define F2 ((unsigned long)450000000)
#define F3 ((unsigned long)525000000)
#define F4 ((unsigned long)562000000)
#define F5 ((unsigned long)600000000)

/* Voltage for each Operating Performance Point, in millivolts. */
#define V1 ((unsigned long)750)
#define V2 ((unsigned long)820)
#define V3 ((unsigned long)850)
#define V4 ((unsigned long)870)
#define V5 ((unsigned long)900)

/* Maximum GPU frequency expected by the Driver, in Hertz. */
#define MAX_FREQ_HZ ((unsigned long)2000000000)
/* Maximum sampling interval expected by the Driver. */
#define MAX_INTERVAL_NS ((uint64_t)NANOSEC_PER_SEC)

/* Real-world counter values to be scaled for use in the test. At the example
 * frequency of 400Hz, this corresponds to about 4.5ms of counter data.
 */
#define INSTR_COUNT ((uint32_t)457999)
#define TEX_ISSUE_COUNT ((uint32_t)1628062)
#define TILE_WB_COUNT ((uint32_t)812802)
#define L2_COUNT ((uint32_t)102924)

/* Real-world counter values to be scaled for use in the test.
 * Obtained by running a benchmark on a Juno platform running at the frequency
 * of 50MHz, with a voltage value of 0.82 volts, and with a single shader core.
 * The values correspond to about 100ms of counter data.
 */
#define ACTIVE_COUNT ((uint32_t)5500004)
#define EXEC_INSTR_FMA_COUNT ((uint32_t)36154)
#define TEX_FILT_NUM_OPERATIONS_COUNT ((uint32_t)3449105)
#define L2_RD_MSG_IN_COUNT ((uint32_t)182772)
#define IDVS_POS_SHAD_STALL_COUNT ((uint32_t)502290)
#define PREFETCH_STALL_COUNT ((uint32_t)2148051)
#define VFETCH_POS_READ_WAIT_COUNT ((uint32_t)276434)
#define IDVS_VAR_SHAD_STALL_COUNT ((uint32_t)341924)
#define L2_EXT_WRITE_NOSNP_FULL_COUNT ((uint32_t)36154)
#define FRAG_QUADS_EZS_UPDATE_COUNT ((uint32_t)134444)
#define LS_MEM_READ_SHORT_COUNT ((uint32_t)0)
#define LS_MEM_WRITE_SHORT_COUNT ((uint32_t)0)
#define VARY_SLOT_16_COUNT ((uint32_t)642335)

/* Real-world counter values to be scaled for use in the test.
 * Obtained by running a benchmark on a Juno platform running at the frequency
 * of 50MHz, with a voltage value of 0.82 volts, and with a single shader core.
 * The values correspond to about 100ms of counter data.
 */
/* Values collected on Mali-G310 */
#define L2_RD_MSG_OUT_COUNT ((uint32_t)68866)
#define L2_WR_MSG_IN_COUNT ((uint32_t)4736)
#define ITER_STALL_COUNT ((uint32_t)15470)
#define PMGR_PTR_RD_STALL_COUNT ((uint32_t)1076)
#define EXEC_INSTR_MSG_COUNT ((uint32_t)611977)
#define FRAG_PARTIAL_QUADS_RAST_COUNT ((uint32_t)24296)
#define FRAG_STARVING_COUNT ((uint32_t)7373)
/* Values collected on Mali-G715 */
#define L2_READ_LOOKUP_COUNT ((uint32_t)25211)
#define VFETCH_VERTEX_WAIT_COUNT ((uint32_t)197)
#define FULL_QUAD_WARPS_COUNT ((uint32_t)107638)
#define EXEC_INSTR_CVT_COUNT ((uint32_t)227380)
/* Values collected on Mali-G720 */
#define PRIMASSY_STALL_COUNT ((uint32_t)425)
#define L2_RD_MSG_IN_CU_COUNT ((uint32_t)21)
#define L2_SNP_MSG_IN_COUNT ((uint32_t)181)
#define L2_EXT_READ_NOSNP_COUNT ((uint32_t)50965)
#define BEATS_RD_TEX_COUNT ((uint32_t)214940)
#define BEATS_RD_LSC_EXT_COUNT ((uint32_t)1772)
#define FRAG_QUADS_COARSE_COUNT ((uint32_t)1206434)
#define BEATS_RD_TEX_EXT_COUNT ((uint32_t)187184)
#define EXEC_INSTR_SFU_COUNT ((uint32_t)131)
/* Values for TKRX are temporarily zeroed.
 * Real-world counter values need to be applied.
 */
#define L2_RD_MSG_IN_STALL_COUNT ((uint32_t)0)
#define L2_EXT_WRITE_COUNT ((uint32_t)0)
#define EXEC_STARVE_ARITH_COUNT ((uint32_t)0)
#define TEX_TFCH_CLK_STALLED_COUNT ((uint32_t)0)
#define RT_RAYS_STARTED_COUNT ((uint32_t)0)
#define TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT ((uint32_t)0)
#define EXEC_INSTR_SLOT1_COUNT ((uint32_t)0)
#define EXEC_ISSUE_SLOT_ANY_COUNT ((uint32_t)0)
#define PRIMASSY_POS_SHADER_WAIT_COUNT ((uint32_t)0)

/* Helper macros to be used to scale counter values to different GPU clock
 * frequencies or counter sampling intervals.
 */
#define SCALE_FREQ(count, freq_hz) ((uint64_t)((count) * ((double)(freq_hz) / FREQ_HZ)))

#define SCALE_INTERVAL(count, interval_ns) \
	((uint64_t)((count) * ((double)(interval_ns) / INTERVAL_NS)))

#define DOUBLE_SCALE(count, freq_hz, interval_ns) \
	SCALE_INTERVAL(SCALE_FREQ(count, freq_hz), interval_ns)

/* List of test fixtures. Attempt to provide a range of counter samples,
 * intervals, scales, and utilizations in order to stress the fixed-point
 * arithmetic as much as possible.
 * One non-obvious part of this is that the utilization isn't necessarily
 * related to GPU_ACTIVE / (interval * frequency). This is because the GPU may
 * turn off during an IPA sample period, causing the counter values to be
 * reset. For example, we could be at 90% utilization, but the GPU may turn off
 * (over a 100ms interval) briefly just before sampling. The power model
 * will compensate by scaling the counter data up according to the utilization,
 * after the power model calculation. The fixtures also contain the reverse -
 * small utilizations with high counter values. This is to stress the model,
 * and isn't meant to represent a real-world scenario.
 */
static struct ipa_fixture opps[IPA_SUITE_FIXTURES] = {
	{ .freq = F1,
	  .voltage = V1,
	  .num_l2_slices = 1,
	  .l2_access = { SCALE_FREQ(L2_COUNT, F1), },
	  .num_cores = 4,
	  .exec_instr_count = {
		SCALE_FREQ(INSTR_COUNT, F1), SCALE_FREQ(INSTR_COUNT, F1),
		SCALE_FREQ(INSTR_COUNT, F1), SCALE_FREQ(INSTR_COUNT, F1) },
	  .tex_issue = {
		SCALE_FREQ(TEX_ISSUE_COUNT, F1),
		SCALE_FREQ(TEX_ISSUE_COUNT, F1),
		SCALE_FREQ(TEX_ISSUE_COUNT, F1),
		SCALE_FREQ(TEX_ISSUE_COUNT, F1) },
	  .tile_wb = {
		SCALE_FREQ(TILE_WB_COUNT, F1), SCALE_FREQ(TILE_WB_COUNT, F1),
		SCALE_FREQ(TILE_WB_COUNT, F1), SCALE_FREQ(TILE_WB_COUNT, F1) },
	  .tex_tfch_num_operations = {
		/* No real-world values available currently */
		SCALE_FREQ(1234, F1), SCALE_FREQ(1234, F1),
		SCALE_FREQ(1234, F1), SCALE_FREQ(1234, F1) },
	  .vary_instr = {
		/* No real-world values available currently */
		SCALE_FREQ(2345, F1), SCALE_FREQ(2345, F1),
		SCALE_FREQ(2345, F1), SCALE_FREQ(2345, F1) },
	  .exec_instr_msg = {
		/* No real-world values available currently */
		SCALE_FREQ(3456, F1), SCALE_FREQ(3456, F1),
		SCALE_FREQ(3456, F1), SCALE_FREQ(3456, F1) },
	  .exec_instr_fma = {
		/* No real-world values available currently */
		SCALE_FREQ(4567, F1), SCALE_FREQ(4567, F1),
		SCALE_FREQ(4567, F1), SCALE_FREQ(4567, F1) },
	  .exec_instr_cvt = {
		/* No real-world values available currently */
		SCALE_FREQ(5678, F1), SCALE_FREQ(5678, F1),
		SCALE_FREQ(5678, F1), SCALE_FREQ(5678, F1) },
	  .exec_instr_sfu = {
		/* No real-world values available currently */
		SCALE_FREQ(6789, F1), SCALE_FREQ(6789, F1),
		SCALE_FREQ(6789, F1), SCALE_FREQ(6789, F1) },
	  .exec_starve_arith = {
		/* No real-world values available currently */
		SCALE_FREQ(6789, F1), SCALE_FREQ(6789, F1),
		SCALE_FREQ(6789, F1), SCALE_FREQ(6789, F1) },
	  .tex_tfch_clk_stalled = {
		/* No real-world values available currently */
		SCALE_FREQ(6789, F1), SCALE_FREQ(6789, F1),
		SCALE_FREQ(6789, F1), SCALE_FREQ(6789, F1) },
	  .tex_filt_num_operations = {
		/* No real-world values available currently */
		SCALE_FREQ(6789, F1), SCALE_FREQ(6789, F1),
		SCALE_FREQ(6789, F1), SCALE_FREQ(6789, F1) },
	  .gpu_active = SCALE_FREQ(ACTIVE_COUNT, F1),
	  .idvs_pos_shad_stall = SCALE_FREQ(IDVS_POS_SHAD_STALL_COUNT, F1),
	  .prefetch_stall = SCALE_FREQ(PREFETCH_STALL_COUNT, F1),
	  .vfetch_pos_read_wait = SCALE_FREQ(VFETCH_POS_READ_WAIT_COUNT, F1),
	  .vfetch_vertex_wait = SCALE_FREQ(VFETCH_VERTEX_WAIT_COUNT, F1),
	  .primassy_stall = SCALE_FREQ(PRIMASSY_STALL_COUNT, F1),
	  .idvs_var_shad_stall = SCALE_FREQ(IDVS_VAR_SHAD_STALL_COUNT, F1),
	  .iter_stall = SCALE_FREQ(ITER_STALL_COUNT, F1),
	  .pmgr_ptr_rd_stall = SCALE_FREQ(PMGR_PTR_RD_STALL_COUNT, F1),
	  .primassy_pos_shader_wait = SCALE_FREQ(PRIMASSY_POS_SHADER_WAIT_COUNT, F1),
	  .l2_rd_msg_in_cu = {
		/* No real-world values available currently */
		SCALE_FREQ(1234, F1), SCALE_FREQ(1234, F1),
		SCALE_FREQ(1234, F1), SCALE_FREQ(1234, F1) },
	  .l2_rd_msg_in = {
		/* No real-world values available currently */
		SCALE_FREQ(1234, F1), SCALE_FREQ(1234, F1),
		SCALE_FREQ(1234, F1), SCALE_FREQ(1234, F1) },
	  .l2_wr_msg_in = {
		/* No real-world values available currently */
		SCALE_FREQ(2345, F1), SCALE_FREQ(2345, F1),
		SCALE_FREQ(2345, F1), SCALE_FREQ(2345, F1) },
	  .l2_snp_msg_in = {
		/* No real-world values available currently */
		SCALE_FREQ(1234, F1), SCALE_FREQ(1234, F1),
		SCALE_FREQ(1234, F1), SCALE_FREQ(1234, F1) },
	  .l2_rd_msg_out = {
		/* No real-world values available currently */
		SCALE_FREQ(2345, F1), SCALE_FREQ(2345, F1),
		SCALE_FREQ(2345, F1), SCALE_FREQ(2345, F1) },
	  .l2_read_lookup = {
		/* No real-world values available currently */
		SCALE_FREQ(3456, F1), SCALE_FREQ(3456, F1),
		SCALE_FREQ(3456, F1), SCALE_FREQ(3456, F1) },
	  .l2_ext_read_nosnp = {
		/* No real-world values available currently */
		SCALE_FREQ(3456, F1), SCALE_FREQ(3456, F1),
		SCALE_FREQ(3456, F1), SCALE_FREQ(3456, F1) },
	  .l2_rd_msg_in_stall = {
		/* No real-world values available currently */
		SCALE_FREQ(2345, F1), SCALE_FREQ(2345, F1),
		SCALE_FREQ(2345, F1), SCALE_FREQ(2345, F1) },
	  .l2_ext_write = {
		/* Manually made values */
		SCALE_FREQ(2345, F1), SCALE_FREQ(2345, F1),
		SCALE_FREQ(2345, F1), SCALE_FREQ(2345, F1) },
	  .l2_ext_write_nosnp_full = {
		/* Manually made values */
		SCALE_FREQ(2345, F1), SCALE_FREQ(2345, F1),
		SCALE_FREQ(2345, F1), SCALE_FREQ(2345, F1) },
	  .ls_mem_read_short = {
		/* Manually made values */
		SCALE_FREQ(3456, F1), SCALE_FREQ(3456, F1),
		SCALE_FREQ(3456, F1), SCALE_FREQ(3456, F1) },
	  .frag_quads_ezs_update = {
		/* Manually made values */
		SCALE_FREQ(4567, F1), SCALE_FREQ(4567, F1),
		SCALE_FREQ(4567, F1), SCALE_FREQ(4567, F1) },
	  .frag_starving = {
		/* No real-world values available currently */
		SCALE_FREQ(5678, F1), SCALE_FREQ(5678, F1),
		SCALE_FREQ(5678, F1), SCALE_FREQ(5678, F1) },
	  .frag_partial_quads_rast = {
		/* No real-world values available currently */
		SCALE_FREQ(4567, F1), SCALE_FREQ(4567, F1),
		SCALE_FREQ(4567, F1), SCALE_FREQ(4567, F1) },
	  .full_quad_warps = {
		/* Manually made values */
		SCALE_FREQ(5678, F1), SCALE_FREQ(5678, F1),
		SCALE_FREQ(5678, F1), SCALE_FREQ(5678, F1) },
	  .ls_mem_write_short = {
		/* Manually made values */
		SCALE_FREQ(6789, F1), SCALE_FREQ(6789, F1),
		SCALE_FREQ(6789, F1), SCALE_FREQ(6789, F1) },
	  .vary_slot_16 = {
		/* Manually made values */
		SCALE_FREQ(1234, F1), SCALE_FREQ(1234, F1),
		SCALE_FREQ(1234, F1), SCALE_FREQ(1234, F1) },
	  .beats_rd_lsc_ext = {
		/* Manually made values */
		SCALE_FREQ(2345, F1), SCALE_FREQ(2345, F1),
		SCALE_FREQ(2345, F1), SCALE_FREQ(2345, F1) },
	  .beats_rd_tex = {
		/* Manually made values */
		SCALE_FREQ(3456, F1), SCALE_FREQ(3456, F1),
		SCALE_FREQ(3456, F1), SCALE_FREQ(3456, F1) },
	  .beats_rd_tex_ext = {
		/* Manually made values */
		SCALE_FREQ(4567, F1), SCALE_FREQ(4567, F1),
		SCALE_FREQ(4567, F1), SCALE_FREQ(4567, F1) },
	  .frag_quads_coarse = {
		/* Manually made values */
		SCALE_FREQ(5678, F1), SCALE_FREQ(5678, F1),
		SCALE_FREQ(5678, F1), SCALE_FREQ(5678, F1) },
	  .rt_rays_started = {
		/* Manually made values */
		SCALE_FREQ(6789, F1), SCALE_FREQ(6789, F1),
		SCALE_FREQ(6789, F1), SCALE_FREQ(6789, F1) },
	  .tex_cfch_num_l1_ct_operations = {
		/* Manually made values */
		SCALE_FREQ(7890, F1), SCALE_FREQ(7890, F1),
		SCALE_FREQ(7890, F1), SCALE_FREQ(7890, F1) },
	  .exec_instr_slot1 = {
		/* Manually made values */
		SCALE_FREQ(8901, F1), SCALE_FREQ(8901, F1),
		SCALE_FREQ(8901, F1), SCALE_FREQ(8901, F1) },
	  .exec_issue_slot_any = {
		/* Manually made values */
		SCALE_FREQ(9012, F1), SCALE_FREQ(9012, F1),
		SCALE_FREQ(9012, F1), SCALE_FREQ(9012, F1) },
	  .interval_ns = INTERVAL_NS,
	  .scale = SCALE,
	  .temp = TEMP,
	  .util = 0.20,
	},

	{ .freq = F2,
	  .voltage = V2,
	  .num_l2_slices = 2,
	  .l2_access = { SCALE_FREQ(L2_COUNT, F2), SCALE_FREQ(L2_COUNT, F2), },
	  .num_cores = 4,
	  .exec_instr_count = {
		SCALE_FREQ(INSTR_COUNT, F2), SCALE_FREQ(INSTR_COUNT, F2),
		SCALE_FREQ(INSTR_COUNT, F2), SCALE_FREQ(INSTR_COUNT, F2) },
	  .tex_issue = {
		SCALE_FREQ(TEX_ISSUE_COUNT, F2),
		SCALE_FREQ(TEX_ISSUE_COUNT, F2),
		SCALE_FREQ(TEX_ISSUE_COUNT, F2),
		SCALE_FREQ(TEX_ISSUE_COUNT, F2) },
	  .tile_wb = {
		SCALE_FREQ(TILE_WB_COUNT, F2), SCALE_FREQ(TILE_WB_COUNT, F2),
		SCALE_FREQ(TILE_WB_COUNT, F2), SCALE_FREQ(TILE_WB_COUNT, F2) },
	  .exec_instr_msg = {
		/* No real-world values available currently */
		SCALE_FREQ(EXEC_INSTR_MSG_COUNT, F2), SCALE_FREQ(EXEC_INSTR_MSG_COUNT, F2),
		SCALE_FREQ(EXEC_INSTR_MSG_COUNT, F2), SCALE_FREQ(EXEC_INSTR_MSG_COUNT, F2) },
	  .exec_instr_fma = {
		SCALE_FREQ(EXEC_INSTR_FMA_COUNT, F2), SCALE_FREQ(EXEC_INSTR_FMA_COUNT, F2),
		SCALE_FREQ(EXEC_INSTR_FMA_COUNT, F2), SCALE_FREQ(EXEC_INSTR_FMA_COUNT, F2) },
	  .exec_instr_cvt = {
		SCALE_FREQ(EXEC_INSTR_CVT_COUNT, F2), SCALE_FREQ(EXEC_INSTR_CVT_COUNT, F2),
		SCALE_FREQ(EXEC_INSTR_CVT_COUNT, F2), SCALE_FREQ(EXEC_INSTR_CVT_COUNT, F2) },
	  .exec_instr_sfu = {
		SCALE_FREQ(EXEC_INSTR_SFU_COUNT, F2), SCALE_FREQ(EXEC_INSTR_SFU_COUNT, F2),
		SCALE_FREQ(EXEC_INSTR_SFU_COUNT, F2), SCALE_FREQ(EXEC_INSTR_SFU_COUNT, F2) },
	  .exec_starve_arith = {
		SCALE_FREQ(EXEC_STARVE_ARITH_COUNT, F2), SCALE_FREQ(EXEC_STARVE_ARITH_COUNT, F2),
		SCALE_FREQ(EXEC_STARVE_ARITH_COUNT, F2), SCALE_FREQ(EXEC_STARVE_ARITH_COUNT, F2) },
	  .tex_tfch_clk_stalled = {
		SCALE_FREQ(TEX_TFCH_CLK_STALLED_COUNT, F2), SCALE_FREQ(TEX_TFCH_CLK_STALLED_COUNT, F2),
		SCALE_FREQ(TEX_TFCH_CLK_STALLED_COUNT, F2), SCALE_FREQ(TEX_TFCH_CLK_STALLED_COUNT, F2) },
	  .tex_filt_num_operations = {
		SCALE_FREQ(TEX_FILT_NUM_OPERATIONS_COUNT, F2), SCALE_FREQ(TEX_FILT_NUM_OPERATIONS_COUNT, F2),
		SCALE_FREQ(TEX_FILT_NUM_OPERATIONS_COUNT, F2), SCALE_FREQ(TEX_FILT_NUM_OPERATIONS_COUNT, F2) },
	  .gpu_active = SCALE_FREQ(ACTIVE_COUNT, F2),
	  .idvs_pos_shad_stall = SCALE_FREQ(IDVS_POS_SHAD_STALL_COUNT, F2),
	  .prefetch_stall = SCALE_FREQ(PREFETCH_STALL_COUNT, F2),
	  .vfetch_pos_read_wait = SCALE_FREQ(VFETCH_POS_READ_WAIT_COUNT, F2),
	  .vfetch_vertex_wait = SCALE_FREQ(VFETCH_VERTEX_WAIT_COUNT, F2),
	  .primassy_stall = SCALE_FREQ(PRIMASSY_STALL_COUNT, F2),
	  .idvs_var_shad_stall = SCALE_FREQ(IDVS_VAR_SHAD_STALL_COUNT, F2),
	  .iter_stall = SCALE_FREQ(ITER_STALL_COUNT, F2),
	  .pmgr_ptr_rd_stall = SCALE_FREQ(PMGR_PTR_RD_STALL_COUNT, F2),
	  .primassy_pos_shader_wait = SCALE_FREQ(PRIMASSY_POS_SHADER_WAIT_COUNT, F2),
	  .l2_rd_msg_in_cu = {
		SCALE_FREQ(L2_RD_MSG_IN_CU_COUNT, F2), SCALE_FREQ(L2_RD_MSG_IN_CU_COUNT, F2),
		SCALE_FREQ(L2_RD_MSG_IN_CU_COUNT, F2), SCALE_FREQ(L2_RD_MSG_IN_CU_COUNT, F2) },
	  .l2_rd_msg_in = {
		SCALE_FREQ(L2_RD_MSG_IN_COUNT, F2), SCALE_FREQ(L2_RD_MSG_IN_COUNT, F2),
		SCALE_FREQ(L2_RD_MSG_IN_COUNT, F2), SCALE_FREQ(L2_RD_MSG_IN_COUNT, F2) },
	  .l2_wr_msg_in = {
		SCALE_FREQ(L2_WR_MSG_IN_COUNT, F2), SCALE_FREQ(L2_WR_MSG_IN_COUNT, F2),
		SCALE_FREQ(L2_WR_MSG_IN_COUNT, F2), SCALE_FREQ(L2_WR_MSG_IN_COUNT, F2) },
	  .l2_snp_msg_in = {
		SCALE_FREQ(L2_SNP_MSG_IN_COUNT, F2), SCALE_FREQ(L2_SNP_MSG_IN_COUNT, F2),
		SCALE_FREQ(L2_SNP_MSG_IN_COUNT, F2), SCALE_FREQ(L2_SNP_MSG_IN_COUNT, F2) },
	  .l2_rd_msg_out = {
		SCALE_FREQ(L2_RD_MSG_OUT_COUNT, F2), SCALE_FREQ(L2_RD_MSG_OUT_COUNT, F2),
		SCALE_FREQ(L2_RD_MSG_OUT_COUNT, F2), SCALE_FREQ(L2_RD_MSG_OUT_COUNT, F2) },
	  .l2_read_lookup = {
		SCALE_FREQ(L2_READ_LOOKUP_COUNT, F2), SCALE_FREQ(L2_READ_LOOKUP_COUNT, F2),
		SCALE_FREQ(L2_READ_LOOKUP_COUNT, F2), SCALE_FREQ(L2_READ_LOOKUP_COUNT, F2) },
	  .l2_ext_read_nosnp = {
		SCALE_FREQ(L2_EXT_READ_NOSNP_COUNT, F2), SCALE_FREQ(L2_EXT_READ_NOSNP_COUNT, F2),
		SCALE_FREQ(L2_EXT_READ_NOSNP_COUNT, F2), SCALE_FREQ(L2_EXT_READ_NOSNP_COUNT, F2) },
	  .l2_rd_msg_in_stall = {
		SCALE_FREQ(L2_RD_MSG_IN_STALL_COUNT, F2), SCALE_FREQ(L2_RD_MSG_IN_STALL_COUNT, F2),
		SCALE_FREQ(L2_RD_MSG_IN_STALL_COUNT, F2), SCALE_FREQ(L2_RD_MSG_IN_STALL_COUNT, F2) },
	  .l2_ext_write = {
		SCALE_FREQ(L2_EXT_WRITE_COUNT, F2), SCALE_FREQ(L2_EXT_WRITE_COUNT, F2),
		SCALE_FREQ(L2_EXT_WRITE_COUNT, F2), SCALE_FREQ(L2_EXT_WRITE_COUNT, F2) },
	  .l2_ext_write_nosnp_full = {
		SCALE_FREQ(L2_EXT_WRITE_NOSNP_FULL_COUNT, F2), SCALE_FREQ(L2_EXT_WRITE_NOSNP_FULL_COUNT, F2),
		SCALE_FREQ(L2_EXT_WRITE_NOSNP_FULL_COUNT, F2), SCALE_FREQ(L2_EXT_WRITE_NOSNP_FULL_COUNT, F2) },
	  .ls_mem_read_short = {
		SCALE_FREQ(LS_MEM_READ_SHORT_COUNT, F2), SCALE_FREQ(LS_MEM_READ_SHORT_COUNT, F2),
		SCALE_FREQ(LS_MEM_READ_SHORT_COUNT, F2), SCALE_FREQ(LS_MEM_READ_SHORT_COUNT, F2) },
	  .frag_quads_ezs_update = {
		SCALE_FREQ(FRAG_QUADS_EZS_UPDATE_COUNT, F2), SCALE_FREQ(FRAG_QUADS_EZS_UPDATE_COUNT, F2),
		SCALE_FREQ(FRAG_QUADS_EZS_UPDATE_COUNT, F2), SCALE_FREQ(FRAG_QUADS_EZS_UPDATE_COUNT, F2) },
	  .frag_starving = {
		SCALE_FREQ(FRAG_STARVING_COUNT, F2), SCALE_FREQ(FRAG_STARVING_COUNT, F2),
		SCALE_FREQ(FRAG_STARVING_COUNT, F2), SCALE_FREQ(FRAG_STARVING_COUNT, F2) },
	  .frag_partial_quads_rast = {
		SCALE_FREQ(FRAG_PARTIAL_QUADS_RAST_COUNT, F2), SCALE_FREQ(FRAG_PARTIAL_QUADS_RAST_COUNT, F2),
		SCALE_FREQ(FRAG_PARTIAL_QUADS_RAST_COUNT, F2), SCALE_FREQ(FRAG_PARTIAL_QUADS_RAST_COUNT, F2) },
	  .full_quad_warps = {
		SCALE_FREQ(FULL_QUAD_WARPS_COUNT, F2), SCALE_FREQ(FULL_QUAD_WARPS_COUNT, F2),
		SCALE_FREQ(FULL_QUAD_WARPS_COUNT, F2), SCALE_FREQ(FULL_QUAD_WARPS_COUNT, F2) },
	  .ls_mem_write_short = {
		SCALE_FREQ(LS_MEM_WRITE_SHORT_COUNT, F2), SCALE_FREQ(LS_MEM_WRITE_SHORT_COUNT, F2),
		SCALE_FREQ(LS_MEM_WRITE_SHORT_COUNT, F2), SCALE_FREQ(LS_MEM_WRITE_SHORT_COUNT, F2) },
	  .vary_slot_16 = {
		SCALE_FREQ(VARY_SLOT_16_COUNT, F2), SCALE_FREQ(VARY_SLOT_16_COUNT, F2),
		SCALE_FREQ(VARY_SLOT_16_COUNT, F2), SCALE_FREQ(VARY_SLOT_16_COUNT, F2) },
	  .beats_rd_lsc_ext = {
		SCALE_FREQ(BEATS_RD_LSC_EXT_COUNT, F2), SCALE_FREQ(BEATS_RD_LSC_EXT_COUNT, F2),
		SCALE_FREQ(BEATS_RD_LSC_EXT_COUNT, F2), SCALE_FREQ(BEATS_RD_LSC_EXT_COUNT, F2) },
	  .beats_rd_tex = {
		SCALE_FREQ(BEATS_RD_TEX_COUNT, F2), SCALE_FREQ(BEATS_RD_TEX_COUNT, F2),
		SCALE_FREQ(BEATS_RD_TEX_COUNT, F2), SCALE_FREQ(BEATS_RD_TEX_COUNT, F2) },
	  .beats_rd_tex_ext = {
		SCALE_FREQ(BEATS_RD_TEX_EXT_COUNT, F2), SCALE_FREQ(BEATS_RD_TEX_EXT_COUNT, F2),
		SCALE_FREQ(BEATS_RD_TEX_EXT_COUNT, F2), SCALE_FREQ(BEATS_RD_TEX_EXT_COUNT, F2) },
	  .frag_quads_coarse = {
		SCALE_FREQ(FRAG_QUADS_COARSE_COUNT, F2), SCALE_FREQ(FRAG_QUADS_COARSE_COUNT, F2),
		SCALE_FREQ(FRAG_QUADS_COARSE_COUNT, F2), SCALE_FREQ(FRAG_QUADS_COARSE_COUNT, F2) },
	  .rt_rays_started = {
		SCALE_FREQ(RT_RAYS_STARTED_COUNT, F2), SCALE_FREQ(RT_RAYS_STARTED_COUNT, F2),
		SCALE_FREQ(RT_RAYS_STARTED_COUNT, F2), SCALE_FREQ(RT_RAYS_STARTED_COUNT, F2) },
	  .tex_cfch_num_l1_ct_operations = {
		SCALE_FREQ(TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, F2), SCALE_FREQ(TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, F2),
		SCALE_FREQ(TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, F2), SCALE_FREQ(TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, F2) },
	  .exec_instr_slot1 = {
		SCALE_FREQ(EXEC_INSTR_SLOT1_COUNT, F2), SCALE_FREQ(EXEC_INSTR_SLOT1_COUNT, F2),
		SCALE_FREQ(EXEC_INSTR_SLOT1_COUNT, F2), SCALE_FREQ(EXEC_INSTR_SLOT1_COUNT, F2) },
	  .exec_issue_slot_any = {
		SCALE_FREQ(EXEC_ISSUE_SLOT_ANY_COUNT, F2), SCALE_FREQ(EXEC_ISSUE_SLOT_ANY_COUNT, F2),
		SCALE_FREQ(EXEC_ISSUE_SLOT_ANY_COUNT, F2), SCALE_FREQ(EXEC_ISSUE_SLOT_ANY_COUNT, F2) },
	  .interval_ns = INTERVAL_NS,
	  .scale = SCALE,
	  .temp = TEMP,
	  .util = 0.30,
	},

	{ .freq = F3,
	  .voltage = V3,
	  .num_l2_slices = 3,
	  .l2_access = { SCALE_FREQ(L2_COUNT, F3), SCALE_FREQ(L2_COUNT, F3), SCALE_FREQ(L2_COUNT, F3), },
	  .num_cores = 8,
	  .exec_instr_count = {
		SCALE_FREQ(INSTR_COUNT, F3), SCALE_FREQ(INSTR_COUNT, F3),
		SCALE_FREQ(INSTR_COUNT, F3), SCALE_FREQ(INSTR_COUNT, F3),
		SCALE_FREQ(INSTR_COUNT, F3), SCALE_FREQ(INSTR_COUNT, F3),
		SCALE_FREQ(INSTR_COUNT, F3), SCALE_FREQ(INSTR_COUNT, F3) },
	  .tex_issue = {
		SCALE_FREQ(TEX_ISSUE_COUNT, F3),
		SCALE_FREQ(TEX_ISSUE_COUNT, F3),
		SCALE_FREQ(TEX_ISSUE_COUNT, F3),
		SCALE_FREQ(TEX_ISSUE_COUNT, F3),
		SCALE_FREQ(TEX_ISSUE_COUNT, F3),
		SCALE_FREQ(TEX_ISSUE_COUNT, F3),
		SCALE_FREQ(TEX_ISSUE_COUNT, F3),
		SCALE_FREQ(TEX_ISSUE_COUNT, F3) },
	  .tile_wb = {
		SCALE_FREQ(TILE_WB_COUNT, F3), SCALE_FREQ(TILE_WB_COUNT, F3),
		SCALE_FREQ(TILE_WB_COUNT, F3), SCALE_FREQ(TILE_WB_COUNT, F3),
		SCALE_FREQ(TILE_WB_COUNT, F3), SCALE_FREQ(TILE_WB_COUNT, F3),
		SCALE_FREQ(TILE_WB_COUNT, F3), SCALE_FREQ(TILE_WB_COUNT, F3) },
	  .exec_instr_msg = {
		/* No real-world values available currently */
		SCALE_FREQ(EXEC_INSTR_MSG_COUNT, F3), SCALE_FREQ(EXEC_INSTR_MSG_COUNT, F3),
		SCALE_FREQ(EXEC_INSTR_MSG_COUNT, F3), SCALE_FREQ(EXEC_INSTR_MSG_COUNT, F3),
		SCALE_FREQ(EXEC_INSTR_MSG_COUNT, F3), SCALE_FREQ(EXEC_INSTR_MSG_COUNT, F3),
		SCALE_FREQ(EXEC_INSTR_MSG_COUNT, F3), SCALE_FREQ(EXEC_INSTR_MSG_COUNT, F3) },
	  .exec_instr_fma = {
		SCALE_FREQ(EXEC_INSTR_FMA_COUNT, F3), SCALE_FREQ(EXEC_INSTR_FMA_COUNT, F3),
		SCALE_FREQ(EXEC_INSTR_FMA_COUNT, F3), SCALE_FREQ(EXEC_INSTR_FMA_COUNT, F3),
		SCALE_FREQ(EXEC_INSTR_FMA_COUNT, F3), SCALE_FREQ(EXEC_INSTR_FMA_COUNT, F3),
		SCALE_FREQ(EXEC_INSTR_FMA_COUNT, F3), SCALE_FREQ(EXEC_INSTR_FMA_COUNT, F3) },
	  .exec_instr_cvt = {
		SCALE_FREQ(EXEC_INSTR_CVT_COUNT, F3), SCALE_FREQ(EXEC_INSTR_CVT_COUNT, F3),
		SCALE_FREQ(EXEC_INSTR_CVT_COUNT, F3), SCALE_FREQ(EXEC_INSTR_CVT_COUNT, F3),
		SCALE_FREQ(EXEC_INSTR_CVT_COUNT, F3), SCALE_FREQ(EXEC_INSTR_CVT_COUNT, F3),
		SCALE_FREQ(EXEC_INSTR_CVT_COUNT, F3), SCALE_FREQ(EXEC_INSTR_CVT_COUNT, F3) },
	  .exec_instr_sfu = {
		SCALE_FREQ(EXEC_INSTR_SFU_COUNT, F3), SCALE_FREQ(EXEC_INSTR_SFU_COUNT, F3),
		SCALE_FREQ(EXEC_INSTR_SFU_COUNT, F3), SCALE_FREQ(EXEC_INSTR_SFU_COUNT, F3),
		SCALE_FREQ(EXEC_INSTR_SFU_COUNT, F3), SCALE_FREQ(EXEC_INSTR_SFU_COUNT, F3),
		SCALE_FREQ(EXEC_INSTR_SFU_COUNT, F3), SCALE_FREQ(EXEC_INSTR_SFU_COUNT, F3) },
	  .exec_starve_arith = {
		SCALE_FREQ(EXEC_STARVE_ARITH_COUNT, F3), SCALE_FREQ(EXEC_STARVE_ARITH_COUNT, F3),
		SCALE_FREQ(EXEC_STARVE_ARITH_COUNT, F3), SCALE_FREQ(EXEC_STARVE_ARITH_COUNT, F3),
		SCALE_FREQ(EXEC_STARVE_ARITH_COUNT, F3), SCALE_FREQ(EXEC_STARVE_ARITH_COUNT, F3),
		SCALE_FREQ(EXEC_STARVE_ARITH_COUNT, F3), SCALE_FREQ(EXEC_STARVE_ARITH_COUNT, F3) },
	  .tex_tfch_clk_stalled = {
		SCALE_FREQ(TEX_TFCH_CLK_STALLED_COUNT, F3), SCALE_FREQ(TEX_TFCH_CLK_STALLED_COUNT, F3),
		SCALE_FREQ(TEX_TFCH_CLK_STALLED_COUNT, F3), SCALE_FREQ(TEX_TFCH_CLK_STALLED_COUNT, F3),
		SCALE_FREQ(TEX_TFCH_CLK_STALLED_COUNT, F3), SCALE_FREQ(TEX_TFCH_CLK_STALLED_COUNT, F3),
		SCALE_FREQ(TEX_TFCH_CLK_STALLED_COUNT, F3), SCALE_FREQ(TEX_TFCH_CLK_STALLED_COUNT, F3) },
	  .tex_filt_num_operations = {
		SCALE_FREQ(TEX_FILT_NUM_OPERATIONS_COUNT, F3), SCALE_FREQ(TEX_FILT_NUM_OPERATIONS_COUNT, F3),
		SCALE_FREQ(TEX_FILT_NUM_OPERATIONS_COUNT, F3), SCALE_FREQ(TEX_FILT_NUM_OPERATIONS_COUNT, F3) },
	  .gpu_active = SCALE_FREQ(ACTIVE_COUNT, F3),
	  .idvs_pos_shad_stall = SCALE_FREQ(IDVS_POS_SHAD_STALL_COUNT, F3),
	  .prefetch_stall = SCALE_FREQ(PREFETCH_STALL_COUNT, F3),
	  .vfetch_pos_read_wait = SCALE_FREQ(VFETCH_POS_READ_WAIT_COUNT, F3),
	  .vfetch_vertex_wait = SCALE_FREQ(VFETCH_VERTEX_WAIT_COUNT, F3),
	  .primassy_stall = SCALE_FREQ(PRIMASSY_STALL_COUNT, F3),
	  .idvs_var_shad_stall = SCALE_FREQ(IDVS_VAR_SHAD_STALL_COUNT, F3),
	  .iter_stall = SCALE_FREQ(ITER_STALL_COUNT, F3),
	  .pmgr_ptr_rd_stall = SCALE_FREQ(PMGR_PTR_RD_STALL_COUNT, F3),
	  .primassy_pos_shader_wait = SCALE_FREQ(PRIMASSY_POS_SHADER_WAIT_COUNT, F3),
	  .l2_rd_msg_in_cu = {
		SCALE_FREQ(L2_RD_MSG_IN_CU_COUNT, F3), SCALE_FREQ(L2_RD_MSG_IN_CU_COUNT, F3),
		SCALE_FREQ(L2_RD_MSG_IN_CU_COUNT, F3), SCALE_FREQ(L2_RD_MSG_IN_CU_COUNT, F3) },
	  .l2_rd_msg_in = {
		SCALE_FREQ(L2_RD_MSG_IN_COUNT, F3), SCALE_FREQ(L2_RD_MSG_IN_COUNT, F3),
		SCALE_FREQ(L2_RD_MSG_IN_COUNT, F3), SCALE_FREQ(L2_RD_MSG_IN_COUNT, F3) },
	  .l2_wr_msg_in = {
		SCALE_FREQ(L2_WR_MSG_IN_COUNT, F3), SCALE_FREQ(L2_WR_MSG_IN_COUNT, F3),
		SCALE_FREQ(L2_WR_MSG_IN_COUNT, F3), SCALE_FREQ(L2_WR_MSG_IN_COUNT, F3) },
	  .l2_snp_msg_in = {
		SCALE_FREQ(L2_SNP_MSG_IN_COUNT, F3), SCALE_FREQ(L2_SNP_MSG_IN_COUNT, F3),
		SCALE_FREQ(L2_SNP_MSG_IN_COUNT, F3), SCALE_FREQ(L2_SNP_MSG_IN_COUNT, F3) },
	  .l2_rd_msg_out = {
		SCALE_FREQ(L2_RD_MSG_OUT_COUNT, F3), SCALE_FREQ(L2_RD_MSG_OUT_COUNT, F3),
		SCALE_FREQ(L2_RD_MSG_OUT_COUNT, F3), SCALE_FREQ(L2_RD_MSG_OUT_COUNT, F3) },
	  .l2_read_lookup = {
		SCALE_FREQ(L2_READ_LOOKUP_COUNT, F3), SCALE_FREQ(L2_READ_LOOKUP_COUNT, F3),
		SCALE_FREQ(L2_READ_LOOKUP_COUNT, F3), SCALE_FREQ(L2_READ_LOOKUP_COUNT, F3) },
	  .l2_ext_read_nosnp = {
		SCALE_FREQ(L2_EXT_READ_NOSNP_COUNT, F3), SCALE_FREQ(L2_EXT_READ_NOSNP_COUNT, F3),
		SCALE_FREQ(L2_EXT_READ_NOSNP_COUNT, F3), SCALE_FREQ(L2_EXT_READ_NOSNP_COUNT, F3) },
	  .l2_rd_msg_in_stall = {
		SCALE_FREQ(L2_RD_MSG_IN_STALL_COUNT, F3), SCALE_FREQ(L2_RD_MSG_IN_STALL_COUNT, F3),
		SCALE_FREQ(L2_RD_MSG_IN_STALL_COUNT, F3), SCALE_FREQ(L2_RD_MSG_IN_STALL_COUNT, F3) },
	  .l2_ext_write = {
		SCALE_FREQ(L2_EXT_WRITE_COUNT, F3), SCALE_FREQ(L2_EXT_WRITE_COUNT, F3),
		SCALE_FREQ(L2_EXT_WRITE_COUNT, F3), SCALE_FREQ(L2_EXT_WRITE_COUNT, F3) },
	  .l2_ext_write_nosnp_full = {
		SCALE_FREQ(L2_EXT_WRITE_NOSNP_FULL_COUNT, F3), SCALE_FREQ(L2_EXT_WRITE_NOSNP_FULL_COUNT, F3),
		SCALE_FREQ(L2_EXT_WRITE_NOSNP_FULL_COUNT, F3), SCALE_FREQ(L2_EXT_WRITE_NOSNP_FULL_COUNT, F3) },
	  .ls_mem_read_short = {
		SCALE_FREQ(LS_MEM_READ_SHORT_COUNT, F3), SCALE_FREQ(LS_MEM_READ_SHORT_COUNT, F3),
		SCALE_FREQ(LS_MEM_READ_SHORT_COUNT, F3), SCALE_FREQ(LS_MEM_READ_SHORT_COUNT, F3),
		SCALE_FREQ(LS_MEM_READ_SHORT_COUNT, F3), SCALE_FREQ(LS_MEM_READ_SHORT_COUNT, F3),
		SCALE_FREQ(LS_MEM_READ_SHORT_COUNT, F3), SCALE_FREQ(LS_MEM_READ_SHORT_COUNT, F3) },
	  .frag_quads_ezs_update = {
		SCALE_FREQ(FRAG_QUADS_EZS_UPDATE_COUNT, F3), SCALE_FREQ(FRAG_QUADS_EZS_UPDATE_COUNT, F3),
		SCALE_FREQ(FRAG_QUADS_EZS_UPDATE_COUNT, F3), SCALE_FREQ(FRAG_QUADS_EZS_UPDATE_COUNT, F3),
		SCALE_FREQ(FRAG_QUADS_EZS_UPDATE_COUNT, F3), SCALE_FREQ(FRAG_QUADS_EZS_UPDATE_COUNT, F3),
		SCALE_FREQ(FRAG_QUADS_EZS_UPDATE_COUNT, F3), SCALE_FREQ(FRAG_QUADS_EZS_UPDATE_COUNT, F3) },
	  .frag_starving = {
		SCALE_FREQ(FRAG_STARVING_COUNT, F3), SCALE_FREQ(FRAG_STARVING_COUNT, F3),
		SCALE_FREQ(FRAG_STARVING_COUNT, F3), SCALE_FREQ(FRAG_STARVING_COUNT, F3),
		SCALE_FREQ(FRAG_STARVING_COUNT, F3), SCALE_FREQ(FRAG_STARVING_COUNT, F3),
		SCALE_FREQ(FRAG_STARVING_COUNT, F3), SCALE_FREQ(FRAG_STARVING_COUNT, F3) },
	  .frag_partial_quads_rast = {
		SCALE_FREQ(FRAG_PARTIAL_QUADS_RAST_COUNT, F3), SCALE_FREQ(FRAG_PARTIAL_QUADS_RAST_COUNT, F3),
		SCALE_FREQ(FRAG_PARTIAL_QUADS_RAST_COUNT, F3), SCALE_FREQ(FRAG_PARTIAL_QUADS_RAST_COUNT, F3),
		SCALE_FREQ(FRAG_PARTIAL_QUADS_RAST_COUNT, F3), SCALE_FREQ(FRAG_PARTIAL_QUADS_RAST_COUNT, F3),
		SCALE_FREQ(FRAG_PARTIAL_QUADS_RAST_COUNT, F3), SCALE_FREQ(FRAG_PARTIAL_QUADS_RAST_COUNT, F3) },
	  .full_quad_warps = {
		SCALE_FREQ(FULL_QUAD_WARPS_COUNT, F3), SCALE_FREQ(FULL_QUAD_WARPS_COUNT, F3),
		SCALE_FREQ(FULL_QUAD_WARPS_COUNT, F3), SCALE_FREQ(FULL_QUAD_WARPS_COUNT, F3),
		SCALE_FREQ(FULL_QUAD_WARPS_COUNT, F3), SCALE_FREQ(FULL_QUAD_WARPS_COUNT, F3),
		SCALE_FREQ(FULL_QUAD_WARPS_COUNT, F3), SCALE_FREQ(FULL_QUAD_WARPS_COUNT, F3) },
	  .ls_mem_write_short = {
		SCALE_FREQ(LS_MEM_WRITE_SHORT_COUNT, F3), SCALE_FREQ(LS_MEM_WRITE_SHORT_COUNT, F3),
		SCALE_FREQ(LS_MEM_WRITE_SHORT_COUNT, F3), SCALE_FREQ(LS_MEM_WRITE_SHORT_COUNT, F3),
		SCALE_FREQ(LS_MEM_WRITE_SHORT_COUNT, F3), SCALE_FREQ(LS_MEM_WRITE_SHORT_COUNT, F3),
		SCALE_FREQ(LS_MEM_WRITE_SHORT_COUNT, F3), SCALE_FREQ(LS_MEM_WRITE_SHORT_COUNT, F3) },
	  .vary_slot_16 = {
		SCALE_FREQ(VARY_SLOT_16_COUNT, F3), SCALE_FREQ(VARY_SLOT_16_COUNT, F3),
		SCALE_FREQ(VARY_SLOT_16_COUNT, F3), SCALE_FREQ(VARY_SLOT_16_COUNT, F3),
		SCALE_FREQ(VARY_SLOT_16_COUNT, F3), SCALE_FREQ(VARY_SLOT_16_COUNT, F3),
		SCALE_FREQ(VARY_SLOT_16_COUNT, F3), SCALE_FREQ(VARY_SLOT_16_COUNT, F3) },
	  .beats_rd_lsc_ext = {
		SCALE_FREQ(BEATS_RD_LSC_EXT_COUNT, F3), SCALE_FREQ(BEATS_RD_LSC_EXT_COUNT, F3),
		SCALE_FREQ(BEATS_RD_LSC_EXT_COUNT, F3), SCALE_FREQ(BEATS_RD_LSC_EXT_COUNT, F3),
		SCALE_FREQ(BEATS_RD_LSC_EXT_COUNT, F3), SCALE_FREQ(BEATS_RD_LSC_EXT_COUNT, F3),
		SCALE_FREQ(BEATS_RD_LSC_EXT_COUNT, F3), SCALE_FREQ(BEATS_RD_LSC_EXT_COUNT, F3) },
	  .beats_rd_tex = {
		SCALE_FREQ(BEATS_RD_TEX_COUNT, F3), SCALE_FREQ(BEATS_RD_TEX_COUNT, F3),
		SCALE_FREQ(BEATS_RD_TEX_COUNT, F3), SCALE_FREQ(BEATS_RD_TEX_COUNT, F3),
		SCALE_FREQ(BEATS_RD_TEX_COUNT, F3), SCALE_FREQ(BEATS_RD_TEX_COUNT, F3),
		SCALE_FREQ(BEATS_RD_TEX_COUNT, F3), SCALE_FREQ(BEATS_RD_TEX_COUNT, F3) },
	  .beats_rd_tex_ext = {
		SCALE_FREQ(BEATS_RD_TEX_EXT_COUNT, F3), SCALE_FREQ(BEATS_RD_TEX_EXT_COUNT, F3),
		SCALE_FREQ(BEATS_RD_TEX_EXT_COUNT, F3), SCALE_FREQ(BEATS_RD_TEX_EXT_COUNT, F3),
		SCALE_FREQ(BEATS_RD_TEX_EXT_COUNT, F3), SCALE_FREQ(BEATS_RD_TEX_EXT_COUNT, F3),
		SCALE_FREQ(BEATS_RD_TEX_EXT_COUNT, F3), SCALE_FREQ(BEATS_RD_TEX_EXT_COUNT, F3) },
	  .frag_quads_coarse = {
		SCALE_FREQ(FRAG_QUADS_COARSE_COUNT, F3), SCALE_FREQ(FRAG_QUADS_COARSE_COUNT, F3),
		SCALE_FREQ(FRAG_QUADS_COARSE_COUNT, F3), SCALE_FREQ(FRAG_QUADS_COARSE_COUNT, F3),
		SCALE_FREQ(FRAG_QUADS_COARSE_COUNT, F3), SCALE_FREQ(FRAG_QUADS_COARSE_COUNT, F3),
		SCALE_FREQ(FRAG_QUADS_COARSE_COUNT, F3), SCALE_FREQ(FRAG_QUADS_COARSE_COUNT, F3) },
	  .rt_rays_started = {
		SCALE_FREQ(RT_RAYS_STARTED_COUNT, F3), SCALE_FREQ(RT_RAYS_STARTED_COUNT, F3),
		SCALE_FREQ(RT_RAYS_STARTED_COUNT, F3), SCALE_FREQ(RT_RAYS_STARTED_COUNT, F3),
		SCALE_FREQ(RT_RAYS_STARTED_COUNT, F3), SCALE_FREQ(RT_RAYS_STARTED_COUNT, F3),
		SCALE_FREQ(RT_RAYS_STARTED_COUNT, F3), SCALE_FREQ(RT_RAYS_STARTED_COUNT, F3) },
	  .tex_cfch_num_l1_ct_operations = {
		SCALE_FREQ(TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, F3), SCALE_FREQ(TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, F3),
		SCALE_FREQ(TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, F3), SCALE_FREQ(TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, F3),
		SCALE_FREQ(TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, F3), SCALE_FREQ(TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, F3),
		SCALE_FREQ(TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, F3), SCALE_FREQ(TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, F3) },
	  .exec_instr_slot1 = {
		SCALE_FREQ(EXEC_INSTR_SLOT1_COUNT, F3), SCALE_FREQ(EXEC_INSTR_SLOT1_COUNT, F3),
		SCALE_FREQ(EXEC_INSTR_SLOT1_COUNT, F3), SCALE_FREQ(EXEC_INSTR_SLOT1_COUNT, F3),
		SCALE_FREQ(EXEC_INSTR_SLOT1_COUNT, F3), SCALE_FREQ(EXEC_INSTR_SLOT1_COUNT, F3),
		SCALE_FREQ(EXEC_INSTR_SLOT1_COUNT, F3), SCALE_FREQ(EXEC_INSTR_SLOT1_COUNT, F3) },
	  .exec_issue_slot_any = {
		SCALE_FREQ(EXEC_ISSUE_SLOT_ANY_COUNT, F3), SCALE_FREQ(EXEC_ISSUE_SLOT_ANY_COUNT, F3),
		SCALE_FREQ(EXEC_ISSUE_SLOT_ANY_COUNT, F3), SCALE_FREQ(EXEC_ISSUE_SLOT_ANY_COUNT, F3),
		SCALE_FREQ(EXEC_ISSUE_SLOT_ANY_COUNT, F3), SCALE_FREQ(EXEC_ISSUE_SLOT_ANY_COUNT, F3),
		SCALE_FREQ(EXEC_ISSUE_SLOT_ANY_COUNT, F3), SCALE_FREQ(EXEC_ISSUE_SLOT_ANY_COUNT, F3) },
	  .interval_ns = INTERVAL_NS,
	  .scale = SCALE,
	  .temp = TEMP,
	  .util = 0.50,
	},

	{ .freq = F4,
	  .voltage = V4,
	  .num_l2_slices = 4,
	  .l2_access = { SCALE_FREQ(L2_COUNT, F4), },
	  .num_cores = 8,
	  .exec_instr_count = {
		SCALE_FREQ(INSTR_COUNT, F4), SCALE_FREQ(INSTR_COUNT, F4),
		SCALE_FREQ(INSTR_COUNT, F4), SCALE_FREQ(INSTR_COUNT, F4),
		SCALE_FREQ(INSTR_COUNT, F4), SCALE_FREQ(INSTR_COUNT, F4),
		SCALE_FREQ(INSTR_COUNT, F4), SCALE_FREQ(INSTR_COUNT, F4) },
	  .tex_issue = {
		SCALE_FREQ(TEX_ISSUE_COUNT, F4),
		SCALE_FREQ(TEX_ISSUE_COUNT, F4),
		SCALE_FREQ(TEX_ISSUE_COUNT, F4),
		SCALE_FREQ(TEX_ISSUE_COUNT, F4),
		SCALE_FREQ(TEX_ISSUE_COUNT, F4),
		SCALE_FREQ(TEX_ISSUE_COUNT, F4),
		SCALE_FREQ(TEX_ISSUE_COUNT, F4),
		SCALE_FREQ(TEX_ISSUE_COUNT, F4) },
	  .tile_wb = {
		SCALE_FREQ(TILE_WB_COUNT, F4), SCALE_FREQ(TILE_WB_COUNT, F4),
		SCALE_FREQ(TILE_WB_COUNT, F4), SCALE_FREQ(TILE_WB_COUNT, F4),
		SCALE_FREQ(TILE_WB_COUNT, F4), SCALE_FREQ(TILE_WB_COUNT, F4),
		SCALE_FREQ(TILE_WB_COUNT, F4), SCALE_FREQ(TILE_WB_COUNT, F4) },
	  .exec_instr_msg = {
		/* No real-world values available currently */
		SCALE_FREQ(EXEC_INSTR_MSG_COUNT, F4), SCALE_FREQ(EXEC_INSTR_MSG_COUNT, F4),
		SCALE_FREQ(EXEC_INSTR_MSG_COUNT, F4), SCALE_FREQ(EXEC_INSTR_MSG_COUNT, F4),
		SCALE_FREQ(EXEC_INSTR_MSG_COUNT, F4), SCALE_FREQ(EXEC_INSTR_MSG_COUNT, F4),
		SCALE_FREQ(EXEC_INSTR_MSG_COUNT, F4), SCALE_FREQ(EXEC_INSTR_MSG_COUNT, F4) },
	  .exec_instr_fma = {
		SCALE_FREQ(EXEC_INSTR_FMA_COUNT, F4), SCALE_FREQ(EXEC_INSTR_FMA_COUNT, F4),
		SCALE_FREQ(EXEC_INSTR_FMA_COUNT, F4), SCALE_FREQ(EXEC_INSTR_FMA_COUNT, F4),
		SCALE_FREQ(EXEC_INSTR_FMA_COUNT, F4), SCALE_FREQ(EXEC_INSTR_FMA_COUNT, F4),
		SCALE_FREQ(EXEC_INSTR_FMA_COUNT, F4), SCALE_FREQ(EXEC_INSTR_FMA_COUNT, F4) },
	  .exec_instr_cvt = {
		SCALE_FREQ(EXEC_INSTR_CVT_COUNT, F4), SCALE_FREQ(EXEC_INSTR_CVT_COUNT, F4),
		SCALE_FREQ(EXEC_INSTR_CVT_COUNT, F4), SCALE_FREQ(EXEC_INSTR_CVT_COUNT, F4),
		SCALE_FREQ(EXEC_INSTR_CVT_COUNT, F4), SCALE_FREQ(EXEC_INSTR_CVT_COUNT, F4),
		SCALE_FREQ(EXEC_INSTR_CVT_COUNT, F4), SCALE_FREQ(EXEC_INSTR_CVT_COUNT, F4) },
	  .exec_instr_sfu = {
		SCALE_FREQ(EXEC_INSTR_SFU_COUNT, F4), SCALE_FREQ(EXEC_INSTR_SFU_COUNT, F4),
		SCALE_FREQ(EXEC_INSTR_SFU_COUNT, F4), SCALE_FREQ(EXEC_INSTR_SFU_COUNT, F4),
		SCALE_FREQ(EXEC_INSTR_SFU_COUNT, F4), SCALE_FREQ(EXEC_INSTR_SFU_COUNT, F4),
		SCALE_FREQ(EXEC_INSTR_SFU_COUNT, F4), SCALE_FREQ(EXEC_INSTR_SFU_COUNT, F4) },
	  .exec_starve_arith = {
		SCALE_FREQ(EXEC_STARVE_ARITH_COUNT, F4), SCALE_FREQ(EXEC_STARVE_ARITH_COUNT, F4),
		SCALE_FREQ(EXEC_STARVE_ARITH_COUNT, F4), SCALE_FREQ(EXEC_STARVE_ARITH_COUNT, F4),
		SCALE_FREQ(EXEC_STARVE_ARITH_COUNT, F4), SCALE_FREQ(EXEC_STARVE_ARITH_COUNT, F4),
		SCALE_FREQ(EXEC_STARVE_ARITH_COUNT, F4), SCALE_FREQ(EXEC_STARVE_ARITH_COUNT, F4) },
	  .tex_tfch_clk_stalled = {
		SCALE_FREQ(TEX_TFCH_CLK_STALLED_COUNT, F4), SCALE_FREQ(TEX_TFCH_CLK_STALLED_COUNT, F4),
		SCALE_FREQ(TEX_TFCH_CLK_STALLED_COUNT, F4), SCALE_FREQ(TEX_TFCH_CLK_STALLED_COUNT, F4),
		SCALE_FREQ(TEX_TFCH_CLK_STALLED_COUNT, F4), SCALE_FREQ(TEX_TFCH_CLK_STALLED_COUNT, F4),
		SCALE_FREQ(TEX_TFCH_CLK_STALLED_COUNT, F4), SCALE_FREQ(TEX_TFCH_CLK_STALLED_COUNT, F4) },
	  .tex_filt_num_operations = {
		SCALE_FREQ(TEX_FILT_NUM_OPERATIONS_COUNT, F4), SCALE_FREQ(TEX_FILT_NUM_OPERATIONS_COUNT, F4),
		SCALE_FREQ(TEX_FILT_NUM_OPERATIONS_COUNT, F4), SCALE_FREQ(TEX_FILT_NUM_OPERATIONS_COUNT, F4) },
	  .gpu_active = SCALE_FREQ(ACTIVE_COUNT, F4),
	  .idvs_pos_shad_stall = SCALE_FREQ(IDVS_POS_SHAD_STALL_COUNT, F4),
	  .prefetch_stall = SCALE_FREQ(PREFETCH_STALL_COUNT, F4),
	  .vfetch_pos_read_wait = SCALE_FREQ(VFETCH_POS_READ_WAIT_COUNT, F4),
	  .vfetch_vertex_wait = SCALE_FREQ(VFETCH_VERTEX_WAIT_COUNT, F4),
	  .primassy_stall = SCALE_FREQ(PRIMASSY_STALL_COUNT, F4),
	  .idvs_var_shad_stall = SCALE_FREQ(IDVS_VAR_SHAD_STALL_COUNT, F4),
	  .iter_stall = SCALE_FREQ(ITER_STALL_COUNT, F4),
	  .pmgr_ptr_rd_stall = SCALE_FREQ(PMGR_PTR_RD_STALL_COUNT, F4),
	  .primassy_pos_shader_wait = SCALE_FREQ(PRIMASSY_POS_SHADER_WAIT_COUNT, F4),
	  .l2_rd_msg_in_cu = {
		SCALE_FREQ(L2_RD_MSG_IN_CU_COUNT, F4), SCALE_FREQ(L2_RD_MSG_IN_CU_COUNT, F4),
		SCALE_FREQ(L2_RD_MSG_IN_CU_COUNT, F4), SCALE_FREQ(L2_RD_MSG_IN_CU_COUNT, F4) },
	  .l2_rd_msg_in = {
		SCALE_FREQ(L2_RD_MSG_IN_COUNT, F4), SCALE_FREQ(L2_RD_MSG_IN_COUNT, F4),
		SCALE_FREQ(L2_RD_MSG_IN_COUNT, F4), SCALE_FREQ(L2_RD_MSG_IN_COUNT, F4) },
	  .l2_wr_msg_in = {
		SCALE_FREQ(L2_WR_MSG_IN_COUNT, F4), SCALE_FREQ(L2_WR_MSG_IN_COUNT, F4),
		SCALE_FREQ(L2_WR_MSG_IN_COUNT, F4), SCALE_FREQ(L2_WR_MSG_IN_COUNT, F4) },
	  .l2_snp_msg_in = {
		SCALE_FREQ(L2_SNP_MSG_IN_COUNT, F4), SCALE_FREQ(L2_SNP_MSG_IN_COUNT, F4),
		SCALE_FREQ(L2_SNP_MSG_IN_COUNT, F4), SCALE_FREQ(L2_SNP_MSG_IN_COUNT, F4) },
	  .l2_rd_msg_out = {
		SCALE_FREQ(L2_RD_MSG_OUT_COUNT, F4), SCALE_FREQ(L2_RD_MSG_OUT_COUNT, F4),
		SCALE_FREQ(L2_RD_MSG_OUT_COUNT, F4), SCALE_FREQ(L2_RD_MSG_OUT_COUNT, F4) },
	  .l2_read_lookup = {
		SCALE_FREQ(L2_READ_LOOKUP_COUNT, F4), SCALE_FREQ(L2_READ_LOOKUP_COUNT, F4),
		SCALE_FREQ(L2_READ_LOOKUP_COUNT, F4), SCALE_FREQ(L2_READ_LOOKUP_COUNT, F4) },
	  .l2_ext_read_nosnp = {
		SCALE_FREQ(L2_EXT_READ_NOSNP_COUNT, F4), SCALE_FREQ(L2_EXT_READ_NOSNP_COUNT, F4),
		SCALE_FREQ(L2_EXT_READ_NOSNP_COUNT, F4), SCALE_FREQ(L2_EXT_READ_NOSNP_COUNT, F4) },
	  .l2_rd_msg_in_stall = {
		SCALE_FREQ(L2_RD_MSG_IN_STALL_COUNT, F4), SCALE_FREQ(L2_RD_MSG_IN_STALL_COUNT, F4),
		SCALE_FREQ(L2_RD_MSG_IN_STALL_COUNT, F4), SCALE_FREQ(L2_RD_MSG_IN_STALL_COUNT, F4) },
	  .l2_ext_write = {
		SCALE_FREQ(L2_EXT_WRITE_COUNT, F4), SCALE_FREQ(L2_EXT_WRITE_COUNT, F4),
		SCALE_FREQ(L2_EXT_WRITE_COUNT, F4), SCALE_FREQ(L2_EXT_WRITE_COUNT, F4) },
	  .l2_ext_write_nosnp_full = {
		SCALE_FREQ(L2_EXT_WRITE_NOSNP_FULL_COUNT, F4), SCALE_FREQ(L2_EXT_WRITE_NOSNP_FULL_COUNT, F4),
		SCALE_FREQ(L2_EXT_WRITE_NOSNP_FULL_COUNT, F4), SCALE_FREQ(L2_EXT_WRITE_NOSNP_FULL_COUNT, F4) },
	  .ls_mem_read_short = {
		SCALE_FREQ(LS_MEM_READ_SHORT_COUNT, F4), SCALE_FREQ(LS_MEM_READ_SHORT_COUNT, F4),
		SCALE_FREQ(LS_MEM_READ_SHORT_COUNT, F4), SCALE_FREQ(LS_MEM_READ_SHORT_COUNT, F4),
		SCALE_FREQ(LS_MEM_READ_SHORT_COUNT, F4), SCALE_FREQ(LS_MEM_READ_SHORT_COUNT, F4),
		SCALE_FREQ(LS_MEM_READ_SHORT_COUNT, F4), SCALE_FREQ(LS_MEM_READ_SHORT_COUNT, F4) },
	  .frag_quads_ezs_update = {
		SCALE_FREQ(FRAG_QUADS_EZS_UPDATE_COUNT, F4), SCALE_FREQ(FRAG_QUADS_EZS_UPDATE_COUNT, F4),
		SCALE_FREQ(FRAG_QUADS_EZS_UPDATE_COUNT, F4), SCALE_FREQ(FRAG_QUADS_EZS_UPDATE_COUNT, F4),
		SCALE_FREQ(FRAG_QUADS_EZS_UPDATE_COUNT, F4), SCALE_FREQ(FRAG_QUADS_EZS_UPDATE_COUNT, F4),
		SCALE_FREQ(FRAG_QUADS_EZS_UPDATE_COUNT, F4), SCALE_FREQ(FRAG_QUADS_EZS_UPDATE_COUNT, F4) },
	  .frag_starving = {
		SCALE_FREQ(FRAG_STARVING_COUNT, F4), SCALE_FREQ(FRAG_STARVING_COUNT, F4),
		SCALE_FREQ(FRAG_STARVING_COUNT, F4), SCALE_FREQ(FRAG_STARVING_COUNT, F4),
		SCALE_FREQ(FRAG_STARVING_COUNT, F4), SCALE_FREQ(FRAG_STARVING_COUNT, F4),
		SCALE_FREQ(FRAG_STARVING_COUNT, F4), SCALE_FREQ(FRAG_STARVING_COUNT, F4) },
	  .frag_partial_quads_rast = {
		SCALE_FREQ(FRAG_PARTIAL_QUADS_RAST_COUNT, F4), SCALE_FREQ(FRAG_PARTIAL_QUADS_RAST_COUNT, F4),
		SCALE_FREQ(FRAG_PARTIAL_QUADS_RAST_COUNT, F4), SCALE_FREQ(FRAG_PARTIAL_QUADS_RAST_COUNT, F4),
		SCALE_FREQ(FRAG_PARTIAL_QUADS_RAST_COUNT, F4), SCALE_FREQ(FRAG_PARTIAL_QUADS_RAST_COUNT, F4),
		SCALE_FREQ(FRAG_PARTIAL_QUADS_RAST_COUNT, F4), SCALE_FREQ(FRAG_PARTIAL_QUADS_RAST_COUNT, F4) },
	  .full_quad_warps = {
		SCALE_FREQ(FULL_QUAD_WARPS_COUNT, F4), SCALE_FREQ(FULL_QUAD_WARPS_COUNT, F4),
		SCALE_FREQ(FULL_QUAD_WARPS_COUNT, F4), SCALE_FREQ(FULL_QUAD_WARPS_COUNT, F4),
		SCALE_FREQ(FULL_QUAD_WARPS_COUNT, F4), SCALE_FREQ(FULL_QUAD_WARPS_COUNT, F4),
		SCALE_FREQ(FULL_QUAD_WARPS_COUNT, F4), SCALE_FREQ(FULL_QUAD_WARPS_COUNT, F4) },
	  .ls_mem_write_short = {
		SCALE_FREQ(LS_MEM_WRITE_SHORT_COUNT, F4), SCALE_FREQ(LS_MEM_WRITE_SHORT_COUNT, F4),
		SCALE_FREQ(LS_MEM_WRITE_SHORT_COUNT, F4), SCALE_FREQ(LS_MEM_WRITE_SHORT_COUNT, F4),
		SCALE_FREQ(LS_MEM_WRITE_SHORT_COUNT, F4), SCALE_FREQ(LS_MEM_WRITE_SHORT_COUNT, F4),
		SCALE_FREQ(LS_MEM_WRITE_SHORT_COUNT, F4), SCALE_FREQ(LS_MEM_WRITE_SHORT_COUNT, F4) },
	  .vary_slot_16 = {
		SCALE_FREQ(VARY_SLOT_16_COUNT, F4), SCALE_FREQ(VARY_SLOT_16_COUNT, F4),
		SCALE_FREQ(VARY_SLOT_16_COUNT, F4), SCALE_FREQ(VARY_SLOT_16_COUNT, F4),
		SCALE_FREQ(VARY_SLOT_16_COUNT, F4), SCALE_FREQ(VARY_SLOT_16_COUNT, F4),
		SCALE_FREQ(VARY_SLOT_16_COUNT, F4), SCALE_FREQ(VARY_SLOT_16_COUNT, F4) },
	  .beats_rd_lsc_ext = {
		SCALE_FREQ(BEATS_RD_LSC_EXT_COUNT, F4), SCALE_FREQ(BEATS_RD_LSC_EXT_COUNT, F4),
		SCALE_FREQ(BEATS_RD_LSC_EXT_COUNT, F4), SCALE_FREQ(BEATS_RD_LSC_EXT_COUNT, F4),
		SCALE_FREQ(BEATS_RD_LSC_EXT_COUNT, F4), SCALE_FREQ(BEATS_RD_LSC_EXT_COUNT, F4),
		SCALE_FREQ(BEATS_RD_LSC_EXT_COUNT, F4), SCALE_FREQ(BEATS_RD_LSC_EXT_COUNT, F4) },
	  .beats_rd_tex = {
		SCALE_FREQ(BEATS_RD_TEX_COUNT, F4), SCALE_FREQ(BEATS_RD_TEX_COUNT, F4),
		SCALE_FREQ(BEATS_RD_TEX_COUNT, F4), SCALE_FREQ(BEATS_RD_TEX_COUNT, F4),
		SCALE_FREQ(BEATS_RD_TEX_COUNT, F4), SCALE_FREQ(BEATS_RD_TEX_COUNT, F4),
		SCALE_FREQ(BEATS_RD_TEX_COUNT, F4), SCALE_FREQ(BEATS_RD_TEX_COUNT, F4) },
	  .beats_rd_tex_ext = {
		SCALE_FREQ(BEATS_RD_TEX_EXT_COUNT, F4), SCALE_FREQ(BEATS_RD_TEX_EXT_COUNT, F4),
		SCALE_FREQ(BEATS_RD_TEX_EXT_COUNT, F4), SCALE_FREQ(BEATS_RD_TEX_EXT_COUNT, F4),
		SCALE_FREQ(BEATS_RD_TEX_EXT_COUNT, F4), SCALE_FREQ(BEATS_RD_TEX_EXT_COUNT, F4),
		SCALE_FREQ(BEATS_RD_TEX_EXT_COUNT, F4), SCALE_FREQ(BEATS_RD_TEX_EXT_COUNT, F4) },
	  .frag_quads_coarse = {
		SCALE_FREQ(FRAG_QUADS_COARSE_COUNT, F4), SCALE_FREQ(FRAG_QUADS_COARSE_COUNT, F4),
		SCALE_FREQ(FRAG_QUADS_COARSE_COUNT, F4), SCALE_FREQ(FRAG_QUADS_COARSE_COUNT, F4),
		SCALE_FREQ(FRAG_QUADS_COARSE_COUNT, F4), SCALE_FREQ(FRAG_QUADS_COARSE_COUNT, F4),
		SCALE_FREQ(FRAG_QUADS_COARSE_COUNT, F4), SCALE_FREQ(FRAG_QUADS_COARSE_COUNT, F4) },
	  .rt_rays_started = {
		SCALE_FREQ(RT_RAYS_STARTED_COUNT, F4), SCALE_FREQ(RT_RAYS_STARTED_COUNT, F4),
		SCALE_FREQ(RT_RAYS_STARTED_COUNT, F4), SCALE_FREQ(RT_RAYS_STARTED_COUNT, F4),
		SCALE_FREQ(RT_RAYS_STARTED_COUNT, F4), SCALE_FREQ(RT_RAYS_STARTED_COUNT, F4),
		SCALE_FREQ(RT_RAYS_STARTED_COUNT, F4), SCALE_FREQ(RT_RAYS_STARTED_COUNT, F4) },
	  .tex_cfch_num_l1_ct_operations = {
		SCALE_FREQ(TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, F4), SCALE_FREQ(TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, F4),
		SCALE_FREQ(TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, F4), SCALE_FREQ(TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, F4),
		SCALE_FREQ(TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, F4), SCALE_FREQ(TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, F4),
		SCALE_FREQ(TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, F4), SCALE_FREQ(TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, F4) },
	  .exec_instr_slot1 = {
		SCALE_FREQ(EXEC_INSTR_SLOT1_COUNT, F4), SCALE_FREQ(EXEC_INSTR_SLOT1_COUNT, F4),
		SCALE_FREQ(EXEC_INSTR_SLOT1_COUNT, F4), SCALE_FREQ(EXEC_INSTR_SLOT1_COUNT, F4),
		SCALE_FREQ(EXEC_INSTR_SLOT1_COUNT, F4), SCALE_FREQ(EXEC_INSTR_SLOT1_COUNT, F4),
		SCALE_FREQ(EXEC_INSTR_SLOT1_COUNT, F4), SCALE_FREQ(EXEC_INSTR_SLOT1_COUNT, F4) },
	  .exec_issue_slot_any = {
		SCALE_FREQ(EXEC_ISSUE_SLOT_ANY_COUNT, F4), SCALE_FREQ(EXEC_ISSUE_SLOT_ANY_COUNT, F4),
		SCALE_FREQ(EXEC_ISSUE_SLOT_ANY_COUNT, F4), SCALE_FREQ(EXEC_ISSUE_SLOT_ANY_COUNT, F4),
		SCALE_FREQ(EXEC_ISSUE_SLOT_ANY_COUNT, F4), SCALE_FREQ(EXEC_ISSUE_SLOT_ANY_COUNT, F4),
		SCALE_FREQ(EXEC_ISSUE_SLOT_ANY_COUNT, F4), SCALE_FREQ(EXEC_ISSUE_SLOT_ANY_COUNT, F4) },
	  .interval_ns = INTERVAL_NS,
	  .scale = SCALE,
	  .temp = TEMP,
	  .util = 0.80,
	},

	{ .freq = F5,
	  .voltage = V5,
	  .num_l2_slices = 1,
	  .l2_access = { SCALE_FREQ(L2_COUNT, F5), },
	  .num_cores = 4,
	  .exec_instr_count = {
		SCALE_FREQ(INSTR_COUNT, F5), SCALE_FREQ(INSTR_COUNT, F5),
		SCALE_FREQ(INSTR_COUNT, F5), SCALE_FREQ(INSTR_COUNT, F5) },
	  .tex_issue = {
		SCALE_FREQ(TEX_ISSUE_COUNT, F5),
		SCALE_FREQ(TEX_ISSUE_COUNT, F5),
		SCALE_FREQ(TEX_ISSUE_COUNT, F5),
		SCALE_FREQ(TEX_ISSUE_COUNT, F5) },
	  .tile_wb = {
		SCALE_FREQ(TILE_WB_COUNT, F5), SCALE_FREQ(TILE_WB_COUNT, F5),
		SCALE_FREQ(TILE_WB_COUNT, F5), SCALE_FREQ(TILE_WB_COUNT, F5) },
	  .exec_instr_msg = {
		/* No real-world values available currently */
		SCALE_FREQ(EXEC_INSTR_MSG_COUNT, F5), SCALE_FREQ(EXEC_INSTR_MSG_COUNT, F5),
		SCALE_FREQ(EXEC_INSTR_MSG_COUNT, F5), SCALE_FREQ(EXEC_INSTR_MSG_COUNT, F5) },
	  .exec_instr_fma = {
		SCALE_FREQ(EXEC_INSTR_FMA_COUNT, F5), SCALE_FREQ(EXEC_INSTR_FMA_COUNT, F5),
		SCALE_FREQ(EXEC_INSTR_FMA_COUNT, F5), SCALE_FREQ(EXEC_INSTR_FMA_COUNT, F5) },
	  .exec_instr_cvt = {
		SCALE_FREQ(EXEC_INSTR_CVT_COUNT, F5), SCALE_FREQ(EXEC_INSTR_CVT_COUNT, F5),
		SCALE_FREQ(EXEC_INSTR_CVT_COUNT, F5), SCALE_FREQ(EXEC_INSTR_CVT_COUNT, F5) },
	  .exec_instr_sfu = {
		SCALE_FREQ(EXEC_INSTR_SFU_COUNT, F5), SCALE_FREQ(EXEC_INSTR_SFU_COUNT, F5),
		SCALE_FREQ(EXEC_INSTR_SFU_COUNT, F5), SCALE_FREQ(EXEC_INSTR_SFU_COUNT, F5) },
	  .exec_starve_arith = {
		SCALE_FREQ(EXEC_STARVE_ARITH_COUNT, F5), SCALE_FREQ(EXEC_STARVE_ARITH_COUNT, F5),
		SCALE_FREQ(EXEC_STARVE_ARITH_COUNT, F5), SCALE_FREQ(EXEC_STARVE_ARITH_COUNT, F5) },
	  .tex_tfch_clk_stalled = {
		SCALE_FREQ(TEX_TFCH_CLK_STALLED_COUNT, F5), SCALE_FREQ(TEX_TFCH_CLK_STALLED_COUNT, F5),
		SCALE_FREQ(TEX_TFCH_CLK_STALLED_COUNT, F5), SCALE_FREQ(TEX_TFCH_CLK_STALLED_COUNT, F5) },
	  .tex_filt_num_operations = {
		SCALE_FREQ(TEX_FILT_NUM_OPERATIONS_COUNT, F5), SCALE_FREQ(TEX_FILT_NUM_OPERATIONS_COUNT, F5),
		SCALE_FREQ(TEX_FILT_NUM_OPERATIONS_COUNT, F5), SCALE_FREQ(TEX_FILT_NUM_OPERATIONS_COUNT, F5) },
	  .gpu_active = SCALE_FREQ(ACTIVE_COUNT, F5),
	  .idvs_pos_shad_stall = SCALE_FREQ(IDVS_POS_SHAD_STALL_COUNT, F5),
	  .prefetch_stall = SCALE_FREQ(PREFETCH_STALL_COUNT, F5),
	  .vfetch_pos_read_wait = SCALE_FREQ(VFETCH_POS_READ_WAIT_COUNT, F5),
	  .vfetch_vertex_wait = SCALE_FREQ(VFETCH_VERTEX_WAIT_COUNT, F5),
	  .primassy_stall = SCALE_FREQ(PRIMASSY_STALL_COUNT, F5),
	  .idvs_var_shad_stall = SCALE_FREQ(IDVS_VAR_SHAD_STALL_COUNT, F5),
	  .iter_stall = SCALE_FREQ(ITER_STALL_COUNT, F5),
	  .pmgr_ptr_rd_stall = SCALE_FREQ(PMGR_PTR_RD_STALL_COUNT, F5),
	  .primassy_pos_shader_wait = SCALE_FREQ(PRIMASSY_POS_SHADER_WAIT_COUNT, F5),
	  .l2_rd_msg_in_cu = {
		SCALE_FREQ(L2_RD_MSG_IN_CU_COUNT, F5), SCALE_FREQ(L2_RD_MSG_IN_CU_COUNT, F5),
		SCALE_FREQ(L2_RD_MSG_IN_CU_COUNT, F5), SCALE_FREQ(L2_RD_MSG_IN_CU_COUNT, F5) },
	  .l2_rd_msg_in = {
		SCALE_FREQ(L2_RD_MSG_IN_COUNT, F5), SCALE_FREQ(L2_RD_MSG_IN_COUNT, F5),
		SCALE_FREQ(L2_RD_MSG_IN_COUNT, F5), SCALE_FREQ(L2_RD_MSG_IN_COUNT, F5) },
	  .l2_wr_msg_in = {
		SCALE_FREQ(L2_WR_MSG_IN_COUNT, F5), SCALE_FREQ(L2_WR_MSG_IN_COUNT, F5),
		SCALE_FREQ(L2_WR_MSG_IN_COUNT, F5), SCALE_FREQ(L2_WR_MSG_IN_COUNT, F5) },
	  .l2_snp_msg_in = {
		SCALE_FREQ(L2_SNP_MSG_IN_COUNT, F5), SCALE_FREQ(L2_SNP_MSG_IN_COUNT, F5),
		SCALE_FREQ(L2_SNP_MSG_IN_COUNT, F5), SCALE_FREQ(L2_SNP_MSG_IN_COUNT, F5) },
	  .l2_rd_msg_out = {
		SCALE_FREQ(L2_RD_MSG_OUT_COUNT, F5), SCALE_FREQ(L2_RD_MSG_OUT_COUNT, F5),
		SCALE_FREQ(L2_RD_MSG_OUT_COUNT, F5), SCALE_FREQ(L2_RD_MSG_OUT_COUNT, F5) },
	  .l2_read_lookup = {
		SCALE_FREQ(L2_READ_LOOKUP_COUNT, F5), SCALE_FREQ(L2_READ_LOOKUP_COUNT, F5),
		SCALE_FREQ(L2_READ_LOOKUP_COUNT, F5), SCALE_FREQ(L2_READ_LOOKUP_COUNT, F5) },
	  .l2_ext_read_nosnp = {
		SCALE_FREQ(L2_EXT_READ_NOSNP_COUNT, F5), SCALE_FREQ(L2_EXT_READ_NOSNP_COUNT, F5),
		SCALE_FREQ(L2_EXT_READ_NOSNP_COUNT, F5), SCALE_FREQ(L2_EXT_READ_NOSNP_COUNT, F5) },
	  .l2_rd_msg_in_stall = {
		SCALE_FREQ(L2_RD_MSG_IN_STALL_COUNT, F5), SCALE_FREQ(L2_RD_MSG_IN_STALL_COUNT, F5),
		SCALE_FREQ(L2_RD_MSG_IN_STALL_COUNT, F5), SCALE_FREQ(L2_RD_MSG_IN_STALL_COUNT, F5) },
	  .l2_ext_write = {
		SCALE_FREQ(L2_EXT_WRITE_COUNT, F5), SCALE_FREQ(L2_EXT_WRITE_COUNT, F5),
		SCALE_FREQ(L2_EXT_WRITE_COUNT, F5), SCALE_FREQ(L2_EXT_WRITE_COUNT, F5) },
	  .l2_ext_write_nosnp_full = {
		SCALE_FREQ(L2_EXT_WRITE_NOSNP_FULL_COUNT, F5), SCALE_FREQ(L2_EXT_WRITE_NOSNP_FULL_COUNT, F5),
		SCALE_FREQ(L2_EXT_WRITE_NOSNP_FULL_COUNT, F5), SCALE_FREQ(L2_EXT_WRITE_NOSNP_FULL_COUNT, F5) },
	  .ls_mem_read_short = {
		SCALE_FREQ(LS_MEM_READ_SHORT_COUNT, F5), SCALE_FREQ(LS_MEM_READ_SHORT_COUNT, F5),
		SCALE_FREQ(LS_MEM_READ_SHORT_COUNT, F5), SCALE_FREQ(LS_MEM_READ_SHORT_COUNT, F5) },
	  .frag_quads_ezs_update = {
		SCALE_FREQ(FRAG_QUADS_EZS_UPDATE_COUNT, F5), SCALE_FREQ(FRAG_QUADS_EZS_UPDATE_COUNT, F5),
		SCALE_FREQ(FRAG_QUADS_EZS_UPDATE_COUNT, F5), SCALE_FREQ(FRAG_QUADS_EZS_UPDATE_COUNT, F5) },
	  .frag_starving = {
		SCALE_FREQ(FRAG_STARVING_COUNT, F5), SCALE_FREQ(FRAG_STARVING_COUNT, F5),
		SCALE_FREQ(FRAG_STARVING_COUNT, F5), SCALE_FREQ(FRAG_STARVING_COUNT, F5) },
	  .frag_partial_quads_rast = {
		SCALE_FREQ(FRAG_PARTIAL_QUADS_RAST_COUNT, F5), SCALE_FREQ(FRAG_PARTIAL_QUADS_RAST_COUNT, F5),
		SCALE_FREQ(FRAG_PARTIAL_QUADS_RAST_COUNT, F5), SCALE_FREQ(FRAG_PARTIAL_QUADS_RAST_COUNT, F5) },
	  .full_quad_warps = {
		SCALE_FREQ(FULL_QUAD_WARPS_COUNT, F5), SCALE_FREQ(FULL_QUAD_WARPS_COUNT, F5),
		SCALE_FREQ(FULL_QUAD_WARPS_COUNT, F5), SCALE_FREQ(FULL_QUAD_WARPS_COUNT, F5) },
	  .ls_mem_write_short = {
		SCALE_FREQ(LS_MEM_WRITE_SHORT_COUNT, F5), SCALE_FREQ(LS_MEM_WRITE_SHORT_COUNT, F5),
		SCALE_FREQ(LS_MEM_WRITE_SHORT_COUNT, F5), SCALE_FREQ(LS_MEM_WRITE_SHORT_COUNT, F5) },
	  .vary_slot_16 = {
		SCALE_FREQ(VARY_SLOT_16_COUNT, F5), SCALE_FREQ(VARY_SLOT_16_COUNT, F5),
		SCALE_FREQ(VARY_SLOT_16_COUNT, F5), SCALE_FREQ(VARY_SLOT_16_COUNT, F5) },
	  .beats_rd_lsc_ext = {
		SCALE_FREQ(BEATS_RD_LSC_EXT_COUNT, F5), SCALE_FREQ(BEATS_RD_LSC_EXT_COUNT, F5),
		SCALE_FREQ(BEATS_RD_LSC_EXT_COUNT, F5), SCALE_FREQ(BEATS_RD_LSC_EXT_COUNT, F5) },
	  .beats_rd_tex = {
		SCALE_FREQ(BEATS_RD_TEX_COUNT, F5), SCALE_FREQ(BEATS_RD_TEX_COUNT, F5),
		SCALE_FREQ(BEATS_RD_TEX_COUNT, F5), SCALE_FREQ(BEATS_RD_TEX_COUNT, F5) },
	  .beats_rd_tex_ext = {
		SCALE_FREQ(BEATS_RD_TEX_EXT_COUNT, F5), SCALE_FREQ(BEATS_RD_TEX_EXT_COUNT, F5),
		SCALE_FREQ(BEATS_RD_TEX_EXT_COUNT, F5), SCALE_FREQ(BEATS_RD_TEX_EXT_COUNT, F5) },
	  .frag_quads_coarse = {
		SCALE_FREQ(FRAG_QUADS_COARSE_COUNT, F5), SCALE_FREQ(FRAG_QUADS_COARSE_COUNT, F5),
		SCALE_FREQ(FRAG_QUADS_COARSE_COUNT, F5), SCALE_FREQ(FRAG_QUADS_COARSE_COUNT, F5) },
	  .rt_rays_started = {
		SCALE_FREQ(RT_RAYS_STARTED_COUNT, F5), SCALE_FREQ(RT_RAYS_STARTED_COUNT, F5),
		SCALE_FREQ(RT_RAYS_STARTED_COUNT, F5), SCALE_FREQ(RT_RAYS_STARTED_COUNT, F5) },
	  .tex_cfch_num_l1_ct_operations = {
		SCALE_FREQ(TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, F5), SCALE_FREQ(TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, F5),
		SCALE_FREQ(TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, F5), SCALE_FREQ(TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, F5) },
	  .exec_instr_slot1 = {
		SCALE_FREQ(EXEC_INSTR_SLOT1_COUNT, F5), SCALE_FREQ(EXEC_INSTR_SLOT1_COUNT, F5),
		SCALE_FREQ(EXEC_INSTR_SLOT1_COUNT, F5), SCALE_FREQ(EXEC_INSTR_SLOT1_COUNT, F5) },
	  .exec_issue_slot_any = {
		SCALE_FREQ(EXEC_ISSUE_SLOT_ANY_COUNT, F5), SCALE_FREQ(EXEC_ISSUE_SLOT_ANY_COUNT, F5),
		SCALE_FREQ(EXEC_ISSUE_SLOT_ANY_COUNT, F5), SCALE_FREQ(EXEC_ISSUE_SLOT_ANY_COUNT, F5) },
	  .interval_ns = INTERVAL_NS,
	  .scale = SCALE,
	  .temp = TEMP,
	  .util = 0.13,
	},

	/* Operating points with small/large intervals */
	{ .freq = FREQ_HZ,
	  .voltage = VOLTAGE_MV,
	  .num_l2_slices = 1,
	  .l2_access = { SCALE_INTERVAL(L2_COUNT, NANOSEC_PER_SEC/1000), },
	  .num_cores = 4,
	  .exec_instr_count = {
		SCALE_INTERVAL(INSTR_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(INSTR_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(INSTR_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(INSTR_COUNT, NANOSEC_PER_SEC/1000) },
	  .tex_issue = {
		SCALE_INTERVAL(TEX_ISSUE_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(TEX_ISSUE_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(TEX_ISSUE_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(TEX_ISSUE_COUNT, NANOSEC_PER_SEC/1000) },
	  .tile_wb = {
		SCALE_INTERVAL(TILE_WB_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(TILE_WB_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(TILE_WB_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(TILE_WB_COUNT, NANOSEC_PER_SEC/1000) },
	  .exec_instr_msg = {
		SCALE_INTERVAL(EXEC_INSTR_MSG_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(EXEC_INSTR_MSG_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(EXEC_INSTR_MSG_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(EXEC_INSTR_MSG_COUNT, NANOSEC_PER_SEC/1000) },
	  .exec_instr_fma = {
		SCALE_INTERVAL(EXEC_INSTR_FMA_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(EXEC_INSTR_FMA_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(EXEC_INSTR_FMA_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(EXEC_INSTR_FMA_COUNT, NANOSEC_PER_SEC/1000) },
	  .exec_instr_cvt = {
		SCALE_INTERVAL(EXEC_INSTR_CVT_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(EXEC_INSTR_CVT_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(EXEC_INSTR_CVT_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(EXEC_INSTR_CVT_COUNT, NANOSEC_PER_SEC/1000) },
	  .exec_instr_sfu = {
		SCALE_INTERVAL(EXEC_INSTR_SFU_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(EXEC_INSTR_SFU_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(EXEC_INSTR_SFU_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(EXEC_INSTR_SFU_COUNT, NANOSEC_PER_SEC/1000) },
	  .exec_starve_arith = {
		SCALE_INTERVAL(EXEC_STARVE_ARITH_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(EXEC_STARVE_ARITH_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(EXEC_STARVE_ARITH_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(EXEC_STARVE_ARITH_COUNT, NANOSEC_PER_SEC/1000) },
	  .tex_tfch_clk_stalled = {
		SCALE_INTERVAL(TEX_TFCH_CLK_STALLED_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(TEX_TFCH_CLK_STALLED_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(TEX_TFCH_CLK_STALLED_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(TEX_TFCH_CLK_STALLED_COUNT, NANOSEC_PER_SEC/1000) },
	  .tex_filt_num_operations = {
		SCALE_INTERVAL(TEX_FILT_NUM_OPERATIONS_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(TEX_FILT_NUM_OPERATIONS_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(TEX_FILT_NUM_OPERATIONS_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(TEX_FILT_NUM_OPERATIONS_COUNT, NANOSEC_PER_SEC/1000) },
	  .gpu_active = SCALE_INTERVAL(ACTIVE_COUNT, NANOSEC_PER_SEC/1000),
	  .idvs_pos_shad_stall = SCALE_INTERVAL(IDVS_POS_SHAD_STALL_COUNT, NANOSEC_PER_SEC/1000),
	  .prefetch_stall = SCALE_INTERVAL(PREFETCH_STALL_COUNT, NANOSEC_PER_SEC/1000),
	  .vfetch_pos_read_wait = SCALE_INTERVAL(VFETCH_POS_READ_WAIT_COUNT, NANOSEC_PER_SEC/1000),
	  .vfetch_vertex_wait = SCALE_INTERVAL(VFETCH_VERTEX_WAIT_COUNT, NANOSEC_PER_SEC/1000),
	  .primassy_stall = SCALE_INTERVAL(PRIMASSY_STALL_COUNT, NANOSEC_PER_SEC/1000),
	  .idvs_var_shad_stall = SCALE_INTERVAL(IDVS_VAR_SHAD_STALL_COUNT, NANOSEC_PER_SEC/1000),
	  .iter_stall = SCALE_INTERVAL(ITER_STALL_COUNT, NANOSEC_PER_SEC/1000),
	  .pmgr_ptr_rd_stall = SCALE_INTERVAL(PMGR_PTR_RD_STALL_COUNT, NANOSEC_PER_SEC/1000),
	  .primassy_pos_shader_wait = SCALE_INTERVAL(PRIMASSY_POS_SHADER_WAIT_COUNT, NANOSEC_PER_SEC/1000),
	  .l2_rd_msg_in_cu = {
		SCALE_INTERVAL(L2_RD_MSG_IN_CU_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(L2_RD_MSG_IN_CU_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(L2_RD_MSG_IN_CU_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(L2_RD_MSG_IN_CU_COUNT, NANOSEC_PER_SEC/1000) },
	  .l2_rd_msg_in = {
		SCALE_INTERVAL(L2_RD_MSG_IN_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(L2_RD_MSG_IN_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(L2_RD_MSG_IN_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(L2_RD_MSG_IN_COUNT, NANOSEC_PER_SEC/1000) },
	  .l2_wr_msg_in = {
		SCALE_INTERVAL(L2_WR_MSG_IN_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(L2_WR_MSG_IN_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(L2_WR_MSG_IN_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(L2_WR_MSG_IN_COUNT, NANOSEC_PER_SEC/1000) },
	  .l2_snp_msg_in = {
		SCALE_INTERVAL(L2_SNP_MSG_IN_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(L2_SNP_MSG_IN_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(L2_SNP_MSG_IN_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(L2_SNP_MSG_IN_COUNT, NANOSEC_PER_SEC/1000) },
	  .l2_rd_msg_out = {
		SCALE_INTERVAL(L2_RD_MSG_OUT_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(L2_RD_MSG_OUT_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(L2_RD_MSG_OUT_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(L2_RD_MSG_OUT_COUNT, NANOSEC_PER_SEC/1000) },
	  .l2_read_lookup = {
		SCALE_INTERVAL(L2_READ_LOOKUP_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(L2_READ_LOOKUP_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(L2_READ_LOOKUP_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(L2_READ_LOOKUP_COUNT, NANOSEC_PER_SEC/1000) },
	  .l2_ext_read_nosnp = {
		SCALE_INTERVAL(L2_EXT_READ_NOSNP_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(L2_EXT_READ_NOSNP_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(L2_EXT_READ_NOSNP_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(L2_EXT_READ_NOSNP_COUNT, NANOSEC_PER_SEC/1000) },
	  .l2_rd_msg_in_stall = {
		SCALE_INTERVAL(L2_RD_MSG_IN_STALL_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(L2_RD_MSG_IN_STALL_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(L2_RD_MSG_IN_STALL_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(L2_RD_MSG_IN_STALL_COUNT, NANOSEC_PER_SEC/1000) },
	  .l2_ext_write = {
		SCALE_INTERVAL(L2_EXT_WRITE_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(L2_EXT_WRITE_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(L2_EXT_WRITE_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(L2_EXT_WRITE_COUNT, NANOSEC_PER_SEC/1000) },
	  .l2_ext_write_nosnp_full = {
		SCALE_INTERVAL(L2_EXT_WRITE_NOSNP_FULL_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(L2_EXT_WRITE_NOSNP_FULL_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(L2_EXT_WRITE_NOSNP_FULL_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(L2_EXT_WRITE_NOSNP_FULL_COUNT, NANOSEC_PER_SEC/1000) },
	  .ls_mem_read_short = {
		SCALE_INTERVAL(LS_MEM_READ_SHORT_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(LS_MEM_READ_SHORT_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(LS_MEM_READ_SHORT_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(LS_MEM_READ_SHORT_COUNT, NANOSEC_PER_SEC/1000) },
	  .frag_starving = {
		SCALE_INTERVAL(FRAG_STARVING_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(FRAG_STARVING_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(FRAG_STARVING_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(FRAG_STARVING_COUNT, NANOSEC_PER_SEC/1000) },
	  .frag_partial_quads_rast = {
		SCALE_INTERVAL(FRAG_PARTIAL_QUADS_RAST_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(FRAG_PARTIAL_QUADS_RAST_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(FRAG_PARTIAL_QUADS_RAST_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(FRAG_PARTIAL_QUADS_RAST_COUNT, NANOSEC_PER_SEC/1000) },
	  .frag_quads_ezs_update = {
		SCALE_INTERVAL(FRAG_QUADS_EZS_UPDATE_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(FRAG_QUADS_EZS_UPDATE_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(FRAG_QUADS_EZS_UPDATE_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(FRAG_QUADS_EZS_UPDATE_COUNT, NANOSEC_PER_SEC/1000) },
	  .full_quad_warps = {
		SCALE_INTERVAL(FULL_QUAD_WARPS_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(FULL_QUAD_WARPS_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(FULL_QUAD_WARPS_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(FULL_QUAD_WARPS_COUNT, NANOSEC_PER_SEC/1000) },
	  .ls_mem_write_short = {
		SCALE_INTERVAL(LS_MEM_WRITE_SHORT_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(LS_MEM_WRITE_SHORT_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(LS_MEM_WRITE_SHORT_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(LS_MEM_WRITE_SHORT_COUNT, NANOSEC_PER_SEC/1000) },
	  .vary_slot_16 = {
		SCALE_INTERVAL(VARY_SLOT_16_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(VARY_SLOT_16_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(VARY_SLOT_16_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(VARY_SLOT_16_COUNT, NANOSEC_PER_SEC/1000) },
	  .beats_rd_lsc_ext = {
		SCALE_INTERVAL(BEATS_RD_LSC_EXT_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(BEATS_RD_LSC_EXT_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(BEATS_RD_LSC_EXT_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(BEATS_RD_LSC_EXT_COUNT, NANOSEC_PER_SEC/1000) },
	  .beats_rd_tex = {
		SCALE_INTERVAL(BEATS_RD_TEX_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(BEATS_RD_TEX_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(BEATS_RD_TEX_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(BEATS_RD_TEX_COUNT, NANOSEC_PER_SEC/1000) },
	  .beats_rd_tex_ext = {
		SCALE_INTERVAL(BEATS_RD_TEX_EXT_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(BEATS_RD_TEX_EXT_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(BEATS_RD_TEX_EXT_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(BEATS_RD_TEX_EXT_COUNT, NANOSEC_PER_SEC/1000) },
	  .frag_quads_coarse = {
		SCALE_INTERVAL(FRAG_QUADS_COARSE_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(FRAG_QUADS_COARSE_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(FRAG_QUADS_COARSE_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(FRAG_QUADS_COARSE_COUNT, NANOSEC_PER_SEC/1000) },
	  .rt_rays_started = {
		SCALE_INTERVAL(RT_RAYS_STARTED_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(RT_RAYS_STARTED_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(RT_RAYS_STARTED_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(RT_RAYS_STARTED_COUNT, NANOSEC_PER_SEC/1000) },
	  .tex_cfch_num_l1_ct_operations = {
		SCALE_INTERVAL(TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, NANOSEC_PER_SEC/1000) },
	  .exec_instr_slot1 = {
		SCALE_INTERVAL(EXEC_INSTR_SLOT1_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(EXEC_INSTR_SLOT1_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(EXEC_INSTR_SLOT1_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(EXEC_INSTR_SLOT1_COUNT, NANOSEC_PER_SEC/1000) },
	  .exec_issue_slot_any = {
		SCALE_INTERVAL(EXEC_ISSUE_SLOT_ANY_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(EXEC_ISSUE_SLOT_ANY_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(EXEC_ISSUE_SLOT_ANY_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(EXEC_ISSUE_SLOT_ANY_COUNT, NANOSEC_PER_SEC/1000) },
	  .interval_ns = NANOSEC_PER_SEC/1000,
	  .scale = SCALE,
	  .temp = TEMP,
	  .util = 0.21,
	},

	{ .freq = FREQ_HZ,
	  .voltage = VOLTAGE_MV,
	  .num_l2_slices = 1,
	  .l2_access = { SCALE_INTERVAL(L2_COUNT, NANOSEC_PER_SEC/100), },
	  .num_cores = 4,
	  .exec_instr_count = {
		SCALE_INTERVAL(INSTR_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(INSTR_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(INSTR_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(INSTR_COUNT, NANOSEC_PER_SEC/100) },
	  .tex_issue = {
		SCALE_INTERVAL(TEX_ISSUE_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(TEX_ISSUE_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(TEX_ISSUE_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(TEX_ISSUE_COUNT, NANOSEC_PER_SEC/100) },
	  .tile_wb = {
		SCALE_INTERVAL(TILE_WB_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(TILE_WB_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(TILE_WB_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(TILE_WB_COUNT, NANOSEC_PER_SEC/100) },
	  .exec_instr_msg = {
		SCALE_INTERVAL(EXEC_INSTR_MSG_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(EXEC_INSTR_MSG_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(EXEC_INSTR_MSG_COUNT, NANOSEC_PER_SEC/1000),
		SCALE_INTERVAL(EXEC_INSTR_MSG_COUNT, NANOSEC_PER_SEC/1000) },
	  .exec_instr_fma = {
		SCALE_INTERVAL(EXEC_INSTR_FMA_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(EXEC_INSTR_FMA_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(EXEC_INSTR_FMA_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(EXEC_INSTR_FMA_COUNT, NANOSEC_PER_SEC/100) },
	  .exec_instr_cvt = {
		SCALE_INTERVAL(EXEC_INSTR_CVT_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(EXEC_INSTR_CVT_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(EXEC_INSTR_CVT_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(EXEC_INSTR_CVT_COUNT, NANOSEC_PER_SEC/100) },
	  .exec_instr_sfu = {
		SCALE_INTERVAL(EXEC_INSTR_SFU_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(EXEC_INSTR_SFU_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(EXEC_INSTR_SFU_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(EXEC_INSTR_SFU_COUNT, NANOSEC_PER_SEC/100) },
	  .exec_starve_arith = {
		SCALE_INTERVAL(EXEC_STARVE_ARITH_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(EXEC_STARVE_ARITH_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(EXEC_STARVE_ARITH_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(EXEC_STARVE_ARITH_COUNT, NANOSEC_PER_SEC/100) },
	  .tex_tfch_clk_stalled = {
		SCALE_INTERVAL(TEX_TFCH_CLK_STALLED_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(TEX_TFCH_CLK_STALLED_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(TEX_TFCH_CLK_STALLED_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(TEX_TFCH_CLK_STALLED_COUNT, NANOSEC_PER_SEC/100) },
	  .tex_filt_num_operations = {
		SCALE_INTERVAL(TEX_FILT_NUM_OPERATIONS_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(TEX_FILT_NUM_OPERATIONS_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(TEX_FILT_NUM_OPERATIONS_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(TEX_FILT_NUM_OPERATIONS_COUNT, NANOSEC_PER_SEC/100) },
	  .gpu_active = SCALE_INTERVAL(ACTIVE_COUNT, NANOSEC_PER_SEC/100),
	  .idvs_pos_shad_stall = SCALE_INTERVAL(IDVS_POS_SHAD_STALL_COUNT, NANOSEC_PER_SEC/100),
	  .prefetch_stall = SCALE_INTERVAL(PREFETCH_STALL_COUNT, NANOSEC_PER_SEC/100),
	  .vfetch_pos_read_wait = SCALE_INTERVAL(VFETCH_POS_READ_WAIT_COUNT, NANOSEC_PER_SEC/100),
	  .vfetch_vertex_wait = SCALE_INTERVAL(VFETCH_VERTEX_WAIT_COUNT, NANOSEC_PER_SEC/100),
	  .primassy_stall = SCALE_INTERVAL(PRIMASSY_STALL_COUNT, NANOSEC_PER_SEC/100),
	  .idvs_var_shad_stall = SCALE_INTERVAL(IDVS_VAR_SHAD_STALL_COUNT, NANOSEC_PER_SEC/100),
	  .iter_stall = SCALE_INTERVAL(ITER_STALL_COUNT, NANOSEC_PER_SEC/100),
	  .pmgr_ptr_rd_stall = SCALE_INTERVAL(PMGR_PTR_RD_STALL_COUNT, NANOSEC_PER_SEC/100),
	  .primassy_pos_shader_wait = SCALE_INTERVAL(PRIMASSY_POS_SHADER_WAIT_COUNT, NANOSEC_PER_SEC/100),
	  .l2_rd_msg_in_cu = {
		SCALE_INTERVAL(L2_RD_MSG_IN_CU_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(L2_RD_MSG_IN_CU_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(L2_RD_MSG_IN_CU_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(L2_RD_MSG_IN_CU_COUNT, NANOSEC_PER_SEC/100) },
	  .l2_rd_msg_in = {
		SCALE_INTERVAL(L2_RD_MSG_IN_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(L2_RD_MSG_IN_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(L2_RD_MSG_IN_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(L2_RD_MSG_IN_COUNT, NANOSEC_PER_SEC/100) },
	  .l2_wr_msg_in = {
		SCALE_INTERVAL(L2_WR_MSG_IN_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(L2_WR_MSG_IN_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(L2_WR_MSG_IN_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(L2_WR_MSG_IN_COUNT, NANOSEC_PER_SEC/100) },
	  .l2_snp_msg_in = {
		SCALE_INTERVAL(L2_SNP_MSG_IN_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(L2_SNP_MSG_IN_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(L2_SNP_MSG_IN_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(L2_SNP_MSG_IN_COUNT, NANOSEC_PER_SEC/100) },
	  .l2_rd_msg_out = {
		SCALE_INTERVAL(L2_RD_MSG_OUT_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(L2_RD_MSG_OUT_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(L2_RD_MSG_OUT_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(L2_RD_MSG_OUT_COUNT, NANOSEC_PER_SEC/100) },
	  .l2_read_lookup = {
		SCALE_INTERVAL(L2_READ_LOOKUP_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(L2_READ_LOOKUP_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(L2_READ_LOOKUP_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(L2_READ_LOOKUP_COUNT, NANOSEC_PER_SEC/100) },
	  .l2_ext_read_nosnp = {
		SCALE_INTERVAL(L2_EXT_READ_NOSNP_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(L2_EXT_READ_NOSNP_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(L2_EXT_READ_NOSNP_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(L2_EXT_READ_NOSNP_COUNT, NANOSEC_PER_SEC/100) },
	  .l2_rd_msg_in_stall = {
		SCALE_INTERVAL(L2_RD_MSG_IN_STALL_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(L2_RD_MSG_IN_STALL_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(L2_RD_MSG_IN_STALL_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(L2_RD_MSG_IN_STALL_COUNT, NANOSEC_PER_SEC/100) },
	  .l2_ext_write = {
		SCALE_INTERVAL(L2_EXT_WRITE_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(L2_EXT_WRITE_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(L2_EXT_WRITE_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(L2_EXT_WRITE_COUNT, NANOSEC_PER_SEC/100) },
	  .l2_ext_write_nosnp_full = {
		SCALE_INTERVAL(L2_EXT_WRITE_NOSNP_FULL_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(L2_EXT_WRITE_NOSNP_FULL_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(L2_EXT_WRITE_NOSNP_FULL_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(L2_EXT_WRITE_NOSNP_FULL_COUNT, NANOSEC_PER_SEC/100) },
	  .ls_mem_read_short = {
		SCALE_INTERVAL(LS_MEM_READ_SHORT_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(LS_MEM_READ_SHORT_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(LS_MEM_READ_SHORT_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(LS_MEM_READ_SHORT_COUNT, NANOSEC_PER_SEC/100) },
	  .frag_starving = {
		SCALE_INTERVAL(FRAG_STARVING_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(FRAG_STARVING_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(FRAG_STARVING_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(FRAG_STARVING_COUNT, NANOSEC_PER_SEC/100) },
	  .frag_partial_quads_rast = {
		SCALE_INTERVAL(FRAG_PARTIAL_QUADS_RAST_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(FRAG_PARTIAL_QUADS_RAST_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(FRAG_PARTIAL_QUADS_RAST_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(FRAG_PARTIAL_QUADS_RAST_COUNT, NANOSEC_PER_SEC/100) },
	  .frag_quads_ezs_update = {
		SCALE_INTERVAL(FRAG_QUADS_EZS_UPDATE_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(FRAG_QUADS_EZS_UPDATE_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(FRAG_QUADS_EZS_UPDATE_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(FRAG_QUADS_EZS_UPDATE_COUNT, NANOSEC_PER_SEC/100) },
	  .full_quad_warps = {
		SCALE_INTERVAL(FULL_QUAD_WARPS_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(FULL_QUAD_WARPS_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(FULL_QUAD_WARPS_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(FULL_QUAD_WARPS_COUNT, NANOSEC_PER_SEC/100) },
	  .ls_mem_write_short = {
		SCALE_INTERVAL(LS_MEM_WRITE_SHORT_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(LS_MEM_WRITE_SHORT_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(LS_MEM_WRITE_SHORT_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(LS_MEM_WRITE_SHORT_COUNT, NANOSEC_PER_SEC/100) },
	  .vary_slot_16 = {
		SCALE_INTERVAL(VARY_SLOT_16_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(VARY_SLOT_16_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(VARY_SLOT_16_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(VARY_SLOT_16_COUNT, NANOSEC_PER_SEC/100) },
	  .beats_rd_lsc_ext = {
		SCALE_INTERVAL(BEATS_RD_LSC_EXT_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(BEATS_RD_LSC_EXT_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(BEATS_RD_LSC_EXT_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(BEATS_RD_LSC_EXT_COUNT, NANOSEC_PER_SEC/100) },
	  .beats_rd_tex = {
		SCALE_INTERVAL(BEATS_RD_TEX_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(BEATS_RD_TEX_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(BEATS_RD_TEX_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(BEATS_RD_TEX_COUNT, NANOSEC_PER_SEC/100) },
	  .beats_rd_tex_ext = {
		SCALE_INTERVAL(BEATS_RD_TEX_EXT_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(BEATS_RD_TEX_EXT_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(BEATS_RD_TEX_EXT_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(BEATS_RD_TEX_EXT_COUNT, NANOSEC_PER_SEC/100) },
	  .frag_quads_coarse = {
		SCALE_INTERVAL(FRAG_QUADS_COARSE_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(FRAG_QUADS_COARSE_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(FRAG_QUADS_COARSE_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(FRAG_QUADS_COARSE_COUNT, NANOSEC_PER_SEC/100) },
	  .rt_rays_started = {
		SCALE_INTERVAL(RT_RAYS_STARTED_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(RT_RAYS_STARTED_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(RT_RAYS_STARTED_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(RT_RAYS_STARTED_COUNT, NANOSEC_PER_SEC/100) },
	  .tex_cfch_num_l1_ct_operations = {
		SCALE_INTERVAL(TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, NANOSEC_PER_SEC/100) },
	  .exec_instr_slot1 = {
		SCALE_INTERVAL(EXEC_INSTR_SLOT1_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(EXEC_INSTR_SLOT1_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(EXEC_INSTR_SLOT1_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(EXEC_INSTR_SLOT1_COUNT, NANOSEC_PER_SEC/100) },
	  .exec_issue_slot_any = {
		SCALE_INTERVAL(EXEC_ISSUE_SLOT_ANY_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(EXEC_ISSUE_SLOT_ANY_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(EXEC_ISSUE_SLOT_ANY_COUNT, NANOSEC_PER_SEC/100),
		SCALE_INTERVAL(EXEC_ISSUE_SLOT_ANY_COUNT, NANOSEC_PER_SEC/100) },
	  .interval_ns = NANOSEC_PER_SEC/100,
	  .scale = SCALE,
	  .temp = TEMP,
	  .util = 0.34,
	},

	{ .freq = FREQ_HZ,
	  .voltage = VOLTAGE_MV,
	  .num_l2_slices = 1,
	  .l2_access = { SCALE_INTERVAL(L2_COUNT, NANOSEC_PER_SEC*1.5), },
	  .num_cores = 4,
	  .exec_instr_count = {
		SCALE_INTERVAL(INSTR_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(INSTR_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(INSTR_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(INSTR_COUNT, NANOSEC_PER_SEC*1.5) },
	  .tex_issue = {
		SCALE_INTERVAL(TEX_ISSUE_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(TEX_ISSUE_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(TEX_ISSUE_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(TEX_ISSUE_COUNT, NANOSEC_PER_SEC*1.5) },
	  .tile_wb = {
		SCALE_INTERVAL(TILE_WB_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(TILE_WB_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(TILE_WB_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(TILE_WB_COUNT, NANOSEC_PER_SEC*1.5) },
	  .exec_instr_msg = {
		SCALE_INTERVAL(EXEC_INSTR_MSG_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(EXEC_INSTR_MSG_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(EXEC_INSTR_MSG_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(EXEC_INSTR_MSG_COUNT, NANOSEC_PER_SEC*1.5) },
	  .exec_instr_fma = {
		SCALE_INTERVAL(EXEC_INSTR_FMA_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(EXEC_INSTR_FMA_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(EXEC_INSTR_FMA_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(EXEC_INSTR_FMA_COUNT, NANOSEC_PER_SEC*1.5) },
	  .exec_instr_cvt = {
		SCALE_INTERVAL(EXEC_INSTR_CVT_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(EXEC_INSTR_CVT_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(EXEC_INSTR_CVT_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(EXEC_INSTR_CVT_COUNT, NANOSEC_PER_SEC*1.5) },
	  .exec_instr_sfu = {
		SCALE_INTERVAL(EXEC_INSTR_SFU_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(EXEC_INSTR_SFU_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(EXEC_INSTR_SFU_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(EXEC_INSTR_SFU_COUNT, NANOSEC_PER_SEC*1.5) },
	  .exec_starve_arith = {
		SCALE_INTERVAL(EXEC_STARVE_ARITH_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(EXEC_STARVE_ARITH_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(EXEC_STARVE_ARITH_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(EXEC_STARVE_ARITH_COUNT, NANOSEC_PER_SEC*1.5) },
	  .tex_tfch_clk_stalled = {
		SCALE_INTERVAL(TEX_TFCH_CLK_STALLED_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(TEX_TFCH_CLK_STALLED_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(TEX_TFCH_CLK_STALLED_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(TEX_TFCH_CLK_STALLED_COUNT, NANOSEC_PER_SEC*1.5) },
	  .tex_filt_num_operations = {
		SCALE_INTERVAL(TEX_FILT_NUM_OPERATIONS_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(TEX_FILT_NUM_OPERATIONS_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(TEX_FILT_NUM_OPERATIONS_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(TEX_FILT_NUM_OPERATIONS_COUNT, NANOSEC_PER_SEC*1.5) },
	  .gpu_active = SCALE_INTERVAL(ACTIVE_COUNT, NANOSEC_PER_SEC*1.5),
	  .idvs_pos_shad_stall = SCALE_INTERVAL(IDVS_POS_SHAD_STALL_COUNT, NANOSEC_PER_SEC*1.5),
	  .prefetch_stall = SCALE_INTERVAL(PREFETCH_STALL_COUNT, NANOSEC_PER_SEC*1.5),
	  .vfetch_pos_read_wait = SCALE_INTERVAL(VFETCH_POS_READ_WAIT_COUNT, NANOSEC_PER_SEC*1.5),
	  .vfetch_vertex_wait = SCALE_INTERVAL(VFETCH_VERTEX_WAIT_COUNT, NANOSEC_PER_SEC*1.5),
	  .primassy_stall = SCALE_INTERVAL(PRIMASSY_STALL_COUNT, NANOSEC_PER_SEC*1.5),
	  .idvs_var_shad_stall = SCALE_INTERVAL(IDVS_VAR_SHAD_STALL_COUNT, NANOSEC_PER_SEC*1.5),
	  .iter_stall = SCALE_INTERVAL(ITER_STALL_COUNT, NANOSEC_PER_SEC*1.5),
	  .pmgr_ptr_rd_stall = SCALE_INTERVAL(PMGR_PTR_RD_STALL_COUNT, NANOSEC_PER_SEC*1.5),
	  .primassy_pos_shader_wait = SCALE_INTERVAL(PRIMASSY_POS_SHADER_WAIT_COUNT, NANOSEC_PER_SEC*1.5),
	  .l2_rd_msg_in_cu = {
		SCALE_INTERVAL(L2_RD_MSG_IN_CU_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(L2_RD_MSG_IN_CU_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(L2_RD_MSG_IN_CU_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(L2_RD_MSG_IN_CU_COUNT, NANOSEC_PER_SEC*1.5) },
	  .l2_rd_msg_in = {
		SCALE_INTERVAL(L2_RD_MSG_IN_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(L2_RD_MSG_IN_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(L2_RD_MSG_IN_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(L2_RD_MSG_IN_COUNT, NANOSEC_PER_SEC*1.5) },
	  .l2_wr_msg_in = {
		SCALE_INTERVAL(L2_WR_MSG_IN_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(L2_WR_MSG_IN_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(L2_WR_MSG_IN_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(L2_WR_MSG_IN_COUNT, NANOSEC_PER_SEC*1.5) },
	  .l2_snp_msg_in = {
		SCALE_INTERVAL(L2_SNP_MSG_IN_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(L2_SNP_MSG_IN_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(L2_SNP_MSG_IN_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(L2_SNP_MSG_IN_COUNT, NANOSEC_PER_SEC*1.5) },
	  .l2_rd_msg_out = {
		SCALE_INTERVAL(L2_RD_MSG_OUT_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(L2_RD_MSG_OUT_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(L2_RD_MSG_OUT_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(L2_RD_MSG_OUT_COUNT, NANOSEC_PER_SEC*1.5) },
	  .l2_read_lookup = {
		SCALE_INTERVAL(L2_READ_LOOKUP_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(L2_READ_LOOKUP_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(L2_READ_LOOKUP_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(L2_READ_LOOKUP_COUNT, NANOSEC_PER_SEC*1.5) },
	  .l2_ext_read_nosnp = {
		SCALE_INTERVAL(L2_EXT_READ_NOSNP_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(L2_EXT_READ_NOSNP_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(L2_EXT_READ_NOSNP_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(L2_EXT_READ_NOSNP_COUNT, NANOSEC_PER_SEC*1.5) },
	  .l2_rd_msg_in_stall = {
		SCALE_INTERVAL(L2_RD_MSG_IN_STALL_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(L2_RD_MSG_IN_STALL_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(L2_RD_MSG_IN_STALL_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(L2_RD_MSG_IN_STALL_COUNT, NANOSEC_PER_SEC*1.5) },
	  .l2_ext_write = {
		SCALE_INTERVAL(L2_EXT_WRITE_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(L2_EXT_WRITE_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(L2_EXT_WRITE_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(L2_EXT_WRITE_COUNT, NANOSEC_PER_SEC*1.5) },
	  .l2_ext_write_nosnp_full = {
		SCALE_INTERVAL(L2_EXT_WRITE_NOSNP_FULL_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(L2_EXT_WRITE_NOSNP_FULL_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(L2_EXT_WRITE_NOSNP_FULL_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(L2_EXT_WRITE_NOSNP_FULL_COUNT, NANOSEC_PER_SEC*1.5) },
	  .ls_mem_read_short = {
		SCALE_INTERVAL(LS_MEM_READ_SHORT_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(LS_MEM_READ_SHORT_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(LS_MEM_READ_SHORT_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(LS_MEM_READ_SHORT_COUNT, NANOSEC_PER_SEC*1.5) },
	  .frag_starving = {
		SCALE_INTERVAL(FRAG_STARVING_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(FRAG_STARVING_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(FRAG_STARVING_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(FRAG_STARVING_COUNT, NANOSEC_PER_SEC*1.5) },
	  .frag_partial_quads_rast = {
		SCALE_INTERVAL(FRAG_PARTIAL_QUADS_RAST_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(FRAG_PARTIAL_QUADS_RAST_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(FRAG_PARTIAL_QUADS_RAST_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(FRAG_PARTIAL_QUADS_RAST_COUNT, NANOSEC_PER_SEC*1.5) },
	  .frag_quads_ezs_update = {
		SCALE_INTERVAL(FRAG_QUADS_EZS_UPDATE_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(FRAG_QUADS_EZS_UPDATE_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(FRAG_QUADS_EZS_UPDATE_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(FRAG_QUADS_EZS_UPDATE_COUNT, NANOSEC_PER_SEC*1.5) },
	  .full_quad_warps = {
		SCALE_INTERVAL(FULL_QUAD_WARPS_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(FULL_QUAD_WARPS_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(FULL_QUAD_WARPS_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(FULL_QUAD_WARPS_COUNT, NANOSEC_PER_SEC*1.5) },
	  .ls_mem_write_short = {
		SCALE_INTERVAL(LS_MEM_WRITE_SHORT_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(LS_MEM_WRITE_SHORT_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(LS_MEM_WRITE_SHORT_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(LS_MEM_WRITE_SHORT_COUNT, NANOSEC_PER_SEC*1.5) },
	  .vary_slot_16 = {
		SCALE_INTERVAL(VARY_SLOT_16_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(VARY_SLOT_16_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(VARY_SLOT_16_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(VARY_SLOT_16_COUNT, NANOSEC_PER_SEC*1.5) },
	  .beats_rd_lsc_ext = {
		SCALE_INTERVAL(BEATS_RD_LSC_EXT_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(BEATS_RD_LSC_EXT_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(BEATS_RD_LSC_EXT_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(BEATS_RD_LSC_EXT_COUNT, NANOSEC_PER_SEC*1.5) },
	  .beats_rd_tex = {
		SCALE_INTERVAL(BEATS_RD_TEX_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(BEATS_RD_TEX_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(BEATS_RD_TEX_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(BEATS_RD_TEX_COUNT, NANOSEC_PER_SEC*1.5) },
	  .beats_rd_tex_ext = {
		SCALE_INTERVAL(BEATS_RD_TEX_EXT_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(BEATS_RD_TEX_EXT_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(BEATS_RD_TEX_EXT_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(BEATS_RD_TEX_EXT_COUNT, NANOSEC_PER_SEC*1.5) },
	  .frag_quads_coarse = {
		SCALE_INTERVAL(FRAG_QUADS_COARSE_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(FRAG_QUADS_COARSE_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(FRAG_QUADS_COARSE_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(FRAG_QUADS_COARSE_COUNT, NANOSEC_PER_SEC*1.5) },
	  .rt_rays_started = {
		SCALE_INTERVAL(RT_RAYS_STARTED_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(RT_RAYS_STARTED_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(RT_RAYS_STARTED_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(RT_RAYS_STARTED_COUNT, NANOSEC_PER_SEC*1.5) },
	  .tex_cfch_num_l1_ct_operations = {
		SCALE_INTERVAL(TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, NANOSEC_PER_SEC*1.5) },
	  .exec_instr_slot1 = {
		SCALE_INTERVAL(EXEC_INSTR_SLOT1_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(EXEC_INSTR_SLOT1_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(EXEC_INSTR_SLOT1_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(EXEC_INSTR_SLOT1_COUNT, NANOSEC_PER_SEC*1.5) },
	  .exec_issue_slot_any = {
		SCALE_INTERVAL(EXEC_ISSUE_SLOT_ANY_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(EXEC_ISSUE_SLOT_ANY_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(EXEC_ISSUE_SLOT_ANY_COUNT, NANOSEC_PER_SEC*1.5),
		SCALE_INTERVAL(EXEC_ISSUE_SLOT_ANY_COUNT, NANOSEC_PER_SEC*1.5) },
	  .interval_ns = NANOSEC_PER_SEC*1.5,
	  .scale = SCALE,
	  .temp = TEMP,
	  .util = 0.55,
	},

	/* One Operating Performance Point with small/large scales */
	{ .freq = FREQ_HZ,
	  .voltage = VOLTAGE_MV,
	  .num_l2_slices = 1,
	  .l2_access = { L2_COUNT, },
	  .num_cores = 4,
	  .exec_instr_count = {
		INSTR_COUNT, INSTR_COUNT, INSTR_COUNT, INSTR_COUNT },
	  .tex_issue = {
		TEX_ISSUE_COUNT, TEX_ISSUE_COUNT,
		TEX_ISSUE_COUNT, TEX_ISSUE_COUNT },
	  .tile_wb = {
		TILE_WB_COUNT, TILE_WB_COUNT, TILE_WB_COUNT, TILE_WB_COUNT },
	  .exec_instr_msg = {
		EXEC_INSTR_MSG_COUNT, EXEC_INSTR_MSG_COUNT,
		EXEC_INSTR_MSG_COUNT, EXEC_INSTR_MSG_COUNT },
	  .exec_instr_fma = {
		EXEC_INSTR_FMA_COUNT, EXEC_INSTR_FMA_COUNT,
		EXEC_INSTR_FMA_COUNT, EXEC_INSTR_FMA_COUNT },
	  .exec_instr_cvt = {
		EXEC_INSTR_CVT_COUNT, EXEC_INSTR_CVT_COUNT,
		EXEC_INSTR_CVT_COUNT, EXEC_INSTR_CVT_COUNT },
	  .exec_instr_sfu = {
		EXEC_INSTR_SFU_COUNT, EXEC_INSTR_SFU_COUNT,
		EXEC_INSTR_SFU_COUNT, EXEC_INSTR_SFU_COUNT },
	  .exec_starve_arith = {
		EXEC_STARVE_ARITH_COUNT, EXEC_STARVE_ARITH_COUNT,
		EXEC_STARVE_ARITH_COUNT, EXEC_STARVE_ARITH_COUNT },
	  .tex_tfch_clk_stalled = {
		TEX_TFCH_CLK_STALLED_COUNT, TEX_TFCH_CLK_STALLED_COUNT,
		TEX_TFCH_CLK_STALLED_COUNT, TEX_TFCH_CLK_STALLED_COUNT },
	  .tex_filt_num_operations = {
		TEX_FILT_NUM_OPERATIONS_COUNT, TEX_FILT_NUM_OPERATIONS_COUNT,
		TEX_FILT_NUM_OPERATIONS_COUNT, TEX_FILT_NUM_OPERATIONS_COUNT },
	  .gpu_active = ACTIVE_COUNT,
	  .idvs_pos_shad_stall = IDVS_POS_SHAD_STALL_COUNT,
	  .prefetch_stall = PREFETCH_STALL_COUNT,
	  .vfetch_pos_read_wait = VFETCH_POS_READ_WAIT_COUNT,
	  .vfetch_vertex_wait = VFETCH_VERTEX_WAIT_COUNT,
	  .primassy_stall = PRIMASSY_STALL_COUNT,
	  .idvs_var_shad_stall = IDVS_VAR_SHAD_STALL_COUNT,
	  .iter_stall = ITER_STALL_COUNT,
	  .pmgr_ptr_rd_stall = PMGR_PTR_RD_STALL_COUNT,
	  .primassy_pos_shader_wait = PRIMASSY_POS_SHADER_WAIT_COUNT,
	  .l2_rd_msg_in_cu = {
		L2_RD_MSG_IN_CU_COUNT, L2_RD_MSG_IN_CU_COUNT,
		L2_RD_MSG_IN_CU_COUNT, L2_RD_MSG_IN_CU_COUNT },
	  .l2_rd_msg_in = {
		L2_RD_MSG_IN_COUNT, L2_RD_MSG_IN_COUNT,
		L2_RD_MSG_IN_COUNT, L2_RD_MSG_IN_COUNT },
	  .l2_wr_msg_in = {
		L2_WR_MSG_IN_COUNT, L2_WR_MSG_IN_COUNT,
		L2_WR_MSG_IN_COUNT, L2_WR_MSG_IN_COUNT },
	  .l2_snp_msg_in = {
		L2_SNP_MSG_IN_COUNT, L2_SNP_MSG_IN_COUNT,
		L2_SNP_MSG_IN_COUNT, L2_SNP_MSG_IN_COUNT },
	  .l2_rd_msg_out = {
		L2_RD_MSG_OUT_COUNT, L2_RD_MSG_OUT_COUNT,
		L2_RD_MSG_OUT_COUNT, L2_RD_MSG_OUT_COUNT },
	  .l2_read_lookup = {
		L2_READ_LOOKUP_COUNT, L2_READ_LOOKUP_COUNT,
		L2_READ_LOOKUP_COUNT, L2_READ_LOOKUP_COUNT },
	  .l2_ext_read_nosnp = {
		L2_EXT_READ_NOSNP_COUNT, L2_EXT_READ_NOSNP_COUNT,
		L2_EXT_READ_NOSNP_COUNT, L2_EXT_READ_NOSNP_COUNT },
	  .l2_rd_msg_in_stall = {
		L2_RD_MSG_IN_STALL_COUNT, L2_RD_MSG_IN_STALL_COUNT,
		L2_RD_MSG_IN_STALL_COUNT, L2_RD_MSG_IN_STALL_COUNT },
	  .l2_ext_write = {
		L2_EXT_WRITE_COUNT, L2_EXT_WRITE_COUNT,
		L2_EXT_WRITE_COUNT, L2_EXT_WRITE_COUNT },
	  .l2_ext_write_nosnp_full = {
		L2_EXT_WRITE_NOSNP_FULL_COUNT, L2_EXT_WRITE_NOSNP_FULL_COUNT,
		L2_EXT_WRITE_NOSNP_FULL_COUNT, L2_EXT_WRITE_NOSNP_FULL_COUNT },
	  .ls_mem_read_short = {
		LS_MEM_READ_SHORT_COUNT, LS_MEM_READ_SHORT_COUNT,
		LS_MEM_READ_SHORT_COUNT, LS_MEM_READ_SHORT_COUNT },
	  .frag_starving = {
		FRAG_STARVING_COUNT, FRAG_STARVING_COUNT,
		FRAG_STARVING_COUNT, FRAG_STARVING_COUNT },
	  .frag_partial_quads_rast = {
		FRAG_PARTIAL_QUADS_RAST_COUNT, FRAG_PARTIAL_QUADS_RAST_COUNT,
		FRAG_PARTIAL_QUADS_RAST_COUNT, FRAG_PARTIAL_QUADS_RAST_COUNT },
	  .frag_quads_ezs_update = {
		FRAG_QUADS_EZS_UPDATE_COUNT, FRAG_QUADS_EZS_UPDATE_COUNT,
		FRAG_QUADS_EZS_UPDATE_COUNT, FRAG_QUADS_EZS_UPDATE_COUNT },
	  .full_quad_warps = {
		FULL_QUAD_WARPS_COUNT, FULL_QUAD_WARPS_COUNT,
		FULL_QUAD_WARPS_COUNT, FULL_QUAD_WARPS_COUNT },
	  .ls_mem_write_short = {
		LS_MEM_WRITE_SHORT_COUNT, LS_MEM_WRITE_SHORT_COUNT,
		LS_MEM_WRITE_SHORT_COUNT, LS_MEM_WRITE_SHORT_COUNT },
	  .vary_slot_16 = {
		VARY_SLOT_16_COUNT, VARY_SLOT_16_COUNT,
		VARY_SLOT_16_COUNT, VARY_SLOT_16_COUNT },
	  .beats_rd_lsc_ext = {
		BEATS_RD_LSC_EXT_COUNT, BEATS_RD_LSC_EXT_COUNT,
		BEATS_RD_LSC_EXT_COUNT, BEATS_RD_LSC_EXT_COUNT },
	  .beats_rd_tex = {
		BEATS_RD_TEX_COUNT, BEATS_RD_TEX_COUNT,
		BEATS_RD_TEX_COUNT, BEATS_RD_TEX_COUNT },
	  .beats_rd_tex_ext = {
		BEATS_RD_TEX_EXT_COUNT, BEATS_RD_TEX_EXT_COUNT,
		BEATS_RD_TEX_EXT_COUNT, BEATS_RD_TEX_EXT_COUNT },
	  .frag_quads_coarse = {
		FRAG_QUADS_COARSE_COUNT, FRAG_QUADS_COARSE_COUNT,
		FRAG_QUADS_COARSE_COUNT, FRAG_QUADS_COARSE_COUNT },
	  .rt_rays_started = {
		RT_RAYS_STARTED_COUNT, RT_RAYS_STARTED_COUNT,
		RT_RAYS_STARTED_COUNT, RT_RAYS_STARTED_COUNT },
	  .tex_cfch_num_l1_ct_operations = {
		TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT,
		TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT },
	  .exec_instr_slot1 = {
		EXEC_INSTR_SLOT1_COUNT, EXEC_INSTR_SLOT1_COUNT,
		EXEC_INSTR_SLOT1_COUNT, EXEC_INSTR_SLOT1_COUNT },
	  .exec_issue_slot_any = {
		EXEC_ISSUE_SLOT_ANY_COUNT, EXEC_ISSUE_SLOT_ANY_COUNT,
		EXEC_ISSUE_SLOT_ANY_COUNT, EXEC_ISSUE_SLOT_ANY_COUNT },
	  .interval_ns = INTERVAL_NS,
	  .scale = SCALE * 0.1,
	  .temp = TEMP,
	  .util = 0.89,
	},

	{ .freq = FREQ_HZ,
	  .voltage = VOLTAGE_MV,
	  .num_l2_slices = 1,
	  .l2_access = { L2_COUNT, },
	  .num_cores = 4,
	  .exec_instr_count = {
		INSTR_COUNT, INSTR_COUNT, INSTR_COUNT, INSTR_COUNT },
	  .tex_issue = {
		TEX_ISSUE_COUNT, TEX_ISSUE_COUNT,
		TEX_ISSUE_COUNT, TEX_ISSUE_COUNT },
	  .tile_wb = {
		TILE_WB_COUNT, TILE_WB_COUNT, TILE_WB_COUNT, TILE_WB_COUNT },
	  .exec_instr_msg = {
		EXEC_INSTR_MSG_COUNT, EXEC_INSTR_MSG_COUNT,
		EXEC_INSTR_MSG_COUNT, EXEC_INSTR_MSG_COUNT },
	  .exec_instr_fma = {
		EXEC_INSTR_FMA_COUNT, EXEC_INSTR_FMA_COUNT,
		EXEC_INSTR_FMA_COUNT, EXEC_INSTR_FMA_COUNT },
	  .exec_instr_cvt = {
		EXEC_INSTR_CVT_COUNT, EXEC_INSTR_CVT_COUNT,
		EXEC_INSTR_CVT_COUNT, EXEC_INSTR_CVT_COUNT },
	  .exec_instr_sfu = {
		EXEC_INSTR_SFU_COUNT, EXEC_INSTR_SFU_COUNT,
		EXEC_INSTR_SFU_COUNT, EXEC_INSTR_SFU_COUNT },
	  .exec_starve_arith = {
		EXEC_STARVE_ARITH_COUNT, EXEC_STARVE_ARITH_COUNT,
		EXEC_STARVE_ARITH_COUNT, EXEC_STARVE_ARITH_COUNT },
	  .tex_tfch_clk_stalled = {
		TEX_TFCH_CLK_STALLED_COUNT, TEX_TFCH_CLK_STALLED_COUNT,
		TEX_TFCH_CLK_STALLED_COUNT, TEX_TFCH_CLK_STALLED_COUNT },
	  .tex_filt_num_operations = {
		TEX_FILT_NUM_OPERATIONS_COUNT, TEX_FILT_NUM_OPERATIONS_COUNT,
		TEX_FILT_NUM_OPERATIONS_COUNT, TEX_FILT_NUM_OPERATIONS_COUNT },
	  .gpu_active = ACTIVE_COUNT,
	  .idvs_pos_shad_stall = IDVS_POS_SHAD_STALL_COUNT,
	  .prefetch_stall = PREFETCH_STALL_COUNT,
	  .vfetch_pos_read_wait = VFETCH_POS_READ_WAIT_COUNT,
	  .vfetch_vertex_wait = VFETCH_VERTEX_WAIT_COUNT,
	  .primassy_stall = PRIMASSY_STALL_COUNT,
	  .idvs_var_shad_stall = IDVS_VAR_SHAD_STALL_COUNT,
	  .iter_stall = ITER_STALL_COUNT,
	  .pmgr_ptr_rd_stall = PMGR_PTR_RD_STALL_COUNT,
	  .primassy_pos_shader_wait = PRIMASSY_POS_SHADER_WAIT_COUNT,
	  .l2_rd_msg_in_cu = {
		L2_RD_MSG_IN_CU_COUNT, L2_RD_MSG_IN_CU_COUNT,
		L2_RD_MSG_IN_CU_COUNT, L2_RD_MSG_IN_CU_COUNT },
	  .l2_rd_msg_in = {
		L2_RD_MSG_IN_COUNT, L2_RD_MSG_IN_COUNT,
		L2_RD_MSG_IN_COUNT, L2_RD_MSG_IN_COUNT },
	  .l2_wr_msg_in = {
		L2_WR_MSG_IN_COUNT, L2_WR_MSG_IN_COUNT,
		L2_WR_MSG_IN_COUNT, L2_WR_MSG_IN_COUNT },
	  .l2_snp_msg_in = {
		L2_SNP_MSG_IN_COUNT, L2_SNP_MSG_IN_COUNT,
		L2_SNP_MSG_IN_COUNT, L2_SNP_MSG_IN_COUNT },
	  .l2_rd_msg_out = {
		L2_RD_MSG_OUT_COUNT, L2_RD_MSG_OUT_COUNT,
		L2_RD_MSG_OUT_COUNT, L2_RD_MSG_OUT_COUNT },
	  .l2_read_lookup = {
		L2_READ_LOOKUP_COUNT, L2_READ_LOOKUP_COUNT,
		L2_READ_LOOKUP_COUNT, L2_READ_LOOKUP_COUNT },
	  .l2_ext_read_nosnp = {
		L2_EXT_READ_NOSNP_COUNT, L2_EXT_READ_NOSNP_COUNT,
		L2_EXT_READ_NOSNP_COUNT, L2_EXT_READ_NOSNP_COUNT },
	  .l2_rd_msg_in_stall = {
		L2_RD_MSG_IN_STALL_COUNT, L2_RD_MSG_IN_STALL_COUNT,
		L2_RD_MSG_IN_STALL_COUNT, L2_RD_MSG_IN_STALL_COUNT },
	  .l2_ext_write = {
		L2_EXT_WRITE_COUNT, L2_EXT_WRITE_COUNT,
		L2_EXT_WRITE_COUNT, L2_EXT_WRITE_COUNT },
	  .l2_ext_write_nosnp_full = {
		L2_EXT_WRITE_NOSNP_FULL_COUNT, L2_EXT_WRITE_NOSNP_FULL_COUNT,
		L2_EXT_WRITE_NOSNP_FULL_COUNT, L2_EXT_WRITE_NOSNP_FULL_COUNT },
	  .ls_mem_read_short = {
		LS_MEM_READ_SHORT_COUNT, LS_MEM_READ_SHORT_COUNT,
		LS_MEM_READ_SHORT_COUNT, LS_MEM_READ_SHORT_COUNT },
	  .frag_starving = {
		FRAG_STARVING_COUNT, FRAG_STARVING_COUNT,
		FRAG_STARVING_COUNT, FRAG_STARVING_COUNT },
	  .frag_partial_quads_rast = {
		FRAG_PARTIAL_QUADS_RAST_COUNT, FRAG_PARTIAL_QUADS_RAST_COUNT,
		FRAG_PARTIAL_QUADS_RAST_COUNT, FRAG_PARTIAL_QUADS_RAST_COUNT },
	  .frag_quads_ezs_update = {
		FRAG_QUADS_EZS_UPDATE_COUNT, FRAG_QUADS_EZS_UPDATE_COUNT,
		FRAG_QUADS_EZS_UPDATE_COUNT, FRAG_QUADS_EZS_UPDATE_COUNT },
	  .full_quad_warps = {
		FULL_QUAD_WARPS_COUNT, FULL_QUAD_WARPS_COUNT,
		FULL_QUAD_WARPS_COUNT, FULL_QUAD_WARPS_COUNT },
	  .ls_mem_write_short = {
		LS_MEM_WRITE_SHORT_COUNT, LS_MEM_WRITE_SHORT_COUNT,
		LS_MEM_WRITE_SHORT_COUNT, LS_MEM_WRITE_SHORT_COUNT },
	  .vary_slot_16 = {
		VARY_SLOT_16_COUNT, VARY_SLOT_16_COUNT,
		VARY_SLOT_16_COUNT, VARY_SLOT_16_COUNT },
	  .beats_rd_lsc_ext = {
		BEATS_RD_LSC_EXT_COUNT, BEATS_RD_LSC_EXT_COUNT,
		BEATS_RD_LSC_EXT_COUNT, BEATS_RD_LSC_EXT_COUNT },
	  .beats_rd_tex = {
		BEATS_RD_TEX_COUNT, BEATS_RD_TEX_COUNT,
		BEATS_RD_TEX_COUNT, BEATS_RD_TEX_COUNT },
	  .beats_rd_tex_ext = {
		BEATS_RD_TEX_EXT_COUNT, BEATS_RD_TEX_EXT_COUNT,
		BEATS_RD_TEX_EXT_COUNT, BEATS_RD_TEX_EXT_COUNT },
	  .frag_quads_coarse = {
		FRAG_QUADS_COARSE_COUNT, FRAG_QUADS_COARSE_COUNT,
		FRAG_QUADS_COARSE_COUNT, FRAG_QUADS_COARSE_COUNT },
	  .rt_rays_started = {
		RT_RAYS_STARTED_COUNT, RT_RAYS_STARTED_COUNT,
		RT_RAYS_STARTED_COUNT, RT_RAYS_STARTED_COUNT },
	  .tex_cfch_num_l1_ct_operations = {
		TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT,
		TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT },
	  .exec_instr_slot1 = {
		EXEC_INSTR_SLOT1_COUNT, EXEC_INSTR_SLOT1_COUNT,
		EXEC_INSTR_SLOT1_COUNT, EXEC_INSTR_SLOT1_COUNT },
	  .exec_issue_slot_any = {
		EXEC_ISSUE_SLOT_ANY_COUNT, EXEC_ISSUE_SLOT_ANY_COUNT,
		EXEC_ISSUE_SLOT_ANY_COUNT, EXEC_ISSUE_SLOT_ANY_COUNT },
	  .interval_ns = INTERVAL_NS,
	  .scale = SCALE * 0.3,
	  .temp = TEMP,
	  .util = 1.0,
	},

	{ .freq = FREQ_HZ,
	  .voltage = VOLTAGE_MV,
	  .num_l2_slices = 1,
	  .l2_access = { L2_COUNT, },
	  .num_cores = 4,
	  .exec_instr_count = {
		INSTR_COUNT, INSTR_COUNT, INSTR_COUNT, INSTR_COUNT },
	  .tex_issue = {
		TEX_ISSUE_COUNT, TEX_ISSUE_COUNT,
		TEX_ISSUE_COUNT, TEX_ISSUE_COUNT },
	  .tile_wb = {
		TILE_WB_COUNT, TILE_WB_COUNT, TILE_WB_COUNT, TILE_WB_COUNT },
	  .exec_instr_msg = {
		EXEC_INSTR_MSG_COUNT, EXEC_INSTR_MSG_COUNT,
		EXEC_INSTR_MSG_COUNT, EXEC_INSTR_MSG_COUNT },
	  .exec_instr_fma = {
		EXEC_INSTR_FMA_COUNT, EXEC_INSTR_FMA_COUNT,
		EXEC_INSTR_FMA_COUNT, EXEC_INSTR_FMA_COUNT },
	  .exec_instr_cvt = {
		EXEC_INSTR_CVT_COUNT, EXEC_INSTR_CVT_COUNT,
		EXEC_INSTR_CVT_COUNT, EXEC_INSTR_CVT_COUNT },
	  .exec_instr_sfu = {
		EXEC_INSTR_SFU_COUNT, EXEC_INSTR_SFU_COUNT,
		EXEC_INSTR_SFU_COUNT, EXEC_INSTR_SFU_COUNT },
	  .exec_starve_arith = {
		EXEC_STARVE_ARITH_COUNT, EXEC_STARVE_ARITH_COUNT,
		EXEC_STARVE_ARITH_COUNT, EXEC_STARVE_ARITH_COUNT },
	  .tex_tfch_clk_stalled = {
		TEX_TFCH_CLK_STALLED_COUNT, TEX_TFCH_CLK_STALLED_COUNT,
		TEX_TFCH_CLK_STALLED_COUNT, TEX_TFCH_CLK_STALLED_COUNT },
	  .tex_filt_num_operations = {
		TEX_FILT_NUM_OPERATIONS_COUNT, TEX_FILT_NUM_OPERATIONS_COUNT,
		TEX_FILT_NUM_OPERATIONS_COUNT, TEX_FILT_NUM_OPERATIONS_COUNT },
	  .gpu_active = ACTIVE_COUNT,
	  .idvs_pos_shad_stall = IDVS_POS_SHAD_STALL_COUNT,
	  .prefetch_stall = PREFETCH_STALL_COUNT,
	  .vfetch_pos_read_wait = VFETCH_POS_READ_WAIT_COUNT,
	  .vfetch_vertex_wait = VFETCH_VERTEX_WAIT_COUNT,
	  .primassy_stall = PRIMASSY_STALL_COUNT,
	  .idvs_var_shad_stall = IDVS_VAR_SHAD_STALL_COUNT,
	  .iter_stall = ITER_STALL_COUNT,
	  .pmgr_ptr_rd_stall = PMGR_PTR_RD_STALL_COUNT,
	  .primassy_pos_shader_wait = PRIMASSY_POS_SHADER_WAIT_COUNT,
	  .l2_rd_msg_in_cu = {
		L2_RD_MSG_IN_CU_COUNT, L2_RD_MSG_IN_CU_COUNT,
		L2_RD_MSG_IN_CU_COUNT, L2_RD_MSG_IN_CU_COUNT },
	  .l2_rd_msg_in = {
		L2_RD_MSG_IN_COUNT, L2_RD_MSG_IN_COUNT,
		L2_RD_MSG_IN_COUNT, L2_RD_MSG_IN_COUNT },
	  .l2_wr_msg_in = {
		L2_WR_MSG_IN_COUNT, L2_WR_MSG_IN_COUNT,
		L2_WR_MSG_IN_COUNT, L2_WR_MSG_IN_COUNT },
	  .l2_snp_msg_in = {
		L2_SNP_MSG_IN_COUNT, L2_SNP_MSG_IN_COUNT,
		L2_SNP_MSG_IN_COUNT, L2_SNP_MSG_IN_COUNT },
	  .l2_rd_msg_out = {
		L2_RD_MSG_OUT_COUNT, L2_RD_MSG_OUT_COUNT,
		L2_RD_MSG_OUT_COUNT, L2_RD_MSG_OUT_COUNT },
	  .l2_read_lookup = {
		L2_READ_LOOKUP_COUNT, L2_READ_LOOKUP_COUNT,
		L2_READ_LOOKUP_COUNT, L2_READ_LOOKUP_COUNT },
	  .l2_ext_read_nosnp = {
		L2_EXT_READ_NOSNP_COUNT, L2_EXT_READ_NOSNP_COUNT,
		L2_EXT_READ_NOSNP_COUNT, L2_EXT_READ_NOSNP_COUNT },
	  .l2_rd_msg_in_stall = {
		L2_RD_MSG_IN_STALL_COUNT, L2_RD_MSG_IN_STALL_COUNT,
		L2_RD_MSG_IN_STALL_COUNT, L2_RD_MSG_IN_STALL_COUNT },
	  .l2_ext_write = {
		L2_EXT_WRITE_COUNT, L2_EXT_WRITE_COUNT,
		L2_EXT_WRITE_COUNT, L2_EXT_WRITE_COUNT },
	  .l2_ext_write_nosnp_full = {
		L2_EXT_WRITE_NOSNP_FULL_COUNT, L2_EXT_WRITE_NOSNP_FULL_COUNT,
		L2_EXT_WRITE_NOSNP_FULL_COUNT, L2_EXT_WRITE_NOSNP_FULL_COUNT },
	  .ls_mem_read_short = {
		LS_MEM_READ_SHORT_COUNT, LS_MEM_READ_SHORT_COUNT,
		LS_MEM_READ_SHORT_COUNT, LS_MEM_READ_SHORT_COUNT },
	  .frag_starving = {
		FRAG_STARVING_COUNT, FRAG_STARVING_COUNT,
		FRAG_STARVING_COUNT, FRAG_STARVING_COUNT },
	  .frag_partial_quads_rast = {
		FRAG_PARTIAL_QUADS_RAST_COUNT, FRAG_PARTIAL_QUADS_RAST_COUNT,
		FRAG_PARTIAL_QUADS_RAST_COUNT, FRAG_PARTIAL_QUADS_RAST_COUNT },
	  .frag_quads_ezs_update = {
		FRAG_QUADS_EZS_UPDATE_COUNT, FRAG_QUADS_EZS_UPDATE_COUNT,
		FRAG_QUADS_EZS_UPDATE_COUNT, FRAG_QUADS_EZS_UPDATE_COUNT },
	  .full_quad_warps = {
		FULL_QUAD_WARPS_COUNT, FULL_QUAD_WARPS_COUNT,
		FULL_QUAD_WARPS_COUNT, FULL_QUAD_WARPS_COUNT },
	  .ls_mem_write_short = {
		LS_MEM_WRITE_SHORT_COUNT, LS_MEM_WRITE_SHORT_COUNT,
		LS_MEM_WRITE_SHORT_COUNT, LS_MEM_WRITE_SHORT_COUNT },
	  .vary_slot_16 = {
		VARY_SLOT_16_COUNT, VARY_SLOT_16_COUNT,
		VARY_SLOT_16_COUNT, VARY_SLOT_16_COUNT },
	  .beats_rd_lsc_ext = {
		BEATS_RD_LSC_EXT_COUNT, BEATS_RD_LSC_EXT_COUNT,
		BEATS_RD_LSC_EXT_COUNT, BEATS_RD_LSC_EXT_COUNT },
	  .beats_rd_tex = {
		BEATS_RD_TEX_COUNT, BEATS_RD_TEX_COUNT,
		BEATS_RD_TEX_COUNT, BEATS_RD_TEX_COUNT },
	  .beats_rd_tex_ext = {
		BEATS_RD_TEX_EXT_COUNT, BEATS_RD_TEX_EXT_COUNT,
		BEATS_RD_TEX_EXT_COUNT, BEATS_RD_TEX_EXT_COUNT },
	  .frag_quads_coarse = {
		FRAG_QUADS_COARSE_COUNT, FRAG_QUADS_COARSE_COUNT,
		FRAG_QUADS_COARSE_COUNT, FRAG_QUADS_COARSE_COUNT },
	  .rt_rays_started = {
		RT_RAYS_STARTED_COUNT, RT_RAYS_STARTED_COUNT,
		RT_RAYS_STARTED_COUNT, RT_RAYS_STARTED_COUNT },
	  .tex_cfch_num_l1_ct_operations = {
		TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT,
		TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT },
	  .exec_instr_slot1 = {
		EXEC_INSTR_SLOT1_COUNT, EXEC_INSTR_SLOT1_COUNT,
		EXEC_INSTR_SLOT1_COUNT, EXEC_INSTR_SLOT1_COUNT },
	  .exec_issue_slot_any = {
		EXEC_ISSUE_SLOT_ANY_COUNT, EXEC_ISSUE_SLOT_ANY_COUNT,
		EXEC_ISSUE_SLOT_ANY_COUNT, EXEC_ISSUE_SLOT_ANY_COUNT },
	  .interval_ns = INTERVAL_NS,
	  .scale = SCALE * 200,
	  .temp = TEMP,
	  .util = 0.14,
	},

	{ .freq = FREQ_HZ,
	  .voltage = VOLTAGE_MV,
	  .num_l2_slices = 1,
	  .l2_access = { L2_COUNT, },
	  .num_cores = 4,
	  .exec_instr_count = {
		INSTR_COUNT, INSTR_COUNT, INSTR_COUNT, INSTR_COUNT },
	  .tex_issue = {
		TEX_ISSUE_COUNT, TEX_ISSUE_COUNT,
		TEX_ISSUE_COUNT, TEX_ISSUE_COUNT },
	  .tile_wb = {
		TILE_WB_COUNT, TILE_WB_COUNT, TILE_WB_COUNT, TILE_WB_COUNT },
	  .exec_instr_msg = {
		EXEC_INSTR_MSG_COUNT, EXEC_INSTR_MSG_COUNT,
		EXEC_INSTR_MSG_COUNT, EXEC_INSTR_MSG_COUNT },
	  .exec_instr_fma = {
		EXEC_INSTR_FMA_COUNT, EXEC_INSTR_FMA_COUNT,
		EXEC_INSTR_FMA_COUNT, EXEC_INSTR_FMA_COUNT },
	  .exec_instr_cvt = {
		EXEC_INSTR_CVT_COUNT, EXEC_INSTR_CVT_COUNT,
		EXEC_INSTR_CVT_COUNT, EXEC_INSTR_CVT_COUNT },
	  .exec_instr_sfu = {
		EXEC_INSTR_SFU_COUNT, EXEC_INSTR_SFU_COUNT,
		EXEC_INSTR_SFU_COUNT, EXEC_INSTR_SFU_COUNT },
	  .exec_starve_arith = {
		EXEC_STARVE_ARITH_COUNT, EXEC_STARVE_ARITH_COUNT,
		EXEC_STARVE_ARITH_COUNT, EXEC_STARVE_ARITH_COUNT },
	  .tex_tfch_clk_stalled = {
		TEX_TFCH_CLK_STALLED_COUNT, TEX_TFCH_CLK_STALLED_COUNT,
		TEX_TFCH_CLK_STALLED_COUNT, TEX_TFCH_CLK_STALLED_COUNT },
	  .tex_filt_num_operations = {
		TEX_FILT_NUM_OPERATIONS_COUNT, TEX_FILT_NUM_OPERATIONS_COUNT,
		TEX_FILT_NUM_OPERATIONS_COUNT, TEX_FILT_NUM_OPERATIONS_COUNT },
	  .gpu_active = ACTIVE_COUNT,
	  .idvs_pos_shad_stall = IDVS_POS_SHAD_STALL_COUNT,
	  .prefetch_stall = PREFETCH_STALL_COUNT,
	  .vfetch_pos_read_wait = VFETCH_POS_READ_WAIT_COUNT,
	  .vfetch_vertex_wait = VFETCH_VERTEX_WAIT_COUNT,
	  .primassy_stall = PRIMASSY_STALL_COUNT,
	  .idvs_var_shad_stall = IDVS_VAR_SHAD_STALL_COUNT,
	  .iter_stall = ITER_STALL_COUNT,
	  .pmgr_ptr_rd_stall = PMGR_PTR_RD_STALL_COUNT,
	  .primassy_pos_shader_wait = PRIMASSY_POS_SHADER_WAIT_COUNT,
	  .l2_rd_msg_in_cu = {
		L2_RD_MSG_IN_CU_COUNT, L2_RD_MSG_IN_CU_COUNT,
		L2_RD_MSG_IN_CU_COUNT, L2_RD_MSG_IN_CU_COUNT },
	  .l2_rd_msg_in = {
		L2_RD_MSG_IN_COUNT, L2_RD_MSG_IN_COUNT,
		L2_RD_MSG_IN_COUNT, L2_RD_MSG_IN_COUNT },
	  .l2_wr_msg_in = {
		L2_WR_MSG_IN_COUNT, L2_WR_MSG_IN_COUNT,
		L2_WR_MSG_IN_COUNT, L2_WR_MSG_IN_COUNT },
	  .l2_snp_msg_in = {
		L2_SNP_MSG_IN_COUNT, L2_SNP_MSG_IN_COUNT,
		L2_SNP_MSG_IN_COUNT, L2_SNP_MSG_IN_COUNT },
	  .l2_rd_msg_out = {
		L2_RD_MSG_OUT_COUNT, L2_RD_MSG_OUT_COUNT,
		L2_RD_MSG_OUT_COUNT, L2_RD_MSG_OUT_COUNT },
	  .l2_read_lookup = {
		L2_READ_LOOKUP_COUNT, L2_READ_LOOKUP_COUNT,
		L2_READ_LOOKUP_COUNT, L2_READ_LOOKUP_COUNT },
	  .l2_ext_read_nosnp = {
		L2_EXT_READ_NOSNP_COUNT, L2_EXT_READ_NOSNP_COUNT,
		L2_EXT_READ_NOSNP_COUNT, L2_EXT_READ_NOSNP_COUNT },
	  .l2_rd_msg_in_stall = {
		L2_RD_MSG_IN_STALL_COUNT, L2_RD_MSG_IN_STALL_COUNT,
		L2_RD_MSG_IN_STALL_COUNT, L2_RD_MSG_IN_STALL_COUNT },
	  .l2_ext_write = {
		L2_EXT_WRITE_COUNT, L2_EXT_WRITE_COUNT,
		L2_EXT_WRITE_COUNT, L2_EXT_WRITE_COUNT },
	  .l2_ext_write_nosnp_full = {
		L2_EXT_WRITE_NOSNP_FULL_COUNT, L2_EXT_WRITE_NOSNP_FULL_COUNT,
		L2_EXT_WRITE_NOSNP_FULL_COUNT, L2_EXT_WRITE_NOSNP_FULL_COUNT },
	  .ls_mem_read_short = {
		LS_MEM_READ_SHORT_COUNT, LS_MEM_READ_SHORT_COUNT,
		LS_MEM_READ_SHORT_COUNT, LS_MEM_READ_SHORT_COUNT },
	  .frag_starving = {
		FRAG_STARVING_COUNT, FRAG_STARVING_COUNT,
		FRAG_STARVING_COUNT, FRAG_STARVING_COUNT },
	  .frag_partial_quads_rast = {
		FRAG_PARTIAL_QUADS_RAST_COUNT, FRAG_PARTIAL_QUADS_RAST_COUNT,
		FRAG_PARTIAL_QUADS_RAST_COUNT, FRAG_PARTIAL_QUADS_RAST_COUNT },
	  .frag_quads_ezs_update = {
		FRAG_QUADS_EZS_UPDATE_COUNT, FRAG_QUADS_EZS_UPDATE_COUNT,
		FRAG_QUADS_EZS_UPDATE_COUNT, FRAG_QUADS_EZS_UPDATE_COUNT },
	  .full_quad_warps = {
		FULL_QUAD_WARPS_COUNT, FULL_QUAD_WARPS_COUNT,
		FULL_QUAD_WARPS_COUNT, FULL_QUAD_WARPS_COUNT },
	  .ls_mem_write_short = {
		LS_MEM_WRITE_SHORT_COUNT, LS_MEM_WRITE_SHORT_COUNT,
		LS_MEM_WRITE_SHORT_COUNT, LS_MEM_WRITE_SHORT_COUNT },
	  .vary_slot_16 = {
		VARY_SLOT_16_COUNT, VARY_SLOT_16_COUNT,
		VARY_SLOT_16_COUNT, VARY_SLOT_16_COUNT },
	  .beats_rd_lsc_ext = {
		BEATS_RD_LSC_EXT_COUNT, BEATS_RD_LSC_EXT_COUNT,
		BEATS_RD_LSC_EXT_COUNT, BEATS_RD_LSC_EXT_COUNT },
	  .beats_rd_tex = {
		BEATS_RD_TEX_COUNT, BEATS_RD_TEX_COUNT,
		BEATS_RD_TEX_COUNT, BEATS_RD_TEX_COUNT },
	  .beats_rd_tex_ext = {
		BEATS_RD_TEX_EXT_COUNT, BEATS_RD_TEX_EXT_COUNT,
		BEATS_RD_TEX_EXT_COUNT, BEATS_RD_TEX_EXT_COUNT },
	  .frag_quads_coarse = {
		FRAG_QUADS_COARSE_COUNT, FRAG_QUADS_COARSE_COUNT,
		FRAG_QUADS_COARSE_COUNT, FRAG_QUADS_COARSE_COUNT },
	  .rt_rays_started = {
		RT_RAYS_STARTED_COUNT, RT_RAYS_STARTED_COUNT,
		RT_RAYS_STARTED_COUNT, RT_RAYS_STARTED_COUNT },
	  .tex_cfch_num_l1_ct_operations = {
		TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT,
		TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT },
	  .exec_instr_slot1 = {
		EXEC_INSTR_SLOT1_COUNT, EXEC_INSTR_SLOT1_COUNT,
		EXEC_INSTR_SLOT1_COUNT, EXEC_INSTR_SLOT1_COUNT },
	  .exec_issue_slot_any = {
		EXEC_ISSUE_SLOT_ANY_COUNT, EXEC_ISSUE_SLOT_ANY_COUNT,
		EXEC_ISSUE_SLOT_ANY_COUNT, EXEC_ISSUE_SLOT_ANY_COUNT },
	  .interval_ns = INTERVAL_NS,
	  .scale = SCALE * 3000,
	  .temp = TEMP,
	  .util = 0.23,
	},

	/* One Operating Performance Point with small/large utilizations */
	{ .freq = FREQ_HZ,
	  .voltage = VOLTAGE_MV,
	  .num_l2_slices = 1,
	  .l2_access = { L2_COUNT * 0.001, },
	  .num_cores = 4,
	  .exec_instr_count = {
		INSTR_COUNT * 0.001, INSTR_COUNT * 0.001,
		INSTR_COUNT * 0.001, INSTR_COUNT * 0.001 },
	  .tex_issue = {
		TEX_ISSUE_COUNT, TEX_ISSUE_COUNT * 0.001,
		TEX_ISSUE_COUNT * 0.001, TEX_ISSUE_COUNT * 0.001 },
	  .tile_wb = {
		TILE_WB_COUNT * 0.001, TILE_WB_COUNT * 0.001,
		TILE_WB_COUNT * 0.001, TILE_WB_COUNT * 0.001 },
	  .exec_instr_msg = {
		EXEC_INSTR_MSG_COUNT * 0.001, EXEC_INSTR_MSG_COUNT * 0.001,
		EXEC_INSTR_MSG_COUNT * 0.001, EXEC_INSTR_MSG_COUNT * 0.001 },
	  .exec_instr_fma = {
		EXEC_INSTR_FMA_COUNT * 0.001, EXEC_INSTR_FMA_COUNT * 0.001,
		EXEC_INSTR_FMA_COUNT * 0.001, EXEC_INSTR_FMA_COUNT * 0.001 },
	  .exec_instr_cvt = {
		EXEC_INSTR_CVT_COUNT * 0.001, EXEC_INSTR_CVT_COUNT * 0.001,
		EXEC_INSTR_CVT_COUNT * 0.001, EXEC_INSTR_CVT_COUNT * 0.001 },
	  .exec_instr_sfu = {
		EXEC_INSTR_SFU_COUNT * 0.001, EXEC_INSTR_SFU_COUNT * 0.001,
		EXEC_INSTR_SFU_COUNT * 0.001, EXEC_INSTR_SFU_COUNT * 0.001 },
	  .exec_starve_arith = {
		EXEC_STARVE_ARITH_COUNT * 0.001, EXEC_STARVE_ARITH_COUNT * 0.001,
		EXEC_STARVE_ARITH_COUNT * 0.001, EXEC_STARVE_ARITH_COUNT * 0.001 },
	  .tex_tfch_clk_stalled = {
		TEX_TFCH_CLK_STALLED_COUNT * 0.001, TEX_TFCH_CLK_STALLED_COUNT * 0.001,
		TEX_TFCH_CLK_STALLED_COUNT * 0.001, TEX_TFCH_CLK_STALLED_COUNT * 0.001 },
	  .tex_filt_num_operations = {
		TEX_FILT_NUM_OPERATIONS_COUNT * 0.001, TEX_FILT_NUM_OPERATIONS_COUNT * 0.001,
		TEX_FILT_NUM_OPERATIONS_COUNT * 0.001, TEX_FILT_NUM_OPERATIONS_COUNT * 0.001 },
	  .gpu_active = ACTIVE_COUNT * 0.01,
	  .idvs_pos_shad_stall = IDVS_POS_SHAD_STALL_COUNT * 0.001,
	  .prefetch_stall = PREFETCH_STALL_COUNT * 0.001,
	  .vfetch_pos_read_wait = VFETCH_POS_READ_WAIT_COUNT * 0.001,
	  .vfetch_vertex_wait = VFETCH_VERTEX_WAIT_COUNT * 0.001,
	  .primassy_stall = PRIMASSY_STALL_COUNT * 0.001,
	  .idvs_var_shad_stall = IDVS_VAR_SHAD_STALL_COUNT * 0.001,
	  .iter_stall = ITER_STALL_COUNT * 0.001,
	  .pmgr_ptr_rd_stall = PMGR_PTR_RD_STALL_COUNT * 0.001,
	  .primassy_pos_shader_wait = PRIMASSY_POS_SHADER_WAIT_COUNT * 0.001,
	  .l2_rd_msg_in_cu = {
		L2_RD_MSG_IN_CU_COUNT * 0.001, L2_RD_MSG_IN_CU_COUNT * 0.001,
		L2_RD_MSG_IN_CU_COUNT * 0.001, L2_RD_MSG_IN_CU_COUNT * 0.001 },
	  .l2_rd_msg_in = {
		L2_RD_MSG_IN_COUNT * 0.001, L2_RD_MSG_IN_COUNT * 0.001,
		L2_RD_MSG_IN_COUNT * 0.001, L2_RD_MSG_IN_COUNT * 0.001 },
	  .l2_wr_msg_in = {
		L2_WR_MSG_IN_COUNT * 0.001, L2_WR_MSG_IN_COUNT * 0.001,
		L2_WR_MSG_IN_COUNT * 0.001, L2_WR_MSG_IN_COUNT * 0.001 },
	  .l2_snp_msg_in = {
		L2_SNP_MSG_IN_COUNT * 0.001, L2_SNP_MSG_IN_COUNT * 0.001,
		L2_SNP_MSG_IN_COUNT * 0.001, L2_SNP_MSG_IN_COUNT * 0.001 },
	  .l2_rd_msg_out = {
		L2_RD_MSG_OUT_COUNT * 0.001, L2_RD_MSG_OUT_COUNT * 0.001,
		L2_RD_MSG_OUT_COUNT * 0.001, L2_RD_MSG_OUT_COUNT * 0.001 },
	  .l2_read_lookup = {
		L2_READ_LOOKUP_COUNT * 0.001, L2_READ_LOOKUP_COUNT * 0.001,
		L2_READ_LOOKUP_COUNT * 0.001, L2_READ_LOOKUP_COUNT * 0.001 },
	  .l2_ext_read_nosnp = {
		L2_EXT_READ_NOSNP_COUNT * 0.001, L2_EXT_READ_NOSNP_COUNT * 0.001,
		L2_EXT_READ_NOSNP_COUNT * 0.001, L2_EXT_READ_NOSNP_COUNT * 0.001 },
	  .l2_rd_msg_in_stall = {
		L2_RD_MSG_IN_STALL_COUNT * 0.001, L2_RD_MSG_IN_STALL_COUNT * 0.001,
		L2_RD_MSG_IN_STALL_COUNT * 0.001, L2_RD_MSG_IN_STALL_COUNT * 0.001 },
	  .l2_ext_write = {
		L2_EXT_WRITE_COUNT * 0.001, L2_EXT_WRITE_COUNT * 0.001,
		L2_EXT_WRITE_COUNT * 0.001, L2_EXT_WRITE_COUNT * 0.001 },
	  .l2_ext_write_nosnp_full = {
		L2_EXT_WRITE_NOSNP_FULL_COUNT * 0.001, L2_EXT_WRITE_NOSNP_FULL_COUNT * 0.001,
		L2_EXT_WRITE_NOSNP_FULL_COUNT * 0.001, L2_EXT_WRITE_NOSNP_FULL_COUNT * 0.001 },
	  .ls_mem_read_short = {
		LS_MEM_READ_SHORT_COUNT * 0.001, LS_MEM_READ_SHORT_COUNT * 0.001,
		LS_MEM_READ_SHORT_COUNT * 0.001, LS_MEM_READ_SHORT_COUNT * 0.001 },
	  .frag_starving = {
		FRAG_STARVING_COUNT * 0.001,
		FRAG_STARVING_COUNT * 0.001,
		FRAG_STARVING_COUNT * 0.001,
		FRAG_STARVING_COUNT * 0.001 },
	  .frag_partial_quads_rast = {
		FRAG_PARTIAL_QUADS_RAST_COUNT * 0.001,
		FRAG_PARTIAL_QUADS_RAST_COUNT * 0.001,
		FRAG_PARTIAL_QUADS_RAST_COUNT * 0.001,
		FRAG_PARTIAL_QUADS_RAST_COUNT * 0.001 },
	  .frag_quads_ezs_update = {
		FRAG_QUADS_EZS_UPDATE_COUNT * 0.001, FRAG_QUADS_EZS_UPDATE_COUNT * 0.001,
		FRAG_QUADS_EZS_UPDATE_COUNT * 0.001, FRAG_QUADS_EZS_UPDATE_COUNT * 0.001 },
	  .full_quad_warps = {
		FULL_QUAD_WARPS_COUNT * 0.001, FULL_QUAD_WARPS_COUNT * 0.001,
		FULL_QUAD_WARPS_COUNT * 0.001, FULL_QUAD_WARPS_COUNT * 0.001 },
	  .ls_mem_write_short = {
		LS_MEM_WRITE_SHORT_COUNT * 0.001, LS_MEM_WRITE_SHORT_COUNT * 0.001,
		LS_MEM_WRITE_SHORT_COUNT * 0.001, LS_MEM_WRITE_SHORT_COUNT * 0.001 },
	  .vary_slot_16 = {
		VARY_SLOT_16_COUNT * 0.001, VARY_SLOT_16_COUNT * 0.001,
		VARY_SLOT_16_COUNT * 0.001, VARY_SLOT_16_COUNT * 0.001 },
	  .beats_rd_lsc_ext = {
		BEATS_RD_LSC_EXT_COUNT * 0.001, BEATS_RD_LSC_EXT_COUNT * 0.001,
		BEATS_RD_LSC_EXT_COUNT * 0.001, BEATS_RD_LSC_EXT_COUNT * 0.001 },
	  .beats_rd_tex = {
		BEATS_RD_TEX_COUNT * 0.001, BEATS_RD_TEX_COUNT * 0.001,
		BEATS_RD_TEX_COUNT * 0.001, BEATS_RD_TEX_COUNT * 0.001 },
	  .beats_rd_tex_ext = {
		BEATS_RD_TEX_EXT_COUNT * 0.001, BEATS_RD_TEX_EXT_COUNT * 0.001,
		BEATS_RD_TEX_EXT_COUNT * 0.001, BEATS_RD_TEX_EXT_COUNT * 0.001 },
	  .frag_quads_coarse = {
		FRAG_QUADS_COARSE_COUNT * 0.001, FRAG_QUADS_COARSE_COUNT * 0.001,
		FRAG_QUADS_COARSE_COUNT * 0.001, FRAG_QUADS_COARSE_COUNT * 0.001 },
	  .rt_rays_started = {
		RT_RAYS_STARTED_COUNT * 0.001, RT_RAYS_STARTED_COUNT * 0.001,
		RT_RAYS_STARTED_COUNT * 0.001, RT_RAYS_STARTED_COUNT * 0.001 },
	  .tex_cfch_num_l1_ct_operations = {
		TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT * 0.001, TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT * 0.001,
		TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT * 0.001, TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT * 0.001 },
	  .exec_instr_slot1 = {
		EXEC_INSTR_SLOT1_COUNT * 0.001, EXEC_INSTR_SLOT1_COUNT * 0.001,
		EXEC_INSTR_SLOT1_COUNT * 0.001, EXEC_INSTR_SLOT1_COUNT * 0.001 },
	  .exec_issue_slot_any = {
		EXEC_ISSUE_SLOT_ANY_COUNT * 0.001, EXEC_ISSUE_SLOT_ANY_COUNT * 0.001,
		EXEC_ISSUE_SLOT_ANY_COUNT * 0.001, EXEC_ISSUE_SLOT_ANY_COUNT * 0.001},
	  .interval_ns = INTERVAL_NS,
	  .scale = SCALE,
	  .temp = TEMP,
	  .util = 0.04,
	},

	{ .freq = FREQ_HZ,
	  .voltage = VOLTAGE_MV,
	  .num_l2_slices = 1,
	  .l2_access = { L2_COUNT, },
	  .num_cores = 4,
	  .exec_instr_count = {
		INSTR_COUNT, INSTR_COUNT, INSTR_COUNT, INSTR_COUNT },
	  .tex_issue = {
		TEX_ISSUE_COUNT, TEX_ISSUE_COUNT,
		TEX_ISSUE_COUNT, TEX_ISSUE_COUNT },
	  .tile_wb = {
		TILE_WB_COUNT, TILE_WB_COUNT, TILE_WB_COUNT, TILE_WB_COUNT },
	  .exec_instr_msg = {
		EXEC_INSTR_MSG_COUNT, EXEC_INSTR_MSG_COUNT,
		EXEC_INSTR_MSG_COUNT, EXEC_INSTR_MSG_COUNT },
	  .exec_instr_fma = {
		EXEC_INSTR_FMA_COUNT, EXEC_INSTR_FMA_COUNT,
		EXEC_INSTR_FMA_COUNT, EXEC_INSTR_FMA_COUNT },
	  .exec_instr_cvt = {
		EXEC_INSTR_CVT_COUNT, EXEC_INSTR_CVT_COUNT,
		EXEC_INSTR_CVT_COUNT, EXEC_INSTR_CVT_COUNT },
	  .exec_instr_sfu = {
		EXEC_INSTR_SFU_COUNT, EXEC_INSTR_SFU_COUNT,
		EXEC_INSTR_SFU_COUNT, EXEC_INSTR_SFU_COUNT },
	  .exec_starve_arith = {
		EXEC_STARVE_ARITH_COUNT, EXEC_STARVE_ARITH_COUNT,
		EXEC_STARVE_ARITH_COUNT, EXEC_STARVE_ARITH_COUNT },
	  .tex_tfch_clk_stalled = {
		TEX_TFCH_CLK_STALLED_COUNT, TEX_TFCH_CLK_STALLED_COUNT,
		TEX_TFCH_CLK_STALLED_COUNT, TEX_TFCH_CLK_STALLED_COUNT },
	  .tex_filt_num_operations = {
		TEX_FILT_NUM_OPERATIONS_COUNT, TEX_FILT_NUM_OPERATIONS_COUNT,
		TEX_FILT_NUM_OPERATIONS_COUNT, TEX_FILT_NUM_OPERATIONS_COUNT },
	  .gpu_active = ACTIVE_COUNT,
	  .idvs_pos_shad_stall = IDVS_POS_SHAD_STALL_COUNT,
	  .prefetch_stall = PREFETCH_STALL_COUNT,
	  .vfetch_pos_read_wait = VFETCH_POS_READ_WAIT_COUNT,
	  .vfetch_vertex_wait = VFETCH_VERTEX_WAIT_COUNT,
	  .primassy_stall = PRIMASSY_STALL_COUNT,
	  .idvs_var_shad_stall = IDVS_VAR_SHAD_STALL_COUNT,
	  .iter_stall = ITER_STALL_COUNT,
	  .pmgr_ptr_rd_stall = PMGR_PTR_RD_STALL_COUNT,
	  .primassy_pos_shader_wait = PRIMASSY_POS_SHADER_WAIT_COUNT,
	  .l2_rd_msg_in_cu = {
		L2_RD_MSG_IN_CU_COUNT, L2_RD_MSG_IN_CU_COUNT,
		L2_RD_MSG_IN_CU_COUNT, L2_RD_MSG_IN_CU_COUNT },
	  .l2_rd_msg_in = {
		L2_RD_MSG_IN_COUNT, L2_RD_MSG_IN_COUNT,
		L2_RD_MSG_IN_COUNT, L2_RD_MSG_IN_COUNT },
	  .l2_wr_msg_in = {
		L2_WR_MSG_IN_COUNT, L2_WR_MSG_IN_COUNT,
		L2_WR_MSG_IN_COUNT, L2_WR_MSG_IN_COUNT },
	  .l2_snp_msg_in = {
		L2_SNP_MSG_IN_COUNT, L2_SNP_MSG_IN_COUNT,
		L2_SNP_MSG_IN_COUNT, L2_SNP_MSG_IN_COUNT },
	  .l2_rd_msg_out = {
		L2_RD_MSG_OUT_COUNT, L2_RD_MSG_OUT_COUNT,
		L2_RD_MSG_OUT_COUNT, L2_RD_MSG_OUT_COUNT },
	  .l2_read_lookup = {
		L2_READ_LOOKUP_COUNT, L2_READ_LOOKUP_COUNT,
		L2_READ_LOOKUP_COUNT, L2_READ_LOOKUP_COUNT },
	  .l2_ext_read_nosnp = {
		L2_EXT_READ_NOSNP_COUNT, L2_EXT_READ_NOSNP_COUNT,
		L2_EXT_READ_NOSNP_COUNT, L2_EXT_READ_NOSNP_COUNT },
	  .l2_rd_msg_in_stall = {
		L2_RD_MSG_IN_STALL_COUNT, L2_RD_MSG_IN_STALL_COUNT,
		L2_RD_MSG_IN_STALL_COUNT, L2_RD_MSG_IN_STALL_COUNT },
	  .l2_ext_write = {
		L2_EXT_WRITE_COUNT, L2_EXT_WRITE_COUNT,
		L2_EXT_WRITE_COUNT, L2_EXT_WRITE_COUNT },
	  .l2_ext_write_nosnp_full = {
		L2_EXT_WRITE_NOSNP_FULL_COUNT, L2_EXT_WRITE_NOSNP_FULL_COUNT,
		L2_EXT_WRITE_NOSNP_FULL_COUNT, L2_EXT_WRITE_NOSNP_FULL_COUNT },
	  .ls_mem_read_short = {
		LS_MEM_READ_SHORT_COUNT, LS_MEM_READ_SHORT_COUNT,
		LS_MEM_READ_SHORT_COUNT, LS_MEM_READ_SHORT_COUNT },
	  .frag_starving = {
		FRAG_STARVING_COUNT, FRAG_STARVING_COUNT,
		FRAG_STARVING_COUNT, FRAG_STARVING_COUNT },
	  .frag_partial_quads_rast = {
		FRAG_PARTIAL_QUADS_RAST_COUNT, FRAG_PARTIAL_QUADS_RAST_COUNT,
		FRAG_PARTIAL_QUADS_RAST_COUNT, FRAG_PARTIAL_QUADS_RAST_COUNT },
	  .frag_quads_ezs_update = {
		FRAG_QUADS_EZS_UPDATE_COUNT, FRAG_QUADS_EZS_UPDATE_COUNT,
		FRAG_QUADS_EZS_UPDATE_COUNT, FRAG_QUADS_EZS_UPDATE_COUNT },
	  .full_quad_warps = {
		FULL_QUAD_WARPS_COUNT, FULL_QUAD_WARPS_COUNT,
		FULL_QUAD_WARPS_COUNT, FULL_QUAD_WARPS_COUNT },
	  .ls_mem_write_short = {
		LS_MEM_WRITE_SHORT_COUNT, LS_MEM_WRITE_SHORT_COUNT,
		LS_MEM_WRITE_SHORT_COUNT, LS_MEM_WRITE_SHORT_COUNT },
	  .vary_slot_16 = {
		VARY_SLOT_16_COUNT, VARY_SLOT_16_COUNT,
		VARY_SLOT_16_COUNT, VARY_SLOT_16_COUNT },
	  .beats_rd_lsc_ext = {
		BEATS_RD_LSC_EXT_COUNT, BEATS_RD_LSC_EXT_COUNT,
		BEATS_RD_LSC_EXT_COUNT, BEATS_RD_LSC_EXT_COUNT },
	  .beats_rd_tex = {
		BEATS_RD_TEX_COUNT, BEATS_RD_TEX_COUNT,
		BEATS_RD_TEX_COUNT, BEATS_RD_TEX_COUNT },
	  .beats_rd_tex_ext = {
		BEATS_RD_TEX_EXT_COUNT, BEATS_RD_TEX_EXT_COUNT,
		BEATS_RD_TEX_EXT_COUNT, BEATS_RD_TEX_EXT_COUNT },
	  .frag_quads_coarse = {
		FRAG_QUADS_COARSE_COUNT, FRAG_QUADS_COARSE_COUNT,
		FRAG_QUADS_COARSE_COUNT, FRAG_QUADS_COARSE_COUNT },
	  .rt_rays_started = {
		RT_RAYS_STARTED_COUNT, RT_RAYS_STARTED_COUNT,
		RT_RAYS_STARTED_COUNT, RT_RAYS_STARTED_COUNT },
	  .tex_cfch_num_l1_ct_operations = {
		TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT,
		TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT },
	  .exec_instr_slot1 = {
		EXEC_INSTR_SLOT1_COUNT, EXEC_INSTR_SLOT1_COUNT,
		EXEC_INSTR_SLOT1_COUNT, EXEC_INSTR_SLOT1_COUNT },
	  .exec_issue_slot_any = {
		EXEC_ISSUE_SLOT_ANY_COUNT, EXEC_ISSUE_SLOT_ANY_COUNT,
		EXEC_ISSUE_SLOT_ANY_COUNT, EXEC_ISSUE_SLOT_ANY_COUNT },
	  .interval_ns = INTERVAL_NS / 1000, /* 1000% util (nonsensical) */
	  .scale = SCALE,
	  .temp = TEMP,
	  .util = 0.99,
	},

	/* One Operating Performance Point with typical utilizations */
	{ .freq = FREQ_HZ,
	  .voltage = VOLTAGE_MV,
	  .num_l2_slices = 1,
	  .l2_access = { L2_COUNT * 0.1, },
	  .num_cores = 4,
	  .exec_instr_count = {
		INSTR_COUNT * 0.1, INSTR_COUNT * 0.1,
		INSTR_COUNT * 0.1, INSTR_COUNT * 0.1 },
	  .tex_issue = {
		TEX_ISSUE_COUNT, TEX_ISSUE_COUNT * 0.1,
		TEX_ISSUE_COUNT * 0.1, TEX_ISSUE_COUNT * 0.1 },
	  .tile_wb = {
		TILE_WB_COUNT * 0.1, TILE_WB_COUNT * 0.1,
		TILE_WB_COUNT * 0.1, TILE_WB_COUNT * 0.1 },
	  .exec_instr_msg = {
		EXEC_INSTR_MSG_COUNT * 0.1, EXEC_INSTR_MSG_COUNT * 0.1,
		EXEC_INSTR_MSG_COUNT * 0.1, EXEC_INSTR_MSG_COUNT * 0.1 },
	  .exec_instr_fma = {
		EXEC_INSTR_FMA_COUNT * 0.1, EXEC_INSTR_FMA_COUNT * 0.1,
		EXEC_INSTR_FMA_COUNT * 0.1, EXEC_INSTR_FMA_COUNT * 0.1 },
	  .exec_instr_cvt = {
		EXEC_INSTR_CVT_COUNT * 0.1, EXEC_INSTR_CVT_COUNT * 0.1,
		EXEC_INSTR_CVT_COUNT * 0.1, EXEC_INSTR_CVT_COUNT * 0.1 },
	  .exec_instr_sfu = {
		EXEC_INSTR_SFU_COUNT * 0.1, EXEC_INSTR_SFU_COUNT * 0.1,
		EXEC_INSTR_SFU_COUNT * 0.1, EXEC_INSTR_SFU_COUNT * 0.1 },
	  .exec_starve_arith = {
		EXEC_STARVE_ARITH_COUNT * 0.1, EXEC_STARVE_ARITH_COUNT * 0.1,
		EXEC_STARVE_ARITH_COUNT * 0.1, EXEC_STARVE_ARITH_COUNT * 0.1 },
	  .tex_tfch_clk_stalled = {
		TEX_TFCH_CLK_STALLED_COUNT * 0.1, TEX_TFCH_CLK_STALLED_COUNT * 0.1,
		TEX_TFCH_CLK_STALLED_COUNT * 0.1, TEX_TFCH_CLK_STALLED_COUNT * 0.1 },
	  .tex_filt_num_operations = {
		TEX_FILT_NUM_OPERATIONS_COUNT * 0.1, TEX_FILT_NUM_OPERATIONS_COUNT * 0.1,
		TEX_FILT_NUM_OPERATIONS_COUNT * 0.1, TEX_FILT_NUM_OPERATIONS_COUNT * 0.1 },
	  .gpu_active = ACTIVE_COUNT * 0.1,
	  .idvs_pos_shad_stall = IDVS_POS_SHAD_STALL_COUNT * 0.1,
	  .prefetch_stall = PREFETCH_STALL_COUNT * 0.1,
	  .vfetch_pos_read_wait = VFETCH_POS_READ_WAIT_COUNT * 0.1,
	  .vfetch_vertex_wait = VFETCH_VERTEX_WAIT_COUNT * 0.1,
	  .primassy_stall = PRIMASSY_STALL_COUNT * 0.1,
	  .idvs_var_shad_stall = IDVS_VAR_SHAD_STALL_COUNT * 0.1,
	  .iter_stall = ITER_STALL_COUNT * 0.1,
	  .pmgr_ptr_rd_stall = PMGR_PTR_RD_STALL_COUNT * 0.1,
	  .primassy_pos_shader_wait = PRIMASSY_POS_SHADER_WAIT_COUNT * 0.1,
	  .l2_rd_msg_in_cu = {
		L2_RD_MSG_IN_CU_COUNT * 0.1, L2_RD_MSG_IN_CU_COUNT * 0.1,
		L2_RD_MSG_IN_CU_COUNT * 0.1, L2_RD_MSG_IN_CU_COUNT * 0.1 },
	  .l2_rd_msg_in = {
		L2_RD_MSG_IN_COUNT * 0.1, L2_RD_MSG_IN_COUNT * 0.1,
		L2_RD_MSG_IN_COUNT * 0.1, L2_RD_MSG_IN_COUNT * 0.1 },
	  .l2_wr_msg_in = {
		L2_WR_MSG_IN_COUNT * 0.1, L2_WR_MSG_IN_COUNT * 0.1,
		L2_WR_MSG_IN_COUNT * 0.1, L2_WR_MSG_IN_COUNT * 0.1 },
	  .l2_snp_msg_in = {
		L2_SNP_MSG_IN_COUNT * 0.1, L2_SNP_MSG_IN_COUNT * 0.1,
		L2_SNP_MSG_IN_COUNT * 0.1, L2_SNP_MSG_IN_COUNT * 0.1 },
	  .l2_rd_msg_out = {
		L2_RD_MSG_OUT_COUNT * 0.1, L2_RD_MSG_OUT_COUNT * 0.1,
		L2_RD_MSG_OUT_COUNT * 0.1, L2_RD_MSG_OUT_COUNT * 0.1 },
	  .l2_read_lookup = {
		L2_READ_LOOKUP_COUNT * 0.1, L2_READ_LOOKUP_COUNT * 0.1,
		L2_READ_LOOKUP_COUNT * 0.1, L2_READ_LOOKUP_COUNT * 0.1 },
	  .l2_ext_read_nosnp = {
		L2_EXT_READ_NOSNP_COUNT * 0.1, L2_EXT_READ_NOSNP_COUNT * 0.1,
		L2_EXT_READ_NOSNP_COUNT * 0.1, L2_EXT_READ_NOSNP_COUNT * 0.1 },
	  .l2_rd_msg_in_stall = {
		L2_RD_MSG_IN_STALL_COUNT * 0.01, L2_RD_MSG_IN_STALL_COUNT * 0.01,
		L2_RD_MSG_IN_STALL_COUNT * 0.01, L2_RD_MSG_IN_STALL_COUNT * 0.01 },
	  .l2_ext_write = {
		L2_EXT_WRITE_COUNT * 0.01, L2_EXT_WRITE_COUNT * 0.01,
		L2_EXT_WRITE_COUNT * 0.01, L2_EXT_WRITE_COUNT * 0.01 },
	  .l2_ext_write_nosnp_full = {
		L2_EXT_WRITE_NOSNP_FULL_COUNT * 0.1, L2_EXT_WRITE_NOSNP_FULL_COUNT * 0.1,
		L2_EXT_WRITE_NOSNP_FULL_COUNT * 0.1, L2_EXT_WRITE_NOSNP_FULL_COUNT * 0.1 },
	  .ls_mem_read_short = {
		LS_MEM_READ_SHORT_COUNT * 0.1, LS_MEM_READ_SHORT_COUNT * 0.1,
		LS_MEM_READ_SHORT_COUNT * 0.1, LS_MEM_READ_SHORT_COUNT * 0.1 },
	  .frag_starving = {
		FRAG_STARVING_COUNT * 0.1, FRAG_STARVING_COUNT * 0.1,
		FRAG_STARVING_COUNT * 0.1, FRAG_STARVING_COUNT * 0.1 },
	  .frag_partial_quads_rast = {
		FRAG_PARTIAL_QUADS_RAST_COUNT * 0.1, FRAG_PARTIAL_QUADS_RAST_COUNT * 0.1,
		FRAG_PARTIAL_QUADS_RAST_COUNT * 0.1, FRAG_PARTIAL_QUADS_RAST_COUNT * 0.1 },
	  .frag_quads_ezs_update = {
		FRAG_QUADS_EZS_UPDATE_COUNT * 0.1, FRAG_QUADS_EZS_UPDATE_COUNT * 0.1,
		FRAG_QUADS_EZS_UPDATE_COUNT * 0.1, FRAG_QUADS_EZS_UPDATE_COUNT * 0.1 },
	  .full_quad_warps = {
		FULL_QUAD_WARPS_COUNT * 0.1, FULL_QUAD_WARPS_COUNT * 0.1,
		FULL_QUAD_WARPS_COUNT * 0.1, FULL_QUAD_WARPS_COUNT * 0.1 },
	  .ls_mem_write_short = {
		LS_MEM_WRITE_SHORT_COUNT * 0.1, LS_MEM_WRITE_SHORT_COUNT * 0.1,
		LS_MEM_WRITE_SHORT_COUNT * 0.1, LS_MEM_WRITE_SHORT_COUNT * 0.1 },
	  .vary_slot_16 = {
		VARY_SLOT_16_COUNT * 0.1, VARY_SLOT_16_COUNT * 0.1,
		VARY_SLOT_16_COUNT * 0.1, VARY_SLOT_16_COUNT * 0.1 },
	  .beats_rd_lsc_ext = {
		BEATS_RD_LSC_EXT_COUNT * 0.1, BEATS_RD_LSC_EXT_COUNT * 0.1,
		BEATS_RD_LSC_EXT_COUNT * 0.1, BEATS_RD_LSC_EXT_COUNT * 0.1 },
	  .beats_rd_tex = {
		BEATS_RD_TEX_COUNT * 0.1, BEATS_RD_TEX_COUNT * 0.1,
		BEATS_RD_TEX_COUNT * 0.1, BEATS_RD_TEX_COUNT * 0.1 },
	  .beats_rd_tex_ext = {
		BEATS_RD_TEX_EXT_COUNT * 0.1, BEATS_RD_TEX_EXT_COUNT * 0.1,
		BEATS_RD_TEX_EXT_COUNT * 0.1, BEATS_RD_TEX_EXT_COUNT * 0.1 },
	  .frag_quads_coarse = {
		FRAG_QUADS_COARSE_COUNT * 0.1, FRAG_QUADS_COARSE_COUNT * 0.1,
		FRAG_QUADS_COARSE_COUNT * 0.1, FRAG_QUADS_COARSE_COUNT * 0.1 },
	  .rt_rays_started = {
		RT_RAYS_STARTED_COUNT * 0.1, RT_RAYS_STARTED_COUNT * 0.1,
		RT_RAYS_STARTED_COUNT * 0.1, RT_RAYS_STARTED_COUNT * 0.1 },
	  .tex_cfch_num_l1_ct_operations = {
		TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT * 0.1, TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT * 0.1,
		TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT * 0.1, TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT * 0.1 },
	  .exec_instr_slot1 = {
		EXEC_INSTR_SLOT1_COUNT * 0.1, EXEC_INSTR_SLOT1_COUNT * 0.1,
		EXEC_INSTR_SLOT1_COUNT * 0.1, EXEC_INSTR_SLOT1_COUNT * 0.1 },
	  .exec_issue_slot_any = {
		EXEC_ISSUE_SLOT_ANY_COUNT * 0.1, EXEC_ISSUE_SLOT_ANY_COUNT * 0.1,
		EXEC_ISSUE_SLOT_ANY_COUNT * 0.1, EXEC_ISSUE_SLOT_ANY_COUNT * 0.1 },
	  .interval_ns = INTERVAL_NS,
	  .scale = SCALE,
	  .temp = TEMP,
	  .util = 0.61,
	},

	{ .freq = FREQ_HZ,
	  .voltage = VOLTAGE_MV,
	  .num_l2_slices = 1,
	  .l2_access = { L2_COUNT * 0.5, },
	  .num_cores = 4,
	  .exec_instr_count = {
		INSTR_COUNT * 0.5, INSTR_COUNT * 0.5,
		INSTR_COUNT * 0.5, INSTR_COUNT * 0.5 },
	  .tex_issue = {
		TEX_ISSUE_COUNT, TEX_ISSUE_COUNT * 0.5,
		TEX_ISSUE_COUNT * 0.5, TEX_ISSUE_COUNT * 0.5 },
	  .tile_wb = {
		TILE_WB_COUNT * 0.5, TILE_WB_COUNT * 0.5,
		TILE_WB_COUNT * 0.5, TILE_WB_COUNT * 0.5 },
	  .exec_instr_msg = {
		EXEC_INSTR_MSG_COUNT * 0.5, EXEC_INSTR_MSG_COUNT * 0.5,
		EXEC_INSTR_MSG_COUNT * 0.5, EXEC_INSTR_MSG_COUNT * 0.5 },
	  .exec_instr_fma = {
		EXEC_INSTR_FMA_COUNT * 0.5, EXEC_INSTR_FMA_COUNT * 0.5,
		EXEC_INSTR_FMA_COUNT * 0.5, EXEC_INSTR_FMA_COUNT * 0.5 },
	  .exec_instr_cvt = {
		EXEC_INSTR_CVT_COUNT * 0.5, EXEC_INSTR_CVT_COUNT * 0.5,
		EXEC_INSTR_CVT_COUNT * 0.5, EXEC_INSTR_CVT_COUNT * 0.5 },
	  .exec_instr_sfu = {
		EXEC_INSTR_SFU_COUNT * 0.5, EXEC_INSTR_SFU_COUNT * 0.5,
		EXEC_INSTR_SFU_COUNT * 0.5, EXEC_INSTR_SFU_COUNT * 0.5 },
	  .exec_starve_arith = {
		EXEC_STARVE_ARITH_COUNT * 0.5, EXEC_STARVE_ARITH_COUNT * 0.5,
		EXEC_STARVE_ARITH_COUNT * 0.5, EXEC_STARVE_ARITH_COUNT * 0.5 },
	  .tex_tfch_clk_stalled = {
		TEX_TFCH_CLK_STALLED_COUNT * 0.5, TEX_TFCH_CLK_STALLED_COUNT * 0.5,
		TEX_TFCH_CLK_STALLED_COUNT * 0.5, TEX_TFCH_CLK_STALLED_COUNT * 0.5 },
	  .tex_filt_num_operations = {
		TEX_FILT_NUM_OPERATIONS_COUNT * 0.5, TEX_FILT_NUM_OPERATIONS_COUNT * 0.5,
		TEX_FILT_NUM_OPERATIONS_COUNT * 0.5, TEX_FILT_NUM_OPERATIONS_COUNT * 0.5 },
	  .gpu_active = ACTIVE_COUNT * 0.5,
	  .idvs_pos_shad_stall = IDVS_POS_SHAD_STALL_COUNT * 0.5,
	  .prefetch_stall = PREFETCH_STALL_COUNT * 0.5,
	  .vfetch_pos_read_wait = VFETCH_POS_READ_WAIT_COUNT * 0.5,
	  .vfetch_vertex_wait = VFETCH_VERTEX_WAIT_COUNT * 0.5,
	  .primassy_stall = PRIMASSY_STALL_COUNT * 0.5,
	  .idvs_var_shad_stall = IDVS_VAR_SHAD_STALL_COUNT * 0.5,
	  .iter_stall = ITER_STALL_COUNT * 0.5,
	  .pmgr_ptr_rd_stall = PMGR_PTR_RD_STALL_COUNT * 0.5,
	  .primassy_pos_shader_wait = PRIMASSY_POS_SHADER_WAIT_COUNT * 0.5,
	  .l2_rd_msg_in_cu = {
		L2_RD_MSG_IN_CU_COUNT * 0.5, L2_RD_MSG_IN_CU_COUNT * 0.5,
		L2_RD_MSG_IN_CU_COUNT * 0.5, L2_RD_MSG_IN_CU_COUNT * 0.5 },
	  .l2_rd_msg_in = {
		L2_RD_MSG_IN_COUNT * 0.5, L2_RD_MSG_IN_COUNT * 0.5,
		L2_RD_MSG_IN_COUNT * 0.5, L2_RD_MSG_IN_COUNT * 0.5 },
	  .l2_wr_msg_in = {
		L2_WR_MSG_IN_COUNT * 0.5, L2_WR_MSG_IN_COUNT * 0.5,
		L2_WR_MSG_IN_COUNT * 0.5, L2_WR_MSG_IN_COUNT * 0.5 },
	  .l2_snp_msg_in = {
		L2_SNP_MSG_IN_COUNT * 0.5, L2_SNP_MSG_IN_COUNT * 0.5,
		L2_SNP_MSG_IN_COUNT * 0.5, L2_SNP_MSG_IN_COUNT * 0.5 },
	  .l2_rd_msg_out = {
		L2_RD_MSG_OUT_COUNT * 0.5, L2_RD_MSG_OUT_COUNT * 0.5,
		L2_RD_MSG_OUT_COUNT * 0.5, L2_RD_MSG_OUT_COUNT * 0.5 },
	  .l2_read_lookup = {
		L2_READ_LOOKUP_COUNT * 0.5, L2_READ_LOOKUP_COUNT * 0.5,
		L2_READ_LOOKUP_COUNT * 0.5, L2_READ_LOOKUP_COUNT * 0.5 },
	  .l2_ext_read_nosnp = {
		L2_EXT_READ_NOSNP_COUNT * 0.5, L2_EXT_READ_NOSNP_COUNT * 0.5,
		L2_EXT_READ_NOSNP_COUNT * 0.5, L2_EXT_READ_NOSNP_COUNT * 0.5 },
	  .l2_rd_msg_in_stall = {
		L2_RD_MSG_IN_STALL_COUNT * 0.5, L2_RD_MSG_IN_STALL_COUNT * 0.5,
		L2_RD_MSG_IN_STALL_COUNT * 0.5, L2_RD_MSG_IN_STALL_COUNT * 0.5 },
	  .l2_ext_write = {
		L2_EXT_WRITE_COUNT * 0.5, L2_EXT_WRITE_COUNT * 0.5,
		L2_EXT_WRITE_COUNT * 0.5, L2_EXT_WRITE_COUNT * 0.5 },
	  .l2_ext_write_nosnp_full = {
		L2_EXT_WRITE_NOSNP_FULL_COUNT * 0.5, L2_EXT_WRITE_NOSNP_FULL_COUNT * 0.5,
		L2_EXT_WRITE_NOSNP_FULL_COUNT * 0.5, L2_EXT_WRITE_NOSNP_FULL_COUNT * 0.5 },
	  .ls_mem_read_short = {
		LS_MEM_READ_SHORT_COUNT * 0.5, LS_MEM_READ_SHORT_COUNT * 0.5,
		LS_MEM_READ_SHORT_COUNT * 0.5, LS_MEM_READ_SHORT_COUNT * 0.5 },
	  .frag_starving = {
		FRAG_STARVING_COUNT * 0.5, FRAG_STARVING_COUNT * 0.5,
		FRAG_STARVING_COUNT * 0.5, FRAG_STARVING_COUNT * 0.5 },
	  .frag_partial_quads_rast = {
		FRAG_PARTIAL_QUADS_RAST_COUNT * 0.5, FRAG_PARTIAL_QUADS_RAST_COUNT * 0.5,
		FRAG_PARTIAL_QUADS_RAST_COUNT * 0.5, FRAG_PARTIAL_QUADS_RAST_COUNT * 0.5 },
	  .frag_quads_ezs_update = {
		FRAG_QUADS_EZS_UPDATE_COUNT * 0.5, FRAG_QUADS_EZS_UPDATE_COUNT * 0.5,
		FRAG_QUADS_EZS_UPDATE_COUNT * 0.5, FRAG_QUADS_EZS_UPDATE_COUNT * 0.5 },
	  .full_quad_warps = {
		FULL_QUAD_WARPS_COUNT * 0.5, FULL_QUAD_WARPS_COUNT * 0.5,
		FULL_QUAD_WARPS_COUNT * 0.5, FULL_QUAD_WARPS_COUNT * 0.5 },
	  .ls_mem_write_short = {
		LS_MEM_WRITE_SHORT_COUNT * 0.5, LS_MEM_WRITE_SHORT_COUNT * 0.5,
		LS_MEM_WRITE_SHORT_COUNT * 0.5, LS_MEM_WRITE_SHORT_COUNT * 0.5 },
	  .vary_slot_16 = {
		VARY_SLOT_16_COUNT * 0.5, VARY_SLOT_16_COUNT * 0.5,
		VARY_SLOT_16_COUNT * 0.5, VARY_SLOT_16_COUNT * 0.5 },
	  .beats_rd_lsc_ext = {
		BEATS_RD_LSC_EXT_COUNT * 0.5, BEATS_RD_LSC_EXT_COUNT * 0.5,
		BEATS_RD_LSC_EXT_COUNT * 0.5, BEATS_RD_LSC_EXT_COUNT * 0.5 },
	  .beats_rd_tex = {
		BEATS_RD_TEX_COUNT * 0.5, BEATS_RD_TEX_COUNT * 0.5,
		BEATS_RD_TEX_COUNT * 0.5, BEATS_RD_TEX_COUNT * 0.5 },
	  .beats_rd_tex_ext = {
		BEATS_RD_TEX_EXT_COUNT * 0.5, BEATS_RD_TEX_EXT_COUNT * 0.5,
		BEATS_RD_TEX_EXT_COUNT * 0.5, BEATS_RD_TEX_EXT_COUNT * 0.5 },
	  .frag_quads_coarse = {
		FRAG_QUADS_COARSE_COUNT * 0.5, FRAG_QUADS_COARSE_COUNT * 0.5,
		FRAG_QUADS_COARSE_COUNT * 0.5, FRAG_QUADS_COARSE_COUNT * 0.5 },
	  .rt_rays_started = {
		RT_RAYS_STARTED_COUNT * 0.5, RT_RAYS_STARTED_COUNT * 0.5,
		RT_RAYS_STARTED_COUNT * 0.5, RT_RAYS_STARTED_COUNT * 0.5 },
	  .tex_cfch_num_l1_ct_operations = {
		TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT * 0.5, TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT * 0.5,
		TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT * 0.5, TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT * 0.5 },
	  .exec_instr_slot1 = {
		EXEC_INSTR_SLOT1_COUNT * 0.5, EXEC_INSTR_SLOT1_COUNT * 0.5,
		EXEC_INSTR_SLOT1_COUNT * 0.5, EXEC_INSTR_SLOT1_COUNT * 0.5 },
	  .exec_issue_slot_any = {
		EXEC_ISSUE_SLOT_ANY_COUNT * 0.5, EXEC_ISSUE_SLOT_ANY_COUNT * 0.5,
		EXEC_ISSUE_SLOT_ANY_COUNT * 0.5, EXEC_ISSUE_SLOT_ANY_COUNT * 0.5 },
	  .interval_ns = INTERVAL_NS,
	  .scale = SCALE,
	  .temp = TEMP,
	  .util = 0.16,
	},

	{ .freq = FREQ_HZ,
	  .voltage = VOLTAGE_MV,
	  .num_l2_slices = 1,
	  .l2_access = { L2_COUNT * 0.8, },
	  .num_cores = 4,
	  .exec_instr_count = {
		INSTR_COUNT * 0.8, INSTR_COUNT * 0.8,
		INSTR_COUNT * 0.8, INSTR_COUNT * 0.8 },
	  .tex_issue = {
		TEX_ISSUE_COUNT, TEX_ISSUE_COUNT * 0.8,
		TEX_ISSUE_COUNT * 0.8, TEX_ISSUE_COUNT * 0.8 },
	  .tile_wb = {
		TILE_WB_COUNT * 0.8, TILE_WB_COUNT * 0.8,
		TILE_WB_COUNT * 0.8, TILE_WB_COUNT * 0.8 },
	  .exec_instr_msg = {
		EXEC_INSTR_MSG_COUNT * 0.8, EXEC_INSTR_MSG_COUNT * 0.8,
		EXEC_INSTR_MSG_COUNT * 0.8, EXEC_INSTR_MSG_COUNT * 0.8 },
	  .exec_instr_fma = {
		EXEC_INSTR_FMA_COUNT * 0.8, EXEC_INSTR_FMA_COUNT * 0.8,
		EXEC_INSTR_FMA_COUNT * 0.8, EXEC_INSTR_FMA_COUNT * 0.8 },
	  .exec_instr_cvt = {
		EXEC_INSTR_CVT_COUNT * 0.8, EXEC_INSTR_CVT_COUNT * 0.8,
		EXEC_INSTR_CVT_COUNT * 0.8, EXEC_INSTR_CVT_COUNT * 0.8 },
	  .exec_instr_sfu = {
		EXEC_INSTR_SFU_COUNT * 0.8, EXEC_INSTR_SFU_COUNT * 0.8,
		EXEC_INSTR_SFU_COUNT * 0.8, EXEC_INSTR_SFU_COUNT * 0.8 },
	  .exec_starve_arith = {
		EXEC_STARVE_ARITH_COUNT * 0.8, EXEC_STARVE_ARITH_COUNT * 0.8,
		EXEC_STARVE_ARITH_COUNT * 0.8, EXEC_STARVE_ARITH_COUNT * 0.8 },
	  .tex_tfch_clk_stalled = {
		TEX_TFCH_CLK_STALLED_COUNT * 0.8, TEX_TFCH_CLK_STALLED_COUNT * 0.8,
		TEX_TFCH_CLK_STALLED_COUNT * 0.8, TEX_TFCH_CLK_STALLED_COUNT * 0.8 },
	  .tex_filt_num_operations = {
		TEX_FILT_NUM_OPERATIONS_COUNT * 0.8, TEX_FILT_NUM_OPERATIONS_COUNT * 0.8,
		TEX_FILT_NUM_OPERATIONS_COUNT * 0.8, TEX_FILT_NUM_OPERATIONS_COUNT * 0.8 },
	  .gpu_active = ACTIVE_COUNT * 0.8,
	  .idvs_pos_shad_stall = IDVS_POS_SHAD_STALL_COUNT * 0.8,
	  .prefetch_stall = PREFETCH_STALL_COUNT * 0.8,
	  .vfetch_pos_read_wait = VFETCH_POS_READ_WAIT_COUNT * 0.8,
	  .vfetch_vertex_wait = VFETCH_VERTEX_WAIT_COUNT * 0.8,
	  .primassy_stall = PRIMASSY_STALL_COUNT * 0.8,
	  .idvs_var_shad_stall = IDVS_VAR_SHAD_STALL_COUNT * 0.8,
	  .iter_stall = ITER_STALL_COUNT * 0.8,
	  .pmgr_ptr_rd_stall = PMGR_PTR_RD_STALL_COUNT * 0.8,
	  .primassy_pos_shader_wait = PRIMASSY_POS_SHADER_WAIT_COUNT * 0.8,
	  .l2_rd_msg_in_cu = {
		L2_RD_MSG_IN_CU_COUNT * 0.8, L2_RD_MSG_IN_CU_COUNT * 0.8,
		L2_RD_MSG_IN_CU_COUNT * 0.8, L2_RD_MSG_IN_CU_COUNT * 0.8 },
	  .l2_rd_msg_in = {
		L2_RD_MSG_IN_COUNT * 0.8, L2_RD_MSG_IN_COUNT * 0.8,
		L2_RD_MSG_IN_COUNT * 0.8, L2_RD_MSG_IN_COUNT * 0.8 },
	  .l2_wr_msg_in = {
		L2_WR_MSG_IN_COUNT * 0.8, L2_WR_MSG_IN_COUNT * 0.8,
		L2_WR_MSG_IN_COUNT * 0.8, L2_WR_MSG_IN_COUNT * 0.8 },
	  .l2_snp_msg_in = {
		L2_SNP_MSG_IN_COUNT * 0.8, L2_SNP_MSG_IN_COUNT * 0.8,
		L2_SNP_MSG_IN_COUNT * 0.8, L2_SNP_MSG_IN_COUNT * 0.8 },
	  .l2_rd_msg_out = {
		L2_RD_MSG_OUT_COUNT * 0.8, L2_RD_MSG_OUT_COUNT * 0.8,
		L2_RD_MSG_OUT_COUNT * 0.8, L2_RD_MSG_OUT_COUNT * 0.8 },
	  .l2_read_lookup = {
		L2_READ_LOOKUP_COUNT * 0.8, L2_READ_LOOKUP_COUNT * 0.8,
		L2_READ_LOOKUP_COUNT * 0.8, L2_READ_LOOKUP_COUNT * 0.8 },
	  .l2_ext_read_nosnp = {
		L2_EXT_READ_NOSNP_COUNT * 0.8, L2_EXT_READ_NOSNP_COUNT * 0.8,
		L2_EXT_READ_NOSNP_COUNT * 0.8, L2_EXT_READ_NOSNP_COUNT * 0.8 },
	  .l2_rd_msg_in_stall = {
		L2_RD_MSG_IN_STALL_COUNT * 0.8, L2_RD_MSG_IN_STALL_COUNT * 0.8,
		L2_RD_MSG_IN_STALL_COUNT * 0.8, L2_RD_MSG_IN_STALL_COUNT * 0.8},
	  .l2_ext_write = {
		L2_EXT_WRITE_COUNT * 0.8, L2_EXT_WRITE_COUNT * 0.8,
		L2_EXT_WRITE_COUNT * 0.8, L2_EXT_WRITE_COUNT * 0.8 },
	  .l2_ext_write_nosnp_full = {
		L2_EXT_WRITE_NOSNP_FULL_COUNT * 0.8, L2_EXT_WRITE_NOSNP_FULL_COUNT * 0.8,
		L2_EXT_WRITE_NOSNP_FULL_COUNT * 0.8, L2_EXT_WRITE_NOSNP_FULL_COUNT * 0.8 },
	  .ls_mem_read_short = {
		LS_MEM_READ_SHORT_COUNT * 0.8, LS_MEM_READ_SHORT_COUNT * 0.8,
		LS_MEM_READ_SHORT_COUNT * 0.8, LS_MEM_READ_SHORT_COUNT * 0.8 },
	  .frag_starving = {
		FRAG_STARVING_COUNT * 0.8, FRAG_STARVING_COUNT * 0.8,
		FRAG_STARVING_COUNT * 0.8, FRAG_STARVING_COUNT * 0.8 },
	  .frag_partial_quads_rast = {
		FRAG_PARTIAL_QUADS_RAST_COUNT * 0.8, FRAG_PARTIAL_QUADS_RAST_COUNT * 0.8,
		FRAG_PARTIAL_QUADS_RAST_COUNT * 0.8, FRAG_PARTIAL_QUADS_RAST_COUNT * 0.8 },
	  .frag_quads_ezs_update = {
		FRAG_QUADS_EZS_UPDATE_COUNT * 0.8, FRAG_QUADS_EZS_UPDATE_COUNT * 0.8,
		FRAG_QUADS_EZS_UPDATE_COUNT * 0.8, FRAG_QUADS_EZS_UPDATE_COUNT * 0.8 },
	  .full_quad_warps = {
		FULL_QUAD_WARPS_COUNT * 0.8, FULL_QUAD_WARPS_COUNT * 0.8,
		FULL_QUAD_WARPS_COUNT * 0.8, FULL_QUAD_WARPS_COUNT * 0.8 },
	  .ls_mem_write_short = {
		LS_MEM_WRITE_SHORT_COUNT * 0.8, LS_MEM_WRITE_SHORT_COUNT * 0.8,
		LS_MEM_WRITE_SHORT_COUNT * 0.8, LS_MEM_WRITE_SHORT_COUNT * 0.8 },
	  .vary_slot_16 = {
		VARY_SLOT_16_COUNT * 0.8, VARY_SLOT_16_COUNT * 0.8,
		VARY_SLOT_16_COUNT * 0.8, VARY_SLOT_16_COUNT * 0.8 },
	  .beats_rd_lsc_ext = {
		BEATS_RD_LSC_EXT_COUNT * 0.8, BEATS_RD_LSC_EXT_COUNT * 0.8,
		BEATS_RD_LSC_EXT_COUNT * 0.8, BEATS_RD_LSC_EXT_COUNT * 0.8 },
	  .beats_rd_tex = {
		BEATS_RD_TEX_COUNT * 0.8, BEATS_RD_TEX_COUNT * 0.8,
		BEATS_RD_TEX_COUNT * 0.8, BEATS_RD_TEX_COUNT * 0.8 },
	  .beats_rd_tex_ext = {
		BEATS_RD_TEX_EXT_COUNT * 0.8, BEATS_RD_TEX_EXT_COUNT * 0.8,
		BEATS_RD_TEX_EXT_COUNT * 0.8, BEATS_RD_TEX_EXT_COUNT * 0.8 },
	  .frag_quads_coarse = {
		FRAG_QUADS_COARSE_COUNT * 0.8, FRAG_QUADS_COARSE_COUNT * 0.8,
		FRAG_QUADS_COARSE_COUNT * 0.8, FRAG_QUADS_COARSE_COUNT * 0.8 },
	  .rt_rays_started = {
		RT_RAYS_STARTED_COUNT * 0.8, RT_RAYS_STARTED_COUNT * 0.8,
		RT_RAYS_STARTED_COUNT * 0.8, RT_RAYS_STARTED_COUNT * 0.8 },
	  .tex_cfch_num_l1_ct_operations = {
		TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT * 0.8, TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT * 0.8,
		TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT * 0.8, TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT * 0.8 },
	  .exec_instr_slot1 = {
		EXEC_INSTR_SLOT1_COUNT * 0.8, EXEC_INSTR_SLOT1_COUNT * 0.8,
		EXEC_INSTR_SLOT1_COUNT * 0.8, EXEC_INSTR_SLOT1_COUNT * 0.8 },
	  .exec_issue_slot_any = {
		EXEC_ISSUE_SLOT_ANY_COUNT * 0.8, EXEC_ISSUE_SLOT_ANY_COUNT * 0.8,
		EXEC_ISSUE_SLOT_ANY_COUNT * 0.8, EXEC_ISSUE_SLOT_ANY_COUNT * 0.8 },
	  .interval_ns = INTERVAL_NS,
	  .scale = SCALE,
	  .temp = TEMP,
	  .util = 0.26,
	},

	/* One Operating Performance Point with one core active */
	{ .freq = FREQ_HZ,
	  .voltage = VOLTAGE_MV,
	  .num_l2_slices = 1,
	  .l2_access = { L2_COUNT, },
	  .num_cores = 16,
	  .exec_instr_count = { INSTR_COUNT },
	  .tex_issue = { TEX_ISSUE_COUNT },
	  .tile_wb = { TILE_WB_COUNT },
	  .exec_instr_msg = { EXEC_INSTR_MSG_COUNT },
	  .exec_instr_fma = { EXEC_INSTR_FMA_COUNT },
	  .exec_instr_cvt = { EXEC_INSTR_CVT_COUNT },
	  .exec_instr_sfu = { EXEC_INSTR_SFU_COUNT },
	  .exec_starve_arith = { EXEC_STARVE_ARITH_COUNT },
	  .tex_tfch_clk_stalled = {	TEX_TFCH_CLK_STALLED_COUNT },
	  .tex_filt_num_operations = { TEX_FILT_NUM_OPERATIONS_COUNT },
	  .gpu_active = ACTIVE_COUNT,
	  .idvs_pos_shad_stall = IDVS_POS_SHAD_STALL_COUNT,
	  .prefetch_stall = PREFETCH_STALL_COUNT,
	  .vfetch_pos_read_wait = VFETCH_POS_READ_WAIT_COUNT,
	  .vfetch_vertex_wait = VFETCH_VERTEX_WAIT_COUNT,
	  .primassy_stall = PRIMASSY_STALL_COUNT,
	  .idvs_var_shad_stall = IDVS_VAR_SHAD_STALL_COUNT,
	  .iter_stall = ITER_STALL_COUNT,
	  .pmgr_ptr_rd_stall = PMGR_PTR_RD_STALL_COUNT,
	  .primassy_pos_shader_wait = PRIMASSY_POS_SHADER_WAIT_COUNT,
	  .l2_rd_msg_in_cu = { L2_RD_MSG_IN_CU_COUNT },
	  .l2_rd_msg_in = { L2_RD_MSG_IN_COUNT },
	  .l2_wr_msg_in = { L2_WR_MSG_IN_COUNT },
	  .l2_snp_msg_in = { L2_SNP_MSG_IN_COUNT },
	  .l2_rd_msg_out = { L2_RD_MSG_OUT_COUNT },
	  .l2_read_lookup = { L2_READ_LOOKUP_COUNT },
	  .l2_ext_read_nosnp = { L2_EXT_READ_NOSNP_COUNT },
	  .l2_rd_msg_in_stall = { L2_RD_MSG_IN_STALL_COUNT },
	  .l2_ext_write = { L2_EXT_WRITE_COUNT },
	  .l2_ext_write_nosnp_full = { L2_EXT_WRITE_NOSNP_FULL_COUNT },
	  .ls_mem_read_short = { LS_MEM_READ_SHORT_COUNT },
	  .frag_starving = { FRAG_STARVING_COUNT },
	  .frag_partial_quads_rast = { FRAG_PARTIAL_QUADS_RAST_COUNT },
	  .frag_quads_ezs_update = { FRAG_QUADS_EZS_UPDATE_COUNT },
	  .full_quad_warps = { FULL_QUAD_WARPS_COUNT },
	  .ls_mem_write_short = { LS_MEM_WRITE_SHORT_COUNT },
	  .vary_slot_16 = { VARY_SLOT_16_COUNT },
	  .beats_rd_lsc_ext = { BEATS_RD_LSC_EXT_COUNT },
	  .beats_rd_tex = { BEATS_RD_TEX_COUNT },
	  .beats_rd_tex_ext = { BEATS_RD_TEX_EXT_COUNT },
	  .frag_quads_coarse = { FRAG_QUADS_COARSE_COUNT },
	  .rt_rays_started = { RT_RAYS_STARTED_COUNT },
	  .tex_cfch_num_l1_ct_operations = { TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT },
	  .exec_instr_slot1 = {	EXEC_INSTR_SLOT1_COUNT },
	  .exec_issue_slot_any = { EXEC_ISSUE_SLOT_ANY_COUNT },
	  .interval_ns = INTERVAL_NS,
	  .scale = SCALE,
	  .temp = TEMP,
	  .util = 0.42,
	},

	/* One Operating Performance Point with different activity on each core
	 */
	{ .freq = FREQ_HZ,
	  .voltage = VOLTAGE_MV,
	  .num_l2_slices = 4,
	  .l2_access = { L2_COUNT, 0, 0, L2_COUNT },
	  .num_cores = 32,
	  .exec_instr_count = { INSTR_COUNT, 0, INSTR_COUNT, 0 },
	  .tex_issue = { TEX_ISSUE_COUNT, TEX_ISSUE_COUNT, 0, 0 },
	  .tile_wb = { TILE_WB_COUNT, 0, 0, TILE_WB_COUNT },
	  .exec_instr_msg = { EXEC_INSTR_MSG_COUNT, EXEC_INSTR_MSG_COUNT, 0, 0 },
	  .exec_instr_fma = { EXEC_INSTR_FMA_COUNT, 0, EXEC_INSTR_FMA_COUNT, 0 },
	  .exec_instr_cvt = { EXEC_INSTR_CVT_COUNT, 0, 0, EXEC_INSTR_CVT_COUNT, },
	  .exec_instr_sfu = { EXEC_INSTR_SFU_COUNT, 0, 0, EXEC_INSTR_SFU_COUNT, },
	  .exec_starve_arith = { EXEC_STARVE_ARITH_COUNT, 0, 0, EXEC_STARVE_ARITH_COUNT, },
	  .tex_tfch_clk_stalled = { TEX_TFCH_CLK_STALLED_COUNT, 0, 0, TEX_TFCH_CLK_STALLED_COUNT, },
	  .tex_filt_num_operations = { 0, TEX_FILT_NUM_OPERATIONS_COUNT, 0, TEX_FILT_NUM_OPERATIONS_COUNT },
	  .gpu_active = ACTIVE_COUNT,
	  .idvs_pos_shad_stall = IDVS_POS_SHAD_STALL_COUNT,
	  .prefetch_stall = PREFETCH_STALL_COUNT,
	  .vfetch_pos_read_wait = VFETCH_POS_READ_WAIT_COUNT,
	  .vfetch_vertex_wait = VFETCH_VERTEX_WAIT_COUNT,
	  .primassy_stall = PRIMASSY_STALL_COUNT,
	  .idvs_var_shad_stall = IDVS_VAR_SHAD_STALL_COUNT,
	  .iter_stall = ITER_STALL_COUNT,
	  .pmgr_ptr_rd_stall = PMGR_PTR_RD_STALL_COUNT,
	  .primassy_pos_shader_wait = PRIMASSY_POS_SHADER_WAIT_COUNT,
	  .l2_rd_msg_in_cu = { L2_RD_MSG_IN_CU_COUNT, L2_RD_MSG_IN_CU_COUNT, 0, 0 },
	  .l2_rd_msg_in = { L2_RD_MSG_IN_COUNT, L2_RD_MSG_IN_COUNT, 0, 0 },
	  .l2_wr_msg_in = { L2_WR_MSG_IN_COUNT, 0, L2_WR_MSG_IN_COUNT, 0, },
	  .l2_snp_msg_in = { L2_SNP_MSG_IN_COUNT, 0, L2_SNP_MSG_IN_COUNT, 0, },
	  .l2_rd_msg_out = { L2_RD_MSG_OUT_COUNT, 0, 0, L2_RD_MSG_OUT_COUNT },
	  .l2_read_lookup = { 0, L2_READ_LOOKUP_COUNT, 0, L2_READ_LOOKUP_COUNT },
	  .l2_ext_read_nosnp = { 0, L2_EXT_READ_NOSNP_COUNT, 0, L2_EXT_READ_NOSNP_COUNT },
	  .l2_rd_msg_in_stall = { 0, L2_RD_MSG_IN_STALL_COUNT, L2_RD_MSG_IN_STALL_COUNT, 0 },
	  .l2_ext_write = { 0, L2_EXT_WRITE_COUNT, L2_EXT_WRITE_COUNT, 0 },
	  .l2_ext_write_nosnp_full = { 0, L2_EXT_WRITE_NOSNP_FULL_COUNT, L2_EXT_WRITE_NOSNP_FULL_COUNT, 0 },
	  .ls_mem_read_short = { LS_MEM_READ_SHORT_COUNT, 0, 0, LS_MEM_READ_SHORT_COUNT },
	  .frag_starving = { 0, FRAG_STARVING_COUNT, FRAG_STARVING_COUNT, 0 },
	  .frag_partial_quads_rast = { FRAG_PARTIAL_QUADS_RAST_COUNT, 0, FRAG_PARTIAL_QUADS_RAST_COUNT, 0 },
	  .frag_quads_ezs_update = { 0, 0, FRAG_QUADS_EZS_UPDATE_COUNT, FRAG_QUADS_EZS_UPDATE_COUNT },
	  .full_quad_warps = { FULL_QUAD_WARPS_COUNT, FULL_QUAD_WARPS_COUNT, 0, 0 },
	  .ls_mem_write_short = { LS_MEM_WRITE_SHORT_COUNT, 0, LS_MEM_WRITE_SHORT_COUNT, 0 },
	  .vary_slot_16 = { VARY_SLOT_16_COUNT, VARY_SLOT_16_COUNT, 0, VARY_SLOT_16_COUNT },
	  .beats_rd_lsc_ext = { BEATS_RD_LSC_EXT_COUNT, BEATS_RD_LSC_EXT_COUNT, 0, BEATS_RD_LSC_EXT_COUNT },
	  .beats_rd_tex = { BEATS_RD_TEX_COUNT, BEATS_RD_TEX_COUNT, 0, BEATS_RD_TEX_COUNT },
	  .beats_rd_tex_ext = { BEATS_RD_TEX_EXT_COUNT, BEATS_RD_TEX_EXT_COUNT, 0, BEATS_RD_TEX_EXT_COUNT },
	  .frag_quads_coarse = { FRAG_QUADS_COARSE_COUNT, FRAG_QUADS_COARSE_COUNT, 0, FRAG_QUADS_COARSE_COUNT },
	  .rt_rays_started = { RT_RAYS_STARTED_COUNT, RT_RAYS_STARTED_COUNT, 0, RT_RAYS_STARTED_COUNT },
	  .tex_cfch_num_l1_ct_operations = { TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT,
										 0, TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT },
	  .exec_instr_slot1 = {	EXEC_INSTR_SLOT1_COUNT, EXEC_INSTR_SLOT1_COUNT, 0, EXEC_INSTR_SLOT1_COUNT },
	  .exec_issue_slot_any = { EXEC_ISSUE_SLOT_ANY_COUNT, EXEC_ISSUE_SLOT_ANY_COUNT, 0, EXEC_ISSUE_SLOT_ANY_COUNT },
	  .interval_ns = INTERVAL_NS,
	  .scale = SCALE,
	  .temp = TEMP,
	  .util = 0.84,
	},

	/* For testing really high counter values by scaling the real world
	 * values for the max GPU frequency and full utilization over the max
	 * sampling interval. The aggregate of the values for some of the shader
	 * core counters, that were derived at 50 MHz over a period of 100 ms,
	 * exceeds 32 bits after the scaling.
	 */
	{ .freq = MAX_FREQ_HZ,
	  .voltage = VOLTAGE_MV,
	  .num_l2_slices = 4,
	  .l2_access = { DOUBLE_SCALE(L2_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .num_cores = 8,
	  .exec_instr_count = { DOUBLE_SCALE(INSTR_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .tex_issue = { DOUBLE_SCALE(TEX_ISSUE_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .tile_wb = { DOUBLE_SCALE(TILE_WB_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .exec_instr_msg = { DOUBLE_SCALE(EXEC_INSTR_MSG_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .exec_instr_fma = { DOUBLE_SCALE(EXEC_INSTR_FMA_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .exec_instr_cvt = { DOUBLE_SCALE(EXEC_INSTR_CVT_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .exec_instr_sfu = { DOUBLE_SCALE(EXEC_INSTR_SFU_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .exec_starve_arith = { DOUBLE_SCALE(EXEC_STARVE_ARITH_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .tex_tfch_clk_stalled = { DOUBLE_SCALE(TEX_TFCH_CLK_STALLED_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .tex_filt_num_operations = { DOUBLE_SCALE(TEX_FILT_NUM_OPERATIONS_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .gpu_active = DOUBLE_SCALE(ACTIVE_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS),
	  .idvs_pos_shad_stall = DOUBLE_SCALE(IDVS_POS_SHAD_STALL_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS),
	  .prefetch_stall = DOUBLE_SCALE(PREFETCH_STALL_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS),
	  .vfetch_pos_read_wait = DOUBLE_SCALE(VFETCH_POS_READ_WAIT_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS),
	  .vfetch_vertex_wait = DOUBLE_SCALE(VFETCH_VERTEX_WAIT_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS),
	  .primassy_stall = DOUBLE_SCALE(PRIMASSY_STALL_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS),
	  .idvs_var_shad_stall = DOUBLE_SCALE(IDVS_VAR_SHAD_STALL_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS),
	  .iter_stall = DOUBLE_SCALE(ITER_STALL_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS),
	  .pmgr_ptr_rd_stall = DOUBLE_SCALE(PMGR_PTR_RD_STALL_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS),
	  .primassy_pos_shader_wait = DOUBLE_SCALE(PRIMASSY_POS_SHADER_WAIT_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS),
	  .l2_rd_msg_in_cu = { DOUBLE_SCALE(L2_RD_MSG_IN_CU_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .l2_rd_msg_in = { DOUBLE_SCALE(L2_RD_MSG_IN_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .l2_wr_msg_in = { DOUBLE_SCALE(L2_WR_MSG_IN_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .l2_snp_msg_in = { DOUBLE_SCALE(L2_SNP_MSG_IN_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .l2_rd_msg_out = { DOUBLE_SCALE(L2_RD_MSG_OUT_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .l2_read_lookup = { DOUBLE_SCALE(L2_READ_LOOKUP_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .l2_ext_read_nosnp = { DOUBLE_SCALE(L2_EXT_READ_NOSNP_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .l2_rd_msg_in_stall = { DOUBLE_SCALE(L2_RD_MSG_IN_STALL_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .l2_ext_write = { DOUBLE_SCALE(L2_EXT_WRITE_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .l2_ext_write_nosnp_full = { DOUBLE_SCALE(L2_EXT_WRITE_NOSNP_FULL_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .ls_mem_read_short = { DOUBLE_SCALE(LS_MEM_READ_SHORT_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .frag_starving = { DOUBLE_SCALE(FRAG_STARVING_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .frag_partial_quads_rast = { DOUBLE_SCALE(FRAG_PARTIAL_QUADS_RAST_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .frag_quads_ezs_update = { DOUBLE_SCALE(FRAG_QUADS_EZS_UPDATE_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .full_quad_warps = { DOUBLE_SCALE(FULL_QUAD_WARPS_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .ls_mem_write_short = { DOUBLE_SCALE(LS_MEM_WRITE_SHORT_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .vary_slot_16 = { DOUBLE_SCALE(VARY_SLOT_16_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .beats_rd_lsc_ext = { DOUBLE_SCALE(BEATS_RD_LSC_EXT_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .beats_rd_tex = { DOUBLE_SCALE(BEATS_RD_TEX_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .beats_rd_tex_ext = { DOUBLE_SCALE(BEATS_RD_TEX_EXT_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .frag_quads_coarse = { DOUBLE_SCALE(FRAG_QUADS_COARSE_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .rt_rays_started = { DOUBLE_SCALE(RT_RAYS_STARTED_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .tex_cfch_num_l1_ct_operations = { DOUBLE_SCALE(TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .exec_instr_slot1 = { DOUBLE_SCALE(EXEC_INSTR_SLOT1_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .exec_issue_slot_any = { DOUBLE_SCALE(EXEC_ISSUE_SLOT_ANY_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .interval_ns = MAX_INTERVAL_NS,
	  .scale = SCALE,
	  .temp = TEMP,
	  .util = 0.99,
	},

	/* Use max number of L2 slices and cores */
	{ .freq = MAX_FREQ_HZ,
	  .voltage = VOLTAGE_MV,
	  .num_l2_slices = KBASE_DUMMY_MODEL_MAX_MEMSYS_BLOCKS,
	  .l2_access = { DOUBLE_SCALE(L2_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .num_cores = KBASE_DUMMY_MODEL_MAX_SHADER_CORES,
	  .exec_instr_count = { DOUBLE_SCALE(INSTR_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .tex_issue = { DOUBLE_SCALE(TEX_ISSUE_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .tile_wb = { DOUBLE_SCALE(TILE_WB_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .exec_instr_msg = { DOUBLE_SCALE(EXEC_INSTR_MSG_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .exec_instr_fma = { DOUBLE_SCALE(EXEC_INSTR_FMA_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .exec_instr_cvt = { DOUBLE_SCALE(EXEC_INSTR_CVT_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .exec_instr_sfu = { DOUBLE_SCALE(EXEC_INSTR_SFU_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .exec_starve_arith = { DOUBLE_SCALE(EXEC_STARVE_ARITH_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .tex_tfch_clk_stalled = { DOUBLE_SCALE(TEX_TFCH_CLK_STALLED_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .tex_filt_num_operations = { DOUBLE_SCALE(TEX_FILT_NUM_OPERATIONS_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .gpu_active = DOUBLE_SCALE(ACTIVE_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS),
	  .idvs_pos_shad_stall = DOUBLE_SCALE(IDVS_POS_SHAD_STALL_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS),
	  .prefetch_stall = DOUBLE_SCALE(PREFETCH_STALL_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS),
	  .vfetch_pos_read_wait = DOUBLE_SCALE(VFETCH_POS_READ_WAIT_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS),
	  .vfetch_vertex_wait = DOUBLE_SCALE(VFETCH_VERTEX_WAIT_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS),
	  .primassy_stall = DOUBLE_SCALE(PRIMASSY_STALL_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS),
	  .idvs_var_shad_stall = DOUBLE_SCALE(IDVS_VAR_SHAD_STALL_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS),
	  .iter_stall = DOUBLE_SCALE(ITER_STALL_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS),
	  .pmgr_ptr_rd_stall = DOUBLE_SCALE(PMGR_PTR_RD_STALL_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS),
	  .primassy_pos_shader_wait = DOUBLE_SCALE(PRIMASSY_POS_SHADER_WAIT_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS),
	  .l2_rd_msg_in_cu = { DOUBLE_SCALE(L2_RD_MSG_IN_CU_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .l2_rd_msg_in = { DOUBLE_SCALE(L2_RD_MSG_IN_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .l2_wr_msg_in = { DOUBLE_SCALE(L2_WR_MSG_IN_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .l2_snp_msg_in = { DOUBLE_SCALE(L2_SNP_MSG_IN_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .l2_rd_msg_out = { DOUBLE_SCALE(L2_RD_MSG_OUT_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .l2_read_lookup = { DOUBLE_SCALE(L2_READ_LOOKUP_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .l2_ext_read_nosnp = { DOUBLE_SCALE(L2_EXT_READ_NOSNP_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .l2_rd_msg_in_stall = { DOUBLE_SCALE(L2_RD_MSG_IN_STALL_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .l2_ext_write = { DOUBLE_SCALE(L2_EXT_WRITE_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .l2_ext_write_nosnp_full = { DOUBLE_SCALE(L2_EXT_WRITE_NOSNP_FULL_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .ls_mem_read_short = { DOUBLE_SCALE(LS_MEM_READ_SHORT_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .frag_starving = { DOUBLE_SCALE(FRAG_STARVING_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .frag_partial_quads_rast = { DOUBLE_SCALE(FRAG_PARTIAL_QUADS_RAST_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .frag_quads_ezs_update = { DOUBLE_SCALE(FRAG_QUADS_EZS_UPDATE_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .full_quad_warps = { DOUBLE_SCALE(FULL_QUAD_WARPS_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .ls_mem_write_short = { DOUBLE_SCALE(LS_MEM_WRITE_SHORT_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .vary_slot_16 = { DOUBLE_SCALE(VARY_SLOT_16_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .beats_rd_lsc_ext = { DOUBLE_SCALE(BEATS_RD_LSC_EXT_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .beats_rd_tex = { DOUBLE_SCALE(BEATS_RD_TEX_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .beats_rd_tex_ext = { DOUBLE_SCALE(BEATS_RD_TEX_EXT_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .frag_quads_coarse = { DOUBLE_SCALE(FRAG_QUADS_COARSE_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .rt_rays_started = { DOUBLE_SCALE(RT_RAYS_STARTED_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .tex_cfch_num_l1_ct_operations = { DOUBLE_SCALE(TEX_CFCH_NUM_L1_CT_OPERATIONS_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .exec_instr_slot1 = { DOUBLE_SCALE(EXEC_INSTR_SLOT1_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .exec_issue_slot_any = { DOUBLE_SCALE(EXEC_ISSUE_SLOT_ANY_COUNT, MAX_FREQ_HZ, MAX_INTERVAL_NS) },
	  .interval_ns = MAX_INTERVAL_NS,
	  .scale = SCALE,
	  .temp = TEMP,
	  .util = 0.99,
	},
};

/* Compare the test's floating-point calculation 'reference' with the result
 * returned by the kernel. Both should be in mW.
 */
static void check_error(double reference, uint64_t kernel)
{
	mali_utf_loginf("Reference = %g mW, kernel = %" PRIu64 " mW\n", reference, kernel);

	const double diff = fabs(reference - (double)kernel);
	const double error = (diff / reference) * 100.0;

	mali_utf_loginf("Difference of %g mW or %g%%\n", diff, error);

	if (diff > MAX_ABSOLUTE_ERROR_MW) {
		MALI_UTF_ASSERT_DOUBLE_LTE(error, MAX_RELATIVE_ERROR);
	}
}

/* All of these tests share the same userdata fixture */
static int ipa_pretest(struct mali_utf_suite *suite)
{
	struct kutf_fixture_data *const fix = suite->fixture;

	fix->extra_funcs_data = &opps[suite->fixture_index % NELEMS(opps)];
	return 0; /* success */
}

static int send_array_elem(const uint64_t *const val, struct mali_utf_suite *const suite,
			   const char *const name, const size_t index)
{
	char var_name[KUTF_TEST_HELPERS_MAX_VAL_NAME_LEN + 1];
	const int n = snprintf(var_name, sizeof(var_name), "%s_%zu", name, index);
	int err;

	MALI_UTF_ASSERT_INT_GTE_M(n, 0, "Bad format string");
	MALI_UTF_ASSERT_INT_LT_M(n, sizeof(var_name), "String buffer overflow");
	if (n < 0 || (size_t)n >= sizeof(var_name)) {
		err = 1;
	} else {
		mali_utf_loginf("%s=%" PRIu64 "\n", var_name, val[index]);
		err = kutf_test_helpers_userdata_send_named_u64(suite, var_name, val[index]);
	}
	return err;
}

static int ipa_send_values(struct mali_utf_suite *const suite)
{
	struct kutf_fixture_data *const fix = suite->fixture;
	struct ipa_fixture *const ipa_fix = fix->extra_funcs_data;
	int err;

	mali_utf_logdbg("Sending values to kernel space\n");

	/* Send frequency value to be tested */
	mali_utf_loginf(IPA_IN_NAME_FREQ "=%lu\n", ipa_fix->freq);
	err = kutf_test_helpers_userdata_send_named_u64(suite, IPA_IN_NAME_FREQ, ipa_fix->freq);

	/* Send voltage value to be tested */
	if (!err) {
		mali_utf_loginf(IPA_IN_NAME_VOLTAGE "=%lu\n", ipa_fix->voltage);
		err = kutf_test_helpers_userdata_send_named_u64(suite, IPA_IN_NAME_VOLTAGE,
								ipa_fix->voltage);
	}

	/* Send number of L2 slices value to be tested */
	if (!err) {
		mali_utf_loginf(IPA_IN_NAME_NUM_L2_SLICES "=%u\n", ipa_fix->num_l2_slices);
		err = kutf_test_helpers_userdata_send_named_u64(suite, IPA_IN_NAME_NUM_L2_SLICES,
								ipa_fix->num_l2_slices);
	}

	/* Send number of cores value to be tested */
	if (!err) {
		mali_utf_loginf(IPA_IN_NAME_NUM_CORES "=%u\n", ipa_fix->num_cores);
		err = kutf_test_helpers_userdata_send_named_u64(suite, IPA_IN_NAME_NUM_CORES,
								ipa_fix->num_cores);
	}

	/* Send performance counter values to be tested */
	for (size_t i = 0; i < ipa_fix->num_l2_slices && !err; ++i) {
		err = send_array_elem(ipa_fix->l2_access, suite, IPA_IN_NAME_L2_ACCESS, i);

		if (!err) {
			err = send_array_elem(ipa_fix->l2_rd_msg_in_cu, suite,
					      IPA_IN_NAME_L2_RD_MSG_IN_CU, i);
		}

		if (!err) {
			err = send_array_elem(ipa_fix->l2_rd_msg_in, suite,
					      IPA_IN_NAME_L2_RD_MSG_IN, i);
		}

		if (!err) {
			err = send_array_elem(ipa_fix->l2_wr_msg_in, suite,
					      IPA_IN_NAME_L2_WR_MSG_IN, i);
		}

		if (!err) {
			err = send_array_elem(ipa_fix->l2_snp_msg_in, suite,
					      IPA_IN_NAME_L2_SNP_MSG_IN, i);
		}

		if (!err) {
			err = send_array_elem(ipa_fix->l2_rd_msg_out, suite,
					      IPA_IN_NAME_L2_RD_MSG_OUT, i);
		}

		if (!err) {
			err = send_array_elem(ipa_fix->l2_read_lookup, suite,
					      IPA_IN_NAME_L2_READ_LOOKUP, i);
		}

		if (!err) {
			err = send_array_elem(ipa_fix->l2_ext_read_nosnp, suite,
					      IPA_IN_NAME_L2_EXT_READ_NOSNP, i);
		}
		if (!err) {
			err = send_array_elem(ipa_fix->l2_rd_msg_in_stall, suite,
					      IPA_IN_NAME_L2_RD_MSG_IN_STALL, i);
		}
		if (!err) {
			err = send_array_elem(ipa_fix->l2_ext_write, suite,
					      IPA_IN_NAME_L2_EXT_WRITE, i);
		}
		if (!err) {
			err = send_array_elem(ipa_fix->l2_ext_write_nosnp_full, suite,
					      IPA_IN_NAME_L2_EXT_WRITE_NOSNP_FULL, i);
		}
	}

	for (size_t i = 0; i < ipa_fix->num_cores && !err; ++i) {
		err = send_array_elem(ipa_fix->exec_instr_count, suite,
				      IPA_IN_NAME_EXEC_INSTR_COUNT, i);

		if (!err) {
			err = send_array_elem(ipa_fix->tex_issue, suite, IPA_IN_NAME_TEX_ISSUE, i);
		}

		if (!err) {
			err = send_array_elem(ipa_fix->tile_wb, suite, IPA_IN_NAME_TILE_WB, i);
		}

		if (!err) {
			err = send_array_elem(ipa_fix->tex_tfch_num_operations, suite,
					      IPA_IN_NAME_TEX_TFCH_NUM_OPERATIONS, i);
		}

		if (!err) {
			err = send_array_elem(ipa_fix->vary_instr, suite, IPA_IN_NAME_VARY_INSTR,
					      i);
		}

		if (!err) {
			err = send_array_elem(ipa_fix->exec_instr_msg, suite,
					      IPA_IN_NAME_EXEC_INSTR_MSG, i);
		}

		if (!err) {
			err = send_array_elem(ipa_fix->exec_instr_fma, suite,
					      IPA_IN_NAME_EXEC_INSTR_FMA, i);
		}

		if (!err) {
			err = send_array_elem(ipa_fix->exec_instr_cvt, suite,
					      IPA_IN_NAME_EXEC_INSTR_CVT, i);
		}

		if (!err) {
			err = send_array_elem(ipa_fix->exec_instr_sfu, suite,
					      IPA_IN_NAME_EXEC_INSTR_SFU, i);
		}
		if (!err) {
			err = send_array_elem(ipa_fix->exec_starve_arith, suite,
					      IPA_IN_NAME_EXEC_STARVE_ARITH, i);
		}

		if (!err) {
			err = send_array_elem(ipa_fix->tex_tfch_clk_stalled, suite,
					      IPA_IN_NAME_TEX_TFCH_CLK_STALLED, i);
		}
		if (!err) {
			err = send_array_elem(ipa_fix->tex_filt_num_operations, suite,
					      IPA_IN_NAME_TEX_FILT_NUM_OPERATIONS, i);
		}

		if (!err) {
			err = send_array_elem(ipa_fix->ls_mem_read_short, suite,
					      IPA_IN_NAME_LS_MEM_READ_SHORT, i);
		}

		if (!err) {
			err = send_array_elem(ipa_fix->frag_starving, suite,
					      IPA_IN_NAME_FRAG_STARVING, i);
		}

		if (!err) {
			err = send_array_elem(ipa_fix->frag_partial_quads_rast, suite,
					      IPA_IN_NAME_FRAG_PARTIAL_QUADS_RAST, i);
		}

		if (!err) {
			err = send_array_elem(ipa_fix->frag_quads_ezs_update, suite,
					      IPA_IN_NAME_FRAG_QUADS_EZS_UPDATE, i);
		}

		if (!err) {
			err = send_array_elem(ipa_fix->full_quad_warps, suite,
					      IPA_IN_NAME_FULL_QUAD_WARPS, i);
		}

		if (!err) {
			err = send_array_elem(ipa_fix->ls_mem_write_short, suite,
					      IPA_IN_NAME_LS_MEM_WRITE_SHORT, i);
		}

		if (!err) {
			err = send_array_elem(ipa_fix->vary_slot_16, suite,
					      IPA_IN_NAME_VARY_SLOT_16, i);
		}

		if (!err) {
			err = send_array_elem(ipa_fix->beats_rd_lsc_ext, suite,
					      IPA_IN_NAME_BEATS_RD_LSC_EXT, i);
		}

		if (!err) {
			err = send_array_elem(ipa_fix->beats_rd_tex, suite,
					      IPA_IN_NAME_BEATS_RD_TEX, i);
		}

		if (!err) {
			err = send_array_elem(ipa_fix->beats_rd_tex_ext, suite,
					      IPA_IN_NAME_BEATS_RD_TEX_EXT, i);
		}

		if (!err) {
			err = send_array_elem(ipa_fix->frag_quads_coarse, suite,
					      IPA_IN_NAME_FRAG_QUADS_COARSE, i);
		}
		if (!err) {
			err = send_array_elem(ipa_fix->rt_rays_started, suite,
					      IPA_IN_NAME_RT_RAYS_STARTED, i);
		}

		if (!err) {
			err = send_array_elem(ipa_fix->tex_cfch_num_l1_ct_operations, suite,
					      IPA_IN_NAME_TEX_CFCH_NUM_L1_CT_OPERATIONS, i);
		}

		if (!err) {
			err = send_array_elem(ipa_fix->exec_instr_slot1, suite,
					      IPA_IN_NAME_EXEC_INSTR_SLOT1, i);
		}

		if (!err) {
			err = send_array_elem(ipa_fix->exec_issue_slot_any, suite,
					      IPA_IN_NAME_EXEC_ISSUE_SLOT_ANY, i);
		}
	}

	if (!err) {
		mali_utf_loginf(IPA_IN_NAME_GPU_ACTIVE "=%" PRIu64 "\n", ipa_fix->gpu_active);
		err = kutf_test_helpers_userdata_send_named_u64(suite, IPA_IN_NAME_GPU_ACTIVE,
								ipa_fix->gpu_active);
	}

	if (!err) {
		mali_utf_loginf(IPA_IN_NAME_IDVS_POS_SHAD_STALL "=%" PRIu64 "\n",
				ipa_fix->idvs_pos_shad_stall);
		err = kutf_test_helpers_userdata_send_named_u64(
			suite, IPA_IN_NAME_IDVS_POS_SHAD_STALL, ipa_fix->idvs_pos_shad_stall);
	}

	if (!err) {
		mali_utf_loginf(IPA_IN_NAME_PREFETCH_STALL "=%" PRIu64 "\n",
				ipa_fix->prefetch_stall);
		err = kutf_test_helpers_userdata_send_named_u64(suite, IPA_IN_NAME_PREFETCH_STALL,
								ipa_fix->prefetch_stall);
	}

	if (!err) {
		mali_utf_loginf(IPA_IN_NAME_VFETCH_POS_READ_WAIT "=%" PRIu64 "\n",
				ipa_fix->vfetch_pos_read_wait);
		err = kutf_test_helpers_userdata_send_named_u64(
			suite, IPA_IN_NAME_VFETCH_POS_READ_WAIT, ipa_fix->vfetch_pos_read_wait);
	}

	if (!err) {
		mali_utf_loginf(IPA_IN_NAME_VFETCH_VERTEX_WAIT "=%" PRIu64 "\n",
				ipa_fix->vfetch_vertex_wait);
		err = kutf_test_helpers_userdata_send_named_u64(
			suite, IPA_IN_NAME_VFETCH_VERTEX_WAIT, ipa_fix->vfetch_vertex_wait);
	}

	if (!err) {
		mali_utf_loginf(IPA_IN_NAME_PRIMASSY_STALL "=%" PRIu64 "\n",
				ipa_fix->primassy_stall);
		err = kutf_test_helpers_userdata_send_named_u64(suite, IPA_IN_NAME_PRIMASSY_STALL,
								ipa_fix->primassy_stall);
	}

	if (!err) {
		mali_utf_loginf(IPA_IN_NAME_IDVS_VAR_SHAD_STALL "=%" PRIu64 "\n",
				ipa_fix->idvs_var_shad_stall);
		err = kutf_test_helpers_userdata_send_named_u64(
			suite, IPA_IN_NAME_IDVS_VAR_SHAD_STALL, ipa_fix->idvs_var_shad_stall);
	}

	if (!err) {
		mali_utf_loginf(IPA_IN_NAME_ITER_STALL "=%" PRIu64 "\n", ipa_fix->iter_stall);
		err = kutf_test_helpers_userdata_send_named_u64(suite, IPA_IN_NAME_ITER_STALL,
								ipa_fix->iter_stall);
	}

	if (!err) {
		mali_utf_loginf(IPA_IN_NAME_PMGR_PTR_RD_STALL "=%" PRIu64 "\n",
				ipa_fix->pmgr_ptr_rd_stall);
		err = kutf_test_helpers_userdata_send_named_u64(
			suite, IPA_IN_NAME_PMGR_PTR_RD_STALL, ipa_fix->pmgr_ptr_rd_stall);
	}
	if (!err) {
		mali_utf_loginf(IPA_IN_NAME_PRIMASSY_POS_SHADER_WAIT "=%" PRIu64 "\n",
				ipa_fix->primassy_pos_shader_wait);
		err = kutf_test_helpers_userdata_send_named_u64(
			suite, IPA_IN_NAME_PRIMASSY_POS_SHADER_WAIT,
			ipa_fix->primassy_pos_shader_wait);
	}
	if (!err) {
		mali_utf_loginf(IPA_IN_NAME_INTERVAL "=%" PRIu64 "\n", ipa_fix->interval_ns);
		err = kutf_test_helpers_userdata_send_named_u64(suite, IPA_IN_NAME_INTERVAL,
								ipa_fix->interval_ns);
	}

	if (!err) {
		mali_utf_loginf(IPA_IN_NAME_TEMP "=%d\n", ipa_fix->temp);
		err = kutf_test_helpers_userdata_send_named_u64(suite, IPA_IN_NAME_TEMP,
								ipa_fix->temp);
	}

	if (!err) {
		uint64_t busy_time_ns = ipa_fix->interval_ns * ipa_fix->util;
		mali_utf_loginf(IPA_IN_NAME_BUSY_TIME "=%" PRIu64 "\n", busy_time_ns);
		err = kutf_test_helpers_userdata_send_named_u64(suite, IPA_IN_NAME_BUSY_TIME,
								busy_time_ns);
	}

	if (!err) {
		mali_utf_loginf(IPA_IN_NAME_SCALE "=%" PRIu32 "\n", ipa_fix->scale);
		err = kutf_test_helpers_userdata_send_named_u64(suite, IPA_IN_NAME_SCALE,
								ipa_fix->scale);
	}

	if (!err) {
		for (size_t i = 0; i < 4 && !err; i++) {
			err = send_array_elem((uint64_t *)ts, suite, IPA_IN_NAME_TS, i);
		}
	}
	if (!err) {
		err = kutf_test_helpers_userdata_send_named_u64(
			suite, IPA_IN_NAME_STATIC_COEFFICIENT,
			(uint64_t)(STATIC_COEFF * 1000000.0));
	}
	if (!err) {
		err = kutf_test_helpers_userdata_send_named_u64(
			suite, IPA_IN_NAME_DYNAMIC_COEFFICIENT,
			(uint64_t)(DYNAMIC_COEFF * 1000000000000.0));
	}

	return err;
}

/* Computes the sum of a counter value for a given number of shader cores.
 */
static uint64_t sum_of_cores(const uint64_t *const counter, const size_t ncores)
{
	uint64_t sum = 0;

	for (size_t i = 0; i < ncores; ++i) {
		sum += counter[i];
	}
	return sum;
}

static bool is_csf(struct mali_utf_suite *suite)
{
	if ((strcmp(suite->name, IPA_SUITE_NAME_G71) == 0) ||
	    (strcmp(suite->name, IPA_SUITE_NAME_G72) == 0) ||
	    (strcmp(suite->name, IPA_SUITE_NAME_TNOX) == 0) ||
	    (strcmp(suite->name, IPA_SUITE_NAME_TGOX_R1) == 0) ||
	    (strcmp(suite->name, IPA_SUITE_NAME_G51) == 0) ||
	    (strcmp(suite->name, IPA_SUITE_NAME_G77) == 0) ||
	    (strcmp(suite->name, IPA_SUITE_NAME_TNAX) == 0) ||
	    (strcmp(suite->name, IPA_SUITE_NAME_TBEX) == 0))
		return false;

	return true;
}

static int num_block_types(struct mali_utf_suite *suite)
{
	return is_csf(suite) ? KBASE_IPA_BLOCK_TYPE_NUM : 1;
}

static void init_array(uint64_t *counter, const size_t ncores)
{
	const uint64_t init_value = counter[0];

	for (size_t i = 1; i < ncores; i++)
		counter[i] = init_value;
}

static void update_fixture_values(struct ipa_fixture *ipa_fix)
{
	if ((ipa_fix->freq == MAX_FREQ_HZ) && (ipa_fix->interval_ns == MAX_INTERVAL_NS)) {
		/* Only 1st element of the counters array is initialized in the
		 * fixtures list. Copy the same value to rest of the elements in
		 * the array. This way less lines of code get used.
		 */
		init_array(ipa_fix->exec_instr_sfu, ipa_fix->num_cores);
		init_array(ipa_fix->exec_instr_fma, ipa_fix->num_cores);
		init_array(ipa_fix->tex_filt_num_operations, ipa_fix->num_cores);
		init_array(ipa_fix->ls_mem_read_short, ipa_fix->num_cores);
		init_array(ipa_fix->frag_quads_ezs_update, ipa_fix->num_cores);
		init_array(ipa_fix->ls_mem_write_short, ipa_fix->num_cores);
		init_array(ipa_fix->vary_slot_16, ipa_fix->num_cores);
		init_array(ipa_fix->beats_rd_lsc_ext, ipa_fix->num_cores);
		init_array(ipa_fix->beats_rd_tex, ipa_fix->num_cores);
		init_array(ipa_fix->beats_rd_tex_ext, ipa_fix->num_cores);
		init_array(ipa_fix->frag_quads_coarse, ipa_fix->num_cores);
		init_array(ipa_fix->exec_starve_arith, ipa_fix->num_cores);
		init_array(ipa_fix->tex_tfch_clk_stalled, ipa_fix->num_cores);
		init_array(ipa_fix->rt_rays_started, ipa_fix->num_cores);
		init_array(ipa_fix->tex_cfch_num_l1_ct_operations, ipa_fix->num_cores);
		init_array(ipa_fix->exec_instr_slot1, ipa_fix->num_cores);
		init_array(ipa_fix->exec_issue_slot_any, ipa_fix->num_cores);

		init_array(ipa_fix->l2_rd_msg_in_cu, ipa_fix->num_l2_slices);
		init_array(ipa_fix->l2_rd_msg_in, ipa_fix->num_l2_slices);
		init_array(ipa_fix->l2_snp_msg_in, ipa_fix->num_l2_slices);
		init_array(ipa_fix->l2_ext_read_nosnp, ipa_fix->num_l2_slices);
		init_array(ipa_fix->l2_rd_msg_in_stall, ipa_fix->num_l2_slices);
		init_array(ipa_fix->l2_ext_write, ipa_fix->num_l2_slices);
		init_array(ipa_fix->l2_ext_write_nosnp_full, ipa_fix->num_l2_slices);
	}
}

static void ipa_sample_dummy_thread_g7x(struct mali_utf_suite *suite)
{
	struct kutf_fixture_data *const fix = suite->fixture;
	struct ipa_fixture *ipa_fix = fix->extra_funcs_data;
	int err;
	struct kutf_test_helpers_named_val power;

	update_fixture_values(ipa_fix);
	err = ipa_send_values(suite);

	/* Receive power value */
	if (!err)
		err = kutf_test_helpers_userdata_receive_check_val(
			&power, suite, IPA_OUT_NAME_POWER, KUTF_TEST_HELPERS_VALTYPE_U64);

	if (!err) {
		/* Calculate expected dynamic power value. */
		const uint64_t total_exec_instr_count =
			sum_of_cores(ipa_fix->exec_instr_count, ipa_fix->num_cores);

		const uint64_t total_tex_issue =
			sum_of_cores(ipa_fix->tex_issue, ipa_fix->num_cores);

		const uint64_t total_tile_wb = sum_of_cores(ipa_fix->tile_wb, ipa_fix->num_cores);

		const uint64_t total_tex_tfch_num_operations =
			sum_of_cores(ipa_fix->tex_tfch_num_operations, ipa_fix->num_cores);

		const uint64_t total_vary_instr =
			sum_of_cores(ipa_fix->vary_instr, ipa_fix->num_cores);

		const uint64_t total_l2_access =
			sum_of_cores(ipa_fix->l2_access, ipa_fix->num_l2_slices);

		const uint64_t total_exec_instr_msg =
			sum_of_cores(ipa_fix->exec_instr_msg, ipa_fix->num_cores);

		const uint64_t total_exec_instr_fma =
			sum_of_cores(ipa_fix->exec_instr_fma, ipa_fix->num_cores);

		const uint64_t total_tex_filt_num_operations =
			sum_of_cores(ipa_fix->tex_filt_num_operations, ipa_fix->num_cores);

		const uint64_t total_l2_rd_msg_in_cu =
			sum_of_cores(ipa_fix->l2_rd_msg_in_cu, ipa_fix->num_l2_slices);

		const uint64_t total_l2_rd_msg_in =
			sum_of_cores(ipa_fix->l2_rd_msg_in, ipa_fix->num_l2_slices);

		const uint64_t total_l2_wr_msg_in =
			sum_of_cores(ipa_fix->l2_wr_msg_in, ipa_fix->num_l2_slices);

		const uint64_t total_l2_snp_msg_in =
			sum_of_cores(ipa_fix->l2_snp_msg_in, ipa_fix->num_l2_slices);

		const uint64_t total_l2_rd_msg_out =
			sum_of_cores(ipa_fix->l2_rd_msg_out, ipa_fix->num_l2_slices);

		const uint64_t total_l2_read_lookup =
			sum_of_cores(ipa_fix->l2_read_lookup, ipa_fix->num_l2_slices);

		const uint64_t total_l2_ext_read_nosnp =
			sum_of_cores(ipa_fix->l2_ext_read_nosnp, ipa_fix->num_l2_slices);
		const uint64_t total_l2_rd_msg_in_stall =
			sum_of_cores(ipa_fix->l2_rd_msg_in_stall, ipa_fix->num_l2_slices);

		const uint64_t total_l2_ext_write =
			sum_of_cores(ipa_fix->l2_ext_write, ipa_fix->num_l2_slices);
		const uint64_t total_l2_ext_write_nosnp_full =
			sum_of_cores(ipa_fix->l2_ext_write_nosnp_full, ipa_fix->num_l2_slices);

		const uint64_t total_ls_mem_read_short =
			sum_of_cores(ipa_fix->ls_mem_read_short, ipa_fix->num_cores);

		const uint64_t total_frag_starving =
			sum_of_cores(ipa_fix->frag_starving, ipa_fix->num_cores);

		const uint64_t total_frag_partial_quads_rast =
			sum_of_cores(ipa_fix->frag_partial_quads_rast, ipa_fix->num_cores);

		const uint64_t total_frag_quads_ezs_update =
			sum_of_cores(ipa_fix->frag_quads_ezs_update, ipa_fix->num_cores);

		const uint64_t total_full_quad_warps =
			sum_of_cores(ipa_fix->full_quad_warps, ipa_fix->num_cores);

		const uint64_t total_exec_instr_cvt =
			sum_of_cores(ipa_fix->exec_instr_cvt, ipa_fix->num_cores);

		const uint64_t total_exec_instr_sfu =
			sum_of_cores(ipa_fix->exec_instr_sfu, ipa_fix->num_cores);

		const uint64_t total_exec_starve_arith =
			sum_of_cores(ipa_fix->exec_starve_arith, ipa_fix->num_cores);
		const uint64_t total_tex_tfch_clk_stalled =
			sum_of_cores(ipa_fix->tex_tfch_clk_stalled, ipa_fix->num_cores);
		const uint64_t total_rt_rays_started =
			sum_of_cores(ipa_fix->rt_rays_started, ipa_fix->num_cores);
		const uint64_t total_tex_cfch_num_l1_ct_operations =
			sum_of_cores(ipa_fix->tex_cfch_num_l1_ct_operations, ipa_fix->num_cores);
		const uint64_t total_exec_instr_slot1 =
			sum_of_cores(ipa_fix->exec_instr_slot1, ipa_fix->num_cores);
		const uint64_t total_exec_issue_slot_any =
			sum_of_cores(ipa_fix->exec_issue_slot_any, ipa_fix->num_cores);

		const uint64_t total_ls_mem_write_short =
			sum_of_cores(ipa_fix->ls_mem_write_short, ipa_fix->num_cores);

		const uint64_t total_vary_slot_16 =
			sum_of_cores(ipa_fix->vary_slot_16, ipa_fix->num_cores);

		const uint64_t total_beats_rd_lsc_ext =
			sum_of_cores(ipa_fix->beats_rd_lsc_ext, ipa_fix->num_cores);

		const uint64_t total_beats_rd_tex =
			sum_of_cores(ipa_fix->beats_rd_tex, ipa_fix->num_cores);

		const uint64_t total_beats_rd_tex_ext =
			sum_of_cores(ipa_fix->beats_rd_tex_ext, ipa_fix->num_cores);

		const uint64_t total_frag_quads_coarse =
			sum_of_cores(ipa_fix->frag_quads_coarse, ipa_fix->num_cores);

		double Pdyn_total = 0;
		const int block_types = num_block_types(suite);

		/* Voltage is in millivolts (1000ths of a volt)
		 * not volts.
		 */
		const double V = (double)ipa_fix->voltage / 1000;

		for (int i = 0; i < block_types; i++) {
			double energy = 0, Pdyn;
			double reference_voltage = G71_REF_VOLTAGE;

			if (strcmp(suite->name, IPA_SUITE_NAME_G71) == 0) {
				/* reference_voltage already set as default case above */
				energy = (G71_C1 * total_l2_access) +
					 (G71_C2 * total_exec_instr_count) +
					 (G71_C3 * total_tex_issue) + (G71_C4 * total_tile_wb) +
					 (G71_C5 * ipa_fix->gpu_active);
			} else if (strcmp(suite->name, IPA_SUITE_NAME_G72) == 0) {
				energy = (G72_C1 * total_l2_access) +
					 (G72_C2 * total_exec_instr_count) +
					 (G72_C3 * total_tex_issue) + (G72_C4 * total_tile_wb) +
					 (G72_C5 * ipa_fix->gpu_active);
				reference_voltage = G72_REF_VOLTAGE;
			} else if (strcmp(suite->name, IPA_SUITE_NAME_TNOX) == 0) {
				energy = (TNOX_C1 * ipa_fix->gpu_active) +
					 (TNOX_C2 * total_exec_instr_count) +
					 (TNOX_C3 * total_vary_instr) +
					 (TNOX_C4 * total_tex_tfch_num_operations) +
					 (TNOX_C5 * total_l2_access);
				reference_voltage = TNOX_REF_VOLTAGE;
			} else if (strcmp(suite->name, IPA_SUITE_NAME_TGOX_R1) == 0) {
				energy = (TGOX_R1_C1 * ipa_fix->gpu_active) +
					 (TGOX_R1_C2 * total_exec_instr_count) +
					 (TGOX_R1_C3 * total_vary_instr) +
					 (TGOX_R1_C4 * total_tex_tfch_num_operations) +
					 (TGOX_R1_C5 * total_l2_access);
				reference_voltage = TGOX_R1_REF_VOLTAGE;
			} else if (strcmp(suite->name, IPA_SUITE_NAME_G51) == 0) {
				energy = (G51_C1 * ipa_fix->gpu_active) +
					 (G51_C2 * total_exec_instr_count) +
					 (G51_C3 * total_vary_instr) +
					 (G51_C4 * total_tex_tfch_num_operations) +
					 (G51_C5 * total_l2_access);
				reference_voltage = G51_REF_VOLTAGE;
			} else if ((strcmp(suite->name, IPA_SUITE_NAME_G77) == 0) ||
				   (strcmp(suite->name, IPA_SUITE_NAME_TNAX) == 0)) {
				energy = (G77_C1 * total_l2_access) +
					 (G77_C2 * total_exec_instr_msg) +
					 (G77_C3 * total_exec_instr_fma) +
					 (G77_C4 * total_tex_filt_num_operations) +
					 (G77_C5 * ipa_fix->gpu_active);
				reference_voltage = G77_REF_VOLTAGE;
			} else if (strcmp(suite->name, IPA_SUITE_NAME_TBEX) == 0) {
				energy = (TBEX_C1 * total_l2_access) +
					 (TBEX_C2 * total_exec_instr_msg) +
					 (TBEX_C3 * total_exec_instr_fma) +
					 (TBEX_C4 * total_tex_filt_num_operations) +
					 (TBEX_C5 * ipa_fix->gpu_active);
				reference_voltage = TBEX_REF_VOLTAGE;
			} else if (strcmp(suite->name, IPA_SUITE_NAME_TODX) == 0) {
				if (i == KBASE_IPA_BLOCK_TYPE_TOP_LEVEL) {
					energy = (TODX_PREFETCH_STALL * ipa_fix->prefetch_stall) +
						 (TODX_IDVS_VAR_SHAD_STALL *
						  ipa_fix->idvs_var_shad_stall) +
						 (TODX_IDVS_POS_SHAD_STALL *
						  ipa_fix->idvs_pos_shad_stall) +
						 (TODX_VFETCH_POS_READ_WAIT *
						  ipa_fix->vfetch_pos_read_wait) +
						 (TODX_L2_RD_MSG_IN * total_l2_rd_msg_in) +
						 (TODX_L2_EXT_WRITE_NOSNP_FULL *
						  total_l2_ext_write_nosnp_full);
				} else {
					energy =
						(TODX_EXEC_INSTR_FMA * total_exec_instr_fma) +
						(TODX_TEX_FILT_NUM_OPERATIONS *
						 total_tex_filt_num_operations) +
						(TODX_LS_MEM_READ_SHORT * total_ls_mem_read_short) +
						(TODX_FRAG_QUADS_EZS_UPDATE *
						 total_frag_quads_ezs_update) +
						(TODX_LS_MEM_WRITE_SHORT *
						 total_ls_mem_write_short) +
						(TODX_VARY_SLOT_16 * total_vary_slot_16);
				}
				reference_voltage = TODX_REF_VOLTAGE;
			} else if (strcmp(suite->name, IPA_SUITE_NAME_TGRX) == 0) {
				if (i == KBASE_IPA_BLOCK_TYPE_TOP_LEVEL) {
					energy = (TGRX_PREFETCH_STALL * ipa_fix->prefetch_stall) +
						 (TGRX_IDVS_VAR_SHAD_STALL *
						  ipa_fix->idvs_var_shad_stall) +
						 (TGRX_IDVS_POS_SHAD_STALL *
						  ipa_fix->idvs_pos_shad_stall) +
						 (TGRX_VFETCH_POS_READ_WAIT *
						  ipa_fix->vfetch_pos_read_wait) +
						 (TGRX_L2_RD_MSG_IN * total_l2_rd_msg_in) +
						 (TGRX_L2_EXT_WRITE_NOSNP_FULL *
						  total_l2_ext_write_nosnp_full);
				} else {
					energy =
						(TGRX_EXEC_INSTR_FMA * total_exec_instr_fma) +
						(TGRX_TEX_FILT_NUM_OPERATIONS *
						 total_tex_filt_num_operations) +
						(TGRX_LS_MEM_READ_SHORT * total_ls_mem_read_short) +
						(TGRX_FRAG_QUADS_EZS_UPDATE *
						 total_frag_quads_ezs_update) +
						(TGRX_LS_MEM_WRITE_SHORT *
						 total_ls_mem_write_short) +
						(TGRX_VARY_SLOT_16 * total_vary_slot_16);
				}
				reference_voltage = TGRX_REF_VOLTAGE;

			} else if (strcmp(suite->name, IPA_SUITE_NAME_TVAX) == 0) {
				if (i == KBASE_IPA_BLOCK_TYPE_TOP_LEVEL) {
					energy = (TVAX_ITER_STALL * ipa_fix->iter_stall) +
						 (TVAX_PMGR_PTR_RD_STALL *
						  ipa_fix->pmgr_ptr_rd_stall) +
						 (TVAX_IDVS_POS_SHAD_STALL *
						  ipa_fix->idvs_pos_shad_stall) +
						 (TVAX_L2_RD_MSG_OUT * total_l2_rd_msg_out) +
						 (TVAX_L2_WR_MSG_IN * total_l2_wr_msg_in);
				} else {
					energy = (TVAX_TEX_FILT_NUM_OPERATIONS *
						  total_tex_filt_num_operations) +
						 (TVAX_EXEC_INSTR_FMA * total_exec_instr_fma) +
						 (TVAX_EXEC_INSTR_MSG * total_exec_instr_msg) +
						 (TVAX_VARY_SLOT_16 * total_vary_slot_16) +
						 (TVAX_FRAG_PARTIAL_QUADS_RAST *
						  total_frag_partial_quads_rast) +
						 (TVAX_FRAG_STARVING * total_frag_starving);
				}
				reference_voltage = TVAX_REF_VOLTAGE;

			} else if (strcmp(suite->name, IPA_SUITE_NAME_TTUX) == 0) {
				if (i == KBASE_IPA_BLOCK_TYPE_TOP_LEVEL) {
					energy = (TTUX_IDVS_POS_SHAD_STALL *
						  ipa_fix->idvs_pos_shad_stall) +
						 (TTUX_VFETCH_VERTEX_WAIT *
						  ipa_fix->vfetch_vertex_wait) +
						 (TTUX_L2_RD_MSG_IN * total_l2_rd_msg_in) +
						 (TTUX_L2_WR_MSG_IN * total_l2_wr_msg_in) +
						 (TTUX_L2_READ_LOOKUP * total_l2_read_lookup);
				} else {
					energy =
						(TTUX_EXEC_INSTR_FMA * total_exec_instr_fma) +
						(TTUX_TEX_FILT_NUM_OPERATIONS *
						 total_tex_filt_num_operations) +
						(TTUX_LS_MEM_READ_SHORT * total_ls_mem_read_short) +
						(TTUX_FULL_QUAD_WARPS * total_full_quad_warps) +
						(TTUX_EXEC_INSTR_CVT * total_exec_instr_cvt) +
						(TTUX_FRAG_QUADS_EZS_UPDATE *
						 total_frag_quads_ezs_update);
				}
				reference_voltage = TTUX_REF_VOLTAGE;
			} else if (strcmp(suite->name, IPA_SUITE_NAME_TTIX) == 0) {
				if (i == KBASE_IPA_BLOCK_TYPE_TOP_LEVEL) {
					energy = (TTIX_PRIMASSY_STALL * ipa_fix->primassy_stall) +
						 (TTIX_IDVS_VAR_SHAD_STALL *
						  ipa_fix->idvs_var_shad_stall) +
						 (TTIX_L2_RD_MSG_IN_CU * total_l2_rd_msg_in_cu) +
						 (TTIX_L2_SNP_MSG_IN * total_l2_snp_msg_in) +
						 (TTIX_L2_EXT_READ_NOSNP * total_l2_ext_read_nosnp);
				} else {
					energy =
						(TTIX_EXEC_INSTR_FMA * total_exec_instr_fma) +
						(TTIX_EXEC_INSTR_MSG * total_exec_instr_msg) +
						(TTIX_BEATS_RD_TEX * total_beats_rd_tex) +
						(TTIX_BEATS_RD_LSC_EXT * total_beats_rd_lsc_ext) +
						(TTIX_FRAG_QUADS_COARSE * total_frag_quads_coarse) +
						(TTIX_LS_MEM_WRITE_SHORT *
						 total_ls_mem_write_short) +
						(TTIX_BEATS_RD_TEX_EXT * total_beats_rd_tex_ext) +
						(TTIX_EXEC_INSTR_SFU * total_exec_instr_sfu);
				}
				reference_voltage = TTIX_REF_VOLTAGE;
			} else if (strcmp(suite->name, IPA_SUITE_NAME_TKRX) == 0) {
				if (i == KBASE_IPA_BLOCK_TYPE_TOP_LEVEL) {
					energy = (TKRX_PRIMASSY_POS_SHADER_WAIT *
						  ipa_fix->primassy_pos_shader_wait) +
						 (TKRX_IDVS_POS_SHAD_STALL *
						  ipa_fix->idvs_pos_shad_stall) +
						 (TKRX_L2_RD_MSG_OUT * total_l2_rd_msg_out) +
						 (TKRX_L2_EXT_WRITE_NOSNP_FULL *
						  total_l2_ext_write_nosnp_full) +
						 (TKRX_L2_EXT_WRITE * total_l2_ext_write) +
						 (TKRX_L2_RD_MSG_IN_STALL *
						  total_l2_rd_msg_in_stall);
				} else {
					energy =
						(TKRX_EXEC_ISSUE_SLOT_ANY *
						 total_exec_issue_slot_any) +
						(TKRX_EXEC_STARVE_ARITH * total_exec_starve_arith) +
						(TKRX_TEX_CFCH_NUM_L1_CT_OPERATIONS *
						 total_tex_cfch_num_l1_ct_operations) +
						(TKRX_EXEC_INSTR_SLOT1 * total_exec_instr_slot1) +
						(TKRX_TEX_TFCH_CLK_STALLED *
						 total_tex_tfch_clk_stalled) +
						(TKRX_EXEC_INSTR_FMA * total_exec_instr_fma) +
						(TKRX_RT_RAYS_STARTED * total_rt_rays_started);
				}
				reference_voltage = TKRX_REF_VOLTAGE;
			} else {
				mali_utf_logerr("No model in test for %s", suite->name);
				mali_utf_test_fatal("No model for GPU in test");
			}

			mali_utf_loginf("Total energy for the period = %g\n", energy);

			/* kbase clamps calculated negative value to 0 */
			if (energy < 0)
				energy = 0;

			Pdyn = energy / ipa_fix->gpu_active;

			mali_utf_loginf("Dynamic power coefficient = %g mW/(V^2 MHz)\n", Pdyn);

			Pdyn /= reference_voltage * reference_voltage;
			mali_utf_loginf(
				"Dynamic power coefficient corrected for reference voltage = %g mW/(V^2 MHz)\n",
				Pdyn);

			Pdyn = Pdyn * ipa_fix->scale;
			mali_utf_loginf("Scaled dynamic power = %g mW/(V^2 MHz)\n", Pdyn);

			Pdyn = Pdyn / 1000;
			mali_utf_loginf("Scaled dynamic power (W) = %g W/(V^2 MHz)\n", Pdyn);

			Pdyn = CSTD_MIN(Pdyn, MAX_DYNAMIC_POWER);
			mali_utf_loginf("Clamped dynamic power = %g W/(V^2 MHz)\n", Pdyn);

			mali_utf_loginf("Voltage = %g V\n", V);

			/* Scale the dynamic power coefficient to an Operating
			 * Performance Point.
			 */
			const double freq_MHz = (double)ipa_fix->freq / MICROSEC_PER_SEC;

			Pdyn *= V * V * freq_MHz;

			mali_utf_loginf("Expected unscaled dynamic power %g W\n", Pdyn);

			Pdyn_total += Pdyn;
		}

		/* Scale by the GPU utilization (between 0.0 and 1.0) */
		Pdyn_total *= ipa_fix->util;

		mali_utf_loginf("Expected dynamic power (scaled by %02g%% utilization) = %g W\n",
				ipa_fix->util, Pdyn_total);

		/* Temperature is in millidegrees celsius
		 * (1000ths of a degree) not degrees.
		 */
		const double T = (double)ipa_fix->temp / 1000.0;

		const double temp_coeff = TS0 + (TS1 * T) + (TS2 * T * T) + (TS3 * T * T * T);

		/* Calculate expected static power value.  */
		const double Pstatic = (V * V * V * temp_coeff * STATIC_COEFF);

		mali_utf_loginf("Expected static power %g W\n", Pstatic);

		/* Multiply by 1000 to convert watts to milliwatts */
		double Preal = (Pstatic + Pdyn_total) * 1000;

		check_error(Preal, power.u.val_u64);
	}
}

static void ipa_sample_dummy_thread_simple(struct mali_utf_suite *suite)
{
	struct kutf_fixture_data *const fix = suite->fixture;
	struct ipa_fixture *ipa_fix = fix->extra_funcs_data;
	int err;
	struct kutf_test_helpers_named_val power;

	err = ipa_send_values(suite);

	/* Receive power value */
	if (!err)
		err = kutf_test_helpers_userdata_receive_check_val(
			&power, suite, IPA_OUT_NAME_POWER, KUTF_TEST_HELPERS_VALTYPE_U64);

	if (!err) {
		/* Voltage is in millivolts (1000ths of a volt)
		 * not volts.
		 */
		const double V = (double)ipa_fix->voltage / 1000.0;

		const double f_Hz = (double)ipa_fix->freq;

		mali_utf_loginf("Voltage = %g V\n", V);

		/* Calculate expected dynamic power value. */
		double power_dyn = (DYNAMIC_COEFF * V * V * f_Hz);

		mali_utf_loginf("Total dynamic power = %g W\n", power_dyn);

		/* Scale by the GPU utilization (between 0.0 and 1.0) */
		power_dyn *= ipa_fix->util;

		mali_utf_loginf("Total dynamic power (scaled by %02g%% utilization) = %g W\n",
				ipa_fix->util, power_dyn);

		/* Temperature is in millidegrees celsius
		 * (1000ths of a degree) not degrees.
		 */
		const double T = (double)ipa_fix->temp / 1000.0;

		double temp_coeff = TS0 + (TS1 * T) + (TS2 * T * T) + (TS3 * T * T * T);

		/* Clamp between 1e7 and 0 */
		if (temp_coeff > 1e7)
			temp_coeff = 1e7;
		else if (temp_coeff < 0)
			temp_coeff = 0;

		mali_utf_loginf("Temperature scaling coeff: %g\n", temp_coeff);

		/* Calculate expected static power value. */
		const double temp_static_coeff = STATIC_COEFF * temp_coeff;

		const double power_static = temp_static_coeff * V * V * V;

		mali_utf_loginf("Total static power = %g W\n", power_static);

		const double total_power = power_dyn + power_static;

		mali_utf_loginf("Total power = %g W\n", total_power);

		/* Multiply by 1000 to convert watts to milliwatts */
		const double Preal = total_power * 1000;

		check_error(Preal, power.u.val_u64);
	}
}

#define IPA_EXTRA_FUNCS_G7X(name)                                               \
	{                                                                       \
		IPA_APP_NAME, /* app_name */                                    \
			name, /* suite_name */                                  \
			IPA_UNIT_TEST_0, /* test_name */                        \
		{                                                               \
			ipa_pretest, ipa_sample_dummy_thread_g7x, /* midtest */ \
				NULL, /* posttest */                            \
		}                                                               \
	}

struct kutf_extra_func_spec kutf_ipa_unit_test_extra_funcs[] = {
	IPA_EXTRA_FUNCS_G7X(IPA_SUITE_NAME_G71),
	IPA_EXTRA_FUNCS_G7X(IPA_SUITE_NAME_G72),
	IPA_EXTRA_FUNCS_G7X(IPA_SUITE_NAME_TNOX),
	IPA_EXTRA_FUNCS_G7X(IPA_SUITE_NAME_G51),
	IPA_EXTRA_FUNCS_G7X(IPA_SUITE_NAME_TGOX_R1),
	{ IPA_APP_NAME, /* app_name */
	  IPA_SUITE_NAME_SIMPLE, /* suite_name */
	  IPA_UNIT_TEST_0, /* test_name */
	  {
		  ipa_pretest, ipa_sample_dummy_thread_simple, /* midtest */
		  NULL /* posttest */
	  } },
	IPA_EXTRA_FUNCS_G7X(IPA_SUITE_NAME_G77),
	IPA_EXTRA_FUNCS_G7X(IPA_SUITE_NAME_TNAX),
	IPA_EXTRA_FUNCS_G7X(IPA_SUITE_NAME_TBEX),
	IPA_EXTRA_FUNCS_G7X(IPA_SUITE_NAME_TODX),
	IPA_EXTRA_FUNCS_G7X(IPA_SUITE_NAME_TGRX),
	IPA_EXTRA_FUNCS_G7X(IPA_SUITE_NAME_TVAX),
	IPA_EXTRA_FUNCS_G7X(IPA_SUITE_NAME_TTUX),
	IPA_EXTRA_FUNCS_G7X(IPA_SUITE_NAME_TTIX),
	IPA_EXTRA_FUNCS_G7X(IPA_SUITE_NAME_TKRX),
	{ { 0 } } /* Marks the end of the list */
};
