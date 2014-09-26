#include "graphics.hpp"

#include "game/world.hpp"
#include "game/chunk.hpp"
#include "stopwatch.hpp"

#include "io/logging.hpp"

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

void Graphics::render() {
	Player &localPlayer = world->getPlayer(localClientID);
	if (!localPlayer.isValid())
		return;

	if (fbo) {
		// render to the fbo and not the screen
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		logOpenGLError();
	} else {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		logOpenGLError();
		glDrawBuffer(GL_BACK);
		logOpenGLError();
	}

	stopwatch->start(CLOCK_CLR);
	glClear(GL_DEPTH_BUFFER_BIT);
	logOpenGLError();
	stopwatch->stop(CLOCK_CLR);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glPushMatrix();
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);
	renderScene(localPlayer);
	logOpenGLError();

	if (fbo) {
		// copy framebuffer to screen, blend multisampling on the way
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
		glDrawBuffer(GL_BACK);
//		if (fxaa) {
//			glMatrixMode(GL_PROJECTION);
//			glLoadIdentity();
//			glMatrixMode(GL_MODELVIEW);
//			glLoadIdentity();
//
//			glDisable(GL_DEPTH_TEST);
//			glDisable(GL_LIGHTING);
//			glDisable(GL_FOG);
//			glEnable(GL_TEXTURE_2D);
//			glColor3f(1.0f, 1.0f, 1.0f);
//
//			glBindTexture(GL_TEXTURE_2D, fbo_texture);
//			glUseProgram(program_postproc);
//			glBegin(GL_QUADS);
//				glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, -1.0f);
//				glTexCoord2f(1.0f, 0.0f); glVertex2f(+1.0f, -1.0f);
//				glTexCoord2f(1.0f, 1.0f); glVertex2f(+1.0f, +1.0f);
//				glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, +1.0f);
//			glEnd();
//		} else {
		glBlitFramebuffer(
				0, 0, width, height,
				0, 0, width, height,
				GL_COLOR_BUFFER_BIT,
				GL_NEAREST
		);
//		}
		logOpenGLError();

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDrawBuffer(GL_BACK);
	}

	if (!menuActive) {
		stopwatch->start(CLOCK_HUD);
		renderHud(localPlayer);
		if (debugActive)
			renderDebugInfo(localPlayer);
		stopwatch->stop(CLOCK_HUD);
	} else {
		renderMenu();
	}

	glMatrixMode(GL_TEXTURE);
	glPopMatrix();
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}

void Graphics::renderScene(const Player &player) {
	//glUseProgram(program);
	switchToPerspective();
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glDisable(GL_FOG);
	glLoadIdentity();
	// player pitch
	glRotated(-player.getPitch(), 1, 0, 0);

	// sky
	glDepthMask(false);
	glBegin(GL_QUADS);

	glColor3fv(fogColor);
	glVertex3d(-2, -2, 2);
	glVertex3d(2, -2, 2);
	glVertex3d(2, 0.3, -1);
	glVertex3d(-2, 0.3, -1);

	glVertex3d(-2, 0.3, -1);
	glVertex3d(2, 0.3, -1);
	glColor3fv(skyColor);
	glVertex3d(2, 2, 2);
	glVertex3d(-2, 2, 2);

	glEnd();
	glDepthMask(true);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);
	if (conf.fog != Fog::NONE) glEnable(GL_FOG);

	// player yaw
	glRotated(-player.getYaw(), 0, 1, 0);
	// tilt coordinate system so z points up
	glRotated(-90, 1, 0, 0);
	// tilt coordinate system so we look into x direction
	glRotated(90, 0, 0, 1);
	//player position
	vec3i64 playerPos = player.getPos();
	int64 m = RESOLUTION * Chunk::WIDTH;
	glTranslated(
		-((playerPos[0] % m + m) % m) / (double) RESOLUTION,
		-((playerPos[1] % m + m) % m) / (double) RESOLUTION,
		-((playerPos[2] % m + m) % m) / (double) RESOLUTION
	);

	// place light
	glLightfv(GL_LIGHT0, GL_POSITION, sunLightPosition);

	logOpenGLError();

	// render chunk
	stopwatch->start(CLOCK_CHR);
	renderChunks();
	stopwatch->stop(CLOCK_CHR);

	// render players
	stopwatch->start(CLOCK_PLA);
	renderPlayers();
	stopwatch->stop(CLOCK_PLA);
}

