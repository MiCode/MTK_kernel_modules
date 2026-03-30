// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <asm/page.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/mm.h>
/* #include <asm/uaccess.h> */
#include <linux/uaccess.h>
#include <linux/hrtimer.h>

#include "met_drv.h"
#include "trace.h"

#include "core_plf_init.h"
#include "core_plf_trace.h"

/*define if the thermal sensor driver use its own timer to sampling the code , otherwise undefine it */
/*define it is better for sampling jitter if thermal sensor driver supports */
/* this define is phase out */
/* #define MET_USE_THERMALDRIVER_TIMER */


static unsigned int CheckAvailableThermalSensor(unsigned int a_u4DoCheck)
{
	static unsigned int u4AvailableSensor;

	unsigned int u4Index;

	if (!a_u4DoCheck)
		return u4AvailableSensor;

	/*Do check */
	if (MTK_THERMAL_SENSOR_COUNT > 32)
		return 0;

	if (mtk_thermal_get_temp_symbol == NULL)
		return 0;

	u4AvailableSensor = 0;

	for (u4Index = 0; u4Index < MTK_THERMAL_SENSOR_COUNT; u4Index++) {
		if (mtk_thermal_get_temp_symbol(u4Index) == (-127000))
			u4AvailableSensor &= (~(1 << u4Index));
		else
			u4AvailableSensor |= (1 << u4Index);
	}

	return u4AvailableSensor;
}

static int do_thermal(void)
{
	static int do_thermal = -1;

	if (do_thermal != -1)
		return do_thermal;

	if (met_thermal.mode == 0)
		do_thermal = 0;
	else
		do_thermal = met_thermal.mode;
	return do_thermal;
}

static unsigned int get_thermal(unsigned int *value)
{
	int j = -1;
	int i;
	unsigned int u4ValidSensors = 0;

	/*Do check */
	if (mtk_thermal_get_temp_symbol == NULL)
		return 0;

	u4ValidSensors = CheckAvailableThermalSensor(0);

	for (i = 0; i < MTK_THERMAL_SENSOR_COUNT; i++) {
		if (u4ValidSensors & (1 << i))
			value[++j] = mtk_thermal_get_temp_symbol(i);
	}

	return j + 1;
}

static void wq_get_thermal(struct work_struct *work)
{
	unsigned char count = 0;
	unsigned int thermal_value[MTK_THERMAL_SENSOR_COUNT] = {0};

	int cpu;

	cpu = smp_processor_id();
	if (do_thermal()) {
		count = get_thermal(thermal_value);
		if (count)
			ms_th(count, thermal_value);
	}
}

#ifdef MET_USE_THERMALDRIVER_TIMER

static void thermal_start(void)
{
	CheckAvailableThermalSensor(1);
	/* get extern symbol by symbol_get */
	if (mt_thermalsampler_registerCB_symbol)
		mt_thermalsampler_registerCB_symbol(wq_get_thermal);
}

static void thermal_stop(void)
{
	if (mt_thermalsampler_registerCB_symbol)
		mt_thermalsampler_registerCB_symbol(NULL);
}

#else

struct delayed_work dwork;
static void thermal_start(void)
{
	CheckAvailableThermalSensor(1);
	/*pr_debug("Thermal Sample:0x%x\n",CheckAvailableThermalSensor(0)); */
	INIT_DELAYED_WORK(&dwork, wq_get_thermal);
}

static void thermal_stop(void)
{
	cancel_delayed_work_sync(&dwork);
}

static void thermal_polling(unsigned long long stamp, int cpu)
{
	schedule_delayed_work(&dwork, 0);
}

#endif

static const char help[] = "  --thermal                             monitor thermal\n";
static int thermal_print_help(char *buf, int len)
{
	return snprintf(buf, PAGE_SIZE, help);
}

static const char g_pThermalHeader[] = "met-info [000] 0.0: thermal_header: ms_th";

static int thermal_print_header(char *buf, int len)
{
	int ret;
	char buffer[256] = {0};
	char ts_buf[8] = {0};
	unsigned int u4ValidSensor = 0;
	int i = 0, ts_sz = 0;
	unsigned int str_len = 0;

	u4ValidSensor = CheckAvailableThermalSensor(0);

	str_len = strlen(g_pThermalHeader);
	if (str_len < 256) {
		strncpy(buffer, g_pThermalHeader, str_len + 1);
		buffer[str_len] = '\0';
	}

	ts_sz = sizeof(ts_buf);

	for ( i = 0 ; i < MTK_THERMAL_SENSOR_COUNT ; i++) {
		if ((1 << i) & u4ValidSensor) {
			ret = snprintf(ts_buf, ts_sz, ",tab_%d", (i + 1));
			if (ret > 0)
				strncat(buffer, ts_buf, 256 - 1);
		}
		memset(ts_buf, '\0', ts_sz);
	}

	strncat(buffer, "\n", 256 - 1);

	return snprintf(buf, PAGE_SIZE, "%s", buffer);
}

