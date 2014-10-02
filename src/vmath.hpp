#ifndef VMATH_HPP
#define VMATH_HPP

#include "std_types.hpp"

template <typename T> struct dotp_t_selector { typedef T dotp_t; };
template <> struct dotp_t_selector<uint8> { typedef uint dotp_t; };
template <> struct dotp_t_selector<int8> { typedef int dotp_t; };

template <typename T, size_t N, template <typename> class Derived>
class vec {
public:
	// constructor
	vec() = default;
	vec(T t);
	vec(vec<T, N, Derived> const &rhs);

	// assignment
	Derived<T> & operator =  (vec<T, N, Derived> const &rhs);

	// manipulation
	void applyPW(T (*)(T));

	// access
	T          & operator [] (size_t i);
	T            operator [] (size_t i) const;

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
	vec2(T t) : vec<T, 2, ::vec2>::vec(t) {}
	vec2(T t1, T t2);
};

template <typename T>
class vec3 : public vec<T, 3, vec3> {
public:
	vec3() = default;
	vec3(T t) : vec<T, 3, ::vec3>::vec(t) {};
	vec3(T t1, T t2, T t3);
};

// instanciation

extern template class vec2<int8>;
extern template class vec2<uint8>;
extern template class vec2<int32>;
extern template class vec2<uint32>;
extern template class vec2<int64>;
extern template class vec2<uint64>;
extern template class vec2<float>;
extern template class vec2<double>;

extern template class vec3<int8>;
extern template class vec3<uint8>;
extern template class vec3<int32>;
extern template class vec3<uint32>;
extern template class vec3<int64>;
extern template class vec3<uint64>;
extern template class vec3<float>;
extern template class vec3<double>;

using vec3i = vec3<int>;
using vec3i8 = vec3<int8>;
using vec3ui8 = vec3<uint8>;
using vec3i64 = vec3<int64>;
using vec3f = vec3<float>;
using vec3d = vec3<double>;

using vec2i = vec2<int>;
using vec2f = vec2<float>;
using vec2d = vec2<double>;

#endif // VMATH_HPP
