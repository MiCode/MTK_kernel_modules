// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#define pr_fmt(fmt) KBUILD_MODNAME "@(%s:%d) " fmt, __func__, __LINE__

#include "../include/consys_hw.h"
#include "../include/connsys_library.h"
#include "../include/consys_reg_mng.h"
#include "../include/consys_reg_util.h"
#include "../include/plat_def.h"
#include "../include/plat_library.h"

#include "include/mt6878.h"
#include "include/mt6878_consys_reg.h"
#include "include/mt6878_consys_reg_offset.h"
#include "include/mt6878_pos.h"

/*******************************************************************************
 *                             D A T A   T Y P E S
 ********************************************************************************
 */

struct rf_cr_backup_data {
	unsigned int addr;
	unsigned int value1;
	unsigned int value2;
};

static struct consys_plat_thermal_data_mt6878 g_consys_plat_therm_data;

/* For calibration backup/restore */
#define MAX_CALIBRATION_DATA_BACKUP_SIZE	256
static struct rf_cr_backup_data mt6637_backup_data[MAX_CALIBRATION_DATA_BACKUP_SIZE];
static unsigned int mt6637_backup_cr_number;
extern phys_addr_t g_con_emi_phy_base;
unsigned long mt6878_power_state_dump_data[POWER_STATE_DUMP_DATA_SIZE];
u32 g_connsys_adie_sku = 0x0;

int consys_co_clock_type_mt6878(void)
{
	return conn_hw_env.clock_type;
}

int consys_enable_power_dump_mt6878(void)
{
	/* Return success because sleep count dump is enable on POS */
	return 0;
}

static int consys_reset_power_state(void)
{
	/* Clear data and disable stop */
	/* I. Clear
	 * i.	Conn_infra:	0x1806_0384[15]
	 * ii.	Wf:		0x1806_0384[9]
	 * iii.	Bt:		0x1806_0384[10]
	 * iv.	Gps:		0x1806_0384[8]
	 */
	CONSYS_REG_WRITE_HW_ENTRY(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_CONN_INFRA_SLEEP_CNT_CLR,
		0x1);
	CONSYS_REG_WRITE_HW_ENTRY(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_WF_SLEEP_CNT_CLR,
		0x1);
	CONSYS_REG_WRITE_HW_ENTRY(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_BT_SLEEP_CNT_CLR,
		0x1);
	CONSYS_REG_WRITE_HW_ENTRY(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_GPS_SLEEP_CNT_CLR,
		0x1);

	udelay(150);

	CONSYS_REG_WRITE_HW_ENTRY(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_CONN_INFRA_SLEEP_CNT_CLR,
		0x0);
	CONSYS_REG_WRITE_HW_ENTRY(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_WF_SLEEP_CNT_CLR,
		0x0);
	CONSYS_REG_WRITE_HW_ENTRY(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_BT_SLEEP_CNT_CLR,
		0x0);
	CONSYS_REG_WRITE_HW_ENTRY(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_GPS_SLEEP_CNT_CLR,
		0x0);

	/* II. Stop
	 * i.	Conn_infra:	0x1806_0384[7]
	 * ii.	Wf:		0x1806_0384[1]
	 * iii.	Bt:		0x1806_0384[2]
	 * iv.	Gps:		0x1806_0384[0]
	 */
	CONSYS_REG_WRITE_HW_ENTRY(
	    CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_CONN_INFRA_SLEEP_CNT_STOP,
	    0x0);
	CONSYS_REG_WRITE_HW_ENTRY(
	    CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_WF_SLEEP_CNT_STOP,
	    0x0);
	CONSYS_REG_WRITE_HW_ENTRY(
	    CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_BT_SLEEP_CNT_STOP,
	    0x0);
	CONSYS_REG_WRITE_HW_ENTRY(
	    CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_GPS_SLEEP_CNT_STOP,
	    0x0);
	return 0;
}

static inline void __sleep_count_trigger_read(void)
{
	CONSYS_REG_WRITE_HW_ENTRY(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_CTL_HOST_SLP_COUNTER_RD_TRIGGER, 0x1);
	udelay(150);
	CONSYS_REG_WRITE_HW_ENTRY(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_CTL_HOST_SLP_COUNTER_RD_TRIGGER, 0x0);

}

