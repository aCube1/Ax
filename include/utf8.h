#ifndef _AX_UTF8_H_
#define _AX_UTF8_H_

#include "types.h"

#include <stdio.h>

#define UTF8_INVALID  UINT32_MAX
#define UTF8_MAXBYTES 4
#define UTF8_EOF      '\0'

/*!
 * Return next UTF-8 character from the provided file
 */
u32 u8_get(FILE *file);

/*!
 * Decode UTF-8 string and return Unicode character
 *
 * @param[in]  str  Pointer to UTF-8 string
 * @param[out] rune Decoded Unicode character
 *
 * @return Pointer to the end of decoded UTF-8 string
 */
const char *u8_decode(const char *str, u32 *rune);

/*!
 * Encode Unicode character into a valid UTF-8 string
 */
usize u8_encode(char *str, u32 c);

#endif
