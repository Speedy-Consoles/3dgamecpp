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

struct WorldParams {
	double elevation_xy_scale   = 1000;
	double vegetation_xy_scale  = 1000;
	double temperature_xy_scale = 1500;
	double hollowness_xy_scale  = 800;

	double ocean_threshold = 0.2;
	double beach_threshold = 0.02;
	double ocean_stretch   = 0.2;
	double ocean_xy_scale  = 1000;
	double ocean_depth     = -200;
	int    ocean_octaves   = 4;
	double ocean_exp       = 0.8;

	double mountain_threshold = 0.6;
	double mountain_stretch   = 0.2;
	double mountain_xy_scale  = 500;
	double mountain_height    = 800;
	int    mountain_octaves   = 8;
	double mountain_exp       = 0.5;

	double flatland_height   = 25;
	double flatland_xy_scale = 800;
	int    flatland_octaves  = 6;
	double flatland_exp      = 0.8;

	double cave_threshold   = 0.7;
	double cave_stretch     = 0.2;
	double cave_start_depth = -100;
	double cave_end_depth   = -400;
	int    cave_octaves     = 8;
	double cave_border      = 100;
	double cave_xy_scale    = 1000;
	double cave_z_scale     = 200;
	double cave_exp         = 0.4;

	double desert_threshold = 0.3;
	double grasland_threshold = 0.5;

	double surfaceScale    = 70;
	double surfaceRelDepth = 0.3;
	double surfaceExp      = 0.4;
	double surfaceThresholdXScale = 1;
	double surfaceThresholdYScale = 1 / surfaceThresholdXScale
			+ 1 / (surfaceThresholdXScale * surfaceThresholdXScale * surfaceThresholdXScale);
};

class WorldGenerator {
public:
	WorldGenerator(uint64 seed, WorldParams params);

	void generateChunk(Chunk *);

private:
	WorldParams wp;

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
