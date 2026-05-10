/*
 * Copyright (c) 2017 Oticon A/S
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * HW IRQ controller model
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "nsi_internal.h"
#include "nsi_cpu_if.h"
#include "nsi_cpu0_interrupts.h"
#include "irq_ctrl.h"
#include "nsi_tasks.h"
#include "nsi_tracing.h"
#include "nsi_hws_models_if.h"

#define BITS_U64 64
#define NUM_U64_IRQS (N_IRQS + BITS_U64 - 1) / BITS_U64

static uint64_t irq_ctrl_timer = NSI_NEVER;

// Bitmasks for each interrupt.
static uint64_t irq_status[NUM_U64_IRQS] = {0}; // Pending interrupts.
static uint64_t irq_premask[NUM_U64_IRQS] = {0}; // Interrupts before the mask.

/*
 * Mask of which interrupts will actually cause the cpu to vector into its
 * irq handler
 * If an interrupt is masked in this way, it will be pending in the premask in
 * case it is enabled later before clearing it.
 * If the irq_mask enables and interrupt pending in irq_premask, it will cause
 * the controller to raise the interrupt immediately
 * 0 means masked, 1 means unmasked
 */
static uint64_t irq_mask[NUM_U64_IRQS] = {0};

/*
 * Interrupts lock/disable. When set, interrupts are registered
 * (in the irq_status) but do not awake the cpu. if when unlocked,
 * irq_status != 0 an interrupt will be raised immediately
 */
static bool irqs_locked;
static bool lock_ignore; /* For the hard fake IRQ, temporarily ignore lock */

static uint8_t irq_prio[N_IRQS]; /* Priority of each interrupt */
/* note that prio = 0 == highest, prio=255 == lowest */

static int currently_running_prio = 256; /* 255 is the lowest prio interrupt */

static uint64_t global_irq_to_bitmask(unsigned int irq, size_t *u64_idx) {
	*u64_idx = irq / BITS_U64;
	return ((uint64_t) 1 << (irq % BITS_U64));
}

static void hw_irq_ctrl_init(void)
{
	for (int i = 0; i < NUM_U64_IRQS; i++) {
		irq_mask[i] = 0;
		irq_premask[i] = 0;
	}
	irqs_locked = false;
	lock_ignore = false;

	for (int i = 0 ; i < N_IRQS; i++) {
		irq_prio[i] = 255U;
	}
}

NSI_TASK(hw_irq_ctrl_init, HW_INIT, 10);

void hw_irq_ctrl_set_cur_prio(int new)
{
	currently_running_prio = new;
}

int hw_irq_ctrl_get_cur_prio(void)
{
	return currently_running_prio;
}

void hw_irq_ctrl_prio_set(unsigned int irq, unsigned int prio)
{
	if (irq >= N_IRQS) {
		nsi_print_error_and_exit("Interrupt %i is out of range\n", irq);
	}
	irq_prio[irq] = prio;
}

uint8_t hw_irq_ctrl_get_prio(unsigned int irq)
{
	if (irq >= N_IRQS) {
		nsi_print_error_and_exit("Interrupt %i is out of range\n", irq);
	}
	return irq_prio[irq];
}

/**
 * Get the currently pending highest priority interrupt which has a priority
 * higher than a possibly currently running interrupt
 *
 * If none, return -1
 */
int hw_irq_ctrl_get_highest_prio_irq(void)
{
	if (irqs_locked) {
		return -1;
	}

	int winner = -1;
	int winner_prio = 256;

	for (int i = 0; i < NUM_U64_IRQS; i++) {
		uint64_t status = irq_status[i];
		while (status != 0) {
			int bit_idx = nsi_find_lsb_set64(status) - 1;
			int irq_nbr = bit_idx + (i * BITS_U64);
			if (winner_prio > irq_prio[irq_nbr] && currently_running_prio > irq_prio[irq_nbr]) {
				winner = irq_nbr;
				winner_prio = irq_prio[irq_nbr];
			}
			status &= ~(1ULL << bit_idx);
		}
	}
	return winner;
}


uint32_t hw_irq_ctrl_get_current_lock(void)
{
	return irqs_locked;
}

/*
 * Change the overall interrupt controller "interrupt lock"
 * The interrupt lock is a flag that provisionally disables all interrupts
 * without affecting their status or their ability to be pended in the meanwhile
 */
uint32_t hw_irq_ctrl_change_lock(uint32_t new_lock)
{
	uint32_t previous_lock = irqs_locked;

	irqs_locked = new_lock;

	if ((previous_lock == true) && (new_lock == false)) {
		for (int i = 0; i < NUM_U64_IRQS; i++) {
			if (irq_status[i] != 0) {
				nsif_cpu0_irq_raised_from_sw();
				break;
			}
		}
	}
	return previous_lock;
}

#if N_IRQS <= 64
uint64_t hw_irq_ctrl_get_irq_status(void)
{
	return irq_status[0];
}
#endif

void hw_irq_ctrl_clear_all_enabled_irqs(void)
{
	for (int i = 0; i < NUM_U64_IRQS; i++) {
		irq_status[i] = 0;
		irq_premask[i] &= ~irq_mask[i];
	}
}

void hw_irq_ctrl_clear_all_irqs(void)
{
	for (int i = 0; i < NUM_U64_IRQS; i++) {
		irq_status[i] = 0;
		irq_premask[i] = 0;
	}
}

