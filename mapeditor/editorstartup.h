/*
 * editorstartup.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <memory>

class CConsoleHandler;
class CBasicLogConfigurator;
class GameLibrary;

struct EditorStartup
{
	std::unique_ptr<CConsoleHandler> console;
	std::unique_ptr<CBasicLogConfigurator> logConfig;
	std::unique_ptr<GameLibrary> library;

	EditorStartup();
	~EditorStartup();
	EditorStartup(const EditorStartup &) = delete;
	EditorStartup & operator=(const EditorStartup &) = delete;

	/// Logging, base filesystem, library filesystem, settings-based logging. False when H3 or VCMI data is missing.
	bool initFilesystem(bool extractArchives);
	/// initializeLibrary, setGameLibrary, session.editor flag.
	void initLibrary();
};
