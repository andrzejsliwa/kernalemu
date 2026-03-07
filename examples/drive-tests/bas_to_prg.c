/*
 * Commodore BASIC 2.0 tokenizer.
 * This work is in the public domain, covered under the UNLICENSE;
 * see the file UNLICENSE in the root directory of this distribution,
 * or http://www.unlicense.org/ for full details.
 *
 * references:
 *   http://justsolve.archiveteam.org/wiki/Commodore_BASIC_tokenized_file
 *   http://www.c64-wiki.com/index.php/BASIC_token
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE_LEN  4096
#define MAX_BYTES     4096
#define MAX_LINES     65536

static const struct { const char *token; int value; } TOKENS[] = {
    { "restore", 140 },
    { "input#",  132 },
    { "return",  142 },
    { "verify",  149 },
    { "print#",  152 },
    { "right$",  201 },
    { "input",   133 },
    { "gosub",   141 },
    { "print",   153 },
    { "close",   160 },
    { "left$",   200 },
    { "next",    130 },
    { "data",    131 },
    { "read",    135 },
    { "goto",    137 },
    { "stop",    144 },
    { "wait",    146 },
    { "load",    147 },
    { "save",    148 },
    { "poke",    151 },
    { "cont",    154 },
    { "list",    155 },
    { "open",    159 },
    { "tab(",    163 },
    { "spc(",    166 },
    { "then",    167 },
    { "step",    169 },
    { "peek",    194 },
    { "str$",    196 },
    { "chr$",    199 },
    { "mid$",    202 },
    { "end",     128 },
    { "for",     129 },
    { "dim",     134 },
    { "let",     136 },
    { "run",     138 },
    { "rem",     143 },
    { "def",     150 },
    { "clr",     156 },
    { "cmd",     157 },
    { "sys",     158 },
    { "get",     161 },
    { "new",     162 },
    { "not",     168 },
    { "and",     175 },
    { "sgn",     180 },
    { "int",     181 },
    { "abs",     182 },
    { "usr",     183 },
    { "fre",     184 },
    { "pos",     185 },
    { "sqr",     186 },
    { "rnd",     187 },
    { "log",     188 },
    { "exp",     189 },
    { "cos",     190 },
    { "sin",     191 },
    { "tan",     192 },
    { "atn",     193 },
    { "len",     195 },
    { "val",     197 },
    { "asc",     198 },
    { "if",      139 },
    { "on",      145 },
    { "to",      164 },
    { "fn",      165 },
    { "or",      176 },
    { "go",      203 },
    { "+",       170 },
    { "-",       171 },
    { "*",       172 },
    { "/",       173 },
    { "^",       174 },
    { ">",       177 },
    { "=",       178 },
    { "<",       179 },
    { NULL, 0 }
};

static const struct { const char *token; int value; } SPECIAL[] = {
    { "{rvs off}",  0x92 },
    { "{rvof}",     0x92 },
    { "{SHIFT-@}",  0xba },
    { "{rvs on}",   0x12 },
    { "{rvon}",     0x12 },
    { "{CBM-+}",    0xa6 },
    { "{CBM-E}",    0xb1 },
    { "{CBM-R}",    0xb2 },
    { "{CBM-T}",    0xa3 },
    { "{down}",     0x11 },
    { "{home}",     0x13 },
    { "{lblu}",     0x9a },
    { "{left}",     0x9d },
    { "{rght}",     0x1d },
    { "{blk}",      0x90 },
    { "{blu}",      0x1f },
    { "{clr}",      0x93 },
    { "{cyn}",      0x9f },
    { "{grn}",      0x1e },
    { "{pur}",      0x9c },
    { "{red}",      0x1c },
    { "{wht}",      0x05 },
    { "{yel}",      0x9e },
    { "{up}",       0x91 },
    { NULL, 0 }
};

static int ascii_to_petscii(int o) {
    if (o <= '@' || o == '[' || o == ']')
        return o;
    if (o >= 'a' && o <= 'z')
        return o - 'a' + 0x41;
    if (o >= 'A' && o <= 'Z')
        return o - 'A' + 0x61 + 0x60;
    fprintf(stderr, "Cannot PETSCII: 0x%x\n", o);
    exit(1);
}

static int scan(const char **s, int tokenize) {
    if (tokenize) {
        for (int i = 0; TOKENS[i].token; i++) {
            size_t len = strlen(TOKENS[i].token);
            if (strncmp(*s, TOKENS[i].token, len) == 0) {
                *s += len;
                return TOKENS[i].value;
            }
        }
    }
    if (**s == '{') {
        for (int i = 0; SPECIAL[i].token; i++) {
            size_t len = strlen(SPECIAL[i].token);
            if (strncmp(*s, SPECIAL[i].token, len) == 0) {
                *s += len;
                return SPECIAL[i].value;
            }
        }
        fprintf(stderr, "Unknown special sequence: %s\n", *s);
        exit(1);
    }
    int byte = ascii_to_petscii((unsigned char)**s);
    (*s)++;
    return byte;
}

static int scan_line_number(const char **s) {
    while (**s == ' ' || **s == '\t') (*s)++;
    int num = 0;
    while (isdigit((unsigned char)**s)) {
        num = num * 10 + (**s - '0');
        (*s)++;
    }
    while (**s == ' ' || **s == '\t') (*s)++;
    return num;
}

typedef struct {
    int line_number;
    int bytes[MAX_BYTES];
    int nbytes;
    int addr;
    int next_addr;
} TokenizedLine;

static void tokenize_line(const char *s, TokenizedLine *tl, int addr) {
    tl->line_number = scan_line_number(&s);
    tl->nbytes = 0;
    tl->addr = addr;
    tl->next_addr = -1;

    int in_quotes = 0;
    int in_remark = 0;
    while (*s) {
        int byte = scan(&s, !(in_quotes || in_remark));
        tl->bytes[tl->nbytes++] = byte;
        if (byte == '"')
            in_quotes = !in_quotes;
        if (byte == 143)
            in_remark = 1;
    }
}

/* next_addr(2) + line_num(2) + bytes + NUL(1) */
static int tl_len(const TokenizedLine *tl) {
    return tl->nbytes + 5;
}

