#include "world_generator.hpp"

#include "shared/engine/math.hpp"
#include "shared/engine/vmath.hpp"

static double elevation_xy_scale   = 1000;
static double vegetation_xy_scale  = 1000;
static double temperature_xy_scale = 1500;
static double hollowness_xy_scale  = 800;

static double ocean_threshold = 0.2;
static double beach_threshold = 0.02;
static double ocean_stretch   = 0.2;
static double ocean_xy_scale  = 1000;
static double ocean_depth     = -200;
static int    ocean_octaves   = 4;
static double ocean_exp       = 0.8;

static double mountain_threshold = 0.6;
static double mountain_stretch   = 0.2;
static double mountain_xy_scale  = 500;
static double mountain_height    = 800;
static int    mountain_octaves   = 8;
static double mountain_exp       = 0.5;

static double flatland_height   = 25;
static double flatland_xy_scale = 800;
static int    flatland_octaves  = 6;
static double flatland_exp      = 0.8;

static double cave_threshold   = 0.7;
static double cave_stretch     = 0.2;
static double cave_start_depth = -100;
static double cave_end_depth   = -400;
static int    cave_octaves     = 8;
static double cave_border      = 100;
static double cave_xy_scale    = 1000;
static double cave_z_scale     = 200;
static double cave_exp         = 0.4;

static double desert_threshold = 0.3;
static double grasland_threshold = 0.5;

static double surfaceScale    = 70;
static double surfaceRelDepth = 0.3;
static double surfaceExp      = 0.4;
static double surfaceThresholdXScale = 1;
static double surfaceThresholdYScale = 1 / surfaceThresholdXScale
		+ 1 / (surfaceThresholdXScale * surfaceThresholdXScale * surfaceThresholdXScale);

WorldGenerator::WorldGenerator(uint64 seed) :
	elevation_perlin(  seed ^ 0x50a9259b7451453e),
	vegetation_perlin( seed ^ 0xbebf64c4966b75db),
	temperature_perlin(seed ^ 0x5364424b2aa0fb15),
	mountain_perlin(   seed ^ 0x9e1dfa650d2f51b3),
	flatland_perlin(   seed ^ 0xf6ed3702ee86009c),
	ocean_perlin(      seed ^ 0x3aa12a5ba619effa),
	hollowness_perlin( seed ^ 0x2e23350f66cb2335),
	cave_perlin(       seed ^ 0xca5857b732d93020),
	perlin(            seed ^ 0x3e02a6291ea49867)
{
	// nothing
}

