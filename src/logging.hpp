#ifndef LOGGING_HPP_
#define LOGGING_HPP_

#include <boost/log/trivial.hpp>

#define LOG(sev) BOOST_LOG_TRIVIAL(sev)

#define logOpenGLError() {\
	GLenum e = glGetError();\
	if (e != GL_NO_ERROR) LOG(error) << gluErrorString(e);\
}

#endif // LOGGING_HPP_
