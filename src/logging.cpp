/*
 * logging.cpp
 *
 *  Created on: 18.09.2014
 *      Author: lars
 */

#include "logging.hpp"

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