void Graphics::renderChunks() {
	using namespace vec_auto_cast;

	Player &localPlayer = world->getPlayer(localClientID);
	vec3i64 pc = localPlayer.getChunkPos();
	vec3d lookDir = getVectorFromAngles(localPlayer.getYaw(), localPlayer.getPitch());

	vec3i64 tbc;
	vec3i64 tcc;
	vec3ui8 ticc;
	int td;
	bool target = localPlayer.getTargetedFace(&tbc, &td);
	if (target) {
		tcc = bc2cc(tbc);
		ticc = bc2icc(tbc);
	}

	newFaces = 0;
	int length = conf.render_distance * 2 + 3;

	texManager.bind(0);

	stopwatch->start(CLOCK_NDL);
	vec3i64 ccc;
	while (newFaces < MAX_NEW_QUADS && world->popChangedChunk(&ccc)) {
		Chunk *chunk = world->getChunk(ccc);
		if(chunk) {
			uint index = ((((ccc[2] % length) + length) % length) * length
					+ (((ccc[1] % length) + length) % length)) * length
					+ (((ccc[0] % length) + length) % length);
			if (chunk->pollChanged() || !dlHasChunk[index] || dlChunks[index] != ccc) {
				GLuint lid = firstDL + index;
				faces -= dlFaces[index];
				glNewList(lid, GL_COMPILE);
				dlFaces[index] = renderChunk(*chunk);
				glEndList();
				dlChunks[index] = ccc;
				dlHasChunk[index] = true;
				faces += dlFaces[index];
				newFaces += dlFaces[index];
			}
		}
	}
	stopwatch->stop(CLOCK_NDL);

	logOpenGLError();

	uint maxChunks = length * length * length;
	uint renderedChunks = 0;
	for (uint i = 0; i < LOADING_ORDER.size() && renderedChunks < maxChunks; i++) {
		vec3i8 cd = LOADING_ORDER[i];
		if (cd.maxAbs() > (int) conf.render_distance)
			continue;
		renderedChunks++;

		vec3i64 cc = pc + cd;

		uint index = ((((cc[2] % length) + length) % length) * length
				+ (((cc[1] % length) + length) % length)) * length
				+ (((cc[0] % length) + length) % length);
		GLuint lid = firstDL + index;

		if (lid != 0
				&& dlFaces[index] > 0
				&& inFrustum(cc, localPlayer.getPos(), lookDir)
				&& dlHasChunk[index]
				&& dlChunks[index] == cc) {
			stopwatch->start(CLOCK_DLC);
			glPushMatrix();
			logOpenGLError();
			glTranslatef(cd[0] * (int) Chunk::WIDTH, cd[1] * (int) Chunk::WIDTH, cd[2] * (int) Chunk::WIDTH);
			glCallList(lid);
			logOpenGLError();
			glPopMatrix();
			logOpenGLError();
			stopwatch->stop(CLOCK_DLC);
		}
	}

	logOpenGLError();

	if (target) {
		glDisable(GL_TEXTURE_2D);
		glBegin(GL_QUADS);
		glColor4d(0.0, 0.0, 0.0, 1.0);
		vec3d pointFive(0.5, 0.5, 0.5);
		for (int d = 0; d < 6; d++) {
			glNormal3d(DIRS[d][0], DIRS[d][1], DIRS[d][2]);
			for (int j = 0; j < 4; j++) {
				vec3d dirOff = DIRS[d].cast<double>() * 0.00005;
				vec3d vOff[4];
				vOff[0] = QUAD_CYCLES_3D[d][j].cast<double>() - pointFive;
				vOff[0][OTHER_DIR_DIMS[d][0]] *= 1.0001;
				vOff[0][OTHER_DIR_DIMS[d][1]] *= 1.0001;
				vOff[1] = QUAD_CYCLES_3D[d][j].cast<double>() - pointFive;
				vOff[1][OTHER_DIR_DIMS[d][0]] *= 0.97;
				vOff[1][OTHER_DIR_DIMS[d][1]] *= 0.97;
				vOff[2] = QUAD_CYCLES_3D[d][(j + 3) % 4].cast<double>() - pointFive;
				vOff[2][OTHER_DIR_DIMS[d][0]] *= 0.97;
				vOff[2][OTHER_DIR_DIMS[d][1]] *= 0.97;
				vOff[3] = QUAD_CYCLES_3D[d][(j + 3) % 4].cast<double>() - pointFive;
				vOff[3][OTHER_DIR_DIMS[d][0]] *= 1.0001;
				vOff[3][OTHER_DIR_DIMS[d][1]] *= 1.0001;

				for (int k = 0; k < 4; k++) {
					vec3d vertex = (tbc - pc * Chunk::WIDTH).cast<double>() + dirOff + vOff[k] + pointFive;
					glVertex3d(vertex[0], vertex[1], vertex[2]);
				}
			}
		}
		glEnd();
		glEnable(GL_TEXTURE_2D);
	}
	logOpenGLError();
}

