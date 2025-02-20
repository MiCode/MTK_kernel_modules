/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#ifndef _PLATFORM_CONNV3_PMIC_MT376_H_
#define _PLATFORM_CONNV3_PMIC_MT376_H_

/* Input buffer format: 4 byte length + 21 byte register_dump
 * register_dump[0~17]: slave: PMIC
 * register_dump[0] : 0x18[1,2,4,5]: OC of [BuckD, BuckIO, BuckR, BuckVB]
 * register_dump[1] : 0x1A[0:7]: OC of LDOs
 * register_dump[2] : 0x19[1,2,4,5]: PG of [BuckD, BuckIO, BuckR, BuckVB]
 * register_dump[3] : 0x1B[0:7]: PG of LDOs
 * register_dump[4] : 0x17[5:7]: Chip OTP, Chip OV, Chip UV
 *
 * register_dump[5] : 0xA0[3:4]: PG/OC mode of RFLDO
 * register_dump[6] : 0xA8[3:4]: PG/OC mode of HIOLDO
 * register_dump[7] : 0xB0[3:4]: PG/OC mode of PHYLDO
 * register_dump[8] : 0xB8[3:4]: PG/OC mode of IOLDO
 * register_dump[9] : 0xC0[3:4]: PG/OC mode of ALDO
 * register_dump[10]: 0xC8[3:4]: PG/OC mode of MLDO
 * register_dump[11]: 0xD0[3:4]: PG/OC mode of ANALDO
 * register_dump[12]: 0xD8[3:4]: PG/OC mode of PALDO
 *
 * register_dump[13]: 0x4A[0]: on UDS mode
 * register_dump[14]: 0x44: switch of i2c recording feature
 * register_dump[15]: 0x45: slave id: 0: slave PMIC, 1: slave BUCK, 2: slave TEST
 * register_dump[16]: 0x46: last ADDR
 * register_dump[17]: 0x47: last WDATA
 *
 * register_dump[18~20]: slave: BUCK
 * register_dump[18]: 0x1C[5:6]: PG/OC mode of BuckD
 * register_dump[19]: 0x24[5:6]: PG/OC mode of BuckIO
 * register_dump[20]: 0x2C[5:6]: PG/OC mode of BuckR
 */

/* slave: PMIC, IC status */
#define PMIC_OTP_EVT		(0x1U << 5)
#define PMIC_SYSOV_EVT		(0x1U << 6)
#define PMIC_SYSUV_EVT		(0x1U << 7)
#define PMIC_STAT_FAIL		(PMIC_OTP_EVT | PMIC_SYSOV_EVT | PMIC_SYSUV_EVT)

/* slave: PMIC, BUCK OC info */
#define PMIC_BUCK_D_OC_EVT	(0x1U << 1)
#define PMIC_BUCK_IO_OC_EVT	(0x1U << 2)
#define PMIC_BUCK_R_OC_EVT	(0x1U << 4)
#define PMIC_PSW_VB_OC_EVT	(0x1U << 5)
#define PMIC_BUCK_OC		(PMIC_BUCK_D_OC_EVT | PMIC_BUCK_IO_OC_EVT | PMIC_BUCK_R_OC_EVT | PMIC_PSW_VB_OC_EVT)

/* slave: PMIC, LDO OC info */
#define PMIC_RFLDO_OC_EVT	(0x1U << 0)
#define PMIC_HIOLDO_OC_EVT	(0x1U << 1)
#define PMIC_PHYLDO_OC_EVT	(0x1U << 2)
#define PMIC_IOLDO_OC_EVT	(0x1U << 3)
#define PMIC_ALDO_OC_EVT	(0x1U << 4)
#define PMIC_MLDO_OC_EVT	(0x1U << 5)
#define PMIC_ANALDO_OC_EVT	(0x1U << 6)
#define PMIC_PALDO_OC_EVT	(0x1U << 7)
#define PMIC_LDO_OC		(PMIC_RFLDO_OC_EVT | PMIC_HIOLDO_OC_EVT | PMIC_PHYLDO_OC_EVT | PMIC_IOLDO_OC_EVT | PMIC_ALDO_OC_EVT | PMIC_MLDO_OC_EVT | PMIC_ANALDO_OC_EVT | PMIC_PALDO_OC_EVT)

