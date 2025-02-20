/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __MTK_CAM_UT_ENGINES_H
#define __MTK_CAM_UT_ENGINES_H

#include <linux/kfifo.h>
#include "mtk_cam_ut-event.h"

struct engine_ops {
	/* test mdl, optional */
	int (*set_size)(struct device *dev,
			int width, int height,
			int pixmode_lg2,
			int pattern,
			int tg_idx,
			int tag);

	int (*initialize)(struct device *dev, void *ext_params);
	int (*reset)(struct device *dev);
	int (*s_stream)(struct device *dev, enum streaming_enum on);
	int (*apply_cq)(struct device *dev,
		dma_addr_t cq_addr, unsigned int cq_size, unsigned int cq_offset,
		unsigned int sub_cq_size,
		unsigned int sub_cq_offset);
	int (*dev_config)(struct device *dev,
		struct mtkcam_ipi_input_param *cfg_in_param);
};

#define CALL_ENGINE_OPS(dev, ops, ...) \
	((dev && dev->ops) ? dev.ops(dev, ##__VA_ARGS__) : -EINVAL)

enum RAW_STREAMON_TYPE {
	STREAM_FROM_TG,
	STREAM_FROM_RAWI_R2,
	STREAM_FROM_RAWI_R3,
	STREAM_FROM_RAWI_R5,
	STREAM_FROM_ADLRD,
};

struct mtk_ut_raw_initial_params {
	int subsample;
	int streamon_type;
	int hardware_scenario;
};

struct mtk_ut_raw_device {
	struct device *dev;
	struct mtk_cam_ut *ut;
	unsigned int id;
	void __iomem *base;
	void __iomem *base_inner;
	void __iomem *yuv_base;
	void __iomem *rms_base;
	unsigned int num_clks;
	struct clk **clks;

	struct ut_event_source event_src;
	struct engine_ops ops;

	unsigned int	fifo_size;
	struct kfifo	msgfifo;

	int is_subsample;
	int is_initial_cq;
	int cq_done_mask; /* [0]: main, [1]: sub */
	int hardware_scenario;

};

enum mtk_ut_raw_module_id {
	RAW_A = 0,
	RAW_B = 1,
	RAW_C = 2,
	RAW_NUM = 3,
};

struct mtk_ut_mraw_initial_params {
	int subsample;
};

void raw_disable_tg_vseol_sub_ctl(struct device *dev);

#define CALL_RAW_OPS(dev, op, ...) \
{\
	struct mtk_ut_raw_device *drvdata = dev_get_drvdata(dev);\
	((dev && drvdata->ops.op) ? drvdata->ops.op(dev, ##__VA_ARGS__) : \
	 -EINVAL);\
}

struct ut_yuv_status {
	/* yuv INT 1/2/4/5 */
	u32 irq;
	u32 wdma;
	u32 drop;
	u32 ofl;
};

struct ut_yuv_debug_csr {
	/* DMAO */
	u32 yuvo_r1_addr;
	u32 yuvo_r3_addr;

    /* FBC */
	u32 fbc_yuvo_r1ctl2;
	u32 fbc_yuvo_r3ctl2;
};

struct mtk_ut_yuv_device {
	struct device *dev;
	unsigned int id;
	void __iomem *base;
	unsigned int num_clks;
	struct clk **clks;

	struct engine_ops ops;
};

#define CALL_YUV_OPS(dev, op, ...) \
{\
	struct mtk_ut_yuv_device *drvdata = dev_get_drvdata(dev);\
	((dev && drvdata->ops.op) ? drvdata->ops.op(dev, ##__VA_ARGS__) : \
	 -EINVAL);\
}

struct mtk_ut_rms_device {
	struct device *dev;
	unsigned int id;
	void __iomem *base;
	unsigned int num_clks;
	struct clk **clks;

	struct engine_ops ops;
};

#define CALL_RMS_OPS(dev, op, ...) \
{\
	struct mtk_ut_rms_device *drvdata = dev_get_drvdata(dev);\
	((dev && drvdata->ops.op) ? drvdata->ops.op(dev, ##__VA_ARGS__) : \
	 -EINVAL);\
}

struct mtk_ut_mraw_device {
	struct device *dev;
	unsigned int id;
	void __iomem *base;
	void __iomem *base_inner;
	void __iomem *mraw_base;
	unsigned int num_clks;
	unsigned int cammux_id;
	struct clk **clks;

	struct ut_event_source event_src;
	struct engine_ops ops;
};

#define CALL_MRAW_OPS(dev, op, ...) \
{\
	struct mtk_ut_mraw_device *mraw = dev_get_drvdata(dev);\
	((dev && mraw->ops.op) ? mraw->ops.op(dev, ##__VA_ARGS__) : -EINVAL);\
}

struct mtk_ut_camsv_device {
	struct device *dev;
	unsigned int id;
	void __iomem *base;
	void __iomem *base_dma;
	void __iomem *base_scq;
	void __iomem *base_inner;
	void __iomem *base_inner_dma;
	void __iomem *base_inner_scq;
	unsigned int num_clks;
	unsigned int cammux_id;
	struct clk **clks;
	unsigned int is_dc_mode;
	struct ut_event_source event_src;
	struct engine_ops ops;
};

#define CALL_CAMSV_OPS(dev, op, ...) \
{\
	struct mtk_ut_camsv_device *camsv = dev_get_drvdata(dev);\
	((dev && camsv->ops.op) ? camsv->ops.op(dev, ##__VA_ARGS__) : -EINVAL);\
}

enum SENINF_ENUM {
	SENINF_1,
	SENINF_2,
	SENINF_3,
	SENINF_4,
	SENINF_5,
	SENINF_6,
	SENINF_7,
	SENINF_8,
	SENINF_9,
	SENINF_10,
	SENINF_11,
	SENINF_12,
	SENINF_NUM,
};

enum SENINF_MUX_ENUM {
	SENINF_MUX1,
	SENINF_MUX2,
	SENINF_MUX3,
	SENINF_MUX4,
	SENINF_MUX5,
	SENINF_MUX6,
	SENINF_MUX7,
	SENINF_MUX8,
	SENINF_MUX9,
	SENINF_MUX10,
	SENINF_MUX11,
	SENINF_MUX12,
	SENINF_MUX13,
	SENINF_MUX14,
	SENINF_MUX15,
	SENINF_MUX16,
	SENINF_MUX17,
	SENINF_MUX18,
	SENINF_MUX19,
	SENINF_MUX20,
	SENINF_MUX21,
	SENINF_MUX22,
	SENINF_MUX_NUM,

	SENINF_MUX_ERROR = -1,
};

enum mux_status {
	IDLE,
	USING,
};
struct mtk_ut_seninf_device {
	struct device *dev;
	void __iomem *base;

	unsigned int num_clks;
	struct clk **clks;

	struct engine_ops ops;

	unsigned int seninf_mux_status[SENINF_MUX_NUM];
	unsigned int seninf_status[SENINF_NUM];

	int mux_camsv_sat_range[2];
	int mux_camsv_range[2];
	int mux_raw_range[2];
	int mux_pdp_range[2];

	int muxvr_camsv_sat_range[2];
	int muxvr_camsv_range[2];
	int muxvr_raw_range[2];
	int muxvr_pdp_range[2];

	int cammux_camsv_sat_range[2];
	int cammux_camsv_range[2];
	int cammux_raw_range[2];
	int cammux_pdp_range[2];
};

static inline int seninf_mux_raw(struct device *dev, int raw_idx)
{
	struct mtk_ut_seninf_device *seninf = dev_get_drvdata(dev);

	return  seninf->mux_raw_range[0] + raw_idx;
}

static inline int seninf_cammux_raw(struct device *dev, int raw_idx)
{
	struct mtk_ut_seninf_device *seninf = dev_get_drvdata(dev);

	return  seninf->cammux_raw_range[0] + raw_idx;
}

#define CALL_SENINF_OPS(dev, op, ...) \
({\
	struct mtk_ut_seninf_device *_seninf = dev_get_drvdata(dev);\
	((dev && _seninf->ops.op) ? _seninf->ops.op(dev, ##__VA_ARGS__) : -EINVAL);\
})

extern struct platform_driver mtk_ut_raw_driver;
extern struct platform_driver mtk_ut_yuv_driver;
extern struct platform_driver mtk_ut_rms_driver;
extern struct platform_driver mtk_ut_camsv_driver;
extern struct platform_driver mtk_ut_mraw_driver;
extern struct platform_driver mtk_ut_seninf_driver;
#define WITH_LARB_DRIVER 1
#define WITH_CAMSV_DRIVER 1
#define WITH_MRAW_DRIVER 1
#define SUPPORT_PM 1
#define SUPPORT_RAWB 0
#define WITH_POWER_DRIVER 1
extern struct platform_driver mtk_ut_larb_driver;
extern const struct mtk_cam_ut_data *cur_platform;

#endif /* __MTK_CAM_UT_ENGINES_H */
