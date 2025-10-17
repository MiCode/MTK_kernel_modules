// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018 MediaTek Inc.
 *
 */
/*include linux common header*/
#include <linux/module.h>
#include <linux/of_device.h>
#include <mt-plat/aee.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
#include "linux/vmalloc.h"
/*include imgsys header*/
#include "mtk_imgsys-dev.h"
#include "mtk_imgsys_frm_sync_isp8.h"
#include "mtk_imgsys_frm_sync_internal_isp8.h"


static struct sw_sync_token_pool_dpe dpe_sync_token_pool;
static struct sw_sync_token_pool_imgsys imgsys_sync_token_pool;
static struct buf_ofst_table_t dpe_gcebuf_ofst_tb_n; /*dpe*/
static struct buf_ofst_table_t imgsys_gcebuf_ofst_tb_n; /*normal*/
static int dpe_init_exist_flag;
static int imgsys_init_exist_flag;
static int vsdof_algo_exist_flag;
static int mcnr_algo_exist_flag;
static int vsdof_init_count;
static int mcnr_init_count;
static int MAX_GCE_RING_DPE = MAX_GCE_NORRING_DPE;
static int MAX_GCE_RING_IMGSYS = MAX_GCE_NORRING_IMGSYS;
static int dpe_gcebuf_index;


DEFINE_HASHTABLE(imgsys_token_map_index_table, 9);
DEFINE_HASHTABLE(dpe_token_map_index_table, 4);
DEFINE_HASHTABLE(mae_token_map_index_table, 9);

static int mcnr_init_isp8(struct mtk_img_frm_sync *mtk_img_frm_sync_dev)
{
	int i = 0;
		/*sw frame sync pool*/
	if (mcnr_algo_exist_flag == 0) {
		imgsys_sync_token_pool.pool.r_idx = -1;
		imgsys_sync_token_pool.pool.r_max = IMGSYS_SYNC_TOKEN_MAX;
		imgsys_sync_token_pool.pool.idx_pool = vzalloc(IMGSYS_SYNC_TOKEN_MAX * sizeof(int));
		if(imgsys_sync_token_pool.pool.idx_pool == NULL) {
			dev_err(mtk_img_frm_sync_dev->dev,
				"sync_token_pool.pool.idx_pool malloc got NULL\n");
			return -1;
		}
		for (i = 0 ; i < IMGSYS_SYNC_TOKEN_MAX ; i++){
			imgsys_sync_token_pool.token_pool[i].token_id = IMGSYS_SYNC_TOKEN_POOL_1 + i;
			imgsys_sync_token_pool.token_pool[i].status = buf_st_avai;
			imgsys_sync_token_pool.pool.idx_pool[i] = i;
		}
		mutex_init(&imgsys_sync_token_pool.pool.buf_pool_lock);
		mutex_init(&mcnr_tokenmap_lock);
		for (i = 0 ; i < IMGSYS_ALL_SYNC_TOKEN_MAX ; i++) {
			mcnr_tokenmap[i].type = sync_type_none;
			mcnr_tokenmap[i].hw_token = TOKEN_INVALID_ID;
			mcnr_tokenmap[i].frm_owner = 0x0;
			mcnr_tokenmap[i].imgstm_inst = 0x0;
			mcnr_tokenmap[i].frame_no = 0;
			mcnr_tokenmap[i].request_no = 0;
			mcnr_tokenmap[i].request_fd = 0;
			mcnr_tokenmap[i].stage = 0;
			mcnr_tokenmap[i].subfrm_sidx = 0;
			mcnr_tokenmap[i].evt_order = 0;
			mcnr_tokenmap[i].evt_histb_idx = 0;
			mcnr_tokenmap[i].hw_comb = 0x0;
		}
		mcnr_algo_exist_flag = 1;
	}

	return 0;
}

static int vsdof_init_isp8(struct mtk_img_frm_sync *mtk_img_frm_sync_dev)
{
	int i = 0;
		/*sw frame sync pool*/
	if (vsdof_algo_exist_flag == 0) {
		dpe_sync_token_pool.pool.r_idx = -1;
		dpe_sync_token_pool.pool.r_max = DPE_SYNC_TOKEN_MAX;
		dpe_sync_token_pool.pool.idx_pool = vzalloc(DPE_SYNC_TOKEN_MAX * sizeof(int));
		if(dpe_sync_token_pool.pool.idx_pool == NULL) {
			dev_err(mtk_img_frm_sync_dev->dev,
				"sync_token_pool.pool.idx_pool malloc got NULL\n");
			return -1;
		}
		for (i = 0 ; i < DPE_SYNC_TOKEN_MAX ; i++){
			dpe_sync_token_pool.token_pool[i].token_id = IMGSYS_DPE_SYNC_TOKEN_DPE_POOL_1 + i;
			dpe_sync_token_pool.token_pool[i].status = buf_st_avai;
			dpe_sync_token_pool.pool.idx_pool[i] = i;
		}
		dpe_gcebuf_index = 0;
		mutex_init(&dpe_sync_token_pool.pool.buf_pool_lock);
		mutex_init(&vsdof_tokenmap_lock);
		for (i = 0 ; i < DPE_SYNC_TOKEN_MAX ; i++) {
			vsdof_tokenmap[i].type = sync_type_none;
			vsdof_tokenmap[i].hw_token = TOKEN_INVALID_ID;
			vsdof_tokenmap[i].frm_owner = 0x0;
			vsdof_tokenmap[i].imgstm_inst = 0x0;
			vsdof_tokenmap[i].frame_no = 0;
			vsdof_tokenmap[i].request_no = 0;
			vsdof_tokenmap[i].request_fd = 0;
			vsdof_tokenmap[i].stage = 0;
			vsdof_tokenmap[i].subfrm_sidx = 0;
			vsdof_tokenmap[i].evt_order = 0;
			vsdof_tokenmap[i].evt_histb_idx = 0;
			vsdof_tokenmap[i].hw_comb = 0x0;
		}
		vsdof_algo_exist_flag = 1;
	}

	return 0;
}

static int mcnr_reset_tbl(void)
{
	int i = 0;

	for (i = 0 ; i < IMGSYS_SYNC_TOKEN_MAX ; i++) {
		imgsys_sync_token_pool.token_pool[i].token_id = IMGSYS_SYNC_TOKEN_POOL_1 + i;
		imgsys_sync_token_pool.token_pool[i].status = buf_st_avai;
		imgsys_sync_token_pool.pool.idx_pool[i] = i;
	}

	for (i = 0 ; i < IMGSYS_ALL_SYNC_TOKEN_MAX ; i++) {
		mcnr_tokenmap[i].type = sync_type_none;
		mcnr_tokenmap[i].hw_token = TOKEN_INVALID_ID;
		mcnr_tokenmap[i].frm_owner = 0x0;
		mcnr_tokenmap[i].imgstm_inst = 0x0;
		mcnr_tokenmap[i].frame_no = 0;
		mcnr_tokenmap[i].request_no = 0;
		mcnr_tokenmap[i].request_fd = 0;
		mcnr_tokenmap[i].stage = 0;
		mcnr_tokenmap[i].subfrm_sidx = 0;
		mcnr_tokenmap[i].evt_order = 0;
		mcnr_tokenmap[i].evt_histb_idx = 0;
		mcnr_tokenmap[i].hw_comb = 0x0;
	}

	return 0;
}

