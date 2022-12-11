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
#include "BattleControlPanel.h"
#include "BattleStacksController.h"
#include "BattleRenderer.h"

#include "../CGameInfo.h"
#include "../CMessage.h"
#include "../CMusicHandler.h"
#include "../CPlayerInterface.h"
#include "../gui/Canvas.h"
#include "../gui/CCursorHandler.h"
#include "../gui/CGuiHandler.h"
#include "../windows/CAdvmapInterface.h"

#include "../../CCallback.h"
#include "../../lib/CStack.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CHeroHandler.h"
#include "../../lib/CondSh.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/NetPacks.h"
#include "../../lib/UnlockGuard.h"

CondSh<bool> BattleInterface::animsAreDisplayed(false);
CondSh<BattleAction *> BattleInterface::givenCommand(nullptr);

BattleInterface::BattleInterface(const CCreatureSet *army1, const CCreatureSet *army2,
		const CGHeroInstance *hero1, const CGHeroInstance *hero2,
		const SDL_Rect & myRect,
		std::shared_ptr<CPlayerInterface> att, std::shared_ptr<CPlayerInterface> defen, std::shared_ptr<CPlayerInterface> spectatorInt)
	: attackingHeroInstance(hero1), defendingHeroInstance(hero2), animCount(0),
	attackerInt(att), defenderInt(defen), curInt(att),
	myTurn(false), moveStarted(false), moveSoundHander(-1), bresult(nullptr), battleActionsStarted(false)
{
	OBJ_CONSTRUCTION;

	projectilesController.reset(new BattleProjectileController(this));

	if(spectatorInt)
	{
		curInt = spectatorInt;
	}
	else if(!curInt)
	{
		//May happen when we are defending during network MP game -> attacker interface is just not present
		curInt = defenderInt;
	}

	animsAreDisplayed.setn(false);
	pos = myRect;
	strongInterest = true;
	givenCommand.setn(nullptr);

	//hot-seat -> check tactics for both players (defender may be local human)
	if(attackerInt && attackerInt->cb->battleGetTacticDist())
		tacticianInterface = attackerInt;
	else if(defenderInt && defenderInt->cb->battleGetTacticDist())
		tacticianInterface = defenderInt;

	//if we found interface of player with tactics, then enter tactics mode
	tacticsMode = static_cast<bool>(tacticianInterface);

	//create stack queue
	bool embedQueue;
	std::string queueSize = settings["battle"]["queueSize"].String();

	if(queueSize == "auto")
		embedQueue = screen->h < 700;
	else
		embedQueue = screen->h < 700 || queueSize == "small";

	queue = std::make_shared<StackQueue>(embedQueue, this);
	if(!embedQueue)
	{
		if (settings["battle"]["showQueue"].Bool())
			pos.y += queue->pos.h / 2; //center whole window

		queue->moveTo(Point(pos.x, pos.y - queue->pos.h));
	}

	//preparing siege info
	const CGTownInstance *town = curInt->cb->battleGetDefendedTown();
	if(town && town->hasFort())
		siegeController.reset(new BattleSiegeController(this, town));

	CPlayerInterface::battleInt = this;

	//initializing armies
	this->army1 = army1;
	this->army2 = army2;

	controlPanel = std::make_shared<BattleControlPanel>(this, Point(0, 556));

	//preparing menu background and terrain
	fieldController.reset( new BattleFieldController(this));
	stacksController.reset( new BattleStacksController(this));
	actionsController.reset( new BattleActionsController(this));
	effectsController.reset(new BattleEffectsController(this));

	//loading hero animations
	if(hero1) // attacking hero
	{
		std::string battleImage;
		if(!hero1->type->battleImage.empty())
		{
			battleImage = hero1->type->battleImage;
		}
		else
		{
			if(hero1->sex)
				battleImage = hero1->type->heroClass->imageBattleFemale;
			else
				battleImage = hero1->type->heroClass->imageBattleMale;
		}

		attackingHero = std::make_shared<BattleHero>(battleImage, false, hero1->tempOwner, hero1->tempOwner == curInt->playerID ? hero1 : nullptr, this);

		auto img = attackingHero->animation->getImage(0, 0, true);
		if(img)
			attackingHero->pos = genRect(img->height(), img->width(), pos.x - 43, pos.y - 19);
	}


	if(hero2) // defending hero
	{
		std::string battleImage;

		if(!hero2->type->battleImage.empty())
		{
			battleImage = hero2->type->battleImage;
		}
		else
		{
			if(hero2->sex)
				battleImage = hero2->type->heroClass->imageBattleFemale;
			else
				battleImage = hero2->type->heroClass->imageBattleMale;
		}

		defendingHero = std::make_shared<BattleHero>(battleImage, true, hero2->tempOwner, hero2->tempOwner == curInt->playerID ? hero2 : nullptr, this);

		auto img = defendingHero->animation->getImage(0, 0, true);
		if(img)
			defendingHero->pos = genRect(img->height(), img->width(), pos.x + 693, pos.y - 19);
	}

	obstacleController.reset(new BattleObstacleController(this));

	if(tacticsMode)
		tacticNextStack(nullptr);

	CCS->musich->stopMusic();
	battleIntroSoundChannel = CCS->soundh->playSoundFromSet(CCS->soundh->battleIntroSounds);
	auto onIntroPlayed = [&]()
	{
		if(LOCPLINT->battleInt)
		{
			CCS->musich->playMusicFromSet("battle", true, true);
			battleActionsStarted = true;
			activateStack();
			controlPanel->blockUI(settings["session"]["spectate"].Bool() || stacksController->getActiveStack() == nullptr);
			battleIntroSoundChannel = -1;
		}
	};

	CCS->soundh->setCallback(battleIntroSoundChannel, onIntroPlayed);

	addUsedEvents(RCLICK | MOVE | KEYBOARD);
	controlPanel->blockUI(true);
	queue->update();
}

