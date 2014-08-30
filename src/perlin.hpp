#ifndef PERLIN_HPP
#define PERLIN_HPP

#include "std_types.hpp"

// slightly modified version of
// https://gist.github.com/Flafla2/f0260a861be0ebdeef76

class Perlin {
private:
	int p[512];

public:
	Perlin(uint64 seed);

	double octavePerlin(double x, double y, double z, int octaves,
			double persistence);

	double perlin(double x, double y, double z);

private:
	static double grad(int hash, double x, double y, double z);

	static double fade(double t);

	static double lerp(double a, double b, double x);
};

#endif // PERLIN_HPP
