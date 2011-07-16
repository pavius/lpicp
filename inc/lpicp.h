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
#define LPP_REG_TABLAT  (0xF5)
#define LPP_REG_EEDATA  (0xA8)
#define LPP_REG_EEADR   (0xA9)
#define LPP_REG_EEADRH  (0xAA)
#define LPP_REG_EECON2  (0xA7)

/* PIC opcodes */
#define LPP_OP_MOVLW(value)                 ((0x0E << 8) | (value))
#define LPP_OP_MOVWF(register_address)      ((0x6E << 8) | (register_address))
#define LPP_OP_NOP                          (0x0)
#define LPP_SET_EEPGD                       (0x8EA6)
#define LPP_SET_CFGS                        (0x8CA6)
#define LPP_CLR_EEPGD                       (0x9EA6)
#define LPP_CLR_CFGS                        (0x9CA6)
#define LPP_SET_WREN                        (0x86A6)
#define LPP_SET_FREE                        (0x88A6)
#define LPP_INC_TBLPTRL                     (0x2AF6)
#define LPP_SET_PC_100K_0                   (0xEF00)
#define LPP_SET_PC_100K_1                   (0xF800)
#define LPP_MOVF_EEDATA_W                   (0x50A8)
#define LPP_MOVF_EECON1_W                   (0x50A6)
#define LPP_SET_EECON1_RD                   (0x80A6)
#define LPP_SET_EECON1_WREN                 (0x84A6)
#define LPP_CLR_EECON1_WREN                 (0x94A6)
#define LPP_SET_EECON1_WR                   (0x82A6)

/* ICSP commands */
#define LPP_ICSP_CMD_CORE_INST              (0x0) // 0000
#define LPP_ICSP_CMD_SHIFT_TABLAT_REG       (0x2) // 0010
#define LPP_ICSP_CMD_TBL_RD                 (0x8) // 1000
#define LPP_ICSP_CMD_TBL_RD_POST_INC        (0x9) // 1001
#define LPP_ICSP_CMD_TBL_RD_POST_DEC        (0xA) // 1010
#define LPP_ICSP_CMD_TBL_RD_PRE_INC         (0xB) // 1011
#define LPP_ICSP_CMD_TBL_WR_POST_INC        (0xC) // 1100
#define LPP_ICSP_CMD_TBL_WR_POST_INC_2      (0xD) // 1101
#define LPP_ICSP_CMD_TBL_WR_PROG_POST_INC_2 (0xE) // 1110
#define LPP_ICSP_CMD_TBL_WR_PROG            (0xF) // 1111

/* notification callback types */
typedef int (*ntfy_progress_t)(struct lpp_context_t *, const unsigned int, const unsigned int);

/* lpp context */
struct lpp_context_t
{
    struct lpp_log_record_t     *log_records;
    unsigned int                log_record_count;
    unsigned int                log_current_idx;
    char                        *icsp_dev_name;
    int                         icsp_dev_file;
    struct lpp_device_t         device;

    /* notifications */
    ntfy_progress_t             ntfy_progress;
};

/* initialize a context */
int lpp_context_init(struct lpp_context_t *context, 
                     const enum lpp_device_family_type_t family,
                     char *icsp_dev_name,
                     ntfy_progress_t ntfy_progress);

/* destroy a context */
int lpp_context_destroy(struct lpp_context_t *context);

/* perform bulk erase */
int lpp_bulk_erase(struct lpp_context_t *context);

/* perform non-bulk erase */
int lpp_non_bulk_erase(struct lpp_context_t *context);

/* read device id */
int lpp_device_id_read(struct lpp_context_t *context, unsigned short *device_id);

/* write an image to the device program */
int lpp_write_image_to_device_program(struct lpp_context_t *context, struct lpp_image_t *image);

/* write an image to the device config */
int lpp_write_image_to_device_config(struct lpp_context_t *context, struct lpp_image_t *image);

/* write an image to device eeprom */ 
int lpp_read_image_to_device_eeprom(struct lpp_context_t *context, struct lpp_image_t *image);

/* read the image from the device eeprom */
int lpp_read_device_eeprom_to_image(struct lpp_context_t *context, struct lpp_image_t *image);

/* read the image from the device */
int lpp_read_device_program_to_image(struct lpp_context_t *context, 
                                     const unsigned int offset,
                                     const unsigned int size_in_bytes,
                                     struct lpp_image_t *image);

/* read the image program from the device */
int lpp_read_device_config_to_image(struct lpp_context_t *context,
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
