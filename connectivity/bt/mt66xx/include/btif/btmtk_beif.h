// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

/* GDMA solution */

#ifndef BEIF_H
#define BEIF_H
typedef int (*BEIF_RX_CB) (unsigned char *p_buf,  unsigned int len);
typedef int (*BEIF_NOTIFY_FW_CB) (void);

struct beif_info_t {
	BEIF_RX_CB rx_cb;               /* receive data sent from firmware */
	BEIF_NOTIFY_FW_CB notify_fw_cb; /* notify firmware of sending data. e.g. trigger IRQ.*/
	phys_addr_t addr;               /* physical address of EMI buffer */
	int size;                       /* size of EMI buffer */
};

/* init related resource, should be called once */
int beif_init(struct beif_info_t *info);

/* deinit related resource */
int beif_deinit(void);

/* reset related resource, can be called during function on */
int beif_reset(void);

/* send data to firmware */
int beif_send_data(const unsigned char *p_buf, unsigned int len);

/* check and receive data sent from firmware. called in irq handler */
int beif_receive_data(void);

/* check whether any data not received by driver or firmware yet */
int beif_is_tx_complete(void);

/* for debug: print recent tx log */
void beif_print_tx_log(void);

/* for debug: print recent rx log */
void beif_print_rx_log(void);

/* for debug: print last tx data in EMI buffer */
int beif_dump_tx_last_data(unsigned int len);

/* for debug: print last rx data in EMI buffer */
int beif_dump_rx_last_data(unsigned int len);

/* for debug: check whether header is corrupted */
int beif_check_header(void);

#endif
