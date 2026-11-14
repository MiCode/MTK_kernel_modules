// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

#include <linux/version.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <net/sock.h>
#include <net/netlink.h>
#include <linux/skbuff.h>
#include <net/genetlink.h>

#include "conn_drv.h"
#include "coredump_macro.h"
#include "conndump_netlink.h"


/*******************************************************************************
*                             MACROS
********************************************************************************
*/
#define MAX_BIND_PROCESS    (4)

#ifndef GENL_ID_GENERATE
#define GENL_ID_GENERATE	0
#endif

#define CONNSYS_DUMP_PKT_SIZE NLMSG_DEFAULT_SIZE

#define CONN_COREDUMP_SYS_LIST(F) \
F(wifi) \
F(bt) \
F(gps)

#define CONN_COREDUMP_SYS_LIST_UPPER(F) \
F(wifi, WIFI) \
F(bt, BT) \
F(gps, GPS)

#define MAX_TAG_LEN 32
#define MAX_INFO_LEN 32
#define SAVE_EMI_HEADER_SIZE 256
#define CONNSYS_HEADER "CONNSYS_HEADER"
#define NLA_DATA(na) ((void *)((char*)(na) + NLA_HDRLEN))

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

enum {
	__CONNDUMP_ATTR_INVALID,
	CONNDUMP_ATTR_MSG,
	CONNDUMP_ATTR_PORT,
	CONNDUMP_ATTR_HEADER,
	CONNDUMP_ATTR_MSG_SIZE,
	CONNDUMP_ATTR_LAST,
	__CONNDUMP_ATTR_MAX,
};
#define CONNDUMP_ATTR_MAX       (__CONNDUMP_ATTR_MAX - 1)

enum {
	__CONNDUMP_COMMAND_INVALID,
	CONNDUMP_COMMAND_BIND,
	CONNDUMP_COMMAND_DUMP,
	CONNDUMP_COMMAND_END,
	CONNDUMP_COMMAND_RESET,
	__CONNDUMP_COMMAND_MAX,
};

enum LINK_STATUS {
	LINK_STATUS_INIT,
	LINK_STATUS_INIT_DONE,
	LINK_STATUS_MAX,
};

struct save_emi {
	phys_addr_t phy_emi_base;
	void __iomem *vir_emi_base;
	char curr_tag[MAX_TAG_LEN];
	char info_buf[MAX_INFO_LEN];
	size_t emi_size;
	unsigned int h_pos; // header pos
	unsigned int w_pos; // write pos
	unsigned int info_len;
	bool check;
};

struct dump_netlink_ctx {
	int conn_type;
	pid_t bind_pid[MAX_BIND_PROCESS];
	unsigned int num_bind_process;
	struct genl_family gnl_family;
	unsigned int seqnum;
	struct mutex nl_lock;
	enum LINK_STATUS status;
	void* coredump_ctx;
	struct netlink_event_cb cb;
	struct save_emi save_emi;
};

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
/* DECLARE_COREDUMP_NETLINK_FUNC */
CONN_COREDUMP_SYS_LIST(DECLARE_COREDUMP_NETLINK_FUNC)

static void conndump_netlink_write_save_emi_header(struct save_emi *s_emi,
	char* tag, unsigned int info, char* buf);

/*******************************************************************************
*                  G L O B A L  V A R I A B L E
********************************************************************************
*/

/* attribute policy */
static struct nla_policy conndump_genl_policy[CONNDUMP_ATTR_MAX + 1] = {
	[CONNDUMP_ATTR_MSG] = {.type = NLA_NUL_STRING},
	[CONNDUMP_ATTR_PORT] = {.type = NLA_U32},
	[CONNDUMP_ATTR_HEADER] = {.type = NLA_NUL_STRING},
	[CONNDUMP_ATTR_MSG_SIZE] = {.type = NLA_U32},
	[CONNDUMP_ATTR_LAST] = {.type = NLA_U32},
};

