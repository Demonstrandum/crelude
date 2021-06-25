//! @file common.h
//! Defines basic macros and datatypes which are in
//! common through-out the whole project.
//! @note Read this:
//!    => https://www.cprogramming.com/tutorial/unicode.html
//!    by Jeff Bezanson, about modern unicode in C.

#ifndef COMMON_HEADER
#define COMMON_HEADER

#undef  _GNU_SOURCE
#define _GNU_SOURCE 1 ///< Use GNU specific source.

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <limits.h>
#include <string.h>
#include <assert.h>
#ifdef __linux__
	#include <unistd.h>
#endif

/* Misc macros */
#define TSTR_HELPER(x) #x
#define TSTR(x) TSTR_HELPER(x)

/* Version number */
#define crelude_V_MAJOR 0
#define crelude_V_MINOR 1
#define crelude_V_PATCH 0

#define crelude_VERSION \
	"v" TSTR(crelude_V_MAJOR) \
	"." TSTR(crelude_V_MINOR) \
	"." TSTR(crelude_V_PATCH)

#define REALLOC_FACTOR 1.5

/* Syntax helpers */
#define loop while (1)
#define unless(cond) if (!(cond))
#define until(cond) while (!(cond))
#define newtype(NT, T) typedef struct _##NT { T value; } NT
#define arrayof(T) struct { \
	usize len;  \
	usize cap;  \
	T (*value); \
}
#define newarray(NT, T) typedef arrayof(T) NT
#define sliceof(T) struct { \
	usize len;  \
	T (*value); \
}
#define newslice(NT, T) typedef sliceof(T) NT
#define hashof(T) struct { \
	u64 hash; \
	T value;  \
}
#define newhashable(NT, T) typedef hashof(T) NT
#define unqualify(D, T) typedef D T T
#define nil NULL

#define UNUSED(x) (void)(x)
#define NO_ERROR EXIT_SUCCESS;
#define OK EXIT_SUCCESS;

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

/* Types */
/// Useful for resource counting etc.
newtype(atomic_t, int);

/// The type that occupies no space.
/// Thanks to Terry for this one.
typedef void u0;
#define UNIT ;

/// Explicitly mark functions that return error-codes
/// as returning `ierr` instead of just `int`.
typedef int ierr;

#define __UCHAR8__ char

#if (CHAR_BIT == 8)
	typedef   signed char i8;
	typedef unsigned char u8;

	#if (CHAR_MIN < 0)
		#undef  __UCHAR8__
		#define __UCHAR8__ unsigned char
	#endif
#else
	typedef  __int8_t i8;
	typedef __uint8_t u8;

	#undef  __UCHAR8__
	#define __UCHAR8__ __uint8_t;
#endif

#ifndef IMPLEMENTATION
	typedef __UCHAR8__ byte;
#else
	typedef __uint8_t byte;  ///< Don't use `char' when you want `byte'.
#endif

/// Such that `sizeof(umin) == 1`.
typedef unsigned char umin;
/// Such that `sizeof(imin) == 1`.
typedef   signed char imin;

typedef  __int16_t i16;
typedef __uint16_t u16;

typedef  __int32_t i32;
typedef __uint32_t u32;

/// Unicode codepoint (32 bits), don't use `char[4]',
/// and definitely do not use `wchar_t'.
typedef u32 rune;

#if (__LONG_WIDTH__ == 64)
	typedef   signed long i64;
	typedef unsigned long u64;
#elif (__LONG_LONG_WIDTH__ == 64)
	typedef   signed long long i64;
	typedef unsigned long long u64;
#else
	typedef  __int64_t i64;
	typedef __uint64_t u64;
#endif

#ifdef __SIZEOF_INT128__
	typedef  __int128_t i128;
	typedef __uint128_t u128;
#endif

typedef ptrdiff_t isize;
typedef    size_t usize; ///< Use for storing array indices or object sizes.

typedef  intptr_t iptr;
typedef uintptr_t uptr; ///< Large enough to store a pointer, like (void *).

typedef  intmax_t imax;
typedef uintmax_t umax;

#ifdef __STDC_IEC_559__
	typedef  float f32;
	typedef double f64;
#endif

#define _LDOUBLE_BIT (__SIZEOF_LONG_DOUBLE__ * CHAR_BIT)

#if (_LDOUBLE_BIT == 80)
	typedef long double f80;
