#include "gl2_renderer.hpp"

#include "game/world.hpp"
#include "game/chunk.hpp"
#include "menu.hpp"
#include "stopwatch.hpp"
#include "engine/time.hpp"

#include "util.hpp"
#include "engine/math.hpp"
#include "constants.hpp"
#include "engine/logging.hpp"

#include "gui/frame.hpp"

#include <SDL2/SDL_image.h>

using namespace gui;

GL2Renderer::GL2Renderer(Client *client) :
	client(client),
	graphics(graphics),
	texManager(client)
{
	makeMaxFOV();
	makePerspective();
	makeOrthogonal();

	// update framebuffer object
	if (client->getConf().aa != AntiAliasing::NONE)
		createFBO();

	initGL();
	for (int i = 0; i < 20; i++) {
		prevFPS[i] = 0;
	}
}

GL2Renderer::~GL2Renderer() {
	LOG(DEBUG, "Destroying Graphics");
	destroyRenderDistanceDependent();

	delete font;
}

void GL2Renderer::destroyRenderDistanceDependent() {
	glDeleteLists(dlFirstAddress, renderDistance * renderDistance * renderDistance);
	delete dlChunks;
	delete dlStatus;
	delete chunkFaces;
	delete chunkPassThroughs;
	delete vsExits;
	delete vsVisited;
	delete vsFringe;
	delete vsIndices;
}

void GL2Renderer::initGL() {
	glClearDepth(1);
	glDepthFunc(GL_LEQUAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glMatrixMode(GL_MODELVIEW);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_CULL_FACE);

	glEnable(GL_LINE_SMOOTH);

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
	const char *block_textures_file = "block_textures.txt";
	if (texManager.load(block_textures_file)) {
		LOG(WARNING, "There was a problem loading '" << block_textures_file << "'");
	}

	// fog
	glEnable(GL_FOG);
	if (GLEW_NV_fog_distance)
		glFogi(GL_FOG_DISTANCE_MODE_NV, GL_EYE_RADIAL_NV);
	else
		LOG(INFO, "GL_NV_fog_distance not available, falling back to z fog");

	glFogfv(GL_FOG_COLOR, fogColor.ptr());
	glFogi(GL_FOG_MODE, GL_LINEAR);
	makeFog();

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

	initRenderDistanceDependent();
}

void GL2Renderer::initRenderDistanceDependent() {
	renderDistance = client->getConf().render_distance * 2 + 3;
	int n = renderDistance * renderDistance * renderDistance;
	dlFirstAddress = glGenLists(n);
	dlChunks = new vec3i64[n];
	dlStatus = new uint8[n];
	chunkFaces = new int[n];
	chunkPassThroughs = new uint16[n];
	vsExits = new uint8[n];
	vsVisited = new bool[n];
	vsFringeCapacity = renderDistance * renderDistance * 6;
	vsFringe = new vec3i64[vsFringeCapacity];
	vsIndices = new int[vsFringeCapacity];
	for (int i = 0; i < n; i++) {
		dlStatus[i] = NO_CHUNK;
		chunkFaces[i] = 0;
		chunkPassThroughs[i] = 0;
		vsVisited[i] = false;
	}
	faces = 0;
}

void GL2Renderer::resize() {
	makePerspective();
	makeOrthogonal();

	// update framebuffer object
	if (fbo)
		destroyFBO();
	if (client->getConf().aa != AntiAliasing::NONE)
		createFBO();
}

void GL2Renderer::makePerspective() {
	double normalRatio = DEFAULT_WINDOWED_RES[0] / (double) DEFAULT_WINDOWED_RES[1];
	double currentRatio = graphics->getWidth() / (double) graphics->getHeight();
	double angle;

	float yfov = client->getConf().fov / normalRatio * TAU / 360.0;
	if (currentRatio > normalRatio)
		angle = atan(tan(yfov / 2) * normalRatio / currentRatio) * 2;
	else
		angle = yfov;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	double zFar = Chunk::WIDTH * sqrt(3 * (client->getConf().render_distance + 1) * (client->getConf().render_distance + 1));
	gluPerspective((float) (angle * 360.0 / TAU),
			(float) currentRatio, ZNEAR, zFar);
	glGetDoublev(GL_PROJECTION_MATRIX, perspectiveMatrix);
	glMatrixMode(GL_MODELVIEW);
}

