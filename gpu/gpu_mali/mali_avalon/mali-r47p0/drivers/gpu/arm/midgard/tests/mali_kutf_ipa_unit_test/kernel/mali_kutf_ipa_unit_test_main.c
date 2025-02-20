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

#include <linux/module.h>

#if defined(CONFIG_MALI_DEVFREQ) && defined(CONFIG_DEVFREQ_THERMAL) && defined(CONFIG_PM_DEVFREQ)

#include <linux/delay.h>
#include <linux/devfreq_cooling.h>
#include <linux/mutex.h>
#include <linux/ktime.h>

#include "mali_kbase.h"
#include "ipa/mali_kbase_ipa.h"
#include "ipa/mali_kbase_ipa_simple.h"
#include "ipa/mali_kbase_ipa_debugfs.h"
#include "csf/ipa_control/mali_kbase_csf_ipa_control.h"

#include <kutf/kutf_suite.h>
#include <kutf/kutf_utils.h>
#include <kutf/kutf_helpers.h>
#include <kutf/kutf_helpers_user.h>

#include "../mali_kutf_ipa_unit_test.h"

#include <backend/gpu/mali_kbase_model_linux.h>
#include <backend/gpu/mali_kbase_pm_internal.h>

/* Performance counter layout for memory passed into
 * gpu_model_set_dummy_prfcnt_kernel_sample() that will cause the dummy model
 * to output counters at the correct  offsets in the hwcnt dump buffer for
 * the GPU. The layout used by gpu_model_set_dummy_prfcnt_sample() is similar
 * to that of the actual GPU, but with the header (4 counters) omitted, hence
 * each block is actually offset by 60 counters, not 64. The index of each
 * counter within the block is also 4 less than that actually used by the GPU
 * (to account for the header being skipped).
 */

#if IS_ENABLED(CONFIG_MALI_NO_MALI) && MALI_UNIT_TEST
/* CSHW counter block comes first */
#define PERF_CSHW ((size_t)0)
/* Index at which the value of GPU_ACTIVE event is stored in the array used for
 * the CSHW counter block.
 */
#define GPU_ACTIVE ((size_t)0)
#define PERF_TILER ((size_t)1 * KBASE_DUMMY_MODEL_COUNTER_PER_CORE)
#define PERF_MEMSYS(mem) ((size_t)((2 + mem) * KBASE_DUMMY_MODEL_COUNTER_PER_CORE))
#define PERF_SC(mem, n) ((size_t)((2 + mem + n) * KBASE_DUMMY_MODEL_COUNTER_PER_CORE))

/* Memory system */
#define L2_ANY_LOOKUP ((size_t)21)
#define L2_RD_MSG_IN_CU ((size_t)9)
#define L2_RD_MSG_IN ((size_t)12)
#define L2_WR_MSG_IN ((size_t)14)
#define L2_SNP_MSG_IN ((size_t)16)
#define L2_RD_MSG_OUT ((size_t)18)
#define L2_READ_LOOKUP ((size_t)22)
#define L2_EXT_READ_NOSNP ((size_t)26)
#define L2_EXT_WRITE_NOSNP_FULL ((size_t)39)
#define L2_RD_MSG_IN_STALL ((size_t)13)
#define L2_EXT_WRITE ((size_t)38)

/* Shader core */
#define EXEC_INSTR_FMA ((size_t)23)
#define EXEC_INSTR_MSG ((size_t)26)
#define TEX_FILT_NUM_OPERATIONS ((size_t)35)
#define FRAG_STARVING ((size_t)4)
#define FRAG_PARTIAL_QUADS_RAST ((size_t)6)
#define FRAG_QUADS_EZS_UPDATE ((size_t)9)
#define FULL_QUAD_WARPS ((size_t)17)
#define EXEC_INSTR_CVT ((size_t)24)
#define EXEC_INSTR_SFU ((size_t)25)
#define LS_MEM_READ_SHORT ((size_t)41)
#define LS_MEM_WRITE_SHORT ((size_t)43)
#define VARY_SLOT_16 ((size_t)47)
#define BEATS_RD_LSC_EXT ((size_t)53)
#define BEATS_RD_TEX ((size_t)54)
#define BEATS_RD_TEX_EXT ((size_t)55)
#define FRAG_QUADS_COARSE ((size_t)64)
#define EXEC_STARVE_ARITH ((size_t)29)
#define TEX_TFCH_CLK_STALLED ((size_t)33)
#define RT_RAYS_STARTED ((size_t)80)
#define TEX_CFCH_NUM_L1_CT_OPERATIONS ((size_t)86)
#define EXEC_INSTR_SLOT1 ((size_t)114)
#define EXEC_ISSUE_SLOT_ANY ((size_t)115)

/* Tiler core */
#define IDVS_POS_SHAD_STALL ((size_t)19)
#define PREFETCH_STALL ((size_t)21)
#define VFETCH_POS_READ_WAIT ((size_t)25)
#define VFETCH_VERTEX_WAIT ((size_t)26)
#define PRIMASSY_STALL ((size_t)28)
#define IDVS_VAR_SHAD_STALL ((size_t)34)
#define ITER_STALL ((size_t)36)
#define PMGR_PTR_RD_STALL ((size_t)44)
#define PRIMASSY_POS_SHADER_WAIT ((size_t)60)

/* Number of milliseconds to wait for model initialization. */
#define WAIT_FOR_MODEL_MS (400U)
#endif /* IS_ENABLED(CONFIG_MALI_NO_MALI) && MALI_UNIT_TEST */

/* KUTF test application pointer for this test */
struct kutf_application *ipa_app;

/**
 * struct kutf_ipa_fixture_data - Fixture data, used by the test functions.
 * @kbdev:	kbase device for the GPU.
 * @model_ops:	ops structure of the model to test
 * @inited_ipa:	ipa model is initialized
 * @ipa_model:  ipa model for this fixture
 * @devfreq:	fake devfreq device if the actual driver didn't init devfreq
 * @orig_shader_present: original value of the bitmask of present shader cores
 * @orig_l2_present:	original value of the bitmask of present L2 slices
 */
struct kutf_ipa_fixture_data {
	struct kbase_device *kbdev;
	struct kbase_ipa_model_ops *model_ops;
	bool inited_ipa;
	struct kbase_ipa_model *ipa_model;
	struct devfreq *devfreq;
	u64 orig_shader_present;
	u64 orig_l2_present;
};

