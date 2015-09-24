#ifndef CHUNK_SERVER_HPP
#define CHUNK_SERVER_HPP

#include "server.hpp"

class ChunkServer {
private:
	Server *server;
	ServerChunkManager *chunkManager;

public:
	ChunkServer(Server *server);
	~ChunkServer();

	void tick();

	void onChunkRequest(ChunkRequest &request, ChunkMessageJob job);
};

#endif // CHUNK_SERVER_HPP
