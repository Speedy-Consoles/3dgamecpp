#include "stopwatch.hpp"
#include "io/logging.hpp"

using namespace my;

Stopwatch::Stopwatch(size_t size) : _clocks(size) {
	auto now = time::now();
	for (EntryType &entry : _clocks) {
		entry.dur = 0;
		entry.start = now;
		entry.rel = 0.0;
	}
	_clocks[size - 1].rel = 1.0;
}

void Stopwatch::start(uint id) {
	auto now = time::now();
	if (!_stack.empty()) {
		EntryType &old_entry = _clocks[_stack.top()];
		old_entry.dur += now - old_entry.start;
		_total += now - old_entry.start;
	}
	EntryType &entry = _clocks[id];
	entry.start = now;
	_stack.push(id);
}

void Stopwatch::stop(uint id) {
	if (_stack.empty()) {
		LOG(DEBUG, "No clock to stop");
		return;
	}
	if (id == (uint) -1) {
		LOG(DEBUG, "Stopped clock " << _stack.top()
				<< " without explicit id given");
	} else if (id != _stack.top()) {
		LOG(DEBUG, "Stopped clock " << _stack.top()
				<< " but " << id << " given");
	}
	auto now = time::now();
	EntryType &entry = _clocks[_stack.top()];
	entry.dur += now - entry.start;
	_total += now - entry.start;
	_stack.pop();
	if (!_stack.empty()) {
		EntryType &old_entry = _clocks[_stack.top()];
		old_entry.start = now;
	}
}

time::time_t Stopwatch::get(uint id) {
	return _clocks[id].dur;
}

time::time_t Stopwatch::getTotal() {
	return _total;
}

float Stopwatch::getRel(uint id) {
	return _clocks[id].rel;
}

void Stopwatch::save() {
	while (!_stack.empty()) {
		LOG(DEBUG, "stopped clock " << _stack.top() << " unnessessarily");
		stop();
	}

	for (auto &clock: _clocks) {
		clock.rel = (double) clock.dur / _total;
		clock.dur = 0;
	}

	_total = 0;
}
