#include "graphics.hpp"

#include "world.hpp"
#include "chunk.hpp"
#include "menu.hpp"
#include "stopwatch.hpp"

#include "util.hpp"
#include "constants.hpp"
#include "logging.hpp"

#include <cmath>

#include <SDL2/SDL_image.h>

Graphics::Graphics(
		World *world,
		Menu *menu,
		int localClientID,
		const GraphicsConf &conf,
		Stopwatch *stopwatch) :
		conf(conf),
		world(world),
		menu(menu),
		localClientID(localClientID),
		stopwatch(stopwatch) {
	LOG(DEBUG, "Constructing Graphics");

	LOG(DEBUG, "Initializing SDL");
	if (SDL_Init(SDL_INIT_VIDEO))
		LOG(FATAL, SDL_GetError());
	int img_init_flags = IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF;
	int img_init_result = IMG_Init(img_init_flags);
	if (!(img_init_flags & img_init_result & IMG_INIT_JPG)) {
		LOG(ERROR, "SDL_Image JPEG Plugin could not be initialized")
	}
	if (!(img_init_flags & img_init_result & IMG_INIT_PNG)) {
		LOG(ERROR, "SDL_Image PNG Plugin could not be initialized")
	}
	if (!(img_init_flags & img_init_result & IMG_INIT_TIF)) {
		LOG(ERROR, "SDL_Image TIFF Plugin could not be initialized")
	}

	LOG(DEBUG, "Creating window");
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	//SDL_GL_SetSwapInterval(0);
	SDL_SetHint("SDL_VIDEO_MINIMIZE_ON_FOCUS_LOSS", "0");
	window = SDL_CreateWindow(
		"3dgame",
		0, 0,
		conf.windowed_res[0], conf.windowed_res[1],
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
	);
	if (!window) LOG(FATAL, SDL_GetError());

	LOG(DEBUG, "Creating Open GL Context");
	glContext = SDL_GL_CreateContext(window);
	if (!glContext) LOG(FATAL, SDL_GetError());

	LOG(INFO, "OpenGL Version: " << glGetString(GL_VERSION));
	LOG(INFO, "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION));

	LOG(DEBUG, "Initializing GLEW");
	GLenum glew_error = glewInit();
	if (glew_error != GLEW_OK)
		LOG(FATAL, glewGetErrorString(glew_error));
	if (!GLEW_VERSION_2_0)
		LOG(FATAL, "OpenGL version 2.0 not available");
	if (GLEW_VERSION_3_0) {
		// TODO fancy stuff
	}

	resize(conf.windowed_res[0], conf.windowed_res[1]);

	makeMaxFOV();

	initGL();

	startTimePoint = high_resolution_clock::now();
}

Graphics::~Graphics() {
	LOG(DEBUG, "Destroying Graphics");

	int length = conf.render_distance * 2 + 1;
	glDeleteLists(firstDL, length * length * length);
	delete dlChunks;
	delete dlHasChunk;
	delete dlFaces;

	delete font;

//	glDeleteProgram(program);
//	glDeleteProgram(program_postproc);

	SDL_GL_DeleteContext(glContext);
	SDL_Quit();
}

void Graphics::initGL() {
	glClearDepth(1);
	glDepthFunc(GL_LEQUAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glMatrixMode(GL_MODELVIEW);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_CULL_FACE);

	// light
	LOG(DEBUG, "Initializing light");
	//float playerLight[4] = {4.0f, 4.0f, 2.4f, 1.0f};
	float lModelAmbient[4] = {0.0f, 0.0f, 0.0f, 1.0f};

	//float playerLightPos[4] = {0.0f, 0.0f, -0.4f, 1.0f};

	glEnable(GL_LIGHTING);
	glShadeModel(GL_SMOOTH);
	float matAmbient[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	float matDiffuse[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	float matSpecular[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	glMaterialfv(GL_FRONT, GL_SPECULAR, matSpecular);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, matDiffuse);
	glMaterialfv(GL_FRONT, GL_AMBIENT, matAmbient);
	//glMaterialf(GL_FRONT, GL_SHININESS, 1.0f);

	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lModelAmbient);

	// sun light
	glEnable(GL_LIGHT0);
	float sunAmbient[4] = {0.5f, 0.5f, 0.47f, 1.0f};
	float sunDiffuse[4] = {0.4f, 0.4f, 0.38f, 1.0f};
	float sunSpecular[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	glLightfv(GL_LIGHT0, GL_AMBIENT, sunAmbient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, sunDiffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, sunSpecular);

	/*glLightfv(GL_LIGHT1, GL_DIFFUSE, playerLight);
	glLightfv(GL_LIGHT1, GL_POSITION, playerLightPos);
	glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, 0.0f);
	glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, 0.0f);
	glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 1.f);*/
	//glEnable(GL_LIGHT1);

	glEnable(GL_COLOR_MATERIAL); // enables opengl to use glColor3f to define material color
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE); // tell opengl glColor3f effects the ambient and diffuse properties of material

	// font
	LOG(DEBUG, "Loading font");
	font = new FTGLTextureFont("res/DejaVuSansMono.ttf");
	if (font) {
		font->FaceSize(16);
		font->CharMap(ft_encoding_unicode);
	} else {
		LOG(ERROR, "Could not open 'res/DejaVuSansMono.ttf'");
	}

	// textures
	LOG(DEBUG, "Loading textures");
	//texManager.loadTexture("img/block.png", 1);
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
	texManager.loadTextures("img/textures_1.png", 16, 16, blocks);
	//texManager.loadTexture("img/textures_1.png", 34);

//	glGenTextures(1, &noTexture);
//	glBindTexture(GL_TEXTURE_2D, noTexture);
//	const uint8 white[4] = {255, 255, 255, 255};
//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &white);

	// shader
//	program = loadProgram(
//			"shaders/vertex_shader.vert", "shaders/fragment_shader.frag");
//	program_postproc = loadProgram(
//			"shaders/postproc.vert", "shaders/postproc.frag");

	// fog
	glEnable(GL_FOG);
	if (GLEW_NV_fog_distance)
		glFogi(GL_FOG_DISTANCE_MODE_NV, GL_EYE_RADIAL_NV);
	else
		LOG(INFO, "GL_NV_fog_distance not available, falling back to z fog");

	glFogfv(GL_FOG_COLOR, fogColor);
	glFogi(GL_FOG_MODE, GL_LINEAR);
	makeFog();

//	glUseProgram(program);
//
//	GLuint fog_color_loc = glGetUniformLocation(program, "fog_color");
//	logOpenGLError();
//	glUniform3f(fog_color_loc, 0.5, 0.5, 0.5);
//	logOpenGLError();
//
//	glUseProgram(program);
//	GLuint fog_end_loc = glGetUniformLocation(program, "fog_end");
//	glUniform1f(fog_end_loc, fogStart - ZNEAR);
//
//	GLuint fog_width_loc = glGetUniformLocation(program, "fog_width");
//	logOpenGLError();
//	glUniform1f(fog_width_loc, 192.0);
//	logOpenGLError();

	// fxaa
//	glUseProgram(program_postproc);
//
//	GLuint fxaa_span_max_loc = glGetUniformLocation(program_postproc, "fxaa_span_max");
//	logOpenGLError();
//	glUniform1f(fxaa_span_max_loc, 8.0);
//	logOpenGLError();
//
//	GLuint fxaa_reduce_mul_loc = glGetUniformLocation(program_postproc, "fxaa_reduce_mul");
//	logOpenGLError();
//	glUniform1f(fxaa_reduce_mul_loc, 1.0/8.0);
//	logOpenGLError();
//
//	GLuint fxaa_reduce_min_loc = glGetUniformLocation(program_postproc, "fxaa_reduce_min");
//	logOpenGLError();
//	glUniform1f(fxaa_reduce_min_loc, 1.0/128.0);
//	logOpenGLError();

	// display lists
	int length = conf.render_distance * 2 + 3;
	int n = length * length * length;
	firstDL = glGenLists(n);
	dlChunks = new vec3i64[n];
	dlHasChunk = new bool[n];
	dlFaces = new int[n];
	for (int i = 0; i < n; i++) {
		dlHasChunk[i] = false;
		dlFaces[i] = 0;
	}
}

void Graphics::resize(int width, int height) {
	LOG(INFO, "Resize to " << width << "x" << height);
	this->width = width;
	this->height = height;
	glViewport(0, 0, width, height);
	makePerspective();
	makeOrthogonal();
	calcDrawArea();

	// update framebuffer object
	if (fbo)
		destroyFBO();
	if (conf.aa != AntiAliasing::NONE)
		createFBO();

//	glUseProgram(program_postproc);
//	GLuint pixel_size_loc = glGetUniformLocation(program_postproc, "pixel_size");
//	logOpenGLError();
//	glUniform2f(pixel_size_loc, 1.0 / width, 1.0 / height);
//	logOpenGLError();
}

void Graphics::makePerspective() {
	double normalRatio = DEFAULT_WINDOWED_RES[0] / (double) DEFAULT_WINDOWED_RES[1];
	double currentRatio = width / (double) height;
	double angle;

	float yfov = conf.fov / normalRatio * TAU / 360.0;
	if (currentRatio > normalRatio)
		angle = atan(tan(yfov / 2) * normalRatio / currentRatio) * 2;
	else
		angle = yfov;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	double zFar = Chunk::WIDTH * sqrt(3 * (conf.render_distance + 1) * (conf.render_distance + 1));
	gluPerspective((float) (angle * 360.0 / TAU),
			(float) currentRatio, ZNEAR, zFar);
	glGetDoublev(GL_PROJECTION_MATRIX, perspectiveMatrix);
	glMatrixMode(GL_MODELVIEW);
}

void Graphics::makeOrthogonal() {
	double normalRatio = DEFAULT_WINDOWED_RES[0] / (double) DEFAULT_WINDOWED_RES[1];
	double currentRatio = width / (double) height;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if (currentRatio > normalRatio)
		glOrtho(-DEFAULT_WINDOWED_RES[0] / 2.0, DEFAULT_WINDOWED_RES[0] / 2.0, -DEFAULT_WINDOWED_RES[0]
				/ currentRatio / 2.0, DEFAULT_WINDOWED_RES[0] / currentRatio / 2.0, 1,
				-1);
	else
		glOrtho(-DEFAULT_WINDOWED_RES[1] * currentRatio / 2.0, DEFAULT_WINDOWED_RES[1]
				* currentRatio / 2.0, -DEFAULT_WINDOWED_RES[1] / 2.0,
				DEFAULT_WINDOWED_RES[1] / 2.0, 1, -1);
	glGetDoublev(GL_PROJECTION_MATRIX, orthogonalMatrix);
	glMatrixMode(GL_MODELVIEW);
}

void Graphics::makeFog() {
	double fogEnd = std::max(0.0, Chunk::WIDTH * (conf.render_distance - 1.0));
	glFogf(GL_FOG_START, fogEnd - ZNEAR - fogEnd / 3.0);
	glFogf(GL_FOG_END, fogEnd - ZNEAR);
}

void Graphics::makeMaxFOV() {
	float ratio = (float) DEFAULT_WINDOWED_RES[0] / DEFAULT_WINDOWED_RES[1];
	float yfov = conf.fov / ratio * TAU / 360.0;
	if (ratio < 1.0)
		maxFOV = yfov;
	else
		maxFOV = atan(ratio * tan(yfov / 2)) * 2;
}

void Graphics::calcDrawArea() {
	double normalRatio = DEFAULT_WINDOWED_RES[0] / (double) DEFAULT_WINDOWED_RES[1];
	double currentRatio = width / (double) height;
	if (currentRatio > normalRatio) {
		drawWidth = DEFAULT_WINDOWED_RES[0];
		drawHeight = DEFAULT_WINDOWED_RES[0] / currentRatio;
	} else {
		drawWidth = DEFAULT_WINDOWED_RES[1] * currentRatio;
		drawHeight = DEFAULT_WINDOWED_RES[1];
	}
}

void Graphics::setMenu(bool menuActive) {
	SDL_SetWindowGrab(window, (SDL_bool) !menuActive);
	SDL_SetRelativeMouseMode((SDL_bool) !menuActive);
	this->menuActive = menuActive;
	if (menuActive) {
		SDL_WarpMouseInWindow(window, (int) (oldRelMouseX * width), (int) (oldRelMouseY * height));
	} else {
		int x = width / 2;
		int y = height / 2;
		SDL_GetMouseState(&x, &y);
		oldRelMouseX = x / (double) width;
		oldRelMouseY = y / (double) height;
	}
}

bool Graphics::isMenu() {
	return menuActive;
}

void Graphics::setDebug(bool debugActive) {
	this->debugActive = debugActive;
}

bool Graphics::isDebug() {
	return debugActive;
}

//GLuint Graphics::loadShader(const char *path, GLenum type) {
//	LOG(INFO) << "Loading shader source '" << path << "'";
//	std::ifstream f(path);
//	LOG_IF(!f.good(), error) << "Could not open '" << path << "'";
//	std::stringstream ss;
//	ss << f.rdbuf();
//	std::string s = ss.str();
//	const char *source = s.c_str();
//	f.close();
//	LOG(INFO) << "Compiling '" << path << "'";
//	GLenum shader = glCreateShaderObjectARB(type);
//	logOpenGLError();
//	glShaderSourceARB(shader, 1, &source, NULL);
//	logOpenGLError();
//	glCompileShaderARB(shader);
//	logOpenGLError();
//
//	GLint logSize = 0;
//	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logSize);
//	logOpenGLError();
//
//	if(logSize > 0) {
//		GLsizei length;
//		GLchar infoLog[logSize];
//		glGetShaderInfoLog(shader, logSize, &length, infoLog);
//		logOpenGLError();
//		if (length > 0)
//			LOG(WARNING) << "Shader log: " << (char *) infoLog;
//	}
//
//	return shader;
//}
//
//GLuint Graphics::loadProgram(const char *vert_src, const char *frag_src) {
//	GLuint vertex_shader = loadShader(vert_src, GL_VERTEX_SHADER_ARB);
//	GLuint fragment_shader = loadShader(frag_src, GL_FRAGMENT_SHADER_ARB);
//
//	LOG(INFO) << "Assembling shader program";
//	GLuint program = glCreateProgramObjectARB();
//	logOpenGLError();
//	glAttachObjectARB(program, vertex_shader);
//	logOpenGLError();
//	glAttachObjectARB(program, fragment_shader);
//	logOpenGLError();
//	glLinkProgramARB(program);
//	logOpenGLError();
//	glValidateProgram(program);
//
//	GLint logSize = 0;
//	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logSize);
//	logOpenGLError();
//
//	if(logSize > 0) {
//		GLsizei length;
//		GLchar infoLog[logSize];
//		glGetProgramInfoLog(program, logSize, &length, infoLog);
//		logOpenGLError();
//		if (length > 0)
//			LOG(WARNING) << "Shader program log: " << (char *) infoLog;
//	}
//
//	int validate_ok;
//	glGetProgramiv(program, GL_VALIDATE_STATUS, &validate_ok);
//	if (!validate_ok) {
//		LOG(FATAL) << "Shader program failed validation";
//	}
//	logOpenGLError();
//
//	return program;
//}

static uint getMSLevelFromAA(AntiAliasing aa) {
	switch (aa) {
		case AntiAliasing::NONE:    return 0;
		case AntiAliasing::MSAA_2:  return 2;
		case AntiAliasing::MSAA_4:  return 4;
		case AntiAliasing::MSAA_8:  return 8;
		case AntiAliasing::MSAA_16: return 16;
		default:                    return 0;
	}
}

void Graphics::setConf(const GraphicsConf &conf) {
	GraphicsConf old_conf = this->conf;
	this->conf = conf;

	if (GLEW_NV_fog_distance) {
		switch (conf.fog) {
		default:
			break;
		case Fog::FAST:
			glFogi(GL_FOG_DISTANCE_MODE_NV, GL_EYE_PLANE_ABSOLUTE_NV);
			break;
		case Fog::FANCY:
			glFogi(GL_FOG_DISTANCE_MODE_NV, GL_EYE_RADIAL_NV);
			break;
		}
	}

	if (conf.aa != old_conf.aa) {
		if (fbo)
			destroyFBO();
		if (conf.aa != AntiAliasing::NONE)
			createFBO();
	}

	if (conf.fullscreen != old_conf.fullscreen) {
		SDL_SetWindowFullscreen(window, conf.fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
	}

	if (conf.render_distance != old_conf.render_distance) {
		makePerspective();
		makeFog();

		int length = old_conf.render_distance * 2 + 3;
		glDeleteLists(firstDL, length * length * length);
		delete dlChunks;
		delete dlHasChunk;
		length = conf.render_distance * 2 + 3;
		int n = length * length * length;
		firstDL = glGenLists(n);
		dlChunks = new vec3i64[n];
		dlHasChunk = new bool[n];
		dlFaces = new int[n];
		for (int i = 0; i < n; i++) {
			dlHasChunk[i] = false;
			dlFaces[i] = 0;
		}
		faces = 0;
	}

	if (conf.fov != old_conf.fov) {
		makePerspective();
		makeMaxFOV();
	}
}

void Graphics::createFBO() {
	uint msLevel = getMSLevelFromAA(conf.aa);
//	LOG(INFO) << "Creating framebuffer with MSAA=" << msaa
//			<< " and fxaa " << (fxaa ? "enabled" : "disabled");
//	bool needs_render_to_texture = false;
//	if (fxaa)
//		needs_render_to_texture = true;
//
//	if (needs_render_to_texture) {
//		glActiveTexture(GL_TEXTURE0);
//		logOpenGLError();
//		glGenTextures(1, &fbo_texture);
//		glBindTexture(GL_TEXTURE_2D, fbo_texture);
//		logOpenGLError();
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//		logOpenGLError();
//		glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
//		logOpenGLError();
//		glBindTexture(GL_TEXTURE_2D, 0);
//		logOpenGLError();
//	} else {
	glGenRenderbuffers(1, &fbo_color_buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, fbo_color_buffer);
	logOpenGLError();
	glRenderbufferStorageMultisample(
			GL_RENDERBUFFER, msLevel, GL_RGB, width, height
	);
	logOpenGLError();
//	}

	// Depth buffer
	glGenRenderbuffers(1, &fbo_depth_buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, fbo_depth_buffer);
	logOpenGLError();
	if (conf.aa != AntiAliasing::NONE) {
		glRenderbufferStorageMultisample(
				GL_RENDERBUFFER, msLevel, GL_DEPTH_COMPONENT24, width, height
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
//	if (needs_render_to_texture) {
//		glFramebufferTexture2D(
//				GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
//				GL_TEXTURE_2D, fbo_texture, 0
//		);
//	} else {
	glFramebufferRenderbuffer(
			GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_RENDERBUFFER, fbo_color_buffer
	);
//	}
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
		LOG(ERROR, "Framebuffer creation failed: " << msg);
	}
	logOpenGLError();
}

void Graphics::destroyFBO() {
//	glDeleteTextures(1, &fbo_texture);
	glDeleteRenderbuffers(1, &fbo_color_buffer);
	glDeleteRenderbuffers(1, &fbo_depth_buffer);
	glDeleteFramebuffers(1, &fbo);
	fbo = fbo_color_buffer = fbo_depth_buffer = /*fbo_texture = */0;
}

int Graphics::getWidth() {
	return width;
}

int Graphics::getHeight() {
	return width;
}

void Graphics::tick() {

	render();

	stopwatch->start(CLOCK_FSH);
	glFinish();
	stopwatch->stop(CLOCK_FSH);

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
