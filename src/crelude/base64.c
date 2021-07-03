#include "base64.h"

#ifndef IMPLEMENTATION

usize base64_encoded_size(usize original)
{
	usize size = original;
	unless (original % 3 == 0)
		size += 3 - (original % 3);
	size /= 3;
	size *= 4;

	return size;
}

usize base64_decoded_size(MemSlice encoded)
{
	usize size = encoded.len / 4 * 3;

	for (usize i = encoded.len; i-- > 0;)
		if (NTH(encoded, i) == '=') --size;
		else break;

	return size;
}

MemSlice base64_encode(MemSlice data)
{
	if (data.len == 0 || PTR(data) == nil)
		return EMPTY(MemSlice);

	usize len = base64_encoded_size(data.len);
	MemSlice out = SMAKE(umin, len + 1);
	NTH(out, len) = NUL;  //< NUL-terminate the bytes.
	--out.len;  //< Don't *count* NUL.

	for (usize i = 0, j = 0; i < data.len; i += 3, j += 4) {
		usize v = NTH(data, i);
		bool not_two_from_end = i + 2 < data.len;
		bool not_one_from_end = i + 1 < data.len;

		v = v << 8 | NTH(data, i + 1) * not_one_from_end;
		v = v << 8 | NTH(data, i + 2) * not_two_from_end;

		NTH(out, j)     = NTH(BASE64_DIGITS, (v >> 18) & 0x3F);
		NTH(out, j + 1) = NTH(BASE64_DIGITS, (v >> 12) & 0x3F);

		NTH(out, j + 2) = not_one_from_end
			? NTH(BASE64_DIGITS, (v >> 6) & 0x3F) : '=';
		NTH(out, j + 3) = not_two_from_end
			? NTH(BASE64_DIGITS,        v & 0x3F) : '=';
	}

	return out;
}

MemSlice base64_decode(MemSlice data)
{
	if (data.len == 0 || PTR(data) == nil)
		return EMPTY(MemSlice);

	foreach (digit, data)
		unless (is_base64_digit(digit))
			return EMPTY(MemSlice);

	usize len = base64_decoded_size(data);
	MemSlice out = SMAKE(umin, len + 1);
	NTH(out, len) = NUL;  //< NUL-terminate the bytes.
	--out.len;  //< Don't *count* NUL.

	for (usize i = 0, j = 0; i < data.len; i += 4, j += 3) {
		// `v` is 4 bytes.  We decode sets of 4 base-64 digits into 3 bytes.
		u32 v = NTH(BASE64_INVERSES, NTH(data, i) - '+');
		v = (v << 6) | NTH(BASE64_INVERSES, NTH(data, i + 1) - '+');

		bool not_2_from_padding = NTH(data, i + 2) != '=';
		bool not_3_from_padding = NTH(data, i + 3) != '=';

		v = v << 6 | NTH(BASE64_INVERSES, NTH(data, i + 2) - '+')
		           * not_2_from_padding;
		v = v << 6 | NTH(BASE64_INVERSES, NTH(data, i + 3) - '+')
		           * not_3_from_padding;

		NTH(out, j) = (v >> 16) & 0xFF;
		if (not_2_from_padding)
			NTH(out, j + 1) = (v >> 8) & 0xFF;
		if (not_3_from_padding)
			NTH(out, j + 2) = v & 0xFF;
	}

	return out;
}

#endif
