#ifndef CHUNK_HPP
#define CHUNK_HPP

#include "std_types.hpp"
#include "vmath.hpp"

#include <unordered_set>

class ChunkLoader;

class World;

struct Face {
	vec3ui8 block;
	uint8 dir;
	mutable uint8 corners;
};

bool operator == (const Face &lhs, const Face &rhs);

class Chunk {
public:
	static const uint8 WIDTH = 32;

	const vec3i64 cc;

	using FaceSet = std::unordered_set<Face, size_t(*)(Face)>;

private:
	bool changed = true;

	uint8 blocks[WIDTH * WIDTH * WIDTH];
	FaceSet faces;
	FaceSet borderFaces;

	ChunkLoader *chunkLoader;

public:
	Chunk(vec3i64 cc, ChunkLoader *chunkLoader = nullptr);

	void initFaces();

	void initBlock(size_t index, uint8 type);
	bool setBlock(vec3ui8 icc, uint8 type);
	uint8 getBlock(vec3ui8 icc) const;
	const uint8 *getBlocks() const { return blocks; }

	void addFace(Face face);
	bool removeFace(Face face);

	const FaceSet &getFaces() const;
	const FaceSet &getBorderFaces() const;

	bool pollChanged();

	void setChunkLoader(ChunkLoader *cl) { chunkLoader = cl; }
	void free();

/*
	void write(ByteBuffer buffer) const;
	static Chunk readChunk(ByteBuffer buffer);
*/
	static int getBlockIndex(vec3ui8 icc);
};

#endif // CHUNK_HPP
