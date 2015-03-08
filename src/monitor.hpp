#ifndef MONITOR_HPP
#define MONITOR_HPP

#include <atomic>

class Monitor {
public:
	using handle_t = int;

    inline Monitor() : rev1(0), rev2(0) { }

    inline void startWrite() { rev1++; }
    inline void finishWrite() { rev2++; }

	// save this handle
    inline handle_t startRead() { return rev2; }

	// pass the handle back in
	// returns true, if the read was successful
    inline bool finishRead(handle_t handle) { return handle == rev1; }

private:
    std::atomic<handle_t> rev1;
    std::atomic<handle_t> rev2;
};

#endif
