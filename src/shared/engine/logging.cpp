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

LevelPtr getLevel(Severity sev) {
	switch (sev) {
		default:
		case Severity::UNSPECIFIED: return Level::getDebug();
		case Severity::ALL:         return Level::getAll();
		case Severity::TRACE:       return Level::getTrace();
		case Severity::DEBUG:       return Level::getDebug();
		case Severity::INFO:        return Level::getInfo();
		case Severity::WARNING:     return Level::getWarn();
		case Severity::ERROR:       return Level::getError();
		case Severity::FATAL:       return Level::getFatal();
		case Severity::OFF:         return Level::getOff();
	}
}

void Logger::submit(const Log &log) {
	::log4cxx::spi::LocationInfo loc_info(log.file, log.func, log.line);

	auto logger = ::log4cxx::Logger::getLogger(name);
	if (logger->isEnabledFor(getLevel(log.sev))) {
		::log4cxx::helpers::MessageBuffer oss_;
		logger->forcedLog(getLevel(log.sev), oss_.str(oss_ << log.msg.str()), loc_info);
	}
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

	std::stringstream console_ss;
	console_ss << "[" << sev_str << "] " << name << ": "
	   << log.msg.str();
	std::string console_str = console_ss.str();

	std::stringstream file_ss;
	file_ss << "[" << sev_str << "] " << name << " "
	   << log.file << ":" << log.func << ":" << log.line << ": "
	   << log.msg.str();
	std::string file_str = file_ss.str();

	std::lock_guard<std::mutex> lock(mutex);
	std::cout << console_str << std::endl;
	sink << file_str << std::endl;
}

} // namespace logging

#endif // NO_LOG4CXX

using namespace logging;

std::unique_ptr<Log> logging::Logger::log() {
	return std::unique_ptr<Log>(new Log(this));
}

std::unique_ptr<Log> logging::Logger::trace() {
	return log() << Severity::TRACE;
}

std::unique_ptr<Log> logging::Logger::debug() {
	return log() << Severity::DEBUG;
}

std::unique_ptr<Log> logging::Logger::info() {
	return log() << Severity::INFO;
}

std::unique_ptr<Log> logging::Logger::warning() {
	return log() << Severity::WARNING;
}

std::unique_ptr<Log> logging::Logger::error() {
	return log() << Severity::ERROR;
}

std::unique_ptr<Log> logging::Logger::fatal() {
	return log() << Severity::FATAL;
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

std::unique_ptr<Log> &&operator << (std::unique_ptr<Log> &&log, Severity sev) {
	log->sev = sev;
	return std::move(log);
}

std::unique_ptr<Log> &&operator << (std::unique_ptr<Log> &&log, LineNumber indicator) {
	log->line = indicator.line;
	return std::move(log);
}

std::unique_ptr<Log> &&operator << (std::unique_ptr<Log> &&log, FileName indicator) {
	log->file = indicator.name;
	return std::move(log);
}

std::unique_ptr<Log> &&operator << (std::unique_ptr<Log> &&log, FunctionName indicator) {
	log->func = indicator.name;
	return std::move(log);
}