static int vsdof_reset_tbl(void)
{
	int i = 0;

	dpe_sync_token_pool.pool.r_idx = -1;
	dpe_sync_token_pool.pool.r_max = DPE_SYNC_TOKEN_MAX;
	for (i = 0 ; i < DPE_SYNC_TOKEN_MAX ; i++){
		dpe_sync_token_pool.token_pool[i].token_id = IMGSYS_DPE_SYNC_TOKEN_DPE_POOL_1 + i;
		dpe_sync_token_pool.token_pool[i].status = buf_st_avai;
		dpe_sync_token_pool.pool.idx_pool[i] = i;
	}

	for (i = 0 ; i < DPE_SYNC_TOKEN_MAX ; i++) {
		vsdof_tokenmap[i].type = sync_type_none;
		vsdof_tokenmap[i].hw_token = TOKEN_INVALID_ID;
		vsdof_tokenmap[i].frm_owner = 0x0;
		vsdof_tokenmap[i].imgstm_inst = 0x0;
		vsdof_tokenmap[i].frame_no = 0;
		vsdof_tokenmap[i].request_no = 0;
		vsdof_tokenmap[i].request_fd = 0;
		vsdof_tokenmap[i].stage = 0;
		vsdof_tokenmap[i].subfrm_sidx = 0;
		vsdof_tokenmap[i].evt_order = 0;
		vsdof_tokenmap[i].evt_histb_idx = 0;
		vsdof_tokenmap[i].hw_comb = 0x0;
	}
	return 0;
}

int mtk_img_frm_sync_init_isp8(struct mtk_img_frm_sync *mtk_img_frm_sync_dev)
{
	int i = 0;

	if (imgsys_init_exist_flag == 0) {
		imgsys_gcebuf_ofst_tb_n.info =
			vzalloc(MAX_GCE_RING_IMGSYS * sizeof(struct buf_ofst_info_t));
		for (i = 0;i < MAX_GCE_RING_IMGSYS;i++) {
			imgsys_gcebuf_ofst_tb_n.info[i].buf_st = buf_st_avai;
			imgsys_gcebuf_ofst_tb_n.info[i].used_sw_token_cnt = 0;
			for(int j = 0 ; j < IMGSYS_SYNC_TOKEN_MAX ; j++) {
				imgsys_gcebuf_ofst_tb_n.info[i].used_sw_token[j] = -1;
				imgsys_gcebuf_ofst_tb_n.info[i].need_release_token[j] = false;
			}
		}
		mutex_init(&imgsys_gcebuf_ofst_tb_n.pool.buf_pool_lock);
		imgsys_init_exist_flag = 1;
	}

	if (dpe_init_exist_flag == 0) {
		dpe_gcebuf_ofst_tb_n.info = vzalloc(MAX_GCE_RING_DPE * sizeof(struct buf_ofst_info_t));
		for (i = 0;i < MAX_GCE_RING_DPE;i++) {
			dpe_gcebuf_ofst_tb_n.info[i].buf_st = buf_st_avai;
			dpe_gcebuf_ofst_tb_n.info[i].used_sw_token_cnt = 0;
			for(int j = 0 ; j < DPE_SYNC_TOKEN_MAX ; j++) {
				dpe_gcebuf_ofst_tb_n.info[i].used_sw_token[j] = -1;
				dpe_gcebuf_ofst_tb_n.info[i].need_release_token[j] = false;
			}
		}
		mutex_init(&dpe_gcebuf_ofst_tb_n.pool.buf_pool_lock);
		dpe_init_exist_flag = 1;
	}

	mcnr_init_isp8(mtk_img_frm_sync_dev);
	vsdof_init_isp8(mtk_img_frm_sync_dev);

	return 0;
}

int mtk_img_frm_sync_uninit_isp8(struct mtk_img_frm_sync *mtk_img_frm_sync_dev)
{
	if (imgsys_init_exist_flag == 1) {
		vfree(imgsys_gcebuf_ofst_tb_n.info);
		mutex_destroy(&imgsys_gcebuf_ofst_tb_n.pool.buf_pool_lock);
		imgsys_init_exist_flag = 0;
	}
	if (dpe_init_exist_flag == 1) {
		vfree(dpe_gcebuf_ofst_tb_n.info);
		mutex_destroy(&dpe_gcebuf_ofst_tb_n.pool.buf_pool_lock);
		dpe_init_exist_flag = 0;
	}
	if (mcnr_algo_exist_flag == 1) {
		vfree(imgsys_sync_token_pool.pool.idx_pool);
		mutex_destroy(&imgsys_sync_token_pool.pool.buf_pool_lock);
		mutex_destroy(&mcnr_tokenmap_lock);
		mcnr_algo_exist_flag = 0;
	}
	if (vsdof_algo_exist_flag == 1) {
		vfree(dpe_sync_token_pool.pool.idx_pool);
		mutex_destroy(&dpe_sync_token_pool.pool.buf_pool_lock);
		mutex_destroy(&vsdof_tokenmap_lock);
		vsdof_algo_exist_flag = 0;
	}
	return 0;
}

int dpe_event_init_isp8(struct mtk_img_frm_sync *mtk_img_frm_sync_dev)
{
	int idx = 0;

	for (idx = 0; idx < IMGSYS_DPE_EVENT_MAX; idx++) {
		of_property_read_u16(mtk_img_frm_sync_dev->dev->of_node,
			imgsys_event[idx].dts_name,
			&imgsys_event[idx].event);
		pr_info("%s: event idx %d is (%s, %d)\n", __func__,
			idx, imgsys_event[idx].dts_name,
			imgsys_event[idx].event);
	}

	dpe_init_exist_flag = 0;
	imgsys_init_exist_flag = 0;
	vsdof_algo_exist_flag = 0;
	mcnr_algo_exist_flag = 0;
	ITF_LOG_LEVEL = 0;
	vsdof_init_count = 0;
	mcnr_init_count = 0;

	return 0;
}

int dpe_open_isp8(void)
{
	if (EVT_HISTB_ON) {
		evtHisTb_DPE = (struct event_history_t *)
			vzalloc(EVT_HISTB_MAX_USER * sizeof(struct event_history_t));
		if (evtHisTb_DPE == NULL) {
			pr_info("evtHisTb malloc fail\n");
			return -1;
		}
	}
	pr_info("mtk-img-frm-sync-open\n");
	return 0;
}

static void vsdof_tbl_dump(struct mtk_img_frm_sync *mtk_img_frm_sync_dev)
{
	int j = 0;

	for (j = 0 ; j < DPE_SYNC_TOKEN_MAX ; j++) {
		if (vsdof_tokenmap[j].hw_token != TOKEN_INVALID_ID) {
			dev_info(mtk_img_frm_sync_dev->dev,
			"token swkey(%d), hwfence(%d) type(%d), frmowner(%s), imgstm_inst(%llx),",
			j + 1, vsdof_tokenmap[j].hw_token, vsdof_tokenmap[j].type,
			(char *)(&(vsdof_tokenmap[j].frm_owner)), vsdof_tokenmap[j].imgstm_inst);
			pr_info(" frame_no(%d), stg(%d), subfrm_idx(%d), hw_comb(0x%x), evt_order(%d)\n",
				vsdof_tokenmap[j].frame_no, vsdof_tokenmap[j].stage,
				vsdof_tokenmap[j].subfrm_sidx,
				vsdof_tokenmap[j].hw_comb, vsdof_tokenmap[j].evt_order);
		}
	}

	for (j = 0 ; j < DPE_SYNC_TOKEN_MAX ; j++){
		if (dpe_sync_token_pool.token_pool[j].status != buf_st_avai) {
			dev_info(mtk_img_frm_sync_dev->dev,
			"dpe sync token table val/r_max(%d/%d)",
			dpe_sync_token_pool.token_pool[j].token_id,
			dpe_sync_token_pool.pool.r_max
			);
		}
	}
}