static void consys_print_irq_status(void)
{
	unsigned int val_1, val_2, val_3, val_4;

	val_1 = CONSYS_REG_READ_BIT(CONN_REG_CONN_HOST_CSR_TOP_ADDR + 0x38, 0x1);
	val_2 = CONSYS_REG_READ_BIT(CONN_REG_CONN_HOST_CSR_TOP_ADDR + 0x34, 0x1);
	val_3 = CONSYS_REG_READ_BIT(CONN_REG_CONN_HOST_CSR_TOP_ADDR + 0x3C, 0x2);
	val_4 = CONSYS_REG_READ_BIT(CONN_REG_CONN_HOST_CSR_TOP_ADDR + 0x44, 0x1);

	/*
	 * conn_bgf_hif_on_host_int_b
	 * (~(0x1806_0038[0] & 0x1806_0034[0])) & (~(0x1806_0038[1] & 0x1806_003C[0]))
	 */
	if ((val_1 && val_2) || (val_1 && val_3))
		pr_info("conn_bgf_hif_on_host_int_b %x %x %x", val_1, val_2, val_3);

	/*
	 * conn_gps_hif_on_host_int_b
	 * ~ (0x1806_0038[0] & 0x1806_0044[0])
	 */
	if (val_1 && val_4)
		pr_info("conn_gps_hif_on_host_int_b %x %x", val_1, val_4);


	if (consys_check_conninfra_on_domain_status_mt6878() != 0)
		return;
	/*
	 * ccif_wf2ap_sw_irq_b	0x1803_C008[7:0]
	 * ccif_bgf2ap_sw_irq_b	0x1803_E008[7:0]
	 */
	if (CONN_REG_CCIF_WF2AP_SWIRQ_ADDR) {
		val_1 = CONSYS_REG_READ(CONN_REG_CCIF_WF2AP_SWIRQ_ADDR) & 0xFF;
		if (val_1 > 0)
			pr_info("ccif_wf2ap_sw_irq_b %x", val_1);
	}

	if (CONN_REG_CCIF_BGF2AP_SWIRQ_ADDR) {
		val_1 = CONSYS_REG_READ(CONN_REG_CCIF_BGF2AP_SWIRQ_ADDR) & 0xFF;
		if (val_1 > 0)
			pr_info("ccif_bgf2ap_sw_irq_b %x", val_1);
	}
}

#define POWER_STATE_BUFF_LENGTH	256
static void consys_power_state(void)
{
	unsigned int i, str_len;
	unsigned int buf_len = 0;
	unsigned int r;
	static const char *const osc_str[] = {
	"fm ", "gps ", "bgf ", "wf ", "conn_infra_bus ", " ", "ap2conn ", " ",
	" ", " ", " ", "conn_pta ", "conn_spi ", " ", "conn_thm "};
	char buf[POWER_STATE_BUFF_LENGTH] = {'\0'};

	CONSYS_REG_WRITE_HW_ENTRY(
		CONN_HOST_CSR_TOP_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL,
		0x0);
	r = CONSYS_REG_READ(CONN_HOST_CSR_TOP_CONN_INFRA_CFG_ON_DBG_ADDR);

	for (i = 0; i < 15; i++) {
		str_len = strlen(osc_str[i]);

		if ((r & (0x1 << (1 + i))) > 0 && (buf_len + str_len < POWER_STATE_BUFF_LENGTH)) {
			strnlcat(buf, osc_str[i], str_len, POWER_STATE_BUFF_LENGTH);
			buf_len += str_len;
		}
	}
	pr_info("[%s] [0x%x] %s", __func__, r, buf);

	consys_print_irq_status();

}

