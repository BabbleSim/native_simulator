/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef NSI_COMMON_SRC_INCL_NCE_IF_H
#define NSI_COMMON_SRC_INCL_NCE_IF_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Native simulator CPU start/stop emulation module interface
 *
 * Check docs/NCE.md for an overview.
 *
 * A description of each function can be found in the .c file
 */

#define NCE_CPU_NBR_NO_CPU_RUNNING -2
#define NCE_CPU_NBR_UNKNOWN -1

void *nce_init(void);
void nce_set_cpu_nbr(void *st, int cpu_nbr);
int nce_get_current_cpu_nbr(void);
void nce_terminate(void *st);
void nce_boot_cpu(void *st, void (*start_routine)(void));
void nce_halt_cpu(void *st);
void nce_wake_cpu(void *st);
int nce_is_cpu_running(void *st);

#ifdef __cplusplus
}
#endif

#endif /* NSI_COMMON_SRC_INCL_NCE_IF_H */
