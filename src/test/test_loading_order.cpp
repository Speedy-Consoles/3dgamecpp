#include "test/gtest.hpp"

#include <unordered_set>

#include "shared/constants.hpp"
#include "shared/block_utils.hpp"

using namespace testing;

namespace {

class LoadingOrderTest : public ::testing::Test {
protected:
	LoadingOrderTest() {
		initUtil();
	}

	virtual ~LoadingOrderTest() {
		LOADING_ORDER = std::vector<vec3i8>();
		LO_MAX_RADIUS_INDICES = std::vector<int>();
		LO_INDEX_FINISHED_RADIUS = std::vector<int>();
	}
};

}

TEST_F(LoadingOrderTest, Correctness) {
	int range = MAX_RENDER_DISTANCE;
	int length = range * 2 + 1;
	std::unordered_set<vec3i64, size_t(*)(vec3i64)> set(0, vec3i64HashFunc);

	for (int i = 0; i < length * length * length; ++i) {
		vec3i64 cd = LOADING_ORDER[i].cast<int64>();
		ASSERT_TRUE(set.find(cd) == set.end())
				<< "Chunk offset "
				<< cd[0] << "," << cd[1] << "," << cd[2] <<
				" is already in loading order";
		set.insert(cd);
	}

	for (int z = -range; z <= range; z++) {
		for (int y = -range; y <= range; y++) {
			for (int x = -range; x <= range; x++) {
				vec3i64 cd(x, y, z);
				ASSERT_FALSE(set.find(cd) == set.end())
						<< "Chunk offset "
						<< x << "," << y << "," << z <<
						" is not in loading order";
			}
		}
	}

	int finishedRadius = -1;
	int maxRadius = 0;
	for (int i = 0; i < length * length * length; ++i) {
		vec3i8 cd = LOADING_ORDER[i];
		int radius = (int) std::ceil(cd.norm());
		while (radius > maxRadius) {
			ASSERT_EQ(i, LO_MAX_RADIUS_INDICES[maxRadius])
					<< "max radius index wrong";
			maxRadius++;
		}
		ASSERT_GE(cd.norm(), finishedRadius)
				<< "chunk " << i << " is behind finished radius";
		ASSERT_GE(LO_INDEX_FINISHED_RADIUS[i], finishedRadius)
				<< "finished radius decreased";
		if (LO_INDEX_FINISHED_RADIUS[i] > finishedRadius) {
			if (i > 0) {
				vec3i8 cd = LOADING_ORDER[i - 1];
				ASSERT_LE(cd.norm(), finishedRadius + 1)
						<< "finished radius is behind";
			}
			finishedRadius = LO_INDEX_FINISHED_RADIUS[i];
		}
	}
}
