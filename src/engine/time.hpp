/*
 * time.hpp
 *
 *  Created on: 24.09.2014
 *      Author: lars
 */

#ifndef TIME_HPP_
#define TIME_HPP_

#include "std_types.hpp"

// all times should be measured in this
using Time = int64;

inline Time micros(Time n)  { return n; }
inline Time millis(long n)  { return n * 1000; }
inline Time seconds(long n) { return n * 1000 * 1000; }
inline Time mins(long n)    { return n * 1000 * 1000 * 60; }
inline Time hours(long n)   { return n * 1000 * 1000 * 60 * 60; }
inline Time days(long n)    { return n * 1000 * 1000 * 60 * 60 * 24; }

// get the current time
Time getCurrentTime();

// general waiting functions
void sleepFor(Time);
void sleepUntil(Time);

#endif // TIME_HPP_
