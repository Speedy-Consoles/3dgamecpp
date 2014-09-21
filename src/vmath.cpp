/*
 * vmath.cpp
 *
 *  Created on: 21.09.2014
 *      Author: lars
 */

#include "vmath.hpp"

#include <cmath>
#include <algorithm>

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
T vec<T, N>::operator [] (size_t i) const {
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
vec<T, N> & vec<T, N>::operator *= (T rhs) {
	for (size_t i = 0; i < N; ++i)
		this->_t[i] *= rhs;
	return *this;
}

template <typename T, size_t N>
vec<T, N> & vec<T, N>::operator /= (T rhs) {
	for (size_t i = 0; i < N; ++i)
		this->_t[i] /= rhs;
	return *this;
}

template <typename T, size_t N>
vec<T, N> vec<T, N>::operator * (T rhs) const {
	vec<T, N> result;
	for (size_t i = 0; i < N; ++i)
		result._t[i] = this->_t[i] * rhs;
	return result;
}

template <typename T, size_t N>
vec<T, N> vec<T, N>::operator / (T rhs) const {
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
double vec<T, N>::norm() const {
	return sqrt(norm2());
}

template <typename T, size_t N>
T vec<T, N>::maxAbs() const {
	auto result = abs(_t[0]);
	for (size_t i = 1; i < N; i++) {
		result = std::max(result, abs(_t[i]));
	}
	return result;
}

template class vec<int, 2>;
template class vec<uint, 2>;
template class vec<int8, 2>;
template class vec<uint8, 2>;
template class vec<int64, 2>;
template class vec<uint64, 2>;
template class vec<float, 2>;
template class vec<double, 2>;

template class vec<int, 3>;
template class vec<uint, 3>;
template class vec<int8, 3>;
template class vec<uint8, 3>;
template class vec<int64, 3>;
template class vec<uint64, 3>;
template class vec<float, 3>;
template class vec<double, 3>;
