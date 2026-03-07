#ifndef CHANNELIO_H_INCLUDED
#define CHANNELIO_H_INCLUDED

// KERNAL Emulator
// Copyright (c) 2009-2021 Michael Steil
// All rights reserved. License: 2-clause BSD

void channelio_init(void);
void channelio_inject(const char *s);
void channelio_inject_patch(uint16_t addr, uint8_t *bytes, int len);
void channelio_inject_patch2(uint16_t addr, uint8_t *bytes, int len);
void SETMSG(void);
void READST(void);
void SETLFS(void);
void SETNAM(void);
void OPEN(void);
void CLOSE(void);
void CHKIN(void);
void CHKOUT(void);
void CLRCHN(void);
void BASIN(void);
void BSOUT(void);
void LOAD(void);
void SAVE(void);
void CLALL(void);

#endif /* CHANNELIO_H_INCLUDED */
