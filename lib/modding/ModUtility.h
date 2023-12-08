/*
 * ModUtility.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

// NOTE: all methods in this namespace should be considered internal to modding system and should not be used outside of this module
namespace ModUtility
{
	DLL_LINKAGE std::string normalizeIdentifier(const std::string & scope, const std::string & remoteScope, const std::string & identifier);

	DLL_LINKAGE void parseIdentifier(const std::string & fullIdentifier, std::string & scope, std::string & type, std::string & identifier);

	DLL_LINKAGE std::string makeFullIdentifier(const std::string & scope, const std::string & type, const std::string & identifier);

};

VCMI_LIB_NAMESPACE_END
