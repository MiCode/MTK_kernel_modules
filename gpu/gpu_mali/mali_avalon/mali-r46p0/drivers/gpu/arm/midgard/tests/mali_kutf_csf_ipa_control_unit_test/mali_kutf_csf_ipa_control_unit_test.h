/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 *
 * (C) COPYRIGHT 2020-2021 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 *
 */

#ifndef _KUTF_CSF_IPA_CONTROL_UNIT_TEST_H_
#define _KUTF_CSF_IPA_CONTROL_UNIT_TEST_H_

#define CSF_IPA_CONTROL_APP_NAME "ipa_control_unit"
#define CSF_IPA_CONTROL_REGISTER_SUITE_NAME "register_unit"
#define CSF_IPA_CONTROL_QUERY_SUITE_NAME "query_unit"
#define CSF_IPA_CONTROL_RESET_SUITE_NAME "reset_unit"
#define CSF_IPA_CONTROL_RATE_CHANGE_SUITE_NAME "rate_change_unit"

#define CSF_IPA_CONTROL_REGISTER_UNIT_TEST_0 "1_client_N_counters"
#define CSF_IPA_CONTROL_REGISTER_UNIT_TEST_1 "N_clients_same_counter"
#define CSF_IPA_CONTROL_REGISTER_UNIT_TEST_2 "N_clients_M_counters"
#define CSF_IPA_CONTROL_REGISTER_UNIT_TEST_3 "1_client_too_many_counters"
#define CSF_IPA_CONTROL_REGISTER_UNIT_TEST_4 "N_clients_incompatible_counters"
#define CSF_IPA_CONTROL_REGISTER_UNIT_TEST_5 "too_many_clients"

#define CSF_IPA_CONTROL_QUERY_UNIT_TEST_0 "1_client_1_counter"
#define CSF_IPA_CONTROL_QUERY_UNIT_TEST_1 "N_clients_same_counter"
#define CSF_IPA_CONTROL_QUERY_UNIT_TEST_2 "gpu_power_cycle"
#define CSF_IPA_CONTROL_QUERY_UNIT_TEST_3 "out_of_order_unregister"

#define CSF_IPA_CONTROL_RESET_UNIT_TEST_0 "manual_sampling_before_reset"
#define CSF_IPA_CONTROL_RESET_UNIT_TEST_1 "STATUS_RESET_unset_after_reset"
#define CSF_IPA_CONTROL_RESET_UNIT_TEST_2 "1_client_1_counter_query_with_reset"
#define CSF_IPA_CONTROL_RESET_UNIT_TEST_3 "N_clients_same_counter_query_after_reset"

#define CSF_IPA_CONTROL_RATE_CHANGE_UNIT_TEST_0 "1_client_1_counter_fixed"
#define CSF_IPA_CONTROL_RATE_CHANGE_UNIT_TEST_1 "2_clients_1_counter_variable"

#endif /* _KUTF_CSF_IPA_CONTROL_UNIT_TEST_H_ */