void hw_irq_ctrl_disable_irq(unsigned int irq)
{
	if (irq >= N_IRQS) {
		nsi_print_error_and_exit("Interrupt %i is out of range\n", irq);
	}
	size_t u64_idx;
	uint64_t bit_mask = global_irq_to_bitmask(irq, &u64_idx);
	irq_mask[u64_idx] &= ~bit_mask;
}

int hw_irq_ctrl_is_irq_enabled(unsigned int irq)
{
	if (irq >= N_IRQS) {
		nsi_print_error_and_exit("Interrupt %i is out of range\n", irq);
	}
	size_t u64_idx;
	uint64_t bit_mask = global_irq_to_bitmask(irq, &u64_idx);
	return irq_mask[u64_idx] & bit_mask;
}

/*
 * Un-pend an interrupt from the interrupt controller.
 *
 * This is an API between the MCU model/IRQ handling side and the IRQ controller
 * model
 */
void hw_irq_ctrl_clear_irq(unsigned int irq)
{
	if (irq >= N_IRQS) {
		nsi_print_error_and_exit("Interrupt %i is out of range\n", irq);
	}
	size_t u64_idx;
	uint64_t bit_mask = global_irq_to_bitmask(irq, &u64_idx);
	irq_status[u64_idx] &= ~bit_mask;
	irq_premask[u64_idx] &= ~bit_mask;
}


/**
 * Enable an interrupt
 *
 * This function may only be called from SW threads
 *
 * If the enabled interrupt is pending, it will immediately vector to its
 * interrupt handler and continue (maybe with some swap() before)
 */
void hw_irq_ctrl_enable_irq(unsigned int irq)
{
	if (irq >= N_IRQS) {
		nsi_print_error_and_exit("Interrupt %i is out of range\n", irq);
	}
	size_t u64_idx;
	uint64_t bit_mask = global_irq_to_bitmask(irq, &u64_idx);
	irq_mask[u64_idx] |= bit_mask;
	if (irq_premask[u64_idx] & bit_mask) { /* if the interrupt is pending */
		hw_irq_ctrl_raise_im_from_sw(irq);
	}
}

static inline void hw_irq_ctrl_irq_raise_prefix(unsigned int irq)
{
	if (irq == PHONY_HARD_IRQ) {
		lock_ignore = true;
	} else if (irq >= N_IRQS) {
		nsi_print_error_and_exit("Interrupt %i is out of range\n", irq);
	}
	size_t u64_idx;
	uint64_t bit_mask = global_irq_to_bitmask(irq, &u64_idx);

	irq_premask[u64_idx] |= bit_mask;

	if (irq_mask[u64_idx] & bit_mask) {
		irq_status[u64_idx] |= bit_mask;
	}
}

/**
 * Set/Raise/Pend an interrupt
 *
 * This function is meant to be used by either the SW manual IRQ raising
 * or by HW which wants the IRQ to be raised in one delta cycle from now
 */
void hw_irq_ctrl_set_irq(unsigned int irq)
{
	if (irq >= N_IRQS) {
		nsi_print_error_and_exit("Interrupt %i is out of range\n", irq);
	}
	hw_irq_ctrl_irq_raise_prefix(irq);
	if ((irqs_locked == false) || lock_ignore) {
		/*
		 * Awake CPU in 1 delta
		 * Note that we awake the CPU even if the IRQ is disabled
		 * => we assume the CPU is always idling in a WFE() like
		 * instruction and the CPU is allowed to awake just with the irq
		 * being marked as pending
		 */
		irq_ctrl_timer = nsi_hws_get_time();
		nsi_hws_find_next_event();
	}
}


static void irq_raising_from_hw_now(void)
{
	/*
	 * We always awake the CPU even if the IRQ was masked,
	 * but not if irqs are locked unless this is due to a
	 * PHONY_HARD_IRQ
	 */
	if ((irqs_locked == false) || lock_ignore) {
		lock_ignore = false;
		nsif_cpu0_irq_raised();
	}
}

/**
 * Set/Raise/Pend an interrupt immediately.
 * Like hw_irq_ctrl_set_irq() but awake immediately the CPU instead of in
 * 1 delta cycle
 *
 * Call only from HW threads; Should be used with care
 */
void hw_irq_ctrl_raise_im(unsigned int irq)
{
	if (irq >= N_IRQS) {
		nsi_print_error_and_exit("Interrupt %i is out of range\n", irq);
	}
	hw_irq_ctrl_irq_raise_prefix(irq);
	irq_raising_from_hw_now();
}

/**
 * Like hw_irq_ctrl_raise_im() but for SW threads
 *
 * Call only from SW threads; Should be used with care
 */
void hw_irq_ctrl_raise_im_from_sw(unsigned int irq)
{
	if (irq >= N_IRQS) {
		nsi_print_error_and_exit("Interrupt %i is out of range\n", irq);
	}
	hw_irq_ctrl_irq_raise_prefix(irq);

	if (irqs_locked == false) {
		nsif_cpu0_irq_raised_from_sw();
	}
}

static void hw_irq_ctrl_timer_triggered(void)
{
	irq_ctrl_timer = NSI_NEVER;
	irq_raising_from_hw_now();
	nsi_hws_find_next_event();
}

NSI_HW_EVENT(irq_ctrl_timer, hw_irq_ctrl_timer_triggered, 900);
