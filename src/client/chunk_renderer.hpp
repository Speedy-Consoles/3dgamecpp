#ifndef CHUNK_RENDERER_HPP
#define CHUNK_RENDERER_HPP

#include <GL/glew.h>

#include "client/client.hpp"
#include "shaders.hpp"
#include "config.hpp"
#include "engine/math.hpp"
#include "game/chunk.hpp"

class World;
class GL3Renderer;

class ChunkRenderer {
private:
	enum VAOStatus {
		NO_CHUNK = 0,
		OUTDATED,
		OK,
	};

	Client *client;
	GL3Renderer *renderer;
	Shaders *shaders;
	GraphicsConf conf;

	// performance limits
	static const int MAX_NEW_QUADS = 6000;
	static const int MAX_NEW_CHUNKS = 500;

	// vao, vbo locations
	GLuint *vaos;
	GLuint *vbos;

	// vao meta data
	vec3i64 *vaoChunks;
	uint8 *vaoStatus;

	// texture location
	GLuint blockTextures;

	// chunk data
	int *chunkFaces;
	uint16 *chunkPassThroughs;

	// visibility search for rendering
	uint8 *vsExits;
	bool *vsVisited;
	int vsFringeCapacity;
	vec3i64 *vsFringe;
	int *vsIndices;

	// performance info
	int newFaces = 0;
	int newChunks = 0;
	int faces = 0;
	int visibleChunks = 0;
	int visibleFaces = 0;

#pragma pack(push)
#pragma pack(1)
	// buffer for block vertices
	struct BlockVertexData {
		GLushort positionIndex;
		GLubyte textureIndex;
		GLubyte dirIndexCornerIndex;
		GLubyte shadowLevels;
	};
#pragma pack(pop)

	BlockVertexData blockVertexBuffer[Chunk::WIDTH * Chunk::WIDTH * (Chunk::WIDTH + 1) * 3 * 2 * 3];

public:
	ChunkRenderer(Client *client, GL3Renderer *renderer, Shaders *shaders);
	~ChunkRenderer();

	void setConf(const GraphicsConf &);
	void render();

private:
	void loadTextures();
	void buildChunk(Chunk &c);

	bool inFrustum(vec3i64 cc, vec3i64 pos, vec3d lookDir);

	void destroyRenderDistanceDependent();
	void initRenderDistanceDependent();
};

#endif // CHUNK_RENDERER_HPP
