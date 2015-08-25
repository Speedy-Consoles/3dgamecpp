/*
 * elevation_generator.cpp
 *
 *  Created on: Aug 25, 2015
 *      Author: speedy
 */

#include "elevation_generator.hpp"

#include <cmath>

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
		delete[] pair.second;
	}
}

const double *ElevationGenerator::getChunk(vec2i64 chunkCoords) {
	auto it = chunks.find(chunkCoords);
	if (it == chunks.end()) {
		double *chunk = new double[Chunk::SIZE];
		generateChunk(chunkCoords, chunk);
		chunks.insert({chunkCoords, chunk});
		return chunk;
	}
	return it->second;
}

void ElevationGenerator::generateChunk(vec2i64 chunkCoords, double *chunk) {
	double base[Chunk::WIDTH * Chunk::WIDTH];
	basePerlin.octavePerlin(
		chunkCoords.cast<double>() * Chunk::WIDTH / wp.elevation_xy_scale,
		vec2d(1 / wp.elevation_xy_scale),
		vec2ui(Chunk::WIDTH),
		1, 1.0, base
	);

	double relOcean[Chunk::WIDTH * Chunk::WIDTH];
	double relFlatland[Chunk::WIDTH * Chunk::WIDTH];
	double relMountain[Chunk::WIDTH * Chunk::WIDTH];

	bool needOcean = false;
	bool needFlatland = false;
	bool needMountain = false;

	for (uint i = 0; i < Chunk::WIDTH * Chunk::WIDTH; i++) {
		if (base[i] < wp.ocean_threshold) {
			relOcean[i] = 1;
			relFlatland[i] = 0;
			relMountain[i] = 0;
			needOcean = true;
		} else if (base[i] < wp.ocean_threshold + wp.ocean_stretch) {
			relMountain[i] = 0;
			relOcean[i] = 1 - (base[i] - wp.ocean_threshold) / wp.ocean_stretch;
			relOcean[i] *= relOcean[i];
			relFlatland[i] = 1 - relOcean[i];
			needOcean = true;
			needFlatland = true;
		} else if (base[i] < wp.mountain_threshold) {
			relOcean[i] = 0;
			relFlatland[i] = 1;
			relMountain[i] = 0;
			needFlatland = true;
		} else if (base[i] < wp.mountain_threshold + wp.mountain_stretch) {
			relOcean[i] = 0;
			relMountain[i] = (base[i] - wp.mountain_threshold) / wp.mountain_stretch;
			relMountain[i] *= relMountain[i];
			relFlatland[i] = 1 - relMountain[i];
			needFlatland = true;
			needMountain = true;
		} else {
			relOcean[i] = 0;
			relFlatland[i] = 0;
			relMountain[i] = 1;
			needMountain = true;
		}
	}

	double oceanHeight[Chunk::WIDTH * Chunk::WIDTH];
	double flatlandHeight[Chunk::WIDTH * Chunk::WIDTH];
	double mountainHeight[Chunk::WIDTH * Chunk::WIDTH];

	oceanPerlin.octavePerlin(
		chunkCoords.cast<double>() * Chunk::WIDTH / wp.ocean_xy_scale,
		vec2d(1 / wp.ocean_xy_scale),
		vec2ui(Chunk::WIDTH),
		wp.ocean_octaves, wp.ocean_exp, oceanHeight
	);
	flatlandPerlin.octavePerlin(
		chunkCoords.cast<double>() * Chunk::WIDTH / wp.flatland_xy_scale,
		vec2d(1 / wp.flatland_xy_scale),
		vec2ui(Chunk::WIDTH),
		wp.flatland_octaves, wp.flatland_exp, flatlandHeight
	);
	mountainPerlin.octavePerlin(
		chunkCoords.cast<double>() * Chunk::WIDTH / wp.mountain_xy_scale,
		vec2d(1 / wp.mountain_xy_scale),
		vec2ui(Chunk::WIDTH),
		wp.mountain_octaves, wp.mountain_exp, mountainHeight
	);

	for (uint i = 0; i < Chunk::WIDTH * Chunk::WIDTH; i++) {
		chunk[i] = -5.0;
		if (relOcean[i] > 0)
			chunk[i] += relOcean[i] * oceanHeight[i] * wp.ocean_depth;
		if (relFlatland[i] > 0)
			chunk[i] += relFlatland[i] * flatlandHeight[i] * wp.flatland_height;
		if (relMountain[i] > 0)
			chunk[i] += relMountain[i] * mountainHeight[i] * wp.mountain_height;
		if (std::isnan(chunk[i])) {
			printf("oceanHeight: %f\n", oceanHeight[i]);
			printf("flatlandHeight: %f\n", flatlandHeight[i]);
			printf("mountainHeight: %f\n", mountainHeight[i]);
			printf("relOcean: %f\n", relOcean[i]);
			printf("relFlatland: %f\n", relFlatland[i]);
			printf("relMountain: %f\n", relMountain[i]);
		}
	}
}
