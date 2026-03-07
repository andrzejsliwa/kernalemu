// KERNAL Emulator
// Copyright (c) 2009-2021 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include "glue.h"
#include "error.h"
#include "cbmdos.h"
#include "screen.h"
#include "printer.h"
#include "channelio.h"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))


uint8_t kernal_msgflag;
uint8_t *STATUS_p;
uint16_t FNADR;
uint8_t FNLEN;
uint8_t LA, FA, SA;
uint8_t DFLTO = KERN_DEVICE_SCREEN;
uint8_t DFLTN = KERN_DEVICE_KEYBOARD;

#define STATUS (*STATUS_p)

/* 256 entries - one per possible LFN (SETLFS accepts uint8_t 0-255) */
#define X16  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
#define X256 X16,X16,X16,X16,X16,X16,X16,X16,X16,X16,X16,X16,X16,X16,X16,X16
uint8_t file_to_device[256] = { X256 };
#undef X16
#undef X256

void
channelio_init()
{
	STATUS_p = &RAM[0x90];
	STATUS = 0;
}

// SETMSG - Control KERNAL messages
void
SETMSG(void)
{
	// TODO: Act on kernal_msgflag
	kernal_msgflag = a;
	a = STATUS;
}

// READST - Read I/O status word
void
READST(void)
{
	a = STATUS;
}

// SETLFS - Set logical, first, and second addresses
void
SETLFS(void)
{
	LA = a;
	FA = x;
	SA = y;
}

// SETNAM - Set file name
void
SETNAM(void)
{
	FNADR = x | y << 8;
	FNLEN = a;
}

// OPEN - Open a logical file
void
OPEN(void)
{
	STATUS = 0;
	if (file_to_device[LA] != 0xFF) {
		set_c(1);
		a = KERN_ERR_FILE_OPEN;
		return;
	}

	char filename[256];
	uint8_t len = MIN(FNLEN, sizeof(filename) - 1);
	memcpy(filename, (char *)&RAM[FNADR], len);
	filename[len] = 0;

	switch (FA) {
		case KERN_DEVICE_SCREEN:
		case KERN_DEVICE_KEYBOARD:
			a = KERN_ERR_NONE;
			break;
		case KERN_DEVICE_CASSETTE:
		case KERN_DEVICE_RS232:
			a = KERN_ERR_DEVICE_NOT_PRESENT;
			break;
		default:
			if (is_printer_device(FA)) {
				printer_open(FA);
				a = KERN_ERR_NONE;
			} else if (is_drive_device(FA)) {
				a = cbmdos_open(LA, FA, SA, filename);
			}
			break;
	}

	if (a == KERN_ERR_NONE) {
		file_to_device[LA] = FA;
	}
	set_c(a != KERN_ERR_NONE);
}

// CLOSE - Close a specified logical file
void
CLOSE(void)
{
	uint8_t dev = file_to_device[a];
	if (dev == 0xFF) {
		set_c(1);
		a = KERN_ERR_FILE_NOT_OPEN;
		return;
	}

	switch (dev) {
		case KERN_DEVICE_KEYBOARD:
		case KERN_DEVICE_CASSETTE:
		case KERN_DEVICE_RS232:
		case KERN_DEVICE_SCREEN:
			set_c(0);
			return;
		default:
			if (is_printer_device(dev)) {
				printer_close(dev);
				a = KERN_ERR_NONE;
			} else if (is_drive_device(dev)) {
				cbmdos_close(a, dev);
				set_c(0);
			}
			break;
	}

	file_to_device[a] = 0xFF;
}

// CHKIN - Open channel for input
void
CHKIN(void)
{
	uint8_t dev = file_to_device[x];
	if (dev == 0xFF) {
		DFLTN = KERN_DEVICE_KEYBOARD;
		set_c(1);
		a = KERN_ERR_FILE_NOT_OPEN;
		return;
	}

	switch (dev) {
		case KERN_DEVICE_KEYBOARD:
		case KERN_DEVICE_CASSETTE:
		case KERN_DEVICE_RS232:
		case KERN_DEVICE_SCREEN:
		case KERN_DEVICE_PRINTERU4:
		case KERN_DEVICE_PRINTERU5:
		case KERN_DEVICE_PRINTERU6:
		case KERN_DEVICE_PRINTERU7:
			set_c(0);
			break;
		default:
			if (is_drive_device(dev)) {
				cbmdos_chkin(x, dev);
				set_c(0);
				break;
			}
			break;
	}
	DFLTN = dev;
}

// CHKOUT - Open channel for output
void
CHKOUT(void)
{
	uint8_t dev = file_to_device[x];
	if (dev == 0xFF) {
		DFLTO = KERN_DEVICE_SCREEN;
		set_c(1);
		a = KERN_ERR_FILE_NOT_OPEN;
		return;
	}

	switch (dev) {
		case KERN_DEVICE_KEYBOARD:
		case KERN_DEVICE_CASSETTE:
		case KERN_DEVICE_RS232:
		case KERN_DEVICE_SCREEN:
		case KERN_DEVICE_PRINTERU4:
		case KERN_DEVICE_PRINTERU5:
		case KERN_DEVICE_PRINTERU6:
		case KERN_DEVICE_PRINTERU7:
			set_c(0);
			break;
		default:
			if (is_drive_device(dev)) {
				cbmdos_chkout(x, dev);
				set_c(0);
				break;
			}
			break;
	}

	DFLTO = dev;
}

// CLRCHN - Close input and output channels
void
CLRCHN(void)
{
	DFLTO = KERN_DEVICE_SCREEN;
	DFLTN = KERN_DEVICE_KEYBOARD;
}

