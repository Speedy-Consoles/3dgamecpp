#include "world_generator.hpp"

#include "shared/engine/math.hpp"
#include "shared/engine/vmath.hpp"

WorldGenerator::WorldGenerator(uint64 seed, WorldParams params) :
	wp(params),
	elevationGenerator(seed ^ 0x50a9259b7451453e, wp),
	surfacePerlin(     seed ^ 0x2e23350f66cb2335),
	vegetation_perlin( seed ^ 0xbebf64c4966b75db),
	temperature_perlin(seed ^ 0x5364424b2aa0fb15),
	cave_perlin(       seed ^ 0xca5857b732d93020),
	perlin(            seed ^ 0x3e02a6291ea49867)
{
	// nothing
}

void WorldGenerator::generateChunk(Chunk *chunk) {
	vec3i64 cc = chunk->getCC();

	const double *elevation = elevationGenerator.getChunk(vec2i64(cc[0], cc[1]));
	for (uint iccx = 0; iccx < Chunk::WIDTH; iccx++) {
		for (uint iccy = 0; iccy < Chunk::WIDTH; iccy++) {
			int64 bcx = cc[0] * Chunk::WIDTH + iccx;
			int64 bcy = cc[1] * Chunk::WIDTH + iccy;

			vec3d bc_xy(bcx, bcy, 0);
			
			double h = elevation[iccy * Chunk::WIDTH + iccx];
			double base_vegetation = 0;//vegetation_perlin.perlin(bc_xy * 1 / wp.vegetation_xy_scale);
			double base_temperature = 0;//temperature_perlin.perlin(bc_xy * 1 / wp.temperature_xy_scale);
			double base_hallowness = 0;//hollowness_perlin.perlin(bc_xy * 1 / wp.hollowness_xy_scale);

			bool solid = false;
			int realDepth = 0;
			for (int iccz = Chunk::WIDTH + 8; iccz >= 0; iccz--) {
				int64 bcz = iccz + cc[2] * Chunk::WIDTH;
				vec3d bc(bcx, bcy, bcz);

				double depth = h - bcz;

				if (depth < 0) {
					solid = false;
				} else if (depth > (h * wp.surfaceRelDepth)) {
					solid = true;
				} else {
					double funPos = (1 - depth / (h * wp.surfaceRelDepth) - 0.5) * 2 / wp.surfaceThresholdXScale;
					double threshold = (funPos + funPos * funPos * funPos) / wp.surfaceThresholdYScale + 0.5;
					double px = bcx / wp.surfaceScale;
					double py = bcy / wp.surfaceScale;
					double pz = bcz / wp.surfaceScale;
					double v = surfacePerlin.noise3(
						px, py, pz,
						wp.surfaceOctaves, wp.surfaceAmplGain, wp.surfaceFreqGain
					);
					if ((v + 1.0) / 2.0 > threshold)
						solid = true;
					else
						solid = false;
				}

				double caveness = 0;
				if (solid) {
					double mod_cave_start_depth = min(h, 0.0) + wp.cave_start_depth;
					if (bcz > mod_cave_start_depth) {
						caveness = 0;
					} else if (bcz > mod_cave_start_depth - wp.cave_border) {
						caveness = (mod_cave_start_depth - bcz) / wp.cave_border;
					} else if (bcz > wp.cave_end_depth + wp.cave_border) {
						caveness = 1;
					} else if (bcz > wp.cave_end_depth) {
						caveness = (bcz - wp.cave_end_depth) / wp.cave_border;
					} else {
						caveness = 0;
					}

					if (caveness > 0) {
						caveness *= caveness;

						double cave = 0;/*cave_perlin.octavePerlin(
							bcx / wp.cave_xy_scale,
							bcy / wp.cave_xy_scale,
							bcz / wp.cave_z_scale, wp.cave_octaves, wp.cave_exp
						);*/

						if ((cave + base_hallowness * 0.3) * caveness > wp.cave_threshold) {
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
//						else if (rel_ocean > wp.beach_threshold && realDepth < 5)
//							chunk->initBlock(icc, 4); // sand
						else if (base_temperature < wp.desert_threshold && realDepth < 5)
							chunk->initBlock(icc, 4); // sand
						else if (base_vegetation > wp.grasland_threshold && realDepth == 1)
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
