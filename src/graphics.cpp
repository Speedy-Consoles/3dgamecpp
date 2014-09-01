#include "graphics.hpp"
#include "vmath.hpp"
#include "util.hpp"
#include "constants.hpp"
#include <cmath>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <GL/glut.h>

Graphics::Graphics(World *world, int localClientID)
		: displayLists(0, vec3i64HashFunc) {
	this->world = world;
	this->localClientID = localClientID;

	//lightPosition.put(5.0f).put(-3.0f).put(20.0f).put(0.0f).flip();

	SDL_Init(SDL_INIT_VIDEO);

	window = SDL_CreateWindow(
		"3dgame",
		0, 0,
		START_WIDTH, START_HEIGHT,
		SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE
	);
	glContext = SDL_GL_CreateContext(window);

	initGL();
	resize(START_WIDTH, START_HEIGHT);

	if (START_WIDTH <= START_HEIGHT)
		maxFOV = YFOV;
	else
		maxFOV = atan(START_WIDTH * tan(YFOV / 2) / START_HEIGHT) * 2;

	font = new FTGLTextureFont("res/DejaVuSansMono.ttf");
    font->FaceSize(16);
    font->CharMap(ft_encoding_unicode);

	startTimePoint = high_resolution_clock::now();
}

Graphics::~Graphics() {
    delete font;
	SDL_GL_DeleteContext(glContext);
	SDL_Quit();
}

void Graphics::tick() {
	render();
	SDL_GL_SwapWindow(window);
	removeDisplayLists();
	frameCounter++;
	int64 time = getMicroTimeSince(startTimePoint);
	if (time - lastFPSUpdate > 1000000) {
		lastFPS = frameCounter;
		lastFPSUpdate += 1000000;
		frameCounter = 0;
	}
}

void Graphics::resize(int width, int height) {
	this->width = width;
	this->height = height;
	glViewport(0, 0, width, height);
	makePerspective();
	makeOrthogonal();
	calcDrawArea();
}

void Graphics::grab() {
	SDL_SetWindowGrab(window, (SDL_bool) !SDL_GetWindowGrab(window));
	SDL_SetRelativeMouseMode((SDL_bool) !SDL_GetRelativeMouseMode());
}

bool Graphics::isGrabbed() {
	return SDL_GetWindowGrab(window);
}

void Graphics::initGL() {

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(1);
	glDepthFunc(GL_LEQUAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glMatrixMode(GL_MODELVIEW);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_CULL_FACE);

	// light
	//float matSpecular[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	float sunLight[4] = {0.5f, 0.5f, 0.4f, 1.0f};
	float lModelAmbient[4] = {0.5f, 0.5f, 0.5f, 1.0f};

	glEnable(GL_LIGHTING); // enables lighting
	glShadeModel(GL_SMOOTH);
	//		glMaterial(GL_FRONT, GL_SPECULAR, matSpecular); // sets specular material color
	//		glMaterialf(GL_FRONT, GL_SHININESS, 1.0f); // sets shininess

	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lModelAmbient); // global ambient light

	//		glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0.01f);
	//		glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.001f);
	//		glLight(GL_LIGHT0, GL_SPECULAR, whiteLight); // sets specular light to white
	glLightfv(GL_LIGHT0, GL_DIFFUSE, sunLight);
	glEnable(GL_LIGHT0);

	glEnable(GL_COLOR_MATERIAL); // enables opengl to use glColor3f to define material color
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE); // tell opengl glColor3f effects the ambient and diffuse properties of material

	// textures
	glEnable(GL_TEXTURE_2D);
	IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF);
	SDL_Surface *blockImage = IMG_Load("img/block.png");

	glGenTextures(1, &blockTexture);
	glBindTexture(GL_TEXTURE_2D, blockTexture);
	gluBuild2DMipmaps(GL_TEXTURE_2D,4,128,128,GL_RGBA,GL_UNSIGNED_BYTE,blockImage->pixels);

	SDL_FreeSurface(blockImage);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0);

	// fog
	//float fogColor[4] = {0.5f, 0.5f, 0.5f, 1.0f};

	//		glEnable(GL_FOG);
	//		glFogi(GL_FOG_MODE, GL_EXP2);
	//		glFog(GL_FOG_COLOR, fogColor);
	//		glFogf(GL_FOG_DENSITY, 0.01f);
	//		glHint(GL_FOG_HINT, GL_NICEST);
	//		//		glFogf(GL_FOG_START, (DEFAULT_VIEW_RANGE - 2) * Chunk::X_WIDTH);
	//		//		glFogf(GL_FOG_END, (DEFAULT_VIEW_RANGE - 1) * Chunk::X_WIDTH);
}

