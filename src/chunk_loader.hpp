#ifndef CHUNK_LOADER_HPP
#define CHUNK_LOADER_HPP

#include "perlin.hpp"
#include "vmath.hpp"
#include "world.hpp"

class ChunkLoader {
private:
	static const int MAX_LOADS_UNTIL_UNLOAD = 100;

	Perlin perlin;
	double perlinScale = 500;
	double perlinAreaScale = 10000;
	double perlinCaveScale = 50;
	bool updateFaces;

	World *world;

public:
	ChunkLoader(World *world, uint64 seed, bool updateFaces);

	void run();

private:
	void loadChunk(vec3i64 cc);
};

#endif // CHUNK_LOADER_HPP
