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
#include "../../lib/CStack.h"
#include "../../lib/battle/BattleAction.h"
#include "../../lib/spells/CSpellHandler.h"
#include "../../lib/spells/ISpellMechanics.h"
#include "../../lib/spells/Problem.h"
#include "../../lib/CGeneralTextHandler.h"

static std::string formatDmgRange(std::pair<ui32, ui32> dmgRange)
{
	if (dmgRange.first != dmgRange.second)
		return (boost::format("%d - %d") % dmgRange.first % dmgRange.second).str();
	else
		return (boost::format("%d") % dmgRange.first).str();
}

BattleActionsController::BattleActionsController(BattleInterface & owner):
	owner(owner),
	heroSpellToCast(nullptr),
	creatureSpellToCast(nullptr)
{}

void BattleActionsController::endCastingSpell()
{
	if(heroSpellToCast)
		heroSpellToCast.reset();

	if(owner.stacksController->getActiveStack())
		possibleActions = getPossibleActionsForStack(owner.stacksController->getActiveStack()); //restore actions after they were cleared

	GH.fakeMouseMove();
}

bool BattleActionsController::isActiveStackSpellcaster() const
{
	const CStack * casterStack = owner.stacksController->getActiveStack();
	if (!casterStack)
		return false;

	const auto randomSpellcaster = casterStack->getBonusLocalFirst(Selector::type()(Bonus::SPELLCASTER));
	return (randomSpellcaster && casterStack->canCast());
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

	if (vstd::contains(possibleActions, PossiblePlayerBattleAction::NO_LOCATION))
	{
		const spells::Caster * caster = owner.stacksController->getActiveStack();
		const CSpell * spell = getStackSpellToCast(BattleHex::INVALID);

		spells::Target target;
		target.emplace_back();

		spells::BattleCast cast(owner.curInt->cb.get(), caster, spells::Mode::CREATURE_ACTIVE, spell);

		auto m = spell->battleMechanics(&cast);
		spells::detail::ProblemImpl ignored;

		const bool isCastingPossible = m->canBeCastAt(target, ignored);

		if (isCastingPossible)
		{
			owner.giveCommand(EActionType::MONSTER_SPELL, BattleHex::INVALID, spell->getId());
			owner.stacksController->setSelectedStack(nullptr);

			CCS->curh->set(Cursor::Combat::POINTER);
		}
	}
	else
	{
		possibleActions = getPossibleActionsForStack(owner.stacksController->getActiveStack());

		auto actionFilterPredicate = [](const PossiblePlayerBattleAction x)
		{
			return (x != PossiblePlayerBattleAction::ANY_LOCATION) && (x != PossiblePlayerBattleAction::NO_LOCATION) &&
				(x != PossiblePlayerBattleAction::FREE_LOCATION) && (x != PossiblePlayerBattleAction::AIMED_SPELL_CREATURE) &&
				(x != PossiblePlayerBattleAction::OBSTACLE);
		};

		vstd::erase_if(possibleActions, actionFilterPredicate);
		GH.fakeMouseMove();
	}
}

std::vector<PossiblePlayerBattleAction> BattleActionsController::getPossibleActionsForStack(const CStack *stack) const
{
	BattleClientInterfaceData data; //hard to get rid of these things so for now they're required data to pass

	if (getStackSpellToCast(BattleHex::INVALID))
		data.creatureSpellToCast = getStackSpellToCast(BattleHex::INVALID)->getId();
	else
		data.creatureSpellToCast = SpellID::NONE;

	data.tacticsMode = owner.tacticsMode;
	auto allActions = owner.curInt->cb->getClientActionsForStack(stack, data);

	allActions.push_back(PossiblePlayerBattleAction::CREATURE_INFO);

	return std::vector<PossiblePlayerBattleAction>(allActions);
}