/* Optional inject buffer - filled by main before emulation starts */
static const char  *inject_buf   = NULL;
static int          inject_pos   = 0;
static uint8_t     *inject_patch = NULL;  /* RAM bytes to restore on first read */
static uint16_t     inject_patch_addr = 0;
static int          inject_patch_len  = 0;
static uint8_t     *inject_patch2 = NULL; /* optional second patch region */
static uint16_t     inject_patch2_addr = 0;
static int          inject_patch2_len  = 0;

void channelio_inject(const char *s) {
    inject_buf = s;
    inject_pos = 0;
}

/* Optionally restore RAM bytes on first BASIN call (before RUN is typed).
   Used to undo BASIC cold-start NEW which zeroes $0801/$0802. */
void channelio_inject_patch(uint16_t addr, uint8_t *bytes, int len) {
    inject_patch_addr = addr;
    inject_patch      = bytes;
    inject_patch_len  = len;
}

/* Second patch region - used to fix VARTAB ($002D/$002E) after cold start. */
void channelio_inject_patch2(uint16_t addr, uint8_t *bytes, int len) {
    inject_patch2_addr = addr;
    inject_patch2      = bytes;
    inject_patch2_len  = len;
}

// BASIN - Input character from channel
void
BASIN(void)
{
	switch (DFLTN) {
		case KERN_DEVICE_KEYBOARD:
			if (inject_buf && inject_buf[inject_pos]) {
				/* On first character, restore any RAM that cold-start wiped */
				if (inject_pos == 0) {
					if (inject_patch && inject_patch_len > 0) {
						memcpy(&RAM[inject_patch_addr], inject_patch, inject_patch_len);
						inject_patch = NULL;
					}
					if (inject_patch2 && inject_patch2_len > 0) {
						memcpy(&RAM[inject_patch2_addr], inject_patch2, inject_patch2_len);
						inject_patch2 = NULL;
					}
				}
				a = (uint8_t)inject_buf[inject_pos++];
				if (a == '\n') a = '\r';
			} else {
				a = getchar(); // stdin
				if (a == '\n') {
					a = '\r';
				}
			}
			break;
		case KERN_DEVICE_CASSETTE:
		case KERN_DEVICE_RS232:
		case KERN_DEVICE_SCREEN:
		case KERN_DEVICE_PRINTERU4:
		case KERN_DEVICE_PRINTERU5:
		case KERN_DEVICE_PRINTERU6:
		case KERN_DEVICE_PRINTERU7:
			set_c(1);
			return;
		default:
			if (is_drive_device(DFLTN)) {
				a = cbmdos_basin(DFLTN, &STATUS);
				set_c(0);
				break;
			}
			break;
	}
}


// BSOUT - Output character to channel
void
BSOUT(void)
{
	switch (DFLTO) {
		case KERN_DEVICE_KEYBOARD:
			set_c(1);
			return;
		case KERN_DEVICE_CASSETTE:
			set_c(1);
			return;
		case KERN_DEVICE_RS232:
			set_c(1);
			return;
		case KERN_DEVICE_SCREEN:
			screen_bsout();
			set_c(0);
			break;
		default:
			if (is_printer_device(DFLTO)) {
				printer_bsout(DFLTO);
				set_c(0);
			} else if (is_drive_device(DFLTO)) {
				STATUS = cbmdos_bsout(DFLTO, a);
				set_c(STATUS);
			}
			break;
	}
}

// LOAD - Load RAM from a device
void
LOAD(void)
{
	int error = KERN_ERR_NONE;

	uint16_t override_address = (x | (y << 8));

	SA = 0; // read
	OPEN();
	if (status & 1) {
		a = status;
		return;
	}

	x = LA;
	CHKIN();

	BASIN();
	if (STATUS & KERN_ST_EOF) {
		error = KERN_ERR_FILE_NOT_FOUND;
		goto end;
	}
	uint16_t address = a;
	BASIN();
	if (STATUS & KERN_ST_EOF) {
		error = KERN_ERR_FILE_NOT_FOUND;
		goto end;
	}
	address |= a << 8;
	if (!SA) {
		address = override_address;
	}

	do {
		BASIN();
		RAM[address++] = a;
	} while (!(STATUS & KERN_ST_EOF));

end:
	CLRCHN();
	a = LA;
	CLOSE();
	if (error != KERN_ERR_NONE) {
		set_c(1);
		a = error;
	} else {
		x = address & 0xff;
		y = address >> 8;
		set_c(0);
		STATUS = 0;
		a = 0;
	}
	//	for (int i = 0; i < 255; i++) {
	//	 if (!(i & 15)) {
	//	 }
	//	}
}

// SAVE - Save RAM to device
void
SAVE(void)
{
	uint16_t address = RAM[a] | RAM[a + 1] << 8;
	uint16_t end = x | y << 8;
	if (end < address) {
		set_c(1);
		a = KERN_ERR_NONE;
		return;
	}

	int error = KERN_ERR_NONE;

	SA = 1; // write
	OPEN();
	if (status & 1) {
		a = status;
		return;
	}

	x = LA;
	CHKOUT();

	a = address & 0xff;;
	BSOUT();
	a = address >> 8;
	BSOUT();

	while (address != end) {
		a = RAM[address++];
		BSOUT();
	};

	CLRCHN();
	a = LA;
	CLOSE();
	if (error != KERN_ERR_NONE) {
		set_c(1);
		a = error;
	} else {
		set_c(0);
		STATUS = 0;
		a = 0;
	}
}

// CLALL - Close all channels and files
void
CLALL(void)
{
	for (int lfn = 0; lfn < (int)(sizeof(file_to_device)/sizeof(file_to_device[0])); lfn++) {
		if (file_to_device[lfn] != 0xFF) {
			a = lfn;
			CLOSE();
		}
	}
}
