/*
 * UnitState.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "UnitState.h"

#include "../../../lib/GameLibrary.h"
#include "../../LuaStack.h"
#include "../../LuaCallWrapper.h"
#include "../Registry.h"

#include "../library/Creature.h"
#include "../library/Spell.h"
#include "BattleHex.h"
#include "BattleHexArray.h"
#include "Unit.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api
{

void LuaUnitStateProxy::registerMethods(MethodRegistrar & R)
{
	// Read-only API
	R.method<&LuaUnitState::getMinDamage>("getMinDamage",
		{{"ranged", "True for ranged attack value, false for melee."}}, {},
		"Returns the minimum damage one creature will deal.");
	R.method<&LuaUnitState::getMaxDamage>("getMaxDamage",
		{{"ranged", "True for ranged attack value, false for melee."}}, {},
		"Returns the maximum damage one creature will deal.");
	R.method<&LuaUnitState::getAttack>("getAttack",
		{{"ranged", "True for ranged attack value, false for melee."}}, {},
		"Returns the creature's attack stat.");
	R.method<&LuaUnitState::getDefense>("getDefense",
		{{"ranged", "True for defense against ranged attacks, false for defense against melee."}}, {},
		"Returns the creature's defense stat.");
	R.method<&LuaUnitState::isAlive>("isAlive", {},
		"True if the stack has at least one alive creature.");
	R.method<&LuaUnitState::isClone>("isClone", {},
		"True if this stack is a clone produced by the Clone spell.");
	R.method<&LuaUnitState::isDead>("isDead", {},
		"True if the stack has no remaining alive creatures.");
	R.method<&LuaUnitState::isGhost>("isGhost", {},
		 "True if the stack was completely removed from the battlefield including its corpse.");
	R.method<&LuaUnitState::isValidTarget>("isValidTarget",
		{{"allowDead", "Pass true to count dead but resurrectable stacks as valid targets."}}, {},
		"True if the stack can be targeted.");
	R.method<&LuaUnitState::isInvincible>("isInvincible", {},
		"True if the stack has invincibility.");
	R.method<&LuaUnitState::isSummoned>("isSummoned", {},
		"True if the stack was summoned during battle.");
	R.method<&LuaUnitState::getOwner>("getOwner", {},
		"Returns the player color controlling this unit.");
	R.method<&LuaUnitState::getSlot>("getSlot", {},
		"Returns the army slot in the army this unit occupies. NOTE: All summoned units share the same slot");
	R.method<&LuaUnitState::getPosition>("getPosition", {},
		"Returns the battlefield hex occupied by the unit, or front hex for double-wide units");
	R.method<&LuaUnitState::getTotalHealth>("getTotalHealth", {},
		"Returns the total hit points across all creatures in the stack, including dead.");
	R.method<&LuaUnitState::getAvailableHealth>("getAvailableHealth", {},
		"Returns the current hit points of living creatures of this unit.");
	R.method<&LuaUnitState::getCount>("getCount", {},
		"Returns the number of creatures currently alive in the stack.");
	R.method<&LuaUnitState::getMaxHealth>("getMaxHealth", {},
		"Returns the maximum hit points of a single creature.");
	R.method<&LuaUnitState::coversPos>("coversPos",
		{{"position", "Battlefield hex to test against the unit's footprint."}}, {},
		"True if the unit currently covers the given hex.");
	R.method<&LuaUnitState::getBaseAmount>("getBaseAmount", {},
		"Returns the initial number of creatures this stack had at battle start.");
	R.function<&LuaUnitStateProxy::hasAbsoluteImmunity>("hasAbsoluteImmunity",
		{{"spell", "Spell to test absolute immunity against."}}, {},
		"True if the unit is absolutely immune to the given spell.");
	R.function<&LuaUnitStateProxy::getCreature>("getCreature", {},
		"Returns the Creature type of the units in this stack.");
	R.function<&LuaUnitStateProxy::getHexes>("getHexes", {},
		"Returns the list of hexes currently occupied by the unit.");

	// Mutable — flags
	R.method<&LuaUnitState::setDefending>("setDefending",
		{{"value", "New defending flag."}}, {},
		"Sets whether unit has defended in this round.");
	R.method<&LuaUnitState::setCloned>("setCloned",
		{{"value", "New cloned flag."}}, {},
		"Marks the stack as a clone.");
	R.method<&LuaUnitState::setDrainedMana>("setDrainedMana",
		{{"value", "New drained-mana flag."}}, {},
		"Marks that the unit has had drained mana from enemy hero this turn.");
	R.method<&LuaUnitState::setFear>("setFear",
		{{"value", "New feared flag — when true the unit will skip its next turn."}}, {},
		"Sets the feared status (skips the unit's current turn).");
	R.method<&LuaUnitState::setHadMorale>("setHadMorale",
		{{"value", "New had-morale flag."}}, {},
		"Marks that the unit has already triggered a morale bonus this turn.");
	R.method<&LuaUnitState::setCastSpellThisTurn>("setCastSpellThisTurn",
		{{"value", "New cast-spell-this-turn flag."}}, {},
		"Marks that the unit has cast a spell this turn.");
	R.method<&LuaUnitState::setGhost>("setGhost",
		{{"value", "New ghost flag — when true the unit becomes invisible to the battle."}}, {},
		"Sets the ghost state removing them from the battle");
	R.method<&LuaUnitState::setGhostPending>("setGhostPending",
		{{"value", "New ghost-pending flag."}}, {},
		"Marks the unit as transitioning to the ghost state.");
	R.method<&LuaUnitState::setMoved>("setMoved",
		{{"value", "New moved-this-round flag."}}, {},
		"Marks that the unit has moved this round.");
	R.method<&LuaUnitState::setSummoned>("setSummoned",
		{{"value", "New summoned flag."}}, {},
		"Marks the unit as summoned during combat.");
	R.method<&LuaUnitState::setWaiting>("setWaiting",
		{{"value", "New waiting flag."}}, {},
		"Marks that the unit is currently waiting.");
	R.method<&LuaUnitState::setWaitedThisTurn>("setWaitedThisTurn",
		{{"value", "New waited-this-turn flag."}}, {},
		"Marks that the unit has already used its wait this turn.");

	// Mutable — other fields
	R.method<&LuaUnitState::setPosition>("setPosition",
		{{"hex", "Destination hex for the unit."}}, {},
		"Moves the unit to the given hex.");
	R.method<&LuaUnitState::setClone>("setClone",
		{{"unit", "Unit instance to register as this stack's clone."}}, {},
		"Links this stack to a clone unit produced by a Clone spell.");

	// Mutable — health
	R.method<&LuaUnitState::damage>("damage",
		{{"amount", "Damage to apply; will be clamped to remaining health."}},
		{"Damage value actually applied (may be less than requested if the stack was killed)."},
		"Deals damage to the stack, clamped to available health.");
	R.cfunction<&LuaUnitStateProxy::heal>("heal",
		{
			{"amount", "integer",     "Hit points to restore."},
			{"level",  "HealLevel",   "Heal tier (heal, resurrect, overheal)."},
			{"power",  "HealPower",   "Persistence — one-battle vs permanent."}
		},
		{"integer, integer", "Healed hit points, and the count of creatures resurrected (zero if none)."},
		"Heals the stack. Returns the healed hit points and the resurrected creature count, if any.");
}

// --- LuaUnitState implementation ---

LuaUnitState::LuaUnitState(std::shared_ptr<::battle::CUnitState> state_)
	: state(std::move(state_))
{
}

int LuaUnitState::getMinDamage(bool ranged) const { return state->getMinDamage(ranged); }
int LuaUnitState::getMaxDamage(bool ranged) const { return state->getMaxDamage(ranged); }
int LuaUnitState::getAttack(bool ranged)    const { return state->getAttack(ranged); }
int LuaUnitState::getDefense(bool ranged)   const { return state->getDefense(ranged); }

bool LuaUnitState::isAlive()                    const { return state->alive(); }
bool LuaUnitState::isClone()                    const { return state->isClone(); }
bool LuaUnitState::isDead()                     const { return state->isDead(); }
bool LuaUnitState::isGhost()                    const { return state->isGhost(); }
bool LuaUnitState::isValidTarget(bool allowDead) const { return state->isValidTarget(allowDead); }
bool LuaUnitState::isInvincible()               const { return state->isInvincible(); }
bool LuaUnitState::isSummoned()                 const { return state->summoned; }

PlayerColor LuaUnitState::getOwner()     const { return state->unitOwner(); }
SlotID      LuaUnitState::getSlot()      const { return state->unitSlot(); }
BattleHex   LuaUnitState::getPosition()  const { return state->getPosition(); }
int64_t     LuaUnitState::getTotalHealth()    const { return state->getTotalHealth(); }
int64_t     LuaUnitState::getAvailableHealth() const { return state->getAvailableHealth(); }
int32_t     LuaUnitState::getCount()         const { return state->getCount(); }
uint32_t    LuaUnitState::getMaxHealth()     const { return state->getMaxHealth(); }
bool        LuaUnitState::coversPos(BattleHex pos) const { return state->coversPos(pos); }
int32_t     LuaUnitState::getBaseAmount()    const { return state->unitBaseAmount(); }

void LuaUnitState::setDefending(bool v)         { state->defending = v; }
void LuaUnitState::setCloned(bool v)            { state->cloned = v; }
void LuaUnitState::setDrainedMana(bool v)       { state->drainedMana = v; }
void LuaUnitState::setFear(bool v)              { state->fear = v; }
void LuaUnitState::setHadMorale(bool v)         { state->hadMorale = v; }
void LuaUnitState::setCastSpellThisTurn(bool v) { state->castSpellThisTurn = v; }
void LuaUnitState::setGhost(bool v)             { state->ghost = v; }
void LuaUnitState::setGhostPending(bool v)      { state->ghostPending = v; }
void LuaUnitState::setMoved(bool v)             { state->movedThisRound = v; }
void LuaUnitState::setSummoned(bool v)          { state->summoned = v; }
void LuaUnitState::setWaiting(bool v)           { state->waiting = v; }
void LuaUnitState::setWaitedThisTurn(bool v)    { state->waitedThisTurn = v; }

void LuaUnitState::setPosition(BattleHex hex)   { state->setPosition(hex); }
void LuaUnitState::setClone(const ::battle::Unit & unit) { state->cloneID = unit.unitId(); }

int64_t LuaUnitState::damage(int64_t amount)
{
	state->damage(amount);
	return amount;
}

// --- LuaUnitStateProxy static methods ---

bool LuaUnitStateProxy::hasAbsoluteImmunity(const LuaUnitState & state, const spells::Spell & spell)
{
	return state.getState()->hasAbsoluteImmunity(spell.getId());
}

const Creature * LuaUnitStateProxy::getCreature(const LuaUnitState & state)
{
	return state.getState()->creatureId().toEntity(LIBRARY);
}

BattleHexArray LuaUnitStateProxy::getHexes(const LuaUnitState & state)
{
	return state.getState()->getHexes();
}

int LuaUnitStateProxy::heal(lua_State * L)
{
	LuaStack S(L);

	LuaUnitState unitState;
	int64_t amount = 0;
	EHealLevel healLevel = EHealLevel::HEAL;
	EHealPower healPower = EHealPower::PERMANENT;

	S.get(1, unitState);
	S.get(2, amount);
	S.get(3, healLevel);
	S.get(4, healPower);

	::battle::HealInfo info = unitState.getState()->heal(amount, healLevel, healPower);

	S.clear();
	S.push(info.healedHealthPoints);
	S.push(static_cast<int64_t>(info.resurrectedCount));
	return 2;
}

}

VCMI_LIB_NAMESPACE_END