int init_isp8(struct mtk_img_frm_sync *mtk_img_frm_sync_dev, struct group k_group)
{
	if (k_group.algo_group_id == algo_all) {
		mcnr_init_count++;
		vsdof_init_count++;
	} else if (k_group.algo_group_id == algo_mcnr) {
		mcnr_init_count++;
	} else if (k_group.algo_group_id == algo_vsdof) {
		if (((mcnr_init_count == 0) && (vsdof_init_count == 0)) ||
		((mcnr_init_count == 1) && (vsdof_init_count == 1)))
			vsdof_init_count++;
		else
			dev_err(mtk_img_frm_sync_dev->dev, "user init flow not current(%d/%d)",
				mcnr_init_count, vsdof_init_count);
	}
	dev_info(mtk_img_frm_sync_dev->dev, "re-init table index stage(%d/%d)\n",
		mcnr_init_count, vsdof_init_count);
	return 0;
}

int uninit_isp8(struct mtk_img_frm_sync *mtk_img_frm_sync_dev, struct group k_group)
{
	if (k_group.algo_group_id == algo_all) {
		mcnr_init_count--;
		vsdof_init_count--;
	} else if (k_group.algo_group_id == algo_mcnr) {
		mcnr_init_count--;
	} else if (k_group.algo_group_id == algo_vsdof) {
		vsdof_init_count--;
	}
	if (mcnr_init_count == 0)
		mcnr_reset_tbl();
	if (vsdof_init_count == 0) {
		vsdof_tbl_dump(mtk_img_frm_sync_dev);
		vsdof_reset_tbl();
	}

	dpe_gcebuf_index = 0;
	dev_info(mtk_img_frm_sync_dev->dev, "uninit stage(%d/%d)\n",
		mcnr_init_count, vsdof_init_count);
	return 0;
}

static unsigned int imgsys_frm_sync_which_event(uint32_t event)
{
	unsigned int ret = 0;

	if (TOKEN_GROUP(event) == mtk_imgsys_frm_sync_event_group_vsdof)
		ret = mtk_imgsys_frm_sync_event_group_vsdof;
	else if (TOKEN_GROUP(event) == mtk_imgsys_frm_sync_event_group_aiseg)
		ret = mtk_imgsys_frm_sync_event_group_aiseg;
	else
		ret = mtk_imgsys_frm_sync_event_group_mcnr;

	return ret;
}

static int mcnr_token_dump(struct mtk_img_frm_sync *mtk_img_frm_sync_dev,
	struct tokenmap_t *k_tokenmap, unsigned int acq_idx)
{
	int j = 0;

	if (ITF_LOG_LEVEL >= 1) {
		dev_info(mtk_img_frm_sync_dev->dev,
			".no available sw token: token_ridx(%d), token_id(%d), avail_token_num:%d\n",
			acq_idx, imgsys_sync_token_pool.token_pool[acq_idx].token_id,
			imgsys_sync_token_pool.pool.r_max);
		dev_info(mtk_img_frm_sync_dev->dev, "===dump token map table===\n");
		for (j = 0 ; j < IMGSYS_ALL_SYNC_TOKEN_MAX ; j++) {
			if (k_tokenmap[j].hw_token != TOKEN_INVALID_ID) {
				dev_info(mtk_img_frm_sync_dev->dev,
					"token swkey(%d), hwfence(%d) type(%d), frmowner(%s), imgstm_inst(%llx)",
					j + 1, k_tokenmap[j].hw_token, k_tokenmap[j].type,
					(char *)(&(k_tokenmap[j].frm_owner)), k_tokenmap[j].imgstm_inst);
				pr_info("frm_no(%d), stg(%d), subfrm_idx(%d), hw_comb(0x%x), evt_ord(%d)\n",
					k_tokenmap[j].frame_no, k_tokenmap[j].stage,
					k_tokenmap[j].subfrm_sidx,
					k_tokenmap[j].hw_comb, k_tokenmap[j].evt_order);
			}
		}
	}
	return 0;
}

static int acquire_token_from_sync_token_pool_mcnr(struct mtk_img_frm_sync *mtk_img_frm_sync_dev,
	struct sw_sync_token_pool_imgsys *imgsys_token_pool, struct tokenmap_t *k_tokenmap)
{
	/* acquire available sw token */
	int acq_r_idx = 0;
	int token_id = 0;
	unsigned long start_time = 0;
	int i = 0;
	/* need mutex */
	if (!mcnr_algo_exist_flag)
		return 0;
	mutex_lock(&imgsys_sync_token_pool.pool.buf_pool_lock);
	start_time = jiffies;
	for (i=0;i<100;i++) {
		imgsys_sync_token_pool.pool.r_idx = imgsys_sync_token_pool.pool.idx_pool[0];
		acq_r_idx = imgsys_sync_token_pool.pool.r_idx;
		if (imgsys_sync_token_pool.token_pool[acq_r_idx].status != buf_st_avai) {
			/*wait empty event*/
			mcnr_token_dump(mtk_img_frm_sync_dev, k_tokenmap, acq_r_idx);
			if (jiffies_to_msecs(jiffies - start_time) > 1000) {
				mutex_unlock(&imgsys_sync_token_pool.pool.buf_pool_lock);
				return 0;
			}
		} else {
			break;
		}
	}
	imgsys_sync_token_pool.pool.idx_pool[0] =
		imgsys_sync_token_pool.pool.idx_pool[imgsys_sync_token_pool.pool.r_max - 1];
	imgsys_sync_token_pool.pool.idx_pool[imgsys_sync_token_pool.pool.r_max - 1] = acq_r_idx;
	imgsys_sync_token_pool.pool.r_max -= 1;

	if (imgsys_sync_token_pool.pool.r_max < imgsys_sync_token_pool.pool.lowest_available_num)
		imgsys_sync_token_pool.pool.lowest_available_num = imgsys_sync_token_pool.pool.r_max;

	imgsys_sync_token_pool.token_pool[acq_r_idx].status = buf_st_occu;
	token_id = imgsys_sync_token_pool.token_pool[acq_r_idx].token_id;
	dev_info(mtk_img_frm_sync_dev->dev, "r_idx/pool[0]/tokenid(%d/%d/%d)",
		acq_r_idx, imgsys_sync_token_pool.pool.idx_pool[0], token_id);
	mutex_unlock(&imgsys_sync_token_pool.pool.buf_pool_lock);

	return token_id;
}

static int vsdof_token_dump(struct mtk_img_frm_sync *mtk_img_frm_sync_dev,
	struct tokenmap_t *k_tokenmap, unsigned int acq_idx)
{
	int j = 0;

	if (ITF_LOG_LEVEL >= 0) {
		dev_info(mtk_img_frm_sync_dev->dev,
			".no available sw token: token_ridx(%d), token_id(%d), avail_token_num:%d\n",
			acq_idx, dpe_sync_token_pool.token_pool[acq_idx].token_id,
			dpe_sync_token_pool.pool.r_max);
		dev_info(mtk_img_frm_sync_dev->dev, "===dump token map table===\n");
		for (j = 0 ; j < DPE_SYNC_TOKEN_MAX ; j++) {
			if (k_tokenmap[j].hw_token != TOKEN_INVALID_ID) {
				dev_info(mtk_img_frm_sync_dev->dev,
				"token swkey(%d), hwfence(%d) type(%d), frmowner(%s), imgstm_inst(%llx),",
				j + 1, k_tokenmap[j].hw_token, k_tokenmap[j].type,
				(char *)(&(k_tokenmap[j].frm_owner)), k_tokenmap[j].imgstm_inst);
				pr_info(" frame_no(%d), stg(%d), subfrm_idx(%d), hw_comb(0x%x), evt_order(%d)\n",
					k_tokenmap[j].frame_no, k_tokenmap[j].stage, k_tokenmap[j].subfrm_sidx,
					k_tokenmap[j].hw_comb, k_tokenmap[j].evt_order);
			}
		}
	}
	return 0;
}

