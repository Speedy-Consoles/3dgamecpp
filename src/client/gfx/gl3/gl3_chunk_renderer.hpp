#ifndef GL3_CHUNK_RENDERER_HPP_
#define GL3_CHUNK_RENDERER_HPP_

#include "client/gfx/chunk_renderer.hpp"

#include "gl3_shaders.hpp"

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

	// vao, vbo locations
	GLuint *vaos;
	GLuint *vbos;

	// texture location
	GLuint blockTextures;

	// chunk construction state
	size_t bufferSize;
	glm::mat4 playerTranslationMatrix;
	BlockVertexData blockVertexBuffer[Chunk::WIDTH * Chunk::WIDTH * (Chunk::WIDTH + 1) * 3 * 2 * 3];

public:
	GL3ChunkRenderer(Client *client, GL3Renderer *renderer);
	~GL3ChunkRenderer();

private:
	void loadTextures();

protected:
	void initRenderDistanceDependent(int renderDistance) override;
	void destroyRenderDistanceDependent() override;

	void beginRender() override;
	void renderChunk(size_t index) override;
	void finishRender() override;
	void beginChunkConstruction() override;
	void emitFace(vec3i64 bc, vec3i64 icc, uint blockType, uint faceDir, int shadowLevels[4]) override;
	void finishChunkConstruction(size_t index) override;
};

#endif // GL3_CHUNK_RENDERER_HPP_
