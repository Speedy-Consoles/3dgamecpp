/*
 * logging.cpp
 *
 *  Created on: 18.09.2014
 *      Author: lars
 */

#include "logging.hpp"

#ifndef NO_LOG4CXX

#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>

using namespace log4cxx;
using namespace log4cxx::helpers;

void initLogging() {
	BasicConfigurator::configure();
}

void initLogging(const char *file) {
	PropertyConfigurator::configure(file);
}

void destroyLogging() {
	// nothing
}

#else

namespace logging {
	std::ofstream stream;
}

void initLogging() {
	logging::stream.open("clientw.log", std::ofstream::out | std::ofstream::app);
}

void initLogging(const char *file) {
	logging::stream.open("clientw.log", std::ofstream::out | std::ofstream::app);
}

void destroyLogging() {
	logging::stream.close();
}

#endif
