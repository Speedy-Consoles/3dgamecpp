#include "gl3_character_renderer.hpp"

#define GLM_FORCE_RADIANS

#include <glm/gtc/matrix_transform.hpp>

#include "shared/engine/logging.hpp"
#include "shared/engine/math.hpp"
#include "client/config.hpp"

#include "gl3_renderer.hpp"

GL3CharacterRenderer::GL3CharacterRenderer(Client *client, GL3Renderer *renderer) :
	client(client),
	renderer(renderer)
{
	GL(GenVertexArrays(1, &vao));
	GL(GenBuffers(1, &vbo));
	GL(BindVertexArray(vao));
	GL(BindBuffer(GL_ARRAY_BUFFER, vbo));
	GL(VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 28, 0));
	GL(VertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 28, (void *) 12));
	GL(EnableVertexAttribArray(0));
	GL(EnableVertexAttribArray(1));
	GL(BindVertexArray(0));

	VertexData vertexData[36];
	int vertexIndices[6] = {0, 1, 2, 0, 2, 3};
	int index = 0;
	for (int d = 0; d < 6; d++) {
		vec3f vertices[4];
		for (int i = 0; i < 4; i++) {
			vertices[i] = vec3f(
				(float)((DIR_QUAD_CORNER_CYCLES_3D[d][i][0] * 2 - 1) * Character::RADIUS),
				(float)((DIR_QUAD_CORNER_CYCLES_3D[d][i][1] * 2 - 1) * Character::RADIUS),
				(float)(DIR_QUAD_CORNER_CYCLES_3D[d][i][2] * Character::HEIGHT - Character::EYE_HEIGHT)
			) * (1.0f / RESOLUTION);
		}
		for (int j = 0; j < 6; j++) {
			vertexData[index].xyz[0] = vertices[vertexIndices[j]][0];
			vertexData[index].xyz[1] = vertices[vertexIndices[j]][1];
			vertexData[index].xyz[2] = vertices[vertexIndices[j]][2];
			vertexData[index].rgba[0] = characterColor[0];
			vertexData[index].rgba[1] = characterColor[1];
			vertexData[index].rgba[2] = characterColor[2];
			vertexData[index].rgba[3] = 1.0f;
			index++;
		}
	}

	GL(BufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW));
	GL(BindBuffer(GL_ARRAY_BUFFER, 0));
}

GL3CharacterRenderer::~GL3CharacterRenderer() {
	GL(DeleteVertexArrays(1, &vao));
	GL(DeleteBuffers(1, &vbo));
}

void GL3CharacterRenderer::render() {
	Character &localCharacter = client->getLocalCharacter();

	glm::mat4 viewMatrix = glm::rotate(glm::mat4(1.0f), (float) (-localCharacter.getPitch() / 36000.0f * TAU), glm::vec3(1.0f, 0.0f, 0.0f));
	viewMatrix = glm::rotate(viewMatrix, (float) (-localCharacter.getYaw() / 36000.0f * TAU), glm::vec3(0.0f, 1.0f, 0.0f));
	viewMatrix = glm::rotate(viewMatrix, (float) (-TAU / 4.0), glm::vec3(1.0f, 0.0f, 0.0f));
	viewMatrix = glm::rotate(viewMatrix, (float) (TAU / 4.0), glm::vec3(0.0f, 0.0f, 1.0f));

	for (uint i = 0; i < MAX_CLIENTS; i++) {
		if (i == client->getLocalClientId())
			continue;
		Character &character = client->getWorld()->getCharacter(i);
		if (!character.isValid())
			continue;
		vec3i64 pDiff = character.getPos() - localCharacter.getPos();
		glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(
			(float) pDiff[0] / RESOLUTION,
			(float) pDiff[1] / RESOLUTION,
			(float) pDiff[2] / RESOLUTION)
		);

		modelMatrix = glm::rotate(modelMatrix, (float) (character.getYaw() / 36000.0f * TAU), glm::vec3(0.0f, 0.0f, 1.0f));
		modelMatrix = glm::rotate(modelMatrix, (float) (-character.getPitch() / 36000.0f * TAU), glm::vec3(0.0f, 1.0f, 0.0f));


		auto &defaultShader = ((GL3Renderer *) renderer)->getShaderManager()->getDefaultShader();

		defaultShader.setViewMatrix(viewMatrix);
		defaultShader.setModelMatrix(modelMatrix);
		defaultShader.setLightEnabled(true);
		defaultShader.setFogEnabled(client->getConf().fog != Fog::NONE);
		defaultShader.useProgram();

		GL(BindVertexArray(vao));
		GL(DrawArrays(GL_TRIANGLES, 0, 144));
		GL(BindVertexArray(0));
	}
}