static void write_word(FILE *f, int word) {
    fputc(word & 0xff, f);
    fputc((word >> 8) & 0xff, f);
}

static void write_line(FILE *f, const TokenizedLine *tl) {
    write_word(f, tl->next_addr);
    write_word(f, tl->line_number);
    for (int i = 0; i < tl->nbytes; i++)
        fputc(tl->bytes[i], f);
    fputc(0, f);
}

int main(int argc, char *argv[]) {
    int start_addr = 0x0801;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-l") == 0 && i + 1 < argc) {
            start_addr = (int)strtol(argv[++i], NULL, 16);
        } else {
            fprintf(stderr, "Unknown switch: %s\n", argv[i]);
            return 1;
        }
    }

    TokenizedLine *lines = malloc(MAX_LINES * sizeof(TokenizedLine));
    if (!lines) { perror("malloc"); return 1; }

    int nlines = 0;
    int addr = start_addr;
    char buf[MAX_LINE_LEN];

    while (fgets(buf, sizeof(buf), stdin)) {
        int len = (int)strlen(buf);
        while (len > 0 && (buf[len-1] == '\n' || buf[len-1] == '\r'))
            buf[--len] = '\0';

        int blank = 1;
        for (int i = 0; buf[i]; i++)
            if (!isspace((unsigned char)buf[i])) { blank = 0; break; }
        if (blank) continue;

        if (nlines >= MAX_LINES) {
            fprintf(stderr, "Too many lines\n");
            return 1;
        }
        tokenize_line(buf, &lines[nlines], addr);
        addr += tl_len(&lines[nlines]);
        nlines++;
    }

    int sentinel_addr = addr;

    for (int i = 0; i < nlines; i++)
        lines[i].next_addr = (i + 1 < nlines) ? lines[i + 1].addr : sentinel_addr;

    write_word(stdout, start_addr);
    for (int i = 0; i < nlines; i++)
        write_line(stdout, &lines[i]);
    write_word(stdout, 0); /* sentinel */

    free(lines);
    return 0;
}
