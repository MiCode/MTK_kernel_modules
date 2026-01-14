/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM mtk_camsys

#if !defined(_MTK_CAM_TRACE_H) || defined(TRACE_HEADER_MULTI_READ)
#define _MTK_CAM_TRACE_H

#include <linux/tracepoint.h>
#include <linux/trace_events.h>

TRACE_EVENT(tracing_mark_write,
	TP_PROTO(const char *fmt, va_list *va),
	TP_ARGS(fmt, va),
	TP_STRUCT__entry(
		__vstring(vstr, fmt, va)
	),
	TP_fast_assign(
		__assign_vstr(vstr, fmt, va);
	),
	TP_printk("%s", __get_str(vstr))
);

TRACE_EVENT_CONDITION(raw_irq,
	TP_PROTO(struct device *dev,
		 unsigned int cookie,
		 unsigned int irq,
		 unsigned int dmao_done,
		 unsigned int dmai_done,
		 unsigned int cq_done,
		 unsigned int dcif_status
		),
	TP_ARGS(dev,
		cookie,
		irq,
		dmao_done,
		dmai_done,
		cq_done,
		dcif_status
	       ),
	TP_CONDITION(irq || dmao_done || dmai_done || cq_done || dcif_status),
	TP_STRUCT__entry(
		__string(device, dev_name(dev))
		__field(unsigned int, cookie)
		__field(unsigned int, irq)
		__field(unsigned int, dmao_done)
		__field(unsigned int, dmai_done)
		__field(unsigned int, cq_done)
		__field(unsigned int, dcif_status)
	),
	TP_fast_assign(
		__assign_str(device, dev_name(dev));
		__entry->cookie = cookie;
		__entry->irq = irq;
		__entry->dmao_done = dmao_done;
		__entry->dmai_done = dmai_done;
		__entry->cq_done = cq_done;
		__entry->dcif_status = dcif_status;
	),
	TP_printk("%s c=0x%x irq=0x%08x dmao=0x%08x dmai=0x%08x cq=0x%08x dcif=0x%08x %s",
		  __get_str(device),
		  __entry->cookie,
		  __entry->irq,
		  __entry->dmao_done,
		  __entry->dmai_done,
		  __entry->cq_done,
		  __entry->dcif_status,
		  __print_flags(__entry->irq & 0x21fe0c0, "|",
				{ BIT(6),	"TG_OVERRUN" },
				{ BIT(7),	"TG_GRABERR" },
				{ BIT(13),	"CQ_DB_LOAD_ERR" },
				{ BIT(14),	"MAX_START_SMALL" },
				{ BIT(15),	"MAX_START_DLY_ERR" },
				{ BIT(16),	"CQ_MAIN_CODE_ERR" },
				{ BIT(17),	"CQ_MAIN_VS_ERR" },
				{ BIT(18),	"CQ_MAIN_TRIG_DLY" },
				{ BIT(19),	"CQ_SUB_CODE_ERR" },
				{ BIT(20),	"CQ_SUB_VS_ERR" },
				{ BIT(25),	"DMA_ERR" })
	)
);

TRACE_EVENT_CONDITION(yuv_irq,
	TP_PROTO(struct device *dev,
		 unsigned int irq,
		 unsigned int dmao_done,
		 unsigned int dmai_done),
	TP_ARGS(dev,
		irq,
		dmao_done,
		dmai_done),
	TP_CONDITION(irq || dmao_done || dmai_done),
	TP_STRUCT__entry(
		__string(device, dev_name(dev))
		__field(unsigned int, irq)
		__field(unsigned int, dmao_done)
		__field(unsigned int, dmai_done)
	),
	TP_fast_assign(
		__assign_str(device, dev_name(dev));
		__entry->irq = irq;
		__entry->dmao_done = dmao_done;
		__entry->dmai_done = dmai_done;
	),
	TP_printk("%s irq=0x%08x dmao=0x%08x dmai=0x%08x %s",
		  __get_str(device),
		  __entry->irq,
		  __entry->dmao_done,
		  __entry->dmai_done,
		  __print_flags(__entry->irq & 0x4, "|",
				{ BIT(2),	"DMA_ERR" })
	)
);

