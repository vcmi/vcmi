/*
 * Unit.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "Unit.h"

#include "../../../lib/GameLibrary.h"
#include "../../../lib/battle/CUnitState.h"
#include "../library/BonusBearerBindings.h"
#include "../Registry.h"

#include "../library/Creature.h"
#include "../library/Spell.h"
#include "BattleHex.h"
#include "BattleHexArray.h"
#include "UnitState.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api
{
using ::battle::IUnitInfo;
using ::battle::Unit;

void UnitProxy::registerMethods(MethodRegistrar & R)
{
	BonusBearerBindings<Unit>::registerMethods(R);

	R.method<&ACreature::getMinDamage, Unit>("getMinDamage",
		{{"ranged", "True for ranged attack value, false for melee."}}, {},
		"Returns the minimum damage one creature in the stack will deal.");
	R.method<&ACreature::getMaxDamage, Unit>("getMaxDamage",
		{{"ranged", "True for ranged attack value, false for melee."}}, {},
		"Returns the maximum damage one creature in the stack will deal.");
	R.method<&ACreature::getAttack, Unit>("getAttack",
		{{"ranged", "True for ranged attack value, false for melee."}}, {},
		"Returns the creature's attack stat.");
	R.method<&ACreature::getDefense, Unit>("getDefense",
		{{"ranged", "True for defense against ranged attacks, false for defense against melee."}}, {},
		"Returns the creature's defense stat.");
	R.method<&Unit::alive>("isAlive", {},
		"True if the stack has at least one alive creature.");
	R.method<&Unit::isClone>("isClone", {},
		"True if this stack is a clone produced by a Clone spell.");
	R.method<&Unit::hasClone>("hasClone", {},
		"True if this stack has an alive clone summoned by a Clone spell.");
	R.method<&Unit::isDead>("isDead", {},
		"True if the stack has no remaining alive creatures.");
	R.method<&Unit::isGhost>("isGhost", {},
		"True if the stack was completely removed from the battlefield including its corpse.");
	R.method<&Unit::isValidTarget>("isValidTarget",
		{{"allowDead", "Pass true to count dead but resurrectable stacks as valid targets."}}, {},
		"True if the stack can be targeted by spells / attacks.");
	R.method<&Unit::isInvincible>("isInvincible", {},
		"True if the stack has invincibility (cannot be damaged or killed).");
	R.function<&UnitProxy::hasAbsoluteImmunity>("hasAbsoluteImmunity",
		{{"spell", "Spell to test absolute immunity against."}}, {},
		"True if the unit is absolutely immune to the given spell.");
	R.method<&Unit::isSummoned>("isSummoned", {},
		"True if the stack was summoned during battle (e.g. by Summon Elementals).");
	R.method<&IUnitInfo::unitOwner, Unit>("getOwner", {},
		"Returns the player color controlling this unit.");
	R.method<&IUnitInfo::unitSlot, Unit>("getSlot", {},
		"Returns the army slot in the army this unit occupies. NOTE: All summoned units share the same slot");
	R.method<&IUnitInfo::unitSide, Unit>("unitSide", {},
		"Returns the battle side (attacker or defender) this unit belongs to.");
	R.method<&Unit::getPosition>("getPosition", {},
		"Returns the battlefield hex occupied by the unit, or front hex for double-wide units");
	R.method<&Unit::getTotalHealth>("getTotalHealth", {},
		"Returns the total hit points across all creatures in the stack, including dead.");
	R.method<&Unit::getAvailableHealth>("getAvailableHealth", {},
		"Returns the current hit points of living creatures of this unit.");
	R.method<&Unit::getCount>("getCount", {},
		"Returns the number of creatures currently alive in the stack.");
	R.method<&ACreature::getMaxHealth, Unit>("getMaxHealth", {},
		"Returns the maximum hit points of a single creature in the stack.");
	R.method<&Unit::coversPos>("coversPos",
		{{"position", "Battlefield hex to test against the unit's footprint."}}, {},
		"True if the unit currently covers the given hex (accounts for double-wide creatures).");
	R.function<&UnitProxy::getCreature>("getCreature", {},
		"Returns the Creature type of the units in this stack.");
	R.method<&Unit::unitBaseAmount, Unit>("getBaseAmount", {},
		"Returns the initial number of creatures this stack had at battle start.");
	R.function<&UnitProxy::getHexes>("getHexes", {},
		"Returns the list of hexes currently occupied by the unit.");
	R.function<&UnitProxy::copy>("copy", {},
		"Returns a copy of the unit's state allowing copying or changing this unit via server calls.");
	R.method<&Unit::creatureLevel>("creatureLevel", {},
		"Returns the creature level (1..7) of the unit's type.");
	R.method<&IUnitInfo::unitId, Unit>("unitID", {},
		"DEPRECATED. Returns the unit's internal numeric identifier.");
}

const Creature * UnitProxy::getCreature(const Unit & unit)
{
	return unit.creatureId().toEntity(LIBRARY);
}

BattleHexArray UnitProxy::getHexes(const Unit & unit)
{
	return unit.getHexes();
}

bool UnitProxy::hasAbsoluteImmunity(const Unit & unit, const spells::Spell & spell)
{
	return unit.hasAbsoluteImmunity(spell.getId());
}

LuaUnitState UnitProxy::copy(const Unit & unit)
{
	return LuaUnitState(unit.acquireState());
}

}

VCMI_LIB_NAMESPACE_END
