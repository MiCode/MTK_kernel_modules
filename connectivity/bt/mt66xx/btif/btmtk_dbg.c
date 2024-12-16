/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#include <linux/kthread.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>

#include "btmtk_define.h"
#include "btmtk_chip_if.h"
#include "btmtk_main.h"
#include "conninfra.h"
#include "connsys_debug_utility.h"
#include "metlog.h"

/*******************************************************************************
*				 C O N S T A N T S
********************************************************************************
*/
#define BT_DBG_PROCNAME "driver/bt_dbg"
#define BUF_LEN_MAX 384
#define BT_DBG_DUMP_BUF_SIZE 1024
#define BT_DBG_PASSWD "4w2T8M65K5?2af+a "
#define BT_DBG_USER_TRX_PREFIX "[user-trx] "

/*******************************************************************************
*			       D A T A	 T Y P E S
********************************************************************************
*/
typedef int(*BT_DEV_DBG_FUNC) (int par1, int par2, int par3);
typedef struct {
  BT_DEV_DBG_FUNC func;
  bool turn_off_availavle; // function can be work when bt off
} tBT_DEV_DBG_STRUCT;

/*******************************************************************************
*			      P U B L I C   D A T A
********************************************************************************
*/
bool g_bt_trace_pt = FALSE;

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
static ssize_t bt_dbg_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
static ssize_t bt_dbg_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
static int bt_dbg_hwver_get(int par1, int par2, int par3);
static int bt_dbg_chip_rst(int par1, int par2, int par3);
static int bt_dbg_read_chipid(int par1, int par2, int par3);
static int bt_dbg_force_bt_wakeup(int par1, int par2, int par3);
static int bt_dbg_get_fwp_datetime(int par1, int par2, int par3);
static int bt_dbg_get_bt_patch_path(int par1, int par2, int par3);
extern int fwp_if_get_datetime(char *buf, int max_len);
extern int fwp_if_get_bt_patch_path(char *buf, int max_len);
#if (CUSTOMER_FW_UPDATE == 1)
static int bt_dbg_set_fwp_update_enable(int par1, int par2, int par3);
static int bt_dbg_get_fwp_update_info(int par1, int par2, int par3);
extern void fwp_if_set_update_enable(int par);
extern int fwp_if_get_update_info(char *buf, int max_len);
#endif
static int bt_dbg_reg_read(int par1, int par2, int par3);
static int bt_dbg_reg_write(int par1, int par2, int par3);
static int bt_dbg_ap_reg_read(int par1, int par2, int par3);
static int bt_dbg_ap_reg_write(int par1, int par2, int par3);
static int bt_dbg_setlog_level(int par1, int par2, int par3);
static int bt_dbg_set_rt_thread(int par1, int par2, int par3);
static int bt_dbg_get_bt_state(int par1, int par2, int par3);
static int bt_dbg_rx_buf_control(int par1, int par2, int par3);
static int bt_dbg_set_rt_thread_runtime(int par1, int par2, int par3);
static int bt_dbg_fpga_test(int par1, int par2, int par3);
static int bt_dbg_is_adie_work(int par1, int par2, int par3);
static int bt_dbg_met_start_stop(int par1, int par2, int par3);
static int bt_dbg_DynamicAdjustTxPower(int par1, int par2, int par3);
#if (BUILD_QA_DBG == 1)
static void bt_dbg_user_trx_proc(char *cmd_raw);
#endif
static int bt_dbg_user_trx_cb(uint8_t *buf, int len);
static int bt_dbg_trace_pt(int par1, int par2, int par3);

extern int32_t btmtk_set_wakeup(struct hci_dev *hdev, uint8_t need_wait);
extern int32_t btmtk_set_sleep(struct hci_dev *hdev, u_int8_t need_wait);
extern void bt_trigger_reset(void);
extern int32_t btmtk_set_power_on(struct hci_dev*, u_int8_t for_precal);
extern int32_t btmtk_set_power_off(struct hci_dev*, u_int8_t for_precal);

/*******************************************************************************
*			     P R I V A T E   D A T A
********************************************************************************
*/
extern struct btmtk_dev *g_sbdev;
extern struct bt_dbg_st g_bt_dbg_st;
static struct proc_dir_entry *g_bt_dbg_entry;
static struct mutex g_bt_lock;
static char g_bt_dump_buf[BT_DBG_DUMP_BUF_SIZE];
static char *g_bt_dump_buf_ptr;
static int g_bt_dump_buf_len;
#if (BUILD_QA_DBG == 1)
static bool g_bt_dbg_enable = FALSE;
#endif

