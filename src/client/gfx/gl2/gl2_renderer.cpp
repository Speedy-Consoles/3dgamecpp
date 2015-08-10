#include "gl2_renderer.hpp"

#include <SDL2/SDL_image.h>

#include "shared/engine/logging.hpp"
#include "shared/engine/stopwatch.hpp"
#include "shared/engine/time.hpp"
#include "shared/engine/math.hpp"
#include "shared/game/world.hpp"
#include "shared/game/chunk.hpp"
#include "shared/constants.hpp"
#include "shared/block_utils.hpp"
#include "client/menu.hpp"
#include "client/gui/frame.hpp"

using namespace gui;

static logging::Logger logger("render");

GL2Renderer::GL2Renderer(Client *client) :
	client(client),
	texManager(client),
	chunkRenderer(client, this),
	skyRenderer(client, this),
	hudRenderer(client, this),
	menuRenderer(client, this),
	debugRenderer(client, this)
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
	LOG_TRACE(logger) << "Destroying Renderer";
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

void GL2Renderer::resize() {
	makePerspective();
	makeOrthogonal();

	// update framebuffer object
	if (fbo)
		destroyFBO();
	if (client->getConf().aa != AntiAliasing::NONE)
		createFBO();
}

void GL2Renderer::setConf(const GraphicsConf &conf, const GraphicsConf &old) {
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

void GL2Renderer::rerenderChunk(vec3i64 chunkCoords) {
	// TODO
}

TextureManager *GL2Renderer::getTextureManager() {
	return &texManager;
}

bool GL2Renderer::inFrustum(vec3i64 cc, vec3i64 pos, vec3d lookDir) {
	double chunkDia = sqrt(3) * Chunk::WIDTH * RESOLUTION;
	vec3d cp = (cc * Chunk::WIDTH * RESOLUTION - pos).cast<double>();
	double chunkLookDist = lookDir * cp + chunkDia;
	if (chunkLookDist < 0)
		return false;
	vec3d orthoChunkPos = cp - lookDir * chunkLookDist;
	double orthoChunkDist = std::max(0.0, orthoChunkPos.norm() - chunkDia);
	return atan(orthoChunkDist / chunkLookDist) <= maxFOV / 2;
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
	LOG_DEBUG(logger) << "Initializing light";
	float lModelAmbient[4] = {0.0f, 0.0f, 0.0f, 1.0f};

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

	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

	// textures
	LOG_DEBUG(logger) << "Loading textures";
	const char *block_textures_file = client->getConf().textures_file.c_str();
	if (texManager.load(block_textures_file)) {
		LOG_WARNING(logger) << "There was a problem loading '" << block_textures_file << "'";
	}

	// fog
	glEnable(GL_FOG);
	if (GLEW_NV_fog_distance)
		glFogi(GL_FOG_DISTANCE_MODE_NV, GL_EYE_RADIAL_NV);
	else
		LOG_INFO(logger) << "GL_NV_fog_distance not available, falling back to z fog";

	glFogfv(GL_FOG_COLOR, fogColor.ptr());
	glFogi(GL_FOG_MODE, GL_LINEAR);
	makeFog();
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

void GL2Renderer::createFBO() {
	uint msLevel = getMSLevelFromAA(client->getConf().aa);
	GL(GenRenderbuffers(1, &fbo_color_buffer));
	GL(BindRenderbuffer(GL_RENDERBUFFER, fbo_color_buffer));
	GL(RenderbufferStorageMultisample(
			GL_RENDERBUFFER, msLevel, GL_RGB, client->getGraphics()->getWidth(), client->getGraphics()->getHeight()
	));

	// Depth buffer
	GL(GenRenderbuffers(1, &fbo_depth_buffer));
	GL(BindRenderbuffer(GL_RENDERBUFFER, fbo_depth_buffer));
	if (client->getConf().aa != AntiAliasing::NONE) {
		GL(RenderbufferStorageMultisample(
				GL_RENDERBUFFER, msLevel, GL_DEPTH_COMPONENT24, client->getGraphics()->getWidth(), client->getGraphics()->getHeight()
		));
	} else {
		GL(RenderbufferStorage(
				GL_RENDERBUFFER, GL_DEPTH_COMPONENT24,
				client->getGraphics()->getWidth(), client->getGraphics()->getHeight()
		));
	}

	GL(GenFramebuffers(1, &fbo));
	GL(BindFramebuffer(GL_FRAMEBUFFER, fbo));
	GL(FramebufferRenderbuffer(
			GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_RENDERBUFFER, fbo_color_buffer
	));
	GL(FramebufferRenderbuffer(
			GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
			GL_RENDERBUFFER, fbo_depth_buffer
	));

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
		LOG_ERROR(logger) << "Framebuffer creation failed: " << msg;
	}
}

void GL2Renderer::destroyFBO() {
	glDeleteRenderbuffers(1, &fbo_color_buffer);
	glDeleteRenderbuffers(1, &fbo_depth_buffer);
	glDeleteFramebuffers(1, &fbo);
	fbo = fbo_color_buffer = fbo_depth_buffer = 0;
}

void GL2Renderer::makePerspective() {
	float normalRatio = DEFAULT_WINDOWED_RES[0] / (float) DEFAULT_WINDOWED_RES[1];
	float currentRatio = client->getGraphics()->getWidth() / (float) client->getGraphics()->getHeight();
	float angle;

	float yfov = client->getConf().fov / normalRatio * (float) (TAU / 360.0);
	if (currentRatio > normalRatio)
		angle = atan(tan(yfov / 2) * normalRatio / currentRatio) * 2;
	else
		angle = yfov;

	GL(MatrixMode(GL_PROJECTION));
	GL(LoadIdentity());
	double zFar = Chunk::WIDTH * sqrt(3 * (client->getConf().render_distance + 1) * (client->getConf().render_distance + 1));
	GL(uPerspective((float) (angle * 360.0 / TAU),
			(float) currentRatio, ZNEAR, zFar));
	GL(GetDoublev(GL_PROJECTION_MATRIX, perspectiveMatrix));
	GL(MatrixMode(GL_MODELVIEW));
}

void GL2Renderer::makeOrthogonal() {
	double normalRatio = DEFAULT_WINDOWED_RES[0] / (double) DEFAULT_WINDOWED_RES[1];
	double currentRatio = client->getGraphics()->getWidth() / (double) client->getGraphics()->getHeight();
	GL(MatrixMode(GL_PROJECTION));
	GL(LoadIdentity());
	if (currentRatio > normalRatio) {
		GL(Ortho(-DEFAULT_WINDOWED_RES[0] / 2.0, DEFAULT_WINDOWED_RES[0] / 2.0, -DEFAULT_WINDOWED_RES[0]
				/ currentRatio / 2.0, DEFAULT_WINDOWED_RES[0] / currentRatio / 2.0, 1,
				-1));
	} else {
		GL(Ortho(-DEFAULT_WINDOWED_RES[1] * currentRatio / 2.0, DEFAULT_WINDOWED_RES[1]
				* currentRatio / 2.0, -DEFAULT_WINDOWED_RES[1] / 2.0,
				DEFAULT_WINDOWED_RES[1] / 2.0, 1, -1));
	}
	GL(GetDoublev(GL_PROJECTION_MATRIX, orthogonalMatrix));
	GL(MatrixMode(GL_MODELVIEW));
}

void GL2Renderer::makeFog() {
	float fogEnd = std::max(0.0f, Chunk::WIDTH * (client->getConf().render_distance - 1.0f));
	GL(Fogf(GL_FOG_START, fogEnd - ZNEAR - fogEnd / 3.0f));
	GL(Fogf(GL_FOG_END, fogEnd - ZNEAR));
}

void GL2Renderer::makeMaxFOV() {
	float ratio = (float) DEFAULT_WINDOWED_RES[0] / DEFAULT_WINDOWED_RES[1];
	float yfov = client->getConf().fov / ratio * (float) (TAU / 360.0);
	if (ratio < 1.0)
		maxFOV = yfov;
	else
		maxFOV = atan(ratio * tan(yfov / 2)) * 2;
}

void GL2Renderer::switchToPerspective() {
	GL(MatrixMode(GL_PROJECTION));
	GL(LoadMatrixd(perspectiveMatrix));
	GL(MatrixMode(GL_MODELVIEW));
}

void GL2Renderer::switchToOrthogonal() {
	GL(MatrixMode(GL_PROJECTION));
	GL(LoadMatrixd(orthogonalMatrix));
	GL(MatrixMode(GL_MODELVIEW));
}

void GL2Renderer::render() {
	if (fbo) {
		GL(BindFramebuffer(GL_FRAMEBUFFER, fbo));
	} else {
		GL(BindFramebuffer(GL_FRAMEBUFFER, 0));
		GL(DrawBuffer(GL_BACK));
	}

	client->getStopwatch()->start(CLOCK_CLR);
	GL(Disable(GL_DEPTH_TEST));
	GL(DepthMask(true));
	GL(Clear(GL_DEPTH_BUFFER_BIT));
	client->getStopwatch()->stop(CLOCK_CLR);

	GL(MatrixMode(GL_MODELVIEW));
	switchToPerspective();
	glLoadIdentity();

	Player &player = client->getLocalPlayer();
	if (player.isValid()) {
		GL(Disable(GL_DEPTH_TEST));
		GL(Disable(GL_TEXTURE_2D));
		GL(Disable(GL_LIGHTING));
		GL(Disable(GL_FOG));
		GL(DepthMask(false));

		GL(Rotated(-player.getPitch(), 1, 0, 0));
		skyRenderer.render();

		GL(Enable(GL_DEPTH_TEST));
		GL(Enable(GL_TEXTURE_2D));
		GL(Enable(GL_LIGHTING));
		if (client->getConf().fog != Fog::NONE)
			glEnable(GL_FOG);
		GL(DepthMask(true));

		GL(Rotatef(-player.getYaw(), 0, 1, 0));
		GL(Rotatef(-90, 1, 0, 0));
		GL(Rotatef(90, 0, 0, 1));
		vec3i64 playerPos = player.getPos();
		int64 m = RESOLUTION * Chunk::WIDTH;
		GL(Translatef(
			(float) -((playerPos[0] % m + m) % m) / RESOLUTION,
			(float) -((playerPos[1] % m + m) % m) / RESOLUTION,
			(float) -((playerPos[2] % m + m) % m) / RESOLUTION
		));
		glLightfv(GL_LIGHT0, GL_POSITION, sunLightPosition.ptr());
		chunkRenderer.render();

		// copy framebuffer to screen
		if (fbo) {
			GL(BindFramebuffer(GL_DRAW_FRAMEBUFFER, 0));
			GL(BindFramebuffer(GL_READ_FRAMEBUFFER, fbo));
			GL(DrawBuffer(GL_BACK));
			GL(BlitFramebuffer(
					0, 0, client->getGraphics()->getWidth(), client->getGraphics()->getHeight(),
					0, 0, client->getGraphics()->getWidth(), client->getGraphics()->getHeight(),
					GL_COLOR_BUFFER_BIT,
					GL_NEAREST
			));

			GL(BindFramebuffer(GL_FRAMEBUFFER, 0));
			GL(DrawBuffer(GL_BACK));
		}
	}

	// render overlay
	switchToOrthogonal();
	GL(LoadIdentity());

	GL(Disable(GL_DEPTH_TEST));
	GL(Disable(GL_TEXTURE_2D));
	GL(Disable(GL_LIGHTING));
	GL(Disable(GL_FOG));
	GL(DepthMask(false));

	if (client->getState() == Client::State::PLAYING && player.isValid()) {
		client->getStopwatch()->start(CLOCK_HUD);
		hudRenderer.render();
		if (client->isDebugOn())
			debugRenderer.render();
		client->getStopwatch()->stop(CLOCK_HUD);
	} else if (client->getState() == Client::State::IN_MENU){
		menuRenderer.render();
	}
}
