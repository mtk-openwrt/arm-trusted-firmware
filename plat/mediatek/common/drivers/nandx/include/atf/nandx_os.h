/*
 * Copyright (C) 2017 MediaTek Inc.
 * Licensed under either
 *     BSD Licence, (see NOTICE for more details)
 *     GNU General Public License, version 2.0, (see NOTICE for more details)
 */

#ifndef __NANDX_OS_H__
#define __NANDX_OS_H__

#include <lib/mmio.h>
#include <common/debug.h>
#include <string.h>

#define NANDX_BULK_IO_USE_DRAM 1
#define NANDX_GLOBAL_VARIBLE 1

#define pr_err(fmt, ...) \
	ERROR("%s: " fmt, __func__, ##__VA_ARGS__)
#define pr_warn(fmt, ...) \
	WARN("%s: " fmt, __func__, ##__VA_ARGS__)
#define pr_info(fmt, ...) \
	INFO(fmt, ##__VA_ARGS__)
#define pr_debug(fmt, ...) \
	VERBOSE(fmt, ##__VA_ARGS__)

#define writel(value, reg) mmio_write_32((uintptr_t)reg, (uint32_t)value)
#define writew(value, reg) mmio_write_16((uintptr_t)reg, (uint32_t)value)
#define writeb(value, reg) mmio_write_8((uintptr_t)reg, (uint32_t)value)
#define readl(reg) mmio_read_32((uintptr_t)reg)
#define readw(reg) mmio_read_16((uintptr_t)reg)
#define readb(reg) mmio_read_8((uintptr_t)reg)


#define nandx_event_create()     NULL
#define nandx_event_destroy(event)
#define nandx_event_complete(event)
#define nandx_event_init(event)
#define nandx_event_wait_complete(event, timeout)        true

static inline int nandx_irq_register(void *dev, int irq, void *irq_handler,
		void *name, void *data)
{
	return 0;
}

static inline u64 nandx_dma_map(void *dev, void *buf, u64 len,
				enum nand_dma_operation op)
{
	u32 addr;

	addr = (u64)buf;

	return addr;
}

static inline void nandx_dma_unmap(void *dev, void *buf, void *addr,
				   u64 len, enum nand_dma_operation op)
{
	return;
}

#define NANDX_NFI_BUF_ADDR	0x4e200000
#define NANDX_UT_SRC_ADDR	0x4e300000
#define NANDX_UT_DST_ADDR	0x4e400000
#define NANDX_CORE_BUF_ADDR	0x4e500000

#endif /* __NANDX_OS_H__ */
