#ifndef _DRAMC_PI_API_H
#define _DRAMC_PI_API_H

#include <drivers/delay_timer.h>
#include "emi.h"

#define CEIL_A_OVER_B(_A, _B)			(((_A) - (_B) * ((_A) / (_B))) > 0 ? (_A) / (_B) + 1 : (_A) / (_B))
#define mcSET_MASK(a, b)			((a) |= (b))
#define mcCLR_MASK(a, b)			((a) &= (~(b)))
#define mcSET_BIT(a, b)				mcSET_MASK(a, BIT(b))
#define mcCLR_BIT(a, b)				mcCLR_MASK(a, BIT(b))
#define mcCHK_BIT1(a, b)			((a) & BIT(b))
#define mcSET_FIELD0(var, value, mask, pos)	mcSET_MASK(var, ((value) << (pos)) & (mask))
#define mcSET_FIELD(var, value, mask, pos)						\
						{						\
						mcCLR_MASK(var, mask);			\
						mcSET_FIELD0(var, value, mask, pos);	\
						}

#define mdelay_dram(x)			mdelay(x)

/* Do Write Leveling with all DQS together */
#define fcWL_ALL
/* MEMPLL reference clock input config (from SYSPLL) */
#define fcMEMPLL_52M_REF_IN
/* MEMPLL BW config */
#define fcWAVEFORM_MEASURE

/* initialization */
/* pattern0 and base address for test engine when we do calibration */
#define DEFAULT_TEST2_1_CAL		0x55000000
/* for testing, to separate TA4-3 address for running simultaneously */
//#define DEFAULT_TEST2_1_CAL		0x55010000
/* pattern1 and offset address for test engine when we do calibraion */
#define DEFAULT_TEST2_2_CAL		0xaa000400
/* pattern0 and base address for test engine when we do dqs gating window */
#define DEFAULT_TEST2_1_DQSIEN		0x55000000
/* pattern1 and offset address for test engine when we do dqs gating window */
#define DEFAULT_TEST2_2_DQSIEN		0xaa000010
/* gold pattern for dqsien compare */
#define DEFAULT_GOLD_DQSIEN		0x20202020
#define DEFAULT_MR1_VALUE_DDR3		0x00002004
#define DEFAULT_MR2_VALUE_LP3		0x001c0002

/* timeout for TE2: (CMP_CPT_POLLING_PERIOD X MAX_CMP_CPT_WAIT_LOOP) */
//#define CMP_CPT_POLLING_PERIOD	1000
#define CMP_CPT_POLLING_PERIOD		10
#define MAX_CMP_CPT_WAIT_LOOP		10000

/** gating window **/
/* to filter fake passed window */
#define fcNEW_GATING_FINETUNE_LIMIT
#if defined(fcNEW_GATING_FINETUNE_LIMIT) && defined(DDR_FT_LOAD_BOARD)
/* improved version for very long fake passed window in MT6595 FT (only for FT) */
#define fcNEW_GATING_FINETUNE_LIMIT_2
#endif

#define DQS_GW_COARSE_START		0//10//22//18
#define DQS_GW_COARSE_END		30//32//29
#define DQS_GW_COARSE_STEP		1
#define DQS_GW_COARSE_MAX		((DQS_GW_COARSE_END - DQS_GW_COARSE_START) / DQS_GW_COARSE_STEP + 1)
#define DQS_GW_FINE_START 		0
#define DQS_GW_FINE_END 		127
#define DQS_GW_FINE_STEP 		4 //4
#define DQS_GW_FINE_MAX 		((DQS_GW_FINE_END - DQS_GW_FINE_START) / DQS_GW_FINE_STEP + 1)
/* depend on dqs_gw[] type. uint32_t currently */
#define DQS_GW_LEN_PER_COARSE_ELEMENT	64//64 //32
/* dqs_gw[] length per coarse tune. each bit represents one test result */
#define DQS_GW_LEN_PER_COARSE		(CEIL_A_OVER_B(DQS_GW_FINE_MAX, DQS_GW_LEN_PER_COARSE_ELEMENT))
/* dqs_gw[] total length */
#define DQS_GW_LEN			(DQS_GW_COARSE_MAX * DQS_GW_LEN_PER_COARSE)
#define DQS_GW_TE_OFFSET		0x10
/* 16 * 16 / 4 = 64 */
#define DQS_GW_GOLD_COUNTER_16BIT	0x40404040
#define DQS_GW_GOLD_COUNTER_32BIT	0x20202020

