#include "world.hpp"

#include "chunk.hpp"

#include "constants.hpp"
#include "util.hpp"
#include "io/logging.hpp"

#include <cstdio>


World::World(std::string id) : id(id), chunks(0, vec3i64HashFunc) {
}

World::~World() {
	for (auto iter : chunks) {
		iter.second->free();
	}
}

void World::tick(int tick, uint localPlayerID) {
	if (pause)
		return;
	for (uint i = 0; i < MAX_CLIENTS; i++) {
		if (players[i].isValid())
			players[i].tick(tick, i == localPlayerID);
	}
}

void World::setPause(bool pause) {
	this->pause = pause;
}

bool World::isPaused() {
	return pause;
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

uint8 World::getBlock(vec3i64 bc) const {
	auto it = chunks.find(bc2cc(bc));
	if (it == chunks.end())
		return 0;
	return it->second->getBlock(bc2icc(bc));
}

bool World::setBlock(vec3i64 bc, uint8 type, bool visual) {
	vec3i64 cc = bc2cc(bc);
	auto it = chunks.find(bc2cc(bc));
	if (it == chunks.end())
		return false;
	Chunk *c = it->second;
	vec3ui8 icc = bc2icc(bc);
	if (c->setBlock(icc, type)) {
		if (visual)
			c->makePassThroughs();

		changedChunks.push_back(cc);
		// TODO change change other chunks
		return true;
	}
	return false;
}

Chunk *World::getChunk(vec3i64 cc) {
	auto iter = chunks.find(cc);
	if (iter == chunks.end())
		return nullptr;
	return iter->second;
}

void World::insertChunk(Chunk *chunk) {
	chunks.insert({chunk->getCC(), chunk});
	// TODO change change other chunks
	changedChunks.push_back(chunk->getCC());
}

Chunk *World::removeChunk(vec3i64 cc) {
	auto iter = chunks.find(cc);
	if (iter == chunks.end())
		return nullptr;
	Chunk *c = iter->second;
	chunks.erase(iter);
	return c;
}

bool World::popChangedChunk(vec3i64 *ccc) {
	if (changedChunks.empty())
		return false;
	*ccc = changedChunks.front();
	changedChunks.pop_front();
	return true;
}

void World::clearChunks() {
	changedChunks.clear();
	for (auto iter : chunks) {
		Chunk *c = iter.second;
		c->free();
	}
	chunks.clear();
}

size_t World::getNumChunks() {
	return chunks.size();
}

Player &World::getPlayer(int playerID) {
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