static const tBT_DEV_DBG_STRUCT bt_dev_dbg_struct[] = {
	[0x0] = {bt_dbg_hwver_get, 				FALSE},
	[0x1] = {bt_dbg_chip_rst, 				FALSE},
	[0x2] = {bt_dbg_read_chipid, 			FALSE},
	[0x3] = {bt_dbg_force_bt_wakeup,		FALSE},
	[0x4] = {bt_dbg_reg_read, 				FALSE},
	[0x5] = {bt_dbg_reg_write, 				FALSE},
	[0x6] = {bt_dbg_get_fwp_datetime,		TRUE},
#if (CUSTOMER_FW_UPDATE == 1)
	[0x7] = {bt_dbg_set_fwp_update_enable, 	TRUE},
	[0x8] = {bt_dbg_get_fwp_update_info,	FALSE},
#endif
	[0x9] = {bt_dbg_ap_reg_read,		FALSE},
	[0xa] = {bt_dbg_ap_reg_write,		TRUE},
	[0xb] = {bt_dbg_setlog_level,		TRUE},
	[0xc] = {bt_dbg_get_bt_patch_path,	TRUE},
	[0xd] = {bt_dbg_set_rt_thread,		TRUE},
	[0xe] = {bt_dbg_get_bt_state,		TRUE},
	[0xf] = {bt_dbg_rx_buf_control,		TRUE},
	[0x10] = {bt_dbg_set_rt_thread_runtime,		FALSE},
	[0x11] = {bt_dbg_fpga_test,			TRUE},
	[0x12] = {bt_dbg_is_adie_work,		TRUE},
	[0x13] = {bt_dbg_met_start_stop,	FALSE},
	[0x14] = {bt_dbg_DynamicAdjustTxPower,		FALSE},
	[0x15] = {bt_dbg_trace_pt,		FALSE},
};

/*******************************************************************************
*			       F U N C T I O N S
********************************************************************************
*/



void _bt_dbg_reset_dump_buf(void)
{
	memset(g_bt_dump_buf, '\0', BT_DBG_DUMP_BUF_SIZE);
	g_bt_dump_buf_ptr = g_bt_dump_buf;
	g_bt_dump_buf_len = 0;
}

int bt_dbg_hwver_get(int par1, int par2, int par3)
{
	BTMTK_INFO("query chip version");
	/* TODO: */
	return 0;
}

int bt_dbg_chip_rst(int par1, int par2, int par3)
{
	if(par2 == 0)
		bt_trigger_reset();
	else
		conninfra_trigger_whole_chip_rst(CONNDRV_TYPE_BT, "bt_dbg");
	return 0;
}

int bt_dbg_trace_pt(int par1, int par2, int par3)
{
	if(par2 == 0)
		g_bt_trace_pt = FALSE;
	else
		g_bt_trace_pt = TRUE;
	return 0;
}

int bt_dbg_read_chipid(int par1, int par2, int par3)
{
	return 0;
}

/* Read BGF SYS address (controller view) by 0x18001104 & 0x18900000 */
int bt_dbg_reg_read(int par1, int par2, int par3)
{
	uint32_t *dynamic_remap_addr = NULL;
	uint32_t *dynamic_remap_value = NULL;

	/* TODO: */
	dynamic_remap_addr = ioremap(0x18001104, 4);
	if (dynamic_remap_addr) {
		*dynamic_remap_addr = par2;
		BTMTK_DBG("read address = [0x%08x]", par2);
	} else {
		BTMTK_ERR("ioremap 0x18001104 fail");
		return -1;
	}
	iounmap(dynamic_remap_addr);

	dynamic_remap_value = ioremap(0x18900000, 4);
	if (dynamic_remap_value)
		BTMTK_INFO("%s: 0x%08x value = [0x%08x]", __func__, par2,
							*dynamic_remap_value);
	else {
		BTMTK_ERR("ioremap 0x18900000 fail");
		return -1;
	}
	iounmap(dynamic_remap_value);
	return 0;
}

