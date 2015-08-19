#ifndef MATH_HPP_
#define MATH_HPP_

#include <cmath>
#include <type_traits>
#include <algorithm>

#include "std_types.hpp"

static const double TAU = atan(1) * 8;

template<typename T> inline
T clamp(T v, T min, T max) {
    if (v != v)
        return v;
    if (v < min)
        return min;
    if (v > max)
        return max;
    return v;
}

template <typename T1, typename T2> inline
typename std::enable_if<
	std::is_integral<T1>::value && std::is_integral<T2>::value,
	T2
>::type
cycle(T1 i, T2 range) {
    return (i % range + range) % range;
}

template <typename T> inline
typename std::enable_if<std::is_floating_point<T>::value, T>::type
cycle(T d, T range) {
    return d - std::floor(d / range) * range;
}

using std::min;
using std::max;

#endif // MATH_HPP_
