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
#include "lpicp_device.h"
#include <string.h>
#include <stdlib.h>

/* initialize a context */
int lpp_context_init(struct lpp_context_t *context,
					 const enum lpp_device_family_e family,
					 char *icsp_dev_name)
{
	/* init structure */
	memset(context, 0, sizeof(struct lpp_context_t));

	/* try to open the driver */
	if (lpp_icsp_init(context, icsp_dev_name))
	{	
		/* try to get the device */
		context->device = lpp_device_get_by_type(context, family);
	}
	else return 0;
}

/* destroy a context */
int lpp_context_destroy(struct lpp_context_t *context)
{
	/* try to close the driver */
	return lpp_icsp_destroy(context);
}

/* execute an instruction */
int lpp_exec_instruction(struct lpp_context_t *context,
						 const unsigned short instuction)
{
	/* perform the instruction */
	return lpp_icsp_write_16(context, LPP_ICSP_CMD_CORE_INST, instuction);
}

/* set tbpltr register */
int lpp_tblptr_set(struct lpp_context_t *context, 
				   const unsigned int value)
{
	/* set TBLPTRU, TBLPTRH and TBLPTRL */
	return lpp_exec_instruction(context, LPP_OP_MOVLW((value >> 16) & 0xFF))	&& 
			lpp_exec_instruction(context, LPP_OP_MOVWF(LPP_REG_TBLPTRU))		&& 
			lpp_exec_instruction(context, LPP_OP_MOVLW((value >> 8) & 0xFF))	&&
			lpp_exec_instruction(context, LPP_OP_MOVWF(LPP_REG_TBLPTRH))		&& 
			lpp_exec_instruction(context, LPP_OP_MOVLW((value >> 0) & 0xFF))	&&
			lpp_exec_instruction(context, LPP_OP_MOVWF(LPP_REG_TBLPTRL));
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
	
	/* delegate to device */
	ret = context->device->bulk_erase(context);

	/* if success, wait */
	if (ret) usleep(20);

	/* return result */
	return ret;
}

/* write an image to the device */
int lpp_program_image_to_device(struct lpp_context_t *context, struct lpp_image_t *image)
{
	/* delegate to device */
	return context->device->image_to_device(context, image);
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

