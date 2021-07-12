//! @file common.h
//! Defines basic macros and datatypes which are in
//! common through-out the whole project.
//! @note Read this:
//!    [unicode](https://www.cprogramming.com/tutorial/unicode.html)
//!    by Jeff Bezanson, about modern unicode in C.

#ifndef COMMON_HEADER_
#define COMMON_HEADER_

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

/* Default macros */
#ifndef FREE
	#define FREE free
#endif
#ifndef MALLOC
	#define MALLOC malloc
#endif
#ifndef REALLOC
	#define REALLOC realloc
#endif

/* Misc macros */
#define VA_NUM_ARGS_IMPL(_1,_2,_3,_4,_5, N,...) N
#define VA_NUM_ARGS(...) VA_NUM_ARGS_IMPL(__VA_ARGS__, 5, 4, 3, 2, 1)

#define _TSTR(x) #x
#define TSTR(x) _TSTR(x)

#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
	#define DO_PRAGMA(x) __pragma(x)
	#define PRAGMA_NO_WARNING __pragma(warning(push, 0))
	#define PRAGMA_POP_WARNING __pragma(warning(pop))
	#define WARNING(...) __pragma(message(Warning: __VA_ARGS__))
#elif defined(__clang__)
	#define DO_PRAGMA(x) _Pragma(#x)
	#define PRAGMA_NO_WARNING \
		_Pragma("clang diagnostic push") \
		_Pragma("clang diagnostic ignored \"-Wall\"") \
		_Pragma("clang diagnostic ignored \"-Wextra\"") \
		_Pragma("clang diagnostic ignored \"-Wpedantic\"")
	#define PRAGMA_POP_WARNING _Pragma("clang diagnostic pop")
	#define WARNING(S) DO_PRAGMA(clang warning S)
#elif defined(__GNUC__)
	#define DO_PRAGMA(x) _Pragma(#x)
	// Does not behave as nicely as CLANG version does.
	#define PRAGMA_NO_WARNING \
		_Pragma("GCC diagnostic push") \
		_Pragma("GCC diagnostic ignored \"-Wall\"") \
		_Pragma("GCC diagnostic ignored \"-Wextra\"") \
		_Pragma("GCC diagnostic ignored \"-Wpedantic\"")
	#define PRAGMA_POP_WARNING _Pragma("GCC diagnostic pop")
	#define WARNING(S) DO_PRAGMA(GCC warning S)
#endif

#define TODO(...) DO_PRAGMA(message("TODO: " #__VA_ARGS__ \
                            " (" __FILE__ ":" TSTR(__LINE__)")"))

/* Version number */
#define crelude_V_MAJOR 0
#define crelude_V_MINOR 1
#define crelude_V_PATCH 0

#define crelude_VERSION \
	"v" TSTR(crelude_V_MAJOR) \
	"." TSTR(crelude_V_MINOR) \
	"." TSTR(crelude_V_PATCH)

#define ARRAY_REALLOC_FACTOR 1.5

