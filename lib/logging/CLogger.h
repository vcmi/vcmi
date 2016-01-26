
/*
 * CLogger.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../CConsoleHandler.h"
#include "../filesystem/FileStream.h"

class CLogger;
struct LogRecord;
class ILogTarget;

namespace ELogLevel
{
	enum ELogLevel
	{
		NOT_SET = 0,
		TRACE,
		DEBUG,
		INFO,
		WARN,
		ERROR
	};

	#ifdef VCMI_ANDROID
		int toAndroid(ELogLevel logLevel);
	#endif
}

/// The class CLoggerDomain provides convenient access to super domains from a sub domain.
class DLL_LINKAGE CLoggerDomain
{
public:
	/// Constructs a CLoggerDomain with the domain designated by name.
	/// Sub-domains can be specified by separating domains by a dot, e.g. "ai.battle". The global domain is named "global".
	explicit CLoggerDomain(std::string name);

	const std::string& getName() const;
	CLoggerDomain getParent() const;
	bool isGlobalDomain() const;

	static const std::string DOMAIN_GLOBAL;

private:
	std::string name;
};

/// The class CLoggerStream provides a stream-like way of logging messages.
class DLL_LINKAGE CLoggerStream
{
public:
	CLoggerStream(const CLogger & logger, ELogLevel::ELogLevel level);
	~CLoggerStream();

	template<typename T>
	CLoggerStream & operator<<(const T & data)
	{
		if(!sbuffer)
			sbuffer = new std::stringstream(std::ios_base::out);

		(*sbuffer) << data;

		return *this;
	}

private:
	const CLogger & logger;
	ELogLevel::ELogLevel level;
	std::stringstream * sbuffer;
};

/// The logger is used to log messages to certain targets of a specific domain/name.
/// It is thread-safe and can be used concurrently by several threads.
class DLL_LINKAGE CLogger
{
public:
	inline ELogLevel::ELogLevel getLevel() const;
	void setLevel(ELogLevel::ELogLevel level);
	const CLoggerDomain & getDomain() const;

	/// Logger access methods
	static CLogger * getLogger(const CLoggerDomain & domain);
	static CLogger * getGlobalLogger();

	/// Log methods for various log levels
	void trace(const std::string & message) const;
	void debug(const std::string & message) const;
	void info(const std::string & message) const;
	void warn(const std::string & message) const;
	void error(const std::string & message) const;

	/// Log streams for various log levels
	CLoggerStream traceStream() const;
	CLoggerStream debugStream() const;
	CLoggerStream infoStream() const;
	CLoggerStream warnStream() const;
	CLoggerStream errorStream() const;

	inline void log(ELogLevel::ELogLevel level, const std::string & message) const;

	void addTarget(std::unique_ptr<ILogTarget> && target);
	void clearTargets();

	/// Returns true if a debug/trace log message will be logged, false if not.
	/// Useful if performance is important and concatenating the log message is a expensive task.
	bool isDebugEnabled() const;
	bool isTraceEnabled() const;

private:
	explicit CLogger(const CLoggerDomain & domain);
	inline ELogLevel::ELogLevel getEffectiveLevel() const; /// Returns the log level applied on this logger whether directly or indirectly.
	inline void callTargets(const LogRecord & record) const;

	CLoggerDomain domain;
	CLogger * parent;
	ELogLevel::ELogLevel level;
	std::vector<std::unique_ptr<ILogTarget> > targets;
	mutable boost::mutex mx;
	static boost::recursive_mutex smx;
};

extern DLL_LINKAGE CLogger * logGlobal;
extern DLL_LINKAGE CLogger * logBonus;
extern DLL_LINKAGE CLogger * logNetwork;
extern DLL_LINKAGE CLogger * logAi;
extern DLL_LINKAGE CLogger * logAnim;

/// RAII class for tracing the program execution.
/// It prints "Leaving function XYZ" automatically when the object gets destructed.
class DLL_LINKAGE CTraceLogger : boost::noncopyable
{
public:
	CTraceLogger(const CLogger * logger, const std::string & beginMessage, const std::string & endMessage);
	~CTraceLogger();

private:
	const CLogger * logger;
	std::string endMessage;
};

/// Macros for tracing the control flow of the application conveniently. If the LOG_TRACE macro is used it should be
/// the first statement in the function. Logging traces via this macro have almost no impact when the trace is disabled.
///
#define RAII_TRACE(logger, onEntry, onLeave)			\
	std::unique_ptr<CTraceLogger> ctl00;						\
	if(logger->isTraceEnabled())						\
		ctl00 = make_unique<CTraceLogger>(logger, onEntry, onLeave);

#define LOG_TRACE(logger) RAII_TRACE(logger,								\
		boost::str(boost::format("Entering %s.") % BOOST_CURRENT_FUNCTION),	\
		boost::str(boost::format("Leaving %s.") % BOOST_CURRENT_FUNCTION))


#define LOG_TRACE_PARAMS(logger, formatStr, params) RAII_TRACE(logger,		\
		boost::str(boost::format("Entering %s: " + std::string(formatStr) + ".") % BOOST_CURRENT_FUNCTION % params), \
		boost::str(boost::format("Leaving %s.") % BOOST_CURRENT_FUNCTION))

/* ---------------------------------------------------------------------------- */
/* Implementation/Detail classes, Private API */
/* ---------------------------------------------------------------------------- */

