#include "gl3_renderer.hpp"

#include <SDL2/SDL.h>
#include <glm/gtc/matrix_transform.hpp>

#include "engine/logging.hpp"
#include "engine/math.hpp"

#include "graphics.hpp"
#include "menu.hpp"
#include "stopwatch.hpp"

#include "game/world.hpp"

using namespace gui;

GL3Renderer::GL3Renderer(
		Graphics *graphics,
		SDL_Window *window,
		World *world,
		const Menu *menu,
		const ClientState *state,
		const uint8 *localClientID,
		const GraphicsConf &conf,
		Stopwatch *stopwatch) :
		graphics(graphics),
		conf(conf),
		window(window),
		world(world),
		menu(menu),
		state(*state),
		localClientID(*localClientID),
		stopwatch(stopwatch) {
	initRenderDistanceDependent();
	makeMaxFOV();

	// light
	glm::vec3 ambientColor = glm::vec3(0.3f, 0.3f, 0.27f);
	glm::vec3 diffuseDirection = glm::vec3(1.0f, 0.5f, 3.0f);
	glm::vec3 diffuseColor = glm::vec3(0.2f, 0.2f, 0.17f);

	shaders.setAmbientLightColor(ambientColor);
	shaders.setDiffuseLightDirection(diffuseDirection);
	shaders.setDiffuseLightColor(diffuseColor);
}

GL3Renderer::~GL3Renderer() {
	LOG(DEBUG, "Destroying GL3 renderer");
	destroyRenderDistanceDependent();
}

void GL3Renderer::resize(int width, int height) {
	LOG(INFO, "Resize to " << width << "x" << height);
	glViewport(0, 0, width, height);
	makePerspectiveMatrix();
	makeOrthogonalMatrix();
}

void GL3Renderer::makePerspectiveMatrix() {
	double normalRatio = DEFAULT_WINDOWED_RES[0] / (double) DEFAULT_WINDOWED_RES[1];
	double currentRatio = graphics->getWidth() / (double) graphics->getHeight();
	double angle;

	float yfov = conf.fov / normalRatio * TAU / 360.0;
	if (currentRatio > normalRatio)
		angle = atan(tan(yfov / 2) * normalRatio / currentRatio) * 2;
	else
		angle = yfov;

	float zFar = Chunk::WIDTH * sqrt(3 * (conf.render_distance + 1) * (conf.render_distance + 1));
	perspectiveMatrix = glm::perspective((float) angle,
			(float) currentRatio, ZNEAR, zFar);
}

void GL3Renderer::makeOrthogonalMatrix() {
	float normalRatio = DEFAULT_WINDOWED_RES[0] / (double) DEFAULT_WINDOWED_RES[1];
	float currentRatio = graphics->getWidth() / (double) graphics->getHeight();
	if (currentRatio > normalRatio)
		orthogonalMatrix = glm::ortho(-DEFAULT_WINDOWED_RES[0] / 2.0f, DEFAULT_WINDOWED_RES[0] / 2.0f, -DEFAULT_WINDOWED_RES[0]
				/ currentRatio / 2.0f, DEFAULT_WINDOWED_RES[0] / currentRatio / 2.0f, 1.0f, -1.0f);
	else
		orthogonalMatrix = glm::ortho(-DEFAULT_WINDOWED_RES[1] * currentRatio / 2.0f, DEFAULT_WINDOWED_RES[1]
				* currentRatio / 2.0f, -DEFAULT_WINDOWED_RES[1] / 2.0f,
				DEFAULT_WINDOWED_RES[1] / 2.0f, 1.0f, -1.0f);
}

void GL3Renderer::makeMaxFOV() {
	float ratio = (float) DEFAULT_WINDOWED_RES[0] / DEFAULT_WINDOWED_RES[1];
	float yfov = conf.fov / ratio * TAU / 360.0;
	if (ratio < 1.0)
		maxFOV = yfov;
	else
		maxFOV = atan(ratio * tan(yfov / 2)) * 2;
}