/* Syntax helpers */
#define loop while (1)
#define whilst while
#define unless(cond) if (!(cond))
#define never if (0)
#define always if (1)
#define until(cond) while (!(cond))
// Newtypes and arrays, slices, maps, etc.
#define unqualify(D, T)  typedef D T T
#define record(NAME)     typedef struct _##NAME NAME; struct _##NAME
#define enumerable(NAME) typedef   enum _##NAME NAME;   enum _##NAME
#define overlap(NAME)    typedef  union _##NAME NAME;  union _##NAME
#define newtype(NT, T) typedef struct _##NT { T value; } NT
#define arrayof(T) struct { \
	T (*value); \
	usize len;  \
	usize cap;  \
}
#define newarray(NT, T) typedef arrayof(T) NT
#define sliceof(T) struct { \
	T (*value); \
	usize len;  \
}
#define newslice(NT, T) typedef sliceof(T) NT
#define hashof(T) struct { \
	u64 hash; \
	T value;  \
}
#define newhashable(NT, T) typedef hashof(T) NT
enum HashKeyType {  // Use this to pick a default hash method/function.
	HKT_STRING  = 1 << 0,
	HKT_RUNIC   = 1 << 1,
	HKT_CSTRING = 1 << 2,
	HKT_MEM_SLICE = 1 << 3, //< can be used for any slice, convert with TO_BYTES(...).
	HKT_RAW_BYTES = 1 << 4  //< just hash the raw bytes.
}; unqualify(enum, HashKeyType);
#define HASHMAP_LOAD_THRESHOLD 0.85
#define HASHMAP_GROWTH_FACTOR  2
#define newmap(NT, K, V) typedef mapof(K, V) NT
#define mapof(K, V) struct { \
	usize len; \
	arrayof(hashnode(K, V)) buckets; /* a hash value of zero indicates absence. */\
	usize value_size; \
	usize   key_size; \
	usize  node_size; \
	/* offsets of `hashnode` struct. */ \
	usize  hash_offset; \
	usize   key_offset; \
	usize value_offset; \
	usize  next_offset; \
	HashKeyType key_type; /* < how should the hash-function hash the key. */ \
	u64 (*hasher)(const u0 *, usize); \
}
#define hashnode(K, V) struct { \
	hashof(K) key; \
	V value;  /* < value stored.   */ \
	u0 *next; /* < next hash-node. */ \
}
#define MMAKE(K, V, CAP) { \
	.buckets = AMAKE(hashnode(K, V), CAP), \
	.key_size = sizeof(K), \
	.value_size = sizeof(V), \
	.node_size = sizeof(hashnode(K, V)), \
	.hash_offset  = offsetof(hashnode(K, V), key) + offsetof(hashof(K), hash), \
	.key_offset   = offsetof(hashnode(K, V), key) + offsetof(hashof(K), value), \
	.value_offset = offsetof(hashnode(K, V), value), \
	.next_offset  = offsetof(hashnode(K, V), next), \
	.key_type = _Generic(*(K *)NULL, \
		string: HKT_STRING, \
		runic: HKT_RUNIC, \
		byte *: HKT_CSTRING, \
		MemSlice: HKT_MEM_SLICE, \
		default: HKT_RAW_BYTES), \
	.hasher = _Generic(*(K *)NULL, \
		string: string_hash, \
		runic: runic_hash, \
		byte *: cstring_hash, \
		MemSlice: mem_hash, \
		default: default_hash) \
}

#define NTH(LIST, N) UNWRAP((LIST))[(N)]
#define GET(LIST, N) __extension__\
	({ __auto_type _list = (LIST); \
	   __auto_type _n = (N); \
	   usize _index = _n < 0 ? (usize)(_list.len + _n) : (usize)_n; \
	   UNWRAP(_list)[_index]; })
#define SET(LIST, N, V) __extension__\
	({ __auto_type _list = (LIST); \
	   __auto_type _n = (N); \
	   usize _index = _n < 0 ? (usize)(_list.len + _n) : (usize)_n; \
	   UNWRAP(_list)[_index] = (V); })

#define UNUSED1(z) (void)(z)
#define UNUSED2(y,z) UNUSED1(y),UNUSED1(z)
#define UNUSED3(x,y,z) UNUSED1(x),UNUSED2(y,z)
#define UNUSED4(b,x,y,z) UNUSED2(b,x),UNUSED2(y,z)
#define UNUSED5(a,b,x,y,z) UNUSED2(a,b),UNUSED3(x,y,z)
#define UNUSED_IMPL_(nargs) UNUSED ## nargs
#define UNUSED_IMPL(nargs) UNUSED_IMPL_(nargs)
#define UNUSED(...) UNUSED_IMPL(VA_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__)

#define NO_ERROR EXIT_SUCCESS
#define OK EXIT_SUCCESS

#define NOOP ((void)0)
#define nil NULL

#define FLOOR(T, N) __extension__\
	({ typeof(N) _n = (N); \
	   (T)_n - (_n < 0 ? 1 : 0); })
#define CEIL(T, N) __extension__\
	({ typeof(N) _n = (N); \
	   (T)_n + (_n < 0 ? 0 : 1); })

/* Types */
/// Useful for resource counting etc.
newtype(atomic_t, int);

/// The type that occupies no space.
/// Thanks to Terry for this one.
typedef void u0;
#define UNIT ;

