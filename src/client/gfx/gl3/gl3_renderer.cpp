#include "gl3_renderer.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include "shared/engine/logging.hpp"
#include "shared/engine/math.hpp"
#include "shared/engine/stopwatch.hpp"
#include "shared/game/world.hpp"
#include "client/menu.hpp"
#include "client/gfx/graphics.hpp"

#include "gl3_chunk_renderer.hpp"
#include "gl3_target_renderer.hpp"
#include "gl3_sky_renderer.hpp"
#include "gl3_crosshair_renderer.hpp"
#include "gl3_hud_renderer.hpp"
#include "gl3_menu_renderer.hpp"
#include "gl3_debug_renderer.hpp"

using namespace gui;

static logging::Logger logger("render");

GL3Renderer::GL3Renderer(Client *client) :
	client(client),
	shaderManager(),
	texManager(client)
{
	p_chunkRenderer = new GL3ChunkRenderer(client, this);
	chunkRenderer = std::unique_ptr<ComponentRenderer>(p_chunkRenderer);
	targetRenderer = std::unique_ptr<ComponentRenderer>(new GL3TargetRenderer(client, this));
	skyRenderer = std::unique_ptr<ComponentRenderer>(new GL3SkyRenderer(client, this));
	crosshairRenderer = std::unique_ptr<ComponentRenderer>(new GL3CrosshairRenderer(client, this));
	hudRenderer = std::unique_ptr<ComponentRenderer>(new GL3HudRenderer(client, this));
	menuRenderer = std::unique_ptr<ComponentRenderer>(new GL3MenuRenderer(client, this));
	debugRenderer = std::unique_ptr<ComponentRenderer>(new GL3DebugRenderer(client, this, p_chunkRenderer));

	makeMaxFOV(client->getConf().fov);
	makePerspectiveMatrix(client->getConf().render_distance, client->getConf().fov);
	makeOrthogonalMatrix();

	// light
	auto &defaultShader = shaderManager.getDefaultShader();
	defaultShader.setAmbientLightColor(ambientColor);
	defaultShader.setDiffuseLightDirection(diffuseDirection);
	defaultShader.setDiffuseLightColor(diffuseColor);

	auto &blockShader = shaderManager.getBlockShader();
	blockShader.setAmbientLightColor(ambientColor);
	blockShader.setDiffuseLightDirection(diffuseDirection);
	blockShader.setDiffuseLightColor(diffuseColor);

	// fog
	float endFog = (float) ((client->getConf().render_distance - 1) * Chunk::WIDTH);
	float startFog = (client->getConf().render_distance - 1) * Chunk::WIDTH * 0.5f;
	bool fog = client->getConf().fog == Fog::FANCY || client->getConf().fog == Fog::FAST;

	defaultShader.setEndFogDistance(endFog);
	defaultShader.setStartFogDistance(startFog);
	defaultShader.setFogEnabled(fog);

	blockShader.setEndFogDistance(endFog);
	blockShader.setStartFogDistance(startFog);
	blockShader.setFogEnabled(fog);

	// sky
	makeSkyFbo();

	// make framebuffer object
	if (client->getConf().aa != AntiAliasing::NONE)
		createFBO(client->getConf().aa);

	// textures
	LOG_DEBUG(logger) << "Loading textures";
	const char *block_textures_file = client->getConf().textures_file.c_str();
	if (texManager.load(block_textures_file)) {
		LOG_WARNING(logger) << "There was a problem loading '" << block_textures_file << "'";
	}

	// gl stuff
	GL(Enable(GL_BLEND));
	GL(BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
	GL(Enable(GL_CULL_FACE));
}

GL3Renderer::~GL3Renderer() {
	LOG_DEBUG(logger) << "Destroying GL3 renderer";

	if (skyFbo) GL(DeleteFramebuffers(1, &skyFbo));
	if (skyTex) GL(DeleteTextures(1, &skyTex));
}

void GL3Renderer::resize() {
	makePerspectiveMatrix(client->getConf().render_distance, client->getConf().fov);
	makeOrthogonalMatrix();
	makeSkyFbo();

	// update framebuffer object
	if (fbo)
		destroyFBO();
	if (client->getConf().aa != AntiAliasing::NONE)
		createFBO(client->getConf().aa);
}

void GL3Renderer::setConf(const GraphicsConf &conf, const GraphicsConf &old) {
	auto &defaultShader = shaderManager.getDefaultShader();
	auto &blockShader = shaderManager.getBlockShader();

	if (conf.render_distance != old.render_distance || conf.fov != old.fov) {
		makePerspectiveMatrix(conf.render_distance, conf.fov);
	}

	if (conf.render_distance != old.render_distance) {

		float endFog = (float) ((conf.render_distance - 1) * Chunk::WIDTH);
		float startFog = (conf.render_distance - 1) * Chunk::WIDTH * 0.5f;

		defaultShader.setEndFogDistance(endFog);
		defaultShader.setStartFogDistance(startFog);

		blockShader.setEndFogDistance(endFog);
		blockShader.setStartFogDistance(startFog);
	}

	if (conf.fov != old.fov) {
		makeMaxFOV(conf.fov);
	}

	if (conf.aa != old.aa) {
		if (fbo)
			destroyFBO();
		if (conf.aa != AntiAliasing::NONE)
			createFBO(conf.aa);
	}

	bool fog = conf.fog == Fog::FANCY || conf.fog == Fog::FAST;
	defaultShader.setFogEnabled(fog);
	blockShader.setFogEnabled(fog);

	chunkRenderer->setConf(conf, old);
}

void GL3Renderer::rebuildChunk(vec3i64 chunkCoords) {
	p_chunkRenderer->rebuildChunk(chunkCoords);
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

void GL3Renderer::createFBO(AntiAliasing antiAliasing) {
	uint msLevel = getMSLevelFromAA(antiAliasing);
	GL(GenRenderbuffers(1, &fbo_color_buffer));
	GL(BindRenderbuffer(GL_RENDERBUFFER, fbo_color_buffer));
	GL(RenderbufferStorageMultisample(
			GL_RENDERBUFFER, msLevel, GL_RGB, client->getGraphics()->getWidth(), client->getGraphics()->getHeight()
	));

	// Depth buffer
	GL(GenRenderbuffers(1, &fbo_depth_buffer));
	GL(BindRenderbuffer(GL_RENDERBUFFER, fbo_depth_buffer));
	if (antiAliasing != AntiAliasing::NONE) {
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

void GL3Renderer::destroyFBO() {
	glDeleteRenderbuffers(1, &fbo_color_buffer);
	glDeleteRenderbuffers(1, &fbo_depth_buffer);
	glDeleteFramebuffers(1, &fbo);
	fbo = fbo_color_buffer = fbo_depth_buffer = 0;
}

void GL3Renderer::makePerspectiveMatrix(int renderDistance, float fieldOfView) {
	float normalRatio = (float) DEFAULT_WINDOWED_RES[0] / DEFAULT_WINDOWED_RES[1];
	float currentRatio = (float) client->getGraphics()->getWidth() / client->getGraphics()->getHeight();
	float angle;

	float yfov = fieldOfView / normalRatio * (float) (TAU / 360.0);
	if (currentRatio > normalRatio)
		angle = atan(tan(yfov / 2) * normalRatio / currentRatio) * 2;
	else
		angle = yfov;

	float zFar = Chunk::WIDTH * sqrtf(3.0f * (renderDistance + 1) * (renderDistance + 1));
	glm::mat4 perspectiveMatrix = glm::perspective((float) angle,
			(float) currentRatio, ZNEAR, zFar);
	auto &defaultShader = shaderManager.getDefaultShader();
	defaultShader.setProjectionMatrix(perspectiveMatrix);
	auto &blockShader = shaderManager.getBlockShader();
	blockShader.setProjectionMatrix(perspectiveMatrix);
}

void GL3Renderer::makeOrthogonalMatrix() {
	float normalRatio = DEFAULT_WINDOWED_RES[0] / (float) DEFAULT_WINDOWED_RES[1];
	float currentRatio = client->getGraphics()->getWidth() / (float) client->getGraphics()->getHeight();
	glm::mat4 hudMatrix;
	if (currentRatio > normalRatio)
		hudMatrix = glm::ortho(-DEFAULT_WINDOWED_RES[0] / 2.0f, DEFAULT_WINDOWED_RES[0] / 2.0f, -DEFAULT_WINDOWED_RES[0]
				/ currentRatio / 2.0f, DEFAULT_WINDOWED_RES[0] / currentRatio / 2.0f, 1.0f, -1.0f);
	else
		hudMatrix = glm::ortho(-DEFAULT_WINDOWED_RES[1] * currentRatio / 2.0f, DEFAULT_WINDOWED_RES[1]
				* currentRatio / 2.0f, -DEFAULT_WINDOWED_RES[1] / 2.0f,
				DEFAULT_WINDOWED_RES[1] / 2.0f, 1.0f, -1.0f);
    shaderManager.getHudShader().setProjectionMatrix(hudMatrix);
    shaderManager.getFontShader().setProjectionMatrix(hudMatrix);
}

void GL3Renderer::makeMaxFOV(float fieldOfView) {
	float ratio = (float) DEFAULT_WINDOWED_RES[0] / DEFAULT_WINDOWED_RES[1];
	float yfov = fieldOfView / ratio * (float) (TAU / 360.0);
	if (ratio < 1.0)
		maxFOV = yfov;
	else
		maxFOV = atan(ratio * tan(yfov / 2)) * 2;
}

void GL3Renderer::makeSkyFbo() {
	if (skyFbo) GL(DeleteFramebuffers(1, &skyFbo));
	if (skyTex) GL(DeleteTextures(1, &skyTex));

	GL(GenTextures(1, &skyTex));
	GL(BindTexture(GL_TEXTURE_2D, skyTex));
	GL(TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, client->getGraphics()->getWidth(), client->getGraphics()->getHeight(), 0, GL_RGBA, GL_FLOAT, 0));
	GL(TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GL(TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

	GL(GenFramebuffers(1, &skyFbo));
	GL(BindFramebuffer(GL_FRAMEBUFFER, skyFbo));
	GL(FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, skyTex, 0));
	GL(BindFramebuffer(GL_FRAMEBUFFER, 0));
}

void GL3Renderer::tick() {
	chunkRenderer->tick();
	debugRenderer->tick();
}

void GL3Renderer::render() {
	if (fbo) {
		GL(BindFramebuffer(GL_FRAMEBUFFER, fbo));
	} else {
		GL(BindFramebuffer(GL_FRAMEBUFFER, 0));
		GL(DrawBuffer(GL_BACK));
	}

	// render sky
	GL(Disable(GL_DEPTH_TEST));
	GL(DepthMask(false));
	skyRenderer->render();
	if (client->getConf().fog == Fog::FANCY || client->getConf().fog == Fog::FAST) {
		GL(BindFramebuffer(GL_FRAMEBUFFER, skyFbo));
		skyRenderer->render();
		if (fbo) {
			GL(BindFramebuffer(GL_FRAMEBUFFER, fbo));
		} else {
			GL(BindFramebuffer(GL_FRAMEBUFFER, 0));
			GL(DrawBuffer(GL_BACK));
		}
		GL(ActiveTexture(GL_TEXTURE1));
		GL(BindTexture(GL_TEXTURE_2D, skyTex));
	}
	
	// render scene
	GL(Enable(GL_DEPTH_TEST));
	GL(DepthMask(true));
	GL(Clear(GL_DEPTH_BUFFER_BIT));
	chunkRenderer->render();
	targetRenderer->render();

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

	// render overlay
	GL(Disable(GL_DEPTH_TEST));
	GL(DepthMask(false));

	crosshairRenderer->render();
	hudRenderer->render();
	debugRenderer->render();
	menuRenderer->render();

	client->getStopwatch()->start(CLOCK_FSH);
	//glFinish();
	client->getStopwatch()->stop(CLOCK_FSH);

	client->getStopwatch()->start(CLOCK_FLP);
	client->getGraphics()->flip();
	client->getStopwatch()->stop(CLOCK_FLP);
}
