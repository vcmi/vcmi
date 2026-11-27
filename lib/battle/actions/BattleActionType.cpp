/*
 * BattleActionType.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleActionType.h"

#include "BattleActionTypeHandler.h"
#include "SpellCastActions.h"

#include "../BattleAction.h"
#include "../CBattleInfoCallback.h"
#include "../CUnitState.h"
#include "../../CStack.h"
#include "../../GameLibrary.h"
#include "../../networkPacks/PacksForClientBattle.h"
#include "../../spells/ISpellMechanics.h"
#include "../../spells/Problem.h"

BattleActionType::BattleActionType(BattleActionID id)
	: id(id)
{
}

BattleActionID BattleActionType::getId() const
{
	return id;
}

const BattleActionType * BattleActionType::find(const BattleAction & action)
{
	return find(action.actionType);
}

const BattleActionType * BattleActionType::find(BattleActionID actionType)
{
	return LIBRARY->battleActionTypeHandler->getById(static_cast<int32_t>(actionType));
}

ActionOption BattleActionType::makeHeroSpellOption(SpellID spell)
{
	return {find(BattleActionID::HERO_SPELL), std::nullopt, spell};
}

ActionOption BattleActionType::makeUnitSpellOption(SpellID spell)
{
	const auto * type = dynamic_cast<const UnitSpellAction *>(find(BattleActionID::UNIT_SPELL));
	if(!type)
		throw std::runtime_error("Unit spell action type is not registered");
	return type->makeOption(spell);
}

void BattleActionType::collectAllOptions(const CBattleInfoCallback & battle, const battle::Unit & actor, std::vector<ActionOption> & out)
{
	for(const auto * type : LIBRARY->battleActionTypeHandler->getAll())
		type->collectOptions(battle, actor, out);
}

bool BattleActionType::isUnitAction() const
{
	return false;
}

bool BattleActionType::isTacticsAction() const
{
	return false;
}

bool BattleActionType::isBattleEndAction() const
{
	return false;
}

std::optional<battle::UnitTurnStatus> BattleActionType::getStartTurnStatus(const BattleAction & action) const
{
	return battle::UnitTurnStatus::ACTED;
}

bool BattleActionType::usesUnitSpellcast() const
{
	return false;
}

StartAction BattleActionType::makeStartAction(const CBattleInfoCallback & battle, const BattleAction & action)
{
	StartAction pack(action);
	pack.battleID = battle.getBattle()->getBattleID();

	// tactics phase moves leave the turn state alone, the client manages the tactics queue
	const auto * type = find(action);
	if(type && type->isUnitAction() && !battle.battleTacticDist())
	{
		pack.turnStatus = type->getStartTurnStatus(action);
		pack.castSpellThisTurn = type->usesUnitSpellcast();
	}
	return pack;
}

void BattleActionType::collectOptions(const CBattleInfoCallback & battle, const battle::Unit & actor, std::vector<ActionOption> & out) const
{
}

int BattleActionType::getPriority(const CBattleInfoCallback & battle, const ActionOption & option, const battle::Unit & actor, const battle::Unit * target) const
{
	throw std::runtime_error("Priority requested from a battle action type that offers no options");
}

std::vector<ActionApproach> BattleActionType::getApproaches(const CBattleInfoCallback & battle, const ActionOption & option, const battle::Unit & actor, const BattleHex & hoveredHex) const
{
	return {};
}

bool BattleActionType::isLegal(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const
{
	const std::optional<BattleAction> action = build(battle, option, context);
	if(!action)
		return false;

	spells::detail::ProblemImpl problem;
	return validate(game, battle, *action, problem);
}

ActionPreview BattleActionType::preview(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const
{
	throw std::runtime_error("Preview requested from a battle action type that offers no options");
}

std::optional<battle::Destination> BattleActionType::buildTargetStep(const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const
{
	return std::nullopt;
}

std::optional<battle::Target> BattleActionType::buildTarget(const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const
{
	throw std::runtime_error("Action built from a battle action type that offers no options");
}

std::optional<BattleAction> BattleActionType::build(const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const
{
	const std::optional<battle::Target> target = buildTarget(battle, option, context);
	if(!target)
		return std::nullopt;

	BattleAction action;
	action.actionType = id;
	action.spell = option.spell;
	action.setTarget(*target);

	// the context of a hero option may name the active unit, which does not make the action
	if(isUnitAction())
	{
		if(!context.actor)
			throw std::runtime_error("Unit action built without an acting unit");
		action.side = context.actor->unitSide();
		action.stackNumber = context.actor->unitId();
	}
	else
	{
		action.side = context.side;
	}

	return action;
}

bool BattleActionType::checkUnitCanAct(const CBattleInfoCallback & battle, const battle::Unit * unit, spells::Problem & problem)
{
	if(!unit)
	{
		problem.add(MetaString::createFromRawString("No such stack!"));
		return false;
	}

	if(!unit->alive())
	{
		problem.add(MetaString::createFromRawString("This stack is dead: " + unit->getDescription()));
		return false;
	}

	if(battle.battleTacticDist())
	{
		if(unit->unitSide() != battle.battleGetTacticsSide())
		{
			problem.add(MetaString::createFromRawString("This is not a stack of side that has tactics!"));
			return false;
		}
	}
	else if(unit != battle.battleActiveUnit())
	{
		problem.add(MetaString::createFromRawString("Action has to be about active stack!"));
		return false;
	}

	return true;
}

const battle::Unit * BattleActionType::getDestinationUnit(const CBattleInfoCallback & battle, const battle::Destination & destination)
{
	if(destination.unitValue)
		return destination.unitValue;
	return battle.battleGetUnitByPos(destination.hexValue, true);
}

std::vector<ActionApproach> BattleActionType::getReachableApproaches(const CBattleInfoCallback & battle, const battle::Unit & actor, const BattleHex & hoveredHex, bool longWeapon)
{
	const BattleHexArray availableHexes = battle.battleGetAvailableHexes(&actor, false);
	std::vector<ActionApproach> result;

	for(const auto direction : {BattleHex::TOP_LEFT, BattleHex::TOP_RIGHT, BattleHex::RIGHT, BattleHex::BOTTOM_RIGHT, BattleHex::BOTTOM_LEFT, BattleHex::LEFT, BattleHex::TOP, BattleHex::BOTTOM})
	{
		const BattleHex from = battle.findAttackPosition(availableHexes, &actor, hoveredHex, direction, longWeapon);
		if(from.isValid())
			result.push_back({direction, from});
	}

	return result;
}

void BattleActionType::addSpellArea(ActionPreview & preview, const spells::Mechanics & mechanics, const BattleHex & hoveredHex)
{
	// the edge columns are not part of the battlefield shown to the player
	for(const BattleHex & hex : mechanics.rangeInHexes(hoveredHex))
		if(hex.getX() != 0 && hex.getX() != GameConstants::BFIELD_WIDTH - 1)
			preview.shadedHexes.insert(hex);

	for(const CStack * unit : mechanics.getAffectedStacks({battle::Destination(hoveredHex)}))
		if(!unit->coversPos(hoveredHex))
			preview.affectedUnits.push_back(unit->unitId());
}

const battle::Unit & BattleActionType::getActor(const CBattleInfoCallback & battle, const BattleAction & action)
{
	const battle::Unit * unit = battle.battleGetUnitByID(action.stackNumber);
	if(!unit)
		throw std::runtime_error("Battle action without an existing actor: " + action.toString());
	return *unit;
}
