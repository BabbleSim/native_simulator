/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef NSI_COMMON_SRC_INCL_NCT_IF_H
#define NSI_COMMON_SRC_INCL_NCT_IF_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Interface provided by the Native simulator CPU threading emulation
 *
 * A description of each function can be found in the C file
 *
 * In docs/NCT.md you can find more information
 */

void *nct_init(void (*fptr)(void *));
void nct_clean_up(void *st);
void nct_swap_threads(void *st, int next_allowed_thread_nbr);
void nct_first_thread_start(void *st, int next_allowed_thread_nbr);
int nct_new_thread(void *st, void *payload);
void nct_abort_thread(void *st, int thread_idx);
int nct_get_unique_thread_id(void *st, int thread_idx);
int nct_thread_name_set(void *st, int thread_idx, const char *str);
void nct_get_thread_stack(void *st, int thread_idx, void **stack_addr, unsigned long *stack_size);

#ifdef __cplusplus
}
#endif

#endif /* NSI_COMMON_SRC_INCL_NCT_IF_H */
