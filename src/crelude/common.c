#include "common.h"
#include "utf.h"

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <wchar.h>

#ifndef IMPLEMENTATION

u0 panic(const byte *message, ...)
{
	va_list args;
	va_start(args, message);
	ierr res = novel_vfprintf_newline(stderr, message, args);
	va_end(args);

	if (res < 0)
		eputs("panic: Out of space.");

	abort();
}

bool is_zero(imax n)
{ return n == 0; }

bool is_zerof(f64 n)
{ return n == 0.0; }

bool is_zeroed(u0 *ptr, usize width)
{
	umin *xs = (umin *)ptr;
	until (width-- == 0) unless (*xs++ == 0) return false;
	return true;
}

u0 zero(u0 *blk, usize width)
{
	if (blk == nil || width == 0)
		return UNIT;
	umin *bytes = blk;
	until (width-- == 0)
		*bytes++ = 0;
	return UNIT;
}

u0 *emalloc(usize len, usize size)
{
	usize bytes = len * size;
	u0 *m = MALLOC(bytes);
	if (m == nil)
		PANIC("Could not allocate %zu bytes.", bytes);
	zero(m, bytes);
	return m;
}

/* in-place */
u0 reverse(u0 *self, usize width)
{
	umin chunk[width];  //< C99 variable-length array.
	MemArray *arr = self;
	umin *ptr = arr->value;
	usize len = arr->len * width;

	for (usize i = 0, j = len - 1; i < j; ++i, --j) {
		memcpy(  chunk, ptr + i, width);
		memcpy(ptr + i, ptr + j, width);
		memcpy(ptr + j,   chunk, width);
	}

	return UNIT;
}

MemSlice reverse_endianness(MemSlice bytes)
{
	MemSlice copy;
	copy = COPY(bytes);

	for (usize i = 0, j = copy.len - 1; i < j; ++i, --j) {
		umin temp = NTH(copy, i);
		NTH(copy, i) = NTH(copy, j);
		NTH(copy, j) = temp;
	}

	return copy;
}

bool is_little_endian()
{
	uword w = 0x01;
	return *(umin *)&w == 1;
}

u128 big_endian(umin *start, usize bytes)
{
	assert(bytes <= sizeof(u128));
	// Write to offset in word (big endian), then reverse
	// depending on endianess. i.e., [0x32][0xF1] becomes:
	//   [0x00][0x00][0x32][0xF1]  (big endian), or
	//   [0xF1][0x32][0x00][0x00]  (little endian)
	umin mem[sizeof(u128)];
	memset(mem, 0, sizeof(u128));
	memcpy(mem + sizeof(u128) - bytes, start, bytes);

	MemSlice reversed = VIEW(MemSlice, mem, 0, sizeof(u128));
	if (is_little_endian())  // reverse `mem` if little endian.
		reverse(&reversed, 1);

	return *(u128 *)mem;
}

static u0 memswap(umin *a, umin *b, usize bytes)
{
	usize words = bytes / WORD_SIZE;
	usize trail = words * WORD_SIZE;  //< Where left-over bytes start.

	// Swap word sized blocks first.
	for (usize i = 0; i < words; i += WORD_SIZE) {
		uword tmp;
		memcpy( &tmp, a + i, WORD_SIZE);
		memcpy(a + i, b + i, WORD_SIZE);
		memcpy(b + i,  &tmp, WORD_SIZE);
	}
	// Swap remaining bytes, byte-by-byte.
	for (usize i = trail; i < bytes; ++i) {
		umin tmp = a[i];
		a[i] = b[i];
		b[i] = tmp;
	}
}

