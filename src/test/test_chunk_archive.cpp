#include "test/gtest.hpp"

#include <cstring>
#include <cstdlib>
#include <random>

#include "shared/engine/std_types.hpp"
#include "shared/game/chunk.hpp"
#include "shared/chunk_archive.hpp"

using namespace testing;

float getRelativeChunkDifference(const Chunk &lhs, const Chunk &rhs) {
	size_t size = Chunk::WIDTH * Chunk::WIDTH * Chunk::WIDTH;
	size_t failed = 0;

	for (size_t i = 0; i < size; ++i) {
		if (lhs.getBlocks()[i] != rhs.getBlocks()[i])
			++failed;
	}

	if (failed == 0)
		return 0.0;
	else
		return (float) failed / size;
}

void store_and_load(const Chunk &supposed, Chunk &actual) {
	ChunkArchive archive("./test/temp/");
	archive.storeChunk(supposed);
	actual.setCC(supposed.getCC());
	archive.loadChunk(actual);
}

template <class Func>
void initChunk(Chunk &chunk, Func func) {
	for (size_t z = 0; z < Chunk::WIDTH; ++z) {
		for (size_t y = 0; y < Chunk::WIDTH; ++y) {
			for (size_t x = 0; x < Chunk::WIDTH; ++x) {
				size_t index = x + Chunk::WIDTH * (y + Chunk::WIDTH * z);
				chunk.initBlock(index, func(x, y, z, index));
			}
		}
	}
}

TEST(ChunkArchiveTest, AirChunk) {
	Chunk supposed(false);
	supposed.setCC({ 0, 0, 0 });
	Chunk actual(false);

	initChunk(supposed, [](size_t x, size_t y, size_t z, size_t index) {
		return 0;
	});
	store_and_load(supposed, actual);
	EXPECT_EQ(0, getRelativeChunkDifference(supposed, actual)) << "Air chunk did not store and load properly";
}

TEST(ChunkArchiveTest, UncompressibleChunk) {
	Chunk supposed(false);
	supposed.setCC({ 0, 0, 0 });
	Chunk actual(false);

	initChunk(supposed, [](size_t x, size_t y, size_t z, size_t index) {
		return index % 2 ? index % 254 : 255;
	});

	ASSERT_NO_DEATH(store_and_load(supposed, actual);) << "Checkered chunk store and load crashed";
	store_and_load(supposed, actual);
	EXPECT_EQ(0, getRelativeChunkDifference(supposed, actual)) << "Checkered chunk did not store and load properly";
}

TEST(ChunkArchiveTest, RandomChunk) {
	Chunk supposed(false);
	supposed.setCC({ 0, 0, 0 });
	Chunk actual(false);

	std::minstd_rand rng;
	rng.seed(1);
	std::uniform_int_distribution<uint> distr(0, 254);

	initChunk(supposed, [&rng, &distr](size_t x, size_t y, size_t z, size_t index) {
		return distr(rng);
	});

	ASSERT_NO_DEATH(store_and_load(supposed, actual);) << "Random chunk store and load crashed";
	store_and_load(supposed, actual);
	EXPECT_EQ(0, getRelativeChunkDifference(supposed, actual)) << "Random chunk did not store and load properly";
}

TEST(ChunkArchiveTest, FarFromSpawnChunk) {
	Chunk supposed(false);
	supposed.setCC({ 9999999, 9999999, 0 });
	Chunk actual(false);

	std::minstd_rand rng;
	rng.seed(1);
	std::uniform_int_distribution<uint> distr(0, 254);

	initChunk(supposed, [&rng, &distr](size_t x, size_t y, size_t z, size_t index) {
		return distr(rng);
	});

	ASSERT_NO_DEATH(store_and_load(supposed, actual);) << "Random chunk store and load crashed";
	store_and_load(supposed, actual);
	EXPECT_EQ(0, getRelativeChunkDifference(supposed, actual)) << "Far from spawn chunk did not store and load properly";
}

TEST(ChunkArchiveTest, RegionWrapAround) {
	Chunk supposed(false);
	supposed.setCC({ 0, 0, 0 });
	Chunk actual(false);

	std::minstd_rand rng;
	rng.seed(1);
	std::uniform_int_distribution<uint> distr(0, 254);

	initChunk(supposed, [&rng, &distr](size_t x, size_t y, size_t z, size_t index) {
		return distr(rng);
	});

	ChunkArchive archive("./test/temp/");
	archive.storeChunk(supposed);

	Chunk other(false);
	other.setCC({ 16, 16, 16 });
	initChunk(other, [&rng, &distr](size_t x, size_t y, size_t z, size_t index) {
		return distr(rng);
	});
	archive.storeChunk(other);
	
	actual.setCC(supposed.getCC());
	archive.loadChunk(actual);

	EXPECT_EQ(0, getRelativeChunkDifference(supposed, actual)) << "Chunks from different regions overlap";
}

TEST(ChunkArchiveTest, SameRegion) {
	Chunk c1(false);
	Chunk c2(false);
	Chunk c3(false);
	c1.setCC({ 0, 0, 0 });
	c2.setCC({ 1, 0, 0 });
	c3.setCC({ 0, 1, 0 });
	Chunk actual(false);

	std::minstd_rand rng;
	rng.seed(1);
	std::uniform_int_distribution<uint> distr(0, 254);
	
	initChunk(c1, [&rng, &distr](size_t x, size_t y, size_t z, size_t index) {return distr(rng);});
	initChunk(c2, [&rng, &distr](size_t x, size_t y, size_t z, size_t index) {return distr(rng);});
	initChunk(c3, [&rng, &distr](size_t x, size_t y, size_t z, size_t index) {return distr(rng);});

	ChunkArchive archive("./test/temp/");
	archive.storeChunk(c1);
	archive.storeChunk(c2);
	archive.storeChunk(c3);
	
	actual.setCC(c1.getCC());
	archive.loadChunk(actual);
	ASSERT_EQ(0, getRelativeChunkDifference(c1, actual)) << "Chunks from same region collide";

	actual.setCC(c2.getCC());
	archive.loadChunk(actual);
	ASSERT_EQ(0, getRelativeChunkDifference(c2, actual)) << "Chunks from same region collide";

	actual.setCC(c3.getCC());
	archive.loadChunk(actual);
	ASSERT_EQ(0, getRelativeChunkDifference(c3, actual)) << "Chunks from same region collide";
}