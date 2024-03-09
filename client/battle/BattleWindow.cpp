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
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"
#include "../windows/CSpellWindow.h"
#include "../widgets/Buttons.h"
#include "../widgets/Images.h"
#include "../windows/CMessage.h"
#include "../render/CAnimation.h"
#include "../render/Canvas.h"
#include "../render/IRenderHandler.h"
#include "../adventureMap/CInGameConsole.h"
#include "../adventureMap/TurnTimerWidget.h"

#include "../../CCallback.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/gameState/InfoAboutArmy.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/CStack.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/filesystem/ResourcePath.h"
#include "../../lib/StartInfo.h"
#include "../../lib/battle/BattleInfo.h"
#include "../../lib/CPlayerState.h"
#include "../windows/settings/SettingsMainWindow.h"

BattleWindow::BattleWindow(BattleInterface & owner):
	owner(owner),
	defaultAction(PossiblePlayerBattleAction::INVALID)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	pos.w = 800;
	pos.h = 600;
	pos = center();

	PlayerColor defenderColor = owner.getBattle()->getBattle()->getSidePlayer(BattleSide::DEFENDER);
	PlayerColor attackerColor = owner.getBattle()->getBattle()->getSidePlayer(BattleSide::ATTACKER);
	bool isDefenderHuman = defenderColor.isValidPlayer() && LOCPLINT->cb->getStartInfo()->playerInfos.at(defenderColor).isControlledByHuman();
	bool isAttackerHuman = attackerColor.isValidPlayer() && LOCPLINT->cb->getStartInfo()->playerInfos.at(attackerColor).isControlledByHuman();
	onlyOnePlayerHuman = isDefenderHuman != isAttackerHuman;

	REGISTER_BUILDER("battleConsole", &BattleWindow::buildBattleConsole);
	
	const JsonNode config(JsonPath::builtin("config/widgets/BattleWindow2.json"));
	
	addShortcut(EShortcut::GLOBAL_OPTIONS, std::bind(&BattleWindow::bOptionsf, this));
	addShortcut(EShortcut::BATTLE_SURRENDER, std::bind(&BattleWindow::bSurrenderf, this));
	addShortcut(EShortcut::BATTLE_RETREAT, std::bind(&BattleWindow::bFleef, this));
	addShortcut(EShortcut::BATTLE_AUTOCOMBAT, std::bind(&BattleWindow::bAutofightf, this));
	addShortcut(EShortcut::BATTLE_END_WITH_AUTOCOMBAT, std::bind(&BattleWindow::endWithAutocombat, this));
	addShortcut(EShortcut::BATTLE_CAST_SPELL, std::bind(&BattleWindow::bSpellf, this));
	addShortcut(EShortcut::BATTLE_WAIT, std::bind(&BattleWindow::bWaitf, this));
	addShortcut(EShortcut::BATTLE_DEFEND, std::bind(&BattleWindow::bDefencef, this));
	addShortcut(EShortcut::BATTLE_CONSOLE_UP, std::bind(&BattleWindow::bConsoleUpf, this));
	addShortcut(EShortcut::BATTLE_CONSOLE_DOWN, std::bind(&BattleWindow::bConsoleDownf, this));
	addShortcut(EShortcut::BATTLE_TACTICS_NEXT, std::bind(&BattleWindow::bTacticNextStack, this));
	addShortcut(EShortcut::BATTLE_TACTICS_END, std::bind(&BattleWindow::bTacticPhaseEnd, this));
	addShortcut(EShortcut::BATTLE_SELECT_ACTION, std::bind(&BattleWindow::bSwitchActionf, this));

	addShortcut(EShortcut::BATTLE_TOGGLE_QUEUE, [this](){ this->toggleQueueVisibility();});
	addShortcut(EShortcut::BATTLE_TOGGLE_HEROES_STATS, [this](){ this->toggleStickyHeroWindowsVisibility();});
	addShortcut(EShortcut::BATTLE_USE_CREATURE_SPELL, [this](){ this->owner.actionsController->enterCreatureCastingMode(); });
	addShortcut(EShortcut::GLOBAL_CANCEL, [this](){ this->owner.actionsController->endCastingSpell(); });

	build(config);
	
	console = widget<BattleConsole>("console");

	owner.console = console;

	owner.fieldController.reset( new BattleFieldController(owner));
	owner.fieldController->createHeroes();

	createQueue();
	createStickyHeroInfoWindows();
	createTimerInfoWindows();

	if ( owner.tacticsMode )
		tacticPhaseStarted();
	else
		tacticPhaseEnded();

	addUsedEvents(LCLICK | KEYBOARD);
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

