#ifndef CHUNK_RENDERER_HPP
#define CHUNK_RENDERER_HPP

#include <unordered_set>
#include <deque>

#include "shared/engine/vmath.hpp"
#include "shared/game/chunk.hpp"
#include "client/client.hpp"
#include "client/config.hpp"

class Renderer;

struct ChunkRendererDebugInfo {
	int checkedDistance = 0;
	int newFaces = 0;
	int newChunks = 0;
	int totalFaces = 0;
	int visibleChunks = 0;
	int visibleFaces = 0;
	int renderQueueSize = 0;
};

class ChunkRenderer {
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
	Renderer *renderer;

	// visibility search
	int vsFringeCapacity = 0;
	vec3i64 *vsFringe = nullptr;
	int *vsIndices = nullptr;

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

protected:
	Client *client;

	// the grid
	GridInfo *chunkGrid = nullptr;

	// cache
	int renderDistance = 0;

public:
	ChunkRenderer(Client *client, Renderer *renderer);
	virtual ~ChunkRenderer();

	void init();

	void setConf(const GraphicsConf &, const GraphicsConf &);
	void tick();
	void render();

	void rerenderChunk(vec3i64 chunkCoords);

	ChunkRendererDebugInfo getDebugInfo();

private:
	void buildChunk(const Chunk *chunks[27]);

	bool inFrustum(vec3i64 cc, vec3i64 pos, vec3d lookDir);

protected:
	virtual void initRenderDistanceDependent(int renderDistance);
	virtual void destroyRenderDistanceDependent();

	virtual void beginRender() = 0;
	virtual void renderChunk(size_t index) = 0;
	virtual void finishRender() = 0;
	virtual void beginChunkConstruction() = 0;
	virtual void emitFace(vec3i64 icc, uint blockType, uint faceDir, uint8 shadowLevels) = 0;
	virtual void finishChunkConstruction(size_t index) = 0;
};

#endif // CHUNK_RENDERER_HPP
