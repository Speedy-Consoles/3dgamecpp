#include "test/gtest.hpp"

#include <cstring>
#include <atomic>

#include "shared/engine/thread_pool.hpp"
#include "shared/engine/thread.hpp"

using namespace testing;

static const int NUM = 1000000;
static const int TASK_NUM = 40;

bool progress_counting(int *i) {
	(*i)++;
	if (*i >= NUM) {
		return false;
	} else {
		return true;
	}
}

TEST(ThreadPoolTest, AirChunk) {
	ThreadPool pool(4);
	int a[TASK_NUM];
	std::atomic<int> num_finished;
	num_finished = 0;
	memset(a, 0, sizeof(a));

	for (int i = 0; i < TASK_NUM; ++i) {
		pool.schedule(
			[](void *data) {
				int *i = reinterpret_cast<int *>(data);
				return progress_counting(i);
			},
			[&num_finished](void *) {
				num_finished++;
			},
			&a[i]
		);
	}

	Time timeout = getCurrentTime() + seconds(10);

	while (num_finished < TASK_NUM && getCurrentTime() < timeout) {
		pool.finishTasks();
		ThisThread::yield();
	}

	ASSERT_EQ(TASK_NUM, num_finished);
	for (int i = 0; i < TASK_NUM; ++i) {
		ASSERT_EQ(NUM, a[i]);
	}

	pool.requestTermination();
}
