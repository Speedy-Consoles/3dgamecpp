#include "world.hpp"

#include <cstdio>

#include "shared/engine/logging.hpp"
#include "shared/constants.hpp"
#include "shared/block_utils.hpp"

#include "chunk.hpp"

static logging::Logger logger("default");

const double World::GRAVITY = -9.81 * RESOLUTION / 60.0 / 60.0 * 4;

World::World(ChunkManager *chunkManager) :
		chunkManager(chunkManager),
		neededChunks(0, vec3i64HashFunc)
{
	LOG_DEBUG(logger) << "Creating World";
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		oldCharValids[i] = false;
	}
}

World::~World() {
	LOG_DEBUG(logger) << "Deleting World";
}

void World::tick() {
	if (chunkManager)
		requestChunks();
	for (uint i = 0; i < MAX_CLIENTS; i++) {
		if (characters[i].isValid())
			characters[i].tick();
	}
	if (chunkManager)
		releaseChunks();
}

void World::requestChunks() {
	for (int p = 0; p < MAX_CLIENTS; p++) {
		if (!characters[p].isValid()) {
			oldCharValids[p] = false;
			continue;
		}
		vec3i64 pc = characters[p].getChunkPos();
		if (!oldCharValids[p] || pc != oldCharChunks[p]) {
			oldCharValids[p] = true;
			oldCharChunks[p] = pc;
			int checkChunkIndex = 0;
			while(LOADING_ORDER[checkChunkIndex].norm() <= LOADING_DISTANCE) {
				vec3i64 cc = pc + LOADING_ORDER[checkChunkIndex].cast<int64>();
				auto it = neededChunks.find(cc);
				if (it == neededChunks.end()) {
					chunkManager->requireChunk(cc);
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
			if (!characters[p].isValid())
				continue;
			if ((cc - characters[p].getChunkPos()).maxAbs() <= (int) LOADING_DISTANCE + 1) {
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

int World::shootRay(vec3i64 start, vec3d ray, double maxDist,
		vec3i boxCorner, vec3i64 *outHit, vec3i64 outHitBlock[3],
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
			int64 planeDist = (block[dirDim] + (sign + 1) / 2) * RESOLUTION - start[dirDim];
			double factor = planeDist / ray[dirDim];
			vec3d hit;
			hit[dirDim] = (double)planeDist;
			hit[otherDims[0]] = (double)ray[otherDims[0]] * factor;
			hit[otherDims[1]] = (double)ray[otherDims[1]] * factor;

			double diff[2] = {
				hit[otherDims[0]]
					- (block[otherDims[0]] * RESOLUTION - start[otherDims[0]]),
				hit[otherDims[1]]
					- (block[otherDims[1]] * RESOLUTION - start[otherDims[1]])
			};
			if (diff[0] >= 0
					&& diff[0] <= RESOLUTION
					&& diff[1] >= 0
					&& diff[1] <= RESOLUTION) {
				if (hit.norm() > maxDist)
					return 0;

				vec3i8 dir = DIRS[d];
				vec3i64 nextBlock = block + dir.cast<int64>();
				if (getBlock(nextBlock)) {
					if (outHit != nullptr)
						*outHit = start + vec3i64((int64)round(hit[0]), (int64)round(hit[1]), (int64)round(hit[2]));
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
	if (chunkManager)
		return chunkManager->getChunk(cc) != 0;
	return false;
}

uint8 World::getBlock(vec3i64 bc) const {
	if (!chunkManager)
		return 0;
	const Chunk *chunk = chunkManager->getChunk(bc2cc(bc));
	if (!chunk)
		return 0;
	return chunk->getBlock(bc2icc(bc));
}

size_t World::getNumNeededChunks() const {
	return neededChunks.size();
}

Character &World::getCharacter(int charId) {
	return characters[charId];
}

const Character &World::getCharacter(int charId) const {
	return characters[charId];
}

void World::addCharacter(int charId) {
	characters[charId].create(this);
}

void World::deleteCharacter(int charId) {
	characters[charId].destroy();
}

//Snapshot World::makeSnapshot(int tick) const {
//	Snapshot snapshot;
//	for (uint i = 0; i < MAX_CLIENTS; i++) {
//		if (characters[i].isValid())
//			snapshot.characterSnapshots[i] = characters[i].makeSnapshot(tick);
//	}
//	return snapshot;
//}
