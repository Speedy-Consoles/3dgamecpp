#ifndef CHUNK_SERVER_HPP
#define CHUNK_SERVER_HPP

#include "server.hpp"

class ChunkServer : Thread {
private:
	Server *server;
	ServerChunkManager *chunkManager;

	std::queue<ChunkMessageJob> requestedQueue;
	ProducerQueue<ChunkMessageJob> toEncodeQueue;
	ProducerQueue<ChunkMessageJob> encodedQueue;

public:
	ChunkServer(Server *server);
	~ChunkServer();

	void tick();
	virtual void doWork() override;

	void onChunkRequest(ChunkMessageJob job);
};

#endif // CHUNK_SERVER_HPP
