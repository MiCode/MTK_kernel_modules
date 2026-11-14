/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#define pr_fmt(fmt) KBUILD_MODNAME "@(%s:%d) " fmt, __func__, __LINE__

#include <linux/firmware.h>
#include "conninfra_conf.h"

/*******************************************************************************
*                         D A T A   T Y P E S
********************************************************************************
*/
struct parse_data {
	char *name;
	int (*parser)(const struct parse_data *data, const char *value);
	char *(*writer)(const struct parse_data *data);
	void (*relase_mem)(const struct parse_data *data);
	/*PCHAR param1, *param2, *param3; */
	/* TODO:[FixMe][George] CLARIFY WHAT SHOULD BE USED HERE!!! */
	char *param1;
	char *param2;
	char *param3;
};


/*******************************************************************************
*                       P R I V A T E   D A T A
********************************************************************************
*/

#define CONNINFRA_CONF_EXP_FILTER_MAX_NUM	10
#define CONNINFRA_CONF_EXP_FILTER_MAX_LEN	40
#define CONNINFRA_CONF_MAX_STRING_SIZE		(CONNINFRA_CONF_EXP_FILTER_MAX_NUM * CONNINFRA_CONF_EXP_FILTER_MAX_LEN)

static char conf_exp_filter_list[CONNINFRA_CONF_EXP_FILTER_MAX_NUM][CONNINFRA_CONF_EXP_FILTER_MAX_LEN];

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

struct conninfra_conf g_conninfra_conf;

/******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
*******************************************************************************
*/
static int conf_parse_char(const struct parse_data *data, const char *pos);

static char *conf_write_char(const struct parse_data *data);

static int conf_parse_short(const struct parse_data *data, const char *pos);

static char *conf_write_short(const struct parse_data *data);

static int conf_parse_int(const struct parse_data *data, const char *pos);

static char *conf_write_int(const struct parse_data *data);

static int conf_parse_byte_array(const struct parse_data *data,
					const char *pos);

static char *conf_write_byte_array(const struct parse_data *data);

static void conf_release_byte_array(const struct parse_data *data);

static int conf_parse_pair(const char *key, const char *pVal);

static int conf_parse(const char *pInBuf, unsigned int size);

static int conf_parse_string(const struct parse_data *data,
					const char *pos);

static char *conf_write_string(const struct parse_data *data);

static void conf_release_string(const struct parse_data *data);

//#define OFFSET(v) ((void *) &((struct conninfra_conf*) 0)->v)

#define CHAR(f) {#f, conf_parse_char, conf_write_char, NULL, (char *)&g_conninfra_conf.f, NULL, NULL}

#define SHORT(f) {#f, conf_parse_short, conf_write_short, NULL, (char *)&g_conninfra_conf.f, NULL, NULL}

#define INT(f) {#f, conf_parse_int, conf_write_int, NULL, (char *)&g_conninfra_conf.f, NULL, NULL}

#define BYTE_ARRAY(f) {#f, conf_parse_byte_array, conf_write_byte_array, conf_release_byte_array,\
		(char *)&g_conninfra_conf.f, NULL, NULL}

#define STRING(f) {#f, conf_parse_string, conf_write_string, conf_release_string,\
		(char *)&g_conninfra_conf.f, NULL, NULL}

/*******************************************************************************
*                          F U N C T I O N S
********************************************************************************
*/

static const struct parse_data cfg_fields[] = {
	SHORT(dummy_short),
	CHAR(co_clock_flag),

	CHAR(tcxo_gpio),
	CHAR(pre_cal_mode),
	INT(vcn33_1_voltage),

	BYTE_ARRAY(dummy_byte_ary),
	STRING(exp_filter),
};

#define NUM_CFG_FIELDS (osal_sizeof(cfg_fields) / osal_sizeof(cfg_fields[0]))

static int conf_parse_char(const struct parse_data *data, const char *pos)
{
	char *dst;
	long res = 0;

	dst = (char *)(data->param1);

	if ((osal_strlen(pos) > 2) && ((*pos) == '0') && (*(pos + 1) == 'x')) {
		osal_strtol(pos + 2, 16, &res);
		*dst = (char)res;
		pr_debug("cfg==> %s=0x%x\n", data->name, *dst);
	} else {
		osal_strtol(pos, 10, &res);
		*dst = (char)res;
		pr_debug("cfg==> %s=%d\n", data->name, *dst);
	}
	return 0;
}

