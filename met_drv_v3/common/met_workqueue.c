/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */


#include <trace/events/workqueue.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/list.h>
#include <linux/smp.h>

#include "interface.h"
#include "met_drv.h"
#include "cpu_pmu.h"
#include "sampler.h"
#include "met_kernel_symbol.h"
#include "str_util.h"
#include "switch.h"
#include "mtk_typedefs.h"

#define MAX_FUNC_LENGTH 64

struct fun_ptr_node{
	void * fun_ptr;
	struct list_head list;
};

struct workqueue_fun_node{
	char name[MAX_FUNC_LENGTH];
};

static int met_workqueue_is_valid_func_name(const char * func_name);
static void met_workqueue_list_clean(struct list_head *head);
static int check_in_list(struct list_head *head, void *fun_ptr, unsigned int *list_length);

static int init_fail_flag=0;

static struct met_str_array *workqueue_func_filter_list; 

//allocate per cpu variable
static int __percpu *per_cpu_valid_list_length;
static struct workqueue_fun_node __percpu *per_cpu_workqueue_func_node;
static struct list_head __percpu *per_cpu_valid_list;
static struct list_head __percpu *per_cpu_invalid_list;


static void clean_per_cpu_var(){
	int cpu;

	for_each_possible_cpu(cpu) {
		if(cpu<0 || cpu>=NR_CPUS)
			continue;

		met_workqueue_list_clean(per_cpu_ptr(per_cpu_valid_list, cpu));
		met_workqueue_list_clean(per_cpu_ptr(per_cpu_invalid_list, cpu));

		/*if(list_empty(per_cpu_ptr(per_cpu_valid_list, cpu))){
			pr_info("[met_workqueue_clean_per_cpu_var]per_cpu_valid_list for cpu=%d is empty\n", cpu);
		}

		if(list_empty(per_cpu_ptr(per_cpu_invalid_list, cpu))){
			pr_info("[met_workqueue_clean_per_cpu_var]per_cpu_invalid_list for cpu=%d is empty\n", cpu);
		}*/
				
		*per_cpu_ptr(per_cpu_valid_list_length, cpu)=0;

		per_cpu_ptr(per_cpu_workqueue_func_node, cpu)->name[0]='\0';
	}
}


static void probe_workqueue_execute_start(void *data, struct work_struct *work){

	int found =0;	
	struct fun_ptr_node *node = NULL;
	int fun_name_len=0;
	int cpu_id = smp_processor_id();

	if (init_fail_flag || cpu_id<0){
		return;
	}

	//pr_info("[probe_workqueue_execute_start]callback function=%ps \n", work->func);
	
	list_for_each_entry(node, per_cpu_ptr(per_cpu_valid_list, cpu_id), list) {
		if (node->fun_ptr == work->func){
			found=1;
			//pr_info("[probe_workqueue_execute_start]valid callback function=%ps cpu=%d\n", work->func, cpu_id);
			break;
		}
	}

	if(!found){
		int need_check =1;

		if(*per_cpu_ptr(per_cpu_valid_list_length, cpu_id) >= workqueue_func_filter_list->str_ptr_array_length){
			//pr_info("[probe_workqueue_execute_start]valid list is full, return directly, cpu=%d\n", cpu_id);
			return;
		}

		list_for_each_entry(node, per_cpu_ptr(per_cpu_invalid_list, cpu_id), list) {
			if (node->fun_ptr == work->func){
				need_check=0;
				//pr_info("[probe_workqueue_execute_start]invalid callback function=%ps, cpu=%d\n", work->func, cpu_id);
				break;
			}
		}

		if(need_check){
			//not in valid list, nor in invalid list, need to check
			struct fun_ptr_node *node = NULL;
			unsigned int current_cpu=0;

			node = kmalloc(sizeof(struct fun_ptr_node), GFP_KERNEL);
			if (node == NULL){
				return;
			}
			
			INIT_LIST_HEAD(&node->list);
			node->fun_ptr = work->func;

			current_cpu = get_cpu();
			
			fun_name_len = snprintf(per_cpu_ptr(per_cpu_workqueue_func_node, current_cpu)->name, MAX_FUNC_LENGTH, "%ps", work->func);
			if(fun_name_len>0){
				if(met_workqueue_is_valid_func_name(per_cpu_ptr(per_cpu_workqueue_func_node, current_cpu)->name)){
					//pr_info("[probe_workqueue_execute_start]add %ps to valid list, cpu=%d\n", work->func, current_cpu);
					unsigned int list_length =0;
					int already_in_list=0;

					already_in_list = check_in_list(per_cpu_ptr(per_cpu_valid_list, current_cpu), node->fun_ptr, &list_length);
					if(!already_in_list){
						list_add(&node->list, per_cpu_ptr(per_cpu_valid_list, current_cpu));					
						*per_cpu_ptr(per_cpu_valid_list_length, current_cpu) = list_length+1;
					}
					found=1;
					//pr_info("[probe_workqueue_execute_start]valid_list_length=%d, cpu=%d\n", *per_cpu_ptr(per_cpu_valid_list_length, current_cpu), current_cpu);
				}else{
					//pr_info("[probe_workqueue_execute_start]add %ps to invalid list, cpu=%d\n", work->func, current_cpu);
					list_add(&node->list, per_cpu_ptr(per_cpu_invalid_list, current_cpu));
				}
			}

			put_cpu();
		}			
	}

	if(found){
		met_perf_cpupmu_polling(0, smp_processor_id());
	}
}