static int consys_power_state_dump(char *buf, unsigned int size, int print_log)
{
#define POWER_STATE_BUF_SIZE 256
#define CONN_32K_TICKS_PER_SEC (32768)
#define CONN_TICK_TO_SEC(TICK) (TICK / CONN_32K_TICKS_PER_SEC)
	static u64 round;
	static u64 t_conninfra_sleep_cnt, t_conninfra_sleep_time;
	static u64 t_wf_sleep_cnt, t_wf_sleep_time;
	static u64 t_bt_sleep_cnt, t_bt_sleep_time;
	static u64 t_gps_sleep_cnt, t_gps_sleep_time;
	unsigned int conninfra_sleep_cnt, conninfra_sleep_time;
	unsigned int wf_sleep_cnt, wf_sleep_time;
	unsigned int bt_sleep_cnt, bt_sleep_time;
	unsigned int gps_sleep_cnt, gps_sleep_time;
	char temp_buf[POWER_STATE_BUF_SIZE];
	char *buf_p = temp_buf;
	int buf_sz = POWER_STATE_BUF_SIZE;
	int ret = 0;

	/* Sleep count */
	/* 1. Setup read select: 0x1806_0380[3:1]
	 *	3'h0: conn_infra sleep counter
	 *	3'h1: wfsys sleep counter
	 *	3'h2: bgfsys sleep counter
	 *	3'h3: gpssys sleep counter
	 * 2. Dump time and count
	 *	a. Timer: 0x1806_0388
	 *	b. Count: 0x1806_038C
	 */
	CONSYS_REG_WRITE_HW_ENTRY(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_CTL_HOST_SLP_COUNTER_SEL,
		0x0);
	__sleep_count_trigger_read();
	conninfra_sleep_time = CONSYS_REG_READ(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_TIMER_ADDR);
	conninfra_sleep_cnt = CONSYS_REG_READ(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_COUNTER_ADDR);

	t_conninfra_sleep_time += conninfra_sleep_time;
	t_conninfra_sleep_cnt += conninfra_sleep_cnt;
	/* Wait 60 us to make sure the duration to next write to SLP_COUNTER_RD_TRIGGER is
	 * long enough.
	 */
	udelay(60);

	CONSYS_REG_WRITE_HW_ENTRY(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_CTL_HOST_SLP_COUNTER_SEL,
		0x1);
	__sleep_count_trigger_read();
	wf_sleep_time = CONSYS_REG_READ(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_TIMER_ADDR);
	wf_sleep_cnt = CONSYS_REG_READ(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_COUNTER_ADDR);

	t_wf_sleep_time += wf_sleep_time;
	t_wf_sleep_cnt += wf_sleep_cnt;
	udelay(60);

	CONSYS_REG_WRITE_HW_ENTRY(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_CTL_HOST_SLP_COUNTER_SEL,
		0x2);
	__sleep_count_trigger_read();
	bt_sleep_time = CONSYS_REG_READ(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_TIMER_ADDR);
	bt_sleep_cnt = CONSYS_REG_READ(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_COUNTER_ADDR);

	t_bt_sleep_time += bt_sleep_time;
	t_bt_sleep_cnt += bt_sleep_cnt;
	udelay(60);

	CONSYS_REG_WRITE_HW_ENTRY(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_CTL_HOST_SLP_COUNTER_SEL,
		0x3);
	__sleep_count_trigger_read();
	gps_sleep_time = CONSYS_REG_READ(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_TIMER_ADDR);
	gps_sleep_cnt = CONSYS_REG_READ(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_COUNTER_ADDR);
	t_gps_sleep_time += gps_sleep_time;
	t_gps_sleep_cnt += gps_sleep_cnt;

	if (print_log) {
		mt6878_power_state_dump_data[0] = round;
		mt6878_power_state_dump_data[1] = CONN_TICK_TO_SEC(conninfra_sleep_time);
		mt6878_power_state_dump_data[2] = CONN_TICK_TO_SEC((conninfra_sleep_time
							    % CONN_32K_TICKS_PER_SEC) * 1000);
		mt6878_power_state_dump_data[3] = conninfra_sleep_cnt;
		mt6878_power_state_dump_data[4] = CONN_TICK_TO_SEC(wf_sleep_time);
		mt6878_power_state_dump_data[5] = CONN_TICK_TO_SEC((wf_sleep_time
							    % CONN_32K_TICKS_PER_SEC) * 1000);
		mt6878_power_state_dump_data[6] = wf_sleep_cnt;
		mt6878_power_state_dump_data[7] = CONN_TICK_TO_SEC(bt_sleep_time);
		mt6878_power_state_dump_data[8] = CONN_TICK_TO_SEC((bt_sleep_time
							    % CONN_32K_TICKS_PER_SEC) * 1000);
		mt6878_power_state_dump_data[9] = bt_sleep_cnt;
		mt6878_power_state_dump_data[10] = CONN_TICK_TO_SEC(gps_sleep_time);
		mt6878_power_state_dump_data[11] = CONN_TICK_TO_SEC((gps_sleep_time
							     % CONN_32K_TICKS_PER_SEC) * 1000);
		mt6878_power_state_dump_data[12] = gps_sleep_cnt;
		mt6878_power_state_dump_data[13] = CONN_TICK_TO_SEC(t_conninfra_sleep_time);
		mt6878_power_state_dump_data[14] = CONN_TICK_TO_SEC((t_conninfra_sleep_time
							     % CONN_32K_TICKS_PER_SEC) * 1000);
		mt6878_power_state_dump_data[15] = t_conninfra_sleep_cnt;
		mt6878_power_state_dump_data[16] = CONN_TICK_TO_SEC(t_wf_sleep_time);
		mt6878_power_state_dump_data[17] = CONN_TICK_TO_SEC((t_wf_sleep_time
							     % CONN_32K_TICKS_PER_SEC) * 1000);
		mt6878_power_state_dump_data[18] = t_wf_sleep_cnt;
		mt6878_power_state_dump_data[19] = CONN_TICK_TO_SEC(t_bt_sleep_time);
		mt6878_power_state_dump_data[20] = CONN_TICK_TO_SEC((t_bt_sleep_time
							     % CONN_32K_TICKS_PER_SEC) * 1000);
		mt6878_power_state_dump_data[21] = t_bt_sleep_cnt;
		mt6878_power_state_dump_data[22] = CONN_TICK_TO_SEC(t_gps_sleep_time);
		mt6878_power_state_dump_data[23] = CONN_TICK_TO_SEC((t_gps_sleep_time
							     % CONN_32K_TICKS_PER_SEC) * 1000);
		mt6878_power_state_dump_data[24] = t_gps_sleep_cnt;

		if (buf != NULL && size > 0) {
			buf_p = buf;
			buf_sz = size;
		}

		ret = snprintf(buf_p, buf_sz,
			"[consys_power_state][round:%lu]conninfra:%lu.%03lu,%lu;wf:%lu.%03lu,%lu;bt:%lu.%03lu,%lu;gps:%lu.%03lu,%lu;[total]conninfra:%lu.%03lu,%lu;wf:%lu.%03lu,%lu;bt:%lu.%03lu,%lu;gps:%lu.%03lu,%lu;",
			mt6878_power_state_dump_data[0],
			mt6878_power_state_dump_data[1],
			mt6878_power_state_dump_data[2],
			mt6878_power_state_dump_data[3],
			mt6878_power_state_dump_data[4],
			mt6878_power_state_dump_data[5],
			mt6878_power_state_dump_data[6],
			mt6878_power_state_dump_data[7],
			mt6878_power_state_dump_data[8],
			mt6878_power_state_dump_data[9],
			mt6878_power_state_dump_data[10],
			mt6878_power_state_dump_data[11],
			mt6878_power_state_dump_data[12],
			mt6878_power_state_dump_data[13],
			mt6878_power_state_dump_data[14],
			mt6878_power_state_dump_data[15],
			mt6878_power_state_dump_data[16],
			mt6878_power_state_dump_data[17],
			mt6878_power_state_dump_data[18],
			mt6878_power_state_dump_data[19],
			mt6878_power_state_dump_data[20],
			mt6878_power_state_dump_data[21],
			mt6878_power_state_dump_data[22],
			mt6878_power_state_dump_data[23],
			mt6878_power_state_dump_data[24]);
		if (ret > 0)
			pr_info("%s", buf_p);

		/* Power state */
		consys_power_state();
	}

	round++;

	/* reset after sleep time is accumulated. */
	consys_reset_power_state();
	return 0;
}

