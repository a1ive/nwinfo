/*
 * Copyright 2024  Veselin Georgiev,
 * anrieffNOSPAM @ mgail_DOT.com (convert to gmail)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* freebsd requires _XOPEN_SOURCE 600 for snprintf()
 * for linux it is enough 500 */
#define _XOPEN_SOURCE 600
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libcpuid.h"
#include "libcpuid_util.h"
#include "libcpuid_arm_driver.h"
#include "rdcpuid.h"

#define CPUID_PATH_LEN 32

#if defined (__linux__) || defined (__gnu_linux__) || defined (__FreeBSD__) || defined (__DragonFly__)
/* Assuming linux with /dev/cpu/x/cpuid: */
/* Assuming FreeBSD with /dev/cpuctlX */
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
struct cpuid_driver_t { int fd; };
static int load_driver(char *cpuid_path)
{
	const int file_exists   = !access(cpuid_path, F_OK);
	const int file_readable = !access(cpuid_path, R_OK);

	if (file_exists && file_readable)
		return 1;
	else if (file_exists && !file_readable)
		return 0;
	else if (getuid() != 0)
		return 0;
	else
# if defined (__linux__) || defined (__gnu_linux__)
		return !system("modprobe cpuid 2> /dev/null");
# elif defined (__FreeBSD__) || defined (__DragonFly__)
		return !system("kldload -n cpuctl 2> /dev/null");
# endif
}

struct cpuid_driver_t* cpu_cpuid_driver_open_core(unsigned core_num)
{
	char cpuid[CPUID_PATH_LEN];
	struct cpuid_driver_t* handle;
	if (core_num >= cpuid_get_total_cpus()) {
		cpuid_set_error(ERR_INVCNB);
		return NULL;
	}
# if defined (__linux__) || defined (__gnu_linux__)
	snprintf(cpuid, CPUID_PATH_LEN, "/dev/cpu/%u/cpuid", core_num);
# elif defined (__FreeBSD__) || defined (__DragonFly__)
	snprintf(cpuid, CPUID_PATH_LEN, "/dev/cpuid%u", core_num);
# endif
	if(!load_driver(cpuid)) {
		cpuid_set_error(ERR_NO_DRIVER);
		return NULL;
	}
	int fd = open(cpuid, O_RDONLY);
	if (fd < 0) {
		if (errno == EIO) {
			cpuid_set_error(ERR_NO_CPUID);
			return NULL;
		}
		cpuid_set_error(ERR_NO_DRIVER);
		return NULL;
	}
	handle = (struct cpuid_driver_t*) malloc(sizeof(struct cpuid_driver_t));
	if (!handle) {
		cpuid_set_error(ERR_NO_MEM);
		close(fd);
		return NULL;
	}
	handle->fd = fd;
	return handle;
}

int cpu_read_arm_register_32b(struct cpuid_driver_t* driver, reg_request_t request, uint32_t* result)
{
	struct read_reg_t read_reg;
	read_reg.request = request;

	if (!driver || driver->fd < 0)
		return cpuid_set_error(ERR_HANDLE);

	if (ioctl(driver->fd, ARM_IOC_READ_REG, &read_reg))
		return cpuid_set_error(ERR_IOCTL);

	if (read_reg.request == REQ_INVALID)
		return cpuid_set_error(ERR_REQUEST);

	*result = read_reg.value_32b;
	return 0;
}

int cpu_read_arm_register_64b(struct cpuid_driver_t* driver, reg_request_t request, uint64_t* result)
{
	struct read_reg_t read_reg;
	read_reg.request = request;

	if (!driver || driver->fd < 0)
		return cpuid_set_error(ERR_HANDLE);

	if (ioctl(driver->fd, ARM_IOC_READ_REG, &read_reg))
		return cpuid_set_error(ERR_IOCTL);

	if (read_reg.request == REQ_INVALID)
		return cpuid_set_error(ERR_REQUEST);

	*result = read_reg.value_64b;
	return 0;
}

int cpu_cpuid_driver_close(struct cpuid_driver_t* drv)
{
	if (drv && drv->fd >= 0) {
		close(drv->fd);
		free(drv);
	}
	return 0;
}

#else /* Unsupported OS */
/* On others OS (i.e., Darwin), we still do not support RDCPUID, so supply dummy struct
   and functions */

struct cpuid_driver_t { int dummy; };

struct cpuid_driver_t* cpu_cpuid_driver_open_core(unsigned core_num)
{
	UNUSED(core_num);
	cpuid_set_error(ERR_NOT_IMP);
	return NULL;
}

int cpu_read_arm_register_32b(struct cpuid_driver_t* driver, reg_request_t request, uint32_t* result)
{
	UNUSED(driver);
	UNUSED(request);
	UNUSED(result);
	return cpuid_set_error(ERR_NOT_IMP);
}

int cpu_read_arm_register_64b(struct cpuid_driver_t* driver, reg_request_t request, uint64_t* result)
{
	UNUSED(driver);
	UNUSED(request);
	UNUSED(result);
	return cpuid_set_error(ERR_NOT_IMP);
}

int cpu_cpuid_driver_close(struct cpuid_driver_t* driver)
{
	UNUSED(driver);
	return cpuid_set_error(ERR_NOT_IMP);
}

#endif /* Unsupported OS */
