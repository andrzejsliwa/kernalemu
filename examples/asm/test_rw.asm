; test_rw.asm - Write a sequential file then read it back
; Run: kernalemu test_rw.prg -start 0xC000 -drive8 <path> -text

        .cpu "6502"
        * = $C000

SETLFS  = $FFBA
SETNAM  = $FFBD
OPEN    = $FFC0
CHKIN   = $FFC6
CHKOUT  = $FFC9
BASIN   = $FFCF
BSOUT   = $FFD2
CLRCHN  = $FFCC
CLOSE   = $FFC3
CHROUT  = $FFD2
READST  = $FFB7

; Zero page string pointer
PTR     = $fb

; print_str: print null-terminated string; address in PTR/PTR+1
        jmp start

print_str
        ldy #0
ps_lp   lda (PTR),y
        beq ps_done
        jsr CHROUT
        iny
        bne ps_lp
ps_done rts

; write_str: write null-terminated string to output channel; address in PTR/PTR+1
write_str
        ldy #0
ws_lp   lda (PTR),y
        beq ws_done
        jsr BSOUT
        iny
        bne ws_lp
ws_done rts

; setptr macro inlined as subroutine - we use a helper
; ptrset: X=lo, Y=hi -> PTR
ptrset  stx PTR
        sty PTR+1
        rts

start
        ldx #<hdr
        ldy #>hdr
        jsr ptrset
        jsr print_str

        ; --- WRITE PHASE ---
        ldx #<msg_writing
        ldy #>msg_writing
        jsr ptrset
        jsr print_str

        lda #1
        ldx #8
        ldy #1
        jsr SETLFS
        lda #fw_end-fw
        ldx #<fw
        ldy #>fw
        jsr SETNAM
        jsr OPEN
        bcc wopen_ok
        jmp write_err
wopen_ok

        ldx #1
        jsr CHKOUT

        ; write three lines
        ldx #<line1
        ldy #>line1
        jsr ptrset
        jsr write_str
        lda #$0d
        jsr BSOUT

        ldx #<line2
        ldy #>line2
        jsr ptrset
        jsr write_str
        lda #$0d
        jsr BSOUT

        ldx #<line3
        ldy #>line3
        jsr ptrset
        jsr write_str
        lda #$0d
        jsr BSOUT

        jsr CLRCHN
        lda #1
        jsr CLOSE

        ldx #<msg_wdone
        ldy #>msg_wdone
        jsr ptrset
        jsr print_str

        ; --- READ PHASE ---
        ldx #<msg_reading
        ldy #>msg_reading
        jsr ptrset
        jsr print_str

        lda #2
        ldx #8
        ldy #2
        jsr SETLFS
        lda #fr_end-fr
        ldx #<fr
        ldy #>fr
        jsr SETNAM
        jsr OPEN
        bcc ropen_ok
        jmp read_err
ropen_ok

        ldx #2
        jsr CHKIN

read_line
        ; read bytes until CR or EOF into linebuf
        ldy #0
read_byte
        jsr BASIN
        cmp #$0d
        beq got_cr
        sta linebuf,y
        iny
        cpy #79
        beq got_cr
        jsr READST
        and #$40
        bne read_eof
        jmp read_byte

got_cr
        lda #0
        sta linebuf,y
        ; print "  <content>\n"
        lda #32
        jsr CHROUT
        ldx #<linebuf
        ldy #>linebuf
        jsr ptrset
        jsr print_str
        lda #$0d
        jsr CHROUT
        jsr READST
        and #$40
        beq read_line

read_eof
        jsr CLRCHN
        lda #2
        jsr CLOSE
        ldx #<msg_rdone
        ldy #>msg_rdone
        jsr ptrset
        jsr print_str
halt    jmp halt

write_err
        ldx #<msg_werr
        ldy #>msg_werr
        jsr ptrset
        jsr print_str
        jmp halt

read_err
        ldx #<msg_rerr
        ldy #>msg_rerr
        jsr ptrset
        jsr print_str
        jmp halt

linebuf         .fill 80,0
hdr             .text "=== FILE READ/WRITE TEST ===",$0d,0
msg_writing     .text "WRITING TESTFILE.SEQ...",$0d,0
msg_wdone       .text "WRITE DONE.",$0d,0
msg_reading     .text "READING TESTFILE.SEQ...",$0d,0
msg_rdone       .text "READ DONE.",$0d,0
msg_werr        .text "ERROR: WRITE OPEN FAILED",$0d,0
msg_rerr        .text "ERROR: READ OPEN FAILED",$0d,0
line1           .text "HELLO FROM ASSEMBLY",0
line2           .text "SECOND LINE OF DATA",0
line3           .text "THIRD AND FINAL LINE",0
fw              .text "testfile.seq,s,w"
fw_end
fr              .text "testfile.seq,s,r"
fr_end
