/* 
 * Linux PIC Programmer (lpicp)
 * Main module implementation
 *
 * Author: Eran Duchan <pavius@gmail.com>
 *
 * This program is free software; you can redistribute  it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 */

#include "lpicp.h"
#include "lpicp_log.h"
#include "lpicp_icsp.h"
#include "lpicp_image.h"
#include <string.h>
#include <stdlib.h>

/* remove */ 
#define usleep(x) 

/* initialize a context */
int lpp_context_init(struct lpp_context_t *context,
					 char *icsp_dev_name)
{
	/* init structure */
	memset(context, 0, sizeof(struct lpp_context_t));

	/* try to open the driver */
	return lpp_icsp_init(context, icsp_dev_name);
}

/* destroy a context */
int lpp_context_destroy(struct lpp_context_t *context)
{
	/* try to close the driver */
	return lpp_icsp_destroy(context);
}

/* set tbpltr register */
int lpp_tblptr_set(struct lpp_context_t *context, 
				   const unsigned int value)
{
	/* set TBLPTRU, TBLPTRH and TBLPTRL */
	return lpp_icsp_write_16(context, LPP_ICSP_CMD_CORE_INST, LPP_OP_MOVLW((value >> 16) & 0xFF))	&& 
			lpp_icsp_write_16(context, LPP_ICSP_CMD_CORE_INST, LPP_OP_MOVWF(LPP_REG_TBLPTRU))		&& 
			lpp_icsp_write_16(context, LPP_ICSP_CMD_CORE_INST, LPP_OP_MOVLW((value >> 8) & 0xFF))	&&
			lpp_icsp_write_16(context, LPP_ICSP_CMD_CORE_INST, LPP_OP_MOVWF(LPP_REG_TBLPTRH))		&& 
			lpp_icsp_write_16(context, LPP_ICSP_CMD_CORE_INST, LPP_OP_MOVLW((value >> 0) & 0xFF))	&&
			lpp_icsp_write_16(context, LPP_ICSP_CMD_CORE_INST, LPP_OP_MOVWF(LPP_REG_TBLPTRL));
}

/* write 16 bits of data to a specified address */
int lpp_write_16(struct lpp_context_t *context, 
				 const unsigned int address, 
				 const unsigned short data)	
{
	/* set TBLPTR and then execute the write */
	return lpp_tblptr_set(context, address) && 
			lpp_icsp_write_16(context, LPP_ICSP_CMD_TBL_WR_POST_INC, data);
}

/* read 16 bits of data from a specified address */
int lpp_read_16(struct lpp_context_t *context, 
				 const unsigned int address, 
				 unsigned short *data)	
{
	unsigned char lsb, msb;
	int ret = 0;

	/* set TBLPTR and then execute the write */
	if (lpp_tblptr_set(context, address) && 
			lpp_icsp_read_8(context, LPP_ICSP_CMD_TBL_RD_POST_INC, &lsb) &&
			lpp_icsp_read_8(context, LPP_ICSP_CMD_TBL_RD_POST_INC, &msb))
	{
		/* set data */
		*data = (msb | ((lsb & 0xFF) << 8));

		/* set result code to success */
		ret = 1;
	}

	/* return result */
	return ret;
}

/* perform bulk erase */
int lpp_bulk_erase(struct lpp_context_t *context)
{
	int ret;
	
	/* try to perform bulk erase */
	ret = lpp_write_16(context, 0x3C0005, 0x3F3F)								&&
			lpp_write_16(context, 0x3C0004, 0x8F8F)								&&
			lpp_icsp_write_16(context, LPP_ICSP_CMD_CORE_INST, LPP_OP_NOP)		&&
			lpp_icsp_write_16(context, LPP_ICSP_CMD_CORE_INST, LPP_OP_NOP);

	/* if success, wait */
	if (ret) usleep(20);

	/* return result */
	return ret;
}

/* read device id */
int lpp_device_id_read(struct lpp_context_t *context, unsigned short *device_id)
{
	/* return result */
	return lpp_read_16(context, 0x3FFFFE, device_id);
}

/* write an image to the device */
int lpp_program_image_to_device(struct lpp_context_t *context, struct lpp_image_t *image)
{
	int ret;
	unsigned int words_left, words_to_write, word_index;
	const unsigned int write_buffer_size_in_words = 16;
	unsigned short current_address, *current_data;

	/* start programming */
	ret = lpp_icsp_write_16(context, LPP_ICSP_CMD_CORE_INST, LPP_SET_EEPGD)	&& 
			lpp_icsp_write_16(context, LPP_ICSP_CMD_CORE_INST, LPP_CLR_CFGS);

	/* point to start data */
	current_data = (unsigned short *)image->contents;

	/* the amount of words left to write is (bytes / 2) + 1 if odd number of bytes */
	lpp_image_get_content_size_in_words(image, &words_left);

	/* while there are bytes left to write */
	for (current_address = 0; words_left && ret; words_left -= words_to_write)
	{
		/* do we have enough data to fill a buffer? if not, just write whatever's left */
		words_to_write = (write_buffer_size_in_words < words_left) ? 
							write_buffer_size_in_words : words_left;

		/* set the current address */
		ret = lpp_tblptr_set(context, current_address);

		/* fill the write buffer and write it */
		for (word_index = 0; (word_index < words_to_write) && ret; ++word_index, current_address += 2)
		{ 
			/* we don't increment the tblptr on the last word, so says the progspec */
			unsigned char command = (word_index != (words_to_write - 1)) ? 
										LPP_ICSP_CMD_TBL_WR_POST_INC_2 : LPP_ICSP_CMD_TBL_WR_PROG;

			/* set address and write data */
			ret = lpp_icsp_write_16(context, command, *current_data++);
		}

		/* perform the special nop procedure after programming */
		if (ret) lpp_icsp_prog_nop(context);
	}

	/* return the result */
	return ret;
}

/* read the image from the device */
int lpp_read_device_to_image(struct lpp_context_t *context, 
							 const unsigned int offset,
							 const unsigned int size_in_bytes,
							 struct lpp_image_t *image)
{
	unsigned char *current_position;
	unsigned int bytes_left, ret;

	/* success by default */
	ret = 1;

	/* allocate space */
	image->contents = malloc(size_in_bytes);

	/* if failed, exit */
	if (image->contents != NULL)
	{
		/* init buffer */
		memset(image->contents, 0xFF, size_in_bytes);

		/* point to start of buffer */
		current_position = image->contents;

		/* save the size */
		image->contents_size = image->max_contents_size = size_in_bytes;

		/* set the current address */
		ret = lpp_tblptr_set(context, offset);

		/* start reading */
		for (bytes_left = size_in_bytes; 
			  bytes_left && ret; 
			  --bytes_left, ++current_position)
		{
			/* read the data */
			ret = lpp_icsp_read_8(context, LPP_ICSP_CMD_TBL_RD_POST_INC, current_position);
		}
	}
	
	/* if failed, free the memory */
	if (ret == 0)
	{
		/* deallocate image and clear flags */
		free(image->contents);
		image->contents = NULL;
		image->contents_size = 0;
	}

	/* done */
	return ret;
}

