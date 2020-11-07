/*
 * Copyright (C) 2017 MediaTek Inc.
 * Licensed under either
 *     BSD Licence, (see NOTICE for more details)
 *     GNU General Public License, version 2.0, (see NOTICE for more details)
 */

/**
 * nfi_base.c - the base logic for nfi to access nand flash
 *
 * slc/mlc/tlc could use same code to access nand
 * of cause, there still some work need to do
 * even for spi nand, there should be a chance to integrate code together
 */

#include "nandx_util.h"
#include "nandx_core.h"
#include "../nfi.h"
#include "../nand_device.h"
#include "nfi_regs.h"
#include "nfiecc.h"
#include "nfi_base.h"

#if NANDX_GLOBAL_VARIBLE
struct nfi_base g_nfi_base;
#endif

static const int spare_size_mt7622[] = {
	16, 26, 27, 28
};

#define RAND_SEED_SHIFT(op) \
	((op) == RAND_ENCODE ? ENCODE_SEED_SHIFT : DECODE_SEED_SHIFT)
#define RAND_EN(op) \
	((op) == RAND_ENCODE ? RAN_ENCODE_EN : RAN_DECODE_EN)

#define SS_SEED_NUM     128
static u16 ss_randomizer_seed[SS_SEED_NUM] = {
	0x576A, 0x05E8, 0x629D, 0x45A3, 0x649C, 0x4BF0, 0x2342, 0x272E,
	0x7358, 0x4FF3, 0x73EC, 0x5F70, 0x7A60, 0x1AD8, 0x3472, 0x3612,
	0x224F, 0x0454, 0x030E, 0x70A5, 0x7809, 0x2521, 0x484F, 0x5A2D,
	0x492A, 0x043D, 0x7F61, 0x3969, 0x517A, 0x3B42, 0x769D, 0x0647,
	0x7E2A, 0x1383, 0x49D9, 0x07B8, 0x2578, 0x4EEC, 0x4423, 0x352F,
	0x5B22, 0x72B9, 0x367B, 0x24B6, 0x7E8E, 0x2318, 0x6BD0, 0x5519,
	0x1783, 0x18A7, 0x7B6E, 0x7602, 0x4B7F, 0x3648, 0x2C53, 0x6B99,
	0x0C23, 0x67CF, 0x7E0E, 0x4D8C, 0x5079, 0x209D, 0x244A, 0x747B,
	0x350B, 0x0E4D, 0x7004, 0x6AC3, 0x7F3E, 0x21F5, 0x7A15, 0x2379,
	0x1517, 0x1ABA, 0x4E77, 0x15A1, 0x04FA, 0x2D61, 0x253A, 0x1302,
	0x1F63, 0x5AB3, 0x049A, 0x5AE8, 0x1CD7, 0x4A00, 0x30C8, 0x3247,
	0x729C, 0x5034, 0x2B0E, 0x57F2, 0x00E4, 0x575B, 0x6192, 0x38F8,
	0x2F6A, 0x0C14, 0x45FC, 0x41DF, 0x38DA, 0x7AE1, 0x7322, 0x62DF,
	0x5E39, 0x0E64, 0x6D85, 0x5951, 0x5937, 0x6281, 0x33A1, 0x6A32,
	0x3A5A, 0x2BAC, 0x743A, 0x5E74, 0x3B2E, 0x7EC7, 0x4FD2, 0x5D28,
	0x751F, 0x3EF8, 0x39B1, 0x4E49, 0x746B, 0x6EF6, 0x44BE, 0x6DB7
};

#if 0
static void dump_register(void *regs)
{
	int i;

	pr_info("registers:\n");
	for (i = 0; i < 0x600; i += 0x10) {
		pr_info("    address 0x%X : %X %X %X %X\n",
			(u32)((unsigned long)regs + i),
			(u32)readl(regs + i),
			(u32)readl(regs + i + 0x4),
			(u32)readl(regs + i + 0x8),
			(u32)readl(regs + i + 0xC));
	}
}
#endif

static int nfi_enable_randomizer(struct nfi *nfi, u32 row, bool encode)
{
	struct nfi_base *nb = nfi_to_base(nfi);
	enum randomizer_op op = RAND_ENCODE;
	void *regs = nb->res.nfi_regs;
	u32 val;

	if (!encode)
		op = RAND_DECODE;

	/* randomizer type and reseed type setup */
	val = readl(regs + NFI_CNFG);
	val |= CNFG_RAND_SEL | CNFG_RESEED_SEC_EN;
	writel(val, regs + NFI_CNFG);

	/* randomizer seed and type setup */
	val = ss_randomizer_seed[row % SS_SEED_NUM] & RAN_SEED_MASK;
	val <<= RAND_SEED_SHIFT(op);
	val |= RAND_EN(op);
	writel(val, regs + NFI_RANDOM_CNFG);

	return 0;
}

static int nfi_disable_randomizer(struct nfi *nfi)
{
	struct nfi_base *nb = nfi_to_base(nfi);

	writel(0, nb->res.nfi_regs + NFI_RANDOM_CNFG);

	return 0;
}

static int nfi_irq_handler(int irq, void *data)
{
	struct nfi_base *nb = (struct nfi_base *) data;
	void *regs = nb->res.nfi_regs;
	u16 status, en;

	status = readw(regs + NFI_INTR_STA);
	en = readw(regs + NFI_INTR_EN);

	if (!(status & en))
		return NAND_IRQ_NONE;

	writew(~status & en, regs + NFI_INTR_EN);

	nandx_event_complete(nb->done);

	return NAND_IRQ_HANDLED;
}

