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

#include "../CGameInfo.h"
#include "../CPlayerInterface.h"
#include "../gui/CursorHandler.h"
#include "../gui/CGuiHandler.h"
#include "../gui/CIntObject.h"
#include "../windows/CCreatureWindow.h"

#include "../../CCallback.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/CGeneralTextHandler.h"
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
		return CGI->generaltexth->translate(baseTextID + ".1");
	return CGI->generaltexth->translate(baseTextID);
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

	return replacePlaceholders(CGI->generaltexth->translate(baseTextID), replacements);
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

BattleActionsController::BattleActionsController(BattleInterface & owner):
	owner(owner),
	selectedStack(nullptr),
	heroSpellToCast(nullptr)
{}

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
	GH.fakeMouseMove();
}

bool BattleActionsController::isActiveStackSpellcaster() const
{
	const CStack * casterStack = owner.stacksController->getActiveStack();
	if (!casterStack)
		return false;

	bool spellcaster = casterStack->hasBonusOfType(Bonus::SPELLCASTER);
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

	if (!isActiveStackSpellcaster())
		return;

	for (auto const & action : possibleActions)
	{
		if (action.get() != PossiblePlayerBattleAction::NO_LOCATION)
			continue;

		const spells::Caster * caster = owner.stacksController->getActiveStack();
		const CSpell * spell = action.spell().toSpell();

		spells::Target target;
		target.emplace_back();

		spells::BattleCast cast(owner.curInt->cb.get(), caster, spells::Mode::CREATURE_ACTIVE, spell);

		auto m = spell->battleMechanics(&cast);
		spells::detail::ProblemImpl ignored;

		const bool isCastingPossible = m->canBeCastAt(target, ignored);

		if (isCastingPossible)
		{
			owner.giveCommand(EActionType::MONSTER_SPELL, BattleHex::INVALID, spell->getId());
			selectedStack = nullptr;

			CCS->curh->set(Cursor::Combat::POINTER);
		}
		return;
	}

	possibleActions = getPossibleActionsForStack(owner.stacksController->getActiveStack());

	auto actionFilterPredicate = [](const PossiblePlayerBattleAction x)
	{
		return !x.spellcast();
	};

	vstd::erase_if(possibleActions, actionFilterPredicate);
	GH.fakeMouseMove();
}

std::vector<PossiblePlayerBattleAction> BattleActionsController::getPossibleActionsForStack(const CStack *stack) const
{
	BattleClientInterfaceData data; //hard to get rid of these things so for now they're required data to pass

	for (auto const & spell : creatureSpells)
		data.creatureSpellsToCast.push_back(spell->id);

	data.tacticsMode = owner.tacticsMode;
	auto allActions = owner.curInt->cb->getClientActionsForStack(stack, data);

	allActions.push_back(PossiblePlayerBattleAction::HERO_INFO);
	allActions.push_back(PossiblePlayerBattleAction::CREATURE_INFO);

	return std::vector<PossiblePlayerBattleAction>(allActions);
}

