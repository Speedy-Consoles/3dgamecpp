/*
 * chunk_loader_internals.cpp
 *
 *  Created on: 17.09.2014
 *      Author: lars
 */

#include "chunk_loader.hpp"

#include "world_generator.hpp"
#include "game/world.hpp"

#include "logging.hpp"
#undef DEFAULT_LOGGER
#define DEFAULT_LOGGER NAMED_LOGGER("chunk")

using namespace std;
using namespace std::chrono;

void ChunkLoader::run() {
	LOG(INFO, "ChunkLoader thread dispatched");

	playerChunkIndex = 0;
	playerChunksLoaded = 0;
	updatePlayerInfo();

	while (!shouldHalt.load(memory_order_seq_cst)) {
		updateRenderDistance();
		if (!loadNextChunks()) {
			this_thread::sleep_for(milliseconds(100));
		}
		sendOffloadQueries();
		storeChunksOnDisk();
	} // while not thread interrupted

	LOG(INFO,"ChunkLoader thread terminating");
}

void ChunkLoader::updateRenderDistance() {
	uint renderDistance = this->renderDistance;
	uint newRenderDistance = this->newRenderDistance;
	if (newRenderDistance == renderDistance)
		return;

	LOG(INFO, "ChunkLoader thread render distance " << newRenderDistance);

	Chunk *chunk = nullptr;
	while ((chunk = getNextLoadedChunk()) != nullptr)
		chunk->free();
	storeChunksOnDisk();
	isLoaded.clear();

	playerChunkIndex = 0;
	playerChunksLoaded = 0;

	this->renderDistance = newRenderDistance;
}

bool ChunkLoader::loadNextChunks() {
	// don't wait for the reading-lock to be released
	if (!updatePlayerInfo(false) || !isPlayerValid)
		return false;

	for (int i = 0; i < 300; ++i) {
		uint64 length = renderDistance * 2 + 1;
		uint64 maxLoads = length * length * length;
		if (playerChunksLoaded > maxLoads)
			return false;

		vec3i64 ccd = getNextChunkToLoad();

		if (playerChunkIndex >= LOADING_ORDER.size())
			return false;

		vec3i64 cc = ccd + lastPcc;
		tryToLoadChunk(cc);

		playerChunksLoaded++;
	}
	return true;
}

vec3i64 ChunkLoader::getNextChunkToLoad() {
	vec3i64 cc;
	while (playerChunkIndex < LOADING_ORDER.size()) {
		cc = LOADING_ORDER[playerChunkIndex++].cast<int64>();
		if (cc.maxAbs() <= (int) renderDistance)
			return cc;
	}
	return {0, 0, 0};
}

void ChunkLoader::tryToLoadChunk(vec3i64 cc) {
	auto iter = isLoaded.find(cc);
	if (iter == isLoaded.end()) {
		isLoaded.insert(cc);
		Chunk *chunk = allocateChunk(cc);
		if (!chunkArchive.loadChunk(cc, *chunk))
			gen->generateChunk(cc, *chunk);
		if (updateFaces)
			chunk->initFaces();
		while (!queue.push(chunk) && !shouldHalt.load(memory_order_seq_cst)) {
			LOG(WARNING, "Output queue is full");
			this_thread::sleep_for(milliseconds(100));
		}
	}
}

void ChunkLoader::storeChunksOnDisk() {
	int counter = 0;
	auto deletedChunksList = deletedChunks.consumeAll();
	while (deletedChunksList)
	{
		Chunk *chunk = deletedChunksList->data;
		chunkArchive.storeChunk(*chunk);
		deallocateChunk(chunk);

		auto tmp = deletedChunksList->next;
		delete deletedChunksList;
		deletedChunksList = tmp;
		counter++;
	}

	if (counter > 0) {
		playerChunkIndex = 0;
		playerChunksLoaded = 0;
	}
}

void ChunkLoader::sendOffloadQueries() {
	updatePlayerInfo();

	for (auto iter = isLoaded.begin(); iter != isLoaded.end();) {
		vec3i64 cc = *iter;
		bool inRange = false;

		if (!isPlayerValid)
			continue;
		if ((cc - lastPcc).maxAbs() <= (int) renderDistance + 1) {
			inRange = true;
		}
		if (!inRange) {
			unloadQueries.push(cc);
			iter = isLoaded.erase(iter);
		} else
			iter++;
	}
}

bool ChunkLoader::updatePlayerInfo(bool wait) {
	int handle;
	bool valid;
	vec3i64 pcc;
	Player &player = world->getPlayer(localPlayer);
	Monitor &validPosMonitor = player.getValidPosMonitor();
	bool success = false;
	do {
		handle = validPosMonitor.startRead();
		valid = player.isValid();
		pcc = player.getChunkPos();
		success = validPosMonitor.finishRead(handle);
	} while (!success && wait);
	if (!success)
		return false;

	if (!valid || lastPcc != pcc) {
		playerChunkIndex = 0;
		playerChunksLoaded = 0;
	}

	lastPcc = pcc;
	isPlayerValid = valid;
	return true;
}

Chunk *ChunkLoader::allocateChunk(vec3i64 cc) {
	Chunk *chunk = nullptr;
	if (!chunkPool.empty()) {
		chunk = chunkPool.back();
		chunkPool.pop_back();
	} else {
		size_t size = sizeof (Chunk);
		void *memory = ::operator new(size);
		chunk = reinterpret_cast<Chunk *>(memory);
	}
	new(chunk) Chunk(cc, this);
	return chunk;
}

void ChunkLoader::deallocateChunk(Chunk *chunk) {
	bool keep = true;
	if (keep) {
		chunk->~Chunk();
		chunkPool.push_back(chunk);
	} else {
		::operator delete(chunk);
	}
}

void ChunkLoader::clearChunkPool() {
	for (Chunk *chunk : chunkPool) {
		::operator delete(chunk);
	}
	chunkPool.clear();
}
