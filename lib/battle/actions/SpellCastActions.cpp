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
#include "../../CStack.h"
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

	const battle::Unit * unit = battle.battleGetUnitByPos(context.hoveredHex, true);
	if(!unit)
		unit = battle.battleGetUnitByPos(context.hoveredHex, false);
	if(!unit)
		return std::nullopt;

	// the unit chosen first stays the target if the battle changes before the last destination is chosen
	if(step == 0 && types.size() > 1)
		return battle::Destination(unit);

	return battle::Destination(context.hoveredHex);
}

/// StartAction has set castSpellThisTurn of the unit already, so Unit::canCast is false in validate for every cast
static bool checkCastsLeft(const battle::Unit & unit, spells::Problem & problem)
{
	if(unit.acquireState()->casts.canUse(1))
		return true;

	problem.add(MetaString::createFromRawString("This stack has no spell casts left: " + unit.getDescription()));
	return false;
}

/// Level of a spell that the unit casts through the ability, which Magic Plains raise
static int getAbilitySpellLevel(const battle::Unit & unit, const Bonus & ability, const CSpell * spell)
{
	int level = std::max(0, ability.val);
	if(spell->getLevel() > 0)
		vstd::amax(level, unit.valOfBonuses(BonusType::MAGIC_SCHOOL_SKILL, BonusSubtypeID(SpellSchool::ANY)));
	return level;
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

	for(const BattleHex & hex : mechanics->rangeInHexes(context.hoveredHex))
		if(hex.getX() != 0 && hex.getX() != GameConstants::BFIELD_WIDTH - 1)
			result.shadedHexes.insert(hex);

	for(const CStack * unit : mechanics->getAffectedStacks({battle::Destination(context.hoveredHex)}))
		if(!unit->coversPos(context.hoveredHex))
			result.affectedUnits.push_back(unit->unitId());

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
	BattleAction action;
	action.actionType = EActionType::HERO_SPELL;
	action.side = context.side;
	action.stackNumber = -1;
	action.spell = option.spell;
	action.setTarget(target);
	return action;
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

	return checkCastsLeft(*unit, problem);
}

BattleAction UnitSpellAction::makeAction(const ActionOption & option, const ActionContext & context, const battle::Target & target) const
{
	return BattleAction::makeCreatureSpellcast(context.actor, target, option.spell);
}

void UnitSpellAction::applyStartState(battle::CUnitState & actor, const BattleAction & action) const
{
	// a spell cast without skipping the turn leaves the unit able to act
	if(!action.spell.toSpell()->canCastWithoutSkip())
	{
		actor.waiting = false;
		actor.movedThisRound = true;
	}
	actor.castSpellThisTurn = true;
}

void UnitSpellAction::apply(IBattleActionEnvironment & env, const CBattleInfoCallback & battle, const BattleAction & action) const
{
	const battle::Unit & unit = getActor(battle, action);
	const CSpell * spell = action.spell.toSpell();

	env.castSpell(unit, spell, spells::Mode::CREATURE_ACTIVE, unit.getSpellSchoolLevel(spell), action.getTarget(&battle));
	env.fireCombatEvent(CombatEventType::UNIT_SPELLCAST, &unit, nullptr, spell);
}

/// Unit aimed at by the only destination of the action, or nullptr
static const battle::Unit * getSubject(const CBattleInfoCallback & battle, const battle::Target & target)
{
	if(target.size() != 1)
		return nullptr;
	if(target.front().unitValue)
		return target.front().unitValue;
	return battle.battleGetUnitByPos(target.front().hexValue, true);
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
	BattleAction action;
	action.actionType = EActionType::RANDOM_SPELL;
	action.side = context.actor->unitSide();
	action.stackNumber = context.actor->unitId();
	action.aimToHex(context.hoveredHex);
	return action;
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

	if(!checkCastsLeft(*unit, problem))
		return false;

	const battle::Unit * subject = getSubject(battle, action.getTarget(&battle));
	if(!subject || subject == unit || !subject->alive() || subject->unitOwner() != unit->unitOwner())
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

void RandomSpellAction::applyStartState(battle::CUnitState & actor, const BattleAction & action) const
{
	BattleActionType::applyStartState(actor, action);
	actor.castSpellThisTurn = true;
}

void RandomSpellAction::apply(IBattleActionEnvironment & env, const CBattleInfoCallback & battle, const BattleAction & action) const
{
	const battle::Unit & unit = getActor(battle, action);
	const battle::Target target = action.getTarget(&battle);

	const std::vector<SpellID> spells = battle.getBeneficialSpells(&unit, getSubject(battle, target));
	const CSpell * spell = RandomGeneratorUtil::nextItem(spells, env.getRandom())->toSpell();
	const auto ability = unit.getBonus(Selector::type()(BonusType::RANDOM_SPELLCASTER));

	env.castSpell(unit, spell, spells::Mode::CREATURE_ACTIVE, getAbilitySpellLevel(unit, *ability, spell), target);
	env.fireCombatEvent(CombatEventType::UNIT_SPELLCAST, &unit, nullptr, spell);
}
