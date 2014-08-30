#ifndef STD_TYPES_HPP
#define STD_TYPES_HPP

#include <cstddef>

typedef signed char schar;
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

#if defined(_MSC_VER)
	typedef signed __int8 int8;
	typedef signed __int16 int16;
	typedef signed __int32 int32;
	typedef signed __int64 int64;
	typedef unsigned __int8 uint8;
	typedef unsigned __int16 uint16;
	typedef unsigned __int32 uint32;
	typedef unsigned __int64 uint64;
#else
	#include <cstdint>
	typedef int8_t int8;
	typedef int16_t int16;
	typedef int32_t int32;
	typedef int64_t int64;
	typedef uint8_t uint8;
	typedef uint16_t uint16;
	typedef uint32_t uint32;
	typedef uint64_t uint64;
#endif

typedef uint8 byte;

typedef float float32;
typedef double float64;

#endif /* STD_TYPES_HPP */
