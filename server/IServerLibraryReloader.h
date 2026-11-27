/*
 * IServerLibraryReloader.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

class ModManager;
class ModSnapshot;

/// Replaces game library of the server with a library of another mod list
class IServerLibraryReloader
{
public:
	virtual ~IServerLibraryReloader() = default;

	/// Mod manager of the process, for dry runs of required mods
	virtual const ModManager & mods() const = 0;

	/// Server thread, returns after the swap. On failure LIBRARY is unchanged and error holds the reason,
	/// empty if the host client has already shown it and kept its library
	virtual bool reload(const ModSnapshot & mods, std::string & error) = 0;
};
