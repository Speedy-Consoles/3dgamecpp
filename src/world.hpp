#ifndef WORLD_HPP
#define WORLD_HPP

#include <unordered_map>
#include "constants.hpp"
#include "chunk.hpp"
#include "player.hpp"

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

	using ChunkMap = std::unordered_map<vec3i64, Chunk, size_t(*)(vec3i64)>;

private:
	ChunkMap chunks;

	Player players[MAX_CLIENTS];

public:

	World();

	void tick(int tick, uint localPlayerID);

	int shootRay(vec3i64 start, vec3d ray, double maxDist,
			vec3i boxCorner, vec3d *outHit, vec3i64 outHitBlock[3],
			int outFaceDir[3]) const;

	bool hasCollision(vec3i64 wc) const;

	bool setBlock(vec3i64 bc, uint8 type, bool updateFaces);
	uint8 getBlock(vec3i64 bc) const;

	void addPlayer(int playerID);
	void deletePlayer(int playerID);

	Player &getPlayer(int playerID);
	ChunkMap &getChunks();

	WorldSnapshot makeSnapshot(int tick) const;
};


#endif // WORLD_HPP
