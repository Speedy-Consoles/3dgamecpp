#ifndef CHUNK_HPP
#define CHUNK_HPP

#include "shared/engine/vmath.hpp"

class Chunk {
public:
	static const uint WIDTH = 32;

private:
	bool visual = false;

	vec3i64 cc;
	uint airBlocks = 0;
	uint32 revision = 0;
	bool initialized = false;

	uint8 blocks[WIDTH * WIDTH * WIDTH];

	mutable uint16 passThroughs = 0;

public:
	Chunk(bool visual);

	void setCC(vec3i64 cc) { this->cc = cc; }
	void setRevision(uint32 revision) { this->revision = revision; }
	void initBlock(size_t index, uint8 type);
	void finishInitialization();
	void reset();
	vec3i64 getCC() const { return cc; }
	void setBlock(size_t index, uint8 type);
	uint8 getBlock(vec3ui8 icc) const;

	const uint8 *getBlocks() const { return blocks; }
	uint16 getPassThroughs() const { return passThroughs; }
	uint getAirBlocks() const { return airBlocks; }
	uint32 getRevision() const {return revision; }
	bool isInitialized() const { return initialized; }

/*
	void write(ByteBuffer buffer) const;
	static Chunk readChunk(ByteBuffer buffer);
*/
	static size_t getBlockIndex(vec3ui8 icc);

private:
	void makePassThroughs() const;
};

#endif // CHUNK_HPP
