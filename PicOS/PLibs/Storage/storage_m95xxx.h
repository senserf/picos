/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_storage_m95xxx_h
#define	__pg_mstorage_95xxx_h	1

#include "kernel.h"
#include "storage.h"
#include "pins.h"

#define	EE_WREN		0x06		// Write enable
#define	EE_WRDI		0x04		// Write disable
#define	EE_RDSR		0x05		// Read status register
#define	EE_WRSR		0x01		// Write status register
#define	EE_READ		0x03
#define	EE_WRITE	0x02

#define	STAT_WIP	0x01		// Write in progress
#define	STAT_INI	0x00

#define	EE_PAGE_SIZE	32	// bytes
#define	EE_PAGE_SIZE_T	32
#define	EE_BLOCK_SIZE	EE_PAGE_SIZE
#define	EE_NPAGES	EE_NBLOCKS
#define	EE_SIZE		(EE_NPAGES * EE_PAGE_SIZE)

#define	EE_ERASE_UNIT		1
#define	EE_ERASE_BEFORE_WRITE	0
#define	EE_RANDOM_WRITE		1

#endif
