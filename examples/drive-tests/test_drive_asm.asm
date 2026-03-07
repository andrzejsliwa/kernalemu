; =============================================================================
; test_drive_asm.asm  -  1541 DOS emulation test
; Assemble: 64tass --cbm-prg -o test_drive_asm.prg test_drive_asm.asm
;
; Tests 7 drive functions and prints PASS/FAIL for each, then a summary.
; Run with kernalemu:
;   kernalemu test_drive_asm.prg -drive8 /some/dir
; =============================================================================

CHROUT  = $FFD2
CHRIN   = $FFCF
KOPEN   = $FFC0
KCLOSE  = $FFC3
CHKIN   = $FFC6
CHKOUT  = $FFC9
CLRCHN  = $FFCC
READST  = $FFB7
SETLFS  = $FFBA
SETNAM  = $FFBD

; Zero-page
TESTNUM = $FC
PASSCNT = $FD

        * = $0801
; BASIC header: 10 SYS 2064
        .word _bnext
        .word 10
        .byte $9e
        .text "2064"
        .byte 0
_bnext  .word 0

        * = $0810

; -------------------------------------------------------
start
        lda #$93
        jsr CHROUT          ; clear screen
        lda #0
        sta PASSCNT
        sta TESTNUM

; -------------------------------------------------------
; T1: UI - drive ident
; -------------------------------------------------------
        lda #1
        sta TESTNUM
        lda #15
        ldx #8
        ldy #15
        jsr SETLFS
        lda #2
        ldx #<n_ui
        ldy #>n_ui
        jsr SETNAM
        jsr KOPEN
        ldx #15
        jsr CHKIN
        ldx #0
_t1r    jsr CHRIN
        cmp #$0d
        beq _t1d
        sta sbuf,x
        inx
        cpx #62
        bne _t1r
_t1d    jsr CLRCHN
        lda #15
        jsr KCLOSE
        lda sbuf+0
        cmp #$37            ; '7'
        bne _t1f
        lda sbuf+1
        cmp #$33            ; '3'
        bne _t1f
        inc PASSCNT
        jsr p_pass
        jmp t2
_t1f    jsr p_fail

; -------------------------------------------------------
; T2: Write file SA=1, read back SA=0
; -------------------------------------------------------
t2      lda #2
        sta TESTNUM
        lda #2
        ldx #8
        ldy #1
        jsr SETLFS
        lda #8
        ldx #<n_tfile
        ldy #>n_tfile
        jsr SETNAM
        jsr KOPEN
        bcc _t2ok1
        jmp _t2f
_t2ok1  ldx #2
        jsr CHKOUT
        lda #$48            ; H
        jsr CHROUT
        lda #$45            ; E
        jsr CHROUT
        lda #$4c            ; L
        jsr CHROUT
        lda #$4c            ; L
        jsr CHROUT
        lda #$4f            ; O
        jsr CHROUT
        jsr CLRCHN
        lda #2
        jsr KCLOSE
        ; read back
        lda #3
        ldx #8
        ldy #0
        jsr SETLFS
        lda #8
        ldx #<n_tfile
        ldy #>n_tfile
        jsr SETNAM
        jsr KOPEN
        bcc _t2ok2
        jmp _t2f
_t2ok2  ldx #3
        jsr CHKIN
        ldx #0
_t2r    jsr CHRIN
        sta rbuf,x
        inx
        jsr READST
        and #$40
        bne _t2e
        cpx #16
        bne _t2r
_t2e    stx rlen
        jsr CLRCHN
        lda #3
        jsr KCLOSE
        lda rlen
        cmp #5
        bne _t2f
        lda rbuf+0
        cmp #$48
        bne _t2f
        lda rbuf+1
        cmp #$45
        bne _t2f
        lda rbuf+2
        cmp #$4c
        bne _t2f
        lda rbuf+3
        cmp #$4c
        bne _t2f
        lda rbuf+4
        cmp #$4f
        bne _t2f
        inc PASSCNT
        jsr p_pass
        jmp t3
_t2f    jsr p_fail

; -------------------------------------------------------
; T3: Scratch file
; -------------------------------------------------------
t3      lda #3
        sta TESTNUM
        lda #4
        ldx #8
        ldy #15
        jsr SETLFS
        lda #n_scr_e - n_scr
        ldx #<n_scr
        ldy #>n_scr
        jsr SETNAM
        jsr KOPEN
        ldx #4
        jsr CHKIN
        jsr CHRIN
        sta tmp0
        jsr CHRIN
        sta tmp1
        jsr CLRCHN
        lda #4
        jsr KCLOSE
        lda tmp0
        cmp #$30
        bne _t3f
        lda tmp1
        cmp #$30            ; "00" OK
        beq _t3ok
        cmp #$31            ; "01" FILES SCRATCHED
        beq _t3ok
_t3f    jsr p_fail
        jmp t4
_t3ok   inc PASSCNT
        jsr p_pass

; -------------------------------------------------------
; T4: Rename  R:newname=oldname
; -------------------------------------------------------
t4      lda #4
        sta TESTNUM
        ; create source file
        lda #5
        ldx #8
        ldy #1
        jsr SETLFS
        lda #5
        ldx #<n_rnsrc
        ldy #>n_rnsrc
        jsr SETNAM
        jsr KOPEN
        bcs _t4f
        ldx #5
        jsr CHKOUT
        lda #$52            ; R
        jsr CHROUT
        lda #$4e            ; N
        jsr CHROUT
        jsr CLRCHN
        lda #5
        jsr KCLOSE
        ; send rename command
        lda #6
        ldx #8
        ldy #15
        jsr SETLFS
        lda #n_rncmd_e - n_rncmd
        ldx #<n_rncmd
        ldy #>n_rncmd
        jsr SETNAM
        jsr KOPEN
        ldx #6
        jsr CHKIN
        jsr CHRIN
        sta tmp0
        jsr CHRIN
        sta tmp1
        jsr CLRCHN
        lda #6
        jsr KCLOSE
        lda tmp0
        cmp #$30
        bne _t4f
        lda tmp1
        cmp #$30
        bne _t4f
        inc PASSCNT
        jsr p_pass
        jmp t5