/* Write BGF SYS address (controller view) by 0x18001104 & 0x18900000 */
int bt_dbg_reg_write(int par1, int par2, int par3)
{
#if 0
#if (CFG_BT_ATF_SUPPORT == 1)
	SendAtfSmcCmd_dbg_write(SMC_BT_DBG_REG_WRITE, par2, par3);
#else
	uint32_t *dynamic_remap_addr = NULL;
	uint32_t *dynamic_remap_value = NULL;

	/* TODO: */
	dynamic_remap_addr = ioremap(0x18001104, 4);
	if (dynamic_remap_addr) {
		*dynamic_remap_addr = par2;
		BTMTK_DBG("write address = [0x%08x]", par2);
	} else {
		BTMTK_ERR("ioremap 0x18001104 fail");
		return -1;
	}
	iounmap(dynamic_remap_addr);

	dynamic_remap_value = ioremap(0x18900000, 4);
	if (dynamic_remap_value)
		*dynamic_remap_value = par3;
	else {
		BTMTK_ERR("ioremap 0x18900000 fail");
		return -1;
	}
	iounmap(dynamic_remap_value);
#endif
#endif
	return 0;

}

int bt_dbg_ap_reg_read(int par1, int par2, int par3)
{
#if (BUILD_QA_DBG == 0)
	return -ENODEV;
#else
	uint32_t *remap_addr = NULL;
	int ret_val = 0;

	/* TODO: */
	remap_addr = ioremap(par2, 4);
	if (!remap_addr) {
		BTMTK_ERR("ioremap [0x%08x] fail", par2);
		return -1;
	}

	ret_val = *remap_addr;
	BTMTK_INFO("%s: 0x%08x read value = [0x%08x]", __func__, par2, ret_val);
	iounmap(remap_addr);
	return ret_val;
#endif
}

int bt_dbg_ap_reg_write(int par1, int par2, int par3)
{
#if 0
#if (CFG_BT_ATF_SUPPORT == 1)
        SendAtfSmcCmd_dbg_write(SMC_BT_DBG_AP_REG_WRITE, par2, par3);
#else
	uint32_t *remap_addr = NULL;

	/* TODO: */
	remap_addr = ioremap(par2, 4);
	if (!remap_addr) {
		BTMTK_ERR("ioremap [0x%08x] fail", par2);
		return -1;
	}

	*remap_addr = par3;
	BTMTK_INFO("%s: 0x%08x write value = [0x%08x]", __func__, par2, par3);
	iounmap(remap_addr);
#endif
#endif
	return 0;
}

int bt_dbg_setlog_level(int par1, int par2, int par3)
{
	if (par2 < BTMTK_LOG_LVL_ERR || par2 > BTMTK_LOG_LVL_DBG) {
		btmtk_log_lvl = BTMTK_LOG_LVL_INFO;
	} else {
		btmtk_log_lvl = par2;
	}
	return 0;
}

int bt_dbg_set_rt_thread(int par1, int par2, int par3)
{
	g_bt_dbg_st.rt_thd_enable = par2;
	return 0;
}

int bt_dbg_set_rt_thread_runtime(int par1, int par2, int par3)
{
	struct sched_param params;
	int policy = 0;
	int ret = 0;
	struct btmtk_btif_dev *cif_dev = (struct btmtk_btif_dev *)g_sbdev->cif_dev;

	/* reference parameter:
		- normal: 0x10 0x01(SCHED_FIFO) 0x01
		- rt_thd: 0x10 0x01(SCHED_FIFO) 0x50(MAX_RT_PRIO - 20)
	*/
	if (par2 > SCHED_DEADLINE || par3 > MAX_RT_PRIO) {
		BTMTK_INFO("%s: parameter not allow!", __func__);
		return 0;
	}
	policy = par2;
	params.sched_priority = par3;
	ret = sched_setscheduler(cif_dev->tx_thread, policy, &params);
	BTMTK_INFO("%s: ret[%d], policy[%d], sched_priority[%d]", __func__, ret, policy, params.sched_priority);

	return 0;
}

int bt_dbg_fpga_test(int par1, int par2, int par3)
{
	/* reference parameter:
		- 0x12 0x01(power on) 0x00
		- 0x12 0x02(power off) 0x00
	*/
	BTMTK_INFO("%s: par2 = %d", __func__, par2);
	switch (par2) {
		case 1:
			btmtk_set_power_on(g_sbdev->hdev, FALSE);
			break;
		case 2:
			btmtk_set_power_off(g_sbdev->hdev, FALSE);
			break;
		default:
			break;
	}
	BTMTK_INFO("%s: done", __func__);

	return 0;
}

