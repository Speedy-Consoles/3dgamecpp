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

GL3ChunkRenderer::GL3ChunkRenderer(Client *client, GL3Renderer *renderer, ShaderManager *shaderManager) :
		ChunkRenderer(client, renderer),
		shaderManager(shaderManager) {
	loadTextures();
}

GL3ChunkRenderer::~GL3ChunkRenderer() {
	LOG_DEBUG(logger) << "Destroying chunk renderer";
	GL(DeleteTextures(1, &blockTextures));
}

void GL3ChunkRenderer::loadTextures() {
	int xTiles = 16;
	int yTiles = 16;
	GLsizei layerCount = 256;
	GLsizei mipLevelCount = 7;

	GLint maxLayerCount;
	GL(GetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &maxLayerCount));
	if (maxLayerCount < layerCount) {
		LOG_ERROR(logger) << "Not enough levels available for texture";
		return;
	}

	SDL_Surface *img = IMG_Load("img/textures_1.png");
	if (!img) {
		LOG_ERROR(logger) << "Textures could not be loaded";
		return;
	}
	int tileW = img->w / xTiles;
	int tileH = img->h / yTiles;
	SDL_Surface *tmp = SDL_CreateRGBSurface(
			0, tileW, tileH, 32,
			0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
	if (!tmp) {
		LOG_ERROR(logger) << "Temporary SDL_Surface could not be created";
		return;
	}

	GL(GenTextures(1, &blockTextures));
	GL(BindTexture(GL_TEXTURE_2D_ARRAY, blockTextures));
	GL(TexStorage3D(GL_TEXTURE_2D_ARRAY, mipLevelCount, GL_RGBA8, tileW, tileH, layerCount));

	uint blocks[] = {
		 1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16,
		17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
		33,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	};

	auto blocks_ptr = blocks;
	for (int j = 0; j < yTiles; ++j) {
		for (int i = 0; i < xTiles; ++i) {
			uint block = *blocks_ptr++;
			if (block == 0)
				continue;
			SDL_Rect rect{i * tileW, j * tileH, tileW, tileH};
			int ret_code = SDL_BlitSurface(img, &rect, tmp, nullptr);
			if (ret_code)
				LOG_ERROR(logger) << "Blit unsuccessful: " << SDL_GetError();
			GL(TexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, block, tileW, tileH, 1, GL_RGBA, GL_UNSIGNED_BYTE, tmp->pixels));
		}
	}

	GL(TexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BASE_LEVEL, 0));
	GL(TexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, mipLevelCount - 1));
	GL(TexParameteri(GL_TEXTURE_2D_ARRAY, GL_GENERATE_MIPMAP, GL_TRUE));

	GL(TexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GL(TexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
	GL(TexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT));
	GL(TexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT));
	GL(GenerateMipmap(GL_TEXTURE_2D_ARRAY));

	SDL_FreeSurface(tmp);
	SDL_FreeSurface(img);
}

void GL3ChunkRenderer::initRenderDistanceDependent(int renderDistance) {
	ChunkRenderer::initRenderDistanceDependent(renderDistance);

	int visibleDiameter = renderDistance * 2 + 1;
	int n = visibleDiameter * visibleDiameter * visibleDiameter;

	vaos = new GLuint[n];
	vbos = new GLuint[n];
	GL(GenVertexArrays(n, vaos));
	GL(GenBuffers(n, vbos));
	for (int i = 0; i < n; i++) {
		GL(BindVertexArray(vaos[i]));
		GL(BindBuffer(GL_ARRAY_BUFFER, vbos[i]));
		GL(BufferData(GL_ARRAY_BUFFER, 0, 0, GL_STATIC_DRAW));
		GL(VertexAttribIPointer(0, 1, GL_UNSIGNED_SHORT, 5, (void *) 0));
		GL(VertexAttribIPointer(1, 1, GL_UNSIGNED_BYTE, 5, (void *) 2));
		GL(VertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE, 5, (void *) 3));
		GL(VertexAttribIPointer(3, 1, GL_UNSIGNED_BYTE, 5, (void *) 4));
		GL(EnableVertexAttribArray(0));
		GL(EnableVertexAttribArray(1));
		GL(EnableVertexAttribArray(2));
		GL(EnableVertexAttribArray(3));
	}
	GL(BindVertexArray(0));
}

void GL3ChunkRenderer::destroyRenderDistanceDependent() {
	ChunkRenderer::destroyRenderDistanceDependent();

	int visibleDiameter = renderDistance * 2 + 1;
	int n = visibleDiameter * visibleDiameter * visibleDiameter;

	GL(DeleteBuffers(n, vbos));
	GL(DeleteVertexArrays(n, vaos));
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

	auto *shader = &shaderManager->getBlockShader();
	shader->setViewMatrix(viewMatrix);
	shader->setLightEnabled(true);

	GL(ActiveTexture(GL_TEXTURE0));
	GL(BindTexture(GL_TEXTURE_2D_ARRAY, blockTextures));
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

	auto *shader = &shaderManager->getBlockShader();
	shader->setModelMatrix(modelMatrix);
	shader->useProgram();

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
	for (int i = 0; i < 6; i++) {
		blockVertexBuffer[bufferSize].positionIndex = posIndices[INDICES[i]];
		blockVertexBuffer[bufferSize].textureIndex = blockType;
		blockVertexBuffer[bufferSize].dirIndexCornerIndex = faceDir | (INDICES[i] << 3);
		blockVertexBuffer[bufferSize].shadowLevels = compactShadowLevels;
		bufferSize++;
	}
}

void GL3ChunkRenderer::finishChunkConstruction(size_t index) {
	GL(BindBuffer(GL_ARRAY_BUFFER, vbos[index]));
	auto size = sizeof(BlockVertexData) * bufferSize;
	GL(BufferData(GL_ARRAY_BUFFER, size, blockVertexBuffer, GL_STATIC_DRAW));
	GL(BindBuffer(GL_ARRAY_BUFFER, 0));
}
