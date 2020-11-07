/*
 * Copyright (C) 2017 MediaTek Inc.
 * Licensed under either
 *     BSD Licence, (see NOTICE for more details)
 *     GNU General Public License, version 2.0, (see NOTICE for more details)
 */

#include "nandx_util.h"
#include "nandx_core.h"
#include "nfiecc_regs.h"
#include "nfiecc.h"

#define NFIECC_IDLE_REG(op) \
	((op) == ECC_ENCODE ? NFIECC_ENCIDLE : NFIECC_DECIDLE)
#define         IDLE_MASK       1
#define NFIECC_CTL_REG(op) \
	((op) == ECC_ENCODE ? NFIECC_ENCCON : NFIECC_DECCON)
#define NFIECC_IRQ_REG(op) \
	((op) == ECC_ENCODE ? NFIECC_ENCIRQEN : NFIECC_DECIRQEN)
#define NFIECC_ADDR(op) \
	((op) == ECC_ENCODE ? NFIECC_ENCDIADDR : NFIECC_DECDIADDR)

#define ECC_TIMEOUT     500000

#if NANDX_GLOBAL_VARIBLE
struct nfiecc g_nfiecc;
#endif

/* ecc strength that each IP supports */
static const int ecc_strength_mt7622[] = {
	4, 6, 8, 10, 12, 14, 16
};

static int nfiecc_irq_handler(void *data)
{
	struct nfiecc *ecc = data;
	void *regs = ecc->res.regs;
	u32 status;

	status = readl(regs + NFIECC_DECIRQSTA) & DEC_IRQSTA_GEN;
	if (status) {
		status = readl(regs + NFIECC_DECDONE);
		if (!(status & ecc->config.sectors))
			return NAND_IRQ_NONE;

		/*
		 * Clear decode IRQ status once again to ensure that
		 * there will be no extra IRQ.
		 */
		readl(regs + NFIECC_DECIRQSTA);
		ecc->config.sectors = 0;
		nandx_event_complete(ecc->done);
	} else {
		status = readl(regs + NFIECC_ENCIRQSTA) & ENC_IRQSTA_GEN;
		if (!status)
			return NAND_IRQ_NONE;

		nandx_event_complete(ecc->done);
	}

	return NAND_IRQ_HANDLED;
}

static inline int nfiecc_wait_idle(struct nfiecc *ecc)
{
	int op = ecc->config.op;
	int ret, val;

	ret = readl_poll_timeout_atomic(ecc->res.regs + NFIECC_IDLE_REG(op),
					val, val & IDLE_MASK,
					10, ECC_TIMEOUT);
	if (ret)
		pr_info("%s not idle\n",
			op == ECC_ENCODE ? "encoder" : "decoder");

	return ret;
}

static int nfiecc_wait_encode_done(struct nfiecc *ecc)
{
	int ret, val;

	if (ecc->ecc_irq_en) {
		/* poll one time to avoid missing irq event */
		ret = readl_poll_timeout_atomic(ecc->res.regs + NFIECC_ENCSTA,
						val, val & ENC_FSM_IDLE, 1, 1);
		if (!ret)
			return 0;

		/* irq done, if not, we can go on to poll status for a while */
		ret = nandx_event_wait_complete(ecc->done, ECC_TIMEOUT);
		if (ret)
			return 0;
	}

	ret = readl_poll_timeout_atomic(ecc->res.regs + NFIECC_ENCSTA,
					val, val & ENC_FSM_IDLE,
					10, ECC_TIMEOUT);
	if (ret)
		pr_info("encode timeout\n");

	return ret;

}