static int acquire_token_from_sync_token_pool_vsdof(struct mtk_img_frm_sync *mtk_img_frm_sync_dev,
	struct sw_sync_token_pool_dpe *dpe_token_pool, struct tokenmap_t *k_tokenmap)
{
	/* acquire available sw token */
	unsigned int acq_r_idx = 0;
	unsigned int token_id = 0;
	unsigned long start_time = 0;
	int i = 0;
	/* need mutex */
	if (!vsdof_algo_exist_flag)
		return 0;
	mutex_lock(&dpe_sync_token_pool.pool.buf_pool_lock);
	start_time = jiffies;
	for (i=0;i<100;i++) {
		dpe_sync_token_pool.pool.r_idx = dpe_sync_token_pool.pool.idx_pool[0];
		acq_r_idx = dpe_sync_token_pool.pool.r_idx;
		if (dpe_sync_token_pool.token_pool[acq_r_idx].status != buf_st_avai) {
			/*wait empty event*/
			vsdof_token_dump(mtk_img_frm_sync_dev, k_tokenmap, acq_r_idx);
			if (jiffies_to_msecs(jiffies - start_time) > 1000) {
				mutex_unlock(&dpe_sync_token_pool.pool.buf_pool_lock);
				return 0;
			}
		} else {
			break;
		}
	}
	dpe_sync_token_pool.pool.idx_pool[0] =
		dpe_sync_token_pool.pool.idx_pool[dpe_sync_token_pool.pool.r_max - 1];
	dpe_sync_token_pool.pool.idx_pool[dpe_sync_token_pool.pool.r_max - 1] = acq_r_idx;
	dpe_sync_token_pool.pool.r_max -= 1;
	if (dpe_sync_token_pool.pool.r_max < dpe_sync_token_pool.pool.lowest_available_num)
		dpe_sync_token_pool.pool.lowest_available_num = dpe_sync_token_pool.pool.r_max;
	dpe_sync_token_pool.token_pool[acq_r_idx].status = buf_st_occu;
	token_id = dpe_sync_token_pool.token_pool[acq_r_idx].token_id;
	if (ITF_LOG_LEVEL >= 1) {
		dev_info(mtk_img_frm_sync_dev->dev, "r_idx/pool[0]/token_id/r_max(%d/%d/%d/%d)",
			acq_r_idx, dpe_sync_token_pool.pool.idx_pool[0], token_id,
			dpe_sync_token_pool.pool.r_max);
	}
	mutex_unlock(&dpe_sync_token_pool.pool.buf_pool_lock);

	return token_id;
}

static int acquire_tokenmap_index(struct mtk_img_frm_sync *mtk_img_frm_sync_dev,
	int virtual_token_value, int algo_group)
{
	struct token_map_index_data_t *token_map_index_data;

	token_map_index_data = vzalloc(sizeof(struct token_map_index_data_t));
	if (token_map_index_data == NULL)
		return -ENOMEM;

	token_map_index_data->value = virtual_token_value;
	if (algo_group == mtk_imgsys_frm_sync_event_group_mcnr) {
		token_map_index_data->key = (virtual_token_value % imgsys_token_map_hash_size);
		hash_add(imgsys_token_map_index_table, &token_map_index_data->hnode,
			token_map_index_data->key);
	} else if (algo_group == mtk_imgsys_frm_sync_event_group_vsdof) {
		token_map_index_data->key = (virtual_token_value % dpe_token_map_hash_size);
		hash_add(dpe_token_map_index_table, &token_map_index_data->hnode,
			token_map_index_data->key);
		if (ITF_LOG_LEVEL >= 1)
			dev_info(mtk_img_frm_sync_dev->dev, "token_map_index(%d/%d)\n",
				virtual_token_value,
				token_map_index_data->key);
	} else
		dev_info(mtk_img_frm_sync_dev->dev, "%s-not support algo", __func__);
	return token_map_index_data->key;
}

static int acquire_frm_sync_tbl_index(struct mtk_img_frm_sync *mtk_img_frm_sync_dev,
	int tbl_index)
{
	int frm_sync_tbl_index;

	frm_sync_tbl_index = (tbl_index % dpe_token_map_hash_size);
	return frm_sync_tbl_index;
}

int Handler_frame_token_sync_imgsys_isp8(struct mtk_img_frm_sync *mtk_img_frm_sync_dev,
					struct imgsys_in_data *in_data, struct imgsys_out_data *out_data)
{
	/*uint32_t stoken_num = in_data->token_list.mSyncTokenNum;*/
	bool acquired = false;
	enum sync_event_type_e event_type;
	bool need_release = false;
	int need_token = false;
	int token_id = -1;
	int i = 0;
	struct buf_ofst_table_t *oft_tb = NULL;
	struct buf_ofst_info_t *oft_info = NULL;
	struct tokenmap_t *tokenmap = NULL;
	int token_cnt = 0;
	int tokenmap_keyidx = 0;
	/*struct token_list_k token_list[16];*/
	struct token_list_k token_list;
	int r_idx = in_data->r_idx;
	int algo_id = 0;
	struct mutex *tokenmap_lock = NULL;

	if (!imgsys_init_exist_flag)
		return 0;
	oft_tb = &imgsys_gcebuf_ofst_tb_n;
	oft_info = oft_tb->info;

	event_type = sync_type_none;
	need_release = false;
	token_id = -1;
	need_token = false;
	acquired = false;

	if (in_data->token_list.mSyncTokenList[i].type == imgsys_token_set) {
		dev_info(mtk_img_frm_sync_dev->dev, "imgsys-token-set(%d)\n",
			in_data->token_list.mSyncTokenList[i].token_value);
		event_type = sync_type_set;
		need_release = false;
		need_token = true;
		token_list.token_type = sync_type_set;
		if (imgsys_frm_sync_which_event(in_data->token_list.mSyncTokenList[i].token_value) ==
			mtk_imgsys_frm_sync_event_group_mcnr) {
			token_list.mSyncToken =
				in_data->token_list.mSyncTokenList[i].token_value;
			tokenmap = mcnr_tokenmap;
			algo_id = algo_mcnr;
			tokenmap_lock = &mcnr_tokenmap_lock;
		} else if (imgsys_frm_sync_which_event(in_data->token_list.mSyncTokenList[i].token_value) ==
			mtk_imgsys_frm_sync_event_group_vsdof) {
			token_list.mSyncToken =
				(in_data->token_list.mSyncTokenList[i].token_value & 0x0000FFFF);
			tokenmap = vsdof_tokenmap;
			algo_id = algo_vsdof;
			tokenmap_lock = &vsdof_tokenmap_lock;
		} else if (imgsys_frm_sync_which_event(in_data->token_list.mSyncTokenList[i].token_value) ==
			mtk_imgsys_frm_sync_event_group_aiseg) {
			token_list.mSyncToken =
				(in_data->token_list.mSyncTokenList[i].token_value & 0x0000FFFF);
			tokenmap = aiseg_tokenmap;
			algo_id = algo_aiseg;
			tokenmap_lock = &aiseg_tokenmap_lock;
		} else {
			dev_info(mtk_img_frm_sync_dev->dev, "not support algo");
			return -1;
		}
	} else {
		dev_info(mtk_img_frm_sync_dev->dev, "imgsys-token-wait(%d)\n",
			in_data->token_list.mSyncTokenList[i].token_value);
		event_type = sync_type_wait;
		need_release = true;
		need_token = true;
		token_list.token_type = sync_type_wait;
		if (imgsys_frm_sync_which_event(in_data->token_list.mSyncTokenList[i].token_value) ==
			mtk_imgsys_frm_sync_event_group_mcnr) {
			token_list.mSyncToken =
				in_data->token_list.mSyncTokenList[i].token_value;
			tokenmap = mcnr_tokenmap;
			algo_id = algo_mcnr;
			tokenmap_lock = &mcnr_tokenmap_lock;
		} else if (imgsys_frm_sync_which_event(in_data->token_list.mSyncTokenList[i].token_value) ==
			mtk_imgsys_frm_sync_event_group_vsdof) {
			token_list.mSyncToken =
				(in_data->token_list.mSyncTokenList[i].token_value & 0x0000FFFF);
			tokenmap = vsdof_tokenmap;
			algo_id = algo_vsdof;
			tokenmap_lock = &vsdof_tokenmap_lock;
		} else if (imgsys_frm_sync_which_event(in_data->token_list.mSyncTokenList[i].token_value) ==
			mtk_imgsys_frm_sync_event_group_aiseg) {
			token_list.mSyncToken =
				(in_data->token_list.mSyncTokenList[i].token_value & 0x0000FFFF);
			tokenmap = aiseg_tokenmap;
			algo_id = algo_aiseg;
			tokenmap_lock = &aiseg_tokenmap_lock;
		} else {
			dev_info(mtk_img_frm_sync_dev->dev, "not support algo");
			return -1;
		}
	}