/// # Swapping memory (on pivot point `b`):
/// ```
///    a       b     c       d
///    |---A---:------B------|
/// == |---A---:--B1-:---B2--|
/// => |---B2--:--B1-:---A---|
/// => |--B1-:---B2--:---A---|  (by same procedure, recursive)
/// == |------B------:---A---|
/// ```
/// When swapping block A and B.
/// Notice the pivot remains the same (b) for each recursive call.
/// (a, b, and c are pointers, and do not change).
///
/// When the pivot point is `c`,
/// i.e. when block A is the larger block, we have:
/// ```
///    a       b     c       d
///    |------A------:---B---|
/// == |---A1--:--A2-:---B---|
/// => |---B---:--A2-:--A1---|
/// => |---B---:--A1---:--A2-|  (by same procedure, recursive)
/// == |---B---:------A------|
/// ```
static u0 swap_blocks(MemSlice block, usize pivot)
{  // Tail-recursion should be optimised here.
	if (pivot == 0 || pivot == block.len)
		return;  // We are already done.

	MemSlice A = SLICE(MemSlice, block, 0, pivot);
	MemSlice B = SLICE(MemSlice, block, pivot, -1);
	MemSlice A1, A2;
	MemSlice B1, B2;

	if (A.len < B.len) {
		B1 = SLICE(MemSlice, B, 0, -A.len - 1);
		B2 = SLICE(MemSlice, B, B1.len, -1);
		assert(A.len == B2.len);

		memswap(A.value, B2.value, A.len);
		B = SLICE(MemSlice, block, 0, B.len);
		swap_blocks(B, pivot);
	} else if (A.len > B.len) {
		A1 = SLICE(MemSlice, A, 0, B.len);
		A2 = SLICE(MemSlice, A, A1.len, -1);
		assert(B.len == A1.len);
		assert(pivot == A1.len + A2.len);

		memswap(B.value, A1.value, B.len);
		A = SLICE(MemSlice, block, B.len, -1);
		swap_blocks(A, pivot - B.len);
	} else {
		// Same length, and thus trivial.
		memswap(A.value, B.value, A.len);
	}
}

u0 swap(u0 *self, usize pivot, usize width)
{
	// Deal with everything in terms of bytes.
	MemSlice blk = *(MemSlice *)self;
	blk.len *= width;
	usize pos = pivot * width;
	swap_blocks(blk, pos);
}

usize push(u0 *restrict self, const u0 *restrict element, usize width)
{
	if (element == nil) return 0;

	MemArray *arr = (MemArray *restrict)self;
	umin *elem = (umin *restrict)element;

	usize old_cap = arr->cap;
	usize new_cap = old_cap;

	unless (arr->len < arr->cap) {
		new_cap = (usize)(arr->cap * REALLOC_FACTOR) + 1;
		arr->value = (umin *)realloc(arr->value, new_cap * width);
		if (arr->value == nil)
			PANIC("Failed to reallocate %zu bytes.", new_cap * width);
		arr->cap = new_cap;
	}

	memcpy(arr->value + width * arr->len++, elem, width);
	return new_cap - old_cap;
}

usize insert(u0 *restrict self, usize index,
             const u0 *restrict element, usize width)
{
	if (element == nil) return 0;

	MemArray *arr = (MemArray *restrict)self;
	umin *elem = (umin *restrict)element;

	usize old_cap = arr->cap;
	usize new_cap = old_cap;

	unless (arr->len < arr->cap) {
		new_cap = (usize)(arr->cap * REALLOC_FACTOR) + 1;
		arr->value = (umin *)realloc(arr->value, new_cap * width);
		if (arr->value == nil)
			PANIC("Failed to reallocate %zu bytes.", new_cap * width);
		arr->cap = new_cap;
	}

	umin *gap = arr->value + width * index;

	// Offset elements down by one, from insertion index,
	// leaving us with a gap for insertion.
	memmove(gap + width, gap, width * (arr->len++ - index));
	// Insert element into the empty space.
	memcpy(gap, elem, width);

	return new_cap - old_cap;
}

