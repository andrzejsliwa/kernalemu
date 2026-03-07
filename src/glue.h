#ifndef GLUE_H_INCLUDED
#define GLUE_H_INCLUDED

// KERNAL Emulator
// Copyright (c) 2009-2021 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/* ---------------------------------------------------------------------------
 * CBM device numbers (used by channelio, cbmdos, printer)
 * --------------------------------------------------------------------------- */
#define KERN_DEVICE_KEYBOARD  0
#define KERN_DEVICE_CASSETTE  1
#define KERN_DEVICE_RS232     2
#define KERN_DEVICE_SCREEN    3
#define KERN_DEVICE_PRINTERU4 4
#define KERN_DEVICE_PRINTERU5 5
#define KERN_DEVICE_PRINTERU6 6
#define KERN_DEVICE_PRINTERU7 7
#define KERN_DEVICE_DRIVEU8   8
#define KERN_DEVICE_DRIVEU9   9
#define KERN_DEVICE_DRIVEU10  10
#define KERN_DEVICE_DRIVEU11  11
#define KERN_DEVICE_DRIVEU12  12
#define KERN_DEVICE_DRIVEU13  13
#define KERN_DEVICE_DRIVEU14  14
#define KERN_DEVICE_DRIVEU15  15

/* ---------------------------------------------------------------------------
 * NYI - Not Yet Implemented
 * Logs the unsupported KERNAL call and aborts. Used in place of stubs that
 * have not been implemented. The do-while(0) wrapper makes it safe after if.
 * --------------------------------------------------------------------------- */
#define NYI() \
    do { \
        fprintf(stderr, "Unsupported KERNAL call %s at PC=$%04X S=$%02X\n", \
                __func__, pc, sp); \
        exit(1); \
    } while (0)


extern uint8_t a, x, y, sp, status;
extern uint16_t pc;
extern uint8_t RAM[65536];

typedef enum {
	MACHINE_PET,
	MACHINE_PET4,
	MACHINE_VIC20,
	MACHINE_C64,
	MACHINE_TED,
	MACHINE_C128,
	MACHINE_C65,
} machine_t;

extern machine_t machine;

extern bool c64_has_external_rom;

/* Set/clear the carry flag in the 6502 status register. */
static inline void
set_c(int f)
{
	status = (uint8_t)((status & ~1) | (f ? 1 : 0));
}

/* Set/clear the zero flag in the 6502 status register. */
static inline void
set_z(int f)
{
	status = (uint8_t)((status & ~2) | (f ? 2 : 0));
}

/* Read a 16-bit little-endian value from the 6502 stack at byte offset i
   from the current stack pointer. Used by stack4() for call-chain inspection. */
#define STACK16(i) \
    ((uint16_t)(RAM[0x0100 + (i)] | (RAM[0x0100 + (i) + 1] << 8)))

/* Returns true if dev is a drive unit (devices 8-15). */
static inline bool is_drive_device(uint8_t dev) {
    return dev >= KERN_DEVICE_DRIVEU8 && dev <= KERN_DEVICE_DRIVEU15;
}

/* Returns true if dev is a printer unit (devices 4-7). */
static inline bool is_printer_device(uint8_t dev) {
    return dev >= KERN_DEVICE_PRINTERU4 && dev <= KERN_DEVICE_PRINTERU7;
}

/* Return true if the four most recent JSR callers match addresses a0-a3.
   Useful for dispatching private KERNAL calls that vary by call context. */
static inline int
stack4(uint16_t addr0, uint16_t addr1, uint16_t addr2, uint16_t addr3)
{
	if (STACK16(sp+1) + 1 != addr0) return 0;
	if (STACK16(sp+3) + 1 != addr1) return 0;
	if (STACK16(sp+5) + 1 != addr2) return 0;
	if (STACK16(sp+7) + 1 != addr3) return 0;
	return 1;
}

#endif /* GLUE_H_INCLUDED */
