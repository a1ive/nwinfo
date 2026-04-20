// SPDX-License-Identifier: Unlicense
#pragma once

#include <stdint.h>
#include <stdbool.h>

uint64_t
mchbar_get_mmio_reg(void);

uint64_t
pch_get_mmio_reg(void);

uint32_t
mchbar_read_32(uint64_t offset);

uint32_t
pch_read_32(uint64_t offset);

uint64_t
mchbar_read_64(uint64_t offset);

bool
mchbar_pch_init(void);
