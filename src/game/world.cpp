#include "world.hpp"

#include <cstdio>

#include "engine/logging.hpp"

#include "chunk.hpp"

#include "constants.hpp"
#include "util.hpp"

const double World::GRAVITY = -9.81 * RESOLUTION / 60.0 / 60.0 * 4;

World::World(std::string id, ChunkManager *chunkManager) :
		id(id),
		chunkManager(chunkManager),
		neededChunks(0, vec3i64HashFunc)
{
	LOG(INFO, "Opening world '" << id << "'");
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		oldPlayerValids[i] = false;
	}
}

World::~World() {
	LOG(DEBUG, "Deleting world '" << id << "'");
}

void World::tick(int tick, uint localPlayerID) {
	requestChunks();
	for (uint i = 0; i < MAX_CLIENTS; i++) {
		if (players[i].isValid())
			players[i].tick(tick, i == localPlayerID);
	}
	releaseChunks();
}

void World::requestChunks() {
	for (int p = 0; p < MAX_CLIENTS; p++) {
		if (!players[p].isValid()) {
			oldPlayerValids[p] = false;
			continue;
		}
		vec3i64 pc = players[p].getChunkPos();
		if (!oldPlayerValids[p] || pc != oldPlayerChunks[p]) {
			oldPlayerValids[p] = true;
			oldPlayerChunks[p] = pc;
			int checkChunkIndex = 0;
			while(LOADING_ORDER[checkChunkIndex].norm() <= LOADING_DISTANCE) {
				vec3i64 cc = pc + LOADING_ORDER[checkChunkIndex].cast<int64>();
				auto it = neededChunks.find(cc);
				if (it == neededChunks.end()) {
					chunkManager->requestChunk(cc);
					neededChunks.insert(cc);
				}
				checkChunkIndex++;
			}
		}
	}
}

void World::releaseChunks() {
	for (auto iter = neededChunks.begin(); iter != neededChunks.end();) {
		vec3i64 cc = *iter;
		bool inRange = false;

		for (int p = 0; p < MAX_CLIENTS; p++) {
			if (!players[p].isValid())
				continue;
			if ((cc - players[p].getChunkPos()).maxAbs() <= (int) LOADING_DISTANCE + 1) {
				inRange = true;
				break;
			}
		}

		if (!inRange) {
			chunkManager->releaseChunk(cc);
			iter = neededChunks.erase(iter);
		} else
			iter++;
	}
}

// TODO make precision position-independent
int World::shootRay(vec3i64 start, vec3d ray, double maxDist,
		vec3i boxCorner, vec3d *outHit, vec3i64 outHitBlock[3],
		int outFaceDir[3]) const {
	int dirs[3];
	for (int i = 0; i < 3; i++) {
		if (ray[i] == 0)
			dirs[i] = -1;
		else
			dirs[i] = getDir(i, sgn(ray[i]));
	}

	vec3i64 block = wc2bc(start);
	if (boxCorner != vec3i(0, 0, 0)) {
		for (int i = 0; i < 3; i++) {
			if (start[i] % RESOLUTION == 0)
				block[i] -= boxCorner[i];
		}
	}

	int blockHitCounter = 0;
	while (blockHitCounter == 0) {
		vec3i64 oldBlock = block;
		for (int d : dirs) {
			if (d == -1)
				continue;
			int dirDim = DIR_DIMS[d];
			const int *otherDims = OTHER_DIR_DIMS[d];
			int sign = DIRS[d][dirDim];
			int64 facePlane = (block[dirDim] + (sign + 1) / 2) * RESOLUTION;
			int64 planeDist = facePlane - start[dirDim];
			double factor = planeDist / ray[dirDim];
			vec3d hit;
			hit[dirDim] = facePlane;
			hit[otherDims[0]] = start[otherDims[0]] + ray[otherDims[0]]
					* factor;
			hit[otherDims[1]] = start[otherDims[1]] + ray[otherDims[1]]
					* factor;

			double diff[2] = {
				hit[otherDims[0]] - block[otherDims[0]] * RESOLUTION,
				hit[otherDims[1]] - block[otherDims[1]] * RESOLUTION
			};
			if (diff[0] >= 0
					&& diff[0] <= RESOLUTION
					&& diff[1] >= 0
					&& diff[1] <= RESOLUTION) {
				double dist = (start.cast<double>() - hit).norm();
				if (dist > maxDist)
					return 0;

				vec3i8 dir = DIRS[d];
				vec3i64 nextBlock = block + dir.cast<int64>();
				if (getBlock(nextBlock)) {
					if (outHit != nullptr)
						*outHit = hit;
					if (outFaceDir != nullptr)
						outFaceDir[blockHitCounter] = (d + 3) % 6;
					if (outHitBlock != nullptr)
						outHitBlock[blockHitCounter] = nextBlock;
					blockHitCounter++;
				} else if (blockHitCounter == 0) {
					block = nextBlock;
				}
			}
		}
		if (block == oldBlock && blockHitCounter == 0) {
			// TODO fix this
			printf("Oh no!\n");
		}
	}
	return blockHitCounter;
}

bool World::hasCollision(vec3i64 wc) const {
	vec3i64 block = wc2bc(wc);
	vec3i onFace(0, 0, 0);
	for (int i = 0; i < 3; i++) {
		if (wc[i] % RESOLUTION == 0)
			onFace[i] = 1;
	}
	for (int x = 0; x <= onFace[0]; x++) {
		for (int y = 0; y <= onFace[1]; y++) {
			for (int z = 0; z <= onFace[2]; z++) {
				if (!getBlock(block - vec3i64(x, y, z)))
					return false;
			}
		}
	}
	return true;
}

bool World::isChunkLoaded(vec3i64 cc) const {
	return chunkManager->getChunk(cc) != 0;
}

uint8 World::getBlock(vec3i64 bc) const {
	const Chunk *chunk = chunkManager->getChunk(bc2cc(bc));
	if (!chunk)
		return 0;
	return chunk->getBlock(bc2icc(bc));
}

size_t World::getNumNeededChunks() const {
	return neededChunks.size();
}

Player &World::getPlayer(int playerID) {
	return players[playerID];
}

const Player &World::getPlayer(int playerID) const {
	return players[playerID];
}

void World::addPlayer(int playerID) {
	players[playerID].create(this);
}

void World::deletePlayer(int playerID) {
	players[playerID].destroy();
}

WorldSnapshot World::makeSnapshot(int tick) const {
	WorldSnapshot snapshot;
	for (uint i = 0; i < MAX_CLIENTS; i++) {
		if (players[i].isValid())
			snapshot.playerSnapshots[i] = players[i].makeSnapshot(tick);
	}
	return snapshot;
}
