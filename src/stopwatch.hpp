#ifndef STOPWATCH_HPP
#define STOPWATCH_HPP

#include <vector>
#include <stack>

#include "std_types.hpp"
#include "time.hpp"

class Stopwatch {
public:
	Stopwatch(size_t size);

	void start(uint);
	void stop(uint id = -1);

	my::time::time_t get(uint);
	my::time::time_t getTotal();
	float getRel(uint);

	void save();

private:

	struct EntryType {
		my::time::time_t start;
		my::time::time_t dur;
		float rel;
	};

	std::vector<EntryType> _clocks;
	std::stack<uint> _stack;
	my::time::time_t _total = 0;
};

#endif // STOPWATCH_HPP
