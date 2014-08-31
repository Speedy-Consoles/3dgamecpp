#ifndef CHUNK_LOADER_HPP
#define CHUNK_LOADER_HPP

#include "perlin.hpp"
#include "vmath.hpp"
#include "world.hpp"

class ChunkLoader {
private:
	static const int MAX_LOADS_UNTIL_UNLOAD = 50;

	Perlin perlin;

	double overAllScale = 1;

	double perlinAreaXYScale = 6000;
	double perlinAreaMountainThreshold = 0.7;
	double perlinAreaSharpness = 20;

	double perlinMountainXYScale = 500;
	double perlinMountainMaxHeight = 800;
	int perlinMountainOctaves = 8;
	double perlinMountainExp = 0.5;

	double perlinFlatlandXYScale = 800;
	double perlinFlatLandMaxHeight = 40;
	int perlinFlatlandOctaves = 6;
	double perlinFlatlandExp = 0.8;

	double perlinCaveScale = 100;

	bool updateFaces;

	World *world;

public:
	ChunkLoader(World *world, uint64 seed, bool updateFaces);

	void run();

private:
	void loadChunk(vec3i64 cc);
};

#endif // CHUNK_LOADER_HPP
