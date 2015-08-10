#ifndef STOPWATCH_HPP
#define STOPWATCH_HPP

#include <vector>
#include <stack>

#include "shared/engine/std_types.hpp"
#include "shared/engine/time.hpp"

class Stopwatch {
public:
	Stopwatch(size_t size);

	void start(uint);
	void stop(uint id = -1);

	Time get(uint);
	Time getTotal();
	float getRel(uint);

	void save();

private:

	struct EntryType {
        Time start;
        Time dur;
		float rel;
	};

	std::vector<EntryType> _clocks;
	std::stack<uint> _stack;
    Time _total = 0;
};

#endif // STOPWATCH_HPP
