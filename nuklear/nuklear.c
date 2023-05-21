// SPDX-License-Identifier: Unlicense

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR

#pragma warning(disable:4996)
#pragma warning(disable:4116)

#include <assert.h>
#define NK_ASSERT(expr) assert(expr)

#define NK_IMPLEMENTATION
#include <nuklear.h>

#pragma warning(disable:4244)
#pragma comment(lib, "gdiplus.lib")

#define NK_GDIP_IMPLEMENTATION
#include <nuklear_gdip.h>
