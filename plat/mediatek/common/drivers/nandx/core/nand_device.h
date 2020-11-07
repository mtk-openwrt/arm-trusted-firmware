/*
 * Copyright (C) 2017 MediaTek Inc.
 * Licensed under either
 *     BSD Licence, (see NOTICE for more details)
 *     GNU General Public License, version 2.0, (see NOTICE for more details)
 */

#ifndef __NAND_DEVICE_H__
#define __NAND_DEVICE_H__

/* onfi 3.2 */
struct nand_onfi_params {
	/* Revision information and features block. 0 */
	/*
	 * Byte 0: 4Fh,
	 * Byte 1: 4Eh,
	 * Byte 2: 46h,
	 * Byte 3: 49h,
	 */
	u8 signature[4];
	/*
	 * 9-15 Reserved (0)
	 * 8 1 = supports ONFI version 3.2
	 * 7 1 = supports ONFI version 3.1
	 * 6 1 = supports ONFI version 3.0
	 * 5 1 = supports ONFI version 2.3
	 * 4 1 = supports ONFI version 2.2
	 * 3 1 = supports ONFI version 2.1
	 * 2 1 = supports ONFI version 2.0
	 * 1 1 = supports ONFI version 1.0
	 * 0 Reserved (0)
	 */
	u16 revision;
	/*
	 * 13-15 Reserved (0)
	 * 12 1 = supports external Vpp
	 * 11 1 = supports Volume addressing
	 * 10 1 = supports NV-DDR2
	 * 9 1 = supports EZ NAND
	 * 8 1 = supports program page register clear enhancement
	 * 7 1 = supports extended parameter page
	 * 6 1 = supports multi-plane read operations
	 * 5 1 = supports NV-DDR
	 * 4 1 = supports odd to even page Copyback
	 * 3 1 = supports multi-plane program and erase operations
	 * 2 1 = supports non-sequential page programming
	 * 1 1 = supports multiple LUN operations
	 * 0 1 = supports 16-bit data bus width
	 */
	u16 features;
	/*
	 * 13-15 Reserved (0)
	 * 12 1 = supports LUN Get and LUN Set Features
	 * 11 1 = supports ODT Configure
	 * 10 1 = supports Volume Select
	 * 9 1 = supports Reset LUN
	 * 8 1 = supports Small Data Move
	 * 7 1 = supports Change Row Address
	 * 6 1 = supports Change Read Column Enhanced
	 * 5 1 = supports Read Unique ID
	 * 4 1 = supports Copyback
	 * 3 1 = supports Read Status Enhanced
	 * 2 1 = supports Get Features and Set Features
	 * 1 1 = supports Read Cache commands
	 * 0 1 = supports Page Cache Program command
	 */
	u16 opt_cmds;
	/*
	 * 4-7 Reserved (0)
	 * 3 1 = supports Multi-plane Block Erase
	 * 2 1 = supports Multi-plane Copyback Program
	 * 1 1 = supports Multi-plane Page Program
	 * 0 1 = supports Random Data Out
	 */
	u8 advance_cmds;
	u8 reserved0[1];
	u16 extend_param_len;
	u8 param_page_num;
	u8 reserved1[17];

	/* Manufacturer information block. 32 */
	u8 manufacturer[12];
	u8 model[20];
	u8 jedec_id;
	u16 data_code;
	u8 reserved2[13];

	/* Memory organization block. 80 */
	u32 page_size;
	u16 spare_size;
	u32 partial_page_size; /* obsolete */
	u16 partial_spare_size; /* obsolete */
	u32 block_pages;
	u32 lun_blocks;
	u8 lun_num;
	/*
	 * 4-7 Column address cycles
	 * 0-3 Row address cycles
	 */
	u8 addr_cycle;
	u8 cell_bits;
	u16 lun_max_bad_blocks;
	u16 block_endurance;
	u8 target_begin_valid_blocks;
	u16 valid_block_endurance;
	u8 page_program_num;
	u8 partial_program_attr; /* obsolete */
	u8 ecc_req;
	/*
	 * 4-7 Reserved (0)
	 * 0-3 Number of plane address bits
	 */
	u8 plane_address_bits;
	/*
	 * 6-7 Reserved (0)
	 * 5 1 = lower bit XNOR block address restriction
	 * 4 1 = read cache supported
	 * 3 Address restrictions for cache operations
	 * 2 1 = program cache supported
	 * 1 1 = no block address restrictions
	 * 0 Overlapped / concurrent multi-plane support
	 */
	u8 multi_plane_attr;
	u8 ez_nand_support;
	u8 reserved3[12];