int consys_reset_power_state_mt6878(void)
{
	return consys_power_state_dump(NULL, 0, 0);
}

int consys_power_state_dump_mt6878(char *buf, unsigned int size)
{
	return consys_power_state_dump(buf, size, 1);
}

unsigned int consys_get_hw_ver_mt6878(void)
{
	return CONN_HW_VER;
}

void update_thermal_data_mt6878(struct consys_plat_thermal_data_mt6878 *input)
{
	memcpy(&g_consys_plat_therm_data, input, sizeof(struct consys_plat_thermal_data_mt6878));
}

#define PRINT_THERMAL_LOG 1
#define MT6631_FAB_ID_UMC 0x1
static int calculate_adie6637_thermal_temperature(int y)
{
	struct consys_plat_thermal_data_mt6878 *data = &g_consys_plat_therm_data;
	int t;
	int const_offset = 30;

	/*  temperature = (y-b)*slope + (offset) */
	/* Postpone division to avoid getting wrong slope because of integer division */
	t = (y - (data->thermal_b == 0 ? 0x38 : data->thermal_b)) *
			(data->slop_molecule + 1866) / 1000 + const_offset;

#if PRINT_THERMAL_LOG
	pr_info("y=[%d] b=[%d] constOffset=[%d] [%d] [%d] => t=[%d]\n",
			y, data->thermal_b, const_offset, data->slop_molecule, data->offset,
			t);
#endif
	return t;
}

