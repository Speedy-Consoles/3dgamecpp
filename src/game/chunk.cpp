#include "chunk.hpp"
#include "world.hpp"
#include "util.hpp"
#include "io/chunk_loader.hpp"

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
		cc(cc),
		chunkLoader(chunkLoader),
		faces(0, faceHashFunc),
		borderFaces(0, faceHashFunc) {
	// nothing
}

bool operator == (const Face &lhs, const Face &rhs) {
	if (&lhs == &rhs)
		return true;
    return lhs.block == rhs.block && lhs.dir == rhs.dir;
}

const double World::GRAVITY = -9.81 * RESOLUTION / 60.0 / 60.0 * 4;

void Chunk::initFaces() {
	uint ds[3];
	vec3ui8 uDirs[3];
	for (uint8 d = 0; d < 3; d++) {
		uDirs[d] = DIRS[d].cast<uint8>();
		ds[d] = getBlockIndex(uDirs[d]);
	}

	for (uint8 d = 0; d < 3; d++) {
		uint i = 0;
		for (uint8 z = 0; z < WIDTH; z++) {
			for (uint8 y = 0; y < WIDTH; y++) {
				for (uint8 x = 0; x < WIDTH; x++, i++) {
					if ((x == WIDTH - 1 && d==0)
							|| (y == WIDTH - 1 && d==1)
							|| (z == WIDTH - 1 && d==2))
						continue;

					uint ni = i + ds[d];

					uint8 thisType = blocks[i];
					uint8 thatType = blocks[ni];
					if(thisType != thatType) {
						vec3ui8 faceBlock;
						uint8 faceDir;
						if (thisType == 0) {
							vec3ui8 dir = uDirs[d];
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
							vec3i dIcc = faceBlock.cast<int>() + v;
							if (		dIcc[0] < 0 || dIcc[0] >= (int) WIDTH
									||	dIcc[1] < 0 || dIcc[1] >= (int) WIDTH
									||	dIcc[2] < 0 || dIcc[2] >= (int) WIDTH)
								continue;
							if (getBlock(dIcc.cast<uint8>())) {
								corners |= 1 << j;
							}
						}
						faces.insert(Face{faceBlock, faceDir, corners});
						if (		((x == WIDTH - 1 || x == 0) && d % 3 != 0)
								||	((y == WIDTH - 1 || y == 0) && d % 3 != 1)
								||	((z == WIDTH - 1 || z == 0) && d % 3 != 2))
							borderFaces.insert(Face{faceBlock, faceDir, corners});
					}
				}
			}
		}
	}
}

void Chunk::makePassThroughs() {
	passThroughs = 0;
	bool visited[WIDTH * WIDTH * WIDTH];

	for (uint i = 0; i < WIDTH * WIDTH * WIDTH; i++) {
		visited[i] = false;
	}

	size_t index = 0;
	for (uint8 z = 0; z < WIDTH; z++) {
		for (uint8 y = 0; y < WIDTH; y++) {
			for (uint8 x = 0; x < WIDTH; x++) {
				visited[index] = true;
				if (blocks[index] != 0) {
					index++;
					continue;
				}

				vec3ui8 fringe[WIDTH * WIDTH * WIDTH];
				fringe[0] = vec3ui8(x, y, z);
				int fringeSize = 1;
				int borderSet = 0;
				while (fringeSize > 0) {
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

				index++;
			}
		}
	}
}

void Chunk::initBlock(size_t index, uint8 type) {
	blocks[index] = type;
}

bool Chunk::setBlock(vec3ui8 icc, uint8 type) {
	if (getBlock(icc) == type)
		return true;
	blocks[getBlockIndex(icc)] = type;
	return true;
}

uint8 Chunk::getBlock(vec3ui8 icc) const {
	return blocks[getBlockIndex(icc)];
}

void Chunk::addFace(Face face) {
	auto pair = faces.insert(face);
	if (!pair.second) {
		pair.first->corners = face.corners;
	}
	if (		((face.block[0] == WIDTH - 1 || face.block[0] == 0) && face.dir % 3 != 0)
			||	((face.block[1] == WIDTH - 1 || face.block[1] == 0) && face.dir % 3 != 1)
			||	((face.block[2] == WIDTH - 1 || face.block[2] == 0) && face.dir % 3 != 2)) {
		auto bPair = borderFaces.insert(face);
		if (!bPair.second) {
			bPair.first->corners = face.corners;
		}
	}
	changed = true;
}

bool Chunk::removeFace(Face face) {
	auto it = faces.find(face);
	if(it == faces.end())
		return false;
	faces.erase(it);
	if (		((face.block[0] == WIDTH - 1 || face.block[0] == 0) && face.dir % 3 != 0)
			||	((face.block[1] == WIDTH - 1 || face.block[1] == 0) && face.dir % 3 != 1)
			||	((face.block[2] == WIDTH - 1 || face.block[2] == 0) && face.dir % 3 != 2))
		borderFaces.erase(face);
	changed = true;
	return true;
}

const Chunk::FaceSet &Chunk::getFaces() const {
	return faces;
}

const Chunk::FaceSet &Chunk::getBorderFaces() const {
	return borderFaces;
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
