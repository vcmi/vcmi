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

#include "StdInc.h"
#include "CThreadHelper.h"

namespace MMAI
{
// Enum-to-int need C++23 to use std::to_underlying
// https://en.cppreference.com/w/cpp/utility/to_underlying
#define EI(enum_value) static_cast<int>(enum_value)

#define ASSERT(cond, msg) \
	if(!(cond))           \
	throw std::runtime_error(std::string("Assertion failed in ") + boost::filesystem::path(__FILE__).filename().string() + ": " + msg)

#define THROW_FORMAT(message, formatting_elems) throw std::runtime_error(boost::str(boost::format(message) % formatting_elems))

static const bool MMAI_VERBOSE = []()
{
	const char * envvar = std::getenv("MMAI_VERBOSE");
	return envvar != nullptr && std::strcmp(envvar, "1") == 0;
}();

/*
 * RAII for temporarily setting a new name for the current thread.
 * The thread name appears in messages logged through VCMI's logger.
 *
 * Example:
 *     {                     // (before) Thread name: "foo"
 *       LogTag _("bar")     // (RAII)   Thread name: "bar"
 *     }                     // (after)  Thread name: "foo"
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
 *     {                           // (before) Thread name: "foo"
 *       NestedLogTag _("bar")     // (RAII)   Thread name: "foo.bar"
 *     }                           // (after)  Thread name: "foo"
 */
struct NestedLogTag
{
	const LogTag logtag;
	explicit NestedLogTag(const std::string & n) : logtag(getThreadName() + "." + n) {};
};

}
