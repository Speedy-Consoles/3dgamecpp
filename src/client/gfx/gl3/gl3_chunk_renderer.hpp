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

	// performance limits
	static const int MAX_NEW_FACES = 3000;
	static const int MAX_NEW_CHUNKS = 100;
	static const int MAX_RENDER_QUEUE_SIZE= 500;

	// the grid
	GridInfo *chunkGrid;

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
	std::unordered_set<vec3i64, size_t(*)(vec3i64)> inRenderQueue;
	std::deque<vec3i64> renderQueue;

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
	GL3ChunkRenderer(Client *client, GL3Renderer *renderer, ShaderManager *shaderManager);
	~GL3ChunkRenderer();

	void setConf(const GraphicsConf &, const GraphicsConf &);
	void render();

	void rerenderChunk(vec3i64 chunkCoords);

	ChunkRendererDebugInfo getDebugInfo();

private:
	void loadTextures();
	void buildChunk(Chunk const *chunkInfos[27]);

	bool inFrustum(vec3i64 cc, vec3i64 pos, vec3d lookDir);

	void destroyRenderDistanceDependent();
	void initRenderDistanceDependent(int renderDistance);
};

#endif // GL3_CHUNK_RENDERER_HPP_