BattleInterface::~BattleInterface()
{
	CPlayerInterface::battleInt = nullptr;
	givenCommand.cond.notify_all(); //that two lines should make any stacksController->getActiveStack() waiting thread to finish

	if (active) //dirty fix for #485
	{
		deactivate();
	}

	if (adventureInt && adventureInt->selection)
	{
		//FIXME: this should be moved to adventureInt which should restore correct track based on selection/active player
		const auto & terrain = *(LOCPLINT->cb->getTile(adventureInt->selection->visitablePos())->terType);
		CCS->musich->playMusicFromSet("terrain", terrain.name, true, false);
	}
	animsAreDisplayed.setn(false);
}

void BattleInterface::setPrintCellBorders(bool set)
{
	Settings cellBorders = settings.write["battle"]["cellBorders"];
	cellBorders->Bool() = set;

	fieldController->redrawBackgroundWithHexes();
	GH.totalRedraw();
}

void BattleInterface::setPrintStackRange(bool set)
{
	Settings stackRange = settings.write["battle"]["stackRange"];
	stackRange->Bool() = set;

	fieldController->redrawBackgroundWithHexes();
	GH.totalRedraw();
}

void BattleInterface::setPrintMouseShadow(bool set)
{
	Settings shadow = settings.write["battle"]["mouseShadow"];
	shadow->Bool() = set;
}

void BattleInterface::activate()
{
	controlPanel->activate();

	if (curInt->isAutoFightOn)
		return;

	CIntObject::activate();

	if (attackingHero)
		attackingHero->activate();
	if (defendingHero)
		defendingHero->activate();

	fieldController->activate();

	if (settings["battle"]["showQueue"].Bool())
		queue->activate();

	LOCPLINT->cingconsole->activate();
}

void BattleInterface::deactivate()
{
	controlPanel->deactivate();
	CIntObject::deactivate();

	fieldController->deactivate();

	if (attackingHero)
		attackingHero->deactivate();
	if (defendingHero)
		defendingHero->deactivate();
	if (settings["battle"]["showQueue"].Bool())
		queue->deactivate();

	LOCPLINT->cingconsole->deactivate();
}

void BattleInterface::keyPressed(const SDL_KeyboardEvent & key)
{
	if(key.keysym.sym == SDLK_q && key.state == SDL_PRESSED)
	{
		if(settings["battle"]["showQueue"].Bool()) //hide queue
			hideQueue();
		else
			showQueue();

	}
	else if(key.keysym.sym == SDLK_f && key.state == SDL_PRESSED)
	{
		actionsController->enterCreatureCastingMode();
	}
	else if(key.keysym.sym == SDLK_ESCAPE)
	{
		if(!battleActionsStarted)
			CCS->soundh->stopSound(battleIntroSoundChannel);
		else
			actionsController->endCastingSpell();
	}
}
void BattleInterface::mouseMoved(const SDL_MouseMotionEvent &event)
{
	BattleHex selectedHex = fieldController->getHoveredHex();

	actionsController->handleHex(selectedHex, MOVE);

	controlPanel->mouseMoved(event);
}

