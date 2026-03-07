// KERNAL Emulator
// Copyright (c) 2009-2021 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "glue.h"
#include "channelio.h"
#include "c128.h"


// Most of the C128 KERNAL interface additions are currently
// unsupported.

// SPIN_SPOUT - Setup fast serial ports for I/O
void SPIN_SPOUT(void) { NYI(); }

// CLOSE_ALL - Close all files on a device
void CLOSE_ALL(void) { NYI(); }

// C64MODE - Reconfigure system as a C64
void C64MODE(void) { NYI(); }

// DMA_CALL - Send command to DMA device
void DMA_CALL(void) { NYI(); }

// BOOT_CALL - Boot load program from disk
void BOOT_CALL(void) { NYI(); }

// PHOENIX - Init function cartridges
void
PHOENIX(void)
{
	// do nothing
}

// LKUPLA - Search tables for given LA
void LKUPLA(void) { NYI(); }

// LKUPSA - Search tables for given SA
void LKUPSA(void) { NYI(); }

// SWAPPER - Switch between 40 and 80 columns
void SWAPPER(void) { NYI(); }

// DLCHR - Init 80-col character RAM
void DLCHR(void) { NYI(); }

// PFKEY - Program a function key
void PFKEY(void) { NYI(); }

// SETBNK - Set bank for I/O operations
void SETBNK(void) { NYI(); }

// GETCFG - Lookup MMU data for given bank
void GETCFG(void) { NYI(); }

// JSRFAR - GOSUB in another bank
void JSRFAR(void) { NYI(); }

// JMPFAR - GOTO another bank
void JMPFAR(void) { NYI(); }

// INDFET - LDA (fetvec),Y from any bank
void INDFET(void) { NYI(); }

// INDSTA - STA (stavec),Y to any bank
void INDSTA(void) { NYI(); }

// INDCMP - CMP (cmpvec),Y to any bank
void INDCMP(void) { NYI(); }

// PRIMM - Print immediate utility
void
PRIMM(void)
{
	uint16_t address = (RAM[0x100 + sp + 1] | (RAM[0x100 + sp + 2] << 8)) + 1;
	while ((a = RAM[address++])) {
		BSOUT();
	}
	address--;
	RAM[0x100 + sp + 1] = address & 0xff;
	RAM[0x100 + sp + 2] = address >> 8;
}
