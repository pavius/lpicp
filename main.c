/* 
 * Linux PIC Programmer (lpicp)
 * Command line interface
 *
 * Author: Eran Duchan <pavius@gmail.com>
 *
 * This program is free software; you can redistribute  it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 */

#include <stdio.h>
#include "lpicp.h"
#include "lpicp_log.h"
#include "lpicp_icsp.h"

/* entry */
int main(int argc, char *argv[])
{
	struct lpp_context_t context;
	unsigned short device_id;

	/* try to init context */
	if (lpp_context_init(&context, "/dev/icsp0") && 
		lpp_log_init(&context, 512))
	{
		/* do something */
		/* lpp_bulk_erase(&context); */
		lpp_device_id_read(&context, &device_id);

		/* print log */
		lpp_log_print(&context);

		/* destroy context */
		lpp_log_destroy(&context);
		lpp_context_destroy(&context);
	}

	/* done */
	return 0;
}
