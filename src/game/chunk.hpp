#ifndef CHUNK_HPP
#define CHUNK_HPP

#include "vmath.hpp"

class ChunkLoader;

class Chunk {
public:
	static const uint WIDTH = 32;

private:
	vec3i64 cc;
	ChunkLoader *chunkLoader;
	bool changed = true;
	uint airBlocks = 0;

	uint8 blocks[WIDTH * WIDTH * WIDTH];

	uint16 passThroughs = 0;

public:
	Chunk(vec3i64 cc, ChunkLoader *chunkLoader = nullptr);

	void makePassThroughs();
	vec3i64 getCC() const { return cc; }
	void setCC(vec3i64 cc) { this->cc = cc; }
	void initBlock(size_t index, uint8 type);
	bool setBlock(vec3ui8 icc, uint8 type);
	uint8 getBlock(vec3ui8 icc) const;

	const uint8 *getBlocks() const { return blocks; }
	uint16 getPassThroughs() const { return passThroughs; }
	uint getAirBlocks() const { return airBlocks; }

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
