//! @file tests.c
//! File for testing the library.
//! Not part of the library.

#include <crelude/common.h>
#include <crelude/utf.h>
#include <crelude/base64.h>

#include <stdio.h>
#include <locale.h>

#define TEST(DOES) do { \
	println("\n" ANSI(BOLD) "[###]" ANSI(RESET) " "\
		ANSI(UNDER) "Test Case" ANSI(RESET) ": %s", DOES); \
} while (false); if (true)

u0 utf8_ucs4_conversions(byte *cstring)
{
	// UTF-8 string.
	string s = to_string(cstring);
	println("s = \"%s\"", UNWRAP(s));
	FOR_EACH(c, s) println("byte: 0x%X", c);
	println("s.len = %zu", s.len);
	// Convert to UCS-4.
	println("[***] Convert to UCS-4.");
	runic ucs4 = SMAKE(rune, s.len + 1);
	ucs4 = utf8_to_ucs4(ucs4, s);
	println("ucs4 = \"%ls\"", (wchar_t *)UNWRAP(ucs4));
	FOR_EACH(c, ucs4) println("rune: U+%08X = '%lc'", c, c);
	println("ucs4.len = %zu", ucs4.len);
	// And back again.
	println("[***] Convert to UTF-8.");
	string utf8 = SMAKE(byte, 4 * ucs4.len + 4);
	utf8 = ucs4_to_utf8(utf8, ucs4);
	println("utf8 = \"%s\"", UNWRAP(utf8));
	FOR_EACH(c, utf8) println("byte: 0x%X", c);
	println("utf8.len = %zu", utf8.len);
}

newtype(Natural, u64);  // New-type idiom.

#ifndef IMPLEMENTATION

