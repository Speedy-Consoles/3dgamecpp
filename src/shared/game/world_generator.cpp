#include "world_generator.hpp"

#include "shared/engine/math.hpp"
#include "shared/engine/vmath.hpp"

WorldGenerator::WorldGenerator(uint64 seed, WorldParams params) :
	wp(params),
	elevationGenerator(seed ^ 0x50a9259b7451453e, wp),
	vegetation_perlin( seed ^ 0xbebf64c4966b75db),
	temperature_perlin(seed ^ 0x5364424b2aa0fb15),
	surfacePerlin(     seed ^ 0x2e23350f66cb2335),
	caveSwitchPerlin(  seed ^ 0x3e02a6291ea49867),
	tunnelPerlin1a(    seed ^ 0xca5857b732d93020),
	tunnelPerlin2a(    seed ^ 0x3b87a637383534d7),
	tunnelPerlin3a(    seed ^ 0xbc48698ffbf20f79),
	tunnelPerlin1b(    seed ^ 0x9fa9e48141d4eed8),
	tunnelPerlin2b(    seed ^ 0x1ddb866bf73756f9),
	tunnelPerlin3b(    seed ^ 0x649e707a89ae7cda),
	roomPerlin(        seed ^ 0x2508660216ec5e91)
{
	caveSwitchBuffer = new double[Chunk::SIZE + 9 * Chunk::WIDTH * Chunk::WIDTH];
}

WorldGenerator::~WorldGenerator() {
	delete[] caveSwitchBuffer;
}

void WorldGenerator::generateChunk(Chunk *chunk) {
	vec3i64 cc = chunk->getCC();

	const ElevationChunk elevation = elevationGenerator.getChunk(vec2i64(cc[0], cc[1]));
	bool caveSwitchBuffered = cc[2] * Chunk::WIDTH <= elevation.min;
	if (caveSwitchBuffered) {
		caveSwitchPerlin.noise3(
			cc.cast<double>() * Chunk::WIDTH / wp.caveSwitchScale / wp.overall_scale,
			vec3d(1 / wp.caveSwitchScale / wp.overall_scale),
			vec3ui(Chunk::WIDTH, Chunk::WIDTH, Chunk::WIDTH + 9),
			wp.caveSwitchOctaves, wp.caveSwitchAmplGain, wp.caveSwitchFreqGain,
			caveSwitchBuffer
		);
	}

	for (uint iccx = 0; iccx < Chunk::WIDTH; iccx++)
	for (uint iccy = 0; iccy < Chunk::WIDTH; iccy++) {
		int64 bcx = cc[0] * Chunk::WIDTH + iccx;
		int64 bcy = cc[1] * Chunk::WIDTH + iccy;

		vec3d bc_xy(bcx, bcy, 0);
			
		double h = elevation.heights[iccy * Chunk::WIDTH + iccx];
		double base_vegetation = 0;//vegetation_perlin.perlin(bc_xy * 1 / wp.vegetation_xy_scale);
		double base_temperature = 0;//temperature_perlin.perlin(bc_xy * 1 / wp.temperature_xy_scale);

		bool solid = false;
		int realDepth = 0;
		for (int iccz = Chunk::WIDTH + 8; iccz >= 0; iccz--) {
			int64 bcz = iccz + cc[2] * Chunk::WIDTH;
			vec3d dbc(bcx, bcy, bcz);
			
			const double depth = h - bcz;

			if (depth < 0) {
				solid = false;
			} else if (depth > (h * wp.surfaceRelDepth)) {
				solid = true;
			} else {
				const double xScale = wp.surfaceThresholdXScale;
				double funPos = 1.0 - depth / (h * wp.surfaceRelDepth) * 2;
				double threshold = funPos * (xScale * xScale + funPos * funPos) / (xScale * xScale + 1) * 2.0;
				double v = surfacePerlin.noise3(
					dbc / wp.surfaceScale,
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

				// TODO cave rooms
				// TODO height dependent
				// caves
				if (block != 0) {
					double caveSwitch;
					if (caveSwitchBuffered) {
						uint index = ((iccz * Chunk::WIDTH) + iccy) * Chunk::WIDTH + iccx;
						caveSwitch = caveSwitchBuffer[index];
					} else {
						caveSwitch = caveSwitchPerlin.noise3(
								dbc / wp.caveSwitchScale / wp.overall_scale, wp.caveSwitchOctaves,
								wp.caveSwitchAmplGain, wp.caveSwitchFreqGain);
					}
					double overLap = wp.caveSwitchOverlap;
					double caveValue1 = 0;
					double caveValue2 = 0;
					vec3d tunnelCoords(dbc / wp.tunnelScale);
					if (caveSwitch > -overLap) {
						const double v1 = std::abs(tunnelPerlin1a.noise3(
								tunnelCoords, wp.tunnelOctaves, wp.tunnelAmplGain, wp.tunnelFreqGain));
						const double v2 = std::abs(tunnelPerlin2a.noise3(
								tunnelCoords, wp.tunnelOctaves, wp.tunnelAmplGain, wp.tunnelFreqGain));
						const double v3 = std::abs(tunnelPerlin3a.noise3(
								tunnelCoords, wp.tunnelOctaves, wp.tunnelAmplGain, wp.tunnelFreqGain));
						double v12 = 1 / ((v1 * v1 + 1) * (v2 * v2 + 1) - 1);
						double v23 = 1 / ((v2 * v2 + 1) * (v3 * v3 + 1) - 1);
						double v31 = 1 / ((v3 * v3 + 1) * (v1 * v1 + 1) - 1);
						double ramp = 1;
						if (caveSwitch < 0)
							ramp = (overLap + caveSwitch) / overLap;
						caveValue1 = (v12 + v23 + v31) * ramp;
					}
					if (block != 0 && caveSwitch < overLap) {
						const double v1 = std::abs(tunnelPerlin1b.noise3(
								tunnelCoords, wp.tunnelOctaves, wp.tunnelAmplGain, wp.tunnelFreqGain));
						const double v2 = std::abs(tunnelPerlin2b.noise3(
								tunnelCoords, wp.tunnelOctaves, wp.tunnelAmplGain, wp.tunnelFreqGain));
						const double v3 = std::abs(tunnelPerlin3b.noise3(
								tunnelCoords, wp.tunnelOctaves, wp.tunnelAmplGain, wp.tunnelFreqGain));
						double v12 = 1 / ((v1 * v1 + 1) * (v2 * v2 + 1) - 1);
						double v23 = 1 / ((v2 * v2 + 1) * (v3 * v3 + 1) - 1);
						double v31 = 1 / ((v3 * v3 + 1) * (v1 * v1 + 1) - 1);
						double ramp = 1;
						if (caveSwitch > 0)
							ramp = (overLap - caveSwitch) / overLap;
						caveValue2 = (v12 + v23 + v31) * ramp;
					}
					if (caveValue1 + caveValue2 > wp.tunnelSquaredThreshold) {
						block = 0;
					}
				}

				vec3ui8 icc(iccx, iccy, iccz);
				chunk->initBlock(icc, block);
			}
		}
	}

	chunk->finishInitialization();
}
