#include "graphics.hpp"
#include "vmath.hpp"
#include "util.hpp"
#include "constants.hpp"
#include <cmath>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <string>
#include <fstream>
#include <sstream>

Graphics::Graphics(World *world, int localClientID, Stopwatch *stopwatch)
		: stopwatch(stopwatch) {
	this->world = world;
	this->localClientID = localClientID;

	SDL_Init(SDL_INIT_VIDEO);

	window = SDL_CreateWindow(
		"3dgame",
		0, 0,
		START_WIDTH, START_HEIGHT,
		SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE
	);
	glContext = SDL_GL_CreateContext(window);

	initGL();
	int length = VIEW_RANGE * 2 + 1;
	firstDL = glGenLists(length * length * length);
	dlChunks = new vec3i64[length * length * length];

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
	int length = VIEW_RANGE * 2 + 1;
	glDeleteLists(firstDL, length * length * length);
	delete dlChunks;
    delete font;
	SDL_GL_DeleteContext(glContext);
	SDL_Quit();
}

void Graphics::tick() {
	render();
	stopwatch->start(CLOCK_FLP);
	SDL_GL_SwapWindow(window);
	stopwatch->stop();
	frameCounter++;
	int64 time = getMicroTimeSince(startTimePoint);
	if (time - lastFPSUpdate > 1000000) {
		lastFPS = frameCounter;
		lastFPSUpdate += 1000000;
		frameCounter = 0;
		stopwatch->stopAndSave();
		stopwatch->start(CLOCK_ALL);
	}
}

void Graphics::resize(int width, int height) {
	this->width = width;
	this->height = height;
	glViewport(0, 0, width, height);
	makePerspective();
	makeOrthogonal();
	calcDrawArea();

	// update framebuffer object
	if (multisampling)
		enableMultisampling(multisampling);
}

void Graphics::grab() {
	SDL_SetWindowGrab(window, (SDL_bool) !SDL_GetWindowGrab(window));
	SDL_SetRelativeMouseMode((SDL_bool) !SDL_GetRelativeMouseMode());
}

bool Graphics::isGrabbed() {
	return SDL_GetWindowGrab(window);
}

void Graphics::initGL() {
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1);
	glDepthFunc(GL_LEQUAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glMatrixMode(GL_MODELVIEW);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_CULL_FACE);


	// light
	float matAmbient[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	float matDiffuse[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	float matSpecular[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	float sunAmbient[4] = {0.5f, 0.5f, 0.5f, 1.0f};
	float sunDiffuse[4] = {0.5f, 0.5f, 0.45f, 1.0f};
	float sunSpecular[4] = {0.8f, 0.8f, 0.7f, 1.0f};
	//float playerLight[4] = {4.0f, 4.0f, 2.4f, 1.0f};
	//float lModelAmbient[4] = {0.6f, 0.6f, 0.6f, 1.0f};

	//float playerLightPos[4] = {0.0f, 0.0f, -0.4f, 1.0f};

	//glEnable(GL_LIGHTING); // enables lighting
	//glShadeModel(GL_SMOOTH);
	glMaterialfv(GL_FRONT, GL_SPECULAR, matSpecular); // sets specular material color
	glMaterialfv(GL_FRONT, GL_DIFFUSE, matDiffuse); // sets diffuse material color
	glMaterialfv(GL_FRONT, GL_AMBIENT, matAmbient); // sets ambient material color
	glMaterialf(GL_FRONT, GL_SHININESS, 1.0f); // sets shininess

	//glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lModelAmbient); // global ambient light

	//glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0.01f);
	//glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.001f);
	//glLight(GL_LIGHT0, GL_SPECULAR, whiteLight); // sets specular light to white
	glLightfv(GL_LIGHT0, GL_AMBIENT, sunAmbient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, sunDiffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, sunSpecular);
	//glEnable(GL_LIGHT0);

	/*glLightfv(GL_LIGHT1, GL_DIFFUSE, playerLight);
	glLightfv(GL_LIGHT1, GL_POSITION, playerLightPos);
	glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, 0.0f);
	glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, 0.0f);
	glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 1.f);*/
	//glEnable(GL_LIGHT1);

	//glEnable(GL_COLOR_MATERIAL); // enables opengl to use glColor3f to define material color
	//glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE); // tell opengl glColor3f effects the ambient and diffuse properties of material


	// textures
	IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF);
	SDL_Surface *blockImage = IMG_Load("img/block.png");

	glGenTextures(1, &blockTexture);
	glBindTexture(GL_TEXTURE_2D, blockTexture);
	gluBuild2DMipmaps(GL_TEXTURE_2D, 4, 128, 128, GL_RGBA, GL_UNSIGNED_BYTE, blockImage->pixels);

	SDL_FreeSurface(blockImage);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glGenTextures(1, &noTexture);
	glBindTexture(GL_TEXTURE_2D, noTexture);
	const uint8 white[4] = {255, 255, 255, 255};
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &white);

	// shader
	makeProgram();

	// enable multisampling
	enableMultisampling();

	// fog
	glUniform3f(glGetUniformLocation(program, "fog_color"), 0.5f, 0.5f, 0.5f);
	glUniform1f(glGetUniformLocation(program, "fog_width"), 192.0f);
}

