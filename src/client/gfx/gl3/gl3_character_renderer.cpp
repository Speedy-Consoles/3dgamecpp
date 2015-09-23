#include "gl3_character_renderer.hpp"

#define GLM_FORCE_RADIANS

#include <glm/gtc/matrix_transform.hpp>

#include "shared/engine/logging.hpp"
#include "shared/engine/math.hpp"
#include "client/config.hpp"

#include "gl3_renderer.hpp"

const vec3f GL3CharacterRenderer::CHARACTER_COLOR = {0.6f, 0.0f, 0.0f};
const vec3i GL3CharacterRenderer::HEAD_SIZE = {400, 400, 500};
const vec3i GL3CharacterRenderer::BODY_SIZE = {300, 600, 1450};


GL3CharacterRenderer::GL3CharacterRenderer(Client *client, GL3Renderer *renderer) :
	client(client),
	renderer(renderer)
{
	buildBody();
	buildHead();
}

GL3CharacterRenderer::~GL3CharacterRenderer() {
	GL(DeleteVertexArrays(1, &bodyVao));
	GL(DeleteBuffers(1, &bodyVbo));
	GL(DeleteVertexArrays(1, &headVao));
	GL(DeleteBuffers(1, &headVbo));
}

void GL3CharacterRenderer::buildBody() {
	GL(GenVertexArrays(1, &bodyVao));
	GL(GenBuffers(1, &bodyVbo));
	GL(BindVertexArray(bodyVao));
	GL(BindBuffer(GL_ARRAY_BUFFER, bodyVbo));
	GL(VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 40, 0));
	GL(VertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 40, (void *) 12));
	GL(VertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 40, (void *) 24));
	GL(EnableVertexAttribArray(0));
	GL(EnableVertexAttribArray(1));
	GL(EnableVertexAttribArray(2));
	GL(BindVertexArray(0));

	VertexData vertexData[36];
	int vertexIndices[6] = {0, 1, 2, 0, 2, 3};
	int index = 0;
	for (int d = 0; d < 6; d++) {
		vec3f normal(0.0);
		normal[d % 3] = DIRS[d][d % 3];
		vec3f vertices[4];
		for (int i = 0; i < 4; i++) {
			vertices[i] = vec3f(
				(DIR_QUAD_CORNER_CYCLES_3D[d][i][0] - 0.5f) * BODY_SIZE[0],
				(DIR_QUAD_CORNER_CYCLES_3D[d][i][1] - 0.5f) * BODY_SIZE[1],
				DIR_QUAD_CORNER_CYCLES_3D[d][i][2] * BODY_SIZE[2] - Character::EYE_HEIGHT
			) * (1.0f / RESOLUTION);
		}
		for (int j = 0; j < 6; j++) {
			vertexData[index].xyz[0] = vertices[vertexIndices[j]][0];
			vertexData[index].xyz[1] = vertices[vertexIndices[j]][1];
			vertexData[index].xyz[2] = vertices[vertexIndices[j]][2];
			vertexData[index].nxyz[0] = normal[0];
			vertexData[index].nxyz[1] = normal[1];
			vertexData[index].nxyz[2] = normal[2];
			vertexData[index].rgba[0] = CHARACTER_COLOR[0];
			vertexData[index].rgba[1] = CHARACTER_COLOR[1];
			vertexData[index].rgba[2] = CHARACTER_COLOR[2];
			vertexData[index].rgba[3] = 1.0f;
			index++;
		}
	}

	GL(BufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW));
	GL(BindBuffer(GL_ARRAY_BUFFER, 0));
}

