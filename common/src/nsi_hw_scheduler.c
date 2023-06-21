/*
 * Copyright (c) 2017 Oticon A/S
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Overall HW models scheduler for the native simulator
 *
 * Models events are registered with NSI_HW_EVENT().
 */

#include <stdint.h>
#include <signal.h>
#include <stddef.h>
#include <inttypes.h>
#include "nsi_tracing.h"
#include "nsi_main.h"
#include "nsi_safe_call.h"
#include "nsi_hw_scheduler.h"

static uint64_t simu_time; /* The actual time as known by the HW models */
static uint64_t end_of_time = NSI_NEVER; /* When will this device stop */

extern uint64_t *__nsi_hw_events_timers_start[];
extern uint64_t *__nsi_hw_events_timers_end[];

extern void (*__nsi_hw_events_callbacks_start[])(void);
extern void (*__nsi_hw_events_callbacks_end[])(void);

static unsigned int number_of_timers;
static unsigned int number_of_callbacks;

static unsigned int next_timer_index;
static uint64_t next_timer_time;

/* Have we received a SIGTERM or SIGINT */
static volatile sig_atomic_t signaled_end;

/**
 * Handler for SIGTERM and SIGINT
 */
static void nsi_hws_signal_end_handler(int sig)
{
	signaled_end = 1;
}

/**
 * Set the handler for SIGTERM and SIGINT which will cause the
 * program to exit gracefully when they are received the 1st time
 *
 * Note that our handler only sets a variable indicating the signal was
 * received, and in each iteration of the hw main loop this variable is
 * evaluated.
 * If for some reason (the program is stuck) we never evaluate it, the program
 * would never exit.
 * Therefore we set SA_RESETHAND: This way, the 2nd time the signal is received
 * the default handler would be called to terminate the program no matter what.
 *
 * Note that SA_RESETHAND requires either _POSIX_C_SOURCE>=200809 or
 * _XOPEN_SOURCE>=500
 */
static void nsi_hws_set_sig_handler(void)
{
	struct sigaction act;

	act.sa_handler = nsi_hws_signal_end_handler;
	NSI_SAFE_CALL(sigemptyset(&act.sa_mask));

	act.sa_flags = SA_RESETHAND;

	NSI_SAFE_CALL(sigaction(SIGTERM, &act, NULL));
	NSI_SAFE_CALL(sigaction(SIGINT, &act, NULL));
}


static void nsi_hws_sleep_until_next_event(void)
{
	if (next_timer_time >= simu_time) { /* LCOV_EXCL_BR_LINE */
		simu_time = next_timer_time;
	} else {
		/* LCOV_EXCL_START */
		nsi_print_warning("next_timer_time corrupted (%"PRIu64"<= %"
				PRIu64", timer idx=%i)\n",
				(uint64_t)next_timer_time,
				(uint64_t)simu_time,
				next_timer_index);
		/* LCOV_EXCL_STOP */
	}

	if (signaled_end || (simu_time > end_of_time)) {
		nsi_print_trace("\nStopped at %.3Lfs\n",
				((long double)simu_time)/1.0e6L);
		nsi_exit(0);
	}
}


/**
 * Find in between all events timers which is the next one.
 * (and update the internal next_timer_* accordingly)
 */
void nsi_hws_find_next_event(void)
{
	next_timer_index = 0;
	next_timer_time  = *__nsi_hw_events_timers_start[0];

	for (unsigned int i = 1; i < number_of_timers ; i++) {
		if (next_timer_time > *__nsi_hw_events_timers_start[i]) {
			next_timer_index = i;
			next_timer_time = *__nsi_hw_events_timers_start[i];
		}
	}
}

/**
 * Execute the next scheduled HW event
 * (advancing time until that event would trigger)
 */
void nsi_hws_one_event(void)
{
	nsi_hws_sleep_until_next_event();

	if (next_timer_index < number_of_timers) { /* LCOV_EXCL_BR_LINE */
		__nsi_hw_events_callbacks_start[next_timer_index]();
	} else {
		nsi_print_error_and_exit("next_timer_index corrupted\n"); /* LCOV_EXCL_LINE */
	}

	nsi_hws_find_next_event();
}

/**
 * Set the simulated time when the process will stop
 */
void nsi_hws_set_end_of_time(uint64_t new_end_of_time)
{
	end_of_time = new_end_of_time;
}

/**
 * Return the current simulated time as known by the device
 */
uint64_t nsi_hws_get_time(void)
{
	return simu_time;
}

/**
 * Function to initialize the HW scheduler
 *
 * Note that the HW models should register their initialization functions
 * as NSI_TASKS of HW_INIT level.
 */
void nsi_hws_init(void)
{
	number_of_timers =
		(__nsi_hw_events_timers_end - __nsi_hw_events_timers_start);
	number_of_callbacks =
		(__nsi_hw_events_callbacks_end - __nsi_hw_events_callbacks_start);

	/* LCOV_EXCL_START */
	if (number_of_timers != number_of_callbacks || number_of_timers == 0) {
		nsi_print_error_and_exit("number_of_timers corrupted\n");
	}
	/* LCOV_EXCL_STOP */

	nsi_hws_set_sig_handler();
	nsi_hws_find_next_event();
}

/**
 * Function to free any resources allocated by the HW scheduler
 *
 * Note that the HW models should register their initialization functions
 * as NSI_TASKS of ON_EXIT_PRE/POST levels.
 */
void nsi_hws_cleanup(void)
{
}