#if IS_ENABLED(CONFIG_MALI_NO_MALI) && MALI_UNIT_TEST
/**
 * struct kutf_ipa_in - Input values copied from user space, used by the test functions.
 * @f_Hz:        GPU clock frequency, in hertz.
 * @v_mV:        GPU voltage, in millivolts.
 * @num_l2_slices: Number of L2 slices
 * @num_cores:   Number of shader cores
 * @l2_access:   Number of level 2 cache lookups.
 * @exec_instr_count: Number of instruction tuples executed per shader core.
 * @tex_issue:   Number of threads issued to the texel coordinate stage per
 *               shader core.
 * @tile_wb:     Number of write beats for the tile buffers per shader core.
 * @gpu_active:  Number of cycles the GPU was active.
 * @tex_tfch_num_operations:  Number of operations executed in the texel fetcher
 *                            hit path.
 * @vary_instr:               Number of varying instructions.
 * @exec_instr_msg:           Number of message instruction per Processing Unit
 * @full_quad_warps:          Number of full quad wraps
 * @exec_instr_fma:           Number of FMA arithmetic operations per Processing Unit
 * @exec_instr_cvt:           Number of CVT arithmetic operations per Processing Unit
 * @exec_instr_sfu:           Number of SFU arithmetic operations per Processing Unit
 * @exec_starve_arith:        Processing Unit starvation
 * @tex_tfch_clk_stalled:     Number of cycles a quad is stalled waiting to enter the texel fetcher
 * @tex_filt_num_operations:  Number of operations executed in the filtering unit
 * @idvs_pos_shad_stall:      Number of cycles stalled on IDVS interface
 *                            requesting position shading
 * @prefetch_stall:           Number of cycles the prefetcher has valid data but
 *                            is stalled by the vertex fetcher
 * @vfetch_pos_read_wait:     Number of cycles spent reading in positions
 * @vfetch_vertex_wait:       Number of cycles waiting for a valid vertex in the
 *                            vertex fetcher
 * @primassy_stall:           Number of cycles the primitive assembly output is stalled
 * @idvs_var_shad_stall:      Number of cycles stalled on IDVS interface
 *                            requesting varying shading
 * @iter_stall:               The number of cycles the iterator has valid output
 *                            data but is stalled by the write compressor and the
 *                            pointer cache
 * @pmgr_ptr_rd_stall:        Number of cycles waiting for a valid pointer in the
 *                            pointer manager
 * @primassy_pos_shader_wait: Number of cycles late primitive assembly is waiting for
 *                            position shading
 * @l2_rd_msg_in:             Number of read messages received by the L2C from
 *                            internal masters
 * @l2_rd_msg_in_cu:          Number of clean unique messages received by the L2C from
 *                            internal requesters
 * @l2_rd_msg_out:            Number of read messages sent by the L2C
 * @l2_wr_msg_in:             Number of write messages received by the L2C from
 *                            internal masters
 * @l2_snp_msg_in:            Number of snoop messages received by the L2C from
 *                            internal requesters
 * @l2_read_lookup:           Number of cache read lookups by the L2C
 * @l2_ext_read_nosnp:        Number of external ReadNoSnoop transactions
 * @l2_rd_msg_in_stall:       Number of cycles input read messages are stalled by the L2C
 * @l2_ext_write:             Number of external write transactions
 * @l2_ext_write_nosnp_full:  Number of external WriteNoSnpFull transactions
 * @ls_mem_read_short:        Number of memory read, partial cache line
 * @frag_quads_ezs_update:    Number of quads doing early ZS update
 * @frag_starving:            Number of cycles the fragment front end is starving
 *                            the execution engine of new threads.
 * @frag_partial_quads_rast:  Number of fragment quads with helper threads that do
 *                            not correspond to a hit sample point
 * @ls_mem_write_short:       Number of memory write, partial cache line
 * @vary_slot_16:             Number of 16-bit varying slots
 * @beats_rd_lsc_ext:         Number of external read beats for the load/store cache
 * @beats_rd_tex:             Number of read beats for the texture cache
 * @beats_rd_tex_ext:         Number of external read beats for the texture cache
 * @frag_quads_coarse:        Number of coarse quads before early culling
 * @rt_rays_started:          Number of rays that tested the root node of
 *                            a top level acceleration structure
 * @tex_cfch_num_l1_ct_operations: Number of cycles the texture L1 cache is being accessed
 * @exec_instr_slot1:         Instructions issued in slot 1
 * @exec_issue_slot_any:      Clock cycles where instructions are issued in any slot
 * @interval_ns:              Interval since last performance counter dump, in
 *                            nanoseconds.
 * @temp:                     Temperature of the thermal zone, in millidegrees
 *                            celsius.
 * @busy_time:                Time the GPU was busy in nanoseconds.
 * @scale:                    User-specified power scaling factor. This is
 *                            interpreted as a fraction where the denominator is
 *                            1000.
 * @ts:                       Temperature scaling factors
 * @static_coefficient:       Static power coefficient, in uW/V^3
 * @dynamic_coefficient:      Dynamic power coefficient, in pW/Hz.V^2
 */
