#include "gl3_sky_renderer.hpp"

#include "client/gl3_renderer.hpp"

#include "engine/logging.hpp"

#include <glm/gtc/matrix_transform.hpp>

GL3SkyRenderer::GL3SkyRenderer(Client *client, GL3Renderer *renderer, ShaderManager *shaderManager, Graphics *graphics) :
	client(client),
	renderer(renderer),
	shaderManager(shaderManager),
	graphics(graphics)
{
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

GL3SkyRenderer::~GL3SkyRenderer() {
	glDeleteFramebuffers(1, &skyFBO);
	glDeleteTextures(1, &skyTexture);
}

void GL3SkyRenderer::render() {
    logOpenGLError();
	renderSky();

    logOpenGLError();
	glBindFramebuffer(GL_FRAMEBUFFER, skyFBO);
	logOpenGLError();
	renderSky();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	logOpenGLError();

	glClear(GL_DEPTH_BUFFER_BIT);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, skyTexture);
	glActiveTexture(GL_TEXTURE0);
}

void GL3SkyRenderer::renderSky() {
	glDisable(GL_DEPTH_TEST);
	glDepthMask(false);
	Player &player = client->getWorld()->getPlayer(client->getLocalClientId());
	if (!player.isValid())
		return;

	glm::mat4 viewMatrix = glm::rotate(glm::mat4(1.0f), (float) (-player.getPitch() / 360.0 * TAU), glm::vec3(1.0f, 0.0f, 0.0f));
	auto &defaultShader = shaderManager->getDefaultShader();
	defaultShader.setModelMatrix(glm::mat4(1.0f));
	defaultShader.setViewMatrix(viewMatrix);
	defaultShader.setFogEnabled(false);
    defaultShader.setLightEnabled(false);
	defaultShader.useProgram();

	glBindVertexArray(skyVAO);
	glDrawArrays(GL_TRIANGLES, 0, 12);
	defaultShader.setFogEnabled(true);
	defaultShader.setLightEnabled(true);
	glDepthMask(true);
	glEnable(GL_DEPTH_TEST);
}