void Graphics::makeProgram() {
	const char *frag_source;
	const char *vert_source;

	std::stringstream ss1;
	std::ifstream f1("shaders/fragment_shader.frag");
	ss1 << f1.rdbuf();
	std::string s1 = ss1.str();
	frag_source = s1.c_str();
	f1.close();

	std::stringstream ss2;
	std::ifstream f2("shaders/vertex_shader.vert");
	ss2 << f2.rdbuf();
	std::string s2 = ss2.str();
	vert_source = s2.c_str();
	f2.close();

	glewInit();

	GLenum fragment_shader;
	GLenum vertex_shader;

	// Create Shader And Program Objects
	program = glCreateProgramObjectARB();
	fragment_shader = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	vertex_shader = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);

	// Load Shader Sources
	glShaderSourceARB(fragment_shader, 1, &frag_source, NULL);
	glShaderSourceARB(vertex_shader, 1, &vert_source, NULL);

	// Compile The Shaders
	glCompileShaderARB(fragment_shader);
	glCompileShaderARB(vertex_shader);

	{
		GLint logSize = 0;
		glGetProgramiv(fragment_shader, GL_INFO_LOG_LENGTH, &logSize);

		if(logSize > 0) {
			GLsizei length;
			GLchar infoLog[logSize];
			glGetProgramInfoLog(fragment_shader, logSize, &length, infoLog);
			if (length > 0)
				printf("%s\n", infoLog);
		}
	}

	{
		GLint logSize = 0;
		glGetProgramiv(vertex_shader, GL_INFO_LOG_LENGTH, &logSize);

		if(logSize > 0) {
			GLsizei length;
			GLchar infoLog[logSize];
			glGetProgramInfoLog(vertex_shader, logSize, &length, infoLog);
			if (length > 0)
				printf("%s\n", infoLog);
		}
	}

	// Attach The Shader Objects To The Program Object
	glAttachObjectARB(program, fragment_shader);
	glAttachObjectARB(program, vertex_shader);

	// Link The Program Object
	glLinkProgramARB(program);

	// Use The Program Object Instead Of Fixed Function OpenGL
	glUseProgramObjectARB(program);

	{
		GLint logSize = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logSize);

		if(logSize > 0) {
			GLsizei length;
			GLchar infoLog[logSize];
			glGetProgramInfoLog(program, logSize, &length, infoLog);
			if (length > 0)
				printf("%s\n", infoLog);
		}
	}
}

void Graphics::enableMultisampling(uint samples) {
	if (multisampling)
		disableMultisampling();

	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	// Color buffer
	glGenRenderbuffers(1, &fbo_color_buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, fbo_color_buffer);
	glRenderbufferStorageMultisample(
			GL_RENDERBUFFER, samples, GL_RGB, width, height
	);
	glFramebufferRenderbuffer(
			GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_RENDERBUFFER, fbo_color_buffer
	);

	// Depth buffer
	glGenRenderbuffers(1, &fbo_depth_buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, fbo_depth_buffer);
	glRenderbufferStorageMultisample(
			GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT24, width, height
	);
	glFramebufferRenderbuffer(
			GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
			GL_RENDERBUFFER, fbo_depth_buffer
	);

	multisampling = samples;
}

void Graphics::disableMultisampling() {
	glDeleteFramebuffers(1, &fbo);
	glDeleteFramebuffers(1, &fbo_color_buffer);
	glDeleteFramebuffers(1, &fbo_depth_buffer);
	fbo = fbo_color_buffer = fbo_depth_buffer = 0;
	multisampling = 0;
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
	float zFar = Chunk::WIDTH * VIEW_RANGE - 2.5;
	gluPerspective((float) (angle * 360.0 / TAU),
			(float) currentRatio, 0.1f, zFar);
	glGetDoublev(GL_PROJECTION_MATRIX, perspectiveMatrix);
	glMatrixMode(GL_MODELVIEW);

	glUniform1f(glGetUniformLocation(program, "fog_end"), zFar - 0.1f);
}