/// Explicitly mark functions that return error-codes
/// as returning `ierr` instead of just `int`.
typedef   signed int ierr;
typedef unsigned int uerr;  ///< Not something very common.
/// `long` is always the same size as a machine word.
typedef unsigned long uword;  ///< Unsigned machine word integer.
typedef   signed long iword;  ///<   Signed machine word integer.
/// Size of a machine word in bytes.
#define WORD_SIZE sizeof(long)

/// `int` in most cases is going to have the natural size
/// suggested by the target architecture, optimal for most things.
typedef unsigned int ufast;
typedef   signed int ifast;

typedef ptrdiff_t isize;
typedef    size_t usize; ///< Use for storing array indices or object sizes.

typedef  intptr_t iptr;
typedef uintptr_t uptr; ///< Large enough to store a pointer, like (void *).

typedef  intmax_t imax;
typedef uintmax_t umax;

/// Such that `sizeof(umin) == 1`.
typedef unsigned char umin;
/// Such that `sizeof(imin) == 1`.
typedef   signed char imin;

_Static_assert(sizeof(umin) == 1 && sizeof(imin) == 1,
               "`umin` and `imin` must have size one (1).");

#define __UCHAR8__ char
#if (CHAR_BIT == 8)
	typedef   signed char i8;
	typedef unsigned char u8;

	#if (CHAR_MIN < 0)
		WARNING("Crelude works best with -funsigned-char.")
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
	typedef __uint8_t byte;  ///< Don't use `char` when you want `byte`.
#endif

typedef  __int16_t i16;
typedef __uint16_t u16;

typedef  __int32_t i32;
typedef __uint32_t u32;

/// Unicode codepoint (USC-4) (32 bits),
/// don't use `char[4]`, and definitely do not use `wchar_t`.
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
/// Hash-table that maps `void *` to `void *`.
newmap(GenericMap, u0 *, u0 *);

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
static const iword ZERO = 0;

