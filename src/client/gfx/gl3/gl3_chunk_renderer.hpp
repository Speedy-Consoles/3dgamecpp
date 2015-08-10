#ifndef GL3_CHUNK_RENDERER_HPP_
#define GL3_CHUNK_RENDERER_HPP_

#include <memory>
#include <unordered_set>
#include <queue>

#include <GL/glew.h>

#include "shared/engine/math.hpp"
#include "shared/game/chunk.hpp"
#include "client/client.hpp"
#include "client/config.hpp"

#include "gl3_shaders.hpp"

class World;
class GL3Renderer;

struct ChunkRendererDebugInfo {
	int checkedDistance = 0;
	int newFaces = 0;
	int newChunks = 0;
	int totalFaces = 0;
	int visibleChunks = 0;
	int visibleFaces = 0;
	int renderQueueSize = 0;
};

class GL3ChunkRenderer {
private:
	// performance limits
	static const int MAX_NEW_FACES = 3000;
	static const int MAX_NEW_CHUNKS = 100;
	static const int MAX_RENDER_QUEUE_SIZE= 500;

	enum ChunkStatus {
		NO_CHUNK = 0,
		OUTDATED,
		OK,
	};

	struct GridInfo {
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

	// the grid
	GridInfo *chunkGrid;

	// vao, vbo locations
	GLuint *vaos;
	GLuint *vbos;

	// texture location
	GLuint blockTextures;

	// visibility search
	int vsFringeCapacity;
	vec3i64 *vsFringe;
	int *vsIndices;

	// chunk requesting
	vec3i64 oldPlayerChunk;
	int checkChunkIndex = 0;
	std::unordered_set<vec3i64, size_t(*)(vec3i64)> inRenderQueue;
	std::deque<vec3i64> renderQueue;

	// performance info
	int newFaces = 0;
	int newChunks = 0;
	int faces = 0;
	int visibleChunks = 0;
	int visibleFaces = 0;

	// chunk construction state
	size_t bufferSize;
	glm::mat4 playerTranslationMatrix;

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

protected:

	// cache
	int renderDistance;

public:
	GL3ChunkRenderer(Client *client, GL3Renderer *renderer, ShaderManager *shaderManager);
	~GL3ChunkRenderer();

	void setConf(const GraphicsConf &, const GraphicsConf &);
	void render();

	void rerenderChunk(vec3i64 chunkCoords);

	ChunkRendererDebugInfo getDebugInfo();

private:
	void loadTextures();

	void buildChunk(const Chunk *chunks[27]);

	void beginRender();
	void renderChunk(size_t index);
	void finishRender();
	void beginChunkConstruction();
	void emitFace(vec3i64 icc, uint blockType, uint faceDir, uint8 shadowLevels);
	void finishChunkConstruction(size_t index);

	bool inFrustum(vec3i64 cc, vec3i64 pos, vec3d lookDir);

	void initRenderDistanceDependent(int renderDistance);
	void destroyRenderDistanceDependent();
};

#endif // GL3_CHUNK_RENDERER_HPP_
