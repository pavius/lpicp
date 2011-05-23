/* 
 * Linux PIC Programmer (lpicp)
 * Logger implementation
 *
 * Author: Eran Duchan <pavius@gmail.com>
 *
 * This program is free software; you can redistribute  it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 */

#include "lpicp_log.h"
#include "lpicp_icsp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* command log record */
struct lpp_log_record_t
{
	unsigned char command;
	unsigned short data;
};

/* print the log - assumes string is 5 bytes long, at least */
void lpp_log_format_cmd_string(const unsigned char cmd, 
							   char *cmd_string,
							   const unsigned int cmd_string_length)
{
	unsigned int char_idx, required_chars;
	unsigned char cmd_mask;

	/* point to MSb */
	cmd_mask = (1 << (LPP_COMMAND_BIT_COUNT - 1));

	/* 4 bits + null term */
	required_chars = LPP_COMMAND_BIT_COUNT + 1; 

	/* verify string length */
	if (cmd_string_length >= required_chars)
	{
		/* make bits */
		for (char_idx = 0; char_idx < required_chars - 1; ++char_idx, cmd_mask >>= 1)
		{
			/* set bit char */
			*cmd_string++ = (cmd & cmd_mask) ? '1' : '0';
		}

		/* terminate */
		*cmd_string = '\0';	
	}
}

/* print the log */
void lpp_log_print(struct lpp_context_t *context)
{
	unsigned int record_idx;
	char cmd_string[LPP_COMMAND_BIT_COUNT + 1];

	/* iterat records */
	for (record_idx = 0; record_idx < context->log_current_idx; ++record_idx)
	{
		/* get formatted string */
		lpp_log_format_cmd_string(context->log_records[record_idx].command, 
								  cmd_string,
								  sizeof(cmd_string));

		/* print it */
		printf("[%.3d] cmd(%s) data(%04X)\n", record_idx, 
				cmd_string,
				context->log_records[record_idx].data);
	}
}

/* log a command */
int lpp_log_command(struct lpp_context_t *context, 
					const unsigned char command, 
					const unsigned short data)
{
	/* check if room available */
	if (context->log_current_idx < context->log_record_count)
	{
		/* log the command */
		context->log_records[context->log_current_idx].command = command;
		context->log_records[context->log_current_idx].data = data;

		/* point to next record */
		context->log_current_idx++;

		/* ok */
		return 1;
	}
	else
	{
		/* no room */
		return 0;
	}
}

/* initialize a context */
int lpp_log_init(struct lpp_context_t *context,
				 const unsigned int log_record_count)
{
	int log_size_bytes;

	/* init current log record */
	context->log_current_idx = 0;

	/* calc how big the log is, in bytes */
	log_size_bytes = sizeof(struct lpp_log_record_t) * log_record_count;

	/* try to allocate log */
	context->log_records = malloc(log_size_bytes);

	/* check allocation */
	if (context->log_records == NULL)
	{
		/* exit */
		goto err_context_allocation;
	}

	/* save command log info */
	context->log_record_count = log_record_count;

	/* init command log */
	memset(context->log_records, 0, log_size_bytes);

	/* success */
	return 1;

err_context_allocation:
	return 0;
}

/* destroy a context */
int lpp_log_destroy(struct lpp_context_t *context)
{
	/* free memory if allocated */
	if (context->log_records)
	{
		/* free memory */
		free(context->log_records);
		context->log_records = NULL;
	}

	/* init counters */
	context->log_record_count = 0;
	context->log_current_idx = 0;

	/* success */
	return 1;
}
