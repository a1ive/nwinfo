// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <winioctl.h>

void ObtainPrivileges(LPCTSTR privilege) {
	HANDLE hToken;
	TOKEN_PRIVILEGES tkp = { 0 };
	BOOL res;
	DWORD error;
	// Obtain required privileges
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
		printf("OpenProcessToken failed!\n");
		error = GetLastError();
		exit(error);
	}

	res = LookupPrivilegeValue(NULL, privilege, &tkp.Privileges[0].Luid);
	if (!res) {
		printf("LookupPrivilegeValue failed!\n");
		error = GetLastError();
		exit(error);
	}
	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);

	error = GetLastError();
	if (error != ERROR_SUCCESS) {
		printf("AdjustTokenPrivileges failed\n");
		exit(error);
	}

}

const char* GetHumanSize(UINT64 size, const char* human_sizes[6], UINT64 base)
{
    UINT64 fsize = size, frac = 0;
    unsigned units = 0;
    static char buf[48];
    const char* umsg;

    while (fsize >= base && units < 5)
    {
        frac = fsize % base;
        fsize = fsize / base;
        units++;
    }

    umsg = human_sizes[units];

    if (units)
    {
        if (frac)
            frac = frac * 100 / base;
        snprintf(buf, sizeof(buf), "%llu.%02llu %s", fsize, frac, umsg);
    }
    else
        snprintf(buf, sizeof(buf), "%llu %s", size, umsg);
    return buf;
}