int bt_dbg_is_adie_work(int par1, int par2, int par3)
{
	int ret = 0, adie_state = 0;
	struct btmtk_btif_dev *cif_dev = (struct btmtk_btif_dev *)g_sbdev->cif_dev;

	if (cif_dev->bt_state == FUNC_ON) {
		adie_state = 0; // power on a-die pass
		goto end;
	}

	ret = conninfra_pwr_on(CONNDRV_TYPE_BT);
	//if ((ret == CONNINFRA_POWER_ON_A_DIE_FAIL) || (ret == CONNINFRA_POWER_ON_D_DIE_FAIL))
	if (ret != 0)
		adie_state = 1; // power on a-die fail, may be evb without DTB
	else {
		adie_state = 0; // power on a-die pass
		conninfra_pwr_off(CONNDRV_TYPE_BT);
	}

end:
	BTMTK_INFO("%s: ret[%d], adie_state[%d]", __func__, ret, adie_state);
	_bt_dbg_reset_dump_buf();
	g_bt_dump_buf[0] = (adie_state == 0 ? '0' : '1'); // '0': adie pass, '1': adie fail
	g_bt_dump_buf[1] = '\0';
	g_bt_dump_buf_len = 2;
	return 0;
}

int bt_dbg_met_start_stop(int par1, int par2, int par3)
{
	uint32_t val = 0, star_addr = 0, end_addr = 0;
	int res = 0;
	struct conn_metlog_info info;
	phys_addr_t emi_base;

	BTMTK_INFO("%s, par2 = %d", __func__, par2);
	/* reference parameter:
		- start: 0x11 0x01 0x00
		- stop: 0x11 0x00 0x00
	*/
	if (par2 == 0x01) {
		/*
		// Set EMI Writing Range
		bt_dbg_ap_reg_write(0, 0x1882140C, 0xF0027000); // BGF_ON_MET_START_ADDR
		bt_dbg_ap_reg_write(0, 0x18821410, 0xF002EFFF); // BGF_ON_MET_END_ADDR
		*/

		// Set Ring Buffer Mode
		val = bt_dbg_ap_reg_read(0, 0x18821404, 0);
		bt_dbg_ap_reg_write(0, 0x18821404, val | 0x0001); // BGF_ON_MET_CTL1[0] = 0x01

		// Set Sampling Rate
		val = bt_dbg_ap_reg_read(0, 0x18821400, 0);
		bt_dbg_ap_reg_write(0, 0x18821400, (val & 0xFFFF80FF) | 0x00001900); // BGF_ON_MET_CTL0[14:8] = 0x19

		// Set Mask Signal
		//bt_dbg_ap_reg_write(0, 0x18821400, (val & 0x0000FFFF) | 0x????0000); // BGF_ON_MET_CTL0[31:16] = ?

		// Enable Connsys MET
		val = bt_dbg_ap_reg_read(0, 0x18821400, 0);
		bt_dbg_ap_reg_write(0, 0x18821400, (val & 0xFFFFFFFC) | 0x00000003); // BGF_ON_MET_CTL0[1:0] = 0x03

		/* write parameters and start MET test */
		conninfra_get_phy_addr(&emi_base, NULL);
		info.type = CONNDRV_TYPE_BT;
		info.read_cr = 0x18821418;
		info.write_cr = 0x18821414;

		// FW will write the star_addr & end_addr to cooresponding CRs when bt on
		star_addr = bt_dbg_ap_reg_read(0, 0x1882140C, 0);
		end_addr = bt_dbg_ap_reg_read(0, 0x18821410, 0);
		BTMTK_INFO("%s: star_addr[0x%08x], end_addr[0x%08x]", __func__, star_addr, end_addr);

		if (star_addr >= 0x00400000 && star_addr <= 0x0041FFFF) {
			// met data on sysram
			info.met_base_ap = 0x18440000 + star_addr;
			info.met_base_fw = star_addr;
		} else if (star_addr >= 0xF0000000 && star_addr <= 0xF3FFFFFF){
			// met data on emi
			info.met_base_ap = emi_base + MET_EMI_ADDR;
			info.met_base_fw = 0xF0000000 + MET_EMI_ADDR;
		} else {
			// error case
			BTMTK_ERR("%s: get unexpected met address!!", __func__);
			return 0;
		}

		info.met_size = end_addr - star_addr + 1;
		info.output_len = 32;
		res = conn_metlog_start(&info);
		BTMTK_INFO("%s: conn_metlog_start, result = %d", __func__, res);
	} else {
		// stop MET test
		res = conn_metlog_stop(CONNDRV_TYPE_BT);
		BTMTK_INFO("%s: conn_metlog_stop, result = %d", __func__, res);

		// Disable Connsys MET
		val = bt_dbg_ap_reg_read(0, 0x18821400, 0);
		bt_dbg_ap_reg_write(0, 0x18821400, val & 0xFFFFFFFE); // BGF_ON_MET_CTL0[0] = 0x00
	}
	return 0;
}

