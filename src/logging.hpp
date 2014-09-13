#ifndef LOGGING_HPP_
#define LOGGING_HPP_

//#define _ELPP_UNICODE
#define _ELPP_NO_DEFAULT_LOG_FILE
#define _ELPP_STACKTRACE_ON_CRASH
#define _ELPP_THREAD_SAFE
//#define _ELPP_FORCE_USE_STD_THREAD

#include "easyloggingpp.h"

#define logOpenGLError() {\
	GLenum e = glGetError();\
	LOG_IF(e != GL_NO_ERROR, ERROR) << gluErrorString(e);\
}

#endif // LOGGING_HPP_
