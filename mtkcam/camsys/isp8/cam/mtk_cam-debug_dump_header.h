/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#ifndef __MTK_CAM_DEBUG_DUMP_HEADER__
#define __MTK_CAM_DEBUG_DUMP_HEADER__

struct mtk_cam_dump_header {
	/* Common Debug Information*/
	__u8	desc[64];
	__u32	request_fd;
	__u32   stream_id;
	__u64	timestamp;
	__u32	sequence;
	__u32	header_size;
	__u32	payload_offset;
	__u32	payload_size;

	/* meta file information */
	__u32	meta_version_major;
	__u32	meta_version_minor;

	/* CQ dump */
	__u32	cq_dump_buf_offset;
	__u32	cq_size;
	__u64	cq_iova;
	__u32	cq_desc_offset;
	__u32	cq_desc_size;
	__u32	sub_cq_desc_offset;
	__u32	sub_cq_desc_size;

	/* meta in */
	__u32	meta_in_dump_buf_offset;
	__u32	meta_in_dump_buf_size;
	__u64	meta_in_iova;

	/* meta out 0 */
	__u32	meta_out_0_dump_buf_offset;
	__u32	meta_out_0_dump_buf_size;
	__u64	meta_out_0_iova;

	/* meta out 1 */
	__u32	meta_out_1_dump_buf_offset;
	__u32	meta_out_1_dump_buf_size;
	__u64	meta_out_1_iova;

	/* meta out 2 */
	__u32	meta_out_2_dump_buf_offset;
	__u32	meta_out_2_dump_buf_size;
	__u64	meta_out_2_iova;

	/* status dump */
	__u32	status_dump_offset;
	__u32	status_dump_size;

	/* ipi frame param */
	__u32	frame_dump_offset;
	__u32	frame_dump_size;

	/* ipi config param */
	__u32	config_dump_offset;
	__u32	config_dump_size;
	__u32	used_stream_num;
};

#endif /* __MTK_CAM_DEBUG_DUMP_HEADER__ */