void GL3Renderer::setDebug(bool debugActive) {
	this->debugActive = debugActive;
}

bool GL3Renderer::isDebug() {
	return debugActive;
}

void GL3Renderer::setConf(const GraphicsConf &conf) {
	GraphicsConf old_conf = this->conf;
	this->conf = conf;

	if (conf.render_distance != old_conf.render_distance) {
		destroyRenderDistanceDependent();
		makePerspectiveMatrix();
		initRenderDistanceDependent();
	}

	if (conf.fov != old_conf.fov) {
		makePerspectiveMatrix();
		makeMaxFOV();
	}
}

void GL3Renderer::initRenderDistanceDependent() {
	int length = conf.render_distance * 2 + 1;
	int n = length * length * length;
	vaos = new GLuint[n];
	vbos = new GLuint[n];
	glGenVertexArrays(n, vaos);
	glGenBuffers(n, vbos);
	vaoChunks = new vec3i64[n];
	vaoStatus = new uint8[n];
	chunkFaces = new int[n];
	chunkPassThroughs = new uint16[n];
	vsExits = new uint8[n];
	vsVisited = new bool[n];
	vsFringeCapacity = length * length * 6;
	vsFringe = new vec3i64[vsFringeCapacity];
	vsIndices = new int[vsFringeCapacity];
	for (int i = 0; i < n; i++) {
		glBindVertexArray(vaos[i]);
		glBindBuffer(GL_ARRAY_BUFFER, vbos[i]);
		glBufferData(GL_ARRAY_BUFFER, 0, 0, GL_STATIC_DRAW);
		glVertexAttribIPointer(0, 1, GL_UNSIGNED_SHORT, 3, 0);
		glVertexAttribIPointer(1, 1, GL_UNSIGNED_BYTE, 3, (void *) 2);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		vaoStatus[i] = NO_CHUNK;
		chunkFaces[i] = 0;
		chunkPassThroughs[i] = 0;
		vsVisited[i] = false;
	}
	glBindVertexArray(0);
	faces = 0;
}

void GL3Renderer::destroyRenderDistanceDependent() {
	int length = conf.render_distance * 2 + 1;
	glDeleteBuffers(length * length * length, vbos);
	glDeleteVertexArrays(length * length * length, vaos);
	delete vaos;
	delete vbos;
	delete vaoChunks;
	delete vaoStatus;
	delete chunkFaces;
	delete chunkPassThroughs;
	delete vsExits;
	delete vsVisited;
	delete vsFringe;
	delete vsIndices;
}

void GL3Renderer::tick() {
	render();

	SDL_GL_SwapWindow(window);

	if (getCurrentTime() - lastStopWatchSave > millis(200)) {
		lastStopWatchSave = getCurrentTime();
		stopwatch->stop(CLOCK_ALL);
		stopwatch->save();
		stopwatch->start(CLOCK_ALL);
	}

	while (getCurrentTime() - lastFPSUpdate > millis(50)) {
		lastFPSUpdate += millis(50);
		fpsSum -= prevFPS[fpsIndex];
		fpsSum += fpsCounter;
		prevFPS[fpsIndex] = fpsCounter;
		fpsCounter = 0;
		fpsIndex = (fpsIndex + 1) % 20;
	}
	fpsCounter++;
}

