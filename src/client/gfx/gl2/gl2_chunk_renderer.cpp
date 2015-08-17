#include "gl2_chunk_renderer.hpp"

#include "shared/engine/logging.hpp"

#include "gl2_renderer.hpp"

static logging::Logger logger("render");

GL2ChunkRenderer::GL2ChunkRenderer(Client *client, GL2Renderer *renderer) :
	ChunkRenderer(client, renderer),
	renderInfos(0, vec3i64HashFunc)
{
	//nothing
}

GL2ChunkRenderer::~GL2ChunkRenderer() {
	//nothing
}

void GL2ChunkRenderer::renderChunk(vec3i64 chunkCoords) {
	auto it = renderInfos.find(chunkCoords);
	if (it == renderInfos.end() || it->second.dl == 0)
		return;

	Player &player = client->getLocalPlayer();
	vec3i64 cd = chunkCoords - player.getChunkPos();

	GL(PushMatrix());
	GL(Translatef(cd[0] * (float) Chunk::WIDTH, cd[1] * (float) Chunk::WIDTH, cd[2] * (float) Chunk::WIDTH))
	GL(CallList(it->second.dl));
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

void GL2ChunkRenderer::finishChunkConstruction(vec3i64 chunkCoords) {
	auto it = renderInfos.find(chunkCoords);
	if (numQuads > 0) {
		if (it == renderInfos.end()) {
			auto pair = renderInfos.insert({chunkCoords, RenderInfo()});
			it = pair.first;
		}
		if (it->second.dl == 0) {
			it->second.dl = glGenLists(1);
		}
		std::sort(&faceIndexBuffer[0], &faceIndexBuffer[numQuads], [](const FaceIndexData &l, const FaceIndexData &r)
		{
			return l.tex < r.tex;
		});

		glNewList(it->second.dl, GL_COMPILE);

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
	} else if (it != renderInfos.end()) {
		if (it->second.dl != 0) {
			GL(DeleteLists(it->second.dl, 1))
		}
		renderInfos.erase(it);
	}
}

void GL2ChunkRenderer::destroyChunkData(vec3i64 chunkCoords) {
	auto it = renderInfos.find(chunkCoords);
	if (it != renderInfos.end()) {
		if (it->second.dl != 0) {
			GL(DeleteLists(it->second.dl, 1))
		}
		renderInfos.erase(it);
	}
}

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
