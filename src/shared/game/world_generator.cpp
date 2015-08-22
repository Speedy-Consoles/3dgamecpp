#include "world_generator.hpp"

#include "shared/engine/math.hpp"

WorldGenerator::WorldGenerator(uint64 seed) : perlin(seed) {
	double s = surfaceThresholdXScale;
	surfaceThresholdYScale = 1/s + 1/(s*s*s);
}

void WorldGenerator::generateChunk(Chunk *chunk) {
	vec3i64 cc = chunk->getCC();

	for (uint ix = 0; ix < Chunk::WIDTH; ix++) {
		for (uint iy = 0; iy < Chunk::WIDTH; iy++) {
			int64 x = (int64) round((cc[0] * Chunk::WIDTH + ix) / overAllScale);
			int64 y = (int64) round((cc[1] * Chunk::WIDTH + iy) / overAllScale);
			double ax = x / areaXYScale;
			double ay = y / areaXYScale;
			double ap = perlin.perlin(ax, ay, 0);
			double mFac = (1 + tanh((ap - areaMountainThreshold)
					* areaSharpness)) / 2;
			double fFac = 1 - mFac;

			double mx = x / mountainXYScale;
			double my = y / mountainXYScale;
			double mh = perlin.octavePerlin(mx, my, 0,
					mountainOctaves, mountainExp)
					* mountainMaxHeight;

			double fx = x / flatlandXYScale;
			double fy = y / flatlandXYScale;
			double fh = perlin.octavePerlin(fx, fy, 0,
					flatlandOctaves, flatlandExp)
					* flatLandMaxHeight;

			double h = fh * fFac + mh * mFac;
			bool solid = false;
			int realDepth = 0;
			for (int iz = Chunk::WIDTH + 4; iz >= 0; iz--) {
				int64 z = iz + cc[2] * Chunk::WIDTH;
				double depth = h - z;

				if (depth < 0) {
					solid = false;
				} else if(depth > (h * surfaceRelDepth)) {
					solid = true;
				} else {
					double funPos = (1 - depth / (h * surfaceRelDepth) - 0.5) * 2 / surfaceThresholdXScale;
					double threshold = (funPos + funPos * funPos * funPos) / surfaceThresholdYScale + 0.5;
					double px = x / surfaceScale;
					double py = y / surfaceScale;
					double pz = z / surfaceScale;
					double v = perlin.octavePerlin(px, py, pz, 6, surfaceExp);
					if (v > threshold)
						solid = true;
					else
						solid = false;
				}
				if (solid)
					realDepth++;
				else
					realDepth = 0;

				vec3ui8 icc(ix, iy, iz);
				if (iz < (int) Chunk::WIDTH) {
					if (realDepth == 1)
						chunk->initBlock(icc, 2);
					else if(realDepth >= 5)
						chunk->initBlock(icc, 3);
					else if(solid)
						chunk->initBlock(icc, 1);
					else
						chunk->initBlock(icc, 0);
				}
			}
		}
	}

	chunk->finishInitialization();
}
