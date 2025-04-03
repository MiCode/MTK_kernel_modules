/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

/**
 * SM_STATE - Declaration of a state machine function
 * @machine: State machine name
 * @state: State machine state
 *
 * This macro is used to declare a state machine function. It is used in place
 * of a C function definition to declare functions to be run when the state is
 * entered by calling SM_ENTER or SM_ENTER_GLOBAL.
 */
#define SM_STATE(machine, state)                                               \
	static void sm_##machine##_##state##_Enter(P_STATE_MACHINE_DATA sm,    \
						   int global)

/**
 * SM_ENTRY - State machine function entry point
 * @machine: State machine name
 * @state: State machine state
 *
 * This macro is used inside each state machine function declared with
 * SM_STATE. SM_ENTRY should be in the beginning of the function body, but
 * after declaration of possible local variables. This macro prints debug
 * information about state transition and update the state machine state.
 */
#define SM_ENTRY(machine, state)                                               \
	do {                                                                   \
		if (!global || sm->machine##_state != machine##_##state) {     \
			sm->changed = TRUE;                                    \
			wpa_printf(MSG_DEBUG, STATE_MACHINE_DEBUG_PREFIX       \
				   ": " #machine " entering state " #state);   \
		}                                                              \
		sm->machine##_state = machine##_##state;                       \
	} while (0)

/**
 * SM_ENTRY_M - State machine function entry point for state machine group
 * @machine: State machine name
 * @_state: State machine state
 * @data: State variable prefix (full variable: prefix_state)
 *
 * This macro is like SM_ENTRY, but for state machine groups that use a shared
 * data structure for more than one state machine. Both machine and prefix
 * parameters are set to "sub-state machine" name. prefix is used to allow more
 * than one state variable to be stored in the same data structure.
 */
#define SM_ENTRY_M(machine, _state, data)                                      \
	do {                                                                   \
		if (!global || sm->data##_##state != machine##_##_state) {     \
			sm->changed = TRUE;                                    \
			wpa_printf(MSG_DEBUG, STATE_MACHINE_DEBUG_PREFIX       \
				   ": " #machine " entering state " #_state);  \
		}                                                              \
		sm->data##_##state = machine##_##_state;                       \
	} while (0)

/**
 * SM_ENTRY_MA - State machine function entry point for state machine group
 * @machine: State machine name
 * @_state: State machine state
 * @data: State variable prefix (full variable: prefix_state)
 *
 * This macro is like SM_ENTRY_M, but a MAC address is included in debug
 * output. STATE_MACHINE_ADDR has to be defined to point to the MAC address to
 * be included in debug.
 */
#define SM_ENTRY_MA(machine, _state, data)                                     \
	do {                                                                   \
		if (!global || sm->data##_##state != machine##_##_state) {     \
			sm->changed = TRUE;                                    \
			wpa_printf(MSG_INFO, STATE_MACHINE_DEBUG_PREFIX        \
				   ": " MACSTR " " #machine                    \
				   " entering state " #_state "\n",            \
				   MAC2STR(STATE_MACHINE_ADDR));               \
		}                                                              \
		sm->data##_##state = machine##_##_state;                       \
	} while (0)

/**
 * SM_ENTER - Enter a new state machine state
 * @machine: State machine name
 * @state: State machine state
 *
 * This macro expands to a function call to a state machine function defined
 * with SM_STATE macro. SM_ENTER is used in a state machine step function to
 * move the state machine to a new state.
 */
#define SM_ENTER(machine, state) sm_##machine##_##state##_Enter(sm, 0)

/**
 * SM_ENTER_GLOBAL - Enter a new state machine state based on global rule
 * @machine: State machine name
 * @state: State machine state
 *
 * This macro is like SM_ENTER, but this is used when entering a new state
 * based on a global (not specific to any particular state) rule. A separate
 * macro is used to avoid unwanted debug message floods when the same global
 * rule is forcing a state machine to remain in on state.
 */
#define SM_ENTER_GLOBAL(machine, state) sm_##machine##_##state##_Enter(sm, 1)

/**
 * SM_STEP - Declaration of a state machine step function
 * @machine: State machine name
 *
 * This macro is used to declare a state machine step function. It is used in
 * place of a C function definition to declare a function that is used to move
 * state machine to a new state based on state variables. This function uses
 * SM_ENTER and SM_ENTER_GLOBAL macros to enter new state.
 */
#define SM_STEP(machine)                                                       \
	static void sm_##machine##_Step(P_STATE_MACHINE_DATA sm)

/**
 * SM_STEP_RUN - Call the state machine step function
 * @machine: State machine name
 *
 * This macro expands to a function call to a state machine step function
 * defined with SM_STEP macro.
 */
#define SM_STEP_RUN(machine) sm_##machine##_Step(sm)

#endif /* STATE_MACHINE_H */
