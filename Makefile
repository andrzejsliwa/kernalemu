CFLAGS=-Wall -Werror -g -std=c11 -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE
SDIR=src
ODIR=build
EXECUTABLE=$(ODIR)/kernalemu
PREFIX?=/usr/local
TESTDIR?=/tmp/ke_test
DRIVE_TESTS?=./examples/drive-tests

_OBJS = console.o cbmdos.o screen.o memory.o time.o ieee488.o channelio.o io.o keyboard.o printer.o vector.o c128.o main.o dispatch.o fake6502.o

_HEADERS = c128.h cbmdos.h channelio.h console.h dispatch.h error.h fake6502.h glue.h ieee488.h io.h keyboard.h memory.h printer.h readdir.h screen.h stat.h time.h vector.h

OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))
HEADERS = $(patsubst %,$(SDIR)/%,$(_HEADERS))

.PHONY: all clean install uninstall test test-asm test-basic help

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJS) $(HEADERS)
	$(CC) -o $(EXECUTABLE) $(OBJS) $(LDFLAGS)

$(ODIR)/%.o: $(SDIR)/%.c
	@mkdir -p $$(dirname $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(ODIR)

install: $(EXECUTABLE)
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(EXECUTABLE) $(DESTDIR)$(PREFIX)/bin/kernalemu

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/kernalemu

test: test-asm test-basic

test-asm: $(EXECUTABLE)
	@mkdir -p $(TESTDIR) && rm -f $(TESTDIR)/*
	@cd $(DRIVE_TESTS) && \
	  64tass --cbm-prg -o test_drive_asm.prg test_drive_asm.asm 2>/dev/null && \
	  $(abspath $(EXECUTABLE)) test_drive_asm.prg -drive8 $(TESTDIR) && \
	  rm -f $(TESTDIR)/*

$(DRIVE_TESTS)/bas_to_prg: $(DRIVE_TESTS)/bas_to_prg.c
	$(CC) -Wall -o $@ $<

test-basic: $(EXECUTABLE) $(DRIVE_TESTS)/bas_to_prg
	@mkdir -p $(TESTDIR) && rm -f $(TESTDIR)/*
	@cd $(DRIVE_TESTS) && \
	  ./bas_to_prg < test_drive.bas > test_drive.prg && \
	  $(abspath $(EXECUTABLE)) $(abspath demo/basic2.prg) test_drive.prg \
	    -drive8 $(TESTDIR) -startind 0xa000 -autorun && \
	  rm -f $(TESTDIR)/*

help:
	@echo "kernalemu - Commodore KERNAL emulator"
	@echo ""
	@echo "Targets:"
	@echo "  all          Build the emulator (default)"
	@echo "  clean        Remove build artifacts"
	@echo "  install      Install to PREFIX (default: /usr/local)"
	@echo "  uninstall    Remove installed binary"
	@echo "  test         Run assembly and BASIC test suites"
	@echo "  test-asm     Run assembly test suite only"
	@echo "  test-basic   Run BASIC test suite only"
	@echo "  help         Show this message"
	@echo ""
	@echo "Variables:"
	@echo "  PREFIX       Install prefix      (default: /usr/local)"
	@echo "  TESTDIR      Temp dir for tests  (default: /tmp/ke_test)"
	@echo "  DRIVE_TESTS  Test suite dir      (default: examples/drive-tests)"
	@echo ""
	@echo "Usage:"
	@echo "  kernalemu [options] file..."
	@echo ""
	@echo "Options:"
	@echo "  -start <addr>          Entry point address"
	@echo "  -startind <addr>       Address of entry point vector"
	@echo "  -machine <m>           KERNAL level: pet|pet4|vic20|c64|ted|c128|c65"
	@echo "  -basic                 C64 BASIC shortcut: implies -startind 0xa000 -autorun"
	@echo "  -autorun               Inject RUN after BASIC cold-start"
	@echo "  -drive8..15 <path>     Map drive unit to local directory"
	@echo "  -text                  Map PETSCII upper/lower to ASCII"
	@echo "  -graphics              No PETSCII mapping (default)"
	@echo "  -columns <n>           Hard line break after n columns"
	@echo "  -continue              Tolerant mode: log unknown KERNAL calls instead of exit"