struct metdevice met_thermal = {
	.name = "thermal",
	.owner = THIS_MODULE,
	.type = MET_TYPE_BUS,
	.cpu_related = 0,
	.ondiemet_mode = 0,
	.start = thermal_start,
	.stop = thermal_stop,
#ifdef MET_USE_THERMALDRIVER_TIMER
#else
	.polling_interval = 50,	/* ms */
	.timed_polling = thermal_polling,
	.tagged_polling = thermal_polling,
#endif
	.print_help = thermal_print_help,
	.print_header = thermal_print_header,
};



/*CPU sensors */
static unsigned int CheckAvailableCPUThermalSensor(unsigned int a_u4DoCheck)
{
	static unsigned int u4AvailableSensor;

	unsigned int u4Index;

	if (tscpu_get_cpu_temp_met_symbol == NULL)
		return 0;

	if (!a_u4DoCheck)
		return u4AvailableSensor;

	u4AvailableSensor = 0;

	for (u4Index = 0; u4Index < MTK_THERMAL_SENSOR_CPU_COUNT; u4Index++) {
		if (tscpu_get_cpu_temp_met_symbol(u4Index) == (-127000))
			u4AvailableSensor &= (~(1 << u4Index));
		else
			u4AvailableSensor |= (1 << u4Index);
	}

	return u4AvailableSensor;
}

noinline void CPUTS(void)
{
	unsigned int u4Index1 = 0, u4Index2 = 0;
	unsigned int u4ValidSensors = 0;
	int i4TSValue[MTK_THERMAL_SENSOR_CPU_COUNT];
	int i = 0, len = 0, total_len = 0, sz;
	char str_buff[128] = {0};

	sz = sizeof(str_buff);

	if (tscpu_get_cpu_temp_met_symbol == NULL)
		return;

	memset(i4TSValue, 0, sizeof(int) * MTK_THERMAL_SENSOR_CPU_COUNT);

	u4ValidSensors = CheckAvailableCPUThermalSensor(0);

	for (; u4Index1 < MTK_THERMAL_SENSOR_CPU_COUNT; u4Index1++) {
		if (u4ValidSensors & (1 << u4Index1)) {
			i4TSValue[u4Index2] = tscpu_get_cpu_temp_met_symbol(u4Index1);
			u4Index2 += 1;
		}
	}

	len = snprintf(str_buff + total_len, 8, "%d", i4TSValue[0]);
	if (len >= 0 && len < sz)
		total_len += len;

	for (i = 1 ; i < u4Index2 ; i++) {
		len = snprintf(str_buff + total_len, 8, ",%d", i4TSValue[i]);
		if (len >= 0 && len < sz)
			total_len += len;
	}

	MET_TRACE("%s\n", str_buff);
}

static void thermal_CPU_start(void)
{
	CheckAvailableCPUThermalSensor(1);
	CPUTS();
	/* get extern symbol by symbol_get */
	if (mt_thermalsampler_registerCB_symbol)
		mt_thermalsampler_registerCB_symbol(CPUTS);
}

static void thermal_CPU_stop(void)
{
	/* release extern symbol by symbol_put */
	if (mt_thermalsampler_registerCB_symbol)
		mt_thermalsampler_registerCB_symbol(NULL);

	CPUTS();
}

static const char help_cpu[] = "  --thermal-cpu                         monitor cpu temperature\n";
static int thermal_CPU_print_help(char *buf, int len)
{
	return snprintf(buf, PAGE_SIZE, help_cpu);
}

static const char g_pCPUThermalHeader[] = "met-info [000] 0.0: thermal_cpu_header: CPUTS";

static int thermal_CPU_print_header(char *buf, int len)
{
	int ret;
	char buffer[256] = {0};
	char ts_buf[8] = {0} ;
	unsigned int u4ValidSensor = 0;
	int i = 0, ts_sz = 0;
	unsigned int str_len = 0;

	u4ValidSensor = CheckAvailableCPUThermalSensor(0);

	str_len = strlen(g_pCPUThermalHeader);
	if (str_len < 256) {
		strncpy(buffer, g_pCPUThermalHeader, str_len + 1);
		buffer[str_len] = '\0';
	}

	ts_sz = sizeof(ts_buf);

	for ( i = 0 ; i < MTK_THERMAL_SENSOR_CPU_COUNT ; i++) {
		if ((1 << i) & u4ValidSensor) {
			ret = snprintf(ts_buf, ts_sz, ",tab_%d", (i + 1));
			if (ret > 0)
				strncat(buffer, ts_buf, 256 - 1);
		}
		memset(ts_buf, '\0', ts_sz);
	}

	strncat(buffer, "\n", 256 - 1);

	return snprintf(buf, PAGE_SIZE, "%s", buffer);
}

struct metdevice met_thermal_cpu = {
	.name = "thermal-cpu",
	.owner = THIS_MODULE,
	.type = MET_TYPE_BUS,
	.cpu_related = 0,
	.start = thermal_CPU_start,
	.stop = thermal_CPU_stop,
	.print_help = thermal_CPU_print_help,
	.print_header = thermal_CPU_print_header,
	.ondiemet_mode = 0,
};
