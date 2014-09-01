#ifndef VMATH_HPP
#define VMATH_HPP

#include <cmath>
#include <type_traits>
#include "std_types.hpp"
#include <algorithm>

template <typename T> struct dotp_t_selector { typedef T dotp_t; };
template <> struct dotp_t_selector<uint8> { typedef uint dotp_t; };
template <> struct dotp_t_selector<int8> { typedef int dotp_t; };

template <typename T, size_t N>
class vec {
public:

	// some arguments are either (const T &) or (T) depending on what T is
	typedef typename std::conditional<
	std::is_arithmetic<T>::value, T, const T &>::type ConstRefT;

	// constructor
	vec() = default;
	template <typename... Args> vec(Args... args);
	vec(T t);

	// assignment
	vec<T, N> & operator =  (vec<T, N> const &rhs);

	// manipulation
	void applyPW(T (*)(T));

	// access
	T         & operator [] (size_t i);
	ConstRefT   operator [] (size_t i) const;

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
	vec<T, N> & operator *= (ConstRefT);
	vec<T, N> & operator /= (ConstRefT);
	vec<T, N>   operator *  (ConstRefT) const;
	vec<T, N>   operator /  (ConstRefT) const;

	// scalar product
	typedef typename dotp_t_selector<T>::dotp_t dotp_t;
	dotp_t      operator *  (vec<T, N> const &rhs) const;

	// query
	dotp_t norm2() const;
	auto norm() const -> decltype(sqrt(dotp_t()));
	auto maxAbs() const -> decltype(abs(T()));

	// cast
	template <typename T2>
	vec<T2, N> cast() const;

private:
	T _t[N];
};

template <typename T, size_t N>
template <typename... Args>
vec<T, N>::vec(Args... args)
	: _t {static_cast<T>(args)...} {
	static_assert(sizeof...(Args) == N,
	"Incorrect number of arguments passed to vec constructor");
}
template <typename T, size_t N>
vec<T, N>::vec(T t) {
	for (size_t i = 0; i < N; ++i) {
		_t[i] = t;
	}
}

template <typename T, size_t N>
vec<T, N> & vec<T, N>::operator = (vec<T, N> const &rhs) {
	for (size_t i = 0; i < N; ++i) {
		this->_t[i] = rhs._t[i];
	}
	return *this;
}

template <typename T, size_t N>
void vec<T, N>::applyPW(T (*cb)(T)) {
	for (size_t i = 0; i < N; i++) {
		_t[i] = cb(_t[i]);
	}
}

template <typename T, size_t N>
T & vec<T, N>::operator [] (size_t i) {
	return _t[i];
}

template <typename T, size_t N>
typename vec<T, N>::ConstRefT vec<T, N>::operator [] (size_t i) const {
	return _t[i];
}

template <typename T, size_t N>
bool vec<T, N>::operator == (vec<T, N> const &rhs) const {
	for (size_t i = 0; i < N; ++i)
		if (this->_t[i] != rhs._t[i]) return false;
	return true;
}

template <typename T, size_t N>
bool vec<T, N>::operator != (vec<T, N> const &rhs) const {
	return !this->operator==(rhs);
}

template <typename T, size_t N>
vec<T, N> vec<T, N>::operator + () const {
	return *this;
}

template <typename T, size_t N>
vec<T, N> vec<T, N>::operator - () const {
	vec<T, N> result;
	for (size_t i = 0; i < N; ++i)
		result._t[i] = -this->_t[i];
	return result;
}

template <typename T, size_t N>
vec<T, N> & vec<T, N>::operator += (vec<T, N> const &rhs) {
	for (size_t i = 0; i < N; ++i)
		this->_t[i] += rhs._t[i];
	return *this;
}

template <typename T, size_t N>
vec<T, N> & vec<T, N>::operator -= (vec<T, N> const &rhs) {
	for (size_t i = 0; i < N; ++i)
		this->_t[i] -= rhs._t[i];
	return *this;
}

template <typename T, size_t N>
vec<T, N> vec<T, N>::operator + (vec<T, N> const &rhs) const {
	vec<T, N> result;
	for (size_t i = 0; i < N; ++i)
		result._t[i] = this->_t[i] + rhs._t[i];
	return result;
}

template <typename T, size_t N>
vec<T, N> vec<T, N>::operator - (vec<T, N> const &rhs) const {
	vec<T, N> result;
	for (size_t i = 0; i < N; ++i)
		result._t[i] = this->_t[i] - rhs._t[i];
	return result;
}

template <typename T, size_t N>
vec<T, N> & vec<T, N>::operator *= (ConstRefT rhs) {
	for (size_t i = 0; i < N; ++i)
		this->_t[i] *= rhs;
	return *this;
}

template <typename T, size_t N>
vec<T, N> & vec<T, N>::operator /= (ConstRefT rhs) {
	for (size_t i = 0; i < N; ++i)
		this->_t[i] /= rhs;
	return *this;
}

template <typename T, size_t N>
vec<T, N> vec<T, N>::operator * (ConstRefT rhs) const {
	vec<T, N> result;
	for (size_t i = 0; i < N; ++i)
		result._t[i] = this->_t[i] * rhs;
	return result;
}

template <typename T, size_t N>
vec<T, N> vec<T, N>::operator / (ConstRefT rhs) const {
	vec<T, N> result;
	for (size_t i = 0; i < N; ++i)
		result._t[i] = this->_t[i] / rhs;
	return result;
}

template <typename T, size_t N>
auto vec<T, N>::operator * (vec<T, N> const &rhs) const -> dotp_t {
	dotp_t result = 0;

	for (size_t i = 0; i < N; ++i)
		result += this->_t[i] * rhs._t[i];

	return result;
}

template <typename T, size_t N>
auto vec<T, N>::norm2() const -> dotp_t {
	return this->operator*(*this);
}

template <typename T, size_t N>
auto vec<T, N>::norm() const -> decltype(sqrt(dotp_t())) {
	return sqrt(norm2());
}

template <typename T, size_t N>
auto vec<T, N>::maxAbs() const -> decltype(abs(T())) {
	auto result = abs(_t[0]);
	for (size_t i = 1; i < N; i++) {
		result = std::max(result, abs(_t[i]));
	}
	return result;
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

using vec3i = vec<int, 3>;
using vec3i8 = vec<int8, 3>;
using vec3ui8 = vec<uint8, 3>;
using vec3i64 = vec<int64, 3>;
using vec3d = vec<double, 3>;

using vec2i = vec<int, 2>;
using vec2d = vec<double, 2>;

#endif // VMATH_HPP
