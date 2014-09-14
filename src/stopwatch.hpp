#ifndef STOPWATCH_HPP
#define STOPWATCH_HPP

#include <chrono>
#include <vector>
#include <stack>

#include "std_types.hpp"

class Stopwatch {
public:
	using dur_type = std::chrono::nanoseconds;
	using time_point_type = std::chrono::time_point<std::chrono::high_resolution_clock>;

	Stopwatch(size_t size);

	void start(uint);
	void stop(uint id = -1);

	dur_type get(uint);
	dur_type getTotal();
	float getRel(uint);

	void save();

private:

	struct EntryType {
		time_point_type start;
		dur_type dur;
		float rel;
	};

	std::vector<EntryType> _clocks;
	std::stack<uint> _stack;
	dur_type _total;
};

#endif // STOPWATCH_HPP
