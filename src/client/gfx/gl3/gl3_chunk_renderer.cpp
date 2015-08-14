#include "gl3_chunk_renderer.hpp"

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
	ChunkRenderer(client, renderer)
{
	initRenderDistanceDependent(client->getConf().render_distance);
}

GL3ChunkRenderer::~GL3ChunkRenderer() {
	destroyRenderDistanceDependent();
}

void GL3ChunkRenderer::initRenderDistanceDependent(int renderDistance) {
	ChunkRenderer::initRenderDistanceDependent(renderDistance);

	int visibleDiameter = renderDistance * 2 + 1;
	int n = visibleDiameter * visibleDiameter * visibleDiameter;

	vaos = new GLuint[n];
	vbos = new GLuint[n];
	for (int i = 0; i < n; i++) {
		vaos[i] = 0;
		vbos[i] = 0;
	}
}

void GL3ChunkRenderer::destroyRenderDistanceDependent() {
	ChunkRenderer::destroyRenderDistanceDependent();

	int visibleDiameter = renderDistance * 2 + 1;
	int n = visibleDiameter * visibleDiameter * visibleDiameter;

	for (int i = 0; i < n; i++) {
		if (vaos[i] != 0) {
			GL(DeleteVertexArrays(1, &vaos[i]));
			GL(DeleteBuffers(1, &vbos[i]));
		}
	}
	delete[] vaos;
	delete[] vbos;
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

void GL3ChunkRenderer::renderChunk(size_t index) {
	Player &player = client->getLocalPlayer();
	vec3i64 cd = chunkGrid[index].content - player.getChunkPos();
	
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

	GL(BindVertexArray(vaos[index]));
	GL(DrawArrays(GL_TRIANGLES, 0, chunkGrid[index].numFaces * 3));
}

void GL3ChunkRenderer::finishRender() {
	GL(BindVertexArray(0));
}

void GL3ChunkRenderer::beginChunkConstruction() {
	bufferSize = 0;
}

void GL3ChunkRenderer::emitFace(vec3i64 bc, vec3i64 icc, uint blockType, uint faceDir, int shadowLevels[4]) {
	
	ushort posIndices[4];
	uint8 compactShadowLevels = 0;
	for (int i = 0; i < 4; i++) {
		vec3i64 v = icc + DIR_QUAD_CORNER_CYCLES_3D[faceDir][i].cast<int64>();
		posIndices[i] = (ushort) ((v[2] * (Chunk::WIDTH + 1) + v[1]) * (Chunk::WIDTH + 1) + v[0]);
		compactShadowLevels |= shadowLevels[i] << 2 * i;
	}
	static const int INDICES[6] = {0, 1, 2, 2, 3, 0};

	GL3TextureManager *texManager = static_cast<GL3Renderer *>(renderer)->getTextureManager();
	GL3TextureManager::Entry entry = texManager->get(blockType, bc, faceDir);

	for (int i = 0; i < 6; i++) {
		blockVertexBuffer[bufferSize].positionIndex = posIndices[INDICES[i]];
		blockVertexBuffer[bufferSize].textureIndex = entry.layer;
		blockVertexBuffer[bufferSize].dirIndexCornerIndex = faceDir | (INDICES[i] << 3);
		blockVertexBuffer[bufferSize].shadowLevels = compactShadowLevels;
		bufferSize++;
	}
	
}

void GL3ChunkRenderer::finishChunkConstruction(size_t index) {
	if (bufferSize > 0) {
		if (vaos[index] == 0) {
			GL(GenVertexArrays(1, &vaos[index]));
			GL(GenBuffers(1, &vbos[index]));
			GL(BindVertexArray(vaos[index]));
			GL(BindBuffer(GL_ARRAY_BUFFER, vbos[index]));
			GL(VertexAttribIPointer(0, 1, GL_UNSIGNED_SHORT, 5, (void *) 0));
			GL(VertexAttribIPointer(1, 1, GL_UNSIGNED_BYTE, 5, (void *) 2));
			GL(VertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE, 5, (void *) 3));
			GL(VertexAttribIPointer(3, 1, GL_UNSIGNED_BYTE, 5, (void *) 4));
			GL(EnableVertexAttribArray(0));
			GL(EnableVertexAttribArray(1));
			GL(EnableVertexAttribArray(2));
			GL(EnableVertexAttribArray(3));
		} else {
			GL(BindBuffer(GL_ARRAY_BUFFER, vbos[index]));
		}
		auto size = sizeof(BlockVertexData) * bufferSize;
		GL(BufferData(GL_ARRAY_BUFFER, size, blockVertexBuffer, GL_STATIC_DRAW));
	} else {
		if (vaos[index] != 0) {
			GL(DeleteVertexArrays(1, &vaos[index]));
			GL(DeleteBuffers(1, &vbos[index]));
		}
	}
}
