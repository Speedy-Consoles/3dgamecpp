#include "gl3_chunk_renderer.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <SDL2/SDL_image.h>
#include <memory>

#include "gl3_renderer.hpp"
#include "engine/logging.hpp"
#include "game/world.hpp"
#include "game/player.hpp"
#include "util.hpp"
#include "shared/chunk_manager.hpp"

using namespace std;

GL3ChunkRenderer::GL3ChunkRenderer(Client *client, GL3Renderer *renderer, ShaderManager *shaderManager) :
		client(client),
		renderer(renderer),
		shaderManager(shaderManager),
		chunkMap(0, vec3i64HashFunc) {
	initRenderDistanceDependent();
	loadTextures();
}

GL3ChunkRenderer::~GL3ChunkRenderer() {
	LOG(DEBUG, "Destroying chunk renderer");
	destroyRenderDistanceDependent();
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
		LOG(ERROR, "Not enough levels available for texture");
		return;
	}

	SDL_Surface *img = IMG_Load("img/textures_1.png");
	if (!img) {
		LOG(ERROR, "Textures could not be loaded");
		return;
	}
	int tileW = img->w / xTiles;
	int tileH = img->h / yTiles;
	SDL_Surface *tmp = SDL_CreateRGBSurface(
			0, tileW, tileH, 32,
			0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
	if (!tmp) {
		LOG(ERROR, "Temporary SDL_Surface could not be created");
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
				LOG(ERROR, "Blit unsuccessful: " << SDL_GetError());
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

void GL3ChunkRenderer::setConf(const GraphicsConf &conf, const GraphicsConf &old) {
	if (conf.render_distance != old.render_distance) {
		destroyRenderDistanceDependent();
		initRenderDistanceDependent();
		checkChunkIndex = 0;
	}
}

void GL3ChunkRenderer::initRenderDistanceDependent() {
	visibleDiameter = client->getConf().render_distance * 2 + 1;
	int n = visibleDiameter * visibleDiameter * visibleDiameter;
	chunkGrid = new GridInfo[n];
	vaos = new GLuint[n];
	vbos = new GLuint[n];
	GL(GenVertexArrays(n, vaos));
	GL(GenBuffers(n, vbos));
	vsFringeCapacity = visibleDiameter * visibleDiameter * 6;
	vsFringe = new vec3i64[vsFringeCapacity];
	vsIndices = new int[vsFringeCapacity];
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
		chunkGrid[i].status = NO_CHUNK;
		chunkGrid[i].numFaces = 0;
		chunkGrid[i].passThroughs = 0;
		chunkGrid[i].vsVisited = false;
	}
	GL(BindVertexArray(0));
	faces = 0;
}

void GL3ChunkRenderer::destroyRenderDistanceDependent() {
	int n = visibleDiameter * visibleDiameter * visibleDiameter;
	delete chunkGrid;
	GL(DeleteBuffers(n, vbos));
	GL(DeleteVertexArrays(n, vaos));
	delete vaos;
	delete vbos;
	delete vsFringe;
	delete vsIndices;
}

void GL3ChunkRenderer::render() {
	Player &player = client->getLocalPlayer();
	if (!player.isValid())
		return;

	glm::mat4 viewMatrix = glm::rotate(glm::mat4(1.0f), (float) (-player.getPitch() / 360.0 * TAU), glm::vec3(1.0f, 0.0f, 0.0f));
	viewMatrix = glm::rotate(viewMatrix, (float) (-player.getYaw() / 360.0 * TAU), glm::vec3(0.0f, 1.0f, 0.0f));
	viewMatrix = glm::rotate(viewMatrix, (float) (-TAU / 4.0), glm::vec3(1.0f, 0.0f, 0.0f));
	viewMatrix = glm::rotate(viewMatrix, (float) (TAU / 4.0), glm::vec3(0.0f, 0.0f, 1.0f));
	auto *shader = &shaderManager->getBlockShader();

	vec3i64 playerPos = player.getPos();
	int64 m = RESOLUTION * Chunk::WIDTH;
	glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(
		(float) -((playerPos[0] % m + m) % m) / RESOLUTION,
		(float) -((playerPos[1] % m + m) % m) / RESOLUTION,
		(float) -((playerPos[2] % m + m) % m) / RESOLUTION)
	);

	shader->setViewMatrix(viewMatrix);
	shader->setLightEnabled(true);

	GL(ActiveTexture(GL_TEXTURE0));
	GL(BindTexture(GL_TEXTURE_2D_ARRAY, blockTextures));

	// put chunks into render queue
	vec3i64 pc = player.getChunkPos();
	if (pc != oldPlayerChunk) {
		double pcDist = (pc - oldPlayerChunk).norm();
		double newRadius = LOADING_ORDER[checkChunkIndex].norm() - pcDist;
		if (newRadius < 0)
			checkChunkIndex = 0;
		else
			checkChunkIndex = LOADING_ORDER_DISTANCE_INDICES[(int) newRadius];
		oldPlayerChunk = pc;
	}
	while(LOADING_ORDER[checkChunkIndex].norm() <= client->getConf().render_distance
			&& chunkMap.size() <= MAX_CHUNK_MAP_SIZE - 27) {
		vec3i64 cc = pc + LOADING_ORDER[checkChunkIndex].cast<int64>();
		int index = gridCycleIndex(cc, visibleDiameter);
		if (chunkGrid[index].status != OK || chunkGrid[index].content != cc) {
			auto it = chunkMap.find(cc);
			if (it == chunkMap.end() || !it->second.inRenderQueue) {
				if (it == chunkMap.end()) {
					chunkMap.insert({cc, ChunkInfo(true)});
					toRequest.push_back(cc);
				} else {
					it->second.inRenderQueue = true;
					it->second.needCounter++;
				}
				renderQueue.push_back(cc);
				for (size_t i = 0; i < 27; ++i) {
					if (i == BIG_CUBE_CYCLE_BASE_INDEX)
						continue;
					vec3i64 ncc = cc + BIG_CUBE_CYCLE[i].cast<int64>();
					auto it2 = chunkMap.find(ncc);
					if (it2 == chunkMap.end()) {
						chunkMap.insert({ncc, ChunkInfo(false)});
						toRequest.push_back(ncc);
					} else {
						it2->second.needCounter++;
					}
				}
			}
		}
		checkChunkIndex++;
	}

	// request required chunks
	while (!toRequest.empty()) {
		vec3i64 cc = toRequest.front();
		if (!client->getChunkManager()->request(cc, GRAPHICS_LISTENER_ID))
			break;
		toRequest.pop_front();
	}

	// save chunks received from chunk manager
	shared_ptr<const Chunk> sp;
	while ((sp = client->getChunkManager()->getNextChunk(GRAPHICS_LISTENER_ID)).get()) {
		vec3i64 cc = sp.get()->getCC();
		auto it = chunkMap.find(cc);
		it->second.chunkPointer = sp;
		holdChunks++;
	}

	// build chunks in render queue
	newFaces = 0;
	newChunks = 0;
	while (!renderQueue.empty() && newChunks < MAX_NEW_CHUNKS && newFaces < MAX_NEW_Faces) {
		vec3i64 cc = renderQueue.front();
		Chunk const *chunks[27];
		ChunkMap::iterator iterators[27];
		bool cantRender = false;
		for (int i = 0; i < 27; ++i) {
			vec3i64 ncc = cc + BIG_CUBE_CYCLE[i].cast<int64>();
			auto it = chunkMap.find(ncc);
			if (!it->second.chunkPointer.get()) {
				cantRender = true;
				break;
			}
			iterators[i] = it;
			chunks[i] = it->second.chunkPointer.get();
		}
		if (cantRender)
			break;
		buildChunk(chunks);
		for (int i = 0; i < 27; ++i) {
			iterators[i]->second.needCounter--;
			if (iterators[i]->second.needCounter == 0) {
				chunkMap.erase(iterators[i]);
				holdChunks--;
			}
		}
		renderQueue.pop_front();
	}

	// render chunks
	vec3d lookDir = getVectorFromAngles(player.getYaw(), player.getPitch());
	vsFringe[0] = pc;
	vsIndices[0] = gridCycleIndex(pc, visibleDiameter);
	chunkGrid[vsIndices[0]].vsExits = 0x3F;
	chunkGrid[vsIndices[0]].vsVisited = true;
	int fringeSize = 1;
	size_t fringeStart = 0;
	size_t fringeEnd = 1;

	visibleChunks = 0;
	visibleFaces = 0;
	while (fringeSize > 0) {
		visibleChunks++;
		vec3i64 cc = vsFringe[fringeStart];
		vec3i64 cd = cc - pc;
		int index = vsIndices[fringeStart];
		chunkGrid[index].vsVisited = false;
		fringeStart = (fringeStart + 1) % vsFringeCapacity;
		fringeSize--;

		if (chunkGrid[index].status != NO_CHUNK && chunkGrid[index].content == cc) {
			if (chunkGrid[index].numFaces > 0) {
				visibleFaces += chunkGrid[index].numFaces;
				GL(BindVertexArray(vaos[index]));
				glm::mat4 modelMatrix = translationMatrix * glm::translate(glm::mat4(1.0f), glm::vec3((float) (cd[0] * (int) Chunk::WIDTH), (float) (cd[1] * (int) Chunk::WIDTH), (float) (cd[2] * (int) Chunk::WIDTH)));
				shader->setModelMatrix(modelMatrix);
				shader->useProgram();
				GL(DrawArrays(GL_TRIANGLES, 0, chunkGrid[index].numFaces * 3));
			}

			for (int d = 0; d < 6; d++) {
				if ((chunkGrid[index].vsExits & (1 << d)) == 0)
					continue;

				vec3i64 ncc = cc + DIRS[d].cast<int64>();
				vec3i64 ncd = ncc - pc;

				if ((uint) ncd.maxAbs() > client->getConf().render_distance
						|| !inFrustum(ncc, player.getPos(), lookDir))
					continue;

				int nIndex = gridCycleIndex(ncc, visibleDiameter);

				if (!chunkGrid[nIndex].vsVisited) {
					chunkGrid[nIndex].vsVisited = true;
					chunkGrid[nIndex].vsExits = 0;
					vsFringe[fringeEnd] = ncc;
					vsIndices[fringeEnd] = nIndex;
					fringeEnd = (fringeEnd + 1) % vsFringeCapacity;
					fringeSize++;
				}

				if (chunkGrid[nIndex].status != NO_CHUNK && chunkGrid[nIndex].content == ncc) {
					int shift = 0;
					int invD = (d + 3) % 6;
					for (int d1 = 0; d1 < invD; d1++) {
						if (ncd[d1 % 3] * (d1 * (-2) + 5) >= 0)
							chunkGrid[nIndex].vsExits |= ((chunkGrid[nIndex].passThroughs & (1 << (shift + invD - d1 - 1))) > 0) << d1;
						shift += 5 - d1;
					}
					for (int d2 = invD + 1; d2 < 6; d2++) {
						if (ncd[d2 % 3] * (d2 * (-2) + 5) >= 0)
							chunkGrid[nIndex].vsExits |= ((chunkGrid[nIndex].passThroughs & (1 << (shift + d2 - invD - 1))) > 0) << d2;
					}
				}
			}
		}
	}
	GL(BindVertexArray(0));
}