void GL3CharacterRenderer::buildHead() {
	GL(GenVertexArrays(1, &headVao));
	GL(GenBuffers(1, &headVbo));
	GL(BindVertexArray(headVao));
	GL(BindBuffer(GL_ARRAY_BUFFER, headVbo));
	GL(VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 40, 0));
	GL(VertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 40, (void *) 12));
	GL(VertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 40, (void *) 24));
	GL(EnableVertexAttribArray(0));
	GL(EnableVertexAttribArray(1));
	GL(EnableVertexAttribArray(2));
	GL(BindVertexArray(0));

	VertexData vertexData[36];
	int vertexIndices[6] = {0, 1, 2, 0, 2, 3};
	int index = 0;
	for (int d = 0; d < 6; d++) {
		vec3f normal(0.0);
		normal[d % 3] = DIRS[d][d % 3];
		vec3f vertices[4];
		for (int i = 0; i < 4; i++) {
			vertices[i] = vec3f(
				(DIR_QUAD_CORNER_CYCLES_3D[d][i][0] - 0.5f) * HEAD_SIZE[0],
				(DIR_QUAD_CORNER_CYCLES_3D[d][i][1] - 0.5f) * HEAD_SIZE[1],
				(DIR_QUAD_CORNER_CYCLES_3D[d][i][2] - 0.5f) * HEAD_SIZE[2] + HEAD_Z_OFFSET
			) * (1.0f / RESOLUTION);
		}
		for (int j = 0; j < 6; j++) {
			vertexData[index].xyz[0] = vertices[vertexIndices[j]][0];
			vertexData[index].xyz[1] = vertices[vertexIndices[j]][1];
			vertexData[index].xyz[2] = vertices[vertexIndices[j]][2];
			vertexData[index].nxyz[0] = normal[0];
			vertexData[index].nxyz[1] = normal[1];
			vertexData[index].nxyz[2] = normal[2];
			vertexData[index].rgba[0] = CHARACTER_COLOR[0];
			vertexData[index].rgba[1] = CHARACTER_COLOR[1];
			vertexData[index].rgba[2] = CHARACTER_COLOR[2];
			vertexData[index].rgba[3] = 1.0f;
			index++;
		}
	}

	GL(BufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW));
	GL(BindBuffer(GL_ARRAY_BUFFER, 0));
}

void GL3CharacterRenderer::render() {
	Character &localCharacter = client->getLocalCharacter();

		auto &defaultShader = ((GL3Renderer *) renderer)->getShaderManager()->getDefaultShader();

	glm::mat4 viewMatrix = glm::rotate(glm::mat4(1.0f), (float) (-localCharacter.getPitch() / 36000.0f * TAU), glm::vec3(1.0f, 0.0f, 0.0f));
	viewMatrix = glm::rotate(viewMatrix, (float) (-localCharacter.getYaw() / 36000.0f * TAU), glm::vec3(0.0f, 1.0f, 0.0f));
	viewMatrix = glm::rotate(viewMatrix, (float) (-TAU / 4.0), glm::vec3(1.0f, 0.0f, 0.0f));
	viewMatrix = glm::rotate(viewMatrix, (float) (TAU / 4.0), glm::vec3(0.0f, 0.0f, 1.0f));

	defaultShader.setViewMatrix(viewMatrix);
	defaultShader.setLightEnabled(true);
	defaultShader.setFogEnabled(client->getConf().fog != Fog::NONE);

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

		defaultShader.setModelMatrix(modelMatrix);
		defaultShader.useProgram();

		GL(BindVertexArray(bodyVao));
		GL(DrawArrays(GL_TRIANGLES, 0, 144));

		modelMatrix = glm::translate(modelMatrix, glm::vec3(
			0.0f,
			0.0f,
			(float)  HEAD_ANCHOR_Z_OFFSET / RESOLUTION)
		);

		int pitch = clamp(character.getPitch(), PITCH_MIN * 100, PITCH_MAX * 100);
		modelMatrix = glm::rotate(modelMatrix, (float) (-pitch / 36000.0f * TAU), glm::vec3(0.0f, 1.0f, 0.0f));

		defaultShader.setModelMatrix(modelMatrix);
		defaultShader.useProgram();

		GL(BindVertexArray(headVao));
		GL(DrawArrays(GL_TRIANGLES, 0, 144));
		GL(BindVertexArray(0));
	}
}