TRACE_EVENT_CONDITION(raw_dma_status,
	TP_PROTO(struct device *dev,
		 unsigned int drop,
		 unsigned int overflow,
		 unsigned int underflow
		),
	TP_ARGS(dev,
		drop,
		overflow,
		underflow),
	TP_CONDITION(drop || overflow || underflow),
	TP_STRUCT__entry(
		__string(device, dev_name(dev))
		__field(unsigned int, drop)
		__field(unsigned int, overflow)
		__field(unsigned int, underflow)
	),
	TP_fast_assign(
		__assign_str(device, dev_name(dev));
		__entry->drop = drop;
		__entry->overflow = overflow;
		__entry->underflow = underflow;
	),
	TP_printk("%s drop=0x%08x overflow=0x%08x underflow=0x%08x",
		  __get_str(device),
		  __entry->drop,
		  __entry->overflow,
		  __entry->underflow
	)
);

TRACE_EVENT_CONDITION(raw_otf_overflow,
	TP_PROTO(struct device *dev, unsigned int otf_overflow),
	TP_ARGS(dev, otf_overflow),
	TP_CONDITION(otf_overflow),
	TP_STRUCT__entry(
		__string(device, dev_name(dev))
		__field(unsigned int, otf_overflow)
	),
	TP_fast_assign(
		__assign_str(device, dev_name(dev));
		__entry->otf_overflow = otf_overflow;
	),
	TP_printk("%s otf_overflow=0x%08x",
		  __get_str(device),
		  __entry->otf_overflow
	)
);

DECLARE_EVENT_CLASS(camsv_irq,
	TP_PROTO(struct device *dev,
		 unsigned int seq_no_inner,
		 unsigned int seq_no,
		 unsigned int status),
	TP_ARGS(dev, seq_no_inner, seq_no, status),
	TP_STRUCT__entry(
		__string(device, dev_name(dev))
		__field(unsigned int, seq_no_inner)
		__field(unsigned int, seq_no)
		__field(unsigned int, status)
	),
	TP_fast_assign(
		__assign_str(device, dev_name(dev));
		__entry->seq_no_inner = seq_no_inner;
		__entry->seq_no = seq_no;
		__entry->status = status;
	),
	TP_printk("%s status=0x%08x seq_no=0x%x_%x",
		  __get_str(device),
		  __entry->status,
		  __entry->seq_no_inner,
		  __entry->seq_no
	)
);

#define DEFINE_CAMSV_IRQ_EVENT(name)			\
DEFINE_EVENT(camsv_irq, camsv_irq_##name,		\
	TP_PROTO(struct device *dev,			\
		 unsigned int seq_no_inner,		\
		 unsigned int seq_no,			\
		 unsigned int status),			\
	TP_ARGS(dev, seq_no_inner, seq_no, status)	\
)

DEFINE_CAMSV_IRQ_EVENT(cq_done);
DEFINE_CAMSV_IRQ_EVENT(done);
DEFINE_CAMSV_IRQ_EVENT(err);

TRACE_EVENT(camsv_irq_sof,
	TP_PROTO(struct device *dev,
		 unsigned int seq_no_inner,
		 unsigned int seq_no,
		 unsigned int sof_status,
		 unsigned int channel_status,
		 const unsigned int *active_group,
		 unsigned int first_tag,
		 unsigned int last_tag,
		 unsigned int tg_cnt),
	TP_ARGS(dev,
		seq_no_inner,
		seq_no,
		sof_status,
		channel_status,
		active_group,
		first_tag,
		last_tag,
		tg_cnt
	       ),
	TP_STRUCT__entry(
		__string(device, dev_name(dev))
		__field(unsigned int, seq_no_inner)
		__field(unsigned int, seq_no)
		__field(unsigned int, sof_status)
		__field(unsigned int, channel_status)
		__array(unsigned int, active_group, 4)
		__field(unsigned int, first_tag)
		__field(unsigned int, last_tag)
		__field(unsigned int, tg_cnt)
		),
	TP_fast_assign(
		__assign_str(device, dev_name(dev));
		__entry->seq_no_inner = seq_no_inner;
		__entry->seq_no = seq_no;
		__entry->sof_status = sof_status;
		__entry->channel_status = channel_status;
		memcpy(__entry->active_group,
		       active_group, 4 * sizeof(unsigned int));
		__entry->first_tag = first_tag;
		__entry->last_tag = last_tag;
		__entry->tg_cnt = tg_cnt;
		),
	TP_printk("%s sof status=0x%x channel status=0x%x seq_no=0x%x_%x group_tags=%s first/last_tag=0x%x 0x%x vf_st_tag4=%d",
		  __get_str(device),
		  __entry->sof_status,
		  __entry->channel_status,
		  __entry->seq_no_inner,
		  __entry->seq_no,
		  __print_array(__entry->active_group,
				ARRAY_SIZE(__entry->active_group),
				sizeof(__entry->active_group[0])),
		__entry->first_tag,
		__entry->last_tag,
		__entry->tg_cnt
	)
);