void Graphics::makePerspective() {
	double normalRatio = START_WIDTH / (double) START_HEIGHT;
	double currentRatio = width / (double) height;
	double angle;
	if (currentRatio > normalRatio)
		angle = atan(tan(YFOV / 2) * normalRatio / currentRatio) * 2;
	else
		angle = YFOV;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	float zFar = (float) sqrt(pow(Chunk::WIDTH * VIEW_RANGE, 2)
			+ pow(Chunk::WIDTH * VIEW_RANGE, 2)
			+ pow(Chunk::WIDTH * VIEW_RANGE, 2));
	gluPerspective((float) (angle * 360.0 / TAU),
			(float) currentRatio, 0.1f, zFar);
	glGetDoublev(GL_PROJECTION_MATRIX, perspectiveMatrix);
	glMatrixMode(GL_MODELVIEW);
}

void Graphics::makeOrthogonal() {
	double normalRatio = START_WIDTH / (double) START_HEIGHT;
	double currentRatio = width / (double) height;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if (currentRatio > normalRatio)
		glOrtho(-START_WIDTH / 2.0, START_WIDTH / 2.0, START_WIDTH
				/ currentRatio / 2.0, -START_WIDTH / currentRatio / 2.0, 1,
				-1);
	else
		glOrtho(-START_HEIGHT * currentRatio / 2.0, START_HEIGHT
				* currentRatio / 2.0, -START_HEIGHT / 2.0,
				START_HEIGHT / 2.0, 1, -1);
	glGetDoublev(GL_PROJECTION_MATRIX, orthogonalMatrix);
	glMatrixMode(GL_MODELVIEW);
}

void Graphics::switchToPerspective() {
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(perspectiveMatrix);
	glMatrixMode(GL_MODELVIEW);
}

void Graphics::switchToOrthogonal() {
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(orthogonalMatrix);
	glMatrixMode(GL_MODELVIEW);
}

void Graphics::calcDrawArea() {
	double normalRatio = START_WIDTH / (double) START_HEIGHT;
	double currentRatio = width / (double) height;
	if (currentRatio > normalRatio) {
		drawWidth = START_WIDTH;
		drawHeight = START_WIDTH / currentRatio;
	} else {
		drawWidth = START_HEIGHT * currentRatio;
		drawHeight = START_HEIGHT;
	}
}

