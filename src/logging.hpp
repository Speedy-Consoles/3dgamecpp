#ifndef LOGGING_HPP_
#define LOGGING_HPP_

////#define _ELPP_UNICODE
//#define _ELPP_NO_DEFAULT_LOG_FILE
//#ifndef _MSC_VER
//	#define _ELPP_STACKTRACE_ON_CRASH
//#endif
//#define _ELPP_THREAD_SAFE
////#define _ELPP_FORCE_USE_STD_THREAD
//
//#include "easyloggingpp.h"

#include <iostream>

extern std::ofstream cnull;

#define LOG(x) std::cout
#define LOG_IF(b, x) ((bool) b ? std::cout : (std::ostream &) cnull)
#define logOpenGLError() {\
	GLenum e = glGetError();\
	LOG_IF(e != GL_NO_ERROR, ERROR) << gluErrorString(e);\
}

#endif // LOGGING_HPP_