CONN_COREDUMP_SYS_LIST(DECLARE_COREDUMP_NETLINK_OPS)

struct dump_netlink_ctx g_netlink_ctx[] = {
	CONN_COREDUMP_SYS_LIST_UPPER(DECLARE_COREDUMP_NETLINK_CTX)
};

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

static int conndump_nl_bind_internal(struct dump_netlink_ctx* ctx, struct sk_buff *skb, struct genl_info *info)
{
	int i;
	struct nlattr *port_na;
	unsigned int port;

	if (info == NULL)
		goto out;

	mutex_lock(&ctx->nl_lock);

	port_na = info->attrs[CONNDUMP_ATTR_PORT];
	if (port_na) {
		port = (unsigned int)nla_get_u32(port_na);
	} else {
		pr_err("%s:-> no port_na found\n", __func__);
		mutex_unlock(&ctx->nl_lock);
		return -1;
	}

	for (i = 0; i < MAX_BIND_PROCESS; i ++) {
		if (ctx->bind_pid[i] == 0) {
			ctx->bind_pid[i] = port;
			ctx->num_bind_process++;
			pr_info("%s():-> pid  = %d\n", __func__, port);
			break;
		}
	}
	if (i == MAX_BIND_PROCESS) {
		pr_err("%s(): exceeding binding limit %d\n", __func__, MAX_BIND_PROCESS);
	}
	mutex_unlock(&ctx->nl_lock);

out:
	return 0;
}

static int save_timestamp(struct save_emi *s_emi, struct genl_info *info)
{
	char* attr_msg = NULL;
	char timestamp[MAX_INFO_LEN];
	struct nlattr *attr = NULL;

	if (info == NULL) {
		pr_notice("[%s] genl_info is null\n", __func__);
		return -1;
	}

	memset(timestamp, '\0', MAX_INFO_LEN);
	attr = nlmsg_find_attr(info->nlhdr, sizeof(struct genlmsghdr),
		CONNDUMP_ATTR_MSG);
	if (attr) {
		attr_msg = (char*)NLA_DATA(attr);
		strncpy(timestamp, attr_msg, MAX_INFO_LEN);
		pr_notice("[%s] timestamp=[%s]\n", __func__, timestamp);
		if (s_emi->emi_size != 0)
			conndump_netlink_write_save_emi_header(s_emi, "time",
				0, timestamp);
	} else
		pr_notice("[%s] attr is null\n", __func__);

	return 0;
}

static int conndump_nl_dump_end_internal(struct dump_netlink_ctx* ctx, struct sk_buff *skb, struct genl_info *info)
{
	if (ctx == NULL) {
		pr_notice("[%s] dump_netlink_ctx is null\n", __func__);
		return -1;
	}

	save_timestamp(&(ctx->save_emi), info);

	if (ctx->cb.coredump_end) {
		pr_info("Get coredump end command, type=%d", ctx->conn_type);
		ctx->cb.coredump_end(ctx->coredump_ctx);
	}

	ctx->save_emi.check = false; // set to unchecked status
	if (ctx->save_emi.vir_emi_base) {
		iounmap(ctx->save_emi.vir_emi_base);
		ctx->save_emi.vir_emi_base = NULL;
	}
	return 0;
}

CONN_COREDUMP_SYS_LIST_UPPER(FUNC_IMPL_COREDUMP_NETLINK)

