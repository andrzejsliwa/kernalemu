# Makefile for kernalemu 1541 test suite
# Requires: 64tass, cc, kernalemu in PATH or ../kernalemu/build/kernalemu

KERNALEMU ?= ../../kernalemu/build/kernalemu
TESTDIR   ?= /tmp/ke_test

.PHONY: all asm basic run-asm run-basic run clean

all: test_drive_asm.prg test_drive.prg

bas_to_prg: bas_to_prg.c
	$(CC) -Wall -o $@ $<

test_drive_asm.prg: test_drive_asm.asm
	64tass --cbm-prg -o $@ $<

test_drive.prg: test_drive.bas bas_to_prg
	./bas_to_prg < $< > $@

run-asm: test_drive_asm.prg
	@mkdir -p $(TESTDIR)
	$(KERNALEMU) $< -drive8 $(TESTDIR)

run-basic: test_drive.prg
	@mkdir -p $(TESTDIR)
	@rm -f $(TESTDIR)/*
	$(KERNALEMU) ../kernalemu/demo/basic2.prg test_drive.prg -drive8 $(TESTDIR) -startind 0xa000 -autorun

run: run-asm run-basic

clean:
	rm -f test_drive_asm.prg test_drive.prg bas_to_prg
