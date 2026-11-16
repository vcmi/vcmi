/*
 * ClientLibraries.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

class GameLibrary;
class ModManager;
class ModSnapshot;

/// Owns mod manager of the client and replaces game library of the client with a library of another mod list
class ClientLibraries : boost::noncopyable
{
	std::unique_ptr<ModManager> modManager;

	std::unique_ptr<GameLibrary> loadedLibrary;
	std::string loadingError;

	void loadLibrary(const ModSnapshot & mods);
	void finishReload(const std::function<void(bool success)> & onFinished);

public:
	explicit ClientLibraries(std::unique_ptr<ModManager> modManager);
	~ClientLibraries();

	ModManager & mods();

	/// Replaces current library with a library of provided mod list, then calls onFinished on main thread. Pregame only
	void reloadLibrary(const ModSnapshot & mods, const std::function<void(bool success)> & onFinished);
};
