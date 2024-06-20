#ifndef _AX_TYPES_H_
#define _AX_TYPES_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef enum TypeStorage {
	// Primitives (Sorted)
	TYPE_BOOL,
	TYPE_F32,
	TYPE_F64,
	TYPE_I16,
	TYPE_I32,
	TYPE_I64,
	TYPE_I8,
	TYPE_U16,
	TYPE_U32,
	TYPE_U64,
	TYPE_U8,
	TYPE_VOID,

	// Misc
	TYPE_FUNC,
	TYPE_POINTER,
	TYPE_ARRAY,

	// Compile constants
	TYPE_INT,
	TYPE_FLOAT,
	TYPE_RUNE,
	TYPE_STRING,
} TypeStorage;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef size_t usize;
typedef ssize_t isize;

typedef float f32;
typedef double f64;

#endif
