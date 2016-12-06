/*
 * Cypress Touchkey firmware list
 *
 * Copyright (C) 2011 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#if defined(CONFIG_MACH_JF_SGLTE)
#define BIN_FW_VERSION		0x08
#define BASE_FW_VERSION		0x01
#ifdef _CYPRESS_TKEY_FW_H
#include "sglte_tkey_fw.h"
#endif

#elif defined(CONFIG_MACH_JF_VZW) || defined(CONFIG_MACH_JF_SPR) || defined(CONFIG_MACH_JF_USC)
#define BIN_FW_VERSION		0x16
#define BASE_FW_VERSION		0x01
#ifdef _CYPRESS_TKEY_FW_H
#include "fusion3_cdma_tkey_fw.h"
#endif

#else
#define BIN_FW_VERSION		0x0E
#define BASE_FW_VERSION		0x01
#ifdef _CYPRESS_TKEY_FW_H
#include "fusion3_tkey_fw.h"
#endif
#endif