/* Common Functions */
extern u0 panic(const byte *, ...) __attribute__((noreturn));
extern u0 *or(const u0 *nullable, const u0 *nonnull);
extern bool is_zero(imax);
extern bool is_zerof(f64);
extern bool is_zeroed(u0 *, usize);
/// Zero a block of memory.
/// @param[out] blk Pointer to start of block.
/// @param[in] width How many bytes to zero.
/// e.g., for an array `width = lenght * sizeof(elem)`.
extern u0 zero(u0 *blk, usize width);
/// Malloc with zeros, and panics when out of memory.
extern u0 *emalloc(usize, usize);
/// Reverse an array or slice in-place.
/// @param[in,out] self A pointer to an array or slice, cast to `u0 *`.
/// @param[in] width The width/`sizeof` of an element in the array.
extern u0 reverse(u0 *self, usize width);
/// Reverse a slice of bytes. Not in-place.
/// @note Heap allocates, remember to free.
extern MemSlice reverse_endianness(MemSlice bytes);
/// Check if system/CPU is using little endian.
/// @returns true if little endian, false if big endian.
bool is_little_endian(void);
/// Read big-endian integer.
u128 big_endian(umin *start, usize bytes);
/// Given a slice, swap the two blocks within the slice
/// formed by selecting a pivot point.
/// ```
/// [----A----|---B---] -> [---B---|----A----]
///           ^ pivot
/// ```
/// @param[in,out] self Pointer to the slice, cast to (`u0 *`).
/// @param[in] pivot The index of the slice that divides the blocks to swap.
/// @param[in] width The `sizeof(T)` where `T` is
///                  the type of element in the slice.
extern u0 swap(u0 *self, usize pivot, usize width);
/// Resizes the array, i.e. changes the capacity to a given value.
/// Akin to `realloc`.
/// @returns How much of the array is empty (i.e. `cap - len`).
extern usize resize(u0 *self, usize cap, usize width);
/// Grows the array length by `count`, reallocates if necessary.
/// Does nothing else.
/// @param[in,out] self Void-pointer to array structure.
/// @param[in] count Number of spaces to grow by.
/// @param[in] width Size of the individual elements in the array, in bytes.
/// @returns How much capacity increased.
extern usize grow(u0 *self, usize count, usize width);
/// Get pointer to element at index in slice/array.
/// @param[in] self Pointer to slice or array.
/// @param[in] index Index of element you wish to retrieve.
/// @param[in] width Size of single array element in bytes.
/// @returns Pointer to element, or NULL if not present / out of bounds.
extern u0 *get(u0 *self, usize index, usize width);
/// Set element at index in slice/array.
/// @param[in] self Pointer to slice or array.
/// @param[in] index Index of element you wish to set.
/// @param[in] width Size of single array element in bytes.
/// @returns Pointer to element just set, or NULL if out of bounds.
extern u0 *set(u0 *self, usize index, const u0 *elem, usize width);
/// Push element to array.
/// @param[in,out] self Pointer to the dynamic array, cast to (`u0 *`).
/// @param[in] element Pointer to element to be pushed,  cast to (`u0 *`).
/// @param[in] width The `sizeof(T)` where `T` is the type of the element
///                  that is being pushed.
/// @returns How much capacity increased.
extern usize push(u0 *self, const u0 *element, usize width);
/// Pops/removes element from top of the stack (dynamic array).
/// @returns Pointer to popped element.
extern u0 *pop(u0 *self, usize width);
/// Works like `pop` but removes from the front.
extern u0 *shift(u0 *self, usize width);
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
/// @returns How much capacity increased.
extern usize extend(u0 *self, const u0 *slice, usize width);
/// Works like extend, but extends or *splices* the array with a slice
/// at some given, arbitrary position.
/// @param[in,out] self A pointer to the dynamic array, of any type.
/// @param[in] index The location for inserting in the slice.
/// @param[in] slice The slice you wish to insert at `index`.
/// @param[in] width The `sizeof(T)` where `T` is the type of the individual
///                  elements stored within the array and slice.
/// @returns How much capacity increased.
extern usize splice(u0 *self, usize index, const u0 *slice, usize width);
/// Deletes a range of elements from an array, starting from some index.
/// @param[in,out] self A pointer to the array.
/// @param[in] from Index to start removing from.
/// @param[in] upto Index of final element to remove in the range.
///                 If parameter is negative, it indicates an index from
///                 the end of the array.
/// @returns A slice holding a void-pointer to the removed elements,
//           along with the number of removed bytes (`len` of slice).
extern GenericSlice cut(u0 *self, usize from, isize upto, usize width);
/// Zero out an array.
/// @returns Number of bytes nulled.
extern usize null(u0 *self, usize width);
/// NUL-terminated string to library string.
extern string from_cstring(const byte *);
/// Compare two strings for equality.
extern bool string_eq(const string, const string);
/// Compare two strings for alphabetic rank.
extern i16 string_cmp(const string, const string);
/// Hash a string.
extern u64 hash_string(const string);
/// Hash a byte slice.
extern u64 hash_bytes(const MemSlice);
/// Map / associate a key with a value, i.e. insert into the hash-map/table.
extern u0 associate(u0 *self, const u0 *key, const u0 *value);
/// Look-up / get value from hash-map/table given the key.
extern u0 *lookup(u0 *self, const u0 *key);
/// Drop / delete / remove key-value pair from the hash-table.
/// This frees/deallocates the node, and the key and value are deleted.
/// Does nothing if key did not exist in the table.
/// @returns `true` if key was present, and node was deleted, `false` if not.
extern bool drop(u0 *self, const u0 *key);
/// Get slice of key pointers of all keys in map/hash-table.
/// @note Returns a heap allocated slice, remember to free.
extern GenericSlice get_keys(u0 *self);
/// Checks if entry / key-value pair is present in hash-table/map given a key.
extern bool has_key(u0 *self, u0 *key);
/// Empties out / deallocates all key-value pairs from the map.
/// Map may still be repopulated again after this.
extern u0 empty_map(u0 *self);
/// Checks if the hash-table / map is empty or freed.
extern bool is_empty_map(u0 *self);
/// Frees the map.  Not only empties it, but deallocates bucket array
/// such that the map may not be used again.
extern u0 free_map(u0 *self);
/// Internal use 99% of the time.
extern usize init_hashnode(u0 *, const u0 *, u64, const u0 *, const u0 *);
/// Hashmap debugging function.
extern u0 dump_hashmap(u0 *self, byte *key_formatter, byte *value_formatter);

