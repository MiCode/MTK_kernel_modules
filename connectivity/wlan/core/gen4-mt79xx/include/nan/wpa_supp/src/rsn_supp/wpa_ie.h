/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef WPA_IE_H
#define WPA_IE_H

struct wpa_sm;

struct wpa_eapol_ie_parse {
	const u8 *wpa_ie;
	size_t wpa_ie_len;
	const u8 *rsn_ie;
	size_t rsn_ie_len;
	const u8 *pmkid;
	const u8 *gtk;
	size_t gtk_len;
	const u8 *mac_addr;
	size_t mac_addr_len;
#if (CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT == 1)
	const u8 *igtk;
	size_t igtk_len;
	const u8 *bigtk;
	size_t bigtk_len;
#endif /* CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT */
	const u8 *mdie;
	size_t mdie_len;
	const u8 *ftie;
	size_t ftie_len;
	const u8 *reassoc_deadline;
	const u8 *key_lifetime;
	const u8 *lnkid;
	size_t lnkid_len;
	const u8 *ext_capab;
	size_t ext_capab_len;
	const u8 *supp_rates;
	size_t supp_rates_len;
	const u8 *ext_supp_rates;
	size_t ext_supp_rates_len;
	const u8 *ht_capabilities;
	const u8 *vht_capabilities;
	const u8 *supp_channels;
	size_t supp_channels_len;
	const u8 *supp_oper_classes;
	size_t supp_oper_classes_len;
	u8 qosinfo;
	u16 aid;
	const u8 *wmm;
	size_t wmm_len;
};

u8 *
wpa_add_kde(u8 *pos, u32 kde, const u8 *data, size_t data_len, const u8 *data2,
	    size_t data2_len);
int
wpa_parse_kde_ies(const u8 *buf, size_t len, struct wpa_eapol_ie_parse *ie);

int wpa_supplicant_parse_ies_wpa(const u8 *buf, size_t len,
				 struct wpa_eapol_ie_parse *ie);
int wpa_gen_wpa_ie(struct wpa_sm *sm, u8 *wpa_ie, size_t wpa_ie_len);

#endif /* WPA_IE_H */