void BattleActionsController::reorderPossibleActionsPriority(const CStack * stack, MouseHoveredHexContext context)
{
	if(owner.tacticsMode || possibleActions.empty()) return; //this function is not supposed to be called in tactics mode or before getPossibleActionsForStack

	auto assignPriority = [&](PossiblePlayerBattleAction const & item) -> uint8_t //large lambda assigning priority which would have to be part of possibleActions without it
	{
		switch(item.get())
		{
		case PossiblePlayerBattleAction::AIMED_SPELL_CREATURE:
		case PossiblePlayerBattleAction::ANY_LOCATION:
		case PossiblePlayerBattleAction::NO_LOCATION:
		case PossiblePlayerBattleAction::FREE_LOCATION:
		case PossiblePlayerBattleAction::OBSTACLE:
			if(!stack->hasBonusOfType(Bonus::NO_SPELLCAST_BY_DEFAULT) && context == MouseHoveredHexContext::OCCUPIED_HEX)
				return 1;
			else
				return 100;//bottom priority
			break;
		case PossiblePlayerBattleAction::RANDOM_GENIE_SPELL:
			return 2; break;
		case PossiblePlayerBattleAction::SHOOT:
			return 4; break;
		case PossiblePlayerBattleAction::ATTACK_AND_RETURN:
			return 5; break;
		case PossiblePlayerBattleAction::ATTACK:
			return 6; break;
		case PossiblePlayerBattleAction::WALK_AND_ATTACK:
			return 7; break;
		case PossiblePlayerBattleAction::MOVE_STACK:
			return 8; break;
		case PossiblePlayerBattleAction::CATAPULT:
			return 9; break;
		case PossiblePlayerBattleAction::HEAL:
			return 10; break;
		case PossiblePlayerBattleAction::CREATURE_INFO:
			return 11; break;
		case PossiblePlayerBattleAction::HERO_INFO:
			return 12; break;
		case PossiblePlayerBattleAction::TELEPORT:
			return 13; break;
		default:
			assert(0);
			return 200; break;
		}
	};

	auto comparer = [&](PossiblePlayerBattleAction const & lhs, PossiblePlayerBattleAction const & rhs)
	{
		return assignPriority(lhs) < assignPriority(rhs);
	};

	std::sort(possibleActions.begin(), possibleActions.end(), comparer);
}

void BattleActionsController::castThisSpell(SpellID spellID)
{
	heroSpellToCast = std::make_shared<BattleAction>();
	heroSpellToCast->actionType = EActionType::HERO_SPELL;
	heroSpellToCast->actionSubtype = spellID; //spell number
	heroSpellToCast->stackNumber = (owner.attackingHeroInstance->tempOwner == owner.curInt->playerID) ? -1 : -2;
	heroSpellToCast->side = owner.defendingHeroInstance ? (owner.curInt->playerID == owner.defendingHeroInstance->tempOwner) : false;

	//choosing possible targets
	const CGHeroInstance *castingHero = (owner.attackingHeroInstance->tempOwner == owner.curInt->playerID) ? owner.attackingHeroInstance : owner.defendingHeroInstance;
	assert(castingHero); // code below assumes non-null hero
	PossiblePlayerBattleAction spellSelMode = owner.curInt->cb->getCasterAction(spellID.toSpell(), castingHero, spells::Mode::HERO);

	if (spellSelMode.get() == PossiblePlayerBattleAction::NO_LOCATION) //user does not have to select location
	{
		heroSpellToCast->aimToHex(BattleHex::INVALID);
		owner.curInt->cb->battleMakeAction(heroSpellToCast.get());
		endCastingSpell();
	}
	else
	{
		possibleActions.clear();
		possibleActions.push_back (spellSelMode); //only this one action can be performed at the moment
		GH.fakeMouseMove();//update cursor
	}

	owner.windowObject->blockUI(true);
}

const CSpell * BattleActionsController::getHeroSpellToCast( ) const
{
	if (heroSpellToCast)
		return SpellID(heroSpellToCast->actionSubtype).toSpell();
	return nullptr;
}

const CSpell * BattleActionsController::getStackSpellToCast(BattleHex hoveredHex)
{
	if (heroSpellToCast)
		return nullptr;

	if (!owner.stacksController->getActiveStack())
		return nullptr;

	if (!hoveredHex.isValid())
		return nullptr;

	auto action = selectAction(hoveredHex);

	if (action.spell() == SpellID::NONE)
		return nullptr;

	return action.spell().toSpell();
}

const CSpell * BattleActionsController::getCurrentSpell(BattleHex hoveredHex)
{
	if (getHeroSpellToCast())
		return getHeroSpellToCast();
	return getStackSpellToCast(hoveredHex);
}

