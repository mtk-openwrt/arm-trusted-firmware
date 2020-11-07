/*
 * Copyright (c) 2019, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __MTK_TIMER_H__
#define __MTK_TIMER_H__

#define GPT_BASE	0x10004000
#define GPT4_CON	(GPT_BASE + 0x0040)
#define GPT4_CLK	(GPT_BASE + 0x0044)
#define GPT4_DAT	(GPT_BASE + 0x0048)

#define GPT4_EN		0x0001
#define GPT4_FREERUN	0x0030
#define GPT4_SYS_CLK	0x0000
#define GPT4_CLK_DIV1	0X0000
#define GPT4_CLK_DIV2	0X0001

#define GPT4_CLK_SETTING	(GPT4_SYS_CLK | GPT4_CLK_DIV1)
#define GPT4_MAX_MS_TIMEOUT	((unsigned int)357770)
#define GPT4_1MS_TICK		((unsigned int)12004)

#define MAX_REG_MS		GPT4_MAX_MS_TIMEOUT
#define MAX_TIMESTAMP_MS	0xffffffff

extern unsigned long get_timer(unsigned long base);
extern void mtk_timer_init(void);

#endif
