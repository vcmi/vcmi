/*
 * BattleActionsController.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleActionsController.h"

#include "BattleWindow.h"
#include "BattleStacksController.h"
#include "BattleInterface.h"
#include "BattleFieldController.h"
#include "BattleSiegeController.h"
#include "BattleInterfaceClasses.h"

#include "../CPlayerInterface.h"
#include "../gui/CursorHandler.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/CIntObject.h"
#include "../gui/WindowHandler.h"
#include "../windows/CCreatureWindow.h"
#include "../windows/InfoWindows.h"

#include "../../CCallback.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/CRandomGenerator.h"
#include "../../lib/CStack.h"
#include "../../lib/battle/BattleAction.h"
#include "../../lib/spells/CSpellHandler.h"
#include "../../lib/spells/ISpellMechanics.h"
#include "../../lib/spells/Problem.h"

struct TextReplacement
{
	std::string placeholder;
	std::string replacement;
};

using TextReplacementList = std::vector<TextReplacement>;

static std::string replacePlaceholders(std::string input, const TextReplacementList & format )
{
	for(const auto & entry : format)
		boost::replace_all(input, entry.placeholder, entry.replacement);

	return input;
}

static std::string translatePlural(int amount, const std::string& baseTextID)
{
	if(amount == 1)
		return LIBRARY->generaltexth->translate(baseTextID + ".1");
	return LIBRARY->generaltexth->translate(baseTextID);
}

static std::string formatPluralImpl(int amount, const std::string & amountString, const std::string & baseTextID)
{
	std::string baseString = translatePlural(amount, baseTextID);
	TextReplacementList replacements {
		{ "%d", amountString }
	};

	return replacePlaceholders(baseString, replacements);
}

static std::string formatPlural(int amount, const std::string & baseTextID)
{
	return formatPluralImpl(amount, std::to_string(amount), baseTextID);
}

static std::string formatPlural(DamageRange range, const std::string & baseTextID)
{
	if (range.min == range.max)
		return formatPlural(range.min, baseTextID);

	std::string rangeString = std::to_string(range.min) + " - " + std::to_string(range.max);

	return formatPluralImpl(range.max, rangeString, baseTextID);
}

static std::string formatAttack(const DamageEstimation & estimation, const std::string & creatureName, const std::string & baseTextID, int shotsLeft)
{
	TextReplacementList replacements = {
		{ "%CREATURE", creatureName },
		{ "%DAMAGE", formatPlural(estimation.damage, "vcmi.battleWindow.damageEstimation.damage") },
		{ "%SHOTS", formatPlural(shotsLeft, "vcmi.battleWindow.damageEstimation.shots") },
		{ "%KILLS", formatPlural(estimation.kills, "vcmi.battleWindow.damageEstimation.kills") },
	};

	return replacePlaceholders(LIBRARY->generaltexth->translate(baseTextID), replacements);
}

static std::string formatMeleeAttack(const DamageEstimation & estimation, const std::string & creatureName)
{
	std::string baseTextID = estimation.kills.max == 0 ?
		"vcmi.battleWindow.damageEstimation.melee" :
		"vcmi.battleWindow.damageEstimation.meleeKills";

	return formatAttack(estimation, creatureName, baseTextID, 0);
}

static std::string formatRangedAttack(const DamageEstimation & estimation, const std::string & creatureName, int shotsLeft)
{
	std::string baseTextID = estimation.kills.max == 0 ?
		"vcmi.battleWindow.damageEstimation.ranged" :
		"vcmi.battleWindow.damageEstimation.rangedKills";

	return formatAttack(estimation, creatureName, baseTextID, shotsLeft);
}

static std::string formatRetaliation(const DamageEstimation & estimation, bool mayBeKilled)
{
	if (estimation.damage.max == 0)
		return LIBRARY->generaltexth->translate("vcmi.battleWindow.damageRetaliation.never");

	std::string baseTextID = estimation.kills.max == 0 ?
								 "vcmi.battleWindow.damageRetaliation.damage" :
								 "vcmi.battleWindow.damageRetaliation.damageKills";

	std::string prefixTextID = mayBeKilled ?
		"vcmi.battleWindow.damageRetaliation.may" :
		"vcmi.battleWindow.damageRetaliation.will";

	return LIBRARY->generaltexth->translate(prefixTextID) + formatAttack(estimation, "", baseTextID, 0);
}

BattleActionsController::BattleActionsController(BattleInterface & owner):
	owner(owner),
	selectedStack(nullptr),
	heroSpellToCast(nullptr)
{
}

void BattleActionsController::endCastingSpell()
{
	if(heroSpellToCast)
	{
		heroSpellToCast.reset();
		owner.windowObject->blockUI(false);
	}

	if(owner.stacksController->getActiveStack())
		possibleActions = getPossibleActionsForStack(owner.stacksController->getActiveStack()); //restore actions after they were cleared

	selectedStack = nullptr;
	ENGINE->fakeMouseMove();
}

bool BattleActionsController::isActiveStackSpellcaster() const
{
	const CStack * casterStack = owner.stacksController->getActiveStack();
	if (!casterStack)
		return false;

	bool spellcaster = casterStack->hasBonusOfType(BonusType::SPELLCASTER);
	return (spellcaster && casterStack->canCast());
}

void BattleActionsController::enterCreatureCastingMode()
{
	//silently check for possible errors
	if (owner.tacticsMode)
		return;

	//hero is casting a spell
	if (heroSpellToCast)
		return;

	if (!owner.stacksController->getActiveStack())
		return;

	if(owner.getBattle()->battleCanTargetEmptyHex(owner.stacksController->getActiveStack()))
	{
		auto actionFilterPredicate = [](const PossiblePlayerBattleAction x)
		{
			return x.get() != PossiblePlayerBattleAction::SHOOT;
		};

		vstd::erase_if(possibleActions, actionFilterPredicate);
		ENGINE->fakeMouseMove();
		return;
	}

	if (!isActiveStackSpellcaster())
		return;

	for(const auto & action : possibleActions)
	{
		if (action.get() != PossiblePlayerBattleAction::NO_LOCATION)
			continue;

		const spells::Caster * caster = owner.stacksController->getActiveStack();
		const CSpell * spell = action.spell().toSpell();

		spells::Target target;
		target.emplace_back();

		spells::BattleCast cast(owner.getBattle().get(), caster, spells::Mode::CREATURE_ACTIVE, spell);

		auto m = spell->battleMechanics(&cast);
		spells::detail::ProblemImpl ignored;

		const bool isCastingPossible = m->canBeCastAt(target, ignored);

		if (isCastingPossible)
		{
			owner.giveCommand(EActionType::MONSTER_SPELL, BattleHex::INVALID, spell->getId());
			selectedStack = nullptr;

			ENGINE->cursor().set(Cursor::Combat::POINTER);
		}
		return;
	}

	possibleActions = getPossibleActionsForStack(owner.stacksController->getActiveStack());

	auto actionFilterPredicate = [](const PossiblePlayerBattleAction x)
	{
		return !x.spellcast();
	};

	vstd::erase_if(possibleActions, actionFilterPredicate);
	ENGINE->fakeMouseMove();
}

std::vector<PossiblePlayerBattleAction> BattleActionsController::getPossibleActionsForStack(const CStack *stack) const
{
	BattleClientInterfaceData data; //hard to get rid of these things so for now they're required data to pass

	for(const auto & spell : creatureSpells)
		data.creatureSpellsToCast.push_back(spell->id);

	data.tacticsMode = owner.tacticsMode;
	auto allActions = owner.getBattle()->getClientActionsForStack(stack, data);

	allActions.push_back(PossiblePlayerBattleAction::HERO_INFO);
	allActions.push_back(PossiblePlayerBattleAction::CREATURE_INFO);

	return std::vector<PossiblePlayerBattleAction>(allActions);
}

void BattleActionsController::reorderPossibleActionsPriority(const CStack * stack, const CStack * targetStack)
{
	if(owner.tacticsMode || possibleActions.empty()) return; //this function is not supposed to be called in tactics mode or before getPossibleActionsForStack

	auto assignPriority = [&](const PossiblePlayerBattleAction & item
						  ) -> uint8_t //large lambda assigning priority which would have to be part of possibleActions without it
	{
		switch(item.get())
		{
			case PossiblePlayerBattleAction::AIMED_SPELL_CREATURE:
			case PossiblePlayerBattleAction::ANY_LOCATION:
			case PossiblePlayerBattleAction::NO_LOCATION:
			case PossiblePlayerBattleAction::FREE_LOCATION:
			case PossiblePlayerBattleAction::OBSTACLE:
				if(!stack->hasBonusOfType(BonusType::NO_SPELLCAST_BY_DEFAULT) && targetStack != nullptr)
				{
					PlayerColor stackOwner = owner.getBattle()->battleGetOwner(targetStack);
					bool enemyTargetingPositiveSpellcast = item.spell().toSpell()->isPositive() && stackOwner != owner.curInt->playerID;
					bool friendTargetingNegativeSpellcast = item.spell().toSpell()->isNegative() && stackOwner == owner.curInt->playerID;

					if(!enemyTargetingPositiveSpellcast && !friendTargetingNegativeSpellcast)
						return 1;
				}
				return 100; //bottom priority

				break;
			case PossiblePlayerBattleAction::RANDOM_GENIE_SPELL:
				return 2;
				break;
			case PossiblePlayerBattleAction::SHOOT:
				if(targetStack == nullptr || targetStack->unitSide() == stack->unitSide() || !targetStack->alive())
					return 100; //bottom priority

				return 4;
				break;
			case PossiblePlayerBattleAction::ATTACK_AND_RETURN:
				return 5;
				break;
			case PossiblePlayerBattleAction::ATTACK:
				return 6;
				break;
			case PossiblePlayerBattleAction::WALK_AND_ATTACK:
				return 7;
				break;
			case PossiblePlayerBattleAction::MOVE_STACK:
				return 8;
				break;
			case PossiblePlayerBattleAction::CATAPULT:
				return 9;
				break;
			case PossiblePlayerBattleAction::HEAL:
				return 10;
				break;
			case PossiblePlayerBattleAction::CREATURE_INFO:
				return 11;
				break;
			case PossiblePlayerBattleAction::HERO_INFO:
				return 12;
				break;
			case PossiblePlayerBattleAction::TELEPORT:
				return 13;
				break;
			default:
				assert(0);
				return 200;
				break;
		}
	};

	auto comparer = [&](const PossiblePlayerBattleAction & lhs, const PossiblePlayerBattleAction & rhs)
	{
		return assignPriority(lhs) < assignPriority(rhs);
	};

	std::sort(possibleActions.begin(), possibleActions.end(), comparer);
}

void BattleActionsController::castThisSpell(SpellID spellID)
{
	heroSpellToCast = std::make_shared<BattleAction>();
	heroSpellToCast->actionType = EActionType::HERO_SPELL;
	heroSpellToCast->spell = spellID;
	heroSpellToCast->stackNumber = -1;
	heroSpellToCast->side = owner.curInt->cb->getBattle(owner.getBattleID())->battleGetMySide();

	//choosing possible targets
	const CGHeroInstance *castingHero = (owner.attackingHeroInstance->tempOwner == owner.curInt->playerID) ? owner.attackingHeroInstance : owner.defendingHeroInstance;
	assert(castingHero); // code below assumes non-null hero
	PossiblePlayerBattleAction spellSelMode = owner.getBattle()->getCasterAction(spellID.toSpell(), castingHero, spells::Mode::HERO);

	if (spellSelMode.get() == PossiblePlayerBattleAction::NO_LOCATION) //user does not have to select location
	{
		heroSpellToCast->aimToHex(BattleHex::INVALID);
		owner.curInt->cb->battleMakeSpellAction(owner.getBattleID(), *heroSpellToCast);
		endCastingSpell();
	}
	else
	{
		possibleActions.clear();
		possibleActions.push_back (spellSelMode); //only this one action can be performed at the moment
		ENGINE->fakeMouseMove();//update cursor
	}

	owner.windowObject->blockUI(true);
}

const CSpell * BattleActionsController::getHeroSpellToCast( ) const
{
	if (heroSpellToCast)
		return heroSpellToCast->spell.toSpell();
	return nullptr;
}

const CSpell * BattleActionsController::getStackSpellToCast(const BattleHex & hoveredHex)
{
	if (heroSpellToCast)
		return nullptr;

	if (!owner.stacksController->getActiveStack())
		return nullptr;

	if (!hoveredHex.isValid())
		return nullptr;

	auto action = selectAction(hoveredHex);

	if(owner.stacksController->getActiveStack()->hasBonusOfType(BonusType::SPELL_LIKE_ATTACK))
	{
		auto bonus = owner.stacksController->getActiveStack()->getBonus(Selector::type()(BonusType::SPELL_LIKE_ATTACK));
		return bonus->subtype.as<SpellID>().toSpell();
	}

	if (action.spell() == SpellID::NONE)
		return nullptr;

	return action.spell().toSpell();
}

const CSpell * BattleActionsController::getCurrentSpell(const BattleHex & hoveredHex)
{
	if (getHeroSpellToCast())
		return getHeroSpellToCast();
	return getStackSpellToCast(hoveredHex);
}

const CStack * BattleActionsController::getStackForHex(const BattleHex & hoveredHex)
{
	const CStack * shere = owner.getBattle()->battleGetStackByPos(hoveredHex, true);
	if(shere)
		return shere;
	return owner.getBattle()->battleGetStackByPos(hoveredHex, false);
}

void BattleActionsController::actionSetCursor(PossiblePlayerBattleAction action, const BattleHex & targetHex)
{
	switch (action.get())
	{
		case PossiblePlayerBattleAction::CHOOSE_TACTICS_STACK:
			ENGINE->cursor().set(Cursor::Combat::POINTER);
			return;

		case PossiblePlayerBattleAction::MOVE_TACTICS:
		case PossiblePlayerBattleAction::MOVE_STACK:
			if (owner.stacksController->getActiveStack()->hasBonusOfType(BonusType::FLYING))
				ENGINE->cursor().set(Cursor::Combat::FLY);
			else
				ENGINE->cursor().set(Cursor::Combat::MOVE);
			return;

		case PossiblePlayerBattleAction::ATTACK:
		case PossiblePlayerBattleAction::WALK_AND_ATTACK:
		case PossiblePlayerBattleAction::ATTACK_AND_RETURN:
		{
			static const std::map<BattleHex::EDir, Cursor::Combat> sectorCursor = {
				{BattleHex::TOP_LEFT,     Cursor::Combat::HIT_SOUTHEAST},
				{BattleHex::TOP_RIGHT,    Cursor::Combat::HIT_SOUTHWEST},
				{BattleHex::RIGHT,        Cursor::Combat::HIT_WEST     },
				{BattleHex::BOTTOM_RIGHT, Cursor::Combat::HIT_NORTHWEST},
				{BattleHex::BOTTOM_LEFT,  Cursor::Combat::HIT_NORTHEAST},
				{BattleHex::LEFT,         Cursor::Combat::HIT_EAST     },
				{BattleHex::TOP,          Cursor::Combat::HIT_SOUTH    },
				{BattleHex::BOTTOM,       Cursor::Combat::HIT_NORTH    }
			};

			auto direction = owner.fieldController->selectAttackDirection(targetHex);

			assert(sectorCursor.count(direction) > 0);
			if (sectorCursor.count(direction))
				ENGINE->cursor().set(sectorCursor.at(direction));

			return;
		}

		case PossiblePlayerBattleAction::SHOOT:
			if (owner.getBattle()->battleHasShootingPenalty(owner.stacksController->getActiveStack(), targetHex))
				ENGINE->cursor().set(Cursor::Combat::SHOOT_PENALTY);
			else
				ENGINE->cursor().set(Cursor::Combat::SHOOT);
			return;

		case PossiblePlayerBattleAction::AIMED_SPELL_CREATURE:
		case PossiblePlayerBattleAction::ANY_LOCATION:
		case PossiblePlayerBattleAction::RANDOM_GENIE_SPELL:
		case PossiblePlayerBattleAction::FREE_LOCATION:
		case PossiblePlayerBattleAction::OBSTACLE:
			ENGINE->cursor().set(Cursor::Spellcast::SPELL);
			return;

		case PossiblePlayerBattleAction::TELEPORT:
			ENGINE->cursor().set(Cursor::Combat::TELEPORT);
			return;

		case PossiblePlayerBattleAction::SACRIFICE:
			ENGINE->cursor().set(Cursor::Combat::SACRIFICE);
			return;

		case PossiblePlayerBattleAction::HEAL:
			ENGINE->cursor().set(Cursor::Combat::HEAL);
			return;

		case PossiblePlayerBattleAction::CATAPULT:
			ENGINE->cursor().set(Cursor::Combat::SHOOT_CATAPULT);
			return;

		case PossiblePlayerBattleAction::CREATURE_INFO:
			ENGINE->cursor().set(Cursor::Combat::QUERY);
			return;
		case PossiblePlayerBattleAction::HERO_INFO:
			ENGINE->cursor().set(Cursor::Combat::HERO);
			return;
	}
	assert(0);
}

void BattleActionsController::actionSetCursorBlocked(PossiblePlayerBattleAction action, const BattleHex & targetHex)
{
	switch (action.get())
	{
		case PossiblePlayerBattleAction::AIMED_SPELL_CREATURE:
		case PossiblePlayerBattleAction::RANDOM_GENIE_SPELL:
		case PossiblePlayerBattleAction::TELEPORT:
		case PossiblePlayerBattleAction::SACRIFICE:
		case PossiblePlayerBattleAction::FREE_LOCATION:
			ENGINE->cursor().set(Cursor::Combat::BLOCKED);
			return;
		default:
			if (targetHex == -1)
				ENGINE->cursor().set(Cursor::Combat::POINTER);
			else
				ENGINE->cursor().set(Cursor::Combat::BLOCKED);
			return;
	}
	assert(0);
}

std::string BattleActionsController::actionGetStatusMessage(PossiblePlayerBattleAction action, const BattleHex & targetHex)
{
	const CStack * targetStack = getStackForHex(targetHex);

	switch (action.get()) //display console message, realize selected action
	{
		case PossiblePlayerBattleAction::CHOOSE_TACTICS_STACK:
			return (boost::format(LIBRARY->generaltexth->allTexts[481]) % targetStack->getName()).str(); //Select %s

		case PossiblePlayerBattleAction::MOVE_TACTICS:
		case PossiblePlayerBattleAction::MOVE_STACK:
			if (owner.stacksController->getActiveStack()->hasBonusOfType(BonusType::FLYING))
				return (boost::format(LIBRARY->generaltexth->allTexts[295]) % owner.stacksController->getActiveStack()->getName()).str(); //Fly %s here
			else
				return (boost::format(LIBRARY->generaltexth->allTexts[294]) % owner.stacksController->getActiveStack()->getName()).str(); //Move %s here

		case PossiblePlayerBattleAction::ATTACK:
		case PossiblePlayerBattleAction::WALK_AND_ATTACK:
		case PossiblePlayerBattleAction::ATTACK_AND_RETURN: //TODO: allow to disable return
			{
				const auto * attacker = owner.stacksController->getActiveStack();
				BattleHex attackFromHex = owner.fieldController->fromWhichHexAttack(targetHex);
				int distance = attacker->position.isValid() ? owner.getBattle()->battleGetDistances(attacker, attacker->getPosition())[attackFromHex.toInt()] : 0;
				DamageEstimation retaliation;
				BattleAttackInfo attackInfo(attacker, targetStack, distance, false );
				DamageEstimation estimation = owner.getBattle()->battleEstimateDamage(attackInfo, &retaliation);
				estimation.kills.max = std::min<int64_t>(estimation.kills.max, targetStack->getCount());
				estimation.kills.min = std::min<int64_t>(estimation.kills.min, targetStack->getCount());
				bool enemyMayBeKilled = estimation.kills.max == targetStack->getCount();

				return formatMeleeAttack(estimation, targetStack->getName()) + "\n" + formatRetaliation(retaliation, enemyMayBeKilled);
			}

		case PossiblePlayerBattleAction::SHOOT:
		{
			if(targetStack == nullptr) //should be true only for spell-like attack
			{
				auto spellLikeAttackBonus = owner.stacksController->getActiveStack()->getBonus(Selector::type()(BonusType::SPELL_LIKE_ATTACK));
				assert(spellLikeAttackBonus != nullptr);
				return boost::str(boost::format(LIBRARY->generaltexth->allTexts[26]) % spellLikeAttackBonus->subtype.as<SpellID>().toSpell()->getNameTranslated());
			}

			const auto * shooter = owner.stacksController->getActiveStack();

			DamageEstimation retaliation;
			BattleAttackInfo attackInfo(shooter, targetStack, 0, true );
			DamageEstimation estimation = owner.getBattle()->battleEstimateDamage(attackInfo, &retaliation);
			estimation.kills.max = std::min<int64_t>(estimation.kills.max, targetStack->getCount());
			estimation.kills.min = std::min<int64_t>(estimation.kills.min, targetStack->getCount());
			return formatRangedAttack(estimation, targetStack->getName(), shooter->shots.available());
		}

		case PossiblePlayerBattleAction::AIMED_SPELL_CREATURE:
			return boost::str(boost::format(LIBRARY->generaltexth->allTexts[27]) % action.spell().toSpell()->getNameTranslated() % targetStack->getName()); //Cast %s on %s

		case PossiblePlayerBattleAction::ANY_LOCATION:
			return boost::str(boost::format(LIBRARY->generaltexth->allTexts[26]) % action.spell().toSpell()->getNameTranslated()); //Cast %s

		case PossiblePlayerBattleAction::RANDOM_GENIE_SPELL: //we assume that teleport / sacrifice will never be available as random spell
			return boost::str(boost::format(LIBRARY->generaltexth->allTexts[301]) % targetStack->getName()); //Cast a spell on %

		case PossiblePlayerBattleAction::TELEPORT:
			return LIBRARY->generaltexth->allTexts[25]; //Teleport Here

		case PossiblePlayerBattleAction::OBSTACLE:
			return LIBRARY->generaltexth->allTexts[550];

		case PossiblePlayerBattleAction::SACRIFICE:
			return (boost::format(LIBRARY->generaltexth->allTexts[549]) % targetStack->getName()).str(); //sacrifice the %s

		case PossiblePlayerBattleAction::FREE_LOCATION:
			return boost::str(boost::format(LIBRARY->generaltexth->allTexts[26]) % action.spell().toSpell()->getNameTranslated()); //Cast %s

		case PossiblePlayerBattleAction::HEAL:
			return (boost::format(LIBRARY->generaltexth->allTexts[419]) % targetStack->getName()).str(); //Apply first aid to the %s

		case PossiblePlayerBattleAction::CATAPULT:
			return ""; // TODO

		case PossiblePlayerBattleAction::CREATURE_INFO:
			return (boost::format(LIBRARY->generaltexth->allTexts[297]) % targetStack->getName()).str();

		case PossiblePlayerBattleAction::HERO_INFO:
			return  LIBRARY->generaltexth->translate("core.genrltxt.417"); // "View Hero Stats"
	}
	assert(0);
	return "";
}

std::string BattleActionsController::actionGetStatusMessageBlocked(PossiblePlayerBattleAction action, const BattleHex & targetHex)
{
	switch (action.get())
	{
		case PossiblePlayerBattleAction::AIMED_SPELL_CREATURE:
		case PossiblePlayerBattleAction::RANDOM_GENIE_SPELL:
			return LIBRARY->generaltexth->allTexts[23];
			break;
		case PossiblePlayerBattleAction::TELEPORT:
			return LIBRARY->generaltexth->allTexts[24]; //Invalid Teleport Destination
			break;
		case PossiblePlayerBattleAction::SACRIFICE:
			return LIBRARY->generaltexth->allTexts[543]; //choose army to sacrifice
			break;
		case PossiblePlayerBattleAction::FREE_LOCATION:
			return boost::str(boost::format(LIBRARY->generaltexth->allTexts[181]) % action.spell().toSpell()->getNameTranslated()); //No room to place %s here
			break;
		default:
			return "";
	}
}

bool BattleActionsController::actionIsLegal(PossiblePlayerBattleAction action, const BattleHex & targetHex)
{
	const CStack * targetStack = getStackForHex(targetHex);
	bool targetStackOwned = targetStack && targetStack->unitOwner() == owner.curInt->playerID;

	switch (action.get())
	{
		case PossiblePlayerBattleAction::CHOOSE_TACTICS_STACK:
			return (targetStack && targetStackOwned && targetStack->getMovementRange() > 0);

		case PossiblePlayerBattleAction::CREATURE_INFO:
			return (targetStack && targetStackOwned && targetStack->alive());

		case PossiblePlayerBattleAction::HERO_INFO:
			if (targetHex == BattleHex::HERO_ATTACKER)
				return owner.attackingHero != nullptr;

			if (targetHex == BattleHex::HERO_DEFENDER)
				return owner.defendingHero != nullptr;

			return false;

		case PossiblePlayerBattleAction::MOVE_TACTICS:
		case PossiblePlayerBattleAction::MOVE_STACK:
			if (!(targetStack && targetStack->alive())) //we can walk on dead stacks
			{
				if(canStackMoveHere(owner.stacksController->getActiveStack(), targetHex))
					return true;
			}
			return false;

		case PossiblePlayerBattleAction::ATTACK:
		case PossiblePlayerBattleAction::WALK_AND_ATTACK:
		case PossiblePlayerBattleAction::ATTACK_AND_RETURN:
			if(owner.getBattle()->battleCanAttack(owner.stacksController->getActiveStack(), targetStack, targetHex))
			{
				if (owner.fieldController->isTileAttackable(targetHex)) // move isTileAttackable to be part of battleCanAttack?
					return true;
			}
			return false;

		case PossiblePlayerBattleAction::SHOOT:
			{
				auto currentStack = owner.stacksController->getActiveStack();
				if(!owner.getBattle()->battleCanShoot(currentStack, targetHex))
					return false;

				if(targetStack == nullptr && owner.getBattle()->battleCanTargetEmptyHex(currentStack))
				{
					auto spellLikeAttackBonus = currentStack->getBonus(Selector::type()(BonusType::SPELL_LIKE_ATTACK));
					const CSpell * spellDataToCheck = spellLikeAttackBonus->subtype.as<SpellID>().toSpell();
					return isCastingPossibleHere(spellDataToCheck, nullptr, targetHex);
				}

				return true;
			}

		case PossiblePlayerBattleAction::NO_LOCATION:
			return false;

		case PossiblePlayerBattleAction::ANY_LOCATION:
			return isCastingPossibleHere(action.spell().toSpell(), nullptr, targetHex);

		case PossiblePlayerBattleAction::AIMED_SPELL_CREATURE:
			return !selectedStack && targetStack && isCastingPossibleHere(action.spell().toSpell(), nullptr, targetHex);

		case PossiblePlayerBattleAction::RANDOM_GENIE_SPELL:
			if(targetStack && targetStackOwned && targetStack != owner.stacksController->getActiveStack() && targetStack->alive()) //only positive spells for other allied creatures
			{
				SpellID spellID = owner.getBattle()->getRandomBeneficialSpell(CRandomGenerator::getDefault(), owner.stacksController->getActiveStack(), targetStack);
				return spellID != SpellID::NONE;
			}
			return false;

		case PossiblePlayerBattleAction::TELEPORT:
			return selectedStack && isCastingPossibleHere(action.spell().toSpell(), selectedStack, targetHex);

		case PossiblePlayerBattleAction::SACRIFICE: //choose our living stack to sacrifice
			return targetStack && targetStack != selectedStack && targetStackOwned && targetStack->alive();

		case PossiblePlayerBattleAction::OBSTACLE:
		case PossiblePlayerBattleAction::FREE_LOCATION:
			return isCastingPossibleHere(action.spell().toSpell(), nullptr, targetHex);

		case PossiblePlayerBattleAction::CATAPULT:
			return owner.siegeController && owner.siegeController->isAttackableByCatapult(targetHex);

		case PossiblePlayerBattleAction::HEAL:
			return targetStack && targetStackOwned && targetStack->canBeHealed();
	}

	assert(0);
	return false;
}

void BattleActionsController::actionRealize(PossiblePlayerBattleAction action, const BattleHex & targetHex)
{
	const CStack * targetStack = getStackForHex(targetHex);

	switch (action.get()) //display console message, realize selected action
	{
		case PossiblePlayerBattleAction::CHOOSE_TACTICS_STACK:
		{
			owner.stackActivated(targetStack);
			return;
		}

		case PossiblePlayerBattleAction::MOVE_TACTICS:
		case PossiblePlayerBattleAction::MOVE_STACK:
		{
			if(owner.stacksController->getActiveStack()->doubleWide())
			{
				BattleHexArray acc = owner.getBattle()->battleGetAvailableHexes(owner.stacksController->getActiveStack(), false);
				BattleHex shiftedDest = targetHex.cloneInDirection(owner.stacksController->getActiveStack()->destShiftDir(), false);
				if(acc.contains(targetHex))
					owner.giveCommand(EActionType::WALK, targetHex);
				else if(acc.contains(shiftedDest))
					owner.giveCommand(EActionType::WALK, shiftedDest);
			}
			else
			{
				owner.giveCommand(EActionType::WALK, targetHex);
			}
			return;
		}

		case PossiblePlayerBattleAction::ATTACK:
		case PossiblePlayerBattleAction::WALK_AND_ATTACK:
		case PossiblePlayerBattleAction::ATTACK_AND_RETURN: //TODO: allow to disable return
		{
			bool returnAfterAttack = action.get() == PossiblePlayerBattleAction::ATTACK_AND_RETURN;
			BattleHex attackFromHex = owner.fieldController->fromWhichHexAttack(targetHex);
			if(attackFromHex.isValid()) //we can be in this line when unreachable creature is L - clicked (as of revision 1308)
			{
				BattleAction command = BattleAction::makeMeleeAttack(owner.stacksController->getActiveStack(), targetHex, attackFromHex, returnAfterAttack);
				owner.sendCommand(command, owner.stacksController->getActiveStack());
			}
			return;
		}

		case PossiblePlayerBattleAction::SHOOT:
		{
			owner.giveCommand(EActionType::SHOOT, targetHex);
			return;
		}

		case PossiblePlayerBattleAction::HEAL:
		{
			owner.giveCommand(EActionType::STACK_HEAL, targetHex);
			return;
		};

		case PossiblePlayerBattleAction::CATAPULT:
		{
			owner.giveCommand(EActionType::CATAPULT, targetHex);
			return;
		}

		case PossiblePlayerBattleAction::CREATURE_INFO:
		{
			ENGINE->windows().createAndPushWindow<CStackWindow>(targetStack, false);
			return;
		}

		case PossiblePlayerBattleAction::HERO_INFO:
		{
			if (targetHex == BattleHex::HERO_ATTACKER)
				owner.attackingHero->heroLeftClicked();

			if (targetHex == BattleHex::HERO_DEFENDER)
				owner.defendingHero->heroLeftClicked();

			return;
		}

		case PossiblePlayerBattleAction::AIMED_SPELL_CREATURE:
		case PossiblePlayerBattleAction::ANY_LOCATION:
		case PossiblePlayerBattleAction::RANDOM_GENIE_SPELL: //we assume that teleport / sacrifice will never be available as random spell
		case PossiblePlayerBattleAction::TELEPORT:
		case PossiblePlayerBattleAction::OBSTACLE:
		case PossiblePlayerBattleAction::SACRIFICE:
		case PossiblePlayerBattleAction::FREE_LOCATION:
		{
			if (action.get() == PossiblePlayerBattleAction::AIMED_SPELL_CREATURE )
			{
				if (action.spell() == SpellID::SACRIFICE)
				{
					heroSpellToCast->aimToHex(targetHex);
					possibleActions.push_back({PossiblePlayerBattleAction::SACRIFICE, action.spell()});
					selectedStack = targetStack;
					return;
				}
				if (action.spell() == SpellID::TELEPORT)
				{
					heroSpellToCast->aimToUnit(targetStack);
					possibleActions.push_back({PossiblePlayerBattleAction::TELEPORT, action.spell()});
					selectedStack = targetStack;
					return;
				}
			}

			if (!heroSpellcastingModeActive())
			{
				if (action.spell().hasValue())
				{
					owner.giveCommand(EActionType::MONSTER_SPELL, targetHex, action.spell());
				}
				else //unknown random spell
				{
					owner.giveCommand(EActionType::MONSTER_SPELL, targetHex);
				}
			}
			else
			{
				assert(getHeroSpellToCast());
				switch (getHeroSpellToCast()->id.toEnum())
				{
					case SpellID::SACRIFICE:
						heroSpellToCast->aimToUnit(targetStack);//victim
						break;
					default:
						heroSpellToCast->aimToHex(targetHex);
						break;
				}
				owner.curInt->cb->battleMakeSpellAction(owner.getBattleID(), *heroSpellToCast);
				endCastingSpell();
			}
			selectedStack = nullptr;
			return;
		}
	}
	assert(0);
	return;
}

PossiblePlayerBattleAction BattleActionsController::selectAction(const BattleHex & targetHex)
{
	assert(owner.stacksController->getActiveStack() != nullptr);
	assert(!possibleActions.empty());
	assert(targetHex.isValid());

	if (owner.stacksController->getActiveStack() == nullptr)
		return PossiblePlayerBattleAction::INVALID;

	if (possibleActions.empty())
		return PossiblePlayerBattleAction::INVALID;

	const CStack * targetStack = getStackForHex(targetHex);

	reorderPossibleActionsPriority(owner.stacksController->getActiveStack(), targetStack);

	for (PossiblePlayerBattleAction action : possibleActions)
	{
		if (actionIsLegal(action, targetHex))
			return action;
	}
	return possibleActions.front();
}

void BattleActionsController::onHexHovered(const BattleHex & hoveredHex)
{
	if (owner.openingPlaying())
	{
		currentConsoleMsg = LIBRARY->generaltexth->translate("vcmi.battleWindow.pressKeyToSkipIntro");
		ENGINE->statusbar()->write(currentConsoleMsg);
		return;
	}

	if (owner.stacksController->getActiveStack() == nullptr)
		return;

	if (hoveredHex == BattleHex::INVALID)
	{
		if (!currentConsoleMsg.empty())
			ENGINE->statusbar()->clearIfMatching(currentConsoleMsg);

		currentConsoleMsg.clear();
		ENGINE->cursor().set(Cursor::Combat::BLOCKED);
		return;
	}

	auto action = selectAction(hoveredHex);

	std::string newConsoleMsg;

	if (actionIsLegal(action, hoveredHex))
	{
		actionSetCursor(action, hoveredHex);
		newConsoleMsg = actionGetStatusMessage(action, hoveredHex);
	}
	else
	{
		actionSetCursorBlocked(action, hoveredHex);
		newConsoleMsg = actionGetStatusMessageBlocked(action, hoveredHex);
	}

	if (!currentConsoleMsg.empty())
		ENGINE->statusbar()->clearIfMatching(currentConsoleMsg);

	if (!newConsoleMsg.empty())
		ENGINE->statusbar()->write(newConsoleMsg);

	currentConsoleMsg = newConsoleMsg;
}

void BattleActionsController::onHoverEnded()
{
	ENGINE->cursor().set(Cursor::Combat::POINTER);

	if (!currentConsoleMsg.empty())
		ENGINE->statusbar()->clearIfMatching(currentConsoleMsg);

	currentConsoleMsg.clear();
}

void BattleActionsController::onHexLeftClicked(const BattleHex & clickedHex)
{
	if (owner.stacksController->getActiveStack() == nullptr)
		return;

	auto action = selectAction(clickedHex);

	std::string newConsoleMsg;

	if (!actionIsLegal(action, clickedHex))
		return;
	
	actionRealize(action, clickedHex);
	ENGINE->statusbar()->clear();
}

void BattleActionsController::tryActivateStackSpellcasting(const CStack *casterStack)
{
	creatureSpells.clear();

	bool spellcaster = casterStack->hasBonusOfType(BonusType::SPELLCASTER);
	if(casterStack->canCast() && spellcaster)
	{
		// faerie dragon can cast only one, randomly selected spell until their next move
		//TODO: faerie dragon type spell should be selected by server
		const auto spellToCast = owner.getBattle()->getRandomCastedSpell(CRandomGenerator::getDefault(), casterStack);

		if (spellToCast.hasValue())
			creatureSpells.push_back(spellToCast.toSpell());
	}

	TConstBonusListPtr bl = casterStack->getBonusesOfType(BonusType::SPELLCASTER);

	for(const auto & bonus : *bl)
	{
		if (bonus->additionalInfo[0] <= 0 && bonus->subtype.as<SpellID>().hasValue())
			creatureSpells.push_back(bonus->subtype.as<SpellID>().toSpell());
	}
}

const spells::Caster * BattleActionsController::getCurrentSpellcaster() const
{
	if (heroSpellToCast)
		return owner.currentHero();
	else
		return owner.stacksController->getActiveStack();
}

spells::Mode BattleActionsController::getCurrentCastMode() const
{
	if (heroSpellToCast)
		return spells::Mode::HERO;
	else
		return spells::Mode::CREATURE_ACTIVE;

}

bool BattleActionsController::isCastingPossibleHere(const CSpell * currentSpell, const CStack *targetStack, const BattleHex & targetHex)
{
	assert(currentSpell);
	if (!currentSpell)
		return false;

	auto caster = getCurrentSpellcaster();

	const spells::Mode mode = heroSpellToCast ? spells::Mode::HERO : spells::Mode::CREATURE_ACTIVE;

	spells::Target target;
	if(targetStack)
		target.emplace_back(targetStack);
	target.emplace_back(targetHex);

	spells::BattleCast cast(owner.getBattle().get(), caster, mode, currentSpell);

	auto m = currentSpell->battleMechanics(&cast);
	spells::detail::ProblemImpl problem; //todo: display problem in status bar

	return m->canBeCastAt(target, problem);
}

bool BattleActionsController::canStackMoveHere(const CStack * stackToMove, const BattleHex & myNumber) const
{
	BattleHexArray acc = owner.getBattle()->battleGetAvailableHexes(stackToMove, false);
	BattleHex shiftedDest = myNumber.cloneInDirection(stackToMove->destShiftDir(), false);

	if (acc.contains(myNumber))
		return true;
	else if (stackToMove->doubleWide() && acc.contains(shiftedDest))
		return true;
	else
		return false;
}

void BattleActionsController::activateStack()
{
	const CStack * s = owner.stacksController->getActiveStack();
	if(s)
	{
		tryActivateStackSpellcasting(s);

		possibleActions = getPossibleActionsForStack(s);
		std::list<PossiblePlayerBattleAction> actionsToSelect;
		if(!possibleActions.empty())
		{
			auto primaryAction = possibleActions.front().get();

			if(primaryAction == PossiblePlayerBattleAction::SHOOT || primaryAction == PossiblePlayerBattleAction::AIMED_SPELL_CREATURE
				|| primaryAction == PossiblePlayerBattleAction::ANY_LOCATION || primaryAction == PossiblePlayerBattleAction::ATTACK_AND_RETURN)
			{
				actionsToSelect.push_back(possibleActions.front());

				auto shootActionPredicate = [](const PossiblePlayerBattleAction& action)
				{
					return action.get() == PossiblePlayerBattleAction::SHOOT;
				};
				bool hasShootSecondaryAction = std::any_of(possibleActions.begin() + 1, possibleActions.end(), shootActionPredicate);

				if(hasShootSecondaryAction) //casters may have shooting capabilities, for example storm elementals
					actionsToSelect.emplace_back(PossiblePlayerBattleAction::SHOOT);

				/* TODO: maybe it would also make sense to check spellcast as non-top priority action ("NO_SPELLCAST_BY_DEFAULT" bonus)?
				 * it would require going beyond this "if" block for melee casters
				 * F button helps, but some mod creatures may have that bonus and more than 1 castable spell */

				actionsToSelect.emplace_back(PossiblePlayerBattleAction::ATTACK); //always allow melee attack as last option
			}
		}
		owner.windowObject->setAlternativeActions(actionsToSelect);
	}
}

