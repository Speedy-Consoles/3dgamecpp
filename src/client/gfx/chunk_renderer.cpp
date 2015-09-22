#include "chunk_renderer.hpp"

#include "../../shared/game/character.hpp"
#include "shared/engine/logging.hpp"
#include "shared/engine/stopwatch.hpp"
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
		toBuildQueue(1024),
		toFinishQueue(1024),
		builtChunks(0, vec3i64HashFunc),
		vsChunks(0, vec3i64HashFunc),
		vsInFringe(0, vec3i64HashFunc),
		client(client),
		renderer(renderer) {
	renderChunks[0] = std::map<vec3i64, vec3i64, bool(*)(vec3i64, vec3i64)>(vec3i64CompFunc);
	renderChunks[1] = std::map<vec3i64, vec3i64, bool(*)(vec3i64, vec3i64)>(vec3i64CompFunc);
	renderDistance = client->getConf().render_distance;
	dispatch();
}

ChunkRenderer::~ChunkRenderer() {
	requestTermination();
	ChunkVisuals cv;
	while (toFinishQueue.pop(cv));
	wait();
}

void ChunkRenderer::setConf(const GraphicsConf &conf, const GraphicsConf &old) {
	if (conf.render_distance != old.render_distance) {
		checkChunkIndex = 0; // TODO make smarter
		this->renderDistance = conf.render_distance;
	}
}

void ChunkRenderer::tick() {
	Character &character = client->getLocalCharacter();
	if (!character.isValid())
		return;

	client->getStopwatch()->start(CLOCK_CRT);

	vec3i64 pc = character.getChunkPos();
	if (pc != oldCharacterChunk) {
		// determine new checkChunkIndex
		vec3i64 diff = pc - oldCharacterChunk;
		if (diff.maxAbs() > LO_INDEX_FINISHED_RADIUS[checkChunkIndex]) {
			checkChunkIndex = 0;
		} else if (checkChunkIndex > 0) {
			double newRadius = LO_INDEX_FINISHED_RADIUS[checkChunkIndex] - diff.norm();
			if (newRadius < 0)
				checkChunkIndex = 0;
			else
				checkChunkIndex = LO_MAX_RADIUS_INDICES[(int) newRadius];
		}
		oldCharacterChunk = pc;

		// delete chunk info of chunks out of range
		for (auto it = builtChunks.begin(); it != builtChunks.end();) {
			if ((it->first - pc).norm() > renderDistance) {
				destroyChunkData(it->first);
				numFaces -= it->second.numFaces;
				it = builtChunks.erase(it);
			} else {
				++it;
			}
		}
	}

	// put chunks into render queue
	while (LO_INDEX_FINISHED_RADIUS[checkChunkIndex] < renderDistance
			&& buildQueue.size() < MAX_BUILD_QUEUE_SIZE) {
		vec3i64 cd = LOADING_ORDER[checkChunkIndex].cast<int64>();
		if (cd.norm() <= renderDistance) {
			vec3i64 cc = pc + cd;
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
		}
		checkChunkIndex++;
	}

	client->getStopwatch()->start(CLOCK_IBQ);
	// build chunks in render queue
	newFaces = 0;
	newChunks = 0;
	while (!buildQueue.empty()) {
		vec3i64 cc = buildQueue.front();
		ChunkArea area;
		if (!getChunkArea(cc, &area))
			break;

		const Chunk *chunk = area.chunks[BIG_CUBE_CYCLE_BASE_INDEX];

		if (!chunkHasQuads(area)) {
			finishChunk(ChunkVisuals{cc, chunk->getRevision(), std::vector<Quad>()});
			for (int i = 0; i < 27; ++i) {
				client->getChunkManager()->releaseChunk(cc + BIG_CUBE_CYCLE[i].cast<int64>());
			}
		} else if (!toBuildQueue.push(area)) // TODO data must be copied or locked for thread safety
			break;

		buildQueue.pop_front();
		inBuildQueue.erase(inBuildQueue.find(cc));
	}
	client->getStopwatch()->stop(CLOCK_IBQ);

	client->getStopwatch()->start(CLOCK_BCH);
	ChunkVisuals cv;
	while(toFinishQueue.pop(cv)) {
		finishChunk(cv);
		for (int i = 0; i < 27; ++i) {
			client->getChunkManager()->releaseChunk(cv.cc + BIG_CUBE_CYCLE[i].cast<int64>());
		}
	}
	client->getStopwatch()->stop(CLOCK_BCH);

	client->getStopwatch()->start(CLOCK_VS);
	visibilitySearch();
	client->getStopwatch()->stop(CLOCK_VS);

	client->getStopwatch()->stop(CLOCK_CRT);
}