void BattleActionsController::reorderPossibleActionsPriority(const CStack * stack, MouseHoveredHexContext context)
{
	if(owner.tacticsMode || possibleActions.empty()) return; //this function is not supposed to be called in tactics mode or before getPossibleActionsForStack

	auto assignPriority = [&](PossiblePlayerBattleAction const & item) -> uint8_t //large lambda assigning priority which would have to be part of possibleActions without it
	{
		switch(item)
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

	if (spellSelMode == PossiblePlayerBattleAction::NO_LOCATION) //user does not have to select location
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
}

const CSpell * BattleActionsController::getHeroSpellToCast( ) const
{
	if (heroSpellToCast)
		return SpellID(heroSpellToCast->actionSubtype).toSpell();
	return nullptr;
}

const CSpell * BattleActionsController::getStackSpellToCast( BattleHex targetHex ) const
{
	if (isActiveStackSpellcaster())
		return creatureSpellToCast;

	return nullptr;
}

const CSpell * BattleActionsController::getAnySpellToCast( BattleHex targetHex ) const
{
	if (getHeroSpellToCast())
		return getHeroSpellToCast();
	return getStackSpellToCast(targetHex);
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
	switch (action)
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
	}
	assert(0);
}

void BattleActionsController::actionSetCursorBlocked(PossiblePlayerBattleAction action, BattleHex targetHex)
{
	switch (action)
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

	switch (action) //display console message, realize selected action
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
				TDmgRange damage = owner.curInt->cb->battleEstimateDamage(owner.stacksController->getActiveStack(), targetStack, attackFromHex);
				std::string estDmgText = formatDmgRange(std::make_pair((ui32)damage.first, (ui32)damage.second)); //calculating estimated dmg
				return (boost::format(CGI->generaltexth->allTexts[36]) % targetStack->getName() % estDmgText).str(); //Attack %s (%s damage)
			}

		case PossiblePlayerBattleAction::SHOOT:
		{
			auto const * shooter = owner.stacksController->getActiveStack();

			TDmgRange damage = owner.curInt->cb->battleEstimateDamage(shooter, targetStack, shooter->getPosition());
			std::string estDmgText = formatDmgRange(std::make_pair((ui32)damage.first, (ui32)damage.second)); //calculating estimated dmg
			//printing - Shoot %s (%d shots left, %s damage)
			return (boost::format(CGI->generaltexth->allTexts[296]) % targetStack->getName() % shooter->shots.available() % estDmgText).str();
		}

		case PossiblePlayerBattleAction::AIMED_SPELL_CREATURE:
			return boost::str(boost::format(CGI->generaltexth->allTexts[27]) % getAnySpellToCast(targetHex)->getNameTranslated() % targetStack->getName()); //Cast %s on %s

		case PossiblePlayerBattleAction::ANY_LOCATION:
			return boost::str(boost::format(CGI->generaltexth->allTexts[26]) % getAnySpellToCast(targetHex)->getNameTranslated()); //Cast %s

		case PossiblePlayerBattleAction::RANDOM_GENIE_SPELL: //we assume that teleport / sacrifice will never be available as random spell
			return boost::str(boost::format(CGI->generaltexth->allTexts[301]) % targetStack->getName()); //Cast a spell on %

		case PossiblePlayerBattleAction::TELEPORT:
			return CGI->generaltexth->allTexts[25]; //Teleport Here

		case PossiblePlayerBattleAction::OBSTACLE:
			return CGI->generaltexth->allTexts[550];

		case PossiblePlayerBattleAction::SACRIFICE:
			return (boost::format(CGI->generaltexth->allTexts[549]) % targetStack->getName()).str(); //sacrifice the %s

		case PossiblePlayerBattleAction::FREE_LOCATION:
			return boost::str(boost::format(CGI->generaltexth->allTexts[26]) % getAnySpellToCast(targetHex)->getNameTranslated()); //Cast %s

		case PossiblePlayerBattleAction::HEAL:
			return (boost::format(CGI->generaltexth->allTexts[419]) % targetStack->getName()).str(); //Apply first aid to the %s

		case PossiblePlayerBattleAction::CATAPULT:
			return ""; // TODO

		case PossiblePlayerBattleAction::CREATURE_INFO:
			return (boost::format(CGI->generaltexth->allTexts[297]) % targetStack->getName()).str();
	}
	assert(0);
	return "";
}

std::string BattleActionsController::actionGetStatusMessageBlocked(PossiblePlayerBattleAction action, BattleHex targetHex)
{
	switch (action)
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
			return boost::str(boost::format(CGI->generaltexth->allTexts[181]) % getAnySpellToCast(targetHex)->getNameTranslated()); //No room to place %s here
			break;
		default:
			return "";
	}
}