#elif (_LDOUBLE_BIT == 128)
	typedef long double f128;
#endif

/// Array with pointer to void.
newarray(GenericArray, u0);
/// Array with pointer type to smallest addressable units of memory.
newarray(MemArray, umin);
/// Slice with pointer to void.
newslice(GenericSlice, u0);
/// Slice with pointer type to smallest addressable units of memory.
newslice(MemSlice, umin);

/// Immutable wrapper for UTF-8 encoded string (bytes are mutable).
newslice(string, byte);
/// Imutable warpper for UCS-4/UTF-32 encoded runic string (runes are mutable).
newslice(runic, rune);

/// Mutable string which is built/pushed-to over time.
newarray(StringBuilder, byte);
/// Mutable runic string which is built/pushed-to over time.
newarray(RunicBuilder, rune);

/// Symbols are interned strings.
newhashable(symbol, string);

/* Common Constants */
static const ierr NUL = 0;
static const byte NUL_BYTE = '\0';
static const string NUL_STRING = { .len = 0, .value = (byte *)&NUL_BYTE };

/* Common Functions */
extern u0 panic(const byte *, ...) __attribute__((noreturn));
extern bool is_zero(imax);
extern bool is_zerof(f64);
extern bool is_zeroed(u0 *, usize);
/// Zero a block of memory.
/// @param[out] blk Pointer to start of block.
/// @param[in] width How many bytes to zero.
/// e.g., for an array `width = lenght * sizeof(elem)`.
extern u0 zero(u0 *blk, usize width);
extern u0 *emalloc(usize, usize);
/// Push element to array.
/// @param[in,out] self Pointer to the dynamic array, cast to (u0 *).
/// @param[in] element Pointer to element to be pushed,  cast to (u0 *).
/// @param[in] width The `sizeof(T)` where `T` is the type of the element
///                  that is being pushed.
/// @returns How much capacity increased.
extern usize push(u0 *self, const u0 *element, usize width);
/// Exactly like `push`, except position of element is arbitrary,
/// with index specified in second argument.
/// @returns How much capacity increased.
extern usize insert(u0 *self, usize index, const u0 *element, usize width);
/// Works like push, but extends the array by multiple elements.
/// @param[in,out] self A pointer to a dynamic array, of any type.
/// @param[in] slice A pointer to a slice, and a slice only
///                  (*not* an array, dynamic array, etc.).
/// @param[in] width The `sizeof(T)` where `T` is the type of the individual
///                  elements that are being appended to the array.
extern usize extend(u0 *self, const u0 *slice, usize width);
/// Pops/removes element from top of the stack (dynamic array).
/// @returns Pointer to popped element.
extern u0 *pop(u0 *self, usize width);
/// `fputs(...)` with `string`.
extern ierr fput(string, FILE *);
/// Same as `fput(..., stdout)`.
extern ierr put(string);
/// Same as `fput(..., stderr)`.
extern ierr eput(string);
/// puts(...) to STDERR.
extern ierr eputs(const byte *);
/// Size of the type of a `printf`-style format specifer.
/// e.g. `sizeof_specifier("hx") == sizeof(unsigned short int);`.
extern usize sizeof_specifier(const byte *);
/// Custom `printf` for other data-types.
/// @note Heap allocates memory, should be freed after printing.
extern string novel_vsprintf(const byte *, va_list);
extern string novel_sprintf(const byte *, ...);
extern ierr novel_vfprintf(FILE *, const byte *, va_list);
extern ierr novel_fprintf(FILE *, const byte *, ...);
extern ierr novel_vfprintf_newline(FILE *, const byte *, va_list);
extern ierr novel_fprintf_newline(FILE *, const byte *, ...);
extern ierr novel_printf(const byte *, ...);
/// Compare two strings for equality.
extern bool string_eq(const string, const string);
extern i16 string_cmp(const string, const string);
/// Hash a string.
extern u64 hash_string(const string);

/* Common Macros */

// ANSI colour code.
#define ANSI(CODE) "\x1b[" CODE "m"
#define BOLD   "1"
#define FAINT  "2"
#define DIM FAINT
#define ITALIC "3"
#define UNDER  "4"
#define BLINK  "5"
#define RAPID  "6"
#define INVERT "7"
#define HIDDEN "8"
#define STRIKE "9"
#define BOLD_OFF   "21"  // Or sometimes, double-underline...
#define FAINT_OFF  "22"
#define ITALIC_OFF "23"
#define UNDER_OFF  "24"
#define BLINK_OFF  "25"
#define RAPID_OFF  "26"
#define INVERT_OFF "27"
#define HIDDEN_OFF "28"
#define STRIKE_OFF "29"
#define RESET "0"

