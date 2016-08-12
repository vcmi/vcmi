/*
 * CLogFormat.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

namespace vstd
{
	namespace detail
	{
		template <typename T>
		void makeFormat(boost::format & fmt, T t)
		{
			fmt % t;
		}

		template <typename T, typename ... Args>
		void makeFormat(boost::format & fmt, T t, Args ... args)
		{
			fmt % t;
			makeFormat(fmt, args...);
		}
	}

    template<typename Logger, typename ... Args>
    void logFormat(Logger * logger, ELogLevel::ELogLevel level, const std::string & format, Args ... args)
    {
		boost::format fmt(format);
		detail::makeFormat(fmt, args...);
		logger->log(level, fmt.str());
    }

    template<typename Logger, typename ... Args>
    void logErrorFormat(Logger * logger, const std::string & format, Args ... args)
    {
    	logFormat(logger, ELogLevel::ERROR, format, args...);
    }

    template<typename Logger, typename ... Args>
    void logWarnFormat(Logger * logger, const std::string & format, Args ... args)
    {
    	logFormat(logger, ELogLevel::WARN, format, args...);
    }

    template<typename Logger, typename ... Args>
    void logInfoFormat(Logger * logger, const std::string & format, Args ... args)
    {
    	logFormat(logger, ELogLevel::INFO, format, args...);
    }

    template<typename Logger, typename ... Args>
    void logDebugFormat(Logger * logger, const std::string & format, Args ... args)
    {
    	logFormat(logger, ELogLevel::DEBUG, format, args...);
    }

    template<typename Logger, typename ... Args>
    void logTraceFormat(Logger * logger, const std::string & format, Args ... args)
    {
    	logFormat(logger, ELogLevel::TRACE, format, args...);
    }
}
