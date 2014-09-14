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
#include "logging.hpp"

Graphics::Graphics(World *world, int localClientID, Stopwatch *stopwatch)
		: stopwatch(stopwatch) {
	LOG(INFO) << "Constructing Graphics";
	this->world = world;
	this->localClientID = localClientID;

	LOG(INFO) << "Initializing SDL";
	auto failure = SDL_Init(SDL_INIT_VIDEO);
	LOG_IF(failure, FATAL) << SDL_GetError();

	LOG(INFO) << "Creating window";
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	window = SDL_CreateWindow(
		"3dgame",
		0, 0,
		START_WIDTH, START_HEIGHT,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
	);
	LOG_IF(!window, FATAL) << SDL_GetError();

	LOG(INFO) << "Creating Open GL Context";
	glContext = SDL_GL_CreateContext(window);
	LOG_IF(!glContext, FATAL) << SDL_GetError();
	failure = SDL_GL_SetSwapInterval(0);
	LOG_IF(failure, WARNING) << SDL_GetError();

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
	LOG(INFO) << "Destroying Graphics";

	int length = VIEW_RANGE * 2 + 1;
	glDeleteLists(firstDL, length * length * length);
	delete dlChunks;
	delete font;

	glDeleteProgram(program);
	glDeleteProgram(program_postproc);

	SDL_GL_DeleteContext(glContext);
	SDL_Quit();
}

void Graphics::tick() {
	render();

	stopwatch->start(CLOCK_FLP);
	SDL_GL_SwapWindow(window);
	stopwatch->stop(CLOCK_FLP);
	frameCounter++;
	int64 time = getMicroTimeSince(startTimePoint);
	if (time - lastFPSUpdate > 1000000) {
		lastFPS = frameCounter;
		lastFPSUpdate += 1000000;
		frameCounter = 0;
		stopwatch->stop(CLOCK_ALL);
		stopwatch->save();
		stopwatch->start(CLOCK_ALL);
	}
}

void Graphics::resize(int width, int height) {
	LOG(INFO) << "Resize to " << width << "x" << height;
	this->width = width;
	this->height = height;
	glViewport(0, 0, width, height);
	makePerspective();
	makeOrthogonal();
	calcDrawArea();

	// update framebuffer object
	if (msaa || fxaa) {
		destroyFBO();
		createFBO();
	}

	glUseProgram(program_postproc);
	GLuint pixel_size_loc = glGetUniformLocation(program_postproc, "pixel_size");
	logOpenGLError();
	glUniform2f(pixel_size_loc, 1.0 / width, 1.0 / height);
	logOpenGLError();
}

void Graphics::grab() {
	SDL_SetWindowGrab(window, (SDL_bool) !SDL_GetWindowGrab(window));
	SDL_SetRelativeMouseMode((SDL_bool) !SDL_GetRelativeMouseMode());
}

bool Graphics::isGrabbed() {
	return SDL_GetWindowGrab(window);
}

