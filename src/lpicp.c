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
                     const enum lpp_device_family_type_t family,
                     char *icsp_dev_name,
                     ntfy_progress_t ntfy_progress)
{
    /* init structure */
    memset(context, 0, sizeof(struct lpp_context_t));

    /* try to open the driver */
    if (lpp_icsp_init(context, icsp_dev_name))
    {    
        /* save callbacks and data */
        context->ntfy_progress = ntfy_progress;

        /* try to get the device */
        return lpp_device_init_by_family(context, family);
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
    return lpp_exec_instruction(context, LPP_OP_MOVLW((value >> 16) & 0xFF))    && 
            lpp_exec_instruction(context, LPP_OP_MOVWF(LPP_REG_TBLPTRU))        && 
            lpp_exec_instruction(context, LPP_OP_MOVLW((value >> 8) & 0xFF))    &&
            lpp_exec_instruction(context, LPP_OP_MOVWF(LPP_REG_TBLPTRH))        && 
            lpp_exec_instruction(context, LPP_OP_MOVLW((value >> 0) & 0xFF))    &&
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
    ret = context->device.group->bulk_erase(context);

    /* if success, wait */
    if (ret) usleep(20);

    /* return result */
    return ret;
}

/* perform bulk erase */
int lpp_non_bulk_erase(struct lpp_context_t *context)
{
    int ret;
    
    /* delegate to device */
    ret = context->device.group->non_bulk_erase(context);

    /* if success, wait */
    if (ret) usleep(20);

    /* return result */
    return ret;
}

/* write an image to the device */
int lpp_write_image_to_device_program(struct lpp_context_t *context, struct lpp_image_t *image)
{
    /* delegate to device */
    return context->device.group->image_to_device_program(context, image);
}

/* write an image to the device config */
int lpp_write_image_to_device_config(struct lpp_context_t *context, struct lpp_image_t *image)
{
    /* delegate to device */
    return context->device.group->image_to_device_config(context, image);
}

/* read the image program from the device */
int lpp_read_device_program_to_image(struct lpp_context_t *context, 
                                     const unsigned int offset,
                                     const unsigned int size_in_bytes,
                                     struct lpp_image_t *image)
{
    unsigned short *current_position;
    unsigned int words_left, total_words, ret;

    /* success by default */
    ret = 1;

    /* point to start of buffer */
    current_position = (unsigned short *)image->contents;

    /* save the size */
    image->contents_size = size_in_bytes;

    /* make sure we have room */
    if (image->contents_size <= image->max_contents_size)
    {
        /* the amount of words left to write is (bytes / 2) + 1 if odd number of bytes */
        lpp_image_get_content_size_in_words(image, &total_words);
    
        /* set the current address (auto increment) */
        ret = lpp_tblptr_set(context, offset);
    
        /* start reading, two bytes at a time */
        for (words_left = total_words; words_left && ret; --words_left, ++current_position)
        {
            unsigned short read_word = 0;
            unsigned char msb, lsb;
    
            /* read the data */
            ret = lpp_icsp_read_8(context, LPP_ICSP_CMD_TBL_RD_POST_INC, &lsb) && 
                  lpp_icsp_read_8(context, LPP_ICSP_CMD_TBL_RD_POST_INC, &msb);
    
            /* make sure its in the correct order */
            read_word = ((msb << 8) | lsb);
    
            /* save */
            *current_position = read_word;
    
            /* progress notification */
            if (ret && context->ntfy_progress)
                context->ntfy_progress(context, (total_words - words_left) * 2, size_in_bytes);
        }
    
        /* progress notification */
        if (ret && context->ntfy_progress)
            context->ntfy_progress(context, size_in_bytes, size_in_bytes);
    }
    else
    {
        /* not enough room in image buffer */
        ret = 0;
    }
    
    /* done */
    return ret;
}

/* read the image config from the device */
int lpp_read_device_config_to_image(struct lpp_context_t *context,
                                    struct lpp_image_t *image)
{
    unsigned int ret, config_byte_idx;

    /* set the config address */
    ret = lpp_tblptr_set(context, context->device.config_address);

    /* start reading, two bytes at a time */
    for (config_byte_idx = 0; 
          config_byte_idx < context->device.config_bytes && ret; 
          ++config_byte_idx)
    {
        /* read the data */
        ret = lpp_icsp_read_8(context, 
                              LPP_ICSP_CMD_TBL_RD_POST_INC, 
                              &image->config[config_byte_idx]);

        /* set the appropriate config byte */
        image->config_valid |= (1 << config_byte_idx);
    }

    /* done */
    return ret;
}

/* read the image eeprom from the device */
int lpp_read_device_eeprom_to_image(struct lpp_context_t *context,
                                    struct lpp_image_t *image)
{
    /* delegate to device */
    return context->device.group->device_eeprom_to_image(context, image);
}

/* read the image eeprom from the device */
int lpp_read_image_to_device_eeprom(struct lpp_context_t *context,
                                    struct lpp_image_t *image)
{
    /* delegate to device */
    return context->device.group->image_to_device_eeprom(context, image);
}

