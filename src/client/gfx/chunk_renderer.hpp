#ifndef CHUNK_RENDERER_HPP
#define CHUNK_RENDERER_HPP

#include <map>
#include <unordered_map>
#include <unordered_set>
#include <deque>
#include <queue>

#include "shared/engine/vmath.hpp"
#include "shared/engine/queue.hpp"
#include "shared/game/chunk.hpp"
#include "shared/chunk_manager.hpp"
#include "client/client.hpp"
#include "client/gfx/component_renderer.hpp"

struct GraphicsConf;
class Renderer;

struct ChunkRendererDebugInfo {
	int checkedDistance = 0;
	int newFaces = 0;
	int newChunks = 0;
	int totalFaces = 0;
	int visibleChunks = 0;
	int visibleFaces = 0;
	int buildQueueSize = 0;
};

class ChunkRenderer : public ComponentRenderer, public Thread {
private:
	// performance limits
	// must be smaller than ChunkManager::CHUNK_POOL_SIZE / 27
	static const int MAX_BUILD_QUEUE_SIZE =
			ChunkManager::CHUNK_POOL_SIZE / 27 > 1000 ?
			1000 : ChunkManager::CHUNK_POOL_SIZE / 27;
	static const int MAX_VS_CHUNKS = 3000;

	struct ChunkArea {
		const Chunk *chunks[27];
	};

	struct ChunkBuildInfo {
		uint32 revision = 0;
		int numFaces = 0;
		uint16 passThroughs = 0;
	};

	struct ChunkVSInfo {
		// visibility search
		uint8 ins = 0;
		uint8 outs = 0;
		uint insVersion = 0;
		uint outsVersion = 0;
	};

protected:
	struct Quad {
		vec3i64 bc;
		vec3ui8 icc;
		uint faceType;
		uint faceDir;
		int shadowLevels[4];
	};

	struct ChunkVisuals {
		vec3i64 cc;
		uint32 revision;
		std::vector<Quad> quads;
	};

private:
	// requesting
	vec3i64 oldPlayerChunk;
	int checkChunkIndex = 0;
	std::unordered_set<vec3i64, size_t(*)(vec3i64)> inBuildQueue;
	std::deque<vec3i64> buildQueue;

	// building
	ProducerQueue<ChunkArea> toBuildQueue;
	ProducerQueue<ChunkVisuals> toFinishQueue;
	std::unordered_map<vec3i64, ChunkBuildInfo, size_t(*)(vec3i64)> builtChunks;

	// visibility search
	std::deque<vec3i64> changedChunksQueue;
	vec3i64 vsPlayerChunk;
	std::unordered_map<vec3i64, ChunkVSInfo, size_t(*)(vec3i64)> vsChunks;
	std::unordered_set<vec3i64, size_t(*)(vec3i64)> vsInFringe;
	int vsFringeCapacity = 0;
	std::queue<std::unordered_map<vec3i64, ChunkVSInfo, size_t (*)(vec3i64)>::iterator> vsFringe;
	uint vsCurrentVersion = 0;
	int vsRenderDistance = 0;
	bool newVs = false;

	// rendering
	std::map<vec3i64, vec3i64, bool(*)(vec3i64, vec3i64)> renderChunks[2];
	int renderChunksPage = 0;

	// performance info
	int newFaces = 0;
	int newChunks = 0;
	int numFaces = 0;
	int visibleChunks = 0;
	int visibleFaces = 0;

protected:
	Client *client;
	Renderer *renderer;

	// cache
	int renderDistance = 0;

public:
	ChunkRenderer(Client *client, Renderer *renderer);
	virtual ~ChunkRenderer();

	void setConf(const GraphicsConf &, const GraphicsConf &) override;
	void tick() override;
	void render() override;

	virtual void doWork() override;

	void rebuildChunk(vec3i64 chunkCoords);

	ChunkRendererDebugInfo getDebugInfo();

private:
	ChunkVisuals buildChunk(ChunkArea area);
	bool getChunkArea(vec3i64 chunkCoordinates, ChunkArea *area);
	bool chunkHasQuads(ChunkArea area);
	void finishChunk(ChunkVisuals);
	void visibilitySearch();
	int updateVsChunk(vec3i64 chunkCoords, ChunkVSInfo *vsInfo, int passThroughs);
	int getOuts(int ins, int passThroughs, vec3i64 chunkDiff, int tolerance);

	bool inFrustum(vec3i64 cc, vec3i64 pos, vec3d lookDir);

protected:
	virtual void beginRender() = 0;
	virtual void renderChunk(vec3i64 chunkCoords) = 0;
	virtual void finishRender() = 0;
	virtual void applyChunkVisuals(ChunkVisuals chunkVisuals) = 0;
	virtual void destroyChunkData(vec3i64 chunkCoords) = 0;
};

#endif // CHUNK_RENDERER_HPP