	/* Electrical parameters block. 128 */
	u8 io_pin_max_capacitance;
	/*
	 * 6-15 Reserved (0)
	 * 5 1 = supports timing mode 5
	 * 4 1 = supports timing mode 4
	 * 3 1 = supports timing mode 3
	 * 2 1 = supports timing mode 2
	 * 1 1 = supports timing mode 1
	 * 0 1 = supports timing mode 0, shall be 1
	 */
	u16 sdr_timing_mode;
	u16 sdr_program_cache_timing_mode; /* obsolete */
	u16 tPROG;
	u16 tBERS;
	u16 tR;
	u16 tCCS;
	/*
	 * 7 Reserved (0)
	 * 6 1 = supports NV-DDR2 timing mode 8
	 * 5 1 = supports NV-DDR timing mode 5
	 * 4 1 = supports NV-DDR timing mode 4
	 * 3 1 = supports NV-DDR timing mode 3
	 * 2 1 = supports NV-DDR timing mode 2
	 * 1 1 = supports NV-DDR timing mode 1
	 * 0 1 = supports NV-DDR timing mode 0
	 */
	u8 nvddr_timing_mode;
	/*
	 * 7 1 = supports timing mode 7
	 * 6 1 = supports timing mode 6
	 * 5 1 = supports timing mode 5
	 * 4 1 = supports timing mode 4
	 * 3 1 = supports timing mode 3
	 * 2 1 = supports timing mode 2
	 * 1 1 = supports timing mode 1
	 * 0 1 = supports timing mode 0
	 */
	u8 nvddr2_timing_mode;
	/*
	 * 4-7 Reserved (0)
	 * 3 1 = device requires Vpp enablement sequence
	 * 2 1 = device supports CLK stopped for data input
	 * 1 1 = typical capacitance
	 * 0 tCAD value to use
	 */
	u8 nvddr_fetures;
	u16 clk_pin_capacitance;
	u16 io_pin_capacitance;
	u16 input_pin_capacitance;
	u8 input_pin_max_capacitance;
	/*
	 * 3-7 Reserved (0)
	 * 2 1 = supports 18 Ohm drive strength
	 * 1 1 = supports 25 Ohm drive strength
	 * 0 1 = supports driver strength settings
	 */
	u8 drive_strength;
	u16 tR_multi_plane;
	u16 tADL;
	u16 tR_ez_nand;
	/*
	 * 6-7 Reserved (0)
	 * 5 1 = external VREFQ required for >= 200 MT/s
	 * 4 1 = supports differential signaling for DQS
	 * 3 1 = supports differential signaling for RE_n
	 * 2 1 = supports ODT value of 30 Ohms
	 * 1 1 = supports matrix termination ODT
	 * 0 1 = supports self-termination ODT
	 */
	u8 nvddr2_features;
	u8 nvddr2_warmup_cycles;
	u8 reserved4[4];

	/* vendor block. 164 */
	u16 vendor_revision;
	u8      vendor_spec[88];

	/* CRC for Parameter Page. 254 */
	u16 crc16;
} __packed;

