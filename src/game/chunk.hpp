#ifndef CHUNK_HPP
#define CHUNK_HPP

#include "vmath.hpp"

#include <unordered_set>

class ChunkLoader;

struct Face {
	vec3ui8 block;
	uint8 dir;
	mutable uint8 corners;
};

bool operator == (const Face &lhs, const Face &rhs);

class Chunk {
public:
	static const uint WIDTH = 32;

	using FaceSet = std::unordered_set<Face, size_t(*)(Face)>;

private:
	vec3i64 cc;
	ChunkLoader *chunkLoader;
	bool changed = true;

	FaceSet faces;
	FaceSet borderFaces;

	uint8 blocks[WIDTH * WIDTH * WIDTH];

	uint16 passThroughs = 0;

public:
	Chunk(vec3i64 cc, ChunkLoader *chunkLoader = nullptr);

	void initFaces();
	void makePassThroughs();
	vec3i64 getCC() const { return cc; }
	void setCC(vec3i64 cc) { this->cc = cc; }
	void initBlock(size_t index, uint8 type);
	bool setBlock(vec3ui8 icc, uint8 type);
	uint8 getBlock(vec3ui8 icc) const;

	const uint8 *getBlocks() const { return blocks; }
	const uint16 getPassThroughs() { return passThroughs; }

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
	static size_t getBlockIndex(vec3ui8 icc);
};

#endif // CHUNK_HPP