/* Common Macros */

/// Copy an array or slice.
/// @note Allocates on the heap.
#define COPY(SELF) __extension__\
	({ __auto_type _self = (SELF); \
	   __auto_type _copy = _self; \
	   PTR(_copy) = emalloc(_self.len, sizeof(*PTR(_self))); \
	   memcpy(PTR(_copy), PTR(_self), sizeof(*PTR(_copy)) * _copy.len); \
	   _copy; })

/// Convert array/slice to slice of bytes.
#define TO_BYTES(SELF) __extension__\
	({ __auto_type _self = (SELF); \
	   MemSlice _bytes = { \
		  .len = _self.len * sizeof(*_self.value), \
		  .value = (umin *)_self.value \
	   }; _bytes; })

/// Convert from a byte array/slice into an array/slice of another type.
#define FROM_BYTES(T, SELF) __extension__\
	({ __auto_type _self = (SELF); \
	   T _normal = { \
		  .len = _self.len / sizeof(*_self.value), \
		  .value = (u0 *)_self.value \
	   }; _normal; })

// ---
// Macros for array functions to avoid use of `sizeof(T)` everywhere.
// These macros do the referencing and void-pointer casting for you, and thus
// let you use non-LVALUES as input (except SELF, SELF must sill be an LVALUE).
// ---

/// In-place reverse.
#define REVERSE(SELF) __extension__\
	({ __auto_type _self = &(SELF); \
	   reverse(_self, sizeof(*_self->value)); \
	   *_self; })

#define SWAP(SELF, PIVOT) __extension__\
	({ __auto_type _self = &(SELF); \
	   swap(_self, (PIVOT), sizeof(*_self->value)); })

#define PUSH(SELF, ELEM) __extension__\
	({ __auto_type           _self = &(SELF); \
	   typeof(*_self->value) _elem =  (ELEM); \
	   push(_self, &_elem, sizeof(_elem)); })

#define POP(SELF) __extension__\
	({ __auto_type _self = &(SELF); \
	   (typeof(_self->value))pop(_self, sizeof(*_self->value)); })

#define SHIFT(SELF) __extension__\
	({ __auto_type _self = &(SELF); \
	   (typeof(_self->value))shift(_self, sizeof(*_self->value)); })

#define INSERT(SELF, INDEX, ELEM) __extension__\
	({ __auto_type           _self = &(SELF); \
	   typeof(*_self->value) _elem =  (ELEM); \
	   insert(_self, (INDEX), &_elem, sizeof(_elem)); })

#define EXTEND(SELF, SLIC) __extension__\
	({ __auto_type _self = &(SELF); \
	   __auto_type _slic =  (SLIC); \
	   extend(_self, &_slic, sizeof(*_slic.value)); })

#define SPLICE(SELF, INDEX, SLIC) __extension__\
	({ __auto_type _self = &(SELF); \
	   __auto_type _slic =  (SLIC); \
	   splice(_self, (INDEX), &_slic, sizeof(*_slic.value)); })

#define CUT(SELF, FROM, UPTO) __extension__\
	({ __auto_type _self = &(SELF); \
	   static GenericSlice _cut; \
	   _cut = cut(_self, (FROM), (UPTO), sizeof(*_self->value)); \
	   (u0 *)&_cut; })

#define REMOVE(SELF, INDEX) __extension__\
	({ __auto_type _self = &(SELF); \
	   usize _indx = (INDEX); \
	   GenericSlice _cut \
	       = cut(_self, _indx, _indx, sizeof(*_self->value)); \
	   (typeof(_self->value))PTR(_cut); })

/* macros for hash-tables (maps) */
#define ASSOCIATE(SELF, KEY, VAL) __extension__\
	({ __auto_type _self = &(SELF); \
	   __auto_type _key = (KEY); \
	   __auto_type _val = (VAL); \
	   associate(_self, &_key, &_val); })

#define LOOKUP(SELF, KEY) __extension__\
	({ __auto_type _self = &(SELF); \
	   __auto_type _key = (KEY); \
	   (typeof(_self->buckets.value[0].value) *)lookup(_self, &_key); })

#define DROP(SELF, KEY) __extension__\
	({ __auto_type _self = &(SELF); \
	   __auto_type _key = (KEY); \
	   drop(_self, &_key); })

