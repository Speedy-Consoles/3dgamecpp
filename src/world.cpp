#include "world.hpp"
#include <cstdio>
#include "constants.hpp"
#include "util.hpp"
#include "logging.hpp"


World::World() : chunks(0, vec3i64HashFunc), dur_ticking(0) {
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
	using namespace vec_auto_cast;
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
			long facePlane = (block[dirDim] + (sign + 1) / 2) * RESOLUTION;
			long planeDist = facePlane - start[dirDim];
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
				double dist = (start - hit).norm();
				if (dist > maxDist)
					return 0;

				vec3i8 dir = DIRS[d];
				vec3i64 nextBlock = block + dir;
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

bool World::setBlock(vec3i64 bc, uint8 type, bool updateFaces) {
	using namespace vec_auto_cast;
	vec3i64 cc = bc2cc(bc);
	auto it = chunks.find(bc2cc(bc));
	if (it == chunks.end())
		return false;
	Chunk *c = it->second;
	vec3ui8 icc = bc2icc(bc);
	if (c->setBlock(icc, type)) {
		bool chunkChanged[27];
		for (int i = 0; i < 27; i++) {
			chunkChanged[i] = false;
		}

		for (uint8 d = 0; d < 6; d++) {
			uint8 invD = (d + 3) % 6;
			vec3i8 dir = DIRS[d];
			vec3i64 nbc = bc + dir;
			vec3i64 ncc = bc2cc(nbc);
			vec3i8 dcc = (ncc - cc).cast<int8>();

			if (updateFace(bc, d))
				chunkChanged[BASE_CUBE_CYCLE] = true;
			else if (updateFace(nbc, invD))
				chunkChanged[vec2CubeCycle(dcc)] = true;

			for (int i = 0; i < 8; i++) {
				vec3i64 obc = bc + EIGHT_CYCLES_3D[d][i];
				if (updateFace(obc, invD)) {
					vec3i64 occ = bc2cc(obc);
					vec3i8 occd = (occ - cc).cast<int8>();
					chunkChanged[vec2CubeCycle(occd)] = true;
				}
			}
		}

		for (int i = 0; i < 27; i++) {
			if(chunkChanged[i])
				changedChunks.push_front(c->cc + CUBE_CYCLE[i]);
		}

		return true;
	}
	return false;
}

bool World::updateFace(vec3i64 bc, uint8 faceDir) {
	using namespace vec_auto_cast;
	vec3i64 cc = bc2cc(bc);
	vec3ui8 icc = bc2icc(bc);
	vec3i8 dir = DIRS[faceDir];
	vec3i64 nbc = bc + dir;
	vec3i64 ncc = bc2cc(nbc);
	vec3ui8 nicc = bc2icc(nbc);

	Chunk *c = getChunk(cc);
	Chunk *nc = getChunk(ncc);
	if (!c || !nc)
		return false;

	uint8 thisType = c->getBlock(icc);
	uint8 thatType = nc->getBlock(nicc);

	if (thisType != 0 && thatType == 0) {
		c->addFace(Face{icc, faceDir, getFaceCorners(cc * Chunk::WIDTH + icc, faceDir)});
		return true;
	} else if((thisType != 0) == (thatType != 0))
		return c->removeFace(Face{icc, faceDir, 0});

	return false;
}

uint8 World::getFaceCorners(vec3i64 bc, uint8 faceDir) const {
	using namespace vec_auto_cast;
	uint8 corners = 0;
	for (int j = 0; j < 8; ++j) {
		vec3i v = EIGHT_CYCLES_3D[faceDir][j];
		vec3i64 dbc = bc + v;
		if (getBlock(dbc)) {
			corners |= 1 << j;
		}
	}
	return corners;
}

Chunk *World::getChunk(vec3i64 cc) {
	auto iter = chunks.find(cc);
	if (iter == chunks.end())
		return nullptr;
	return iter->second;
}

void World::insertChunk(Chunk *chunk) {
	using namespace vec_auto_cast;
	chunks.insert({chunk->cc, chunk});
	patchBorders(chunk);
	changedChunks.push_back(chunk->cc);
	if (chunks.size() % 100 == 0) LOG(INFO,
			"currently " << chunks.size() << " chunks are loaded");
}

void World::patchBorders(Chunk *c) {
	using namespace vec_auto_cast;
	bool chunkChanged[27];
	for (int i = 0; i < 27; i++) {
		chunkChanged[i] = false;
	}

	// TODO make more efficient
	for (int i = 0; i < 27; i++) {
		vec3i64 ncc = c->cc + CUBE_CYCLE[i];
		Chunk *nc = getChunk(ncc);
		if (!nc)
			continue;
		for (Face f : nc->getBorderFaces()) {
			vec3i64 nbc = ncc * Chunk::WIDTH + f.block;
			if (bc2cc(nbc - CUBE_CYCLE[i]) != c->cc)
				continue;
			if (updateFace(nbc, f.dir))
				chunkChanged[i] = true;
		}
	}

	for (uint8 d = 0; d < 6; d++) {
		uint8 invD = (d + 3) % 6;
		int dim = DIR_DIMS[d];
		vec3i64 ncc = c->cc + DIRS[d];
		Chunk *nc = getChunk(ncc);
		if (!nc)
			continue;

		vec3ui8 icc(0, 0, 0);
		vec3ui8 nicc(0, 0, 0);
		icc[dim] = (1 - d / 3) * (Chunk::WIDTH - 1);
		nicc[dim] = Chunk::WIDTH - 1 - icc[dim];

		for(uint8 a = 0; a < Chunk::WIDTH; a++) {
			for(uint8 b = 0; b < Chunk::WIDTH; b++) {
				icc[OTHER_DIR_DIMS[d][0]] = a;
				nicc[OTHER_DIR_DIMS[d][0]] = a;
				icc[OTHER_DIR_DIMS[d][1]] = b;
				nicc[OTHER_DIR_DIMS[d][1]] = b;

				uint8 type = c->getBlock(icc);
				if (nc->getBlock(nicc) != 0) {
					if (type == 0) {
						nc->addFace(Face{nicc, invD, getFaceCorners(ncc * Chunk::WIDTH + nicc, invD)});
						chunkChanged[DIR_2_CUBE_CYCLE[d]] = true;
					} else if (nc->removeFace(Face{nicc, invD, 0}))
						chunkChanged[DIR_2_CUBE_CYCLE[d]] = true;
				} else {
					if (type != 0)
						c->addFace(Face{icc, d, getFaceCorners(c->cc * Chunk::WIDTH + icc, d)});
					else
						c->removeFace(Face{icc, d, 0});
				}
			}
		}
	}
	for (int i = 0; i < 27; i++) {
		if(chunkChanged[i])
			changedChunks.push_back(c->cc + CUBE_CYCLE[i]);
	}
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

std::chrono::microseconds World::getTickingTime() {
	auto dur = dur_ticking;
	dur_ticking = microseconds::zero();
	return dur;
}
