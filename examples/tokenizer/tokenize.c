/*
 * tokenize.c - Commodore 64 BASIC V2 Tokenizer
 *
 * Converts plain-text BASIC source (one line per text line,
 * format: "<linenum> <statement>") into a .PRG file loadable
 * at $0801 (C64 BASIC start address).
 *
 * Build:
 *   cc -Wall -o tokenize tokenize.c
 *
 * Usage:
 *   tokenize input.bas output.prg
 *
 * Source format:
 *   - Lines start with a decimal line number (1-63999)
 *   - Keywords are tokenized (case-insensitive in source)
 *   - Strings in double-quotes are passed through verbatim
 *   - REM and DATA content passed through verbatim after token
 *   - Line numbers in GOTO/GOSUB etc. are NOT pre-resolved
 *     (they stay as ASCII digits - same as a real C64 does)
 *
 * The output PRG has a 2-byte load address header ($01 $08),
 * followed by the tokenized BASIC program.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#define LOAD_ADDR    0x0801
#define MAX_LINE_LEN 256
#define MAX_PRG_SIZE 65536

/* -------------------------------------------------------------------------
 * BASIC V2 keyword table
 * Order matters: longer/earlier entries take priority.
 * Token byte = 0x80 + index.
 * -------------------------------------------------------------------------*/
static const char *keywords[] = {
    /* 0x80 */ "END",
    /* 0x81 */ "FOR",
    /* 0x82 */ "NEXT",
    /* 0x83 */ "DATA",
    /* 0x84 */ "INPUT#",
    /* 0x85 */ "INPUT",
    /* 0x86 */ "DIM",
    /* 0x87 */ "READ",
    /* 0x88 */ "LET",
    /* 0x89 */ "GOTO",
    /* 0x8A */ "RUN",
    /* 0x8B */ "IF",
    /* 0x8C */ "RESTORE",
    /* 0x8D */ "GOSUB",
    /* 0x8E */ "RETURN",
    /* 0x8F */ "REM",
    /* 0x90 */ "STOP",
    /* 0x91 */ "ON",
    /* 0x92 */ "WAIT",
    /* 0x93 */ "LOAD",
    /* 0x94 */ "SAVE",
    /* 0x95 */ "VERIFY",
    /* 0x96 */ "DEF",
    /* 0x97 */ "POKE",
    /* 0x98 */ "PRINT#",
    /* 0x99 */ "PRINT",
    /* 0x9A */ "CONT",
    /* 0x9B */ "LIST",
    /* 0x9C */ "CLR",
    /* 0x9D */ "CMD",
    /* 0x9E */ "SYS",
    /* 0x9F */ "OPEN",
    /* 0xA0 */ "CLOSE",
    /* 0xA1 */ "GET",
    /* 0xA2 */ "NEW",
    /* 0xA3 */ "TAB(",
    /* 0xA4 */ "TO",
    /* 0xA5 */ "FN",
    /* 0xA6 */ "SPC(",
    /* 0xA7 */ "THEN",
    /* 0xA8 */ "NOT",
    /* 0xA9 */ "STEP",
    /* 0xAA */ "AND",  /* also + - * / ^ */
    /* 0xAB */ "OR",
    /* 0xAC */ ">",
    /* 0xAD */ "=",
    /* 0xAE */ "<",
    /* 0xAF */ "SGN",
    /* 0xB0 */ "INT",
    /* 0xB1 */ "ABS",
    /* 0xB2 */ "USR",
    /* 0xB3 */ "FRE",
    /* 0xB4 */ "POS",
    /* 0xB5 */ "SQR",
    /* 0xB6 */ "RND",
    /* 0xB7 */ "LOG",
    /* 0xB8 */ "EXP",
    /* 0xB9 */ "COS",
    /* 0xBA */ "SIN",
    /* 0xBB */ "TAN",
    /* 0xBC */ "ATN",
    /* 0xBD */ "PEEK",
    /* 0xBE */ "LEN",
    /* 0xBF */ "STR$",
    /* 0xC0 */ "VAL",
    /* 0xC1 */ "ASC",
    /* 0xC2 */ "CHR$",
    /* 0xC3 */ "LEFT$",
    /* 0xC4 */ "RIGHT$",
    /* 0xC5 */ "MID$",
    /* 0xC6 */ "GO",
    NULL
};
#define NUM_KEYWORDS 71   /* 0x80 .. 0xC6 */

/* -------------------------------------------------------------------------
 * Tokenize one logical BASIC line.
 * src  : pointer into the text after the line number
 * out  : output buffer
 * Returns number of bytes written (not including the terminating 0x00).
 * -------------------------------------------------------------------------*/