static int calculate_adie6631_thermal_temperature(int y)
{
	struct consys_plat_thermal_data_mt6878 *data = &g_consys_plat_therm_data;
	int t;
	int const_offset = 25;

	/* temperature = (y-b)*slope + (offset)
	 * Read FAB_ID A die 0x120[12:11] for different THADC slope and offset, TSMC
	 * modify efuse value (thermal_b - 4) by FT, only for 6631
	 * const_offset in TSMC = 25, UMC = 23
	 */
	unsigned int fab_id;

	fab_id = CONSYS_REG_READ(CONN_INFRA_SYSRAM_SW_CR_A_DIE_EFUSE_DATA_1);
	fab_id = (fab_id & 0x1800) >> 11;
	if (fab_id == MT6631_FAB_ID_UMC) {
		t = ((y - (data->thermal_b == 0 ? 0x32 : data->thermal_b - 4)) *
				2476 / 1000 + const_offset - 2);
	} else {
		t = ((y - (data->thermal_b == 0 ? 0x32 : data->thermal_b - 4)) *
				138 / 59 + const_offset);
	}

#if PRINT_THERMAL_LOG
	pr_info("y=[%d] b=[%d] constOffset=[%d] [%d] [%d] => t=[%d]\n",
			y, data->thermal_b, const_offset, data->slop_molecule, data->offset,
			t);
#endif
	return t;
}

int consys_thermal_query_adie6637_mt6878(void)
{
#define THERMAL_DUMP_NUM	11
#define LOG_TMP_BUF_SZ		256
#define TEMP_SIZE		13
#define CONN_GPT2_CTRL_BASE	0x1801F000
#define CONN_GPT2_CTRL_AP_EN	0x38

	mapped_addr addr = 0;
	int cal_val, res = 0;
	/* Base: 0x1800_2000, CONN_TOP_THERM_CTL */
	const unsigned int thermal_dump_crs[THERMAL_DUMP_NUM] = {
		0x00, 0x04, 0x08, 0x0c,
		0x10, 0x14, 0x18, 0x1c,
		0x20, 0x24, 0x28,
	};
	char tmp[TEMP_SIZE] = {'\0'};
	char tmp_buf[LOG_TMP_BUF_SZ] = {'\0'};
	unsigned int i;
	unsigned int efuse0, efuse1, efuse2, efuse3;

	addr = ioremap(CONN_GPT2_CTRL_BASE, 0x100);
	if (addr == 0) {
		pr_err("GPT2_CTRL_BASE remap fail");
		return -1;
	}

	connsys_adie_top_ck_en_ctl_mt6878(true);

	/* Hold Semaphore, TODO: may not need this, because
	 * thermal cr separate for different
	 */
	if (consys_sema_acquire_timeout_mt6878(CONN_SEMA_THERMAL_INDEX, CONN_SEMA_TIMEOUT)
		== CONN_SEMA_GET_FAIL) {
		pr_err("[THERM QRY] Require semaphore fail\n");
		connsys_adie_top_ck_en_ctl_mt6878(false);
		iounmap(addr);
		return -1;
	}

	/* therm cal en */
	CONSYS_SET_BIT(CONN_THERM_CTL_THERMEN3_ADDR, (0x1 << 19));
	/* GPT2 En */
	CONSYS_REG_WRITE(addr + CONN_GPT2_CTRL_AP_EN, 0x1);

	/* thermal trigger */
	CONSYS_SET_BIT(CONN_THERM_CTL_THERMEN3_ADDR, (0x1 << 18));
	udelay(500);
	/* get thermal value */
	cal_val = CONSYS_REG_READ(CONN_THERM_CTL_THERMEN3_ADDR);
	cal_val = (cal_val >> 8) & 0x7f;

	/* thermal debug dump */
	efuse0 = CONSYS_REG_READ(CONN_INFRA_SYSRAM_SW_CR_A_DIE_EFUSE_DATA_0);
	efuse1 = CONSYS_REG_READ(CONN_INFRA_SYSRAM_SW_CR_A_DIE_EFUSE_DATA_1);
	efuse2 = CONSYS_REG_READ(CONN_INFRA_SYSRAM_SW_CR_A_DIE_EFUSE_DATA_2);
	efuse3 = CONSYS_REG_READ(CONN_INFRA_SYSRAM_SW_CR_A_DIE_EFUSE_DATA_3);
	for (i = 0; i < THERMAL_DUMP_NUM; i++) {
		if (snprintf(
			tmp, TEMP_SIZE, "[0x%08x]",
			CONSYS_REG_READ(CONN_REG_CONN_THERM_CTL_ADDR + thermal_dump_crs[i])) >= 0)
			strnlcat(tmp_buf, tmp, strlen(tmp), LOG_TMP_BUF_SZ);
	}
#if PRINT_THERMAL_LOG
	pr_info("[%s] efuse:[0x%08x][0x%08x][0x%08x][0x%08x] thermal dump: %s",
		__func__, efuse0, efuse1, efuse2, efuse3, tmp_buf);
#endif
	res = calculate_adie6637_thermal_temperature(cal_val);

	/* GPT2 disable */
	CONSYS_REG_WRITE(addr + CONN_GPT2_CTRL_AP_EN, 0);

	/* disable */
	CONSYS_CLR_BIT(CONN_THERM_CTL_THERMEN3_ADDR, (0x1 << 19));

	consys_sema_release_mt6878(CONN_SEMA_THERMAL_INDEX);
	connsys_adie_top_ck_en_ctl_mt6878(false);

	iounmap(addr);

	return res;
}