/* slave: PMIC, BUCK PG info */
#define PMIC_BUCK_PG_EVT_ADDR 0x19

#define PMIC_BUCK_D_PGB_EVT	(0x1U << 1)
#define PMIC_BUCK_IO_PGB_EVT	(0x1U << 2)
#define PMIC_BUCK_R_PGB_EVT	(0x1U << 4)
#define PMIC_BUCK_PG		(PMIC_BUCK_D_PGB_EVT | PMIC_BUCK_IO_PGB_EVT | PMIC_BUCK_R_PGB_EVT)

/* slave: PMIC, LDO PG info */
#define PMIC_LDO_PG_EVT_ADDR 0x1B

#define PMIC_RFLDO_PGB_EVT	(0x1U << 0)
#define PMIC_HIOLDO_PGB_EVT	(0x1U << 1)
#define PMIC_PHYLDO_PGB_EVT	(0x1U << 2)
#define PMIC_IOLDO_PGB_EVT	(0x1U << 3)
#define PMIC_ALDO_PGB_EVT	(0x1U << 4)
#define PMIC_MLDO_PGB_EVT	(0x1U << 5)
#define PMIC_ANALDO_PGB_EVT	(0x1U << 6)
#define PMIC_PALDO_PGB_EVT	(0x1U << 7)
#define PMIC_LDO_PG		(PMIC_RFLDO_PGB_EVT | PMIC_HIOLDO_PGB_EVT | PMIC_PHYLDO_PGB_EVT | PMIC_IOLDO_PGB_EVT | PMIC_ALDO_PGB_EVT | PMIC_MLDO_PGB_EVT | PMIC_ANALDO_PGB_EVT | PMIC_PALDO_PGB_EVT)

/* slave: BUCK, BUCK op mode info */
#define BUCK_BUCK_OC_MODE_DBGFLAG	(0x1U << 6)
#define BUCK_BUCK_PG_MODE_DBGFLAG	(0x1U << 5)

/* slave: PMIC, LDO op mode info */
#define PMIC_LDO_OC_MODE_DBGFLAG	(0x1U << 4)
#define PMIC_LDO_PG_MODE_DBGFLAG	(0x1U << 3)

/* slave: PMIC, UDS enable status */
#define PMIC_UDS_EN_STAT_BIT		(0x1U << 0)

#define PMIC_STAT_SIZE  21