const CStack * BattleActionsController::getStackForHex(BattleHex hoveredHex)
{
	const CStack * shere = owner.curInt->cb->battleGetStackByPos(hoveredHex, true);
	if(shere)
		return shere;
	return owner.curInt->cb->battleGetStackByPos(hoveredHex, false);
}

void BattleActionsController::actionSetCursor(PossiblePlayerBattleAction action, BattleHex targetHex)
{
	switch (action.get())
	{
		case PossiblePlayerBattleAction::CHOOSE_TACTICS_STACK:
			CCS->curh->set(Cursor::Combat::POINTER);
			return;

		case PossiblePlayerBattleAction::MOVE_TACTICS:
		case PossiblePlayerBattleAction::MOVE_STACK:
			if (owner.stacksController->getActiveStack()->hasBonusOfType(Bonus::FLYING))
				CCS->curh->set(Cursor::Combat::FLY);
			else
				CCS->curh->set(Cursor::Combat::MOVE);
			return;

		case PossiblePlayerBattleAction::ATTACK:
		case PossiblePlayerBattleAction::WALK_AND_ATTACK:
		case PossiblePlayerBattleAction::ATTACK_AND_RETURN:
			owner.fieldController->setBattleCursor(targetHex);
			return;

		case PossiblePlayerBattleAction::SHOOT:
			if (owner.curInt->cb->battleHasShootingPenalty(owner.stacksController->getActiveStack(), targetHex))
				CCS->curh->set(Cursor::Combat::SHOOT_PENALTY);
			else
				CCS->curh->set(Cursor::Combat::SHOOT);
			return;

		case PossiblePlayerBattleAction::AIMED_SPELL_CREATURE:
		case PossiblePlayerBattleAction::ANY_LOCATION:
		case PossiblePlayerBattleAction::RANDOM_GENIE_SPELL:
		case PossiblePlayerBattleAction::FREE_LOCATION:
		case PossiblePlayerBattleAction::OBSTACLE:
			CCS->curh->set(Cursor::Spellcast::SPELL);
			return;

		case PossiblePlayerBattleAction::TELEPORT:
			CCS->curh->set(Cursor::Combat::TELEPORT);
			return;

		case PossiblePlayerBattleAction::SACRIFICE:
			CCS->curh->set(Cursor::Combat::SACRIFICE);
			return;

		case PossiblePlayerBattleAction::HEAL:
			CCS->curh->set(Cursor::Combat::HEAL);
			return;

		case PossiblePlayerBattleAction::CATAPULT:
			CCS->curh->set(Cursor::Combat::SHOOT_CATAPULT);
			return;

		case PossiblePlayerBattleAction::CREATURE_INFO:
			CCS->curh->set(Cursor::Combat::QUERY);
			return;
		case PossiblePlayerBattleAction::HERO_INFO:
			CCS->curh->set(Cursor::Combat::HERO);
			return;
	}
	assert(0);
}

void BattleActionsController::actionSetCursorBlocked(PossiblePlayerBattleAction action, BattleHex targetHex)
{
	switch (action.get())
	{
		case PossiblePlayerBattleAction::AIMED_SPELL_CREATURE:
		case PossiblePlayerBattleAction::RANDOM_GENIE_SPELL:
		case PossiblePlayerBattleAction::TELEPORT:
		case PossiblePlayerBattleAction::SACRIFICE:
		case PossiblePlayerBattleAction::FREE_LOCATION:
			CCS->curh->set(Cursor::Combat::BLOCKED);
			return;
		default:
			if (targetHex == -1)
				CCS->curh->set(Cursor::Combat::POINTER);
			else
				CCS->curh->set(Cursor::Combat::BLOCKED);
			return;
	}
	assert(0);
}