static int nfi_select_chip(struct nfi *nfi, int cs)
{
	struct nfi_base *nb = nfi_to_base(nfi);

	writel(cs, nb->res.nfi_regs + NFI_CSEL);

	return 0;
}

static inline void set_op_mode(void *regs, u32 mode)
{
	u32 val = readl(regs + NFI_CNFG);

	val &= ~CNFG_OP_MODE_MASK;
	val |= mode;

	writel(val, regs + NFI_CNFG);
}

static int nfi_reset(struct nfi *nfi)
{
	struct nfi_base *nb = nfi_to_base(nfi);
	void *regs = nb->res.nfi_regs;
	int ret, val;

	/* The NFI reset to reset all registers and force the NFI
	 * master be early terminated
	 */
	writel(CON_FIFO_FLUSH | CON_NFI_RST, regs + NFI_CON);

	/* check state of NFI internal FSM and NAND interface FSM */
	ret = readl_poll_timeout_atomic(regs + NFI_MASTER_STA, val,
					!(val & MASTER_BUS_BUSY),
					10, NFI_TIMEOUT);
	if (ret)
		pr_info("nfi reset timeout...\n");

	writel(CON_FIFO_FLUSH | CON_NFI_RST, regs + NFI_CON);
	writew(STAR_DE, regs + NFI_STRDATA);

	return ret;
}

static void bad_mark_swap(struct nfi *nfi, u8 *buf, u8 *fdm)
{
	struct nfi_base *nb = nfi_to_base(nfi);
	u32 start_sector = div_down(nb->col, nfi->sector_size);
	u32 data_mark_pos;
	u8 temp;

	/* raw access, no need to do swap. */
	if (!nb->ecc_en)
		return;

	if (!buf || !fdm)
		return;

	if (nb->bad_mark_ctrl.sector < start_sector ||
	    nb->bad_mark_ctrl.sector > start_sector + nb->rw_sectors)
		return;

	data_mark_pos = nb->bad_mark_ctrl.position +
			(nb->bad_mark_ctrl.sector - start_sector) *
			nfi->sector_size;

	temp = *fdm;
	*fdm = *(buf + data_mark_pos);
	*(buf + data_mark_pos) = temp;
}

static u8 *fdm_shift(struct nfi *nfi, u8 *fdm, int sector)
{
	struct nfi_base *nb = nfi_to_base(nfi);
	u8 *pos;

	if (!fdm)
		return NULL;

	/* map the sector's FDM data to free oob:
	 * the beginning of the oob area stores the FDM data of bad mark sectors
	 */
	if (sector < nb->bad_mark_ctrl.sector)
		pos = fdm + (sector + 1) * nfi->fdm_size;
	else if (sector == nb->bad_mark_ctrl.sector)
		pos = fdm;
	else
		pos = fdm + sector * nfi->fdm_size;

	return pos;

}

static void set_bad_mark_ctrl(struct nfi_base *nb)
{
	int temp, page_size = nb->format.page_size;

	nb->bad_mark_ctrl.bad_mark_swap = bad_mark_swap;
	nb->bad_mark_ctrl.fdm_shift = fdm_shift;

	temp = nb->nfi.sector_size + nb->nfi.sector_spare_size;
	nb->bad_mark_ctrl.sector = div_down(page_size, temp);
	nb->bad_mark_ctrl.position = reminder(page_size, temp);
}

/* NOTE: check if page_size valid future */
static int setup_format(struct nfi_base *nb, int spare_idx)
{
	struct nfi *nfi = &nb->nfi;
	u32 page_size = nb->format.page_size;
	u32 val;

	switch (page_size) {
	case 512:
		val = PAGEFMT_512_2K | PAGEFMT_SEC_SEL_512;
		break;

	case KB(2):
		if (nfi->sector_size == 512)
			val = PAGEFMT_2K_4K | PAGEFMT_SEC_SEL_512;
		else
			val = PAGEFMT_512_2K;

		break;

	case KB(4):
		if (nfi->sector_size == 512)
			val = PAGEFMT_4K_8K | PAGEFMT_SEC_SEL_512;
		else
			val = PAGEFMT_2K_4K;

		break;

	case KB(8):
		if (nfi->sector_size == 512)
			val = PAGEFMT_8K_16K | PAGEFMT_SEC_SEL_512;
		else
			val = PAGEFMT_4K_8K;

		break;

	case KB(16):
		val = PAGEFMT_8K_16K;
		break;

	default:
		pr_info("invalid page len: %d\n", page_size);
		return -EINVAL;
	}

	val |= spare_idx << PAGEFMT_SPARE_SHIFT;
	val |= nfi->fdm_size << PAGEFMT_FDM_SHIFT;
	val |= nfi->fdm_ecc_size << PAGEFMT_FDM_ECC_SHIFT;
	writel(val, nb->res.nfi_regs + NFI_PAGEFMT);

	if (nb->custom_sector_en) {
		val = nfi->sector_spare_size + nfi->sector_size;
		val |= SECCUS_SIZE_EN;
		writel(val, nb->res.nfi_regs + NFI_SECCUS_SIZE);
	}

	return 0;
}

static int adjust_spare(struct nfi_base *nb, int *spare)
{
	int multi = nb->nfi.sector_size == 512 ? 1 : 2;
	int i, count = nb->caps->spare_size_num;

	if (*spare >= nb->caps->spare_size[count - 1] * multi) {
		*spare = nb->caps->spare_size[count - 1] * multi;
		return count - 1;
	}

	if (*spare < nb->caps->spare_size[0] * multi)
		return -EINVAL;

	for (i = 1; i < count; i++) {
		if (*spare < nb->caps->spare_size[i] * multi) {
			*spare = nb->caps->spare_size[i - 1] * multi;
			return i - 1;
		}
	}

	return -EINVAL;
}

