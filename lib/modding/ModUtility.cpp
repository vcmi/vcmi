/*
 * CModHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ModUtility.h"

#include <vstd/StringUtils.h>

VCMI_LIB_NAMESPACE_BEGIN

std::string ModUtility::normalizeIdentifier(const std::string & scope, const std::string & remoteScope, const std::string & identifier)
{
	auto p = vstd::splitStringToPair(identifier, ':');

	if(p.first.empty())
		p.first = scope;

	if(p.first == remoteScope)
		p.first.clear();

	return p.first.empty() ? p.second : p.first + ":" + p.second;
}

void ModUtility::parseIdentifier(const std::string & fullIdentifier, std::string & scope, std::string & type, std::string & identifier)
{
	auto p = vstd::splitStringToPair(fullIdentifier, ':');

	scope = p.first;

	auto p2 = vstd::splitStringToPair(p.second, '.');

	if(!p2.first.empty())
	{
		type = p2.first;
		identifier = p2.second;
	}
	else
	{
		type = p.second;
		identifier.clear();
	}
}

std::string ModUtility::makeFullIdentifier(const std::string & scope, const std::string & type, const std::string & identifier)
{
	if(type.empty())
		logGlobal->error("Full identifier (%s %s) requires type name", scope, identifier);

	std::string actualScope = scope;
	std::string actualName = identifier;

	//ignore scope if identifier is scoped
	auto scopeAndName = vstd::splitStringToPair(identifier, ':');

	if(!scopeAndName.first.empty())
	{
		actualScope = scopeAndName.first;
		actualName = scopeAndName.second;
	}

	if(actualScope.empty())
	{
		return actualName.empty() ? type : type + "." + actualName;
	}
	else
	{
		return actualName.empty() ? actualScope+ ":" + type : actualScope + ":" + type + "." + actualName;
	}
}

VCMI_LIB_NAMESPACE_END
