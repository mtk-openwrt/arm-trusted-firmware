/*
 * Copyright (C) 2017 MediaTek Inc.
 * Licensed under either
 *     BSD Licence, (see NOTICE for more details)
 *     GNU General Public License, version 2.0, (see NOTICE for more details)
 */

#include "nandx_util.h"
#include "nandx_core.h"
#include "nand_chip.h"
#include "nand_device.h"
#include "nand_base.h"
#include "nandx_os.h"

#define MAX_CHIP_DEVICE 4
#define PARAM_PAGE_LEN  2048
#define ONFI_CRC_BASE   0x4f4e

#if NANDX_GLOBAL_VARIBLE
u8 g_para_buf[2048];
#endif

static u16 nand_onfi_crc16(u16 crc, u8 const *p, size_t len)
{
	int i;

	while (len--) {
		crc ^= *p++ << 8;

		for (i = 0; i < 8; i++)
			crc = (crc << 1) ^ ((crc & 0x8000) ? 0x8005 : 0);
	}

	return crc;
}

static inline void decode_addr_cycle(u8 addr_cycle, u8 *row_cycle,
				     u8 *col_cycle)
{
	*row_cycle = addr_cycle & 0xf;
	*col_cycle = (addr_cycle >> 4) & 0xf;
}

static int detect_onfi(struct nand_device *dev,
		       struct nand_onfi_params *onfi)
{
	struct nand_endurance *endurance = dev->endurance;
	u16 size, i, crc16;
	u8 *id;

	size = sizeof(struct nand_onfi_params) - sizeof(u16);

	for (i = 0; i < 3; i++) {
		crc16 = nand_onfi_crc16(ONFI_CRC_BASE, (u8 *)&onfi[i], size);

		if (onfi[i].signature[0] == 'O' &&
		    onfi[i].signature[1] == 'N' &&
		    onfi[i].signature[2] == 'F' &&
		    onfi[i].signature[3] == 'I' &&
		    onfi[i].crc16 == crc16)
			break;

		/* in some spi nand, onfi signature maybe "NAND" */
		if (onfi[i].signature[0] == 'N' &&
		    onfi[i].signature[1] == 'A' &&
		    onfi[i].signature[2] == 'N' &&
		    onfi[i].signature[3] == 'D' &&
		    onfi[i].crc16 == crc16)
			break;
	}

	if (i == 3)
		return -ENODEV;

	memcpy(dev->name, onfi[i].model, 20);
	id = onfi[i].manufacturer;
	dev->id = NAND_PACK_ID(id[0], id[1], id[2], id[3], id[4], id[5], id[6],
			       id[7]);
	dev->id_len = MAX_ID_NUM;
	dev->io_width = (onfi[i].features & 1) ? NAND_IO16 : NAND_IO8;
	decode_addr_cycle(onfi[i].addr_cycle, &dev->row_cycle,
			  &dev->col_cycle);
	dev->target_num = 1;
	dev->lun_num = onfi[i].lun_num;
	dev->plane_num = BIT(onfi[i].plane_address_bits);
	dev->block_num = onfi[i].lun_blocks / dev->plane_num;
	dev->block_size = onfi[i].block_pages * onfi[i].page_size;
	dev->page_size = onfi[i].page_size;
	dev->spare_size = onfi[i].spare_size;

	endurance->ecc_req = onfi[i].ecc_req;
	endurance->pe_cycle = onfi[i].valid_block_endurance;
	endurance->max_bitflips = endurance->ecc_req >> 1;

	return 0;
}

static int detect_jedec(struct nand_device *dev,
			struct nand_jedec_params *jedec)
{
	struct nand_endurance *endurance = dev->endurance;
	u16 size, i, crc16;
	u8 *id;

	size = sizeof(struct nand_jedec_params) - sizeof(u16);

	for (i = 0; i < 3; i++) {
		crc16 = nand_onfi_crc16(ONFI_CRC_BASE, (u8 *)&jedec[i], size);

		if (jedec[i].signature[0] == 'J' &&
		    jedec[i].signature[1] == 'E' &&
		    jedec[i].signature[2] == 'S' &&
		    jedec[i].signature[3] == 'D' &&
		    jedec[i].crc16 == crc16)
			break;
	}

	if (i == 3)
		return -ENODEV;

