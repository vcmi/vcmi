/*
 * CombatEventPayload.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/scripting/ApiTags.h>

namespace battle
{
class Unit;
}

/// One unit hit by an attack, as reported to combat scripts.
struct DLL_LINKAGE AttackedTarget final : public scripting::ApiSerializable<AttackedTarget>
{
	const battle::Unit * unit = nullptr;
	int64_t damage = 0;
	int32_t killed = 0;
	int64_t damageBeforeDefense = 0;
	int64_t healthBeforeAttack = 0;

	template<typename Serializer>
	void serializeScript(Serializer & s)
	{
		s("unit",   unit,   "Unit that was hit.");
		s("damage", damage, "Damage dealt to it.");
		s("killed", killed, "How many of its creatures died.");
		s("damageBeforeDefense", damageBeforeDefense, "Highest damage the attack could have dealt to it with the defence of the target ignored.");
		s("healthBeforeAttack", healthBeforeAttack, "Health the unit had left before the attack landed.");
	}
};

/// Data describing one specific combat event. Events that carry no data leave every field empty,
/// so a script may read the fields of the event it handles without checking which event fired.
struct DLL_LINKAGE CombatEventPayload final : public scripting::ApiSerializable<CombatEventPayload>
{
	std::vector<AttackedTarget> targets;
	bool ranged = false;

	template<typename Serializer>
	void serializeScript(Serializer & s)
	{
		s("targets", targets, "Units hit by the attack that caused this event.");
		s("ranged",  ranged,  "Whether the attack that caused this event was a shot.");
	}
};
