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

	std::atomic<bool> returnRequested = false;

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

	/// Reloads library of the active preset as reloadLibrary does, unless current library already has its mods. Main thread, pregame only
	void returnToHomeLibrary(const std::function<void(bool success)> & onFinished);

	/// Requests return to library of the active preset once the main menu is the top window again
	void requestReturnToHomeLibrary();
	bool isReturnRequested() const;
};
