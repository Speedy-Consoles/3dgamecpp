#ifndef RANDOM_HPP_
#define RANDOM_HPP_

#include <random>
#include <limits>

template <typename T>
class uniform_int_distribution {
	T min, max;
public:
	using result_type = T;

	uniform_int_distribution(result_type min = 0,
			result_type max = std::numeric_limits<result_type>::max()) :
			min(min), max(max) {}
	
	result_type a() const { return min; }
	result_type b() const { return max; }

	template <typename RNG>
	result_type operator () (RNG& rng) {
		using rng_type = typename RNG::result_type;

		if (max < min)
			return (result_type)rng();

		rng_type n = max - min + 1;
		rng_type remainder = rng.max() % n;
		rng_type x;
		do
		{
			x = rng();
		} while (x >= rng.max() - remainder);
		rng_type result = min + x % n;
		return (result_type)result;
	}
};

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