static int nfiecc_wait_decode_done(struct nfiecc *ecc)
{
	u32 secbit = BIT(ecc->config.sectors - 1);
	void *regs = ecc->res.regs;
	int ret, val;

	if (ecc->ecc_irq_en) {
		ret = readl_poll_timeout_atomic(regs + NFIECC_DECDONE,
						val, val & secbit, 1, 1);
		if (!ret)
			return 0;

		ret = nandx_event_wait_complete(ecc->done, ECC_TIMEOUT);
		if (ret)
			return 0;
	}

	ret = readl_poll_timeout_atomic(regs + NFIECC_DECDONE,
					val, val & secbit,
					10, ECC_TIMEOUT);
	if (ret) {
		pr_info("decode timeout\n");
		return ret;
	}

	/* decode done does not stands for ecc all work done.
	 * we need check syn, bma, chien, autoc all idle.
	 * just check it when ECC_DECCNFG[13:12] is 3,
	 * which means auto correct.
	 */
	ret = readl_poll_timeout_atomic(regs + NFIECC_DECFSM,
					val, (val & FSM_MASK) == FSM_IDLE,
					10, ECC_TIMEOUT);
	if (ret)
		pr_info("decode fsm(0x%x) is not idle\n",
		       readl(regs + NFIECC_DECFSM));

	return ret;
}

static int nfiecc_wait_done(struct nfiecc *ecc)
{
	if (ecc->config.op == ECC_ENCODE)
		return nfiecc_wait_encode_done(ecc);

	return nfiecc_wait_decode_done(ecc);
}

static void nfiecc_encode_config(struct nfiecc *ecc, u32 ecc_idx)
{
	struct nfiecc_config *config = &ecc->config;
	u32 val;

	val = ecc_idx | (config->mode << ecc->caps->ecc_mode_shift);

	if (config->mode == ECC_DMA_MODE)
		val |= ENC_BURST_EN;

	val |= (config->len << 3) << ENCCNFG_MS_SHIFT;
	writel(val, ecc->res.regs + NFIECC_ENCCNFG);
}

static void nfiecc_decode_config(struct nfiecc *ecc, u32 ecc_idx)
{
	struct nfiecc_config *config = &ecc->config;
	u32 dec_sz = (config->len << 3) +
		     config->strength * ecc->caps->parity_bits;
	u32 val;

	val = ecc_idx | (config->mode << ecc->caps->ecc_mode_shift);

	if (config->mode == ECC_DMA_MODE)
		val |= DEC_BURST_EN;

	val |= (dec_sz << DECCNFG_MS_SHIFT) |
	       (config->deccon << DEC_CON_SHIFT);
	val |= DEC_EMPTY_EN;
	writel(val, ecc->res.regs + NFIECC_DECCNFG);
}

static void nfiecc_config(struct nfiecc *ecc)
{
	u32 idx;

	for (idx = 0; idx < ecc->caps->ecc_strength_num; idx++) {
		if (ecc->config.strength == ecc->caps->ecc_strength[idx])
			break;
	}

	if (ecc->config.op == ECC_ENCODE)
		nfiecc_encode_config(ecc, idx);
	else
		nfiecc_decode_config(ecc, idx);
}

static int nfiecc_enable(struct nfiecc *ecc)
{
	enum nfiecc_operation op = ecc->config.op;
	void *regs = ecc->res.regs;

	nfiecc_config(ecc);

	writel(ECC_OP_EN, regs + NFIECC_CTL_REG(op));

	if (ecc->ecc_irq_en) {
		writel(ECC_IRQEN, regs + NFIECC_IRQ_REG(op));

		if (ecc->page_irq_en)
			writel(ECC_IRQEN | ECC_PG_IRQ_SEL,
			       regs + NFIECC_IRQ_REG(op));

		nandx_event_init(ecc->done);
	}

	return 0;
}

static int nfiecc_disable(struct nfiecc *ecc)
{
	enum nfiecc_operation op = ecc->config.op;
	void *regs = ecc->res.regs;

	nfiecc_wait_idle(ecc);

	writel(0, regs + NFIECC_IRQ_REG(op));
	writel(~ECC_OP_EN, regs + NFIECC_CTL_REG(op));

	return 0;
}