static int nfi_set_format(struct nfi *nfi, struct nfi_format *format)
{
	struct nfi_base *nb = nfi_to_base(nfi);
	struct nfiecc *ecc = nb->ecc;
	int ecc_strength = format->ecc_req;
	int min_fdm, min_ecc, max_ecc;
	u32 temp, page_sectors;
	int spare_idx = 0;

	if (!nb->buf) {
#if NANDX_BULK_IO_USE_DRAM
		nb->buf = (u8 *)NANDX_NFI_BUF_ADDR;
#else
		nb->buf = mem_alloc(1, format->page_size + format->spare_size);
#endif
		if (!nb->buf)
			return -ENOMEM;
	}

	nb->format = *format;

	/* ToBeFixed: for spi nand, now sector size is 512,
	 * it should be same with slc.
	 */
	nfi->sector_size = 512;
	/* format->ecc_req is the requirement per 1KB */
	ecc_strength >>= 1;

	page_sectors = div_down(format->page_size, nfi->sector_size);
	nfi->sector_spare_size = div_down(format->spare_size, page_sectors);

	if (!nb->custom_sector_en) {
		spare_idx = adjust_spare(nb, &nfi->sector_spare_size);
		if (spare_idx < 0)
			return -EINVAL;
	}

	/* calculate ecc strength and fdm size */
	temp = (nfi->sector_spare_size - nb->caps->max_fdm_size) * 8;
	min_ecc = div_down(temp, nb->caps->ecc_parity_bits);
	min_ecc = ecc->adjust_strength(ecc, min_ecc);
	if (min_ecc < 0)
		return -EINVAL;

	temp = div_up(nb->res.min_oob_req, page_sectors);
	temp = (nfi->sector_spare_size - temp) * 8;
	max_ecc = div_down(temp, nb->caps->ecc_parity_bits);
	max_ecc = ecc->adjust_strength(ecc, max_ecc);
	if (max_ecc < 0)
		return -EINVAL;

	temp = div_up(temp * nb->caps->ecc_parity_bits, 8);
	temp = nfi->sector_spare_size - temp;
	min_fdm = min_t(u32, temp, (u32)nb->caps->max_fdm_size);

	if (ecc_strength > max_ecc) {
		pr_info("required ecc strength %d, max supported %d\n",
			ecc_strength, max_ecc);
		nfi->ecc_strength = max_ecc;
		nfi->fdm_size = min_fdm;
	} else if (format->ecc_req < min_ecc) {
		nfi->ecc_strength = min_ecc;
		nfi->fdm_size = nb->caps->max_fdm_size;
	} else {
		ecc_strength = ecc->adjust_strength(ecc, ecc_strength);
		if (ecc_strength < 0)
			return -EINVAL;

		nfi->ecc_strength = ecc_strength;
		temp = div_up(ecc_strength * nb->caps->ecc_parity_bits, 8);
		nfi->fdm_size = nfi->sector_spare_size - temp;
	}

	nb->page_sectors = div_down(format->page_size, nfi->sector_size);

	/* some IC has fixed fdm_ecc_size, if not assigend, set to fdm_size */
	nfi->fdm_ecc_size = nb->caps->fdm_ecc_size ? : nfi->fdm_size;

	nfi->ecc_parity_size = div_up(nfi->ecc_strength *
				      nb->caps->ecc_parity_bits,
				      8);
	set_bad_mark_ctrl(nb);

	pr_debug("sector_size: %d\n", nfi->sector_size);
	pr_debug("sector_spare_size: %d\n", nfi->sector_spare_size);
	pr_debug("fdm_size: %d\n", nfi->fdm_size);
	pr_debug("fdm_ecc_size: %d\n", nfi->fdm_ecc_size);
	pr_debug("ecc_strength: %d\n", nfi->ecc_strength);
	pr_debug("ecc_parity_size: %d\n", nfi->ecc_parity_size);

	return setup_format(nb, spare_idx);
}

static int nfi_ctrl(struct nfi *nfi, int cmd, void *args)
{
	struct nfi_base *nb = nfi_to_base(nfi);
	int ret = 0;

	switch (cmd) {
	case NFI_CTRL_DMA:
		nb->dma_en = *(bool *)args;
		break;

	case NFI_CTRL_AUTOFORMAT:
		nb->auto_format = *(bool *)args;
		break;

	case NFI_CTRL_NFI_IRQ:
		nb->nfi_irq_en = *(bool *)args;
		break;

	case NFI_CTRL_PAGE_IRQ:
		nb->page_irq_en = *(bool *)args;
		break;

	case NFI_CTRL_BAD_MARK_SWAP:
		nb->bad_mark_swap_en = *(bool *)args;
		break;

	case NFI_CTRL_ECC:
		nb->ecc_en = *(bool *)args;
		break;

	case NFI_CTRL_ECC_MODE:
		nb->ecc_mode = *(enum nfiecc_mode *)args;
		break;

	case NFI_CTRL_ECC_CLOCK:
		/* NOTE: it seems that there's nothing need to do
		 * if new IC need, just add tht logic
		 */
		nb->ecc_clk_en = *(bool *)args;
		break;

	case NFI_CTRL_ECC_IRQ:
		nb->ecc_irq_en = *(bool *)args;
		break;

	case NFI_CTRL_ECC_DECODE_MODE:
		nb->ecc_deccon = *(enum nfiecc_deccon *)args;
		break;

	default:
		pr_info("invalid arguments.\n");
		ret = -EOPNOTSUPP;
		break;
	}

	pr_debug("%s: set cmd(%d) to %d\n", __func__, cmd, *(int *)args);
	return ret;
}

