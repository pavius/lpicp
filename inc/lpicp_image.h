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

#ifndef __LPICPC_IMAGE_H
#define __LPICPC_IMAGE_H

#include "lpicp.h"

/* image structure */
struct lpp_image_t
{
	unsigned char	*contents;
	unsigned int	contents_size; /* in bytes */
	unsigned int	max_contents_size; /* in bytes */
};

/* read a file into an image */
int lpp_image_read_from_file(struct lpp_context_t *context, 
							 struct lpp_image_t *image, 
							 const char *file_name,
							 const unsigned int max_image_size);

/* print an image to stdout */
int lpp_image_print(struct lpp_context_t *context, 
					struct lpp_image_t *image);

/* get image size in words */
#define lpp_image_get_content_size_in_words(image, size_in_words)	\
		*size_in_words = (image->contents_size >> 1);				\
		*size_in_words += (image->contents_size & 0x1) ? 1 : 0;

#endif /* __LPICPC_IMAGE_H */