static const uint8 shuffle[256] = {
0xa3,0xd7,0x09,0x83,0xf8,0x48,0xf6,0xf4,0xb3,0x21,0x15,0x78,0x99,0xb1,0xaf,0xf9,
0xe7,0x2d,0x4d,0x8a,0xce,0x4c,0xca,0x2e,0x52,0x95,0xd9,0x1e,0x4e,0x38,0x44,0x28,
0x0a,0xdf,0x02,0xa0,0x17,0xf1,0x60,0x68,0x12,0xb7,0x7a,0xc3,0xe9,0xfa,0x3d,0x53,
0x96,0x84,0x6b,0xba,0xf2,0x63,0x9a,0x19,0x7c,0xae,0xe5,0xf5,0xf7,0x16,0x6a,0xa2,
0x39,0xb6,0x7b,0x0f,0xc1,0x93,0x81,0x1b,0xee,0xb4,0x1a,0xea,0xd0,0x91,0x2f,0xb8,
0x55,0xb9,0xda,0x85,0x3f,0x41,0xbf,0xe0,0x5a,0x58,0x80,0x5f,0x66,0x0b,0xd8,0x90,
0x35,0xd5,0xc0,0xa7,0x33,0x06,0x65,0x69,0x45,0x00,0x94,0x56,0x6d,0x98,0x9b,0x76,
0x97,0xfc,0xb2,0xc2,0xb0,0xfe,0xdb,0x20,0xe1,0xeb,0xd6,0xe4,0xdd,0x47,0x4a,0x1d,
0x42,0xed,0x9e,0x6e,0x49,0x3c,0xcd,0x43,0x27,0xd2,0x07,0xd4,0xde,0xc7,0x67,0x18,
0x89,0xcb,0x30,0x1f,0x8d,0xc6,0x8f,0xaa,0xc8,0x74,0xdc,0xc9,0x5d,0x5c,0x31,0xa4,
0x70,0x88,0x61,0x2c,0x9f,0x0d,0x2b,0x87,0x50,0x82,0x54,0x64,0x26,0x7d,0x03,0x40,
0x34,0x4b,0x1c,0x73,0xd1,0xc4,0xfd,0x3b,0xcc,0xfb,0x7f,0xab,0xe6,0x3e,0x5b,0xa5,
0xad,0x04,0x23,0x9c,0x14,0x51,0x22,0xf0,0x29,0x79,0x71,0x7e,0xff,0x8c,0x0e,0xe2,
0x0c,0xef,0xbc,0x72,0x75,0x6f,0x37,0xa1,0xec,0xd3,0x8e,0x62,0x8b,0x86,0x10,0xe8,
0x08,0x77,0x11,0xbe,0x92,0x4f,0x24,0xc5,0x32,0x36,0x9d,0xcf,0xf3,0xa6,0xbb,0xac,
0x5e,0x6c,0xa9,0x13,0x57,0x25,0xb5,0xe3,0xbd,0xa8,0x3a,0x01,0x05,0x59,0x2a,0x46,
};

static uint8 scramble(vec3i64 vec) {
	const uint8 *data = reinterpret_cast<uint8 *>(&vec);
	uint32 result = 1;
	for (uint i = 0; i < sizeof (vec3i64); ++i) {
		result *= 31;
		result += data[i];
		result ^= shuffle[ data[i] & 0xFF ];
	}

	return result ^ (result >> 8) ^ (result >> 16) ^ (result >> 24);
}

