/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NSI_COMMON_SRC_INCL_HWS_MODELS_IF_H
#define NSI_COMMON_SRC_INCL_HWS_MODELS_IF_H

#include <stdint.h>
#include "nsi_utils.h"
#include "nsi_hw_scheduler.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Register an event timer and event callback
 *
 * The HW scheduler will keep track of this event, and call its callback whenever its
 * timer is reached.
 * The ordering of events in the same microsecond is given by prio (lowest first),
 * and if also in the same priority by alphabetical order of the callback.
 * (Normally HW models will not care about the event ordering, and will simply set a prio like 100)
 *
 * Only very particular models will need to execute before or after others.
 */
#define NSI_HW_EVENT(timer, fn, prio)	\
	static void (* const NSI_CONCAT(__nsi_hw_event_cb_, fn))(void) \
		__attribute__((__used__)) \
		__attribute__((__section__(".nsi_hw_event" NSI_STRINGIFY(prio) "_callback")))\
		= fn; \
	static uint64_t * const NSI_CONCAT(__nsi_hw_event_ti_, fn) \
		__attribute__((__used__)) \
		__attribute__((__section__(".nsi_hw_event" NSI_STRINGIFY(prio) "_timer")))\
		= &timer

#ifdef __cplusplus
}
#endif

#endif /* NSI_COMMON_SRC_INCL_HWS_MODELS_IF_H */
