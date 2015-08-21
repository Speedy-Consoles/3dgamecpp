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
		LOADING_ORDER_DISTANCE_INDICES = std::vector<int>();
		LOADING_ORDER_INDEX_DISTANCES = std::vector<int>();
	}
};

}

TEST_F(LoadingOrderTest, Correctness) {
	int range = MAX_RENDER_DISTANCE;
	int length = range * 2 + 1;
	int loadedDistance = 0;
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

	for (int i = 0; i < length * length * length; ++i) {
		vec3i8 cd = LOADING_ORDER[i];
		ASSERT_GE(cd.norm(), loadedDistance) << "chunk " << i << " is behind loaded distance";
		ASSERT_GE(LOADING_ORDER_INDEX_DISTANCES[i], loadedDistance) << "Loaded distance decreased";
		loadedDistance = LOADING_ORDER_INDEX_DISTANCES[i];
	}

	loadedDistance = -1;
	for (int i = 0; i < length * length * length; ++i) {
		if (LOADING_ORDER_INDEX_DISTANCES[i] > loadedDistance) {
			loadedDistance = LOADING_ORDER_INDEX_DISTANCES[i];
			if (i > 0) {
				vec3i8 cd = LOADING_ORDER[i - 1];
				ASSERT_LT(cd.norm(), loadedDistance)
					<< "Loaded distance is behind";
			}
			ASSERT_EQ(i, LOADING_ORDER_DISTANCE_INDICES[loadedDistance])
					<< "Loaded distance index wrong, loaded distance: "
					<< loadedDistance;
		}
	}
}
