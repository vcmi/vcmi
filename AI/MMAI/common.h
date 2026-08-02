/*
 * common.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "StdInc.h" // IWYU pragma: keep

#include "CThreadHelper.h"
#include <format>

namespace MMAI
{
// Enum-to-int need C++23 to use std::to_underlying
// https://en.cppreference.com/w/cpp/utility/to_underlying
#define EI(enum_value) static_cast<int>(enum_value)

inline void assertImpl(
	bool cond,
	std::string_view msg,
	std::string_view file,
	int line,
	std::string_view function)
{
	if(!cond)
	{
		throw std::runtime_error(
			std::string("Assertion failed in ")
			+ boost::filesystem::path(std::string(file)).filename().string()
			+ ":"
			+ std::to_string(line)
			+ " in "
			+ std::string(function)
			+ ": "
			+ std::string(msg)
		);
	}
}

#define ASSERT(condition, message) \
	::MMAI::assertImpl((condition), (message), __FILE__, __LINE__, BOOST_CURRENT_FUNCTION)

#define THROW_FORMAT(message, formatting_elems) throw std::runtime_error(boost::str(boost::format(message) % formatting_elems))

template<class... Args>
[[noreturn]] void throwf(std::format_string<Args...> format, Args&&... args)
{
    throw std::runtime_error(std::format(format, std::forward<Args>(args)...));
}

// constexpr version of EI with proper underlying type conversion
// underlying_type_t requires C++23, but Global.h uses it already
template<typename E>
requires std::is_enum_v<E>
constexpr std::underlying_type_t<E> EU(E value) noexcept
{
	return static_cast<std::underlying_type_t<E>>(value);
}

inline bool isMMAIVerbose()
{
	static const bool value = []
	{
		const char * envvar = std::getenv("MMAI_VERBOSE");
		return envvar != nullptr && std::strcmp(envvar, "1") == 0;
	}();
	return value;
}

inline bool isMMAIAutoRender()
{
	static const bool value = []
	{
		const char * envvar = std::getenv("MMAI_AUTO_RENDER");
		return envvar != nullptr && std::strcmp(envvar, "1") == 0;
	}();
	return value;
}

inline bool isMMAIAutoVerify()
{
	static const bool value = []
	{
		const char * envvar = std::getenv("MMAI_AUTO_VERIFY");
		return envvar != nullptr && std::strcmp(envvar, "1") == 0;
	}();
	return value;
}

/*
 * RAII for temporarily setting a new name for the current thread.
 * The thread name appears in messages logged through VCMI's logger.
 *
 * Example:
 *     {                     * (before) Thread name: "foo"
 *       LogTag _("bar")     * (RAII)   Thread name: "bar"
 *     }                     * (after)  Thread name: "foo"
 */
struct LogTag
{
	const std::string oldname;

	explicit LogTag(const std::string & n) : oldname(getThreadName())
	{
		setThreadName(n);
	};

	~LogTag()
	{
		setThreadName(oldname);
	}

	LogTag(const LogTag &) = delete;
	LogTag & operator=(const LogTag &) = delete;
	LogTag(LogTag &&) = delete;
	LogTag & operator=(LogTag &&) = delete;
};

/*
 * Similar to LogTag, but *appends* the given string instead.
 * Example:
 *     {                           * (before) Thread name: "foo"
 *       NestedLogTag _("bar")     * (RAII)   Thread name: "foo.bar"
 *     }                           * (after)  Thread name: "foo"
 */
struct NestedLogTag
{
	const LogTag logtag;
	explicit NestedLogTag(const std::string & n) : logtag(getThreadName() + "." + n) {};
};

}
