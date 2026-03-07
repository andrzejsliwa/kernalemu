// KERNAL Emulator
// Copyright (c) 2009-2021 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "glue.h"
#include "ieee488.h"


// The low-level IEEE-488/IEC calls are currently
// unsupported. Few applications need them, and it
// is not clear what mapping these to modern systems
// would mean in the first place.

void SECOND(void) { NYI(); }
void TKSA(void) { NYI(); }
void SETTMO(void) { NYI(); }
void ACPTR(void) { NYI(); }
void CIOUT(void) { NYI(); }
void UNTLK(void) { NYI(); }
void UNLSN(void) { NYI(); }
void LISTEN(void) { NYI(); }
void TALK(void) { NYI(); }