static void probe_workqueue_execute_end(void *data, struct work_struct *work, work_func_t function){


	int found =0;
	struct fun_ptr_node *node = NULL;
	int fun_name_len=0;
	int cpu_id = smp_processor_id();

	if (init_fail_flag || cpu_id<0){
		return;
	}
		
	//pr_info("[probe_workqueue_execute_end]");
	
	list_for_each_entry(node, per_cpu_ptr(per_cpu_valid_list, cpu_id), list) {
		if (node->fun_ptr == function){
			found=1;
			//pr_info("[probe_workqueue_execute_end]valid callback function=%ps\n", function);
			break;
		}
	}

	if(!found){
		int need_check =1;

		if(*per_cpu_ptr(per_cpu_valid_list_length, cpu_id) >= workqueue_func_filter_list->str_ptr_array_length){
			//pr_info("[probe_workqueue_execute_end]valid list is full, return directly, cpu=%d\n", smp_processor_id());
			return;
		}

		list_for_each_entry(node, per_cpu_ptr(per_cpu_invalid_list, cpu_id), list) {
			if (node->fun_ptr == function){
				need_check=0;
				//pr_info("[probe_workqueue_execute_end]invalid callback function=%ps, cpu=%d\n", function, smp_processor_id());
				break;
			}
		}

		if(need_check){
			//not in valid list, nor in invalid list, need to check
			struct fun_ptr_node *node = NULL;
			int current_cpu=0;

			node = kmalloc(sizeof(struct fun_ptr_node), GFP_KERNEL);
			if (node == NULL){
				return;
			}
						
			INIT_LIST_HEAD(&node->list);
			node->fun_ptr = function;

			current_cpu = get_cpu();

			if(current_cpu<0){
				return;
			}

			fun_name_len = snprintf(per_cpu_ptr(per_cpu_workqueue_func_node, current_cpu)->name, MAX_FUNC_LENGTH, "%ps", function);
			if(fun_name_len>0){
				if(met_workqueue_is_valid_func_name(per_cpu_ptr(per_cpu_workqueue_func_node, current_cpu)->name)){
					//pr_info("[probe_workqueue_execute_end]add %ps to valid list, cpu=%d\n", function, current_cpu);
					unsigned int list_length =0;
					int already_in_list=0;

					already_in_list = check_in_list(per_cpu_ptr(per_cpu_valid_list, current_cpu), node->fun_ptr, &list_length);
					if(!already_in_list){
						list_add(&node->list, per_cpu_ptr(per_cpu_valid_list, current_cpu));					
						*per_cpu_ptr(per_cpu_valid_list_length, current_cpu) = list_length+1;
					}
					found=1;
					//pr_info("[probe_workqueue_execute_end]valid_list_length=%d, cpu=%d\n", *per_cpu_ptr(per_cpu_valid_list_length, current_cpu), current_cpu);
				}else{
					//pr_info("[probe_workqueue_execute_end]add %ps to invalid list, cpu=%d\n", function, current_cpu);
					list_add(&node->list, per_cpu_ptr(per_cpu_invalid_list, current_cpu));
				}
			}
			put_cpu();
		}
	}
	
	if(found){
		met_perf_cpupmu_polling(0, smp_processor_id());
	}
}


