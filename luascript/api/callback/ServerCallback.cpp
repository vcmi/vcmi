/*
 * ServerCallback.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ServerCallback.h"

#include "GameLibrary.h"
#include "IBattleInfoCallback.h"

#include "../Enums.h"
#include "../LuaMetaString.h"
#include "../Registry.h"
#include "../battle/UnitState.h"
#include "../battle/SpellObstacleDescriptor.h"
#include "../library/BonusDescriptor.h"

#include "../battle/BattleHex.h"
#include "../battle/Obstacle.h"
#include "../battle/Unit.h"
#include "../library/Bonus.h"
#include "../library/Spell.h"

#include "../../LuaStack.h"
#include "../../../lib/battle/SiegeInfo.h"
#include "../../../lib/networkPacks/PacksForClientBattle.h"
#include "../../../lib/networkPacks/SetStackEffect.h"
#include "../../../lib/battle/Unit.h"
#include "../../../lib/battle/CObstacleInstance.h"
#include "../../../lib/battle/IBattleState.h"
#include "../../../lib/CStack.h"
#include "../../../lib/spells/AbilityCaster.h"
#include "../../../lib/bonuses/BonusList.h"
#include "../../../lib/bonuses/Bonus.h"
#include "../../../lib/battle/CUnitState.h"
#include "../../../lib/battle/CBattleInfoCallback.h"
#include "../../../lib/battle/Destination.h"
#include "../../../lib/spells/CSpellHandler.h"
#include "../../../lib/spells/ISpellMechanics.h"
#include "../../../lib/texts/MetaString.h"
#include "../../../lib/constants/EntityIdentifiers.h"
#include "modding/IdentifierStorage.h"
#include "modding/ModScope.h"

#include <vstd/RNG.h>

namespace scripting::api
{

void ServerCallbackProxy::registerMethods(MethodRegistrar & R)
{
	R.function<&ServerCallbackProxy::addUnit>("addUnit",
		{
			{"battle", "Battle in which the unit is created."},
			{"info",   "Descriptor of the unit to spawn (creature type, side, position, ...)."}
		}, {},
		"Spawns a new battle unit described by the given UnitInfo. Returns the created unit.");
	R.cfunction<&ServerCallbackProxy::healUnit>("healUnit",
		{
			{"battle",      "Battle",     "Battle in which the unit is healed."},
			{"unit",        "Unit",       "Target unit."},
			{"amount",      "integer",    "Hit points to restore."},
			{"level",       "HealLevel",  "Heal tier (heal / resurrect / overheal)."},
			{"power",       "HealPower",  "Persistence — one-battle vs permanent."}
		},
		{"integer, integer", "Healed hit points, and the count of creatures resurrected."},
		"Heals the given unit by the provided amount of health points.");
	R.cfunction<&ServerCallbackProxy::changeUnit>("changeUnit",
		{
			{"battle",      "Battle",      "Battle in which the unit is modified."},
			{"unitState",   "UnitState",   "New unit state to apply (returned by `Unit:copy`)."},
			{"healthDelta", "integer?",    "Optional health delta — positive heals, negative damages."}
		}, {},
		"Applies a UnitState mutation to the unit, optionally adjusting current health.");
	R.cfunction<&ServerCallbackProxy::damageUnit>("damageUnit",
		{
			{"battle", "Battle",  "Battle in which damage is dealt."},
			{"unit",   "Unit",    "Target unit."},
			{"damage", "integer", "Damage points to deal (will be clamped to remaining health)."}
		},
		{"integer, integer", "Damage actually dealt, and the count of killed creatures."},
		"Damages the unit, returning the actual damage dealt and the number of killed creatures.");
	R.function<&ServerCallbackProxy::removeUnit>("removeUnit",
		{
			{"battle", "Battle the unit belongs to."},
			{"unit",   "Unit (alive or as corpse) to remove."}
		}, {},
		"Removes the unit or its corpse from the battlefield.");
	R.function<&ServerCallbackProxy::removeObstacle>("removeObstacle",
		{
			{"battle",   "Battle the obstacle belongs to."},
			{"obstacle", "Obstacle to remove."}
		}, {},
		"Removes the given obstacle from the battlefield.");
	R.function<&ServerCallbackProxy::moveUnit>("moveUnit",
		{
			{"battle",      "Battle in which the unit is moved."},
			{"unit",        "Unit to move."},
			{"destination", "Target hex of the move."},
			{"isTeleport",  "Pass true to use teleport semantics (no path walk)."}
		}, {},
		"Moves the unit to the destination hex.");
	R.function<&ServerCallbackProxy::appendLog>("appendLog",
		{
			{"battle",  "Battle whose log is being appended."},
			{"message", "Formatted log line (use `MetaString`)."}
		}, {},
		"Appends a formatted log entry to the battle log.");
	R.function<&ServerCallbackProxy::describeChanges>("describeChanges", {},
		"Returns whether netpack changes should be described in the battle log.");
	R.function<&ServerCallbackProxy::removeUnitBonuses>("removeUnitBonuses",
		{
			{"battle",    "Battle the unit belongs to."},
			{"unit",      "Unit whose bonuses are removed."},
			{"bonusList", "Bonuses to remove from the unit."}
		}, {},
		"Removes the listed bonuses from the unit.");
	R.function<&ServerCallbackProxy::addUnitBonus>("addUnitBonus",
		{
			{"battle",     "Battle the unit belongs to."},
			{"unit",       "Unit to add the bonus to."},
			{"descriptor", "BonusDescriptor that creates the bonus."},
			{"cumulative", "Pass true to stack with existing same-source bonus, false to update in-place."}
		}, {},
		"Adds a bonus described by the descriptor to the unit.");
	R.function<&ServerCallbackProxy::addBattleBonus>("addBattleBonus",
		{
			{"battle",     "Battle to attach the bonus to."},
			{"descriptor", "BonusDescriptor that creates the bonus."}
		}, {},
		"Adds a bonus to the battle-wide bonus set.");
	R.function<&ServerCallbackProxy::addObstacle>("addObstacle",
		{
			{"battle",     "Battle the obstacle is placed in."},
			{"descriptor", "SpellObstacleDescriptor describing the obstacle to create."}
		}, {},
		"Creates a new obstacle described by the descriptor on the battlefield.");
	R.function<&ServerCallbackProxy::catapultAttack>("catapultAttack",
		{
			{"battle",       "Battle in which the catapult attack happens."},
			{"attacker",     "Unit performing the catapult attack, or nil for spell-caused attacks."},
			{"attackedPart", "Wall section to attack."},
			{"damageDealt",  "Damage to apply to the wall section."}
		}, {},
		"Performs a catapult attack against the given wall section, dealing the supplied damage.");
	R.function<&ServerCallbackProxy::rngInt>("rngInt",
		{
			{"low",  "Inclusive lower bound."},
			{"high", "Inclusive upper bound."}
		},
		{"Random integer in [low, high]."},
		"Returns a server-side random integer in the inclusive range [low, high].");
	R.function<&ServerCallbackProxy::rngBinomial>("rngBinomial",
		{
			{"trials", "How many independent chances are rolled."},
			{"chance", "Chance of each one succeeding, from 0 to 1."}
		},
		{"How many of them succeeded."},
		"Rolls the same chance many times over and returns how many succeeded. Use this rather "
		"than a loop over `rngInt` for abilities that roll once per creature in a stack, which "
		"can number in the thousands.");
	R.function<&ServerCallbackProxy::rollCombatAbility>("rollCombatAbility",
		{
			{"battle",           "Battle the acting unit fights in."},
			{"actor",            "Unit whose ability is being rolled."},
			{"percentageChance", "Chance to succeed, in percent."}
		},
		{"True if the ability triggers."},
		"Rolls a chance-based combat ability. Use this rather than `rngInt` for abilities that "
		"trigger with a percentage chance - it draws from the per-army biased sequence");
	R.function<&ServerCallbackProxy::castSpell>("castSpell",
		{
			{"battle",      "Battle the spell is cast in."},
			{"caster",      "Unit whose ability casts the spell."},
			{"spell",       "Spell to cast."},
			{"target",      "Units the spell is aimed at."},
			{"effectValue", "Magnitude handed to the spell's effects, for spells that take one."}
		}, {},
		"Casts a spell as a passive ability of the caster: announces it so that clients play its "
		"animation and sound, filters the targets through the spell's own immunity rules, and "
		"applies its effects with the given magnitude. Casting this way never fires a combat "
		"event, so a script that casts cannot re-enter itself.");
	R.function<&ServerCallbackProxy::applySpellEffects>("applySpellEffects",
		{
			{"battle",         "Battle the spell is applied in."},
			{"caster",         "Unit acting as the caster. Its stats scale the effects."},
			{"spell",          "Spell whose effects are applied."},
			{"target",         "Units to affect."},
			{"spellLevel",     "Mastery level the effects are applied at."},
			{"effectDuration", "How many turns timed effects of the spell last."},
			{"ignoreImmunity", "Pass true to affect units that are immune to the spell."}
		}, {},
		"Applies the effects of a spell to the given units, and nothing else. Unlike casting the "
		"spell, the target list is used as given rather than expanded through the spell's range, "
		"magic resistance and magic mirror are not rolled, countering effects are not removed, and "
		"no spell animation or battle log entry is produced. Use it for abilities that behave as "
		"if the spell were already in effect.");
	R.function<&ServerCallbackProxy::showBattleAnimation>("showBattleAnimation",
		{
			{"battle",       "Battle the animation is played in."},
			{"target",       "Units and hexes the animation is played on, one copy each."},
			{"animation",    "Resource name of the animation to play, e.g. `SP06_`."},
			{"sound",        "Resource name of the sound to play alongside it, or an empty string for none."},
			{"transparency", "Opacity of the animation, from 0 for invisible to 1 for opaque."},
			{"deferred",     "Pass true to hold the animation back until the next change the script makes, so that both play at once."}
		}, {},
		"Plays a one-shot animation on the battlefield. Changes no game state, so use it to give a "
		"visual to a change the script made itself. The animation plays after whatever is currently "
		"animating has finished, rather than overlapping it. A deferred animation instead plays "
		"together with the animations of the next change the script makes, e.g. the flinch of a "
		"unit it damages right after - and is dropped if no such change follows.");
	R.function<&ServerCallbackProxy::refreshBattleUnits>("refreshBattleUnits",
		{{"battle", "Battle whose units were changed."}}, {},
		"Makes the client play back pending unit changes. Needed after adding or removing units "
		"outside of an attack, which otherwise produce no animation.");
}

bool ServerCallbackProxy::describeChanges(ServerCallback & object)
{
	return object.describeChanges();
}

bool ServerCallbackProxy::rollCombatAbility(ServerCallback & object, const IBattleInfoCallback & battle, const battle::Unit & actor, int percentageChance)
{
	return object.rollCombatAbility(battle, actor, percentageChance);
}

void ServerCallbackProxy::applySpellEffects(ServerCallback & object, const IBattleInfoCallback & battle, const battle::Unit & caster, const spells::Spell & spell, const std::vector<const battle::Unit *> & target, int spellLevel, int effectDuration, bool ignoreImmunity)
{
	const auto * cb = dynamic_cast<const CBattleInfoCallback *>(&battle);
	if(!cb)
		throw std::runtime_error("Attempt to apply spell effects outside of a battle!");

	const CSpell * spellObject = spell.getId().toSpell();
	if(!spellObject)
		throw std::runtime_error("Attempt to apply effects of an unknown spell!");

	spells::BattleCast cast(cb, &caster, spells::Mode::PASSIVE, spellObject);
	cast.setSpellLevel(spellLevel);
	cast.setEffectDuration(effectDuration);

	spells::Target destinations;
	for(const auto * unit : target)
		if(unit)
			destinations.emplace_back(unit);

	cast.applyEffects(&object, destinations, false, ignoreImmunity);
}

void ServerCallbackProxy::castSpell(ServerCallback & object, const IBattleInfoCallback & battle, const battle::Unit & caster, const spells::Spell & spell, const std::vector<const battle::Unit *> & target, int64_t effectValue)
{
	const auto * cb = dynamic_cast<const CBattleInfoCallback *>(&battle);
	if(!cb)
		throw std::runtime_error("Attempt to cast a spell outside of a battle!");

	const CSpell * spellObject = spell.getId().toSpell();
	if(!spellObject)
		throw std::runtime_error("Attempt to cast an unknown spell!");

	spells::AbilityCaster abilityCaster(&caster, 0);
	spells::BattleCast cast(cb, &abilityCaster, spells::Mode::PASSIVE, spellObject);
	cast.setEffectValue(effectValue);

	spells::Target destinations;
	for(const auto * unit : target)
		if(unit)
			destinations.emplace_back(unit);

	cast.cast(&object, destinations);
}

void ServerCallbackProxy::showBattleAnimation(ServerCallback & object, const IBattleInfoCallback & battle, const std::vector<battle::Destination> & target, const std::string & animation, const std::string & sound, double transparency, std::optional<bool> deferred)
{
	BattleAnimationPlayed pack;
	pack.battleID = battle.getBattle()->getBattleID();
	pack.animation = AnimationPath::builtin(animation);
	pack.sound = sound.empty() ? AudioPath() : AudioPath::builtin(sound);
	pack.transparency = static_cast<float>(transparency);
	pack.deferred = deferred.value_or(false);

	for(const auto & destination : target)
	{
		BattleAnimationPlayed::Target entry;
		entry.unitID = destination.unitValue ? destination.unitValue->unitId() : -1;
		entry.tile = destination.hexValue;
		pack.targets.push_back(entry);
	}

	object.apply(static_cast<CPackForClient &>(pack));
}

void ServerCallbackProxy::refreshBattleUnits(ServerCallback & object, const IBattleInfoCallback & battle)
{
	StacksInjured pack;
	pack.battleID = battle.getBattle()->getBattleID();
	object.apply(pack);
}

void ServerCallbackProxy::removeUnitBonuses(ServerCallback & object, const IBattleInfoCallback & battle, const battle::Unit & unit, const BonusList & bonusList)
{
	std::vector<Bonus> buffer;
	for(const auto & b : bonusList)
		buffer.emplace_back(*b);

	if(buffer.empty())
		return;

	SetStackEffect sse;
	sse.battleID = battle.getBattle()->getBattleID();
	sse.toRemove.emplace_back(unit.unitId(), buffer);
	object.apply(sse);
}

void ServerCallbackProxy::addUnitBonus(ServerCallback & object, const IBattleInfoCallback & battle, const battle::Unit & unit, const BonusDescriptor & data, bool cumulative)
{
	Bonus b = data.toBonus();

	SetStackEffect sse;
	sse.battleID = battle.getBattle()->getBattleID();
	if(cumulative)
		sse.toAdd.emplace_back(unit.unitId(), std::vector<Bonus>{b});
	else
		sse.toUpdate.emplace_back(unit.unitId(), std::vector<Bonus>{b});
	object.apply(sse);
}

void ServerCallbackProxy::addBattleBonus(ServerCallback & object, const IBattleInfoCallback & battle, const BonusDescriptor & data)
{
	GiveBonus gb(GiveBonus::ETarget::BATTLE);
	gb.id = battle.getBattle()->getBattleID();
	gb.bonus = data.toBonus();
	object.apply(gb);
}

void ServerCallbackProxy::addObstacle(ServerCallback & object, const IBattleInfoCallback & battle, const SpellObstacleDescriptor & descriptor)
{
	SpellCreatedObstacle obstacle = descriptor.toObstacle();
	obstacle.uniqueID = battle.nextObstacleId();

	BattleObstaclesChanged pack;
	pack.battleID = battle.getBattle()->getBattleID();
	obstacle.toInfo(pack.change);
	object.apply(pack);
}

const battle::Unit * ServerCallbackProxy::addUnit(ServerCallback & object, const IBattleInfoCallback & battle, const battle::UnitInfo & info)
{
	battle::UnitInfo unitInfo = info;
	unitInfo.id = battle.battleNextUnitId();

	BattleUnitsChanged buc;
	UnitChanges uc;
	uc.operation = UnitChanges::EOperation::ADD;
	buc.battleID = battle.getBattle()->getBattleID();
	uc.id = unitInfo.id;
	unitInfo.save(uc.data);
	buc.changedStacks.push_back(uc);
	object.apply(buc);

	return battle.battleGetUnitByID(unitInfo.id);
}

void ServerCallbackProxy::removeUnit(ServerCallback & object, const IBattleInfoCallback & battle, const battle::Unit & unit)
{
	BattleUnitsChanged buc;
	UnitChanges uc;
	buc.battleID = battle.getBattle()->getBattleID();
	uc.operation = UnitChanges::EOperation::REMOVE;
	uc.id = unit.unitId();
	buc.changedStacks.push_back(uc);
	object.apply(buc);
}

void ServerCallbackProxy::removeObstacle(ServerCallback & object, const IBattleInfoCallback & battle, std::shared_ptr<const CObstacleInstance> obstacle)
{
	if(!obstacle)
		return;

	BattleObstaclesChanged pack;
	pack.battleID = battle.getBattle()->getBattleID();
	pack.change = ObstacleChanges(obstacle->uniqueID, BattleChanges::EOperation::REMOVE);
	auto * serializable = const_cast<CObstacleInstance*>(obstacle.get());
	serializable->toInfo(pack.change, BattleChanges::EOperation::REMOVE);
	object.apply(pack);
}

void ServerCallbackProxy::catapultAttack(ServerCallback & object, const IBattleInfoCallback & battle, const battle::Unit * attacker, EWallPart attackedPart, int32_t damageDealt)
{
	CatapultAttack ca;
	ca.battleID = battle.getBattle()->getBattleID();
	ca.attacker = attacker ? attacker->unitId() : -1;
	ca.attackedPart = attackedPart;
	ca.destinationTile = battle.wallPartToBattleHex(attackedPart).toInt();
	ca.damageDealt = static_cast<ui8>(std::clamp(damageDealt, 0, 255));

	ca.killedTowerShooter = -1;
	if(attackedPart == EWallPart::KEEP || attackedPart == EWallPart::BOTTOM_TOWER || attackedPart == EWallPart::UPPER_TOWER)
	{
		EWallState stateAfter = SiegeInfo::applyDamage(battle.battleGetWallState(attackedPart), ca.damageDealt);
		if(stateAfter == EWallState::DESTROYED)
		{
			BattleHex towerHex = battle.getTowerShooterHex(attackedPart);
			const battle::Unit * shooter = battle.battleGetUnitByPos(towerHex, false);
			if(shooter && !shooter->isGhost())
				ca.killedTowerShooter = shooter->unitId();
		}
	}

	object.apply(ca);
}

int ServerCallbackProxy::rngInt(ServerCallback & object, int low, int high)
{
	return object.getRNG()->nextInt(low, high);
}

int ServerCallbackProxy::rngBinomial(ServerCallback & object, int trials, double chance)
{
	return object.getRNG()->nextBinomialInt(trials, chance);
}

void ServerCallbackProxy::moveUnit(ServerCallback & object, const IBattleInfoCallback & battle, const battle::Unit & unit, BattleHex destination, bool isTeleport)
{
	BattleStackMoved pack;
	pack.battleID = battle.getBattle()->getBattleID();
	pack.stack = unit.unitId();
	pack.distance = 0;
	pack.teleporting = isTeleport;
	BattleHexArray tiles;
	tiles.insert(destination);
	pack.tilesToMove = tiles;
	object.apply(pack);
}

void ServerCallbackProxy::appendLog(ServerCallback & object, const IBattleInfoCallback & battle, const LuaMetaString & config)
{
	BattleLogMessage msg;
	msg.battleID = battle.getBattle()->getBattleID();
	msg.lines.push_back(config.toMetaString());
	object.apply(msg);
}

int ServerCallbackProxy::changeUnit(lua_State * L)
{
	LuaStack S(L);

	ServerCallback * object = nullptr;
	const IBattleInfoCallback * battle = nullptr;
	LuaUnitState unitState;
	int64_t healthDelta = 0;

	S.get(1, object);
	S.getNonNull(2, battle);
	S.get(3, unitState);
	if(S.stackSize() >= 4)
		S.get(4, healthDelta);

	auto * cstate = unitState.getState();

	BattleUnitsChanged buc;
	UnitChanges uc(cstate->unitId(), UnitChanges::EOperation::UPDATE);
	buc.battleID = battle->getBattle()->getBattleID();
	uc.data = cstate->save();
	uc.healthDelta = healthDelta;
	buc.changedStacks.push_back(uc);
	object->apply(buc);

	return S.retVoid();
}

int ServerCallbackProxy::damageUnit(lua_State * L)
{
	LuaStack S(L);

	ServerCallback * object = nullptr;
	const IBattleInfoCallback * battle = nullptr;
	const battle::Unit * unit = nullptr;
	int64_t damageAmount = 0;

	S.get(1, object);
	S.getNonNull(2, battle);
	S.getNonNull(3, unit);
	S.get(4, damageAmount);

	BattleStackAttacked bsa;
	bsa.damageAmount = damageAmount;
	bsa.stackAttacked = unit->unitId();
	bsa.attackerID = -1;
	auto newState = unit->acquireState();
	CStack::prepareAttacked(bsa, *object->getRNG(), newState);

	StacksInjured si;
	si.battleID = battle->getBattle()->getBattleID();
	si.stacks.push_back(bsa);
	object->apply(si);

	S.clear();
	S.push(bsa.damageAmount);
	S.push(static_cast<int64_t>(bsa.killedAmount));
	return 2;
}

int ServerCallbackProxy::healUnit(lua_State * L)
{
	LuaStack S(L);

	ServerCallback * object = nullptr;
	const IBattleInfoCallback * battle = nullptr;
	const battle::Unit * unit = nullptr;
	int64_t healthDelta;
	EHealLevel healLevel;
	EHealPower healPower;

	S.get(1, object);
	S.getNonNull(2, battle);
	S.getNonNull(3, unit);
	S.get(4, healthDelta);
	S.get(5, healLevel);
	S.get(6, healPower);

	auto changedUnit = unit->acquire();
	battle::HealInfo info = changedUnit->heal(healthDelta, healLevel, healPower);

	BattleUnitsChanged buc;
	UnitChanges uc;
	buc.battleID = battle->getBattle()->getBattleID();
	uc.operation = UnitChanges::EOperation::UPDATE;
	uc.id = unit->unitId();
	uc.data = changedUnit->save();
	uc.healthDelta = info.healedHealthPoints;
	buc.changedStacks.push_back(uc);

	object->apply(buc);

	S.clear();
	S.push(info.healedHealthPoints);
	S.push(static_cast<int64_t>(info.resurrectedCount));
	return 2;
}

}
