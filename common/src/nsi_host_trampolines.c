/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * See description in header
 */

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int nsi_host_close(int fd)
{
	return close(fd);
}

int nsi_host_isatty(int fd)
{
	return isatty(fd);
}

long nsi_host_random(void)
{
	return random();
}

long nsi_host_read(int fd, void *buffer, unsigned long  size)
{
	return read(fd, buffer, size);
}

void nsi_host_srandom(unsigned int seed)
{
	srandom(seed);
}

int nsi_host_open(const char *pathname, int flags)
{
	return open(pathname, flags);
}

long nsi_host_write(int fd, void *buffer, unsigned long size)
{
	return write(fd, buffer, size);
}
