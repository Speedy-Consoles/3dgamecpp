#ifndef VMATH_HPP
#define VMATH_HPP

#include "std_types.hpp"

template <typename T> struct dotp_t_selector { typedef T dotp_t; };
template <> struct dotp_t_selector<uint8> { typedef uint dotp_t; };
template <> struct dotp_t_selector<int8> { typedef int dotp_t; };

template <typename T, size_t N>
class vec {
public:
	// constructor
	vec() = default;
	vec(T t);
	template <typename... Args> vec(Args... args);

	// assignment
	vec<T, N> & operator =  (vec<T, N> const &rhs);

	// manipulation
	void applyPW(T (*)(T));

	// access
	T         & operator [] (size_t i);
	T           operator [] (size_t i) const;

	// comparison
	bool        operator == (vec<T, N> const &rhs) const;
	bool        operator != (vec<T, N> const &rhs) const;

	// negating
	vec<T, N>   operator +  () const;
	vec<T, N>   operator -  () const;

	// addition
	vec<T, N> & operator += (vec<T, N> const &rhs);
	vec<T, N> & operator -= (vec<T, N> const &rhs);
	vec<T, N>   operator +  (vec<T, N> const &rhs) const;
	vec<T, N>   operator -  (vec<T, N> const &rhs) const;

	// multiplication
	vec<T, N> & operator *= (T);
	vec<T, N> & operator /= (T);
	vec<T, N>   operator *  (T) const;
	vec<T, N>   operator /  (T) const;

	// scalar product
	typedef typename dotp_t_selector<T>::dotp_t dotp_t;
	dotp_t      operator *  (vec<T, N> const &rhs) const;

	// query
	dotp_t norm2() const;
	double norm() const;
	T maxAbs() const;

	// cast
	template <typename T2>
	vec<T2, N> cast() const;

private:
	T _t[N];
};

template <typename T, size_t N>
vec<T, N>::vec(T t) {
	for (size_t i = 0; i < N; ++i) {
		_t[i] = t;
	}
}

template <typename T, size_t N>
template <typename... Args>
vec<T, N>::vec(Args... args)
	: _t {static_cast<T>(args)...} {
	static_assert(sizeof...(Args) == N,
	"Incorrect number of arguments passed to vec constructor");
}

template <typename T, size_t N>
template <typename T2>
vec<T2, N> vec<T, N>::cast() const {
	vec<T2, N> result;
	for (size_t i = 0; i < N; ++i) {
		result[i] = (*this)[i];
	}
	return result;
}

namespace vec_auto_cast {

template <typename T1, typename T2, size_t N>
auto operator + (vec<T1, N> const &lhs, vec<T2, N> const &rhs)
		-> decltype( vec<decltype(T1() + T2()), N>() ) {
	vec<decltype(T1() + T2()), N> result;
	for (size_t i = 0; i < N; ++i) {
		result[i] = lhs[i] + rhs[i];
	}

	return result;
}

template <typename T1, typename T2, size_t N>
auto operator - (vec<T1, N> const &lhs, vec<T2, N> const &rhs)
		-> decltype( vec<decltype(T1() - T2()), N>() ) {
	vec<decltype(T1() - T2()), N> result;
	for (size_t i = 0; i < N; ++i) {
		result[i] = lhs[i] - rhs[i];
	}

	return result;
}

} // vec_auto_cast

extern template class vec<int8, 2>;
extern template class vec<uint8, 2>;
extern template class vec<int32, 2>;
extern template class vec<uint32, 2>;
extern template class vec<int64, 2>;
extern template class vec<uint64, 2>;
extern template class vec<float, 2>;
extern template class vec<double, 2>;

extern template class vec<int8, 3>;
extern template class vec<uint8, 3>;
extern template class vec<int32, 3>;
extern template class vec<uint32, 3>;
extern template class vec<int64, 3>;
extern template class vec<uint64, 3>;
extern template class vec<float, 3>;
extern template class vec<double, 3>;

using vec3i = vec<int, 3>;
using vec3i8 = vec<int8, 3>;
using vec3ui8 = vec<uint8, 3>;
using vec3i64 = vec<int64, 3>;
using vec3f = vec<float, 3>;
using vec3d = vec<double, 3>;

using vec2i = vec<int, 2>;
using vec2f = vec<float, 2>;
using vec2d = vec<double, 2>;

#endif // VMATH_HPP
