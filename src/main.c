// KERNAL Emulator
// Copyright (c) 2009-2021 Michael Steil, James Abbatiello et al.
// All rights reserved. License: 2-clause BSD

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>  // getcwd, chdir
#include <windows.h> // GetLocalTime, SetLocalTime
#include <conio.h>   // _kbhit, _getch
#else
#include <sys/time.h>
#include <unistd.h>
#include <termios.h>
#endif
#include "fake6502.h"
#include "glue.h"
#include "dispatch.h"
#include "screen.h"
#include "cbmdos.h"
#include "channelio.h"

machine_t machine;
bool c64_has_external_rom;

#ifndef _WIN32
static struct termios orig_termios;
static bool raw_mode_active = false;

static void restore_terminal(void) {
	if (raw_mode_active) {
		tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
	}
}

static void enable_raw_input(void) {
	if (!isatty(STDIN_FILENO)) return;
	if (tcgetattr(STDIN_FILENO, &orig_termios) != 0) return;
	atexit(restore_terminal);
	struct termios raw = orig_termios;
	raw.c_lflag &= ~(ECHO | ICANON);
	raw.c_cc[VMIN]  = 1;
	raw.c_cc[VTIME] = 0;
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
	raw_mode_active = true;
}
#endif

static uint16_t
parse_num(const char *s)
{
	int base = 10;
	if (s[0] == '$') {
		s++;
		base = 16;
	} else if (s[0] == '0' && s[1] == 'x') {
		s += 2;
		base = 16;
	}
	return strtoul(s, NULL, base);
}

int
main(int argc, char **argv)
{
	if (argc <= 1) {
		printf("Usage: %s <filenames> [<arguments>]\n", argv[0]);
		exit(1);
	}

	bool has_start_address = false;
	uint16_t start_address;
	bool has_start_address_indirect = false;
	uint16_t start_address_indirect;
	bool has_machine = false;
	bool charset_text = false;
	uint8_t columns = 0;
	const char *autorun_cmd = NULL;
	uint16_t basic_load_addr = 0;
	uint16_t basic_prog_end  = 0;
	bool continue_mode       = false; /* -continue: log unknown KERNAL calls instead of exit */

	c64_has_external_rom = false;

	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			if (!strcmp(argv[i], "-start")) {
				if (i == argc - 1) {
					printf("%s: -start requires argument!\n", argv[0]);
					exit(1);
				}
				start_address = parse_num(argv[i + 1 ]);
				has_start_address = true;
			} else if (!strcmp(argv[i], "-startind")) {
				if (i == argc - 1) {
					printf("%s: -startind requires argument!\n", argv[0]);
					exit(1);
				}
				start_address_indirect = parse_num(argv[i + 1 ]);
				has_start_address_indirect = true;
			} else if (!strcmp(argv[i], "-machine")) {
				if (i == argc - 1) {
					printf("%s: -machine requires argument!\n", argv[0]);
					exit(1);
				}
				if (!strcmp(argv[i + 1], "pet")) {
					machine = MACHINE_PET;
				} else if (!strcmp(argv[i + 1], "pet4")) {
					machine = MACHINE_PET4;
				} else if (!strcmp(argv[i + 1], "vic20")) {
					machine = MACHINE_VIC20;
				} else if (!strcmp(argv[i + 1], "c64")) {
					machine = MACHINE_C64;
				} else if (!strcmp(argv[i + 1], "ted")) {
					machine = MACHINE_TED;
				} else if (!strcmp(argv[i + 1], "c128")) {
					machine = MACHINE_C128;
				} else if (!strcmp(argv[i + 1], "c65")) {
					machine = MACHINE_C65;
				} else {
					printf("%s: Valid values for \"-machine\" are pet, pet4, vic20, c64, ted, c128, c65!\n", argv[0]);
					exit(1);
				}
				has_machine = true;
			} else if (!strcmp(argv[i], "-text")) {
				charset_text = true;
			} else if (!strcmp(argv[i], "-graphics")) {
				charset_text = false;
			} else if (!strcmp(argv[i], "-columns")) {
				if (i == argc - 1) {
					printf("%s: -columns requires argument!\n", argv[0]);
					exit(1);
				}
				columns = parse_num(argv[i + 1 ]);
			} else if (!strncmp(argv[i], "-drive", 6)) {
				// -drive8 <path> through -drive15 <path>
				int unit = atoi(argv[i] + 6);
				if (unit >= 8 && unit <= 15) {
					if (i == argc - 1) {
						printf("%s: %s requires a path argument!\n", argv[0], argv[i]);
						exit(1);
					}
					cbmdos_set_drive_root((uint8_t)unit, argv[i + 1]);
				} else {
					printf("%s: valid drive units are 8-15 (e.g. -drive8 /path)\n", argv[0]);
					exit(1);
				}
			} else if (!strcmp(argv[i], "-autorun")) {
				/* Inject "RUN\r" into keyboard buffer so BASIC starts program automatically */
				autorun_cmd = "RUN\r";
			} else if (!strcmp(argv[i], "-basic")) {
				/* Convenience shortcut for C64 BASIC V2:
				   equivalent to -startind 0xa000 -autorun */
				start_address_indirect = 0xa000;
				has_start_address_indirect = true;
				autorun_cmd = "RUN\r";
			} else if (!strcmp(argv[i], "-continue")) {
				/* Tolerant mode: log unhandled KERNAL calls instead of aborting */
				continue_mode = true;
			}
			i++;
		} else {
			FILE *binary = fopen(argv[i], "rb");
			if (!binary) {
				printf("Error opening: %s\n", argv[i]);
				exit(1);
			}
			uint8_t lo = fgetc(binary);
			uint8_t hi = fgetc(binary);
			uint16_t load_address = lo | hi << 8;
			long file_start = ftell(binary);
			fread(&RAM[load_address], 65536 - load_address, 1, binary);
			long file_end = ftell(binary);
			fclose(binary);
			if (load_address == 0x8000) {
				c64_has_external_rom = true;
			}
			bool has_basic_start = false;
			switch (load_address) {
				case 0x401:  // PET
					if (!has_machine) {
						machine = MACHINE_PET4;
					}
					has_basic_start = true;
					break;
				case 0x801:  // C64
					if (!has_machine) {
						machine = MACHINE_C64;
					}
					has_basic_start = true;
					/* Record program extent for autorun restore */
					basic_load_addr = load_address;
					basic_prog_end  = (uint16_t)(load_address + (file_end - file_start));
					break;
				case 0x1001: // TED
					if (!has_machine) {
						machine = MACHINE_TED;
					}
					has_basic_start = true;
					break;
				case 0x1C01: // C128
					if (!has_machine) {
						machine = MACHINE_C128;
					}
					has_basic_start = true;
					break;
			}
			if (!has_start_address) {
				if (has_basic_start && RAM[load_address + 4] == 0x9e /* SYS */) {
					char *sys_arg = (char *)&RAM[load_address + 5];
					if (*sys_arg == '(') {
						sys_arg++;
					}
					start_address = parse_num(sys_arg);
					has_start_address = true;
				} else {
					start_address = load_address;
					has_start_address = true;
				}
			}
		}
	}

	if (!has_machine) {
		machine = MACHINE_C64;
	}

	reset6502();
	sp = 0xff;

	if (has_start_address_indirect) {
		pc = RAM[start_address_indirect] | (RAM[start_address_indirect + 1] << 8);
	} else if (has_start_address) {
		pc = start_address;
	} else {
		printf("%s: You need to specify at least one binary file!\n", argv[0]);
		exit(1);
	}

	kernal_init();
	screen_init(columns, charset_text);
