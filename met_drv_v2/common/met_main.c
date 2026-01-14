// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/hrtimer.h>
#include <linux/cpu.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/profile.h>
#include <linux/dcache.h>
#include <linux/types.h>
#include <linux/dcookies.h>
#include <linux/sched.h>
#include <linux/fs.h>
/* #include <mt-plat/mtk_chip.h> */

#include <asm/irq_regs.h>

#include "met_struct.h"
#include "met_drv.h"
#include "met_kernel_symbol.h"
#include "interface.h"
#include <linux/of.h>

#if IS_ENABLED(CONFIG_MTK_TINYSYS_SSPM_SUPPORT)
#if defined(ONDIEMET_SUPPORT) || defined(TINYSYS_SSPM_SUPPORT)
#if defined(SSPM_VERSION_V2)
#include "sspm_ipi_id.h"  /* for sspm_ipidev */
#endif
#endif
#endif

extern struct device_node *of_root;
static const char *platform_name;

struct cpu_type_name {
	char full_name[32];
	char abbr_name[8];
};

static struct cpu_type_name met_known_cpu_type[] = {
	{"arm,cortex-a53", "CA53"},
	{"arm,cortex-a55", "CA55"},
	{"arm,cortex-a72", "CA72"},
	{"arm,cortex-a73", "CA73"},
	{"arm,cortex-a75", "CA75"},
	{"arm,cortex-a76", "CA76"},
};
#define MET_KNOWN_CPU_TYPE_COUNT \
	(sizeof(met_known_cpu_type)/sizeof(struct cpu_type_name))

static char met_cpu_topology[64];

struct met_register_tbl *met_register_api_symbol;
EXPORT_SYMBOL(met_register_api_symbol);
struct met_export_tbl *met_export_api_symbol;
EXPORT_SYMBOL(met_export_api_symbol);

#if IS_ENABLED(CONFIG_MTK_TINYSYS_SSPM_SUPPORT)
#if defined(ONDIEMET_SUPPORT) || defined(TINYSYS_SSPM_SUPPORT)
#if defined(SSPM_VERSION_V2)
struct mtk_ipi_device *sspm_ipidev_symbol = NULL;
#endif
#endif
#endif

unsigned int (*mt_get_chip_id_symbol)(void);

static int met_minor = -1;
module_param(met_minor, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);


#ifdef MTK_PLATFORM
#define _SHOW_MTK_PLATFORM(X) #X
#define SHOW_MTK_PLATFORM(X) _SHOW_MTK_PLATFORM(X)
#endif

static int is_platform_name_valid(const char * buf)
{
	int len = strlen(buf);
	int i;

	for (i=0; i<len; i++) {
		if ((buf[i] == 'm' && buf[i+1] == 't')
			|| (buf[i] == 'M' && buf[i+1] == 'T')
			|| (buf[i] == 'M' && buf[i+1] == 't')
			|| (buf[i] == 'm' && buf[i+1] == 'T')) {
			return i;
		}
	}
	return -1;
}

#if 0
const char *met_get_platform_name(void)
{
	const char default_platform_name[7] = "mtxxxx";
	int found = -1;

	found = is_platform_name_valid(platform_name);

	if(found < 0){
#ifdef MTK_PLATFORM
		const char buf[7] = SHOW_MTK_PLATFORM(MTK_PLATFORM);
		found = is_platform_name_valid(buf);
		if ( !(found < 0) )
			platform_name = buf;
		else
#endif
			platform_name = default_platform_name;
	}

	strncpy(&buf_tmp[2], &platform_name[found+2], 4);
	return platform_name;
}
EXPORT_SYMBOL(met_get_platform_name);
#endif

static void get_cpu_type_name(const char *compatible, char *cpu_type)
{
	int i;

	for (i = 0; i < MET_KNOWN_CPU_TYPE_COUNT; i++) {
		if (!strncmp(compatible, met_known_cpu_type[i].full_name,
					strlen(met_known_cpu_type[i].full_name)))
			strncpy(cpu_type, met_known_cpu_type[i].abbr_name,
					strlen(met_known_cpu_type[i].abbr_name) + 1);
	}
}

static void met_set_cpu_topology(int core_id, int cluster_core_num)
{
	int i, buf_len = strlen(met_cpu_topology);
	struct device_node *node = NULL;
	const char *prev_cptb = NULL;
	const char *cptb;
	char cpu_type[16];

	for (i = 0; i < cluster_core_num; i++) {
		node = of_get_cpu_node(core_id + i, NULL);
		if (node) {
			cptb = of_get_property(node, "compatible", NULL);
			if (cptb) {
				get_cpu_type_name(cptb, cpu_type);
				if (prev_cptb == NULL)
					/* first write:  write core_type & core_number */
					buf_len += snprintf(met_cpu_topology + buf_len,
								sizeof(met_cpu_topology) - buf_len,
								"%s:%d", cpu_type, core_id + i);
				else if (!strncmp(prev_cptb, cptb, strlen(prev_cptb)))
					/* cpu type is the same with before */
					/* write core_number */
					buf_len += snprintf(met_cpu_topology + buf_len,
								sizeof(met_cpu_topology) - buf_len,
								",%d", core_id + i);
				else
					/* cpu type is different with before */
					/* write core_type & core_number */
					buf_len += snprintf(met_cpu_topology + buf_len,
								sizeof(met_cpu_topology) - buf_len,
								"|%s:%d", cpu_type, core_id + i);

				prev_cptb = cptb;
			}
		}
	}
}