std::string BattleActionsController::actionGetStatusMessage(PossiblePlayerBattleAction action, BattleHex targetHex)
{
	const CStack * targetStack = getStackForHex(targetHex);

	switch (action.get()) //display console message, realize selected action
	{
		case PossiblePlayerBattleAction::CHOOSE_TACTICS_STACK:
			return (boost::format(CGI->generaltexth->allTexts[481]) % targetStack->getName()).str(); //Select %s

		case PossiblePlayerBattleAction::MOVE_TACTICS:
		case PossiblePlayerBattleAction::MOVE_STACK:
			if (owner.stacksController->getActiveStack()->hasBonusOfType(Bonus::FLYING))
				return (boost::format(CGI->generaltexth->allTexts[295]) % owner.stacksController->getActiveStack()->getName()).str(); //Fly %s here
			else
				return (boost::format(CGI->generaltexth->allTexts[294]) % owner.stacksController->getActiveStack()->getName()).str(); //Move %s here

		case PossiblePlayerBattleAction::ATTACK:
		case PossiblePlayerBattleAction::WALK_AND_ATTACK:
		case PossiblePlayerBattleAction::ATTACK_AND_RETURN: //TODO: allow to disable return
			{
				BattleHex attackFromHex = owner.fieldController->fromWhichHexAttack(targetHex);
				DamageEstimation estimation = owner.curInt->cb->battleEstimateDamage(owner.stacksController->getActiveStack(), targetStack, attackFromHex);
				estimation.kills.max = std::min<int64_t>(estimation.kills.max, targetStack->getCount());
				estimation.kills.min = std::min<int64_t>(estimation.kills.min, targetStack->getCount());

				return formatMeleeAttack(estimation, targetStack->getName());
			}

		case PossiblePlayerBattleAction::SHOOT:
		{
			const auto * shooter = owner.stacksController->getActiveStack();

			DamageEstimation estimation = owner.curInt->cb->battleEstimateDamage(shooter, targetStack, shooter->getPosition());
			estimation.kills.max = std::min<int64_t>(estimation.kills.max, targetStack->getCount());
			estimation.kills.min = std::min<int64_t>(estimation.kills.min, targetStack->getCount());

			return formatRangedAttack(estimation, targetStack->getName(), shooter->shots.available());
		}

		case PossiblePlayerBattleAction::AIMED_SPELL_CREATURE:
			return boost::str(boost::format(CGI->generaltexth->allTexts[27]) % action.spell().toSpell()->getNameTranslated() % targetStack->getName()); //Cast %s on %s

		case PossiblePlayerBattleAction::ANY_LOCATION:
			return boost::str(boost::format(CGI->generaltexth->allTexts[26]) % action.spell().toSpell()->getNameTranslated()); //Cast %s

		case PossiblePlayerBattleAction::RANDOM_GENIE_SPELL: //we assume that teleport / sacrifice will never be available as random spell
			return boost::str(boost::format(CGI->generaltexth->allTexts[301]) % targetStack->getName()); //Cast a spell on %

		case PossiblePlayerBattleAction::TELEPORT:
			return CGI->generaltexth->allTexts[25]; //Teleport Here

		case PossiblePlayerBattleAction::OBSTACLE:
			return CGI->generaltexth->allTexts[550];

		case PossiblePlayerBattleAction::SACRIFICE:
			return (boost::format(CGI->generaltexth->allTexts[549]) % targetStack->getName()).str(); //sacrifice the %s

		case PossiblePlayerBattleAction::FREE_LOCATION:
			return boost::str(boost::format(CGI->generaltexth->allTexts[26]) % action.spell().toSpell()->getNameTranslated()); //Cast %s

		case PossiblePlayerBattleAction::HEAL:
			return (boost::format(CGI->generaltexth->allTexts[419]) % targetStack->getName()).str(); //Apply first aid to the %s

		case PossiblePlayerBattleAction::CATAPULT:
			return ""; // TODO

		case PossiblePlayerBattleAction::CREATURE_INFO:
			return (boost::format(CGI->generaltexth->allTexts[297]) % targetStack->getName()).str();

		case PossiblePlayerBattleAction::HERO_INFO:
			return  CGI->generaltexth->translate("core.genrltxt.417"); // "View Hero Stats"
	}
	assert(0);
	return "";
}

