// KERNAL Emulator
// Copyright (c) 2009-2021 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "glue.h"
#include "vector.h"


// RESTOR - Restore default system and interrupt vectors
void
RESTOR(void)
{
	// TODO
}

// VECTOR - Read/set vectored I/O
void
VECTOR(void)
{
	// TODO: Generic support for user vectors would be
	// needed:
	// * CINV
	// * CBINV
	// * NMINV
	// * IOPEN
	// * ICLOSE
	// * ICHKIN
	// * ICKOUT
	// * ICLRCH
	// * IBASIN
	// * IBSOUT
	// * ISTOP
	// * IGETIN
	// * ICLALL
	// * USRCMD
	// * ILOAD
	// * ISAVE
	NYI();
}
