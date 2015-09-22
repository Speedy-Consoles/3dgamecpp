#ifndef WORLD_HPP
#define WORLD_HPP

#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <memory>

#include "character.hpp"
#include "shared/constants.hpp"
#include "shared/chunk_manager.hpp"

class Chunk;

class World {
public:
	static const double GRAVITY;
	static const int LOADING_DISTANCE = 3;
	static const int LOADING_DIAMETER = LOADING_DISTANCE * 2 + 1;

private:
	ChunkManager *chunkManager;
	std::unordered_set<vec3i64, size_t(*)(vec3i64)> neededChunks;

	Character characters[MAX_CLIENTS];
	vec3i64 oldCharChunks[MAX_CLIENTS];
	bool oldCharValids[MAX_CLIENTS];

public:
	explicit World(ChunkManager *chunkManager);
	~World();

	void tick();

	int shootRay(vec3i64 start, vec3d ray, double maxDist,
			vec3i boxCorner, vec3i64 *outHit, vec3i64 outHitBlock[3],
			int outFaceDir[3]) const;

	bool hasCollision(vec3i64 wc) const;

	void addCharacter(int charId);
	void deleteCharacter(int charId);

	Character &getCharacter(int charId);
	const Character &getCharacter(int charId) const;

	bool isChunkLoaded(vec3i64 cc) const;
	uint8 getBlock(vec3i64 bc) const;

	size_t getNumNeededChunks() const;

	//Snapshot makeSnapshot(int tick) const;

private:
	void requestChunks();
	void releaseChunks();
};


#endif // WORLD_HPP