void BattleInterface::clickRight(tribool down, bool previousState)
{
	if (!down)
	{
		actionsController->endCastingSpell();
	}
}

void BattleInterface::stackReset(const CStack * stack)
{
	stacksController->stackReset(stack);
}

void BattleInterface::stackAdded(const CStack * stack)
{
	stacksController->stackAdded(stack);
}

void BattleInterface::stackRemoved(uint32_t stackID)
{
	stacksController->stackRemoved(stackID);
	fieldController->redrawBackgroundWithHexes();
	queue->update();
}

void BattleInterface::stackActivated(const CStack *stack) //TODO: check it all before game state is changed due to abilities
{
	stacksController->stackActivated(stack);
}

void BattleInterface::stackMoved(const CStack *stack, std::vector<BattleHex> destHex, int distance)
{
	stacksController->stackMoved(stack, destHex, distance);
}

void BattleInterface::stacksAreAttacked(std::vector<StackAttackedInfo> attackedInfos)
{
	stacksController->stacksAreAttacked(attackedInfos);

	std::array<int, 2> killedBySide = {0, 0};

	int targets = 0;
	for(const StackAttackedInfo & attackedInfo : attackedInfos)
	{
		++targets;

		ui8 side = attackedInfo.defender->side;
		killedBySide.at(side) += attackedInfo.amountKilled;
	}

	for(ui8 side = 0; side < 2; side++)
	{
		if(killedBySide.at(side) > killedBySide.at(1-side))
			setHeroAnimation(side, CCreatureAnim::HERO_DEFEAT);
		else if(killedBySide.at(side) < killedBySide.at(1-side))
			setHeroAnimation(side, CCreatureAnim::HERO_VICTORY);
	}
}

void BattleInterface::stackAttacking( const CStack *attacker, BattleHex dest, const CStack *attacked, bool shooting )
{
	stacksController->stackAttacking(attacker, dest, attacked, shooting);
}

void BattleInterface::newRoundFirst( int round )
{
	waitForAnims();
}

void BattleInterface::newRound(int number)
{
	controlPanel->console->addText(CGI->generaltexth->allTexts[412]);
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

	auto ba = new BattleAction(); //is deleted in CPlayerInterface::stacksController->getActiveStack()()
	ba->side = side.get();
	ba->actionType = action;
	ba->aimToHex(tile);
	ba->actionSubtype = additional;

	sendCommand(ba, actor);
}

void BattleInterface::sendCommand(BattleAction *& command, const CStack * actor)
{
	command->stackNumber = actor ? actor->unitId() : ((command->side == BattleSide::ATTACKER) ? -1 : -2);

	if(!tacticsMode)
	{
		logGlobal->trace("Setting command for %s", (actor ? actor->nodeName() : "hero"));
		myTurn = false;
		stacksController->setActiveStack(nullptr);
		givenCommand.setn(command);
	}
	else
	{
		curInt->cb->battleMakeTacticAction(command);
		vstd::clear_pointer(command);
		stacksController->setActiveStack(nullptr);
		//next stack will be activated when action ends
	}
}

const CGHeroInstance * BattleInterface::getActiveHero()
{
	const CStack *attacker = stacksController->getActiveStack();
	if(!attacker)
	{
		return nullptr;
	}

	if(attacker->side == BattleSide::ATTACKER)
	{
		return attackingHeroInstance;
	}

	return defendingHeroInstance;
}

