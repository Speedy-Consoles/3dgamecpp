#include "chunk_renderer.hpp"

#include "shared/engine/logging.hpp"
#include "shared/engine/stopwatch.hpp"
#include "shared/game/player.hpp"
#include "shared/block_utils.hpp"
#include "shared/chunk_manager.hpp"
#include "shared/constants.hpp"
#include "client/client.hpp"
#include "client/config.hpp"

#include "renderer.hpp"

using namespace std;

static logging::Logger logger("render");

ChunkRenderer::ChunkRenderer(Client *client, Renderer *renderer) :
		inBuildQueue(0, vec3i64HashFunc),
		client(client),
		renderer(renderer) {
}

ChunkRenderer::~ChunkRenderer() {
	destroyRenderDistanceDependent();
}

void ChunkRenderer::init() {
	initRenderDistanceDependent(client->getConf().render_distance);
}

void ChunkRenderer::setConf(const GraphicsConf &conf, const GraphicsConf &old) {
	if (conf.render_distance != old.render_distance) {
		destroyRenderDistanceDependent();
		initRenderDistanceDependent(conf.render_distance);
		checkChunkIndex = 0;
		faces = 0;
		vsCounter++;
	}
}

void ChunkRenderer::tick() {
	Player &player = client->getLocalPlayer();
	if (!player.isValid())
		return;

	int visibleDiameter = renderDistance * 2 + 1;

	// put chunks into render queue
	vec3i64 pc = player.getChunkPos();
	if (pc != oldPlayerChunk) {
		double pcDist = (pc - oldPlayerChunk).norm();
		double newRadius = LOADING_ORDER[checkChunkIndex].norm() - pcDist;
		if (newRadius < 0)
			checkChunkIndex = 0;
		else
			checkChunkIndex = LOADING_ORDER_DISTANCE_INDICES[(int) newRadius];
		oldPlayerChunk = pc;

		vsCounter++;
		visibilitySearch(pc);
	}
	while (LOADING_ORDER[checkChunkIndex].norm() <= client->getConf().render_distance
			&& buildQueue.size() < MAX_RENDER_QUEUE_SIZE) {
		vec3i64 cc = pc + LOADING_ORDER[checkChunkIndex].cast<int64>();
		int64 index = gridCycleIndex(cc, visibleDiameter);
		if (chunkGrid[index].status != OK || chunkGrid[index].content != cc) {
			auto it = inBuildQueue.find(cc);
			if (it == inBuildQueue.end()) {
				inBuildQueue.insert(cc);
				buildQueue.push_back(cc);
				for (size_t i = 0; i < 27; ++i) {
					client->getChunkManager()->requestChunk(cc + BIG_CUBE_CYCLE[i].cast<int64>());
				}
			}
		}
		checkChunkIndex++;
	}

	client->getStopwatch()->start(CLOCK_IRQ);
	// build chunks in render queue
	newFaces = 0;
	newChunks = 0;
	while (!buildQueue.empty() && newChunks < MAX_NEW_CHUNKS && newFaces < MAX_NEW_FACES) {
		vec3i64 cc = buildQueue.front();
		Chunk const *chunks[27];
		bool cantRender = false;
		for (int i = 0; i < 27; ++i) {
			chunks[i] = client->getChunkManager()->getChunk(cc + BIG_CUBE_CYCLE[i].cast<int64>());
			if (!chunks[i]) {
				cantRender = true;
				break;
			}
		}
		if (cantRender)
			break;
		client->getStopwatch()->start(CLOCK_BCH);
		buildChunk(chunks);
		client->getStopwatch()->stop(CLOCK_BCH);
		visibilitySearch(cc);
		for (int i = 0; i < 27; ++i) {
			client->getChunkManager()->releaseChunk(chunks[i]->getCC());
		}
		buildQueue.pop_front();
		inBuildQueue.erase(inBuildQueue.find(cc));
	}
	client->getStopwatch()->stop(CLOCK_IRQ);
}

