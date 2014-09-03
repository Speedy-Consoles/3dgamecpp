#include "chunk_loader.hpp"

#include <cstring>

#include <thread>
#include <future>
#include <chrono>

#include "util.hpp"
#include "constants.hpp"
#include "chunk.hpp"
#include "monitor.hpp"

using namespace std;

ChunkLoader::ChunkLoader(World *world, uint64 seed, bool updateFaces) :
		perlin(seed), isLoaded(0, vec3i64HashFunc), queue(1000) {
	this->world = world;
	this->updateFaces = updateFaces;
}

ChunkLoader::~ChunkLoader() {
	Chunk *chunk = nullptr;
	while ((chunk = next()) != nullptr) {
		delete chunk;
	}
}

void ChunkLoader::dispatch() {
	using namespace vec_auto_cast;

	shouldHalt = false;

	fut = async(launch::async, [this]() {

		vec3i64 oldPcc[MAX_CLIENTS];
		memset(oldPcc, 0, MAX_CLIENTS * sizeof (vec3i64));

		uint playerChunkIndex[MAX_CLIENTS];
		int playerChunksLoaded[MAX_CLIENTS];
		for (uint8 i = 0; i < MAX_CLIENTS; i++) {
			playerChunkIndex[i] = 0;
			playerChunksLoaded[i] = 0;
		}

		const int length = CHUNK_LOAD_RANGE * 2 + 1;
		const int maxLoads = length * length * length;

		while (!shouldHalt.load(memory_order_seq_cst)) {
			int loads = 0;
			int oldLoads;
			do {
				oldLoads = loads;
				for (uint8 i = 0; i < MAX_CLIENTS; i++) {
					Player &player = world->getPlayer(i);
					Monitor &validPosMonitor = player.getValidPosMonitor();
					int handle = validPosMonitor.startRead();
					bool valid = player.isValid();
					vec3i64 pcc = player.getChunkPos();
					if(!validPosMonitor.finishRead(handle))
						continue;

					if (!valid) {
						playerChunkIndex[i] = 0;
						playerChunksLoaded[i] = 0;
						continue;
					}

					if (oldPcc[i] != pcc) {
						playerChunkIndex[i] = 0;
						playerChunksLoaded[i] = 0;
						oldPcc[i] = pcc;
					}

					if (playerChunkIndex[i] >= LOADING_ORDER.size()
							|| playerChunksLoaded[i] > maxLoads)
						continue;

					vec3i8 ccd = LOADING_ORDER[playerChunkIndex[i]++];
					while (ccd.maxAbs() > CHUNK_LOAD_RANGE
							&& playerChunkIndex[i] < LOADING_ORDER.size())
						ccd = LOADING_ORDER[playerChunkIndex[i]++];

					if (playerChunkIndex[i] >= LOADING_ORDER.size())
						continue;

					vec3i64 cc = ccd + pcc;
					auto iter = isLoaded.find(cc);
					if (iter == isLoaded.end()) {
						isLoaded.insert(cc);
						Chunk *chunk = loadChunk(cc);
						while (!queue.push(chunk))
							this_thread::yield();
					}

					playerChunksLoaded[i]++;
					loads++;
				}
			} while (loads < MAX_LOADS_UNTIL_UNLOAD && loads > oldLoads && !shouldHalt.load(memory_order_seq_cst));

			bool valid[MAX_CLIENTS];

			for (uint8 i = 0; i < MAX_CLIENTS; i++) {
				Player &player = world->getPlayer(i);
				Monitor &validPosMonitor = player.getValidPosMonitor();
				int handle;
				do {
					handle = validPosMonitor.startRead();
					valid[i] = player.isValid();
				} while (!validPosMonitor.finishRead(handle));
			}

			for (auto iter = isLoaded.begin(); iter != isLoaded.end();) {
				vec3i64 cc = *iter;
				bool inRange = false;
				for (uint8 i = 0; i < MAX_CLIENTS; i++) {
					if (!valid[i])
						continue;
					if ((cc - oldPcc[i]).maxAbs() <= CHUNK_UNLOAD_RANGE) {
						inRange = true;
						break;
					}
				}
				if (!inRange) {
					unloadQueries.push(cc);
					iter = isLoaded.erase(iter);
				}
				else
					iter++;
			}
		} // while not thread interrupted

	}); // lambda end
}

void ChunkLoader::requestTermination() {
	 shouldHalt.store(true, memory_order_relaxed);
}


void ChunkLoader::wait() {
	requestTermination();
	fut.get();
}

Chunk *ChunkLoader::next() {
	Chunk *chunk;
	bool result = queue.pop(chunk);
	return result ? chunk : nullptr;
}

ProducerStack<vec3i64>::Node *ChunkLoader::getUnloadQueries() {
	return unloadQueries.consumeAll();
}

Chunk *ChunkLoader::loadChunk(vec3i64 cc) {
	uint8 blocks[Chunk::WIDTH * Chunk::WIDTH * Chunk::WIDTH];
	for (uint ix = 0; ix < Chunk::WIDTH; ix++) {
		for (uint iy = 0; iy < Chunk::WIDTH; iy++) {
			long x = round((cc[0] * Chunk::WIDTH + ix) / overAllScale);
			long y = round((cc[1] * Chunk::WIDTH + iy) / overAllScale);
			double ax = x / perlinAreaXYScale;
			double ay = y / perlinAreaXYScale;
			double ap = perlin.perlin(ax, ay, 0);
			double mFac = (1 + tanh((ap - perlinAreaMountainThreshold)
					* perlinAreaSharpness)) / 2;
			double fFac = 1 - mFac;

			double mx = x / perlinMountainXYScale;
			double my = y / perlinMountainXYScale;
			double mh = perlin.octavePerlin(mx, my, 0,
					perlinMountainOctaves, perlinMountainExp)
					* perlinMountainMaxHeight;

			double fx = x / perlinFlatlandXYScale;
			double fy = y / perlinFlatlandXYScale;
			double fh = perlin.octavePerlin(fx, fy, 0,
					perlinFlatlandOctaves, perlinFlatlandExp)
					* perlinFlatLandMaxHeight;

			double h = fh * fFac + mh * mFac;
			for (uint iz = 0; iz < Chunk::WIDTH; iz++) {
				int index = Chunk::getBlockIndex(vec3ui8(ix, iy, iz));
				long wz = iz + cc[2] * Chunk::WIDTH;
				if (wz > h) {
					blocks[index] = 0;
					continue;
				}
				double px = (cc[0] * Chunk::WIDTH + ix) / perlinCaveScale;
				double py = (cc[1] * Chunk::WIDTH + iy) / perlinCaveScale;
				double pz = (cc[2] * Chunk::WIDTH + iz) / perlinCaveScale;
				double v = perlin.octavePerlin(px, py, pz, 6, 0.5);
				if (v > 0.12 * (3 - 1 / ((floor(h) - wz) / 10 + 1)))
					blocks[index] = 1;
				else
					blocks[index] = 0;
			}
		}
	}
	Chunk *chunk = new Chunk(cc);
	chunk->init(blocks);
	// TODO this is not threadsafe
	if (updateFaces)
		chunk->initFaces(*world);
	return chunk;
}
