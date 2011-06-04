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

#ifndef __LPICPC_ICSP_H
#define __LPICPC_ICSP_H

#include "lpicp.h"
#include <linux/mc_icsp.h>

/* number of bits in command */
#define LPP_COMMAND_BIT_COUNT (4)

/* open access to driver */
int lpp_icsp_init(struct lpp_context_t *context, char *icsp_dev_name);

/* close access to driver */
int lpp_icsp_destroy(struct lpp_context_t *context);

/* Write 16 bits via ICSP driver */
int lpp_icsp_write_16(struct lpp_context_t *context, 
                    const unsigned char command, 
                    const unsigned short data);

/* Read 8 bits via ICSP driver */
int lpp_icsp_read_8(struct lpp_context_t *context, 
                    const unsigned char command, 
                    unsigned char *data);

/* send only command */
int lpp_icsp_command_only(struct lpp_context_t *context, 
                          const struct mc_icsp_cmd_only_t *cmd_config);

/* send only data */
int lpp_icsp_data_only(struct lpp_context_t *context, 
                       const unsigned int data);

/* delay and return success */
int lpp_icsp_delay_us(struct lpp_context_t *context, const unsigned int delay_us);

#endif /* __LPICPC_ICSP_H */

