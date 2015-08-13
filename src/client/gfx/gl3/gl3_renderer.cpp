#include "gl3_renderer.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include "shared/engine/logging.hpp"
#include "shared/engine/math.hpp"
#include "shared/engine/stopwatch.hpp"
#include "shared/game/world.hpp"
#include "client/menu.hpp"
#include "client/gfx/graphics.hpp"

#include "gl3_chunk_renderer.hpp"
#include "gl3_debug_renderer.hpp"

using namespace gui;

static logging::Logger logger("render");

GL3Renderer::GL3Renderer(Client *client) :
	client(client),
	shaderManager(),
	texManager(client),
	fontTimes(&shaderManager.getFontShader()),
	fontDejavu(&shaderManager.getFontShader())
{
	p_chunkRenderer = new GL3ChunkRenderer(client, this);
	p_chunkRenderer->init();
	chunkRenderer = std::unique_ptr<ComponentRenderer>(p_chunkRenderer);
	targetRenderer = std::unique_ptr<ComponentRenderer>(new GL3TargetRenderer(client, this));
	skyRenderer = std::unique_ptr<ComponentRenderer>(new GL3SkyRenderer(client, this));
	hudRenderer = std::unique_ptr<ComponentRenderer>(new GL3HudRenderer(client, this));
	menuRenderer = std::unique_ptr<ComponentRenderer>(new GL3MenuRenderer(client, this));
	debugRenderer = std::unique_ptr<ComponentRenderer>(new GL3DebugRenderer(client, this, p_chunkRenderer));

	makeMaxFOV();
	makePerspectiveMatrix();
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

    // font
	fontTimes.load("fonts/times32.fnt");
	fontTimes.setEncoding(Font::Encoding::UTF8);
	fontDejavu.load("fonts/dejavusansmono16.fnt");
	fontDejavu.setEncoding(Font::Encoding::UTF8);

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
	makePerspectiveMatrix();
	makeOrthogonalMatrix();
	makeSkyFbo();
}

void GL3Renderer::setConf(const GraphicsConf &conf, const GraphicsConf &old) {
	auto &defaultShader = shaderManager.getDefaultShader();
	auto &blockShader = shaderManager.getBlockShader();

	if (conf.render_distance != old.render_distance) {
		makePerspectiveMatrix();

		float endFog = (float) ((conf.render_distance - 1) * Chunk::WIDTH);
		float startFog = (conf.render_distance - 1) * Chunk::WIDTH * 0.5f;

		defaultShader.setEndFogDistance(endFog);
		defaultShader.setStartFogDistance(startFog);

		blockShader.setEndFogDistance(endFog);
		blockShader.setStartFogDistance(startFog);
	}

	if (conf.fov != old.fov) {
		makePerspectiveMatrix();
		makeMaxFOV();
	}

	bool fog = conf.fog == Fog::FANCY || conf.fog == Fog::FAST;
	defaultShader.setFogEnabled(fog);
	blockShader.setFogEnabled(fog);

	chunkRenderer->setConf(conf, old);
}

void GL3Renderer::rebuildChunk(vec3i64 chunkCoords) {
	p_chunkRenderer->rebuildChunk(chunkCoords);
}

void GL3Renderer::makePerspectiveMatrix() {
	float normalRatio = (float) DEFAULT_WINDOWED_RES[0] / DEFAULT_WINDOWED_RES[1];
	float currentRatio = (float) client->getGraphics()->getWidth() / client->getGraphics()->getHeight();
	float angle;

	float yfov = client->getConf().fov / normalRatio * (float) (TAU / 360.0);
	if (currentRatio > normalRatio)
		angle = atan(tan(yfov / 2) * normalRatio / currentRatio) * 2;
	else
		angle = yfov;

	float zFar = Chunk::WIDTH * sqrtf(3.0f * (client->getConf().render_distance + 1) * (client->getConf().render_distance + 1));
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

void GL3Renderer::makeMaxFOV() {
	float ratio = (float) DEFAULT_WINDOWED_RES[0] / DEFAULT_WINDOWED_RES[1];
	float yfov = client->getConf().fov / ratio * (float) (TAU / 360.0);
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
	client->getStopwatch()->start(CLOCK_CRT);
	chunkRenderer->tick();
	client->getStopwatch()->stop(CLOCK_CRT);
	debugRenderer->tick();
}

void GL3Renderer::render() {
	// render sky
	GL(Disable(GL_DEPTH_TEST));
	GL(DepthMask(false));
	skyRenderer->render();
	if (client->getConf().fog == Fog::FANCY || client->getConf().fog == Fog::FAST) {
		GL(BindFramebuffer(GL_FRAMEBUFFER, skyFbo));
		skyRenderer->render();
		GL(BindFramebuffer(GL_FRAMEBUFFER, 0));
		GL(ActiveTexture(GL_TEXTURE1));
		GL(BindTexture(GL_TEXTURE_2D, skyTex));
	}
	
	// render scene
	GL(Enable(GL_DEPTH_TEST));
	GL(DepthMask(true));
	GL(Clear(GL_DEPTH_BUFFER_BIT));
	client->getStopwatch()->start(CLOCK_CRR);
	chunkRenderer->render();
	client->getStopwatch()->stop(CLOCK_CRR);
	targetRenderer->render();

	// render overlay
	GL(Disable(GL_DEPTH_TEST));
	GL(DepthMask(false));

	hudRenderer->render();
	debugRenderer->render();
	menuRenderer->render();

	client->getGraphics()->flip();
}