std::string BattleActionsController::actionGetStatusMessageBlocked(PossiblePlayerBattleAction action, BattleHex targetHex)
{
	switch (action.get())
	{
		case PossiblePlayerBattleAction::AIMED_SPELL_CREATURE:
		case PossiblePlayerBattleAction::RANDOM_GENIE_SPELL:
			return CGI->generaltexth->allTexts[23];
			break;
		case PossiblePlayerBattleAction::TELEPORT:
			return CGI->generaltexth->allTexts[24]; //Invalid Teleport Destination
			break;
		case PossiblePlayerBattleAction::SACRIFICE:
			return CGI->generaltexth->allTexts[543]; //choose army to sacrifice
			break;
		case PossiblePlayerBattleAction::FREE_LOCATION:
			return boost::str(boost::format(CGI->generaltexth->allTexts[181]) % action.spell().toSpell()->getNameTranslated()); //No room to place %s here
			break;
		default:
			return "";
	}
}

bool BattleActionsController::actionIsLegal(PossiblePlayerBattleAction action, BattleHex targetHex)
{
	const CStack * targetStack = getStackForHex(targetHex);
	bool targetStackOwned = targetStack && targetStack->owner == owner.curInt->playerID;

	switch (action.get())
	{
		case PossiblePlayerBattleAction::CHOOSE_TACTICS_STACK:
			return (targetStack && targetStackOwned && targetStack->Speed() > 0);

		case PossiblePlayerBattleAction::CREATURE_INFO:
			return (targetStack && targetStackOwned);

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
			if(owner.curInt->cb->battleCanAttack(owner.stacksController->getActiveStack(), targetStack, targetHex))
			{
				if (owner.fieldController->isTileAttackable(targetHex)) // move isTileAttackable to be part of battleCanAttack?
					return true;
			}
			return false;

		case PossiblePlayerBattleAction::SHOOT:
			return owner.curInt->cb->battleCanShoot(owner.stacksController->getActiveStack(), targetHex);

		case PossiblePlayerBattleAction::NO_LOCATION:
			return false;

		case PossiblePlayerBattleAction::ANY_LOCATION:
			return isCastingPossibleHere(action.spell().toSpell(), owner.stacksController->getActiveStack(), targetStack, targetHex);

		case PossiblePlayerBattleAction::AIMED_SPELL_CREATURE:
			return !selectedStack && targetStack && isCastingPossibleHere(action.spell().toSpell(), owner.stacksController->getActiveStack(), targetStack, targetHex);

		case PossiblePlayerBattleAction::RANDOM_GENIE_SPELL:
			if(targetStack && targetStackOwned && targetStack != owner.stacksController->getActiveStack() && targetStack->alive()) //only positive spells for other allied creatures
			{
				int spellID = owner.curInt->cb->battleGetRandomStackSpell(CRandomGenerator::getDefault(), targetStack, CBattleInfoCallback::RANDOM_GENIE);
				return spellID > -1;
			}
			return false;

		case PossiblePlayerBattleAction::TELEPORT:
		{
			ui8 skill = getCurrentSpellcaster()->getEffectLevel(SpellID(SpellID::TELEPORT).toSpell());
			return owner.curInt->cb->battleCanTeleportTo(selectedStack, targetHex, skill);
		}

		case PossiblePlayerBattleAction::SACRIFICE: //choose our living stack to sacrifice
			return targetStack && targetStack != selectedStack && targetStackOwned && targetStack->alive();

		case PossiblePlayerBattleAction::OBSTACLE:
		case PossiblePlayerBattleAction::FREE_LOCATION:
			return isCastingPossibleHere(action.spell().toSpell(), owner.stacksController->getActiveStack(), targetStack, targetHex);
			return isCastingPossibleHere(action.spell().toSpell(), owner.stacksController->getActiveStack(), targetStack, targetHex);

		case PossiblePlayerBattleAction::CATAPULT:
			return owner.siegeController && owner.siegeController->isAttackableByCatapult(targetHex);

		case PossiblePlayerBattleAction::HEAL:
			return targetStack && targetStackOwned && targetStack->canBeHealed();
	}

	assert(0);
	return false;
}

