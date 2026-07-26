/*
 * LuaMapEventDispatcher.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/scripting/MapEventDispatcher.h>

class Environment;
class CMap;

namespace scripting
{

class LuaScriptInstance;
class LuaContext;

/// Runs a map's generated Lua event script, forwarding engine triggers to the script's dispatcher.
class LuaMapEventDispatcher final : public MapEventDispatcher
{
public:
	/// Creates the script context and runs init method, if present to bind handlers to map objects.
	LuaMapEventDispatcher(std::shared_ptr<const LuaScriptInstance> script, const Environment * env, CMap & map);
	~LuaMapEventDispatcher();

	void onObjectVisit(IGameEventCallback & server, const std::string & handler,
		const CGObjectInstance * object, const CGHeroInstance * hero) override;
	void onPlayerTurnStart(IGameEventCallback & server, const std::string & handler, PlayerColor player) override;
	void onTownTurnStart(IGameEventCallback & server, const std::string & handler, const CGTownInstance * town) override;

private:
	std::shared_ptr<const LuaScriptInstance> script;
	const Environment * env;
	std::shared_ptr<LuaContext> context;
};

}