static int nfi_send_cmd(struct nfi *nfi, short cmd)
{
	struct nfi_base *nb = nfi_to_base(nfi);
	void *regs = nb->res.nfi_regs;
	int ret;
	u32 val;

	pr_debug("%s: cmd 0x%x\n", __func__, cmd);

	if (cmd < 0)
		return -EINVAL;

	set_op_mode(regs, nb->op_mode);

	writel(cmd, regs + NFI_CMD);

	ret = readl_poll_timeout_atomic(regs + NFI_STA,
					val, !(val & STA_CMD),
					5, NFI_TIMEOUT);
	if (ret)
		pr_info("send cmd 0x%x timeout\n", cmd);

	return ret;
}

static int nfi_send_addr(struct nfi *nfi, int col, int row,
			 int col_cycle, int row_cycle)
{
	struct nfi_base *nb = nfi_to_base(nfi);
	void *regs = nb->res.nfi_regs;
	int ret;
	u32 val;

	pr_debug("%s: col 0x%x, row 0x%x, col_cycle 0x%x, row_cycle 0x%x\n",
		 __func__, col, row, col_cycle, row_cycle);

	nb->col = col;
	nb->row = row;

	writel(col, regs + NFI_COLADDR);
	writel(row, regs + NFI_ROWADDR);
	writel(col_cycle | (row_cycle << ROW_SHIFT), regs + NFI_ADDRNOB);

	ret = readl_poll_timeout_atomic(regs + NFI_STA,
					val, !(val & STA_ADDR),
					5, NFI_TIMEOUT);
	if (ret)
		pr_info("send address timeout\n");

	return ret;
}

static int nfi_trigger(struct nfi *nfi)
{
	/* Nothing need to do. */
	return 0;
}

static inline int wait_io_ready(void *regs)
{
	u32 val;
	int ret;

	ret = readl_poll_timeout_atomic(regs + NFI_PIO_DIRDY,
					val, val & PIO_DI_RDY,
					2, NFI_TIMEOUT);
	if (ret)
		pr_info("wait io ready timeout\n");

	return ret;
}

static int wait_ready_irq(struct nfi_base *nb, u32 timeout)
{
	void *regs = nb->res.nfi_regs;
	int ret;
	u32 val;

	writel(0xf1, regs + NFI_CNRNB);
	nandx_event_init(nb->done);

	writel(INTR_BUSY_RETURN_EN, (void *)(regs + NFI_INTR_EN));

	/**
	 * check if nand already bean ready,
	 * avoid issue that casued by missing irq-event.
	 */
	val = readl(regs + NFI_STA);
	if (val & STA_BUSY2READY) {
		readl(regs + NFI_INTR_STA);
		writel(0, (void *)(regs + NFI_INTR_EN));
		return 0;
	}

	ret = nandx_event_wait_complete(nb->done, timeout);

	writew(0, regs + NFI_CNRNB);
	return ret;
}

static void wait_ready_twhr2(struct nfi_base *nb, u32 timeout)
{
	/* NOTE: this for tlc */
}

static int wait_ready_poll(struct nfi_base *nb, u32 timeout)
{
	void *regs = nb->res.nfi_regs;
	int ret;
	u32 val;

	writel(0x21, regs + NFI_CNRNB);
	ret = readl_poll_timeout_atomic(regs + NFI_STA, val,
					val & STA_BUSY2READY,
					2, timeout);
	writew(0, regs + NFI_CNRNB);

	return ret;
}

static int nfi_wait_ready(struct nfi *nfi, int type, u32 timeout)
{
	struct nfi_base *nb = nfi_to_base(nfi);
	int ret;

	switch (type) {
	case NAND_WAIT_IRQ:
		if (nb->nfi_irq_en)
			ret = wait_ready_irq(nb, timeout);
		else
			ret = -EINVAL;

		break;

	case NAND_WAIT_POLLING:
		ret = wait_ready_poll(nb, timeout);
		break;

	case NAND_WAIT_TWHR2:
		wait_ready_twhr2(nb, timeout);
		ret = 0;
		break;

	default:
		ret = -EINVAL;
		break;
	}

	if (ret)
		pr_info("%s: type 0x%x, timeout 0x%x\n",
		       __func__, type, timeout);

	return ret;
}

static int enable_ecc_decode(struct nfi_base *nb, int sectors)
{
	struct nfi *nfi = &nb->nfi;
	struct nfiecc *ecc = nb->ecc;

	ecc->config.op = ECC_DECODE;
	ecc->config.mode = nb->ecc_mode;
	ecc->config.deccon = nb->ecc_deccon;
	ecc->config.sectors = sectors;
	ecc->config.len = nfi->sector_size + nfi->fdm_ecc_size;
	ecc->config.strength = nfi->ecc_strength;

	return ecc->enable(ecc);
}

static int enable_ecc_encode(struct nfi_base *nb)
{
	struct nfiecc *ecc = nb->ecc;
	struct nfi *nfi = &nb->nfi;

	ecc->config.op = ECC_ENCODE;
	ecc->config.mode = nb->ecc_mode;
	ecc->config.len = nfi->sector_size + nfi->fdm_ecc_size;
	ecc->config.strength = nfi->ecc_strength;

	return ecc->enable(ecc);
}

static void read_fdm(struct nfi_base *nb, u8 *fdm, int start_sector,
		     int sectors)
{
	void *regs = nb->res.nfi_regs;
	int j, i = start_sector;
	u32 vall, valm;
	u8 *buf = fdm;

	for (; i < start_sector + sectors; i++) {
		if (nb->bad_mark_swap_en)
			buf = nb->bad_mark_ctrl.fdm_shift(&nb->nfi, fdm, i);

		vall = readl(regs + NFI_FDML(i));
		valm = readl(regs + NFI_FDMM(i));

		for (j = 0; j < nb->nfi.fdm_size; j++)
			*buf++ = (j >= 4 ? valm : vall) >> ((j & 3) << 3);
	}
}

