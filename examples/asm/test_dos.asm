; test_dos.asm - DOS command channel: scratch, rename, copy, validate, UI reset
; Run: kernalemu test_dos.prg -start 0xC000 -drive8 <path> -text
;
; Steps:
;   1. Create work.seq
;   2. Rename work.seq -> renamed.seq
;   3. Copy renamed.seq -> copy.seq
;   4. Read copy.seq back
;   5. Validate (V)
;   6. Scratch renamed.seq,copy.seq
;   7. Soft reset UI - check 1541 ident
;   8. Wildcard scratch *.tmp (should scratch 0 files)

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

PTR     = $fb
CMDP    = $fd   ; pointer for command strings

ptrset  stx PTR
        sty PTR+1
        rts

        jmp start

print_str
        ldy #0
ps_lp   lda (PTR),y
        beq ps_done
        jsr CHROUT
        iny
        bne ps_lp
ps_done rts

write_str
        ldy #0
ws_lp   lda (PTR),y
        beq ws_done
        jsr BSOUT
        iny
        bne ws_lp
ws_done rts

; open_cmd: open LFN 15, device 8, SA 15 with command string at CMDP/CMDP+1
; A = length of command string
open_cmd
        pha
        lda #15
        ldx #8
        ldy #15
        jsr SETLFS
        pla
        ldx CMDP
        ldy CMDP+1
        jsr SETNAM
        jsr OPEN
        rts

; read_status: CHKIN 15, print until CR, CLRCHN, CLOSE 15
read_status
        ldx #15
        jsr CHKIN
        lda #32
        jsr CHROUT
rs_lp   jsr BASIN
        cmp #$0d
        beq rs_done
        jsr CHROUT
        jsr READST
        and #$40
        beq rs_lp
rs_done lda #$0d
        jsr CHROUT
        jsr CLRCHN
        lda #15
        jsr CLOSE
        rts

start
        ldx #<hdr
        ldy #>hdr
        jsr ptrset
        jsr print_str

        ; Step 1: create work.seq
        ldx #<s_step1
        ldy #>s_step1
        jsr ptrset
        jsr print_str

        lda #1
        ldx #8
        ldy #1
        jsr SETLFS
        lda #fw_work_e-fw_work
        ldx #<fw_work
        ldy #>fw_work
        jsr SETNAM
        jsr OPEN
        bcs step1_err

        ldx #1
        jsr CHKOUT
        ldx #<payload
        ldy #>payload
        jsr ptrset
        jsr write_str
        lda #$0d
        jsr BSOUT
        jsr CLRCHN
        lda #1
        jsr CLOSE
        jmp step2

step1_err
        ldx #<err_create
        ldy #>err_create
        jsr ptrset
        jsr print_str
        jmp halt

        ; Step 2: read drive status (idle)
step2   ldx #<s_step2
        ldy #>s_step2
        jsr ptrset
        jsr print_str
        lda #0
        ldx #<cmd_empty
        stx CMDP
        ldy #>cmd_empty
        sty CMDP+1
        jsr open_cmd
        jsr read_status

        ; Step 3: rename work.seq -> renamed.seq
        ldx #<s_step3
        ldy #>s_step3
        jsr ptrset
        jsr print_str
        lda #cmd_rename_e-cmd_rename
        ldx #<cmd_rename
        stx CMDP
        ldy #>cmd_rename
        sty CMDP+1
        jsr open_cmd
        jsr read_status

        ; Step 4: copy renamed.seq -> copy.seq
        ldx #<s_step4
        ldy #>s_step4
        jsr ptrset
        jsr print_str
        lda #cmd_copy_e-cmd_copy
        ldx #<cmd_copy
        stx CMDP
        ldy #>cmd_copy
        sty CMDP+1
        jsr open_cmd
        jsr read_status

        ; Step 5: read copy.seq back
        ldx #<s_step5
        ldy #>s_step5
        jsr ptrset
        jsr print_str
        lda #3
        ldx #8
        ldy #2
        jsr SETLFS
        lda #fr_copy_e-fr_copy
        ldx #<fr_copy
        ldy #>fr_copy
        jsr SETNAM
        jsr OPEN
        bcs step5_err

        ldx #3
        jsr CHKIN
        ldy #0