void Graphics::initGL() {
	LOG(INFO) << "Initializing GLEW";
	GLenum glew_error = glewInit();
	LOG_IF(glew_error != GLEW_OK, ERROR) << glewGetErrorString(glew_error);
	if (GLEW_VERSION_4_0) {
		LOG(INFO) << "OpenGL 4.0 available";
	} else if (GLEW_VERSION_3_2) {
		LOG(INFO) << "OpenGL 3.2 available";
	} else if (GLEW_VERSION_2_1) {
		LOG(INFO) << "OpenGL 2.1 available";
	} else if (GLEW_VERSION_1_2) {
		LOG(INFO) << "OpenGL 1.2 available";
	} else {
		LOG(INFO) << "No known OpenGL Version found";
	}

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1);
	glDepthFunc(GL_LEQUAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glMatrixMode(GL_MODELVIEW);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_CULL_FACE);


	// light
	LOG(INFO) << "Initializing light";
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
	LOG(INFO) << "Initializing textures";
	IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF);
	SDL_Surface *blockImage = IMG_Load("img/block.png");
	LOG_IF(!blockImage, ERROR)  << "Could not open 'block.png'";

	glGenTextures(1, &blockTexture);
	glBindTexture(GL_TEXTURE_2D, blockTexture);
	logOpenGLError();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, blockImage->pixels);
	//gluBuild2DMipmaps(GL_TEXTURE_2D, 4, 128, 128, GL_RGBA, GL_UNSIGNED_BYTE, blockImage->pixels);
	SDL_FreeSurface(blockImage);
	logOpenGLError();

	glGenTextures(1, &noTexture);
	glBindTexture(GL_TEXTURE_2D, noTexture);
	const uint8 white[4] = {255, 255, 255, 255};
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &white);

	// shader
	program = loadProgram(
			"shaders/vertex_shader.vert", "shaders/fragment_shader.frag");
	program_postproc = loadProgram(
			"shaders/postproc.vert", "shaders/postproc.frag");

	// fog
	glUseProgram(program);

	GLuint fog_color_loc = glGetUniformLocation(program, "fog_color");
	logOpenGLError();
	glUniform3f(fog_color_loc, 0.5, 0.5, 0.5);
	logOpenGLError();

	GLuint fog_width_loc = glGetUniformLocation(program, "fog_width");
	logOpenGLError();
	glUniform1f(fog_width_loc, 192.0);
	logOpenGLError();

	// fxaa
	glUseProgram(program_postproc);

	GLuint fxaa_span_max_loc = glGetUniformLocation(program_postproc, "fxaa_span_max");
	logOpenGLError();
	glUniform1f(fxaa_span_max_loc, 8.0);
	logOpenGLError();

	GLuint fxaa_reduce_mul_loc = glGetUniformLocation(program_postproc, "fxaa_reduce_mul");
	logOpenGLError();
	glUniform1f(fxaa_reduce_mul_loc, 1.0/8.0);
	logOpenGLError();

	GLuint fxaa_reduce_min_loc = glGetUniformLocation(program_postproc, "fxaa_reduce_min");
	logOpenGLError();
	glUniform1f(fxaa_reduce_min_loc, 1.0/128.0);
	logOpenGLError();

	// enable multisampling
	//createFBO();
}

GLuint Graphics::loadShader(const char *path, GLenum type) {
	LOG(INFO) << "Loading shader source '" << path << "'";
	std::ifstream f(path);
	LOG_IF(!f.good(), ERROR) << "Could not open '" << path << "'";
	std::stringstream ss;
	ss << f.rdbuf();
	std::string s = ss.str();
	const char *source = s.c_str();
	f.close();
	LOG(INFO) << "Compiling '" << path << "'";
	GLenum shader = glCreateShaderObjectARB(type);
	logOpenGLError();
	glShaderSourceARB(shader, 1, &source, NULL);
	logOpenGLError();
	glCompileShaderARB(shader);
	logOpenGLError();

	GLint logSize = 0;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logSize);
	logOpenGLError();

	if(logSize > 0) {
		GLsizei length;
		GLchar infoLog[logSize];
		glGetShaderInfoLog(shader, logSize, &length, infoLog);
		logOpenGLError();
		if (length > 0)
			LOG(WARNING) << "Shader log: " << (char *) infoLog;
	}

	return shader;
}

