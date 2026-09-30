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

/// Attack or spell target data reported to combat scripts.
/// BEFORE_ATTACK provides only `unit` and `healthBeforeAttack` because damage is not rolled yet.
struct DLL_LINKAGE AttackedTarget final : public scripting::ApiSerializable<AttackedTarget>
{
	const battle::Unit * unit = nullptr;
	int64_t damage = 0;
	int32_t killed = 0;
	int64_t damageBeforeDefense = 0;
	int64_t healthBeforeAttack = 0;
	/// Non-owning pointer to pre-cast state retained by the caller during dispatch
	const battle::Unit * unitBefore = nullptr;

	template<typename Serializer>
	void serializeScript(Serializer & s)
	{
		s("unit",   unit,   "Target unit.");
		s("damage", damage, "Damage dealt to the target.");
		s("killed", killed, "Creatures killed in the target stack.");
		s("damageBeforeDefense", damageBeforeDefense, "Damage before applying target defence modifiers.");
		s("healthBeforeAttack", healthBeforeAttack, "Target health before the attack.");
		s("unitBefore", unitBefore, "Target state before spell effects. Set only for the spell hit event.");
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
		s("targets",     targets,     "Attack or spell targets. Before an attack, only identity and remaining health are available.");
		s("spell",       spell,       "Spell that caused this event, for the spellcast and spell hit events. Nil for every other event.");
		s("ranged",      ranged,      "Whether the attack that caused this event was a shot.");
		s("isCounter",   isCounter,   "Whether the attack is a counterattack - either a first strike or a regular retaliation.");
		s("attackIndex", attackIndex, "Zero-based index of this attack among those its own side makes in this action, so the second hit of a double attack is 1. A counterattack is its side's attack 0.");
	}
};
