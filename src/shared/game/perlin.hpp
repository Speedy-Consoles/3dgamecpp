#ifndef PERLIN_HPP
#define PERLIN_HPP

#include "shared/engine/vmath.hpp"

// modified version of
// https://gist.github.com/Flafla2/f0260a861be0ebdeef76

class NoiseBase {
public:
	virtual double noise3(double x, double y, double z, uint octaves, double amplGain, double freqGain) = 0;

	virtual double noise2(double x, double y, uint octaves, double amplGain, double freqGain);

	virtual void noise3(double sx, double sy, double sz, double dx, double dy, double dz,
			uint nx, uint ny, uint nz, uint octaves, double amplGain, double freqGain, double *buffer);

	virtual void noise2(double sx, double sy, double dx, double dy,
			uint nx, uint ny, uint octaves, double amplGain, double freqGain, double *buffer);

	virtual double noise3(vec3d r, int octaves, double amplGain, double freqGain) final {
		return noise3(r[0], r[1], r[2], octaves, amplGain, freqGain);
	}

	virtual void noise3(vec3d pos, vec3d stepSize, vec3ui numSteps,
			uint octaves, double amplGain, double freqGain, double *buffer) final {
		noise3(
			pos[0], pos[1], pos[2],
			stepSize[0], stepSize[1], stepSize[2],
			numSteps[0], numSteps[1], numSteps[2],
			octaves, amplGain, freqGain, buffer
		);
	}

	virtual void noise2(vec2d pos, vec2d stepSize, vec2ui numSteps,
			uint octaves, double amplGain, double freqGain, double *buffer) final {
		noise2(
			pos[0], pos[1],
			stepSize[0], stepSize[1],
			numSteps[0], numSteps[1],
			octaves, amplGain, freqGain, buffer
		);
	}
};

class Perlin : public NoiseBase {
	class Hasher {
		uint16 p[0x400];
		uint16 state = 0;

	public:
		Hasher(uint64 seed);
		inline Hasher &reset() { state = 0; return *this; }
		inline void feed(int v) { state = p[(state ^ v) & 0x3FF]; }
		inline uint16 get() const { return state; }

		inline Hasher &operator << (int i) { feed(i); return *this; }
	};

	Hasher hasher;

public:
	Perlin(uint64 seed);

	using NoiseBase::noise3;
	using NoiseBase::noise2;
	
	double noise3(double x, double y, double z, uint octaves, double amplGain, double freqGain) override;
	double noise2(double x, double y, uint octaves, double amplGain, double freqGain) override;

	void noise3(double sx, double sy, double sz, double dx, double dy, double dz,
			uint nx, uint ny, uint nz, uint octaves, double amplGain, double freqGain, double *buffer) override;
	void noise2(double sx, double sy, double dx, double dy,
			uint nx, uint ny, uint octaves, double amplGain, double freqGain, double *buffer) override;

private:
	double perlin3(double x, double y, double z, int which_octave);
	double perlin2(double x, double y, int which_octave);
	
	void perlin3(double sx, double sy, double sz, double dx, double dy, double dz,
			uint nx, uint ny, uint nz, int which_octave, double amplitude, double *buffer);
	void perlin2(double sx, double sy, double dx, double dy,
			uint nx, uint ny, int which_octave, double amplitude, double *buffer);
	
	static double grad3(uint8 hash, double x, double y, double z);
	static double grad2(uint8 hash, double x, double y);
	static double fade(double t);
	static double lerp(double a, double b, double x);
};

#endif // PERLIN_HPP
