// KERNAL Emulator
// Copyright (c) 2009-2021 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "glue.h"
#include "channelio.h"
#include "keyboard.h"


// SCNKEY - Scan keyboard
void
SCNKEY(void)
{
	// This should only be called by custom interrupt handlers.
	// do nothing
}

// STOP - Scan stop key
void
STOP(void)
{
	// TODO: We don't support the STOP key
	set_z(0);
}

// GETIN - Get a character
void
GETIN(void)
{
	BASIN();
}