void BattleWindow::createStickyHeroInfoWindows()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	if(owner.defendingHeroInstance)
	{
		InfoAboutHero info;
		info.initFromHero(owner.defendingHeroInstance, InfoAboutHero::EInfoLevel::INBATTLE);
		defenderHeroWindow = std::make_shared<HeroInfoBasicPanel>(info, nullptr);
	}
	if(owner.attackingHeroInstance)
	{
		InfoAboutHero info;
		info.initFromHero(owner.attackingHeroInstance, InfoAboutHero::EInfoLevel::INBATTLE);
		attackerHeroWindow = std::make_shared<HeroInfoBasicPanel>(info, nullptr);
	}

	bool showInfoWindows = settings["battle"]["stickyHeroInfoWindows"].Bool();

	if(!showInfoWindows)
	{
		if(attackerHeroWindow)
			attackerHeroWindow->disable();

		if(defenderHeroWindow)
			defenderHeroWindow->disable();
	}

	setPositionInfoWindow();
}

void BattleWindow::createTimerInfoWindows()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	if(LOCPLINT->cb->getStartInfo()->turnTimerInfo.battleTimer != 0 || LOCPLINT->cb->getStartInfo()->turnTimerInfo.unitTimer != 0)
	{
		PlayerColor attacker = owner.getBattle()->sideToPlayer(BattleSide::ATTACKER);
		PlayerColor defender = owner.getBattle()->sideToPlayer(BattleSide::DEFENDER);

		if (attacker.isValidPlayer())
		{
			if (GH.screenDimensions().x >= 1000)
				attackerTimerWidget = std::make_shared<TurnTimerWidget>(Point(-92, 1), attacker);
			else
				attackerTimerWidget = std::make_shared<TurnTimerWidget>(Point(1, 135), attacker);
		}

		if (defender.isValidPlayer())
		{
			if (GH.screenDimensions().x >= 1000)
				defenderTimerWidget = std::make_shared<TurnTimerWidget>(Point(pos.w + 16, 1), defender);
			else
				defenderTimerWidget = std::make_shared<TurnTimerWidget>(Point(pos.w - 78, 135), defender);
		}
	}
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
	setPositionInfoWindow();
	GH.windows().totalRedraw();
}

void BattleWindow::showQueue()
{
	if(settings["battle"]["showQueue"].Bool() == true)
		return;

	Settings showQueue = settings.write["battle"]["showQueue"];
	showQueue->Bool() = true;

	createQueue();
	updateQueue();
	setPositionInfoWindow();
	GH.windows().totalRedraw();
}

void BattleWindow::toggleStickyHeroWindowsVisibility()
{
	if(settings["battle"]["stickyHeroInfoWindows"].Bool())
		hideStickyHeroWindows();
	else
		showStickyHeroWindows();
}

void BattleWindow::hideStickyHeroWindows()
{
	if(settings["battle"]["stickyHeroInfoWindows"].Bool() == false)
		return;

	Settings showStickyHeroInfoWindows = settings.write["battle"]["stickyHeroInfoWindows"];
	showStickyHeroInfoWindows->Bool() = false;

	if(attackerHeroWindow)
		attackerHeroWindow->disable();

	if(defenderHeroWindow)
		defenderHeroWindow->disable();

	GH.windows().totalRedraw();
}

void BattleWindow::showStickyHeroWindows()
{
	if(settings["battle"]["stickyHeroInfoWindows"].Bool() == true)
		return;

	Settings showStickyHeroInfoWindows = settings.write["battle"]["stickyHeroInfoWindows"];
	showStickyHeroInfoWindows->Bool() = true;


	createStickyHeroInfoWindows();
	GH.windows().totalRedraw();
}

void BattleWindow::updateQueue()
{
	queue->update();
}