/* JESD230-B */
struct nand_jedec_params {
	/* Revision information and features block. 0 */
	/*
	 * Byte 0:4Ah
	 * Byte 1:45h
	 * Byte 2:53h
	 * Byte 3:44h
	 */
	u8 signature[4];
	/*
	 * 3-15: Reserved (0)
	 * 2: 1 = supports parameter page revision 1.0 and standard revision 1.0
	 * 1: 1 = supports vendor specific parameter page
	 * 0: Reserved (0)
	 */
	u16 revision;
	/*
	 * 9-15 Reserved (0)
	 * 8: 1 = supports program page register clear enhancement
	 * 7: 1 = supports external Vpp
	 * 6: 1 = supports Toggle Mode DDR
	 * 5: 1 = supports Synchronous DDR
	 * 4: 1 = supports multi-plane read operations
	 * 3: 1 = supports multi-plane program and erase operations
	 * 2: 1 = supports non-sequential page programming
	 * 1: 1 = supports multiple LUN operations
	 * 0: 1 = supports 16-bit data bus width
	 */
	u16 features;
	/*
	 * 11-23: Reserved (0)
	 * 10: 1 = supports Synchronous Reset
	 * 9: 1 = supports Reset LUN (Primary)
	 * 8: 1 = supports Small Data Move
	 * 7: 1 = supports Multi-plane Copyback Program (Primary)
	 * 6: 1 = supports Random Data Out (Primary)
	 * 5: 1 = supports Read Unique ID
	 * 4: 1 = supports Copyback
	 * 3: 1 = supports Read Status Enhanced (Primary)
	 * 2: 1 = supports Get Features and Set Features
	 * 1: 1 = supports Read Cache commands
	 * 0: 1 = supports Page Cache Program command
	 */
	u8 opt_cmds[3];
	/*
	 * 8-15: Reserved (0)
	 * 7: 1 = supports secondary Read Status Enhanced
	 * 6: 1 = supports secondary Multi-plane Block Erase
	 * 5: 1 = supports secondary Multi-plane Copyback Program
	 * 4: 1 = supports secondary Multi-plane Program
	 * 3: 1 = supports secondary Random Data Out
	 * 2: 1 = supports secondary Multi-plane Copyback Read
	 * 1: 1 = supports secondary Multi-plane Read Cache Random
	 * 0: 1 = supports secondary Multi-plane Read
	 */
	u16 secondary_cmds;
	u8 param_page_num;
	u8 reserved0[18];

	/* Manufacturer information block. 32*/
	u8 manufacturer[12];
	u8 model[20];
	u8 jedec_id[6];
	u8 reserved1[10];

	/* Memory organization block. 80 */
	u32 page_size;
	u16 spare_size;
	u8 reserved2[6];
	u32 block_pages;
	u32 lun_blocks;
	u8 lun_num;
	/*
	 * 4-7 Column address cycles
	 * 0-3 Row address cycles
	 */
	u8 addr_cycle;
	u8 cell_bits;
	u8 page_program_num;
	/*
	 * 4-7 Reserved (0)
	 * 0-3 Number of plane address bits
	 */
	u8 plane_address_bits;
	/*
	 * 3-7: Reserved (0)
	 * 2: 1= read cache supported
	 * 1: 1 = program cache supported
	 * 0: 1= No multi-plane block address restrictions
	 */
	u8 multi_plane_attr;
	u8 reserved3[38];

