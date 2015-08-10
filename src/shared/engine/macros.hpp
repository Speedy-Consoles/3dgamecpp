#ifndef MACROS_HPP_
#define MACROS_HPP_

#ifdef __GNUC__
#define DEPRECATED(func) func __attribute__ ((deprecated))
#elif defined(_MSC_VER)
#define DEPRECATED(func) __declspec(deprecated) func
#else
#pragma message("WARNING: You need to implement DEPRECATED for this compiler")
#define DEPRECATED(func) func
#endif

#ifdef __GNUC__
#define PACKED(func) func __attribute__((__packed__))
#elif defined(_MSC_VER)
#define PACKED(func) __pragma(pack(push, 1)) func __pragma(pack(pop))
#else
#pragma message("WARNING: You need to implement PACKED for this compiler")
#define PACKED(func) func
#endif

#endif // MACROS_HPP_
