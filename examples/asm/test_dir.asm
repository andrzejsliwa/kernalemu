; test_dir.asm - Read and print directory listing from drive 8
; Run: kernalemu test_dir.prg -start 0xC000 -drive8 <path> -text
;
; The directory ($) is a BASIC-tokenised stream:
;   2 bytes  PRG load address (skip)
;   per line:
;     2 bytes link pointer (skip)
;     2 bytes line number  (skip)
;     N bytes content (print printable $20-$7E)
;     1 byte  $00 end-of-line
;   2 bytes $0000 end-of-program

        .cpu "6502"
        * = $C000

SETLFS  = $FFBA
SETNAM  = $FFBD
OPEN    = $FFC0
CHKIN   = $FFC6
BASIN   = $FFCF
CLRCHN  = $FFCC
CLOSE   = $FFC3
CHROUT  = $FFD2
READST  = $FFB7

start
        ; print header
        ldx #0
hdr_lp  lda hdr,x
        beq hdr_done
        jsr CHROUT
        inx
        bne hdr_lp
hdr_done

        ; OPEN 1,8,0,"$"
        lda #1
        ldx #8
        ldy #0
        jsr SETLFS
        lda #1
        ldx #<fn_dir
        ldy #>fn_dir
        jsr SETNAM
        jsr OPEN
        bcs open_err

        ldx #1
        jsr CHKIN

        ; skip 2-byte PRG load address
        jsr BASIN
        jsr BASIN

next_line
        ; read link lo
        jsr BASIN
        jsr READST
        and #$40
        bne dir_done
        ; link hi
        jsr BASIN
        ; line number lo + hi
        jsr BASIN
        jsr BASIN

        ; print bytes until $00
line_lp
        jsr BASIN
        beq got_eol
        ; print everything except control chars
        cmp #$20
        bcc skip_ch
        jsr CHROUT
skip_ch
        jsr READST
        and #$40
        bne dir_done
        jmp line_lp

got_eol
        lda #$0d
        jsr CHROUT
        jmp next_line

dir_done
        jsr CLRCHN
        lda #1
        jsr CLOSE

        ; footer
        ldx #0
ftr_lp  lda ftr,x
        beq halt
        jsr CHROUT
        inx
        bne ftr_lp

open_err
        ldx #0
err_lp  lda errmsg,x
        beq halt
        jsr CHROUT
        inx
        bne err_lp
halt    jmp halt

hdr     .text "=== DIRECTORY LISTING ===",$0d,0
ftr     .text "=== END ===",$0d,0
errmsg  .text "ERROR: CANNOT OPEN $",$0d,0
fn_dir  .text "$"