	/* Electrical parameters block. 144 */
	/*
	 * 6-15: Reserved (0)
	 * 5: 1 = supports 20 ns speed grade (50 MHz)
	 * 4: 1 = supports 25 ns speed grade (40 MHz)
	 * 3: 1 = supports 30 ns speed grade (~33 MHz)
	 * 2: 1 = supports 35 ns speed grade (~28 MHz)
	 * 1: 1 = supports 50 ns speed grade (20 MHz)
	 * 0: 1 = supports 100 ns speed grade (10 MHz)
	 */
	u16 sdr_speed;
	/*
	 * 8-15: Reserved (0)
	 * 7: 1 = supports 5 ns speed grade (200 MHz)
	 * 6: 1 = supports 6 ns speed grade (~166 MHz)
	 * 5: 1 = supports 7.5 ns speed grade (~133 MHz)
	 * 4: 1 = supports 10 ns speed grade (100 MHz)
	 * 3: 1 = supports 12 ns speed grade (~83 MHz)
	 * 2: 1 = supports 15 ns speed grade (~66 MHz)
	 * 1: 1 = supports 25 ns speed grade (40 MHz)
	 * 0: 1 = supports 30 ns speed grade (~33 MHz)
	 */
	u16 toggle_ddr_speed;
	/*
	 * 6-15: Reserved (0)
	 * 5: 1 = supports 10 ns speed grade (100 MHz)
	 * 4: 1 = supports 12 ns speed grade (~83 MHz)
	 * 3: 1 = supports 15 ns speed grade (~66 MHz)
	 * 2: 1 = supports 20 ns speed grade (50 MHz)
	 * 1: 1 = supports 30 ns speed grade (~33 MHz)
	 * 0: 1 = supports 50 ns speed grade (20 MHz)
	 */
	u16 sync_ddr_speed;
	u8 sdr_features;
	u8 toggle_ddr_features;
	/*
	 * 2-7: Reserved (0)
	 * 1: Device supports CK stopped for data input
	 * 0: tCAD value to use
	 */
	u8 sync_ddr_features;
	u16 tPROG;
	u16 tBERS;
	u16 tR;
	u16 tR_multi_plane;
	u16 tCCS;
	u16 io_pin_capacitance;
	u16 input_pin_capacitance;
	u16 ck_pin_capacitance;
	/*
	 * 3-7: Reserved (0)
	 * 2: 1 = supports 18 ohm drive strength
	 * 1: 1 = supports 25 ohm drive strength
	 * 0: 1 = supports 35ohm/50ohm drive strength
	 */
	u8 drive_strength;
	u16 tADL;
	u8 reserved4[36];

	/* ECC and endurance block. 208 */
	u8 target_begin_valid_blocks;
	u16 valid_block_endurance;
	/*
	 * Byte 0: Number of bits ECC correctability
	 * Byte 1: Codeword size
	 * Byte 2-3: Bad blocks maximum per LUN
	 * Byte 4-5: Block endurance
	 * Byte 6-7: Reserved (0)
	 */
	u8 endurance_block0[8];
	u8 endurance_block1[8];
	u8 endurance_block2[8];
	u8 endurance_block3[8];
	u8 reserved5[29];

	/* Reserved. 272 */
	u8 reserved6[148];

	/* Vendor specific block. 420  */
	u16 vendor_revision;
	u8      vendor_spec[88];

	/* CRC for Parameter Page. 510 */
	u16 crc16;
} __packed;

/* parallel nand io width */
enum nand_io_width {
	NAND_IO8,
	NAND_IO16
};

/* all supported nand timming type */
enum nand_timing_type {
	NAND_TIMING_SDR,
	NAND_TIMING_SYNC_DDR,
	NAND_TIMING_TOGGLE_DDR,
	NAND_TIMING_NVDDR2
};

/* nand basic commands */
struct nand_cmds {
	short reset;
	short read_id;
	short read_status;
	short read_param_page;
	short set_feature;
	short get_feature;
	short read_1st;
	short read_2nd;
	short random_out_1st;
	short random_out_2nd;
	short program_1st;
	short program_2nd;
	short erase_1st;
	short erase_2nd;
	short read_cache;
	short read_cache_last;
	short program_cache;
};

/*
 * addressing for nand physical address
 * @row_bit_start: row address start bit
 * @block_bit_start: block address start bit
 * @plane_bit_start: plane address start bit
 * @lun_bit_start: lun address start bit
 */
struct nand_addressing {
	u8 row_bit_start;
	u8 block_bit_start;
	u8 plane_bit_start;
	u8 lun_bit_start;
};

/*
 * nand operations status
 * @array_busy: indicates device array operation busy
 * @write_protect: indicates the device cannot be wrote or erased
 * @erase_fail: indicates erase operation fail
 * @program_fail: indicates program operation fail
 */
struct nand_status {
	u8 array_busy;
	u8 write_protect;
	u8 erase_fail;
	u8 program_fail;
};

/*
 * nand endurance information
 * @pe_cycle: max program/erase cycle for nand stored data stability
 * @ecc_req: ecc strength required for the nand, measured per 1KB
 * @max_bitflips: bitflips is ecc corrected bits,
 *    max_bitflips is the threshold for nand stored data stability
 *    if corrected bits is over max_bitflips, stored data must be moved
 *    to another good block
 */