int Graphics::renderChunk(const Chunk &c) {
	using namespace vec_auto_cast;
	int faces = 0;

	texManager.bind(0);
	glBegin(GL_QUADS);
	const Chunk::FaceSet &faceSet = c.getFaces();
	for (Face f : faceSet) {
		auto nextBlock = c.getBlock(f.block);
		texManager.bind(nextBlock, GL_QUADS);
		logOpenGLError();

		if (texManager.isWangTileBound()) {
			uint8 left, right, top, bot;
			enum { RIGHT, BACK, TOP, LEFT, FRONT, BOTTOM };
			vec3i64 coord = f.block + c.getCC() * c.WIDTH;
			switch (f.dir) {
				case RIGHT:
					left   = (uint8) scramble(coord + vec3i64(1, 0, 0)) & 0x01;
					right  = (uint8) scramble(coord + vec3i64(1, 1, 0)) & 0x01;
					bot    = (uint8) scramble(coord + vec3i64(0, 0, 0)) & 0x02;
					top    = (uint8) scramble(coord + vec3i64(0, 0, 1)) & 0x02;
					break;
				case LEFT:
					left   = (uint8) scramble(coord + vec3i64(0, 1, 0)) & 0x01;
					right  = (uint8) scramble(coord + vec3i64(0, 0, 0)) & 0x01;
					bot    = (uint8) scramble(coord + vec3i64(0, 0, 0)) & 0x02;
					top    = (uint8) scramble(coord + vec3i64(0, 0, 1)) & 0x02;
					break;
				case BACK:
					left   = (uint8) scramble(coord + vec3i64(1, 1, 0)) & 0x01;
					right  = (uint8) scramble(coord + vec3i64(0, 1, 0)) & 0x01;
					bot    = (uint8) scramble(coord + vec3i64(0, 0, 0)) & 0x02;
					top    = (uint8) scramble(coord + vec3i64(0, 0, 1)) & 0x02;
					break;
				case FRONT:
					left   = (uint8) scramble(coord + vec3i64(0, 0, 0)) & 0x01;
					right  = (uint8) scramble(coord + vec3i64(1, 0, 0)) & 0x01;
					bot    = (uint8) scramble(coord + vec3i64(0, 0, 0)) & 0x02;
					top    = (uint8) scramble(coord + vec3i64(0, 0, 1)) & 0x02;
					break;
				case TOP:
					left   = (uint8) scramble(coord + vec3i64(0, 0, 0)) & 0x04;
					right  = (uint8) scramble(coord + vec3i64(1, 0, 0)) & 0x04;
					bot    = (uint8) scramble(coord + vec3i64(0, 0, 0)) & 0x08;
					top    = (uint8) scramble(coord + vec3i64(0, 1, 0)) & 0x08;
					break;
				case BOTTOM:
					left   = (uint8) scramble(coord + vec3i64(1, 0, 0)) & 0x10;
					right  = (uint8) scramble(coord + vec3i64(0, 0, 0)) & 0x10;
					bot    = (uint8) scramble(coord + vec3i64(0, 0, 0)) & 0x20;
					top    = (uint8) scramble(coord + vec3i64(0, 1, 0)) & 0x20;
					break;
			}

			uint8 variant = 0;
			if      ( left && !right) variant += 1;
			else if (!left && !right) variant += 2;
			else if (!left &&  right) variant += 3;
			if      ( bot && !top)    variant += 4;
			else if (!bot && !top)    variant += 8;
			else if (!bot &&  top)    variant += 12;

			glEnd();
			glMatrixMode(GL_TEXTURE);
			glPopMatrix();
			glPushMatrix();
			glTranslatef(variant % 4, 3 - (variant / 4), 0);
			glMatrixMode(GL_MODELVIEW);
			glBegin(GL_QUADS);
		}

		vec3d color = {1.0, 1.0, 1.0};

		glNormal3d(DIRS[f.dir][0], DIRS[f.dir][1], DIRS[f.dir][2]);
		for (int j = 0; j < 4; j++) {
			float s = QUAD_CYCLE_2D[j][0];
			float t = 1.0 - QUAD_CYCLE_2D[j][1];
			glTexCoord2f(s, t);
			double light = 1.0;
			bool s1 = (f.corners & FACE_CORNER_MASK[j][0]) > 0;
			bool s2 = (f.corners & FACE_CORNER_MASK[j][2]) > 0;
			bool m = (f.corners & FACE_CORNER_MASK[j][1]) > 0;
			if (s1)
				light -= 0.2;
			if (s2)
				light -= 0.2;
			if (m && !(s1 && s2))
				light -= 0.2;
			glColor3d(color[0] * light, color[1] * light, color[2] * light);
			vec3d vertex = (f.block + QUAD_CYCLES_3D[f.dir][j]).cast<double>();
			glVertex3d(vertex[0], vertex[1], vertex[2]);
		}
		faces++;
	}
	glEnd();
	return faces;
}

