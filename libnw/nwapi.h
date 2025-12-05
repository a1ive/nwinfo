// SPDX-License-Identifier: Unlicense
#pragma once

#if defined(LIBNW_DLL)
#define LIBNW_API __declspec(dllexport)
#elif defined(LIBNW_USE_DLL)
#define LIBNW_API __declspec(dllimport)
#else
#define LIBNW_API
#endif