int bt_dbg_DynamicAdjustTxPower_cb(uint8_t *buf, int len)
{
	BTMTK_INFO("%s", __func__);
	bt_dbg_user_trx_cb(buf, len);
	return 0;
}

int bt_dbg_DynamicAdjustTxPower(int par1, int par2, int par3)
{
	uint8_t mode = (uint8_t)par2;
	int8_t set_val = (int8_t)par3;

	/* reference parameter:
		- query: 0x14 0x01(query) 0x00
		- set:   0x14 0x02(set)   0x??(set_dbm_val)
	*/
	BTMTK_INFO("%s", __func__);
	btmtk_inttrx_DynamicAdjustTxPower(mode, set_val, bt_dbg_DynamicAdjustTxPower_cb, TRUE);
	return 0;
}

/*
sample code to use gpio
int bt_dbg_device_is_evb(int par1, int par2, int par3)
{
	struct device_node *node = NULL;
	int gpio_addr = 0, gpio_val = 0;

	node = of_find_compatible_node(NULL, NULL, "mediatek,evb_gpio");
	gpio_addr = of_get_named_gpio(node, "evb_gpio", 0);
	if (gpio_addr > 0)
		gpio_val = gpio_get_value(gpio_addr); // 0x00: phone, 0x01: evb

	BTMTK_INFO("%s: gpio_addr[%d], gpio_val[%d]", __func__, gpio_addr, gpio_val);
	_bt_dbg_reset_dump_buf();
	g_bt_dump_buf[0] = (gpio_val == 0 ? '0' : '1'); // 0x00: phone, 0x01: evb
	g_bt_dump_buf[1] = '\0';
	g_bt_dump_buf_len = 2;

	return 0;
}
dts setting
	evb_gpio: evb_gpio@1100c000 {
		compatible = "mediatek,evb_gpio";
		evb_gpio = <&pio 57 0x0>;
	};
*/

int bt_dbg_get_bt_state(int par1, int par2, int par3)
{
	struct btmtk_btif_dev *cif_dev = (struct btmtk_btif_dev *)g_sbdev->cif_dev;
	bool bt_state = 0;

	// 0x01: bt on, 0x00: bt off
	bt_state = (cif_dev->bt_state == FUNC_ON ? 1 : 0);

	BTMTK_INFO("%s: bt_state[%d]", __func__, bt_state);
	_bt_dbg_reset_dump_buf();
	g_bt_dump_buf[0] = bt_state;
	g_bt_dump_buf[1] = '\0';
	g_bt_dump_buf_len = 2;
	return 0;
}

int bt_dbg_force_bt_wakeup(int par1, int par2, int par3)
{
	int ret;
	struct btmtk_btif_dev *cif_dev = (struct btmtk_btif_dev *)g_sbdev->cif_dev;

	BTMTK_INFO("%s", __func__);

	switch(par2) {
	case 0:
		cif_dev->psm.force_on = FALSE;
		ret = btmtk_set_sleep(g_sbdev->hdev, TRUE);
		break;

	case 1:
		cif_dev->psm.force_on = TRUE;
		ret = btmtk_set_wakeup(g_sbdev->hdev, TRUE);
		break;
	default:
		BTMTK_ERR("Not support");
		return -1;
	}

	BTMTK_INFO("bt %s %s", (par2 == 1) ? "wakeup" : "sleep",
			        (ret) ? "fail" : "success");

	return 0;
}

int bt_dbg_get_fwp_datetime(int par1, int par2, int par3)
{
	_bt_dbg_reset_dump_buf();
	g_bt_dump_buf_len = fwp_if_get_datetime(g_bt_dump_buf, BT_DBG_DUMP_BUF_SIZE);
	return 0;
}