static void write_fdm(struct nfi_base *nb, u8 *fdm)
{
	struct nfi *nfi = &nb->nfi;
	void *regs = nb->res.nfi_regs;
	u32 vall, valm;
	int i, j;
	u8 *buf = fdm;

	for (i = 0; i < nb->page_sectors; i++) {
		if (nb->bad_mark_swap_en)
			buf = nb->bad_mark_ctrl.fdm_shift(nfi, fdm, i);

		vall = 0;
		for (j = 0; j < 4; j++)
			vall |= (j < nfi->fdm_size ? *buf++ : 0xff) << (j * 8);
		writel(vall, regs + NFI_FDML(i));

		valm = 0;
		for (j = 0; j < 4; j++)
			valm |= (j < nfi->fdm_size ? *buf++ : 0xff) << (j * 8);
		writel(valm, regs + NFI_FDMM(i));
	}
}

/* NOTE: pio not use auto format */
static int pio_rx_data(struct nfi_base *nb, u8 *data, u8 *fdm,
		       int sectors)
{
	struct nfiecc_status ecc_status;
	struct nfi *nfi = &nb->nfi;
	void *regs = nb->res.nfi_regs;
	u32 val, bitflips = 0;
	int len, ret, i;
	u8 *buf;

	val = readl(regs + NFI_CNFG) | CNFG_BYTE_RW;
	writel(val, regs + NFI_CNFG);

	len = nfi->sector_size + nfi->sector_spare_size;
	len *= sectors;

	for (i = 0; i < len; i++) {
		ret = wait_io_ready(regs);
		if (ret)
			return ret;

		nb->buf[i] = readb(regs + NFI_DATAR);
	}

	/* TODO: do error handle for autoformat setting of pio */
	if (nb->ecc_en) {
		for (i = 0; i < sectors; i++) {
			buf = nb->buf + i * (nfi->sector_size +
					     nfi->sector_spare_size);
			ret = nb->ecc->correct_data(nb->ecc, &ecc_status,
						    buf, i);
			if (data)
				memcpy(data + i * nfi->sector_size,
				       buf, nfi->sector_size);
			if (fdm)
				memcpy(fdm + i * nfi->fdm_size,
				       buf + nfi->sector_size, nfi->fdm_size);
			if (ret) {
				ret = nb->ecc->decode_status(nb->ecc, i, 1);
				if (ret < 0)
					return ret;

				bitflips = max_t(int, (int)bitflips, ret);
			}
		}

		return bitflips;
	}

	/* raw read, only data not null, and its length should be $len */
	if (data)
		memcpy(data, nb->buf, len);

	return 0;
}

static int pio_tx_data(struct nfi_base *nb, u8 *data, u8 *fdm,
		       int sectors)
{
	struct nfi *nfi = &nb->nfi;
	void *regs = nb->res.nfi_regs;
	u32 i, val;
	int len, ret;

	val = readw(regs + NFI_CNFG) | CNFG_BYTE_RW;
	writew(val, regs + NFI_CNFG);

	len = nb->ecc_en ? nfi->sector_size :
	      nfi->sector_size + nfi->sector_spare_size;
	len *= sectors;

	/* data shouldn't null,
	 * and if ecc enable ,fdm been written in prepare process
	 */
	for (i = 0; i < len; i++) {
		ret = wait_io_ready(regs);
		if (ret)
			return ret;
		writeb(data[i], regs + NFI_DATAW);
	}

	return 0;
}

static bool is_page_empty(struct nfi_base *nb, u8 *data, u8 *fdm,
			  int sectors)
{
	u32 empty = readl(nb->res.nfi_regs + NFI_STA) & STA_EMP_PAGE;

	if (empty) {
		pr_info("empty page!\n");
		return true;
	}

	return false;
}

static int rw_prepare(struct nfi_base *nb, int sectors, u8 *data,
		      u8 *fdm, bool read)
{
	void *regs = nb->res.nfi_regs;
	u32 len = nb->nfi.sector_size * sectors;
	bool irq_en = nb->dma_en && nb->nfi_irq_en;
	void *dma_addr;
	u32 val;
	int ret;

	nb->rw_sectors = sectors;

	if (irq_en) {
		nandx_event_init(nb->done);
		writel(INTR_AHB_DONE_EN, regs + NFI_INTR_EN);
	}

	val = readw(regs + NFI_CNFG);
	if (read)
		val |= CNFG_READ_EN;
	else
		val &= ~CNFG_READ_EN;

	/* as design, now, auto format enabled when ecc enabled */
	if (nb->ecc_en) {
		val |= CNFG_HW_ECC_EN | CNFG_AUTO_FMT_EN;

		if (read)
			ret = enable_ecc_decode(nb, sectors);
		else
			ret = enable_ecc_encode(nb);

		if (ret) {
			pr_info("%s: ecc enable %s fail!\n", __func__,
			       read ? "decode" : "encode");
			return ret;
		}
	} else {
		val &= ~(CNFG_HW_ECC_EN | CNFG_AUTO_FMT_EN);
	}

	if (!read && nb->bad_mark_swap_en)
		nb->bad_mark_ctrl.bad_mark_swap(&nb->nfi, data, fdm);

	if (!nb->ecc_en && read)
		len += sectors * nb->nfi.sector_spare_size;

	if (nb->dma_en) {
		val |= CNFG_DMA_BURST_EN | CNFG_AHB;

		if (read) {
			dma_addr = (void *)(unsigned long)nandx_dma_map(
						nb->res.dev, nb->buf,
						(u64)len, NDMA_FROM_DEV);
		} else {
			memcpy(nb->buf, data, len);
			dma_addr = (void *)(unsigned long)nandx_dma_map(
						nb->res.dev, nb->buf,
						(u64)len, NDMA_TO_DEV);
		}

		writel((unsigned long)dma_addr, (void *)regs + NFI_STRADDR);

		nb->access_len = len;
		nb->dma_addr = dma_addr;
	}

	if (nb->ecc_en && !read && fdm)
		write_fdm(nb, fdm);

	writew(val, regs + NFI_CNFG);
	/* setup R/W sector number */
	writel(sectors << CON_SEC_SHIFT, regs + NFI_CON);

	return 0;
}

