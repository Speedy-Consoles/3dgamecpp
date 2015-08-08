#include "chunk_manager.hpp"
#include "engine/logging.hpp"
#include "engine/time.hpp"
#include "client/client.hpp"
#include "client/server_interface.hpp"

using namespace std;

ChunkManager::ChunkManager(Client *client) :
	chunks(0, vec3i64HashFunc),
	needCounter(0, vec3i64HashFunc),
	client(client)
{
	// nothing
}

ChunkManager::~ChunkManager() {
	for (auto it = chunks.begin(); it != chunks.end(); ++it) {
		delete it->second;
	}
}

void ChunkManager::dispatch() {
	shouldHalt = false;
	fut = async(launch::async, [this]() { run(); });
}

void ChunkManager::tick() {
	Chunk *chunk;
	while (chunks.size() < MAX_LOADED_CHUNKS && (chunk = client->getServerInterface()->getNextChunk())) {
		auto it = needCounter.find(chunk->getCC());
		if (it != needCounter.end())
			chunks.insert({chunk->getCC(), chunk});
		else
			delete chunk;
	}
}

void ChunkManager::placeBlock(vec3i64 blockCoords, uint8 blockType) {
	vec3i64 cc = bc2cc(blockCoords);
	auto it = chunks.find(cc);
	if (it != chunks.end())
		it->second->setBlock(bc2icc(blockCoords), blockType);
	// TODO also operate on cache
}

const Chunk *ChunkManager::getChunk(vec3i64 chunkCoords) const {
	auto it = chunks.find(chunkCoords);
	if (it != chunks.end())
		return it->second;
	return nullptr;
}

void ChunkManager::requestChunk(vec3i64 chunkCoords) {
	auto it = needCounter.find(chunkCoords);
	if (it == needCounter.end()) {
		needCounter.insert({chunkCoords, 1});
		client->getServerInterface()->requestChunk(chunkCoords);
	} else {
		it->second++;
	}
}

void ChunkManager::releaseChunk(vec3i64 chunkCoords) {
	auto it1 = needCounter.find(chunkCoords);
	if (it1 != needCounter.end()) {
		it1->second--;
		if (it1->second == 0) {
			needCounter.erase(it1);
			auto it2 = chunks.find(chunkCoords);
			if (it2 != chunks.end()) {
				delete it2->second;
				chunks.erase(it2);
			}
		}
	}
}

int ChunkManager::getNumNeededChunks() const {
	return needCounter.size();
}

int ChunkManager::getNumLoadedChunks() const {
	return chunks.size();
}

void ChunkManager::requestTermination() {
	 shouldHalt.store(true, memory_order_relaxed);
}

void ChunkManager::wait() {
	requestTermination();
	if (fut.valid())
		fut.get();
}

void ChunkManager::run() {
	sleepFor(millis(100));
}