static inline int connv3_pmic_parse_state_mt6376(char *buffer, int buf_sz)
{
#define TMP_LOG_SIZE 128
#define MT6376_REG_SIZE 8
	u8 *register_dump;
	u8 pmic_stat = 0;
	u8 buck_oc_stat = 0, ldo_oc_stat = 0;
	u8 buck_pg_stat = 0, ldo_pg_stat = 0;
	u8 uds_status;
	u8 i2c_last_dev, i2c_last_addr, i2c_last_wdata;
	u8 log_len = 0;
	int i;
	u32 data_size = 0;
	char log_buf[TMP_LOG_SIZE];
	static int s_first_dump = 1;
	const char *pmic_exception_type_str[MT6376_REG_SIZE] = {"", "", "", "", "", "OverTemperature ", "OverVoltage ", "UVLO "};
	const char *buck_name_str[MT6376_REG_SIZE] = {"", "BUCK_D ", "BUCK_IO ", "", "BUCK_R ", "PSW_VB ", "", ""};
	const char *ldo_name_str[MT6376_REG_SIZE] = {"RFLDO ", "HIOLDO ", "PHYLDO ", "IOLDO ", "ALDO ", "MLDO ", "ANALDO ", "PALDO "};

	if (!buffer){
		pr_err("[%s] PMIC dump register is NULL\n", __func__);
		return -1;
	}

	/* 4 byte to describe number of PMIC registers */
	memcpy(&data_size, buffer, 4);
	pr_info("[%s] data size=[%d]", __func__, data_size);

	if (data_size != PMIC_STAT_SIZE) {
		pr_notice("[MT6376] incorrect state size=[%d]", data_size);
		return -1;
	}

	register_dump = buffer + 4;

	/* 1. OT/OV/UV status */
	pmic_stat = register_dump[4] & (PMIC_STAT_FAIL);

	/* 2. check BUCK/LDO OC status */
	buck_oc_stat = register_dump[0] & PMIC_BUCK_OC;
	ldo_oc_stat = register_dump[1] & PMIC_LDO_OC;

	/* 3. check BUCK/LDO PG status */
	buck_pg_stat = register_dump[2] & PMIC_BUCK_PG;
	ldo_pg_stat = register_dump[3] & PMIC_LDO_PG;

	/* 4. UDS status */
	uds_status = register_dump[13] & (PMIC_UDS_EN_STAT_BIT);

	/* 5. check last i2c write command */
	/* i2c_last_dev: 0x00: write to PMIC, 0x01: write to BUCK, 0x10: write to TM */
	i2c_last_dev = register_dump[15];
	i2c_last_addr = register_dump[16];
	i2c_last_wdata = register_dump[17];

	if (s_first_dump && pmic_stat == PMIC_SYSUV_EVT
		&& buck_oc_stat == 0 && ldo_oc_stat == 0
		&& buck_pg_stat == 0 && ldo_pg_stat == 0) {
		pr_info("[%s] 1st time enable PMIC, UVLO happen before reboot.\n", __func__);
	} else if (s_first_dump && buck_oc_stat == PMIC_BUCK_IO_OC_EVT
		&& ldo_oc_stat == 0
		&& buck_pg_stat == 0 && ldo_pg_stat == 0){
		pr_info("[%s] 1st time enable PMIC, BUCK_IO OC happen before reboot.\n", __func__);
	} else if (pmic_stat
		|| buck_oc_stat || ldo_oc_stat
		|| buck_pg_stat || ldo_pg_stat) {

		pr_notice("[MT6376] EXCEPTION pmic=[%02X] oc=[%02X][%02X] pg=[%02X][%02X]",
				pmic_stat, buck_oc_stat, ldo_oc_stat, buck_pg_stat, ldo_pg_stat);

		/* 2.1. check "op mode when OC happen" */
		/* 3.1. check "op mode when PG happen" */
		if (buck_oc_stat || ldo_oc_stat || buck_pg_stat || ldo_pg_stat) {
			pr_notice("[MT6376] EXCEPTION op=%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
					(register_dump[18] & (BUCK_BUCK_OC_MODE_DBGFLAG | BUCK_BUCK_PG_MODE_DBGFLAG)),
					(register_dump[19] & (BUCK_BUCK_OC_MODE_DBGFLAG | BUCK_BUCK_PG_MODE_DBGFLAG)),
					(register_dump[20] & (BUCK_BUCK_OC_MODE_DBGFLAG | BUCK_BUCK_PG_MODE_DBGFLAG)),
					(register_dump[5] & (PMIC_LDO_OC_MODE_DBGFLAG | PMIC_LDO_PG_MODE_DBGFLAG)),
					(register_dump[6] & (PMIC_LDO_OC_MODE_DBGFLAG | PMIC_LDO_PG_MODE_DBGFLAG)),
					(register_dump[7] & (PMIC_LDO_OC_MODE_DBGFLAG | PMIC_LDO_PG_MODE_DBGFLAG)),
					(register_dump[8] & (PMIC_LDO_OC_MODE_DBGFLAG | PMIC_LDO_PG_MODE_DBGFLAG)),
					(register_dump[9] & (PMIC_LDO_OC_MODE_DBGFLAG | PMIC_LDO_PG_MODE_DBGFLAG)),
					(register_dump[10] & (PMIC_LDO_OC_MODE_DBGFLAG | PMIC_LDO_PG_MODE_DBGFLAG)),
					(register_dump[11] & (PMIC_LDO_OC_MODE_DBGFLAG | PMIC_LDO_PG_MODE_DBGFLAG)),
					(register_dump[12] & (PMIC_LDO_OC_MODE_DBGFLAG | PMIC_LDO_PG_MODE_DBGFLAG)));
		}

		if (pmic_stat) {
			log_buf[0] = '\0';
			for (i = 5; i <= 7; i++) {
				if (pmic_stat & (0x1 << i))
					strncat(log_buf, pmic_exception_type_str[i], strlen(pmic_exception_type_str[i]));
			}
			pr_notice("[MT6376] EXCEPTION PMIC exception: %s", log_buf);
		}
		if (buck_oc_stat) {
			log_buf[0] = '\0';
			for (i = 1; i < 6; i++) {
				if (buck_oc_stat & (0x1 << i))
					strncat(log_buf, buck_name_str[i], strlen(buck_name_str[i]));
			}
			pr_notice("[MT6376] EXCEPTION BUCK OC: %s", log_buf);
		}
		if (ldo_oc_stat) {
			log_buf[0] = '\0';
			for (i = 0; i < MT6376_REG_SIZE; i++) {
				if (ldo_oc_stat & (0x1 << i))
					strncat(log_buf, ldo_name_str[i], strlen(ldo_name_str[i]));
			}
			pr_notice("[MT6376] EXCEPTION LDO OC: %s", log_buf);
		}
		if (buck_pg_stat) {
			log_buf[0] = '\0';
			/* There is no PSW_VB PG */
			for (i = 1; i < 5; i++) {
				if (buck_pg_stat & (0x1 << i))
					strncat(log_buf, buck_name_str[i], strlen(buck_name_str[i]));
			}
			pr_notice("[MT6376] EXCEPTION BUCK PG: %s", log_buf);
		}
		if (ldo_pg_stat) {
			log_buf[0] = '\0';
			for (i = 0; i < MT6376_REG_SIZE; i++) {
				if (ldo_pg_stat & (0x1 << i))
					strncat(log_buf, ldo_name_str[i], strlen(ldo_name_str[i]));
			}
			pr_notice("[MT6376] EXCEPTION LDO PG: %s", log_buf);
		}

		pr_notice("[MT6376] EXCEPTION UDS=[%d], Last i2c write to %s, write 0x%x by 0x%x\n",
			uds_status, (i2c_last_dev == 0x0)?"PMIC":"BUCK", i2c_last_addr, i2c_last_wdata);

		log_buf[0] = '\0';
		log_len = snprintf(log_buf + log_len, TMP_LOG_SIZE - log_len, "[MT6376] EXCEPTION: ");
		if (pmic_stat) {
			if (pmic_stat & PMIC_OTP_EVT)
				log_len += snprintf(log_buf + log_len, TMP_LOG_SIZE - log_len, "OT ");
			if (pmic_stat & PMIC_SYSOV_EVT)
				log_len += snprintf(log_buf + log_len, TMP_LOG_SIZE - log_len, "OV ");
			if (pmic_stat & PMIC_SYSUV_EVT)
				log_len += snprintf(log_buf + log_len, TMP_LOG_SIZE - log_len, "UV ");
		}
		if (buck_oc_stat)
			log_len += snprintf(log_buf + log_len, TMP_LOG_SIZE - log_len, "BUCK_OC %02X ", buck_oc_stat);

		if (ldo_oc_stat)
			log_len += snprintf(log_buf + log_len, TMP_LOG_SIZE - log_len, "LDO_OC %02X ", ldo_oc_stat);

		if (buck_pg_stat)
			log_len += snprintf(log_buf + log_len, TMP_LOG_SIZE - log_len, "BUCK_PG %02X ", buck_pg_stat);

		if (ldo_pg_stat)
			log_len += snprintf(log_buf + log_len, TMP_LOG_SIZE - log_len, "LDO_PG %02X ", ldo_pg_stat);
		log_len += snprintf(log_buf + log_len, TMP_LOG_SIZE - log_len, "\n");

		osal_dbg_kernel_exception("Connv3", "%s", log_buf);
	}
	s_first_dump = 0;

	/* print UDS and I2C command for more info */
	pr_info("[%s] UDS status=[%d], Last i2c write to %s, write 0x%x by 0x%x\n",
			__func__, uds_status, (i2c_last_dev == 0x0)?"PMIC":"BUCK", i2c_last_addr, i2c_last_wdata);

	return 0;
}



#endif /* _PLATFORM_CONNV3_PMIC_MT376_H_ */

