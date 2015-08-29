#ifndef ELEVATION_GENERATOR_HPP
#define ELEVATION_GENERATOR_HPP

#include <unordered_map>

#include "shared/block_utils.hpp"
#include "perlin.hpp"

struct WorldParams;

struct ElevationChunk {
	double min = 0;
	double max = 0;
	double *heights = nullptr;
};

class ElevationGenerator
{
private:
	const WorldParams &wp;

	Perlin basePerlin;
	Perlin mountainPerlin;
	Perlin flatlandPerlin;
	Perlin oceanPerlin;

	std::unordered_map<vec2i64, ElevationChunk, size_t(*)(vec2i64)> chunks;

public:
	ElevationGenerator(uint64 seed, const WorldParams &params);
	virtual ~ElevationGenerator();

	const ElevationChunk getChunk(vec2i64 segmentCoords);

private:
	void generateChunk(vec2i64 segmentCoords, ElevationChunk *chunk);
};

#endif /* ELEVATION_GENERATOR_HPP */