int bt_dbg_get_bt_patch_path(int par1, int par2, int par3)
{
	_bt_dbg_reset_dump_buf();
	g_bt_dump_buf_len = fwp_if_get_bt_patch_path(g_bt_dump_buf, BT_DBG_DUMP_BUF_SIZE);
	return 0;
}

#if (CUSTOMER_FW_UPDATE == 1)
int bt_dbg_set_fwp_update_enable(int par1, int par2, int par3)
{
	fwp_if_set_update_enable(par2);
	return 0;
}


int bt_dbg_get_fwp_update_info(int par1, int par2, int par3)
{
	_bt_dbg_reset_dump_buf();
	g_bt_dump_buf_len = fwp_if_get_update_info(g_bt_dump_buf, BT_DBG_DUMP_BUF_SIZE);
	return 0;
}
#endif

int bt_dbg_rx_buf_control(int par1, int par2, int par3)
{
	/*
		0x00: disable
		0x01: wait rx buffer available for max 200ms
	*/
	BTMTK_INFO("%s: rx_buf_ctrl[%d] set to [%d]", __func__, g_bt_dbg_st.rx_buf_ctrl, par2);
	g_bt_dbg_st.rx_buf_ctrl = par2;
	return 0;
}

ssize_t bt_dbg_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
#if (BUILD_QA_DBG == 0)
	return -ENODEV;
#else
	int ret = 0;
	int dump_len;

	BTMTK_INFO("%s: count[%zd]", __func__, count);
	ret = mutex_lock_killable(&g_bt_lock);
	if (ret) {
		BTMTK_ERR("%s: dump_lock fail!!", __func__);
		return ret;
	}

	if (g_bt_dump_buf_len == 0)
		goto exit;

	if (*f_pos == 0)
		g_bt_dump_buf_ptr = g_bt_dump_buf;

	dump_len = g_bt_dump_buf_len >= count ? count : g_bt_dump_buf_len;
	ret = copy_to_user(buf, g_bt_dump_buf_ptr, dump_len);
	if (ret) {
		BTMTK_ERR("%s: copy to dump info buffer failed, ret:%d", __func__, ret);
		ret = -EFAULT;
		goto exit;
	}

	*f_pos += dump_len;
	g_bt_dump_buf_len -= dump_len;
	g_bt_dump_buf_ptr += dump_len;
	BTMTK_INFO("%s: after read, wmt for dump info buffer len(%d)", __func__, g_bt_dump_buf_len);

	ret = dump_len;
exit:

	mutex_unlock(&g_bt_lock);
	return ret;
#endif
}

int bt_osal_strtol(const char *str, unsigned int adecimal, long *res)
{
	if (sizeof(long) == 4)
		return kstrtou32(str, adecimal, (unsigned int *) res);
	else
		return kstrtol(str, adecimal, res);
}

int bt_dbg_user_trx_cb(uint8_t *buf, int len)
{
	unsigned char *ptr = buf;
	int i = 0;

	_bt_dbg_reset_dump_buf();
	// write event packet type
	if (snprintf(g_bt_dump_buf, 6, "0x04 ") < 0) {
		BTMTK_INFO("%s: snprintf error", __func__);
		goto end;
	}
	for (i = 0; i < len; i++) {
		if (snprintf(g_bt_dump_buf + 5*(i+1), 6, "0x%02X ", ptr[i]) < 0) {
			BTMTK_INFO("%s: snprintf error", __func__);
			goto end;
		}
	}
	len++;
	g_bt_dump_buf[5*len] = '\n';
	g_bt_dump_buf[5*len + 1] = '\0';
	g_bt_dump_buf_len = 5*len + 1;

end:
	return 0;
}

#if (BUILD_QA_DBG == 1)
void bt_dbg_user_trx_proc(char *cmd_raw)
{
#define LEN_64 64
	unsigned char hci_cmd[LEN_64];
	int len = 0;
	long tmp = 0;
	char *ptr = NULL, *pRaw = NULL;

	// Parse command raw data
	memset(hci_cmd, 0, sizeof(hci_cmd));
	pRaw = cmd_raw;
	ptr = cmd_raw;
	while(*ptr != '\0' && pRaw != NULL) {
		if (len > LEN_64 - 1) {
			BTMTK_INFO("%s: skip since cmd length exceed!", __func__);
			return;
		}
		ptr = strsep(&pRaw, " ");
		if (ptr != NULL) {
			bt_osal_strtol(ptr, 16, &tmp);
			hci_cmd[len++] = (unsigned char)tmp;
		}
	}

	// Send command and wait for command_complete event
	btmtk_btif_internal_trx(hci_cmd, len, bt_dbg_user_trx_cb, TRUE, TRUE);
}
#endif