#ifndef _WIN32
	enable_raw_input();
#endif

	if (autorun_cmd) {
		channelio_inject(autorun_cmd);
		/* Restore the 2 bytes BASIC cold-start NEW zeroes at the program start
		   address, and fix VARTAB to point past the program end.
		   Only applies to C64/C128: other machines (PET, TED) have different
		   BASIC start addresses and VARTAB zero-page locations. */
		if ((machine == MACHINE_C64 || machine == MACHINE_C128) &&
		     basic_load_addr == 0x0801 && basic_prog_end > basic_load_addr) {
			static uint8_t patch_prog[2];   /* restore $0801/$0802 */
			static uint8_t patch_vartab[2]; /* restore VARTAB $002D/$002E */
			patch_prog[0]    = RAM[basic_load_addr];
			patch_prog[1]    = RAM[basic_load_addr + 1];
			patch_vartab[0]  = (uint8_t)(basic_prog_end & 0xFF);
			patch_vartab[1]  = (uint8_t)(basic_prog_end >> 8);
			channelio_inject_patch(basic_load_addr, patch_prog, 2);
			channelio_inject_patch2(0x002D, patch_vartab, 2);
		}
	}

//	RAM[0xFFFC] = 0xD1;
//	RAM[0xFFFD] = 0xFC;
//	RAM[0xFFFE] = 0x1B;
//	RAM[0xFFFF] = 0xE6;

	for (;;) {
		while (!RAM[pc]) {
			bool success = kernal_dispatch(machine);
			if (!success) {
				uint16_t caller = (RAM[0x100 + sp + 1] | (RAM[0x100 + sp + 2] << 8)) + 1;
				fprintf(stderr, "\nunknown PC=$%04X S=$%02X (caller: $%04X)\n",
				        pc, sp, caller);
				if (!continue_mode) {
					exit(1);
				}
				/* Tolerant mode: simulate RTS to let execution continue */
				pc = (RAM[0x100 + sp + 1] | (RAM[0x100 + sp + 2] << 8)) + 1;
				sp += 2;
			}
		}
		step6502();
	}

	return 0;
}