void Graphics::render() {
	Player &localPlayer = world->getPlayer(localClientID);
	if (!localPlayer.isValid())
		return;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	switchToPerspective();
	glEnable(GL_LIGHTING);
	//		glEnable(GL_FOG);
	glEnable(GL_DEPTH_TEST);
	glLoadIdentity();
	// player yaw
	glRotated(-localPlayer.getPitch(), 1, 0, 0);
	// player pitch
	glRotated(-localPlayer.getYaw(), 0, 1, 0);
	// tilt coordinate system so z points up
	glRotated(-90, 1, 0, 0);
	// tilt coordinate system so we look into x direction
	glRotated(90, 0, 0, 1);
	//player position
	vec3i64 playerPos = localPlayer.getPos();
	glTranslated(
		-playerPos[0] / (double) RESOLUTION,
		-playerPos[1] / (double) RESOLUTION,
		-playerPos[2] / (double) RESOLUTION
	);

	// now we are in world coordinates

	// place light
	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

	// render chunk
	renderChunks();

	// render players
	renderPlayers();

	switchToOrthogonal();
	glDisable(GL_LIGHTING);
	glDisable(GL_FOG);
	glDisable(GL_DEPTH_TEST);
	glLoadIdentity();

	// render overlay
	glColor4d(0, 0, 0, 0.5);
	glBegin(GL_QUADS);
	glVertex2d(-20, -2);
	glVertex2d(20, -2);
	glVertex2d(20, 2);
	glVertex2d(-20, 2);

	glVertex2d(-2, -20);
	glVertex2d(2, -20);
	glVertex2d(2, 20);
	glVertex2d(-2, 20);
	glEnd();

	vec3d playerVel = localPlayer.getVel();

    glPushMatrix();
	glColor3f(1.0f, 1.0f, 1.0f);
	glTranslatef(-drawWidth / 2 + 3, drawHeight / 2, 0);
	char buffer[1024];
	#define RENDER_LINE(args...) sprintf(buffer, args);\
			glTranslatef(0, -16, 0);\
			font->Render(buffer)

	RENDER_LINE("fps: %d", lastFPS);
	RENDER_LINE("quads: %d", lastNewQuads);
	RENDER_LINE("x: %ld", playerPos[0]);
	RENDER_LINE("y: %ld", playerPos[1]);
	RENDER_LINE("z: %ld", playerPos[2]);
	RENDER_LINE("yaw:   %6.1f", localPlayer.getYaw());
	RENDER_LINE("pitch: %6.1f", localPlayer.getPitch());
	RENDER_LINE("xvel: %8.1f", playerVel[0]);
	RENDER_LINE("yvel: %8.1f", playerVel[1]);
	RENDER_LINE("zvel: %8.1f", playerVel[2]);
    glPopMatrix();
}

void Graphics::renderChunks() {
	using namespace vec_auto_cast;
	lastNewQuads = 0;
	Player localPlayer = world->getPlayer(localClientID);
	vec3i64 pc = localPlayer.getChunkPos();
	vec3d lookDir = getVectorFromAngles(localPlayer.getYaw(),
			localPlayer.getPitch());

	vec3i64 tbc;
	vec3i64 tcc;
	vec3ui8 ticc;
	int td;
	bool target = localPlayer.getTargetedFace(&tbc, &td);
	if (target) {
		tcc = bc2cc(tbc);
		ticc = bc2icc(tbc);
	}

	//		DoubleBuffer globalMat = BufferUtils.createDoubleBuffer(16);
	//		glGetDouble(GL_MODELVIEW_MATRIX, globalMat);
	int length = VIEW_RANGE * 2 + 1;
	int maxChunks = length * length * length;
	int chunks = 0;
	for (uint i = 0; i < LOADING_ORDER.size() && chunks < maxChunks; i++) {
		vec3i8 cd = LOADING_ORDER[i];
		if (cd.maxAbs() > VIEW_RANGE)
			continue;
		chunks++;

		vec3i64 cc = pc + cd;
		auto chunkIt= world->getChunks().find(cc);

		auto listIt = displayLists.find(cc);
		GLuint lid = 0;
		if (chunkIt != world->getChunks().end()
				&& listIt == displayLists.end()) {
			lid = glGenLists(1);
			if (lid != 0)
				displayLists.insert({cc, lid});
		} else if(listIt != displayLists.end())
			lid = listIt->second;

		if (chunkIt != world->getChunks().end()
				&& chunkIt->second.isReady()
				&& chunkIt->second.pollChanged()
				&& lid != 0) {
			glNewList(lid, GL_COMPILE);
			renderChunk(chunkIt->second, false, vec3ui8(0, 0, 0), 0);
			glEndList();
		}

		//			glTranslated(cc[0] * Chunk::WIDTH, cc[1] * Chunk::WIDTH, cc[2]
		//					* Chunk::WIDTH);

		bool tChunk = target && cc == tcc;
		if (inFrustum(cc, localPlayer.getPos(), lookDir)) {
			if (chunkIt != world->getChunks().end() && (tChunk || lid == 0))
				renderChunk(chunkIt->second, tChunk, ticc, td);
			else if (lid != 0)
				glCallList(lid);
		}

		//			glLoadMatrix(globalMat);
	}
}