	if (ITF_LOG_LEVEL >= 1) {
		dev_info(mtk_img_frm_sync_dev->dev, "token_key = %x(%d), need_token = %d\n",
			token_list.mSyncToken, event_type, need_token);
	}

	if (need_token) {
		tokenmap_keyidx = acquire_tokenmap_index(mtk_img_frm_sync_dev,
			token_list.mSyncToken, algo_id);
		if (tokenmap_keyidx < 0) {
			dev_info(mtk_img_frm_sync_dev->dev,
				"mcnr acquire map index fail(%d)\n", tokenmap_keyidx);
			return -ENOMEM;
		}
		//mutex lock

		mutex_lock(tokenmap_lock);
		//debug log
		if (ITF_LOG_LEVEL >= 2) {
			dev_info(mtk_img_frm_sync_dev->dev,
				"key/type(%d/%d) tokenMap[%d] = (%d, %d), need_token = %d, base/ofst(%d/%d)\n",
				token_list.mSyncToken, event_type,
				tokenmap_keyidx,
				tokenmap[tokenmap_keyidx].type,
				tokenmap[tokenmap_keyidx].hw_token,
				need_token, IMGSYS_DPE_TOKEN_BASE, IMGSYS_DPE_TOKEN_OFFSET);
		}

		if(tokenmap[tokenmap_keyidx].hw_token != TOKEN_INVALID_ID) {
			if (tokenmap[tokenmap_keyidx].type == event_type) {
				aee_kernel_exception("CRDISPATCH_KEY:IMGSYS_FRAME_SYNC",
					"DISPATCH:MW send continued key(%d) with wrong status(input:%d <-> Map:%d)",
					token_list.mSyncToken,
					token_list.token_type,
					tokenmap[tokenmap_keyidx].type);
				mutex_unlock(tokenmap_lock);
				return -1;
			}
			/*previous set/wait token_key get available hw_token before*/
			token_id = tokenmap[tokenmap_keyidx].hw_token;
			/* set token index to init value*/
			tokenmap[tokenmap_keyidx].type = imgsys_token_none;
			tokenmap[tokenmap_keyidx].hw_token = TOKEN_INVALID_ID;
			tokenmap[tokenmap_keyidx].frm_owner = 0;
			tokenmap[tokenmap_keyidx].imgstm_inst = 0;
			tokenmap[tokenmap_keyidx].frame_no = 0;
			tokenmap[tokenmap_keyidx].request_no = 0;
			tokenmap[tokenmap_keyidx].request_fd = 0;
			tokenmap[tokenmap_keyidx].stage = 0;
			tokenmap[tokenmap_keyidx].subfrm_sidx = 0;
			tokenmap[tokenmap_keyidx].evt_order = 0;
			tokenmap[tokenmap_keyidx].evt_histb_idx = -1;
			tokenmap[tokenmap_keyidx].hw_comb = 0;
			if (ITF_LOG_LEVEL >= 1) {
				dev_info(mtk_img_frm_sync_dev->dev,
				"key/type(%d/%d) clear tokenMap[%d] = (%d, %d), token_id = %d\n",
				token_list.mSyncToken, event_type,
				token_list.mSyncToken, tokenmap[tokenmap_keyidx].type,
				tokenmap[tokenmap_keyidx].hw_token, token_id);
			}
		} else {
			if (algo_id == algo_mcnr) {
				token_id = acquire_token_from_sync_token_pool_mcnr(mtk_img_frm_sync_dev,
					&imgsys_sync_token_pool, tokenmap);
			} else if(algo_id == algo_vsdof) {
				token_id = acquire_token_from_sync_token_pool_vsdof(mtk_img_frm_sync_dev,
					&dpe_sync_token_pool, tokenmap);
			}else {
				dev_info(mtk_img_frm_sync_dev->dev, "not support algo");
			}

			tokenmap[tokenmap_keyidx].type = event_type;
			tokenmap[tokenmap_keyidx].hw_token = token_id;
			tokenmap[tokenmap_keyidx].frm_owner = in_data->frm_owner;
			tokenmap[tokenmap_keyidx].imgstm_inst = in_data->imgstm_inst;
			tokenmap[tokenmap_keyidx].frame_no = in_data->frm_no;
			tokenmap[tokenmap_keyidx].request_no = in_data->req_no;
			tokenmap[tokenmap_keyidx].request_fd = in_data->req_fd;

			if (ITF_LOG_LEVEL >= 1) {
				dev_info(mtk_img_frm_sync_dev->dev,
					"key/type(%d/%d) update tokenMap[%d] = (%d, %d), token_id = %d\n",
					token_list.mSyncToken, event_type,
					tokenmap_keyidx,
					tokenmap[tokenmap_keyidx].type,
					tokenmap[tokenmap_keyidx].hw_token,
					token_id);
			}
		}
		mutex_unlock(tokenmap_lock);
		mutex_lock(&oft_tb->pool.buf_pool_lock);
		oft_info[r_idx].used_sw_token[token_cnt] = token_id;
		oft_info[r_idx].need_release_token[token_cnt] = need_release;
		oft_info[r_idx].used_sw_token_cnt += 1;
		oft_info[r_idx].gce_event_id = imgsys_event[token_id].event;
		dev_info(mtk_img_frm_sync_dev->dev, "imgsys-oft_info(%d/%d/%d/%d/%d/%d/%d/%d/%d)",
			token_cnt,
			r_idx,
			oft_info[r_idx].used_sw_token[token_cnt],
			oft_info[r_idx].need_release_token[token_cnt],
			oft_info[r_idx].used_sw_token_cnt,
			oft_info[r_idx].gce_event_id,
			dpe_sync_token_pool.pool.idx_pool[0],
			token_id,
			dpe_sync_token_pool.pool.r_max);
		token_cnt = oft_info[r_idx].used_sw_token_cnt;
		mutex_unlock(&oft_tb->pool.buf_pool_lock);
		out_data->event_id = imgsys_event[token_id].event;
		out_data->sw_ridx = r_idx;
	}
	return 0;
}