void BattleActionsController::actionRealize(PossiblePlayerBattleAction action, BattleHex targetHex)
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
				std::vector<BattleHex> acc = owner.curInt->cb->battleGetAvailableHexes(owner.stacksController->getActiveStack());
				BattleHex shiftedDest = targetHex.cloneInDirection(owner.stacksController->getActiveStack()->destShiftDir(), false);
				if(vstd::contains(acc, targetHex))
					owner.giveCommand(EActionType::WALK, targetHex);
				else if(vstd::contains(acc, shiftedDest))
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
				auto command = new BattleAction(BattleAction::makeMeleeAttack(owner.stacksController->getActiveStack(), targetHex, attackFromHex, returnAfterAttack));
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
			GH.pushIntT<CStackWindow>(targetStack, false);
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

			if (!spellcastingModeActive())
			{
				if (action.spell().toSpell())
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
				owner.curInt->cb->battleMakeAction(heroSpellToCast.get());
				endCastingSpell();
			}
			selectedStack = nullptr;
			return;
		}
	}
	assert(0);
	return;
}

PossiblePlayerBattleAction BattleActionsController::selectAction(BattleHex targetHex)
{
	assert(owner.stacksController->getActiveStack() != nullptr);
	assert(!possibleActions.empty());
	assert(targetHex.isValid());

	if (owner.stacksController->getActiveStack() == nullptr)
		return PossiblePlayerBattleAction::INVALID;

	if (possibleActions.empty())
		return PossiblePlayerBattleAction::INVALID;

	const CStack * targetStack = getStackForHex(targetHex);

	reorderPossibleActionsPriority(owner.stacksController->getActiveStack(), targetStack ? MouseHoveredHexContext::OCCUPIED_HEX : MouseHoveredHexContext::UNOCCUPIED_HEX);

	for (PossiblePlayerBattleAction action : possibleActions)
	{
		if (actionIsLegal(action, targetHex))
			return action;
	}
	return possibleActions.front();
}