void GL3ChunkRenderer::buildChunk(Chunk const *chunks[27]) {
	const Chunk &chunk = *(chunks[BIG_CUBE_CYCLE_BASE_INDEX]);
	vec3i64 cc = chunk.getCC();

	uint index = gridCycleIndex(cc, visibleDiameter);

	if (chunkGrid[index].status != NO_CHUNK)
		faces -= chunkGrid[index].numFaces;

	chunkGrid[index].numFaces = 0;
	chunkGrid[index].content = cc;
	chunkGrid[index].status = OK;
	chunkGrid[index].passThroughs = chunk.getPassThroughs();

	GL(BindBuffer(GL_ARRAY_BUFFER, vbos[index]));

	if (chunk.getAirBlocks() == Chunk::WIDTH * Chunk::WIDTH * Chunk::WIDTH) {
		GL(BufferData(GL_ARRAY_BUFFER, 0, 0, GL_STATIC_DRAW));
		GL(BindBuffer(GL_ARRAY_BUFFER, 0));
		return;
	}

	newChunks++;

	vec3ui8 uDirs[3];
	for (uint8 d = 0; d < 3; d++) {
		uDirs[d] = DIRS[d].cast<uint8>();
	}

	size_t bufferSize = 0;

	const uint8 *blocks = chunk.getBlocks();
	for (uint8 d = 0; d < 3; d++) {
		vec3i64 dir = uDirs[d].cast<int64>();
		uint dimFlipIndexDiff = ((dir[2] * Chunk::WIDTH + dir[1]) * Chunk::WIDTH + dir[0]) * (Chunk::WIDTH - 1);
		uint i = 0;
		uint ni = 0;
		for (int z = (d == 2) ? -1 : 0; z < (int) Chunk::WIDTH; z++) {
			for (int y = (d == 1) ? -1 : 0; y < (int) Chunk::WIDTH; y++) {
				for (int x = (d == 0) ? -1 : 0; x < (int) Chunk::WIDTH; x++) {
					uint8 thatType;
					uint8 thisType;
					bool thisOutside = x == -1 || y == -1 || z == -1;
					bool thatOutside = !thisOutside
							&& ((x == Chunk::WIDTH - 1 && d==0)
							|| (y == Chunk::WIDTH - 1 && d==1)
							|| (z == Chunk::WIDTH - 1 && d==2));
					if (thatOutside) {
						const Chunk &otherChunk = *chunks[DIR_TO_BIG_CUBE_CYCLE_INDEX[d]];
						thatType = otherChunk.getBlocks()[i - dimFlipIndexDiff];
						if (thatType != 0) {
							if (!thisOutside)
								i++;
							continue;
						}
					} else
						thatType = blocks[ni++];

					if (thisOutside) {
						const Chunk &otherChunk = *chunks[DIR_TO_BIG_CUBE_CYCLE_INDEX[d + 3]];
						thisType = otherChunk.getBlocks()[ni - 1 + dimFlipIndexDiff];
						if (thisType != 0)
							continue;
					} else {
						thisType = blocks[i++];
					}

					if((thisType == 0) != (thatType == 0)) {
						vec3i64 faceBlock;
						uint8 faceType;
						uint8 faceDir;
						if (thisType == 0) {
							faceBlock = vec3i64(x, y, z) + dir;
							faceDir = (uint8) (d + 3);
							faceType = thatType;
						} else {
							faceBlock = vec3i64(x, y, z);
							faceDir = d;
							faceType = thisType;
						}

						uint8 corners = 0;
						for (int j = 0; j < 8; ++j) {
							vec3i64 v = DIR_QUAD_EIGHT_NEIGHBOR_CYCLES[faceDir][j].cast<int64>();
							vec3i64 dIcc = faceBlock + v;
							vec3i8 ncc(0, 0, 0);
							for (int i = 0; i < 3; i++) {
								if (dIcc[i] < 0) {
									ncc[i] = -1;
									dIcc[i] += Chunk::WIDTH;
								} else if (dIcc[i] >= (int) Chunk::WIDTH) {
									ncc[i] = 1;
									dIcc[i] -= Chunk::WIDTH;
								}
							}
							const Chunk &otherChunk = *chunks[vec2BigCubeCycleIndex(ncc)];
							uint8 cornerBlock = otherChunk.getBlock(dIcc.cast<uint8>());
							if (cornerBlock != 0) {
								corners |= 1 << j;
							}
						}
						vec3i64 bc = chunk.getCC() * chunk.WIDTH + faceBlock.cast<int64>();

						ushort posIndices[4];
						uchar shadowCombination = 0;
						for (int j = 0; j < 4; j++) {
							int shadowLevel = 0;
							bool s1 = (corners & QUAD_CORNER_MASK[j][0]) > 0;
							bool s2 = (corners & QUAD_CORNER_MASK[j][2]) > 0;
							bool m = (corners & QUAD_CORNER_MASK[j][1]) > 0;
							if (s1)
								shadowLevel++;
							if (s2)
								shadowLevel++;
							if (m || (s1 && s2))
								shadowLevel++;
							shadowCombination |= shadowLevel << 2 * j;
							vec3ui8 vertex = faceBlock.cast<uint8>() + DIR_QUAD_CORNER_CYCLES_3D[faceDir][j].cast<uint8>();
							posIndices[j] = (vertex[2] * (Chunk::WIDTH + 1) + vertex[1]) * (Chunk::WIDTH + 1) + vertex[0];
						}
						int indices[6] = {0, 1, 2, 2, 3, 0};
						for (int j = 0; j < 6; j++) {
							blockVertexBuffer[bufferSize].positionIndex = posIndices[indices[j]];
							blockVertexBuffer[bufferSize].textureIndex = faceType;
							blockVertexBuffer[bufferSize].dirIndexCornerIndex = faceDir | (indices[j] << 3);
							blockVertexBuffer[bufferSize].shadowLevels = shadowCombination;
							bufferSize++;
						}
					}
				}
			}
		}
	}

	GL(BufferData(GL_ARRAY_BUFFER, sizeof(BlockVertexData) * bufferSize, blockVertexBuffer, GL_STATIC_DRAW));

	chunkGrid[index].numFaces = bufferSize / 3;
	newFaces += bufferSize / 3;
	faces += bufferSize / 3;
	GL(BindBuffer(GL_ARRAY_BUFFER, 0));
}

bool GL3ChunkRenderer::inFrustum(vec3i64 cc, vec3i64 pos, vec3d lookDir) {
	double chunkDia = sqrt(3) * Chunk::WIDTH * RESOLUTION;
	vec3d cp = (cc * Chunk::WIDTH * RESOLUTION - pos).cast<double>();
	double chunkLookDist = lookDir * cp + chunkDia;
	if (chunkLookDist < 0)
		return false;
	vec3d orthoChunkPos = cp - lookDir * chunkLookDist;
	double orthoChunkDist = max(0.0, orthoChunkPos.norm() - chunkDia);
	return atan(orthoChunkDist / chunkLookDist) <= renderer->getMaxFOV() / 2;
}

ChunkRendererDebugInfo GL3ChunkRenderer::getDebugInfo() {
	ChunkRendererDebugInfo info;
	info.newFaces = newFaces;
	info.newChunks = newChunks;
	info.totalFaces = faces;
	info.visibleChunks = visibleChunks;
	info.visibleFaces = visibleFaces;
	info.chunkMapSize = chunkMap.size();
	info.holdChunks = holdChunks;
	info.renderQueueSize = renderQueue.size();
	info.requestQueueSize = toRequest.size();

	return info;
}
