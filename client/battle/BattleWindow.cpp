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

#include "../CPlayerInterface.h"
#include "../gui/CursorHandler.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"
#include "../windows/CSpellWindow.h"
#include "../widgets/Buttons.h"
#include "../widgets/Images.h"
#include "../windows/CMessage.h"
#include "../windows/CCreatureWindow.h"
#include "../render/CAnimation.h"
#include "../render/Canvas.h"
#include "../render/IRenderHandler.h"
#include "../adventureMap/CInGameConsole.h"
#include "../adventureMap/TurnTimerWidget.h"
#include "../windows/settings/SettingsMainWindow.h"

#include "../../CCallback.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/CPlayerState.h"
#include "../../lib/CStack.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/StartInfo.h"
#include "../../lib/battle/BattleInfo.h"
#include "../../lib/entities/artifact/CArtHandler.h"
#include "../../lib/filesystem/ResourcePath.h"
#include "../../lib/gameState/InfoAboutArmy.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/texts/CGeneralTextHandler.h"

BattleWindow::BattleWindow(BattleInterface & Owner):
	owner(Owner),
	lastAlternativeAction(PossiblePlayerBattleAction::INVALID)
{
	OBJECT_CONSTRUCTION;
	pos.w = 800;
	pos.h = 600;
	pos = center();

	PlayerColor defenderColor = owner.getBattle()->getBattle()->getSidePlayer(BattleSide::DEFENDER);
	PlayerColor attackerColor = owner.getBattle()->getBattle()->getSidePlayer(BattleSide::ATTACKER);
	bool isDefenderHuman = defenderColor.isValidPlayer() && GAME->interface()->cb->getStartInfo()->playerInfos.at(defenderColor).isControlledByHuman();
	bool isAttackerHuman = attackerColor.isValidPlayer() && GAME->interface()->cb->getStartInfo()->playerInfos.at(attackerColor).isControlledByHuman();
	onlyOnePlayerHuman = isDefenderHuman != isAttackerHuman;

	REGISTER_BUILDER("battleConsole", &BattleWindow::buildBattleConsole);
	
	const JsonNode config(JsonPath::builtin("config/widgets/BattleWindow2.json"));
	
	addShortcut(EShortcut::BATTLE_TOGGLE_QUICKSPELL, [this](){ this->toggleStickyQuickSpellVisibility();});
	addShortcut(EShortcut::BATTLE_SPELL_SHORTCUT_0,  [this](){ useSpellIfPossible(0);  });
	addShortcut(EShortcut::BATTLE_SPELL_SHORTCUT_1,  [this](){ useSpellIfPossible(1);  });
	addShortcut(EShortcut::BATTLE_SPELL_SHORTCUT_2,  [this](){ useSpellIfPossible(2);  });
	addShortcut(EShortcut::BATTLE_SPELL_SHORTCUT_3,  [this](){ useSpellIfPossible(3);  });
	addShortcut(EShortcut::BATTLE_SPELL_SHORTCUT_4,  [this](){ useSpellIfPossible(4);  });
	addShortcut(EShortcut::BATTLE_SPELL_SHORTCUT_5,  [this](){ useSpellIfPossible(5);  });
	addShortcut(EShortcut::BATTLE_SPELL_SHORTCUT_6,  [this](){ useSpellIfPossible(6);  });
	addShortcut(EShortcut::BATTLE_SPELL_SHORTCUT_7,  [this](){ useSpellIfPossible(7);  });
	addShortcut(EShortcut::BATTLE_SPELL_SHORTCUT_8,  [this](){ useSpellIfPossible(8);  });
	addShortcut(EShortcut::BATTLE_SPELL_SHORTCUT_9,  [this](){ useSpellIfPossible(9);  });
	addShortcut(EShortcut::BATTLE_SPELL_SHORTCUT_10, [this](){ useSpellIfPossible(10); });
	addShortcut(EShortcut::BATTLE_SPELL_SHORTCUT_11, [this](){ useSpellIfPossible(11); });

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
	addShortcut(EShortcut::BATTLE_OPEN_ACTIVE_UNIT, std::bind(&BattleWindow::bOpenActiveUnit, this));
	addShortcut(EShortcut::BATTLE_OPEN_HOVERED_UNIT, std::bind(&BattleWindow::bOpenHoveredUnit, this));

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
	createQuickSpellWindow();
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
	OBJECT_CONSTRUCTION;

	//create stack queue and adjust our own position
	bool embedQueue;
	bool showQueue = settings["battle"]["showQueue"].Bool();
	std::string queueSize = settings["battle"]["queueSize"].String();

	if(queueSize == "auto")
		embedQueue = ENGINE->screenDimensions().y < 700;
	else
		embedQueue = ENGINE->screenDimensions().y < 700 || queueSize == "small";

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
	OBJECT_CONSTRUCTION;

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

void BattleWindow::createQuickSpellWindow()
{
	OBJECT_CONSTRUCTION;

	quickSpellWindow = std::make_shared<QuickSpellPanel>(owner);
	quickSpellWindow->moveTo(Point(pos.x - 67, pos.y));

	if(settings["battle"]["enableQuickSpellPanel"].Bool())
		showStickyQuickSpellWindow();
	else
		hideStickyQuickSpellWindow();
}

void BattleWindow::toggleStickyQuickSpellVisibility()
{
	if(settings["battle"]["enableQuickSpellPanel"].Bool())
		hideStickyQuickSpellWindow();
	else
		showStickyQuickSpellWindow();
}

void BattleWindow::hideStickyQuickSpellWindow()
{
	Settings showStickyQuickSpellWindow = settings.write["battle"]["enableQuickSpellPanel"];
	showStickyQuickSpellWindow->Bool() = false;

	quickSpellWindow->disable();
	quickSpellWindow->isEnabled = false;

	setPositionInfoWindow();
	createTimerInfoWindows();
	ENGINE->windows().totalRedraw();
}

void BattleWindow::showStickyQuickSpellWindow()
{
	Settings showStickyQuickSpellWindow = settings.write["battle"]["enableQuickSpellPanel"];
	showStickyQuickSpellWindow->Bool() = true;

	auto hero = owner.getBattle()->battleGetMyHero();

	if(ENGINE->screenDimensions().x >= 1050 && hero != nullptr && hero->hasSpellbook())
	{
		quickSpellWindow->enable();
		quickSpellWindow->isEnabled = true;
	}
	else
	{
		quickSpellWindow->disable();
		quickSpellWindow->isEnabled = false;
	}

	setPositionInfoWindow();
	createTimerInfoWindows();
	ENGINE->windows().totalRedraw();
}

void BattleWindow::createTimerInfoWindows()
{
	OBJECT_CONSTRUCTION;

	int xOffsetAttacker = quickSpellWindow->isEnabled ? -53 : 0;

	if(GAME->interface()->cb->getStartInfo()->turnTimerInfo.battleTimer != 0 || GAME->interface()->cb->getStartInfo()->turnTimerInfo.unitTimer != 0)
	{
		PlayerColor attacker = owner.getBattle()->sideToPlayer(BattleSide::ATTACKER);
		PlayerColor defender = owner.getBattle()->sideToPlayer(BattleSide::DEFENDER);

		if (attacker.isValidPlayer())
		{
			if (ENGINE->screenDimensions().x >= 1000)
				attackerTimerWidget = std::make_shared<TurnTimerWidget>(Point(-92 + xOffsetAttacker, 1), attacker);
			else
				attackerTimerWidget = std::make_shared<TurnTimerWidget>(Point(1, 135), attacker);
		}

		if (defender.isValidPlayer())
		{
			if (ENGINE->screenDimensions().x >= 1000)
				defenderTimerWidget = std::make_shared<TurnTimerWidget>(Point(pos.w + 16, 1), defender);
			else
				defenderTimerWidget = std::make_shared<TurnTimerWidget>(Point(pos.w - 78, 135), defender);
		}
	}
}

std::shared_ptr<BattleConsole> BattleWindow::buildBattleConsole(const JsonNode & config) const
{
	auto rect = readRect(config["rect"]);
	auto offset = readPosition(config["imagePosition"]);
	auto background = widget<CPicture>("menuBattle");
	return std::make_shared<BattleConsole>(owner, background, rect.topLeft(), offset, rect.dimensions() );
}

void BattleWindow::useSpellIfPossible(int slot)
{
	SpellID id;
	bool fromSettings;
	std::tie(id, fromSettings) = quickSpellWindow->getSpells()[slot];

	if(id == SpellID::NONE)
		return;

	if(id.hasValue() && owner.getBattle()->battleGetMyHero() && id.toSpell()->canBeCast(owner.getBattle().get(), spells::Mode::HERO, owner.getBattle()->battleGetMyHero()))
	{
		owner.castThisSpell(id);
	}
};

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
	ENGINE->windows().totalRedraw();
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
	ENGINE->windows().totalRedraw();
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

	ENGINE->windows().totalRedraw();
}

