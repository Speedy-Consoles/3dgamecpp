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

int64 getMicroTimeSince(time_point<high_resolution_clock> since) {
	auto dur = high_resolution_clock::now() - since;
	return duration_cast<microseconds>(dur).count();
}

int64 getMilliTimeSince(time_point<steady_clock> since) {
	auto dur = steady_clock::now() - since;
	return duration_cast<milliseconds>(dur).count();
}

namespace my{ namespace time {

int64 get() {
	auto static start = high_resolution_clock::now();
	auto stop = high_resolution_clock::now();
	auto diff = stop - start;
	return duration_cast<microseconds>(diff).count();
}

void sleepFor(time_t dur) {
	this_thread::sleep_for(std::chrono::microseconds(dur));
}

void sleepUntil(time_t t) {
	this_thread::sleep_for(std::chrono::microseconds(t - get()));
}

}} // namespace my::time