void BattleInterface::hexLclicked(int whichOne)
{
	actionsController->handleHex(whichOne, LCLICK);
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

void BattleInterface::battleFinished(const BattleResult& br)
{
	bresult = &br;
	{
		auto unlockPim = vstd::makeUnlockGuard(*CPlayerInterface::pim);
		animsAreDisplayed.waitUntil(false);
	}
	stacksController->setActiveStack(nullptr);
	displayBattleFinished();
}

void BattleInterface::displayBattleFinished()
{
	CCS->curh->changeGraphic(ECursor::ADVENTURE,0);
	if(settings["session"]["spectate"].Bool() && settings["session"]["spectate-skip-battle-result"].Bool())
	{
		close();
		return;
	}

	GH.pushInt(std::make_shared<BattleResultWindow>(*bresult, *(this->curInt)));
	curInt->waitWhileDialog(); // Avoid freeze when AI end turn after battle. Check bug #1897
	CPlayerInterface::battleInt = nullptr;
}

void BattleInterface::spellCast(const BattleSpellCast * sc)
{
	const SpellID spellID = sc->spellID;
	const CSpell * spell = spellID.toSpell();

	assert(spell);
	if(!spell)
		return;

	const std::string & castSoundPath = spell->getCastSound();

	if (!castSoundPath.empty())
		CCS->soundh->playSound(castSoundPath);

	if ( sc->activeCast )
	{
		const CStack * casterStack = curInt->cb->battleGetStackByID(sc->casterStack);

		if(casterStack != nullptr )
		{
			displaySpellCast(spellID, casterStack->getPosition());

			stacksController->addNewAnim(new CCastAnimation(this, casterStack, sc->tile, curInt->cb->battleGetStackByPos(sc->tile), spell));
		}
		else
		if (sc->tile.isValid() && !spell->animationInfo.projectile.empty())
		{
			// this is spell cast by hero with valid destination & valid projectile -> play animation

			const CStack * target = curInt->cb->battleGetStackByPos(sc->tile);
			Point srccoord = (sc->side ? Point(770, 60) : Point(30, 60)) + pos;	//hero position
			Point destcoord = stacksController->getStackPositionAtHex(sc->tile, target); //position attacked by projectile
			destcoord += Point(250, 240); // FIXME: what are these constants?

			projectilesController->createSpellProjectile( nullptr, srccoord, destcoord, spell);
			projectilesController->emitStackProjectile( nullptr );

			// wait fo projectile to end
			stacksController->addNewAnim(new CWaitingProjectileAnimation(this, nullptr));
		}
	}

	waitForAnims(); //wait for projectile animation

	displaySpellHit(spellID, sc->tile);

	//queuing affect animation
	for(auto & elem : sc->affectedCres)
	{
		auto stack = curInt->cb->battleGetStackByID(elem, false);
		if(stack)
			displaySpellEffect(spellID, stack->getPosition());
	}

	//queuing additional animation
	for(auto & elem : sc->customEffects)
	{
		auto stack = curInt->cb->battleGetStackByID(elem.stack, false);
		if(stack)
			effectsController->displayEffect(EBattleEffect::EBattleEffect(elem.effect), stack->getPosition());
	}

	waitForAnims();
	//mana absorption
	if (sc->manaGained > 0)
	{
		Point leftHero = Point(15, 30) + pos;
		Point rightHero = Point(755, 30) + pos;
		stacksController->addNewAnim(new CPointEffectAnimation(this, soundBase::invalid, sc->side ? "SP07_A.DEF" : "SP07_B.DEF", leftHero));
		stacksController->addNewAnim(new CPointEffectAnimation(this, soundBase::invalid, sc->side ? "SP07_B.DEF" : "SP07_A.DEF", rightHero));
	}
}

void BattleInterface::battleStacksEffectsSet(const SetStackEffect & sse)
{
	if(stacksController->getActiveStack() != nullptr)
		fieldController->redrawBackgroundWithHexes();
}

void BattleInterface::setHeroAnimation(ui8 side, int phase)
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
		if(!controlPanel->console->addText(formatted))
			logGlobal->warn("Too long battle log line");
	}
}

void BattleInterface::displaySpellAnimationQueue(const CSpell::TAnimationQueue & q, BattleHex destinationTile, bool isHit)
{
	for(const CSpell::TAnimation & animation : q)
	{
		if(animation.pause > 0)
			stacksController->addNewAnim(new CDummyAnimation(this, animation.pause));
		else
		{
			int flags = 0;

			if (isHit)
				flags |= CPointEffectAnimation::FORCE_ON_TOP;

			if (animation.verticalPosition == VerticalPosition::BOTTOM)
				flags |= CPointEffectAnimation::ALIGN_TO_BOTTOM;

			if (!destinationTile.isValid())
				flags |= CPointEffectAnimation::SCREEN_FILL;

			if (!destinationTile.isValid())
				stacksController->addNewAnim(new CPointEffectAnimation(this, soundBase::invalid, animation.resourceName, flags));
			else
				stacksController->addNewAnim(new CPointEffectAnimation(this, soundBase::invalid, animation.resourceName, destinationTile, flags));
		}
	}
}

