// KERNAL Emulator
// Copyright (c) 2009-2021 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "console.h"
#include "glue.h"


/* ---------------------------------------------------------------------------
 * PETSCII control codes referenced in screen_bsout().
 * Values are the standard Commodore screen editor control characters.
 * Source: Commodore 64 Programmer's Reference Guide, Appendix C.
 * --------------------------------------------------------------------------- */
#define PETSCII_WHITE          5    /* Set foreground color: white              */
#define PETSCII_LF            10    /* Line feed - ignored in terminal output   */
#define PETSCII_CR            13    /* Carriage return                          */
#define PETSCII_TEXT_MODE     14    /* Switch to text (lower/upper) charset     */
#define PETSCII_CSR_DOWN      17    /* Cursor down                              */
#define PETSCII_REVERSE_ON    18    /* Reverse video on (not renderable)        */
#define PETSCII_CSR_HOME      19    /* Cursor home                              */
#define PETSCII_DEL           20    /* Delete / backspace                       */
#define PETSCII_RED           28    /* Set foreground color: red                */
#define PETSCII_CSR_RIGHT     29    /* Cursor right                             */
#define PETSCII_GREEN         30    /* Set foreground color: green              */
#define PETSCII_BLUE          31    /* Set foreground color: blue               */
#define PETSCII_ORANGE       129    /* Set foreground color: orange             */
#define PETSCII_GRAPHICS     142    /* Switch to graphics (upper/gfx) charset   */
#define PETSCII_BLACK        144    /* Set foreground color: black              */
#define PETSCII_CSR_UP       145    /* Cursor up                                */
#define PETSCII_REVERSE_OFF  146    /* Reverse video off (not renderable)       */
#define PETSCII_CLR_SCREEN   147    /* Clear screen and home cursor             */
#define PETSCII_INSERT       148    /* Insert                                   */
#define PETSCII_BROWN        149    /* Set foreground color: brown              */
#define PETSCII_LT_RED       150    /* Set foreground color: light red          */
#define PETSCII_GREY1        151    /* Set foreground color: grey 1             */
#define PETSCII_GREY2        152    /* Set foreground color: grey 2             */
#define PETSCII_LT_GREEN     153    /* Set foreground color: light green        */
#define PETSCII_LT_BLUE      154    /* Set foreground color: light blue         */
#define PETSCII_GREY3        155    /* Set foreground color: grey 3             */
#define PETSCII_PURPLE       156    /* Set foreground color: purple             */
#define PETSCII_CSR_LEFT     157    /* Cursor left                              */
#define PETSCII_YELLOW       158    /* Set foreground color: yellow             */
#define PETSCII_CYAN         159    /* Set foreground color: cyan               */

static uint8_t columns;
static uint8_t column = 0;
static bool text_mode = false;
static int quote_mode = 0;

void
screen_init(uint8_t initial_columns, bool initial_text_mode)
{
	columns   = initial_columns;
	text_mode = initial_text_mode;
}

// CINT - Initialize screen editor and devices
void
CINT(void)
{
	// do nothing
}

// SCREEN - Return screen format
void
SCREEN(void)
{
	// TODO: return actual values
	x = 80;
	y = 25;
}

// PLOT
void
PLOT() // Read/set X,Y cursor position
{
	if (status & 1) {
		int CX, CY;
		get_cursor(&CX, &CY);
		y = CX;
		x = CY;
	} else {
		NYI();  /* PLOT set-cursor not yet implemented */
	}
}

void
screen_bsout()
{
	if (quote_mode) {
		if (a == '"') {
			quote_mode = 0;
		}
		putchar(a);
	} else {
		switch (a) {
			case PETSCII_WHITE:
				set_color(COLOR_WHITE);
				break;
			case PETSCII_LF:
				break;
			case PETSCII_CR:
				putchar(13);
				putchar(10);
				column = 0;
				break;
			case PETSCII_TEXT_MODE: // lower/upper charset
				text_mode = true;
				break;
			case PETSCII_CSR_DOWN:
				down_cursor();
				break;
			case PETSCII_REVERSE_ON: // not renderable in terminal; silently consume
				break;
			case PETSCII_CSR_HOME:
				move_cursor(0, 0);
				break;
			case PETSCII_DEL: // backspace
				putchar('\b');
				putchar(' ');
				putchar('\b');
				if (column) column--;
				break;
			case PETSCII_RED:
				set_color(COLOR_RED);
				break;
			case PETSCII_CSR_RIGHT:
				right_cursor();
				break;
			case PETSCII_GREEN:
				set_color(COLOR_GREEN);
				break;
			case PETSCII_BLUE:
				set_color(COLOR_BLUE);
				break;
			case PETSCII_ORANGE:
				set_color(COLOR_ORANGE);
				break;
			case PETSCII_GRAPHICS: // upper/graphics charset
				text_mode = false;
				break;
			case PETSCII_BLACK:
				set_color(COLOR_BLACK);
				break;
			case PETSCII_CSR_UP:
				up_cursor();
				break;
			case PETSCII_REVERSE_OFF: // silently consume
				break;
			case PETSCII_CLR_SCREEN:
				clear_screen();
				break;
			case PETSCII_INSERT: // silently consume in terminal context
				break;
			case PETSCII_BROWN:
				set_color(COLOR_BROWN);
				break;
			case PETSCII_LT_RED:
				set_color(COLOR_LTRED);
				break;
			case PETSCII_GREY1:
				set_color(COLOR_GREY1);
				break;
			case PETSCII_GREY2:
				set_color(COLOR_GREY2);
				break;
			case PETSCII_LT_GREEN:
				set_color(COLOR_LTGREEN);
				break;
			case PETSCII_LT_BLUE:
				set_color(COLOR_LTBLUE);
				break;
			case PETSCII_GREY3:
				set_color(COLOR_GREY3);
				break;
			case PETSCII_PURPLE:
				set_color(COLOR_PURPLE);
				break;
			case PETSCII_YELLOW:
				set_color(COLOR_YELLOW);
				break;
			case PETSCII_CYAN:
				set_color(COLOR_CYAN);
				break;
			case PETSCII_CSR_LEFT:
				left_cursor();
				break;
			case '"':
				quote_mode = 1;
				// fallthrough
			default: {
				unsigned char c = a;
				if (text_mode) {
					if (c >= 0x41 && c <= 0x5a) {
						c += 0x20;
					} else if (c >= 0x61 && c <= 0x7a) {
						c -= 0x20;
					} else if (c >= 0xc1 && c <= 0xda) {
						c -= 0x80;
					}
				}
				putchar(c);
				if (c == '\b') {
					if (column) {
						column--;
					}
				} else {
					column++;
					if (column == columns) {
						putchar('\n');
						column = 0;
					}
				}
			}
		}
	}
	fflush(stdout);
}