static void conndump_netlink_write_save_emi_header(struct save_emi *s_emi,
	char* tag, unsigned int info, char* buf)
{
	int ret = 0;
	unsigned int written_len;

	memset(s_emi->info_buf, '\0', MAX_INFO_LEN);
	/* "tag=info", ex: CONNSYS_HEADER=0x12345678 or INFO=0x374 */
	if (strcmp(tag, CONNSYS_HEADER) == 0)
		info = s_emi->phy_emi_base;

	if (strcmp(tag, "time") == 0 && buf != NULL)
		ret = snprintf(s_emi->info_buf, MAX_INFO_LEN, "%s=%s,",
			tag, buf);
	else
		ret = snprintf(s_emi->info_buf, MAX_INFO_LEN, "%s=0x%x,",
			tag, info);
	if (ret <= 0)
		pr_notice("[%s] snprintf fail for tag=[%s]", __func__, tag);
	else {
		written_len = strlen(s_emi->info_buf);

		if ((s_emi->h_pos + written_len) < SAVE_EMI_HEADER_SIZE) {
			pr_info("[%s] write header=[%s], size=[%d], h_pos=[%d]",
				__func__, s_emi->info_buf, written_len, s_emi->h_pos);
			memcpy_toio(s_emi->vir_emi_base + s_emi->h_pos, s_emi->info_buf,
				written_len);
			s_emi->h_pos += written_len;
		} else
			pr_notice("[%s] no more space for save emi header", __func__);
	}
}

static void conndump_netlink_dump_to_save_emi(struct save_emi *s_emi, char* tag,
	char *buf, unsigned int len)
{
	char *curr_tag = s_emi->curr_tag;

	if (strcmp(curr_tag, tag) != 0) {
		pr_info("[%s] curr tag is %s, new tag is %s", __func__, curr_tag, tag);

		/* write header */
		conndump_netlink_write_save_emi_header(s_emi, curr_tag,
			s_emi->info_len, NULL);

		memset(curr_tag, '\0', MAX_TAG_LEN);
		strncpy(curr_tag, tag, MAX_TAG_LEN);
		s_emi->info_len = 0;
	}

	/* write content */
	if ((s_emi->w_pos + len) < s_emi->emi_size) {
		memcpy_toio(s_emi->vir_emi_base + s_emi->w_pos, buf, len);
		s_emi->w_pos += len;
		s_emi->info_len += len;
	} else
		pr_notice("[%s] no more space in save emi", __func__);
}

/*****************************************************************************
 * FUNCTION
 *  conndump_netlink_init
 * DESCRIPTION
 *
 * PARAMETERS
 *
 * RETURNS
 *
 *****************************************************************************/
int conndump_netlink_init(int conn_type, void* dump_ctx, struct netlink_event_cb* cb)
{
	int ret = 0;
	struct dump_netlink_ctx* ctx;

	if (conn_type < CONN_ADAPTOR_DRV_WIFI || conn_type >= sizeof(g_netlink_ctx)/sizeof(struct dump_netlink_ctx) ) {
		pr_notice("[%s] Incorrect type (%d)\n", __func__, conn_type);
		return -1;
	}

	ctx = &g_netlink_ctx[conn_type];
	mutex_init(&ctx->nl_lock);
	ret = genl_register_family(&ctx->gnl_family);
	if (ret != 0) {
		pr_err("%s(): GE_NELINK family registration fail (ret=%d)\n", __func__, ret);
		if (ret != -EEXIST)
			return -2;
	}
	ctx->status = LINK_STATUS_INIT_DONE;
	ctx->save_emi.check = false; // set to unchecked status
	memset(ctx->bind_pid, 0, sizeof(ctx->bind_pid));
	ctx->coredump_ctx = dump_ctx;
	memcpy(&(ctx->cb), cb, sizeof(struct netlink_event_cb));

	return 0;
}