void Graphics::renderPlayers() {
	using namespace vec_auto_cast;
	glBindTexture(GL_TEXTURE_2D, 0);
	glBegin(GL_QUADS);
	for (uint i = 0; i < MAX_CLIENTS; i++) {
		if (i == localClientID)
			continue;
		Player &player = world->getPlayer(i);
		if (!player.isValid())
			continue;
		vec3i64 pos = player.getPos();
		for (int d = 0; d < 6; d++) {
			vec3i8 dir = DIRS[d];
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

void Graphics::renderHud(const Player &player) {
	//glUseProgram(0);
	switchToOrthogonal();
	glDisable(GL_LIGHTING);
	glDisable(GL_FOG);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_TEXTURE_2D);
	glLoadIdentity();

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

	glEnable(GL_TEXTURE_2D);
	texManager.bind(0);
	texManager.bind(player.getBlock());
	glPushMatrix();
	glTranslatef(-drawWidth * 0.48, -drawHeight * 0.48, 0);
	float d = (width < height ? width : height) * 0.05;
	glColor4f(1, 1, 1, 1);
	glBegin(GL_QUADS);
		glTexCoord2f(0, 1); glVertex2f(0, 0);
		glTexCoord2f(1, 1); glVertex2f(d, 0);
		glTexCoord2f(1, 0); glVertex2f(d, d);
		glTexCoord2f(0, 0); glVertex2f(0, d);
	glEnd();
	glPopMatrix();
}

void Graphics::renderDebugInfo(const Player &player) {
	vec3i64 playerPos = player.getPos();
	vec3d playerVel = player.getVel();
	uint32 windowFlags = SDL_GetWindowFlags(window);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);

	glDisable(GL_TEXTURE_2D);
	glPushMatrix();
	glColor3f(1.0f, 1.0f, 1.0f);
	glTranslatef(-drawWidth / 2 + 3, drawHeight / 2, 0);
	char buffer[1024];
	#define RENDER_LINE(args...) sprintf(buffer, args);\
			glTranslatef(0, -16, 0);\
			font->Render(buffer)

	RENDER_LINE("fps: %d", fpsSum);
	RENDER_LINE("new faces: %d", newFaces);
	RENDER_LINE("faces: %d", faces);
	RENDER_LINE("x: %ld (%ld)", playerPos[0], playerPos[0] / RESOLUTION);
	RENDER_LINE("y: %ld (%ld)", playerPos[1], playerPos[1] / RESOLUTION);
	RENDER_LINE("z: %ld (%ld)", playerPos[2],
			(playerPos[2] - Player::EYE_HEIGHT - 1) / RESOLUTION);
	RENDER_LINE("yaw:   %6.1f", player.getYaw());
	RENDER_LINE("pitch: %6.1f", player.getPitch());
	RENDER_LINE("xvel: %8.1f", playerVel[0]);
	RENDER_LINE("yvel: %8.1f", playerVel[1]);
	RENDER_LINE("zvel: %8.1f", playerVel[2]);
	RENDER_LINE("chunks loaded: %lu", world->getNumChunks());
	RENDER_LINE("block: %d", player.getBlock());
	if ((SDL_WINDOW_FULLSCREEN & windowFlags) > 0)
		glColor3f(1.0f, 0.0f, 0.0f);
	else
		glColor3f(1.0f, 1.0f, 1.0f);
	RENDER_LINE("SDL_WINDOW_FULLSCREEN");
	if ((SDL_WINDOW_FULLSCREEN_DESKTOP & windowFlags) > 0)
		glColor3f(1.0f, 0.0f, 0.0f);
	else
		glColor3f(1.0f, 1.0f, 1.0f);
	RENDER_LINE("SDL_WINDOW_FULLSCREEN_DESKTOP");
	if ((SDL_WINDOW_OPENGL & windowFlags) > 0)
		glColor3f(1.0f, 0.0f, 0.0f);
	else
		glColor3f(1.0f, 1.0f, 1.0f);
	RENDER_LINE("SDL_WINDOW_OPENGL");
	if ((SDL_WINDOW_SHOWN & windowFlags) > 0)
		glColor3f(1.0f, 0.0f, 0.0f);
	else
		glColor3f(1.0f, 1.0f, 1.0f);
	RENDER_LINE("SDL_WINDOW_SHOWN");
	if ((SDL_WINDOW_HIDDEN & windowFlags) > 0)
		glColor3f(1.0f, 0.0f, 0.0f);
	else
		glColor3f(1.0f, 1.0f, 1.0f);
	RENDER_LINE("SDL_WINDOW_HIDDEN");
	if ((SDL_WINDOW_BORDERLESS & windowFlags) > 0)
		glColor3f(1.0f, 0.0f, 0.0f);
	else
		glColor3f(1.0f, 1.0f, 1.0f);
	RENDER_LINE("SDL_WINDOW_BORDERLESS");
	if ((SDL_WINDOW_RESIZABLE & windowFlags) > 0)
		glColor3f(1.0f, 0.0f, 0.0f);
	else
		glColor3f(1.0f, 1.0f, 1.0f);
	RENDER_LINE("SDL_WINDOW_RESIZABLE");
	if ((SDL_WINDOW_MINIMIZED & windowFlags) > 0)
		glColor3f(1.0f, 0.0f, 0.0f);
	else
		glColor3f(1.0f, 1.0f, 1.0f);
	RENDER_LINE("SDL_WINDOW_MINIMIZED");
	if ((SDL_WINDOW_MAXIMIZED & windowFlags) > 0)
		glColor3f(1.0f, 0.0f, 0.0f);
	else
		glColor3f(1.0f, 1.0f, 1.0f);
	RENDER_LINE("SDL_WINDOW_MAXIMIZED");
	if ((SDL_WINDOW_INPUT_GRABBED & windowFlags) > 0)
		glColor3f(1.0f, 0.0f, 0.0f);
	else
		glColor3f(1.0f, 1.0f, 1.0f);
	RENDER_LINE("SDL_WINDOW_INPUT_GRABBED");
	if ((SDL_WINDOW_INPUT_FOCUS & windowFlags) > 0)
		glColor3f(1.0f, 0.0f, 0.0f);
	else
		glColor3f(1.0f, 1.0f, 1.0f);
	RENDER_LINE("SDL_WINDOW_INPUT_FOCUS");
	if ((SDL_WINDOW_MOUSE_FOCUS & windowFlags) > 0)
		glColor3f(1.0f, 0.0f, 0.0f);
	else
		glColor3f(1.0f, 1.0f, 1.0f);
	RENDER_LINE("SDL_WINDOW_MOUSE_FOCUS");
	if ((SDL_WINDOW_FOREIGN & windowFlags) > 0)
		glColor3f(1.0f, 0.0f, 0.0f);
	else
		glColor3f(1.0f, 1.0f, 1.0f);
	RENDER_LINE("SDL_WINDOW_FOREIGN");
	if ((SDL_WINDOW_ALLOW_HIGHDPI & windowFlags) > 0)
		glColor3f(1.0f, 0.0f, 0.0f);
	else
		glColor3f(1.0f, 1.0f, 1.0f);
	RENDER_LINE("SDL_WINDOW_ALLOW_HIGHDPI");
//	if ((SDL_WINDOW_MOUSE_CAPTURE & windowFlags) > 0)
//		glColor3f(1.0f, 0.0f, 0.0f);
//	else
//		glColor3f(1.0f, 1.0f, 1.0f);
//	RENDER_LINE("SDL_WINDOW_MOUSE_CAPTURE");

	glPopMatrix();

	if (stopwatch)
		renderPerformance();

}

void Graphics::renderPerformance() {
	const char *rel_names[] = {
		"CLR",
		"NDL",
		"DLC",
		"CHL",
		"CHR",
		"PLA",
		"HUD",
		"FLP",
		"TIC",
		"NET",
		"SYN",
		"FSH",
		"ALL"
	};

	vec<float, 3> rel_colors[] {
		{0.6f, 0.6f, 1.0f},
		{0.0f, 0.0f, 0.8f},
		{0.6f, 0.0f, 0.8f},
		{0.0f, 0.6f, 0.8f},
		{0.4f, 0.4f, 0.8f},
		{0.7f, 0.7f, 0.0f},
		{0.8f, 0.8f, 0.3f},
		{0.0f, 0.8f, 0.0f},
		{0.0f, 0.4f, 0.0f},
		{0.7f, 0.1f, 0.7f},
		{0.7f, 0.7f, 0.4f},
		{0.2f, 0.6f, 0.6f},
		{0.8f, 0.0f, 0.0f},
	};

	float rel = 0.0;
	float cum_rels[CLOCK_ID_NUM + 1];
	cum_rels[0] = 0;
	float center_positions[CLOCK_ID_NUM];

	glPushMatrix();
	glTranslatef(+drawWidth / 2.0, -drawHeight / 2, 0);
	glScalef(10.0, drawHeight, 1.0);
	glTranslatef(-1, 0, 0);
	glBegin(GL_QUADS);
	for (uint i = 0; i < CLOCK_ID_NUM; ++i) {
		glColor3f(rel_colors[i][0], rel_colors[i][1], rel_colors[i][2]);
		glVertex2f(0, rel);
		glVertex2f(1, rel);
		rel += stopwatch->getRel(i);
		cum_rels[i + 1] = rel;
		center_positions[i] = (cum_rels[i] + cum_rels[i + 1]) / 2.0;
		glVertex2f(1, rel);
		glVertex2f(0, rel);
	}
	glEnd();
	glPopMatrix();

	static const float REL_THRESHOLD = 0.001;
	uint labeled_ids[CLOCK_ID_NUM];
	int num_displayed_labels = 0;
	for (int i = 0; i < CLOCK_ID_NUM; ++i) {
		if (stopwatch->getRel(i) > REL_THRESHOLD)
			labeled_ids[num_displayed_labels++] = i;
	}

	float used_positions[CLOCK_ID_NUM + 2];
	used_positions[0] = 0.0;
	for (int i = 0; i < num_displayed_labels; ++i) {
		int id = labeled_ids[i];
		used_positions[i + 1] = center_positions[id];
	}
	used_positions[num_displayed_labels + 1] = 1.0;

	for (int iteration = 0; iteration < 3; ++iteration)
	for (int i = 1; i < num_displayed_labels + 1; ++i) {
		float d1 = used_positions[i] - used_positions[i - 1];
		float d2 = used_positions[i + 1] - used_positions[i];
		float diff = 2e-4 * (1.0 / d1 - 1.0 / d2);
		used_positions[i] += clamp((double) diff, -0.02, 0.02);
	}

	glPushMatrix();
	glTranslatef(+drawWidth / 2.0, -drawHeight / 2, 0);
	glTranslatef(-15, 0, 0);
	glRotatef(90.0, 0.0, 0.0, 1.0);
	for (int i = 0; i < num_displayed_labels; ++i) {
		int id = labeled_ids[i];
		glPushMatrix();
		glTranslatef(used_positions[i + 1] * drawHeight - 14, 0, 0);
		char buffer[1024];
		sprintf(buffer, "%s", rel_names[id]);
		auto color = rel_colors[id];
		glColor3f(color[0], color[1], color[2]);
		font->Render(buffer);
		glPopMatrix();
	}
	glPopMatrix();
}

bool Graphics::inFrustum(vec3i64 cc, vec3i64 pos, vec3d lookDir) {
	using namespace vec_auto_cast;
	double chunkDia = sqrt(3) * Chunk::WIDTH * RESOLUTION;
	vec3d cp = (cc * Chunk::WIDTH * RESOLUTION - pos).cast<double>();
	double chunkLookDist = lookDir * cp + chunkDia;
	if (chunkLookDist < 0)
		return false;
	vec3d orthoChunkPos = cp - lookDir * chunkLookDist;
	double orthoChunkDist = std::max(0.0, orthoChunkPos.norm() - chunkDia);
	return atan(orthoChunkDist / chunkLookDist) <= maxFOV / 2;
}
