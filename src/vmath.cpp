/*
 * vmath.cpp
 *
 *  Created on: 21.09.2014
 *      Author: lars
 */

#include "vmath.hpp"

#include <cmath>
#include <algorithm>

template <typename T, size_t N, template <typename T> class Derived>
vec<T, N, Derived>::vec(T t) {
	for (size_t i = 0; i < N; ++i) {
		_t[i] = t;
	}
}

template <typename T, size_t N, template <typename T> class Derived>
vec<T, N, Derived>::vec(vec<T, N, Derived> const &that) {
	for (size_t i = 0; i < N; ++i) {
		_t[i] = that[i];
	}
}

template <typename T, size_t N, template <typename T> class Derived>
Derived<T> & vec<T, N, Derived>::operator = (vec<T, N, Derived> const &rhs) {
	for (size_t i = 0; i < N; ++i) {
		this->_t[i] = rhs._t[i];
	}
	return *static_cast<Derived *>(this);
}

template <typename T, size_t N, template <typename T> class Derived>
void vec<T, N, Derived>::applyPW(T (*cb)(T)) {
	for (size_t i = 0; i < N; i++) {
		_t[i] = cb(_t[i]);
	}
}

template <typename T, size_t N, template <typename T> class Derived>
T & vec<T, N, Derived>::operator [] (size_t i) {
	return _t[i];
}

template <typename T, size_t N, template <typename T> class Derived>
T vec<T, N, Derived>::operator [] (size_t i) const {
	return _t[i];
}

template <typename T, size_t N, template <typename T> class Derived>
bool vec<T, N, Derived>::operator == (vec<T, N, Derived> const &rhs) const {
	for (size_t i = 0; i < N; ++i)
		if (this->_t[i] != rhs._t[i]) return false;
	return true;
}

template <typename T, size_t N, template <typename T> class Derived>
bool vec<T, N, Derived>::operator != (vec<T, N, Derived> const &rhs) const {
	return !this->operator==(rhs);
}

template <typename T, size_t N, template <typename T> class Derived>
Derived<T> vec<T, N, Derived>::operator + () const {
	return *static_cast<Derived *>(this);
}

template <typename T, size_t N, template <typename T> class Derived>
Derived<T> vec<T, N, Derived>::operator - () const {
	Derived result;
	for (size_t i = 0; i < N; ++i)
		result._t[i] = -this->_t[i];
	return result;
}

template <typename T, size_t N, template <typename T> class Derived>
Derived<T> & vec<T, N, Derived>::operator += (vec<T, N, Derived> const &rhs) {
	for (size_t i = 0; i < N; ++i)
		this->_t[i] += rhs._t[i];
	return *static_cast<Derived *>(this);
}

template <typename T, size_t N, template <typename T> class Derived>
Derived<T> & vec<T, N, Derived>::operator -= (vec<T, N, Derived> const &rhs) {
	for (size_t i = 0; i < N; ++i)
		this->_t[i] -= rhs._t[i];
	return *static_cast<Derived *>(this);
}

template <typename T, size_t N, template <typename T> class Derived>
Derived<T> vec<T, N, Derived>::operator + (vec<T, N, Derived> const &rhs) const {
	Derived result;
	for (size_t i = 0; i < N; ++i)
		result._t[i] = this->_t[i] + rhs._t[i];
	return result;
}

template <typename T, size_t N, template <typename T> class Derived>
Derived<T> vec<T, N, Derived>::operator - (vec<T, N, Derived> const &rhs) const {
	Derived result;
	for (size_t i = 0; i < N; ++i)
		result._t[i] = this->_t[i] - rhs._t[i];
	return result;
}

template <typename T, size_t N, template <typename T> class Derived>
Derived<T> & vec<T, N, Derived>::operator *= (T rhs) {
	for (size_t i = 0; i < N; ++i)
		this->_t[i] *= rhs;
	return *static_cast<Derived *>(this);
}

template <typename T, size_t N, template <typename T> class Derived>
Derived<T> & vec<T, N, Derived>::operator /= (T rhs) {
	for (size_t i = 0; i < N; ++i)
		this->_t[i] /= rhs;
	return *static_cast<Derived *>(this);
}

template <typename T, size_t N, template <typename T> class Derived>
Derived<T> vec<T, N, Derived>::operator * (T rhs) const {
	Derived result;
	for (size_t i = 0; i < N; ++i)
		result._t[i] = this->_t[i] * rhs;
	return result;
}

template <typename T, size_t N, template <typename T> class Derived>
Derived<T> vec<T, N, Derived>::operator / (T rhs) const {
	Derived result;
	for (size_t i = 0; i < N; ++i)
		result._t[i] = this->_t[i] / rhs;
	return result;
}

template <typename T, size_t N, template <typename T> class Derived>
auto vec<T, N, Derived>::operator * (vec<T, N, Derived> const &rhs) const -> dotp_t {
	dotp_t result = 0;

	for (size_t i = 0; i < N; ++i)
		result += this->_t[i] * rhs._t[i];

	return result;
}

template <typename T, size_t N, template <typename T> class Derived>
auto vec<T, N, Derived>::norm2() const -> dotp_t {
	return this->operator*(*this);
}

template <typename T, size_t N, template <typename T> class Derived>
double vec<T, N, Derived>::norm() const {
	return sqrt(norm2());
}

template <typename T, size_t N, template <typename T> class Derived>
T vec<T, N, Derived>::maxAbs() const {
	auto result = abs(_t[0]);
	for (size_t i = 1; i < N; i++) {
		result = std::max(result, abs(_t[i]));
	}
	return result;
}

template <typename T, size_t N, template <typename T> class Derived>
template <typename S>
Derived<S> vec<T, N, Derived>::cast() const {
	Derived<S> result;
	for (size_t i = 0; i < N; ++i) {
		result._t[i] = static_cast<S>(_t[i]);
	}
	return result;
}

// specialized versions for 2 and 3 components

template <typename T>
vec2<T>::vec2(T t1, T t2) {
	_t[0] = t1;
	_t[1] = t2;
}

template <typename T>
vec3<T>::vec3(T t1, T t2, T t3) {
	_t[0] = t1;
	_t[1] = t2;
	_t[2] = t3;
}

template <typename T>
vec4<T>::vec4(T t1, T t2, T t3, T t4) {
	_t[0] = t1;
	_t[1] = t2;
	_t[2] = t3;
	_t[3] = t4;
}

// instanciation

template class vec2<int8>;
template class vec2<uint8>;
template class vec2<int32>;
template class vec2<uint32>;
template class vec2<int64>;
template class vec2<uint64>;
template class vec2<float>;
template class vec2<double>;

template class vec3<int8>;
template class vec3<uint8>;
template class vec3<int32>;
template class vec3<uint32>;
template class vec3<int64>;
template class vec3<uint64>;
template class vec3<float>;
template class vec3<double>;

template class vec4<int8>;
template class vec4<uint8>;
template class vec4<int32>;
template class vec4<uint32>;
template class vec4<int64>;
template class vec4<uint64>;
template class vec4<float>;
template class vec4<double>;
