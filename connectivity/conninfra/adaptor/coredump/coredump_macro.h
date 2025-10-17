/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef _COREDUMP_MACRO_H_
#define _COREDUMP_MACRO_H_


#define DECLARE_COREDUMP_NETLINK_FUNC(name) \
static int conndump_nl_bind_##name(struct sk_buff *skb, struct genl_info *info); \
static int conndump_nl_dump_##name(struct sk_buff *skb, struct genl_info *info); \
static int conndump_nl_dump_end_##name(struct sk_buff *skb, struct genl_info *info); \
static int conndump_nl_reset_##name(struct sk_buff *skb, struct genl_info *info);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 2, 0))
#define COREDUMP_NETLINK_POLICY_DEF .policy = conndump_genl_policy,
#else
#define COREDUMP_NETLINK_POLICY_DEF
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 2, 0))
#define COREDUMP_NETLINK_POLICY_DEF_FOR_GNL_FAMILY .policy = conndump_genl_policy,
#endif

#define DECLARE_COREDUMP_NETLINK_OPS(name) \
static struct genl_ops conndump_gnl_ops_array_##name[] = { \
	{ \
		.cmd = CONNDUMP_COMMAND_BIND, \
		.flags = 0, \
		COREDUMP_NETLINK_POLICY_DEF \
		.doit = conndump_nl_bind_##name, \
		.dumpit = NULL, \
	}, \
	{ \
		.cmd = CONNDUMP_COMMAND_DUMP, \
		.flags = 0, \
		COREDUMP_NETLINK_POLICY_DEF \
		.doit = conndump_nl_dump_##name, \
		.dumpit = NULL, \
	}, \
	{ \
		.cmd = CONNDUMP_COMMAND_END, \
		.flags = 0, \
		COREDUMP_NETLINK_POLICY_DEF \
		.doit = conndump_nl_dump_end_##name, \
		.dumpit = NULL, \
	}, \
	{ \
		.cmd = CONNDUMP_COMMAND_RESET, \
		.flags = 0, \
		COREDUMP_NETLINK_POLICY_DEF \
		.doit = conndump_nl_reset_##name, \
		.dumpit = NULL, \
	}, \
};


#define DECLARE_COREDUMP_NETLINK_CTX(dname, uname) \
	{ \
		.conn_type = CONN_ADAPTOR_DRV_##uname, \
		.gnl_family = { \
			.id = GENL_ID_GENERATE, \
			.hdrsize = 0, \
			.name = "CONNDUMP_" #uname, \
			.version = 1, \
			.maxattr = CONNDUMP_ATTR_MAX, \
			.ops = conndump_gnl_ops_array_##dname, \
			.n_ops = ARRAY_SIZE(conndump_gnl_ops_array_##dname), \
			COREDUMP_NETLINK_POLICY_DEF_FOR_GNL_FAMILY \
		}, \
		.status = LINK_STATUS_INIT, \
		.num_bind_process = 0, \
		.seqnum = 0, \
	},



#define FUNC_IMPL_COREDUMP_NETLINK(name, uname) \
static int conndump_nl_bind_##name(struct sk_buff *skb, struct genl_info *info) \
{ \
	int ret = 0; \
\
	ret = conndump_nl_bind_internal(&g_netlink_ctx[CONN_ADAPTOR_DRV_##uname], skb, info); \
	return ret; \
} \
\
static int conndump_nl_dump_##name(struct sk_buff *skb, struct genl_info *info) \
{ \
	pr_err("%s(): should not be invoked\n", __func__); \
	return 0; \
} \
\
static int conndump_nl_dump_end_##name(struct sk_buff *skb, struct genl_info *info) \
{ \
	int ret = 0; \
\
	ret = conndump_nl_dump_end_internal(&g_netlink_ctx[CONN_ADAPTOR_DRV_##uname], skb, info); \
	return ret; \
} \
\
static int conndump_nl_reset_##name(struct sk_buff *skb, struct genl_info *info) \
{ \
	pr_err("%s(): should not be invoked\n", __func__); \
	return 0; \
}




#endif /* _COREDUMP_MACRO_H_ */