	memcpy(dev->name, jedec[i].model, 20);
	id = jedec[i].manufacturer;
	dev->id = NAND_PACK_ID(id[0], id[1], id[2], id[3], id[4], id[5], id[6],
			       id[7]);
	dev->id_len = MAX_ID_NUM;
	dev->io_width = (jedec[i].features & 1) ? NAND_IO16 : NAND_IO8;
	decode_addr_cycle(jedec[i].addr_cycle, &dev->row_cycle,
			  &dev->col_cycle);
	dev->target_num = 1;
	dev->lun_num = jedec[i].lun_num;
	dev->plane_num = BIT(jedec[i].plane_address_bits);
	dev->block_num = jedec[i].lun_blocks / dev->plane_num;
	dev->block_size = jedec[i].block_pages * jedec[i].page_size;
	dev->page_size = jedec[i].page_size;
	dev->spare_size = jedec[i].spare_size;

	endurance->ecc_req = jedec[i].endurance_block0[0];
	endurance->pe_cycle = jedec[i].valid_block_endurance;
	endurance->max_bitflips = endurance->ecc_req >> 1;

	return 0;
}

static struct nand_device *detect_parameters_page(struct nand_base
						  *nand)
{
	struct nand_device *dev = nand->dev;
	void *params;
	int ret;

#if NANDX_GLOBAL_VARIBLE
	params = g_para_buf;
#else

	params = mem_alloc(1, PARAM_PAGE_LEN);
	if (!params)
		return NULL;
#endif

	memset(params, 0, PARAM_PAGE_LEN);
	ret = nand->read_param_page(nand, params, PARAM_PAGE_LEN);
	if (ret < 0) {
		pr_info("read parameters page fail!\n");
		goto error;
	}

	ret = detect_onfi(dev, params);
	if (ret) {
		pr_info("detect onfi device fail! try to detect jedec\n");
		ret = detect_jedec(dev, params);
		if (ret) {
			pr_info("detect jedec device fail!\n");
			goto error;
		}
	}

#if !NANDX_GLOBAL_VARIBLE
	mem_free(params);
#endif
	return dev;

error:
#if !NANDX_GLOBAL_VARIBLE
	mem_free(params);
#endif
	return NULL;
}

static int read_device_id(struct nand_base *nand, int cs, u8 *id)
{
	int i;

	nand->select_device(nand, cs);
	nand->reset(nand);
	nand->read_id(nand, id, MAX_ID_NUM);
	pr_info("device %d ID: ", cs);

	for (i = 0; i < MAX_ID_NUM; i++)
		pr_info("%x ", id[i]);

	pr_info("\n");

	return 0;
}

static int detect_more_device(struct nand_base *nand, u8 *id)
{
	u8 id_ext[MAX_ID_NUM];
	int i, j, target_num = 0;

	for (i = 1; i < MAX_CHIP_DEVICE; i++) {
		memset(id_ext, 0xff, MAX_ID_NUM);
		read_device_id(nand, i, id_ext);

		for (j = 0; j < MAX_ID_NUM; j++) {
			if (id_ext[j] != id[j])
				goto out;
		}

		target_num += 1;
	}

out:
	return target_num;
}

static struct nand_device *scan_device_table(const u8 *id, int id_len)
{
	struct nand_device *dev;
	int i = 0, j;
	u8 ids[MAX_ID_NUM] = {0};

	while (1) {
		dev = nand_get_device(i);

		if (!strcmp(dev->name, "NO-DEVICE"))
			break;

		if (id_len < dev->id_len) {
			i += 1;
			continue;
		}

		NAND_UNPACK_ID(dev->id, ids, MAX_ID_NUM);
		for (j = 0; j < dev->id_len; j++) {
			if (ids[j] != id[j])
				break;
		}

		if (j == dev->id_len)
			break;

		i += 1;
	}

	return dev;
}

int nand_detect_device(struct nand_base *nand)
{
	struct nand_device *dev;
	u8 id[MAX_ID_NUM] = { 0 };
	int target_num = 0;

	/* Get nand device default setting for reset/read_id */
	nand->dev = scan_device_table(NULL, -1);

	read_device_id(nand, 0, id);
	dev = scan_device_table(id, MAX_ID_NUM);

	if (!strcmp(dev->name, "NO-DEVICE")) {
		pr_info("device scan fail\n");
		return -ENODEV;
	}

	/* TobeFix: has null pointer issue in this funciton */
	if (!strcmp(dev->name, "NO-DEVICE")) {
		pr_info("device scan fail, detect parameters page\n");
		dev = detect_parameters_page(nand);
		if (!dev) {
			pr_info("detect parameters fail\n");
			return -ENODEV;
		}
	}

	if (dev->target_num > 1)
		target_num = detect_more_device(nand, id);

	target_num += 1;
	pr_debug("chip has target device num: %d\n", target_num);

	if (dev->target_num != target_num)
		dev->target_num = target_num;

	nand->dev = dev;

	return 0;
}