void BattleInterface::displaySpellCast(SpellID spellID, BattleHex destinationTile)
{
	const CSpell * spell = spellID.toSpell();

	if(spell)
		displaySpellAnimationQueue(spell->animationInfo.cast, destinationTile, false);
}

void BattleInterface::displaySpellEffect(SpellID spellID, BattleHex destinationTile)
{
	const CSpell *spell = spellID.toSpell();

	if(spell)
		displaySpellAnimationQueue(spell->animationInfo.affect, destinationTile, false);
}

void BattleInterface::displaySpellHit(SpellID spellID, BattleHex destinationTile)
{
	const CSpell * spell = spellID.toSpell();

	if(spell)
		displaySpellAnimationQueue(spell->animationInfo.hit, destinationTile, true);
}

void BattleInterface::setAnimSpeed(int set)
{
	Settings speed = settings.write["battle"]["animationSpeed"];
	speed->Float() = float(set) / 100;
}

int BattleInterface::getAnimSpeed() const
{
	if(settings["session"]["spectate"].Bool() && !settings["session"]["spectate-battle-speed"].isNull())
		return static_cast<int>(vstd::round(settings["session"]["spectate-battle-speed"].Float() *100));

	return static_cast<int>(vstd::round(settings["battle"]["animationSpeed"].Float() *100));
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
	if(!battleActionsStarted)
		return; //"show" function should re-call this function

	stacksController->activateStack();

	const CStack * s = stacksController->getActiveStack();
	if(!s)
		return;

	myTurn = true;
	queue->update();
	fieldController->redrawBackgroundWithHexes();
	actionsController->activateStack();
	GH.fakeMouseMove();
}

void BattleInterface::endAction(const BattleAction* action)
{
	const CStack *stack = curInt->cb->battleGetStackByID(action->stackNumber);

	if(action->actionType == EActionType::HERO_SPELL)
		setHeroAnimation(action->side, CCreatureAnim::HERO_HOLDING);

	stacksController->endAction(action);

	queue->update();

	if (tacticsMode) //stack ended movement in tactics phase -> select the next one
		tacticNextStack(stack);

	if(action->actionType == EActionType::HERO_SPELL) //we have activated next stack after sending request that has been just realized -> blockmap due to movement has changed
		fieldController->redrawBackgroundWithHexes();

//	if (stacksController->getActiveStack() && !animsAreDisplayed.get() && pendingAnims.empty() && !active)
//	{
//		logGlobal->warn("Something wrong... interface was deactivated but there is no animation. Reactivating...");
//		controlPanel->blockUI(false);
//	}
//	else
//	{
		// block UI if no active stack (e.g. enemy turn);
	controlPanel->blockUI(stacksController->getActiveStack() == nullptr);
//	}
}

void BattleInterface::hideQueue()
{
	Settings showQueue = settings.write["battle"]["showQueue"];
	showQueue->Bool() = false;

	queue->deactivate();

	if (!queue->embedded)
	{
		moveBy(Point(0, -queue->pos.h / 2));
		GH.totalRedraw();
	}
}

void BattleInterface::showQueue()
{
	Settings showQueue = settings.write["battle"]["showQueue"];
	showQueue->Bool() = true;

	queue->activate();

	if (!queue->embedded)
	{
		moveBy(Point(0, +queue->pos.h / 2));
		GH.totalRedraw();
	}
}

void BattleInterface::startAction(const BattleAction* action)
{
	controlPanel->blockUI(true);

	if(action->actionType == EActionType::END_TACTIC_PHASE)
	{
		controlPanel->tacticPhaseEnded();
		return;
	}

	const CStack *stack = curInt->cb->battleGetStackByID(action->stackNumber);

	if (stack)
	{
		queue->update();
	}
	else
	{
		assert(action->actionType == EActionType::HERO_SPELL); //only cast spell is valid action without acting stack number
	}

	stacksController->startAction(action);

	redraw(); // redraw after deactivation, including proper handling of hovered hexes

	if(action->actionType == EActionType::HERO_SPELL) //when hero casts spell
	{
		setHeroAnimation(action->side, CCreatureAnim::HERO_CAST_SPELL);
		return;
	}

	if (!stack)
	{
		logGlobal->error("Something wrong with stackNumber in actionStarted. Stack number: %d", action->stackNumber);
		return;
	}

	effectsController->startAction(action);
}

