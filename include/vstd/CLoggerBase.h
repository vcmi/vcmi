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
}

namespace vstd
{
	class DLL_LINKAGE CLoggerBase
	{
	public:
		virtual ~CLoggerBase(){};

		virtual void log(ELogLevel::ELogLevel level, const std::string & message) const = 0;

		/// Log methods for various log levels
		inline void trace(const std::string & message) const
		{
			log(ELogLevel::TRACE, message);
		};

		inline void debug(const std::string & message) const
		{
			log(ELogLevel::DEBUG, message);
		};

		inline void info(const std::string & message) const
		{
			log(ELogLevel::INFO, message);
		};

		inline void warn(const std::string & message) const
		{
			log(ELogLevel::WARN, message);
		};

		inline void error(const std::string & message) const
		{
			log(ELogLevel::ERROR, message);
		};
	};
}