usize extend(u0 *restrict self, const u0 *restrict slice, usize width)
{
	if (slice == nil) return 0;

	MemArray *arr = (MemArray *restrict)self;
	MemSlice *sub = (MemSlice *restrict)slice;

	if (sub->len == 0) return 0;

	usize old_cap = arr->cap;
	usize new_cap = old_cap;

	unless (arr->len + sub->len < arr->cap) {
		new_cap = (usize)(arr->cap * REALLOC_FACTOR) + sub->len;
		arr->value = (umin *)realloc(arr->value, new_cap * width);
		if (arr->value == nil)
			PANIC("Failed to reallocate %zu bytes.", new_cap * width);
		arr->cap = new_cap;
	}

	memcpy(arr->value + width * arr->len, sub->value, width * sub->len);
	arr->len += sub->len;
	return new_cap - old_cap;
}

usize splice(u0 *restrict self, usize index, const u0 *restrict slice, usize width)
{
	if (slice == nil) return 0;

	MemArray *arr = (MemArray *restrict)self;
	MemSlice *sub = (MemSlice *restrict)slice;

	if (sub->len == 0) return 0;

	usize old_cap = arr->cap;
	usize new_cap = old_cap;

	unless (arr->len + sub->len < arr->cap) {
		new_cap = (usize)(arr->cap * REALLOC_FACTOR) + sub->len;
		arr->value = (umin *)realloc(arr->value, new_cap * width);
		if (arr->value == nil)
			PANIC("Failed to reallocate %zu bytes.", new_cap * width);
		arr->cap = new_cap;
	}

	umin *gap = arr->value + width * index;

	// Offset elements down by the slice length, from insertion index,
	// leaving us with a `width * sub.len` byte gap for insertion.
	memmove(gap + width * sub->len, gap, width * (arr->len - index));
	arr->len += sub->len;
	// Insert slice into empty space.
	memcpy(gap, sub->value, sub->len * width);

	return new_cap - old_cap;
}

GenericSlice cut(u0 *self, usize from, isize upto, usize width)
{
	GenericArray *arr_ptr = self;
	MemArray arr = *(MemArray *)self;

	usize final = upto < 0 ? arr.len + upto : (usize)upto;
	assert(final < arr.len);

	if (from > final) {
		usize tmp = from;
		from = final;
		final = tmp;
	}
	usize pivot = final - from + 1;

	// Deal with bytes instead.
	arr.len *= width;
	arr.cap *= width;
	from *= width;
	final *= width;
	pivot *= width;

	MemSlice tail = SLICE(MemSlice, arr, from, arr.len);
	swap_blocks(tail, pivot);
	tail = SLICE(MemSlice, tail, tail.len - pivot, -1);

	u0 *tail_ptr = &tail;
	GenericSlice trailing = *(GenericSlice *)tail_ptr;
	trailing.len /= width;
	arr_ptr->len -= pivot / width;  // Capacity the same.
	return trailing;  // Return removed slice which is left over at end.
}

u0 *pop(u0 *self, usize width)
{
	MemArray *arr = self;
	assert(arr->len > 0);
	return (u0 *)(arr->value + --arr->len * width);
}

u0 *shift(u0 *self, usize width)
{
	MemSlice *arr = self;
	assert(arr->len > 0);

	swap(self, 1, width);
	return (u0 *)(arr->value + --arr->len * width);
}

string from_cstring(const byte *cstring)
{
	return VIEW(string, (byte *)cstring, 0, strlen(cstring));
}

bool string_eq(string self, const string other)
{
	unless (self.len == other.len)
		return false;
	else if (self.value == other.value)
		return true;

	foreach (c, self)
		if (c != UNWRAP(other)[it.index])
			return false;
	return true;
}

i16 string_cmp(const string self, const string other)
{
	byte *ptr0 = self.value,
	     *ptr1 = other.value;

	if (ptr0 == ptr1) {
		if (self.len == other.len) return 0;
		return self.len > other.len
			? ptr0[other.len] - 0
			: 0 - ptr1[self.len];
	}

	byte c0, c1;
	usize len = min(self.len, other.len);
	for (usize i = 0; i < len; ++i)
		if ((c0 = ptr0[i]) != (c1 = ptr1[i]))
			return c0 - c1;

	if (self.len == other.len) return 0;
	if (self.len == len)
		return 0 - ptr1[len];
	return ptr0[len] - 0;
}

