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
		builtChunks(0, vec3i64HashFunc),
		vsChunks(0, vec3i64HashFunc),
		vsInFringe(0, vec3i64HashFunc),
		client(client),
		renderer(renderer) {
	renderChunks[0] = std::map<vec3i64, vec3i64, bool(*)(vec3i64, vec3i64)>(vec3i64CompFunc);
	renderChunks[1] = std::map<vec3i64, vec3i64, bool(*)(vec3i64, vec3i64)>(vec3i64CompFunc);
	renderDistance = client->getConf().render_distance;
}

ChunkRenderer::~ChunkRenderer() {
	// nothing
}

void ChunkRenderer::setConf(const GraphicsConf &conf, const GraphicsConf &old) {
	if (conf.render_distance != old.render_distance) {
		checkChunkIndex = 0; // TODO make smarter
		this->renderDistance = conf.render_distance;
	}
}

void ChunkRenderer::tick() {
	Player &player = client->getLocalPlayer();
	if (!player.isValid())
		return;

	client->getStopwatch()->start(CLOCK_CRT);

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

		for (auto it = builtChunks.begin(); it != builtChunks.end();) {
			if ((it->first - pc).norm() > renderDistance) {
				destroyChunkData(it->first);
				it = builtChunks.erase(it);
			} else {
				++it;
			}
		}
	}
	while (LOADING_ORDER[checkChunkIndex].norm() <= renderDistance
			&& buildQueue.size() < MAX_RENDER_QUEUE_SIZE) {
		vec3i64 cc = pc + LOADING_ORDER[checkChunkIndex].cast<int64>();
		auto it = builtChunks.find(cc);
		if (it == builtChunks.end()) {
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

	client->getStopwatch()->start(CLOCK_IBQ);
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
		changedChunksQueue.push_back(cc);
		for (int i = 0; i < 27; ++i) {
			client->getChunkManager()->releaseChunk(chunks[i]->getCC());
		}
		buildQueue.pop_front();
		inBuildQueue.erase(inBuildQueue.find(cc));
	}
	client->getStopwatch()->stop(CLOCK_IBQ);

	client->getStopwatch()->start(CLOCK_VS);
	visibilitySearch();
	client->getStopwatch()->stop(CLOCK_VS);

	client->getStopwatch()->stop(CLOCK_CRT);
}

void ChunkRenderer::render() {
	// render chunks
	Player &player = client->getLocalPlayer();
	if (!player.isValid())
		return;

	client->getStopwatch()->start(CLOCK_CRR);

	beginRender();

	vec3d lookDir = getVectorFromAngles(player.getYaw(), player.getPitch());

	visibleChunks = 0;
	visibleFaces = 0;

	for (auto renderIt = renderChunks[renderChunksPage].begin(); renderIt != renderChunks[renderChunksPage].end(); ++renderIt) {
		vec3i64 cc = renderIt->second;
		auto builtIt = builtChunks.find(cc);
		if (builtIt != builtChunks.end()
				&& (cc - player.getChunkPos()).norm() <= renderDistance
				&& inFrustum(cc, player.getPos(), lookDir)){
			renderChunk(cc);
			visibleChunks++;
			visibleFaces += builtIt->second.numFaces;
		}
	}

	finishRender();
	
	client->getStopwatch()->stop(CLOCK_CRR);
}

void ChunkRenderer::rebuildChunk(vec3i64 chunkCoords) {
	auto it = builtChunks.find(chunkCoords);
	if (it == builtChunks.end() || it->second.outDated)
		return;
	it->second.outDated = true;
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

	auto it = builtChunks.find(cc);

	if (it == builtChunks.end()) {
		auto pair = builtChunks.insert({cc, ChunkBuildInfo()});
		it = pair.first;
	} else {
		faces -= it->second.numFaces;
	}

	it->second.numFaces = 0;
	it->second.outDated = false;
	it->second.passThroughs = chunk.getPassThroughs();

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
		finishChunkConstruction(cc);
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
						it->second.numFaces += 2;
					}
				}
			}
		}
	}

	finishChunkConstruction(cc);
	newFaces += it->second.numFaces;
	faces += it->second.numFaces;
}

