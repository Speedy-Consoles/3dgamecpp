/*
 * time.cpp
 *
 *  Created on: 24.09.2014
 *      Author: lars
 */

#include "time.hpp"

#include <chrono>
#include <thread>

using namespace std;
using namespace std::chrono;

namespace my{ namespace time {

int64 now() {
	auto static start = high_resolution_clock::now();
	auto stop = high_resolution_clock::now();
	auto diff = stop - start;
	return duration_cast<microseconds>(diff).count();
}

void sleepFor(time_t dur) {
	this_thread::sleep_for(std::chrono::microseconds(dur));
}

void sleepUntil(time_t t) {
	this_thread::sleep_for(std::chrono::microseconds(t - now()));
}

}} // namespace my::time