#define KEYS(MAP) __extension__\
	({ __auto_type _map = &(MAP); \
	   static GenericSlice _keys; \
	   _keys = get_keys(_map); \
	   (u0 *)&_keys; })

#define HAS_KEY(MAP, KEY) __extension__\
	({ __auto_type _map = &(MAP); \
	   __auto_type _key = (KEY); \
	   has_key(_map, &_key); })

// Some aliases and shortcuts:
#define APPEND(SELF, ELEM) PUSH(SELF, ELEM)
#define PREPEND(SELF, ELEM) INSERT(SELF, 0, ELEM)
#define UNSHIFT(SELF, ELEM) PREPEND(SELF, ELEM)
#define PREFIX(SELF, SLIC) SPLICE(SELF, 0, SLIC)
#define HEAD(SELF, END) SLICE(SELF, 0, END)
#define TAIL(SELF, BEG) SLICE(SELF, BEG, -1)
#define FIRST(SELF) NTH(SELF, 0)
#define LAST(SELF) GET(SELF, -1)

/// Minimal helper/wrapper around in-place `qsort` for arrays/slices.
#define QSORT(SELF, CMPR) __extension__\
	({ __auto_type _self = (SELF); \
	   qsort(PTR(_self), _self.len, sizeof(*_self.value), CMPR); \
	   _self; })

/// Wrapper around in-place `qsort` for arrays and slices.
/// Creates a nested function within a statement-expression
/// that is defined by the function body passed in as __VA_ARGS__
/// to the macro, which defines the comparison function for the sort.
/// This makes use of two GNU extensions:
///   'nested functions', and 'statement expressions'.
/// e.g.
///     SORT(arr, self, other, {
///         if ((Struct *)self->x == (Struct *)other->x) return  0;
///         if ((Struct *)self->x  < (Struct *)other->x) return -1;
///         if ((Struct *)self->x  > (Struct *)other->x) return +1;
///     });
///     SORT(arr, self, other, { return my_compare(self, other); });
///     SORT(arr, self, other, CMP((Struct *)self->x, (Struct *)other->x));
#define SORT(SELF, SUBJ, OTHR, ...) __extension__\
	({ int compar_fn_(const u0 *SUBJ, const u0 *OTHR) __VA_ARGS__ \
	   __auto_type _self = (SELF); \
	   qsort(PTR(_self), _self.len, sizeof(*_self.value), compar_fn_); \
	   _self; })

/// Function body for comparison of
/// items which have '>', '=' and '<' defined, e.g.
///     int comparef(float a, float b) CMP(a, b)
#define CMP(A, B) \
	{ if ((A) <  (B)) return -1; \
	  if ((A) == (B)) return  0; \
	  if ((A) >  (B)) return +1; }

// --- ANSI colour codes. ---

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

#define min(A, B) __extension__({ \
	typeof(A) _a = (A); \
	typeof(B) _b = (B); \
	_a > _b ? _b : _a; })
#define max(A, B) __extension__({ \
	typeof(A) _a = (A); \
	typeof(B) _b = (B); \
	_b > _a ? _b : _a; })

#define deref(T, PTR, ALT) __extension__\
	({ T _alt = (ALT); \
	   *(T *)or(PTR, &_alt); })

/// Unwraps pointer/value in sizing wrapper struct.
#define UNWRAP(STRUCTURE) (STRUCTURE).value
/// Explicitly only extract pointer from array/slice.
#define PTR(ARR) (ARR).value
/// Call to `free` of inside of slice/array/newtype, etc.
#define FREE_INSIDE(S) FREE((S).value)
/// Initialise sizing wrapper with literal.
#define INIT(TYPE, ...) { \
	.len = sizeof((TYPE[])__VA_ARGS__)/sizeof(TYPE), \
	.value = (TYPE[])__VA_ARGS__ \
}
/// Can be used to make slices from literal arrays.
#define LIST(TYPE, ...) __extension__({ \
	TYPE _slice; \
	typeof(*_slice.value) _elem; \
	static typeof(_elem) _list[] = __VA_ARGS__; \
	_slice = ((typeof(_slice)){ \
		.len = sizeof(_list)/sizeof(_elem), \
		.value = _list \
	}); _slice; })

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
/// Empty / zero struct.
#define EMPTY(TYPE) ((TYPE){ 0 })

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

