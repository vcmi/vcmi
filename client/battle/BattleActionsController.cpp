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
	creatureCasting(false),
	spellDestSelectMode(false),
	spellToCast(nullptr),
	currentSpell(nullptr)
{
	currentAction = PossiblePlayerBattleAction::INVALID;
	selectedAction = PossiblePlayerBattleAction::INVALID;
}

void BattleActionsController::endCastingSpell()
{
	if(spellDestSelectMode)
	{
		spellToCast.reset();

		currentSpell = nullptr;
		spellDestSelectMode = false;
		CCS->curh->set(Cursor::Combat::POINTER);

		if(owner.stacksController->getActiveStack())
		{
			possibleActions = getPossibleActionsForStack(owner.stacksController->getActiveStack()); //restore actions after they were cleared
			owner.myTurn = true;
		}
	}
	else
	{
		if(owner.stacksController->getActiveStack())
		{
			possibleActions = getPossibleActionsForStack(owner.stacksController->getActiveStack());
			GH.fakeMouseMove();
		}
	}
}

void BattleActionsController::enterCreatureCastingMode()
{
	//silently check for possible errors
	if (!owner.myTurn)
		return;

	if (owner.tacticsMode)
		return;

	//hero is casting a spell
	if (spellDestSelectMode)
		return;

	if (!owner.stacksController->getActiveStack())
		return;

	if (!owner.stacksController->activeStackSpellcaster())
		return;

	//random spellcaster
	if (owner.stacksController->activeStackSpellToCast() == SpellID::NONE)
		return;

	if (vstd::contains(possibleActions, PossiblePlayerBattleAction::NO_LOCATION))
	{
		const spells::Caster * caster = owner.stacksController->getActiveStack();
		const CSpell * spell = owner.stacksController->activeStackSpellToCast().toSpell();

		spells::Target target;
		target.emplace_back();

		spells::BattleCast cast(owner.curInt->cb.get(), caster, spells::Mode::CREATURE_ACTIVE, spell);

		auto m = spell->battleMechanics(&cast);
		spells::detail::ProblemImpl ignored;

		const bool isCastingPossible = m->canBeCastAt(target, ignored);

		if (isCastingPossible)
		{
			owner.myTurn = false;
			owner.giveCommand(EActionType::MONSTER_SPELL, BattleHex::INVALID, owner.stacksController->activeStackSpellToCast());
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
	data.creatureSpellToCast = owner.stacksController->activeStackSpellToCast();
	data.tacticsMode = owner.tacticsMode;
	auto allActions = owner.curInt->cb->getClientActionsForStack(stack, data);

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
		default:
			return 200; break;
		}
	};

	auto comparer = [&](PossiblePlayerBattleAction const & lhs, PossiblePlayerBattleAction const & rhs)
	{
		return assignPriority(lhs) > assignPriority(rhs);
	};

	std::make_heap(possibleActions.begin(), possibleActions.end(), comparer);
}

void BattleActionsController::castThisSpell(SpellID spellID)
{
	spellToCast = std::make_shared<BattleAction>();
	spellToCast->actionType = EActionType::HERO_SPELL;
	spellToCast->actionSubtype = spellID; //spell number
	spellToCast->stackNumber = (owner.attackingHeroInstance->tempOwner == owner.curInt->playerID) ? -1 : -2;
	spellToCast->side = owner.defendingHeroInstance ? (owner.curInt->playerID == owner.defendingHeroInstance->tempOwner) : false;
	spellDestSelectMode = true;
	creatureCasting = false;

	//choosing possible targets
	const CGHeroInstance *castingHero = (owner.attackingHeroInstance->tempOwner == owner.curInt->playerID) ? owner.attackingHeroInstance : owner.defendingHeroInstance;
	assert(castingHero); // code below assumes non-null hero
	currentSpell = spellID.toSpell();
	PossiblePlayerBattleAction spellSelMode = owner.curInt->cb->getCasterAction(currentSpell, castingHero, spells::Mode::HERO);

	if (spellSelMode == PossiblePlayerBattleAction::NO_LOCATION) //user does not have to select location
	{
		spellToCast->aimToHex(BattleHex::INVALID);
		owner.curInt->cb->battleMakeAction(spellToCast.get());
		endCastingSpell();
	}
	else
	{
		possibleActions.clear();
		possibleActions.push_back (spellSelMode); //only this one action can be performed at the moment
		GH.fakeMouseMove();//update cursor
	}
}