static int met_workqueue_create_subfs(struct kobject *parent)
{
	int ret = 0;
	int cpu=0;
	
	/*this function will be called @ each execution of insmod met.ko */
	//pr_info("[met_workqueue_create_subfs]\n");
	
	init_fail_flag=0;
	
	per_cpu_valid_list_length = alloc_percpu(typeof(*per_cpu_valid_list_length));
	if (!per_cpu_valid_list_length) {
		PR_BOOTMSG("percpu per_cpu_valid_list_length allocate fail\n");
		pr_debug("percpu per_cpu_valid_list_length allocate fail\n");
		init_fail_flag = 1;
		return 0;
	}

	per_cpu_workqueue_func_node = alloc_percpu(typeof(*per_cpu_workqueue_func_node));
	if (!per_cpu_workqueue_func_node) {
		PR_BOOTMSG("percpu per_cpu_workqueue_func_node allocate fail\n");
		pr_debug("percpu per_cpu_workqueue_func_node allocate fail\n");
		init_fail_flag = 1;
		return 0;
	}

	per_cpu_valid_list = alloc_percpu(typeof(*per_cpu_valid_list));
	if (!per_cpu_valid_list) {
		PR_BOOTMSG("percpu per_cpu_valid_list allocate fail\n");
		pr_debug("percpu per_cpu_valid_list allocate fail\n");
		init_fail_flag = 1;
		return 0;
	}

	per_cpu_invalid_list = alloc_percpu(typeof(*per_cpu_invalid_list));
	if (!per_cpu_invalid_list) {
		PR_BOOTMSG("percpu per_cpu_invalid_list allocate fail\n");
		pr_debug("percpu per_cpu_invalid_list allocate fail\n");
		init_fail_flag = 1;
		return 0;
	}
	
	/* cpu from 0 to 7*/
	for_each_possible_cpu(cpu) {
		if(cpu<0 || cpu>=NR_CPUS)
			continue;

		/*init head of list for each cpu*/
		INIT_LIST_HEAD(per_cpu_ptr(per_cpu_valid_list, cpu));
		INIT_LIST_HEAD(per_cpu_ptr(per_cpu_invalid_list, cpu));
		//pr_info("[met_workqueue_create_subfs]init list head for cpu=%d\n", cpu);
	}
	//pr_info("[met_workqueue_create_subfs]per cpu variable init successfully\n");
	
	return ret;
}


static void met_workqueue_delete_subfs(void)
{
	//pr_info("[met_workqueue_delete_subfs]\n");
	 
	if (per_cpu_valid_list_length) {
		free_percpu(per_cpu_valid_list_length);
	}

	if (per_cpu_workqueue_func_node) {
		free_percpu(per_cpu_workqueue_func_node);
	}

	if (per_cpu_valid_list) {
		free_percpu(per_cpu_valid_list);
	}

	if (per_cpu_invalid_list) {
		free_percpu(per_cpu_invalid_list);
	}
}


static void met_workqueue_start(void)
{
	/*only register this callback when we add --workqueue_filter=xxxx in mtxxxx.sh*/

	if (init_fail_flag) {
		MET_TRACE("percpu var for met workqueue allocate fail, will not start met_workqueue function\n");
		met_workqueue.mode = 0;
		return;
	}

	/* register tracepoints */
	met_tracepoint_probe_reg("workqueue_execute_start", probe_workqueue_execute_start);
	met_tracepoint_probe_reg("workqueue_execute_end", probe_workqueue_execute_end);

	//pr_info("[met_workqueue_start]register tracepoints successfully\n");
}


static void met_workqueue_stop(void)
{
	//pr_info("[met_workqueue_stop]");

	/* unregister tracepoints */
	met_tracepoint_probe_unreg("workqueue_execute_start", probe_workqueue_execute_start);
	met_tracepoint_probe_unreg("workqueue_execute_end", probe_workqueue_execute_end);
}