int conndump_netlink_msg_send(struct dump_netlink_ctx* ctx, char* tag, char* buf, unsigned int length, pid_t pid, unsigned int seq)
{
	struct sk_buff *skb;
	void* msg_head = NULL;
	int ret = 0;
	int conn_type = ctx->conn_type;

	/* Allocating a Generic Netlink message buffer */
	skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
	if (skb != NULL) {
		/* Create message header */
		msg_head = genlmsg_put(skb, 0, seq, &ctx->gnl_family, 0, CONNDUMP_COMMAND_DUMP);
		if (msg_head == NULL) {
			pr_err("%s(): genlmsg_put fail(conn_type=%d).\n", __func__, conn_type);
			nlmsg_free(skb);
			return -EMSGSIZE;
		}
		/* Add message attribute and content */
		ret = nla_put_string(skb, CONNDUMP_ATTR_HEADER, tag);
		if (ret != 0) {
			pr_err("%s(): nla_put_string header fail(conn_type=%d): %d\n", __func__, conn_type, ret);
			genlmsg_cancel(skb, msg_head);
			nlmsg_free(skb);
			return ret;
		}
		if (length) {
			ret = nla_put(skb, CONNDUMP_ATTR_MSG, length, buf);
			if (ret != 0) {
				pr_err("%s(): nla_put fail(conn_type=%d): %d\n", __func__, conn_type, ret);
				genlmsg_cancel(skb, msg_head);
				nlmsg_free(skb);
				return ret;
			}
			ret = nla_put_u32(skb, CONNDUMP_ATTR_MSG_SIZE, length);
			if (ret != 0) {
				pr_err("%s(): nal_put_u32 fail(conn_type=%d): %d\n", __func__, conn_type, ret);
				genlmsg_cancel(skb, msg_head);
				nlmsg_free(skb);
				return ret;
			}
		}
		/* finalize the message */
		genlmsg_end(skb, msg_head);

		/* sending message */
		ret = genlmsg_unicast(&init_net, skb, pid);
	} else {
		pr_err("Allocate message error\n");
		ret = -ENOMEM;
	}

	return ret;
}

/*****************************************************************************
 * FUNCTION
 *  conndump_send_to_native
 * DESCRIPTION
 *  Send dump content to native layer (AEE or dump service)
 * PARAMETERS
 *  conn_type      [IN]        subsys type
 *  tag            [IN]        the tag for the content (null end string)
 *                             [M] for coredump
 *  buf            [IN]        the content to dump (a buffer may not have end character)
 *  length         [IN]        dump length
 * RETURNS
 *
 *****************************************************************************/
int conndump_netlink_send_to_native_internal(struct dump_netlink_ctx* ctx, char* tag, char* buf, unsigned int length)
{
	int killed_num = 0;
	int i, j, ret = 0;
	unsigned int retry;

	for (i = 0; i < ctx->num_bind_process; i++) {
		ret =conndump_netlink_msg_send(ctx, tag, buf, length, ctx->bind_pid[i], ctx->seqnum);
		if (ret != 0) {
			pr_err("%s(): genlmsg_unicast fail (ret=%d): pid = %d seq=%d tag=%s\n",
				__func__, ret, ctx->bind_pid[i], ctx->seqnum, tag);
			if (ret == -EAGAIN) {
				retry = 0;
				while (retry < 100 && ret == -EAGAIN) {
					msleep(10);
					ret =conndump_netlink_msg_send(ctx, tag, buf, length, ctx->bind_pid[i], ctx->seqnum);
					retry ++;
				}
				if (ret) {
					pr_err("%s(): genlmsg_unicast fail (ret=%d) after retry %d times: pid = %d seq=%d tag=%s\n",
							__func__, ret, retry, ctx->bind_pid[i], ctx->seqnum, tag);
				} else {
					pr_notice("%s(): genlmsg_unicast retry %d times pass: pid = %d seq=%d tag=%s\n",
						__func__,
						retry, ctx->bind_pid[i], ctx->seqnum, tag);
				}
			}
			if (ret == -ECONNREFUSED) {
				ctx->bind_pid[i] = 0;
				killed_num++;
			}
		}
	}

	ctx->seqnum ++;

	/* Clean up invalid bind_pid */
	if (killed_num > 0) {
		mutex_lock(&ctx->nl_lock);
		for (i = 0; i < ctx->num_bind_process - killed_num; i++) {
			if (ctx->bind_pid[i] == 0) {
				for (j = ctx->num_bind_process - 1; j > i; j--) {
					if (ctx->bind_pid[j] > 0) {
						ctx->bind_pid[i] = ctx->bind_pid[j];
						ctx->bind_pid[j] = 0;
					}
				}
			}
		}
		ctx->num_bind_process -= killed_num;
		mutex_unlock(&ctx->nl_lock);
	}

	return ret;
}


