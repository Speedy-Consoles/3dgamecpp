#include "logging.hpp"

#include <iostream>
#include <mutex>

static std::mutex mutex;

#ifndef NO_LOG4CXX

#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>

using namespace log4cxx;
using namespace log4cxx::helpers;

namespace logging {

void init() {
	BasicConfigurator::configure();
}

void init(const char *file) {
	PropertyConfigurator::configure(file);
}

static Level level_all((int)Severity::ALL, "all");
static Level level_trace((int)Severity::TRACE, "trace");
static Level level_debug((int)Severity::DEBUG, "debug");
static Level level_unspecified((int)Severity::UNSPECIFIED, "unspecified");
static Level level_info((int)Severity::INFO, "info");
static Level level_warning((int)Severity::WARNING, "warning");
static Level level_error((int)Severity::ERROR, "error");
static Level level_fatal((int)Severity::FATAL, "fatal");
static Level level_off((int)Severity::OFF, "off");

LevelPtr getLevel(Severity sev) {
	switch (log.sev) {
		default:
		case Severity::UNSPECIFIED: return level_unspecified;
		case Severity::ALL:         return level_all;
		case Severity::TRACE:       return level_trace;
		case Severity::DEBUG:       return level_debug;
		case Severity::INFO:        return level_info;
		case Severity::WARNING:     return level_warning;
		case Severity::ERROR:       return level_error;
		case Severity::FATAL:       return level_fatal;
		case Severity::OFF:         return level_off;
	}
}

void Logger::submit(const Log &log) {
	auto logger = ::log4cxx::Logger::getLogger(name);

	static Level unspecified()
	LevelPtr level = getLevel(log.sev);

	const char *sev_str = nullptr;
	switch (log.sev) {
		default:
		case Severity::UNSPECIFIED: sev_str = "   "; break;
		case Severity::TRACE:       sev_str = "TRC"; break;
		case Severity::DEBUG:       sev_str = "DBG"; break;
		case Severity::INFO:        sev_str = "INF"; break;
		case Severity::WARNING:     sev_str = "WRN"; break;
		case Severity::ERROR:       sev_str = "ERR"; break;
		case Severity::FATAL:       sev_str = "FAT"; break;
	}

	LOG4CXX_LOG(logger, sev, msg)
	printf("[%s] %s:%s:%d: %s\n", sev_str, log.file, log.func, log.line, log.msg.str().c_str());
}

} // namespace logging

#else

namespace logging {

static std::ofstream sink;

void init() {
	sink.open("client.log", ::std::ofstream::out | ::std::ofstream::app);
}

void init(const char *) {
	init();
}

void Logger::submit(const Log &log) {
	const char *sev_str = nullptr;
	switch (log.sev) {
		default:
		case Severity::UNSPECIFIED: sev_str = "   "; break;
		case Severity::TRACE:       sev_str = "TRC"; break;
		case Severity::DEBUG:       sev_str = "DBG"; break;
		case Severity::INFO:        sev_str = "INF"; break;
		case Severity::WARNING:     sev_str = "WRN"; break;
		case Severity::ERROR:       sev_str = "ERR"; break;
		case Severity::FATAL:       sev_str = "FAT"; break;
	}

	std::stringstream ss;
	ss << "[" << sev_str << "] " << name << ": "
	   << log.msg.str();
	std::string console_str = ss.str();
	ss.clear();
	ss << "[" << sev_str << "] " << name << " "
	   << log.file << ":" << log.func << ":" << log.line << ": "
	   << log.msg.str();
	std::string file_str = ss.str();

	std::lock_guard<std::mutex> lock(mutex);
	std::cout << console_str << std::endl;
	sink << file_str << std::endl;
}

} // namespace logging

#endif // NO_LOG4CXX

using namespace logging;

std::unique_ptr<Log> Logger::log() {
	return std::unique_ptr<Log>(new Log(this));
}

std::unique_ptr<Log> Logger::trace() {
	auto l = log();
	l << Severity::TRACE;
	return l;
}

std::unique_ptr<Log> Logger::debug() {
	auto l = log();
	l << Severity::DEBUG;
	return l;
}

std::unique_ptr<Log> Logger::info() {
	auto l = log();
	l << Severity::INFO;
	return l;
}

std::unique_ptr<Log> Logger::warning() {
	auto l = log();
	l << Severity::WARNING;
	return l;
}

std::unique_ptr<Log> Logger::error() {
	auto l = log();
	l << Severity::ERROR;
	return l;
}

std::unique_ptr<Log> Logger::fatal() {
	auto l = log();
	l << Severity::FATAL;
	return l;
}

void Log::submit() {
	logger->submit(*this);
}

using namespace logging;

namespace std {
	template<>
	void default_delete<Log>::operator()(Log* log) const {
		try {
			log->submit();
		} catch (...) {
			fprintf(stderr, "Exception thrown while trying to log another error!\n");
		}
		delete log;
	}
}

std::unique_ptr<Log> &operator << (std::unique_ptr<Log> &log, Severity sev) {
	log->sev = sev;
	return log;
}

std::unique_ptr<Log> &operator << (std::unique_ptr<Log> &log, LineNumber indicator) {
	log->line = indicator.line;
	return log;
}

std::unique_ptr<Log> &operator << (std::unique_ptr<Log> &log, FileName indicator) {
	log->file = indicator.name;
	return log;
}

std::unique_ptr<Log> &operator << (std::unique_ptr<Log> &log, FunctionName indicator) {
	log->func = indicator.name;
	return log;
}
