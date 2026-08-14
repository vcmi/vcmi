/*
 * ICombatEventScript.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "CombatEventPayload.h"

#include "../constants/Enumerations.h"

class CBattleInfoCallback;
class JsonNode;
class ServerCallback;

namespace battle
{
class Unit;
}

/// Reacts to combat events on behalf of a unit carrying a COMBAT_EVENT_TRIGGER bonus.
/// Implementations are stateless and shared - all per-instance data arrives in parameters.
class DLL_LINKAGE ICombatEventScript
{
public:
	virtual ~ICombatEventScript() = default;

	/// self is the unit the event happened to, other is the unit on the opposite side of it (may be null).
	/// `parameters` configure the script and come from the bonus; `payload` describes this specific
	/// event and is empty for events that carry no extra data.
	virtual void run(ServerCallback * server, const CBattleInfoCallback & battle, CombatEventType event, const battle::Unit * self, const battle::Unit * other, const JsonNode & parameters, const CombatEventPayload & payload) const = 0;
};
