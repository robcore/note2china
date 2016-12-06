/*
 * include/linux/sec_param.h
 *
 * Copyright (c) 2011 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

struct sec_param_data {
	unsigned int debuglevel;
	unsigned int uartsel;
	unsigned int rory_control;
	unsigned int movinand_checksum_done;
	unsigned int movinand_checksum_pass;
	unsigned int cp_debuglevel;
#if defined(CONFIG_MACH_APEXQ) || defined(CONFIG_MACH_AEGIS2)
	unsigned int slideCount;
#endif
#ifdef CONFIG_SGLTE_QSC_MODEM
	unsigned int update_cp_bin;
#endif
} ;

enum sec_param_index {
	param_index_debuglevel,
	param_index_uartsel,
	param_rory_control,
	param_index_movinand_checksum_done,
	param_index_movinand_checksum_pass,
#if defined(CONFIG_MACH_APEXQ) || defined(CONFIG_MACH_AEGIS2)
	param_slideCount,
#endif
#ifdef CONFIG_SGLTE_QSC_MODEM		//SPI_SETUP
	param_update_cp_bin,
#endif
} ;

extern bool sec_open_param(void);
extern bool sec_get_param(enum sec_param_index index, void *value);
extern bool sec_set_param(enum sec_param_index index, void *value);
