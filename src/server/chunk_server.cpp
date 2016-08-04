#include "chunk_server.hpp"

#include "shared/chunk_compression.hpp"

static logging::Logger logger("cserver");

ChunkServer::ChunkServer(Server *server) : server(server),
		encodedBuffer(new uint8[Chunk::SIZE * MAX_CHUNKS_PER_MESSAGE])
{
	LOG_INFO(logger) << "Creating chunk server";
	chunkManager = server->getChunkManager();
	for (int i = 0; i < MAX_CLIENTS; i++) {
		requestAnchors[i] = vec3i64(0, 0, 0);
		messageAnchors[i] = vec3i64(0, 0, 0);
	}
}

ChunkServer::~ChunkServer() {
	// nothing
}

void ChunkServer::tick() {
	for (int i = 0; i < MAX_CLIENTS; i++) {
		ChunkMessage msg;
		uint msgChunks = 0;
		msg.encodedBuffer = encodedBuffer.get();
		uint8 *eb = encodedBuffer.get();
		while(!requestedQueue[i].empty()) {
			SingleChunkRequest scr = requestedQueue[i].front();
			const Chunk *chunk = chunkManager->getChunk(scr.coords);
			if (!chunk)
				break;
			requestedQueue[i].pop_front();
			vec3i64 anchorDiff = scr.coords - messageAnchors[i];
			bool smallEnough = true;
			int64 limit = 1 << 3;
			for (int j = 0; j < 3; j++) {
				if (anchorDiff[j] >= limit or anchorDiff[j] < -limit) {
					smallEnough = false;
					break;
				}
			}
			if ((!smallEnough && msgChunks > 0) || msgChunks >= MAX_CHUNKS_PER_REQUEST) {
				msg.numChunks = msgChunks;
				server->send(msg, i, CHANNEL_BLOCK_DATA, true);
				eb = encodedBuffer.get();
				msgChunks = 0;
			}
			if (!smallEnough) {
				ChunkAnchorSet anchorSet;
				anchorSet.coords = scr.coords;
				server->send(anchorSet, i, CHANNEL_BLOCK_DATA, true);
				messageAnchors[i] = scr.coords;
			}
			msg.chunkMessageData[msgChunks].relCoords = scr.coords - messageAnchors[i];
			msg.chunkMessageData[msgChunks].revision = chunk->getRevision();
			if (!scr.cached || chunk->getRevision() != scr.cachedRevision) {
				size_t el = encodeBlocks_RLE(chunk->getBlocks(), eb, Chunk::SIZE);
				msg.chunkMessageData[msgChunks].encodedLength = el;
				eb += el;
			} else {
				msg.chunkMessageData[msgChunks].encodedLength = 0;
			}
			msgChunks++;
			chunkManager->releaseChunk(scr.coords);
		}
		if(msgChunks > 0) {
			msg.numChunks = msgChunks;
			server->send(msg, i, CHANNEL_BLOCK_DATA, true);
		}
	}
}

void ChunkServer::onClientLeave(int id) {
	requestAnchors[id] = vec3i64(0, 0, 0);
	messageAnchors[id] = vec3i64(0, 0, 0);
	requestedQueue[id].clear();
}

void ChunkServer::onChunkRequest(ChunkRequest request, int clientId) {
	// TODO check for revision first
	// TODO request compressed blocks instead of whole chunk
	for (uint i = 0; i < request.numChunks; i++) {
		ChunkRequestData &crd = request.chunkRequestData[i];
		vec3i64 coords = crd.relCoords + requestAnchors[clientId];
		chunkManager->requireChunk(coords);
		requestedQueue[clientId].push_back(SingleChunkRequest{
				coords,
				crd.cached,
				crd.cachedRevision,
		});
	}
}

void ChunkServer::onAnchorSet(ChunkAnchorSet anchorSet, int clientId) {
	requestAnchors[clientId] = anchorSet.coords;
}
