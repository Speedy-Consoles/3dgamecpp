/*
 * elevation_generator.cpp
 *
 *  Created on: Aug 25, 2015
 *      Author: speedy
 */

#include "elevation_generator.hpp"

#include <cmath>
#include <limits>

#include "world_generator.hpp"
#include "chunk.hpp"

ElevationGenerator::ElevationGenerator(uint64 seed, const WorldParams &params) :
		wp(params),
		basePerlin(seed),
		mountainPerlin(seed ^ 0x9e1dfa650d2f51b3),
		flatlandPerlin(seed ^ 0xf6ed3702ee86009c),
		oceanPerlin(seed ^ 0x3aa12a5ba619effa),
		chunks(0, vec2i64HashFunc) {

}

ElevationGenerator::~ElevationGenerator() {
	for (auto pair: chunks) {
		delete[] pair.second.heights;
	}
}

const ElevationChunk ElevationGenerator::getChunk(vec2i64 chunkCoords) {
	auto it = chunks.find(chunkCoords);
	if (it == chunks.end()) {
		ElevationChunk chunk;
		chunk.heights = new double[Chunk::SIZE];
		generateChunk(chunkCoords, &chunk);
		chunks.insert({chunkCoords, chunk});
		return chunk;
	}
	return it->second;
}

void ElevationGenerator::generateChunk(vec2i64 chunkCoords, ElevationChunk *chunk) {
	double base[Chunk::WIDTH * Chunk::WIDTH];
	double mountain[Chunk::WIDTH * Chunk::WIDTH];
	basePerlin.noise2(
		chunkCoords.cast<double>() * Chunk::WIDTH / wp.elevation_xy_scale / wp.overall_scale,
		vec2d(1 / wp.elevation_xy_scale / wp.overall_scale),
		vec2ui(Chunk::WIDTH),
		wp.elevation_octaves, wp.elevation_ampl_gain, wp.elevation_freq_gain, base
	);

	basePerlin.noise2(
		chunkCoords.cast<double>() * Chunk::WIDTH / wp.mountain_xy_scale / wp.overall_scale,
		vec2d(1 / wp.mountain_xy_scale / wp.overall_scale),
		vec2ui(Chunk::WIDTH),
		wp.mountain_octaves, wp.mountain_ampl_gain, wp.mountain_freq_gain, mountain
	);

	double minHeight = std::numeric_limits<double>::max();
	double maxHeight = std::numeric_limits<double>::min();
	for (uint i = 0; i < Chunk::WIDTH * Chunk::WIDTH; i++) {
		double height = base[i] * wp.elevation_z_scale * wp.overall_scale;
		height += std::pow((mountain[i] + 1.0) / 2.0, wp.mountain_exp) * wp.mountain_z_scale * wp.overall_scale;

		if (height < minHeight)
			minHeight = height;
		if (height > maxHeight)
			maxHeight = height;
		chunk->heights[i] = height;
	}
	chunk->max = maxHeight;
	chunk->min = minHeight;
}