void BattleWindow::setPositionInfoWindow()
{
	if(defenderHeroWindow)
	{
		Point position = (GH.screenDimensions().x >= 1000)
				? Point(pos.x + pos.w + 15, pos.y + 60)
				: Point(pos.x + pos.w -79, pos.y + 195);
		defenderHeroWindow->moveTo(position);
	}
	if(attackerHeroWindow)
	{
		Point position = (GH.screenDimensions().x >= 1000)
				? Point(pos.x - 93, pos.y + 60)
				: Point(pos.x + 1, pos.y + 195);
		attackerHeroWindow->moveTo(position);
	}
	if(defenderStackWindow)
	{
		Point position = (GH.screenDimensions().x >= 1000)
				? Point(pos.x + pos.w + 15, defenderHeroWindow ? defenderHeroWindow->pos.y + 210 : pos.y + 60)
				: Point(pos.x + pos.w -79, defenderHeroWindow ? defenderHeroWindow->pos.y : pos.y + 195);
		defenderStackWindow->moveTo(position);
	}
	if(attackerStackWindow)
	{
		Point position = (GH.screenDimensions().x >= 1000)
				? Point(pos.x - 93, attackerHeroWindow ? attackerHeroWindow->pos.y + 210 : pos.y + 60)
				: Point(pos.x + 1, attackerHeroWindow ? attackerHeroWindow->pos.y : pos.y + 195);
		attackerStackWindow->moveTo(position);
	}
}

void BattleWindow::updateHeroInfoWindow(uint8_t side, const InfoAboutHero & hero)
{
	std::shared_ptr<HeroInfoBasicPanel> panelToUpdate = side == 0 ? attackerHeroWindow : defenderHeroWindow;
	panelToUpdate->update(hero);
}

void BattleWindow::updateStackInfoWindow(const CStack * stack)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	bool showInfoWindows = settings["battle"]["stickyHeroInfoWindows"].Bool();

	if(stack && stack->unitSide() == BattleSide::DEFENDER)
	{
		defenderStackWindow = std::make_shared<StackInfoBasicPanel>(stack);
		defenderStackWindow->setEnabled(showInfoWindows);
	}
	else
		defenderStackWindow = nullptr;
	
	if(stack && stack->unitSide() == BattleSide::ATTACKER)
	{
		attackerStackWindow = std::make_shared<StackInfoBasicPanel>(stack);
		attackerStackWindow->setEnabled(showInfoWindows);
	}
	else
		attackerStackWindow = nullptr;
	
	setPositionInfoWindow();
}

void BattleWindow::heroManaPointsChanged(const CGHeroInstance * hero)
{
	if(hero == owner.attackingHeroInstance || hero == owner.defendingHeroInstance)
	{
		InfoAboutHero heroInfo = InfoAboutHero();
		heroInfo.initFromHero(hero, InfoAboutHero::INBATTLE);

		updateHeroInfoWindow(hero == owner.attackingHeroInstance ? 0 : 1, heroInfo);
	}
	else
	{
		logGlobal->error("BattleWindow::heroManaPointsChanged: 'Mana points changed' called for hero not belonging to current battle window");
	}
}

void BattleWindow::activate()
{
	GH.setStatusbar(console);
	CIntObject::activate();
	LOCPLINT->cingconsole->activate();
}

void BattleWindow::deactivate()
{
	GH.setStatusbar(nullptr);
	CIntObject::deactivate();
	LOCPLINT->cingconsole->deactivate();
}

bool BattleWindow::captureThisKey(EShortcut key)
{
	return owner.openingPlaying();
}

void BattleWindow::keyPressed(EShortcut key)
{
	if (owner.openingPlaying())
	{
		owner.openingEnd();
		return;
	}
	InterfaceObjectConfigurable::keyPressed(key);
}

void BattleWindow::clickPressed(const Point & cursorPosition)
{
	if (owner.openingPlaying())
	{
		owner.openingEnd();
		return;
	}
	InterfaceObjectConfigurable::clickPressed(cursorPosition);
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

	GH.windows().createAndPushWindow<SettingsMainWindow>(&owner);
}