_t4f    jsr p_fail

; -------------------------------------------------------
; T5: Copy  C:dest=src
; -------------------------------------------------------
t5      lda #5
        sta TESTNUM
        lda #7
        ldx #8
        ldy #15
        jsr SETLFS
        lda #n_cpcmd_e - n_cpcmd
        ldx #<n_cpcmd
        ldy #>n_cpcmd
        jsr SETNAM
        jsr KOPEN
        ldx #7
        jsr CHKIN
        jsr CHRIN
        sta tmp0
        jsr CHRIN
        sta tmp1
        jsr CLRCHN
        lda #7
        jsr KCLOSE
        lda tmp0
        cmp #$30
        bne _t5f
        lda tmp1
        cmp #$30
        bne _t5f
        inc PASSCNT
        jsr p_pass
        jmp t6
_t5f    jsr p_fail

; -------------------------------------------------------
; T6: Directory listing $
; -------------------------------------------------------
t6      lda #6
        sta TESTNUM
        lda #8
        ldx #8
        ldy #0
        jsr SETLFS
        lda #1
        ldx #<n_dir
        ldy #>n_dir
        jsr SETNAM
        jsr KOPEN
        bcs _t6f
        ldx #8
        jsr CHKIN
        jsr CHRIN
        sta tmp0
        jsr CHRIN
        sta tmp1
        jsr CLRCHN
        lda #8
        jsr KCLOSE
        lda tmp0
        cmp #$01
        bne _t6f
        lda tmp1
        cmp #$08
        bne _t6f
        inc PASSCNT
        jsr p_pass
        jmp t7
_t6f    jsr p_fail

; -------------------------------------------------------
; T7: Error on missing file
; -------------------------------------------------------
t7      lda #7
        sta TESTNUM
        lda #9
        ldx #8
        ldy #0
        jsr SETLFS
        lda #10
        ldx #<n_nofile
        ldy #>n_nofile
        jsr SETNAM
        jsr KOPEN
        bcc _t7f            ; no error = fail
        ; read command channel - should show 62
        lda #10
        ldx #8
        ldy #15
        jsr SETLFS
        lda #0
        ldx #0
        ldy #0
        jsr SETNAM
        jsr KOPEN
        ldx #10
        jsr CHKIN
        jsr CHRIN
        sta tmp0
        jsr CHRIN
        sta tmp1
        jsr CLRCHN
        lda #10
        jsr KCLOSE
        lda tmp0
        cmp #$36            ; '6'
        bne _t7f
        lda tmp1
        cmp #$32            ; '2'
        bne _t7f
        inc PASSCNT
        jsr p_pass
        jmp t_summ
_t7f    jsr p_fail

; -------------------------------------------------------
; Summary
; -------------------------------------------------------
t_summ
        lda #$0d
        jsr CHROUT
        ldx #0
_sl     lda m_summ,x
        beq _sd
        jsr CHROUT
        inx
        bne _sl
_sd     lda PASSCNT
        ora #$30
        jsr CHROUT
        lda #$2f
        jsr CHROUT          ; /
        lda #$37
        jsr CHROUT          ; 7
        lda #$0d
        jsr CHROUT
        ; Exit with code 0 if all 7 passed, 1 otherwise
        lda PASSCNT
        cmp #7
        beq _exit_ok
        jsr $ff01           ; synthetic exit(1) - failure
_exit_ok
        jsr $ff00           ; synthetic exit(0) - success
        rts

; -------------------------------------------------------
; Helpers
; -------------------------------------------------------
p_pass  jsr pnum
        ldx #0
_pp     lda m_pass,x
        beq _pe
        jsr CHROUT
        inx
        bne _pp
_pe     rts

p_fail  jsr pnum
        ldx #0
_pf     lda m_fail,x
        beq _pfe
        jsr CHROUT
        inx
        bne _pf
_pfe    rts

pnum    ldx #0
_pn     lda m_test,x
        beq _pnd
        jsr CHROUT
        inx
        bne _pn
_pnd    lda TESTNUM
        ora #$30
        jsr CHROUT
        lda #$3a
        jsr CHROUT          ; :
        lda #$20
        jsr CHROUT          ; space
        rts

; -------------------------------------------------------
; Data
; -------------------------------------------------------
m_test  .text "TEST "
        .byte 0
m_pass  .text "PASS"
        .byte $0d, 0
m_fail  .text "FAIL"
        .byte $0d, 0
m_summ  .text "PASSED "
        .byte 0

n_ui        .text "ui"
n_tfile     .text "testfile"
n_scr       .text "s:testfile"
n_scr_e
n_rnsrc     .text "rnsrc"
n_rncmd     .text "r:rndest=rnsrc"
n_rncmd_e
n_cpcmd     .text "c:cpdest=rndest"
n_cpcmd_e
n_dir       .text "$"
n_nofile    .text "nosuchfile"

tmp0        .byte 0
tmp1        .byte 0
rlen        .byte 0
sbuf        .fill 64, 0
rbuf        .fill 16, 0
