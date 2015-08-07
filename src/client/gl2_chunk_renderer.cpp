#include "gl2_chunk_renderer.hpp"

#include "gl2_renderer.hpp"

#include "engine/logging.hpp"

GL2ChunkRenderer::GL2ChunkRenderer(Client *client, GL2Renderer *renderer) :
	client(client), renderer(renderer)
{
	initRenderDistanceDependent();
}

GL2ChunkRenderer::~GL2ChunkRenderer() {
	destroyRenderDistanceDependent();
}

void GL2ChunkRenderer::setConf(const GraphicsConf &conf, const GraphicsConf &old) {
	if (conf.render_distance != old.render_distance) {
		destroyRenderDistanceDependent();
	}

	renderDistance = conf.render_distance;

	if (conf.render_distance != old.render_distance) {
		initRenderDistanceDependent();
	}
}

void GL2ChunkRenderer::render() {
	client->getStopwatch()->start(CLOCK_CHR);
	renderChunks();
	client->getStopwatch()->stop(CLOCK_CHR);

	renderTarget();

	client->getStopwatch()->start(CLOCK_PLA);
	renderPlayers();
	client->getStopwatch()->stop(CLOCK_PLA);
}

void GL2ChunkRenderer::initRenderDistanceDependent() {
	renderDistance = client->getConf().render_distance * 2 + 3;
	int n = renderDistance * renderDistance * renderDistance;

	dlFirstAddress = glGenLists(n);
	dlChunks = new vec3i64[n];
	dlStatus = new uint8[n];

	chunkFaces = new int[n];
	chunkPassThroughs = new uint16[n];

	vsExits = new uint8[n];
	vsVisited = new bool[n];
	vsFringeCapacity = renderDistance * renderDistance * 6;
	vsFringe = new vec3i64[vsFringeCapacity];
	vsIndices = new int[vsFringeCapacity];

	for (int i = 0; i < n; i++) {
		dlStatus[i] = NO_CHUNK;
		chunkFaces[i] = 0;
		chunkPassThroughs[i] = 0;
		vsVisited[i] = false;
	}

	faces = 0;
}

void GL2ChunkRenderer::destroyRenderDistanceDependent() {
	int n = renderDistance * renderDistance * renderDistance;
	GL(DeleteLists(dlFirstAddress, n))
	delete dlChunks;
	delete dlStatus;
	delete chunkFaces;
	delete chunkPassThroughs;
	delete vsExits;
	delete vsVisited;
	delete vsFringe;
	delete vsIndices;
}

void GL2ChunkRenderer::renderChunks() {

	int length = client->getConf().render_distance * 2 + 3;

	vec3i64 ccc;
	while (false/*client->getWorld()->popChangedChunk(&ccc)*/) {
		int index = ((((ccc[2] % length) + length) % length) * length
				+ (((ccc[1] % length) + length) % length)) * length
				+ (((ccc[0] % length) + length) % length);
		if (dlStatus[index] != NO_CHUNK)
			dlStatus[index] = OUTDATED;
	}
	
	Player &player = client->getLocalPlayer();
	vec3i64 pc = player.getChunkPos();
	vec3d lookDir = getVectorFromAngles(player.getYaw(), player.getPitch());

	newFaces = 0;
	newChunks = 0;

	vsFringe[0] = pc;
	vsIndices[0] = ((((pc[2] % length) + length) % length) * length
			+ (((pc[1] % length) + length) % length)) * length
			+ (((pc[0] % length) + length) % length);
	vsExits[vsIndices[0]] = 0x3F;
	vsVisited[vsIndices[0]] = true;
	int fringeSize = 1;
	size_t fringeStart = 0;
	size_t fringeEnd = 1;

	visibleChunks = 0;
	visibleFaces = 0;
	while (fringeSize > 0) {
		visibleChunks++;
		vec3i64 cc = vsFringe[fringeStart];
		vec3i64 cd = cc - pc;
		int index = vsIndices[fringeStart];
		vsVisited[index] = false;
		fringeStart = (fringeStart + 1) % vsFringeCapacity;
		fringeSize--;

		if ((dlStatus[index] != OK || dlChunks[index] != cc) && (newChunks < MAX_NEW_CHUNKS && newFaces < MAX_NEW_QUADS)) {
			Chunk *c = 0;//client->getWorld()->getChunk(cc);
			if (c)
				renderChunk(*c);
		}

		if (dlStatus[index] != NO_CHUNK && dlChunks[index] == cc) {
			if (chunkFaces[index] > 0) {
				visibleFaces += chunkFaces[index];
				client->getStopwatch()->start(CLOCK_DLC);
				GL(PushMatrix());
				GL(Translatef(cd[0] * (int)Chunk::WIDTH, cd[1] * (int)Chunk::WIDTH, cd[2] * (int)Chunk::WIDTH))
				GL(CallList(dlFirstAddress + index));
				GL(PopMatrix());
				client->getStopwatch()->stop(CLOCK_DLC);
			}

			for (int d = 0; d < 6; d++) {
				if ((vsExits[index] & (1 << d)) == 0)
					continue;

				vec3i64 ncc = cc + DIRS[d].cast<int64>();
				vec3i64 ncd = ncc - pc;

				if ((uint) abs(ncd[0]) > client->getConf().render_distance + 1
						|| (uint) abs(ncd[1]) > client->getConf().render_distance + 1
						|| (uint) abs(ncd[2]) > client->getConf().render_distance + 1
						|| !renderer->inFrustum(ncc, player.getPos(), lookDir))
					continue;

				int nIndex = ((((ncc[2] % length) + length) % length) * length
						+ (((ncc[1] % length) + length) % length)) * length
						+ (((ncc[0] % length) + length) % length);

				if (!vsVisited[nIndex]) {
					vsVisited[nIndex] = true;
					vsExits[nIndex] = 0;
					vsFringe[fringeEnd] = ncc;
					vsIndices[fringeEnd] = nIndex;
					fringeEnd = (fringeEnd + 1) % vsFringeCapacity;
					fringeSize++;
				}

				if (dlStatus[nIndex] != NO_CHUNK && dlChunks[nIndex] == ncc) {
					int shift = 0;
					int invD = (d + 3) % 6;
					for (int d1 = 0; d1 < invD; d1++) {
						if (ncd[d1 % 3] * (d1 * (-2) + 5) >= 0)
							vsExits[nIndex] |= ((chunkPassThroughs[nIndex] & (1 << (shift + invD - d1 - 1))) > 0) << d1;
						shift += 5 - d1;
					}
					for (int d2 = invD + 1; d2 < 6; d2++) {
						if (ncd[d2 % 3] * (d2 * (-2) + 5) >= 0)
							vsExits[nIndex] |= ((chunkPassThroughs[nIndex] & (1 << (shift + d2 - invD - 1))) > 0) << d2;
					}
				}
			}
		}
	}
}

