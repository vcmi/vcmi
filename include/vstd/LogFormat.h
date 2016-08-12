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

	//TODO: Put into CLogger after log api extract

    template<typename Logger>
    void log(Logger * logger, ELogLevel::ELogLevel level, const std::string & message)
    {
		logger->log(level, message);
    }

    template<typename Logger, typename T, typename ... Args>
    void log(Logger * logger, ELogLevel::ELogLevel level, const std::string & format, T t, Args ... args)
    {
		boost::format fmt(format);
		detail::makeFormat(fmt, t, args...);
		logger->log(level, fmt.str());
    }

    template<typename Logger, typename T, typename ... Args>
    void logError(Logger * logger, const std::string & format, T t, Args ... args)
    {
    	log(logger, ELogLevel::ERROR, format, t, args...);
    }

    template<typename Logger, typename T, typename ... Args>
    void logWarn(Logger * logger, const std::string & format, T t, Args ... args)
    {
    	log(logger, ELogLevel::WARN, format, t, args...);
    }

    template<typename Logger, typename T, typename ... Args>
    void logInfo(Logger * logger, const std::string & format, T t, Args ... args)
    {
    	log(logger, ELogLevel::INFO, format, t, args...);
    }

    template<typename Logger, typename T, typename ... Args>
    void logDebug(Logger * logger, const std::string & format, T t, Args ... args)
    {
    	log(logger, ELogLevel::DEBUG, format, t, args...);
    }

    template<typename Logger, typename T, typename ... Args>
    void logTrace(Logger * logger, const std::string & format, T t, Args ... args)
    {
    	log(logger, ELogLevel::TRACE, format, t, args...);
    }
}
