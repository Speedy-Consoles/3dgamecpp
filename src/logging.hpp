#ifndef LOGGING_HPP_
#define LOGGING_HPP_

#include <log4cxx/logger.h>

#define NAMED_LOGGER(name) log4cxx::Logger::getLogger(name)
//#define DEFAULT_LOGGER log4cxx::Logger::getRootLogger()
#define DEFAULT_LOGGER NAMED_LOGGER("default")
#define LOG_TO(logger, sev, msg) LOG4CXX_LOG(logger, sev, msg)
#define LOG(sev, msg) LOG_TO(DEFAULT_LOGGER, sev, msg)



#define FATAL ::log4cxx::Level::getFatal()
#define ERROR ::log4cxx::Level::getError()
#define WARNING ::log4cxx::Level::getWarn()
#define INFO ::log4cxx::Level::getInfo()
#define DEBUG ::log4cxx::Level::getDebug()
#define TRACE ::log4cxx::Level::getTrace()

#define logOpenGLError() {\
	GLenum e = glGetError();\
	if (e != GL_NO_ERROR) LOG(ERROR, gluErrorString(e));\
}

void initLogging();
void initLogging(const char *file);
void destroyLogging();

#endif // LOGGING_HPP_