void GL2ChunkRenderer::renderChunk(Chunk &c) {
	client->getStopwatch()->start(CLOCK_NDL);
	vec3i64 cc = c.getCC();
	int length = client->getConf().render_distance * 2 + 3;
	uint index = ((((cc[2] % length) + length) % length) * length
			+ (((cc[1] % length) + length) % length)) * length
			+ (((cc[0] % length) + length) % length);

	if (dlStatus[index] != NO_CHUNK)
		faces -= chunkFaces[index];

	chunkFaces[index] = 0;
	dlChunks[index] = cc;
	dlStatus[index] = OK;
	chunkPassThroughs[index] = c.getPassThroughs();

	if (c.getAirBlocks() == Chunk::WIDTH * Chunk::WIDTH * Chunk::WIDTH) {
		GL(NewList(dlFirstAddress + index, GL_COMPILE));
		GL(EndList());
		client->getStopwatch()->stop(CLOCK_NDL);
		return;
	}

	newChunks++;

	vec3ui8 uDirs[3];
	for (uint8 d = 0; d < 3; d++) {
		uDirs[d] = DIRS[d].cast<uint8>();
	}

	const uint8 *blocks = c.getBlocks();
	int n = 0;
	for (uint8 d = 0; d < 3; d++) {
		vec3i64 dir = uDirs[d].cast<int64>();
		uint i = 0;
		uint ni = 0;
		for (int z = (d == 2) ? -1 : 0; z < (int) Chunk::WIDTH; z++) {
			for (int y = (d == 1) ? -1 : 0; y < (int) Chunk::WIDTH; y++) {
				for (int x = (d == 0) ? -1 : 0; x < (int) Chunk::WIDTH; x++) {
					uint8 thatType;
					uint8 thisType;
					if ((x == Chunk::WIDTH - 1 && d==0)
							|| (y == Chunk::WIDTH - 1 && d==1)
							|| (z == Chunk::WIDTH - 1 && d==2)) {
						thatType = 0;//client->getWorld()->getBlock(cc * Chunk::WIDTH + vec3i64(x, y, z) + dir);
						if (thatType != 0) {
							if (x != -1 && y != -1 && z != -1)
								i++;
							continue;
						}
					} else
						thatType = blocks[ni++];

					if (x == -1 || y == -1 || z == -1) {
						thisType = 0;//client->getWorld()->getBlock(cc * Chunk::WIDTH + vec3i64(x, y, z));
						if (thisType != 0)
							continue;
					} else {
						thisType = blocks[i++];
					}

					if((thisType == 0) != (thatType == 0)) {
						vec3i64 faceBlock;
						uint8 faceType;
						uint8 faceDir;
						if (thisType == 0) {
							faceBlock = vec3i64(x, y, z) + dir;
							faceDir = (uint8) (d + 3);
							faceType = thatType;
						} else {
							faceBlock = vec3i64(x, y, z);
							faceDir = d;
							faceType = thisType;
						}

						uint8 corners = 0;
						for (int j = 0; j < 8; ++j) {
							vec3i64 v = DIR_QUAD_EIGHT_NEIGHBOR_CYCLES[faceDir][j].cast<int64>();
							vec3i64 dIcc = faceBlock + v;
							uint8 cornerBlock;
							if (		dIcc[0] < 0 || dIcc[0] >= (int) Chunk::WIDTH
									||	dIcc[1] < 0 || dIcc[1] >= (int) Chunk::WIDTH
									||	dIcc[2] < 0 || dIcc[2] >= (int) Chunk::WIDTH)
								cornerBlock = 0;//client->getWorld()->getBlock(cc * Chunk::WIDTH + dIcc);
							else
								cornerBlock = c.getBlock(dIcc.cast<uint8>());
							if (cornerBlock) {
								corners |= 1 << j;
							}
						}

						vec2f texs[4];
						vec3i64 bc = c.getCC() * c.WIDTH + faceBlock.cast<int64>();
						TextureManager::Entry tex_entry = renderer->getTextureManager()->get(faceType, bc, faceDir);
						TextureManager::getVertices(tex_entry, texs);

						faceIndexBuffer[n] = FaceIndexData{tex_entry.tex, n};
						
						vb[n].normal[0] = DIRS[faceDir][0];
						vb[n].normal[1] = DIRS[faceDir][1];
						vb[n].normal[2] = DIRS[faceDir][2];

						for (int j = 0; j < 4; j++) {
							vb[n].tex[j][0] = texs[j][0];
							vb[n].tex[j][1] = texs[j][1];
							double light = 1.0;
							bool s1 = (corners & QUAD_CORNER_MASK[j][0]) > 0;
							bool s2 = (corners & QUAD_CORNER_MASK[j][2]) > 0;
							bool m = (corners & QUAD_CORNER_MASK[j][1]) > 0;
							if (s1)
								light -= 0.2;
							if (s2)
								light -= 0.2;
							if (m || (s1 && s2))
								light -= 0.2;
							vb[n].color[j][0] = light;
							vb[n].color[j][1] = light;
							vb[n].color[j][2] = light;
							vec3f vertex = (faceBlock.cast<int>() + DIR_QUAD_CORNER_CYCLES_3D[faceDir][j]).cast<float>();
							vb[n].vertex[j][0] = vertex[0];
							vb[n].vertex[j][1] = vertex[1];
							vb[n].vertex[j][2] = vertex[2];
						}
						++n;
					}
				}
			}
		}
	}

	if (n == 0) {
		GL(NewList(dlFirstAddress + index, GL_COMPILE));
		GL(EndList());
		client->getStopwatch()->stop(CLOCK_NDL);
		return;
	}

	std::sort(&faceIndexBuffer[0], &faceIndexBuffer[n], [](const FaceIndexData &l, const FaceIndexData &r)
	{
		return l.tex < r.tex;
	});

	glNewList(dlFirstAddress + index, GL_COMPILE);

	GLuint lastTex = 0;
	for (int facei = 0; facei < n; ++facei) {
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
		chunkFaces[index]++;
		glEnd();
	}

	glEndList();
	logOpenGLError();

	newFaces += chunkFaces[index];
	faces += chunkFaces[index];
	client->getStopwatch()->stop(CLOCK_NDL);
}

