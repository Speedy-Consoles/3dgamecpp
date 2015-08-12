#ifndef CHUNK_HPP
#define CHUNK_HPP

#include "shared/engine/vmath.hpp"

class Chunk {
public:
	static const uint WIDTH = 32;

	bool initialized = false;

private:
	vec3i64 cc;
	uint airBlocks = 0;

	uint8 blocks[WIDTH * WIDTH * WIDTH];

	uint16 passThroughs = 0;

public:
	Chunk() = default;
	Chunk(vec3i64 cc);

	void makePassThroughs();
	void setCC(vec3i64 cc) { this->cc = cc; }
	void initBlock(size_t index, uint8 type);
	void reset();
	vec3i64 getCC() const { return cc; }
	void setBlock(vec3ui8 icc, uint8 type);
	uint8 getBlock(vec3ui8 icc) const;

	const uint8 *getBlocks() const { return blocks; }
	uint16 getPassThroughs() const { return passThroughs; }
	uint getAirBlocks() const { return airBlocks; }

/*
	void write(ByteBuffer buffer) const;
	static Chunk readChunk(ByteBuffer buffer);
*/
	static size_t getBlockIndex(vec3ui8 icc);
};

#endif // CHUNK_HPP