void GL3Renderer::render() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	logOpenGLError();

	Player &player = world->getPlayer(localClientID);
	if (player.isValid()) {
		shaders.setProjectionMatrix(perspectiveMatrix);

		// view matrix for sky
		glm::mat4 viewMatrix = glm::rotate(glm::mat4(1.0f), (float) (-player.getPitch() / 360.0 * TAU), glm::vec3(1.0f, 0.0f, 0.0f));
		shaders.setModelMatrix(glm::mat4(1.0f));
		shaders.setViewMatrix(viewMatrix);

		// render sky
		//renderSky();

		// view matrix for scene
		viewMatrix = glm::rotate(viewMatrix, (float) (-player.getYaw() / 360.0 * TAU), glm::vec3(0.0f, 1.0f, 0.0f));
		viewMatrix = glm::rotate(viewMatrix, (float) (-TAU / 4.0), glm::vec3(1.0f, 0.0f, 0.0f));
		viewMatrix = glm::rotate(viewMatrix, (float) (TAU / 4.0), glm::vec3(0.0f, 0.0f, 1.0f));
		vec3i64 playerPos = player.getPos();
		int64 m = RESOLUTION * Chunk::WIDTH;
		viewMatrix = glm::translate(viewMatrix, glm::vec3(
			(float) -((playerPos[0] % m + m) % m) / RESOLUTION,
			(float) -((playerPos[1] % m + m) % m) / RESOLUTION,
			(float) -((playerPos[2] % m + m) % m) / RESOLUTION)
		);
		shaders.setViewMatrix(viewMatrix);

		// render scene
		renderChunks();
		renderTarget();
		renderPlayers();
	}

	// render overlay
	shaders.setModelMatrix(glm::mat4(1.0f));
	shaders.setViewMatrix(glm::mat4(1.0f));
	shaders.setProjectionMatrix(orthogonalMatrix);

	if (state == PLAYING && player.isValid()) {
		//renderHud(player);
		//if (debugActive)
		//	renderDebugInfo(player);
	} else if (state == IN_MENU){
		renderMenu();
	}
}

void GL3Renderer::renderChunks() {
	int length = conf.render_distance * 2 + 1;

	vec3i64 ccc;
	while (world->popChangedChunk(&ccc)) {
		int index = ((((ccc[2] % length) + length) % length) * length
				+ (((ccc[1] % length) + length) % length)) * length
				+ (((ccc[0] % length) + length) % length);
		if (vaoStatus[index] != NO_CHUNK)
			vaoStatus[index] = OUTDATED;
	}

	Player &localPlayer = world->getPlayer(localClientID);
	vec3i64 pc = localPlayer.getChunkPos();
	vec3d lookDir = getVectorFromAngles(localPlayer.getYaw(), localPlayer.getPitch());

	newFaces = 0;
	newChunks = 0;

	vsFringe[0] = pc;
	vsIndices[0] = ((((pc[2] % length) + length) % length) * length
			+ (((pc[1] % length) + length) % length)) * length
			+ (((pc[0] % length) + length) % length);
	vsExits[vsIndices[0]] = 0x3F;
	vsVisited[vsIndices[0]] = true;
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
		vsVisited[index] = false;
		fringeStart = (fringeStart + 1) % vsFringeCapacity;
		fringeSize--;

		if ((vaoStatus[index] != OK || vaoChunks[index] != cc) && (newChunks < MAX_NEW_CHUNKS && newFaces < MAX_NEW_QUADS)) {
			Chunk *c = world->getChunk(cc);
			if (c)
				renderChunk(*c);
		}

		if (vaoStatus[index] != NO_CHUNK && vaoChunks[index] == cc) {
			if (chunkFaces[index] > 0) {
				visibleFaces += chunkFaces[index];
				glBindVertexArray(vaos[index]);
				glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3((float) (cd[0] * (int) Chunk::WIDTH), (float) (cd[1] * (int) Chunk::WIDTH), (float) (cd[2] * (int) Chunk::WIDTH)));
				shaders.setModelMatrix(modelMatrix);
				shaders.prepareProgram(BLOCK_PROGRAM);
				glDrawArrays(GL_TRIANGLES, 0, chunkFaces[index] * 3);
				logOpenGLError();
			}

			for (int d = 0; d < 6; d++) {
				if ((vsExits[index] & (1 << d)) == 0)
					continue;

				vec3i64 ncc = cc + DIRS[d].cast<int64>();
				vec3i64 ncd = ncc - pc;

				if ((uint) abs(ncd[0]) > conf.render_distance
						|| (uint) abs(ncd[1]) > conf.render_distance
						|| (uint) abs(ncd[2]) > conf.render_distance
						|| !inFrustum(ncc, localPlayer.getPos(), lookDir))
					continue;

				int nIndex = ((((ncc[2] % length) + length) % length) * length
						+ (((ncc[1] % length) + length) % length)) * length
						+ (((ncc[0] % length) + length) % length);

				if (!vsVisited[nIndex]) {
					vsVisited[nIndex] = true;
					vsExits[nIndex] = 0;
					vsFringe[fringeEnd] = ncc;
					vsIndices[fringeEnd] = nIndex;
					fringeEnd = (fringeEnd + 1) % vsFringeCapacity;
					fringeSize++;
				}

				if (vaoStatus[nIndex] != NO_CHUNK && vaoChunks[nIndex] == ncc) {
					int shift = 0;
					int invD = (d + 3) % 6;
					for (int d1 = 0; d1 < invD; d1++) {
						if (ncd[d1 % 3] * (d1 * (-2) + 5) >= 0)
							vsExits[nIndex] |= ((chunkPassThroughs[nIndex] & (1 << (shift + invD - d1 - 1))) > 0) << d1;
						shift += 5 - d1;
					}
					for (int d2 = invD + 1; d2 < 6; d2++) {
						if (ncd[d2 % 3] * (d2 * (-2) + 5) >= 0)
							vsExits[nIndex] |= ((chunkPassThroughs[nIndex] & (1 << (shift + d2 - invD - 1))) > 0) << d2;
					}
				}
			}
		}
	}
	glBindVertexArray(0);
	logOpenGLError();
}

