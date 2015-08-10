/*
 * world_generator.hpp
 *
 *  Created on: 17.09.2014
 *      Author: lars
 */

#ifndef WORLD_GENERATOR_HPP_
#define WORLD_GENERATOR_HPP_

#include "shared/engine/macros.hpp"

#include "perlin.hpp"
#include "chunk.hpp"

class WorldGenerator {
public:
	WorldGenerator(uint64 seed);

	DEPRECATED(Chunk *generateChunk(vec3i64 cc));
	void generateChunk(vec3i64 cc, Chunk &);
	void generateChunk(Chunk &);

private:
	Perlin perlin;

	double overAllScale = 1;

	double areaXYScale = 6000;
	double areaMountainThreshold = 0.7;
	double areaSharpness = 20;

	double mountainXYScale = 500;
	double mountainMaxHeight = 800;
	int mountainOctaves = 8;
	double mountainExp = 0.5;

	double flatlandXYScale = 800;
	double flatLandMaxHeight = 40;
	int flatlandOctaves = 6;
	double flatlandExp = 0.8;

	double surfaceScale = 70;
	double surfaceRelDepth = 0.3;
	double surfaceExp = 0.4;
	double surfaceThresholdXScale = 1;
	double surfaceThresholdYScale;
};

#endif // WORLD_GENERATOR_HPP_
