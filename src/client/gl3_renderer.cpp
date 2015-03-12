#include "gl3_renderer.hpp"

#include <fstream>
#include <SDL2/SDL.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "engine/logging.hpp"
#include "engine/math.hpp"

#include "graphics.hpp"
#include "menu.hpp"
#include "stopwatch.hpp"

#include "game/world.hpp"
#include "game/chunk.hpp"

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
	loadShaders();
	initRenderDistanceDependent();

	/*// generate and bind new vao
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// generate and bind new vbo
	GLuint vertexBuffer;
	glGenBuffers(1, &vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);

	// fill vbo with data
	GLshort vertexBufferData[] = {1, 3200, 5578};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexBufferData), vertexBufferData, GL_STATIC_DRAW);

	// make attribute pointer for vao
	glVertexAttribIPointer(0, 1, GL_SHORT, 0, (void*)0);
	glEnableVertexAttribArray(0);

	// unbind vao
	glBindVertexArray(0);*/
}

GL3Renderer::~GL3Renderer() {
	LOG(DEBUG, "Destroying Graphics");
	destroyRenderDistanceDependent();
}

void GL3Renderer::loadShaders() {
	// Create the shaders
	GLuint vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string vertexShaderCode;
	std::ifstream vertexShaderStream("shaders/vertex_shader.vert", std::ios::in);
	if(vertexShaderStream.is_open())
	{
		std::string Line = "";
		while(getline(vertexShaderStream, Line))
			vertexShaderCode += "\n" + Line;
		vertexShaderStream.close();
	}

	// Read the Fragment Shader code from the file
	std::string fragmentShaderCode;
	std::ifstream fragmentShaderStream("shaders/fragment_shader.frag", std::ios::in);
	if(fragmentShaderStream.is_open()){
		std::string Line = "";
		while(getline(fragmentShaderStream, Line))
			fragmentShaderCode += "\n" + Line;
		fragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	LOG(DEBUG, "Compiling vertex shader");
	char const * VertexSourcePointer = vertexShaderCode.c_str();
	glShaderSource(vertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(vertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(vertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(vertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> VertexShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(vertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	// TODO use logging
	fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

	// Compile Fragment Shader
	LOG(DEBUG, "Compiling fragment shader");
	char const * FragmentSourcePointer = fragmentShaderCode.c_str();
	glShaderSource(fragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(fragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(fragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(fragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(fragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	// TODO use logging
	fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

	// Link the program
	LOG(DEBUG, "Linking program");
	progLoc = glCreateProgram();
	glAttachShader(progLoc, vertexShaderID);
	glAttachShader(progLoc, fragmentShaderID);
	glLinkProgram(progLoc);

	// Check the program
	glGetProgramiv(progLoc, GL_LINK_STATUS, &Result);
	glGetProgramiv(progLoc, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> programErrorMessage(std::max(InfoLogLength, int(1)));
	glGetProgramInfoLog(progLoc, InfoLogLength, NULL, &programErrorMessage[0]);
	// TODO use logging
	fprintf(stdout, "%s\n", &programErrorMessage[0]);

	glDeleteShader(vertexShaderID);
	glDeleteShader(fragmentShaderID);

	glUseProgram(progLoc);

	// save uniform locations
	projMatLoc = glGetUniformLocation(progLoc, "projectionMatrix");
	viewMatLoc = glGetUniformLocation(progLoc, "viewMatrix");
	modelMatLoc = glGetUniformLocation(progLoc, "modelMatrix");
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

void GL3Renderer::setPerspectiveMatrix() {
	glUniformMatrix4fv(projMatLoc, 1, GL_FALSE, glm::value_ptr(perspectiveMatrix));
}

void GL3Renderer::setOrthogonalMatrix() {
	glUniformMatrix4fv(projMatLoc, 1, GL_FALSE, glm::value_ptr(orthogonalMatrix));
}

void GL3Renderer::setViewMatrix() {
	glUniformMatrix4fv(viewMatLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
}

void GL3Renderer::setModelMatrix() {
	glUniformMatrix4fv(modelMatLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
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
		glVertexAttribIPointer(0, 1, GL_SHORT, 0, (void*)0);
		glEnableVertexAttribArray(0);
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

	stopwatch->start(CLOCK_FSH);
	//glFinish();
	stopwatch->stop(CLOCK_FSH);

	stopwatch->start(CLOCK_FLP);
	SDL_GL_SwapWindow(window);
	stopwatch->stop(CLOCK_FLP);

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

	setPerspectiveMatrix();

	Player &player = world->getPlayer(localClientID);
	if (player.isValid()) {

		modelMatrix = glm::mat4(1.0f);
		setModelMatrix();

		// Render sky
		viewMatrix = glm::rotate(glm::mat4(1.0f), (float) (-player.getPitch() / 360.0 * TAU), glm::vec3(1.0f, 0.0f, 0.0f));
		setViewMatrix();
		//renderSky();

		// Render Scene
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
		setViewMatrix();
		renderScene();
	}

	// render overlay
	setOrthogonalMatrix();
	modelMatrix = glm::mat4(1.0);

	if (state == PLAYING && player.isValid()) {
		stopwatch->start(CLOCK_HUD);
		//renderHud(player);
		//if (debugActive)
		//	renderDebugInfo(player);
		stopwatch->stop(CLOCK_HUD);
	} else if (state == IN_MENU){
		renderMenu();
	}
}

void GL3Renderer::renderScene() {
	/*glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);
	if (conf.fog != Fog::NONE) glEnable(GL_FOG);

	glLightfv(GL_LIGHT0, GL_POSITION, sunLightPosition.ptr());*/

	// render chunks
	stopwatch->start(CLOCK_CHR);
	renderChunks();
	renderTarget();
	stopwatch->stop(CLOCK_CHR);

	// render players
	stopwatch->start(CLOCK_PLA);
	renderPlayers();
	stopwatch->stop(CLOCK_PLA);
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
				stopwatch->start(CLOCK_DLC);
				glm::mat4 oldModelMatrix = modelMatrix;
				modelMatrix = glm::translate(modelMatrix, glm::vec3((float) (cd[0] * (int) Chunk::WIDTH), (float) (cd[1] * (int) Chunk::WIDTH), (float) (cd[2] * (int) Chunk::WIDTH)));
				setModelMatrix();
				glBindVertexArray(vaos[index]);
				glDrawArrays(GL_TRIANGLES, 0, chunkFaces[index] * 3);
				logOpenGLError();
				modelMatrix = oldModelMatrix;
				stopwatch->stop(CLOCK_DLC);
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
	stopwatch->start(CLOCK_NDL);
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

	if (c.getAirBlocks() == Chunk::WIDTH * Chunk::WIDTH * Chunk::WIDTH) {
		glBufferData(GL_ARRAY_BUFFER, 0, 0, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		stopwatch->stop(CLOCK_NDL);
		return;
	}

	newChunks++;

	vec3ui8 uDirs[3];
	for (uint8 d = 0; d < 3; d++) {
		uDirs[d] = DIRS[d].cast<uint8>();
	}

	ushort vertexBufferData[Chunk::WIDTH * (Chunk::WIDTH + 1) * 3 * 2 * 3];
	size_t bufferSize = 0;

	const uint8 *blocks = c.getBlocks();
	int fbi = 0;
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

						vec2f texs[4];
						vec3i64 bc = c.getCC() * c.WIDTH + faceBlock.cast<int64>();

						ushort posIndices[4];
						float light[4];
						for (int j = 0; j < 4; j++) {
							light[j] = 1.0;
							bool s1 = (corners & FACE_CORNER_MASK[j][0]) > 0;
							bool s2 = (corners & FACE_CORNER_MASK[j][2]) > 0;
							bool m = (corners & FACE_CORNER_MASK[j][1]) > 0;
							if (s1)
								light[j] -= 0.2;
							if (s2)
								light[j] -= 0.2;
							if (m && !(s1 && s2))
								light[j] -= 0.2;
							vec3f vertex = (faceBlock.cast<int>() + QUAD_CYCLES_3D[faceDir][j]).cast<float>();
							posIndices[j] = (vertex[2] * Chunk::WIDTH + vertex[1]) * Chunk::WIDTH + vertex[0];
						}
						vertexBufferData[bufferSize++] = posIndices[0];
						vertexBufferData[bufferSize++] = posIndices[1];
						vertexBufferData[bufferSize++] = posIndices[2];
						vertexBufferData[bufferSize++] = posIndices[2];
						vertexBufferData[bufferSize++] = posIndices[3];
						vertexBufferData[bufferSize++] = posIndices[0];
					}
				}
			}
		}
	}

	glBufferData(GL_ARRAY_BUFFER, sizeof(ushort) * bufferSize, vertexBufferData, GL_STATIC_DRAW);

	chunkFaces[index] = bufferSize / 3;
	newFaces += chunkFaces[index];
	faces += chunkFaces[index];
	stopwatch->stop(CLOCK_NDL);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GL3Renderer::renderMenu() {

}

void GL3Renderer::renderTarget() {

}

void GL3Renderer::renderPlayers() {

}