static int nfiecc_correct_data(struct nfiecc *ecc,
			       struct nfiecc_status *status,
			       u8 *data, u32 sector)
{
	u32 err, offset, i;
	u32 loc, byteloc, bitloc;

	status->corrected = 0;
	status->failed = 0;

	offset = (sector >> 2);
	err = readl(ecc->res.regs + NFIECC_DECENUM(offset));
	err >>= (sector % 4) * 8;
	err &= ecc->caps->err_mask;

	if (err == ecc->caps->err_mask) {
		status->failed++;
		return -ENANDREAD;
	}

	status->corrected += err;
	status->bitflips = max_t(u32, status->bitflips, err);

	for (i = 0; i < err; i++) {
		loc = readl(ecc->res.regs + NFIECC_DECEL(i >> 1));
		loc >>= ((i & 0x1) << 4);
		byteloc = loc >> 3;
		bitloc = loc & 0x7;
		data[byteloc] ^= (1 << bitloc);
	}

	return 0;
}

static int nfiecc_fill_data(struct nfiecc *ecc, u8 *data)
{
	struct nfiecc_config *config = &ecc->config;
	void *regs = ecc->res.regs;
	int size, ret, i;
	u32 val;

	if (config->mode == ECC_DMA_MODE) {
		if ((unsigned long)config->dma_addr & 0x3)
			pr_info("encode address is not 4B aligned: 0x%x\n",
			       (u32)(unsigned long)config->dma_addr);

		writel((unsigned long)config->dma_addr,
		       regs + NFIECC_ADDR(config->op));
	} else if (config->mode == ECC_PIO_MODE) {
		if (config->op == ECC_ENCODE) {
			size = (config->len + 3) >> 2;
		} else {
			size = config->strength * ecc->caps->parity_bits;
			size = (size + 7) >> 3;
			size += config->len;
			size >>= 2;
		}

		for (i = 0; i < size; i++) {
			ret = readl_poll_timeout_atomic(regs + NFIECC_PIO_DIRDY,
							val, val & PIO_DI_RDY,
							10, ECC_TIMEOUT);
			if (ret)
				return ret;

			writel(*((u32 *)data + i), regs + NFIECC_PIO_DI);
		}
	}

	return 0;
}

static int nfiecc_encode(struct nfiecc *ecc, u8 *data)
{
	struct nfiecc_config *config = &ecc->config;
	u32 len, i, val = 0;
	u8 *p;
	int ret;

	/* Under NFI mode, nothing need to do */
	if (config->mode == ECC_NFI_MODE)
		return 0;

	ret = nfiecc_fill_data(ecc, data);
	if (ret)
		return ret;

	ret = nfiecc_wait_encode_done(ecc);
	if (ret)
		return ret;

	ret = nfiecc_wait_idle(ecc);
	if (ret)
		return ret;

	/* Program ECC bytes to OOB: per sector oob = FDM + ECC + SPARE */
	len = (config->strength * ecc->caps->parity_bits + 7) >> 3;
	p = data + config->len;

	/* Write the parity bytes generated by the ECC back to the OOB region */
	for (i = 0; i < len; i++) {
		if ((i % 4) == 0)
			val = readl(ecc->res.regs + NFIECC_ENCPAR(i / 4));

		p[i] = (val >> ((i % 4) * 8)) & 0xff;
	}

	return 0;
}

static int nfiecc_decode(struct nfiecc *ecc, u8 *data)
{
	int ret;

	/* Under NFI mode, nothing need to do */
	if (ecc->config.mode == ECC_NFI_MODE)
		return 0;

	ret = nfiecc_fill_data(ecc, data);
	if (ret)
		return ret;

	return nfiecc_wait_decode_done(ecc);
}

static int nfiecc_decode_status(struct nfiecc *ecc, u32 start_sector,
				u32 sectors)
{
	void *regs = ecc->res.regs;
	u32 i, val = 0, err;
	u32 bitflips = 0;

	for (i = start_sector; i < start_sector + sectors; i++) {
		if ((i % 4) == 0)
			val = readl(regs + NFIECC_DECENUM(i / 4));

		err = val >> ((i % 4) * 5);
		err &= ecc->caps->err_mask;

		if (err == ecc->caps->err_mask)
			pr_err("sector %d is uncorrect\n", i);

		bitflips = max_t(u32, bitflips, err);
	}

	if (bitflips == ecc->caps->err_mask)
		return -ENANDREAD;

	if (bitflips)
		pr_info("bitflips %d is corrected\n", bitflips);

	return bitflips;
}

