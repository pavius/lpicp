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

#include "lpicp_device.h"

/* forward declare all structures */
extern struct lpp_device_t lpp_device_18f2xx_4xx;
extern struct lpp_device_t lpp_device_18f2xxx_4xxx;

/* read device id */
int lpp_device_id_read(struct lpp_context_t *context, unsigned short *device_id)
{
	/* return result */
	return lpp_read_16(context, 0x3FFFFE, device_id);
}

/* get device structure by type */
struct lpp_device_t* lpp_device_get_by_type(struct lpp_context_t *context, 
											const enum lpp_device_family_e family)
{
	unsigned short device_id;

	/* by family */
	if (family == LPP_DEVICE_FAMILY_18F)
	{
		/* get the device id */
		if (/* lpp_device_id_read(context, &device_id) */ 1)
		{
			/* by result TODO */
			return &lpp_device_18f2xx_4xx;
		}
	}
}

