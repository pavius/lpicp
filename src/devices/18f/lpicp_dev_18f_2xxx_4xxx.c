/* 
 * Linux PIC Programmer (lpicp)
 * Implementation of 18f2xxx/4xxx devices
 *
 * Author: Eran Duchan <pavius@gmail.com>
 *
 * This program is free software; you can redistribute  it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 */

#include "lpicp.h"
#include "lpicp_device.h"
#include "lpicp_icsp.h"
#include "lpicp_image.h"

/* perform bulk erase */
int lpp_device_18f2xxx_4xxx_bulk_erase(struct lpp_context_t *context)
{
	/* start the transaction */
	if (lpp_write_16(context, 0x3C0005, 0x3F3F) && 
			lpp_write_16(context, 0x3C0004, 0x8F8F))
	{
		/* set up the transaction */
		struct mc_icsp_cmd_only_t cmd_config = 
		{
			.command = 0x0,
			.pgc_value_after_cmd = 0,
			.pgd_value_after_cmd = 0,
			.mdelay = 5, /* P11 */
			.udelay = 0 
		};

		/* do the command only transaction */
		if (lpp_icsp_command_only(context, &cmd_config))
		{
			/* wait P10 */
			usleep(200);

			/* success */
			return 1;
		}
	}

	/* failed for some reason */
	return 0;
}

/* start writing to code memory */
int lpp_device_18f2xxx_4xxx_code_write_start(struct lpp_context_t *context)
{
	/* start programming */
	return lpp_exec_instruction(context, LPP_SET_EEPGD)	&& 
			lpp_exec_instruction(context, LPP_CLR_CFGS);
}

/* start writing to config memory */
int lpp_device_18f2xxx_4xxx_config_write_start(struct lpp_context_t *context)
{
	/* start config programming */
	return lpp_exec_instruction(context, LPP_SET_EEPGD)	&& 
			lpp_exec_instruction(context, LPP_SET_CFGS);
}

/* burn the image to the device */
int lpp_device_18f2xxx_4xxx_image_to_device(struct lpp_context_t *context, 
											struct lpp_image_t *image)
{
	int ret;
	unsigned int words_left, words_to_write, word_index;
	unsigned short current_address, *current_data;

	/* point to start data */
	current_data = (unsigned short *)image->contents;

	/* the amount of words left to write is (bytes / 2) + 1 if odd number of bytes */
	lpp_image_get_content_size_in_words(image, &words_left);

	/* start by entering code programming mode */
	ret = lpp_device_18f2xx_4xx_code_write_start(context);

	/* while there are bytes left to write */
	for (current_address = 0; words_left && ret; words_left -= words_to_write)
	{
		/* get words per writes */
		unsigned int write_buffer_size_in_words = context->device->code_words_per_write;

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
		if (ret)
		{
			/* set up the transaction */
			struct mc_icsp_cmd_only_t cmd_config = 
			{
				.command = 0x0,
				.pgc_value_after_cmd = 1,
				.pgd_value_after_cmd = 0,
				.mdelay = 1, /* P9 */
				.udelay = 250 /* add a bit to P9 */ 
			};

			/* send command only */
			ret = lpp_icsp_command_only(context, &cmd_config);
		}
	}

	/* return the result */
	return ret;
}

/* operations */
struct lpp_device_t lpp_device_18f2xxx_4xxx = 
{
	.bulk_erase 			= lpp_device_18f2xxx_4xxx_bulk_erase,
	.image_to_device 		= lpp_device_18f2xxx_4xxx_image_to_device,
	.code_words_per_write	= 16
};

