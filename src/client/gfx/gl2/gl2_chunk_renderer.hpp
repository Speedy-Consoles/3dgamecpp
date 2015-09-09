#ifndef GL2_CHUNK_RENDERER_HPP_
#define GL2_CHUNK_RENDERER_HPP_

#include "client/gfx/chunk_renderer.hpp"

#include <GL/glew.h>

#include "shared/engine/macros.hpp"
#include "shared/engine/std_types.hpp"
#include "shared/game/chunk.hpp"

class Client;
class GL2Renderer;
struct GraphicsConf;

class GL2ChunkRenderer : public ChunkRenderer {

	// face buffer for chunk rendering
	struct FaceVertexData {
		vec3f vertex[4];
		vec3f color[4];
		vec2f tex[4];
		vec3f normal;
	};

	struct FaceIndexData {
		GLuint tex;
		int index;
	};

	struct RenderInfo {
		GLuint dl = 0;
	};

	std::unordered_map<vec3i64, RenderInfo, size_t(*)(vec3i64)> renderInfos;

	// chunk construction state
	int numQuads = 0;
	FaceVertexData vb[(Chunk::WIDTH + 1) * Chunk::WIDTH * Chunk::WIDTH * 3];
	FaceIndexData faceIndexBuffer[(Chunk::WIDTH + 1) * Chunk::WIDTH * Chunk::WIDTH * 3];

public:
	GL2ChunkRenderer(Client *client, GL2Renderer *renderer);
	~GL2ChunkRenderer();

protected:
	void beginRender() override {}
	void renderChunk(vec3i64 chunkCoords) override;
	void finishRender() override {}
	void finishChunk(ChunkVisuals chunkVisuals) override;
	void destroyChunkData(vec3i64 chunkCoords) override;

//	void renderTarget();
//	void renderPlayers();
};

#endif //GL2_CHUNK_RENDERER_HPP_
