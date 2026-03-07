// KERNAL Emulator - 1541 DOS Emulation Layer
// Copyright (c) 2009-2021 Michael Steil
// Additional 1541 emulation by kernalemu contributors
// All rights reserved. License: 2-clause BSD
//
// Emulates a Commodore 1541 floppy drive against the local filesystem.
// Each unit (8-15) may be mapped to a separate directory via
// cbmdos_set_drive_root(). Without that, all units use the CWD.
//
// Supported command channel (SA=15) commands:
//   I             - Initialize (returns OK)
//   UI            - Soft reset (returns 1541 ident)
//   UJ            - Hard reset
//   S:name[,name] - Scratch (delete) files, wildcards (* and ?) supported
//   R:newname=old - Rename
//   C:dest=src    - Copy (single file)
//   V             - Validate (no-op, returns OK)
//   CD:path       - Change directory (extension beyond real 1541)
//
// File open secondary addresses:
//   0        - PRG load (read)
//   1        - PRG save (write)
//   2-14     - Data channel (mode from filename suffix ,X,R / ,X,W)
//   15       - Command channel

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fnmatch.h>
#include <errno.h>
#include <ctype.h>
#include "readdir.h"
#include "error.h"
#include "cbmdos.h"
#include <limits.h>

/* PATH_MAX is defined by POSIX but not guaranteed. Provide a fallback. */
#ifndef PATH_MAX
#  define PATH_MAX 4096
#endif

// ---------------------------------------------------------------------------
// Drive roots - up to 8 units (device 8..15)
// ---------------------------------------------------------------------------
#define NUM_UNITS 8
static char drive_root[NUM_UNITS][PATH_MAX];

void cbmdos_set_drive_root(uint8_t unit, const char *path) {
    if (unit >= 8 && unit < 8 + NUM_UNITS) {
        strncpy(drive_root[unit - 8], path, sizeof(drive_root[0]) - 1);
        drive_root[unit - 8][sizeof(drive_root[0]) - 1] = '\0';
    }
}

static const char *get_drive_root(uint8_t unit) {
    if (unit >= 8 && unit < 8 + NUM_UNITS) {
        if (drive_root[unit - 8][0]) {
            return drive_root[unit - 8];
        }
    }
    return NULL;
}

// ---------------------------------------------------------------------------
// 1541 DOS status strings
// ---------------------------------------------------------------------------
#define STATUS_BUF_SIZE 64
static char cur_drive_status[STATUS_BUF_SIZE];
static const char *drive_status_p;

static void set_drive_status(const char *s) {
    strncpy(cur_drive_status, s, STATUS_BUF_SIZE - 1);
    cur_drive_status[STATUS_BUF_SIZE - 1] = '\0';
    drive_status_p = cur_drive_status;
}

static void set_drive_status_scratched(int n) {
    snprintf(cur_drive_status, STATUS_BUF_SIZE, "01,FILES SCRATCHED,%02d,00\r", n);
    drive_status_p = cur_drive_status;
}

// ---------------------------------------------------------------------------
// File table
// ---------------------------------------------------------------------------
#define FILE_COMMAND_CHAN ((FILE *)1)
#define FILE_DIRECTORY   ((FILE *)2)
#define MAX_LFN 256

static FILE    *files[MAX_LFN];
static uint8_t  file_sa[MAX_LFN];
static uint8_t  file_unit[MAX_LFN];  /* drive unit (8-15) for each open LFN */

uint8_t in_lfn  = 0;
uint8_t out_lfn = 0;

// ---------------------------------------------------------------------------
// Directory listing buffer
// ---------------------------------------------------------------------------
#define DIR_BUF_SIZE 65536
static uint8_t directory_data[DIR_BUF_SIZE];
static uint8_t *directory_data_p;
static uint8_t *directory_data_end;

// ---------------------------------------------------------------------------
// Command channel buffer
// ---------------------------------------------------------------------------
#define CMD_BUF_SIZE 256
static char command[CMD_BUF_SIZE];
static char *command_p;
static uint8_t command_unit_for_interpret = 8; /* resolved at interpret time */

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static bool wildcard_match(const char *pattern, const char *name) {
    if (strcmp(pattern, name) == 0) return true;
    return fnmatch(pattern, name, FNM_NOESCAPE) == 0;
}

