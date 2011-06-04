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
struct lpp_device_t;

/* device info */
struct lpp_device_group_t
{
    /* callbacks */ 
    int (*open)(struct lpp_context_t *);
    int (*bulk_erase)(struct lpp_context_t *);
    int (*non_bulk_erase)(struct lpp_context_t *);
    int (*image_to_device_program)(struct lpp_context_t *, struct lpp_image_t *);
    int (*image_to_device_config)(struct lpp_context_t *, struct lpp_image_t *);
    int (*code_write_start)(struct lpp_context_t *);
    int (*config_write_start)(struct lpp_context_t *);
};

/* operations */
struct lpp_device_t
{
    const char          *name;
    unsigned short      id;
    unsigned int        code_memory_size;
    unsigned int        code_words_per_write;
    unsigned int        code_erase_page_size;
    unsigned int        config_address;
    unsigned int        config_bytes;

    /* pointer to anything common to the group */
    struct lpp_device_group_t *group;
};

/* families of devices */
enum lpp_device_family_type_t
{
    LPP_DEVICE_FAMILY_18F,
};

/* get device structure by type */
int lpp_device_init_by_family(struct lpp_context_t *context, 
                              const enum lpp_device_family_type_t family);

#endif /* __LPICPC_DEVICE_H */

