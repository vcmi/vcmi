/*
 * BattleControlPanel.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleControlPanel.h"

#include "BattleInterface.h"
#include "BattleInterfaceClasses.h"
#include "BattleStacksController.h"
#include "BattleActionsController.h"

#include "../CGameInfo.h"
#include "../CPlayerInterface.h"
#include "../gui/CCursorHandler.h"
#include "../gui/CGuiHandler.h"
#include "../windows/CSpellWindow.h"
#include "../widgets/Buttons.h"
#include "../widgets/Images.h"

#include "../../CCallback.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/CStack.h"
#include "../../lib/CConfigHandler.h"

BattleControlPanel::BattleControlPanel(BattleInterface & owner, const Point & position):
	owner(owner)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	pos += position;

	//preparing buttons and console
	bOptions = std::make_shared<CButton>    (Point(  3,  5), "icm003.def", CGI->generaltexth->zelp[381], std::bind(&BattleControlPanel::bOptionsf,this), SDLK_o);
	bSurrender = std::make_shared<CButton>  (Point( 54,  5), "icm001.def", CGI->generaltexth->zelp[379], std::bind(&BattleControlPanel::bSurrenderf,this), SDLK_s);
	bFlee = std::make_shared<CButton>       (Point(105,  5), "icm002.def", CGI->generaltexth->zelp[380], std::bind(&BattleControlPanel::bFleef,this), SDLK_r);
	bAutofight = std::make_shared<CButton>  (Point(158,  5), "icm004.def", CGI->generaltexth->zelp[382], std::bind(&BattleControlPanel::bAutofightf,this), SDLK_a);
	bSpell = std::make_shared<CButton>      (Point(645,  5), "icm005.def", CGI->generaltexth->zelp[385], std::bind(&BattleControlPanel::bSpellf,this), SDLK_c);
	bWait = std::make_shared<CButton>       (Point(696,  5), "icm006.def", CGI->generaltexth->zelp[386], std::bind(&BattleControlPanel::bWaitf,this), SDLK_w);
	bDefence = std::make_shared<CButton>    (Point(747,  5), "icm007.def", CGI->generaltexth->zelp[387], std::bind(&BattleControlPanel::bDefencef,this), SDLK_d);
	bConsoleUp = std::make_shared<CButton>  (Point(624,  5), "ComSlide.def", std::make_pair("", ""),     std::bind(&BattleControlPanel::bConsoleUpf,this), SDLK_UP);
	bConsoleDown = std::make_shared<CButton>(Point(624, 24), "ComSlide.def", std::make_pair("", ""),     std::bind(&BattleControlPanel::bConsoleDownf,this), SDLK_DOWN);

	bDefence->assignedKeys.insert(SDLK_SPACE);
	bConsoleUp->setImageOrder(0, 1, 0, 0);
	bConsoleDown->setImageOrder(2, 3, 2, 2);

	console = std::make_shared<BattleConsole>(Rect(211, 4, 406,38));
	GH.statusbar = console;

	if ( owner.tacticsMode )
		tacticPhaseStarted();
	else
		tacticPhaseEnded();
}

void BattleControlPanel::show(SDL_Surface * to)
{
	//show menu before all other elements to keep it in background
	menu->show(to);
	CIntObject::show(to);
}

void BattleControlPanel::showAll(SDL_Surface * to)
{
	//show menu before all other elements to keep it in background
	menu->showAll(to);
	CIntObject::showAll(to);
}


void BattleControlPanel::tacticPhaseStarted()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	btactNext = std::make_shared<CButton>(Point(213, 4), "icm011.def", std::make_pair("", ""), [&]() { bTacticNextStack();}, SDLK_SPACE);
	btactEnd = std::make_shared<CButton>(Point(419,  4), "icm012.def", std::make_pair("", ""),  [&](){ bTacticPhaseEnd();}, SDLK_RETURN);
	menu = std::make_shared<CPicture>("COPLACBR.BMP", 0, 0);
	menu->colorize(owner.curInt->playerID);
	menu->recActions &= ~(SHOWALL | UPDATE);
}
void BattleControlPanel::tacticPhaseEnded()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	btactNext.reset();
	btactEnd.reset();

	menu = std::make_shared<CPicture>("CBAR.BMP", 0, 0);
	menu->colorize(owner.curInt->playerID);
	menu->recActions &= ~(SHOWALL | UPDATE);
}

void BattleControlPanel::bOptionsf()
{
	if (owner.actionsController->spellcastingModeActive())
		return;

	CCS->curh->set(Cursor::Map::POINTER);

	GH.pushIntT<BattleOptionsWindow>(owner);
}

void BattleControlPanel::bSurrenderf()
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

void BattleControlPanel::bFleef()
{
	if (owner.actionsController->spellcastingModeActive())
		return;

	if ( owner.curInt->cb->battleCanFlee() )
	{
		CFunctionList<void()> ony = std::bind(&BattleControlPanel::reallyFlee,this);
		owner.curInt->showYesNoDialog(CGI->generaltexth->allTexts[28], ony, nullptr); //Are you sure you want to retreat?
	}
	else
	{
		std::vector<std::shared_ptr<CComponent>> comps;
		std::string heroName;
		//calculating fleeing hero's name
		if (owner.attackingHeroInstance)
			if (owner.attackingHeroInstance->tempOwner == owner.curInt->cb->getMyColor())
				heroName = owner.attackingHeroInstance->name;
		if (owner.defendingHeroInstance)
			if (owner.defendingHeroInstance->tempOwner == owner.curInt->cb->getMyColor())
				heroName = owner.defendingHeroInstance->name;
		//calculating text
		auto txt = boost::format(CGI->generaltexth->allTexts[340]) % heroName; //The Shackles of War are present.  %s can not retreat!

		//printing message
		owner.curInt->showInfoDialog(boost::to_string(txt), comps);
	}
}

void BattleControlPanel::reallyFlee()
{
	owner.giveCommand(EActionType::RETREAT);
	CCS->curh->set(Cursor::Map::POINTER);
}

void BattleControlPanel::reallySurrender()
{
	if (owner.curInt->cb->getResourceAmount(Res::GOLD) < owner.curInt->cb->battleGetSurrenderCost())
	{
		owner.curInt->showInfoDialog(CGI->generaltexth->allTexts[29]); //You don't have enough gold!
	}
	else
	{
		owner.giveCommand(EActionType::SURRENDER);
		CCS->curh->set(Cursor::Map::POINTER);
	}
}

void BattleControlPanel::bAutofightf()
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
		ai->init(owner.curInt->env, owner.curInt->cb);
		ai->battleStart(owner.army1, owner.army2, int3(0,0,0), owner.attackingHeroInstance, owner.defendingHeroInstance, owner.curInt->cb->battleGetMySide());
		owner.curInt->autofightingAI = ai;
		owner.curInt->cb->registerBattleInterface(ai);

		owner.requestAutofightingAIToTakeAction();
	}
}

void BattleControlPanel::bSpellf()
{
	if (owner.actionsController->spellcastingModeActive())
		return;

	if (!owner.myTurn)
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
			std::string heroName = myHero->hasArt(artID) ? myHero->name : owner.enemyHero().name;

			//%s wields the %s, an ancient artifact which creates a p dead to all magic.
			LOCPLINT->showInfoDialog(boost::str(boost::format(CGI->generaltexth->allTexts[683])
										% heroName % CGI->artifacts()->getByIndex(artID)->getName()));
		}
	}
}

void BattleControlPanel::bWaitf()
{
	if (owner.actionsController->spellcastingModeActive())
		return;

	if (owner.stacksController->getActiveStack() != nullptr)
		owner.giveCommand(EActionType::WAIT);
}

void BattleControlPanel::bDefencef()
{
	if (owner.actionsController->spellcastingModeActive())
		return;

	if (owner.stacksController->getActiveStack() != nullptr)
		owner.giveCommand(EActionType::DEFEND);
}

void BattleControlPanel::bConsoleUpf()
{
	if (owner.actionsController->spellcastingModeActive())
		return;

	console->scrollUp();
}

void BattleControlPanel::bConsoleDownf()
{
	if (owner.actionsController->spellcastingModeActive())
		return;

	console->scrollDown();
}

void BattleControlPanel::bTacticNextStack()
{
	owner.tacticNextStack(nullptr);
}

void BattleControlPanel::bTacticPhaseEnd()
{
	owner.tacticPhaseEnd();
}

void BattleControlPanel::blockUI(bool on)
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

	bOptions->block(on);
	bFlee->block(on || !owner.curInt->cb->battleCanFlee());
	bSurrender->block(on || owner.curInt->cb->battleGetSurrenderCost() < 0);

	// block only if during enemy turn and auto-fight is off
	// otherwise - crash on accessing non-exisiting active stack
	bAutofight->block(!owner.curInt->isAutoFightOn && !owner.stacksController->getActiveStack());

	if (owner.tacticsMode && btactEnd && btactNext)
	{
		btactNext->block(on);
		btactEnd->block(on);
	}
	else
	{
		bConsoleUp->block(on);
		bConsoleDown->block(on);
	}


	bSpell->block(on || owner.tacticsMode || !canCastSpells);
	bWait->block(on || owner.tacticsMode || !canWait);
	bDefence->block(on || owner.tacticsMode);
}