/* define max support bus width in the system (to allocate array size) */
#define DQ_DATA_WIDTH			16
#define DQS_BIT_NUMBER			8
#define DQS_NUMBER			(DQ_DATA_WIDTH / DQS_BIT_NUMBER)

/* RX DQ/DQS */
/* 0x018, May set back to 64 if no need */
#define MAX_RX_DQSDLY_TAPS		64
/* 0x210~0x22c, 0~15 delay tap */
#define MAX_RX_DQDLY_TAPS		16

/* DATLAT[4:0] = {0xf0[25] 0x0e4[4] 0x07c[6:4]} */
#define DATLAT_TAP_NUMBER		22

/* TX DQ/DQS */
/* first step to DQ delay */
#define FIRST_TX_DQ_DELAY		0
/* first step to DQS delay */
#define FIRST_TX_DQS_DELAY		0
/* max DQ TAP number */
#define MAX_TX_DQDLY_TAPS		16
/* max DQS TAP number */
#define MAX_TX_DQSDLY_TAPS		16

#define ENABLE				1
#define DISABLE				0

enum dram_status {
	DRAM_OK,
	DRAM_FAIL
};

enum dram_channel {
	CHANNEL_A,
	CHANNEL_B,
	CHANNEL_C,
};

enum dram_type {
	TYPE_DDR = 1,
	TYPE_LPDDR2,
	TYPE_LPDDR3,
	TYPE_PCDDR2,
	TYPE_PCDDR3
};

enum dram_data_width {
	DATA_WIDTH_16BIT = 16,
	DATA_WIDTH_32BIT = 32
};

enum dram_package {
	PACKAGE_SBS,
	PACKAGE_POP
};

enum dram_te_op {
	TE_OP_WRITE_READ_CHECK,
	TE_OP_READ_CHECK
};

enum dram_test_pattern {
	TEST_ISI_PATTERN,
	TEST_AUDIO_PATTERN,
	TEST_TA1_SIMPLE,
	TEST_TESTPAT4,
	TEST_TESTPAT4_3,
	TEST_XTALK_PATTERN,
	TEST_MIX_PATTERN
};

struct dramc_ctx_t {
	enum dram_channel channel;
	enum dram_type dram_type;
	enum dram_package package;
	enum dram_data_width data_width;
	uint32_t test2_1;
	uint32_t test2_2;
	enum dram_test_pattern test_pattern;
	uint16_t frequency;
	uint16_t frequency_low;
	uint8_t fglow_freq_write_en;
	uint8_t ssc_en;
	uint8_t en_4bitMux;
};

struct rxdqs_perbit_dly {
	int8_t first_dqdly_pass;
	int8_t last_dqdly_pass;
	int8_t first_dqsdly_pass;
	int8_t last_dqsdly_pass;
	int8_t best_first_dqdly_pass;
	int8_t best_last_dqdly_pass;
	int8_t best_first_dqsdly_pass;
	int8_t best_last_dqsdly_pass;
	uint8_t best_dqdly;
	uint8_t best_dqsdly;
};

struct txdqs_perbit_dly {
	int8_t first_dqdly_pass;
	int8_t last_dqdly_pass;
	int8_t first_dqsdly_pass;
	int8_t last_dqsdly_pass;
	int8_t best_first_dqdly_pass;
	int8_t best_last_dqdly_pass;
	int8_t best_first_dqsdly_pass;
	int8_t best_last_dqsdly_pass;
	uint8_t best_dqdly;
	uint8_t best_dqsdly;
};

void dle_factor_handler(struct dramc_ctx_t *p, uint8_t curr_val);
uint32_t dramc_engine1(struct dramc_ctx_t *p, uint32_t test2_1,
		       uint32_t test2_2, int16_t loopforever, uint8_t period);
uint32_t dramc_engine2(struct dramc_ctx_t *p, enum dram_te_op wr,
		       uint32_t test2_1, uint32_t test2_2, uint8_t testaudpat,
		       int16_t loopforever, uint8_t period,
		       uint8_t log2loopcount);
void dramc_phy_reset(struct dramc_ctx_t *p);
enum dram_status dramc_rxdqs_gating_cal(struct dramc_ctx_t *p);

void dram_reg_write(uint32_t u4reg_addr, uint32_t u4reg_value);
void dram_reg_read(uint32_t u4reg_addr, uint32_t *pu4reg_value);
void do_calib(struct dramc_ctx_t *p, struct emi_settings *emi_set);
#endif /* _PI_API_H */
