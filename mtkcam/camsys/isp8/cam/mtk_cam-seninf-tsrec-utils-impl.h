/* SPDX-License-Identifier: GPL-2.0 */
// Copyright (c) 2023 MediaTek Inc.

#ifndef __MTK_CAM_SENINF_TSREC_UTILS_IMPL_H__
#define __MTK_CAM_SENINF_TSREC_UTILS_IMPL_H__


#ifndef FS_UT
#include <linux/of.h>           /* for dts property */
#include <linux/of_irq.h>       /* for dts of node irq */
#endif

#include "mtk_cam-seninf-tsrec-def.h"


/*******************************************************************************
 * TSREC utils --- structures/enums/define/etc
 ******************************************************************************/

/*----------------------------------------------------------------------------*/
// dts
/*----------------------------------------------------------------------------*/
#define SENINF_TOP_COMP_NAME            "mediatek,seninf-core"
#define TSREC_TOP_COMP_NAME             "mediatek,seninf-tsrec-top"
#define TSREC_COMP_NAME                 "mediatek,seninf-tsrec"
#define TSREC_GROUP_NAME                "tsrecs"


/*******************************************************************************
 * TSREC utils --- static inline functions
 ******************************************************************************/
#ifndef FS_UT
/*----------------------------------------------------------------------------*/
// dts
/*----------------------------------------------------------------------------*/
static inline struct device_node *tsrec_utils_of_find_node_by_name(
	struct device_node *from, const char *name, const char *caller)
{
	struct device_node *np;

	np = of_find_node_by_name(from, name);
	if (unlikely(np == NULL)) {
		TSREC_LOG_INF(
			"[%s] ERROR: can't find dts node's name:'%s' from node:'%s'\n",
			caller, name, from->full_name);
	}
	return np;
}


static inline struct device_node *tsrec_utils_of_find_comp_node(
	struct device_node *from, const char *compatible_name,
	const char *caller, const bool log_type)
{
	struct device_node *np;

	np = of_find_compatible_node(from, NULL, compatible_name);
	if (unlikely(np == NULL)) {
		if (log_type) {
			TSREC_LOG_INF(
				"[%s] ERROR: can't find dts compatible name:'%s' from node:'%s'\n",
				caller, compatible_name, from->full_name);
		} else {
			TSREC_LOG_INF(
				"[%s] WARNING: can't find dts compatible_name:'%s' from node:'%s', this will cause all operation that related to this node to be skipped\n",
				caller, compatible_name, from->full_name);
		}
	}
	return np;
}


/* return: 0 on succes */
/*         , -EINVAL    if the property does not exist */
/*         , -ENODATA   if the property does not have a value */
/*         , -EOVERFLOW if the property data isn't large enough. */
static inline int tsrec_utils_of_prop_r_u32(struct device_node *node,
	const char *prop_name, unsigned int *p_val,
	const char *caller, const bool en_log)
{
	int ret = 0;

	ret = of_property_read_u32(node, prop_name, p_val);
	if (unlikely((ret != 0) && en_log)) {
		TSREC_LOG_INF(
			"[%s] ERROR: get dts property name:'%s' from node:'%s' failed, ret:%d(-EINVAL:%d/-ENODATA:%d/-EOVERFLOW:%d)\n",
			caller, prop_name, node->full_name,
			ret, -EINVAL, -ENODATA, -EOVERFLOW);
	}
	return ret;
}


static inline int tsrec_utils_of_irq_count(struct device_node *dev)
{
	struct of_phandle_args irq;
	int nr = 0;

	while (of_irq_parse_one(dev, nr, &irq) == 0)
		nr++;
	return nr;
}
#endif // !FS_UT

#endif