static int nfiecc_adjust_strength(struct nfiecc *ecc, int strength)
{
	struct nfiecc_caps *caps = ecc->caps;
	int i, count = caps->ecc_strength_num;

	if (strength >= caps->ecc_strength[count - 1])
		return caps->ecc_strength[count - 1];

	if (strength < caps->ecc_strength[0])
		return -EINVAL;

	for (i = 1; i < count; i++) {
		if (strength < caps->ecc_strength[i])
			return caps->ecc_strength[i - 1];
	}

	return -EINVAL;
}

static int nfiecc_ctrl(struct nfiecc *ecc, int cmd, void *args)
{
	int ret = 0;

	switch (cmd) {
	case NFI_CTRL_ECC_IRQ:
		ecc->ecc_irq_en = *(bool *)args;
		break;

	case NFI_CTRL_ECC_PAGE_IRQ:
		ecc->page_irq_en = *(bool *)args;
		break;

	default:
		pr_info("invalid arguments.\n");
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int nfiecc_hw_init(struct nfiecc *ecc)
{
	int ret;

	ret = nfiecc_wait_idle(ecc);
	if (ret)
		return ret;

	writel(~ECC_OP_EN, ecc->res.regs + NFIECC_ENCCON);

	ret = nfiecc_wait_idle(ecc);
	if (ret)
		return ret;

	writel(~ECC_OP_EN, ecc->res.regs + NFIECC_DECCON);

	return 0;
}

static struct nfiecc_caps nfiecc_caps_mt7622 = {
	.err_mask = 0x1f,
	.ecc_mode_shift = 4,
	.parity_bits = 13,
	.ecc_strength = ecc_strength_mt7622,
	.ecc_strength_num = 7,
};

static struct nfiecc_caps *nfiecc_get_match_data(enum mtk_ic_version ic)
{
	/* NOTE: add other IC's data */
	return &nfiecc_caps_mt7622;
}

struct nfiecc *nfiecc_init(struct nfiecc_resource *res)
{
	struct nfiecc *ecc;
	int ret;

#if NANDX_GLOBAL_VARIBLE
	ecc = &g_nfiecc;
#else
	ecc = mem_alloc(1, sizeof(struct nfiecc));
	if (!ecc)
		return NULL;
#endif
	ecc->res = *res;

	ret = nandx_irq_register(res->dev, res->irq_id, nfiecc_irq_handler,
				 "mtk-ecc", ecc);
	if (ret) {
		pr_info("ecc irq register failed!\n");
		goto error;
	}

	ecc->ecc_irq_en = false;
	ecc->page_irq_en = false;
	ecc->done = nandx_event_create();
	ecc->caps = nfiecc_get_match_data(res->ic_ver);

	ecc->adjust_strength = nfiecc_adjust_strength;
	ecc->enable = nfiecc_enable;
	ecc->disable = nfiecc_disable;
	ecc->decode = nfiecc_decode;
	ecc->encode = nfiecc_encode;
	ecc->wait_done = nfiecc_wait_done;
	ecc->decode_status = nfiecc_decode_status;
	ecc->correct_data = nfiecc_correct_data;
	ecc->nfiecc_ctrl = nfiecc_ctrl;

	ret = nfiecc_hw_init(ecc);
	if (ret)
		return NULL;

	return ecc;

error:
#if !NANDX_GLOBAL_VARIBLE
	mem_free(ecc);
#endif

	return NULL;
}

void nfiecc_exit(struct nfiecc *ecc)
{
	nandx_event_destroy(ecc->done);
#if !NANDX_GLOBAL_VARIBLE
	mem_free(ecc);
#endif
}

