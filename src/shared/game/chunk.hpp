#ifndef CHUNK_HPP
#define CHUNK_HPP

#include "shared/engine/vmath.hpp"

class Chunk {
public:
	static const uint WIDTH_EXPONENT = 5;
	static const uint WIDTH = 1 << WIDTH_EXPONENT;

private:
	bool visual = false;

	vec3i64 cc;
	uint numAirBlocks = 0;
	uint32 revision = 0;
	bool passThroughsInitialized = false;
	bool initialized = false;

	uint8 blocks[WIDTH * WIDTH * WIDTH];

	mutable uint16 passThroughs = 0;

public:
	Chunk(bool visual);

	void initCC(vec3i64 cc) { this->cc = cc; }
	void initRevision(uint32 revision) { this->revision = revision; }
	void initBlock(size_t index, uint8 type);
	void initPassThroughs(uint16 passThroughs);
	uint8 *getBlocksForInit() { return blocks; }
	void finishInitialization();
	void reset();

	void setBlock(size_t index, uint8 type);
	uint8 getBlock(vec3ui8 icc) const;

	vec3i64 getCC() const { return cc; }
	uint32 getRevision() const {return revision; }
	uint16 getPassThroughs() const { return passThroughs; }
	uint getNumAirBlocks() const { return numAirBlocks; }
	const uint8 *getBlocks() const { return blocks; }
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