static void rw_trigger(struct nfi_base *nb, bool read)
{
	void *regs = nb->res.nfi_regs;
	u32 val;

	val = read ? CON_BRD : CON_BWR;
	val |= readl(regs + NFI_CON);
	writel(val, regs + NFI_CON);

	writel(STAR_EN, regs + NFI_STRDATA);
}

static int rw_wait_done(struct nfi_base *nb, int sectors, bool read)
{
	void *regs = nb->res.nfi_regs;
	bool irq_en = nb->dma_en && nb->nfi_irq_en;
	int ret;
	u32 val;

	if (irq_en) {
		ret = nandx_event_wait_complete(nb->done, NFI_TIMEOUT);
		if (!ret) {
			writew(0, regs + NFI_INTR_EN);
			return ret;
		}
	}

	if (read) {
		ret = readl_poll_timeout_atomic(regs + NFI_BYTELEN, val,
						ADDRCNTR_SEC(val) >=
						(u32)sectors,
						2, NFI_TIMEOUT);
		/* HW issue: if not wait ahb done, need polling bus busy */
		if (!ret && !irq_en)
			ret = readl_poll_timeout_atomic(regs + NFI_MASTER_STA,
							val,
							!(val &
							  MASTER_BUS_BUSY),
							2, NFI_TIMEOUT);
	} else {
		ret = readl_poll_timeout_atomic(regs + NFI_ADDRCNTR, val,
						ADDRCNTR_SEC(val) >=
						(u32)sectors,
						2, NFI_TIMEOUT);
	}

	if (ret) {
		pr_info("do page %s timeout\n", read ? "read" : "write");
		return ret;
	}

	if (read && nb->ecc_en) {
		ret = nb->ecc->wait_done(nb->ecc);
		if (ret)
			return ret;

		return nb->ecc->decode_status(nb->ecc, 0, sectors);
	}

	return 0;
}

static int rw_data(struct nfi_base *nb, u8 *data, u8 *fdm, int sectors,
		   bool read)
{
	if (read && nb->dma_en && nb->ecc_en && fdm)
		read_fdm(nb, fdm, 0, sectors);

	if (!nb->dma_en) {
		if (read)
			return pio_rx_data(nb, data, fdm, sectors);

		return pio_tx_data(nb, data, fdm, sectors);
	}

	return 0;
}

static void rw_complete(struct nfi_base *nb, u8 *data, u8 *fdm,
			bool read)
{
	int data_len = 0;
	bool is_empty;

	if (nb->dma_en) {
		if (read) {
			nandx_dma_unmap(nb->res.dev, nb->buf, nb->dma_addr,
					(u64)nb->access_len, NDMA_FROM_DEV);

			if (data) {
				data_len = nb->rw_sectors * nb->nfi.sector_size;
				memcpy(data, nb->buf, data_len);
			}

			if (fdm)
				memcpy(fdm, nb->buf + data_len,
				       nb->access_len - data_len);

			if (nb->read_status == -ENANDREAD) {
				is_empty = nb->is_page_empty(nb, data, fdm,
							nb->rw_sectors);
				if (is_empty)
					nb->read_status = 0;
			}
		} else {
			nandx_dma_unmap(nb->res.dev, nb->buf, nb->dma_addr,
					(u64)nb->access_len, NDMA_TO_DEV);
		}
	}

	/* whether it's reading or writing, we all check if nee swap
	 * for write, we need to restore data
	 */
	if (nb->bad_mark_swap_en)
		nb->bad_mark_ctrl.bad_mark_swap(&nb->nfi, data, fdm);

	if (nb->ecc_en)
		nb->ecc->disable(nb->ecc);

	writel(0, nb->res.nfi_regs + NFI_CNFG);
	writel(0, nb->res.nfi_regs + NFI_CON);
}

static int nfi_read_sectors(struct nfi *nfi, u8 *data, u8 *fdm,
			    int sectors)
{
	struct nfi_base *nb = nfi_to_base(nfi);
	int bitflips = 0, ret;

	pr_debug("%s: read page#%d\n", __func__, nb->row);
	pr_debug("%s: data address 0x%x, fdm address 0x%x, sectors 0x%x\n",
		 __func__, (u32)((unsigned long)data),
		 (u32)((unsigned long)fdm), sectors);

	nb->read_status = 0;

	ret = nb->rw_prepare(nb, sectors, data, fdm, true);
	if (ret)
		return ret;

	nb->rw_trigger(nb, true);

	if (nb->dma_en) {
		ret = nb->rw_wait_done(nb, sectors, true);
		if (ret > 0)
			bitflips = ret;
		else if (ret == -ENANDREAD)
			nb->read_status = -ENANDREAD;
		else if (ret < 0)
			goto complete;

	}

	ret = nb->rw_data(nb, data, fdm, sectors, true);
	if (ret > 0)
		ret = max_t(int, ret, bitflips);

complete:
	nb->rw_complete(nb, data, fdm, true);

	if (nb->read_status == -ENANDREAD)
		return -ENANDREAD;

	return ret;
}

