#ifndef C128_H_INCLUDED
#define C128_H_INCLUDED

// KERNAL Emulator
// Copyright (c) 2009-2021 Michael Steil
// All rights reserved. License: 2-clause BSD

void SPIN_SPOUT(void);
void CLOSE_ALL(void);
void C64MODE(void);
void DMA_CALL(void);
void BOOT_CALL(void);
void PHOENIX(void);
void LKUPLA(void);
void LKUPSA(void);
void SWAPPER(void);
void DLCHR(void);
void PFKEY(void);
void SETBNK(void);
void GETCFG(void);
void JSRFAR(void);
void JMPFAR(void);
void INDFET(void);
void INDSTA(void);
void INDCMP(void);
void PRIMM(void);

#endif /* C128_H_INCLUDED */
