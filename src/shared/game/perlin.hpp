#ifndef PERLIN_HPP
#define PERLIN_HPP

#include "shared/engine/vmath.hpp"

// slightly modified version of
// https://gist.github.com/Flafla2/f0260a861be0ebdeef76

class Hasher {
	int p[0x100];
	int state;

public:
	Hasher(uint64 seed);
	Hasher &reset() { state = 0; return *this; }
	void feed(int i) { state = p[(state + i) & 0xFF]; }
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

	void octavePerlin(
		double startX, double startY, double startZ,
		double stepSizeX, double stepSizeY, double stepSizeZ,
		uint numStepsX, uint numStepsY, uint numStepsZ,
		uint octaves, double persistence, double *buffer
	);
	void octavePerlin(
		double startX, double startY,
		double stepSizeX, double stepSizeY,
		uint numStepsX, uint numStepsY,
		uint octaves, double persistence, double *buffer
	);

	void octavePerlin(vec3d pos, vec3d stepSize, vec3ui numSteps,
			uint octaves, double persistence, double *buffer) {
		octavePerlin(
			pos[0], pos[1], pos[2],
			stepSize[0], stepSize[1], stepSize[2],
			numSteps[0], numSteps[1], numSteps[2],
			octaves, persistence, buffer
		);
	}

	void octavePerlin(vec2d pos, vec2d stepSize, vec2ui numSteps,
			uint octaves, double persistence, double *buffer) {
		octavePerlin(
			pos[0], pos[1],
			stepSize[0], stepSize[1],
			numSteps[0], numSteps[1],
			octaves, persistence, buffer
		);
	}

private:
	static double grad(int hash, double x, double y, double z);
	static double fade(double t);
	static double lerp(double a, double b, double x);
};

#endif // PERLIN_HPP
