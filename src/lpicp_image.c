/* 
 * Linux PIC Programmer (lpicp)
 * Image loading module
 *
 * Author: Eran Duchan <pavius@gmail.com>
 *
 * This program is free software; you can redistribute  it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 */

#include "lpicp_image.h"
#include "ihex.h"
#include <stdio.h>

/* initialize an image */
int lpp_image_init(struct lpp_context_t *context, 
                   struct lpp_image_t *image,
                   const unsigned int max_size)
{
    /* zero out the image */
    memset(image, 0, sizeof(struct lpp_image_t));

    /* allocate contents */
    image->contents = malloc(max_size);
    
    /* check allocation */
    if (image->contents != NULL)
    {
        /* init buffer */
        memset(image->contents, 0xFF, max_size);

        /* save sizes */
        image->max_contents_size = max_size;
    }
}

/* destroy an image */
int lpp_image_destroy(struct lpp_context_t *context, 
                      struct lpp_image_t *image)
{                   
    /* check if we have contents */
    if (image->contents != NULL)
    {
        /* free the image */
        free(image->contents);
    }

    /* zero out the image */
    memset(image, 0, sizeof(struct lpp_image_t));
}

/* record to big endian */
void lpp_image_data_record_to_big_endian(struct lpp_context_t *context, 
                                         IHexRecord *hex_record)
{
    unsigned int byte_index;

    /* doesn't support records with odd number of bytes */
    /* lpp_assert(hex_record->dataLen & 0x1 == 0, 
               "HEX file record must contain even number of bytes"); */

    /* swap all bytes @ N, N+1 */
    for (byte_index = 0; byte_index < hex_record->dataLen; byte_index += 2)
    {   
        /* save original */
        unsigned char original_byte_n = hex_record->data[byte_index];

        /* do swap */
        hex_record->data[byte_index] = hex_record->data[byte_index + 1];
        hex_record->data[byte_index + 1] = original_byte_n;
    }
}

/* handle record type */
int lpp_image_read_handle_data_record(struct lpp_context_t *context, 
                                      struct lpp_image_t *image, 
                                      const unsigned short address_ext,
                                      IHexRecord *hex_record)
{
    /* check address extension */
    if (address_ext == 0)
    {
        /* get the address at which this block ends */
        unsigned int write_end_address = hex_record->address + hex_record->dataLen;

        /* is there enough room for this data? */
        if (write_end_address > image->max_contents_size)
        {
            /* not enough room for records */
            printf("Not enough space @ address %04X\n", hex_record->address);
            goto err_not_enough_space;
        }

        /* convert record to big endian */
        lpp_image_data_record_to_big_endian(context, hex_record);

        /* read the record to the proper offset */
        memcpy(image->contents + hex_record->address, 
               hex_record->data, 
               hex_record->dataLen);

        /* check if we passed the high address watermark */
        if (write_end_address > image->contents_size) 
            image->contents_size = write_end_address;
    }
    /* config space. TODO: make device unspecific */
    else if (address_ext == 0x3000)
    {
        /* write to configuration */
        if ((hex_record->address + hex_record->dataLen) <= sizeof(image->config))
        {
            unsigned int valid_byte_idx;

            /* copy configuration to offset */
            memcpy(&image->config[hex_record->address], 
                   hex_record->data,
                   hex_record->dataLen);

            /* set valid bits, indicating that this configuration byte has been read from source */
            for (valid_byte_idx = hex_record->address;
                  valid_byte_idx < hex_record->address + hex_record->dataLen;
                  ++valid_byte_idx)
            {
                /* set appropriate byte valid bit */
                image->config_valid |= (1 << valid_byte_idx);
            }
        }
        /* can't store this, not supported */
        else goto err_not_enough_space;
    }

    /* done */
    return 1;

err_not_enough_space:
    return 0;
}

/* read a file into an image */
int lpp_image_read_from_file(struct lpp_context_t *context, 
                             struct lpp_image_t *image, 
                             const char *file_name)
{
    FILE *hex_file;
    IHexRecord hex_record;
    enum IHexErrors record_read_result;
    unsigned short address_ext = 0;

    /* try to open the file */
    hex_file = fopen(file_name, "r");

    /* success? */
    if (hex_file)
    {
        /* read records */
        while (1)
        {
            /* read the record */
            record_read_result = Read_IHexRecord(&hex_record, hex_file);

            /* if no error, read to image */
            if (record_read_result == IHEX_OK)
            {
                /* data record */
                if (hex_record.type == IHEX_TYPE_00)
                {
                    /* handle the data record */
                    if (!lpp_image_read_handle_data_record(context, image, address_ext, &hex_record))
                    {
                        /* error */
                        goto err_handling_data_record;
                    }
                }
                /* set address MSb */
                else if (hex_record.type == IHEX_TYPE_04)
                {
                    /* set the extended address */
                    address_ext = hex_record.data[0];
                    address_ext |= (hex_record.data[1] << 8);
                }
                /* end record */
                else if (hex_record.type == IHEX_TYPE_01)
                {
                    break;
                }
            }
            /* end of file? */
            else if (record_read_result == IHEX_ERROR_EOF)
            {
                /* if we got here, we're done */
                break;
            }
            /* some other error */
            else
            {
                /* error */
                printf("Failed to parse file. err(%d)\n", record_read_result);
                goto err_parse_file;
            }
        }
    }
    else
    {
        /* error */
        printf("Failed to open file @ %s\n", file_name);
        goto err_alloc_open_file;
    }

    /* close file */
    fclose(hex_file);

    /* success */
    return 1;

err_handling_data_record:
err_parse_file:
    fclose(hex_file);
err_alloc_open_file:
    return 0;    
}

/* write the image to file */
int lpp_image_write_to_file(struct lpp_context_t *context, 
                             struct lpp_image_t *image, 
                             const char *file_name)
{
    /* TODO */
}

/* print an image to stdout */
int lpp_image_print(struct lpp_context_t *context, 
                    struct lpp_image_t *image)
{
    unsigned int byte_idx, row_address, config_byte_idx;
    const unsigned int row_byte_count = 16;

    /* iterate through the bytes */
    for (row_address = 0, byte_idx = 0; 
          byte_idx < image->contents_size; 
          ++byte_idx)
    {
        /* starting new row? */
        if ((byte_idx & (row_byte_count - 1)) == 0)
        {
            /* row header */
            printf("\n[%04X] ", row_address);

            /* next address */
            row_address += row_byte_count;
        }

        /* print the byte */
        printf("%02X", image->contents[byte_idx]);
    }

    /* space out */
    printf("\n.\n");

    /* print configuration bytes */
    for (config_byte_idx = 0;
          config_byte_idx < LPP_MAX_CONFIG_BITS;
          ++config_byte_idx)
    {
        /* is the byte valid? */
        if (image->config_valid & (1 << config_byte_idx))
        {
            /* print config byte */
            printf("CFG%02X: %02X\n", config_byte_idx, image->config[config_byte_idx]);
        }
    }

    /* done */
    printf("\n");
}

