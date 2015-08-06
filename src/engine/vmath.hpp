#ifndef VMATH_HPP
#define VMATH_HPP

#include "std_types.hpp"

#include <cmath>
#include <algorithm>

template <typename T> struct dotp_t_selector { typedef T dotp_t; };
template <> struct dotp_t_selector<uint8> { typedef uint dotp_t; };
template <> struct dotp_t_selector<int8> { typedef int dotp_t; };

template <typename T, size_t N, template <typename> class Derived>
class vec {
public:
	// constructor
	vec() = default;
	explicit vec(T t);

	// manipulation
	void applyPW(T (*)(T));

	// access
	T          & operator [] (size_t i);
	T            operator [] (size_t i) const;
	T          * ptr() { return _t; }
	const T    * ptr() const { return _t; }

	// comparison
	bool         operator == (vec<T, N, Derived> const &rhs) const;
	bool         operator != (vec<T, N, Derived> const &rhs) const;

	// negating
	Derived<T>   operator +  () const;
	Derived<T>   operator -  () const;

	// addition
	Derived<T> & operator += (vec<T, N, Derived> const &rhs);
	Derived<T> & operator -= (vec<T, N, Derived> const &rhs);
	Derived<T>   operator +  (vec<T, N, Derived> const &rhs) const;
	Derived<T>   operator -  (vec<T, N, Derived> const &rhs) const;

	// multiplication
	Derived<T> & operator *= (T);
	Derived<T> & operator /= (T);
	Derived<T>   operator *  (T) const;
	Derived<T>   operator /  (T) const;

	// scalar product
	typedef typename dotp_t_selector<T>::dotp_t dotp_t;
	dotp_t    operator *  (vec<T, N, Derived> const &rhs) const;

	// query
	dotp_t norm2() const;
	double norm() const;
	T maxAbs() const;

	// cast
	template <typename S>
	Derived<S> cast() const;

protected:
	T _t[N];
};

// template specializations for 2 and 3 components

template <typename T>
class vec2 : public vec<T, 2, vec2> {
public:
	vec2() = default;
	explicit vec2(T t) : vec<T, 2, ::vec2>::vec(t) {}
	vec2(T t1, T t2);
};

template <typename T>
class vec3 : public vec<T, 3, vec3> {
public:
	vec3() = default;
	explicit vec3(T t) : vec<T, 3, ::vec3>::vec(t) {};
	vec3(T t1, T t2, T t3);
};

template <typename T>
class vec4 : public vec<T, 4, vec4> {
public:
	vec4() = default;
	explicit vec4(T t) : vec<T, 4, ::vec4>::vec(t) {};
	vec4(T t1, T t2, T t3, T t4);
};

// methods

template <typename T, size_t N, template <typename T> class Derived>
vec<T, N, Derived>::vec(T t) {
	for (size_t i = 0; i < N; ++i) {
		_t[i] = t;
	}
}

template <typename T, size_t N, template <typename T> class Derived>
void vec<T, N, Derived>::applyPW(T(*cb)(T)) {
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
	return *static_cast<Derived<T> *>(this);
}

template <typename T, size_t N, template <typename T> class Derived>
Derived<T> vec<T, N, Derived>::operator - () const {
	Derived<T> result;
	for (size_t i = 0; i < N; ++i)
		result._t[i] = -this->_t[i];
	return result;
}

template <typename T, size_t N, template <typename T> class Derived>
Derived<T> & vec<T, N, Derived>::operator += (vec<T, N, Derived> const &rhs) {
	for (size_t i = 0; i < N; ++i)
		this->_t[i] += rhs._t[i];
	return *static_cast<Derived<T> *>(this);
}

template <typename T, size_t N, template <typename T> class Derived>
Derived<T> & vec<T, N, Derived>::operator -= (vec<T, N, Derived> const &rhs) {
	for (size_t i = 0; i < N; ++i)
		this->_t[i] -= rhs._t[i];
	return *static_cast<Derived<T> *>(this);
}

template <typename T, size_t N, template <typename T> class Derived>
Derived<T> vec<T, N, Derived>::operator + (vec<T, N, Derived> const &rhs) const {
	Derived<T> result;
	for (size_t i = 0; i < N; ++i)
		result._t[i] = this->_t[i] + rhs._t[i];
	return result;
}

template <typename T, size_t N, template <typename T> class Derived>
Derived<T> vec<T, N, Derived>::operator - (vec<T, N, Derived> const &rhs) const {
	Derived<T> result;
	for (size_t i = 0; i < N; ++i)
		result._t[i] = this->_t[i] - rhs._t[i];
	return result;
}

template <typename T, size_t N, template <typename T> class Derived>
Derived<T> & vec<T, N, Derived>::operator *= (T rhs) {
	for (size_t i = 0; i < N; ++i)
		this->_t[i] *= rhs;
	return *static_cast<Derived<T> *>(this);
}

template <typename T, size_t N, template <typename T> class Derived>
Derived<T> & vec<T, N, Derived>::operator /= (T rhs) {
	for (size_t i = 0; i < N; ++i)
		this->_t[i] /= rhs;
	return *static_cast<Derived<T> *>(this);
}

template <typename T, size_t N, template <typename T> class Derived>
Derived<T> vec<T, N, Derived>::operator * (T rhs) const {
	Derived<T> result;
	for (size_t i = 0; i < N; ++i)
		result._t[i] = this->_t[i] * rhs;
	return result;
}

template <typename T, size_t N, template <typename T> class Derived>
Derived<T> vec<T, N, Derived>::operator / (T rhs) const {
	Derived<T> result;
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
		result[i] = static_cast<S>(_t[i]);
	}
	return result;
}

// specialized versions for 2 and 3 components

template <typename T>
vec2<T>::vec2(T t1, T t2) {
	this->_t[0] = t1;
	this->_t[1] = t2;
}

template <typename T>
vec3<T>::vec3(T t1, T t2, T t3) {
	this->_t[0] = t1;
	this->_t[1] = t2;
	this->_t[2] = t3;
}

template <typename T>
vec4<T>::vec4(T t1, T t2, T t3, T t4) {
	this->_t[0] = t1;
	this->_t[1] = t2;
	this->_t[2] = t3;
	this->_t[3] = t4;
}

// abbreviations

using vec2i = vec2<int>;
using vec2f = vec2<float>;
using vec2d = vec2<double>;

using vec3i = vec3<int>;
using vec3i8 = vec3<int8>;
using vec3ui8 = vec3<uint8>;
using vec3i64 = vec3<int64>;
using vec3f = vec3<float>;
using vec3d = vec3<double>;

using vec4f = vec4<float>;
using vec4d = vec4<double>;

#endif // VMATH_HPP
