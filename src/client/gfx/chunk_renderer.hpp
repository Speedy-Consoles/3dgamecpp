#ifndef CHUNK_RENDERER_HPP
#define CHUNK_RENDERER_HPP

#include <unordered_set>
#include <deque>

#include "shared/engine/vmath.hpp"
#include "shared/game/chunk.hpp"
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

class ChunkRenderer : public ComponentRenderer {
private:
	// performance limits
	static const int MAX_NEW_FACES = 3000;
	static const int MAX_NEW_CHUNKS = 100;
	static const int MAX_RENDER_QUEUE_SIZE= 500;
	static const int MAX_VS_CHUNKS = 5000;

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
		uint8 vsIns;
		uint8 vsOuts;
		uint vsInsVersion;
		uint vsOutsVersion;
		vec3i64 vsContent;
		bool vsInFringe;
	};

	// visibility search
	std::deque<vec3i64> vsQueue;
	vec3i64 vsPlayerChunk;
	int vsFringeCapacity = 0;
	vec3i64 *vsFringe = nullptr;
	int vsFringeSize = 0;
	int vsFringeStart = 0;
	int vsFringeEnd = 0;
	int *vsIndices = nullptr;
	uint vsCurrentVersion = 0;
	uint vsLastCompleteVersion = 0;

	// chunk requesting
	vec3i64 oldPlayerChunk;
	int checkChunkIndex = 0;
	std::unordered_set<vec3i64, size_t(*)(vec3i64)> inBuildQueue;
	std::deque<vec3i64> buildQueue;

	// performance info
	int newFaces = 0;
	int newChunks = 0;
	int faces = 0;
	int visibleChunks = 0;
	int visibleFaces = 0;

protected:
	Client *client;
	Renderer *renderer;

	// the grid
	GridInfo *chunkGrid = nullptr;

	// cache
	int renderDistance = 0;

public:
	ChunkRenderer(Client *client, Renderer *renderer);
	virtual ~ChunkRenderer();

	void setConf(const GraphicsConf &, const GraphicsConf &) override;
	void tick() override;
	void render() override;

	void rebuildChunk(vec3i64 chunkCoords);

	ChunkRendererDebugInfo getDebugInfo();

private:
	void buildChunk(const Chunk *chunks[27]);
	void visibilitySearch();
	int updateVsChunk(size_t index, vec3i64 chunkCoords);
	int getOuts(int ins, int passThroughs);

	bool inFrustum(vec3i64 cc, vec3i64 pos, vec3d lookDir);

protected:
	virtual void initRenderDistanceDependent(int renderDistance);
	virtual void destroyRenderDistanceDependent();

	virtual void beginRender() = 0;
	virtual void renderChunk(size_t index) = 0;
	virtual void finishRender() = 0;
	virtual void beginChunkConstruction() = 0;
	virtual void emitFace(vec3i64 bc, vec3i64 icc, uint blockType, uint faceDir, int shadowLevels[4]) = 0;
	virtual void finishChunkConstruction(size_t index) = 0;
};

#endif // CHUNK_RENDERER_HPP