void Graphics::makeOrthogonal() {
	double normalRatio = START_WIDTH / (double) START_HEIGHT;
	double currentRatio = width / (double) height;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if (currentRatio > normalRatio)
		glOrtho(-START_WIDTH / 2.0, START_WIDTH / 2.0, -START_WIDTH
				/ currentRatio / 2.0, START_WIDTH / currentRatio / 2.0, 1,
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

	if (multisampling) {
		// render to the fbo and not the screen
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
	}

	stopwatch->start(CLOCK_CLR);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	stopwatch->stop();

	glUseProgram(program);
	switchToPerspective();
	glEnable(GL_LIGHTING);
	//glEnable(GL_FOG);
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
	glLightfv(GL_LIGHT0, GL_POSITION, sunLightPosition);

	// render chunk
	stopwatch->start(CLOCK_CHR);
	renderChunks();
	stopwatch->stop();

	// render players
	stopwatch->start(CLOCK_PLA);
	renderPlayers();
	stopwatch->stop();

	glUseProgram(0);
	switchToOrthogonal();
	glDisable(GL_LIGHTING);
	glDisable(GL_FOG);
	glDisable(GL_DEPTH_TEST);
	glLoadIdentity();

	stopwatch->start(CLOCK_HUD);
	renderHud(localPlayer);
	renderDebugInfo(localPlayer);
	stopwatch->stop();

	if (multisampling) {
		// copy framebuffer to screen, blend multisampling on the way
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
		glDrawBuffer(GL_BACK);
		glBlitFramebuffer(
				0, 0, width, height,
				0, 0, width, height,
				GL_COLOR_BUFFER_BIT,
				GL_NEAREST
		);
	}
}

void Graphics::renderChunks() {
	using namespace vec_auto_cast;

	Player &localPlayer = world->getPlayer(localClientID);
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

	newQuads = 0;
	int length = VIEW_RANGE * 2 + 1;

	stopwatch->start(CLOCK_NDL);
	vec3i64 ccc;
	while (newQuads < MAX_NEW_QUADS && world->popChangedChunk(&ccc)) {
		Chunk *chunk = world->getChunk(ccc);
		if(chunk && chunk->pollChanged()) {
			uint index = ((((ccc[2] % length) + length) % length) * length
					+ (((ccc[1] % length) + length) % length)) * length
					+ (((ccc[0] % length) + length) % length);
			GLuint lid = firstDL + index;
			glNewList(lid, GL_COMPILE);
			renderChunk(*chunk, false, vec3ui8(0, 0, 0), 0);
			glEndList();
			dlChunks[index] = ccc;
		}
	}
	stopwatch->stop();

	uint maxChunks = length * length * length;
	uint renderedChunks = 0;
	for (uint i = 0; i < LOADING_ORDER.size() && renderedChunks < maxChunks; i++) {
		vec3i8 cd = LOADING_ORDER[i];
		if (cd.maxAbs() > VIEW_RANGE)
			continue;
		renderedChunks++;

		vec3i64 cc = pc + cd;

		uint index = ((((cc[2] % length) + length) % length) * length
				+ (((cc[1] % length) + length) % length)) * length
				+ (((cc[0] % length) + length) % length);
		GLuint lid = firstDL + index;

		if (lid != 0
				&& inFrustum(cc, localPlayer.getPos(), lookDir)
				&& (!target || tcc != cc)
				&& dlChunks[index] == cc) {
			stopwatch->start(CLOCK_DLC);
			glCallList(lid);
			stopwatch->stop();
		} else if (target && tcc == cc)
			renderChunk(*world->getChunk(cc), true, ticc, td);
	}
}

void Graphics::renderChunk(const Chunk &c, bool targeted, vec3ui8 ticc, int td) {
	using namespace vec_auto_cast;
	//render blocks
	glBindTexture(GL_TEXTURE_2D, blockTexture);
	glBegin(GL_QUADS);

	const Chunk::FaceSet &faceSet = c.getFaces();
	for (Face f : faceSet) {
		newQuads++;

		vec3d color = {1, 1, 1};
		if (targeted && f.block == ticc && f.dir == td)
			color = {0.8, 0.2, 0.2};

		glNormal3d(DIRS[f.dir][0], DIRS[f.dir][1], DIRS[f.dir][2]);
		for (int j = 0; j < 4; j++) {
			glTexCoord2d(QUAD_CYCLE_2D[j][0], QUAD_CYCLE_2D[j][1]);
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
			vec3d vertex = (c.cc * Chunk::WIDTH
					+ f.block + QUAD_CYCLES_3D[f.dir][j]).cast<double>();
			glVertex3d(vertex[0], vertex[1], vertex[2]);
		}
	}
	glEnd();
}

void Graphics::renderPlayers() {
	using namespace vec_auto_cast;
	glBindTexture(GL_TEXTURE_2D, noTexture);
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
}

void Graphics::renderDebugInfo(const Player &player) {
	vec3i64 playerPos = player.getPos();
	vec3d playerVel = player.getVel();

	glPushMatrix();
	glColor3f(1.0f, 1.0f, 1.0f);
	glTranslatef(-drawWidth / 2 + 3, drawHeight / 2, 0);
	char buffer[1024];
	#define RENDER_LINE(args...) sprintf(buffer, args);\
			glTranslatef(0, -16, 0);\
			font->Render(buffer)

	RENDER_LINE("fps: %d", lastFPS);
	RENDER_LINE("quads: %d", newQuads);
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
	RENDER_LINE("MSAA: %u", multisampling);

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
		used_positions[i] += std::min(std::max(-0.02, (double) diff), 0.02);
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
