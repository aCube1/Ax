// Code stolen from/inspired by:
// https://git.sr.ht/~sircmpwn/harec/tree/master/item/src/lex.c

#include "utf8.h"

static i32 u8_size(u8 c) {
	// clang-format off
	static const struct {
		u8 mask;
		u8 res;
		i32 bytes;
	} sizes[7] = {
		{ 0x80, 0x00, 1 }, // 0b0xxx_xxxx
		{ 0xe0, 0xc0, 2 }, // 0b110x_xxxx
		{ 0xf0, 0xe0, 3 }, // 0b1110_xxxx
		{ 0xf8, 0xf0, 4 }, // 0b1111_0xxx
		{ 0xfc, 0xf8, 5 }, // 0b1111_10xx
		{ 0xfe, 0xfc, 6 }, // 0b1111_1110
		{ 0x80, 0x80, -1},
	}; // clang-format on

	for (usize i = 0; i < 7; i += 1) {
		if ((c & sizes[i].mask) == sizes[i].res) {
			return sizes[i].bytes;
		}
	}

	return -1;
}

u32 u8_get(FILE *file) {
	char c = (char)fgetc(file);
	if (c == EOF) {
		return UTF8_EOF;
	}

	char buf[UTF8_MAXBYTES] = { 0 };
	buf[0] = c;

	i32 size = u8_size(c);
	if (size > UTF8_MAXBYTES) {
		fseek(file, size - 1, SEEK_CUR);
		return UTF8_INVALID;
	}

	if (size > 1) {
		// Try to read all bytes from the UTF-8 sequence
		i32 count = (i32)fread(buf + 1, sizeof(char), size - 1, file);
		if (count != size - 1) {
			return UTF8_INVALID;
		}
	}

	u32 rune;
	u8_decode(buf, &rune);
	return rune;
}

const char *u8_decode(const char *str, u32 *rune) {
	static const u8 masks[] = {
		0x7f, 0x1f, 0x0f, 0x07, 0x03, 0x01,
	};

	const u8 *out = (const u8 *)str;

	// If the first byte is ASCII, return it right away
	if (*out <= 0x7f) {
		*rune = *out;
		out += 1;
		return (const char *)out;
	}

	i32 size = u8_size(*out);
	if (size < 1) {
		*rune = UTF8_INVALID;
		out += 1; // Skip invalid byte
		return (const char *)out;
	}

	*rune = (*out) & masks[size - 1];
	out += 1; // Go to next byte

	size -= 1;
	while (size > 0) {
		u8 c = *out;
		out += 1;

		// Check if continuation octet is valid
		if ((c & 0xc0) != 0x80) {
			*rune = UTF8_INVALID;
			return (const char *)out;
		}

		// Shift codepoint bits to accommodate new octet
		*rune <<= 6;
		*rune |= c & 0x3f;

		size -= 1;
	}

	return (const char *)out;
}

usize u8_encode(char *str, u32 c) {
	// If it is ASCII, return it right away
	if (c <= 0x7f) {
		str[0] = (char)c;
		return 1;
	}

	usize len;
	u8 first; // First byte mask

	if (c <= 0x7ff) {
		len = 2;
		first = 0xc0;
	} else if (c <= 0xffff) {
		len = 3;
		first = 0xe0;
	} else {
		len = 4;
		first = 0xf0;
	}

	for (usize i = len - 1; i > 0; i -= 1) {
		str[i] = (char)((c & 0x3f) | 0x80);
		c >>= 6; // Go to the next octet
	}

	str[0] = (char)(c | first); // Store first byte
	return len;
}