GLuint Graphics::loadProgram(const char *vert_src, const char *frag_src) {
	GLuint vertex_shader = loadShader(vert_src, GL_VERTEX_SHADER_ARB);
	GLuint fragment_shader = loadShader(frag_src, GL_FRAGMENT_SHADER_ARB);

	LOG(INFO) << "Assembling shader program";
	GLuint program = glCreateProgramObjectARB();
	logOpenGLError();
	glAttachObjectARB(program, vertex_shader);
	logOpenGLError();
	glAttachObjectARB(program, fragment_shader);
	logOpenGLError();
	glLinkProgramARB(program);
	logOpenGLError();
	glValidateProgram(program);

	GLint logSize = 0;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logSize);
	logOpenGLError();

	if(logSize > 0) {
		GLsizei length;
		GLchar infoLog[logSize];
		glGetProgramInfoLog(program, logSize, &length, infoLog);
		logOpenGLError();
		if (length > 0)
			LOG(WARNING) << "Shader program log: " << (char *) infoLog;
	}

	int validate_ok;
	glGetProgramiv(program, GL_VALIDATE_STATUS, &validate_ok);
	if (!validate_ok) {
		LOG(ERROR) << "Shader program failed validation";
	}
	logOpenGLError();

	return program;
}

void Graphics::enableMSAA(uint samples) {
	destroyFBO();
	msaa = samples;
	fxaa = false;
	createFBO();
}

void Graphics::disableMSAA() {
	destroyFBO();
	msaa = 0;
	fxaa = false;
}

void Graphics::enableFXAA() {
	destroyFBO();
	msaa = 0;
	fxaa = true;
	createFBO();
}

void Graphics::disableFXAA() {
	destroyFBO();
	msaa = 0;
	fxaa = false;
}

void Graphics::createFBO() {
	LOG(INFO) << "Creating framebuffer with MSAA=" << msaa
			<< " and fxaa " << (fxaa ? "enabled" : "disabled");
	bool needs_render_to_texture = false;
	if (fxaa)
		needs_render_to_texture = true;

	if (needs_render_to_texture) {
		glActiveTexture(GL_TEXTURE0);
		logOpenGLError();
		glGenTextures(1, &fbo_texture);
		glBindTexture(GL_TEXTURE_2D, fbo_texture);
		logOpenGLError();
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		logOpenGLError();
		glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		logOpenGLError();
		glBindTexture(GL_TEXTURE_2D, 0);
		logOpenGLError();
	} else {
		glGenRenderbuffers(1, &fbo_color_buffer);
		glBindRenderbuffer(GL_RENDERBUFFER, fbo_color_buffer);
		logOpenGLError();
		glRenderbufferStorageMultisample(
				GL_RENDERBUFFER, msaa, GL_RGB, width, height
		);
		logOpenGLError();
	}

	// Depth buffer
	glGenRenderbuffers(1, &fbo_depth_buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, fbo_depth_buffer);
	logOpenGLError();
	if (msaa) {
		glRenderbufferStorageMultisample(
				GL_RENDERBUFFER, msaa, GL_DEPTH_COMPONENT24, width, height
		);
	} else {
		glRenderbufferStorage(
				GL_RENDERBUFFER, GL_DEPTH_COMPONENT24,
				width, height
		);
	}
	logOpenGLError();

	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	logOpenGLError();
	if (needs_render_to_texture) {
		glFramebufferTexture2D(
				GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
				GL_TEXTURE_2D, fbo_texture, 0
		);
	} else {
		glFramebufferRenderbuffer(
				GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
				GL_RENDERBUFFER, fbo_color_buffer
		);
	}
	logOpenGLError();
	glFramebufferRenderbuffer(
			GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
			GL_RENDERBUFFER, fbo_depth_buffer
	);
	logOpenGLError();

	GLuint status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		const char *msg = 0;
		switch (status) {
		case GL_FRAMEBUFFER_UNDEFINED:
			msg = "framebuffer undefined"; break;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			msg = "incomplete attachment"; break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			msg = "missind attachment"; break;
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
			msg = "incomplete draw buffer"; break;
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
			msg = "incomplete read buffer"; break;
		case GL_FRAMEBUFFER_UNSUPPORTED:
			msg = "unsupported"; break;
		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
			msg = "incomplete multisampling"; break;
		case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
			msg = "incomplete layer targets"; break;
		}
		LOG(ERROR) << "Framebuffer creation failed: " << msg;
	}
	logOpenGLError();
}