static char *conf_write_char(const struct parse_data *data)
{
	char *src;
	int res;
	char *value;

	src = data->param1;

	value = osal_malloc(20);
	if (value == NULL)
		return NULL;
	res = osal_snprintf(value, 20, "0x%x", *src);
	if (res < 0 || res >= 20) {
		osal_free(value);
		return NULL;
	}
	value[20 - 1] = '\0';
	return value;
}

static int conf_parse_short(const struct parse_data *data, const char *pos)
{
	unsigned short *dst;
	long res = 0;

	dst = (unsigned short *)data->param1;

	if ((osal_strlen(pos) > 2) && ((*pos) == '0') && (*(pos + 1) == 'x')) {
		osal_strtol(pos + 2, 16, &res);
		*dst = (unsigned short)res;
		pr_debug("cfg==> %s=0x%x\n", data->name, *dst);
	} else {
		osal_strtol(pos, 10, &res);
		*dst = (unsigned short)res;
		pr_debug("cfg==> %s=%d\n", data->name, *dst);
	}

	return 0;
}

static char *conf_write_short(const struct parse_data *data)
{
	short *src;
	int res;
	char *value;

	/* TODO: [FixMe][George] FIX COMPILE WARNING HERE! */
	src = (short *)data->param1;

	value = osal_malloc(20);
	if (value == NULL)
		return NULL;
	res = osal_snprintf(value, 20, "0x%x", *src);
	if (res < 0 || res >= 20) {
		osal_free(value);
		return NULL;
	}
	value[20 - 1] = '\0';
	return value;
}

static int conf_parse_int(const struct parse_data *data, const char *pos)
{
	int *dst;
	long res = 0;

	dst = (int *)data->param1;

	if ((osal_strlen(pos) > 2) && ((*pos) == '0') && (*(pos + 1) == 'x')) {
		osal_strtol(pos + 2, 16, &res);
		*dst = (unsigned int)res;
		pr_debug("cfg==> %s=0x%x\n", data->name, *dst);
	} else {
		osal_strtol(pos, 10, &res);
		*dst = (unsigned int)res;
		pr_debug("cfg==> %s=%d\n", data->name, *dst);
	}

	return 0;
}

static char *conf_write_int(const struct parse_data *data)
{
	int *src;
	int res;
	char *value;

	src = (unsigned int *) data->param1;

	value = osal_malloc(20);
	if (value == NULL)
		return NULL;
	res = osal_snprintf(value, 20, "0x%x", *src);
	if (res < 0 || res >= 20) {
		osal_free(value);
		return NULL;
	}
	value[20 - 1] = '\0';
	return value;
}

static int conf_parse_byte_array(const struct parse_data *data,
						const char *pos)
{
	unsigned char **dst;
	struct conf_byte_ary *ba;
	unsigned char *buffer;
	int size = osal_strlen(pos) / 2;
	unsigned char temp[3];
	int i;
	long value;

	if (size <= 1) {
		pr_err("cfg==> %s has no value assigned\n",
			data->name);
		return -1;
	} else if (size & 0x1) {
		pr_debug("cfg==> %s, length should be even\n", data->name);
		return -1;
	}

	ba = (struct conf_byte_ary *)osal_malloc(sizeof(struct conf_byte_ary));
	if (ba == NULL) {
		pr_err("cfg==> %s malloc fail\n", data->name);
		return -1;
	}

	buffer = osal_malloc(size);
	if (buffer == NULL) {
		osal_free(ba);
		pr_err("cfg==> %s malloc fail, size %d\n", data->name, size);
		return -1;
	}

	temp[2] = '\0';
	for (i = 0; i < size; i++) {
		osal_memcpy(temp, &pos[i * 2], 2);
		if (osal_strtol(temp, 16, &value) < 0) {
			pr_err("cfg==> %s should be hexadecimal format\n",
						data->name);
			osal_free(ba);
			osal_free(buffer);
			return -1;
		}
		buffer[i] = (char)value;
	}
	ba->data = buffer;
	ba->size = size;

	dst = (unsigned char **) data->param1;
	*dst = (unsigned char *)ba;

	return 0;
}

