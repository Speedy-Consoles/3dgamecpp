#ifndef WORLD_HPP
#define WORLD_HPP

#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <memory>

#include "constants.hpp"
#include "player.hpp"
#include "shared/chunk_manager.hpp"

class Chunk;

using namespace std;

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
	static const int LOADING_RANGE = 2;
	static const int LOADING_DIAMETER = LOADING_RANGE * 2 + 1;

	using ChunkMap = unordered_map<vec3i64, shared_ptr<const Chunk>, size_t(*)(vec3i64)>;
	using ChunkRequestedSet = unordered_set<vec3i64, size_t(*)(vec3i64)>;

private:
	string id;
	ChunkManager *chunkManager;
	ChunkMap chunks;
	ChunkRequestedSet requested;
	deque<vec3i64> changedChunks;

	Player players[MAX_CLIENTS];

	vec3i64 oldPlayerChunks[MAX_CLIENTS];
	int playerCheckChunkIndex[MAX_CLIENTS];
	int playerCheckedChunks[MAX_CLIENTS];
public:
	World(string id, ChunkManager *chunkManager);
	~World();

	string getId() const { return id; }

	void tick(int tick, uint localPlayerID);

	int shootRay(vec3i64 start, vec3d ray, double maxDist,
			vec3i boxCorner, vec3d *outHit, vec3i64 outHitBlock[3],
			int outFaceDir[3]) const;

	bool hasCollision(vec3i64 wc) const;

	//bool setBlock(vec3i64 bc, uint8 type, bool updateFaces);
	uint8 getBlock(vec3i64 bc) const;

	void addPlayer(int playerID);
	void deletePlayer(int playerID);

	Player &getPlayer(int playerID);
	const Player &getPlayer(int playerID) const;

//	Chunk *getChunk(vec3i64 cc);
//	void insertChunk(Chunk *chunk);
//	Chunk *removeChunk(vec3i64 cc);

//	bool popChangedChunk(vec3i64 *ccc);
//	void clearChunks();
	bool chunkLoaded(vec3i64 cc);

	size_t getNumChunks();

	WorldSnapshot makeSnapshot(int tick) const;

private:
	void requestChunks();
	void insertChunks();
	void removeChunks();
};


#endif // WORLD_HPP
