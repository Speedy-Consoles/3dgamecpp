#include "chunk.hpp"
#include "world.hpp"
#include "util.hpp"
#include "chunk_loader.hpp"

static size_t faceHashFunc(Face f) {
		static const int prime = 31;
		int result = 1;
		result = prime * result + f.block[0];
		result = prime * result + f.block[1];
		result = prime * result + f.block[2];
		result = prime * result + f.dir;
		return result;
}

Chunk::Chunk(vec3i64 cc, ChunkLoader *chunkLoader) :
		cc(cc), faces(0, faceHashFunc), chunkLoader(chunkLoader) {
	// nothing
}

bool operator == (const Face &lhs, const Face &rhs) {
	if (&lhs == &rhs)
		return true;
    return lhs.block == rhs.block && lhs.dir == rhs.dir;
}

const double World::GRAVITY = -9.81 * RESOLUTION / 60.0 / 60.0 * 4;

void Chunk::initFaces() {
	// TODO only one loop
	using namespace vec_auto_cast;
	uint i = 0;
	for (uint z = 0; z < WIDTH; z++) {
		for (uint y = 0; y < WIDTH; y++) {
			for (uint x = 0; x < WIDTH; x++) {
				for (uint8 d = 0; d < 3; d++) {
					vec3ui8 dir = DIRS[d].cast<uint8>();
					if ((x == WIDTH - 1 && d==0)
							|| (y == WIDTH - 1 && d==1)
							|| (z == WIDTH - 1 && d==2))
						continue;

					uint ni = i + getBlockIndex(dir);

					uint8 thisType = blocks[i];
					uint8 thatType = blocks[ni];
					if(thisType != thatType) {
						vec3ui8 faceBlock;
						uint8 faceDir;
						if (thisType == 0) {
							faceBlock = vec3ui8(x, y, z) + dir;
							faceDir = (uint8) (d + 3);
						} else if (thatType == 0){
							faceBlock = vec3ui8(x, y, z);
							faceDir = d;
						} else
							continue;

						uint8 corners = 0;
						for (int j = 0; j < 8; ++j) {
							vec3i v = EIGHT_CYCLES_3D[faceDir][j];
							vec3i dIcc = faceBlock + v;
							if (		dIcc[0] < 0 || dIcc[0] >= WIDTH
									||	dIcc[1] < 0 || dIcc[1] >= WIDTH
									||	dIcc[2] < 0 || dIcc[2] >= WIDTH)
								continue;
							if (getBlock(dIcc.cast<uint8>())) {
								corners |= 1 << j;
							}
						}
							faces.insert(Face{faceBlock, faceDir, corners});
					}
				}
				i++;
			}
		}
	}
}

void Chunk::patchBorders(World *world, bool changedChunks[7]) {
	using namespace vec_auto_cast;
	changedChunks[6] = false;
	for (uint8 d = 0; d < 6; d++) {
		changedChunks[d] = false;
		uint8 invD = (d + 3) % 6;
		int dim = DIR_DIMS[d];
		vec3i64 ncc = cc + DIRS[d];
		Chunk *nc = world->getChunk(ncc);
		if (!nc)
			continue;

		vec3ui8 icc(0, 0, 0);
		vec3ui8 nIcc(0, 0, 0);
		icc[dim] = (1 - d / 3) * (WIDTH - 1);
		nIcc[dim] = WIDTH - 1 - icc[dim];

		for(uint8 a = 0; a < WIDTH; a++) {
			for(uint8 b = 0; b < WIDTH; b++) {
				icc[OTHER_DIR_DIMS[d][0]] = a;
				nIcc[OTHER_DIR_DIMS[d][0]] = a;
				icc[OTHER_DIR_DIMS[d][1]] = b;
				nIcc[OTHER_DIR_DIMS[d][1]] = b;

				uint8 neighborType = nc->getBlock(nIcc);

				if (neighborType != 0) {
					if (getBlock(icc) != 0)
						nc->faces.erase(Face{nIcc, invD, 0});
					else
						nc->faces.insert(Face{nIcc, invD, TEST_CORNERS[invD]});
					changedChunks[d] = true;
					nc->changed = true;
				} else {
					if (getBlock(icc) != 0)
						faces.insert(Face{icc, d, TEST_CORNERS[d]});
					else
						faces.erase(Face{icc, d, 0});
					changedChunks[6] = true;
					changed = true;
				}
			}
		}
	}
}

void Chunk::initBlock(size_t index, uint8 type) {
	blocks[index] = type;
}

bool Chunk::setBlock(vec3ui8 icc, uint8 type, World *world, bool changedChunks[7]) {
	if (getBlock(icc) == type)
		return true;
	blocks[getBlockIndex(icc)] = type;
	updateBlockFaces(icc, *world, changedChunks);
	return true;
}

uint8 Chunk::getBlock(vec3ui8 icc) const {
	return blocks[getBlockIndex(icc)];
}

const Chunk::FaceSet &Chunk::getFaceSet() const {
	return faces;
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
int Chunk::getBlockIndex(vec3ui8 icc) {
	return (icc[2] * WIDTH + icc[1]) * WIDTH + icc[0];
}

static int8 helperFunc(int8 t) {
	return (t + Chunk::WIDTH) % Chunk::WIDTH;
}

void Chunk::updateBlockFaces(vec3ui8 icc, World &world, bool changedChunks[7]) {
	using namespace vec_auto_cast;
	changedChunks[6] = false;
	for (uint8 d = 0; d < 6; d++) {
		changedChunks[d] = false;
		uint8 invD = (d + 3) % 6;
		vec3i8 dir = DIRS[d];
		vec3i8 pnIcc = icc.cast<int8>() + dir;
		vec3ui8 nIcc;
		uint8 neighborType;
		FaceSet *nFaces = nullptr;
		Chunk *nc = nullptr;
		if (		pnIcc[0] < 0 || pnIcc[0] >= WIDTH
				||	pnIcc[1] < 0 || pnIcc[1] >= WIDTH
				||	pnIcc[2] < 0 || pnIcc[2] >= WIDTH) {
			pnIcc.applyPW(helperFunc);
			nIcc = pnIcc.cast<uint8>();
			vec3i64 ncc = cc + dir;
			nc = world.getChunk(ncc);
			if (!nc)
				continue;
			neighborType = nc->getBlock(nIcc);
			nFaces = &nc->faces;
		} else {
			nIcc = pnIcc.cast<uint8>();
			neighborType = getBlock(nIcc);
			nFaces = &faces;
		}

		if (neighborType != 0) {
			if (getBlock(icc) == 0)
				nFaces->insert(Face{nIcc, invD, TEST_CORNERS[invD]});
			else
				nFaces->erase(Face{nIcc, invD, 0});
			if(nc) {
				changedChunks[d] = true;
				nc->changed = true;
			}
		} else {
			if (getBlock(icc) != 0)
				faces.insert(Face{icc, d, TEST_CORNERS[d]});
			else
				faces.erase(Face{icc, d, 0});
			changedChunks[6] = true;
			changed = true;
		}
	}
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
