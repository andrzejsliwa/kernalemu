#ifndef ERROR_H_INCLUDED
#define ERROR_H_INCLUDED

// KERNAL Emulator
// Copyright (c) 2009-2021 Michael Steil
// All rights reserved. License: 2-clause BSD

/* KERNAL error codes - returned in register A by OPEN, LOAD, SAVE etc.
   These are the standard Commodore KERNAL error values. */
typedef enum {
    KERN_ERR_NONE                  = 0,
    KERN_ERR_FILE_OPEN             = 2,
    KERN_ERR_FILE_NOT_OPEN         = 3,
    KERN_ERR_FILE_NOT_FOUND        = 4,
    KERN_ERR_DEVICE_NOT_PRESENT    = 5,
    KERN_ERR_NOT_INPUT_FILE        = 6,
    KERN_ERR_NOT_OUTPUT_FILE       = 7,
    KERN_ERR_MISSING_FILE_NAME     = 8,
    KERN_ERR_ILLEGAL_DEVICE_NUMBER = 9,
} kern_err_t;

/* KERNAL STATUS register bits - accumulated in $90, read via READST ($FFB7).
   Multiple bits may be set simultaneously. */
typedef enum {
    KERN_ST_TIME_OUT_READ = 0x02,  /* read timeout on serial bus      */
    KERN_ST_EOF           = 0x40,  /* end of file / end of tape block  */
} kern_status_t;

#endif /* ERROR_H_INCLUDED */
