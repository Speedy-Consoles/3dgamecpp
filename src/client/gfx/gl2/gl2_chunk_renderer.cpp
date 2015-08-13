#include "gl2_chunk_renderer.hpp"

#include "shared/engine/logging.hpp"

#include "gl2_renderer.hpp"

static logging::Logger logger("render");

GL2ChunkRenderer::GL2ChunkRenderer(Client *client, GL2Renderer *renderer) :
	ChunkRenderer(client, renderer)
{
}

GL2ChunkRenderer::~GL2ChunkRenderer() {
	LOG_DEBUG(logger) << "Destroying chunk renderer";
}

void GL2ChunkRenderer::initRenderDistanceDependent(int renderDistance) {
	ChunkRenderer::initRenderDistanceDependent(renderDistance);

	int visibleDiameter = renderDistance * 2 + 1;
	int n = visibleDiameter * visibleDiameter * visibleDiameter;

	dlFirstAddress = glGenLists(n);
}

void GL2ChunkRenderer::destroyRenderDistanceDependent() {
	ChunkRenderer::destroyRenderDistanceDependent();

	int n = renderDistance * renderDistance * renderDistance;
	GL(DeleteLists(dlFirstAddress, n))
}

void GL2ChunkRenderer::renderChunk(size_t index) {
	Player &player = client->getLocalPlayer();
	vec3i64 cd = chunkGrid[index].content - player.getChunkPos();

	GL(PushMatrix());
	GL(Translatef(cd[0] * (float) Chunk::WIDTH, cd[1] * (float) Chunk::WIDTH, cd[2] * (float) Chunk::WIDTH))
	GL(CallList(dlFirstAddress + index));
	GL(PopMatrix());
}

void GL2ChunkRenderer::beginChunkConstruction() {
	numQuads = 0;
}

void GL2ChunkRenderer::emitFace(vec3i64 bc, vec3i64 icc, uint blockType, uint faceDir, int shadowLevels[4]) {
	vec2f texs[4];
	GL2TextureManager::Entry tex_entry = ((GL2Renderer *) renderer)->getTextureManager()->get(blockType, bc, faceDir);
	GL2TextureManager::getTextureCoords(tex_entry.index, tex_entry.type, texs);

	faceIndexBuffer[numQuads] = FaceIndexData{tex_entry.tex, numQuads};

	vb[numQuads].normal[0] = DIRS[faceDir][0];
	vb[numQuads].normal[1] = DIRS[faceDir][1];
	vb[numQuads].normal[2] = DIRS[faceDir][2];

	for (int j = 0; j < 4; j++) {
		vb[numQuads].tex[j][0] = texs[j][0];
		vb[numQuads].tex[j][1] = texs[j][1];
		float light = 1.0f - shadowLevels[j] * 0.2f;
		vb[numQuads].color[j][0] = light;
		vb[numQuads].color[j][1] = light;
		vb[numQuads].color[j][2] = light;
		vec3f vertex = (icc.cast<int>() + DIR_QUAD_CORNER_CYCLES_3D[faceDir][j]).cast<float>();
		vb[numQuads].vertex[j][0] = vertex[0];
		vb[numQuads].vertex[j][1] = vertex[1];
		vb[numQuads].vertex[j][2] = vertex[2];
	}
	++numQuads;
}

void GL2ChunkRenderer::finishChunkConstruction(size_t index) {
	std::sort(&faceIndexBuffer[0], &faceIndexBuffer[numQuads], [](const FaceIndexData &l, const FaceIndexData &r)
	{
		return l.tex < r.tex;
	});

	glNewList(dlFirstAddress + index, GL_COMPILE);

	GLuint lastTex = 0;
	for (int facei = 0; facei < numQuads; ++facei) {
		const FaceIndexData *fid = &faceIndexBuffer[facei];
		const FaceVertexData *fvd = &vb[fid->index];
		if (fid->tex != lastTex) {
			glBindTexture(GL_TEXTURE_2D, fid->tex);
			lastTex = fid->tex;
		}
		glBegin(GL_QUADS);
		glNormal3f(fvd->normal[0], fvd->normal[1], fvd->normal[2]);
		for (int j = 0; j < 4; j++) {
			glTexCoord2f(fvd->tex[j][0], fvd->tex[j][1]);
			glColor3f(fvd->color[j][0], fvd->color[j][1], fvd->color[j][2]);
			glVertex3f(fvd->vertex[j][0], fvd->vertex[j][1], fvd->vertex[j][2]);
		}
		glEnd();
	}

	glEndList();
	LOG_OPENGL_ERROR;
}

