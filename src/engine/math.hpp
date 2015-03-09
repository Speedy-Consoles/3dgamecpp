#ifndef MATH_HPP_
#define MATH_HPP_

#include <cmath>
#include <type_traits>

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

template <typename T> inline
typename std::enable_if<std::is_integral<T>::value, T>::type
cycle(T i, T range) {
    return (i % range + range) % range;
}

template <typename T> inline
typename std::enable_if<std::is_floating_point<T>::value, T>::type
cycle(T i, T range) {
    return d - std::floor(d / range) * range;
}

#endif // MATH_HPP_