static int dpe_token_dump(struct mtk_img_frm_sync *mtk_img_frm_sync_dev, int acq_idx)
{
	int j = 0;

	dev_info(mtk_img_frm_sync_dev->dev,
		".no avail sw token: token_ridx(%d), token_id(%d), avail_token_num:%d\n",
		acq_idx, dpe_sync_token_pool.token_pool[acq_idx].token_id,
		dpe_sync_token_pool.pool.r_max);
	dev_info(mtk_img_frm_sync_dev->dev, "===dump token map table===\n");
	for (j = 0 ; j < DPE_SYNC_TOKEN_MAX ; j++) {
		if (vsdof_tokenmap[j].hw_token != TOKEN_INVALID_ID) {
			dev_info(mtk_img_frm_sync_dev->dev,
				"token swkey(%d), hwfence(%d) type(%d), frmowner(%s), imgstm_inst(%llx),",
				j + 1, vsdof_tokenmap[j].hw_token, vsdof_tokenmap[j].type,
				(char *)(&(vsdof_tokenmap[j].frm_owner)),
				vsdof_tokenmap[j].imgstm_inst);
			pr_info("frame_no(%d), stg(%d), subfrm_idx(%d), hw_comb(0x%x), evt_order(%d)\n",
				vsdof_tokenmap[j].frame_no, vsdof_tokenmap[j].stage,
				vsdof_tokenmap[j].subfrm_sidx,
				vsdof_tokenmap[j].hw_comb, vsdof_tokenmap[j].evt_order);
		}
	}
	return 0;
}

int Handler_frame_token_sync_DPE_isp8(struct mtk_img_frm_sync *mtk_img_frm_sync_dev,
					struct dpe_in_data *in_data, struct dpe_out_data *out_data)
{
	uint32_t stoken_num = in_data->token_info.mSyncTokenNum;
	bool acquired = false;
	enum sync_event_type_e event_type;
	bool need_release = false;
	int need_token = false;
	int token_id = -1;
	int i = 0, j = 0;
	unsigned int acq_r_idx = 0;
	struct buf_ofst_table_t *oft_tb = NULL;
	struct buf_ofst_info_t *oft_info = NULL;
	int token_cnt = 0;
	int tokenmap_keyidx = 0;
	/*struct token_list_k token_list[16];*/
	struct token_list_k token_list;
	int r_idx = 0;
	unsigned long start_time = 0;
	unsigned int gce_index;

	oft_tb = &dpe_gcebuf_ofst_tb_n;
	oft_info = oft_tb->info;
	gce_index = dpe_gcebuf_index++;

	event_type = sync_type_none;
	need_release = false;
	token_id = -1;
	need_token = false;
	acquired = false;

	if (in_data->token_info.mSyncTokenList[i].type == imgsys_token_set) {
		event_type = sync_type_set;
		need_release = false;
		need_token = true;
		token_list.token_type = sync_type_set;
		dev_info(mtk_img_frm_sync_dev->dev, "imgsys-dpe-token-set(%d)\n",
			in_data->token_info.mSyncTokenList[i].token_value);
		if (imgsys_frm_sync_which_event(in_data->token_info.mSyncTokenList[i].token_value))
			token_list.mSyncToken =
				(in_data->token_info.mSyncTokenList[i].token_value & 0x0000FFFF);
		else
			token_list.mSyncToken = in_data->token_info.mSyncTokenList[i].token_value;
	} else {
		event_type = sync_type_wait;
		need_release = true;
		need_token = true;
		token_list.token_type = sync_type_wait;
			dev_info(mtk_img_frm_sync_dev->dev, "imgsys-dpe-token-wait(%d/%d)\n",
				in_data->token_info.mSyncTokenList[i].token_value, need_release);
		if (imgsys_frm_sync_which_event(in_data->token_info.mSyncTokenList[i].token_value))
			token_list.mSyncToken =
				(in_data->token_info.mSyncTokenList[i].token_value & 0x0000FFFF);
		else
			token_list.mSyncToken = in_data->token_info.mSyncTokenList[i].token_value;
	}

	if (ITF_LOG_LEVEL >= 1) {
		dev_info(mtk_img_frm_sync_dev->dev, "token_key = %x(%d), need_token = %d\n",
			token_list.mSyncToken, event_type, need_token);
	}

	if (need_token) {
		tokenmap_keyidx = acquire_tokenmap_index(mtk_img_frm_sync_dev,
			token_list.mSyncToken, mtk_imgsys_frm_sync_event_group_vsdof);
		if (tokenmap_keyidx < 0) {
			dev_info(mtk_img_frm_sync_dev->dev,
				"vsdof acquire map index fail(%d)\n", tokenmap_keyidx);
			return -ENOMEM;
		}

		/* mutex lock */
		mutex_lock(&vsdof_tokenmap_lock);
		/* debug log */
		if (ITF_LOG_LEVEL >= 2) {
			dev_info(mtk_img_frm_sync_dev->dev,
				"key/type(%d/%d) tokenMap[%d] = (%d, %d), need_token = %d\n",
				token_list.mSyncToken, event_type,
				tokenmap_keyidx,
				vsdof_tokenmap[tokenmap_keyidx].type,
				vsdof_tokenmap[tokenmap_keyidx].hw_token,
				need_token);
		}

		if(vsdof_tokenmap[tokenmap_keyidx].hw_token != TOKEN_INVALID_ID) {
			if (vsdof_tokenmap[tokenmap_keyidx].type == event_type) {
				dev_info(mtk_img_frm_sync_dev->dev,
				"wrong status: continued key(%d) with type(input:%d <-> Map:%d)\n",
				token_list.mSyncToken,
				event_type,
				vsdof_tokenmap[tokenmap_keyidx].type);
				mutex_unlock(&vsdof_tokenmap_lock);
				return -1;
			}

			/*previous set/wait token_key get available hw_token before*/
			token_id = vsdof_tokenmap[tokenmap_keyidx].hw_token;
			vsdof_tokenmap[tokenmap_keyidx].type = imgsys_token_none;
			vsdof_tokenmap[tokenmap_keyidx].hw_token = TOKEN_INVALID_ID;
			vsdof_tokenmap[tokenmap_keyidx].frm_owner = 0;
			vsdof_tokenmap[tokenmap_keyidx].imgstm_inst = 0;
			vsdof_tokenmap[tokenmap_keyidx].frame_no = 0;
			vsdof_tokenmap[tokenmap_keyidx].request_no = 0;
			vsdof_tokenmap[tokenmap_keyidx].request_fd = 0;
			vsdof_tokenmap[tokenmap_keyidx].stage = 0;
			vsdof_tokenmap[tokenmap_keyidx].subfrm_sidx = 0;
			vsdof_tokenmap[tokenmap_keyidx].evt_order = 0;
			vsdof_tokenmap[tokenmap_keyidx].evt_histb_idx = -1;
			vsdof_tokenmap[tokenmap_keyidx].hw_comb = 0;

			if (ITF_LOG_LEVEL >= 1) {
				dev_info(mtk_img_frm_sync_dev->dev,
				"key/type(%d/%d) clear tokenMap[%d] = (%d, %d), token_id = %d\n",
				token_list.mSyncToken, event_type,
				token_list.mSyncToken, vsdof_tokenmap[tokenmap_keyidx].type,
				vsdof_tokenmap[tokenmap_keyidx].hw_token, token_id);
			}

		} else {
			/* acquire available sw token */
			/* need mutex */
			mutex_lock(&dpe_sync_token_pool.pool.buf_pool_lock);
				start_time = jiffies;
			for (j = 0; j < 100; j++) {
				dpe_sync_token_pool.pool.r_idx = dpe_sync_token_pool.pool.idx_pool[0];
				acq_r_idx = dpe_sync_token_pool.pool.r_idx;
				if (dpe_sync_token_pool.token_pool[acq_r_idx].status != buf_st_avai) {
					/*wait event*/
					dpe_token_dump(mtk_img_frm_sync_dev, acq_r_idx);
				} else {
					break;
				}
			}

			dpe_sync_token_pool.pool.idx_pool[0] =
				dpe_sync_token_pool.pool.idx_pool[dpe_sync_token_pool.pool.r_max - 1];
			dpe_sync_token_pool.pool.idx_pool[dpe_sync_token_pool.pool.r_max - 1] = acq_r_idx;
			dpe_sync_token_pool.pool.r_max -= 1;
			if (dpe_sync_token_pool.pool.r_max < dpe_sync_token_pool.pool.lowest_available_num)
				dpe_sync_token_pool.pool.lowest_available_num = dpe_sync_token_pool.pool.r_max;

			dpe_sync_token_pool.token_pool[acq_r_idx].status = buf_st_occu;
			token_id = dpe_sync_token_pool.token_pool[acq_r_idx].token_id;
			acquired = true;
			mutex_unlock(&dpe_sync_token_pool.pool.buf_pool_lock);
			vsdof_tokenmap[tokenmap_keyidx].type = event_type;
			vsdof_tokenmap[tokenmap_keyidx].hw_token = token_id;
			vsdof_tokenmap[tokenmap_keyidx].frm_owner = in_data->frm_owner;
			vsdof_tokenmap[tokenmap_keyidx].imgstm_inst = in_data->imgstm_inst;
			vsdof_tokenmap[tokenmap_keyidx].frame_no = in_data->frm_no;
			vsdof_tokenmap[tokenmap_keyidx].request_no = in_data->req_no;
			vsdof_tokenmap[tokenmap_keyidx].request_fd = gce_index;
			vsdof_tokenmap[tokenmap_keyidx].gce_event_id = imgsys_event[token_id].event;
			if (ITF_LOG_LEVEL >= 1) {
				dev_info(mtk_img_frm_sync_dev->dev,
					"r_idx/pool[0]/r_max(%d/%d/%d) update tkMap[%d] = (%d, %d), token_id = %d\n",
					acq_r_idx,
					dpe_sync_token_pool.pool.idx_pool[0],
					dpe_sync_token_pool.pool.r_max,
					token_list.mSyncToken, vsdof_tokenmap[tokenmap_keyidx].type,
					vsdof_tokenmap[tokenmap_keyidx].hw_token,
					token_id);
			}
		}
		mutex_lock(&oft_tb->pool.buf_pool_lock);
		r_idx = acquire_frm_sync_tbl_index(mtk_img_frm_sync_dev, gce_index);
		oft_info[r_idx].used_sw_token[token_cnt] = token_id;
		oft_info[r_idx].need_release_token[token_cnt] = need_release;
		oft_info[r_idx].used_sw_token_cnt += 1;
		oft_info[r_idx].gce_event_id = imgsys_event[token_id].event;
		token_cnt = oft_info[r_idx].used_sw_token_cnt;
		dev_info(mtk_img_frm_sync_dev->dev, "DPE_isp8-oft_info(%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d)",
			r_idx,
			token_cnt,
			oft_info[r_idx].used_sw_token[token_cnt-1],
			oft_info[r_idx].need_release_token[token_cnt-1],
			oft_info[r_idx].used_sw_token_cnt,
			oft_info[r_idx].gce_event_id,
			token_id,
			dpe_sync_token_pool.pool.idx_pool[0],
			dpe_sync_token_pool.pool.r_max,
			gce_index,
			stoken_num);
		mutex_unlock(&oft_tb->pool.buf_pool_lock);
		out_data->event_id = imgsys_event[token_id].event;
		out_data->req_fd = r_idx;
		mutex_unlock(&vsdof_tokenmap_lock);
	}