void ChunkRenderer::render() {
	// render chunks
	Character &character = client->getLocalCharacter();
	if (!character.isValid())
		return;

	client->getStopwatch()->start(CLOCK_CRR);

	beginRender();

	vec3d lookDir = getVectorFromAngles(character.getYaw() / 100.0f, character.getPitch() / 100.0f);

	visibleChunks = 0;
	visibleFaces = 0;

	vec3i64 pc = character.getChunkPos();
	for (int i = 0; i < 27; i++) {
		vec3i64 cc = BIG_CUBE_CYCLE[i].cast<int64>() + pc;
		auto builtIt = builtChunks.find(cc);
		if (builtIt != builtChunks.end()){
			renderChunk(cc);
			visibleChunks++;
			visibleFaces += builtIt->second.numFaces;
		}
	}

	for (auto renderIt = renderChunks[renderChunksPage].begin(); renderIt != renderChunks[renderChunksPage].end(); ++renderIt) {
		vec3i64 cc = renderIt->second;
		auto builtIt = builtChunks.find(cc);
		if (builtIt != builtChunks.end()
				&& (cc - pc).norm() >= 2
				&& (cc - pc).norm() <= renderDistance
				&& inFrustum(cc, character.getPos(), lookDir)){
			renderChunk(cc);
			visibleChunks++;
			visibleFaces += builtIt->second.numFaces;
		}
	}

	finishRender();
	
	client->getStopwatch()->stop(CLOCK_CRR);
}

void ChunkRenderer::doWork() {
	ChunkArea area;
	if(toBuildQueue.pop(area)) {
		ChunkVisuals cv = buildChunk(area);
		while(!toFinishQueue.push(cv))
			sleepFor(millis(50));
	} else {
		sleepFor(millis(100));
	}
}

void ChunkRenderer::rebuildChunk(vec3i64 chunkCoords) {
	auto it = builtChunks.find(chunkCoords);
	if (it == builtChunks.end())
		return;

	ChunkArea area;
	if (!getChunkArea(chunkCoords, &area) && inBuildQueue.find(chunkCoords) == inBuildQueue.end()) {
		inBuildQueue.insert(chunkCoords);
		buildQueue.push_front(chunkCoords);
		for (size_t i = 0; i < 27; ++i) {
			client->getChunkManager()->requestChunk(chunkCoords + BIG_CUBE_CYCLE[i].cast<int64>());
		}
		return;
	}

	const Chunk *chunk = area.chunks[BIG_CUBE_CYCLE_BASE_INDEX];

	ChunkVisuals cv;
	if (!chunkHasQuads(area))
		cv = ChunkVisuals{chunkCoords, chunk->getRevision(), std::vector<Quad>()};
	else
		cv = buildChunk(area);
	finishChunk(cv);
}

ChunkRendererDebugInfo ChunkRenderer::getDebugInfo() {
	ChunkRendererDebugInfo info;
	info.checkedDistance = LO_INDEX_FINISHED_RADIUS[checkChunkIndex];
	info.newFaces = newFaces;
	info.newChunks = newChunks;
	info.totalFaces = numFaces;
	info.visibleChunks = visibleChunks;
	info.visibleFaces = visibleFaces;
	info.buildQueueSize = buildQueue.size();

	return info;
}