TRACE_EVENT(mraw_irq,
	TP_PROTO(struct device *dev,
		 unsigned int irq_status,
		 unsigned int irq_status2,
		 unsigned int err_status,
		 unsigned int irq_status6,
		 unsigned int dma_err_status,
		 unsigned int seq_inner,
		 unsigned int seq
		),
	TP_ARGS(dev,
		irq_status,
		irq_status2,
		err_status,
		irq_status6,
		dma_err_status,
		seq_inner,
		seq
	       ),
	TP_STRUCT__entry(
		__string(device, dev_name(dev))
		__field(unsigned int, irq_status)
		__field(unsigned int, irq_status2)
		__field(unsigned int, err_status)
		__field(unsigned int, irq_status6)
		__field(unsigned int, dma_err_status)
		__field(unsigned int, seq_inner)
		__field(unsigned int, seq)
	),
	TP_fast_assign(
		__assign_str(device, dev_name(dev));
		__entry->irq_status = irq_status;
		__entry->irq_status2 = irq_status2;
		__entry->err_status = err_status;
		__entry->irq_status6 = irq_status6;
		__entry->dma_err_status = dma_err_status;
		__entry->seq_inner = seq_inner;
		__entry->seq = seq;
	),
	TP_printk("%s status=0x%x_%x(err=0x%x)/0x%x dma_err=0x%x seq_no=0x%x_%x",
		  __get_str(device),
		  __entry->irq_status,
		  __entry->irq_status2,
		  __entry->err_status,
		  __entry->irq_status6,
		  __entry->dma_err_status,
		  __entry->seq_inner,
		  __entry->seq
	)
);

#endif /*_MTK_CAM_TRACE_H */

#undef TRACE_INCLUDE_PATH
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_PATH .
#define TRACE_INCLUDE_FILE mtk_cam-trace
/* This part must be outside protection */
#include <trace/define_trace.h>


#ifndef __MTK_CAM_TRACE_H
#define __MTK_CAM_TRACE_H

#if IS_ENABLED(CONFIG_TRACING) && defined(MTK_CAM_TRACE_SUPPORT)

#include <linux/sched.h>
#include <linux/kernel.h>

int mtk_cam_trace_enabled_tags(void);

#define _MTK_CAM_TRACE_ENABLED(category)	\
	(mtk_cam_trace_enabled_tags() & (1UL << category))

__printf(1, 2)
void mtk_cam_trace(const char *fmt, ...);

#define _MTK_CAM_TRACE(category, fmt, args...)			\
do {								\
	if (unlikely(_MTK_CAM_TRACE_ENABLED(category)))		\
		mtk_cam_trace(fmt, ##args);			\
} while (0)

#else

#define _MTK_CAM_TRACE_ENABLED(category)	0
#define _MTK_CAM_TRACE(category, fmt, args...)

#endif

enum trace_category {
	TRACE_BASIC,
	TRACE_HW_IRQ,
	TRACE_BUFFER,
	TRACE_FBC,
};

#define _TRACE_CAT(cat)		TRACE_ ## cat

#define MTK_CAM_TRACE_ENABLED(category)	\
	_MTK_CAM_TRACE_ENABLED(_TRACE_CAT(category))

#define MTK_CAM_TRACE(category, fmt, args...)				\
	_MTK_CAM_TRACE(_TRACE_CAT(category), "camsys:" fmt, ##args)

/*
 * systrace format
 */

#define MTK_CAM_TRACE_BEGIN(category, fmt, args...)			\
	_MTK_CAM_TRACE(_TRACE_CAT(category), "B|%d|camsys:" fmt,	\
		      task_tgid_nr(current), ##args)

#define MTK_CAM_TRACE_END(category)					\
	_MTK_CAM_TRACE(_TRACE_CAT(category), "E|%d",			\
		      task_tgid_nr(current))

#define MTK_CAM_TRACE_FUNC_BEGIN(category)				\
	MTK_CAM_TRACE_BEGIN(category, "%s", __func__)

#endif /* __MTK_CAM_TRACE_H */