struct kutf_ipa_in {
	struct kutf_helper_named_val f_Hz;
	struct kutf_helper_named_val v_mV;
	struct kutf_helper_named_val num_l2_slices;
	struct kutf_helper_named_val num_cores;
	struct kutf_helper_named_val l2_access[KBASE_DUMMY_MODEL_MAX_MEMSYS_BLOCKS];
	struct kutf_helper_named_val exec_instr_count[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	struct kutf_helper_named_val tex_issue[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	struct kutf_helper_named_val tile_wb[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	struct kutf_helper_named_val tex_tfch_num_operations[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	struct kutf_helper_named_val vary_instr[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	struct kutf_helper_named_val exec_instr_msg[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	struct kutf_helper_named_val full_quad_warps[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	struct kutf_helper_named_val exec_instr_fma[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	struct kutf_helper_named_val exec_instr_cvt[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	struct kutf_helper_named_val exec_instr_sfu[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	struct kutf_helper_named_val exec_starve_arith[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	struct kutf_helper_named_val tex_tfch_clk_stalled[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	struct kutf_helper_named_val tex_filt_num_operations[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	struct kutf_helper_named_val gpu_active;
	struct kutf_helper_named_val idvs_pos_shad_stall;
	struct kutf_helper_named_val prefetch_stall;
	struct kutf_helper_named_val vfetch_pos_read_wait;
	struct kutf_helper_named_val vfetch_vertex_wait;
	struct kutf_helper_named_val primassy_stall;
	struct kutf_helper_named_val idvs_var_shad_stall;
	struct kutf_helper_named_val iter_stall;
	struct kutf_helper_named_val pmgr_ptr_rd_stall;
	struct kutf_helper_named_val primassy_pos_shader_wait;
	struct kutf_helper_named_val l2_rd_msg_in_cu[KBASE_DUMMY_MODEL_MAX_MEMSYS_BLOCKS];
	struct kutf_helper_named_val l2_rd_msg_in[KBASE_DUMMY_MODEL_MAX_MEMSYS_BLOCKS];
	struct kutf_helper_named_val l2_rd_msg_out[KBASE_DUMMY_MODEL_MAX_MEMSYS_BLOCKS];
	struct kutf_helper_named_val l2_wr_msg_in[KBASE_DUMMY_MODEL_MAX_MEMSYS_BLOCKS];
	struct kutf_helper_named_val l2_snp_msg_in[KBASE_DUMMY_MODEL_MAX_MEMSYS_BLOCKS];
	struct kutf_helper_named_val l2_read_lookup[KBASE_DUMMY_MODEL_MAX_MEMSYS_BLOCKS];
	struct kutf_helper_named_val l2_ext_read_nosnp[KBASE_DUMMY_MODEL_MAX_MEMSYS_BLOCKS];
	struct kutf_helper_named_val l2_rd_msg_in_stall[KBASE_DUMMY_MODEL_MAX_MEMSYS_BLOCKS];
	struct kutf_helper_named_val l2_ext_write[KBASE_DUMMY_MODEL_MAX_MEMSYS_BLOCKS];
	struct kutf_helper_named_val l2_ext_write_nosnp_full[KBASE_DUMMY_MODEL_MAX_MEMSYS_BLOCKS];
	struct kutf_helper_named_val ls_mem_read_short[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	struct kutf_helper_named_val frag_quads_ezs_update[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	struct kutf_helper_named_val frag_starving[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	struct kutf_helper_named_val frag_partial_quads_rast[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	struct kutf_helper_named_val ls_mem_write_short[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	struct kutf_helper_named_val vary_slot_16[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	struct kutf_helper_named_val beats_rd_lsc_ext[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	struct kutf_helper_named_val beats_rd_tex[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	struct kutf_helper_named_val beats_rd_tex_ext[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	struct kutf_helper_named_val frag_quads_coarse[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	struct kutf_helper_named_val rt_rays_started[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	struct kutf_helper_named_val
		tex_cfch_num_l1_ct_operations[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	struct kutf_helper_named_val exec_instr_slot1[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	struct kutf_helper_named_val exec_issue_slot_any[KBASE_DUMMY_MODEL_MAX_SHADER_CORES];
	struct kutf_helper_named_val interval_ns;
	struct kutf_helper_named_val temp;
	struct kutf_helper_named_val busy_time;
	struct kutf_helper_named_val scale;
	struct kutf_helper_named_val ts[4];
	struct kutf_helper_named_val static_coefficient;
	struct kutf_helper_named_val dynamic_coefficient;
};

static int kutf_ipa_set_prfcnt(const struct kutf_ipa_in *const in,
			       struct kutf_context *const context);
#endif

/**
 * dummy_devfreq_new() - Create fake dynamic voltage and frequency scaling info
 * @kbdev: kbase device
 *
 * Return: Pointer to the dummy devfreq, or NULL if memory allocation failed.
 *
 * The dummy information is used by the kbase device if the driver didn't
 * initialize devfreq. The caller must free the dummy information allocated by
 * this function when no longer required.
 */
static struct devfreq *dummy_devfreq_new(struct kbase_device *const kbdev)
{
	struct devfreq *devfreq;

	pr_debug("Alloc fake devfreq\n");
	devfreq = kzalloc(sizeof(*devfreq), GFP_KERNEL);
	if (devfreq != NULL)
		dev_set_drvdata(&devfreq->dev, kbdev);

	return devfreq;
}

/**
 * mali_kutf_ipa_unit_create_fixture() - Creates the fixture data required
 *                                          for all the tests in the ipa suite.
 * @context:	KUTF context.
 *
 * Return: Fixture data created on success or NULL on failure
 */
static void *mali_kutf_ipa_unit_create_fixture(struct kutf_context *context)
{
	struct kutf_ipa_fixture_data *data;
	struct kbase_device *kbdev = NULL;
	int err = 0;
	const u32 raw_gpu_id = context->suite->suite_data.u32_value;
	struct kbase_gpu_id_props gpu_id;
	const char *model_name;
	const struct kbase_ipa_model_ops *model_ops;

	kbase_gpuprops_parse_gpu_id(&gpu_id, raw_gpu_id);

	pr_debug("Finding power model under test for ID 0x%X\n", raw_gpu_id);
	model_name = kbase_ipa_model_name_from_id(&gpu_id);
	if (!model_name) {
		kutf_test_fail(context, "Failed to get model name from GPU ID");
		return NULL;
	}
	pr_debug("Finding ops for model name \'%s\'\n", model_name);
	model_ops = kbase_ipa_model_ops_find(kbdev, model_name);
	if (!model_ops) {
		pr_err("Failed to find model ops for model \'%s\'", model_name);
		kutf_test_fail(context, "Failed to find model ops for model");
		return NULL;
	}
	pr_debug("Got ops %pK with name \'%s\'\n", model_ops, model_ops->name);

	pr_debug("Creating fixture\n");
	data = kutf_mempool_alloc(&context->fixture_pool, sizeof(struct kutf_ipa_fixture_data));

	if (!data)
		return NULL;

	*data = (const struct kutf_ipa_fixture_data){ 0 };

	/* Acquire the kbase device */
	pr_debug("Finding device\n");
	kbdev = kbase_find_device(-1);
	if (kbdev == NULL) {
		kutf_test_fail(context, "Failed to find kbase device");
		return NULL;
	}

	data->kbdev = kbdev;

#if IS_ENABLED(CONFIG_MALI_NO_MALI) && MALI_UNIT_TEST
	gpu_model_get_dummy_prfcnt_cores(kbdev, &data->orig_l2_present, &data->orig_shader_present);
#endif

	/* IPA wasn't initialized (maybe some other part of devfreq init
	 * failed) - force initialize it here.
	 */
	if (!kbdev->ipa.fallback_model) {
		pr_debug("Init IPA\n");
		err = kbase_ipa_init(kbdev);
		pr_debug("Done init IPA\n");
		data->inited_ipa = true;
	} else {
		data->inited_ipa = false;
	}

	if (!err) {
		pr_debug("Register model\n");
		mutex_lock(&kbdev->ipa.lock);
		if (kbdev->ipa.configured_model != kbdev->ipa.fallback_model) {
			/* Terminate an existing configured_model and assign fallback_model to it.
			 * This is to avoid having more than 1 IPA model instances at the same time.
			 */
			pr_debug("Terminating model\n");
			kbase_ipa_term_model(kbdev->ipa.configured_model);
			kbdev->ipa.configured_model = kbdev->ipa.fallback_model;
		}
		/* Initiate new model for the test. */
		data->ipa_model = kbase_ipa_init_model(kbdev, model_ops);
		mutex_unlock(&kbdev->ipa.lock);

		if (!data->ipa_model) {
			pr_err("Couldn't register ipa model (%s)\n", model_ops->name);
			kutf_test_fail(context, "Couldn't register ipa model");
			err = -ENOMEM;
		} else {
			if (!kbdev->devfreq) {
				data->devfreq = dummy_devfreq_new(kbdev);

				if (!data->devfreq) {
					kutf_test_fail(context, "Couldn't allocate fake devfreq");
					err = -ENOMEM;
				}
			} else {
				data->devfreq = NULL;
			}

			if (err) {
				mutex_lock(&kbdev->ipa.lock);
				kbase_ipa_term_model(data->ipa_model);
				mutex_unlock(&kbdev->ipa.lock);
			}
		}
		if (err && data->inited_ipa)
			kbase_ipa_term(kbdev);
	}

	if (err) {
		data = NULL;
		kbase_release_device(kbdev);
		pr_debug("Failed to create fixture\n");
	} else {
		/* All models other than simple one use HW counters. */
		if (model_ops != &kbase_simple_ipa_model_ops)
			kbase_pm_context_active(kbdev);

		pr_debug("Final\n");
		if (!kbdev->devfreq)
			kbdev->devfreq = data->devfreq;

		pr_debug("Created fixture\n");
	}

	return data;
}

/**
 * mali_kutf_ipa_unit_remove_fixture() - Destroy fixture data previously
 * created by mali_kutf_ipa_unit_create_fixture.
 *
 * @context:             KUTF context.
 */
static void mali_kutf_ipa_unit_remove_fixture(struct kutf_context *context)
{
	struct kutf_ipa_fixture_data *data = context->fixture;
	struct kbase_device *kbdev = data->kbdev;
	bool idle_pm = false;

	pr_debug("Destroying fixture\n");
	if (data->ipa_model) {
		idle_pm = (data->ipa_model->ops != &kbase_simple_ipa_model_ops);
		mutex_lock(&kbdev->ipa.lock);
		kbase_ipa_term_model(data->ipa_model);
		mutex_unlock(&kbdev->ipa.lock);
	}

#if MALI_USE_CSF && IS_ENABLED(CONFIG_MALI_NO_MALI) && MALI_UNIT_TEST
	/* Clear the counter values array maintained on the dummy model side,
	 * so that the initial samples are zero for the kbase_ipa_control
	 * client registered in the next fixture.
	 */
	gpu_model_clear_prfcnt_values();
#endif

	if (data->inited_ipa)
		kbase_ipa_term(kbdev);

	if (idle_pm)
		kbase_pm_context_idle(kbdev);

	if (data->devfreq)
		kbdev->devfreq = NULL;

	kfree(data->devfreq);

#if IS_ENABLED(CONFIG_MALI_NO_MALI) && MALI_UNIT_TEST
	gpu_model_set_dummy_prfcnt_cores(kbdev, data->orig_l2_present, data->orig_shader_present);
#endif
	/* Text buffers don't need to be terminated */

	kbase_release_device(kbdev);
	pr_debug("Destroyed fixture\n");
}

#if IS_ENABLED(CONFIG_MALI_NO_MALI) && MALI_UNIT_TEST

/**
 * kbase_ipa_real_power() - get the real power consumption of the GPU
 * @in:       Pointer to a set of input values copied from user-side
 * @context:  KUTF context
 * @power:    Where to store the power consumption, in mW.
 *
 * This function first injects fake performance counter values into the dummy
 * model. Then calls the kernel function, that is only exposed for use by unit
 * tests, to estimate power.
 * Both operations are done in atomic way with kbdev->ipa.lock held.
 * The returned value incorporates static and dynamic power consumption.
 *
 * Return: 0 on success, or an error code.
 */
static int kbase_ipa_real_power(const struct kutf_ipa_in *const in,
				struct kutf_context *const context, u32 *power)
{
	struct kutf_ipa_fixture_data *data = context->fixture;
	struct kbase_device *kbdev = data->kbdev;
	struct kbasep_pm_metrics diff;
	unsigned long f_Hz = in->f_Hz.u.val_u64;
	unsigned long v_mV = in->v_mV.u.val_u64;
	unsigned long total_time = in->interval_ns.u.val_u64;
	unsigned long busy_time = in->busy_time.u.val_u64;
	int ret;

	mutex_lock(&kbdev->ipa.lock);

	/* With the lock held inject the fake counter values. This prevents
	 * any interference from kbase_ipa_reset_data() called due to the
	 * devfreq polling.
	 */
	ret = kutf_ipa_set_prfcnt(in, context);
	if (ret) {
		kutf_test_fail(context, "failed to inject counter values inside dummy model");
		mutex_unlock(&kbdev->ipa.lock);
		return ret;
	}

	/* Override the utilization numbers for testing purposes */
	kbase_pm_get_dvfs_metrics(kbdev, &kbdev->ipa.last_metrics, &diff);
	kbdev->ipa.last_metrics.time_busy -= (u32)busy_time;
	kbdev->ipa.last_metrics.time_idle -= (u32)(total_time - busy_time);
	kbdev->ipa.last_sample_time = ktime_get_raw();

	/* Call the Driver function to compute the power.
	 * The fake values injected previously should get read by the Driver
	 * for the dynamic power estimation.
	 */
	ret = kbase_get_real_power_locked(kbdev, power, f_Hz, v_mV);

	mutex_unlock(&kbdev->ipa.lock);

	return ret;
}

/**
 * set_power_model() - Initialize and set ipa model
 *
 * @kbdev:      kbase device
 * @new_model:  ipa model to set for the device
 * @old_model:  currently configured model on the device
 */
static void set_power_model(struct kbase_device *kbdev, struct kbase_ipa_model *new_model,
			    struct kbase_ipa_model **old_model)
{
	mutex_lock(&kbdev->ipa.lock);

	*old_model = kbdev->ipa.configured_model;
	kbdev->ipa.configured_model = new_model;

	mutex_unlock(&kbdev->ipa.lock);
}

/**
 * get_fallback_power_model - Return pointer to the fallback power model
 * @kbdev: Device pointer
 *
 * Return: Pointer to the fallback power model
 */
static struct kbase_ipa_model *get_fallback_power_model(struct kbase_device *kbdev)
{
	struct kbase_ipa_model *fallback_model;

	mutex_lock(&kbdev->ipa.lock);
	fallback_model = kbdev->ipa.fallback_model;
	mutex_unlock(&kbdev->ipa.lock);

	return fallback_model;
}

/**
 * restore_power_model() - Restore ipa model
 * @kbdev:      kbase device
 * @old_model:  model to restore
 *
 */
static void restore_power_model(struct kbase_device *kbdev, struct kbase_ipa_model *old_model)
{
	mutex_lock(&kbdev->ipa.lock);

	kbdev->ipa.configured_model = old_model;

	mutex_unlock(&kbdev->ipa.lock);
}

/**
 * receive_array_elem() - Receive an integer array element value from user-side
 * @val:     Pointer to the output array of named unsigned integer values.
 * @context: KUTF context.
 * @name:    Name of the array (as specified by the user-side test).
 * @index:   Index of the array element to receive.
 *
 * Return: 0 on success. Negative value on failure to receive from the 'run'
 *         file, positive value indicates an enum kutf_helper_err value for
 *         correct reception of data but invalid parsing.
 */
static int receive_array_elem(struct kutf_helper_named_val *const val,
			      struct kutf_context *const context, const char *const name,
			      const size_t index)
{
	int err;
	char var_name[KUTF_HELPER_MAX_VAL_NAME_LEN + 1];
	const int n = snprintf(var_name, sizeof(var_name), "%s_%zu", name, index);

	if (n < 0) {
		kutf_test_fail(context, "Bad format string");
		err = -EINVAL;
	} else if (n >= sizeof(var_name)) {
		kutf_test_fail(context, "String buffer overflow");
		err = -EINVAL;
	} else {
		pr_debug("Receiving %s\n", var_name);
		err = kutf_helper_receive_check_val(&val[index], context, var_name,
						    KUTF_HELPER_VALTYPE_U64);
	}
	return err;
}

/**
 * kutf_ipa_in_new() - Receive all variable values from user-side
 * @context: KUTF context.
 * @values:  Where to store a pointer to a set of input values copied
 *           from user-side, if successful.
 *
 * Return: 0 on success. Negative value on failure to receive from the 'run'
 *         file or memory allocation failure, positive value indicates an
 *         enum kutf_helper_err value for correct reception of data but
 *         invalid parsing.
 *
 * The caller need not free the set of input values allocated by this
 * function because they are allocated from a memory pool with the same
 * lifetime as the test fixture.
 */
static int kutf_ipa_in_new(struct kutf_context *const context, struct kutf_ipa_in **const values)
{
	u64 i;
	int err = 0;
	struct kutf_ipa_in *const in = kutf_mempool_alloc(&context->fixture_pool, sizeof(*in));

	if (!in) {
		kutf_test_fail(context, "Out of memory");
		err = -ENOMEM;
	}

	/* Receive frequency value */
	if (!err)
		err = kutf_helper_receive_check_val(&in->f_Hz, context, IPA_IN_NAME_FREQ,
						    KUTF_HELPER_VALTYPE_U64);

	/* Receive voltage value */
	if (!err)
		err = kutf_helper_receive_check_val(&in->v_mV, context, IPA_IN_NAME_VOLTAGE,
						    KUTF_HELPER_VALTYPE_U64);

	/* Receive number of L2 slices value */
	if (!err)
		err = kutf_helper_receive_check_val(&in->num_l2_slices, context,
						    IPA_IN_NAME_NUM_L2_SLICES,
						    KUTF_HELPER_VALTYPE_U64);

	/* Receive number of cores value */
	if (!err)
		err = kutf_helper_receive_check_val(&in->num_cores, context, IPA_IN_NAME_NUM_CORES,
						    KUTF_HELPER_VALTYPE_U64);

	/* Receive performance counter values */
	for (i = 0; i < in->num_l2_slices.u.val_u64 && !err; ++i) {
		err = receive_array_elem(in->l2_access, context, IPA_IN_NAME_L2_ACCESS, i);
		if (!err)
			err = receive_array_elem(in->l2_rd_msg_in_cu, context,
						 IPA_IN_NAME_L2_RD_MSG_IN_CU, i);
		if (!err)
			err = receive_array_elem(in->l2_rd_msg_in, context,
						 IPA_IN_NAME_L2_RD_MSG_IN, i);
		if (!err)
			err = receive_array_elem(in->l2_wr_msg_in, context,
						 IPA_IN_NAME_L2_WR_MSG_IN, i);
		if (!err)
			err = receive_array_elem(in->l2_snp_msg_in, context,
						 IPA_IN_NAME_L2_SNP_MSG_IN, i);
		if (!err)
			err = receive_array_elem(in->l2_rd_msg_out, context,
						 IPA_IN_NAME_L2_RD_MSG_OUT, i);
		if (!err)
			err = receive_array_elem(in->l2_read_lookup, context,
						 IPA_IN_NAME_L2_READ_LOOKUP, i);
		if (!err)
			err = receive_array_elem(in->l2_ext_read_nosnp, context,
						 IPA_IN_NAME_L2_EXT_READ_NOSNP, i);
		if (!err) {
			err = receive_array_elem(in->l2_rd_msg_in_stall, context,
						 IPA_IN_NAME_L2_RD_MSG_IN_STALL, i);
		}
		if (!err) {
			err = receive_array_elem(in->l2_ext_write, context,
						 IPA_IN_NAME_L2_EXT_WRITE, i);
		}
		if (!err)
			err = receive_array_elem(in->l2_ext_write_nosnp_full, context,
						 IPA_IN_NAME_L2_EXT_WRITE_NOSNP_FULL, i);
	}

	for (i = 0; i < in->num_cores.u.val_u64 && !err; ++i) {
		if (!err)
			err = receive_array_elem(in->exec_instr_count, context,
						 IPA_IN_NAME_EXEC_INSTR_COUNT, i);
		if (!err)
			err = receive_array_elem(in->tex_issue, context, IPA_IN_NAME_TEX_ISSUE, i);

		if (!err)
			err = receive_array_elem(in->tile_wb, context, IPA_IN_NAME_TILE_WB, i);

		if (!err)
			err = receive_array_elem(in->tex_tfch_num_operations, context,
						 IPA_IN_NAME_TEX_TFCH_NUM_OPERATIONS, i);
		if (!err)
			err = receive_array_elem(in->vary_instr, context, IPA_IN_NAME_VARY_INSTR,
						 i);
		if (!err)
			err = receive_array_elem(in->exec_instr_msg, context,
						 IPA_IN_NAME_EXEC_INSTR_MSG, i);
		if (!err)
			err = receive_array_elem(in->exec_instr_fma, context,
						 IPA_IN_NAME_EXEC_INSTR_FMA, i);
		if (!err)
			err = receive_array_elem(in->exec_instr_cvt, context,
						 IPA_IN_NAME_EXEC_INSTR_CVT, i);
		if (!err)
			err = receive_array_elem(in->exec_instr_sfu, context,
						 IPA_IN_NAME_EXEC_INSTR_SFU, i);
		if (!err)
			err = receive_array_elem(in->exec_starve_arith, context,
						 IPA_IN_NAME_EXEC_STARVE_ARITH, i);
		if (!err)
			err = receive_array_elem(in->tex_tfch_clk_stalled, context,
						 IPA_IN_NAME_TEX_TFCH_CLK_STALLED, i);
		if (!err)
			err = receive_array_elem(in->tex_filt_num_operations, context,
						 IPA_IN_NAME_TEX_FILT_NUM_OPERATIONS, i);
		if (!err)
			err = receive_array_elem(in->ls_mem_read_short, context,
						 IPA_IN_NAME_LS_MEM_READ_SHORT, i);
		if (!err)
			err = receive_array_elem(in->frag_starving, context,
						 IPA_IN_NAME_FRAG_STARVING, i);
		if (!err)
			err = receive_array_elem(in->frag_partial_quads_rast, context,
						 IPA_IN_NAME_FRAG_PARTIAL_QUADS_RAST, i);
		if (!err)
			err = receive_array_elem(in->frag_quads_ezs_update, context,
						 IPA_IN_NAME_FRAG_QUADS_EZS_UPDATE, i);
		if (!err)
			err = receive_array_elem(in->full_quad_warps, context,
						 IPA_IN_NAME_FULL_QUAD_WARPS, i);
		if (!err)
			err = receive_array_elem(in->ls_mem_write_short, context,
						 IPA_IN_NAME_LS_MEM_WRITE_SHORT, i);
		if (!err)
			err = receive_array_elem(in->vary_slot_16, context,
						 IPA_IN_NAME_VARY_SLOT_16, i);
		if (!err)
			err = receive_array_elem(in->beats_rd_lsc_ext, context,
						 IPA_IN_NAME_BEATS_RD_LSC_EXT, i);
		if (!err)
			err = receive_array_elem(in->beats_rd_tex, context,
						 IPA_IN_NAME_BEATS_RD_TEX, i);
		if (!err)
			err = receive_array_elem(in->beats_rd_tex_ext, context,
						 IPA_IN_NAME_BEATS_RD_TEX_EXT, i);
		if (!err)
			err = receive_array_elem(in->frag_quads_coarse, context,
						 IPA_IN_NAME_FRAG_QUADS_COARSE, i);
		if (!err)
			err = receive_array_elem(in->rt_rays_started, context,
						 IPA_IN_NAME_RT_RAYS_STARTED, i);
		if (!err)
			err = receive_array_elem(in->tex_cfch_num_l1_ct_operations, context,
						 IPA_IN_NAME_TEX_CFCH_NUM_L1_CT_OPERATIONS, i);
		if (!err)
			err = receive_array_elem(in->exec_instr_slot1, context,
						 IPA_IN_NAME_EXEC_INSTR_SLOT1, i);
		if (!err)
			err = receive_array_elem(in->exec_issue_slot_any, context,
						 IPA_IN_NAME_EXEC_ISSUE_SLOT_ANY, i);
	}

	if (!err)
		err = kutf_helper_receive_check_val(
			&in->gpu_active, context, IPA_IN_NAME_GPU_ACTIVE, KUTF_HELPER_VALTYPE_U64);

	if (!err)
		err = kutf_helper_receive_check_val(&in->idvs_pos_shad_stall, context,
						    IPA_IN_NAME_IDVS_POS_SHAD_STALL,
						    KUTF_HELPER_VALTYPE_U64);
	if (!err)
		err = kutf_helper_receive_check_val(&in->prefetch_stall, context,
						    IPA_IN_NAME_PREFETCH_STALL,
						    KUTF_HELPER_VALTYPE_U64);
	if (!err)
		err = kutf_helper_receive_check_val(&in->vfetch_pos_read_wait, context,
						    IPA_IN_NAME_VFETCH_POS_READ_WAIT,
						    KUTF_HELPER_VALTYPE_U64);
	if (!err)
		err = kutf_helper_receive_check_val(&in->vfetch_vertex_wait, context,
						    IPA_IN_NAME_VFETCH_VERTEX_WAIT,
						    KUTF_HELPER_VALTYPE_U64);
	if (!err)
		err = kutf_helper_receive_check_val(&in->primassy_stall, context,
						    IPA_IN_NAME_PRIMASSY_STALL,
						    KUTF_HELPER_VALTYPE_U64);
	if (!err)
		err = kutf_helper_receive_check_val(&in->idvs_var_shad_stall, context,
						    IPA_IN_NAME_IDVS_VAR_SHAD_STALL,
						    KUTF_HELPER_VALTYPE_U64);
	if (!err)
		err = kutf_helper_receive_check_val(
			&in->iter_stall, context, IPA_IN_NAME_ITER_STALL, KUTF_HELPER_VALTYPE_U64);
	if (!err)
		err = kutf_helper_receive_check_val(&in->pmgr_ptr_rd_stall, context,
						    IPA_IN_NAME_PMGR_PTR_RD_STALL,
						    KUTF_HELPER_VALTYPE_U64);
	if (!err)
		err = kutf_helper_receive_check_val(&in->primassy_pos_shader_wait, context,
						    IPA_IN_NAME_PRIMASSY_POS_SHADER_WAIT,
						    KUTF_HELPER_VALTYPE_U64);
	/* Receive interval and temperature values */
	if (!err)
		err = kutf_helper_receive_check_val(&in->interval_ns, context, IPA_IN_NAME_INTERVAL,
						    KUTF_HELPER_VALTYPE_U64);

	if (!err)
		err = kutf_helper_receive_check_val(&in->temp, context, IPA_IN_NAME_TEMP,
						    KUTF_HELPER_VALTYPE_U64);

	/* Receive utilization value */

	if (!err)
		err = kutf_helper_receive_check_val(&in->busy_time, context, IPA_IN_NAME_BUSY_TIME,
						    KUTF_HELPER_VALTYPE_U64);

	/* Receive scaling factor */
	if (!err) {
		err = kutf_helper_receive_check_val(&in->scale, context, IPA_IN_NAME_SCALE,
						    KUTF_HELPER_VALTYPE_U64);

		if (!err && in->scale.u.val_u64 > S32_MAX) {
			err = -EINVAL;
			kutf_test_fail(context, "Scale value is out of range");
		}
	}

	if (!err) {
		for (i = 0; i < 4 && !err; ++i) {
			if (!err)
				err = receive_array_elem(in->ts, context, IPA_IN_NAME_TS, i);
		}
	}

	if (!err)
		err = kutf_helper_receive_check_val(&in->static_coefficient, context,
						    IPA_IN_NAME_STATIC_COEFFICIENT,
						    KUTF_HELPER_VALTYPE_U64);

	if (!err)
		err = kutf_helper_receive_check_val(&in->dynamic_coefficient, context,
						    IPA_IN_NAME_DYNAMIC_COEFFICIENT,
						    KUTF_HELPER_VALTYPE_U64);

	if (!err) {
		if (values)
			*values = in;
		else
			err = -EINVAL;
	}

	return err;
}

/**
 * kutf_ipa_set_prfcnt - set dummy performance counter values
 * @in:       Pointer to a set of input values copied from user-side
 * @context:  KUTF context
 *
 * Return: 0 on success, or an error code.
 *
 * This function configures the dummy GPU model to report fake performance
 * counter values to the driver. The fake values will be used by the power
 * model during the test, to ensure deterministic behavior.
 */
static int kutf_ipa_set_prfcnt(const struct kutf_ipa_in *const in,
			       struct kutf_context *const context)
{
	struct kutf_ipa_fixture_data *data = context->fixture;
	struct kbase_device *kbdev = data->kbdev;
	int err = 0;
	const size_t hwcnt_size = KBASE_DUMMY_MODEL_COUNTER_TOTAL * sizeof(u64);
	u64 *const hwcnt_tbl = kutf_mempool_alloc(&context->fixture_pool, hwcnt_size);

	if (!hwcnt_tbl) {
		kutf_test_fail(context, "out of memory");
		err = -ENOMEM;
	} else {
		u64 i;

		const uint64_t num_cores = in->num_cores.u.val_u64;
		const uint64_t num_l2_slices = in->num_l2_slices.u.val_u64;

		memset(hwcnt_tbl, 0, hwcnt_size);
		hwcnt_tbl[PERF_CSHW + GPU_ACTIVE] = in->gpu_active.u.val_u64;

		hwcnt_tbl[PERF_TILER + IDVS_POS_SHAD_STALL] = in->idvs_pos_shad_stall.u.val_u64;
		hwcnt_tbl[PERF_TILER + PREFETCH_STALL] = in->prefetch_stall.u.val_u64;
		hwcnt_tbl[PERF_TILER + VFETCH_POS_READ_WAIT] = in->vfetch_pos_read_wait.u.val_u64;
		hwcnt_tbl[PERF_TILER + VFETCH_VERTEX_WAIT] = in->vfetch_vertex_wait.u.val_u64;
		hwcnt_tbl[PERF_TILER + PRIMASSY_STALL] = in->primassy_stall.u.val_u64;
		hwcnt_tbl[PERF_TILER + IDVS_VAR_SHAD_STALL] = in->idvs_var_shad_stall.u.val_u64;
		hwcnt_tbl[PERF_TILER + ITER_STALL] = in->iter_stall.u.val_u64;
		hwcnt_tbl[PERF_TILER + PMGR_PTR_RD_STALL] = in->pmgr_ptr_rd_stall.u.val_u64;
		hwcnt_tbl[PERF_TILER + PRIMASSY_POS_SHADER_WAIT] =
			in->primassy_pos_shader_wait.u.val_u64;

		for (i = 0; i < num_l2_slices; ++i) {
			hwcnt_tbl[PERF_MEMSYS(i) + L2_ANY_LOOKUP] = in->l2_access[i].u.val_u64;
			hwcnt_tbl[PERF_MEMSYS(i) + L2_RD_MSG_IN_CU] =
				in->l2_rd_msg_in_cu[i].u.val_u64;
			hwcnt_tbl[PERF_MEMSYS(i) + L2_RD_MSG_IN] = in->l2_rd_msg_in[i].u.val_u64;
			hwcnt_tbl[PERF_MEMSYS(i) + L2_SNP_MSG_IN] = in->l2_snp_msg_in[i].u.val_u64;
			hwcnt_tbl[PERF_MEMSYS(i) + L2_WR_MSG_IN] = in->l2_wr_msg_in[i].u.val_u64;
			hwcnt_tbl[PERF_MEMSYS(i) + L2_RD_MSG_OUT] = in->l2_rd_msg_out[i].u.val_u64;
			hwcnt_tbl[PERF_MEMSYS(i) + L2_READ_LOOKUP] =
				in->l2_read_lookup[i].u.val_u64;
			hwcnt_tbl[PERF_MEMSYS(i) + L2_EXT_READ_NOSNP] =
				in->l2_ext_read_nosnp[i].u.val_u64;
			hwcnt_tbl[PERF_MEMSYS(i) + L2_EXT_WRITE_NOSNP_FULL] =
				in->l2_ext_write_nosnp_full[i].u.val_u64;
			hwcnt_tbl[PERF_MEMSYS(i) + L2_RD_MSG_IN_STALL] =
				in->l2_rd_msg_in_stall[i].u.val_u64;
			hwcnt_tbl[PERF_MEMSYS(i) + L2_EXT_WRITE] = in->l2_ext_write[i].u.val_u64;
		}

		for (i = 0; i < num_cores; ++i) {
			/* TEX_FILT_NUM_OPERATIONS */
			hwcnt_tbl[PERF_SC(num_l2_slices, i) + TEX_FILT_NUM_OPERATIONS] =
				in->tex_filt_num_operations[i].u.val_u64;
			/* EXEC_INSTR_FMA */
			hwcnt_tbl[PERF_SC(num_l2_slices, i) + EXEC_INSTR_FMA] =
				in->exec_instr_fma[i].u.val_u64;
			/* EXEC_INSTR_MSG */
			hwcnt_tbl[PERF_SC(num_l2_slices, i) + EXEC_INSTR_MSG] =
				in->exec_instr_msg[i].u.val_u64;
			hwcnt_tbl[PERF_SC(num_l2_slices, i) + EXEC_INSTR_SFU] =
				in->exec_instr_sfu[i].u.val_u64;
			hwcnt_tbl[PERF_SC(num_l2_slices, i) + EXEC_INSTR_CVT] =
				in->exec_instr_cvt[i].u.val_u64;
			hwcnt_tbl[PERF_SC(num_l2_slices, i) + FRAG_QUADS_EZS_UPDATE] =
				in->frag_quads_ezs_update[i].u.val_u64;
			hwcnt_tbl[PERF_SC(num_l2_slices, i) + FRAG_STARVING] =
				in->frag_starving[i].u.val_u64;
			hwcnt_tbl[PERF_SC(num_l2_slices, i) + FRAG_PARTIAL_QUADS_RAST] =
				in->frag_partial_quads_rast[i].u.val_u64;
			hwcnt_tbl[PERF_SC(num_l2_slices, i) + FULL_QUAD_WARPS] =
				in->full_quad_warps[i].u.val_u64;
			hwcnt_tbl[PERF_SC(num_l2_slices, i) + LS_MEM_READ_SHORT] =
				in->ls_mem_read_short[i].u.val_u64;
			hwcnt_tbl[PERF_SC(num_l2_slices, i) + LS_MEM_WRITE_SHORT] =
				in->ls_mem_write_short[i].u.val_u64;
			hwcnt_tbl[PERF_SC(num_l2_slices, i) + VARY_SLOT_16] =
				in->vary_slot_16[i].u.val_u64;
			hwcnt_tbl[PERF_SC(num_l2_slices, i) + BEATS_RD_LSC_EXT] =
				in->beats_rd_lsc_ext[i].u.val_u64;
			hwcnt_tbl[PERF_SC(num_l2_slices, i) + BEATS_RD_TEX] =
				in->beats_rd_tex[i].u.val_u64;
			hwcnt_tbl[PERF_SC(num_l2_slices, i) + BEATS_RD_TEX_EXT] =
				in->beats_rd_tex_ext[i].u.val_u64;
			hwcnt_tbl[PERF_SC(num_l2_slices, i) + FRAG_QUADS_COARSE] =
				in->frag_quads_coarse[i].u.val_u64;
			hwcnt_tbl[PERF_SC(num_l2_slices, i) + EXEC_STARVE_ARITH] =
				in->exec_starve_arith[i].u.val_u64;
			hwcnt_tbl[PERF_SC(num_l2_slices, i) + TEX_TFCH_CLK_STALLED] =
				in->tex_tfch_clk_stalled[i].u.val_u64;
			hwcnt_tbl[PERF_SC(num_l2_slices, i) + RT_RAYS_STARTED] =
				in->rt_rays_started[i].u.val_u64;
			hwcnt_tbl[PERF_SC(num_l2_slices, i) + TEX_CFCH_NUM_L1_CT_OPERATIONS] =
				in->tex_cfch_num_l1_ct_operations[i].u.val_u64;
			hwcnt_tbl[PERF_SC(num_l2_slices, i) + EXEC_INSTR_SLOT1] =
				in->exec_instr_slot1[i].u.val_u64;
			hwcnt_tbl[PERF_SC(num_l2_slices, i) + EXEC_ISSUE_SLOT_ANY] =
				in->exec_issue_slot_any[i].u.val_u64;
		}

#if IS_ENABLED(CONFIG_MALI_NO_MALI) && MALI_UNIT_TEST
		gpu_model_set_dummy_prfcnt_cores(kbdev, (1ULL << num_l2_slices) - 1,
						 (1ULL << num_cores) - 1);
#endif

		pr_debug("Setting dummy counter values\n");
		gpu_model_set_dummy_prfcnt_kernel_sample(hwcnt_tbl, hwcnt_size);
	}

	return err;
}

/**
 * mali_kutf_ipa_sample_dummy() - Calculate dynamic power figure with preloaded
 *                                performance counter values
 * @context:	KUTF context
 *
 * Ensure the calculated dynamic power figure matches with no crashes
 * or assertion failures.
 */
static void mali_kutf_ipa_sample_dummy(struct kutf_context *context)
{
	struct kutf_ipa_fixture_data *data = context->fixture;
	struct kbase_ipa_model *old_model;
	struct kbase_device *kbdev = data->kbdev;
	u32 power;
	int err;
	struct kutf_ipa_in *in;
	struct kbase_ipa_model *simple_model;

	pr_debug("Test start\n");

	err = kutf_ipa_in_new(context, &in);
	if (err)
		return;

	kbase_simple_power_model_set_dummy_temp((int)in->temp.u.val_u64);

	kbase_ipa_model_param_set_s32(data->ipa_model, "scale", (s32)in->scale.u.val_u64);

	/* If we are using one of the counter-based power models, then the
	 * fallback model will be used for static power. If we are using the
	 * simple power model, then the model created at data->ipa_model will be
	 * used for static power. Ensure that the static power coefficients are
	 * sent to the correct model.
	 */
	if (data->ipa_model->ops == &kbase_simple_ipa_model_ops)
		simple_model = data->ipa_model;
	else
		simple_model = get_fallback_power_model(kbdev);

	kbase_ipa_model_param_set_s32(simple_model, "static-coefficient",
				      (s32)in->static_coefficient.u.val_u64);
	kbase_ipa_model_param_set_s32(simple_model, "dynamic-coefficient",
				      (s32)in->dynamic_coefficient.u.val_u64);
	kbase_ipa_model_param_set_s32(simple_model, "ts.0", (s32)in->ts[0].u.val_u64);
	kbase_ipa_model_param_set_s32(simple_model, "ts.1", (s32)in->ts[1].u.val_u64);
	kbase_ipa_model_param_set_s32(simple_model, "ts.2", (s32)in->ts[2].u.val_u64);
	kbase_ipa_model_param_set_s32(simple_model, "ts.3", (s32)in->ts[3].u.val_u64);

	set_power_model(kbdev, data->ipa_model, &old_model);

	/* Sleep until the temperature has been read. */
	msleep(WAIT_FOR_MODEL_MS);

	/* Request dynamic power */
	pr_debug("IPA\n");

	err = kbase_ipa_real_power(in, context, &power);
	if (err) {
		kutf_test_fail(context, "ipa_real_power returned an error");
	} else {
		/* Send power value */
		(void)kutf_helper_send_named_u64(context, IPA_OUT_NAME_POWER, power);
	}

	restore_power_model(kbdev, old_model);
	pr_debug("Test end\n");
}
#else /* !CONFIG_MALI_NO_MALI */
static void mali_kutf_ipa_sample_dummy(struct kutf_context *context)
{
	kutf_test_skip(context);
}
#endif /* CONFIG_MALI_NO_MALI */

struct suite_list {
	const char *name;
	u32 gpu_id;
} suite_list[] = {
	{ IPA_SUITE_NAME_TODX, GPU_ID2_PRODUCT_TODX },
	{ IPA_SUITE_NAME_TGRX, GPU_ID2_PRODUCT_TGRX },
	{ IPA_SUITE_NAME_TVAX, GPU_ID2_PRODUCT_TVAX },
	{ IPA_SUITE_NAME_TTUX, GPU_ID2_PRODUCT_TTUX },
	{ IPA_SUITE_NAME_TTIX, GPU_ID2_PRODUCT_TTIX },
	{ IPA_SUITE_NAME_TKRX, GPU_ID2_PRODUCT_TKRX },
};

/**
 * mali_kutf_ipa_unit_test_main_init() - Module entry point for this test.
 *
 * Return: 0 on success.
 */
static int __init mali_kutf_ipa_unit_test_main_init(void)
{
	struct kutf_suite *suite;
	unsigned int filters;
	int i;
	union kutf_callback_data data;

	pr_debug("Creating app\n");
	ipa_app = kutf_create_application(IPA_APP_NAME);

	for (i = 0; i < ARRAY_SIZE(suite_list); i++) {
		data.u32_value = suite_list[i].gpu_id;

		pr_debug("Create suite %s\n", suite_list[i].name);
		suite = kutf_create_suite_with_filters_and_data(ipa_app, suite_list[i].name,
								IPA_SUITE_FIXTURES,
								mali_kutf_ipa_unit_create_fixture,
								mali_kutf_ipa_unit_remove_fixture,
								KUTF_F_TEST_GENERIC, data);

		filters = suite->suite_default_flags;
		kutf_add_test_with_filters(suite, 0x0, IPA_UNIT_TEST_0, mali_kutf_ipa_sample_dummy,
					   filters);
	}

	pr_debug("Init complete\n");
	return 0;
}

/**
 * mali_kutf_ipa_unit_test_main_exit() - Module exit point for this test.
 */
static void __exit mali_kutf_ipa_unit_test_main_exit(void)
{
	pr_debug("Exit start\n");
	kutf_destroy_application(ipa_app);
	pr_debug("Exit complete\n");
}

#else

static int __init mali_kutf_ipa_unit_test_main_init(void)
{
	return 0;
}

static void __exit mali_kutf_ipa_unit_test_main_exit(void)
{
}

#endif

module_init(mali_kutf_ipa_unit_test_main_init);
module_exit(mali_kutf_ipa_unit_test_main_exit);

MODULE_LICENSE("GPL");
