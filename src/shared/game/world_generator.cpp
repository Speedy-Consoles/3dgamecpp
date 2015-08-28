#include "world_generator.hpp"

#include "shared/engine/math.hpp"
#include "shared/engine/vmath.hpp"

WorldGenerator::WorldGenerator(uint64 seed, WorldParams params) :
	wp(params),
	elevationGenerator(seed ^ 0x50a9259b7451453e, wp),
	surfacePerlin(     seed ^ 0x2e23350f66cb2335),
	vegetation_perlin( seed ^ 0xbebf64c4966b75db),
	temperature_perlin(seed ^ 0x5364424b2aa0fb15),
	cave_perlin1(      seed ^ 0xca5857b732d93020),
	cave_perlin2(      seed ^ 0x3b87a637383534d7),
	perlin(            seed ^ 0x3e02a6291ea49867)
{
	// nothing
}

void WorldGenerator::generateChunk(Chunk *chunk) {
	vec3i64 cc = chunk->getCC();

	const double *elevation = elevationGenerator.getChunk(vec2i64(cc[0], cc[1]));
	for (uint iccx = 0; iccx < Chunk::WIDTH; iccx++)
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
			
			const double depth = h - bcz;

			if (depth < 0) {
				solid = false;
			} else if (depth > (h * wp.surfaceRelDepth)) {
				solid = true;
			} else {
				const double xScale = wp.surfaceThresholdXScale;
				double funPos = 1.0 - depth / (h * wp.surfaceRelDepth) * 2;
				double threshold = funPos * (xScale * xScale + funPos * funPos) / (xScale * xScale + 1) * 2.0;
				double px = bcx / wp.surfaceScale;
				double py = bcy / wp.surfaceScale;
				double pz = bcz / wp.surfaceScale;
				double v = surfacePerlin.noise3(
					px, py, pz,
					wp.surfaceOctaves, wp.surfaceAmplGain, wp.surfaceFreqGain
				);
				if (v > threshold)
					solid = true;
				else
					solid = false;
			}

			if (solid)
				realDepth++;
			else
				realDepth = 0;

			uint8 block;
			if (iccz < (int) Chunk::WIDTH) {
				if (solid) {
					if (base_temperature > wp.desert_threshold && realDepth < 5)
						block = 4; // sand
					else if (base_vegetation > wp.grasland_threshold && realDepth == 1)
						block = 2; // gras
					else if (realDepth >= 5)
						block = 3; // dirt
					else
						block = 1; // stone
				} else {
					if (realDepth <= 0 && bcz <= 0)
						block = 62; // water
					else
						block = 0; // air
				}

				// caves
				if (block != 0) {
					const double v1 = std::abs(cave_perlin1.noise3(bc * 0.005, 4, 0.3, 2));
					const double v2 = std::abs(cave_perlin2.noise3(bc * 0.005, 4, 0.3, 2));
					if (((v1 + 1) * (v2 + 1) - 1) < 0.02) {
						block = 0; // air
					}
				}

				vec3ui8 icc(iccx, iccy, iccz);
				chunk->initBlock(icc, block);
			}
		}
	}

	chunk->finishInitialization();
}