void Graphics::renderChunk(Chunk &c, bool targeted, vec3ui8 ticc, int td) {
	using namespace vec_auto_cast;
	//render blocks
	glBindTexture(GL_TEXTURE_2D, blockTexture);
	glBegin(GL_QUADS);

	const Chunk::FaceSet &faceSet = c.getFaceSet();
	// TODO thread safe?
	for (Face f : faceSet) {
		lastNewQuads++;

		if (targeted && f.block == ticc && f.dir == td)
			glColor3d(0.8, 0.2, 0.2);
		else
			glColor3d(1.0, 1.0, 1.0);

		glNormal3d(DIRS[f.dir][0], DIRS[f.dir][1], DIRS[f.dir][2]);
		for (int j = 0; j < 4; j++) {
			glTexCoord2d(QUAD_CYCLE_2D[j][0], QUAD_CYCLE_2D[j][1]);
			//					glVertex3d(f.x + c3d[j][0], f.y + c3d[j][1], f.z
			//							+ c3d[j][2]);
			vec3d vertex = (c.cc * Chunk::WIDTH
					+ f.block + QUAD_CYCLES_3D[f.dir][j]).cast<double>();
			glVertex3d(vertex[0], vertex[1], vertex[2]);
		}
	}
	glEnd();
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Graphics::renderPlayers() {
	using namespace vec_auto_cast;
	glBegin(GL_QUADS);
	for (uint i = 0; i < MAX_CLIENTS; i++) {
		if (i == localClientID)
			continue;
		Player &player = world->getPlayer(i);
		if (!player.isValid())
			continue;
		vec3i64 pos = player.getPos();
		for (int d = 0; d < 6; d++) {
			vec3i dir = DIRS[d];
			glColor3d(0.8, 0.2, 0.2);

			glNormal3d(dir[0], dir[1], dir[2]);
			for (int j = 0; j < 4; j++) {
				vec3i off(
					(QUAD_CYCLES_3D[d][j][0] * 2 - 1) * Player::RADIUS,
					(QUAD_CYCLES_3D[d][j][1] * 2 - 1) * Player::RADIUS,
					QUAD_CYCLES_3D[d][j][2] * Player::HEIGHT - Player::EYE_HEIGHT
				);
				glTexCoord2d(QUAD_CYCLE_2D[j][0], QUAD_CYCLE_2D[j][1]);
				vec3d vertex = (pos + off).cast<double>() * (1.0 / RESOLUTION);
				glVertex3d(vertex[0], vertex[1], vertex[2]);
			}
		}
	}
	glEnd();
}

bool Graphics::inFrustum(vec3i64 cc, vec3i64 pos, vec3d lookDir) {
	// TODO fix this
	return true;
	using namespace vec_auto_cast;
	double chunkDia = vec3ui8(Chunk::WIDTH, Chunk::WIDTH, Chunk::WIDTH).norm()
			* RESOLUTION;
	vec3d cp = (cc * Chunk::WIDTH * RESOLUTION - pos).cast<double>();
	double chunkLookDist = lookDir * cp + chunkDia;
	if (chunkLookDist < 0)
		return false;
	vec3d orthoChunkPos = cp - lookDir * chunkLookDist;
	double orthoChunkDist = std::max(0.0, orthoChunkPos.norm() - chunkDia);
	return atan(orthoChunkDist / chunkLookDist) <= maxFOV / 2;
}

void Graphics::removeDisplayLists() {
	vec3i64 pcc = world->getPlayer(localClientID).getChunkPos();
	for (auto iter = displayLists.begin(); iter != displayLists.end();) {
		vec3i64 cc = iter->first;
		if ((cc - pcc).maxAbs() > VIEW_RANGE) {
			glDeleteLists(iter->second, 1);
			iter = displayLists.erase(iter);
		} else
			iter++;
	}
}
