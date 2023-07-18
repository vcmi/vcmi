/*
 * BattleInterface.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleInterface.h"

#include "BattleAnimationClasses.h"
#include "BattleActionsController.h"
#include "BattleInterfaceClasses.h"
#include "CreatureAnimation.h"
#include "BattleProjectileController.h"
#include "BattleEffectsController.h"
#include "BattleObstacleController.h"
#include "BattleSiegeController.h"
#include "BattleFieldController.h"
#include "BattleWindow.h"
#include "BattleStacksController.h"
#include "BattleRenderer.h"

#include "../CGameInfo.h"
#include "../CMusicHandler.h"
#include "../CPlayerInterface.h"
#include "../gui/CursorHandler.h"
#include "../gui/CGuiHandler.h"
#include "../gui/WindowHandler.h"
#include "../render/Canvas.h"
#include "../adventureMap/AdventureMapInterface.h"

#include "../../CCallback.h"
#include "../../lib/CStack.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CHeroHandler.h"
#include "../../lib/CondSh.h"
#include "../../lib/gameState/InfoAboutArmy.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/NetPacks.h"
#include "../../lib/UnlockGuard.h"
#include "../../lib/TerrainHandler.h"

BattleInterface::BattleInterface(const CCreatureSet *army1, const CCreatureSet *army2,
		const CGHeroInstance *hero1, const CGHeroInstance *hero2,
		std::shared_ptr<CPlayerInterface> att,
		std::shared_ptr<CPlayerInterface> defen,
		std::shared_ptr<CPlayerInterface> spectatorInt)
	: attackingHeroInstance(hero1)
	, defendingHeroInstance(hero2)
	, attackerInt(att)
	, defenderInt(defen)
	, curInt(att)
	, battleOpeningDelayActive(true)
{
	if(spectatorInt)
	{
		curInt = spectatorInt;
	}
	else if(!curInt)
	{
		//May happen when we are defending during network MP game -> attacker interface is just not present
		curInt = defenderInt;
	}

	//hot-seat -> check tactics for both players (defender may be local human)
	if(attackerInt && attackerInt->cb->battleGetTacticDist())
		tacticianInterface = attackerInt;
	else if(defenderInt && defenderInt->cb->battleGetTacticDist())
		tacticianInterface = defenderInt;

	//if we found interface of player with tactics, then enter tactics mode
	tacticsMode = static_cast<bool>(tacticianInterface);

	//initializing armies
	this->army1 = army1;
	this->army2 = army2;

	const CGTownInstance *town = curInt->cb->battleGetDefendedTown();
	if(town && town->hasFort())
		siegeController.reset(new BattleSiegeController(*this, town));

	windowObject = std::make_shared<BattleWindow>(*this);
	projectilesController.reset(new BattleProjectileController(*this));
	stacksController.reset( new BattleStacksController(*this));
	actionsController.reset( new BattleActionsController(*this));
	effectsController.reset(new BattleEffectsController(*this));
	obstacleController.reset(new BattleObstacleController(*this));

	adventureInt->onAudioPaused();
	ongoingAnimationsState.set(true);

	GH.windows().pushWindow(windowObject);
	windowObject->blockUI(true);
	windowObject->updateQueue();

	playIntroSoundAndUnlockInterface();
}

void BattleInterface::playIntroSoundAndUnlockInterface()
{
	auto onIntroPlayed = [this]()
	{
		if(LOCPLINT->battleInt)
		{
			boost::unique_lock<boost::recursive_mutex> un(*CPlayerInterface::pim);
			onIntroSoundPlayed();
		}
	};

	int battleIntroSoundChannel = CCS->soundh->playSoundFromSet(CCS->soundh->battleIntroSounds);
	if (battleIntroSoundChannel != -1)
	{
		CCS->soundh->setCallback(battleIntroSoundChannel, onIntroPlayed);

		if (settings["gameTweaks"]["skipBattleIntroMusic"].Bool())
			openingEnd();
	}
	else // failed to play sound
	{
		onIntroSoundPlayed();
	}
}

bool BattleInterface::openingPlaying()
{
	return battleOpeningDelayActive;
}

void BattleInterface::onIntroSoundPlayed()
{
	if (openingPlaying())
		openingEnd();

	CCS->musich->playMusicFromSet("battle", true, true);
}

void BattleInterface::openingEnd()
{
	assert(openingPlaying());
	if (!openingPlaying())
		return;

	onAnimationsFinished();
	if(tacticsMode)
		tacticNextStack(nullptr);
	activateStack();
	battleOpeningDelayActive = false;
}

BattleInterface::~BattleInterface()
{
	CPlayerInterface::battleInt = nullptr;

	if (adventureInt)
		adventureInt->onAudioResumed();

	awaitingEvents.clear();
	onAnimationsFinished();
}

void BattleInterface::redrawBattlefield()
{
	fieldController->redrawBackgroundWithHexes();
	GH.windows().totalRedraw();
}

void BattleInterface::stackReset(const CStack * stack)
{
	stacksController->stackReset(stack);
}

void BattleInterface::stackAdded(const CStack * stack)
{
	stacksController->stackAdded(stack, false);
}

void BattleInterface::stackRemoved(uint32_t stackID)
{
	stacksController->stackRemoved(stackID);
	fieldController->redrawBackgroundWithHexes();
	windowObject->updateQueue();
}

void BattleInterface::stackActivated(const CStack *stack)
{
	stacksController->stackActivated(stack);
}

void BattleInterface::stackMoved(const CStack *stack, std::vector<BattleHex> destHex, int distance, bool teleport)
{
	if (teleport)
		stacksController->stackTeleported(stack, destHex, distance);
	else
		stacksController->stackMoved(stack, destHex, distance);
}

void BattleInterface::stacksAreAttacked(std::vector<StackAttackedInfo> attackedInfos)
{
	stacksController->stacksAreAttacked(attackedInfos);

	std::array<int, 2> killedBySide = {0, 0};

	for(const StackAttackedInfo & attackedInfo : attackedInfos)
	{
		ui8 side = attackedInfo.defender->unitSide();
		killedBySide.at(side) += attackedInfo.amountKilled;
	}

	for(ui8 side = 0; side < 2; side++)
	{
		if(killedBySide.at(side) > killedBySide.at(1-side))
			setHeroAnimation(side, EHeroAnimType::DEFEAT);
		else if(killedBySide.at(side) < killedBySide.at(1-side))
			setHeroAnimation(side, EHeroAnimType::VICTORY);
	}
}

void BattleInterface::stackAttacking( const StackAttackInfo & attackInfo )
{
	stacksController->stackAttacking(attackInfo);
}

void BattleInterface::newRoundFirst( int round )
{
	waitForAnimations();
}

void BattleInterface::newRound(int number)
{
	console->addText(CGI->generaltexth->allTexts[412]);
}

void BattleInterface::giveCommand(EActionType action, BattleHex tile, si32 additional)
{
	const CStack * actor = nullptr;
	if(action != EActionType::HERO_SPELL && action != EActionType::RETREAT && action != EActionType::SURRENDER)
	{
		actor = stacksController->getActiveStack();
	}

	auto side = curInt->cb->playerToSide(curInt->playerID);
	if(!side)
	{
		logGlobal->error("Player %s is not in battle", curInt->playerID.getStr());
		return;
	}

	BattleAction ba;
	ba.side = side.value();
	ba.actionType = action;
	ba.aimToHex(tile);
	ba.actionSubtype = additional;

	sendCommand(ba, actor);
}

void BattleInterface::sendCommand(BattleAction command, const CStack * actor)
{
	command.stackNumber = actor ? actor->unitId() : ((command.side == BattleSide::ATTACKER) ? -1 : -2);

	if(!tacticsMode)
	{
		logGlobal->trace("Setting command for %s", (actor ? actor->nodeName() : "hero"));
		stacksController->setActiveStack(nullptr);
		LOCPLINT->cb->battleMakeUnitAction(command);
	}
	else
	{
		curInt->cb->battleMakeTacticAction(command);
		stacksController->setActiveStack(nullptr);
		//next stack will be activated when action ends
	}
	CCS->curh->set(Cursor::Combat::POINTER);
}

const CGHeroInstance * BattleInterface::getActiveHero()
{
	const CStack *attacker = stacksController->getActiveStack();
	if(!attacker)
	{
		return nullptr;
	}

	if(attacker->unitSide() == BattleSide::ATTACKER)
	{
		return attackingHeroInstance;
	}

	return defendingHeroInstance;
}

void BattleInterface::stackIsCatapulting(const CatapultAttack & ca)
{
	if (siegeController)
		siegeController->stackIsCatapulting(ca);
}

void BattleInterface::gateStateChanged(const EGateState state)
{
	if (siegeController)
		siegeController->gateStateChanged(state);
}

void BattleInterface::battleFinished(const BattleResult& br, QueryID queryID)
{
	checkForAnimations();
	stacksController->setActiveStack(nullptr);

	CCS->curh->set(Cursor::Map::POINTER);
	curInt->waitWhileDialog();

	if(settings["session"]["spectate"].Bool() && settings["session"]["spectate-skip-battle-result"].Bool())
	{
		curInt->cb->selectionMade(0, queryID);
		windowObject->close();
		return;
	}

	auto wnd = std::make_shared<BattleResultWindow>(br, *(this->curInt));
	wnd->resultCallback = [=](ui32 selection)
	{
		curInt->cb->selectionMade(selection, queryID);
	};
	GH.windows().pushWindow(wnd);
	
	curInt->waitWhileDialog(); // Avoid freeze when AI end turn after battle. Check bug #1897
	CPlayerInterface::battleInt = nullptr;
}

void BattleInterface::spellCast(const BattleSpellCast * sc)
{
	// Do not deactivate anything in tactics mode
	// This is battlefield setup spells
	if(!tacticsMode)
	{
		windowObject->blockUI(true);

		// Disable current active stack duing the cast
		// Store the current activeStack to stackToActivate
		stacksController->deactivateStack();
	}

	CCS->curh->set(Cursor::Combat::BLOCKED);

	const SpellID spellID = sc->spellID;
	const CSpell * spell = spellID.toSpell();
	auto targetedTile = sc->tile;

	assert(spell);
	if(!spell)
		return;

	const std::string & castSoundPath = spell->getCastSound();

	if (!castSoundPath.empty())
	{
		auto group = spell->animationInfo.projectile.empty() ?
					EAnimationEvents::HIT:
					EAnimationEvents::BEFORE_HIT;//FIXME: recheck whether this should be on projectile spawning

		addToAnimationStage(group, [=]() {
			CCS->soundh->playSound(castSoundPath);
		});
	}

	if ( sc->activeCast )
	{
		const CStack * casterStack = curInt->cb->battleGetStackByID(sc->casterStack);

		if(casterStack != nullptr )
		{
			addToAnimationStage(EAnimationEvents::BEFORE_HIT, [=]()
			{
				stacksController->addNewAnim(new CastAnimation(*this, casterStack, targetedTile, curInt->cb->battleGetStackByPos(targetedTile), spell));
				displaySpellCast(spell, casterStack->getPosition());
			});
		}
		else
		{
			auto hero = sc->side ? defendingHero : attackingHero;
			assert(hero);

			addToAnimationStage(EAnimationEvents::BEFORE_HIT, [=]()
			{
				stacksController->addNewAnim(new HeroCastAnimation(*this, hero, targetedTile, curInt->cb->battleGetStackByPos(targetedTile), spell));
			});
		}
	}

	addToAnimationStage(EAnimationEvents::HIT, [=](){
		displaySpellHit(spell, targetedTile);
	});

	//queuing affect animation
	for(auto & elem : sc->affectedCres)
	{
		auto stack = curInt->cb->battleGetStackByID(elem, false);
		assert(stack);
		if(stack)
		{
			addToAnimationStage(EAnimationEvents::HIT, [=](){
				displaySpellEffect(spell, stack->getPosition());
			});
		}
	}

	for(auto & elem : sc->reflectedCres)
	{
		auto stack = curInt->cb->battleGetStackByID(elem, false);
		assert(stack);
		addToAnimationStage(EAnimationEvents::HIT, [=](){
			effectsController->displayEffect(EBattleEffect::MAGIC_MIRROR, stack->getPosition());
		});
	}

	if (!sc->resistedCres.empty())
	{
		addToAnimationStage(EAnimationEvents::HIT, [=](){
			CCS->soundh->playSound("MAGICRES");
		});
	}

	for(auto & elem : sc->resistedCres)
	{
		auto stack = curInt->cb->battleGetStackByID(elem, false);
		assert(stack);
		addToAnimationStage(EAnimationEvents::HIT, [=](){
			effectsController->displayEffect(EBattleEffect::RESISTANCE, stack->getPosition());
		});
	}

	//mana absorption
	if (sc->manaGained > 0)
	{
		Point leftHero = Point(15, 30);
		Point rightHero = Point(755, 30);
		bool side = sc->side;

		addToAnimationStage(EAnimationEvents::AFTER_HIT, [=](){
			stacksController->addNewAnim(new EffectAnimation(*this, side ? "SP07_A.DEF" : "SP07_B.DEF", leftHero));
			stacksController->addNewAnim(new EffectAnimation(*this, side ? "SP07_B.DEF" : "SP07_A.DEF", rightHero));
		});
	}
}

void BattleInterface::battleStacksEffectsSet(const SetStackEffect & sse)
{
	if(stacksController->getActiveStack() != nullptr)
		fieldController->redrawBackgroundWithHexes();
}

void BattleInterface::setHeroAnimation(ui8 side, EHeroAnimType phase)
{
	if(side == BattleSide::ATTACKER)
	{
		if(attackingHero)
			attackingHero->setPhase(phase);
	}
	else
	{
		if(defendingHero)
			defendingHero->setPhase(phase);
	}
}

void BattleInterface::displayBattleLog(const std::vector<MetaString> & battleLog)
{
	for(const auto & line : battleLog)
	{
		std::string formatted = line.toString();
		boost::algorithm::trim(formatted);
		appendBattleLog(formatted);
	}
}

void BattleInterface::displaySpellAnimationQueue(const CSpell * spell, const CSpell::TAnimationQueue & q, BattleHex destinationTile, bool isHit)
{
	for(const CSpell::TAnimation & animation : q)
	{
		if(animation.pause > 0)
			stacksController->addNewAnim(new DummyAnimation(*this, animation.pause));

		if (!animation.effectName.empty())
		{
			const CStack * destStack = getCurrentPlayerInterface()->cb->battleGetStackByPos(destinationTile, false);

			if (destStack)
				stacksController->addNewAnim(new ColorTransformAnimation(*this, destStack, animation.effectName, spell ));
		}

		if(!animation.resourceName.empty())
		{
			int flags = 0;

			if (isHit)
				flags |= EffectAnimation::FORCE_ON_TOP;

			if (animation.verticalPosition == VerticalPosition::BOTTOM)
				flags |= EffectAnimation::ALIGN_TO_BOTTOM;

			if (!destinationTile.isValid())
				flags |= EffectAnimation::SCREEN_FILL;

			if (!destinationTile.isValid())
				stacksController->addNewAnim(new EffectAnimation(*this, animation.resourceName, flags));
			else
				stacksController->addNewAnim(new EffectAnimation(*this, animation.resourceName, destinationTile, flags));
		}
	}
}

void BattleInterface::displaySpellCast(const CSpell * spell, BattleHex destinationTile)
{
	if(spell)
		displaySpellAnimationQueue(spell, spell->animationInfo.cast, destinationTile, false);
}

void BattleInterface::displaySpellEffect(const CSpell * spell, BattleHex destinationTile)
{
	if(spell)
		displaySpellAnimationQueue(spell, spell->animationInfo.affect, destinationTile, false);
}

void BattleInterface::displaySpellHit(const CSpell * spell, BattleHex destinationTile)
{
	if(spell)
		displaySpellAnimationQueue(spell, spell->animationInfo.hit, destinationTile, true);
}

CPlayerInterface *BattleInterface::getCurrentPlayerInterface() const
{
	return curInt.get();
}

void BattleInterface::trySetActivePlayer( PlayerColor player )
{
	if ( attackerInt && attackerInt->playerID == player )
		curInt = attackerInt;

	if ( defenderInt && defenderInt->playerID == player )
		curInt = defenderInt;
}

void BattleInterface::activateStack()
{
	stacksController->activateStack();

	const CStack * s = stacksController->getActiveStack();
	if(!s)
		return;

	windowObject->updateQueue();
	windowObject->blockUI(false);
	fieldController->redrawBackgroundWithHexes();
	actionsController->activateStack();
	GH.fakeMouseMove();
}

bool BattleInterface::makingTurn() const
{
	return stacksController->getActiveStack() != nullptr;
}

void BattleInterface::endAction(const BattleAction* action)
{
	// it is possible that tactics mode ended while opening music is still playing
	waitForAnimations();

	const CStack *stack = curInt->cb->battleGetStackByID(action->stackNumber);

	// Activate stack from stackToActivate because this might have been temporary disabled, e.g., during spell cast
	activateStack();

	stacksController->endAction(action);
	windowObject->updateQueue();

	//stack ended movement in tactics phase -> select the next one
	if (tacticsMode)
		tacticNextStack(stack);

	//we have activated next stack after sending request that has been just realized -> blockmap due to movement has changed
	if(action->actionType == EActionType::HERO_SPELL)
		fieldController->redrawBackgroundWithHexes();
}

void BattleInterface::appendBattleLog(const std::string & newEntry)
{
	console->addText(newEntry);
}

void BattleInterface::startAction(const BattleAction* action)
{
	if(action->actionType == EActionType::END_TACTIC_PHASE)
	{
		windowObject->tacticPhaseEnded();
		return;
	}

	const CStack *stack = curInt->cb->battleGetStackByID(action->stackNumber);

	if (stack)
	{
		windowObject->updateQueue();
	}
	else
	{
		assert(action->actionType == EActionType::HERO_SPELL); //only cast spell is valid action without acting stack number
	}

	stacksController->startAction(action);

	if(action->actionType == EActionType::HERO_SPELL) //when hero casts spell
		return;

	if (!stack)
	{
		logGlobal->error("Something wrong with stackNumber in actionStarted. Stack number: %d", action->stackNumber);
		return;
	}

	effectsController->startAction(action);
}

void BattleInterface::tacticPhaseEnd()
{
	stacksController->setActiveStack(nullptr);
	tacticsMode = false;

	auto side = tacticianInterface->cb->playerToSide(tacticianInterface->playerID);
	auto action = BattleAction::makeEndOFTacticPhase(*side);

	tacticianInterface->cb->battleMakeTacticAction(action);
}

static bool immobile(const CStack *s)
{
	return !s->speed(0, true); //should bound stacks be immobile?
}

void BattleInterface::tacticNextStack(const CStack * current)
{
	if (!current)
		current = stacksController->getActiveStack();

	//no switching stacks when the current one is moving
	checkForAnimations();

	TStacks stacksOfMine = tacticianInterface->cb->battleGetStacks(CBattleCallback::ONLY_MINE);
	vstd::erase_if (stacksOfMine, &immobile);
	if (stacksOfMine.empty())
	{
		tacticPhaseEnd();
		return;
	}

	auto it = vstd::find(stacksOfMine, current);
	if (it != stacksOfMine.end() && ++it != stacksOfMine.end())
		stackActivated(*it);
	else
		stackActivated(stacksOfMine.front());

}

void BattleInterface::obstaclePlaced(const std::vector<std::shared_ptr<const CObstacleInstance>> oi)
{
	obstacleController->obstaclePlaced(oi);
}

void BattleInterface::obstacleRemoved(const std::vector<ObstacleChanges> & obstacles)
{
	obstacleController->obstacleRemoved(obstacles);
}

const CGHeroInstance *BattleInterface::currentHero() const
{
	if (attackingHeroInstance && attackingHeroInstance->tempOwner == curInt->playerID)
		return attackingHeroInstance;

	if (defendingHeroInstance && defendingHeroInstance->tempOwner == curInt->playerID)
		return defendingHeroInstance;

	return nullptr;
}

InfoAboutHero BattleInterface::enemyHero() const
{
	InfoAboutHero ret;
	if (attackingHeroInstance->tempOwner == curInt->playerID)
		curInt->cb->getHeroInfo(defendingHeroInstance, ret);
	else
		curInt->cb->getHeroInfo(attackingHeroInstance, ret);

	return ret;
}

void BattleInterface::requestAutofightingAIToTakeAction()
{
	assert(curInt->isAutoFightOn);

	boost::thread aiThread([&]()
	{
		if(curInt->cb->battleIsFinished())
		{
			return; // battle finished with spellcast
		}

		if (tacticsMode)
		{
			// Always end tactics mode. Player interface is blocked currently, so it's not possible that
			// the AI can take any action except end tactics phase (AI actions won't be triggered)
			//TODO implement the possibility that the AI will be triggered for further actions
			//TODO any solution to merge tactics phase & normal phase in the way it is handled by the player and battle interface?
			tacticPhaseEnd();
			stacksController->setActiveStack(nullptr);
		}
		else
		{
			const CStack* activeStack = stacksController->getActiveStack();

			// If enemy is moving, activeStack can be null
			if (activeStack)
				curInt->autofightingAI->activeStack(activeStack);

			stacksController->setActiveStack(nullptr);
		}
	});

	aiThread.detach();
}

void BattleInterface::castThisSpell(SpellID spellID)
{
	actionsController->castThisSpell(spellID);
}

void BattleInterface::executeStagedAnimations()
{
	EAnimationEvents earliestStage = EAnimationEvents::COUNT;

	for(const auto & event : awaitingEvents)
		earliestStage = std::min(earliestStage, event.event);

	if(earliestStage != EAnimationEvents::COUNT)
		executeAnimationStage(earliestStage);
}

void BattleInterface::executeAnimationStage(EAnimationEvents event)
{
	decltype(awaitingEvents) executingEvents;

	for(auto it = awaitingEvents.begin(); it != awaitingEvents.end();)
	{
		if(it->event == event)
		{
			executingEvents.push_back(*it);
			it = awaitingEvents.erase(it);
		}
		else
			++it;
	}
	for(const auto & event : executingEvents)
		event.action();
}

void BattleInterface::onAnimationsStarted()
{
	ongoingAnimationsState.setn(true);
}

void BattleInterface::onAnimationsFinished()
{
	ongoingAnimationsState.setn(false);
}

void BattleInterface::waitForAnimations()
{
	{
		auto unlockPim = vstd::makeUnlockGuard(*CPlayerInterface::pim);
		ongoingAnimationsState.waitUntil(false);
	}

	assert(!hasAnimations());
	assert(awaitingEvents.empty());

	if (!awaitingEvents.empty())
	{
		logGlobal->error("Wait for animations finished but we still have awaiting events!");
		awaitingEvents.clear();
	}
}

bool BattleInterface::hasAnimations()
{
	return ongoingAnimationsState.get();
}

void BattleInterface::checkForAnimations()
{
	assert(!hasAnimations());
	if(hasAnimations())
		logGlobal->error("Unexpected animations state: expected all animations to be over, but some are still ongoing!");

	waitForAnimations();
}

void BattleInterface::addToAnimationStage(EAnimationEvents event, const AwaitingAnimationAction & action)
{
	awaitingEvents.push_back({action, event});
}

void BattleInterface::setBattleQueueVisibility(bool visible)
{
	windowObject->hideQueue();
	if(visible)
		windowObject->showQueue();
}
