#include "test/gtest.hpp"

#include <cstring>
#include <cstdlib>
#include <random>

#include "shared/engine/std_types.hpp"
#include "shared/game/chunk.hpp"
#include "shared/chunk_archive.hpp"

using namespace testing;

float getRelativeChunkDifference(const Chunk &lhs, const Chunk &rhs) {
	size_t failed = 0;

	for (size_t i = 0; i < Chunk::SIZE; ++i) {
		if (lhs.getBlocks()[i] != rhs.getBlocks()[i])
			++failed;
	}

	if (failed == 0)
		return 0.0;
	else
		return (float) failed / Chunk::SIZE;
}

void store_and_load(const Chunk &supposed, Chunk *actual) {
	ChunkArchive archive("./test/temp/");
	archive.storeChunk(supposed);
	actual->initCC(supposed.getCC());
	archive.loadChunk(actual);
}

template <class Func>
void initChunk(Chunk &chunk, Func func) {
	for (size_t index = 0, z = 0; z < Chunk::WIDTH; ++z) {
		for (size_t y = 0; y < Chunk::WIDTH; ++y) {
			for (size_t x = 0; x < Chunk::WIDTH; ++x, index++) {
				chunk.initBlock(index, func(x, y, z, index));
			}
		}
	}
	chunk.finishInitialization();
}

TEST(ChunkArchiveTest, AirChunk) {
	Chunk supposed;
	supposed.initCC({ 0, 0, 0 });
	Chunk actual;

	initChunk(supposed, [](size_t, size_t, size_t, size_t) -> uint8 {
		return 0;
	});
	store_and_load(supposed, &actual);
	EXPECT_EQ(0, getRelativeChunkDifference(supposed, actual)) << "Air chunk did not store and load properly";
}

TEST(ChunkArchiveTest, StoneChunk) {
	Chunk supposed;
	supposed.initCC({ 0, 0, 0 });
	Chunk actual;

	initChunk(supposed, [](size_t, size_t, size_t, size_t) -> uint8 {
		return 1;
	});
	store_and_load(supposed, &actual);
	EXPECT_EQ(0, getRelativeChunkDifference(supposed, actual)) << "Stone chunk did not store and load properly";
}

TEST(ChunkArchiveTest, UncompressibleChunk) {
	Chunk supposed;
	supposed.initCC({ 0, 0, 0 });
	Chunk actual;

	initChunk(supposed, [](size_t, size_t, size_t, size_t index) -> uint8 {
		return index % 2 ? index % 254 : 255;
	});

	ASSERT_NO_DEATH(store_and_load(supposed, &actual);) << "Checkered chunk store and load crashed";
	store_and_load(supposed, &actual);
	EXPECT_EQ(0, getRelativeChunkDifference(supposed, actual)) << "Checkered chunk did not store and load properly";
}

TEST(ChunkArchiveTest, RandomChunk) {
	Chunk supposed;
	supposed.initCC({ 0, 0, 0 });
	Chunk actual;

	std::minstd_rand rng;
	rng.seed(1);
	std::uniform_int_distribution<uint> distr(0, 254);

	initChunk(supposed, [&rng, &distr](size_t, size_t, size_t, size_t) -> uint8 {
		return distr(rng);
	});

	ASSERT_NO_DEATH(store_and_load(supposed, &actual);) << "Random chunk store and load crashed";
	store_and_load(supposed, &actual);
	EXPECT_EQ(0, getRelativeChunkDifference(supposed, actual)) << "Random chunk did not store and load properly";
}

TEST(ChunkArchiveTest, RunLengths) {
	Chunk supposed;
	supposed.initCC({ 0, 0, 0 });
	Chunk actual;

	uint8 data[] = { 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1 };
	int ndata = sizeof(data) / sizeof(uint8);

	initChunk(supposed, [&data, ndata](size_t, size_t, size_t, size_t index) -> uint8 {
		if (index < ndata)
			return data[index];
		else
			return (index / 0x200) % 2;
	});

	store_and_load(supposed, &actual);
	EXPECT_EQ(0, getRelativeChunkDifference(supposed, actual)) << "RunLengths chunk did not store and load properly";
}

TEST(ChunkArchiveTest, FarFromSpawnChunk) {
	Chunk supposed;
	supposed.initCC({ 9999999, 9999999, 0 });
	Chunk actual;

	std::minstd_rand rng;
	rng.seed(1);
	std::uniform_int_distribution<uint> distr(0, 254);

	initChunk(supposed, [&rng, &distr](size_t, size_t, size_t, size_t) -> uint8 {
		return distr(rng);
	});

	ASSERT_NO_DEATH(store_and_load(supposed, &actual);) << "Random chunk store and load crashed";
	store_and_load(supposed, &actual);
	EXPECT_EQ(0, getRelativeChunkDifference(supposed, actual)) << "Far from spawn chunk did not store and load properly";
}

TEST(ChunkArchiveTest, RegionWrapAround) {
	Chunk supposed;
	supposed.initCC({ 0, 0, 0 });
	Chunk actual;

	std::minstd_rand rng;
	rng.seed(1);
	std::uniform_int_distribution<uint> distr(0, 254);

	initChunk(supposed, [&rng, &distr](size_t, size_t, size_t, size_t) {
		return distr(rng);
	});

	ChunkArchive archive("./test/temp/");
	archive.storeChunk(supposed);

	Chunk other;
	other.initCC({ 16, 16, 16 });
	initChunk(other, [&rng, &distr](size_t, size_t, size_t, size_t) {
		return distr(rng);
	});
	archive.storeChunk(other);
	
	actual.initCC(supposed.getCC());
	archive.loadChunk(&actual);

	EXPECT_EQ(0, getRelativeChunkDifference(supposed, actual)) << "Chunks from different regions overlap";
}

TEST(ChunkArchiveTest, SameRegion) {
	Chunk c1;
	Chunk c2;
	Chunk c3;
	c1.initCC({ 0, 0, 0 });
	c2.initCC({ 1, 0, 0 });
	c3.initCC({ 0, 1, 0 });
	Chunk actual;

	std::minstd_rand rng;
	rng.seed(1);
	std::uniform_int_distribution<uint> distr(0, 254);
	
	initChunk(c1, [&rng, &distr](size_t, size_t, size_t, size_t) {return distr(rng);});
	initChunk(c2, [&rng, &distr](size_t, size_t, size_t, size_t) {return distr(rng);});
	initChunk(c3, [&rng, &distr](size_t, size_t, size_t, size_t) {return distr(rng);});

	ChunkArchive archive("./test/temp/");
	archive.storeChunk(c1);
	archive.storeChunk(c2);
	archive.storeChunk(c3);
	
	actual.initCC(c1.getCC());
	archive.loadChunk(&actual);
	ASSERT_EQ(0, getRelativeChunkDifference(c1, actual)) << "Chunks from same region collide";

	actual.initCC(c2.getCC());
	archive.loadChunk(&actual);
	ASSERT_EQ(0, getRelativeChunkDifference(c2, actual)) << "Chunks from same region collide";

	actual.initCC(c3.getCC());
	archive.loadChunk(&actual);
	ASSERT_EQ(0, getRelativeChunkDifference(c3, actual)) << "Chunks from same region collide";
}
