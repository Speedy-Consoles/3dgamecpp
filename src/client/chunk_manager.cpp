#include "chunk_manager.hpp"

ChunkManager::ChunkManager() : inQueue(1024) {
	for (int i = 0; i < MAX_LISTENERS; i++) {
		outQueues[i] = new ProducerQueue<shared_ptr<const Chunk>>(1024);
	}
}

ChunkManager::~ChunkManager() {
	for (int i = 0; i < MAX_LISTENERS; i++) {
		delete outQueues[i];
	}
}

void ChunkManager::subscribe(vec3i64 chunkCoords, int listenerId) {
	inQueue.push(Subscription{chunkCoords, false, listenerId});
}

void ChunkManager::unsubscribe(vec3i64 chunkCoords, int listenerId) {
	inQueue.push(Subscription{chunkCoords, true, listenerId});
}

shared_ptr<const Chunk> ChunkManager::getNextChunk(int listenerId) {
	shared_ptr<const Chunk> sp;
	outQueues[listenerId].pop(sp);
	return sp;
}

void ChunkManager::run() {
	LOG(INFO, "ChunkManager thread dispatched");

	while (noStopIssued) {
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
		//
	} // while not thread interrupted

	LOG(INFO,"ChunkManager thread terminating");
}
