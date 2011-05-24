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
#include <getopt.h>
#include "lpicp.h"
#include "lpicp_log.h"
#include "lpicp_icsp.h"

/* current version */
const char *version_string = "0.0.1";

/* whether to read or write */
enum lpicp_opmode_t
{ 
	LPICP_OPMODE_UNDEFINED,
	LPICP_OPMODE_READ,
	LPICP_OPMODE_WRITE,
};

/* running configuration */
struct lpicp_config_t
{
	int verbose;
	char *dev_name;
	enum lpicp_opmode_t opmode;
	unsigned int offset;
};

/* initialize default configuration */
void lpicp_main_init_default_config(struct lpicp_config_t *config)
{
	config->verbose = 0;
	config->dev_name = NULL;
	config->opmode = LPICP_OPMODE_UNDEFINED;
	config->offset = 0;
}

/* print usage */
void lpicp_main_usage(void)
{
	printf("Linux PIC Programmer v.%s\n", version_string);
	printf("Usage: lpicp [options]\n");
	printf("  -r, --read            Read PIC non-volatile memory to file\n");
	printf("  -w, --write           Write file to PIC non-volatile memory\n");
	printf("  -d, --dev             ICSP device name (e.g. /dev/icsp0)\n");
	printf("  -o, --offset          Read from offset, Write to offset\n");
	printf("  -v, --verbose         Verbose operation\n");
	printf("  -h, --help            Prints this usage\n");
	printf("\n");
}

/* parse arguments to configuration */
int lpicp_main_parse_args_to_config(int argc, char *argv[], struct lpicp_config_t *config)
{
	/* iterate over options */ 
	while (1)
	{
		/* holds teh current option */
		int current_option;

		/* declare options */
		static struct option long_options[] =
		{
			/* name			has-arg		flag			val */
			{"verbose", 	0,  		0, 				'v'},
			{"read",     	0,  		0, 				'r'},
			{"write",  		0,  		0, 				'w'},
			{"help",  		0,  		0, 				'h'},
			{"dev",  		1, 			0, 				'd'},
			{"offset",  	1, 			0, 				'o'},
			{0, 0, 0, 0}
		};

		/* getopt_long stores the option index here. */
		int option_index = 0;

		/* get the options */
		current_option = getopt_long (argc, argv, "hvrwd:o:", long_options, &option_index);

		/* Detect the end of the options. */
		if (current_option == -1)
			break;

		/* get option */
		switch (current_option)
		{
			/* value */
			case 0:
				break;

			/* verbose */
			case 'v':
			{
				/* save flag */
				config->verbose = 1;
			}
			break;

			/* read */
			case 'r':
			{
				/* set mode */
				config->opmode = LPICP_OPMODE_READ;
			}
			break;

			/* write */
			case 'w':
			{
				/* set mode */
				config->opmode = LPICP_OPMODE_WRITE;
			}
			break;

			/* file */
			case 'd':
			{
				/* save file name */
				config->dev_name = optarg;
			}
			break;

			/* help */
			case 'h':
			{
				/* print usage */
				lpicp_main_usage();

				/* dont' execute the config */
				return 0;
			}
		}
	}

	/* go ahead with the configuration */
	return 1;
}

/* parse arguments to configuration */
int lpicp_main_execute_config(struct lpicp_config_t *config)
{
	struct lpp_context_t context;
	int ret;

	/* assume no error */
	ret = 0;

	/* try to init context */
	if (lpp_context_init(&context, config->dev_name))
	{
		/* init log if verbose */
		if (config->verbose)
		{
			/* try to init log */
			if (!lpp_log_init(&context, 512))
			{
				/* set err */
				ret = 1;

				/* error */
				goto err_log_init;
			}
		}

		/* read? */
		if (config->opmode == LPICP_OPMODE_READ)
		{
		}
		/* write */
		else if (config->opmode == LPICP_OPMODE_WRITE)
		{
		}

		/* print log if verbose */
		if (config->verbose)
		{
			/* print the log */
			lpp_log_print(&context);

			/* and release it */
			lpp_log_destroy(&context);
		}
	}
	else
	{
		/* set err */
		ret = 1;

		/* error */
		goto err_init_context;
	}

err_log_init:
	lpp_context_destroy(&context);
err_init_context:
	return ret;
}

/* entry */
int main(int argc, char *argv[])
{
	int ret;
	struct lpicp_config_t running_config;

	/* by default, return with error */
	ret = 1;

	/* set default configuration */
	lpicp_main_init_default_config(&running_config);

	/* try to parse the args into config */
	if (lpicp_main_parse_args_to_config(argc, argv, &running_config))
	{
		/* execute the configuration */
		ret = lpicp_main_execute_config(&running_config);	
	}

	/* done */
	return ret;
}

