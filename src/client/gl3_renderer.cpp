#include "gl3_renderer.hpp"

#include <SDL2/SDL.h>
#include <glm/gtc/matrix_transform.hpp>

#include "engine/logging.hpp"
#include "engine/math.hpp"

#include "gl3_chunk_renderer.hpp"
#include "gl3_debug_renderer.hpp"
#include "graphics.hpp"
#include "menu.hpp"
#include "stopwatch.hpp"

#include "game/world.hpp"

using namespace gui;

GL3Renderer::GL3Renderer(
	Client *client,
	Graphics *graphics,
	SDL_Window *window)
	:
	client(client),
	graphics(graphics),
	conf(*client->getConf()),
	window(window),
	shaderManager(),
	fontTimes(&shaderManager.getFontShader()),
	fontDejavu(&shaderManager.getFontShader()),
	chunkRenderer(client, this, &shaderManager),
	skyRenderer(client, this, &shaderManager, graphics),
	hudRenderer(client, this, &shaderManager, graphics),
	menuRenderer(client, this, &shaderManager, graphics),
	debugRenderer(client, this, &shaderManager, graphics)
{
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
	auto endFog = (conf.render_distance - 1) * Chunk::WIDTH;
	auto startFog = (conf.render_distance - 1) * Chunk::WIDTH * 1 / 2.0;

	defaultShader.setEndFogDistance(endFog);
	defaultShader.setStartFogDistance(startFog);

	blockShader.setEndFogDistance(endFog);
	blockShader.setStartFogDistance(startFog);

	// sky
	GL(GenTextures(1, &skyTex));
	GL(BindTexture(GL_TEXTURE_2D, skyTex));
	GL(TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, graphics->getWidth(), graphics->getHeight(), 0, GL_RGBA, GL_FLOAT, 0));
	GL(TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GL(TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

	GL(GenFramebuffers(1, &skyFbo));
	GL(BindFramebuffer(GL_FRAMEBUFFER, skyFbo));
	GL(FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, skyTex, 0));
	GL(BindFramebuffer(GL_FRAMEBUFFER, 0));

    // font
	fontTimes.load("fonts/times32.fnt");
	fontTimes.setEncoding(Font::Encoding::UTF8);
	fontDejavu.load("fonts/dejavusansmono16.fnt");
	fontDejavu.setEncoding(Font::Encoding::UTF8);

	// gl stuff
	GL(Enable(GL_BLEND));
	GL(BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
	GL(Enable(GL_CULL_FACE));
}

GL3Renderer::~GL3Renderer() {
	LOG(DEBUG, "Destroying GL3 renderer");

	GL(DeleteFramebuffers(1, &skyFbo));
	GL(DeleteTextures(1, &skyTex));
}

void GL3Renderer::resize() {
	makePerspectiveMatrix();
	makeOrthogonalMatrix();
}

void GL3Renderer::makePerspectiveMatrix() {
	double normalRatio = DEFAULT_WINDOWED_RES[0] / (double) DEFAULT_WINDOWED_RES[1];
	double currentRatio = graphics->getWidth() / (double) graphics->getHeight();
	double angle;

	float yfov = conf.fov / normalRatio * TAU / 360.0;
	if (currentRatio > normalRatio)
		angle = atan(tan(yfov / 2) * normalRatio / currentRatio) * 2;
	else
		angle = yfov;

	float zFar = Chunk::WIDTH * sqrt(3 * (conf.render_distance + 1) * (conf.render_distance + 1));
	glm::mat4 perspectiveMatrix = glm::perspective((float) angle,
			(float) currentRatio, ZNEAR, zFar);
	auto &defaultShader = shaderManager.getDefaultShader();
	defaultShader.setProjectionMatrix(perspectiveMatrix);
	auto &blockShader = shaderManager.getBlockShader();
	blockShader.setProjectionMatrix(perspectiveMatrix);
}

void GL3Renderer::makeOrthogonalMatrix() {
	float normalRatio = DEFAULT_WINDOWED_RES[0] / (double) DEFAULT_WINDOWED_RES[1];
	float currentRatio = graphics->getWidth() / (double) graphics->getHeight();
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
	float yfov = conf.fov / ratio * TAU / 360.0;
	if (ratio < 1.0)
		maxFOV = yfov;
	else
		maxFOV = atan(ratio * tan(yfov / 2)) * 2;
}

void GL3Renderer::setConf(const GraphicsConf &conf) {
	auto &defaultShader = shaderManager.getDefaultShader();
	auto &blockShader = shaderManager.getBlockShader();

	GraphicsConf old_conf = this->conf;
	this->conf = conf;

	if (conf.render_distance != old_conf.render_distance) {
		makePerspectiveMatrix();

		auto endFog = (conf.render_distance - 1) * Chunk::WIDTH;
		auto startFog = (conf.render_distance - 1) * Chunk::WIDTH * 1 / 2.0;

		defaultShader.setEndFogDistance(endFog);
		defaultShader.setStartFogDistance(startFog);

		blockShader.setEndFogDistance(endFog);
		blockShader.setStartFogDistance(startFog);
	}

	if (conf.fov != old_conf.fov) {
		makePerspectiveMatrix();
		makeMaxFOV();
	}

	bool fog = conf.fog == Fog::FANCY || conf.fog == Fog::FAST;
	defaultShader.setFogEnabled(fog);
	blockShader.setFogEnabled(fog);
}

void GL3Renderer::tick() {
	render();

	SDL_GL_SwapWindow(window);

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

void GL3Renderer::render() {
	Player &player = client->getWorld()->getPlayer(client->getLocalClientId());

	// render sky
	GL(Disable(GL_DEPTH_TEST));
	GL(DepthMask(false));
	skyRenderer.render();
	if (conf.fog == Fog::FANCY || conf.fog == Fog::FAST) {
		GL(BindFramebuffer(GL_FRAMEBUFFER, skyFbo));
		skyRenderer.render();
		GL(BindFramebuffer(GL_FRAMEBUFFER, 0));
		GL(ActiveTexture(GL_TEXTURE1));
		GL(BindTexture(GL_TEXTURE_2D, skyTex));
	}
	
	// render scene
	GL(Enable(GL_DEPTH_TEST));
	GL(DepthMask(true));
	GL(Clear(GL_DEPTH_BUFFER_BIT));
	chunkRenderer.render();
	renderTarget();

	// render overlay
	GL(Disable(GL_DEPTH_TEST));
	GL(DepthMask(false));
	if (client->getState() == Client::State::PLAYING) {
		hudRenderer.render();
		if (client->isDebugOn())
			debugRenderer.render();
	} else if (client->getState() == Client::State::IN_MENU){
		menuRenderer.render();
	}
}

void GL3Renderer::renderTarget() {
	Player &player = client->getWorld()->getPlayer(client->getLocalClientId());
	if (!player.isValid())
		return;
	// view matrix for scene
	glm::mat4 viewMatrix = glm::rotate(glm::mat4(1.0f), (float) (-player.getPitch() / 360.0 * TAU), glm::vec3(1.0f, 0.0f, 0.0f));
	viewMatrix = glm::rotate(viewMatrix, (float) (-player.getYaw() / 360.0 * TAU), glm::vec3(0.0f, 1.0f, 0.0f));
	viewMatrix = glm::rotate(viewMatrix, (float) (-TAU / 4.0), glm::vec3(1.0f, 0.0f, 0.0f));
	viewMatrix = glm::rotate(viewMatrix, (float) (TAU / 4.0), glm::vec3(0.0f, 0.0f, 1.0f));
	vec3i64 playerPos = player.getPos();
	int64 m = RESOLUTION * Chunk::WIDTH;
	viewMatrix = glm::translate(viewMatrix, glm::vec3(
		(float) -((playerPos[0] % m + m) % m) / RESOLUTION,
		(float) -((playerPos[1] % m + m) % m) / RESOLUTION,
		(float) -((playerPos[2] % m + m) % m) / RESOLUTION)
	);
	auto &defaultShader = shaderManager.getDefaultShader();
	defaultShader.useProgram();
	defaultShader.setViewMatrix(viewMatrix);
	defaultShader.setLightEnabled(false);
	defaultShader.setFogEnabled(false);
	// TODO
}
