#ifndef LOGGING_HPP_
#define LOGGING_HPP_

#include <sstream>
#include <fstream>
#include <memory>

#undef ERROR

namespace logging {

void init();
void init(const char *);

enum class Severity {
	ALL,
	TRACE,
	DEBUG,
	UNSPECIFIED,
	INFO,
	WARNING,
	ERROR,
	FATAL,
	OFF,
};

class Logger;
class Log;

class Logger {
	std::string name;

public:
	Logger() : name("default") {}
	explicit Logger(std::string name) : name(name) {}
	~Logger() = default;

	Logger(const Logger &) = delete;
	Logger &operator = (const Logger &) = delete;
	
	::std::unique_ptr<Log> log();

	::std::unique_ptr<Log> trace();
	::std::unique_ptr<Log> debug();
	::std::unique_ptr<Log> info();
	::std::unique_ptr<Log> warning();
	::std::unique_ptr<Log> error();
	::std::unique_ptr<Log> fatal();

private:
	friend Log;
	void submit(const Log &log);
};

class Log {
	Logger *logger = nullptr;
public:
	Log() = delete;
	Log(Logger *logger) : logger(logger) {}
	~Log() = default;

	Log(const Log &) = delete;
	Log &operator = (const Log &) = delete;

	void submit();

	Severity sev = Severity::UNSPECIFIED;
	long line = 0;
	const char *file = nullptr;
	const char *func = nullptr;
	::std::stringstream msg;
};

struct LineNumber { long line; };
struct FileName { const char *name; };
struct FunctionName { const char *name; };

} // namespace logging

namespace std {
	template<>
	void default_delete<logging::Log>::operator()(logging::Log* log) const;
}

std::unique_ptr<logging::Log> &&operator << (std::unique_ptr<logging::Log> &&, logging::Severity sev);
std::unique_ptr<logging::Log> &&operator << (std::unique_ptr<logging::Log> &&, logging::LineNumber indicator);
std::unique_ptr<logging::Log> &&operator << (std::unique_ptr<logging::Log> &&, logging::FileName indicator);
std::unique_ptr<logging::Log> &&operator << (std::unique_ptr<logging::Log> &&, logging::FunctionName indicator);

template <typename T>
std::unique_ptr<logging::Log> &&operator << (std::unique_ptr<logging::Log> &&log, T t) {
	log->msg << t;
	return std::move(log);
}

#define LOG_ENV \
		::logging::LineNumber{__LINE__} << \
		::logging::FileName{__FILE__} << \
		::logging::FunctionName{__FUNCTION__}

#define LOG_TRACE(logger) logger.trace() << LOG_ENV
#define LOG_DEBUG(logger) logger.debug() << LOG_ENV
#define LOG_INFO(logger) logger.info() << LOG_ENV
#define LOG_WARNING(logger) logger.warning() << LOG_ENV
#define LOG_ERROR(logger) logger.error() << LOG_ENV
#define LOG_FATAL(logger) logger.fatal() << LOG_ENV

static logging::Logger __opengl_logger("ogl");

#define LOG_OPENGL_ERROR {\
	GLenum e = glGetError();\
	if (e != GL_NO_ERROR) LOG_ERROR(__opengl_logger) << gluErrorString(e); \
}

#define GL(x) {\
	gl ## x;\
	LOG_OPENGL_ERROR;\
}

#endif // LOGGING_HPP_
