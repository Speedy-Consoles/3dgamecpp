#ifndef GL3_CHUNK_RENDERER_HPP_
#define GL3_CHUNK_RENDERER_HPP_

#include "client/gfx/chunk_renderer.hpp"

#include "gl3_shaders.hpp"
#include "gl3_texture_manager.hpp"

class GL3Renderer;

class GL3ChunkRenderer : public ChunkRenderer {
private:

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

	struct RenderInfo {
		GLuint vao = 0;
		GLuint vbo = 0;
		int numFaces = 0;
	};

	std::unordered_map<vec3i64, RenderInfo, size_t(*)(vec3i64)> renderInfos;

	// chunk construction state
	size_t bufferSize = 0;
	glm::mat4 playerTranslationMatrix;
	BlockVertexData blockVertexBuffer[Chunk::WIDTH * Chunk::WIDTH * (Chunk::WIDTH + 1) * 3 * 2 * 3];

public:
	GL3ChunkRenderer(Client *client, GL3Renderer *renderer);
	~GL3ChunkRenderer();

protected:
	void beginRender() override;
	void renderChunk(vec3i64 chunkCoords) override;
	void finishRender() override;
	void finishChunk(ChunkVisuals chunkVisuals) override;
	void destroyChunkData(vec3i64 chunkCoords) override;
};

#endif // GL3_CHUNK_RENDERER_HPP_
