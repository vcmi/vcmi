/*
 * scripting/Service.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/Environment.h>

#include <boost/filesystem/path.hpp>

class CGameState;

class ScriptService;

namespace scripting
{

class MapEventDispatcher;

using BattleCb = Environment::BattleCb;
using GameCb = Environment::GameCb;

class DLL_LINKAGE Context
{
public:
	virtual ~Context() = default;
};

class DLL_LINKAGE Script
{
public:
	virtual ~Script() = default;

	virtual std::string getIdentifier() const = 0;
};

class DLL_LINKAGE Pool
{
public:
	virtual ~Pool() = default;

	virtual std::shared_ptr<Context> getContext(const Script * script) const = 0;
};

class DLL_LINKAGE Service
{
public:
	virtual ~Service() = default;

	/// Registers this language as a script backend. Called once, before any content is loaded.
	virtual void installScripting(ScriptService & scripts) = 0;

	virtual std::unique_ptr<Pool> createPoolInstance(const Environment * ENV) const = 0;

	/// Builds the dispatcher for the game's map event script, or nullptr if the map has none
	virtual std::unique_ptr<MapEventDispatcher> createMapScriptDispatcher(CGameState & gs, bool runInit) const = 0;

	/// Writes Markdown and Lua Language Server reference files describing every exposed API type into the given output directory
	virtual void exportDocs(const boost::filesystem::path & outDir) const = 0;
};

}
