#include "graphics.hpp"
#include "vmath.hpp"
#include "util.hpp"
#include "constants.hpp"
#include "logging.hpp"

#include <cmath>
#include <string>
#include <fstream>
#include <sstream>

Graphics::Graphics(
		World *world,
		int localClientID,
		const GraphicsConf &conf,
		Stopwatch *stopwatch) :
		conf(conf),
		world(world),
		localClientID(localClientID),
		stopwatch(stopwatch) {
	LOG(INFO) << "Constructing Graphics";

	LOG(INFO) << "Initializing SDL";
	auto failure = SDL_Init(SDL_INIT_VIDEO);
	LOG_IF(failure, FATAL) << SDL_GetError();

	LOG(INFO) << "Creating window";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	window = SDL_CreateWindow(
		"3dgame",
		0, 0,
		conf.windowed_res[0], conf.windowed_res[1],
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
	);
	LOG_IF(!window, FATAL) << SDL_GetError();

	LOG(INFO) << "Creating Open GL Context";
	glContext = SDL_GL_CreateContext(window);
	LOG_IF(!glContext, FATAL) << SDL_GetError();

	LOG(INFO) << "OpenGL Version: " << glGetString(GL_VERSION);
	LOG(INFO) << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION);

	LOG(INFO) << "Initializing GLEW";
	GLenum glew_error = glewInit();
	LOG_IF(glew_error != GLEW_OK, FATAL) << glewGetErrorString(glew_error);
	LOG_IF(!GLEW_VERSION_2_0, FATAL) << "OpenGL version 2.0 not available";
	if (GLEW_VERSION_3_0) {
		// TODO fancy stuff
	}

	initGL();
	resize(conf.windowed_res[0], conf.windowed_res[1]);

	if (START_WIDTH <= START_HEIGHT)
		maxFOV = YFOV;
	else
		maxFOV = atan(START_WIDTH * tan(YFOV / 2) / START_HEIGHT) * 2;

	startTimePoint = high_resolution_clock::now();
}

Graphics::~Graphics() {
	LOG(INFO) << "Destroying Graphics";

	int length = VIEW_RANGE * 2 + 1;
	glDeleteLists(firstDL, length * length * length);
	delete dlChunks;
	delete font;

//	glDeleteProgram(program);
//	glDeleteProgram(program_postproc);

	SDL_GL_DeleteContext(glContext);
	SDL_Quit();
}

void Graphics::initGL() {
	glClearColor(fogColor[0], fogColor[1], fogColor[2], 1.0);
	glClearDepth(1);
	glDepthFunc(GL_LEQUAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glMatrixMode(GL_MODELVIEW);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_CULL_FACE);

	// light
	LOG(INFO) << "Initializing light";
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
	LOG(INFO) << "Initializing font";
	font = new FTGLTextureFont("res/DejaVuSansMono.ttf");
	if (font) {
		font->FaceSize(16);
		font->CharMap(ft_encoding_unicode);
	} else {
		LOG(ERROR)  << "Could not open 'res/DejaVuSansMono.ttf'";
	}

	// textures
	LOG(INFO) << "Initializing textures";
	IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF);
	SDL_Surface *blockImage = IMG_Load("img/block.png");
	if (blockImage) {
		glGenTextures(1, &blockTexture);
		glBindTexture(GL_TEXTURE_2D, blockTexture);
		logOpenGLError();
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
		glTexImage2D(GL_TEXTURE_2D, 0, 4, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, blockImage->pixels);
		SDL_FreeSurface(blockImage);
		logOpenGLError();
	} else {
		LOG(ERROR)  << "Could not open 'block.png'";
	}

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
		LOG(INFO) << "GL_NV_fog_distance not available, falling back to z fog";

	glFogi(GL_FOG_MODE, GL_LINEAR);
	glFogf(GL_FOG_START, ZFAR - 0.1f - 192.0);
	glFogf(GL_FOG_END, ZFAR - 0.1f);
	glFogfv(GL_FOG_COLOR, fogColor);

