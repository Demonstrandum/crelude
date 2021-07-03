//! @file base64.h
//! Base-64 encoding and decoding.

#pragma once
#include "common.h"

/// Encoding characters / digits, each representing a value between 0..63.
static const sliceof(byte) BASE64_DIGITS = INIT(byte,
	{ "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/" });

/// Inverse/decoding table. (-1) represents absence from the table.
static const sliceof(i8) BASE64_INVERSES = INIT(i8, {
//  ------------------10------------------
    62, -1, -1, -1, 63, 52, 53, 54, 55, 56, // |
    57, 58, 59, 60, 61, -1, -1, -1, -1, -1, // |
    -1, -1,  0,  1,  2,  3,  4,  5,  6,  7, // |
     8,  9, 10, 11, 12, 13, 14, 15, 16, 17, // 8
    18, 19, 20, 21, 22, 23, 24, 25, -1, -1, // |
    -1, -1, -1, -1, 26, 27, 28, 29, 30, 31, // |
    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, // |
    42, 43, 44, 45, 46, 47, 48, 49, 50, 51  // |
});

/// Generates the above inverses table (length 80).
/// This could be a `constexpr` or something in a better language.
static i8 *base64_decode_table(void) __attribute__((unused));
static i8 *base64_decode_table()
{
	static i8 table[80];
	memset(table, -1, sizeof(table));

	for (usize i = 0; i < BASE64_DIGITS.len - 1; ++i)
		table[NTH(BASE64_DIGITS, i) - '+'] = i;

	return &*table;
}

static inline bool is_base64_digit(byte d)
{
	return (d >= '0' && d <= '9')
	    || (d >= 'A' && d <= 'Z')
	    || (d >= 'a' && d <= 'z')
	    ||  d == '+' || d == '/'
	    ||  d == '=';
}

/// Calculate encoded size given original size.
usize base64_encoded_size(usize);
/// Calculate decoded size given the endcoded data.
usize base64_decoded_size(MemSlice);

/// Encode bytes into base-64.
/// @returns A slice holding a heap-allocated free-able pointer
///          to the encoded data.
MemSlice base64_encode(MemSlice);

/// Decode from base-64 to raw bytes.
/// If input is not a valid base-64 string,
/// an empty nil-slice will be returned.
/// @returns A heap allocated slice holding the raw decoded data.
MemSlice base64_decode(MemSlice);
