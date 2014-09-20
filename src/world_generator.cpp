/*
 * world_generator.cpp
 *
 *  Created on: 17.09.2014
 *      Author: lars
 */

#include "world_generator.hpp"

WorldGenerator::WorldGenerator(uint64 seed) : perlin(seed) {
	double s = surfaceThresholdXScale;
	surfaceThresholdYScale = 1/s + 1/(s*s*s);
}

Chunk *WorldGenerator::generateChunk(vec3i64 cc) {
	Chunk *chunk = new Chunk(cc);
	generateChunk(*chunk);
	return chunk;
}

void WorldGenerator::generateChunk(vec3i64 cc, Chunk &chunk) {
	chunk.setCC(cc);
	generateChunk(chunk);
}

void WorldGenerator::generateChunk(Chunk &chunk) {
	auto cc = chunk.getCC();
	for (uint ix = 0; ix < Chunk::WIDTH; ix++) {
		for (uint iy = 0; iy < Chunk::WIDTH; iy++) {
			long x = round((cc[0] * Chunk::WIDTH + ix) / overAllScale);
			long y = round((cc[1] * Chunk::WIDTH + iy) / overAllScale);
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
			for (uint iz = 0; iz < Chunk::WIDTH; iz++) {
				int index = Chunk::getBlockIndex(vec3ui8(ix, iy, iz));
				long z = iz + cc[2] * Chunk::WIDTH;
				double depth = h - z;
				if (depth < 0) {
					chunk.initBlock(index, 0);
					continue;
				} else if(depth > (h * surfaceRelDepth)) {
					chunk.initBlock(index, 1);
					continue;
				}
				double funPos = (1 - depth / (h * surfaceRelDepth) - 0.5) * 2 / surfaceThresholdXScale;
				double threshold = (funPos + funPos * funPos * funPos) / surfaceThresholdYScale + 0.5;
				double px = x / surfaceScale;
				double py = y / surfaceScale;
				double pz = z / surfaceScale;
				double v = perlin.octavePerlin(px, py, pz, 6, surfaceExp);
				if (v > threshold)
					chunk.initBlock(index, 1);
				else
					chunk.initBlock(index, 0);
			}
		}
	}
}