int nfi_write_page(struct nfi *nfi, u8 *data, u8 *fdm)
{
	struct nfi_base *nb = nfi_to_base(nfi);
	u32 sectors = div_down(nb->format.page_size, nfi->sector_size);
	int ret;

	pr_debug("%s: data address 0x%x, fdm address 0x%x\n",
		 __func__, (int)((unsigned long)data),
		 (int)((unsigned long)fdm));

	ret = nb->rw_prepare(nb, sectors, data, fdm, false);
	if (ret)
		return ret;

	nb->rw_trigger(nb, false);

	ret = nb->rw_data(nb, data, fdm, sectors, false);
	if (ret)
		return ret;

	ret = nb->rw_wait_done(nb, sectors, false);

	nb->rw_complete(nb, data, fdm, false);

	return ret;
}

static int nfi_rw_bytes(struct nfi *nfi, u8 *data, int count, bool read)
{
	struct nfi_base *nb = nfi_to_base(nfi);
	void *regs = nb->res.nfi_regs;
	int i, ret;
	u32 val;

	for (i = 0; i < count; i++) {
		val = readl(regs + NFI_STA) & NFI_FSM_MASK;
		if (val != NFI_FSM_CUSTDATA) {
			val = readw(regs + NFI_CNFG) | CNFG_BYTE_RW;
			if (read)
				val |= CNFG_READ_EN;
			writew(val, regs + NFI_CNFG);

			val = div_up(count, nfi->sector_size);
			val = (val << CON_SEC_SHIFT) | CON_BRD | CON_BWR;
			writel(val, regs + NFI_CON);

			writew(STAR_EN, regs + NFI_STRDATA);
		}

		ret = wait_io_ready(regs);
		if (ret)
			return ret;

		if (read)
			data[i] = readb(regs + NFI_DATAR);
		else
			writeb(data[i], regs + NFI_DATAW);
	}

	writel(0, nb->res.nfi_regs + NFI_CNFG);

	return 0;
}

static int nfi_read_bytes(struct nfi *nfi, u8 *data, int count)
{
	return nfi_rw_bytes(nfi, data, count, true);
}

static int nfi_write_bytes(struct nfi *nfi, u8 *data, int count)
{
	return nfi_rw_bytes(nfi, data, count, false);
}

/* As register map says, only when flash macro is idle,
 * sw reset or nand interface change can be issued
 */
static inline int wait_flash_macro_idle(void *regs)
{
	u32 val;

	return readl_poll_timeout_atomic(regs + NFI_STA, val,
					 val & FLASH_MACRO_IDLE, 2,
					 NFI_TIMEOUT);
}

#define ACCTIMING(tpoecs, tprecs, tc2r, tw2r, twh, twst, trlt) \
	((tpoecs) << 28 | (tprecs) << 22 | (tc2r) << 16 | \
	 (tw2r) << 12 | (twh) << 8 | (twst) << 4 | (trlt))

static int nfi_set_sdr_timing(struct nfi *nfi, void *timing, u8 type)
{
	struct nand_sdr_timing *sdr = (struct nand_sdr_timing *) timing;
	struct nfi_base *nb = nfi_to_base(nfi);
	void *regs = nb->res.nfi_regs;
	u32 tpoecs, tprecs, tc2r, tw2r, twh, twst, trlt, tstrobe;
	u32 rate, val;
	int ret;

	ret = wait_flash_macro_idle(regs);
	if (ret)
		return ret;

	/* turn clock rate into KHZ */
	rate = nb->res.clock_1x / 1000;

	tpoecs = max_t(u16, sdr->tALH, sdr->tCLH);
	tpoecs = div_up(tpoecs * rate, 1000000);
	tpoecs &= 0xf;

	tprecs = max_t(u16, sdr->tCLS, sdr->tALS);
	tprecs = div_up(tprecs * rate, 1000000);
	tprecs &= 0x3f;

	/* tc2r is in unit of 2T */
	tc2r = div_up(sdr->tCR * rate, 1000000);
	tc2r = div_down(tc2r, 2);
	tc2r &= 0x3f;

	tw2r = div_up(sdr->tWHR * rate, 1000000);
	tw2r = div_down(tw2r, 2);
	tw2r &= 0xf;

	twh = max_t(u16, sdr->tREH, sdr->tWH);
	twh = div_up(twh * rate, 1000000) - 1;
	twh &= 0xf;

	twst = div_up(sdr->tWP * rate, 1000000) - 1;
	twst &= 0xf;

	trlt = div_up(sdr->tRP * rate, 1000000) - 1;
	trlt &= 0xf;

	/* If tREA is bigger than tRP, setup strobe sel here */
	if ((trlt + 1) * 1000000 / rate < sdr->tREA) {
		tstrobe = sdr->tREA - (trlt + 1) * 1000000 / rate;
		tstrobe = div_up(tstrobe * rate, 1000000);
		val = readl(regs + NFI_DEBUG_CON1);
		val &= ~STROBE_MASK;
		val |= tstrobe << STROBE_SHIFT;
		writel(val, regs + NFI_DEBUG_CON1);
	}

	/*
	 * ACCON: access timing control register
	 * -------------------------------------
	 * 31:28: tpoecs, minimum required time for CS post pulling down after
	 *        accessing the device
	 * 27:22: tprecs, minimum required time for CS pre pulling down before
	 *        accessing the device
	 * 21:16: tc2r, minimum required time from NCEB low to NREB low
	 * 15:12: tw2r, minimum required time from NWEB high to NREB low.
	 * 11:08: twh, write enable hold time
	 * 07:04: twst, write wait states
	 * 03:00: trlt, read wait states
	 */
	val = ACCTIMING(tpoecs, tprecs, tc2r, tw2r, twh, twst, trlt);
	pr_info("acctiming: 0x%x\n", val);
	writel(val, regs + NFI_ACCCON);

	/* set NAND type */
	writel(NAND_TYPE_ASYNC, regs + NFI_NAND_TYPE_CNFG);

	return ret;
}

