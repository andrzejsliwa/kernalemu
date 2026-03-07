#ifndef CBMDOS_H_INCLUDED
#define CBMDOS_H_INCLUDED

// KERNAL Emulator
// Copyright (c) 2009-2021 Michael Steil
// All rights reserved. License: 2-clause BSD

// Set the local filesystem root for a given drive unit (8-15).
// Pass NULL or don't call to use the CWD for that unit.
void cbmdos_set_drive_root(uint8_t unit, const char *path);

void cbmdos_init(void);
int  cbmdos_open(uint8_t lfn, uint8_t unit, uint8_t sec, const char *filename);
void cbmdos_close(uint8_t lfn, uint8_t unit);
void cbmdos_chkin(uint8_t lfn, uint8_t unit);
void cbmdos_chkout(uint8_t lfn, uint8_t unit);
uint8_t cbmdos_basin(uint8_t unit, uint8_t *status);
int  cbmdos_bsout(uint8_t unit, uint8_t c);

#endif /* CBMDOS_H_INCLUDED */
