#ifndef PERLIN_HPP
#define PERLIN_HPP

#include "shared/engine/vmath.hpp"

// slightly modified version of
// https://gist.github.com/Flafla2/f0260a861be0ebdeef76

class Hasher {
	int p[512];
	int state;

public:
	Hasher(uint64 seed);
	Hasher &reset() { state = 0; return *this; }
	void feed(int i) { state = p[state + i]; }
	int get() const { return state; }
};

class Perlin {
	Hasher hasher;

public:
	Perlin(uint64 seed);

	double octavePerlin(double x, double y, double z, int octaves, double persistence);
	double perlin(double x, double y, double z);
	
	double octavePerlin(vec3d r, int octaves, double persistence) {
		return octavePerlin(r[0], r[1], r[2], octaves, persistence);
	}
	double perlin(vec3d r) {
		return perlin(r[0], r[1], r[2]);
	}

private:
	static double grad(int hash, double x, double y, double z);
	static double fade(double t);
	static double lerp(double a, double b, double x);
};

#endif // PERLIN_HPP