int consys_thermal_query_adie6631_mt6878(void)
{
#define THERMAL_DUMP_NUM	11
#define LOG_TMP_BUF_SZ		256
#define TEMP_SIZE		13
#define CONN_GPT2_CTRL_BASE	0x1801F000
#define CONN_GPT2_CTRL_AP_EN	0x38
/* ref by mt_thermal.c and connv2_drv.c */
#define THERMAL_TEMP_INVALID	-275

	mapped_addr addr = 0;
	int cal_val, res = 0;
	unsigned int only_gps_on = (0x1 << CONNDRV_TYPE_GPS);
	/* Base: 0x1800_2000, CONN_TOP_THERM_CTL */
	const unsigned int thermal_dump_crs[THERMAL_DUMP_NUM] = {
		0x00, 0x04, 0x08, 0x0c,
		0x10, 0x14, 0x18, 0x1c,
		0x20, 0x24, 0x28,
	};
	char tmp[TEMP_SIZE] = {'\0'};
	char tmp_buf[LOG_TMP_BUF_SZ] = {'\0'};
	unsigned int i;
	unsigned int efuse0, efuse1, efuse2, efuse3;
	unsigned int efuse4, efuse5, efuse6, efuse7;

	addr = ioremap(CONN_GPT2_CTRL_BASE, 0x100);
	if (addr == 0) {
		pr_err("GPT2_CTRL_BASE remap fail");
		return -1;
	}

	/* In two adie sku, gps won't query 6631 temp */
	if ((CONSYS_REG_READ(CONN_INFRA_SYSRAM_SW_CR_RADIO_STATUS) == only_gps_on)
		&& (consys_get_adie_chipid_mt6878() == ADIE_6686))
		return THERMAL_TEMP_INVALID;

	connsys_adie_top_ck_en_ctl_mt6878(true);

	/* Hold Semaphore, TODO: may not need this, because
	 * thermal cr separate for different
	 */
	if (consys_sema_acquire_timeout_mt6878(CONN_SEMA_THERMAL_INDEX, CONN_SEMA_TIMEOUT)
		== CONN_SEMA_GET_FAIL) {
		pr_err("[THERM QRY] Require semaphore fail\n");
		connsys_adie_top_ck_en_ctl_mt6878(false);
		iounmap(addr);
		return -1;
	}

	/* therm cal en */
	CONSYS_SET_BIT(CONN_THERM_CTL_THERMEN3_ADDR, (0x1 << 19));
	/* GPT2 En */
	CONSYS_REG_WRITE(addr + CONN_GPT2_CTRL_AP_EN, 0x1);

	/* thermal trigger */
	CONSYS_SET_BIT(CONN_THERM_CTL_THERMEN3_ADDR, (0x1 << 18));
	udelay(500);
	/* get thermal value */
	cal_val = CONSYS_REG_READ(CONN_THERM_CTL_THERMEN3_ADDR);
	cal_val = (cal_val >> 8) & 0x7f;

	/* thermal debug dump */
	efuse0 = CONSYS_REG_READ(CONN_INFRA_SYSRAM_SW_CR_A_DIE_EFUSE_DATA_0);
	efuse1 = CONSYS_REG_READ(CONN_INFRA_SYSRAM_SW_CR_A_DIE_EFUSE_DATA_1);
	efuse2 = CONSYS_REG_READ(CONN_INFRA_SYSRAM_SW_CR_A_DIE_EFUSE_DATA_2);
	efuse3 = CONSYS_REG_READ(CONN_INFRA_SYSRAM_SW_CR_A_DIE_EFUSE_DATA_3);
	efuse4 = CONSYS_REG_READ(CONN_INFRA_SYSRAM_SW_CR_A_DIE_EFUSE_DATA_4);
	efuse5 = CONSYS_REG_READ(CONN_INFRA_SYSRAM_SW_CR_A_DIE_EFUSE_DATA_5);
	efuse6 = CONSYS_REG_READ(CONN_INFRA_SYSRAM_SW_CR_A_DIE_EFUSE_DATA_6);
	efuse7 = CONSYS_REG_READ(CONN_INFRA_SYSRAM_SW_CR_A_DIE_EFUSE_DATA_7);

	for (i = 0; i < THERMAL_DUMP_NUM; i++) {
		if (snprintf(
			tmp, TEMP_SIZE, "[0x%08x]",
			CONSYS_REG_READ(CONN_REG_CONN_THERM_CTL_ADDR + thermal_dump_crs[i])) >= 0)
			strnlcat(tmp_buf, tmp, strlen(tmp), LOG_TMP_BUF_SZ);
	}
#if PRINT_THERMAL_LOG
	pr_info("[%s] efuse:[0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x]\
			thermal dump: %s", __func__, efuse0, efuse1, efuse2, efuse3, efuse4, efuse5,\
			efuse6, efuse7, tmp_buf);
#endif
	res = calculate_adie6631_thermal_temperature(cal_val);

	/* GPT2 disable */
	CONSYS_REG_WRITE(addr + CONN_GPT2_CTRL_AP_EN, 0);

	/* disable */
	CONSYS_CLR_BIT(CONN_THERM_CTL_THERMEN3_ADDR, (0x1 << 19));

	consys_sema_release_mt6878(CONN_SEMA_THERMAL_INDEX);
	connsys_adie_top_ck_en_ctl_mt6878(false);

	iounmap(addr);

	return res;
}

