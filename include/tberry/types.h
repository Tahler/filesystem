/*
 * Rust-like naming conventions for primitive types.
 * u8, u16, u32, u64
 * i8, i16, i32, i64
 * f32, f64
 * usize, isize
 *
 * Includes stdbool.h
 * - bool type
 * - true literal
 * - false literal
 *
 * Includes stddef.h
 * - NULL
 */

#ifndef _TBERRY_TYPES_H
#define _TBERRY_TYPES_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef size_t usize;
typedef ptrdiff_t isize;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

#endif
