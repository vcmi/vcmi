/*
 * BattleWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleWindow.h"

#include "BattleInterface.h"
#include "BattleInterfaceClasses.h"
#include "BattleFieldController.h"
#include "BattleStacksController.h"
#include "BattleActionsController.h"

#include "../CGameInfo.h"
#include "../CPlayerInterface.h"
#include "../CMusicHandler.h"
#include "../gui/CursorHandler.h"
#include "../gui/CGuiHandler.h"
#include "../windows/CSpellWindow.h"
#include "../widgets/Buttons.h"
#include "../widgets/Images.h"
#include "../windows/CMessage.h"
#include "../render/CAnimation.h"
#include "../render/Canvas.h"
#include "../adventureMap/CInGameConsole.h"

#include "../../CCallback.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/CStack.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/filesystem/ResourceID.h"
#include "../windows/settings/SettingsMainWindow.h"

BattleWindow::BattleWindow(BattleInterface & owner):
	owner(owner),
	defaultAction(PossiblePlayerBattleAction::INVALID)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	pos.w = 800;
	pos.h = 600;
	pos = center();

	REGISTER_BUILDER("battleConsole", &BattleWindow::buildBattleConsole);
	
	const JsonNode config(ResourceID("config/widgets/BattleWindow.json"));
	
	addCallback("options", std::bind(&BattleWindow::bOptionsf, this));
	addCallback("surrender", std::bind(&BattleWindow::bSurrenderf, this));
	addCallback("flee", std::bind(&BattleWindow::bFleef, this));
	addCallback("autofight", std::bind(&BattleWindow::bAutofightf, this));
	addCallback("spellbook", std::bind(&BattleWindow::bSpellf, this));
	addCallback("wait", std::bind(&BattleWindow::bWaitf, this));
	addCallback("defence", std::bind(&BattleWindow::bDefencef, this));
	addCallback("consoleUp", std::bind(&BattleWindow::bConsoleUpf, this));
	addCallback("consoleDown", std::bind(&BattleWindow::bConsoleDownf, this));
	addCallback("tacticNext", std::bind(&BattleWindow::bTacticNextStack, this));
	addCallback("tacticEnd", std::bind(&BattleWindow::bTacticPhaseEnd, this));
	addCallback("alternativeAction", std::bind(&BattleWindow::bSwitchActionf, this));
	
	build(config);
	
	console = widget<BattleConsole>("console");

	GH.statusbar = console;
	owner.console = console;

	owner.fieldController.reset( new BattleFieldController(owner));
	owner.fieldController->createHeroes();

	createQueue();

	if ( owner.tacticsMode )
		tacticPhaseStarted();
	else
		tacticPhaseEnded();

	addUsedEvents(RCLICK | KEYBOARD);
}

void BattleWindow::createQueue()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	//create stack queue and adjust our own position
	bool embedQueue;
	bool showQueue = settings["battle"]["showQueue"].Bool();
	std::string queueSize = settings["battle"]["queueSize"].String();

	if(queueSize == "auto")
		embedQueue = GH.screenDimensions().y < 700;
	else
		embedQueue = GH.screenDimensions().y < 700 || queueSize == "small";

	queue = std::make_shared<StackQueue>(embedQueue, owner);
	if(!embedQueue && showQueue)
	{
		//re-center, taking into account stack queue position
		pos.y -= queue->pos.h;
		pos.h += queue->pos.h;
		pos = center();
	}

	if (!showQueue)
		queue->disable();
}

BattleWindow::~BattleWindow()
{
	CPlayerInterface::battleInt = nullptr;
}

std::shared_ptr<BattleConsole> BattleWindow::buildBattleConsole(const JsonNode & config) const
{
	auto rect = readRect(config["rect"]);
	auto offset = readPosition(config["imagePosition"]);
	auto background = widget<CPicture>("menuBattle");
	return std::make_shared<BattleConsole>(background, rect.topLeft(), offset, rect.dimensions() );
}

void BattleWindow::toggleQueueVisibility()
{
	if(settings["battle"]["showQueue"].Bool())
		hideQueue();
	else
		showQueue();
}

void BattleWindow::hideQueue()
{
	if(settings["battle"]["showQueue"].Bool() == false)
		return;

	Settings showQueue = settings.write["battle"]["showQueue"];
	showQueue->Bool() = false;

	queue->disable();

	if (!queue->embedded)
	{
		//re-center, taking into account stack queue position
		pos.y += queue->pos.h;
		pos.h -= queue->pos.h;
		pos = center();
	}
	GH.totalRedraw();
}

void BattleWindow::showQueue()
{
	if(settings["battle"]["showQueue"].Bool() == true)
		return;

	Settings showQueue = settings.write["battle"]["showQueue"];
	showQueue->Bool() = true;

	createQueue();
	updateQueue();
	GH.totalRedraw();
}

void BattleWindow::updateQueue()
{
	queue->update();
}

void BattleWindow::activate()
{
	GH.statusbar = console;
	CIntObject::activate();
	LOCPLINT->cingconsole->activate();
}

void BattleWindow::deactivate()
{
	CIntObject::deactivate();
	LOCPLINT->cingconsole->deactivate();
}

void BattleWindow::keyPressed(const SDL_Keycode & key)
{
	if (owner.openingPlaying())
	{
		owner.openingEnd();
		return;
	}

	if(key == SDLK_q)
	{
		toggleQueueVisibility();
	}
	else if(key == SDLK_f)
	{
		owner.actionsController->enterCreatureCastingMode();
	}
	else if(key == SDLK_ESCAPE)
	{
		owner.actionsController->endCastingSpell();
	}
	else if(GH.isKeyboardShiftDown())
	{
		// save and activate setting
		Settings movementHighlightOnHover = settings.write["battle"]["movementHighlightOnHover"];
		movementHighlightOnHoverCache = movementHighlightOnHover->Bool();
		movementHighlightOnHover->Bool() = true;
	}
}

void BattleWindow::keyReleased(const SDL_Keycode & key)
{
	if(!GH.isKeyboardShiftDown())
	{
		// set back to initial state
		Settings movementHighlightOnHover = settings.write["battle"]["movementHighlightOnHover"];
		movementHighlightOnHover->Bool() = movementHighlightOnHoverCache;
	}
}

void BattleWindow::clickRight(tribool down, bool previousState)
{
	if (!down)
		owner.actionsController->endCastingSpell();
}

void BattleWindow::tacticPhaseStarted()
{
	auto menuBattle = widget<CIntObject>("menuBattle");
	auto console = widget<CIntObject>("console");
	auto menuTactics = widget<CIntObject>("menuTactics");
	auto tacticNext = widget<CIntObject>("tacticNext");
	auto tacticEnd = widget<CIntObject>("tacticEnd");
	auto alternativeAction = widget<CIntObject>("alternativeAction");

	menuBattle->disable();
	console->disable();
	if (alternativeAction)
		alternativeAction->disable();

	menuTactics->enable();
	tacticNext->enable();
	tacticEnd->enable();

	redraw();
}

void BattleWindow::tacticPhaseEnded()
{
	auto menuBattle = widget<CIntObject>("menuBattle");
	auto console = widget<CIntObject>("console");
	auto menuTactics = widget<CIntObject>("menuTactics");
	auto tacticNext = widget<CIntObject>("tacticNext");
	auto tacticEnd = widget<CIntObject>("tacticEnd");
	auto alternativeAction = widget<CIntObject>("alternativeAction");

	menuBattle->enable();
	console->enable();
	if (alternativeAction)
		alternativeAction->enable();

	menuTactics->disable();
	tacticNext->disable();
	tacticEnd->disable();

	redraw();
}

void BattleWindow::bOptionsf()
{
	if (owner.actionsController->spellcastingModeActive())
		return;

	CCS->curh->set(Cursor::Map::POINTER);

	GH.pushIntT<SettingsMainWindow>(&owner);
}

void BattleWindow::bSurrenderf()
{
	if (owner.actionsController->spellcastingModeActive())
		return;

	int cost = owner.curInt->cb->battleGetSurrenderCost();
	if(cost >= 0)
	{
		std::string enemyHeroName = owner.curInt->cb->battleGetEnemyHero().name;
		if(enemyHeroName.empty())
		{
			logGlobal->warn("Surrender performed without enemy hero, should not happen!");
			enemyHeroName = "#ENEMY#";
		}

		std::string surrenderMessage = boost::str(boost::format(CGI->generaltexth->allTexts[32]) % enemyHeroName % cost); //%s states: "I will accept your surrender and grant you and your troops safe passage for the price of %d gold."
		owner.curInt->showYesNoDialog(surrenderMessage, [this](){ reallySurrender(); }, nullptr);
	}
}

void BattleWindow::bFleef()
{
	if (owner.actionsController->spellcastingModeActive())
		return;

	if ( owner.curInt->cb->battleCanFlee() )
	{
		CFunctionList<void()> ony = std::bind(&BattleWindow::reallyFlee,this);
		owner.curInt->showYesNoDialog(CGI->generaltexth->allTexts[28], ony, nullptr); //Are you sure you want to retreat?
	}
	else
	{
		std::vector<std::shared_ptr<CComponent>> comps;
		std::string heroName;
		//calculating fleeing hero's name
		if (owner.attackingHeroInstance)
			if (owner.attackingHeroInstance->tempOwner == owner.curInt->cb->getMyColor())
				heroName = owner.attackingHeroInstance->getNameTranslated();
		if (owner.defendingHeroInstance)
			if (owner.defendingHeroInstance->tempOwner == owner.curInt->cb->getMyColor())
				heroName = owner.defendingHeroInstance->getNameTranslated();
		//calculating text
		auto txt = boost::format(CGI->generaltexth->allTexts[340]) % heroName; //The Shackles of War are present.  %s can not retreat!

		//printing message
		owner.curInt->showInfoDialog(boost::to_string(txt), comps);
	}
}

void BattleWindow::reallyFlee()
{
	owner.giveCommand(EActionType::RETREAT);
	CCS->curh->set(Cursor::Map::POINTER);
}

void BattleWindow::reallySurrender()
{
	if (owner.curInt->cb->getResourceAmount(EGameResID::GOLD) < owner.curInt->cb->battleGetSurrenderCost())
	{
		owner.curInt->showInfoDialog(CGI->generaltexth->allTexts[29]); //You don't have enough gold!
	}
	else
	{
		owner.giveCommand(EActionType::SURRENDER);
		CCS->curh->set(Cursor::Map::POINTER);
	}
}

void BattleWindow::showAlternativeActionIcon(PossiblePlayerBattleAction action)
{
	auto w = widget<CButton>("alternativeAction");
	if(!w)
		return;
	
	std::string iconName = variables["actionIconDefault"].String();
	switch(action.get())
	{
		case PossiblePlayerBattleAction::ATTACK:
			iconName = variables["actionIconAttack"].String();
			break;
			
		case PossiblePlayerBattleAction::SHOOT:
			iconName = variables["actionIconShoot"].String();
			break;
			
		case PossiblePlayerBattleAction::AIMED_SPELL_CREATURE:
			iconName = variables["actionIconSpell"].String();
			break;
			
		//TODO: figure out purpose of this icon
		//case PossiblePlayerBattleAction::???:
			//iconName = variables["actionIconWalk"].String();
			//break;
			
		case PossiblePlayerBattleAction::ATTACK_AND_RETURN:
			iconName = variables["actionIconReturn"].String();
			break;
			
		case PossiblePlayerBattleAction::WALK_AND_ATTACK:
			iconName = variables["actionIconNoReturn"].String();
			break;
	}
		
	auto anim = std::make_shared<CAnimation>(iconName);
	w->setImage(anim, false);
	w->redraw();
}

void BattleWindow::setAlternativeActions(const std::list<PossiblePlayerBattleAction> & actions)
{
	alternativeActions = actions;
	defaultAction = PossiblePlayerBattleAction::INVALID;
	if(alternativeActions.size() > 1)
		defaultAction = alternativeActions.back();
	if(!alternativeActions.empty())
		showAlternativeActionIcon(alternativeActions.front());
	else
		showAlternativeActionIcon(defaultAction);
}

void BattleWindow::bAutofightf()
{
	if (owner.actionsController->spellcastingModeActive())
		return;

	//Stop auto-fight mode
	if(owner.curInt->isAutoFightOn)
	{
		assert(owner.curInt->autofightingAI);
		owner.curInt->isAutoFightOn = false;
		logGlobal->trace("Stopping the autofight...");
	}
	else if(!owner.curInt->autofightingAI)
	{
		owner.curInt->isAutoFightOn = true;
		blockUI(true);

		auto ai = CDynLibHandler::getNewBattleAI(settings["server"]["friendlyAI"].String());
		ai->initBattleInterface(owner.curInt->env, owner.curInt->cb);
		ai->battleStart(owner.army1, owner.army2, int3(0,0,0), owner.attackingHeroInstance, owner.defendingHeroInstance, owner.curInt->cb->battleGetMySide());
		owner.curInt->autofightingAI = ai;
		owner.curInt->cb->registerBattleInterface(ai);

		owner.requestAutofightingAIToTakeAction();
	}
}

void BattleWindow::bSpellf()
{
	if (owner.actionsController->spellcastingModeActive())
		return;

	if (!owner.makingTurn())
		return;

	auto myHero = owner.currentHero();
	if(!myHero)
		return;

	CCS->curh->set(Cursor::Map::POINTER);

	ESpellCastProblem::ESpellCastProblem spellCastProblem = owner.curInt->cb->battleCanCastSpell(myHero, spells::Mode::HERO);

	if(spellCastProblem == ESpellCastProblem::OK)
	{
		GH.pushIntT<CSpellWindow>(myHero, owner.curInt.get());
	}
	else if (spellCastProblem == ESpellCastProblem::MAGIC_IS_BLOCKED)
	{
		//TODO: move to spell mechanics, add more information to spell cast problem
		//Handle Orb of Inhibition-like effects -> we want to display dialog with info, why casting is impossible
		auto blockingBonus = owner.currentHero()->getBonusLocalFirst(Selector::type()(Bonus::BLOCK_ALL_MAGIC));
		if (!blockingBonus)
			return;

		if (blockingBonus->source == Bonus::ARTIFACT)
		{
			const auto artID = ArtifactID(blockingBonus->sid);
			//If we have artifact, put name of our hero. Otherwise assume it's the enemy.
			//TODO check who *really* is source of bonus
			std::string heroName = myHero->hasArt(artID) ? myHero->getNameTranslated() : owner.enemyHero().name;

			//%s wields the %s, an ancient artifact which creates a p dead to all magic.
			LOCPLINT->showInfoDialog(boost::str(boost::format(CGI->generaltexth->allTexts[683])
										% heroName % CGI->artifacts()->getByIndex(artID)->getNameTranslated()));
		}
	}
}

void BattleWindow::bSwitchActionf()
{
	if(alternativeActions.empty())
		return;
	
	if(alternativeActions.front() == defaultAction)
	{
		alternativeActions.push_back(alternativeActions.front());
		alternativeActions.pop_front();
	}
	
	auto actions = owner.actionsController->getPossibleActions();
	if(!actions.empty() && actions.front() == alternativeActions.front())
	{
		owner.actionsController->removePossibleAction(alternativeActions.front());
		showAlternativeActionIcon(defaultAction);
	}
	else
	{
		owner.actionsController->pushFrontPossibleAction(alternativeActions.front());
		showAlternativeActionIcon(alternativeActions.front());
	}
	
	alternativeActions.push_back(alternativeActions.front());
	alternativeActions.pop_front();
}

void BattleWindow::bWaitf()
{
	if (owner.actionsController->spellcastingModeActive())
		return;

	if (owner.stacksController->getActiveStack() != nullptr)
		owner.giveCommand(EActionType::WAIT);
}

void BattleWindow::bDefencef()
{
	if (owner.actionsController->spellcastingModeActive())
		return;

	if (owner.stacksController->getActiveStack() != nullptr)
		owner.giveCommand(EActionType::DEFEND);
}

void BattleWindow::bConsoleUpf()
{
	if (owner.actionsController->spellcastingModeActive())
		return;

	console->scrollUp();
}

void BattleWindow::bConsoleDownf()
{
	if (owner.actionsController->spellcastingModeActive())
		return;

	console->scrollDown();
}

void BattleWindow::bTacticNextStack()
{
	owner.tacticNextStack(nullptr);
}

void BattleWindow::bTacticPhaseEnd()
{
	owner.tacticPhaseEnd();
}

void BattleWindow::blockUI(bool on)
{
	bool canCastSpells = false;
	auto hero = owner.curInt->cb->battleGetMyHero();

	if(hero)
	{
		ESpellCastProblem::ESpellCastProblem spellcastingProblem = owner.curInt->cb->battleCanCastSpell(hero, spells::Mode::HERO);

		//if magic is blocked, we leave button active, so the message can be displayed after button click
		canCastSpells = spellcastingProblem == ESpellCastProblem::OK || spellcastingProblem == ESpellCastProblem::MAGIC_IS_BLOCKED;
	}

	bool canWait = owner.stacksController->getActiveStack() ? !owner.stacksController->getActiveStack()->waitedThisTurn : false;

	if(auto w = widget<CButton>("options"))
		w->block(on);
	if(auto w = widget<CButton>("flee"))
		w->block(on || !owner.curInt->cb->battleCanFlee());
	if(auto w = widget<CButton>("surrender"))
		w->block(on || owner.curInt->cb->battleGetSurrenderCost() < 0);
	if(auto w = widget<CButton>("cast"))
		w->block(on || owner.tacticsMode || !canCastSpells);
	if(auto w = widget<CButton>("wait"))
		w->block(on || owner.tacticsMode || !canWait);
	if(auto w = widget<CButton>("defence"))
		w->block(on || owner.tacticsMode);
	if(auto w = widget<CButton>("alternativeAction"))
		w->block(on || owner.tacticsMode);
	if(auto w = widget<CButton>("autofight"))
		w->block(owner.actionsController->spellcastingModeActive());

	auto btactEnd = widget<CButton>("tacticEnd");
	auto btactNext = widget<CButton>("tacticNext");
	if(owner.tacticsMode && btactEnd && btactNext)
	{
		btactNext->block(on);
		btactEnd->block(on);
	}
	else
	{
		auto bConsoleUp = widget<CButton>("consoleUp");
		auto bConsoleDown = widget<CButton>("consoleDown");
		if(bConsoleUp && bConsoleDown)
		{
			bConsoleUp->block(on);
			bConsoleDown->block(on);
		}
	}
}

std::optional<uint32_t> BattleWindow::getQueueHoveredUnitId()
{
	return queue->getHoveredUnitIdIfAny();
}

void BattleWindow::showAll(SDL_Surface *to)
{
	CIntObject::showAll(to);

	if (GH.screenDimensions().x != 800 || GH.screenDimensions().y !=600)
		CMessage::drawBorder(owner.curInt->playerID, to, pos.w+28, pos.h+29, pos.x-14, pos.y-15);
}

void BattleWindow::show(SDL_Surface *to)
{
	CIntObject::show(to);
	LOCPLINT->cingconsole->show(to);
}

void BattleWindow::close()
{
	if(GH.topInt().get() != this)
		logGlobal->error("Only top interface must be closed");
	GH.popInts(1);
}