rd_lp   jsr BASIN
        cmp #$0d
        beq rd_done
        sta linebuf,y
        iny
        cpy #79
        beq rd_done
        jsr READST
        and #$40
        beq rd_lp
rd_done lda #0
        sta linebuf,y
        jsr CLRCHN
        lda #3
        jsr CLOSE
        lda #32
        jsr CHROUT
        ldx #<linebuf
        ldy #>linebuf
        jsr ptrset
        jsr print_str
        lda #$0d
        jsr CHROUT
        jmp step6

step5_err
        ldx #<err_read
        ldy #>err_read
        jsr ptrset
        jsr print_str

        ; Step 6: validate
step6   ldx #<s_step6
        ldy #>s_step6
        jsr ptrset
        jsr print_str
        lda #cmd_validate_e-cmd_validate
        ldx #<cmd_validate
        stx CMDP
        ldy #>cmd_validate
        sty CMDP+1
        jsr open_cmd
        jsr read_status

        ; Step 7: scratch renamed.seq,copy.seq
        ldx #<s_step7
        ldy #>s_step7
        jsr ptrset
        jsr print_str
        lda #cmd_scratch_e-cmd_scratch
        ldx #<cmd_scratch
        stx CMDP
        ldy #>cmd_scratch
        sty CMDP+1
        jsr open_cmd
        jsr read_status

        ; Step 8: soft reset UI - check 1541 ident
        ldx #<s_step8
        ldy #>s_step8
        jsr ptrset
        jsr print_str
        lda #cmd_ui_e-cmd_ui
        ldx #<cmd_ui
        stx CMDP
        ldy #>cmd_ui
        sty CMDP+1
        jsr open_cmd
        jsr read_status

        ; Step 9: wildcard scratch *.tmp
        ldx #<s_step9
        ldy #>s_step9
        jsr ptrset
        jsr print_str
        lda #cmd_wc_e-cmd_wc
        ldx #<cmd_wc
        stx CMDP
        ldy #>cmd_wc
        sty CMDP+1
        jsr open_cmd
        jsr read_status

        ; Done
        ldx #<s_done
        ldy #>s_done
        jsr ptrset
        jsr print_str
halt    jmp halt

linebuf         .fill 80,0
hdr             .text "=== DOS COMMAND CHANNEL TEST ===",$0d,0
s_step1         .text "1: CREATE work.seq",$0d,0
s_step2         .text "2: DRIVE STATUS (idle)",$0d,0
s_step3         .text "3: RENAME work.seq->renamed.seq",$0d,0
s_step4         .text "4: COPY renamed.seq->copy.seq",$0d,0
s_step5         .text "5: READ copy.seq",$0d,0
s_step6         .text "6: VALIDATE",$0d,0
s_step7         .text "7: SCRATCH renamed.seq,copy.seq",$0d,0
s_step8         .text "8: SOFT RESET (UI)",$0d,0
s_step9         .text "9: SCRATCH *.tmp (wildcard)",$0d,0
s_done          .text "ALL STEPS DONE.",$0d,0
err_create      .text "  ERROR CREATING work.seq",$0d,0
err_read        .text "  ERROR READING copy.seq",$0d,0
payload         .text "PAYLOAD FROM ASSEMBLY TEST",0
fw_work         .text "work.seq,s,w"
fw_work_e
fr_copy         .text "copy.seq,s,r"
fr_copy_e
cmd_empty
cmd_rename      .text "r:renamed.seq=work.seq"
cmd_rename_e
cmd_copy        .text "c:copy.seq=renamed.seq"
cmd_copy_e
cmd_validate    .text "v"
cmd_validate_e
cmd_scratch     .text "s:renamed.seq,copy.seq"
cmd_scratch_e
cmd_ui          .text "ui"
cmd_ui_e
cmd_wc          .text "s:*.tmp"
cmd_wc_e
