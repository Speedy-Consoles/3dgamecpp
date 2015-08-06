#ifndef CHUNK_MANAGER_HPP
#define CHUNK_MANAGER_HPP

#include <unordered_map>
#include <memory>
#include <atomic>
#include <future>

#include "util.hpp"
#include "engine/vmath.hpp"
#include "engine/queue.hpp"
#include "io/archive.hpp"
#include "game/chunk.hpp"

class Client;

enum {
	GRAPHICS_LISTENER_ID = 0,
	WORLD_LISTENER_ID,
	MAX_LISTENERS
};

class ChunkManager {
	struct Request {
		vec3i64 chunkCoords;
		int listenerId;
	};

	ProducerQueue<std::shared_ptr<const Chunk>> *outQueues[MAX_LISTENERS];
	ProducerQueue<Request> inQueue;

	std::atomic<bool> shouldHalt;
	std::future<void> fut;

	Client *client = nullptr;

public:
	ChunkManager(Client *client);
	~ChunkManager();

	void dispatch();

	bool request(vec3i64 chunkCoords, int listenerId);

	std::shared_ptr<const Chunk> getNextChunk(int listenerId);
	void requestTermination();
	void wait();

private:
	void run();
};

#endif /* CHUNK_MANAGER_HPP */