/*****************************************************************************
 * FUNCTION
 *  conndump_send_to_native
 * DESCRIPTION
 *  Send dump content to native layer (AEE or dump service)
 * PARAMETERS
 *  conn_type      [IN]        subsys type
 *  tag            [IN]        the tag for the content (null end string)
 *                             [M] for coredump
 *  buf            [IN]        the content to dump (a buffer may not have end character)
 *  length         [IN]        dump length
 * RETURNS
 *
 *****************************************************************************/
int conndump_netlink_send_to_native(int conn_type, char* tag, char* buf, unsigned int length)
{
	struct dump_netlink_ctx* ctx;
	struct save_emi *s_emi = NULL;
	int idx = 0;
	unsigned int send_len;
	unsigned int remain_len = length;
	int ret;

	if (conn_type < CONN_ADAPTOR_DRV_WIFI || conn_type >= sizeof(g_netlink_ctx)/sizeof(struct dump_netlink_ctx)  || tag == NULL) {
		pr_err("Incorrect type (%d), tag = %s\n", conn_type, tag);
		return -1;
	}

	ctx = &g_netlink_ctx[conn_type];
	s_emi = &(ctx->save_emi);
	if (ctx->status != LINK_STATUS_INIT_DONE) {
		pr_err("%s(): netlink should be init (type=%d).\n", __func__, conn_type);
		return -2;
	}

	if (ctx->num_bind_process == 0) {
		pr_err("No bind service\n");
		return -3;
	}

	/* Since memdump mode may be changed in runtime,
	 * it is necessary to check save emi status for each coredump.
	 */
	if (s_emi->check == false && ctx->cb.coredump_get_save_emi) {
		ctx->cb.coredump_get_save_emi(ctx->coredump_ctx, &(s_emi->phy_emi_base),
			&(s_emi->emi_size));
		s_emi->check = true; // set status checked
		pr_info("[%s] check save emi: phy_emi_base=[0x%x], emi_size=[0x%x]\n",
			 __func__, s_emi->phy_emi_base, s_emi->emi_size);

		if (s_emi->emi_size != 0) {
			if (s_emi->vir_emi_base == NULL)
				s_emi->vir_emi_base = ioremap(s_emi->phy_emi_base,
					s_emi->emi_size);

			if (s_emi->vir_emi_base) {
				pr_info("[%s] s_emi->vir_emi_base ioremap ok\n", __func__);
				s_emi->h_pos = 0; // write header pos
				s_emi->w_pos = SAVE_EMI_HEADER_SIZE; // write content pos
				s_emi->info_len = 0;
				memset(s_emi->curr_tag, '\0', MAX_TAG_LEN);
				strncpy(s_emi->curr_tag, CONNSYS_HEADER,
					strlen(CONNSYS_HEADER));
				/* clean save emi */
				memset_io(s_emi->vir_emi_base, 0x0, s_emi->emi_size);
			}
		}
	}

	while (remain_len) {
		send_len = (remain_len > CONNSYS_DUMP_PKT_SIZE? CONNSYS_DUMP_PKT_SIZE : remain_len);

		/* Dump to emi should be called before sending "coredump_end" msg
		 * to native to avoid accessing null vir_emi_base ptr, which may be
		 * occurred when conndump_nl_dump_end_internal is triggered before
		 * saving to emi task is completed.
		 */
		if ((s_emi->emi_size != 0) && s_emi->vir_emi_base)
			conndump_netlink_dump_to_save_emi(s_emi, tag, &buf[idx], send_len);

		ret = conndump_netlink_send_to_native_internal(ctx, tag, &buf[idx], send_len);
		if (ret) {
			pr_notice("[%s][%s] length=%d send section (%d, %d) fail, ret=%d\n",
				__func__, tag, length, idx, send_len, ret);
			break;
		}
		remain_len -= send_len;
		idx += send_len;
	}
	return idx;
}

