//! @file io.h
//! New printf style functions and aliases.

#pragma once
#include "common.h"

#define PANIC(lit, ...) \
	panic("\n[**] Panicking!\n[**] CAUSE:\n -- \t%s(): " \
	      lit "\n[**] Aborting...\n", __func__, ## __VA_ARGS__)

#define print(...) novel_fprintf(stdout, __VA_ARGS__)
#define sprint novel_sprintf
#define println(...) novel_fprintf_newline(stdout, __VA_ARGS__)
#define eprintf(...) novel_fprintf(stderr, __VA_ARGS__)
#define eprint(...) eprintf(__VA_ARGS__)
#define eprintln(...) novel_fprintf_newline(stderr, __VA_ARGS__)
#define fprint novel_fprintf

/// `fputs(...)` with `string`.
extern ierr fput(string, FILE *);
/// Same as `fput(..., stdout)`.
extern ierr put(string);
/// Same as `fput(..., stderr)`.
extern ierr eput(string);
/// puts(...) to STDERR.
extern ierr eputs(const byte *);

/// Custom `printf` for other data-types.
/// @note Heap allocates memory, should be freed after printing.
extern string novel_vsprintf(const byte *, va_list);
/// @note Returns heap-allocated memory, should be freed.
extern string novel_sprintf(const byte *, ...);
extern ierr novel_vfprintf(FILE *, const byte *, va_list);
extern ierr novel_fprintf(FILE *, const byte *, ...);
extern ierr novel_vfprintf_newline(FILE *, const byte *, va_list);
extern ierr novel_fprintf_newline(FILE *, const byte *, ...);
extern ierr novel_printf(const byte *, ...);

/// Size of the type of a `printf`-style format specifer.
/// e.g. `sizeof_specifier("hx") == sizeof(unsigned short int);`.
extern usize sizeof_specifier(const byte *);

