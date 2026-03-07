#ifndef SCREEN_H_INCLUDED
#define SCREEN_H_INCLUDED

// KERNAL Emulator
// Copyright (c) 2009-2021 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdbool.h>

void screen_init(uint8_t columns, bool text_mode);
void CINT(void);
void SCREEN(void);
void PLOT(void);
void screen_bsout(void);

#endif /* SCREEN_H_INCLUDED */
