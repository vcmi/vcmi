/*
 * CBattleInterface.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CBattleInterface.h"

#include "CBattleAnimations.h"
#include "CBattleActionsController.h"
#include "CBattleInterfaceClasses.h"
#include "CCreatureAnimation.h"
#include "CBattleProjectileController.h"
#include "CBattleObstacleController.h"
#include "CBattleSiegeController.h"
#include "CBattleFieldController.h"
#include "CBattleControlPanel.h"
#include "CBattleStacksController.h"

#include "../CBitmapHandler.h"
#include "../CGameInfo.h"
#include "../CMessage.h"
#include "../CMT.h"
#include "../CMusicHandler.h"
#include "../CPlayerInterface.h"
#include "../CVideoHandler.h"
#include "../Graphics.h"
#include "../gui/CAnimation.h"
#include "../gui/CCursorHandler.h"
#include "../gui/CGuiHandler.h"
#include "../gui/SDL_Extensions.h"
#include "../windows/CAdvmapInterface.h"
#include "../windows/CCreatureWindow.h"
#include "../windows/CSpellWindow.h"

#include "../../CCallback.h"
#include "../../lib/CStack.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CHeroHandler.h"
#include "../../lib/CondSh.h"
#include "../../lib/CRandomGenerator.h"
#include "../../lib/spells/CSpellHandler.h"
#include "../../lib/spells/ISpellMechanics.h"
#include "../../lib/spells/Problem.h"
#include "../../lib/CTownHandler.h"
#include "../../lib/BattleFieldHandler.h"
#include "../../lib/ObstacleHandler.h"
#include "../../lib/CGameState.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/NetPacks.h"
#include "../../lib/UnlockGuard.h"

CondSh<bool> CBattleInterface::animsAreDisplayed(false);
CondSh<BattleAction *> CBattleInterface::givenCommand(nullptr);

CBattleInterface::CBattleInterface(const CCreatureSet *army1, const CCreatureSet *army2,
		const CGHeroInstance *hero1, const CGHeroInstance *hero2,
		const SDL_Rect & myRect,
		std::shared_ptr<CPlayerInterface> att, std::shared_ptr<CPlayerInterface> defen, std::shared_ptr<CPlayerInterface> spectatorInt)
	: attackingHeroInstance(hero1), defendingHeroInstance(hero2), animCount(0),
	attackerInt(att), defenderInt(defen), curInt(att),
	myTurn(false), moveStarted(false), moveSoundHander(-1), bresult(nullptr), battleActionsStarted(false)
{
	OBJ_CONSTRUCTION;

	projectilesController.reset(new CBattleProjectileController(this));

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

	queue = std::make_shared<CStackQueue>(embedQueue, this);
	if(!embedQueue)
	{
		if (settings["battle"]["showQueue"].Bool())
			pos.y += queue->pos.h / 2; //center whole window

		queue->moveTo(Point(pos.x, pos.y - queue->pos.h));
	}
	queue->update();

	//preparing siege info
	const CGTownInstance *town = curInt->cb->battleGetDefendedTown();
	if(town && town->hasFort())
		siegeController.reset(new CBattleSiegeController(this, town));

	CPlayerInterface::battleInt = this;

	//initializing armies
	this->army1 = army1;
	this->army2 = army2;

	//preparing menu background and terrain
	fieldController.reset( new CBattleFieldController(this));
	stacksController.reset( new CBattleStacksController(this));
	actionsController.reset( new CBattleActionsController(this));

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

		attackingHero = std::make_shared<CBattleHero>(battleImage, false, hero1->tempOwner, hero1->tempOwner == curInt->playerID ? hero1 : nullptr, this);

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

		defendingHero = std::make_shared<CBattleHero>(battleImage, true, hero2->tempOwner, hero2->tempOwner == curInt->playerID ? hero2 : nullptr, this);

		auto img = defendingHero->animation->getImage(0, 0, true);
		if(img)
			defendingHero->pos = genRect(img->height(), img->width(), pos.x + 693, pos.y - 19);
	}

	obstacleController.reset(new CBattleObstacleController(this));

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
			controlPanel->blockUI(settings["session"]["spectate"].Bool());
			battleIntroSoundChannel = -1;
		}
	};

	CCS->soundh->setCallback(battleIntroSoundChannel, onIntroPlayed);

	addUsedEvents(RCLICK | MOVE | KEYBOARD);
	controlPanel->blockUI(true);
}

CBattleInterface::~CBattleInterface()
{
	CPlayerInterface::battleInt = nullptr;
	givenCommand.cond.notify_all(); //that two lines should make any stacksController->getActiveStack() waiting thread to finish

	if (active) //dirty fix for #485
	{
		deactivate();
	}

	//TODO: play AI tracks if battle was during AI turn
	//if (!curInt->makingTurn)
	//CCS->musich->playMusicFromSet(CCS->musich->aiMusics, -1);

	if (adventureInt && adventureInt->selection)
	{
		const auto & terrain = *(LOCPLINT->cb->getTile(adventureInt->selection->visitablePos())->terType);
		CCS->musich->playMusicFromSet("terrain", terrain.name, true, false);
	}
	animsAreDisplayed.setn(false);
}

void CBattleInterface::setPrintCellBorders(bool set)
{
	Settings cellBorders = settings.write["battle"]["cellBorders"];
	cellBorders->Bool() = set;

	fieldController->redrawBackgroundWithHexes();
	GH.totalRedraw();
}

void CBattleInterface::setPrintStackRange(bool set)
{
	Settings stackRange = settings.write["battle"]["stackRange"];
	stackRange->Bool() = set;

	fieldController->redrawBackgroundWithHexes();
	GH.totalRedraw();
}

void CBattleInterface::setPrintMouseShadow(bool set)
{
	Settings shadow = settings.write["battle"]["mouseShadow"];
	shadow->Bool() = set;
}

void CBattleInterface::activate()
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

void CBattleInterface::deactivate()
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

void CBattleInterface::keyPressed(const SDL_KeyboardEvent & key)
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
void CBattleInterface::mouseMoved(const SDL_MouseMotionEvent &sEvent)
{
	BattleHex selectedHex = fieldController->getHoveredHex();

	actionsController->handleHex(selectedHex, MOVE);
}

void CBattleInterface::clickRight(tribool down, bool previousState)
{
	if (!down)
	{
		actionsController->endCastingSpell();
	}
}

void CBattleInterface::stackReset(const CStack * stack)
{
	stacksController->stackReset(stack);
}

void CBattleInterface::stackAdded(const CStack * stack)
{
	stacksController->stackAdded(stack);
}

void CBattleInterface::stackRemoved(uint32_t stackID)
{
	stacksController->stackRemoved(stackID);
	fieldController->redrawBackgroundWithHexes();
	queue->update();
}

void CBattleInterface::stackActivated(const CStack *stack) //TODO: check it all before game state is changed due to abilities
{
	stacksController->stackActivated(stack);
}

void CBattleInterface::stackMoved(const CStack *stack, std::vector<BattleHex> destHex, int distance)
{
	stacksController->stackMoved(stack, destHex, distance);
}

void CBattleInterface::stacksAreAttacked(std::vector<StackAttackedInfo> attackedInfos)
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
			setHeroAnimation(side, 2);
		else if(killedBySide.at(side) < killedBySide.at(1-side))
			setHeroAnimation(side, 3);
	}
}

void CBattleInterface::stackAttacking( const CStack *attacker, BattleHex dest, const CStack *attacked, bool shooting )
{
	stacksController->stackAttacking(attacker, dest, attacked, shooting);
}

void CBattleInterface::newRoundFirst( int round )
{
	waitForAnims();
}

void CBattleInterface::newRound(int number)
{
	controlPanel->console->addText(CGI->generaltexth->allTexts[412]);
}

void CBattleInterface::giveCommand(EActionType action, BattleHex tile, si32 additional)
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

void CBattleInterface::sendCommand(BattleAction *& command, const CStack * actor)
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

const CGHeroInstance * CBattleInterface::getActiveHero()
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

void CBattleInterface::hexLclicked(int whichOne)
{
	actionsController->handleHex(whichOne, LCLICK);
}

void CBattleInterface::stackIsCatapulting(const CatapultAttack & ca)
{
	if (siegeController)
		siegeController->stackIsCatapulting(ca);
}

void CBattleInterface::gateStateChanged(const EGateState state)
{
	if (siegeController)
		siegeController->gateStateChanged(state);
}

void CBattleInterface::battleFinished(const BattleResult& br)
{
	bresult = &br;
	{
		auto unlockPim = vstd::makeUnlockGuard(*CPlayerInterface::pim);
		animsAreDisplayed.waitUntil(false);
	}
	stacksController->setActiveStack(nullptr);
	displayBattleFinished();
}

void CBattleInterface::displayBattleFinished()
{
	CCS->curh->changeGraphic(ECursor::ADVENTURE,0);
	if(settings["session"]["spectate"].Bool() && settings["session"]["spectate-skip-battle-result"].Bool())
	{
		close();
		return;
	}

	GH.pushInt(std::make_shared<CBattleResultWindow>(*bresult, *(this->curInt)));
	curInt->waitWhileDialog(); // Avoid freeze when AI end turn after battle. Check bug #1897
	CPlayerInterface::battleInt = nullptr;
}

void CBattleInterface::spellCast(const BattleSpellCast * sc)
{
	const SpellID spellID = sc->spellID;
	const CSpell * spell = spellID.toSpell();

	if(!spell)
		return;

	const std::string & castSoundPath = spell->getCastSound();

	if (!castSoundPath.empty())
		CCS->soundh->playSound(castSoundPath);

	const auto casterStackID = sc->casterStack;
	const CStack * casterStack = nullptr;
	if(casterStackID >= 0)
	{
		casterStack = curInt->cb->battleGetStackByID(casterStackID);
	}

	Point srccoord = (sc->side ? Point(770, 60) : Point(30, 60)) + pos;	//hero position by default
	{
		if(casterStack != nullptr)
		{
			srccoord = CClickableHex::getXYUnitAnim(casterStack->getPosition(), casterStack, this);
			srccoord.x += 250;
			srccoord.y += 240;
		}
	}

	if(casterStack != nullptr && sc->activeCast)
	{
		//todo: custom cast animation for hero
		displaySpellCast(spellID, casterStack->getPosition());

		stacksController->addNewAnim(new CCastAnimation(this, casterStack, sc->tile, curInt->cb->battleGetStackByPos(sc->tile)));
	}

	waitForAnims(); //wait for cast animation

	//playing projectile animation
	if (sc->tile.isValid())
	{
		Point destcoord = CClickableHex::getXYUnitAnim(sc->tile, curInt->cb->battleGetStackByPos(sc->tile), this); //position attacked by projectile
		destcoord.x += 250; destcoord.y += 240;

		//animation angle
		double angle = atan2(static_cast<double>(destcoord.x - srccoord.x), static_cast<double>(destcoord.y - srccoord.y));
		bool Vflip = (angle < 0);
		if (Vflip)
			angle = -angle;

		std::string animToDisplay = spell->animationInfo.selectProjectile(angle);

		if(!animToDisplay.empty())
		{
			//TODO: calculate inside CEffectAnimation
			std::shared_ptr<CAnimation> tmp = std::make_shared<CAnimation>(animToDisplay);
			tmp->load(0, 0);
			auto first = tmp->getImage(0, 0);

			//displaying animation
			double diffX = (destcoord.x - srccoord.x)*(destcoord.x - srccoord.x);
			double diffY = (destcoord.y - srccoord.y)*(destcoord.y - srccoord.y);
			double distance = sqrt(diffX + diffY);

			int steps = static_cast<int>(distance / AnimationControls::getSpellEffectSpeed() + 1);
			int dx = (destcoord.x - srccoord.x - first->width())/steps;
			int dy = (destcoord.y - srccoord.y - first->height())/steps;

			stacksController->addNewAnim(new CEffectAnimation(this, animToDisplay, srccoord.x, srccoord.y, dx, dy, Vflip));
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
			displayEffect(elem.effect, stack->getPosition());
	}

	waitForAnims();
	//mana absorption
	if (sc->manaGained > 0)
	{
		Point leftHero = Point(15, 30) + pos;
		Point rightHero = Point(755, 30) + pos;
		stacksController->addNewAnim(new CEffectAnimation(this, sc->side ? "SP07_A.DEF" : "SP07_B.DEF", leftHero.x, leftHero.y, 0, 0, false));
		stacksController->addNewAnim(new CEffectAnimation(this, sc->side ? "SP07_B.DEF" : "SP07_A.DEF", rightHero.x, rightHero.y, 0, 0, false));
	}
}

void CBattleInterface::battleStacksEffectsSet(const SetStackEffect & sse)
{
	if(stacksController->getActiveStack() != nullptr)
		fieldController->redrawBackgroundWithHexes();
}

void CBattleInterface::setHeroAnimation(ui8 side, int phase)
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

void CBattleInterface::displayBattleLog(const std::vector<MetaString> & battleLog)
{
	for(const auto & line : battleLog)
	{
		std::string formatted = line.toString();
		boost::algorithm::trim(formatted);
		if(!controlPanel->console->addText(formatted))
			logGlobal->warn("Too long battle log line");
	}
}

void CBattleInterface::displayCustomEffects(const std::vector<CustomEffectInfo> & customEffects)
{
	for(const CustomEffectInfo & one : customEffects)
	{
		if(one.sound != 0)
			CCS->soundh->playSound(soundBase::soundID(one.sound));
		const CStack * s = curInt->cb->battleGetStackByID(one.stack, false);
		if(s && one.effect != 0)
			displayEffect(one.effect, s->getPosition());
	}
}

void CBattleInterface::displayEffect(ui32 effect, BattleHex destTile)
{
	std::string customAnim = graphics->battleACToDef[effect][0];

	stacksController->addNewAnim(new CEffectAnimation(this, customAnim, destTile));
}

void CBattleInterface::displaySpellAnimationQueue(const CSpell::TAnimationQueue & q, BattleHex destinationTile)
{
	for(const CSpell::TAnimation & animation : q)
	{
		if(animation.pause > 0)
			stacksController->addNewAnim(new CDummyAnimation(this, animation.pause));
		else
			stacksController->addNewAnim(new CEffectAnimation(this, animation.resourceName, destinationTile, false, animation.verticalPosition == VerticalPosition::BOTTOM));
	}
}

void CBattleInterface::displaySpellCast(SpellID spellID, BattleHex destinationTile)
{
	const CSpell * spell = spellID.toSpell();

	if(spell)
		displaySpellAnimationQueue(spell->animationInfo.cast, destinationTile);
}

void CBattleInterface::displaySpellEffect(SpellID spellID, BattleHex destinationTile)
{
	const CSpell *spell = spellID.toSpell();

	if(spell)
		displaySpellAnimationQueue(spell->animationInfo.affect, destinationTile);
}

void CBattleInterface::displaySpellHit(SpellID spellID, BattleHex destinationTile)
{
	const CSpell * spell = spellID.toSpell();

	if(spell)
		displaySpellAnimationQueue(spell->animationInfo.hit, destinationTile);
}

void CBattleInterface::battleTriggerEffect(const BattleTriggerEffect & bte)
{
	const CStack * stack = curInt->cb->battleGetStackByID(bte.stackID);
	if(!stack)
	{
		logGlobal->error("Invalid stack ID %d", bte.stackID);
		return;
	}
	//don't show animation when no HP is regenerated
	switch(bte.effect)
	{
		//TODO: move to bonus type handler
		case Bonus::HP_REGENERATION:
			displayEffect(74, stack->getPosition());
			CCS->soundh->playSound(soundBase::REGENER);
			break;
		case Bonus::MANA_DRAIN:
			displayEffect(77, stack->getPosition());
			CCS->soundh->playSound(soundBase::MANADRAI);
			break;
		case Bonus::POISON:
			displayEffect(67, stack->getPosition());
			CCS->soundh->playSound(soundBase::POISON);
			break;
		case Bonus::FEAR:
			displayEffect(15, stack->getPosition());
			CCS->soundh->playSound(soundBase::FEAR);
			break;
		case Bonus::MORALE:
		{
			std::string hlp = CGI->generaltexth->allTexts[33];
			boost::algorithm::replace_first(hlp,"%s",(stack->getName()));
			displayEffect(20,stack->getPosition());
			CCS->soundh->playSound(soundBase::GOODMRLE);
			controlPanel->console->addText(hlp);
			break;
		}
		default:
			return;
	}
	//waitForAnims(); //fixme: freezes game :?
}

void CBattleInterface::setAnimSpeed(int set)
{
	Settings speed = settings.write["battle"]["animationSpeed"];
	speed->Float() = float(set) / 100;
}

int CBattleInterface::getAnimSpeed() const
{
	if(settings["session"]["spectate"].Bool() && !settings["session"]["spectate-battle-speed"].isNull())
		return static_cast<int>(vstd::round(settings["session"]["spectate-battle-speed"].Float() *100));

	return static_cast<int>(vstd::round(settings["battle"]["animationSpeed"].Float() *100));
}

CPlayerInterface *CBattleInterface::getCurrentPlayerInterface() const
{
	return curInt.get();
}

void CBattleInterface::trySetActivePlayer( PlayerColor player )
{
	if ( attackerInt && attackerInt->playerID == player )
		curInt = attackerInt;

	if ( defenderInt && defenderInt->playerID == player )
		curInt = defenderInt;
}

void CBattleInterface::activateStack()
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

void CBattleInterface::endAction(const BattleAction* action)
{
	const CStack *stack = curInt->cb->battleGetStackByID(action->stackNumber);

	if(action->actionType == EActionType::HERO_SPELL)
		setHeroAnimation(action->side, 0);

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

void CBattleInterface::hideQueue()
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

void CBattleInterface::showQueue()
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

void CBattleInterface::startAction(const BattleAction* action)
{
	//setActiveStack(nullptr);
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
		setHeroAnimation(action->side, 4);
		return;
	}

	if (!stack)
	{
		logGlobal->error("Something wrong with stackNumber in actionStarted. Stack number: %d", action->stackNumber);
		return;
	}

	int txtid = 0;
	switch(action->actionType)
	{
	case EActionType::WAIT:
		txtid = 136;
		break;
	case EActionType::BAD_MORALE:
		txtid = -34; //negative -> no separate singular/plural form
		displayEffect(30, stack->getPosition());
		CCS->soundh->playSound(soundBase::BADMRLE);
		break;
	}

	if(txtid != 0)
		controlPanel->console->addText(stack->formatGeneralMessage(txtid));

	//displaying special abilities
	auto actionTarget = action->getTarget(curInt->cb.get());
	switch(action->actionType)
	{
		case EActionType::STACK_HEAL:
			displayEffect(74, actionTarget.at(0).hexValue);
			CCS->soundh->playSound(soundBase::REGENER);
			break;
	}
}

void CBattleInterface::waitForAnims()
{
	auto unlockPim = vstd::makeUnlockGuard(*CPlayerInterface::pim);
	animsAreDisplayed.waitWhileTrue();
}

void CBattleInterface::tacticPhaseEnd()
{
	stacksController->setActiveStack(nullptr);
	controlPanel->blockUI(true);
	tacticsMode = false;
}

static bool immobile(const CStack *s)
{
	return !s->Speed(0, true); //should bound stacks be immobile?
}

void CBattleInterface::tacticNextStack(const CStack * current)
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

void CBattleInterface::obstaclePlaced(const CObstacleInstance & oi)
{
	obstacleController->obstaclePlaced(oi);
}

const CGHeroInstance *CBattleInterface::currentHero() const
{
	if (attackingHeroInstance->tempOwner == curInt->playerID)
		return attackingHeroInstance;
	else
		return defendingHeroInstance;
}

InfoAboutHero CBattleInterface::enemyHero() const
{
	InfoAboutHero ret;
	if (attackingHeroInstance->tempOwner == curInt->playerID)
		curInt->cb->getHeroInfo(defendingHeroInstance, ret);
	else
		curInt->cb->getHeroInfo(attackingHeroInstance, ret);

	return ret;
}

void CBattleInterface::requestAutofightingAIToTakeAction()
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

void CBattleInterface::showAll(SDL_Surface *to)
{
	show(to);
}

void CBattleInterface::show(SDL_Surface *to)
{
	assert(to);

	SDL_Rect buf;
	SDL_GetClipRect(to, &buf);
	SDL_SetClipRect(to, &pos);

	++animCount;

	if (stacksController->getActiveStack() != nullptr /*&& creAnims[stacksController->getActiveStack()->ID]->isIdle()*/) //show everything with range
	{
		// FIXME: any *real* reason to keep this separate? Speed difference can't be that big // TODO: move to showAll?
		fieldController->showBackgroundImageWithHexes(to);
	}
	else
	{
		fieldController->showBackgroundImage(to);
		obstacleController->showAbsoluteObstacles(to);
		if ( siegeController )
			siegeController->showAbsoluteObstacles(to);
	}
	fieldController->showHighlightedHexes(to);

	showBattlefieldObjects(to);
	projectilesController->showProjectiles(to);

	if(battleActionsStarted)
		stacksController->updateBattleAnimations();

	SDL_SetClipRect(to, &buf); //restoring previous clip_rect

	showInterface(to);

	//activation of next stack, if any
	//TODO: should be moved to the very start of this method?
	activateStack();
}

