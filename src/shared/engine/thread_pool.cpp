#include "thread_pool.hpp"

#include "thread.hpp"
#include "logging.hpp"

static logging::Logger logger("threads");

class PoolThread : public Thread {
	ThreadPool *_pool = nullptr;
	ThreadPool::Task *_task = nullptr;

public:
	PoolThread(ThreadPool *pool) : _pool(pool) {}
	
	void doWork() override;
	void onStop() override;
};

void PoolThread::doWork() {
	if (!_task) {
		bool has_work = false;
		if (!has_work || !_task)
			has_work = _pool->_task_in_queue_priority.pop(_task);
		if (!has_work || !_task)
			has_work = _pool->_task_in_queue.pop(_task);
	}

	if (_task) {
		bool has_finished = !_task->progress_lambda(_task->data);
		if (has_finished) {
			bool could_push = _pool->_task_out_queue.push(_task);
			if (could_push)
				_task = nullptr;
		}
	} else {
		std::this_thread::yield();
	}
}

void PoolThread::onStop() {
	if (_task) {
		bool could_push = _pool->_task_in_queue_priority.push(_task);
		if (!could_push) {
			LOG_ERROR(logger) << "Lost ThreadPool::Task because priority queue was full";
		}
	}
}

ThreadPool::ThreadPool(int num) :
	_num_active_threads(num),
	_task_in_queue(1000),
	_task_in_queue_priority(100),
	_task_out_queue(1000)
{
	_threads.reserve(num);
	growPool(num);
}

ThreadPool::~ThreadPool() {
	requestTermination();
	waitFor(seconds(10));
	for (Thread *thread : _threads) {
		delete thread;
	}
}

void ThreadPool::resize(int num) {
	if (num < 0)
		num = 0;

	if (num > _num_active_threads) {
		_threads.reserve(num);
		growPool(num - _num_active_threads);
	} else if (num < _num_active_threads) {
		shrinkPool(_num_active_threads - num);
	}
}

void ThreadPool::requestTermination() {
	for (Thread *thread : _threads) {
		thread->requestTermination();
	}
}

void ThreadPool::wait() {
	for (Thread *thread : _threads) {
		thread->wait();
	}
}

bool ThreadPool::waitFor(Time t) {
	Time timout = getCurrentTime() + t;
	for (Thread *thread : _threads) {
		if (thread->waitUntil(timout) == false)
			return false;
	}
	return true;
}

bool ThreadPool::waitUntil(Time t) {
	for (Thread *thread : _threads) {
		if (thread->waitUntil(t) == false)
			return false;
	}
	return true;
}

void ThreadPool::schedule(
	progress_lambda_t progress_lambda,
	finish_lambda_t finish_lambda,
	void *data
) {
	schedule(progress_lambda, finish_lambda, [](void *){}, data);
}

void ThreadPool::schedule(
	progress_lambda_t progress_lambda,
	finish_lambda_t finish_lambda,
	finish_lambda_t cancel_lambda,
	void *data
) {
	Task *task = new Task{ progress_lambda, finish_lambda, cancel_lambda, data };
	_task_in_queue.push(task);
}

void ThreadPool::finishTasks() {
	if ((int) _threads.size() > _num_active_threads) {
		auto iter = _threads.begin();
		while (iter != _threads.end()) {
			bool has_finished = (*iter)->waitFor(0);
			if (has_finished) {
				Thread *thread = *iter;
				delete thread;
				iter = _threads.erase(iter);
			} else {
				++iter;
			}
		}
	}
	Task *task;
	while (_task_out_queue.pop(task)) {
		task->finish_lambda(task->data);
		delete task;
	}
}

void ThreadPool::growPool(int num) {
	for (int i = 0; i < num; ++i) {
		PoolThread *thread = new PoolThread(this);
		thread->dispatch();
		_threads.push_back(thread);
	}
	_num_active_threads += num;
}

void ThreadPool::shrinkPool(int num) {
	auto iter = _threads.rbegin();
	for (int i = 0; i < num; ++i) {
		while (iter != _threads.rend() && (*iter)->isTerminationRequested()) {
			iter++;
		}
		if (iter != _threads.rend())
			return;
		(*iter)->requestTermination();
		_num_active_threads--;
	}
}
