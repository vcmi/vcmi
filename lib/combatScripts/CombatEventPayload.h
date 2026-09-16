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

namespace spells
{
class Spell;
}

/// One unit hit by an attack or by a spell, as reported to combat scripts.
/// Before the attack only `unit` and `healthBeforeAttack` are known - no damage has been rolled yet.
struct DLL_LINKAGE AttackedTarget final : public scripting::ApiSerializable<AttackedTarget>
{
	const battle::Unit * unit = nullptr;
	int64_t damage = 0;
	int32_t killed = 0;
	int64_t damageBeforeDefense = 0;
	int64_t healthBeforeAttack = 0;
	/// Only the spell hit event fills this in, and the snapshots it points into outlive the payload.
	const battle::Unit * unitBefore = nullptr;

	template<typename Serializer>
	void serializeScript(Serializer & s)
	{
		s("unit",   unit,   "Unit that was hit.");
		s("damage", damage, "Damage dealt to it.");
		s("killed", killed, "How many of its creatures died.");
		s("damageBeforeDefense", damageBeforeDefense, "Damage this same hit would have dealt with the defences of the target ignored.");
		s("healthBeforeAttack", healthBeforeAttack, "Health the unit had left before the attack landed.");
		s("unitBefore", unitBefore, "The unit as it stood before the spell reached it, which is what tells apart what the spell did from what the unit already was. Only the spell hit event captures one; every other event leaves it nil.");
	}
};

/// Data describing one specific combat event. Events that carry no data leave every field empty,
/// so a script may read the fields of the event it handles without checking which event fired.
struct DLL_LINKAGE CombatEventPayload final : public scripting::ApiSerializable<CombatEventPayload>
{
	std::vector<AttackedTarget> targets;
	const spells::Spell * spell = nullptr;
	bool ranged = false;
	bool isCounter = false;
	int32_t attackIndex = 0;

	template<typename Serializer>
	void serializeScript(Serializer & s)
	{
		s("targets",     targets,     "Units hit by the attack or spell that caused this event. Before the attack, only their identity and remaining health are known.");
		s("spell",       spell,       "Spell that caused this event, for the spellcast and spell hit events. Nil for every other event.");
		s("ranged",      ranged,      "Whether the attack that caused this event was a shot.");
		s("isCounter",   isCounter,   "Whether the attack is a counterattack - either a first strike or a regular retaliation.");
		s("attackIndex", attackIndex, "Zero-based index of this attack among those its own side makes in this action, so the second hit of a double attack is 1. A counterattack is its side's attack 0.");
	}
};
