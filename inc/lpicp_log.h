/* 
 * Linux PIC Programmer (lpicp)
 * Logger header
 *
 * Author: Eran Duchan <pavius@gmail.com>
 *
 * This program is free software; you can redistribute  it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 */

#ifndef __LPICPC_LOG_H
#define __LPICPC_LOG_H

#include "lpicp.h"

/* print the log */
void lpp_log_print(struct lpp_context_t *context);

/* log a command */
int lpp_log_command(struct lpp_context_t *context, 
                    const unsigned char command, 
                    const unsigned short data);

/* initialize a context */
int lpp_log_init(struct lpp_context_t *context,
                 const unsigned int cmd_log_record_count);

/* destroy a context */
int lpp_log_destroy(struct lpp_context_t *context);

#endif /* __LPICPC_LOG_H */