static int nfi_set_timing(struct nfi *nfi, void *timing, int type)
{
	switch (type) {
	case NAND_TIMING_SDR:
		return nfi_set_sdr_timing(nfi, timing, type);

	/* NOTE: for mlc/tlc */
	case NAND_TIMING_SYNC_DDR:
	case NAND_TIMING_TOGGLE_DDR:
	case NAND_TIMING_NVDDR2:
	default:
		return -EINVAL;
	}

	return 0;
}

static void set_nfi_funcs(struct nfi *nfi)
{
	nfi->select_chip = nfi_select_chip;
	nfi->set_format = nfi_set_format;
	nfi->nfi_ctrl = nfi_ctrl;
	nfi->set_timing = nfi_set_timing;

	nfi->reset = nfi_reset;
	nfi->send_cmd = nfi_send_cmd;
	nfi->send_addr = nfi_send_addr;
	nfi->trigger = nfi_trigger;

	nfi->write_page = nfi_write_page;
	nfi->write_bytes = nfi_write_bytes;
	nfi->read_sectors = nfi_read_sectors;
	nfi->read_bytes = nfi_read_bytes;

	nfi->wait_ready = nfi_wait_ready;

	nfi->enable_randomizer = nfi_enable_randomizer;
	nfi->disable_randomizer = nfi_disable_randomizer;
}

static struct nfi_caps nfi_caps_mt7622 = {
	.max_fdm_size = 8,
	.fdm_ecc_size = 1,
	.ecc_parity_bits = 13,
	.spare_size = spare_size_mt7622,
	.spare_size_num = 4,
};

static struct nfi_caps *nfi_get_match_data(enum mtk_ic_version ic)
{
	/* NOTE: add other IC's data */
	return &nfi_caps_mt7622;
}

static void set_nfi_base_params(struct nfi_base *nb)
{
	nb->ecc_en = false;
	nb->dma_en = false;
	nb->nfi_irq_en = false;
	nb->ecc_irq_en = false;
	nb->page_irq_en = false;
	nb->ecc_clk_en = false;
	nb->randomize_en = false;
	nb->custom_sector_en = false;
	nb->bad_mark_swap_en = false;

	nb->op_mode = CNFG_CUSTOM_MODE;
	nb->ecc_deccon = ECC_DEC_CORRECT;
	nb->ecc_mode = ECC_NFI_MODE;

	nb->done = nandx_event_create();
	nb->caps = nfi_get_match_data(nb->res.ic_ver);

	nb->set_op_mode = set_op_mode;
	nb->is_page_empty = is_page_empty;

	nb->rw_prepare = rw_prepare;
	nb->rw_trigger = rw_trigger;
	nb->rw_wait_done = rw_wait_done;
	nb->rw_data = rw_data;
	nb->rw_complete = rw_complete;
}

struct nfi *__weak nfi_extend_init(struct nfi_base *nb)
{
	return &nb->nfi;
}

void __weak nfi_extend_exit(struct nfi_base *nb)
{
#if !NANDX_GLOBAL_VARIBLE
	mem_free(nb);
#endif
}

struct nfi *nfi_init(struct nfi_resource *res)
{
	struct nfiecc_resource ecc_res;
	struct nfi_base *nb;
	struct nfiecc *ecc;
	struct nfi *nfi;
	int ret;

#if NANDX_GLOBAL_VARIBLE
	nb = &g_nfi_base;
#else
	nb = mem_alloc(1, sizeof(struct nfi_base));
	if (!nb) {
		pr_info("nfi alloc memory fail @%s.\n", __func__);
		return NULL;
	}
#endif
	nb->res = *res;

	ret = nandx_irq_register(res->dev, res->nfi_irq_id, nfi_irq_handler,
				 "mtk_nand", nb);
	if (ret) {
		pr_info("nfi irq register failed!\n");
		goto error;
	}

	/* fill ecc paras and init ecc */
	ecc_res.ic_ver = nb->res.ic_ver;
	ecc_res.dev = nb->res.dev;
	ecc_res.irq_id = nb->res.ecc_irq_id;
	ecc_res.regs = nb->res.ecc_regs;
	ecc = nfiecc_init(&ecc_res);
	if (!ecc) {
		pr_info("nfiecc init fail.\n");
		return NULL;
	}

	nb->ecc = ecc;

	set_nfi_base_params(nb);
	set_nfi_funcs(&nb->nfi);

	/* Assign a temp sector size for reading ID & para page.
	 * We may assign new value later.
	 */
	nb->nfi.sector_size = 512;

	/* give a default timing, and as discuss
	 * this is the only thing what we need do for nfi init
	 * if need do more, then we can add a function
	 */
	writel(0x30C77FFF, nb->res.nfi_regs + NFI_ACCCON);

	nfi = nfi_extend_init(nb);
	if (nfi)
		return nfi;

error:
#if !NANDX_GLOBAL_VARIBLE
	mem_free(nb);
#endif
	return NULL;
}

void nfi_exit(struct nfi *nfi)
{
	struct nfi_base *nb = nfi_to_base(nfi);

	nandx_event_destroy(nb->done);
	nfiecc_exit(nb->ecc);
#if !NANDX_BULK_IO_USE_DRAM
	mem_free(nb->buf);
#endif
	nfi_extend_exit(nb);
}