void GL2Renderer::makeOrthogonal() {
	double normalRatio = DEFAULT_WINDOWED_RES[0] / (double) DEFAULT_WINDOWED_RES[1];
	double currentRatio = graphics->getWidth() / (double) graphics->getHeight();
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

void GL2Renderer::makeFog() {
	double fogEnd = std::max(0.0, Chunk::WIDTH * (client->getConf().render_distance - 1.0));
	glFogf(GL_FOG_START, fogEnd - ZNEAR - fogEnd / 3.0);
	glFogf(GL_FOG_END, fogEnd - ZNEAR);
}

void GL2Renderer::makeMaxFOV() {
	float ratio = (float) DEFAULT_WINDOWED_RES[0] / DEFAULT_WINDOWED_RES[1];
	float yfov = client->getConf().fov / ratio * TAU / 360.0;
	if (ratio < 1.0)
		maxFOV = yfov;
	else
		maxFOV = atan(ratio * tan(yfov / 2)) * 2;
}

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

void GL2Renderer::setConf(const GraphicsConf &conf, const GraphicsConf &old) {
	if (conf.render_distance != old.render_distance) {
		destroyRenderDistanceDependent();
	}

	renderDistance = conf.render_distance;

	if (conf.render_distance != old.render_distance) {
		initRenderDistanceDependent();
	}

	if (conf.fov != old.fov || conf.render_distance != old.render_distance) {
		makeMaxFOV();
		makePerspective();
		makeFog();
	}

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

	if (conf.aa != old.aa) {
		if (fbo)
			destroyFBO();
		if (conf.aa != AntiAliasing::NONE)
			createFBO();
	}

	texManager.setConfig(conf, old);
}

void GL2Renderer::createFBO() {
	uint msLevel = getMSLevelFromAA(client->getConf().aa);
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
			GL_RENDERBUFFER, msLevel, GL_RGB, graphics->getWidth(), graphics->getHeight()
	);
	logOpenGLError();
//	}

	// Depth buffer
	glGenRenderbuffers(1, &fbo_depth_buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, fbo_depth_buffer);
	logOpenGLError();
	if (client->getConf().aa != AntiAliasing::NONE) {
		glRenderbufferStorageMultisample(
				GL_RENDERBUFFER, msLevel, GL_DEPTH_COMPONENT24, graphics->getWidth(), graphics->getHeight()
		);
	} else {
		glRenderbufferStorage(
				GL_RENDERBUFFER, GL_DEPTH_COMPONENT24,
				graphics->getWidth(), graphics->getHeight()
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

void GL2Renderer::destroyFBO() {
//	glDeleteTextures(1, &fbo_texture);
	glDeleteRenderbuffers(1, &fbo_color_buffer);
	glDeleteRenderbuffers(1, &fbo_depth_buffer);
	glDeleteFramebuffers(1, &fbo);
	fbo = fbo_color_buffer = fbo_depth_buffer = /*fbo_texture = */0;
}

void GL2Renderer::tick() {
	render();

	client->getStopwatch()->start(CLOCK_FSH);
	//glFinish();
	client->getStopwatch()->stop(CLOCK_FSH);

	client->getStopwatch()->start(CLOCK_FLP);
	client->getGraphics()->flip();
	client->getStopwatch()->stop(CLOCK_FLP);

    if (getCurrentTime() - lastStopWatchSave > millis(200)) {
		lastStopWatchSave = getCurrentTime();
		client->getStopwatch()->stop(CLOCK_ALL);
		client->getStopwatch()->save();
		client->getStopwatch()->start(CLOCK_ALL);
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
