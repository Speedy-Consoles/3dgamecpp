#ifndef THREAD_POOL_HPP_
#define THREAD_POOL_HPP_

#include <functional>
#include <vector>

#include "queue.hpp"
#include "time.hpp"

class PoolThread;

class ThreadPool {
public:
	typedef std::function<bool(void *)> progress_lambda_t;
	typedef std::function<void(void *)> finish_lambda_t;

	ThreadPool(int num = 4);
	~ThreadPool();

	ThreadPool(const ThreadPool &) = delete;
	ThreadPool &operator=(const ThreadPool &) = delete;

	void resize(int num);
	void requestTermination();
	void wait();
	bool waitFor(Time);
	bool waitUntil(Time);
	
	void schedule(progress_lambda_t progress_lambda,
		finish_lambda_t finish_lambda, void *data);
	void schedule(progress_lambda_t progress_lambda,
		finish_lambda_t finish_lambda, finish_lambda_t cancel_lambda, void *data);
	void finishTasks();

private:
	friend PoolThread;

	struct Task {
		std::function<bool(void *)> progress_lambda;
		std::function<void(void *)> finish_lambda;
		std::function<void(void *)> cancel_lambda;
		void *data;
	};
	
	void growPool(int num = 1);
	void shrinkPool(int num = 1);
	
	std::vector<PoolThread *> _threads;
	int _num_active_threads = 0;
	
	ProducerQueue<Task *> _task_in_queue;
	ProducerQueue<Task *> _task_in_queue_priority;
	ProducerQueue<Task *> _task_out_queue;
};

#endif // THREAD_POOL_HPP_
