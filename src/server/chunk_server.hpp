#ifndef CHUNK_SERVER_HPP
#define CHUNK_SERVER_HPP

#include "server.hpp"

struct TaggedChunkRequest {
	int clientId;
	vec3i64 coords;
	bool cached;
	uint32 cachedRevision;
};

class ChunkServer {
private:
	Server *server;
	ServerChunkManager *chunkManager;

	std::queue<TaggedChunkRequest> requestedQueue;
	vec3i64 anchors[MAX_CLIENTS];

public:
	ChunkServer(Server *server);
	~ChunkServer();

	void tick();

	void onClientLeave(int id);
	void onChunkRequest(ChunkRequest request, int clientId);
	void onAnchorSet(ChunkAnchorSet anchorSet, int clientId);
};

#endif // CHUNK_SERVER_HPP