void BattleActionsController::onHexRightClicked(const BattleHex & clickedHex)
{
	bool isCurrentStackInSpellcastMode = creatureSpellcastingModeActive();

	if (heroSpellcastingModeActive() || isCurrentStackInSpellcastMode)
	{
		endCastingSpell();
		CRClickPopup::createAndPush(LIBRARY->generaltexth->translate("core.genrltxt.731")); // spell cancelled
		return;
	}

	auto selectedStack = owner.getBattle()->battleGetStackByPos(clickedHex, true);

	if (selectedStack != nullptr)
		ENGINE->windows().createAndPushWindow<CStackWindow>(selectedStack, true);

	if (clickedHex == BattleHex::HERO_ATTACKER && owner.attackingHero)
		owner.attackingHero->heroRightClicked();

	if (clickedHex == BattleHex::HERO_DEFENDER && owner.defendingHero)
		owner.defendingHero->heroRightClicked();
}

bool BattleActionsController::heroSpellcastingModeActive() const
{
	return heroSpellToCast != nullptr;
}

bool BattleActionsController::creatureSpellcastingModeActive() const
{
	auto spellcastModePredicate = [](const PossiblePlayerBattleAction & action)
	{
		return action.spellcast() || action.get() == PossiblePlayerBattleAction::SHOOT; //for hotkey-eligible SPELL_LIKE_ATTACK creature should have only SHOOT action
	};

	return !possibleActions.empty() && std::all_of(possibleActions.begin(), possibleActions.end(), spellcastModePredicate);
}

bool BattleActionsController::currentActionSpellcasting(const BattleHex & hoveredHex)
{
	if (heroSpellToCast)
		return true;

	if (!owner.stacksController->getActiveStack())
		return false;

	auto action = selectAction(hoveredHex);

	return action.spellcast();
}

const std::vector<PossiblePlayerBattleAction> & BattleActionsController::getPossibleActions() const
{
	return possibleActions;
}

void BattleActionsController::removePossibleAction(PossiblePlayerBattleAction action)
{
	vstd::erase(possibleActions, action);
}

void BattleActionsController::pushFrontPossibleAction(PossiblePlayerBattleAction action)
{
	possibleActions.insert(possibleActions.begin(), action);
}

void BattleActionsController::resetCurrentStackPossibleActions()
{
	possibleActions = getPossibleActionsForStack(owner.stacksController->getActiveStack());
}