/// The class CLogManager is a global storage for logger objects.
class DLL_LINKAGE CLogManager : public boost::noncopyable
{
public:
	static CLogManager & get();

	void addLogger(CLogger * logger);
	CLogger * getLogger(const CLoggerDomain & domain); /// Returns a logger or nullptr if no one is registered for the given domain.

private:
	CLogManager();
	~CLogManager();

	std::map<std::string, CLogger *> loggers;
	mutable boost::mutex mx;
	static boost::recursive_mutex smx;
};

/// The struct LogRecord holds the log message and additional logging information.
struct DLL_LINKAGE LogRecord
{
	LogRecord(const CLoggerDomain & domain, ELogLevel::ELogLevel level, const std::string & message)
		: domain(domain),
		level(level),
		message(message),
		timeStamp(boost::posix_time::microsec_clock::local_time()),
		threadId(boost::lexical_cast<std::string>(boost::this_thread::get_id())) { }

	CLoggerDomain domain;
	ELogLevel::ELogLevel level;
	std::string message;
	boost::posix_time::ptime timeStamp;
	std::string threadId;
};

/// The class CLogFormatter formats log records.
///
/// There are several pattern characters which can be used to format a log record:
/// %d = Date/Time
/// %l = Log level
/// %n = Logger name
/// %t = Thread ID
/// %m = Message
class DLL_LINKAGE CLogFormatter
{
public:
	CLogFormatter();
	CLogFormatter(const CLogFormatter & copy);
	CLogFormatter(CLogFormatter && move);

	CLogFormatter(const std::string & pattern);

	CLogFormatter & operator=(const CLogFormatter & copy);
	CLogFormatter & operator=(CLogFormatter && move);

	void setPattern(const std::string & pattern);
	void setPattern(std::string && pattern);

	const std::string & getPattern() const;

	std::string format(const LogRecord & record) const;

private:
	std::string pattern;
	mutable std::stringstream dateStream;
};

/// The interface ILogTarget is used by all log target implementations. It holds
/// the abstract method write which sub-classes should implement.
class DLL_LINKAGE ILogTarget : public boost::noncopyable
{
public:
	virtual ~ILogTarget() { };
	virtual void write(const LogRecord & record) = 0;
};

/// The class CColorMapping maps a logger name and a level to a specific color. Supports domain inheritance.
class DLL_LINKAGE CColorMapping
{
public:
	CColorMapping();

	void setColorFor(const CLoggerDomain & domain, ELogLevel::ELogLevel level, EConsoleTextColor::EConsoleTextColor color);
	EConsoleTextColor::EConsoleTextColor getColorFor(const CLoggerDomain & domain, ELogLevel::ELogLevel level) const;

private:
	std::map<std::string, std::map<ELogLevel::ELogLevel, EConsoleTextColor::EConsoleTextColor> > map;
};

/// This target is a logging target which writes message to the console.
/// The target may be shared among multiple loggers. All methods except write aren't thread-safe.
/// The console target is intended to be configured once and then added to a logger.
class DLL_LINKAGE CLogConsoleTarget : public ILogTarget
{
public:
	explicit CLogConsoleTarget(CConsoleHandler * console);

	bool isColoredOutputEnabled() const;
	void setColoredOutputEnabled(bool coloredOutputEnabled);

	ELogLevel::ELogLevel getThreshold() const;
	void setThreshold(ELogLevel::ELogLevel threshold);

	const CLogFormatter & getFormatter() const;
	void setFormatter(const CLogFormatter & formatter);

	const CColorMapping & getColorMapping() const;
	void setColorMapping(const CColorMapping & colorMapping);

	void write(const LogRecord & record) override;

private:
	CConsoleHandler * console;
	ELogLevel::ELogLevel threshold;
	bool coloredOutputEnabled;
	CLogFormatter formatter;
	CColorMapping colorMapping;
	mutable boost::mutex mx;
};

/// This target is a logging target which writes messages to a log file.
/// The target may be shared among multiple loggers. All methods except write aren't thread-safe.
/// The file target is intended to be configured once and then added to a logger.
class DLL_LINKAGE CLogFileTarget : public ILogTarget
{
public:
	/// Constructs a CLogFileTarget and opens the file designated by filePath. If the append parameter is true, the file
	/// will be appended to. Otherwise the file designated by filePath will be truncated before being opened.
	explicit CLogFileTarget(boost::filesystem::path filePath, bool append = true);

	const CLogFormatter & getFormatter() const;
	void setFormatter(const CLogFormatter & formatter);

	void write(const LogRecord & record) override;

private:
	FileStream file;
	CLogFormatter formatter;
	mutable boost::mutex mx;
};