void ChunkRenderer::render() {
	// render chunks
	Player &player = client->getLocalPlayer();
	if (!player.isValid())
		return;

	beginRender();

	int visibleDiameter = renderDistance * 2 + 1;
	vec3d lookDir = getVectorFromAngles(player.getYaw(), player.getPitch());
	vec3i64 pc = player.getChunkPos();

	visibleChunks = 0;
	visibleFaces = 0;

	for (int i = 0; i < visibleDiameter * visibleDiameter * visibleDiameter; i++) {
		if (chunkGrid[i].status != NO_CHUNK
				&& (chunkGrid[i].content - pc).norm() <= client->getConf().render_distance
				&& chunkGrid[i].vsInsVersion == vsCounter
				&& chunkGrid[i].vsIns > 0
				&& chunkGrid[i].numFaces > 0
				&& inFrustum(chunkGrid[i].content, player.getPos(), lookDir)) {
			renderChunk(i);
			visibleChunks++;
			visibleFaces += chunkGrid[i].numFaces;
		}
	}

	finishRender();
}

void ChunkRenderer::rebuildChunk(vec3i64 chunkCoords) {
	int visibleDiameter = renderDistance * 2 + 1;
	int64 index = gridCycleIndex(chunkCoords, visibleDiameter);
	if (chunkGrid[index].status != OK || chunkGrid[index].content != chunkCoords)
		return;
	chunkGrid[index].status = OUTDATED;
	inBuildQueue.insert(chunkCoords);
	buildQueue.push_front(chunkCoords);
	for (size_t i = 0; i < 27; ++i) {
		client->getChunkManager()->requestChunk(chunkCoords + BIG_CUBE_CYCLE[i].cast<int64>());
	}
}

ChunkRendererDebugInfo ChunkRenderer::getDebugInfo() {
	ChunkRendererDebugInfo info;
	info.checkedDistance = (int) LOADING_ORDER[checkChunkIndex].norm();
	info.newFaces = newFaces;
	info.newChunks = newChunks;
	info.totalFaces = faces;
	info.visibleChunks = visibleChunks;
	info.visibleFaces = visibleFaces;
	info.buildQueueSize = buildQueue.size();

	return info;
}

void ChunkRenderer::buildChunk(Chunk const *chunks[27]) {
	beginChunkConstruction();

	const Chunk &chunk = *(chunks[BIG_CUBE_CYCLE_BASE_INDEX]);
	vec3i64 cc = chunk.getCC();

	int visibleDiameter = renderDistance * 2 + 1;
	size_t index = gridCycleIndex(cc, visibleDiameter);

	if (chunkGrid[index].status != NO_CHUNK)
		faces -= chunkGrid[index].numFaces;

	chunkGrid[index].numFaces = 0;
	chunkGrid[index].content = cc;
	chunkGrid[index].status = OK;
	chunkGrid[index].passThroughs = chunk.getPassThroughs();

	// skip air chunks and earth chunks
	bool skip = false;
	if (chunk.getAirBlocks() == Chunk::WIDTH * Chunk::WIDTH * Chunk::WIDTH) {
		skip = true;
	} else if (chunk.getAirBlocks() == 0) {
		skip = true;
		for (int i = 0; i < 27; ++i) {
			if (BIG_CUBE_CYCLE[i].norm() <= 1 && chunks[i]->getAirBlocks() != 0) {
				skip = false;
				break;
			}
		}
	}

	if (skip) {
		finishChunkConstruction(index);
		return;
	}

	newChunks++;

	vec3ui8 uDirs[3];
	for (uint8 d = 0; d < 3; d++) {
		uDirs[d] = DIRS[d].cast<uint8>();
	}

	const uint8 *blocks = chunk.getBlocks();
	for (uint8 d = 0; d < 3; d++) {
		vec3i64 dir = uDirs[d].cast<int64>();
		uint dimFlipIndexDiff = (uint) (((dir[2] * Chunk::WIDTH + dir[1]) * Chunk::WIDTH + dir[0]) * (Chunk::WIDTH - 1));
		uint i = 0;
		uint ni = 0;
		for (int z = (d == 2) ? -1 : 0; z < (int) Chunk::WIDTH; z++) {
			for (int y = (d == 1) ? -1 : 0; y < (int) Chunk::WIDTH; y++) {
				for (int x = (d == 0) ? -1 : 0; x < (int) Chunk::WIDTH; x++) {
					uint8 thatType;
					uint8 thisType;
					bool thisOutside = x == -1 || y == -1 || z == -1;
					bool thatOutside = !thisOutside
							&& ((x == Chunk::WIDTH - 1 && d==0)
							|| (y == Chunk::WIDTH - 1 && d==1)
							|| (z == Chunk::WIDTH - 1 && d==2));
					if (thatOutside) {
						const Chunk &otherChunk = *chunks[DIR_TO_BIG_CUBE_CYCLE_INDEX[d]];
						thatType = otherChunk.getBlocks()[i - dimFlipIndexDiff];
						if (thatType != 0) {
							if (!thisOutside)
								i++;
							continue;
						}
					} else
						thatType = blocks[ni++];

					if (thisOutside) {
						const Chunk &otherChunk = *chunks[DIR_TO_BIG_CUBE_CYCLE_INDEX[d + 3]];
						thisType = otherChunk.getBlocks()[ni - 1 + dimFlipIndexDiff];
						if (thisType != 0)
							continue;
					} else {
						thisType = blocks[i++];
					}

					if ((thisType == 0) != (thatType == 0)) {
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
							vec3i8 ncc(0, 0, 0);
							for (int i = 0; i < 3; i++) {
								if (dIcc[i] < 0) {
									ncc[i] = -1;
									dIcc[i] += Chunk::WIDTH;
								} else if (dIcc[i] >= (int) Chunk::WIDTH) {
									ncc[i] = 1;
									dIcc[i] -= Chunk::WIDTH;
								}
							}
							const Chunk &otherChunk = *chunks[vec2BigCubeCycleIndex(ncc)];
							uint8 cornerBlock = otherChunk.getBlock(dIcc.cast<uint8>());
							if (cornerBlock != 0) {
								corners |= 1 << j;
							}
						}
						vec3i64 bc = chunk.getCC() * chunk.WIDTH + faceBlock;

						int shadowLevels[4];
						for (int j = 0; j < 4; j++) {
							shadowLevels[j] = 0;
							bool s1 = (corners & QUAD_CORNER_MASK[j][0]) > 0;
							bool s2 = (corners & QUAD_CORNER_MASK[j][2]) > 0;
							bool m = (corners & QUAD_CORNER_MASK[j][1]) > 0;
							if (s1)
								shadowLevels[j]++;
							if (s2)
								shadowLevels[j]++;
							if (m || (s1 && s2))
								shadowLevels[j]++;
						}

						emitFace(bc, faceBlock, faceType, faceDir, shadowLevels);
						chunkGrid[index].numFaces += 2;
					}
				}
			}
		}
	}

	finishChunkConstruction(index);
	newFaces += chunkGrid[index].numFaces;
	faces += chunkGrid[index].numFaces;
}

