// SPDX-License-Identifier: Unlicense
#pragma once


#define VC_EXTRALEAN
#include <windows.h>

struct wr0_shmem_t
{
	HANDLE file;
	LPVOID addr;
	SIZE_T size;
	MEMORY_BASIC_INFORMATION mbi;
	SIZE_T mbi_size;
};

int WR0_OpenShMem(struct wr0_shmem_t* shmem, LPCWSTR name);

SIZE_T WR0_ReadShMem(struct wr0_shmem_t* shmem, SIZE_T offset, PVOID buffer, SIZE_T size);

void WR0_CloseShMem(struct wr0_shmem_t* shmem);