ChunkRenderer::ChunkVisuals ChunkRenderer::buildChunk(ChunkArea area) {
	const Chunk &chunk = *(area.chunks[BIG_CUBE_CYCLE_BASE_INDEX]);
	vec3i64 cc = chunk.getCC();

	std::vector<Quad> quads;
	quads.reserve(Chunk::WIDTH * Chunk::WIDTH * (Chunk::WIDTH + 1) * 3);

	const uint8 *blocks = chunk.getBlocks();
	for (uint8 d = 0; d < 3; d++) {
		vec3i64 dir = DIRS[d].cast<int64>();
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
						const Chunk &otherChunk = *area.chunks[DIR_TO_BIG_CUBE_CYCLE_INDEX[d]];
						thatType = otherChunk.getBlocks()[i - dimFlipIndexDiff];
						if (thatType != 0) {
							if (!thisOutside)
								i++;
							continue;
						}
					} else
						thatType = blocks[ni++];

					if (thisOutside) {
						const Chunk &otherChunk = *area.chunks[DIR_TO_BIG_CUBE_CYCLE_INDEX[d + 3]];
						thisType = otherChunk.getBlocks()[ni - 1 + dimFlipIndexDiff];
						if (thisType != 0)
							continue;
					} else {
						thisType = blocks[i++];
					}

					if ((thisType == 0) != (thatType == 0)) {
						Quad quad;
						if (thisType == 0) {
							quad.icc = vec3ui8(x, y, z) + DIRS[d].cast<uint8>();
							quad.faceDir = (uint8) (d + 3);
							quad.faceType = thatType;
						} else {
							quad.icc = vec3ui8(x, y, z);
							quad.faceDir = d;
							quad.faceType = thisType;
						}

						uint8 corners = 0;
						for (int j = 0; j < 8; ++j) {
							vec3i v = DIR_QUAD_EIGHT_NEIGHBOR_CYCLES[quad.faceDir][j];
							vec3i dIcc = quad.icc.cast<int>() + v;
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
							const Chunk &otherChunk = *area.chunks[vec2BigCubeCycleIndex(ncc)];
							uint8 cornerBlock = otherChunk.getBlock(dIcc.cast<uint8>());
							if (cornerBlock != 0) {
								corners |= 1 << j;
							}
						}
						quad.bc = chunk.getCC() * chunk.WIDTH + quad.icc.cast<int64>();

						for (int j = 0; j < 4; j++) {
							quad.shadowLevels[j] = 0;
							bool s1 = (corners & QUAD_CORNER_MASK[j][0]) > 0;
							bool s2 = (corners & QUAD_CORNER_MASK[j][2]) > 0;
							bool m = (corners & QUAD_CORNER_MASK[j][1]) > 0;
							if (s1)
								quad.shadowLevels[j]++;
							if (s2)
								quad.shadowLevels[j]++;
							if (m || (s1 && s2))
								quad.shadowLevels[j]++;
						}
						quads.push_back(quad);
					}
				}
			}
		}
	}

	quads.shrink_to_fit();
	return ChunkVisuals{cc, chunk.getRevision(), quads};
}

void ChunkRenderer::finishChunk(ChunkVisuals cv) {
	auto it = builtChunks.find(cv.cc);
	if (it == builtChunks.end()) {
		auto pair = builtChunks.insert({cv.cc, ChunkBuildInfo()});
		it = pair.first;
	}

	if (it->second.revision > cv.revision)
		return;

	numFaces -= it->second.numFaces;

	applyChunkVisuals(cv);
	newChunks++;

	it->second.numFaces = cv.quads.size() * 2;
	it->second.revision = cv.revision;
	it->second.passThroughs = client->getChunkManager()->getChunk(cv.cc)->getPassThroughs();

	newFaces += it->second.numFaces;
	numFaces += it->second.numFaces;
	changedChunksQueue.push_back(cv.cc);
}

