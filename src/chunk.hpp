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
	uint8 corners;
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

	ChunkLoader *chunkLoader;

public:
	Chunk(vec3i64 cc, ChunkLoader *chunkLoader = nullptr);

	void initFaces();
	void patchBorders(World *world, bool changedChunks[7]);

	void initBlock(size_t index, uint8 type);
	bool setBlock(vec3ui8 icc, uint8 type, World *world, bool changedChunks[7]);
	uint8 getBlock(vec3ui8 icc) const;
	const uint8 *getBlocks() const { return blocks; }

	const FaceSet &getFaceSet() const;

	bool pollChanged();

	void setChunkLoader(ChunkLoader *cl) { chunkLoader = cl; }
	void free();

/*
	void write(ByteBuffer buffer) const;
	static Chunk readChunk(ByteBuffer buffer);
*/
	static int getBlockIndex(vec3ui8 icc);


private:
	void updateBlockFaces(vec3ui8 icc, World &world, bool changedChunks[7]);
};

#endif // CHUNK_HPP