unsigned int consys_adie_detection_adie6637_mt6878(unsigned int drv_type)
{
	g_connsys_adie_sku = ADIE_6637;
	return ADIE_6637;
}

unsigned int consys_adie_detection_adie6631_mt6878(unsigned int drv_type)
{
	g_connsys_adie_sku = ADIE_6631;
	return ADIE_6631;
}

unsigned int consys_adie_detection_adie6686_mt6878(unsigned int drv_type)
{
	g_connsys_adie_sku = ADIE_6686;
	if (drv_type == CONNDRV_TYPE_GPS)
		return ADIE_6686;
	else
		return ADIE_6631;
}

unsigned int consys_get_adie_chipid_mt6878(void)
{
	/* It's only different gps adie between each sku in mt6878 */
	return g_connsys_adie_sku;
}

void consys_set_mcu_control_mt6878(int type, bool onoff)
{
	pr_info("[%s] Set mcu control type=[%d] onoff=[%d]\n", __func__, type, onoff);

	if (onoff) // Turn on
		CONSYS_SET_BIT(CONN_INFRA_SYSRAM_SW_CR_MCU_LOG_CONTROL, (0x1 << type));
	else // Turn off
		CONSYS_CLR_BIT(CONN_INFRA_SYSRAM_SW_CR_MCU_LOG_CONTROL, (0x1 << type));
}