void BattleActionsController::handleHex(BattleHex myNumber, int eventType)
{
	if (!owner.myTurn) //we are not permit to do anything
		return;

	// This function handles mouse move over hexes and l-clicking on them.
	// First we decide what happens if player clicks on this hex and set appropriately
	// consoleMsg, cursorFrame/Type and prepare lambda realizeAction.
	//
	// Then, depending whether it was hover/click we either call the action or set tooltip/cursor.

	std::string newConsoleMsg;
	//used when hovering -> tooltip message and cursor to be set
	bool setCursor = true; //if we want to suppress setting cursor
	bool spellcastingCursor = false;
	auto cursorFrame = Cursor::Combat::POINTER;

	//used when l-clicking -> action to be called upon the click
	std::function<void()> realizeAction;

	//Get stack on the hex - first try to grab the alive one, if not found -> allow dead stacks.
	const CStack * shere = owner.curInt->cb->battleGetStackByPos(myNumber, true);
	if(!shere)
		shere = owner.curInt->cb->battleGetStackByPos(myNumber, false);

	if(!owner.stacksController->getActiveStack())
		return;

	bool ourStack = false;
	if (shere)
		ourStack = shere->owner == owner.curInt->playerID;

	localActions.clear();
	illegalActions.clear();

	reorderPossibleActionsPriority(owner.stacksController->getActiveStack(), shere ? MouseHoveredHexContext::OCCUPIED_HEX : MouseHoveredHexContext::UNOCCUPIED_HEX);
	const bool forcedAction = possibleActions.size() == 1;

	for (PossiblePlayerBattleAction action : possibleActions)
	{
		bool legalAction = false; //this action is legal and can be performed
		bool notLegal = false; //this action is not legal and should display message

		switch (action)
		{
			case PossiblePlayerBattleAction::CHOOSE_TACTICS_STACK:
				if (shere && ourStack)
					legalAction = true;
				break;
			case PossiblePlayerBattleAction::MOVE_TACTICS:
			case PossiblePlayerBattleAction::MOVE_STACK:
			{
				if (!(shere && shere->alive())) //we can walk on dead stacks
				{
					if(canStackMoveHere(owner.stacksController->getActiveStack(), myNumber))
						legalAction = true;
				}
				break;
			}
			case PossiblePlayerBattleAction::ATTACK:
			case PossiblePlayerBattleAction::WALK_AND_ATTACK:
			case PossiblePlayerBattleAction::ATTACK_AND_RETURN:
			{
				if(owner.curInt->cb->battleCanAttack(owner.stacksController->getActiveStack(), shere, myNumber))
				{
					if (owner.fieldController->isTileAttackable(myNumber)) // move isTileAttackable to be part of battleCanAttack?
					{
						BattleHex attackFromHex = owner.fieldController->fromWhichHexAttack(myNumber);

						if (attackFromHex >= 0) //we can be in this line when unreachable creature is L - clicked (as of revision 1308)
							legalAction = true;
					}
				}
			}
				break;
			case PossiblePlayerBattleAction::SHOOT:
				if(owner.curInt->cb->battleCanShoot(owner.stacksController->getActiveStack(), myNumber))
					legalAction = true;
				break;
			case PossiblePlayerBattleAction::ANY_LOCATION:
				if (myNumber > -1) //TODO: this should be checked for all actions
				{
					if(isCastingPossibleHere(owner.stacksController->getActiveStack(), shere, myNumber))
						legalAction = true;
				}
				break;
			case PossiblePlayerBattleAction::AIMED_SPELL_CREATURE:
				if(shere && isCastingPossibleHere(owner.stacksController->getActiveStack(), shere, myNumber))
					legalAction = true;
				break;
			case PossiblePlayerBattleAction::RANDOM_GENIE_SPELL:
			{
				if(shere && ourStack && shere != owner.stacksController->getActiveStack() && shere->alive()) //only positive spells for other allied creatures
				{
					int spellID = owner.curInt->cb->battleGetRandomStackSpell(CRandomGenerator::getDefault(), shere, CBattleInfoCallback::RANDOM_GENIE);
					if(spellID > -1)
					{
						legalAction = true;
					}
				}
			}
				break;
			case PossiblePlayerBattleAction::OBSTACLE:
				if(isCastingPossibleHere(owner.stacksController->getActiveStack(), shere, myNumber))
					legalAction = true;
				break;
			case PossiblePlayerBattleAction::TELEPORT:
			{
				//todo: move to mechanics
				ui8 skill = 0;
				if (creatureCasting)
					skill = owner.stacksController->getActiveStack()->getEffectLevel(SpellID(SpellID::TELEPORT).toSpell());
				else
					skill = owner.getActiveHero()->getEffectLevel(SpellID(SpellID::TELEPORT).toSpell());
				//TODO: explicitely save power, skill
				if (owner.curInt->cb->battleCanTeleportTo(owner.stacksController->getSelectedStack(), myNumber, skill))
					legalAction = true;
				else
					notLegal = true;
			}
				break;
			case PossiblePlayerBattleAction::SACRIFICE: //choose our living stack to sacrifice
				if (shere && shere != owner.stacksController->getSelectedStack() && ourStack && shere->alive())
					legalAction = true;
				else
					notLegal = true;
				break;
			case PossiblePlayerBattleAction::FREE_LOCATION:
				legalAction = true;
				if(!isCastingPossibleHere(owner.stacksController->getActiveStack(), shere, myNumber))
				{
					legalAction = false;
					notLegal = true;
				}
				break;
			case PossiblePlayerBattleAction::CATAPULT:
				if (owner.siegeController && owner.siegeController->isAttackableByCatapult(myNumber))
					legalAction = true;
				break;
			case PossiblePlayerBattleAction::HEAL:
				if (shere && ourStack && shere->canBeHealed())
					legalAction = true;
				break;
		}
		if (legalAction)
			localActions.push_back (action);
		else if (notLegal || forcedAction)
			illegalActions.push_back (action);
	}
	illegalAction = PossiblePlayerBattleAction::INVALID; //clear it in first place

	if (vstd::contains(localActions, selectedAction)) //try to use last selected action by default
		currentAction = selectedAction;
	else if (localActions.size()) //if not possible, select first available action (they are sorted by suggested priority)
		currentAction = localActions.front();
	else //no legal action possible
	{
		currentAction = PossiblePlayerBattleAction::INVALID; //don't allow to do anything

		if (vstd::contains(illegalActions, selectedAction))
			illegalAction = selectedAction;
		else if (illegalActions.size())
			illegalAction = illegalActions.front();
		else if (shere && ourStack && shere->alive()) //last possibility - display info about our creature
		{
			currentAction = PossiblePlayerBattleAction::CREATURE_INFO;
		}
		else
			illegalAction = PossiblePlayerBattleAction::INVALID; //we should never be here
	}

	bool isCastingPossible = false;
	bool secondaryTarget = false;

	if (currentAction > PossiblePlayerBattleAction::INVALID)
	{
		switch (currentAction) //display console message, realize selected action
		{
			case PossiblePlayerBattleAction::CHOOSE_TACTICS_STACK:
				newConsoleMsg = (boost::format(CGI->generaltexth->allTexts[481]) % shere->getName()).str(); //Select %s
				realizeAction = [=](){ owner.stackActivated(shere); };
				break;
			case PossiblePlayerBattleAction::MOVE_TACTICS:
			case PossiblePlayerBattleAction::MOVE_STACK:
				if (owner.stacksController->getActiveStack()->hasBonusOfType(Bonus::FLYING))
				{
					cursorFrame = Cursor::Combat::FLY;
					newConsoleMsg = (boost::format(CGI->generaltexth->allTexts[295]) % owner.stacksController->getActiveStack()->getName()).str(); //Fly %s here
				}
				else
				{
					cursorFrame = Cursor::Combat::MOVE;
					newConsoleMsg = (boost::format(CGI->generaltexth->allTexts[294]) % owner.stacksController->getActiveStack()->getName()).str(); //Move %s here
				}

				realizeAction = [=]()
				{
					if(owner.stacksController->getActiveStack()->doubleWide())
					{
						std::vector<BattleHex> acc = owner.curInt->cb->battleGetAvailableHexes(owner.stacksController->getActiveStack());
						BattleHex shiftedDest = myNumber.cloneInDirection(owner.stacksController->getActiveStack()->destShiftDir(), false);
						if(vstd::contains(acc, myNumber))
							owner.giveCommand(EActionType::WALK, myNumber);
						else if(vstd::contains(acc, shiftedDest))
							owner.giveCommand(EActionType::WALK, shiftedDest);
					}
					else
					{
						owner.giveCommand(EActionType::WALK, myNumber);
					}
				};
				break;
			case PossiblePlayerBattleAction::ATTACK:
			case PossiblePlayerBattleAction::WALK_AND_ATTACK:
			case PossiblePlayerBattleAction::ATTACK_AND_RETURN: //TODO: allow to disable return
				{
					owner.fieldController->setBattleCursor(myNumber); //handle direction of cursor
					setCursor = false; //don't overwrite settings from the call above //TODO: what does it mean?

					bool returnAfterAttack = currentAction == PossiblePlayerBattleAction::ATTACK_AND_RETURN;

					realizeAction = [=]()
					{
						BattleHex attackFromHex = owner.fieldController->fromWhichHexAttack(myNumber);
						if(attackFromHex.isValid()) //we can be in this line when unreachable creature is L - clicked (as of revision 1308)
						{
							auto command = new BattleAction(BattleAction::makeMeleeAttack(owner.stacksController->getActiveStack(), myNumber, attackFromHex, returnAfterAttack));
							owner.sendCommand(command, owner.stacksController->getActiveStack());
						}
					};

					BattleHex attackFromHex = owner.fieldController->fromWhichHexAttack(myNumber);
					TDmgRange damage = owner.curInt->cb->battleEstimateDamage(owner.stacksController->getActiveStack(), shere, attackFromHex);
					std::string estDmgText = formatDmgRange(std::make_pair((ui32)damage.first, (ui32)damage.second)); //calculating estimated dmg
					newConsoleMsg = (boost::format(CGI->generaltexth->allTexts[36]) % shere->getName() % estDmgText).str(); //Attack %s (%s damage)
				}
				break;
			case PossiblePlayerBattleAction::SHOOT:
			{
				auto const * shooter = owner.stacksController->getActiveStack();

				if (owner.curInt->cb->battleHasShootingPenalty(shooter, myNumber))
					cursorFrame = Cursor::Combat::SHOOT_PENALTY;
				else
					cursorFrame = Cursor::Combat::SHOOT;

				realizeAction = [=](){owner.giveCommand(EActionType::SHOOT, myNumber);};
				TDmgRange damage = owner.curInt->cb->battleEstimateDamage(shooter, shere, shooter->getPosition());
				std::string estDmgText = formatDmgRange(std::make_pair((ui32)damage.first, (ui32)damage.second)); //calculating estimated dmg
				//printing - Shoot %s (%d shots left, %s damage)
				newConsoleMsg = (boost::format(CGI->generaltexth->allTexts[296]) % shere->getName() % shooter->shots.available() % estDmgText).str();
			}
				break;
			case PossiblePlayerBattleAction::AIMED_SPELL_CREATURE:
				currentSpell = CGI->spellh->objects[creatureCasting ? owner.stacksController->activeStackSpellToCast() : spellToCast->actionSubtype]; //necessary if creature has random Genie spell at same time
				newConsoleMsg = boost::str(boost::format(CGI->generaltexth->allTexts[27]) % currentSpell->name % shere->getName()); //Cast %s on %s
				switch (currentSpell->id)
				{
					case SpellID::SACRIFICE:
					case SpellID::TELEPORT:
						owner.stacksController->setSelectedStack(shere); //remember first target
						secondaryTarget = true;
						break;
				}
				isCastingPossible = true;
				break;
			case PossiblePlayerBattleAction::ANY_LOCATION:
				currentSpell = CGI->spellh->objects[creatureCasting ? owner.stacksController->activeStackSpellToCast() : spellToCast->actionSubtype]; //necessary if creature has random Genie spell at same time
				newConsoleMsg = boost::str(boost::format(CGI->generaltexth->allTexts[26]) % currentSpell->name); //Cast %s
				isCastingPossible = true;
				break;
			case PossiblePlayerBattleAction::RANDOM_GENIE_SPELL: //we assume that teleport / sacrifice will never be available as random spell
				currentSpell = nullptr;
				newConsoleMsg = boost::str(boost::format(CGI->generaltexth->allTexts[301]) % shere->getName()); //Cast a spell on %
				creatureCasting = true;
				isCastingPossible = true;
				break;
			case PossiblePlayerBattleAction::TELEPORT:
				newConsoleMsg = CGI->generaltexth->allTexts[25]; //Teleport Here
				cursorFrame = Cursor::Combat::TELEPORT;
				isCastingPossible = true;
				break;
			case PossiblePlayerBattleAction::OBSTACLE:
				newConsoleMsg = CGI->generaltexth->allTexts[550];
				//TODO: remove obstacle cursor
				isCastingPossible = true;
				break;
			case PossiblePlayerBattleAction::SACRIFICE:
				newConsoleMsg = (boost::format(CGI->generaltexth->allTexts[549]) % shere->getName()).str(); //sacrifice the %s
				cursorFrame = Cursor::Combat::SACRIFICE;
				isCastingPossible = true;
				break;
			case PossiblePlayerBattleAction::FREE_LOCATION:
				newConsoleMsg = boost::str(boost::format(CGI->generaltexth->allTexts[26]) % currentSpell->name); //Cast %s
				isCastingPossible = true;
				break;
			case PossiblePlayerBattleAction::HEAL:
				cursorFrame = Cursor::Combat::HEAL;
				newConsoleMsg = (boost::format(CGI->generaltexth->allTexts[419]) % shere->getName()).str(); //Apply first aid to the %s
				realizeAction = [=](){ owner.giveCommand(EActionType::STACK_HEAL, myNumber); }; //command healing
				break;
			case PossiblePlayerBattleAction::CATAPULT:
				cursorFrame = Cursor::Combat::SHOOT_CATAPULT;
				realizeAction = [=](){ owner.giveCommand(EActionType::CATAPULT, myNumber); };
				break;
			case PossiblePlayerBattleAction::CREATURE_INFO:
			{
				cursorFrame = Cursor::Combat::QUERY;
				newConsoleMsg = (boost::format(CGI->generaltexth->allTexts[297]) % shere->getName()).str();
				realizeAction = [=](){ GH.pushIntT<CStackWindow>(shere, false); };
				break;
			}
		}
	}
	else //no possible valid action, display message
	{
		switch (illegalAction)
		{
			case PossiblePlayerBattleAction::AIMED_SPELL_CREATURE:
			case PossiblePlayerBattleAction::RANDOM_GENIE_SPELL:
				cursorFrame = Cursor::Combat::BLOCKED;
				newConsoleMsg = CGI->generaltexth->allTexts[23];
				break;
			case PossiblePlayerBattleAction::TELEPORT:
				cursorFrame = Cursor::Combat::BLOCKED;
				newConsoleMsg = CGI->generaltexth->allTexts[24]; //Invalid Teleport Destination
				break;
			case PossiblePlayerBattleAction::SACRIFICE:
				newConsoleMsg = CGI->generaltexth->allTexts[543]; //choose army to sacrifice
				break;
			case PossiblePlayerBattleAction::FREE_LOCATION:
				cursorFrame = Cursor::Combat::BLOCKED;
				newConsoleMsg = boost::str(boost::format(CGI->generaltexth->allTexts[181]) % currentSpell->name); //No room to place %s here
				break;
			default:
				if (myNumber == -1)
					CCS->curh->set(Cursor::Combat::POINTER);
				else
					cursorFrame = Cursor::Combat::BLOCKED;
				break;
		}
	}

	if (isCastingPossible) //common part
	{
		switch (currentAction) //don't use that with teleport / sacrifice
		{
			case PossiblePlayerBattleAction::TELEPORT: //FIXME: more generic solution?
			case PossiblePlayerBattleAction::SACRIFICE:
				break;
			default:
				spellcastingCursor = true;
				if (newConsoleMsg.empty() && currentSpell)
					newConsoleMsg = boost::str(boost::format(CGI->generaltexth->allTexts[26]) % currentSpell->name); //Cast %s
				break;
		}

		realizeAction = [=]()
		{
			if(secondaryTarget) //select that target now
			{

				possibleActions.clear();
				switch (currentSpell->id.toEnum())
				{
					case SpellID::TELEPORT: //don't cast spell yet, only select target
						spellToCast->aimToUnit(shere);
						possibleActions.push_back(PossiblePlayerBattleAction::TELEPORT);
						break;
					case SpellID::SACRIFICE:
						spellToCast->aimToHex(myNumber);
						possibleActions.push_back(PossiblePlayerBattleAction::SACRIFICE);
						break;
				}
			}
			else
			{
				if (creatureCasting)
				{
					if (currentSpell)
					{
						owner.giveCommand(EActionType::MONSTER_SPELL, myNumber, owner.stacksController->activeStackSpellToCast());
					}
					else //unknown random spell
					{
						owner.giveCommand(EActionType::MONSTER_SPELL, myNumber);
					}
				}
				else
				{
					assert(currentSpell);
					switch (currentSpell->id.toEnum())
					{
					case SpellID::SACRIFICE:
						spellToCast->aimToUnit(shere);//victim
						break;
					default:
						spellToCast->aimToHex(myNumber);
						break;
					}
					owner.curInt->cb->battleMakeAction(spellToCast.get());
					endCastingSpell();
				}
				owner.stacksController->setSelectedStack(nullptr);
			}
		};
	}

	{
		if (eventType == CIntObject::MOVE)
		{
			if (setCursor)
			{
				if (spellcastingCursor)
					CCS->curh->set(Cursor::Spellcast::SPELL);
				else
					CCS->curh->set(cursorFrame);
			}

			if (!currentConsoleMsg.empty())
				GH.statusbar->clearIfMatching(currentConsoleMsg);
			if (!newConsoleMsg.empty())
				GH.statusbar->write(newConsoleMsg);

			currentConsoleMsg = newConsoleMsg;
		}
		if (eventType == CIntObject::LCLICK && realizeAction)
		{
			//opening creature window shouldn't affect myTurn...
			if ((currentAction != PossiblePlayerBattleAction::CREATURE_INFO) && !secondaryTarget)
			{
				owner.myTurn = false; //tends to crash with empty calls
			}
			realizeAction();
			if (!secondaryTarget) //do not replace teleport or sacrifice cursor
				CCS->curh->set(Cursor::Combat::POINTER);
			GH.statusbar->clear();
		}
	}
}


