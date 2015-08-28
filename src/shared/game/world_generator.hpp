/*
 * world_generator.hpp
 *
 *  Created on: 17.09.2014
 *      Author: lars
 */

#ifndef WORLD_GENERATOR_HPP_
#define WORLD_GENERATOR_HPP_

#include "shared/engine/macros.hpp"

#include "elevation_generator.hpp"
#include "perlin.hpp"
#include "chunk.hpp"

class ElevationGenerator;

struct WorldParams {
	double overall_scale = 1;

	double elevation_xy_scale  = 100000;
	double elevation_z_scale   = 100;
	double elevation_octaves   = 6;
	double elevation_ampl_gain = 0.4;
	double elevation_freq_gain = 5.0;

	double mountain_xy_scale  = 5000;
	double mountain_z_scale   = 6000;
	double mountain_octaves   = 7;
	double mountain_ampl_gain = 0.4;
	double mountain_freq_gain = 2.5;
	double mountain_exp       = 12;

	double surfaceScale           = 70;
	double surfaceRelDepth        = 0.3;
	double surfaceOctaves         = 6;
	double surfaceAmplGain        = 0.4;
	double surfaceFreqGain        = 2.0;
	double surfaceThresholdXScale = 1;

	double vegetation_xy_scale  = 1000;
	double temperature_xy_scale = 1500;
	double hollowness_xy_scale  = 800;

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
};

class WorldGenerator {
public:
	WorldGenerator(uint64 seed, WorldParams params);

	void generateChunk(Chunk *);

private:
	WorldParams wp;

	ElevationGenerator elevationGenerator;
	Perlin vegetation_perlin;
	Perlin temperature_perlin;
	
	Perlin surfacePerlin;
	Perlin cave_perlin;

	Perlin perlin;
};

#endif // WORLD_GENERATOR_HPP_