bool ChunkRenderer::getChunkArea(vec3i64 chunkCoordinates, ChunkArea *area) {
	for (int i = 0; i < 27; ++i) {
		area->chunks[i] = client->getChunkManager()->getChunk(chunkCoordinates + BIG_CUBE_CYCLE[i].cast<int64>());
		if (!area->chunks[i])
			return false;
	}
	return true;
}

bool ChunkRenderer::chunkHasQuads(ChunkArea area) {
	const Chunk *chunk = area.chunks[BIG_CUBE_CYCLE_BASE_INDEX];
	// skip air chunks and earth chunks
	if (chunk->isEmpty())
		return false;
	if (chunk->getNumAirBlocks() == 0) {
		for (int i = 0; i < 27; ++i) {
			if (BIG_CUBE_CYCLE[i].norm() <= 1 && area.chunks[i]->getNumAirBlocks() != 0)
				return true;
		}
		return false;
	}
	return true;
}

void ChunkRenderer::visibilitySearch() {
	Character &character = client->getLocalCharacter();
	if (!character.isValid())
		return;

	vec3i64 pc = character.getChunkPos();

	int traversedChunks = 0;
	while ((
				!vsFringe.empty()
				|| pc != vsCharacterChunk
				|| vsRenderDistance != renderDistance
				|| !changedChunksQueue.empty()
				|| vsCurrentVersion == 0)
			&& traversedChunks < MAX_VS_CHUNKS) {
		vec3i64 startChunkCoords;
		if (vsFringe.empty()) {
			if (pc != vsCharacterChunk
					|| vsCurrentVersion == 0
					|| vsRenderDistance != renderDistance) {
				for (auto it = vsChunks.begin(); it != vsChunks.end();) {
					if ((it->first - vsCharacterChunk).norm() > renderDistance) {
						it = vsChunks.erase(it);
					} else {
						++it;
					}
				}
				vsCharacterChunk = pc;
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

				vec3i64 cds[26];
				for (uint i = 0, j = 0; i < 27; i++) {
					if (i == BIG_CUBE_CYCLE_BASE_INDEX)
						continue;
					cds[j++] = BIG_CUBE_CYCLE[i].cast<int64>();
				}
				std::sort(cds, cds + 26, vec3i64CompFunc);
				for (uint i = 0; i < 26; i++) {
					vec3i64 cc = vsCharacterChunk + cds[i];
					auto it = vsChunks.find(cc);
					if (it == vsChunks.end()) {
						auto pair = vsChunks.insert({cc, ChunkVSInfo()});
						it = pair.first;
					}

					it->second.outs = 0x3F;
					it->second.outsVersion = vsCurrentVersion;
					it->second.ins = 0x3F;
					it->second.insVersion = vsCurrentVersion;

					renderChunks[1 - renderChunksPage].insert({cds[i], cc});

					for (int d = 0; d < 6; d++) {
						vec3i64 ncd = cds[i] + DIRS[d].cast<int64>();
						if (ncd.maxAbs() <= 1)
							continue;
						vec3i64 ncc = vsCharacterChunk + ncd;
						auto nit = vsChunks.find(ncc);
						if (nit == vsChunks.end()) {
							auto pair = vsChunks.insert({ncc, ChunkVSInfo()});
							nit = pair.first;
						}
						nit->second.ins = (1 << ((d + 3) % 6));
						nit->second.insVersion = vsCurrentVersion;
						vsFringe.push(nit);
						vsInFringe.insert(ncc);
					}
				}
			} else if ((vsCharacterChunk - startChunkCoords).maxAbs() > 1
					&& startIt->second.ins > 0
					&& startIt->second.insVersion == vsCurrentVersion) {
				vsFringe.push(startIt);
				vsInFringe.insert(startChunkCoords);
			} else {
				continue;
			}
		}

		while (!vsFringe.empty() && traversedChunks < MAX_VS_CHUNKS) {
			traversedChunks++;
			auto vsIt = vsFringe.front();
			vec3i64 cc = vsIt->first;
			vsFringe.pop();
			vsInFringe.erase(cc);
			auto builtIt = builtChunks.find(cc);

			int passThroughs = 0x3FFF;
			if (builtIt != builtChunks.end())
				passThroughs = builtIt->second.passThroughs;
			int changedOuts = updateVsChunk(cc, &vsIt->second, passThroughs);

			int page = newVs ? 1 - renderChunksPage : renderChunksPage;
			if (builtIt != builtChunks.end()
					&& builtIt->second.numFaces > 0
					&& vsIt->second.ins > 0)
				renderChunks[page].insert({cc - vsCharacterChunk, cc});
			else
				renderChunks[page].erase(cc - vsCharacterChunk);

			for (int d = 0; d < 6; d++) {
				if (((changedOuts >> d) & 1) == 0)
					continue;

				vec3i64 ncc = cc + DIRS[d].cast<int64>();

				if ((ncc - vsCharacterChunk).norm() > vsRenderDistance)
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
				if ((vsIt->second.outs & (1 << d)) != 0)
					nVsIt->second.ins |= 1 << ((d + 3) % 6);
				else
					nVsIt->second.ins &= ~(1 << ((d + 3) % 6));

				if (vsInFringe.find(ncc) == vsInFringe.end()) {
					vsFringe.push(nVsIt);
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

int ChunkRenderer::updateVsChunk(vec3i64 chunkCoords, ChunkVSInfo *vsInfo, int passThroughs) {
	vec3i64 cd = chunkCoords - vsCharacterChunk;

	int oldOuts = vsInfo->outs;
	if (vsInfo->outsVersion != vsCurrentVersion)
		oldOuts = 0;
	vsInfo->outs = getOuts(vsInfo->ins, passThroughs, cd, 1);
	vsInfo->outsVersion = vsCurrentVersion;

	return oldOuts ^ vsInfo->outs;
}

int ChunkRenderer::getOuts(int ins, int passThroughs, vec3i64 chunkDiff, int tolerance) {
	int outs = 0;
	int dirMask = 0;
	for (int dim = 0; dim < 3; dim++) {
		if (chunkDiff[dim] >= -tolerance)
			dirMask |= 1 << dim;
		if (chunkDiff[dim] <= tolerance)
			dirMask |= 8 << dim;
	}

	for (int d1 = 0; d1 < 6; d1++) {
		if (((ins >> d1) & 1) == 0)
			continue;
		int dimDiff = (int)std::abs(chunkDiff[d1 % 3]);
		int otherDimDiff1 = (int)std::abs(chunkDiff[OTHER_DIR_DIMS[d1][0]]);
		int otherDimDiff2 = (int)std::abs(chunkDiff[OTHER_DIR_DIMS[d1][1]]);
		bool oppositeVisible = otherDimDiff1 - tolerance <= dimDiff
				&& otherDimDiff2 - tolerance <= dimDiff;

		int shift = 0;
		for (int d2 = 0; d2 < d1; d2++) {
			if (((dirMask >> d2) & 1) != 0 && (oppositeVisible || d1 % 3 != d2 % 3))
				outs |= ((passThroughs & (1 << (shift + d1 - d2 - 1))) > 0) << d2;
			shift += 5 - d2;
		}
		for (int d2 = d1 + 1; d2 < 6; d2++) {
			if (((dirMask >> d2) & 1) != 0 && (oppositeVisible || d1 % 3 != d2 % 3))
				outs |= ((passThroughs & (1 << (shift + d2 - d1 - 1))) > 0) << d2;
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
