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

#include <string>

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

	virtual void onObjectVisit(IGameEventCallback & server, const std::string & handler,
		const CGObjectInstance * object, const CGHeroInstance * hero) = 0;
	virtual void onPlayerTurnStart(IGameEventCallback & server, const std::string & handler, PlayerColor player) = 0;
	virtual void onTownTurnStart(IGameEventCallback & server, const std::string & handler, const CGTownInstance * town) = 0;
};

}
