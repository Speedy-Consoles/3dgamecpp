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
		chunks(0, vec3i64HashFunc),
		requested(0, vec3i64HashFunc){
	LOG(INFO, "Opening world '" << id << "'");
}

World::~World() {
	LOG(DEBUG, "Deleting world '" << id << "'");
//	for (auto iter : chunks) {
//		iter.second->free();
//	}
}

void World::tick(int tick, uint localPlayerID) {
	requestChunks();
	insertChunks();
	for (uint i = 0; i < MAX_CLIENTS; i++) {
		if (players[i].isValid())
			players[i].tick(tick, i == localPlayerID);
	}
	removeChunks();
}

void World::requestChunks() {
	for (int p = 0; p < MAX_CLIENTS; p++) { // TODO better order
		if (!players[p].isValid())
			continue;
		vec3i64 pc = players[p].getChunkPos();
		if (pc != oldPlayerChunks[p]) {
			playerCheckChunkIndex[p] = 0;
			playerCheckedChunks[p] = 0;
		}
		while(playerCheckedChunks[p] < LOADING_DIAMETER * LOADING_DIAMETER * LOADING_DIAMETER) {
			vec3i64 cd = LOADING_ORDER[playerCheckChunkIndex[p]].cast<int64>();
			if (cd.maxAbs() <= LOADING_RANGE) {
				vec3i64 cc = pc + cd;
				auto it2 = requested.find(cc);
				if (it2 == requested.end()) {
					if (!chunkManager->request(cc, WORLD_LISTENER_ID))
						break;
					requested.insert(cc);
				}
				playerCheckedChunks[p]++;
			}
			playerCheckChunkIndex[p]++;
		}
	}
}

void World::insertChunks() {
	shared_ptr<const Chunk> sp;
	while ((sp = chunkManager->getNextChunk(WORLD_LISTENER_ID)).get()) {
		chunks.insert({sp->getCC(), sp});
	}
}

void World::removeChunks() {
	for (auto iter = chunks.begin(); iter != chunks.end();) {
		vec3i64 cc = iter->first;
		bool inRange = false;

		for (int p = 0; p < MAX_CLIENTS; p++) { // TODO better order
			if (!players[p].isValid())
				continue;
			if ((cc - players[p].getChunkPos()).maxAbs() <= (int) LOADING_RANGE + 1) {
				inRange = true;
				break;
			}
		}

		if (!inRange) {
			iter = chunks.erase(iter);
			auto iter2 = requested.find(cc);
			requested.erase(iter2);
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

uint8 World::getBlock(vec3i64 bc) const {
	auto it = chunks.find(bc2cc(bc));
	if (it == chunks.end())
		return 0;
	return it->second->getBlock(bc2icc(bc));
}

bool World::chunkLoaded(vec3i64 cc) {
	return chunks.find(cc) != chunks.end();
}

//bool World::setBlock(vec3i64 bc, uint8 type, bool visual) {
//	vec3i64 cc = bc2cc(bc);
//	auto it = chunks.find(bc2cc(bc));
//	if (it == chunks.end())
//		return false;
//	Chunk *c = it->second;
//	vec3ui8 icc = bc2icc(bc);
//	if (c->setBlock(icc, type)) {
//		if (visual)
//			c->makePassThroughs();
//
//		for (size_t i = 0; i < 27; i++) {
//			if (i == BASE_NINE_CUBE_CYCLE){
//				changedChunks.push_back(cc);
//				continue;
//			}
//
//			vec3i64 diff = NINE_CUBE_CYCLE[i].cast<int64>();
//			bool leave = false;
//			for (int dim = 0; dim < 3; dim++) {
//				if (diff[dim] != 0 && (diff[dim] + 1) / 2 * (Chunk::WIDTH - 1) != icc[dim]) {
//					leave = true;
//					break;
//				}
//			}
//			if (leave)
//				continue;
//
//			Chunk *c = getChunk(cc + diff);
//			if (c)
//				c->setChanged();
//			changedChunks.push_back(cc + diff);
//		}
//
//		return true;
//	}
//	return false;
//}

//Chunk *World::getChunk(vec3i64 cc) {
//	auto iter = chunks.find(cc);
//	if (iter == chunks.end())
//		return nullptr;
//	return iter->second;
//}

//void World::insertChunk(Chunk *chunk) {
//	chunks.insert({chunk->getCC(), chunk});
//	for (size_t i = 0; i < 27; i++) {
//		if (i == BASE_NINE_CUBE_CYCLE){
//			changedChunks.push_back(chunk->getCC());
//			continue;
//		}
//		vec3i64 cc = chunk->getCC() + NINE_CUBE_CYCLE[i].cast<int64>();
//		Chunk *c = getChunk(cc);
//		if (c) {
//			c->setChanged();
//			changedChunks.push_back(cc);
//		}
//	}
//}

//Chunk *World::removeChunk(vec3i64 cc) {
//	auto iter = chunks.find(cc);
//	if (iter == chunks.end())
//		return nullptr;
//	Chunk *c = iter->second;
//	chunks.erase(iter);
//	return c;
//}

//bool World::popChangedChunk(vec3i64 *ccc) {
//	if (changedChunks.empty())
//		return false;
//	*ccc = changedChunks.front();
//	changedChunks.pop_front();
//	return true;
//}

//void World::clearChunks() {
//	changedChunks.clear();
//	for (auto iter : chunks) {
//		Chunk *c = iter.second;
//		c->free();
//	}
//	chunks.clear();
//}

size_t World::getNumChunks() const {
	return chunks.size();
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