/// `djb2` hash-algo.
u64 hash_string(string str)
{
	u64 hash = 5381;
	foreach (c, str)
		hash += c + (hash << 5);

	return hash;
}

ierr fput(string s, FILE *stream)
{
	ierr err = 1;
	foreach (c, s) {
		unless (err == NUL) err = fputc(c, stream);
		else return err;
	}
	err = fputc('\n', stream);
	return err;
}

ierr put(string s)
{ return fput(s, stdout); }

ierr eput(string s)
{ return fput(s, stderr); }

ierr eputs(const byte *s)
{
	ierr err;
	err = fputs(s, stderr);
	if (err <= 0) return err;
	err = fputc('\n', stderr);
	return err;
}

usize sizeof_specifier(const byte *spec)
{
	UNUSED(spec);
	PANIC("Not implemented.");
	return 0;
}

static string FORMATTER_FLAGS
	= INIT(byte, { '+', '-', ' ', '#', '0' });
static string LENGTH_SUBSPECIFIERS
	= INIT(byte, { 'h', 'l', 'j', 'z', 't', 'L' });

/// A `printf` style formatter is in the form
///
/// formatter ::= '%' [flags] [width] ['.' precision] [length] specifier
///
/// Here, those correspond to:
///
/// flag ::= '+' | '-' | ' ' | '#' | '0'
/// flags ::= {flag}
/// width ::= {'0'..'9'} | '*'
/// precision ::= {'0'..'9'} | '*'
/// length ::= 'hh' | 'h' | 'll' | 'l' | 'j' | 'z' | 't' | 'L'
/// specifier ::= %|C|r|S|U|b|D|V|d|i|u|o|x|X|f|F|e|E|g|G|a|A|c|s|p|n

struct Formatter {
	string flags;
	string width;
	string precision; ///< Without '.' character.
	string length;
	byte specifier;

	usize offset;
};
unqualify(struct, Formatter);

// NOTE: Allocates on heap.
static string formatter_string(Formatter formatter)
{
	StringBuilder repr = AMAKE(byte, formatter.offset + 2);

	push(&repr, "%", 1);
	extend(&repr, &formatter.flags, 1);
	extend(&repr, &formatter.width, 1);
	unless (IS_EMPTY(formatter.precision)) {
		push(&repr, ".", 1);
		extend(&repr, &formatter.precision, 1);
	}
	extend(&repr, &formatter.length, 1);
	push(&repr, &formatter.specifier, 1);

	// NUL-terminate the string slice.
	push(&repr, &NUL_BYTE, 1);
	return SLICE(string, repr, 0, -2);
}

static Formatter parse_formatter(string formatter)
{
	usize i = 0;
	if (formatter.value[i] == '%') ++i;

	usize flags_start = i;
	loop {  // Parse flags.
		bool found = false;
		foreach (ss, FORMATTER_FLAGS) {
			if (formatter.value[i] == ss) {
				found = true;
				++i;
			}
		}
		unless (found) break;
	}
	string flags = SLICE(string, formatter, flags_start, i);


	usize width_start = i;  // Parse width.
	if (formatter.value[i] == '*') ++i;
	else while (isdigit(formatter.value[i])) ++i;
	string width = SLICE(string, formatter, width_start, i);

	usize precision_start = i;
	if (formatter.value[i] == '.') {  // Parse precision.
		++i;  // Skip '.'
		if (formatter.value[i] == '*') ++i;
		else while (isdigit(formatter.value[i])) ++i;
	}
	string precision = SLICE(string, formatter, precision_start, i);

	usize length_start = i;
	loop {  // Parse length subspecifier.
		bool found = false;
		foreach (ss, LENGTH_SUBSPECIFIERS) {
			if (formatter.value[i] == ss) {
				found = true;
				++i;
			}
		}
		unless (found) break;
	}
	string length = SLICE(string, formatter, length_start, i);

	// Parse specifier as the single byte immediately after.
	byte specifier = formatter.value[i++];

	return ((Formatter){
		.flags = flags,
		.width = width,
		.precision = precision,
		.length = length,
		.specifier = specifier,
		.offset = i
	});
}