//void GL2ChunkRenderer::renderTarget() {
//	Player &player = client->getLocalPlayer();
//	vec3i64 pc = player.getChunkPos();
//
//	vec3i64 tbc;
//	vec3i64 tcc;
//	vec3ui8 ticc;
//	int td;
//	bool target = player.getTargetedFace(&tbc, &td);
//	if (target) {
//		tcc = bc2cc(tbc);
//		ticc = bc2icc(tbc);
//	}
//
//	if (target) {
//		GL(Disable(GL_TEXTURE_2D));
//		glBegin(GL_QUADS);
//		glColor4d(0.0, 0.0, 0.0, 1.0);
//		vec3d pointFive(0.5, 0.5, 0.5);
//		for (int d = 0; d < 6; d++) {
//			glNormal3d(DIRS[d][0], DIRS[d][1], DIRS[d][2]);
//			for (int j = 0; j < 4; j++) {
//				vec3d dirOff = DIRS[d].cast<double>() * 0.0005;
//				vec3d vOff[4];
//				vOff[0] = DIR_QUAD_CORNER_CYCLES_3D[d][j].cast<double>() - pointFive;
//				vOff[0][OTHER_DIR_DIMS[d][0]] *= 1.001;
//				vOff[0][OTHER_DIR_DIMS[d][1]] *= 1.001;
//				vOff[1] = DIR_QUAD_CORNER_CYCLES_3D[d][j].cast<double>() - pointFive;
//				vOff[1][OTHER_DIR_DIMS[d][0]] *= 0.97;
//				vOff[1][OTHER_DIR_DIMS[d][1]] *= 0.97;
//				vOff[2] = DIR_QUAD_CORNER_CYCLES_3D[d][(j + 3) % 4].cast<double>() - pointFive;
//				vOff[2][OTHER_DIR_DIMS[d][0]] *= 0.97;
//				vOff[2][OTHER_DIR_DIMS[d][1]] *= 0.97;
//				vOff[3] = DIR_QUAD_CORNER_CYCLES_3D[d][(j + 3) % 4].cast<double>() - pointFive;
//				vOff[3][OTHER_DIR_DIMS[d][0]] *= 1.001;
//				vOff[3][OTHER_DIR_DIMS[d][1]] *= 1.001;
//
//				for (int k = 0; k < 4; k++) {
//					vec3d vertex = (tbc - pc * Chunk::WIDTH).cast<double>() + dirOff + vOff[k] + pointFive;
//					glVertex3d(vertex[0], vertex[1], vertex[2]);
//				}
//			}
//		}
//		glEnd();
//		LOG_OPENGL_ERROR;
//
//		GL(Enable(GL_TEXTURE_2D));
//	}
//}
//
//void GL2ChunkRenderer::renderPlayers() {
//	GL(BindTexture(GL_TEXTURE_2D, 0));
//	glBegin(GL_QUADS);
//	for (uint i = 0; i < MAX_CLIENTS; i++) {
//		if (i == client->getLocalClientId())
//			continue;
//		Player &player = client->getWorld()->getPlayer(i);
//		if (!player.isValid())
//			continue;
//		vec3i64 pos = player.getPos();
//		for (int d = 0; d < 6; d++) {
//			vec3i8 dir = DIRS[d];
//			glColor3d(0.8, 0.2, 0.2);
//
//			glNormal3d(dir[0], dir[1], dir[2]);
//			for (int j = 0; j < 4; j++) {
//				vec3i off(
//					(DIR_QUAD_CORNER_CYCLES_3D[d][j][0] * 2 - 1) * Player::RADIUS,
//					(DIR_QUAD_CORNER_CYCLES_3D[d][j][1] * 2 - 1) * Player::RADIUS,
//					DIR_QUAD_CORNER_CYCLES_3D[d][j][2] * Player::HEIGHT - Player::EYE_HEIGHT
//				);
//				glTexCoord2d(QUAD_CORNER_CYCLE[j][0], QUAD_CORNER_CYCLE[j][1]);
//				vec3d vertex = (pos + off.cast<int64>()).cast<double>() * (1.0 / RESOLUTION);
//				glVertex3d(vertex[0], vertex[1], vertex[2]);
//			}
//		}
//	}
//	glEnd();
//	LOG_OPENGL_ERROR;
//}
