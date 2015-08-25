#ifndef ELEVATION_GENERATOR_HPP
#define ELEVATION_GENERATOR_HPP

#include <unordered_map>

#include "shared/block_utils.hpp"
#include "perlin.hpp"

struct WorldParams;

class ElevationGenerator
{
private:
	const WorldParams &wp;

	Perlin basePerlin;
	Perlin mountainPerlin;
	Perlin flatlandPerlin;
	Perlin oceanPerlin;

	std::unordered_map<vec2i64, double *, size_t(*)(vec2i64)> chunks;

public:
	ElevationGenerator(uint64 seed, const WorldParams &params);
	virtual ~ElevationGenerator();

	const double *getChunk(vec2i64 segmentCoords);

private:
	void generateChunk(vec2i64 segmentCoords, double *chunk);
};

#endif /* ELEVATION_GENERATOR_HPP */