ssize_t bt_dbg_write(struct file *filp, const char __user *buffer, size_t count, loff_t *f_pos)
{
#if (BUILD_QA_DBG == 0)
	return -ENODEV;
#else
	bool is_passwd = FALSE, is_turn_on = FALSE;
	size_t len = count;
	char buf[256], *pBuf;
	int x = 0, y = 0, z = 0;
	long res = 0;
	char* pToken = NULL;
	char* pDelimiter = " \t";
	struct btmtk_btif_dev *cif_dev = (struct btmtk_btif_dev *)g_sbdev->cif_dev;
	bool bt_state = 0;

	bt_state = (cif_dev->bt_state == FUNC_ON ? 1 : 0);

	if (len <= 0 || len >= sizeof(buf)) {
		BTMTK_ERR("%s: input handling fail!", __func__);
		len = sizeof(buf) - 1;
		return -1;
	}

	memset(buf, 0, sizeof(buf));
	if (copy_from_user(buf, buffer, len))
		return -EFAULT;
	buf[len] = '\0';
	BTMTK_INFO("%s: bt_state[%d], dbg_enable[%d], len[%d]",
		__func__, bt_state, g_bt_dbg_enable, (int)len);

	/* Check debug function is enabled or not
	 *   - not enable yet: user should enable it
	 *   - already enabled: user can disable it
	 */
	if (len > strlen(BT_DBG_PASSWD) &&
		0 == memcmp(buf, BT_DBG_PASSWD, strlen(BT_DBG_PASSWD))) {
		is_passwd = TRUE;
		if (0 == memcmp(buf + strlen(BT_DBG_PASSWD), "ON", strlen("ON")))
			is_turn_on = TRUE;
	}
	if(!g_bt_dbg_enable) {
		if(is_passwd && is_turn_on)
			g_bt_dbg_enable = TRUE;
		return len;
	} else {
		if(is_passwd && !is_turn_on) {
			g_bt_dbg_enable = FALSE;
			return len;
		}
	}

	/* Mode 1: User trx flow: send command, get response */
	if (0 == memcmp(buf, BT_DBG_USER_TRX_PREFIX, strlen(BT_DBG_USER_TRX_PREFIX))) {
		if(!bt_state) // only work when bt on
			return len;
		buf[len - 1] = '\0';
		bt_dbg_user_trx_proc(buf + strlen(BT_DBG_USER_TRX_PREFIX));
		return len;
	}

	/* Mode 2: Debug cmd flow, parse three parameters */
	pBuf = buf;
	pToken = strsep(&pBuf, pDelimiter);
	if (pToken != NULL) {
		bt_osal_strtol(pToken, 16, &res);
		x = (int)res;
	} else {
		x = 0;
	}

	pToken = strsep(&pBuf, "\t\n ");
	if (pToken != NULL) {
		bt_osal_strtol(pToken, 16, &res);
		y = (int)res;
		BTMTK_INFO("%s: y = 0x%08x", __func__, y);
	} else {
		y = 3000;
		/*efuse, register read write default value */
		if (0x5 == x || 0x6 == x)
			y = 0x80000000;
	}

	pToken = strsep(&pBuf, "\t\n ");
	if (pToken != NULL) {
		bt_osal_strtol(pToken, 16, &res);
		z = (int)res;
	} else {
		z = 10;
		/*efuse, register read write default value */
		if (0x5 == x || 0x6 == x)
			z = 0xffffffff;
	}

	BTMTK_INFO("%s: x(0x%08x), y(0x%08x), z(0x%08x)", __func__, x, y, z);
	if (ARRAY_SIZE(bt_dev_dbg_struct) > x && NULL != bt_dev_dbg_struct[x].func) {
		if(!bt_state && !bt_dev_dbg_struct[x].turn_off_availavle) {
			BTMTK_WARN("%s: command id(0x%08x) only work when bt on!", __func__, x);
		} else {
			(*bt_dev_dbg_struct[x].func) (x, y, z);
		}
	} else {
		BTMTK_WARN("%s: command id(0x%08x) no handler defined!", __func__, x);
	}

	return len;
#endif
}

