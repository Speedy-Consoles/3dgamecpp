#include "chunk_loader.hpp"
#include "util.hpp"
#include "constants.hpp"
#include "chunk.hpp"

ChunkLoader::ChunkLoader(World *world, uint64 seed, bool updateFaces) : perlin(seed) {
	this->world = world;
	this->updateFaces = updateFaces;
}

void ChunkLoader::run() {
	using namespace vec_auto_cast;
	vec3i64 oldPcc[MAX_CLIENTS];
	uint playerChunkIndex[MAX_CLIENTS];
	int playerChunksLoaded[MAX_CLIENTS];
	for (uint8 i = 0; i < MAX_CLIENTS; i++) {
		playerChunkIndex[i] = 0;
		playerChunksLoaded[i] = 0;
	}
	int length = CHUNK_LOAD_RANGE * 2 + 1;
	int maxLoads = length * length * length;

	//while (!Thread.interrupted()) {
		int loads = 0;
		int oldLoads;
		do {
			oldLoads = loads;
			for (uint8 i = 0; i < MAX_CLIENTS; i++) {
				//if (Thread.interrupted())
				//	return;
				Player player = world->getPlayer(i);
				if (!player.isValid()) {
					playerChunkIndex[i] = 0;
					playerChunksLoaded[i] = 0;
					continue;
				}

				vec3i64 pcc = player.getChunkPos();
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
				if (world->getChunks().find(cc) == world->getChunks().end())
					loadChunk(cc);

				playerChunksLoaded[i]++;
				loads++;
			}
		} while (loads < MAX_LOADS_UNTIL_UNLOAD && loads > oldLoads);

		for (auto iter = world->getChunks().begin();
				iter != world->getChunks().end();) {
			vec3i64 cc = iter->first;
			bool inRange = false;
			for (uint8 i = 0; i < MAX_CLIENTS; i++) {
				if (!world->getPlayer(i).isValid())
					continue;
				if ((cc - oldPcc[i]).maxAbs() <= CHUNK_UNLOAD_RANGE) {
					inRange = true;
					break;
				}
			}
			if (!inRange)
				iter = world->getChunks().erase(iter);
			else
				iter++;
		}
		//Thread.sleep(100);
	//}
}

void ChunkLoader::loadChunk(vec3i64 cc) {
	uint8 blocks[Chunk::WIDTH * Chunk::WIDTH * Chunk::WIDTH];
	for (uint x = 0; x < Chunk::WIDTH; x++) {
		for (uint y = 0; y < Chunk::WIDTH; y++) {
			double sx = (cc[0] * Chunk::WIDTH + x) / perlinScale;
			double sy = (cc[1] * Chunk::WIDTH + y) / perlinScale;
			double ax = (cc[0] * Chunk::WIDTH + x) / perlinAreaScale;
			double ay = (cc[1] * Chunk::WIDTH + y) / perlinAreaScale;
			double h = perlin.octavePerlin(sx, sy, 0, 6, 0.5) * perlinScale
					* perlin.perlin(ax, ay, 0);
			for (uint z = 0; z < Chunk::WIDTH; z++) {
				int index = Chunk::getBlockIndex(vec3ui8(x, y, z));
				long wz = z + cc[2] * Chunk::WIDTH;
				if (wz > h) {
					blocks[index] = 0;
					continue;
				}
				double px = (cc[0] * Chunk::WIDTH + x) / perlinCaveScale;
				double py = (cc[1] * Chunk::WIDTH + y) / perlinCaveScale;
				double pz = (cc[2] * Chunk::WIDTH + z) / perlinCaveScale;
				double v = perlin.octavePerlin(px, py, pz, 6, 0.5);
				if (v > 0.12 * (3 - 1 / ((floor(h) - wz) / 10 + 1)))
					blocks[index] = 1;
				else
					blocks[index] = 0;
			}
		}
	}
	Chunk c = Chunk(cc);
	c.init(blocks);
	if (updateFaces)
		c.initFaces(*world);
	c.setReady();
	world->getChunks().insert({cc, c});
}
