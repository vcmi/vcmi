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

		template<typename T, typename ... Args>
		void log(ELogLevel::ELogLevel level, const std::string & format, T t, Args ... args) const
		{
			boost::format fmt(format);
			makeFormat(fmt, t, args...);
			log(level, fmt.str());
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
}