#define SYMBOLIC(STR) ((symbol){ \
	.hash = hash_string(STR), \
	.value = STR \
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

#define SMAP(T, func, list) __extension__({ \
	T _mapped; \
	_mapped = ((T)SMAKE(typeof(*_mapped.value), (list).len)); \
	for (usize _i = 0; _i < (list).len; ++_i) \
		_mapped.value[_i] = (func)((list).value[_i]); \
	_mapped; \
})

#define AMAP(T, func, list) __extension__({ \
	T _mapped; \
	_mapped = ((T)AMAKE(typeof(*_mapped.value), (list).len)); \
	for (usize _i = 0; _i < (list).len; ++_i, ++_mapped.len) \
		_mapped.value[_i] = (func)((list).value[_i]); \
	_mapped; \
})

/// For-each loop, iterates across an array or slice.
/// It creates an `it` variable, that holds:
///  - it.index (index in array);
///  - it.item  (current item of array);
///  - it.ptr   (pointer to current item in array);
///  - it.first (pointer to frist item in array);
///  - it.once  (a bool, true if we are on the first iteration).
/// For example:
/// ```c
/// newarray(IntArray, int);
/// IntArray xs = AMAKE(IntArray, 2);
///
/// int elem1 = 5;
/// int elem2 = 3;
/// sliceof(int) elems = INIT(int, { 6, 9, 1 });
///
/// push(&xs, &elem1, sizeof(int));
/// push(&xs, &elem2, sizeof(int));
/// extend(&xs, &elems, sizeof(int));
///
/// FOR_EACH(x, xs) {
///     printf("xs[%zu] = %d\n", it.index, x);
/// }
/// ```
/// Will print:
/// ```
///   xs[0] = 5
///   xs[1] = 3
///   xs[2] = 6
///   xs[3] = 9
///   xs[4] = 1
/// ```
#define FOR_EACH(ELEM, ELEMS) \
	for (struct { typeof(*(ELEMS).value) item; \
			      typeof((ELEMS).value) ptr, first; \
				  usize index; \
				  bool once; \
				} it = { *(ELEMS).value, \
				         (ELEMS).value, \
				         (ELEMS).value, \
				         0, true \
				       }; it.once; it.once = false) \
		for (typeof(*(ELEMS).value) ELEM = *(ELEMS).value; \
		    it.index < (ELEMS).len; \
			++it.ptr, it.index = (it.ptr - it.first), \
			  it.item = *it.ptr, ELEM = it.item)

#define foreach FOR_EACH

/* static functions */
/* |- default hash functions */

 __attribute__((unused))
static u64 string_hash(const u0 *key, usize _)
{
	UNUSED(_);
	return hash_string(*(string *)key);
}

 __attribute__((unused))
static u64 runic_hash(const u0 *key, usize _)
{
	UNUSED(_);
	runic runes = *(runic *)key;
	return hash_bytes(TO_BYTES(runes));
}

 __attribute__((unused))
static u64 mem_hash(const u0 *key, usize _)
{
	UNUSED(_);
	return hash_bytes(*(MemSlice *)key);
}

 __attribute__((unused))
static u64 cstring_hash(const u0 *key, usize _)
{
	UNUSED(_);
	return hash_string(from_cstring(*(byte **)key));
}

 __attribute__((unused))
static u64 default_hash(const u0 *key, usize size)
{  // just hashes the plain raw bytes.
	return hash_bytes(VIEW(MemSlice, (umin *)key, 0, size));
}

/* Only define a `main` if ENTRY_FUNCTION is defined */
#ifdef ENTRY_FUNCTION
	newslice(Arguments, string);
	newslice(CArguments, const char *);

	ierr (ENTRY_FUNCTION)(Arguments);

	ierr main(ifast argc, const char **argv)
	{
		Arguments args;
		ierr res;

		args = SMAP(Arguments,
			from_cstring, SCOLLECT(CArguments, argc, argv));
		res = (ENTRY_FUNCTION)(args);

		free(UNWRAP(args));
		return res;
	}
#endif

#endif
