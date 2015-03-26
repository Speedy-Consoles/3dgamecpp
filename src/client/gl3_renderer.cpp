#include "gl3_renderer.hpp"

#include <SDL2/SDL.h>
#include <glm/gtc/matrix_transform.hpp>

#include "engine/logging.hpp"
#include "engine/math.hpp"

#include "chunk_renderer.hpp"
#include "graphics.hpp"
#include "menu.hpp"
#include "stopwatch.hpp"

#include "game/world.hpp"

using namespace gui;

GL3Renderer::GL3Renderer(
		Graphics *graphics,
		SDL_Window *window,
		World *world,
		const Menu *menu,
		const ClientState *state,
		const uint8 *localClientID,
		const GraphicsConf &conf,
		Stopwatch *stopwatch) :
		graphics(graphics),
		conf(conf),
		window(window),
		world(world),
		menu(menu),
		state(*state),
		localClientID(*localClientID),
		stopwatch(stopwatch),
		chunkRenderer(world, &shaders, this, localClientID, conf) {
	makeMaxFOV();
	makePerspectiveMatrix();
	makeOrthogonalMatrix();

	// light
	shaders.setAmbientLightColor(ambientColor);
	shaders.setDiffuseLightDirection(diffuseDirection);
	shaders.setDiffuseLightColor(diffuseColor);

	// fog
	shaders.setEndFogDistance((conf.render_distance - 1) * Chunk::WIDTH);
	shaders.setStartFogDistance((conf.render_distance - 1) * Chunk::WIDTH * 1 / 2.0);

	buildCrossHair();
	buildSky();

    // font
    LOG(DEBUG, "Loading font");
    font = new FTGLTextureFont("res/DejaVuSansMono.ttf");
    if (font) {
        font->FaceSize(16);
        font->CharMap(ft_encoding_unicode);
    } else {
        LOG(ERROR, "Could not open 'res/DejaVuSansMono.ttf'");
    }

	// gl stuff
	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_CULL_FACE);
	logOpenGLError();

	// fog fbo and texture
	glGenTextures(1, &skyTexture);
	glBindTexture(GL_TEXTURE_2D, skyTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, graphics->getWidth(), graphics->getHeight(), 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	logOpenGLError();

	glGenFramebuffers(1, &skyFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, skyFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, skyTexture, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	logOpenGLError();
}

GL3Renderer::~GL3Renderer() {
	LOG(DEBUG, "Destroying GL3 renderer");

    delete font;

	glDeleteFramebuffers(1, &skyFBO);
	glDeleteTextures(1, &skyTexture);
}

void GL3Renderer::buildCrossHair() {
	glGenVertexArrays(1, &crossHairVAO);
	glGenBuffers(1, &crossHairVBO);
	glBindVertexArray(crossHairVAO);
	glBindBuffer(GL_ARRAY_BUFFER, crossHairVBO);

	HudVertexData vertexData[12] = {
			// x        y       r     g     b     a
			{{-20.0f,  -2.0f}, {0.0f, 0.0f, 0.0f, 0.5f}},
			{{ 20.0f,  -2.0f}, {0.0f, 0.0f, 0.0f, 0.5f}},
			{{ 20.0f,   2.0f}, {0.0f, 0.0f, 0.0f, 0.5f}},
			{{ 20.0f,   2.0f}, {0.0f, 0.0f, 0.0f, 0.5f}},
			{{-20.0f,   2.0f}, {0.0f, 0.0f, 0.0f, 0.5f}},
			{{-20.0f,  -2.0f}, {0.0f, 0.0f, 0.0f, 0.5f}},
			{{ -2.0f, -20.0f}, {0.0f, 0.0f, 0.0f, 0.5f}},
			{{  2.0f, -20.0f}, {0.0f, 0.0f, 0.0f, 0.5f}},
			{{  2.0f,  20.0f}, {0.0f, 0.0f, 0.0f, 0.5f}},
			{{  2.0f,  20.0f}, {0.0f, 0.0f, 0.0f, 0.5f}},
			{{ -2.0f,  20.0f}, {0.0f, 0.0f, 0.0f, 0.5f}},
			{{ -2.0f, -20.0f}, {0.0f, 0.0f, 0.0f, 0.5f}},
	};

	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 24, 0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 24, (void *) 8);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void GL3Renderer::buildSky() {
	glGenVertexArrays(1, &skyVAO);
	glGenBuffers(1, &skyVBO);
	glBindVertexArray(skyVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyVBO);

	VertexData vertexData[12] = {
			// x      y      z       r            g            b            a
			{{-2.0f, -2.0f,  2.0f}, {fogColor[0], fogColor[1], fogColor[2], 1.0f}},
			{{ 2.0f, -2.0f,  2.0f}, {fogColor[0], fogColor[1], fogColor[2], 1.0f}},
			{{ 2.0f,  0.3f, -1.0f}, {fogColor[0], fogColor[1], fogColor[2], 1.0f}},
			{{ 2.0f,  0.3f, -1.0f}, {fogColor[0], fogColor[1], fogColor[2], 1.0f}},
			{{-2.0f,  0.3f, -1.0f}, {fogColor[0], fogColor[1], fogColor[2], 1.0f}},
			{{-2.0f, -2.0f,  2.0f}, {fogColor[0], fogColor[1], fogColor[2], 1.0f}},

			{{-2.0f,  0.3f, -1.0f}, {fogColor[0], fogColor[1], fogColor[2], 1.0f}},
			{{ 2.0f,  0.3f, -1.0f}, {fogColor[0], fogColor[1], fogColor[2], 1.0f}},
			{{ 2.0f,  2.0f,  2.0f}, {skyColor[0], skyColor[1], skyColor[2], 1.0f}},
			{{ 2.0f,  2.0f,  2.0f}, {skyColor[0], skyColor[1], skyColor[2], 1.0f}},
			{{-2.0f,  2.0f,  2.0f}, {skyColor[0], skyColor[1], skyColor[2], 1.0f}},
			{{-2.0f,  0.3f, -1.0f}, {fogColor[0], fogColor[1], fogColor[2], 1.0f}},
	};

	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 28, 0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 28, (void *) 12);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void GL3Renderer::resize() {
	makePerspectiveMatrix();
	makeOrthogonalMatrix();
	glBindTexture(GL_TEXTURE_2D, skyTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, graphics->getWidth(), graphics->getHeight(), 0, GL_RGBA, GL_FLOAT, 0);
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
	shaders.setProjectionMatrix(perspectiveMatrix);
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
	shaders.setHudProjectionMatrix(hudMatrix);
}

void GL3Renderer::makeMaxFOV() {
	float ratio = (float) DEFAULT_WINDOWED_RES[0] / DEFAULT_WINDOWED_RES[1];
	float yfov = conf.fov / ratio * TAU / 360.0;
	if (ratio < 1.0)
		maxFOV = yfov;
	else
		maxFOV = atan(ratio * tan(yfov / 2)) * 2;
}

void GL3Renderer::setDebug(bool debugActive) {
	this->debugActive = debugActive;
}

bool GL3Renderer::isDebug() {
	return debugActive;
}

void GL3Renderer::setConf(const GraphicsConf &conf) {
	GraphicsConf old_conf = this->conf;
	this->conf = conf;

	if (conf.render_distance != old_conf.render_distance) {
		makePerspectiveMatrix();
		shaders.setEndFogDistance((conf.render_distance - 1) * Chunk::WIDTH);
		shaders.setStartFogDistance((conf.render_distance - 1) * Chunk::WIDTH * 1 / 2.0);
	}

	if (conf.fov != old_conf.fov) {
		makePerspectiveMatrix();
		makeMaxFOV();
	}
}

void GL3Renderer::tick() {
	render();

	SDL_GL_SwapWindow(window);

	if (getCurrentTime() - lastStopWatchSave > millis(200)) {
		lastStopWatchSave = getCurrentTime();
		stopwatch->stop(CLOCK_ALL);
		stopwatch->save();
		stopwatch->start(CLOCK_ALL);
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
    shaders.setLightEnabled(false);
    logOpenGLError();
	renderSky();

    logOpenGLError();
	glBindFramebuffer(GL_FRAMEBUFFER, skyFBO);
	logOpenGLError();
	renderSky();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	logOpenGLError();
	shaders.setLightEnabled(true);

	glClear(GL_DEPTH_BUFFER_BIT);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, skyTexture);
	glActiveTexture(GL_TEXTURE0);
	logOpenGLError();
	chunkRenderer.render();
	renderTarget();
	renderPlayers();

	Player &player = world->getPlayer(localClientID);
	// render overlay
	if (state == PLAYING) {
		renderHud(player);
		//if (debugActive)
		//	renderDebugInfo(player);
	} else if (state == IN_MENU){
		renderMenu();
	}
}

void GL3Renderer::renderHud(const Player &player) {
	glDisable(GL_DEPTH_TEST);
	glDepthMask(false);
	glBindVertexArray(crossHairVAO);
	shaders.prepareProgram(HUD_PROGRAM);
	glDrawArrays(GL_TRIANGLES, 0, 12);

	/*vec2f texs[4];
	texManager.bind(player.getBlock());
	glBindTexture(GL_TEXTURE_2D, texManager.getTexture());
	texManager.getTextureVertices(texs);

	glColor4f(1, 1, 1, 1);

	float d = (graphics->getWidth() < graphics->getHeight() ? graphics->getWidth() : graphics->getHeight()) * 0.05;
	glPushMatrix();
	glTranslatef(-graphics->getDrawWidth() * 0.48, -graphics->getDrawHeight() * 0.48, 0);
	glBegin(GL_QUADS);
		glTexCoord2f(texs[0][0], texs[0][1]); glVertex2f(0, 0);
		glTexCoord2f(texs[1][0], texs[1][1]); glVertex2f(d, 0);
		glTexCoord2f(texs[2][0], texs[2][1]); glVertex2f(d, d);
		glTexCoord2f(texs[3][0], texs[3][1]); glVertex2f(0, d);
	glEnd();
	glPopMatrix();*/
	glDepthMask(true);
	glEnable(GL_DEPTH_TEST);
}

void GL3Renderer::renderSky() {
	glDisable(GL_DEPTH_TEST);
	glDepthMask(false);
	Player &player = world->getPlayer(localClientID);
	if (!player.isValid())
		return;

	glm::mat4 viewMatrix = glm::rotate(glm::mat4(1.0f), (float) (-player.getPitch() / 360.0 * TAU), glm::vec3(1.0f, 0.0f, 0.0f));
	shaders.setModelMatrix(glm::mat4(1.0f));
	shaders.setViewMatrix(viewMatrix);
	shaders.setFogEnabled(false);
	shaders.prepareProgram(DEFAULT_PROGRAM);

	glBindVertexArray(skyVAO);
	glDrawArrays(GL_TRIANGLES, 0, 12);
	shaders.setFogEnabled(true);
	glDepthMask(true);
	glEnable(GL_DEPTH_TEST);
}

void GL3Renderer::renderMenu() {

}

void GL3Renderer::renderTarget() {
	Player &player = world->getPlayer(localClientID);
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
	shaders.setViewMatrix(viewMatrix);
	shaders.prepareProgram(DEFAULT_PROGRAM);
	// TODO
}

void GL3Renderer::renderPlayers() {

}
