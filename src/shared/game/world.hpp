#ifndef WORLD_HPP
#define WORLD_HPP

#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <memory>

#include "shared/constants.hpp"
#include "shared/chunk_manager.hpp"

#include "player.hpp"

class Chunk;

struct WorldSnapshot {
	PlayerSnapshot playerSnapshots[MAX_CLIENTS];

/*
	public void write(ByteBuffer buffer) {
		for (int i = 0; i < Globals.MAX_CLIENTS; i++) {
			PlayerSnapshot ps = playerSnapshots[i];
			if (ps == null) {
				buffer.put((byte) 0);
				continue;
			}
			buffer.put((byte) 1);
			playerSnapshots[i].write(buffer);
		}
	}

	public static Snapshot readSnapshot(ByteBuffer buffer) {
		Snapshot snapshot = new Snapshot();
		for (int i = 0; i < Globals.MAX_CLIENTS; i++) {
			if (buffer.get() == 1)
				snapshot.playerSnapshots[i] = PlayerSnapshot.read(buffer);
		}
		return snapshot;
	}
*/
};

class World {
public:
	static const double GRAVITY;
	static const int LOADING_DISTANCE = 3;
	static const int LOADING_DIAMETER = LOADING_DISTANCE * 2 + 1;

private:
	ChunkManager *chunkManager;
	std::unordered_set<vec3i64, size_t(*)(vec3i64)> neededChunks;

	Player players[MAX_CLIENTS];
	vec3i64 oldPlayerChunks[MAX_CLIENTS];
	bool oldPlayerValids[MAX_CLIENTS];

public:
	explicit World(ChunkManager *chunkManager);
	~World();

	void tick(uint localPlayerID);

	int shootRay(vec3i64 start, vec3d ray, double maxDist,
			vec3i boxCorner, vec3i64 *outHit, vec3i64 outHitBlock[3],
			int outFaceDir[3]) const;

	bool hasCollision(vec3i64 wc) const;

	void addPlayer(int playerID);
	void deletePlayer(int playerID);

	Player &getPlayer(int playerID);
	const Player &getPlayer(int playerID) const;

	bool isChunkLoaded(vec3i64 cc) const;
	uint8 getBlock(vec3i64 bc) const;

	size_t getNumNeededChunks() const;

	WorldSnapshot makeSnapshot(int tick) const;

private:
	void requestChunks();
	void releaseChunks();
};


#endif // WORLD_HPP
