// SPDX-License-Identifier: Unlicense

#include "shmem.h"

int WR0_OpenShMem(struct wr0_shmem_t* shmem, LPCWSTR name)
{
	ZeroMemory(shmem, sizeof(struct wr0_shmem_t));
	shmem->file = OpenFileMappingW(FILE_MAP_READ, FALSE, name);
	if (!shmem->file)
		goto fail;
	shmem->addr = MapViewOfFile(shmem->file, FILE_MAP_READ, 0, 0, 0);
	if (!shmem->addr)
		goto fail;
	shmem->mbi_size = VirtualQuery(shmem->addr, &shmem->mbi, sizeof(MEMORY_BASIC_INFORMATION));
	if (shmem->mbi_size == 0)
		goto fail;
	shmem->size = shmem->mbi.RegionSize;
	return 0;

fail:
	if (shmem->addr)
		UnmapViewOfFile(shmem->addr);
	if (shmem->file)
		CloseHandle(shmem->file);
	ZeroMemory(shmem, sizeof(struct wr0_shmem_t));
	return -1;
}

SIZE_T WR0_ReadShMem(struct wr0_shmem_t* shmem, SIZE_T offset, PVOID buffer, SIZE_T size)
{
	if (!shmem->addr)
		return 0;
	if (offset + size > shmem->size)
		size = shmem->size - offset;
	CopyMemory(buffer, (PBYTE)shmem->addr + offset, size);
	return size;
}

void WR0_CloseShMem(struct wr0_shmem_t* shmem)
{
	if (shmem->addr)
		UnmapViewOfFile(shmem->addr);
	if (shmem->file)
		CloseHandle(shmem->file);
	ZeroMemory(shmem, sizeof(struct wr0_shmem_t));
}
