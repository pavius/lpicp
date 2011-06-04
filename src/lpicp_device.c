/* 
 * Linux PIC Programmer (lpicp)
 * Device base implementation
 *
 * Author: Eran Duchan <pavius@gmail.com>
 *
 * This program is free software; you can redistribute  it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 */

#include <stdlib.h>
#include "lpicp.h"
#include "lpicp_device.h"

/* forward declare all structures */
extern struct lpp_device_group_t lpp_device_18f2xx_4xx;
extern struct lpp_device_group_t lpp_device_18f2xxx_4xxx;

/* read device id */
int lpp_device_id_read(struct lpp_context_t *context, unsigned short *device_id)
{
	/* return result */
	return lpp_read_16(context, 0x3FFFFE, device_id);
}

/* get device structure by type */
int lpp_device_init_by_family(struct lpp_context_t *context, 
                              const enum lpp_device_family_type_t family)
{
	unsigned short device_id;

	/* by family */
	if (family == LPP_DEVICE_FAMILY_18F)
	{
		/* get the device id */
		if (lpp_device_id_read(context, &context->device.id))
		{
            /* get devs */
            const unsigned char dev2 = (context->device.id & 0xFF);

            /* by dev2 */
            switch (dev2)
            {
                /* 2xx/4xx */
                case 0x4:
                case 0x8:
                    context->device.group = &lpp_device_18f2xx_4xx;
                    break;
            }
		}
	}

    /* open the device */
    if (context->device.group) context->device.group->open(context);

    /* success if a group has been assigned */
    return (context->device.group != NULL);
}

