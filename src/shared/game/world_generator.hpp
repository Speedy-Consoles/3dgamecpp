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

	void generateChunk(Chunk *);

private:
	Perlin elevation_perlin;
	Perlin vegetation_perlin;
	Perlin temperature_perlin;

	Perlin mountain_perlin;
	Perlin flatland_perlin;
	Perlin ocean_perlin;
	
	Perlin hollowness_perlin;
	Perlin cave_perlin;

	Perlin perlin;
};

#endif // WORLD_GENERATOR_HPP_