void BattleWindow::showStickyHeroWindows()
{
	if(settings["battle"]["stickyHeroInfoWindows"].Bool() == true)
		return;

	Settings showStickyHeroInfoWindows = settings.write["battle"]["stickyHeroInfoWindows"];
	showStickyHeroInfoWindows->Bool() = true;


	createStickyHeroInfoWindows();
	ENGINE->windows().totalRedraw();
}

void BattleWindow::updateQueue()
{
	queue->update();
	createQuickSpellWindow();
}

void BattleWindow::setPositionInfoWindow()
{
	int xOffsetAttacker = quickSpellWindow->isEnabled ? -53 : 0;
	if(defenderHeroWindow)
	{
		Point position = (ENGINE->screenDimensions().x >= 1000)
				? Point(pos.x + pos.w + 15, pos.y + 60)
				: Point(pos.x + pos.w -79, pos.y + 195);
		defenderHeroWindow->moveTo(position);
	}
	if(attackerHeroWindow)
	{
		Point position = (ENGINE->screenDimensions().x >= 1000)
				? Point(pos.x - 93 + xOffsetAttacker, pos.y + 60)
				: Point(pos.x + 1, pos.y + 195);
		attackerHeroWindow->moveTo(position);
	}
	if(defenderStackWindow)
	{
		Point position = (ENGINE->screenDimensions().x >= 1000)
				? Point(pos.x + pos.w + 15, defenderHeroWindow ? defenderHeroWindow->pos.y + 210 : pos.y + 60)
				: Point(pos.x + pos.w -79, defenderHeroWindow ? defenderHeroWindow->pos.y : pos.y + 195);
		defenderStackWindow->moveTo(position);
	}
	if(attackerStackWindow)
	{
		Point position = (ENGINE->screenDimensions().x >= 1000)
				? Point(pos.x - 93 + xOffsetAttacker, attackerHeroWindow ? attackerHeroWindow->pos.y + 210 : pos.y + 60)
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
	OBJECT_CONSTRUCTION;

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
	createTimerInfoWindows();
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
	ENGINE->setStatusbar(console);
	CIntObject::activate();
	GAME->interface()->cingconsole->activate();
}

void BattleWindow::deactivate()
{
	ENGINE->setStatusbar(nullptr);
	CIntObject::deactivate();
	GAME->interface()->cingconsole->deactivate();
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
	if (owner.actionsController->heroSpellcastingModeActive())
		return;

	ENGINE->cursor().set(Cursor::Map::POINTER);

	ENGINE->windows().createAndPushWindow<SettingsMainWindow>(&owner);
}

void BattleWindow::bSurrenderf()
{
	if (owner.actionsController->heroSpellcastingModeActive())
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

		std::string surrenderMessage = boost::str(boost::format(LIBRARY->generaltexth->allTexts[32]) % enemyHeroName % cost); //%s states: "I will accept your surrender and grant you and your troops safe passage for the price of %d gold."
		owner.curInt->showYesNoDialog(surrenderMessage, [this](){ reallySurrender(); }, nullptr);
	}
}