static int met_create_cpu_topology(void)
{
	int i, j, len;
	struct device_node *node = NULL, *core_node = NULL;
	int start_core_id = 0;
	int cluster_num = 0, cluster_core_num = 0;
	char cluster_name[16], core_name[16];

	node = of_find_node_by_name(NULL, "cpu-map");
	if (!node)
		node = of_find_node_by_name(NULL, "virtual-cpu-map");

	if (node) {
		cluster_num = of_get_child_count(node);
		of_node_put(node);

		for (i = 0; i < cluster_num; i++) {
			snprintf(cluster_name, sizeof(cluster_name), "cluster%d", i);
			node = of_find_node_by_name(NULL, cluster_name);
			if (node) {

				j = 0;
				cluster_core_num = 0;
				do {
					snprintf(core_name, sizeof(core_name), "core%d", j);
					core_node = of_get_child_by_name(node, core_name);
					if (core_node) {
						cluster_core_num++;
						of_node_put(core_node);
					}
					j++;
				} while (core_node);
				of_node_put(node);

				/* "|" use to separate different cluster */
				if (i > 0) {
					len = strlen(met_cpu_topology);
					snprintf(met_cpu_topology + len, sizeof(met_cpu_topology) - len, "|");
				}

				met_set_cpu_topology(start_core_id, cluster_core_num);
				start_core_id = cluster_core_num;
			}
		}
	}

	return strlen(met_cpu_topology);
}

static int met_kernel_symbol_get(void)
{
	int ret = 0;

	ret += _MET_SYMBOL_GET(met_register_api);
	ret += _MET_SYMBOL_GET(met_export_api);

#if IS_ENABLED(CONFIG_MTK_TINYSYS_SSPM_SUPPORT)
#if defined(ONDIEMET_SUPPORT) || defined(TINYSYS_SSPM_SUPPORT)
#if defined(SSPM_VERSION_V2)
	ret += _MET_SYMBOL_GET(sspm_ipidev);
#endif
#endif
#endif
	return ret;
}

static void met_kernel_symbol_put(void)
{
	_MET_SYMBOL_PUT(met_register_api);
	_MET_SYMBOL_PUT(met_export_api);

#if IS_ENABLED(CONFIG_MTK_TINYSYS_SSPM_SUPPORT)
#if defined(ONDIEMET_SUPPORT) || defined(TINYSYS_SSPM_SUPPORT)
#ifdef SSPM_VERSION_V2
	_MET_SYMBOL_PUT(sspm_ipidev);
#endif
#endif
#endif
}

DEFINE_PER_CPU(struct met_cpu_struct, met_cpu);

static int __init met_drv_init(void)
{
	int cpu;
	int ret;
	int cpu_topology_len;
	struct met_cpu_struct *met_cpu_ptr;
	unsigned int chip_id;

	for_each_possible_cpu(cpu) {
		met_cpu_ptr = &per_cpu(met_cpu, cpu);
		/* snprintf(&(met_cpu_ptr->name[0]), sizeof(met_cpu_ptr->name), "met%02d", cpu); */
		met_cpu_ptr->cpu = cpu;
	}

	ret = met_kernel_symbol_get();
	if (ret) {
		pr_notice("[MET] met_kernel_symbol_get fail, ret = %d\n", ret);
		return ret;
	}

	ret = fs_reg(met_minor);
	if (ret) {
		pr_notice("[MET] met fs_reg fail, ret = %d\n", ret);
		return ret;
	}

	if (of_root){
		/*
			mt6765.dts
			model = "MT6765";
			compatible = "mediatek,MT6765";
			interrupt-parent = <&sysirq>;
		*/
		if (of_root->properties) {
			of_property_read_string(of_root, "compatible", &platform_name);
			PR_BOOTMSG("dts property compatible=%s\n", platform_name);
		}
	}
	if (platform_name) {
		char buf[7];
		int found = -1;

		found = is_platform_name_valid(platform_name);

		if ( !(found < 0) ) {
			memset(buf, 0x0, 7);
			buf[0] = 'm';
			buf[1] = 't';
			strncpy(&buf[2], &platform_name[found+2], 4);
			met_set_platform(buf, 1);
			PR_BOOTMSG("Get platform info from dts, platform_name=%s\n", buf);
		} else {
#ifdef MTK_PLATFORM
			memset(buf, 0x0, 7);
			strcpy(buf, SHOW_MTK_PLATFORM(MTK_PLATFORM));
			found = is_platform_name_valid((const char *)buf);
			if( !(found < 0) ){
				PR_BOOTMSG("Get platform info from met_drv Kbuild, platform_name=%s\n", buf);
				met_set_platform(buf, 1);
			}
			else
#endif
			{
				PR_BOOTMSG("Can not get platform info from dts nor met_drv Kbuild, set platform_name=mtxxxx\n");
				met_set_platform("mtxxxx", 1);
			}
		}
	}

	/* get the chip id */
	_MET_SYMBOL_GET(mt_get_chip_id);
	if (mt_get_chip_id_symbol != NULL)
		met_set_chip_id(mt_get_chip_id_symbol());
	else {
		chip_id = met_get_chipid_from_atag();
		if (((int) chip_id) < 0) {
			PR_BOOTMSG("Can not get chip id info, set chip_id=0x0\n");
		} else
			met_set_chip_id(chip_id);
	}

	cpu_topology_len = met_create_cpu_topology();
	if (cpu_topology_len)
		met_set_topology(met_cpu_topology, 1);

#ifdef MET_PLF_USE
	core_plf_init();
#endif
	return 0;
}

static void __exit met_drv_exit(void)
{
#ifdef MET_PLF_USE
	core_plf_exit();
#endif
	fs_unreg();

	met_kernel_symbol_put();
	_MET_SYMBOL_PUT(mt_get_chip_id);
}
module_init(met_drv_init);
module_exit(met_drv_exit);

MODULE_AUTHOR("DT_DM5");
MODULE_DESCRIPTION("MET_CORE");
MODULE_LICENSE("GPL");