bool BattleActionsController::actionIsLegal(PossiblePlayerBattleAction action, BattleHex targetHex)
{
	const CStack * targetStack = getStackForHex(targetHex);
	bool targetStackOwned = targetStack && targetStack->owner == owner.curInt->playerID;

	switch (action)
	{
		case PossiblePlayerBattleAction::CHOOSE_TACTICS_STACK:
		case PossiblePlayerBattleAction::CREATURE_INFO:
			return (targetStack && targetStackOwned);

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

		case PossiblePlayerBattleAction::ANY_LOCATION:
			return isCastingPossibleHere(owner.stacksController->getActiveStack(), targetStack, targetHex);

		case PossiblePlayerBattleAction::AIMED_SPELL_CREATURE:
			return targetStack && isCastingPossibleHere(owner.stacksController->getActiveStack(), targetStack, targetHex);

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
			return owner.curInt->cb->battleCanTeleportTo(owner.stacksController->getSelectedStack(), targetHex, skill);
		}

		case PossiblePlayerBattleAction::SACRIFICE: //choose our living stack to sacrifice
			return targetStack && targetStack != owner.stacksController->getSelectedStack() && targetStackOwned && targetStack->alive();

		case PossiblePlayerBattleAction::OBSTACLE:
		case PossiblePlayerBattleAction::FREE_LOCATION:
			return isCastingPossibleHere(owner.stacksController->getActiveStack(), targetStack, targetHex);
			return isCastingPossibleHere(owner.stacksController->getActiveStack(), targetStack, targetHex);

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

	switch (action) //display console message, realize selected action
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
			bool returnAfterAttack = action == PossiblePlayerBattleAction::ATTACK_AND_RETURN;
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

		case PossiblePlayerBattleAction::AIMED_SPELL_CREATURE:
		case PossiblePlayerBattleAction::ANY_LOCATION:
		case PossiblePlayerBattleAction::RANDOM_GENIE_SPELL: //we assume that teleport / sacrifice will never be available as random spell
		case PossiblePlayerBattleAction::TELEPORT:
		case PossiblePlayerBattleAction::OBSTACLE:
		case PossiblePlayerBattleAction::SACRIFICE:
		case PossiblePlayerBattleAction::FREE_LOCATION:
		{
			if (action == PossiblePlayerBattleAction::AIMED_SPELL_CREATURE )
			{
				if (getAnySpellToCast(targetHex)->id == SpellID::SACRIFICE)
				{
					heroSpellToCast->aimToHex(targetHex);
					possibleActions.push_back(PossiblePlayerBattleAction::SACRIFICE);
					return;
				}
				if (getAnySpellToCast(targetHex)->id == SpellID::TELEPORT)
				{
					heroSpellToCast->aimToUnit(targetStack);
					possibleActions.push_back(PossiblePlayerBattleAction::TELEPORT);
					return;
				}
			}

			if (!spellcastingModeActive())
			{
				if (getStackSpellToCast(targetHex))
				{
					owner.giveCommand(EActionType::MONSTER_SPELL, targetHex, getStackSpellToCast(targetHex)->getId());
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
			owner.stacksController->setSelectedStack(nullptr);
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
	if (owner.stacksController->getActiveStack() == nullptr)
		return;

	if (hoveredHex == BattleHex::INVALID)
	{
		if (!currentConsoleMsg.empty())
			GH.statusbar->clearIfMatching(currentConsoleMsg);

		currentConsoleMsg.clear();
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

void BattleActionsController::onHexClicked(BattleHex clickedHex)
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
	const auto spellcaster = casterStack->getBonusLocalFirst(Selector::type()(Bonus::SPELLCASTER));
	if(casterStack->canCast() && spellcaster)
	{
		// faerie dragon can cast only one, randomly selected spell until their next move
		//TODO: what if creature can cast BOTH random genie spell and aimed spell?
		//TODO: faerie dragon type spell should be selected by server
		creatureSpellToCast = owner.curInt->cb->battleGetRandomStackSpell(CRandomGenerator::getDefault(), casterStack, CBattleInfoCallback::RANDOM_AIMED).toSpell();
	}
}

const spells::Caster * BattleActionsController::getCurrentSpellcaster() const
{
	if (heroSpellToCast)
		return owner.getActiveHero();
	else
		return owner.stacksController->getActiveStack();

}

bool BattleActionsController::isCastingPossibleHere(const CStack *casterStack, const CStack *targetStack, BattleHex targetHex)
{
	auto currentSpell = getAnySpellToCast(targetHex);
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
			switch(possibleActions.front())
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

bool BattleActionsController::spellcastingModeActive() const
{
	return heroSpellToCast != nullptr;;
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
