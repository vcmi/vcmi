/*
 * JsonComparer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../lib/JsonNode.h"

namespace vstd
{
	template<typename T>
	class ScopeGuard;
}

class JsonComparer
{
public:
	JsonComparer(bool strict_);

	void compare(const std::string & name, const JsonNode & actual, const JsonNode & expected);

private:
	typedef std::function<void(void)> TScopeGuard;

	bool strict;
	std::list<std::string> namePath;

	vstd::ScopeGuard<TScopeGuard> pushName(const std::string & name);

	std::string buildMessage(const std::string & message);

	bool isEmpty(const JsonNode & value);

	void check(const bool condition, const std::string & message);

	void checkExcessStructField(const JsonNode & actualValue, const std::string & name, const JsonMap & expected);
	void checkStructField(const JsonMap & actual, const std::string & name, const JsonNode & expectedValue);

	void checkEqualInteger(const si64 actual, const si64 expected);
	void checkEqualFloat(const double actual, const double expected);
	void checkEqualString(const std::string & actual, const std::string & expected);

	void checkEqualJson(const JsonNode & actual, const JsonNode & expected);
	void checkEqualJson(const JsonMap & actual, const JsonMap & expected);
	void checkEqualJson(const JsonVector & actual, const JsonVector & expected);
};