static char *conf_write_byte_array(const struct parse_data *data)
{
	unsigned char **src;
	char *value;
	struct conf_byte_ary *ba;
	int i;

	src = (unsigned char **) data->param1;
	if (*src == NULL)
		return NULL;

	ba = (struct conf_byte_ary *)*src;

	value = osal_malloc(ba->size * 2 + 1);
	if (value == NULL)
		return NULL;

	for (i = 0; i < ba->size; i++)
		osal_snprintf(&value[i * 2], 3, "%x", ba->data[i]);

	return value;
}


static void conf_release_byte_array(const struct parse_data *data)
{
	//unsigned char *buffer;
	struct conf_byte_ary *ba;
	unsigned char **src;

	if (data->param1 != NULL) {
		src = (unsigned char **) data->param1;
		ba = (struct conf_byte_ary *) *src;
		if (ba->data != NULL) {
			osal_free(ba->data);
			ba->data = NULL;
		}
		osal_free(ba);
		//data->param1 = NULL;
		*src = NULL;
	}
}

static int conf_parse_string(const struct parse_data *data,
					const char *pos)
{
	int size = osal_strlen(pos);
	struct conf_string_data *str;
	char *buffer;
	unsigned char **dst;

	if (size <= 0) {
		pr_notice("cfg==> %s has no value assigned\n",
			data->name);
		return -1;
	} else if (size >= CONNINFRA_CONF_MAX_STRING_SIZE) {
		pr_notice("cfg==> %s value too long(%d > %d)\n", data->name, size, CONNINFRA_CONF_MAX_STRING_SIZE);
		size = CONNINFRA_CONF_MAX_STRING_SIZE;
	}

	str = (struct conf_string_data *)osal_malloc(sizeof(struct conf_string_data));
	if (str == NULL) {
		pr_notice("cfg==> %s malloc fail\n", data->name);
		return -1;
	}
	buffer = osal_malloc(sizeof(char) * (size + 1));
	if (buffer == NULL) {
		osal_free(str);
		pr_notice("cfg==> %s malloc fail, size %d\n", data->name, size);
		return -1;
	}
	memset(buffer, '\0', sizeof(char) * (size + 1));

	strncpy(buffer, pos, size);
	str->data = buffer;
	str->size = size;

	dst = (unsigned char**) data->param1;
	*dst = (unsigned char*) str;
	return 0;
}

static char *conf_write_string(const struct parse_data *data)
{
	unsigned char **src;
	char *value;
	struct conf_string_data *str;
	int str_size;

	src = (unsigned char **) data->param1;
	if (*src == NULL)
		return NULL;

	str = (struct conf_string_data *)*src;

	str_size = sizeof(char) * (str->size + 1);
	value = osal_malloc(str_size);
	if (value == NULL)
		return NULL;

	memset(value, '\0', str_size);
	strncpy(value, str->data, str->size);

	return value;
}

static void conf_release_string(const struct parse_data *data)
{
	struct conf_string_data *str;
	unsigned char **src;

	if (data->param1 != NULL) {
		src = (unsigned char **) data->param1;
		str = (struct conf_string_data *) *src;
		if (str != NULL) {
			if (str->data != NULL) {
				osal_free(str->data);
				str->data = NULL;
			}
			osal_free(str);
		}
		//data->param1 = NULL;
		*src = NULL;
	}
}

static int conf_parse_pair(const char *pKey, const char *pVal)
{
	int i = 0;
	int ret = 0;

	for (i = 0; i < NUM_CFG_FIELDS; i++) {
		const struct parse_data *field = &cfg_fields[i];

		if (osal_strcmp(pKey, field->name) != 0)
			continue;
		if (field->parser(field, pVal)) {
			pr_err("failed to parse %s '%s'.\n", pKey, pVal);
			ret = -1;
		}
		break;
	}
	if (i == NUM_CFG_FIELDS) {
		pr_err("unknown field '%s'.\n", pKey);
		ret = -1;
	}

	return ret;
}