#define MIN(A, B) (((A) > (B)) ? (B) : (A))
#define MAX(A, B) (((A) > (B)) ? (A) : (B))

/// Unwraps pointer/value in sizing wrapper struct.
#define UNWRAP(STRUCTURE) (STRUCTURE).value
/// Initialise sizing wrapper with literal.
#define INIT(TYPE, ...) { \
	.len = sizeof((TYPE[])__VA_ARGS__)/sizeof(TYPE), \
	.value = (TYPE[])__VA_ARGS__ \
}
/// Initialise sizing wrapper with of string literal.
#define STRING(...) { \
	.len = sizeof((byte[]){ __VA_ARGS__ }) - 1, \
	.value = (byte[]){ __VA_ARGS__ } \
}

#define STR(...) ((string)STRING(__VA_ARGS__))

/// Empty slice of certain type.
#define SEMPTY(TYPE) ((TYPE){ .len = 0, .value = nil })
/// Empty array of certain type.
#define AEMPTY(TYPE) ((TYPE){ .len = 0, .cap = 0, .value = nil })

/// Is array empty?
#define IS_EMPTY(ARR) ((ARR).len == 0)

/// Heap allocates a variable sized array.
#define AMAKE(TYPE, CAP) { \
	.len = 0, \
	.cap = (CAP), \
	.value = emalloc((CAP), sizeof(TYPE)) \
}

/// Heap allocates a constant sized slice type.
#define SMAKE(TYPE, LEN) { \
	.len = (LEN), \
	.value = emalloc((LEN), sizeof(TYPE)) \
}

/// Take a slice/substring/view of sized type.
#define SLICE(TYPE, OBJ, START, END) ((TYPE){ \
	.len = (((isize)(END) < 0) ? (OBJ).len + 1 : 0) + (END) - (START), \
	.value = (OBJ).value + (START) \
})

/// Works like `SLICE`, but on a pointer instead of an array.
#define VIEW(TYPE, PTR, START, END) ((TYPE){ \
	.len = (END) - (START), \
	.value = (PTR) + (START) \
})

#define SYMBOLIC(str) ((symbol){ \
	.hash = hash_string(str), \
	.value = str \
})

#define SYMBOL_LITERAL(STR_LIT) ((symbol){ \
	.hash = hash_string(STRING(STR_LIT)), \
	.value = STRING(STR_LIT) \
})

/// C array to dynamic array wrapper.
#define ACOLLECT(T, count, pointer) ((T){ \
	.len = count,     \
	.cap = count,     \
	.value = pointer, \
})

/// C array to slice wrapper.
#define SCOLLECT(T, count, pointer) ((T){ \
	.len = count,     \
	.value = pointer, \
})

#define SMAP(T, func, list) ({ \
	T _mapped; \
	_mapped = ((T)SMAKE(typeof(*_mapped.value), (list).len)); \
	for (usize _i = 0; _i < (list).len; ++_i) \
		_mapped.value[_i] = (func)((list).value[_i]); \
	_mapped; \
})

#define AMAP(T, func, list) ({ \
	T _mapped; \
	_mapped = ((T)AMAKE(typeof(*_mapped.value), (list).len)); \
	for (usize _i = 0; _i < (list).len; ++_i, ++_mapped.len) \
		_mapped.value[_i] = (func)((list).value[_i]); \
	_mapped; \
})

/// For-each loop, iterates across an array or slice.
/// For example:
/// ```c
/// newarray(IntArray, int);
/// IntArray xs = AMAKE(IntArray, 5);
///
/// int elem1 = 5;
/// int elem2 = 3;
/// push(&xs, &elem1, sizeof(int));
/// push(&xs, &elem2, sizeof(int));
///
/// FOR_EACH(x, xs) {
///     printf("%d\n", *x);
/// }
/// ```
/// Will print `5` then `3`.
#define FOR_EACH(ELEM, ELEMS) \
	for (typeof((ELEMS).value) ELEM = (ELEMS).value, _frst = ELEM; \
		(usize)(ELEM - _frst) < (ELEMS).len; \
		++ELEM)

#define foreach FOR_EACH

#endif
