/* 
 * Linux PIC Programmer (lpicp)
 * Microchip ICSP driver interface
 *
 * Author: Eran Duchan <pavius@gmail.com>
 *
 * This program is free software; you can redistribute  it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/mc_icsp.h>
#include "lpicp_icsp.h"
#include "lpicp_log.h"

/* open access to driver */
int lpp_icsp_init(struct lpp_context_t *context, char *icsp_dev_name)
{
	/* open ICSP driver */
	context->icsp_dev_file = open(icsp_dev_name, O_RDWR);

	/* check */
	if (context->icsp_dev_file >= 0)
	{
		/* save name */
		context->icsp_dev_name = icsp_dev_name;
	}
	else
	{
		/* failed */
		printf("Failed to open ICSP driver @ %s\n", icsp_dev_name);

		/* failed */
		goto err_icsp_open;
	}

	/* success */
	return 1;

err_icsp_open:
	return 0;
}

/* close access to driver */
int lpp_icsp_destroy(struct lpp_context_t *context)
{
	/* check if file exists */
	if (context->icsp_dev_file)
	{
		/* close the file */
		close(context->icsp_dev_file);
	}

	/* nullify */
	context->icsp_dev_file = -1;
	context->icsp_dev_name = NULL;

	/* success */
	return 1;
}

/* execute a command via ICSP driver */
int lpp_icsp_write_16(struct lpp_context_t *context, 
					  const unsigned char command, 
					  const unsigned short data)
{
	unsigned int xfer_command = 0, ret;

	/* log write, if applicable */
	lpp_log_command(context, command, data);

	/* encode the xfer */
	MC_ICSP_ENCODE_XFER(command, data, xfer_command);

	/* tx */
	return (ioctl(context->icsp_dev_file, MC_ICSP_IOC_TX, &xfer_command) == 0);
}

/* Read 8 bits via ICSP driver */
int lpp_icsp_read_8(struct lpp_context_t *context, 
					const unsigned char command, 
					unsigned char *data)
{
	unsigned int xfer_command = 0;
	int ret;

	/* encode the xfer */
	MC_ICSP_ENCODE_XFER(command, 0, xfer_command);

	/* rx */
	ret = (ioctl(context->icsp_dev_file, MC_ICSP_IOC_RX, &xfer_command) == 0);

	/* get LSB */
	*data = (xfer_command & 0xFF);

	/* log read, if applicable */
	lpp_log_command(context, command, (unsigned short)*data);

	/* return result */
	return ret;
}