void CBattleInterface::showBattlefieldObjects(SDL_Surface *to)
{
	auto showHexEntry = [&](BattleObjectsByHex::HexData & hex)
	{
		if (siegeController)
			siegeController->showPiecesOfWall(to, hex.walls);
		obstacleController->showObstacles(to, hex.obstacles);
		stacksController->showAliveStacks(to, hex.alive);
		showBattleEffects(to, hex.effects);
	};

	BattleObjectsByHex objects = sortObjectsByHex();

	// dead stacks should be blit first
	stacksController->showStacks(to, objects.beforeAll.dead);
	for (auto & data : objects.hex)
		stacksController->showStacks(to, data.dead);
	stacksController->showStacks(to, objects.afterAll.dead);

	// display objects that must be blit before anything else (e.g. topmost walls)
	showHexEntry(objects.beforeAll);

	// show heroes after "beforeAll" - e.g. topmost wall in siege
	if (attackingHero)
		attackingHero->show(to);
	if (defendingHero)
		defendingHero->show(to);

	// actual blit of most of objects, hex by hex
	// NOTE: row-by-row blitting may be a better approach
	for (auto &data : objects.hex)
		showHexEntry(data);

	// objects that must be blit *after* everything else - e.g. bottom tower or some spell effects
	showHexEntry(objects.afterAll);
}

