#ifndef CHUNK_HPP
#define CHUNK_HPP

#include "std_types.hpp"
#include "vmath.hpp"
#include <unordered_set>

class World;

struct Face {
	vec3ui8 block;
	uint8 dir;
};

bool operator == (const Face &lhs, const Face &rhs);

class Chunk {
public:
	static const uint WIDTH = 32;

	const vec3i64 cc;

	using FaceSet = std::unordered_set<Face, size_t(*)(Face)>;

private:
	bool changed = true;

	uint8 blocks[WIDTH * WIDTH * WIDTH];
	FaceSet faces;

public:
	Chunk(vec3i64 cc);

	void init(uint8 blocks[WIDTH * WIDTH * WIDTH]);
	void initFaces();
	void patchBorders(World *world);

	bool setBlock(vec3ui8 icc, uint8 type, World *world);
	uint8 getBlock(vec3ui8 icc) const;

	const FaceSet &getFaceSet() const;

	bool pollChanged();
/*
	void write(ByteBuffer buffer) const;
	static Chunk readChunk(ByteBuffer buffer);
*/
	static int getBlockIndex(vec3ui8 icc);

private:
	void updateBlockFaces(vec3ui8 icc, World *world);
};

#endif // CHUNK_HPP
