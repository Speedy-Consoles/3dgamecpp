#include "gl3_chunk_renderer.hpp"

#define GLM_FORCE_RADIANS

#include <memory>

#include <SDL2/SDL_image.h>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "shared/engine/logging.hpp"
#include "shared/engine/math.hpp"
#include "shared/game/world.hpp"
#include "shared/game/player.hpp"
#include "shared/block_utils.hpp"
#include "shared/chunk_manager.hpp"

#include "gl3_renderer.hpp"

using namespace std;

static logging::Logger logger("render");

GL3ChunkRenderer::GL3ChunkRenderer(Client *client, GL3Renderer *renderer) :
	ChunkRenderer(client, renderer),
	renderInfos(0, vec3i64HashFunc)
{
	// nothing
}

GL3ChunkRenderer::~GL3ChunkRenderer() {
	// nothing
}

void GL3ChunkRenderer::beginRender() {
	Player &player = client->getLocalPlayer();
	if (!player.isValid())
		return;

	glm::mat4 viewMatrix = glm::rotate(glm::mat4(1.0f), (float) (-player.getPitch() / 360.0 * TAU), glm::vec3(1.0f, 0.0f, 0.0f));
	viewMatrix = glm::rotate(viewMatrix, (float) (-player.getYaw() / 360.0 * TAU), glm::vec3(0.0f, 1.0f, 0.0f));
	viewMatrix = glm::rotate(viewMatrix, (float) (-TAU / 4.0), glm::vec3(1.0f, 0.0f, 0.0f));
	viewMatrix = glm::rotate(viewMatrix, (float) (TAU / 4.0), glm::vec3(0.0f, 0.0f, 1.0f));

	vec3i64 playerPos = player.getPos();
	int64 m = RESOLUTION * Chunk::WIDTH;
	playerTranslationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(
		(float) -cycle(playerPos[0], m) / RESOLUTION,
		(float) -cycle(playerPos[1], m) / RESOLUTION,
		(float) -cycle(playerPos[2], m) / RESOLUTION)
	);

	auto *shader = &((GL3Renderer *) renderer)->getShaderManager()->getBlockShader();
	shader->setViewMatrix(viewMatrix);
	shader->setLightEnabled(true);
}

void GL3ChunkRenderer::renderChunk(vec3i64 chunkCoords) {
	auto it = renderInfos.find(chunkCoords);
	if (it == renderInfos.end() || it->second.vao == 0)
		return;

	Player &player = client->getLocalPlayer();
	vec3i64 cd = chunkCoords - player.getChunkPos();

	glm::mat4 chunkTranslationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(
		(float) (cd[0] * Chunk::WIDTH),
		(float) (cd[1] * Chunk::WIDTH),
		(float) (cd[2] * Chunk::WIDTH)
	));
	glm::mat4 modelMatrix = playerTranslationMatrix * chunkTranslationMatrix;

	auto *shader = &((GL3Renderer *) renderer)->getShaderManager()->getBlockShader();
	shader->setModelMatrix(modelMatrix);
	shader->useProgram();

	GL3TextureManager *texManager = static_cast<GL3Renderer *>(renderer)->getTextureManager();
	GL3TextureManager::Entry entry = texManager->get(-1, 0);
	GL(ActiveTexture(GL_TEXTURE0));
	GL(BindTexture(GL_TEXTURE_2D_ARRAY, entry.tex));

	GL(BindVertexArray(it->second.vao));
	GL(DrawArrays(GL_TRIANGLES, 0, it->second.numFaces * 3));
}

void GL3ChunkRenderer::finishRender() {
	GL(BindVertexArray(0));
}

void GL3ChunkRenderer::finishChunk(ChunkVisuals chunkVisuals) {
	bufferSize = 0;

	for (Quad quad : chunkVisuals.quads) {
		ushort posIndices[4];
		uint8 compactShadowLevels = 0;
		for (int i = 0; i < 4; i++) {
			vec3i64 v = quad.icc.cast<int64>() + DIR_QUAD_CORNER_CYCLES_3D[quad.faceDir][i].cast<int64>();
			posIndices[i] = (ushort) ((v[2] * (Chunk::WIDTH + 1) + v[1]) * (Chunk::WIDTH + 1) + v[0]);
			compactShadowLevels |= quad.shadowLevels[i] << 2 * i;
		}
		static const int INDICES[6] = {0, 1, 2, 2, 3, 0};

		GL3TextureManager *texManager = static_cast<GL3Renderer *>(renderer)->getTextureManager();
		GL3TextureManager::Entry entry = texManager->get(quad.faceType, quad.bc, quad.faceDir);

		for (int i = 0; i < 6; i++) {
			blockVertexBuffer[bufferSize].positionIndex = posIndices[INDICES[i]];
			blockVertexBuffer[bufferSize].textureIndex = entry.layer;
			blockVertexBuffer[bufferSize].dirIndexCornerIndex = quad.faceDir | (INDICES[i] << 3);
			blockVertexBuffer[bufferSize].shadowLevels = compactShadowLevels;
			bufferSize++;
		}
	}

	auto it = renderInfos.find(chunkVisuals.cc);
	if (bufferSize > 0) {
		if (it == renderInfos.end()) {
			auto pair = renderInfos.insert({chunkVisuals.cc, RenderInfo()});
			it = pair.first;
		}
		if (it->second.vao == 0) {
			GL(GenVertexArrays(1, &it->second.vao));
			GL(GenBuffers(1, &it->second.vbo));
			GL(BindVertexArray(it->second.vao));
			GL(BindBuffer(GL_ARRAY_BUFFER, it->second.vbo));
			GL(VertexAttribIPointer(0, 1, GL_UNSIGNED_SHORT, 5, (void *) 0));
			GL(VertexAttribIPointer(1, 1, GL_UNSIGNED_BYTE, 5, (void *) 2));
			GL(VertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE, 5, (void *) 3));
			GL(VertexAttribIPointer(3, 1, GL_UNSIGNED_BYTE, 5, (void *) 4));
			GL(EnableVertexAttribArray(0));
			GL(EnableVertexAttribArray(1));
			GL(EnableVertexAttribArray(2));
			GL(EnableVertexAttribArray(3));
		} else {
			GL(BindBuffer(GL_ARRAY_BUFFER, it->second.vbo));
		}
		auto size = sizeof(BlockVertexData) * bufferSize;
		GL(BufferData(GL_ARRAY_BUFFER, size, blockVertexBuffer, GL_STATIC_DRAW));
		it->second.numFaces = bufferSize / 3;
	} else if (it != renderInfos.end()) {
		if (it->second.vao != 0) {
			GL(DeleteVertexArrays(1, &it->second.vao));
			GL(DeleteBuffers(1, &it->second.vbo));
		}
		renderInfos.erase(it);
	}
}

void GL3ChunkRenderer::destroyChunkData(vec3i64 chunkCoords) {
	auto it = renderInfos.find(chunkCoords);
	if (it != renderInfos.end()) {
		if (it->second.vao != 0) {
			GL(DeleteVertexArrays(1, &it->second.vao));
			GL(DeleteBuffers(1, &it->second.vbo));
		}
		renderInfos.erase(it);
	}
}
