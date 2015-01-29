#include "chunk.hpp"
#include "world.hpp"
#include "util.hpp"
#include "io/chunk_loader.hpp"

Chunk::Chunk(vec3i64 cc, ChunkLoader *chunkLoader) :
		cc(cc),
		chunkLoader(chunkLoader) {
	// nothing
}

const double World::GRAVITY = -9.81 * RESOLUTION / 60.0 / 60.0 * 4;

void Chunk::makePassThroughs() {
	const uint size = WIDTH * WIDTH * WIDTH;
	if (airBlocks > size - WIDTH * WIDTH) {
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

				if (foundAirBlocks >= airBlocks)
					return;

				index++;
			}
		}
	}
}

void Chunk::initBlock(size_t index, uint8 type) {
	blocks[index] = type;
	if (type == 0)
		airBlocks++;
}

bool Chunk::setBlock(vec3ui8 icc, uint8 type) {
	if (getBlock(icc) == type)
		return true;
	blocks[getBlockIndex(icc)] = type;
	if (type == 0)
		airBlocks++;
	else
		airBlocks--;
	return true;
}

uint8 Chunk::getBlock(vec3ui8 icc) const {
	return blocks[getBlockIndex(icc)];
}

/*
void Chunk::write(ByteBuffer buffer) const {
	buffer.putLong(cx);
	buffer.putLong(cy);
	buffer.putLong(cz);
	Deflater deflater = new Deflater();
	deflater.setInput(blocks);
	deflater.finish();
	byte cBytes[(int) ((WIDTH * WIDTH * WIDTH) * 1.1)];
	int written = deflater.deflate(cBytes, 0, cBytes.length,
			Deflater.SYNC_FLUSH);
	buffer.putShort((short) written);
	buffer.put(cBytes, 0, written);
}

static Chunk Chunk::readChunk(ByteBuffer buffer) {
	vec3i64 cc(
		buffer.getLong(),
		buffer.getLong(),
		buffer.getLong()
	);
	short written = buffer.getShort();
	byte cBytes[written];
	buffer.get(cBytes);
	Inflater inflater = new Inflater();
	inflater.setInput(cBytes);
	byte blocks[WIDTH * WIDTH * WIDTH];
	try {
		inflater.inflate(blocks);
	} catch (DataFormatException e) {
		e.printStackTrace();
	}
	return Chunk(cc, blocks);
}
*/
size_t Chunk::getBlockIndex(vec3ui8 icc) {
	return (icc[2] * WIDTH + icc[1]) * WIDTH + icc[0];
}

bool Chunk::pollChanged() {
	bool tmp = changed;
	changed = false;
	return tmp;
}

void Chunk::free() {
	 if (chunkLoader)
		 chunkLoader->free(this);
	 else
		 delete this;
}