static char *resolve_path(uint8_t unit, const char *filename) {
    const char *root = get_drive_root(unit);
    if (!root) {
        return strdup(filename);
    }
    size_t len = strlen(root) + 1 + strlen(filename) + 1;
    char *buf = malloc(len);
    if (!buf) return NULL;
    snprintf(buf, len, "%s/%s", root, filename);
    return buf;
}

static const char *strip_drive_prefix(const char *filename) {
    if (filename[0] >= '0' && filename[0] <= '9' && filename[1] == ':') {
        return filename + 2;
    }
    return filename;
}

// ---------------------------------------------------------------------------
// Command implementations
// ---------------------------------------------------------------------------

static int do_scratch(uint8_t unit, const char *arg) {
    char patterns[CMD_BUF_SIZE];
    strncpy(patterns, arg, CMD_BUF_SIZE - 1);
    patterns[CMD_BUF_SIZE - 1] = '\0';

    int scratched = 0;
    char *pat = strtok(patterns, ",");
    while (pat) {
        while (*pat == ' ') pat++;
        pat = (char *)strip_drive_prefix(pat);

        const char *root = get_drive_root(unit);
        const char *scan = root ? root : ".";
        DIR *dirp = opendir(scan);
        if (dirp) {
            struct dirent *dp;
            while ((dp = readdir(dirp))) {
                if (dp->d_name[0] == '.') continue;
                if (wildcard_match(pat, dp->d_name)) {
                    char *full = resolve_path(unit, dp->d_name);
                    if (full) {
                        if (remove(full) == 0) scratched++;
                        free(full);
                    }
                }
            }
            closedir(dirp);
        }
        pat = strtok(NULL, ",");
    }
    return scratched;
}

static void do_rename(uint8_t unit, const char *arg) {
    char buf[CMD_BUF_SIZE];
    strncpy(buf, arg, CMD_BUF_SIZE - 1);
    buf[CMD_BUF_SIZE - 1] = '\0';

    char *eq = strchr(buf, '=');
    if (!eq) { set_drive_status("31,SYNTAX ERROR,00,00\r"); return; }
    *eq = '\0';
    const char *newname = strip_drive_prefix(buf);
    const char *oldname = strip_drive_prefix(eq + 1);

    char *oldpath = resolve_path(unit, oldname);
    char *newpath = resolve_path(unit, newname);
    if (!oldpath || !newpath) {
        free(oldpath); free(newpath);
        set_drive_status("74,DRIVE NOT READY,00,00\r");
        return;
    }

    struct stat st;
    if (stat(newpath, &st) == 0) {
        set_drive_status("63,FILE EXISTS,00,00\r");
    } else if (rename(oldpath, newpath) == 0) {
        set_drive_status("00, OK,00,00\r");
    } else {
        set_drive_status(errno == ENOENT ? "62,FILE NOT FOUND,00,00\r" : "31,SYNTAX ERROR,00,00\r");
    }
    free(oldpath); free(newpath);
}

static void do_copy(uint8_t unit, const char *arg) {
    char buf[CMD_BUF_SIZE];
    strncpy(buf, arg, CMD_BUF_SIZE - 1);
    buf[CMD_BUF_SIZE - 1] = '\0';

    char *eq = strchr(buf, '=');
    if (!eq) { set_drive_status("31,SYNTAX ERROR,00,00\r"); return; }
    *eq = '\0';
    const char *destname = strip_drive_prefix(buf);
    const char *srcname  = strip_drive_prefix(eq + 1);

    char *srcpath  = resolve_path(unit, srcname);
    char *destpath = resolve_path(unit, destname);
    if (!srcpath || !destpath) {
        free(srcpath); free(destpath);
        set_drive_status("74,DRIVE NOT READY,00,00\r");
        return;
    }

    FILE *src  = fopen(srcpath, "rb");
    FILE *dest = src ? fopen(destpath, "wb") : NULL;
    if (!src) {
        set_drive_status("62,FILE NOT FOUND,00,00\r");
    } else if (!dest) {
        set_drive_status("72,DISK FULL,00,00\r");
    } else {
        uint8_t tmp[4096];
        size_t n;
        bool ok = true;
        while ((n = fread(tmp, 1, sizeof(tmp), src)) > 0) {
            if (fwrite(tmp, 1, n, dest) != n) { ok = false; break; }
        }
        set_drive_status(ok ? "00, OK,00,00\r" : "25,WRITE ERROR,00,00\r");
    }
    if (src)  fclose(src);
    if (dest) fclose(dest);
    free(srcpath); free(destpath);
}