static int conf_parse(const char *pInBuf, unsigned int size)
{
	char *pch;
	char *pBuf;
	char *pLine;
	char *pKey;
	char *pVal;
	char *pPos;
	int ret = 0;
	int i = 0;
	char *pa = NULL;

	pBuf = osal_malloc(size+1);
	if (!pBuf)
		return -1;

	osal_memcpy(pBuf, pInBuf, size);
	pBuf[size] = '\0';

	pch = pBuf;
	/* pch is to be updated by strsep(). Keep pBuf unchanged!! */

	while ((pLine = osal_strsep(&pch, "\r\n")) != NULL) {
		/* pch is updated to the end of pLine by
		 * strsep() and updated to '\0'
		 */
		/* parse each line */

		if (!*pLine)
			continue;

		pVal = osal_strchr(pLine, '=');
		if (!pVal) {
			pr_warn("mal-format cfg string(%s)\n", pLine);
			continue;
		}

		/* |<-pLine->|'='<-pVal->|'\n' ('\0')|  */
		*pVal = '\0';	/* replace '=' with '\0' to get key */
		/* |<-pKey->|'\0'|<-pVal->|'\n' ('\0')|  */
		pKey = pLine;

		if ((pVal - pBuf) < size)
			pVal++;

		/*key handling */
		pPos = pKey;
		/*skip space characeter */
		while (((*pPos) == ' ') ||
				((*pPos) == '\t') || ((*pPos) == '\n')) {
			if ((pPos - pBuf) >= size)
				break;
			pPos++;
		}
		/*key head */
		pKey = pPos;
		while (((*pPos) != ' ') &&
				((*pPos) != '\t') && ((*pPos) != '\0')
		       && ((*pPos) != '\n')) {
			if ((pPos - pBuf) >= size)
				break;
			pPos++;
		}
		/*key tail */
		(*pPos) = '\0';

		/*value handling */
		pPos = pVal;
		/* skip leading space characeter for value */
		while (((*pPos) == ' ') ||
				((*pPos) == '\t') || ((*pPos) == '\n')) {
			if ((pPos - pBuf) >= size)
				break;
			pPos++;
		}
		/*value head */
		pVal = pPos;
		/* Don't skip space character for key */
		while (/*((*pPos) != ' ') &&*/
				((*pPos) != '\t') && ((*pPos) != '\0')
		       && ((*pPos) != '\n')) {
			if ((pPos - pBuf) >= size)
				break;
			pPos++;
		}
		/*value tail */
		(*pPos) = '\0';

		ret = conf_parse_pair(pKey, pVal);
		pr_debug("parse (%s, %s, %d)\n", pKey, pVal, ret);
		if (ret)
			pr_notice("parse fail (%s, %s, %d)\n", pKey, pVal, ret);
	}

	for (i = 0; i < NUM_CFG_FIELDS; i++) {
		const struct parse_data *field = &cfg_fields[i];

		pa = field->writer(field);
		if (pa) {
			pr_debug("#%d(%s)=>%s\n", i, field->name, pa);
			osal_free(pa);
		} else
			pr_err("failed to parse '%s'.\n", field->name);
	}
	osal_free(pBuf);
	return 0;
}

static int platform_request_firmware(char *patch_name, osal_firmware **ppPatch)
{
	int ret;
	osal_firmware *fw = NULL;

	if (!ppPatch) {
		pr_err("invalid ppBufptr!\n");
		return -1;
	}

	*ppPatch = NULL;
	do {
		ret = request_firmware((const struct firmware **)&fw,
							patch_name, NULL);
		if (ret == -EAGAIN) {
			pr_err("failed to open or read!(%s), retry again!\n",
						patch_name);
			osal_sleep_ms(100);
		}
	} while (ret == -EAGAIN);
	if (ret != 0) {
		pr_err("failed to open or read!(%s)\n", patch_name);
		release_firmware(fw);
		return -1;
	}
	pr_debug("loader firmware %s  ok!!\n", patch_name);
	ret = 0;
	*ppPatch = fw;

	return ret;
}

static int platform_release_firmware(osal_firmware **ppPatch)
{
	if (*ppPatch != NULL) {
		release_firmware((const struct firmware *)*ppPatch);
		*ppPatch = NULL;
	}
	return 0;
}

