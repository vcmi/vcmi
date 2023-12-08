/*
 * ModScope.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

namespace ModScope
{

/// returns true if scope is reserved for internal use and can not be used by mods
inline bool isScopeReserved(const std::string & scope)
{
	//following scopes are reserved - either in use by mod system or by filesystem
	static const std::array<std::string, 9> reservedScopes = {
		"core", "map", "game", "root", "saves", "config", "local", "initial", "mapEditor"
	};

	return std::find(reservedScopes.begin(), reservedScopes.end(), scope) != reservedScopes.end();
}

/// reserved scope name for referencing built-in (e.g. H3) objects
inline const std::string & scopeBuiltin()
{
	static const std::string scope = "core";
	return scope;
}

/// reserved scope name for accessing objects from any loaded mod
inline const std::string & scopeGame()
{
	static const std::string scope = "game";
	return scope;
}

/// reserved scope name for accessing object for map loading
inline const std::string & scopeMap()
{
	//TODO: implement accessing map dependencies for both H3 and VCMI maps
	// for now, allow access to any identifiers
	static const std::string scope = "game";
	return scope;
}

};

VCMI_LIB_NAMESPACE_END