void BattleActionsController::onHexHovered(BattleHex hoveredHex)
{
	if (owner.openingPlaying())
	{
		currentConsoleMsg = VLC->generaltexth->translate("vcmi.battleWindow.pressKeyToSkipIntro");
		GH.statusbar->write(currentConsoleMsg);
		return;
	}

	if (owner.stacksController->getActiveStack() == nullptr)
		return;

	if (hoveredHex == BattleHex::INVALID)
	{
		if (!currentConsoleMsg.empty())
			GH.statusbar->clearIfMatching(currentConsoleMsg);

		currentConsoleMsg.clear();
		CCS->curh->set(Cursor::Combat::BLOCKED);
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
		GH.statusbar->clearIfMatching(currentConsoleMsg);

	if (!newConsoleMsg.empty())
		GH.statusbar->write(newConsoleMsg);

	currentConsoleMsg = newConsoleMsg;
}

void BattleActionsController::onHoverEnded()
{
	CCS->curh->set(Cursor::Combat::POINTER);

	if (!currentConsoleMsg.empty())
		GH.statusbar->clearIfMatching(currentConsoleMsg);

	currentConsoleMsg.clear();
}

void BattleActionsController::onHexLeftClicked(BattleHex clickedHex)
{
	if (owner.stacksController->getActiveStack() == nullptr)
		return;

	auto action = selectAction(clickedHex);

	std::string newConsoleMsg;

	if (!actionIsLegal(action, clickedHex))
		return;

	actionRealize(action, clickedHex);

	GH.statusbar->clear();
}

void BattleActionsController::tryActivateStackSpellcasting(const CStack *casterStack)
{
	creatureSpells.clear();

	bool spellcaster = casterStack->hasBonusOfType(Bonus::SPELLCASTER);
	if(casterStack->canCast() && spellcaster)
	{
		// faerie dragon can cast only one, randomly selected spell until their next move
		//TODO: faerie dragon type spell should be selected by server
		const auto * spellToCast = owner.curInt->cb->battleGetRandomStackSpell(CRandomGenerator::getDefault(), casterStack, CBattleInfoCallback::RANDOM_AIMED).toSpell();

		if (spellToCast)
			creatureSpells.push_back(spellToCast);
	}

	TConstBonusListPtr bl = casterStack->getBonuses(Selector::type()(Bonus::SPELLCASTER));

	for (auto const & bonus : *bl)
	{
		if (bonus->additionalInfo[0] <= 0)
			creatureSpells.push_back(SpellID(bonus->subtype).toSpell());
	}
}

const spells::Caster * BattleActionsController::getCurrentSpellcaster() const
{
	if (heroSpellToCast)
		return owner.getActiveHero();
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

bool BattleActionsController::isCastingPossibleHere(const CSpell * currentSpell, const CStack *casterStack, const CStack *targetStack, BattleHex targetHex)
{
	assert(currentSpell);
	if (!currentSpell)
		return false;

	auto caster = getCurrentSpellcaster();

	const spells::Mode mode = heroSpellToCast ? spells::Mode::HERO : spells::Mode::CREATURE_ACTIVE;

	spells::Target target;
	target.emplace_back(targetHex);

	spells::BattleCast cast(owner.curInt->cb.get(), caster, mode, currentSpell);

	auto m = currentSpell->battleMechanics(&cast);
	spells::detail::ProblemImpl problem; //todo: display problem in status bar

	return m->canBeCastAt(target, problem);
}

bool BattleActionsController::canStackMoveHere(const CStack * stackToMove, BattleHex myNumber) const
{
	std::vector<BattleHex> acc = owner.curInt->cb->battleGetAvailableHexes(stackToMove);
	BattleHex shiftedDest = myNumber.cloneInDirection(stackToMove->destShiftDir(), false);

	if (vstd::contains(acc, myNumber))
		return true;
	else if (stackToMove->doubleWide() && vstd::contains(acc, shiftedDest))
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
			switch(possibleActions.front().get())
			{
				case PossiblePlayerBattleAction::SHOOT:
					actionsToSelect.push_back(possibleActions.front());
					actionsToSelect.push_back(PossiblePlayerBattleAction::ATTACK);
					break;
					
				case PossiblePlayerBattleAction::ATTACK_AND_RETURN:
					actionsToSelect.push_back(possibleActions.front());
					actionsToSelect.push_back(PossiblePlayerBattleAction::WALK_AND_ATTACK);
					break;
					
				case PossiblePlayerBattleAction::AIMED_SPELL_CREATURE:
					actionsToSelect.push_back(possibleActions.front());
					break;
			}
		}
		owner.windowObject->setAlternativeActions(actionsToSelect);
	}
}

void BattleActionsController::onHexRightClicked(BattleHex clickedHex)
{
	auto selectedStack = owner.curInt->cb->battleGetStackByPos(clickedHex, true);

	if (selectedStack != nullptr)
		GH.pushIntT<CStackWindow>(selectedStack, true);

	if (clickedHex == BattleHex::HERO_ATTACKER && owner.attackingHero)
		owner.attackingHero->heroRightClicked();

	if (clickedHex == BattleHex::HERO_DEFENDER && owner.defendingHero)
		owner.defendingHero->heroRightClicked();
}

bool BattleActionsController::spellcastingModeActive() const
{
	return heroSpellToCast != nullptr;;
}

bool BattleActionsController::currentActionSpellcasting(BattleHex hoveredHex)
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