bool BattleActionsController::isCastingPossibleHere(const CStack *sactive, const CStack *shere, BattleHex myNumber)
{
	creatureCasting = owner.stacksController->activeStackSpellcaster() && !spellDestSelectMode; //TODO: allow creatures to cast aimed spells

	bool isCastingPossible = true;

	int spellID = -1;
	if (creatureCasting)
	{
		if (owner.stacksController->activeStackSpellToCast() != SpellID::NONE && (shere != sactive)) //can't cast on itself
			spellID = owner.stacksController->activeStackSpellToCast(); //TODO: merge with SpellTocast?
	}
	else //hero casting
	{
		spellID = spellToCast->actionSubtype;
	}


	currentSpell = nullptr;
	if (spellID >= 0)
		currentSpell = CGI->spellh->objects[spellID];

	if (currentSpell)
	{
		const spells::Caster *caster = creatureCasting ? static_cast<const spells::Caster *>(sactive) : static_cast<const spells::Caster *>(owner.curInt->cb->battleGetMyHero());
		if (caster == nullptr)
		{
			isCastingPossible = false;//just in case
		}
		else
		{
			const spells::Mode mode = creatureCasting ? spells::Mode::CREATURE_ACTIVE : spells::Mode::HERO;

			spells::Target target;
			target.emplace_back(myNumber);

			spells::BattleCast cast(owner.curInt->cb.get(), caster, mode, currentSpell);

			auto m = currentSpell->battleMechanics(&cast);
			spells::detail::ProblemImpl problem; //todo: display problem in status bar

			isCastingPossible = m->canBeCastAt(target, problem);
		}
	}
	else
		isCastingPossible = false;
	if (!myNumber.isAvailable() && !shere) //empty tile outside battlefield (or in the unavailable border column)
			isCastingPossible = false;

	return isCastingPossible;
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
	return spellDestSelectMode;
}

SpellID BattleActionsController::selectedSpell() const
{
	if (!spellToCast)
		return SpellID::NONE;
	return SpellID(spellToCast->actionSubtype);
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