void BattleInterface::waitForAnims()
{
	auto unlockPim = vstd::makeUnlockGuard(*CPlayerInterface::pim);
	animsAreDisplayed.waitWhileTrue();
}

void BattleInterface::tacticPhaseEnd()
{
	stacksController->setActiveStack(nullptr);
	controlPanel->blockUI(true);
	tacticsMode = false;
}

static bool immobile(const CStack *s)
{
	return !s->Speed(0, true); //should bound stacks be immobile?
}

void BattleInterface::tacticNextStack(const CStack * current)
{
	if (!current)
		current = stacksController->getActiveStack();

	//no switching stacks when the current one is moving
	waitForAnims();

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

const CGHeroInstance *BattleInterface::currentHero() const
{
	if (attackingHeroInstance->tempOwner == curInt->playerID)
		return attackingHeroInstance;
	else
		return defendingHeroInstance;
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
		auto ba = make_unique<BattleAction>(curInt->autofightingAI->activeStack(stacksController->getActiveStack()));

		if(curInt->cb->battleIsFinished())
		{
			return; // battle finished with spellcast
		}

		if (curInt->isAutoFightOn)
		{
			if (tacticsMode)
			{
				// Always end tactics mode. Player interface is blocked currently, so it's not possible that
				// the AI can take any action except end tactics phase (AI actions won't be triggered)
				//TODO implement the possibility that the AI will be triggered for further actions
				//TODO any solution to merge tactics phase & normal phase in the way it is handled by the player and battle interface?
				stacksController->setActiveStack(nullptr);
				controlPanel->blockUI(true);
				tacticsMode = false;
			}
			else
			{
				givenCommand.setn(ba.release());
			}
		}
		else
		{
			boost::unique_lock<boost::recursive_mutex> un(*CPlayerInterface::pim);
			activateStack();
		}
	});

	aiThread.detach();
}

void BattleInterface::showAll(SDL_Surface *to)
{
	show(to);
}

void BattleInterface::show(SDL_Surface *to)
{
	Canvas canvas(to);
	assert(to);

	SDL_Rect buf;
	SDL_GetClipRect(to, &buf);
	SDL_SetClipRect(to, &pos);

	++animCount;

	fieldController->renderBattlefield(canvas);

	if(battleActionsStarted)
		stacksController->updateBattleAnimations();

	SDL_SetClipRect(to, &buf); //restoring previous clip_rect

	showInterface(to);

	//activation of next stack, if any
	//TODO: should be moved to the very start of this method?
	//activateStack();
}

void BattleInterface::collectRenderableObjects(BattleRenderer & renderer)
{
	if (attackingHero)
	{
		renderer.insert(EBattleFieldLayer::HEROES, BattleHex(0),[this](BattleRenderer::RendererPtr canvas)
		{
			attackingHero->render(canvas);
		});
	}
	if (defendingHero)
	{
		renderer.insert(EBattleFieldLayer::HEROES, BattleHex(GameConstants::BFIELD_WIDTH-1),[this](BattleRenderer::RendererPtr canvas)
		{
			defendingHero->render(canvas);
		});
	}
}

void BattleInterface::showInterface(SDL_Surface * to)
{
	//showing in-game console
	LOCPLINT->cingconsole->show(to);
	controlPanel->showAll(to);

	Rect posWithQueue = Rect(pos.x, pos.y, 800, 600);

	if (settings["battle"]["showQueue"].Bool())
	{
		if (!queue->embedded)
		{
			posWithQueue.y -= queue->pos.h;
			posWithQueue.h += queue->pos.h;
		}

		queue->showAll(to);
	}

	//printing border around interface
	if (screen->w != 800 || screen->h !=600)
	{
		CMessage::drawBorder(curInt->playerID,to,posWithQueue.w + 28, posWithQueue.h + 28, posWithQueue.x-14, posWithQueue.y-15);
	}
}

void BattleInterface::castThisSpell(SpellID spellID)
{
	actionsController->castThisSpell(spellID);
}
