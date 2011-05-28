/* 
 * Linux PIC Programmer (lpicp)
 * Main module header
 *
 * Author: Eran Duchan <pavius@gmail.com>
 *
 * This program is free software; you can redistribute  it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 */

#ifndef __LPICPC_H
#define __LPICPC_H

#include "lpicp_device.h"

/* forward declare */
struct lpp_image_t;

/* PIC registers */
#define LPP_REG_TBLPTRU (0xF8)
#define LPP_REG_TBLPTRH (0xF7)
#define LPP_REG_TBLPTRL (0xF6)

/* PIC opcodes */
#define LPP_OP_MOVLW(value)				((0x0E << 8) | (value))
#define LPP_OP_MOVWF(register_address)	((0x6E << 8) | (register_address))
#define LPP_OP_NOP						(0x0)
#define LPP_SET_EEPGD					(0x8EA6)
#define LPP_SET_CFGS					(0x8CA6)
#define LPP_CLR_CFGS					(0x9CA6)
#define LPP_SET_WREN					(0x86A6)

/* ICSP commands */
#define LPP_ICSP_CMD_CORE_INST				(0x0) // 0000
#define LPP_ICSP_CMD_SHIFT_TABLAT_REG		(0x2) // 0010
#define LPP_ICSP_CMD_TBL_RD					(0x8) // 1000
#define LPP_ICSP_CMD_TBL_RD_POST_INC		(0x9) // 1001
#define LPP_ICSP_CMD_TBL_RD_POST_DEC		(0xA) // 1010
#define LPP_ICSP_CMD_TBL_RD_PRE_INC			(0xB) // 1011
#define LPP_ICSP_CMD_TBL_WR_POST_INC		(0xC) // 1100
#define LPP_ICSP_CMD_TBL_WR_POST_INC_2		(0xD) // 1101
#define LPP_ICSP_CMD_TBL_WR_PROG_POST_INC_2 (0xE) // 1110
#define LPP_ICSP_CMD_TBL_WR_PROG			(0xF) // 1111

/* lpp context */
struct lpp_context_t
{
	struct lpp_log_record_t		*log_records;
	unsigned int				log_record_count;
	unsigned int				log_current_idx;
	char						*icsp_dev_name;
	int							icsp_dev_file;
	struct lpp_device_t			*device;
};

/* initialize a context */
int lpp_context_init(struct lpp_context_t *context, 
					 const enum lpp_device_family_e family,
					 char *icsp_dev_name);

/* destroy a context */
int lpp_context_destroy(struct lpp_context_t *context);

/* perform bulk erase */
int lpp_bulk_erase(struct lpp_context_t *context);

/* read device id */
int lpp_device_id_read(struct lpp_context_t *context, unsigned short *device_id);

/* write an image to the device */
int lpp_program_image_to_device(struct lpp_context_t *context, struct lpp_image_t *image);

/* read the image from the device */
int lpp_read_device_to_image(struct lpp_context_t *context, 
							 const unsigned int offset,
							 const unsigned int size_in_bytes,
							 struct lpp_image_t *image);

/* execute an instruction */
int lpp_exec_instruction(struct lpp_context_t *context,
						 const unsigned short instuction);

/* write 16 bits of data to a specified address */
int lpp_write_16(struct lpp_context_t *context,
				 const unsigned int address,
				 const unsigned short data);

/* read 16 bits of data from a specified address */
int lpp_read_16(struct lpp_context_t *context,
				 const unsigned int address,
				 unsigned short *data);

/* set tbpltr register */
int lpp_tblptr_set(struct lpp_context_t *context,
				   const unsigned int value);

#endif /* __LPICPC_H */