static int conf_exp_list_init(void)
{
	int index = 0, length;
	int i, j, k;
	char input;

	if (g_conninfra_conf.cfg_exist == 0) {
		pr_info("[%s] return case -1\n", __func__);
		return 0;
	}
	if (g_conninfra_conf.exp_filter == NULL) {
		pr_info("[%s] return case -2\n", __func__);
		return 0;
	}

	pr_info("[%s] legth=%d, str=%s\n", __func__,
		g_conninfra_conf.exp_filter->size, g_conninfra_conf.exp_filter->data);
	memset(conf_exp_filter_list, '\0', sizeof(char)*CONNINFRA_CONF_EXP_FILTER_MAX_LEN*CONNINFRA_CONF_EXP_FILTER_MAX_NUM);

	j = 0; // current substring position
	k = 0; // current character position within current substring

	length = g_conninfra_conf.exp_filter->size;
	for (i = 0; i < length; i++) {
		input = g_conninfra_conf.exp_filter->data[i];
		if (input == ',') {
			// move to the next substring
			j++;
			k = 0;

			// check if exceeding the limit of substrings
			if (j >= CONNINFRA_CONF_EXP_FILTER_MAX_NUM) {
				break;
			}
		} else {
			conf_exp_filter_list[j][k] = input;
			k++;
			if (k >= CONNINFRA_CONF_EXP_FILTER_MAX_LEN - 1) {
				break;
			}
		}
	}
	index = j + 1;

	for (i = 0; i < index; i++) {
		pr_info("[%s][%d] %s\n", __func__, i, conf_exp_filter_list[i]);
	}

	return index;
}

int conninfra_conf_exp_filter_check(const char *input)
{
	static int s_list_size = -1;
	int i;
	char *pStr;

	if (s_list_size == -1) {
		s_list_size = conf_exp_list_init();
		pr_info("[%s] s_list_size=%d\n", __func__, s_list_size);
	}

	for (i = 0; i < s_list_size; i++) {
		pStr = strstr(input, conf_exp_filter_list[i]);
		if (pStr != NULL) {
			pr_info("[%s] match item %s at index %d\n", __func__, conf_exp_filter_list[i], i);
			return i;
		}
	}

	return -CONNINFRA_CONF_EXP_PATTERN_NOT_FOUND;
}

int conninfra_conf_init(void)
{
	int ret = -1;
	const osal_firmware *conf_inst;

	osal_memset(&g_conninfra_conf, 0, osal_sizeof(struct conninfra_conf));
	osal_strcpy(&(g_conninfra_conf.conf_name[0]), "conninfra.cfg");

	pr_debug("config file:%s\n", &(g_conninfra_conf.conf_name[0]));
#ifdef CONFIG_FPGA_EARLY_PORTING
	pr_info("For FPGA, skip %s\n", __func__);
	g_conninfra_conf.cfg_exist = 0;
	return 0;
#endif
	if (0 ==
	    platform_request_firmware(&g_conninfra_conf.conf_name[0],
					(osal_firmware **) &conf_inst)) {
		/*get full name patch success */
		pr_debug("get full file name(%s) buf(0x%p) size(%zu)\n",
			      &g_conninfra_conf.conf_name[0], conf_inst->data,
			      conf_inst->size);
		if (0 ==
		    conf_parse((const char *)conf_inst->data,
				   conf_inst->size)) {
			/*config file exists */
			g_conninfra_conf.cfg_exist = 1;
			ret = 0;
		} else {
			pr_err("conf parsing fail\n");
			ret = -1;
		}
		platform_release_firmware((osal_firmware **) &conf_inst);
		return ret;
	}
	pr_err("read %s file fails\n", &(g_conninfra_conf.conf_name[0]));
	g_conninfra_conf.cfg_exist = 0;
	return ret;
}

int conninfra_conf_set_cfg_file(const char *name)
{
	if (name == NULL) {
		pr_err("name is NULL\n");
		return -1;
	}
	if (osal_strlen(name) >= osal_sizeof(g_conninfra_conf.conf_name)) {
		pr_err("name is too long, length=%d, expect to < %zu\n",
				osal_strlen(name),
				osal_sizeof(g_conninfra_conf.conf_name));
		return -2;
	}
	osal_memset(&g_conninfra_conf.conf_name[0], 0,
				osal_sizeof(g_conninfra_conf.conf_name));
	osal_strcpy(&(g_conninfra_conf.conf_name[0]), name);
	pr_err("WMT config file is set to (%s)\n",
				&(g_conninfra_conf.conf_name[0]));

	return 0;
}

const struct conninfra_conf *conninfra_conf_get_cfg(void)
{
	if (g_conninfra_conf.cfg_exist == 0)
		return NULL;

	return &g_conninfra_conf;
}

int conninfra_conf_deinit(void)
{
	int i;

	if (g_conninfra_conf.cfg_exist == 0)
		return -1;

	for (i = 0; i < NUM_CFG_FIELDS; i++) {
		const struct parse_data *field = &cfg_fields[i];
		field->relase_mem(field);
	}
	g_conninfra_conf.cfg_exist = 0;
	return 0;
}