ierr main(i32 argc, const byte **argv)
{
	UNUSED(argc); UNUSED(argv);
	byte *locale; UNUSED(locale);
    locale = setlocale(LC_ALL, "");
	println("Locale is UTF-8?  %s.",
		is_locale_utf8(locale) ? "true" : "false");

	Natural n = { 7 };  // How to use newtypes.
	n.value = 4;  UNUSED(n);

	/* UTF-8 <--> UCS-4, conversions. */
	TEST("Converts between UTF-8 and UCS-4.") {
		println("=== Chinese Characters ===");
		utf8_ucs4_conversions("你好");
		println("\n=== Latin-1/ASCII Characters ===");
		utf8_ucs4_conversions("AaBb");
		println("\n=== Look-alike Characters ===");
		utf8_ucs4_conversions("maña");  // U+00F1 = ñ.
		utf8_ucs4_conversions("maña");  // n + U+0303 = ñ.
		// The second string has one extra 'code-point', but both
		// have the exact same number of human distinguishable 'graphemes'.
	}

	/* UTF-8 Escapes */
	TEST("Unescaped ASCII/UTF-8 strings to UTF-8") {
		println("=== Unescaping Strings ===");
		string unescaped = SMAKE(byte, 128);
		string escaped = STRING("Hello, \\U1F30E.");
		println("escaped.len = %zu", escaped.len);
		unescaped = utf8_unescape(unescaped, escaped);
		println("unescaped.len = %zu", unescaped.len);
		println("\"%s\" --> \"%s\"", escaped.value, unescaped.value);
	}

	TEST("Escaped UTF-8 strings to escaped ASCII") {
		println("=== Escaped Strings ===");
		string escaped = SMAKE(byte, 128);
		string unescaped = STRING("Hello, 🌎.");
		println("unescaped.len = %zu", unescaped.len);
		escaped = utf8_escape(escaped, unescaped, false);
		println("escaped.len = %zu", escaped.len);
		println("\"%s\" --> \"%s\"", unescaped.value, escaped.value);
	}

	TEST("The new (internal) printf variant function") {
		rune c = U'𰻝';
		println("Code point for '%C' = %U.", c, c);

		runic phrase = INIT(rune, { 0x73AB, 0x7470, 0x6C34 });
		println("The runes { %V{U+%04X}{, } }, represent: \"%r\".", phrase, phrase);
		println("Which are the three graphemes: %V{'%C'}{, }.", phrase);

		long long int some_ll_int = 36;
		println("A normal base-10 long long integer: %05lld.", some_ll_int);

		StringBuilder builder = AMAKE(byte, 5);

		extend(&builder, &STR("Hello"), sizeof(byte));
		push(&builder, ",", sizeof(byte));
		string name = STRING("Bob.");
		extend(&builder, &name, sizeof(byte));
		assert('.' == *(byte *)pop(&builder, sizeof(byte)));
		push(&builder, "!", sizeof(byte));
		insert(&builder, 6, " ", sizeof(byte));

		string slice = SLICE(string, builder, 0, -1);
		println("The characters %D{(%c)}-, say: \"%S\".", builder, slice);
		string slice_with_nul = SLICE(string, builder, 0, 12);
		println("With ASCII values: [%V{#%02hhX}{, }].", slice_with_nul);

		string alt = STRING("Strings may be printed like this too.\n");
		print("%Vc{}", alt);
		println("(that is, %V02hhX-)", alt);  // No curly-braces needed if unambiguous!
	}

	TEST("String comparison: string_cmp vs. strcmp") {
		string s0 = STRING("Latino");
		string s1 = STRING("Latina");
		byte *p0 = UNWRAP(s0);
		byte *p1 = UNWRAP(s1);

		println("string_cmp: %s <~> %s = %hd", p0, p1, string_cmp(s0, s1));
		println("strcmp:     %s <~> %s = %d",  p0, p1, strcmp(p0, p1));
		assert(string_cmp(s0, s1) == strcmp(p0, p1));

		string s2 = SLICE(string, s0, 0, -2);
		string s3 = SLICE(string, s1, 0, -2);
		println("string_cmp: %S <~> %S = %hd", s2, s3, string_cmp(s2, s3));

		println("string_cmp: %S <~> %S = %hd", s3, s0, string_cmp(s3, s0));
		println("string_cmp: %S <~> %S = %hd", s2, s0, string_cmp(s2, s0));
		p1[s1.len - 1] = '\0';
		println("strcmp:     %s <~> %s = %d", p1, p0, strcmp(p1, p0));
	}

	TEST("Array removal and block swapping") {
		newarray(IntArray, int);
		IntArray arr = AMAKE(int, 5);
		PUSH(arr, 0);
		__auto_type list = LIST(sliceof(int), { 1, 2, 3, 4, 5, 6 });
		UNSHIFT(arr, -1);
		println("list[..%zu] = { %V%d{, } };", list.len, list);
		EXTEND(arr, list);
		println("arr = %D%d{, };", arr);
		int *popped = SHIFT(arr);
		println("arr = %D%d{, };  (popped: %d)", arr, *popped);
		SWAP(arr, 3);  // Swap the three first elements to the back.
		println("arr = %D%d{, };", arr);
		REMOVE(arr, 4);  // Remove 4th element (0).
		println("arr = %D%d{, };", arr);
		SWAP(arr, 4);
		println("arr = %D%d{, };", arr);
		sliceof(int) *cutout = CUT(arr, 2, 4);
		println("arr = %D%d{, };  (cut out: %V%d{, })", arr, *cutout);
	}

	TEST("Base64 encoding and decoding.") {
		sliceof(u16) data = INIT(u16, {
			__extension__ 0b0000000000000000,  //<      0 (0x00 0x00).
			__extension__ 0b0011001000110010,  //< 12,850 (0x32 0x32).
			__extension__ 0b0000011000000110,  //<  1,542 (0x06 0x06).
			__extension__ 0b0111010101110101   //< 30,069 (0x75 0x75).
		}); //< 64-bit integer value = 55,190,430,840,181 (big endian).

		MemSlice bytes = TO_BYTES(data);
		println("{ %V%hu{, } } <=> { %V{0x%02hhX}{, } }", data, bytes);

		MemSlice repr = bytes;
		if (is_little_endian())  // Convert to big endian byte order.
			repr = reverse_endianness(repr);
		println("  (decimal: %lu)", *(u64 *)PTR(repr));

		MemSlice encoded = base64_encode(bytes);
		println("encodes to: %V%c{}", encoded);

		if (is_little_endian())
				FREE_INSIDE(repr);

		MemSlice decoded = base64_decode(encoded);
		repr = decoded;
		if (is_little_endian())
			repr = reverse_endianness(repr);

		println("decodes to: %lu", *(u64 *)PTR(repr));

		FREE_INSIDE(encoded);
		if (is_little_endian())
			FREE_INSIDE(repr);

		string str = STRING("Hello, World!");
		println("\"%S\" <=> { %V{0x%hhX}{, } }", str, str);
		encoded = base64_encode(TO_BYTES(str));
		println("encodes to: %V%c{}", encoded);

		decoded = base64_decode(encoded);
		println("decodes to: \"%V%c{}\".", decoded);

		FREE_INSIDE(encoded);
		FREE_INSIDE(decoded);
	}

	return EXIT_SUCCESS;
}

#endif
