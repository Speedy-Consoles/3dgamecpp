#ifndef GL3_CHUNK_RENDERER_HPP_
#define GL3_CHUNK_RENDERER_HPP_

#include <GL/glew.h>

#include "client/client.hpp"
#include "shaders.hpp"
#include "config.hpp"
#include "engine/math.hpp"
#include "game/chunk.hpp"

class World;
class GL3Renderer;

class GL3ChunkRenderer {
private:
	enum ChunkStatus {
		NO_CHUNK = 0,
		OK,
	};

	struct ChunkInfo {
		// render info
		ChunkStatus status; // TODO maybe make compact?
		vec3i64 content;
		int numFaces;
		uint16 passThroughs;

		// visibility search
		bool vsVisited;
		uint8 vsExits;
	};

	Client *client;
	GL3Renderer *renderer;
	ShaderManager *shaderManager;

	// performance limits
	static const int MAX_NEW_Faces = 6000;
	static const int MAX_NEW_CHUNKS = 500;
	static const int MAX_CHUNK_CHECKS = 10000;

	// the grid
	ChunkInfo * chunkGrid;

	// vao, vbo locations
	GLuint *vaos;
	GLuint *vbos;

	// cache
	int visibleDiameter;

	// texture location
	GLuint blockTextures;

	// visibility search
	int vsFringeCapacity;
	vec3i64 *vsFringe;
	int *vsIndices;

	// chunk requesting
	vec3i64 oldPlayerChunk;
	int checkChunkIndex = 0;

	// performance info
	int newFaces = 0;
	int newChunks = 0;
	int faces = 0;
	int visibleChunks = 0;
	int visibleFaces = 0;
	int newlyCheckedChunks = 0;

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
	GL3ChunkRenderer(Client *client, GL3Renderer *renderer, ShaderManager *shaderManager);
	~GL3ChunkRenderer();

	void setConf(const GraphicsConf &, const GraphicsConf &);
	void render();

private:
	void loadTextures();
	void buildChunk(const Chunk &c);

	bool inFrustum(vec3i64 cc, vec3i64 pos, vec3d lookDir);

	void destroyRenderDistanceDependent();
	void initRenderDistanceDependent();
};

#endif // GL3_CHUNK_RENDERER_HPP_
