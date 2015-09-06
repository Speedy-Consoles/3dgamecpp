#ifndef RANDOM_HPP_
#define RANDOM_HPP_

#include <random>
#include <limits>
#include <boost/random/uniform_int_distribution.hpp>

using boost::random::uniform_int_distribution;

template <class T, class RNG>
void shuffle(T begin, T end, RNG &rng) {
	size_t n = end - begin;
	T iter = begin;
	for (size_t i = 0; i < n; ++i, ++iter) {
		uniform_int_distribution<size_t> distr(i, n - 1);
		size_t val = distr(rng);
		std::swap(*(begin + i), *(begin + val));
	}
};

#endif // RANDOM_HPP_
