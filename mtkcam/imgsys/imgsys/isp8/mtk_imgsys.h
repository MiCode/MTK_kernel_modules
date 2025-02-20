/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2018 MediaTek Inc.
 */

#ifndef MTK_DIP_H
#define MTK_DIP_H

#include <linux/videodev2.h>
#include <linux/v4l2-controls.h>
#include "mtk-img-ipi.h"

#define FD_MAX (32)
#define STANDARD_MODE_MAX_FRAMES (1)
#define BATCH_MODE_MAX_FRAMES (32)
#define MCRP_D1 (0)
#define MCRP_D2 (1)
#define LSC_HEADER_ADDR_OFFSET (1024 * 48)
#define LSC_DATA_ADDR_OFFSET (1024 * 50)

struct fd_info {
	uint8_t fd_num;
	unsigned int fds[FD_MAX];
	size_t fds_size[FD_MAX];
} __packed;

struct fd_tbl {
	uint8_t fd_num;
	unsigned int *fds;
} __packed;

struct sensor_info {
	uint16_t full_wd;
	uint16_t full_ht;
};

struct init_info {
	struct sensor_info sensor;
	uint32_t sec_tag;
};

struct mem_info {
	uint8_t  is_smvr;
    uint8_t  is_capture;
};

struct mtk_imgsys_fd_info {
	int     fd;
	uint8_t enable_acp_coherance;  /* 0:disabled 1:enabled */
	uint8_t reserved[3];
};

struct mtk_imgsys_fd_info_tbl {
	struct mtk_imgsys_fd_info fds[32];
	int fd_num;
};

struct ctrl_info {
	uint32_t id;
	int32_t value;
} __packed;

enum imgsys_user_enum {
	DIP_DEFAULT,
	DIP_AIHDRCORE_FE_FM,
	DIP_AINRCORE_FE_FM,
	DIP_BOKEHNODE,
	DIP_CAPTURE,
	DIP_DEPTH_MAP,
	DIP_MFLLCORE_MFB,
	DIP_STREAMING,
};

/*
 * New added for v4l2 dip driver - batch mode/multi-frames.
 * Use case:
 *    ioctl(fd, MTKDIP_IOC_QBUF, struct frame_param_pack);
 */
struct frame_param_pack {
	uint8_t num_frames;
	struct img_ipi_frameparam *frame_params;

	uint64_t seq_num;   // used by user
	void *cookie;       // used by user
} __packed;

/*
 * Used in driver internaly. (kernel driver <-> user space daemon)
 * User should not use this structure.
 */
struct framepack_buf_data {
	struct frame_param_pack header;
	uint8_t unprocessed_count;
	struct img_ipi_frameparam params[BATCH_MODE_MAX_FRAMES];
} __packed;

struct mtk_imgsys_ioctl_clear_user_resource_info {
	uint64_t frm_owner;
	uint64_t imgstm_inst;
};

/* ioctl interface for user space*/
#define MTKDIP_IOC_QBUF \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 1, struct frame_param_pack)
#define MTKDIP_IOC_DQBUF \
	_IOR('V', BASE_VIDIOC_PRIVATE + 2, struct frame_param_pack)
#define MTKDIP_IOC_STREAMON _IO('V', BASE_VIDIOC_PRIVATE + 3)
#define MTKDIP_IOC_STREAMOFF _IO('V', BASE_VIDIOC_PRIVATE + 4)
#define MTKDIP_IOC_G_REG_SIZE \
			_IOWR('V', BASE_VIDIOC_PRIVATE + 5, unsigned int)
#define MTKDIP_IOC_G_ISP_VERSION \
			_IOWR('V', BASE_VIDIOC_PRIVATE + 6, unsigned int)
#define MTKDIP_IOC_S_USER_ENUM _IOW('V', BASE_VIDIOC_PRIVATE + 7, int)
#define MTKDIP_IOC_ADD_KVA _IOW('V', BASE_VIDIOC_PRIVATE + 8, struct fd_info)
#define MTKDIP_IOC_DEL_KVA _IOW('V', BASE_VIDIOC_PRIVATE + 9, struct fd_info)
#define MTKDIP_IOC_ADD_IOVA _IOW('V', BASE_VIDIOC_PRIVATE + 10, struct fd_tbl)
#define MTKDIP_IOC_DEL_IOVA _IOW('V', BASE_VIDIOC_PRIVATE + 11, struct fd_tbl)
#define MTKDIP_IOC_S_INIT_INFO \
			_IOW('V', BASE_VIDIOC_PRIVATE + 12, struct init_info)
#define MTKDIP_IOC_SET_CONTROL _IOW('V', BASE_VIDIOC_PRIVATE + 13, struct ctrl_info)
#define MTKDIP_IOC_GET_CONTROL _IOWR('V', BASE_VIDIOC_PRIVATE + 14, struct ctrl_info)
#define MTKDIP_IOC_ADD_FENCE _IOW('V', BASE_VIDIOC_PRIVATE + 15, struct fd_tbl)
#define MTKDIP_IOC_DEL_FENCE _IOW('V', BASE_VIDIOC_PRIVATE + 16, struct fd_tbl)
#define MTKDIP_IOC_ALOC_BUF _IOW('V', BASE_VIDIOC_PRIVATE + 17, struct mem_info)
#define MTKDIP_IOC_FREE_BUF _IOW('V', BASE_VIDIOC_PRIVATE + 18, struct mem_info)
#define MTKDIP_IOC_CLEAR_USER_RESOURCE \
	_IOW('V', BASE_VIDIOC_PRIVATE + 19, struct mtk_imgsys_ioctl_clear_user_resource_info)
#define MTKDIP_IOC_ACQUIRE_IOVA _IOW('V', BASE_VIDIOC_PRIVATE + 20, struct mtk_imgsys_fd_info_tbl)
#define MTKDIP_IOC_RELEASE_IOVA _IOW('V', BASE_VIDIOC_PRIVATE + 21, struct mtk_imgsys_fd_info_tbl)
#define V4L2_CID_IMGSYS_OFFSET	(0xC000)
#define V4L2_CID_IMGSYS_BASE    (V4L2_CID_USER_BASE + V4L2_CID_IMGSYS_OFFSET)
#define V4L2_CID_IMGSYS_APU_DC  (V4L2_CID_IMGSYS_BASE + 1)
#endif