static void do_cd(uint8_t unit, const char *arg) {
    const char *path = strip_drive_prefix(arg);
    const char *root = get_drive_root(unit);
    if (root) {
        char newpath[PATH_MAX];
        if (path[0] == '/') {
            snprintf(newpath, PATH_MAX, "%s", path);
        } else {
            snprintf(newpath, PATH_MAX, "%s/%s", root, path);
        }
        cbmdos_set_drive_root(unit, newpath);
        set_drive_status("00, OK,00,00\r");
    } else {
        set_drive_status(chdir(path) == 0 ? "00, OK,00,00\r" : "62,FILE NOT FOUND,00,00\r");
    }
}

// ---------------------------------------------------------------------------
// Command channel interpreter
// ---------------------------------------------------------------------------

static void interpret_command(void) {
    *command_p = '\0';
    char *cr = strchr(command, '\r');
    if (!cr) return;
    *cr = '\0';

    /* Uppercase only the command letter(s), not filenames */
    command[0] = (char)toupper((unsigned char)command[0]);
    if (command[1] && command[1] != ':') {
        command[1] = (char)toupper((unsigned char)command[1]);
    }

    if (command[0] == '\0') {
        drive_status_p = cur_drive_status;
        command_p = command;
        return;
    }

    char cmd  = command[0];
    char cmd2 = command[1];
    const char *raw_arg = command + 1;
    if (*raw_arg == ':') raw_arg++;

    switch (cmd) {
        case 'I':
            set_drive_status("00, OK,00,00\r");
            break;
        case 'V':
            set_drive_status("00, OK,00,00\r");
            break;
        case 'S':
            if (!*raw_arg) {
                set_drive_status("33,SYNTAX ERROR,00,00\r");
            } else {
                int n = do_scratch(command_unit_for_interpret, raw_arg);
                set_drive_status_scratched(n);
            }
            break;
        case 'R':
            if (!*raw_arg) set_drive_status("33,SYNTAX ERROR,00,00\r");
            else do_rename(command_unit_for_interpret, raw_arg);
            break;
        case 'C':
            if (cmd2 == 'D') {
                const char *cd_arg = command + 2;
                if (*cd_arg == ':') cd_arg++;
                do_cd(command_unit_for_interpret, cd_arg);
            } else {
                if (!*raw_arg) set_drive_status("33,SYNTAX ERROR,00,00\r");
                else do_copy(command_unit_for_interpret, raw_arg);
            }
            break;
        case 'U':
            if (cmd2 == 'I') {
                set_drive_status("73,CBM DOS V2.6 1541,00,00\r");
            } else if (cmd2 == 'J') {
                cbmdos_init();
            } else {
                set_drive_status("31,SYNTAX ERROR,00,00\r");
            }
            break;
        default:
            set_drive_status("31,SYNTAX ERROR,00,00\r");
            break;
    }
    command_p = command;
}

// ---------------------------------------------------------------------------
// Directory listing builder
// ---------------------------------------------------------------------------

