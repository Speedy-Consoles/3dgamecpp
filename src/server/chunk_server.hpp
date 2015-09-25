#ifndef CHUNK_SERVER_HPP
#define CHUNK_SERVER_HPP

#include "server.hpp"

struct TaggedChunkRequest {
	int clientId;
	ChunkRequest request;
};

class ChunkServer {
private:
	Server *server;
	ServerChunkManager *chunkManager;

	std::queue<TaggedChunkRequest> requestedQueue;

public:
	ChunkServer(Server *server);
	~ChunkServer();

	void tick();

	void onClientLeave(int id);
	void onChunkRequest(ChunkRequest request, int clientId);
};

#endif // CHUNK_SERVER_HPP