void GL2ChunkRenderer::renderTarget() {
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
		glColor4d(0.0, 0.0, 0.0, 1.0);
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
		logOpenGLError();

		GL(Enable(GL_TEXTURE_2D));
	}
}

void GL2ChunkRenderer::renderPlayers() {
	GL(BindTexture(GL_TEXTURE_2D, 0));
	glBegin(GL_QUADS);
	for (uint i = 0; i < MAX_CLIENTS; i++) {
		if (i == client->getLocalClientId())
			continue;
		Player &player = client->getWorld()->getPlayer(i);
		if (!player.isValid())
			continue;
		vec3i64 pos = player.getPos();
		for (int d = 0; d < 6; d++) {
			vec3i8 dir = DIRS[d];
			glColor3d(0.8, 0.2, 0.2);

			glNormal3d(dir[0], dir[1], dir[2]);
			for (int j = 0; j < 4; j++) {
				vec3i off(
					(DIR_QUAD_CORNER_CYCLES_3D[d][j][0] * 2 - 1) * Player::RADIUS,
					(DIR_QUAD_CORNER_CYCLES_3D[d][j][1] * 2 - 1) * Player::RADIUS,
					DIR_QUAD_CORNER_CYCLES_3D[d][j][2] * Player::HEIGHT - Player::EYE_HEIGHT
				);
				glTexCoord2d(QUAD_CORNER_CYCLE[j][0], QUAD_CORNER_CYCLE[j][1]);
				vec3d vertex = (pos + off.cast<int64>()).cast<double>() * (1.0 / RESOLUTION);
				glVertex3d(vertex[0], vertex[1], vertex[2]);
			}
		}
	}
	glEnd();
	logOpenGLError();
}