void Graphics::destroyFBO() {
	glDeleteTextures(1, &fbo_texture);
	glDeleteRenderbuffers(1, &fbo_color_buffer);
	glDeleteRenderbuffers(1, &fbo_depth_buffer);
	glDeleteFramebuffers(1, &fbo);
	fbo = fbo_color_buffer = fbo_depth_buffer = fbo_texture = 0;
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

	glUseProgram(program);
	GLuint fog_end_loc = glGetUniformLocation(program, "fog_end");
	glUniform1f(fog_end_loc, zFar - 0.1f);
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

	logOpenGLError();
	if (msaa || fxaa) {
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
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	logOpenGLError();
	stopwatch->stop(CLOCK_CLR);

	renderScene(localPlayer);
	logOpenGLError();

	if (msaa || fxaa) {
		// copy framebuffer to screen, blend multisampling on the way
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
		glDrawBuffer(GL_BACK);
		if (fxaa) {
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();

			glDisable(GL_DEPTH_TEST);
			glDisable(GL_LIGHTING);
			glDisable(GL_FOG);
			glEnable(GL_TEXTURE_2D);
			glColor3f(1.0f, 1.0f, 1.0f);

			glBindTexture(GL_TEXTURE_2D, fbo_texture);
			glUseProgram(program_postproc);
			glBegin(GL_QUADS);
				glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, -1.0f);
				glTexCoord2f(1.0f, 0.0f); glVertex2f(+1.0f, -1.0f);
				glTexCoord2f(1.0f, 1.0f); glVertex2f(+1.0f, +1.0f);
				glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, +1.0f);
			glEnd();
		} else {
			glBlitFramebuffer(
					0, 0, width, height,
					0, 0, width, height,
					GL_COLOR_BUFFER_BIT,
					GL_NEAREST
			);
		}
		logOpenGLError();
	}

	stopwatch->start(CLOCK_HUD);
	renderHud(localPlayer);
	renderDebugInfo(localPlayer);
	stopwatch->stop(CLOCK_HUD);
}

void Graphics::renderScene(const Player &player) {
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glUseProgram(program);
	logOpenGLError();
	switchToPerspective();
	logOpenGLError();
	glEnable(GL_LIGHTING);
	//glEnable(GL_FOG);
	glEnable(GL_DEPTH_TEST);
	glLoadIdentity();
	// player yaw
	glRotated(-player.getPitch(), 1, 0, 0);
	// player pitch
	glRotated(-player.getYaw(), 0, 1, 0);
	// tilt coordinate system so z points up
	glRotated(-90, 1, 0, 0);
	// tilt coordinate system so we look into x direction
	glRotated(90, 0, 0, 1);
	//player position
	vec3i64 playerPos = player.getPos();
	glTranslated(
		-playerPos[0] / (double) RESOLUTION,
		-playerPos[1] / (double) RESOLUTION,
		-playerPos[2] / (double) RESOLUTION
	);

	// now we are in world coordinates

	// place light
	glLightfv(GL_LIGHT0, GL_POSITION, sunLightPosition);
	logOpenGLError();

	// render chunk
	stopwatch->start(CLOCK_CHR);
	renderChunks();
	logOpenGLError();
	stopwatch->stop(CLOCK_CHR);

	// render players
	stopwatch->start(CLOCK_PLA);
	renderPlayers();
	logOpenGLError();
	stopwatch->stop(CLOCK_PLA);
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
	stopwatch->stop(CLOCK_NDL);

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
			stopwatch->stop(CLOCK_DLC);
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
	glUseProgram(0);
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

	renderDebugInfo(player);
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
	RENDER_LINE("MSAA: %u", msaa);
	RENDER_LINE("FXAA: %s", fxaa ? "on" : "off");

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