static bool create_directory_listing(uint8_t unit) {
    directory_data_p = directory_data;

    const char *root = get_drive_root(unit);
    const char *scan_dir = root ? root : ".";

    // PRG load address $0801
    *directory_data_p++ = 0x01;
    *directory_data_p++ = 0x08;

    // Header line (line number 0)
    *directory_data_p++ = 0x01; *directory_data_p++ = 0x01; // fake link
    *directory_data_p++ = 0x00; *directory_data_p++ = 0x00; // line 0
    *directory_data_p++ = 0x12; // REVERSE ON
    *directory_data_p++ = '"';

    char disk_name[17];
    memset(disk_name, ' ', 16);
    disk_name[16] = '\0';
    {
        const char *base = root ? root : ".";
        char cwd[256];
        if (!root && getcwd(cwd, sizeof(cwd))) base = cwd;
        const char *last = strrchr(base, '/');
        last = last ? last + 1 : base;
        size_t len = strlen(last);
        if (len > 16) len = 16;
        memcpy(disk_name, last, len);
    }
    memcpy(directory_data_p, disk_name, 16);
    directory_data_p += 16;
    *directory_data_p++ = '"';
    *directory_data_p++ = ' ';
    *directory_data_p++ = '0'; *directory_data_p++ = '0';
    *directory_data_p++ = ' ';
    *directory_data_p++ = '2'; *directory_data_p++ = 'A';
    *directory_data_p++ = 0x00;

    DIR *dirp = opendir(scan_dir);
    if (!dirp) return false;

    struct dirent *dp;
    while ((dp = readdir(dirp))) {
        if (dp->d_name[0] == '.') continue;
        if (directory_data_p - directory_data > DIR_BUF_SIZE - 64) break;

        char full_path[PATH_MAX];
        if (root)
            snprintf(full_path, PATH_MAX, "%s/%s", root, dp->d_name);
        else
            snprintf(full_path, PATH_MAX, "%s", dp->d_name);

        struct stat st;
        stat(full_path, &st);
        bool is_dir = S_ISDIR(st.st_mode);
        uint16_t blocks = is_dir ? 0 : (uint16_t)((st.st_size + 253) / 254);
        if (blocks > 0xFFFF) blocks = 0xFFFF;

        *directory_data_p++ = 0x01; *directory_data_p++ = 0x01; // fake link
        *directory_data_p++ = blocks & 0xFF;
        *directory_data_p++ = blocks >> 8;
        // Column alignment padding
        if (blocks <    10) *directory_data_p++ = ' ';
        if (blocks <   100) *directory_data_p++ = ' ';
        if (blocks <  1000) *directory_data_p++ = ' ';

        *directory_data_p++ = '"';
        size_t namlen = strlen(dp->d_name);
        if (namlen > 16) namlen = 16;
        memcpy(directory_data_p, dp->d_name, namlen);
        directory_data_p += namlen;
        *directory_data_p++ = '"';
        for (size_t i = namlen; i < 16; i++) *directory_data_p++ = ' ';
        *directory_data_p++ = ' ';

        if (is_dir) {
            *directory_data_p++ = 'D'; *directory_data_p++ = 'I'; *directory_data_p++ = 'R';
        } else {
            const char *dot = strrchr(dp->d_name, '.');
            if (dot && strcasecmp(dot, ".seq") == 0) {
                *directory_data_p++ = 'S'; *directory_data_p++ = 'E'; *directory_data_p++ = 'Q';
            } else if (dot && strcasecmp(dot, ".usr") == 0) {
                *directory_data_p++ = 'U'; *directory_data_p++ = 'S'; *directory_data_p++ = 'R';
            } else {
                *directory_data_p++ = 'P'; *directory_data_p++ = 'R'; *directory_data_p++ = 'G';
            }
        }
        *directory_data_p++ = ' '; *directory_data_p++ = ' ';
        *directory_data_p++ = 0x00;
    }
    closedir(dirp);

    // "BLOCKS FREE." footer line - fake 664 blocks free
    *directory_data_p++ = 0x01; *directory_data_p++ = 0x01;
    *directory_data_p++ = 0x98; *directory_data_p++ = 0x02; // 664
    const char *bf = "BLOCKS FREE.             ";
    memcpy(directory_data_p, bf, strlen(bf));
    directory_data_p += strlen(bf);
    *directory_data_p++ = 0x00;

    // Link terminator
    *directory_data_p++ = 0x00; *directory_data_p++ = 0x00;

    directory_data_end = directory_data_p;
    directory_data_p   = directory_data;

    return true;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void cbmdos_init(void) {
    for (int i = 0; i < MAX_LFN; i++) {
        if (files[i] && files[i] != FILE_COMMAND_CHAN && files[i] != FILE_DIRECTORY) {
            fclose(files[i]);
        }
        files[i]     = NULL;
        file_sa[i]   = 0xFF;
        file_unit[i] = 8;  /* default unit */
    }
    set_drive_status("73,CBM DOS V2.6 1541,00,00\r");
    command_p = command;
    directory_data_p   = directory_data;
    directory_data_end = directory_data;
}

int cbmdos_open(uint8_t lfn, uint8_t unit, uint8_t sec, const char *filename) {
    file_sa[lfn]   = sec;
    file_unit[lfn] = unit;

    if (sec == 15) {
        files[lfn] = FILE_COMMAND_CHAN;
        if (strlen(filename) >= CMD_BUF_SIZE - 2) {
            set_drive_status("32,SYNTAX ERROR,00,00\r");
            return KERN_ERR_NONE;
        }
        if (strlen(filename) > 0) {
            strcpy(command, filename);
            command_p = command + strlen(command);
            *command_p++ = '\r';
            command_unit_for_interpret = file_unit[lfn];
            interpret_command();
        }
        return KERN_ERR_NONE;
    }

    if (filename[0] == '$') {
        files[lfn] = FILE_DIRECTORY;
        return create_directory_listing(unit) ? KERN_ERR_NONE : KERN_ERR_FILE_NOT_FOUND;
    }

    filename = strip_drive_prefix(filename);

    // Parse ,type,mode suffix
    char namebuf[256];
    strncpy(namebuf, filename, sizeof(namebuf) - 1);
    namebuf[sizeof(namebuf) - 1] = '\0';

    char mode_char = 0;
    char *comma1 = strchr(namebuf, ',');
    if (comma1) {
        *comma1 = '\0';
        char *comma2 = strchr(comma1 + 1, ',');
        if (comma2) mode_char = comma2[1];
    }

    if (namebuf[0] == '\0') return KERN_ERR_MISSING_FILE_NAME;

    const char *fmode;
    if (sec == 1 || mode_char == 'W' || mode_char == 'w') {
        fmode = "wb";
    } else if (mode_char == 'A' || mode_char == 'a') {
        fmode = "ab";
    } else {
        fmode = "rb";
    }

    char *path = resolve_path(unit, namebuf);
    if (!path) {
        set_drive_status("74,DRIVE NOT READY,00,00\r");
        return KERN_ERR_FILE_NOT_FOUND;
    }

    files[lfn] = fopen(path, fmode);
    free(path);

    if (files[lfn]) return KERN_ERR_NONE;
    set_drive_status("62,FILE NOT FOUND,00,00\r");
    return KERN_ERR_FILE_NOT_FOUND;
}

void cbmdos_close(uint8_t lfn, uint8_t unit) {
    (void)unit;
    FILE *f = files[lfn];
    if (f == FILE_COMMAND_CHAN) {
        drive_status_p = cur_drive_status;
    } else if (f && f != FILE_DIRECTORY) {
        fclose(f);
    }
    files[lfn] = NULL;
    file_sa[lfn] = 0xFF;
}

void cbmdos_chkin(uint8_t lfn, uint8_t unit) {
    (void)unit;
    in_lfn = lfn;
}

void cbmdos_chkout(uint8_t lfn, uint8_t unit) {
    (void)unit;
    out_lfn = lfn;
}

uint8_t cbmdos_basin(uint8_t unit, uint8_t *status) {
    (void)unit;
    FILE *f = files[in_lfn];

    if (f == FILE_COMMAND_CHAN) {
        uint8_t c = (uint8_t)*drive_status_p;
        drive_status_p++;
        if (!*drive_status_p) {
            *status = KERN_ST_EOF;
            drive_status_p--;
        }
        return c;
    }

    if (f == FILE_DIRECTORY) {
        if (directory_data_p >= directory_data_end) {
            *status = KERN_ST_EOF | KERN_ST_TIME_OUT_READ;
            return 0;
        }
        uint8_t c = *directory_data_p++;
        if (directory_data_p >= directory_data_end) {
            *status = KERN_ST_EOF | KERN_ST_TIME_OUT_READ;
        }
        return c;
    }

    if (f) {
        int c = fgetc(f);
        if (c == EOF) {
            *status = KERN_ST_EOF;
            return 0;
        }
        // Peek ahead to signal EOF on last byte (matches real 1541 behaviour)
        int peek = fgetc(f);
        if (peek == EOF) {
            *status = KERN_ST_EOF;
        } else {
            ungetc(peek, f);
        }
        return (uint8_t)c;
    }

    *status = KERN_ST_EOF;
    return 0;
}

int cbmdos_bsout(uint8_t unit, uint8_t c) {
    (void)unit;
    FILE *f = files[out_lfn];

    if (f == FILE_COMMAND_CHAN) {
        if (command_p - command >= CMD_BUF_SIZE - 1) {
            set_drive_status("32,SYNTAX ERROR,00,00\r");
            return KERN_ERR_NONE;
        }
        *command_p++ = (char)c;
        /* Real 1541 only interprets the command when CR ($0D) is received */
        if (c == '\r') {
            command_unit_for_interpret = file_unit[out_lfn];
            interpret_command();
        }
        return KERN_ERR_NONE;
    }

    if (f && f != FILE_DIRECTORY) {
        return fputc(c, f) == EOF ? KERN_ERR_NOT_OUTPUT_FILE : KERN_ERR_NONE;
    }

    return KERN_ERR_NOT_OUTPUT_FILE;
}