static int tokenize_line(const char *src, uint8_t *out) {
    int outlen = 0;
    int in_rem  = 0;   /* inside REM - pass through raw */
    int in_data = 0;   /* inside DATA - pass through raw */

    while (*src) {
        /* -- String literals: pass through verbatim including the quotes -- */
        if (*src == '"') {
            out[outlen++] = (uint8_t)*src++;
            while (*src && *src != '"') {
                out[outlen++] = (uint8_t)*src++;
            }
            if (*src == '"') out[outlen++] = (uint8_t)*src++;
            continue;
        }

        /* -- After REM token: copy rest of line verbatim -- */
        if (in_rem) {
            out[outlen++] = (uint8_t)*src++;
            continue;
        }

        /* -- After DATA token: pass through until colon or end -- */
        if (in_data) {
            if (*src == ':') {
                in_data = 0;
                out[outlen++] = (uint8_t)*src++;
            } else {
                out[outlen++] = (uint8_t)*src++;
            }
            continue;
        }

        /* -- Try to match a keyword (case-insensitive) -- */
        int matched = 0;
        for (int k = 0; keywords[k]; k++) {
            size_t klen = strlen(keywords[k]);
            if (strncasecmp(src, keywords[k], klen) == 0) {
                uint8_t tok = (uint8_t)(0x80 + k);
                out[outlen++] = tok;
                src += klen;
                if (tok == 0x8F /* REM */) in_rem  = 1;
                if (tok == 0x83 /* DATA*/) in_data = 1;
                matched = 1;
                break;
            }
        }
        if (matched) continue;

        /* -- Pass through space-stripped character -- */
        /* Real C64 tokenizer strips spaces outside strings/REM/DATA */
        if (*src == ' ') {
            src++;
            continue;
        }

        out[outlen++] = (uint8_t)(toupper((unsigned char)*src));
        src++;
    }

    out[outlen] = 0x00; /* line terminator */
    return outlen;
}

/* -------------------------------------------------------------------------
 * Main
 * -------------------------------------------------------------------------*/
int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s input.bas output.prg\n", argv[0]);
        return 1;
    }

    FILE *fin = fopen(argv[1], "r");
    if (!fin) { perror(argv[1]); return 1; }

    /* PRG buffer: starts with 2-byte load address, then BASIC program */
    uint8_t prg[MAX_PRG_SIZE];
    int prg_pos = 0;

    /* Load address $0801 - little-endian */
    prg[prg_pos++] = LOAD_ADDR & 0xFF;
    prg[prg_pos++] = LOAD_ADDR >> 8;

    /* BASIC program starts at $0801 in memory.
     * We build link pointers relative to that base. */
    uint16_t mem_addr = LOAD_ADDR;

    char text[MAX_LINE_LEN];
    uint8_t line_buf[MAX_LINE_LEN * 2]; /* tokenized line content */

    /* Collect all lines first so we can patch link pointers */
    typedef struct {
        uint16_t linenum;
        uint8_t  data[MAX_LINE_LEN * 2];
        int      datalen; /* bytes in data[], not counting the NUL terminator */
    } BasicLine;

    BasicLine *lines = malloc(sizeof(BasicLine) * 10000);
    if (!lines) { fprintf(stderr, "Out of memory\n"); return 1; }
    int num_lines = 0;

    while (fgets(text, sizeof(text), fin)) {
        /* Strip trailing newline/CR */
        int tlen = (int)strlen(text);
        while (tlen > 0 && (text[tlen-1] == '\n' || text[tlen-1] == '\r')) {
            text[--tlen] = '\0';
        }
        if (tlen == 0) continue;      /* blank line - skip */
        if (text[0] == ';') continue; /* comment line */

        /* Parse line number */
        char *endnum;
        long linenum = strtol(text, &endnum, 10);
        if (endnum == text || linenum < 0 || linenum > 63999) {
            fprintf(stderr, "Bad line number: %s\n", text);
            fclose(fin);
            free(lines);
            return 1;
        }

        /* Skip whitespace after line number */
        while (*endnum == ' ') endnum++;

        /* Tokenize the statement part */
        int tok_len = tokenize_line(endnum, line_buf);

        lines[num_lines].linenum = (uint16_t)linenum;
        memcpy(lines[num_lines].data, line_buf, tok_len + 1); /* +1 for NUL */
        lines[num_lines].datalen = tok_len;
        num_lines++;

        if (num_lines >= 10000) {
            fprintf(stderr, "Too many lines (max 10000)\n");
            fclose(fin);
            free(lines);
            return 1;
        }
    }
    fclose(fin);

    /* Now emit all lines with correct link pointers */
    for (int i = 0; i < num_lines; i++) {
        /* Each line record:
         *   2 bytes: link to next line (absolute address in C64 memory)
         *   2 bytes: line number (little-endian)
         *   N bytes: tokenized statement
         *   1 byte:  0x00 terminator
         */
        int record_len = 2 + 2 + lines[i].datalen + 1;

        /* Calculate address of NEXT line */
        uint16_t next_addr;
        if (i < num_lines - 1) {
            next_addr = mem_addr + (uint16_t)record_len;
        } else {
            next_addr = mem_addr + (uint16_t)record_len;
            /* After last line comes the end-of-program marker (0x0000) */
        }

        /* Link pointer */
        prg[prg_pos++] = next_addr & 0xFF;
        prg[prg_pos++] = next_addr >> 8;

        /* Line number */
        prg[prg_pos++] = lines[i].linenum & 0xFF;
        prg[prg_pos++] = lines[i].linenum >> 8;

        /* Tokenized content + NUL */
        memcpy(prg + prg_pos, lines[i].data, lines[i].datalen + 1);
        prg_pos += lines[i].datalen + 1;

        mem_addr = next_addr;

        if (prg_pos >= MAX_PRG_SIZE - 16) {
            fprintf(stderr, "PRG too large\n");
            free(lines);
            return 1;
        }
    }

    /* End-of-program marker: link = $0000 */
    prg[prg_pos++] = 0x00;
    prg[prg_pos++] = 0x00;

    free(lines);

    /* Write output */
    FILE *fout = fopen(argv[2], "wb");
    if (!fout) { perror(argv[2]); return 1; }
    if (fwrite(prg, 1, prg_pos, fout) != (size_t)prg_pos) {
        perror("fwrite");
        fclose(fout);
        return 1;
    }
    fclose(fout);

    fprintf(stderr, "Wrote %d bytes (%d lines) to %s\n",
            prg_pos, num_lines, argv[2]);
    return 0;
}