//	glUseProgram(program);
//
//	GLuint fog_color_loc = glGetUniformLocation(program, "fog_color");
//	logOpenGLError();
//	glUniform3f(fog_color_loc, 0.5, 0.5, 0.5);
//	logOpenGLError();
//
//	glUseProgram(program);
//	GLuint fog_end_loc = glGetUniformLocation(program, "fog_end");
//	glUniform1f(fog_end_loc, ZFAR - 0.1f);
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
	int length = VIEW_RANGE * 2 + 1;
	firstDL = glGenLists(length * length * length);
	dlChunks = new vec3i64[length * length * length];
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
	if (fbo)
		destroyFBO();
	if (getMSAA())
		createFBO();

//	glUseProgram(program_postproc);
//	GLuint pixel_size_loc = glGetUniformLocation(program_postproc, "pixel_size");
//	logOpenGLError();
//	glUniform2f(pixel_size_loc, 1.0 / width, 1.0 / height);
//	logOpenGLError();
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
	gluPerspective((float) (angle * 360.0 / TAU),
			(float) currentRatio, 0.1f, ZFAR);
	glGetDoublev(GL_PROJECTION_MATRIX, perspectiveMatrix);
	glMatrixMode(GL_MODELVIEW);
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

void Graphics::setMenu(bool menu) {
	SDL_SetWindowGrab(window, (SDL_bool) !menu);
	SDL_SetRelativeMouseMode((SDL_bool) !menu);
	this->menu = menu;
	if (menu) {
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
	return menu;
}

//GLuint Graphics::loadShader(const char *path, GLenum type) {
//	LOG(INFO) << "Loading shader source '" << path << "'";
//	std::ifstream f(path);
//	LOG_IF(!f.good(), ERROR) << "Could not open '" << path << "'";
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

static uint getMSAA(AntiAliasing aa) {
	switch (aa) {
		case AntiAliasing::NONE:    return 0;
		case AntiAliasing::MSAA_2:  return 2;
		case AntiAliasing::MSAA_4:  return 4;
		case AntiAliasing::MSAA_8:  return 8;
		case AntiAliasing::MSAA_16: return 16;
		default:                    return 0;
	}
}

static AntiAliasing getAA(uint msaa) {
	switch (msaa) {
		case 0:  return AntiAliasing::NONE;
		case 2:  return AntiAliasing::MSAA_2;
		case 4:  return AntiAliasing::MSAA_4;
		case 8:  return AntiAliasing::MSAA_8;
		case 16: return AntiAliasing::MSAA_16;
		default: return AntiAliasing::NONE;
	}
}

void Graphics::enableMSAA(uint samples) {
	destroyFBO();
	conf.aa = getAA(samples);
	createFBO();
}

void Graphics::disableMSAA() {
	destroyFBO();
	conf.aa = AntiAliasing::NONE;
}

uint Graphics::getMSAA() const {
	return ::getMSAA(conf.aa);
}

//void Graphics::enableFXAA() {
//	destroyFBO();
//	msaa = 0;
//	fxaa = true;
//	createFBO();
//}
//
//void Graphics::disableFXAA() {
//	destroyFBO();
//	msaa = 0;
//	fxaa = false;
//}
//
void Graphics::createFBO() {
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
			GL_RENDERBUFFER, getMSAA(), GL_RGB, width, height
	);
	logOpenGLError();
//	}

	// Depth buffer
	glGenRenderbuffers(1, &fbo_depth_buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, fbo_depth_buffer);
	logOpenGLError();
	if (getMSAA()) {
		glRenderbufferStorageMultisample(
				GL_RENDERBUFFER, getMSAA(), GL_DEPTH_COMPONENT24, width, height
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
		LOG(ERROR) << "Framebuffer creation failed: " << msg;
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
