#ifndef FAKE6502_H_INCLUDED
#define FAKE6502_H_INCLUDED

#include <stdint.h>

extern void reset6502();
extern void step6502();
extern void exec6502(uint32_t tickcount);

#endif /* FAKE6502_H_INCLUDED */