void BattleWindow::bFleef()
{
	if (owner.actionsController->heroSpellcastingModeActive())
		return;

	if ( owner.getBattle()->battleCanFlee() )
	{
		auto ony = std::bind(&BattleWindow::reallyFlee,this);
		owner.curInt->showYesNoDialog(LIBRARY->generaltexth->allTexts[28], ony, nullptr); //Are you sure you want to retreat?
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
		auto txt = boost::format(LIBRARY->generaltexth->allTexts[340]) % heroName; //The Shackles of War are present.  %s can not retreat!

		//printing message
		owner.curInt->showInfoDialog(boost::str(txt), comps);
	}
}

void BattleWindow::reallyFlee()
{
	owner.giveCommand(EActionType::RETREAT);
	ENGINE->cursor().set(Cursor::Map::POINTER);
}

void BattleWindow::reallySurrender()
{
	if (owner.curInt->cb->getResourceAmount(EGameResID::GOLD) < owner.getBattle()->battleGetSurrenderCost())
	{
		owner.curInt->showInfoDialog(LIBRARY->generaltexth->allTexts[29]); //You don't have enough gold!
	}
	else
	{
		owner.giveCommand(EActionType::SURRENDER);
		ENGINE->cursor().set(Cursor::Map::POINTER);
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
	assert(actions.size() != 1);

	alternativeActions = actions;
	lastAlternativeAction = PossiblePlayerBattleAction::INVALID;

	if(alternativeActions.size() > 1)
	{
		lastAlternativeAction = alternativeActions.back();
		showAlternativeActionIcon(alternativeActions.front());
	}
	else
		showAlternativeActionIcon(PossiblePlayerBattleAction::INVALID);
}

void BattleWindow::bAutofightf()
{
	if (owner.actionsController->heroSpellcastingModeActive())
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
	if (owner.actionsController->heroSpellcastingModeActive())
		return;

	if (!owner.makingTurn())
		return;

	auto myHero = owner.currentHero();
	if(!myHero)
		return;

	ENGINE->cursor().set(Cursor::Map::POINTER);

	ESpellCastProblem spellCastProblem = owner.getBattle()->battleCanCastSpell(myHero, spells::Mode::HERO);

	if(spellCastProblem == ESpellCastProblem::OK)
	{
		ENGINE->windows().createAndPushWindow<CSpellWindow>(myHero, owner.curInt.get());
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
			std::string heroName = myHero->hasArt(artID, true) ? myHero->getNameTranslated() : owner.enemyHero().name;

			//%s wields the %s, an ancient artifact which creates a p dead to all magic.
			GAME->interface()->showInfoDialog(boost::str(boost::format(LIBRARY->generaltexth->allTexts[683])
										% heroName % LIBRARY->artifacts()->getByIndex(artID)->getNameTranslated()));
		}
		else if(blockingBonus->source == BonusSource::OBJECT_TYPE)
		{
			if(blockingBonus->sid.as<MapObjectID>() == Obj::GARRISON || blockingBonus->sid.as<MapObjectID>() == Obj::GARRISON2)
				GAME->interface()->showInfoDialog(LIBRARY->generaltexth->allTexts[684]);
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

	auto actions = owner.actionsController->getPossibleActions();

	if(!actions.empty() && actions.front() != lastAlternativeAction)
	{
		owner.actionsController->removePossibleAction(alternativeActions.front());
		showAlternativeActionIcon(*std::next(alternativeActions.begin()));
	}
	else
	{
		owner.actionsController->resetCurrentStackPossibleActions();
		showAlternativeActionIcon(owner.actionsController->getPossibleActions().front());
	}
	
	alternativeActions.push_back(alternativeActions.front());
	alternativeActions.pop_front();
}

void BattleWindow::bWaitf()
{
	if (owner.actionsController->heroSpellcastingModeActive())
		return;

	if (owner.stacksController->getActiveStack() != nullptr)
		owner.giveCommand(EActionType::WAIT);
}

void BattleWindow::bDefencef()
{
	if (owner.actionsController->heroSpellcastingModeActive())
		return;

	if (owner.stacksController->getActiveStack() != nullptr)
		owner.giveCommand(EActionType::DEFEND);
}

void BattleWindow::bConsoleUpf()
{
	if (owner.actionsController->heroSpellcastingModeActive())
		return;

	console->scrollUp();
}

void BattleWindow::bConsoleDownf()
{
	if (owner.actionsController->heroSpellcastingModeActive())
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
	setShortcutBlocked(EShortcut::BATTLE_OPEN_ACTIVE_UNIT, on);
	setShortcutBlocked(EShortcut::BATTLE_OPEN_HOVERED_UNIT, on);
	setShortcutBlocked(EShortcut::BATTLE_RETREAT, on || !owner.getBattle()->battleCanFlee());
	setShortcutBlocked(EShortcut::BATTLE_SURRENDER, on || owner.getBattle()->battleGetSurrenderCost() < 0);
	setShortcutBlocked(EShortcut::BATTLE_CAST_SPELL, on || owner.tacticsMode || !canCastSpells);
	setShortcutBlocked(EShortcut::BATTLE_WAIT, on || owner.tacticsMode || !canWait);
	setShortcutBlocked(EShortcut::BATTLE_DEFEND, on || owner.tacticsMode);
	setShortcutBlocked(EShortcut::BATTLE_SELECT_ACTION, on || owner.tacticsMode);
	setShortcutBlocked(EShortcut::BATTLE_AUTOCOMBAT, (settings["battle"]["endWithAutocombat"].Bool() && onlyOnePlayerHuman) ? on || owner.tacticsMode || owner.actionsController->heroSpellcastingModeActive() : owner.actionsController->heroSpellcastingModeActive());
	setShortcutBlocked(EShortcut::BATTLE_END_WITH_AUTOCOMBAT, on || owner.tacticsMode || !onlyOnePlayerHuman || owner.actionsController->heroSpellcastingModeActive());
	setShortcutBlocked(EShortcut::BATTLE_TACTICS_END, on || !owner.tacticsMode);
	setShortcutBlocked(EShortcut::BATTLE_TACTICS_NEXT, on || !owner.tacticsMode);
	setShortcutBlocked(EShortcut::BATTLE_CONSOLE_DOWN, on && !owner.tacticsMode);
	setShortcutBlocked(EShortcut::BATTLE_CONSOLE_UP, on && !owner.tacticsMode);

	quickSpellWindow->setInputEnabled(!on);
}

void BattleWindow::bOpenActiveUnit()
{
	const auto * unit = owner.stacksController->getActiveStack();

	if (unit)
		ENGINE->windows().createAndPushWindow<CStackWindow>(unit, false);
}

void BattleWindow::bOpenHoveredUnit()
{
	const auto units = owner.stacksController->getHoveredStacksUnitIds();

	if (!units.empty())
	{
		const auto * unit = owner.getBattle()->battleGetStackByID(units[0]);
		if (unit)
			ENGINE->windows().createAndPushWindow<CStackWindow>(unit, false);
	}
}

std::optional<uint32_t> BattleWindow::getQueueHoveredUnitId()
{
	return queue->getHoveredUnitIdIfAny();
}

void BattleWindow::endWithAutocombat() 
{
	if(!owner.makingTurn() || owner.tacticsMode)
		return;

	GAME->interface()->showYesNoDialog(
		LIBRARY->generaltexth->translate("vcmi.battleWindow.endWithAutocombat"),
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

	if (ENGINE->screenDimensions().x != 800 || ENGINE->screenDimensions().y !=600)
		CMessage::drawBorder(owner.curInt->playerID, to, pos.w+28, pos.h+29, pos.x-14, pos.y-15);
}

void BattleWindow::show(Canvas & to)
{
	CIntObject::show(to);
	GAME->interface()->cingconsole->show(to);
}

void BattleWindow::close()
{
	if(!ENGINE->windows().isTopWindow(this))
		logGlobal->error("Only top interface must be closed");
	ENGINE->windows().popWindows(1);
}
