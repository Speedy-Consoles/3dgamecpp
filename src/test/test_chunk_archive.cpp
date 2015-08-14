#include "test/gtest.hpp"

#include <cstring>
#include <cstdlib>
#include <random>

#include "shared/engine/std_types.hpp"
#include "shared/game/chunk.hpp"
#include "shared/chunk_archive.hpp"

using namespace testing;

bool operator == (const Chunk &lhs, const Chunk &rhs) {
	const char *plhs = (const char *)lhs.getBlocks();
	const char *prhs = (const char *)rhs.getBlocks();
	size_t width = Chunk::WIDTH;
	size_t size = width * width * width * sizeof(uint8);
	return 0 == memcmp(plhs, prhs, size);
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
	EXPECT_EQ(supposed, actual) << "Air chunk did not store and load properly";
}

TEST(ChunkArchiveTest, UncompressibleChunk) {
	Chunk supposed(false);
	supposed.setCC({ 0, 0, 0 });
	Chunk actual(false);

	initChunk(supposed, [](size_t x, size_t y, size_t z, size_t index) {
		return index % 2 ? index % 254 : 255;
	});

	ASSERT_NO_DEATH(store_and_load(supposed, actual););
	EXPECT_EQ(supposed, actual) << "Checkered chunk did not store and load properly";
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

	ASSERT_NO_DEATH(store_and_load(supposed, actual););
	EXPECT_EQ(supposed, actual) << "Random chunk did not store and load properly";
}