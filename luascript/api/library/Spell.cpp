/*
 * api/Spell.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Spell.h"

#include "EntityBindings.h"
#include "SpellSchool.h"
#include "../Registry.h"
#include "../../../lib/constants/EntityIdentifiers.h"
#include "../../../lib/GameLibrary.h"
#include "../../../lib/battle/CBattleInfoCallback.h"
#include "../../../lib/battle/Unit.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../../../lib/spells/CSpell.h"

namespace scripting::api
{

using ::spells::Spell;

void SpellProxy::registerMethods(MethodRegistrar & R)
{
	EntityBindings<Spell>::registerMethods(R);
	R.method<&Entity::getNameTextID, Spell>("getNameTextID", {},
		"Returns the text ID of the spell name.");
	R.method<&Spell::isAdventure>("isAdventure", {},
		"True if the spell can only be cast on the adventure map.");
	R.method<&Spell::isCombat>("isCombat", {},
		"True if the spell can only be cast during combat.");
	R.method<&Spell::isCreatureAbility>("isCreatureAbility", {},
		"True if the spell is a creature's innate ability rather than a learnable spell.");
	R.method<&Spell::isPositive>("isPositive", {},
		"True if the spell is classified as beneficial to its target.");
	R.method<&Spell::isNegative>("isNegative", {},
		"True if the spell is classified as harmful to its target.");
	R.method<&Spell::isNeutral>("isNeutral", {},
		"True if the spell is classified as neutral (neither positive nor negative).");
	R.method<&Spell::isDamage>("isDamage", {},
		"True if the spell deals direct damage.");
	R.method<&Spell::isOffensive>("isOffensive", {},
		"True if the spell is offensive (damage / curse-type effects).");
	R.method<&Spell::isSpecial>("isSpecial", {},
		"True if the spell is marked as special (e.g. cannot be obtained from common sources).");
	R.method<&Spell::isPersistent>("isPersistent", {},
		"True if the spell's effect persists and can't be dispelled normally.");
	R.method<&Spell::isMagical>("isMagical", {},
		"True if the spell is magical in nature (as opposed to a non-magical ability).");

	R.method<&Spell::getCost>("getCost",
		{{"skillLevel", "Mastery level used to look up the cost (0=basic, up to 3=expert)."}}, {},
		"Returns the mana cost of the spell at the requested skill level.");
	R.method<&Spell::getBasePower>("getBasePower", {},
		"Returns the spell's base power before caster bonuses.");
	R.method<&Spell::getLevelPower>("getLevelPower",
		{{"skillLevel", "Mastery level used to look up the power bonus (0=basic, up to 3=expert)."}}, {},
		"Returns the spell's per-level power bonus.");
	R.function<&SpellProxy::getSchools>("getSchools", {},
		"Returns the list of magic schools the spell belongs to.");
	R.function<&SpellProxy::adjustDamage>("adjustDamage",
		{
			{"battle",    "Battle the damage is dealt in."},
			{"actor",     "Unit whose ability deals the damage. Its hero, if it has one, provides the caster bonuses."},
			{"target",    "Unit taking the damage, whose resistances and vulnerabilities apply."},
			{"rawDamage", "Damage before any of those modifiers."}
		}, {},
		"Runs a raw damage amount through this spell's damage pipeline - the school bonuses of the "
		"actor's hero, and the target's resistances, vulnerabilities and immunities. Use it for "
		"abilities that damage as if they were this spell without actually casting it.");
}

int64_t SpellProxy::adjustDamage(const Spell & spell, const IBattleInfoCallback & battle, const battle::Unit & actor, const battle::Unit & target, int64_t rawDamage)
{
	const auto * owner = dynamic_cast<const CSpell *>(&spell);
	const auto * cb = dynamic_cast<const CBattleInfoCallback *>(&battle);

	if(!owner)
		throw std::runtime_error("Attempt to adjust damage of an unknown spell!");

	if(!cb)
		throw std::runtime_error("Attempt to adjust spell damage outside of a battle!");

	// a unit fighting under a hero benefits from that hero's magic, as in a normal cast
	const spells::Caster * caster = cb->battleGetFightingHero(actor.unitSide());

	if(!caster)
		caster = &actor;

	return owner->adjustRawDamage(caster, &target, rawDamage);
}

std::vector<const spells::SpellSchoolType *> SpellProxy::getSchools(const Spell & spell)
{
	std::vector<const spells::SpellSchoolType *> result;
	spell.forEachSchool([&result](const SpellSchool & school, bool &)
	{
		result.push_back(school.toEntity(LIBRARY));
	});
	return result;
}

}