void ChunkRenderer::visibilitySearch(vec3i64 startChunkCoords) {
	Player &player = client->getLocalPlayer();
	if (!player.isValid())
		return;

	int visibleDiameter = renderDistance * 2 + 1;
	int startIndex = (int) gridCycleIndex(startChunkCoords, visibleDiameter);

	vec3i64 pc = player.getChunkPos();

	int fringeSize = 0;
	size_t fringeStart = 0;
	size_t fringeEnd = 0;
	if (startChunkCoords == pc) {
		chunkGrid[startIndex].vsOuts = 0x3F;
		chunkGrid[startIndex].vsOutsVersion = vsCounter;
		chunkGrid[startIndex].vsIns = 0x3F;
		chunkGrid[startIndex].vsInsVersion = vsCounter;

		for (int d = 0; d < 6; d++) {
			vec3i64 ncc = startChunkCoords + DIRS[d].cast<int64>();
			int nIndex = gridCycleIndex(ncc, visibleDiameter);
			vsFringe[fringeEnd] = ncc;
			vsIndices[fringeEnd] = nIndex;
			chunkGrid[nIndex].vsIns = (1 << ((d + 3) % 6));
			chunkGrid[nIndex].vsInsVersion = vsCounter;
			chunkGrid[nIndex].vsInFringe = true;
			fringeEnd = (fringeEnd + 1) % vsFringeCapacity;
			fringeSize++;
		}
	} else if (chunkGrid[startIndex].vsIns > 0 && chunkGrid[startIndex].vsInsVersion == vsCounter) {
		vsFringe[fringeEnd] = startChunkCoords;
		vsIndices[fringeEnd] = startIndex;
		chunkGrid[startIndex].vsInFringe = true;
		fringeEnd = (fringeEnd + 1) % vsFringeCapacity;
		fringeSize++;
	} else {
		return;
	}

	while (fringeSize > 0) {
		vec3i64 cc = vsFringe[fringeStart];
		int index = vsIndices[fringeStart];
		fringeStart = (fringeStart + 1) % vsFringeCapacity;
		fringeSize--;
		chunkGrid[index].vsInFringe = false;
		int changedOuts = updateVsChunk(index, cc);

		for (int d = 0; d < 6; d++) {
			if (((changedOuts >> d) & 1) == 0)
				continue;

			vec3i64 ncc = cc + DIRS[d].cast<int64>();

			if ((ncc - pc).norm() > client->getConf().render_distance)
				continue;

			int nIndex = gridCycleIndex(ncc, visibleDiameter);

			if (chunkGrid[nIndex].vsInsVersion != vsCounter) {
				chunkGrid[nIndex].vsIns = 0;
				chunkGrid[nIndex].vsInsVersion = vsCounter;
			}
			chunkGrid[nIndex].vsIns |= (1 << ((d + 3) % 6));

			if (chunkGrid[nIndex].status == NO_CHUNK || chunkGrid[nIndex].content != ncc)
				continue;

			if (chunkGrid[nIndex].vsInFringe == false) {
				chunkGrid[nIndex].vsInFringe = true;
				vsFringe[fringeEnd] = ncc;
				vsIndices[fringeEnd] = nIndex;
				fringeEnd = (fringeEnd + 1) % vsFringeCapacity;
				fringeSize++;
			}
		}
	}
}