// TODO(maybe): Add a binary formatter.
string novel_vsprintf(const byte *format, va_list args)
{
	newarray(ByteArray, byte);
	ByteArray bytes = AMAKE(byte, strlen(format) + 64);

	usize i = 0;
	byte c;
	until ('\0' == (c = format[i++])) {
		unless (c == '%') {
			push(&bytes, &c, sizeof(byte));  // Other characters are preserved.
			continue;
		}

		string format_string = VIEW(string, (byte *)format, i, -1);
		Formatter formatter = parse_formatter(format_string);
		i += formatter.offset;

		bool is_array_formatter = false;

		// TODO: Padding, text formatting etc. on the novel formatters,
		//       i.e. %b, %S, %C, %r, %U, %D and %V.
		switch (formatter.specifier) {
		case 'S': {  // '%S', string slice formatter.
			string value = va_arg(args, string);
			extend(&bytes, &value, sizeof(byte));
		} break;
		case 'C': {  // '%C', rune formatter.
			rune value = va_arg(args, rune);
			string ucs_bytes = INIT(byte, { 0, 0, 0, 0, 0 });
			ucs_bytes = rune_to_utf8(ucs_bytes, value);
			extend(&bytes, &ucs_bytes, sizeof(byte));
		} break;
		case 'r': {  // '%r', runic (UCS-4) string formatter.
			runic value = va_arg(args, runic);
			string ucs_bytes = SMAKE(byte, 4 * (value.len + 1));
			ucs_bytes = ucs4_to_utf8(ucs_bytes, value);
			extend(&bytes, &ucs_bytes, sizeof(byte));
		} break;
		case 'U': {  // '%U', runic 8-(hex)digit unicode codepoint formatter.
			rune value = va_arg(args, rune);
			byte res[] = "U+00000000";
			string sliced = VIEW(string, res, 0, 10);
			sprintf(res + 2, "%08X", value);
			extend(&bytes, &sliced, sizeof(byte));
		} break;
		case 'b': {  // '%b' boolean formatter.
			int value = va_arg(args, int);  //< _Bool gets promoted to int.
			string bool_string = value ? STR("true") : STR("false");
			extend(&bytes, &bool_string, sizeof(byte));
		} break;
		case 'D':  // '%D{·}{·}' dynamic array type formatter.
			// Following the 'D' must be the format specifier for the elements,
			// and finally the delimiter characters.
			// e.g. array of `int`s, separated by command and a space (", "):
			// `println("[%Dd{, }]", my_arr);`.
			is_array_formatter = true;
			/* fallthrough */
		case 'V': {  // '%V{·}{·}' view/slice type formatter.
			// Following the 'V' must be the format specifier for the elements.
			// and finally the delimiter characters.
			// e.g. slice of doubles, separated by tabs:
			// `println("{ %V{%0.3lf}\t }", my_slice);`.
			string elem_repr = SEMPTY(string);
			Formatter elem_formatter;

			if (format[i] == '{') {
				usize begin = ++i;  // TODO: Nesting '{...}'?
				until (format[i] == '}') ++i;
				usize len = i - begin;
				++i;  // Skip '}'.

				// `elem_repr` must NUL-terminate, hence we make a copy.
				byte *buf = emalloc(len, sizeof(byte));
				buf = memcpy(buf, format + begin, len * sizeof(byte));
				buf[len] = '\0';
				elem_repr = VIEW(string, buf, 0, len);

				usize j = 0;
				until (elem_repr.value[j] == '%')  // TODO: Better error message.
					if (j >= elem_repr.len)
						PANIC("Broken printf element formatter, missing '%%'.");
					else ++j;
				// Slice with everything after the
				// '%' sign in the element formatter.
				string elem_format_string = SLICE(string, elem_repr, j + 1, -1);
				elem_formatter = parse_formatter(elem_format_string);
			} else {  // Otherwise, it should just have a specifier.
				// Prepend a '%' character, if not given.
				if (format[i] == '%') ++i;

				string elem_format_string = VIEW(string, (byte *)format, i, -1);
				elem_formatter = parse_formatter(elem_format_string);
				usize offs = elem_formatter.offset;

				// `elem_repr` must NUL-terminate, hence we make a copy.
				byte *buf = emalloc(offs + 2, sizeof(byte));
				buf[0] = '%';
				memcpy(buf + 1, format + i, offs * sizeof(byte));
				buf[offs + 1] = '\0';
				elem_repr = VIEW(string, buf, 0, offs + 1);

				i += offs;
			}

			string delim = SEMPTY(string);
			if (format[i] == '{') {
				usize begin = i + 1;  // TODO: Nesting '{...}'?
				until (format[++i] == '}');
				delim = VIEW(string, (byte *)format, begin, i);
				++i;  // Skip last brace.
			} else {  // Single character for the delimiter.
				delim = VIEW(string, (byte *)format, i, i + 1);
				++i;
			}

			#define SPRINT_ELEM(TYPE) do { \
				newslice(TSlice, TYPE); \
				newarray(TArray, TYPE); \
				TSlice slice; \
				if (is_array_formatter) { \
					TArray arr = va_arg(args, TArray); \
					slice = SLICE(TSlice, arr, 0, -1); \
				} else { \
					slice = va_arg(args, TSlice); \
				} \
				for (usize n = 0; n < slice.len; ++n) { \
					TYPE *elem = slice.value + n; \
					string elem_str = novel_sprintf(elem_repr.value, *elem); \
					extend(&bytes, &elem_str, sizeof(byte)); \
					FREE(elem_str.value); \
					unless (n == slice.len - 1) \
						extend(&bytes, &delim, sizeof(byte)); \
				} \
			} while (false)

			string fmt_l = elem_formatter.length;

			switch (elem_formatter.specifier) {
			// New formatters.
			case 'C': SPRINT_ELEM(rune); break;
			case 'U': SPRINT_ELEM(rune); break;
			case 'r': SPRINT_ELEM(runic); break;
			case 'S': SPRINT_ELEM(string); break;
			case 'D': SPRINT_ELEM(GenericArray); break;
			case 'V': SPRINT_ELEM(GenericSlice); break;
			// Standard C formatters:
			case 'i': case 'd':
				if (string_eq(fmt_l, STR("hh")))
					SPRINT_ELEM(signed char);
				else if (string_eq(fmt_l, STR("ll")))
					SPRINT_ELEM(long long int);
				else if (string_eq(fmt_l, STR("h")))
					SPRINT_ELEM(short int);
				else if (string_eq(fmt_l, STR("l")))
					SPRINT_ELEM(long int);
				else if (string_eq(fmt_l, STR("j")))
					SPRINT_ELEM(imax);
				else if (string_eq(fmt_l, STR("z")))
					SPRINT_ELEM(isize);
				else if (string_eq(fmt_l, STR("t")))
					SPRINT_ELEM(iptr);
				else
					SPRINT_ELEM(int);
				break;
			case 'o': case 'u': case 'x': case 'X':
				if (string_eq(fmt_l, STR("hh")))
					SPRINT_ELEM(unsigned char);
				else if (string_eq(fmt_l, STR("ll")))
					SPRINT_ELEM(unsigned long long int);
				else if (string_eq(fmt_l, STR("h")))
					SPRINT_ELEM(unsigned short int);
				else if (string_eq(fmt_l, STR("l")))
					SPRINT_ELEM(unsigned long int);
				else if (string_eq(fmt_l, STR("j")))
					SPRINT_ELEM(umax);
				else if (string_eq(fmt_l, STR("z")))
					SPRINT_ELEM(usize);
				else if (string_eq(fmt_l, STR("t")))
					SPRINT_ELEM(uptr);
				else
					SPRINT_ELEM(unsigned int);
				break;
			case 'c':
				if (string_eq(fmt_l, STR("l")))
					SPRINT_ELEM(wint_t);
				else
					SPRINT_ELEM(char);
				break;
			case 'e': case 'f': case 'g': case 'a':
			case 'E': case 'F': case 'G': case 'A':
				if (string_eq(fmt_l, STR("L")))
					SPRINT_ELEM(long double);
				else
					SPRINT_ELEM(double);
				break;
			case 's':
				if (string_eq(fmt_l, STR("l")))
					SPRINT_ELEM(wchar_t *);
				else
					SPRINT_ELEM(char *);
				break;
			case 'p':
				SPRINT_ELEM(uptr); break;
			case 'n':
				if (string_eq(fmt_l, STR("hh")))
					SPRINT_ELEM(signed char *);
				else if (string_eq(fmt_l, STR("ll")))
					SPRINT_ELEM(long long int *);
				else if (string_eq(fmt_l, STR("h")))
					SPRINT_ELEM(short int *);
				else if (string_eq(fmt_l, STR("l")))
					SPRINT_ELEM(long int *);
				else if (string_eq(fmt_l, STR("j")))
					SPRINT_ELEM(imax *);
				else if (string_eq(fmt_l, STR("z")))
					SPRINT_ELEM(usize *);
				else if (string_eq(fmt_l, STR("t")))
					SPRINT_ELEM(iptr *);
				else
					SPRINT_ELEM(int *);
				break;
			default:
				PANIC("Unknown formatter ('%%%c') in array/slice.",
					elem_formatter.specifier);
				break;
			}

			FREE_INSIDE(elem_repr);
		} break;
		default: {
			// Send it off to internal `vsprintf`.
			string c_formatter = formatter_string(formatter);

			byte *buf;
			isize len = vasprintf(&buf, c_formatter.value, args);
			string buf_slice = VIEW(string, buf, 0, len);
			extend(&bytes, &buf_slice, sizeof(byte));
			FREE(buf);
			FREE(c_formatter.value);
		} break;
		}
	}

	// NUL-terminate the string slice.
	push(&bytes, &NUL_BYTE, sizeof(byte));
	--bytes.len;  //< But, don't count the NUL-byte as part of the length.
	return SLICE(string, bytes, 0, -1);
}

