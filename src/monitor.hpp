#ifndef MONITOR_HPP
#define MONITOR_HPP

#include <atomic>

class Monitor {
public:
	using read_access_handle = int;

	Monitor();

	void startWrite();
	void finishWrite();

	// save this handle
	int startRead();

	// pass the handle back in
	// returns true, if the read was successful
	bool finishRead(int);

private:
	std::atomic<int> rev1;
	std::atomic<int> rev2;
};

#endif