void BattleWindow::bSurrenderf()
{
	if (owner.actionsController->spellcastingModeActive())
		return;

	int cost = owner.getBattle()->battleGetSurrenderCost();
	if(cost >= 0)
	{
		std::string enemyHeroName = owner.getBattle()->battleGetEnemyHero().name;
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

	if ( owner.getBattle()->battleCanFlee() )
	{
		auto ony = std::bind(&BattleWindow::reallyFlee,this);
		owner.curInt->showYesNoDialog(CGI->generaltexth->allTexts[28], ony, nullptr); //Are you sure you want to retreat?
	}
	else
	{
		std::vector<std::shared_ptr<CComponent>> comps;
		std::string heroName;
		//calculating fleeing hero's name
		if (owner.attackingHeroInstance)
			if (owner.attackingHeroInstance->tempOwner == owner.curInt->cb->getPlayerID())
				heroName = owner.attackingHeroInstance->getNameTranslated();
		if (owner.defendingHeroInstance)
			if (owner.defendingHeroInstance->tempOwner == owner.curInt->cb->getPlayerID())
				heroName = owner.defendingHeroInstance->getNameTranslated();
		//calculating text
		auto txt = boost::format(CGI->generaltexth->allTexts[340]) % heroName; //The Shackles of War are present.  %s can not retreat!

		//printing message
		owner.curInt->showInfoDialog(boost::str(txt), comps);
	}
}

void BattleWindow::reallyFlee()
{
	owner.giveCommand(EActionType::RETREAT);
	CCS->curh->set(Cursor::Map::POINTER);
}

void BattleWindow::reallySurrender()
{
	if (owner.curInt->cb->getResourceAmount(EGameResID::GOLD) < owner.getBattle()->battleGetSurrenderCost())
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
	
	AnimationPath iconName = AnimationPath::fromJson(variables["actionIconDefault"]);
	switch(action.get())
	{
		case PossiblePlayerBattleAction::ATTACK:
			iconName = AnimationPath::fromJson(variables["actionIconAttack"]);
			break;
			
		case PossiblePlayerBattleAction::SHOOT:
			iconName = AnimationPath::fromJson(variables["actionIconShoot"]);
			break;
			
		case PossiblePlayerBattleAction::AIMED_SPELL_CREATURE:
			iconName = AnimationPath::fromJson(variables["actionIconSpell"]);
			break;

		case PossiblePlayerBattleAction::ANY_LOCATION:
			iconName = AnimationPath::fromJson(variables["actionIconSpell"]);
			break;
			
		//TODO: figure out purpose of this icon
		//case PossiblePlayerBattleAction::???:
			//iconName = variables["actionIconWalk"].String();
			//break;
			
		case PossiblePlayerBattleAction::ATTACK_AND_RETURN:
			iconName = AnimationPath::fromJson(variables["actionIconReturn"]);
			break;
			
		case PossiblePlayerBattleAction::WALK_AND_ATTACK:
			iconName = AnimationPath::fromJson(variables["actionIconNoReturn"]);
			break;
	}

	w->setImage(iconName);
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

	if(settings["battle"]["endWithAutocombat"].Bool() && onlyOnePlayerHuman)
	{
		endWithAutocombat();
		return;
	}

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

		AutocombatPreferences autocombatPreferences = AutocombatPreferences();
		autocombatPreferences.enableSpellsUsage = settings["battle"]["enableAutocombatSpells"].Bool();

		ai->initBattleInterface(owner.curInt->env, owner.curInt->cb, autocombatPreferences);
		ai->battleStart(owner.getBattleID(), owner.army1, owner.army2, int3(0,0,0), owner.attackingHeroInstance, owner.defendingHeroInstance, owner.getBattle()->battleGetMySide(), false);
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

	ESpellCastProblem spellCastProblem = owner.getBattle()->battleCanCastSpell(myHero, spells::Mode::HERO);

	if(spellCastProblem == ESpellCastProblem::OK)
	{
		GH.windows().createAndPushWindow<CSpellWindow>(myHero, owner.curInt.get());
	}
	else if (spellCastProblem == ESpellCastProblem::MAGIC_IS_BLOCKED)
	{
		//TODO: move to spell mechanics, add more information to spell cast problem
		//Handle Orb of Inhibition-like effects -> we want to display dialog with info, why casting is impossible
		auto blockingBonus = owner.currentHero()->getFirstBonus(Selector::type()(BonusType::BLOCK_ALL_MAGIC));
		if (!blockingBonus)
			return;

		if (blockingBonus->source == BonusSource::ARTIFACT)
		{
			const auto artID = blockingBonus->sid.as<ArtifactID>();
			//If we have artifact, put name of our hero. Otherwise assume it's the enemy.
			//TODO check who *really* is source of bonus
			std::string heroName = myHero->hasArt(artID) ? myHero->getNameTranslated() : owner.enemyHero().name;

			//%s wields the %s, an ancient artifact which creates a p dead to all magic.
			LOCPLINT->showInfoDialog(boost::str(boost::format(CGI->generaltexth->allTexts[683])
										% heroName % CGI->artifacts()->getByIndex(artID)->getNameTranslated()));
		}
		else if(blockingBonus->source == BonusSource::OBJECT_TYPE)
		{
			if(blockingBonus->sid.as<MapObjectID>() == Obj::GARRISON || blockingBonus->sid.as<MapObjectID>() == Obj::GARRISON2)
				LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[684]);
		}
	}
	else
	{
		logGlobal->warn("Unexpected problem with readiness to cast spell");
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
	auto hero = owner.getBattle()->battleGetMyHero();

	if(hero)
	{
		ESpellCastProblem spellcastingProblem = owner.getBattle()->battleCanCastSpell(hero, spells::Mode::HERO);

		//if magic is blocked, we leave button active, so the message can be displayed after button click
		canCastSpells = spellcastingProblem == ESpellCastProblem::OK || spellcastingProblem == ESpellCastProblem::MAGIC_IS_BLOCKED;
	}

	bool canWait = owner.stacksController->getActiveStack() ? !owner.stacksController->getActiveStack()->waitedThisTurn : false;

	setShortcutBlocked(EShortcut::GLOBAL_OPTIONS, on);
	setShortcutBlocked(EShortcut::BATTLE_RETREAT, on || !owner.getBattle()->battleCanFlee());
	setShortcutBlocked(EShortcut::BATTLE_SURRENDER, on || owner.getBattle()->battleGetSurrenderCost() < 0);
	setShortcutBlocked(EShortcut::BATTLE_CAST_SPELL, on || owner.tacticsMode || !canCastSpells);
	setShortcutBlocked(EShortcut::BATTLE_WAIT, on || owner.tacticsMode || !canWait);
	setShortcutBlocked(EShortcut::BATTLE_DEFEND, on || owner.tacticsMode);
	setShortcutBlocked(EShortcut::BATTLE_SELECT_ACTION, on || owner.tacticsMode);
	setShortcutBlocked(EShortcut::BATTLE_AUTOCOMBAT, (settings["battle"]["endWithAutocombat"].Bool() && onlyOnePlayerHuman) ? on || owner.tacticsMode || owner.actionsController->spellcastingModeActive() : owner.actionsController->spellcastingModeActive());
	setShortcutBlocked(EShortcut::BATTLE_END_WITH_AUTOCOMBAT, on || owner.tacticsMode || !onlyOnePlayerHuman || owner.actionsController->spellcastingModeActive());
	setShortcutBlocked(EShortcut::BATTLE_TACTICS_END, on && owner.tacticsMode);
	setShortcutBlocked(EShortcut::BATTLE_TACTICS_NEXT, on && owner.tacticsMode);
	setShortcutBlocked(EShortcut::BATTLE_CONSOLE_DOWN, on && !owner.tacticsMode);
	setShortcutBlocked(EShortcut::BATTLE_CONSOLE_UP, on && !owner.tacticsMode);
}

std::optional<uint32_t> BattleWindow::getQueueHoveredUnitId()
{
	return queue->getHoveredUnitIdIfAny();
}

void BattleWindow::endWithAutocombat() 
{
	if(!owner.makingTurn() || owner.tacticsMode)
		return;

	LOCPLINT->showYesNoDialog(
		VLC->generaltexth->translate("vcmi.battleWindow.endWithAutocombat"),
		[this]()
		{
			owner.curInt->isAutoFightEndBattle = true;

			auto ai = CDynLibHandler::getNewBattleAI(settings["server"]["friendlyAI"].String());

			AutocombatPreferences autocombatPreferences = AutocombatPreferences();
			autocombatPreferences.enableSpellsUsage = settings["battle"]["enableAutocombatSpells"].Bool();

			ai->initBattleInterface(owner.curInt->env, owner.curInt->cb, autocombatPreferences);
			ai->battleStart(owner.getBattleID(), owner.army1, owner.army2, int3(0,0,0), owner.attackingHeroInstance, owner.defendingHeroInstance, owner.getBattle()->battleGetMySide(), false);

			owner.curInt->isAutoFightOn = true;
			owner.curInt->cb->registerBattleInterface(ai);
			owner.curInt->autofightingAI = ai;

			owner.requestAutofightingAIToTakeAction();

			close();

			owner.curInt->battleInt.reset();
		},
		nullptr
	);
}

void BattleWindow::showAll(Canvas & to)
{
	CIntObject::showAll(to);

	if (GH.screenDimensions().x != 800 || GH.screenDimensions().y !=600)
		CMessage::drawBorder(owner.curInt->playerID, to, pos.w+28, pos.h+29, pos.x-14, pos.y-15);
}

void BattleWindow::show(Canvas & to)
{
	CIntObject::show(to);
	LOCPLINT->cingconsole->show(to);
}

void BattleWindow::close()
{
	if(!GH.windows().isTopWindow(this))
		logGlobal->error("Only top interface must be closed");
	GH.windows().popWindows(1);
}