bool GL3Renderer::inFrustum(vec3i64 cc, vec3i64 pos, vec3d lookDir) {
	double chunkDia = sqrt(3) * Chunk::WIDTH * RESOLUTION;
	vec3d cp = (cc * Chunk::WIDTH * RESOLUTION - pos).cast<double>();
	double chunkLookDist = lookDir * cp + chunkDia;
	if (chunkLookDist < 0)
		return false;
	vec3d orthoChunkPos = cp - lookDir * chunkLookDist;
	double orthoChunkDist = std::max(0.0, orthoChunkPos.norm() - chunkDia);
	return atan(orthoChunkDist / chunkLookDist) <= maxFOV / 2;
}

void GL3Renderer::renderChunk(Chunk &c) {
	vec3i64 cc = c.getCC();

	int length = conf.render_distance * 2 + 1;
	uint index = ((((cc[2] % length) + length) % length) * length
			+ (((cc[1] % length) + length) % length)) * length
			+ (((cc[0] % length) + length) % length);

	if (vaoStatus[index] != NO_CHUNK)
		faces -= chunkFaces[index];

	chunkFaces[index] = 0;
	vaoChunks[index] = cc;
	vaoStatus[index] = OK;
	chunkPassThroughs[index] = c.getPassThroughs();

	glBindBuffer(GL_ARRAY_BUFFER, vbos[index]);
	logOpenGLError();

	if (c.getAirBlocks() == Chunk::WIDTH * Chunk::WIDTH * Chunk::WIDTH) {
		glBufferData(GL_ARRAY_BUFFER, 0, 0, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		return;
	}

	newChunks++;

	vec3ui8 uDirs[3];
	for (uint8 d = 0; d < 3; d++) {
		uDirs[d] = DIRS[d].cast<uint8>();
	}

	size_t bufferSize = 0;

	const uint8 *blocks = c.getBlocks();
	for (uint8 d = 0; d < 3; d++) {
		vec3i64 dir = uDirs[d].cast<int64>();
		uint i = 0;
		uint ni = 0;
		for (int z = (d == 2) ? -1 : 0; z < (int) Chunk::WIDTH; z++) {
			for (int y = (d == 1) ? -1 : 0; y < (int) Chunk::WIDTH; y++) {
				for (int x = (d == 0) ? -1 : 0; x < (int) Chunk::WIDTH; x++) {
					uint8 thatType;
					uint8 thisType;
					if ((x == Chunk::WIDTH - 1 && d==0)
							|| (y == Chunk::WIDTH - 1 && d==1)
							|| (z == Chunk::WIDTH - 1 && d==2)) {
						thatType = world->getBlock(cc * Chunk::WIDTH + vec3i64(x, y, z) + dir);
						if (thatType != 0) {
							if (x != -1 && y != -1 && z != -1)
								i++;
							continue;
						}
					} else
						thatType = blocks[ni++];

					if (x == -1 || y == -1 || z == -1) {
						thisType = world->getBlock(cc * Chunk::WIDTH + vec3i64(x, y, z));
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
							vec3i64 v = EIGHT_CYCLES_3D[faceDir][j].cast<int64>();
							vec3i64 dIcc = faceBlock + v;
							uint8 cornerBlock;
							if (		dIcc[0] < 0 || dIcc[0] >= (int) Chunk::WIDTH
									||	dIcc[1] < 0 || dIcc[1] >= (int) Chunk::WIDTH
									||	dIcc[2] < 0 || dIcc[2] >= (int) Chunk::WIDTH)
								cornerBlock = world->getBlock(cc * Chunk::WIDTH + dIcc);
							else
								cornerBlock = c.getBlock(dIcc.cast<uint8>());
							if (cornerBlock) {
								corners |= 1 << j;
							}
						}
						vec3i64 bc = c.getCC() * c.WIDTH + faceBlock.cast<int64>();

						ushort posIndices[4];
						int shadowLevel[4];
						for (int j = 0; j < 4; j++) {
							shadowLevel[j] = 0;
							bool s1 = (corners & FACE_CORNER_MASK[j][0]) > 0;
							bool s2 = (corners & FACE_CORNER_MASK[j][2]) > 0;
							bool m = (corners & FACE_CORNER_MASK[j][1]) > 0;
							if (s1)
								shadowLevel[j]++;
							if (s2)
								shadowLevel[j]++;
							if (m && !(s1 && s2))
								shadowLevel[j]++;
							vec3ui8 vertex = faceBlock.cast<uint8>() + QUAD_CYCLES_3D[faceDir][j].cast<uint8>();
							posIndices[j] = (vertex[2] * (Chunk::WIDTH + 1) + vertex[1]) * (Chunk::WIDTH + 1) + vertex[0];
						}
						blockVertexBuffer[bufferSize].positionIndex = posIndices[0];
						blockVertexBuffer[bufferSize].dirIndexShadowLevel = faceDir | (shadowLevel[0] << 3);
						bufferSize++;
						blockVertexBuffer[bufferSize].positionIndex = posIndices[1];
						blockVertexBuffer[bufferSize].dirIndexShadowLevel = faceDir | (shadowLevel[1] << 3);
						bufferSize++;
						blockVertexBuffer[bufferSize].positionIndex = posIndices[2];
						blockVertexBuffer[bufferSize].dirIndexShadowLevel = faceDir | (shadowLevel[2] << 3);
						bufferSize++;
						blockVertexBuffer[bufferSize].positionIndex = posIndices[2];
						blockVertexBuffer[bufferSize].dirIndexShadowLevel = faceDir | (shadowLevel[2] << 3);
						bufferSize++;
						blockVertexBuffer[bufferSize].positionIndex = posIndices[3];
						blockVertexBuffer[bufferSize].dirIndexShadowLevel = faceDir | (shadowLevel[3] << 3);
						bufferSize++;
						blockVertexBuffer[bufferSize].positionIndex = posIndices[0];
						blockVertexBuffer[bufferSize].dirIndexShadowLevel = faceDir | (shadowLevel[0] << 3);
						bufferSize++;
					}
				}
			}
		}
	}

	glBufferData(GL_ARRAY_BUFFER, sizeof(BlockVertexData) * bufferSize, blockVertexBuffer, GL_STATIC_DRAW);
	logOpenGLError();

	chunkFaces[index] = bufferSize / 3;
	newFaces += chunkFaces[index];
	faces += chunkFaces[index];
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	logOpenGLError();
}

void GL3Renderer::renderMenu() {

}

void GL3Renderer::renderTarget() {

}

void GL3Renderer::renderPlayers() {

}
