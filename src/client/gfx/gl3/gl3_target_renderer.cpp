#include "gl3_target_renderer.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include "shared/engine/logging.hpp"
#include "client/config.hpp"

#include "gl3_renderer.hpp"

GL3TargetRenderer::GL3TargetRenderer(Client *client, GL3Renderer *renderer, ShaderManager *shaderManager) :
	client(client),
	renderer(renderer),
	shaderManager(shaderManager)
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

	VertexData vertexData[144];
	int vertexIndices[6] = {0, 1, 2, 0, 2, 3};
	vec3f pointFive(0.5, 0.5, 0.5);
	int index = 0;
	for (int d = 0; d < 6; d++) {
		for (int i = 0; i < 4; i++) {
			vec3f vertices[4];
			vec3f dirOff = DIRS[d].cast<float>() * 0.0005f;
			vec3f vOff[4];
			vOff[0] = DIR_QUAD_CORNER_CYCLES_3D[d][i].cast<float>() - pointFive;
			vOff[0][OTHER_DIR_DIMS[d][0]] *= 1.001f;
			vOff[0][OTHER_DIR_DIMS[d][1]] *= 1.001f;
			vOff[1] = DIR_QUAD_CORNER_CYCLES_3D[d][i].cast<float>() - pointFive;
			vOff[1][OTHER_DIR_DIMS[d][0]] *= 0.97f;
			vOff[1][OTHER_DIR_DIMS[d][1]] *= 0.97f;
			vOff[2] = DIR_QUAD_CORNER_CYCLES_3D[d][(i + 3) % 4].cast<float>() - pointFive;
			vOff[2][OTHER_DIR_DIMS[d][0]] *= 0.97f;
			vOff[2][OTHER_DIR_DIMS[d][1]] *= 0.97f;
			vOff[3] = DIR_QUAD_CORNER_CYCLES_3D[d][(i + 3) % 4].cast<float>() - pointFive;
			vOff[3][OTHER_DIR_DIMS[d][0]] *= 1.001f;
			vOff[3][OTHER_DIR_DIMS[d][1]] *= 1.001f;

			for (int j = 0; j < 4; j++) {
				vertices[j] = dirOff + vOff[j] + pointFive;
			}

			for (int j = 0; j < 6; j++) {
				vertexData[index].xyz[0] = vertices[vertexIndices[j]][0];
				vertexData[index].xyz[1] = vertices[vertexIndices[j]][1];
				vertexData[index].xyz[2] = vertices[vertexIndices[j]][2];
				vertexData[index].rgba[0] = targetColor[0];
				vertexData[index].rgba[1] = targetColor[1];
				vertexData[index].rgba[2] = targetColor[2];
				vertexData[index].rgba[3] = 1.0f;
				index++;
			}
		}
	}

	GL(BufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW));
	GL(BindBuffer(GL_ARRAY_BUFFER, 0));
}

GL3TargetRenderer::~GL3TargetRenderer() {
	// nothing
}

void GL3TargetRenderer::render() {
	Player &player = client->getLocalPlayer();
	if (!player.isValid())
		return;

	vec3i64 tbc;
	int td;
	bool target = player.getTargetedFace(&tbc, &td);
	if (target) {
		glm::mat4 viewMatrix = glm::rotate(glm::mat4(1.0f), (float) (-player.getPitch() / 360.0 * TAU), glm::vec3(1.0f, 0.0f, 0.0f));
		viewMatrix = glm::rotate(viewMatrix, (float) (-player.getYaw() / 360.0 * TAU), glm::vec3(0.0f, 1.0f, 0.0f));
		viewMatrix = glm::rotate(viewMatrix, (float) (-TAU / 4.0), glm::vec3(1.0f, 0.0f, 0.0f));
		viewMatrix = glm::rotate(viewMatrix, (float) (TAU / 4.0), glm::vec3(0.0f, 0.0f, 1.0f));

		vec3i64 diff = tbc * RESOLUTION - player.getPos();
		glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(
			(float) diff[0] / RESOLUTION,
			(float) diff[1] / RESOLUTION,
			(float) diff[2] / RESOLUTION)
		);

		auto &defaultShader = shaderManager->getDefaultShader();

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