void WorldGenerator::generateChunk(Chunk *chunk) {
	vec3i64 cc = chunk->getCC();

	for (int iccx = 0; iccx < Chunk::WIDTH; iccx++) {
		for (int iccy = 0; iccy < Chunk::WIDTH; iccy++) {
			int64 bcx = cc[0] * Chunk::WIDTH + iccx;
			int64 bcy = cc[1] * Chunk::WIDTH + iccy;

			vec3d bc_xy(bcx, bcy, 0);
			
			double base_elevation = elevation_perlin.perlin(bc_xy * 1 / elevation_xy_scale);
			double base_vegetation = vegetation_perlin.perlin(bc_xy * 1 / vegetation_xy_scale);
			double base_temperature = temperature_perlin.perlin(bc_xy * 1 / temperature_xy_scale);
			double base_hallowness = hollowness_perlin.perlin(bc_xy * 1 / hollowness_xy_scale);

			double rel_ocean, rel_flatland, rel_mountain;

			if (base_elevation < ocean_threshold) {
				rel_ocean = 1;
				rel_flatland = 0;
				rel_mountain = 0;
			} else if (base_elevation < ocean_threshold + ocean_stretch) {
				rel_mountain = 0;
				rel_ocean = 1 - (base_elevation - ocean_threshold) / ocean_stretch;
				rel_ocean *= rel_ocean;
				rel_flatland = 1 - rel_ocean;
			} else if (base_elevation < mountain_threshold) {
				rel_ocean = 0;
				rel_flatland = 1;
				rel_mountain = 0;
			} else if (base_elevation < mountain_threshold + mountain_stretch) {
				rel_ocean = 0;
				rel_mountain = (base_elevation - mountain_threshold) / mountain_stretch;
				rel_mountain *= rel_mountain;
				rel_flatland = 1 - rel_mountain;
			} else {
				rel_ocean = 0;
				rel_flatland = 0;
				rel_mountain = 1;
			}

			double ocean_h = 0, flatland_h = 0, mountain_h = 0, h = -5;

			if (rel_ocean > 0) {
				ocean_h = ocean_perlin.octavePerlin(
					bc_xy * 1 / ocean_xy_scale, ocean_octaves, ocean_exp
				);
				h += rel_ocean * ocean_h * ocean_depth;
			}
			if (rel_flatland > 0) {
				flatland_h = flatland_perlin.octavePerlin(
					bc_xy * 1 / flatland_xy_scale, flatland_octaves, flatland_exp
				);
				h += rel_flatland * flatland_h * flatland_height;
			}
			if (rel_mountain > 0) {
				mountain_h = mountain_perlin.octavePerlin(
					bc_xy * 1 / mountain_xy_scale, mountain_octaves, mountain_exp
				);
				h += rel_mountain * mountain_h * mountain_height;
			}

			bool solid = false;
			int realDepth = 0;
			for (int iccz = Chunk::WIDTH + 8; iccz >= 0; iccz--) {
				int64 bcz = iccz + cc[2] * Chunk::WIDTH;
				vec3d bc(bcx, bcy, bcz);

				double depth = h - bcz;

				if (depth < 0) {
					solid = false;
				} else if (depth > (h * surfaceRelDepth)) {
					solid = true;
				} else {
					double funPos = (1 - depth / (h * surfaceRelDepth) - 0.5) * 2 / surfaceThresholdXScale;
					double threshold = (funPos + funPos * funPos * funPos) / surfaceThresholdYScale + 0.5;
					double px = bcx / surfaceScale;
					double py = bcy / surfaceScale;
					double pz = bcz / surfaceScale;
					double v = perlin.octavePerlin(px, py, pz, 6, surfaceExp);
					if (v > threshold)
						solid = true;
					else
						solid = false;
				}

				double caveness = 0;
				if (solid) {
					double mod_cave_start_depth = min(h, 0.0) + cave_start_depth;
					if (bcz > mod_cave_start_depth) {
						caveness = 0;
					} else if (bcz > mod_cave_start_depth - cave_border) {
						caveness = (mod_cave_start_depth - bcz) / cave_border;
					} else if (bcz > cave_end_depth + cave_border) {
						caveness = 1;
					} else if (bcz > cave_end_depth) {
						caveness = (bcz - cave_end_depth) / cave_border;
					} else {
						caveness = 0;
					}

					if (caveness > 0) {
						caveness *= caveness;

						double cave = cave_perlin.octavePerlin(
							bcx / cave_xy_scale,
							bcy / cave_xy_scale,
							bcz / cave_z_scale, cave_octaves, cave_exp
						);

						if ((cave + base_hallowness * 0.3) * caveness > cave_threshold) {
							solid = false;
						}
					}
				}

				if (solid)
					realDepth++;
				else
					realDepth = 0;

				vec3ui8 icc(iccx, iccy, iccz);
				if (iccz < (int) Chunk::WIDTH) {
					if (solid) {
						if (caveness > 0)
							chunk->initBlock(icc, 3); // stone
						else if (rel_ocean > beach_threshold && realDepth < 5)
							chunk->initBlock(icc, 4); // sand
						else if (base_temperature < desert_threshold && realDepth < 5)
							chunk->initBlock(icc, 4); // sand
						else if (base_vegetation > grasland_threshold && realDepth == 1)
							chunk->initBlock(icc, 2); // gras
						else if (realDepth >= 5)
							chunk->initBlock(icc, 3); // dirt
						else
							chunk->initBlock(icc, 1); // stone
					} else {
						if (caveness > 0)
							chunk->initBlock(icc, 0);
						else if (realDepth <= 0 && bcz <= 0)
							chunk->initBlock(icc, 62); // water
						else
							chunk->initBlock(icc, 0); // air
					}
				}
			}
		}
	}

	chunk->finishInitialization();
}
