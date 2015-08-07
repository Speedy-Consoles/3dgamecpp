#include "chunk_manager.hpp"
#include "engine/logging.hpp"
#include "engine/time.hpp"
#include "client/client.hpp"
#include "client/server_interface.hpp"

using namespace std;

ChunkManager::ChunkManager(Client *client) :
	chunks(0, vec3i64HashFunc),
	needCounter(0, vec3i64HashFunc),
	loadedQueue(1024),
	toLoadQueue(1024),
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
	while (chunks.size() < MAX_LOADED_CHUNKS && loadedQueue.pop(chunk)) {
		auto it = needCounter.find(chunk->getCC());
		if (it != needCounter.end())
			chunks.insert({chunk->getCC(), chunk});
		else
			delete chunk;
	}

	while (!preToLoadQueue.empty()) {
		vec3i64 cc = preToLoadQueue.front();
		if (!toLoadQueue.push(cc))
			break;
		preToLoadQueue.pop();
	}
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
		preToLoadQueue.push(chunkCoords);
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
	LOG(INFO, "ChunkManager thread dispatched");

	while (!shouldHalt.load(memory_order_seq_cst)) {
		// FOR THE FUTURE:
		// handle chunk data from server
			// if chunk differs from cache / is not cached
				// cache chunk
				// put chunk into outQueue of all listeners listening to it via chunkListeners map
			// else
				// ignore chunk

		// handle inQueue
			// add subscription to chunkListeners map
			// if chunk is cached
				// or put cached chunk into
			// else
				// send chunk request to server

		//FOR NOW:
		vec3i64 cc;
		if (toLoadQueue.pop(cc)) {
			// TODO this part is, like, totally hacky
			client->getServerInterface()->requestChunk(cc);
			Chunk *chunk = client->getServerInterface()->getNextChunk();
			chunk->makePassThroughs();
			while (!loadedQueue.push(chunk) && !shouldHalt.load(memory_order_seq_cst)) {
				sleepFor(millis(50));
			}
		} else {
			sleepFor(millis(100));
		}
	} // while not thread interrupted

	LOG(INFO,"ChunkManager thread terminating");
}