// TODO less map find calls
void ChunkRenderer::visibilitySearch() {
	Player &player = client->getLocalPlayer();
	if (!player.isValid())
		return;

	vec3i64 pc = player.getChunkPos();

	int traversedChunks = 0;
	while ((
				!vsFringe.empty()
				|| pc != vsPlayerChunk
				|| vsRenderDistance != renderDistance
				|| !changedChunksQueue.empty()
				|| vsCurrentVersion == 0)
			&& traversedChunks < MAX_VS_CHUNKS) {
		vec3i64 startChunkCoords;
		if (vsFringe.empty()) {
			if (pc != vsPlayerChunk
					|| vsCurrentVersion == 0
					|| vsRenderDistance != renderDistance) {
				for (auto it = vsChunks.begin(); it != vsChunks.end();) {
					if ((it->first - vsPlayerChunk).norm() > renderDistance) {
						it = vsChunks.erase(it);
					} else {
						++it;
					}
				}
				vsPlayerChunk = pc;
				vsRenderDistance = renderDistance;
				startChunkCoords = pc;
				vsCurrentVersion++;
				newVs = true;
				changedChunksQueue.clear();
			} else if (!changedChunksQueue.empty()) {
				startChunkCoords = changedChunksQueue.front();
				changedChunksQueue.pop_front();
			}

			auto startIt = vsChunks.find(startChunkCoords);
			if (startIt == vsChunks.end()) {
				auto pair = vsChunks.insert({startChunkCoords, ChunkVSInfo()});
				startIt = pair.first;
			}

			if (newVs) {
				startIt->second.outs = 0x3F;
				startIt->second.outsVersion = vsCurrentVersion;
				startIt->second.ins = 0x3F;
				startIt->second.insVersion = vsCurrentVersion;

				renderChunks[1 - renderChunksPage].insert({{0, 0, 0}, startChunkCoords});

				for (int d = 0; d < 6; d++) {
					vec3i64 ncc = startChunkCoords + DIRS[d].cast<int64>();
					auto nit = vsChunks.find(ncc);
					if (nit == vsChunks.end()) {
							auto pair = vsChunks.insert({ncc, ChunkVSInfo()});
							nit = pair.first;
					}
					vsFringe.push(ncc);
					nit->second.ins = (1 << ((d + 3) % 6));
					nit->second.insVersion = vsCurrentVersion;
					vsInFringe.insert(ncc);
				}
			} else if (startIt->second.ins > 0 && startIt->second.insVersion == vsCurrentVersion) {
				vsFringe.push(startChunkCoords);
				vsInFringe.insert(startChunkCoords);
			} else {
				return;
			}
		}

		while (!vsFringe.empty() && traversedChunks < MAX_VS_CHUNKS) {
			traversedChunks++;
			vec3i64 cc = vsFringe.front();
			vsFringe.pop();
			vsInFringe.erase(cc);

			auto builtIt = builtChunks.find(cc);
			auto vsIt = vsChunks.find(cc);

			int changedOuts = updateVsChunk(cc);

			int page = newVs ? 1 - renderChunksPage : renderChunksPage;
			if (builtIt != builtChunks.end()
					&& builtIt->second.numFaces > 0
					&& vsIt->second.ins > 0)
				renderChunks[page].insert({cc - vsPlayerChunk, cc});
			else
				renderChunks[page].erase(cc - vsPlayerChunk);

			for (int d = 0; d < 6; d++) {
				if (((changedOuts >> d) & 1) == 0)
					continue;

				vec3i64 ncc = cc + DIRS[d].cast<int64>();

				if ((ncc - vsPlayerChunk).norm() > vsRenderDistance)
					continue;

				auto nVsIt = vsChunks.find(ncc);
				if (nVsIt == vsChunks.end()) {
						auto pair = vsChunks.insert({ncc, ChunkVSInfo()});
						nVsIt = pair.first;
				}

				if (nVsIt->second.insVersion != vsCurrentVersion) {
					nVsIt->second.ins = 0;
					nVsIt->second.insVersion = vsCurrentVersion;
				}
				nVsIt->second.ins |= (1 << ((d + 3) % 6));

				if (vsInFringe.find(ncc) == vsInFringe.end()) {
					vsFringe.push(ncc);
					vsInFringe.insert(ncc);
				}
			}
		}
		if (vsFringe.empty() && newVs) {
			newVs = false;
			renderChunks[renderChunksPage].clear();
			renderChunksPage = 1 - renderChunksPage;
		}
	}
}

int ChunkRenderer::updateVsChunk(vec3i64 chunkCoords) {
	auto builtIt = builtChunks.find(chunkCoords);
	auto vsIt = vsChunks.find(chunkCoords);

	int passThroughs = 0x3FFF;
	if (builtIt != builtChunks.end())
		passThroughs = builtIt->second.passThroughs;

	vec3i64 cd = chunkCoords - vsPlayerChunk;
	int dirMask = 0;
	for (int dim = 0; dim < 3; dim++) {
		if (cd[dim] >= -1)
			dirMask |= 1 << dim;
		if (cd[dim] <= 1)
			dirMask |= 8 << dim;
	}

	int oldOuts = vsIt->second.outs;
	if (vsIt->second.outsVersion != vsCurrentVersion)
		oldOuts = 0;
	vsIt->second.outs = getOuts(vsIt->second.ins, passThroughs) & dirMask;
	vsIt->second.outsVersion = vsCurrentVersion;

	return oldOuts ^ vsIt->second.outs;
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
