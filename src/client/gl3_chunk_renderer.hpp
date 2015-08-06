#ifndef GL3_CHUNK_RENDERER_HPP_
#define GL3_CHUNK_RENDERER_HPP_

#include <GL/glew.h>
#include <memory>
#include <unordered_map>
#include <queue>

#include "client/client.hpp"
#include "shaders.hpp"
#include "config.hpp"
#include "engine/math.hpp"
#include "game/chunk.hpp"

class World;
class GL3Renderer;

struct ChunkRendererDebugInfo {
	int newFaces = 0;
	int newChunks = 0;
	int totalFaces = 0;
	int visibleChunks = 0;
	int visibleFaces = 0;
	int chunkMapSize = 0;
	int renderQueueSize = 0;
	int requestQueueSize = 0;
	int holdChunks = 0;
};

class GL3ChunkRenderer {
private:
	enum ChunkStatus {
		NO_CHUNK = 0,
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

	struct ChunkInfo {
		bool inRenderQueue;
		int needCounter = 1;
		std::shared_ptr<const Chunk> chunkPointer = nullptr;

		ChunkInfo(bool inRenderQueue) : inRenderQueue(inRenderQueue) {
			// nothng
		}
	};

	Client *client;
	GL3Renderer *renderer;
	ShaderManager *shaderManager;

	// performance limits
	static const int MAX_NEW_FACES = 3000;
	static const int MAX_NEW_CHUNKS = 100;
	static const int MAX_RENDER_QUEUE_SIZE= 2000;
	static const int MAX_CHUNK_MAP_SIZE= 10000;

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
	using ChunkMap = std::unordered_map<vec3i64, ChunkInfo, size_t(*)(vec3i64)>;
	ChunkMap chunkMap;
	std::deque<vec3i64> renderQueue;
	std::deque<vec3i64> toRequest;

	// performance info
	int newFaces = 0;
	int newChunks = 0;
	int faces = 0;
	int visibleChunks = 0;
	int visibleFaces = 0;
	int holdChunks = 0;

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

	ChunkRendererDebugInfo getDebugInfo();

private:
	void loadTextures();
	void buildChunk(Chunk const *chunkInfos[27]);

	bool inFrustum(vec3i64 cc, vec3i64 pos, vec3d lookDir);

	void destroyRenderDistanceDependent();
	void initRenderDistanceDependent(int renderDistance);
};

#endif // GL3_CHUNK_RENDERER_HPP_