void CBattleInterface::showBattleEffects(SDL_Surface *to, const std::vector<const BattleEffect *> &battleEffects)
{
	for (auto & elem : battleEffects)
	{
		int currentFrame = static_cast<int>(floor(elem->currentFrame));
		currentFrame %= elem->animation->size();

		auto img = elem->animation->getImage(currentFrame);

		SDL_Rect temp_rect = genRect(img->height(), img->width(), elem->x, elem->y);

		img->draw(to, &temp_rect, nullptr);
	}
}

void CBattleInterface::showInterface(SDL_Surface *to)
{
	//showing in-game console
	LOCPLINT->cingconsole->show(to);
	controlPanel->show(to);

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

BattleObjectsByHex CBattleInterface::sortObjectsByHex()
{
	BattleObjectsByHex sorted;

	// Sort creatures
	stacksController->sortObjectsByHex(sorted);

	// Sort battle effects (spells)
	for (auto & battleEffect : battleEffects)
	{
		if (battleEffect.position.isValid())
			sorted.hex[battleEffect.position].effects.push_back(&battleEffect);
		else
			sorted.afterAll.effects.push_back(&battleEffect);
	}

	// Sort obstacles
	obstacleController->sortObjectsByHex(sorted);

	// Sort wall parts
	if (siegeController)
		siegeController->sortObjectsByHex(sorted);

	return sorted;
}

void CBattleInterface::castThisSpell(SpellID spellID)
{
	actionsController->castThisSpell(spellID);
}
