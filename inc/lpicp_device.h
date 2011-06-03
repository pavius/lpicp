/* 
 * Linux PIC Programmer (lpicp)
 * Device base header
 *
 * Author: Eran Duchan <pavius@gmail.com>
 *
 * This program is free software; you can redistribute  it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 */

#ifndef __LPICPC_DEVICE_H
#define __LPICPC_DEVICE_H

/* forward declare */
struct lpp_context_t;
struct lpp_image_t;

/* operations */
struct lpp_device_t
{
	/* callbacks */ 
    int (*open)(struct lpp_context_t *);
	int (*bulk_erase)(struct lpp_context_t *);
    int (*non_bulk_erase)(struct lpp_context_t *);
	int (*image_to_device)(struct lpp_context_t *, struct lpp_image_t *);
	int (*code_write_start)(struct lpp_context_t *);
	int (*config_write_start)(struct lpp_context_t *);

	/* vars */
	unsigned int code_words_per_write;
    unsigned int code_memory_size;
    unsigned int code_erase_page_size;
};

/* families of devices */
enum lpp_device_family_e
{
	LPP_DEVICE_FAMILY_18F,
};

/* get device structure by type */
struct lpp_device_t* lpp_device_get_by_type(struct lpp_context_t *context, 
											const enum lpp_device_family_e family);

#endif /* __LPICPC_DEVICE_H */
