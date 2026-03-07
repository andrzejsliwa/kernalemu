# Changelog

## [Unreleased]

### Added

- **Drive support**: `-drive8` through `-drive15` options map CBM drive units to local filesystem directories (up to 8 simultaneous drives)
- **`-autorun` flag**: Injects `RUN` into the keyboard buffer after BASIC cold-start and restores program memory zeroed by the `NEW` routine
- **Expanded CBM DOS emulation**: Full 1541-style command channel support — `S:` (scratch/delete), `R:` (rename), `C:` (copy), `V` (validate), `CD:` (change directory), `UI`/`UJ` (reset)
- **Wildcard matching**: Directory listing and file operations support CBM-style wildcards via `fnmatch`
- **Synthetic KERNAL exit calls**: `JSR $FF00` (exit 0 / pass) and `JSR $FF01` (exit 1 / fail) for portable test program termination; from BASIC: `SYS 65280` / `SYS 65281`
- **TTY detection**: ANSI escape sequences (cursor movement, screen clear) are suppressed when stdout is not a terminal, making output safe to redirect or pipe
- **Makefile targets**: `install`, `uninstall`, `test`, `test-asm`, `test-basic`, `help`; configurable via `PREFIX`, `TESTDIR`, `DRIVE_TESTS` variables
- **C11 build flags**: `CFLAGS` now includes `-std=c11 -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE` for standards-compliant compilation
- **Test suite**: Assembly and BASIC drive test suites in `examples/drive-tests/` (requires `64tass` and `python3`); all 14 tests pass
- **`examples/` directory**: Organised example ASM programs, BASIC programs, and drive tests

### Changed

- **`cbmdos`**: Rewritten to support up to 8 drive units; file table expanded from 16 to 256 entries; filename buffers expanded to 256 bytes; `PATH_MAX` used throughout instead of hardcoded sizes
- **`channelio`**: Drive and printer device handling consolidated into helper functions; keyboard injection API added (`channelio_inject`, `channelio_inject_patch`, `channelio_inject_patch2`)
- **`glue.h`**: Now the single source for `KERN_DEVICE_*` constants, `NYI()` macro, and `is_drive_device()` / `is_printer_device()` helpers
- **`error.h`**: Error codes and status bits converted from `#define` to typed enums (`kern_err_t`, `kern_status_t`) for type safety
- **All headers**: Added include guards; all function declarations updated from `()` to `(void)` for C11 conformance
- **`settimeofday`**: Failure is now handled gracefully instead of crashing (requires root on some systems)
- **`simonsbasic.prg`**: File permission corrected from executable (755) to regular (644)

### Removed

- Duplicate `#define NYI()` macros across `c128.c`, `ieee488.c`, `keyboard.c`, `vector.c` (consolidated into `glue.h`)
- Dead `#if 0` debug blocks and commented-out `printf` traces in `channelio.c`
