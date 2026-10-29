/*
 * SpellCastActions.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "SpellCastActions.h"

#include "IBattleActionEnvironment.h"

#include "../BattleAction.h"
#include "../CBattleInfoCallback.h"
#include "../CUnitState.h"
#include "../../bonuses/BonusSelector.h"
#include "../../mapObjects/CGHeroInstance.h"
#include "../../spells/CSpell.h"
#include "../../spells/ISpellMechanics.h"
#include "../../spells/Problem.h"

#include <vstd/RNG.h>

static bool needsNoTarget(const std::vector<spells::AimType> & types)
{
	return types.empty() || types.front() == spells::AimType::NOTHING;
}

/// Destination that the hovered hex makes for the next aim type of the target, if the hex has what the aim type needs
static std::optional<battle::Destination> getStepDestination(const CBattleInfoCallback & battle, const std::vector<spells::AimType> & types, const ActionContext & context)
{
	const size_t step = context.partialTarget.size();
	if(step >= types.size() || !context.hoveredHex.isValid())
		return std::nullopt;

	if(types.at(step) != spells::AimType::CREATURE)
		return battle::Destination(context.hoveredHex);

	const battle::Unit * unit = battle.battleGetUnitOrCorpseByPos(context.hoveredHex);
	if(!unit)
		return std::nullopt;

	// the unit chosen first stays the target if the battle changes before the last destination is chosen
	if(step == 0 && types.size() > 1)
		return battle::Destination(unit);

	return battle::Destination(context.hoveredHex);
}

static bool checkCanCast(const battle::Unit & unit, spells::Problem & problem)
{
	if(unit.canCast())
		return true;

	problem.add(MetaString::createFromRawString("This stack can't cast a spell now: " + unit.getDescription()));
	return false;
}

static BattleSide getSide(const ActionContext & context)
{
	return context.actor ? context.actor->unitSide() : context.side;
}

std::unique_ptr<spells::Mechanics> SpellCastAction::getMechanics(const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const
{
	const spells::Caster * caster = getCaster(battle, getSide(context), context.actor);
	if(!caster)
		return nullptr;

	const CSpell * spell = option.spell.toSpell();
	const spells::BattleCast cast(&battle, caster, getMode(), spell);
	return spell->battleMechanics(&cast);
}

std::optional<battle::Destination> SpellCastAction::buildTargetStep(const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const
{
	const auto mechanics = getMechanics(battle, option, context);
	if(!mechanics)
		return std::nullopt;

	const auto types = mechanics->getTargetTypes();
	if(context.partialTarget.size() + 1 >= types.size())
		return std::nullopt;

	const auto destination = getStepDestination(battle, types, context);
	if(!destination)
		return std::nullopt;

	battle::Target target = context.partialTarget;
	target.push_back(*destination);
	if(!mechanics->canBeCastAt(target))
		return std::nullopt;

	return destination;
}

std::optional<BattleAction> SpellCastAction::build(const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const
{
	const auto mechanics = getMechanics(battle, option, context);
	if(!mechanics)
		return std::nullopt;

	const auto types = mechanics->getTargetTypes();
	if(needsNoTarget(types))
	{
		// cast as soon as it is selected, without a click on the battlefield
		if(context.hoveredHex.isValid())
			return std::nullopt;
		return makeAction(option, context, {battle::Destination()});
	}

	if(context.partialTarget.size() + 1 != types.size())
		return std::nullopt;

	const auto destination = getStepDestination(battle, types, context);
	if(!destination)
		return std::nullopt;

	battle::Target target = context.partialTarget;
	target.push_back(*destination);
	return makeAction(option, context, target);
}

ActionPreview SpellCastAction::preview(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const
{
	const auto mechanics = getMechanics(battle, option, context);
	if(!mechanics || !context.hoveredHex.isValid())
		return {"combatBlocked", {}, {}, {}};

	const auto step = mechanics->describeTargetStep(context.partialTarget, context.hoveredHex);

	if(!buildTargetStep(battle, option, context) && !isLegal(game, battle, option, context))
		return {"combatBlocked", step.blockedText, {}, {}};

	ActionPreview result{step.cursor, step.text, {}, {}};

	addSpellArea(result, *mechanics, context.hoveredHex);

	return result;
}

bool SpellCastAction::validate(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const BattleAction & action, spells::Problem & problem) const
{
	if(!action.spell.hasValue())
	{
		problem.add(MetaString::createFromRawString("Spell cast without a valid spell: " + action.toString()));
		return false;
	}

	if(!validateCaster(battle, action, problem))
		return false;

	const CSpell * spell = action.spell.toSpell();
	const spells::Caster * caster = getCaster(battle, action.side, battle.battleGetUnitByID(action.stackNumber));
	const spells::BattleCast cast(&battle, caster, getMode(), spell);
	const auto mechanics = spell->battleMechanics(&cast);

	// as BattleCast::cast does
	battle::Target target = action.getTarget(&battle);
	if(target.empty())
		target.emplace_back();

	const auto types = mechanics->getTargetTypes();
	if(!needsNoTarget(types) && target.size() != types.size())
	{
		problem.add(MetaString::createFromRawString("Spell " + spell->getJsonKey() + " is cast at a wrong number of destinations: " + action.toString()));
		return false;
	}

	for(const auto & destination : target)
	{
		if(!needsNoTarget(types) && !destination.hexValue.isValid())
		{
			problem.add(MetaString::createFromRawString("Spell " + spell->getJsonKey() + " is cast at a destination without a hex: " + action.toString()));
			return false;
		}
	}

	if(!mechanics->canBeCastAt(target, problem))
	{
		problem.add(MetaString::createFromRawString("Spell " + spell->getJsonKey() + " can't be cast at its target: " + action.toString()));
		return false;
	}

	return true;
}

spells::Mode HeroSpellAction::getMode() const
{
	return spells::Mode::HERO;
}

const spells::Caster * HeroSpellAction::getCaster(const CBattleInfoCallback & battle, BattleSide side, const battle::Unit * actor) const
{
	return battle.battleGetFightingHero(side);
}

bool HeroSpellAction::validateCaster(const CBattleInfoCallback & battle, const BattleAction & action, spells::Problem & problem) const
{
	if(battle.battleGetFightingHero(action.side))
		return true;

	problem.add(MetaString::createFromRawString("Hero spell cast by a side without a hero: " + action.toString()));
	return false;
}

BattleAction HeroSpellAction::makeAction(const ActionOption & option, const ActionContext & context, const battle::Target & target) const
{
	return BattleAction::makeHeroSpellcast(context.side, target, option.spell);
}

void HeroSpellAction::apply(IBattleActionEnvironment & env, const CBattleInfoCallback & battle, const BattleAction & action) const
{
	env.castHeroSpell(action.side, action.spell.toSpell(), action.getTarget(&battle));
}

bool UnitSpellAction::isUnitAction() const
{
	return true;
}

ActionOption UnitSpellAction::makeOption(SpellID spell) const
{
	return {this, UnitActionButton{6, spell, {}, {}}, spell};
}

void UnitSpellAction::collectOptions(const CBattleInfoCallback & battle, const battle::Unit & actor, std::vector<ActionOption> & out) const
{
	if(battle.battleTacticDist() || !actor.canCast())
		return;

	// a bonus with parameters is one of the spells that a unit rolls for the turn; the client adds the rolled one
	for(const auto & bonus : *actor.getBonusesOfType(BonusType::SPELLCASTER))
		if(!bonus->parameters && bonus->subtype.as<SpellID>().hasValue())
			out.push_back(makeOption(bonus->subtype.as<SpellID>()));
}

int UnitSpellAction::getPriority(const CBattleInfoCallback & battle, const ActionOption & option, const battle::Unit & actor, const battle::Unit * target) const
{
	const auto types = getMechanics(battle, option, {&actor, BattleHex::INVALID})->getTargetTypes();

	// a spell that moves a unit to a hex is tried after the attacks of the unit
	if(types.size() > 1 && types.at(1) != spells::AimType::CREATURE)
		return 15;

	if(!target || actor.hasBonusOfType(BonusType::NO_SPELLCAST_BY_DEFAULT))
		return 100;

	const CSpell * spell = option.spell.toSpell();
	const bool ownTarget = battle.battleGetOwner(target) == battle.battleGetOwner(&actor);

	if((spell->isPositive() && !ownTarget) || (spell->isNegative() && ownTarget))
		return 100;

	return 1;
}

spells::Mode UnitSpellAction::getMode() const
{
	return spells::Mode::CREATURE_ACTIVE;
}

const spells::Caster * UnitSpellAction::getCaster(const CBattleInfoCallback & battle, BattleSide side, const battle::Unit * actor) const
{
	return actor;
}

bool UnitSpellAction::validateCaster(const CBattleInfoCallback & battle, const BattleAction & action, spells::Problem & problem) const
{
	const battle::Unit * unit = battle.battleGetUnitByID(action.stackNumber);
	if(!checkUnitCanAct(battle, unit, problem))
		return false;

	if(!unit->hasBonus(Selector::typeSubtype(BonusType::SPELLCASTER, BonusSubtypeID(action.spell))))
	{
		problem.add(MetaString::createFromRawString("This stack can't cast the spell: " + unit->getDescription()));
		return false;
	}

	return checkCanCast(*unit, problem);
}

BattleAction UnitSpellAction::makeAction(const ActionOption & option, const ActionContext & context, const battle::Target & target) const
{
	return BattleAction::makeCreatureSpellcast(context.actor, target, option.spell);
}

std::optional<battle::UnitTurnStatus> UnitSpellAction::getStartTurnStatus(const BattleAction & action) const
{
	// a spell cast without skipping the turn leaves the unit able to act
	if(action.spell.toSpell()->canCastWithoutSkip())
		return std::nullopt;
	return battle::UnitTurnStatus::ACTED;
}

bool UnitSpellAction::usesUnitSpellcast() const
{
	return true;
}

void UnitSpellAction::apply(IBattleActionEnvironment & env, const CBattleInfoCallback & battle, const BattleAction & action) const
{
	const battle::Unit & unit = getActor(battle, action);
	const CSpell * spell = action.spell.toSpell();

	env.castSpell(unit, spell, spells::Mode::CREATURE_ACTIVE, unit.getSpellSchoolLevel(spell), action.getTarget(&battle));
	env.fireCombatEvent(CombatEventType::UNIT_SPELLCAST, &unit, nullptr, spell);
	env.triggerObstaclesUnder(unit);
}


bool RandomSpellAction::isUnitAction() const
{
	return true;
}

void RandomSpellAction::collectOptions(const CBattleInfoCallback & battle, const battle::Unit & actor, std::vector<ActionOption> & out) const
{
	if(!battle.battleTacticDist() && actor.canCast() && actor.hasBonusOfType(BonusType::RANDOM_SPELLCASTER))
		out.push_back({this, UnitActionButton{4, SpellID::NONE, ImagePath::builtin("battle/actionGenie"), "vcmi.battle.action.genie"}});
}

int RandomSpellAction::getPriority(const CBattleInfoCallback & battle, const ActionOption & option, const battle::Unit & actor, const battle::Unit * target) const
{
	return 2;
}

ActionPreview RandomSpellAction::preview(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const
{
	if(!isLegal(game, battle, option, context))
		return {"combatBlocked", MetaString::createFromTextID("core.genrltxt.23"), {}, {}};

	const battle::Unit * subject = battle.battleGetUnitByPos(context.hoveredHex, true);

	MetaString text = MetaString::createFromTextID("core.genrltxt.301"); // Cast a spell on %s
	text.replaceName(subject->creatureId(), subject->getCount());
	return {"castSpell", text, {}, {}};
}

std::optional<BattleAction> RandomSpellAction::build(const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const
{
	return BattleAction::makeRandomSpellcast(context.actor, context.hoveredHex);
}

bool RandomSpellAction::validate(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const BattleAction & action, spells::Problem & problem) const
{
	const battle::Unit * unit = battle.battleGetUnitByID(action.stackNumber);
	if(!checkUnitCanAct(battle, unit, problem))
		return false;

	if(!unit->hasBonusOfType(BonusType::RANDOM_SPELLCASTER))
	{
		problem.add(MetaString::createFromRawString("That stack can't cast spells!"));
		return false;
	}

	if(!checkCanCast(*unit, problem))
		return false;

	const battle::Target target = action.getTarget(&battle);
	const battle::Unit * subject = target.size() == 1 ? getDestinationUnit(battle, target.front()) : nullptr;
	if(!subject || subject == unit || !subject->alive() || battle.battleMatchOwner(unit, subject))
	{
		problem.add(MetaString::createFromRawString("Invalid target for random spellcaster!"));
		return false;
	}

	if(battle.getBeneficialSpells(unit, subject).empty())
	{
		problem.add(MetaString::createFromRawString("Random spellcaster has no spell of use to the target: " + subject->getDescription()));
		return false;
	}

	return true;
}

bool RandomSpellAction::usesUnitSpellcast() const
{
	return true;
}

void RandomSpellAction::apply(IBattleActionEnvironment & env, const CBattleInfoCallback & battle, const BattleAction & action) const
{
	const battle::Unit & unit = getActor(battle, action);
	const battle::Target target = action.getTarget(&battle);

	const std::vector<SpellID> spells = battle.getBeneficialSpells(&unit, getDestinationUnit(battle, target.front()));
	const CSpell * spell = RandomGeneratorUtil::nextItem(spells, env.getRandom())->toSpell();
	const auto ability = unit.getBonus(Selector::type()(BonusType::RANDOM_SPELLCASTER));

	env.castSpell(unit, spell, spells::Mode::CREATURE_ACTIVE, unit.getAbilitySpellLevel(std::max(0, ability->val), spell), target);
	env.fireCombatEvent(CombatEventType::UNIT_SPELLCAST, &unit, nullptr, spell);
	env.triggerObstaclesUnder(unit);
}

/// Unit that a walk and cast action is aimed at, which may be dead
static const battle::Unit * getWalkAndCastSubject(const CBattleInfoCallback & battle, const battle::Target & target)
{
	return target.at(1).unitValue ? target.at(1).unitValue : battle.battleGetUnitOrCorpseByPos(target.at(1).hexValue);
}

static std::shared_ptr<const Bonus> getAdjacentCastAbility(const battle::Unit & unit, SpellID spell)
{
	return unit.getBonus(Selector::typeSubtype(BonusType::ADJACENT_SPELLCASTER, BonusSubtypeID(spell)));
}

static std::unique_ptr<spells::Mechanics> getUnitMechanics(const CBattleInfoCallback & battle, const battle::Unit & caster, SpellID spellID)
{
	const CSpell * spell = spellID.toSpell();
	const spells::BattleCast cast(&battle, &caster, spells::Mode::CREATURE_ACTIVE, spell);
	return spell->battleMechanics(&cast);
}

bool WalkAndCastAction::isUnitAction() const
{
	return true;
}

void WalkAndCastAction::collectOptions(const CBattleInfoCallback & battle, const battle::Unit & actor, std::vector<ActionOption> & out) const
{
	if(battle.battleTacticDist() || !actor.canCast())
		return;

	for(const auto & bonus : *actor.getBonusesOfType(BonusType::ADJACENT_SPELLCASTER))
		if(bonus->subtype.as<SpellID>().hasValue())
			out.push_back({this, UnitActionButton{6, bonus->subtype.as<SpellID>(), {}, {}}, bonus->subtype.as<SpellID>()});
}

int WalkAndCastAction::getPriority(const CBattleInfoCallback & battle, const ActionOption & option, const battle::Unit & actor, const battle::Unit * target) const
{
	return 9;
}

std::vector<ActionApproach> WalkAndCastAction::getApproaches(const CBattleInfoCallback & battle, const ActionOption & option, const battle::Unit & actor, const BattleHex & hoveredHex) const
{
	const battle::Unit * target = battle.battleGetUnitOrCorpseByPos(hoveredHex);
	if(!target || target == &actor)
		return {};

	return getReachableApproaches(battle, actor, hoveredHex, false);
}

ActionPreview WalkAndCastAction::preview(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const
{
	if(!isLegal(game, battle, option, context))
		return {"combatBlocked", {}, {}, {}};

	const battle::Unit & caster = *context.actor;
	const auto mechanics = getUnitMechanics(battle, caster, option.spell);

	ActionPreview result{"castSpell", mechanics->describeTargetStep({}, context.hoveredHex).text, {}, {}};

	for(const BattleHex & hex : caster.getHexes(context.approach.value().from))
		result.shadedHexes.insert(hex);

	addSpellArea(result, *mechanics, context.hoveredHex);

	return result;
}

std::optional<BattleAction> WalkAndCastAction::build(const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const
{
	if(!context.approach)
		return std::nullopt;

	return BattleAction::makeWalkAndCast(context.actor, context.approach->from, battle.battleGetUnitOrCorpseByPos(context.hoveredHex), option.spell);
}

bool WalkAndCastAction::validate(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const BattleAction & action, spells::Problem & problem) const
{
	const battle::Unit * unit = battle.battleGetUnitByID(action.stackNumber);
	if(!checkUnitCanAct(battle, unit, problem))
		return false;

	if(!action.spell.hasValue() || !getAdjacentCastAbility(*unit, action.spell))
	{
		problem.add(MetaString::createFromRawString("Creature cannot walk and spellcast."));
		return false;
	}

	if(!checkCanCast(*unit, problem))
		return false;

	const battle::Target target = action.getTarget(&battle);
	if(target.size() < 2)
	{
		problem.add(MetaString::createFromRawString("Two destinations required for walk and spellcast action."));
		return false;
	}

	const BattleHex castFrom = target.at(0).hexValue;
	const battle::Unit * subject = getWalkAndCastSubject(battle, target);
	if(!subject || subject == unit)
	{
		problem.add(MetaString::createFromRawString("Invalid target for walk and spellcast"));
		return false;
	}

	// obstacles on the way are not checked: a unit stopped by one, such as a hidden mine, has still made its action
	if(!battle.battleGetAvailableHexes(unit, false).contains(castFrom))
	{
		problem.add(MetaString::createFromRawString("Stack failed walk and spellcast - unable to reach target!"));
		return false;
	}

	bool adjacent = false;
	for(const BattleHex & hex : subject->getHexes())
		adjacent = adjacent || unit->getSurroundingHexes(castFrom).contains(hex);

	if(!adjacent)
	{
		problem.add(MetaString::createFromRawString("Walk and spellcast is not made from a hex next to the target."));
		return false;
	}

	if(!getUnitMechanics(battle, *unit, action.spell)->canBeCastAt({battle::Destination(subject)}, problem))
	{
		problem.add(MetaString::createFromRawString("Spell " + action.spell.toSpell()->getJsonKey() + " can't be cast at its target: " + action.toString()));
		return false;
	}

	return true;
}

void WalkAndCastAction::apply(IBattleActionEnvironment & env, const CBattleInfoCallback & battle, const BattleAction & action) const
{
	const battle::Unit & unit = getActor(battle, action);
	const battle::Target target = action.getTarget(&battle);
	const uint32_t subjectID = getWalkAndCastSubject(battle, target)->unitId();

	// the unit stepped on quicksand or a mine: the cast is cancelled, and the action counts as made
	const UnitMoveResult movement = env.moveUnit(unit, target.at(0).hexValue);
	if(!movement.completed)
		return;

	// handlers of the movement events may have killed or removed either unit
	const battle::Unit * subject = battle.battleGetUnitByID(subjectID);
	if(!unit.alive() || !subject)
		return;

	const CSpell * spell = action.spell.toSpell();
	const auto ability = getAdjacentCastAbility(unit, action.spell);

	env.castSpell(unit, spell, spells::Mode::CREATURE_ACTIVE, unit.getAbilitySpellLevel(std::max(0, ability->val), spell), {battle::Destination(subject)});
	env.fireCombatEvent(CombatEventType::UNIT_SPELLCAST, &unit, nullptr, spell);

	// a unit that moved was charged by the obstacles during the movement
	if(movement.distance == 0)
		env.triggerObstaclesUnder(unit);
}