	return 0;
}

int Handler_frame_token_sync_MAE_isp8(struct mtk_img_frm_sync *mtk_img_frm_sync_dev,
					struct mae_in_data *in_data, struct mae_out_data *out_data)
{
	return 0;
}

static int release_frame_token_vsdof_isp8(struct mtk_img_frm_sync *mtk_img_frm_sync_dev,
	struct buf_ofst_info_t *oft_info, int r_idx)
{
	int token_pool_idx = 0;
	int idx_check = 0;
	int r_max = 0;
	int i = 0, j = 0, k = 0;

	for (i = 0 ; i < oft_info[r_idx].used_sw_token_cnt ; i++){
		idx_check = 0;
		if (oft_info[r_idx].need_release_token[i]) {
			mutex_lock(&dpe_sync_token_pool.pool.buf_pool_lock);
			token_pool_idx = (oft_info[r_idx].used_sw_token[i]) - IMGSYS_DPE_SYNC_TOKEN_DPE_POOL_1;
			if(token_pool_idx < 0){
				dev_err(mtk_img_frm_sync_dev->dev,
					"wrong token_pool_idx(%d) <- i(%d) with token(%d)\n", token_pool_idx, i,
				oft_info[r_idx].used_sw_token[i]);
				mutex_unlock(&dpe_sync_token_pool.pool.buf_pool_lock);
				return -1;
			}
			dpe_sync_token_pool.token_pool[token_pool_idx].status = buf_st_avai;
			for (j = DPE_SYNC_TOKEN_MAX - 1 ; j >= dpe_sync_token_pool.pool.r_max  ; j--) {
				if (dpe_sync_token_pool.pool.idx_pool[j] == token_pool_idx)
					idx_check = j;
			}
			r_max = dpe_sync_token_pool.pool.r_max;
			for (k = idx_check ; k > r_max  ; k--)
				dpe_sync_token_pool.pool.idx_pool[k] =
					dpe_sync_token_pool.pool.idx_pool[k-1];
			dpe_sync_token_pool.pool.idx_pool[r_max] = token_pool_idx;
			dpe_sync_token_pool.pool.r_max += 1;
			oft_info[r_idx].need_release_token[i] = false;
			oft_info[r_idx].used_sw_token[i] = -1;
			dev_info(mtk_img_frm_sync_dev->dev,
				"release vsdof DPE_isp8 tkp_idx(%d)/tokenid(%d)/status(%d)/r_max(%d)/gce(%d)\n",
				token_pool_idx,
				dpe_sync_token_pool.token_pool[token_pool_idx].token_id,
				dpe_sync_token_pool.token_pool[token_pool_idx].status,
				dpe_sync_token_pool.pool.r_max,
				oft_info[r_idx].gce_event_id);
			mutex_unlock(&dpe_sync_token_pool.pool.buf_pool_lock);
		} else {
			oft_info[r_idx].used_sw_token[i] = -1;
		}
	}
	oft_info[r_idx].used_sw_token_cnt = 0;
	//oft_info[r_idx].buf_st = buf_st_avai;
	oft_info[r_idx].gce_event_id = -1;
	return 0;
}

static int release_frame_token_mcnr_isp8(struct mtk_img_frm_sync *mtk_img_frm_sync_dev,
	struct buf_ofst_info_t *oft_info, int r_idx)
{
	int token_pool_idx = 0;
	int idx_check = 0;
	int r_max = 0;
	int i = 0, j = 0, k = 0;

	//mutex_lock(&oft_tb->pool.buf_pool_lock);
	for (i = 0 ; i < oft_info[r_idx].used_sw_token_cnt ; i++){
		idx_check = 0;
		if (oft_info[r_idx].need_release_token[i]) {
			token_pool_idx = (oft_info[r_idx].used_sw_token[i]) - IMGSYS_SYNC_TOKEN_POOL_1;
			if(token_pool_idx < 0){
				dev_err(mtk_img_frm_sync_dev->dev,
					"wrong token_pool_idx(%d) <- i(%d) with token(%d)\n", token_pool_idx, i,
				oft_info[r_idx].used_sw_token[i]);
				return -1;
			}
			imgsys_sync_token_pool.token_pool[token_pool_idx].status = buf_st_avai;
			dev_info(mtk_img_frm_sync_dev->dev,
				"release token_pool_idx(%d), tokenid(%d)/status(%d)\n", token_pool_idx,
				imgsys_sync_token_pool.token_pool[token_pool_idx].token_id,
				imgsys_sync_token_pool.token_pool[token_pool_idx].status);
			for (j = DPE_SYNC_TOKEN_MAX - 1 ; j >= imgsys_sync_token_pool.pool.r_max  ; j--) {
				if (imgsys_sync_token_pool.pool.idx_pool[j] == token_pool_idx)
					idx_check = j;
			}
			r_max = imgsys_sync_token_pool.pool.r_max;
			for (k = idx_check ; k > r_max  ; k--)
				imgsys_sync_token_pool.pool.idx_pool[k] =
					imgsys_sync_token_pool.pool.idx_pool[k-1];
			imgsys_sync_token_pool.pool.idx_pool[r_max] = token_pool_idx;
			imgsys_sync_token_pool.pool.r_max += 1;
			oft_info[r_idx].need_release_token[i] = false;
			oft_info[r_idx].used_sw_token[i] = -1;
		} else {
			oft_info[r_idx].used_sw_token[i] = -1;
		}
	}
	oft_info[r_idx].used_sw_token_cnt = 0;
	oft_info[r_idx].gce_event_id = -1;
	return 0;
}

