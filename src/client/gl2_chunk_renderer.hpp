#ifndef GL2_CHUNK_RENDERER_HPP_
#define GL2_CHUNK_RENDERER_HPP_

#include <GL/glew.h>

#include "game/chunk.hpp"

class Client;
class GL2Renderer;
struct GraphicsConf;

class GL2ChunkRenderer {
	enum DisplayListStatus {
		NO_CHUNK = 0,
		OUTDATED,
		OK,
	};

	static const int MAX_NEW_QUADS = 6000;
	static const int MAX_NEW_CHUNKS = 500;

	Client *client;
	GL2Renderer *renderer;

	int renderDistance = 0;

	// display lists
	GLuint dlFirstAddress;
	vec3i64 *dlChunks;
	uint8 *dlStatus;

	// chunk data
	int *chunkFaces;
	uint16 *chunkPassThroughs;

	// visibility search for rendering
	uint8 *vsExits;
	bool *vsVisited;
	int vsFringeCapacity;
	vec3i64 *vsFringe;
	int *vsIndices;

	// face buffer for chunk rendering
	int faceBufferIndices[255][(Chunk::WIDTH + 1) * Chunk::WIDTH * Chunk::WIDTH * 3];
	float faceBuffer[(Chunk::WIDTH + 1) * Chunk::WIDTH * Chunk::WIDTH * 3 * (3 + 4 * (2 + 3 + 3))];

	// performance stuff
	int newFaces = 0;
	int newChunks = 0;
	int faces = 0;
	int visibleChunks = 0;
	int visibleFaces = 0;

public:
	GL2ChunkRenderer(Client *client, GL2Renderer *renderer);
	~GL2ChunkRenderer();

	void setConf(const GraphicsConf &, const GraphicsConf &);
	void render();

private:
	void initRenderDistanceDependent();
	void destroyRenderDistanceDependent();

	void renderChunks();
	void renderChunk(Chunk &c);
	void renderTarget();
	void renderPlayers();
};

#endif //GL2_CHUNK_RENDERER_HPP_