static int met_workqueue_process_argument(const char *arg, int len)
{
    /*this function will be called @ each execution of go.long.bat */

	/*reset variable*/
	met_util_str_array_clean(workqueue_func_filter_list);
	workqueue_func_filter_list = NULL;
	clean_per_cpu_var();
	met_workqueue.mode = 0;

	pr_info("[met_workqueue_process_argument]arg=%s, len=%d\n", arg, len);

	if(len == 0 || arg == NULL){
		return 0;
	}

	/*return a pointer to a struct met_str_array object*/
	workqueue_func_filter_list = met_util_str_split(arg, ',');

	/*only when met_workqueue.mode==1, then met_workqueue_start() will be called*/

	if(workqueue_func_filter_list==NULL){
		met_workqueue.mode = 0;
	}else{
		if(workqueue_func_filter_list->str_ptr_array_length==0){
			met_workqueue.mode = 0;
		}else{
			met_workqueue.mode = 1;
		}
		//pr_info("[met_workqueue_process_argument] workqueue_func_filter_list->str_ptr_array_length=%d\n", workqueue_func_filter_list->str_ptr_array_length);
	}

	pr_info("[met_workqueue_process_argument] met_workqueue.mode=%d\n", met_workqueue.mode);

	return 0;
}


static int met_workqueue_is_valid_func_name(const char * func_name){

	char *ptr = NULL;
	int compare_char_count=0;

	if(func_name == NULL){
		return 0;
	}

	//func_name may have following types:
	//	1. kbase_jd_done_worker.cfi_jt [mali_kbase]
	//	2. ged_kpi_work_cb [ged]
	//	3. ffs_user_copy_worker

	ptr = strchr(func_name, '.');

	if(ptr!=NULL){
		compare_char_count = ptr - func_name;
	}else{
		ptr = strchr(func_name, ' ');
		if(ptr!=NULL){
			compare_char_count = ptr - func_name;
		}
	}
	return met_util_in_str_array(func_name, compare_char_count, workqueue_func_filter_list);
}


static void met_workqueue_list_clean(struct list_head *head){
	struct fun_ptr_node *node = NULL;
	struct fun_ptr_node *tmp = NULL;

	if(head ==NULL){
		return;
	}
	
	list_for_each_entry_safe(node, tmp, head, list) {
		if(node != NULL){
			list_del(&node->list);
			kfree(node);
			node = NULL;
		}
	}
}

static int check_in_list(struct list_head *head, void *fun_ptr, unsigned int *list_length){
	unsigned int length=0;
	int found=0;
	struct fun_ptr_node *node = NULL;

	list_for_each_entry(node, head, list) {
		if (node->fun_ptr == fun_ptr){
			found=1;
			length++;
		}
	}
	*list_length = length;
	return found;
}

static const char help[] =
"  please provide at least one workgueue fun and reference following example\n"
"  --worqueue_filter=func1                     trace only one workgueue fun\n"
"  --worqueue_filter=func1,func2,func3,....    trace multiple workgueue fun\n";


static int met_workqueue_print_help(char *buf, int len)
{
	return snprintf(buf, PAGE_SIZE, help);
}


static int met_workqueue_print_header(char *buf, int len)
{
	char *start_point = buf;
	int i=0;

	if(workqueue_func_filter_list!=NULL && workqueue_func_filter_list->str_ptr_array_length>0){
		/*use str array*/
		i = workqueue_func_filter_list->str_ptr_array_length-1;
		for(; i>=0; i--){
			int n = snprintf(start_point, PAGE_SIZE, "met-info [000] 0.0: workqueue_filter: %s\n", workqueue_func_filter_list->str_ptr_array[i]);
			//pr_info("[met_workqueue_print_header] n=%d\n", n);
			start_point += n;
		}
	}	
	
	return start_point-buf;
}


struct metdevice met_workqueue = {
	.name = "workqueue_pmu",
	.type = MET_TYPE_PMU,
	.create_subfs = met_workqueue_create_subfs,
	.delete_subfs = met_workqueue_delete_subfs,
	.start = met_workqueue_start,
	.stop = met_workqueue_stop,
	.process_argument = met_workqueue_process_argument,
	.print_help = met_workqueue_print_help,
	.print_header = met_workqueue_print_header,
};