int ChunkRenderer::updateVsChunk(size_t index, vec3i64 chunkCoords) {
	int passThroughs = 0x3FFF;
	if (chunkGrid[index].status != NO_CHUNK && chunkGrid[index].content == chunkCoords)
		passThroughs = chunkGrid[index].passThroughs;

	vec3i64 cd = chunkCoords - client->getLocalPlayer().getChunkPos();
	int dirMask = 0;
	for (int dim = 0; dim < 3; dim++) {
		if (cd[dim] >= 0)
			dirMask |= 1 << dim;
		if (cd[dim] <= 0)
			dirMask |= 8 << dim;
	}

	int oldOuts = chunkGrid[index].vsOuts;
	if (chunkGrid[index].vsOutsVersion != vsCounter)
		oldOuts = 0;
	chunkGrid[index].vsOuts = getOuts(chunkGrid[index].vsIns, passThroughs) & dirMask;
	chunkGrid[index].vsOutsVersion = vsCounter;

	return oldOuts ^ chunkGrid[index].vsOuts;
}

int ChunkRenderer::getOuts(int ins, int passThroughs) {
	int outs = 0;
	for (int d = 0; d < 6; d++) {
		if (((ins >> d) & 1) == 0)
			continue;
		int shift = 0;
		for (int d1 = 0; d1 < d; d1++) {
			outs |= ((passThroughs & (1 << (shift + d - d1 - 1))) > 0) << d1;
			shift += 5 - d1;
		}
		for (int d2 = d + 1; d2 < 6; d2++) {
			outs |= ((passThroughs & (1 << (shift + d2 - d - 1))) > 0) << d2;
		}
	}
	return outs;
}

bool ChunkRenderer::inFrustum(vec3i64 cc, vec3i64 pos, vec3d lookDir) {
	double chunkDia = sqrt(3) * Chunk::WIDTH * RESOLUTION;
	vec3d cp = (cc * Chunk::WIDTH * RESOLUTION - pos).cast<double>();
	double chunkLookDist = lookDir * cp + chunkDia;
	if (chunkLookDist < 0)
		return false;
	vec3d orthoChunkPos = cp - lookDir * chunkLookDist;
	double orthoChunkDist = max(0.0, orthoChunkPos.norm() - chunkDia);
	return atan(orthoChunkDist / chunkLookDist) <= renderer->getMaxFOV() / 2;
}

void ChunkRenderer::initRenderDistanceDependent(int renderDistance) {
	int visibleDiameter = renderDistance * 2 + 1;
	int n = visibleDiameter * visibleDiameter * visibleDiameter;
	chunkGrid = new GridInfo[n];
	vsFringeCapacity = visibleDiameter * visibleDiameter * 6;
	vsFringe = new vec3i64[vsFringeCapacity];
	vsIndices = new int[vsFringeCapacity];
	for (int i = 0; i < n; i++) {
		chunkGrid[i].status = NO_CHUNK;
		chunkGrid[i].numFaces = 0;
		chunkGrid[i].passThroughs = 0;
		chunkGrid[i].vsInsVersion = 0;
		chunkGrid[i].vsOutsVersion = 0;
		chunkGrid[i].vsInFringe = false;
	}

	this->renderDistance = renderDistance;
}

void ChunkRenderer::destroyRenderDistanceDependent() {
	delete[] chunkGrid;
	delete[] vsFringe;
	delete[] vsIndices;
}
