/*
 * scripting/MapEventDispatcher.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

class IGameEventCallback;
class CGObjectInstance;
class CGHeroInstance;
class CGTownInstance;
class PlayerColor;

namespace scripting
{

/// Fires the map's script-defined event handlers. The engine holds this behind the abstract interface
/// and passes an opaque handler name (stored on the triggering object/event) plus the event context;
/// how the name maps to a script function is entirely the script's concern.
class DLL_LINKAGE MapEventDispatcher
{
public:
	virtual ~MapEventDispatcher() = default;

	/// Fires a handler. Returns an opaque coroutine handle if the script paused on a blocking action
	/// (the caller must keep a LuaScriptQuery alive to resume it), or std::nullopt if it ran to completion.
	virtual std::optional<int> onObjectVisit(IGameEventCallback & server, const std::string & handler, const CGObjectInstance * object, const CGHeroInstance * hero) = 0;
	virtual std::optional<int> onPlayerTurnStart(IGameEventCallback & server, const std::string & handler, PlayerColor player) = 0;
	virtual std::optional<int> onTownTurnStart(IGameEventCallback & server, const std::string & handler, const CGTownInstance * town) = 0;

	/// Resumes a paused coroutine with the player's reply (nullopt for non-dialog children such as combat).
	/// Returns true if the coroutine finished (its query may now be removed), false if it paused again.
	virtual bool resumeCoroutine(IGameEventCallback & server, int coroutineHandle, std::optional<int> answer) = 0;
};

}