string novel_sprintf(const byte *format, ...)
{
	va_list args;
	va_start(args, format);
	string res = novel_vsprintf(format, args);
	va_end(args);
	return res;
}

ierr novel_vfprintf(FILE *stream, const byte *format, va_list args)
{
	string s = novel_vsprintf(format, args);
	ierr res = fputs(s.value, stream);
	FREE(s.value);
	return res;
}

ierr novel_fprintf(FILE *stream, const byte *format, ...)
{
	va_list args;
	va_start(args, format);
	ierr res = novel_vfprintf(stream, format, args);
	va_end(args);
	return res;
}

ierr novel_vfprintf_newline(FILE *stream, const byte *format, va_list args)
{
	ierr res = novel_vfprintf(stream, format, args);
	if (res < 0) return res;
	if (EOF == fputc('\n', stream))
		return EOF;
	return res + 1;
}

ierr novel_fprintf_newline(FILE *stream, const byte *format, ...)
{
	va_list args;
	va_start(args, format);
	ierr res = novel_vfprintf_newline(stream, format, args);
	va_end(args);
	return res;
}

ierr novel_printf(const byte *format, ...)
{
	va_list args;
	va_start(args, format);
	ierr res = novel_vfprintf(stdout, format, args);
	va_end(args);
	return res;
}

#endif

