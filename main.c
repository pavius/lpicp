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
#include "lpicp_image.h"

/* current version */
const char *version_string = "0.0.1";

/* whether to read or write */
enum lpicp_opmode_t
{ 
	LPICP_OPMODE_UNDEFINED,
	LPICP_OPMODE_READ,
	LPICP_OPMODE_WRITE,
	LPICP_OPMODE_GET_DEVID,
	LPICP_OPMODE_ERASE_DEVICE
};

/* running configuration */
struct lpp_config_t
{
	int verbose;
	char *dev_name;
	char *file_name;
	enum lpicp_opmode_t opmode;
	unsigned int offset;
};

/* initialize default configuration */
void lpicp_main_init_default_config(struct lpp_config_t *config)
{
	config->verbose = 0;
	config->dev_name = NULL;
	config->file_name = NULL;
	config->opmode = LPICP_OPMODE_UNDEFINED;
	config->offset = 0;
}

/* print usage */
void lpicp_main_usage(void)
{
	printf("Linux PIC Programmer v.%s\n", version_string);
	printf("Usage: lpicp [options]\n");
	printf("  -x, --exec            r, read | w, write | e, erase | devid\n");
	printf("  -d, --dev             ICSP device name (e.g. /dev/icsp0)\n");
	printf("  -f, --file            Path to Intel HEX file\n");
	printf("  -o, --offset          Read from offset, Write to offset\n");
	printf("  -v, --verbose         Verbose operation\n");
	printf("  -h, --help            Prints this usage\n");
	printf("\n");
}

/* parse arguments to configuration */
int lpicp_main_parse_args_to_config(int argc, char *argv[], struct lpp_config_t *config)
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
			{"exec",     	0,  		0, 				'x'},
			{"help",  		0,  		0, 				'h'},
			{"dev",  		1, 			0, 				'd'},
			{"file",  		1, 			0, 				'f'},
			{"offset",  	1, 			0, 				'o'},
			{0, 0, 0, 0}
		};

		/* getopt_long stores the option index here. */
		int option_index = 0;

		/* get the options */
		current_option = getopt_long (argc, argv, "hvx:d:f:o:", long_options, &option_index);

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

			/* execute */
			case 'x':
			{
				/* read? */
				if (strcmp(optarg, "read") == 0 || strcmp(optarg, "r") == 0)
				{
					/* set mode */
					config->opmode = LPICP_OPMODE_READ;
				}
				/* write? */
				else if (strcmp(optarg, "write") == 0 || strcmp(optarg, "w") == 0)
				{
					/* set mode */
					config->opmode = LPICP_OPMODE_WRITE;
				}
				/* erase? */
				else if (strcmp(optarg, "erase") == 0 || strcmp(optarg, "e") == 0)
				{
					/* set mode */
					config->opmode = LPICP_OPMODE_ERASE_DEVICE;
				}
				/* get device? */
				else if (strcmp(optarg, "devid") == 0)
				{
					/* set mode */
					config->opmode = LPICP_OPMODE_GET_DEVID;
				}
			}
			break;

			/* device */
			case 'd':
			{
				/* save file name */
				config->dev_name = optarg;
			}
			break;

			/* file */
			case 'f':
			{
				/* save file name */
				config->file_name = optarg;
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

	/* check that all args have been passed */
	if (config->opmode == LPICP_OPMODE_UNDEFINED)
	{
		printf("Execution mode (-x,--exec) not passed\n");
		return 0;
	}

	/* check that all args have been passed */
	if (config->dev_name == NULL)
	{
		printf("Device name (-d,--dev) not passed\n");
		return 0;
	}

	/* go ahead with the configuration */
	return 1;
}

/* do read device id */
int lpicp_main_execute_read_devid(struct lpp_context_t *context, 
								  struct lpp_config_t *config)
{
	unsigned short device_id = 0;

	/* try to get device id */
	if (lpp_device_id_read(context, &device_id))
	{
		/* print device-id */
		printf("Device ID: %02X.%02X\n", ((device_id >> 8) & 0xFF), (device_id & 0xFF));

		/* success */
		return 1;
	}
	else
	{
		/* error reading dev id */
		printf("Error reading device ID\n");

		/* error */
		return 0;
	}
}

/* do erase device */
int lpicp_main_execute_erase_device(struct lpp_context_t *context, 
									struct lpp_config_t *config)
{
	/* read the file */
	if (lpp_bulk_erase(context))
	{
		/* success */
		return 1;
	}
	else
	{
		/* error writing file */
		printf("Error erasing device\n");

		/* error */
		return 0;
	}
}

/* do write */
int lpicp_main_execute_image_write(struct lpp_context_t *context, 
								   struct lpp_config_t *config)
{
	struct lpp_image_t image;

	/* read the file and write to device */
	if (lpp_image_read_from_file(context, &image, config->file_name, 64 * 1024) && 
		lpp_program_image_to_device(context, &image))
	{
		/* success */
		return 1;
	}
	else
	{
		/* error writing file */
		printf("Error writing file\n");

		/* error */
		return 0;
	}
}

/* do read */
int lpicp_main_execute_image_read(struct lpp_context_t *context, 
								  struct lpp_config_t *config)
{
	struct lpp_image_t image;

	/* try to read the image */
	if (lpp_read_device_to_image(context, 0, 64 * 1024, &image) &&
		lpp_image_write_to_file(context, &image, config->file_name))
	{
		/* success */
		return 1;
	}
	else
	{
		/* error writing file */
		printf("Error reading file from device\n");

		/* error */
		return 0;
	}
}
/* parse arguments to configuration */
int lpicp_main_execute_config(struct lpp_config_t *config)
{
	struct lpp_context_t context;
	int ret;

	/* assume no error */
	ret = 1;

	/* try to init context */
	if (lpp_context_init(&context, LPP_DEVICE_FAMILY_18F, config->dev_name))
	{
		/* init log if verbose */
		if (config->verbose)
		{
			/* try to init log */
			if (!lpp_log_init(&context, 512))
			{
				/* set err */
				ret = 0;

				/* error */
				goto err_log_init;
			}
		}

		/* handle opmode */
		switch (config->opmode)
		{
			/* read image from device and write to file */
			case LPICP_OPMODE_READ:  
				ret = lpicp_main_execute_image_read(&context, config); 
				break;

			/* write image from file to the device */
			case LPICP_OPMODE_WRITE: 
				ret = lpicp_main_execute_image_write(&context, config); 
				break;

			/* erase the device */
			case LPICP_OPMODE_ERASE_DEVICE: 
				ret = lpicp_main_execute_erase_device(&context, config); 
				break;

			/* read device-id */
			case LPICP_OPMODE_GET_DEVID: 
				ret = lpicp_main_execute_read_devid(&context, config); 
				break;
		}

		/* print log if verbose */
		if (config->verbose)
		{
			/* print the log on success */
			if (ret) lpp_log_print(&context);

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
	struct lpp_config_t running_config;

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

