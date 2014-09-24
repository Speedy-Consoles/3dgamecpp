/*
 * time.hpp
 *
 *  Created on: 24.09.2014
 *      Author: lars
 */

#ifndef TIME_HPP_
#define TIME_HPP_

#include "std_types.hpp"

#include <chrono>

int64 getMicroTimeSince(std::chrono::time_point<std::chrono::high_resolution_clock>);
int64 getMilliTimeSince(std::chrono::time_point<std::chrono::steady_clock>);

namespace my { namespace time {

// all times should be measured in this
using time_t = int64;

// all times are measured in microseconds
time_t get();

void sleepFor(time_t);
void sleepUntil(time_t);

inline time_t micros(time_t n) { return n; }
inline time_t millis(int n)    { return n * 1000; }
inline time_t seconds(int n)   { return n * 1000 * 1000; }
inline time_t mins(int n )     { return n * 1000 * 1000 * 60; }
inline time_t hours(int n )    { return n * 1000 * 1000 * 60 * 60; }
inline time_t days(int n)      { return n * 1000 * 1000 * 60 * 60 * 24; }

}} // namespace my::time

#endif // TIME_HPP_
