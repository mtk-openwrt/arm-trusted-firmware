/*
 * Copyright (c) 2019, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <lib/mmio.h>
#include "timer.h"

static volatile unsigned int timestamp;
static volatile unsigned int lastinc;

//===========================================================================
// GPT4 fixed 13MHz counter
//===========================================================================
static void gpt4_start(void)
{
	mmio_write_32(GPT4_CLK, GPT4_CLK_SETTING);
	mmio_write_32(GPT4_CON, GPT4_EN | GPT4_FREERUN);
}

static void gpt4_stop(void)
{
	mmio_write_32(GPT4_CON, 0x0);		/* disable */
	mmio_write_32(GPT4_CON, 0x2);		/* clear counter */
}

static void gpt4_init()
{
	gpt4_stop();
	gpt4_start();
}

static unsigned int gpt4_tick2time_ms(unsigned int tick)
{
	return ((tick + (GPT4_1MS_TICK - 1)) / GPT4_1MS_TICK);
}

static void reset_timer_masked(void)
{
	lastinc = gpt4_tick2time_ms(mmio_read_32(GPT4_DAT));
	timestamp = 0;
}

static void reset_timer(void)
{
	reset_timer_masked();
}

static unsigned long get_timer_masked(void)
{
	volatile unsigned int now;

	now = gpt4_tick2time_ms(mmio_read_32(GPT4_DAT));

	if (now >= lastinc) {
		timestamp = timestamp + now - lastinc;		/* normal */
	} else {
		timestamp =
			timestamp + MAX_REG_MS - lastinc + now; /* overflow */
	}
	lastinc = now;

	return timestamp;
}


unsigned long get_timer(unsigned long base)
{
	unsigned long current_timestamp = 0;
	unsigned long temp = 0;

	current_timestamp = get_timer_masked();

	/* timestamp normal */
	if (current_timestamp >= base) {
		return (current_timestamp - base);
	}

	/* timestamp overflow */
	temp = base - current_timestamp;

	return (MAX_TIMESTAMP_MS - temp);
}

void mtk_timer_init(void)
{
	gpt4_init();
	reset_timer();
}
