/* 
 * Linux PIC Programmer (lpicp)
 * Implementation of 18f2xx/4xx devices
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
int lpp_device_18f2xx_4xx_bulk_erase(struct lpp_context_t *context)
{
	/* start the transaction */
	if (lpp_write_16(context, 0x3C0004, 0x0080) && 
		lpp_exec_instruction(context, LPP_OP_NOP))
	{
		/* set up the transaction */
		struct mc_icsp_cmd_only_t cmd_config = 
		{
			.command = 0x0,
			.pgc_value_after_cmd = 0,
			.pgd_value_after_cmd = 0,
			.mdelay = 10, /* P11 */
			.udelay = 0 
		};

		/* do the command only transaction */
		if (lpp_icsp_command_only(context, &cmd_config))
		{
			/* wait P10 */
			usleep(50);

			/* success */
			return 1;
		}
	}

	/* failed for some reason */
	return 0;
}

/* start writing to code memory */
int lpp_device_18f2xx_4xx_code_write_start(struct lpp_context_t *context)
{
	/* start programming */
	return lpp_exec_instruction(context, LPP_SET_EEPGD)	&& 
			lpp_exec_instruction(context, LPP_CLR_CFGS);
}

/* start writing to config memory */
int lpp_device_18f2xx_4xx_config_write_start(struct lpp_context_t *context)
{
	/* start programming */
	return lpp_exec_instruction(context, LPP_SET_EEPGD)	&&
			lpp_exec_instruction(context, LPP_SET_CFGS) &&
			lpp_exec_instruction(context, LPP_SET_WREN);
}

/* burn the image to the device */
int lpp_device_18f2xx_4xx_image_to_device(struct lpp_context_t *context, 
										  struct lpp_image_t *image)
{
	/* start by disabling multipanel writes */
	if (lpp_device_18f2xx_4xx_config_write_start(context) &&
		lpp_write_16(context, 0x3C0006, 0x0000))
	{
		/* re-use 2xxx_4xxx method */
		return (lpp_device_18f2xxx_4xxx_image_to_device(context, image));
	}
}

/* operations */
struct lpp_device_t lpp_device_18f2xx_4xx = 
{
	.bulk_erase 			= lpp_device_18f2xx_4xx_bulk_erase,
	.image_to_device 		= lpp_device_18f2xx_4xx_image_to_device,
	.code_write_start		= lpp_device_18f2xx_4xx_code_write_start,
	.config_write_start		= lpp_device_18f2xx_4xx_config_write_start,
	.code_words_per_write	= 4
};

