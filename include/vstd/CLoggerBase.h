/*
 * CLoggerBase.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

VCMI_LIB_NAMESPACE_BEGIN

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

	inline std::string to_string(ELogLevel level)
	{
		switch (level) {
			case NOT_SET:
				return "not set";
			case TRACE:
				return "trace";
			case DEBUG:
				return "debug";
			case INFO:
				return "info";
			case WARN:
				return "warn";
			case ERROR:
				return "error";
			default:
#ifdef NO_STD_TOSTRING
				return "invalid";
#else
				return std::string("invalid (") + std::to_string(level) + ")";
#endif
		}
	}
}

namespace vstd
{
class DLL_LINKAGE CLoggerBase
{
public:
	virtual ~CLoggerBase();

	virtual void log(ELogLevel::ELogLevel level, const std::string & message) const = 0;
	virtual void log(ELogLevel::ELogLevel level, const boost::format & fmt) const = 0;

	/// Returns true if a debug/trace log message will be logged, false if not.
	/// Useful if performance is important and concatenating the log message is a expensive task.
	virtual bool isDebugEnabled() const = 0;
	virtual bool isTraceEnabled() const = 0;

	template<typename T, typename ... Args>
	void log(ELogLevel::ELogLevel level, const std::string & format, T t, Args ... args) const
	{
		try
		{
			boost::format fmt(format);
			makeFormat(fmt, t, args...);
			log(level, fmt);
		}
		catch(...)
		{
			log(ELogLevel::ERROR, "Log formatting failed, format was:");
			log(ELogLevel::ERROR, format);
		}
	}

	/// Log methods for various log levels
	inline void error(const std::string & message) const
	{
		log(ELogLevel::ERROR, message);
	};

	template<typename T, typename ... Args>
	void error(const std::string & format, T t, Args ... args) const
	{
		log(ELogLevel::ERROR, format, t, args...);
	}

	inline void warn(const std::string & message) const
	{
		log(ELogLevel::WARN, message);
	};

	template<typename T, typename ... Args>
	void warn(const std::string & format, T t, Args ... args) const
	{
		log(ELogLevel::WARN, format, t, args...);
	}

	inline void info(const std::string & message) const
	{
		log(ELogLevel::INFO, message);
	};

	template<typename T, typename ... Args>
	void info(const std::string & format, T t, Args ... args) const
	{
		log(ELogLevel::INFO, format, t, args...);
	}

	inline void debug(const std::string & message) const
	{
		log(ELogLevel::DEBUG, message);
	};


	template<typename T, typename ... Args>
	void debug(const std::string & format, T t, Args ... args) const
	{
		log(ELogLevel::DEBUG, format, t, args...);
	}

	inline void trace(const std::string & message) const
	{
		log(ELogLevel::TRACE, message);
	};

	template<typename T, typename ... Args>
	void trace(const std::string & format, T t, Args ... args) const
	{
		log(ELogLevel::TRACE, format, t, args...);
	}

private:
	template <typename T>
	void makeFormat(boost::format & fmt, T t) const
	{
		fmt % t;
	}

	template <typename T, typename ... Args>
	void makeFormat(boost::format & fmt, T t, Args ... args) const
	{
		fmt % t;
		makeFormat(fmt, args...);
	}
};

/// RAII class for tracing the program execution.
/// It prints "Leaving function XYZ" automatically when the object gets destructed.
class DLL_LINKAGE CTraceLogger
{
public:
	CTraceLogger(const CLoggerBase * logger, const std::string & beginMessage, const std::string & endMessage);
	CTraceLogger(const CTraceLogger & other) = delete;
	~CTraceLogger();

private:
	const CLoggerBase * logger;
	std::string endMessage;
};

}//namespace vstd


/// Macros for tracing the control flow of the application conveniently. If the LOG_TRACE macro is used it should be
/// the first statement in the function. Logging traces via this macro have almost no impact when the trace is disabled.
///
#define RAII_TRACE(logger, onEntry, onLeave)			\
	std::unique_ptr<vstd::CTraceLogger> ctl00;						\
	if(logger->isTraceEnabled())						\
		ctl00 = std::make_unique<vstd::CTraceLogger>(logger, onEntry, onLeave);

#define LOG_TRACE(logger) RAII_TRACE(logger,								\
		boost::str(boost::format("Entering %s.") % BOOST_CURRENT_FUNCTION),	\
		boost::str(boost::format("Leaving %s.") % BOOST_CURRENT_FUNCTION))


#define LOG_TRACE_PARAMS(logger, formatStr, params) RAII_TRACE(logger,		\
		boost::str(boost::format("Entering %s: " + std::string(formatStr) + ".") % BOOST_CURRENT_FUNCTION % params), \
		boost::str(boost::format("Leaving %s.") % BOOST_CURRENT_FUNCTION))


extern DLL_LINKAGE vstd::CLoggerBase * logGlobal;
extern DLL_LINKAGE vstd::CLoggerBase * logBonus;
extern DLL_LINKAGE vstd::CLoggerBase * logNetwork;
extern DLL_LINKAGE vstd::CLoggerBase * logAi;
extern DLL_LINKAGE vstd::CLoggerBase * logAnim;
extern DLL_LINKAGE vstd::CLoggerBase * logMod;

VCMI_LIB_NAMESPACE_END
