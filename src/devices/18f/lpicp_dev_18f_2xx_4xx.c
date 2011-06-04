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

#include <stdio.h>

/* initialize the device by id */
int lpp_device_18f2xx_4xx_open(struct lpp_context_t *context)
{
    const unsigned char dev1 = (context->device.id >> 8) & 0xFF;

    /* by sub type (get 3 MSb) */
    switch ((dev1 & 0XE0) >> 5)
    {
        /* 001: PIC18F452 */
        case 0x1: 
            context->device.code_words_per_write   = 4;
            context->device.code_erase_page_size   = 64;
            context->device.code_memory_size       = 32 * 1024;
            context->device.config_address         = 0x300000;
            context->device.config_bytes           = 14;
            context->device.name                   = "PIC18F452";
            break;

        /* unsupported */
        default: return 0;
    }

    /* found */
    return 1;
}

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

/* perform erase chip without using bulk */
int lpp_device_18f2xx_4xx_non_bulk_erase(struct lpp_context_t *context)
{
    unsigned int current_address, ret;

    /* initial success */
    ret = 1;

    /* start by disabling multipanel writes and entering erase mode */
	if (lpp_device_18f2xx_4xx_config_write_start(context)   &&
		lpp_write_16(context, 0x3C0006, 0x0000))
        
    {
        /* iterate through the pages */
        for (current_address = 0; 
              current_address < context->device.code_memory_size && ret; 
              current_address += context->device.code_erase_page_size)
        {
            /* enter erase mode, set the current address and do the programming */
            ret = lpp_exec_instruction(context, LPP_SET_EEPGD)                  &&
                    lpp_exec_instruction(context, LPP_CLR_CFGS)                 &&
                    lpp_exec_instruction(context, LPP_SET_FREE)                 &&        
                    lpp_tblptr_set(context, current_address)                    && 
                    lpp_icsp_write_16(context, LPP_ICSP_CMD_TBL_WR_PROG, 0x0);

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

                /* send special command only, then wait P10, then 16 0s of data */
                ret = lpp_icsp_command_only(context, &cmd_config) && 
                        lpp_icsp_delay_us(context, 5) && 
                        lpp_icsp_data_only(context, 0x0);
            }

            /* progress notification */
            if (context->ntfy_progress)
                context->ntfy_progress(context, current_address, context->device.code_memory_size);
        }

        /* progress notification */
        if (context->ntfy_progress)
            context->ntfy_progress(context, current_address, context->device.code_memory_size);
    }

    /* success */
    return 1;
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
int lpp_device_18f2xx_4xx_image_to_device_config(struct lpp_context_t *context, 
                                                 struct lpp_image_t *image)
{
    unsigned int ret, config_byte_idx;

    /* enter config write, set PC and set the config address */
    ret = lpp_exec_instruction(context, LPP_SET_EEPGD)	                &&
			lpp_exec_instruction(context, LPP_SET_CFGS)                 && 
            lpp_exec_instruction(context, LPP_SET_PC_100K_0)            && 
            lpp_exec_instruction(context, LPP_SET_PC_100K_1)            && 
            lpp_tblptr_set(context, context->device.config_address);

    /* start reading, two bytes at a time */
    for (config_byte_idx = 0; 
          config_byte_idx < context->device.config_bytes && ret; 
          ++config_byte_idx)
    {
        unsigned short value = 0;

        /* valid configuration byte? */
        if (image->config_valid & (1 << config_byte_idx))
        {
            /* odd address? */
            if (config_byte_idx & 0x1)
            {
                /* odd addresses uses msb */
                value = (image->config[config_byte_idx] << 8);
            }
            else
            {
                /* even addresses uses lsb */
                value = image->config[config_byte_idx];
            }
    
            /* write the data (only one byte has actual data) */
            ret = lpp_icsp_write_16(context, 
                                    LPP_ICSP_CMD_TBL_WR_PROG, 
                                    value);

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

                /* send special command only, then wait P10, then 16 0s of data */
                ret = lpp_icsp_command_only(context, &cmd_config) && 
                        lpp_icsp_delay_us(context, 5) && 
                        lpp_icsp_data_only(context, 0x0);
            }
        }

        /* increment tblptr by one byte */
        ret = ret && lpp_exec_instruction(context, LPP_INC_TBLPTRL);
    }

    /* done */
    return ret;
}

/* burn the image to the device */
int lpp_device_18f2xx_4xx_image_to_device_program(struct lpp_context_t *context, 
                                                  struct lpp_image_t *image)
{
	/* start by disabling multipanel writes */
	if (lpp_device_18f2xx_4xx_config_write_start(context) &&
		lpp_write_16(context, 0x3C0006, 0x0000))
	{
		/* re-use 2xxx_4xxx method */
		return (lpp_device_18f2xxx_4xxx_image_to_device_program(context, image));
	}
    else return 0;
}

/* operations */
struct lpp_device_group_t lpp_device_18f2xx_4xx = 
{
    .open                       = lpp_device_18f2xx_4xx_open,
	.bulk_erase 			    = lpp_device_18f2xx_4xx_bulk_erase,
    .non_bulk_erase 		    = lpp_device_18f2xx_4xx_non_bulk_erase,
	.image_to_device_program 	= lpp_device_18f2xx_4xx_image_to_device_program,
    .image_to_device_config 	= lpp_device_18f2xx_4xx_image_to_device_config,
	.code_write_start		    = lpp_device_18f2xx_4xx_code_write_start,
	.config_write_start		    = lpp_device_18f2xx_4xx_config_write_start
};

