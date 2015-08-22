#include "chunk.hpp"

#include "shared/engine/logging.hpp"
#include "shared/block_utils.hpp"

static logging::Logger logger("chunk");

Chunk::Chunk(int flags) {
	this->flags = flags & VISUAL;
}

void Chunk::initCC(vec3i64 chunkCoords) {
	this->cc = chunkCoords;
	flags |= COORDS_INITIALIZED;
}

void Chunk::initNumAirBlocks(int numAirBlocks) {
	this->numAirBlocks = numAirBlocks;
	flags |= NUM_AIR_BLOCKS_INITIALIZED;
}

void Chunk::initPassThroughs(uint16 passThroughs) {
	this->passThroughs = passThroughs;
	flags |= PASSTHROUGHS_INITIALIZED;
}

void Chunk::initBlock(vec3ui8 intraChunkCoords, uint8 type) {
	blocks[getBlockIndex(intraChunkCoords)] = type;
}

void Chunk::initBlock(size_t index, uint8 type) {
	blocks[index] = type;
}

void Chunk::finishInitialization() {
	if (!(flags & COORDS_INITIALIZED))
		LOG_ERROR(logger) << "Chunk coordinates not initialized";
	if (!(flags & NUM_AIR_BLOCKS_INITIALIZED)) {
		for (size_t i = 0; i < WIDTH * WIDTH * WIDTH; i++) {
			if (blocks[i] == 0)
				numAirBlocks++;
		}
	}

	if (!(flags & (VISUAL | PASSTHROUGHS_INITIALIZED))) {
		makePassThroughs();
		flags |= PASSTHROUGHS_INITIALIZED;
	}
	flags |= INITIALIZED;
}

void Chunk::reset() {
	flags = flags & VISUAL;
	numAirBlocks = 0;
	passThroughs = 0;
	revision = 0;
}

void Chunk::setBlock(size_t index, uint8 type) {
	if (blocks[index] == type)
		return;

	blocks[index] = type;
	if (type == 0)
		numAirBlocks++;
	else
		numAirBlocks--;
	revision++;
	if (flags & VISUAL)
		makePassThroughs();
}

uint8 Chunk::getBlock(vec3ui8 icc) const {
	return blocks[getBlockIndex(icc)];
}

size_t Chunk::getBlockIndex(vec3ui8 icc) {
	return (icc[2] * WIDTH + icc[1]) * WIDTH + icc[0];
}

void Chunk::makePassThroughs() {
	const uint size = WIDTH * WIDTH * WIDTH;
	if (numAirBlocks > size - WIDTH * WIDTH) {
		passThroughs = 0x7FFF;
		return;
	}
	passThroughs = 0;
	bool visited[size];

	for (uint i = 0; i < size; i++) {
		visited[i] = false;
	}

	size_t index = 0;
	uint foundAirBlocks = 0;
	for (uint8 z = 0; z < WIDTH; z++) {
		for (uint8 y = 0; y < WIDTH; y++) {
			for (uint8 x = 0; x < WIDTH; x++) {
				visited[index] = true;
				if (blocks[index] != 0) {
					index++;
					continue;
				}

				vec3ui8 fringe[size];
				fringe[0] = vec3ui8(x, y, z);
				int fringeSize = 1;
				int borderSet = 0;
				while (fringeSize > 0) {
					foundAirBlocks++;
					vec3ui8 icc = fringe[--fringeSize];
					for (int d = 0; d < 6; d++) {
						if (icc[DIR_DIMS[d]] == (1 - d / 3) * (WIDTH - 1))
							borderSet |= (1 << d);
						else {
							vec3ui8 nIcc = icc + DIRS[d].cast<uint8>();
							size_t nIndex = getBlockIndex(nIcc);
							if (blocks[nIndex] == 0 && !visited[nIndex]) {
								visited[nIndex] = true;
								fringe[fringeSize++] = nIcc;
							}
						}
					}
				}

				int shift = 0;
				for (int d1 = 0; d1 < 5; d1++) {
					if (borderSet & (1 << d1)) {
						for (int d2 = d1 + 1; d2 < 6; d2++) {
							if (borderSet & (1 << d2))
								passThroughs |= (1 << shift);
							shift++;
						}
					} else
						shift += 5 - d1;
				}

				if (foundAirBlocks >= numAirBlocks)
					return;

				index++;
			}
		}
	}
}