int release_frame_token_imgsys_isp8(struct mtk_img_frm_sync *mtk_img_frm_sync_dev,
					struct imgsys_deque_done_in_data *in_data)
{
	/* need to modify oft_info*/
	struct buf_ofst_table_t *oft_tb = NULL;
	struct buf_ofst_info_t *oft_info = NULL;
	int r_idx = in_data->sw_ridx;
	int ret = 0;

	if (r_idx >= MAX_GCE_RING_IMGSYS)
		return ret;

	oft_tb = &imgsys_gcebuf_ofst_tb_n;
	oft_info = oft_tb->info;

	if (!oft_info[r_idx].used_sw_token_cnt)
		return ret;

	if ((oft_info[r_idx].gce_event_id) >= 874 && (oft_info[r_idx].gce_event_id <= 889))
		ret = release_frame_token_vsdof_isp8(mtk_img_frm_sync_dev, oft_info, r_idx);
	else
		ret = release_frame_token_mcnr_isp8(mtk_img_frm_sync_dev, oft_info, r_idx);


	return ret;
}

int release_frame_token_DPE_isp8(struct mtk_img_frm_sync *mtk_img_frm_sync_dev,
					struct dpe_deque_done_in_data *in_data)
{
	/* need to modify oft_info*/
	int token_pool_idx = 0;
	int idx_check = 0;
	int r_max = 0;
	int i = 0, j = 0, k = 0;
	struct buf_ofst_table_t *oft_tb = NULL;
	struct buf_ofst_info_t *oft_info = NULL;
	int r_idx = in_data->req_fd;

	oft_tb = &dpe_gcebuf_ofst_tb_n;
	oft_info = oft_tb->info;

	if (!oft_info[r_idx].used_sw_token_cnt)
		return 0;

	for (i = 0 ; i < oft_info[r_idx].used_sw_token_cnt ; i++) {
		idx_check = 0;
		if (oft_info[r_idx].need_release_token[i]) {
			mutex_lock(&dpe_sync_token_pool.pool.buf_pool_lock);
			token_pool_idx = (oft_info[r_idx].used_sw_token[i]) - IMGSYS_DPE_SYNC_TOKEN_DPE_POOL_1;
			if(token_pool_idx < 0) {
				dev_err(mtk_img_frm_sync_dev->dev,
					"wrong token_pool_idx(%d) <- i(%d) with token(%d)\n", token_pool_idx, i,
				oft_info[r_idx].used_sw_token[i]);
				mutex_unlock(&dpe_sync_token_pool.pool.buf_pool_lock);
				return -1;
			}
			dpe_sync_token_pool.token_pool[token_pool_idx].status = buf_st_avai;
			for (j = DPE_SYNC_TOKEN_MAX - 1 ; j >= dpe_sync_token_pool.pool.r_max ; j--) {
				if (dpe_sync_token_pool.pool.idx_pool[j] == token_pool_idx)
					idx_check = j;
			}
			r_max = dpe_sync_token_pool.pool.r_max;
			for (k = idx_check ; k > r_max  ; k--)
				dpe_sync_token_pool.pool.idx_pool[k] = dpe_sync_token_pool.pool.idx_pool[k-1];

			dpe_sync_token_pool.pool.idx_pool[r_max] = token_pool_idx;
			dpe_sync_token_pool.pool.r_max += 1;
			oft_info[r_idx].need_release_token[i] = false;
			oft_info[r_idx].used_sw_token[i] = -1;
			dev_info(mtk_img_frm_sync_dev->dev,
				"rls DPE_isp8 tkp_idx(%d)/tkid(%d)/status(%d)/gce_index(%d)/r_max(%d)/gce(%d)\n",
				token_pool_idx,
				dpe_sync_token_pool.token_pool[token_pool_idx].token_id,
				dpe_sync_token_pool.token_pool[token_pool_idx].status,
				r_idx,
				dpe_sync_token_pool.pool.r_max,
				oft_info[r_idx].gce_event_id);
			mutex_unlock(&dpe_sync_token_pool.pool.buf_pool_lock);
		} else {
			oft_info[r_idx].used_sw_token[i] = -1;
		}
	}
	oft_info[r_idx].used_sw_token_cnt = 0;
	//oft_info[r_idx].buf_st = buf_st_avai;
	oft_info[r_idx].gce_event_id = -1;
	return 0;
}

int release_frame_token_MAE_isp8(struct mtk_img_frm_sync *mtk_img_frm_sync_dev,
					struct mae_deque_done_in_data *in_data)
{
	return 0;
}

int clear_token_user_isp8(struct mtk_img_frm_sync *mtk_img_frm_sync_dev,
					unsigned long frm_owner, unsigned long imgstm_inst)
{
	return 0;
}

int frm_sync_timeout_isp8(struct mtk_img_frm_sync *mtk_img_frm_sync_dev,
					int gce_event_id)
{
	int i = 0;

	if ((gce_event_id >= DPE_GCE_EVENT_BEGIN) && (gce_event_id <= DPE_GCE_EVENT_END)) {
		for (i = 0;i < DPE_SYNC_TOKEN_MAX;i++) {
			dev_info(mtk_img_frm_sync_dev->dev,
			"tokmap-type-id-frm_owner-img_inst-frm_no-req_no-req_fd(%d-%d-%llx-%llx-%d-%d-%d-%d)",
			vsdof_tokenmap[i].type,
			vsdof_tokenmap[i].hw_token,
			vsdof_tokenmap[i].frm_owner,
			vsdof_tokenmap[i].imgstm_inst,
			vsdof_tokenmap[i].frame_no,
			vsdof_tokenmap[i].request_no,
			vsdof_tokenmap[i].request_fd,
			vsdof_tokenmap[i].gce_event_id);
		}
		aee_kernel_warning("CRDISPATCH_KEY:DPE","time event id(%d)", gce_event_id);
	} else
		dev_info(mtk_img_frm_sync_dev->dev, "not support event in isp8");
	return 0;
}
struct mtk_img_frm_sync_data mtk_img_frm_sync_data_isp8 = {
	.open = dpe_open_isp8,
	.init = init_isp8,
	.uninit = uninit_isp8,
	.Handler_frame_token_sync_imgsys = Handler_frame_token_sync_imgsys_isp8,
	.Handler_frame_token_sync_DPE = Handler_frame_token_sync_DPE_isp8,
	.Handler_frame_token_sync_MAE = Handler_frame_token_sync_MAE_isp8,
	.release_frame_token_imgsys = release_frame_token_imgsys_isp8,
	.release_frame_token_DPE = release_frame_token_DPE_isp8,
	.release_frame_token_MAE = release_frame_token_MAE_isp8,
	.clear_token_user = clear_token_user_isp8,
	.frm_sync_timeout = frm_sync_timeout_isp8,
};
