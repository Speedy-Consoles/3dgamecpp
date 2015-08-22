#ifndef CHUNK_HPP
#define CHUNK_HPP

#include "shared/engine/vmath.hpp"

class Chunk {
public:
	static const uint WIDTH_EXPONENT = 5;
	static const uint WIDTH = 1 << WIDTH_EXPONENT;
	static const uint SIZE = WIDTH * WIDTH * WIDTH;

	enum ChunkFlags {
		VISUAL = 0,
		INITIALIZED = 1,
		COORDS_INITIALIZED = 2,
		NUM_AIR_BLOCKS_INITIALIZED = 4,
		PASSTHROUGHS_INITIALIZED = 8,
	};

private:
	vec3i64 cc;
	uint numAirBlocks = 0;
	uint32 revision = 0;
	uint16 passThroughs = 0;
	uint8 flags = 0;

	uint8 blocks[WIDTH * WIDTH * WIDTH];

public:
	Chunk(int flags = 0);

	void initCC(vec3i64 chunkCoords);
	void initRevision(uint32 revision) { this->revision = revision; }
	void initNumAirBlocks(int numAirBlocks);
	void initPassThroughs(uint16 passThroughs);
	void initBlock(vec3ui8 intraChunkCoords, uint8 type);
	void initBlock(size_t index, uint8 type);
	uint8 *getBlocksForInit() { return blocks; }
	void finishInitialization();
	void reset();

	void setBlock(size_t index, uint8 type);
	uint8 getBlock(vec3ui8 intraChunkCoords) const;

	vec3i64 getCC() const { return cc; }
	uint32 getRevision() const {return revision; }
	uint16 getPassThroughs() const { return passThroughs; }
	uint getNumAirBlocks() const { return numAirBlocks; }
	const uint8 *getBlocks() const { return blocks; }
	bool isEmpty() const { return numAirBlocks == SIZE; }
	bool isVisual() const { return (flags & VISUAL) != 0; }
	bool isInitialized() const { return flags & INITIALIZED; }

/*
	void write(ByteBuffer buffer) const;
	static Chunk readChunk(ByteBuffer buffer);
*/
	static size_t getBlockIndex(vec3ui8 icc);

private:
	void makePassThroughs();
};

#endif // CHUNK_HPP
