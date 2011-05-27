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

		/* read the record to the proper offset */
		memcpy(image->contents + hex_record->address, 
			   hex_record->data, 
			   hex_record->dataLen);

		/* check if we passed the high address watermark */
		if (write_end_address > image->contents_size) 
			image->contents_size = write_end_address;
	}
	/* config space. TODO: make devince unspecific */
	else if (address_ext == 0x3000)
	{
	}

	/* done */
	return 1;

err_not_enough_space:
	return 0;
}

/* read a file into an image */
int lpp_image_read_from_file(struct lpp_context_t *context, 
							 struct lpp_image_t *image, 
							 const char *file_name,
							 const unsigned int max_image_size)
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
		/* allocate enough memory to hold the file */
		image->contents = malloc(max_image_size);

		/* succeeded? */
		if (image->contents == NULL)
		{
			/* error */
			printf("Failed to allocate %d bytes for contents\n", max_image_size);
			goto err_alloc_contents;
		}

		/* initialize counters and contents */
		memset(image->contents, 0xFF, max_image_size);
		image->max_contents_size = max_image_size;
		image->contents_size = 0;

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
					memcpy(&address_ext, hex_record.data, sizeof(address_ext));
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
	free(image->contents);
err_alloc_contents:
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
