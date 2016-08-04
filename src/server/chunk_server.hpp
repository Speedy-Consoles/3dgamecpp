#ifndef CHUNK_SERVER_HPP
#define CHUNK_SERVER_HPP

#include <deque>

#include "server.hpp"

struct SingleChunkRequest {
	vec3i64 coords;
	bool cached;
	uint32 cachedRevision;
};

class ChunkServer {
private:
	Server *server;
	ServerChunkManager *chunkManager;

	std::deque<SingleChunkRequest> requestedQueue[MAX_CLIENTS];
	vec3i64 requestAnchors[MAX_CLIENTS];
	vec3i64 messageAnchors[MAX_CLIENTS];

	std::unique_ptr<uint8> encodedBuffer; // TODO make this obsolete

public:
	ChunkServer(Server *server);
	~ChunkServer();

	void tick();

	void onClientLeave(int id);
	void onChunkRequest(ChunkRequest request, int clientId);
	void onAnchorSet(ChunkAnchorSet anchorSet, int clientId);
};

#endif // CHUNK_SERVER_HPP