int consys_pre_cal_backup_mt6878(unsigned int offset, unsigned int size)
{
	mapped_addr vir_addr = 0;
	unsigned int expected_size = 0;

	pr_info("[%s] emi base=0x%lx offset=0x%x size=%d", __func__,
		g_con_emi_phy_base, offset, size);
	if ((size == 0) || ((offset & 0x3) != 0x0))
		return 1;

	/* Read CR number from EMI */
	vir_addr = ioremap(g_con_emi_phy_base + offset, 4);
	if (vir_addr == 0) {
		pr_err("[%s] ioremap CR number fail", __func__);
		return -ENOMEM;
	}
	mt6637_backup_cr_number = readl(vir_addr);
	if (mt6637_backup_cr_number > MAX_CALIBRATION_DATA_BACKUP_SIZE)
		pr_err("[%s] requested back size overflow=%d > %d\n", __func__,
		      mt6637_backup_cr_number, MAX_CALIBRATION_DATA_BACKUP_SIZE);

	iounmap(vir_addr);
	expected_size = sizeof(struct rf_cr_backup_data)*mt6637_backup_cr_number + 4;
	if (size < expected_size) {
		pr_err("[%s] cr number=%d, expected_size=0x%x size=0x%x", __func__,
			mt6637_backup_cr_number, expected_size, size);
		mt6637_backup_cr_number = 0;
		return 1;
	}

	vir_addr = ioremap(g_con_emi_phy_base + offset + 4,
			sizeof(struct rf_cr_backup_data)*mt6637_backup_cr_number);
	if (vir_addr == 0) {
		pr_err("[%s] ioremap data fail", __func__);
		return -ENOMEM;
	}
	memcpy_fromio(mt6637_backup_data, vir_addr,
		sizeof(struct rf_cr_backup_data)*mt6637_backup_cr_number);
	iounmap(vir_addr);

	return 0;
}

int consys_pre_cal_restore_mt6878(void)
{
	int i;

	if (mt6637_backup_cr_number == 0) {
		pr_info("[%s] mt6637_backup_cr_number=%d",
			__func__, mt6637_backup_cr_number);
		return 1;
	}
	pr_info("[%s] mt6637_backup_cr_number=%d",
		__func__, mt6637_backup_cr_number);
	/* Acquire semaphore */
	if (consys_sema_acquire_timeout_mt6878(CONN_SEMA_RFSPI_INDEX, CONN_SEMA_TIMEOUT)
		== CONN_SEMA_GET_FAIL) {
		pr_err("[%s] Require semaphore fail\n", __func__);
		return CONNINFRA_SPI_OP_FAIL;
	}
	/* Enable a-die top_ck en */
	connsys_adie_top_ck_en_ctl_mt6878(true);
	/* Enable WF clock
	 * ATOP 0xb04 0xfe000000
	 * ATOP 0xb08 0xe0000000
	 * ATOP 0xa04 0xffffffff
	 * ATOP 0xaf4 0xffffffff
	 */
	consys_spi_write_nolock_mt6878(SYS_SPI_TOP, 0xb04, 0xfe000000);
	consys_spi_write_nolock_mt6878(SYS_SPI_TOP, 0xb08, 0xe0000000);
	consys_spi_write_nolock_mt6878(SYS_SPI_TOP, 0xa04, 0xffffffff);
	consys_spi_write_nolock_mt6878(SYS_SPI_TOP, 0xaf4, 0xffffffff);
	/* Write CR back, SYS_SPI_WF & SYS_SPI_WF1 */
	for (i = 0; i < mt6637_backup_cr_number; i++) {
		consys_spi_write_nolock_mt6878(
			SYS_SPI_WF,
			mt6637_backup_data[i].addr,
			mt6637_backup_data[i].value1);
	}
	for (i = 0; i < mt6637_backup_cr_number; i++) {
		consys_spi_write_nolock_mt6878(
			SYS_SPI_WF1,
			mt6637_backup_data[i].addr,
			mt6637_backup_data[i].value2);
	}
	/* Disable WF clock
	 * ATOP 0xb04 0x88000000
	 * ATOP 0xb08 0x00000000
	 * ATOP 0xa04 0x00000000
	 * ATOP 0xaf4 0x00000000
	 */
	consys_spi_write_nolock_mt6878(SYS_SPI_TOP, 0xb04, 0x88000000);
	consys_spi_write_nolock_mt6878(SYS_SPI_TOP, 0xb08, 0x00000000);
	consys_spi_write_nolock_mt6878(SYS_SPI_TOP, 0xa04, 0x00000000);
	consys_spi_write_nolock_mt6878(SYS_SPI_TOP, 0xaf4, 0x00000000);
	/* Release semaphore */
	consys_sema_release_mt6878(CONN_SEMA_RFSPI_INDEX);
	/* Disable a-die top ck en */
	connsys_adie_top_ck_en_ctl_mt6878(false);
	return 0;
}

int consys_pre_cal_clean_data_mt6878(void)
{
	pr_info("[%s]", __func__);
	mt6637_backup_cr_number = 0;

	return 0;
}