int bt_dev_dbg_init(void)
{
	int i_ret = 0;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
	static const struct proc_ops bt_dbg_fops = {
		.proc_read = bt_dbg_read,
		.proc_write = bt_dbg_write,
	};
#else
	static const struct file_operations bt_dbg_fops = {
		.owner = THIS_MODULE,
		.read = bt_dbg_read,
		.write = bt_dbg_write,
	};
#endif

	// initialize debug function struct
	g_bt_dbg_st.rt_thd_enable = FALSE;
	g_bt_dbg_st.rx_buf_ctrl = TRUE;

	g_bt_dbg_entry = proc_create(BT_DBG_PROCNAME, 0664, NULL, &bt_dbg_fops);
	if (g_bt_dbg_entry == NULL) {
		BTMTK_ERR("Unable to create [%s] bt proc entry", BT_DBG_PROCNAME);
		i_ret = -1;
	}

	mutex_init(&g_bt_lock);

	return i_ret;
}

int bt_dev_dbg_deinit(void)
{
	mutex_destroy(&g_bt_lock);

	if (g_bt_dbg_entry != NULL) {
		proc_remove(g_bt_dbg_entry);
		g_bt_dbg_entry = NULL;
	}

	return 0;
}

/*******************************************************************************
*                           bt host debug information for low power
********************************************************************************
*/
#define BTHOST_INFO_MAX	16
#define BTHOST_DESC_LEN 16

struct bthost_info{
	uint32_t		id; //0 for not used
	char 		desc[BTHOST_DESC_LEN];
	uint32_t		value;
};
struct bthost_info bthost_info_table[BTHOST_INFO_MAX];

void bthost_debug_init(void)
{
	uint32_t i = 0;
	for (i = 0; i < BTHOST_INFO_MAX; i++){
		bthost_info_table[i].id = 0;
		bthost_info_table[i].desc[0] = '\0';
		bthost_info_table[i].value = 0;
	}
}

void bthost_debug_print(void)
{
	uint32_t i = 0;
	int32_t ret = 0;
	uint8_t *pos = NULL, *end = NULL;
	uint8_t dump_buffer[700]={0};

	pos = &dump_buffer[0];
	end = pos + 700 - 1;

	ret = snprintf(pos, (end - pos + 1), "[bt host info] ");
	if (ret < 0 || ret >= (end - pos + 1)) {
		BTMTK_ERR("snprintf [bt host info] fail");
	} else {
		pos += ret;
	}

	for (i = 0; i < BTHOST_INFO_MAX; i++){
		if (bthost_info_table[i].id == 0){
			ret = snprintf(pos, (end - pos + 1),"[%d-%d] not set", i, BTHOST_INFO_MAX);
			if (ret < 0 || ret >= (end - pos + 1)){
				BTMTK_ERR("%s: snprintf fail i[%d] ret[%d]", __func__, i, ret);
				break;
			}
			pos += ret;
			break;
		}
		else {
			ret = snprintf(pos, (end - pos + 1),"[%d][%s : 0x%08x] ", i,
			bthost_info_table[i].desc,
			bthost_info_table[i].value);
			if (ret < 0 || ret >= (end - pos + 1)){
				BTMTK_ERR("%s: snprintf fail i[%d] ret[%d]", __func__, i, ret);
				break;
			}
			pos += ret;
		}
	}
	BTMTK_INFO("%s", dump_buffer);
}

void bthost_debug_save(uint32_t id, uint32_t value, char* desc)
{
	uint32_t i = 0;
	if (id == 0) {
		BTMTK_WARN("%s: id (%d) must > 0\n", __func__, id);
		return;
	}
	for (i = 0; i < BTHOST_INFO_MAX; i++){
		// if the id is existed, save to the same column
		if (bthost_info_table[i].id == id){
			bthost_info_table[i].value = value;
			return;
		}
		// save to the new column
		if (bthost_info_table[i].id == 0){
			bthost_info_table[i].id = id;
			strncpy(bthost_info_table[i].desc, desc, BTHOST_DESC_LEN - 1);
			bthost_info_table[i].value = value;
			return;
		}
	}
	BTMTK_WARN("%s: no space for %d\n", __func__, id);
}



