#include "gl2_target_renderer.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include "shared/engine/logging.hpp"
#include "shared/engine/math.hpp"
#include "client/config.hpp"

#include "gl2_renderer.hpp"

GL2TargetRenderer::GL2TargetRenderer(Client *client, GL2Renderer *renderer) :
	client(client),
	renderer(renderer)
{
	// nothing
}

GL2TargetRenderer::~GL2TargetRenderer() {
	// nothing
}

void GL2TargetRenderer::render() {
	Player &player = client->getLocalPlayer();
	vec3i64 pc = player.getChunkPos();

	vec3i64 tbc;
	vec3i64 tcc;
	vec3ui8 ticc;
	int td;
	bool target = player.getTargetedFace(&tbc, &td);
	if (target) {
		tcc = bc2cc(tbc);
		ticc = bc2icc(tbc);
	}

	if (target) {
		GL(Disable(GL_TEXTURE_2D));
		glBegin(GL_QUADS);
		glColor4d(targetColor[0], targetColor[1], targetColor[2], 1.0);
		vec3d pointFive(0.5, 0.5, 0.5);
		for (int d = 0; d < 6; d++) {
			glNormal3d(DIRS[d][0], DIRS[d][1], DIRS[d][2]);
			for (int j = 0; j < 4; j++) {
				vec3d dirOff = DIRS[d].cast<double>() * 0.0005;
				vec3d vOff[4];
				vOff[0] = DIR_QUAD_CORNER_CYCLES_3D[d][j].cast<double>() - pointFive;
				vOff[0][OTHER_DIR_DIMS[d][0]] *= 1.001;
				vOff[0][OTHER_DIR_DIMS[d][1]] *= 1.001;
				vOff[1] = DIR_QUAD_CORNER_CYCLES_3D[d][j].cast<double>() - pointFive;
				vOff[1][OTHER_DIR_DIMS[d][0]] *= 0.97;
				vOff[1][OTHER_DIR_DIMS[d][1]] *= 0.97;
				vOff[2] = DIR_QUAD_CORNER_CYCLES_3D[d][(j + 3) % 4].cast<double>() - pointFive;
				vOff[2][OTHER_DIR_DIMS[d][0]] *= 0.97;
				vOff[2][OTHER_DIR_DIMS[d][1]] *= 0.97;
				vOff[3] = DIR_QUAD_CORNER_CYCLES_3D[d][(j + 3) % 4].cast<double>() - pointFive;
				vOff[3][OTHER_DIR_DIMS[d][0]] *= 1.001;
				vOff[3][OTHER_DIR_DIMS[d][1]] *= 1.001;

				for (int k = 0; k < 4; k++) {
					vec3d vertex = (tbc - pc * Chunk::WIDTH).cast<double>() + dirOff + vOff[k] + pointFive;
					glVertex3d(vertex[0], vertex[1], vertex[2]);
				}
			}
		}
		glEnd();
		LOG_OPENGL_ERROR;

		GL(Enable(GL_TEXTURE_2D));
	}
}