struct nand_endurance {
	int pe_cycle;
	int ecc_req;
	int max_bitflips;
};

/* wait for nand busy type */
enum nand_wait_type {
	NAND_WAIT_IRQ,
	NAND_WAIT_POLLING,
	NAND_WAIT_TWHR2,
};

/* each nand array operations time */
struct nand_array_timing {
	u16 tRST;
	u16 tWHR;
	u16 tR;
	u16 tRCBSY;
	u16 tFEAT;
	u16 tPROG;
	u16 tPCBSY;
	u16 tBERS;
	u16 tDBSY;
};

/* nand sdr interface timing required */
struct nand_sdr_timing {
	u16 tREA;
	u16 tREH;
	u16 tCR;
	u16 tRP;
	u16 tWP;
	u16 tWH;
	u16 tWHR;
	u16 tCLS;
	u16 tALS;
	u16 tCLH;
	u16 tALH;
	u16 tWC;
	u16 tRC;
};

/* nand onfi ddr (nvddr) interface timing required */
struct nand_onfi_timing {
	u16 tCAD;
	u16 tWPRE;
	u16 tWPST;
	u16 tWRCK;
	u16 tDQSCK;
	u16 tWHR;
};

/* nand toggle ddr (toggle 1.0) interface timing required */
struct nand_toggle_timing {
	u16 tCS;
	u16 tCH;
	u16 tCAS;
	u16 tCAH;
	u16 tCALS;
	u16 tCALH;
	u16 tWP;
	u16 tWPRE;
	u16 tWPST;
	u16 tWPSTH;
	u16 tCR;
	u16 tRPRE;
	u16 tRPST;
	u16 tRPSTH;
	u16 tCDQSS;
	u16 tWHR;
};

/* nand basic device information */
struct nand_device {
	char *name;
	u64 id;
	u8 id_len;
	u8 io_width;
	u8 row_cycle;
	u8 col_cycle;
	u8 target_num;
	u8 lun_num;
	u8 plane_num;
	int block_num;
	int block_size;
	int page_size;
	int spare_size;
	int min_program_pages;
	struct nand_cmds *cmds;
	struct nand_addressing *addressing;
	struct nand_status *status;
	struct nand_endurance *endurance;
	struct nand_array_timing *array_timing;
};

#define NAND_DEVICE(_name, _id, _id_len, _io_width, _row_cycle, \
		    _col_cycle, _target_num, _lun_num, _plane_num, \
		    _block_num, _block_size, _page_size, _spare_size, \
		    _min_program_pages, _cmds, _addressing, _status, \
		    _endurance, _array_timing) \
{ \
	_name, _id, _id_len, _io_width, _row_cycle, \
	_col_cycle, _target_num, _lun_num, _plane_num, \
	_block_num, _block_size, _page_size, _spare_size, \
	_min_program_pages, _cmds, _addressing, _status, \
	_endurance, _array_timing \
}

#define MAX_ID_NUM      sizeof(u64)

#define NAND_PACK_ID(id0, id1, id2, id3, id4, id5, id6, id7) \
	( \
	  id0 | id1 << 8 | id2 << 16 | id3 << 24 | \
	  (u64)id4 << 32 | (u64)id5 << 40 | \
	  (u64)id6 << 48 | (u64)id7 << 56 \
	)

#define NAND_UNPACK_ID(id, ids, len) \
	do { \
		int _i; \
		for (_i = 0; _i < len; _i++) \
			ids[_i] = id >> (_i << 3) & 0xff; \
	} while (0)

static inline int nand_block_pages(struct nand_device *device)
{
	return div_down(device->block_size, device->page_size);
}

static inline int nand_lun_blocks(struct nand_device *device)
{
	return device->plane_num * device->block_num;
}

static inline int nand_target_blocks(struct nand_device *device)
{
	return device->lun_num * device->plane_num * device->block_num;
}

static inline int nand_total_blocks(struct nand_device *device)
{
	return device->target_num * device->lun_num * device->plane_num *
	       device->block_num;
}

struct nand_device *nand_get_device(int index);
#endif /* __NAND_DEVICE_H__ */
