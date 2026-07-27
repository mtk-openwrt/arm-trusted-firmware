// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2026, MediaTek Inc. All rights reserved.
 *
 * Optional MMU/D-cache support for BL2. Enabled by the build option
 * MTK_BL2_ENABLE_DCACHE=1. Speeds up flash boot flows whose CPU-side
 * cost dominates (UBI attach/scan, NMBM scan, CRC checks, memcpy from
 * the SPI DMA bounce buffer, XZ decompression).
 */

#include <stdbool.h>

#include <arch_helpers.h>
#include <common/bl_common.h>
#include <common/debug.h>
#include <lib/utils_def.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <platform_def.h>

#include "bl2_plat_setup.h"

IMPORT_SYM(uintptr_t, __STACKS_START__, BL2_STACKS_START);
IMPORT_SYM(uintptr_t, __STACKS_END__, BL2_STACKS_END);

static bool bl2_dcache_enabled;

void mtk_bl2_enable_dcache(size_t dram_size)
{
	uintptr_t sram_end = L2_SRAM_BASE + L2_SRAM_SIZE;
	uintptr_t dev_end = MTK_DEV_BASE + MTK_DEV_SIZE;
	uintptr_t qspi_buf_end = QSPI_BUF_OFFSET + QSPI_BUF_SIZE;
	uintptr_t dram_end = DRAM_BASE + dram_size;

	if (!dram_size) {
		WARN("BL2: DRAM size unknown, D-cache stays off\n");
		return;
	}

	/* BL2 code and RO data, executable */
	mmap_add_region(BL_CODE_BASE, BL_CODE_BASE,
			BL_CODE_END - BL_CODE_BASE,
			MT_CODE | MT_SECURE);

	/* L2 SRAM below BL2 (BL2PL / image header area) */
	if (BL_CODE_BASE > L2_SRAM_BASE)
		mmap_add_region(L2_SRAM_BASE, L2_SRAM_BASE,
				BL_CODE_BASE - L2_SRAM_BASE,
				MT_MEMORY | MT_RW | MT_EXECUTE_NEVER |
				MT_SECURE);

	/* Rest of L2 SRAM: BL2 data, BSS, stacks, xlat tables */
	mmap_add_region(BL_CODE_END, BL_CODE_END, sram_end - BL_CODE_END,
			MT_MEMORY | MT_RW | MT_EXECUTE_NEVER | MT_SECURE);

	/* Peripheral registers */
	mmap_add_region(sram_end, sram_end, dev_end - sram_end,
			MT_DEVICE | MT_RW | MT_SECURE);

	/* DRAM below the SPI DMA bounce buffer */
	mmap_add_region(DRAM_BASE, DRAM_BASE, QSPI_BUF_OFFSET - DRAM_BASE,
			MT_MEMORY | MT_RW | MT_EXECUTE_NEVER | MT_SECURE);

	/*
	 * The mtk-qspi/mtk-snand drivers DMA into this bounce buffer
	 * without any cache maintenance, so it must stay non-cacheable.
	 */
	mmap_add_region(QSPI_BUF_OFFSET, QSPI_BUF_OFFSET, QSPI_BUF_SIZE,
			MT_NON_CACHEABLE | MT_RW | MT_EXECUTE_NEVER |
			MT_SECURE);

	/* Remaining DRAM: UBI/NMBM scratch, decompression buffers, images */
	mmap_add_region(qspi_buf_end, qspi_buf_end, dram_end - qspi_buf_end,
			MT_MEMORY | MT_RW | MT_EXECUTE_NEVER | MT_SECURE);

	init_xlat_tables();
	enable_mmu_el3(0);

	bl2_dcache_enabled = true;

	NOTICE("BL2: MMU and D-cache enabled\n");
}

void mtk_bl2_exit_dcache_maintenance(void)
{
	if (!bl2_dcache_enabled)
		return;

	/*
	 * bl2_run_next_image() has already turned the MMU and D-cache off
	 * at this point. The current stack frames were written with the
	 * cache disabled, so stale dirty lines covering the stack must be
	 * discarded (not cleaned) first, otherwise the full-cache clean
	 * below would write old data over them.
	 */
	inv_dcache_range(BL2_STACKS_START,
			 BL2_STACKS_END - BL2_STACKS_START);

	/* Flush everything else left behind from the cached phase */
	dcsw_op_all(DCCISW);

	bl2_dcache_enabled = false;
}
