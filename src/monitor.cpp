#include "monitor.hpp"

Monitor::Monitor() :
		rev1(0), rev2(0) {};

void Monitor::startWrite() {
	rev1++;
}

void Monitor::finishWrite() {
	rev2++;
}

int Monitor::startRead() {
	return rev2;
}

bool Monitor::finishRead(int handle) {
	return handle == rev1;
}
