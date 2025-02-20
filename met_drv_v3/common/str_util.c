// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include "str_util.h"
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/slab.h>


struct met_str_array * met_util_str_split(const char *input_str, int delim){

	struct met_str_array *str_array_obj=NULL;

	char *delim_ptr = NULL;

	int index=0;
	int i=0;
	int str_length=0;
	int delim_count=0;

	if(input_str==NULL){
		return NULL;
	}

	str_length = strlen(input_str);
	//pr_info("[met_util_str_split]str=%s str_length=%d\n", input_str, str_length);

	if(str_length == 0){
		return NULL;
	}

	for(;i<str_length;i++){
		if(input_str[i]==delim){
			delim_count++;
		}
	}

	//pr_info("[met_util_str_split]delim_count=%d\n", delim_count);

	str_array_obj = kmalloc(sizeof(struct met_str_array), GFP_KERNEL);

	if(str_array_obj ==NULL){
		pr_info("[met_util_str_split]Failed to allocate memory for a struct met_str_array obj\n");
		return NULL;
	}

	/*partial init str_array_obj due to kzalloc not work for coverity scan tool*/
	str_array_obj->str_ptr_array_length = 0;
	str_array_obj->str_ptr_array = NULL;
	str_array_obj->target_str = NULL;


	str_array_obj->target_str = kmalloc(str_length+1, GFP_KERNEL);
	if(str_array_obj->target_str ==NULL){
		pr_info("[met_util_str_split]Failed to allocate memory for target str\n");
		met_util_str_array_clean(str_array_obj);
		return NULL;
	}

	strncpy(str_array_obj->target_str, input_str, str_length);
	*(str_array_obj->target_str + str_length)='\0';

	//pr_info("[met_util_str_split]str_array_obj->target_str=%s\n", str_array_obj->target_str);

	str_array_obj->str_ptr_array = kmalloc(sizeof(char *)*(delim_count+1), GFP_KERNEL);
	if(str_array_obj->str_ptr_array ==NULL){
		pr_info("[met_util_str_split]Failed to allocate memory for str array\n");
		met_util_str_array_clean(str_array_obj);
		return NULL;
	}

	delim_ptr = strrchr(str_array_obj->target_str, delim);

	/*create a list to storage process filter list*/
	while(delim_ptr != NULL){
		if(strlen(delim_ptr+1)!=0){
			str_array_obj->str_ptr_array[index] = delim_ptr+1;
			//pr_info("str_array[%d]=%s, len=%d\n", index, str_array_obj->str_ptr_array[index], strlen(str_array_obj->str_ptr_array[index]));
			index++;
		}
		*delim_ptr = '\0';
		delim_ptr = strrchr(str_array_obj->target_str, delim);
	}

	if(strlen(str_array_obj->target_str)!=0){
		str_array_obj->str_ptr_array[index] = str_array_obj->target_str;
		str_array_obj->str_ptr_array_length = index+1;
	}

	for(i=0; i<str_array_obj->str_ptr_array_length; i++){
		pr_info("str_array[%d]=%s\n", i, str_array_obj->str_ptr_array[i]);
	}

	return str_array_obj;
}


void met_util_str_array_clean(struct met_str_array *str_array_obj){
	if(str_array_obj !=NULL){

		if(str_array_obj ->target_str != NULL){
			kfree(str_array_obj ->target_str);
			str_array_obj ->target_str = NULL;
		}

		if(str_array_obj -> str_ptr_array != NULL){
			kfree(str_array_obj -> str_ptr_array);
			str_array_obj -> str_ptr_array = NULL;
		}

		kfree(str_array_obj);
	}
}


int met_util_in_str_array(const char *input_str, int compare_char_count, struct met_str_array *str_array_obj){
	int found =0;
	int i=0;

	if(str_array_obj==NULL || str_array_obj->str_ptr_array_length==0){
		return found;
	}

	i=str_array_obj->str_ptr_array_length-1;

	for(;i>=0; i--){
		if(compare_char_count){
			if(strncmp(input_str, str_array_obj->str_ptr_array[i] , compare_char_count)==0){
				found=1;
				break;
			}
		}else{
			if(strcmp(input_str, str_array_obj->str_ptr_array[i])==0){
				found=1;
				break;
			}
		}
	}

	return found;
}




